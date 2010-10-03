
#include <winioctl.h>
#include "kernelresource.h"

#define MAXBLOCK 4096

#include "TNCINFO.h"
/*
struct STREAMINFO
{
//	struct TRANSPORTENTRY * AttachedSession;

	int PACTORtoBPQ_Q;			// Frames for BPQ
	int BPQtoPACTOR_Q;			// Frames for PACTOR
	int	FramesOutstanding;		// Frames Queued - used for flow control
	BOOL InternalCmd;			// Last Command was generated internally
	int	IntCmdDelay;			// To limit internal commands

	BOOL Attached;					// Set what attached to a BPQ32 stream
	BOOL Connected;				// When set, all data is passed as data instead of commands
	BOOL Connecting;			// Set when Outward Connect in progress
	BOOL ReportDISC;			// Need to report an incoming DISC to kernel

	int Timeout;				// Timeout response counter

	char MyCall[10]	;				// Call we are using
	char RemoteCall[10];			// Callsign

	int BytesTXed;
	int BytesAcked;
	int BytesRXed;
	int BytesOutstanding;		// For Packet Channels
};



struct TNCINFO
{
	HWND hDlg;						// Status Window Handle
	struct _EXTPORTDATA * PortRecord; // BPQ32 port record for this port
	struct RIGINFO * RIG;			// Pointer to Rig Control RIG record 
	char * InitScript;			// Initialisation Commands
	struct STREAMINFO Streams[27];	// 0 is Pactor on HF port, Rest are ax.25 on VHF port.
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	char * InitPtr;				// Next Command
	int	ReinitState;			// Reinit State Machine
	BOOL TNCOK;					// TNC is reponding
	BOOL InternalCmd;			// Last Command was generated internally
	int CmdStream;				// Stream last command was issued on
	int	IntCmdDelay;			// To limit internal commands


	char NodeCall[10];				// Call we listen for (PORTCALL or NODECALL

	HANDLE hDevice;
	BOOL HostMode;					// Set if in DED Host Mode
	BOOL NeedPACTOR;				// Set if need to send PACTOR to put into Standby Mode
	BOOL OldMode;					// Use PACTOR instead of TOR (for old software)
	int Timeout;					// Timeout response counter
//	int Retries;
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
	UCHAR RXBuffer[500];			// Message being received - may not arrive all at once
	int RXLen;						// Data in RXBUffer

	int Mem1;						// Free Bytes (VHF /HF)
	int Mem2;

	BOOL HFPacket;					// Set if HF port is in Packet mode instead of Pactor Mode

	int TimeInRX;					// Time waiting for ISS before sending
	char TXRXState;					// Current ISS/IRS State

	
	char * RigConfigMsg;			// Message to pass to rig control

};

*/



#define IOCTL_SERIAL_IS_COM_OPEN CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GETDATA     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x801,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SETDATA     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x802,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_SET_CTS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x803,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DSR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x804,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DCD     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x805,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_SERIAL_CLR_CTS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x806,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DSR     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x807,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DCD     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x808,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_BPQ_ADD_DEVICE     CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x809,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_BPQ_DELETE_DEVICE  CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80a,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define W98_SERIAL_GETDATA     0x801
#define W98_SERIAL_SETDATA     0x802
