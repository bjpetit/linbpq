// Kantronics Host Mode Emulator for BPQ32 Switch
//

//	Version 1.0.0 December 2008

//		First Version

//	Version 1.1.0 January 2009

//		Supports Win98 Virtual COM Ports
//		Add StartMinimized Option

#include "stdafx.h"
#include "bpqhostmodes.h"
#include "bpq32.h"
#include "GetVersion.h"

// Global Variables:
HINSTANCE hInst;								// current instance

char ClassName[]="KHOSTMAINWINDOW";					// the main window class name

HWND MainWnd;

char szBuff[80];

int CurrentConnections = 0;

#define MaxConnections 20

HANDLE hControl;		// BPq Serial Driver Control Device Handle

BOOL Win98 = FALSE;		// Running on Win98


struct ConnectionInfo * Connections[MaxConnections+1];

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
int AddLine(HWND hWnd);
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
int BPQSerialSetPollDelay(HANDLE hDevice, int PollDelay);

BOOL InitBPQStream(int Port,int * Stream);

VOID ProcessPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
VOID ProcessKPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
VOID ProcessKHOSTPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
VOID ProcessKNormCommand(struct ConnectionInfo * conn, UCHAR * rxbuffer);
VOID ProcessDEDPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len);
BOOL TfPut(struct ConnectionInfo * conn, UCHAR character); 
BOOL InitDED(struct ConnectionInfo * conn);
int PUTCHARx(struct ConnectionInfo * conn, UCHAR c);
int DOCOMMAND(struct ConnectionInfo * conn);


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
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQHOSTMODES",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey, &disp);


	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = Connections[i];
	
		if (conn->hDevice > 0)
		{
			BPQSerialClrCTS(conn->hDevice);
			BPQSerialClrDSR(conn->hDevice);
			BPQSerialClrDCD(conn->hDevice);
		
			if (!Win98)
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

	CloseHandle(hControl);
	
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

#define BGCOLOUR RGB(236,233,233)

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

HWND hWnd, hList;
HWND hCheck[MaxConnections+1];
HWND hLabel[MaxConnections+1];
HWND hCOM[MaxConnections+1];
HWND hStreams[MaxConnections+1];
HWND hMask[MaxConnections+1];
HWND hKant[MaxConnections+1];
HWND hDED[MaxConnections+1];
HWND hRTS[MaxConnections+1];
HWND hCTS[MaxConnections+1];
HWND hDTR[MaxConnections+1];
HWND hDSR[MaxConnections+1];
HWND hDCD[MaxConnections+1];
HFONT hFont;
LOGFONT LFTTYFONT ;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int err, ret, i=0;
	char Title[80];
	INITCOMMONCONTROLSEX init;

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

   
	 // setup default font information

   LFTTYFONT.lfHeight =			15;
   LFTTYFONT.lfWidth =          6 ;
   LFTTYFONT.lfEscapement =     0 ;
   LFTTYFONT.lfOrientation =    0 ;
   LFTTYFONT.lfWeight =         0 ;
   LFTTYFONT.lfItalic =         0 ;
   LFTTYFONT.lfUnderline =      0 ;
   LFTTYFONT.lfStrikeOut =      0 ;
   LFTTYFONT.lfCharSet =        DEFAULT_CHARSET  ;
   LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
   LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
   LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
   LFTTYFONT.lfPitchAndFamily = VARIABLE_PITCH ;
   lstrcpy( LFTTYFONT.lfFaceName, "MS Sans Serif" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;




	init.dwICC=ICC_STANDARD_CLASSES;
	init.dwSize=sizeof(init);
	ret=InitCommonControlsEx(&init);

/*	hList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT , "4", WS_CHILD | WS_BORDER | WS_VISIBLE,
                 60,183,25,20, hWnd, NULL, hInstance, NULL);


	hList = CreateWindow(WC_BUTTON , "Kant", BS_AUTORADIOBUTTON | WS_GROUP | WS_CHILD,
                 100,186,70,14, hWnd, NULL, hInstance, NULL);

   ShowWindow(hList, nCmdShow);

		hList = CreateWindow(WC_BUTTON , "DED", BS_AUTORADIOBUTTON | WS_CHILD,
                 180,186,100,14, hWnd, NULL, hInstance, NULL);

   ShowWindow(hList, nCmdShow);

	hList = CreateWindow(WC_BUTTON , "Kant", BS_AUTORADIOBUTTON | WS_GROUP | WS_CHILD,
                 100,216,70,14, hWnd, NULL, hInstance, NULL);

   ShowWindow(hList, nCmdShow);

		hList = CreateWindow(WC_BUTTON , "DED", BS_AUTOCHECKBOX  | WS_CHILD,
                 180,216,100,14, hWnd, NULL, hInstance, NULL);

		//171,118,49,14
   ShowWindow(hList, nCmdShow);
*/

	GetVersionInfo(NULL);

	wsprintf(Title,"BPQ Hostmodes Emulator Version %s", VersionString);

	SetWindowText(hWnd,Title);

	cfgMinToTray = GetMinimizetoTrayFlag();

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (cfgMinToTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);

   	BPQMsg = RegisterWindowMessage(BPQWinMsg);

   	return Initialise();
}
int CreateDialogLine(int i)
{
	int row = i * 30 + 10;

	hCheck[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 10,row+5,14,14, hWnd, NULL, hInst, NULL);

	hLabel[i] = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 30,row+5,85,22, hWnd, NULL, hInst, NULL);
	
	SendMessage(hLabel[i], WM_SETFONT,(WPARAM) hFont, 0);


//g_myStatic = CreateWindowEx(0, L"STATIC", L"Some static text", 
 //           WS_CHILD | WS_VISIBLE , 
  //          25, 125, 150, 20, hDlg, 0, 0, 0);


	hCOM[i] = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT , "", WS_CHILD | WS_BORDER | WS_VISIBLE,
                 115,row,25,22, hWnd, NULL, hInst, NULL);

	SendMessage(hCOM[i], WM_SETFONT,(WPARAM) hFont, 0);

	hStreams[i] = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT , "", WS_CHILD | WS_BORDER | WS_VISIBLE,
                 160,row,25,22, hWnd, NULL, hInst, NULL);

	SendMessage(hStreams[i], WM_SETFONT,(WPARAM) hFont, 0);

	hMask[i] = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT , "", WS_CHILD | WS_BORDER | WS_VISIBLE,
                 205,row,25,22, hWnd, NULL, hInst, NULL);

	SendMessage(hMask[i], WM_SETFONT,(WPARAM) hFont, 0);

	hKant[i] = CreateWindow(WC_BUTTON , "Kant", BS_AUTORADIOBUTTON | WS_GROUP | WS_CHILD | WS_VISIBLE ,
                 250,row+5,50,14, hWnd, NULL, hInst, NULL);

	SendMessage(hKant[i], WM_SETFONT,(WPARAM) hFont, 0);

	hDED[i] = CreateWindow(WC_BUTTON , "DED", BS_AUTORADIOBUTTON | WS_CHILD | WS_VISIBLE ,
                 305,row+5,50,14, hWnd, NULL, hInst, NULL);

	SendMessage(hDED[i], WM_SETFONT,(WPARAM) hFont, 0);

	hRTS[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 360,row+5,14,14, hWnd, NULL, hInst, NULL);

	hCTS[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 390,row+5,14,14, hWnd, NULL, hInst, NULL);

	hDCD[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 420,row+5,14,14, hWnd, NULL, hInst, NULL);

	hDSR[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 450,row+5,14,14, hWnd, NULL, hInst, NULL);

	hDTR[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 480,row+5,14,14, hWnd, NULL, hInst, NULL);

	return 0;
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
		{
		case TN_ADD:

			AddLine(hWnd);

			break;

		case TN_SAVE:

			SaveConfig(hWnd);

			break;
		}

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
	int numChannels, ComPort, DEDMode, InHostMode, ApplMask;
	RECT Rect;

	HKEY hKey=0;
	
#pragma warning(push)
#pragma warning(disable : 4996)

	if (HIBYTE(_winver) < 5)
		Win98 = TRUE;

#pragma warning(pop)

	// Get config from Registry 

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
		"SOFTWARE\\G8BPQ\\BPQ32\\BPQHOSTMODES",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

//'   Get Params from Registry

	for (i = 1; i <= MaxConnections; i++)
	{
		wsprintf(Key,"Port%dStreams",i);
		Vallen=4;
		numChannels = 0;
		retCode = RegQueryValueEx(hKey,Key,0,			
			(ULONG *)&Type,(UCHAR *)&numChannels,(ULONG *)&Vallen);

		if (numChannels)
		{
			wsprintf(Key,"Port%d",i);
			Vallen=4;
			retCode = RegQueryValueEx(hKey,Key,0,			
				(ULONG *)&Type,(UCHAR *)&ComPort,(ULONG *)&Vallen);

			wsprintf(Key,"Port%dMask",i);
			Vallen=4;
			retCode = RegQueryValueEx(hKey,Key,0,			
				(ULONG *)&Type,(UCHAR *)&ApplMask,(ULONG *)&Vallen);
	
			wsprintf(Key,"Port%dMode",i);
			Vallen=4;
			retCode = RegQueryValueEx(hKey,Key,0,			
				(ULONG *)&Type,(UCHAR *)&InHostMode,(ULONG *)&Vallen);

			wsprintf(Key,"Port%dDEDMode",i);
			Vallen=4;
			retCode = RegQueryValueEx(hKey,Key,0,			
				(ULONG *)&Type,(UCHAR *)&DEDMode,(ULONG *)&Vallen);

			conn = malloc(sizeof (struct ConnectionInfo));
			memset(conn, 0, sizeof (struct ConnectionInfo));
			Connections[++CurrentConnections] = conn;

			conn->ApplMask =ApplMask;
			conn->numChannels = numChannels;
			conn->ComPort = ComPort;
			conn->DEDMode = DEDMode;
			conn->InHostMode = InHostMode;

		}

	}

	RegCloseKey(hKey);

	// If running Win98 must open COM Device before opening the /vxd. Don't know why

	// Make Sure BPQVCOM is available
	
	if (!Win98)
	{

		hControl = BPQOpenSerialControl(&Errorval);

		if (hControl == (HANDLE) -1)
		{
			MessageBox (MainWnd, "BPQ Virtual COM Driver Control Channel Open Failed","",0);
			return FALSE;
		}
	}

	for (i = 1; i <= CurrentConnections; i++)
	{
		CreateDialogLine(i);

		conn = Connections[i];

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

			if (conn->DEDMode)
				BPQSerialSetPollDelay(conn->hDevice, 100);
			
			conn->CTS = 1;
			conn->DSR = 1;

			for (j = 0; j <= conn->numChannels; j++)
			{
				// Use Stream zero for defaults
				
				conn->Channels[j] = malloc(sizeof (struct StreamInfo));
				channel = conn->Channels[j];
				channel->Chan_TXQ = 0;
				channel->BPQStream = 0;
				channel->Connected = FALSE;
				channel->MYCall[0] = 0;

				if (i > 0)
					if (!conn->DEDMode)
						InitBPQStream(i, &conn->Channels[j]->BPQStream);            
			}

			if (conn->DEDMode)
				InitDED(conn);

		}
		else
			wsprintf(conn->PortLabel,"Open Failed", conn->ComPort);

	}

	if (Win98)
	{
		hControl = BPQOpenSerialControl(&Errorval);

		if (hControl == (HANDLE) -1)
		{
			MessageBox (MainWnd, "BPQVCOMm.vxd Driver Open Failed","",0);
			return FALSE;
		}
	}

	TimerHandle=SetTimer(NULL,0,100,lpTimerFunc);

	GetWindowRect(hWnd, &Rect);      
	SetWindowPos(hWnd, HWND_TOP, Rect.left, Rect.top, 550, CurrentConnections*30+130, 0);
	SetWindowPos(GetDlgItem(hWnd, TN_ADD), NULL, 180, CurrentConnections*30+50, 70, 30, 0);
	SetWindowPos(GetDlgItem(hWnd, TN_SAVE), NULL, 300, CurrentConnections*30+50, 80, 30, 0);

	Refresh();

	if (cfgMinToTray)
	
		AddTrayMenuItem(MainWnd, "Hostmodes Emulator");

	return TRUE;
}

int Refresh()
{
	struct ConnectionInfo * conn;
	int i;
	char Buff[10];

	for (i = 1; i <= CurrentConnections; i++)
	{
		conn = Connections[i];

		if (conn->ComPort > 0)
		{

		if (conn->DEDMode)
			SendMessage(hDED[i],BM_SETCHECK, 1,0);
		else
			SendMessage(hKant[i],BM_SETCHECK, 1,0);

		SendMessage(hCheck[i],BM_SETCHECK,(WPARAM)conn->PortEnabled,0);
		SendMessage(hLabel[i], WM_SETTEXT,0,(LPARAM)&conn->PortLabel);
		SendMessage(hMask[i],WM_SETTEXT,0,(LPARAM) _ltoa(conn->ApplMask,Buff,10));
		SendMessage(hStreams[i],WM_SETTEXT,0,(LPARAM) _ltoa(conn->numChannels,Buff,10));
		SendMessage(hCOM[i],WM_SETTEXT,0,(LPARAM) _ltoa(conn->ComPort,Buff,10));
		SendMessage(hCTS[i],BM_SETCHECK,(WPARAM)conn->CTS,0);
		SendMessage(hRTS[i],BM_SETCHECK,(WPARAM)conn->RTS,0);
		SendMessage(hDCD[i],BM_SETCHECK,(WPARAM)conn->DCD,0);
		SendMessage(hDSR[i],BM_SETCHECK,(WPARAM)conn->DSR,0);
		SendMessage(hDTR[i],BM_SETCHECK,(WPARAM)conn->DTR,0);
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
		conn = Connections[i];

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
			Refresh();

		}

        if (conn->PortEnabled != ConCount)
		{
			conn->PortEnabled = ConCount;

			// Application has just connected - Set APPLMASK to allow incoming connects

			for (j = 1; j <= conn->numChannels; j++)
			{
				channel = conn->Channels[j];
				SetAppl(channel->BPQStream, 2, conn->ApplMask);

				// if DED Mode, reset State Machine

				if (conn->DEDMode)
				{
					conn->HOSTSTATE = 0;
					conn->InHostMode = FALSE;
				}
			}

       		Refresh();
		}

		resp = BPQSerialGetQCounts(conn->hDevice, &RXCount, &TXCount);
                
		if (RXCount > 0)
		{
			// If we have a partial packet, append this data to it

			resp = BPQSerialGetData(conn->hDevice, &conn->RXBuffer[conn->RXBPtr], 1000 - conn->RXBPtr, &Read);

			conn->RXBPtr += Read;
			if (conn->DEDMode)
				ProcessDEDPacket(conn, (UCHAR *)&conn->RXBuffer, conn->RXBPtr);
			else
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

		if (conn->DEDMode)
		{
			int Len=0;
			UCHAR Message[1000];

			while (conn->RXCOUNT > 0)
			{
				Message[Len++]= *(conn->RXPOINTER++);

				conn->RXCOUNT--;

				if (conn->RXPOINTER == &conn->PCBUFFER[512])
					conn->RXPOINTER = (PUCHAR)&conn->PCBUFFER;

				if (Len > 900) 
				{
					BPQSerialSendData(conn->hDevice, Message, Len);
					Len = 0;
				}
			}
				
			if (Len > 0) 
			{
				BPQSerialSendData(conn->hDevice, Message, Len);
			}
		}
	}

	return;

}

int AddLine(HWND hWnd)
{
	struct ConnectionInfo * conn;
	RECT Rect;

	if (CurrentConnections == MaxConnections)
		return 0;

	conn = malloc(sizeof (struct ConnectionInfo));
	memset(conn, 0, sizeof (struct ConnectionInfo));
	Connections[++CurrentConnections] = conn;

	GetWindowRect(hWnd, &Rect);      
	SetWindowPos(hWnd, HWND_TOP, Rect.left, Rect.top, 550, CurrentConnections*30+130, 0);
	SetWindowPos(GetDlgItem(hWnd, TN_ADD), NULL, 180, CurrentConnections*30+50, 70, 30, 0);
	SetWindowPos(GetDlgItem(hWnd, TN_SAVE), NULL, 300, CurrentConnections*30+50, 80, 30, 0);

	CreateDialogLine(CurrentConnections);

	Refresh();

	return 0;
}

int SaveConfig(HWND hWnd)
{
	int i;
	int retCode,disp;
	char Key[20];
	BOOL DED;
	int Port, ApplMask, Streams, Len;
	BOOL DUFF=FALSE;
	HKEY hKey=0;
	char Val[80];

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQHOSTMODES",
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

		Len = SendMessage(hCOM[i],WM_GETTEXT, 80, (LPARAM) (LPCSTR) Val);
		Port = atoi(Val);


		if (Port < 0 || Port > 255)
		{
			SendMessage(hCOM[i],WM_SETTEXT,0,(LPARAM) "?");
			DUFF=TRUE;
		}

		if (Port > 0)
		{
			Len = SendMessage(hStreams[i],WM_GETTEXT, 80, (LPARAM) (LPCSTR) Val);
			Streams = atoi(Val);

			if (Streams < 1 || Streams > 26)
			{
				SendMessage(hStreams[i],WM_SETTEXT,0,(LPARAM) "?");
				DUFF=TRUE;
			}

			Len = SendMessage(hMask[i],WM_GETTEXT, 80, (LPARAM) (LPCSTR) Val);
			ApplMask = atoi(Val);

			if (ApplMask < 0 || ApplMask > 255)
			{
				SendMessage(hMask[i],WM_SETTEXT,0,(LPARAM) "?");
				DUFF=TRUE;
			}
		
			DED = Button_GetCheck(hDED[i]);
	
			if (!DUFF)
			{
				wsprintf(Key,"Port%d",i);
				RegSetValueEx(hKey,Key,0,REG_DWORD,(BYTE *)&Port,4);

				wsprintf(Key,"Port%dStreams",i);
				RegSetValueEx(hKey,Key,0,REG_DWORD,(BYTE *)&Streams,4);

				wsprintf(Key,"Port%dMask",i);
				RegSetValueEx(hKey,Key,0,REG_DWORD,(BYTE *)&ApplMask,4);

				wsprintf(Key,"Port%dDEDMode",i);
				RegSetValueEx(hKey,Key,0,REG_DWORD,(BYTE *)&DED, 4);
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

			wsprintf(Key,"Port%dDEDMode",i);
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
	UCHAR CmdReply[]="C00";

	UCHAR Chan, Stream;
	int i, j, TXLen, StreamNo;
	
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
		SendKISSData(conn, "C00Invalid Stream", 17);
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

			FreeBytes = 2000;

			// Ideally I should do flow control by channel, but Paclink (at least) doesn't have a mechanism

			for (i = 1; i < conn->numChannels; i++)
			{
				if (conn->Channels[i]->Connected)
					if (TXCount(conn->Channels[1]->BPQStream) > 10)
						FreeBytes = 0;
			}

			// This format works with Paclink and RMS Packet, but it doesn't seem to conform to the spec

			// I think maybe the Channel status should be in the same Frame.

			TXLen = wsprintf(Reply, "C00FREE BYTES %d\r", FreeBytes);
			SendKISSData(conn, Reply, TXLen);

			for (j=1; j <= conn->numChannels; j++)
			{
				channel = conn->Channels[j];
			
				if (channel->Connected)
				{
					TXLen = wsprintf(Reply, "C00%c/V stream - CONNECTED to %s", j + '@', "SWITCH");
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
					// Have a via string as 2nd param 

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
							
							SendKISSData(conn, "C00Invalid via String (First must be Port)", 42);
							return;		

						}
					}
				}
				else
					TXLen = wsprintf(TXBuff, "%s %s %s %s %s\r", Cmd, Arg1, Arg2, Arg3, Context);

			}
			else
			{
				TXLen = wsprintf(TXBuff, "C %s\r", Arg1);
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
			}

			SendMsg(conn->Channels[StreamNo]->BPQStream, TXBuff, TXLen);

			return;
			
		}

		if (_stricmp(Cmd, "D") == 0)
		{
			// Disconnect

			SessionControl(conn->Channels[StreamNo]->BPQStream, 2, 0);

			return;
		}

		if (_memicmp(Cmd, "INT", 3) == 0)
		{
			SendKISSData(conn, "C00INTFACE HOST", 15);
			return;
		}

		if (_stricmp(Cmd, "PACLEN") == 0)
		{
			SendKISSData(conn, "C00PACLEN 128/128", 17);
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
  
// BPQ Serial Device Support

// On W2K and above, BPQVIrtualCOM.sys provides a pair of cross-connected devices, and a control channel
//	to enumerate, add and delete devices.

// On Win98 BPQVCOMM.VXD provides a single IOCTL interface, over which calls for each COM device are multiplexed


/*
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
*/
#define IOCTL_SERIAL_GET_COMMSTATUS     CTL_CODE(FILE_DEVICE_SERIAL_PORT,27,METHOD_BUFFERED,FILE_ANY_ACCESS)
/*
#define IOCTL_SERIAL_XOFF_COUNTER       CTL_CODE(FILE_DEVICE_SERIAL_PORT,28,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_SERIAL_GET_PROPERTIES     CTL_CODE(FILE_DEVICE_SERIAL_PORT,29,METHOD_BUFFERED,FILE_ANY_ACCESS)
*/
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


HANDLE BPQOpenSerialControl(ULONG * lasterror)
{            
	HANDLE hDevice;

	*lasterror=0;

	if (Win98)
	{
		hDevice = CreateFile( "\\\\.\\BPQVCOMM.VXD", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}
	else
	{
		hDevice = CreateFile( "//./BPQControl", GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
	}
				  
	if (hDevice == (HANDLE) -1 )
	{
		*lasterror=GetLastError();
	}

	return hDevice;

}
int BPQSerialAddDevice(HANDLE hDevice, ULONG * port, ULONG * result)
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
	char szPort[15];
	HANDLE hDevice;

	*lasterror=0;

	if (Win98)
	{
		wsprintf( szPort, "\\\\.\\COM%d",port) ;

		hDevice = CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );
				  
		if (hDevice == (HANDLE) -1 )
		{
			// If In Use(5) ok, else fail

			if (GetLastError() == 5)
				return (HANDLE)(port<<16);			// Port Number is a pseudohandle to the device

			return (HANDLE) -1;
		}

		CloseHandle(hDevice);

		return (HANDLE)(port<<16);			// Port Number is a pseudohandle to the device
	}


  // load the COM prefix string and append port number
   
	
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

	if (Win98)
		return DeviceIoControl(hControl,(UINT)(UINT)hDevice | W98_SERIAL_SET_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_CTS,NULL,0,NULL,0, &bytesReturned,NULL);

}

int BPQSerialSetDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_SET_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_DSR, NULL,0,NULL,0, &bytesReturned,NULL);

                  
}

int BPQSerialSetDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_SET_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_SET_DCD,NULL,0,NULL,0, &bytesReturned,NULL);

                  
}

int BPQSerialClrCTS(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_CLR_CTS,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_CTS,NULL,0,NULL,0, &bytesReturned,NULL);

                  
}
int BPQSerialClrDSR(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_CLR_DSR,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DSR,NULL,0,NULL,0, &bytesReturned,NULL);

                  
}

int BPQSerialClrDCD(HANDLE hDevice)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_CLR_DCD,NULL,0,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_CLR_DCD, NULL,0,NULL,0, &bytesReturned,NULL);

                     
}

int BPQSerialSendData(HANDLE hDevice,UCHAR * Message,int MsgLen)
{
	ULONG bytesReturned;

	if (MsgLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_SETDATA,Message,MsgLen,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialGetData(HANDLE hDevice,UCHAR * Message,int BufLen, ULONG * MsgLen)
{
	if (BufLen > 4096 )	return ERROR_INVALID_PARAMETER;
	
	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_GETDATA,NULL,0,Message,BufLen,MsgLen,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_GETDATA,NULL,0,Message,BufLen,MsgLen,NULL);
                  
}

int BPQSerialGetQCounts(HANDLE hDevice,ULONG * RXCount, ULONG * TXCount)
{

	SERIAL_STATUS Resp;
	int MsgLen;
	int ret;

	if (Win98)
		ret = DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_GET_COMMSTATUS,NULL,0,&Resp,sizeof(SERIAL_STATUS),&MsgLen,NULL);
	else
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

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_IS_COM_OPEN,NULL,0,Count,4,&bytesReturned,NULL);                
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_IS_COM_OPEN,NULL,0,Count,4,&bytesReturned,NULL);                
}

int BPQSerialGetDTRRTS(HANDLE hDevice,ULONG * Flags)
{
	ULONG bytesReturned;

	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_SERIAL_GET_DTRRTS,NULL,0,Flags,4,&bytesReturned,NULL);                
	else
		return DeviceIoControl(hDevice,IOCTL_SERIAL_GET_DTRRTS,NULL,0,Flags,4,&bytesReturned,NULL);                
}

int BPQSerialSetPollDelay(HANDLE hDevice, int PollDelay)
{
	ULONG bytesReturned;
	
	if (Win98)
		return DeviceIoControl(hControl, (UINT)hDevice | W98_BPQ_SET_POLLDELAY,&PollDelay,4,NULL,0, &bytesReturned,NULL);
	else
		return DeviceIoControl(hDevice,IOCTL_BPQ_SET_POLLDELAY,&PollDelay,4,NULL,0, &bytesReturned,NULL);
                  
}

int BPQSerialSetDebugMask(HANDLE hDevice, int DebugMask)
{
	ULONG bytesReturned;
	
	return DeviceIoControl(hDevice, IOCTL_BPQ_SET_DEBUGMASK, &DebugMask, 4, NULL, 0, &bytesReturned,NULL);
                  
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
		conn = Connections[i];

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
		conn = Connections[i];

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
		conn = Connections[i];

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

//	WA8DED Host Mode Routines

//	Based on BPQDED32 DLL


HINSTANCE ExtDriver=0;

int *BPQAPI=0;

typedef int (FAR *FARPROCX)();

FARPROCX GETAPI;

int AttachedProcesses=0;


//unsigned char RXBUFFER[512];		//	BUFFER TO PC APPLICATION

//unsigned char * RXPOINTER=&RXBUFFER[0];

//unsigned char * PUTPTR=&RXBUFFER[0] ;

//unsigned char LINEBUFFER[300];		// MSG FROM PC APPL

//unsigned char * CURSOR=&LINEBUFFER[0];


unsigned char NODEBUFFER[300];		// MESSAGE FROM NODE
unsigned char WORKAREA[300];		// UNPACK AREA FOR CHAINED BUFFERS

unsigned char DummyCall[11];		// For Connection Status Call

;

int APPLMASK=1;

char APPLFLAGS=0x42;		// REQUEST AUTOTIMERS, MSG TO USER
char DEDMODE=' ';			// CLUSTER MODE - DONT ALLOW DUP CONNECTS
UCHAR MonEnabled=0;			// Record current MON state - used to set APPLFLAGS mon bit

#define MAXSTREAMS 32


unsigned int MMASK=0xFFFF;
unsigned short D10=10;
unsigned short D16=16;

unsigned short TENK[]={10000,1000,100,10};

unsigned short NEWVALUE;

unsigned int ecxsave;
char ahsave;
char alsave;

#define CHAN_TXQ	0	//		; FRAMES QUEUED TO NODE

#define BUFFERS	800 //1099
#define BUFFLEN	50

unsigned int FREE_Q=0;

unsigned int QCOUNT=0;

unsigned int MINBUFFCOUNT=BUFFERS;

unsigned int NOBUFFCOUNT=0;

unsigned char BUFFERPOOL[BUFFLEN*BUFFERS];



/*
;
;	BUFFER POOL USES A SYSTEM OF CHAINED SMALL BUFFERS
;
;	FIRST DWORD OF FIRST IS USED FOR CHAINING MESSAGES
;	SECOND DWORD IS LENGTH OF DATA
;	LAST DWORD IS POINTER TO NEXT BUFFER
;
;	ALL BUT LAST FOUR BYTES OF SUBSEQUENT BUFFERS ARE USED FOR DATA
;
;	BUFFERS ARE ONLY USED TO STORE OUTBOUND MESSAGES WHEN SESSION IS
;	BUSY, OR NODE IS CRITICALLY SHORT OF BUFFERS
;

*/

unsigned char PARAMREPLY[]="* 0 0 64 10 4 4 10 100 18000 30 2 0 2\r\n";

#define PARAMPORT PARAMREPLY[2]

#define LPARAMREPLY	39

unsigned char BADCMDREPLY[]="\x2" "INVALID COMMAND\x0";

#define LBADCMDREPLY 17 //sizeof BADCMDREPLY


unsigned char  ICMDREPLY[]="\x2" "         \x0";
#define LICMDREPLY 11

unsigned char DATABUSYMSG[]="\x2" "TNC BUSY - LINE IGNORED\x0";
#define LDATABUSY 25

unsigned char BADCONNECT[]="\x2" "INVALID CALLSIGN\x0";
#define LBADCONNECT	18

unsigned char BUSYMSG[]="BUSY fm SWITCH\x0";

//unsigned char CONSWITCH[]="\x3" "(1) CONNECTED to           \x0";

UCHAR CONCALL[11];
UCHAR CONMSG[50];

unsigned char SWITCH[]="\x1" "0:SWITCH    \x0";
#define CHECKCALL SWITCH+3
#define LSWITCH	14
	
unsigned char NOTCONMSG[]="\x1" "CHANNEL NOT CONNECTED\x0";
#define LNOTCON	23

unsigned char ALREADYCONMSG[]="You are already connected on another port\r";
#define ALREADYLEN	45

unsigned char MONITORDATA[350];			//  RAW FRAME FROM NODE

//unsigned char MONBUFFER[258]="\x6";

//int MONLENGTH;
//int MONFLAG=0;

unsigned char * MONCURSOR=0;

unsigned char MONHEADER[256];



//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

#define MSGCHAIN 0		// CHAIN WORD
#define MSGPORT	4		// PORT 
#define MMSGLENGTH WORD PTR 5	// LENGTH

#define MSGDEST	7		// DESTINATION
#define MSGORIGIN 14

//	 MAY BE UP TO 56 BYTES OF DIGIS

#define MSGCONTROL	21		// CONTROL BYTE
#define MSGPID		22		// PROTOCOL IDENTIFIER
#define MSGDATA		23		// START OF LEVEL 2 MESSAGE

unsigned char PORT_NO=0		;	// Received port number 0 - 256
unsigned char FRAME_TYPE=0		;// Frame Type           UI etc in Hex
unsigned char PID=0;			// Protocol ID
unsigned short FRAME_LENGTH=0;	// Length of frame      0 - 65...
unsigned char INFO_FLAG=0;		// Information Packet ? 0 No, 1 Yes
unsigned char OPCODE=0;			//L4 FRAME TYPE
unsigned char FRMRFLAG=0;

unsigned char MCOM=1;
unsigned char MALL=1;
unsigned char HEADERLN=1;
unsigned char MTX=0;

#define CR 0x0D
#define LF 0x0A

static char TO_MSG		[]=" to ";
static char FM_MSG		[]="fm ";

static char MYCCT_MSG[]=" my";
static char CCT_MSG	[]=" cct=";
static char TIM_MSG[]=" t/o=";

char NR_INFO_FLAG;		// Information Packet ? 0 No, 1 Yes

//	HDLC COMMANDS (WITHOUT P/F)

#define SABM	0x2F
#define DISC	0x43
#define DM		0x0F
#define UA		0x63
#define FRMR	0x87

#define PFBIT	0x10

#define UI 3
#define RR 1
#define RNR 5
#define REJ 9


static char CTL_MSG[]=" ctl ";
static char VIA_MSG[]=" via ";
static char PID_MSG[]=" pid ";
static char SABM_MSG[]="SABM";
static char DISC_MSG[]="DISC";
static char UA_MSG[]="UA";

static char DM_MSG	[]="DM";
static char RR_MSG	[]="RR";
static char RNR_MSG[]="RNR";
static char I_MSG[]="I pid ";
static char UI_MSG[]="UI pid ";
static char FRMR_MSG[]="FRMR";
static char REJ_MSG[]="REJ";

static char AX25CALL[8];	// WORK AREA FOR AX25 <> NORMAL CALL CONVERSION
static char NORMCALL[11];	// CALLSIGN IN NORMAL FORMAT
static int NORMLEN;			// LENGTH OF CALL IN NORMCALL	

int MSGLENGTH;
char MSGCHANNEL;
char MSGTYPE;

UCHAR * LINEBUFFERPTR;


BOOL Recovering=FALSE;

BOOL InitDED(struct ConnectionInfo * conn)
{
	int i;
	char Errbuff[100];
	char buff[20];

	if (QCOUNT == 0)				// First DED Port
	{
	_asm{

;
;	BUILD BUFFER POOL
;
	MOV	EDI,OFFSET BUFFERPOOL
	MOV	ECX,BUFFERS

BUFF000:

	MOV	ESI,OFFSET FREE_Q

	MOV	EAX,[ESI]			; OLD FIRST IN CHAIN
	MOV	[EDI],EAX
	MOV	[ESI],EDI			; PUT NEW ON FRONT

	INC	QCOUNT

	ADD	EDI,BUFFLEN
	LOOP	BUFF000

	MOV	EAX,QCOUNT
	MOV	MINBUFFCOUNT,EAX

	}
  
	ExtDriver=LoadLibrary("bpq32.dll");

	if (ExtDriver == NULL)
	{
		OutputDebugString("BPQHOST Error Loading bpq32.dll");
		return FALSE;
	}

	GETAPI = (FARPROCX)GetProcAddress(ExtDriver,"_GETBPQAPI@0");
	
	if (GETAPI == NULL)
	{
		OutputDebugString("BPQHOST Error finding BPQ32 API Entry Points");
		return FALSE;
	}

	BPQAPI=(int *)GETAPI();

	}

	for (i=1; i <= conn->numChannels; i++)
	{
		conn->Channels[i]->BPQStream=FindFreeStream();
			
	//	if (conn->Channels[i]->BPQStream) SetAppl(conn->Channels[i]->BPQStream,APPLFLAGS,conn->ApplMask);

		strcpy(Errbuff,	"BPQDHOST init Stream ");

		_itoa(i,buff,10);
		strcat(Errbuff,buff);
		strcat(Errbuff," BPQ Stream");

		_itoa(conn->Channels[i]->BPQStream,buff,10);
		strcat(Errbuff,buff);

		strcat(Errbuff,"\n");

		OutputDebugString(Errbuff);

		conn->CURSOR = (PUCHAR)&conn->LINEBUFFER;

		conn->PUTPTR = (PUCHAR)&conn->PCBUFFER;
		conn->RXPOINTER = conn->PUTPTR;
		conn->MONFLAG = 0;
		conn->MONBUFFER[0] = 6;	// Mon Data

	}

	conn->Channels[0]->BPQStream=conn->Channels[1]->BPQStream;				// For monitoring


	return TRUE;
}


/*
BOOL  DEDClose(struct ConnectionInfo * conn) 
{
   int i,port;

	char Errbuff[100];
	char buff[20];

	OutputDebugString("BPQDED32 TfClose Entered");

	for (i=1; i <= conn->numChannels; i++)
	{
		port=conn->Channels[i]->BPQStream;
			
		if (port)
		{
			DeallocateStream(port);

			strcpy(Errbuff,	"BPQDED Close Stream ");

			_itoa(i,buff,10);
			strcat(Errbuff,buff);
			strcat(Errbuff," BPQ Stream");

			_itoa(bpqstream[i],buff,10);
			strcat(Errbuff,buff);

			strcat(Errbuff,"\n");

			OutputDebugString(Errbuff);

			bpqstream[i]=0;		// Disable DED stream
		}
	}
	return TRUE;
}
*/
VOID ProcessDEDPacket(struct ConnectionInfo * conn, UCHAR * rxbuffer, int Len)
{
	int i;

	for(i = 0; i < Len; i++)
	{
		TfPut(conn, rxbuffer[i]);
	}

	conn->RXBPtr = 0;

	return;
}


BOOL TfPut(struct ConnectionInfo * conn, UCHAR character) 
{
	char Errbuff[1000];
	char buff[20];
	char msg[100];
	HKEY hKey=0;
	struct StreamInfo * Channel;
	int MonDataLen;
	UCHAR * ptr1;

//	int retCode,Type,Vallen=4;
//	char * RegValue;

	int i;

	if (!conn->InHostMode)
		goto CHARMODE;

//	HOST MODE

	if (conn->HOSTSTATE == 0)
	{
		conn->MSGCHANNEL = character;			// WORKING FIELD
		conn->HOSTSTATE++;
		return TRUE;
	}

	if (conn->HOSTSTATE == 1)
	{
		conn->MSGTYPE = character;			// WORKING FIELD
		conn->HOSTSTATE++;
		return TRUE;
	}

	if (conn->HOSTSTATE == 2)
	{
		conn->MSGCOUNT = character;			// WORKING FIELD
		conn->MSGLENGTH = character;
		conn->MSGCOUNT++;			// WORKING FIELD
		conn->MSGLENGTH ++;

		conn->HOSTSTATE++;
		return TRUE;
	}

//	RECEIVING COMMAND/DATA

	*(conn->CURSOR++) = character;

	conn->MSGCOUNT--;

	if (conn->MSGCOUNT)
		return TRUE;				// MORE TO COME

	conn->HOSTSTATE=0;

	MSGCHANNEL = conn->MSGCHANNEL;
	MSGLENGTH = conn->MSGLENGTH;
	MSGTYPE = conn->MSGTYPE;
	LINEBUFFERPTR = (PUCHAR)&conn->LINEBUFFER;

	if (MSGCHANNEL <= conn->numChannels)
		Channel = conn->Channels[MSGCHANNEL];
	else
		Channel = conn->Channels[1];

_asm{
	call	PROCESSHOSTPACKET
}
	conn->HOSTSTATE = 0;

	conn->CURSOR = (PUCHAR)&conn->LINEBUFFER;

	return TRUE;


CHARMODE:

	if (character == 0x11) return TRUE;

	if (character == 0x18)
	{
		//	CANCEL INPUT
 
		conn->CURSOR = (PUCHAR)&conn->LINEBUFFER;
		
		return(TRUE);
	}

	*(conn->CURSOR++) = character;

	if (conn->CURSOR == &conn->LINEBUFFER[300])
		conn->CURSOR--;

	if (character == 0x0d)
	{
		//	PROCESS COMMAND (UNLESS HOST MODE)

		*(conn->CURSOR++) = 0;

		DOCOMMAND(conn);
	}
	return TRUE;

	_asm {

ATCOMMAND:

	}
	if (conn->LINEBUFFER[1] == 'B')
		goto BUFFSTAT;

	if (conn->LINEBUFFER[1] == 'M')
		goto BUFFMIN;

	goto BADCMD;

	_asm{

BUFFMIN:

	PUSH	MINBUFFCOUNT
	JMP SHORT BUFFCOMM
;
BUFFSTAT:

	PUSH	QCOUNT

BUFFCOMM:

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

	MOV	AL,1
	CALL	PUTCHAR
;
;	GET BUFFER COUNT
;
	POP	EAX
	CALL	CONV_5DIGITS

	MOV	AL,0
	CALL	PUTCHAR

	RET

ICMD:


	JMP	SENDHOSTOK

	MOV	ESI,OFFSET ICMDREPLY
	MOV	ECX,LICMDREPLY

	JMP	SENDCMDREPLY

PROCESSHOSTPACKET:

	}

	if ((UINT)Channel->Chan_TXQ == 0xffffffff)
	{
		Channel->Chan_TXQ = 0;
	}
		
	_asm{

	mov	ebx, Channel			; POINT EBX TO CORRECT ENTRY

	CMP	MSGTYPE,0
	JNE	NOTDATA

	JMP	HOSTDATAPACKET

;HOSTCMDS:
;	DD	'G','I', 'J', 'C', 'D', 'L', '@',      'Y', 'M'
;	DD	POLL,ICMD,JCMD,CCMD,DCMD,LCMD,ATCOMMAND,YCMD,HOSTMON

NOTDATA:


	}

	if (conn->LINEBUFFER[0] == 1)
	{
		// recovering

		if (!conn->Recovering)
		{
			wsprintf(msg, "Port %d DED Recovery started\n", conn->ComPort);
			OutputDebugString(msg);
			conn->Recovering = TRUE;
		}
	}
	else
	{
		// Not recovery
				
		if (conn->Recovering)
		{
			wsprintf(msg, "Port %d DED Recovery completed\n", conn->ComPort);
			OutputDebugString(msg);
			conn->Recovering = FALSE;
		}

	}

	if (conn->LINEBUFFER[0] == 1)
		goto DUFFHOSTCMD;

//	wsprintf(msg,"DED CMD: Port %d  CMD %c MSGCHANNEL %d\n", conn->ComPort, conn->LINEBUFFER[0], MSGCHANNEL);
//	OutputDebugString(msg);

	if (_memicmp(conn->LINEBUFFER, "QRES", 4 == 0))
		goto SENDHOSTOK;

	switch (toupper(conn->LINEBUFFER[0]))
	{
	case 1:
		
		goto DUFFHOSTCMD;

	case 'G':

		goto POLL;

	case 'I':
		goto ICMD;
	
	case 'J':

		conn->InHostMode = conn->LINEBUFFER[5] & 1;

//		if (!conn->InHostMode)
//			DisableAppl();

		goto SENDHOSTOK;

	case 'C':
		goto CCMD;

	case 'D':
		goto DCMD;

	case 'L':
		goto LCMD;

	case '@':
		goto ATCOMMAND;

	case 'Y':
		goto YCMD;
	
	case 'E':
		goto ECMD;

	case 'M':

		if (conn->LINEBUFFER[1] == 'N')
			goto DISABLEMONITOR;

		goto	ENABLEMONITOR;

	case 'K':
	case 'O':
		goto SENDHOSTOK;


	default:
		goto SENDHOSTOK;
	}
		
	_asm{

ECMD:

	JMP	SENDHOSTOK

	

DUFFHOSTCMD:

BADCMD:

	pushad
	}


	PUTCHARx(conn, conn->MSGCHANNEL);

	for (i=0; i < LBADCMDREPLY; i++)
	{
		PUTCHARx(conn, BADCMDREPLY[i]);
	}
	_asm{

	popad

	RET

SENDCMDREPLY:

	jecxz duffxx	
		
	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

CMDREPLYLOOP:


	LODSB
	CALL	PUTCHAR

	LOOP	CMDREPLYLOOP

duffxx:
	RET

ENABLEMONITOR:

	MOV	MonEnabled,80H
	jmp MONCOM

DISABLEMONITOR:

	MOV	MonEnabled,0

MONCOM:

	pushad

	}

	SetAppl(conn->Channels[0]->BPQStream, 2 | MonEnabled, conn->ApplMask);

	_asm{

	popad

	JMP	SENDHOSTOK

CCMD:
;
;	CONNECT REQUEST - WE ONLY ALLOW CONNECT TO SWITCH, BUT DONT CHECK
;
	CMP	MSGCHANNEL,0
	JE	SENDHOSTOK			; SETTING UNPROTO ADDR - JUST ACK IT

	CMP	MSGLENGTH,1
	JNE	REALCALL
;
;	STATUS REQUEST - IF CONNECTED, GET CALL
;
	MOV	EDI,OFFSET CHECKCALL	; FOR ANY RETURNED DATA
	MOV	BYTE PTR [EDI],0		; FIDDLE FOR CONNECTED CHECK

	MOV	AH,8
	MOV	AL,MSGCHANNEL		; GET STATUS, AND CALL IF ANY
	CALL	NODE
;
	CMP	CHECKCALL,0
	JE	NOTCONNECTED

	MOV	ESI,OFFSET SWITCH
	MOV	ECX,LSWITCH

	JMP SHORT SENDCMDREPLY

NOTCONNECTED:

	MOV	ESI,OFFSET NOTCONMSG
	MOV	ECX,LNOTCON

	JMP SENDCMDREPLY

REALCALL:

	MOV	CX,1			; CONNECT
	JMP SHORT CDCOMMAND

DCMD:
;
;	DISCONNECT REQUEST
;
	MOV	CX,2			; DISCONNECT
CDCOMMAND:
	MOV	AL,MSGCHANNEL
	MOV	AH,6
	CALL	NODE
;
;	CONNECT/DISCONNECT WILL BE REPORTED VIA NORMAL STATUS CHANGE
;
	JMP	SENDHOSTOK



LCMD:

	CALL	CHECKTXQUEUE		; SEE IF ANYTHING TO SEND ON THIS CHAN

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

	MOV	AL,1
	CALL	PUTCHAR
;
;	GET STATE AND QUEUED BUFFERS
;
	MOV	AL,MSGCHANNEL
	OR	AL,AL
	JNZ	NORM_L
;
;	TO MONITOR CHANNEL 
;
;	SEE IF MONITORED FRAMES AVAILABLE

	MOV ESI,0			; In case NODE call failes
	
	MOV	AH,7
	MOV	AL,1
	CALL	NODE

	MOV	DH,30H			; RETURN TWO ZEROS

	CMP	ESI,0
	JE	MON_L			; NO FRAMES

	MOV	DH,31H			; RETURN 1 FRAME

	JMP SHORT MON_L

NORM_L:

	MOV ECX,0			; In case NODE call failes
	MOV	AH,4			; SEE IF CONNECTED
	CALL	NODE
;
	MOV	DH,'0'			; AX.25 STATE
	OR	CX,CX
	JZ	NOTCON

	MOV	DH,'4'			; CONNECTED

NOTCON:

	MOV	AL,DL			; STATUS MSGS ON RX Q
	CALL	CONV_DIGITS

	MOV	AL,20H
	CALL	PUTCHAR
;
;	GET OTHER QUEUE COUNTS
;
	PUSH	EDX

	MOV EBX,0			; In case NODE call failes
	MOV	AH,7
	MOV	AL,MSGCHANNEL
	CALL	NODE

	POP	EDX

	MOV	AL,BL			; DATA ON RX Q
	CALL	CONV_DIGITS

	MOV	AL,20H
	CALL	PUTCHAR
;
;	NOT SENT IS NUMBER ON OUR QUEUE, NOT ACKED NUMBER FROM SWITCH
;
	mov	ebx, Channel			; POINT EBX TO CORRECT ENTRY
;
;	SEE HOW MANY BUFFERS ATTACHED TO Q HEADER IN BX
;
 	XOR	AL,AL

COUNT_Q_LOOP:

	CMP	DWORD PTR [EBX],0
	JE	COUNT_RET

	INC	AL
	MOV	EBX,[EBX]			; FOLLOW CHAIN
	JMP	COUNT_Q_LOOP

COUNT_RET:

	OR	AL,AL
	JZ	LCOMM05			; NOT BUSY

	MOV	DH,'8'			; BUSY

LCOMM05:

	CALL	CONV_DIGITS		; FRAMES NOT SENT (MAY BE > 10)
;
	MOV	AL,20H
	CALL	PUTCHAR

	MOV	AL,CL
	ADD	AL,30H
	CALL	PUTCHAR			; NOT ACKED

	MOV	AL,20H
	CALL	PUTCHAR

MON_L:

	MOV	AL,30H			; TRIES
	CALL	PUTCHAR

	MOV	AL,20H
	CALL	PUTCHAR

	MOV	AL,DH			; STATE
	CALL	PUTCHAR

	MOV	AL,0
	CALL	PUTCHAR

	RET

HOSTDATAPACKET:

//	}
//	{
//		UCHAR msg[100];

//	wsprintf(msg,"Host Data Packet: Port %d\n", conn->ComPort);
//	OutputDebugString(msg);
//	}
//	_asm{



	MOV	ESI,LINEBUFFERPTR
	MOV	ECX,MSGLENGTH
;
;	IF WE ALREADY HAVE DATA QUEUED, ADD THIS IT QUEUE
;

	mov	ebx, Channel			; POINT EBX TO CORRECT ENTRY

	CMP	DWORD PTR CHAN_TXQ[EBX],0
	JE	NOTHINGQUEUED
;
;	COPY MESSAGE TO A CHAIN OF BUFFERS
;
	CMP	QCOUNT,10
	JB	CANTSEND		; NO SPACE - RETURN ERROR (?)

QUEUEFRAME:

	CALL	COPYMSGTOBUFFERS	; RETURNS EDI = FIRST (OR ONLY) FRAGMENT

	mov	ebx, Channel			; POINT EBX TO CORRECT ENTRY

	LEA	ESI,CHAN_TXQ[EBX]
	CALL	Q_ADD

	JMP	SENDHOSTOK

NOTHINGQUEUED:
;
;	MAKE SURE NODE ISNT BUSY
;
	cmp	MSGCHANNEL,0		; UNPROTO Channel
	je	SendUnproto

	PUSH	ESI
	PUSH	ECX
	PUSH	EBX

	MOV ECX,0			; In case NODE call failes
	MOV	AH,7
	MOV	AL,MSGCHANNEL
	CALL	NODE

	POP	EBX
	CMP	ECX,4
	JA	MUSTQUEUEIT		; ALREADY BUSY

	CMP	EDX,40			; Buffers left
	JB	MUSTQUEUEIT
;
;	OK TO PASS TO NODE
;
	POP	ECX
	POP	ESI
	
	CALL	SENDENFORCINGPACLEN
;
	JMP	SENDHOSTOK

SendUnproto:

	MOV	AH,2			; SEND DATA
	MOV	AL,0			; unproto to all ports
	CALL	INTVAL

	JMP	SENDHOSTOK

MUSTQUEUEIT:

	POP	ECX
	POP	ESI

	JMP	QUEUEFRAME

CANTSEND:

	MOV	ESI,OFFSET DATABUSYMSG
	MOV	ECX,LDATABUSY

	JMP	SENDCMDREPLY


SENDENFORCINGPACLEN:

; ESI=MSG ECX LENGTH

	push	esi
	push	ecx

;AH = 8		Port control/information.  Called with a stream number
;		in AL returns:
;
;		AL = Radio port on which channel is connected (or zero)
;		AH = SESSION TYPE BITS
;		BX = L2 paclen for the radio port
;		CX = L2 maxframe for the radio port
;		DX = L4 window size (if L4 circuit, or zero)
;		ES:EDI = CALLSIGN
;
;	Calling this for every packet is a bit OTT, but paclen could change (eg on L2 downconnect)

	mov	edi,offset DummyCall

	MOV	AH,8			; Connection Info
	MOV	AL,MSGCHANNEL
	CALL	NODE

	pop		ecx
	pop		esi

	cmp	edx,-1
	je short nochange

	cmp	ebx,0
	je short nochange						; paclen not set

fragloop:

	cmp	ebx,ecx
	jge	short nochange						; msglen <= paclen

;	need to fragment

	push	ebx
	push	ecx
	push	esi

	mov	ecx,ebx

	MOV	AH,2			; SEND DATA
	MOV	AL,MSGCHANNEL
	CALL	NODE

	pop		esi
	pop		ecx
	pop		ebx

	add		esi,ebx
	sub		ecx,ebx

	jnc		fragloop

	ret

nochange:

	MOV	AH,2			; SEND DATA
	MOV	AL,MSGCHANNEL
	CALL	NODE

	RET



POLL:

	CALL	CHECKTXQUEUE		; SEE IF ANYTHING TO SEND
	CALL	PROCESSPOLL
	RET

YCMD:

	}
	*(conn->CURSOR++) = ' ';
	*(conn->CURSOR++) = ' ';

	conn->LINEBUFFER[conn->MSGLENGTH+2]=0x0;
	conn->LINEBUFFER[conn->MSGLENGTH+3]=0x0a;
	conn->LINEBUFFER[conn->MSGLENGTH+4]=0x0;

//	OutputDebugString(conn->LINEBUFFER);

	NEWVALUE=atoi(&conn->LINEBUFFER[1]);
	
/*
if (NEWVALUE >= 0 && NEWVALUE <= MAXSTREAMS)
	{
		if (NEWVALUE < HOSTSTREAMS)
		{
			// Need to get rid of some streams

			for (i=NEWVALUE+1; i<=HOSTSTREAMS; i++)
			{
				port=bpqstream[i];
			
				if (port)
				{
					DeallocateStream(port);

					strcpy(Errbuff,	"BPQDED32 YCMD Release Stream ");

					_itoa(i,buff,10);
					strcat(Errbuff,buff);
					strcat(Errbuff," BPQ Stream ");

					_itoa(port,buff,10);
					strcat(Errbuff,buff);

					strcat(Errbuff,"\n");

					OutputDebugString(Errbuff);

					bpqstream[i]=0;		// Disable DED stream
				}
			}
		}
		else
		{
			for (i=HOSTSTREAMS+1; i <= NEWVALUE; i++)
			{
				bpqstream[i]=FindFreeStream();
			
				if (bpqstream[i])  SetAppl(bpqstream[i],APPLFLAGS,APPLMASK);

				strcpy(Errbuff,	"BPQDED32 YCMD Stream ");

				_itoa(i,buff,10);
				strcat(Errbuff,buff);
				strcat(Errbuff," BPQ Stream ");

				_itoa(bpqstream[i],buff,10);
				strcat(Errbuff,buff);

				strcat(Errbuff,"\n");

				OutputDebugString(Errbuff);
			}
		}
		HOSTSTREAMS=NEWVALUE;
	}
*/
	 _asm {


SENDHOSTOK:

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

	MOV	AL,0
	CALL	PUTCHAR			; NOTHING DOING

	RET

PROCESSPOLL:
;
;	ASK SWITCH FOR STATUS CHANGE OR ANY RECEIVED DATA
;
	MOV	CL,0			; POLL TYPE

	CMP	MSGLENGTH,1
	JE	GENERALPOLL
;
;	HE'S BEING AWKWARD, AND USING SPECIFIC DATA/STATUS POLL
;
	 }
	 
	 if (conn->LINEBUFFER[1] == '0')
		goto DATAONLY;

	 _asm{

	CALL	STATUSPOLL
	JMP SHORT POLLEND

GENERALPOLL:

	CALL	STATUSPOLL
	JNZ	POLLRET

DATAONLY:
	CALL	DATAPOLL

POLLEND:
	JNZ	POLLRET			; GOT DATA

	CALL	SENDHOSTOK		; NOTHING DOING

POLLRET:
	RET

DATAPOLL:

	MOV	EDI,OFFSET NODEBUFFER	; FOR ANY RETURNED DATA
	MOV	AH,3
	MOV	AL,MSGCHANNEL
	OR	AL,AL
	JNZ	NOTMONITOR
;
;	POLL FOR MONITOR DATA
;
	 }
	if (conn->MONFLAG == 0)
		goto NOMONITOR;

//	HAVE ALREADY GOT DATA PART OF MON FRAME OT SEND

	conn->MONFLAG = 0;

	ptr1 = (PUCHAR)&conn->MONBUFFER;
	MonDataLen = conn->MONLENGTH;

	_asm{

	mov	ESI, ptr1
	MOV	ECX,MonDataLen

	jecxz BADMONITOR

	JMP	SENDCMDREPLY

BADMONITOR:

	 }
	 OutputDebugString("BPQHOST Mondata Flag Set with no data");
	 _asm{

NOMONITOR:
;
;	SEE IF ANYTHING TO MONITOR
;
	MOV	AL,1			; FIRST PORT
	MOV	AH,11
	MOV ECX,0			; In case NODE call failes

	MOV	EDI,OFFSET MONITORDATA
	CALL	NODE

	CMP	ECX,0
	JE	DATAPOLLRET

	MOV	EDI,OFFSET MONITORDATA

	CMP MTX,1
	JE MONTX

	TEST MSGPORT[EDI],80H		; TX?
	JNZ	DATAPOLLRET

MONTX:
	CALL	DISPLAYFRAME		; DISPLAY TRACE FRAME

	CMP	MONCURSOR,OFFSET MONHEADER+1
	JE	DATAPOLLRET		; NOTHING DOING

	MOV	EDI,MONCURSOR
	MOV	BYTE PTR [EDI],0		; NULL TERMINATOR

	MOV	ECX,EDI
	SUB	ECX,OFFSET MONHEADER-1	; LENGTH

	MOV	ESI,OFFSET MONHEADER

	CALL	SENDCMDREPLY

	OR	AL,1			; HAVE SEND SOMETHING
	RET

NOTMONITOR:

	MOV ECX,0			; In case NODE call failes
	CALL	NODE
;
	CMP	ECX,0
	JE	DATAPOLLRET

	cmp ecx,256
	jbe okf

	mov ecxsave,ecx

	pushad

	 }

	_itoa(ecxsave,buff,10);
	strcpy(Errbuff,	"BPQHOST Corrupt Length = ");
	strcat(Errbuff,buff);
	OutputDebugString(Errbuff);
	
	_asm {

	popad

	XOR	AL,AL			; HAVE NOTHING TO SEND
	RET

okf:
;
;	SEND DATA
;
//	pushad
//	}
//	OutputDebugString("RXed Data\n");
//
//	_asm{
//
//	popad

	MOV	AL,MSGCHANNEL
	CALL	PUTCHAR

	MOV	AL,7
	CALL	PUTCHAR

	DEC	ECX
	MOV	AL,CL
	CALL	PUTCHAR			; LENGTH-1
	INC	CX

	MOV	ESI,OFFSET NODEBUFFER

SENDDATLOOP:

 	LODSB
	CALL	PUTCHAR

	LOOP	SENDDATLOOP

	OR	AL,1			; HAVE SEND SOMETHING
	RET


DATAPOLLRET:

	XOR	AL,AL
	RET

STATUSPOLL:

	MOV	AH,4
	MOV	AL,MSGCHANNEL
	CMP	AL,0
	JE	NOSTATECHANGE		; ?? Channel Zero For Unproto ??

	MOV EDX,0			; in case NODE call fails
	CALL	NODE
;
	CMP	DX,0
	JE	NOSTATECHANGE
;
;	PORT HAS CONNECTED OR DISCONNECTED - SEND STATUS CHANGE TO PC
;
	PUSH	ECX			; SAVE

	MOV	AH,5
	MOV	AL,MSGCHANNEL

	CALL	NODE			; ACK THE STATUS CHANGE

	POP	ECX
	CMP	ECX,0
	JNE	SENDHOSTCON
;
;	DISCONNECTED
;
}
	i = wsprintf(CONMSG, "\x3(%d) DISCONNECTED fm 0:SWITCH", MSGCHANNEL);
	i++;

	_asm{


	MOV	ESI,OFFSET CONMSG
	MOV	ECX,i

	JMP STATUSPOLLEND

SENDHOSTCON:
;
;	GET CALLSIGN
;
	MOV	EDI,OFFSET CONCALL	; FOR ANY RETURNED DATA
	MOV	AH,8
	MOV	AL,MSGCHANNEL		; GET STATUS, AND CALL IF ANY
	CALL	NODE

/*;
;	IF IN CLUSTER MODE, DONT ALLOW DUPLICATE CONNECTS
;
	CMP	DEDMODE,'C'
	JNE	DONTCHECK

	MOV	AL,1
	MOV	ECX,HOSTSTREAMS

CHECKLOOP:

	PUSH	ECX
	PUSH	EAX

	CMP	AL,MSGCHANNEL
	JE	CHECKNEXT		; DONT CHECK OUR STREAM!

	MOV	EDI,OFFSET CHECKCALL	; FOR ANY RETURNED DATA
	MOV	AH,8
	CALL	NODE
;
	CMP	AH,0
	JE	CHECKNEXT		; NOT CONNECTED (AH HAS SESSION FLAGS)

	MOV	ESI,OFFSET CHECKCALL
	MOV	EDI,OFFSET CONCALL
	MOV	ECX,10
	REP CMPSB

	JNE	CHECKNEXT
;
;	ALREADY CONNECTED - KILL NEW SESSION
;
	MOV	ECX,ALREADYLEN
	MOV	ESI,OFFSET ALREADYCONMSG
	
	MOV	AH,2			; SEND DATA
	MOV	AL,MSGCHANNEL
	CALL	NODE
;
	MOV	CX,2			; DISCONNECT

	MOV	AL,MSGCHANNEL
	MOV	AH,6
	CALL	NODE
;
	MOV	AH,5
	MOV	AL,MSGCHANNEL
	CALL	NODE			; ACK THE STATUS CHANGE

	POP	EAX
	POP	ECX

	XOR	AL,AL			; SET Z

CHECKNEXT:

	POP	EAX
	POP	ECX

	INC	AL
	LOOP	CHECKLOOP

DONTCHECK:
*/
}

	i = wsprintf(CONMSG, "\x3(%d) CONNECTED to %s\x0", MSGCHANNEL, CONCALL);
	i++;

	_asm{



	MOV	ESI,OFFSET CONMSG
	MOV	ECX,i

STATUSPOLLEND:

	CALL	SENDCMDREPLY
	OR	AL,1			; SET NZ
	RET

NOSTATECHANGE:

	RET


CHECKTXQUEUE:
;
;	IF ANYTHING TO SEND, AND NOT BUSY, SEND IT
;
	mov	ebx, Channel			; POINT EBX TO CORRECT ENTRY

	CMP	DWORD PTR CHAN_TXQ[EBX],0
	JNZ	SOMETHINGONQUEUE

	RET

SOMETHINGONQUEUE:
;
;	MAKE SURE NODE ISNT BUSY
;
	MOV	AH,7
	MOV	AL,MSGCHANNEL
	CALL	NODE

	CMP	ECX,4
	JA	STILLBUSY		; ALREADY BUSY

	CMP	EDX,25
	JB	STILLBUSY
;
;	OK TO PASS TO NODE
;

	mov	ebx, Channel			; POINT EBX TO CORRECT ENTRY

	LEA	ESI,CHAN_TXQ[EBX]
	CALL	Q_REM

	PUSH	EBX

	MOV	ESI,EDI			; BUFFER CHAIN
	MOV	EDI,OFFSET WORKAREA

	MOV		ECX,4[ESI]		; LENGTH
	PUSH	ECX
	CALL	COPYBUFFERSTOSTRING	; UNPACK CHAIN
	POP		ECX

	MOV	ESI,OFFSET WORKAREA

	CALL	SENDENFORCINGPACLEN

	POP	EBX

STILLBUSY:

	RET


NODE:

	MOV	ahsave,AH
	pushad
	}

	if (MSGCHANNEL > conn->numChannels)
		goto INTRET;

	alsave=conn->Channels[MSGCHANNEL]->BPQStream;

	_asm{

	popad

	mov ah,ahsave
	mov al,alsave


INTVAL:

	nop
	call BPQAPI
	RET

INTRET:
	popad
	RET

RELBUFF:

	MOV	ESI,OFFSET FREE_Q
	CALL	Q_ADDF			; RETURN BUFFER TO FREE QUEUE
	RET

COPYMSGTOBUFFERS:
;
;	COPIES MESSAGE IN ESI, LENGTH ECX TO A CHAIN OF BUFFERS
;
;	RETURNS EDI = FIRST (OR ONLY) FRAGMENT
;
	PUSH	ESI			; SAVE MSG

	CALL	GETBUFF			; GET FIRST
	JNZ	BUFFEROK1		; NONE - SHOULD NEVER HAPPEN

	POP		ESI
	MOV	EDI,0			; INDICATE NO BUFFER
	
	RET

BUFFEROK1:

	POP	ESI			; RECOVER DATA

	MOV	EDX,EDI			; SAVE FIRST BUFFER

	MOV	4[EDI],ECX		; SAVE LENGTH
	ADD	EDI,8

	CMP	ECX,BUFFLEN-12	; MAX DATA IN FIRST
	JA	NEEDCHAIN
;
;	IT WILL ALL FIT IN ONE BUFFER
;
COPYLASTBIT:

	REP MOVSB

	MOV	EDI,EDX			; FIRST BUFFER
	
	RET

NEEDCHAIN:

	PUSH	ECX
	MOV		ECX,BUFFLEN-12
	REP MOVSB			; COPY FIRST CHUNK
	POP	ECX
	SUB	ECX,BUFFLEN-12
;
;	EDI NOW POINTS TO CHAIN WORD OF BUFFER
;
COPYMSGLOOP:
;
;	GET ANOTHER BUFFER
;
	PUSH	ESI			; MESSAGE 
	PUSH	EDI			; FIRST BUFFER CHAIN WORD

	CALL	GETBUFF
	JNZ	BUFFEROK2		; NONE - SHOULD NEVER HAPPEN

	POP		ESI			; PREVIOUS BUFFER
	POP		ESI
	MOV	EDI,0			; INDICATE NO BUFFER
	
	RET

BUFFEROK2:

	POP	ESI			; PREVIOUS BUFFER
	MOV	[ESI],EDI			; CHAIN NEW BUFFER

	POP	ESI			; MESSAGE

	CMP	ECX,BUFFLEN-4		; MAX DATA IN REST
	JBE	COPYLASTBIT

	PUSH	ECX
	MOV	ECX,BUFFLEN-4
	REP MOVSB			; COPY FIRST CHUNK
	POP	ECX
	SUB	ECX,BUFFLEN-4
;
	JMP	COPYMSGLOOP

COPYBUFFERSTOSTRING:
;
;	UNPACKS CHAIN OF BUFFERS IN SI TO DI
;
;
	MOV	ECX,4[ESI]		; LENGTH

	MOV	EDX,ESI

	ADD	ESI,8

	CMP	ECX,BUFFLEN-12
	JA	MORETHANONE
;
;	ITS ALL IN ONE BUFFER
;
	REP MOVSB

	MOV	EDI,EDX			; BUFFER
	CALL	RELBUFF

	RET

MORETHANONE:

	PUSH	ECX
	MOV	ECX,BUFFLEN-12
	REP MOVSB

	POP	ECX
	SUB	ECX,BUFFLEN-12

UNCHAINLOOP:

	PUSH	EDI			; SAVE TARGET

	MOV	EDI,EDX			; OLD BUFFER

	MOV	EDX,[ESI]			; NEXT BUFFER

	CALL	RELBUFF

	POP	EDI
	MOV	ESI,EDX

	CMP	ECX,BUFFLEN-4
	JBE	LASTONE


	PUSH	ECX
	MOV	ECX,BUFFLEN-4
	REP MOVSB
	POP	ECX
	SUB	ECX,BUFFLEN-4

	JMP	UNCHAINLOOP

LASTONE:

	REP MOVSB

	MOV	EDI,EDX
	CALL	RELBUFF

	RET

Q_REM:

	MOV	EDI,DWORD PTR[ESI]		; GET ADDR OF FIRST BUFFER
	CMP	EDI,0
	JE	Q_RET					; EMPTY

	MOV	EAX,DWORD PTR [EDI]			; CHAIN FROM BUFFER
	MOV	[ESI],EAX			; STORE IN HEADER

Q_RET:
	RET

;               
Q_ADD:
Q_ADD05:
	CMP	DWORD PTR [ESI],0		; END OF CHAIN
	JE	Q_ADD10

	MOV	ESI,DWORD PTR [ESI]		; NEXT IN CHAIN
	JMP	Q_ADD05
Q_ADD10:
	MOV	DWORD PTR [EDI],0		; CLEAR CHAIN ON NEW BUFFER
	MOV	[ESI],EDI			; CHAIN ON NEW BUFFER

	RET

;
;	ADD TO FRONT OF QUEUE - MUST ONLY BE USED FOR FREE QUEUE
;
Q_ADDF:

	MOV	EAX,DWORD PTR[ESI]	; OLD FIRST IN CHAIN
	MOV	[EDI],EAX
	MOV	[ESI],EDI			; PUT NEW ON FRONT

	INC	QCOUNT
	RET

GETBUFF:

	MOV	ESI,OFFSET FREE_Q
	CALL	Q_REM
;
	JZ	NOBUFFS

	DEC	QCOUNT
	MOV	EAX,QCOUNT
	CMP	EAX,MINBUFFCOUNT
	JA	GETBUFFRET
	MOV	MINBUFFCOUNT,EAX

GETBUFFRET:

	OR	AL,1			; SET NZ
 	RET

NOBUFFS:

	INC	NOBUFFCOUNT
	XOR	AL,AL
	RET


CONV_DIGITS:

	MOV	AH,0

CONV_5DIGITS:

	PUSH	EDX
	PUSH	EBX

	MOV	EBX,OFFSET TENK		; 10000

	CMP	AX,10
	JB	UNITS			; SHORT CUT AND TO STOP LOOP

START_LOOP:

	cmp	AX,WORD PTR [EBX]
	JAE	STARTCONV

	ADD	EBX,2

	JMP SHORT START_LOOP
;
STARTCONV:

	MOV	DX,0
	DIV	WORD PTR [EBX]		; 
	ADD	AL,30H			; MUST BE LESS THAN 10
	CALL	PUTCHAR
;
	MOV	AX,DX			; REMAINDER

	ADD	EBX,2
	CMP	EBX,OFFSET TENK+8	; JUST DIVIDED BY 10?
	JNE	STARTCONV		; NO, SO ANOTHER TO DO
;
;	REST MUST BE UNITS
;
UNITS:

	add	AL,30H
	CALL	PUTCHAR

	POP	EBX
	POP	EDX

	ret


MONPUTCHAR:

	PUSH	EDI
	MOV	EDI,MONCURSOR
	STOSB
	MOV	MONCURSOR,EDI
	POP	EDI

	RET

DISPLAYFRAME:

/*

    From DEDHOST Documentation
	
	  
Codes of 4 and 5 both signify a monitor header.  This  is  a  null-terminated
format message containing the

    fm {call} to {call} via {digipeaters} ctl {name} pid {hex}

string  that  forms  a monitor header.  The monitor header is also identical to
the monitor header displayed in user mode.  If the code was  4,  the  monitored
frame contained no information field, so the monitor header is all you get.  If
you monitor KB6C responding to a connect request from me and then poll  channel
0, you'll get:

    0004666D204B42364320746F204B42354D552063746C2055612070494420463000
    ! ! f m   K B 6 C   t o   K B 5 M U   c t l   U A   p i d   F 0 !
    ! !                                                             !
    ! +---- Code = 4 (Monitor, no info)        Null termination ----+
    +------- Channel = 0 (Monitor info is always on channel 0)

  If  the code was 5, the monitored frame did contain an information field.  In
this case, another G command to channel 0 will return the monitored information
with  a code of 6.  Since data transmissions must be transparent, the monitored
information is passed as a byte-count format transmission.    That  is,  it  is
preceded  by a count byte (one less than the number of bytes in the field).  No
null terminator is used in this case.  Since codes  4,  5,  and  6  pertain  to
monitored  information,  they will be seen only on channel 0.  If you hear KB6C
say "Hi" to NK6K, and then poll channel 0, you'll get:

    0005666D204B42364320746F204E4B364B2063746C204930302070494420463000
    ! ! f m   K B 6 C   t o   N K 6 K   c t l   I 0 0   p i d   F 0 !
    ! !                                           ! !               !
    ! !                           or whatever ----+-+               !
    ! !                                                             !
    ! +---- Code = 5 (Monitor, info follows)   Null termination ----+
    +------ Channel = 0 (Monitor info is always on channel 0)

and then the very next poll to channel 0 will get:

         00 06 02 48 69 0D
         !  !  !  H  i  CR
         !  !  !        !
         !  !  !        +----    (this is a data byte)
         !  !  +---- Count = 2   (three bytes of data)
         !  +------- Code = 6    (monitored information)
         +---------- Channel = 0 (Monitor info is always on channel 0)

*/


	MOV	MONHEADER,4		; NO DATA FOLLOWS
	MOV	MONCURSOR,OFFSET MONHEADER+1
;
;	GET THE CONTROL BYTE, TO SEE IF THIS FRAME IS TO BE DISPLAYED
;
	PUSH	EDI
	MOV	ECX,8			; MAX DIGIS
CTRLLOOP:
	TEST	BYTE PTR (MSGCONTROL-1)[EDI],1
	JNZ	CTRLFOUND

	ADD	EDI,7
	LOOP	CTRLLOOP
;
;	INVALID FRAME
;
	POP	EDI
	RET

CTRLFOUND:
	MOV	AL,MSGCONTROL[EDI]

	and	AL,NOT PFBIT		; Remove P/F bit
	mov	FRAME_TYPE,AL

	
	POP	EDI
;
	TEST	AL,1			; I FRAME
	JZ	IFRAME

	CMP	AL,3			; UI
	JE	OKTOTRACE		; ALWAYS DO UI

	CMP	AL,FRMR
	JE	OKTOTRACE		; ALWAYS DO FRMR
;
;	USEQ/CONTROL - TRACE IF MCOM ON
;
	CMP	MCOM,0
	JNE	OKTOTRACE

	RET

;-----------------------------------------------------------------------------;
;       Check for MALL                                                        ;
;-----------------------------------------------------------------------------;

IFRAME:
	cmp	MALL,0
	jne	OKTOTRACE

	ret

OKTOTRACE:
;
;-----------------------------------------------------------------------------;
;       Get the port number of the received frame                             ;
;-----------------------------------------------------------------------------;
;
;	CHECK FOR PORT SELECTIVE MONITORING
;
	MOV	CL,MSGPORT[EDI]
	mov	PORT_NO,CL

	DEC	CL
	MOV	EAX,1
	SHL	EAX,CL			; SHIFT BIT UP

	TEST	MMASK,EAX
	JNZ	TRACEOK1

	RET

TRACEOK1:

	MOV	FRMRFLAG,0
	push	EDI
	mov	AH,MSGDEST+6[EDI]
	mov	AL,MSGORIGIN+6[EDI]

;
;       Display Origin Callsign                                               ;
;

;    0004666D204B42364320746F204B42354D552063746C2055612070494420463000
;    ! ! f m   K B 6 C   t o   K B 5 M U   c t l   U A   p i d   F 0 !
;    ! !                                                             !
;    ! +---- Code = 4 (Monitor, no info)        Null termination ----+
 ;   +------- Channel = 0 (Monitor info is always on channel 0)

	mov	ESI,OFFSET FM_MSG
	call	NORMSTR

	lea	ESI,MSGORIGIN[EDI]
	call	CONVFROMAX25
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	pop	EDI
	push	EDI

	mov	ESI,OFFSET TO_MSG
	call	NORMSTR
;
;       Display Destination Callsign                                          ;
;
	lea	ESI,MSGDEST[EDI]
	call	CONVFROMAX25
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	pop	EDI
	push	EDI

	mov	AX,MMSGLENGTH[EDI]
	mov	FRAME_LENGTH,AX
	mov	ECX,8			; Max number of digi-peaters
;
;       Display any Digi-Peaters                                              ;
;
	test	MSGORIGIN+6[EDI],1
	jnz	NO_MORE_DIGIS

	mov	ESI,OFFSET VIA_MSG
	call	NORMSTR
	jmp short skipspace

NEXT_DIGI:
	test	MSGORIGIN+6[EDI],1
	jnz	NO_MORE_DIGIS

	mov	AL,' '
	call	MONPUTCHAR
skipspace:
	add	EDI,7
	sub	FRAME_LENGTH,7		; Reduce length

	push	EDI
	push	ECX
	lea	ESI,MSGORIGIN[EDI]
	call	CONVFROMAX25		; Convert to call

	push	EAX			; Last byte is in AH

	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	pop	EAX

	test	AH,80H
	jz	NOT_REPEATED

	mov	AL,'*'
	call	MONPUTCHAR

NOT_REPEATED:
	pop	ECX
	pop	EDI
	loop	NEXT_DIGI

NO_MORE_DIGIS:	

;----------------------------------------------------------------------------;
;       Display ctl                                    ;
;----------------------------------------------------------------------------;

	mov	ESI,OFFSET CTL_MSG
	call	NORMSTR

;-----------------------------------------------------------------------------;
;       Start displaying the frame information                                ;
;-----------------------------------------------------------------------------;


	mov	INFO_FLAG,0

	mov	AL,FRAME_TYPE

	test	AL,1
	jne	NOT_I_FRAME

;-----------------------------------------------------------------------------;
;       Information frame                                                     ;
;-----------------------------------------------------------------------------;

	mov	AL,'I'
	call	MONPUTCHAR
	mov	INFO_FLAG,1

	mov	ESI,OFFSET I_MSG
	call	NORMSTR

	lea	ESI,MSGPID[EDI]
	lodsb

	call BYTE_TO_HEX
	

	jmp	END_OF_TYPE

NOT_I_FRAME:

;-----------------------------------------------------------------------------;
;       Un-numbered Information Frame                                         ;
;-----------------------------------------------------------------------------;

	cmp	AL,UI
	jne	NOT_UI_FRAME

	mov	ESI,OFFSET UI_MSG
	call	NORMSTR

	lea	ESI,MSGPID[EDI]
	lodsb

	call BYTE_TO_HEX
	
	mov	INFO_FLAG,1
	jmp	END_OF_TYPE

NOT_UI_FRAME:
	test	AL,10B
	jne	NOT_R_FRAME

;-----------------------------------------------------------------------------;
;       Process supervisory frames                                            ;
;-----------------------------------------------------------------------------;


	and	AL,0FH			; Mask the interesting bits
	cmp	AL,RR	
	jne	NOT_RR_FRAME

	mov	ESI,OFFSET RR_MSG
	call	NORMSTR
	jmp	END_OF_TYPE

NOT_RR_FRAME:
	cmp	AL,RNR
	jne	NOT_RNR_FRAME

	mov	ESI,OFFSET RNR_MSG
	call	NORMSTR
	jmp END_OF_TYPE

NOT_RNR_FRAME:
	cmp	AL,REJ
	jne	NOT_REJ_FRAME

	mov	ESI,OFFSET REJ_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_REJ_FRAME:
	mov	AL,'?'			; Print "?"
	call	MONPUTCHAR
	jmp	SHORT END_OF_TYPE

;
;       Process all other frame types                                         ;
;

NOT_R_FRAME:
	cmp	AL,UA
	jne	NOT_UA_FRAME

	mov	ESI,OFFSET UA_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_UA_FRAME:
	cmp	AL,DM
	jne	NOT_DM_FRAME

	mov	ESI,OFFSET DM_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_DM_FRAME:
	cmp	AL,SABM
	jne	NOT_SABM_FRAME

	mov	ESI,OFFSET SABM_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_SABM_FRAME:
	cmp	AL,DISC
	jne	NOT_DISC_FRAME

	mov	ESI,OFFSET DISC_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_DISC_FRAME:
	cmp	AL,FRMR
	jne	NOT_FRMR_FRAME

	mov	ESI,OFFSET FRMR_MSG
	call	NORMSTR
	MOV	FRMRFLAG,1
	jmp	SHORT END_OF_TYPE

NOT_FRMR_FRAME:
	mov	AL,'?'
	call	MONPUTCHAR

END_OF_TYPE:

	CMP	FRMRFLAG,0
	JE	NOTFRMR
;
;	DISPLAY FRMR BYTES
;
	lea	ESI,MSGPID[EDI]
	MOV	CX,3			; TESTING
FRMRLOOP:
	lodsb
	CALL	BYTE_TO_HEX

	LOOP	FRMRLOOP

	JMP	NO_INFO

NOTFRMR:

	MOVZX	ECX,FRAME_LENGTH


	cmp	INFO_FLAG,1		; Is it an information packet ?
	jne	NO_INFO


	XOR	AL,AL			; IN CASE EMPTY

	sub	ECX,23
	CMP ecx,0
	je	NO_INFO			; EMPTY I FRAME

;
;	PUT DATA IN MONBUFFER, LENGTH IN MONLENGTH
;

	pushad
}
	conn->MONFLAG = 1;

	_asm{

	popad

	MOV	MONHEADER,5		; DATA FOLLOWS

	cmp	ECX,257
	jb	LENGTH_OK
;
	mov	ECX,256
;
LENGTH_OK:
;
	mov	MonDataLen, ECX

	pushad

	}

	conn->MONBUFFER[1] = MonDataLen & 0xff;
	conn->MONBUFFER[1]--;


	conn->MONLENGTH = MonDataLen+2;

	ptr1=&conn->MONBUFFER[2];

	_asm{

	popad
		
	MOV	EDI,ptr1

MONCOPY:
	LODSB
	CMP	AL,7			; REMOVE BELL
	JNE	MONC00

	MOV	AL,20H
MONC00:
	STOSB

	LOOP	MONCOPY

	POP	EDI
	RET

NO_INFO:
;
;	ADD CR UNLESS DATA ALREADY HAS ONE
;
	CMP	AL,CR
	JE	NOTANOTHER

	mov	AL,CR
	call	MONPUTCHAR

NOTANOTHER:
;
	pop	EDI
	ret

;----------------------------------------------------------------------------;
;       Display ASCIIZ strings                                               ;
;----------------------------------------------------------------------------;

NORMSTR:
	lodsb
	cmp	AL,0		; End of String ?
	je	NORMSTR_RET	; Yes
	call	MONPUTCHAR
	jmp	SHORT NORMSTR

NORMSTR_RET:
	ret

;-----------------------------------------------------------------------------;
;       Display Callsign pointed to by SI                                     ;
;-----------------------------------------------------------------------------;

DISPADDR:
	jcxz	DISPADDR_RET

	lodsb
	call	MONPUTCHAR

	loop	DISPADDR

DISPADDR_RET:
	ret


;-----------------------------------------------------------------------------;
;       Convert byte in AL to nn format                                       ;
;-----------------------------------------------------------------------------;

DISPLAY_BYTE_2:
	cmp	AL,100
	jb	TENS_2

	sub	AL,100
	jmp	SHORT DISPLAY_BYTE_2

TENS_2:
	mov	AH,0

TENS_LOOP_2:
	cmp	AL,10
	jb	TENS_LOOP_END_2

	sub	AL,10
	inc	AH
	jmp	SHORT TENS_LOOP_2

TENS_LOOP_END_2:
	push	EAX
	mov	AL,AH
	add	AL,30H
	call	MONPUTCHAR
	pop	EAX

	add	AL,30H
	call	MONPUTCHAR

	ret

;-----------------------------------------------------------------------------;
;       Convert byte in AL to Hex display                                     ;
;-----------------------------------------------------------------------------;

BYTE_TO_HEX:
	push	EAX
	shr	AL,1
	shr	AL,1
	shr	AL,1
	shr	AL,1
	call	NIBBLE_TO_HEX
	pop	EAX
	call	NIBBLE_TO_HEX
	ret

NIBBLE_TO_HEX:
	and	AL,0FH
	cmp	AL,10

	jb	LESS_THAN_10
	add	AL,7

LESS_THAN_10:
	add	AL,30H
	call	MONPUTCHAR
	ret



CONVFROMAX25:
;
;	CONVERT AX25 FORMAT CALL IN [SI] TO NORMAL FORMAT IN NORMCALL
;	   RETURNS LENGTH IN CX AND NZ IF LAST ADDRESS BIT IS SET
;
	PUSH	ESI			; SAVE
	MOV	EDI,OFFSET NORMCALL
	MOV	ECX,10			; MAX ALPHANUMERICS
	MOV	AL,20H
	REP STOSB			; CLEAR IN CASE SHORT CALL
	MOV	EDI,OFFSET NORMCALL
	MOV	CL,6
CONVAX50:
	LODSB
	CMP	AL,40H
	JE	CONVAX60		; END IF CALL - DO SSID

	SHR	AL,1
	STOSB
	LOOP	CONVAX50
CONVAX60:
	POP	ESI
	ADD	ESI,6			; TO SSID
	LODSB
	MOV	AH,AL			; SAVE FOR LAST BIT TEST
	SHR	AL,1
	AND	AL,0FH
	JZ	CONVAX90		; NO SSID - FINISHED
;
	MOV	BYTE PTR [EDI],'-'
	INC	EDI
	CMP	AL,10
	JB	CONVAX70
	SUB	AL,10
	MOV	BYTE PTR [EDI],'1'
	INC	EDI
CONVAX70:
	ADD	AL,30H			; CONVERT TO DIGIT
	STOSB
CONVAX90:
	MOV	ECX,EDI
	SUB	ECX,OFFSET NORMCALL
	MOV	NORMLEN,ECX		; SIGNIFICANT LENGTH

	TEST	AH,1			; LAST BIT SET?
	RET

	

PUTCHAR:

	pushad
	push eax
	push conn
	call PUTCHARx
	pop eax
	pop eax
	popad
	ret
}

}



int DOCOMMAND(struct ConnectionInfo * conn)
{
	char Errbuff[500];
	int i;

//	PROCESS NORMAL MODE COMMAND

	wsprintf(Errbuff, "BPQHOST Port %d Normal Mode CMD %s\n",conn->ComPort, conn->LINEBUFFER);  
	OutputDebugString(Errbuff);

 
	if (conn->LINEBUFFER[0] != 0x1b)
		goto NOTCOMMAND;		// DATA IN NORMAL MODE - IGNORE

//	IF ECHO ENABLED, ECHO IT

	if (conn->ECHOFLAG)
	{
		UCHAR * ptr = conn->LINEBUFFER;
		UCHAR c;

		do 
		{
			c = *(ptr++);
			
			if (c == 0x1b) c = ':';

			PUTCHARx(conn, c);

		} while (c != 0x13);
	}

	switch (toupper(conn->LINEBUFFER[1]))
	{

	
	case 'J':

		if (conn->LINEBUFFER[6] == 0x0d)
			conn->InHostMode = 0;
		else
			conn->InHostMode = conn->LINEBUFFER[6] & 1;

//		DisableAppl();

		if (conn->InHostMode)
		{
			//	send host mode ack

			PUTCHARx(conn, 0);
			PUTCHARx(conn, 0);

			conn->CURSOR = (PUCHAR)&conn->LINEBUFFER;
			return 0;
		}

		break;

	case 'E':

		conn->ECHOFLAG = conn->LINEBUFFER[2] & 1;
		break;

	case 'I':
		break;;

	case 'P':

//	PARAMS COMMAND - RETURN FIXED STRING

		PARAMPORT = conn->LINEBUFFER[2];

		for (i=0; i < LPARAMREPLY; i++)
		{
			PUTCHARx(conn, PARAMREPLY[i]);
		}

		break;

	default:

		break;

	}

	PUTCHARx(conn, 'c');
	PUTCHARx(conn, 'm');
	PUTCHARx(conn, 'd');
	PUTCHARx(conn, ':');
	PUTCHARx(conn, 13);

NOTCOMMAND:

	conn->CURSOR = (PUCHAR)&conn->LINEBUFFER;

	return 0;

}

int PUTCHARx(struct ConnectionInfo * conn, UCHAR c)
{
	*(conn->PUTPTR++) = c;

	if (conn->PUTPTR == &conn->PCBUFFER[512])
		conn->PUTPTR = (PUCHAR)&conn->PCBUFFER;

	conn->RXCOUNT++;
	
	return 0;
}





