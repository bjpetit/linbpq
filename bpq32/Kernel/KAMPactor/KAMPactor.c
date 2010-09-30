//
//	DLL to inteface KAM TNC in Pactor Mode to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface
//

// Version 1.2.1.2 July 2010

// Send Change to ISS before each transmission
// Support up to 32 BPQ Ports

// Version 1.2.1.3 August 2010 

// Drop RTS as well as DTR on close

// Version 1.2.1.4 August 2010 

// Save Minimized State

// Version 1.2.1.5 September 2010

// Fix Freq Display after Node reconfig
// Only use AutoConnect APPL for Pactor Connects

// Version 1.2.2.1 September 2010

// Add option to get config from bpq32.cfg

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

#include "KAMPactor.h"
#include "ASMStrucs.h"

#include "..\RigControl.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetModuleHandle instead of LoadLibrary 
#include "bpq32.h"

#define KAM 1

static char ClassName[]="PACTORSTATUS";
static char WindowTitle[] = "KAM Pactor";
static int RigControlRow = 190;

#include "..\PactorCommon.c"
 

DllImport UINT CRCTAB;
DllImport char * CTEXTMSG;
DllImport USHORT CTEXTLEN;
DllImport UINT FULL_CTEXT;

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

int MaxStreams = 26;

struct TNCINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct TNCINFO * conn);
BOOL NEAR WriteCommBlock(struct TNCINFO * TNC);
BOOL NEAR DestroyTTYInfo(int port);
void CheckRX(struct TNCINFO * TNC);
OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed);
VOID KAMPoll(int Port);
VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * rxbuff, int len);
VOID ProcessTermModeResponse(struct TNCINFO * TNC);
VOID DoTNCReinit(struct TNCINFO * TNC);
VOID DoTermModeTimeout(struct TNCINFO * TNC);

VOID ProcessPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKHOSTPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKNormCommand(struct TNCINFO * TNC, UCHAR * rxbuffer);
VOID ProcessHostFrame(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len);

//	Note that Kantronics host Mode uses KISS format Packets (without a KISS COntrol Byte)

VOID EncodeAndSend(struct TNCINFO * TNC, UCHAR * txbuffer, int Len);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	KissDecode(UCHAR * inbuff, UCHAR * outbuff, int len);


DllImport UCHAR NEXTID;
DllImport struct TRANSPORTENTRY * L4TABLE;
DllImport WORD MAXCIRCUITS;
DllImport UCHAR L4DEFAULTWINDOW;
DllImport WORD L4T1;

// Buffer Pool

#define NUMBEROFBUFFERS 20

static UINT FREE_Q=0;

static UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers

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
		return 1;
   		
	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
	
		return 1;
    
	case DLL_PROCESS_DETACH:

		for (i=1; i<33; i++)
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
				wsprintf(Size,"%d,%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom, Minimized);
				retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));

				RegCloseKey(hKey);
			}
			if (MinimizetoTray)	
				DeleteTrayMenuItem(TNC->hDlg);
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
	int Stream;

	if (TNC == NULL || TNC->hDevice == (HANDLE) -1)
		return 0;							// Port not open

	if (!TNC->RIG)
	{
		TNC->RIG = Rig_GETPTTREC(port);

		if (TNC->RIG == 0)
			TNC->RIG = &DummyRig;			// Not using Rig control, so use Dummy
	}	

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

		CheckRX(TNC);
		KAMPoll(port);

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

		// Find TNC Record

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

		if(TNC->Streams[Stream].Connected)
		{
			TNC->Streams[Stream].FramesOutstanding++;
			TNC->Streams[Stream].BytesOutstanding += txlen;
		}
		return (0);


	case 3:				// CHECK IF OK TO SEND. Also used to check if TNC is responding
		
		_asm 
		{
			MOV	EAX,buff
			mov Stream,eax
		}

		if (Stream == 0)
		{
			if (TNC->HFPacket)
			{
				if (TNC->Mem1 < 2000 || TNC->Streams[0].FramesOutstanding  > 4)	
					return (1 | TNC->HostMode << 8);
			}
			else
			{
				if (TNC->Streams[0].FramesOutstanding  > 4)
					return (1 | TNC->HostMode << 8);
			}
		}
		else
		{
			if (TNC->Streams[Stream].FramesOutstanding > 3 ||
				TNC->Streams[Stream].BytesOutstanding > 500 || TNC->Mem1 < 500)	
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

	case 6:				// Scan Control

		return 0;		// None Yet

	}
	return 0;

}
BOOL FirstInit = TRUE;

DllExport int APIENTRY ExtInit(EXTPORTDATA *  PortEntry)
{
	char msg[500];
	struct TNCINFO * TNC;
	int port;
	char * ptr;
	char * TempScript;

	//
	//	Will be called once for each Pactor Port
	//	The COM port number is in IOBASE
	//

	if (FirstInit)
	{
		int i;

		FirstInit = FALSE;

		GetAPI();					// Load BPQ32
		LoadRigDriver();

		// Build buffer pool

		FREE_Q = 0;			// In case reloading;

		for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
		{	
			ReleaseBuffer(&BufferPool[100*i]);
		}
	}

	wsprintf(msg,"KAM Pactor COM%d", PortEntry->PORTCONTROL.IOBASE);
	WritetoConsole(msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;

	ReadConfigFile("KAMPACTOR.CFG", port);

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(msg," ** Error - no info in KAMPACTOR.cfg for this port");
		WritetoConsole(msg);

		return (int)ExtProc;
	}

	if (TNC->RigConfigMsg)
	{
		char * SaveRigConfig = _strdup(TNC->RigConfigMsg);

		TNC->RIG = RigConfig(TNC->RigConfigMsg, port);
			
		if (TNC->RIG == NULL)
		{
			// Report Error

			wsprintf(msg,"Invalid Rig Config %s", SaveRigConfig);
			WritetoConsole(msg);

		}
		free(SaveRigConfig);
	}
	else
		TNC->RIG = NULL;		// In case restart


	PortEntry->MAXHOSTMODESESSIONS = 11;		// Default

	// look for the MAXUSERS config line, and get the limits

	TNC->InitScript = _strupr(TNC->InitScript);

	ptr = strstr(TNC->InitScript, "MAXUSERS");
	
	if (ptr)
	{
		ptr = strchr(ptr,'/');  // to the separator
		if (ptr)
			PortEntry->MAXHOSTMODESESSIONS = atoi(++ptr) + 1;
	}

	if (PortEntry->MAXHOSTMODESESSIONS > 26)
		PortEntry->MAXHOSTMODESESSIONS = 26;

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;
	PortEntry->SCANCAPABILITIES = NONE;		// No Scan Control 

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set Essential Params and MYCALL

	TempScript = malloc(4000);

	strcpy(TempScript, "MARK 1400\r");
	strcat(TempScript, "SPACE 1600\r");
	strcat(TempScript, "INV ON\r");

	strcat(TempScript, TNC->InitScript);

	free(TNC->InitScript);
	TNC->InitScript = TempScript;

	// Others go on end so they can't be overriden

	strcat(TNC->InitScript, "ECHO OFF\r");
	strcat(TNC->InitScript, "XMITECHO ON\r");
	strcat(TNC->InitScript, "TXFLOW OFF\r");
	strcat(TNC->InitScript, "XFLOW OFF\r");
	strcat(TNC->InitScript, "TRFLOW OFF\r");
	strcat(TNC->InitScript, "AUTOCR 0\r");
	strcat(TNC->InitScript, "AUTOLF OFF\r");
	strcat(TNC->InitScript, "CRADD OFF\r");
	strcat(TNC->InitScript, "CRSUP OFF\r");
	strcat(TNC->InitScript, "CRSUP OFF/OFF\r");
	strcat(TNC->InitScript, "LFADD OFF/OFF\r");
	strcat(TNC->InitScript, "LFADD OFF\r");
	strcat(TNC->InitScript, "LFSUP OFF/OFF\r");
	strcat(TNC->InitScript, "LFSUP OFF\r");
	strcat(TNC->InitScript, "RING OFF\r");
	strcat(TNC->InitScript, "ARQBBS OFF\r");

	// Set the ax.25 MYCALL


	wsprintf(msg, "MYCALL %s/%s\r", TNC->NodeCall, TNC->NodeCall);
	strcat(TNC->InitScript, msg);

	MinimizetoTray = GetMinimizetoTrayFlag();

	CreatePactorWindow(TNC);
	
	OpenCOMMPort(TNC, PortEntry->PORTCONTROL.IOBASE, PortEntry->PORTCONTROL.BAUDRATE);

	return ((int)ExtProc);
}


 
VOID KISSCLOSE(int Port)
{ 
	struct TNCINFO * conn = TNCInfo[Port];

	SetCommMask(conn->hDevice, 0);

   // drop DTR and RTS

   EscapeCommFunction(conn->hDevice, CLRDTR);
   EscapeCommFunction(conn->hDevice, CLRRTS);

   // purge any outstanding reads/writes and close device handle

   PurgeComm(conn->hDevice, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
   CloseHandle(conn->hDevice);
 
   return;

}

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
		wsprintf(buf," COM%d Setup Failed %d ", Port, GetLastError());
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

	// If first char != FEND, then probably a Terminal Mode Frame. Wait for CR on end

			
	if (TNC->RXBuffer[0] != FEND)
	{
		// Char Mode Frame I think we need to see cmd: on end

		// If we think we are in host mode, then to could be noise - just discard.

		if (TNC->HostMode)
		{
			TNC->RXLen = 0;		// Ready for next frame
			return;
		}

		TNC->RXBuffer[TNC->RXLen] = 0;

//		if (TNC->RXBuffer[TNC->RXLen-2] != ':')
		if (strstr(TNC->RXBuffer, "cmd:") == 0)
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

	if (TNC->HostMode == 0)	// If we are in Term Mode, discard it. Probably in recovery
	{
		TNC->RXLen = 0;		// Ready for next frame
		return;
	}

	if (Length < 3)				// Minimum Frame Sise
		return;

	if (TNC->RXBuffer[Length-1] != FEND)
		return;					// Wait till we have a full frame

	ProcessHostFrame(TNC, TNC->RXBuffer, Length);	// Could have multiple packets in buffer

	TNC->RXLen = 0;		// Ready for next frame

		
	return;

}

VOID ProcessHostFrame(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len)
{
	UCHAR * FendPtr;
	int NewLen;

	//	Split into KISS Packets. By far the most likely is a single KISS frame
	//	so treat as special case

	if (rxbuffer[1] == FEND)			// Two FENDS - probably got out of sync
	{
		rxbuffer++;
		Len--;
	}
	
	FendPtr = memchr(&rxbuffer[1], FEND, Len-1);
	
	if (FendPtr == &rxbuffer[Len-1])
	{
		ProcessKHOSTPacket(TNC, &rxbuffer[1], Len - 2);
		return;
	}
		
	// Process the first Packet in the buffer

	NewLen =  FendPtr - rxbuffer -1;

	ProcessKHOSTPacket(TNC, &rxbuffer[1], NewLen);
	
	// Loop Back

	ProcessHostFrame(TNC, FendPtr+1, Len - NewLen - 2);
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

	return TRUE;
}

VOID KAMPoll(int Port)
{
	struct TNCINFO * TNC = TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	char Status[80];
	int Stream;

	// If Pactor Session has just been attached, drop back to cmd mode and set Pactor Call to 
	// the connecting user's callsign

	if (TNC->PortRecord->ATTACHEDSESSIONS[0] && TNC->Streams[0].Attached == 0)
	{
		// New Attach

		int calllen;
		UCHAR TXMsg[1000] = "D20";
		int datalen;
		char Msg[80];

		TNC->Streams[0].Attached = TRUE;
		TNC->HFPacket = FALSE;
		TNC->TimeInRX = 0;

		calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4USER, TNC->Streams[0].MyCall);
		TNC->Streams[0].MyCall[calllen] = 0;
		
		EncodeAndSend(TNC, "X", 1);			// ??Return to packet mode??
		datalen = wsprintf(TXMsg, "C20MYPTCALL %s", TNC->Streams[0].MyCall);
		EncodeAndSend(TNC, TXMsg, datalen);
		TNC->InternalCmd = 'M';

		TNC->NeedPACTOR = 0;		// Cancel enter Pactor

		wsprintf(Status, "In Use by %s", TNC->Streams[0].MyCall);
		SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

		// Stop Scanning

		wsprintf(Msg, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
		if (Rig_Command)
			Rig_Command(-1, Msg);

	}
	
	if (TNC->Timeout)
	{
		TNC->Timeout--;
		
		if (TNC->Timeout)			// Still waiting
			return;

		// Timed Out

		if (TNC->HostMode == 0)
		{
			DoTermModeTimeout(TNC);
			return;
		}

		// Timed out in host mode - Clear any connection and reinit the TNC

		Debugprintf("KAM PACTOR - Link to TNC Lost");
		TNC->TNCOK = FALSE;
		TNC->HostMode = 0;
		TNC->ReinitState = 0;
				
		wsprintf(Status,"COM%d Open but TNC not responding", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream])		// Connected
			{
				TNC->Streams[Stream].Connected = FALSE;		// Back to Command Mode
				TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Nod
			}
		}
	}

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0 && TNC->Streams[Stream].Attached)
		{
			// Node has disconnected - clear any connection
			
			UINT * buffptr;
			UCHAR * Poll = TNC->TXBuffer;

			TNC->Streams[Stream].Attached = FALSE;
			TNC->Streams[Stream].Connected = FALSE;
			TNC->Streams[Stream].FramesOutstanding = 0;
			TNC->Streams[Stream].BytesOutstanding = 0;

			if (Stream == 0)					// Pactor Stream
			{
				TNC->TimeInRX = 0;
				if (TNC->HFPacket)
					EncodeAndSend(TNC, "C2AD", 4);		// Disconnect
				else
					EncodeAndSend(TNC, "X", 1);			// ??Return to packet mode??

				TNC->HFPacket = FALSE;

				TNC->NeedPACTOR = 50;				// Need to Send PACTOR command after 5 secs

				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Free");
			}
			else
			{
				UCHAR TXMsg[10];

				wsprintf(TXMsg, "C1%cD", Stream + '@');
//				EncodeAndSend(TNC, TXMsg, 4);
				EncodeAndSend(TNC, TXMsg, 4);		// Send twice - must force a disconnect
				TNC->Timeout = 50;
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
		}
	}

	// if we have just restarted or TNC appears to be in terminal mode, run Initialisation Sequence

	if (!TNC->HostMode)
	{
		DoTNCReinit(TNC);
		return;
	}

	if (TNC->NeedPACTOR)
	{
		TNC->NeedPACTOR--;

		if (TNC->NeedPACTOR == 0)
		{
			int datalen;
			UCHAR TXMsg[80] = "D20";

			datalen = wsprintf(TXMsg, "C20MYPTCALL %s", TNC->NodeCall);
			EncodeAndSend(TNC, TXMsg, datalen);

			if (TNC->OldMode)
				EncodeAndSend(TNC, "C20PACTOR", 9);			// Back to Listen
			else
				EncodeAndSend(TNC, "C20TOR", 6);			// Back to Listen

			TNC->InternalCmd = 'T';
			TNC->Timeout = 50;
			TNC->IntCmdDelay--;

			// Restart Scanning

			wsprintf(Status, "%d SCANSTART 15", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
			if (Rig_Command)
				Rig_Command(-1, Status);

			return;
		}
	}

	for (Stream = 0; Stream <= MaxStreams; Stream++)
	{

		// If in HF Packet mode, normal flow control doesn't seem to work
		//	If more that 4 packets sent, send a status poll. and use response to 
		//	reset frames outstanding

		if ((Stream == 0) && (TNC->HFPacket) && (TNC->Streams[0].FramesOutstanding  > 4))
		{
			EncodeAndSend(TNC, "C10S", 4);
			TNC->InternalCmd = 'S';
			TNC->Timeout = 50;
			return;

		}

		if (TNC->TNCOK && TNC->Streams[Stream].BPQtoPACTOR_Q)
		{
			int datalen;
			UCHAR TXMsg[1000] = "D20";
			INT * buffptr;
			UCHAR * MsgPtr;
			char Status[80];
			

			if (TNC->Streams[Stream].Connected)
			{
				if (Stream > 0)
					wsprintf(TXMsg, "D1%c", Stream + '@');
				else if (TNC->HFPacket)
					memcpy(TXMsg, "D2A", 3);
				else
				{
					// Pactor

					// Dont send if IRS State
					// If in IRS state for too long, force turnround

					if (TNC->TXRXState == 'R')
					{
						if (TNC->TimeInRX++ > 15)
							EncodeAndSend(TNC, "T", 1);			// Changeover to ISS 
						else
							goto Poll;
					}
					TNC->TimeInRX = 0;
				}

				buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);
				datalen=buffptr[1];
				MsgPtr = (UCHAR *)&buffptr[2];

				memcpy(&TXMsg[3], buffptr + 2, datalen);
				EncodeAndSend(TNC, TXMsg, datalen + 3);
				ReleaseBuffer(buffptr);
				TNC->Streams[Stream].BytesTXed += datalen; 

				if (Stream == 0)
				{
					wsprintf(Status, "RX %d TX %d ACKED %d ",
						TNC->Streams[0].BytesRXed, TNC->Streams[0].BytesTXed, TNC->Streams[0].BytesAcked);
					SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);

					if ((TNC->HFPacket == 0) && (TNC->Streams[0].BPQtoPACTOR_Q == 0))		// Nothing following
					{
						EncodeAndSend(TNC, "E", 1);			// Changeover when all sent
					}
				}

				return;
			}
			else // Not Connected
			{
				buffptr=Q_REM(&TNC->Streams[Stream].BPQtoPACTOR_Q);
				datalen=buffptr[1];
				MsgPtr = (UCHAR *)&buffptr[2];

				// Command. Do some sanity checking and look for things to process locally

				datalen--;				// Exclude CR
				MsgPtr[datalen] = 0;	// Null Terminate
				_strupr(MsgPtr);

				if ((Stream == 0) && memcmp(MsgPtr, "RADIO ", 6) == 0)
				{
					wsprintf(&MsgPtr[40], "%d %s", TNC->PortRecord->PORTCONTROL.PORTNUMBER, &MsgPtr[6]);
					if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK->CIRCUITINDEX, &MsgPtr[40]))
					{
						ReleaseBuffer(buffptr);
					}
					else
					{
						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], &MsgPtr[40]);
						Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
					}
					return;
				}

				if (_memicmp(MsgPtr, "D\r", 2) == 0)
				{
					TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
					return;
				}

				if ((Stream == 0) && memcmp(MsgPtr, "HFPACKET", 8) == 0)
				{
					TNC->HFPacket = TRUE;
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "KAM} OK\r");
					Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);
					return;
				}

				if (MsgPtr[0] == 'C' && MsgPtr[1] == ' ' && datalen > 2)	// Connect
				{
					memcpy(TNC->Streams[Stream].RemoteCall, &MsgPtr[2], 9);
					TNC->Streams[Stream].Connecting = TRUE;

					// If Stream 0, Convert C CALL to PACTOR CALL

					if (Stream == 0)
					{
	//					TNC->HFPacket = TRUE;

						if (TNC->HFPacket)
							datalen = wsprintf(TXMsg, "C2AC %s", TNC->Streams[0].RemoteCall);
						else
							datalen = wsprintf(TXMsg, "C20PACTOR %s", TNC->Streams[0].RemoteCall);

						wsprintf(Status, "%s Connecting to %s",
							TNC->Streams[0].MyCall, TNC->Streams[0].RemoteCall);
						SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
					}
					else
						datalen = wsprintf(TXMsg, "C1%cC %s", Stream + '@', TNC->Streams[Stream].RemoteCall);

					EncodeAndSend(TNC, TXMsg, datalen);
					TNC->Timeout = 50;
					TNC->InternalCmd = 'C';			// So we dont send the reply to the user.
					ReleaseBuffer(buffptr);
					TNC->Streams[Stream].Connecting = TRUE;

					return;
				}

				if (memcmp(MsgPtr, "DISCONNECT", datalen) == 0)	// Disconnect
				{
					if (Stream == 0)
					{
						if (TNC->HFPacket)
							EncodeAndSend(TNC, "C2AD", 4);		// ??Return to packet mode??
						else
							EncodeAndSend(TNC, "X", 1);			// ??Return to packet mode??
			
						TNC->NeedPACTOR = 50;
					}
					else
					{
						wsprintf(TXMsg, "C1%cD", Stream + '@');
						EncodeAndSend(TNC, TXMsg, 4);
						TNC->CmdStream = Stream;
						TNC->Timeout = 50;
					}

					TNC->Timeout = 0;					//	Don't expect a response
					TNC->Streams[Stream].Connecting = FALSE;
					TNC->Streams[Stream].ReportDISC = TRUE;
					ReleaseBuffer(buffptr);

					return;
				}
	
				// Other Command ??

				if (Stream > 0)
					datalen = wsprintf(TXMsg, "C1%c%s", Stream + '@', MsgPtr);
				else
					datalen = wsprintf(TXMsg, "C20%s", MsgPtr);
				EncodeAndSend(TNC, TXMsg, datalen);
				ReleaseBuffer(buffptr);
				TNC->Timeout = 50;
				TNC->InternalCmd = 0;
				TNC->CmdStream = Stream;

			}
		}
	}

Poll:

	// Need to poll data and control channel (for responses to commands)

	// Also check status if we have data buffered (for flow control)

	if (TNC->TNCOK)
	{
		if (TNC->IntCmdDelay == 50)
		{
			EncodeAndSend(TNC, "C10S", 4);
			TNC->InternalCmd = 'S';
			TNC->Timeout = 50;
			TNC->IntCmdDelay--;
			return;
		}

		if (TNC->IntCmdDelay <=0)
		{
			EncodeAndSend(TNC, "?", 1);
			TNC->InternalCmd = '?';
			TNC->Timeout = 50;
			TNC->IntCmdDelay = 100;	// Every 30
			return;
		}
		else
			TNC->IntCmdDelay--;
	}

	return;

}

VOID DoTNCReinit(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 1)		// Forcing back to Term
		TNC->ReinitState = 0;		// Got Response, so must be back in term mode

	if (TNC->ReinitState == 0)
	{
		// Just Starting - Send a TNC Mode Command to see if in Terminal or Host Mode

		char Status[80];
		
		wsprintf(Status,"COM%d Initialising TNC", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		Poll[0] = 13;
		TNC->TXLen = 1;

		WriteCommBlock(TNC);
		TNC->Timeout = 50;

		return;
	}

	if (TNC->ReinitState == 2)		// In Term State, Sending Initialisation Commands
	{
		char * start, * end;
		int len;

		start = TNC->InitPtr;
		
		if (*(start) == 0)			// End of Script
		{
			// Put into Host Mode

			memcpy(Poll, "INTFACE HOST\r", 13);

			TNC->TXLen = 13;
			WriteCommBlock(TNC);
			TNC->Timeout = 50;

			TNC->ReinitState = 4;	// Need Reset 

			return;
		}
		
		end = strchr(start, 13);
		len = ++end - start;
		TNC->InitPtr = end;
		memcpy(Poll, start, len);

		TNC->TXLen = len;
		WriteCommBlock(TNC);
		TNC->Timeout = 50;

		return;

	}
}

VOID DoTermModeTimeout(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0)
	{
		//Checking if in Terminal Mode - Try to set back to Term Mode

		TNC->ReinitState = 1;
		Poll[0] = 3;
		Poll[1] = 0x58;				// ?? Back to cmd: mode ??
		TNC->TXLen = 2;

		Poll[0] = 0xc0;
		Poll[1] = 'Q';				// ?? Back to cmd: mode ??
		Poll[2] = 0xc0;
		TNC->TXLen = 3;

		WriteCommBlock(TNC);

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
		return;
	}
}

	

VOID ProcessTermModeResponse(struct TNCINFO * TNC)
{
	UCHAR * Poll = TNC->TXBuffer;

	if (TNC->ReinitState == 0 || TNC->ReinitState == 1) 
	{
		// Testing if in Term Mode. It is, so can now send Init Commands

		TNC->InitPtr = TNC->InitScript;
		TNC->ReinitState = 2;
		DoTNCReinit(TNC);		// Send First Command
		return;
	}
	if (TNC->ReinitState == 2)
	{
		// Sending Init Commands

		DoTNCReinit(TNC);		// Send Next Command
		return;
	}

	if (TNC->ReinitState == 4)		// Send INTFACE, Need RESET
	{
		TNC->ReinitState = 5;
			
		memcpy(Poll, "RESET\r", 6);

		TNC->TXLen = 6;
		WriteCommBlock(TNC);
		TNC->Timeout = 50;
		TNC->HostMode = TRUE;		// Should now be in Host Mode
		TNC->NeedPACTOR = 50;		// Need to Send PACTOR command after 5 secs

		return;
	}

	if (TNC->ReinitState == 5)		// RESET sent
	{
		TNC->ReinitState = 5;

		return;
	}



}

VOID ProcessKHOSTPacket(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	UINT * buffptr;
	char * Buffer = &Msg[3];			// Data portion of frame
	char * Call;
	char Status[80];
	int Stream = 0;

	// Any valid frame is an ACK

	TNC->TNCOK = TRUE;

	Len = KissDecode(Msg, Msg, Len);	// Remove KISS transparency

	if (Msg[1] == '2') Stream = 0; else Stream = Msg[2] - '@';	

	//	See if Poll Reply or Data

	Msg[Len] = 0; // Terminate

	if (Msg[0] == 'M')					// Monitor
	{
		DoMonitor(TNC, Msg, Len);
		return;
	}


	if (Msg[0] == 'E')					// Data Echo
	{
		if (Msg[1] == '2')				// HF Port
		{
			TNC->Streams[0].BytesAcked += Len -3;
			wsprintf(Status, "RX %d TX %d ACKED %d ",
				TNC->Streams[0].BytesRXed, TNC->Streams[0].BytesTXed, TNC->Streams[0].BytesAcked);
			SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);

			if (TNC->Streams[0].BytesTXed - TNC->Streams[0].BytesAcked < 500)
				TNC->Streams[0].FramesOutstanding = 0;
		}
		return;
	}

	if (Msg[0] == 'D')					// Data
	{	
		// Pass to Appl

		buffptr = Q_REM(&FREE_Q);
		if (buffptr == NULL) return;			// No buffers, so ignore

		Len-=3;							// Remove Header

		buffptr[1] = Len;				// Length
		TNC->Streams[Stream].BytesRXed += Len;
		memcpy(&buffptr[2], Buffer, Len);
		Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		if (Stream == 0)
		{
			wsprintf(Status, "RX %d TX %d ACKED %d ", 
					TNC->Streams[0].BytesRXed, TNC->Streams[0].BytesTXed, TNC->Streams[0].BytesAcked);
			SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
		}
		return;
	}


	if (Msg[0] == 'C')					// Command Reponse
	{
		TNC->Timeout = 0;
	
		// See if we need to process locally (Response to our command, Incoming Call, Disconencted, etc

		// See if a response to internal command

		if (TNC->InternalCmd)
		{
			// Process it

			if (TNC->InternalCmd == 'S')		// Status
			{
				char * Line;
				char * ptr;
					
				// Message is line giving free bytes, followed by a line for each active (packet) stream

				// FREE BYTES 1366/5094
				// A/2 #1145(12) CONNECTED to KE7XO-3
				// S/2 CONNECTED to NLV

				// each line is teminated by CR, and by the time it gets here it is null terminated

				//FREE BYTES 2628
				//A/H #80(1) CONNECTED to DK0MNL..

				if (TNC->HFPacket)
					TNC->Streams[0].FramesOutstanding = 0;

				Line = strchr(&Msg[3], 13);
				if (Line == 0) return;
				ptr =  strchr(&Msg[13], '/');
				if (ptr == 0) return;

				TNC->Mem1 = atoi(&Msg[13]);
				TNC->Mem2 = atoi(++ptr);

				while (Line[1] != 0)		// End of stream
				{
					Stream = Line[1] - '@';
					if (Line[5] == '#')
					{
						TNC->Streams[Stream].BytesOutstanding = atoi(&Line[6]);
						ptr = strchr(&Line[6], '(');
						if (ptr)
							TNC->Streams[Stream].FramesOutstanding = atoi(++ptr);
					}
					else
					{
						TNC->Streams[Stream].BytesOutstanding = 0;
						TNC->Streams[Stream].FramesOutstanding = 0;
					}
						
					Line = strchr(&Line[1], 13);
				}
				return;
			}
			return;
		}

		// Pass to Appl

		Stream = TNC->CmdStream;


		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"KAM} %s", Buffer);

		Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (Msg[0] == 'I')					// ISS/IRS State
	{
		if (Msg[2] == '1')
		{
			SetDlgItemText(TNC->hDlg, IDC_1, "Sender");
			TNC->TXRXState = 'S';
		}
		else
		{
			SetDlgItemText(TNC->hDlg, IDC_1, "Receiver");
			TNC->TXRXState = 'R';
		}
		return;
	}

	if (Msg[0] == '?')					// Status
	{
		TNC->Timeout = 0;
		return;
	}

	if (Msg[0] == 'S')					// Status
	{
		if (Len < 4)
		{
			// Reset Response FEND FEND S00 FEND
					
			char Status[80];

			TNC->Timeout = 0;

			wsprintf(Status,"COM%d TNC link OK", TNC->PortRecord->PORTCONTROL.IOBASE);
			SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

			return;
		}
		
		// Pass to Appl

		if (strstr(Buffer, "STANDBY>") || strstr(Buffer, "*** DISCONNECTED"))
		{
			if ((TNC->Streams[Stream].Connecting | TNC->Streams[Stream].Connected) == 0)
			{
				// Not connected or Connecting. Probably response to going into Pactor Listen Mode

				return;
			}
	
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

				return;
			}

			// Must Have been connected - Release Session

			TNC->Streams[Stream].Connecting = FALSE;
			TNC->Streams[Stream].Connected = FALSE;		// Back to Command Mode
			TNC->Streams[Stream].ReportDISC = TRUE;		// Tell Node
			TNC->Streams[Stream].FramesOutstanding = 0;

			if (Stream == 0)
			{
				// Need to reset Pactor Call in case it was changed

				EncodeAndSend(TNC, "X", 1);			// ??Return to packet mode??
				TNC->NeedPACTOR = 20;
			}

			return;
		}

		if (Msg[2] == '0')
			Call = strstr(Buffer, "<LINKED TO");
		else
			Call = strstr(Buffer, "NNECTED to");

		if (Call)
		{	
			Call+=11;					// To Callsign
			
			if (Stream == 0 && TNC->HFPacket == 0)
			{
				Buffer[Len-4] = 0;
				UpdateMH(TNC, Call, '+');
			}

			TNC->Streams[Stream].BytesRXed = TNC->Streams[Stream].BytesTXed = TNC->Streams[Stream].BytesAcked = 0;

			if (Stream == 0)
			{
				// Stop Scanner

				char Msg[80];
				
				wsprintf(Msg, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
				if (Rig_Command)
					Rig_Command(-1, Msg);

				wsprintf(Status, "RX %d TX %d ACKED %d ", 
					TNC->Streams[0].BytesRXed, TNC->Streams[0].BytesTXed, TNC->Streams[0].BytesAcked);
				
				SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
			}

			if (TNC->PortRecord->ATTACHEDSESSIONS[Stream] == 0)
			{
				// Incoming Connect

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

				TNC->Streams[Stream].Attached = TRUE;

				if (Msg[1] == '2' && Msg[2] == 'A')
					TNC->HFPacket = TRUE;

				strcpy(TNC->Streams[Stream].RemoteCall, Call);	// Save Text Callsign 

				ConvToAX25(Call, Session->L4USER);
				ConvToAX25(TNC->NodeCall, Session->L4MYCALL);
	
				Session->CIRCUITINDEX = Index;
				Session->CIRCUITID = NEXTID;
				NEXTID++;
				if (NEXTID == 0) NEXTID++;		// Keep non-zero

				TNC->PortRecord->ATTACHEDSESSIONS[Stream] = Session;

				Session->L4TARGET = TNC->PortRecord;
				Session->L4CIRCUITTYPE = UPLINK+PACTOR;
				Session->L4WINDOW = L4DEFAULTWINDOW;
				Session->L4STATE = 5;
				Session->SESSIONT1 = L4T1;
				Session->SESSPACLEN = 100;
				Session->KAMSESSION = Stream;

				TNC->Streams[Stream].Connected = TRUE;			// Subsequent data to data channel

				if (Stream == 0)
				{
					if (TNC->RIG)
						wsprintf(Status, "%s Connected to %s Inbound Freq %s", TNC->Streams[0].RemoteCall, TNC->NodeCall, TNC->RIG->Valchar);
					else
						wsprintf(Status, "%s Connected to %s Inbound", TNC->Streams[0].RemoteCall, TNC->NodeCall);

					SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

					// If an autoconnect APPL is defined, send it	

					if (TNC->ApplCmd)
					{
						buffptr = Q_REM(&FREE_Q);
						if (buffptr == 0) return;			// No buffers, so ignore

						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "%s\r", TNC->ApplCmd);
						Q_ADD(&TNC->Streams[Stream].PACTORtoBPQ_Q, buffptr);

						return;
					}
				}

				if (FULL_CTEXT)
				{
					char CTBuff[300] = "D20";
					int Len = CTEXTLEN, CTPaclen = 50;
					int Next = 0;

					if (Stream > 0)
						wsprintf(CTBuff, "D1%c%", Stream + '@');

					while (Len > CTPaclen)		// CTEXT Paclen
					{
						memcpy(&CTBuff[3], &CTEXTMSG[Next], CTPaclen);
						EncodeAndSend(TNC, CTBuff, CTPaclen + 3);
						Next += CTPaclen;
						Len -= CTPaclen;
					}

					memcpy(&CTBuff[3], &CTEXTMSG[Next], Len);
					EncodeAndSend(TNC, CTBuff, Len + 3);
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
	
				TNC->Streams[Stream].Connecting = FALSE;
				TNC->Streams[Stream].Connected = TRUE;			// Subsequent data to data channel

				
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
	}
}


int	KissDecode(UCHAR * inbuff, UCHAR * outbuff, int len)
{
	int i,txptr=0;
	UCHAR c;

	for (i=0;i<len;i++)
	{
		c=inbuff[i];

		if (c == FESC)
		{
			c=inbuff[++i];
			{
				if (c == TFESC)
					c=FESC;
				else
				if (c == TFEND)
					c=FEND;
			}
		}

		outbuff[txptr++]=c;
	}

	return txptr;

}

VOID EncodeAndSend(struct TNCINFO * TNC, UCHAR * txbuffer, int Len)
{
	// Send A Packet With KISS Encoding

	TNC->TXLen = KissEncode(txbuffer, TNC->TXBuffer, Len);

	WriteCommBlock(TNC);
}

int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len)
{
	int i,txptr=0;
	UCHAR c;

	outbuff[0]=FEND;
	txptr=1;

	for (i=0;i<len;i++)
	{
		c=inbuff[i];
		
		switch (c)
		{
		case FEND:
			outbuff[txptr++]=FESC;
			outbuff[txptr++]=TFEND;
			break;

		case FESC:

			outbuff[txptr++]=FESC;
			outbuff[txptr++]=TFESC;
			break;

		default:

			outbuff[txptr++]=c;
		}
	}

	outbuff[txptr++]=FEND;

	return txptr;

}

VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len)
{
	// Update HM list and maube pass to monitor somehow

	UCHAR * ptr;

	ptr = strchr(&Msg[3], '>');

	if (ptr) *(ptr) = 0;

	UpdateMH(TNC, &Msg[3], ' ');

}

