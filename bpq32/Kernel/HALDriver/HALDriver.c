//
//	DLL to inteface HAL Communications Corp Clover/Pacor controllers to BPQ32 switch 
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

#include "HALDriver.h"
#include "ASMStrucs.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetModuleHandle instead of LoadLibrary 
#include "bpq32.h"

#define HAL 1

#define SetMYCALL 0x13

#define SetTones 0xec

static char ClassName[]="HALSTATUS";
#include "..\PactorCommon.c"
 
#define DllImport	__declspec(dllimport)
#define DllExport	__declspec(dllexport)

DllImport UINT CRCTAB;
DllImport char * CTEXTMSG;
DllImport USHORT CTEXTLEN;
DllImport UINT FULL_CTEXT;

#define	SOH	0x01	// CONTROL CODES 
#define	ETB	0x17
#define	DLE	0x10

//int MaxStreams = 0;

char status[23][50] = {"IDLE", "TFC", "RQ", "ERR", "PHS", "OVER", "FSK TX",
		"FSK RX", "P-MODE100", "P-MODE200", "HUFMAN ON", "HUFMAN OFF", "P-MODE SBY(LISTEN ON)",
		"P-MODE SBY(LISTEN OFF)", "ISS", "IRS",
		"AMTOR SBY(LISTEN ON)", "AMTOR SBY(LISTEN OFF)", "AMTOR FEC TX", "AMTOR FEC RX",  "P-MODE FEC TX", 
		"FREE SIGNAL TX (AMTOR)", "FREE SIGNAL TX TIMED OUT (AMTOR)"};

struct TNCINFO * CreateTTYInfo(int port, int speed);
BOOL NEAR OpenConnection(int);
BOOL NEAR SetupConnection(int);
BOOL CloseConnection(struct TNCINFO * conn);
BOOL NEAR WriteCommBlock(struct TNCINFO * TNC);
BOOL NEAR DestroyTTYInfo(int port);
void CheckRX(struct TNCINFO * TNC);
OpenCOMMPort(struct TNCINFO * conn, int Port, int Speed);
VOID HALPoll(int Port);
VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * rxbuff, int len);
VOID ProcessTermModeResponse(struct TNCINFO * TNC);
VOID DoTNCReinit(struct TNCINFO * TNC);
VOID DoTermModeTimeout(struct TNCINFO * TNC);

VOID ProcessHALCmd(struct TNCINFO * TNC);
VOID ProcessHALData(struct TNCINFO * TNC);
VOID ProcessKHOSTPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKNormCommand(struct TNCINFO * TNC, UCHAR * rxbuffer);
VOID ProcessHostFrame(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID DoMonitor(struct TNCINFO * TNC, UCHAR * Msg, int Len);

BOOL HALConnected(struct TNCINFO * TNC, char * Call);
VOID HALDisconnected(struct TNCINFO * TNC);

//	Note that AEA host Mode uses SOH/ETB delimiters, with DLE stuffing

VOID EncodeAndSend(struct TNCINFO * TNC, UCHAR * txbuffer, int Len);
VOID SendCmd(struct TNCINFO * TNC, UCHAR * txbuffer, int Len);
int	DLEEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	DLEDecode(UCHAR * inbuff, UCHAR * outbuff, int len);

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
VOID ShowTraffic(struct TNCINFO * TNC)
{
	char Status[80];

	wsprintf(Status, "RX %d TX %d ACKED %d ", 
		TNC->BytesRXed, TNC->BytesTXed, TNC->BytesAcked);

	SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);
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

		// Read Config

		GetAPI();					// Load BPQ32
		ReadConfigFile("HALDriver.cfg");

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

			Sleep(50);

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

			CloseHandle(TNC->hDevice);

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

	switch (fn)
	{
	case 1:				// poll

		//for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->ReportDISC)
			{
				TNC->ReportDISC = FALSE;
				buff[4] = 0;

				return -1;
			}
		}

		CheckRX(TNC);
		HALPoll(port);

		//for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->PACTORtoBPQ_Q !=0)
			{
				int datalen;
			
				buffptr=Q_REM(&TNC->PACTORtoBPQ_Q);

				datalen=buffptr[1];

				buff[4] = 0;
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

			Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
			
			return 0;
		}

		txlen=(buff[6]<<8) + buff[5]-8;	
		buffptr[1] = txlen;
		memcpy(buffptr+2, &buff[8], txlen);
		
		Q_ADD(&TNC->BPQtoPACTOR_Q, buffptr);

		if(TNC->Connected)
		{
			TNC->FramesQueued++;
			TNC->BytesOutstanding += txlen;
		}
		return (0);


	case 3:				// CHECK IF OK TO SEND. Also used to check if TNC is responding
		
		_asm 
		{
			MOV	EAX,buff
			mov Stream,eax
		}
			
		if (TNC->FramesQueued  > 4)
			(1 | TNC->HostMode << 8);
	
		return TNC->HostMode << 8;		// OK

	case 4:				// reinit

		return (0);

	case 5:				// Close

		CloseHandle(TNCInfo[port]->hDevice);
		return (0);

	case 6:				// Scan Control

		return 0;		// None Yet

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

	wsprintf(msg,"HAL Driver COM%d", PortEntry->PORTCONTROL.IOBASE);
	WritetoConsole(msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;

	TNC = TNCInfo[port];

	if (TNC == NULL)
	{
		// Not defined in Config file

		wsprintf(msg," ** Error - no info in HALDriver.cfg for this port");
		WritetoConsole(msg);

		return (int)ExtProc;
	}

	PortEntry->MAXHOSTMODESESSIONS = 1;		// Default


	TNC->PortRecord = PortEntry;

	if (PortEntry->PORTCONTROL.PORTCALL[0] == 0)
	{
		Debugprintf("PORTCALL not set - Using NODECALL");
		memcpy(TNC->NodeCall, GetNodeCall(), 10);
	}
	else
	{
		Debugprintf("Using PORTCALL");
		ConvFromAX25(&PortEntry->PORTCONTROL.PORTCALL[0], TNC->NodeCall);
	}

	Debugprintf("Pactor Call set to %s", TNC->NodeCall);

	PortEntry->PORTCONTROL.PROTOCOL = 10;
	PortEntry->PORTCONTROL.PORTQUALITY = 0;

	ptr=strchr(TNC->NodeCall, ' ');
	if (ptr) *(ptr) = 0;					// Null Terminate

	// Set the ax.25 MYCALL and EAS ON|

	wsprintf(msg, "EAS ON\rMYCALL %s\rHPOLL ON\r", TNC->NodeCall);
	strcat(TNC->InitScript, msg);

	MinimizetoTray = GetMinimizetoTrayFlag();

	CreatePactorWindow(TNC);
	
	LoadRigDriver();

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

   // drop DTR and RTS

   EscapeCommFunction(conn->hDevice, CLRDTR);
   EscapeCommFunction(conn->hDevice, CLRRTS);

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
	UCHAR RXBuff[500];
	int Length;
	UCHAR Char;
	UCHAR * inptr;
	UCHAR * cmdptr;
	UCHAR * dataptr;
	BOOL CmdEsc, DataEsc;

	// only try to read number of bytes in queue 

	ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);

	Length = min(500, ComStat.cbInQue);

	if (Length == 0)
		return;					// Nothing doing

	fReadStat = ReadFile(TNC->hDevice, RXBuff, Length, &Length, NULL);
	
	if (!fReadStat)
	{
		ClearCommError(TNC->hDevice, &dwErrorFlags, &ComStat);		
		return;
	}
	
	// HAL uses HEX 80 as a command escape, 81 as ESCAPE

	// Command Responses can be variable length

	// Scan sream, and put commands into one buffer, data into another
	// Command Handler will check for each command/response if it has enough - if not it will wait till more arrives

	inptr = &RXBuff[0];
	cmdptr = &TNC->CmdBuffer[TNC->CmdLen];
	dataptr = &TNC->DataBuffer[TNC->DataLen];
	CmdEsc = TNC->CmdEsc;
	DataEsc = TNC->DataEsc;

	while(Length--)
	{
		Char = *(inptr++);

		if (CmdEsc)
		{
			CmdEsc = FALSE;
			*(cmdptr++) = Char;
		}
		else if (DataEsc)
		{
			DataEsc = FALSE;
			*(dataptr++) = Char;
		}
		else if (Char == 0x80)				// Next Char is Command
			CmdEsc = TRUE;
		else if (Char == 0x81)				// Next Char is escapred data (80 or 81)
			DataEsc = TRUE;
		else
			*(dataptr++) = Char;			// Normal Data
	}

	// Save State

	TNC->CmdLen = cmdptr - TNC->CmdBuffer;
	TNC->DataLen = dataptr - TNC->DataBuffer;

	TNC->CmdEsc = CmdEsc;
	TNC->DataEsc = DataEsc;

	if (TNC->DataLen)
		ProcessHALData(TNC);

	if (TNC->CmdLen)
		ProcessHALCmd(TNC);
		
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

VOID HALPoll(int Port)
{
	struct TNCINFO * TNC = TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	char Status[80];
	UCHAR TXMsg[1000];
	int datalen;

	if (TNC->Timeout)
	{
		TNC->Timeout--;
		
		if (TNC->Timeout)			// Still waiting
			return;

		// Timed Out

		TNC->TNCOK = FALSE;
		TNC->HostMode = 0;
				
		wsprintf(Status,"COM%d Open but TNC not responding", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		//for (Stream = 0; Stream <= MaxStreams; Stream++)
		{
			if (TNC->PortRecord->ATTACHEDSESSIONS[0])		// Connected
			{
				TNC->Connected = FALSE;		// Back to Command Mode
				TNC->ReportDISC = TRUE;		// Tell Nod
			}
		}
	}

	// if we have just restarted or TNC appears to be in terminal mode, run Initialisation Sequence

	if (TNC->TNCOK)
		if (!TNC->HostMode)
		{
			DoTNCReinit(TNC);
			return;
		}	


	if (TNC->PortRecord->ATTACHEDSESSIONS[0] && TNC->Attached == 0)
	{
		// New Attach

		int calllen;
		char Msg[80];

		TNC->Attached = TRUE;

		calllen = ConvFromAX25(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4USER, TNC->MyCall);
		TNC->MyCall[calllen] = 0;
		
		datalen = wsprintf(TXMsg, "%c%s", SetMYCALL, TNC->MyCall);
		SendCmd(TNC, TXMsg, datalen + 1);			// Send the NULL

		wsprintf(Status, "In Use by %s", TNC->MyCall);
		SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

		// Stop Scanning

		wsprintf(Msg, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
		if (Rig_Command)
			Rig_Command(-1, Msg);

		// Shouldn't we also take out of standby mode?? PN is Pactor Listen, for monitoring

		return;

	}
	
	//for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0 && TNC->Attached)
		{
			// Node has disconnected - clear any connection
			
			UINT * buffptr;
			UCHAR * Poll = TNC->TXBuffer;

			TNC->Attached = FALSE;
			TNC->Connected = FALSE;
			TNC->FramesQueued = 0;
			TNC->BytesOutstanding = 0;

			SendCmd(TNC, "\x06", 1);		// Force Disconnect
			
			// Set call back to NodeCall
			
			datalen = wsprintf(TXMsg, "%c%s", SetMYCALL, TNC->NodeCall);
			SendCmd(TNC, TXMsg, datalen + 1);			// Send the NULL

			SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, "Free");

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
	}

	if (TNC->NeedPACTOR)
	{
		TNC->NeedPACTOR--;

		if (TNC->NeedPACTOR == 0)
		{
			int datalen;
			UCHAR TXMsg[80];

			datalen = wsprintf(TXMsg, "%c%s", SetMYCALL, TNC->NodeCall);
			SendCmd(TNC, TXMsg, datalen + 1);			// Send the NULL

			// Restart Scanning

			wsprintf(Status, "%d SCANSTART 15", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
			if (Rig_Command)
				Rig_Command(-1, Status);

			return;
		}

	}

	//for (Stream = 0; Stream <= MaxStreams; Stream++)
	{
		if (TNC->TNCOK && TNC->BPQtoPACTOR_Q)
		{
			int datalen;
			INT * buffptr;
			UCHAR * MsgPtr;
			char Status[80];
			char TXMsg[500];

			
			buffptr=Q_REM(&TNC->BPQtoPACTOR_Q);

			TNC->FramesQueued--;

			datalen=buffptr[1];
			MsgPtr = (UCHAR *)&buffptr[2];

			if (TNC->Connected)
			{
				EncodeAndSend(TNC,(UCHAR *) buffptr + 2, datalen);
				ReleaseBuffer(buffptr);
				TNC->BytesTXed += datalen; 
				ShowTraffic(TNC);

				return;
			}
			else
			{
				// Command. Do some sanity checking and look for things to process locally

				datalen--;				// Exclude CR
				MsgPtr[datalen] = 0;	// Null Terminate
//				_strupr(MsgPtr);

				if (memcmp(MsgPtr, "RADIO ", 6) == 0)
				{
					wsprintf(&MsgPtr[40], "%d %s", TNC->PortRecord->PORTCONTROL.PORTNUMBER, &MsgPtr[6]);
					if (Rig_Command(TNC->PortRecord->ATTACHEDSESSIONS[0]->L4CROSSLINK->CIRCUITINDEX, &MsgPtr[40]))
					{
						ReleaseBuffer(buffptr);
					}
					else
					{
						buffptr[1] = wsprintf((UCHAR *)&buffptr[2], &MsgPtr[40]);
						Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
					}
					return;
				}

				if (memcmp(MsgPtr, "MODE CLOVER", 11) == 0)
				{
					TNC->CurrentMode = Clover;
						buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"KAM} Ok");
						Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

					return;
				}

				if (memcmp(MsgPtr, "MODE PACTOR", 11) == 0)
				{
					TNC->CurrentMode = Pactor;
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"KAM} Ok");
					Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

					return;
				}
				if (memcmp(MsgPtr, "MODE AMTOR", 11) == 0)
				{
					TNC->CurrentMode = Clover;
					buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"KAM} Ok");
					Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

					return;
				}

				if (MsgPtr[0] == 'C' && MsgPtr[1] == ' ' && datalen > 2)	// Connect
				{
					memcpy(TNC->RemoteCall, &MsgPtr[2], 9);
					TNC->Connecting = TRUE;

					switch (TNC->CurrentMode)
					{
					case Pactor:

						SendCmd(TNC, "\x83", 1);		// Select P-MODE Standby
						SendCmd(TNC, "\x57", 1);		// Enable TX buffer clear on disconnect

						datalen = wsprintf(TXMsg, "%\x19%s", TNC->RemoteCall);
						SendCmd(TNC, TXMsg, datalen + 1);	// Include NULL
					
						wsprintf(Status, "%s Connecting to %s - PACTOR",
						TNC->MyCall, TNC->RemoteCall);

						break;

					case Clover:

						SendCmd(TNC, "\x54", 1);		// Enable adaptive Clover format
						SendCmd(TNC, "\x57", 1);		// Enable TX buffer clear on disconnect

						datalen = wsprintf(TXMsg, "%\x11%s", TNC->RemoteCall);
						SendCmd(TNC, TXMsg, datalen + 1);	// Include NULL
					
						wsprintf(Status, "%s Connecting to %s - CLOVER", TNC->MyCall, TNC->RemoteCall);

						break;			
					}
					
					SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

					SendCmd(TNC, TXMsg, datalen);

					ReleaseBuffer(buffptr);
					TNC->Connecting = TRUE;

					return;
				}

				if (memcmp(MsgPtr, "CLOVER ", 7) == 0)
				{
					memcpy(TNC->RemoteCall, &MsgPtr[2], 9);
					TNC->Connecting = TRUE;

					SendCmd(TNC, "\x54", 1);		// Enable adaptive Clover format
					SendCmd(TNC, "\x57", 1);		// Enable TX buffer clear on disconnect

					datalen = wsprintf(TXMsg, "%\x11%s", TNC->RemoteCall);
					SendCmd(TNC, TXMsg, datalen + 1);	// Include NULL
					
					wsprintf(Status, "%s Connecting to %s - CLOVER",
					TNC->MyCall, TNC->RemoteCall);
					SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

					ReleaseBuffer(buffptr);
					TNC->Connecting = TRUE;

					return;
				}

				if (memcmp(MsgPtr, "DISCONNECT", datalen) == 0)	// Disconnect
				{
					SendCmd(TNC, "\x07", 1);		// Normal Disconnect
					TNC->NeedPACTOR = 50;
	
					TNC->Connecting = FALSE;
					TNC->ReportDISC = TRUE;
					ReleaseBuffer(buffptr);

					return;
				}
	
				// Other Command ?? Treat as HEX string

				datalen = sscanf(MsgPtr, "%X %X %X %X %X %X %X %X %X %X %X %X %X %X ",
					&TXMsg[0], &TXMsg[1], &TXMsg[2], &TXMsg[3], &TXMsg[4], &TXMsg[6], &TXMsg[6], &TXMsg[7], 
					&TXMsg[8], &TXMsg[9], &TXMsg[10], &TXMsg[11], &TXMsg[12], &TXMsg[13], &TXMsg[14], &TXMsg[15]);

				SendCmd(TNC, TXMsg, datalen);
				ReleaseBuffer(buffptr);
				TNC->InternalCmd = 0;
			}
		}
	}

	// Nothing doing - send Poll (but not too often)

	TNC->PollDelay++;

	if (TNC->PollDelay < 10)
		return;

	TNC->PollDelay = 0;

	SendCmd(TNC, "\x7d" , 1);			// Use Get LEDS as Poll

	TNC->Timeout = 20;

	return;
}

VOID DoTNCReinit(struct TNCINFO * TNC)
{
	// TNC Has Restarted, send init commands (can probably send all at once)

	SendCmd(TNC, TNC->InitScript, TNC->InitScriptLen);

	TNC->HostMode = TRUE;		// Should now be in Host Mode
	TNC->NeedPACTOR = 20;		// Need to set Calls and start scan

	return;

}	

VOID ProcessHALData(struct TNCINFO * TNC)
{
	// Received Data just pass to Appl

	UINT * buffptr;
	int Len = TNC->DataLen;

	buffptr = Q_REM(&FREE_Q);
	if (buffptr == NULL) return;	// No buffers, so ignore

	buffptr[1] = Len;				// Length
	TNC->BytesRXed += Len;

	memcpy(&buffptr[2], TNC->DataBuffer, Len);

	Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

	ShowTraffic(TNC);

	TNC->DataLen = 0;

	return;
}



VOID ProcessHALCmd(struct TNCINFO * TNC)
{
	char * Call;
	char Status[80];
	int Stream = 0;
	int Opcode = TNC->CmdBuffer[0];
	int StatusByte;
	int Len = TNC->CmdLen;
	int Used;

	// We may have more than one response in the buffer, and only each cmd/response decoder knows how many it needs

	switch(Opcode)
	{
	case 0x09:			//Hardware Reset - equivalent to power on reset

		// Hardware has reset - need to reinitialise

		TNC->HostMode = 0;			// Force Reinit

		Used = 1;
		break;

	case 0x7a:				// FSK Modes Status

		// Mixture of mode and state - eg listen huffman on/off irs/iss, so cant just display

		if (Len < 2) return;		// Wait for more
	
		StatusByte = TNC->CmdBuffer[1];

		switch (StatusByte)
		{
			case 0x06:		// FSK TX (RTTY)
			case 0x07:		// FSK RX (RTTY)
			case 0x0C:		// P-MODE STANDBY (LISTEN ON)
			case 0x0D:		// P-MODE STANDBY (LISTEN OFF)
			case 0x10:		// AMTOR STANDBY (LISTEN ON)
			case 0x11:		// AMTOR STANDBY (LISTEN OFF)
			case 0x12:		// AMTOR FEC TX (AMTOR)
			case 0x13:		// AMTOR FEC RX (AMTOR)
			case 0x14:		// P-MODE FEC TX (P-MODE)
			case 0x15:		// FREE SIGNAL TX (AMTOR)
			case 0x16:		// FREE SIGNAL TX TIMED OUT (AMTOR)

			// Diaplay Linke Status

			SetDlgItemText(TNC->hDlg, IDC_2, status[StatusByte]);

			break;

			case 0x0E:		// ISS (AMTOR/P-MODE)

				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE,"ISS");
				TNC->TXRXState = 'S';
				break;

			case 0x0F:		// IRS (AMTOR/P-MODE)

				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE,"IRS");
				TNC->TXRXState = 'R';
				break;

			case 0x00:		//  IDLE (AMTOR/P-MODE)
			case 0x01:		//  TFC (AMTOR/P-MODE)
			case 0x02:		//  RQ (AMTOR/P-MODE)
			case 0x03:		//  ERR (AMTOR/P-MODE)
			case 0x04:		//  PHS (AMTOR/P-MODE)
			case 0x05:		//  OVER (AMTOR/P-MODE) (not implemented)


//$807A $8008 P-MODE100 (P-MODE)
//$807A $8009 P-MODE200 (P-MODE)
//$807A $800A HUFFMAN ON (P-MODE)
//$807A $800B HUFFMAN OFF (P-MODE)
				;
		}
		Used = 2;
		break;
		

	case 0x7d:				// Get LED Status
		
		// We use Get LED Status as a Poll

		if (Len < 2) return;		// Wait for more
	
		TNC->TNCOK = TRUE;
		TNC->Timeout = 0;

		wsprintf(Status,"COM%d TNC link OK", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);
		SetDlgItemText(TNC->hDlg, IDC_LEDS, Status);

		Used = 2;
		break;


	case 0x27:						// Clover ARQ LINK REQUEST status message 
		
		//indicates an incoming link request to either MYCALL ($8027 $8000), or MYALTCALL ($8027 $8001).

		if (Len < 2) return;		// Wait for more

		// Don't need to do anything (but may eventally use ALTCALL as an APPLCALL
		Used = 2;
		break;

	case 0x2D:						// FSK ARQ Link Request status message
		
		// $802D $8001 $8000 CLOVER Link Request (not implemented)
		// $802D $8002 $8000 AMTOR CCIR-476 Link Request
		// $802D $8003 $8000 AMTOR CCIR-625 Link Request
		// $802D $8004 $8000 P-MODE Link Request

		if (Len < 3) return;		// Wait for more

		// Don't need to do anything (but may save Session type later

		Used = 3;
		break;


	case 0x20:			// Clover Linked with - Call Connected
	case 0x29:			// The Linked 476 message indicates the start of a CCIR 476 linked session.
	case 0x2A:			// The Linked 625 message indicates the start of a CCIR 625 linked session to <CALL>.
	case 0x2B:			// P-MODE link to

		// As the reply is variable, make sure we have the terminating NULL

		if (memchr(TNC->CmdBuffer, 0, Len) == NULL)
			return;					// Wait for more

		Call = &TNC->CmdBuffer[1];
		Used = strlen(Call) + 2;	// Opcode and Null

		HALConnected(TNC, Call);

		break;

	case 0x23:						// Normal Disconnected - followed by $8000
	case 0x24:						// Link failed (any of the link errors)
	case 0x25:						// Signal Lost (LOS)

		if (Len < 2) return;		// Wait for more

		HALDisconnected(TNC);

		Used = 2;
		break;


		// Stream Switch Reports - we will need to do something with these if Echo as Sent is set
		// or we do something with the secondary port

	case 0x30:						// Switch to Receive Data characters
	case 0x31:						// Switch to Transmit Data characters
	case 0x32:						// Switch to RX data from secondary port
	case 0x33:						// Send TX data to modem
	case 0x34:						// Send TX data to secondary port

		Used = 1;
		break;


	case 0x7F:						// Error $80xx $80yy Error in command $80xx of type $80yy
									// $807F $80xx $8030 Invalid or unimplemented command code
									// $807F $80xx $8031 Invalid parameter value
									// $807F $80xx $8032 Not allowed when connected
									// $807F $80xx $8033 Not allowed when disconnected
									// $807F $80xx $8034 Not valid in this mode
									// $807F $80xx $8035 Not valid in this code
									// $807F $8096 $8036 EEPROM write error

		if (Len < 3) return;		// Wait for more

		Debugprintf("HAL Command Error Cmd %X Error %X", TNC->CmdBuffer[1], TNC->CmdBuffer[2]);

		Used = 3;
		break;

		// Following are all immediate commands - response is echo of command

	case 0x00:			// P Load LOD file
	case 0x01:			// P Load S28 file
	case 0x02:			//Check Unit Error Status
	case 0x03:			//F Check System Clock
	case 0x04:			//C Close PTT and transmit Clover waveform
	case 0x05:			//Open PTT and stop transmit test
	case 0x06:			//Immediate Abort (Panic Kill)
	case 0x07:			//Normal disconnect (wait for ACK)
	case 0x08:			//Software reset - restore all program defaults
	case 0x0A:			//Send CW ID
	case 0x0B:			//Close PTT and transmit Single Tone
	case 0x0C:			//F Normal OVER (AMTOR,P-MODE)
	case 0x0D:			//F Force RTTY TX (Baudot/ASCII)
	case 0x0E:			//F Go to RTTY RX (Baudot/ASCII)
	case 0x0F:			//Go to LOD/S28 file loader

	case SetMYCALL:		// Set MYCALL Response
	case 0x1E:			// Set MYALTCALL Response

	case 0x46:
	case 0x56:			// Expanded Link State Reports OFF/ON

	case 0x80:			//Switch to CLOVER mode
	case 0x81:			//Select AMTOR Standby
	case 0x82:			//Select AMTOR FEC
	case 0x83:			//Select P-MODE Standby
	case 0x84:			//Switch to FSK modes
	case 0x85:			//Select Baudot
	case 0x86:			//Select ASCII
	case 0x87:			//Forced OVER (AMTOR, P-MODE)
	case 0x88:			//Forced END (AMTOR, P-MODE)
	case 0x89:			//Force LTRS shift
	case 0x8A:			//Force FIGS shift
	case 0x8B:			//Send MARK tone
	case 0x8C:			//Send SPACE tone
	case 0x8D:			//Send MARK/SPACE tones
	case 0x8E:			//Received first character on line
	case 0x8F:			//Close PTT only (no tones)

	case SetTones:

		Used = 1;
		break;

	default:

		// We didn't recognise command, so don't know how long it is - disaster!

		Debugprintf("HAL Unrecognised Command %x", Opcode);
		TNC->CmdLen = 0;

		return;
	}

	if (Used == Len)
	{
		// All used - most likelt case

		TNC->CmdLen = 0;
		return;
	}

	// Move Command Down buffer, and reenter

	TNC->CmdLen =- Used;

	memmove(TNC->CmdBuffer, &TNC->CmdBuffer[Used], TNC->CmdLen);

	ProcessHALCmd(TNC);

}

/*










		}
	}

	if (Opcode == 2)
	{
		// Echoed Data

		TNC->BytesAcked += Len -1;
		ShowTraffic(TNC);

		return;
	}

	if (Opcode == 3)
	{
		// Received Data

		// Pass to Appl

		buffptr = Q_REM(&FREE_Q);
		if (buffptr == NULL) return;	// No buffers, so ignore

		Len--;							// Remove Header

		buffptr[1] = Len;				// Length
		TNC->BytesRXed += Len;
		memcpy(&buffptr[2], Buffer, Len);
		Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

		if (Stream == 0)
			ShowTraffic(TNC);

		return;
	}


	if (Opcode == 4)
	{
		// Link Status or Command Response

		TNC->CommandBusy = FALSE;

		if (TNC->InternalCmd)
		{
			// Process it

			if (TNC->InternalCmd == 'S')		// Status
			{
//				if (Msg[3] == 'P' && Msg[4] == 'G')
				{
					SetDlgItemText(TNC->hDlg, IDC_2, status[Msg[5] - 0x30]);
					
					TNC->TXRXState = Msg[6];

					if (Msg[6] == 'S')
						SetDlgItemText(TNC->hDlg, IDC_1, "Sender");
					else
						SetDlgItemText(TNC->hDlg, IDC_1, "Receiver");

					Msg[12] = 0;
					SetDlgItemText(TNC->hDlg, IDC_3, Msg);

					// Testing.. I think ZF returns buffers

					SendCmd(TNC, "OZF", 3);
					TNC->InternalCmd = 'Z';
					TNC->CommandBusy = TRUE;
				}
			
				return;
			}

			if (TNC->InternalCmd == 'Z')		// Buffers?
			{
				Msg[Len] = 0;
				SetDlgItemText(TNC->hDlg, IDC_4, &Msg[3]);
				return;
			}
			return;
		}

		// Reply to Manual command - Pass to Appl

		Stream = TNC->CmdStream;


		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore

		Buffer[Len - 1] = 13;
		Buffer[Len] = 0;

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"AEA} %s", Buffer);

		Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

		return;

	}

	if (Opcode == 5)
	{
		// Link Messages (Connect, etc)

		if (Stream == 15)
		{
			// SOH $5F X X $00 ETB data acknowledgement 

			if (Msg[1] == 'X' && Msg[2] == 'X' && Msg[3] == 0)
			{
				TNC->DataBusy = FALSE;

				// If nothing more to send, turn round link
						
				if (TNC->BPQtoPACTOR_Q == 0)		// Nothing following
				{
					SendCmd(TNC, "OAG", 3);
					TNC->InternalCmd = 'A';
					TNC->CommandBusy = TRUE;
				}
			}
			return;
		}

		if (strstr(Buffer, "Transmit data remaining"))
		{
			// Seems to cause problems - restart TNC

			TNC->TNCOK = FALSE;
			TNC->HostMode = 0;
			TNC->ReinitState = 0;

			return;
		}
*/
	
VOID HALDisconnected(struct TNCINFO * TNC)
{
	if ((TNC->Connecting | TNC->Connected) == 0)
	{
		// Not connected or Connecting. Probably response to going into Pactor Listen Mode

		return;
	}

	if (TNC->Connecting)
	{
		UINT * buffptr;

		// Connect Failed - actually I think HAL uses another code for connect failed, but leave here for now
			
		buffptr = Q_REM(&FREE_Q);
	
		if (buffptr)
		{
			buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Failure with %s\r", TNC->RemoteCall);

			Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
		}

		TNC->Connecting = FALSE;
		TNC->Connected = FALSE;				// In case!
		TNC->FramesQueued = 0;

		return;
	}

	// Must Have been connected - Release Session

	TNC->Connecting = FALSE;
	TNC->Connected = FALSE;		// Back to Command Mode
	TNC->ReportDISC = TRUE;		// Tell Node
	TNC->FramesQueued = 0;

	// Need to reset Pactor Call in case it was changed

	TNC->NeedPACTOR = 20;

}

BOOL HALConnected(struct TNCINFO * TNC, char * Call)
{
	char Msg[80];
	UINT * buffptr;
	char Status[80];

	UpdateMH(TNC, Call, '+');

	TNC->BytesRXed = TNC->BytesTXed = TNC->BytesAcked = 0;

	// Stop Scanner

	wsprintf(Msg, "%d SCANSTOP", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
		
	if (Rig_Command)
		Rig_Command(-1, Msg);

	ShowTraffic(TNC);

	if (TNC->PortRecord->ATTACHEDSESSIONS[0] == 0)
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
			return FALSE;					// Tables Full

		TNC->Attached = TRUE;

		strcpy(TNC->RemoteCall, Call);	// Save Text Callsign 

		ConvToAX25(Call, Session->L4USER);
		ConvToAX25(TNC->NodeCall, Session->L4MYCALL);
	
		Session->CIRCUITINDEX = Index;
		Session->CIRCUITID = NEXTID;
		NEXTID++;
		if (NEXTID == 0) NEXTID++;		// Keep non-zero

		TNC->PortRecord->ATTACHEDSESSIONS[0] = Session;

		Session->L4TARGET = TNC->PortRecord;
		Session->L4CIRCUITTYPE = UPLINK+PACTOR;
		Session->L4WINDOW = L4DEFAULTWINDOW;
		Session->L4STATE = 5;
		Session->SESSIONT1 = L4T1;
		Session->SESSPACLEN = 100;
		Session->KAMSESSION = 0;

		TNC->Connected = TRUE;			// Subsequent data to data channel
					
		wsprintf(Status, "%s Connected to %s Inbound", TNC->RemoteCall, TNC->NodeCall);
		SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

		// If an autoconnect APPL is defined, send it

		if (TNC->ApplCmd)
		{
			buffptr = Q_REM(&FREE_Q);
			if (buffptr == 0) return TRUE;			// No buffers, so ignore

			buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "%s\r", TNC->ApplCmd);
			Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

			return TRUE;
		}

		if (FULL_CTEXT)
		{
			char CTBuff[300] = "D20";
			int Len = CTEXTLEN, CTPaclen = 50;
			int Next = 0;

	/*		while (Len > CTPaclen)		// CTEXT Paclen
			{
						memcpy(&CTBuff[3], &CTEXTMSG[Next], CTPaclen);
						SendCmd(TNC, CTBuff, CTPaclen + 3);
						Next += CTPaclen;
						Len -= CTPaclen;
					}

					memcpy(&CTBuff[3], &CTEXTMSG[Next], Len);
					SendCmd(TNC, CTBuff, Len + 3);
				}
				return;
*/
		}
		else
		{
			// Connect Complete
			
			buffptr = Q_REM(&FREE_Q);
			if (buffptr == 0) return TRUE;			// No buffers, so ignore

			buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Connected to %s\r", Call);;

			Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
	
			TNC->Connecting = FALSE;
			TNC->Connected = TRUE;			// Subsequent data to data channel

			wsprintf(Status, "%s Connected to %s Outbound", TNC->NodeCall, TNC->RemoteCall);
				SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);

		}
	}
	return TRUE;
}	


VOID EncodeAndSend(struct TNCINFO * TNC, UCHAR * txbuffer, int Len)
{
	// Send A Packet With DLE Encoding Encoding

	TNC->TXLen = DLEEncode(txbuffer, TNC->TXBuffer, Len);

	WriteCommBlock(TNC);
}

VOID SendCmd(struct TNCINFO * TNC, UCHAR * txbuffer, int Len)
{
	// Send A Packet With COmmand Encoding (preceed each with 0x80

	int i,txptr=0;
	UCHAR * outbuff = TNC->TXBuffer;

	for (i=0; i<Len; i++)
	{
		outbuff[txptr++] = 0x80;
		outbuff[txptr++] = txbuffer[i];
	}

	TNC->TXLen = txptr;
	WriteCommBlock(TNC);
}

int	DLEEncode(UCHAR * inbuff, UCHAR * outbuff, int len)
{
	int i,txptr=0;
	UCHAR c;

	for (i=0;i<len;i++)
	{
		c=inbuff[i];
		
		switch (c)
		{
		case 0x80:
		case 0x81:

			outbuff[txptr++] = 0x81;
		}
		
		outbuff[txptr++] = c;
	}

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

