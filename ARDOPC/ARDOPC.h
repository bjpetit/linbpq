// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#define ProductName "ARDOP TNC"
#define ProductVersion "0.4.0-BPQ"

//	Sound interface buffer size

#define SendSize 1200		// 100 mS for now

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#ifndef WIN32
#define max(x, y) ((x) > (y) ? (x) : (y))
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#ifdef M_PI
#undef M_PI
#endif

#define M_PI       3.141592f

#ifndef TARGET_STM32F4
#ifndef WIN32
#define LINUX
#endif
#endif


#ifdef TARGET_STM32F4
#define Now ticks
extern volatile int ticks;
#else
#define Now getTicks()
#endif
#include "ecc.h"				// RS Constants

typedef int BOOL;
typedef unsigned char UCHAR;

#define VOID void

#define FALSE 0
#define TRUE 1

BOOL KeyPTT(BOOL State);

UCHAR FrameCode(char * strFrameName);
BOOL FrameInfo(UCHAR bytFrameType, int * blnOdd, int * intNumCar, char * strMod,
   int * intBaud, int * intDataLen, int * intRSLen, UCHAR * bytQualThres, char * strType);

void ClearDataToSend();
int EncodeFSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedBytes);
int EncodePSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedBytes);
int Encode4FSKIDFrame(char * Callsign, char * Square, unsigned char * bytreturn);
int EncodeDATAACK(int intQuality, UCHAR bytSessionID, UCHAR * bytreturn);
int EncodeDATANAK(int intQuality , UCHAR bytSessionID, UCHAR * bytreturn);
void Mod4FSKDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen);
void Mod4FSK600BdDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen);
void Mod16FSKDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen);
void Mod8FSKDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen);
void ModPSKDataAndPlay(int Type, unsigned char * bytEncodedBytes, int Len, int intLeaderLen);
BOOL IsDataFrame(UCHAR intFrameType);
BOOL CheckValidCallsignSyntax(char * strTargetCallsign);
void StartCodec(char * strFault);
void StopCodec(char * strFault);
//void SetARDOPProtocolState(int value);
BOOL SendARQConnectRequest(char * strMycall, char * strTargetCall);
void AddDataToDataToSend(UCHAR * bytNewData, int Len);
BOOL StartFEC(UCHAR * bytData, int Len, char * strDataMode, int intRepeats, BOOL blnSendID);
void SendID(BOOL blnEnableCWID);
BOOL CheckGSSyntax(char * GS);

unsigned int getTicks();
void txSleep(int mS);
void rxSleep(int mS);

//exern "C" void SampleSink(short Sample);
//extern "C" void SoundFlush();
//extern "C" void SetFilter(void * Filter());

void SampleSink(short Sample);
void SoundFlush();
void StopCapture();
void StartCapture();

void SetFilter(void * Filter());

void AddTrailer();
void CWID(char * strID, short * intSamples, BOOL blnPlay);
void sendCWID(char * Call, BOOL Play);
UCHAR ComputeTypeParity(UCHAR bytFrameType);
void GenCRC16FrameType(char * Data, int Length, UCHAR bytFrameType);
BOOL CheckCRC16FrameType(unsigned char * Data, int Length, UCHAR bytFrameType);
char * strlop(char * buf, char delim);
void QueueCommandToHost(char * Cmd);

#ifdef WIN32
void ProcessNewSamples(short * Samples, int nSamples);
VOID Debugprintf(const char * format, ...);
void ardopmain();
BOOL GetNextFECFrame();
void GenerateFSKTemplates();
void printtick(char * msg);
void InitValidFrameTypes();
#endif

BOOL DemodDecode4FSKID(UCHAR bytFrameType, char * strCallID, char * strGridSquare);
void DeCompressCallsign(char * bytCallsign, char * returned);
void DeCompressGridSquare(char * bytGS, char * returned);

int RSEncode(UCHAR * bytToRS, UCHAR * bytRSEncoded, int MaxErr, int Len);
BOOL RSDecode(UCHAR * bytRcv, int Length, int CheckLen, BOOL * blnRSOK);

void ProcessRcvdFECDataFrame(int intFrameType, UCHAR * bytData, BOOL blnFrameDecodedOK);
void ProcessUnconnectedConReqFrame(int intFrameType, UCHAR * bytData);
void ProcessRcvdARQFrame(UCHAR intFrameType, UCHAR * bytData, int DataLen, BOOL blnFrameDecodedOK);
void InitializeConnection();

void AddTagToDataAndSendToHost(UCHAR * Msg, char * Type, int Len);

void RemoveDataFromQueue(int Len);
void RemodulateLastFrame();

void GetSemaphore();
void FreeSemaphore();
const char * Name(UCHAR bytID);
void InitSound();
void initFilter(int Width);
void FourierTransform(int NumSamples, short * RealIn, float * RealOut, float * ImagOut, int InverseTransform);

enum _ReceiveState		// used for initial receive testing...later put in correct protocol states
{
	SearchingForLeader,
	AcquireSymbolSync,
	AcquireFrameSync,
	AcquireFrameType,
	DecodeFrameType,
	AcquireFrame
};

extern enum _ReceiveState State;

enum _ARQBandwidth
{
	B200FORCED,
	B500FORCED,
	B1000FORCED,
	B2000FORCED,
	B200MAX,
	B500MAX,
	B1000MAX,
	B2000MAX
};

extern enum _ARQBandwidth ARQBandwidth;

extern const char ARQBandwidths[8][12];


enum _ARDOPState
{
	OFFLINE,
	DISC,
	ISS,
	IRS,
	IDLE,
	FECSend,
	FECRcv,
};

extern enum _ARDOPState ProtocolState;

extern const char ARDOPStates[7][8];


// Enum of ARQ Substates

enum _ARQSubStates
{
	None,
	ISSConReq,
	ISSConAck,
	ISSData,
	ISSId,
	IRSConAck,
	IRSData,
	IRSBreak,
	IRSfromISS,
	DISCArqEnd
};

extern enum _ARQSubStates ARQState;

enum _ProtocolMode
{
	Undef,
	FEC,
	ARQ
};

extern enum _ProtocolMode ProtocolMode;

extern enum _ARQSubStates ARQState;

struct SEM
{
	unsigned int Flag;
	int Clashes;
	int	Gets;
	int Rels;
};

extern struct SEM Semaphore;

extern short intTwoToneLeaderTemplate[120];  // holds just 1 symbol (10 ms) of the leader

extern short intPSK100bdCarTemplate[9][4][120];	// The actual templates over 9 carriers for 4 phase values and 120 samples
    //   (only positive Phase values are in the table, sign reversal is used to get the negative phase values) This reduces the table size from 7680 to 3840 integers
extern short intPSK200bdCarTemplate[9][4][72];		// Templates for 200 bd with cyclic prefix
extern short intFSK25bdCarTemplate[16][480];		// Template for 16FSK carriers spaced at 25 Hz, 25 baud
extern short intFSK50bdCarTemplate[4][240];		// Template for 4FSK carriers spaced at 50 Hz, 50 baud
extern short intFSK100bdCarTemplate[20][120];		// Template for 4FSK carriers spaced at 100 Hz, 100 baud
extern short intFSK600bdCarTemplate[4][20];		// Template for 4FSK carriers spaced at 600 Hz, 600 baud  (used for FM only)

// Config Params
extern char GridSquare[7];
extern char Callsign[10];
extern BOOL wantCWID;
extern int LeaderLength;
extern int TrailerLength;
extern int ARQTimeout;
extern int TuningRange;
extern BOOL DebugLog;
extern int ARQConReqRepeats;
extern BOOL CommandTrace;
extern char strFECMode[];
extern char CaptureDevice[];
extern char PlaybackDevice[];
extern int port;
extern BOOL RadioControl;



extern char * CaptureDevices;
extern char * PlaybackDevices;

extern int dttCodecStarted;
extern int dttStartRTMeasure;

extern int intCalcLeader;        // the computed leader to use based on the reported Leader Length

extern const char strFrameType[256][16];
extern BOOL Capturing;
extern BOOL SoundIsPlaying;
extern int blnLastPTT;
extern BOOL blnAbort;
extern BOOL blnClosing;
extern BOOL blnCodecStarted;
extern BOOL blnInitializing;
extern BOOL blnARQDisconnect;
extern int DriveLevel;
extern int FECRepeats;
extern BOOL FECId;
extern int Squelch;
extern BOOL blnEnbARQRpt;
extern int dttNextPlay;

extern UCHAR bytDataToSend[];
extern int bytDataToSendLength;

extern BOOL blnListen;

extern unsigned char bytEncodedBytes[1800];
extern int EncLen;

extern char AuxCalls[10][10];
extern int AuxCallsLength;

extern int bytValidFrameTypesLength;

extern BOOL blnTimeoutTriggered;
extern int intFrameRepeatInterval;
extern BOOL PlayComplete;

extern const UCHAR bytValidFrameTypes[];

extern const char strAllDataModes[][14];
extern int strAllDataModesLen;

// RS Variables

extern int MaxCorrections;

// Has to follow enum defs

BOOL EncodeARQConRequest(char * strMyCallsign, char * strTargetCallsign, enum _ARQBandwidth ARQBandwidth, UCHAR * bytReturn);

