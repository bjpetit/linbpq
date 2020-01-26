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

#include "UZ7HOStuff.h"

#pragma comment(lib, "winmm.lib")
void printtick(char * msg);
void PollReceivedSamples();
unsigned short * SoundInit();

#include <math.h>

void GetSoundDevices();

// Windows works with signed samples +- 32767
// STM32 DAC uses unsigned 0 - 4095

// Currently use 1200 samples for TX but 480 for RX to reduce latency

short buffer[2][SendSize * 2];		// Two Transfer/DMA buffers of 0.1 Sec  (x2 for Stereo)
short inbuffer[5][ReceiveSize * 2];	// Input Transfer/ buffers of 0.1 Sec (x2 for Stereo)

extern unsigned short * DMABuffer;
extern int Number;

BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

char CaptureDevice[80] = "real"; //"2";
char PlaybackDevice[80] = "real"; //"1";

int CaptureIndex = -1;		// Card number
PlayBackIndex = -1;

BOOL UseLeft = 1;
BOOL UseRight = 1;
char LogDir[256] = "";

FILE *logfile[3] = {NULL, NULL, NULL};
char LogName[3][256] = {"ARDOPDebug", "ARDOPException", "ARDOPSession"};

char * CaptureDevices = NULL;
char * PlaybackDevices = NULL;

int CaptureCount = 0;
int PlaybackCount = 0;

char CaptureNames[16][256]= {""};
char PlaybackNames[16][256]= {""};

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 2, 12000, 48000, 4, 16, 0 };

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

WAVEOUTCAPSA pwoc;
WAVEINCAPSA pwic;

unsigned int RTC = 0;

void InitSound(BOOL Quiet);
void HostPoll();

BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite);

int Ticks;

LARGE_INTEGER Frequency;
LARGE_INTEGER StartTicks;
LARGE_INTEGER NewTicks;

int LastNow;

extern BOOL blnDISCRepeating;

#define TARGET_RESOLUTION 1         // 1-millisecond target resolution

	
VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	WriteDebugLog(LOGDEBUG, Mess);

	return;
}


void platformInit()
{
	TIMECAPS tc;
	unsigned int     wTimerRes;
	DWORD	t, lastt = 0;
	int i = 0;


	_strupr(CaptureDevice);
	_strupr(PlaybackDevice);

	QueryPerformanceFrequency(&Frequency);
	Frequency.QuadPart /= 1000;			// Microsecs
	QueryPerformanceCounter(&StartTicks);

	GetSoundDevices();

	if (!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
		printf("Failed to set High Priority (%d)\n", GetLastError());
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
	WriteDebugLog(LOGCRIT, "%s %i\r", msg, Now - LastNow);
	LastNow = Now;
}

void txSleep(int mS)
{
	// called while waiting for next TX buffer. Run background processes

	while (mS > 50)
	{
		PollReceivedSamples();			// discard any received samples

		Sleep(50);
		mS -= 50;
	}

	Sleep(mS);

	PollReceivedSamples();			// discard any received samples
}

int PriorSize = 0;

int Index = 0;				// DMA TX Buffer being used 0 or 1
int inIndex = 0;			// DMA Buffer being used


FILE * wavfp1;

BOOL DMARunning = FALSE;		// Used to start DMA on first write

short * SendtoCard(unsigned short * buf, int n)
{
	header[Index].dwBufferLength = n * 4;

	waveOutPrepareHeader(hWaveOut, &header[Index], sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header[Index], sizeof(WAVEHDR));

	// wait till previous buffer is complete

	while (!(header[!Index].dwFlags & WHDR_DONE))
	{
		txSleep(10);				// Run buckground while waiting 
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

	WriteDebugLog(LOGALERT, "Capture Devices");

	CaptureCount = waveInGetNumDevs();

	CaptureDevices = malloc((MAXPNAMELEN + 2) * CaptureCount);
	CaptureDevices[0] = 0;
	
	for (i = 0; i < CaptureCount; i++)
	{
		waveInOpen(&hWaveIn, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveInGetDevCapsA((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPSA));

		if (CaptureDevices)
			strcat(CaptureDevices, ",");
		strcat(CaptureDevices, pwic.szPname);
		WriteDebugLog(LOGALERT, "%d %s", i, pwic.szPname);
		memcpy(&CaptureNames[i][0], pwic.szPname, MAXPNAMELEN);
		_strupr(&CaptureNames[i][0]);
	}

	WriteDebugLog(LOGALERT, "Playback Devices");

	PlaybackCount = waveOutGetNumDevs();

	PlaybackDevices = malloc((MAXPNAMELEN + 2) * PlaybackCount);
	PlaybackDevices[0] = 0;

	for (i = 0; i < PlaybackCount; i++)
	{
		waveOutOpen(&hWaveOut, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveOutGetDevCapsA((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPSA));

		if (PlaybackDevices[0])
			strcat(PlaybackDevices, ",");
		strcat(PlaybackDevices, pwoc.szPname);
		WriteDebugLog(LOGALERT, "%i %s", i, pwoc.szPname);
		memcpy(&PlaybackNames[i][0], pwoc.szPname, MAXPNAMELEN);
		_strupr(&PlaybackNames[i][0]);
		waveOutClose(hWaveOut);
	}
}


void InitSound(BOOL Report)
{
	int i, t, ret;

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
		WriteDebugLog(LOGALERT, "Failed to open WaveOut Device %s Error %d", PlaybackDevice, ret);
	else
	{
		ret = waveOutGetDevCapsA((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPSA));
		if (Report)
			WriteDebugLog(LOGALERT, "Opened WaveOut Device %s", pwoc.szPname);
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
		WriteDebugLog(LOGALERT, "Failed to open WaveIn Device %s Error %d", CaptureDevice, ret);
	else
	{
		ret = waveInGetDevCapsA((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPSA));
		if (Report)
			WriteDebugLog(LOGALERT, "Opened WaveIn Device %s", pwic.szPname);
	}

//	wavfp1 = fopen("s:\\textxxx.wav", "wb");

	for (i = 0; i < NumberofinBuffers; i++)
	{
		inheader[i].dwBufferLength = ReceiveSize * 4;

		ret = waveInPrepareHeader(hWaveIn, &inheader[i], sizeof(WAVEHDR));
		ret = waveInAddBuffer(hWaveIn, &inheader[i], sizeof(WAVEHDR));
	}

	ret = waveInStart(hWaveIn);

	DMABuffer = SoundInit();


// This generates a nice musical pattern for sound interface testing

/*
for (i = 0; i < 100; i++)
{
	for (t = 0; t < SendSize ; ++t)
		SampleSink(((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xff) * 255);
}
*/

}

int min = 0, max = 0, lastlevelGUI = 0, lastlevelreport = 0;

UCHAR CurrentLevel = 0;		// Peak from current samples

void PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data

	// For level display we want a fairly rapid level average but only want to report 
	// to log every 10 secs or so

	// with Windows we get mono data

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

		CurrentLevel = ((max - min) * 75) /32768;	// Scale to 150 max

		if ((Now - lastlevelGUI) > 2000)	// 2 Secs
		{
//			if (WaterfallActive == 0 && SpectrumActive == 0)				// Don't need to send as included in Waterfall Line
//				SendtoGUI('L', &CurrentLevel, 1);	// Signal Level
			
			lastlevelGUI = Now;

			if ((Now - lastlevelreport) > 10000)	// 10 Secs
			{
				char HostCmd[64];
				lastlevelreport = Now;

				sprintf(HostCmd, "INPUTPEAKS %d %d", min, max);
				WriteDebugLog(LOGDEBUG, "Input peaks = %d, %d", min, max);

			}
			min = max = 0;
		}

//		WriteDebugLog(LOGDEBUG, "Process %d %d", inIndex, inheader[inIndex].dwBytesRecorded/2);
//		if (Capturing && Loopback == FALSE)
			ProcessNewSamples(&inbuffer[inIndex][0], inheader[inIndex].dwBytesRecorded/4);

		waveInUnprepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		inheader[inIndex].dwFlags = 0;
		waveInPrepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));

		inIndex++;
		
		if (inIndex == NumberofinBuffers)
			inIndex = 0;
	}
}



/*

// Pre GUI Version
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
			char HostCmd[64];
			leveltimer = 0;
			sprintf(HostCmd, "INPUTPEAKS %d %d", min, max);
			QueueCommandToHost(HostCmd);

			WriteDebugLog(LOGDEBUG, "Input peaks = %d, %d", min, max);
			min = max = 0;
		}

//		WriteDebugLog(LOGDEBUG, "Process %d %d", inIndex, inheader[inIndex].dwBytesRecorded/2);
		if (Capturing && Loopback == FALSE)
			ProcessNewSamples(&inbuffer[inIndex][0], inheader[inIndex].dwBytesRecorded/2);

		waveInUnprepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		inheader[inIndex].dwFlags = 0;
		waveInPrepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));

		inIndex++;
		
		if (inIndex == NumberofinBuffers)
			inIndex = 0;
	}
}
*/

void StopCapture()
{
	Capturing = FALSE;

//	waveInStop(hWaveIn);
//	WriteDebugLog(LOGDEBUG, "Stop Capture");
}

void StartCapture()
{
	Capturing = TRUE;
//	WriteDebugLog(LOGDEBUG, "Start Capture");
}
void CloseSound()
{ 
	waveInClose(hWaveIn);
	waveOutClose(hWaveOut);
}

#include <stdarg.h>

VOID CloseDebugLog()
{	
	if(logfile[0])
		fclose(logfile[0]);
	logfile[0] = NULL;
}


FILE *statslogfile = NULL;

VOID CloseStatsLog()
{
	fclose(statslogfile);
	statslogfile = NULL;
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

extern int Number;				// Number of samples waiting to be sent

void SoundFlush()
{
	// Append Trailer then wait for TX to complete

//	AddTrailer();			// add the trailer.

//	if (Loopback)
//		ProcessNewSamples(buffer[Index], Number);

	SendtoCard(buffer[Index], Number);

	//	Wait for all sound output to complete
	
	while (!(header[0].dwFlags & WHDR_DONE))
		txSleep(10);
	while (!(header[1].dwFlags & WHDR_DONE))
		txSleep(10);

	// I think we should turn round the link here. I dont see the point in
	// waiting for MainPoll

	SoundIsPlaying = FALSE;

//	StartCapture();

	return;
}


void StartCodec(char * strFault)
{
	strFault[0] = 0;
	InitSound(FALSE);

}

void StopCodec(char * strFault)
{
	CloseSound();
	strFault[0] = 0;
}

// Serial Port Stuff

HANDLE OpenCOMPort(char * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char szPort[80];
	BOOL fRetVal;
	COMMTIMEOUTS  CommTimeOuts;
	int	Err;
	char buf[100];
	HANDLE fd;
	DCB dcb;

	if (_memicmp(pPort, "COM", 3) == 0)
	{
		char * pp = (char *)pPort;
		int p = atoi(&pp[3]);
		sprintf(szPort, "\\\\.\\COM%d", p);
	}
	else	
		strcpy(szPort, pPort);
	
	// open COMM device

	fd = CreateFileA(szPort, GENERIC_READ | GENERIC_WRITE,
		0,                    // exclusive access
		NULL,                 // no security attrs
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (fd == (HANDLE)-1)
	{
		if (Quiet == 0)
		{
			sprintf(buf, " %s could not be opened \r\n ", pPort);
			OutputDebugStringA(buf);
		}
		return (FALSE);
	}

	Err = GetFileType(fd);

	// setup device buffers

	SetupComm(fd, 4096, 4096);

	// purge any information in the buffer

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT |
		PURGE_TXCLEAR | PURGE_RXCLEAR);

	// set up for overlapped I/O

	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	//     CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutConstant = 500;
	SetCommTimeouts(fd, &CommTimeOuts);

	dcb.DCBlength = sizeof(DCB);

	GetCommState(fd, &dcb);

	dcb.BaudRate = speed;
	dcb.ByteSize = 8;
	dcb.Parity = 0;
	dcb.StopBits = TWOSTOPBITS;
	dcb.StopBits = Stopbits;

	// setup hardware flow control

	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;

	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;

	// setup software flow control

	dcb.fInX = dcb.fOutX = 0;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.XonLim = 100;
	dcb.XoffLim = 100;

	// other various settings

	dcb.fBinary = TRUE;
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
		sprintf(buf, "%s Setup Failed %d ", pPort, GetLastError());

		printf(buf);
		OutputDebugStringA(buf);
		CloseHandle(fd);
		return 0;
	}

	return fd;

}

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	// only try to read number of bytes in queue

	ClearCommError(fd, &dwErrorFlags, &ComStat);

	dwLength = min((DWORD)MaxLength, ComStat.cbInQue);

	if (dwLength > 0)
	{
		fReadStat = ReadFile(fd, Block, dwLength, &dwLength, NULL);

		if (!fReadStat)
		{
			dwLength = 0;
			ClearCommError(fd, &dwErrorFlags, &ComStat);
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
		&BytesWritten, NULL);

	if ((!fWriteStat) || (BytesToWrite != BytesWritten))
	{
		int Err = GetLastError();
		ClearCommError(fd, &ErrorFlags, &ComStat);
		return FALSE;
	}
	return TRUE;
}


VOID CloseCOMPort(int fd)
{
	SetCommMask((HANDLE)fd, 0);

	// drop DTR

	COMClearDTR(fd);

	// purge any outstanding reads/writes and close device handle

	PurgeComm((HANDLE)fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	CloseHandle((HANDLE)fd);
}


VOID COMSetDTR(int fd)
{
	EscapeCommFunction((HANDLE)fd, SETDTR);
}

VOID COMClearDTR(int fd)
{
	EscapeCommFunction((HANDLE)fd, CLRDTR);
}

VOID COMSetRTS(int fd)
{
	EscapeCommFunction((HANDLE)fd, SETRTS);
}

VOID COMClearRTS(int fd)
{
	EscapeCommFunction((HANDLE)fd, CLRRTS);
}

/*

void CatWrite(char * Buffer, int Len)
{
	if (hCATDevice)
		WriteCOMBlock(hCATDevice, Buffer, Len);
}

*/


