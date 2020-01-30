
#include "UZ7HOStuff.h"
#include "fftw3.h"
#include <time.h>
#include "ecc.h"				// RS Constants


BOOL KISSServ;
int KISSPort;

BOOL AGWServ;
int AGWPort;

int Number = 0;				// Number waiting to be sent

int SoundIsPlaying = 0;
int Capturing = 0;

extern unsigned short buffer[2][1200];

short * DMABuffer;

unsigned short * SendtoCard(unsigned short * buf, int n);
short * SoundInit();
void DoTX(int Chan);


int SampleNo;

extern int pnt_change[5];				// Freq Changed Flag

// fftw library interface


fftwf_complex *in, *out;
fftwf_plan p;

#define N 2048

void initfft()
{
	in = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
	out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * N);
	p = fftwf_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_MEASURE);
}

void dofft(short * inp, float * outr, float * outi)
{
	int i;
	
	fftwf_complex * fft = in;

	for (i = 0; i < N; i++)
	{
		fft[0][0] = inp[0] * 1.0f;
		fft[0][1] = 0;
		fft++;
		inp++;
	}

	fftwf_execute(p); 

	fft = out;

	for (i = 0; i < N; i++)
	{
		outr[0] = fft[0][0];
		outi[0] = fft[0][1];
		fft++;
		outi++;
		outr++;
	}
}

void freefft()
{
	fftwf_destroy_plan(p);
	fftwf_free(in);
	fftwf_free(out);
}

int nonGUIMode = 0;

void soundMain()
{
	// non platform specific initialisation

	platformInit();

	// initialise fft library

	initfft();

	// Initialise KISS port

	detector_init();
	KISS_init();
	agw_init();
	ax25_init();
	init_raduga();			// Set up waterfall colour table

	//	waterfall_init;
	//	ReadIni;

	if (nonGUIMode)
	{
		Firstwaterfall = 0;
		Secondwaterfall = 0;
	}

	OpenPTTPort();
}


void SampleSink(int LR, short Sample)
{
	// This version is passed samples one at a time, as we don't have
	//	enough RAM in embedded systems to hold a full audio frame

	// LR - 1 == Right Chan

#ifdef TEENSY	
		int work = Sample;
		DMABuffer[Number++] = (work + 32768) >> 4; // 12 bit left justify
#else
	if (SCO)			// Single Channel Output - same to both L and R
	{
		DMABuffer[2 * Number] = Sample;
		DMABuffer[1 + 2 * Number] = Sample;

	}
	else
	{
		if (LR)				// Right
		{
			DMABuffer[1 + 2 * Number] = Sample;
			DMABuffer[2 * Number] = 0;
		}
		else
		{
			DMABuffer[2 * Number] = Sample;
			DMABuffer[1 + 2 * Number] = 0;
		}
	}
	Number++;
#endif
		if (Number == SendSize)
		{
			// send this buffer to sound interface

			DMABuffer = SendtoCard(DMABuffer, SendSize);
			Number = 0;
		}
	

//	Last120[Last120Put++] = Sample;

//	if (Last120Put == (intN + 1))
//		Last120Put = 0;

	SampleNo++;
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

void ProcessNewSamples(short * Samples, int nSamples)
{
	if (SoundIsPlaying == FALSE)
		BufferFull(Samples, nSamples);

	/*
	
	// Append new data to anything in rawSamples

	memcpy(&rawSamples[rawSamplesLength], Samples, nSamples * 2);
	rawSamplesLength += nSamples;

	if (rawSamplesLength > maxrawSamplesLength)
		maxrawSamplesLength = rawSamplesLength;

	if (rawSamplesLength >= 2400)
		WriteDebugLog(LOGCRIT, "Corrupt rawSamplesLength %d", rawSamplesLength);

	nSamples = rawSamplesLength;
	Samples = rawSamples;

	rawSamplesLength = 0;

	//	printtick("Start Busy");
	UpdateBusyDetector(Samples);
	//	printtick("Done Busy");

	*/
};

void doCalib(int Chan, int Act)
{
	calib_mode[Chan] = Act;

	if (Act == 0)
	{
		tx_status[Chan] = TX_SILENCE;		// Stop TX
		Flush();
		RadioPTT(Chan, 0);
		Debugprintf("Stop Calib");
	}
}

int Freq_Change(int Chan, int Freq)
{
	int low, high;

	low = round(rx_shift[1] / 2 + RCVR[Chan] * rcvr_offset[Chan] + 1);
	high = round(RX_Samplerate / 2 - (rx_shift[Chan] / 2 + RCVR[Chan] * rcvr_offset[Chan]));

	if (Freq < low)
		return rx_freq[Chan];				// Dont allow change

	if (Freq > high)
		return rx_freq[Chan];				// Dont allow change

	rx_freq[Chan] = Freq;
	tx_freq[Chan] = Freq;

	pnt_change[Chan] = TRUE;
	wf_pointer(soundChannel[Chan]);

	return Freq;
}

void MainLoop()
{
	// Called by background thread every 5 ms (maybe)

	// Actually we may have two cards
	
	// Original only allowed one channel per card.
	// I think we should be able to run more, ie two or more
	// modems on same soundcard channel
	
	// So All the soundcard stuff will need to be generalised

	PollReceivedSamples();

	DoTX(0);
	DoTX(1);


}

void DoTX(int Chan)
{
	// This kicks off a send sequence or calibrate

	if (calib_mode[Chan])
	{
		// Maybe new calib or continuation

		if (pnt_change[Chan])
		{
			make_core_BPF(Chan, rx_freq[Chan], bpf[Chan]);
			make_core_TXBPF(Chan, tx_freq[Chan], txbpf[Chan]);
			pnt_change[Chan] = FALSE;
		}
		
		// Note this may block in SendtoCard
		
		modulator(Chan, tx_bufsize);
		return;
	}

	// I think we have to detect NO_DATA here and drop PTT and return to SILENCE


	if (tx_status[Chan] == TX_NO_DATA)
	{
		Flush();
		WriteDebugLog(LOGALERT, "TX Complete");
		RadioPTT(0, 0);
		tx_status[Chan] = TX_SILENCE;

		// We should now send any ackmode acks as the channel is now free for dest to reply

		sendAckModeAcks(Chan);
	}
	
	if (tx_status[Chan] != TX_SILENCE)
	{
		// Continue the send

		modulator(Chan, tx_bufsize);
		return;
	}

	if (SoundIsPlaying)
		return;

	// Not doing anything so see if we have anything new to send

	// See if frequency has changed

	if (pnt_change[Chan])
	{
		make_core_BPF(Chan, rx_freq[Chan], bpf[Chan]);
		make_core_TXBPF(Chan, tx_freq[Chan], txbpf[Chan]);
		pnt_change[Chan] = FALSE;
	}


	if (all_frame_buf[Chan].Count == 0)
		return;

	// Start a new send. modulator should handle TXD etc

	WriteDebugLog(LOGALERT, "TX Start");
	SampleNo = 0;

	SoundIsPlaying = TRUE;
	RadioPTT(Chan, 1);

	modulator(Chan, tx_bufsize);

	return;
}

void pttoff(int snd_ch)
{
}

void ptton(int snd_ch)
{
}

void stoptx(int snd_ch)
{
	Flush();
	WriteDebugLog(LOGALERT, "TX Complete");
	RadioPTT(snd_ch, 0);
	tx_status[snd_ch] = TX_SILENCE;

	snd_status[snd_ch] = SND_IDLE;
}

void TX2RX(int snd_ch)
{
	if (snd_status[snd_ch] == SND_TX)
		stoptx(snd_ch);

	if (snd_status[snd_ch] == SND_IDLE)
		RadioPTT(snd_ch, 0);
}

void RX2TX(int snd_ch)
{
	if (snd_status[snd_ch] == SND_IDLE)
	{
		ptton(snd_ch);
		DoTX(snd_ch);
	}
}

// PTT Stuff

int hPTTDevice = 0;
char PTTPort[80] = "";			// Port for Hardware PTT - may be same as control port.
int PTTBAUD = 19200;
int PTTMode = PTTRTS;			// PTT Control Flags.


char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;
	return ptr;
}

void OpenPTTPort()
{
	if (PTTPort[0] && strcmp(PTTPort, "None") != 0)
	{
		char * Baud = strlop(PTTPort, ':');
		if (Baud)
			PTTBAUD = atoi(Baud);

		hPTTDevice = OpenCOMPort(PTTPort, PTTBAUD, FALSE, FALSE, FALSE, 0);
	}
}

void ClosePTTPort()
{
	CloseCOMPort(hPTTDevice);
	hPTTDevice = 0;
}

void RadioPTT(int snd_ch, BOOL PTTState)
{
	if (hPTTDevice == 0)
		return;

	if (DualPTT && modemtoSoundLR[snd_ch] == 1)		// use DTR
	{
		if (PTTState)
			COMSetDTR(hPTTDevice);
		else
			COMClearDTR(hPTTDevice);
	}
	else
	{
		if (PTTMode & PTTRTS)
			if (PTTState)
				COMSetRTS(hPTTDevice);
			else
				COMClearRTS(hPTTDevice);
	}

/*
	if (PTTMode & PTTCI_V)
		if (PTTState)
			WriteCOMBlock(hCATDevice, PTTOnCmd, PTTOnCmdLen);
		else
			WriteCOMBlock(hCATDevice, PTTOffCmd, PTTOffCmdLen);
*/
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


// Reed Solomon Stuff


int NPAR = -1;	// Number of Parity Bytes - used in RS Code

int xMaxErrors = 0;

int RSEncode(UCHAR * bytToRS, UCHAR * RSBytes, int DataLen, int RSLen)
{
	// This just returns the Parity Bytes. I don't see the point
	// in copying the message about

	unsigned char Padded[256];		// The padded Data

	int Length = DataLen + RSLen;	// Final Length of packet
	int PadLength = 255 - Length;	// Padding bytes needed for shortened RS codes

	//	subroutine to do the RS encode. For full length and shortend RS codes up to 8 bit symbols (mm = 8)

	if (NPAR != RSLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = RSLen;
		xMaxErrors = NPAR / 2;
		initialize_ecc();
	}

	// Copy the supplied data to end of data array.

	memset(Padded, 0, PadLength);
	memcpy(&Padded[PadLength], bytToRS, DataLen);

	encode_data(Padded, 255 - RSLen, RSBytes);

	return RSLen;
}

//	Main RS decode function

extern int index_of[];
extern int recd[];
int Corrected[256];
extern int tt;		//  number of errors that can be corrected 
extern int kk;		// Info Symbols

BOOL blnErrorsCorrected;


BOOL RSDecode(UCHAR * bytRcv, int Length, int CheckLen, BOOL * blnRSOK)
{


	// Using a modified version of Henry Minsky's code

	//Copyright Henry Minsky (hqm@alum.mit.edu) 1991-2009

	// Rick's Implementation processes the byte array in reverse. and also 
	//	has the check bytes in the opposite order. I've modified the encoder
	//	to allow for this, but so far haven't found a way to mske the decoder
	//	work, so I have to reverse the data and checksum to decode G8BPQ Nov 2015

	//	returns TRUE if was ok or correction succeeded, FALSE if correction impossible

	UCHAR intTemp[256];				// WOrk Area to pass to Decoder		
	int i;
	UCHAR * ptr2 = intTemp;
	UCHAR * ptr1 = &bytRcv[Length - CheckLen - 1]; // Last Byte of Data

	int DataLen = Length - CheckLen;
	int PadLength = 255 - Length;		// Padding bytes needed for shortened RS codes

	*blnRSOK = FALSE;

	if (Length > 255 || Length < (1 + CheckLen))		//Too long or too short 
		return FALSE;

	if (NPAR != CheckLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = CheckLen;
		xMaxErrors = NPAR / 2;

		initialize_ecc();
	}


	//	We reverse the data while zero padding it to speed things up

	//	We Need (Data Reversed) (Zero Padding) (Checkbytes Reversed)

	// Reverse Data

	for (i = 0; i < DataLen; i++)
	{
		*(ptr2++) = *(ptr1--);
	}

	//	Clear padding

	memset(ptr2, 0, PadLength);

	ptr2 += PadLength;

	// Error Bits

	ptr1 = &bytRcv[Length - 1];			// End of check bytes

	for (i = 0; i < CheckLen; i++)
	{
		*(ptr2++) = *(ptr1--);
	}

	decode_data(intTemp, 255);

	// check if syndrome is all zeros 

	if (check_syndrome() == 0)
	{
		// RS ok, so no need to correct

		*blnRSOK = TRUE;
		return TRUE;		// No Need to Correct
	}

	if (correct_errors_erasures(intTemp, 255, 0, 0) == 0) // Dont support erasures at the momnet

		// Uncorrectable

		return FALSE;

	// Data has been corrected, so need to reverse again

	ptr1 = &intTemp[DataLen - 1];
	ptr2 = bytRcv; // Last Byte of Data

	for (i = 0; i < DataLen; i++)
	{
		*(ptr2++) = *(ptr1--);
	}

	// ?? Do we need to return the check bytes ??

	// Yes, so we can redo RS Check on supposedly connected frame

	ptr1 = &intTemp[254];	// End of Check Bytes

	for (i = 0; i < CheckLen; i++)
	{
		*(ptr2++) = *(ptr1--);
	}

	return TRUE;
}






