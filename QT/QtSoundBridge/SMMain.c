/*
Copyright (C) 2019-2020 Andrei Kopanchuk UZ7HO

This file is part of QtSoundBridge

QtSoundBridge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

QtSoundBridge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with QtSoundBridge.  If not, see http://www.gnu.org/licenses

*/

// UZ7HO Soundmodem Port by John Wiseman G8BPQ

#include "UZ7HOStuff.h"
#include <time.h>
#include "ecc.h"				// RS Constants
#include <fcntl.h>
#include <errno.h>

BOOL KISSServ;
int KISSPort;

BOOL AGWServ;
int AGWPort;

int Number = 0;				// Number waiting to be sent

int SoundIsPlaying = 0;
int Capturing = 0;

extern unsigned short buffer[2][1200];
extern int SoundMode;

short * DMABuffer;

unsigned short * SendtoCard(unsigned short * buf, int n);
short * SoundInit();
void DoTX(int Chan);


int SampleNo;

extern int pnt_change[5];				// Freq Changed Flag

// fftw library interface



int nonGUIMode = 0;

void soundMain()
{
	// non platform specific initialisation

	platformInit();
}


void Flush()
{
	SoundFlush(Number);
}

int ipow(int base, int exp)
{
	int result = 1;
	while (exp)
	{
		if (exp & 1)
			result *= base;
		exp >>= 1;
		base *= base;
	}

	return result;
}

int NumberOfBitsNeeded(int PowerOfTwo)
{
	int i;

	for (i = 0; i <= 16; i++)
	{
		if ((PowerOfTwo & ipow(2, i)) != 0)
			return i;

	}
	return 0;
}


int ReverseBits(int Index, int NumBits)
{
	int i, Rev = 0;

	for (i = 0; i < NumBits; i++)
	{
		Rev = (Rev * 2) | (Index & 1);
		Index = Index / 2;
	}

	return Rev;
}


void FourierTransform(int NumSamples, short * RealIn, float * RealOut, float * ImagOut, int InverseTransform)
{
	float AngleNumerator;
	unsigned char NumBits;

	int i, j, K, n, BlockSize, BlockEnd;
	float DeltaAngle, DeltaAr;
	float Alpha, Beta;
	float TR, TI, AR, AI;

	if (InverseTransform)
		AngleNumerator = -2.0f * M_PI;
	else
		AngleNumerator = 2.0f * M_PI;

	NumBits = NumberOfBitsNeeded(NumSamples);

	for (i = 0; i < NumSamples; i++)
	{
		j = ReverseBits(i, NumBits);
		RealOut[j] = RealIn[i];
		ImagOut[j] = 0.0f; // Not using i in ImageIn[i];
	}

	BlockEnd = 1;
	BlockSize = 2;

	while (BlockSize <= NumSamples)
	{
		DeltaAngle = AngleNumerator / BlockSize;
		Alpha = sinf(0.5f * DeltaAngle);
		Alpha = 2.0f * Alpha * Alpha;
		Beta = sinf(DeltaAngle);

		i = 0;

		while (i < NumSamples)
		{
			AR = 1.0f;
			AI = 0.0f;

			j = i;

			for (n = 0; n < BlockEnd; n++)
			{
				K = j + BlockEnd;
				TR = AR * RealOut[K] - AI * ImagOut[K];
				TI = AI * RealOut[K] + AR * ImagOut[K];
				RealOut[K] = RealOut[j] - TR;
				ImagOut[K] = ImagOut[j] - TI;
				RealOut[j] = RealOut[j] + TR;
				ImagOut[j] = ImagOut[j] + TI;
				DeltaAr = Alpha * AR + Beta * AI;
				AI = AI - (Alpha * AI - Beta * AR);
				AR = AR - DeltaAr;
				j = j + 1;
			}
			i = i + BlockSize;
		}
		BlockEnd = BlockSize;
		BlockSize = BlockSize * 2;
	}

	if (InverseTransform)
	{
		//	Normalize the resulting time samples...

		for (i = 0; i < NumSamples; i++)
		{
			RealOut[i] = RealOut[i] / NumSamples;
			ImagOut[i] = ImagOut[i] / NumSamples;
		}
	}
}



int LastBusyCheck = 0;

extern UCHAR CurrentLevel;

#ifdef PLOTSPECTRUM		
float dblMagSpectrum[206];
float dblMaxScale = 0.0f;
extern UCHAR Pixels[4096];
extern UCHAR * pixelPointer;
#endif

int blnBusyStatus = 0;
BusyDet = 0;

#define PLOTWATERFALL

int WaterfallActive = 1;
int SpectrumActive;

/*

void UpdateBusyDetector(short * bytNewSamples)
{
	float dblReF[1024];
	float dblImF[1024];
	float dblMag[206];
#ifdef PLOTSPECTRUM
	float dblMagMax = 0.0000000001f;
	float dblMagMin = 10000000000.0f;
#endif
	UCHAR Waterfall[256];			// Colour index values to send to GUI
	int clrTLC = Lime;				// Default Bandwidth lines on waterfall

	static BOOL blnLastBusyStatus;

	float dblMagAvg = 0;
	int intTuneLineLow, intTuneLineHi, intDelta;
	int i;

	//	if (State != SearchingForLeader)
	//		return;						// only when looking for leader

	if (Now - LastBusyCheck < 100)
		return;

	LastBusyCheck = Now;

	FourierTransform(1024, bytNewSamples, &dblReF[0], &dblImF[0], FALSE);

	for (i = 0; i < 206; i++)
	{
		//	starting at ~300 Hz to ~2700 Hz Which puts the center of the signal in the center of the window (~1500Hz)

		dblMag[i] = powf(dblReF[i + 25], 2) + powf(dblImF[i + 25], 2);	 // first pass 
		dblMagAvg += dblMag[i];
#ifdef PLOTSPECTRUM		
		dblMagSpectrum[i] = 0.2f * dblMag[i] + 0.8f * dblMagSpectrum[i];
		dblMagMax = max(dblMagMax, dblMagSpectrum[i]);
		dblMagMin = min(dblMagMin, dblMagSpectrum[i]);
#endif
	}

	//	LookforPacket(dblMag, dblMagAvg, 206, &dblReF[25], &dblImF[25]);
	//	packet_process_samples(bytNewSamples, 1200);

	intDelta = roundf(500 / 2) + 50 / 11.719f;

	intTuneLineLow = max((103 - intDelta), 3);
	intTuneLineHi = min((103 + intDelta), 203);

//	if (ProtocolState == DISC)		// ' Only process busy when in DISC state
	{
	//	blnBusyStatus = BusyDetect3(dblMag, intTuneLineLow, intTuneLineHi);

		if (blnBusyStatus && !blnLastBusyStatus)
		{
//			QueueCommandToHost("BUSY TRUE");
//			newStatus = TRUE;				// report to PTC

			if (!WaterfallActive && !SpectrumActive)
			{
				UCHAR Msg[2];

//				Msg[0] = blnBusyStatus;
//				SendtoGUI('B', Msg, 1);
			}
		}
		//    stcStatus.Text = "TRUE"
			//    queTNCStatus.Enqueue(stcStatus)
			//    'Debug.WriteLine("BUSY TRUE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))

		else if (blnLastBusyStatus && !blnBusyStatus)
		{
//			QueueCommandToHost("BUSY FALSE");
//			newStatus = TRUE;				// report to PTC

			if (!WaterfallActive && !SpectrumActive)
			{
				UCHAR Msg[2];

				Msg[0] = blnBusyStatus;
//				SendtoGUI('B', Msg, 1);
			}
		}
		//    stcStatus.Text = "FALSE"
		//    queTNCStatus.Enqueue(stcStatus)
		//    'Debug.WriteLine("BUSY FALSE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))

		blnLastBusyStatus = blnBusyStatus;
	}

	if (BusyDet == 0)
		clrTLC = Goldenrod;
	else if (blnBusyStatus)
		clrTLC = Fuchsia;

	// At the moment we only get here what seaching for leader,
	// but if we want to plot spectrum we should call
	// it always



	if (WaterfallActive)
	{
#ifdef PLOTWATERFALL
		dblMagAvg = log10f(dblMagAvg / 5000.0f);

		for (i = 0; i < 206; i++)
		{
			// The following provides some AGC over the waterfall to compensate for avg input level.

			float y1 = (0.25f + 2.5f / dblMagAvg) * log10f(0.01 + dblMag[i]);
			int objColor;

			// Set the pixel color based on the intensity (log) of the spectral line
			if (y1 > 6.5)
				objColor = Orange; // Strongest spectral line 
			else if (y1 > 6)
				objColor = Khaki;
			else if (y1 > 5.5)
				objColor = Cyan;
			else if (y1 > 5)
				objColor = DeepSkyBlue;
			else if (y1 > 4.5)
				objColor = RoyalBlue;
			else if (y1 > 4)
				objColor = Navy;
			else
				objColor = Black;

			if (i == 102)
				Waterfall[i] = Tomato;  // 1500 Hz line (center)
			else if (i == intTuneLineLow || i == intTuneLineLow - 1 || i == intTuneLineHi || i == intTuneLineHi + 1)
				Waterfall[i] = clrTLC;
			else
				Waterfall[i] = objColor; // ' Else plot the pixel as received
		}

		// Send Signal level and Busy indicator to save extra packets

		Waterfall[206] = CurrentLevel;
		Waterfall[207] = blnBusyStatus;

		doWaterfall(Waterfall);
#endif
	}
	else if (SpectrumActive)
	{
#ifdef PLOTSPECTRUM
		// This performs an auto scaling mechansim with fast attack and slow release
		if (dblMagMin / dblMagMax < 0.0001) // more than 10000:1 difference Max:Min
			dblMaxScale = max(dblMagMax, dblMaxScale * 0.9f);
		else
			dblMaxScale = max(10000 * dblMagMin, dblMagMax);

//		clearDisplay();

		for (i = 0; i < 206; i++)
		{
			// The following provides some AGC over the spectrum to compensate for avg input level.

			float y1 = -0.25f * (SpectrumHeight - 1) *  log10f((max(dblMagSpectrum[i], dblMaxScale / 10000)) / dblMaxScale); // ' range should be 0 to bmpSpectrumHeight -1
			int objColor = Yellow;

			Waterfall[i] = round(y1);
		}

		// Send Signal level and Busy indicator to save extra packets

		Waterfall[206] = CurrentLevel;
		Waterfall[207] = blnBusyStatus;
		Waterfall[208] = intTuneLineLow;
		Waterfall[209] = intTuneLineHi;

//		SendtoGUI('X', Waterfall, 210);
#endif
	}
}

*/

short rawSamples[2400];	// Get Frame Type need 2400 and we may add 1200
int rawSamplesLength = 0;
int maxrawSamplesLength;

extern void SendReceivedSamples();

void MainLoop()
{
	// Called by background thread every 10 ms (maybe)

	// Actually we may have two cards

	// Original only allowed one channel per card.
	// I think we should be able to run more, ie two or more
	// modems on same soundcard channel

	// So All the soundcard stuff will need to be generalised

	PollReceivedSamples();
	SendReceivedSamples();
}




char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;
	return ptr;
}

char ShortDT[] = "HH:MM:SS";

char * ShortDateTime()
{
	struct tm * tm;
	time_t NOW = time(NULL);

	tm = gmtime(&NOW);

	sprintf(ShortDT, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
	return ShortDT;
}


extern TStringList detect_list[5];
extern TStringList detect_list_c[5];

