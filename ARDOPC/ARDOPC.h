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

#include <windows.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <math.h>

typedef int BOOL;
typedef unsigned char UCHAR;

#define FALSE 0
#define TRUE 1

UCHAR FrameCode(char * strFrameName);
BOOL FrameInfo(UCHAR bytFrameType, int * blnOdd, int * intNumCar, char * strMod,
   int * intBaud, int * intDataLen, int * intRSLen, UCHAR * bytQualThres, char * strType);

int EncodeFSKData(UCHAR bytFrameType, UCHAR * bytDataToSend, int Length, unsigned char * bytEncodedData);
int Encode4FSKIDFrame(char * Callsign, char * Square, unsigned char * bytReturn);
void Mod4FSKDataAndPlay(int Type, unsigned char * bytEncodedData, int Len, int intLeaderLen);
void FSXmtFilter200_1500Hz(short * intNewSamples, int Length);
void FSXmtFilter500_1500Hz(short * intNewSamples, int Length);
SampleSink(UCHAR * Samples, int Count);
SoundFlush();
int AddTrailer(short * intSamples, int Length);
void CWID(char * strID, short * intSamples, BOOL blnPlay);
void sendCWID(char * Call, BOOL Play);
UCHAR ComputeTypeParity(UCHAR bytFrameType);
void GenCRC16FrameType(char * Data, int intStartIndex, int intStopIndex, UCHAR bytFrameType);

int GetDataFromQueue(UCHAR * Data, int MaxLen);
void GetSemaphore();
void FreeSemaphore();



VOID __cdecl Debugprintf(const char * format, ...);



enum ReceiveState		// used for initial receive testing...later put in correct protocol states
{
	SearchingForLeader,
	AcquireSymbolSync,
	AcquireFrameSync,
	AcquireFrameType,
	DecodeFrameType,
	AcquireFrame
};

enum ReceiveState State;

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

enum _ARDOPState ProtocolState;
enum _ARDOPState ARDOPState;

char GridSquare[7];
char Callsign[10];
BOOL wantCWID;
int LeaderLength;
int TrailerLength;

time_t Now;
