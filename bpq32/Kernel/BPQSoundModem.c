
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
#include "BPQSoundModem.h"

HINSTANCE hInst; 

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

UINT * Q_REM(UINT *Q);
int Q_ADD(UINT *Q,UINT *BUFF);
UINT ReleaseBuffer(UINT *BUFF);

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

BOOL cfgMinToTray;

LOGFONT LFTTYFONT ;

HFONT hFont ;

BOOL MinimizetoTray=FALSE;

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

	// The Command Line is the registry key of the config - eg BPQ32_0 For fisrt soundcard

	sscanf(lpCmdLine, "%d %d", &ConfigNo, &PORTPERSISTANCE);

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

	SoundCardInterface(ConfigNo, CLOSING, 0, 0);

	if (cfgMinToTray)
		DeleteTrayMenuItem(hWnd);


	return (msg.wParam);
}

VOID RXThread()
{
	while (TRUE)
	{
		rxf = (L1FRAME *)SoundModem(drv_rx_frame, 0, 0, 0);

		if (rxf)
		{
			UINT * buffptr = (UINT *)SoundCardInterface(ConfigNo, GETBUFFER, 0, 0);

			if (buffptr)
			{
				buffptr[1] = rxf->len;
				memcpy(buffptr+2, rxf->frame, rxf->len);
				SoundCardInterface(ConfigNo, RXPACKET, (UINT)buffptr, rxf->kanal);
				rxf = NULL;
			}
		} 
		Sleep(50);
	}
}

HBRUSH bgBrush;

#define BGCOLOUR RGB(236,233,216)

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int err, ret, i=0;
	char Title[80];
	HKEY hKey = 0;
	HKEY hKey2 = 0;
	char Key[80];
	char msg[80];
	WNDCLASS  wc;
	struct modemchannel * Info;

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

	wsprintf(Title, "SoundModem Interface %s", Config);

	SetWindowText(hWnd, Title);

	if (!hWnd)
	{
		err=GetLastError();
		return FALSE;
	}

	hDCD = GetDlgItem(hWnd, IDC_DCD);
	hPTT = GetDlgItem(hWnd, IDC_PTT);

	cfgMinToTray = GetMinimizetoTrayFlag();

	if ((nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_SHOWMINNOACTIVE))
		if (cfgMinToTray)
			ShowWindow(hWnd, SW_HIDE);
		else
			ShowWindow(hWnd, nCmdShow);
	else
		ShowWindow(hWnd, nCmdShow);

   	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	if (cfgMinToTray)
		AddTrayMenuItem(hWnd, "SoundModem TNC");

	ExtDriver=LoadLibrary("soundmodem.dll");

	if (ExtDriver)
		SoundModem = (FARPROCX)GetProcAddress(ExtDriver,"flexnet_driver");

	if (SoundModem == NULL)
	{
		wsprintf(msg,"Couldn't Load soundmodem.dll");
		SetDlgItemText(hWnd, IDC_STATUS, msg);

		return TRUE;
	}

	wsprintf(Key, "SOFTWARE\\FlexNet\\SoundModem\\%s", Config);

	ret = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey);

	if (ret != ERROR_SUCCESS)
	{
		wsprintf(msg,"Couldn't find SoundModem Config");
		SetDlgItemText(hWnd, IDC_STATUS, msg);

		return TRUE;
	}

	wsprintf(Key, "SOFTWARE\\FlexNet\\SoundModem\\%s\\audio", Config);

	ret = RegOpenKeyEx (HKEY_LOCAL_MACHINE, Key, 0, KEY_QUERY_VALUE, &hKey2);

	wsprintf(msg,"Couldn't find SoundCard Config");

	if (ret == ERROR_SUCCESS)
	{
		int retCode, Type, Vallen = 80;

		retCode = RegQueryValueEx(hKey2,"dscapture",0,			
			&Type, (UCHAR *)&msg, &Vallen);

	}

	SetDlgItemText(hWnd, IDC_SOUNDCARD, msg);
	RegCloseKey(hKey2);


	SoundModem(drv_init_device, 0, 0, hKey);

	wsprintf(msg,"SoundModem Started");
	SetDlgItemText(hWnd, IDC_STATUS, msg);

	RegCloseKey(hKey);

//		SoundModem(drv_config, 4, NULL, 1);

	ChannelCount = SoundModem(drv_get_ch_cnt, 1, 0, 0);

	if (ChannelCount == 0)
	{
		wsprintf(msg,"No Channels Defined");
		SetDlgItemText(hWnd, IDC_STATUS, msg);

		return TRUE;
	}
	
	// Build buffer pool
	
	for ( i  = 0; i < NUMBEROFBUFFERS; i++ )
	{	
		ReleaseBuffer(&BufferPool[100*i]);
	}

	srand((unsigned)time(NULL));		// Seed the CSMA random number 

	for (i = 0; i < ChannelCount; i++)
	{
		if (SoundCardInterface(ConfigNo, CHECKCHAN, i, 0))
		{
			SoundModem(drv_init_kanal, i, 0, 0);
			Info = (struct modemchannel *)SoundModem(drv_confinfo, i, 0, 0);

			wsprintf(msg," (%s) %d", Info->devname, Info->bitrate);
			SetDlgItemText(hWnd, IDC_MODEM1 + i, msg);
		}
	}

	hRXThread = _beginthread(RXThread,0,0);

	TimerHandle = SetTimer(hWnd,WM_TIMER, 100, NULL);

	return (TRUE);

}

VOID TimerProc()
{
	// Called every 100 ms (or so!)

	// Look for messages from BPQ32 and run csma timer

	L1FRAME * txf;
	UINT * buffptr;
	int NewState = SoundModem(drv_ch_state, 0);

	if (State != NewState)
	{
		RECT Rect = {0, 0, 1000, 100};
		State = NewState;
		InvalidateRect(hWnd, &Rect, FALSE);
		Debugprintf("State %x", State);
	}

	buffptr = (UINT *)SoundCardInterface(ConfigNo, POLL, State, 0);	// Pass State to BPQ32 for stats

	if (buffptr)	
	{
		int Channel =  buffptr[2];

		if (TXQ)
		{
			// Already have some queued, so must queue this one

			UINT * buffptr2;
		
		Queueit:

			buffptr2 = Q_REM(&FREE_Q);

			if (buffptr2)
			{
				memcpy(buffptr2, buffptr, buffptr[1] + 12);
				Q_ADD(&TXQ, buffptr);
			}
			return;
		}
		else
		{
			// Nothing queued

			if (State & 0x20)		// Already sending so send this one
				goto Sendit;

			if (State & 0x10)		// If DCD active queue it.
				goto Queueit;

		Sendit:

			Debugprintf("TX Frame chan %d", Channel);

			if (txf = (L1FRAME *)SoundModem(drv_get_framebuf, Channel, 0, 0))
			{
				int len = buffptr[1];
			
				txf->len = len;
				memcpy(txf->frame, &buffptr[3], len);
				txf->txdelay = 0;
				SoundModem(drv_tx_frame, 0, 0, 0);
			}
			return;
		}
	}

	if ((State & 0x10) == 0)
	{
		// DCD False - if frames queued, try to send

		if (TXQ)
		{
			UCHAR rnd = rand() & 255;
				
			Debugprintf ("CSMA rnd = %d, PP = %d", rnd, PORTPERSISTANCE);
				
			if (rnd > PORTPERSISTANCE)
			{
				Debugprintf("Waiting SLOTTIME");
			}
			else
			{
				UINT * buffptr;
				int Channel;

				while (TXQ)
				{
					buffptr = Q_REM(&TXQ);
					Channel =  buffptr[2];

					if (txf = (L1FRAME *)SoundModem(drv_get_framebuf, Channel, 0, 0))
					{
						int len = buffptr[1];
						txf->len = len;
						memcpy(txf->frame, &buffptr[3], len);
						ReleaseBuffer(buffptr);

						SoundModem(drv_tx_frame, 0, 0, 0);
					}
				}
			}
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

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

UINT ReleaseBuffer(UINT *BUFF)
{
	UINT * pointer;
	
	(UINT)pointer=FREE_Q;
	*BUFF=(UINT)pointer;
	FREE_Q=(UINT)BUFF;

	return (0);
}


int Q_ADD(UINT *Q,UINT *BUFF)
{
	UINT * next;
	
	BUFF[0]=0;							// Clear chain in new buffer
	if (Q[0] == 0)						// Empty
	{
		Q[0]=(UINT)BUFF;				// New one on front
		return(0);
	}

	(int)next=Q[0];

	while (next[0]!= 0)

		next = (UINT *)next[0];			// Chain to end of queue

	next[0] = (UINT)BUFF;				// New one on end

	return(0);
}


