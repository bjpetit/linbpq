
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>


//#include "bpqterm.h"
#include "..\include\bpq32.h"			// BPQ32 API Defines

#define BPQICON 400

HINSTANCE hInst; 
char AppName[] = "BPQ32";
char Title[80] = "Program to hold BPQ32.dll in memory";


// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY SetupTrayIcon();

int TimerHandle = 0;


LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;

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

	KillTimer(NULL,TimerHandle);

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

	// Fill in window class structure with parameters that describe
    // the main window.
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = (WNDPROC)WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE(BPQICON));
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

//		wc.lpszMenuName =  MAKEINTRESOURCE(BPQMENU) ;
 
        wc.lpszClassName = AppName;

        // Register the window class and return success/failure code.

		return RegisterClass(&wc);
      
}

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
	HWND hWnd;
	hInst = hInstance; // Store instance handle in our global variable

	MinimizetoTray=GetMinimizetoTrayFlag();


	hWnd = CreateWindow(AppName, Title, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 350,50,
		NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return (FALSE);
	}
	
	
	// setup default font information

   LFTTYFONT.lfHeight =			12;
   LFTTYFONT.lfWidth =          8 ;
   LFTTYFONT.lfEscapement =     0 ;
   LFTTYFONT.lfOrientation =    0 ;
   LFTTYFONT.lfWeight =         0 ;
   LFTTYFONT.lfItalic =         0 ;
   LFTTYFONT.lfUnderline =      0 ;
   LFTTYFONT.lfStrikeOut =      0 ;
   LFTTYFONT.lfCharSet =        OEM_CHARSET ;
   LFTTYFONT.lfOutPrecision =   OUT_DEFAULT_PRECIS ;
   LFTTYFONT.lfClipPrecision =  CLIP_DEFAULT_PRECIS ;
   LFTTYFONT.lfQuality =        DEFAULT_QUALITY ;
   LFTTYFONT.lfPitchAndFamily = FIXED_PITCH | FF_MODERN ;
   lstrcpy( LFTTYFONT.lfFaceName, "Fixedsys" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;
	
	SetWindowText(hWnd,Title);
		
	if (MinimizetoTray)
	{
		//	Set up Tray ICON

		AddTrayMenuItem(hWnd, "BPQ32.EXE");
	}

	TimerHandle=SetTimer(hWnd,WM_TIMER,5000,NULL);

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (MinimizetoTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);


	CheckTimer();

	SetupTrayIcon();


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
	int i;

	switch (message) { 

		case WM_TIMER:

			CheckTimer();

			return 0;


		case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

	
		case  SC_MINIMIZE: 

			if (MinimizetoTray)

				return ShowWindow(hWnd, SW_HIDE);
			else
				return (DefWindowProc(hWnd, message, wParam, lParam));
				
				
			break;
		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}


	case WM_CLOSE:
		
			
			i=GetAttachedProcesses();

			if (i > 1)
			{
				if (MessageBox (GetFocus(),"Other BPQ32 programs are connected\nPress OK to close, CANCEL to leave running",
                           	"BPQ32", MB_OKCANCEL) == IDCANCEL)
					break;
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


