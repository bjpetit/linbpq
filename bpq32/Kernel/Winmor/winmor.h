
#include "resource.h"

#pragma pack(1)
#pragma pack()


typedef struct TNCINFO
{ 
	int WINMORtoBPQ_Q;			// Frames for BPQ, indexed by BPQ Port
	int BPQtoWINMOR_Q;			// Frames for WINMOR. indexed by WINMOR port. Only used it TCP session is blocked

	SOCKET WINMORSock;			// Control Socket, indexed by BPQ Port
	SOCKET WINMORDataSock;		// Data Socket, indexed by BPQ Port

	char * WINMORSignon;		// Pointer to message for secure signin
	char * WINMORHostName;		// WINMOR Host - may be dotted decimal or DNS Name
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	char * InitScript;			// Initialisation Commands

    UCHAR TCPBuffer[1000];		// For converting byte stream to messages
    int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

	BOOL Connected;				// When set, all data is passed to Data Socket
	BOOL Connecting;
	BOOL Attached;				// Set what attached to a BPQ32 stream
	BOOL StartSent;				// COdec Start send (so will get a disconnect)

	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

	time_t lasttime;

	BOOL CONNECTING;			// TCP Session Flags
	BOOL CONNECTED;
	BOOL DATACONNECTING;
	BOOL DATACONNECTED;

	char MyCall[10]	;				// Call we are using
	char RemoteCall[10];			// Callsign
	char NodeCall[10];				// Call we listen for (PORTCALL or NODECALL

	SOCKADDR_IN destaddr;
	SOCKADDR_IN Datadestaddr;

	struct _EXTPORTDATA * PortRecord;

	HWND hDlg;						// Status Window Handle


};

