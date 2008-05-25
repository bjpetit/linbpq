
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>	
#include <memory.h>


#include "bpqterm.h"
#include "..\include\bpq32.h"			// BPQ32 API Defines



HINSTANCE hInst; 
char AppName[] = "BPQTerm 32";
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


int xsize,ysize;		// Screen size at startup

#define MAXLINELEN 120
#define MAXSCREENLEN 450

int LINELEN=120;
int SCREENLEN=40;


char Screen[MAXLINELEN*MAXSCREENLEN];
char kbbuf[160];
int kbptr=0;
char readbuff[512];

int ptr=0;

int Stream;
int len,count;

char callsign[10];
int state;
int change;
int applmask = 0;
int applflags = 0;


CONNECTED=FALSE;
AUTOCONNECT=TRUE;

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


	sscanf(lpCmdLine,"%x %x",&applmask,&applflags);
	
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
        wc.hIcon         = LoadIcon (hInstance, AppName);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

		wc.lpszMenuName =  MAKEINTRESOURCE(BPQMENU) ;
 
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
	HMENU hMenu;
	hInst = hInstance; // Store instance handle in our global variable

	MinimizetoTray=GetMinimizetoTrayFlag();


	hWnd = CreateWindow(AppName, Title, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 500, SCREENLEN*14+50,
		NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return (FALSE);
	}

	// Getr Screen Size

	xsize=GetSystemMetrics(SM_CXSCREEN);

	ysize=GetSystemMetrics(SM_CYSCREEN);
	
	
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
	

	memset(Screen, ' ', LINELEN*SCREENLEN); 

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

	
	wsprintf(Title,"PAC4/32 Version %d.%d.%d.%d - using stream %d",1,3,0,1,Stream);

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

	SetAppl(Stream,applflags,applmask);
	
	hMenu=GetMenu(hWnd);

	if (applmask & 0x01)	CheckMenuItem(hMenu,BPQAPPL1,MF_CHECKED);
	if (applmask & 0x02)	CheckMenuItem(hMenu,BPQAPPL2,MF_CHECKED);
	if (applmask & 0x04)	CheckMenuItem(hMenu,BPQAPPL3,MF_CHECKED);
	if (applmask & 0x08)	CheckMenuItem(hMenu,BPQAPPL4,MF_CHECKED);
	if (applmask & 0x10)	CheckMenuItem(hMenu,BPQAPPL5,MF_CHECKED);
	if (applmask & 0x20)	CheckMenuItem(hMenu,BPQAPPL6,MF_CHECKED);
	if (applmask & 0x40)	CheckMenuItem(hMenu,BPQAPPL7,MF_CHECKED);
	if (applmask & 0x80)	CheckMenuItem(hMenu,BPQAPPL8,MF_CHECKED);

// CMD_TO_APPL	EQU	1B		; PASS COMMAND TO APPLICATION
// MSG_TO_USER	EQU	10B		; SEND 'CONNECTED' TO USER
// MSG_TO_APPL	EQU	100B		; SEND 'CONECTED' TO APPL
//		0x40 = Send Keepalives

	if (applflags & 0x01)	CheckMenuItem(hMenu,BPQFLAGS1,MF_CHECKED);
	if (applflags & 0x02)	CheckMenuItem(hMenu,BPQFLAGS2,MF_CHECKED);
	if (applflags & 0x04)	CheckMenuItem(hMenu,BPQFLAGS3,MF_CHECKED);
	if (applflags & 0x40)	CheckMenuItem(hMenu,BPQFLAGS4,MF_CHECKED);
	

	GetCallsign(Stream, callsign);

	if (MinimizetoTray)
	{
	
		//	Set up Tray ICON

		AddTrayMenuItem(hWnd, "BPQTERM");
	}





	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);


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
	PAINTSTRUCT ps;
	HDC hdc;
    HFONT    hOldFont ;
	HGLOBAL	hMem;

	int i;

	if (message == BPQMsg)
	{
		if (lParam & BPQDataAvail)
			DoReceivedData(hWnd);
				
		if (lParam & BPQStateChange)
			DoStateChange(hWnd);

		return (0);
	}

	
	switch (message) { 


	case WM_CHAR:

 		if (wParam == 8)
		{
			//	Backspace

			if (kbptr > 0)
				kbptr--;

			kbbuf[kbptr]=0x20;
			Screen[LINELEN*(SCREENLEN-1)+kbptr]=0x20;
			InvalidateRect(hWnd,NULL,FALSE);

			return(0);
		}

		kbbuf[kbptr++]=wParam;

		if (wParam == 13)
		{
			NewLine();
			InvalidateRect(hWnd,NULL,FALSE);

			//
			//	if not connected, and autoconnect is
			//	enabled, connect now
			
			if (!CONNECTED && AUTOCONNECT)
				SessionControl(Stream, 1, 0);
				
			SendMsg(Stream, &kbbuf[0], kbptr);
			kbptr=0;
		}
		else
		{
			Screen[LINELEN*(SCREENLEN-1)-1+kbptr]=wParam;
			InvalidateRect(hWnd,NULL,FALSE);
		}
		
		if (kbptr > LINELEN-1)
			kbptr=LINELEN-1;
		
		return (0);


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		//Parse the menu selections:
		
		switch (wmId) {
        
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

		case BPQCOPY:
		
			//
			//	Copy buffer to clipboard
			//
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,sizeof(Screen));
		
			if (hMem != 0)
			{
				if (OpenClipboard(hWnd))
				{
					CopyScreentoBuffer(GlobalLock(hMem));
					GlobalUnlock(hMem);
					EmptyClipboard();
					SetClipboardData(CF_TEXT,hMem);
					CloseClipboard();
				}
				else
				{
					GlobalFree(hMem);
				}

			}

			break;

		}


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




		case WM_PAINT:

			hdc = BeginPaint (hWnd, &ps);
			
			hOldFont = SelectObject( hdc, hFont) ;
			
			for (i=0; i<SCREENLEN; i++)
			{
				TextOut(hdc,0,i*14,&Screen[i*LINELEN],LINELEN);
			}
			
			SelectObject( hdc, hOldFont ) ;
			EndPaint (hWnd, &ps);
	
			break;        

		case WM_DESTROY:
		
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
			
			CONNECTED=TRUE;

	//		GetCallsign(Stream, callsign);

			
			GetConnectionInfo(Stream, callsign,
										 &port, &sesstype, &paclen,
										 &maxframe, &l4window);

				
			wsprintf(Title,"PAC4/32 - using stream %d - Connected to %s",Stream,callsign);
			SetWindowText(hWnd,Title);
			DisableConnectMenu(hWnd);
			EnableDisconnectMenu(hWnd);

		}
		else
		{
			CONNECTED=FALSE;
			wsprintf(Title,"PAC4/32 - using stream %d - Disconnected",Stream);
			SetWindowText(hWnd,Title);
			DisableDisconnectMenu(hWnd);
			EnableConnectMenu(hWnd);
		}
	}

	return (0);

}
		

DoReceivedData(HWND hWnd)
{
	if (RXCount(Stream) > 0)
	{
		do {
		
			GetMsg(Stream, readbuff,&len,&count);
		
			if (len > 0)
			{
				ProcessBuff(readbuff,len);
				if (count == 0)
					InvalidateRect(hWnd,NULL,FALSE);
			
			}

			} while (count > 0);
		
		
	}
	return (0);
}



int line=0;
int col=0;

int	ProcessBuff(char * buff,int len)
{
	int ptr;

	line=SCREENLEN-1;
	
	ptr=0;

	while (ptr < len) 
	{
		if (buff[ptr] == 13)
		{
			NewLine();
			ptr++;
		}
		else
		{
			Screen[line*LINELEN+col] = buff[ptr];
			col++;
			if (col == LINELEN)
				NewLine();
		
		ptr++;
		}
	}
	return (0);
}
int NewLine()
{
	col=0;
	line++;
	if (line > (SCREENLEN-1))
	{
		memmove(Screen,Screen+LINELEN,LINELEN*(SCREENLEN-1));
		memset(Screen+LINELEN*(SCREENLEN-1),' ',LINELEN);
		line=SCREENLEN-1;
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
	
	applflags = applflags ^ mask;
	
	if (applflags & mask)

		CheckMenuItem(hMenu,Item,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

	SetAppl(Stream,applflags,applmask);

    return (0);
  
}

CopyScreentoBuffer(char * buff)
{
	int i,j;
	char line [MAXLINELEN+2];
	
	for (i=0;i != SCREENLEN; i++)
	{
		memcpy(line,&Screen[i*LINELEN],LINELEN);
		
		//
		//	scan line backwards, and replace last space with crlf
		//

		for (j=LINELEN-1;j != -1; j--)
		{
			if (line[j] == ' ')
				continue;

			if (j == -1)
				break;			// Ignore blank lines

			j++;
			line[j++] = '\r';
			line[j++] = '\n';
		
			memcpy(buff,line,j);
			buff+=j;

			break;
		}



	}
	
	*buff++=0;

	return (0);
}
	
