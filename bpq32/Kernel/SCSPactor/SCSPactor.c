//
//	DLL to inteface SCS TNC in Pactor Mode to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface
//

// Dec 29 2009

//	Add Scan Control using %W Hostmode Command
//	Map Rig control port to a Virtual Serial Port.
//	Add Support for packet port(s).


#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <stdlib.h>
#include "time.h"

//#include <process.h>
//#include <time.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#include "SCSPactor.h"
#include "ASMStrucs.h"
#include "..\RigControl.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetModuleHandle instead of LoadLibrary 
#include "bpq32.h"

static char ClassName[]="PACTORSTATUS";

#define SCS

#include "..\PactorCommon.c"
 
#define DllImport	__declspec(dllimport)
#define DllExport	__declspec(dllexport)

DllImport UINT CRCTAB;
DllImport char * CTEXTMSG;
DllImport USHORT CTEXTLEN;
DllImport UINT FULL_CTEXT;
DllImport BPQTRACE();


RECT Rect;

struct TNCINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct TNCINFO * conn);
BOOL NEAR WriteCommBlock(struct TNCINFO * TNC);
BOOL NEAR DestroyTTYInfo(int port);
void CheckRX(struct TNCINFO * TNC);
OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed);
VOID DEDPoll(int Port);
VOID CRCStuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len);
unsigned short int compute_crc(unsigned char *buf,int len);
int Unstuff(UCHAR * MsgIn, UCHAR * MsgOut, int len);
VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * rxbuff, int len);
VOID ProcessTermModeResponse(struct TNCINFO * TNC);
BOOL ReadConfigFile(char * filename);
int ProcessLine(char * buf);
VOID ExitHost(struct TNCINFO * TNC);
VOID DoTNCReinit(struct TNCINFO * TNC);
VOID DoTermModeTimeout(struct TNCINFO * TNC);
BOOL OpenVirtualSerialPort(struct TNCINFO * TNC);
int ReadVCommBlock(struct TNCINFO * TNC, char * Block, int MaxLength);
VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len);


DllImport UCHAR NEXTID;
DllImport struct TRANSPORTENTRY * L4TABLE;
DllImport WORD MAXCIRCUITS;
DllImport UCHAR L4DEFAULTWINDOW;
DllImport WORD L4T1;

// Buffer Pool

#define NUMBEROFBUFFERS 20

static UINT FREE_Q=0;

static UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers

char status[8][8] = {"ERROR",  "REQUEST", "TRAFFIC", "IDLE", "OVER", "PHASE", "SYNCH", ""};

char ModeText[8][14] = {"STANDBY", "AMTOR-ARQ",  "PACTOR-ARQ", "AMTOR-FEC", "PACTOR-FEC", "RTTY / CW", "LISTEN", "Channel-Busy"};

char PactorLevelText[4][14] = {"Not Connected", "PACTOR-I", "PACTOR-II", "PACTOR-III"};


// Get buffer from Queue

UINT * Q_REM(UINT *Q)
{
	UINT  * first,next;
	
	(int)first=Q[0];
	if (first == 0) return (0);			// Empty
	
	next=first[0];						// Address of next buffer
	Q[0]=next;

	return (first);
}

// Return Buffer to Free Queue

UINT ReleaseBuffer(UINT *BUFF)
{
	UINT * pointer;
	
	(UINT)pointer=FREE_Q;
	*BUFF=(UINT)pointer;
	FREE_Q=(UINT)BUFF;

	return (0);
}


int Q_ADD(UINT *Q,UINT *BUFF)
{
	UINT * next;
	
	BUFF[0]=0;							// Clear chain in new buffer

	if (Q[0] == 0)						// Empty
	{
		Q[0]=(UINT)BUFF;				// New one on front
		return(0);
	}

	(int)next=Q[0];

	while (next[0]!=0)
		next=(UINT *)next[0];			// Chain to end of queue

	next[0]=(UINT)BUFF;					// New one on end

	return(0);
}

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[255];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");

	OutputDebugString(Mess);

	return;
}

HANDLE hInstance;

BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	int i;
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	struct TNCINFO * TNC;

	switch(ul_reason_being_called)
	{
	case DLL_PROCESS_ATTACH:

		hInstance = hInst;
	
		// Read Config

		GetAPI();					// Load BPQ32
		ReadConfigFile("SCSPACTOR.CFG");

		// Build buffer pool

		FREE_Q = 0;			// In case reloading;

		for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
		{	
			ReleaseBuffer(&BufferPool[100*i]);
		}

		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
	
		return 1;
    
	case DLL_PROCESS_DETACH:
	
		for (i=1; i<17; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
			if (TNC->hDlg == NULL)
				continue;

			ShowWindow(TNC->hDlg, SW_RESTORE);
			GetWindowRect(TNC->hDlg, &Rect);

			wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", i);
	
			retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, Key, 0, 0, 0,
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

			if (retCode == ERROR_SUCCESS)
			{
				wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
				retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));

				RegCloseKey(hKey);
			}
		}
 		return 1;
	}
 
	return 1;
}

DllExport int ExtProc(int fn, int port,unsigned char * buff)
{
	int txlen = 0;
	UINT * buffptr;
	struct TNCINFO * TNC = TNCInfo[port];
	int Param;
	char Block[100];
	int Stream = 0;

	if (TNC == NULL || TNC->hDevice == (HANDLE) -1)
		return 0;							// Port not open

	switch (fn)
	{
	case 1:				// poll

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->Streams[Stream].ReportDISC)
			{
				TNC->Streams[Stream].ReportDISC = FALSE;
				buff[4] = Stream;

				return -1;
			}
		}

		if (TNC->VCOMHandle)
			ReadVCommBlock(TNC, Block, 100);

	
		DEDPoll(port);

		CheckRX(TNC);


		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->Streams[Stream].PACTORtoBPQ_Q !=0)
			{
				int datalen;
			
				buffptr=Q_REM(&TNC->Streams[Stream].PACTORtoBPQ_Q);

				datalen=buffptr[1];

				buff[4] = Stream;
				buff[7] = 0xf0;
				memcpy(&buff[8],buffptr+2,datalen);		// Data goes to +7, but we have an extra byte
				datalen+=8;
				buff[5]=(datalen & 0xff);
				buff[6]=(datalen >> 8);
		
				ReleaseBuffer(buffptr);
	
				return (1);
			}
		}
	
			
		return 0;

	case 2:				// send

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == 0) return (0);			// No buffers, so ignore

		Stream = buff[4];

		if (!TNC->TNCOK)
		{
			// Send Error Response

			buffptr[1] = 36;
			memcpy(buffptr+2, "No Connection to PACTOR TNC\r", 36);

			Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
			
			return 0;
		}

		txlen=(buff[6]<<8) + buff[5]-8;	
		buffptr[1] = txlen;
		memcpy(buffptr+2, &buff[8], txlen);
		
		Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);

		TNC->Streams[Stream].FramesOutstanding++;
		
		return (0);


	case 3:				// CHECK IF OK TO SEND. Also used to check if TNC is responding

		_asm 
		{
			MOV	EAX,buff
			mov Stream,eax
		}

		if (Stream == 0)
		{
			if (TNC->Streams[0].FramesOutstanding  > 4)
				return (1 | TNC->HostMode << 8);
		}
		else
		{
			if (TNC->Streams[Stream].FramesOutstanding > 3 || TNC->Buffers < 200)	
				return (1 | TNC->HostMode << 8);		}

		return TNC->HostMode << 8;		// OK

	case 4:				// reinit

		return (0);

	case 5:				// Close

		CloseHandle(TNCInfo[port]->hDevice);
				
		PostMessage(TNC->hDlg, WM_DESTROY,0,0);
		DestroyWindow(TNC->hDlg);

		if (MinimizetoTray)	
			DeleteTrayMenuItem(TNC->hDlg);

		TNC->hDlg = 0;

		return (0);

	case 6:				// Scan Stop Interface

		_asm 
		{
			MOV	EAX,buff
			mov Param,eax
		}

		if (Param == 1)		// Request Permission
		{
			TNC->WantToChangeFreq = TRUE;
			TNC->OKToChangeFreq = FALSE;
			return TRUE;
		}

		if (Param == 2)		// Check  Permission
			return TNC->OKToChangeFreq;

		if (Param == 3)		// Release  Permission
		{
			TNC->DontWantToChangeFreq = TRUE;
			return 0;
		}

		return 0;

	}

	return 0;

}

DllExport int APIENTRY ExtInit(EXTPORTDATA *  PortEntry)
{
	char msg[80];
	struct TNCINFO * TNC;
	int port;
	char * ptr;
	int Stream = 0;

	//
	//	Will be called once for each Pactor Port
	//	The COM port number is in IOBASE
	//

	wsprintf(msg,"Pactor COM%d", PortEntry->PORTCONTROL.IOBASE);
	WritetoConsole(msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(msg," ** Error - no info in SCSPACTOR.cfg for this port");
		WritetoConsole(msg);

		return (int) ExtProc;
	}

	// Set up DED addresses for streams (first stream (Pactor) = DED 31
	
	TNC->Streams[0].DEDStream = 31;

	for (Stream = 1; Stream <= MaxStreams; Stream++)
	{
		TNC->Streams[Stream].DEDStream = Stream;
	}

	PortEntry->MAXHOSTMODESESSIONS = MaxStreams;		// Default
	PortEntry->PERMITGATEWAY = TRUE;					// Can change ax.15 call on each stream

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);
		
	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set MYCALL

	wsprintf(msg, "MYCALL %s\rPAC MYCALL %s\r", TNC->NodeCall, TNC->NodeCall);
	strcat(TNC->InitScript, msg);

	MinimizetoTray = GetMinimizetoTrayFlag();

	CreatePactorWindow(TNC);

	LoadRigDriver();

	OpenCOMMPort(TNC, PortEntry->PORTCONTROL.IOBASE, PortEntry->PORTCONTROL.BAUDRATE);

	if (TNC->VCOMPort)
		OpenVirtualSerialPort(TNC);

	return ((int)ExtProc);
}


VOID KISSCLOSE(int Port)
{ 
	DestroyTTYInfo(Port);
}

BOOL NEAR DestroyTTYInfo(int port)
{
   // force connection closed (if not already closed)

   CloseConnection(TNCInfo[port]);

   return TRUE;

} // end of DestroyTTYInfo()


BOOL CloseConnection(struct TNCINFO * conn)
{
   // disable event notification and wait for thread
   // to halt

   SetCommMask(conn->hDevice, 0);

   // drop DTR

   EscapeCommFunction(conn->hDevice, CLRDTR);

   // purge any outstanding reads/writes and close device handle

   PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
   CloseHandle(conn->hDevice);
 
   return TRUE;

} // end of CloseConnection()

OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed)
{
	char szPort[15];
	char buf[80];
	BOOL fRetVal;
	COMMTIMEOUTS CommTimeOuts;

	DCB	dcb;

	// load the COM prefix string and append port number
   
	wsprintf( szPort, "//./COM%d", Port) ;

	// open COMM device

	conn->hDevice =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (conn->hDevice == (HANDLE) -1)
	{
		wsprintf(buf," COM%d Setup Failed - Error %d ", Port, GetLastError());
		WritetoConsole(buf);
		OutputDebugString(buf);
		SetDlgItemText(conn->hDlg, IDC_COMMSSTATE, buf);

		return (FALSE);
	}

	SetupComm(conn->hDevice, 4096, 4096); // setup device buffers

	// purge any information in the buffer

	PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	// set up for overlapped I/O
	  
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 1000;
	SetCommTimeouts(conn->hDevice, &CommTimeOuts);

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
	
	dcb.DCBlength = sizeof(DCB);
	GetCommState(conn->hDevice, &dcb);

	 // setup hardware flow control

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
//	dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	dcb.BaudRate = Speed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fInX = dcb.fOutX = 0;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.XonLim = 0;
	dcb.XoffLim = 0;

	// other various settings

	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;

	fRetVal = SetCommState(conn->hDevice, &dcb);

//	conn->RTS = 1;
//	conn->DTR = 1;

	EscapeCommFunction(conn->hDevice,SETDTR);
	EscapeCommFunction(conn->hDevice,SETRTS);

	wsprintf(buf,"COM%d Open", Port);
	SetDlgItemText(conn->hDlg, IDC_COMMSSTATE, buf);

	return TRUE;
}
void CheckRX(struct TNCINFO * TNC)
{
	BOOL       fReadStat;
	COMSTAT    ComStat;
	DWORD      dwErrorFlags;
	int Length;
	unsigned short crc;
	char UnstuffBuffer[500];

	// only try to read number of bytes in queue 

	if (TNC->RXLen == 500)
		TNC->RXLen = 0;

	ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);

	Length = min(500 - (DWORD)TNC->RXLen, ComStat.cbInQue);

	if (Length == 0)
		return;					// Nothing doing

	fReadStat = ReadFile(TNC->hDevice, &TNC->RXBuffer[TNC->RXLen], Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);		
		return;
	}
	
	TNC->RXLen += Length;

	Length = TNC->RXLen;

	// DED mode doesn't have an end of frame delimiter. We need to know if we have a full frame

	// Fortunately this is a polled protocol, so we only get one frame at a time

	// If first char != 170, then probably a Terminal Mode Frame. Wait for CR on end

	// If first char is 170, we could check rhe length field, but that could be corrupt, as
	// we haen't checked CRC. All I can think of is to check the CRC and if it is ok, assume frame is
	// complete. If CRC is duff, we will eventually time out and get a retry. The retry code
	// can clear the RC buffer
			
	if (TNC->RXBuffer[0] != 170)
	{
		// Char Mode Frame I think we need to see cmd: on end

		// If we think we are in host mode, then to could be noise - just discard.

		if (TNC->HostMode)
		{
			TNC->RXLen = 0;		// Ready for next frame
			return;
		}

		TNC->RXBuffer[TNC->RXLen] = 0;

//		if (TNC->Streams[Stream].RXBuffer[TNC->Streams[Stream].RXLen-2] != ':')

		if (strlen(TNC->RXBuffer) < TNC->RXLen)
			TNC->RXLen = 0;

		if (strstr(TNC->RXBuffer, "cmd: ") == 0)

			return;				// Wait for rest of frame

		// Complete Char Mode Frame

		TNC->RXLen = 0;		// Ready for next frame
					
		if (TNC->HostMode == 0)
		{
			// We think TNC is in Terminal Mode
			ProcessTermModeResponse(TNC);
			return;
		}
		// We thought it was in Host Mode, but are wrong.

		TNC->HostMode = FALSE;
		return;
	}

	// Receiving a Host Mode frame

	if (Length < 6)				// Minimum Frame Sise
		return;

	if (TNC->RXBuffer[2] == 170)
	{
		// Retransmit Request
	
		TNC->RXLen = 0;
		return;				// Ignore for now
	}

	// Can't unstuff into same buffer - fails if partial msg received, and we unstuff twice


	Length = Unstuff(&TNC->RXBuffer[2], &UnstuffBuffer[2], Length - 2);

	if (Length == -1)
	{
		// Unstuff returned an errors (170 not followed by 0)

		TNC->RXLen = 0;
		return;				// Ignore for now
	}
	crc = compute_crc(&UnstuffBuffer[2], Length);

	if (crc == 0xf0b8)		// Good CRC
	{
		TNC->RXLen = 0;		// Ready for next frame
		ProcessDEDFrame(TNC, UnstuffBuffer, Length);

		// If there are more channels to poll (more than 1 entry in general poll response,
		// and link is not active, poll the next one

		if (TNC->Timeout == 0)
		{
			UCHAR * Poll = TNC->TXBuffer;

			if (TNC->NexttoPoll[0])
			{
				UCHAR Chan = TNC->NexttoPoll[0] - 1;

				memmove(&TNC->NexttoPoll[0], &TNC->NexttoPoll[1], 19);

				Poll[2] = Chan;			// Channel
				Poll[3] = 0x1;			// Command

				if (Chan == 254)		// Status - Send Extended Status (G3)
				{
					Poll[4] = 1;			// Len-1
					Poll[5] = 'G';			// Extended Status Poll
					Poll[6] = '3';
				}
				else
				{
					Poll[4] = 0;			// Len-1
					Poll[5] = 'G';			// Poll
				}

				CRCStuffAndSend(TNC, Poll, Poll[4] + 6);
				TNC->InternalCmd = FALSE;

				return;
			}
			else
			{
				// if last message wasn't a general poll, send one now

				if (TNC->PollSent)
					return;

				TNC->PollSent = TRUE;

				// Use General Poll (255)

				Poll[2] = 255 ;			// Channel
				Poll[3] = 0x1;			// Command

				Poll[4] = 0;			// Len-1
				Poll[5] = 'G';			// Poll

				CRCStuffAndSend(TNC, Poll, 6);
				TNC->InternalCmd = FALSE;
			}
		}
		return;
	}

	// Bad CRC - assume incomplete frame, and wait for rest. If it was a full bad frame, timeout and retry will recover link.

	return;
}

BOOL NEAR WriteCommBlock(struct TNCINFO * TNC)
{
	BOOL        fWriteStat;
	DWORD       dwBytesWritten;
	DWORD       dwErrorFlags;
	COMSTAT     ComStat;

	fWriteStat = WriteFile(TNC->hDevice, TNC->TXBuffer, TNC->TXLen, &dwBytesWritten, NULL);

	if ((!fWriteStat) || (TNC->TXLen != dwBytesWritten))
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);
	}

	TNC->Timeout = 20;				// 2 secs

	return TRUE;
}

VOID DEDPoll(int Port)
{
	struct TNCINFO * TNC = TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	char Status[80];
	int Stream = 0;
	int nn;

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] && TNC->Streams[Stream].Attached == 0)
		{
			// New Attach

			// If Pactor, stop scanning and take out of listen mode.

			// Set call to connecting user's call

			int calllen=0;

			TNC->Streams[Stream].Attached = TRUE;

			calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[Stream]->L4USER, TNC->Streams[Stream].MyCall);
			TNC->Streams[Stream].MyCall[calllen] = 0;

			TNC->Streams[Stream].CmdSet = TNC->Streams[Stream].CmdSave = malloc(100);
			wsprintf(TNC->Streams[Stream].CmdSet, "I%s\r", TNC->Streams[Stream].MyCall);

			if (Stream == 0)
			{
				wsprintf(Status, "In Use by %s", TNC->Streams[0].MyCall);
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

				// Stop Scanner
		
				wsprintf(Status, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
				if (Rig_Command)
					Rig_Command(-1, Status);
			}
		}
	}

	if (TNC->Timeout)
	{
		TNC->Timeout--;
		
		if (TNC->Timeout)			// Still waiting
			return;

		TNC->Retries--;

		if(TNC->Retries)
		{
			WriteCommBlock(TNC);	// Retransmit Block
			return;
		}

		// Retried out.

		if (TNC->HostMode == 0)
		{
			DoTermModeTimeout(TNC);
			return;
		}

		// Retried out in host mode - Clear any connection and reinit the TNC

		Debugprintf("PACTOR - Link to TNC Lost");
		TNC->TNCOK = FALSE;

		wsprintf(Status,"COM%d Open but TNC not responding", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		TNC->HostMode = 0;
		TNC->ReinitState = 0;
		
		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])		// Connected
			{
				TNC->Streams[Stream].Connected = FALSE;		// Back to Command Mode
				TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
			}
		}
	}

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0 && TNC->Streams[Stream].Attached)
		{
			UINT * buffptr;

			// Node has disconnected us. If connected to remote, disconnect.
			// Then set mycall back to Node or Port Call
		
			TNC->Streams[Stream].Attached = FALSE;

			if (Stream == 0)
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Free");

			// Queue it - it won't go till after the response to the D command

			TNC->Streams[Stream].CmdSet = TNC->Streams[Stream].CmdSave = malloc(100);
			wsprintf(TNC->Streams[Stream].CmdSet, "I%s\r", TNC->NodeCall);

			//	if Pactor Channel, Start Scanner
				
			if (Stream == 0)
			{
				wsprintf(Status, "%d SCANSTART 15", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
				if (Rig_Command) Rig_Command(-1, Status);
			}

			while(TNC->Streams[Stream].BPQtoPACTOR_Q)
			{
				buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);
				ReleaseBuffer(buffptr);
			}

			while(TNC->Streams[Stream].PACTORtoBPQ_Q)
			{
				buffptr=Q_REM(&TNC->Streams[Stream].PACTORtoBPQ_Q);
				ReleaseBuffer(buffptr);
			}

			if (TNC->Streams[Stream].Connected)
			{
				// Node has disconnected
			
				UCHAR * Poll = TNC->TXBuffer;

				TNC->Streams[Stream].Connected = FALSE;
				TNC->TXBuffer[2] = TNC->Streams[Stream].DEDStream;
				TNC->TXBuffer[3] = 0x1;
				TNC->TXBuffer[4] = 0x0;
				TNC->TXBuffer[5] = 'D';

				CRCStuffAndSend(TNC, TNC->TXBuffer, 6);

				return;
			}
		}
	}

	// if we have just restarted or TNC appears to be in terminal mode, run Initialisation Sequence

	if (!TNC->HostMode)
	{
		DoTNCReinit(TNC);
		return;
	}

	TNC->PollSent = FALSE;

	//If sending internal command list, send next element

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->Streams[Stream].CmdSet)
		{
			char * start, * end;
			int len;

			start = TNC->Streams[Stream].CmdSet;
		
			if (*(start) == 0)			// End of Script
			{
				free(TNC->Streams[Stream].CmdSave);
				TNC->Streams[Stream].CmdSet = NULL;
			}
			else
			{
				end = strchr(start, 13);
				len = ++end - start -1;	// exclude cr
				TNC->Streams[Stream].CmdSet = end;

				Poll[2] = TNC->Streams[Stream].DEDStream;		// Channel
				Poll[3] = 1;			// Command
				Poll[4] = len - 1;
				memcpy(&Poll[5], start, len);
		
				CRCStuffAndSend(TNC, Poll, len + 5);

				return;
			}
		}
	}
	// if Freq Change needed, check if ok to do it.
	
	if (TNC->TNCOK)
	{
		if (TNC->WantToChangeFreq)
		{
			Poll[2] = 31;			// Command
			Poll[3] = 1;			// Command
			Poll[4] = 2;			// Len -1
			Poll[5] = '%';
			Poll[6] = 'W';
			Poll[7] = '0';
		
			CRCStuffAndSend(TNC, Poll, 8);

			TNC->InternalCmd = TRUE;
			TNC->WantToChangeFreq = FALSE;

			return;
		}

		if (TNC->DontWantToChangeFreq)
		{
			Poll[2] = 31;			// Command
			Poll[3] = 1;			// Command
			Poll[4] = 2;			// Len -1
			Poll[5] = '%';
			Poll[6] = 'W';
			Poll[7] = '1';
		
			CRCStuffAndSend(TNC, Poll, 8);

			TNC->InternalCmd = TRUE;
			TNC->DontWantToChangeFreq = FALSE;
			TNC->OKToChangeFreq = FALSE;

			return;
		}

	}


	// Send Radio Command if avail

	if (TNC->TNCOK && TNC->BPQtoRadio_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&TNC->BPQtoRadio_Q);

		datalen=buffptr[1];

		Poll[2] = 253;		// Radio Channel
		Poll[3] = 0;		// Data?
		Poll[4] = datalen - 1;
	
		memcpy(&Poll[5], buffptr+2, datalen);
		
		ReleaseBuffer(buffptr);
		
		CRCStuffAndSend(TNC, Poll, datalen + 5);

//		Debugprintf("SCS Sending Rig Command");

		return;
	}

		// Check status Periodically
		
	if (TNC->TNCOK)
	{
		if (TNC->IntCmdDelay == 6)
		{
			Poll[2] = 254;			// Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = 'G';			// Extended Status Poll
			Poll[6] = '3';

			CRCStuffAndSend(TNC, Poll, 7);
						
			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;

			return;
		}

		if (TNC->IntCmdDelay == 4)
		{
			Poll[2] = 31;			 // Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = '%';			// Bytes acked Status
			Poll[6] = 'T';

			CRCStuffAndSend(TNC, Poll, 7);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;

			return;
		}

		if (TNC->IntCmdDelay <=0)
		{
			Poll[2] = 31;			// Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = '@';			// Buffer Status
			Poll[6] = 'B';

			CRCStuffAndSend(TNC, Poll, 7);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay = 20;	// Every 2 secs

			return;
		}
		else
			TNC->IntCmdDelay--;
	}




	// If busy, send status poll, send Data if avail

	// We need to start where we last left off, or a busy stream will lock out the others

	for (nn = 0; nn <= MaxStreams; nn++)
	{
		Stream = TNC->LastStream++;

		if (TNC->LastStream > MaxStreams) TNC->LastStream = 0;


		if (TNC->TNCOK && TNC->Streams[Stream].BPQtoPACTOR_Q)
		{
			int datalen;
			UINT * buffptr;
			
			buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);

			datalen=buffptr[1];

			Poll[2] = TNC->Streams[Stream].DEDStream;		// Channel

			if (TNC->Streams[Stream].Connected)
			{
				Poll[3] = 0;			// Data?
				TNC->Streams[Stream].BytesTXed += datalen;
			}
			else
			{
				// Command. Do some sanity checking and look for things to process locally

				char * Buffer = (char *)&buffptr[2];	// Data portion of frame

				Poll[3] = 1;			// Command
				datalen--;				// Exclude CR
				Buffer[datalen] = 0;	// Null Terminate
				_strupr(Buffer);

				if (memcmp(Buffer, "RADIO ", 6) == 0)
				{
					wsprintf(&Buffer[40], "%d %s", TNC->PortRecord->PORTCONTROL.PORTNUMBER, &Buffer[6]);

					if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK->CIRCUITINDEX, &Buffer[40]))
					{
						ReleaseBuffer(buffptr);
					}
					else
					{
						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], &Buffer[40]);
						Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
					}
					return;
				}

				if (Buffer[0] == 'C' && datalen > 2)	// Connect
				{
					if (*(++Buffer) == ' ') Buffer++;		// Space isn't needed
					memcpy(TNC->Streams[Stream].RemoteCall, Buffer, 9);
					TNC->Streams[Stream].Connecting = TRUE;

					if (Stream == 0)
					{
						wsprintf(Status, "%s Connecting to %s", TNC->Streams[Stream].MyCall, TNC->Streams[Stream].RemoteCall);
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}
				}
			}

			Poll[4] = datalen - 1;
			memcpy(&Poll[5], buffptr+2, datalen);
		
			ReleaseBuffer(buffptr);
		
			CRCStuffAndSend(TNC, Poll, datalen + 5);

			TNC->Streams[Stream].InternalCmd = TNC->Streams[Stream].Connected;

			return;
		}

		// if frames outstanding, issue a poll

		if (TNC->Streams[Stream].FramesOutstanding)
		{
			Poll[2] = TNC->Streams[Stream].DEDStream;
			Poll[3] = 0x1;			// Command
			Poll[4] = 0;			// Len-1
			Poll[5] = 'L';			// Status

			CRCStuffAndSend(TNC, Poll, 6);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;
			return;
		}

	}

	TNC->PollSent = TRUE;

	// Use General Poll (255)

	Poll[2] = 255 ;			// Channel
	Poll[3] = 0x1;			// Command

	if (TNC->ReinitState == 3)
	{
		TNC->ReinitState = 0;
		Poll[3] = 0x41;
	}

	Poll[4] = 0;			// Len-1
	Poll[5] = 'G';			// Poll

	CRCStuffAndSend(TNC, Poll, 6);
	TNC->InternalCmd = FALSE;

	return;

}

VOID DoTNCReinit(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		// Just Starting - Send a TNC Mode Command to see if in Terminal or Host Mode

		char Status[80];
		
		TNC->TNCOK = FALSE;
		wsprintf(Status,"COM%d Initialising TNC", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		Poll[0] = 13;
		TNC->TXLen = 1;

		WriteCommBlock(TNC);
		TNC->Retries = 1;
	}

	if (TNC->ReinitState == 1)		// Forcing back to Term
		TNC->ReinitState = 0;

	if (TNC->ReinitState == 2)		// In Term State, Sending Initialisation Commands
	{
		char * start, * end;
		int len;

		start = TNC->InitPtr;
		
		if (*(start) == 0)			// End of Script
		{
			// Put into Host Mode

			TNC->TXBuffer[2] = 0;
			TNC->Toggle = 0;

			memcpy(Poll, "JHOST4\r", 7);

			TNC->TXLen = 7;
			WriteCommBlock(TNC);

			// Timeout will enter host mode

			TNC->Timeout = 1;
			TNC->Retries = 1;
			TNC->Toggle = 0;
			TNC->ReinitState = 3;	// Set toggle force bit

			return;
		}
		
		end = strchr(start, 13);
		len = ++end - start;
		TNC->InitPtr = end;
		memcpy(Poll, start, len);

		TNC->TXLen = len;
		WriteCommBlock(TNC);

		TNC->Retries = 2;
	}
}

VOID DoTermModeTimeout(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		//Checking if in Terminal Mode - Try to set back to Term Mode

		TNC->ReinitState = 1;
		ExitHost(TNC);
		TNC->Retries = 1;

		return;
	}
	if (TNC->ReinitState == 1)
	{
		// Forcing back to Term Mode

		TNC->ReinitState = 0;
		DoTNCReinit(TNC);				// See if worked
		return;
	}

	if (TNC->ReinitState == 3)
	{
		// Entering Host Mode
	
		// Assume ok

		TNC->HostMode = TRUE;
		TNC->IntCmdDelay = 10;

		return;
	}
}

	



VOID ExitHost(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	// Try to exit Host Mode

	TNC->TXBuffer[2] = 31;
	TNC->TXBuffer[3] = 0x41;
	TNC->TXBuffer[4] = 0x5;
	memcpy(&TNC->TXBuffer[5], "JHOST0", 6);

	CRCStuffAndSend(TNC, Poll, 11);

	return;
}
unsigned short int compute_crc(unsigned char *buf,int len)
{
	int fcs;

	_asm{

	mov	esi,buf
	mov	ecx,len
	mov	edx,-1		; initial value
	mov	edi,CRCTAB	; Going via Import Table

crcloop:

	lodsb

	XOR	DL,AL		; OLD FCS .XOR. CHAR
	MOVZX EBX,DL		; CALC INDEX INTO TABLE FROM BOTTOM 8 BITS
	ADD	EBX,EBX
	ADD	EBX, EDI	; To Table entry
	MOV	DL,DH		; SHIFT DOWN 8 BITS
	XOR	DH,DH		; AND CLEAR TOP BITS
	XOR	DX,[EBX]	; XOR WITH TABLE ENTRY
	
	loop	crcloop

	mov	fcs,EDX

	}	
	return (fcs);
  }

VOID CRCStuffAndSend(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	unsigned short int crc;
	UCHAR StuffedMsg[500];
	int i, j;

    Msg[3] |= TNC->Toggle;
	TNC->Toggle ^= 0x80;

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

	TNC->TXLen = Len;

	Msg[0] = 170;
	Msg[1] = 170;

	WriteCommBlock(TNC);

	TNC->Retries = 5;
}

int Unstuff(UCHAR * MsgIn, UCHAR * MsgOut, int len)
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

VOID ProcessTermModeResponse(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		// Testing if in Term Mode. It is, so can now send Init Commands

		TNC->InitPtr = TNC->InitScript;
		TNC->ReinitState = 2;

		// Send Restart to make sure PTC is in a known state

		strcpy(Poll, "RESTART\r");

		TNC->TXLen = 8;
		WriteCommBlock(TNC);

		return;
	}
	if (TNC->ReinitState == 2)
	{
		// Sending Init Commands

		DoTNCReinit(TNC);		// Send Next Command
		return;
	}
}

VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * Msg, int framelen)
{
	UINT * buffptr;
	char * Buffer;				// Data portion of frame
	char Status[80];
	int Stream = 0;

	// Any valid frame is an ACK

	TNC->Timeout = 0;

	if (TNC->TNCOK == FALSE)
	{
		// Just come up
		char Status[80];
		
		TNC->TNCOK = TRUE;
		wsprintf(Status,"COM%d TNC link OK", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);
	}

	Stream = Msg[2];

	if (Stream == 31) Stream = 0;

	//	See if Poll Reply or Data
	
	if (Msg[3] == 0)
	{
		// Success - Nothing Follows

		if (Stream < 32)
			if (TNC->Streams[Stream].CmdSet)
				return;						// Response to Command Set

		if ((TNC->TXBuffer[3] & 1) == 0)	// Data
			return;

		// If the response to a Command, then we should convert to a text "Ok" for forward scripts, etc

		if (TNC->TXBuffer[5] == 'G')	// Poll
			return;

		if (TNC->TXBuffer[5] == 'C')	// Connect - reply we need is async
			return;

		if (TNC->TXBuffer[5] == 'L')	// Shouldnt happen!
			return;

		if (TNC->TXBuffer[5] == '%' && TNC->TXBuffer[6] == 'W')	// Scan Control - Response to W1
			if (TNC->InternalCmd)
				return;					// Just Ignore

		if (TNC->TXBuffer[5] == 'J')	// JHOST
		{
			if (TNC->TXBuffer[10] == '0')	// JHOST0
			{
				TNC->Timeout = 1;			// 
				return;
			}
		}	



		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"Pactor} Ok\r");

		Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (Msg[3] > 0 && Msg[3] < 6)
	{
		// Success with message - null terminated

		UCHAR * ptr;
		int len;

		if (Msg[2] == 0xff)			// General Poll Response
		{
			UCHAR * Poll = TNC->TXBuffer;
			UCHAR Chan = Msg[4] - 1;

			if (Chan == 255)
				return;				// Nothing doing

			if (Msg[5] != 0)
			{
				// More than one to poll - save the list of channels to poll

				strcpy(TNC->NexttoPoll, &Msg[5]);
			}

			// Poll the channel that had data

			Poll[2] = Chan;			// Channel
			Poll[3] = 0x1;			// Command

			if (Chan == 254)		// Status - Send Extended Status (G3)
			{
				Poll[4] = 1;			// Len-1
				Poll[5] = 'G';			// Extended Status Poll
				Poll[6] = '3';
			}
			else
			{
				Poll[4] = 0;			// Len-1
				Poll[5] = 'G';			// Poll
			}

			CRCStuffAndSend(TNC, Poll, Poll[4] + 6);
			TNC->InternalCmd = FALSE;

			return;
		}

		Buffer = &Msg[4];
		
		ptr = strchr(Buffer, 0);

		if (ptr == 0)
			return;

		*(ptr++) = 13;
		*(ptr) = 0;

		len = ptr - Buffer;

		if (len > 256)
			return;

		// See if we need to process locally (Response to our command, Incoming Call, Disconencted, etc

		if (Msg[3] < 3)						// 1 or 2 - Success or Fail
		{
			// See if a response to internal command

			if (TNC->InternalCmd)
			{
				// Process it

				char LastCmd = TNC->TXBuffer[5];

				if (LastCmd == 'L')		// Status
				{
					int s1, s2, s3, s4, s5, s6, num;

					num = sscanf(Buffer, "%d %d %d %d %d %d", &s1, &s2, &s3, &s4, &s5, &s6);
			
					TNC->Streams[Stream].FramesOutstanding = s3;
					return;
				}

				if (LastCmd == '@')		// @ Commands
				{
					if (TNC->TXBuffer[6]== 'B')	// Buffer Status
					{
						TNC->Buffers = atoi(Buffer);
						SetDlgItemText(TNC->hDlg, IDC_4, Buffer);
						return;
					}
				}

				if (LastCmd == '%')		// % Commands
				{
					char Status[80];
					
					if (TNC->TXBuffer[6]== 'T')	// TX count Status
					{
						wsprintf(Status, "RX %d TX %d ACKED %s", TNC->Streams[Stream].BytesRXed, TNC->Streams[Stream].BytesTXed, Buffer);
						SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
						return;
					}

					if (TNC->TXBuffer[6] == 'W')	// Scan Control
					{
						if (Msg[4] == '1')			// Ok to Change
							TNC->OKToChangeFreq = 1;
						else
							TNC->OKToChangeFreq = -1;
					}
				}
				return;
			}
		}

		if (Msg[3] == 3)					// Status
		{			
			if (strstr(Buffer, "DISCONNECTED"))
			{
				if ((TNC->Streams[Stream].Connecting | TNC->Streams[Stream].Connected) == 0)
					return;

				if (TNC->Streams[Stream].Connecting)
				{
					// Connect Failed
			
					buffptr = Q_REM(&FREE_Q);
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Failure with %s\r", TNC->Streams[Stream].RemoteCall);

					Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
	
					TNC->Streams[Stream].Connecting = FALSE;
					TNC->Streams[Stream].Connected = FALSE;				// In case!
					TNC->Streams[Stream].FramesOutstanding = 0;

					if (Stream == 0)
					{
						wsprintf(Status, "In Use by %s", TNC->Streams[Stream].MyCall);
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}

					return;
				}
					
				// Must Have been connected - Release Session

				TNC->Streams[Stream].Connecting = FALSE;
				TNC->Streams[Stream].Connected = FALSE;		// Back to Command Mode
				TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
				TNC->Streams[Stream].FramesOutstanding = 0;

				return;
			}

			if (strstr(Buffer, "CONNECTED"))
			{
				char * Call = strstr(Buffer, " to ");
				char * ptr;

				Call += 4;

				if (Call[1] == ':')
					Call +=2;

				ptr = strchr(Call, ' ');	
				if (ptr) *ptr = 0;

				ptr = strchr(Call, 13);	
				if (ptr) *ptr = 0;


				TNC->Streams[Stream].Connected = TRUE;			// Subsequent data to data channel
				TNC->Streams[Stream].Connecting = FALSE;

				TNC->Streams[Stream].BytesRXed = TNC->Streams[Stream].BytesTXed = 0;

				//	Stop Scanner

				if (Stream == 0)
				{
					wsprintf(Status, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
					if (Rig_Command) Rig_Command(-1, Status);
				}

				if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0)
				{
					// Incomming Connect

					struct TRANSPORTENTRY * Session;
					int Index = 0;

					Session=L4TABLE;

					// Find a free Circuit Entry

					while (Index < MAXCIRCUITS)
					{
						if (Session->L4USER[0] == 0)
							break;

						Session++;
						Index++;
					}

					if (Index == MAXCIRCUITS)
						return;					// Tables Full

					Buffer[len-1] = 0;

					memcpy(TNC->Streams[Stream].RemoteCall, Call, 9);	// Save Text Callsign 

					if (Stream == 0)
						UpdateMH(TNC, Call, '+');

					ConvToAX25(Call, Session->L4USER);
					ConvToAX25(GetNodeCall(), Session->L4MYCALL);
	
					Session->CIRCUITINDEX = Index;
					Session->CIRCUITID = NEXTID;
					NEXTID++;
					if (NEXTID == 0) NEXTID++;		// Keep non-zero

					TNC->PortRecord->ATTACHEDSESSIONS[Stream] = Session;
					TNC->Streams[Stream].Attached = TRUE;

					Session->L4TARGET = TNC->PortRecord;
					Session->L4CIRCUITTYPE = UPLINK+PACTOR;
					Session->L4WINDOW = L4DEFAULTWINDOW;
					Session->L4STATE = 5;
					Session->SESSIONT1 = L4T1;
					Session->SESSPACLEN = 100;
					Session->KAMSESSION = Stream;

					if (Stream == 0)
					{
						if (TNC->RIG)
							wsprintf(Status, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, TNC->NodeCall, TNC->RIG->Valchar);
						else
							wsprintf(Status, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, TNC->NodeCall);
					
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}

					// If an autoconnect APPL is defined, send it

					if (TNC->ApplCmd)
					{
						buffptr = Q_REM(&FREE_Q);
						if (buffptr == 0) return;			// No buffers, so ignore

						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "%s\r", TNC->ApplCmd);
						Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
						return;
					}

					if (FULL_CTEXT)
					{
						int Len = CTEXTLEN, CTPaclen = 100;
						int Next = 0;

						while (Len > CTPaclen)		// CTEXT Paclen
						{
							buffptr = Q_REM(&FREE_Q);
							if (buffptr == 0) return;			// No buffers, so ignore

							buffptr[1] = CTPaclen;
							memcpy(&buffptr[2], &CTEXTMSG[Next], CTPaclen);
							Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);

							Next += CTPaclen;
							Len -= CTPaclen;
						}

						buffptr = Q_REM(&FREE_Q);
						if (buffptr == 0) return;			// No buffers, so ignore

						buffptr[1] = Len;
						memcpy(&buffptr[2], &CTEXTMSG[Next], Len);
						Q_ADD(&TNC->Streams[Stream].BPQtoPACTOR_Q, buffptr);
					}

					return;
				}
				else
				{
					// Connect Complete
			
					buffptr = Q_REM(&FREE_Q);
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Connected to %s\r", Call);;

					Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

					if (Stream == 0)
					{
						if (TNC->RIG)
							wsprintf(Status, "%s Connected to %s Outbound Freq %s", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall, TNC->RIG->Valchar);
						else
							wsprintf(Status, "%s Connected to %s Outbound", TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);

					SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}
	
					return;
				}
			}
			return;

		}

		if (Msg[3] == 4 || Msg[3] == 5)
		{
			// Monitor

			DoMonitor(TNC, &Msg[3], framelen - 3);
			return;

		}

		// 1, 2, 4, 5 - pass to Appl

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"Pactor} %s", &Msg[4]);

		Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (Msg[3] == 6)
	{
		// Monitor Data With length)

		DoMonitor(TNC, &Msg[3], framelen - 3);
		return;
	}

	if (Msg[3] == 7)
	{
		char StatusMsg[60];
		int Status, Mode, ISS, Offset;
		
		if (Msg[2] == 0xfe)						// Status Poll Response
		{
			Status = Msg[5];
			
			TNC->Streams[0].PTCStatus0 = Status;
			TNC->Streams[0].PTCStatus1 = Msg[6] & 3;		// Pactor Level 1-3
			TNC->Streams[0].PTCStatus2 = Msg[7];			// Speed Level
			Offset = Msg[8];

			if (Offset > 128)
				Offset -= 128;

			TNC->Streams[0].PTCStatus3 = Offset; 

			Mode = (Status >> 4) & 7;
			ISS = Status & 8;
			Status &= 7;

			wsprintf(StatusMsg, "%x %x %x %x", TNC->Streams[0].PTCStatus0,
				TNC->Streams[0].PTCStatus1, TNC->Streams[0].PTCStatus2, TNC->Streams[0].PTCStatus3);
			
			if (ISS)
				SetDlgItemText(TNC->hDlg, IDC_1, "Sender");
			else
				SetDlgItemText(TNC->hDlg, IDC_1, "Receiver");

			SetDlgItemText(TNC->hDlg, IDC_2, status[Status]);
			SetDlgItemText(TNC->hDlg, IDC_3, ModeText[Mode]);

			if (Offset == 128)		// Undefined
				wsprintf(StatusMsg, "Mode %s Speed Level %d Freq Offset Unknown",
					PactorLevelText[TNC->Streams[0].PTCStatus1], Msg[7]);
			else
				wsprintf(StatusMsg, "Mode %s Speed Level %d Freq Offset %d",
					PactorLevelText[TNC->Streams[0].PTCStatus1], Msg[7], Offset);

			SetDlgItemText(TNC->hDlg, IDC_PACTORLEVEL, StatusMsg);

			return;
		}
		
		if (Msg[2] == 253)						// Rig Port Response
		{
			int ret;

			// (Win98)
			//	return DeviceIoControl(
			//			VCOMInfo[port]->ComDev,(VCOMInfo[port]->Port << 16) | W98_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
			//else

			DeviceIoControl(
					TNC->VCOMHandle, IOCTL_SERIAL_SETDATA, &Msg[5], Msg[4] + 1, NULL, 0, &ret, NULL);

//			Debugprintf("SCS - Rig Control Response");


			return;
		}
		// Connected Data
		
		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore
			
		buffptr[1] = Msg[4] + 1;				// Length
		TNC->Streams[Stream].BytesRXed += buffptr[1];
		memcpy(&buffptr[2], &Msg[5], buffptr[1]);
		Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}
}

BOOL OpenVirtualSerialPort(struct TNCINFO * TNC)
{
	char szPort[16];
	char buf[80];

/*#pragma warning( push )
#pragma warning( disable : 4996 )

   if (HIBYTE(_winver) < 5)
		Win98 = TRUE;

#pragma warning( pop ) 

   if (Win98)
	{
		VCOMInfo[bpqport]->ComDev = CreateFile( "\\\\.\\BPQVCOMM.VXD", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}
	else
*/
	{
		wsprintf( szPort, "\\\\.\\BPQ%d", TNC->VCOMPort) ;

		TNC->VCOMHandle = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}		  
	if (TNC->VCOMHandle == (HANDLE) -1 )
	{
		wsprintf(buf,"Virtual COM Port %d could not be opened ", TNC->VCOMPort);
		WritetoConsole(buf);

		return (FALSE) ;
	}

	return (TRUE) ;
}

int ReadVCommBlock(struct TNCINFO * TNC, char * Block, int MaxLength)
{
	DWORD Length;
	UINT * buffptr;

	
	Length = 0;

//	if (Win98)
//		DeviceIoControl(
//			pVCOMInfo->ComDev, (pVCOMInfo->Port << 16) |W98_SERIAL_GETDATA,NULL,0,lpszBlock,nMaxLength, &dwLength,NULL);
//	else
	DeviceIoControl(
			TNC->VCOMHandle, IOCTL_SERIAL_GETDATA, NULL, 0, Block, MaxLength, &Length,NULL);

	if (Length == 0)
		return 0;

	if (Length == MaxLength)
		return 0;							// Probably garbage in buffer

	if (!TNC->TNCOK)
		return 0;

	// Queue for TNC

	buffptr = Q_REM(&FREE_Q);

	if (buffptr == 0) return (0);			// No buffers, so ignore

	buffptr[1] = Length;
		
	memcpy(buffptr+2, Block, Length);
		
	Q_ADD(&TNC->BPQtoRadio_Q, buffptr);

//	Debugprintf("SCS Rig Command Queued");

   return 0;

}
MESSAGE Monframe;		// I frames come in two parts.

#define TIMESTAMP 352

VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	// Convert to ax.25 form and pass to monitor

	UCHAR * ptr;

	if (Msg[0] == 6)		// Second part of I or UI
	{
		int len = Msg[1] +1;

		memcpy(Monframe.L2DATA, &Msg[2], len);
		Monframe.LENGTH += len;

		_asm {

		pushad

		mov edi, offset Monframe
		mov eax, BPQTRACE
		call eax

		popad
		}

		return;
	}

	Monframe.LENGTH = 23;				// Control Frame
	Monframe.PORT = TNC->PortRecord->PORTCONTROL.PORTNUMBER;


	ptr = strstr(Msg, "fm ");

	ConvToAX25(&ptr[3], Monframe.ORIGIN);

	UpdateMH(TNC, &ptr[3], ' ');

	ptr = strstr(ptr, "to ");

	ConvToAX25(&ptr[3], Monframe.DEST);

	Monframe.ORIGIN[6] |= 1;				// Set end of address

	ptr = strstr(ptr, "ctl ");

	if (memcmp(&ptr[4], "SABM", 4) == 0)
		Monframe.CTL = 0x2f;
	else  
	if (memcmp(&ptr[4], "DISC", 4) == 0)
		Monframe.CTL = 0x43;
	else 
	if (memcmp(&ptr[4], "UA", 2) == 0)
		Monframe.CTL = 0x63;
	else  
	if (memcmp(&ptr[4], "DM", 2) == 0)
		Monframe.CTL = 0x0f;
	else 
	if (memcmp(&ptr[4], "UI", 2) == 0)
		Monframe.CTL = 0x03;
	else 
	if (memcmp(&ptr[4], "RR", 2) == 0)
		Monframe.CTL = 0x1 | (ptr[6] << 5);
	else 
	if (memcmp(&ptr[4], "RNR", 3) == 0)
		Monframe.CTL = 0x5 | (ptr[7] << 5);
	else 
	if (memcmp(&ptr[4], "REJ", 3) == 0)
		Monframe.CTL = 0x9 | (ptr[7] << 5);
	else 
	if (memcmp(&ptr[4], "FRMR", 4) == 0)
		Monframe.CTL = 0x87;
	else  
	if (ptr[4] == 'I')
	{
		Monframe.CTL = (ptr[5] << 5) | (ptr[6] & 7) << 1 ;
	}

	if (strchr(&ptr[4], '+'))
	{
		Monframe.CTL |= 0x10;
		Monframe.DEST[6] |= 0x80;				// SET COMMAND
	}

	if (strchr(&ptr[4], '-'))	
	{
		Monframe.CTL |= 0x10;
		Monframe.ORIGIN[6] |= 0x80;				// SET COMMAND
	}

	if (Msg[0] == 5)							// More to come
	{
		ptr = strstr(ptr, "pid ");	
		sscanf(&ptr[3], "%x", &Monframe.PID);
		return;	
	}

	_asm {

	pushad

	mov edi, offset Monframe

	push	ecx
	push	edx
	
	push	0
	call	time

	add	esp,4
	
	pop		edx
	pop		ecx

	MOV	TIMESTAMP[EDI],EAX

	mov eax, BPQTRACE
	
	call eax

	popad

	}


	







// Message in EDI







}
//1:fm G8BPQ to KD6PGI-1 ctl I11^ pid F0
//fm KD6PGI-1 to G8BPQ ctl DISC+