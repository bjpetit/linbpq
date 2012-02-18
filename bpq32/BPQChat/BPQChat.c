// Chat Server for BPQ32 Packet Switch
//
//
// Based on MailChat Version 1.4.48.1


#include "stdafx.h"
#include <new.h>

#define CHAT
#include "Versions.h"

#include "GetVersion.h"

#define MAX_LOADSTRING 100

BOOL WINE = FALSE;


HKEY REGTREE = HKEY_LOCAL_MACHINE;		// Default
char * REGTREETEXT = "HKEY_LOCAL_MACHINE";

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

int LastVer[4] = {0, 0, 0, 0};					// In case we need to do somthing the first time a version is run

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

struct SEM ConSemaphore = {0, 0};

struct SEM OutputSEM = {0, 0};

struct UserInfo ** UserRecPtr=NULL;
int NumberofUsers=0;

struct UserInfo * BBSChain = NULL;					// Chain of users that are BBSes

struct MsgInfo ** MsgHddrPtr=NULL;
int NumberofMessages=0;

int FirstMessageIndextoForward=0;					// Lowest Message wirh a forward bit set - limits search

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


ULONG BBSApplMask;
ULONG ChatApplMask;

int BBSApplNum=0;
int ChatApplNum=0;

//int	StartStream=0;
int	NumberofStreams=0;
int MaxStreams=0;

char ChatSID[]="[BPQChatServer-%d.%d.%d.%d]\r";

char NewUserPrompt[100]="Please enter your Name\r>\r";

char SignoffMsg[100];

char AbortedMsg[100]="\rOutput aborted\r";

char BaseDir[MAX_PATH];

char RlineVer[50];

BOOL KISSOnly = FALSE;

UCHAR * OtherNodes=NULL;
char OtherNodesList[1000];


int ProgramErrors = 0;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
ATOM				RegisterMainWindowClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ChatMapDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
unsigned long _beginthread( void( *start_address )(VOID * DParam),
				unsigned stack_size, VOID * DParam);

VOID SaveWindowConfig();
void WriteLogLine(CIRCUIT * conn, int Flag, char * Msg, int MsgLen, int Flags);
char * lookupuser(char * call);

struct _EXCEPTION_POINTERS exinfox;
	
CONTEXT ContextRecord;
EXCEPTION_RECORD ExceptionRecord;

DWORD Stack[16];

BOOL Restarting = FALSE;

Dump_Process_State(struct _EXCEPTION_POINTERS * exinfo, char * Msg)
{
	unsigned int SPPtr;
	unsigned int SPVal;

	memcpy(&ContextRecord, exinfo->ContextRecord, sizeof(ContextRecord));
	memcpy(&ExceptionRecord, exinfo->ExceptionRecord, sizeof(ExceptionRecord));
		
	SPPtr = ContextRecord.Esp;

	Debugprintf("BPQChat *** Program Error %x at %x in %s",
	ExceptionRecord.ExceptionCode, ExceptionRecord.ExceptionAddress, Msg);	


	__asm{

		mov eax, SPPtr
		mov SPVal,eax
		lea edi,Stack
		mov esi,eax
		mov ecx,64
		rep movsb

	}

	Debugprintf("EAX %x EBX %x ECX %x EDX %x ESI %x EDI %x ESP %x",
		ContextRecord.Eax, ContextRecord.Ebx, ContextRecord.Ecx,
		ContextRecord.Edx, ContextRecord.Esi, ContextRecord.Edi, SPVal);
		
	Debugprintf("Stack:");

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal, Stack[0], Stack[1], Stack[2], Stack[3], Stack[4], Stack[5], Stack[6], Stack[7]);

	Debugprintf("%08x %08x %08x %08x %08x %08x %08x %08x %08x ",
		SPVal+32, Stack[8], Stack[9], Stack[10], Stack[11], Stack[12], Stack[13], Stack[14], Stack[15]);

}



void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
	Logprintf(LOG_DEBUG, NULL, '!', "*** Error **** C Run Time Invalid Parameter Handler Called");

	if (expression && function && file)
	{
		Logprintf(LOG_DEBUG, NULL, '!', "Expression = %S", expression);
		Logprintf(LOG_DEBUG, NULL, '!', "Function %S", function);
		Logprintf(LOG_DEBUG, NULL, '!', "File %S Line %d", file, line);
	}
}

char IniFileName[MAX_PATH];
char Session[20] = "Session 1";


// If program gets too many program errors, it will restart itself  and shut down

VOID CheckProgramErrors()
{
	STARTUPINFO  SInfo;			// pointer to STARTUPINFO 
    PROCESS_INFORMATION PInfo; 	// pointer to PROCESS_INFORMATION 
	char ProgName[256];

	if (Restarting)
		exit(0);				// Make sure can't loop in restarting

	ProgramErrors++;

	if (ProgramErrors > 25)
	{
		char Command[30];
		
		Restarting = TRUE;

		Logprintf(LOG_DEBUG, NULL, '!', "Too Many Program Errors - Closing");

		if (cfgMinToTray)
		{
			DeleteTrayMenuItem(MainWnd);
			if (ConsHeader[0]->hConsole)
				DeleteTrayMenuItem(ConsHeader[0]->hConsole);
			if (ConsHeader[1]->hConsole)
				DeleteTrayMenuItem(ConsHeader[1]->hConsole);
			if (hMonitor)
				DeleteTrayMenuItem(hMonitor);
			if (hDebug)
				DeleteTrayMenuItem(hDebug);
		}

		SInfo.cb=sizeof(SInfo);
		SInfo.lpReserved=NULL; 
		SInfo.lpDesktop=NULL; 
		SInfo.lpTitle=NULL; 
		SInfo.dwFlags=0; 
		SInfo.cbReserved2=0; 
  		SInfo.lpReserved2=NULL; 

		GetModuleFileName(NULL, ProgName, 256);

		Debugprintf("Attempting to Restart %s", ProgName);

		wsprintf(Command, "WAIT %s", Session);

		CreateProcess(ProgName, Command, NULL, NULL, FALSE, 0, NULL, NULL, &SInfo, &PInfo);
					
		exit(0);
	}
}


VOID SaveIntValue(char * Key, int Value)
{
	char Val[100];

	wsprintf(Val, "%d", Value);
	WritePrivateProfileString(Session, Key, Val, IniFileName);
}

VOID SaveStringValue(char * Key, char * Value)
{
	WritePrivateProfileString(Session, Key, Value, IniFileName);
}

int GetIntValue(char * Key, int Default)
{
	return GetPrivateProfileInt(Session, Key, Default, IniFileName);
}

VOID GetStringValue(char * Key, char * Value, int Len)
{
	GetPrivateProfileString(Session, Key, "", Value, Len, IniFileName);
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;
	int BPQStream, n;
	struct _EXCEPTION_POINTERS exinfo;
	_invalid_parameter_handler oldHandler, newHandler;
	char Msg[100];
	int i = 60;
	char * ptr;

	if (strstr(lpCmdLine, "WAIT"))				// If AutoRestart then Delay 60 Secs
	{	
		hWnd = CreateWindow("STATIC", "Chat Restarting after Failure - Please Wait", 0,
		CW_USEDEFAULT, 100, 550, 70,
		NULL, NULL, hInstance, NULL);

		ShowWindow(hWnd, nCmdShow);

		while (i-- > 0)
		{
			wsprintf(Msg, "Chat Restarting after Failure - Please Wait %d secs.", i);
			SetWindowText(hWnd, Msg);
			
			Sleep(1000);
		}

		DestroyWindow(hWnd);
	}

	ptr = strstr(_strupr(lpCmdLine), "SESSION");

	if (ptr)
		strcpy (Session, ptr);


	__try {

	// Trap CRT Errors

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

	Logprintf(LOG_DEBUG, NULL, '!', "Program Starting");
	Debugprintf("BPQChat Starting");

	} My__except_Routine("Init");

	while (GetMessage(&msg, NULL, 0, 0))
	{
		__try
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		#define EXCEPTMSG "GetMessageLoop"
		#include "StdExcept.c"

		CheckProgramErrors();
		}
	}

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

	ClearChatLinkStatus();
	SendChatLinkStatus();

	hWnd = CreateWindow("STATIC", "Chat Closing - Please Wait", 0,
				150, 200, 350, 40, NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);

	Sleep(1000);				// A bit of time for links to close

	SendChatLinkStatus();		// Send again to reduce chance of being missed

	Sleep(2000);

	DestroyWindow(hWnd);

	if (ConsHeader[0]->hConsole)
		DestroyWindow(ConsHeader[0]->hConsole);
	if (ConsHeader[1]->hConsole)
		DestroyWindow(ConsHeader[1]->hConsole);
	if (hMonitor)
		DestroyWindow(hMonitor);
	if (hDebug)
		DestroyWindow(hDebug);

	if (cfgMinToTray)
	{
		DeleteTrayMenuItem(MainWnd);
		if (ConsHeader[0]->hConsole)
			DeleteTrayMenuItem(ConsHeader[0]->hConsole);
		if (ConsHeader[1]->hConsole)
			DeleteTrayMenuItem(ConsHeader[1]->hConsole);
		if (hMonitor)
			DeleteTrayMenuItem(hMonitor);
		if (hDebug)
			DeleteTrayMenuItem(hDebug);
	}


	FreeChatMemory();

	n = 0;

	_CrtDumpMemoryLeaks();

	}
	My__except_Routine("Close Processing");

	SaveWindowConfig();

	CloseBPQ32();				// Close Ext Drivers if last bpq32 process

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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(BPQICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= bgBrush;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BPQMailChat);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(BPQICON));

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
	int retCode;
	char Size[80];
	RECT InitRect;
	RECT SessRect;
	int len;
	char * ptr1;

	struct _EXCEPTION_POINTERS exinfo;

	HMODULE ExtDriver=LoadLibrary("bpq32.dll");

	// See if running under WINE

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine",  0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		WINE =TRUE;
		Debugprintf("Running under WINE");
	}

	// See if we need to warn of possible problem with BaseDir moved by installer

	wsprintf(BaseDir, "%s", GetBPQDirectory());
	
	len = strlen(BaseDir);
	ptr1 = BaseDir;

	while (*ptr1)
	{
		if (*(ptr1) == '/') *(ptr1) = '\\';
		ptr1++;
	}

	wsprintf(IniFileName, "%s\\BPQChat.ini", BaseDir);

	// Get Window Size  From Registry


	GetStringValue("WindowSize", Size, 80);		
	sscanf(Size,"%d,%d,%d,%d",&MainRect.left,&MainRect.right,&MainRect.top,&MainRect.bottom);

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

	MoveWindow(hWnd,MainRect.left,MainRect.top, MainRect.right-MainRect.left, MainRect.bottom-MainRect.top, TRUE);

	GetVersionInfo(NULL);

	wsprintf(Title,"G8BPQ Chat Server Version %s", VersionString);

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

//	hFWDMenu=GetSubMenu(hActionMenu,0);
	hMenu=GetSubMenu(hActionMenu,0);
	hDisMenu=GetSubMenu(hActionMenu,1);

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

   __try {
    
   return Initialise();

   }My__except_Routine("Initialise");

   return FALSE;
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
		if (lParam & BPQDataAvail)
		{
			//	Dont trap error at this level - let Node error handler pick it up
//			__try
//			{
				DoReceivedData(wParam);
//			}
//			My__except_Routine("DoReceivedData")
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
					if (state == 1) // Connected	
					{
						GetSemaphore(&ConSemaphore);
						__try {Connected(wParam);}
						My__except_Routine("Connected");
						FreeSemaphore(&ConSemaphore);
					}
					else
					{
						GetSemaphore(&ConSemaphore);
						__try{Disconnected(wParam);}
						My__except_Routine("Disconnected");
						FreeSemaphore(&ConSemaphore);
					}
				}
			}
			My__except_Routine("DoStateChange");

		}

		return 0;
	}


	switch (message)
	{

	case WM_KEYUP:	

		switch (wParam)
		{	
		case VK_F3:
			CreateConsole(-2);
			return 0;

		case VK_F4:
			CreateMonitor();
			return 0;

		case VK_F5:
			CreateDebugWindow();
			return 0;

		case VK_TAB:
			return TRUE;

		break;



		}
		return 0;
 			
	case WM_TIMER:

		if (wParam == 1)		// Slow = 10 secs
		{
			__try
			{
				RefreshMainWindow();
				CheckTimer();
				ChatTimer();
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

		if (wParam == (WPARAM)hActionMenu)
		{
			if (IsClipboardFormatAvailable(CF_TEXT))
				EnableMenuItem(hActionMenu,ID_ACTIONS_SENDMSGFROMCLIPBOARD, MF_BYCOMMAND | MF_ENABLED);
			else
				EnableMenuItem(hActionMenu,ID_ACTIONS_SENDMSGFROMCLIPBOARD, MF_BYCOMMAND | MF_GRAYED );
			
			return TRUE;
		}

		if (wParam == (WPARAM)hDisMenu)
		{
			// Set up Disconnect Menu

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

		if (wmEvent == LBN_DBLCLK)

				break;

		if (wmId >= IDM_DISCONNECT && wmId < IDM_DISCONNECT+MaxSockets+1)
		{
			// disconnect user

			conn=&Connections[wmId-IDM_DISCONNECT];
		
			if (conn->Active)
			{	
				Disconnect(conn->BPQStream);
			}
		}


		switch (wmId)
		{

		case IDM_LOGCHAT:

			ToggleParam(hMenu, hWnd, &LogCHAT, IDM_LOGCHAT);
			break;


		case IDM_CHATCONSOLE:

			CreateConsole(-2);
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

		case ID_HELP_ONLINEHELP:

			ShellExecute(hWnd,"open",
				"http://www.cantab.net/users/john.wiseman/Documents/BPQ%20Mail%20and%20Chat%20Server.htm",
				"", NULL, SW_SHOWNORMAL); 
		
			break;

		case IDM_CONFIG:
			DialogBox(hInst, MAKEINTRESOURCE(CHAT_CONFIG), hWnd, ConfigWndProc);
			break;

		case ID_ACTIONS_UPDATECHATMAPINFO:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_UPDATECHATMAP), hWnd, ChatMapDialogProc);
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
		if (ConsHeader[0]->hConsole)
			GetWindowRect(ConsHeader[0]->hConsole, &ConsHeader[0]->ConsoleRect);	// For save soutine
		if (ConsHeader[1]->hConsole)
			GetWindowRect(ConsHeader[1]->hConsole, &ConsHeader[1]->ConsoleRect);	// For save soutine
		if (hMonitor)
			GetWindowRect(hMonitor,	&MonitorRect);	// For save soutine
		if (hDebug)
			GetWindowRect(hDebug,	&DebugRect);	// For save soutine

		KillTimer(hWnd,1);
		KillTimer(hWnd,2);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

INT_PTR CALLBACK ChatMapDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char Position[81] = "";
	char PopupText[251] = "";
	char Msg[500];
	int len;
	int Click = 0, Hover = 0;
	char ClickHover[3] = "";

	switch (message)
	{
	case WM_INITDIALOG:

		GetStringValue("MapPosition", Position, 80);
		GetStringValue("MapPopup", PopupText, 250);
		Click = GetIntValue("PopupMode", 0);

		SetDlgItemText(hDlg, IDC_MAPPOSITION, Position);
		SetDlgItemText(hDlg, IDC_POPUPTEXT, PopupText);

		SendDlgItemMessage(hDlg, IDC_CLICK, BM_SETCHECK, Click , Click);
		SendDlgItemMessage(hDlg, IDC_HOVER, BM_SETCHECK, !Click , !Click);

		return TRUE; 

	case WM_COMMAND:

		switch LOWORD(wParam)
		{
		case IDSENDTOMAP:

			GetDlgItemText(hDlg, IDC_MAPPOSITION, Position, 80);
			GetDlgItemText(hDlg, IDC_POPUPTEXT, PopupText, 250);
		
			Click = SendDlgItemMessage(hDlg, IDC_CLICK, BM_GETCHECK, 0 ,0);
			Hover = SendDlgItemMessage(hDlg, IDC_HOVER, BM_GETCHECK, 0 ,0);
		
//			if (AXIPPort)
			{
				if (Click) ClickHover[0] = '1';
				else 
					if (Hover) ClickHover[0] = '0';

				len = wsprintf(Msg, "INFO %s|%s|%s|\r", Position, PopupText, ClickHover);

				if (len < 256)
					Send_MON_Datagram(Msg, len);
			}
			
		break;

		case IDOK:

			GetDlgItemText(hDlg, IDC_MAPPOSITION, Position, 80);
			GetDlgItemText(hDlg, IDC_POPUPTEXT, PopupText, 250);
		
			Click = SendDlgItemMessage(hDlg, IDC_CLICK, BM_GETCHECK, 0 ,0);
			Hover = SendDlgItemMessage(hDlg, IDC_HOVER, BM_GETCHECK, 0 ,0);

			SaveStringValue("MapPosition", Position);
			SaveStringValue("MapPopup", PopupText);
			SaveIntValue("PopupMode", Click);
		
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		
		case IDC_MAPHELP:
			
			ShellExecute(hDlg,"open",
				"http://www.cantab.net/users/john.wiseman/Documents/BPQChatMap.htm",
				"", NULL, SW_SHOWNORMAL); 

			return TRUE;
		}
	}
	return FALSE;
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
						"CHAT", conn->OutputQueueLength - conn->OutputGetPointer);
				}
			}
		}
		SendDlgItemMessage(MainWnd,100,LB_ADDSTRING,0,(LPARAM)msg);
	}

	SetDlgItemInt(hWnd, IDC_MSGS, NumberofMessages, FALSE);

	n = 0;


	SetDlgItemInt(hWnd, IDC_SYSOPMSGS, SYSOPMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_HELD, HeldMsgs, FALSE);
	SetDlgItemInt(hWnd, IDC_SMTP, SMTPMsgs, FALSE);

	SetDlgItemInt(hWnd, IDC_CHATSEM, ChatSemaphore.Clashes, FALSE);

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
	int i, len;
	ConnectionInfo * conn;
	struct UserInfo * user = NULL;
	HKEY hKey=0;
	char * ptr1;
	char ProgramDir[MAX_PATH];

	//	Register message for posting by BPQDLL

	BPQMsg = RegisterWindowMessage(BPQWinMsg);
	wsprintf(ProgramDir, "%s\\BPQMailChat", GetProgramDirectory());
	
	len = strlen(ProgramDir);
	ptr1 = ProgramDir;

	while (*ptr1)
	{
		if (*(ptr1) == '/') *(ptr1) = '\\';
		ptr1++;
	}

	len = strlen(BaseDir);
	ptr1 = BaseDir;

	while (*ptr1)
	{
		if (*(ptr1) == '/') *(ptr1) = '\\';
		ptr1++;
	}

	// Set up file and directory names
		
	strcpy(RtUsr, BaseDir);
	strcat(RtUsr, "\\ChatUsers.txt");

	strcpy(RtUsrTemp, BaseDir);
	strcat(RtUsrTemp, "\\ChatUsers.tmp");

	strcpy(RtKnown, BaseDir);
	strcat(RtKnown, "\\RTKnown.txt");

Retry:

	ChatApplNum = GetIntValue("ApplNum", 0);
	MaxStreams = GetIntValue("MaxStreams", 0);

	GetStringValue("OtherChatNodes", OtherNodesList, 999);

	if (ChatApplNum)
	{
		char * ptr1 = GetApplCall(ChatApplNum);
		char * ptr2;

		if (*ptr1 > 0x20)
		{
			char * Context;
			
			memcpy(OurNode, ptr1, 10);
			strlop(OurNode, ' ');
			strcpy(SYSOPCall, OurNode);
			strlop(SYSOPCall, '-');

			ptr1=GetApplAlias(ChatApplNum);
			memcpy(OurAlias, ptr1,10);
			strlop(OurAlias, ' ');

			wsprintf(SignoffMsg, "73 de %s\r", SYSOPCall);

			ChatApplMask = 1<<(ChatApplNum-1);
		
			// Set up other nodes list. rtlink messes with the string so pass copy
	
			ptr2 = ptr1 = strtok_s(_strdup(OtherNodesList), " ,\r", &Context);

			while (ptr1)
			{
				rtlink(ptr1);			
				ptr1 = strtok_s(NULL, " ,\r", &Context);
			}

			free(ptr2);

			SetupChat();
		}
	}
	else
	{
		DialogBox(hInst, MAKEINTRESOURCE(CHAT_CONFIG), hWnd, ConfigWndProc);
		goto Retry;
	}

	// Allocate Streams

	for (i=0; i < MaxStreams; i++)
	{
		conn = &Connections[i];
		conn->BPQStream = FindFreeStream();

		if (conn->BPQStream == 255) break;

		NumberofStreams++;

		BPQSetHandle(conn->BPQStream, hWnd);

		SetAppl(conn->BPQStream, 2, ChatApplMask);
		Disconnect(conn->BPQStream);
	}


	if (cfgMinToTray)
	{
		AddTrayMenuItem(MainWnd, "Chat Server");
	}
	
	SetTimer(hWnd,1,10000,NULL);	// Slow Timer (10 Secs)
	SetTimer(hWnd,2,100,NULL);		// Send to Node (100 ms)

	CheckMenuItem(hMenu,IDM_LOGCHAT, (LogCHAT) ? MF_CHECKED : MF_UNCHECKED);

	RefreshMainWindow();

	return TRUE;
}

int Connected(Stream)
{
	int n;
	CIRCUIT * conn;
	struct UserInfo * user = NULL;
	char callsign[10];
	int port, paclen, maxframe, l4window;
	char ConnectedMsg[] = "*** CONNECTED    ";
	char Msg[100];
	LINK    *link;
	KNOWNNODE *node;

	for (n = 0; n < NumberofStreams; n++)
	{
  		conn = &Connections[n];
		
		if (Stream == conn->BPQStream)
		{
			if (conn->Active)
			{
				// Probably an outgoing connect
		
				if (conn->rtcflags == p_linkini)
				{
					conn->paclen = 236;
					nprintf(conn, "c %s\r", conn->u.link->call);
					return 0;
				}
			}
	
			memset(conn, 0, sizeof(ConnectionInfo));		// Clear everything
			conn->Active = TRUE;
			conn->BPQStream = Stream;

			conn->Secure_Session = GetConnectionInfo(Stream, callsign,
				&port, &conn->SessType, &paclen, &maxframe, &l4window);

			conn->paclen = paclen;

			strlop(callsign, ' ');		// Remove trailing spaces

			memcpy(conn->Callsign, callsign, 10);

			strlop(callsign, '-');		// Remove any SSID

			user = zalloc(sizeof(struct UserInfo));

			strcpy(user->Call, callsign);

			conn->UserPointer = user;

			n=sprintf_s(Msg, sizeof(Msg), "Incoming Connect from %s", user->Call);
			
			// Send SID and Prompt

			WriteLogLine(conn, '|',Msg, n, LOG_CHAT);
			conn->Flags |= CHATMODE;

			nodeprintf(conn, ChatSID, Ver[0], Ver[1], Ver[2], Ver[3]);

			// See if from a defined node
				
			for (link = link_hd; link; link = link->next)
			{
				if (matchi(conn->Callsign, link->call))
				{
					conn->rtcflags = p_linkwait;
					return 0;						// Wait for *RTL
				}
			}

			// See if from a previously known node

			node = knownnode_find(conn->Callsign);

			if (node)
			{
				// A node is trying to link, but we don't have it defined - close

				Logprintf(LOG_CHAT, conn, '!', "Node %s connected, but is not defined as a Node - closing",
					conn->Callsign);

				nodeprintf(conn, "Node %s does not have %s defined as a node to link to - closing.\r",
					OurNode, conn->Callsign);

				Flush(conn);

				Sleep(500);

				Disconnect(conn->BPQStream);

				return 0;
			}

			if (user->Name[0] == 0)
			{
				char * Name = lookupuser(user->Call);

				if (Name)
				{
					if (strlen(Name) > 17)
						Name[17] = 0;

					strcpy(user->Name, Name);
					free(Name);
				}
				else
				{
					conn->Flags |= GETTINGUSER;
					BBSputs(conn, NewUserPrompt);
					return TRUE;
				}
			}

			SendWelcomeMsg(Stream, conn, user);
			RefreshMainWindow();
			Flush(conn);
			
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
			{
				return 0;
			}

			ClearQueue(conn);

			conn->Active = FALSE;
			RefreshMainWindow();

			if (conn->Flags & CHATMODE)
			{
				if (conn->Flags & CHATLINK)
				{
					len=sprintf_s(Msg, sizeof(Msg), "Chat Node %s Disconnected", conn->u.link->call);
					WriteLogLine(conn, '|',Msg, len, LOG_CHAT);
					__try {link_drop(conn);} My__except_Routine("link_drop");
				}
				else
				{
					len=sprintf_s(Msg, sizeof(Msg), "Chat User %s Disconnected", conn->Callsign);
					WriteLogLine(conn, '|',Msg, len, LOG_CHAT);
					__try
					{
						logout(conn);
					}
					#define EXCEPTMSG "logout"
					#include "StdExcept.c"
					}
				}

				conn->Flags = 0;
				conn->u.link = NULL;
				conn->UserPointer = NULL;	
				return 0;
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
	char Buffer[10000];
	int Written;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];

		if (Stream == conn->BPQStream)
		{
			do
			{ 
				// May have several messages per packet, or message split over packets

				if (conn->InputLen + 1000 > 10000)	// Shouldnt have lines longer  than this in text mode
					conn->InputLen = 0;				// discard	
				
				GetMsg(Stream, &conn->InputBuffer[conn->InputLen], &InputLen, &count);

				if (InputLen == 0) return 0;

				if (conn->DebugHandle)				// Receiving a Compressed Message
					WriteFile(conn->DebugHandle, &conn->InputBuffer[conn->InputLen],
						InputLen, &Written, NULL);

				conn->Watchdog = 900;				// 15 Minutes

				conn->InputLen += InputLen;

				{

			loop:

				if (conn->InputLen == 1 && conn->InputBuffer[0] == 0)		// Single Null
				{
					conn->InputLen = 0;
					return 0;
				}

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
							if (conn->rtcflags == p_linkini)		// Chat Connect
								ProcessConnecting(conn, conn->InputBuffer, conn->InputLen);
							else
								ProcessLine(conn, user, conn->InputBuffer, conn->InputLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							conn->InputBuffer[conn->InputLen] = 0;
							Debugprintf("CHAT *** Program Error Processing input %s ", conn->InputBuffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							CheckProgramErrors();
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
							if (conn->rtcflags == p_linkini)
								ProcessConnecting(conn, Buffer, MsgLen);
							else
								ProcessLine(conn, user, Buffer, MsgLen);
						}
						__except(EXCEPTION_EXECUTE_HANDLER)
						{
							Buffer[MsgLen] = 0;
							Debugprintf("CHAT *** Program Error Processing input %s ", Buffer);
							Disconnect(conn->BPQStream);
							conn->InputLen=0;
							CheckProgramErrors();
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


VOID SendWelcomeMsg(int Stream, ConnectionInfo * conn, struct UserInfo * user)
{
		if (!rtloginu (conn, TRUE))
		{
			// Already connected - close
			
			Flush(conn);
			Sleep(1000);
			Disconnect(conn->BPQStream);
		}
		return;

}

VOID SendPrompt(ConnectionInfo * conn, struct UserInfo * user)
{
	nodeprintf(conn, "de %s>\r", OurNode);
}

VOID ProcessLine(CIRCUIT * conn, struct UserInfo * user, char* Buffer, int len)
{
	char seps[] = " \t\r";
	struct _EXCEPTION_POINTERS exinfo;

	if (conn->Flags & GETTINGUSER)
	{
		conn->Flags &=  ~GETTINGUSER;

		memcpy(user->Name, Buffer, len-1);
		SendWelcomeMsg(conn->BPQStream, conn, user);

		return;
	}

	{
		GetSemaphore(&ChatSemaphore);

		__try 
		{
			ProcessChatLine(conn, user, Buffer, len);
		}
			#define EXCEPTMSG "ProcessChatLine"
			#include "StdExcept.c"

			FreeSemaphore(&ChatSemaphore);
	
			if (conn->BPQStream <  0)
				CloseConsole(conn->BPQStream);
			else
				Disconnect(conn->BPQStream);	

			return;
		}
		FreeSemaphore(&ChatSemaphore);
		return;
	}

	//	Send if possible

	Flush(conn);
}


VOID SendUnbuffered(int stream, char * msg, int len)
{
	if (stream < 0)
		WritetoConsoleWindow(stream, msg, len);
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

	GetSemaphore(&OutputSEM);

	memcpy(&conn->OutputQueue[conn->OutputQueueLength], msg, len);
	conn->OutputQueueLength += len;

	FreeSemaphore(&OutputSEM);

	return len;
}

void TrytoSend()
{
	// call Flush on any connected streams with queued data

	ConnectionInfo * conn;
	struct ConsoleInfo * Cons;

	int n;

	for (n = 0; n < NumberofStreams; n++)
	{
		conn = &Connections[n];
		
		if (conn->Active == TRUE)
			Flush(conn);
	}

	for (Cons = ConsHeader[0]; Cons; Cons = Cons->next)
	{
		if (Cons->Console)
			Flush(Cons->Console);
	}
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
	{
		// Nothing to send. If Close after Flush is set, disconnect

		if (conn->CloseAfterFlush)
		{
			conn->CloseAfterFlush--;
			
			if (conn->CloseAfterFlush)
				return;

			Disconnect(conn->BPQStream);
		}

		return;						// Nothing to send
	}
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

		GetSemaphore(&OutputSEM);

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
					FreeSemaphore(&OutputSEM);
					return;

				}
				ptr2 = memchr(ptr2, 0x0d, lenleft);
			}
		}

		SendUnbuffered(conn->BPQStream, &conn->OutputQueue[conn->OutputGetPointer], len);

		conn->OutputGetPointer+=len;

		FreeSemaphore(&OutputSEM);

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
	GetSemaphore(&OutputSEM);

	conn->OutputGetPointer=0;
	conn->OutputQueueLength=0;

	FreeSemaphore(&OutputSEM);
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

/*
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
*/
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

			n=sprintf_s(Msg, sizeof(Msg), "Connecting to Chat Node %s", conn->u.link->alias);

			strcpy(conn->Callsign, conn->u.link->alias);

			WriteLogLine(conn, '|',Msg, n, LOG_CHAT);

			ConnectUsingAppl(conn->BPQStream, ChatApplMask);

			//	Connected Event will trigger connect to remote system

			return;
		}
	}

	return;
	

}

ProcessConnecting(CIRCUIT * circuit, char * Buffer, int Len)
{
	WriteLogLine(circuit, '<' ,Buffer, Len-1, LOG_CHAT);

	Buffer = _strupr(Buffer);

	if (memcmp(Buffer, "[BPQCHATSERVER-", 15) == 0)
	{
		char * ptr = strchr(Buffer, ']');
		if (ptr)
		{
			*ptr = 0;
			strcpy(circuit->FBBReplyChars, &Buffer[15]);
		}
		else
			circuit->FBBReplyChars[0] = 0;

		return 0;
	}

	if (memcmp(Buffer, "OK", 2) == 0)
	{
		// Make sure node isn't known. There is a window here that could cause a loop

		if (node_find(circuit->u.link->call))
		{
			Logprintf(LOG_CHAT, circuit, '|', "Dropping link with %s to prevent a loop", circuit->Callsign);
			Disconnect(circuit->BPQStream);
			return FALSE;
		}

		circuit->u.link->flags = p_linked;
 	  	circuit->rtcflags = p_linked;
		state_tell(circuit, circuit->FBBReplyChars);
		NeedStatus = TRUE;

		return TRUE;
	}

	
	if (strstr(Buffer, "CONNECTED") || strstr(Buffer, "LINKED"))
	{
		// Connected - Send *RTL 
		
		nputs(circuit, "*RTL\r");  // Log in to the remote RT system.
		nprintf(circuit, "%c%c%s %s %s\r", FORMAT, id_keepalive, OurNode, circuit->u.link->call, Verstring);

		return TRUE;

	}

	if (strstr(Buffer, "BUSY") || strstr(Buffer, "FAILURE") || strstr(Buffer, "DOWNLINK")|| strstr(Buffer, "SORRY"))
	{
		link_drop(circuit);
		Disconnect(circuit->BPQStream);
	}
	
	return FALSE;

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


int	CriticalErrorHandler(char * error)
{
	Debugprintf("Critical Error %s", error);
	ProgramErrors = 25;
	CheckProgramErrors();				// Force close
	return 0;
}


VOID SaveWindowConfig()
{
	char Size[80];

		wsprintf(Size,"%d,%d,%d,%d",MonitorRect.left,MonitorRect.right,MonitorRect.top,MonitorRect.bottom);
		SaveStringValue("MonitorSize", Size);

		wsprintf(Size,"%d,%d,%d,%d",DebugRect.left,DebugRect.right,DebugRect.top,DebugRect.bottom);
		SaveStringValue("DebugSize", Size);

		wsprintf(Size,"%d,%d,%d,%d",MainRect.left,MainRect.right,MainRect.top,MainRect.bottom);
		SaveStringValue("WindowSize", Size);

//		retCode = RegSetValueEx(hKey,"Log_BBS",0,REG_DWORD,(BYTE *)&LogBBS,4);
//		retCode = RegSetValueEx(hKey,"Log_TCP",0,REG_DWORD,(BYTE *)&LogTCP,4);
//		retCode = RegSetValueEx(hKey,"Log_CHAT",0,REG_DWORD,(BYTE *)&LogCHAT,4);

		wsprintf(Size,"%d,%d,%d,%d", Ver[0], Ver[1], Ver[2], Ver[3]);
		SaveStringValue("Version", Size);

		if (ConsHeader[1]->ConsoleRect.right)
		{
			wsprintf(Size,"%d,%d,%d,%d",ConsHeader[1]->ConsoleRect.left, ConsHeader[1]->ConsoleRect.right,
				ConsHeader[1]->ConsoleRect.top, ConsHeader[1]->ConsoleRect.bottom);

			SaveStringValue("ConsoleSize", Size);
		}

	return;
}

char InfoBoxText[100];


INT_PTR CALLBACK InfoDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command;
		
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, 5050, InfoBoxText);

		return (INT_PTR)TRUE;

	case WM_COMMAND:

		Command = LOWORD(wParam);

		switch (Command)
		{
		case 0:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		}
		break;
	}
	
	return (INT_PTR)FALSE;
}


GetMultiLineDialog(HWND hDialog, int DLGItem)
{
	char Text[10000];
	char * ptr1, * ptr2;

	GetDlgItemText(hDialog, DLGItem, Text, 10000);

	// replace crlf with single space

	if (Text[strlen(Text)-1] != '\n')			// no terminating crlf?
		strcat(Text, "\r\n");

	ptr1 = Text;
	ptr2 = OtherNodesList;
		
	while (*ptr1)
	{
		if (*ptr1 == '\r')
		{
			while (*(ptr1+2) == '\r')			// Blank line
				ptr1+=2;

			*++ptr1 = 32;
		}
		*ptr2++=*ptr1++;
	}

	*ptr2++ = 0;

	return TRUE;
}



VOID SaveChatConfig(HWND hDlg)
{
	BOOL OK1;
	HKEY hKey=0;
	int OldChatAppl;
	char * ptr1, * ptr2;
	char * Context;

	OldChatAppl = ChatApplNum;
	
	ChatApplNum = GetDlgItemInt(hDlg, ID_CHATAPPL, &OK1, FALSE);
	MaxStreams = GetDlgItemInt(hDlg, ID_STREAMS, &OK1, FALSE);

	if (ChatApplNum)	
	{
		ptr1=GetApplCall(ChatApplNum);

		if (ptr1 && (*ptr1 < 0x21))
		{
			MessageBox(NULL, "WARNING - There is no APPLCALL in BPQCFG matching the confgured ChatApplNum. Chat will not work",
				"BPQMailChat", MB_ICONINFORMATION);
		}
	}

	SaveIntValue("ApplNum", ChatApplNum);
	SaveIntValue("MaxStreams", MaxStreams);

	GetMultiLineDialog(hDlg, IDC_ChatNodes);

	SaveStringValue("OtherChatNodes", OtherNodesList);
	
	// Show dialog box now - gives time for links to close
	
	// reinitialise other nodes list. rtlink messes with the string so pass copy

	node_close();

	if (ChatApplNum == OldChatAppl)
		wsprintf(InfoBoxText, "Configuration Changes Saved and Applied");
	else
	{
		ChatApplNum = OldChatAppl;
		wsprintf(InfoBoxText, "Warning Program must be restarted to change Chat Appl Num");
	}
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	removelinks();
	 
	// Set up other nodes list. rtlink messes with the string so pass copy
	
	ptr2 = ptr1 = strtok_s(_strdup(OtherNodesList), " ,\r", &Context);

	while (ptr1)
	{
		rtlink(ptr1);			
		ptr1 = strtok_s(NULL, " ,\r", &Context);
	}

	free(ptr2);

	if (user_hd)			// Any Users?
		makelinks();		// Bring up links
}

INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	This processes messages from controls on the tab subpages
	int Command;
	char Text[10000] = "";
	char * ptr1, *ptr2, *Context;

	switch (message)
	{

	case WM_INITDIALOG:

		ptr2 = ptr1 = strtok_s(_strdup(OtherNodesList), " ,\r", &Context);

		while (ptr1)
		{
			strcat(Text, ptr1);	
			strcat(Text, "\r\n");	
			ptr1 = strtok_s(NULL, " ,\r", &Context);
		}

		free(ptr2);

		SetDlgItemInt(hDlg, ID_CHATAPPL, ChatApplNum, FALSE);
		SetDlgItemInt(hDlg, ID_STREAMS, MaxStreams, FALSE);
		SetDlgItemText(hDlg, ID_CHATNODES, Text);

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

		Command = LOWORD(wParam);

		if (Command == 2002)
			return TRUE;

		switch (Command)
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case IDC_CHATSAVE:
			
			SaveChatConfig(hDlg);
			return TRUE;
		}
		break;

	}	
	return (INT_PTR)FALSE;
}






