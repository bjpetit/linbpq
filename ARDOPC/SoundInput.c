//	ARDOP Modem Decode Sound Samples

#include "ARDOPC.h"

#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))

BOOL Capturing = TRUE;

char strCurrentFrame[16];
char strRcvFrameTag[16];

BOOL blnLeaderFound = FALSE;
BOOL blnLeaderDetected;
int intRcvdSamplesRPtr = 0;

int intLeaderRcvdMs = 1000;		// Leader length??
short intPSKRefPhase;			// PSK Reference symbol in milliradians) (+/- 3142)

extern int intLastRcvdFrameQuality;

short intPriorMixedSamples[120];  // a buffer of 120 samples to hold the prior samples used in the filter
int	intPriorMixedSamplesLength = 120;  // size of Prior sample buffer

short intFilteredMixedSamples[60000];
int intFilteredMixedSamplesLength = 0;

short intMixedSamples[60000];		// may need to be int
int	intMixedSamplesLength = 0;	//size of intMixedSamples

BOOL blnSymbolSyncFound, blnFrameSyncFound;

int intFrameType;

UCHAR bytLastARQSessionID;

extern int intSamplesToCompleteFrame;

// dont think I need it short intRcvdSamples[12000];		// 1 second. May need to optimise

float dblOffsetLastGoodDecode = 0;
time_t dttLastGoodFrameTypeDecode = 0;

float dblOffsetHz = 0;;
time_t dttLastLeaderDetect;

time_t intRmtLeaderMeasure = 0;

BOOL Connected = FALSE;
BOOL Pending = FALSE;
UCHAR PendingSessionID;
extern UCHAR bytSessionID;

time_t dttLastGoodFrameTypeDecod;
time_t dttStartRmtLeaderMeasure;
time_t dttStartRTMeasure;

int intARQRTmeasuredMs;
int TuningRange = 100;

float dbl2Pi = 2 * M_PI; 

float dblSNdBPwr, dblSNdBPwr_1, dblSNdBPwr_2;
float dblNCOFreq = 3000;	 // nominal NC) frequency
float dblNCOPhase = 0;
float dblNCOPhaseInc = 2 * M_PI * 3000 / 12000;  // was dblNCOFreq

int	intMFSReadPtr = 30;				// reset the MFSReadPtr offset 30 to accomodate the filter delay

int RcvdSamplesLen = 0;				// Samples in RX buffer

BOOL SearchFor2ToneLeader(short * intNewSamples, int Ptr, int Length, float * dblOffsetHz);

const int SamplesToComplete[256] = {
//  lookup the number of samples (@12 KHz) needed to complete the frame after the Frame ID is detected 
// Value is increased by factor of 1.005 (5000 ppm)  to accomodate sample rate offsets in Transmitter and Receiver 


// Also used to validate frame type (len != -1)

//	Note these samples DO NOT include the PSK reference symbols which are accomodated in DemodPSK
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 00 - 0F    ACK and NAK
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 10 - 1F
	-1,-1,-1,0,-1,-1,0,-1,-1,0,-1,-1,0,0,0,-1,	// BREAK=23, IDLE=26, DISC=29, END=2C, ConRejBusy=2D, ConRejBW=2E

	(int)(1.005 * (6 + 6 + 2) * 4 * 240),	// 30 - 38 ID Frame Call sign + Grid Square, Connect request frames
	(int)(1.005 * (6 + 6 + 2) * 4 * 240),	// 30 - 38 ID Frame Call sign + Grid Square, Connect request frames
	(int)(1.005 * (6 + 6 + 2) * 4 * 240),	// 30 - 38 ID Frame Call sign + Grid Square, Connect request frames
	(int)(1.005 * (6 + 6 + 2) * 4 * 240),	// 30 - 38 ID Frame Call sign + Grid Square, Connect request frames
	(int)(1.005 * (6 + 6 + 2) * 4 * 240),	// 30 - 38 ID Frame Call sign + Grid Square, Connect request frames
	(int)(1.005 * (6 + 6 + 2) * 4 * 240),	// 30 - 38 ID Frame Call sign + Grid Square, Connect request frames
	(int)(1.005 * (6 + 6 + 2) * 4 * 240),	// 30 - 38 ID Frame Call sign + Grid Square, Connect request frames
	(int)(1.005 * (6 + 6 + 2) * 4 * 240),	// 30 - 38 ID Frame Call sign + Grid Square, Connect request frames
	(int)(1.005 * (6 + 6 + 2) * 4 * 240),	// 30, 38 ID Frame Call sign + Grid Square, Connect request frames
  
	(int)(1.005 * 240 * 3 * 4),	// 39 - 3c	 Con ACK with timing data 
	(int)(1.005 * 240 * 3 * 4),	// 39 - 3c	 Con ACK with timing data 
	(int)(1.005 * 240 * 3 * 4),	// 39 - 3c	 Con ACK with timing data 
	(int)(1.005 * 240 * 3 * 4),	// 39 - 3c	 Con ACK with timing data 
	-1,-1,-1,						// 3d - 3f			

	(int)(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)), // 40, 41 1 carrier 100 baud 4PSK
	(int)(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)), // 40, 41 1 carrier 100 baud 4PSK
	(int)(1.005 * (120 + (1 + 16 + 2 + 8) * 4 * 120)),	// 42, 43 1 carrier 100 baud 4PSK Short
	(int)(1.005 * (120 + (1 + 16 + 2 + 8) * 4 * 120)),	// 42, 43 1 carrier 100 baud 4PSK Short			
	(int)(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) / 3))), // 44, 45 1 carrier 100 baud 8PSK
	(int)(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) / 3))), // 44, 45 1 carrier 100 baud 8PSK	
	(int)(1.005 * (240 * 4 * (1 + 32 + 2 + 8))),	// 46, 47 1 carrier 50 baud 4FSK
	(int)(1.005 * (240 * 4 * (1 + 32 + 2 + 8))),	// 46, 47 1 carrier 50 baud 4FSK
	(int)(1.005 * (240 * 4 * (1 + 16 + 2 + 4))),	// 48, 49 ' 1 carrier 50 baud 4FSK short 
	(int)(1.005 * (240 * 4 * (1 + 16 + 2 + 4))),	// 48, 49 ' 1 carrier 50 baud 4FSK short 
	(int)(1.005 * (120 * 4 * (1 + 64 + 2 + 16))),	// 4A, 4B ' 1 carrier 100 baud 4FSK 
	(int)(1.005 * (120 * 4 * (1 + 64 + 2 + 16))),	// 4A, 4B ' 1 carrier 100 baud 4FSK 			
	(int)(1.005 * (120 * 4 * (1 + 32 + 2 + 8))),	// 4C, 4d ' 1 carrier 100 baud 4FSK short 
	(int)(1.005 * (120 * 4 * (1 + 32 + 2 + 8))),	// 4C, 4d ' 1 carrier 100 baud 4FSK short 
	(int)(1.005 * (480 * (8 * (1 + 24 + 2 + 6)) / 3)),	// 4E, 4F ' 1 carrier 25 baud 8FSK 
	(int)(1.005 * (480 * (8 * (1 + 24 + 2 + 6)) / 3)),	// 4E, 4F ' 1 carrier 25 baud 8FSK 

	(int)(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)),	// 50, 51 ' 2 carrier 100 baud 4PSK
	(int)(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)),	// 50, 51 ' 2 carrier 100 baud 4PSK
	(int)(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) / 3))),	// 52, 53 ' 2 carrier 100 baud 8PSK
	(int)(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) / 3))),	// 52, 53 ' 2 carrier 100 baud 8PSK
	(int)(1.005 * (72 + (1 + 120 + 2 + 40) * 4 * 72)),	// 54, 55 ' 2 carrier 167 baud 4PSK
	(int)(1.005 * (72 + (1 + 120 + 2 + 40) * 4 * 72)),	// 54, 55 ' 2 carrier 167 baud 4PSK
	(int)(1.005 * (72 + (72 * (8 * (1 + 159 + 2 + 60)) / 3))),	// 56, 57 ' 2 carrier 167 baud 8PSK
	(int)(1.005 * (72 + (72 * (8 * (1 + 159 + 2 + 60)) / 3))),	// 56, 57 ' 2 carrier 167 baud 8PSK
	(int)(1.005 * (480 * 2 * (1 + 32 + 2 + 8))),	// 58, 59 ' 1 carrier 25 baud 16FSK (in testing) 
	(int)(1.005 * (480 * 2 * (1 + 32 + 2 + 8))),	// 58, 59 ' 1 carrier 25 baud 16FSK (in testing) 
	(int)(1.005 * (480 * 2 * (1 + 16 + 2 + 4))),	// 5A, 5B ' 1 carrier 25 baud 16FSK Short (in testing) 
	(int)(1.005 * (480 * 2 * (1 + 16 + 2 + 4))),	// 5A, 5B ' 1 carrier 25 baud 16FSK Short (in testing) 
	-1,-1,-1,-1,			// 5C -5F

	(int)(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)),	// 60, 61 ' 4 carrier 100 baud 4PSK
	(int)(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)),	// 60, 61 ' 4 carrier 100 baud 4PSK
	(int)(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) / 3))),	//62, 63 ' 4 carrier 100 baud 8PSK
	(int)(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) / 3))),	//62, 63 ' 4 carrier 100 baud 8PSK
	(int)(1.005 * (72 + (1 + 120 + 2 + 40) * 4 * 72)),	// 64, 65 ' 4 carrier 167 baud 4PSK
	(int)(1.005 * (72 + (1 + 120 + 2 + 40) * 4 * 72)),	// 64, 65 ' 4 carrier 167 baud 4PSK
	(int)(1.005 * (72 + (72 * (8 * (1 + 159 + 2 + 60)) / 3))),	// 66, 67 ' 4 carrier 167 baud 8PSK
	(int)(1.005 * (72 + (72 * (8 * (1 + 159 + 2 + 60)) / 3))),	// 66, 67 ' 4 carrier 167 baud 8PSK
	(int)(1.005 * (120 * 4 * (1 + 64 + 2 + 16))),	// 68, 69 ' 2 carrier 100 baud 4FSK 
	(int)(1.005 * (120 * 4 * (1 + 64 + 2 + 16))),	// 68, 69 ' 2 carrier 100 baud 4FSK 
	-1,-1,-1,-1,-1,-1,				// 6A - 6F

	(int)(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)),	// 70, 71 ' 8 carrier 100 baud 4PSK
	(int)(1.005 * (120 + (1 + 64 + 2 + 32) * 4 * 120)),	// 70, 71 ' 8 carrier 100 baud 4PSK
	(int)(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) / 3))),	// 72, 73 ' 8 carrier 100 baud 8PSK
	(int)(1.005 * (120 + (120 * (8 * (1 + 108 + 2 + 36)) / 3))),	// 72, 73 ' 8 carrier 100 baud 8PSK
   	(int)(1.005 * (72 + (1 + 120 + 2 + 40) * 4 * 72)),	//74, 75 ' 8 carrier 167 baud 4PSK
	(int)(1.005 * (72 + (1 + 120 + 2 + 40) * 4 * 72)),	//74, 75 ' 8 carrier 167 baud 4PSK            
	(int)(1.005 * (72 + (72 * (8 * (1 + 159 + 2 + 60)) / 3))),	// 76, 77 ' 2 carrier 167 baud 8PSK ' 8 carrier 167 baud 4PSK
	(int)(1.005 * (72 + (72 * (8 * (1 + 159 + 2 + 60)) / 3))),	// 76, 77 ' 2 carrier 167 baud 8PSK ' 8 carrier 167 baud 4PSK            
	(int)(1.005 * (120 * 4 * (1 + 64 + 2 + 16))),	// 78, 79 ' 4 carrier 100 baud 4FSK 
	(int)(1.005 * (120 * 4 * (1 + 64 + 2 + 16))),	// 78, 79 ' 4 carrier 100 baud 4FSK 
            
	// experimental 600 baud for VHF/UHF FM
            	
	(int)(1.005 * (20 * 4 * 3 * (1 + 200 + 2 + 50))),	// 7A, 7B ' 1 carrier 600 baud 4FSK (3 groups of 200 bytes each for RS compatibility) 
	(int)(1.005 * (20 * 4 * 3 * (1 + 200 + 2 + 50))),	// 7A, 7B ' 1 carrier 600 baud 4FSK (3 groups of 200 bytes each for RS compatibility) 
	(int)(1.005 * (20 * 4 * (1 + 200 + 2 + 50))),	// 7C, 7D ' 1 carrier 600 baud 4FSK short
	(int)(1.005 * (20 * 4 * (1 + 200 + 2 + 50))),	// 7C, 7D ' 1 carrier 600 baud 4FSK short
	-1,-1,					// 7E, 7F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,	// 80 - 8F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,	// 90 - 9F
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,	// A0 - AF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,	// B0 - BF
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,	// C0 - CF

	// experimental SOUNDINGs

	(int)(1.005 * 60 * 18 * 40),		// D0
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,		// D1 - DF

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// e0 - eF    ACK and NAK
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	// f0 - ff

// Function to determine if a valide frame type


BOOL IsValidFrameType(UCHAR bytType)
{
	//  used in the minimum distance decoder (update if frames added or removed)

	return (SamplesToComplete[bytType] != -1);
/*
	if (bytType >= 0 && bytType <= 0x1F)  return TRUE;
    if (bytType == 0x23 || bytType == 0x26 || bytType == 0x29 || bytType == 0x2C || bytType == 0x2D || bytType == 0x2E) return TRUE;

	if (bytType >= 0x30 && bytType <= 0x3c)  return TRUE;
	if (bytType >= 0x30 && bytType <= 0x3c)  return TRUE;
	if (bytType >= 0x40 && bytType <= 0x4f)  return TRUE;
	if (bytType >= 0x50 && bytType <= 0x5b)  return TRUE;
	if (bytType >= 0x60 && bytType <= 0x69)  return TRUE;
	if (bytType >= 0x70 && bytType <= 0x7d)  return TRUE;
	if (bytType >= 0xe0 && bytType <= 0xff)  return TRUE;
	if (bytType == 0xd0)  return TRUE;

	return FALSE;
*/
}

// Function to determine if frame type is short control frame
  
BOOL IsShortControlFrame(UCHAR bytType)
{
	if (bytType <= 0x1F) return TRUE;  // NAK
	if (bytType == 0x23 || bytType == 0x26 || bytType == 0x29 || bytType == 0x2C || bytType == 0x2D || bytType == 0x2E) return TRUE; // BREAK, IDLE, DISC, END, ConRejBusy, ConRejBW
	if (bytType >= 0xE0) return TRUE;  // ACK
	return FALSE;
}
 
//	 Function to determine if it is a data frame (Even OR Odd) 

BOOL IsDataFrame(UCHAR intFrameType)
{
	char * String = Name(intFrameType);

	if (String == NULL || String[0] == 0)
		return FALSE;

	if (strstr(String, ".E") || strstr(String, ".O"))
		return TRUE;

	return FALSE;
}

void ResetSNPwrs()
{
	dblSNdBPwr = 0;
	dblSNdBPwr_1 = 0;
	dblSNdBPwr_2 = 0;
}

//    Subroutine to clear all mixed samples 

void ClearAllMixedSamples()
{
	intFilteredMixedSamplesLength = 0;
	intMixedSamplesLength = 0;
	// not sure as this is fixed len intPriorMixedSamplesLength = 120;
	intMFSReadPtr = 0;
}

//  Subroutine to Initialize mixed samples

InitializeMixedSamples()
{
	// Measure the time from release of PTT to leader detection of reply.

	intARQRTmeasuredMs = min(10000, time(NULL) - dttStartRTMeasure); //?????? needs work

	intMixedSamplesLength = 0;	// ' Zero the intMixedSamples array
	intPriorMixedSamplesLength = 120;  // zero out prior samples in Prior sample buffer
	intFilteredMixedSamplesLength = 0;	// zero out the FilteredMixedSamples array
	intMFSReadPtr = 30;				// reset the MFSReadPtr offset 30 to accomodate the filter delay
}

//	Subroutine to discard all sampled prior to current intRcvdSamplesRPtr

DiscardOldSamples()
{
	// This restructures the intRcvdSamples array discarding all samples prior to intRcvdSamplesRPtr
 
	//not sure why we need this !!
/*
	if (RcvdSamplesLen - intRcvdSamplesRPtr <= 0)
		RcvdSamplesLen = intRcvdSamplesRPtr = 0;
	else
	{
		// This is rather slow. I'd prefer a cyclic buffer. Lets see....
		
		memmove(intRcvdSamples, &intRcvdSamples[intRcvdSamplesRPtr], (RcvdSamplesLen - intRcvdSamplesRPtr)* 2);
		RcvdSamplesLen -= intRcvdSamplesRPtr;
		intRcvdSamplesRPtr = 0;
	}
*/
}

//	Subroutine to apply 2000 Hz filter to mixed samples 

FSMixFilter2000Hz()
{
	// assumes sample rate of 12000
	// implements  23 100 Hz wide sections   (~2000 Hz wide @ - 30dB centered on 1500 Hz)

	// FSF (Frequency Selective Filter) variables

	// This works on intMixedSamples, len intMixedSamplesLength;

	// Filtered data is appended to intFilteredMixedSamples

	static float dblR = 0.9995;		// insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
	static int intN = 120;				//Length of filter 12000/100
	static float dblRn;
	static float dblR2;
	static float dblCoef[27] = {0.0};			// the coefficients
	float dblZin = 0, dblZin_1 = 0, dblZin_2 = 0, dblZComb= 0;  // Used in the comb generator
	// The resonators 
      
	float dblZout_0[27] = {0.0};	// resonator outputs
	float dblZout_1[27] = {0.0};	// resonator outputs delayed one sample
	float dblZout_2[27] = {0.0};	// resonator outputs delayed two samples

	int intFilLen = intN / 2;
	int i, j;

	float intFilteredSample = 0;			//  Filtered sample
	float largest = 0;

	dblRn = pow(dblR, intN);

	dblR2 = pow(dblR, 2);

	// Initialize the coefficients
    
	if (dblCoef[26] == 0)
	{
		for (i = 4; i <= 26; i++)
		{
			dblCoef[i] = 2 * dblR * cos(2 * M_PI * i / intN);  // For Frequency = bin i
		}
	}

	for (i = 0; i < intMixedSamplesLength; i++)
	{
		intFilteredSample = 0;

		if (i < intN)
			dblZin = intMixedSamples[i] - dblRn * intPriorMixedSamples[i];
		else 
			dblZin = intMixedSamples[i] - dblRn * intMixedSamples[i - intN];
 
		//Compute the Comb

		dblZComb = dblZin - dblZin_2 * dblR2;
		dblZin_2 = dblZin_1;
		dblZin_1 = dblZin;

		// Now the resonators
		for (j = 4; j <= 26; j++)	   // calculate output for 3 resonators 
		{
			dblZout_0[j] = dblZComb + dblCoef[j] * dblZout_1[j] - dblR2 * dblZout_2[j];
			dblZout_2[j] = dblZout_1[j];
			dblZout_1[j] = dblZout_0[j];

			//' scale each by transition coeff and + (Even) or - (Odd) 
			//' Resonators 2 and 13 scaled by .389 get best shape and side lobe supression 
			//' Scaling also accomodates for the filter "gain" of approx 60. 
 
			if (j == 4 || j == 26)
				intFilteredSample += 0.389 * dblZout_0[j];
			else if ((j & 1) == 0)
				intFilteredSample += dblZout_0[j];
			else
				intFilteredSample -= dblZout_0[j];
		}
		intFilteredMixedSamples[intFilteredMixedSamplesLength++] = (int)ceil(intFilteredSample * 0.00833333333);  // rescales for gain of filter
	}
	
	// update the prior intPriorMixedSamples array for the next filter call 
   
	memmove(intPriorMixedSamples, &intMixedSamples[intMixedSamplesLength - intN], intPriorMixedSamplesLength);		 
}

//	Function to apply 150Hz filter used in Envelope correlator

void Filter150Hz(short * intFilterOut)
{
	// assumes sample rate of 12000
	// implements  3 100 Hz wide sections   (~150 Hz wide @ - 30dB centered on 1500 Hz)

	// FSF (Frequency Selective Filter) variables

	static float dblR = 0.9995;		// insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
	static int intN = 120;				//Length of filter 12000/100
	static float dblRn;
	static float dblR2;
	static float dblCoef[17] = {0.0};			// the coefficients
	float dblZin = 0, dblZin_1 = 0, dblZin_2 = 0, dblZComb= 0;  // Used in the comb generator
	// The resonators 
      
	float dblZout_0[17] = {0.0};	// resonator outputs
	float dblZout_1[17] = {0.0};	// resonator outputs delayed one sample
	float dblZout_2[17] = {0.0};	// resonator outputs delayed two samples

	int intFilLen = intN / 2;
	int i, j;

	float FilterOut = 0;			//  Filtered sample
	float largest = 0;

	dblRn = pow(dblR, intN);

	dblR2 = pow(dblR, 2);

	// Initialize the coefficients
    
	if (dblCoef[17] == 0)
	{
		for (i = 14; i <= 16; i++)
		{
			dblCoef[i] = 2 * dblR * cos(2 * M_PI * i / intN);  // For Frequency = bin i
		}
	}

	for (i = 0; i < 480; i++)
	{
		if (i < intN)
			dblZin = intFilteredMixedSamples[intMFSReadPtr + i] - dblRn * 0;	// no prior mixed samples
		else
			dblZin = intFilteredMixedSamples[intMFSReadPtr + i] - dblRn * intFilteredMixedSamples[intMFSReadPtr + i - intN];

		// Compute the Comb
		
		dblZComb = dblZin - dblZin_2 * dblR2;
		dblZin_2 = dblZin_1;
		dblZin_1 = dblZin;

		// Now the resonators

		for (j = 14; j <= 16; j++)		   // calculate output for 3 resonators 
		{
			dblZout_0[j] = dblZComb + dblCoef[j] * dblZout_1[j] - dblR2 * dblZout_2[j];
			dblZout_2[j] = dblZout_1[j];
			dblZout_1[j] = dblZout_0[j];
	
			//	scale each by transition coeff and + (Even) or - (Odd) 

			// Scaling also accomodates for the filter "gain" of approx 120. 
			// These transition coefficients fairly close to optimum for WGN 0db PSK4, 100 baud (yield highest average quality) 5/24/2014
 
			if (j == 14 || j == 16)
				FilterOut = 0.2 * dblZout_0[j];	 // this transisiton minimizes ringing and peaks
			else
				FilterOut -= dblZout_0[j];
		}
		intFilterOut[i] = (int)ceil(FilterOut * 0.00833333333);	 // rescales for gain of filter
	}
}
// Subroutine to Mix new samples with NCO to tune to nominal 1500 Hz center with reversed sideband and filter. 

void MixNCOFilter(short * intNewSamples, int Length, int intReadPtr, float dblOffsetHz)
{
	// Correct the dimension of intPriorMixedSamples if needed (should only happen after a bandwidth setting change). 

	int i;

	if (Length == 0)
		return;

	// Nominal NCO freq is 3000 Hz  to downmix intNewSamples  (NCO - Fnew) to center of 1500 Hz (invertes the sideband too) 

	dblNCOFreq = 3000 + dblOffsetHz;
	dblNCOPhaseInc = dblNCOFreq * dbl2Pi / 12000;

	intMixedSamplesLength = (Length - 1 - intReadPtr);

	for (i = 0; i < Length - 1 - intReadPtr; i++)
	{
		intMixedSamples[i] = (int)ceil(intNewSamples[intReadPtr + i] * cos(dblNCOPhase));  // later may want a lower "cost" implementation of "Cos"
		dblNCOPhase += dblNCOPhaseInc;
		if (dblNCOPhase > dbl2Pi)
			dblNCOPhase -= dbl2Pi;
	}

	intReadPtr = 0;
	Length = 0;
	
	// showed no significant difference if the 2000 Hz filer used for all bandwidths.
	
	FSMixFilter2000Hz();   // filter through the FS filter (required to reject image from Local oscillator)
}







// Subroutine to process new samples as received from the sound card via Main.ProcessCapturedData
// Only called when not transmitting

void ProcessNewSamples(short * Samples, int nSamples)
{
	int intRcvdSamplesWPtr = 0;
 //       Dim stcStatus As Status = Nothing
	BOOL blnFrameDecodedOK = FALSE;
	
	UCHAR bytData[2000];
/*
            intRcvdSamplesWPtr = intRcvdSamples.Length
            ReDim Preserve intRcvdSamples(intRcvdSamples.Length + bytSamples.Length \ 2 - 1)
            For i As Integer = 0 To bytSamples.Length \ 2 - 1
                intRcvdSamples(intRcvdSamplesWPtr) = System.BitConverter.ToInt16(bytSamples, 2 * i)
                intRcvdSamplesWPtr += 1
            Next i
			*/

	if (ProtocolState == FECSend)
		return;

	// Searching for leader

	if (State == SearchingForLeader)
	{
		// Search for leader as long as 960 samples (8  symbols) available

		while (State == SearchingForLeader && (intRcvdSamplesRPtr + 960) <= nSamples)
		{
			blnLeaderFound = SearchFor2ToneLeader(Samples, intRcvdSamplesRPtr, nSamples, &dblOffsetHz);
		
			if (blnLeaderFound)
			{
				dttLastLeaderDetect = time(NULL);
                //        stcStatus.ControlName = "lblOffset"
                //        stcStatus.Text = "Offset: " & (Format(dblOffsetHz, "#0.0").PadLeft(6)) & " Hz"
                 //       queTNCStatus.Enqueue(stcStatus)

				InitializeMixedSamples();
				State = AcquireSymbolSync;
				ResetSNPwrs();
			}
		}
		if (State == SearchingForLeader)
		{
			DiscardOldSamples();
			return;
		}
	}

	// Got leader

	//	At this point samples haven't been processed, and are in Samples, len nSamples

	//	it looks like we filter through MixNCOFilter
	// Acquire Symbol Sync 

    if (State == AcquireSymbolSync)
	{
		MixNCOFilter(Samples, nSamples, intRcvdSamplesRPtr, dblOffsetHz); // Mix and filter new samples (Mixing consumes all intRcvdSamples)
		nSamples = 0;	//	all used

		if ((intFilteredMixedSamplesLength - intMFSReadPtr) > 500)
		{
			blnSymbolSyncFound = Acquire2ToneLeaderSymbolFraming();  // adjust the pointer to the nominal symbol start based on phase
			if (blnSymbolSyncFound)
				State = AcquireFrameSync;
			else
			{
				DiscardOldSamples();
				ClearAllMixedSamples();
				State = SearchingForLeader;
				return;
			}
		}
	}
	
	//	Acquire Frame Sync
	
	if (State == AcquireFrameSync)
	{
		if (nSamples)		// Aleady used them??
			MixNCOFilter(Samples, nSamples, intRcvdSamplesRPtr, dblOffsetHz);  // Mix and filter new samples
		nSamples = 0;	//	all used

		blnFrameSyncFound = AcquireFrameSyncRSB();
		if (blnFrameSyncFound)
			State = AcquireFrameType;
		else if (intMFSReadPtr > 12000)		 // no Frame sync within 1000 ms (may want to make this limit a funciton of Mode and leaders)
		{
			DiscardOldSamples();
			ClearAllMixedSamples();
			State = SearchingForLeader;
		}
	}
	
	//	Acquire Frame Type

	if (State == AcquireFrameType)
	{
		if (nSamples)		// Aleady used them??
			MixNCOFilter(Samples, nSamples, intRcvdSamplesRPtr, dblOffsetHz);  // Mix and filter new samples
		nSamples = 0;
		
		intFrameType = Acquire4FSKFrameType();
		if (intFrameType == -2)
			return;		// ' isufficient samples 

		if (intFrameType == -1)		  // poor decode quality (large decode distance)
		{
			State = SearchingForLeader;
			ClearAllMixedSamples();
			DiscardOldSamples();

			// stcStatus.BackColor = SystemColors.Control
			// stcStatus.Text = ""
			// stcStatus.ControlName = "lblRcvFrame"
			// queTNCStatus.Enqueue(stcStatus)
		}
		else
		{
			strcpy(strCurrentFrame, Name(intFrameType));

			if (!IsShortControlFrame(intFrameType))
			{
               //         stcStatus.BackColor = Color.Khaki
               //         stcStatus.Text = strCurrentFrame
               //         stcStatus.ControlName = "lblRcvFrame"
               //         queTNCStatus.Enqueue(stcStatus)
			}
	
			intSamplesToCompleteFrame = SamplesToComplete[intFrameType];
			State = AcquireFrame;
			
			if (strcmp(ProtocolMode, "FEC") == 0 && IsDataFrame(intFrameType) && State != FECSend)
				ProtocolState = FECRcv;
		}
	}
	// Acquire Frame

	if (State == AcquireFrame)
	{
		if (nSamples)		// Aleady used them??
			MixNCOFilter(Samples, nSamples, intRcvdSamplesRPtr, dblOffsetHz);  // Mix and filter new samples
		nSamples = 0;

		if ((intFilteredMixedSamplesLength - intMFSReadPtr) >= intSamplesToCompleteFrame + 240)
		{
			blnFrameDecodedOK = DecodeFrame(intFrameType, bytData);
/*		
			if (blnFrameDecodedOK)
			{
				if (strcmp(ProtocolMode, "FEC") == 0)
				{
					if (IsDataFrame(intFrameType))	// ' check to see if a data frame
						ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK);
					else if (intFrameType = 0x30)
						AddTagToDataAndSendToHost(bytData, "IDF");
					else if (intFrameType >= 0x31 && intFrameType <= 0x38)
						ProcessUnconnectedConReqFrame(intFrameType, bytData);
				}				
				else if (strcmp(ProtocolMode, "ARQ") == 0)
				{
					if (!blnTimeoutTriggered)
						ProcessRcvdARQFrame(intFrameType, bytData, blnFrameDecodedOK);  // Process connected ARQ frames here 

					if (ProtocolState == DISC)		  // allows ARQ mode to operate like FEC when not connected
						if (intFrameType == 0x30)				
							AddTagToDataAndSendToHost(bytData, "IDF");			
						else if (intFrameType >= 0x31 && intFrameType <= 0x38)
							ProcessUnconnectedConReqFrame(intFrameType, bytData);			
						else if (IsDataFrame(intFrameType)) // check to see if a data frame
							ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK);			
				}
			}
			else
			{
				//	Bad decode

				if (strcmp(ProtocolMode, "FEC") == 0)
				{
					if (IsDataFrame(intFrameType))	// ' check to see if a data frame
						ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK);
					else if (intFrameType = 0x30)
						AddTagToDataAndSendToHost(bytData, "ERR");
				}				
				else if (strcmp(ProtocolMode, "ARQ") == 0)
				{
					if (ProtocolState == DISC)		  // allows ARQ mode to operate like FEC when not connected
						if (intFrameType == 0x30)				
							AddTagToDataAndSendToHost(bytData, "ERR");			

						else if (IsDataFrame(intFrameType))		// check to see if a data frame
							ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK);

					if (!blnTimeoutTriggered)
						ProcessRcvdARQFrame(intFrameType, bytData, blnFrameDecodedOK);  // Process connected ARQ frames here 
 
				}
  				if (strcmp(ProtocolMode, "FEC") == 0 && ProtocolState != FECSend)
				{
					ProtocolState = DISC;
					InitializeConnection();
				}
			}
			
			State = SearchingForLeader;
			ClearAllMixedSamples();        
			DiscardOldSamples();
			return;
*/
		}
	}
}

void ProcessCapturedData(short * Samples, int nSamples)
{
  //      Dim bytCaptureData(-1) As Byte
	//int intReadPos;
	//int intCapturePos;

  //      Dim stcStatus As Status = Nothing
	static int intRcvPeak = 0;
	static int intGraphicsCtr = 0;
	static int intRecoveryCnt;
	static int intPkRcvLevelCnt = 0;


	if (nSamples < 2)
		return;

 //intRcvPeak = Math.Max(intRcvPeak, Math.Abs(intSample))
      
	if (!Capturing)
		return;

	ProcessNewSamples(Samples, nSamples);


  //          intNextCaptureOffset += bytCaptureData.Length
   //         intNextCaptureOffset = intNextCaptureOffset Mod intCaptureBufferSize ' Circular buffer
}

// Subroutine to compute Goertzel algorithm and return Real and Imag components for a single frequency bin

GoertzelRealImag(short intRealIn[], int intPtr, int N, float m, float * dblReal, float * dblImag)
{
	// intRealIn is a buffer at least intPtr + N in length
	// N need not be a power of 2
	// m need not be an integer
	// Computes the Real and Imaginary Freq values for bin m
	// Verified to = FFT results for at least 10 significant digits
	// Timings for 1024 Point on Laptop (64 bit Core Duo 2.2 Ghz)
	//        GoertzelRealImag .015 ms   Normal FFT (.5 ms)
	//  assuming Goertzel is proportional to N and FFT time proportional to Nlog2N
	//  FFT:Goertzel time  ratio ~ 3.3 Log2(N)

	//  Sanity check

	//if (intPtr < 0 Or (intRealIn.Length - intPtr) < N Then
    //        dblReal = 0 : dblImag = 0 : Exit Sub
     //   End If

	float dblZ_1 = 0.0, dblZ_2 = 0.0, dblW = 0.0;
	float dblCoeff = 2 * cos(2 * M_PI * m / N);
	int i;

	for (i = 0; i <= N; i++)
	{
		if (i == N)
			dblW = dblZ_1 * dblCoeff - dblZ_2;
		else
			dblW = intRealIn[intPtr] + dblZ_1 * dblCoeff - dblZ_2;

		dblZ_2 = dblZ_1;
		dblZ_1 = dblW;
		intPtr += 1;
	}
	*dblReal = 2 * (dblW - cos(2 * M_PI * m / N) * dblZ_2) / N;  // scale results by N/2
	*dblImag = 2 * (sin(2 * M_PI * m / N) * dblZ_2) / N;  // scale results by N/2   (this sign agrees with Scope DSP phase values) 
}

// Function to interpolate spectrum peak using Quinn algorithm

float QuinnSpectralPeakLocator(float XkM1Re, float XkM1Im, float XkRe, float XkIm, float XkP1Re, float XkP1Im)
{
	// based on the Quinn algorithm in Streamlining Digital Processing page 139
	// Alpha1 = Re(Xk-1/Xk)
	// Alpha2 = Re(Xk+1/Xk)
	//Delta1 = Alpha1/(1 - Alpha1)
	//'Delta2 = Alpha2/(1 - Alpha2)
	// if Delta1 > 0 and Delta2 > 0 then Delta = Delta2 else Delta = Delta1
	// should be within .1 bin for S:N > 2 dB

	float dblDenom = pow(XkRe, 2) + pow(XkIm, 2);
	float dblAlpha1;
	float dblAlpha2;
	float dblDelta1;
	float dblDelta2;

	dblAlpha1 = ((XkM1Re * XkRe) + (XkM1Im * XkIm)) / dblDenom;
	dblAlpha2 = ((XkP1Re * XkRe) + (XkP1Im * XkIm)) / dblDenom;
	dblDelta1 = dblAlpha1 / (1 - dblAlpha1);
	dblDelta2 = dblAlpha2 / (1 - dblAlpha2);

	if (dblDelta1 > 0 &&  dblDelta2 > 0)
		return dblDelta2;
	else
		return dblDelta1;
}



// Function to detect and tune the 2 tone leader (for all bandwidths) 

BOOL SearchFor2ToneLeader(short * intNewSamples, int Ptr, int Length, float * dblOffsetHz)
{
	//' Status July 6, 2015 Good performance down to MPP_5dB. Operation over 4 search ranges confirmed (50, 100, 150, 200 Hz) 
	//       Optimized for July 6, 2015
	// search through the samples looking for the telltail 2 tone pattern (nominal tones 1450, 1550 Hz)
	// Find the offset in Hz (due to missmatch in transmitter - receiver tuning
	//'  Finds the power ratio of the tones 1450 and 1550 ratioed to 1350, 1400, 1500, 1600, and 1650

	float dblGoertzelReal[57];
	float dblGoertzelImag[57];
	float dblMag[57];
	float dblPower;
	float dblMaxPeak = 0.0, dblMaxPeakSN, dblInterpM, dblBinAdj, dblBinAdj1450, dblBinAdj1550;
	int intInterpCnt = 0;  // the count 0 to 3 of the interpolations that were < +/- .5 bin
	int  intIatMaxPeak = 0;
	float dblAlpha = 0.3;  // Works well possibly some room for optimization Changed from .5 to .3 on Rev 0.1.5.3
	float dblInterpretThreshold= 1.5; // Good results June 6, 2014 (was .4)  ' Works well possibly some room for optimization
	float dblFilteredMaxPeak = 0;
	int intStartBin, intStopBin;
	float dblLeftCar, dblRightCar, dblBinInterpLeft, dblBinInterpRight, dblCtrR, dblCtrI, dblCtrP, dblLeftP, dblRightP;
	float dblLeftR[2], dblLeftI[2], dblRightR[2], dblRightI[2];
	time_t Now = time(NULL);
	int i;

	if ((Length - Ptr) < 960)
		return FALSE;		// insure there are at least 960 samples (8 symbols of 120 samples)

	// Compute the start and stop bins based on the tuning range Each bin is 12000/960 or 12.5 Hz/bin
 
	if (Connected && (Now - dttLastGoodFrameTypeDecod < 15) || TuningRange == 0)
	{
		// Limited range tuning

		dblLeftCar = 120 - 4 + *dblOffsetHz / 12.5;  // the nominal positions of the two tone carriers based on the last computerd dblOffsetHz
		dblRightCar = 120 + 4 + *dblOffsetHz / 12.5;
		// Calculate 4 bins total for Noise value in S/N computation
		
		GoertzelRealImag(intNewSamples, Ptr, 960, 121 + *dblOffsetHz / 12.5, &dblCtrR, &dblCtrI); // nominal center + 12.5 Hz
		dblCtrP = pow(dblCtrR, 2) + pow(dblCtrI, 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, 119 + *dblOffsetHz / 12.5, &dblCtrR, &dblCtrI); //  center - 12.5 Hz
		dblCtrP += pow(dblCtrR, 2) + pow(dblCtrI, 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, 127 + *dblOffsetHz / 12.5, &dblCtrR, &dblCtrI); // Center + 87.5 Hz
		dblCtrP += pow(dblCtrR,2) + pow(dblCtrI, 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, 113 + *dblOffsetHz / 12.5, &dblCtrR, &dblCtrI); // center - 87.5 Hz
		dblCtrP += pow(dblCtrR,2) + pow(dblCtrI, 2);

		// Calculate one bin above and below the two nominal 2 tone positions for Quinn Spectral Peak locator
		GoertzelRealImag(intNewSamples, Ptr, 960, dblLeftCar - 1, &dblLeftR[0], &dblLeftI[0]);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblLeftCar, &dblLeftR[1], &dblLeftI[1]);
		dblLeftP = pow(dblLeftR[1], 2) + pow(dblLeftI[1],  2);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblLeftCar + 1, &dblLeftR[2], &dblLeftI[2]);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblRightCar - 1, &dblRightR[0], &dblRightI[0]);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblRightCar, &dblRightR[1], &dblRightI[1]);
		dblRightP = pow(dblRightR[1], 2) + pow(dblRightI[1], 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblRightCar + 1, &dblRightR[2], &dblRightI[2]);
		// Calculate the total power in the two tones (use product vs sum to help reject a single carrier).
		dblPower = sqrt(dblLeftP * dblRightP); // sqrt converts back to units of power 
		dblSNdBPwr_1 = dblSNdBPwr;
		dblSNdBPwr = 10 * log(dblPower / (0.25 * dblCtrP)); // Power S:N
                
		if (dblSNdBPwr > 22 && dblSNdBPwr_1 > 19)
		{
			// Calculate the interpolation based on the left of the two tones

			dblBinInterpLeft = QuinnSpectralPeakLocator(dblLeftR[0], dblLeftI[0], dblLeftR[1], dblLeftI[1], dblLeftR[2], dblLeftI[2]);
			// And the right of the two tones
			dblBinInterpRight = QuinnSpectralPeakLocator(dblRightR[0], dblRightI[0], dblRightR[1], dblRightI[1], dblRightR[2], dblRightI[2]); 

			if (abs(dblBinInterpLeft + dblBinInterpRight) < 1.2)
			{
				// sanity check for the interpolators

				if (dblBinInterpLeft + dblBinInterpLeft > 0)
					*dblOffsetHz = *dblOffsetHz + min((dblBinInterpLeft + dblBinInterpRight) * 6.25, 3);  // average left and right, adjustment bounded to +/- 3Hz max
				else
					*dblOffsetHz = *dblOffsetHz + max((dblBinInterpLeft + dblBinInterpRight) * 6.25, -3);
                    
				//strDecodeCapture = "Interp Ldr;S:N=" & Format(dblSNdBPwr, "#.0") & "dB, Offset=" & Format(dblOffsetHz, "##0.0") & "Hz: ";
				dttStartRmtLeaderMeasure = Now;
				blnLeaderDetected = TRUE;

				//if (AccumulateStats)
				//{
				//	With stcTuningStats
                //     .dblLeaderSNAvg = ((.dblLeaderSNAvg * .intLeaderDetects) + dblSNdBPwr) / (1 + .intLeaderDetects)
                //     .intLeaderDetects += 1
                 //           End With
				//}
				return TRUE;
			}
		}
		Ptr += 480;
		blnLeaderDetected = FALSE;
		return FALSE;
	}

	// this is the full search over the full tuning range selected.  Uses more CPU time and with possibly larger deviation once connected. 
		
	intStartBin = ((200 - TuningRange) / 12.5);
		//intStopBin = ((dblMag.Length - 1) - ((200 - TuningRange) / 12.5));
	intStopBin = ((57 - 1) - ((200 - TuningRange) / 12.5));
	// Generate the Power magnitudes for up to 46 12.5 Hz bins (a function of MCB.TuningRange) 
      
	for (i = intStartBin; i <= intStopBin; i++)
	{
		GoertzelRealImag(intNewSamples, Ptr, 960, i + 92, &dblGoertzelReal[i], &dblGoertzelImag[i]);
			dblMag[i] = pow(dblGoertzelReal[i], 2) + pow(dblGoertzelImag[i], 2); // dblMag(i) in units of power (V^2)
	}
		
	//Search for the bins for the max power in the two tone signal.  

	for (i = intStartBin ; i <= intStopBin - 24; i++)	// ' +/- MCB.TuningRange from nominal 
	{
		dblPower = sqrt(dblMag[i + 8] * dblMag[i + 16]); // using the product to minimize sensitivity to one strong carrier vs the two tone
		// sqrt coonverts back to units of power from Power ^2
		if (dblPower > dblMaxPeak)
		{
			dblMaxPeak = dblPower;
			intIatMaxPeak = i + 104;
		}
	}

		// Now compute the max peak:Noise (power)  using the product of the 2 tone bins @ 1450 and 1550 Hz (nominal...spaced at 100 Hz)
		// Divided by the Noise power at nominal bins 1350, 1400, 1500, 1600, 1650 Hz
		// Denominator uses average of 5 "noise" bins to reduce variation. Factor of .2 adjusts for the #Bins in the  Denom 
		// Note sum vs product appeared more consistant with multipath poor. 

		dblMaxPeakSN = dblMaxPeak / (0.2 * (dblMag[intIatMaxPeak - 104] + dblMag[intIatMaxPeak - 100] +
               dblMag[intIatMaxPeak - 92] + dblMag[intIatMaxPeak - 84] + dblMag[intIatMaxPeak - 80]));
               // 'dblMaxPeakSN = dblMaxPeak / (0.3333 * (dblMag(intIatMaxPeak - 100) + dblMag(intIatMaxPeak - 92) + dblMag(intIatMaxPeak - 84)))
		dblSNdBPwr_2 = dblSNdBPwr_1;
		dblSNdBPwr_1 = dblSNdBPwr;
		dblSNdBPwr = 10 * log(dblMaxPeakSN);
		if (dblSNdBPwr > 29 && dblSNdBPwr_1 > 24) // These values selected during optimizatin tests 7/5/2015 @ mpp -5 dB S:N
		{
			// Do the interpolation based on the two carriers at nominal 1450 and 1550Hz
    
			if ((intIatMaxPeak - 97) >= intStartBin && (intIatMaxPeak - 87) <= intStopBin) // check to insure no index errors
			{
				// Interpolate the adjacent bins using QuinnSpectralPeakLocator
					
					dblBinAdj1450 = QuinnSpectralPeakLocator(dblGoertzelReal[intIatMaxPeak - 97], dblGoertzelImag[intIatMaxPeak - 97],
							dblGoertzelReal[intIatMaxPeak - 96], dblGoertzelImag[intIatMaxPeak - 96],
							dblGoertzelReal[intIatMaxPeak - 95], dblGoertzelImag[intIatMaxPeak - 95]);
				if (dblBinAdj1450 < dblInterpretThreshold && dblBinAdj1450 > -dblInterpretThreshold)
				{
					dblBinAdj = dblBinAdj1450;
					intInterpCnt += 1;
				}

				dblBinAdj1550 = QuinnSpectralPeakLocator(dblGoertzelReal[intIatMaxPeak - 89], dblGoertzelImag[intIatMaxPeak - 89], 
						dblGoertzelReal[intIatMaxPeak - 88], dblGoertzelImag[intIatMaxPeak - 88], 
						dblGoertzelReal[intIatMaxPeak - 87], dblGoertzelImag[intIatMaxPeak - 87]);

				if (dblBinAdj1550 < dblInterpretThreshold && dblBinAdj1550 > -dblInterpretThreshold)
				{
					dblBinAdj += dblBinAdj1550;
					intInterpCnt += 1;
				}
				if (intInterpCnt == 0)
				{
					Ptr += 480;
					blnLeaderDetected = FALSE;
					return FALSE;
				}
				else
				{
					dblBinAdj = dblBinAdj / intInterpCnt; // average the offsets that are within .5 bin
					dblInterpM = intIatMaxPeak + dblBinAdj;
				}
			}
			else
			{
				Ptr += 480; // ' ..reduces CPU loading
				return FALSE;
			}
			// update the offsetHz and setup the NCO new freq and Phase inc. Note no change to current NCOphase

			*dblOffsetHz = 12.5 * (dblInterpM - 120); // compute the tuning offset in Hz using dblInterpM
		}
		else
		{
			Ptr += 480;  //  Ptr += 240 ' evaluate if this is OK ..reduces CPU loading
			blnLeaderDetected = FALSE;
			return FALSE;
		}
     
		dblNCOFreq = 3000 + *dblOffsetHz;  //Set the NCO frequency and phase inc for mixing 
		dblNCOPhaseInc = 2 * M_PI * dblNCOFreq / 12000;
		Ptr = Ptr + 480; // advance 4 symbols to avoid any noise in start ' optimize?
		State = AcquireSymbolSync;
		blnLeaderDetected = TRUE;
        //        If MCB.AccumulateStats Then
         //           With stcTuningStats
        //                .dblLeaderSNAvg = ((.dblLeaderSNAvg * .intLeaderDetects) + dblSNdBPwr) / (1 + .intLeaderDetects)
         //               .intLeaderDetects += 1
         //           End With
         //       End If

		//strDecodeCapture = "Ldr;S:N=" & Format(dblSNdBPwr, "#.0") & "dB, Offset=" & Format(dblOffsetHz, "##0.0") & "Hz: ";
		dttStartRmtLeaderMeasure = Now;
		return TRUE;
}

//	Function to look at the 2 tone leader and establishes the Symbol framing using envelope search and minimal phase error. 

BOOL Acquire2ToneLeaderSymbolFraming()
{
	float dblCarPh;
	float dblReal, dblImag;
	int intLocalPtr = intMFSReadPtr;  // try advancing one symbol to minimize initial startup errors 
	float dblAbsPhErr;
	float dblMinAbsPhErr = 5000;	 // initialize to an excessive value
	int intIatMinErr;
	float dblPhaseAtMinErr;
	int intAbsPeak = 0;
	int intJatPeak = 0;
	int i;

	// Use Phase of 1500 Hz leader  to establish symbol framing. Nominal phase is 0 or 180 degrees

	if ((intFilteredMixedSamplesLength - intLocalPtr) < 500)
		return FALSE;			// not enough
	
	intLocalPtr = intMFSReadPtr + EnvelopeCorrelator(); // should position the pointer at the symbol boundary

	// Check 3 samples either side of the intLocalPtr for minimum phase error.(closest to Pi or -Pi) 
    
	for (i = -3; i <= 3; i++)	 // 0 To 0 '  -2 To 2 ' for just 5 samples
	{
		// using the full symbol seemed to work best on weak Signals (0 to -5 dB S/N) June 15, 2015
	
		GoertzelRealImag(intFilteredMixedSamples, intLocalPtr + i, 120, 15, &dblReal, &dblImag); // Carrier at 1500 Hz nominal Positioning 
		dblCarPh = atan2(dblImag, dblReal);
		dblAbsPhErr = abs(dblCarPh - (ceil(dblCarPh / M_PI) * M_PI));
		if (dblAbsPhErr < dblMinAbsPhErr)
		{
			dblMinAbsPhErr = dblAbsPhErr;
			intIatMinErr = i;
			dblPhaseAtMinErr = dblCarPh;
		}     
	}

	intMFSReadPtr = intLocalPtr + intIatMinErr;
	// Debug.WriteLine("[Acquire2ToneLeaderSymbolFraming] intIatMinError=" & intIatMinErr.ToString)
	State = AcquireFrameSync;

	//if (AccumulateStats)
	 //   intLeaderSyncs += 1;

	//Debug.WriteLine("   [Acquire2ToneLeaderSymbolSync] iAtMinError = " & intIatMinErr.ToString & "   Ptr = " & intMFSReadPtr.ToString & "  MinAbsPhErr = " & Format(dblMinAbsPhErr, "#.00"))
	//Debug.WriteLine("   [Acquire2ToneLeaderSymbolSync]      Ph1500 @ MinErr = " & Format(dblPhaseAtMinErr, "#.000"))
        
	//strDecodeCapture &= "Framing; iAtMinErr=" & intIatMinErr.ToString & ", Ptr=" & intMFSReadPtr.ToString & ", MinAbsPhErr=" & Format(dblMinAbsPhErr, "#.00") & ": "

	return TRUE;
}

// Function to establish symbol sync 

int EnvelopeCorrelator()
{
	// Compute the two symbol correlation with the Two tone leader template.
	// slide the correlation one sample and repeat up to 120 steps 
	// keep the point of maximum or minimum correlation...and use this to identify the the symbol start. 

	float dblCorMax  = -1000000.0;		//  Preset to excessive values
	float dblCorMin  = 1000000.0;
	int intJatMax = 0, intJatMin = 0;
	float dblCorSum;
	int i,j;
	int int150HzFilered[480];

	if (intFilteredMixedSamplesLength < intMFSReadPtr + 480)
		return -1;
	
	Filter150Hz(int150HzFilered); // This filter appears to help reduce avg decode distance (10 frames) by about 14%-19% at WGN-5 May 3, 2015
	
	for (j = 0; j < 120; j++)
	{
		dblCorSum = 0;
		for (i = 0; i < 240; i++)	 // over two 100 baud symbols (may be able to reduce to 1 symbol)
		if (i < 120)
			dblCorSum += intTwoToneLeaderTemplate[i] * int150HzFilered[120 + i + j];
		else
			dblCorSum -= intTwoToneLeaderTemplate[i - 120] * int150HzFilered[120 + i + j];
	}
	
	if (dblCorSum > dblCorMax)
	{
		dblCorMax = dblCorSum;             
		intJatMax = j;
	}
	else if (dblCorSum < dblCorMin)
	{
		dblCorMin = dblCorSum;
		intJatMin = j;
	}

	if (dblCorMax > abs(dblCorMin))
		return intJatMax;
	else
		return intJatMin + 120;
}

//	Function to acquire the Frame Sync for all Frames 

BOOL AcquireFrameSyncRSB()
{
	// Two improvements could be incorporated into this function:
	//    1) Provide symbol tracking until the frame sync is found (small corrections should be less than 1 sample per 4 symbols ~2000 ppm)
	//    2) Ability to more accurately locate the symbol center (could be handled by symbol tracking 1) above. 

	//  This is for acquiring FSKFrameSync After Mixing Tones Mirrored around 1500 Hz. e.g. Reversed Sideband
	//  Frequency offset should be near 0 (normally within +/- 1 Hz)  
	//  Locate the sync Symbol which has no phase change from the prior symbol (BPSK leader @ 1500 Hz)   

	int intLocalPtr = intMFSReadPtr;
	int intAvailableSymbols = (intFilteredMixedSamplesLength - intMFSReadPtr) / 120;
	float dblPhaseSym1;	//' phase of the first symbol 
	float dblPhaseSym2;	//' phase of the second symbol 
	float dblPhaseSym3;	//' phase of the third symbol

	float dblReal, dblImag;
	float dblPhaseDiff12, dblPhaseDiff23;

	int i;

	if (intAvailableSymbols < 3)
		return FALSE;				// must have at least 360 samples to search
 
	// Calculate the Phase for the First symbol 
	
	GoertzelRealImag(intFilteredMixedSamples, intLocalPtr, 120, 15, &dblReal, &dblImag); // Carrier at 1500 Hz nominal Positioning with no cyclic prefix
	dblPhaseSym1 = atan2(dblImag, dblReal);
	intLocalPtr += 120l;	// advance one symbol
	GoertzelRealImag(intFilteredMixedSamples, intLocalPtr, 120, 15, &dblReal, &dblImag); // Carrier at 1500 Hz nominal Positioning with no cyclic prefix
	dblPhaseSym2 = atan2(dblImag, dblReal);
	intLocalPtr += 120;		// advance one symbol

	for (i = 0; i <=  intAvailableSymbols - 4; i++)
	{
		// Compute the phase of the next symbol  
	
		GoertzelRealImag(intFilteredMixedSamples, intLocalPtr, 120, 15, &dblReal, &dblImag); // Carrier at 1500 Hz nominal Positioning with no cyclic prefix
		dblPhaseSym3 = atan2(dblImag, dblReal);
		// Compute the phase differences between sym1-sym2, sym2-sym3
		dblPhaseDiff12 = dblPhaseSym1 - dblPhaseSym2;
		if (dblPhaseDiff12 > M_PI)		// bound phase diff to +/- Pi
			dblPhaseDiff12 -= dbl2Pi;
		else if (dblPhaseDiff12 < -M_PI)
			dblPhaseDiff12 += dbl2Pi;

		dblPhaseDiff23 = dblPhaseSym2 - dblPhaseSym3;
		if (dblPhaseDiff23 > M_PI)		//  bound phase diff to +/- Pi
			dblPhaseDiff23 -= dbl2Pi;
		else if (dblPhaseDiff23 < -M_PI)
			dblPhaseDiff23 += dbl2Pi;

		if (abs(dblPhaseDiff12) > 0.6667 * M_PI && abs(dblPhaseDiff23) < 0.3333 * M_PI)  // Tighten the margin to 60 degrees
		{
			intPSKRefPhase = (short)dblPhaseSym3 * 1000;

			intLeaderRcvdMs = (int)ceil((intLocalPtr - 30) / 12);	 // 30 is to accomodate offset of inital pointer for filter length. 
			intMFSReadPtr = intLocalPtr + 120;		 // Position read pointer to start of the symbol following reference symbol 
	
//	If (AccumulateStats)
//		intFrameSyncs += 1;		 // accumulate tuning stats
	//'strDecodeCapture &= "Sync; Phase1>2=" & Format(dblPhaseDiff12, "0.00") & " Phase2>3=" & Format(dblPhaseDiff23, "0.00") & ": "
	
			return TRUE;	 // pointer is pointing to first 4FSK data symbol. (first symbol of frame type)
		}
		else
		{
			dblPhaseSym1 = dblPhaseSym2;           
			dblPhaseSym2 = dblPhaseSym3;
			intLocalPtr += 120;			// advance one symbol 
		}
	}

	intMFSReadPtr = intLocalPtr - 240;		 // back up 2 symbols for next attempt (Current Sym2 will become new Sym1)
	return FALSE;	
}

//	 Function to Demod FrameType4FSK

BOOL DemodFrameType4FSK(int intPtr, short * intSamples, int * intToneMags)
{
	float dblReal, dblImag;
	int i;

	if ((intFilteredMixedSamplesLength - intPtr) < 2400)
		return FALSE;

	for (i = 0; i < 10; i++)
	{
		GoertzelRealImag(intSamples, intPtr, 240, 1575 / 50.0, &dblReal, &dblImag);
		intToneMags[4 * i] = (int)pow(dblReal, 2) + pow(dblImag, 2);
		GoertzelRealImag(intSamples, intPtr, 240, 1525 / 50.0, &dblReal, &dblImag);
		intToneMags[1 + 4 * i] = (int)pow(dblReal, 2) + pow(dblImag, 2);
		GoertzelRealImag(intSamples, intPtr, 240, 1475 / 50.0, &dblReal, &dblImag);
		intToneMags[2 + 4 * i] = (int)pow(dblReal, 2) + pow(dblImag, 2);
		GoertzelRealImag(intSamples, intPtr, 240, 1425 / 50.0, &dblReal, &dblImag);
		intToneMags[3 + 4 * i] = (int)pow(dblReal, 2) + pow(dblImag, 2);
		intPtr += 240;
	}
	
	return TRUE;
}
/*
    ' Function to demodulate one carrier for all low baud rate 4FSK frame types
    Private Function Demod1Car4FSK(intPtr As Int32, ByRef intSamples() As Int32, intBaud As Int32, intCenterFreq As Int32, intNumOfSymbols As Int32, ByRef bytData() As Byte, ByRef intToneMags() As Int32) As Boolean
        ' Converts intSamples to an array of bytes demodulating the 4FSK symbols with center freq intCenterFreq
        ' intPtr should be pointing to the approximate start of the first data symbol  
        ' Updates bytData() with demodulated bytes
        ' Updates bytMinSymQuality with the minimum (range is 25 to 100) symbol making up each byte.

        Dim intSampPerSymbol As Int32 = CInt(12000 / intBaud)
        Dim dblReal, dblImag As float
        Dim intSearchFreq As Integer
        Dim dblMagSum As float = 0
        Dim dblMag(3) As float ' The magnitude for each of the 4FSK frequency bins
        Dim bytSym As Byte
        Static bytSymHistory(2) As Byte

        If (intSamples.Length - intPtr) < (intSampPerSymbol * intNumOfSymbols) Then
            Logs.Exception("[DemodulateDecode.Demod1Car4FSK] Insufficient Samples for requested " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If (intNumOfSymbols Mod 4) <> 0 Then
            Logs.Exception("[DemodulateDecode.Demod1Car4FSK] Number of Symbols not a multiple of 4: " & intNumOfSymbols.ToString & " symbols.")
            Return False
        End If
        If MCB.AccumulateStats Then stcTuningStats.intFSKSymbolCnt += intNumOfSymbols
        ReDim intToneMags(4 * intNumOfSymbols - 1)
        ReDim bytData(intNumOfSymbols \ 4 - 1)

        For i As Integer = 0 To bytData.Length - 1 ' For each data byte
            intSearchFreq = CInt(intCenterFreq + 1.5 * intBaud) ' the highest freq (equiv to lowest sent freq because of sideband reversal)
            For j As Integer = 0 To 3 ' for each 4FSK symbol (2 bits) in a byte
                dblMagSum = 0
                GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, intSearchFreq / intBaud, dblReal, dblImag)
                dblMag(0) = (dblReal ^ 2) + (dblImag ^ 2)
                dblMagSum += dblMag(0)
                GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (intSearchFreq - intBaud) / intBaud, dblReal, dblImag)
                dblMag(1) = (dblReal ^ 2) + (dblImag ^ 2)
                dblMagSum += dblMag(1)
                GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (intSearchFreq - 2 * intBaud) / intBaud, dblReal, dblImag)
                dblMag(2) = (dblReal ^ 2) + (dblImag ^ 2)
                dblMagSum += dblMag(2)
                GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (intSearchFreq - 3 * intBaud) / intBaud, dblReal, dblImag)
                dblMag(3) = (dblReal ^ 2) + (dblImag ^ 2)
                dblMagSum += dblMag(3)
                If dblMag(0) > dblMag(1) And dblMag(0) > dblMag(2) And dblMag(0) > dblMag(3) Then
                    bytSym = 0
                ElseIf dblMag(1) > dblMag(0) And dblMag(1) > dblMag(2) And dblMag(1) > dblMag(3) Then
                    bytSym = 1
                ElseIf dblMag(2) > dblMag(0) And dblMag(2) > dblMag(1) And dblMag(2) > dblMag(3) Then
                    bytSym = 2
                Else
                    bytSym = 3
                End If
                bytData(i) = (bytData(i) << 2) + bytSym
                intToneMags(16 * i + 4 * j) = dblMag(0)
                intToneMags(16 * i + 4 * j + 1) = dblMag(1)
                intToneMags(16 * i + 4 * j + 2) = dblMag(2)
                intToneMags(16 * i + 4 * j + 3) = dblMag(3)
                bytSymHistory(0) = bytSymHistory(1)
                bytSymHistory(1) = bytSymHistory(2)
                bytSymHistory(2) = bytSym
                If (bytSymHistory(0) <> bytSymHistory(1)) And (bytSymHistory(1) <> bytSymHistory(2)) Then ' only track when adjacent symbols are different (statistically about 56% of the time)
                    ' this should allow tracking over 2000 ppm sampling rate error
                    Track1Car4FSK(intSamples, intPtr, intSampPerSymbol, intSearchFreq, intBaud, bytSymHistory)
                End If
                intPtr += intSampPerSymbol ' advance the pointer one symbol
            Next j
        Next i
        Return True
    End Function  '  Demod1Car4FSK

*/ 

// Function to compute the "distance" from a speicific bytFrame Xored by bytID using 1 symbol parity 

float ComputeDecodeDistance(int intTonePtr, int * intToneMags, UCHAR bytFrameType, UCHAR bytID)
{
	// intTonePtr is the offset into the Frame type symbols. 0 for first Frame byte 20 = (5 x 4) for second frame byte 

	float dblDistance = 0;
	int int4ToneSum;
	int intToneIndex;
	UCHAR bytMask = 0xC0;
	int j, k;

	for (j = 0; j <= 4; j++)		//  over 5 symbols
	{
		int4ToneSum = 0;
		for (k = 0; k <=3; k++)
		{
			int4ToneSum += intToneMags[intTonePtr + (4 * j) + k];
		}
		if (int4ToneSum == 0)
			int4ToneSum = 1;		//  protects against possible overflow
		if (j < 4)
		    intToneIndex = ((bytFrameType ^ bytID) & bytMask) >> (6 - 2 * j);
		else
			intToneIndex = ComputeTypeParity(bytFrameType);

		dblDistance += 1.0 - ((1.0 * intToneMags[intTonePtr + (4 * j) + intToneIndex]) / (1.0 * int4ToneSum));
		bytMask = bytMask >> 2;
	}
	
	dblDistance = dblDistance / 5;		// normalize back to 0 to 1 range 
	return dblDistance;
}


//	Function to compute the frame type by selecting the minimal distance from all valid frame types.

int MinimalDistanceFrameType(int * intToneMags, UCHAR bytSessionID)
{
	float dblMinDistance1 = 5; // minimal distance for the first byte initialize to large value
	float dblMinDistance2 = 5; // minimal distance for the second byte initialize to large value
	float dblMinDistance3 = 5; // minimal distance for the second byte under exceptional cases initialize to large value
	int intIatMinDistance1, intIatMinDistance2, intIatMinDistance3;
	float dblDistance1, dblDistance2, dblDistance3;
	int i;


	for (i = 0; i < bytValidFrameTypesLength; i++)
	{
		dblDistance1 = ComputeDecodeDistance(0, intToneMags, bytValidFrameTypes[i], 0);
		dblDistance2 = ComputeDecodeDistance(20, intToneMags, bytValidFrameTypes[i], bytSessionID);

		if (Pending)
		    dblDistance3 = ComputeDecodeDistance(20, intToneMags, bytValidFrameTypes[i], 0xFF);
		else
			dblDistance3 = ComputeDecodeDistance(20, intToneMags, bytValidFrameTypes[i], bytLastARQSessionID);

		if (dblDistance1 < dblMinDistance1)
		{
			dblMinDistance1 = dblDistance1;
			intIatMinDistance1 = bytValidFrameTypes[i];
		}
		if (dblDistance2 < dblMinDistance2)
		{
			dblMinDistance2 = dblDistance2;
			intIatMinDistance2 = bytValidFrameTypes[i];
		}
		if (dblDistance3 < dblMinDistance3)
		{
			dblMinDistance3 = dblDistance3;
			intIatMinDistance3 = bytValidFrameTypes[i];
		}
	}
	
	if (bytSessionID == 0xFF)		// ' we are in a FEC QSO, monitoring an ARQ session or have not yet reached the ARQ Pending or Connected status 
	{
		// This handles the special case of a DISC command received from the prior session (where the station sending DISC did not receive an END). 

		if (intIatMinDistance1 == 0x29 && intIatMinDistance3 == 0x29 && ((dblMinDistance1 < 0.4) || (dblMinDistance3 < 0.4)))
		{
			// strDecodeCapture &= "MD Decode;1 ID=H" & Format(.bytLastARQSessionID, "X") & ", Type=H29:" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D3=" & Format(dblMinDistance3, "#.00")
			
			//if (DebugLog)
			//	WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture);

			return intIatMinDistance1;
		}
		
		// no risk of damage to and existing ARQConnection with END, BREAK, DISC, or ACK frames so loosen decoding threshold 

		if (intIatMinDistance1 == intIatMinDistance2 && ((dblMinDistance1 < 0.4) || (dblMinDistance2 < 0.4)))
		{
			// strDecodeCapture &= "MD Decode;2 ID=H" & Format(bytSessionID, "X") & ", Type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
            //        If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
			dblOffsetLastGoodDecode = dblOffsetHz;
			dttLastGoodFrameTypeDecode = Now;
			return intIatMinDistance1;
		}
		if ((dblMinDistance1 < 0.3) && (dblMinDistance1 < dblMinDistance2) && IsDataFrame(intIatMinDistance1) )	//  this would handle the case of monitoring an ARQ connection where the SessionID is not &HFF
		{
			//strDecodeCapture &= "MD Decode;3 ID=H" & Format(bytSessionID, "X") & ", Type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
            //   If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
			
			return intIatMinDistance1;
		}

		if ((dblMinDistance2 < 0.3) && (dblMinDistance2 < dblMinDistance1) && IsDataFrame(intIatMinDistance2))  // this would handle the case of monitoring an FEC transmission that failed above when the session ID is = &HFF
		{
			//strDecodeCapture &= "MD Decode;4 ID=H" & Format(bytSessionID, "X") & ", Type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")      
			//If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
 
			return intIatMinDistance2;
		}

		//	strDecodeCapture &= "MD Decode;5 Type1=H" & Format(intIatMinDistance1, "X") & ", Type2=H" & Format(intIatMinDistance2, "X") & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
		//  If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
	}
	else if (Pending)			 // We have a Pending ARQ connection 
	{
		// this should be a Con Ack from the ISS if we are Pending

		if (intIatMinDistance1 == intIatMinDistance2)  // matching indexes at minimal distances so high probablity of correct decode.
		{
			//strDecodeCapture &= "MD Decode;6 ID=H" & Format(bytSessionID, "X") & ", type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
			if ((dblMinDistance1 < 0.4) || (dblMinDistance2 < 0.4))
			{
				dblOffsetLastGoodDecode = dblOffsetHz;
				dttLastGoodFrameTypeDecode = Now;		// This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
				// MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
				return intIatMinDistance1;
			}
			else
			{
				//if MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
				return -1;		 // indicates poor quality decode so  don't use
			}
		}
	}
	//	handles the case of a received ConReq frame based on an ID of &HFF (ISS must have missed ConAck reply from IRS so repeated ConReq)

	else if (intIatMinDistance1 == intIatMinDistance3)	 //matching indexes at minimal distances so high probablity of correct decode.
	{
		//strDecodeCapture &= "MD Decode;7 ID=H" & Format(&HFF, "X") & ", type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D3=" & Format(dblMinDistance3, "#.00")
		if (intIatMinDistance1 >= 0x31 && intIatMinDistance1 <= 0x38 && ((dblMinDistance1 < 0.4) || (dblMinDistance3 < 0.4)))  // Check for ConReq (ISS must have missed previous ConAck  
		{
			dblOffsetLastGoodDecode = dblOffsetHz;
			dttLastGoodFrameTypeDecode = Now;		 // This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
			//If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
			return intIatMinDistance1;
		}
		else
		{
			//if MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)

			return -1;	 // indicates poor quality decode so  don't use
		}
	}
	else if (Connected)		// ' we have an ARQ connected session.
	{
			//if AccumulateStats Then
              //      With stcTuningStats
                //        .dblAvgDecodeDistance = (.dblAvgDecodeDistance * .intDecodeDistanceCount + 0.5 * (dblMinDistance1 + dblMinDistance2)) / (.intDecodeDistanceCount + 1)
                  //      .intDecodeDistanceCount += 1
                   // End With
               // End If
			
		if (intIatMinDistance1 == intIatMinDistance2) // matching indexes at minimal distances so high probablity of correct decode.
		{
			if ((intIatMinDistance1 >= 0xE0 && intIatMinDistance1 <=0xFF) || (intIatMinDistance1 == 0x23) || 
				(intIatMinDistance1 == 0x2C) || (intIatMinDistance1 == 0x29))  // Check for critical ACK, BREAK, END, or DISC frames  
			{
				//strDecodeCapture &= "MD Decode;8 ID=H" & Format(bytSessionID, "X") & ", Critical type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
				if ((dblMinDistance1 < 0.25) || (dblMinDistance2 < 0.25)) // use tighter limits   here
				{
					dblOffsetLastGoodDecode = dblOffsetHz;
					dttLastGoodFrameTypeDecode = Now;		 // This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
					//If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
					return intIatMinDistance1;
				}
				else
				{
					//if MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
					return -1;		 // indicates poor quality decode so  don't use
				}
			}
			else	//  non critical frames
			{
				//strDecodeCapture &= "MD Decode;9 ID=H" & Format(bytSessionID, "X") & ", Type=H" & Format(intIatMinDistance1, "X") & ":" & objFrameInfo.Name(intIatMinDistance1) & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
				//  use looser limits here, there is no risk of protocol damage from these frames
				if ((dblMinDistance1 < 0.4) || (dblMinDistance2 < 0.4))
				{
					//If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode OK  ] " & strDecodeCapture)
						
					dblOffsetLastGoodDecode = dblOffsetHz;
					dttLastGoodFrameTypeDecode = Now;	 // This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
					return intIatMinDistance1;
				}
				else
				{
					//if MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
					return -1;		// indicates poor quality decode so  don't use
				}
			}
		}
		else		//non matching indexes
		{
			//strDecodeCapture &= "MD Decode;10  Type1=H" & Format(intIatMinDistance1, "X") & ", Type2=H" & Format(intIatMinDistance2, "X") & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
				//If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture)
			return -1; // indicates poor quality decode so  don't use
		}
	}
	//  strDecodeCapture &= "MD Decode;11  Type1=H" & Format(intIatMinDistance1, "X") & ", Type2=H" & Format(intIatMinDistance2, "X") & ", D1=" & Format(dblMinDistance1, "#.00") & ", D2=" & Format(dblMinDistance2, "#.00")
	//If MCB.DebugLog Then Logs.WriteDebug("[Frame Type Decode Fail] " & strDecodeCapture) 
	return -1; // indicates poor quality decode so  don't use
}


//	Function to acquire the 4FSK frame type

int Acquire4FSKFrameType()
{
	// intMFSReadPtr is pointing to start of first symbol of Frame Type (total of 10 4FSK symbols in frame type (2 bytes) + 1 parity symbol per byte 
	// Returns -1 if minimal distance decoding is below threshold (low likelyhood of being correct)
	// Returns -2 if insufficient samples 
	// Else returns frame type 0-255

	int NewType = 0;
	int intToneMags[100] = {0};		// ?? len

	if ((intFilteredMixedSamplesLength - intMFSReadPtr) < (240 * 10))
		return -2;		//  Check for 12 available 4FSK Symbols (but only 10 are used)  

	if (!DemodFrameType4FSK(intMFSReadPtr, intFilteredMixedSamples, intToneMags))
	{
//		Update4FSKConstellation(intToneMags, intLastRcvdFrameQuality);
		intMFSReadPtr += (240 * 10);
		return -1;
	}
	
	intRmtLeaderMeasure = (Now - dttStartRmtLeaderMeasure);

	// Now do check received  Tone array for testing minimum distance decoder
	
	// don't like this, as it is mixing modem functions with proto states

	if (Pending)			// If we have a pending connection (btween the IRS first decode of ConReq until it receives a ConAck from the iSS)  
		NewType = MinimalDistanceFrameType(intToneMags, PendingSessionID);		 // The pending session ID will become the session ID once connected) 
	else if (Connected)		// If we are connected then just use the stcConnection.bytSessionID
		NewType = MinimalDistanceFrameType(intToneMags, bytSessionID);
	else					// not connected and not pending so use &FF (FEC or ARQ unconnected session ID
		NewType = MinimalDistanceFrameType(intToneMags, 0xFF);
  
//	if (NewType > 0x30 && NewType < 0x39)
//		QueueCommandToHost("PENDING");			 // early pending notice to stop scanners

//	if (NewType >= 0 &&  IsShortControlFrame(NewType))		// update the constellation if a short frame (no data to follow)
//		Update4FSKConstellation(intToneMags, intLastRcvdFrameQuality);

	//if (AccumulateStats)
	//	if (Acquire4FSKFrameType > 0)
	//		intGoodFSKFrameTypes += 1;
	//	else
	//		intFailedFSKFrameTypes += 1;
	
	intMFSReadPtr += (240 * 10);			 // advance to read pointer to the next symbol (if there is one)
	
	return NewType;
}
/*
   ' Subroutine to track 1 carrier 4FSK. Used for both single and multiple simultaneous carrier 4FSK modes.
    Private Sub Track1Car4FSK(ByRef intSamples() As Int32, ByRef intPtr As Int32, intSampPerSymbol As Int32, intSearchFreq As Int32, intBaud As Int32, bytSymHistory() As Byte)
        'look at magnitude of the tone for bytHistory(1)  2 sample2 earlier and 2 samples later.  and pick the maximum adjusting intPtr + or - 1
        ' this seems to work fine on test Mar 16, 2015. This should handle sample rate offsets (sender to receiver) up to about 2000 ppm

        Dim dblReal, dblImag, dblMagEarly, dblMag, dblMagLate As float
        Dim dblBinToSearch As float = (intSearchFreq - (intBaud * bytSymHistory(1))) / intBaud ' select the 2nd last symbol for magnitude comparison
        Try
            GoertzelRealImag(intSamples, (intPtr - intSampPerSymbol - 2), intSampPerSymbol, dblBinToSearch, dblReal, dblImag)
            dblMagEarly = (dblReal ^ 2) + (dblImag ^ 2)
            GoertzelRealImag(intSamples, (intPtr - intSampPerSymbol), intSampPerSymbol, dblBinToSearch, dblReal, dblImag)
            dblMag = (dblReal ^ 2) + (dblImag ^ 2)
            GoertzelRealImag(intSamples, (intPtr - intSampPerSymbol + 2), intSampPerSymbol, dblBinToSearch, dblReal, dblImag)
            dblMagLate = (dblReal ^ 2) + (dblImag ^ 2)
            If dblMagEarly > dblMag And dblMagEarly > dblMagLate Then
                intPtr -= 1
                If MCB.AccumulateStats Then stcTuningStats.intAccumFSKTracking -= 1
            ElseIf dblMagLate > dblMag And dblMagLate > dblMagEarly Then
                intPtr += 1
                If MCB.AccumulateStats Then stcTuningStats.intAccumFSKTracking += 1
            End If
        Catch
            ' should handle any pointer boundary issues with no corrections
        End Try
    End Sub '  Track1Car4FSK

*/

// Function to demodulate one carrier for all low baud rate 4FSK frame types
 
BOOL Demod1Car4FSK(int intPtr, short * intSamples, int intBaud, int intCenterFreq, int intNumOfSymbols, UCHAR * bytData, int * intToneMags)
{
	// Converts intSamples to an array of bytes demodulating the 4FSK symbols with center freq intCenterFreq
	// intPtr should be pointing to the approximate start of the first data symbol  
	// Updates bytData() with demodulated bytes
	// Updates bytMinSymQuality with the minimum (range is 25 to 100) symbol making up each byte.

	int intSampPerSymbol = 12000 / intBaud;
	float dblReal, dblImag;
	float dblSearchFreq;
	float dblMagSum = 0;
	float  dblMag[4];	// The magnitude for each of the 4FSK frequency bins
	UCHAR bytSym;
	static UCHAR bytSymHistory[3];
	int i, j, datalen;

	if (intFilteredMixedSamplesLength - intPtr < (intSampPerSymbol * intNumOfSymbols))
	{
		//Logs.Exception("[DemodulateDecode.Demod1Car4FSK] Insufficient Samples for requested " & intNumOfSymbols.ToString & " symbols.")
		return FALSE;
	}

	if ((intNumOfSymbols & 3) != 0)
	{
		//  Logs.Exception("[DemodulateDecode.Demod1Car4FSK] Number of Symbols not a multiple of 4: " & intNumOfSymbols.ToString & " symbols.")

		return FALSE;
	}
	
	// If MCB.AccumulateStats Then stcTuningStats.intFSKSymbolCnt += intNumOfSymbols
    
	//	ReDim intToneMags(4 * intNumOfSymbols - 1)
    //    ReDim bytData(intNumOfSymbols \ 4 - 1)

	datalen = intNumOfSymbols /4;

	for (i = 0; i < datalen; i++)	//  For each data byte
	{

		dblSearchFreq = intCenterFreq + (1.5 * intBaud);	// the highest freq (equiv to lowest sent freq because of sideband reversal)

		for (j = 0; j < 4; j++)		// for each 4FSK symbol (2 bits) in a byte
		{
			dblMagSum = 0;
			GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, dblSearchFreq / intBaud, &dblReal, &dblImag);
			dblMag[0] = pow(dblReal,2) + pow(dblImag, 2);
			dblMagSum += dblMag[0];

            GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (dblSearchFreq - intBaud) / intBaud, &dblReal, &dblImag);
			dblMag[1] = pow(dblReal,2) + pow(dblImag, 2);
			dblMagSum += dblMag[1];

			GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (dblSearchFreq - 2 * intBaud) / intBaud, &dblReal, &dblImag);
			dblMag[2] = pow(dblReal,2) + pow(dblImag, 2);
			dblMagSum += dblMag[2];
			
			GoertzelRealImag(intSamples, intPtr, intSampPerSymbol, (dblSearchFreq - 3 * intBaud) / intBaud, &dblReal,& dblImag);
			dblMag[3] = pow(dblReal,2) + pow(dblImag, 2);
			dblMagSum += dblMag[3];

			if (dblMag[0] > dblMag[1] && dblMag[0] > dblMag[2] && dblMag[0] > dblMag[3])
				bytSym = 0;
			else if (dblMag[1] > dblMag[0] && dblMag[1] > dblMag[2] && dblMag[1] > dblMag[3])
				bytSym = 1;
			else if (dblMag[2] > dblMag[0] && dblMag[2] > dblMag[1] && dblMag[2] > dblMag[3])
				bytSym = 2;
			else
				bytSym = 3;

		    bytData[i] = (bytData[i] << 2) + bytSym;

			intToneMags[16 * i + 4 * j] = dblMag[0];
			intToneMags[16 * i + 4 * j + 1] = dblMag[1];
			intToneMags[16 * i + 4 * j + 2] = dblMag[2];
			intToneMags[16 * i + 4 * j + 3] = dblMag[3];
			bytSymHistory[0] = bytSymHistory[1];
			bytSymHistory[1] = bytSymHistory[2];
			bytSymHistory[2] = bytSym;

			if ((bytSymHistory[0] != bytSymHistory[1]) && (bytSymHistory[1] != bytSymHistory[2]))
			{
				// only track when adjacent symbols are different (statistically about 56% of the time) 
				// this should allow tracking over 2000 ppm sampling rate error	
			//	Track1Car4FSK(intSamples, intPtr, intSampPerSymbol, intSearchFreq, intBaud, bytSymHistory);
			}
			intPtr += intSampPerSymbol; // advance the pointer one symbol
		}
	}
	
	return TRUE;
}


//	Function to Demod and Decode 1 carrier 4FSK 50 baud ID frame  

BOOL DemodDecode4FSKID(UCHAR bytFrameType, char * strCallID, char * strGridSquare)
{
	BOOL blnOdd;
	int	intNumCar, intBaud, intDataLen, intRSLen, intCenterFreq, intNumSymbolsPerCar;

	char strType[16] = "";
	char strMod[16] = "";

	UCHAR bytQualThresh;
	int intToneMags[1024];
	UCHAR bytRawData[64];
	UCHAR bytCall[6];
	BOOL blnRSOK = FALSE;
	UCHAR bytDecodedRS[64];
	UCHAR bytCheck[64];
	
	//'Look up the details by Frame type

	FrameInfo(bytFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytQualThresh, strType);

	intNumSymbolsPerCar = (intDataLen + intRSLen) * 4; //  Compresssed Call + Compressed GS + 2 byte RS Parity 
	intCenterFreq = 1500;

	Demod1Car4FSK(intMFSReadPtr, intFilteredMixedSamples, intBaud, intCenterFreq, intNumSymbolsPerCar, bytRawData, intToneMags);

	//Update4FSKConstellation(intToneMags, intLastRcvdFrameQuality)

	MaxCorrections = 1;


	RSDecode(bytRawData, 14, bytDecodedRS, &blnRSOK);

	memcpy(bytCall, bytDecodedRS, 6);
	DeCompressCallsign(bytCall, strCallID);
	memcpy(bytCall, &bytDecodedRS[6], 6);
	DeCompressGridSquare(bytCall, strGridSquare);
	/*
        If strGridSquare.Length = 6 Then
            strGridSquare = "[" & strGridSquare.Substring(0, 4).ToUpper & strGridSquare.Substring(4, 2).ToLower & "]"
        ElseIf strGridSquare.Length = 8 Then
            strGridSquare = "[" & strGridSquare.Substring(0, 4).ToUpper & strGridSquare.Substring(4, 2).ToLower & strGridSquare.Substring(6, 2) & "]"
        Else
            strGridSquare = "[" & strGridSquare.ToUpper & "]"
        End If
		*/

        ///strRcvFrameTag = "_" & strCallID & " " & strGridSquare

	// Double check decode by reencoing
	
	RSEncode(bytDecodedRS, bytCheck, MaxCorrections, 12);
        
	if ((bytCheck[12] == bytRawData[12]) || (bytCheck[13] == bytRawData[13]) && blnRSOK)
	{
		//If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            
		//intTestFrameCorrectCnt += 1;
		return TRUE;
	}
	//   If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
	return FALSE;
}
 

 
//  Function to Decode Frames based on frame type

BOOL DecodeFrame(int intFrameType, char * bytData)
{
	UCHAR bytFrameData[2000] = "";
	BOOL blnDecodeOK;
 //       Dim stcStatus As Status = Nothing
	char strCallerCallsign[10] = "";
	char strTargetCallsign[10] = "";
	int intRcvdQuality;
	short intPhase[10];
	short intMag[10];
	char strIDCallSign[10] = "";
	char strGridSQ[10] = "";

	int intBW;
	int intTiming;
	int intConstellationQuality = 0;

 //       ReDim bytData(-1)

	strRcvFrameTag[0] = 0;
	//stcStatus.ControlName = "lblRcvFrame"

/*
	'DataACK/NAK and short control frames 
        If intFrameType >= 0 And intFrameType <= &H1F Or intFrameType >= &HE0 Then ' DataACK/NAK
            blnDecodeOK = DecodeACKNAK(intFrameType, intRcvdQuality)
            stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag
        ElseIf (objFrameInfo.IsShortControlFrame(intFrameType)) Then ' Short Control Frames
            blnDecodeOK = True
            stcStatus.Text = objFrameInfo.Name(intFrameType)
        End If

        ' Special Frames
 */
	switch (intFrameType)
	{
		case 0x39:
		case 0x3A:
		case 0x3B:
		case 0x3C:		 // Connect ACKs with Timing
 
//			blnDecodeOK = DemodDecode4FSKConACK(intFrameType, intTiming)
// /               If blnDecodeOK Then ReDim bytData(0) : bytData(0) = CByte(intTiming / 10)
//                stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag

		break;
		
		case 0x30:		 // ID Frame
						
			blnDecodeOK = DemodDecode4FSKID(0x30, strIDCallSign, strGridSQ);
			
			sprintf(bytData, "ID:%s %s : " , strIDCallSign, strGridSQ);
			
			//stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag

			break;
/*
            Case &H31, &H32, &H33, &H34, &H35, &H36, &H37, &H38    ' Connect Request
                blnDecodeOK = DemodDecode4FSKConReq(intFrameType, intBW, strCallerCallsign, strTargetCallsign)
                stcStatus.Text = objFrameInfo.Name(intFrameType) & " " & strCallerCallsign & ">" & strTargetCallsign
                If blnDecodeOK Then bytData = GetBytes(strCallerCallsign & " " & strTargetCallsign)

                ' 1 Carrier Data frames
                '  PSK Data
            Case &H40, &H41, &H42, &H43, &H44, &H45 ' OK
                blnDecodeOK = DemodPSK(intFrameType, intPhase, intMag, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If blnDecodeOK Then
                    blnDecodeOK = DecodePSKData(intFrameType, intPhase, intMag, bytData)
                End If

                ' 4FSK Data
            Case &H46, &H47, &H48, &H49, &H4A, &H4B, &H4C, &H4D
                blnDecodeOK = DemodDecode4FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                ' 8FSK Data
            Case &H4E, &H4F
                blnDecodeOK = Decode8FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                ' 4FSK Data (600 bd)
            Case &H7A, &H7B, &H7C, &H7D
                blnDecodeOK = Decode4FSK600bdData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                ' 16FSK Data
            Case &H58, &H59, &H5A, &H5B
                blnDecodeOK = Decode16FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)

                ' 2 Carrier Data frames
            Case &H50, &H51, &H52, &H53, &H54, &H55, &H56, &H57
                blnDecodeOK = DemodPSK(intFrameType, intPhase, intMag, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If blnDecodeOK Then
                    blnDecodeOK = DecodePSKData(intFrameType, intPhase, intMag, bytData)
                End If

                ' 1000 Hz  Data frames
            Case &H60, &H61, &H62, &H63, &H64, &H65, &H66, &H67
                blnDecodeOK = DemodPSK(intFrameType, intPhase, intMag, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If blnDecodeOK Then
                    blnDecodeOK = DecodePSKData(intFrameType, intPhase, intMag, bytData)
                End If

            Case &H68, &H69
                blnDecodeOK = DemodDecode4FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If MCB.DebugLog Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode " & blnDecodeOK.ToString & "  Quality= " & intLastRcvdFrameQuality.ToString)

                ' 2000 Hz Data frames
            Case &H70, &H71, &H72, &H73, &H74, &H75, &H76, &H77
                blnDecodeOK = DemodPSK(intFrameType, intPhase, intMag, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
                If blnDecodeOK Then
                    blnDecodeOK = DecodePSKData(intFrameType, intPhase, intMag, bytData)
                End If
            Case &H78, &H79
                blnDecodeOK = DemodDecode4FSKData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)
            Case &H7A, &H7B, &H7C, &H7D
                blnDecodeOK = Decode4FSK600bdData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)

                ' Experimental Sounding frame
            Case &HD0
                DemodSounder(intMFSReadPtr, intFilteredMixedSamples)
                blnDecodeOK = True
  */
	}
	
//	if (blnDecodeOK)
//		stcStatus.BackColor = Color.LightGreen;
//	else
//		stcStatus.BackColor = Color.LightSalmon;
   
//	queTNCStatus.Enqueue(stcStatus);

	if (intConstellationQuality > 0)
	{
		intLastRcvdFrameQuality = intConstellationQuality;

        //    If MCB.DebugLog And Not blnDecodeOK Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode FAIL,   Constellation Quality= " & intLastRcvdFrameQuality.ToString)
        //    If MCB.DebugLog And blnDecodeOK Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode PASS,   Constellation Quality= " & intLastRcvdFrameQuality.ToString)
	}
	else
	{
        //    If MCB.DebugLog And Not blnDecodeOK Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode FAIL")
        //    If MCB.DebugLog And blnDecodeOK Then Logs.WriteDebug("[DecodeFrame] Frame: " & objFrameInfo.Name(intFrameType) & " Decode PASS")
	}

	return blnDecodeOK;
}





