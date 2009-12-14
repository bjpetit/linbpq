
#include <winioctl.h>
#include "resource.h"
#include "resrc1.h"


#define MAXBLOCK 4096

struct STREAMINFO
{
//	struct TRANSPORTENTRY * AttachedSession;

	int PACTORtoBPQ_Q;			// Frames for BPQ
	int BPQtoPACTOR_Q;			// Frames for PACTOR

	BOOL Attached;				// Set what attached to a BPQ32 stream
	int BPQPort;				// Port this radio is attached to.
	UCHAR RigAddr;
	BOOL Scanning;				// Scanning enabled
	int ScanCounter;
	int PollCounter;			// Don't poll too often;
	int ScanFreq;				//

	BOOL TNCOK;					// TNC is reponding

	int Session;				// BPQ L4 Record
	UCHAR Mode;					// Save to send after getting freq ack
	UCHAR Filter;

	char RigName[10];

	// Frequency list is a block of Set Freq/Mode commands in link format, null terminated

	char * FreqList;
	char * FreqPtr;

};



struct TNCINFO
{
	int IOBASE;
	int SPEED;
	struct STREAMINFO Streams[27];	// 0 is Pactor on HF port, Rest are ax.25 on VHF port.
	char * ApplCmd;				// Application to connect to on incoming connect (null = leave at command handler)
	char * InitScript;			// Initialisation Commands
	char * InitPtr;				// Next Command
	int	ReinitState;			// Reinit State Machine
	BOOL InternalCmd;			// Last Command was generated internally
	int CmdStream;				// Stream last command was issued on
	int	IntCmdDelay;			// To limit internal commands
	int	MaxStreams;				// Radios on this interface
	int CurrentStream;			// Radio last accessed.

	int Timeout;				// Timeout response counter
	int Retries;
	BOOL TNCOK;					// TNC is reponding

	char NodeCall[10];				// Call we listen for (PORTCALL or NODECALL

	HANDLE hDevice;
	BOOL HostMode;					// Set if in DED Host Mode
	BOOL NeedPACTOR;				// Set if need to send PACTOR to put into Standby Mode
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
	UCHAR RXBuffer[500];			// Message being received - may not arrive all at once
	int RXLen;						// Data in RXBUffer

	HWND hDlg;						// Status Window Handle
};



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
