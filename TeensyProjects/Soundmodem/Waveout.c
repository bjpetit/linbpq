//
//	Passes audio samples to the sound interface

//	Windows uses WaveOut

//	Nucleo uses DMA

//	Linux will use ALSA

//	This is the Windows Version

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "winmm.lib")
void printtick(char * msg);
void PollReceivedSamples();
short * SendtoCard(unsigned short * buf, int n);

HANDLE OpenCOMPort(VOID * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits);
VOID COMSetDTR(HANDLE fd);
VOID COMClearDTR(HANDLE fd);
VOID COMSetRTS(HANDLE fd);
VOID COMClearRTS(HANDLE fd);
BOOL KeyPTT(BOOL blnPTT);
void pktProcessNewSamples(short * buf, int count);

VOID WriteDebugLog(int LogLevel, const char * format, ...);

#include <math.h>

#define Now getTicks()

void GetSoundDevices();

BOOL gotGPIO = FALSE;
BOOL useGPIO = FALSE;

int pttGPIOPin = -1;

extern int port;

HANDLE hPTTDevice = 0;			// port for PTT
char PTTPORT[80] = "";			// Port for Hardware PTT - may be same as control port.

#define PTTRTS		1
#define PTTDTR		2
#define PTTCI_V		4		// Not used here (but may be later)

int PTTMode = PTTRTS;				// PTT Control Flags.

int RadioControl = 0;

int Capturing = 1;


// Windows works with signed samples +- 32767
// STM32 DAC uses unsigned 0 - 4095

// Currently use 1200 samples for TX but 480 for RX to reduce latency

#define SendSize 2400		// ?? needed for 48KHz 9600 ??
#define ReceiveSize 1200

#define NumberofinBuffers 2

short buffer[2][SendSize];		// Two Transfer/DMA buffers of 0.1 Sec
short inbuffer[NumberofinBuffers][ReceiveSize];	// Input Transfer/ buffers of 0.1 Sec

short * DMABuffer = &buffer[0][0];

int Number;				// Number of samples waiting to be sent

BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

char CaptureDevice[80] = "0"; //"2";
char PlaybackDevice[80] = "0"; //"1";

char * CaptureDevices = NULL;
char * PlaybackDevices = NULL;

int CaptureCount = 0;
int PlaybackCount = 0;

int CaptureIndex = -1;		// Card number
int PlayBackIndex = -1;


char CaptureNames[16][MAXPNAMELEN + 2]= {""};
char PlaybackNames[16][MAXPNAMELEN + 2]= {""};

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 12000,12000, 2, 16, 0 };

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

WAVEHDR header[2] =
{
	{(char *)buffer[0], 0, 0, 0, 0, 0, 0, 0},
	{(char *)buffer[1], 0, 0, 0, 0, 0, 0, 0}
};

WAVEHDR inheader[5] =
{
	{(char *)inbuffer[0], 0, 0, 0, 0, 0, 0, 0},
	{(char *)inbuffer[1], 0, 0, 0, 0, 0, 0, 0},
	{(char *)inbuffer[2], 0, 0, 0, 0, 0, 0, 0},
	{(char *)inbuffer[3], 0, 0, 0, 0, 0, 0, 0},
	{(char *)inbuffer[4], 0, 0, 0, 0, 0, 0, 0}
};

WAVEOUTCAPS pwoc;
WAVEINCAPS pwic;

void InitSound(int SampleRate, BOOL Quiet);
void HostPoll();

int Ticks;

LARGE_INTEGER Frequency;
LARGE_INTEGER StartTicks;
LARGE_INTEGER NewTicks;

int LastNow;

extern void Generate50BaudTwoToneLeaderTemplate();
extern BOOL blnDISCRepeating;

#define TARGET_RESOLUTION 1         // 1-millisecond target resolution

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}

void wavmain(int argc, char * argv[])
{

	TIMECAPS tc;
	UINT     wTimerRes;
	DWORD	t, lastt = 0;

//	Generate50BaudTwoToneLeaderTemplate();
//	GeneratePSKTemplates();

	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) 
	{
	    // Error; application can't continue.
	}

	wTimerRes = min(max(tc.wPeriodMin, TARGET_RESOLUTION), tc.wPeriodMax);
	timeBeginPeriod(wTimerRes); 

	t = timeGetTime();

	if (argc > 1)
		port = atoi(argv[1]);
	
	if (argc > 3)
	{
		strcpy(CaptureDevice, argv[2]);
		strcpy(PlaybackDevice, argv[3]);
		_strupr(CaptureDevice);
		_strupr(PlaybackDevice);
	}

	if (argc > 4)
		strcpy(PTTPORT, argv[4]);

	if (PTTPORT[0])
		hPTTDevice = OpenCOMPort(PTTPORT, 19200, FALSE, FALSE, FALSE, 0);

	if (hPTTDevice)
	{
		COMClearRTS(hPTTDevice);
		COMClearDTR(hPTTDevice);
		printf("Using RTS on port %s for PTT", PTTPORT); 
		RadioControl = TRUE;
	}

	QueryPerformanceFrequency(&Frequency);
	Frequency.QuadPart /= 1000;			// Microsecs
	QueryPerformanceCounter(&StartTicks);

	GetSoundDevices();
	
	if(!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
		printf("Failed to set High Priority (%d)\n"), GetLastError();
}

unsigned int getTicks()
{
	return timeGetTime();
//		QueryPerformanceCounter(&NewTicks);
//		return (int)(NewTicks.QuadPart - StartTicks.QuadPart) / Frequency.QuadPart;
}

void printtick(char * msg)
{
	QueryPerformanceCounter(&NewTicks);
	printf("%s %i\r\n", msg, Now - LastNow);
	LastNow = Now;
}

void txSleep(int mS)
{
	// called while waiting for next TX buffer. Run background processes

	PollReceivedSamples();			// discard any received samples
	HostPoll();	
	Sleep(mS);
}

int PriorSize = 0;

int Index = 0;				// DMA TX Buffer being used 0 or 1
int inIndex = 0;			// DMA Buffer being used


FILE * wavfp1;

BOOL DMARunning = FALSE;		// Used to start DMA on first write

int totSamples = 0;

void SampleSink(short Sample)
{
	totSamples++;

#ifdef TEENSY	
	int work = (short)Sample);
	DMABuffer[Number++] = (work + 32768) >> 4; // 12 bit left justify
#else
	DMABuffer[Number++] = Sample;
#endif
	if (Number == SendSize)
	{
		// send this buffer to sound interface

		DMABuffer = SendtoCard(DMABuffer, SendSize);
		Number = 0;
	}
}



short * SendtoCard(unsigned short * buf, int n)
{
	if (Loopback)
	{
		// Loop back   to decode for testing

		pktProcessNewSamples(buf, 1200);		// signed
	}

//	WriteDebugLog(7, "SendtoCard %d", n);


	header[Index].dwBufferLength = n * 2;

	waveOutPrepareHeader(hWaveOut, &header[Index], sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header[Index], sizeof(WAVEHDR));

	// wait till previous buffer is complete

	while (!(header[!Index].dwFlags & WHDR_DONE))
	{
		txSleep(1);				// Run buckground while waiting 
	}

	waveOutUnprepareHeader(hWaveOut, &header[!Index], sizeof(WAVEHDR));
	Index = !Index;

	return &buffer[Index][0];
}


//		// This generates a nice musical pattern for sound interface testing
//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);

void GetSoundDevices()
{
	int i;

	printf("Capture Devices\r\n");

	CaptureCount = waveInGetNumDevs();

	CaptureDevices = malloc((MAXPNAMELEN + 2) * CaptureCount);
	CaptureDevices[0] = 0;
	
	for (i = 0; i < CaptureCount; i++)
	{
		waveInOpen(&hWaveIn, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveInGetDevCaps((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPS));

		if (CaptureDevices)
			strcat(CaptureDevices, ",");
		strcat(CaptureDevices, pwic.szPname);
		printf("%d %s\r\n", i, pwic.szPname);
		memcpy(&CaptureNames[i][0], pwic.szPname, MAXPNAMELEN);
		_strupr(&CaptureNames[i][0]);
		waveInClose(hWaveIn);
	}

	printf("Playback Devices\r\n");

	PlaybackCount = waveOutGetNumDevs();

	PlaybackDevices = malloc((MAXPNAMELEN + 2) * PlaybackCount);
	PlaybackDevices[0] = 0;

	for (i = 0; i < PlaybackCount; i++)
	{
		waveOutOpen(&hWaveOut, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveOutGetDevCaps((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPS));

		if (PlaybackDevices[0])
			strcat(PlaybackDevices, ",");
		strcat(PlaybackDevices, pwoc.szPname);
		printf("%i %s\r\n", i, pwoc.szPname);
		memcpy(&PlaybackNames[i][0], pwoc.szPname, MAXPNAMELEN);
		_strupr(&PlaybackNames[i][0]);
		waveOutClose(hWaveOut);
	}
}


void InitSound(int SampleRate, BOOL Report)
{
	int i, ret;

	wfx.nSamplesPerSec = SampleRate;

	header[0].dwFlags = WHDR_DONE;
	header[1].dwFlags = WHDR_DONE;

	if (strlen(PlaybackDevice) <= 2)
		PlayBackIndex = atoi(PlaybackDevice);
	else
	{
		// Name instead of number. Look for a substring match

		for (i = 0; i < PlaybackCount; i++)
		{
			if (strstr(&PlaybackNames[i][0], PlaybackDevice))
			{
				PlayBackIndex = i;
				break;
			}
		}
	}

    ret = waveOutOpen(&hWaveOut, PlayBackIndex, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER

	if (ret)
		printf("Failed to open WaveOut Device %s Error %d\r\n", PlaybackDevice, ret);
	else
	{
		ret = waveOutGetDevCaps((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPS));
		if (Report)
			printf("Opened WaveOut Device %s Sample Rate %d\r\n", pwoc.szPname, SampleRate);
	}

	if (strlen(CaptureDevice) <= 2)
		CaptureIndex = atoi(CaptureDevice);
	else
	{
		// Name instead of number. Look for a substring match

		for (i = 0; i < CaptureCount; i++)
		{
			if (strstr(&CaptureNames[i][0], CaptureDevice))
			{
				CaptureIndex = i;
				break;
			}
		}
	}

    ret = waveInOpen(&hWaveIn, CaptureIndex, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
	if (ret)
		printf("Failed to open WaveIn Device %s Error %d\r\n", CaptureDevice, ret);
	else
	{
		ret = waveInGetDevCaps((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPS));
		if (Report)
			printf("Opened WaveIn Device %s Sample Rate %d\n", pwic.szPname, SampleRate);
	}

//	wavfp1 = fopen("s:\\textxxx.wav", "wb");

	for (i = 0; i < NumberofinBuffers; i++)
	{
		inheader[i].dwBufferLength = ReceiveSize * 2;

		ret = waveInPrepareHeader(hWaveIn, &inheader[i], sizeof(WAVEHDR));
		ret = waveInAddBuffer(hWaveIn, &inheader[i], sizeof(WAVEHDR));
	}

	ret = waveInStart(hWaveIn);

	Index = 0;
	DMABuffer = &buffer[0][0];

}

int min = 0, max = 0, leveltimer = 0;

void PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data

	if (inheader[inIndex].dwFlags & WHDR_DONE)
	{
		short * ptr = &inbuffer[inIndex][0];
		int i;

		for (i = 0; i < ReceiveSize; i++)
		{
			if (*(ptr) < min)
				min = *ptr;
			else if (*(ptr) > max)
				max = *ptr;
			ptr++;
		}
		leveltimer++;

		if (leveltimer > 100)
		{
			leveltimer = 0;
			printf("Input peaks = %d, %d\n", min, max);
			min = max = 0;
		}

//		WriteDebugLog(LOGDEBUG, "Process %d %d", inIndex, inheader[inIndex].dwBytesRecorded/2);
		if (Capturing && Loopback == FALSE)
			pktProcessNewSamples(&inbuffer[inIndex][0], inheader[inIndex].dwBytesRecorded/2);

		waveInUnprepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		inheader[inIndex].dwFlags = 0;
		waveInPrepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));

		inIndex++;
		
		if (inIndex == NumberofinBuffers)
			inIndex = 0;
	}
}


void StopCapture()
{
	Capturing = FALSE;

//	waveInStop(hWaveIn);
//	WriteDebugLog(LOGDEBUG, "Stop Capture");
}

void DiscardOldSamples();
void ClearAllMixedSamples();

void StartCapture()
{
	// Get rid of anything received - at 9600 can see echos
	memset (inbuffer, 0 ,NumberofinBuffers * ReceiveSize * 2);
	Capturing = TRUE;
}
void CloseSound()
{ 
	waveInClose(hWaveIn);
	waveOutClose(hWaveOut);
}

#include <stdarg.h>

FILE *logfile = NULL;

VOID CloseDebugLog()
{	
	if(logfile)
		fclose(logfile);
	logfile = NULL;
}

VOID WriteDebugLog(int LogLevel, const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	
	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	printf("%s", Mess);
}

VOID WriteSamples(short * buffer, int len)
{
	fwrite(buffer, 1, len * 2, wavfp1);
}

unsigned short * SoundInit()
{
	Index = 0;
	return &buffer[0][0];
}
	
//	Called at end of transmission


void SoundFlush()
{
	// Append Trailer then wait for TX to complete

	if (Loopback)
		pktProcessNewSamples(buffer[Index], Number);

	WriteDebugLog(7, "Flushing %d", Number);

	SendtoCard(buffer[Index], Number);

	//	Wait for all sound output to complete
	
	while (!(header[0].dwFlags & WHDR_DONE))
		txSleep(1);
	while (!(header[1].dwFlags & WHDR_DONE))
		txSleep(1);

	Number = 0;
	Index = 0;
	DMABuffer = &buffer[0][0];

	KeyPTT(FALSE);		 // Unkey the Transmitter

	// Clear the capture buffers. I think this is only  needed when testing
	// with audio loopback.

//	memset(&buffer[0], 0, 2400);
//	memset(&buffer[1], 0, 2400);

	StartCapture();

		//' clear the transmit label 
        //        stcStatus.BackColor = SystemColors.Control
        //        stcStatus.ControlName = "lblXmtFrame" ' clear the transmit label
        //        queTNCStatus.Enqueue(stcStatus)
        //        stcStatus.ControlName = "lblRcvFrame" ' clear the Receive label
        //        queTNCStatus.Enqueue(stcStatus)
          
	
	WriteDebugLog(7, "totSamples %d", totSamples);
	totSamples = 0;
	return;
}


void StartCodec(char * strFault)
{
	strFault[0] = 0;
	InitSound(wfx.nSamplesPerSec, FALSE);

}

void StopCodec(char * strFault)
{
	CloseSound();
	strFault[0] = 0;
}

VOID RadioPTT(BOOL PTTState)
{
	if (PTTMode & PTTRTS)
		if (PTTState)
			COMSetRTS(hPTTDevice);
		else
			COMClearRTS(hPTTDevice);

	if (PTTMode & PTTDTR)
		if (PTTState)
			COMSetDTR(hPTTDevice);
		else
			COMClearDTR(hPTTDevice);
}

BOOL KeyPTT(BOOL blnPTT)
{
	// Returns TRUE if successful False otherwise

	if (RadioControl)
		RadioPTT(blnPTT);

	return TRUE;
}

void PlatformSleep()
{
	//	Sleep to avoid using all cpu

	Sleep(10);
}

void displayState(const char * State)
{
	// Dummy for i2c display
}

void displayCall(int dirn, char * call)
{
	// Dummy for i2c display
}

HANDLE OpenCOMPort(VOID * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char szPort[80];
	BOOL fRetVal ;
	COMMTIMEOUTS  CommTimeOuts ;
	int	Err;
	char buf[100];
	HANDLE fd;
	DCB dcb;

	// if Port Name starts COM, convert to \\.\COM or ports above 10 wont work

	if ((UINT)pPort < 256)			// just a com port number
		sprintf( szPort, "\\\\.\\COM%d", pPort);

	else if (_memicmp(pPort, "COM", 3) == 0)
	{
		char * pp = (char *)pPort;
		int p = atoi(&pp[3]);
		sprintf( szPort, "\\\\.\\COM%d", p);
	}
	else
		strcpy(szPort, pPort);

	// open COMM device

	fd = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL,
                  NULL );

	if (fd == (HANDLE) -1)
	{
		if (Quiet == 0)
		{
			if (pPort < (VOID *)256)
				sprintf(buf," COM%d could not be opened \r\n ", (UINT)pPort);
			else
				sprintf(buf," %s could not be opened \r\n ", pPort);

	//		WritetoConsoleLocal(buf);
			OutputDebugString(buf);
		}
		return (FALSE);
	}

	Err = GetFileType(fd);

	// setup device buffers

	SetupComm(fd, 4096, 4096 ) ;

	// purge any information in the buffer

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	// set up for overlapped I/O

	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
	CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
//     CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutConstant = 500 ;
	SetCommTimeouts(fd, &CommTimeOuts ) ;

   dcb.DCBlength = sizeof( DCB ) ;

   GetCommState(fd, &dcb ) ;

   dcb.BaudRate = speed;
   dcb.ByteSize = 8;
   dcb.Parity = 0;
   dcb.StopBits = TWOSTOPBITS;
   dcb.StopBits = Stopbits;

	// setup hardware flow control

	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE ;

	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE ;

	// setup software flow control

   dcb.fInX = dcb.fOutX = 0;
   dcb.XonChar = 0;
   dcb.XoffChar = 0;
   dcb.XonLim = 100 ;
   dcb.XoffLim = 100 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = FALSE;

   fRetVal = SetCommState(fd, &dcb);

	if (fRetVal)
	{
		if (SetDTR)
			EscapeCommFunction(fd, SETDTR);
		if (SetRTS)
			EscapeCommFunction(fd, SETRTS);
	}
	else
	{
		if ((UINT)pPort < 256)
			sprintf(buf,"COM%d Setup Failed %d ", pPort, GetLastError());
		else
			sprintf(buf,"%s Setup Failed %d ", pPort, GetLastError());

		printf(buf);
		OutputDebugString(buf);
		CloseHandle(fd);
		return 0;
	}

	return fd;

}

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength )
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	// only try to read number of bytes in queue

	ClearCommError(fd, &dwErrorFlags, &ComStat);

	dwLength = min((DWORD) MaxLength, ComStat.cbInQue);

	if (dwLength > 0)
	{
		fReadStat = ReadFile(fd, Block, dwLength, &dwLength, NULL) ;

		if (!fReadStat)
		{
		    dwLength = 0 ;
			ClearCommError(fd, &dwErrorFlags, &ComStat ) ;
		}
	}

   return dwLength;
}


BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	BOOL        fWriteStat;
	DWORD       BytesWritten;
	DWORD       ErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(fd, Block, BytesToWrite,
	                       &BytesWritten, NULL );

	if ((!fWriteStat) || (BytesToWrite != BytesWritten))
	{
		int Err = GetLastError();
		ClearCommError(fd, &ErrorFlags, &ComStat);
		return FALSE;
	}
	return TRUE;
}

VOID CloseCOMPort(HANDLE fd)
{
	SetCommMask(fd, 0);

	// drop DTR

	COMClearDTR(fd);

	// purge any outstanding reads/writes and close device handle

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	CloseHandle(fd);
}


VOID COMSetDTR(HANDLE fd)
{
	EscapeCommFunction(fd, SETDTR);
}

VOID COMClearDTR(HANDLE fd)
{
	EscapeCommFunction(fd, CLRDTR);
}

VOID COMSetRTS(HANDLE fd)
{
	EscapeCommFunction(fd, SETRTS);
}

VOID COMClearRTS(HANDLE fd)
{
	EscapeCommFunction(fd, CLRRTS);
}

VOID displayDCD(int x)
{
}

VOID SaveEEPROM(int Reg, UCHAR Val)
{
}
