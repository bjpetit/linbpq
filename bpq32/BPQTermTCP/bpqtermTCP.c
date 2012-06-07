
// Version 2.0.1 November 2007

// Change resizing algorith


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

// Version 1.0.4.1 June 2012

//	Add UIOnly Monitor Option

#define _CRT_SECURE_NO_DEPRECATE

#include "winsock2.h"
#include "WS2tcpip.h"
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>
//#define _RICHEDIT_VER	0x0100
#include <Richedit.h> 

#include "bpqtermTCP.h"
#define TermTCP
#include "Versions.h"
#include "GetVersion.h"

#define BPQICON 2

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

BOOL WINE;

// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
int NewLine();
VOID DoRefresh(struct RTFTerm * OPData);
VOID SendTraceOptions();
int	ProcessBuff(char * readbuff,int len);
VOID EnableConnectMenu(HWND hWnd);
VOID EnableDisconnectMenu(HWND hWnd);
VOID DisableConnectMenu(HWND hWnd);
VOID DisableDisconnectMenu(HWND hWnd);
int DoReceivedData(HWND hWnd);
int	DoStateChange(HWND hWnd);
int ToggleFlags(HWND hWnd, int Item, int mask);
int DoMonData(HWND hWnd);
int TogglePort(HWND hWnd, int Item, int mask);
int ToggleMTX(HWND hWnd);
int ToggleMCOM(HWND hWnd);
int ToggleMUI(HWND hWnd);
int ToggleParam(HWND hWnd, BOOL * Param, int Item);
int ToggleChat(HWND hWnd);
void MoveWindows();
void CopyListToClipboard(HWND hWnd);
void CopyRichTextToClipboard(HWND hWnd);
BOOL OpenMonitorLogfile();
void WriteMonitorLine(char * Msg, int MsgLen);
VOID WritetoOutputWindow(struct RTFTerm * OPData, char * Msg, int len);
int SendMsg(char * msg, int len);
TCPConnect(char * Host, int Port);
int Telnet_Connected(SOCKET sock, int Error);
VOID DataSocket_Read(SOCKET sock);
VOID Socket_Data(int sock, int error, int eventcode);

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
//LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY SplitProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;


WNDPROC wpOrigInputProc; 
//WNDPROC wpOrigOutputProc; 
WNDPROC wpOrigMonProc; 
WNDPROC wpOrigSplitProc; 

HWND hwndInput;
HWND hwndOutput;
HWND hwndMon;
HWND hwndSplit;

HMENU trayMenu;

int xsize,ysize;		// Screen size at startup

double Split=0.5;
int SplitPos=300;
#define SplitBarHeight 5
#define InputBoxHeight 25
RECT Rect;
RECT MonRect;
//RECT OutputRect;
RECT SplitRect;

int OutputBoxHeight;

int Height, Width, LastY;

int maxlinelen = 80;

char Host[5][100] = {"localhost"};
int Port[5] = {0};
char UserName[5][80] = {""};
char Password[5][80] = {""};

char HN[5][6] = {"Host1", "Host2", "Host3", "Host4"};
char PN[5][6] = {"Port1", "Port2", "Port3", "Port4"};
char PASSN[5][6] = {"Pass1", "Pass2", "Pass3", "Pass4"};
char UN[5][6] = {"User1", "User2", "User3", "User4"};

#define MAXLINES 1000
#define LINELEN 200

char RTFHeader[4000];

int RTFHddrLen;

typedef struct RTFTerm
{
	int CurrentLine;				// Line we are writing to in circular buffer.

	int Index;
	BOOL SendHeader;
	BOOL Finished;

	char OutputScreen[MAXLINES][LINELEN];

	int Colourvalue[MAXLINES];
	int LineLen[MAXLINES];
	int CharWidth;
	int CurrentColour;
	int Thumb;
	int FirstTime;
	int RTFHeight;				// Height of RTF control in pixels
	BOOL Scrolled;				// Window is scrolled back

};

char FontName[100] = "FixedSys";
int FontSize = 20;
int FontWidth = 8;
int CodePage = 437;
int CharSet = 0;


struct RTFTerm OutputData;

int CurrentHost = 0;
int CfgNo = 0;

SOCKET sock;

BOOL MonData = FALSE;

char Key[80];
int portmask=0;
int mtxparam=1;
int mcomparam=1;
int monUI=0;

char kbbuf[160];
int kbptr=0;
char readbuff[100000];				// for stupid bbs programs

//int ptr=0;

int Stream;
int len,count;

char callsign[10];
int state;
int change;
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
int MonPorts = 1;

HANDLE 	MonHandle=INVALID_HANDLE_VALUE;

HCURSOR DragCursor;
HCURSOR	Cursor;

BOOL MinimizetoTray=FALSE;
//BOOL StartMinimized = FALSE;

HWND MainWnd, hWnd;

BOOL SocketActive = FALSE;
BOOL Connecting = FALSE;
BOOL Connected = FALSE;
BOOL Disconnecting = FALSE;

VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime );

TIMERPROC lpTimerFunc = (TIMERPROC) TimerProc;

int TimerHandle = 0;

int SlowTimer;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);
	return;
}


VOID SaveIntValue(char * Key, int Value)
{
	char Val[100];

	wsprintf(Val, "%d", Value);
	WritePrivateProfileString("Session 1", Key, Val, ".\\BPQTermTCP.ini");
}

VOID SaveStringValue(char * Key, char * Value)
{
	WritePrivateProfileString("Session 1", Key, Value, ".\\BPQTermTCP.ini");
}

int GetIntValue(char * Key, int Default)
{
	return GetPrivateProfileInt("Session 1", Key, Default, ".\\BPQTermTCP.ini");
}

VOID GetStringValue(char * Key, char * Value, int Len)
{
	GetPrivateProfileString("Session 1", Key, "", Value, Len, ".\\BPQTermTCP.ini");
}

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
	char Size[80];
	int n;

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

	SaveIntValue("PortMask", portmask);
	SaveIntValue("Bells", Bells);
	SaveIntValue("StripLF", StripLF);

	for (n = 0; n < 4; n++)
	{
		SaveStringValue(HN[n], Host[n]);
		SaveStringValue(UN[n], UserName[n]);
		SaveStringValue(PASSN[n], Password[n]);
		SaveIntValue(PN[n], Port[n]);
	}
	SaveIntValue("MTX",mtxparam);
	SaveIntValue("MCOM", mcomparam);
	SaveIntValue("MUIONLY", monUI);

#pragma warning(push)
#pragma warning(disable:4244)
		SaveIntValue("Split", Split * 100);
#pragma warning(pop)

	SaveIntValue("MONColour", MonitorColour);
	SaveIntValue("MonNODES", MonitorNODES);
	SaveIntValue("MonPorts", MonPorts);
	SaveIntValue("ChatMode", ChatMode);
	SaveIntValue("CurrentHost", CurrentHost);
	
	wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
	SaveStringValue("Size", Size);

	SaveStringValue("FontName", FontName);
	SaveIntValue("CharSet", CharSet);
	SaveIntValue("CodePage", CodePage);
	SaveIntValue("FontSize", FontSize);
	SaveIntValue("FontWidth", FontWidth);

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
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON));
    wc.hCursor = NULL; //LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	return (RegisterClass(&wc));

}

HMENU hMenu, hPopMenu1, hPopMenu2, hPopMenu3;		// handle of menu 

VOID SetupRTFHddr()
{
	int i, n;
	char RTFColours[3000];
	char Temp[1000];

	// Set up RTF Header, including Colours String;

	memcpy(RTFColours, "{\\colortbl ;", 12);
	n = 12;

	for (i = 1; i < 100; i++)
	{
		COLORREF Colour = Colours[i];
		n += wsprintf(&RTFColours[n], "\\red%d\\green%d\\blue%d;", GetRValue(Colour), GetGValue(Colour),GetBValue(Colour));
	}

	RTFColours[n++] = '}';
	RTFColours[n] = 0;

//	strcpy(RTFHeader, "{\\rtf1\\deff0{\\fonttbl{\\f0\\fmodern\\fprq1\\fcharset204 ;}}");
	wsprintf(RTFHeader, "{\\rtf1\\deff0{\\fonttbl{\\f0\\fprq1\\cpg%d\\fcharset%d %s;}}", CodePage,CharSet, FontName);
	strcat(RTFHeader, RTFColours);
	wsprintf(Temp, "\\viewkind4\\uc1\\pard\\f0\\fs%d", FontSize);
	strcat(RTFHeader, Temp);

	RTFHddrLen = strlen(RTFHeader);
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int i, n, tempmask=0xffff;
	char msg[20];
	char Size[80];
	WSADATA WsaData;            // receives data from WSAStartup
	struct RTFTerm * OPData;
	int retCode;
	HKEY hKey=0;


	hInst = hInstance; // Store instance handle in our global variable

	WSAStartup(MAKEWORD(2, 0), &WsaData);

	// See if running under WINE

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, "SOFTWARE\\Wine",  0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		WINE =TRUE;
		Debugprintf("Running under WINE");
	}
// Get saved config from ini

	MonitorNODES = GetIntValue("MonNODES", 0);
	MonPorts = GetIntValue("MonPorts", 0);
	mtxparam = GetIntValue("MTX", 0);
	mcomparam = GetIntValue("MCOM", 0);
	monUI = GetIntValue("MUIONLY", 0);
	tempmask = GetIntValue("PortMask", 0);
	ChatMode = GetIntValue("ChatMode", 0);
	MonitorColour = GetIntValue("MONColour", 0);
	Bells = GetIntValue("Bells", 0);
	StripLF = GetIntValue("StripLF", 0);
	CurrentHost = GetIntValue("CurrentHost", 0);
	Split = GetIntValue("Split", 0);
	if (Split == 0)
		Split = 50;

	Split /= 100;		// Stored as a %,used as 0 - 1

	for (n = 0; n < 4; n++)
	{
		GetStringValue(HN[n], Host[n], 100);
		GetStringValue(UN[n], UserName[n], 80);
		GetStringValue(PASSN[n], Password[n], 80);
		Port[n] = GetIntValue(PN[n], 0);
	}

	GetStringValue("FontName", FontName, 99);

	if (FontName[0] == 0)
		strcpy(FontName, "FixedSys");

	CharSet =  GetIntValue("CharSet", 0);
	CodePage =  GetIntValue("CodePage", 437);
	FontSize =  GetIntValue("FontSize", 20);
	FontWidth = GetIntValue("FontWidth", 8);

	OutputData.CharWidth = FontWidth;

	// Create a dialog box as the main window

	hWnd = CreateDialog(hInst,ClassName,0,NULL);
	
    if (!hWnd)
        return (FALSE);

	MainWnd=hWnd;

	GetStringValue("Size", Size, 80);
	sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);
	
	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(hWnd,	&Rect);
	}

	Height = Rect.bottom-Rect.top;
	Width = Rect.right-Rect.left;

#pragma warning(push)
#pragma warning(disable:4244)
	SplitPos=Height*Split;
#pragma warning(pop)

	SetupRTFHddr();

	// Create a Rich Text Control 

	OPData = &OutputData;

	OPData->SendHeader = TRUE;
	OPData->Finished = TRUE;
	OPData->CurrentColour = 1;

	LoadLibrary("riched20.dll");

	hwndOutput = CreateWindowEx(WS_EX_CLIENTEDGE, RICHEDIT_CLASS, "",
		WS_CHILD |  WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_NOHIDESEL | WS_VSCROLL | ES_READONLY,
		6,145,290,130, MainWnd, NULL, hInstance, NULL);

	// Register for Mouse Events for Copy/Paste
	
	SendMessage(hwndOutput, EM_SETEVENTMASK, (WPARAM)0, (LPARAM)ENM_MOUSEEVENTS | ENM_SCROLLEVENTS | ENM_KEYEVENTS);
	SendMessage(hwndOutput, EM_EXLIMITTEXT, 0, MAXLINES * LINELEN);

	trayMenu = CreatePopupMenu();

	AppendMenu(trayMenu,MF_STRING,40000,"Copy");

	// Retrieve the handlse to the edit controls. 

	hwndInput = GetDlgItem(hWnd, 118); 
	hwndSplit = GetDlgItem(hWnd, 119); 
	hwndMon = GetDlgItem(hWnd, 116); 
 
	// Set our own WndProcs for the controls. 

	wpOrigInputProc = (WNDPROC) SetWindowLong(hwndInput, GWL_WNDPROC, (LONG) InputProc); 
	wpOrigMonProc = (WNDPROC)SetWindowLong(hwndMon, GWL_WNDPROC, (LONG)MonProc);
	wpOrigSplitProc = (WNDPROC)SetWindowLong(hwndSplit, GWL_WNDPROC, (LONG)SplitProc);

	MoveWindow(hWnd,Rect.left,Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

	MoveWindows();
	
	GetVersionInfo(NULL);

	wsprintf(Title,"BPQTermTCP Version %s", VersionString);

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

	hPopMenu1=GetSubMenu(hMenu,3);

	for (i = 1; i <= MonPorts; i++)
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

	CheckMenuItem(hMenu, BPQMTX, (mtxparam) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BPQMCOM, (mcomparam) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, MUIONLY, (monUI) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BPQBELLS, (Bells) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BPQStripLF, (StripLF) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, BPQMNODES, (MonitorNODES) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, MONCOLOUR, (MonitorColour) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu, CHATTERM, (ChatMode) ? MF_CHECKED : MF_UNCHECKED);

	for (i = 0; i < 4; i++)
	{
		if (Host[i][0])
		{
			ModifyMenu(hMenu, IDC_HOST1 + i, MF_BYCOMMAND, IDC_HOST1 + i, Host[i]);
			ModifyMenu(hMenu, BPQCONNECT1 + i, MF_BYCOMMAND, BPQCONNECT1 + i, Host[i]);
		}
	}

	EnableMenuItem(hMenu,BPQDISCONNECT,MF_GRAYED);

	DrawMenuBar(hWnd);	

	if (portmask) applflags |= 0x80;

	SendTraceOptions();
	
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

		SetDlgItemText(hDlg, IDC_HOST, Host[CfgNo]);
		SetDlgItemInt(hDlg, IDC_PORT, Port[CfgNo], FALSE);
		SetDlgItemText(hDlg, IDC_USER, UserName[CfgNo]);
		SetDlgItemText(hDlg, IDC_PASS, Password[CfgNo]);

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

			GetDlgItemText(hDlg, IDC_HOST, Host[CfgNo], 99);
			Port[CfgNo] = GetDlgItemInt(hDlg, IDC_PORT,NULL, FALSE);
			GetDlgItemText(hDlg, IDC_USER, UserName[CfgNo], 79);
			GetDlgItemText(hDlg, IDC_PASS, Password[CfgNo], 79);

			ModifyMenu(hMenu, BPQCONNECT1 + CfgNo, MF_BYCOMMAND, BPQCONNECT1 + CfgNo, Host[CfgNo]);
			ModifyMenu(hMenu, IDC_HOST1 + CfgNo, MF_BYCOMMAND, IDC_HOST1 + CfgNo, Host[CfgNo]);


		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
	}

	return FALSE;

}

INT_PTR CALLBACK FontConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	POINT Point;
	int Min, Max;

	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, IDC_FONTNAME, FontName);
		SetDlgItemInt(hDlg, IDC_CHARSET, CharSet, FALSE);
		SetDlgItemInt(hDlg, IDC_CODEPAGE, CodePage, FALSE);
		SetDlgItemInt(hDlg, IDC_FONTSIZE, FontSize, FALSE);
		SetDlgItemInt(hDlg, IDC_FONTWIDTH, FontWidth, FALSE);

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

			GetDlgItemText(hDlg, IDC_FONTNAME, FontName, 99);
			CharSet = GetDlgItemInt(hDlg, IDC_CHARSET, NULL, FALSE);
			CodePage = GetDlgItemInt(hDlg, IDC_CODEPAGE, NULL, FALSE);
			FontSize = GetDlgItemInt(hDlg, IDC_FONTSIZE, NULL, FALSE);
			FontWidth = GetDlgItemInt(hDlg, IDC_FONTWIDTH, NULL, FALSE);

			SaveStringValue("FontName", FontName);
			SaveIntValue("CharSet", CharSet);
			SaveIntValue("CodePage", CodePage);
			SaveIntValue("FontSize", FontSize);
			SaveIntValue("FontWidth", FontWidth);

			OutputData.CharWidth = FontWidth;

			SetupRTFHddr();

			Point.x = 0;
			Point.y = 25000;					// Should be plenty for any font

			SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
			OutputData.Scrolled = FALSE;

			GetScrollRange(hwndOutput, SB_VERT, &Min, &Max);	// Get Actual Height
			OutputData.RTFHeight = Max;

			DoRefresh(&OutputData);

			return TRUE;

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
    int y, i;  
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

//		case WM_CREATE:
 
//  SendMessage(hWndRichEdit, EM_SETEVENTMASK, (WPARAM)0 (LPARAM)ENM_MOUSEEVENTS);
//  break;


	case WM_CTLCOLOREDIT:
		
		if (OutputData.Scrolled)
		{
			HDC hdcStatic = (HDC)wParam;
			SetBkMode(hdcStatic, TRANSPARENT);

			return (LONG)GetStockObject(LTGRAY_BRUSH);
		}
		return (DefWindowProc(hWnd, message, wParam, lParam));

	case WM_NOTIFY:
	{
		const MSGFILTER * pF = (MSGFILTER *)lParam;
		POINT pos;
		CHARRANGE Range;

		if(pF->nmhdr.hwndFrom == hwndOutput)
		{
			if(pF->msg == WM_VSCROLL)
			{
//				OutputData.Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);
				DoRefresh(&OutputData);
//				return TRUE;		
			}

			if(pF->msg == WM_KEYUP)
			{
				if (pF->wParam == VK_PRIOR || pF->wParam == VK_NEXT)
				{
//					OutputData.Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);
					DoRefresh(&OutputData);
				}
			}

			if(pF->msg == WM_RBUTTONDOWN)
			{
				// Only allow popup if something is selected

				SendMessage(hwndOutput, EM_EXGETSEL , 0, (WPARAM)&Range);
				if (Range.cpMin == Range.cpMax)
					return TRUE;

				GetCursorPos(&pos);
				TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, hWnd, 0);
				return TRUE;
			}
		}
		break;
	}

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

		case 40000:
		{
			int len=0;
			HGLOBAL	hMem;
			char * ptr;
			CHARRANGE Range;

			// Copy Rich Text Selection to Clipboard
	
			SendMessage(hwndOutput, EM_EXGETSEL , 0, (WPARAM)&Range);
	
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, Range.cpMax - Range.cpMin + 1);

			if (hMem != 0)
			{
				ptr=GlobalLock(hMem);
	
				if (OpenClipboard(MainWnd))
				{
					len = SendMessage(hwndOutput, EM_GETSELTEXT  , 0, (WPARAM)ptr);

					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT,hMem);
					CloseClipboard();
				}
			}
			else
				GlobalFree(hMem);

			SetFocus(hwndInput);
			return TRUE;
		}
		
		case BPQMTX:
	
			ToggleMTX(hWnd);
			break;

		case BPQMCOM:

			ToggleMCOM(hWnd);
			break;

		case MUIONLY:

			ToggleMUI(hWnd);
			break;

		case BPQCONNECT1:
		case BPQCONNECT2:
		case BPQCONNECT3:
		case BPQCONNECT4:

			CurrentHost = wmId - BPQCONNECT1;

			TCPConnect(Host[CurrentHost], Port[CurrentHost]);
			break;
			        
		case BPQDISCONNECT:
			
			if (Disconnecting)
			{
				// Force close
					
				if (SocketActive)
					closesocket(sock);

				wsprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
				SetWindowText(hWnd,Title);
				DisableDisconnectMenu(hWnd);
				EnableConnectMenu(hWnd);

				WritetoOutputWindow(&OutputData, "*** Disconnected\r", 17);		
				DoRefresh(&OutputData);
				SocketActive = FALSE;
				Connected = FALSE;
				Disconnecting = FALSE;	

				break;
			}

			shutdown(sock, 2);		// SD_BOTH
			Disconnecting = TRUE;
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

		case IDC_HOST1:
		case IDC_HOST2:
		case IDC_HOST3:
		case IDC_HOST4:

			CfgNo = wmId - IDC_HOST1;

			DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hWnd, ConfigWndProc);
			break;

		case ID_SETUP_FONT:

			DialogBox(hInst, MAKEINTRESOURCE(IDD_FONT), hWnd, FontConfigWndProc);
			break;

		case BPQMNODES:

			ToggleParam(hWnd, &MonitorNODES, BPQMNODES);
			SendTraceOptions();
			break;


		case MONITOR_ADDPORT:

		wsprintf(Buffer,"Port %d", ++MonPorts);
		
		AppendMenu(hPopMenu1,MF_STRING | MF_UNCHECKED,BPQBASE + MonPorts, Buffer);


		case MONCOLOUR:

			ToggleParam(hWnd, &MonitorColour, MONCOLOUR);
			SendTraceOptions();
			break;

		case BPQLogMonitor:

			ToggleParam(hWnd, &LogMonitor, BPQLogMonitor);
			break;

		case BPQCLEARMON:

			SendMessage(hwndMon,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCLEAROUT:

			for (i = 0; i < MAXLINES; i++)
			{
				OutputData.OutputScreen[i][0] = 0;
			}

			OutputData.CurrentLine = 0;
			DoRefresh(&OutputData);
			break;

		case BPQCOPYMON:

			CopyListToClipboard(hwndMon);
			break;

		case BPQCOPYOUT:
		
			CopyRichTextToClipboard(hwndOutput);
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

		case SC_RESTORE:

		case SC_MINIMIZE: 
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_SIZE:

			MoveWindows();
			
			return TRUE;

		case WM_SIZING:

			lprc = (LPRECT) lParam;

			Height = lprc->bottom-lprc->top;
			Width = lprc->right-lprc->left;

#pragma warning(push)
#pragma warning(disable:4244)
			SplitPos=Height*Split;
#pragma warning(pop)

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
	char DisplayLine[200] = "\x1b\x21";

	if (uMsg == WM_CTLCOLOREDIT)
	{
		HBRUSH Brush = CreateSolidBrush(RGB(0, 255, 0));
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
		SetBkMode(hdcStatic, TRANSPARENT);

		return (LONG)Brush;
	}

	
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
			{
				TCPConnect(Host[CurrentHost], Port[CurrentHost]);
				SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");
				return 0;
			}

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

			if (OutputData.Scrolled)
			{
				POINT Point;
				Point.x = 0;
				Point.y = 25000;					// Should be plenty for any font

				SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
				OutputData.Scrolled = FALSE;
			}

			WritetoOutputWindow(&OutputData, DisplayLine, kbptr+3);
			DoRefresh(&OutputData);
		
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
			DoRefresh(&OutputData);
			return 0;

		case WM_MOUSEMOVE:

 			// Used to size split between Monitor and Output Windows

			SetCursor(DragCursor);
			break;
	}

    return CallWindowProc(wpOrigSplitProc, hwnd, uMsg, wParam, lParam); 
} 

DWORD CALLBACK EditStreamCallback(DWORD_PTR Cookie, LPBYTE lpBuff, LONG cb, PLONG pcb)
{
	struct RTFTerm * OPData	= (struct RTFTerm *)Cookie;
	int ReqLen = cb;
	int i;
	int Line;

//	if (cb != 4092)
//		return 0;

	if (OPData->SendHeader)
	{
		// Return header

		memcpy(lpBuff, RTFHeader, RTFHddrLen);
		*pcb = RTFHddrLen;
		OPData->SendHeader = FALSE;
		OPData->Finished = FALSE;
		OPData->Index = 0;
		return 0;
	}

	if (OPData->Finished)
	{
		*pcb = 0;
		return 0;
	}
	
/*
	if (BufferLen > cb)
	{
		memcpy(lpBuff, &Buffer[Offset], cb);
		BufferLen -= cb;
		Offset += cb;
		*pcb = cb;
		return 0;
	}

	memcpy(lpBuff, &Buffer[Offset], BufferLen);

    *pcb = BufferLen;
*/

	// Return 10 line at a time

	for (i = 0; i < 10; i++);
	{
	Line = OPData->Index++ + OPData->CurrentLine - MAXLINES;

	if (Line <0)
		Line = Line + MAXLINES;

	wsprintf(lpBuff, "\\cf%d ", OPData->Colourvalue[Line]);
	strcat(lpBuff, OPData->OutputScreen[Line]);
	strcat(lpBuff, "\\line");

	if (OPData->Index == MAXLINES)
	{
		OPData->Finished = TRUE;
		strcat(lpBuff, "}");
		i = 10;
	}
	}
	*pcb = strlen(lpBuff);
	return 0;
}

VOID ForceRefresh(struct RTFTerm * OPData)
{
	OPData->Thumb = 25000;
	DoRefresh(OPData);
	OPData->Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);
}

VOID DoRefresh(struct RTFTerm * OPData)
{
	EDITSTREAM es = {0};
	int Min, Max, Pos;
	POINT Point;
	SCROLLINFO ScrollInfo;
	int LoopTrap = 0;

	if(WINE)
		OPData->Thumb = 30000;
	else
		OPData->Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);

	Pos = OPData->Thumb + OutputBoxHeight;

	if (Pos > OPData->RTFHeight - 10)		// Don't bother writing to screen if scrolled back
	{
		es.pfnCallback = EditStreamCallback;
		es.dwCookie = (DWORD_PTR)OPData;
		OPData->SendHeader = TRUE;
		SendMessage(hwndOutput, EM_STREAMIN, SF_RTF, (LPARAM)&es);
	}
	else
		Debugprintf("Pos %d RTFHeight %d - Not refreshing", Pos, OPData->RTFHeight);

	GetScrollRange(hwndOutput, SB_VERT, &Min, &Max);
	ScrollInfo.cbSize = sizeof(ScrollInfo);
    ScrollInfo.fMask = SIF_ALL;

	GetScrollInfo(hwndOutput, SB_VERT, &ScrollInfo);

//	Debugprintf("Thumb %d Pos %d Min %d Max %d nMax %d ClientH %d RTFHeight %d",
//		OPData->Thumb, Pos, Min, Max, ScrollInfo.nMax, OutputBoxHeight, OPData->RTFHeight);

	if (OPData->FirstTime == FALSE)
	{
		// RTF Controls don't immediately scroll to end - don't know why.
		
		OPData->FirstTime = TRUE;
		Point.x = 0;
		Point.y = 25000;					// Should be plenty for any font

		while (LoopTrap++ < 20)
		{
			SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
		}

		GetScrollRange(hwndOutput, SB_VERT, &Min, &Max);	// Get Actual Height
		OPData->RTFHeight = Max;
		Point.x = 0;
		Point.y = OPData->RTFHeight - ScrollInfo.nPage;
		SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
		OPData->Thumb = SendMessage(hwndOutput, EM_GETTHUMB, 0, 0);
	}

	Point.x = 0;
	Point.y = OPData->RTFHeight - ScrollInfo.nPage;

	if (OPData->Thumb > (Point.y - 10))		// Don't Scroll if user has scrolled back 
	{
//		Debugprintf("Scrolling to %d", Point.y);
		SendMessage(hwndOutput, EM_SETSCROLLPOS, 0, (LPARAM) &Point);
		if (OPData->Scrolled)
		{
			OPData->Scrolled = FALSE;
			InvalidateRect(hwndInput, NULL, TRUE);
		}
		return;
	}

	Debugprintf("Thumb = %d Point.y = %d - Not Scrolling", OPData->Thumb, Point.y);

	if (!OPData->Scrolled)
	{
		OPData->Scrolled = TRUE;
		InvalidateRect(hwndInput, NULL, TRUE);
	}
}

VOID AddLinetoWindow(struct RTFTerm * OPData, char * Line)
{
	int Len = strlen(Line);
	char * ptr1 = Line;
	char * ptr2;
	int l, Index;
	char LineCopy[LINELEN * 2];
	
	if (Line[0] ==  0x1b && Len > 1)
	{
		// Save Colour Char
		
		OPData->CurrentColour = Line[1] - 10;
		ptr1 +=2;
		Len -= 2;
	}

	strcpy(OPData->OutputScreen[OPData->CurrentLine], ptr1);

	// Look for chars we need to escape (\  { })

	ptr1 = OPData->OutputScreen[OPData->CurrentLine];
	Index = 0;
	ptr2 = strchr(ptr1, '\\');				// Look for Backslash first, as we may add some later

	if (ptr2)
	{
		while (ptr2)
		{
			l = ++ptr2 - ptr1;
			memcpy(&LineCopy[Index], ptr1, l);	// Copy Including found char
			Index += l;
			LineCopy[Index++] = '\\';
			Len++;
			ptr1 = ptr2;
			ptr2 = strchr(ptr1, '\\');
		}
		strcpy(&LineCopy[Index], ptr1);			// Copy in rest
		strcpy(OPData->OutputScreen[OPData->CurrentLine], LineCopy);
	}

	ptr1 = OPData->OutputScreen[OPData->CurrentLine];
	Index = 0;
	ptr2 = strchr(ptr1, '{');

	if (ptr2)
	{
		while (ptr2)
		{
			l = ptr2 - ptr1;
			memcpy(&LineCopy[Index], ptr1, l);
			Index += l;
			LineCopy[Index++] = '\\';
			LineCopy[Index++] = '{';
			Len++;
			ptr1 = ++ptr2;
			ptr2 = strchr(ptr1, '{');
		}
		strcpy(&LineCopy[Index], ptr1);			// Copy in rest
		strcpy(OPData->OutputScreen[OPData->CurrentLine], LineCopy);
	}

	ptr1 = OPData->OutputScreen[OPData->CurrentLine];
	Index = 0;
	ptr2 = strchr(ptr1, '}');				// Look for Backslash first, as we may add some later

	if (ptr2)
	{
		while (ptr2)
		{
			l = ptr2 - ptr1;
			memcpy(&LineCopy[Index], ptr1, l);	// Copy 
			Index += l;
			LineCopy[Index++] = '\\';
			LineCopy[Index++] = '}';
			Len++;
			ptr1 = ++ptr2;
			ptr2 = strchr(ptr1, '}');
		}
		strcpy(&LineCopy[Index], ptr1);			// Copy in rest
		strcpy(OPData->OutputScreen[OPData->CurrentLine], LineCopy);
	}


	OPData->Colourvalue[OPData->CurrentLine] = OPData->CurrentColour;
	OPData->LineLen[OPData->CurrentLine++] = Len;
	if (OPData->CurrentLine >= MAXLINES) OPData->CurrentLine = 0;
}

VOID WritetoOutputWindow(struct RTFTerm * OPData, char * Msg, int len)
{
	char * ptr1, * ptr2;

	if (PartLinePtr != 0)
	{
		OPData->CurrentLine--;				// Overwrite part line in buffer
		
		if (Msg[0] == 0x1b && len > 1) 
		{
			Msg += 2;		// Remove Colour Escape
			len -= 2;
		}
	}
	
	memcpy(&readbuff[PartLinePtr], Msg, len);
		
	len += PartLinePtr;

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
		//	copy text to buffer a line at a time	
	
		ptr2=memchr(ptr1,13,len);

		if (ptr2 == 0)
		{
			// no newline. Move data to start of buffer and Save pointer

			PartLinePtr = len;
			memmove(readbuff,ptr1,len);
			AddLinetoWindow(OPData, ptr1);
			return;
		}
		
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

			AddLinetoWindow(OPData, ptr1);

			ptr1[foldlen] = save;
			linelen -= foldlen;
			ptr1 = ptr3;

			if (linelen > maxlinelen)
				goto foldloop;
						
			AddLinetoWindow(OPData, ptr1);						
			ptr1 = saveptr1;
					
		}
		else
			AddLinetoWindow(OPData, ptr1);

			
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
		goto lineloop;
	}
}



char MonSave[1000];
int MonSaveLen;

VOID WritetoMonWindow(char * Msg, int len)
{
	char * ptr1 = Msg, * ptr2;
	int index;

	if (MonSaveLen)
	{
		// Have part line - append to it
		memcpy(&MonSave[MonSaveLen], Msg, len);
		MonSaveLen += len;
		ptr1 = MonSave;
		len = MonSaveLen;
		MonSaveLen = 0;
	}

lineloop:

	if (len <=  0)
		return;

	//	copy text to control a line at a time	

	ptr2 = memchr(ptr1,13,len);
				
	if (ptr2 == 0)	// No CR
	{
		memmove(MonSave, ptr1, len);
		MonSaveLen = len;
		return;
	}

	*(ptr2++) = 0;

	index = SendMessage(hwndMon, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR) ptr1 );

	if (LogMonitor) WriteMonitorLine(ptr1, ptr2 - ptr1);

	if (index > 1200)
		do {index=SendMessage(hwndMon,LB_DELETESTRING, 0, 0);} while (index > 1000);

	SendMessage(hwndMon,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

	len -= (ptr2 - ptr1);
	ptr1 = ptr2;
					
	goto lineloop;
}

VOID DisableConnectMenu(HWND hWnd)
{
	EnableMenuItem(GetMenu(hWnd), BPQCONNECT1, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), BPQCONNECT2, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), BPQCONNECT3, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), BPQCONNECT4, MF_GRAYED);
	DrawMenuBar(hWnd);	
}	
VOID DisableDisconnectMenu(HWND hWnd)
{
	EnableMenuItem(GetMenu(hWnd), BPQDISCONNECT, MF_GRAYED);
	DrawMenuBar(hWnd);
}	

VOID EnableConnectMenu(HWND hWnd)
{
	HMENU hMenu;	// handle of menu 

	hMenu=GetMenu(hWnd);
	 
	EnableMenuItem(hMenu,BPQCONNECT1,MF_ENABLED);
	EnableMenuItem(hMenu,BPQCONNECT2,MF_ENABLED);
	EnableMenuItem(hMenu,BPQCONNECT3,MF_ENABLED);
	EnableMenuItem(hMenu,BPQCONNECT4,MF_ENABLED);
	DrawMenuBar(hWnd);	
}	

VOID EnableDisconnectMenu(HWND hWnd)
{
 	EnableMenuItem(GetMenu(hWnd), BPQDISCONNECT, MF_ENABLED);
	DrawMenuBar(hWnd);
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

int TogglePort(HWND hWnd, int Item, int mask)
{
	portmask ^= mask;
	
	if (portmask & mask)
		CheckMenuItem(hMenu,Item,MF_CHECKED);
	else
		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

	SendTraceOptions();

    return (0);
  
}
int ToggleMTX(HWND hWnd)
{
	mtxparam = mtxparam ^ 1;
	
	if (mtxparam & 1)

		CheckMenuItem(hMenu,BPQMTX,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,BPQMTX,MF_UNCHECKED);

	SendTraceOptions();

    return (0);
  
}
int ToggleMCOM(HWND hWnd)
{
	mcomparam = mcomparam ^ 1;
	CheckMenuItem(hMenu, BPQMCOM, (mcomparam) ? MF_CHECKED : MF_UNCHECKED);
	SendTraceOptions();

    return (0);
  
}
int ToggleMUI(HWND hWnd)
{
	monUI = monUI ^ 1;
	CheckMenuItem(hMenu, MUIONLY, (monUI) ? MF_CHECKED : MF_UNCHECKED);
	SendTraceOptions();

    return (0);
  
}
int ToggleParam(HWND hWnd, BOOL * Param, int Item)
{
	*Param = !(*Param);

	CheckMenuItem(hMenu,Item, (*Param) ? MF_CHECKED : MF_UNCHECKED);
	
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

	if (ClientWidth == 0)		// Minimized
		return;

	OutputBoxHeight = ClientHeight - SplitPos - InputBoxHeight - SplitBarHeight - SplitBarHeight;

	MoveWindow(hwndMon,2, 0, ClientWidth-4, SplitPos, TRUE);
	MoveWindow(hwndOutput,2, SplitPos+SplitBarHeight, ClientWidth-4, OutputBoxHeight, TRUE);
	MoveWindow(hwndInput,2, ClientHeight-InputBoxHeight-2, ClientWidth-4, InputBoxHeight, TRUE);
	MoveWindow(hwndSplit,0, SplitPos, ClientWidth, SplitBarHeight, TRUE);

	GetClientRect(hwndOutput, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;
	
	if (ClientWidth > 16)
		maxlinelen = ClientWidth/OutputData.CharWidth - 1;

	InvalidateRect(hWnd, NULL, TRUE);
}

void CopyRichTextToClipboard(HWND hWnd)
{
	int len=0;
	HGLOBAL	hMem;
	char * ptr;

	// Copy Rich Text to Clipboard
	
	len = SendMessage(hwndOutput, WM_GETTEXTLENGTH, 0, 0);
	
	hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len + 1);

	if (hMem != 0)
	{
		ptr=GlobalLock(hMem);

		if (OpenClipboard(MainWnd))
		{
			len = SendMessage(hwndOutput, WM_GETTEXT  , len, (LPARAM)ptr);

			GlobalUnlock(hMem);
			EmptyClipboard();
			SetClipboardData(CF_TEXT,hMem);
			CloseClipboard();
		}
	}
	else
		GlobalFree(hMem);
}


void CopyListToClipboard(HWND hWnd)
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
	int addrlen=sizeof(sinx);
	char PortString[10];

	struct addrinfo hints, *res;

	wsprintf(PortString, "%d", Port);


	// get host info, make socket, and connect it

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;  // use IPv4 or IPv6, whichever
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo(Host, PortString, &hints, &res);

	if (!res)
	{
		MessageBox(NULL, "Resolve HostName Failed", "BPQTermTCP", MB_OK);
		wsprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
		SetWindowText(hWnd,Title);

		return FALSE;			// Resolve failed
	
	}

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	
	WSAAsyncSelect(sock, hWnd, WSA_DATA,
		FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

/*
	
	// Resolve Name if needed

	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(Port);
	destaddr.sin_addr.s_addr = inet_addr(Host);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
		struct hostent * HostEnt;

		//	Resolve name to address

		wsprintf(Title,"BPQTermTCP Version %s - Resolving %s", VersionString, Host);
		SetWindowText(hWnd,Title);

		HostEnt = gethostbyname(Host);
		 
		 if (!HostEnt)
		 {
			MessageBox(NULL, "Resolve HostName Failed", "BPQTermTCP", MB_OK);
			wsprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
			SetWindowText(hWnd,Title);

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
*/
	if ((status = WSAAsyncSelect(sock, hWnd, WSA_CONNECT, FD_CONNECT)) > 0)
	{
		closesocket(sock);
		return FALSE;
	}

	if (connect(sock, res->ai_addr, res->ai_addrlen) == 0)
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

			EnableDisconnectMenu(hWnd);
			DisableConnectMenu(hWnd);

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

		wsprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
		SetWindowText(hWnd,Title);
		DisableDisconnectMenu(hWnd);
		EnableConnectMenu(hWnd);

		return 0;

	}

	WSAAsyncSelect(sock, hWnd, WSA_DATA,
					FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
	
	SocketActive = TRUE;
	Connecting = FALSE;
	Connected = TRUE;

	Len = wsprintf(Msg, "%s\r%s\rBPQTermTCP\r", UserName[CurrentHost], Password[CurrentHost]);
	
	SendMsg(Msg, Len);

	SendTraceOptions();

	SlowTimer = 0;
			
	wsprintf(Title,"BPQTermTCP Version %s - Connected to %s", VersionString, Host[CurrentHost]);
	SetWindowText(hWnd,Title);
	DisableConnectMenu(hWnd);
	EnableDisconnectMenu(hWnd);

	return 0;
}

VOID Socket_Data(int sock, int error, int eventcode)
{
	char Title[100];

	switch (eventcode)
	{
		case FD_READ:
			
			DataSocket_Read(sock);
			return;

		case FD_WRITE:
		case FD_OOB:
		case FD_ACCEPT:
		case FD_CONNECT:

			return;

		case FD_CLOSE:

			if (SocketActive)
				closesocket(sock);

			wsprintf(Title,"BPQTermTCP Version %s - Disconnected", VersionString);
			SetWindowText(hWnd,Title);
			DisableDisconnectMenu(hWnd);
			EnableConnectMenu(hWnd);

			WritetoOutputWindow(&OutputData, "*** Disconnected\r", 17);		
			DoRefresh(&OutputData);
			SocketActive = FALSE;
			Connected = FALSE;
			Disconnecting = FALSE;	
			return ;
	}
	return ;
}

VOID DataSocket_Read(SOCKET sock)
{
	int len = 0, MonLen;
	char Buffer[500];
	char * ptr;
	char * Buffptr;
	char * FEptr = 0;

	ioctlsocket(sock, FIONREAD, &len);

	if (len > 500) len = 500;

	len = recv(sock, Buffer, len, 0);

	if (len == SOCKET_ERROR || len == 0)
	{
		// Failed or closed - clear connection

		return;
	}

	// Look for MON delimiters (FF/FE)

	Buffptr = Buffer;

	if (MonData)
	{
		// Already in MON State

		FEptr = memchr(Buffptr, 0xfe, len);

		if (!FEptr)
		{
			// no FE - so send all to monitor
			
			WritetoMonWindow(Buffptr, len);
			return;
		}

		MonData = FALSE;

		MonLen = FEptr - Buffptr;		// Mon Data, Excluding the FE

		WritetoMonWindow(Buffptr, MonLen);

		Buffptr = ++FEptr;				// Char following FE

		if (++MonLen < len)
		{
			len -= MonLen;
			goto MonLoop;				// See if next in MON or Data
		}

		// Nothing Left

		DoRefresh(&OutputData);
		return ;
	}

MonLoop:

	if (ptr = memchr(Buffptr, 0xff, len))
	{
		// Buffer contains Mon Data

		if (ptr > Buffptr)
		{
			// Some Normal Data before the FF

			int NormLen = ptr - Buffptr;				// Before the FF
			WritetoOutputWindow(&OutputData, Buffptr, NormLen);

			len -= NormLen;
			Buffptr = ptr;
			goto MonLoop;
		}

		MonData = TRUE;

		FEptr = memchr(Buffptr, 0xfe, len);

		if (FEptr)
		{
			MonData = FALSE;

			MonLen = FEptr + 1 - Buffptr;				// MonLen includes FF and FE
			WritetoMonWindow(Buffptr+1, MonLen - 2);
			DoRefresh(&OutputData);

			len -= MonLen;
			Buffptr += MonLen;							// Char Following FE

			if (len <= 0)
			{
				return;
			}
			goto MonLoop;
		}
		else
		{
			// No FE, so rest of buffer is MON Data
			
			WritetoMonWindow(Buffptr+1, len -1);		// Exclude FF
//			DoRefresh();
			return;
		}
	}

	// No FF, so must be session data

	WritetoOutputWindow(&OutputData, Buffptr, len);
	DoRefresh(&OutputData);
	SlowTimer = 0;
	return;
}

int SendMsg(char * msg, int len)
{
	send(sock, msg, len, 0);
	return 0;
}

VOID SendTraceOptions()
{
	char Buffer[80];

	int Len = wsprintf(Buffer,"\\\\\\\\%x %x %x %x %x %x\r", portmask, mtxparam, mcomparam, MonitorNODES, MonitorColour, monUI);

	send(sock, Buffer, Len, 0);

}
