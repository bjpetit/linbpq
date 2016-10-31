// ARDOPC.cpp : Defines the entry point for the console application.
//


#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
#endif

#include "ARDOPC.h"

void CompressCallsign(char * Callsign, UCHAR * Compressed);
void CompressGridSquare(char * Square, UCHAR * Compressed);
void  ASCIIto6Bit(char * Padded, UCHAR * Compressed);
void GetTwoToneLeaderWithSync(int intSymLen);
void SendID(BOOL blnEnableCWID);
void PollReceivedSamples();
void CheckTimers();
BOOL GetNextARQFrame();
BOOL HostInit();
void HostPoll();
BOOL MainPoll();
void PlatformSleep();
BOOL BusyDetect2(float * dblMag, int intStart, int intStop);

// Config parameters

char GridSquare[7] = "";
char Callsign[10] = "";
BOOL wantCWID = FALSE;
int LeaderLength = 500;
int TrailerLength = 0;
unsigned int ARQTimeout = 120;
int TuningRange = 100;
int ARQConReqRepeats = 5;
BOOL DebugLog = TRUE;
BOOL CommandTrace = TRUE;
int DriveLevel = 100;
char strFECMode[16] = "4FSK.200.50";
int FECRepeats = 0;
BOOL FECId = FALSE;
int Squelch = 5;
int BusyDet = 5;
enum _ARQBandwidth ARQBandwidth = B2000MAX;
int port = 8515;
BOOL RadioControl = FALSE;
BOOL SlowCPU = FALSE;
BOOL AccumulateStats = TRUE;
BOOL Use600Modes = FALSE;
BOOL FSKOnly = FALSE;
BOOL fastStart = TRUE;
BOOL skip167 = FALSE;

// Stats

//    Public Structure QualityStats
  
int int4FSKQuality;
int int4FSKQualityCnts;
int int8FSKQuality;
int int8FSKQualityCnts;
int int16FSKQuality;
int int16FSKQualityCnts;
int intFSKSymbolsDecoded;
int intPSKQuality[2];
int intPSKQualityCnts[2];
int intPSKSymbolsDecoded; 

int intQAMQuality;
int intQAMQualityCnts;
int intQAMSymbolsDecoded;
int intGoodQAMSummationDecodes;

BOOL blnInitializing = FALSE;

BOOL blnLastPTT = FALSE;

BOOL PlayComplete = FALSE;

BOOL blnBusyStatus;
BOOL newStatus;

unsigned int tmrSendTimeout;

int intCalcLeader;        // the computed leader to use based on the reported Leader Length
int intRmtLeaderMeasure = 0;

int dttCodecStarted;

enum _ReceiveState State;
enum _ARDOPState ProtocolState;

const char ARDOPStates[8][9] = {"OFFLINE", "DISC", "ISS", "IRS", "IDLE", "IRStoISS", "FECSEND", "FECRCV"};

struct SEM Semaphore = {0, 0, 0, 0};

BOOL SoundIsPlaying = FALSE;
BOOL Capturing = TRUE;

int DecodeCompleteTime;

BOOL blnAbort = FALSE;
int intRepeatCount;
BOOL blnARQDisconnect = FALSE;

enum _ProtocolMode ProtocolMode = FEC;

extern BOOL blnEnbARQRpt;
extern BOOL blnDISCRepeating;
extern char strRemoteCallsign[10];
extern char strLocalCallsign[10];
extern char strFinalIDCallsign[10];
extern int dttTimeoutTrip;
extern unsigned int dttLastFECIDSent;
extern int intFrameRepeatInterval;
extern BOOL blnPending;
extern unsigned int tmrIRSPendingTimeout;
extern unsigned int tmrFinalID;
extern unsigned int tmrPollOBQueue;
int Encode4FSKControl(UCHAR bytFrameType, UCHAR bytSessionID, UCHAR * bytreturn);

int intRepeatCnt;

BOOL blnFramePending = FALSE;
BOOL blnClosing = FALSE;
BOOL blnCodecStarted = FALSE;

unsigned int dttNextPlay = 0;


const UCHAR bytValidFrameTypesALL[]=
{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
35, 36, 41, 44, 45,
 46, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 64, 65, 66,
 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
 84, 85, 88, 89, 90, 91, 92, 93, 96, 97, 98, 99, 100, 101, 102, 103,
 104, 105, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
 124, 125, 208, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234,
 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
 249, 250, 251, 252, 253, 254, 255};

const UCHAR bytValidFrameTypesISS[]=		// ACKs, NAKs, END, DISC, BREAK
{
 //NAK
 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
 // BREAK, DISC, or END
 BREAK, DISCFRAME, END, ConRejBusy, ConRejBW, 
// Con req and Con ACK
 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
 //ACK
 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255};


const UCHAR * bytValidFrameTypes;

int bytValidFrameTypesLengthISS = sizeof(bytValidFrameTypesISS);
int bytValidFrameTypesLengthALL = sizeof(bytValidFrameTypesALL);
int bytValidFrameTypesLength;

UCHAR isValidFrame[256]= 
{
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 00 - 0F    ACK and NAK
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 10 - 1F
	0,0,0,1,1,0,0,0,0,1,0,0,1,1,1,0,	// BREAK=23, IDLE=24, DISC=29, END=2C, ConRejBusy=2D, ConRejBW=2E

	1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,	// 30 - 3F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 40 - 4F
	1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,	// 50 - 5F - 56, 57 were 167 500
	1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	// 60 - 6F
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,	// 70 - 7F
			

	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 80 - 8F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 90 - 9F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// A0 - AF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// B0 - BF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// C0 - CF

	// experimental SOUNDINGs D0

	1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// D0 - DF
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// e0 - eF    ACK and NAK
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1		// f0 - ff
};


BOOL blnTimeoutTriggered = FALSE;

//	We can't keep the audio samples for retry, but we can keep the
//	encoded data

unsigned char bytEncodedBytes[1800] ="";		// I think the biggest is 600 bd 768 + overhead
int EncLen;


extern UCHAR bytSessionID;

int intLastRcvdFrameQuality;

int intAmp = 26000;	   // Selected to have some margin in calculations with 16 bit values (< 32767) this must apply to all filters as well. 

const char strAllDataModes[26][15] = {"8FSK.200.25", "4FSK.200.50S", "4FSK.200.50", "4PSK.200.100S",
		"4PSK.200.100", "8PSK.200.100",  "16QAM.200.100", "16FSK.500.25S", "16FSK.500.25", "4FSK.500.100S",
		"4FSK.500.100", "4PSK.500.100", "8PSK.500.100", "16QAM.500.100",
		"4FSK.1000.100", "4PSK.1000.100", "8PSK.1000.100", "4PSK.1000.167", "8PSK.1000.167", 
		"4FSK.2000.600S", "4FSK.2000.600", "4FSK.2000.100", "4PSK.2000.100", "8PSK.2000.100",
		"4PSK.2000.167", "8PSK.2000.167"};

int strAllDataModesLen = 26;

// Frame Speed By Type (from Rick's spreadsheet)

const short Rate[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 00 - 0F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 10 - 1F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 20 - 2F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 30 - 3F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 40 - 4F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 50 - 5F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 60 - 6F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 70 - 7F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 80 - 8F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 90 - 9F
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// a0 - AF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// b0 - BF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// c0 - CF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// d0 - DF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// e0 - EF
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	// f0 - FF

const char strFrameType[256][18] = {
	"DataNAK", //  Range 0x00 to 0x1F includes 5 bits for quality 1 Car, 200Hz,4FSK
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","",

        //Short Control Frames 1 Car, 200Hz,4FSK  ' Reassigned May 22, 2015 for maximum "distance"
 
	"BREAK", "IDLE", "",
	"", "", "",
	"DISC", "", "",
	"END",
	"ConRejBusy",
	"ConRejBW",
	"",

	//Special frames 1 Car, 200Hz,4FSK 0x30 +

	"IDFrame",
	"ConReq200M",
	"ConReq500M",
	"ConReq1000M",
	"ConReq2000M",
	"ConReq200F",
	"ConReq500F",
	"ConReq1000F",
	"ConReq2000F",
	"ConAck200",
	"ConAck500",
	"ConAck1000",
	"ConAck2000",
	// Types 0x3D to 0x3F reserved
	"","","",
	// 200 Hz Bandwidth Data 
	// 1 Car PSK Data Modes 200 HzBW  100 baud 
	
	"4PSK.200.100.E",	// 0x40
	"4PSK.200.100.O",
	"4PSK.200.100S.E",
	"4PSK.200.100S.O",
	"8PSK.200.100.E",
	"8PSK.200.100.O",

	// 1 Car 4FSK Data mode 200 HzBW, 50 baud 

	"4FSK.200.50.E",
	"4FSK.200.50.O",
	"4FSK.200.50S.E", // 48
	"4FSK.200.50S.O",
	"4FSK.500.100.E",
	"4FSK.500.100.O",
	"4FSK.500.100S.E",
	"4FSK.500.100S.O",
	"8FSK.200.25.E",
	"8FSK.200.25.O",

	//' 2 Car PSK Data Modes 100 baud
	"4PSK.500.100.E", // 50
	"4PSK.500.100.O",
	"8PSK.500.100.E",
	"8PSK.500.100.O",

	// 2 Car Data modes 16 QAM baud  

	"16QAM.500.100.E",	//54
	"16QAM.500.100.O",
	"", "",				// 56, 57 were 500 167 modes	

	// 1 Car 16FSK mode 25 baud

	"16FSK.500.25.E", // 58
	"16FSK.500.25.O",
	"16FSK.500.25S.E",
	"16FSK.500.25S.O",
	
	// 1 Car 16QAM Data mode 200 Hz BW, 100 baud

	"16QAM.200.100.E",	// 5C
    "16QAM.200.100.O",	// 5D
	"","",				// 5E/F

	//1 Khz Bandwidth Data Modes 
	//  4 Car 100 baud PSK
	"4PSK.1000.100.E", //60
	"4PSK.1000.100.O",
	"8PSK.1000.100.E",
	"8PSK.1000.100.O",
	// 4 car 167 baud PSK
	"4PSK.1000.167.E",
	"4PSK.1000.167.O",
	"8PSK.1000.167.E",
	"8PSK.1000.167.O",
	// 2 Car 4FSK 100 baud
	"4FSK.1000.100.E", //68
	"4FSK.1000.100.O","","","","","","",

	// 2Khz Bandwidth Data Modes 
	//  8 Car 100 baud PSK
	"4PSK.2000.100.E", //70 
	"4PSK.2000.100.O",
	"8PSK.2000.100.E",
	"8PSK.2000.100.O",
	//  8 Car 167 baud PSK
	"4PSK.2000.167.E",
	"4PSK.2000.167.O",
	"8PSK.2000.167.E",
	"8PSK.2000.167.O",
	// 4 Car 4FSK 100 baud
	"4FSK.2000.100.E",
	"4FSK.2000.100.O",
	// 1 Car 4FSK 600 baud (FM only)
	"4FSK.2000.600.E", // Experimental 
	"4FSK.2000.600.O", // Experimental
	"4FSK.2000.600S.E", // Experimental
	"4FSK.2000.600S.O", // Experimental //7d
	"","",	// 7e-7f
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","",
	"","","","","","","","","","","","","","","","", //C0

	//Frame Types 0xA0 to 0xDF reserved for experimentation 
	"SOUND2K" //D0
	"","","","","","","","","","","","","","","","",
    //Data ACK  1 Car, 200Hz,4FSK
	"DataACK"		// Range 0xE0 to 0xFF includes 5 bits for quality 
};

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}

#ifdef WIN32
float round(float x)
{
	return floorf(x + 0.5f);
}
#endif

void GetSemaphore()
{
}

void FreeSemaphore()
{
}

BOOL CheckValidCallsignSyntax(char * strCallsign)
{
	// Function for checking valid call sign syntax

	char * Dash = strchr(strCallsign, '-');
	int callLen = strlen(strCallsign);
	char * ptr = strCallsign;
	int SSID;

	if (Dash)
	{
		callLen = Dash - strCallsign;

		SSID = atoi(Dash + 1);
		if (SSID > 15)
			return FALSE;

		if (strlen(Dash + 1) > 2)
			return FALSE;

		if (!isalnum(*(Dash + 1)))
			return FALSE;
	}
		
	if (callLen > 7 || callLen < 3)
			return FALSE;

	while (callLen--)
	{
		if (!isalnum(*(ptr++)))
			return FALSE;
	}
	return TRUE;
}

//	 Function to check for proper syntax of a 4, 6 or 8 character GS

BOOL CheckGSSyntax(char * GS)
{
	int Len = strlen(GS);

	if (!(Len == 4 || Len == 6 || Len == 8))
		return FALSE;

	if (!isalpha(GS[0]) || !isalpha(GS[1]))
		return FALSE;
	
	if (!isdigit(GS[2]) || !isdigit(GS[3]))
		return FALSE;

	if (Len == 4)
		return TRUE;

	if (!isalpha(GS[4]) || !isalpha(GS[5]))
		return FALSE;

	if (Len == 6)
		return TRUE;

	if (!isdigit(GS[6]) || !isdigit(GS[7]))
		return FALSE;

	return TRUE;
}

// Function polled by Main polling loop to see if time to play next wave stream

BOOL GetNextFrame()
{
	// returning TRUE sets frame pending in Main

	if (ProtocolMode == FEC || ProtocolState == FECSend)
	{
		if (ProtocolState == FECSend || ProtocolState == FECRcv || ProtocolState == DISC)
			return GetNextFECFrame();
		else
			return FALSE;
	}
	if (ProtocolMode == ARQ)
		if (ARQState == None)
			return FALSE;
		else
            return GetNextARQFrame();

	return FALSE;
}
     
#ifdef WIN32

extern LARGE_INTEGER Frequency;
extern LARGE_INTEGER StartTicks;
extern LARGE_INTEGER NewTicks;

#endif

extern int NErrors;

void testRS()
{
	// feed random data into RS to check robustness

	BOOL blnRSOK, FrameOK;
	char bytRawData[256];
	int DataLen = 128;
	int intRSLen = 64;
	int i;

	for (i = 0; i < DataLen; i++)
	{
		bytRawData[i] = rand() % 256;
	}

	FrameOK = RSDecode(bytRawData, DataLen, intRSLen, &blnRSOK);
}



void ardopmain()
{
	blnTimeoutTriggered = FALSE;
	SetARDOPProtocolState(DISC);

	InitSound();

	HostInit();

//	while (1)
//		testRS();

	tmrPollOBQueue = Now + 10000;

	ProtocolMode = ARQ;

	while(!blnClosing)
	{
		PollReceivedSamples();
		CheckTimers();	
		HostPoll();
		MainPoll();
		PlatformSleep();
	}
	return;
}


void SendCWID(char * Callsign, BOOL x)
{
}

// Subroutine to generate 1 symbol of leader

//	 returns pointer to Frame Type Name

const char * Name(UCHAR bytID)
{
	if (bytID < 0x20)
		return strFrameType[0];
	else if (bytID >= 0xE0)
		return strFrameType[0xE0];
	else
		return strFrameType[bytID];
}

// Function to look up frame info from bytFrameType

BOOL FrameInfo(UCHAR bytFrameType, int * blnOdd, int * intNumCar, char * strMod,
			   int * intBaud, int * intDataLen, int * intRSLen, UCHAR * bytQualThres, char * strType)
{
	//Used to "lookup" all parameters by frame Type. 
	// returns TRUE if all fields updated otherwise FALSE (improper bytFrameType)

	// 1 Carrier 4FSK control frames 
   
	if ((bytFrameType >= 0 &&  bytFrameType <= 0x1F) || bytFrameType >= 0xE0)
	{
		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 40;
	}
	else
	{

	switch(bytFrameType)
	{
	case 0x23:
	case 0x24:
	case 0x29:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 60;
		break;
	
	case 0x2C:
	case 0x2D:
	case 0x2E:
		
		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 60;
		break;

	case 0x30:
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 12;
		*intRSLen = 4;			// changed 0.8.0
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 50;
		break;

	case 0x39:
	case 0x3A:
	case 0x3B:
	case 0x3C:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 3;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 50;
		break;

	case 0xe0:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 0;
		*intRSLen = 0;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 60;
		break;

	// 1 Carrier Data modes
    //  100 baud PSK (200 baud not compatible with 200 Hz bandwidth)  (Note 1 carrier modes Qual Threshold reduced to 30 (was 50) for testing April 20, 2015

	case 0x40:
	case 0x41:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 30;	
		break;
	
	case 0x42:
	case 0x43:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 16;
		*intRSLen = 8;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;
	
	case 0x44:
	case 0x45:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;

	// 50 baud 4FSK 200 Hz bandwidth

	case 0x46:
	case 0x47:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 32;
		*intRSLen = 8;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 30;
		break;

	case 0x48:
	case 0x49:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 16;
		*intRSLen = 4;
		strcpy(strMod, "4FSK");
		*intBaud = 50;
		*bytQualThres = 30;
		break;
	
	// 100 baud 4FSK 500 Hz bandwidth

	case 0x4A:
	case 0x4B:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 64;
		*intRSLen = 16;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;
 
 	case 0x4C:
	case 0x4D:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 32;
		*intRSLen = 8;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 30;
		break;

	// 25 baud 8FSK 200 Hz bandwidth (in testing)
 
 	case 0x4E:
	case 0x4F:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 24;
		*intRSLen = 6;
		strcpy(strMod, "8FSK");
		*intBaud = 25;
		*bytQualThres = 30; 
		break;
		
	// 25 baud 16FSK 500 Hz bandwidth 

 	case 0x58:
	case 0x59:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 32;
		*intRSLen = 8;
		strcpy(strMod, "16FSK");
		*intBaud = 25;
		*bytQualThres = 30;
 		break;

	case 0x5A:
 	case 0x5B:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 16;
		*intRSLen = 4;
		strcpy(strMod, "16FSK");
		*intBaud = 25;
		*bytQualThres = 30;
 		break;

	case 0x5C:
 	case 0x5D:

		// 100 baud 16QAM 
  
		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 128;
		*intRSLen = 64;
		strcpy(strMod, "16QAM");
		*intBaud = 100;
		*bytQualThres = 30;
 		break;

	// 600 baud 4FSK 2000 Hz bandwidth 

	case 0x7a:
	case 0x7b:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 600;
		*intRSLen = 150;
		strcpy(strMod, "4FSK");
		*intBaud = 600;
		*bytQualThres = 30;
		break;
 
	case 0x7C:
	case 0x7D:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 1;
		*intDataLen = 200;
		*intRSLen = 50;
		strcpy(strMod, "4FSK");
		*intBaud = 600;
		*bytQualThres = 30;
		break;
 
	// 2 Carrier Data Modes
	// 100 baud 

	case 0x50:
	case 0x51:

		*blnOdd = (1 & bytFrameType) != 0;
		*intNumCar = 2;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
 		break;

	case 0x52:
	case 0x53:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;
						
	// 16 QAM 2 Carrier 
    
	case 0x54:
	case 0x55:

		// 100 baud 16QAM
  
		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 128;
		*intRSLen = 64;
		strcpy(strMod, "16QAM");
		*intBaud = 100;
		*bytQualThres = 50;
		break;
		
	//' 4 Carrier Data Modes
	//	100 baud
     	
	case 0x60:
	case 0x61:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;
	
	case 0x62:
	case 0x63:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	//	167 Baud

	case 0x64:
	case 0x65:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 120;
		*intRSLen = 40;
		strcpy(strMod, "4PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;

	case 0x66:
	case 0x67:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 159;
		*intRSLen = 60;
		strcpy(strMod, "8PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;
	
	// 100 baud 2 carrier 4FSK
     
	case 0x68:
	case 0x69:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 2;
		*intDataLen = 64;
		*intRSLen = 16;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	// 8 Carrier Data modes
	//	100 baud

	case 0x70:
	case 0x71:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 64;
		*intRSLen = 32;
		strcpy(strMod, "4PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	case 0x72:
	case 0x73:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 108;
		*intRSLen = 36;
		strcpy(strMod, "8PSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	// 167 baud

	case 0x74:
	case 0x75:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 120;
		*intRSLen = 40;
		strcpy(strMod, "4PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;
		
	case 0x76:
	case 0x77:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 8;
		*intDataLen = 159;
		*intRSLen = 60;
		strcpy(strMod, "8PSK");
		*intBaud = 167;
		*bytQualThres = 50;
		break;

	// 100 baud 4 carrier 4FSK
 
	case 0x78:
	case 0x79:

		*blnOdd = bytFrameType & 1;
		*intNumCar = 4;
		*intDataLen = 64;
		*intRSLen = 16;
		strcpy(strMod, "4FSK");
		*intBaud = 100;
		*bytQualThres = 50;
		break;

	default:
		//'Logs.Exception("[PSKDataInfo] No data for frame type= H" & Format(bytFrameType, "x"))
        return FALSE;
	}
	}
	
	if (bytFrameType >= 0 && bytFrameType <= 0x1F)
		strcpy(strType,strFrameType[0]);
	else
		if (bytFrameType >= 0xE0) 
			strcpy(strType,strFrameType[0xE0]);
        else
			strcpy(strType,strFrameType[bytFrameType]);

	return TRUE;
}

int NPAR = -1;	// Number of Parity Bytes - used in RS Code

int MaxErrors = 0;

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
		MaxErrors = NPAR / 2;
		initialize_ecc();
	}

	// Copy the supplied data to end of data array.

	memset(Padded, 0, PadLength);
	memcpy(&Padded[PadLength], bytToRS, DataLen); 

	encode_data(Padded, 255-RSLen, RSBytes);

	return RSLen;
}

//	Main RS decode function

extern int index_of[];
extern int recd[];
int Corrected[256];
extern int tt;		//  number of errors that can be corrected 
extern int kk;		// Info Symbols

BOOL blnErrorsCorrected;

#define NEWRS

BOOL RSDecode(UCHAR * bytRcv, int Length, int CheckLen, BOOL * blnRSOK)
{	
#ifdef NEWRS

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
	UCHAR * ptr1 = &bytRcv[Length - CheckLen -1]; // Last Byte of Data

	int DataLen = Length - CheckLen;
	int PadLength = 255 - Length;		// Padding bytes needed for shortened RS codes

	*blnRSOK = FALSE;

	if (Length > 255 || Length < (1 + CheckLen))		//Too long or too short 
		return FALSE;

	if (NPAR != CheckLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = CheckLen;
		MaxErrors = NPAR /2;

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

	ptr2+= PadLength;
	
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

    if (correct_errors_erasures (intTemp, 255, 0, 0) == 0) // Dont support erasures at the momnet

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

#else

	// Old (Rick's) code

	// Sets blnRSOK if OK without correction

	// Returns TRUE if OK oe Corrected
	// False if Can't correct


	UCHAR intTemp[256];				// Work Area to pass to Decoder		
	int i;
	int intStartIndex;
	UCHAR * ptr2 = intTemp;
	UCHAR * ptr1 = bytRcv;
	BOOL RSWasOK;

	int DataLen = Length - CheckLen;
	int PadLength = 255 - Length;		// Padding bytes needed for shortened RS codes

	*blnRSOK = FALSE;

	if (Length > 255 || Length < (1 + CheckLen))		//Too long or too short 
		return FALSE;


	if (NPAR != CheckLen)		// Changed RS Len, so recalc constants;
	{
		NPAR = CheckLen;
		tt = sqrt(NPAR);
		kk = 255-CheckLen; 
		generate_gf();
		gen_poly();
	}
	
	intStartIndex =  255 - Length; // set the start point for shortened RS codes

	//	We always work on a 255 byte buffer, prepending zeros if neccessary

 	//	Clear padding

	memset(ptr2, 0, PadLength);	
	ptr2 += PadLength;

	memcpy(ptr2, ptr1, Length);
	
	// convert to indexed form

	for(i = 0; i < 256; i++)
	{
//		intIsave = i;
//		intIndexSave = index_of[intTemp[i]];
		recd[i] = index_of[intTemp[i]];
	}

//	printtick("entering decode_rs");

	blnErrorsCorrected = FALSE;

	RSWasOK = decode_rs();

//	printtick("decode_rs Done");

	*blnRSOK = RSWasOK;

	if (RSWasOK)
		return TRUE;

	if(blnErrorsCorrected)
	{
		for (i = 0; i < DataLen; i++)
		{
			bytRcv[i] = recd[i + intStartIndex];
		}
		return TRUE;
	}

	return FALSE;
}
#endif

// Function to encode data for all PSK frame types

int EncodePSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedBytes)
{
	// Objective is to use this to use this to send all PSK data frames 
	// 'Output is a byte array which includes:
	//  1) A 2 byte Header which include the Frame ID.  This will be sent using 4FSK at 50 baud. It will include the Frame ID and ID Xored by the Session bytID.
	//  2) n sections one for each carrier that will inlcude all data (with FEC appended) for the entire frame. Each block will be identical in length.
	//  Ininitial implementation:
	//    intNum Car may be 1, 2, 4 or 8
	//    intBaud may be 100, 167
	//    intPSKMode may be 4 (4PSK) or 8 (8PSK) 
	//    bytDataToSend must be equal to or less than max data supported by the frame or a exception will be logged and an empty array returned

	//  First determine if bytDataToSend is compatible with the requested modulation mode.

	int intNumCar, intBaud, intDataLen, intRSLen, bytDataToSendLengthPtr, intEncodedDataPtr;

	int intCarDataCnt, intStartIndex;
	BOOL blnOdd;
	char strType[18];
	char strMod[16];
	BOOL blnFrameTypeOK;
	UCHAR bytQualThresh;
	int i;
	UCHAR * bytToRS = &bytEncodedBytes[2]; 

	blnFrameTypeOK = FrameInfo(bytFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytQualThresh, strType);

	if (intDataLen == 0 || Length == 0 || !blnFrameTypeOK)
	{
		//Logs.Exception("[EncodeFSKFrameType] Failure to update parameters for frame type H" & Format(bytFrameType, "X") & "  DataToSend Len=" & bytDataToSend.Length.ToString)
		return 0;
	}
		
	//	Generate the 2 bytes for the frame type data:
	
	bytEncodedBytes[0] = bytFrameType;
	bytEncodedBytes[1] = bytFrameType ^ bytSessionID;

	bytDataToSendLengthPtr = 0;
	intEncodedDataPtr = 2;

	// Now compute the RS frame for each carrier in sequence and move it to bytEncodedBytes 
		
	for (i = 0; i < intNumCar; i++)		//  across all carriers
	{
		intCarDataCnt = Length - bytDataToSendLengthPtr;
			
		if (intCarDataCnt >= intDataLen) // why not > ??
		{
			// Won't all fit 

			bytToRS[0] = intDataLen;
			intStartIndex = intEncodedDataPtr;
			memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intDataLen);
			bytDataToSendLengthPtr += intDataLen;
		}
		else
		{
			// Last bit

			bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

			if (intCarDataCnt > 0)
			{
				memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intCarDataCnt);
				bytDataToSendLengthPtr += intCarDataCnt;
			}	
		}
		
		GenCRC16FrameType(bytToRS, intDataLen + 1, bytFrameType); // calculate the CRC on the byte count + data bytes

		RSEncode(bytToRS, bytToRS+intDataLen+3, intDataLen + 3, intRSLen);  // Generate the RS encoding ...now 14 bytes total
     
 		//  Need: (2 bytes for Frame Type) +( Data + RS + 1 byte byteCount + 2 Byte CRC per carrier)

 		intEncodedDataPtr += intDataLen + 3 + intRSLen;

		bytToRS += intDataLen + 3 + intRSLen;		
	}
	return intEncodedDataPtr;
}


// Function to encode data for all FSK frame types
  
int EncodeFSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedBytes)
{
	// Objective is to use this to use this to send all 4FSK data frames 
	// 'Output is a byte array which includes:
	//  1) A 2 byte Header which include the Frame ID.  This will be sent using 4FSK at 50 baud. It will include the Frame ID and ID Xored by the Session bytID.
	//  2) n sections one for each carrier that will inlcude all data (with FEC appended) for the entire frame. Each block will be identical in length.
	//  Ininitial implementation:
	//    intNum Car may be 1, 2, 4 or 8
	//    intBaud may be 50, 100
	//    strMod is 4FSK) 
	//    bytDataToSend must be equal to or less than max data supported by the frame or a exception will be logged and an empty array returned

	//  First determine if bytDataToSend is compatible with the requested modulation mode.

	int intNumCar, intBaud, intDataLen, intRSLen, bytDataToSendLengthPtr, intEncodedDataPtr;

	int intCarDataCnt, intStartIndex;
	BOOL blnOdd;
	char strType[18];
	char strMod[16];
	BOOL blnFrameTypeOK;
	UCHAR bytQualThresh;
	int i;
	UCHAR * bytToRS = &bytEncodedBytes[2]; 

	blnFrameTypeOK = FrameInfo(bytFrameType, &blnOdd, &intNumCar, strMod, &intBaud, &intDataLen, &intRSLen, &bytQualThresh, strType);

	if (intDataLen == 0 || Length == 0 || !blnFrameTypeOK)
	{
		//Logs.Exception("[EncodeFSKFrameType] Failure to update parameters for frame type H" & Format(bytFrameType, "X") & "  DataToSend Len=" & bytDataToSend.Length.ToString)
		return 0;
	}
	
	//	Generate the 2 bytes for the frame type data:
	
	bytEncodedBytes[0] = bytFrameType;
	bytEncodedBytes[1] = bytFrameType ^ bytSessionID;

     //   Dim bytToRS(intDataLen + 3 - 1) As Byte ' Data + Count + 2 byte CRC

	bytDataToSendLengthPtr = 0;
	intEncodedDataPtr = 2;

	if (intBaud < 600 || intDataLen < 600)
	{
		// Now compute the RS frame for each carrier in sequence and move it to bytEncodedBytes 
		
		for (i = 0; i < intNumCar; i++)		//  across all carriers
		{
			intCarDataCnt = Length - bytDataToSendLengthPtr;
			
			if (intCarDataCnt >= intDataLen) // why not > ??
			{
				// Won't all fit 

				bytToRS[0] = intDataLen;
				intStartIndex = intEncodedDataPtr;
				memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intDataLen);
				bytDataToSendLengthPtr += intDataLen;
			}
			else
			{
				// Last bit

				bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

				if (intCarDataCnt > 0)
				{
					memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intCarDataCnt);
                    bytDataToSendLengthPtr += intCarDataCnt;
				}	
			}
		
			GenCRC16FrameType(bytToRS, intDataLen + 1, bytFrameType); // calculate the CRC on the byte count + data bytes

			RSEncode(bytToRS, bytToRS+intDataLen+3, intDataLen + 3, intRSLen);  // Generate the RS encoding ...now 14 bytes total

 			//  Need: (2 bytes for Frame Type) +( Data + RS + 1 byte byteCount + 2 Byte CRC per carrier)

 			intEncodedDataPtr += intDataLen + 3 + intRSLen;

			bytToRS += intDataLen + 3 + intRSLen;
		}
		return intEncodedDataPtr;
	}

	// special case for 600 baud 4FSK which has 600 byte data field sent as three sequencial (200 byte + 50 byte RS) groups

	for (i = 0; i < 3; i++)		 // for three blocks of RS data
	{
		intCarDataCnt = Length - bytDataToSendLengthPtr;
			
		if (intCarDataCnt >= intDataLen /3 ) // why not > ??
		{
			// Won't all fit 

			bytToRS[0] = intDataLen / 3;
			intStartIndex = intEncodedDataPtr;
			memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intDataLen / 3);
			bytDataToSendLengthPtr += intDataLen /3;
		}
		else
		{
			// Last bit

			bytToRS[0] = intCarDataCnt;  // Could be 0 if insuffient data for # of carriers 

			if (intCarDataCnt > 0)
			{
				memcpy(&bytToRS[1], &bytDataToSend[bytDataToSendLengthPtr], intCarDataCnt);
                bytDataToSendLengthPtr += intCarDataCnt;
			}	
		}
		GenCRC16FrameType(bytToRS, intDataLen / 3 + 1, bytFrameType); // calculate the CRC on the byte count + data bytes

 		RSEncode(bytToRS, bytToRS + intDataLen / 3 + 3, intDataLen / 3 + 3, intRSLen / 3);  // Generate the RS encoding ...now 14 bytes total
		intEncodedDataPtr += intDataLen / 3  + 3 + intRSLen / 3;
		bytToRS += intDataLen / 3  + 3 + intRSLen / 3;
	}		
	return intEncodedDataPtr;
}
	
//  Function to encode ConnectRequest frame 

BOOL EncodeARQConRequest(char * strMyCallsign, char * strTargetCallsign, enum _ARQBandwidth ARQBandwidth, UCHAR * bytReturn)
{
	//  Encodes a 4FSK 200 Hz BW Connect Request frame ( ~ 1950 ms with default leader/trailer)

	UCHAR * bytToRS= &bytReturn[2];

	if (strcmp(strTargetCallsign, "CQ") != 0)  // skip syntax checking for psuedo call "CQ"
	{
		if (!CheckValidCallsignSyntax(strMyCallsign))
		{
			//Logs.Exception("[EncodeModulate.EncodeARQConnectRequest] Illegal Call sign syntax. MyCallsign = " & strMyCallsign & ", TargetCallsign = " & strTargetCallsign)

			return 0;
		}
		if (!CheckValidCallsignSyntax(strTargetCallsign) || !CheckValidCallsignSyntax(strMyCallsign))
		{            
			//Logs.Exception("[EncodeModulate.EncodeARQConnectRequest] Illegal Call sign syntax. MyCallsign = " & strMyCallsign & ", TargetCallsign = " & strTargetCallsign)
			
			return 0;
		}
	}		
	if (ARQBandwidth == B200MAX)
		bytReturn[0] = 0x31;
	else if (ARQBandwidth == B500MAX)
		bytReturn[0] = 0x32;
	else if (ARQBandwidth == B1000MAX)
		bytReturn[0] = 0x33;
	else if (ARQBandwidth == B2000MAX)
		bytReturn[0] = 0x34;

	else if (ARQBandwidth == B200FORCED)
		bytReturn[0] = 0x35;
	else if (ARQBandwidth == B500FORCED)
		bytReturn[0] = 0x36;
	else if (ARQBandwidth == B1000FORCED)
		bytReturn[0] = 0x37;
	else if (ARQBandwidth == B2000FORCED)
		bytReturn[0] = 0x38;
	else
	{
		//Logs.Exception("[EncodeModulate.EncodeFSK500_1S] Bandwidth error.  Bandwidth = " & strBandwidth)
		return 0;
	}

	bytReturn[1] = bytReturn[0] ^ 0xFF;  // Connect Request always uses session ID of 0xFF

	// Modified May 24, 2015 to use RS instead of 2 byte CRC. (same as ID frame)

	CompressCallsign(strMyCallsign, &bytToRS[0]);
	CompressCallsign(strTargetCallsign, &bytToRS[6]);  //this uses compression to accept 4, 6, or 8 character Grid squares.

	RSEncode(bytToRS, &bytReturn[14], 12, 4);  // Generate the RS encoding ...now 14 bytes total
 
	return 18;
}


 
int Encode4FSKIDFrame(char * Callsign, char * Square, unsigned char * bytreturn)
{
	// Function to encodes ID frame 
	// returns length of encoded message 

	UCHAR * bytToRS= &bytreturn[2];

	 if (!CheckValidCallsignSyntax(Callsign))
	 {
		//       Logs.Exception("[EncodeModulate.EncodeIDFrame] Illegal Callsign syntax or Gridsquare length. MyCallsign = " & strMyCallsign & ", Gridsquare = " & strGridSquare)
		
		 return 0;
	 }

	bytreturn[0] = 0x30;
	bytreturn[1] = 0x30 ^ 0xFF;

	// Modified May 9, 2015 to use RS instead of 2 byte CRC.
       
	CompressCallsign(Callsign, &bytToRS[0]);

    if (Square[0])
		CompressGridSquare(Square, &bytToRS[6]);  //this uses compression to accept 4, 6, or 8 character Grid squares.

	RSEncode(bytToRS, &bytreturn[14], 12, 4);  // Generate the RS encoding ...now 14 bytes total

	return 18;
}

//  Funtion to encodes a short 4FSK 50 baud Control frame  (2 bytes total) BREAK, END, DISC, IDLE, ConRejBusy, ConRegBW  

int Encode4FSKControl(UCHAR bytFrameType, UCHAR bytSessionID, UCHAR * bytreturn)
{
	// Encodes a short control frame (normal length ~320 ms with default 160 ms leader+trailer) 
    
	//If IsShortControlFrame(intFrameCode) Then
    //        Logs.Exception("[EncodeModulate.EncodeFSKControl] Illegal control frame code: H" & Format(intFrameCode, "X"))
    //        return Nothing
    //    End If

	bytreturn[0] = bytFrameType;
	bytreturn[1] = bytFrameType ^ bytSessionID;

	return 2;		// Length
}

//  Function to encode a CONACK frame with Timing data  (6 bytes total)  

int EncodeConACKwTiming(UCHAR bytFrameType, int intRcvdLeaderLenMs, UCHAR bytSessionID, UCHAR * bytreturn)
{
	// Encodes a Connect ACK with one byte Timing info. (Timing info repeated 2 times for redundancy) 

	//If intFrameCode < 0x39 Or intFrameCode > 0x3C Then
    //        Logs.Exception("[EncodeConACKwTiming] Illegal Frame code: " & Format(intFrameCode, "X"))
    //        return Nothing
    //    End If

	UCHAR bytTiming = min(255, intRcvdLeaderLenMs / 10);  // convert to 10s of ms.

	if (intRcvdLeaderLenMs > 2550 || intRcvdLeaderLenMs < 0)
	{
		// Logs.Exception("[EncodeConACKwTiming] Timing value out of range: " & intRcvdLeaderLenMs.ToString & " continue with forced value = 0")
        bytTiming = 0;
	}

	bytreturn[0] = bytFrameType;
	bytreturn[1] = bytFrameType ^ bytSessionID;

	bytreturn[2] = bytTiming;
	bytreturn[3] = bytTiming;

	return 4;
}

//	' Function to encode an ACK control frame  (2 bytes total) ...with 5 bit Quality code 

int EncodeDATAACK(int intQuality, UCHAR bytSessionID, UCHAR * bytreturn)
{
	// Encodes intQuality and DataACK frame (normal length ~320 ms with default leader/trailer)

	int intScaledQuality;

	if (intQuality > 100)
		intQuality = 100;

	intScaledQuality = max(0, (intQuality / 2) - 19); // scale quality value to fit 5 bit field of 0 represents Q <= of 38 (pretty poor)
	
	bytreturn[0] = 0xE0 + intScaledQuality;		//ACKs 0xE0 - 0xFF
	bytreturn[1] = bytreturn[0] ^ bytSessionID;

	return 2;
}

//  Function to encode a NAK frame  (2 bytes total) ...with 5 bit Quality code 

int EncodeDATANAK(int intQuality , UCHAR bytSessionID, UCHAR * bytreturn)
{
	// Encodes intQuality and DataACK frame (normal length ~320 ms with default leader/trailer)

	int intScaledQuality;

	intScaledQuality = max(0, (intQuality / 2) - 19); // scale quality value to fit 5 bit field of 0 represents Q <= of 38 (pretty poor)
	
	bytreturn[0] = intScaledQuality;		// NAKS 00 - 0x1F
	bytreturn[1] = bytreturn[0] ^ bytSessionID;

	return 2;
}



void SendID(BOOL blnEnableCWID)
{
	unsigned char bytIDSent[80];
	int Len;

	// Schedular needs to ensure this isnt called if already playing

	if (SoundIsPlaying)
		return;

    if (GridSquare[0] == 0)
	{
		EncLen = Encode4FSKIDFrame(Callsign, "No GS", bytEncodedBytes);
		Len = sprintf(bytIDSent," %s:[No GS] ", Callsign);
	}
	else
	{
		EncLen = Encode4FSKIDFrame(Callsign, GridSquare, bytEncodedBytes);
		Len = sprintf(bytIDSent," %s:[%s] ", Callsign, GridSquare);
	}

	AddTagToDataAndSendToHost(bytIDSent, "IDF", Len);

	// On embedded platforms we don't have the memory to create full sound stream before playiong,
	// so this is structured differently from Rick's code

	Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], EncLen, 0);		// only returns when all sent

    if (blnEnableCWID)
		sendCWID(Callsign, FALSE);

}

// Function to generate a 5 second burst of two tone (1450 and 1550 Hz) used for setting up drive level
 
void ModTwoToneTest()
{
	GetTwoToneLeaderWithSync(500);
	SampleSink(0);	// 5 secs
	SoundFlush();
}


//  Subroutine to send 5 seconds of two tone

void Send5SecTwoTone()
{
	if (!SoundIsPlaying)
	{
//		CreateWaveStream(ModTwoToneTest());
	}
}



void  ASCIIto6Bit(char * Padded, UCHAR * Compressed)
{
	// Input must be 8 bytes which will convert to 6 bytes of packed 6 bit characters and
	// inputs must be the ASCII character set values from 32 to 95....
    
	unsigned long long intSum = 0;

	int i;

	for (i=0; i<4; i++)
	{
		intSum = (64 * intSum) + Padded[i] - 32;
	}

	Compressed[0] = (UCHAR)(intSum >> 16) & 255;
	Compressed[1] = (UCHAR)(intSum >> 8) &  255;
	Compressed[2] = (UCHAR)intSum & 255;

	intSum = 0;

	for (i=4; i<8; i++)
	{
		intSum = (64 * intSum) + Padded[i] - 32;
	}

	Compressed[3] = (UCHAR)(intSum >> 16) & 255;
	Compressed[4] = (UCHAR)(intSum >> 8) &  255;
	Compressed[5] = (UCHAR)intSum & 255;
}

void Bit6ToASCII(UCHAR * Padded, UCHAR * UnCompressed)
{
	// uncompress 6 to 8

	// Input must be 6 bytes which represent packed 6 bit characters that well 
	// result will be 8 ASCII character set values from 32 to 95...

	unsigned long long intSum = 0;

	int i;

	for (i=0; i<3; i++)
	{
		intSum = (intSum << 8) + Padded[i];
	}

	UnCompressed[0] = (UCHAR)((intSum >> 18) & 63) + 32;
	UnCompressed[1] = (UCHAR)((intSum >> 12) & 63) + 32;
	UnCompressed[2] = (UCHAR)((intSum >> 6) & 63) + 32;
	UnCompressed[3] = (UCHAR)(intSum & 63) + 32;

	intSum = 0;

	for (i=3; i<6; i++)
	{
		intSum = (intSum << 8) + Padded[i] ;
	}

	UnCompressed[4] = (UCHAR)((intSum >> 18) & 63) + 32;
	UnCompressed[5] = (UCHAR)((intSum >> 12) & 63) + 32;
	UnCompressed[6] = (UCHAR)((intSum >> 6) & 63) + 32;
	UnCompressed[7] = (UCHAR)(intSum & 63) + 32;
}


// Function to compress callsign (up to 7 characters + optional "-"SSID   (-0 to -15 or -A to -Z) 
    
void CompressCallsign(char * inCallsign, UCHAR * Compressed)
{
	char Callsign[10];
	char Padded[16];
	int SSID;
	char * Dash;

	memcpy(Callsign, inCallsign, 10);
	Dash = strchr(Callsign, '-');
	
	if (Dash == 0)		// if No SSID
	{
		strcpy(Padded, Callsign);
		strcat(Padded, "    ");
		Padded[7] = '0';			//  "0" indicates no SSID
	}
	else
	{
		*(Dash++) = 0;
		SSID = atoi(Dash);

		strcpy(Padded, Callsign);
		strcat(Padded, "    ");

		if (SSID >= 10)		// ' handles special case of -10 to -15 : ; < = > ? '
			Padded[7] = ':' + SSID - 10;
		else
			Padded[7] = *(Dash);
	}

	ASCIIto6Bit(Padded, Compressed); //compress to 8 6 bit characters   6 bytes total
}

// Function to compress Gridsquare (up to 8 characters)

void CompressGridSquare(char * Square, UCHAR * Compressed)
{
	char Padded[17];
        
	if (strlen(Square) > 8)
		return;

	strcpy(Padded, Square);
	strcat(Padded, "        ");

	ASCIIto6Bit(Padded, Compressed); //compress to 8 6 bit characters   6 bytes total
}

// Function to decompress 6 byte call sign to 7 characters plus optional -SSID of -0 to -15 or -A to -Z
  
void DeCompressCallsign(char * bytCallsign, char * returned)
{
	char bytTest[10] = "";
	char SSID[8] = "";
    
	Bit6ToASCII(bytCallsign, bytTest);

	memcpy(returned, bytTest, 6);
	returned[6] = 0;
	strlop(returned, ' ');		// remove trailing space

	if (bytTest[7] == '0') // Value of "0" so No SSID
		returned[6] = 0;
	else if (bytTest[7] >= 58 && bytTest[7] <= 63) //' handles special case for -10 to -15
		sprintf(SSID, "-%d", bytTest[7] - 48);
	else
		sprintf(SSID, "-%c", bytTest[7]);
	
	strcat(returned, SSID);
}


// Function to decompress 6 byte Grid square to 4, 6 or 8 characters

void DeCompressGridSquare(char * bytGS, char * returned)
{
	char bytTest[10] = "";
	Bit6ToASCII(bytGS, bytTest);

	strlop(bytTest, ' ');
	strcpy(returned, bytTest);
}

// A function to compute the parity symbol used in the frame type encoding

UCHAR ComputeTypeParity(UCHAR bytFrameType)
{
	UCHAR bytMask = 0xC0;
	UCHAR bytParitySum = 1;
	UCHAR bytSym = 0;
	int k;

	for (k = 0; k < 4; k++)
	{
		bytSym = (bytMask & bytFrameType) >> (2 * (3 - k));
		bytParitySum = bytParitySum ^ bytSym;
		bytMask = bytMask >> 2;
	}
    
	return bytParitySum & 0x3;
}

// Subroutine to make a CW ID Wave File

void sendCWID(char * Call, BOOL Play)
{
}

void CWID(char * strID, short * intSamples, BOOL blnPlay)
{
	// This generates a phase synchronous FSK MORSE keying of strID
	// FSK used to maintain VOX on some sound cards
	// Sent at 90% of  max ampllitude

    char strAlphabe[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/";

	// Look up table for strAlphabet...each bit represents one dot time, 3 adjacent dots = 1 dash
	// one dot spacing between dots or dashes
/*
        Dim intCW() As Integer = {0x17, 0x1D5, 0x75D, 0x75, 0x1, 0x15D, _
           0x1DD, 0x55, 0x5, 0x1777, 0x1D7, 0x175, _
           0x77, 0x1D, 0x777, 0x5DD, 0x1DD7, 0x5D, _
           0x15, 0x7, 0x57, 0x157, 0x177, 0x757, _
           0x1D77, 0x775, 0x77777, 0x17777, 0x5777, 0x1577, _
           0x557, 0x155, 0x755, 0x1DD5, 0x7775, 0x1DDDD, 0x1D57, 0x1D57}

        If strID.IndexOf("-") <> -1 Then
            ' strip off the -ssid for CWID
            strID = strID.Substring(0, strID.IndexOf("-"))
        End If

        Dim dblHiPhaseInc As float = 2 * PI * 1609.375 / 12000 ' 1609.375 Hz High tone
        Dim dblLoPhaseInc As float = 2 * PI * 1390.625 / 12000 ' 1390.625  low tone
        Dim dblHiPhase As float
        Dim dblLoPhase As float
        Dim intDotSampCnt As Integer = 768 ' about 12 WPM or so (should be a multiple of 256
        Dim intDot(intDotSampCnt - 1) As Int16
        Dim intSpace(intDotSampCnt - 1) As Int16

        ' Generate the dot samples (high tone) and space samples (low tone) 
        For i As Integer = 0 To intDotSampCnt - 1
            intSpace(i) = CShort(Sin(dblLoPhase) * 0.9 * intAmp)
            intDot(i) = CShort(Sin(dblHiPhase) * 0.9 * intAmp)
            dblHiPhase += dblHiPhaseInc
            If dblHiPhase > 2 * PI Then dblHiPhase -= 2 * PI
            dblLoPhase += dblLoPhaseInc
            If dblLoPhase > 2 * PI Then dblLoPhase -= 2 * PI
        Next i
        Dim intMask As Int32
        Dim intWav((6 * intDotSampCnt) - 1) As Int32
        ' Generate leader for VOX 6 dots long
        For k As Integer = 6 To 1 Step -1
            Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
        Next k

        For j As Integer = 0 To strID.Length - 1 ' for each character in the string
            intMask = 0x40000000
            Dim intIdx As Integer = strAlphabet.IndexOf(strID.ToUpper.Substring(j, 1))
            If intIdx = -1 Then
                ' process this as a space adding 6 dots worth of space to the wave file
                ReDim Preserve intWav(intWav.Length + (6 * intDotSampCnt) - 1)
                For k As Integer = 6 To 1 Step -1
                    Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
                Next k
            Else
                While intMask > 0 ' search for the first non 0 bit
                    If (intMask And intCW(intIdx)) <> 0 Then
                        Exit While ' intMask is pointing to the first non 0 entry
                    Else
                        intMask = intMask \ 2 ' Right shift mask
                    End If
                End While
                While intMask > 0
                    ReDim Preserve intWav(intWav.Length + intDotSampCnt - 1)
                    If (intMask And intCW(intIdx)) <> 0 Then
                        Array.Copy(intDot, 0, intWav, intWav.Length - intDotSampCnt, intDotSampCnt)
                    Else
                        Array.Copy(intSpace, 0, intWav, intWav.Length - intDotSampCnt, intDotSampCnt)
                    End If
                    intMask = intMask \ 2 ' Right shift mask
                End While
            End If
            ' add 3 dot spaces for inter letter spacing
            ReDim Preserve intWav(intWav.Length + (3 * intDotSampCnt) - 1)
            For k As Integer = 3 To 1 Step -1
                Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
            Next k
        Next
        ' add 3 spaces for the end tail
        ReDim Preserve intWav(intWav.Length + (3 * intDotSampCnt) - 1)
        For k As Integer = 3 To 1 Step -1
            Array.Copy(intSpace, 0, intWav, intWav.Length - k * intDotSampCnt, intDotSampCnt)
        Next k
        If Not blnPlay Then
            intSamples = intWav
            Exit Sub
        End If
        ' Convert the integer array to bytes
        Dim aryWave(2 * intWav.Length - 1) As Byte
        For j As Integer = 0 To intWav.Length - 1
            aryWave(2 * j) = CByte(intWav(j) And 0xFF) ' LSByte
            aryWave(1 + 2 * j) = CByte((intWav(j) And 0xFF00) \ 256) ' MSbyte
        Next j
        ' *********************************
        ' Debug code to look at wave file 
        'If IO.Directory.Exists(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav") = FALSE Then
        '    IO.Directory.CreateDirectory(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav")
        'End If
        'objWave.WriteRIFF(Application.ExecutablePath.Substring(0, Application.ExecutablePath.LastIndexOf("\")) & "\Wav\CWID.wav", 12000, 16, aryWave)
        '' End of debug code
        '************************************
        objWave.WriteRIFFStream(memWaveStream, 12000, 16, aryWave)
*/
}


// Function to look up the byte value from the frame string name

UCHAR FrameCode(char * strFrameName)
{
	int i;

    for (i = 0; i < 256; i++)
	{
		if (strcmp(strFrameType[i], strFrameName) == 0)
		{
			return i;
		}
	}
	return 0;
}

unsigned int GenCRC16(unsigned char * Data, unsigned short length)
{
	// For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
    // intSeed is the seed value for the shift register and must be in the range 0-0xFFFF

	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j < length; j++)	
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}
 
	return intRegister;
}

BOOL checkcrc16(unsigned char * Data, unsigned short length)
{
	int intRegister = 0xffff; //intSeed
	int i,j;
	int Bit;
	int intPoly = 0x8810;	//  This implements the CRC polynomial  x^16 + x^12 +x^5 + 1

	for (j = 0; j <  (length - 2); j++)		// ' 2 bytes short of data length
	{
		int Mask = 0x80;			// Top bit first

		for (i = 0; i < 8; i++)	// for each bit processing MS bit first
		{
			Bit = Data[j] & Mask;
			Mask >>= 1;

            if (intRegister & 0x8000)		//  Then ' the MSB of the register is set
			{
                // Shift left, place data bit as LSB, then divide
                // Register := shiftRegister left shift 1
                // Register := shiftRegister xor polynomial
                 
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
	
				intRegister = intRegister ^ intPoly;
			}
			else  
			{
				// the MSB is not set
                // Register is not divisible by polynomial yet.
                // Just shift left and bring current data bit onto LSB of shiftRegister
              if (Bit)
                 intRegister = 0xFFFF & (1 + (intRegister << 1));
			  else
                  intRegister = 0xFFFF & (intRegister << 1);
			}
		}
	}

    if (Data[length - 2] == intRegister >> 8)
		if (Data[length - 1] == (intRegister & 0xFF))
			return TRUE;
   
	return FALSE;
}


//	Subroutine to compute a 16 bit CRC value and append it to the Data... With LS byte XORed by bytFrameType
    
void GenCRC16FrameType(char * Data, int Length, UCHAR bytFrameType)
{
	unsigned int CRC = GenCRC16(Data, Length);

	// Put the two CRC bytes after the stop index

	Data[Length++] = (CRC >> 8);		 // MS 8 bits of Register
	Data[Length] = (CRC & 0xFF) ^ bytFrameType;  // LS 8 bits of Register
}

// Function to compute a 16 bit CRC value and check it against the last 2 bytes of Data (the CRC) XORing LS byte with bytFrameType..
 
unsigned short int compute_crc(unsigned char *buf,int len);

BOOL  CheckCRC16FrameType(unsigned char * Data, int Length, UCHAR bytFrameType)
{
	// returns TRUE if CRC matches, else FALSE
    // For  CRC-16-CCITT =    x^16 + x^12 +x^5 + 1  intPoly = 1021 Init FFFF
    // intSeed is the seed value for the shift register and must be in the range 0-0xFFFF

	unsigned int CRC = GenCRC16(Data, Length);
	unsigned short CRC2 =  compute_crc(Data, Length);
	CRC2 ^= 0xffff;
  
	// Compare the register with the last two bytes of Data (the CRC) 
    
	if ((CRC >> 8) == Data[Length])
		if (((CRC & 0xFF) ^ bytFrameType) == Data[Length + 1])
			return TRUE;

	return FALSE;
}

// Subroutine to get intDataLen bytes from outbound queue (bytDataToSend)

void ClearDataToSend()
{
	GetSemaphore();
	bytDataToSendLength = 0;
	FreeSemaphore();

	QueueCommandToHost("BUFFER 0");
}

void SaveQueueOnBreak()
{
	// Save data we are about to remove from TX buffer
}



#ifdef PTC

extern UCHAR bytEchoData[2048];		// has to be at least max packet size (?1280)

extern int bytesEchoed;

extern UCHAR DelayedEcho;

#endif


void RemoveDataFromQueue(int Len)
{
	char HostCmd[32];

	if (Len == 0)
		return;

	// Called when ACK received, or on FEC send

	//	If using PTC Serial Interface and delayed echo requested, send it

#ifdef PTC

	if (DelayedEcho == '1')
	{
		memcpy(bytEchoData, bytDataToSend, Len);
		bytesEchoed = Len;
	}

#endif

	GetSemaphore();

	if (Len > bytDataToSendLength)
		Len = bytDataToSendLength;			// Shouldn't happen, unless the Q is cleared

	bytDataToSendLength -= Len;

	if (bytDataToSendLength > 0)
		memmove(bytDataToSend, &bytDataToSend[Len], bytDataToSendLength);

	FreeSemaphore();

	sprintf(HostCmd, "BUFFER %d", bytDataToSendLength);
	QueueCommandToHost(HostCmd);
}

// Timer Rotines

void CheckTimers()
{
	//	Check for Timeout after a send that needs to be repeated

	if ((blnEnbARQRpt || blnDISCRepeating) && Now > dttNextPlay)
	{
		// No response Timeout

		if (GetNextFrame())
		{
			// I think this only returns TRUE if we have to repeat the last 

			//	Repeat mechanism for normal repeated FEC or ARQ frames
      
			WriteDebugLog("Repeating Last Frame");
			RemodulateLastFrame();
		}
		else
			// I think this means we have exceeded retries or had an abort

			blnEnbARQRpt = FALSE;
	}


	//  Event triggered by tmrSendTimeout elapse. Ends an ARQ session and sends a DISC frame 
    
	if (tmrSendTimeout && Now > tmrSendTimeout)
	{
		char HostCmd[80];

		// (Handles protocol rule 1.7)
       
		tmrSendTimeout = 0;

			//Dim dttStartWait As Date = Now
			//While objMain.blnLastPTT And Now.Subtract(dttStartWait).TotalSeconds < 10
			// Thread.Sleep(50)
			// End While

		WriteDebugLog("ARDOPprotocol.tmrSendTimeout]  ARQ Timeout from ProtocolState: %s Going to DISC state", ARDOPStates[ProtocolState]);
        
			// Confirmed proper operation of this timeout and rule 4.0 May 18, 2015
			// Send an ID frame (Handles protocol rule 4.0)
		
		EncLen = Encode4FSKIDFrame(strLocalCallsign, GridSquare, bytEncodedBytes);
		Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent
		dttLastFECIDSent = Now;
			
		if (AccumulateStats) LogStats();

		QueueCommandToHost("DISCONNECTED");
			
		sprintf(HostCmd, "STATUS ARQ Timeout from Protocol State:  %s", ARDOPStates[ProtocolState]);
		QueueCommandToHost(HostCmd);
		blnEnbARQRpt = FALSE;
		//Thread.Sleep(2000)
		ClearDataToSend();

		EncLen = Encode4FSKControl(0x29, bytSessionID, bytEncodedBytes);
		Mod4FSKDataAndPlay(0x29, &bytEncodedBytes[0], EncLen, LeaderLength);		// only returns when all sent

		intFrameRepeatInterval = 2000;
		SetARDOPProtocolState(DISC);
			
		InitializeConnection(); // reset all Connection data
				
			// Clear the mnuBusy status on the main form
			//Dim stcStatus As Status = Nothing
		    //stcStatus.ControlName = "mnuBusy"
			//stcStatus.Text = "FALSE"
		    //queTNCStatus.Enqueue(stcStatus)
        
		blnTimeoutTriggered = FALSE ;// prevents a retrigger
	}

	// Elapsed Subroutine for Pending timeout
   
	if (tmrIRSPendingTimeout && Now > tmrIRSPendingTimeout)
	{
		char HostCmd[80];
		
		tmrIRSPendingTimeout = 0;

		WriteDebugLog("ARDOPprotocol.tmrIRSPendingTimeout]  ARQ Timeout from ProtocolState: %s Going to DISC state",  ARDOPStates[ProtocolState]);

		QueueCommandToHost("DISCONNECTED");
		sprintf(HostCmd, "STATUS ARQ CONNECT REQUEST TIMEOUT FROM PROTOCOL STATE: %s",ARDOPStates[ProtocolState]);

		QueueCommandToHost(HostCmd);

		blnEnbARQRpt = FALSE;
		ProtocolState = DISC;
		blnPending = FALSE;
		InitializeConnection();	 // reset all Connection data

			//' Clear the mnuBusy status on the main form
		     //Dim stcStatus As Status = Nothing
			//stcStatus.ControlName = "mnuBusy"
			//stcStatus.Text = "FALSE"
			//	queTNCStatus.Enqueue(stcStatus)
	}

	// Subroutine for tmrFinalIDElapsed

	if (tmrFinalID && Now > tmrFinalID)
	{
		tmrFinalID = 0;
		
		WriteDebugLog("[ARDOPprotocol.tmrFinalID_Elapsed]  Send Final ID (%s, [%s])", strFinalIDCallsign, GridSquare);
   
		if (CheckValidCallsignSyntax(strFinalIDCallsign))
		{
			EncLen = Encode4FSKIDFrame(strFinalIDCallsign, GridSquare, bytEncodedBytes);
			Mod4FSKDataAndPlay(0x30, &bytEncodedBytes[0], 16, 0);		// only returns when all sent
			dttLastFECIDSent = Now;
		}
	}

	if (Now > tmrPollOBQueue)
	{
		char HostCmd[32];
		sprintf(HostCmd, "BUFFER %d", bytDataToSendLength);
		QueueCommandToHost(HostCmd);
	
		tmrPollOBQueue = Now + 10000;		// 10 Secs
	}
}

// Main polling Function returns True or FALSE if closing 

BOOL MainPoll()
{

	//   Dim stcStatus As Status = Nothing

   //     ' Check the sound card to insure still sampling
     //   If (Now.Subtract(dttLastSoundCardSample).TotalSeconds > 10) And objProtocol.GetARDOPProtocolState() <> ProtocolState.OFFLINE Then
       //     tmrStartCODEC.Interval = 1000
         //   dttLastSoundCardSample = Now
        //    Logs.Exception("[tmrPoll_Tick] > 10 seconds with no sound card samples...Restarting Codec")
          //  tmrStartCODEC.Start() ' Start the timer to retry starting sound card
       // End If
	
	// Checks to see if frame ready for playing

	if (!SoundIsPlaying && !blnEnbARQRpt && !blnDISCRepeating)		// Idle (check playing in case we call from txSleep())
	{
		if (GetNextFrame())
		{
			// As we will send the frame if one is available, and won't return
			// till it is all sent, I don't think I have to do anything here
		}
	}
//            If Not SoundIsPlaying Then
//               SendIDToolStripMenuItem.Enabled = objProtocol.GetARDOPSetARDOPProtocolState(ProtocolState.DISC
//         Else
     //           SendIDToolStripMenuItem.Enabled = FALSE
       //     End If
    
		/*
		' Update any form Main display items from the TNCStatus queue
            While queTNCStatus.Count > 0
                Try
                    stcStatus = CType(queTNCStatus.Dequeue, Status)
                    Select Case stcStatus.ControlName
                        ' Receive controls:
                        Case "lblQuality"
                            lblQuality.Text = stcStatus.Text
                        Case "ConstellationPlot"
                            DisplayPlot()
                            intRepeatCnt += 0
                        Case "lblXmtFrame"
                            lblXmtFrame.Text = stcStatus.Text
                            lblXmtFrame.BackColor = stcStatus.BackColor
                        Case "lblRcvFrame"
                            lblRcvFrame.Text = stcStatus.Text
                            lblRcvFrame.BackColor = stcStatus.BackColor
                        Case "prgReceiveLevel"
                            prgReceiveLevel.Value = stcStatus.Value
                            If stcStatus.Value < 64 Then ' < 12% of Full scale (16 bit A/D)
                                prgReceiveLevel.ForeColor = Color.SkyBlue
                            ElseIf stcStatus.Value > 170 Then ' > 88% of full scale (16 bit A/D)
                                prgReceiveLevel.ForeColor = Color.LightSalmon
                            Else
                                prgReceiveLevel.ForeColor = Color.LightGreen
                            End If
                        Case "lblOffset"
                            lblOffset.Text = stcStatus.Text
                        Case "lblHost"
                            If stcStatus.Text <> "" Then lblHost.Text = stcStatus.Text
                            lblHost.BackColor = stcStatus.BackColor
                        Case "lblState"
                            lblState.Text = stcStatus.Text
                            lblState.BackColor = stcStatus.BackColor
                        Case "lblCF"
                            lblCF.Text = stcStatus.Text
                        Case "mnuBusy"
                            If stcStatus.Text.ToUpper = "TRUE" Or stcStatus.Text.ToUpper = "FALSE" Then
                                ChannelBusyToolStripMenuItem.Text = "Channel Busy"
                                ChannelBusyToolStripMenuItem.Visible = CBool(stcStatus.Text)
                            Else
                                ChannelBusyToolStripMenuItem.Text = stcStatus.Text
                                ChannelBusyToolStripMenuItem.Visible = True
                            End If
                    End Select
                Catch
                    Logs.Exception("[Main.tmrPoll.Tick] queTNCStatus Err: " & Err.Description)
                    Exit While
                End Try
            End While
        */

	
	if	(blnClosing)	// Check for closing
		return FALSE;

	return TRUE;
}

int dttLastBusy;
int dttLastClear;
int dttStartRTMeasure;
int intLastStart;
int intLastStop;
float dblAvgBaselineSlow;
float dblAvgBaselineFast;
float dblAvgPk2BaselineRatio;

//  Functino to extract bandwidth from ARQBandwidth

int ExtractARQBandwidth()
{
	return atoi(ARQBandwidths[ARQBandwidth]);
}

//	 Function to implement a busy detector based on 1024 point FFT
 /*
BOOL BusyDetect(float * dblMag, int intStart, int intStop)
{
	// this only called while searching for leader ...once leader detected, no longer called.
	// First look at simple peak over the frequency band of  interest.
	// Status:  May 28, 2014.  Some initial encouraging results. But more work needed.
	//       1) Use of Start and Stop ranges good and appear to work well ...may need some tweaking +/_ a few bins.
	//       2) Using a Fast attack and slow decay for the dblAvgPk2BaselineRation number e.g.
	//       dblAvgPk2BaselineRatio = Max(dblPeakPwrAtFreq / dblAvgBaselineX, 0.9 * dblAvgPk2BaselineRatio)
	// Seems to work well for the narrow detector. Tested on real CW, PSK, RTTY. 
	// Still needs work on the wide band detector. (P3, P4 etc)  May be issues with AGC speed. (my initial on-air tests using AGC Fast).
	// Ideally can find a set of parameters that won't require user "tweaking"  (Busy Squelch) but that is an alternative if needed. 
	// use of technique of re initializing some parameters on a change in detection bandwidth looks good and appears to work well with 
	// scanning.  Could be expanded with properties to "read back" these parameters so they could be saved and re initialize upon each new frequency. 

	static int intBusyCountPkToBaseline = 0;
	static int intBusyCountFastToSlow = 0;
	static int intBusyCountSlowToFast = 0;
	static BOOL blnLastBusy = FALSE;

	float dblAvgBaseline = 0;
	float dblPwrAtPeakFreq = 0;
	float dblAvgBaselineX;
	float dblAlphaBaselineSlow = 0.1f; // This factor affects the filtering of baseline slow. smaller = slower filtering
	float dblAlphaBaselineFast = 0.5f; // This factor affects the filtering of baseline fast. smaller = slower filtering
	int intPkIndx = 0;
	float dblFSRatioNum, dblSFRatioNum;
	BOOL  blnFS, blnSF, blnPkBaseline;
	int i;

	// This holds off any processing of data until 100 ms after PTT release to allow receiver recovery.
      
	if (Now - dttStartRTMeasure < 100)
		return blnLastBusy;

	for (i = intStart; i <= intStop; i++)	 // cover a range that matches the bandwidth expanded (+/-) by the tuning range
	{
		if (dblMag[i] > dblPwrAtPeakFreq)
		{
			dblPwrAtPeakFreq = dblMag[i];
			intPkIndx = i;
		}
		dblAvgBaseline += dblMag[i];
	}
     
	if (intPkIndx == 0)
		return 0;
	
	// add in the 2 bins above and below the peak (about 59 Hz total bandwidth)
	// This needs refinement for FSK mods like RTTY which have near equal peaks making the Peak and baseline on strong signals near equal
	// Computer the power within a 59 Hz spectrum around the peak

	dblPwrAtPeakFreq += (dblMag[intPkIndx - 2] + dblMag[intPkIndx - 1]) + (dblMag[intPkIndx + 2] + dblMag[intPkIndx + 1]);
	dblAvgBaselineX = (dblAvgBaseline - dblPwrAtPeakFreq) / (intStop - intStart - 5);  // the avg Pwr per bin ignoring the 59 Hz area centered on the peak
	dblPwrAtPeakFreq = dblPwrAtPeakFreq / 5;  //the the average Power (per bin) in the region of the peak (peak +/- 2bins...about 59 Hz)

	if (intStart == intLastStart && intStop == intLastStop)
	{
		dblAvgPk2BaselineRatio = dblPwrAtPeakFreq / dblAvgBaselineX;
		dblAvgBaselineSlow = (1 - dblAlphaBaselineSlow) * dblAvgBaselineSlow + dblAlphaBaselineSlow * dblAvgBaseline;
		dblAvgBaselineFast = (1 - dblAlphaBaselineFast) * dblAvgBaselineFast + dblAlphaBaselineFast * dblAvgBaseline;
	}
	else
	{
		// This initializes the values after a bandwidth change

		dblAvgPk2BaselineRatio = dblPwrAtPeakFreq / dblAvgBaselineX;
		dblAvgBaselineSlow = dblAvgBaseline;
		dblAvgBaselineFast = dblAvgBaseline;
		intLastStart = intStart;
		intLastStop = intStop;
	}
	
	if (Now - dttLastBusy < 1000 ||  ProtocolState != DISC)	// Why??
		return blnLastBusy;
	
	if (dblAvgPk2BaselineRatio > 1.118f * powf(Squelch, 1.5f))   // These values appear to work OK but may need optimization April 21, 2015
	{
		blnPkBaseline = TRUE;
		dblAvgBaselineSlow = dblAvgBaseline;
		dblAvgBaselineFast = dblAvgBaseline;
	}
	else
	{
       // 'If intBusyCountPkToBaseline > 0 Then

		blnPkBaseline = FALSE;
	}
	
	// This detects wide band "pulsy" modes like Pactor 3, MFSK etc

	switch(Squelch)		 // this provides a modest adjustment to the ratio limit based on practical squelch values
	{
		//These values yield less sensiivity for F:S which minimizes trigger by static crashes but my need further optimization May 2, 2015

	case 0:
	case 1:
	case 2:
		dblFSRatioNum = 1.9f;
		dblSFRatioNum = 1.2f;
		break;
		
	case 3:
		dblFSRatioNum = 2.1f;
		dblSFRatioNum = 1.4f;
		break;
	case 4:
		dblFSRatioNum = 2.3f;
		dblSFRatioNum = 1.6f;
		break;
	case 5:
		dblFSRatioNum = 2.5f;
		dblSFRatioNum = 1.8f;
		break;
	case 6:
		dblFSRatioNum = 2.7f;
		dblSFRatioNum = 2.0f;
	case 7:
		dblFSRatioNum = 2.9f;
		dblSFRatioNum = 2.2f;
	case 8:
	case 9:
	case 10:
        dblFSRatioNum = 3.1f;
		dblSFRatioNum = 2.4f;
	}
	
	// This covers going from Modulation to no modulation e.g. End of Pactor frame

	if ((dblAvgBaselineSlow / dblAvgBaselineFast) > dblSFRatioNum)
	
		//Debug.WriteLine("     Slow to Fast")
		blnSF = TRUE;
	else
		blnSF = FALSE;
	
	//  This covers going from no modulation to Modulation e.g. Start of Pactor Frame or Static crash
	
	if ((dblAvgBaselineFast / dblAvgBaselineSlow) > dblFSRatioNum)
         //Debug.WriteLine("     Fast to Slow")
		blnFS = TRUE;
	else
		blnFS = FALSE;

	if (blnFS || blnSF || blnPkBaseline)
	{
		//'If blnFS Then Debug.WriteLine("Busy: Fast to Slow")
		//'If blnSF Then Debug.WriteLine("Busy: Slow to Fast")
		//'If blnPkBaseline Then Debug.WriteLine("Busy: Pk to Baseline")
		blnLastBusy = TRUE;
		dttLastBusy = Now;
		return TRUE;
	}
	else
	{
		blnLastBusy = FALSE;
		dttLastClear = Now;
        return FALSE;
	}
	return blnLastBusy;
}
*/
//	Subroutine to update the Busy detector when not displaying Spectrum or Waterfall (graphics disabled)
 		
int LastBusyCheck = 0;

void UpdateBusyDetector(short * bytNewSamples)
{
	float dblReF[1024];
	float dblImF[1024];

	float dblMag[206];
	
	static BOOL blnLastBusyStatus;
	
	float dblMagAvg = 0;
	int intTuneLineLow, intTuneLineHi, intDelta;
	int i;

	if (ProtocolState != DISC)		// ' Only process busy when in DISC state
		return;

	if (State != SearchingForLeader)
		return;						// only when looking for leader

	if (Now - LastBusyCheck < 100)
		return;

	LastBusyCheck = Now;

	FourierTransform(1024, bytNewSamples, &dblReF[0], &dblImF[0], FALSE);

	for (i = 0; i <  206; i++)
	{
		//	starting at ~300 Hz to ~2700 Hz Which puts the center of the signal in the center of the window (~1500Hz)
            
		dblMag[i] = powf(dblReF[i + 25], 2) + powf(dblImF[i + 25], 2);	 // first pass 
		dblMagAvg += dblMag[i];
	}
	intDelta = (ExtractARQBandwidth() / 2 + TuningRange) / 11.719f;

	intTuneLineLow = max((103 - intDelta), 3);
	intTuneLineHi = min((103 + intDelta), 203);
    
//	if (ProtocolState == DISC)		// ' Only process busy when in DISC state
	{
		blnBusyStatus = BusyDetect2(dblMag, intTuneLineLow, intTuneLineHi);
		
		if (blnBusyStatus && !blnLastBusyStatus)
		{
			QueueCommandToHost("BUSY TRUE");
         	newStatus = TRUE;				// report to PTC
		}
		//    stcStatus.Text = "True"
            //    queTNCStatus.Enqueue(stcStatus)
            //    'Debug.WriteLine("BUSY TRUE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))
			
		else if (blnLastBusyStatus && !blnBusyStatus)
		{
			QueueCommandToHost("BUSY FALSE");
			newStatus = TRUE;				// report to PTC
		} 
		//    stcStatus.Text = "False"
        //    queTNCStatus.Enqueue(stcStatus)
        //    'Debug.WriteLine("BUSY FALSE @ " & Format(DateTime.UtcNow, "HH:mm:ss"))

		blnLastBusyStatus = blnBusyStatus;
	}
}

unsigned const short CRCTAB[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 
0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 
0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 
0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 
0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 
0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 
0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 
0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 
0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 
0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 
0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 
0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 
0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 
0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 
0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 
0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 
0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 
0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 
0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 
0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 
0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 
0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 
0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 
0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 
0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 
0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 
0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78 
}; 


unsigned short int compute_crc(unsigned char *buf,int len)
{
	unsigned short fcs = 0xffff; 
	int i;

	for(i = 0; i < len; i++) 
		fcs = (fcs >>8 ) ^ CRCTAB[(fcs ^ buf[i]) & 0xff]; 

	return fcs;
}
