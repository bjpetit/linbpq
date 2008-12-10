// Kantronics Host Mode Emulator for BPQ32 Switch
//

//	Version 1.0.0 December 2008

//		First Version

#include "stdafx.h"
#include "bpqkanthost.h"
#include "bpq32.h"
#include "GetVersion.h"

// Global Variables:
HINSTANCE hInst;								// current instance

char ClassName[]="KHOSTMAINWINDOW";					// the main window class name

HWND MainWnd;

char szBuff[80];

int CurrentConnections = 5;

HANDLE hControl;		// BPq Serial Driver Control Device Handle

struct ConnectionInfo Connections[6];
struct StreamInfo Streams[6];

//	Variables for reassembling host mode packets

BOOL PartPacket = FALSE;
int	PartLen;
UCHAR *	PartPtr;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
BOOL Initialise();
int DoStateChange(int Stream);
int DoReceivedData(int Stream);
int DoMonitorData(int Stream);
int Connected(int Stream);
int Disconnected(int Stream);
int SaveConfig(HWND hWnd);
int Refresh();

HANDLE BPQOpenSerialControl(ULONG * lasterror);
int BPQSerialAddDevice(HANDLE hDevice,ULONG * port,ULONG * result);
int BPQSerialDeleteDevice(HANDLE hDevice,ULONG * port,ULONG * result);
HANDLE BPQOpenSerialPort( int port,DWORD * lasterror);
int BPQSerialSetCTS(HANDLE hDevice);
int BPQSerialSetDSR(HANDLE hDevice);
int BPQSerialSetDCD(HANDLE hDevice);
int BPQSerialClrCTS(HANDLE hDevice);
int BPQSerialClrDSR(HANDLE hDevice);
int BPQSerialClrDCD(HANDLE hDevice);
int BPQSerialSendData(HANDLE hDevice,UCHAR * Message,int MsgLen);
int BPQSerialGetData(HANDLE hDevice,UCHAR * Message,int BufLen, ULONG * MsgLen);
int BPQSerialGetQCounts(HANDLE hDevice,ULONG * RXCount, ULONG * TXCount);
int BPQSerialGetDeviceList(HANDLE hDevice,ULONG * Slot,ULONG * Port);
int BPQSerialIsCOMOpen(HANDLE hDevice,ULONG * Count);
int BPQSerialGetDTRRTS(HANDLE hDevice,ULONG * Flags);

BOOL InitBPQStream(int Port,int * Stream);

VOID ProcessPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
VOID ProcessKPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
VOID ProcessKHOSTPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
VOID ProcessKNormCommand(struct ConnectionInfo * conn, UCHAR * rxbuffer);

//	Note that Kantronics host Mode uses KISS format Packets (without a KISS COntrol Byte)

VOID SendKISSData(struct ConnectionInfo * conn, UCHAR * txbuffer, int Len);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	KissDecode(UCHAR * inbuff, UCHAR * outbuff, int len);

BOOL ReleaseBPQStream(int Stream);

VOID CALLBACK TimerProc();
TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;
UINT_PTR TimerHandle = 0;


BOOL cfgMinToTray;

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	int i, j;
	ULONG Errorval;
	struct ConnectionInfo * conn;
	struct StreamInfo * channel;
	int retCode,disp;
	char Key[20];
	HKEY hKey=0;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//	Closing

	KillTimer(NULL,TimerHandle);

	if (cfgMinToTray) DeleteTrayMenuItem(MainWnd);

	// Open Registry to save Default Mode (HOST/TERM) Flag

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQKHOST",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey, &disp);


	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];
	
		if (conn->hDevice > 0)
		{
			BPQSerialClrCTS(conn->hDevice);
			BPQSerialClrDSR(conn->hDevice);
			BPQSerialClrDCD(conn->hDevice);
		
			CloseHandle(conn->hDevice);

			wsprintf(Key,"Port%dMode",i);
			RegSetValueEx(hKey,Key,0,REG_DWORD,(BYTE *)&conn->InHostMode,4);

			conn->nextMode = conn->InHostMode;

			for (j=1; j <= conn->numChannels; j++)
			{
				channel = conn->Channels[j];
				ReleaseBPQStream(channel->BPQStream);
			}

			if (conn->Created) BPQSerialDeleteDevice(hControl, &conn->ComPort, &Errorval);
		}
	}
	
	RegCloseKey(hKey);

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  connENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//

#define BGCOLOUR RGB(236,233,216)

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASS  wc;
	HBRUSH bgBrush;

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE( BPQICON ) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 
	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	return RegisterClass(&wc);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//

HWND hWnd;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int err;
	char Title[80];

	hInst = hInstance; // Store instance handle in our global variable

	hWnd=CreateDialog(hInst,ClassName,0,NULL);


   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
    //  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
	   err=GetLastError();
      return FALSE;
   }

   MainWnd=hWnd;

	GetVersionInfo(NULL);

	wsprintf(Title,"BPQ Kantronics Hostmode Emulator Version %s", VersionString);

	SetWindowText(hWnd,Title);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   	BPQMsg = RegisterWindowMessage(BPQWinMsg);

   	return Initialise();
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	int state,change;

	if (message == BPQMsg)
	{
		if (lParam & BPQMonitorAvail)
		{
			DoMonitorData(wParam);
			return 0;
		}
		if (lParam & BPQDataAvail)
		{
			DoReceivedData(wParam);
			return 0;
		}
		if (lParam & BPQStateChange)
		{

			//	Get current Session State. Any state changed is ACK'ed
			//	automatically. See BPQHOST functions 4 and 5.
	
			SessionState(wParam, &state, &change);
		
			if (change == 1)
			{
				if (state == 1)
	
				// Connected
			
					Connected(wParam);	
				else
					Disconnected(wParam);
			}
		}
		return 0;
	}

	switch (message)
	{

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		switch (wmId)
		
		case TN_SAVE:

			SaveConfig(hWnd);

			break;

	case WM_SIZE:

		if (wParam == SIZE_MINIMIZED)
			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);

		return (0);

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


BOOL Initialise()
{
	int i, j, resp;
	ULONG Errorval;
	struct ConnectionInfo * conn;
	struct StreamInfo * channel;
	int retCode,Type,Vallen;
	char Key[20];

	HKEY hKey=0;
	
	cfgMinToTray = GetMinimizetoTrayFlag();

	// Make Sure BPQVCOM is available
	
	hControl = BPQOpenSerialControl(&Errorval);

	if (hControl == (HANDLE) -1)
	{
		MessageBox (MainWnd, "BPQ Virtual COM Driver Control Channel Open Failed","",0);
		return FALSE;
	}

	// Get config from Registry 

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
		"SOFTWARE\\G8BPQ\\BPQ32\\BPQKHOST",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

//'   Get Params from Registry

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		wsprintf(Key,"Port%dStreams",i);
		Vallen=4;
		retCode = RegQueryValueEx(hKey,Key,0,			
			(ULONG *)&Type,(UCHAR *)&conn->numChannels,(ULONG *)&Vallen);

		if (conn->numChannels)
		{
			wsprintf(Key,"Port%d",i);
			Vallen=4;
			retCode = RegQueryValueEx(hKey,Key,0,			
				(ULONG *)&Type,(UCHAR *)&conn->ComPort,(ULONG *)&Vallen);

			wsprintf(Key,"Port%dMask",i);
			Vallen=4;
			retCode = RegQueryValueEx(hKey,Key,0,			
				(ULONG *)&Type,(UCHAR *)&conn->ApplMask,(ULONG *)&Vallen);
	
			wsprintf(Key,"Port%dMode",i);
			Vallen=4;
			retCode = RegQueryValueEx(hKey,Key,0,			
				(ULONG *)&Type,(UCHAR *)&conn->InHostMode,(ULONG *)&Vallen);
		}

	}

	RegCloseKey(hKey);

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		if (conn->numChannels == 0 ) continue;

		conn->hDevice = BPQOpenSerialPort(conn->ComPort, &Errorval);
        
		if (conn->hDevice == (HANDLE) -1 && Errorval == 2)
		{
			//' Not found, so create
           
			resp = BPQSerialAddDevice(hControl, &conn->ComPort, &Errorval);
			conn->Created = resp;
         		
			conn->hDevice = BPQOpenSerialPort(conn->ComPort, &Errorval);
		}			

		if (conn->hDevice != (HANDLE) -1)
		{
			wsprintf(conn->PortLabel,"Virtual COM%d", conn->ComPort);
                        
			resp = BPQSerialSetCTS(conn->hDevice);
			resp = BPQSerialSetDSR(conn->hDevice);
            
			conn->CTS = 1;
			conn->DSR = 1;

			for (j = 0; j <= conn->numChannels; j++)
			{
				// Use Stream zero for defaults
				
				conn->Channels[j] = malloc(sizeof (struct StreamInfo));
				channel = conn->Channels[j];
				channel->BPQStream = 0;
				channel->Connected = FALSE;
				channel->MYCall[0] = 0;

				if (i > 0) InitBPQStream(i, &conn->Channels[j]->BPQStream);            
			}
		}
		else
			wsprintf(conn->PortLabel,"Open Failed", conn->ComPort);

	}

	TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);

	Refresh();

	if (cfgMinToTray)
	
		AddTrayMenuItem(MainWnd, "KHOST Emulator");

	return TRUE;
}

int Refresh()
{
	struct ConnectionInfo * conn;
	int i;
	char Buff[10];

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		if (conn->ComPort > 0)
		{

		SendDlgItemMessage(MainWnd,TN_Check+i,BM_SETCHECK,(WPARAM)conn->PortEnabled,0);
		SendDlgItemMessage(MainWnd,TN_Label+i,WM_SETTEXT,0,(LPARAM)&conn->PortLabel);
		SendDlgItemMessage(MainWnd,TN_BPQ+i,WM_SETTEXT,0,(LPARAM) _ltoa(conn->ApplMask,Buff,10));
		SendDlgItemMessage(MainWnd,TN_TYPE+i,WM_SETTEXT,0,(LPARAM) _ltoa(conn->numChannels,Buff,10));
		SendDlgItemMessage(MainWnd,TN_COM+i,WM_SETTEXT,0,(LPARAM) _ltoa(conn->ComPort,Buff,10));
		SendDlgItemMessage(MainWnd,TN_CTS+i,BM_SETCHECK,(WPARAM)conn->CTS,0);
		SendDlgItemMessage(MainWnd,TN_RTS+i,BM_SETCHECK,(WPARAM)conn->RTS,0);
		SendDlgItemMessage(MainWnd,TN_DCD+i,BM_SETCHECK,(WPARAM)conn->DCD,0);
		SendDlgItemMessage(MainWnd,TN_DSR+i,BM_SETCHECK,(WPARAM)conn->DSR,0);
		SendDlgItemMessage(MainWnd,TN_DTR+i,BM_SETCHECK,(WPARAM)conn->DTR,0);
		}
	}


	return 0;
}

VOID CALLBACK TimerProc()
{
	struct ConnectionInfo * conn;
	struct StreamInfo * channel;

	int i, j, ConCount, ModemStat;
	int RXCount, TXCount, Read, resp;

	CheckTimer();			// Make sure BPQ32 TImer Process is running

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		ConCount = 0;
    
		BPQSerialIsCOMOpen(conn->hDevice, &ConCount);
    
		if (conn->PortEnabled == 1 && ConCount == 0)
		{
			//' Connection has just closed - disconnect all streams
        
			for (j = 1; j <= conn->numChannels; j++)
			{
				channel = conn->Channels[j];
				SessionControl(channel->BPQStream, 2, 0);
				SetAppl(channel->BPQStream, 0, 0);
			}

			conn->PortEnabled = FALSE;
		}

        if (conn->PortEnabled != ConCount)
		{
			conn->PortEnabled = ConCount;

			// Application has just connected - Set APPLMASK to allow incoming connects

			for (j = 1; j <= conn->numChannels; j++)
			{
				channel = conn->Channels[j];
				SetAppl(channel->BPQStream, 2, conn->ApplMask);
			}

       		Refresh();
		}

		resp = BPQSerialGetQCounts(conn->hDevice, &RXCount, &TXCount);
                
		if (RXCount > 0)
		{
			// If we have a partial packet, append this data to it

			resp = BPQSerialGetData(conn->hDevice, &conn->RXBuffer[conn->RXBPtr], 1000 - conn->RXBPtr, &Read);

			conn->RXBPtr += Read;
			ProcessPacket(conn, (UCHAR *)&conn->RXBuffer, conn->RXBPtr);
		}
       
		BPQSerialGetDTRRTS(conn->hDevice,&ModemStat);
			
		if ((ModemStat & 1) != conn->DTR)
		{
			conn->DTR=!conn->DTR;
			Refresh();
		}

		if ((ModemStat & 2) >> 1 != conn->RTS)
		{
			conn->RTS=!conn->RTS;
			Refresh();
		}

	}


	return;

}
int SaveConfig(HWND hWnd)
{
	int i;
	int retCode,disp;
	char Key[20];
	BOOL OK1;
	int Port, ApplMask, Streams;
	BOOL DUFF=FALSE;
	HKEY hKey=0;

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQKHOST",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

	if (retCode != ERROR_SUCCESS) return 0;

	for (i = 1; i <= CurrentConnections; i++)
	{
		Port = 0;

		Port=GetDlgItemInt(MainWnd,TN_COM+i,&OK1,FALSE);

		if (Port < 0 || Port > 255)
		{
			SetDlgItemText(MainWnd,TN_COM+i,"?");
			DUFF=TRUE;
		}

		if (Port > 0)
		{
			Streams=GetDlgItemInt(MainWnd,TN_TYPE+i,&OK1,FALSE);

			if (!OK1 || Streams < 1 || Streams > 26)
			{
				SetDlgItemText(MainWnd,TN_TYPE+i,"?");
				DUFF=TRUE;
			}

			ApplMask=GetDlgItemInt(MainWnd,TN_BPQ+i,&OK1,FALSE);

			if (!OK1 || ApplMask < 0 || ApplMask > 255)
			{
				SetDlgItemText(MainWnd,TN_BPQ+i,"?");
				DUFF=TRUE;
			}
		
			if (!DUFF)
			{
				wsprintf(Key,"Port%d",i);
				RegSetValueEx(hKey,Key,0,REG_DWORD,(BYTE *)&Port,4);

				wsprintf(Key,"Port%dStreams",i);
				RegSetValueEx(hKey,Key,0,REG_DWORD,(BYTE *)&Streams,4);

				wsprintf(Key,"Port%dMask",i);
				RegSetValueEx(hKey,Key,0,REG_DWORD,(BYTE *)&ApplMask,4);
			}
		}
		else		// if Port = 0
		{
			wsprintf(Key,"Port%d",i);
			RegDeleteValue(hKey,Key);		// Clear
			
			wsprintf(Key,"Port%dStreams",i);
			RegDeleteValue(hKey,Key);		// Clear
			
			wsprintf(Key,"Port%dMask",i);
			RegDeleteValue(hKey,Key);		// Clear

			wsprintf(Key,"Port%dMode",i);
			RegDeleteValue(hKey,Key);		// Clear
		}

	}

	RegCloseKey(hKey);

	MessageBox(MainWnd,"You must restart BPQKHOST for changes to be actioned","BPQKHOST",0);

	return 0;
}

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02

// Kantronics Host Mode Stuff

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD


byte * EncodeCall(byte * Call)
{
	static char axcall[10];

	ConvToAX25(Call, axcall);
	return &axcall[0];

}

VOID ProcessPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len)
{
	UCHAR * FendPtr;
	int NewLen;
	
	if (!conn->InHostMode)
	{
		//	In Terminal Mode - Pass to Term Mode Handler
		
		ProcessKPacket(conn, rxbuffer, Len);
		return;
	}

	//	Split into KISS Packets. By far the most likely is a single KISS frame
	//	so treat as special case

	if (!(rxbuffer[0] == FEND))
	{
		// Getting Non Host Data in Host Mode - Appl will have to sort the mess
		// Discard any data

		conn->RXBPtr = 0;
		return;
	}

	conn->RXBPtr = 0;				// Assume we will use all data in buffer - will reset if part packet received
	
	FendPtr = memchr(&rxbuffer[1], FEND, Len-1);
	
	if (FendPtr == &rxbuffer[Len-1])
	{
		ProcessKHOSTPacket(conn, &rxbuffer[1], Len - 2);
		return;
	}

	if (FendPtr == NULL)
	{
		// We have a partial Packet - Save it

		conn->RXBPtr = Len;
		memcpy(&conn->RXBuffer[0], rxbuffer, Len);
		return;
	}
		
	// Process the first Packet in the buffer

	NewLen =  FendPtr - rxbuffer -1;
	ProcessKHOSTPacket(conn, &rxbuffer[1], NewLen );
	
	// Loop Back

	ProcessPacket(conn, FendPtr+1, Len - NewLen -2);
	return;

}

VOID ProcessKPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len)
{
	UCHAR Char;
	PUCHAR cmdStart;
	UINT BytesLeft;

	// we will often get a whole connamd at once, but may not, so be prepared to receive char by char
	//	Could also get more than one command per packet

	cmdStart = rxbuffer;
	
	while (Len > 0)
	{
		Char = *(rxbuffer++);
		Len--;

//		if (conn->TermPtr > 120) conn->TermPtr = 120;	// Prevent overflow 

		if (conn->Echo) BPQSerialSendData(conn->hDevice, &Char, 1);

		if (Char == 0x0d)
		{
			// We have a command line
		
			*(rxbuffer-1) = 0;
			ProcessKNormCommand(conn, cmdStart);

			cmdStart = rxbuffer;
		}
	}

	// if we have a residue, copy to front of buffer

	if (cmdStart == (PUCHAR)&conn->RXBuffer) return;		// Haven't used any

	BytesLeft = rxbuffer - cmdStart;

	conn->RXBPtr = BytesLeft;

	if (BytesLeft == 0) return;						// Used it all

	memcpy(&conn->RXBuffer[0], cmdStart, BytesLeft);
}

VOID ProcessKNormCommand(struct ConnectionInfo * conn, UCHAR * rxbuffer)
{
//	UCHAR CmdReply[]="C00";
	UCHAR ResetReply[] = "\xC0\xC0S00\xC0";
	int Len;

	char seps[] = " \t\r";
	char * Command, * Arg1, * Arg2;
	char * Context;

	if (conn->Channels[1]->Connected)
	{
		Len = strlen(rxbuffer);
		rxbuffer[Len] = 0x0d;
		SendMsg(conn->Channels[1]->BPQStream, rxbuffer, Len+1);
		return;
	}

    Command = strtok_s(rxbuffer, seps, &Context);
    Arg1 = strtok_s(NULL, seps, &Context);
    Arg2 = strtok_s(NULL, seps, &Context);

	if (Command == NULL)
	{
		BPQSerialSendData(conn->hDevice, "cmd:", 4);
		return;
	}
		
	if (_stricmp(Command, "RESET") == 0)
	{
		if (conn->nextMode)		
			BPQSerialSendData(conn->hDevice, ResetReply, 6);
		else
			BPQSerialSendData(conn->hDevice, "cmd:", 4);

		conn->InHostMode = conn->nextMode;

		return;
	}

	if (_stricmp(Command, "K") == 0)
	{
		SessionControl(conn->Channels[1]->BPQStream, 1, 0);
		return;
	}

	if (_memicmp(Command, "INT", 3) == 0)
	{
		if (Arg1)
		{
			if (_stricmp(Arg1, "HOST") == 0)
				conn->nextMode = TRUE;
			else
				conn->nextMode = FALSE;
		}

		BPQSerialSendData(conn->hDevice, "INTFACE was TERMINAL\r", 21);
		BPQSerialSendData(conn->hDevice, "cmd:", 4);
		return;
	}

//cmd:

//INTFACE HOST
//INTFACE was TERMINAL
//cmd:RESET
//ÀÀS00À
//ÀC20XFLOW OFFÀ


	//SendKISSData(conn, CmdReply, 3);
	
	BPQSerialSendData(conn->hDevice, "cmd:", 4);


	//	Process Non-Hostmode Packet

	return;
}
int FreeBytes = 999;

VOID ProcessKHOSTPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len)
{
	struct StreamInfo * channel;
	UCHAR Command[80];
	UCHAR Reply[400];
	UCHAR TXBuff[400];
	UCHAR CmdReply[]="C10";

	UCHAR Chan, Stream;
	int j, TXLen, StreamNo;
	
	char * Cmd, * Arg1, * Arg2, * Arg3;
	char * Context;
	char seps[] = " \t\r\xC0";
	int CmdLen;

	if ((Len == 1) && ((rxbuffer[0] == 'q') || (rxbuffer[0] == 'Q')))
	{
		// Force Back to Command Mode

		Sleep(3000);
		conn->InHostMode = FALSE;
		BPQSerialSendData(conn->hDevice, "\r\r\rcmd:", 7);
		return;
	}

	Chan = rxbuffer[1];
	Stream = rxbuffer[2];
	StreamNo = Stream == '0' ? 0 : Stream - '@';

	if (StreamNo > conn->numChannels)
	{
		SendKISSData(conn, "C10Invalid Stream", 17);
		return;		
	}

	switch (rxbuffer[0])
	{
	case 'C':

		// Command Packet. Extract Command

		if (Len > 80) Len = 80;
	
		memcpy(Command, &rxbuffer[3], Len-3);
		Command[Len-3] = 0;

		Cmd = strtok_s(Command, seps, &Context);
		Arg1 = strtok_s(NULL, seps, &Context);
		Arg2 = strtok_s(NULL, seps, &Context);
		Arg3 = strtok_s(NULL, seps, &Context);
		CmdLen = strlen(Cmd);

		if (_stricmp(Cmd, "S") == 0)
		{
			// Status

			FreeBytes = 0;

			if (conn->Channels[1]->Connected)
				if (TXCount(conn->Channels[1]->BPQStream) > 5)
					FreeBytes = 0;

			TXLen = wsprintf(Reply, "C00FREE BYTES %d ", FreeBytes);
//			TXLen = wsprintf(Reply, "C10");
			SendKISSData(conn, Reply, TXLen);

			for (j=1; j <= conn->numChannels; j++)
			{
				channel = conn->Channels[j];
			
				if (channel->Connected)
				{
					TXLen = wsprintf(Reply, "C10%c/V stream - CONNECTED to %s", j + '@', "SWITCH");
					SendKISSData(conn, Reply, TXLen);
				}
//				else
//					TXLen = wsprintf(Reply, "C00%c/V stream - DISCONNECTED", j + '@');

			}
			return;
		}

		if (_memicmp(Cmd, "C", CmdLen) == 0)
		{
			int Port;
			struct StreamInfo * channel;
			int BPQStream;
			UCHAR * MYCall;

			// Connect. If command has a via string and first call is numeric use it as a port number

			if (Arg2 && Arg3)	
			{
				if (_memicmp(Arg2, "via", strlen(Arg2)) == 0)
				{
					// Have a via string

					Port = atoi(Arg3);
					{
						if (Port > 0)					// First Call is numeric
						{
							if (strlen(Context) > 0)	// More Digis
								TXLen = wsprintf(TXBuff, "c %s %s v %s\r", Arg3, Arg1, Context);
							else
								TXLen = wsprintf(TXBuff, "c %s %s\r", Arg3, Arg1);
						}
						else
						{
							// First Call Isn't Numeric. This won't work, as Digis without a port is invalid
							
							SendKISSData(conn, "C10Invalid via String (First must be Port)", 42);
							return;		

						}
					}
				}
			}
			else
			{
				TXLen = wsprintf(TXBuff, "%s %s\r", Command, Arg1);
			}

			Reply[0] = 'C';
			Reply[1] = Chan;
			Reply[2] = Stream;
			SendKISSData(conn, Reply, 3);

			channel = conn->Channels[StreamNo];
			BPQStream = channel->BPQStream;
			MYCall = (PUCHAR)&channel->MYCall;

			Connect(BPQStream);

			if (MYCall[0] > 0)
			{
				ChangeSessionCallsign(BPQStream, EncodeCall(MYCall));
				SendMsg(conn->Channels[StreamNo]->BPQStream, TXBuff, TXLen);
				return;
			}
		}

		if (_stricmp(Cmd, "D") == 0)
		{
			// Disconnect

			SessionControl(conn->Channels[StreamNo]->BPQStream, 2, 0);

			return;
		}

		if (_memicmp(Cmd, "INT", 3) == 0)
		{
			SendKISSData(conn, "C10INTFACE HOST", 15);
			return;
		}

		if (_stricmp(Cmd, "PACLEN") == 0)
		{
			SendKISSData(conn, "C10PACLEN 128/128", 17);
			return;
		}

		if (_memicmp(Cmd, "MYCALL", CmdLen > 1 ? CmdLen : 2) == 0)
		{
			if (strlen(Arg1) < 30)
				strcpy(conn->Channels[StreamNo]->MYCall, Arg1);
		}

		memcpy(Reply,CmdReply,3);
		SendKISSData(conn, Reply, 3);
		return;

	case 'D':

		// Data to send

			
		if (StreamNo > conn->numChannels) return;		// Protect

		TXLen = KissDecode(&rxbuffer[3], TXBuff, Len-3);
		SendMsg(conn->Channels[StreamNo]->BPQStream, TXBuff, TXLen);

		return;

	default:

		memcpy(Reply,CmdReply,3);
		SendKISSData(conn, Reply, 3);
		return;
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

VOID SendKISSData(struct ConnectionInfo * conn, UCHAR * txbuffer, int Len)
{
	// Send A Packet With KISS Encoding

	UCHAR EncodedReply[800];
	int TXLen;

	TXLen = KissEncode(txbuffer, EncodedReply, Len);

	BPQSerialSendData(conn->hDevice, EncodedReply, TXLen);

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


HANDLE BPQOpenSerialControl(ULONG * lasterror)
{            
	HANDLE hDevice;

	*lasterror=0;
	
	hDevice = CreateFile( "//./BPQControl", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (hDevice == (HANDLE) -1 )
	{
		*lasterror=GetLastError();
	}

	return hDevice;

}
int BPQSerialAddDevice(HANDLE hDevice,ULONG * port,ULONG * result)
{
	ULONG bytesReturned;

	return DeviceIoControl (hDevice,IOCTL_BPQ_ADD_DEVICE,port,4,result,4,&bytesReturned,NULL);
}

int BPQSerialDeleteDevice(HANDLE hDevice,ULONG * port,ULONG * result)
{
	ULONG bytesReturned;

	return DeviceIoControl (hDevice,IOCTL_BPQ_DELETE_DEVICE,port,4,result,4,&bytesReturned,NULL);
}


HANDLE BPQOpenSerialPort( int port,DWORD * lasterror)
{            
	char szPort[ 15 ];
	HANDLE hDevice;

  // load the COM prefix string and append port number
   
	*lasterror=0;
	
	wsprintf( szPort, "\\\\.\\BPQ%d",port) ;

	hDevice = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
	if (hDevice == (HANDLE) -1 )
	{
		*lasterror=GetLastError();
	}

   return hDevice;
}
 

int BPQSerialSetCTS(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSetDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(	hDevice,IOCTL_SERIAL_SET_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSetDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialClrCTS(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}
int BPQSerialClrDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialClrDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSendData(HANDLE hDevice,UCHAR * Message,int MsgLen)
{
	ULONG bytesReturned;

	if (MsgLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	return DeviceIoControl(hDevice,IOCTL_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialGetData(HANDLE hDevice,UCHAR * Message,int BufLen, ULONG * MsgLen)
{
	if (BufLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	return DeviceIoControl(hDevice,IOCTL_SERIAL_GETDATA,NULL,0,Message,BufLen,MsgLen,NULL);
                  
}

int BPQSerialGetQCounts(HANDLE hDevice,ULONG * RXCount, ULONG * TXCount)
{

	SERIAL_STATUS Resp;
	int MsgLen;
	int ret;

	ret = DeviceIoControl(hDevice,IOCTL_SERIAL_GET_COMMSTATUS,NULL,0,&Resp,sizeof(SERIAL_STATUS),&MsgLen,NULL);

    *RXCount=Resp.AmountInInQueue;
	*TXCount=Resp.AmountInOutQueue;

	return ret;

}

int BPQSerialGetDeviceList(HANDLE hDevice,ULONG * Slot,ULONG * Port)
{
	ULONG bytesReturned;

	return  DeviceIoControl (hDevice,IOCTL_BPQ_LIST_DEVICES,Slot,4,Port,4,&bytesReturned,NULL);

}




int BPQSerialIsCOMOpen(HANDLE hDevice,ULONG * Count)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_IS_COM_OPEN,NULL,0,Count,4,&bytesReturned,NULL);                
}
int BPQSerialGetDTRRTS(HANDLE hDevice,ULONG * Flags)
{
	ULONG bytesReturned;

	return DeviceIoControl(hDevice,IOCTL_SERIAL_GET_DTRRTS,NULL,0,Flags,4,&bytesReturned,NULL);                
}


BOOL InitBPQStream(int Port,int * Stream)
{
	int ret;
		
	if (*Stream == 0)
	{
		*Stream = FindFreeStream();

		if (*Stream == 255) return FALSE;

		BPQSetHandle(*Stream, hWnd);
	}
	else
	{
		ret = AllocateStream(*Stream);

		if (ret != 0) return FALSE;


	}


	return TRUE;

}

BOOL ReleaseBPQStream(int Stream)
{	

	if (Stream == 0) return FALSE;
	
	SetAppl(Stream, 0, 0);
	SessionControl(Stream, 2, 0);
	DeallocateStream(Stream);

	return TRUE;

}

int Connected(Stream)
{
	byte ConnectingCall[10];
	byte ApplCall[10]="";
	UCHAR Msg[50];
	int i, j, Len;
	struct ConnectionInfo * conn;
	struct StreamInfo * channel;

	GetCallsign(Stream, ConnectingCall);

	for (i=9;i>0;i--)
		if (ConnectingCall[i]==32)
			ConnectingCall[i]=0;

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		for (j=1; j <= conn->numChannels; j++)
		{
			channel = conn->Channels[j];

			if (channel->BPQStream == Stream)
			{
				if (conn->InHostMode)
				{
					Len = wsprintf (Msg, "S1%c*** CONNECTED to %s ", j + '@', ConnectingCall);
					SendKISSData(conn, Msg, Len);
				}
				else
				{
					Len = wsprintf (Msg, "*** CONNECTED to %s\r", ConnectingCall);
					BPQSerialSendData(conn->hDevice, Msg, Len);
					BPQSerialSetDCD(conn->hDevice);
					Refresh();
				}
				
				channel->Connected = TRUE;

				return 0;
			}
		}
	}

	// Unknown Stream

	return 0;
}

int Disconnected (Stream)
{
	UCHAR Msg[50];
	int i, j, Len;
	struct ConnectionInfo * conn;
	struct StreamInfo * channel;

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		for (j = 1; j <= conn->numChannels; j++)
		{
			channel = conn->Channels[j];

			if (channel->BPQStream == Stream)
			{
				if (conn->InHostMode)
				{
					Len = wsprintf (Msg, "S1%c*** DISCONNECTED", j + '@');
					SendKISSData(conn, Msg, Len);
				}
				else
				{
					BPQSerialSendData(conn->hDevice, "*** DISCONNECTED\r", 17); 
					BPQSerialClrDCD(conn->hDevice);
					Refresh();
				}
				channel->Connected = FALSE;

				return 0;
			}
		}
	}

	return 0;

}
int DoReceivedData(int Stream)
{
	byte Buffer[400];
	int len, count, i, j;
	struct ConnectionInfo * conn;
	struct StreamInfo * channel;

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = &Connections[i];

		for (j = 1; j <= conn->numChannels; j++)
		{
			channel = conn->Channels[j];

			if (channel->BPQStream == Stream)
			{
				do { 

					GetMsg(Stream, &Buffer[3], &len, &count);

					if (len > 0)
					{
						if (conn->InHostMode)
						{
							Buffer[0] = 'D';
							Buffer[1] = '1';
							Buffer[2] = j + '@';
							SendKISSData(conn, Buffer, len+3);
						}
						else
							BPQSerialSendData(conn->hDevice, &Buffer[3], len); 
					}
	  
				} while (count > 0);

				return 0;
			}
		}
	}
	return 0;
}

int DoMonitorData(int Stream)
{
	byte Buffer[500];
	int RawLen,Count;
	int Stamp;

	do
	{
		Stamp=GetRaw(Stream, Buffer, &RawLen, &Count );
	}

	while (Count > 0);

	return 0;
}






