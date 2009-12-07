//
//	DLL to inteface SCS TNC in Pactor Mode to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface
//

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

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetModuleHandle instead of LoadLibrary 
#include "bpq32.h"
#include "..\PactorCommon.c"
 
#define DllImport	__declspec(dllimport)
#define DllExport	__declspec(dllexport)

DllImport UINT CRCTAB;
DllImport char * CTEXTMSG;
DllImport USHORT CTEXTLEN;
DllImport UINT FULL_CTEXT;

RECT Rect;

HANDLE STDOUT=0;

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
int Unstuff(UCHAR * Msg, int len);
VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * rxbuff, int len);
VOID ProcessTermModeResponse(struct TNCINFO * TNC);
BOOL ReadConfigFile(char * filename);
int ProcessLine(char * buf);
VOID ExitHost(struct TNCINFO * TNC);
VOID DoTNCReinit(struct TNCINFO * TNC);
VOID DoTermModeTimeout(struct TNCINFO * TNC);

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

	if (TNC->hDevice == (HANDLE) -1)
		return 0;							// Port not open

	switch (fn)
	{
	case 1:				// poll

		if (TNC->ReportDISC)
		{
			TNC->ReportDISC = FALSE;
			buff[4] = 0;

			return -1;
		}
	
		DEDPoll(port);

		CheckRX(TNC);

		if (TNC->PACTORtoBPQ_Q !=0)
		{
			int datalen;
			
			buffptr=Q_REM(&TNC->PACTORtoBPQ_Q);

			datalen=buffptr[1];

			buff[4] = 0;							// Compatibility with Kam Driver
			buff[7] = 0xf0;
			memcpy(&buff[8],buffptr+2,datalen);		// Data goes to +7, but we have an extra byte
			datalen+=8;
			buff[5]=(datalen & 0xff);
			buff[6]=(datalen >> 8);
		
			ReleaseBuffer(buffptr);

			return (1);
		}
			
		return 0;

	case 2:				// send

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == 0) return (0);			// No buffers, so ignore
		
		if (!TNC->TNCOK)
		{
			// Send Error Response

			buffptr[1] = 36;
			memcpy(buffptr+2, "No Connection to PACTOR TNC\r", 36);

			Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
			
			return 0;
		}

		txlen=(buff[6]<<8) + buff[5]-8;	
		buffptr[1] = txlen;
		memcpy(buffptr+2, &buff[8], txlen);
		
		Q_ADD(&TNC->BPQtoPACTOR_Q, buffptr);

		TNC->FramesOutstanding++;
		
		return (0);


	case 3:				// CHECK IF OK TO SEND. Also used to check if TNC is responding

		if (TNC->FramesOutstanding > 10)
			return (1 | TNC->HostMode << 8);
		else
			return TNC->HostMode << 8;		// OK

	case 4:				// reinit

		return (0);

	case 5:				// Close

		CloseHandle(TNCInfo[port]->hDevice);
		return (0);
	}

	return 0;

}

DllExport int APIENTRY ExtInit(EXTPORTDATA *  PortEntry)
{
	char msg[80];
	struct TNCINFO * TNC;
	int port;
	char * ptr;

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

		return 0;
	}

	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	else
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);
		
	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set MYCALL

	wsprintf(msg, "MYCALL %s\r", TNC->NodeCall);
	strcat(TNC->InitScript, msg);

	MinimizetoTray = GetMinimizetoTrayFlag();

	CreatePactorWindow(TNC);
	
	OpenCOMMPort(TNC, PortEntry->PORTCONTROL.IOBASE, PortEntry->PORTCONTROL.BAUDRATE);

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

//		if (TNC->RXBuffer[TNC->RXLen-2] != ':')
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

	Length = Unstuff(&TNC->RXBuffer[2], Length - 2);

	if (Length == -1)
	{
		// Unstuff returned an errors (170 not followed by 0)

		TNC->RXLen = 0;
		return;				// Ignore for now
	}
	crc = compute_crc(&TNC->RXBuffer[2], Length);

	if (crc == 0xf0b8)		// Good CRC
	{
		TNC->RXLen = 0;		// Ready for next frame
		ProcessDEDFrame(TNC, TNC->RXBuffer, Length);
		
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

	TNC->Timeout = 100;

	return TRUE;
}

VOID DEDPoll(int Port)
{
	struct TNCINFO * TNC = TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	char Status[80];

	// if we have just been attached, Issue a Y0 to stop any connects, and an I command to change to
	// the connecting user's callsign

	if (TNC->PortRecord->ATTACHEDSESSIONS[0] && TNC->Attached == 0)
	{
		// New Attach

		int calllen=0;

		TNC->Attached = TRUE;

		calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4USER, TNC->MyCall);
		TNC->MyCall[calllen] = 0;

		TNC->CmdSet = TNC->CmdSave = malloc(100);
		wsprintf(TNC->CmdSet, "Y0\rI%s\r", TNC->MyCall);

		wsprintf(Status, "In Use by %s", TNC->MyCall);
		SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
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
		
		if (TNC->PortRecord->ATTACHEDSESSIONS[0])		// COnnected
		{
			TNC->Connected = FALSE;		// Back to Command Mode
			TNC->ReportDISC = TRUE;		// Tell Nod
		}

	}

	if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0 && TNC->Attached)
	{
		// Node has disconnected us. If connected to remote, disconnect.
		// Then set users back to 1 and mycall back to Node or Port Call
		
		TNC->Attached = FALSE;
		SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Free");

		if (TNC->Connected)
		{
			// Node has disconnected
			
			UINT * buffptr;
			UCHAR * Poll = TNC->TXBuffer;

			TNC->Connected = FALSE;
			TNC->TXBuffer[2] = 0x1f;
			TNC->TXBuffer[3] = 0x1;
			TNC->TXBuffer[4] = 0x0;
			TNC->TXBuffer[5] = 'D';

			CRCStuffAndSend(TNC, TNC->TXBuffer, 6);

			while(TNC->BPQtoPACTOR_Q)
			{
				buffptr=Q_REM(&TNC->BPQtoPACTOR_Q);
				ReleaseBuffer(buffptr);
			}

			while(TNC->PACTORtoBPQ_Q)
			{
				buffptr=Q_REM(&TNC->PACTORtoBPQ_Q);
				ReleaseBuffer(buffptr);
			}
		}

		TNC->CmdSet = TNC->CmdSave = malloc(100);
		wsprintf(TNC->CmdSet, "I%s\rY1\r", TNC->NodeCall);
	}

	// if we have just restarted or TNC appears to be in terminal mode, run Initialisation Sequence

	if (!TNC->HostMode)
	{
		DoTNCReinit(TNC);
		return;
	}

	//If sending internal command list, send next element

	if (TNC->CmdSet)
	{
		char * start, * end;
		int len;

		start = TNC->CmdSet;
		
		if (*(start) == 0)			// End of Script
		{
			free(TNC->CmdSave);
			TNC->CmdSet = NULL;
		}
		else
		{
			end = strchr(start, 13);
			len = ++end - start -1;	// exclude cr
			TNC->CmdSet = end;

			Poll[2] = 31;		// PACTOR Channel
			Poll[3] = 1;			// Command
			Poll[4] = len - 1;
			memcpy(&Poll[5], start, len);
		
			CRCStuffAndSend(TNC, Poll, len + 5);

			return;
		}
	}

	// Send Data if avail, else send poll

	if (TNC->TNCOK && TNC->BPQtoPACTOR_Q)
	{
		int datalen;
		UINT * buffptr;
			
		buffptr=Q_REM(&TNC->BPQtoPACTOR_Q);

		datalen=buffptr[1];

		Poll[2] = 31;		// PACTOR Channel

		if (TNC->Connected)
		{
			Poll[3] = 0;			// Data?
			TNC->BytesTXed += datalen;
		}
		else
		{
			// Command. Do some sanity checking and look for things to process locally

			char * Buffer = (char *)&buffptr[2];	// Data portion of frame

			Poll[3] = 1;			// Command
			datalen--;				// Exclude CR
			Buffer[datalen] = 0;	// Null Terminate
			_strupr(Buffer);


			if (Buffer[0] == 'C' && datalen > 2)	// Connect
			{
				if (*(++Buffer) == ' ') Buffer++;		// Space isn't needed
				memcpy(TNC->RemoteCall, Buffer, 9);
				TNC->Connecting = TRUE;
				wsprintf(Status, "%s Connecting to %s", TNC->MyCall, TNC->RemoteCall);
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);
			}
		}

		Poll[4] = datalen - 1;
		memcpy(&Poll[5], buffptr+2, datalen);
		
		ReleaseBuffer(buffptr);
		
		CRCStuffAndSend(TNC, Poll, datalen + 5);

		TNC->InternalCmd = TNC->Connected;

		return;
	}

	// Check status Periodically
		
	if (TNC->TNCOK)
	{
		if (TNC->IntCmdDelay == 6)
		{
			Poll[2] = 254;			 // Channel
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
			Poll[2] = 31; // Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 1;			// Len-1
			Poll[5] = '%';			// Bytes acked Status
			Poll[6] = 'T';

			CRCStuffAndSend(TNC, Poll, 7);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;

			return;
		}

		if (TNC->IntCmdDelay == 2)
		{
			Poll[2] = 31; // Channel
			Poll[3] = 0x1;			// Command
			Poll[4] = 0;			// Len-1
			Poll[5] = 'L';			// Status

			CRCStuffAndSend(TNC, Poll, 6);

			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;
			return;
		}

		if (TNC->IntCmdDelay <=0)
		{
			Poll[2] = 31; // Channel
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

	// Use General Poll (255)

	Poll[2] = 255 ; // Channel
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

int Unstuff(UCHAR * Msg, int len)
{
	int i, j=0;

	for (i=0; i<len; i++, j++)
	{
		Msg[j] = Msg[i];
		if (Msg[i] == 170)			// Remove next byte
		{
			i++;
			if (Msg[i] != 0)
				return -1;
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

	//	See if Poll Reply or Data
	
	if (Msg[3] == 0)
	{
		// Success - Nothing Follows

		if (TNC->CmdSet)
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

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"Pactor} Ok\r");

		Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

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
			
					TNC->FramesOutstanding = s3;
					return;
				}

				if (LastCmd == '@')		// @ Commands
				{
					if (TNC->TXBuffer[6]== 'B')	// Buffer Status
					{
						SetDlgItemText(TNC->hDlg, IDC_4, Buffer);
						return;
					}
				}

				if (LastCmd == '%')		// % Commands
				{
					char Status[80];
					
					if (TNC->TXBuffer[6]== 'T')	// TX count Status
					{
						wsprintf(Status, "RX %d TX %d ACKED %s", TNC->BytesRXed, TNC->BytesTXed, Buffer);
						SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
						return;
					}
				}
				return;
			}
		}

		if (Msg[3] == 3)					// Status
		{			
			if (strstr(Buffer, "DISCONNECTED"))
			{
				if ((TNC->Connecting | TNC->Connected) == 0)
					return;

				if (TNC->Connecting)
				{
					// Connect Failed
			
					buffptr = Q_REM(&FREE_Q);
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Failure with %s\r", TNC->RemoteCall);

					Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
	
					TNC->Connecting = FALSE;
					TNC->Connected = FALSE;				// In case!
					TNC->FramesOutstanding = 0;

					wsprintf(Status, "In Use by %s", TNC->MyCall);
					SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

					return;
				}
					
				// Must Have been connected - Release Session

				TNC->Connecting = FALSE;
				TNC->Connected = FALSE;		// Back to Command Mode
				TNC->ReportDISC = TRUE;		// Tell Node
				TNC->FramesOutstanding = 0;

				return;
			}

			if (strstr(Buffer, "CONNECTED"))
			{
				TNC->Connected = TRUE;			// Subsequent data to data channel
				TNC->Connecting = FALSE;

				TNC->BytesRXed = TNC->BytesTXed = 0;

				if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0)
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

					memcpy(TNC->RemoteCall, &Buffer[18], 9);	// Save Text Callsign 

					UpdateMH(TNC, &Buffer[18], '+');

					ConvToAX25(&Buffer[18], Session->L4USER);
					ConvToAX25(GetNodeCall(), Session->L4MYCALL);
	
					Session->CIRCUITINDEX = Index;
					Session->CIRCUITID = NEXTID;
					NEXTID++;
					if (NEXTID == 0) NEXTID++;		// Keep non-zero

					TNC->PortRecord->ATTACHEDSESSIONS[0] = Session;
					TNC->Attached = TRUE;

					Session->L4TARGET = TNC->PortRecord;
					Session->L4CIRCUITTYPE = UPLINK+PACTOR;
					Session->L4WINDOW = L4DEFAULTWINDOW;
					Session->L4STATE = 5;
					Session->SESSIONT1 = L4T1;
					Session->SESSPACLEN = 100;

					wsprintf(Status, "%s Connected to %s Inbound", TNC->RemoteCall, TNC->NodeCall);
					SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

					// If an autoconnect APPL is defined, send it

					if (TNC->ApplCmd)
					{
						buffptr = Q_REM(&FREE_Q);
						if (buffptr == 0) return;			// No buffers, so ignore

						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "%s\r", TNC->ApplCmd);
						Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
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
							Q_ADD(&TNC->BPQtoPACTOR_Q, buffptr);

							Next += CTPaclen;
							Len -= CTPaclen;
						}

						buffptr = Q_REM(&FREE_Q);
						if (buffptr == 0) return;			// No buffers, so ignore

						buffptr[1] = Len;
						memcpy(&buffptr[2], &CTEXTMSG[Next], Len);
						Q_ADD(&TNC->BPQtoPACTOR_Q, buffptr);
					}

					return;
				}
				else
				{
					// Connect Complete
			
					buffptr = Q_REM(&FREE_Q);
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Connected to %s", &Buffer[18]);;

					Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

					wsprintf(Status, "%s Connected to %s Outbound", TNC->MyCall, TNC->RemoteCall);
					SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

	
					return;
				}
			}
			return;

		}

		// 1, 2, 4, 5 - pass to Appl

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"Pactor} %s", &Msg[4]);

		Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (Msg[3] == 7)
	{
		char StatusMsg[60];
		int Status, Mode, ISS, Offset;
		
		if (Msg[2] == 0xfe)						// Status Poll Response
		{
			Status = Msg[5];
			
			TNC->PTCStatus0 = Status;
			TNC->PTCStatus1 = Msg[6] & 3;		// Pactor Level 1-3
			TNC->PTCStatus2 = Msg[7];			// Speed Level
			Offset = Msg[8];

			if (Offset > 128)
				Offset -= 128;

			TNC->PTCStatus3 = Offset; 

			Mode = (Status >> 4) & 7;
			ISS = Status & 8;
			Status &= 7;

			wsprintf(StatusMsg, "%x %x %x %x", TNC->PTCStatus0,
				TNC->PTCStatus1, TNC->PTCStatus2, TNC->PTCStatus3);
			
			if (ISS)
				SetDlgItemText(TNC->hDlg, IDC_1, "Sender");
			else
				SetDlgItemText(TNC->hDlg, IDC_1, "Receiver");

			SetDlgItemText(TNC->hDlg, IDC_2, status[Status]);
			SetDlgItemText(TNC->hDlg, IDC_3, ModeText[Mode]);

			if (Offset == 128)		// Undefined
				wsprintf(StatusMsg, "Mode %s Speed Level %d Freq Offset Unknown",
					PactorLevelText[TNC->PTCStatus1], Msg[7]);
			else
				wsprintf(StatusMsg, "Mode %s Speed Level %d Freq Offset %d",
					PactorLevelText[TNC->PTCStatus1], Msg[7], Offset);

			SetDlgItemText(TNC->hDlg, IDC_PACTORLEVEL, StatusMsg);

			return;
		}
		
		// Connected Data
		
		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore
			
		buffptr[1] = Msg[4] + 1;				// Length
		TNC->BytesRXed += buffptr[1];
		memcpy(&buffptr[2], &Msg[5], buffptr[1]);
		Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

		return;
	}
}

