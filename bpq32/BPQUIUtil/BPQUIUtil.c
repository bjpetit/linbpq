
// Version 1. 0. 2. 1 October 2010

//	Add Delay on start option, and dynamically load bpq32

#define _CRT_SECURE_NO_DEPRECATE 
#define WIN32_LEAN_AND_MEAN
#define _USE_32BIT_TIME_T

#include <windows.h>
#include "time.h"

#include <stdlib.h>
#include <stdio.h>
//#include <malloc.h>	
//#include <memory.h>

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

//#define DYNLOADBPQ		// Dynamically Load BPQ32.dll
#include "bpq32.h"
#include "ASMStrucs.h"
#include "BPQUIUtil.h"

HINSTANCE hInst; 

UINT UIMask = 0;

#define MAXFLEN 400     /* Maximale Laenge eines Frames */
                        /* maximum length of a frame */
typedef signed short    i16;
typedef unsigned char   byte;
typedef unsigned long   u32;

// soundmodem.dll Command Codes

enum drvc
        {
        drv_interfaceversion=1, drv_ident, drv_version,
        drv_config, drv_confinfo, drv_init_device, drv_get_ch_cnt,
        drv_exit, drv_ch_active, drv_init_kanal,
        drv_stat, drv_ch_state, drv_scale,
        drv_tx_calib, drv_set_led, drv_rx_frame,
        drv_get_framebuf, drv_tx_frame,
        drv_get_txdelay, drv_get_mode, drv_get_baud,
        drv_set_txdelay
        };

struct modemchannel {
	char devname[64];
	unsigned int regchnum;

	unsigned int rxrunning;
	UINT rxthread;
	struct modulator *modch;
	struct demodulator *demodch;
	void *modstate;
	void *demodstate;

	byte dcd;
	byte mode;
	unsigned int bitrate;
	unsigned int scale;
	unsigned int calib;

};

#pragma pack(1)

typedef struct
{
    i16  len;               /* Laenge des Frames - length of the frame */
    byte kanal;             /* Kanalnummer - channel number */
    byte txdelay;           /* RX: Gemessenes TxDelay [*10ms],
                                   0 wenn nicht unterstuetzt
                               TX: Zu sendendes TxDelay */
                            /* RX: measured transmitter keyup delay (TxDelay) in 10ms units,
                                   0 if not supported
                               TX: transmitter keyup delay (TxDelay) that should be sent */
    byte frame[MAXFLEN];    /* L1-Frame (ohne CRC) - L1 frame without CRC */
} L1FRAME;



#pragma pack()

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

DllImport UINT APIENTRY SoundCardInterface(int Port, int Function, UINT Param1, UINT Param2);

enum SMCmds
{
	INIT,
	CHECKCHAN,		// See if channel is configured
	GETBUFFER,
	POLL,
	RXPACKET,
	CLOSING
};

int TimerHandle = 0;

char ClassName[]="SOUNDMAINWINDOW";					// the main window class name

HWND hWnd;

LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;
BOOL Minimized = FALSE;

HWND hPTT;
HWND hDCD;

int State;

int ChannelCount;		// Channels defined on this soundcard

typedef UINT (FAR *FARPROCX)();

UINT (FAR *SoundModem)() = NULL;

HINSTANCE ExtDriver=0;
HANDLE hRXThread;

L1FRAME * rxf;
UINT TXQ;

int ConfigNo;
char Config[20];

UINT PORTPERSISTANCE = 64;
char TXDELAY[10] = "500";

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


	// The Command Line is the registry key of the config - eg BPQ32_0 For fisrt soundcard

	if (sscanf(lpCmdLine, "%d %d %s", &ConfigNo, &PORTPERSISTANCE, &TXDELAY) != 3)
	{
//		MessageBox(NULL,"BPQSoundModem Should only be run by BPQ32","BPQ32",MB_ICONSTOP);
//		return(999);
	}

	SoundCardInterface(ConfigNo, INIT, GetCurrentProcessId(), 0);
 
	wsprintf(Config, "BPQ32_%d", ConfigNo);

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

VOID RXThread()
{
	while (TRUE)
	{
		rxf = (L1FRAME *)SoundModem(drv_rx_frame, 0, 0, 0);

		if (rxf)
			SoundCardInterface(ConfigNo, RXPACKET, (UINT)rxf, rxf->kanal);

		Sleep(50);
	}
}

HBRUSH bgBrush;

#define BGCOLOUR RGB(236,233,216)

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int retCode, Type, Vallen;
	char Key[80];
	int err, i=0, n, Top, Left;
	char Size[80];
	RECT Rect = {0};
	char Title[80];
	HKEY hKey = 0;
	HKEY hKey2 = 0;
	WNDCLASS  wc;
	struct _EXTPORTDATA * PORTVEC;

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

	wsprintf(Title, "Unproto Interface");

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


	}

	Top = Rect.top;
	Left = Rect.left;

	GetWindowRect(hWnd, &Rect);	// Get the real size

	MoveWindow(hWnd, Left, Top, Rect.right - Rect.left, Rect.bottom - Rect.top, TRUE);

	MinimizetoTray = GetMinimizetoTrayFlag();
		
	if (Minimized)
		if (MinimizetoTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, SW_SHOWMINIMIZED);
	else
		ShowWindow(hWnd, SW_RESTORE);

   	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	if (MinimizetoTray)
	{
		wsprintf(Title, "UIRoutine");
		AddTrayMenuItem(hWnd, Title);
	}

	for (n = 1; n <= 32; n++)
	{
		ShowWindow(GetDlgItem(hWnd, BASECHECK + n), SW_HIDE);
	}

	for (n = 1; n <= GetNumberofPorts(); n++)
	{
		i = GetPortNumber(n);
		ShowWindow(GetDlgItem(hWnd, BASECHECK + i), SW_SHOW);

		PORTVEC = (struct _EXTPORTDATA *)GetPortTableEntry(i);

		if (PORTVEC->PORTCONTROL.PORTTYPE != 16 || PORTVEC->PORTCONTROL.PROTOCOL != 10)	// Pactor/WINMOR
			EnableWindow(GetDlgItem(hWnd, BASECHECK + i), TRUE);

		if (UIMask & (1<<(i-1)))
			CheckDlgButton(hWnd, BASECHECK + 1, 1);
	}
	TimerHandle = SetTimer(hWnd,WM_TIMER, 100, NULL);

	return (TRUE);
}

VOID TimerProc()
{
	// Called every 100 ms (or so!)

	// Look for messages from BPQ32 

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	HKEY hKey=0;
	char Size[80];
	char Key[80];
	int retCode, disp;
	RECT Rect;

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

		if (wmId > BASECHECK && wmId < BASECHECK + 33)
		{
			int Mask = 0x1 << (wmId - (BASECHECK + 1));
			BOOL Active = IsDlgButtonChecked(hWnd, wmId);	
			
			if (Active)
				UIMask |= Mask;
			else
				UIMask &= ~Mask;

			return 0;
		}

		switch (wmId) {

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

			wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\SOUNDMODEM\\PORT%d", ConfigNo);
	
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
/*
TXFrame()
{
	int State = SoundModem(drv_ch_state, 0);
	L1FRAME * txf;

	0x40   RxB RxBuffer is not empty
    0x20   PTT Transmitter is keyed on
    0x10   DCD Receiver is active
    0x08   FDX Channel is full duplex, receiver can always receive
    0x04   TBY Transmitter not ready, for example because of ongoing
                    calibration


			State = SoundModem(drv_ch_state, 0);



	if (State & 0x20)		// Already sending so send this one
			goto Sendit;

		if (State & 0x10)	// If DCD active queue it.
			goto Queueit;

	Sendit:

		if (txf = (L1FRAME *)SoundModem(drv_get_framebuf, Channel, 0, 0))
		{
			txlen = (buff[6] << 8) + buff[5] - 7;
			txf->len = txlen;
			memcpy(txf->frame, &buff[7], txlen);
			txf->txdelay = 0;
			SoundModem(drv_tx_frame, 0, 0, 0);
			return 0;
		}
					
		return (0);

				if (State & 0x20)
			PORTVEC->SENDING++;

		if (State & 0x10)
		{
			PORTVEC->ACTIVE++;
		}
		else
		{
			// DCD False - if frames queued, try to send

			if (PORT->TXQ)
			{
				UCHAR rnd = rand() & 255;
				
				Debugprintf ("CSMA rnd = %d, PP = %d", rnd, PORTVEC->PORTPERSISTANCE);
				
				if (rnd > PORTVEC->PORTPERSISTANCE)
				{
					Debugprintf("Waiting SLOTTIME");
				}
				else
				{
					UINT * buffptr;

					while (PORT->TXQ)
					{
						buffptr = Q_REM(&PORT->TXQ);
						if (txf = (L1FRAME *)SoundModem(drv_get_framebuf, PORT->Channel, 0, 0))
						{
							txlen = buffptr[1];
							txf->len = txlen;
							memcpy(txf->frame, &buffptr[2], txlen);
							ReleaseBuffer(buffptr);

							SoundModem(drv_tx_frame, 0, 0, 0);
						}
					}
				}
			}
		}



*/

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


