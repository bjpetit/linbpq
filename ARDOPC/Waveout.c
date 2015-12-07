//
//	Passes audio samples to the sound interface

//	Windows uses WaveOut

//	Nucleo uses DMA

//	Linux will probably use ALSA

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#ifdef WIN32
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
void printtick(char * msg);
#endif

#include <math.h>

#include "ARDOPC.h"

// Windows works with signed samples +- 32767
// STM32 DAC uses unsigned 0 - 4095

#ifdef WIN32
short buffer[2][1200];			// Two Transfer/DMA buffers of 0.1 Sec
short inbuffer[2][1200];			// Two Transfer/DMA buffers of 0.1 Sec
#else
unsigned short buffer[2][1200];	// Two Transfer/DMA buffers of 0.1 Sec
unsigned short work;
#endif
BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

#ifdef WIN32

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 12000, 12000, 2, 16, 0 };

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


int Ticks;

LARGE_INTEGER Frequency;
LARGE_INTEGER StartTicks;
LARGE_INTEGER NewTicks;

int LastNow;

void main()
{
	QueryPerformanceFrequency(&Frequency);
	Frequency.QuadPart /= 1000;			// Microsecs
	QueryPerformanceCounter(&StartTicks);

	printtick("Test Start");
	Sleep(1000);
	printtick("Test End");

//	xxmain();
	ardopmain();
}
void printtick(char * msg)
{
	QueryPerformanceCounter(&NewTicks);
	Now = (NewTicks.QuadPart - StartTicks.QuadPart) /Frequency.QuadPart;
	printf("%s %i\r\n", msg, Now - LastNow);
	LastNow = Now;
}


#else
#include <stm32f4xx_dma.h>
#endif

int PriorSize = 0;

int Number = 0;				// Number waiting to be sent
int Index = 0;				// DMA Buffer being used 0 or 1
int inIndex = 0;			// DMA Buffer being used 0 or 1

int SendSize = 1200;		// 100 mS for now

// Filter State Variables

static float dblR = (float)0.9995f;	// insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
static int intN = 120;				//Length of filter 12000/100
static float dblRn;

static float dblR2;
static float dblCoef[26] = {0.0f};			// the coefficients
float dblZin = 0, dblZin_1 = 0, dblZin_2 = 0, dblZComb= 0;  // Used in the comb generator

// The resonators 
      
float dblZout_0[26] = {0.0f};	// resonator outputs
float dblZout_1[26] = {0.0f};	// resonator outputs delayed one sample
float dblZout_2[26] = {0.0f};	// resonator outputs delayed two samples

int fWidth;				// Filter BandWidth
int SampleNo;
int outCount = 0;
int first, last;		// Filter slots

float largest = 0;
float smallest = 0;

short Last120[128];

int Last120Get = 0;
int Last120Put = 120;

// initFilter is called to set up each packet. It selects filter width

FILE * wavfp1;

void initFilter(int Width)
{
	int i, j;
	fWidth = Width;
	largest = smallest = 0;
	SampleNo = 0;
	Number = 0;
	outCount = 0;
	memset(Last120, 0, 256);
	Index = 0;

	KeyPTT(TRUE);
	SoundIsPlaying = TRUE;
	StopCapture();

	Last120Get = 0;
	Last120Put = 120;

	dblRn = (float)pow(dblR, intN);
	dblR2 = (float)pow(dblR, 2);

	dblZin_2 = dblZin_1 = 0;

	switch (fWidth)
	{
	case 200:

		// implements 3 100 Hz wide sections centered on 1500 Hz  (~200 Hz wide @ - 30dB centered on 1500 Hz)

		first = 14;
		last = 16;		// 3 filter sections
		break;

	case 500:

		// implements 7 100 Hz wide sections centered on 1500 Hz  (~500 Hz wide @ - 30dB centered on 1500 Hz)

		first = 12;
		last = 18;		// 7 filter sections
		break;

	case 1000:
		
		// implements 11 100 Hz wide sections centered on 1500 Hz  (~1000 Hz wide @ - 30dB centered on 1500 Hz)

		first = 10;
		last = 20;		// 7 filter sections
		break;

	case 2000:
		
		// implements 21 100 Hz wide sections centered on 1500 Hz  (~2000 Hz wide @ - 30dB centered on 1500 Hz)

		first = 5;
		last = 25;		// 7 filter sections


	}

	for (j = first; j <= last; j++)	   // calculate output for 3 resonators 
	{
		dblZout_0[j] = 0;
		dblZout_1[j] = 0;
		dblZout_2[j] = 0;
	}

	// Initialise the coefficients

	if (dblCoef[last] == 0.0)
	{
		for (i = first; i <= last; i++)
		{
			dblCoef[i] = 2 * dblR * cosf(2 * M_PI * i / intN); // For Frequency = bin i
		}
	}
 }

BOOL DMARunning = FALSE;		// Used to start DMA on first write

void SendtoCard(int n)
{
#ifdef WIN32
	header[Index].dwBufferLength = n * 2;

	waveOutPrepareHeader(hWaveOut, &header[Index], sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header[Index], sizeof(WAVEHDR));

	// wait till previous buffer is complete

	while (!(header[!Index].dwFlags & WHDR_DONE));
	waveOutUnprepareHeader(hWaveOut, &header[!Index], sizeof(WAVEHDR));
	Index = !Index;

#else

	// Embedded

	// Start DMA if first call

	if (DMARunning == FALSE)
	{
		StartDAC();
		DMARunning = TRUE;
	}

	// wait for other DMA buffer to finish

//	printtick("Start Wait");		// FOr timing tests

	while (1)
	{
		int chan = DMA_GetCurrentMemoryTarget(DMA1_Stream5);

		if (chan == Index) 	// we've started sending current buffer
		{
			Index = !Index;
//			printtick("Stop Wait");
			return;
		}
	}
#endif
}
short loopbuff[1200];		// Temp for testing - loop sent samples to decoder

void SampleSink(short Sample)
{
	//	Filter and send to sound interface

	// This version is passed samples one at a time, as we don't have
	//	enough RAM in embedded systems to hold a full audio frame

	int intFilLen = intN / 2;
	int j;
	float intFilteredSample = 0;			//  Filtered sample

	//	We save the previous intN samples
	//	The samples are held in a cyclic buffer

	if (SampleNo < intN)
		dblZin = Sample;
	else 
		dblZin = Sample - dblRn * Last120[Last120Get];

	if (++Last120Get == 121)
		Last120Get = 0;

	//Compute the Comb

	dblZComb = dblZin - dblZin_2 * dblR2;
	dblZin_2 = dblZin_1;
	dblZin_1 = dblZin;

	// Now the resonators
		
	for (j = first; j <= last; j++)	   // calculate output for 3 or 7 resonators 
	{
		dblZout_0[j] = dblZComb + dblCoef[j] * dblZout_1[j] - dblR2 * dblZout_2[j];
		dblZout_2[j] = dblZout_1[j];
		dblZout_1[j] = dblZout_0[j];

		switch (fWidth)
		{
		case 200:

			// scale each by transition coeff and + (Even) or - (Odd) 

			if (SampleNo >= intFilLen)
			{
				if (j == 14 || j == 16)
					intFilteredSample += (float)0.7389f * dblZout_0[j];
				else
					intFilteredSample -= (float)dblZout_0[j];
			}
			break;

		case 500:

			// scale each by transition coeff and + (Even) or - (Odd) 
			// Resonators 6 and 9 scaled by .15 to get best shape and side lobe supression to - 45 dB while keeping BW at 500 Hz @ -26 dB
			// practical range of scaling .05 to .25
			// Scaling also accomodates for the filter "gain" of approx 60. 

			if (SampleNo >= intFilLen)
			{
				if (j == 12 || j == 18)
					intFilteredSample += 0.10601f * dblZout_0[j];
				else if (j == 13 || j == 17)
					intFilteredSample -= 0.59383f * dblZout_0[j];
				else if ((j & 1) == 0)	// 14 15 16
					intFilteredSample += (int)dblZout_0[j];
				else
					intFilteredSample -= (int)dblZout_0[j];
			}
        
			break;
		
		case 1000:

			// scale each by transition coeff and + (Even) or - (Odd) 
			// Resonators 6 and 9 scaled by .15 to get best shape and side lobe supression to - 45 dB while keeping BW at 500 Hz @ -26 dB
			// practical range of scaling .05 to .25
			// Scaling also accomodates for the filter "gain" of approx 60. 
         

			if (SampleNo >= intFilLen)
			{
				if (j == 10 || j == 20)
					intFilteredSample +=  0.389f * dblZout_0[j];
				else if ((j & 1) == 0)	// Even
					intFilteredSample += (int)dblZout_0[j];
				else
					intFilteredSample -= (int)dblZout_0[j];
			}
        
			break;

		case 2000:

			// scale each by transition coeff and + (Even) or - (Odd) 
			// Resonators 6 and 9 scaled by .15 to get best shape and side lobe supression to - 45 dB while keeping BW at 500 Hz @ -26 dB
			// practical range of scaling .05 to .25
			// Scaling also accomodates for the filter "gain" of approx 60. 
          
			if (SampleNo >= intFilLen)
			{
				if (j == 5 || j == 25)
					intFilteredSample +=  0.389f * dblZout_0[j];
				else if ((j & 1) == 0)	// Even
					intFilteredSample += (int)dblZout_0[j];
				else
					intFilteredSample -= (int)dblZout_0[j];
			}
		}
	}

	if (SampleNo >= intFilLen)
	{
		intFilteredSample = intFilteredSample * 0.00833333333f; //  rescales for gain of filter
		largest = max(largest, intFilteredSample);	
		smallest = min(smallest, intFilteredSample);
		
		if (intFilteredSample > 32700)  // Hard clip above 32700
			intFilteredSample = 32700;
		else if (intFilteredSample < -32700)
			intFilteredSample = -32700;

#ifdef WIN32
		buffer[Index][Number++] = (short)intFilteredSample;
#else
		work = (short)(intFilteredSample);
		loopbuff[Number] = work;
		buffer[Index][Number++] = (work + 32768) >> 4; // 12 bit left justify
#endif
		if (Number == SendSize)
		{
			// send this buffer to sound interface
			// Loop back   to decode for testing

			if (Loopback)
			{
#ifdef WIN32
				ProcessNewSamples(buffer[Index], 1200);		// signed
#else
				ProcessNewSamples(loopbuff, 1200);
#endif
			}
			SendtoCard(SendSize);
			Number = 0;
		}
	}
		
	Last120[Last120Put++] = Sample;

	if (Last120Put == 121)
		Last120Put = 0;

	SampleNo++;
}

	
//	Called at end of transmission

void SoundFlush()
{
	// Append Trailer then send remaining samples

	AddTrailer();			// add the trailer.

#ifdef WIN32
	ProcessNewSamples(buffer[Index], Number);
#else
	ProcessNewSamples(loopbuff, Number);
#endif

	SendtoCard(Number * 2);

#ifndef WIN32

	// Wait for other DMA buffer to empty beofre shutting down DAC

	while (1)
	{
		int chan = DMA_GetCurrentMemoryTarget(DMA1_Stream5);

		if (chan == Index) 	// we've started sending current buffer
		{
			break;
		}
	}

	stopDAC();
	DMARunning = FALSE;

#endif

	StartCapture();
	SoundIsPlaying = FALSE;
	PlayComplete = TRUE;
	return;
}

//		// This generates a nice musical pattern for sound interface testing
//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);



void InitSound()
{
#ifdef WIN32

	int ret;

	header[0].dwFlags = WHDR_DONE;
	header[1].dwFlags = WHDR_DONE;

    waveOutOpen(&hWaveOut, 0, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
	waveOutGetDevCaps((UINT_PTR)hWaveOut, &pwoc, sizeof(WAVEOUTCAPS));

    waveInOpen(&hWaveIn, 0, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
	waveInGetDevCaps((UINT_PTR)hWaveIn, &pwic, sizeof(WAVEINCAPS));

//	wavfp1 = fopen("s:\\textxxx.wav", "wb");

	inheader[0].dwBufferLength = 2400;
	inheader[1].dwBufferLength = 2400;

	ret = waveInPrepareHeader(hWaveIn, &inheader[0], sizeof(WAVEHDR));
	ret = waveInAddBuffer(hWaveIn, &inheader[0], sizeof(WAVEHDR));

	ret = waveInPrepareHeader(hWaveIn, &inheader[1], sizeof(WAVEHDR));
	ret = waveInAddBuffer(hWaveIn, &inheader[1], sizeof(WAVEHDR));

	ret = waveInStart(hWaveIn);

#endif
}

PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data

#ifdef WIN32

	if (inheader[0].dwFlags & WHDR_DONE)
	{
		waveInUnprepareHeader(hWaveIn, &inheader[0], sizeof(WAVEHDR));
//		printf("Process 1 %d\n", inheader[0].dwBytesRecorded/2);
		if (Capturing && Loopback == FALSE)
			ProcessNewSamples(&inbuffer[0][0], inheader[0].dwBytesRecorded/2);
		inheader[0].dwFlags = 0;
		waveInPrepareHeader(hWaveIn, &inheader[0], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &inheader[0], sizeof(WAVEHDR));
	}

	if (inheader[1].dwFlags & WHDR_DONE)
	{
		waveInUnprepareHeader(hWaveIn, &inheader[1], sizeof(WAVEHDR));
//		printf("Process 2 %d\n", inheader[1].dwBytesRecorded/2);
		if (Capturing && Loopback == FALSE)
			ProcessNewSamples(&inbuffer[1][0], inheader[1].dwBytesRecorded/2);
		inheader[1].dwFlags = 0;

		waveInPrepareHeader(hWaveIn, &inheader[1], sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &inheader[1], sizeof(WAVEHDR));
	}
#endif
}


void StopCapture()
{
	Capturing = FALSE;
#ifdef WIND32
	waveInStop(hWaveIn);
#endif
//	printf("Stop Capture\n");
}
void StartCapture()
{
	Capturing = TRUE;
	DiscardOldSamples();
	ClearAllMixedSamples();
	State = SearchingForLeader;

#ifdef WIN32
//	waveInStart(hWaveIn);
#endif
//	printf("Start Capture\n");
}
void CloseSound()
{ 
#ifdef WIN32
//	waveOutClose(hWaveOut);
	fclose(wavfp1);
#endif
}

#ifdef WIN32
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
#endif




VOID WriteSamples(short * buffer, int len)
{

#ifdef WIN32
	fwrite(buffer, 1, len * 2, wavfp1);
#endif
}


