
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>

#include <stdlib.h>
#include <malloc.h>	
#include <memory.h>

#define Dll	__declspec( dllimport )

#include "bpqanal.h"
#include "..\include\bpq32.h"			// BPQ32 API Defines
#include "..\include\asmstrucs.h"

#pragma pack(1)

struct DEST
{
	int DEST_CHAIN;			// SORTED LIST CHAIN

	char DEST_CALL[7];		// DESTINATION CALLSIGN (AX25 FORMAT)
	char DEST_ALIAS[6];
	char DEST_STATE;		// CONTROL BITS - SETTING UP, ACTIVE ETC	
	char DEST_ROUTE;		// CURRENTY ACTIVE DESTINATION

	UCHAR ROUT1_QUALITY;	// QUALITY
	UCHAR ROUT1_OBSCOUNT;	//
	struct ROUTE * ROUT1_NEIGHBOUR;	// POINTER TO NEXT NODE IN PATH

	UCHAR ROUT2_QUALITY;	// QUALITY
	UCHAR ROUT2_OBSCOUNT;	//
	struct ROUTE * ROUT2_NEIGHBOUR;	// POINTER TO NEXT NODE IN PATH

	UCHAR ROUT3_QUALITY;	// QUALITY
	UCHAR ROUT3_OBSCOUNT;	//
	struct ROUTE * ROUT3_NEIGHBOUR;	// POINTER TO NEXT NODE IN PATH


	int	DEST_Q;				// QUEUE OF FRAMES FOR THIS DESTINATION

	short DEST_RTT;			// SMOOTHED ROUND TRIP TIMER
	short DEST_COUNT;		// FRAMES SENT

};

struct ROUTE
{
	UCHAR NEIGHBOUR_CALL[7];		//	; AX25 CALLSIGN	
	UCHAR NEIGHBOUR_DIGI1[7];		//	; DIGIS EN ROUTE (MAX 2 - ?? REMOVE)
	UCHAR NEIGHBOUR_DIGI2[7];

	UCHAR NEIGHBOUR_PORT;			

	UCHAR NEIGHBOUR_QUAL;
	UCHAR NEIGHBOUR_FLAG;			//; SET IF 'LOCKED' ROUTE

	char * NEIGHBOUR_LINK;		// POINTER TO LINK FOR THIS NEIGHBOUR

	short NEIGHBOUR_TIME;		// TIME LAST HEARD (HH MM)

	int NBOUR_IFRAMES;			// FRAMES SENT/RECEIVED
	int NBOUR_RETRIES;			// RETRASMISSIONS

	UCHAR NBOUR_MAXFRAME;		// FOR OPTIMISATION CODE
	UCHAR NBOUR_FRACK;
	UCHAR NBOUR_PACLEN;
};

#pragma pack()

#include "..\BPQIPModule\ipcode.h"

#define BPQICON 400

HINSTANCE hInst; 
char AppName[] = "BPQ Dump Analyser";
char Title[80];

UCHAR Memory[300000];
int DumpLen;

#define MAXLINELEN 120
#define MAXSCREENLEN 50


// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
void LoadToolHelperRoutines();

int DoStatus();
int DoRoutes(UCHAR * Line);
int DoNodes(UCHAR * Line);
int DoStats(UCHAR * Line);
int  MyTextOut( HDC hdc, int x,  int y, LPCSTR lpString, int c);


CopyScreentoBuffer(char * buff);
VOID DoARPLine(int i);
BOOL LoadIPDriver();

#define SCREENLEN MAXLINELEN * MAXSCREENLEN

UCHAR * Screen;
UCHAR * Base;
UCHAR * DATABASE;
UCHAR * QCOUNT;
PUCHAR ROUTES;
UCHAR * STATS;
UCHAR * NODES;


struct DATABASE * DataBase;
struct DEST * Dests;
struct ROUTE * Routes;

int MaxRoutes;
int MaxNodes;

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
	HANDLE handle;
	TCHAR Buf[100000];
	int Ret;

	Ret=QueryDosDevice("COM6",Buf,100000);

 

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(AppName, Title, WS_OVERLAPPEDWINDOW | WS_VSCROLL ,
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
   lstrcpy( LFTTYFONT.lfFaceName, "Fixedsys" ) ;

	hFont = CreateFontIndirect(&LFTTYFONT) ;
	


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

//	SetTimer(hWnd,1,1000,NULL);
	

	LoadToolHelperRoutines();


	handle = CreateFile("c:\\BPQDUMP1.dat",
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);


				if (handle == INVALID_HANDLE_VALUE)
				{
					MessageBox(hWnd,"Can't open Dump File", "", 0);
				}
				else
				{
					ReadFile(handle,Memory,300000,&DumpLen,NULL); 
					CloseHandle(handle);
				}

	// 128 Stack, #define MAXLINELEN 120 * #define MAXSCREENLEN 50 Screen, Mem

	// DATABASE 00004ba4

	Base = &Memory[MAXLINELEN *  MAXSCREENLEN + 128];

	Screen = &Memory[128];

	DATABASE = Base + 0x04a04;
//	DATABASE = Base + 0x04a7c;

	QCOUNT = Base + 0x40f8 ; 
	STATS = Base + 0x3fe9;

	ROUTES = DATABASE + 0x20;

	_asm{

		mov esi,ROUTES
		mov	eax,ROUTES
		movzx ebx,6[ESI]
		mov MaxRoutes, ebx
		mov eax,[eax]
		sub	eax, 4201f02cH
		add	eax, Base
		mov	ROUTES, EAX

	}

	Routes = (struct ROUTE *)ROUTES;

	NODES = DATABASE + 0x28;

	_asm{

		mov esi,NODES
		mov	eax,NODES
		movzx ebx,6[ESI]
		mov MaxNodes, ebx
		mov eax,[eax]
		sub	eax, 4201f02cH
		add	eax, Base
		mov	NODES, EAX

	}

	Dests = (struct DEST *)NODES;


	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

return (TRUE);
}

UCHAR * Relocate(UCHAR * Addr)
{
	UCHAR * vret;
	
	_asm{

		mov	eax, Addr
		sub	eax, 4201f02cH
		add	eax, Base
		mov	vret, EAX

	}

	return vret;
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


int StartLine;
int LocaLine;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
    HFONT    hOldFont ;
	HGLOBAL	hMem;
	UCHAR Line[200];
	int i, LineNo, Len;


	switch (message) { 


	case WM_TIMER:

		InvalidateRect(hWnd,NULL,FALSE);
		return (0);


	case WM_VSCROLL:

		if (LOWORD(wParam) == SB_THUMBPOSITION)
		{
			StartLine = HIWORD(wParam);
			InvalidateRect(hWnd,NULL,FALSE);
		}

		return 0;


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

				return (DefWindowProc(hWnd, message, wParam, lParam));
				break;
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}


		case WM_PAINT:

			hdc = BeginPaint (hWnd, &ps);
			
			hOldFont = SelectObject( hdc, hFont) ;
			LineNo=0;

			for (i=0; i<MAXSCREENLEN; i++)
			{
				if (Screen[i*MAXLINELEN] != ' ')
					MyTextOut(hdc,0,LineNo++*14,&Screen[i*MAXLINELEN], MAXLINELEN);
			}
			
			Routes = (struct ROUTE *)ROUTES;
			Dests = (struct DEST *)NODES;
			STATS = Base + 0x3fe9;//+0x78;

			while (STATS[0])
			{
				DoStats(Line);
				Len = strlen(Line);
				if (Len > 0)
					MyTextOut(hdc,0,LineNo++*14,Line, Len);
			}

			for (i=0; i<MaxRoutes; i++)
			{
				DoRoutes(Line);
				Len = strlen(Line);
				if (Len > 0)
					MyTextOut(hdc,0,LineNo++*14,Line, Len);
			}

			for (i=0; i<MaxNodes; i++)
			{
				DoNodes(Line);
				Len = strlen(Line);
				if (Len > 0)
					MyTextOut(hdc,0,LineNo++*14,Line, Len);
			}

			SelectObject( hdc, hOldFont ) ;
			EndPaint (hWnd, &ps);

			break;        

		case WM_DESTROY:
		
			DestroyMenu(hMenu);
			PostQuitMessage(0);
			
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}

int  MyTextOut( HDC hdc, int x,  int y, LPCSTR lpString, int c)
{
	if (y < StartLine*14)
		return 0;
	
	TextOut(hdc, x, y-(StartLine*14), lpString, c);
	return 0;
}

int DoRoutes(UCHAR * Line)
{
	char digis[30]="";
	char locked[3];

	char Normcall[10];
	char Portcall[10];

	if (Routes->NEIGHBOUR_CALL[0] != 0)
	{
		len=ConvFromAX25(Routes->NEIGHBOUR_CALL,Normcall);
		Normcall[len]=0;

		if ((Routes->NEIGHBOUR_FLAG & 1) == 1)
	
			strcpy(locked," !");
		else
			strcpy(locked," ");


		if (Routes->NEIGHBOUR_DIGI1[0] != 0)
		{
			memcpy(digis," VIA ",5);

			len=ConvFromAX25(Routes->NEIGHBOUR_DIGI1,Portcall);
			Portcall[len]=0;
			strcpy(&digis[5],Portcall);

			if (Routes->NEIGHBOUR_DIGI2[0] != 0)
			{
				len=ConvFromAX25(Routes->NEIGHBOUR_DIGI2,Portcall);
				Portcall[len]=0;
				strcat(digis," ");
				strcat(digis,Portcall);
			}
		}

		wsprintf(Line,
				"%s %d %d%s%s %d %d %d %.2d:%.2d",
					Normcall,
					Routes->NEIGHBOUR_PORT,
					Routes->NEIGHBOUR_QUAL,
					locked,digis,
					Routes->NBOUR_MAXFRAME,
					Routes->NBOUR_FRACK,
					Routes->NBOUR_PACLEN,
					Routes->NEIGHBOUR_TIME/256,
					Routes->NEIGHBOUR_TIME & 255);
		}
		else
			Line[0]= 0;


	Routes+=1;
	
	return (0);
}

int DoNodes(UCHAR * Line)
{
	char digis[30]="";

	char Normcall[10];	
	char Portcall[10];
	char Alias[10];
	int cursor = 0;
	int i;

	Line[0] = 0;

	{
		
		if (Dests->DEST_CALL[0] == 0)
		{
			Dests+=1;
			return 0;
		}

		len=ConvFromAX25(Dests->DEST_CALL,Normcall);
		Normcall[len]=0;


		memcpy(Alias,Dests->DEST_ALIAS,6);
		
		Alias[6]=0;

		for (i=0;i<6;i++)
		{
			if (Alias[i] == ' ')
				Alias[i] = 0;
		}

		if (Dests->ROUT1_NEIGHBOUR == 0)
		{
			cursor=wsprintf(Line,"%s:%s",
			Alias,Normcall);
			Dests+=1;
			return 0;
		}

		len=ConvFromAX25(
			Relocate(Dests->ROUT1_NEIGHBOUR->NEIGHBOUR_CALL), Portcall);
		
		Portcall[len]=0;

		cursor=wsprintf(Line,"%s:%s %s %d %d ",
			Alias,Normcall,Portcall,
			Dests->ROUT1_NEIGHBOUR->NEIGHBOUR_PORT,
			Dests->ROUT1_QUALITY);

		if (Dests->ROUT1_OBSCOUNT > 127)
		{
			len=wsprintf(&Line[cursor],"! ");
			cursor+=len;
		}


		if (Dests->ROUT2_NEIGHBOUR != 0)
		{
			len=ConvFromAX25(
				Relocate(Dests->ROUT2_NEIGHBOUR->NEIGHBOUR_CALL), Portcall);
			Portcall[len]=0;

			len=wsprintf(&Line[cursor],"%s %d %d ",
				Portcall,
				Dests->ROUT2_NEIGHBOUR->NEIGHBOUR_PORT,
				Dests->ROUT2_QUALITY);

			cursor+=len;

			if (Dests->ROUT2_OBSCOUNT > 127)
			{
				len=wsprintf(&Line[cursor],"! ");
				cursor+=len;
			}


		}

		if (Dests->ROUT3_NEIGHBOUR != 0)
		{
			len=ConvFromAX25(
				Relocate(Dests->ROUT3_NEIGHBOUR->NEIGHBOUR_CALL), Portcall);
			Portcall[len]=0;

			len=wsprintf(&Line[cursor],"%s %d %d ",
				Portcall,
				Dests->ROUT3_NEIGHBOUR->NEIGHBOUR_PORT,
				Dests->ROUT3_QUALITY);
		
			cursor+=len;

			if (Dests->ROUT3_OBSCOUNT > 127)
			{
				len=wsprintf(&Line[cursor],"! ");
				cursor+=len;
			}

		}

			

	}
	Dests+=1;
	return (0);

}

#define STATSLEN 28

int DoStats(UCHAR * Line)
{
	UCHAR Num;
	ULONG Val;
	UCHAR DecVal[20];
	int i;

	memcpy(Line, STATS, STATSLEN);

	Line[STATSLEN]=0;
	STATS +=STATSLEN;
	Num = STATS++[0];

	for (i=1; i <= Num; i++)
	{
	
	_asm {

		mov	eax,STATS
		mov eax,[eax]
		mov Val,eax
	}

	wsprintf(DecVal,"%10d", Val);
	strcat ( Line, DecVal);


	STATS+=4;
	}


/*
	MOV	ESI,OFFSET32 STATS

CMDST00:
	CMP	BYTE PTR [ESI],0
	JE SHORT CMDST15			; DO PORT STATS

	CALL	CHECKBUFFER

	MOV	ECX,STATSLEN
	REP MOVSB

	LODSB				; COUNT OF NUMBERS TO PROCESS

	CBW
	MOV	CX,AX

CMDST05:

	LODSW
	MOV	DX,AX
	LODSW
	XCHG	DX,AX			; LOW ORDER TO AX, HI TO DX

	CALL	CONVTO10DIGITS		; DISPLAY COUNTERS

	LOOP	CMDST05

	MOV	AL,0DH
	STOSB

	JMP	CMDST00
;
;
*/
	return 0;
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

