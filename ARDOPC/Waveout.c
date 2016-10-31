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
#pragma comment(lib, "winmm.lib")
void printtick(char * msg);

#include <math.h>

#include "ARDOPC.h"

void GetSoundDevices();


// Windows works with signed samples +- 32767
// STM32 DAC uses unsigned 0 - 4095

// Currently use 1200 samples for TX but 480 for RX to reduce latency

short buffer[2][SendSize];		// Two Transfer/DMA buffers of 0.1 Sec
short inbuffer[5][ReceiveSize];	// Input Transfer/ buffers of 0.1 Sec

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

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 12000, 12000, 2, 16, 0 };

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

void InitSound(BOOL Quiet);
void HostPoll();

int Ticks;

LARGE_INTEGER Frequency;
LARGE_INTEGER StartTicks;
LARGE_INTEGER NewTicks;

int LastNow;

extern void Generate50BaudTwoToneLeaderTemplate();
extern BOOL blnDISCRepeating;

#define TARGET_RESOLUTION 1         // 1-millisecond target resolution

void main(int argc, char * argv[])
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

	WriteDebugLog("ARDOPC Version %s", ProductVersion);

	if (argc > 1)
		port = atoi(argv[1]);
	
	if (argc == 4)
	{
		strcpy(CaptureDevice, argv[2]);
		strcpy(PlaybackDevice, argv[3]);
		_strupr(CaptureDevice);
		_strupr(PlaybackDevice);
	}

	QueryPerformanceFrequency(&Frequency);
	Frequency.QuadPart /= 1000;			// Microsecs
	QueryPerformanceCounter(&StartTicks);

	GetSoundDevices();
	
	if(!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
		printf("Failed to set High Priority (%d)\n"), GetLastError();

	ardopmain();
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
	WriteDebugLog("%s %i\r", msg, Now - LastNow);
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

#define NumberofinBuffers 5

FILE * wavfp1;

BOOL DMARunning = FALSE;		// Used to start DMA on first write

short * SendtoCard(unsigned short * buf, int n)
{
	if (Loopback)
	{
		// Loop back   to decode for testing

		ProcessNewSamples(buf, 1200);		// signed
	}

	header[Index].dwBufferLength = n * 2;

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

	WriteDebugLog("Capture Devices");

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
		WriteDebugLog("%d %s", i, pwic.szPname);
		memcpy(&CaptureNames[i][0], pwic.szPname, MAXPNAMELEN);
		_strupr(&CaptureNames[i][0]);
	}

	WriteDebugLog("Playback Devices");

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
		WriteDebugLog("%i %s", i, pwoc.szPname);
		memcpy(&PlaybackNames[i][0], pwoc.szPname, MAXPNAMELEN);
		_strupr(&PlaybackNames[i][0]);
		waveOutClose(hWaveOut);
	}
}


void InitSound(BOOL Report)
{
	int i, ret;

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
		WriteDebugLog("Failed to open WaveOut Device %s Error %d", PlaybackDevice, ret);
	else
	{
		ret = waveOutGetDevCaps((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPS));
		if (Report)
			WriteDebugLog("Opened WaveOut Device %s", pwoc.szPname);
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
		WriteDebugLog("Failed to open WaveIn Device %s Error %d", CaptureDevice, ret);
	else
	{
		ret = waveInGetDevCaps((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPS));
		if (Report)
			WriteDebugLog("Opened WaveIn Device %s", pwic.szPname);
	}

//	wavfp1 = fopen("s:\\textxxx.wav", "wb");

	for (i = 0; i < NumberofinBuffers; i++)
	{
		inheader[i].dwBufferLength = ReceiveSize * 2;

		ret = waveInPrepareHeader(hWaveIn, &inheader[i], sizeof(WAVEHDR));
		ret = waveInAddBuffer(hWaveIn, &inheader[i], sizeof(WAVEHDR));
	}

	ret = waveInStart(hWaveIn);
}

int min = 0, max = 0, leveltimer = 0;

PollReceivedSamples()
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

		if (leveltimer > 1000)
		{
			leveltimer = 0;
			WriteDebugLog("Input peaks = %d, %d", min, max);
			min = max = 0;
		}

//		WriteDebugLog("Process %d %d", inIndex, inheader[inIndex].dwBytesRecorded/2);
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


void StopCapture()
{
	Capturing = FALSE;

//	waveInStop(hWaveIn);
//	WriteDebugLog("Stop Capture");
}

void DiscardOldSamples();
void ClearAllMixedSamples();

void StartCapture()
{
	Capturing = TRUE;
	DiscardOldSamples();
	ClearAllMixedSamples();
	State = SearchingForLeader;

//	WriteDebugLog("Start Capture");
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

VOID WriteDebugLog(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);
	char timebuf[128];
	UCHAR Value[100];
	SYSTEMTIME st;

	if (!DebugLog)
		return;
	
	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	printf(Mess);

	GetSystemTime(&st);
	sprintf(Value, "%s_%04d%02d%02d.log",
				"ARDOPDebug", st.wYear, st.wMonth, st.wDay);
	
	if (logfile == NULL)
	{
		sprintf(Value, "%s_%04d%02d%02d.log",
				"ARDOPDebug", st.wYear, st.wMonth, st.wDay);

		if ((logfile = fopen(Value, "ab")) == NULL)
			return;
	}
	sprintf(timebuf, "%02d:%02d:%02d.%03d ",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	fputs(timebuf, logfile);

	fputs(Mess, logfile);
	return;
}

VOID WriteExceptionLog(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);
	char timebuf[32];
	UCHAR Value[100];
	FILE *logfile = NULL;
	SYSTEMTIME st;
	
	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	printf(Mess);

	GetSystemTime(&st);
	sprintf(Value, "%s_%04d%02d%02d.log",
				"ARDOPException", st.wYear, st.wMonth, st.wDay);
	
	if ((logfile = fopen(Value, "ab")) == NULL)
		return;

	sprintf(timebuf, "%02d:%02d:%02d.%03d ",
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	fputs(timebuf, logfile);

	fputs(Mess, logfile);
	fclose(logfile);

	return;
}

FILE *statslogfile = NULL;

VOID CloseStatsLog()
{
	fclose(statslogfile);
	statslogfile = NULL;
}
VOID Statsprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);
	UCHAR Value[100];
	char timebuf[32];

	SYSTEMTIME st;

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	if (statslogfile == NULL)
	{
		GetSystemTime(&st);
		sprintf(Value, "%s_%04d%02d%02d.log",
			"ARDOPSession", st.wYear, st.wMonth, st.wDay);

		if ((statslogfile = fopen(Value, "ab")) == NULL)
			return;
		else
		{
			sprintf(timebuf, "%02d:%02d:%02d.%03d\r\n",
				st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
			fputs(timebuf, statslogfile);
		}
	}

	fputs(Mess, statslogfile);
	printf(Mess);

	return;
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

	AddTrailer();			// add the trailer.

	if (Loopback)
		ProcessNewSamples(buffer[Index], Number);

	SendtoCard(buffer[Index], Number);

	//	Wait for all sound output to complete
	
	while (!(header[0].dwFlags & WHDR_DONE))
		txSleep(10);
	while (!(header[1].dwFlags & WHDR_DONE))
		txSleep(10);

	// I think we should turn round the link here. I dont see the point in
	// waiting for MainPoll

	SoundIsPlaying = FALSE;

	//'Debug.WriteLine("[tmrPoll.Tick] Play stop. Length = " & Format(Now.Subtract(dttTestStart).TotalMilliseconds, "#") & " ms")
          
//		WriteDebugLog("Play complete blnEnbARQRpt = %d", blnEnbARQRpt);

	if (blnEnbARQRpt > 0 || blnDISCRepeating)	// Start Repeat Timer if frame should be repeated
		dttNextPlay = Now + intFrameRepeatInterval;

//	WriteDebugLog("Now %d Now - dttNextPlay 1  = %d", Now, Now - dttNextPlay);

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

VOID RadioPTT()			// No Radio Control in Windows Version (but may add later)
{
}

//  Function to send PTT TRUE or PTT FALSE comannad to Host or if local Radio control Keys radio PTT 

const char BoolString[2][6] = {"FALSE", "TRUE"};

BOOL KeyPTT(BOOL blnPTT)
{
	// Returns TRUE if successful False otherwise

	if (blnLastPTT &&  !blnPTT)
		dttStartRTMeasure = Now;	 // start a measurement on release of PTT.

	if (!RadioControl)
		if (blnPTT)
			QueueCommandToHost("PTT TRUE");
		else
			QueueCommandToHost("PTT FALSE");

	else
		RadioPTT(blnPTT);

	WriteDebugLog("[Main.KeyPTT]  PTT-%s", BoolString[blnPTT]);

	blnLastPTT = blnPTT;
	return TRUE;
}

void PlatformSleep()
{
	//	Sleep to avoid using all cpu

	Sleep(10);
}

void displayState()
{
	// Dummy for i2c display
}

void displayCall(int dirn, char * call)
{
	// Dummy for i2c display
}