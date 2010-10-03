
#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#include "windows.h"
#include <stdio.h>
#include <time.h>
#include <Psapi.h>
#include <commctrl.h>

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#include "bpq32.h"
#include "kernelresource.h"
#include "TNCINFO.h"
#include "RigControl.h"
#include "AsmStrucs.h"

extern char * PortConfig[33];
extern BOOL MinimizetoTray;
HANDLE hInstance;
HBRUSH bgBrush;
#define BGCOLOUR RGB(236,233,216)

KillTNC(struct TNCINFO * TNC);
RestartTNC(struct TNCINFO * TNC);

VOID __cdecl Debugprintf(const char * format, ...);

extern UCHAR BPQDirectory[];

BOOL Minimized;				// Start Minimized Flag

RECT Rect;
BOOL RestartAfterFailure = FALSE;

struct TNCINFO * TNCInfo[34] = {NULL};		// Records are Malloc'd

#define WSA_ACCEPT WM_USER + 1
#define WSA_DATA WM_USER + 2
#define WSA_CONNECT WM_USER + 3

int Winmor_Socket_Data(int sock, int error, int eventcode);

VOID MoveWindows(struct TNCINFO * TNC)
{
	RECT rcClient;
	int ClientHeight, ClientWidth;

	GetClientRect(TNC->hDlg, &rcClient); 

	ClientHeight = rcClient.bottom;
	ClientWidth = rcClient.right;

	MoveWindow(TNC->hMonitor,4 , 200, ClientWidth-8, ClientHeight-205, TRUE);
}

FILE *file;
char * Config;
char * ptr1, * ptr2;

BOOL ReadConfigFile(char * fn, int Port, int ProcLine())
{
	char buf[256],errbuf[256];
	
	Config = PortConfig[Port];

	if (Config)
	{
		// Using config from bpq32.cfg

		ptr1 = Config;

		ptr2 = strchr(ptr1, 13);
		while(ptr2)
		{
			memcpy(buf, ptr1, ptr2 - ptr1);
			buf[ptr2 - ptr1] = 0;
			ptr1 = ptr2 + 2;
			ptr2 = strchr(ptr1, 13);

			strcpy(errbuf,buf);			// save in case of error
	
			if (!ProcLine(buf, Port))
			{
				WritetoConsole("Bad config record ");
				WritetoConsole(errbuf);
			}
		}
	}
	else
	{

	UCHAR Value[100];

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value,fn);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value,fn);
	}
		
	if ((file = fopen(Value,"r")) == NULL)
	{
		wsprintf(buf," Config file %s not found ",Value);
		WritetoConsole(buf);
		return (TRUE);			// Will give Port Not Defined error later
	}

	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcLine(buf, Port))
		{
			WritetoConsole(" Bad config record ");
			WritetoConsole(errbuf);
		}			
	}

	fclose(file);
	file = NULL;
	}
	return (TRUE);
}
GetLine(char * buf)
{
loop:

	if (file)
	{
		if (fgets(buf, 255, file) == NULL)
			return 0;
	}
	else
	{
		if (ptr2 == NULL) 
			return 0;

		memcpy(buf, ptr1, ptr2 - ptr1 + 2);
		buf[ptr2 - ptr1 + 2] = 0;
		ptr1 = ptr2 + 2;
		ptr2 = strchr(ptr1, 13);
	}

	if (buf[0] < 0x20) goto loop;
	if (buf[0] == '#') goto loop;
	if (buf[0] == ';') goto loop;

	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	if (buf[strlen(buf)-1] < 0x20) buf[strlen(buf)-1] = 0;
	buf[strlen(buf)] = 13;

	return 1;
}

LRESULT CALLBACK PacWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	int i;
	struct TNCINFO * TNC;

	switch (message) { 

//		case WM_ACTIVATE:

//			SetFocus(hwndInput);
//			break;


	case WSA_DATA: // Notification on data socket

		Winmor_Socket_Data(wParam, WSAGETSELECTERROR(lParam), WSAGETSELECTEVENT(lParam));
		return 0;

	case WM_INITMENUPOPUP:

		for (i=1; i<33; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
			
			if (TNC->hDlg == hWnd)
				break;
		}


		if (wParam == (WPARAM)TNC->hPopMenu)
		{
			if (TNC->ProgramPath)
			{
				if (strstr(TNC->ProgramPath, "WINMOR TNC"))
				{
					EnableMenuItem(TNC->hPopMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(TNC->hPopMenu, WINMOR_KILL, MF_BYCOMMAND | MF_ENABLED);
		
					return TRUE;
				}
			}

			EnableMenuItem(TNC->hPopMenu, WINMOR_RESTART, MF_BYCOMMAND | MF_GRAYED);
			EnableMenuItem(TNC->hPopMenu, WINMOR_KILL, MF_BYCOMMAND | MF_GRAYED);
		}
			
		break;

	case WM_COMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		for (i=1; i<33; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
		
			if (TNC->hDlg == hWnd)
				break;
		}


		switch (wmId) {

//		case WINMOR_CONFIG:
//
//			DialogBoxParam(hInstance, MAKEINTRESOURCE(WINMORCONFIG), hWnd, ConfigDialogProc, (LPARAM)TNC);
//			break;

		case WINMOR_KILL:

			KillTNC(TNC);
			break;

		case WINMOR_RESTART:

			KillTNC(TNC);
			RestartTNC(TNC);
 
			break;

		case WINMOR_RESTARTAFTERFAILURE:

			RestartAfterFailure = !RestartAfterFailure;
			CheckMenuItem(TNC->hPopMenu, WINMOR_RESTARTAFTERFAILURE, (RestartAfterFailure) ? MF_CHECKED : MF_UNCHECKED);

			break;

//		case IDC_TEST:

//			SendExitEnter(TNC);

//			break;

		default:

			return 0;

		}

	case WM_SIZING:

		for (i=1; i<33; i++)
		{
			TNC = TNCInfo[i];
			if (TNC == NULL)
				continue;
		
			if (TNC->hDlg == hWnd)
				break;
		}

		MoveWindows(TNC);
			
		return TRUE;

	case WM_SYSCOMMAND:

		wmId    = LOWORD(wParam); // Remember, these are...
		wmEvent = HIWORD(wParam); // ...different for Win32!

		switch (wmId) { 

		case SC_RESTORE:

			Minimized = FALSE;
			return (DefWindowProc(hWnd, message, wParam, lParam));

		case  SC_MINIMIZE: 

			Minimized = TRUE;
			if (MinimizetoTray)
				return ShowWindow(hWnd, SW_HIDE);		
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
		}

		case WM_CTLCOLORDLG:
			return (LONG)bgBrush;

		 case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(0, 0, 0));
			SetBkMode(hdcStatic, TRANSPARENT);
			return (LONG)bgBrush;
		}

		case WM_DESTROY:
		
			// Remove the subclass from the edit control. 


			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);

			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));

	}
	return (0);
}



//HMENU hPopMenu;



BOOL CreatePactorWindow(struct TNCINFO * TNC, char * ClassName, char * WindowTitle, int RigControlRow)
{
    WNDCLASS  wc;
	char Title[80];
	int retCode, Type, Vallen;
	HKEY hKey=0;
	char Key[80];
	char Size[80];
	int Top, Left;
	HANDLE hDlg;

	if (TNC->hDlg)
	{
		ShowWindow(TNC->hDlg, SW_SHOWNORMAL);
		SetForegroundWindow(TNC->hDlg);
		return FALSE;							// Already open
	}

	bgBrush = CreateSolidBrush(BGCOLOUR);

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
    wc.lpfnWndProc = PacWndProc;       
                                        
    wc.cbClsExtra = 0;                
    wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
    wc.hIcon = LoadIcon( hInstance, MAKEINTRESOURCE(IDI_ICON2) );
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = bgBrush; 

	wc.lpszMenuName = NULL;	
	wc.lpszClassName = ClassName; 

	RegisterClass(&wc);

	TNC->hDlg = hDlg = CreateDialog(hInstance,ClassName,0,NULL);
	
	if (TNC->Hardware == H_WINMOR)
		wsprintf(Title, "%s Status - Port %d", WindowTitle, TNC->PortRecord->PORTCONTROL.PORTNUMBER);
	else
		wsprintf(Title,"%s Status - COM%d", WindowTitle, TNC->PortRecord->PORTCONTROL.IOBASE);

	SetWindowText(hDlg, Title);

	if (MinimizetoTray)
		AddTrayMenuItem(hDlg, Title);


	wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\PACTOR\\PORT%d", TNC->PortRecord->PORTCONTROL.PORTNUMBER);
	
	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=80;

		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		if (retCode == ERROR_SUCCESS)
			sscanf(Size,"%d,%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom, &Minimized);

		if (TNC->Hardware == H_WINMOR)	
			retCode = RegQueryValueEx(hKey,"RestartAfterFailure",0,			
				(ULONG *)&Type,(UCHAR *)&RestartAfterFailure,(ULONG *)&Vallen);
	}
	Top = Rect.top;
	Left = Rect.left;

	GetWindowRect(hDlg, &Rect);	// Get the real size

	MoveWindow(hDlg, Left, Top, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);

	if (Minimized)
		ShowWindow(hDlg, SW_HIDE);
	else
		ShowWindow(hDlg, SW_SHOWNORMAL);

	if (TNC->RIG)
	{
		struct RIGINFO * RIG = TNC->RIG;
		int RigRow = RigControlRow;

		RIG->hLabel = CreateWindow(WC_STATIC , "", WS_CHILD | WS_VISIBLE,
                 10, RigRow, 80,20, hDlg, NULL, hInstance, NULL);
	
		RIG->hCAT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 90, RigRow, 40,20, hDlg, NULL, hInstance, NULL);
	
		RIG->hFREQ = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 135, RigRow, 100,20, hDlg, NULL, hInstance, NULL);
	
		RIG->hMODE = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 240, RigRow, 60,20, hDlg, NULL, hInstance, NULL);
	
		RIG->hSCAN = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 300, RigRow, 20,20, hDlg, NULL, hInstance, NULL);

		RIG->hPTT = CreateWindow(WC_STATIC , "",  WS_CHILD | WS_VISIBLE,
                 320, RigRow, 20,20, hDlg, NULL, hInstance, NULL);

		//if (PORT->PortType == ICOM)
		//{
		//	wsprintf(msg,"%02X", PORT->Rigs[i].RigAddr);
		//	SetWindowText(RIG->hCAT, msg);
		//}
		SetWindowText(RIG->hLabel, RIG->RigName);

	}

	return TRUE;
}
