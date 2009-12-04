//
//	DLL to inteface SCS TNC in Pactor Mode to BPQ32 switch 
//
//	Uses BPQ EXTERNAL interface
//

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE

#include "windows.h"
#include <stdio.h>
#include <stdlib.h>

//#include <process.h>
//#include <time.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0

#include "KAMPactor.h"
#include "ASMStrucs.h"

#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#define EXTDLL			// Use GetModuleHandle instead of LoadLibrary 
#include "bpq32.h"
 
#define DllImport	__declspec(dllimport)
#define DllExport	__declspec(dllexport)

DllImport UINT CRCTAB;

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

BOOL MinimizetoTray = FALSE;
HANDLE hInstance;
HBRUSH bgBrush;

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
VOID ProcessDEDFrame(struct TNCINFO * TNC, UCHAR * rxbuff, int len);
VOID ProcessTermModeResponse(struct TNCINFO * TNC);
BOOL ReadConfigFile(char * filename);
int ProcessLine(char * buf);
VOID DoTNCReinit(struct TNCINFO * TNC);
VOID DoTermModeTimeout(struct TNCINFO * TNC);
BOOL CreatePactorWindow();

VOID ProcessPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKHOSTPacket(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);
VOID ProcessKNormCommand(struct TNCINFO * TNC, UCHAR * rxbuffer);
VOID ProcessHostFrame(struct TNCINFO * TNC, UCHAR * rxbuffer, int Len);

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

		// Read Config

		GetAPI();					// Load BPQ32
		ReadConfigFile("KAMPACTOR.CFG");

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
			TNC = &TNCInfo[i];
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
	struct TNCINFO * TNC = &TNCInfo[port];

	if (TNC->hDevice == (HANDLE) -1)
		return 0;							// Port not open

	switch (fn)
	{
	case 1:				// poll

		if (TNC->ReportDISC)
		{
			TNC->ReportDISC = FALSE;
			return -1;
		}
	
		DEDPoll(port);

		CheckRX(TNC);

		if (TNC->PACTORtoBPQ_Q !=0)
		{
			int datalen;
			
			buffptr=Q_REM(&TNC->PACTORtoBPQ_Q);

			datalen=buffptr[1];

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

		if(TNC->Connected)
			TNC->FramesOutstanding++;
		
		return (0);


	case 3:				// CHECK IF OK TO SEND

		if (TNC->FramesOutstanding > 5)
			return 1;
		else
			return 0;		// OK

	case 4:				// reinit

		return (0);

	case 5:				// Close

		CloseHandle(TNCInfo[port].hDevice);
		return (0);
	}

	return 0;

}

DllExport int APIENTRY ExtInit(EXTPORTDATA *  PortEntry)
{
	char msg[80];
	struct TNCINFO * TNC;
	int port;

	//
	//	Will be called once for each Pactor Port
	//	The COM port number is in IOBASE
	//

	wsprintf(msg,"KAM Pactor COM%d", PortEntry->PORTCONTROL.IOBASE);
	WritetoConsole(msg);

	port=PortEntry->PORTCONTROL.PORTNUMBER;
	TNC = &TNCInfo[port];

	TNC->PortRecord = PortEntry;

	MinimizetoTray = GetMinimizetoTrayFlag();

	CreatePactorWindow(TNC);

	OpenCOMMPort(TNC, PortEntry->PORTCONTROL.IOBASE, PortEntry->PORTCONTROL.BAUDRATE);

	return ((int)ExtProc);
}


static char ClassName[]="PACTORSTATUS";

#define BGCOLOUR RGB(236,233,216)

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	
	switch (message) { 

//		case WM_ACTIVATE:

//			SetFocus(hwndInput);
//			break;


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) {


		default:

			return 0;

		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

		case  SC_MINIMIZE: 

			if (MinimizetoTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_CTLCOLORDLG:
			return (LONG)bgBrush;

		 case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)bgBrush;
		}

		case WM_SIZING:
			
			return TRUE;


		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 


			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}

BOOL CreatePactorWindow(struct TNCINFO * TNC)
{
    WNDCLASS  wc;
	char Title[80];
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Key[80];
	char Size[80];

	if (TNC->hDlg)
	{
		ShowWindow(TNC->hDlg, SW_SHOWNORMAL);
		SetForegroundWindow(TNC->hDlg);
		return FALSE;							// Already open
	}

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON2) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	TNC->hDlg = CreateDialog(hInstance,ClassName,0,NULL);
	
	wsprintf(Title,"Pactor Status - COM%d", TNC->PortRecord->PORTCONTROL.IOBASE);

	SetWindowText(TNC->hDlg, Title);

	if (MinimizetoTray)
		AddTrayMenuItem(TNC->hDlg, Title);


	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
	
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);
	}

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(TNC->hDlg, &Rect);
	}

	MoveWindow(TNC->hDlg, Rect.left, Rect.top, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);

	ShowWindow(TNC->hDlg, SW_SHOWNORMAL);

	return TRUE;
}



 
VOID KISSCLOSE(int Port)
{ 
	DestroyTTYInfo(Port);
}

BOOL NEAR DestroyTTYInfo(int port)
{
   // force connection closed (if not already closed)

   CloseConnection(&TNCInfo[port]);

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
		wsprintf(buf,"COM%d Setup Failed %d ", Port, GetLastError());
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

	TNC->Timeout = 50;

	return TRUE;
}

VOID DEDPoll(int Port)
{
	struct TNCINFO * TNC = &TNCInfo[Port];
	UCHAR * Poll = TNC->TXBuffer;
	char Status[80];

	if (TNC->PortRecord->ATTACHEDSESSION)		// Connected to Node
		if (TNC->Connected)
			if (TNC->PortRecord->ATTACHEDSESSION->L4CIRCUITTYPE & UPLINK)
				wsprintf(Status, "Connected to %s Inbound", TNC->RemoteCall);
			else
				wsprintf(Status, "Connected to %s Outbound", TNC->RemoteCall);
		else
			if (TNC->Connecting)
				wsprintf(Status, "Connecting to %s", TNC->RemoteCall);
			else
				wsprintf(Status, "In Use");
	else
		wsprintf(Status, "Free");

	SetDlgItemText(TNC->hDlg, IDC_TNCSTATE, Status);


	if (TNC->Timeout)
	{
		TNC->Timeout--;
		
		if (TNC->Timeout)			// Still waiting
			return;

	//	TNC->Retries--;

	//	if(TNC->Retries)
	//	{
	//		WriteCommBlock(TNC);	// Retransmit Block
	//		return;
	//	}

		// Retried out.

		if (TNC->HostMode == 0)
		{
			DoTermModeTimeout(TNC);
			return;
		}

		// Retried out in host mode - Clear any connection and reinit the TNC

		Debugprintf("KAM PACTOR - Link to TNC Lost");
		TNC->TNCOK = FALSE;
		TNC->HostMode = 0;
		TNC->ReinitState = 0;
				
		wsprintf(Status,"COM%d Open but TNC not responding", TNC->PortRecord->PORTCONTROL.IOBASE);
		SetDlgItemText(TNC->hDlg, IDC_COMMSSTATE, Status);

		
		if (TNC->PortRecord->ATTACHEDSESSION)		// COnnected
		{
			TNC->Connected = FALSE;		// Back to Command Mode
			TNC->ReportDISC = TRUE;		// Tell Nod
		}
	}

	if (TNC->PortRecord->ATTACHEDSESSION == 0 && TNC->Connected)
	{
		// Node has disconnected - clear any connection
			
		UINT * buffptr;
		UCHAR * Poll = TNC->TXBuffer;

		TNC->Connected = FALSE;

		EncodeAndSend(TNC, "X", 1);			// ??Return to packet mode??
		TNC->NeedPACTOR = 50;				// Need to Send PACTOR command after 5 secs
		TNC->Timeout = 0;					//	Don't expect a response.
		
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
		return;
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
			EncodeAndSend(TNC, "C20PACTOR", 9);			// Back to Listen

		return;
	}

	// Send Data if avail, else send poll

	if (TNC->TNCOK && TNC->BPQtoPACTOR_Q)
	{
		int datalen;
		UCHAR TXMsg[1000] = "D20";
		INT * buffptr;
		UCHAR * MsgPtr;
		char Status[80];
			
		buffptr=Q_REM(&TNC->BPQtoPACTOR_Q);

		datalen=buffptr[1];
		MsgPtr = (UCHAR *)&buffptr[2];

		if (TNC->Connected)
		{
			memcpy(&TXMsg[3], buffptr + 2, datalen);
			EncodeAndSend(TNC, TXMsg, datalen + 3);
			ReleaseBuffer(buffptr);
			TNC->BytesTXed += datalen;
			wsprintf(Status, "RX %d TX %d ACKED %d ", TNC->BytesRXed, TNC->BytesTXed, TNC->BytesAcked);
			SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);

			if (TNC->BPQtoPACTOR_Q == 0)		// Nothing following
				EncodeAndSend(TNC, "E", 1);			// Changeover when all sent

			TNC->Timeout = 0;					//	Don't expect a response

			return;
		}
		else
		{
			// Command. Do some sanity checking and look for things to process locally

			datalen--;				// Exclude CR
			MsgPtr[datalen] = 0;	// Null Terminate
			_strupr(MsgPtr);


			if (MsgPtr[0] == 'C' && datalen > 2)	// Connect
			{
				// Convert C CALL to PACTOR CALL

				memcpy(TNC->RemoteCall, &MsgPtr[2], 9);
				TNC->Connecting = TRUE;
				datalen = wsprintf(TXMsg, "C20PACTOR %s", TNC->RemoteCall);
				datalen = wsprintf(TXMsg, "C1BC %s", TNC->RemoteCall);
//				EncodeAndSend(TNC, "X", 1);			// ??Return to packet mode??
				EncodeAndSend(TNC, TXMsg, datalen);
				TNC->InternalCmd = TRUE;			// So we dont send the reply to the user.
				ReleaseBuffer(buffptr);
				return;
			}

			if (memcmp(MsgPtr, "DISCONNECT", datalen) == 0)	// Disconnect
			{
				EncodeAndSend(TNC, "X", 1);			// ??Return to packet mode??
				TNC->Timeout = 0;					//	Don't expect a response
	
				TNC->Connecting = FALSE;
				TNC->ReportDISC = TRUE;
				TNC->NeedPACTOR = 50;
				ReleaseBuffer(buffptr);

				return;
			}
	
			// Other COmmand ??

			datalen = wsprintf(TXMsg, "C20%s", MsgPtr);
			EncodeAndSend(TNC, TXMsg, datalen);
			ReleaseBuffer(buffptr);
	
			TNC->InternalCmd = 0;

		}

		return;
	}

	// Need to poll data and control channel (for responses to commands)

	// Also check status if we have data buffered (for flow control)

	if (TNC->TNCOK)
	{
		if (TNC->IntCmdDelay == 50)
		{
			EncodeAndSend(TNC, "C20S", 4);
			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay--;

			return;
		}

		if (TNC->IntCmdDelay <=0)
		{
			EncodeAndSend(TNC, "?", 1);
			TNC->InternalCmd = TRUE;
			TNC->IntCmdDelay = 600;	// Every Minute
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

		TNC->Retries = 1;
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
			TNC->Retries = 1;
			TNC->ReinitState = 4;	// Need Reset 

			return;
		}
		
		end = strchr(start, 13);
		len = ++end - start;
		TNC->InitPtr = end;
		memcpy(Poll, start, len);

		TNC->TXLen = len;
		WriteCommBlock(TNC);

		TNC->Retries = 1;
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
		TNC->Retries = 1;

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

	// Any valid frame is an ACK

	TNC->TNCOK = TRUE;

	Len = KissDecode(Msg, Msg, Len);	// Remove KISS transparency

	//	See if Poll Reply or Data

	Msg[Len] = 0; // Terminate

	if (Msg[0] == 'E')					// Data Echo
	{
		TNC->BytesAcked += Len -3;
		wsprintf(Status, "RX %d TX %d ACKED %d ", TNC->BytesRXed, TNC->BytesTXed, TNC->BytesAcked);
		SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);

		if (TNC->BytesTXed - TNC->BytesAcked < 500)
			TNC->FramesOutstanding = 0;

		return;
	}

	if (Msg[0] == 'D')					// Data
	{
		// Pass to Appl

		buffptr = Q_REM(&FREE_Q);
		if (buffptr == NULL) return;			// No buffers, so ignore

		Len-=3;							// Remove Header

		buffptr[1] = Len;				// Length
		TNC->BytesRXed += Len;
		memcpy(&buffptr[2], Buffer, Len);
		Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
		wsprintf(Status, "RX %d TX %d ACKED %d ", TNC->BytesRXed, TNC->BytesTXed, TNC->BytesAcked);
		SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);

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

				char LastCmd = TNC->TXBuffer[4];

				if (LastCmd == 'S')		// Status
				{
					return;
				}
				return;
		}

		// Pass to Appl

		buffptr = Q_REM(&FREE_Q);

		if (buffptr == NULL) return;			// No buffers, so ignore

		buffptr[1] = wsprintf((UCHAR *)&buffptr[2],"Pactor} %s", Buffer);

		Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);

		return;
	}

	if (Msg[0] == 'I')					// ISS/IRS State
	{
		if (Msg[2] == '1')
			SetDlgItemText(TNC->hDlg, IDC_1, "Sender");
		else
			SetDlgItemText(TNC->hDlg, IDC_1, "Receiver");

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

		if (strstr(Buffer, "<PACTOR STANDBY>"))	// 
		{
			if ((TNC->Connecting | TNC->Connected) == 0)
			{
				// Not connected or Connecting. Probably response to going into Pactor Listen Mode

				return;
			}

			if (TNC->Connecting)
			{
				// Connect Failed
			
				buffptr = Q_REM(&FREE_Q);
				if (buffptr == 0) return;			// No buffers, so ignore

				buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Failure with %s\r", TNC->RemoteCall);

				Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
	
				TNC->Connecting = FALSE;
				TNC->Connected = FALSE;				// In case!

				return;
			}

			// Must Have been connected - Release Session

			TNC->Connecting = FALSE;
			TNC->Connected = FALSE;		// Back to Command Mode
			TNC->ReportDISC = TRUE;		// Tell Node

			return;
		}

		Call = strstr(Buffer, "<LINKED TO");

		if (Call)
		{	
			Call+=11;					// To Callsign
			Buffer[Len-4] = 0;

			TNC->BytesRXed = TNC->BytesTXed = TNC->BytesAcked = 0;

			wsprintf(Status, "RX %d TX %d ACKED %d ", TNC->BytesRXed, TNC->BytesTXed, TNC->BytesAcked);
			SetDlgItemText(TNC->hDlg, IDC_TRAFFIC, Status);

			if (TNC->PortRecord->ATTACHEDSESSION == 0)
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

				strcpy(TNC->RemoteCall, Call);	// Save Text Callsign 

				ConvToAX25(Call, Session->L4USER);
				ConvToAX25(GetNodeCall(), Session->L4MYCALL);
	
				Session->CIRCUITINDEX = Index;
				Session->CIRCUITID = NEXTID;
				NEXTID++;
				if (NEXTID == 0) NEXTID++;		// Keep non-zero

				TNC->PortRecord->ATTACHEDSESSION = Session;

				Session->L4TARGET = TNC->PortRecord;
				Session->L4CIRCUITTYPE = UPLINK+PACTOR;
				Session->L4WINDOW = L4DEFAULTWINDOW;
				Session->L4STATE = 5;
				Session->SESSIONT1 = L4T1;
				Session->SESSPACLEN = 100;

				TNC->Connected = TRUE;			// Subsequent data to data channel

				// If an autoconnect APPL is defined, send it

				if (TNC->ApplCmd)
				{
					buffptr = Q_REM(&FREE_Q);
					if (buffptr == 0) return;			// No buffers, so ignore

					buffptr[1] = wsprintf((UCHAR *)&buffptr[2], "%s\r", TNC->ApplCmd);
					Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
				}
				return;
			}
			else
			{

				// Connect Complete
			
				buffptr = Q_REM(&FREE_Q);
				if (buffptr == 0) return;			// No buffers, so ignore

				buffptr[1]  = wsprintf((UCHAR *)&buffptr[2], "*** Connected to %s\r", Call);;

				Q_ADD(&TNC->PACTORtoBPQ_Q, buffptr);
	
				TNC->Connecting = FALSE;
				TNC->Connected = TRUE;			// Subsequent data to data channel

				return;
			}
		}

	}

}

FILE *file;

BOOL ReadConfigFile(char * fn)
{
	char buf[256],errbuf[256];

	UCHAR Value[100];
	UCHAR * BPQDirectory;

	BPQDirectory=GetBPQDirectory();

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value,fn);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value,fn);
	}
		
	if ((file = fopen(Value,"r")) == NULL)
	{
		wsprintf(buf,"KAMPACTOR - Config file %s not found ",Value);
		WritetoConsole(buf);
		return (TRUE);			// Dont need it at the moment
	}

	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessLine(buf))
		{
			WritetoConsole("KAMPACTOR - Bad config record ");
			WritetoConsole(errbuf);
		}			
	}

	fclose(file);
	return (TRUE);
}
GetLine(char * buf)
{
loop:
	if (fgets(buf, 255, file) == NULL)
		return 0;

	if (buf[0] < 0x20) goto loop;
	if (buf[0] == '#') goto loop;
	if (buf[0] == ';') goto loop;

	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	buf[strlen(buf)] = 13;

	return 1;
}

ProcessLine(char * buf)
{
	char * ptr,* p_cmd;
	int BPQport;
	int len=510;
	struct TNCINFO * TNC;

	ptr = strtok(buf, " \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	ptr = strtok(NULL, " \t\n\r");

	BPQport=0;

	BPQport = atoi(ptr);


	if(BPQport > 0 && BPQport <17)
	{
		TNC = &TNCInfo[BPQport];

		p_cmd = strtok(NULL, " \t\n\r");
			
		if (p_cmd != NULL)
		{
			TNC->ApplCmd=_strdup(p_cmd);
		}

		// Read Initialisation lines

		TNC->InitScript = malloc(1000);

		TNC->InitScript[0] = 0;

		while(TRUE)
		{
			if (GetLine(buf) == 0)
				return TRUE;

			if (memcmp(buf, "****", 4) == 0)
				return TRUE;

			ptr = strchr(buf, ';');
			if (ptr)
			{
				*ptr++ = 13;
				*ptr = 0;
			}
			strcat (TNC->InitScript, buf);
		}
	}

	return (FALSE);
	
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
  

/*
ID: 1004_ke7xo
Date: 2009/11/28 12:27
Type: Private
From: ke7xo
To: g8bpq
Subject: Trace
Mbo: ke7xo
Body: 2192

cmd=  XFLOW OFF
reply=XFLOW was OFF
cmd=  ECHO ON
reply=ECHO was ON
cmd=  XMITECHO ON
reply=XMITECHO was ON
cmd=  TXFLOW OFF
reply=TXFLOW was OFF
cmd=  XFLOW OFF
reply=XFLOW was OFF
cmd=  TRFLOW OFF
reply=TRFLOW was OFF
cmd=  AUTOCR 0
reply=AUTOCR was 0
cmd=  AUTOLF OFF
reply=AUTOLF was OFF
cmd=  CRADD OFF
reply=CRADD was OFF
cmd=  MAXUSERS 10/10
reply=MAXUSERS was 10/10
cmd=  CRSUP OFF/OFF
reply=CRSUP was OFF/OFF
cmd=  LFADD OFF/OFF
reply=LFADD was OFF/OFF
cmd=  LFSUP OFF/OFF
reply=LFSUP was OFF/OFF
cmd=  ARQID 0
reply=ARQID was 0
cmd=  ARQBBS OFF
reply=ARQBBS was OFF
cmd=  PTHUFF ON
reply=PTHUFF was ON
cmd=  SHIFT MODEM
reply=SHIFT was MODEM
cmd=  SPACE 3000
reply=SPACE was 1600
cmd=  MARK 1400
reply=MARK was 1400
cmd=  SPACE 1600
reply=SPACE was 3000
cmd=  INV ON
reply=INVERT was OFF
cmd=  MYPT ke7xo
reply=ok
cmd=  MON OFF/OFF
reply=MONITOR was OFF/OFF
cmd=  PACTOR
reply=ok
cmd=  MYPT ke7xo
reply=ok
<PACTOR STANDBY>
2009/11/28 12:15:00 KAM-XL modem released
2009/11/28 12:16:58 KAM-XL modem initialized OK
cmd=  XFLOW OFF
reply=XFLOW was OFF
cmd=  ECHO ON
reply=ECHO was ON
cmd=  XMITECHO ON
reply=XMITECHO was ON
cmd=  TXFLOW OFF
reply=TXFLOW was OFF
cmd=  XFLOW OFF
reply=XFLOW was OFF
cmd=  TRFLOW OFF
reply=TRFLOW was OFF
cmd=  AUTOCR 0
reply=AUTOCR was 0
cmd=  AUTOLF OFF
reply=AUTOLF was OFF
cmd=  CRADD OFF
reply=CRADD was OFF
cmd=  MAXUSERS 10/10
reply=MAXUSERS was 10/10
cmd=  CRSUP OFF/OFF
reply=CRSUP was OFF/OFF
cmd=  LFADD OFF/OFF
reply=LFADD was OFF/OFF
cmd=  LFSUP OFF/OFF
reply=LFSUP was OFF/OFF
cmd=  ARQID 0
reply=ARQID was 0
cmd=  ARQBBS OFF
reply=ARQBBS was OFF
cmd=  PTHUFF ON
reply=PTHUFF was ON
cmd=  SHIFT MODEM
reply=SHIFT was MODEM
cmd=  SPACE 3000
reply=SPACE was 1600
cmd=  MARK 1400
reply=MARK was 1400
cmd=  SPACE 1600
reply=SPACE was 3000
cmd=  INV ON
reply=INVERT was OFF
cmd=  MYPT ke7xo
reply=ok
cmd=  MON OFF/OFF
reply=MONITOR was OFF/OFF
cmd=  PACTOR
reply=ok
cmd=  MYPT ke7xo
reply=ok
<PACTOR STANDBY>
2009/11/28 12:18:36 Calling G8BPQ

cmd=  PACTOR G8BPQ
reply=ok
2009/11/28 12:18:39 Connected to G8BPQ
<LINKED TO G8BPQ>
1KE7XO
*/