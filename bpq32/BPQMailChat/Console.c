// Mail and Chat Server for BPQ32 Packet Switch
//
//	Console Window Module

#include "stdafx.h"

char ClassName[]="CONSOLEWINDOW";

char SYSOPCall[50];

CIRCUIT * Console;

struct UserInfo * user;

static WNDPROC wpOrigInputProc; 
static WNDPROC wpOrigOutputProc; 

HWND hConsole;

static HWND hwndInput;
static HWND hwndOutput;

static HMENU hMenu;		// handle of menu 


#define InputBoxHeight 25
RECT ConsoleRect;
RECT OutputRect;

int Height, Width, LastY;

int ClientHeight, ClientWidth;


static char kbbuf[160];
static int kbptr=0;
static char readbuff[101000];

BOOL Bells = TRUE;
BOOL StripLF = TRUE;

BOOL WarnWrap = TRUE;
BOOL FlashOnConnect = TRUE;
BOOL WrapInput = TRUE;
BOOL CloseWindowOnBye = TRUE;

static int WrapLen = 80;
static int WarnLen = 90;
static int maxlinelen = 80;

static int PartLinePtr=0;
static int PartLineIndex=0;		// Listbox index of (last) incomplete line


static LRESULT CALLBACK ConsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
static LRESULT APIENTRY InputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY MonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static LRESULT APIENTRY SplitProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) ;
static void MoveWindows();


#define BGCOLOUR RGB(236,233,216)

BOOL CreateConsole()
{
    WNDCLASS  wc;
	HBRUSH bgBrush;

	if (hConsole)
	{
		ShowWindow(hConsole, SW_SHOWNORMAL);
		SetForegroundWindow(hConsole);
		return FALSE;							// Alreaqy open
	}

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

	CheckMenuItem(hMenu,BPQBELLS, (Bells) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,BPQStripLF, (StripLF) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_WARNINPUT, (WarnWrap) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_WRAPTEXT, (WrapInput) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_Flash, (FlashOnConnect) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(hMenu,IDM_CLOSEWINDOW, (CloseWindowOnBye) ? MF_CHECKED : MF_UNCHECKED);

	DrawMenuBar(hWnd);	

	// Retrieve the handlse to the edit controls. 

	hwndInput = GetDlgItem(hConsole, 118); 
	hwndOutput = GetDlgItem(hConsole, 117); 
 
	// Set our own WndProcs for the controls. 

	wpOrigInputProc = (WNDPROC) SetWindowLong(hwndInput, GWL_WNDPROC, (LONG) InputProc); 
	wpOrigOutputProc = (WNDPROC)SetWindowLong(hwndOutput, GWL_WNDPROC, (LONG)OutputProc);

	if (cfgMinToTray)
		AddTrayMenuItem(hConsole, "Mail/Chat Console");

	ShowWindow(hConsole, SW_SHOWNORMAL);

	if (ConsoleRect.right < 100 || ConsoleRect.bottom < 100)
	{
		GetWindowRect(hConsole,	&ConsoleRect);
	}

	MoveWindow(hConsole,ConsoleRect.left,ConsoleRect.top, ConsoleRect.right-ConsoleRect.left, ConsoleRect.bottom-ConsoleRect.top, TRUE);

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

	Console->PageLen = user->PageLen;
	Console->Paging = (user->PageLen > 0);

	nodeprintf(Console, BBSSID, Ver[0], Ver[1], Ver[2], Ver[3],
		ALLOWCOMPRESSED ? "B" : "", "", "");

	if (user->Name[0] == 0)
	{
		Console->Flags |= GETTINGUSER;
		SendUnbuffered(-1, NewUserPrompt, strlen(NewUserPrompt));
	}
	else
		SendWelcomeMsg(-1, Console, user);

	return TRUE;

}

VOID CloseConsole(int Stream)
{
	if (CloseWindowOnBye)
	{
//		PostMessage(hConsole, WM_DESTROY, 0, 0);
		DestroyWindow(hConsole);
	}
}

void MoveWindows()
{
	RECT rcClient;

	GetClientRect(hConsole, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	MoveWindow(hwndOutput,2, 2, ClientWidth-4, ClientHeight-InputBoxHeight-4, TRUE);
	MoveWindow(hwndInput,2, ClientHeight-InputBoxHeight-2, ClientWidth-4, InputBoxHeight, TRUE);

	GetClientRect(hwndOutput, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;
	
	WarnLen = ClientWidth/8 - 1;
	WrapLen = WarnLen;
	maxlinelen = WarnLen;

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

			ToggleParam(hMenu, hWnd, &Bells, BPQBELLS);
			break;

		case BPQStripLF:

			ToggleParam(hMenu, hWnd, &StripLF, BPQStripLF);
			break;

		case IDM_WARNINPUT:

			ToggleParam(hMenu, hWnd, &WarnWrap, IDM_WARNINPUT);
			break;


		case IDM_WRAPTEXT:

			ToggleParam(hMenu, hWnd, &WrapInput, IDM_WRAPTEXT);
			break;

		case IDM_Flash:

			ToggleParam(hMenu, hWnd, &FlashOnConnect, IDM_WRAPTEXT);
			break;

		case IDM_CLOSEWINDOW:

			ToggleParam(hMenu, hWnd, &CloseWindowOnBye, IDM_CLOSEWINDOW);
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

			GetWindowRect(hWnd,	&ConsoleRect);	// For save soutine

            SetWindowLong(hwndInput, GWL_WNDPROC, 
                (LONG) wpOrigInputProc); 
         

			if (cfgMinToTray) 
				DeleteTrayMenuItem(hWnd);


			if (Console && Console->Active)
			{
				ClearQueue(Console);
		
				Console->Active = FALSE;
				RefreshMainWindow();

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
	int TextLen;

	if (uMsg == WM_CHAR) 
	{
		if(WarnWrap || WrapInput)
		{
			TextLen = SendMessage(hwndInput,WM_GETTEXTLENGTH, 0, 0);

			if (WarnWrap)
				if (TextLen == WarnLen) Beep(220, 150);
			
			if (WrapInput)
				if ((wParam == 0x20) && (TextLen > WrapLen))
					wParam = 13;		// Replace space with Enter

		}

		if (wParam == 13)
		{
			kbptr=SendMessage(hwndInput,WM_GETTEXT,159,(LPARAM) (LPCSTR) kbbuf);

			kbbuf[kbptr]=13;

			// Echo

			WritetoConsoleWindow(kbbuf, kbptr+1);

			ProcessLine(Console, user, &kbbuf[0], kbptr+1);

			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
		if (wParam == 0x1a)  // Ctrl/Z
		{
	
			kbbuf[0]=0x1a;
			kbbuf[1]=13;

			ProcessLine(Console, user, &kbbuf[0], 2);


			SendMessage(hwndInput,WM_SETTEXT,0,(LPARAM)(LPCSTR) "");

			return 0; 
		}
 
	}

    return CallWindowProc(wpOrigInputProc, hwnd, uMsg, wParam, lParam); 
} 

LRESULT APIENTRY OutputProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Trap mouse messages, so we can't select stuff in output and mon windows,
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

	if (PartLinePtr > 100000)
		PartLinePtr = 0;

	if (len > 0)
	{
		//	copy text to control a line at a time	
					
		ptr2=memchr(ptr1,13,len);

		if (ptr2 == 0)
		{
			// no newline. Move data to start of buffer and Save pointer

			PartLineIndex=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );
			SendMessage(hwndOutput,LB_SETCARETINDEX,(WPARAM) PartLineIndex, MAKELPARAM(FALSE, 0));

			PartLinePtr=len;
			memmove(readbuff,ptr1,len);

			return (0);

		}

		*(ptr2++)=0;

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
			index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );					
			ptr1[foldlen] = save;
			linelen -= foldlen;
			ptr1 = ptr3;

			if (linelen > maxlinelen)
				goto foldloop;
						
			if (linelen > 0)
				index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );

			ptr1 = saveptr1;
		}
		else
		{
			index=SendMessage(hwndOutput,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) ptr1 );					
		}

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


	return (0);
}

int ToggleParam(HMENU hMenu, HWND hWnd, BOOL * Param, int Item)
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
/*
#define XBITMAP 80 
#define YBITMAP 20 
 
#define BUFFER MAX_PATH 
 
HBITMAP hbmpPencil, hbmpCrayon, hbmpMarker, hbmpPen, hbmpFork; 
HBITMAP hbmpPicture, hbmpOld; 
 
void AddItem(HWND hwnd, LPSTR lpstr, HBITMAP hbmp) 
{ 
    int nItem; 
 
    nItem = SendMessage(hwnd, LB_ADDSTRING, 0, (LPARAM)lpstr); 
    SendMessage(hwnd, LB_SETITEMDATA, (WPARAM)nItem, (LPARAM)hbmp); 
} 
 
DWORD APIENTRY DlgDrawProc( 
        HWND hDlg,            // window handle to dialog box 
        UINT message,         // type of message 
        UINT wParam,          // message-specific information 
        LONG lParam) 
{ 
    int nItem; 
    TCHAR tchBuffer[BUFFER]; 
    HBITMAP hbmp; 
    HWND hListBox; 
    TEXTMETRIC tm; 
    int y; 
    HDC hdcMem; 
    LPMEASUREITEMSTRUCT lpmis; 
    LPDRAWITEMSTRUCT lpdis; 
    RECT rcBitmap;
	HRESULT hr; 
	size_t * pcch;
 
    switch (message) 
    { 
 
        case WM_INITDIALOG: 
 
            // Load bitmaps. 
 
            hbmpPencil = LoadBitmap(hinst, MAKEINTRESOURCE(700)); 
            hbmpCrayon = LoadBitmap(hinst, MAKEINTRESOURCE(701)); 
            hbmpMarker = LoadBitmap(hinst, MAKEINTRESOURCE(702)); 
            hbmpPen = LoadBitmap(hinst, MAKEINTRESOURCE(703)); 
            hbmpFork = LoadBitmap(hinst, MAKEINTRESOURCE(704)); 
 
            // Retrieve list box handle. 
 
            hListBox = GetDlgItem(hDlg, IDL_STUFF); 
 
            // Initialize the list box text and associate a bitmap 
            // with each list box item. 
 
            AddItem(hListBox, "pencil", hbmpPencil); 
            AddItem(hListBox, "crayon", hbmpCrayon); 
            AddItem(hListBox, "marker", hbmpMarker); 
            AddItem(hListBox, "pen",    hbmpPen); 
            AddItem(hListBox, "fork",   hbmpFork); 
 
            SetFocus(hListBox); 
            SendMessage(hListBox, LB_SETCURSEL, 0, 0); 
            return TRUE; 
 
        case WM_MEASUREITEM: 
 
            lpmis = (LPMEASUREITEMSTRUCT) lParam; 
 
            // Set the height of the list box items. 
 
            lpmis->itemHeight = 20; 
            return TRUE; 
 
        case WM_DRAWITEM: 
 
            lpdis = (LPDRAWITEMSTRUCT) lParam; 
 
            // If there are no list box items, skip this message. 
 
            if (lpdis->itemID == -1) 
            { 
                break; 
            } 
 
            // Draw the bitmap and text for the list box item. Draw a 
            // rectangle around the bitmap if it is selected. 
 
            switch (lpdis->itemAction) 
            { 
                case ODA_SELECT: 
                case ODA_DRAWENTIRE: 
 
                    // Display the bitmap associated with the item. 
 
                    hbmpPicture =(HBITMAP)SendMessage(lpdis->hwndItem, 
                        LB_GETITEMDATA, lpdis->itemID, (LPARAM) 0); 
 
                    hdcMem = CreateCompatibleDC(lpdis->hDC); 
                    hbmpOld = SelectObject(hdcMem, hbmpPicture); 
 
                    BitBlt(lpdis->hDC, 
                        lpdis->rcItem.left, lpdis->rcItem.top, 
                        lpdis->rcItem.right - lpdis->rcItem.left, 
                        lpdis->rcItem.bottom - lpdis->rcItem.top, 
                        hdcMem, 0, 0, SRCCOPY); 
 
                    // Display the text associated with the item. 
 
                    SendMessage(lpdis->hwndItem, LB_GETTEXT, 
                        lpdis->itemID, (LPARAM) tchBuffer); 
 
                    GetTextMetrics(lpdis->hDC, &tm); 
 
                    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - 
                        tm.tmHeight) / 2;
						
                    hr = StringCchLength(tchBuffer, BUFFER, pcch);
                    if (FAILED(hr))
                    {
                        // TODO: Handle error.
                    }
 
                    TextOut(lpdis->hDC, 
                        XBITMAP + 6, 
                        y, 
                        tchBuffer, 
                        pcch); 						
 
                    SelectObject(hdcMem, hbmpOld); 
                    DeleteDC(hdcMem); 
 
                    // Is the item selected? 
 
                    if (lpdis->itemState & ODS_SELECTED) 
                    { 
                        // Set RECT coordinates to surround only the 
                        // bitmap. 
 
                        rcBitmap.left = lpdis->rcItem.left; 
                        rcBitmap.top = lpdis->rcItem.top; 
                        rcBitmap.right = lpdis->rcItem.left + XBITMAP; 
                        rcBitmap.bottom = lpdis->rcItem.top + YBITMAP; 
 
                        // Draw a rectangle around bitmap to indicate 
                        // the selection. 
 
                        DrawFocusRect(lpdis->hDC, &rcBitmap); 
                    } 
                    break; 
 
                case ODA_FOCUS: 
 
                    // Do not process focus changes. The focus caret 
                    // (outline rectangle) indicates the selection. 
                    // The IDOK button indicates the final 
                    // selection. 
 
                    break; 
            } 
            return TRUE; 
 
        case WM_COMMAND: 
 
            switch (LOWORD(wParam)) 
            { 
                case IDOK: 
                    // Get the selected item's text. 
 
                    nItem = SendMessage(GetDlgItem(hDlg, IDL_STUFF), 
                       LB_GETCURSEL, 0, (LPARAM) 0); 
                       hbmp = SendMessage(GetDlgItem(hDlg, IDL_STUFF), 
                            LB_GETITEMDATA, nItem, 0); 
 
                    // If the item is not the correct answer, tell the 
                    // user to try again. 
                    //
                    // If the item is the correct answer, congratulate 
                    // the user and destroy the dialog box. 
 
                    if (hbmp != hbmpFork) 
                    { 
                        MessageBox(hDlg, "Try again!", "Oops", MB_OK); 
                        return FALSE; 
                    } 
                    else 
                    { 
                        MessageBox(hDlg, "You're right!", 
                            "Congratulations.", MB_OK); 
 
                      // Fall through. 
                    } 
 
                case IDCANCEL: 
 
                    // Destroy the dialog box. 
 
                    EndDialog(hDlg, TRUE); 
                    return TRUE; 
 
                default: 
 
                    return FALSE; 
            } 
 
        case WM_DESTROY: 
 
            // Free any resources used by the bitmaps. 
 
            DeleteObject(hbmpPencil); 
            DeleteObject(hbmpCrayon); 
            DeleteObject(hbmpMarker); 
            DeleteObject(hbmpPen); 
            DeleteObject(hbmpFork); 
 
            return TRUE; 
 
        default: 
            return FALSE; 
 
    } 
    return FALSE; 
} 
*/
