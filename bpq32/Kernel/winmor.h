
#include "kernelresource.h"

#pragma pack(1)
#pragma pack()

struct WL2KInfo
{
	char * Freq;
	char Bandwidth;
	char * TimeList;		// eg 06-10,12-15
};

#define MAXFREQS 20

typedef struct TNCINFO
{ 
	int Hardware;				// Hardware Type

#define H_WINMOR 1
#define H_SCS 2
#define H_KAM 3
#define H_AEA 4
#define H_HAL 5

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
	BOOL DiscPending;			// Set if Disconnect Pending Received. So we can time out stuck in Disconnecting
	int NeedDisc;				// Timer to send DISC if appl not available
	BOOL HadConnect;				// Flag to say have been in session
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

	char TargetCall[10];			// Call incoming connect is addressed to (for appl call support)

	SOCKADDR_IN destaddr;
	SOCKADDR_IN Datadestaddr;

	struct _EXTPORTDATA * PortRecord;
	struct RIGINFO * RIG;			// Pointer to Rig Control RIG record (For PTT)
	char * RigConfigMsg;			// Message to pass to rig control

	int PTTMode;					// PTT Mode Flags

	int WIMMORPID;
	char * CaptureDevices;
	char * PlaybackDevices;
	char * ProgramPath;

	int Restarts;					// TNC Kill/Restarts done
	time_t LastRestart;
	
	int TimeSinceLast;				// Time since last message from TNC (10ths of a sec)
	int HeartBeat;

	int Interlock;					// Port Interlock Group

	HWND hDlg;						// Status Window Handle
	HWND hMonitor;					// Handle to Monitor control
	HMENU hPopMenu;					// Actions Menu Handle

	BOOL OverrideBusy;
	int BusyDelay;					// Timer for busy timeout
	char * ConnectCmd;				// Saved command if waiting for busy to clear

	// Fields for reporting to WL2K Map

	char * Host;
	short Port;

	int UpdateWL2KTimer;
	BOOL UpdateWL2K;
	char RMSCall[10];
	char BaseCall[10];
	char GridSquare[7];
	char Comment[80];
	BOOL UseRigCtrlFreqs;
	char WL2KFreq[12];
	char WL2KMode;

	struct WL2KInfo WL2KInfoList[MAXFREQS];		// Freqs for sending to WL2K

};

