
//	Program to Generate one time passwords for BPQ Mail Authentication

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T
#define BPQICON 2

#include <time.h>


#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "commctrl.h"

#include "BPQAuth.h"
#define AUTH
#include "Versions.h"
#include "GetVersion.h"

#include "..\Kernel\MD5.c"


ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);

char ClassName[]="AUTHMAINWINDOW";					// the main window class name

HWND hWnd;

HINSTANCE hInst;
HANDLE TimerHandle;

int LastNow;
int PassCode;

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

	if (!InitInstance(hInstance, nCmdShow))
		return (FALSE);

	// Main message loop:

	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	KillTimer(NULL, 1);
	
//	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\G8BPQ\\BPQ32\\AUTH", 0, 0, 0,
//				KEY_ALL_ACCESS, NULL, &hKey, &disp);

//	RegSetValueEx(hKey,"PortMask",0,REG_DWORD,(BYTE *)&UIMask,4);

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

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	hWnd=CreateDialog(hInst,ClassName,0,NULL);

	GetVersionInfo(NULL);
	wsprintf(Title,"BPQ32 Auth Utility Version %s", VersionString);

	SetWindowText(hWnd, Title);

	if (!hWnd)
	{
		err=GetLastError();
		return FALSE;
	}


	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\AUTH");
	
	retCode = RegOpenKeyEx (HKEY_CURRENT_USER, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);

//		Vallen=999;
//		retCode = RegQueryValueEx(hKey, "Message", 0, &Type, &Message[0], &Vallen);
//		SetDlgItemText(hWnd, IDC_MESSAGE, Message);

		RegCloseKey(hKey);
	}


	SetTimer(hWnd, 1, 1000, NULL);

	ShowWindow(hWnd, SW_RESTORE);

	return (TRUE);
}

VOID CreateOneTimePassword(char * KeyPhrase);


VOID TimerProc()
{
	char Password[80];
	
	GetDlgItemText(hWnd, IDC_PASS, Password, 79);
	CreateOneTimePassword(Password);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	int retCode, disp;
	RECT Rect;
	int len=0;
	HGLOBAL	hMem;
	char * ptr;
	char PassString[80];

	switch (message) { 

		case WM_COMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			switch (wmId)
			{
			case IDC_PASS:
				
				LastNow = 0;
				break;

			case ID_COPY:

				// Set Current Passcode to Clipboard
			
				len = wsprintf(PassString, "AUTH %d", PassCode);
					
				hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, len + 1);

				if (hMem != 0)
				{
					ptr=GlobalLock(hMem);
	
					if (OpenClipboard(hWnd))
					{
						strcpy(ptr, PassString);

						GlobalUnlock(hMem);
						EmptyClipboard();
						SetClipboardData(CF_TEXT,hMem);
						CloseClipboard();
					}
				}
				else
					GlobalFree(hMem);

				break;
			}

		case WM_TIMER:

			TimerProc();
			return 0;

		case WM_CTLCOLORDLG:
			return (LONG)bgBrush;

		case WM_CLOSE:

			ShowWindow(hWnd, SW_RESTORE);
			GetWindowRect(hWnd, &Rect);

			strcpy(Key, "SOFTWARE\\G8BPQ\\BPQ32\\AUTH");
	
			retCode = RegCreateKeyEx(HKEY_CURRENT_USER, Key, 0, 0, 0,
				KEY_ALL_ACCESS, NULL, &hKey, &disp);

			if (retCode == ERROR_SUCCESS)
			{
				wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
				retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));
				RegCloseKey(hKey);
			}

			PostQuitMessage(0);

			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}

	return (0);
}

VOID CreateOneTimePassword(char * KeyPhrase)
{
	// Create a time dependent One Time Password from the KeyPhrase

	time_t NOW = time(NULL);
	UCHAR Hash[16];
	char Password[20];
	char Key[1000];
	int i, chr;
	long long Val;

	NOW = NOW/30;							// Only Change every 30 secs

	if (NOW == LastNow)
		return;

	LastNow = NOW;

	wsprintf(Key, "%s%x", KeyPhrase, NOW);

	md5(Key, Hash);

	for (i=0; i<16; i++)
	{
		chr = (Hash[i] & 31);
		if (chr > 9) chr += 7;
		
		Password[i] = chr + 48; 
	}

	Password[16] = 0;

	memcpy(&Val, Password, 8);
	PassCode = Val %= 1000000;

	SetDlgItemInt(hWnd, IDC_CODE, PassCode, FALSE);

	return;
}
