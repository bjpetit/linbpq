#pragma once

#include "resource.h"

struct ConnectionInfo
{ 
	int ComPort;
	BOOL InHostMode;
	BOOL nextMode;			// Mode to enter following a reset
	int RTS;
	int CTS;
	int DCD;
	int DTR;
	int DSR;
	char PortLabel[20];
	HANDLE hDevice;
	BOOL Created;
	BOOL PortEnabled;
	struct StreamInfo * Channels[27];
	int numChannels;
	int ApplMask;

	UCHAR RXBuffer[1000];		// Need to build a complete packet
	int	RXBPtr;

	BOOL Echo;
};

struct StreamInfo
{ 
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
