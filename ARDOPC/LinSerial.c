// ARDOP TNC Host Interface
//
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <winioctl.h>
#else
#define HANDLE int
#endif

#include "ARDOPC.h"

VOID ProcessSCSPacket(UCHAR * rxbuffer, int Length);
VOID EmCRCStuffAndSend(UCHAR * Msg, int Len);
WriteCOMBlock(hDevice, Message, MsgLen);
int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength);


UCHAR bytDataToSend[4096];// =
	//	"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n"
	//	"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n"
	//	"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n"
	//	"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n"
	//	"HelloHelloAAAABBBBCCCCDDDD\r\nHelloHelloHelloHelloHello\rHelloHelloHelloHelloHelloHelloHello\r\n";


// May malloc this, or change to cyclic buffer

int bytDataToSendLength = 0;

char ReportCall[10];

UCHAR bytDataforHost[2048];		// has to be at least max packet size (?1280)

int bytesforHost = 0;

UCHAR bytEchoData[2048];		// has to be at least max packet size (?1280)

int bytesEchoed = 0;

UCHAR DelayedEcho = 0;

UCHAR SCSReply[256 + 5];	// could be smaller??

extern int port;
int ComPort = 35;
int Speed;
int PollDelay;
BOOL MODE;
int Toggle;

char MYCALL[10];

BOOL Term4Mode;
BOOL PACMode;

BOOL NewVCOM;
BOOL PortEnabled;
HANDLE hDevice;
BOOL VCOM;

char PORTNAME[80] = "com35";

int Mode;
int CTS, DTR, RTS;

BOOL TNCRUNNING;

#define SCS 1

int RXBPtr;

int change = 0;			// Flag for connect/disconnect reports
int state = 0;

int DataChannel = 31;
int ReplyLen;

extern float dblOffsetHz;
extern int intSessionBW;

void QueueCommandToHost(char * Cmd)
{
	if (memcmp(Cmd, "STATUS ", 7) == 0)
	{
		if (memcmp(&Cmd[7], "CONNECT TO", 10) == 0)
		{
			memcpy(ReportCall, &Cmd[18], 10);
			strlop(ReportCall, ' ');
			change = 1;
			state = 0;
		}
	}
	if (memcmp(Cmd, "CONNECTED ", 10) == 0)
	{
		memcpy(ReportCall, &Cmd[10], 10);
		strlop(ReportCall, ' ');
		change = 1;
		state = 1;
	}

	if (memcmp(Cmd, "DISCON", 6) == 0)
	{
		change = 1;
		state = 0;
	}

	Debugprintf("Command to Host %s", Cmd);
}

void SendCommandToHost(char * Cmd)
{
	// if possible convert to equivalent PTC message

	if (memcmp(Cmd, "STATUS CONNECT TO", 20) == 0)
	{
		change = 1;
		state = 0;
	}

	Debugprintf("Command to Host %s", Cmd);
}

void AddTagToDataAndSendToHost(UCHAR * Msg, char * Type, int Len)
{
	if (strcmp(Type, "ARQ") == 0)
	{
		memcpy(&bytDataforHost[bytesforHost], Msg, Len);
		bytesforHost += Len;
	}
	else
	{
		Msg[Len] = 0;
		Debugprintf("RX Data %s %s", Type, Msg);
	}
}

BOOL CheckStatusChange()
{
	if (change == 1)
	{
		change = 0;

		if (state == 1)
		{
			// Connected

			SCSReply[2] = DataChannel;
			SCSReply[3] = 3;
			ReplyLen  = sprintf(&SCSReply[4], "(%d) CONNECTED to %s", DataChannel, ReportCall);
			ReplyLen += 5;
			EmCRCStuffAndSend(SCSReply, ReplyLen);

			return TRUE;
		}
		// Disconnected
		
		SCSReply[2] = DataChannel;
		SCSReply[3] = 3;
		ReplyLen  = sprintf(&SCSReply[4], "(%d) DISCONNECTED fm %s", DataChannel, ReportCall);
		ReplyLen += 5;		// Include Null
		EmCRCStuffAndSend(SCSReply, ReplyLen);

		return TRUE;

	}

	return FALSE;

}

BOOL CheckForData()
{
	int Length;

	if (bytesEchoed)
	{
		// Echo back acked data

		if (bytesEchoed > 256)
			Length = 256;
		else
			Length = bytesEchoed;

		memcpy(&SCSReply[5], bytEchoData, Length);

		bytesEchoed -= Length;

		if (bytesEchoed)
			memmove(bytEchoData, &bytEchoData[Length], bytesEchoed);

		SCSReply[2] = DataChannel;
		SCSReply[3] = 8;		// PTC Special - delayed echoed data
		SCSReply[4] = Length - 1;

		ReplyLen = Length + 5;
		EmCRCStuffAndSend(SCSReply, ReplyLen);

		return TRUE;
	}

	if (bytesforHost == 0)
		return FALSE;

	if (bytesforHost > 256)
		Length = 256;
	else
		Length = bytesforHost;

	memcpy(&SCSReply[5], bytDataforHost, Length);

	bytesforHost -= Length;

	if (bytesforHost)
		memmove(bytDataforHost, &bytDataforHost[Length], bytesforHost);

	SCSReply[2] = DataChannel;
	SCSReply[3] = 7;
	SCSReply[4] = Length - 1;

	ReplyLen = Length + 5;
	EmCRCStuffAndSend(SCSReply, ReplyLen);

	return TRUE;
}






// SCS Emulator


int BPQSerialSetPollDelay(HANDLE hDevice, int PollDelay);

// BPQ Serial Device Support

#ifdef WIN32

#define IOCTL_SERIAL_SET_BAUD_RATE      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 1,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_QUEUE_SIZE     CTL_CODE(FILE_DEVICE_SERIAL_PORT, 2,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_LINE_CONTROL   CTL_CODE(FILE_DEVICE_SERIAL_PORT, 3,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_BREAK_ON       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 4,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_BREAK_OFF      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 5,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_IMMEDIATE_CHAR     CTL_CODE(FILE_DEVICE_SERIAL_PORT, 6,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 7,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_TIMEOUTS       CTL_CODE(FILE_DEVICE_SERIAL_PORT, 8,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_DTR            CTL_CODE(FILE_DEVICE_SERIAL_PORT, 9,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_DTR            CTL_CODE(FILE_DEVICE_SERIAL_PORT,10,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_RESET_DEVICE       CTL_CODE(FILE_DEVICE_SERIAL_PORT,11,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_RTS            CTL_CODE(FILE_DEVICE_SERIAL_PORT,12,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_CLR_RTS            CTL_CODE(FILE_DEVICE_SERIAL_PORT,13,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_XOFF           CTL_CODE(FILE_DEVICE_SERIAL_PORT,14,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_XON            CTL_CODE(FILE_DEVICE_SERIAL_PORT,15,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_WAIT_MASK      CTL_CODE(FILE_DEVICE_SERIAL_PORT,16,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_WAIT_MASK      CTL_CODE(FILE_DEVICE_SERIAL_PORT,17,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_WAIT_ON_MASK       CTL_CODE(FILE_DEVICE_SERIAL_PORT,18,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_PURGE              CTL_CODE(FILE_DEVICE_SERIAL_PORT,19,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_BAUD_RATE      CTL_CODE(FILE_DEVICE_SERIAL_PORT,20,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_LINE_CONTROL   CTL_CODE(FILE_DEVICE_SERIAL_PORT,21,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_CHARS          CTL_CODE(FILE_DEVICE_SERIAL_PORT,22,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_CHARS          CTL_CODE(FILE_DEVICE_SERIAL_PORT,23,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_HANDFLOW       CTL_CODE(FILE_DEVICE_SERIAL_PORT,24,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_SET_HANDFLOW       CTL_CODE(FILE_DEVICE_SERIAL_PORT,25,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_MODEMSTATUS    CTL_CODE(FILE_DEVICE_SERIAL_PORT,26,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_COMMSTATUS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,27,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_XOFF_COUNTER       CTL_CODE(FILE_DEVICE_SERIAL_PORT,28,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_PROPERTIES     CTL_CODE(FILE_DEVICE_SERIAL_PORT,29,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_DTRRTS         CTL_CODE(FILE_DEVICE_SERIAL_PORT,30,METHOD_BUFFERED,FILE_ANY_ACCESS)


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
#define IOCTL_BPQ_LIST_DEVICES   CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80b,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define	IOCTL_BPQ_SET_POLLDELAY	 CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80c,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define	IOCTL_BPQ_SET_DEBUGMASK	 CTL_CODE(FILE_DEVICE_SERIAL_PORT,0x80d,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define W98_SERIAL_IS_COM_OPEN 0x800
#define W98_SERIAL_GETDATA     0x801
#define W98_SERIAL_SETDATA     0x802

#define W98_SERIAL_SET_CTS     0x803
#define W98_SERIAL_SET_DSR     0x804
#define W98_SERIAL_SET_DCD     0x805

#define W98_SERIAL_CLR_CTS     0x806
#define W98_SERIAL_CLR_DSR     0x807
#define W98_SERIAL_CLR_DCD     0x808

#define W98_BPQ_ADD_DEVICE     0x809
#define W98_BPQ_DELETE_DEVICE  0x80a
#define W98_BPQ_LIST_DEVICES   0x80b

#define	W98_BPQ_SET_POLLDELAY	 0x80c
#define	W98_BPQ_SET_DEBUGMASK	 0x80d

#define W98_SERIAL_GET_COMMSTATUS    27
#define W98_SERIAL_GET_DTRRTS        30

#define DebugModemStatus 1
#define DebugCOMStatus 2
#define DebugWaitCompletion 4
#define DebugReadCompletion 8

HANDLE hControl;

typedef struct _SERIAL_STATUS {
    unsigned long Errors;
    unsigned long HoldReasons;
    unsigned long AmountInInQueue;
    unsigned long AmountInOutQueue;
    BOOL EofReceived;
    BOOL WaitForImmediate;
} SERIAL_STATUS,*PSERIAL_STATUS;

#endif

#ifdef LINUX

//	#include <pty.h>

HANDLE LinuxOpenPTY(char * Name)
{            
	// Open a Virtual COM Port

	HANDLE hDevice, slave;;
	char slavedevice[80];
	int ret;
	u_long param=1;
	struct termios term;

#ifdef MACBPQ

	// Create a pty pair
	
	openpty(&hDevice, &slave, &slavedevice[0], NULL, NULL);
	close(slave);

#else
	 
	hDevice = posix_openpt(O_RDWR|O_NOCTTY);

	if (hDevice == -1 || grantpt (hDevice) == -1 || unlockpt (hDevice) == -1 ||
		 ptsname_r(hDevice, slavedevice, 80) != 0)
	{
		perror("Create PTY pair failed");
		return -1;
	} 

#endif

	Debugprintf("slave device: %s. ", slavedevice);
 
	if (tcgetattr(hDevice, &term) == -1)
	{
		perror("tty_speed: tcgetattr");
		return FALSE;
	}

	cfmakeraw(&term);

	if (tcsetattr(hDevice, TCSANOW, &term) == -1)
	{
		perror("tcsetattr");
		return -1;
	}

	ioctl(hDevice, FIONBIO, &param);

	chmod(slavedevice, S_IRUSR|S_IRGRP|S_IWUSR|S_IWGRP|S_IROTH|S_IWOTH);

	unlink (Name);
		
	ret = symlink (slavedevice, Name);
		
	if (ret == 0)
		printf ("symlink to %s created", Name);
	else
		printf ("symlink to %s failed", Name);	
	
	return hDevice;
}
#endif


#ifdef WIN32

HANDLE BPQOpenSerialPort(DWORD * lasterror)
{
	// Open a Virtual COM Port

	int port = ComPort;
	char szPort[80];
	HANDLE hDevice;
	int Err;

	*lasterror=0;

	// Try New Style VCOM firsrt

	sprintf( szPort, "\\\\.\\pipe\\BPQCOM%d", port ) ;

	hDevice = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );

	Err = GetLastError();

	if (hDevice != (HANDLE) -1)
	{
		NewVCOM = TRUE;
		PortEnabled = TRUE;
		Err = GetFileType(hDevice);
	}
		  
	if (hDevice == (HANDLE) -1 )
	{
		*lasterror=GetLastError();
	}

   return hDevice;
}


int BPQSerialSetCTS(HANDLE hDevice)
{
	unsigned long bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
}

int BPQSerialSetDSR(HANDLE hDevice)
{
	unsigned long bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_DSR, NULL,0,NULL,0, &bytesReturned,NULL);
}

int BPQSerialSetDCD(HANDLE hDevice)
{
	unsigned long bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
}

int BPQSerialClrCTS(HANDLE hDevice)
{
	unsigned long bytesReturned;
	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
              
}
int BPQSerialClrDSR(HANDLE hDevice)
{
	unsigned long bytesReturned;
	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DSR,NULL,0,NULL,0, &bytesReturned,NULL);            
}

int BPQSerialClrDCD(HANDLE hDevice)
{
	unsigned long bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DCD, NULL,0,NULL,0, &bytesReturned,NULL);                 
}

#endif

int BPQSerialSendData(UCHAR * Message,int MsgLen)
{
	unsigned long bytesReturned;

	// Host Mode code calls BPQSerialSendData for all ports, so it a real port, pass on to real send routine

	if (!VCOM)
		return WriteCOMBlock(hDevice, Message, MsgLen);

#ifdef LINUX
	
	// Linux usies normal IO for all ports
	return WriteCOMBlock(hDevice, Message, MsgLen);

#endif

#ifdef WIN32

	if (MsgLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
		if (NewVCOM)
		{
			// Have to escape all oxff chars, as these are used to get status info 

			UCHAR NewMessage[1000];
			UCHAR * ptr1 = Message;
			UCHAR * ptr2 = NewMessage;
			UCHAR c;

			int Length = MsgLen;

			while (Length != 0)
			{
				c = *(ptr1++);
				*(ptr2++) = c;

				if (c == 0xff)
				{
					*(ptr2++) = c;
					MsgLen++;
				}
				Length--;
			}

			return WriteFile(hDevice, NewMessage, MsgLen, &bytesReturned, NULL);
		}
		else
			return DeviceIoControl(hDevice,IOCTL_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
#endif
}

int BPQSerialGetData(UCHAR * Message, unsigned int BufLen, unsigned long * MsgLen)
{
#ifdef WIN32
	DWORD dwLength = 0;
	DWORD Available = 0;
	int Length, RealLen = 0;

	if (BufLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	if (NewVCOM)
	{
		int ret = PeekNamedPipe(hDevice, NULL, 0, NULL, &Available, NULL);

		if (ret == 0)
		{
			ret = GetLastError();

			if (ret == ERROR_BROKEN_PIPE)
			{
				CloseHandle(hDevice);
				hDevice = INVALID_HANDLE_VALUE;
				return 0;
			}
		}

		if (Available > BufLen)
			Available = BufLen;
		
		if (Available)
		{
			UCHAR * ptr1 = Message;
			UCHAR * ptr2 = Message;
			UCHAR c;
			
			ReadFile(hDevice, Message, Available, &Length, NULL);

			// Have to look for FF escape chars

			RealLen = Length;

			while (Length != 0)
			{
				c = *(ptr1++);
				Length--;

				if (c == 0xff)
				{
					c = c = *(ptr1++);
					Length--;
					
					if (c == 0xff)			// ff ff means ff
					{
						RealLen--;
					}
					else
					{
						// This is connection statua from other end

						RealLen -= 2;
//						PortEnabled = c;
						continue;
					}
				}
				*(ptr2++) = c;
			}
		}
		*MsgLen = RealLen;
		return 0;
	}

	return DeviceIoControl(hDevice,IOCTL_SERIAL_GETDATA,NULL,0,Message,BufLen,MsgLen,NULL); 
}
#else
	return 0;
}
#endif

#ifdef BPQ32

int BPQSerialIsCOMOpen(HANDLE hDevice,unsigned long * Count)
{
	unsigned long bytesReturned;
	return DeviceIoControl(hDevice,IOCTL_SERIAL_IS_COM_OPEN,NULL,0,Count,4,&bytesReturned,NULL);                
}

int BPQSerialGetDTRRTS(HANDLE hDevice, unsigned long * Flags)
{
	unsigned long bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_GET_DTRRTS,NULL,0,Flags,4,&bytesReturned,NULL);                
}

int BPQSerialSetPollDelay(HANDLE hDevice, int PollDelay)
{
	unsigned long bytesReturned;
	
	return DeviceIoControl(hDevice,IOCTL_BPQ_SET_POLLDELAY,&PollDelay,4,NULL,0, &bytesReturned,NULL);            
}

#endif

VOID PUTSTRING(UCHAR * Msg)
{
	BPQSerialSendData(Msg, strlen(Msg));
}

const unsigned short CRCTAB[256];

unsigned short int compute_crc(unsigned char *buf,int len)
{
	unsigned short fcs = 0xffff; 
	int i;

	for(i = 0; i < len; i++) 
		fcs = (fcs >>8 ) ^ CRCTAB[(fcs ^ buf[i]) & 0xff]; 

	return fcs;
}



int PUTCHARx(UCHAR c)
{
	BPQSerialSendData(&c, 1);
	return 0;
}




BOOL HostInit()
{
	int resp;
	unsigned long OpenCount = 0;
	unsigned int Errorval;
	char * Baud = strlop(PORTNAME, ',');

	VCOM = TRUE;
						
#ifdef WIN32
	hDevice = BPQOpenSerialPort(&Errorval);
#endif
#ifdef LINUX
	hDevice = LinuxOpenPTY(PORTNAME);
#endif
	if (hDevice != (HANDLE) -1)
	{            
		if (NewVCOM == 0)
		{
//			resp = BPQSerialIsCOMOpen(hDevice, &OpenCount);
//			PortEnabled = OpenCount;
		}

		resp = BPQSerialSetCTS(hDevice);
		resp = BPQSerialSetDSR(hDevice); 
	}
	else
	{
		Debugprintf("TNC - Open Failed for Port %s", PORTNAME);
		hDevice = 0;
	}

	if (hDevice)
	{
		// Set up buffer pointers
	}
	return TRUE;
}

#ifdef LINUX

int TNCReadCOMBlock(HANDLE fd, char * Block, int MaxLength, int * err)
{
	int Length;
	
	*err = 0;

	Length = read(fd, Block, MaxLength);

	if (Length < 0)
	{
		if (errno != 11 && errno != 35)					// Would Block
			*err = errno;

		return 0;
	}

	return Length;
}

#endif

UCHAR RXBUFFER[300];

VOID HostPoll()
{
	unsigned int n;
	unsigned long Read = 0;


	if (VCOM)
	{
		n = BPQSerialGetData(&RXBUFFER[RXBPtr], 300 - RXBPtr, &Read);
	}
	else
	{
			// Real Port or Linux Virtual 
			
			int Read;
#ifdef LINUX
			int err;

			// We can tell if partner has gone on PTY Pair - read returns 5

			if (Mode == KANTRONICS || Mode == SCS)
				Read = TNCReadCOMBlock(hDevice, &RXBUFFER[RXBPtr], 1000 - RXBPtr, &err);
			else
				Read = TNCReadCOMBlock(hDevice, rxbuffer, 300, &err);

			if (err)
			{
				if (PortEnabled)
				{
					PortEnabled = FALSE;
					DisableAppl(TNC);
					Debugprintf("Device %s closed", PORTNAME);
				}
			}
			else
				PortEnabled = TRUE;

#endif
#ifdef WIN32
				Read = ReadCOMBlock(hDevice, &RXBUFFER[RXBPtr], 300 - RXBPtr);
#endif
	}
	if (Read)
	{		
		RXBPtr += Read;
		ProcessSCSPacket(RXBUFFER, RXBPtr);
	}
	n=0;
}

		

// SCS Mode Stuff

unsigned short int compute_crc(unsigned char *buf,int len);
VOID EmCRCStuffAndSend(UCHAR * Msg, int Len);

int EmUnstuff(UCHAR * MsgIn, UCHAR * MsgOut, int len)
{
	int i, j=0;

	for (i=0; i<len; i++, j++)
	{
		MsgOut[j] = MsgIn[i];
		if (MsgIn[i] == 170)			// Remove next byte
		{
			i++;
			if (MsgIn[i] != 0)
				if (i != len) return -1;
		}
	}

	return j;
}


VOID ProcessSCSHostFrame(UCHAR *  Buffer, int Length)
{
	int Channel = Buffer[0];
	int Command = Buffer[1] & 0x3f;
	int Len = Buffer[2];
	char * MYCall;
	int len;

	if (Toggle == (Buffer[1] & 0x80) && (Buffer[1] & 0x40) == 0)
	{
		// Repeat Condition

		//EmCRCStuffAndSend( SCSReply, ReplyLen);
		//return;
	}

	Toggle = (Buffer[1] & 0x80);
	Toggle ^= 0x80;

//	if (Channel == 255 &&  Len == 0)
	if (Channel == 255)
	{
		UCHAR * NextChan = &SCSReply[4];
		
		// General Poll

		// See if any channels have anything available
		
		if (bytesforHost || bytesEchoed || change)
			*(NextChan++) = DataChannel;			// Something for this channel

		*(NextChan++) = 0;

		SCSReply[2] = 255;
		SCSReply[3] = 1;

		ReplyLen = NextChan - &SCSReply[0];

		EmCRCStuffAndSend(SCSReply, ReplyLen);
		return;
	}

	if (Channel == 254)			// Status
	{
		// Extended Status Poll

		SCSReply[2] = 254;
		SCSReply[3] = 7;		// Status
		SCSReply[4] = 3;		// Len -1

		if (ProtocolState == IDLE || ProtocolState == IRS || ProtocolState == ISS)
		{
			// connected states

			SCSReply[5] = 0xa2;		// ARQ, Traffic

			if (ProtocolState != IRS)
				SCSReply[5] |= 0x8;	// Send Bit

			SCSReply[6] = 3;		// Pactor 3

			if (intSessionBW == 200)
				SCSReply[7] = 1;		// P1
			else if (intSessionBW == 500)
				SCSReply[7] = 2;		// P2
			else
				SCSReply[7] = 3;		// P3

			SCSReply[7] = 3;			// SpeedLevel		
			
			SCSReply[8] = dblOffsetHz;	// Freq Error
		}
		else
		{
			SCSReply[5] = 0;
			SCSReply[6] = 0;
			SCSReply[7] = 0;
			SCSReply[8] = 0;
		}

		ReplyLen = 9;
		EmCRCStuffAndSend(SCSReply, 9);
		return;
	}


	if (Command == 0)
	{
		// Data Frame

		AddDataToDataToSend(&Buffer[3], Buffer[2]+ 1);
		goto AckIt;
	}

	switch (Buffer[3])
	{
	case 'J':				// JHOST

		MODE = FALSE;

		return;

	case 'L':
/*
                            ' Status responses from the 'L' command will have 6 fields separated by spaces.
                            ' The field definitions are as follows:
                            ' Field 1: Number of link status messages not yet displayed
                            ' Field 2: NUmber of receive frames not yet displayed
                            ' Field 3: NUmber of send frames not yet delivered
                            ' Field 4: NUmber of transmitted frames not yet acknowledged
                            ' Field 5: NUmber of tries on the current operation
                            ' Field 6: Link state:
                            '   0 = Disconnected
                            '   1 = Link Setop
                            '   2 = Frame Reject
                            '   3 = Disconnect Request
                            '   4 = Information transfer
                            '   5 = Reject Frame Sent
                            '   6 = Waiting Acknowledgement
                            '   7 = Device Busy
                            '   8 = Remote Device Busy
                            '   9 = Both Devices Busy
                            '   10 = Waiting Acknowledgement and Device Busy
                            '   11 = Waiting Acknowledgement and Remote Busy
                            '   12 = Waiting Acknowledgement and Both Device Busy
                            '   13 = Reject Frame Sent and Device Busy
                            '   14 = Reject Frame Sent and Remote Busy
                            '   15 = Reject Frame Sent and Both Device Busy
                            '
							*/

			// I think only 3 is important as it is used for flow control
			// RMS Express stops at 20

		SCSReply[2] = Channel;
		SCSReply[3] = 1;
		len = sprintf(&SCSReply[4], "%d %d %d %d %d %d", 0, 0, bytDataToSendLength/170, 0, 0, 4);
		ReplyLen = len + 5;
		EmCRCStuffAndSend(SCSReply, len + 5);
		return;


		return;

	case 'G':				// Specific Poll
	
		if (CheckStatusChange())
			return;						// It has sent reply

		if (CheckForData())
			return;						// It has sent reply

		SCSReply[2] = Channel;
		SCSReply[3] = 0;
		ReplyLen = 4;
		EmCRCStuffAndSend(SCSReply, 4);
		return;

	case 'C':				// Connect

		// Could be real, or just C to request status

		if (Channel == 0)
			goto AckIt;

		if (Length == 0)
		{
			// STATUS REQUEST - IF CONNECTED, GET CALL

			return;
		}
		Buffer[Length - 2] = 0;

		SendARQConnectRequest(MYCALL, &Buffer[6]);

	AckIt:

		SCSReply[2] = Channel;
		SCSReply[3] = 0;
		ReplyLen = 4;
		EmCRCStuffAndSend(SCSReply, 4);
		return;

	case 'D':

		// Disconnect

		if (ProtocolState == IDLE || ProtocolState == IRS || ProtocolState == ISS)
			blnARQDisconnect = TRUE;

		goto AckIt;
		
	case '%':

		// %X commands

		switch (Buffer[4])
		{
		case 'V':					// Version

			SCSReply[2] = Channel;
			SCSReply[3] = 1;
			strcpy(&SCSReply[4], "4.8 1.32");
			ReplyLen = 13;
			EmCRCStuffAndSend(SCSReply, 13);

			return;


		case 'M':

			DelayedEcho = Buffer[5];
			
			SCSReply[2] = Channel;
			SCSReply[3] = 1;
			SCSReply[4] = 0;

			ReplyLen = 5;
			EmCRCStuffAndSend(SCSReply, 5);

			return;

		default:
						
			SCSReply[2] = Channel;
			SCSReply[3] = 1;
			SCSReply[4] = 0;

			ReplyLen = 5;
			EmCRCStuffAndSend(SCSReply, 5);

			return;
		}
	case '@':

		// Pobably just @B

			SCSReply[2] = Channel;
			SCSReply[3] = 1;
			len = sprintf(&SCSReply[4], "%d ", 4096 - bytDataToSendLength);
			ReplyLen = len + 5;
			EmCRCStuffAndSend(SCSReply, len + 5);
			return;

	default:
						
		SCSReply[2] = Channel;
		SCSReply[3] = 1;
		SCSReply[4] = 0;

		ReplyLen = 5;
		EmCRCStuffAndSend(SCSReply, 5);
	}
}


VOID ProcessSCSTextCommand(char * Command, int Len)
{
	// Command to SCS in non-Host mode.

	// We can probably just dump anything but JHOST 4 and MYCALL

	if (Len == 1)
		goto SendPrompt;		// Just a CR

	if (_memicmp(Command, "JHOST4", 6) == 0)
	{
		MODE = TRUE;
		Toggle = 0;

		return;
	}

	if (_memicmp(Command, "TERM 4", 6) == 0)
		Term4Mode = TRUE;

	else if (_memicmp(Command, "T 0", 3) == 0)
		Term4Mode = FALSE;

	else if (_memicmp(Command, "PAC 4", 5) == 0)
		PACMode = TRUE;

	if (_memicmp(Command, "MYC", 3) == 0)
	{
		char * ptr = strchr(Command, ' ');
		char MYResp[80];

		Command[Len-1] = 0;		// Remove CR
		
		if (ptr && (strlen(ptr) > 2))
		{
			strcpy(MYCALL, ++ptr); 
		}

		sprintf(MYResp, "\rMycall: >%s<", MYCALL);
		PUTSTRING(MYResp);
	}

	else if (_memicmp(Command, "SYS SERN", 8) == 0)
	{
		char SerialNo[] = "\r\nSerial number: 0100000000000000";
		PUTSTRING(SerialNo);
	}
	else 
	{
		char SerialNo[] = "\rXXXX";
		PUTSTRING(SerialNo);
	}

SendPrompt:	
	
	if (PACMode)
	{
		PUTCHARx( 13);
		PUTCHARx( 'p');
		PUTCHARx( 'a');
		PUTCHARx( 'c');
		PUTCHARx( ':');
		PUTCHARx( ' ');

		return;
	}

	if (Term4Mode)
	{
		PUTCHARx( 13);
		PUTCHARx( 4);
		PUTCHARx( 13);
		PUTCHARx( 'c');
		PUTCHARx( 'm');
		PUTCHARx( 'd');
		PUTCHARx( ':');
		PUTCHARx( ' ');
		PUTCHARx( 1);
	}
	else
	{
		PUTCHARx( 13);
		PUTCHARx( 'c');
		PUTCHARx( 'm');
		PUTCHARx( 'd');
		PUTCHARx( ':');
		PUTCHARx( ' ');
	}



	if (Term4Mode)
		PUTCHARx( 4);

	PUTCHARx( 13);
	PUTCHARx( 'c');
	PUTCHARx( 'm');
	PUTCHARx( 'd');
	PUTCHARx( ':');
	PUTCHARx( ' ');
	
	return;
}



VOID ProcessSCSPacket(UCHAR * rxbuffer, int Length)
{
	unsigned short crc;
	char UnstuffBuffer[500];

	// DED mode doesn't have an end of frame delimiter. We need to know if we have a full frame

	// Fortunately this is a polled protocol, so we only get one frame at a time

	// If first char != 170, then probably a Terminal Mode Frame. Wait for CR on end

	// If first char is 170, we could check rhe length field, but that could be corrupt, as
	// we haen't checked CRC. All I can think of is to check the CRC and if it is ok, assume frame is
	// complete. If CRC is duff, we will eventually time out and get a retry. The retry code
	// can clear the RC buffer
			
Loop:

	if (rxbuffer[0] != 170)
	{
		UCHAR *ptr;
		int cmdlen;
		
		// Char Mode Frame I think we need to see CR on end (and we could have more than one in buffer

		// If we think we are in host mode, then to could be noise - just discard.

		if (MODE)
		{
			RXBPtr = 0;
			return;
		}

		rxbuffer[Length] = 0;

		if (rxbuffer[0] == 0x1b)
		{
			// Just ignore (I think!)

			RXBPtr--;
			if (RXBPtr)
			{
				memmove(rxbuffer, rxbuffer+1, RXBPtr + 1);
				Length--;
				goto Loop;
			}
			return;
		}

		if (rxbuffer[0] == 0x1e)
		{
			// Status POLL

			RXBPtr--;
			if (RXBPtr)
			{
				memmove(rxbuffer, rxbuffer+1, RXBPtr + 1);
				Length--;
				goto Loop;
			}
			PUTCHARx(30);
			PUTCHARx(0x87);
	if (Term4Mode)
	{
		PUTCHARx(13);
		PUTCHARx(4);
		PUTCHARx(13);
		PUTCHARx('c');
		PUTCHARx('m');
		PUTCHARx('d');
		PUTCHARx(':');
		PUTCHARx(' ');
		PUTCHARx(1);
	}
	else
	{
		PUTCHARx(13);
		PUTCHARx('c');
		PUTCHARx('m');
		PUTCHARx('d');
		PUTCHARx(':');
		PUTCHARx(' ');
	}


			return;
		}
		ptr = strchr(rxbuffer, 13);

		if (ptr == 0)
			return;		// Wait for rest of frame

		ptr++;

		cmdlen = ptr - rxbuffer;

		// Complete Char Mode Frame

		RXBPtr -= cmdlen;		// Ready for next frame
					
		ProcessSCSTextCommand(rxbuffer, cmdlen);

		if (RXBPtr)
		{
			memmove(rxbuffer, ptr, RXBPtr + 1);
			if (Mode)
			{
				// now in host mode, so pass rest up a level
				
				ProcessSCSPacket(RXBUFFER, RXBPtr);
				return;
			}
			goto Loop;
		}
		return;
	}

	// Receiving a Host Mode frame

	if (Length < 6)				// Minimum Frame Sise
		return;

	if (rxbuffer[2] == 170)
	{
		// Retransmit Request
	
		RXBPtr = 0;
		return;				// Ignore for now
	}

	// Can't unstuff into same buffer - fails if partial msg received, and we unstuff twice


	Length = EmUnstuff(&rxbuffer[2], &UnstuffBuffer[2], Length - 2);

	if (Length == -1)
	{
		// Unstuff returned an errors (170 not followed by 0)

		RXBPtr = 0;
		return;				// Ignore for now
	}
	crc = compute_crc(&UnstuffBuffer[2], Length);

	if (crc == 0xf0b8)		// Good CRC
	{
		RXBPtr = 0;		// Ready for next frame
		ProcessSCSHostFrame(&UnstuffBuffer[2], Length);
		return;
	}

	// Bad CRC - assume incomplete frame, and wait for rest. If it was a full bad frame, timeout and retry will recover link.

	return;
}


VOID EmCRCStuffAndSend(UCHAR * Msg, int Len)
{
	unsigned short int crc;
	UCHAR StuffedMsg[500];
	int i, j;

	crc = compute_crc(&Msg[2], Len-2);
	crc ^= 0xffff;

	Msg[Len++] = (crc&0xff);
	Msg[Len++] = (crc>>8);

	for (i = j = 2; i < Len; i++)
	{
		StuffedMsg[j++] = Msg[i];
		if (Msg[i] == 170)
		{
			StuffedMsg[j++] = 0;
		}
	}

	if (j != i)
	{
		Len = j;
		memcpy(Msg, StuffedMsg, j);
	}

	Msg[0] = 170;
	Msg[1] = 170;

	BPQSerialSendData( Msg, Len);
}


#ifdef WIN32

HANDLE OpenCOMPort(char * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char szPort[80];
	BOOL fRetVal ;
	COMMTIMEOUTS  CommTimeOuts ;
	int	Err;
	char buf[100];
	HANDLE fd;
	DCB dcb;

	// if Port Name starts COM, convert to \\.\COM or ports above 10 wont work

	if (_memicmp(pPort, "COM", 3) == 0)
	{
		char * pp = (char *)pPort;
		int p = atoi(&pp[3]);
		sprintf( szPort, "\\\\.\\COM%d", p);
	}
	else
		strcpy(szPort, pPort);

	// open COMM device

	fd = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL,
                  NULL );

	if (fd == (HANDLE) -1)
	{
		if (Quiet == 0)
		{
			sprintf(buf," %s could not be opened ", pPort);

			Debugprintf(buf);
		}
		return (FALSE);
	}

	Err = GetFileType(fd);

	// setup device buffers

	SetupComm(fd, 4096, 4096 ) ;

	// purge any information in the buffer

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	// set up for overlapped I/O

	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
	CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
//     CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
	CommTimeOuts.WriteTotalTimeoutConstant = 500 ;
	SetCommTimeouts(fd, &CommTimeOuts ) ;

   dcb.DCBlength = sizeof( DCB ) ;

   GetCommState(fd, &dcb ) ;

   dcb.BaudRate = speed;
   dcb.ByteSize = 8;
   dcb.Parity = 0;
   dcb.StopBits = TWOSTOPBITS;
   dcb.StopBits = Stopbits;

	// setup hardware flow control

	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE ;

	dcb.fOutxCtsFlow = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE ;

	// setup software flow control

   dcb.fInX = dcb.fOutX = 0;
   dcb.XonChar = 0;
   dcb.XoffChar = 0;
   dcb.XonLim = 100 ;
   dcb.XoffLim = 100 ;

   // other various settings

   dcb.fBinary = TRUE ;
   dcb.fParity = FALSE;

   fRetVal = SetCommState(fd, &dcb);

	if (fRetVal)
	{
		if (SetDTR)
			EscapeCommFunction(fd, SETDTR);
		if (SetRTS)
			EscapeCommFunction(fd, SETRTS);
	}
	else
	{
		sprintf(buf,"%s Setup Failed %d ", pPort, GetLastError());

		Debugprintf(buf);
		CloseHandle(fd);
		return 0;
	}

	return fd;
}

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength )
{
	BOOL       fReadStat ;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	// only try to read number of bytes in queue

	ClearCommError(fd, &dwErrorFlags, &ComStat);

	dwLength = min((DWORD) MaxLength, ComStat.cbInQue);

	if (dwLength > 0)
	{
		fReadStat = ReadFile(fd, Block, dwLength, &dwLength, NULL) ;

		if (!fReadStat)
		{
		    dwLength = 0 ;
			ClearCommError(fd, &dwErrorFlags, &ComStat ) ;
		}
	}

   return dwLength;
}


BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	BOOL        fWriteStat;
	DWORD       BytesWritten;
	DWORD       ErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(fd, Block, BytesToWrite,
	                       &BytesWritten, NULL );

	if ((!fWriteStat) || (BytesToWrite != BytesWritten))
	{
		int Err = GetLastError();
		ClearCommError(fd, &ErrorFlags, &ComStat);
		return FALSE;
	}
	return TRUE;
}

VOID COMClearDTR(HANDLE fd);

VOID CloseCOMPort(HANDLE fd)
{
	SetCommMask(fd, 0);

	// drop DTR

	COMClearDTR(fd);

	// purge any outstanding reads/writes and close device handle

	PurgeComm(fd, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

	CloseHandle(fd);
}


VOID COMSetDTR(HANDLE fd)
{
	EscapeCommFunction(fd, SETDTR);
}

VOID COMClearDTR(HANDLE fd)
{
	EscapeCommFunction(fd, CLRDTR);
}

VOID COMSetRTS(HANDLE fd)
{
	EscapeCommFunction(fd, SETRTS);
}

VOID COMClearRTS(HANDLE fd)
{
	EscapeCommFunction(fd, CLRRTS);
}


#endif

#ifdef LINUX

static struct speed_struct
{
	int	user_speed;
	speed_t termios_speed;
} speed_table[] = {
	{300,         B300},
	{600,         B600},
	{1200,        B1200},
	{2400,        B2400},
	{4800,        B4800},
	{9600,        B9600},
	{19200,       B19200},
	{38400,       B38400},
	{57600,       B57600},
	{115200,      B115200},
	{-1,          B0}
};


HANDLE OpenCOMPort(VOID * pPort, int speed, BOOL SetDTR, BOOL SetRTS, BOOL Quiet, int Stopbits)
{
	char Port[256];
	char buf[100];

	//	Linux Version.

	int fd;
	int hwflag = 0;
	u_long param=1;
	struct termios term;
	struct speed_struct *s;

	// As Serial ports under linux can have all sorts of odd names, the code assumes that
	// they are symlinked to a com1-com255 in the BPQ Directory (normally the one it is started from

	if ((UINT)pPort < 256)
		sprintf(Port, "%s/com%d", BPQDirectory, (int)pPort);
	else
		strcpy(Port, pPort);

	if ((fd = open(Port, O_RDWR | O_NDELAY)) == -1)
	{
		if (Quiet == 0)
		{
			perror("Com Open Failed");
			sprintf(buf," %s could not be opened", Port);
			Debugprintf(buf);
		}
		return 0;
	}

	// Validate Speed Param

	for (s = speed_table; s->user_speed != -1; s++)
		if (s->user_speed == speed)
			break;

   if (s->user_speed == -1)
   {
	   fprintf(stderr, "tty_speed: invalid speed %d", speed);
	   return FALSE;
   }

   if (tcgetattr(fd, &term) == -1)
   {
	   perror("tty_speed: tcgetattr");
	   return FALSE;
   }

   	cfmakeraw(&term);
	cfsetispeed(&term, s->termios_speed);
	cfsetospeed(&term, s->termios_speed);

	if (tcsetattr(fd, TCSANOW, &term) == -1)
	{
		perror("tty_speed: tcsetattr");
		return FALSE;
	}

	ioctl(fd, FIONBIO, &param);

	Debugprintf("LinBPQ Port %s fd %d", Port, fd);

	return fd;
}

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength)
{
	int Length;

	Length = read(fd, Block, MaxLength);

	if (Length < 0)
	{
		if (errno != 11 && errno != 35)					// Would Block
		{
			perror("read");
			Debugprintf("Handle %d Errno %d", fd, errno);
		}
		return 0;
	}

	return Length;
}

BOOL WriteCOMBlock(HANDLE fd, char * Block, int BytesToWrite)
{
	//	Some systems seem to have a very small max write size
	
	int ToSend = BytesToWrite;
	int Sent = 0, ret;

	while (ToSend)
	{
		ret = write(fd, &Block[Sent], ToSend);

		if (ret >= ToSend)
			return TRUE;

		if (ret == -1)
		{
			if (errno != 11 && errno != 35)					// Would Block
				return FALSE;
	
			usleep(10000);
			ret = 0;
		}
						
		Sent += ret;
		ToSend -= ret;
	}
	return TRUE;
}

VOID CloseCOMPort(HANDLE fd)
{
	close(fd);
}

VOID COMSetDTR(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_DTR;
    ioctl(fd, TIOCMSET, &status);
}

VOID COMClearDTR(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status &= ~TIOCM_DTR;
    ioctl(fd, TIOCMSET, &status);
}

VOID COMSetRTS(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status |= TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);
}

VOID COMClearRTS(HANDLE fd)
{
	int status;

	ioctl(fd, TIOCMGET, &status);
	status &= ~TIOCM_RTS;
    ioctl(fd, TIOCMSET, &status);
}

#endif


const unsigned short CRCTAB[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf, 
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7, 
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e, 
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876, 
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd, 
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 
0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 
0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 
0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 
0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3, 
0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a, 
0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72, 
0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9, 
0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1, 
0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 
0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 
0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 
0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 
0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036, 
0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e, 
0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5, 
0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd, 
0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134, 
0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 
0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 
0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 
0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 
0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 
0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1, 
0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9, 
0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330, 
0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78 
}; 

