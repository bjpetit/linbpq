
#include "resource.h"

#pragma pack(1)
#pragma pack()


typedef struct TNCINFO
{ 
	int WINMORtoBPQ_Q;			// Frames for BPQ, indexed by BPQ Port
	int BPQtoWINMOR_Q;			// Frames for WINMOR. indexed by WINMOR port. Only used it TCP session is blocked

	SOCKET WINMORSock;			// Control Socket
	SOCKET WINMORDataSock;		// Data Socket

	char * WINMORSignon;		// Pointer to message for secure signin
	char * WINMORHostName;		// WINMOR Host - may be dotted decimal or DNS Name
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	char * InitScript;			// Initialisation Commands

    UCHAR TCPBuffer[1000];		// For converting byte stream to messages
    int InputLen;				// Data we have alreasdy = Offset of end of an incomplete packet;

	BOOL Connected;				// When set, all data is passed to Data Socket
	BOOL Connecting;
	BOOL Disconnecting;			// Disconnect Sent - waiting for DISCONNECTED or Timout
	int	DiscTimeout;			// Disconnect Timeout counter.
	BOOL Attached;				// Set what attached to a BPQ32 stream
	BOOL StartSent;				// Codec Start send (so will get a disconnect)
	BOOL ConnectPending;		// Set if Connect Pending Received. If so, mustn't allow freq change.

	BOOL FECMode;				// In FEC Mode
	BOOL FEC1600;				// Use 1600 Hz FEC Mode
	int FECIDTimer;				// Time in FEC Mode. Used to trigger ID broadcasts

	int Busy;					// Channel Busy Flags
#define CDBusy 1
#define PTTBusy 2

	BOOL FECPending;			// Need an FEC Send when channel is next idle

	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

	time_t lasttime;

	BOOL CONNECTING;			// TCP Session Flags
	BOOL CONNECTED;
	BOOL Alerted;				// COnnect Failed Prompt sent
	BOOL DATACONNECTING;
	BOOL DATACONNECTED;

	char MyCall[10]	;				// Call we are using
	char RemoteCall[10];			// Callsign
	char NodeCall[10];				// Call we listen for (PORTCALL or NODECALL
	char CurrentMYC[10];			// Save current call so we don't change it unnecessarily

	SOCKADDR_IN destaddr;
	SOCKADDR_IN Datadestaddr;

	struct _EXTPORTDATA * PortRecord;
	struct RIGINFO * RIG;			// Pointer to Rig Control RIG record (For PTT)
	int PTTMode;					// PTT Mode Flags

	int WIMMORPID;
	char * CaptureDevices;
	char * PlaybackDevices;
	char * ProgramPath;

	int Restarts;					// TNC Kill/Restarts done
	time_t LastRestart;
	
	int TimeSinceLast;				// Time since last message from TNC (10ths of a sec)

	int Interlock;					// Port Interlock Group

	HWND hDlg;						// Status Window Handle
	HMENU hPopMenu;					// Actions Menu Handle

};

