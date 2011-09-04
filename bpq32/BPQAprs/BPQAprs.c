#define _CRT_SECURE_NO_DEPRECATE 

//------------------------------------
//  VisualPng.C -- Shows a PNG image
//------------------------------------

// Copyright 2000, Willem van Schaik.  For conditions of distribution and
// use, see the copyright/license/disclaimer notice in png.h

// constants

#define MARGIN 4

// standard includes

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <malloc.h>
#include <memory.h>

#include <setjmp.h>


#define M_PI       3.14159265358979323846


// application includes

#include "png.h"
#include "pngfile.h"
#include "resource.h"

#include "bpq32.h"

#define APRS
#include "Versions.h"
#include "GetVersion.h"

#define BPQICON 2

#include "BPQAPRS.h"

// macros

// function prototypes

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

BOOL LoadImageFile(HWND hwnd, PTSTR pstrPathName,
        png_byte **ppbImage, int *pxImgSize, int *pyImgSize, int *piChannels,
        png_color *pBkgColor);

BOOL DisplayImage (HWND hwnd, BYTE **ppDib,
        BYTE **ppDiData, int cxWinSize, int cyWinSize,
        BYTE *pbImage, int cxImgSize, int cyImgSize, int cImgChannels,
        BOOL bStretched);

BOOL InitBitmap (
        BYTE *pDiData, int cxWinSize, int cyWinSize);

BOOL FillBitmap (int x, int y);

VOID LoadImageSet(int Zoom, int startx, int starty);

// a few global variables


char OSMDir[MAX_PATH] = "C:\\OSMTiles";


static png_color          bkgColor = {127, 127, 127};
static BOOL               bStretched = FALSE;
static BYTE              *pDib = NULL;
static BYTE              *pDiData = NULL;

static png_structp png_ptr = NULL;
static png_infop info_ptr = NULL;

UCHAR * pbImage;

// Image chunks are 256 rows of 3 * 256 bytes

// Read 8 * 8 files, and copy to a 2048 * 3 * 2048 array. The display scross over this area, and
// it is refreshed when window approaches the edge of the array.

UCHAR Image[2048 * 3 * 2048];


int SetBaseX;				// Lowest Tile
int SetBaseY;

int Zoom;

static int cxWinSize, cyWinSize;
static int cxImgSize, cyImgSize;
static int cImgChannels;

int ScrollX;
int ScrollY;

int MapCentreX;
int MapCentreY;

int MouseX, MouseY;

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

HINSTANCE hInst; 
HWND hWnd, hPopupWnd;

char szAppName[] = "BPQAPRS";
char szTitle[80]   = "BPQAPRS" ; // The title bar text

BOOL APRSISOpen = FALSE;

int StationCount=0;

struct STATIONRECORD ** StationRecords;

BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int NewLine(HWND hWnd);
int	ProcessBuff(HWND hWnd, UCHAR * readbuff,int len,int stamp);
int TogglePort(HWND hWnd, int Item, int mask);
int CopyScreentoBuffer(char * buff);
VOID SendFrame(UCHAR * buff, int txlen);
int	KissEncode(UCHAR * inbuff, UCHAR * outbuff, int len);
int	KissDecode(UCHAR * inbuff, int len);
VOID ResolveThread();
VOID APRSISThread();
struct STATIONRECORD * FindStation(char * Call);
void UpdateStation(char * Call, char * Path, char * Comment, double V_Lat, double V_Lon, double V_SOG, double V_COG, int iconRow, int iconCol);
VOID DrawStation(struct STATIONRECORD * ptr);
VOID FindStationsByPixel(int MouseX, int MouseY);
void RefreshStationList();
void DecodeAPRSISMsg(char * msg);
BOOL DecodeLocationString(char * Payload, struct STATIONRECORD * Station);
VOID DecodeAPRSPayload(char * Payload, struct STATIONRECORD * Station);

VOID OSMThread(char * tile, char * FN);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, int arglist);

SOCKADDR_IN destaddr;

unsigned int ipaddr;

unsigned short port = 10091;
UCHAR BPQPort = 7;

char Host[] = "gm8bpq.no-ip.com";

char Screen[22000];
char readbuff[512];
int baseline=216;
int Stream;

extern short CRCTAB;

LOGFONT LFTTYFONT ;

HFONT hFont ;

SOCKET udpsock;

BOOL MinimizetoTray=FALSE;


VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);

	va_start(arglist, format);
	vsprintf(Mess, format, arglist);
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);

	return;
}

int long2tilex(double lon, int z) 
{ 
	return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z))); 
}
 
int lat2tiley(double lat, int z)
{ 
	return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z))); 
}

double long2x(double lon, int z) 
{ 
	return (lon + 180.0) / 360.0 * pow(2.0, z); 
}
 
double lat2y(double lat, int z)
{ 
	return (1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z); 
}


double tilex2long(double x, int z) 
{
	return x / pow(2.0, z) * 360.0 - 180;
}
 
double tiley2lat(double y, int z) 
{
	double n = M_PI - 2.0 * M_PI * y / pow(2.0, z);
	return 180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n)));
}

void GetMouseLatLon(double * Lat, double * Lon)
{
	int X = ScrollX + MouseX;
	int Y = ScrollY + MouseY;

	*Lat = tiley2lat(SetBaseY + (Y / 256.0), Zoom);
	*Lon = tilex2long(SetBaseX + (X / 256.0), Zoom);
}

BOOL CentrePosition(double Lat, double Lon)
{
	// Positions the centre of the map at the specified location

	int X, Y, Pointer, i, j;

	if (GetLocPixels(Lat, Lon, &X, &Y) == FALSE)
		return FALSE;							// Off map

	// if ScrollX, Y are zero, the centre of the map corresponds to 1024, 1024
	
	ScrollX -= 1024 - X;
	ScrollY -= 1024 - Y;

	if (ScrollX < 0 || ScrollY < 0)
	{
		// Need to move image

		while(ScrollX < 0)
		{
			SetBaseX++;
			ScrollX += 256;
		}

		while(ScrollY < 0)
		{
			SetBaseY++;
			ScrollY += 256;
		}

		LoadImageSet(Zoom, SetBaseX, SetBaseY);
	}
	return TRUE;
}

BOOL CentrePositionToMouse(double Lat, double Lon)
{
	// Positions  specified location at the mouse

	int X, Y, Pointer, i, j;

	if (GetLocPixels(Lat, Lon, &X, &Y) == FALSE)
		return FALSE;							// Off map

	// if ScrollX, Y are zero, the centre of the map corresponds to 1024, 1024
	
	ScrollX -= 1024 - X;				// Posn to centre
	ScrollY -= 1024 - Y;

	ScrollX += cxWinSize/2 - MouseX;
	ScrollY += cyWinSize/2 - MouseY;

	if (ScrollX < 0 || ScrollY < 0)
	{
		// Need to move image

		while(ScrollX < 0)
		{
			SetBaseX++;
			ScrollX += 256;
		}

		while(ScrollY < 0)
		{
			SetBaseY++;
			ScrollY += 256;
		}

		LoadImageSet(Zoom, SetBaseX, SetBaseY);
	}
	return TRUE;
}

	
BOOL GetLocPixels(double Lat, double Lon, int * X, int * Y)
{
	// Get the pixel offet of supplied location in current image.

	// If location is outside current image, return FAlSE

	int TileX;
	int TileY;
	int OffsetX, OffsetY;
	double FX;
	double FY;

	// if TileX or TileY are outside the window, return null

	FX = long2x(Lon, Zoom);
	TileX = (int)floor(FX);
	OffsetX = TileX - SetBaseX;

	if (OffsetX < 0 || OffsetX > 7)
		return FALSE;

	FY = lat2y(Lat, Zoom);
	TileY = (int)floor(FY);
	OffsetY = TileY - SetBaseY;

	if (OffsetY < 0 || OffsetY > 7)
		return FALSE;

	FX -= TileX;
	FX = FX * 256.0;

	*X = (int)FX + 256 * OffsetX;

	FY -= TileY;
	FY = FY * 256.0;

	*Y = (int)FY + 256 * OffsetY;

	return TRUE;
}




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
}

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
	wc.lpszMenuName =  NULL;
	wc.lpszClassName = szAppName;

	// Register the window class and return success/failure code.

	return RegisterClass(&wc);
}

int len,count,i;
char msg[20];

HMENU hMenu,hPopMenu1,hPopMenu2,hPopMenu3;		// handle of menu 

UCHAR * iconImage = NULL;

char Test[] = "2E0AYY>APU25N,TCPIP*,qAC,AHUBSWE2:=5105.18N/00108.19E-Paul in Folkestone Kent {UIV32N}\r\n";

CRITICAL_SECTION Crit;
	
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	BOOL bcopt=TRUE;
	u_long param=1;
	WSADATA WsaData;            // receives data from WSAStartup
	int ImgSizeX, ImgSizeY, ImgChannels;
	png_color bgColor;

	InitializeCriticalSection(&Crit); 
      
	// hBitmap = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_SYMBOL1), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

	LoadImageFile (NULL, "S:\\DEV\\Source\\BPQ32\\BPQAprs\\mysymbx.png", &iconImage, &ImgSizeX, &ImgSizeY, &ImgChannels, &bgColor);

	hInst = hInstance; // Store instance handle in our global variable

	MinimizetoTray=GetMinimizetoTrayFlag();

	hWnd = CreateWindow(szAppName, szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 800, 600,
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
	
	SetTimer(hWnd,1,1000,NULL);

	memset(Screen, ' ', 20000); 

	for (Stream = 63; Stream > 0; Stream--)
	{
		if (AllocateStream(Stream) == 0)
			break;
	}

	if (Stream == 0)
	{
		MessageBox(NULL, "Couldn't find a free stream", "BPQAPRS", MB_OK);
		return (FALSE);
	}

	//
	//	Register message for posting by BPQDLL
	//

	BPQMsg = RegisterWindowMessage(BPQWinMsg);

	BPQSetHandle(Stream, hWnd);
	
	SetWindowText(hWnd,szTitle);

	UpdateStation("GM8BPQ", "", "", 58.47583, -6.21151, 0.0, 0.0, ('=' - '!') >> 4, ('=' - '!') & 15);
	
	DecodeAPRSISMsg(Test);

//	Top bit of mask controlls monitoring

	SetAppl(Stream,0x80,0);
/*
;	hMenu=GetMenu(hWnd);
	hMenu=CreateMenu();
	hPopMenu1=CreatePopupMenu();
	hPopMenu2=CreatePopupMenu();
	hPopMenu3=CreatePopupMenu();
	SetMenu(hWnd,hMenu);


	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu1,"Ports");
	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu2,"Flags");
	AppendMenu(hMenu,MF_STRING + MF_POPUP,(UINT)hPopMenu3,"Edit");

	for (i=1;i <= GetNumberofPorts();i++)
	{
		wsprintf(msg,"Port %d",i);
		AppendMenu(hPopMenu1,
			MF_STRING | MF_CHECKED,BPQBASE + i,msg);
	}

	
	AppendMenu(hPopMenu3,MF_STRING,BPQCOPY,"Copy");

	DrawMenuBar(hWnd);	
*/
//	SetScrollRange(hWnd,SB_VERT,0,216,TRUE);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON

		AddTrayMenuItem(hWnd, "BPQAPRS");
	}
	
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	ScrollX = MapCentreX;
	ScrollY = MapCentreY;

	Zoom = 6;
	Zoom = 3;
	
//	SetBaseX = long2tilex(-6.3, Zoom);
//	SetBaseY = lat2tiley(58.5, Zoom);

	SetBaseX = long2tilex(-7.0, Zoom) - 4;
	SetBaseY = lat2tiley(59, Zoom) - 4;				// Set Location at middle

	while(SetBaseX < 0)
	{
		SetBaseX++;
		ScrollX -= 256;
	}

	while(SetBaseY < 0)
	{
		SetBaseY++;
		ScrollY -= 256;
	}

	LoadImageSet(Zoom, SetBaseX, SetBaseY);

	WSAStartup(MAKEWORD(2, 0), &WsaData);

//	OSMThread("/01/1/0.png");

	udpsock = socket(AF_INET,SOCK_DGRAM,0);

	if (udpsock == INVALID_SOCKET)
	{
		int err;
		MessageBox(NULL, (LPSTR) "Failed to create UDP socket",NULL,MB_OK);
		err = WSAGetLastError();
  	 	return FALSE; 
	}

	ioctlsocket (udpsock,FIONBIO,&param);
 
	setsockopt (udpsock,SOL_SOCKET,SO_BROADCAST,(const char FAR *)&bcopt,4);
/*
	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = htons(udpport[i]);

	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(0);

	if (bind(udpsock[i], (LPSOCKADDR) &sinx, sizeof(sinx)) != 0 )
	{
		//
		//	Bind Failed
		//
		err = WSAGetLastError();
		wsprintf(Msg, "Bind Failed for UDP socket - error code = %d", err);
		MessageBox(NULL,Msg,NULL, MB_OK);
		return;
	}
*/
	
	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(0);

	_beginthread(ResolveThread,0,0);

	_beginthread(APRSISThread,0,0);

	return (TRUE);
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  MESSAGES:
//
//	WM_COMMAND - process the application menu
//	WM_PAINT - Paint the main window
//	WM_DESTROY - post a quit message and return
//    WM_DISPLAYCHANGE - message sent to Plug & Play systems when the display changes
//    WM_RBUTTONDOWN - Right mouse click -- put up context menu here if appropriate
//    WM_NCRBUTTONUP - User has clicked the right button on the application's system menu
//
//

BOOL MouseLeftDown = FALSE;

int DownX;			// Mouse coords when left button was pressed
int DownY;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HGLOBAL	hMem;
	int DeltaX, DeltaY;
	double MouseLat, MouseLon;

	int stamp;
	int nScrollCode,nPos;

	if (message == BPQMsg)
	{
		if (MONCount(Stream) > 0)
		{
			do {
		
			stamp=GetRaw(Stream, readbuff,&len,&count);
			
			if (len > 0)
			{
				ProcessBuff(hWnd,readbuff,len,stamp);
				if (count == 0)
					InvalidateRect(hWnd,NULL,FALSE);
			}

			} while (count > 0);

		}
	}

	switch (message)
	{
		case WM_CHAR:

			GetMouseLatLon(&MouseLat, &MouseLon);

			if (wParam == '=')
				Zoom++;
			if (wParam == '-')
				Zoom--;

			if (Zoom < 3)
			{
				Zoom = 3;
				return TRUE;
			}
			if (Zoom > 14)
			{
				Zoom = 14;
				return TRUE;
			}
			
			// Centre on Current Cursor
			
			SetBaseX = long2tilex(MouseLon, Zoom) - 4;
			SetBaseY = lat2tiley(MouseLat, Zoom) - 4;

			ScrollX = MapCentreX;
			ScrollY = MapCentreY;

			while(SetBaseX < 0)
			{
				SetBaseX++;
				ScrollX -= 256;
			}

			while(SetBaseY < 0)
			{
				SetBaseY++;
				ScrollY -= 256;
			}

			LoadImageSet(Zoom, SetBaseX, SetBaseY);

			CentrePositionToMouse(MouseLat, MouseLon);	
			InvalidateRect(hWnd,NULL,FALSE);

			break;

		case 0x020A:				//WM_MOUSEWHEEL  

			if ((int)wParam > 0)
				PostMessage(hWnd, WM_CHAR, '=', 0);
			else
				PostMessage(hWnd, WM_CHAR, '-', 0);

			break;

		case WM_MOUSEMOVE:
			
			if (MouseX == LOWORD(lParam) || MouseY == HIWORD(lParam))
				return TRUE;		// Not Moved

			MouseX = LOWORD(lParam);
			MouseY = HIWORD(lParam);
			
			if (MouseLeftDown)
			{
				// Dragging


				DeltaX = MouseX - DownX;
				DeltaY = MouseY - DownY;

				ScrollX -= DeltaX;
				ScrollY -= DeltaY;

				if (ScrollX < 0)
				{
					// Load Previous Tile
					
					SetBaseX--;
					
					if (SetBaseX)
					{
						LoadImageSet(Zoom, SetBaseX, SetBaseY);
						ScrollX += 256;
					}
					else
						ScrollX = 0;

				}
				if (ScrollY < 0)
				{
					// Load Previous Tile
					
					SetBaseY--;
					
					if (SetBaseY)
					{
						LoadImageSet(Zoom, SetBaseX, SetBaseY);
						ScrollY += 256;
					}
					else
						ScrollY= 0;

				}

				if (ScrollX > 2048 - cxWinSize)
				{
					// Load Next Tile

					SetBaseX++;
					
//					if (SetBaseX)
					{
						LoadImageSet(Zoom, SetBaseX, SetBaseY);
						ScrollX -= 256;
					}
//					else
//						ScrollX = 2048 - cxWinSize;

				}

				if (ScrollY > 2048 - cyWinSize)
				{
					// Load Next Tile

					SetBaseY++;
					
//					if (SetBaseX)
					{
						LoadImageSet(Zoom, SetBaseX, SetBaseY);
						ScrollY -= 256;
					}
//					else
//						ScrollX = 2048 - cxWinSize;

				}


				InvalidateRect(hWnd, NULL, FALSE);

				DownX = MouseX;
				DownY = MouseY;

				break;
			}

			// Left not down - see if cursor is over a station Icon

			FindStationsByPixel(MouseX + ScrollX, MouseY + ScrollY);
			
			break;
			
		case WM_LBUTTONDOWN:

			MouseLeftDown = TRUE;
			DownX = LOWORD(lParam);
			DownY = HIWORD(lParam);
			break;

		case WM_LBUTTONUP:
			
			MouseLeftDown = FALSE;
			MouseX = LOWORD(lParam);
			MouseY = HIWORD(lParam);

			break;

		case WM_VSCROLL:
		
			nScrollCode = (int) LOWORD(wParam); // scroll bar value 
			nPos = (short int) HIWORD(wParam);  // scroll box position 

			//hwndScrollBar = (HWND) lParam;      // handle of scroll bar 

			if (nScrollCode == 0)
			{
				baseline--;
				if (baseline <0)
					baseline=0;
			}
			if (nScrollCode == 1)
			{

				baseline++;
				if (baseline > 216)
					baseline = 216;
			}

			SetScrollPos(hWnd,SB_VERT,baseline,TRUE);

			InvalidateRect(hWnd,NULL,FALSE);
			break;
		
		case WM_SIZE:
        
			cxWinSize = LOWORD (lParam);
			cyWinSize = HIWORD (lParam);

			MapCentreX = (2048 - cxWinSize) /2;
			MapCentreY = (2048 - cyWinSize) /2;

			break;


		case WM_PAINT:

			hdc = BeginPaint (hWnd, &ps);

			RefreshStationList();
			
			DisplayImage (hWnd, &pDib, &pDiData, cxWinSize, cyWinSize,
                pbImage, cxImgSize, cyImgSize, cImgChannels, bStretched);

			if (pDib)
				SetDIBitsToDevice (hdc, 0, 0, cxWinSize, cyWinSize, 0, 0,
					0, cyWinSize, pDiData, (BITMAPINFO *) pDib, DIB_RGB_COLORS);

			
			EndPaint (hWnd, &ps);
			break; 
			
				
		case WM_SYSCOMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			switch (wmId)
			{ 	
			case  SC_MINIMIZE: 

				if (MinimizetoTray)

					return ShowWindow(hWnd, SW_HIDE);
				else
					return (DefWindowProc(hWnd, message, wParam, lParam));
					
				break;
		
			default:
		
				return (DefWindowProc(hWnd, message, wParam, lParam));
			}

		case WM_DESTROY:
			
			DeallocateStream(Stream);
			DestroyMenu(hMenu);

			PostQuitMessage(0);

			if (MinimizetoTray) 
				DeleteTrayMenuItem(hWnd);
			
			break;

		case WM_COMMAND:

			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			//Parse the menu selections:
		
			if (wmId > BPQBASE && wmId < BPQBASE + 17)
			{
				TogglePort(hWnd,wmId,0x1 << (wmId - (BPQBASE + 1)));
				break;
			}
		
			switch (wmId)
			{
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

			default:
				return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0);
}

unsigned short int compute_crc(unsigned char *buf,int len)
{
	int fcs;

	_asm{

	mov	esi,buf
	mov	ecx,len
	mov	edx,-1		; initial value

crcloop:

	lodsb

	XOR	DL,AL		; OLD FCS .XOR. CHAR
	MOVZX EBX,DL		; CALC INDEX INTO TABLE FROM BOTTOM 8 BITS
	ADD	EBX,EBX
	MOV	DL,DH		; SHIFT DOWN 8 BITS
	XOR	DH,DH		; AND CLEAR TOP BITS
	XOR	DX,CRCTAB[EBX]	; XOR WITH TABLE ENTRY
	
	loop	crcloop

	mov	fcs,EDX

	}	

	return (fcs);

  }


int line=240;
int col=0;

int	ProcessBuff(HWND hWnd, UCHAR * buff,int buflen, int timestamp)
{
	unsigned short int crc;
	int txlen=(buff[6]<<8) + buff[5] - 5;			// Len includes buffer header (7) but we add crc

	buff[4] &=  127	;			// Mask TX bit
								
	if (buff[4] != BPQPort)
		return 0;									// Only Port 1

	crc=compute_crc(&buff[7], txlen - 2);
	crc ^= 0xffff;

	buff[txlen+5]=(crc&0xff);
	buff[txlen+6]=(crc>>8);

	// Pass to g8bpq.org.uk port 10091
	
	destaddr.sin_port = htons(port);

	sendto(udpsock, &buff[7], txlen, 0,(LPSOCKADDR)&destaddr, sizeof(destaddr));		

	return (0);
}
int xNewLine()
{
	while (col <80)
		Screen[line*80+col++] = ' ';
	
	col=0;
	line++;
	if (line >= 250)
	{
		line=0;
	}

	baseline=line-25;

	if (baseline<0)
		baseline=baseline+249;

	return (0);
}

int NewLine(HWND hWnd)
{

	col=0;
	line++;
	if (line > 240)
	{
		memmove(Screen,Screen+80,19200);
		memset(Screen+19200,' ',80);
		line=240;
		baseline=216;
	}

		SetScrollPos(hWnd,SB_VERT,baseline,TRUE);

	return (0);
}

int portmask=0xffff;
int mtxparam=1;
int mcomparam=1;

int TogglePort(HWND hWnd, int Item, int mask)
{
	portmask = portmask ^ mask;
	
	if (portmask & mask)

		CheckMenuItem(hMenu,Item,MF_CHECKED);
	
	else

		CheckMenuItem(hMenu,Item,MF_UNCHECKED);

	SetTraceOptions(portmask,mtxparam,mcomparam);

    return (0);
  
}

int CopyScreentoBuffer(char * buff)
{
	int i,j;
	char line [82];
	
	for (i=0;i != 241; i++)
	{
		memcpy(line,&Screen[i*80],80);
		
		//
		//	scan line backwards, and replace last space with crlf
		//

		for (j=79;j != -1; j--)
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

VOID ResolveThread()
{
	struct hostent * HostEnt;
	int err;

	while (TRUE)
	{

	// Resolve Name if needed

	HostEnt = gethostbyname(Host);
		 
	if (!HostEnt)
	{
		err = WSAGetLastError();

		Debugprintf("Resolve Failed for %s %d %x", Host, err, err);
	}
	else
	{
		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
	}


	Sleep(60 * 15 * 1000);
	}

	return;
}

char Header[] = "Accept: */*\r\nHost: tile.openstreetmap.org\r\nConnection: close\r\nContent-Length: 0\r\nUser-Agent: BPQ32(G8BPQ)\r\n\r\n";

VOID OSMThread(char * Tile, char * FN)
{
	// Request a page from OSM

	short Port = 80;
	char Host[] = "tile.openstreetmap.org";

	SOCKET sock;
	SOCKADDR_IN sinx; 
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Request[100];

	UCHAR Buffer[100000];
	int Len, InputLen = 0;
	char errmsg[40];
	char * ptr;
	int inptr = 0;

	// Resolve Name

	Debugprintf(FN);

	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(Port);

	destaddr.sin_addr.s_addr = inet_addr(Host);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
	//	Resolve name to address

		HostEnt = gethostbyname (Host);
		 
		if (!HostEnt)
		{
			err = WSAGetLastError();

			wsprintf(errmsg, TEXT("Res Falied %d %x"),err, err);
			MessageBox(NULL, errmsg, NULL, MB_OK);

			return;			// Resolve failed
		}
		
		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

//   Allocate a Socket entry

	sock=socket(AF_INET,SOCK_STREAM,0);

	if (sock == INVALID_SOCKET)
  	 	return; 
 
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//

		return; 
	}

	if (connect(sock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) != 0)
	{
		err=WSAGetLastError();

		//
		//	Connect failed
		//

		return;
	}

//GET /15/15810/9778.png HTTP/1.0
//Accept: */*
//Host: tile.openstreetmap.org
//Connection: close
//Content-Length: 0
//User-Agent: APRSIS32(G8BPQ)

	Len = wsprintf(Request, "GET %s HTTP/1.0\r\n", Tile);
	send(sock, Request, Len, 0);

	send(sock, Header, strlen(Header), 0);

	while (InputLen != -1)
	{
		InputLen = recv(sock, &Buffer[inptr], 100000 - inptr, 0);

		if (InputLen > 0)
			inptr += InputLen;
		else
		{
			// File Complete??

			if (strstr(Buffer, " 200 OK"))
			{
				ptr = strstr(Buffer, "Content-Length:");

				if (ptr)
				{
					int FileLen = atoi(ptr + 15);
					ptr = strstr(Buffer, "\r\n\r\n");

					if (ptr)
					{
						ptr += 4;
						if (FileLen == inptr - (ptr - Buffer))
						{
							// File is OK

							int cnt;
							
							HANDLE Handle = CreateFile(FN, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
							if (Handle != INVALID_HANDLE_VALUE)
							{
								WriteFile(Handle ,ptr , FileLen, &cnt, NULL);
								CloseHandle(Handle);
							}
							else
							{
								if (GetLastError() == 3)
								{
									// Invalid Path
									
									char * Dir = strdup(FN);
									ptr = strchr(Dir,'/');
									if (ptr)
									{
										ptr = strchr(++ptr,'/');
										if (ptr)
										{
											ptr = strchr(++ptr,'/');
											*ptr = 0;
											CreateDirectory(Dir, NULL);
											 Handle = CreateFile(FN, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
											if (Handle != INVALID_HANDLE_VALUE)
											{
												WriteFile(Handle ,ptr , FileLen, &cnt, NULL);
												CloseHandle(Handle);
											}
										}
									}
									free(Dir);
								}

							}
							break;
						}
					}
				}
			}
			Debugprintf("OSM GET Bad Response");



			break;
		}
	}

	closesocket(sock);

	return;
}



char APRSMsg[300];

VOID APRSISThread()
{
	// Receive from core server

	short Port = 14580;
	char Host[] = "england.aprs2.net";
	//char Host[] = "aprswest.net";

	char Signon[] = "user G8BPQ pass 10122 vers BPQ32 2011/08/20 filter  a/61/-10/49/2\r\n";
//	char Filter[] = "#filter \r\n"; 

	SOCKET sock;
	SOCKADDR_IN sinx; 
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Buffer[1000];
	int InputLen = 0;
	char errmsg[40];
	char * ptr;
	int inptr = 0;
	char APRSinMsg[1000];

	// Resolve Name if needed

	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(Port);

	destaddr.sin_addr.s_addr = inet_addr(Host);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
	//	Resolve name to address

		HostEnt = gethostbyname (Host);
		 
		if (!HostEnt)
		{
			err = WSAGetLastError();

			wsprintf(errmsg, TEXT("Res Falied %d %x"),err, err);
			MessageBox(NULL, errmsg, NULL, MB_OK);

			return;			// Resolve failed
		}
		
		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

//   Allocate a Socket entry

	sock=socket(AF_INET,SOCK_STREAM,0);

	if (sock == INVALID_SOCKET)
  	 	return; 
 
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sock, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//

		return; 
	}

	if (connect(sock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) != 0)
	{
		err=WSAGetLastError();

		//
		//	Connect failed
		//

		return;
	}

	InputLen=recv(sock, Buffer, 5500, 0);

	Buffer[InputLen] = 0;
	Debugprintf(Buffer);

	send(sock, Signon, strlen(Signon), 0);

	InputLen=recv(sock, Buffer, 500, 0);

	Buffer[InputLen] = 0;
	Debugprintf(Buffer);
 
	InputLen=recv(sock, Buffer, 500, 0);

	Buffer[InputLen] = 0;
	Debugprintf(Buffer);

	APRSISOpen = TRUE;

	while (InputLen != -1)
	{
		InputLen = recv(sock, &APRSinMsg[inptr], 500 - inptr, 0);

		if (InputLen > 0)
		{
			inptr += InputLen;

			ptr = memchr(APRSinMsg, 0x0a, inptr);

			while (ptr != NULL)
			{
				ptr++;									// include lf
				len = ptr-(char *)APRSinMsg;					
				memcpy(&APRSMsg, APRSinMsg, len);	

				APRSMsg[len] = 0;
				DecodeAPRSISMsg(APRSMsg);

				inptr -= len;						// bytes left

				if (inptr > 0)
				{
					memmove(APRSinMsg, ptr, inptr);
					ptr = memchr(APRSinMsg, 0x0a, inptr);
				}
				else
					ptr = 0;
			}
		}
	}

	closesocket(sock);

	APRSISOpen = FALSE;

	return;
}

VOID LoadImageSet(int Zoom, int startx, int starty)
{
	char FN[100];
	int i, x, y;
	int StartRow;
	int StartCol;
	char Tile[100];

	int Limit = pow(2, Zoom);

	
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			wsprintf(Tile, "/%02d/%d/%d.png", Zoom, startx + x, starty + y);

			if ((startx + x) >= Limit || (starty + y) >= Limit || startx < 0 || starty + y < 0)
			{
				if (pbImage)
					free(pbImage);
				pbImage = NULL;
				goto NoFile;
			}
			wsprintf(FN, "%s%s", OSMDir, Tile);

			__try {
				
				LoadImageFile (NULL, FN, &pbImage, &cxImgSize, &cyImgSize, &cImgChannels, &bkgColor);
		}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				Debugprintf("Corrupt file %s", FN);
				DeleteFile(FN);

				if (pbImage)
				{
					free(pbImage);
					pbImage = NULL;
				}
			}

			if (pbImage == 0)
				OSMThread(Tile, FN);

			// Copy to Image Array 3 * 256 *250
		NoFile:
			StartCol = x * 768;
			StartRow = y * 256;

			for (i = 0; i < 256; i++)
			{
				if (pbImage)
					memcpy(&Image[((StartRow +i) * 2048 * 3) + StartCol], &pbImage[768 * i], 768);
				else
					memset(&Image[((StartRow +i) * 2048 * 3) + StartCol], 0x80, 768);
			}
		}
	}
}

BOOL LoadImageFile (HWND hwnd, PTSTR pstrPathName,
                png_byte **ppbImage, int *pxImgSize, int *pyImgSize,
                int *piChannels, png_color *pBkgColor)
{
    static TCHAR szTmp [MAX_PATH];

    // if there's an existing PNG, free the memory

    if (*ppbImage)
    {
        free (*ppbImage);
        *ppbImage = NULL;
    }

    // Load the entire PNG into memory

 //   SetCursor (LoadCursor (NULL, IDC_WAIT));
 //   ShowCursor (TRUE);

    PngLoadImage (pstrPathName, ppbImage, pxImgSize, pyImgSize, piChannels,
                  pBkgColor);

 //   ShowCursor (FALSE);
 //   SetCursor (LoadCursor (NULL, IDC_ARROW));

    if (*ppbImage != NULL)
    {
  //      sprintf (szTmp, "VisualPng - %s", strrchr(pstrPathName, '\\') + 1);
   //     SetWindowText (hwnd, szTmp);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

//----------------
//  DisplayImage
//----------------

BOOL DisplayImage (HWND hwnd, BYTE **ppDib,
        BYTE **ppDiData, int cxWinSize, int cyWinSize,
        BYTE *pbImage, int cxImgSize, int cyImgSize, int cImgChannels,
        BOOL bStretched)
{
    BYTE                       *pDib = *ppDib;
    BYTE                       *pDiData = *ppDiData;
    // BITMAPFILEHEADER        *pbmfh;
    BITMAPINFOHEADER           *pbmih;
    WORD                        wDIRowBytes;
    png_color                   bkgBlack = {0, 0, 0};
    png_color                   bkgGray  = {127, 127, 127};
    png_color                   bkgWhite = {255, 255, 255};

    // allocate memory for the Device Independant bitmap

    wDIRowBytes = (WORD) ((3 * cxWinSize + 3L) >> 2) << 2;

    if (pDib)
    {
        free (pDib);
        pDib = NULL;
    }

    if (!(pDib = (BYTE *) malloc (sizeof(BITMAPINFOHEADER) +
        wDIRowBytes * cyWinSize)))
    {
        *ppDib = pDib = NULL;
        return FALSE;
    }
    *ppDib = pDib;
    memset (pDib, 0, sizeof(BITMAPINFOHEADER));

    // initialize the dib-structure

    pbmih = (BITMAPINFOHEADER *) pDib;
    pbmih->biSize = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth = cxWinSize;
    pbmih->biHeight = -((long) cyWinSize);
    pbmih->biPlanes = 1;
    pbmih->biBitCount = 24;
    pbmih->biCompression = 0;
    pDiData = pDib + sizeof(BITMAPINFOHEADER);
    *ppDiData = pDiData;

    // first fill bitmap with gray and image border

    InitBitmap (pDiData, cxWinSize, cyWinSize);

    // then fill bitmap with image

//	for (x = 0; x < 8; x++)
//	{
//		for (y = 0; y < 8; y++)
//		{
//			FillBitmap (x,y);
//		}
//	}

	FillBitmap(0,0);

	return TRUE;
}

//--------------
//  InitBitmap
//--------------

BOOL InitBitmap (BYTE *pDiData, int cxWinSize, int cyWinSize)
{
    BYTE *dst;
    int x, y, col;

    // initialize the background with gray

    dst = pDiData;
    for (y = 0; y < cyWinSize; y++)
    {
        col = 0;
        for (x = 0; x < cxWinSize; x++)
        {
            // fill with GRAY
            *dst++ = 127;
            *dst++ = 127;
            *dst++ = 127;
            col += 3;
        }
        // rows start on 4 byte boundaries
        while ((col % 4) != 0)
        {
            dst++;
            col++;
        }
    }

    return TRUE;
}

//--------------
//  FillBitmap
//--------------


BOOL FillBitmap (int cx, int cy)
{
    BYTE *src, *dst;
    BYTE r, g, b, a;
    const int cDIChannels = 3;
    WORD wImgRowBytes;
    WORD wDIRowBytes;
    int xWin, yWin;
    int xImg, yImg;
	int cxImgPos, cyImgPos;
	
	cxImgPos = cx * 256;
	cxImgPos += MARGIN;
	cyImgPos = cy * 256;
	cyImgPos += MARGIN;

       // calculate both row-bytes

        wImgRowBytes = cImgChannels * 2048;
        wDIRowBytes = (WORD) ((cDIChannels * cxWinSize + 3L) >> 2) << 2;

        // copy image to screen

        for (yImg = 0, yWin = cyImgPos; yImg < cyWinSize; yImg++, yWin++)
        {
            if (yWin >= cyWinSize - MARGIN)
                break;

			if (yImg + ScrollY > 2048)
				break;

            src = Image + yImg * wImgRowBytes + ScrollY * 6144 + ScrollX * 3;
            dst = pDiData + yWin * wDIRowBytes + cxImgPos * cDIChannels;

            for (xImg = 0, xWin = cxImgPos; xImg < 2048; xImg++, xWin++)
            {
                if (xWin >= cxWinSize - MARGIN)
                    break;
                r = *src++;
                g = *src++;
                b = *src++;
                *dst++ = b; /* note the reverse order */
                *dst++ = g;
                *dst++ = r;
                if (cImgChannels == 4)
                {
                    a = *src++;
                }
            }
        }
    
    return TRUE;
}

Myabort()
{
	int i = 10;
	int j = 0;

	i /= j;				// Force PE to trigger __except
	return 0;
}

//	if (GetLocPixels(58.47583, -6.21151, &X, &Y))


VOID DrawStation(struct STATIONRECORD * ptr)
{
	int X, Y, Pointer, i, j;

//	if (ptr->Lat > 60)
//		return;

//	if (GetLocPixels(58.47583, -6.21151, &X, &Y))
	if (GetLocPixels(ptr->Lat, ptr->Lon, &X, &Y))
	{
		ptr->DispX = X;
		ptr->DispY = Y;					// Save for mouse over checks

		// X and Y are offsets into the pixel data in array Image. Actual Bytes are at Y * 2048 * 3 + (X * 3)

		// Draw Icon
		
		Pointer = ((Y -8) * 2048 * 3) + (X * 3) - 24;

		j = ptr->iconRow * 21 * 337 * 3 + ptr->iconCol * 21 * 3 + 9 + 337 * 9;
		for (i = 0; i < 16; i++)
		{
			memcpy(&Image[Pointer], &iconImage[j], 16 * 3);
			Pointer += 6144;
			j += 337 * 3;
		}

/*
		// Draw cross

		for (j = -5; j < 6; j ++)
		{
			Pointer = ((Y + j) * 2048 * 3) + (X * 3);
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
		}

		Pointer = (Y * 2048 * 3) + (X * 3) - 15;

		for (i = 0; i < 11; i++)
		{
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
		}
*/
	}
	else
	{
		ptr->DispX = 0;
		ptr->DispY = 0;			// Off Screen
	}
}

VOID FindStationsByPixel(int MouseX, int MouseY)
{
	int i, j=0;
	struct STATIONRECORD * ptr;
	struct STATIONRECORD * List[1000];

	for (i = 0; i < StationCount; i++)
	{
		ptr=StationRecords[i];
	
		if (abs((ptr->DispX - MouseX)) < 8 && abs((ptr->DispY - MouseY)) < 8)
			List[j++] = ptr;
	}

	if (j == 0)
	{
		if (hPopupWnd)
		{
			DestroyWindow(hPopupWnd);
			hPopupWnd = 0;
		}
		return;
	}
	//	If only one, display info popup, else display selection popup 

	if (hPopupWnd)
		return;						// Already on display

	hPopupWnd = CreateWindow("LISTBOX", List[0]->Callsign, 0, MouseX - ScrollX,
		MouseY - ScrollY, 150, 150, hWnd, NULL, hInst, NULL);

	for (; j > 0; j--)
	{
		SendMessage(hPopupWnd, LB_ADDSTRING, 0, (LPARAM)List[j-1]->Callsign);
	}

	ShowWindow(hPopupWnd, SW_SHOWNORMAL);
}

void RefreshStationList()
{
	int i, j=0;
	struct STATIONRECORD * ptr;

	// Entered Every 5 secs

/*	if (DecayTimer++ > 11)				// 5 Sec timer to minutes
	{
		DecayTimer = 0;
		CheckAgeofTargets(MaxAge);
	}
*/

	for (i = 0; i < StationCount; i++)
	{
		EnterCriticalSection(&Crit);		// In case list is being realloced
	
		ptr=StationRecords[i];

		LeaveCriticalSection(&Crit);		// In case list is being realloced

		if (ptr)
			DrawStation(ptr);
		else
			Debugprintf("Bad Station rec %d", i);
	}
}

struct STATIONRECORD * FindStation(char * Call)
{
	int i;
	struct STATIONRECORD * ptr;

	for (i = 0; i < StationCount; i++)
	{
		ptr = StationRecords[i];
 
	    if (strcmp(ptr->Callsign, Call) == 0)
			return ptr;

	}
 
	//   Not found - add on end

	EnterCriticalSection(&Crit);

	if (StationCount == 0)

		StationRecords = malloc(4);
	else
		StationRecords=realloc(StationRecords, (StationCount + 1) * 4);

	ptr = malloc(sizeof(struct STATIONRECORD));
	memset(ptr, 0, sizeof(struct STATIONRECORD));

	if (ptr == NULL) return NULL;
	
	StationRecords[StationCount] = ptr;

	StationCount++;

	LeaveCriticalSection(&Crit);
        
	strcpy(ptr->Callsign, Call);
	ptr->TimeAdded = time(NULL);


	return ptr;
}


void UpdateStation(char * Call, char * Path, char * Comment, double V_Lat, double V_Lon, double V_SOG, double V_COG, int iconRow, int iconCol)
{
	int i;
	struct STATIONRECORD * ptr;

	for (i = 0; i < StationCount; i++)
	{
		ptr=StationRecords[i];
 
	    if (strcmp(ptr->Callsign, Call) == 0)
		{       
            ptr->Lat = V_Lat;
            ptr->Lon = V_Lon;
            ptr->Course = V_COG;
            ptr->Speed = V_SOG;
			ptr->iconRow = iconRow;
			ptr->iconCol = iconCol;
			if (Path)
				strcpy(ptr->Path, Path);

			if (Comment)
				strcpy(ptr->Comment, Comment);

//            ptr->LatIncr = ptr->speed * cos(radians(ptr->course)) / 216000;
//            ptr->LongIncr = ptr->speed * sin(radians(ptr->course)) / cos(radians(ptr->lat)) / 216000;
    
            ptr->TimeLastUpdated = time(NULL);
			
			return;
		}
	}

//   Not found - add on end

	if (StationCount == 0)

		StationRecords = malloc(4);
	else
		StationRecords=realloc(StationRecords, (StationCount + 1) * 4);

	ptr = malloc(sizeof(struct STATIONRECORD));
	memset(ptr, 0, sizeof(struct STATIONRECORD));

	if (ptr == NULL) return;
	
	StationRecords[StationCount] = ptr;
        
	strcpy(ptr->Callsign, Call);
	ptr->TimeAdded = time(NULL);

	StationCount++;

	UpdateStation(Call, "", "", V_Lat, V_Lon, V_SOG, V_COG, iconRow, iconCol);

	return;
}

/*
2E0AYY>APU25N,TCPIP*,qAC,AHUBSWE2:=5105.18N/00108.19E-Paul in Folkestone Kent {UIV32N}
G0AVP-12>APT310,MB7UC*,WIDE3-2,qAR,G3PWJ:!5047.19N\00108.45Wk074/000/Paul mobile
G0CJM-12>CQ,TCPIP*,qAC,AHUBSWE2:=/3&R<NDEp/  B>io94sg
M0HFC>APRS,WIDE2-1,qAR,G0MNI:!5342.83N/00013.79W# Humber Fortress ARC Look us up on QRZ
G8WVW-3>APTT4,WIDE1-1,WIDE2-1,qAS,G8WVW:T#063,123,036,000,000,000,00000000
*/


void DecodeAPRSISMsg(char * Msg)
{
	char * Payload;
	char * Path = NULL;
	char * Comment = NULL;
	char * Callsign;
	struct STATIONRECORD * Station;

	Debugprintf(Msg);
	
	Payload = strchr(Msg, ':');			// End of Address String

	if (Payload == NULL)
	{
		Debugprintf("Invalid Msg %s", Msg);
		return;
	}

	*Payload++ = 0;

	Callsign = Msg;

	Path = strchr(Msg, '>');

	if (Path == NULL)
	{
		Debugprintf("Invalid Meader %s", Msg);
		return;
	}

	*Path++ = 0;

	// Look up station - create a new one if not found

	Station = FindStation(Callsign);

	DecodeAPRSPayload(Payload, Station);

//		UpdateStation(Callsign, Path, Comment, Lat, Lon, 0.0, 0.0, Symbol >> 4, Symbol & 15);


	// No APRS-like location. Probably a normal beacon

	InvalidateRect(hWnd, NULL, FALSE);

}

double Cube91 = 91.0 * 91.0 * 91.0;
double Square91 = 91.0 * 91.0;

BOOL DecodeLocationString(char * Payload, struct STATIONRECORD * Station)
{
	UCHAR SymChar;
	char SymSet;
	char NS;
	char EW;
//	double Lat,Lon;
	char LatDeg[3], LonDeg[4];

	// Compressed has first character not a digit (it is symbol table)

	// /YYYYXXXX$csT

	if (!isdigit(*Payload))
	{
		SymSet = *Payload;
		SymChar = Payload[9];

		Station->Lat = 90.0 - ((Payload[1] - 33) * Cube91 + (Payload[2] - 33) * Square91 +
			(Payload[3] - 33) * 91.0 + (Payload[4] - 33)) / 380926.0;

		Payload += 4;
				
		Station->Lon = -180.0 + ((Payload[1] - 33) * Cube91 + (Payload[2] - 33) * Square91 +
			(Payload[3] - 33) * 91.0 + (Payload[4] - 33)) / 190463.0;
	}
	else
	{
		// Standard format ddmm.mmN/dddmm.mmE?

		NS = Payload[7];
		EW = Payload[17];

		SymSet = Payload[8];
		SymChar = Payload[18];

		memcpy(LatDeg, Payload,2);
		LatDeg[2]=0;
		Station->Lat=atof(LatDeg) + (atof(Payload+2) / 60);
       
		if (NS == 'S')
			Station->Lat = -Station->Lat;
		else
			if (NS != 'N')
				return FALSE;

		memcpy(LonDeg,Payload + 9, 3);
		LonDeg[3]=0;
		Station->Lon = atof(LonDeg) + (atof(Payload+12) / 60);
       
		if (EW == 'W')
			Station->Lon = -Station->Lon;
		else
			if (EW != 'E')
				return FALSE;
	}

	SymChar -= '!';

	if (SymSet == '\\')
		SymChar += 96;

	Station->iconRow = SymChar >> 4;
	Station->iconCol = SymChar & 15;

	return TRUE;
}

VOID DecodeAPRSPayload(char * Payload, struct STATIONRECORD * Station)
{
	char * TimeStamp;
	char * ObjName;
	char ObjState;
	struct STATIONRECORD * Object;
	
	switch(*Payload)
	{
	case '`':
	case 0x27:					// '
	case 0x1c:
	case 0x1d:

		break;					// MIC-E

	case '$':					// NMEA
		break;

	case ';':					// Object

		ObjName = Payload + 1;
		ObjState = Payload[10];	// * Live, _Killed
		Payload[10] = 0;

		Object = FindStation(ObjName);

		TimeStamp = Payload + 11;

		DecodeLocationString(Payload + 18, Object);

		if (Object->Lat > 60.0)
			break;

		return;

	case '@':
	case '/':					// Timestamp, No Messaging

		TimeStamp = ++Payload;
		Payload += 6;

	case '=':
	case '!':

		Payload++;
	
		DecodeLocationString(Payload, Station);
		return;			

	default:
		return;
	}
}
