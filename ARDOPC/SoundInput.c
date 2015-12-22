//	ARDOP Modem Decode Sound Samples

#include "ARDOPC.h"


//#define max(x, y) ((x) > (y) ? (x) : (y))
//#define min(x, y) ((x) < (y) ? (x) : (y))

char strRcvFrameTag[16];

BOOL blnLeaderFound = FALSE;
BOOL blnLeaderDetected;

int intLeaderRcvdMs = 1000;		// Leader length??
short intPSKRefPhase;			// PSK Reference symbol in milliradians) (+/- 3142)

extern int intLastRcvdFrameQuality;
extern int intReceivedLeaderLen;
extern UCHAR bytLastDataFrameType;
extern int NErrors;

short intPriorMixedSamples[120];  // a buffer of 120 samples to hold the prior samples used in the filter
int	intPriorMixedSamplesLength = 120;  // size of Prior sample buffer

short intFilteredMixedSamples[3600];	// Get Frame Type need 2400 and we may add 1200
int intFilteredMixedSamplesLength = 0;

int intFrameType;				// Type we are decoding

char strDecodeCapture[256];

//	Frame type parameters

#define MAX_DATA_LENGTH	8 * 159 // I think! 8PSK.2000.167

int intCenterFreq = 1500;
int intCarFreq;				// Are these the same ??
int intNumCar;
int intBaud;
int intDataLen;
int intRSLen;
int intSampleLen;
int intDataPtr;
int intSampPerSym;
int intDataBytesPerCar;
BOOL blnOdd;
char strType[16] = "";
char strMod[16] = "";
UCHAR bytMinQualThresh;
int intPSKMode;

#define MAX_RAW_LENGTH	256     // I think! Max length of an RS block
#define MAX_DATA_LENGTH	8 * 159 // I think! 8PSK.2000.167

int intToneMags[16 * MAX_RAW_LENGTH] = {0};	// Do we need one per carrier?

int intToneMagsLength;

short intPhases[8][8] = {0};	// We will decode as soon as we have 4 or 8 depending on mode
								//	(but need one set per carrier)

short intMags[8][8];

//	If we so Mem ARQ we will need a fair amount of RAM

int intPhasesLen;

// Received Frame

UCHAR bytData[MAX_DATA_LENGTH];
int frameLen;

int totalRSErrors;


// We need one raw buffer per carrier

// This can be optimized quite a bit to save space
// We can probably overlay on bytData

UCHAR bytFrameData1[760];					// Received chars
UCHAR bytFrameData2[MAX_RAW_LENGTH];		// Received chars
UCHAR bytFrameData3[MAX_RAW_LENGTH];		// Received chars
UCHAR bytFrameData4[MAX_RAW_LENGTH];		// Received chars
UCHAR bytFrameData5[MAX_RAW_LENGTH];		// Received chars
UCHAR bytFrameData6[MAX_RAW_LENGTH];		// Received chars
UCHAR bytFrameData7[MAX_RAW_LENGTH];		// Received chars
UCHAR bytFrameData8[MAX_RAW_LENGTH];		// Received chars

char CarrierOk[8];	// RS OK Flags per carrier
int CarriersOK;

int charIndex = 0;				// Index into received chars

int SymbolsLeft;				// number still to decode

BOOL PSKInitDone = FALSE;


BOOL blnSymbolSyncFound, blnFrameSyncFound;

extern UCHAR bytLastARQSessionID;


// dont think I need it short intRcvdSamples[12000];		// 1 second. May need to optimise

float dblOffsetLastGoodDecode = 0;
int dttLastGoodFrameTypeDecode = 0;

float dblOffsetHz = 0;;
int dttLastLeaderDetect;

extern int intRmtLeaderMeasure;

extern BOOL blnARQConnected;


extern BOOL blnPending;
extern UCHAR bytPendingSessionID;
extern UCHAR bytSessionID;

int dttLastGoodFrameTypeDecod;
int dttStartRmtLeaderMeasure;


int GotBitSyncTicks;

int intARQRTmeasuredMs;

float dbl2Pi = 2 * M_PI; 

float dblSNdBPwr, dblSNdBPwr_1, dblSNdBPwr_2;
float dblNCOFreq = 3000;	 // nominal NC) frequency
float dblNCOPhase = 0;
float dblNCOPhaseInc = 2 * M_PI * 3000 / 12000;  // was dblNCOFreq

int	intMFSReadPtr = 30;				// reset the MFSReadPtr offset 30 to accomodate the filter delay

int RcvdSamplesLen = 0;				// Samples in RX buffer

BOOL Acquire2ToneLeaderSymbolFraming();
BOOL SearchFor2ToneLeader(short * intNewSamples, int Length, float * dblOffsetHz);
BOOL AcquireFrameSyncRSB();
int Acquire4FSKFrameType();

void DemodulateFrame(int intFrameType);
void Demod1Car4FSKChar(int Start, char * Decoded);
VOID Track1Car4FSK(short * intSamples, int * intPtr, int intSampPerSymbol, float intSearchFreq, int intBaud, UCHAR * bytSymHistory);
VOID Decode1CarPSKChar(UCHAR * Decoded, int Carrier);
int EnvelopeCorrelator();
BOOL DecodeFrame(int intFrameType, char * bytData);

int Update4FSKConstellation(int * intToneMags, int * intQuality);
void Update16FSKConstellation(int * intToneMags, int * intQuality);
void Update8FSKConstellation(int * intToneMags, int * intQuality);

BOOL DemodPSK();

/*

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

	*/

// Function to determine if a valide frame type

extern UCHAR isValidFrame[256];

BOOL IsValidFrameType(UCHAR bytType)
{
	//  used in the minimum distance decoder (update if frames added or removed)

	return (isValidFrame[bytType]);
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
	const char * String = Name(intFrameType);

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
	intMFSReadPtr = 0;
}

//  Subroutine to Initialize mixed samples

void InitializeMixedSamples()
{
	// Measure the time from release of PTT to leader detection of reply.

	intARQRTmeasuredMs = min(10000, Now - dttStartRTMeasure); //?????? needs work
	intPriorMixedSamplesLength = 120;  // zero out prior samples in Prior sample buffer
	intFilteredMixedSamplesLength = 0;	// zero out the FilteredMixedSamples array
	intMFSReadPtr = 30;				// reset the MFSReadPtr offset 30 to accomodate the filter delay
}

//	Subroutine to discard all sampled prior to current intRcvdSamplesRPtr

void DiscardOldSamples()
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

static float dblZin_1 = 0, dblZin_2 = 0, dblZComb= 0;  // Used in the comb generator

	// The resonators 
      
static float dblZout_0[27] = {0.0f};	// resonator outputs
static float dblZout_1[27] = {0.0f};	// resonator outputs delayed one sample
static float dblZout_2[27] = {0.0f};	// resonator outputs delayed two samples
static float dblCoef[27] = {0.0};		// the coefficients
static float dblR = 0.9995f;			// insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
static int intN = 120;				//Length of filter 12000/100


void FSMixFilter2000Hz(short * intMixedSamples, int intMixedSamplesLength)
{
	// assumes sample rate of 12000
	// implements  23 100 Hz wide sections   (~2000 Hz wide @ - 30dB centered on 1500 Hz)

	// FSF (Frequency Selective Filter) variables

	// This works on intMixedSamples, len intMixedSamplesLength;

	// Filtered data is appended to intFilteredMixedSamples

	static float dblRn;
	static float dblR2;

	static float dblZin = 0;
      
	int i, j;

	float intFilteredSample = 0;			//  Filtered sample

	dblRn = powf(dblR, intN);

	dblR2 = powf(dblR, 2);

	// Initialize the coefficients
    
	if (dblCoef[26] == 0)
	{
		for (i = 4; i <= 26; i++)
		{
			dblCoef[i] = 2 * dblR * cosf(2 * M_PI * i / intN);  // For Frequency = bin i
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
				intFilteredSample += 0.389f * dblZout_0[j];
			else if ((j & 1) == 0)
				intFilteredSample += dblZout_0[j];
			else
				intFilteredSample -= dblZout_0[j];
		}
		intFilteredMixedSamples[intFilteredMixedSamplesLength++] = (int)ceil(intFilteredSample * 0.00833333333f);  // rescales for gain of filter
	}
	
	// update the prior intPriorMixedSamples array for the next filter call 
   
	memmove(intPriorMixedSamples, &intMixedSamples[intMixedSamplesLength - intN], intPriorMixedSamplesLength * 2);		 
}

//	Function to apply 150Hz filter used in Envelope correlator

void Filter150Hz(short * intFilterOut)
{
	// assumes sample rate of 12000
	// implements  3 100 Hz wide sections   (~150 Hz wide @ - 30dB centered on 1500 Hz)

	// FSF (Frequency Selective Filter) variables

	static float dblR = 0.9995f;		// insures stability (must be < 1.0) (Value .9995 7/8/2013 gives good results)
	static int intN = 120;				//Length of filter 12000/100
	static float dblRn;
	static float dblR2;
	static float dblCoef[17] = {0.0};			// the coefficients
	float dblZin = 0, dblZin_1 = 0, dblZin_2 = 0, dblZComb= 0;  // Used in the comb generator
	// The resonators 
      
	float dblZout_0[17] = {0.0};	// resonator outputs
	float dblZout_1[17] = {0.0};	// resonator outputs delayed one sample
	float dblZout_2[17] = {0.0};	// resonator outputs delayed two samples

	int i, j;

	float FilterOut = 0;			//  Filtered sample
	float largest = 0;

	dblRn = powf(dblR, intN);

	dblR2 = powf(dblR, 2);

	// Initialize the coefficients
    
	if (dblCoef[17] == 0)
	{
		for (i = 14; i <= 16; i++)
		{
			dblCoef[i] = 2 * dblR * cosf(2 * M_PI * i / intN);  // For Frequency = bin i
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
				FilterOut = 0.2f * dblZout_0[j];	 // this transisiton minimizes ringing and peaks
			else
				FilterOut -= dblZout_0[j];
		}
		intFilterOut[i] = (int)ceil(FilterOut * 0.00833333333);	 // rescales for gain of filter
	}

}
// Subroutine to Mix new samples with NCO to tune to nominal 1500 Hz center with reversed sideband and filter. 

void MixNCOFilter(short * intNewSamples, int Length, float dblOffsetHz)
{
	// Correct the dimension of intPriorMixedSamples if needed (should only happen after a bandwidth setting change). 

	int i;
	short intMixedSamples[1200];	// All we need at once ( I hope!)		// may need to be int
	int	intMixedSamplesLength ;		//size of intMixedSamples

	if (Length == 0)
		return;

	// Nominal NCO freq is 3000 Hz  to downmix intNewSamples  (NCO - Fnew) to center of 1500 Hz (invertes the sideband too) 

	dblNCOFreq = 3000 + dblOffsetHz;
	dblNCOPhaseInc = dblNCOFreq * dbl2Pi / 12000;

	intMixedSamplesLength = Length;

	for (i = 0; i < Length; i++)
	{
		intMixedSamples[i] = (int)ceilf(intNewSamples[i] * cosf(dblNCOPhase));  // later may want a lower "cost" implementation of "Cos"
		dblNCOPhase += dblNCOPhaseInc;
		if (dblNCOPhase > dbl2Pi)
			dblNCOPhase -= dbl2Pi;
	}

	
	
	// showed no significant difference if the 2000 Hz filer used for all bandwidths.
//	printtick("Start Filter");
	FSMixFilter2000Hz(intMixedSamples, intMixedSamplesLength);   // filter through the FS filter (required to reject image from Local oscillator)
//	printtick("Done Filter");

	// save for analysys

//	WriteSamples(&intFilteredMixedSamples[oldlen], Length);
//	WriteSamples(intMixedSamples, Length);

}

//	Function to Correct Raw demodulated data with Reed Solomon FEC 

int CorrectRawDataWithRS(UCHAR * bytRawData, UCHAR * bytCorrectedData, int intDataLen, int intRSLen, int bytFrameType, UCHAR * OK)
{
	BOOL blnRSOK;
	BOOL FrameOK;

	//Dim bytNoRS(1 + intDataLen + 2 - 1) As Byte  ' 1 byte byte Count, Data, 2 byte CRC 
	//Array.Copy(bytRawData, 0, bytNoRS, 0, bytNoRS.Length)

	if (CheckCRC16FrameType(bytRawData, intDataLen + 1, bytFrameType)) // No RS correction needed
	{
		// return the actual data
		
		memcpy(bytCorrectedData, &bytRawData[1], bytRawData[0]);    
		Debugprintf("[DemodDecode4FSKData] OK without RS");
		*OK = TRUE;
		return bytRawData[0];
	}
	
	// Try correcting with RS Parity

	
	FrameOK = RSDecode(bytRawData, intDataLen + 3 + intRSLen, intRSLen, &blnRSOK);

	if (blnRSOK)
		Debugprintf("RS Says blnRSOK");

	if (FrameOK)
		Debugprintf("RS Says FrameOK");
	else
	{
		Debugprintf("RS Says Can't Correct");
		goto returnBad;
	}

    if (FrameOK &&  CheckCRC16FrameType(bytRawData, intDataLen + 1, bytFrameType)) // RS correction successful 
	{
		int intFailedByteCnt = 0;
		
		// need to fix this if we want to use it
		//  test code just to determine how many corrections were applied  ...later remove
        //for (j = 0 ; j < intDataLen + 3; j++)
		//{
		//	if (bytRawData[j] <> bytCorrectedData[j])
		//		intFailedByteCnt++;
		//}

        Debugprintf("[DemodDecode4FSKData] OK with RS %d corrections", NErrors);
		totalRSErrors += NErrors;
 
		// End of test code

		memcpy(bytCorrectedData, &bytRawData[1], bytRawData[0]);  
		*OK = TRUE;
		return bytRawData[0];
	}
	else
        Debugprintf("[DemodDecode4FSKData] RS says ok but CRC still bad");
	
	// return uncorrected data without byte count or RS Parity

returnBad:

	memcpy(bytCorrectedData, &bytRawData[1], intDataLen);    

	//Array.Copy(bytRawData, 1, bytCorrectedData, 0, bytCorrectedData.Length) 
     
	*OK = FALSE;
	return intDataLen;
}






// Subroutine to process new samples as received from the sound card via Main.ProcessCapturedData
// Only called when not transmitting

double dblPhaseInc;  // in milliradians
short intNforGoertzel[8];
short intPSKPhase_1[8], intPSKPhase_0[8];
int intPCThresh = 194;  // (about 22 degrees... should work for 4PSK or 8PSK)
short intCP[8];	  // Cyclic prefix offset 
float dblFreqBin[8];


void ProcessNewSamples(short * Samples, int nSamples)
{
	int intRcvdSamplesWPtr = 0;

	char Msg[80];

 //       Dim stcStatus As Status = Nothing
	BOOL blnFrameDecodedOK = FALSE;

	if (0)		// Experimental UZ7HO FSK Detect
	{
		int Used;
		int Start = 0, i;

		dblPhaseInc = 2 * M_PI * 1000 / 4;
		intSampPerSym = 120;
		intCarFreq = 1800; // start at the highest carrier freq which is actually the lowest transmitted carrier due to Reverse sideband mixing
		intPhasesLen = 0;

		for (i= 0; i < 4; i++)
		{
			intCP[i] = 28; // This value selected for best decoding percentage (56%) and best Averag 4PSK Quality (77) on mpg +5 dB
			intNforGoertzel[i] = 60;
			dblFreqBin[i] = intCarFreq / 200;
			intCarFreq -= 200;
		}
	


		MixNCOFilter(Samples, nSamples, dblOffsetHz); // Mix and filter new samples (Mixing consumes all intRcvdSamples)
		Start = 0;

		// If this is a multicarrier mode, we must call the
		// decode char routing for each carrier

		while (1)
		{
			intCarFreq = 1800;
		
			Demod1CarPSKChar(Start, 0);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 
	
			Demod1CarPSKChar(Start, 1);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Demod1CarPSKChar(Start, 2);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Used = Demod1CarPSKChar(Start, 3);


		if (intPhasesLen ==	8)
		{
	//		CorrectPhaseForTuningOffset(intPhases, intPhasesLen, strMod);

			return ;
			Decode1CarPSKChar(bytFrameData1, 0);
			Decode1CarPSKChar(bytFrameData2, 1);
			Decode1CarPSKChar(bytFrameData3, 2);
			Decode1CarPSKChar(bytFrameData4, 3);
	
		}
		
		Start += Used; //intSampPerSym * 4;	
		intFilteredMixedSamplesLength -= Used; //intSampPerSym * 4;
		}
		return;
	}	// en d of UZ7HO
	
	if (State == SearchingForLeader)
		UpdateBusyDetector(Samples);

	if (ProtocolState == FECSend)
		return;

	// it seems that searchforleader runs on unmixed and unfilered samples

	// Searching for leader

	if (State == SearchingForLeader)
	{
		// Search for leader as long as 960 samples (8  symbols) available

//		Debugprintf("Looking for Leader");

		while (State == SearchingForLeader && nSamples > 960)
		{
			blnLeaderFound = SearchFor2ToneLeader(Samples, nSamples, &dblOffsetHz);
		
			if (blnLeaderFound)
			{
//				Debugprintf("Got Leader");

				dttLastLeaderDetect = Now;
                //        stcStatus.ControlName = "lblOffset"
                //        stcStatus.Text = "Offset: " & (Format(dblOffsetHz, "#0.0").PadLeft(6)) & " Hz"
                 //       queTNCStatus.Enqueue(stcStatus)

				InitializeMixedSamples();
				State = AcquireSymbolSync;
				ResetSNPwrs();
			}
			nSamples -= 480;
			Samples += 480;		// !!!! needs attention !!!
		}
		if (State == SearchingForLeader)
		{
			DiscardOldSamples();
			return;
		}
	}


	// Got leader

	//	At this point samples haven't been processed, and are in Samples, len nSamples

	// I'm going to filter all samples into intFilteredMixedSamples.

//	printtick("Start Mix");

	MixNCOFilter(Samples, nSamples, dblOffsetHz); // Mix and filter new samples (Mixing consumes all intRcvdSamples)
	nSamples = 0;	//	all used

//	printtick("Done Mix Samples");

	// Acquire Symbol Sync 

    if (State == AcquireSymbolSync)
	{
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
//			printtick("Got Sym Sync");
		}
	}
	
	//	Acquire Frame Sync
	
	if (State == AcquireFrameSync)
	{
		blnFrameSyncFound = AcquireFrameSyncRSB();
	
		if (blnFrameSyncFound)
		{
			State = AcquireFrameType;
				
			//	Have frame Sync. Remove used samples from buffer

	//		printtick("Got Frame Sync");

		}

		// Remove used samples

		intFilteredMixedSamplesLength -= intMFSReadPtr;

		memmove(intFilteredMixedSamples,
			&intFilteredMixedSamples[intMFSReadPtr], intFilteredMixedSamplesLength * 2);

		intMFSReadPtr = 0;

		if ((Now - dttLastLeaderDetect) > 1000)		 // no Frame sync within 1000 ms (may want to make this limit a funciton of Mode and leaders)
		{
			DiscardOldSamples();
			ClearAllMixedSamples();
			State = SearchingForLeader;
			printtick("frame sync timeout");
		}
	}
	
	//	Acquire Frame Type

	if (State == AcquireFrameType)
	{
//		printtick("getting frame type");

		intFrameType = Acquire4FSKFrameType();
		if (intFrameType == -2)
		{
//			sprintf(Msg, "not enough %d %d", intFilteredMixedSamplesLength, intMFSReadPtr);
//			printtick(Msg);
			return;		//  insufficient samples
		}

		if (intFrameType == -1)		  // poor decode quality (large decode distance)
		{
			State = SearchingForLeader;
			ClearAllMixedSamples();
			DiscardOldSamples();
//			printtick("poor frame type decode");

			// stcStatus.BackColor = SystemColors.Control
			// stcStatus.Text = ""
			// stcStatus.ControlName = "lblRcvFrame"
			// queTNCStatus.Enqueue(stcStatus)
		}
		else
		{
			//	Get Frame info and Initialise Demodulate variables

			// We've used intMFSReadPtr samples, so remove from Buffer

//			sprintf(Msg, "Got Frame Type %x", intFrameType);
//			printtick(Msg);

			intFilteredMixedSamplesLength -= intMFSReadPtr;
	
			memmove(intFilteredMixedSamples,
				&intFilteredMixedSamples[intMFSReadPtr], intFilteredMixedSamplesLength * 2); 

			intMFSReadPtr = 0;

			if (!FrameInfo(intFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytMinQualThresh, strType))
			{
				printtick("bad frame type");
				State = SearchingForLeader;
				ClearAllMixedSamples();
				DiscardOldSamples();
				return;
			}

			if (IsShortControlFrame(intFrameType))
			{
				// Frame has no data so is now complete

				// prepare for next

				DiscardOldSamples();
				ClearAllMixedSamples();
				State = SearchingForLeader;
				blnFrameDecodedOK = TRUE;
				Debugprintf("[DecodeFrame] Frame: %s ", Name(intFrameType));

				goto ProcessFrame;
			}

			intSampPerSym = 12000 / intBaud;
			if (IsDataFrame(intFrameType))
				SymbolsLeft = intDataLen + intRSLen + 3; // Data has a length byte
			else
				SymbolsLeft = intDataLen + intRSLen + 2;

			intToneMagsLength = (4 * intDataLen);

			charIndex = 0;	
			PSKInitDone = 0;
			
			if (!IsShortControlFrame(intFrameType))
			{
               //         stcStatus.BackColor = Color.Khaki
               //         stcStatus.Text = strType
               //         stcStatus.ControlName = "lblRcvFrame"
               //         queTNCStatus.Enqueue(stcStatus)
			}


			State = AcquireFrame;
			
			if (ProtocolMode == FEC && IsDataFrame(intFrameType) && ProtocolState != FECSend)
				SetARDOPProtocolState(FECRcv);
		}
	}
	// Acquire Frame

	if (State == AcquireFrame)
	{
		// Call DemodulateFrame for each set of samples

		DemodulateFrame(intFrameType);

		if (State == AcquireFrame)

			// We haven't got it all yet so wait for more samples	
			return;	

		//	We have the whole frame, so process it


//		printtick("got whole frame");

		frameLen = 0;

		if (strcmp (strMod, "4FSK") == 0)
			Update4FSKConstellation(intToneMags, &intLastRcvdFrameQuality);
		else if (strcmp (strMod, "16FSK") == 0)
			Update16FSKConstellation(intToneMags, &intLastRcvdFrameQuality);
		else if (strcmp (strMod, "8FSK") == 0)
			Update8FSKConstellation(intToneMags, &intLastRcvdFrameQuality);
		else
			intLastRcvdFrameQuality = UpdatePhaseConstellation(intPhases, intMags, strMod, FALSE);


		Debugprintf("Qual = %d", intLastRcvdFrameQuality);

		blnFrameDecodedOK = DecodeFrame(intFrameType, bytData);

ProcessFrame:	

		if (blnFrameDecodedOK)
		{
			if (ProtocolMode == FEC)
			{
				if (IsDataFrame(intFrameType))	// ' check to see if a data frame
					ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK);
				else if (intFrameType == 0x30)
					AddTagToDataAndSendToHost(bytData, "IDF", frameLen);
				else if (intFrameType >= 0x31 && intFrameType <= 0x38)
					ProcessUnconnectedConReqFrame(intFrameType, bytData);
			}				
			else if (ProtocolMode == ARQ)
			{
				if (!blnTimeoutTriggered)
					ProcessRcvdARQFrame(intFrameType, bytData, frameLen, blnFrameDecodedOK);  // Process connected ARQ frames here 

				if (ProtocolState == DISC)		  // allows ARQ mode to operate like FEC when not connected
					if (intFrameType == 0x30)				
						AddTagToDataAndSendToHost(bytData, "IDF", frameLen);			
					else if (intFrameType >= 0x31 && intFrameType <= 0x38)
						ProcessUnconnectedConReqFrame(intFrameType, bytData);			
					else if (IsDataFrame(intFrameType)) // check to see if a data frame
						ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK);			
			}
			else
			{
				// Unknown Mode
				bytData[frameLen] = 0;
				Debugprintf("Received Data, No State %s", bytData);
			}
		}
		else
		{
			//	Bad decode

			if (ProtocolMode == FEC)
			{
				if (IsDataFrame(intFrameType))	// ' check to see if a data frame
					ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK);
				else if (intFrameType == 0x30)
					AddTagToDataAndSendToHost(bytData, "ERR", frameLen);
			}				
			else if (ProtocolMode == ARQ)
			{
				if (ProtocolState == DISC)		  // allows ARQ mode to operate like FEC when not connected
				{
					if (intFrameType == 0x30)				
						AddTagToDataAndSendToHost(bytData, "ERR", frameLen);			

					else if (IsDataFrame(intFrameType))		// check to see if a data frame
						ProcessRcvdFECDataFrame(intFrameType, bytData, blnFrameDecodedOK);
				}
				if (!blnTimeoutTriggered)
					ProcessRcvdARQFrame(intFrameType, bytData, frameLen, blnFrameDecodedOK);  // Process connected ARQ frames here 
 
			}
  			if (ProtocolMode == FEC && ProtocolState != FECSend)
			{
				SetARDOPProtocolState(DISC);
				InitializeConnection();
			}
		}
			
		State = SearchingForLeader;
		ClearAllMixedSamples();
		DiscardOldSamples();
		return;

	}
}
// Subroutine to compute Goertzel algorithm and return Real and Imag components for a single frequency bin

void GoertzelRealImag(short intRealIn[], int intPtr, int N, float m, float * dblReal, float * dblImag)
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

	float dblZ_1 = 0.0f, dblZ_2 = 0.0f, dblW = 0.0f;
	float dblCoeff = 2 * cosf(2 * M_PI * m / N);
	int i;

	for (i = 0; i <= N; i++)
	{
		if (i == N)
			dblW = dblZ_1 * dblCoeff - dblZ_2;
		else
			dblW = intRealIn[intPtr] + dblZ_1 * dblCoeff - dblZ_2;

		dblZ_2 = dblZ_1;
		dblZ_1 = dblW;
		intPtr++;
	}
	*dblReal = 2 * (dblW - cosf(2 * M_PI * m / N) * dblZ_2) / N;  // scale results by N/2
	*dblImag = 2 * (sinf(2 * M_PI * m / N) * dblZ_2) / N;  // scale results by N/2   (this sign agrees with Scope DSP phase values) 
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

	float dblDenom = powf(XkRe, 2) + powf(XkIm, 2);
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

BOOL SearchFor2ToneLeader(short * intNewSamples, int Length, float * dblOffsetHz)
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
	float dblAlpha = 0.3f;  // Works well possibly some room for optimization Changed from .5 to .3 on Rev 0.1.5.3
	float dblInterpretThreshold= 1.5f; // Good results June 6, 2014 (was .4)  ' Works well possibly some room for optimization
	float dblFilteredMaxPeak = 0;
	int intStartBin, intStopBin;
	float dblLeftCar, dblRightCar, dblBinInterpLeft, dblBinInterpRight, dblCtrR, dblCtrI, dblCtrP, dblLeftP, dblRightP;
	float dblLeftR[3], dblLeftI[3], dblRightR[3], dblRightI[3];
	int i;
	int Ptr = 0;

	if ((Length) < 960)
		return FALSE;		// insure there are at least 960 samples (8 symbols of 120 samples)

	// Compute the start and stop bins based on the tuning range Each bin is 12000/960 or 12.5 Hz/bin

	if (blnARQConnected && (Now - dttLastGoodFrameTypeDecod < 15000) || TuningRange == 0)
	{
		// Limited range tuning

		dblLeftCar = 120 - 4 + *dblOffsetHz / 12.5f;  // the nominal positions of the two tone carriers based on the last computerd dblOffsetHz
		dblRightCar = 120 + 4 + *dblOffsetHz / 12.5f;
		// Calculate 4 bins total for Noise value in S/N computation
		
		GoertzelRealImag(intNewSamples, Ptr, 960, 121 + *dblOffsetHz / 12.5f, &dblCtrR, &dblCtrI); // nominal center + 12.5 Hz
		dblCtrP = powf(dblCtrR, 2) + powf(dblCtrI, 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, 119 + *dblOffsetHz / 12.5f, &dblCtrR, &dblCtrI); //  center - 12.5 Hz
		dblCtrP += powf(dblCtrR, 2) + powf(dblCtrI, 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, 127 + *dblOffsetHz / 12.5f, &dblCtrR, &dblCtrI); // Center + 87.5 Hz
		dblCtrP += powf(dblCtrR,2) + powf(dblCtrI, 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, 113 + *dblOffsetHz / 12.5f, &dblCtrR, &dblCtrI); // center - 87.5 Hz
		dblCtrP += powf(dblCtrR,2) + powf(dblCtrI, 2);

		// Calculate one bin above and below the two nominal 2 tone positions for Quinn Spectral Peak locator
		GoertzelRealImag(intNewSamples, Ptr, 960, dblLeftCar - 1, &dblLeftR[0], &dblLeftI[0]);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblLeftCar, &dblLeftR[1], &dblLeftI[1]);
		dblLeftP = powf(dblLeftR[1], 2) + powf(dblLeftI[1],  2);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblLeftCar + 1, &dblLeftR[2], &dblLeftI[2]);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblRightCar - 1, &dblRightR[0], &dblRightI[0]);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblRightCar, &dblRightR[1], &dblRightI[1]);
		dblRightP = powf(dblRightR[1], 2) + powf(dblRightI[1], 2);
		GoertzelRealImag(intNewSamples, Ptr, 960, dblRightCar + 1, &dblRightR[2], &dblRightI[2]);
		// Calculate the total power in the two tones (use product vs sum to help reject a single carrier).
		dblPower = sqrtf(dblLeftP * dblRightP); // sqrt converts back to units of power 
		dblSNdBPwr_1 = dblSNdBPwr;
		dblSNdBPwr = 10 * logf(dblPower / (0.25f * dblCtrP)); // Power S:N
                
		if (dblSNdBPwr > 22 && dblSNdBPwr_1 > 19)
		{
			// Calculate the interpolation based on the left of the two tones

			dblBinInterpLeft = QuinnSpectralPeakLocator(dblLeftR[0], dblLeftI[0], dblLeftR[1], dblLeftI[1], dblLeftR[2], dblLeftI[2]);
			// And the right of the two tones
			dblBinInterpRight = QuinnSpectralPeakLocator(dblRightR[0], dblRightI[0], dblRightR[1], dblRightI[1], dblRightR[2], dblRightI[2]); 

			if (fabsf(dblBinInterpLeft + dblBinInterpRight) < 1.2f)
			{
				// sanity check for the interpolators

				if (dblBinInterpLeft + dblBinInterpLeft > 0)
					*dblOffsetHz = *dblOffsetHz + min((dblBinInterpLeft + dblBinInterpRight) * 6.25f, 3);  // average left and right, adjustment bounded to +/- 3Hz max
				else
					*dblOffsetHz = *dblOffsetHz + max((dblBinInterpLeft + dblBinInterpRight) * 6.25f, -3);
                    
				sprintf(strDecodeCapture, "Interp Ldr;S:N=%3.1fdB, Offset=%3.1fHz: ", dblSNdBPwr, *dblOffsetHz);

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
		
	intStartBin = ((200 - TuningRange) / 12.5f);
		//intStopBin = ((dblMag.Length - 1) - ((200 - TuningRange) / 12.5));
	intStopBin = ((56) - ((200 - TuningRange) / 12.5f));
	// Generate the Power magnitudes for up to 46 12.5 Hz bins (a function of MCB.TuningRange) 

	for (i = intStartBin; i <= intStopBin; i++)
	{
		GoertzelRealImag(intNewSamples, Ptr, 960, i + 92.0f, &dblGoertzelReal[i], &dblGoertzelImag[i]);
			dblMag[i] = powf(dblGoertzelReal[i], 2) + powf(dblGoertzelImag[i], 2); // dblMag(i) in units of power (V^2)
	}

	//Search for the bins for the max power in the two tone signal.  

	for (i = intStartBin ; i <= intStopBin - 24; i++)	// ' +/- MCB.TuningRange from nominal 
	{
		dblPower = sqrtf(dblMag[i + 8] * dblMag[i + 16]); // using the product to minimize sensitivity to one strong carrier vs the two tone
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

		dblMaxPeakSN = dblMaxPeak / (0.2f * (dblMag[intIatMaxPeak - 104] + dblMag[intIatMaxPeak - 100] +
               dblMag[intIatMaxPeak - 92] + dblMag[intIatMaxPeak - 84] + dblMag[intIatMaxPeak - 80]));
               // 'dblMaxPeakSN = dblMaxPeak / (0.3333 * (dblMag(intIatMaxPeak - 100) + dblMag(intIatMaxPeak - 92) + dblMag(intIatMaxPeak - 84)))
		dblSNdBPwr_2 = dblSNdBPwr_1;
		dblSNdBPwr_1 = dblSNdBPwr;
		dblSNdBPwr = 10 * logf(dblMaxPeakSN);
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

			*dblOffsetHz = 12.5f * (dblInterpM - 120); // compute the tuning offset in Hz using dblInterpM
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

		sprintf(strDecodeCapture, "Ldr;S:N=%3.1fdB, Offset=%3.1fHz: ", dblSNdBPwr, *dblOffsetHz);
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
		dblCarPh = atan2f(dblImag, dblReal);
		dblAbsPhErr = fabsf(dblCarPh - (ceil(dblCarPh / M_PI) * M_PI));
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

	float dblCorMax  = -1000000.0f;		//  Preset to excessive values
	float dblCorMin  = 1000000.0f;
	int intJatMax = 0, intJatMin = 0;
	float dblCorSum;
	int i,j;
	short int150HzFilered[480];

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
	}

	if (dblCorMax > fabsf(dblCorMin))
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
	dblPhaseSym1 = atan2f(dblImag, dblReal);
	intLocalPtr += 120;	// advance one symbol
	GoertzelRealImag(intFilteredMixedSamples, intLocalPtr, 120, 15, &dblReal, &dblImag); // Carrier at 1500 Hz nominal Positioning with no cyclic prefix
	dblPhaseSym2 = atan2f(dblImag, dblReal);
	intLocalPtr += 120;		// advance one symbol

	for (i = 0; i <=  intAvailableSymbols - 4; i++)
	{
		// Compute the phase of the next symbol  
	
		GoertzelRealImag(intFilteredMixedSamples, intLocalPtr, 120, 15, &dblReal, &dblImag); // Carrier at 1500 Hz nominal Positioning with no cyclic prefix
		dblPhaseSym3 = atan2f(dblImag, dblReal);
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

		if (fabsf(dblPhaseDiff12) > 0.6667f * M_PI && fabsf(dblPhaseDiff23) < 0.3333f * M_PI)  // Tighten the margin to 60 degrees
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

	intToneMagsLength = 10;

	for (i = 0; i < 10; i++)
	{
		GoertzelRealImag(intSamples, intPtr, 240, 1575 / 50.0f, &dblReal, &dblImag);
		intToneMags[4 * i] = (int)powf(dblReal, 2) + powf(dblImag, 2);
		GoertzelRealImag(intSamples, intPtr, 240, 1525 / 50.0f, &dblReal, &dblImag);
		intToneMags[1 + 4 * i] = (int)powf(dblReal, 2) + powf(dblImag, 2);
		GoertzelRealImag(intSamples, intPtr, 240, 1475 / 50.0f, &dblReal, &dblImag);
		intToneMags[2 + 4 * i] = (int)powf(dblReal, 2) + powf(dblImag, 2);
		GoertzelRealImag(intSamples, intPtr, 240, 1425 / 50.0f, &dblReal, &dblImag);
		intToneMags[3 + 4 * i] = (int)powf(dblReal, 2) + powf(dblImag, 2);
		intPtr += 240;
	}
	
	return TRUE;
}

// Function to compute the "distance" from a specific bytFrame Xored by bytID using 1 symbol parity 

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

		dblDistance += 1.0f - ((1.0f * intToneMags[intTonePtr + (4 * j) + intToneIndex]) / (1.0f * int4ToneSum));
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

		if (blnPending)
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

//	Debugprintf("%x %x %x Sess %x pend %d conn %d", intIatMinDistance1, intIatMinDistance2,
//		intIatMinDistance3, bytSessionID, blnPending, blnARQConnected); 
	
	if (bytSessionID == 0xFF)		// ' we are in a FEC QSO, monitoring an ARQ session or have not yet reached the ARQ Pending or Connected status 
	{
		// This handles the special case of a DISC command received from the prior session (where the station sending DISC did not receive an END). 

		if (intIatMinDistance1 == 0x29 && intIatMinDistance3 == 0x29 && ((dblMinDistance1 < 0.4) || (dblMinDistance3 < 0.4)))
		{
			sprintf(strDecodeCapture, "%s MD Decode;1 ID=H%X, Type=H29: %s, D1= %.2f, D3= %.2f",
				 strDecodeCapture, bytLastARQSessionID, Name(intIatMinDistance1), dblMinDistance1, dblMinDistance3);
			
			if (DebugLog) Debugprintf("[Frame Type Decode OK  ] %s", strDecodeCapture);

			return intIatMinDistance1;
		}
		
		// no risk of damage to and existing ARQConnection with END, BREAK, DISC, or ACK frames so loosen decoding threshold 

		if (intIatMinDistance1 == intIatMinDistance2 && ((dblMinDistance1 < 0.4) || (dblMinDistance2 < 0.4)))
		{
			sprintf(strDecodeCapture, "%s MD Decode;2 ID=H%X, Type=H%X:%s, D1= %.2f, D2= %.2f",
				 strDecodeCapture, bytSessionID, intIatMinDistance1, Name(intIatMinDistance1), dblMinDistance1, dblMinDistance2);
			if (DebugLog) Debugprintf("[Frame Type Decode OK  ] %s", strDecodeCapture);
			dblOffsetLastGoodDecode = dblOffsetHz;
			dttLastGoodFrameTypeDecode = Now;

			return intIatMinDistance1;
		}
		if ((dblMinDistance1 < 0.3) && (dblMinDistance1 < dblMinDistance2) && IsDataFrame(intIatMinDistance1) )	//  this would handle the case of monitoring an ARQ connection where the SessionID is not 0xFF
		{
			sprintf(strDecodeCapture, "%s MD Decode;3 ID=H%X, Type=H%X:%s, D1= %.2f, D2= %.2f",
				 strDecodeCapture, bytSessionID, intIatMinDistance1, Name(intIatMinDistance1), dblMinDistance1, dblMinDistance2);
			if (DebugLog) Debugprintf("[Frame Type Decode OK  ] %s", strDecodeCapture);
			
			return intIatMinDistance1;
		}

		if ((dblMinDistance2 < 0.3) && (dblMinDistance2 < dblMinDistance1) && IsDataFrame(intIatMinDistance2))  // this would handle the case of monitoring an FEC transmission that failed above when the session ID is = 0xFF
		{
			sprintf(strDecodeCapture, "%s MD Decode;4 ID=H%X, Type=H%X:%s, D1= %.2f, D2= %.2f",
				 strDecodeCapture, bytSessionID, intIatMinDistance1, Name(intIatMinDistance1), dblMinDistance1, dblMinDistance2);
			if (DebugLog) Debugprintf("[Frame Type Decode OK  ] %s", strDecodeCapture);

			return intIatMinDistance2;
		}

		sprintf(strDecodeCapture, "%s MD Decode;5 Type1=H%X, Type2=H%X, D1= %.2f, D2= %.2f",
			 strDecodeCapture, intIatMinDistance1, intIatMinDistance2, dblMinDistance1, dblMinDistance2);
		if (DebugLog) Debugprintf("[Frame Type Decode Fail] %s", strDecodeCapture);

	}

	else if (blnPending)		 // We have a Pending ARQ connection 
	{
		// this should be a Con Ack from the ISS if we are Pending

		if (intIatMinDistance1 == intIatMinDistance2)  // matching indexes at minimal distances so high probablity of correct decode.
		{
			sprintf(strDecodeCapture, "%s MD Decode;6 ID=H%X, Type=H%X:%s, D1= %.2f, D2= %.2f",
				 strDecodeCapture, bytSessionID, intIatMinDistance1, Name(intIatMinDistance1), dblMinDistance1, dblMinDistance2);


			if ((dblMinDistance1 < 0.4) || (dblMinDistance2 < 0.4))
			{
				dblOffsetLastGoodDecode = dblOffsetHz;
				dttLastGoodFrameTypeDecode = Now;		// This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
				if (DebugLog) Debugprintf("[Frame Type Decode OK  ] %s", strDecodeCapture);
				return intIatMinDistance1;
			}
			else
			{
				if (DebugLog) Debugprintf("[Frame Type Decode Fail] %s", strDecodeCapture);
				return -1;		 // indicates poor quality decode so  don't use
			}
		}

		//	handles the case of a received ConReq frame based on an ID of &HFF (ISS must have missed ConAck reply from IRS so repeated ConReq)

		else if (intIatMinDistance1 == intIatMinDistance3)	 //matching indexes at minimal distances so high probablity of correct decode.
		{
			sprintf(strDecodeCapture, "%s MD Decode;7 ID=H%X, Type=H%X:%s, D1= %.2f, D3= %.2f",
				 strDecodeCapture, bytSessionID, intIatMinDistance1, Name(intIatMinDistance1), dblMinDistance1, dblMinDistance3);

			if (intIatMinDistance1 >= 0x31 && intIatMinDistance1 <= 0x38 && ((dblMinDistance1 < 0.4) || (dblMinDistance3 < 0.4)))  // Check for ConReq (ISS must have missed previous ConAck  
			{
				dblOffsetLastGoodDecode = dblOffsetHz;
				dttLastGoodFrameTypeDecode = Now;		 // This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
				if (DebugLog) Debugprintf("[Frame Type Decode OK  ] %s", strDecodeCapture);
				return intIatMinDistance1;
			}
			else
			{
				if (DebugLog) Debugprintf("[Frame Type Decode Fail] %s", strDecodeCapture);

				return -1;	 // indicates poor quality decode so  don't use
			}
		}
	}
	else if (blnARQConnected)		// ' we have an ARQ connected session.
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
				sprintf(strDecodeCapture, "%s MD Decode;8 ID=H%X, Critical Type=H%X: %s, D1= %.2f, D2= %.2f",
					 strDecodeCapture, bytSessionID, intIatMinDistance1, Name(intIatMinDistance1), dblMinDistance1, dblMinDistance2);
				if ((dblMinDistance1 < 0.25) || (dblMinDistance2 < 0.25)) // use tighter limits   here
				{
					dblOffsetLastGoodDecode = dblOffsetHz;
					dttLastGoodFrameTypeDecode = Now;		 // This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
					if (DebugLog) Debugprintf("[Frame Type Decode OK  ] %s", strDecodeCapture);
					return intIatMinDistance1;
				}
				else
				{
				if (DebugLog) Debugprintf("[Frame Type Decode Fail] %s", strDecodeCapture);
					return -1;		 // indicates poor quality decode so  don't use
				}
			}
			else	//  non critical frames
			{
				sprintf(strDecodeCapture, "%s MD Decode;9 ID=H%X, Type=H%X: %s, D1= %.2f, D2= %.2f",
					 strDecodeCapture, bytSessionID, intIatMinDistance1, Name(intIatMinDistance1), dblMinDistance1, dblMinDistance2);
				//  use looser limits here, there is no risk of protocol damage from these frames
				if ((dblMinDistance1 < 0.4) || (dblMinDistance2 < 0.4))
				{
					if (DebugLog) Debugprintf("[Frame Type Decode OK  ] %s", strDecodeCapture);
						
					dblOffsetLastGoodDecode = dblOffsetHz;
					dttLastGoodFrameTypeDecode = Now;	 // This allows restricting tuning changes to about +/- 4Hz from last dblOffsetHz
					return intIatMinDistance1;
				}
				else
				{
					if (DebugLog) Debugprintf("[Frame Type Decode Fail] %s", strDecodeCapture);
					return -1;		// indicates poor quality decode so  don't use
				}
			}
		}
		else		//non matching indexes
		{
			sprintf(strDecodeCapture, "%s MD Decode;10  Type1=H%X: Type2=H%X: , D1= %.2f, D2= %.2f",
				 strDecodeCapture, intIatMinDistance1 , intIatMinDistance2, dblMinDistance1, dblMinDistance2);
			if (DebugLog) Debugprintf("[Frame Type Decode Fail] %s", strDecodeCapture);
			return -1; // indicates poor quality decode so  don't use
		}
	}
	sprintf(strDecodeCapture, "%s MD Decode;11  Type1=H%X: Type2=H%X: , D1= %.2f, D2= %.2f",
		strDecodeCapture, intIatMinDistance1 , intIatMinDistance2, dblMinDistance1, dblMinDistance2);
	if (DebugLog) Debugprintf("[Frame Type Decode Fail] %s", strDecodeCapture);
	return -1; // indicates poor quality decode so  don't use
}


//	Function to acquire the 4FSK frame type

int Acquire4FSKFrameType()
{
	// intMFSReadPtr is pointing to start of first symbol of Frame Type (total of 10 4FSK symbols in frame type (2 bytes) + 1 parity symbol per byte 
	// returns -1 if minimal distance decoding is below threshold (low likelyhood of being correct)
	// returns -2 if insufficient samples 
	// Else returns frame type 0-255

	int NewType = 0;

	if ((intFilteredMixedSamplesLength - intMFSReadPtr) < (240 * 10))
		return -2;		//  Check for 12 available 4FSK Symbols (but only 10 are used)  

	if (!DemodFrameType4FSK(intMFSReadPtr, intFilteredMixedSamples, intToneMags))
	{
		Update4FSKConstellation(intToneMags, &intLastRcvdFrameQuality);
		intMFSReadPtr += (240 * 10);
		return -1;
	}
	
	intRmtLeaderMeasure = (Now - dttStartRmtLeaderMeasure);

	// Now do check received  Tone array for testing minimum distance decoder

	if (blnPending)			// If we have a pending connection (btween the IRS first decode of ConReq until it receives a ConAck from the iSS)  
		NewType = MinimalDistanceFrameType(intToneMags, bytPendingSessionID);		 // The pending session ID will become the session ID once connected) 
	else if (blnARQConnected)		// If we are connected then just use the stcConnection.bytSessionID
		NewType = MinimalDistanceFrameType(intToneMags, bytSessionID);
	else					// not connected and not pending so use &FF (FEC or ARQ unconnected session ID
		NewType = MinimalDistanceFrameType(intToneMags, 0xFF);
  
	if (NewType > 0x30 && NewType < 0x39)
		QueueCommandToHost("PENDING");			 // early pending notice to stop scanners

	if (NewType >= 0 &&  IsShortControlFrame(NewType))		// update the constellation if a short frame (no data to follow)
		Update4FSKConstellation(intToneMags, &intLastRcvdFrameQuality);

	//if (AccumulateStats)
	//	if (Acquire4FSKFrameType > 0)
	//		intGoodFSKFrameTypes += 1;
	//	else
	//		intFailedFSKFrameTypes += 1;
	
	intMFSReadPtr += (240 * 10);			 // advance to read pointer to the next symbol (if there is one)
	
	return NewType;
}


//	Demodulate Functions. These are called repeatedly as samples addive
//	and buld a frame in static array  bytFrameData

// Function to demodulate one carrier for all low baud rate 4FSK frame types
 
//	Is called repeatedly to decode multitone modes
int Corrections = 0;

BOOL Demod1Car4FSK()
{
	int Start = 0;
	
	// We can't wait for the full frame as we don't have enough data, so
	// we do one character at a time, until we run out or end of frame

	// Only continue if we have more than intSampPerSym * 4 chars

	while (State == AcquireFrame)
	{
		if (intFilteredMixedSamplesLength < ((intSampPerSym * 4) + 20)) // allow for correcrions
		{
			// Move any unprocessessed data down buffer

			//	(while checking process - will use cyclic buffer eventually

//			Debugprintf("Corrections %d", Corrections);

			// If corrections is non-zero, we have to adjust
			//	number left

			intFilteredMixedSamplesLength -= Corrections;

			Corrections = 0;

			if (intFilteredMixedSamplesLength > 0)
				memmove(intFilteredMixedSamples,
					&intFilteredMixedSamples[Start], intFilteredMixedSamplesLength * 2); 

			return FALSE;
		}

		// If this is a multicarrier mode, we must call the
		// decode char routing for each carrier
	
		switch (intNumCar)
		{
		case 1:

			intCenterFreq = 1500;
			Demod1Car4FSKChar(Start, bytFrameData1);
			break;

		case 2:

			intCenterFreq = 1750;
			Demod1Car4FSKChar(Start, bytFrameData1);
			intCenterFreq = 1250;
			Demod1Car4FSKChar(Start, bytFrameData2);
			break;

		case 4:

			intCenterFreq = 2250;
			Demod1Car4FSKChar(Start, bytFrameData1);
			intCenterFreq = 1750;
			Demod1Car4FSKChar(Start, bytFrameData2);
			intCenterFreq = 1250;
			Demod1Car4FSKChar(Start, bytFrameData3);
			intCenterFreq = 750;
			Demod1Car4FSKChar(Start, bytFrameData4);
			break;
		}


		charIndex++;			// Index into received chars
		SymbolsLeft--;			// number still to decode
		Start += intSampPerSym * 4;	// 4 FSK bit pairs per byte
		intFilteredMixedSamplesLength -= intSampPerSym * 4;

		if (SymbolsLeft == 0)	
		{	
			//- prepare for next

			DiscardOldSamples();
			ClearAllMixedSamples();
			State = SearchingForLeader;
		}
	}
	return TRUE;
}

// Function to demodulate one carrier for all low baud rate 4FSK frame types
 
void Demod1Car4FSKChar(int Start, char * Decoded)
{
	// Converts intSamples to an array of bytes demodulating the 4FSK symbols with center freq intCenterFreq
	// intPtr should be pointing to the approximate start of the first data symbol  
	// Updates bytData() with demodulated bytes
	// Updates bytMinSymQuality with the minimum (range is 25 to 100) symbol making up each byte.

	float dblReal, dblImag;
	float dblSearchFreq;
	float dblMagSum = 0;
	float  dblMag[4];	// The magnitude for each of the 4FSK frequency bins
	UCHAR bytSym;
	static UCHAR bytSymHistory[3];
	int j;
	UCHAR bytData = 0;
	
	// If MCB.AccumulateStats Then stcTuningStats.intFSKSymbolCnt += intNumOfSymbols
    
	//	ReDim intToneMags(4 * intNumOfSymbols - 1)
    //    ReDim bytData(intNumOfSymbols \ 4 - 1)

	dblSearchFreq = intCenterFreq + (1.5f * intBaud);	// the highest freq (equiv to lowest sent freq because of sideband reversal)

	// Do one symbol

	for (j = 0; j < 4; j++)		// for each 4FSK symbol (2 bits) in a byte
	{
		dblMagSum = 0;
		GoertzelRealImag(intFilteredMixedSamples, Start, intSampPerSym, dblSearchFreq / intBaud, &dblReal, &dblImag);
		dblMag[0] = powf(dblReal,2) + powf(dblImag, 2);
		dblMagSum += dblMag[0];

        GoertzelRealImag(intFilteredMixedSamples, Start, intSampPerSym, (dblSearchFreq - intBaud) / intBaud, &dblReal, &dblImag);
		dblMag[1] = powf(dblReal,2) + powf(dblImag, 2);
		dblMagSum += dblMag[1];

		GoertzelRealImag(intFilteredMixedSamples, Start, intSampPerSym, (dblSearchFreq - 2 * intBaud) / intBaud, &dblReal, &dblImag);
		dblMag[2] = powf(dblReal,2) + powf(dblImag, 2);
		dblMagSum += dblMag[2];
			
		GoertzelRealImag(intFilteredMixedSamples, Start, intSampPerSym, (dblSearchFreq - 3 * intBaud) / intBaud, &dblReal,& dblImag);
		dblMag[3] = powf(dblReal,2) + powf(dblImag, 2);
		dblMagSum += dblMag[3];

		if (dblMag[0] > dblMag[1] && dblMag[0] > dblMag[2] && dblMag[0] > dblMag[3])
			bytSym = 0;
		else if (dblMag[1] > dblMag[0] && dblMag[1] > dblMag[2] && dblMag[1] > dblMag[3])
			bytSym = 1;
		else if (dblMag[2] > dblMag[0] && dblMag[2] > dblMag[1] && dblMag[2] > dblMag[3])
			bytSym = 2;
		else
			bytSym = 3;

		bytData = (bytData << 2) + bytSym;

		// !!!!!!! this needs attention !!!!!!!!

		intToneMags[16 * charIndex + 4 * j] = dblMag[0];
		intToneMags[16 * charIndex + 4 * j + 1] = dblMag[1];
		intToneMags[16 * charIndex + 4 * j + 2] = dblMag[2];
		intToneMags[16 * charIndex + 4 * j + 3] = dblMag[3];
		bytSymHistory[0] = bytSymHistory[1];
		bytSymHistory[1] = bytSymHistory[2];
		bytSymHistory[2] = bytSym;

//		if ((bytSymHistory[0] != bytSymHistory[1]) && (bytSymHistory[1] != bytSymHistory[2]))
		{
			// only track when adjacent symbols are different (statistically about 56% of the time) 
			// this should allow tracking over 2000 ppm sampling rate error	
//			if (Start > intSampPerSym + 2)
//				Track1Car4FSK(intFilteredMixedSamples, &Start, intSampPerSym, dblSearchFreq, intBaud, bytSymHistory);
		}
		Start += intSampPerSym; // advance the pointer one symbol
	}

	Decoded[charIndex] = bytData;	
	return;
}

void Demod1Car8FSKChar(int Start, char * Decoded);

BOOL Demod1Car8FSK()
{
	int Start = 0;
	
	// We can't wait for the full frame as we don't have enough data, so
	// we do one character at a time, until we run out or end of frame

	// Only continue if we have more than intSampPerSym * 8 chars

	while (State == AcquireFrame)
	{
		if (intFilteredMixedSamplesLength < ((intSampPerSym * 8) + 20)) // allow for correcrions
		{
			// Move any unprocessessed data down buffer

			//	(while checking process - will use cyclic buffer eventually

//			Debugprintf("Corrections %d", Corrections);

			// If corrections is non-zero, we have to adjust
			//	number left

			intFilteredMixedSamplesLength -= Corrections;

			Corrections = 0;

			if (intFilteredMixedSamplesLength > 0)
				memmove(intFilteredMixedSamples,
					&intFilteredMixedSamples[Start], intFilteredMixedSamplesLength * 2); 

			return FALSE;
		}

		// If this is a multicarrier mode, we must call the
		// decode char routing for each carrier
	
		switch (intNumCar)
		{
		case 1:

			intCenterFreq = 1500;
			Demod1Car8FSKChar(Start, bytFrameData1);
			break;

		case 2:

			intCenterFreq = 1750;
			Demod1Car8FSKChar(Start, bytFrameData1);
			intCenterFreq = 1250;
			Demod1Car8FSKChar(Start, bytFrameData2);
			break;
		}

		SymbolsLeft -=3;			// number still to decode
		Start += intSampPerSym * 8;	// 8 FSK bit triplea 
		intFilteredMixedSamplesLength -= intSampPerSym * 8;

		if (SymbolsLeft <= 0)	
		{	
			//- prepare for next

			DiscardOldSamples();
			ClearAllMixedSamples();
			State = SearchingForLeader;
		}
	}
	return TRUE;
}

// Function to demodulate one carrier for all 8FSK frame types
 
void Demod1Car8FSKChar(int Start, char * Decoded)
{
	// Converts intSamples to an array of bytes demodulating the 4FSK symbols with center freq intCenterFreq
	// intPtr should be pointing to the approximate start of the first data symbol  
	// Updates bytData() with demodulated bytes
	// Updates bytMinSymQuality with the minimum (range is 25 to 100) symbol making up each byte.

	float dblReal, dblImag;
	float dblSearchFreq;
	float dblMagSum;
	UCHAR bytSym;
	static UCHAR bytSymHistory[3];
	int j, k;
	unsigned int intThreeBytes = 0;
	int intMaxMag;
	
	// If MCB.AccumulateStats Then stcTuningStats.intFSKSymbolCnt += intNumOfSymbols
    
	//	ReDim intToneMags(4 * intNumOfSymbols - 1)
    //    ReDim bytData(intNumOfSymbols \ 4 - 1)

	dblSearchFreq = intCenterFreq + (1.5f * intBaud);	// the highest freq (equiv to lowest sent freq because of sideband reversal)

	// Do one symbol

	for (j = 0; j < 8; j++)		// for each group of 8 symbols (24 bits) 
	{
		dblMagSum = 0;
		intMaxMag = 0;

		dblSearchFreq = intCenterFreq + 3.5f * intBaud; //' the highest freq (equiv to lowest sent freq because of sideband reversal)
			
		for (k = 0; k < 8; k++)  // for each of 8 possible tones per symbol
		{
			GoertzelRealImag(intFilteredMixedSamples, Start, intSampPerSym, (dblSearchFreq - k * intBaud) / intBaud, &dblReal, &dblImag);
 
			intToneMags[8 * j + k] = powf(dblReal, 2) + powf(dblImag, 2);
			dblMagSum += intToneMags[8 * j + k];

			if (intToneMags[8 * j + k] > intMaxMag)
			{
				intMaxMag = intToneMags[8 * j + k];
				bytSym = k;
			}
		}

		intThreeBytes = (intThreeBytes << 3) + bytSym;

		// !!!!!!! this needs attention !!!!!!!!

/*		intToneMags[16 * charIndex + 4 * j] = dblMag[0];
		intToneMags[16 * charIndex + 4 * j + 1] = dblMag[1];
		intToneMags[16 * charIndex + 4 * j + 2] = dblMag[2];
		intToneMags[16 * charIndex + 4 * j + 3] = dblMag[3];
*/
		bytSymHistory[0] = bytSymHistory[1];
		bytSymHistory[1] = bytSymHistory[2];
		bytSymHistory[2] = bytSym;

//		if ((bytSymHistory[0] != bytSymHistory[1]) && (bytSymHistory[1] != bytSymHistory[2]))
		{
			// only track when adjacent symbols are different (statistically about 56% of the time) 
			// this should allow tracking over 2000 ppm sampling rate error	
//			if (Start > intSampPerSym + 2)
//				Track1Car4FSK(intFilteredMixedSamples, &Start, intSampPerSym, dblSearchFreq, intBaud, bytSymHistory);
		}
		Start += intSampPerSym; // advance the pointer one symbol
	}

	Decoded[charIndex++] = intThreeBytes >> 16;	
	Decoded[charIndex++] = intThreeBytes >> 8;	
	Decoded[charIndex++] = intThreeBytes;	

	return;
}

//	Function to Decode 1 carrier 4FSK 50 baud Connect Request 



void Demod1Car16FSKChar(int Start, char * Decoded);

BOOL Demod1Car16FSK()
{
	int Start = 0;
	
	// We can't wait for the full frame as we don't have enough data, so
	// we do one character at a time, until we run out or end of frame

	// Only continue if we have more than intSampPerSym * 2 chars

	while (State == AcquireFrame)
	{
		if (intFilteredMixedSamplesLength < ((intSampPerSym * 2) + 20)) // allow for correcrions
		{
			// Move any unprocessessed data down buffer

			//	(while checking process - will use cyclic buffer eventually

//			Debugprintf("Corrections %d", Corrections);

			// If corrections is non-zero, we have to adjust
			//	number left

			intFilteredMixedSamplesLength -= Corrections;

			Corrections = 0;

			if (intFilteredMixedSamplesLength > 0)
				memmove(intFilteredMixedSamples,
					&intFilteredMixedSamples[Start], intFilteredMixedSamplesLength * 2); 

			return FALSE;
		}

		// If this is a multicarrier mode, we must call the
		// decode char routing for each carrier
	
		switch (intNumCar)
		{
		case 1:

			intCenterFreq = 1500;
			Demod1Car16FSKChar(Start, bytFrameData1);
			break;

		case 2:

			intCenterFreq = 1750;
			Demod1Car16FSKChar(Start, bytFrameData1);
			intCenterFreq = 1250;
			Demod1Car16FSKChar(Start, bytFrameData2);
			break;
		}

		SymbolsLeft--;			// number still to decode
		Start += intSampPerSym * 2;	// 2 FSK nibbles 
		intFilteredMixedSamplesLength -= intSampPerSym * 2;

		if (SymbolsLeft <= 0)	
		{	
			//- prepare for next

			DiscardOldSamples();
			ClearAllMixedSamples();
			State = SearchingForLeader;
		}
	}
	return TRUE;
}

// Function to demodulate one carrier for all 8FSK frame types
 
void Demod1Car16FSKChar(int Start, char * Decoded)
{
	// Converts intSamples to an array of bytes demodulating the 4FSK symbols with center freq intCenterFreq
	// intPtr should be pointing to the approximate start of the first data symbol  
	// Updates bytData() with demodulated bytes
	// Updates bytMinSymQuality with the minimum (range is 25 to 100) symbol making up each byte.

	float dblReal, dblImag;
	float dblSearchFreq;
	float dblMagSum = 0;
	UCHAR bytSym = 0;
	static UCHAR bytSymHistory[3];
	int j, k;
	UCHAR bytData = 0;
	int intMaxMag;
	
	// If MCB.AccumulateStats Then stcTuningStats.intFSKSymbolCnt += intNumOfSymbols
    
	//	ReDim intToneMags(4 * intNumOfSymbols - 1)
    //    ReDim bytData(intNumOfSymbols \ 4 - 1)

	dblSearchFreq = intCenterFreq + (7.5f * intBaud);	// the highest freq (equiv to lowest sent freq because of sideband reversal)

	// Do one symbol
 
	for (j = 0; j < 2; j++)  // for each 16FSK symbol (4 bits) in a byte
	{
		dblMagSum = 0;
		intMaxMag = 0;

		dblSearchFreq = intCenterFreq + 7.5f * intBaud; //' the highest freq (equiv to lowest sent freq because of sideband reversal)
			
		for (k = 0; k < 16; k++)  // for each of 8 possible tones per symbol
		{
			GoertzelRealImag(intFilteredMixedSamples, Start, intSampPerSym, (dblSearchFreq - k * intBaud) / intBaud, &dblReal, &dblImag);
 
			intToneMags[charIndex * 32 + 16 * j + k] = powf(dblReal, 2) + powf(dblImag, 2);
			dblMagSum += intToneMags[charIndex * 32 + 16 * j + k];

			if (intToneMags[charIndex * 32 + 16 * j + k] > intMaxMag)
			{
				intMaxMag = intToneMags[charIndex * 32 + 16 * j + k];
				bytSym = k;
			}
		}

		bytData = (bytData << 4) + bytSym;

		bytSymHistory[0] = bytSymHistory[1];
		bytSymHistory[1] = bytSymHistory[2];
		bytSymHistory[2] = bytSym;

//		if ((bytSymHistory[0] != bytSymHistory[1]) && (bytSymHistory[1] != bytSymHistory[2]))
		{
			// only track when adjacent symbols are different (statistically about 56% of the time) 
			// this should allow tracking over 2000 ppm sampling rate error	
//			if (Start > intSampPerSym + 2)
//				Track1Car4FSK(intFilteredMixedSamples, &Start, intSampPerSym, dblSearchFreq, intBaud, bytSymHistory);
		}
		Start += intSampPerSym; // advance the pointer one symbol
	}

//	Debugprintf("Tone Mags Index %d", charIndex * 32 + 16 * j + k);

	Decoded[charIndex++] = bytData;;	
	return;
}

//	Function to Decode 1 carrier 4FSK 50 baud Connect Request 



extern int intBW;

BOOL Decode4FSKConReq()
{
	UCHAR strCaller[10];
	UCHAR strTarget [10];
	UCHAR bytCall[6];
	BOOL blnRSOK;
	BOOL FrameOK;

	// Modified May 24, 2015 to use RS encoding vs CRC (similar to ID Frame)
 
	FrameOK = RSDecode(bytFrameData1, 14, 2, &blnRSOK);

	memcpy(bytCall, bytFrameData1, 6);
	DeCompressCallsign(bytCall, strCaller);
	memcpy(bytCall, &bytFrameData1[6], 6);
	DeCompressCallsign(bytCall, strTarget);

	printtick(strCaller);
	printtick(strTarget);
	
	sprintf(strRcvFrameTag, "_%s > %s", strCaller, strTarget);
	sprintf(bytData, "%s %s", strCaller, strTarget);

	// Recheck the returned data by reencoding
	
//	if (((bytCheck[12] == bytRawData[12]) || (bytCheck[13] == bytRawData[13])) && blnRSOK)
	{
		if (intFrameType == 0x31)
			intBW = 200;
		else if (intFrameType == 0x32)
			intBW = 500;
		else if (intFrameType == 0x33)
			intBW = 1000;
		else if (intFrameType == 0x34)
			intBW = 2000;
	
        //    If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
        //    intTestFrameCorrectCnt += 1

		return TRUE;
	}

	//If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1

	return FALSE;
}

// Function to Decode 1 carrier 4FSK 50 baud Connect Ack with timing 

BOOL Decode4FSKConACK(UCHAR bytFrameType, int * intTiming)
{
	int Timing = 0;

 //Dim bytCall(5) As Byte


	if (bytFrameData1[0] == bytFrameData1[1]) 
		Timing = 10 * bytFrameData1[0];
	else if (bytFrameData1[0] == bytFrameData1[2])
		Timing = 10 * bytFrameData1[0];
	else if (bytFrameData1[1] == bytFrameData1[2])
		Timing = 10 * bytFrameData1[1];

	if (Timing >= 0)
	{
		*intTiming = Timing;

		// strRcvFrameTag = "_" & intTiming.ToString & " ms"
        //    if (DebugLog) Debugprintf(("[DemodDecode4FSKConACK]  Remote leader timing reported: " & intTiming.ToString & " ms")
         //   If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
         
		//intTestFrameCorrectCnt++;
		intReceivedLeaderLen = intLeaderRcvdMs;
		bytLastDataFrameType = 0;  // initialize the LastFrameType to an illegal Data frame
        return TRUE;
	}
	
	//If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1

	return FALSE;
}

BOOL Decode4FSKID(UCHAR bytFrameType, char * strCallID, char * strGridSquare)
{
	UCHAR bytCall[6];
	BOOL blnRSOK;
	BOOL FrameOK;

	FrameOK = RSDecode(bytFrameData1, 14, 2, &blnRSOK);

	memcpy(bytCall, bytFrameData1, 6);
	DeCompressCallsign(bytCall, strCallID);
	memcpy(bytCall, &bytFrameData1[6], 6);
	DeCompressGridSquare(bytCall, strGridSquare);

	printtick(strCallID);
	printtick(strGridSquare);


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

	// Double check decode by reencoding

	//	Not sure if needed, but may be

	if (FrameOK)
	{
		if (blnRSOK)
		{
			// Good without Correction
		//if (AccumulateStats) intGoodFSKFrameDataDecodes++;
			return TRUE;
		}
		// Good after correction

		Debugprintf("ID Frame Corrected by RS");
		return TRUE;
	}

	//   if (AccumulateStats)  intFailedFSKFrameDataDecodes++;
	return FALSE;	// Not correctable
}

//	RSEncode(bytDecodedRS, bytCheck, MaxCorrections, 12);
        
//	if ((bytCheck[12] == bytFrameData[12]) || (bytCheck[13] == bytFrameData[13]) && blnRSOK)
//	{
		//If MCB.AccumulateStats Then stcTuningStats.intGoodFSKFrameDataDecodes += 1
            
		//intTestFrameCorrectCnt += 1;
//		return TRUE;
//	}
	//   If MCB.AccumulateStats Then stcTuningStats.intFailedFSKFrameDataDecodes += 1
//	return FALSE;
//}






 
//  Function to Demodulate Frame based on frame type
//	Will be called repeatedly as new samples arrive

void DemodulateFrame(int intFrameType)
{
 //       Dim stcStatus As Status = Nothing

	int intConstellationQuality = 0;

 //       ReDim bytData(-1)

	strRcvFrameTag[0] = 0;

	//stcStatus.ControlName = "lblRcvFrame"

	//	DataACK/NAK and short control frames

	if ((intFrameType >= 0 && intFrameType <= 0x1f) ||  intFrameType == 0xe0)	 // DataACK/NAK
	{
		//blnDecodeOK = DecodeACKNAK(intFrameType, intRcvdQuality)
        //    stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag
        //ElseIf (objFrameInfo.IsShortControlFrame(intFrameType)) Then ' Short Control Frames
        //    blnDecodeOK = TRUE
        //    stcStatus.Text = objFrameInfo.Name(intFrameType)
        //End If
			
		Demod1Car4FSK();
		return;
	}

	if (intFrameType >= 0x30 && intFrameType <= 0x38)
	{
		// ID and CON Req

		Demod1Car4FSK();
		return;
	}


/*
            Case 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38    ' Connect Request
                blnDecodeOK = DemodDecode4FSKConReq(intFrameType, intBW, strCallerCallsign, strTargetCallsign)
                stcStatus.Text = objFrameInfo.Name(intFrameType) & " " & strCallerCallsign & ">" & strTargetCallsign
                If blnDecodeOK Then bytData = GetBytes(strCallerCallsign & " " & strTargetCallsign)

				


        ' Special Frames
 */
	switch (intFrameType)
	{
		case 0x39:
		case 0x3A:
		case 0x3B:
		case 0x3C:		 // Connect ACKs with Timing
 
		Demod1Car4FSK();
		return;

		break;
		
	
			// 1 Carrier Data frames
			// PSK Data

					
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:

			DemodPSK();
			break;

		//4FSK Data

		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:

		case 0x68:		// 2 Carrier FSK
		case 0x69:		// 2 Carrier FSK

			Demod1Car4FSK();
			break;

		// 8FSK Data

		case 0x4E:
		case 0x4F:

			Demod1Car8FSK();
			break;

        // 4FSK Data (600 bd)

		case 0x7A:
		case 0x7B:
		case 0x7C:
		case 0x7D:

//			Demod1Car4FSK600();
			break;
  
		// 16FSK Data

		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:
			
			Demod1Car16FSK();
			break;

			 //2 Carrier PSK Data frames

		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57:

			DemodPSK();
			break;
			
		// 1000 Hz  Data frames

		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:

			DemodPSK();
			break;

			// 2000 Hz PSK 8 Carr Data frames
			
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:

			DemodPSK();
			break;

		case 0x78:		// 4 Carrier FSK
		case 0x79:		// 4 Carrier FSK

			Demod1Car4FSK();
			break;


  /*              ' Experimental Sounding frame
            Case 0xD0
                DemodSounder(intMFSReadPtr, intFilteredMixedSamples)
                blnDecodeOK = TRUE
  */
		default:

			Debugprintf("Unsupported frame type %x", intFrameType);
			DiscardOldSamples();
			ClearAllMixedSamples();
			State = SearchingForLeader;


			intFilteredMixedSamplesLength = 0;	// Testing
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
}





 



// Function to Strip quality from ACK/NAK frame types

BOOL DecodeACKNAK(int intFrameType, int *  intQuality)
{
	*intQuality = 38 + (2 * (intFrameType & 0x1F));  //mask off lower 5 bits ' Range of 38 to 100 in steps of 2
     // strRcvFrameTag = "_Q" & intQuality.ToString
	return TRUE;
}





BOOL DecodeFrame(int intFrameType, char * bytData)
{
	BOOL blnDecodeOK = FALSE;
 //       Dim stcStatus As Status = Nothing
	char strCallerCallsign[10] = "";
	char strTargetCallsign[10] = "";
	char strIDCallSign[10] = "";
	char strGridSQ[10] = "";
	int CarrierLen;
	int intTiming;
	int intRcvdQuality;

 //       ReDim bytData(-1)

	strRcvFrameTag[0] = 0;
	//stcStatus.ControlName = "lblRcvFrame"

	//DataACK/NAK and short control frames 


	if ((intFrameType >= 0 && intFrameType <= 0x1F) || intFrameType >= 0xE0) // DataACK/NAK
	{
		blnDecodeOK = DecodeACKNAK(intFrameType, &intRcvdQuality);
        //stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag

		goto returnframe;
	}
	else if (IsShortControlFrame(intFrameType)) // Short Control Frames
	{
		blnDecodeOK = TRUE;
        //    stcStatus.Text = objFrameInfo.Name(intFrameType)
		goto returnframe;
	}

	totalRSErrors = 0;

	switch (intFrameType)
	{
		case 0x39:
		case 0x3A:
		case 0x3B:
		case 0x3C:		 // Connect ACKs with Timing
 
			blnDecodeOK = Decode4FSKConACK(intFrameType, &intTiming);

			if (blnDecodeOK)
				bytData[0] = intTiming / 10;
//                stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag

		break;
		
		case 0x30:		 // ID Frame

			printtick("Decoding IDFrame");
						
			blnDecodeOK = Decode4FSKID(0x30, strIDCallSign, strGridSQ);
			
			frameLen = sprintf(bytData, "ID:%s %s : " , strIDCallSign, strGridSQ);
			
			printtick(bytData);

			//stcStatus.Text = objFrameInfo.Name(intFrameType) & strRcvFrameTag

			break;

		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:

			blnDecodeOK = Decode4FSKConReq();
			break;

			
		//   PSK 1 Carrier Data
	
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:

		// FSK 1 Carrier Modes

		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4a:
		case 0x4b:
		case 0x4c:
		case 0x4d:
		case 0x4e:
		case 0x4f:
		case 0x58:
		case 0x59:
		case 0x5A:
		case 0x5B:

			frameLen = CorrectRawDataWithRS(bytFrameData1, bytData, intDataLen, intRSLen, intFrameType, &CarrierOk[0]);
			blnDecodeOK = CarrierOk[0];

			// Memory ARQ goes here

			break;

			// 2 Carrier Data frames
			
		case 0x50:
		case 0x51:
		case 0x52:
		case 0x53:
		case 0x54:
		case 0x55:
		case 0x56:
		case 0x57:
		case 0x68:
		case 0x69:

			// 2 Carrier Modes
			
			frameLen = CorrectRawDataWithRS(bytFrameData1, bytData, intDataLen, intRSLen, intFrameType, &CarrierOk[0]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData2, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[1]);

			if (CarrierOk[0] && CarrierOk[1])
				blnDecodeOK = TRUE;

			break;


		// 2000 Hz Data frames 8 Carrier

		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:

			// 8 Carrier Modes
 
			frameLen = CorrectRawDataWithRS(bytFrameData1, bytData, intDataLen, intRSLen, intFrameType, &CarrierOk[0]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData2, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[1]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData3, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[2]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData4, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[3]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData5, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[4]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData6, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[5]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData7, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[6]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData8, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[7]);

			if (CarrierOk[0] && CarrierOk[1] && CarrierOk[2] && CarrierOk[3] 
				&& CarrierOk[4] && CarrierOk[6] && CarrierOk[6] && CarrierOk[7])
				blnDecodeOK = TRUE;

			break;
	
		case 0x78:
		case 0x79:

			// 4 Carrier FSK Modes

		case 0x60:
		case 0x61:
		case 0x62:
		case 0x63:
		case 0x64:
		case 0x65:
		case 0x66:
		case 0x67:

			//	4 carrrier PSK 1000 Hz  Data frames
 
 
			frameLen = CorrectRawDataWithRS(bytFrameData1, bytData, intDataLen, intRSLen, intFrameType, &CarrierOk[0]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData2, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[1]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData3, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[2]);
			frameLen +=  CorrectRawDataWithRS(bytFrameData4, &bytData[frameLen], intDataLen, intRSLen, intFrameType, &CarrierOk[3]);

			if (CarrierOk[0] && CarrierOk[1] && CarrierOk[2] && CarrierOk[3]) 
				blnDecodeOK = TRUE;

			break;
/*
            Case 0x7A, 0x7B, 0x7C, 0x7D
                blnDecodeOK = Decode4FSK600bdData(intFrameType, bytData, intConstellationQuality)
                stcStatus.Text = objFrameInfo.Name(intFrameType)

                ' Experimental Sounding frame
            Case 0xD0
                DemodSounder(intMFSReadPtr, intFilteredMixedSamples)
                blnDecodeOK = TRUE
  */
	}

	//	if a data frame with few error and quality very low, adjust

	if (blnDecodeOK && (totalRSErrors / intNumCar) < (intRSLen / 4) && intLastRcvdFrameQuality < 80)
	{
		Debugprintf("RS Errors %d Carriers %d RLen %d Qual %d - adjusting Qual",
			totalRSErrors, intNumCar, intRSLen, intLastRcvdFrameQuality);
		
		intLastRcvdFrameQuality = 80;
	}


returnframe:
	
//	if (blnDecodeOK)
//		stcStatus.BackColor = Color.LightGreen;
//	else
//		stcStatus.BackColor = Color.LightSalmon;
   
//	queTNCStatus.Enqueue(stcStatus);

	if (DebugLog)
		if (blnDecodeOK)
			Debugprintf("[DecodeFrame] Frame: %s Decode PASS, Constellation Quality= %d", Name(intFrameType), intLastRcvdFrameQuality);
		else
			Debugprintf("[DecodeFrame] Frame: %s Decode FAIL, Constellation Quality= %d", Name(intFrameType), intLastRcvdFrameQuality);

	return blnDecodeOK;
}

// Subroutine to update the 4FSK Constellation

int Update4FSKConstellation(int * intToneMags, int * intQuality)
{
	// Subroutine to update bmpConstellation plot for 4FSK modes...
        
	float dblRad = 0;
	int  intToneSum = 0;
	float intMagMax = 0;
	float dblPi4  = 0.25 * M_PI;
	float dblDistanceSum = 0;
	//float dblPlotRotation;
	// Dim stcStatus As Status

	//int yCenter, xCenter;
	int intRad = 0;
	// Dim clrPixel As System.Drawing.Color
	// bmpConstellation = New Bitmap(89, 89)
	int i;

	for (i = 0; i < intToneMagsLength; i += 4)  // for the number of symbols represented by intToneMags
	{
		intToneSum = intToneMags[i] + intToneMags[i + 1] + intToneMags[i + 2] + intToneMags[i + 3];
        
		if (intToneMags[i] > intToneMags[i + 1] && intToneMags[i] > intToneMags[i + 2] && intToneMags[i] > intToneMags[i + 3])
		{
			if (intToneSum > 0)
				intRad = max(5.0f, 42.0f - 80 * (intToneMags[i + 1] + intToneMags[i + 2] + intToneMags[i + 3]) / intToneSum);
		}
		else if (intToneMags[i + 1] > intToneMags[i] && intToneMags[i + 1] > intToneMags[i + 2] && intToneMags[i + 1] > intToneMags[i + 3])
		{
			if (intToneSum > 0)
				intRad = max(5.0f, 42.0f - 80 * (intToneMags[i] + intToneMags[i + 2] + intToneMags[i + 3]) / intToneSum);
		}
		else if (intToneMags[i + 2] > intToneMags[i] && intToneMags[i + 2] > intToneMags[i + 1] && intToneMags[i + 2] > intToneMags[i + 3]) 
		{
            if (intToneSum > 0)
				intRad = max(5.0f, 42.0f - 80 * (intToneMags[i + 1] + intToneMags[i] + intToneMags[i + 3]) / intToneSum);
		}
		else if (intToneSum > 0)
			intRad = max(5.0f, 42.0f - 80 * (intToneMags[i + 1] + intToneMags[i + 2] + intToneMags[i]) / intToneSum);	

		dblDistanceSum += (43 - intRad);
	}

	*intQuality = 100 - (2.7f * (dblDistanceSum / (intToneMagsLength / 4))); // ' factor 2.7 emperically chosen for calibration (Qual range 25 to 100)

	if (*intQuality < 0)
		*intQuality = 0;

	// if AccumulateStats
	//{
	//	int4FSKQualityCnts += 1;
	//	int4FSKQuality += intQuality;
	//}

	return *intQuality;
}

// Subroutine to update the 16FSK constallation

void Update16FSKConstellation(int * intToneMags, int * intQuality)
{
	//	Subroutine to update bmpConstellation plot for 16FSK modes...

	float dblRad;
	int	intToneSum = 0;
	float intMagMax = 0;
	float dblAng;
	float dblDistanceSum = 0;
	float dblPlotRotation = 0;
//            Dim stcStatus As Status
	int	yCenter, xCenter;
	int	intRad;
//            Dim clrPixel As System.Drawing.Color
	int	intJatMaxMag;
	int i, j;

	for (i = 0; i< intToneMagsLength; i += 16)  // for the number of symbols represented by intToneMags
	{
		intToneSum = 0;
		intMagMax = 0;

		for (j = 0; j < 16; j++)
		{
			if (intToneMags[i + j] > intMagMax)
			{
				intMagMax = intToneMags[i + j];
				intJatMaxMag = j;
			}
			intToneSum += intToneMags[i + j];
		}
		intRad = max(5, 42 - 40 * (intToneSum - intMagMax) / intToneSum);
		dblDistanceSum += (43 - intRad);
	}
		
	*intQuality = max(0, (100 - 2.2 * (dblDistanceSum / (intToneMagsLength / 16))));	 // factor 2.2 emperically chosen for calibration (Qual range 25 to 100)
//	*intQuality = max(0, (100 - 1.0 * (dblDistanceSum / (intToneMagsLength / 16))));	 // factor 2.2 emperically chosen for calibration (Qual range 25 to 100)

		//If MCB.AccumulateStats Then
        //        stcQualityStats.int16FSKQualityCnts += 1
        //        stcQualityStats.int16FSKQuality += intQuality
         //   End If

}

//	Subroutine to udpate the 8FSK Constellation

void Update8FSKConstellation(int * intToneMags, int * intQuality)
{
	//	Subroutine to update bmpConstellation plot for 8FSK modes...
/*
            Dim dblRad As Double = 0
            Dim intToneSum As Int32 = 0
            Dim intMagMax As Double = 0
            Dim dblAng As Double
            Dim dblDistanceSum As Double
            Dim dblPlotRotation As Double = 0
            Dim stcStatus As Status
            Dim yCenter, xCenter As Integer
            Dim intRad As Int32
            Dim clrPixel As System.Drawing.Color
            Dim intJatMaxMag As Int32
            bmpConstellation = New Bitmap(89, 89)
            ' Draw the axis and paint the black background area
            yCenter = (bmpConstellation.Height - 2) \ 2
            xCenter = (bmpConstellation.Width - 2) \ 2
            For x As Integer = 0 To bmpConstellation.Width - 1
                For y As Integer = 0 To bmpConstellation.Height - 1
                    If y = yCenter Or x = xCenter Then
                        bmpConstellation.SetPixel(x, y, Color.DeepSkyBlue)
                    End If
                Next y
            Next x
            For i As Integer = 0 To (intToneMags.Length - 1) Step 8  ' for the number of symbols represented by intToneMags
                intToneSum = 0
                intMagMax = 0
                For j As Integer = 0 To 7
                    If intToneMags(i + j) > intMagMax Then
                        intMagMax = intToneMags(i + j)
                        intJatMaxMag = j
                    End If
                    intToneSum += intToneMags(i + j)
                Next j
                intRad = Max(5, 42 - 40 * (intToneSum - intMagMax) / intToneSum)
                If intRad < 15 Then
                    clrPixel = Color.Tomato
                ElseIf intRad < 30 Then
                    clrPixel = Color.Gold
                Else
                    clrPixel = Color.Lime
                End If
                ' plot the symbols rotated to avoid the axis
                dblAng = PI / 9 + (intJatMaxMag * PI / 4)
                bmpConstellation.SetPixel(CInt(xCenter + intRad * Cos(dblAng)), CInt(yCenter + intRad * Sin(dblAng)), clrPixel)
                dblDistanceSum += (43 - intRad)
            Next i
            intQuality = Math.Max(0, CInt(100 - 2.0 * (dblDistanceSum / (intToneMags.Length \ 8)))) ' factor 2.0 emperically chosen for calibration (Qual range 25 to 100)
            stcStatus.ControlName = "lblQuality"
            stcStatus.Text = "8FSK Quality: " & intQuality.ToString
            queTNCStatus.Enqueue(stcStatus)
            If MCB.AccumulateStats Then
                stcQualityStats.int8FSKQualityCnts += 1
                stcQualityStats.int8FSKQuality += intQuality
            End If
            stcStatus.ControlName = "ConstellationPlot"
            queTNCStatus.Enqueue(stcStatus)
        Catch ex As Exception
            Logs.Exception("[Main.Update8FSKConstellation] Err: " & ex.ToString)
        End Try
    End Sub ' Update8FSKConstellation
	*/
	return;

}



//	Subroutine to Update the PhaseConstellation

int intMagLength;
int intPhasesLength;

int UpdatePhaseConstellation(short * intPhases, short * intMags, char * strMod, BOOL blnQAM)
{
	// Subroutine to update bmpConstellation plot for PSK modes...
	// Skip plotting and calculations of intPSKPhase(0) as this is a reference phase (9/30/2014)

	int intPSKPhase = strMod[0] - '0';
	float dblPhaseError; 
	float dblPhaseErrorSum = 0;
	int intPSKIndex;
	int intX, intY, intP = 0;
	float dblRad = 0;
	float dblAvgRad = 0;
	float intMagMax = 0;
	float dblPi4 = 0.25 * M_PI;
	float dbPhaseStep  = 2 * M_PI / intPSKPhase;
	float dblRadError = 0;
	float  dblPlotRotation = 0;
	// stcStatus As Status
	int yCenter = 0, xCenter = 0;
	int i,j;

	intMagLength = intPhasesLength = intPSKMode;

	if (intPSKPhase == 4)
		intPSKIndex = 0;
	else
		intPSKIndex = 1;
 
           // bmpConstellation = New Bitmap(89, 89)
           // ' Draw the axis and paint the black background area
            //yCenter = (bmpConstellation.Height - 1) \ 2
           // xCenter = (bmpConstellation.Width - 1) \ 2
            //For x As Integer = 0 To bmpConstellation.Width - 1
             //   For y As Integer = 0 To bmpConstellation.Height - 1
             //       If y = yCenter Or x = xCenter Then
             //           bmpConstellation.SetPixel(x, y, Color.Tomato)
             //       End If
             //   Next y
            //Next x

	for (j = 1; j < intMagLength; j++)   // skip the magnitude of the reference in calculation
	{
		intMagMax = max(intMagMax, intMags[j]); // find the max magnitude to auto scale
		dblAvgRad += intMags[j];
	}
	dblAvgRad = dblAvgRad / (intMagLength);  // the average radius
  
	for (i = 1; i <  intPhasesLength; i++)  // Don't plot the first phase (reference)
	{
		dblRad = 40 * intMags[i] / intMagMax; // scale the radius dblRad based on intMagMax
        //        intX = CInt(xCenter + dblRad * Math.Cos(dblPlotRotation + intPhases(i) / 1000))
		intY = (yCenter + dblRad * sinf(dblPlotRotation + intPhases[i] / 1000));
        intP = 0.001 * intPhases[i] / dbPhaseStep;

		//' compute the Phase and Raduius errors

		dblRadError += powf(dblAvgRad - intMags[i], 2); 
		dblPhaseError = fabsf(((0.001 * intPhases[i]) - intP * dbPhaseStep)); // always positive and < .5 *  dblPhaseStep
		dblPhaseErrorSum += dblPhaseError;

        // If intX <> xCenter And intY <> yCenter Then bmpConstellation.SetPixel(intX, intY, Color.Yellow) ' don't plot on top of axis
	}

	dblRadError = sqrtf(dblRadError / (intPhasesLength)) / dblAvgRad;
    
	if (blnQAM) 
	{
		//include Radius error for QAM ...Lifted from WINMOR....may need work
        //        return = CInt(Math.Max(0, (1 - dblRadError) * (100 - 200 * (dblPhaseErrorSum / (intPhases.Length - 1)) / dbPhaseStep)))
	}
	//       ' This gives good quality with probable seccessful decoding threshold around quality value of 60 to 70

	return  max(0, ((100 - 200 * (dblPhaseErrorSum / (intPhasesLength)) / dbPhaseStep))); // ignore radius error for (PSK) but include for QAM
	//        'Debug.WriteLine("  Avg Radius Error: " & Format(dblRadError, "0.0"))


            //If MCB.AccumulateStats Then
             //   stcQualityStats.intPSKQualityCnts(intPSKIndex) += 1
             //   stcQualityStats.intPSKQuality(intPSKIndex) += intQuality
             //   stcQualityStats.intPSKSymbolsDecoded += intPhases.Length
            //End If
           // stcStatus.ControlName = "lblQuality"
           // stcStatus.Text = strMod & " Quality: " & intQuality.ToString
           // queTNCStatus.Enqueue(stcStatus)
           // stcStatus.ControlName = "ConstellationPlot"
           // queTNCStatus.Enqueue(stcStatus)
}


// Subroutine to track 1 carrier 4FSK. Used for both single and multiple simultaneous carrier 4FSK modes.


VOID Track1Car4FSK(short * intSamples, int * intPtr, int intSampPerSymbol, float dblSearchFreq, int intBaud, UCHAR * bytSymHistory)
{
	// look at magnitude of the tone for bytHistory(1)  2 sample2 earlier and 2 samples later.  and pick the maximum adjusting intPtr + or - 1
	// this seems to work fine on test Mar 16, 2015. This should handle sample rate offsets (sender to receiver) up to about 2000 ppm

	float dblReal, dblImag, dblMagEarly, dblMag, dblMagLate;
	float dblBinToSearch = (dblSearchFreq - (intBaud * bytSymHistory[1])) / intBaud; //  select the 2nd last symbol for magnitude comparison


	GoertzelRealImag(intSamples, (*intPtr - intSampPerSymbol - 2), intSampPerSymbol, dblBinToSearch, &dblReal, &dblImag);
	dblMagEarly = powf(dblReal, 2) + powf(dblImag, 2);
	GoertzelRealImag(intSamples, (*intPtr - intSampPerSymbol), intSampPerSymbol, dblBinToSearch, &dblReal, &dblImag);
	dblMag = powf(dblReal, 2) + powf(dblImag, 2);
	GoertzelRealImag(intSamples, (*intPtr - intSampPerSymbol + 2), intSampPerSymbol, dblBinToSearch, &dblReal, &dblImag);
	dblMagLate = powf(dblReal, 2) + powf(dblImag, 2);

	if (dblMagEarly > dblMag && dblMagEarly > dblMagLate)
	{
		*intPtr --;
		Corrections--;
        //        If AccumulateStats Then intAccumFSKTracking -= 1
	}
	else if (dblMagLate > dblMag && dblMagLate > dblMagEarly)
	{
		*intPtr ++;
		Corrections++;
       //         If AccumulateStats Then intAccumFSKTracking += 1
	}
}

//	Function to Decode one Carrier of PSK modulation 

//	Ideally want to be able to call on for each symbol, as I don't have the
//	RAM to build whole frame

//	Call for each set of 4 or 8 Phase Values

VOID Decode1CarPSKChar(UCHAR * Decoded, int Carrier)
{
	unsigned int int24Bits;
	UCHAR bytRawData;
	int k;
	int Start = 0;

	// Phase Samples are in intPhases

	switch (intPSKMode)
	{
	case 4:		// process 4 sequential phases per byte (2 bits per phase)

		for (k = 0; k < 4; k++)
		{
			if (k == 0)
				bytRawData = 0;
			else
				bytRawData <<= 2;
			
			if (intPhases[Carrier][Start] < 786 && intPhases[Carrier][Start] > -786)
			{
			}		// Zero so no need to do anything
			else if (intPhases[Carrier][Start] >= 786 && intPhases[Carrier][Start] < 2356)
				bytRawData += 1;
			else if (intPhases[Carrier][Start] >= 2356 || intPhases[Carrier][Start] <= -2356)
				bytRawData += 2;
			else
				bytRawData += 3;

			Start++;
		}

		Decoded[charIndex] = bytRawData;

		break;

	case 8: // Process 8 sequential phases (3 bits per phase)  for 24 bits or 3 bytes  

		//	Status verified on 1 Carrier 8PSK with no RS needed for High S/N

		//	Assume we check for 8 available phase samples before being called

		int24Bits = 0;

		for (k = 0; k < 8; k++)
		{
			int24Bits <<= 3;

			if (intPhases[Carrier][Start] < 393 && intPhases[Carrier][Start] > -393)
			{
			}		// Zero so no need to do anything
			else if (intPhases[Carrier][Start] >= 393 && intPhases[Carrier][Start] < 1179)
				int24Bits += 1;
			else if (intPhases[Carrier][Start] >= 1179 && intPhases[Carrier][Start] < 1965)
				int24Bits += 2;
			else if (intPhases[Carrier][Start] >= 1965 && intPhases[Carrier][Start] < 2751)
				int24Bits += 3;
			else if (intPhases[Carrier][Start] >= 2751 || intPhases[Carrier][Start] < -2751)
				int24Bits += 4;
			else if (intPhases[Carrier][Start] >= -2751 && intPhases[Carrier][Start] < -1965)
				int24Bits += 5;
			else if (intPhases[Carrier][Start] >= -1965 && intPhases[Carrier][Start] <= -1179)
				int24Bits += 6;
			else 
				int24Bits += 7;

			Start ++;
	
		}
		Decoded[charIndex] = int24Bits >> 16;
		Decoded[charIndex + 1] = int24Bits >> 8;
		Decoded[charIndex + 2] = int24Bits ;

		break;
	}


	return;
}


//	Function to compute PSK symbol tracking (all PSK modes, used for single or multiple carrier modes) 

int Track1CarPSK(int intCarFreq, char * strPSKMod, float dblUnfilteredPhase, BOOL blnInit)
{
	// This routine initializes and tracks the phase offset per symbol and adjust intPtr +/-1 when the offset creeps to a threshold value.
	// adjusts (by Ref) intPtr 0, -1 or +1 based on a filtering of phase offset. 
	// this seems to work fine on test Mar 21, 2015. May need optimization after testing with higher sample rate errors. This should handle sample rate offsets (sender to receiver) up to about 2000 ppm

	float dblAlpha = 0.3f; // low pass filter constant  may want to optimize value after testing with large sample rate error. 
		// (Affects how much averaging is done) lower values of dblAlpha will minimize adjustments but track more slugishly.

	float dblPhaseOffset;

	static float dblTrackingPhase = 0;
	static float dblModFactor;
	static float dblRadiansPerSample;  // range is .4188 @ car freq = 800 to 1.1195 @ car freq 2200
	static float dblPhaseAtLastTrack;
	static int intCountAtLastTrack;
	static float dblFilteredPhaseOffset;

	if (blnInit)
	{
		// dblFilterredPhase = dblUnfilteredPhase;
		dblTrackingPhase = dblUnfilteredPhase;
		
		if (strPSKMod[0] == '8')
			dblModFactor = M_PI / 4;
		else if (strPSKMod[0] == '4')
			dblModFactor = M_PI / 2;

		dblRadiansPerSample = intCarFreq * dbl2Pi / 12000;
		dblPhaseOffset = dblUnfilteredPhase - dblModFactor * round(dblUnfilteredPhase / dblModFactor);
		dblPhaseAtLastTrack = dblPhaseOffset;
		dblFilteredPhaseOffset = dblPhaseOffset;
		intCountAtLastTrack = 0;
		return 0;
	}

	intCountAtLastTrack += 1;
	dblPhaseOffset = dblUnfilteredPhase - dblModFactor * round(dblUnfilteredPhase / dblModFactor);
	dblFilteredPhaseOffset = (1 - dblAlpha) * dblFilteredPhaseOffset + dblAlpha * dblPhaseOffset;

	if ((dblFilteredPhaseOffset - dblPhaseAtLastTrack) > dblRadiansPerSample)
	{
		//Debug.WriteLine("Filtered>LastTrack: Cnt=" & intCountAtLastTrack.ToString & "  Filtered = " & Format(dblFilteredPhaseOffset, "00.000") & "  Offset = " & Format(dblPhaseOffset, "00.000") & "  Unfiltered = " & Format(dblUnfilteredPhase, "00.000"))
		dblFilteredPhaseOffset = dblPhaseOffset - dblRadiansPerSample;
		dblPhaseAtLastTrack = dblFilteredPhaseOffset;
	
		//if MCB.AccumulateStats Then
        //       stcTuningStats.intPSKTrackAttempts += 1
        //        stcTuningStats.intAccumPSKTracking -= 1
        // 

		return -1;
	}

	if ((dblPhaseAtLastTrack - dblFilteredPhaseOffset) > dblRadiansPerSample)
	{
		//'Debug.WriteLine("Filtered<LastTrack: Cnt=" & intCountAtLastTrack.ToString & "  Filtered = " & Format(dblFilteredPhaseOffset, "00.000") & "  Offset = " & Format(dblPhaseOffset, "00.000") & "  Unfiltered = " & Format(dblUnfilteredPhase, "00.000"))
		dblFilteredPhaseOffset = dblPhaseOffset + dblRadiansPerSample;
		dblPhaseAtLastTrack = dblFilteredPhaseOffset;
        //If MCB.AccumulateStats Then
        //    stcTuningStats.intPSKTrackAttempts += 1
        //  stcTuningStats.intAccumPSKTracking += 1
	
		return 1;
	}
	// 'Debug.WriteLine("Filtered Phase = " & Format(dblFilteredPhaseOffset, "00.000") & "  Offset = " & Format(dblPhaseOffset, "00.000") & "  Unfiltered = " & Format(dblUnfilteredPhase, "00.000"))

	return 0;
}
 
// Function to compute the differenc of two angles 

int ComputeAng1_Ang2(int intAng1, int intAng2)
{
	// do an angle subtraction intAng1 minus intAng2 (in milliradians) 
	// Results always between -3142 and 3142 (+/- Pi)

	int intDiff;

	intDiff = intAng1 - intAng2;

	if (intDiff < -3142)
		intDiff += 6284;
	else if (intDiff > 3142 )
		intDiff -= 6284;

	return intDiff;
}

// Subroutine to "rotate" the phases to try and set the average offset to 0. 

void CorrectPhaseForTuningOffset(short * intPhase, int intPhaseLength, char * strMod)
{
	// A tunning error of -1 Hz will rotate the phase calculation Clockwise ~ 64 milliradians (~4 degrees)
	//   This corrects for:
	// 1) Small tuning errors which result in a phase bias (rotation) of then entire constellation
	// 2) Small Transmitter/receiver drift during the frame by averaging and adjusting to constellation to the average. 
	//   It only processes phase values close to the nominal to avoid generating too large of a correction from outliers: +/- 30 deg for 4PSK, +/- 15 deg for 8PSK
	//  Is very affective in handling initial tuning error.  

	short intPhaseMargin  = 2095 / intPSKMode; // Compute the acceptable phase correction range (+/-30 degrees for 4 PSK)
	short intPhaseInc = 6284 / intPSKMode;
	int intTest;
	int i;
	int intOffset, intAvgOffset, intAvgOffsetBeginning, intAvgOffsetEnd;
	int intAccOffsetCnt = 0, intAccOffsetCntBeginning = 0, intAccOffsetCntEnd = 0;

	// Compute the average offset (rotation) for all symbols within +/- intPhaseMargin of nominal
            
	int intAccOffset = 0, intAccOffsetBeginning = 0, intAccOffsetEnd = 0;

	for (i = 0; i <  intPhaseLength; i++)
	{
		intTest = (intPhase[i] + intPhaseInc/2) / intPhaseInc;
		intOffset = intPhase[i] - intTest * intPhaseInc;

		if ((intOffset >= 0 && intOffset <= intPhaseMargin) || (intOffset < 0 && intOffset >= -intPhaseMargin))
		{
			intAccOffsetCnt += 1;
			intAccOffset += intOffset;
			
			if (i <= intPhaseLength / 4)
			{
				intAccOffsetCntBeginning += 1;
				intAccOffsetBeginning += intOffset;
			}
			else if (i >= (3 * intPhaseLength) / 4)
			{
				intAccOffsetCntEnd += 1;
				intAccOffsetEnd += intOffset;
			}
		}
	}
	
	if (intAccOffsetCnt > 0)
		intAvgOffset = (intAccOffset / intAccOffsetCnt);
	if (intAccOffsetCntBeginning > 0)
		intAvgOffsetBeginning = (intAccOffsetBeginning / intAccOffsetCntBeginning);
	if (intAccOffsetCntEnd > 0)
		intAvgOffsetEnd = (intAccOffsetEnd / intAccOffsetCntEnd);
     
	//Debugprintf("[CorrectPhaseForOffset] Beginning: %d End: %d Total: %d",
		//intAvgOffsetBeginning, intAvgOffsetEnd, intAvgOffset);

	if ((intAccOffsetCntBeginning > intPhaseLength / 8) && (intAccOffsetCntEnd > intPhaseLength / 8))
	{
		for (i = 0; i < intPhaseLength; i++)
		{
			intPhase[i] = intPhase[i] - ((intAvgOffsetBeginning * (intPhaseLength - i) / intPhaseLength) + (intAvgOffsetEnd * i / intPhaseLength));
			if (intPhase[i] > 3142)
				intPhase[i] -= 6284;
			else if (intPhase[i] < -3142)
				intPhase[i] += 6284;
		}
		if (DebugLog) Debugprintf("[CorrectPhaseForTuningOffset] AvgOffsetBeginning=%d AvgOffsetEnd=%d AccOffsetCnt=%d/%d",
				intAvgOffsetBeginning, intAvgOffsetEnd, intAccOffsetCnt, intPhaseLength);
	}
	else if (intAccOffsetCnt > intPhaseLength / 2)
	{
		for (i = 0; i < intPhaseLength; i++)
		{
			intPhase[i] -= intAvgOffset;
			if (intPhase[i] > 3142)
				intPhase[i] -= 6284;
			else if (intPhase[i] < -3142)
				intPhase[i] += 6284;
		}
		if (DebugLog) Debugprintf("[CorrectPhaseForTuningOffset] AvgOffset=%d AccOffsetCnt=%d/%d",
				intAvgOffset, intAccOffsetCnt, intPhaseLength);

	}
}


//	Functions to demod all PSKData frames single or multiple carriers 

double dblPhaseInc;  // in milliradians
short intNforGoertzel[8];
short intPSKPhase_1[8], intPSKPhase_0[8];
//int intPCThresh = 194;  // (about 22 degrees... should work for 4PSK or 8PSK)
short intCP[8];	  // Cyclic prefix offset 
float dblFreqBin[8];


VOID InitDemodPSK()
{
	// Called at start of frame

	int i;
	float dblPhase, dblReal, dblImag;

	intPSKMode = strMod[0] - '0';
	PSKInitDone = TRUE;

	if (intPSKMode == 8)
		dblPhaseInc = 2 * M_PI * 1000 / 8;
	else
		dblPhaseInc = 2 * M_PI * 1000 / 4;

	if (intBaud == 167)
		intSampPerSym = 72;
	else
		intSampPerSym = 120;

	if (intNumCar == 1)
		intCarFreq = 1500;
	else
		intCarFreq = 1400 + (intNumCar / 2) * 200; // start at the highest carrier freq which is actually the lowest transmitted carrier due to Reverse sideband mixing
  
	for (i= 0; i < intNumCar; i++)
	{
		if (intBaud == 100 && intCarFreq == 1500) 
		{
		intCP[i] = 20;  //  These values selected for best decode percentage (92%) and best average 4PSK Quality (82) on MPP0dB channel
		dblFreqBin[i] = intCarFreq / 150;
		intNforGoertzel[i] = 80;
		}
		else if (intBaud == 100)
		{
			intCP[i] = 28; // This value selected for best decoding percentage (56%) and best Averag 4PSK Quality (77) on mpg +5 dB
			intNforGoertzel[i] = 60;
			dblFreqBin[i] = intCarFreq / 200;
		}
		else if (intBaud == 167)
		{
			intCP[i] = 6;  // Need to optimize (little difference between 6 and 12 @ wgn5, 2 Car 500 Hz)
			intNforGoertzel[i] = 60;
			dblFreqBin[i] = intCarFreq / 200;
		}
	
		// Get initial Reference Phase
		
		GoertzelRealImag(intFilteredMixedSamples, intCP[i], intNforGoertzel[i], dblFreqBin[i], &dblReal, &dblImag);
		dblPhase = atan2f(dblImag, dblReal);
		Track1CarPSK(intCarFreq, strMod, dblPhase, TRUE);
		intPSKPhase_1[i] = 1000 * dblPhase;
		intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 
	}
}

int Demod1CarPSKChar(int Start, int Carrier);

BOOL DemodPSK()
{
	int Used;
	int Start = 0;

	// We can't wait for the full frame as we don't have enough RAM, so
	// we do one DMA Buffer at a time, until we run out or end of frame

	// Only continue if we have enough samples

	
	while (State == AcquireFrame)
	{
		if (intFilteredMixedSamplesLength < 4 * intSampPerSym + 10) // allow for a few phase corrections
		{
			// Move any unprocessessed data down buffer

			//	(while checking process - will use cyclic buffer eventually

			if (intFilteredMixedSamplesLength > 0)
				memmove(intFilteredMixedSamples,
					&intFilteredMixedSamples[Start], intFilteredMixedSamplesLength * 2); 

			return FALSE;
		}
		

		if (PSKInitDone == 0)		// First time through
		{	
			if (intFilteredMixedSamplesLength < 8 * intSampPerSym + 10) 
				return FALSE;				// Wait for at least 2 chars worth

			InitDemodPSK();
			intFilteredMixedSamplesLength -= intSampPerSym;
			Start += intSampPerSym;	
		}

		// If this is a multicarrier mode, we must call the
		// decode char routing for each carrier

		if (intNumCar == 1)
			intCarFreq = 1500;
		else
			intCarFreq = 1400 + (intNumCar / 2) * 200; // start at the highest carrier freq which is actually the lowest transmitted carrier due to Reverse sideband mixing

	
		switch (intNumCar)
		{
		case 1:

			Used = Demod1CarPSKChar(Start, 0);
			break;

		case 2:

			Demod1CarPSKChar(Start, 0);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Used = Demod1CarPSKChar(Start, 1);
			break;

		case 4:

			Demod1CarPSKChar(Start, 0);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 
	
			Demod1CarPSKChar(Start, 1);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Demod1CarPSKChar(Start, 2);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Used = Demod1CarPSKChar(Start, 3);
			break;

		case 8:

			Demod1CarPSKChar(Start, 0);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Demod1CarPSKChar(Start, 1);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Demod1CarPSKChar(Start, 2);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Demod1CarPSKChar(Start, 3);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Demod1CarPSKChar(Start, 4);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Demod1CarPSKChar(Start, 5);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Demod1CarPSKChar(Start, 6);
			intPhasesLen -= 4; //intPSKMode;
			intCarFreq -= 200;  // Step through each carrier Highest to lowest which is equivalent to lowest to highest before RSB mixing. 

			Used = Demod1CarPSKChar(Start, 7);	
		}

		//	if we have 4 or 8 Phase samples, decode them 
		
		// (or each carrier)

		if (intPhasesLen >= intPSKMode)
		{
	//		CorrectPhaseForTuningOffset(intPhases, intPhasesLen, strMod);

			Decode1CarPSKChar(bytFrameData1, 0);
			if (intNumCar > 1)
				Decode1CarPSKChar(bytFrameData2, 1);
			if (intNumCar > 2)
			{
				Decode1CarPSKChar(bytFrameData3, 2);
				Decode1CarPSKChar(bytFrameData4, 3);
			}
			if (intNumCar > 4)
			{
				Decode1CarPSKChar(bytFrameData5, 4);
				Decode1CarPSKChar(bytFrameData6, 5);
				Decode1CarPSKChar(bytFrameData7, 6);
				Decode1CarPSKChar(bytFrameData8, 7);
			}
			intPhasesLen -= intPSKMode;

			if (intPSKMode == 4)
			{
				SymbolsLeft--;		// number still to decode
				charIndex++;
			}
			else
			{
				SymbolsLeft -=3;
				charIndex += 3;
			}		
		}

		Start += Used; //intSampPerSym * 4;	
		intFilteredMixedSamplesLength -= Used; //intSampPerSym * 4;

		if (SymbolsLeft <= 0)	
		{	
			// prepare for next

			DiscardOldSamples();
			ClearAllMixedSamples();
			State = SearchingForLeader;
		}
	}
	return TRUE;
}

// Function to demodulate one carrier for all PSK frame types
int Demod1CarPSKChar(int Start, int Carrier)
{
	// Converts intSample to an array of differential phase and magnitude values for the Specific Carrier Freq
	// intPtr should be pointing to the approximate start of the first reference/training symbol (1 of 3) 
	// intPhase() is an array of phase values (in milliradians range of 0 to 6283) for each symbol 
	// intMag() is an array of Magnitude values (not used in PSK decoding but for constellation plotting or QAM decoding)
	// Objective is to use Minimum Phase Error Tracking to maintain optimum pointer position

	//	This is called for one DMA buffer of samples (normally 1200)

	float dblReal, dblImag;
	int intMiliRadPerSample = intCarFreq * M_PI / 6;
	int i;
	int intNumOfSymbols = 4;
	int origStart = Start;;

	for (i = 0; i <  intNumOfSymbols; i++)
	{
		GoertzelRealImag(intFilteredMixedSamples, Start + intCP[Carrier], intNforGoertzel[Carrier], dblFreqBin[Carrier], &dblReal, &dblImag);
		intMags[Carrier][intPhasesLen] = sqrtf(powf(dblReal, 2) + powf(dblImag, 2));
		intPSKPhase_0[Carrier] = 1000 * atan2f(dblImag, dblReal);
		intPhases[Carrier][intPhasesLen] = -(ComputeAng1_Ang2(intPSKPhase_0[Carrier], intPSKPhase_1[Carrier]));

		Corrections = Track1CarPSK(intCarFreq, strMod, atan2f(dblImag, dblReal), FALSE);

		if (Corrections != 0)
		{
			Start += Corrections;

			GoertzelRealImag(intFilteredMixedSamples, Start + intCP[Carrier], intNforGoertzel[Carrier], dblFreqBin[Carrier], &dblReal, &dblImag);
			intPSKPhase_0[Carrier] = 1000 * atan2f(dblImag, dblReal);
		}
		intPSKPhase_1[Carrier] = intPSKPhase_0[Carrier];
		intPhasesLen++;
		Start += intSampPerSym;
	}
       // If AccumulateStats Then intPSKSymbolCnt += intPhase.Length

	return (Start - origStart);	// Symbols we've consumed
}

