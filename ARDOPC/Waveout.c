//
//	Passes audio samples to the sound interface

//	Windows uses WaveOut

//	Nucleo uses DMA

//	Linux will probably use ALSA

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

short buffer[2][1200];			// Two Transfer/DMA buffers of 0.1 Sec
short inbuffer[2][1200];		// Two Transfer/DMA buffers of 0.1 Sec

BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

char CaptureDevice[80] = "0"; //"2";
char PlaybackDevice[80] = "0"; //"1";

char * CaptureDevices = NULL;
char * PlaybackDevices = NULL;

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 12000, 12000, 2, 16, 0 };

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

WAVEHDR header[2] =
{
	{(char *)buffer[0], 0, 0, 0, 0, 0, 0, 0},
	{(char *)buffer[1], 0, 0, 0, 0, 0, 0, 0}
};

WAVEHDR inheader[2] =
{
	{(char *)inbuffer[0], 0, 0, 0, 0, 0, 0, 0},
	{(char *)inbuffer[1], 0, 0, 0, 0, 0, 0, 0}
};

WAVEOUTCAPS pwoc;
WAVEINCAPS pwic;

void InitSound();
void HostPoll();

int Ticks;

LARGE_INTEGER Frequency;
LARGE_INTEGER StartTicks;
LARGE_INTEGER NewTicks;

int LastNow;

void main(int argc, char * argv[])
{
	if (argc > 1)
		port = atoi(argv[1]);
	
	if (argc == 4)
	{
		strcpy(CaptureDevice, argv[2]);
		strcpy(PlaybackDevice, argv[3]);
	}

	QueryPerformanceFrequency(&Frequency);
	Frequency.QuadPart /= 1000;			// Microsecs
	QueryPerformanceCounter(&StartTicks);

	GetSoundDevices();

	Debugprintf("ARDOPC listening on port %d", port);

//	xxmain();
	ardopmain();
}

unsigned int getTicks()
{
		QueryPerformanceCounter(&NewTicks);
		return (int)(NewTicks.QuadPart - StartTicks.QuadPart) / Frequency.QuadPart;
}

void printtick(char * msg)
{
	QueryPerformanceCounter(&NewTicks);
	Debugprintf("%s %i\r", msg, Now - LastNow);
	LastNow = Now;
}

void txSleep(int mS)
{
	// called while waiting for next TX buffer. Run background processes

	HostPoll();
	Sleep(mS);
}

int PriorSize = 0;

int Index = 0;				// DMA Buffer being used 0 or 1
int inIndex = 0;				// DMA Buffer being used 0 or 1

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
	int count, i;

	Debugprintf("Capture Devices");

	count = waveInGetNumDevs();

	CaptureDevices = malloc((MAXPNAMELEN + 2) * count);
	CaptureDevices[0] = 0;
	
	for (i = 0; i < count; i++)
	{
		waveInOpen(&hWaveIn, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveInGetDevCaps((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPS));

		if (CaptureDevices)
			strcat(CaptureDevices, ",");
		strcat(CaptureDevices, pwic.szPname);
		Debugprintf("%d %s", i, pwic.szPname);
		waveInClose(hWaveIn);
	}

	Debugprintf("Playback Devices");

	count = waveOutGetNumDevs();

	PlaybackDevices = malloc((MAXPNAMELEN + 2) * count);
	PlaybackDevices[0] = 0;

	for (i = 0; i < count; i++)
	{
		waveOutOpen(&hWaveOut, i, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
		waveOutGetDevCaps((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPS));

		if (PlaybackDevices[0])
			strcat(PlaybackDevices, ",");
		strcat(PlaybackDevices, pwoc.szPname);
		Debugprintf("%i %s", i, pwoc.szPname);
		waveOutClose(hWaveOut);
	}
}

void InitSound()
{
	int ret;

	header[0].dwFlags = WHDR_DONE;
	header[1].dwFlags = WHDR_DONE;

    waveOutOpen(&hWaveOut, atoi(PlaybackDevice), &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
	waveOutGetDevCaps((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPS));
	Debugprintf("Opened WaveOut Device %s", pwoc.szPname);

    waveInOpen(&hWaveIn, atoi(CaptureDevice), &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
	waveInGetDevCaps((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPS));
	Debugprintf("Opened WaveIn Device %s", pwic.szPname);

//	wavfp1 = fopen("s:\\textxxx.wav", "wb");

	inheader[0].dwBufferLength = 2400;
	inheader[1].dwBufferLength = 2400;

	ret = waveInPrepareHeader(hWaveIn, &inheader[0], sizeof(WAVEHDR));
	ret = waveInAddBuffer(hWaveIn, &inheader[0], sizeof(WAVEHDR));

	ret = waveInPrepareHeader(hWaveIn, &inheader[1], sizeof(WAVEHDR));
	ret = waveInAddBuffer(hWaveIn, &inheader[1], sizeof(WAVEHDR));

	ret = waveInStart(hWaveIn);
}

PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data

	if (inheader[inIndex].dwFlags & WHDR_DONE)
	{
//		Debugprintf("Process %d %d", inIndex, inheader[inIndex].dwBytesRecorded/2);
		if (Capturing && Loopback == FALSE)
			ProcessNewSamples(&inbuffer[inIndex][0], inheader[inIndex].dwBytesRecorded/2);

		waveInUnprepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		inheader[inIndex].dwFlags = 0;
		waveInPrepareHeader(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &inheader[inIndex], sizeof(WAVEHDR));

		inIndex = !inIndex;
	}
}


void StopCapture()
{
	Capturing = FALSE;

//	waveInStop(hWaveIn);
//	Debugprintf("Stop Capture");
}

void DiscardOldSamples();
void ClearAllMixedSamples();

void StartCapture()
{
	Capturing = TRUE;
	DiscardOldSamples();
	ClearAllMixedSamples();
	State = SearchingForLeader;

//	Debugprintf("Start Capture");
}
void CloseSound()
{ 
	waveInClose(hWaveIn);
	waveOutClose(hWaveOut);
}

#include <stdarg.h>

VOID Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

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

	SendtoCard(buffer[Index], Number * 2);

	//	Wait for all sound output to complete
	
	while (!(header[0].dwFlags & WHDR_DONE));
	while (!(header[1].dwFlags & WHDR_DONE));

	// I think we should turn round the link here. I dont see the point in
	// waiting for MainPoll

	SoundIsPlaying = FALSE;

	//'Debug.WriteLine("[tmrPoll.Tick] Play stop. Length = " & Format(Now.Subtract(dttTestStart).TotalMilliseconds, "#") & " ms")
          
//		Debugprintf("Play complete blnEnbARQRpt = %d", blnEnbARQRpt);

	if (blnEnbARQRpt > 0)	// Start Repeat Timer if frame should be repeated
		dttNextPlay = Now + intFrameRepeatInterval;

//	Debugprintf("Now %d Now - dttNextPlay 1  = %d", Now, Now - dttNextPlay);

	KeyPTT(FALSE);		 // Unkey the Transmitter

	// Clear the capture buffers. I think this is only  needed when testing
	// with audio loopback.

	memset(&buffer[0], 0, 2400);
	memset(&buffer[1], 0, 2400);

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
	InitSound();

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

	if (DebugLog) Debugprintf("[Main.KeyPTT]  PTT-%s", BoolString[blnPTT]);

	blnLastPTT = blnPTT;
	return TRUE;
}

void PlatformSleep()
{
	//	Sleep to avoid using all cpu

	Sleep(10);
}

