#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T


#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 12000, 12000, 2, 16, 0 };

short buffer[24000 * 10];			// One Sec

WAVEHDR header = {(char *)buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };

int Number = 0;				// Number waiting to be sent

void main()
{
	ardopmain();
}

void SampleSink(short Sample, void * Filter())
{
	// We probably need to filter samples. 
	// Filters run 120 samples behind, so we need to 
	// save state. Don't want to call too often cos of
	// the overhead of saving and restoring context, but
	// also don't want to use too much RAM


	buffer[Number++]= Sample;
}


void SoundFlush()
{
	// loop back to decoder for testing

	header.dwBufferLength = Number * 2;

	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

	while (!(header.dwFlags & WHDR_DONE));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));

	ProcessNewSamples(buffer, Number + 1200);
	Number = 0;

	return;
}


//    for (t = 0; t < sizeof(buffer); ++t)
//        buffer[t] =((((t * (t >> 8 | t >> 9) & 46 & t >> 8)) ^ (t & t >> 13 | t >> 6)) & 0xFF);

WAVEOUTCAPS pwoc;
WAVEINCAPS pwic;


InitSound()
{
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



