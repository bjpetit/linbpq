
// Version 1.0.5 January 2009

//	Add Start Minimized Option

// Version 1.0.6 September 2010

// Support IP Module as part of BPQ32.dll

// Version 1.0.7 October 2011

// Call CloseBPQ32 on exit

#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <malloc.h>	
#include <memory.h>

#define Dll	__declspec( dllimport )

#include "bpqsts.h"
#include "..\include\bpq32.h"			// BPQ32 API Defines
#include "..\include\asmstrucs.h"

#include "..\kernel\ipcode.h"

HINSTANCE hInst; 
char AppName[] = "BPQ Node Stream Status";
char Title[80];

ARPDATA * ARPRecord = NULL;				// ARP Table - malloc'ed as needed
IPSTATS * IPStats = NULL;

UCHAR * BPQDirectory;
HINSTANCE hIPModule=0;
BOOL (FAR WINAPI * GetIPInfo) ();

// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void LoadToolHelperRoutines();

int DoStatus();
int DoIPStatus();
CopyScreentoBuffer(char * buff);
VOID DoARPLine(int i);
BOOL LoadIPDriver();

char Screen[4000];
char NewScreen[4000];

int IPStatsState = -1;
int ARPCount = 1;

int ptr=0;

int Stream;
int len,count;

char callsign[10];
int state;
int change;
int applmask = 0;
int applflags=0;

BOOL StreamDisplay = TRUE;

CONNECTED=FALSE;
AUTOCONNECT=TRUE;

LOGFONT LFTTYFONT ;

HFONT hFont ;

HMENU hMenu,hPopMenu1,hPopMenu2,hPopMenu3;		// handle of menu 

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

	CloseBPQ32();				// Close Ext Drivers if last bpq32 process

	return (msg.wParam);

	lpCmdLine; // This will prevent 'unused formal parameter' warnings
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

	hInst = hInstance; // Store instance handle in our global variable

	MinimizetoTray=GetMinimizetoTrayFlag();


	hWnd = CreateWindow(AppName, Title, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 760, 33*16,
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
   lstrcpy( LFTTYFONT.lfFaceName, "Terminal" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;
	

	memset(Screen, ' ', 4000); 

	SetWindowText(hWnd,AppName);
	
	hMenu=CreateMenu();
	hPopMenu2=CreatePopupMenu();
	hPopMenu3=CreatePopupMenu();
	SetMenu(hWnd,hMenu);

	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu2,"Window");
	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu3,"Edit");

	AppendMenu(hPopMenu2,MF_STRING,BPQSTREAMS,"Streams");
	AppendMenu(hPopMenu2,MF_STRING,BPQIPSTATUS,"IP Status");
	AppendMenu(hPopMenu3,MF_STRING,BPQCOPY,"Copy");

	CheckMenuItem(hMenu,BPQSTREAMS,MF_CHECKED);

	DrawMenuBar(hWnd);	


	if (MinimizetoTray)
	{
		//	Set up Tray ICON

		AddTrayMenuItem(hWnd, "Stream Status");
	}


	SetTimer(hWnd,1,1000,NULL);
	
	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (MinimizetoTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);

	LoadToolHelperRoutines();

	return (TRUE);
}

#include <tlhelp32.h>

typedef  int (WINAPI FAR *FARPROCX)();

FARPROCX CreateToolHelp32SnapShotPtr;
FARPROCX Process32Firstptr;
FARPROCX Process32Nextptr;

void LoadToolHelperRoutines()
{
	HINSTANCE ExtDriver=0;
	int err;
	char msg[100];

	ExtDriver=LoadLibrary("kernel32.dll");

	if (ExtDriver == NULL)
	{
		err=GetLastError();

		wsprintf(msg,"BPQ32 Error loading kernel32.dll - Error code %d", err);
		
		OutputDebugString(msg);

		return;
	}

	CreateToolHelp32SnapShotPtr = (FARPROCX)GetProcAddress(ExtDriver,"CreateToolhelp32Snapshot");
	Process32Firstptr = (FARPROCX)GetProcAddress(ExtDriver,"Process32First");
	Process32Nextptr = (FARPROCX)GetProcAddress(ExtDriver,"Process32Next");
	
	if (CreateToolHelp32SnapShotPtr == 0)
	{
		err=GetLastError();

		wsprintf(msg,"BPQ32 Error getting CreateToolhelp32Snapshot entry point - Error code %d", err);
		
		OutputDebugString(msg);

		return;
	}

}
BOOL GetProcess(int ProcessID, char * Program)
{
  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;

   if (CreateToolHelp32SnapShotPtr==0)
   {
	   return (TRUE);	// Routine not available
   }
  // Take a snapshot of all processes in the system.
  hProcessSnap = (HANDLE)CreateToolHelp32SnapShotPtr(TH32CS_SNAPPROCESS, 0);
  if( hProcessSnap == INVALID_HANDLE_VALUE )
  {
    OutputDebugString( "CreateToolhelp32Snapshot (of processes) Failed" );
    return( FALSE );
  }

  // Set the size of the structure before using it.
  pe32.dwSize = sizeof( PROCESSENTRY32 );

  // Retrieve information about the first process,
  // and exit if unsuccessful
  if( !Process32Firstptr( hProcessSnap, &pe32 ) )
  {
    OutputDebugString( "Process32First Failed" );  // Show cause of failure
    CloseHandle( hProcessSnap );     // Must clean up the snapshot object!
    return( FALSE );
  }

  // Now walk the snapshot of processes, and
  // get information about this process 
  do
  {
	  if (ProcessID==pe32.th32ProcessID)
	  {
		  wsprintf(Program,"%s", pe32.szExeFile);
		  CloseHandle( hProcessSnap );
		  return( TRUE );
	  }

  } while( Process32Nextptr( hProcessSnap, &pe32 ) );


  wsprintf(Program,"PID %d Not Found", ProcessID);
  CloseHandle( hProcessSnap );
  return(FALSE);
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

	
	switch (message) { 


	case WM_TIMER:

		if (StreamDisplay) DoStatus(); else DoIPStatus();

		InvalidateRect(hWnd,NULL,FALSE);
		return (0);


	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		//Parse the menu selections:
		
		switch (wmId) {

			case BPQSTREAMS:
	
				CheckMenuItem(hMenu,BPQSTREAMS,MF_CHECKED);
				CheckMenuItem(hMenu,BPQIPSTATUS,MF_UNCHECKED);

				StreamDisplay = TRUE;

				break;

			case BPQIPSTATUS:
	
				CheckMenuItem(hMenu,BPQSTREAMS,MF_UNCHECKED);
				CheckMenuItem(hMenu,BPQIPSTATUS,MF_CHECKED);

				StreamDisplay = FALSE;
				memset(Screen, ' ', 4000); 


				break;

        
			case BPQCOPY:
		
			//
			//	Copy buffer to clipboard
			//
			hMem=GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, 33*110);
		
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
			
			for (i=0; i<33; i++)
			{
				TextOut(hdc,0,i*14,&Screen[i*108],108);
			}
			
			SelectObject( hdc, hOldFont ) ;
			EndPaint (hWnd, &ps);
	
			break;        

		case WM_DESTROY:
		
			DestroyMenu(hMenu);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			PostQuitMessage(0);
			
			break;


		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}




CopyScreentoBuffer(char * buff)
{
	int i,j;
	char line [120];
	
	for (i=0; i!=33; i++)
	{
		memcpy(line,&Screen[i*108],108);
		
		for (j=0;j != 108; j++)
		{
			if (line[j] == '\0') line[j]=' ';
		}
		//
		//	scan line backwards, and replace last space with crlf
		//

		for (j=107;j != -1; j--)
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

	return (0);
}
	
int Mask;
int Flags;

	char flag[2];

int DoStatus()
{
	int i, pid, p;
	char Program[256];

	memset(Screen, ' ', 3000); 

	strcpy(Screen,"    RX  TX MON App Flg Callsign  Program                  RX  TX MON App Flg Callsign Program");

	for (i=1;i<65; i++)
	{
			
		callsign[0]=0;
		
		if (GetAllocationState(i))
			
			strcpy(flag,"*");
		else
			strcpy(flag," ");

		GetCallsign(i,callsign);

		Mask=GetApplMask(i);

		Flags=GetApplFlags(i);

		pid=GetStreamPID(i);

		strcpy(Program, "Not Found");

		if (pid)
			GetProcess(pid,&Program[0]);
		else
			Program[0]=0;

// if running on 98, program contains the full path - remove it

		for (p=strlen(Program); p>=0; p--)
		{
			if (Program[p]=='\\') 
			{
				break;
			}
		}
		p++;	
		wsprintf(&Screen[(i+1)*54],"%2d%s%3d %3d %3d %3x %3x %10s%-20s",
			i,flag,RXCount(i),TXCount(i),MONCount(i),Mask,Flags,callsign,&Program[p]);
	}

	return(0);
}

int DoIPStatus()
{
	CheckTimer();

	if (GetIPInfo == NULL) LoadIPDriver();

	if (GetIPInfo == NULL)
	{
		// Falied to Load

		CheckMenuItem(hMenu,BPQSTREAMS,MF_CHECKED);
		CheckMenuItem(hMenu,BPQIPSTATUS,MF_UNCHECKED);
		StreamDisplay = TRUE;

		return 0;
	}

	if (IPStatsState == -1)
	{
		//	Start a new cycle.

		//	As the arp table is in shared memory, we have to ask bpq32.dll to copy each entry to our
		//	memory. Do one entry on each call of the timer.

		strcpy(Screen,"     IP Address     MAC Address        Port Type Valid Timer");

		GetIPInfo(&ARPRecord, &IPStats, 0);

		IPStatsState = 0;

		return 0;
	}

	if (IPStatsState == 0)
	{
		//should have first entry, and number of entries. Should always be at least 1.

		ARPCount = IPStats->ARPEntries;
		if (ARPCount == 0)
			ARPCount = 1;
	}

	DoARPLine(IPStatsState++);

	if (IPStatsState == ARPCount)
	{
		memset(&Screen[(IPStatsState+1)*108], ' ', 4000-((IPStatsState+1)*108)); 
		IPStatsState = -1;
	}
	else
		GetIPInfo(&ARPRecord, &IPStats, IPStatsState);
	
	return 0;
}

VOID DoARPLine(int i)
{
	int SSID, j;
	char Mac[20];
	char Call[7];
	char IP[20];

	struct in_addr Addr;
	Addr.s_addr = ARPRecord->IPADDR;

	wsprintf(IP, "%d.%d.%d.%d", 
		Addr.S_un.S_un_b.s_b1, Addr.S_un.S_un_b.s_b2, Addr.S_un.S_un_b.s_b3, Addr.S_un.S_un_b.s_b4); 

	if(ARPRecord->ARPINTERFACE == 255)		// Ethernet
	{
			wsprintf(Mac," %02x:%02x:%02x:%02x:%02x:%02x", 
				ARPRecord->HWADDR[0],
				ARPRecord->HWADDR[1],
				ARPRecord->HWADDR[2],
				ARPRecord->HWADDR[3],
				ARPRecord->HWADDR[4],
				ARPRecord->HWADDR[5]);
	}
	else
	{
			for (j=0; j< 6; j++)
			{
				Call[j] = ARPRecord->HWADDR[j]/2;
				if (Call[j] == 32) Call[j] = 0;
			}
			Call[6] = 0;
			SSID = (ARPRecord->HWADDR[6] & 31)/2;
			
			wsprintf(Mac," %s-%d", Call, SSID);
	}

	wsprintf(&Screen[(i+1)*108],"%18s %-19s %4d   %c  %3d   %d",
			IP, Mac, ARPRecord->ARPINTERFACE,
			ARPRecord->ARPTYPE, ARPRecord->ARPVALID, ARPRecord->ARPTIMER);

	

	return;
}

BOOL LoadIPDriver()
{
	char msg[128];
	int err=0;
	UCHAR Value[MAX_PATH];
	char DLL[]="BPQ32.dll";
	
	// If no directory, use current

	BPQDirectory=GetBPQDirectory();

//	if (BPQDirectory[0] == 0)
	{
		strcpy(Value, DLL);
	}
/*		else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value, DLL);
	}
*/		
	hIPModule=LoadLibrary(Value);

	if (hIPModule == NULL)
	{
		err=GetLastError();

		wsprintf(msg,"Error loading Driver %s - Error code %d",	DLL,err);
		CheckMenuItem(hMenu,BPQSTREAMS,MF_CHECKED);
		CheckMenuItem(hMenu,BPQIPSTATUS,MF_UNCHECKED);
		StreamDisplay = TRUE;
		
		MessageBox(NULL,msg,"BPQ32",MB_ICONSTOP);
		return FALSE;

	}
	else
	{
		GetIPInfo = (int (__stdcall *)())GetProcAddress(hIPModule,"_GetIPInfo@12");
	}
	return TRUE;
}