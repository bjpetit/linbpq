#pragma once

#include "resource.h"

struct ConnectionInfo
{ 
	int ComPort;
	BOOL DEDMode;			// True for DED mode, False for Kant mode
	BOOL SCSMode;			// True for SCS varient of DED
	BOOL Term4Mode;			// Used by Airmail
	BOOL Toggle;				// SCS Sequence Toggle
	BOOL InHostMode;
	int numChannels;
	int ApplMask;
	BOOL nextMode;			// Mode to enter following a reset
	int RTS;
	int CTS;
	int DCD;
	int DTR;
	int DSR;
	char Params[20];				// Init Params (eg 9600,n,8)
	char PortLabel[20];

	HANDLE hDevice;
	BOOL Created;
	BOOL PortEnabled;
	struct StreamInfo * Channels[27];

	UCHAR RXBuffer[1000];		// Need to build a complete packet
	int	RXBPtr;

	BOOL Echo;

	char MYCall[11];			// 

	// DED Mode Fields

	unsigned char LINEBUFFER[300];		// MSG FROM PC APPL
	unsigned char * CURSOR;				// Pointer into
	unsigned char PCBUFFER[512];		//	BUFFER TO PC APPLICATION
	unsigned char * RXPOINTER;
	unsigned char * PUTPTR;
	int RXCOUNT;

	unsigned char MONBUFFER[258]; //="\x6";
	int MONLENGTH;
	int MONFLAG;


	BOOL Recovering;

	char ECHOFLAG;		// ECHO ENABLED
//	char MODE;				// INITIALLY TERMINAL MODE
	char HOSTSTATE;		// HOST STATE MACHINE 
	int MSGCOUNT;			// LENGTH OF MESSAGE EXPECTED
	int MSGLENGTH;
	char MSGTYPE;
	char MSGCHANNEL;

};

struct StreamInfo
{ 
	UCHAR * Chan_TXQ;		//	!! Leave at front so ASM Code Finds it
							// FRAMES QUEUED TO NODE
    int BPQStream;
	BOOL Connected;					// Set if connected to Node
	UCHAR MYCall[30];
};

typedef struct _SERIAL_STATUS {
    ULONG Errors;
    ULONG HoldReasons;
    ULONG AmountInInQueue;
    ULONG AmountInOutQueue;
    BOOLEAN EofReceived;
    BOOLEAN WaitForImmediate;
} SERIAL_STATUS,*PSERIAL_STATUS;

#define Disconnect(stream) SessionControl(stream,2,0)
#define Connect(stream) SessionControl(stream,1,0)
