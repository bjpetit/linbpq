
// Version 2.0.1 November 2007

// Change resizing algorithm


// Version 2.0.2 January 2008

// Restore Checked state of Bells and AutoConnect Flags
// Call CheckTimer on startup (for new Initialisation Scheme for perl)

// Version 2.0.3 July 2008

// Display lines received without a terminaing CR


// Version 2.0.4 November 2008

// Add option to remove a Line Feed following a CR
// Add Logging Option

// Version 2.0.5 January 2009

// Add Start Minimized Option

// Version 2.0.6 June 2009

// Add Option to send *** Disconnnected on disconnect
// Add line wrap code
// Add option not to monitor NODES broadcasts

// Version 2.0.7 October 2009

// Add input buffer scrollback.
// Fix monitoring when PORTNUM specified

// Version 2.0.8 December 2009

// Fix use of numeric keypad 2 and 8 (were treated as up and down)

// Version 2.0.9 March 2010

// Add colour for monitor and BPQ Chat

// Version 2.0.0 October 2010

// Add Chat Terminal Mode (sends keepalives)

#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>

#include "bpqtermTCP.h"
#include "GetVersion.h"

#define BGCOLOUR RGB(236,233,216)

#define WSA_ACCEPT WM_USER + 1
#define WSA_CONNECT WM_USER + 2
#define WSA_DATA WM_USER + 3

#define BPQBASE 1100

HBRUSH bgBrush;

HINSTANCE hInst; 
char AppName[] = "BPQTerm 32";
char ClassName[] = "BPQMAINWINDOW";
char Title[80];

// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
int NewLine();

int	ProcessBuff(char * readbuff,int len);

int EnableConnectMenu(HWND hWnd);
int EnableDisconnectMenu(HWND hWnd);
int	DisableConnectMenu(HWND hWnd);
int DisableDisconnectMenu(HWND hWnd);
int DoReceivedData(HWND hWnd);
int	DoStateChange(HWND hWnd);
int ToggleFlags(HWND hWnd, int Item, int mask);
int CopyScreentoBuffer(char * buff);
int DoMonData(HWND hWnd);
int TogglePort(HWND hWnd, int Item, int mask);
int ToggleMTX(HWND hWnd);
int ToggleMCOM(HWND hWnd);
int ToggleParam(HWND hWnd, BOOL * Param, int Item);
int ToggleChat(HWND hWnd);
void MoveWindows();
void CopyToClipboard(HWND hWnd);
BOOL OpenMonitorLogfile();
void WriteMonitorLine(char * Msg, int MsgLen);
VOID WritetoOutputWindow(char * Msg, int len);
int SendMsg(char * msg, int len);
TCPConnect(char * Host, int Port);
int Telnet_Connected(SOCKET sock, int Error);
int DataSocket_Read(SOCKET sock);
int Socket_Data(int sock, int error, int eventcode);

COLORREF Colours[256] = {0,
		RGB(0,0,0), RGB(0,0,128), RGB(0,0,192), RGB(0,0,255),				// 1 - 4
		RGB(0,64,0), RGB(0,64,128), RGB(0,64,192), RGB(0,64,255),			// 5 - 8
		RGB(0,128,0), RGB(0,128,128), RGB(0,128,192), RGB(0,128,255),		// 9 - 12
		RGB(0,192,0), RGB(0,192,128), RGB(0,192,192), RGB(0,192,255),		// 13 - 16
		RGB(0,255,0), RGB(0,255,128), RGB(0,255,192), RGB(0,255,255),		// 17 - 20

		RGB(64,0,0), RGB(64,0,128), RGB(64,0,192), RGB(0,0,255),				// 21 
		RGB(64,64,0), RGB(64,64,128), RGB(64,64,192), RGB(64,64,255),
		RGB(64,128,0), RGB(64,128,128), RGB(64,128,192), RGB(64,128,255),
		RGB(64,192,0), RGB(64,192,128), RGB(64,192,192), RGB(64,192,255),
		RGB(64,255,0), RGB(64,255,128), RGB(64,255,192), RGB(64,255,255),

		RGB(128,0,0), RGB(128,0,128), RGB(128,0,192), RGB(128,0,255),				// 41
		RGB(128,64,0), RGB(128,64,128), RGB(128,64,192), RGB(128,64,255),
		RGB(128,128,0), RGB(128,128,128), RGB(128,128,192), RGB(128,128,255),
		RGB(128,192,0), RGB(128,192,128), RGB(128,192,192), RGB(128,192,255),
		RGB(128,255,0), RGB(128,255,128), RGB(128,255,192), RGB(128,255,255),

		RGB(192,0,0), RGB(192,0,128), RGB(192,0,192), RGB(192,0,255),				// 61
		RGB(192,64,0), RGB(192,64,128), RGB(192,64,192), RGB(192,64,255),
		RGB(192,128,0), RGB(192,128,128), RGB(192,128,192), RGB(192,128,255),
		RGB(192,192,0), RGB(192,192,128), RGB(192,192,192), RGB(192,192,255),
		RGB(192,255,0), RGB(192,255,128), RGB(192,255,192), RGB(192,2552,255),

		RGB(255,0,0), RGB(255,0,128), RGB(255,0,192), RGB(255,0,255),				// 81
		RGB(255,64,0), RGB(255,64,128), RGB(255,64,192), RGB(255,64,255),
		RGB(255,128,0), RGB(255,128,128), RGB(255,128,192), RGB(255,128,255),
		RGB(255,192,0), RGB(255,192,128), RGB(255,192,192), RGB(255,192,255),
		RGB(255,255,0), RGB(255,255,128), RGB(255,255,192), RGB(255,2552,255)
};



LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY SplitProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;


WNDPROC wpOrigInputProc; 
WNDPROC wpOrigOutputProc; 
WNDPROC wpOrigMonProc; 
WNDPROC wpOrigSplitProc; 

HWND hwndInput;
HWND hwndOutput;
HWND hwndMon;
HWND hwndSplit;

int xsize,ysize;		// Screen size at startup

double Split=0.5;
int SplitPos=300;
#define SplitBarHeight 5
#define InputBoxHeight 25
RECT Rect;
RECT MonRect;
RECT OutputRect;
RECT SplitRect;

int Height, Width, LastY;

int maxlinelen = 80;

char Host[100] = "localhost";
int Port = 0;
char UserName[80] = "";
char Password[80] = "";

SOCKET sock;

char Key[80];
int portmask=0;
int mtxparam=1;
int mcomparam=1;

char kbbuf[160];
int kbptr=0;
char readbuff[100000];				// for stupid bbs programs

int ptr=0;

int Stream;
int len,count;

char callsign[10];
int state;
int change;
int applmask = 0;
int applflags = 2;				// Message to Application
int Sessno = 0;

int PartLinePtr=0;
int PartLineIndex=0;		// Listbox index of (last) incomplete line

BOOL Bells = FALSE;
BOOL StripLF = FALSE;
BOOL LogMonitor = FALSE;
BOOL LogOutput = FALSE;
BOOL SendDisconnected = TRUE;
BOOL MonitorNODES = TRUE;
BOOL MonitorColour = TRUE;
BOOL ChatMode = FALSE;

HANDLE 	MonHandle=INVALID_HANDLE_VALUE;

HCURSOR DragCursor;
HCURSOR	Cursor;

LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;
//BOOL StartMinimized = FALSE;

HWND MainWnd, hWnd;

BOOL SocketActive = FALSE;
BOOL Connecting = FALSE;
BOOL Connected = FALSE;

VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime );

TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;

int TimerHandle = 0;

int SlowTimer;

VOID CALLBACK TimerProc(

    HWND  hwnd,	// handle of window for timer messages 
    UINT  uMsg,	// WM_TIMER message
    UINT  idEvent,	// timer identifier
    DWORD  dwTime) 	// current system time	
{
	// entered every 10 secs

	if (!Connected)
		return;

	if (!ChatMode)
		return;

	SlowTimer++;

	if (SlowTimer > 50)				// About 9 mins
	{
		SlowTimer = 0;
		SendMsg("\0", 1);
	}
}

//
//  FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
//
//  PURPOSE: Entry point for the application.
//
//  COMMENTS:
//
//	This function initializes the application and processes the
//	message loop.
//
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];

/*	PCHAR p1, p2, p3;

	p1 = strtok(lpCmdLine, " ,\t\n\r");
	p2 = strtok(NULL, " ,\t\n\r");
	p3 = strtok(NULL, " ,\t\n\r");

	if (nCmdShow == SW_SHOWMINIMIZED)
		StartMinimized = TRUE;

	if (p1)
	{
		if (strlen(p1) > 5)
		{
			if (_stricmp(p1, "StartMinimized") == 0)
				StartMinimized = TRUE;

			if (p2) Sessno = atoi(p2);
			if (p3) sscanf(p3,"%x", &applflags);
		}
		else 
		{		
			if (p2)
			{
				if (strlen(p2) > 5)
				{
					if (_stricmp(p2, "StartMinimized") == 0)
						StartMinimized = TRUE;
		
					if (p1) Sessno = atoi(p1);
					if (p3) sscanf(p3,"%x", &applflags);
				}
			}
			else
			{
				if (p3)
				{
					if (strlen(p3) > 5)
					{
						if (_stricmp(p3, "StartMinimized") == 0)
							StartMinimized = TRUE;

						if (p1) Sessno = atoi(p1);
						if (p2) sscanf(p2,"%x", &applflags);
					}
				}
			}
		}
	}
*/
	sscanf(lpCmdLine,"%d %x",&Sessno, &applflags);

	if (!InitApplication(hInstance)) 
			return (FALSE);

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// Save Config
	
	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              Key,
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

	if (retCode == ERROR_SUCCESS)
	{
		retCode = RegSetValueEx(hKey,"PortMask",0,REG_DWORD,(BYTE *)&portmask,4);
		retCode = RegSetValueEx(hKey,"Bells",0,REG_DWORD,(BYTE *)&Bells,4);
		retCode = RegSetValueEx(hKey,"StripLF",0,REG_DWORD,(BYTE *)&StripLF,4);
		retCode = RegSetValueEx(hKey,"Host",0,REG_SZ,Host, strlen(Host));
		retCode = RegSetValueEx(hKey,"User",0,REG_SZ, UserName, strlen(UserName));
		retCode = RegSetValueEx(hKey,"Password",0,REG_SZ,Password, strlen(Password));
		retCode = RegSetValueEx(hKey,"Port",0,REG_DWORD,(BYTE *)&Port,4);
		retCode = RegSetValueEx(hKey,"MTX",0,REG_DWORD,(BYTE *)&mtxparam,4);
		retCode = RegSetValueEx(hKey,"MCOM",0,REG_DWORD,(BYTE *)&mcomparam,4);
		retCode = RegSetValueEx(hKey,"Split",0,REG_BINARY,(BYTE *)&Split,sizeof(Split));
		retCode = RegSetValueEx(hKey,"MONColour",0,REG_DWORD,(BYTE *)&MonitorColour,4);

		wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
		retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));

		RegCloseKey(hKey);
	}

	KillTimer(NULL, TimerHandle);

	return (msg.wParam);
}

//

//
//  FUNCTION: InitApplication(HANDLE)
//
//  PURPOSE: Initializes window data and registers window class 
//
//  COMMENTS:
//
//       In this function, we initialize a window class by filling out a data
//       structure of type WNDCLASS and calling either RegisterClass or 
//       the internal MyRegisterClass.
//
BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;

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

	return (RegisterClass(&wc));

}

HMENU hMenu, hPopMenu1, hPopMenu2, hPopMenu3;		// handle of menu 


//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window 
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int i, tempmask=0xffff;
	char msg[20];
	int retCode,Type,Vallen;
	HKEY hKey=0;
	char Size[80];
	WSADATA WsaData;            // receives data from WSAStartup

	hInst = hInstance; // Store instance handle in our global variable

	WSAStartup(MAKEWORD(2, 0), &WsaData);


	// Create a dialog box as the main window

	hWnd = CreateDialog(hInst,ClassName,0,NULL);
	
    if (!hWnd)
        return (FALSE);

	MainWnd=hWnd;

	// Retrieve the handlse to the edit controls. 

	hwndInput = GetDlgItem(hWnd, 118); 
	hwndOutput = GetDlgItem(hWnd, 117); 
	hwndSplit = GetDlgItem(hWnd, 119); 
	hwndMon = GetDlgItem(hWnd, 116); 
 
	// Set our own WndProcs for the controls. 

	wpOrigInputProc = (WNDPROC) SetWindowLong(hwndInput, GWL_WNDPROC, (LONG) InputProc); 
	wpOrigOutputProc = (WNDPROC)SetWindowLong(hwndOutput, GWL_WNDPROC, (LONG)OutputProc);
	wpOrigMonProc = (WNDPROC)SetWindowLong(hwndMon, GWL_WNDPROC, (LONG)MonProc);
	wpOrigSplitProc = (WNDPROC)SetWindowLong(hwndSplit, GWL_WNDPROC, (LONG)SplitProc);

	// Get saved config from Registry

	// Get config from Registry 

	wsprintf(Key,"SOFTWARE\\G8BPQ\\BPQ32\\BPQTermTCP");

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"ApplMask",0,			
			(ULONG *)&Type,(UCHAR *)&applmask,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MTX",0,			
			(ULONG *)&Type,(UCHAR *)&mtxparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MCOM",0,			
			(ULONG *)&Type,(UCHAR *)&mcomparam,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"PortMask",0,			
			(ULONG *)&Type,(UCHAR *)&tempmask,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"MONColour",0,			
			(ULONG *)&Type,(UCHAR *)&MonitorColour,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Bells",0,			
			(ULONG *)&Type,(UCHAR *)&Bells,(ULONG *)&Vallen);
	
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"StripLF",0,			
			(ULONG *)&Type,(UCHAR *)&StripLF,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "SendDisconnected",0,			
			(ULONG *)&Type,(UCHAR *)&StripLF,(ULONG *)&Vallen);
	
		Vallen=8;
		retCode = RegQueryValueEx(hKey,"Split",0,			
			(ULONG *)&Type,(UCHAR *)&Split,(ULONG *)&Vallen);

		Vallen=80;
		retCode = RegQueryValueEx(hKey,"Host",0,			
			(ULONG *)&Type,(UCHAR *)&Host,(ULONG *)&Vallen);

		Vallen=80;
		retCode = RegQueryValueEx(hKey,"User",0,			
			(ULONG *)&Type,(UCHAR *)&UserName,(ULONG *)&Vallen);

		Vallen=80;
		retCode = RegQueryValueEx(hKey,"Password",0,			
			(ULONG *)&Type,(UCHAR *)&Password,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Port",0,			
			(ULONG *)&Type,(UCHAR *)&Port,(ULONG *)&Vallen);

		retCode = RegSetValueEx(hKey,"Host",0,REG_SZ,Host, strlen(Host));
		retCode = RegSetValueEx(hKey,"User",0,REG_SZ, UserName, strlen(UserName));
		retCode = RegSetValueEx(hKey,"Password",0,REG_SZ,Password, strlen(Password));
		retCode = RegSetValueEx(hKey,"Port",0,REG_DWORD,(BYTE *)&Port,4);


		Vallen=80;
		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);
	
	}

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(hWnd,	&Rect);
	}

	Height = Rect.bottom-Rect.top;
	Width = Rect.right-Rect.left;

	SplitPos=Height*Split;

	MoveWindow(hWnd,Rect.left,Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

	MoveWindows();

	//
	//	Enable Async notification
	//
	
	GetVersionInfo(NULL);

	wsprintf(Title,"BPQTermTCPVersion %s", VersionString);

	SetWindowText(hWnd,Title);
		

//	Sets Application Flags and Mask for stream. (BPQHOST function 1)
//	AH = 1	Set application mask to value in DL (or even DX if 16
//		applications are ever to be supported).
//
//		Set application flag(s) to value in CL (or CX).
//		whether user gets connected/disconnected messages issued
//		by the node etc.
//
//		Top bit of flags controls monitoring

	hMenu=GetMenu(hWnd);

	hPopMenu1=GetSubMenu(hMenu,1);

	for (i=1;i <= 2;i++)
	{
		wsprintf(msg,"Port %d",i);

		if (tempmask & (1<<(i-1)))
		{
			AppendMenu(hPopMenu1,MF_STRING | MF_CHECKED,BPQBASE + i,msg);
			portmask |= (1<<(i-1));
		}
		else
			AppendMenu(hPopMenu1,MF_STRING | MF_UNCHECKED,BPQBASE + i,msg);
	}
	
	if (mtxparam & 1)
		CheckMenuItem(hMenu,BPQMTX,MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQMTX,MF_UNCHECKED);

	if (mcomparam & 1)
		CheckMenuItem(hMenu,BPQMCOM,MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQMCOM,MF_UNCHECKED);
	
  	if (Bells & 1)
		CheckMenuItem(hMenu,BPQBELLS, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQBELLS, MF_UNCHECKED);

  	if (SendDisconnected & 1)
		CheckMenuItem(hMenu,BPQSendDisconnected, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQSendDisconnected, MF_UNCHECKED);

  	if (StripLF & 1)
		CheckMenuItem(hMenu,BPQStripLF, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQStripLF, MF_UNCHECKED);

	CheckMenuItem(hMenu,BPQMNODES, (MonitorNODES) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(hMenu,MONCOLOUR, (MonitorColour) ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(hWnd);	

	if (portmask) applflags |= 0x80;

//	SetTraceOptions(portmask,mtxparam,mcomparam);
	

	DragCursor = LoadCursor(hInstance, "IDC_DragSize");
	Cursor = LoadCursor(NULL, IDC_ARROW);

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (MinimizetoTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);

	UpdateWindow(hWnd);

	TimerHandle = SetTimer(NULL, 0, 10000, lpTimerFunc);

	return (TRUE);
}


INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, IDC_HOST, Host);
		SetDlgItemInt(hDlg, IDC_PORT, Port, FALSE);
		SetDlgItemText(hDlg, IDC_USER, UserName);
		SetDlgItemText(hDlg, IDC_PASS, Password);

		return (INT_PTR)TRUE;

	case WM_CTLCOLORDLG:
        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case IDOK:

			GetDlgItemText(hDlg, IDC_HOST, Host, 99);
			Port = GetDlgItemInt(hDlg, IDC_PORT,NULL, FALSE);
			GetDlgItemText(hDlg, IDC_USER, UserName, 79);
			GetDlgItemText(hDlg, IDC_PASS, Password, 79);

		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
	}

	return FALSE;

}


//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPRECT lprc;
	UCHAR Buffer[1000];
	UCHAR * buf = Buffer;
    TEXTMETRIC tm; 
    int y;  
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis; 

	switch (message) 
	{
	case WSA_DATA: // Notification on data socket

		Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		return 0;

	case WSA_CONNECT: /* Notification if a socket connection is pending. */

		Telnet_Connected(wParam, WSAGETSELECTERROR(lParam));
		return 0;

   
	case WM_MEASUREITEM: 
 
		lpmis = (LPMEASUREITEMSTRUCT) lParam; 
		return TRUE; 
 
	case WM_DRAWITEM: 
 
		lpdis = (LPDRAWITEMSTRUCT) lParam; 
 
		// If there are no list box items, skip this message. 
 
		if (lpdis->itemID == -1) 
		{ 
			return TRUE; 
		} 
 
            switch (lpdis->itemAction) 
            { 
				case ODA_SELECT: 
                case ODA_DRAWENTIRE: 
 
				  // if Chat Console, and message has a colour eacape, action it 
					
					SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM) Buffer); 
 
                    GetTextMetrics(lpdis->hDC, &tm); 
 
                    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;

					if (Buffer[0] == 0x1b)
					{
						SetTextColor(lpdis->hDC,  Colours[Buffer[1] - 10]);
						buf += 2;
					}
//					SetBkColor(lpdis->hDC, 0);

                    TextOut(lpdis->hDC, 
                        6, 
                        y, 
                        buf, 
                        strlen(buf)); 						
 
 //					SetTextColor(lpdis->hDC, OldColour);

                    break; 
			}

			return TRUE;

		case WM_ACTIVATE:

			SetFocus(hwndInput);
			break;



	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		//Parse the menu selections:
		
		if (wmId > BPQBASE && wmId < BPQBASE + 32)
		{
			TogglePort(hWnd,wmId,0x1 << (wmId - (BPQBASE + 1)));
			break;
		}
		
		switch (wmId) {
        
		case BPQMTX:
	
			ToggleMTX(hWnd);
			break;

		case BPQMCOM:

			ToggleMCOM(hWnd);
			break;

		case BPQCONNECT:

			TCPConnect(Host, Port);
			break;
			        
		case BPQDISCONNECT:
			
			shutdown(sock, 2);		// SD_BOTH
			break;
						
		case BPQBELLS:

			ToggleParam(hWnd, &Bells, BPQBELLS);
			break;

		case BPQStripLF:

			ToggleParam(hWnd, &StripLF, BPQStripLF);
			break;

		case BPQLogOutput:

			ToggleParam(hWnd, &LogOutput, BPQLogOutput);
			break;

		case CHATTERM:

			ToggleParam(hWnd, &ChatMode, CHATTERM);
			break;

		case BPQ_CONFIG:

			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hWnd, ConfigWndProc);
			break;

		case BPQMNODES:

			ToggleParam(hWnd, &MonitorNODES, BPQMNODES);
			break;

		case MONCOLOUR:

			ToggleParam(hWnd, &MonitorColour, MONCOLOUR);
			break;

		case BPQLogMonitor:

			ToggleParam(hWnd, &LogMonitor, BPQLogMonitor);
			break;

		case BPQCHAT:

			ToggleChat(hWnd);
			break;

		case BPQCLEARMON:

			SendMessage(hwndMon,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCLEAROUT:

			SendMessage(hwndOutput,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCOPYMON:

			CopyToClipboard(hwndMon);
			break;

		case BPQCOPYOUT:
		
			CopyToClipboard(hwndOutput);
			break;

		case BPQHELP:

//			HtmlHelp(hWnd,"BPQTerminal.chm",HH_HELP_FINDER,0);  
			break;

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

		case WM_SIZING:

			lprc = (LPRECT) lParam;

			Height = lprc->bottom-lprc->top;
			Width = lprc->right-lprc->left;

			SplitPos=Height*Split;

			MoveWindows();
			
			return TRUE;


		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 

			GetWindowRect(hWnd,	&Rect);	// For save soutine

            SetWindowLong(hwndInput, GWL_WNDPROC, 
                (LONG) wpOrigInputProc); 
         
			PostQuitMessage(0);

			
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}

static char * KbdStack[20];

int StackIndex=0;
 
LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	char DisplayLine[200] = "\x1b\xb";
	
	if (uMsg == WM_KEYUP)
	{
		unsigned int i;

//		Debugprintf("5%x", LOBYTE(HIWORD(lParam)));

		if (LOBYTE(HIWORD(lParam)) == 0x48 && wParam == 0x26)
		{
			// Scroll up

			if (KbdStack[StackIndex] == NULL)
				return TRUE;

			SendMessage(hwndInput, WM_SETTEXT,0,(LPARAM)(LPCSTR) KbdStack[StackIndex]);
			
			for (i = 0; i < strlen(KbdStack[StackIndex]); i++)
			{
				SendMessage(hwndInput, WM_KEYDOWN, VK_RIGHT, 0);
				SendMessage(hwndInput, WM_KEYUP, VK_RIGHT, 0);
			}

			StackIndex++;
			if (StackIndex == 20)
				StackIndex = 19;

			return TRUE;
		}

		if (LOBYTE(HIWORD(lParam)) == 0x50 && wParam == 0x28)
		{
			// Scroll up

			StackIndex--;
			if (StackIndex < 0)
				StackIndex = 0;

			if (KbdStack[StackIndex] == NULL)
				return TRUE;
			
			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) KbdStack[StackIndex]);

			for (i = 0; i < strlen(KbdStack[StackIndex]); i++)
			{
				SendMessage(hwndInput, WM_KEYDOWN, VK_RIGHT, 0);
				SendMessage(hwndInput, WM_KEYUP, VK_RIGHT, 0);
			}
			
			return TRUE;
		}
	}

	if (uMsg == WM_CHAR) 
	{
		if (wParam == 13)
		{
			int i;
	
			if (!Connected && ! Connecting)
				TCPConnect(Host, Port);

			kbptr=SendMessage(hwndInput,WM_GETTEXT,159,(LPARAM) (LPCSTR) kbbuf);

			// Stack it

			StackIndex = 0;

			if (KbdStack[19])
				free(KbdStack[19]);

			for (i = 18; i >= 0; i--)
			{
				KbdStack[i+1] = KbdStack[i];
			}

			KbdStack[0] = _strdup(kbbuf);

			kbbuf[kbptr]=13;

			SlowTimer = 0;

			// Echo, with set Black escape

			memcpy(&DisplayLine[2], kbbuf, kbptr+1);

			WritetoOutputWindow(DisplayLine, kbptr+3);
		
			// Replace null with CR, and send to Node

			SendMsg(&kbbuf[0], kbptr+1);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
		if (wParam == 0x1a)  // Ctrl/Z
		{	
			kbbuf[0]=0x1a;
			kbbuf[1]=13;

			SendMsg(&kbbuf[0], 2);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

        return 0; 
		}

	}
 
    return CallWindowProc(wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 

//int FirstItem=-1, LastItem=-1, SELECTING=FALSE, MOVED=FALSE, LastSelected=0;

LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	// Trap mouse messages, so we cant select stuff in output and mon windows,
	//	otherwise scrolling doesnt work.

/*	if (uMsg == WM_MOUSEMOVE)
	{
		if (SELECTING)
		{
			MOVED=TRUE;
			LastItem = SendMessage(hwndOutput, LB_ITEMFROMPOINT, 0, lParam );
		
			if (LastItem < LastSelected)
				SendMessage(hwndOutput, LB_SETSEL, FALSE, LastSelected);

			if (LastItem > LastSelected)
				SendMessage(hwndOutput, LB_SETSEL, TRUE, LastItem);

			LastSelected=LastItem;
		}

        return TRUE; 
	}

	if (uMsg == WM_LBUTTONDOWN) 
	{
		SendMessage(hwndOutput, LB_SELITEMRANGE, 0, MAKELPARAM(0, 32767));

		FirstItem = SendMessage(hwndOutput, LB_ITEMFROMPOINT, 0, lParam );
		SendMessage(hwndOutput, LB_SETSEL, TRUE, FirstItem);

		SELECTING=TRUE;
		MOVED=FALSE;

        return TRUE; 
	}

	if (uMsg == WM_LBUTTONUP) 
	{
		LastItem = SendMessage(hwndOutput, LB_ITEMFROMPOINT, 0, lParam );
		
		SendMessage(hwndOutput, LB_SELITEMRANGE, 0, MAKELPARAM(0, 32767));
		if (MOVED) SendMessage(hwndOutput, LB_SELITEMRANGE, 1, MAKELPARAM(FirstItem, LastItem));

		FirstItem=-1;

		SELECTING=FALSE;

		return TRUE; 
	}
*/

	if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) 
        return TRUE; 

	return CallWindowProc(wpOrigOutputProc, hwnd, uMsg, wParam, lParam); 
} 

LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) 
        return TRUE; 
 
    return CallWindowProc(wpOrigMonProc, hwnd, uMsg, wParam, lParam); 
} 

LRESULT APIENTRY SplitProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int xPos, yPos;
	
	switch (uMsg) { 

		case WM_LBUTTONDOWN:

			SetCapture(hwnd);
			return 0;

		case WM_LBUTTONUP:

			xPos = LOWORD(lParam);  // horizontal position of cursor 
			yPos = lParam >> 16;  // vertical position of cursor 

			SplitPos = SplitPos + yPos - 5;

			if (SplitPos < 0) SplitPos = 0;

			Split=SplitPos;
			Split=Split/Height;

			ReleaseCapture();
			MoveWindows();
			return 0;

		case WM_MOUSEMOVE:

 			// Used to size split between Monitor and Output Windows

			SetCursor(DragCursor);

/*			if (wParam && MK_LBUTTON)
			{
				xPos = LOWORD(lParam);  // horizontal position of cursor 
				yPos = lParam >> 16;  // vertical position of cursor 
 
				if (LastY == 0)
	
					LastY=yPos;

				else
				{
					if (yPos > LastY)
						SplitPos = SplitPos + 1;
					if (yPos < LastY)
						SplitPos = SplitPos - 1;

//					if (Split < .1) Split = .1;
//					if (Split > .9) Split = .9;

					LastY=yPos;


//					MoveWindows();
				}
				return 0;
			}
			else

				LastY=0;

			break;
*/	

			break;



	}

    return CallWindowProc(wpOrigSplitProc, hwnd, uMsg, wParam, lParam); 
} 



		
VOID WritetoOutputWindow(char * Msg, int len)
{
	char * ptr1, * ptr2;
	int index;

		
			if (PartLinePtr != 0)
			{
				SendMessage(hwndOutput,LB_DELETESTRING,PartLineIndex,(LPARAM)(LPCTSTR) 0 );	
				if (Msg[0] == 0x1b && len > 1) 
				{
					Msg += 2;		// Remove Colour Escape
					len -= 2;
				}
			}
			memcpy(&readbuff[PartLinePtr], Msg, len);
		
			len=len+PartLinePtr;

			ptr1=&readbuff[0];
			readbuff[len]=0;

			if (Bells)
			{
				do {

					ptr2=memchr(ptr1,7,len);
					
					if (ptr2)
					{
						*(ptr2)=32;
						Beep(440,250);
					}
	
				} while (ptr2);

			}

		lineloop:

			if (len > 0)
			{
				//	copy text to control a line at a time	
	
				ptr2=memchr(ptr1,13,len);
				
				if (ptr2 == 0)
				{
					// no newline. Move data to start of buffer and Save pointer

					PartLinePtr=len;

					PartLineIndex=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
			
					memmove(readbuff,ptr1,len);

					SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) PartLineIndex, MAKELPARAM(FALSE, 0));

					return;

				}
				else
				{
					*(ptr2++)=0;

					if (LogOutput) WriteMonitorLine(ptr1, ptr2 - ptr1);
					
					// If len is greater that screen with, fold

					if ((ptr2 - ptr1) > maxlinelen)
					{
						char * ptr3;
						char * saveptr1 = ptr1;
						int linelen = ptr2 - ptr1;
						int foldlen;
						char save;
					
					foldloop:

						ptr3 = ptr1 + maxlinelen;
					
						while(*ptr3!= 0x20 && ptr3 > ptr1)
						{
							ptr3--;
						}
						
						foldlen = ptr3 - ptr1 ;

						if (foldlen == 0)
						{
							// No space before, so split at width

							foldlen = maxlinelen;
							ptr3 = ptr1 + maxlinelen;

						}
						else
						{
							ptr3++ ; // Omit space
							linelen--;
						}
						save = ptr1[foldlen];
						ptr1[foldlen] = 0;
						index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );					
						ptr1[foldlen] = save;
						linelen -= foldlen;
						ptr1 = ptr3;

						if (linelen > maxlinelen)
							goto foldloop;
						
						index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );					
						ptr1 = saveptr1;
					}

					else
					{
						index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );					
					}


					PartLinePtr=0;

					len-=(ptr2-ptr1);

					ptr1=ptr2;

					if ((len > 0) && StripLF)
					{
						if (*ptr1 == 0x0a)					// Line Feed
						{
							ptr1++;
							len--;
						}
					}

					if (index > 1200)
						
					do{

						index=SendMessage(hwndOutput,LB_DELETESTRING, 0, 0);

					
						} while (index > 1000);

					SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

					goto lineloop;
				}
}
}			

DoMonData(HWND hWnd)
{
/*	char * ptr1, * ptr2;
	int index, stamp;
	int len;
	unsigned char buffer[1024] = "\x1b\xb", monbuff[512];


if (MONCount(Stream) > 0)
	{
		do {
		
			stamp=GetRaw(Stream, monbuff,&len,&count);

			if (MonitorColour)
			{
				if (monbuff[4] & 0x80)		// TX
					buffer[1] = 91;
				else
					buffer[1] = 17;
			}

			// See if a NODES

			if (!MonitorNODES && monbuff[21] == 3 && monbuff[22] == 0xcf && monbuff[23] == 0xff)
				len = 0;
			else
			{
				len=DecodeFrame(monbuff,&buffer[2],stamp);
				len +=2;
			}

			ptr1=&buffer[0];

		lineloop:

			if (len > 0)
			{
				//	copy text to control a line at a time	

				ptr2=memchr(ptr1,13,len);
				
				if (ptr2 == 0)
				{
					// no newline. Move data to start of buffer and Save pointer

					memmove(buffer,ptr1,len);

					return (0);

				}
				else
				{
					*(ptr2++)=0;

					index=SendMessage(hwndMon,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );

					if (LogMonitor) WriteMonitorLine(ptr1, ptr2 - ptr1);

					if (index > 1200)

					do{

						index=SendMessage(hwndMon,LB_DELETESTRING, 0, 0);
					
						} while (index > 1000);

					SendMessage(hwndMon,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

					len-=(ptr2-ptr1);

					ptr1=ptr2;
					
					goto lineloop;
				}
			}
			
		} while (count > 0);
	}
	*/
	return (0);
}





int DisableConnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQCONNECT,MF_GRAYED);

	return (0);
}	
int DisableDisconnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQDISCONNECT,MF_GRAYED);
	return (0);
}	

int	EnableConnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQCONNECT,MF_ENABLED);
	return (0);
}	

int EnableDisconnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQDISCONNECT,MF_ENABLED);

    return (0);
}


int ToggleAppl(HWND hWnd, int Item, int mask)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	
	applmask = applmask ^ mask;
	
	if (applmask & mask)

		CheckMenuItem(hMenu,Item,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

//	SetAppl(Stream,applflags,applmask);

    return (0);
  
}

int ToggleFlags(HWND hWnd, int Item, int mask)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	
	applflags ^= mask;
	
	if (applflags & mask)

		CheckMenuItem(hMenu,Item,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

    return (0);
  
}

CopyScreentoBuffer(char * buff)
{

	return (0);
}
	
int TogglePort(HWND hWnd, int Item, int mask)
{
	portmask ^= mask;
	
	if (portmask & mask)
		CheckMenuItem(hMenu,Item,MF_CHECKED);
	else
		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

	if (portmask)
		applflags |= 0x80;
	else
		applflags &= 0x7f;

//	SetAppl(Stream,applflags,applmask);

//	SetTraceOptions(portmask,mtxparam,mcomparam);

    return (0);
  
}
int ToggleMTX(HWND hWnd)
{
	mtxparam = mtxparam ^ 1;
	
	if (mtxparam & 1)

		CheckMenuItem(hMenu,BPQMTX,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQMTX,MF_UNCHECKED);

//	SetTraceOptions(portmask,mtxparam,mcomparam);

    return (0);
  
}
int ToggleMCOM(HWND hWnd)
{
	mcomparam = mcomparam ^ 1;
	
	if (mcomparam & 1)

		CheckMenuItem(hMenu,BPQMCOM,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQMCOM,MF_UNCHECKED);

//	SetTraceOptions(portmask,mtxparam,mcomparam);

    return (0);
  
}
int ToggleParam(HWND hWnd, BOOL * Param, int Item)
{
	*Param = !(*Param);

	CheckMenuItem(hMenu,Item, (*Param) ? MF_CHECKED : MF_UNCHECKED);
	
    return (0);
}

int ToggleChat(HWND hWnd)
{	
	applmask = applmask ^ 02;
	
	if (applmask & 02)

		CheckMenuItem(hMenu,BPQCHAT,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQCHAT,MF_UNCHECKED);

//	SetAppl(Stream,applflags,applmask);

    return (0);
  
}
void MoveWindows()
{
	RECT rcMain, rcClient;
	int ClientHeight, ClientWidth;

	GetWindowRect(hWnd, &rcMain);
	GetClientRect(hWnd, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	MoveWindow(hwndMon,2, 0, ClientWidth-4, SplitPos, TRUE);
	MoveWindow(hwndOutput,2, SplitPos+SplitBarHeight, ClientWidth-4, ClientHeight-SplitPos-InputBoxHeight-SplitBarHeight-SplitBarHeight, TRUE);
	MoveWindow(hwndInput,2, ClientHeight-InputBoxHeight-2, ClientWidth-4, InputBoxHeight, TRUE);
	MoveWindow(hwndSplit,0, SplitPos, ClientWidth, SplitBarHeight, TRUE);

	GetClientRect(hwndOutput, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;
	
	maxlinelen = ClientWidth/8 - 1;
}

void CopyToClipboard(HWND hWnd)
{
	int i,n, len=0;
	HGLOBAL	hMem;
	char * ptr;
	//
	//	Copy List Box to clipboard
	//
	
	n = SendMessage(hWnd, LB_GETCOUNT, 0, 0);		
	
	for (i=0; i<n; i++)
	{
		len+=SendMessage(hWnd, LB_GETTEXTLEN, i, 0);
	}

	hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len+n+n+1);
	

	if (hMem != 0)
	{
		ptr=GlobalLock(hMem);
	
		if (OpenClipboard(MainWnd))
		{
			//			CopyScreentoBuffer(GlobalLock(hMem));
			
			for (i=0; i<n; i++)
			{
				ptr+=SendMessage(hWnd, LB_GETTEXT, i, (LPARAM) ptr);
				*(ptr++)=13;
				*(ptr++)=10;
			}

			*(ptr)=0;					// end of data

			GlobalUnlock(hMem);
			EmptyClipboard();
			SetClipboardData(CF_TEXT,hMem);
			CloseClipboard();
		}
			else
				GlobalFree(hMem);		
	}
}

BOOL OpenMonitorLogfile()
{
	UCHAR FN[MAX_PATH];

	wsprintf(FN,"BPQTerm_%d.log", Stream);

	MonHandle = CreateFile(FN,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					OPEN_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	SetFilePointer(MonHandle, 0, 0, FILE_END);

	return (MonHandle != INVALID_HANDLE_VALUE);
}

void WriteMonitorLine(char * Msg, int MsgLen)
{
	int cnt;
	char CRLF[2] = {0x0d,0x0a};

	if (MonHandle == INVALID_HANDLE_VALUE) OpenMonitorLogfile();

	if (MonHandle == INVALID_HANDLE_VALUE) return;

	WriteFile(MonHandle ,Msg , MsgLen, &cnt, NULL);
	WriteFile(MonHandle ,CRLF , 2, &cnt, NULL);
}

// TCP Interface

TCPConnect(char * Host, int Port)
{
	int err, status;
	u_long param=1;
	BOOL bcopt=TRUE;
	SOCKADDR_IN sinx; 
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	int i;
		
	sock = socket(AF_INET, SOCK_STREAM, 0);
	
	WSAAsyncSelect(sock, hWnd, WSA_DATA,
		FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
	
	// Resolve Name if needed

	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(Port);
	destaddr.sin_addr.s_addr = inet_addr(Host);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		struct hostent * HostEnt;

		//	Resolve name to address

		HostEnt = gethostbyname(Host);
		 
		 if (!HostEnt)
		 {
			MessageBox(NULL, "Resolve HostName Failed", "BPQTermTCP", MB_OK);
			return FALSE;			// Resolve failed
		 }

		 i = 0;

		 memcpy(&destaddr.sin_addr.s_addr, HostEnt->h_addr, 4);
	}

	ioctlsocket (sock, FIONBIO, &param);
 
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		MessageBox(NULL, "Bind Failed", "BPQTermTCP", MB_OK);
  	 	return FALSE; 
	}

	if ((status = WSAAsyncSelect(sock, hWnd, WSA_CONNECT, FD_CONNECT)) > 0)
	{
		closesocket(sock);
		return FALSE;
	}

	if (connect(sock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		return TRUE;
	}
	else
	{
		err=WSAGetLastError();

		if (err == WSAEWOULDBLOCK)
		{
			//	Connect in Progress

			wsprintf(Title,"BPQTermTCP Version %s - Connecting to %s", VersionString, Host);
			SetWindowText(hWnd,Title);

			return TRUE;
		}
		else
		{
			//	Connect failed

			closesocket(sock);
			MessageBox(NULL, "Connect Failed", "BPQTermTCP", MB_OK);
			return FALSE;
		}
	}
	return FALSE;
}

int Telnet_Connected(SOCKET sock, int Error)
{
	char Msg[80];
	int Len;

	// Connect Complete
				
	if (Error)
	{
		wsprintf(Msg, "Connect Failed - Error %d\r", Error);
					
		MessageBox(NULL, Msg, "BPQTermTCP", MB_OK);

		closesocket(sock);
		Connecting = FALSE;
		SocketActive = FALSE;
		return 0;

	}

	WSAAsyncSelect(sock, hWnd, WSA_DATA,
					FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
	
	SocketActive = TRUE;
	Connecting = FALSE;
	Connected = TRUE;

	Len = wsprintf(Msg, "%s\r%s\ri\r", UserName, Password);
	
	SendMsg(Msg, Len);

	SlowTimer = 0;
			
	wsprintf(Title,"BPQTermTCP Version %s - Connected to %s", VersionString, Host);
	SetWindowText(hWnd,Title);
	DisableConnectMenu(hWnd);
	EnableDisconnectMenu(hWnd);

	return 0;
}

int Socket_Data(int sock, int error, int eventcode)
{
	char Title[100];

	switch (eventcode)
	{
		case FD_READ:
			
			return DataSocket_Read(sock);

		case FD_WRITE:
		case FD_OOB:
		case FD_ACCEPT:
		case FD_CONNECT:

			return 0;

		case FD_CLOSE:

			if (SocketActive)
				closesocket(sock);

			wsprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
			SetWindowText(hWnd,Title);
			DisableDisconnectMenu(hWnd);
			EnableConnectMenu(hWnd);

			if (SendDisconnected)
			{
				int index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) "*** Disconnected");		
				SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));
			}

			SocketActive = FALSE;
			Connected = FALSE;
			return 0;
	}

	return 0;
}

int DataSocket_Read(SOCKET sock)
{
	int len=0;
	char Buffer[500];

	ioctlsocket(sock, FIONREAD, &len);

	if (len > 500) len = 500;

	len = recv(sock, Buffer, len, 0);

	if (len == SOCKET_ERROR || len == 0)
	{
		// Failed or closed - clear connection

		return 0;
	}

	WritetoOutputWindow(Buffer, len);
	SlowTimer = 0;

	return 0;
}


int SendMsg(char * msg, int len)
{
	send(sock, msg, len, 0);
	return 0;
}