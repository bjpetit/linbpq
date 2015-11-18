#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

HWAVEOUT hWaveOut = 0;
HWAVEIN hWaveIn = 0;

WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, 12000, 12000, 2, 16, 0 };

short buffer[24000 * 10];			// One Sec

WAVEHDR header = {(char *)buffer, sizeof(buffer), 0, 0, 0, 0, 0, 0 };

int Number = 0;				// Number waiting to be sent

SampleSink(UCHAR * Samples, int Count)
{
	if (Number + Count > 120000)			// Will overflow
	{
		memcpy(&buffer[Number], Samples, (120000 - Number) * 2);
		waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
		waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

		while (!(header.dwFlags & WHDR_DONE));
		waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));

		Count -= (12000 - Number);
		Samples = &Samples[12000 - Number];
		Number = 0;
	}

	memcpy(&buffer[Number], Samples, Count * 2);

	Number += Count;

	return 0;
}


SoundFlush()
{
	header.dwBufferLength = Number * 2;

	waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

	while (!(header.dwFlags & WHDR_DONE));
	waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
	Number = 0;

	return 0;
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