//
//	Passes audio samples to the sound interface

//	Windows uses WaveOut

//	Nucleo uses DMA

//	Linux will probably use ALSA

//	This is the Nucleo Version

#include <math.h>
#include "ARDOPC.h"

// Windows works with signed samples +- 32767
// STM32 DAC uses unsigned 0 - 4095

unsigned short buffer[2][1200];	// Two Transfer/DMA buffers of 0.1 Sec
unsigned short work;

BOOL Loopback = FALSE;
//BOOL Loopback = TRUE;

char CaptureDevice[] = "DMA";
char PlaybackDevice[] = "DMA";

char * CaptureDevices = CaptureDevice;
char * PlaybackDevices = PlaybackDevice;

void InitSound();

int Ticks;
int LastNow;

void main()
{
	ardopmain();
}

void printtick(char * msg)
{
	printf("%s %i\r\n", msg, Now - LastNow);
	LastNow = Now;
}

#include <stm32f4xx_dma.h>

int PriorSize = 0;

int Index = 0;				// DMA Buffer being used 0 or 1
int inIndex = 0;				// DMA Buffer being used 0 or 1

BOOL DMARunning = FALSE;		// Used to start DMA on first write

unsigned short * SendtoCard(unsigned short buf, int n)
{
	if (Loopback)
	{
		// Loop back   to decode for testing

		ProcessNewSamples(buf, 1200);		// signed
	}

	// Start DMA if first call

	if (DMARunning == FALSE)
	{
		StartDAC();
		DMARunning = TRUE;
	}

	// wait for other DMA buffer to finish

	printtick("Start Wait");		// FOr timing tests

	while (1)
	{
		int chan = DMA_GetCurrentMemoryTarget(DMA1_Stream5);

		if (chan == Index) 	// we've started sending current buffer
		{
			Index = !Index;
			printtick("Stop Wait");
		}
		txSleep(10);				// Run buckground while waiting 
	}
	return &buffer[Index][0];
}

//		// This generates a nice musical pattern for sound interface testing
//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);


void InitSound()
{
}

PollReceivedSamples()
{
	// Process any captured samples
	// Ideally call at least every 100 mS, more than 200 will loose data
}


void StopCapture()
{
	Capturing = FALSE;
//	printf("Stop Capture\n");
}

void StartCodec(char * strFault)
{
	strFault[0] = 0;
}

void StopCodec(char * strFault)
{
	strFault[0] = 0;
}

void StartCapture()
{
	Capturing = TRUE;
	DiscardOldSamples();
	ClearAllMixedSamples();
	State = SearchingForLeader;

//	printf("Start Capture\n");
}
void CloseSound()
{ 
}

unsigned short * SoundInit()
{
	Index = 0;
	return &buffer[0][0];
}
	
//	Called at end of transmission

void SoundFlush(Number)
{
	// Append Trailer then send remaining samples

	AddTrailer();			// add the trailer.

	if (Loopback)
	{
		short loopbuff[1200];		// Temp for testing - loop sent samples to decoder
		ProcessNewSamples(loopbuff, Number);
	}

	SendtoCard(buffer[Index], Number * 2);

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

	SoundIsPlaying = FALSE;
	PlayComplete = TRUE;
	return;
}

	
