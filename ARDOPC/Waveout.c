#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T


#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 12000, 12000, 2, 16, 0 };

short buffer[2][1200];			// Two Transfer/DMA buffers of 0.1 Sec

short SavedFiltered[120];		// Samples we have partly filtered
short toFilter[1200];			// Samples waiting to be filtered

WAVEHDR header[2] =
{
	{(char *)buffer[0], 0, 0, 0, 0, 0, 0, 0},
	{(char *)buffer[1], 0, 0, 0, 0, 0, 0, 0}
};

int Number = 0;				// Number waiting to be sent

int Index = 0;

void FSXmtFilter200_1500Hz(short * intNewSamples, int Length);

void main()
{
	ardopmain();
}

typedef void (*PROCX)();

int (* Filter)() = FSXmtFilter200_1500Hz;

//PROCX Filter = NULL;

short Dummy[1200];

int SendSize = 1200;		// 100 mS for now

void SetFilter(void * NewFilter)
{
	Filter = NewFilter;
}

void FilterandSend()
{
	//	Filter Samples, then send to sound interface

	Filter(Dummy, Number);

	//	Save partly filtered samples (Last 120 samples)

	// Wait till current buffer is available


	memcpy(&buffer[Index][0], toFilter, Number * 2);
	header[Index].dwBufferLength = Number * 2;

	waveOutPrepareHeader(hWaveOut, &header[Index], sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header[Index], sizeof(WAVEHDR));

	while (!(header[!Index].dwFlags & WHDR_DONE));
	waveOutUnprepareHeader(hWaveOut, &header[!Index], sizeof(WAVEHDR));

	Index = !Index;		// Switch buffers

	Number = 0;
}



void SampleSink(short Sample)
{
	// We need to filter samples. 
	// Filters run 120 samples behind, so we need to 
	// save state. Don't want to call too often cos of
	// the overhead of saving and restoring context, but
	// also don't want to use too much RAM

	// We also buffer up samples before sending to the sound interface,
	// which could be a proper sound card, or an ADC

	// Buffer to max size 

	toFilter[Number++] = Sample;

	if (Number == SendSize)
	{
		FilterandSend();
	}
	return;
}





void SoundFlush()
{
	//Append Trailer then send remainig samples

	
//	Length = AddTrailer(intNewSamples, Length);  // add the trailer before filtering



	FilterandSend();

	// loop back to decoder for testing

	return;
}


//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);

WAVEOUTCAPS pwoc;
WAVEINCAPS pwic;


InitSound()
{
	header[0].dwFlags = WHDR_DONE;
	header[1].dwFlags = WHDR_DONE;

    waveOutOpen(&hWaveOut, 0, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
	waveOutGetDevCaps(hWaveOut, &pwoc, sizeof(WAVEOUTCAPS));

    waveInOpen(&hWaveIn, 0, &wfx, 0, 0, CALLBACK_NULL); //WAVE_MAPPER
	waveInGetDevCaps(hWaveIn, &pwic, sizeof(WAVEINCAPS));
}

CloseSound()
{ 
	waveOutClose(hWaveOut);
}


VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}



