
//	Program to send ax.25 Beacons from a BPQ32 Switch

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "commctrl.h"

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


HINSTANCE hInst; 

UINT UIMask = 0;

#define MAXFLEN 400     /* Maximale Laenge eines Frames */
                        /* maximum length of a frame */
typedef signed short    i16;
typedef unsigned char   byte;
typedef unsigned long   u32;

HWND hCheck[33];
HWND hLabel[33];
HWND hUIBox[33];

UINT UIPortMask = 0;
BOOL UIEnabled[33];
char * UIDigi[33];
char * UIDigiAX[33];		// ax.25 version of digistring
int UIDigiLen[33];			// Length of AX string

char UIDEST[11];			// Dest for Beacons

char AXDEST[7];
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
VOID SetupUI();
VOID SendBeacons();

int TimerHandle = 0;

char ClassName[]="UIMAINWINDOW";					// the main window class name

HWND hWnd;

LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;
BOOL Minimized = FALSE;

HWND hPTT;
HWND hDCD;

int State;

UCHAR FN[256] = "";	// Filename
int Interval;		// Beacon Interval (Mins)
BOOL SendFromFile;
char Message[1000];	// Beacon Text

typedef UINT (FAR *FARPROCX)();

UINT (FAR *SoundModem)() = NULL;

HINSTANCE ExtDriver=0;
HANDLE hRXThread;

UINT TXQ;

int ConfigNo;
char Config[20];

UINT PORTPERSISTANCE = 64;
char TXDELAY[10] = "500";

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

	return (msg.wParam);
}

HBRUSH bgBrush;

#define BGCOLOUR RGB(236,233,216)

int CreateDialogLine(HWND hWnd, int i, int row)
{
	char PortNo[60];
	char PortDesc[31];

	// Only allow UI on ax.25 ports

	struct _EXTPORTDATA * PORTVEC;

	PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntry(i);

	if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
		if (PORTVEC->PORTCONTROL.PROTOCOL == 10)	// Pactor/WINMOR
			return FALSE;


	GetPortDescription(i, PortDesc);
	wsprintf(PortNo, "Port %2d %30s", GetPortNumber(i), PortDesc);
	
	hCheck[i] = CreateWindow(WC_BUTTON , "", BS_AUTOCHECKBOX  | WS_CHILD | WS_VISIBLE,
                 50,row+7,14,14, hWnd, NULL, hInst, NULL);

	Button_SetCheck(hCheck[i], UIEnabled[i]);

	hLabel[i] = CreateWindow(WC_STATIC , PortNo,  WS_CHILD | WS_VISIBLE,
                 80,row+5,300,22, hWnd, NULL, hInst, NULL);
	
	SendMessage(hLabel[i], WM_SETFONT,(WPARAM) hFont, 0);


	hUIBox[i] = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT , "", WS_CHILD | WS_BORDER | WS_VISIBLE | ES_UPPERCASE,
                 400,row,200,22, hWnd, NULL, hInst, NULL);

	SendMessage(hUIBox[i], WM_SETFONT,(WPARAM) hFont, 0);
	SetWindowText(hUIBox[i], UIDigi[i]);

	return TRUE;
}

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

	GetVersionInfo(NULL);
	wsprintf(Title,"BPQ32 Beacon Utility Version %s", VersionString);

	SetWindowText(hWnd, Title);

	if (!hWnd)
	{
		err=GetLastError();
		return FALSE;
	}

	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil");
	
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

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

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "Interval", 0,			
			(ULONG *)&Type, (UCHAR *)&Interval, (ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey, "SendFromFile", 0,			
			(ULONG *)&Type, (UCHAR *)&SendFromFile, (ULONG *)&Vallen);

		CheckDlgButton(hWnd, IDC_FROMFILE, SendFromFile);

		SetDlgItemInt(hWnd, IDC_INTERVAL, Interval, FALSE);

		Vallen=10;
		retCode = RegQueryValueEx(hKey, "UIDEST", 0, &Type, &UIDEST[0], &Vallen);
		SetDlgItemText(hWnd, IDC_UIDEST, UIDEST);

		Vallen=255;
		retCode = RegQueryValueEx(hKey, "FileName", 0, &Type, &FN[0], &Vallen);
		SetDlgItemText(hWnd, IDC_FILENAME, FN);

		Vallen=999;
		retCode = RegQueryValueEx(hKey, "Message", 0, &Type, &Message[0], &Vallen);
		SetDlgItemText(hWnd, IDC_MESSAGE, Message);
		RegCloseKey(hKey);
	}

	for (i=1; i<=32; i++)
	{
		wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", i);

		retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
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
			
			RegCloseKey(hKey);
		}
	}

	SetupUI();

   	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	MinimizetoTray = GetMinimizetoTrayFlag();

	if (MinimizetoTray)
	{
		wsprintf(Title, "UIRoutine");
		AddTrayMenuItem(hWnd, Title);
	}

	CheckTimer();		// Force BPQ32 to initialise

	for (i = 1; i <= GetNumberofPorts(); i++)
		if (CreateDialogLine(hWnd, i, Row))
			Row += 30;

	GetWindowRect(hWnd, &Rect);      
	SetWindowPos(hWnd, HWND_TOP, Rect.left, Rect.top, 650, Row+100, 0);
	SetWindowPos(GetDlgItem(hWnd, IDOK), NULL, 260, Row+20, 60, 30, 0);
	SetWindowPos(GetDlgItem(hWnd, ID_TEST), NULL, 330, Row+20, 60, 30, 0);
		
	if (Minimized)
		if (MinimizetoTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(hWnd, SW_RESTORE);

	return (TRUE);
}


VOID TimerProc()
{
	 SendBeacons();
}

VOID Free_UI()
{
	int i;

	for (i = 1; i <= 32; i++)
	{
		if (UIDigi[i])
		{
			free(UIDigi[i]);
			UIDigi[i] = NULL;
		}

		if (UIDigiAX[i])
		{
			free(UIDigiAX[i]);
			UIDigiAX[i] = NULL;
		}
	}
}

SaveUIConfig()
{
	int Num = GetNumberofPorts();
	int i, Len, retCode, disp;
	char Key[80];
	HKEY hKey;

	Free_UI();

	for (i=1; i<=Num; i++)
	{
		UIEnabled[i] =  Button_GetCheck(hCheck[i]);
	
		Len = GetWindowTextLength(hUIBox[i]);
	
		UIDigi[i] = malloc(Len+1);
		GetWindowText(hUIBox[i], UIDigi[i], Len+1);
		
		wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", i);

		retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

		if (retCode == ERROR_SUCCESS)
		{		
			retCode = RegSetValueEx(hKey, "Enabled", 0, REG_DWORD,(BYTE *)&UIEnabled[i], 4);
			retCode = RegSetValueEx(hKey, "Digis",0, REG_SZ,(BYTE *)UIDigi[i],strlen(UIDigi[i]));

			RegCloseKey(hKey);
		}
	}
	
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	int retCode, disp;
	RECT Rect;
	OPENFILENAME ofn;
	BOOL OK;

	switch (message) { 

		case WM_TIMER:

			TimerProc();
			return 0;

		case WM_CTLCOLORDLG:
			return (LONG)bgBrush;

		case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);

			if (lParam == (LPARAM)hDCD && (State & 0x10) )
			{
				HBRUSH Brush = CreateSolidBrush(RGB(0, 255, 0));
				return (LONG)Brush;
			}

			if (lParam == (LPARAM)hPTT && (State & 0x20) )
			{
				HBRUSH Brush = CreateSolidBrush(RGB(255, 0, 0));
				SetTextColor(hdcStatic, RGB(255, 255, 255));
				return (LONG)Brush;
			}
			return (LONG)bgBrush;
		}

		case WM_COMMAND:

		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId) {

		case IDC_FILE:

			memset(&ofn, 0, sizeof (OPENFILENAME));
			ofn.lStructSize = sizeof (OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = FN;
			ofn.nMaxFile = 250;
			ofn.lpstrTitle = "File to send as beacon";
			ofn.lpstrInitialDir = GetBPQDirectory();

			if (GetOpenFileName(&ofn))
				SetDlgItemText(hWnd, IDC_FILENAME, FN);

			break;

		case IDOK:

			GetDlgItemText(hWnd, IDC_UIDEST, UIDEST, 10); 
			GetDlgItemText(hWnd, IDC_FILENAME, FN, 255); 
			GetDlgItemText(hWnd, IDC_MESSAGE, Message, 1000); 
	
			Interval = GetDlgItemInt(hWnd, IDC_INTERVAL, &OK, FALSE); 
			SendFromFile = IsDlgButtonChecked(hWnd, IDC_FROMFILE);
			
			retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
					"SOFTWARE\\G8BPQ\\BPQ32\\UIUtil", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);
	
			if (retCode == ERROR_SUCCESS)
			{
				retCode = RegSetValueEx(hKey, "UIDEST", 0, REG_SZ,(BYTE *)&UIDEST, strlen(UIDEST));
				retCode = RegSetValueEx(hKey, "FileName", 0, REG_SZ,(BYTE *)&FN, strlen(FN));
				retCode = RegSetValueEx(hKey, "Message", 0, REG_SZ,(BYTE *)&Message, strlen(Message));
				retCode = RegSetValueEx(hKey, "Interval", 0, REG_DWORD,(BYTE *)&Interval, 4);
				retCode = RegSetValueEx(hKey, "SendFromFile", 0, REG_DWORD,(BYTE *)&SendFromFile, 4);

				RegCloseKey(hKey);
			}

			SaveUIConfig();
			SetupUI();

			return TRUE;

		case ID_TEST:

			SendBeacons();
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
	
			retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, Key, 0, 0, 0,
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

VOID SetupUI()
{
	int i;

	ConvToAX25(GetNodeCall(), MYCALL);
	ConvToAX25(UIDEST, AXDEST);

	UIPortMask = 0;

	for (i = 1; i <= GetNumberofPorts(); i++)
	{
		if (UIEnabled[i])
		{
			char DigiString[100], * DigiLeft;

			UIPortMask |= 1 << (i-1);
			UIDigiLen[i] = 0;

			if (UIDigi[i])
			{
				UIDigiAX[i] = zalloc(100);
				strcpy(DigiString, UIDigi[i]);
				DigiLeft = strlop(DigiString,',');

				while(DigiString[0])
				{
					ConvToAX25(DigiString, &UIDigiAX[i][UIDigiLen[i]]);
					UIDigiLen[i] += 7;

					if (DigiLeft)
					{
						_asm
						{
							mov eax,dword ptr [DigiLeft] 
							push eax
							lea	eax,[DigiString] 
							push eax
							call strcpy
							add	esp,8
						}
						DigiLeft = strlop(DigiString,',');
					}
					else
						DigiString[0] = 0;
				}
			}
		}
	}
	if (TimerHandle)
	{
		KillTimer(hWnd, TimerHandle);
		TimerHandle = 0;
	}
	if (Interval)
		TimerHandle = SetTimer(hWnd, WM_TIMER, Interval * 60000, NULL);
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


VOID SendUI(char * Msg, int Len)
{
	int Mask = UIPortMask;
	int NumPorts = GetNumberofPorts();
	int i;

	for (i=1; i <= NumPorts; i++)
	{
		if (Mask & 1)
			Send_AX_Datagram(Msg, Len, i, AXDEST, TRUE);
		
		Mask>>=1;
	}
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



VOID SendBeacons()
{
	char UIMessage[1024];
	int Len = strlen(Message);
	int Index = 0;

	if (SendFromFile)
	{
		HANDLE Handle;

		Handle = CreateFile(FN, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (Handle == INVALID_HANDLE_VALUE)
			return;
		
		ReadFile(Handle, &UIMessage, 1024, &Len, NULL); 
		CloseHandle(Handle);

	}
	else
		strcpy(UIMessage, Message);

	
	Len =  RemoveLF(UIMessage, Len);

	while (Len > 256)
	{
		SendUI(&UIMessage[Index], 256);
		Index += 256;
		Len -= 256;
	}

	SendUI(&UIMessage[Index], Len);
}



