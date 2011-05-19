
//	Program to Generate one time passwords for BPQ Mail Authentication

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "commctrl.h"

#include "BPQAuth.h"
#define AUTH
#include "GetVersion.h"


ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);

char ClassName[]="AUTHMAINWINDOW";					// the main window class name

HWND hWnd;



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
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(BPQICON) );
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

/*
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
*/

	SetTimer(hWnd, 1, 1000, NULL);

	ShowWindow(hWnd, SW_RESTORE);

	return (TRUE);
}


VOID TimerProc()
{
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

		case WM_CLOSE:

			ShowWindow(hWnd, SW_RESTORE);
			GetWindowRect(hWnd, &Rect);

			strcpy(Key, "SOFTWARE\\G8BPQ\\BPQ32\\AUTH");
	
			retCode = RegCreateKeyEx(HKEY_CURRENT_USER, Key, 0, 0, 0,
				KEY_ALL_ACCESS, NULL, &hKey, &disp);

			if (retCode == ERROR_SUCCESS)
			{
				wsprintf(Size,"%d,%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom, Minimized);
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

