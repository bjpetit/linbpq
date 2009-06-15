// Mail and Chat Server for BPQ32 Packet Switch
//
//	Console Window Module

#include "stdafx.h"

char ClassName[]="BPQMAINWINDOW";

char SYSOPCall[50];

CIRCUIT * Console;

struct UserInfo * user;


WNDPROC wpOrigInputProc; 
WNDPROC wpOrigOutputProc; 
//WNDPROC wpOrigMonProc; 
//WNDPROC wpOrigSplitProc; 

HWND hConsole;
HWND hwndInput;
HWND hwndOutput;
//HWND hwndMon;
//HWND hwndSplit;

HMENU hMenu;		// handle of menu 


#define InputBoxHeight 25
RECT Rect;
//RECT MonRect;
RECT OutputRect;
//RECT SplitRect;

int Height, Width, LastY;

char kbbuf[160];
int kbptr=0;
char readbuff[1024];

BOOL Bells = TRUE;
BOOL StripLF = TRUE;
BOOL LogMonitor = FALSE;
BOOL LogOutput = FALSE;
BOOL SendDisconnected = TRUE;



int PartLinePtr=0;
int PartLineIndex=0;		// Listbox index of (last) incomplete line


LRESULT CALLBACK ConsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
LRESULT APIENTRY SplitProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
void MoveWindows();

#define BGCOLOUR RGB(236,233,216)

BOOL CreateConsole()
{
    WNDCLASS  wc;
	HBRUSH bgBrush;

	if (hConsole)
		return FALSE;							// Alreaqy open

	bgBrush = CreateSolidBrush(BGCOLOUR);

    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = ConsWndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, MAKEINTRESOURCE(IDI_BPQMailChat) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	hConsole=CreateDialog(hInst,ClassName,0,NULL);
	
    if (!hConsole)
        return (FALSE);

	hMenu=GetMenu(hConsole);

	if (Bells & 1)
		CheckMenuItem(hMenu,BPQBELLS, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQBELLS, MF_UNCHECKED);

  	if (StripLF & 1)
		CheckMenuItem(hMenu,BPQStripLF, MF_CHECKED);
	else
		CheckMenuItem(hMenu,BPQStripLF, MF_UNCHECKED);

	DrawMenuBar(hWnd);	



	// Retrieve the handlse to the edit controls. 

	hwndInput = GetDlgItem(hConsole, 118); 
	hwndOutput = GetDlgItem(hConsole, 117); 
//	hwndSplit = GetDlgItem(hConsole, 119); 
//	hwndMon = GetDlgItem(hConsole, 116); 
 
	// Set our own WndProcs for the controls. 

	wpOrigInputProc = (WNDPROC) SetWindowLong(hwndInput, GWL_WNDPROC, (LONG) InputProc); 
	wpOrigOutputProc = (WNDPROC)SetWindowLong(hwndOutput, GWL_WNDPROC, (LONG)OutputProc);
//	wpOrigMonProc = (WNDPROC)SetWindowLong(hwndMon, GWL_WNDPROC, (LONG)MonProc);
//	wpOrigSplitProc = (WNDPROC)SetWindowLong(hwndSplit, GWL_WNDPROC, (LONG)SplitProc);

		if (cfgMinToTray)
			AddTrayMenuItem(hConsole, "Mail/Chat Console");


	ShowWindow(hConsole, SW_SHOWNORMAL);

	MoveWindows();

	Console = zalloc(sizeof(CIRCUIT));

	Console->Active = TRUE;
	Console->BPQStream = -1;

	strcpy(Console->Callsign, SYSOPCall);

	user = LookupCall(SYSOPCall);

	if (user == NULL)
	{
		user = AllocateUserRecord(SYSOPCall);

		if (user == NULL) return 0; //		Cant happen??
	}

	time(&user->TimeLastCOnnected);

	Console->UserPointer = user;
	Console->lastmsg = user->lastmsg;
	Console->paclen=236;
	Console->sysop = TRUE;

	SendUnbuffered(-1, BBSSID, strlen(BBSSID));

	if (user->Name[0] == 0)
	{
		Console->Flags |= GETTINGUSER;
		SendUnbuffered(-1, NewUserPrompt, strlen(NewUserPrompt));
	}
	else
		SendWelcomeMsg(-1, Console, user);

	return TRUE;

}


void MoveWindows()
{
	RECT rcMain, rcClient;
	int ClientHeight, ClientWidth;

	GetWindowRect(hConsole, &rcMain);
	GetClientRect(hConsole, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

//	MoveWindow(hwndMon,2, 0, ClientWidth-4, SplitPos, TRUE);
	MoveWindow(hwndOutput,2, 2, ClientWidth-4, ClientHeight-InputBoxHeight-4, TRUE);
	MoveWindow(hwndInput,2, ClientHeight-InputBoxHeight-2, ClientWidth-4, InputBoxHeight, TRUE);
//	MoveWindow(hwndSplit,0, SplitPos, ClientWidth, SplitBarHeight, TRUE);
}


LRESULT CALLBACK ConsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LPRECT lprc;
	
	switch (message) { 

		case WM_ACTIVATE:

			SetFocus(hwndInput);
			break;


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) {

		case BPQBELLS:

			ToggleParam(hWnd, &Bells, BPQBELLS);
			break;

		case BPQStripLF:

			ToggleParam(hWnd, &StripLF, BPQStripLF);
			break;

		case BPQLogOutput:

			ToggleParam(hWnd, &LogOutput, BPQLogOutput);
			break;


		case BPQLogMonitor:

			ToggleParam(hWnd, &LogMonitor, BPQLogMonitor);
			break;

		case BPQCLEAROUT:

			SendMessage(hwndOutput,LB_RESETCONTENT, 0, 0);		
			break;

		case BPQCOPYOUT:
		
			CopyToClipboard(hwndOutput);
			break;



		//case BPQHELP:

		//	HtmlHelp(hWnd,"BPQTerminal.chm",HH_HELP_FINDER,0);  
		//	break;

		default:

			return 0;

		}

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

		case  SC_MINIMIZE: 

			if (cfgMinToTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_SIZING:

			lprc = (LPRECT) lParam;

			Height = lprc->bottom-lprc->top;
			Width = lprc->right-lprc->left;

			MoveWindows();
			
			return TRUE;


		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 

			GetWindowRect(hWnd,	&Rect);	// For save soutine

            SetWindowLong(hwndInput, GWL_WNDPROC, 
                (LONG) wpOrigInputProc); 
         

			if (cfgMinToTray) 
				DeleteTrayMenuItem(hWnd);


			if (Console->Active)
			{
				ClearQueue(Console);
		
				Console->Active = FALSE;
				ShowConnections();

				if (Console->Flags & CHATMODE)
				{
					logout(Console);
				}
				else
				{
					SendUnbuffered(Console->BPQStream, SignoffMsg, strlen(SignoffMsg));
					user->lastmsg = Console->lastmsg;
				}
			}

			Sleep(500);

			free(Console);
			Console = 0;
			hConsole = NULL;
			
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}



LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
{ 
	int index;
	char Temp[255];

	if (uMsg == WM_CHAR) 
	{
		if (wParam == 13)
		{
			//
			//	if not connected, and autoconnect is
			//	enabled, connect now
						
			kbptr=SendMessage(hwndInput,WM_GETTEXT,159,(LPARAM) (LPCSTR) kbbuf);
	
			// Echo

			if (PartLinePtr != 0)
			{
				// Last Line was not terminated with LF - append input text to it
				
				SendMessage(hwndOutput,LB_GETTEXT,PartLineIndex,(LPARAM)(LPCTSTR) Temp );
				strcat(Temp, kbbuf);
				PartLinePtr=0;
				index=SendMessage(hwndOutput,LB_DELETESTRING,PartLineIndex,(LPARAM)(LPCTSTR) &Temp[0] );		
				index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) &Temp[0] );		
			}
			else
				index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) &kbbuf[0] );		
			
			SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) index, MAKELPARAM(FALSE, 0));

			// Replace null with CR, and send to Node

			kbbuf[kbptr]=13;

			ProcessLine(Console, user, &kbbuf[0], kbptr+1);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
		if (wParam == 0x1a)  // Ctrl/Z
		{
	
			kbbuf[0]=0x1a;
			kbbuf[1]=13;

//			SendMsg(Stream, &kbbuf[0], 2);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

        return 0; 
		}

	}
 
    return CallWindowProc(wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 

LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	// Trap mouse messages, so we cant select stuff in output and mon windows,
	//	otherwise scrolling doesnt work.

	if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) 
        return TRUE; 

	return CallWindowProc(wpOrigOutputProc, hwnd, uMsg, wParam, lParam); 
} 

int WritetoConsoleWindow(char * Msg, int len)
{
	char * ptr1, * ptr2;
	int index;

	if (PartLinePtr != 0)
		SendMessage(hwndOutput,LB_DELETESTRING,PartLineIndex,(LPARAM)(LPCTSTR) 0 );		

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
			memmove(readbuff,ptr1,len);
			PartLineIndex=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
			SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) PartLineIndex, MAKELPARAM(FALSE, 0));

			return (0);

		}
		else
		{
			*(ptr2++)=0;

			index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
					
	//		if (LogOutput) WriteMonitorLine(ptr1, ptr2 - ptr1);

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

	return (0);
}

int ToggleParam(HWND hWnd, BOOL * Param, int Item)
{
	*Param = !(*Param);

	CheckMenuItem(hMenu,Item, (*Param) ? MF_CHECKED : MF_UNCHECKED);
	
    return (0);
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


