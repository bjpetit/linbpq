#pragma once

#include "resource.h"

struct ConnectionInfo
{ 
	int ComPort;
//	char PortType;
	int RTS;
	int CTS;
	int DCD;
	int DTR;
	int DSR;
    int BPQPort;
	int HostPort;
	char Params[20];				// Init Params (eg 9600,n,8)
	char PortLabel[20];
	char TypeFlag[2];
	HANDLE hDevice;
	BOOL Created;
	BOOL PortEnabled;
	int FLOWCTRL;
	BOOL KHOST;						// Set if in Kantronics Host Mode
};


typedef struct _SERIAL_STATUS {
    ULONG Errors;
    ULONG HoldReasons;
    ULONG AmountInInQueue;
    ULONG AmountInOutQueue;
    BOOLEAN EofReceived;
    BOOLEAN WaitForImmediate;
} SERIAL_STATUS,*PSERIAL_STATUS;

