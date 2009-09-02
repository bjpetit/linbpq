// Mail and Chat Server for BPQ32 Packet Switch
//
//

// Version 1.0.0.17

//	Split Messasge, User and BBS Editing from Main Config.
//	Add word wrap to Console input and output
//  Flash Console on chat user connect
//	Fix procesing Name response in chat mode
//	Fix processing of *RTL from station not defined as a Chat Node
//	Fix overlength lines ln List responses
//  Housekeeping expires BIDs 
//  Killing a message removes it from the forwarding counts

// Version 1.0.0.18

// Save User Database when name is entered or updated so it is not lost on a crash
// Fix Protocol Error in Compressed Forwarding when switching direction
// Add Housekeeping results dialog.

// Version 1.0.0.19

// Allow PACLEN in forward scripts.
// Store and forward messages with CRLF as line ends
// Send Disconnect after FQ ( for LinFBB)
// "Last Listed" is saved if MailChat is closed without closing Console
// Maximum acceptable message length can be specified (in Forwarding Config)

// Version 1.0.0.20

// Fix error in saving forwarding config (introduced in .19)
// Limit size of FBB forwarding block.
// Clear old connection (instead of new) if duplicate connect on Chat Node-Node link
// Send FA for Compressed Mail (was sending FB for both Compressed and Uncompressed)

// Version 1.0.0.21

// Fix Connect Script Processing (wasn't waiting for CONNECTED from last step)
// Implement Defer
// Fix MBL-style forwarding
// Fix Add User (Params were not saved)
// Add SC (Send Copy) Command
// Accept call@bbs as well as call @ bbs

// Version 1.0.0.22

// Implement RB RP LN LR LF LN L$ Commands.
// Implement QTH and ZIP Commands.
// Entering an empty Title cancels the message.
// Uses HomeBBS field to set @ field for local users.
// Creates basic WP Database.
// Uses WP to lookup @ field for non-local calls.
// Console "Actions" Menu renamed "Options".
// Excluded flag is actioned.
// Asks user to set HomeBBS if not already set.
// Fix "Shrinking Message" problem, where message got shorter each time it was read Initroduced in .19).
// Flash Server window when anyone connects to chat (If Console Option "Flash on Chat User Connect" set).

// Version 1.0.0.23

// Fix R: line scan bug

// Version 1.0.0.24

// Fix closing console window on 'B'.
// Fix Message Creation time.
// Enable Delete function in WP edit dialog

// Version 1.0.0.25

// Implement K< and K> commands
// Experimental support for B1 and B2 forwarding
// Experimental UI System
// Fix extracting QTH from WP updates

// Version 1.0.0.26

// Add YN etc responses for FBB B1/B2

// Version 1.0.0.27

// Fix crash if NULL received as start of a packet.
// Add Save WP command
// Make B2 flag BBS-specific.
// Implement B2 Send

// Version 1.0.0.28

// Fix parsing of smtp to addresses - eg smtp:john.wiseman@cantab.net
// Flag messages as Held if smtp server rejects from or to addresses
// Fix Kill to (K> Call)
// Edit Message dialog shows latest first
// Add chat debug window to try to track down occasional chat connection problems

// Version 1.0.0.29

// Add loads of try/excspt

// Version 1.0.0.30

// Writes Debug output to LOG_DEBUG and Monitor Window

// Version 1.0.0.32

// Allow use of GoogleMail for ISP functions
// Accept SYSOP as alias for SYSOPCall - ie user can do SP SYSOP, and it will appear in sysop's LM, RM, etc
// Email Housekeeping Results to SYSOP

// Version 1.0.0.33

// Housekeeping now runs at Maintenance Time. Maintenance Interval removed. 
// Allow multiple numbers on R and K commands
// Fix L command with single number
// Log if Forward count is out of step with messages to forward.
// UI Processing improved and F< command implemented

// Version 1.0.0.34

// Semaphore Chat Messages
// Display Semaphore Clashes
// More Program Error Traps
// Kill Messages more than BIDLifetime old

// Version 1.0.0.35

// Test for Mike - Remove B1 check from Parse_SID

// Version 1.0.0.36

// Fix calculation of Housekeeping Time.
// Set dialog box background explicitly.
// Remove tray entry for chat debug window.
// Add date to log file name.
// Add Actions Menu option to disable logging.
// Fix size of main window when it changes between versions.

// Version 1.0.0.37

// Implement Paging.
// Fix L< command (was giving no messages).
// Implement LR LR mmm-nnn LR nnn- (and L nnn-)
// KM should no longer kill SYSOP bulls.
// ISP interfaces allows SMTP Auth to be configured
// SMTP Client would fail to send any more messages if a connection failed

// Version 1.0.0.38

// Don't include killed messages in L commands (except LK!)
// Implement l@
// Add forwarding timebands
// Allow resizing of main window.
// Add Ver command.

// Version 1.0.1.1

// First Public Beta

// Fix part line handling in Console
// Maintenance deletes old log files.
// Add option to delete files to the recycle bin.

// Version 1.0.2.1

// Allow all Node SYSOP commands in connect scripts.
// Implement FBB B1 Protocol with Resume
// Make FBB Max Block size settable for each BBS.
// Add extra logging when Chat Sessions refused.
// Fix Crash on invalid housekeeping override.
// Add Hold Messages option.
// Trap CRT Errors
// Sort Actions/Start Forwarding List

// Version 1.2.2.2

// Fill in gaps in BBS Number sequence
// Fix PE if ctext contains }
// Run Houskeeping at startup if previous Housekeeping was missed

// Version 1.2.2.

// Add configued nodes to /p listing

// Rewrite forwarding by HA.


#include "stdafx.h"

#define SPECIALVERSION "Beta"

#include "GetVersion.h"

#define MAX_LOADSTRING 100

INT_PTR CALLBACK UserEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK MsgEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK FwdEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK WPEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

UCHAR * (FAR WINAPI * GetVersionStringptr)();

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

HWND MainWnd;
HWND hWndSess;
RECT MainRect;
HMENU hActionMenu;
static HMENU hMenu;
HMENU hDisMenu;									// Disconnect Menu Handle
HMENU hFWDMenu;									// Forward Menu Handle

int SessX, SessY, SessWidth;					// Params for Session Window

char szBuff[80];

#define MaxSockets 64

ConnectionInfo Connections[MaxSockets+1];


struct SEM ChatSemaphore = {0, 0};

struct SEM AllocSemaphore = {0, 0};

struct UserInfo ** UserRecPtr=NULL;
int NumberofUsers=0;

struct UserInfo * BBSChain = NULL;						// Chain of users that are BBSes

struct MsgInfo ** MsgHddrPtr=NULL;
int NumberofMessages=0;

int FirstMessagetoForward=0;					// Lowest Message wirh a forward bit set - limits search

BIDRec ** BIDRecPtr=NULL;
int NumberofBIDs=0;

BIDRec ** TempBIDRecPtr=NULL;
int NumberofTempBIDs=0;

WPRec ** WPRecPtr=NULL;
int NumberofWPrecs=0;

int LatestMsg = 0;
struct SEM MsgNoSemaphore = {0, 0};			// For locking updates to LatestMsg
int HighestBBSNumber = 0;

int MaxMsgno = 60000;
int BidLifetime = 60;
int MaintInterval = 24;
int MaintTime = 0;

BOOL cfgMinToTray;

BOOL DisconnectOnClose=FALSE;


char PasswordMsg[100]="Password:";

char cfgHOSTPROMPT[100];

char cfgCTEXT[100];

char cfgLOCALECHO[100];

char AttemptsMsg[] = "Too many attempts - Disconnected\r\r";
char disMsg[] = "Disconnected by SYSOP\r\r";

char LoginMsg[]="user:";

char BlankCall[]="         ";


UCHAR BBSApplMask;
UCHAR ChatApplMask;

int BBSApplNum=0;
int ChatApplNum=0;

//int	StartStream=0;
int	NumberofStreams=0;
int MaxStreams=0;

char BBSSID[]="[BPQ-%d.%d.%d.%d-%s%s%sFH$]\r";
//char BBSSID[]="[BPQ-1.00-AB1FHMRX$]\r";

char ChatSID[]="[BPQChatServer-%d.%d.%d.%d]\r";

char NewUserPrompt[100]="Please enter your Name: ";

char BBSName[100];

char HRoute[100];

char SignoffMsg[100];

char AbortedMsg[100]="\rOutput aborted\r";

char UserDatabaseName[MAX_PATH] = "BPQBBSUsers.dat";
char UserDatabasePath[MAX_PATH];

char MsgDatabasePath[MAX_PATH];
char MsgDatabaseName[MAX_PATH] = "DIRMES.SYS";

char BIDDatabasePath[MAX_PATH];
char BIDDatabaseName[MAX_PATH] = "WFBID.SYS";

char WPDatabasePath[MAX_PATH];
char WPDatabaseName[MAX_PATH] = "WP.SYS";

char BaseDir[MAX_PATH];

char MailDir[MAX_PATH];


BOOL ALLOWCOMPRESSED = TRUE;

BOOL EnableUI = FALSE;

UCHAR * OtherNodes=NULL;

char zeros[NBMASK];						// For forward bitmask tests

time_t MaintClock;						// Time to run housekeeping


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
ATOM				RegisterMainWindowClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
	Logprintf(LOG_DEBUG, '!', "*** Error **** C Run Time Invalid Parameter Handler Called");

	if (expression && function && file)
	{
		Logprintf(LOG_DEBUG, '!', "Expression = %S", expression);
		Logprintf(LOG_DEBUG, '!', "Function %S", function);
		Logprintf(LOG_DEBUG, '!', "File %S Line %d", file, line);
	}
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	int BPQStream, n;
	struct UserInfo * user;
	struct _EXCEPTION_POINTERS exinfo;

	// Trap CRT Errors
	
	_invalid_parameter_handler oldHandler, newHandler;
   
	newHandler = myInvalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_BPQMailChat, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BPQMailChat));

	// Main message loop:

	__try
	{	
		while (GetMessage(&msg, NULL, 0, 0))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	My__except_Routine("GetMessageLoop");

	__try
	{
	for (n = 0; n < NumberofStreams; n++)
	{
		BPQStream=Connections[n].BPQStream;
		
		if (BPQStream)
		{
			SetAppl(BPQStream, 0, 0);
			Disconnect(BPQStream);
			DeallocateStream(BPQStream);
		}
	}

	Sleep(2000);		// A bit of time for links to close

	if (hConsole)
		DestroyWindow(hConsole);

	SaveUserDatabase();
	SaveMessageDatabase();
	SaveBIDDatabase();

	if (cfgMinToTray)
	{
		DeleteTrayMenuItem(MainWnd);
		if (hConsole)
			DeleteTrayMenuItem(hConsole);
		if (hMonitor)
			DeleteTrayMenuItem(hMonitor);
		if (hDebug)
			DeleteTrayMenuItem(hDebug);
	}

	// Free all allocated memory

	for (n = 0; n <= NumberofUsers; n++)
	{
		user = UserRecPtr[n];

		if (user->ForwardingInfo)
		{
			FreeForwardingStruct(user);
			free(user->ForwardingInfo); 
		}

		free(user);
	}
	
	free(UserRecPtr);

	for (n = 0; n <= NumberofMessages; n++)
		free(MsgHddrPtr[n]);

	free(MsgHddrPtr);

	for (n = 0; n <= NumberofWPrecs; n++)
		free(WPRecPtr[n]);

	free(WPRecPtr);

	for (n = 0; n <= NumberofBIDs; n++)
		free(BIDRecPtr[n]);

	free(BIDRecPtr);

	if (TempBIDRecPtr)
		free(TempBIDRecPtr);

	free(OtherNodes);

	FreeChatMemory();

	FreeOverrides();

	Free_UI();

	_CrtDumpMemoryLeaks();

	SaveWindowConfig();

	}
	My__except_Routine("Close Processing");

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
//
#define BGCOLOUR RGB(236,233,216)
//#define BGCOLOUR RGB(245,245,245)

HBRUSH bgBrush;

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= DLGWINDOWEXTRA;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BPQMailChat));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= bgBrush;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BPQMailChat);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
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
	char Title[80];
	WSADATA WsaData;
	HMENU hTopMenu;		// handle of menu 
	HKEY hKey=0;
	int retCode,Type,Vallen;
	char Size[80];
	RECT InitRect;
	RECT SessRect;

	HMODULE ExtDriver=LoadLibrary("bpq32.dll");

	GetVersionStringptr = (UCHAR *(__stdcall *)())GetProcAddress(ExtDriver,"_GetVersionString@0");


	// Get Window Size  From Registry


	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;
		RegQueryValueEx(hKey,"WindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&MainRect.left,&MainRect.right,&MainRect.top,&MainRect.bottom);
		RegCloseKey(hKey);
	}

	hInst = hInstance;

	hWnd=CreateDialog(hInst,szWindowClass,0,NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	MainWnd=hWnd;

	if (MainRect.right < 100 || MainRect.bottom < 100)
	{
		GetWindowRect(hWnd,	&MainRect);
	}

	// Keep the size in case changed from previous version

	GetWindowRect(hWnd,	&InitRect);

	MoveWindow(hWnd,MainRect.left,MainRect.top, InitRect.right-InitRect.left, InitRect.bottom-InitRect.top, TRUE);

	GetVersionInfo(NULL);

	wsprintf(Title,"G8BPQ Mail and Chat Server Beta Version %s", VersionString);

	SetWindowText(hWnd,Title);

	hWndSess = GetDlgItem(hWnd, 100); 

	GetWindowRect(hWnd,	&InitRect);
	GetWindowRect(hWndSess, &SessRect);

	SessX = SessRect.left - InitRect.left ;
	SessY = SessRect.top -InitRect.top;
	SessWidth = SessRect.right - SessRect.left;

   	// Get handles fou updating menu items

	hTopMenu=GetMenu(MainWnd);
	hActionMenu=GetSubMenu(hTopMenu,0);

	hFWDMenu=GetSubMenu(hActionMenu,0);
	hMenu=GetSubMenu(hActionMenu,1);
	hDisMenu=GetSubMenu(hActionMenu,2);

   CheckTimer();

 	cfgMinToTray = GetMinimizetoTrayFlag();

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (cfgMinToTray)
		{
			ShowWindow(hWnd, SW_HIDE);
		}
		else
		{
			ShowWindow(hWnd, nCmdShow);
		}
	else
		ShowWindow(hWnd, nCmdShow);

   UpdateWindow(hWnd);

   WSAStartup(MAKEWORD(2, 0), &WsaData);
    
   return Initialise();
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int state,change;
	ConnectionInfo * conn;
	struct _EXCEPTION_POINTERS exinfo;


	if (message == BPQMsg)
	{
		if (lParam & BPQMonitorAvail)
		{
			__try
			{
				DoMonitorData(wParam);
			}
			My__except_Routine("DoMonitorData");

			return 0;
			
		}
		if (lParam & BPQDataAvail)
		{
			__try
			{
				DoReceivedData(wParam);
			}
			My__except_Routine("DoReceivedData")
			return 0;
		}
		if (lParam & BPQStateChange)
		{
			//	Get current Session State. Any state changed is ACK'ed
			//	automatically. See BPQHOST functions 4 and 5.
	
			__try
			{
				SessionState(wParam, &state, &change);
		
				if (change == 1)
				{
					if (state == 1)
					{

						// Connected
					
						__try
						{
							Connected(wParam);	
						}
						My__except_Routine("Connected");
					}
					else
					{
						__try
						{
							Disconnected(wParam);
						}
						My__except_Routine("Disconnected");
					}
				}
			}
			My__except_Routine("DoStateChange");

		}

		return 0;
	}


	switch (message)
	{

	case WSA_DATA: // Notification on data socket

		Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WSA_ACCEPT: /* Notification if a socket connection is pending. */

		Socket_Accept(wParam);
		return 0;

	case WSA_CONNECT: /* Notification if a socket connection completed. */

		Socket_Connect(wParam, WSAGETSELECTERROR(lParam));
		return 0;


			
	case WM_TIMER:

		if (wParam == 1)		// Slow = 10 secs
		{
			__try
			{
				RefreshMainWindow();
				CheckTimer();
				TCPTimer();
				FWDTimerProc();
				ChatTimer();
				if (MaintClock < time(NULL))
				{				
					DoHouseKeeping(FALSE);
					MaintClock += 86400;					
				}
			}
			My__except_Routine("Slow Timer");
		}
		else
			__try
			{
				TrytoSend();
			}
			My__except_Routine("TrytoSend");
		
		return (0);

	
	case WM_CTLCOLORDLG:
        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }

	case WM_INITMENUPOPUP:

		if (wParam == (WPARAM)hFWDMenu)
		{
			// Set up Forward Menu

			struct UserInfo * user;
			char MenuLine[30];

			for (user = BBSChain; user; user = user->BBSNext)
			{
				wsprintf(MenuLine, "%s %d Msgs", user->Call, user->ForwardingInfo->MsgCount);

				if (ModifyMenu(hFWDMenu, IDM_FORWARD_ALL + user->BBSNumber, 
					MF_BYCOMMAND | MF_STRING, IDM_FORWARD_ALL + user->BBSNumber, MenuLine) == 0)
	
				AppendMenu(hFWDMenu, MF_STRING,IDM_FORWARD_ALL + user->BBSNumber, MenuLine);
			}
			return TRUE;
		}

		if (wParam == (WPARAM)hDisMenu)
		{
			// Set up Forward Menu

			CIRCUIT * conn;
			char MenuLine[30];
			int n;

			for (n = 0; n <= NumberofStreams-1; n++)
			{
				conn=&Connections[n];

				RemoveMenu(hDisMenu, IDM_DISCONNECT + n, MF_BYCOMMAND);

				if (conn->Active)
				{
					sprintf_s(MenuLine, 30, "%d %s", conn->BPQStream, conn->Callsign);
					AppendMenu(hDisMenu, MF_STRING, IDM_DISCONNECT + n, MenuLine);
				}
			}
			return TRUE;
		}
		break;


	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		if (wmId >= IDM_DISCONNECT && wmId < IDM_DISCONNECT+MaxSockets+1)
		{
			// disconnect user

			conn=&Connections[wmId-IDM_DISCONNECT];
		
			if (conn->Active)
			{	
				Disconnect(conn->BPQStream);
			}
		}

		if (wmId >= IDM_FORWARD_ALL && wmId < IDM_FORWARD_ALL + 100)
		{
			StartForwarding(wmId - IDM_FORWARD_ALL);
			return 0;
		}

		switch (wmId)
		{

		case IDM_LOGBBS:

			ToggleParam(hMenu, hWnd, &LogBBS, IDM_LOGBBS);
			break;

		case IDM_LOGCHAT:

			ToggleParam(hMenu, hWnd, &LogCHAT, IDM_LOGCHAT);
			break;

		case IDM_LOGTCP:

			ToggleParam(hMenu, hWnd, &LogTCP, IDM_LOGTCP);
			break;

		case IDM_HOUSEKEEPING:

			DoHouseKeeping(TRUE);
			break;

		case IDM_CONSOLE:

			CreateConsole();
			break;

		case IDM_MONITOR:

			CreateMonitor();
			break;

		case IDM_DEBUG:

			CreateDebugWindow();
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_CONFIG:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hWnd, ConfigWndProc);
			break;

		case IDM_USERS:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_USEREDIT), hWnd, UserEditDialogProc);
			break;

		case IDM_FWD:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_FORWARDING), hWnd, FwdEditDialogProc);
			break;

		case IDM_MESSAGES:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_MSGEDIT), hWnd, MsgEditDialogProc);
			break;

		case IDM_WP:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_EDITWP), hWnd, WPEditDialogProc);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

    case WM_SIZE:

		if (wParam == SIZE_MINIMIZED)
			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);

		return (0);

	
	case WM_SIZING:
	{
		LPRECT lprc = (LPRECT) lParam;
		int Height = lprc->bottom-lprc->top;
		int Width = lprc->right-lprc->left;

		MoveWindow(hWndSess, 0, 30, SessWidth, Height - 100, TRUE);
			
		return TRUE;
	}


	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:

		GetWindowRect(MainWnd,	&MainRect);	// For save soutine
	
		if (hConsole)
			GetWindowRect(hConsole,	&ConsoleRect);	// For save soutine
		if (hMonitor)
			GetWindowRect(hMonitor,	&MonitorRect);	// For save soutine

		KillTimer(hWnd,1);
		KillTimer(hWnd,2);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
			return (INT_PTR)TRUE;

		break;
	}
	return (INT_PTR)FALSE;
}

int RefreshMainWindow()
{
	char msg[80];
	CIRCUIT * conn;
	int i,n, SYSOPMsgs = 0, HeldMsgs = 0, SMTPMsgs = 0;
	time_t now;
	struct tm * tm;
	char tim[20];

	SendDlgItemMessage(MainWnd,100,LB_RESETCONTENT,0,0);

	for (n = 0; n < NumberofStreams; n++)
	{
		conn=&Connections[n];

		if (!conn->Active)
		{
			strcpy(msg,"Idle");
		}
		else
		{
			if (conn->Flags & CHATLINK)
			{
				i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
					"Chat Link", conn->u.link->alias, conn->BPQStream,
					"", conn->OutputQueueLength - conn->OutputGetPointer);
			}
			else
			if ((conn->Flags & CHATMODE)  && conn->topic)
			{
				i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
					conn->UserPointer->Name, conn->u.user->call, conn->BPQStream,
					conn->topic->topic->name, conn->OutputQueueLength - conn->OutputGetPointer);
			}
			else
			{
				if (conn->UserPointer == 0)
					strcpy(msg,"Logging in");
				else
				{
					i=sprintf_s(msg, sizeof(msg), "%-10s %-10s %2d %-10s%5d",
						conn->UserPointer->Name, conn->UserPointer->Call, conn->BPQStream,
						"BBS", conn->OutputQueueLength - conn->OutputGetPointer);
				}
			}
		}
		SendDlgItemMessage(MainWnd,100,LB_ADDSTRING,0,(LPARAM)msg);
	}

	SetDlgItemInt(hWnd, IDC_MSGS, NumberofMessages, FALSE);

	n = 0;

	for (i=1; i <= NumberofMessages; i++)
	{
		if (MsgHddrPtr[i]->status == 'N')
		{
			if (_stricmp(MsgHddrPtr[i]->to, SYSOPCall) == 0  || _stricmp(MsgHddrPtr[i]->to, "SYSOP") == 0)
				SYSOPMsgs++;
			else
			if (MsgHddrPtr[i]->to[0] == 0)
				SMTPMsgs++;
		}
		else
		{
			if (MsgHddrPtr[i]->status == 'H')
				HeldMsgs++;
		}
	}

	SetDlgItemInt(hWnd, IDC_SYSOPMSGS, SYSOPMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_HELD, HeldMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_SMTP, SMTPMsgs, FALSE);

	SetDlgItemInt(hWnd, IDC_CHATSEM, ChatSemaphore.Clashes, FALSE);
	SetDlgItemInt(hWnd, IDC_MSGSEM, MsgNoSemaphore.Clashes, FALSE);
	SetDlgItemInt(hWnd, IDC_ALLOCSEM, AllocSemaphore.Clashes, FALSE);

	now = time(NULL);

	tm = gmtime(&now);	
	sprintf_s(tim, sizeof(tim), "%02d:%02d", tm->tm_hour, tm->tm_min);
	SetDlgItemText(hWnd, IDC_UTC, tim);

	tm = localtime(&now);
	sprintf_s(tim, sizeof(tim), "%02d:%02d", tm->tm_hour, tm->tm_min);
	SetDlgItemText(hWnd, IDC_LOCAL, tim);


	return 0;
}

#define MAX_PENDING_CONNECTS 4

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

SOCKADDR_IN local_sin;  /* Local socket - internet style */

PSOCKADDR_IN psin;

SOCKET sock;

BOOL Initialise()
{
	int i, ptr, len;
	ConnectionInfo * conn;
	struct UserInfo * user = NULL;
	HKEY hKey=0;
	char * ptr1;
	int Attrs, ret;
	char msg[MAX_PATH + 50];

	//	Register message for posting by BPQDLL

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	if (!GetConfigFromRegistry())
		return FALSE;

	wsprintf(SignoffMsg, "73 de %s\r", BBSName);

	// Make Sure BASEDIR Exists

	Attrs = GetFileAttributes(BaseDir);

	if (Attrs == -1)
	{
		sprintf_s(msg, sizeof(msg), "Base Directory %s not found - should it be created?", BaseDir);
		ret = MessageBox(NULL, msg, "BPQMailChat", MB_YESNO);

		if (ret == IDYES)
		{
			ret = CreateDirectory(BaseDir, NULL);
			if (ret == 0)
			{
				MessageBox(NULL, "Failed to created Base Directory - exiting", "BPQMailChat", MB_ICONSTOP);
				return FALSE;
			}
		}
		else
		{
			MessageBox(NULL, "Can't Continue without a Base Directory - exiting", "BPQMailChat", MB_ICONSTOP);
			return FALSE;
		}
	}
	else
	{
		if (!(Attrs & FILE_ATTRIBUTE_DIRECTORY))
		{
			sprintf_s(msg, sizeof(msg), "Base Directory %s is a file not a directory - exiting", BaseDir);
			ret = MessageBox(NULL, msg, "BPQMailChat", MB_ICONSTOP);

			return FALSE;
		}
	}

	ret = CreateDirectory("c:\\MAIL", NULL);
	ret = CreateDirectory("c:\\MAIL\\MAIL0", NULL);

	
	// Set up file and directory names
		
	strcpy(UserDatabasePath, BaseDir);
	strcat(UserDatabasePath, "\\");
	strcat(UserDatabasePath, UserDatabaseName);

	strcpy(MsgDatabasePath, BaseDir);
	strcat(MsgDatabasePath, "\\");
	strcat(MsgDatabasePath, MsgDatabaseName);

	strcpy(BIDDatabasePath, BaseDir);
	strcat(BIDDatabasePath, "\\");
	strcat(BIDDatabasePath, BIDDatabaseName);

	strcpy(WPDatabasePath, BaseDir);
	strcat(WPDatabasePath, "\\");
	strcat(WPDatabasePath, WPDatabaseName);

	strcpy(MailDir, BaseDir);
	strcat(MailDir, "\\");
	strcat(MailDir, "Mail");

	CreateDirectory(MailDir, NULL);		// Just in case

	strcpy(RtUsr, BaseDir);
	strcat(RtUsr, "\\ChatUsers.txt");

	strcpy(RtUsrTemp, BaseDir);
	strcat(RtUsrTemp, "\\ChatUsers.tmp");

	BBSApplMask = 1<<(BBSApplNum-1);
	ChatApplMask = 1<<(ChatApplNum-1);

	if (ChatApplNum)
	{
		ptr1=GetApplCall(ChatApplNum);
		memcpy(OurNode, ptr1, 10);
		strlop(OurNode, ' ');

		ptr1=GetApplAlias(ChatApplNum);
		memcpy(OurAlias, ptr1,10);
		strlop(OurAlias, ' ');

		// Set up other nodes list. rtlink messes with the string so pass copy
	
		ptr=0;

		while (OtherNodes[ptr])
		{
			len=strlen(&OtherNodes[ptr]);		
			rtlink(_strdup(&OtherNodes[ptr]));			
			ptr+= (len + 1);
		}
	}

	// Make backup copies of Databases
	
	CopyBIDDatabase();
	CopyMessageDatabase();
	CopyUserDatabase();
	CopyWPDatabase();

	GetMessageDatabase();
	GetUserDatabase();
	GetBIDDatabase();
	GetWPDatabase();

	// Allocate Streams

	for (i=0; i < MaxStreams; i++)
	{
		conn = &Connections[i];
		conn->BPQStream = FindFreeStream();

		if (conn->BPQStream == 255) break;

		NumberofStreams++;

		BPQSetHandle(conn->BPQStream, hWnd);

		SetAppl(conn->BPQStream, (i == 0 && EnableUI) ? 0x82 : 2, BBSApplMask | ChatApplMask);
		Disconnect(conn->BPQStream);
	}

	InitialiseTCP();

	if (EnableUI)
		SetupUIInterface();

	if (cfgMinToTray)
	{
		AddTrayMenuItem(MainWnd, "Mail/Chat Server");
	}

	SetupMyHA();
	
	SetTimer(hWnd,1,10000,NULL);	// Slow Timer (10 Secs)
	SetTimer(hWnd,2,100,NULL);		// Send to Node (100 ms)

	// Calulate time to run Housekeeping

	{
		struct tm *tm;
		time_t now;

		now = time(NULL);

		tm = gmtime(&now);

		tm->tm_hour = MaintTime / 100;
		tm->tm_min = MaintTime % 100;
		tm->tm_sec = 0;

		MaintClock = _mkgmtime(tm);

		if (MaintClock < now)
			MaintClock += 86400;

		if (LastFWDTime)
		{
			if ((MaintClock - LastFWDTime) > 86400)
				DoHouseKeeping(FALSE);
		}
	}

	CheckMenuItem(hMenu,IDM_LOGBBS, (LogBBS) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_LOGTCP, (LogTCP) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_LOGCHAT, (LogCHAT) ? MF_CHECKED : MF_UNCHECKED);

	RefreshMainWindow();

	return TRUE;
}

int Connected(Stream)
{
	int n, Mask;
	CIRCUIT * conn;
	struct UserInfo * user = NULL;
	char callsign[10];
	int port, sesstype, paclen, maxframe, l4window;
	char ConnectedMsg[] = "CONNECTED  ";
	char Msg[100];

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (Stream == conn->BPQStream)
		{
			if (conn->Active)
			{
				// Probably an outgoing connect

				if (conn->BBSFlags & RunningConnectScript)
				{
					// BBS Outgoing Connect

					conn->paclen = 128;

					// Run first line of connect script

					conn->UserPointer->ForwardingInfo->ScriptIndex = -1;  // Incremented before being used
					ProcessBBSConnectScript(conn, ConnectedMsg, 10);
					return 0;
				}
		
				if (conn->flags == p_linkini)
				{
					conn->paclen = 128;
					nprintf(conn, "c %s\r", conn->u.link->call);
					return 0;
				}
			}
	
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->Active = TRUE;
			conn->BPQStream = Stream;

			GetConnectionInfo(Stream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);

			strlop(callsign, ' ');		// Remove trailing spaces

			memcpy(conn->Callsign, callsign, 10);

			strlop(callsign, '-');		// Remove any SSID

			user = LookupCall(callsign);

			if (user == NULL)
			{
				user = AllocateUserRecord(callsign);
	
				if (user == NULL) return 0; //		Cant happen??

				user->flags |= F_HOLDMAIL;
			}

			time(&user->TimeLastCOnnected);

			conn->UserPointer = user;

			conn->lastmsg = user->lastmsg;

			conn->PageLen = user->PageLen;
			conn->Paging = (user->PageLen > 0);

			conn->NextMessagetoForward = FirstMessagetoForward;

			if (paclen == 0)
				paclen = 236;
			
			conn->paclen=paclen;

			//	Set SYSOP flag if user is defined as SYSOP and Host Session 
			
			if (((sesstype & 0x20) == 0x20) && (user->flags & F_SYSOP))
				conn->sysop = TRUE;

			Mask = 1 << (GetApplNum(Stream) - 1);

			if (user->flags & F_Excluded)
			{
				n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s Rejected by Exclude Flag", user->Call);
				WriteLogLine('|',Msg, n, LOG_CHAT);
				Disconnect(Stream);
				return 0;
			}

			n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s", user->Call);
			
			// Send SID and Prompt

			if (Mask == ChatApplMask)
			{
				WriteLogLine('|',Msg, n, LOG_CHAT);
				conn->Flags |= CHATMODE;

				nodeprintf(conn, ChatSID, Ver[0], Ver[1], Ver[2], Ver[3]);
			}
			else
			{
				BOOL B1 = FALSE, B2 = FALSE;

				if(conn->UserPointer->ForwardingInfo)
				{
					B1 = conn->UserPointer->ForwardingInfo->AllowB1;
					B2 = conn->UserPointer->ForwardingInfo->AllowB2;
				}
				WriteLogLine('|',Msg, n, LOG_BBS);
				nodeprintf(conn, BBSSID, Ver[0], Ver[1], Ver[2], Ver[3],
					ALLOWCOMPRESSED ? "B" : "", B1 ? "1" : "", B2 ? "2" : "");
			}

			if (user->Name[0] == 0)
			{
				conn->Flags |= GETTINGUSER;
				BBSputs(conn, NewUserPrompt);
			}
			else
				SendWelcomeMsg(Stream, conn, user);

			RefreshMainWindow();
			
			return 0;
		}
	}

	return 0;
}

int Disconnected (Stream)
{
	struct UserInfo * user = NULL;
	CIRCUIT * conn;
	int n;
	char Msg[255];
	int len;
	struct _EXCEPTION_POINTERS exinfo;

	for (n = 0; n <= NumberofStreams-1; n++)
	{
		conn=&Connections[n];

		if (Stream == conn->BPQStream)
		{
			if (conn->Active == FALSE)
				return 0;

			ClearQueue(conn);

			if (conn->InputMode == 'B')
			{
				// Save partly received message for a restart
						
				if (conn->BBSFlags & FBBB1Mode)
					SaveFBBBinary(conn);		
			}

			conn->Active = FALSE;
			RefreshMainWindow();

			if (conn->Flags & CHATMODE)
			{
				if (conn->Flags & CHATLINK)
				{
					len=sprintf_s(Msg, sizeof(Msg), "Chat Node %s Disconnected", conn->u.link->call);
					WriteLogLine('|',Msg, len, LOG_CHAT);
					__try {link_drop(conn);} My__except_Routine("link_drop");
				}
				else
				{
					len=sprintf_s(Msg, sizeof(Msg), "Chat User %s Disconnected", conn->Callsign);
					WriteLogLine('|',Msg, len, LOG_CHAT);
					__try {logout(conn);} My__except_Routine("logout");

				}

				return 0;
			}

			RemoveTempBIDS(conn);

			len=sprintf_s(Msg, sizeof(Msg), "%s Disconnected", conn->Callsign);
			WriteLogLine('|',Msg, len, LOG_BBS);

			if (conn->FBBHeaders)
			{
				free(conn->FBBHeaders);
				conn->FBBHeaders = NULL;
			}

			if (conn->UserPointer)
			{
				if (conn->UserPointer->ForwardingInfo)
				{
					conn->UserPointer->ForwardingInfo->Forwarding = FALSE;
					conn->UserPointer->ForwardingInfo->FwdTimer = 0;
				}
			}
			return 0;
		}
	}

	return 0;
}

int DoReceivedData(int Stream)
{
	int count, InputLen;
	UINT MsgLen;
	int n;
	CIRCUIT * conn;
	struct UserInfo * user;
	char * ptr, * ptr2;
	char Buffer[600];

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];

		if (Stream == conn->BPQStream)
		{
			do
			{ 
				// May have several messages per packet, or message split over packets

				if (conn->InputLen > 300)	// Shouldnt have lines longer  than this in text mode
				{
					conn->InputLen=0;
				}
				
				GetMsg(Stream, &conn->InputBuffer[conn->InputLen], &InputLen, &count);

				if (InputLen == 0) return 0;

				conn->InputLen += InputLen;

				if (conn->InputMode == 'B')
				{
					__try
					{
						UnpackFBBBinary(conn);
					}
					__except(EXCEPTION_EXECUTE_HANDLER)
					{
						conn->InputBuffer[conn->InputLen] = 0;
						Debugprintf("MAILCHAT *** Program Error in UnpackFBBBinary");
						Disconnect(conn->BPQStream);
						conn->InputLen=0;
						return 0;
					}
				}
				else
				{

			loop:
				ptr = memchr(conn->InputBuffer, '\r', conn->InputLen);

				if (ptr)	//  CR in buffer
				{
					user = conn->UserPointer;
				
					ptr2 = &conn->InputBuffer[conn->InputLen];
					
					if (++ptr == ptr2)
					{
						// Usual Case - single meg in buffer

						__try
						{
							if (conn->flags == p_linkini)		// Chat Connect
								ProcessConnecting(conn, conn->InputBuffer, conn->InputLen);
							else if (conn->BBSFlags & RunningConnectScript)
								ProcessBBSConnectScript(conn, conn->InputBuffer, conn->InputLen);
							else
								ProcessLine(conn, user, conn->InputBuffer, conn->InputLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							conn->InputBuffer[conn->InputLen] = 0;
							Debugprintf("MAILCHAT *** Program Error Processing input %s ", conn->InputBuffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							return 0;
						}
						conn->InputLen=0;
					}
					else
					{
						// buffer contains more that 1 message

						MsgLen = conn->InputLen - (ptr2-ptr);

						memcpy(Buffer, conn->InputBuffer, MsgLen);
						__try
						{
							if (conn->flags == p_linkini)
								ProcessConnecting(conn, Buffer, MsgLen);
							else if (conn->BBSFlags & RunningConnectScript)
								ProcessBBSConnectScript(conn, Buffer, MsgLen);
							else
								ProcessLine(conn, user, Buffer, MsgLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							Buffer[MsgLen] = 0;
							Debugprintf("MAILCHAT *** Program Error Processing input %s ", Buffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							return 0;
						}


						if (*ptr == 0 || *ptr == '\n')
						{
							/// CR LF or CR Null

							ptr++;
							conn->InputLen--;
						}

						memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

						conn->InputLen -= MsgLen;

						goto loop;

					}
				}
				}
			} while (count > 0);

			return 0;
		}
	}

	// Socket not found

	return 0;

}
int DoMonitorData(int Stream)
{
//	UCHAR Buffer[1000];
	UCHAR buff[500];

	int len,count=0;
	int stamp;

			do { 

			stamp=GetRaw(Stream, buff,&len,&count);

			if (len == 0) return 0;

			SeeifBBSUIFrame((struct _MESSAGE *)buff, len);
			
		}


		while (count > 0);	
 
	

	return 0;

}
int ConnectState(Stream)
{
	int state;

	SessionStateNoAck(Stream, &state);
	return state;
}
UCHAR * EncodeCall(UCHAR * Call)
{
	static char axcall[10];

	ConvToAX25(Call, axcall);
	return &axcall[0];

}


int WriteLog(char * msg)
{
	FILE *file;
	char timebuf[128];

	time_t ltime;
    struct tm today;
 
	if ((file = fopen("BPQTelnetServer.log","a")) == NULL)
		return FALSE;

	time(&ltime);

	_localtime32_s( &today, &ltime );

	strftime( timebuf, 128,
		"%d/%m/%Y %H:%M:%S ", &today );
    
	fputs(timebuf, file);

	fputs(msg, file);

	fclose(file);

	return 0;
}




struct UserInfo * AllocateUserRecord(char * Call)
{
	struct UserInfo * User = zalloc(sizeof (struct UserInfo));
		
	strcpy(User->Call, Call);

	GetSemaphore(&AllocSemaphore);

	UserRecPtr=realloc(UserRecPtr,(++NumberofUsers+1)*4);
	UserRecPtr[NumberofUsers]= User;

	FreeSemaphore(&AllocSemaphore);

	return User;
}

struct MsgInfo * AllocateMsgRecord()
{
	struct MsgInfo * Msg = zalloc(sizeof (struct MsgInfo));

	GetSemaphore(&AllocSemaphore);

	MsgHddrPtr=realloc(MsgHddrPtr,(++NumberofMessages+1)*4);
	MsgHddrPtr[NumberofMessages] = Msg;

	FreeSemaphore(&AllocSemaphore);

	return Msg;
}

BIDRec * AllocateBIDRecord()
{
	BIDRec * BID = zalloc(sizeof (BIDRec));
	
	GetSemaphore(&AllocSemaphore);

	BIDRecPtr=realloc(BIDRecPtr,(++NumberofBIDs+1)*4);
	BIDRecPtr[NumberofBIDs] = BID;

	FreeSemaphore(&AllocSemaphore);

	return BID;
}

BIDRec * AllocateTempBIDRecord()
{
	BIDRec * BID = zalloc(sizeof (BIDRec));
	
	GetSemaphore(&AllocSemaphore);

	TempBIDRecPtr=realloc(TempBIDRecPtr,(++NumberofTempBIDs+1)*4);
	TempBIDRecPtr[NumberofTempBIDs] = BID;

	FreeSemaphore(&AllocSemaphore);

	return BID;
}

struct UserInfo * LookupCall(char * Call)
{
	struct UserInfo * ptr = NULL;
	int i;

	for (i=1; i <= NumberofUsers; i++)
	{
		ptr = UserRecPtr[i];

		if (_stricmp(ptr->Call, Call) == 0) return ptr;

	}

	return NULL;
}

VOID GetUserDatabase()
{
	struct UserInfo UserRec;
	HANDLE Handle;
	int ReadLen;
	struct UserInfo * user;

	Handle = CreateFile(UserDatabasePath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
	{
		// Initialise a new File

		UserRecPtr=malloc(4);
		UserRecPtr[0]= malloc(sizeof (struct UserInfo));
		memset(UserRecPtr[0], 0, sizeof (struct UserInfo));
		NumberofUsers = 0;

		return;
	}


	// Get First Record
		
	ReadFile(Handle, &UserRec, sizeof (UserRec), &ReadLen, NULL); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&UserRec, 0, sizeof (struct UserInfo));
	}

	// Set up control record

	UserRecPtr=malloc(4);
	UserRecPtr[0]= malloc(sizeof (struct UserInfo));
	memcpy(UserRecPtr[0], &UserRec,  sizeof (UserRec));

	NumberofUsers = 0;

Next:

	ReadFile(Handle, &UserRec, sizeof (UserRec), &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		user = AllocateUserRecord(UserRec.Call);
		memcpy(user, &UserRec,  sizeof (UserRec));

		user->ForwardingInfo = NULL;	// In case left behind on crash
		user->BBSNext = NULL;
		user->POP3Locked = FALSE;


		if (user->flags & F_BBS)
		{
			// Defined as BBS - allocate and initialise forwarding structure

			SetupForwardingStruct(user);

			// Add to BBS Chain;

			user->BBSNext = BBSChain;
			BBSChain = user;

			// Save Highest BBS Number

			if (user->BBSNumber > HighestBBSNumber) HighestBBSNumber = user->BBSNumber;

		}
		goto Next;
	}

	SortBBSChain();


	CloseHandle(Handle);	
}

VOID CopyUserDatabase()
{
	char Backup[MAX_PATH];

	strcpy(Backup, UserDatabasePath);
	strcat(Backup, ".bak");

	CopyFile(UserDatabasePath, Backup, FALSE);
}

VOID SaveUserDatabase()
{
	HANDLE Handle;
	int WriteLen;
	int i;

	Handle = CreateFile(UserDatabasePath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	UserRecPtr[0]->nbcon = NumberofUsers;

	for (i=0; i <= NumberofUsers; i++)
	{
		WriteFile(Handle, UserRecPtr[i], sizeof (struct UserInfo), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

VOID GetMessageDatabase()
{
	struct MsgInfo MsgRec;
	HANDLE Handle;
	int ReadLen;
	struct MsgInfo * Msg;
	char * MsgBytes;

	Handle = CreateFile(MsgDatabasePath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
	{
		// Initialise a new File

		MsgHddrPtr=malloc(4);
		MsgHddrPtr[0]= zalloc(sizeof (MsgRec));
		NumberofMessages = 0;

		return;
	}


	// Get First Record
		
	ReadFile(Handle, &MsgRec, sizeof (MsgRec), &ReadLen, NULL); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&MsgRec, 0, sizeof (MsgRec));
	}

	// Set up control record

	MsgHddrPtr=malloc(4);
	MsgHddrPtr[0]= malloc(sizeof (MsgRec));
	memcpy(MsgHddrPtr[0], &MsgRec,  sizeof (MsgRec));

	LatestMsg=MsgHddrPtr[0]->length;

	NumberofMessages = 0;

Next:

	ReadFile(Handle, &MsgRec, sizeof (MsgRec), &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		// Validate Header

		if (MsgRec.type == 0)
			goto Next;

		MsgBytes = ReadMessageFile(MsgRec.number);

		if (MsgBytes)
		{
			MsgRec.length = strlen(MsgBytes);
			free(MsgBytes);
		}
		else
			goto Next;


		Msg = AllocateMsgRecord();
		memcpy(Msg, &MsgRec,  sizeof (MsgRec));

		// If any forward bits are set, increment count on corresponding BBS record.

		if (memcmp(MsgRec.fbbs, zeros, NBMASK) != 0)
		{
			if (FirstMessagetoForward == 0)
				FirstMessagetoForward = NumberofMessages;			// limit search
		}

		goto Next;
	}

	if (FirstMessagetoForward == 0)
		FirstMessagetoForward = NumberofMessages;			// limit search


	CloseHandle(Handle);

}

VOID CopyMessageDatabase()
{
	char Backup[MAX_PATH];

	strcpy(Backup, MsgDatabasePath);
	strcat(Backup, ".bak");

	CopyFile(MsgDatabasePath, Backup, FALSE);
}

VOID SaveMessageDatabase()
{
	HANDLE Handle;
	int WriteLen;
	int i;

	Handle = CreateFile(MsgDatabasePath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	MsgHddrPtr[0]->number = NumberofMessages;
	MsgHddrPtr[0]->length = LatestMsg;

	for (i=0; i <= NumberofMessages; i++)
	{
		WriteFile(Handle, MsgHddrPtr[i], sizeof (struct MsgInfo), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

VOID GetBIDDatabase()
{
	BIDRec BIDRec;
	HANDLE Handle;
	int ReadLen;
	BIDRecP BID;

	Handle = CreateFile(BIDDatabasePath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (Handle == INVALID_HANDLE_VALUE)
	{
		// Initialise a new File

		BIDRecPtr=malloc(4);
		BIDRecPtr[0]= malloc(sizeof (BIDRec));
		memset(BIDRecPtr[0], 0, sizeof (BIDRec));
		NumberofBIDs = 0;

		return;
	}


	// Get First Record
		
	ReadFile(Handle, &BIDRec, sizeof (BIDRec), &ReadLen, NULL); 

	if (ReadLen == 0)
	{
		// Duff file

		memset(&BIDRec, 0, sizeof (BIDRec));
	}

	// Set up control record

	BIDRecPtr=malloc(4);
	BIDRecPtr[0]= malloc(sizeof (BIDRec));
	memcpy(BIDRecPtr[0], &BIDRec,  sizeof (BIDRec));

	NumberofBIDs = 0;

Next:

	ReadFile(Handle, &BIDRec, sizeof (BIDRec), &ReadLen, NULL); 

	if (ReadLen > 0)
	{
		BID = AllocateBIDRecord();
		memcpy(BID, &BIDRec,  sizeof (BIDRec));

		if (BID->u.timestamp == 0) 	
			BID->u.timestamp = LOWORD(time(NULL)/86400);

		goto Next;
	}

	CloseHandle(Handle);
}

VOID CopyBIDDatabase()
{
	char Backup[MAX_PATH];

	strcpy(Backup, BIDDatabasePath);
	strcat(Backup, ".bak");

	CopyFile(BIDDatabasePath, Backup, FALSE);
}

VOID SaveBIDDatabase()
{
	HANDLE Handle;
	int WriteLen;
	int i;

	Handle = CreateFile(BIDDatabasePath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	BIDRecPtr[0]->u.msgno = NumberofBIDs;			// First Record has file size

	for (i=0; i <= NumberofBIDs; i++)
	{
		WriteFile(Handle, BIDRecPtr[i], sizeof (BIDRec), &WriteLen, NULL);
	}

	CloseHandle(Handle);

	return;
}

BIDRec * LookupBID(char * BID)
{
	BIDRec * ptr = NULL;
	int i;

	for (i=1; i <= NumberofBIDs; i++)
	{
		ptr = BIDRecPtr[i];

		if (_stricmp(ptr->BID, BID) == 0)
			return ptr;
	}

	return NULL;
}

BIDRec * LookupTempBID(char * BID)
{
	BIDRec * ptr = NULL;
	int i;

	for (i=1; i <= NumberofTempBIDs; i++)
	{
		ptr = TempBIDRecPtr[i];

		if (_stricmp(ptr->BID, BID) == 0) return ptr;
	}

	return NULL;
}

VOID RemoveTempBIDS(CIRCUIT * conn)
{
	// Remove any Temp BID records for conn. Called when connection closes - Msgs will be complete or failed
	
	if (NumberofTempBIDs == 0)
		return;
	else
	{
		BIDRec * ptr = NULL;
		BIDRec ** NewTempBIDRecPtr = zalloc((NumberofTempBIDs+1) * 4);
		int i = 0, n;

		for (n = 1; n <= NumberofTempBIDs; n++)
		{
			ptr = TempBIDRecPtr[n];

			if (ptr->u.conn == conn)
				// Remove this entry 
				free(ptr);
			else
				NewTempBIDRecPtr[++i] = ptr;
		}

		NumberofTempBIDs = i;

		free(TempBIDRecPtr);

		TempBIDRecPtr = NewTempBIDRecPtr;
	}
}


VOID SendWelcomeMsg(int Stream, ConnectionInfo * conn, struct UserInfo * user)
{
	LINK    *link;
//	char Msg[255];
//	int len;

	if (conn->Flags & CHATMODE)
	{	
		// See if from a defined node
		
		for (link = link_hd; link; link = link->next)
		{
			if (matchi(conn->Callsign, link->call))
			{
				conn->flags = p_linkwait;
				return;						// Wait for *RTL
			}
		}

		// Not a defined node - assume it's a user

		if (!rtloginu (conn))
		{
			// Already connected - close
			
			Flush(conn);
			Sleep(1000);
			Disconnect(conn->BPQStream);
		}
		return;
	}
	
	nodeprintf(conn, "Hello %s. Latest Message is %d, Last listed is %d\r",
		user->Name, LatestMsg, user->lastmsg);

	if (user->HomeBBS[0] == 0)
		BBSputs(conn, "Please enter your Home BBS using the Home command.\rYou may also enter your QTH and ZIP/Postcode using qth and zip commands.\r");

	nodeprintf(conn, "%s BBS (H for help) >\r", BBSName);
}

VOID SendPrompt(ConnectionInfo * conn, struct UserInfo * user)
{
	nodeprintf(conn, "%s>\r", BBSName);
}


VOID ProcessLine(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int len)
{
	char * Cmd, * Arg1;
	char * Context;
	char seps[] = " \t\r";
	int CmdLen;
	struct _EXCEPTION_POINTERS exinfo;


	if (conn->Flags & CHATMODE)
	{
		GetSemaphore(&ChatSemaphore);

		__try 
		{
			ProcessChatLine(conn, user, Buffer, len);
		}
		My__except_Routine("ProcessChatLine");
		
		FreeSemaphore(&ChatSemaphore);
		return;
	}

	WriteLogLine('<',Buffer, len-1, LOG_BBS);


	if (conn->BBSFlags & FBBForwarding)
	{
		__try 
		{
			ProcessFBBLine(conn, user, Buffer, len);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error in ProcessFBBLine");
			Disconnect(conn->BPQStream);
		}
		return;
	}

	if (conn->Flags & GETTINGMESSAGE)
	{
		__try 
		{
			ProcessMsgLine(conn, user, Buffer, len);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error in ProcessMSGLine");
			Disconnect(conn->BPQStream);
		}
		return;	}

	if (conn->Flags & GETTINGTITLE)
	{
		__try 
		{
			ProcessMsgTitle(conn, user, Buffer, len);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error in ProcessMsgTitle");
			Disconnect(conn->BPQStream);
		}
		return;
	}

	if (conn->BBSFlags & MBLFORWARDING)
	{
		__try 
		{
			ProcessMBLLine(conn, user, Buffer, len);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			Debugprintf("MAILCHAT *** Program Error in ProcessMBLLine");
			Disconnect(conn->BPQStream);
		}
		return;
	}
	if (conn->Flags & GETTINGUSER)
	{
		memcpy(user->Name, Buffer, len-1);
		conn->Flags &=  ~GETTINGUSER;
		SendWelcomeMsg(conn->BPQStream, conn, user);
		UpdateWPWithUserInfo(user);
		SaveUserDatabase();

		return;
	}


	// Process Command

	if (conn->Paging && (conn->LinesSent >= conn->PageLen))
	{
		// Waiting for paging prompt

		if (len > 1)
		{
			if (_memicmp(Buffer, "Abort", 1) == 0)
			{
				ClearQueue(conn);
				conn->LinesSent = 0;

				QueueMsg(conn, AbortedMsg, strlen(AbortedMsg));
				SendPrompt(conn, user);
				return;
			}
		}

		conn->LinesSent = 0;
		return;
	}

	if (len == 1)
	{
		SendPrompt(conn, user);
		return;
	}

	Buffer[len] = 0;

	if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
	{
		// If a BBS, set BBS Flag

		if (user->flags & F_BBS)
		{
			if (user->ForwardingInfo)
			{
				if (user->ForwardingInfo->Forwarding)
				{
					BBSputs(conn, "Already Connected\r");
					Flush(conn);
					Sleep(500);
					Disconnect(conn->BPQStream);
					return;
				}

			}

			if (user->ForwardingInfo)
			{
				user->ForwardingInfo->Forwarding = TRUE;
				user->ForwardingInfo->FwdTimer = 0;			// So we dont send to immediately
			}

			Parse_SID(conn, &Buffer[1], len-4);
			
			if (conn->BBSFlags & FBBForwarding)
			{
				conn->FBBIndex = 0;		// ready for first block;
				conn->FBBChecksum = 0;
			}
			else
				BBSputs(conn, ">\r");

		}
		
		return;
	}

	Cmd = strtok_s(Buffer, seps, &Context);

	if (Cmd == NULL)
	{
		BBSputs(conn, "Invalid Command\r");
		SendPrompt(conn, user);
		return;
	}

	Arg1 = strtok_s(NULL, seps, &Context);
	CmdLen = strlen(Cmd);

	if (_memicmp(Cmd, "Abort", 1) == 0)
	{
		ClearQueue(conn);
		conn->LinesSent = 0;

		QueueMsg(conn, AbortedMsg, strlen(AbortedMsg));
		SendPrompt(conn, user);
		return;
	}
	if (_memicmp(Cmd, "Bye", CmdLen) == 0)
	{
		SendUnbuffered(conn->BPQStream, SignoffMsg, strlen(SignoffMsg));
		user->lastmsg = conn->lastmsg;
		Sleep(1000);

		if (conn->BPQStream > 0)
			Disconnect(conn->BPQStream);
		else
			CloseConsole(conn->BPQStream);

		return;
	}

	if (_memicmp(Cmd, "K", 1) == 0)
	{
		DoKillCommand(conn, user, Cmd, Arg1, Context);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "L", 1) == 0)
	{
		DoListCommand(conn, user, Cmd, Arg1);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "Name", CmdLen) == 0)
	{
		if (Arg1)
		{
			if (strlen(Arg1) > 17)
				Arg1[17] = 0;

			strcpy(user->Name, Arg1);
			UpdateWPWithUserInfo(user);

		}

		SendWelcomeMsg(conn->BPQStream, conn, user);
		SaveUserDatabase();

		return;
	}

	if (_memicmp(Cmd, "OP", 2) == 0)
	{
		int Lines;
		
		// Paging Control. Param is number of lines per page

		if (Arg1)
		{
			Lines = atoi(Arg1);

			user->PageLen = Lines;
			conn->PageLen = Lines;
			conn->Paging = (Lines > 0);
			SaveUserDatabase();
		}
		
		nodeprintf(conn,"Page Length is %d\r", user->PageLen);
		SendPrompt(conn, user);

		return;
	}

	if (_memicmp(Cmd, "QTH", CmdLen) == 0)
	{
		if (Arg1)
		{
			// QTH may contain spaces, so put back together, and just split at cr
			
			Arg1[strlen(Arg1)] = ' ';
			strtok_s(Arg1, "\r", &Context);

			if (strlen(Arg1) > 60)
				Arg1[60] = 0;

			strcpy(user->Address, Arg1);
			UpdateWPWithUserInfo(user);

		}

		nodeprintf(conn,"QTH is %s\r", user->Address);
		SendPrompt(conn, user);

		SaveUserDatabase();

		return;
	}

	if (_memicmp(Cmd, "ZIP", CmdLen) == 0)
	{
		if (Arg1)
		{
			if (strlen(Arg1) > 8)
				Arg1[8] = 0;

			strcpy(user->ZIP, _strupr(Arg1));
			UpdateWPWithUserInfo(user);
		}

		nodeprintf(conn,"ZIP is %s\r", user->ZIP);
		SendPrompt(conn, user);

		SaveUserDatabase();

		return;
	}

	if (_memicmp(Cmd, "R", 1) == 0)
	{
		DoReadCommand(conn, user, Cmd, Arg1, Context);
		SendPrompt(conn, user);
		return;
	}

	if (_memicmp(Cmd, "S", 1) == 0)
	{
		if (!DoSendCommand(conn, user, Cmd, Arg1, Context))
			SendPrompt(conn, user);
		return;
	}

	if ((_memicmp(Cmd, "Help", CmdLen) == 0) || (_memicmp(Cmd, "?", 1) == 0))
	{
		BBSputs(conn, "A - Abort Output\r");
		BBSputs(conn, "B - Logoff\r");
		BBSputs(conn, "K - Kill Message(s) - K num, KM (Kill my read messages)\r");
		BBSputs(conn, "L - List Message(s) - L = List New, LM = List Mine L> Call, L< Call = List to or from\r");
		BBSputs(conn, "                      LL num = List Last, L num-num = List Range\r");
		BBSputs(conn, "                      LN LY LH LK LF L$ - List Mesaage with corresponding Status\r");
		BBSputs(conn, "                      LB LP - List Mesaage with corresponding Type\r");
		BBSputs(conn, "N Name - Set Name\r");
		BBSputs(conn, "OP n - Set Page Length (Output will pause every n lines)\r");
		BBSputs(conn, "Q QTH - Set QTH\r");
		BBSputs(conn, "R - Read Message(s) - R num, RM (Read new messages to me)\r");
		BBSputs(conn, "S - Send Message - S or SP Send Personal, SB Send Bull, SR Num - Send Reply, SC Num - Send Copy\r");

		SendPrompt(conn, user);

		return;

	}

	if (_memicmp(Cmd, "Ver", CmdLen) == 0)
	{
		if (GetVersionStringptr)
			nodeprintf(conn, "BBS Version %s\rNode Version %s\r", VersionStringWithBuild, GetVersionStringptr());
		else
			nodeprintf(conn, "BBS Version %s\r", VersionStringWithBuild);

		SendPrompt(conn, user);

		return;
	}

	if (_memicmp(Cmd, "HOMEBBS", CmdLen) == 0)
	{
		if (Arg1)
		{
			if (strlen(Arg1) > 40) Arg1[40] = 0;

			strcpy(user->HomeBBS, _strupr(Arg1));
			UpdateWPWithUserInfo(user);
	
			if (!strchr(Arg1, '.'))
				BBSputs(conn, "Please enter HA with HomeBBS eg g8bpq.gbr.eu - this will help message routing\r");
		}

		nodeprintf(conn,"HomeBBS is %s\r", user->HomeBBS);
		SendPrompt(conn, user);

		SaveUserDatabase();

		return;
	}


	if (_memicmp(Cmd, "Chat", 4) == 0)
	{
		if(ChatApplNum == 0)
		{
			BBSputs(conn, "Chat Node is disabled\r");
			SendPrompt(conn, user);
			return;
		}

		if (rtloginu (conn))
			conn->Flags |= CHATMODE;
		else
			SendPrompt(conn, user);

		return;
	}

	if (conn->Flags == 0)
	{
		BBSputs(conn, "Invalid Command\r");
		SendPrompt(conn, user);
	}

	//	Send if possible

	Flush(conn);
}


VOID SendUnbuffered(int stream, char * msg, int len)
{
	if (stream == -1)
		WritetoConsoleWindow(msg, len);
	else
		SendMsg(stream, msg, len);
}


int QueueMsg(ConnectionInfo * conn, char * msg, int len)
{
	// Add Message to queue for this connection

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	// Create or extend buffer

	conn->OutputQueue=realloc(conn->OutputQueue, conn->OutputQueueLength + len);

	if (conn->OutputQueue == NULL)
	{
		// relloc failed - should never happen, but clean up

		CriticalErrorHandler("realloc failed to expand output queue");
		return 0;
	}

	memcpy(&conn->OutputQueue[conn->OutputQueueLength], msg, len);

	conn->OutputQueueLength+=len;

	return len;
}

void TrytoSend()
{
	// call Flush on any connected streams with queued data

	ConnectionInfo * conn;
	int n;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (conn->Active == TRUE)
			if (conn->OutputQueue)
				Flush(conn);
	}

	if (Console)
		Flush(Console);
}


void Flush(CIRCUIT * conn)
{
	int tosend, len, sent;
	
	// Try to send data to user. May be stopped by user paging or node flow control

	//	UCHAR * OutputQueue;		// Messages to user
	//	int OutputQueueLength;		// Total Malloc'ed size. Also Put Pointer for next Message
	//	int OutputGetPointer;		// Next byte to send. When Getpointer = Quele Length all is sent - free the buffer and start again.

	//	BOOL Paging;				// Set if user wants paging
	//	int LinesSent;				// Count when paging
	//	int PageLen;				// Lines per page


	if (conn->OutputQueue == NULL)
		return;						// Nothing to send

	tosend = conn->OutputQueueLength - conn->OutputGetPointer;

	sent=0;

	while (tosend > 0)
	{
		if (TXCount(conn->BPQStream) > 4)
			return;						// Busy

		if (conn->Paging && (conn->LinesSent >= conn->PageLen))
			return;

		if (tosend <= conn->paclen)
			len=tosend;
		else
			len=conn->paclen;

		if (conn->Paging)
		{
			// look for CR chars in message to send. Increment LinesSent, and stop if at limit

			char * ptr1 = &conn->OutputQueue[conn->OutputGetPointer];
			char * ptr2;
			int lenleft = len;

			ptr2 = memchr(ptr1, 0x0d, len);

			while (ptr2)
			{
				conn->LinesSent++;
				ptr2++;
				lenleft = len - (ptr2 - ptr1);

				if (conn->LinesSent >= conn->PageLen)
				{
					len = ptr2 - &conn->OutputQueue[conn->OutputGetPointer];
					
					SendUnbuffered(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);
					conn->OutputGetPointer+=len;
					tosend-=len;
					SendUnbuffered(conn->BPQStream, "<A>bort, <CR> Continue..>", 25);

					return;

				}
				ptr2 = memchr(ptr2, 0x0d, lenleft);
			}
		}

		SendUnbuffered(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);

		conn->OutputGetPointer+=len;

		tosend-=len;

		
	
		sent++;

		if (sent > 4)
			return;

	}

	// All Sent. Free buffers and reset pointers

	conn->LinesSent = 0;

	ClearQueue(conn);
}

VOID ClearQueue(ConnectionInfo * conn)
{
	if (conn->OutputQueue == NULL)
		return;

	free(conn->OutputQueue);

	conn->OutputQueue=NULL;
	conn->OutputGetPointer=0;
	conn->OutputQueueLength=0;
}



VOID FlagAsKilled(struct MsgInfo * Msg)
{
	struct UserInfo * user;

	Msg->status='K';
	Msg->datechanged=time(NULL);

	// Remove any forwarding references

	if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
	{	
		for (user = BBSChain; user; user = user->BBSNext)
		{
			if (check_fwd_bit(Msg->fbbs, user->BBSNumber))
			{
				user->ForwardingInfo->MsgCount--;
				clear_fwd_bit(Msg->fbbs, user->BBSNumber);
			}
		}
	}
}



void DoKillCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	int i;
	struct MsgInfo * Msg;
	
	switch (toupper(Cmd[1]))
	{

	case 0:					// Just K

		while (Arg1)
		{
			msgno = atoi(Arg1);
			KillMessage(conn, user, msgno);

			Arg1 = strtok_s(NULL, " \r", &Context);
		}

		return;

	case 'M':					// Kill Mine

		for (i=NumberofMessages; i>0; i--)
		{
			Msg = MsgHddrPtr[i];

			if ((_stricmp(Msg->to, user->Call) == 0) || ((conn->sysop) && (_stricmp(Msg->to, "SYSOP") == 0)))
			{
				if (Msg->type == 'P' && Msg->status == 'Y')
				{
					FlagAsKilled(Msg);
					nodeprintf(conn, "Message #%d Killed\r", Msg->number);
				}
			}
		}
		return;

	case '>':			// K> - Kill to 

		if (conn->sysop)
		{
			if (Arg1)
				if (KillMessagesTo(conn, user, Arg1) == 0)
				BBSputs(conn, "No Messages found\r");
		
		
			return;
		}

	case '<':

		if (conn->sysop)
		{
			if (Arg1)
				if (KillMessagesFrom(conn, user, Arg1) == 0);
					BBSputs(conn, "No Messages found\r");

					return;
		}
	}

	nodeprintf(conn, "*** Error: Invalid Kill option %c\r", Cmd[1]);

	return;

}

int KillMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;
	struct MsgInfo * Msg;

	for (i=NumberofMessages; i>0; i--)
	{
		Msg = MsgHddrPtr[i];
		if (Msg->status != 'K' && _stricmp(Msg->to, Call) == 0)
		{
			Msgs++;
			KillMessage(conn, user, MsgHddrPtr[i]->number);
		}
	}
	
	return(Msgs);
}

int KillMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;
	struct MsgInfo * Msg;


	for (i=NumberofMessages; i>0; i--)
	{
		Msg = MsgHddrPtr[i];
		if (Msg->status != 'K' && _stricmp(Msg->from, Call) == 0)
		{
			Msgs++;
			KillMessage(conn, user, MsgHddrPtr[i]->number);
		}
	}
	
	return(Msgs);
}



void KillMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno)
{
	struct MsgInfo * Msg;

	Msg = FindMessage(user->Call, msgno, conn->sysop);

	if (Msg == NULL)
	{
		nodeprintf(conn, "Message %d not found\r", msgno);
		return;
	}

	FlagAsKilled(Msg);

	nodeprintf(conn, "Message #%d Killed\r", msgno);

}


VOID ListMessage(struct MsgInfo * Msg, ConnectionInfo * conn)
{
	if (Msg->to[0] == 0 && Msg->via[0] != 0)
		nodeprintf(conn, "%-6d %s %c%c   %5d %-6s %-6s %-s\r",
				Msg->number, FormatDateAndTime(Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, Msg->via, Msg->from, Msg->title);

	else
		if (Msg->via[0] != 0)
			nodeprintf(conn, "%-6d %s %c%c   %5d %-6s@%-6s %-6s %-s\r",
				Msg->number, FormatDateAndTime(Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, Msg->to, Msg->via, Msg->from, Msg->title);
	else
		nodeprintf(conn, "%-6d %s %c%c   %5d %-6s        %-6s %-s\r",
				Msg->number, FormatDateAndTime(Msg->datecreated, TRUE), Msg->type, Msg->status, Msg->length, Msg->to, Msg->from, Msg->title);

	if (Msg->number > conn->lastmsg) 
		conn->lastmsg = Msg->number;

}

void DoListCommand(ConnectionInfo * conn, struct UserInfo * user, char * Cmd, char * Arg1)
{
	switch (toupper(Cmd[1]))
	{

	case 0:					// Just L
	case 'R':			// LR = List Reverse

		if (Arg1)
		{
			// Range nnn-nnn or single value

			char * Arg2, * Arg3;
			char * Context;
			char seps[] = " -\t\r";
			int From=LatestMsg, To=0;
			char * Range = strchr(Arg1, '-');
			
			Arg2 = strtok_s(Arg1, seps, &Context);
			Arg3 = strtok_s(NULL, seps, &Context);

			if (Arg2)
				To = From = atoi(Arg2);

			if (Arg3)
				From = atoi(Arg3);
			else
				if (Range)
					From = LatestMsg;


			if (Cmd[1])
				ListMessagesInRangeForwards(conn, user, user->Call, From, To);
			else
				ListMessagesInRange(conn, user, user->Call, From, To);

		}
		else

			if (Cmd[1])
				ListMessagesInRangeForwards(conn, user, user->Call, LatestMsg, conn->lastmsg);
			else
				ListMessagesInRange(conn, user, user->Call, LatestMsg, conn->lastmsg);

		return;



	case 'L':				// List Last

		if (Arg1)
		{
			int i = atoi(Arg1);
			int m = NumberofMessages;
				
			for (; i>0 && m != 0; i--)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					ListMessage(MsgHddrPtr[m], conn);
					m--;
				}
			}
		}
		return;

	case 'M':			// LM - List Mine

		if (ListMessagesTo(conn, user, user->Call) == 0)
			BBSputs(conn, "No Messages found\r");
		return;

	case '>':			// L> - List to 

		if (Arg1)
			if (ListMessagesTo(conn, user, Arg1) == 0)
				BBSputs(conn, "No Messages found\r");
		
		
		return;

	case '<':

		if (Arg1)
			if (ListMessagesFrom(conn, user, Arg1) == 0)
				BBSputs(conn, "No Messages found\r");
		
		return;

	case '@':

		if (Arg1)
			if (ListMessagesAT(conn, user, Arg1) == 0)
				BBSputs(conn, "No Messages found\r");
		
		return;

	case 'H':				// List Status
	case 'N':
	case 'Y':
	case 'F':
	case '%':
		{
			int m = NumberofMessages;
				
			while (m > 0)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					if (MsgHddrPtr[m]->status == toupper(Cmd[1]))
						ListMessage(MsgHddrPtr[m], conn);
					m--;
				}
			}
		}
		return;

	case 'K':
		{
			int i, Msgs = 0;

			for (i=NumberofMessages; i>0; i--)
			{
				if (MsgHddrPtr[i]->status == 'K')
				{
					Msgs++;
					ListMessage(MsgHddrPtr[i], conn);
				}
			}

			if (Msgs == 0)
				BBSputs(conn, "No Messages found\r");

		}

	case 'P':
	case 'B':
		{
			int m = NumberofMessages;
				
			while (m > 0)
			{
				m = GetUserMsg(m, user->Call, conn->sysop);

				if (m > 0)
				{
					if (MsgHddrPtr[m]->type == toupper(Cmd[1]))
						ListMessage(MsgHddrPtr[m], conn);
					m--;
				}
			}

			return;
		}
		}
	
	nodeprintf(conn, "*** Error: Invalid List option %c\r", Cmd[1]);

}
	
int ListMessagesTo(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;

	for (i=NumberofMessages; i>0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if ((_stricmp(MsgHddrPtr[i]->to, Call) == 0) ||
			((conn->sysop) && (_stricmp(MsgHddrPtr[i]->to, "SYSOP") == 0)  && (_stricmp(Call, SYSOPCall) == 0)))
		{
			Msgs++;
			ListMessage(MsgHddrPtr[i], conn);
		}
	}
	
	return(Msgs);
}

int ListMessagesFrom(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;

	for (i=NumberofMessages; i>0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if (_stricmp(MsgHddrPtr[i]->from, Call) == 0)
		{
			Msgs++;
			ListMessage(MsgHddrPtr[i], conn);
		}
	}
	
	return(Msgs);
}

int ListMessagesAT(ConnectionInfo * conn, struct UserInfo * user, char * Call)
{
	int i, Msgs = 0;

	for (i=NumberofMessages; i>0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if (_stricmp(MsgHddrPtr[i]->via, Call) == 0)
		{
			Msgs++;
			ListMessage(MsgHddrPtr[i], conn);
		}
	}
	
	return(Msgs);
}
int GetUserMsg(int m, char * Call, BOOL SYSOP)
{
	struct MsgInfo * Msg;
	
	// Get Next (usually backwards) message which should be shown to this user
	//	ie Not Deleted, and not Private unless to or from Call

	do
	{
		Msg=MsgHddrPtr[m];

		if (Msg->status != 'K')
		{
			if (SYSOP) return m;			// Sysop can list or read anything
	
			if (Msg->type == 'B') return m;

			if (Msg->type == 'P')
			{
				if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
					return m;
			}
		}

		m--;

	} while (m> 0);

	return 0;

}

int GetUserMsgForwards(int m, char * Call, BOOL SYSOP)
{
	struct MsgInfo * Msg;
	
	// Get Next (usually backwards) message which should be shown to this user
	//	ie Not Deleted, and not Private unless to or from Call

	do
	{
		Msg=MsgHddrPtr[m];
		
		if (Msg->status != 'K')
		{
			if (SYSOP) return m;			// Sysop can list or read anything

			if (Msg->type == 'B') return m;

			if (Msg->type == 'P')
			{
				if ((_stricmp(Msg->to, Call) == 0) || (_stricmp(Msg->from, Call) == 0))
					return m;
			}
		}

		m++;

	} while (m <= NumberofMessages);

	return 0;

}


void ListMessagesInRange(ConnectionInfo * conn, struct UserInfo * user, char * Call, int Start, int End)
{
	int m=NumberofMessages;

	struct MsgInfo * Msg;

	if (NumberofMessages == 0)
		return;


	do
	{
		m = GetUserMsg(m, user->Call, conn->sysop);

		if (m < 1)
			return;

		Msg=MsgHddrPtr[m];

		if (Msg->number < End)
			return;

		if (Msg->number <= Start)
			ListMessage(MsgHddrPtr[m], conn);

		m--;

	} while (m> 0);
}

void ListMessagesInRangeForwards(ConnectionInfo * conn, struct UserInfo * user, char * Call, int End, int Start)
{
	int m=1;

	struct MsgInfo * Msg;

	if (NumberofMessages == 0)
		return;


	do
	{
		m = GetUserMsgForwards(m, user->Call, conn->sysop);

		if (m > NumberofMessages)
			return;

		Msg=MsgHddrPtr[m];

		if (Msg->number > End)
			return;

		if (Msg->number >= Start)
			ListMessage(MsgHddrPtr[m], conn);

		m++;

	} while (m <= NumberofMessages);
}



void DoReadCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	int msgno=-1;
	int i;
	struct MsgInfo * Msg;

	
	switch (toupper(Cmd[1]))
	{
	case 0:					// Just R

		while (Arg1)
		{
			msgno = atoi(Arg1);
			ReadMessage(conn, user, msgno);
			Arg1 = strtok_s(NULL, " \r", &Context);
		}

		return;

	case 'M':					// Read Mine (Unread Messages)

		for (i=NumberofMessages; i>0; i--)
		{
			Msg = MsgHddrPtr[i];

			if ((_stricmp(Msg->to, user->Call) == 0) || ((conn->sysop) && (_stricmp(Msg->to, "SYSOP") == 0)))
				if (Msg->status == 'N')
					ReadMessage(conn, user, Msg->number);
		}

		return;
	}
	
	nodeprintf(conn, "*** Error: Invalid Read option %c\r", Cmd[1]);
	
	return;
}

int RemoveLF(char * Message, int len)
{
	// Remove lf chars

	char * ptr1, * ptr2;

	ptr1 = ptr2 = Message;

	while (len-- > 0)
	{
		*ptr2 = *ptr1;
	
		if (*ptr1 == '\r')
			if (*(ptr1+1) == '\n')
			{
				ptr1++;
				len--;
			}
		ptr1++;
		ptr2++;
	}

	return (ptr2 - Message);
}

void ReadMessage(ConnectionInfo * conn, struct UserInfo * user, int msgno)
{
	struct MsgInfo * Msg;
	char * MsgBytes;

	Msg = FindMessage(user->Call, msgno, conn->sysop);

	if (Msg == NULL)
	{
		nodeprintf(conn, "Message %d not found\r", msgno);
		return;
	}

	nodeprintf(conn, "From: %s\rTo: %s\rType/Status %c%c\rDate/Time: %s\rBid: %s\rTitle: %s\r\r",
		Msg->from, Msg->to, Msg->type, Msg->status, FormatDateAndTime(Msg->datecreated, FALSE), Msg->bid, Msg->title);

	MsgBytes = ReadMessageFile(msgno);

	if (MsgBytes)
	{
		// Remove lf chars

		int Length = RemoveLF(MsgBytes, strlen(MsgBytes));

		QueueMsg(conn, MsgBytes, Length);
		free(MsgBytes);

		nodeprintf(conn, "\r\r[End of Message #%d from %s]\r", msgno, Msg->from);

		if ((_stricmp(Msg->to, user->Call) == 0) || ((conn->sysop) && (_stricmp(Msg->to, "SYSOP") == 0)))
		{
			Msg->status = 'Y';
			Msg->datechanged=time(NULL);
		}


	}
	else
	{
		nodeprintf(conn, "File for Message %d not found\r", msgno);
	}

}
 struct MsgInfo * FindMessage(char * Call, int msgno, BOOL sysop)
 {
	int m=NumberofMessages;

	struct MsgInfo * Msg;

	do
	{
		m = GetUserMsg(m, Call, sysop);

		if (m == 0)
			return NULL;

		Msg=MsgHddrPtr[m];

		if (Msg->number == msgno)
			return Msg;

		m--;

	} while (m> 0);

	return NULL;

}

char * ReadMessageFile(int msgno)
{
	int FileSize;
	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	char * MsgBytes;
	int ReadLen;
 
	sprintf_s(MsgFile, sizeof(MsgFile), "%s\\m_%06d.mes", MailDir, msgno);
	
	hFile = CreateFile(MsgFile,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;

	FileSize = GetFileSize(hFile, NULL);

	MsgBytes=malloc(FileSize+1);

	ReadFile(hFile, MsgBytes, FileSize, &ReadLen, NULL); 

	CloseHandle(hFile);

	MsgBytes[FileSize]=0;

	return MsgBytes;


}

/*
r 1378
From         : G8BPQ
To           : G8BPQ
Type/Status  : PN
Date/Time    : 09-May 13:01
Bid          : 1378_G8BPQ
Message #    : 1378
Title        : Error-report
> *** System boot on Sun 01/03/09 00:01 ***
> *** System boot on Tue 05/05/09 14:21 ***
> *** System boot on Sat 09/05/09 11:23 ***
> *** System boot on Sat 09/05/09 11:30 ***
> *** System boot on Sat 09/05/09 11:49 ***
> *** System boot on Sat 09/05/09 12:24 ***
***************



[End of Message #1378 from G8BPQ]
*/

char * FormatDateAndTime(time_t Datim, BOOL DateOnly)
{
	struct tm *tm;
	static char Date[]="xx-xxx hh:mmZ";

	tm = gmtime(&Datim);
	
	if (tm)
		sprintf_s(Date, sizeof(Date), "%02d-%3s %02d:%02dZ",
					tm->tm_mday, month[tm->tm_mon], tm->tm_hour, tm->tm_min);

	if (DateOnly)
	{
		Date[6]=0;
		return Date;
	}
	
	return Date;
}

BOOL DecodeSendParams(CIRCUIT * conn, char * Context, char ** From, char ** To, char ** ATBBS, char ** BID);


BOOL DoSendCommand(CIRCUIT * conn, struct UserInfo * user, char * Cmd, char * Arg1, char * Context)
{
	// SB WANT @ ALLCAN < N6ZFJ $4567_N0ARY
	
	char * From = NULL;
	char * BID = NULL;
	char * ATBBS = NULL;
	char seps[] = " \t\r";
	struct MsgInfo * OldMsg;
	char OldTitle[62];
	char NewTitle[62];

	int msgno;

	switch (toupper(Cmd[1]))
	{

	case 0:					// Just S means SP
		
		Cmd[1] = 'P';

	case 'P':
	case 'B':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: The 'TO' callsign is missing\r");
			return FALSE;
		}

		if (!DecodeSendParams(conn, Context, &From, &Arg1, &ATBBS, &BID))
			return FALSE;

		return CreateMessage(conn, From, Arg1, ATBBS, toupper(Cmd[1]), BID, NULL);	

	case 'R':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: Message Number is missing\r");
			return FALSE;
		}

		msgno = atoi(Arg1);

		OldMsg = FindMessage(user->Call, msgno, conn->sysop);

		if (OldMsg == NULL)
		{
			nodeprintf(conn, "Message %d not found\r", msgno);
			return FALSE;
		}

		Arg1=&OldMsg->from[0];

		if (!DecodeSendParams(conn, "", &From, &Arg1, &ATBBS, &BID))
			return FALSE;

		strcpy(OldTitle, OldMsg->title);

		if (strlen(OldTitle) > 57) OldTitle[57] = 0;

		strcpy(NewTitle, "Re:");
		strcat(NewTitle, OldTitle);

		return CreateMessage(conn, From, Arg1, ATBBS, 'P', BID, NewTitle);	

		return TRUE;

	case 'C':
				
		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: Message Number is missing\r");
			return FALSE;
		}

		msgno = atoi(Arg1);

		Arg1 = strtok_s(NULL, seps, &Context);

		if (Arg1 == NULL)
		{
			nodeprintf(conn, "*** Error: The 'TO' callsign is missing\r");
			return FALSE;
		}

		if (!DecodeSendParams(conn, Context, &From, &Arg1, &ATBBS, &BID))
			return FALSE;
	
		OldMsg = FindMessage(user->Call, msgno, conn->sysop);

		if (OldMsg == NULL)
		{
			nodeprintf(conn, "Message %d not found\r", msgno);
			return FALSE;
		}

		strcpy(OldTitle, OldMsg->title);

		if (strlen(OldTitle) > 56) OldTitle[56] = 0;

		strcpy(NewTitle, "Fwd:");
		strcat(NewTitle, OldTitle);

		conn->CopyBuffer = ReadMessageFile(msgno);

		return CreateMessage(conn, From, Arg1, ATBBS, 'P', BID, NewTitle);	
	}


	nodeprintf(conn, "*** Error: Invalid Send option %c\r", Cmd[1]);

	return FALSE;
}

BOOL DecodeSendParams(CIRCUIT * conn, char * Context, char ** From, char ** To, char ** ATBBS, char ** BID)
{
	char * ptr;
	char seps[] = " \t\r";
	WPRecP WP;


	// Accept call@call (without spaces) - but check for smtp addresses

	if (_memicmp(*To, "smtp:", 5) != 0)
	{
		ptr = strchr(*To, '@');

		if (ptr)
		{
			*ATBBS = strlop(*To, '@');
		}
	}

	// Look for Optional fields;

	ptr = strtok_s(NULL, seps, &Context);

	while (ptr)
	{
		if (strcmp(ptr, "@") == 0)
		{
			*ATBBS = _strupr(strtok_s(NULL, seps, &Context));
		}
		else if(strcmp(ptr, "<") == 0)
		{
			*From = strtok_s(NULL, seps, &Context);
		}
		else if (ptr[0] == '$')
			*BID = &ptr[1];
		else
		{
			nodeprintf(conn, "*** Error: Invalid Format\r");
			return FALSE;
		}
		ptr = strtok_s(NULL, seps, &Context);
	}

	// Only allow < from a BBS

	if (*From)
	{
		if (!(conn->BBSFlags & BBS))
		{
			nodeprintf(conn, "*** < can only be used by a BBS\r");
			return FALSE;
		}
	}

	if (!*From)
		*From = conn->UserPointer->Call;

	if (!(conn->BBSFlags & BBS))
	{
		// if a normal user, check that TO and/or AT are known and warn if not.

		if (_stricmp(*To, "SYSOP") == 0)
		{
			conn->LocalMsg = TRUE;
			return TRUE;
		}

		if (!*ATBBS)
		{
			// No routing, if not a user and not known to forwarding or WP warn

			struct UserInfo * ToUser = LookupCall(*To);

			if (ToUser)
			{
				// Local User. If Home BBS is specified, use it

				if (ToUser->HomeBBS[0])
				{
					*ATBBS = ToUser->HomeBBS;
					nodeprintf(conn, "Address @%s added from HomeBBS\r", *ATBBS);
				}
				else
				{
					conn->LocalMsg = TRUE;
				}
			}
			else
			{
				conn->LocalMsg = FALSE;
				WP = LookupWP(*To);

				if (WP)
				{
					*ATBBS = WP->first_homebbs;
					nodeprintf(conn, "Address @%s added from WP\r", *ATBBS);
				}
			}
		}
	}
	return TRUE;
}

BOOL CreateMessage(CIRCUIT * conn, char * From, char * ToCall, char * ATBBS, char MsgType, char * BID, char * Title)
{
	struct MsgInfo * Msg;
	char * via = NULL;

	// Create a temp msg header entry

	Msg = malloc(sizeof (struct MsgInfo));

	if (Msg == 0)
	{
		CriticalErrorHandler("malloc failed for new message header");
		return FALSE;
	}
	
	memset(Msg, 0, sizeof (struct MsgInfo));

	conn->TempMsg = Msg;

	Msg->type = MsgType;
	
	if (conn->UserPointer->flags & F_HOLDMAIL)
		Msg->status = 'H';
	else
		Msg->status = 'N';
	
	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	if (BID)
	{
		BIDRec * TempBID;
		
		if (LookupBID(BID))
		{
			// Duplicate bid
	
			if (conn->BBSFlags & BBS)
				nodeprintf(conn, "NO - BID\r");
			else
				nodeprintf(conn, "*** Error- Duplicate BID\r");

			return FALSE;
		}

		if (strlen(BID) > 12) BID[12] = 0;
		strcpy(Msg->bid, BID);

		// Save BID in temp list in case we are offered it again before completion
			
		TempBID = AllocateTempBIDRecord();
		strcpy(TempBID->BID, BID);
		TempBID->u.conn = conn;

	}

	if (_memicmp(ToCall, "smtp:", 5) == 0)
	{
		if (ISP_Gateway_Enabled)
		{
			if ((conn->UserPointer->flags & F_EMAIL) == 0)
			{
				nodeprintf(conn, "*** Error - You need to ask the SYSOP to allow you to use Internet Mail\r");
				return FALSE;
			}
			via=strlop(ToCall, ':');
			ToCall[0] = 0;
		}
		else
		{
			nodeprintf(conn, "*** Error - Sending mail to smtp addresses is disabled\r");
			return FALSE;
		}
	}
	else
	{
		_strupr(ToCall);
		if (ATBBS)
			via=_strupr(ATBBS);
	}

	if (strlen(ToCall) > 6) ToCall[6] = 0;
	
	strcpy(Msg->to, ToCall);

	if (via)
	{
		if (strlen(via) > 40) via[40] = 0;

		strcpy(Msg->via, via);
	}

	strcpy(Msg->from, From);

	if (Title)					// Only used by SR and SC
	{
		strcpy(Msg->title, Title);
		conn->Flags |= GETTINGMESSAGE;
		nodeprintf(conn, "Enter Message Text (end with /ex or ctrl/z)\r");
		return TRUE;
	}

	if (!(conn->BBSFlags & FBBCompressed))
		conn->Flags |= GETTINGTITLE;

	if (!(conn->BBSFlags & BBS))
		nodeprintf(conn, "Enter Title (only):\r");
	else
		if (!(conn->BBSFlags & FBBForwarding))
			nodeprintf(conn, "OK\r");

	return TRUE;
}

VOID ProcessMsgTitle(ConnectionInfo * conn, struct UserInfo * user, char* Buffer, int msglen)
{
		
	conn->Flags &= ~GETTINGTITLE;

	if (msglen == 1)
	{
		nodeprintf(conn, "*** Message Cancelled\r");
		SendPrompt(conn, user);
		return;
	}

	if (msglen > 60) msglen = 60;

	Buffer[msglen-1] = 0;

	strcpy(conn->TempMsg->title, Buffer);

	// Create initial buffer of 10K. Expand if needed later

	conn->MailBuffer=malloc(10000);
	conn->MailBufferSize=10000;

	if (conn->MailBuffer == NULL)
	{
		nodeprintf(conn, "Failed to create Message Buffer\r");
		return;
	}

	conn->Flags |= GETTINGMESSAGE;

	if (!conn->BBSFlags & BBS)
		nodeprintf(conn, "Enter Message Text (end with /ex or ctrl/z)\r");

}

VOID ProcessMsgLine(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int msglen)
{
	char * ptr2 = NULL;

	if (((msglen < 3) && (Buffer[0] == 0x1a)) || ((msglen == 4) && (_memicmp(Buffer, "/ex", 3) == 0)))
	{
		conn->Flags &= ~GETTINGMESSAGE;

		CreateMessageFromBuffer(conn);
		return;

	}

	Buffer[msglen++] = 0x0a;

	if ((conn->TempMsg->length + msglen) > conn->MailBufferSize)
	{
		conn->MailBufferSize += 10000;
		conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
		if (conn->MailBuffer == NULL)
		{
			nodeprintf(conn, "Failed to extend Message Buffer\r");

			conn->Flags &= ~GETTINGMESSAGE;
			return;
		}
	}

	memcpy(&conn->MailBuffer[conn->TempMsg->length], Buffer, msglen);

	conn->TempMsg->length += msglen;
}

VOID CreateMessageFromBuffer(CIRCUIT * conn)
{
	struct MsgInfo * Msg;
	BIDRec * BIDRec;
	char * ptr1, * ptr2 = NULL;
	int FWDCount;
	char OldMess[] = "\r\n\r\nOriginal Message:\r\n\r\n";
	struct _EXCEPTION_POINTERS exinfo;
	int Age;

	// If doing SC, Append Old Message

	if (conn->CopyBuffer)
	{
		if ((conn->TempMsg->length + (int) strlen(conn->CopyBuffer) + 80 )> conn->MailBufferSize)
		{
			conn->MailBufferSize += strlen(conn->CopyBuffer) + 80;
			conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
			if (conn->MailBuffer == NULL)
			{
				nodeprintf(conn, "Failed to extend Message Buffer\r");

				conn->Flags &= ~GETTINGMESSAGE;
				return;
			}
		}

		memcpy(&conn->MailBuffer[conn->TempMsg->length], OldMess, strlen(OldMess));

		conn->TempMsg->length += strlen(OldMess);

		memcpy(&conn->MailBuffer[conn->TempMsg->length], conn->CopyBuffer, strlen(conn->CopyBuffer));

		conn->TempMsg->length += strlen(conn->CopyBuffer);

		free(conn->CopyBuffer);
		conn->CopyBuffer = NULL;
	}

		// Allocate a message Record slot

		Msg = AllocateMsgRecord();
		memcpy(Msg, conn->TempMsg, sizeof(struct MsgInfo));

		free(conn->TempMsg);

		// Set number here so they remain in sequence
		
		GetSemaphore(&MsgNoSemaphore);
		Msg->number = ++LatestMsg;
		FreeSemaphore(&MsgNoSemaphore);

		// Create BID if non supplied

		if (Msg->bid[0] == 0)
			sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

		// if message body had R: lines, get date created from last (not very accurate, but best we can do)

		ptr1 = conn->MailBuffer;

nextline:

		if (memcmp(ptr1, "R:", 2) == 0)
		{
			// see if another

			ptr2 = ptr1;			// save
			ptr1 = strchr(ptr1, '\r');
			ptr1++;
			if (*ptr1 == '\n') ptr1++;

			goto nextline;
		}

		// ptr2 points to last R: line (if any)

		if (ptr2)
		{
			struct tm rtime;
			time_t result;

			memset(&rtime, 0, sizeof(struct tm));

			if (ptr2[10] == '/')
			{
				// Dodgy 4 char year
			
				sscanf(&ptr2[2], "%04d%02d%02d/%02d%02d",
					&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min);
				rtime.tm_year -= 1900
					;
			}
			else
			{
				sscanf(&ptr2[2], "%02d%02d%02d/%02d%02d",
					&rtime.tm_year, &rtime.tm_mon, &rtime.tm_mday, &rtime.tm_hour, &rtime.tm_min);

				rtime.tm_year += 100;
			}

			rtime.tm_mon--;

//			result = _mkgmtime(&rtime);

			if ((result = _mkgmtime(&rtime)) != (time_t)-1 )
			{
				Msg->datecreated =  result;	
				Age = (time(NULL) - result)/86400;
				if (Age > BidLifetime)
					Msg->status = 'K';
				else
					GetWPInfoFromRLine(Msg->from, ptr2, result);
			}

			if (Msg->status != 'K' && strcmp(Msg->to, "WP") == 0)
				ProcessWPMsg(conn->MailBuffer, Msg->length, ptr2);

		}

		CreateMessageFile(conn, Msg);

		BIDRec = AllocateBIDRecord();

		strcpy(BIDRec->BID, Msg->bid);
		BIDRec->mode = Msg->type;
		BIDRec->u.msgno = LOWORD(Msg->number);
		BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

		if (Msg->length > MaxTXSize)
		{
			Msg->status = 'H';

			if (!(conn->BBSFlags & BBS))
				nodeprintf(conn, "*** Warning Message length exceeds sysop-defined maximum of %d - Message will be held\r", MaxTXSize);
		}

		// Set up forwarding bitmap

		FWDCount = MatchMessagetoBBSList(Msg);

		if (!(conn->BBSFlags & BBS))
		{
			nodeprintf(conn, "Message: %d Bid:  %s Size: %d\r", Msg->number, Msg->bid, Msg->length);

			if (Msg->via[0])
			{	
				if (FWDCount ==  0 &&  Msg->to[0] != 0)		// unless smtp msg
					nodeprintf(conn, "@BBS specified, but no forwarding info is available - msg may not be delivered\r");
			}
			else
			{
				if (FWDCount ==  0 && conn->LocalMsg == 0)
					// Not Local and no forward route
					nodeprintf(conn, "Message is not for a local user, and no forwarding info is available - msg may not be delivered\r");
			}
			SendPrompt(conn, conn->UserPointer);
		}
		else
			if (!(conn->BBSFlags & FBBForwarding))
				BBSputs(conn, ">\r");
			else
				SetupNextFBBMessage(conn);
		
		if(Msg->to[0] == 0)
			SMTPMsgCreated=TRUE;

		SaveMessageDatabase();
		SaveBIDDatabase();

		__try
		{
			SendMsgUI(Msg);
		}
		My__except_Routine("SendMsgUI");

		return;
}

VOID CreateMessageFile(ConnectionInfo * conn, struct MsgInfo * Msg)
{
	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	int WriteLen=0;
	char Mess[255];
	int len;

	sprintf_s(MsgFile, sizeof(MsgFile), "%s\\m_%06d.mes", MailDir, Msg->number);
	
	hFile = CreateFile(MsgFile,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


	if (hFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, conn->MailBuffer, Msg->length, &WriteLen, NULL);
		CloseHandle(hFile);
	}

	free(conn->MailBuffer);
	conn->MailBufferSize=0;
	conn->MailBuffer=0;

	if (WriteLen != Msg->length)
	{
		len = sprintf_s(Mess, sizeof(Mess), "Failed to create Message File\r");
		QueueMsg(conn, Mess, len);
		CriticalErrorHandler(Mess);
	}
	return;
}


void chat_link_out (LINK *link)
{
	int n, p;
	CIRCUIT * conn;
	char Msg[80];

	for (n = NumberofStreams-1; n >= 0 ; n--)
	{
		conn = &Connections[n];
		
		if (conn->Active == FALSE)
		{
			p = conn->BPQStream;
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->BPQStream = p;

			conn->Active = TRUE;
			circuit_new(conn,p_linkini);
			conn->u.link = link;
			conn->Flags = CHATMODE | CHATLINK;

			ConnectUsingAppl(conn->BPQStream, ChatApplMask);

			n=sprintf_s(Msg, sizeof(Msg), "Connecting to Chat Node %s", conn->u.link->alias);

			WriteLogLine('|',Msg, n, LOG_CHAT);
	
			//	Connected Event will trigger connect to remote system

			return;
		}
	}

	return;
	

}

ProcessConnecting(CIRCUIT * circuit, char * Buffer, int Len)
{
	WriteLogLine('<',Buffer, Len-1, LOG_CHAT);

	Buffer = _strupr(Buffer);

	if (memcmp(Buffer, "OK\r", 3) == 0)
	{
		circuit->u.link->flags = p_linked;
 	  	circuit->flags = p_linked;
		state_tell(circuit);
		return TRUE;
	}

	
	if (strstr(Buffer, "CONNECTED") || strstr(Buffer, "LINKED"))
	{
		// Connected - Send *RTL 

		nputs(circuit, "*RTL\r");  // Log in to the remote RT system.
		return TRUE;

	}

	if (strstr(Buffer, "BUSY") || strstr(Buffer, "FAILURE") || strstr(Buffer, "DOWNLINK")|| strstr(Buffer, "SORRY"))
	{
		link_drop(circuit);
		Disconnect(circuit->BPQStream);
	}
	
	return FALSE;

}

VOID SetupFwdTimes(struct BBSForwardingInfo * ForwardingInfo)
{
	char ** Times = ForwardingInfo->FWDTimes;
	int Start, End;
	int Count = 0;

	ForwardingInfo->FWDBands = zalloc(sizeof(struct FWDBAND));

	if (Times)
	{
		while(Times[0])
		{
			ForwardingInfo->FWDBands = realloc(ForwardingInfo->FWDBands, (Count+2)* sizeof(struct FWDBAND));
			ForwardingInfo->FWDBands[Count] = zalloc(sizeof(struct FWDBAND));

			Start = atoi(Times[0]);
			End = atoi(&Times[0][5]);

			ForwardingInfo->FWDBands[Count]->FWDStartBand =  (time_t)(Start / 100) * 3600 + (Start % 100) * 60; 
			ForwardingInfo->FWDBands[Count]->FWDEndBand =  (time_t)(End / 100) * 3600 + (End % 100) * 60 + 59; 

			Count++;
			Times++;
		}
		ForwardingInfo->FWDBands[Count] = NULL;
	}
}

VOID SetupForwardingStruct(struct UserInfo * user)
{
	struct	BBSForwardingInfo * ForwardingInfo;
	HKEY hKey=0;
	int retCode,Type,Vallen;
	char Key[100] =  "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\BBSForwarding\\";
	int m;
	struct MsgInfo * Msg;

	ForwardingInfo = user->ForwardingInfo = zalloc(sizeof(struct BBSForwardingInfo));

	strcat(Key, user->Call);
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		ForwardingInfo->ConnectScript = GetMultiStringValue(hKey,  "Connect Script");
		ForwardingInfo->TOCalls = GetMultiStringValue(hKey,  "TOCalls");
		ForwardingInfo->ATCalls = GetMultiStringValue(hKey,  "ATCalls");
		ForwardingInfo->Haddresses = GetMultiStringValue(hKey,  "HRoutes");
		ForwardingInfo->FWDTimes = GetMultiStringValue(hKey,  "FWD Times");

		Vallen=4;
		retCode += RegQueryValueEx(hKey, "Enabled", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->Enabled,(ULONG *)&Vallen);
				
		Vallen=4;
		retCode += RegQueryValueEx(hKey, "RequestReverse", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->ReverseFlag,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey, "Use B1 Protocol", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->AllowB1,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey, "Use B2 Protocol", 0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->AllowB2,(ULONG *)&Vallen);

		Vallen=4;

		RegQueryValueEx(hKey,"FWDInterval",0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->FwdInterval,(ULONG *)&Vallen);

		RegQueryValueEx(hKey,"MaxFBBBlock",0,			
			(ULONG *)&Type,(UCHAR *)&ForwardingInfo->MaxFBBBlockSize,(ULONG *)&Vallen);

		if (ForwardingInfo->MaxFBBBlockSize == 0)
			ForwardingInfo->MaxFBBBlockSize = 10000;

		if (ForwardingInfo->FwdInterval == 0)
				ForwardingInfo->FwdInterval = 3600;

		RegCloseKey(hKey);

		// Convert FWD Times and H Addresses

		if (ForwardingInfo->FWDTimes)
			SetupFwdTimes(ForwardingInfo);

		if (ForwardingInfo->Haddresses)
			SetupHAddreses(ForwardingInfo);
	}

	for (m = FirstMessagetoForward; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		// If any forward bits are set, increment count on  BBS record.

		if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
		{
			if (Msg->type && check_fwd_bit(Msg->fbbs, user->BBSNumber))
			{
				user->ForwardingInfo->MsgCount++;
			}
		}
	}
}

VOID * GetMultiStringValue(HKEY hKey, char * ValueName)
{
	int retCode,Type,Vallen;
	char * MultiString;
	int ptr, len;
	int Count = 0;
	char ** Value;

	Value = zalloc(4);				// always NULL entry on end even if no values

	Value[0] = NULL;

	Vallen=0;

	retCode = RegQueryValueEx(hKey, ValueName, 0, (ULONG *)&Type, NULL, (ULONG *)&Vallen);

	if ((retCode != 0)  || (Vallen == 0)) 
		return FALSE;

	MultiString = malloc(Vallen);

	retCode = RegQueryValueEx(hKey, ValueName, 0,			
			(ULONG *)&Type,(UCHAR *)MultiString,(ULONG *)&Vallen);

	ptr=0;

	while (MultiString[ptr])
	{
		len=strlen(&MultiString[ptr]);

		Value = realloc(Value, (Count+2)*4);
		Value[Count++] = _strupr(_strdup(&MultiString[ptr]));
		ptr+= (len + 1);
	}

	Value[Count] = NULL;

	free(MultiString);

	return Value;
}

VOID FreeForwardingStruct(struct UserInfo * user)
{
	struct	BBSForwardingInfo * ForwardingInfo;

	ForwardingInfo = user->ForwardingInfo;

	FreeList(ForwardingInfo->TOCalls);
	FreeList(ForwardingInfo->ATCalls);
	FreeList(ForwardingInfo->Haddresses);
	FreeList(ForwardingInfo->HADDRS);
	FreeList(ForwardingInfo->ConnectScript);
	FreeList(ForwardingInfo->FWDTimes);
	FreeList((char **)ForwardingInfo->FWDBands);
}

VOID FreeList(char ** Hddr)
{
	VOID ** Save;
	
	if (Hddr)
	{
		Save = Hddr;
		while(Hddr[0])
		{
			free(Hddr[0]);
			Hddr++;
		}	
		free(Save);
	}
}


BOOL ConnecttoBBS (struct UserInfo * user)
{
	int n, p;
	CIRCUIT * conn;

	for (n = NumberofStreams-1; n >= 0 ; n--)
	{
		conn = &Connections[n];
		
		if (conn->Active == FALSE)
		{
			p = conn->BPQStream;
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->BPQStream = p;

			conn->Active = TRUE;
			strcpy(conn->Callsign, user->Call); 
			conn->BBSFlags |= RunningConnectScript;
			conn->UserPointer = user;

			ConnectUsingAppl(conn->BPQStream, BBSApplMask);

			Logprintf(LOG_BBS, '|', "Connecting to BBS %s", user->Call);

			//	Connected Event will trigger connect to remote system

			RefreshMainWindow();

			return TRUE;
		}
	}

	return FALSE;
	
}

BOOL ProcessBBSConnectScript(CIRCUIT * conn, char * Buffer, int len)
{
	struct	BBSForwardingInfo * ForwardingInfo = conn->UserPointer->ForwardingInfo;
	char ** Scripts;
	char callsign[10];
	int port, sesstype, paclen, maxframe, l4window;
	char * ptr, * ptr2;

	WriteLogLine('<',Buffer, len-1, LOG_BBS);

	Buffer[len]=0;
	_strupr(Buffer);

	if (strstr(Buffer, "BUSY") || strstr(Buffer, "FAILURE") || strstr(Buffer, "DOWNLINK") ||
		strstr(Buffer, "SORRY") || strstr(Buffer, "INVALID") || strstr(Buffer, "RETRIED"))
	{
		// Connect Failed

		Disconnect(conn->BPQStream);

		return FALSE;
	}

	// The pointer is only updated when we get the connect, so we can tell when the last line is acked
	// The first entry is always from Connected event, so don't have to worry about testing entry -1 below

	Scripts = ForwardingInfo->ConnectScript;


	if (strstr(Buffer, "CONNECTED") || strstr(Buffer, "PACLEN") || strstr(Buffer, "OK"))
	{
		ForwardingInfo->ScriptIndex++;
		
		if (Scripts[ForwardingInfo->ScriptIndex]) // If more to send, send it
			nodeprintf(conn, "%s\r", Scripts[ForwardingInfo->ScriptIndex]);

		return TRUE;
	}

	ptr = strchr(Buffer, '}');

	if (ptr && Scripts[ForwardingInfo->ScriptIndex]) // Beware it could be part of ctext
	{
		// Could be respsonse to Node Command 

		ptr+=2;
		
		ptr2 = strchr(&ptr[0], ' ');

		if (ptr2)
		{
		if (memcmp(ptr, Scripts[ForwardingInfo->ScriptIndex], ptr2-ptr) == 0)	// Reply to last sscript command
		{
			ForwardingInfo->ScriptIndex++;
		
			if (Scripts[ForwardingInfo->ScriptIndex]) // If more to send, send it
				nodeprintf(conn, "%s\r", Scripts[ForwardingInfo->ScriptIndex]);

			return TRUE;
		}
		}
	}

	// Not Success or Fail. If last line is still outstanding, wait fot Respon
	//		else look for SID or Prompt

	if (Scripts[ForwardingInfo->ScriptIndex])
		return TRUE;

	// No more steps, Look for SID or Prompt


	if (Buffer[0] == '[' && Buffer[len-2] == ']')		// SID
	{
		// Update PACLEN

		GetConnectionInfo(conn->BPQStream, callsign, &port, &sesstype, &paclen, &maxframe, &l4window);

		if (paclen > 0)
			conn->paclen = paclen;

		
		Parse_SID(conn, &Buffer[1], len-4);
			
		if (conn->BBSFlags & FBBForwarding)
		{
			conn->FBBIndex = 0;		// ready for first block;
			conn->FBBChecksum = 0;
		}

		return TRUE;
	}

	if (Buffer[len-2] == '>')
	{		
		conn->BBSFlags &= ~RunningConnectScript;

		// Only delare B1 and B2 if other end did, and we are configued for it

		nodeprintf(conn, BBSSID, Ver[0], Ver[1], Ver[2], Ver[3],
			ALLOWCOMPRESSED ? "B" : "", 
			(conn->BBSFlags & FBBB1Mode) ? "1" : "",
			(conn->BBSFlags & FBBB2Mode) ? "2" : ""); 

		conn->NextMessagetoForward = FirstMessagetoForward;

		if (conn->BBSFlags & FBBForwarding)
		{
			if (!FBBDoForward(conn))				// Send proposal if anthing to forward
				BBSputs(conn, "FF\r");

			return TRUE;
		}

		return TRUE;
	}

	return TRUE;
}

VOID Parse_SID(CIRCUIT * conn, char * SID, int len)
{
	// scan backwards for first '-'

	while (len > 0)
	{
		switch (SID[len--])
		{
		case '-':
			len=0;
			break;

		case '$':
			conn->BBSFlags |= BBS;
			conn->BBSFlags |= MBLFORWARDING;

			break;

		case 'F':			// FBB Blocked Forwarding

			conn->BBSFlags |= FBBForwarding;
			conn->BBSFlags &= ~MBLFORWARDING;

		
			// Allocate a Header Block

			conn->FBBHeaders = zalloc(5 * sizeof(struct FBBHeaderLine));
			break;

		case 'B':

			if (ALLOWCOMPRESSED)
			{
				conn->BBSFlags |= FBBCompressed;

//				if (conn->UserPointer->ForwardingInfo->AllowB1) // !!!!! Testing !!!!
//					conn->BBSFlags |= FBBB1Mode;

				
				// Look for 1 or 2 or 12 as next 2 chars

				if (SID[len+2] == '1')
				{
					conn->BBSFlags |= FBBB1Mode;

					if (SID[len+3] == '2')
						if (conn->UserPointer->ForwardingInfo->AllowB2)
							conn->BBSFlags |= FBBB1Mode | FBBB2Mode;	// B2 uses B1 mode (crc on front of file)

					break;
				}

				if (SID[len+2] == '2')
						if (conn->UserPointer->ForwardingInfo->AllowB2)
							conn->BBSFlags |= FBBB1Mode | FBBB2Mode;	// B2 uses B1 mode (crc on front of file)
	
				break;
			}

			break;
		}
	}
	return;
}
int	CriticalErrorHandler(char * error)
{
	return 0;
}

VOID FWDTimerProc()
{
	struct UserInfo * user;
	struct	BBSForwardingInfo * ForwardingInfo ;

	for (user = BBSChain; user; user = user->BBSNext)
	{
		// See if any messages are queued for this BBS

		ForwardingInfo = user->ForwardingInfo;
		ForwardingInfo->FwdTimer+=10;

		if (ForwardingInfo->FwdTimer >= ForwardingInfo->FwdInterval)
		{
			ForwardingInfo->FwdTimer=0;

			if (ForwardingInfo->FWDBands)
			{
				// Check Timebands

				struct FWDBAND ** Bands = ForwardingInfo->FWDBands;
				int Count = 0;
				time_t now = time(NULL);

				now %= 86400;		// Secs in to day

				while(Bands[Count])
				{
					if ((Bands[Count]->FWDStartBand < now) && (Bands[Count]->FWDEndBand >= now))
						goto FWD;	// In band

				Count++;
				}
				continue;				// Out of bands
			}
		FWD:	

				if (ForwardingInfo->Enabled)
					if (ForwardingInfo->ConnectScript  && (ForwardingInfo->Forwarding == 0) && ForwardingInfo->ConnectScript[0])
						if (ForwardingInfo->MsgCount || ForwardingInfo->ReverseFlag)
							if (ConnecttoBBS(user))
								ForwardingInfo->Forwarding = TRUE;

		}
	}
}

void StartForwarding(int BBSNumber)
{
	struct UserInfo * user;
	struct	BBSForwardingInfo * ForwardingInfo ;

	for (user = BBSChain; user; user = user->BBSNext)
	{
		// See if any messages are queued for this BBS

		ForwardingInfo = user->ForwardingInfo;

		if ((BBSNumber == 0) || (user->BBSNumber == BBSNumber))
			if (ForwardingInfo)
				if (ForwardingInfo->Enabled || BBSNumber)		// Menu Command overrides enable
					if (ForwardingInfo->ConnectScript  && (ForwardingInfo->Forwarding == 0) && ForwardingInfo->ConnectScript[0])
						if (ForwardingInfo->MsgCount || ForwardingInfo->ReverseFlag || BBSNumber) // Menu Command overrides Reverse
							if (ConnecttoBBS(user))
								ForwardingInfo->Forwarding = TRUE;
	}

	return;
}

// R:090209/0128Z 33040@N4JOA.#WPBFL.FL.USA.NOAM [164113] FBB7.01.35 alpha


char * DateAndTimeForHLine(time_t Datim)
{
	struct tm *newtime;
    char Time[80];
	static char Date[]="yymmdd/hhmmZ";
  
	newtime = gmtime(&Datim);
	asctime_s(Time, sizeof(Time), newtime);
	Date[0]=Time[22];
	Date[1]=Time[23];
	Date[3]=Time[4];
	Date[4]=Time[5];
	Date[5]=Time[6];
	
	return Date;
}


BOOL FindMessagestoForward (CIRCUIT * conn)
{
	// See if any messages are queued for this BBS

	int m;
	struct MsgInfo * Msg;
	struct UserInfo * user = conn->UserPointer;
	struct FBBHeaderLine * FBBHeader;
	BOOL Found = FALSE;
	char RLine[100];
	int TotalSize = 0;

	conn->FBBIndex = 0;

//	if (user->ForwardingInfo->MsgCount == 0)
//		return FALSE;

	for (m = conn->NextMessagetoForward; m <= NumberofMessages; m++)
	{
		Msg=MsgHddrPtr[m];

		if (Msg->type && check_fwd_bit(Msg->fbbs, user->BBSNumber))
		{
			// Message to be sent - do a consistancy check (State, etc)

			conn->NextMessagetoForward = m + 1;			// So we don't offer again if defered

			// if FBB forwarding add to list, eise save pointer

			if (conn->BBSFlags & FBBForwarding)
			{
				struct tm *tm;

				FBBHeader = &conn->FBBHeaders[conn->FBBIndex++];
  
				FBBHeader->FwdMsg = Msg;
				FBBHeader->MsgType = Msg->type;
				FBBHeader->Size = Msg->length;
				TotalSize += Msg->length;
				strcpy(FBBHeader->From, Msg->from);
				strcpy(FBBHeader->To, Msg->to);
				strcpy(FBBHeader->ATBBS, Msg->via);
				strcpy(FBBHeader->BID, Msg->bid);

				// Set up R:Line, so se can add its length to the sise

				tm = gmtime(&Msg->datecreated);	
	
				FBBHeader->Size += sprintf_s(RLine, sizeof(RLine),"R:%02d%02d%02d/%02d%02dZ %d@%s.%s BPQ1.0.0\r\n",
					tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
					Msg->number, BBSName, HRoute);

				// If using B2 forwarding we need the message size and Compressed size for FC proposal

				if (conn->BBSFlags & FBBB2Mode)
					CreateB2Message(FBBHeader, RLine);

				if (conn->FBBIndex == 5  || TotalSize > user->ForwardingInfo->MaxFBBBlockSize)
					return TRUE;							// Got max number or too big

				Found = TRUE;								// Remember we have some
			}
			else
			{
				conn->FwdMsg = Msg;
				return TRUE;
			}
		}
	}

	return Found;
}

int check_fwd_bit(char *mask, int bbsnumber)
{
	return (mask[(bbsnumber - 1) / 8] & (1 << ((bbsnumber - 1) % 8)));
}


void set_fwd_bit(char *mask, int bbsnumber)
{
	if (bbsnumber)
		mask[(bbsnumber - 1) / 8] |= (1 << ((bbsnumber - 1) % 8));
}


void clear_fwd_bit (char *mask, int bbsnumber)
{
	if (bbsnumber)
		mask[(bbsnumber - 1) / 8] &= (~(1 << ((bbsnumber - 1) % 8)));
}

#ifndef NEWROUTING

VOID SetupHAddreses(struct BBSForwardingInfo * ForwardingInfo)
{
}
VOID SetupMyHA()
{
}

int MatchMessagetoBBSList(struct MsgInfo * Msg)
{
	struct UserInfo * bbs;
	struct	BBSForwardingInfo * ForwardingInfo;
	char ATBBS[41];
	char * HRoute;
	int Count =0;

	strcpy(ATBBS, Msg->via);
	HRoute = strlop(ATBBS, '.');

	if (Msg->type == 'P')
	{
		// P messages are only sent to one BBS, but check the TO and AT of all BBSs before routing on HA

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSToList(Msg, bbs, ForwardingInfo))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
					ForwardingInfo->MsgCount++;
				}
				return 1;
			}
		}

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSAtList(Msg, bbs, ForwardingInfo, ATBBS))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
					ForwardingInfo->MsgCount++;
				}
				return 1;
			}
		}

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSHList(Msg, bbs, ForwardingInfo, ATBBS, HRoute))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
					ForwardingInfo->MsgCount++;
				}
				return 1;
			}
		}

		return FALSE;
	}

	// Bulls go to all matching BBSs, so the order of checking doesn't matter

	for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
	{		
		ForwardingInfo = bbs->ForwardingInfo;

		if (CheckABBS(Msg, bbs, ForwardingInfo, ATBBS, HRoute))		
		{
			if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
			{
				set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
				ForwardingInfo->MsgCount++;
			}
			Count++;
		}
	}

	return Count;
}
BOOL CheckABBS(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute)
{
	char ** Calls;
	char ** HRoutes;
	int i, j;

	if (strcmp(ATBBS, bbs->Call) == 0)					// @BBS = BBS
		return TRUE;

	// Check TO distributions

	if (ForwardingInfo->TOCalls)
	{
		Calls = ForwardingInfo->TOCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], Msg->to) == 0)	
				return TRUE;

			Calls++;
		}
	}

	// Check AT distributions

	if (ForwardingInfo->ATCalls)
	{
		Calls = ForwardingInfo->ATCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}
	if ((HRoute) &&	(ForwardingInfo->Haddresses))
	{
		// Match on Routes

		HRoutes = ForwardingInfo->Haddresses;

		while(HRoutes[0])
		{
			i = strlen(HRoutes[0]) - 1;
			j = strlen(HRoute) - 1;

			while ((i >= 0) && (j >= 0))				// Until one string rus out
			{
				if (HRoutes[0][i--] != HRoute[j--])	// Compare backwards
					goto next;
			}

			return TRUE;
		next:	
			HRoutes++;
		}
	}


	return FALSE;

}

BOOL CheckBBSToList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo)
{
	char ** Calls;

	// Check TO distributions

	if (ForwardingInfo->TOCalls)
	{
		Calls = ForwardingInfo->TOCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], Msg->to) == 0)	
				return TRUE;

			Calls++;
		}
	}
	return FALSE;
}

BOOL CheckBBSAtList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS)
{
	char ** Calls;

	// Check AT distributions

	if (strcmp(ATBBS, bbs->Call) == 0)			// @BBS = BBS
		return TRUE;

	if (ForwardingInfo->ATCalls)
	{
		Calls = ForwardingInfo->ATCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}
	return FALSE;
}

BOOL CheckBBSHList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute)
{
	char ** HRoutes;
	int i, j;

	if ((HRoute) &&	(ForwardingInfo->Haddresses))
	{
		// Match on Routes

		HRoutes = ForwardingInfo->Haddresses;

		while(HRoutes[0])
		{
			i = strlen(HRoutes[0]) - 1;
			j = strlen(HRoute) - 1;

			while ((i >= 0) && (j >= 0))				// Until one string rus out
			{
				if (HRoutes[0][i--] != HRoute[j--])	// Compare backwards
					goto next;
			}

			return TRUE;
		next:	
			HRoutes++;
		}
	}
	return FALSE;
}

#endif

