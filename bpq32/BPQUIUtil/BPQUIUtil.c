
//	Program to send ax.25 Beacons from a BPQ32 Switch

// Version 0.1.1.0 January 2012

//		Call CloseBPQ32 on exit
//		Allow use with Tracker and UZ7HO ports

#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "commctrl.h"
#include "Commdlg.h"


#include "bpq32.h"
#include "ASMStrucs.h"
#include "BPQUIUtil.h"
#define UIUTIL
#include "GetVersion.h"

#pragma pack(1)

typedef struct _MESSAGEX
{
//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

	struct _MESSAGEX * CHAIN;

	UCHAR	PORT;
	USHORT	LENGTH;

	UCHAR	DEST[7];
	UCHAR	ORIGIN[7];

//	 MAY BE UP TO 56 BYTES OF DIGIS

	UCHAR	CTL;
	UCHAR	PID;
	UCHAR	DATA[256];
	UCHAR	PADDING[56];			// In case he have Digis

}MESSAGEX, *PMESSAGEX;

#pragma pack()

HKEY REGTREE = HKEY_CURRENT_USER;

int CurrentPage=0;				// Page currently on show in tabbed Dialog

int PageCount;

typedef struct tag_dlghdr {

HWND hwndTab; // tab control
HWND hwndDisplay; // current child dialog box
RECT rcDisplay; // display rectangle for the tab control


DLGTEMPLATE *apRes[33];

} DLGHDR;


HINSTANCE hInst; 

HWND hwndDlg;		// Config Dialog
HWND hwndDisplay;   // Current child dialog box


UINT UIMask = 0;

#define MAXFLEN 400     /* Maximale Laenge eines Frames */
                        /* maximum length of a frame */
typedef signed short    i16;
typedef unsigned char   byte;
typedef unsigned long   u32;

int PortNum[33];		// Tab nunber to port

UINT UIPortMask = 0;
BOOL UIEnabled[33];
char * UIDigi[33];
char * UIDigiAX[33];		// ax.25 version of digistring
int UIDigiLen[33];			// Length of AX string

char UIDEST[33][11];		// Dest for Beacons

char AXDEST[33][7];
char MYCALL[7];

#define NUMBEROFBUFFERS 20

static UINT FREE_Q = 0;
static UINT BufferPool[100*NUMBEROFBUFFERS];		// 400 Byte buffers

int QCount = 0;

UINT * Q_REM(UINT *Q);
VOID Q_ADD(UINT *Q,UINT *BUFF);
UINT * GetBuffer();
VOID ReleaseBuffer(UINT *BUFF);

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
HANDLE _beginthread( void( *start_address )(), unsigned stack_size, int arglist);
VOID SetupUI(int Port);
VOID SendBeacon(int Port);
VOID WINAPI OnTabbedDialogInit(HWND hDlg);
VOID WINAPI OnSelChanged(HWND hwndDlg);
DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName);
VOID WINAPI OnChildDialogInit(HWND hwndDlg);


int TimerHandle = 0;

char ClassName[]="UIMAINWINDOW";					// the main window class name

HWND hWnd;

LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;
BOOL Minimized = FALSE;


UCHAR FN[33][256];			// Filename
int Interval[33];			// Beacon Interval (Mins)
int MinCounter[33];			// Interval Countdown

BOOL SendFromFile[33];
char Message[33][1000];		// Beacon Text

typedef UINT (FAR *FARPROCX)();

UINT TXQ;


VOID * zalloc(int len)
{
	void * ptr;

	ptr=malloc(len);
	memset(ptr, 0, len);
	return ptr;
}

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++=0;

	return ptr;
}


VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[10000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HKEY hKey=0;
	int retCode, disp;

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);

	// Main message loop:

	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	KillTimer(NULL,TimerHandle);
	
	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil", 0, 0, 0,
				KEY_ALL_ACCESS, NULL, &hKey, &disp);

	RegSetValueEx(hKey,"PortMask",0,REG_DWORD,(BYTE *)&UIMask,4);

	if (MinimizetoTray)
		DeleteTrayMenuItem(hWnd);

	CloseBPQ32();				// Close Ext Drivers if last bpq32 process

	return (msg.wParam);
}

HBRUSH bgBrush;

#define BGCOLOUR RGB(236,233,216)


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int retCode, Type, Vallen;
	char Key[80];
	int err, i=0;
	char Size[80];
	RECT Rect = {0};
	char Title[80];
	HKEY hKey = 0;
	HKEY hKey2 = 0;
	WNDCLASS  wc;
	int Row = 210;

	hInst = hInstance; // Store instance handle in our global variable

	REGTREE = GetRegistryKey();

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	hWnd=CreateDialog(hInst,ClassName,0,NULL);

	if (!hWnd)
	{
		err=GetLastError();
		return FALSE;
	}

	GetVersionInfo(NULL);
	wsprintf(Title,"BPQ32 Beacon Utility Version %s", VersionString);

	SetWindowText(hWnd, Title);

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil");
	
	retCode = RegOpenKeyEx (REGTREE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &Minimized);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "PortMask", 0,			
			(ULONG *)&Type, (UCHAR *)&UIMask, (ULONG *)&Vallen);

		RegCloseKey(hKey);
	}

	for (i=1; i<=32; i++)
	{
		wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", i);

		retCode = RegOpenKeyEx (REGTREE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

		if (retCode == ERROR_SUCCESS)
		{
			Vallen=4;
			RegQueryValueEx(hKey,"Enabled",0,			
					(ULONG *)&Type,(UCHAR *)&UIEnabled[i],(ULONG *)&Vallen);
	
			Vallen=0;
			RegQueryValueEx(hKey,"Digis",0,			
				(ULONG *)&Type, NULL, (ULONG *)&Vallen);

			if (Vallen)
			{
				UIDigi[i] = malloc(Vallen);
				RegQueryValueEx(hKey,"Digis",0,			
					(ULONG *)&Type, UIDigi[i], (ULONG *)&Vallen);
			}

			Vallen=4;
			retCode = RegQueryValueEx(hKey, "Interval", 0,			
				(ULONG *)&Type, (UCHAR *)&Interval[i], (ULONG *)&Vallen);

			MinCounter[i] = Interval[i];

			Vallen=4;
			retCode = RegQueryValueEx(hKey, "SendFromFile", 0,			
				(ULONG *)&Type, (UCHAR *)&SendFromFile[i], (ULONG *)&Vallen);


			Vallen=10;
			retCode = RegQueryValueEx(hKey, "UIDEST", 0, &Type, &UIDEST[i][0], &Vallen);

			Vallen=255;
			retCode = RegQueryValueEx(hKey, "FileName", 0, &Type, &FN[i][0], &Vallen);

			Vallen=999;
			retCode = RegQueryValueEx(hKey, "Message", 0, &Type, &Message[i][0], &Vallen);

			SetupUI(i);

			RegCloseKey(hKey);
		}
	}

   	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	MinimizetoTray = GetMinimizetoTrayFlag();

	if (MinimizetoTray)
	{
		wsprintf(Title, "UIRoutine");
		AddTrayMenuItem(hWnd, Title);
	}

	CheckTimer();		// Force BPQ32 to initialise

	SetTimer(hWnd, 1, 5000, NULL);			// Fast Timer for Node CheckTimer()
	SetTimer(hWnd, 2, 60000, NULL);			// Minute Timer

	if (Minimized)
		if (MinimizetoTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(hWnd, SW_RESTORE);

	Sleep(5000);						// Wait for switch to initialise

	OnTabbedDialogInit(hWnd);			// Set up pages

	return (TRUE);
}


VOID TimerProc()
{
	int Port, MaxPorts = GetNumberofPorts();

	for (Port = 1; Port <= MaxPorts; Port++)
	{
		if (MinCounter[Port])
		{
			MinCounter[Port] --;

			if (MinCounter[Port] == 0)
			{
				MinCounter[Port] = Interval[Port];
				SendBeacon(Port);
			}
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	int retCode, disp;
	RECT Rect;

	switch (message) { 

	case WM_INITDIALOG:
		OnTabbedDialogInit(hWnd);
		return (INT_PTR)TRUE;

	case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
		case TCN_SELCHANGE:
			 OnSelChanged(hWnd);
				 return TRUE;
         // More cases on WM_NOTIFY switch.
		case NM_CHAR:
			return TRUE;
        }

       break;


		case WM_TIMER:

			if (wParam == 1)
				CheckTimer();			// Check Node Timer
			else
				TimerProc();

			return 0;

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

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId) {

		case IDOK:


			return TRUE;

		default:

			return 0;
		}


		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId)
		{
		case SC_RESTORE:

			Minimized = FALSE;
			return (DefWindowProc(hWnd, message, wParam, lParam));

		case  SC_MINIMIZE: 

			Minimized = TRUE;
			
			if (MinimizetoTray)
				return ShowWindow(hWnd, SW_HIDE);
			else
				return (DefWindowProc(hWnd, message, wParam, lParam));
						
			break;
		
		default:
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_CLOSE:

			ShowWindow(hWnd, SW_RESTORE);
			GetWindowRect(hWnd, &Rect);

			strcpy(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil");
	
			retCode = RegCreateKeyEx(REGTREE, Key, 0, 0, 0,
				KEY_ALL_ACCESS, NULL, &hKey, &disp);

			if (retCode == ERROR_SUCCESS)
			{
				wsprintf(Size,"%d,%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom, Minimized);
				retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));
				RegCloseKey(hKey);
			}

			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}

	return (0);
}

UINT * GetBuffer()
{
	// Get Buffer from Free Queue
	
	UINT * Buff = Q_REM(&FREE_Q);

	if (Buff)
		QCount--;

	return Buff;
}

// Get buffer from Queue

UINT * Q_REM(UINT *Q)
{
	UINT  * first,next;

	(int)first = Q[0];
	if (first == 0) return (0);			// Empty
	
	next=first[0];						// Address of next buffer
	Q[0]=next;
	return (first);
}


// Return Buffer to Free Queue

VOID ReleaseBuffer(UINT *BUFF)
{
	UINT * pointer;
	
	(UINT)pointer=FREE_Q;
	*BUFF=(UINT)pointer;
	FREE_Q=(UINT)BUFF;

	QCount++;

}


VOID Q_ADD(UINT *Q,UINT *BUFF)
{
	UINT * next;
	
	BUFF[0]=0;							// Clear chain in new buffer
	if (Q[0] == 0)						// Empty
	{
		Q[0]=(UINT)BUFF;				// New one on front
		return;
	}

	(int)next=Q[0];

	while (next[0]!= 0)
		next = (UINT *)next[0];			// Chain to end of queue

	next[0] = (UINT)BUFF;				// New one on end
}

VOID SetupUI(int Port)
{
	char DigiString[100], * DigiLeft;

	ConvToAX25(GetNodeCall(), MYCALL);
	ConvToAX25(UIDEST[Port], &AXDEST[Port][0]);

	UIDigiLen[Port] = 0;

	if (UIDigi[Port])
	{
		UIDigiAX[Port] = zalloc(100);
		strcpy(DigiString, UIDigi[Port]);
		DigiLeft = strlop(DigiString,',');

		while(DigiString[0])
		{
			ConvToAX25(DigiString, &UIDigiAX[Port][UIDigiLen[Port]]);
			UIDigiLen[Port] += 7;

			if (DigiLeft)
			{
				_asm
				{
					mov eax,dword ptr [DigiLeft] 
					push eax
					lea	eax,[DigiString] 
					push eax
					call strcpy				// Must compile this with no Optimisations
					add	esp,8
				}
				DigiLeft = strlop(DigiString,',');
			}
			else
				DigiString[0] = 0;
		}
	}
}


VOID Send_AX_Datagram(UCHAR * Msg, DWORD Len, UCHAR Port, UCHAR * HWADDR, BOOL Queue)
{
	MESSAGEX AXMSG;
	PMESSAGEX AXPTR = &AXMSG;
	int DataLen = Len;

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXPTR->DEST, HWADDR, 7);
	memcpy(AXPTR->ORIGIN, MYCALL, 7);
	AXPTR->DEST[6] &= 0x7e;			// Clear End of Call
	AXPTR->DEST[6] |= 0x80;			// set Command Bit

	if (UIDigi[Port])
	{
		// This port has a digi string

		int DigiLen = UIDigiLen[Port];

		memcpy(&AXPTR->CTL, UIDigiAX[Port], DigiLen);
		
		_asm{					// Adjusting pointers to structs is difficult in C!
			mov	eax,AXPTR
			add eax, DigiLen
			mov AXPTR,EAX
		}

		Len += DigiLen;

	}
	AXPTR->ORIGIN[6] |= 1;			// Set End of Call
	AXPTR->CTL = 3;		//UI
	AXPTR->PID = 0xf0;
	memcpy(AXPTR->DATA, Msg, DataLen);

//	if (Queue)
//		QueueRaw(Port, &AXMSG, Len + 16);
//	else
		SendRaw(Port, (char *)&AXMSG.DEST, Len + 16);

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



VOID SendBeacon(int Port)
{
	char UIMessage[1024];
	int Len = strlen(Message[Port]);
	int Index = 0;

	if (SendFromFile[Port])
	{
		HANDLE Handle;

		Handle = CreateFile(FN[Port], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (Handle == INVALID_HANDLE_VALUE)
			return;
		
		ReadFile(Handle, &UIMessage, 1024, &Len, NULL); 
		CloseHandle(Handle);

	}
	else
		strcpy(UIMessage, Message[Port]);

	Len =  RemoveLF(UIMessage, Len);

	while (Len > 256)
	{
		Send_AX_Datagram(&UIMessage[Index], 256, Port, AXDEST[Port], TRUE);
		Index += 256;
		Len -= 256;
		Sleep(2000);
	}
	Send_AX_Datagram(&UIMessage[Index], Len, Port, AXDEST[Port], TRUE);
	Sleep(2000);
}


INT_PTR CALLBACK ChildDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	This processes messages from controls on the tab subpages
	int Command;

	int retCode, disp;
	char Key[80];
	HKEY hKey;
	BOOL OK;
	OPENFILENAME ofn;
	char Digis[100];

	int Port = PortNum[CurrentPage];


	switch (message)
	{
	case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
		case TCN_SELCHANGE:
			 OnSelChanged(hDlg);
				 return TRUE;
         // More cases on WM_NOTIFY switch.
		case NM_CHAR:
			return TRUE;
        }

       break;
	case WM_INITDIALOG:
		OnChildDialogInit( hDlg);
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
			case IDC_FILE:

			memset(&ofn, 0, sizeof (OPENFILENAME));
			ofn.lStructSize = sizeof (OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = &FN[Port][0];
			ofn.nMaxFile = 250;
			ofn.lpstrTitle = "File to send as beacon";
			ofn.lpstrInitialDir = GetBPQDirectory();

			if (GetOpenFileName(&ofn))
				SetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0]);

			break;


		case IDOK:

			GetDlgItemText(hDlg, IDC_UIDEST, &UIDEST[Port][0], 10);

			if (UIDigi[Port])
			{
				free(UIDigi[Port]);
				UIDigi[Port] = NULL;
			}

			if (UIDigiAX[Port])
			{
				free(UIDigiAX[Port]);
				UIDigiAX[Port] = NULL;
			}

			GetDlgItemText(hDlg, IDC_UIDIGIS, Digis, 99); 
		
			UIDigi[Port] = _strdup(Digis);
		
			GetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0], 255); 
			GetDlgItemText(hDlg, IDC_MESSAGE, &Message[Port][0], 1000); 
	
			Interval[Port] = GetDlgItemInt(hDlg, IDC_INTERVAL, &OK, FALSE); 

			MinCounter[Port] = Interval[Port];

			SendFromFile[Port] = IsDlgButtonChecked(hDlg, IDC_FROMFILE);

			wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", PortNum[CurrentPage]);

			retCode = RegCreateKeyEx(REGTREE,
					Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);
	
			if (retCode == ERROR_SUCCESS)
			{
				retCode = RegSetValueEx(hKey, "UIDEST", 0, REG_SZ,(BYTE *)&UIDEST[Port][0], strlen(&UIDEST[Port][0]));
				retCode = RegSetValueEx(hKey, "FileName", 0, REG_SZ,(BYTE *)&FN[Port][0], strlen(&FN[Port][0]));
				retCode = RegSetValueEx(hKey, "Message", 0, REG_SZ,(BYTE *)&Message[Port][0], strlen(&Message[Port][0]));
				retCode = RegSetValueEx(hKey, "Interval", 0, REG_DWORD,(BYTE *)&Interval[Port], 4);
				retCode = RegSetValueEx(hKey, "SendFromFile", 0, REG_DWORD,(BYTE *)&SendFromFile[Port], 4);
				retCode = RegSetValueEx(hKey, "Enabled", 0, REG_DWORD,(BYTE *)&UIEnabled[Port], 4);
				retCode = RegSetValueEx(hKey, "Digis",0, REG_SZ, Digis, strlen(Digis));

				RegCloseKey(hKey);
			}

			SetupUI(Port);

			return (INT_PTR)TRUE;


		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case ID_TEST:

			SendBeacon(Port);
			return TRUE;

		}
		break;

	}	
	return (INT_PTR)FALSE;
}


VOID WINAPI OnTabbedDialogInit(HWND hDlg)
{
	DLGHDR *pHdr = (DLGHDR *) LocalAlloc(LPTR, sizeof(DLGHDR));
	DWORD dwDlgBase = GetDialogBaseUnits();
	int cxMargin = LOWORD(dwDlgBase) / 4;
	int cyMargin = HIWORD(dwDlgBase) / 8;

	TC_ITEM tie;
	RECT rcTab;

	int i, pos, tab = 0;
	INITCOMMONCONTROLSEX init;

	char PortNo[60];
	struct _EXTPORTDATA * PORTVEC;

	hwndDlg = hDlg;			// Save Window Handle

	// Save a pointer to the DLGHDR structure.

	SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pHdr);

	// Create the tab control.


	init.dwICC = ICC_STANDARD_CLASSES;
	init.dwSize=sizeof(init);
	i=InitCommonControlsEx(&init);

	pHdr->hwndTab = CreateWindow(WC_TABCONTROL, "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, 100, 100, hwndDlg, NULL, hInst, NULL);

	if (pHdr->hwndTab == NULL) {

	// handle error

	}

	// Add a tab for each of the child dialog boxes.

	tie.mask = TCIF_TEXT | TCIF_IMAGE;

	tie.iImage = -1;

	for (i = 1; i <= GetNumberofPorts(); i++)
	{
		// Only allow UI on ax.25 ports

		PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntry(i);

		if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
		{
			if (strstr(PORTVEC->PORT_DLL_NAME, "UZ7") || strstr(PORTVEC->PORT_DLL_NAME, "TRKMULTI") ||
					strstr(PORTVEC->PORT_DLL_NAME, "TRACKER"))
				goto OK;
		}
		if (PORTVEC->PORTCONTROL.PROTOCOL == 10)	// Pactor/WINMOR
			continue;
OK:
		wsprintf(PortNo, "Port %2d", GetPortNumber(i));
		PortNum[tab] = GetPortNumber(i);

		tie.pszText = PortNo;
		TabCtrl_InsertItem(pHdr->hwndTab, tab, &tie);
	
		pHdr->apRes[tab++] = DoLockDlgRes("PORTPAGE");

	}

	PageCount = tab;

	// Determine the bounding rectangle for all child dialog boxes.

	SetRectEmpty(&rcTab);

	for (i = 0; i < PageCount; i++)
	{
		if (pHdr->apRes[i]->cx > rcTab.right)
			rcTab.right = pHdr->apRes[i]->cx;

		if (pHdr->apRes[i]->cy > rcTab.bottom)
			rcTab.bottom = pHdr->apRes[i]->cy;

	}

	MapDialogRect(hwndDlg, &rcTab);

//	rcTab.right = rcTab.right * LOWORD(dwDlgBase) / 4;

//	rcTab.bottom = rcTab.bottom * HIWORD(dwDlgBase) / 8;

	// Calculate how large to make the tab control, so

	// the display area can accomodate all the child dialog boxes.

	TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);

	OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top);

	// Calculate the display rectangle.

	CopyRect(&pHdr->rcDisplay, &rcTab);

	TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

	// Set the size and position of the tab control, buttons,

	// and dialog box.

	SetWindowPos(pHdr->hwndTab, NULL, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, SWP_NOZORDER);

	// Move the Buttons to bottom of page

	pos=rcTab.left+cxMargin;

	
	// Size the dialog box.

	SetWindowPos(hwndDlg, NULL, 0, 0, rcTab.right + cyMargin + 2 * GetSystemMetrics(SM_CXDLGFRAME),
		rcTab.bottom  + 2 * cyMargin + 2 * GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION),
		SWP_NOMOVE | SWP_NOZORDER);

	// Simulate selection of the first item.

	OnSelChanged(hwndDlg);

}

// DoLockDlgRes - loads and locks a dialog template resource.

// Returns a pointer to the locked resource.

// lpszResName - name of the resource

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName)
{
	HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);
	HGLOBAL hglb = LoadResource(hInst, hrsrc);

	return (DLGTEMPLATE *) LockResource(hglb);
}

//The following function processes the TCN_SELCHANGE notification message for the main dialog box. The function destroys the dialog box for the outgoing page, if any. Then it uses the CreateDialogIndirect function to create a modeless dialog box for the incoming page.

// OnSelChanged - processes the TCN_SELCHANGE notification.

// hwndDlg - handle of the parent dialog box

VOID WINAPI OnSelChanged(HWND hwndDlg)
{
	char PortDesc[40];
	int Port;

	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	CurrentPage = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Destroy the current child dialog box, if any.

	if (pHdr->hwndDisplay != NULL)

		DestroyWindow(pHdr->hwndDisplay);

	// Create the new child dialog box.

	pHdr->hwndDisplay = CreateDialogIndirect(hInst, pHdr->apRes[CurrentPage], hwndDlg, ChildDialogProc);

	hwndDisplay = pHdr->hwndDisplay;		// Save

	Port = PortNum[CurrentPage];
	// Fill in the controls

	GetPortDescription(PortNum[CurrentPage], PortDesc);

	SetDlgItemText(hwndDisplay, IDC_PORTNAME, PortDesc);

	CheckDlgButton(hwndDisplay, IDC_FROMFILE, SendFromFile[Port]);

	SetDlgItemInt(hwndDisplay, IDC_INTERVAL, Interval[Port], FALSE);

	SetDlgItemText(hwndDisplay, IDC_UIDEST, &UIDEST[Port][0]);
	SetDlgItemText(hwndDisplay, IDC_UIDIGIS, UIDigi[Port]);



	SetDlgItemText(hwndDisplay, IDC_FILENAME, &FN[Port][0]);
	SetDlgItemText(hwndDisplay, IDC_MESSAGE, &Message[Port][0]);

	ShowWindow(pHdr->hwndDisplay, SW_SHOWNORMAL);

}

//The following function processes the WM_INITDIALOG message for each of the child dialog boxes. You cannot specify the position of a dialog box created using the CreateDialogIndirect function. This function uses the SetWindowPos function to position the child dialog within the tab control's display area.

// OnChildDialogInit - Positions the child dialog box to fall

// within the display area of the tab control.

VOID WINAPI OnChildDialogInit(HWND hwndDlg)
{
	HWND hwndParent = GetParent(hwndDlg);
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndParent, GWL_USERDATA);

	SetWindowPos(hwndDlg, HWND_TOP, pHdr->rcDisplay.left, pHdr->rcDisplay.top, 0, 0, SWP_NOSIZE);
}
