// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

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

#define M_PI       3.141592f

#include "ecc.h"				// RS Constants

typedef int BOOL;
typedef unsigned char UCHAR;

#define VOID void

#define FALSE 0
#define TRUE 1

void KeyPTT(BOOL State);


UCHAR FrameCode(char * strFrameName);
BOOL FrameInfo(UCHAR bytFrameType, int * blnOdd, int * intNumCar, char * strMod,
   int * intBaud, int * intDataLen, int * intRSLen, UCHAR * bytQualThres, char * strType);

void ClearDataToSend();
int EncodeFSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedData);
int Encode4FSKIDFrame(char * Callsign, char * Square, unsigned char * bytReturn);
void Mod4FSKDataAndPlay(int Type, unsigned char * bytEncodedData, int Len, int intLeaderLen);
void FSXmtFilter200_1500Hz(short * intNewSamples, int Length, short * PriorSamples, short * Filtered);
void FSXmtFilter500_1500Hz(short * intNewSamples, int Length);
//extern "C" void SampleSink(short Sample);
//extern "C" void SoundFlush();
//extern "C" void SetFilter(void * Filter());

void SampleSink(short Sample);
void SoundFlush();
void SetFilter(void * Filter());

void AddTrailer();
void CWID(char * strID, short * intSamples, BOOL blnPlay);
void sendCWID(char * Call, BOOL Play);
UCHAR ComputeTypeParity(UCHAR bytFrameType);
void GenCRC16FrameType(char * Data, int Length, UCHAR bytFrameType);
BOOL CheckCRC16FrameType(unsigned char * Data, int Length, UCHAR bytFrameType);
char * strlop(char * buf, char delim);

#ifdef WIN32
void ProcessNewSamples(short * Samples, int nSamples);
VOID Debugprintf(const char * format, ...);
void ardopmain();
#endif

BOOL DemodDecode4FSKID(UCHAR bytFrameType, char * strCallID, char * strGridSquare);
void DeCompressCallsign(char * bytCallsign, char * returned);
void DeCompressGridSquare(char * bytGS, char * returned);

int RSEncode(UCHAR * bytToRS, UCHAR * bytRSEncoded, int MaxErr, int Len);
BOOL RSDecode(UCHAR * bytRcv, int Length, int CheckLen, BOOL * blnRSOK);

void AddTagToDataAndSendToHost(char * Msg, char * Type);

int GetDataFromQueue(UCHAR * Data, int MaxLen);
void GetSemaphore();
void FreeSemaphore();
const char * Name(UCHAR bytID);

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
extern enum _ARDOPState ARDOPState;

extern short intTwoToneLeaderTemplate[120];  // holds just 1 symbol (10 ms) of the leader

extern short intPSK100bdCarTemplate[9][4][120];	// The actual templates over 9 carriers for 4 phase values and 120 samples
    //   (only positive Phase values are in the table, sign reversal is used to get the negative phase values) This reduces the table size from 7680 to 3840 integers
extern short intPSK200bdCarTemplate[9][4][72];		// Templates for 200 bd with cyclic prefix
extern short intFSK25bdCarTemplate[16][480];		// Template for 16FSK carriers spaced at 25 Hz, 25 baud
extern short intFSK50bdCarTemplate[4][240];		// Template for 4FSK carriers spaced at 50 Hz, 50 baud
extern short intFSK100bdCarTemplate[20][120];		// Template for 4FSK carriers spaced at 100 Hz, 100 baud
extern short intFSK600bdCarTemplate[4][20];		// Template for 4FSK carriers spaced at 600 Hz, 600 baud  (used for FM only)


extern char GridSquare[7];
extern char Callsign[10];
extern BOOL wantCWID;
extern int LeaderLength;
extern int TrailerLength;

extern char ProtocolMode[4];

extern time_t Now;
extern UCHAR bytValidFrameTypes[256];

extern int bytValidFrameTypesLength;

extern BOOL blnTimeoutTriggered;


// RS Variables

extern int MaxCorrections;
