
#include <winioctl.h>
#include "resource.h"
#include "resrc1.h"


#define MAXBLOCK 4096

struct RIGINFO
{
//	struct TRANSPORTENTRY * AttachedSession;

	int BPQtoRADIO_Q;			// Frames for PACTOR

	int BPQPort;				// Port this radio is attached to.
	struct _EXTPORTDATA * PortRecord; // BPQ32 port record for this port

	UCHAR RigAddr;
	BOOL Scanning;				// Scanning enabled
	int ScanCounter;
	int PollCounter;			// Don't poll too often;
	int ScanFreq;				// Scan Rate

	BOOL OKtoChange;			// Can Change
	BOOL WaitingForPermission;	//

	BOOL RIGOK;					// RIG is reponding

	int Session;				// BPQ L4 Record Number
	UCHAR Mode;					// Save to send after getting freq ack
	UCHAR Filter;

	char RigName[10];

	// Frequency list is a block of Set Freq/Mode commands in link format, null terminated

	char * FreqList;
	char * FreqPtr;

	HWND hLabel;
	HWND hCAT;
	HWND hFREQ;
	HWND hMODE;
	HWND hSCAN;

};

#define ICOM 1
#define YAESU 2
#define KENWOOD 3

struct PORTINFO
{
	int PortType;				// ICOM, Yaesu, Etc
	int IOBASE;
	int SPEED;
	struct RIGINFO Rigs[10];	// Rigs off a port
	char * InitPtr;				// Next Command
	int CmdSent;				// Last Command sent
	int CmdStream;				// Stream last command was issued on
//	int	IntCmdDelay;			// To limit internal commands
	int	ConfiguredRigs;			// Radios on this interface
	int CurrentRig;				// Radio last accessed.

	int Timeout;				// Timeout response counter
	int Retries;
	BOOL PORTOK;				// PORT is reponding

	HANDLE hDevice;					// COM device Handle
	UCHAR TXBuffer[500];			// Last message sent - saved for Retry
	int TXLen;						// Len of last sent
	UCHAR RXBuffer[500];			// Message being received - may not arrive all at once
	int RXLen;						// Data in RXBUffer
	HWND hStatus;
	BOOL AutoPoll;					// set if last command was a Timer poll 

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
