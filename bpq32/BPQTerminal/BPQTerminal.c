
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
#include <htmlhelp.h>

#include "bpqterminal.h"
#include "bpq32.h"	// BPQ32 API Defines
#include "GetVersion.h"

#define BGCOLOUR RGB(236,233,216)

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
int	ToggleAutoConnect(HWND hWnd);
int ToggleAppl(HWND hWnd, int Item, int mask);
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

CONNECTED=FALSE;
AUTOCONNECT=TRUE;

LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;
//BOOL StartMinimized = FALSE;

HWND MainWnd;

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

	CheckTimer();

	if (!CONNECTED)
		return;

	if (!ChatMode)
		return;

	SlowTimer++;

	if (SlowTimer > 50)				// About 9 mins
	{
		SlowTimer = 0;
		SendMsg(Stream, "\0", 1);
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
		retCode = RegSetValueEx(hKey,"AutoConnect",0,REG_DWORD,(BYTE *)&AUTOCONNECT,4);
		retCode = RegSetValueEx(hKey,"PortMask",0,REG_DWORD,(BYTE *)&portmask,4);
		retCode = RegSetValueEx(hKey,"Bells",0,REG_DWORD,(BYTE *)&Bells,4);
		retCode = RegSetValueEx(hKey,"StripLF",0,REG_DWORD,(BYTE *)&StripLF,4);
		retCode = RegSetValueEx(hKey,"SendDisconnected",0,REG_DWORD,(BYTE *)&SendDisconnected,4);
		retCode = RegSetValueEx(hKey,"ApplMask",0,REG_DWORD,(BYTE *)&applmask,4);
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

	HWND hWnd;

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int i, n, tempmask=0xffff;
	char msg[20];
	int retCode,Type,Vallen;
	HKEY hKey=0;
	char Size[80];

	hInst = hInstance; // Store instance handle in our global variable

	MinimizetoTray=GetMinimizetoTrayFlag();

	// Create a dialog box as the main window

	hWnd=CreateDialog(hInst,ClassName,0,NULL);
	
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

	wsprintf(Key,"SOFTWARE\\G8BPQ\\BPQ32\\BPQTerminal\\Session%d",Sessno);

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
		retCode = RegQueryValueEx(hKey,"AutoConnect",0,			
			(ULONG *)&Type,(UCHAR *)&AUTOCONNECT,(ULONG *)&Vallen);

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

	Stream=FindFreeStream();
	
	if (Stream == 255)
	{
		MessageBox(NULL,"No free streams available",NULL,MB_OK);
		return (FALSE);
	}

	//
	//	Register message for posting by BPQDLL
	//

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	//
	//	Enable Async notification
	//
	
	BPQSetHandle(Stream, hWnd);

	GetVersionInfo(NULL);

	wsprintf(Title,"BPQTerminal Version %s - using stream %d", VersionString, Stream);

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

//	if (applmask & 0x01)	CheckMenuItem(hMenu,BPQAPPL1,MF_CHECKED);
	if (applmask & 0x02)	CheckMenuItem(hMenu,BPQCHAT,MF_CHECKED);
//	if (applmask & 0x04)	CheckMenuItem(hMenu,BPQAPPL3,MF_CHECKED);
//	if (applmask & 0x08)	CheckMenuItem(hMenu,BPQAPPL4,MF_CHECKED);
//	if (applmask & 0x10)	CheckMenuItem(hMenu,BPQAPPL5,MF_CHECKED);
//	if (applmask & 0x20)	CheckMenuItem(hMenu,BPQAPPL6,MF_CHECKED);
//	if (applmask & 0x40)	CheckMenuItem(hMenu,BPQAPPL7,MF_CHECKED);
//	if (applmask & 0x80)	CheckMenuItem(hMenu,BPQAPPL8,MF_CHECKED);

// CMD_TO_APPL	EQU	1B		; PASS COMMAND TO APPLICATION
// MSG_TO_USER	EQU	10B		; SEND 'CONNECTED' TO USER
// MSG_TO_APPL	EQU	100B		; SEND 'CONECTED' TO APPL
//		0x40 = Send Keepalives

//	if (applflags & 0x01)	CheckMenuItem(hMenu,BPQFLAGS1,MF_CHECKED);
//	if (applflags & 0x02)	CheckMenuItem(hMenu,BPQFLAGS2,MF_CHECKED);
//	if (applflags & 0x04)	CheckMenuItem(hMenu,BPQFLAGS3,MF_CHECKED);
//	if (applflags & 0x40)	CheckMenuItem(hMenu,BPQFLAGS4,MF_CHECKED);


	hPopMenu1=GetSubMenu(hMenu,1);

	for (n=1;n <= GetNumberofPorts();n++)
	{
		i = GetPortNumber(n);

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
	
	if (AUTOCONNECT)
		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_UNCHECKED);

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

	SetAppl(Stream, applflags, applmask);

	SetTraceOptions(portmask,mtxparam,mcomparam);
	


	DragCursor = LoadCursor(hInstance, "IDC_DragSize");
	Cursor = LoadCursor(NULL, IDC_ARROW);

	GetCallsign(Stream, callsign);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON
	
		wsprintf(Title,"BPQTerminal Stream %d",Stream);
		AddTrayMenuItem(hWnd, Title);
	}

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (MinimizetoTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);

	UpdateWindow(hWnd);

	CheckTimer();

	TimerHandle = SetTimer(NULL, 0, 10000, lpTimerFunc);

	return (TRUE);
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


	if (message == BPQMsg)
	{
		if (lParam & BPQDataAvail)
			DoReceivedData(hWnd);
				
		if (lParam & BPQMonitorAvail)
			DoMonData(hWnd);
				
		if (lParam & BPQStateChange)
			DoStateChange(hWnd);

		return (0);
	}
	
	switch (message) { 

        case WM_MEASUREITEM: 
 
            lpmis = (LPMEASUREITEMSTRUCT) lParam; 
 
            // Set the height of the list box items. 
 
  //          lpmis->itemHeight = 15; 
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

			//fActive = LOWORD(wParam);           // activation flag 
			//fMinimized = (BOOL) HIWORD(wParam); // minimized flag 
			//	hwnd = (HWND) lParam;               // window handle 
 
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
		
			SessionControl(Stream, 1, 0);
			break;
			        
		case BPQDISCONNECT:
			
			SessionControl(Stream, 2, 0);
			break;
			
		case BPQAUTOCONNECT:

			ToggleAutoConnect(hWnd);
			break;
			
		case BPQAPPL1:

			ToggleAppl(hWnd,BPQAPPL1,0x1);
			break;

		case BPQAPPL2:

			ToggleAppl(hWnd,BPQAPPL2,0x2);
			break;

		case BPQAPPL3:

			ToggleAppl(hWnd,BPQAPPL3,0x4);
			break;

		case BPQAPPL4:

			ToggleAppl(hWnd,BPQAPPL4,0x8);
			break;

		case BPQAPPL5:

			ToggleAppl(hWnd,BPQAPPL5,0x10);
			break;

		case BPQAPPL6:

			ToggleAppl(hWnd,BPQAPPL6,0x20);
			break;

		case BPQAPPL7:

			ToggleAppl(hWnd,BPQAPPL7,0x40);
			break;

		case BPQAPPL8:

			ToggleAppl(hWnd,BPQAPPL8,0x80);
			break;

		case BPQFLAGS1:

			ToggleFlags(hWnd,BPQFLAGS1,0x01);
			break;

		case BPQFLAGS2:

			ToggleFlags(hWnd,BPQFLAGS2,0x02);
			break;

		case BPQFLAGS3:

			ToggleFlags(hWnd,BPQFLAGS3,0x04);
			break;

		case BPQFLAGS4:

			ToggleFlags(hWnd,BPQFLAGS4,0x40);
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

			
		case BPQSendDisconnected:

			ToggleParam(hWnd, &SendDisconnected, BPQSendDisconnected);
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

			HtmlHelp(hWnd,"BPQTerminal.chm",HH_HELP_FINDER,0);  
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
         
			SessionControl(Stream, 2, 0);
			DeallocateStream(Stream);
			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);
			
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

			//
			//	if not connected, and autoconnect is
			//	enabled, connect now
			
			if (!CONNECTED && AUTOCONNECT)
				SessionControl(Stream, 1, 0);
			
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

			SendMsg(Stream, &kbbuf[0], kbptr+1);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
		if (wParam == 0x1a)  // Ctrl/Z
		{
			//
			//	if not connected, and autoconnect is
			//	enabled, connect now
			
			if (!CONNECTED && AUTOCONNECT)
				SessionControl(Stream, 1, 0);
	
			kbbuf[0]=0x1a;
			kbbuf[1]=13;

			SendMsg(Stream, &kbbuf[0], 2);

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




DoStateChange(HWND hWnd)
{
	int port, sesstype, paclen, maxframe, l4window;

	//	Get current Session State. Any state changed is ACK'ed
	//	automatically. See BPQHOST functions 4 and 5.
	
	SessionState(Stream, &state, &change);
		
	if (change == 1)
	{
		if (state == 1)
		{
			// Connected
			
			CONNECTED = TRUE;
			SlowTimer = 0;

	//		GetCallsign(Stream, callsign);
			
			GetConnectionInfo(Stream, callsign,
										 &port, &sesstype, &paclen,
										 &maxframe, &l4window);

				
			wsprintf(Title,"BPQTerminal Version %s - using stream %d - Connected to %s",VersionString,Stream,callsign);
			SetWindowText(hWnd,Title);
			DisableConnectMenu(hWnd);
			EnableDisconnectMenu(hWnd);

		}
		else
		{	
			CONNECTED=FALSE;
			wsprintf(Title,"BPQTerminal Version %s - using stream %d - Disconnected",VersionString,Stream);
			SetWindowText(hWnd,Title);
			DisableDisconnectMenu(hWnd);
			EnableConnectMenu(hWnd);

			if (SendDisconnected)
			{
				int index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) "*** Disconnected");		
				SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));
			}
		}
	}

	return (0);

}
		
DoReceivedData(HWND hWnd)
{
	char Msg[300];

	if (RXCount(Stream) > 0)
	{
		do {

			GetMsg(Stream, &Msg[0],&len,&count);
		
			WritetoOutputWindow(Msg, len);

		} while (count > 0);
	}
	return (0);
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
	char * ptr1, * ptr2;
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

int ToggleAutoConnect(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	
	AUTOCONNECT = !AUTOCONNECT;
	
	if (AUTOCONNECT)

		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQAUTOCONNECT,MF_UNCHECKED);

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

	SetAppl(Stream,applflags,applmask);

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

	SetAppl(Stream,applflags,applmask);

	SetTraceOptions(portmask,mtxparam,mcomparam);

    return (0);
  
}
int ToggleMTX(HWND hWnd)
{
	mtxparam = mtxparam ^ 1;
	
	if (mtxparam & 1)

		CheckMenuItem(hMenu,BPQMTX,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQMTX,MF_UNCHECKED);

	SetTraceOptions(portmask,mtxparam,mcomparam);

    return (0);
  
}
int ToggleMCOM(HWND hWnd)
{
	mcomparam = mcomparam ^ 1;
	
	if (mcomparam & 1)

		CheckMenuItem(hMenu,BPQMCOM,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQMCOM,MF_UNCHECKED);

	SetTraceOptions(portmask,mtxparam,mcomparam);

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

	SetAppl(Stream,applflags,applmask);

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
	UCHAR * BPQDirectory=GetBPQDirectory();
	UCHAR FN[MAX_PATH];

	if (BPQDirectory[0] == 0)
		wsprintf(FN,"BPQTerm_%d.log", Stream);
	else
		wsprintf(FN,"%s\\BPQTerm_%d.log", BPQDirectory, Stream);

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