#pragma once

#include "resource.h"

struct PortInfo
{ 
	BOOL ApplPort;
	int Index;
	int ComPort;
	char PortType[2];
	BOOL NewVCOM;				// Using User Mode Virtual COM Driver
	int ReopenTimer;			// Retry if open failed delay
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
	int LastError;
	int FLOWCTRL;
	int gpsinptr;
	OVERLAPPED Overlapped;
	OVERLAPPED OverlappedRead;
	char GPSinMsg[10000];
	BOOL LoopAppls;						// Loop frames to other appls
	char IntoOutMap[17];				// TNC Channel to Appl Channel Map
	char OuttoInMap[17];				// Appl Channel to TNC Channel Map

	// Fields for Polled KISS Operation

	BOOL Polled;
	BOOL CheckSum;
	BOOL AckMode;
	UINT KISS_Q;						// Frames to Send 

	int PollTimer;						// Timeout
	UINT PollMask;						// Bit Mask of ports to Poll
	UINT PollLimit;						// Highest to Poll
	int NextPoll;						// Next to Poll

};


typedef struct _SERIAL_STATUS {
    ULONG Errors;
    ULONG HoldReasons;
    ULONG AmountInInQueue;
    ULONG AmountInOutQueue;
    BOOLEAN EofReceived;
    BOOLEAN WaitForImmediate;
} SERIAL_STATUS,*PSERIAL_STATUS;

// TCP Support (for GPS via ActiveSync)

struct tcp_table_entry
{
	union
	{
		struct in_addr in_addr;
		unsigned int ipaddr;
	};
	unsigned short port;
	unsigned char hostname[64];
	unsigned int error;
	SOCKET TCPListenSock;			// Listening socket if slave
	SOCKET TCPSock;
	int  TCPMode;				// TCPMaster ot TCPSlave
	UCHAR * TCPBuffer;			// Area for building TCP message from byte stream
	int InputLen;				// Bytes in TCPBuffer
	SOCKADDR_IN sin; 
	SOCKADDR_IN destaddr;
	BOOL TCPState;
	UINT TCPThreadID;			// Thread ID if TCP Master
	UINT TCPOK; 				// Cleared when Message RXed . Incremented by timer
};

struct SatInfo
{
	int PRN;					// Sat Number
	int Az;
	int Ele;
	int SNR;
	BOOL InView;
	BOOL Inuse;
	BOOL DGPS;
};

