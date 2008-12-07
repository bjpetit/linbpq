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
	struct StreamInfo * Channels[6];
	int numChannels;
	int ApplMask;

	UCHAR RXBuffer[1000];		// Need to build a complete packet
	int	RXBPtr;

	BOOL Echo;
};

struct StreamInfo
{ 
    int BPQPort;
	BOOL Connected;					// Set if connected to Node
};

typedef struct _SERIAL_STATUS {
    ULONG Errors;
    ULONG HoldReasons;
    ULONG AmountInInQueue;
    ULONG AmountInOutQueue;
    BOOLEAN EofReceived;
    BOOLEAN WaitForImmediate;
} SERIAL_STATUS,*PSERIAL_STATUS;

