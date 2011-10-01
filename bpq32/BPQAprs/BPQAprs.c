#define _CRT_SECURE_NO_DEPRECATE 
#define _WIN32_WINNT 0x0501	


#define MARGIN 0

// standard includes

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h> 
#include "Commdlg.h"

#include <malloc.h>
#include <memory.h>

#include <setjmp.h>


#define M_PI       3.14159265358979323846


// application includes

#include "png.h"
#include "pngfile.h"
#include "resource.h"

#include "bpq32.h"
#include "ASMStrucs.h"
#define APRS
#include "Versions.h"
#include "GetVersion.h"
#define BPQICON 2
#include "BPQAPRS.h"

int CurrentPage=0;				// Page currently on show in tabbed Dialog

int PageCount;

int PortNum[33];		// Tab nunber to port

UINT UIPortMask = 0;
BOOL UIEnabled[33];
char * UIDigi[33];
char * UIDigiAX[33];		// ax.25 version of digistring
int UIDigiLen[33];			// Length of AX string

char UIDEST[33][11];		// Dest for Beacons

char AXDEST[33][7];

RECT Rect;

HKEY REGTREE = HKEY_LOCAL_MACHINE;		// Default

char Key[80];

int portmask=0xffff;
int mtxparam=1;
int mcomparam=1;

typedef struct tag_dlghdr {

HWND hwndTab; // tab control
HWND hwndDisplay; // current child dialog box
RECT rcDisplay; // display rectangle for the tab control


DLGTEMPLATE *apRes[33];

} DLGHDR;

// Station Name Font


const unsigned char ASCII[][5] = {
//const u08 ASCII[][5]  = {
  {0x00, 0x00, 0x00, 0x00, 0x00} // 20  
  ,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
  ,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
  ,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
  ,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
  ,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
  ,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
  ,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
  ,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
  ,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
  ,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
  ,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
  ,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
  ,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
  ,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
  ,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
  ,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
  ,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
  ,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
  ,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
  ,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
  ,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
  ,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
  ,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
  ,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
  ,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
  ,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
  ,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
  ,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
  ,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
  ,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
  ,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
  ,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
  ,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
  ,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
  ,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
  ,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
  ,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
  ,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
  ,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
  ,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
  ,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
  ,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
  ,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
  ,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
  ,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
  ,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
  ,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
  ,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
  ,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
  ,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
  ,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
  ,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
  ,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
  ,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
  ,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
  ,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
  ,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
  ,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
  ,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
  ,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c 
  ,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
  ,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
  ,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
  ,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
  ,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
  ,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
  ,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
  ,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
  ,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
  ,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
  ,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
  ,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
  ,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
  ,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j 
  ,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
  ,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
  ,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
  ,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
  ,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
  ,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
  ,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
  ,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
  ,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
  ,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
  ,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
  ,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
  ,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
  ,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
  ,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
  ,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
  ,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
  ,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
  ,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
  ,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e ~
  ,{0x78, 0x46, 0x41, 0x46, 0x78} // 7f DEL
};



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

BOOL ReloadMaps;

char OSMDir[MAX_PATH] = "C:\\OSMTiles";
char Symbols[MAX_PATH] ;

static png_color bkgColor = {127, 127, 127};
static BOOL bStretched = FALSE;
static BYTE *pDib = NULL;
static BYTE *pDiData = NULL;

static png_structp png_ptr = NULL;
static png_infop info_ptr = NULL;

// Image chunks are 256 rows of 3 * 256 bytes

// Read 8 * 8 files, and copy to a 2048 * 3 * 2048 array. The display scrolls over this area, and
// it is refreshed when window approaches the edge of the array.

UCHAR Image[2048 * 3 * 2048];

int SetBaseX = 0;				// Lowest Tiles in currently loaded set
int SetBaseY = 0;

int Zoom = 3;

static int cxWinSize, cyWinSize;
static int cxImgSize, cyImgSize;
static int cImgChannels;

int ScrollX;
int ScrollY;

int MapCentreX = 0;
int MapCentreY = 0;

int MouseX, MouseY;
int PopupX, PopupY;

#define	FEND	0xC0	// KISS CONTROL CODES 
#define	FESC	0xDB
#define	TFEND	0xDC
#define	TFESC	0xDD

HINSTANCE hInst; 
HWND hWnd, hPopupWnd, hSelWnd, hStatus, hwndDlg, hwndDisplay;

char szAppName[] = "BPQAPRS";
char szTitle[80]   = "BPQAPRS" ; // The title bar text

BOOL APRSISOpen = FALSE;

int StationCount=0;

struct STATIONRECORD ** StationRecords;

struct OSMQUEUE OSMQueue = {NULL,0,0,0};

int OSMQueueCount;

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
VOID Decode_MIC_E_Packet(char * Payload, struct STATIONRECORD * Station);
BOOL GetLocPixels(double Lat, double Lon, int * X, int * Y);
VOID ProcessRFFrame(char * buffer, int len);
DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName);
VOID WINAPI OnSelChanged(HWND hwndDlg);
VOID WINAPI OnChildDialogInit(HWND hwndDlg);

VOID OSMThread();
VOID RefreshTile(char * FN, int Zoom, int x, int y);

unsigned long _beginthread( void( *start_address )(), unsigned stack_size, void * arglist);

SOCKADDR_IN destaddr;

unsigned int ipaddr;

unsigned short port = 10091;
UCHAR BPQPort = 7;

char Host[] = "tile.openstreetmap.org";

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

	int X, Y;

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

		ReloadMaps = TRUE;
	}
	return TRUE;
}

BOOL CentrePositionToMouse(double Lat, double Lon)
{
	// Positions  specified location at the mouse

	int X, Y;

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

		ReloadMaps = TRUE;
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
	int retCode, disp;
	HKEY hKey=0;
	char Size[80];

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

	retCode = RegCreateKeyEx(REGTREE, Key, 0,  0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		retCode = RegSetValueEx(hKey, "PortMask", 0, REG_DWORD, (BYTE *)&portmask, 4);
		retCode = RegSetValueEx(hKey, "SetBaseX", 0, REG_DWORD, (BYTE *)&SetBaseX, 4);
		retCode = RegSetValueEx(hKey, "SetBaseY", 0, REG_DWORD, (BYTE *)&SetBaseY, 4);
		retCode = RegSetValueEx(hKey, "ScrollX", 0, REG_DWORD, (BYTE *)&ScrollX, 4);
		retCode = RegSetValueEx(hKey, "ScrollY", 0, REG_DWORD, (BYTE *)&ScrollY, 4);
		retCode = RegSetValueEx(hKey, "Zoom", 0, REG_DWORD, (BYTE *)&Zoom, 4);

		wsprintf(Size,"%d,%d,%d,%d",Rect.left,Rect.right,Rect.top,Rect.bottom);
		retCode = RegSetValueEx(hKey,"Size",0,REG_SZ,(BYTE *)&Size, strlen(Size));

		RegCloseKey(hKey);
	}

//	KillTimer(NULL, TimerHandle);

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

UCHAR Icons[100000];


char Test[] = "2E0AYY>APU25N,TCPIP*,qAC,AHUBSWE2:=5105.18N/00108.19E-Paul in Folkestone Kent {UIV32N}\r\n";

CRITICAL_SECTION Crit, OSMCrit;


HBRUSH bgBrush;

#define BGCOLOUR RGB(236,233,216)

	
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	int i, tempmask=0xffff;
	char msg[20];
	int retCode,Type,Vallen;
	HKEY hKey=0;
	char Size[80];

	BOOL bcopt=TRUE;
	u_long param=1;
	WSADATA WsaData;            // receives data from WSAStartup
	int ImgSizeX, ImgSizeY, ImgChannels;
	png_color bgColor;
	UCHAR * BPQDirectory = GetBPQDirectory();

	REGTREE = GetRegistryKey();
	MinimizetoTray = GetMinimizetoTrayFlag();

	// Get config from Registry 

	wsprintf(Key,"SOFTWARE\\G8BPQ\\BPQ32\\BPQAPRS");

	retCode = RegOpenKeyEx (REGTREE,
                              Key,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=4;
		retCode = RegQueryValueEx(hKey,"PortMask",0,			
			(ULONG *)&Type,(UCHAR *)&tempmask,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"Zoom",0,			
			(ULONG *)&Type,(UCHAR *)&Zoom,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"SetBaseX",0,			
			(ULONG *)&Type,(UCHAR *)&SetBaseX,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"SetBaseY",0,			
			(ULONG *)&Type,(UCHAR *)&SetBaseY,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"ScrollX",0,			
			(ULONG *)&Type,(UCHAR *)&ScrollX,(ULONG *)&Vallen);

		Vallen=4;
		retCode = RegQueryValueEx(hKey,"ScrollY",0,			
			(ULONG *)&Type,(UCHAR *)&ScrollY,(ULONG *)&Vallen);

		Vallen=80;
		retCode = RegQueryValueEx(hKey,"Size",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&Rect.left,&Rect.right,&Rect.top,&Rect.bottom);

		RegCloseKey(hKey);
	}


	if (BPQDirectory[0] == 0)
		wsprintf(OSMDir, "OSMTiles");
	else
		wsprintf(OSMDir,"%s\\OSMTiles", BPQDirectory);

	if (BPQDirectory[0] == 0)
		wsprintf(Symbols, "BPQAPRS/Symbols.png");
	else
		wsprintf(Symbols, "%s/BPQAPRS/Symbols.png", BPQDirectory);


	InitializeCriticalSection(&Crit); 
	InitializeCriticalSection(&OSMCrit); 
	
	LoadImageFile (NULL, Symbols, &iconImage, &ImgSizeX, &ImgSizeY, &ImgChannels, &bgColor);

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szAppName, szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 800, 600,
		NULL, NULL, hInstance, NULL);

	if (!hWnd) {
		return (FALSE);
	}

	if (Rect.right < 100 || Rect.bottom < 100)
	{
		GetWindowRect(hWnd,	&Rect);
	}

	MoveWindow(hWnd,Rect.left,Rect.top, Rect.right-Rect.left, Rect.bottom-Rect.top, TRUE);

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
	
	SetTimer(hWnd,1,10000,NULL);

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

;	hMenu=GetMenu(hWnd);
	hMenu=CreateMenu();
	hPopMenu1=CreatePopupMenu();
	hPopMenu2=CreatePopupMenu();
	hPopMenu3=CreatePopupMenu();
	SetMenu(hWnd,hMenu);


	AppendMenu(hMenu, MF_STRING + MF_POPUP, (UINT)hPopMenu1,"Ports");
	AppendMenu(hMenu, MF_STRING + MF_POPUP, (UINT)hPopMenu2,"Flags");
	AppendMenu(hMenu, MF_STRING + MF_POPUP, (UINT)hPopMenu3,"Help");

	for (i=1;i <= GetNumberofPorts();i++)
	{
		wsprintf(msg,"Port %d",i);
		AppendMenu(hPopMenu1,
			MF_STRING | MF_CHECKED,BPQBASE + i,msg);
	}

	
	AppendMenu(hPopMenu3, MF_STRING, IDM_ABOUT, "About");

	DrawMenuBar(hWnd);

//	hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
//		0, 0, 0, 0, hWnd, (HMENU)100, GetModuleHandle(NULL), NULL);

//	SetScrollRange(hWnd,SB_VERT,0,216,TRUE);

	if (MinimizetoTray)
	{
		//	Set up Tray ICON

		AddTrayMenuItem(hWnd, "BPQAPRS");
	}
	
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

//	ScrollX = MapCentreX;
//	ScrollY = MapCentreY;
	
//	SetBaseX = long2tilex(-6.3, Zoom);
//	SetBaseY = lat2tiley(58.5, Zoom);

//	SetBaseX = long2tilex(-7.0, Zoom) - 4;
//	SetBaseY = lat2tiley(59, Zoom) - 4;				// Set Location at middle

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

	ReloadMaps = TRUE;

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
	destaddr.sin_port = htons(80);

	_beginthread(ResolveThread,0,0);
	_beginthread(APRSISThread,0,0);
	_beginthread(OSMThread,0,0);

	return (TRUE);
}


INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
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

int Statwidths[] = {150, 300, 400, 800, 810, -1};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	HGLOBAL	hMem;
	int DeltaX, DeltaY;
	double MouseLat, MouseLon;
	char MouseLoc[80];

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
		case WM_CREATE:
	
			// Initialize common controls
			InitCommonControls();

			// Create status bar
			hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL, WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, 0, 0, 0, 0, hWnd, (HMENU)100, GetModuleHandle(NULL), NULL);

			// Create status bar "compartments" one width 150, other 300, then 400... last -1 means that it fills the rest of the window

			SendMessage(hStatus, SB_SETPARTS, (WPARAM)(sizeof(Statwidths)/sizeof(int)), (LPARAM)Statwidths);
			SendMessage((HWND) hStatus, (UINT) SB_SETTEXT, (WPARAM)(INT) 0 | 0, (LPARAM) (LPSTR) TEXT("Hello"));
		
			break;


		case WM_TIMER:

			if (APRSISOpen == FALSE)
				_beginthread(APRSISThread,0,0);

			break;
		
	
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

			ReloadMaps = TRUE;

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
			
			if (MouseX == LOWORD(lParam) || MouseY == HIWORD(lParam) + 20)
				return TRUE;		// Not Moved

			MouseX = LOWORD(lParam);
			MouseY = HIWORD(lParam) + 20;
			
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
						ReloadMaps = TRUE;
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
						ReloadMaps = TRUE;
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
						ReloadMaps = TRUE;
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
						ReloadMaps = TRUE;
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

			GetMouseLatLon(&MouseLat, &MouseLon);
			sprintf(MouseLoc, "%1.3f %1.3f", MouseLat, MouseLon);
			SendMessage(hStatus, SB_SETTEXT, (WPARAM)(INT) 0 | 0, (LPARAM)MouseLoc);

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

			// Auto-resize statusbar (Send WM_SIZE message does just that)

			SendMessage(hStatus, WM_SIZE, 0, 0);

			break;


		case WM_PAINT:

			if (ReloadMaps)
			{
				ReloadMaps = FALSE;
				LoadImageSet(Zoom, SetBaseX, SetBaseY);
			}

			hdc = BeginPaint (hWnd, &ps);

			RefreshStationList();
			
			DisplayImage (hWnd, &pDib, &pDiData, cxWinSize, cyWinSize,
                NULL, cxImgSize, cyImgSize, cImgChannels, bStretched);

			if (pDib)
				SetDIBitsToDevice (hdc, 0, 0, cxWinSize, cyWinSize - 20, 0, 0,
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

			GetWindowRect(hWnd,	&Rect);	// For save soutine

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

			if (lParam == (LPARAM)hSelWnd)
			{
				Debugprintf("%d", wmEvent);

				if (wmEvent == LBN_SELCHANGE)
				{
					int Index = SendMessage(hSelWnd, LB_GETCURSEL, 0, 0);
					char Key[20];

					if (Index != -1)
					{
						struct STATIONRECORD * Station;

				
						SendMessage(hSelWnd, LB_GETTEXT, Index, (LPARAM)Key); 
						Station = FindStation(Key);
								
						DestroyWindow(hSelWnd);
						hSelWnd = 0;

						hPopupWnd = CreateWindow("LISTBOX", "", WS_CHILD | WS_BORDER | LBS_NOTIFY,
							PopupX, PopupY, 450, 150, hWnd, NULL, hInst, NULL);
		
						SendMessage(hPopupWnd, LB_ADDSTRING, 0, (LPARAM)Station->Callsign);
						SendMessage(hPopupWnd, LB_ADDSTRING, 0, (LPARAM)Station->Path);
						SendMessage(hPopupWnd, LB_ADDSTRING, 0, (LPARAM)Station->LastPacket);
	
						ShowWindow(hPopupWnd, SW_SHOWNORMAL);

						return TRUE;
					}
				}
			}
		
			if (wmId > BPQBASE && wmId < BPQBASE + 33)
			{
				TogglePort(hWnd,wmId,0x1 << (wmId - (BPQBASE + 1)));
				break;
			}
		
			switch (wmId)
			{
		
			case IDM_ABOUT:

				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
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
	int len;
	unsigned char buffer[1024];

	// See if a NODES

	if (buff[21] == 3 && buff[22] == 0xcf && buff[23] == 0xff)
		return 0;

	len=DecodeFrame(buff, buffer, timestamp);

	if (strstr(buffer, "<UI") == 0)
		return 0;

	ProcessRFFrame(buffer, len);
								
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

VOID OSMGet(int x, int y, int zoom)
{
	struct OSMQUEUE * OSMRec = malloc(sizeof(struct OSMQUEUE));
	
	EnterCriticalSection(&OSMCrit);

	OSMQueueCount++;

	OSMRec->Next = OSMQueue.Next;
	OSMQueue.Next = OSMRec;
	OSMRec->x = x;
	OSMRec->y = y;
	OSMRec->Zoom = zoom;

	LeaveCriticalSection(&OSMCrit);

}

VOID OSMThread()
{
	// Request a page from OSM

	char FN[MAX_PATH];
	char Tile[80];
	struct OSMQUEUE * OSMRec;
	int Zoom, x, y;

	SOCKET sock;
	SOCKADDR_IN sinx; 
	int addrlen=sizeof(sinx);
	int err;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Request[100];

	UCHAR Buffer[100000];
	int Len, InputLen = 0;
	char * ptr;
	int inptr = 0;

	while (TRUE)
	{
	while (OSMQueue.Next)
	{
		EnterCriticalSection(&OSMCrit);

		OSMRec = OSMQueue.Next;
		OSMQueue.Next = OSMRec->Next;

		OSMQueueCount--;

		LeaveCriticalSection(&OSMCrit);

		x = OSMRec->x;
		y = OSMRec->y;
		Zoom = OSMRec->Zoom;

		free(OSMRec);

		wsprintf(Tile, "/%02d/%d/%d.png", Zoom, x, y);
		wsprintf(FN, "%s%s", OSMDir, Tile);


		Len = wsprintf(Request, "GET %s HTTP/1.0\r\n", Tile);

		Debugprintf(FN);

	//   Allocate a Socket entry

		sock=socket(AF_INET,SOCK_STREAM,0);

		if (sock == INVALID_SOCKET)
  		 	return; 
 
		setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);
/*
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
*/

		if (connect(sock,(LPSOCKADDR) &destaddr, sizeof(destaddr)) != 0)
		{
			err=WSAGetLastError();

			//
			//	Connect failed
			//

			break;
		}

//GET /15/15810/9778.png HTTP/1.0
//Accept: */*
//Host: tile.openstreetmap.org
//Connection: close
//Content-Length: 0
//User-Agent: APRSIS32(G8BPQ)

		InputLen = 0;
		inptr = 0;

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
								HANDLE Handle;

								Handle = CreateFile(FN, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
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
									
										char * Dir = _strdup(FN);
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

								Debugprintf("Tile %s Loaded", FN);
								RefreshTile(FN, Zoom, x, y);
								InvalidateRect(hWnd, NULL, FALSE);

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
	}

	// Queue is empty

	Sleep(1000);
}
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
	int InputLen = 1;		// Non-zero
	char errmsg[40];
	char * ptr;
	int inptr = 0;
	char APRSinMsg[1000];

	Debugprintf("APRS IS Thread");

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
//			MessageBox(NULL, errmsg, NULL, MB_OK);

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

	while (InputLen > 0)
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

				APRSMsg[len - 2] = 0;
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

	Debugprintf("APRS IS Thread Exited");

	return;
}

VOID LoadImageSet(int Zoom, int startx, int starty)
{
	char FN[100];
	int i, x, y;
	int StartRow;
	int StartCol;
	char Tile[100];
	UCHAR * pbImage = NULL;

	int Limit = (int)pow(2, Zoom);

	
	for (x = 0; x < 8; x++)
	{
		for (y = 0; y < 8; y++)
		{
			wsprintf(Tile, "/%02d/%d/%d.png", Zoom, startx + x, starty + y);

			if ((startx + x) >= Limit || (starty + y) >= Limit || startx + x < 0 || starty + y < 0)
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
				OSMGet(startx + x, starty + y, Zoom);

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

VOID RefreshTile(char * FN, int TileZoom, int Tilex, int Tiley)
{
	// Called when a new tile has been diwnloaded from OSM

	int StartRow, StartCol;
	UCHAR * pbImage = NULL;
	int x, y;

	if (TileZoom != Zoom)
		return;					// Zoom level has changed

	x = Tilex - SetBaseX;
	y = Tiley -  SetBaseY;

	if (x < 0 || x > 7 || y < 0 || y > 7)	
		return;					// Tile isn't part of current image;

	__try
	{				
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

	// Copy to Image Array 3 * 256 *250
	
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
	int X, Y, Pointer, i, c, index, bit, mask, calllen;
	UINT j;


//	if (ptr->Lat > 60)
//		return;

//	if (GetLocPixels(58.47583, -6.21151, &X, &Y))
	if (GetLocPixels(ptr->Lat, ptr->Lon, &X, &Y))
	{
		ptr->DispX = X;
		ptr->DispY = Y;					// Save for mouse over checks

		// X and Y are offsets into the pixel data in array Image. Actual Bytes are at Y * 2048 * 3 + (X * 3)

		// Draw Icon

		if (Y < 8) Y = 8;
		if (X < 8) X = 8;
		
		Pointer = ((Y -8) * 2048 * 3) + (X * 3) - 24;

		j = ptr->iconRow * 21 * 337 * 3 + ptr->iconCol * 21 * 3 + 9 + 337 * 9;
		for (i = 0; i < 16; i++)
		{
			memcpy(&Image[Pointer], &iconImage[j], 16 * 3);
			Pointer += 6144;
			j += 337 * 3;
		}

		calllen = strlen(ptr->Callsign) * 6 + 4;
	
		// Draw Callsign Box

		Pointer = ((Y - 7) * 2048 * 3) + (X * 3) + 30;

		for (j = 0; j < 13; j++)
		{
			Image[Pointer++] = 0;
			Image[Pointer + calllen*3] = 0;
			Image[Pointer++] = 0;
			Image[Pointer + calllen*3] = 0;
			Image[Pointer++] = 0;
			Image[Pointer + calllen*3] = 0;
			Pointer += 2047 * 3;
		}

		for (i = 0; i < calllen; i++)
		{
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
		}

		Pointer = ((Y - 7) * 2048 * 3) + (X * 3) + 30;

		for (i = 0; i < calllen; i++)
		{
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
		}

		// Draw Callsign. 

		// Font is 5 bits wide x 8 high. Each byte of font contains one column, so 5 bytes per char

		for (j = 0; j < strlen(ptr->Callsign); j++)
		{

		Pointer = ((Y - 5) * 2048 * 3) + (X * 3) + 36 + (j * 18);

		mask = 1;

		for (i = 0; i < 2; i++)
		{
			for (index = 0 ; index < 6 ; index++)
			{
				Image[Pointer++] = 255;				// Blank lines above chars between chars
				Image[Pointer++] = 255;
				Image[Pointer++] = 255;
			}
			Pointer += 2042 * 3;
		}

	//	Pointer = ((Y - 3) * 2048 * 3) + (X * 3) + 36 + (j * 18);

		for (i = 0; i < 7; i++)
		{
			Image[Pointer++] = 255;				// Blank col between chars
			Image[Pointer++] = 255;
			Image[Pointer++] = 255;

			for (index = 0 ; index < 5 ; index++)
			{
				c = ASCII[ptr->Callsign[j] - 0x20][index];	// Font data
				bit = c & mask;

				if (bit)
				{
					Image[Pointer++] = 0;
					Image[Pointer++] = 0;
					Image[Pointer++] = 0;
				}
				else
				{
					Image[Pointer++] = 255;
					Image[Pointer++] = 255;
					Image[Pointer++] = 255;
				}
			}
			mask <<= 1;
			Pointer += 2042 * 3;
		}
		
	//	Pointer = ((Y - 3) * 2048 * 3) + (X * 3) + 36 + (j * 18);

		mask = 1;

		for (i = 0; i < 2; i++)
		{
			for (index = 0 ; index < 6 ; index++)
			{
				Image[Pointer++] = 255;				// Blank lines below chars between chars
				Image[Pointer++] = 255;
				Image[Pointer++] = 255;
			}
			Pointer += 2042 * 3;
		}

	}
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

		if (hSelWnd)
		{
			DestroyWindow(hSelWnd);
			hSelWnd = 0;
		}
		return;

	}
	//	If only one, display info popup, else display selection popup 

	if (hPopupWnd || hSelWnd)
		return;						// Already on display


	if (j == 1)
	{

		hPopupWnd = CreateWindow("LISTBOX", "", WS_CHILD | WS_BORDER | LBS_NOTIFY,
			 MouseX - ScrollX, MouseY - ScrollY, 450, 150, hWnd, NULL, hInst, NULL);
		
		SendMessage(hPopupWnd, LB_ADDSTRING, 0, (LPARAM)List[0]->Callsign);
		SendMessage(hPopupWnd, LB_ADDSTRING, 0, (LPARAM)List[0]->Path);
		SendMessage(hPopupWnd, LB_ADDSTRING, 0, (LPARAM)List[0]->LastPacket);
	
		ShowWindow(hPopupWnd, SW_SHOWNORMAL);
	}
	else
	{
		PopupX = MouseX - ScrollX;
		PopupY = MouseY - ScrollY - 20;
		
		hSelWnd = CreateWindow("LISTBOX", "", WS_CHILD | WS_BORDER | LBS_NOTIFY,
			PopupX, PopupY, 150, 150, hWnd, NULL, hInst, NULL);

		for (; j > 0; j--)
		{	
			SendMessage(hSelWnd, LB_ADDSTRING, 0, (LPARAM)List[j-1]->Callsign);
		}
		ShowWindow(hSelWnd, SW_SHOWNORMAL);
	}
}

void RefreshStationList()
{
	int i, j=0;
	struct STATIONRECORD * ptr;
	char Msg[20];

	// Entered Every 5 secs

/*	if (DecayTimer++ > 11)				// 5 Sec timer to minutes
	{
		DecayTimer = 0;
		CheckAgeofTargets(MaxAge);
	}
*/

	wsprintf(Msg, "%d", StationCount);
	SendMessage(hStatus, SB_SETTEXT, (WPARAM)(INT) 0 | 1, (LPARAM)Msg);

	wsprintf(Msg, "%d", OSMQueueCount);
	SendMessage(hStatus, SB_SETTEXT, (WPARAM)(INT) 0 | 2, (LPARAM)Msg);

	for (i = 0; i < StationCount; i++)
	{
		EnterCriticalSection(&Crit);		// In case list is being realloced
	
		ptr=StationRecords[i];

		LeaveCriticalSection(&Crit);

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

VOID ProcessRFFrame(char * Msg, int len)
{
	char * Payload;
	char * Path = NULL;
	char * Comment = NULL;
	char * Callsign;
	char * ptr;
	struct STATIONRECORD * Station;

	Msg[len - 1] = 0;


	Debugprintf("%s", Msg);

	Msg += 10;				// Skip Timestamp
	
	Payload = strchr(Msg, ':');			// End of Address String

	if (Payload == NULL)
	{
		Debugprintf("Invalid Msg %s", Msg);
		return;
	}

	Payload++;

	if (*Payload != 0x0d)
		return;

	*Payload++ = 0;

	Callsign = Msg;

	Path = strchr(Msg, '>');

	if (Path == NULL)
	{
		Debugprintf("Invalid Meader %s", Msg);
		return;
	}

	*Path++ = 0;

	ptr = strchr(Path, ' ');

	if (ptr)
		*ptr = 0;

	// Look up station - create a new one if not found

	Station = FindStation(Callsign);
	
	strcpy(Station->Path, Path);
	strcpy(Station->LastPacket, Payload);

	DecodeAPRSPayload(Payload, Station);

//	DrawStation(Station);

//		UpdateStation(Callsign, Path, Comment, Lat, Lon, 0.0, 0.0, Symbol >> 4, Symbol & 15);


	// No APRS-like location. Probably a normal beacon

	InvalidateRect(hWnd, NULL, FALSE);

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
	
	Debugprintf("%s", Msg);
	
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

	if (strlen(Callsign) > 11)
	{
		Debugprintf("Invalid Meader %s", Msg);
		return;
	}

	Station = FindStation(Callsign);
	
	strcpy(Station->Path, Path);
	strcpy(Station->LastPacket, Payload);

	DecodeAPRSPayload(Payload, Station);

//	DrawStation(Station);


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

		NS = Payload[7] & 0xdf;		// Mask Lower Case Bit
		EW = Payload[17] & 0xdf;

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
	case 0x1d:					// MIC-E

		Decode_MIC_E_Packet(Payload, Station);
		return;

	case '$':					// NMEA
		break;

	case ';':					// Object

		ObjName = Payload + 1;
		ObjState = Payload[10];	// * Live, _Killed

		Payload[10] = 0;
		Object = FindStation(ObjName);
		Payload[10] = ObjState;

		strcpy(Object->Path, Station->Callsign);
		strcat(Object->Path, ">");
		strcat(Object->Path, Station->Path);

		strcpy(Object->LastPacket, Payload);

		TimeStamp = Payload + 11;

		DecodeLocationString(Payload + 18, Object);
		DrawStation(Object);

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

// Convert MIC-E Char to Lat Digit (offset by 0x30)
//				  0123456789      @ABCDEFGHIJKLMNOPQRSTUVWXYZ				
char MicELat[] = "0123456789???????0123456789  ???0123456789 " ;

char MicECode[]= "0000000000???????111111111110???22222222222" ;


VOID Decode_MIC_E_Packet(char * Payload, struct STATIONRECORD * Station)
{
	// Info is encoded in the Dest Addr (in Station->Path) as well as Payload. 
	// See APRS Spec for full details

	char Lat[10];		// DDMMHH
	char LatDeg[3];
	char * ptr;
	char c;
	int i, n;
	int LonDeg, LonMin;
	BOOL LonOffset = FALSE;
	char NS = 'S';
	char EW = 'E';
	UCHAR SymChar, SymSet;

	ptr = &Station->Path[0];

	for (i = 0; i < 6; i++)
	{
		n = (*(ptr++)) - 0x30;
		c = MicELat[n];

		if (c == '?')			// Illegal
			return;

		Lat[i] = c;

	}

	Lat[6] = 0;

	if (Station->Path[3] > 'O')
		NS = 'N';

	if (Station->Path[5] > 'O')
		EW = 'W';

	if (Station->Path[4] > 'O')
		LonOffset = TRUE;

	n = Payload[1] - 28;			// Lon Degrees

	if (LonOffset)
		n += 100;

	if (n > 179 && n < 190)
		n -= 80;
	else
	if (n > 189 && n < 200)
		n -= 190;

	LonDeg = n;

/*
	To decode the longitude degrees value:
1. subtract 28 from the d+28 value to obtain d.
2. if the longitude offset is +100 degrees, add 100 to d.
3. subtract 80 if 180 ˜ d ˜ 189
(i.e. the longitude is in the range 100–109 degrees).
4. or, subtract 190 if 190 ˜ d ˜ 199.
(i.e. the longitude is in the range 0–9 degrees).
*/

	n = Payload[2] - 28;			// Lon Mins

	if (n > 59)
		n -= 60;

	LonMin = n;

	n = Payload[3] - 28;			// Lon Mins/100;

//1. subtract 28 from the m+28 value to obtain m.
//2. subtract 60 if m ™ 60.
//(i.e. the longitude minutes is in the range 0–9).


	memcpy(LatDeg, Lat, 2);
	LatDeg[2]=0;
	
	Station->Lat=atof(LatDeg) + (atof(Lat+2) / 6000.0);
       
	if (NS == 'S')
		Station->Lat = -Station->Lat;

	Station->Lon = LonDeg + LonMin / 60.0 + n / 6000.0;
       
	if (EW == 'W')				// West
		Station->Lon = -Station->Lon;

	SymChar = Payload[7];			// Symbol
	SymSet = Payload[8];			// Symbol

	SymChar -= '!';

	if (SymSet == '\\')
		SymChar += 96;

	Station->iconRow = SymChar >> 4;
	Station->iconCol = SymChar & 15;

	return;

}



INT_PTR CALLBACK ChildDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	This processes messages from controls on the tab subpages
	int Command;

//	int retCode, disp;
//	char Key[80];
//	HKEY hKey;
//	BOOL OK;
//	OPENFILENAME ofn;
//	char Digis[100];

	int Port = PortNum[CurrentPage];

	switch (message)
	{
	case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
		case TCN_SELCHANGE:
			 OnSelChanged(hDlg);
				 return TRUE;
         // More cases on WM_NOTIFY switch.
		case NM_CHAR:
			return TRUE;
        }

       break;
	case WM_INITDIALOG:
		OnChildDialogInit( hDlg);
		return (INT_PTR)TRUE;

	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


	case WM_COMMAND:

		Command = LOWORD(wParam);

		if (Command == 2002)
			return TRUE;

		switch (Command)
		{
/*			case IDC_FILE:

			memset(&ofn, 0, sizeof (OPENFILENAME));
			ofn.lStructSize = sizeof (OPENFILENAME);
			ofn.hwndOwner = hDlg;
			ofn.lpstrFile = &FN[Port][0];
			ofn.nMaxFile = 250;
			ofn.lpstrTitle = "File to send as beacon";
			ofn.lpstrInitialDir = GetBPQDirectory();

			if (GetOpenFileName(&ofn))
				SetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0]);

			break;

*/
		case IDOK:

/*			GetDlgItemText(hDlg, IDC_UIDEST, &UIDEST[Port][0], 10);

			if (UIDigi[Port])
			{
				free(UIDigi[Port]);
				UIDigi[Port] = NULL;
			}

			if (UIDigiAX[Port])
			{
				free(UIDigiAX[Port]);
				UIDigiAX[Port] = NULL;
			}

			GetDlgItemText(hDlg, IDC_UIDIGIS, Digis, 99); 
		
			UIDigi[Port] = _strdup(Digis);
		
			GetDlgItemText(hDlg, IDC_FILENAME, &FN[Port][0], 255); 
			GetDlgItemText(hDlg, IDC_MESSAGE, &Message[Port][0], 1000); 
	
			Interval[Port] = GetDlgItemInt(hDlg, IDC_INTERVAL, &OK, FALSE); 

			MinCounter[Port] = Interval[Port];

			SendFromFile[Port] = IsDlgButtonChecked(hDlg, IDC_FROMFILE);

			wsprintf(Key, "SOFTWARE\\G8BPQ\\BPQ32\\UIUtil\\UIPort%d", PortNum[CurrentPage]);

			retCode = RegCreateKeyEx(REGTREE,
					Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);
	
			if (retCode == ERROR_SUCCESS)
			{
				retCode = RegSetValueEx(hKey, "UIDEST", 0, REG_SZ,(BYTE *)&UIDEST[Port][0], strlen(&UIDEST[Port][0]));
				retCode = RegSetValueEx(hKey, "FileName", 0, REG_SZ,(BYTE *)&FN[Port][0], strlen(&FN[Port][0]));
				retCode = RegSetValueEx(hKey, "Message", 0, REG_SZ,(BYTE *)&Message[Port][0], strlen(&Message[Port][0]));
				retCode = RegSetValueEx(hKey, "Interval", 0, REG_DWORD,(BYTE *)&Interval[Port], 4);
				retCode = RegSetValueEx(hKey, "SendFromFile", 0, REG_DWORD,(BYTE *)&SendFromFile[Port], 4);
				retCode = RegSetValueEx(hKey, "Enabled", 0, REG_DWORD,(BYTE *)&UIEnabled[Port], 4);
				retCode = RegSetValueEx(hKey, "Digis",0, REG_SZ, Digis, strlen(Digis));

				RegCloseKey(hKey);
			}

			SetupUI(Port);
*/
			return (INT_PTR)TRUE;


		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

/*		case ID_TEST:

			SendBeacon(Port);
			return TRUE;


*/

		}
		break;

	}	
	return (INT_PTR)FALSE;
}




VOID WINAPI OnTabbedDialogInit(HWND hDlg)
{
	DLGHDR *pHdr = (DLGHDR *) LocalAlloc(LPTR, sizeof(DLGHDR));
	DWORD dwDlgBase = GetDialogBaseUnits();
	int cxMargin = LOWORD(dwDlgBase) / 4;
	int cyMargin = HIWORD(dwDlgBase) / 8;

	TC_ITEM tie;
	RECT rcTab;

	int i, pos, tab = 0;
	INITCOMMONCONTROLSEX init;

	char PortNo[60];
	struct _EXTPORTDATA * PORTVEC;

	hwndDlg = hDlg;			// Save Window Handle

	// Save a pointer to the DLGHDR structure.

	SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pHdr);

	// Create the tab control.


	init.dwICC = ICC_STANDARD_CLASSES;
	init.dwSize=sizeof(init);
	i=InitCommonControlsEx(&init);

	pHdr->hwndTab = CreateWindow(WC_TABCONTROL, "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, 100, 100, hwndDlg, NULL, hInst, NULL);

	if (pHdr->hwndTab == NULL) {

	// handle error

	}

	// Add a tab for each of the child dialog boxes.

	tie.mask = TCIF_TEXT | TCIF_IMAGE;

	tie.iImage = -1;

	for (i = 1; i <= GetNumberofPorts(); i++)
	{
		// Only allow UI on ax.25 ports

		PORTVEC = (struct _EXTPORTDATA * )GetPortTableEntry(i);

		if (PORTVEC->PORTCONTROL.PORTTYPE == 16)		// EXTERNAL
			if (PORTVEC->PORTCONTROL.PROTOCOL == 10)	// Pactor/WINMOR
				continue;

		wsprintf(PortNo, "Port %2d", GetPortNumber(i));
		PortNum[tab] = GetPortNumber(i);

		tie.pszText = PortNo;
		TabCtrl_InsertItem(pHdr->hwndTab, tab, &tie);
	
		pHdr->apRes[tab++] = DoLockDlgRes("PORTPAGE");

	}

	PageCount = tab;

	// Determine the bounding rectangle for all child dialog boxes.

	SetRectEmpty(&rcTab);

	for (i = 0; i < PageCount; i++)
	{
		if (pHdr->apRes[i]->cx > rcTab.right)
			rcTab.right = pHdr->apRes[i]->cx;

		if (pHdr->apRes[i]->cy > rcTab.bottom)
			rcTab.bottom = pHdr->apRes[i]->cy;

	}

	MapDialogRect(hwndDlg, &rcTab);

//	rcTab.right = rcTab.right * LOWORD(dwDlgBase) / 4;

//	rcTab.bottom = rcTab.bottom * HIWORD(dwDlgBase) / 8;

	// Calculate how large to make the tab control, so

	// the display area can accomodate all the child dialog boxes.

	TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);

	OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top);

	// Calculate the display rectangle.

	CopyRect(&pHdr->rcDisplay, &rcTab);

	TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

	// Set the size and position of the tab control, buttons,

	// and dialog box.

	SetWindowPos(pHdr->hwndTab, NULL, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, SWP_NOZORDER);

	// Move the Buttons to bottom of page

	pos=rcTab.left+cxMargin;

	
	// Size the dialog box.

	SetWindowPos(hwndDlg, NULL, 0, 0, rcTab.right + cyMargin + 2 * GetSystemMetrics(SM_CXDLGFRAME),
		rcTab.bottom  + 2 * cyMargin + 2 * GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION),
		SWP_NOMOVE | SWP_NOZORDER);

	// Simulate selection of the first item.

	OnSelChanged(hwndDlg);

}

// DoLockDlgRes - loads and locks a dialog template resource.

// Returns a pointer to the locked resource.

// lpszResName - name of the resource

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName)
{
	HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);
	HGLOBAL hglb = LoadResource(hInst, hrsrc);

	return (DLGTEMPLATE *) LockResource(hglb);
}

//The following function processes the TCN_SELCHANGE notification message for the main dialog box. The function destroys the dialog box for the outgoing page, if any. Then it uses the CreateDialogIndirect function to create a modeless dialog box for the incoming page.

// OnSelChanged - processes the TCN_SELCHANGE notification.

// hwndDlg - handle of the parent dialog box

VOID WINAPI OnSelChanged(HWND hwndDlg)
{
	char PortDesc[40];
	int Port;

	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	CurrentPage = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Destroy the current child dialog box, if any.

	if (pHdr->hwndDisplay != NULL)

		DestroyWindow(pHdr->hwndDisplay);

	// Create the new child dialog box.

	pHdr->hwndDisplay = CreateDialogIndirect(hInst, pHdr->apRes[CurrentPage], hwndDlg, ChildDialogProc);

	hwndDisplay = pHdr->hwndDisplay;		// Save

	Port = PortNum[CurrentPage];
	// Fill in the controls

	GetPortDescription(PortNum[CurrentPage], PortDesc);

	SetDlgItemText(hwndDisplay, IDC_PORTNAME, PortDesc);

//	CheckDlgButton(hwndDisplay, IDC_FROMFILE, SendFromFile[Port]);

//	SetDlgItemInt(hwndDisplay, IDC_INTERVAL, Interval[Port], FALSE);

	SetDlgItemText(hwndDisplay, IDC_UIDEST, &UIDEST[Port][0]);
	SetDlgItemText(hwndDisplay, IDC_UIDIGIS, UIDigi[Port]);



//	SetDlgItemText(hwndDisplay, IDC_FILENAME, &FN[Port][0]);
//	SetDlgItemText(hwndDisplay, IDC_MESSAGE, &Message[Port][0]);

	ShowWindow(pHdr->hwndDisplay, SW_SHOWNORMAL);

}

//The following function processes the WM_INITDIALOG message for each of the child dialog boxes. You cannot specify the position of a dialog box created using the CreateDialogIndirect function. This function uses the SetWindowPos function to position the child dialog within the tab control's display area.

// OnChildDialogInit - Positions the child dialog box to fall

// within the display area of the tab control.

VOID WINAPI OnChildDialogInit(HWND hwndDlg)
{
	HWND hwndParent = GetParent(hwndDlg);
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndParent, GWL_USERDATA);

	SetWindowPos(hwndDlg, HWND_TOP, pHdr->rcDisplay.left, pHdr->rcDisplay.top, 0, 0, SWP_NOSIZE);
}

#ifdef ZZZZZZZZZ

/*
 * $Id: mic_e.cpp,v 1.11 2003/03/31 04:49:40 kg4ijb Exp $
 *
 * aprsd, Automatic Packet Reporting System Daemon
 * Copyright (C) 1997,2002 Dale A. Heatherington, WA4DSY
 * Copyright (C) 2001-2002 aprsd Dev Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Look at the README for more information on the program.
 */



#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>


#include "constant.h"
#include "mic_e.h"

using namespace std;

static char *msgname[] = {
   "EMERGENCY.",
   "PRIORITY..",
   "Special...",
   "Committed.",
   "Returning.",
   "In Service",
   "Enroute...",
   "Off duty..",
   "          ",
   "Custom-6..",
   "Custom-5..",
   "Custom-4..",
   "Custom-3..",
   "Custom-2..",
   "Custom-1..",
   "Custom-0.."
};

bool ConvertMicE = false; //Set TRUE causes Mic-E pkts to be converted
                          // to classic APRS pkts.


/* Latitude digit lookup table based on APRS Protocol Spec 1.0, page 42 */
static char lattb[] = "0123456789       0123456789     0123456789 " ;


/* if this function fails to convert the packet it returns ZERO
   and the user supplied length variables, l1 and l2 are zero.
   Otherwise it returns non-zero values. */

int fmt_mic_e(const u_char *t, /* tocall */
          const u_char *i,   /* info */
          const int l,    /* length of info */
          u_char *buf1,      /* output buffer */
          int *l1,     /* length of output buffer */
          u_char *buf2,      /* 2nd output buffer */
          int *l2,     /* length of 2nd output buffer */
          time_t tick)    /* received timestamp of the packet */
{
    u_int msg,sp,dc,se,spd,cse;
    char north, west,c;
    int lon1,lonDD,lonMM,lonHH;
    char *bp;

    enum {none=0, BETA, REV1} rev = none; /* mic_e revision level */
    int gps_valid = 0;
    char symtbl = '/', symbol = '$', adti = '!';
    int buf2_n = 0;
    char lat[6];
    int x;
    unsigned lx;
    char msgclass;
    int trash = 0;

    /* Try to avoid conversion of bogus packets   */

    if ( l > 255) 
        return 0; /* info field must be less than 256 bytes long */

    if (strlen((char*)t) != 6) 
        return 0;  /* AX25 dest addr must be 6 bytes long. */

    for (x = 0; x < 6; x++) {
        c = t[x];        /* search for invalid characters in the ax25 destination */
        if ((c < '0')                 /* Nothing below zero */
                || (c  > 'Z')          /* Nothing above 'Z' */
                || (c == 'M')          /* M,N,O not allowed */
                || (c == 'N')
                || (c == 'O'))
            trash |= 1 ;

        if ((c > '9') && (c < 'A')) 
            trash |= 2;               /* symbols :;<>?@ are invalid */
      
        if ((x >= 3) && (c >= 'A') && (c <= 'K'))  
            trash |= 4; /* A-K not allowed in last 3 bytes */
    }

    for (x = 1; x < 7; x++) {
        if ((i[x] < 0x1c) || (i[x] > 0x7f)) 
            trash |= 8;  // Range check the longitude and speed/course
    }

    //printf("Trash=%d %s %s \n",trash, t, i);   //Debug
    if (trash) 
        return 0;  /* exit without converting packet if it's malformed */

    *l1 = *l2 = 0;

    /* This switch statement processes the 1st information field byte, byte 0 */

    switch (i[0]) {       /* possible valid MIC-E flags */
        case 0x60:         /* ` Current GPS data (not used in Kenwood TM-D700) */
            gps_valid = 1;
            rev = REV1;
            if (l < 9) 
                return 0;  /* reject packet if less than 9 bytes */
         
            break;

        case 0x27:        /* ' Old GPS data or Current data in Kenwood TM-D700 */
            gps_valid = 0;
            rev = REV1;
            if (l < 9) 
                return 0;  /* reject packet if less than 9 bytes */
            
            break;

        case 0x1c:        /* Rev 0 beta units current GPS data */
            gps_valid = 1;
            rev = BETA;
            if (l < 8) 
                return 0;  /* reject packet if less than 8 bytes */
         
            break;

        case 0x1d:        /* Rev 0 beta units old GPS data */
            gps_valid = 0;
            rev = BETA;
            if (l < 8) 
                return 0;  /* reject packet if less than 8 bytes */
         
            break;

        default:  
            return 0;   /* Don't attempt to process a packet that's not a Mic-E type */
    }


    if ((i[9] == ']') || (i[9] == '>'))  
        adti = '=';  /* Kenwood with messaging detected */

    if (l >= 7 ) { /* <-- old beta units could have only 7 bytes, new units should have 9 or more bytes */
        msg = ((t[0] >= 'P')?4:0) | ((t[1] >= 'P')?2:0) | ((t[2] >= 'P')?1:0);  //Standard messages  0..7

        if (msg == 0) {
            msg = (((t[0] >= 'A') && (t[0] < 'L'))?0xC:0)        //Custom messages  9..15
               | (((t[1] >= 'A') && (t[1] < 'L'))?0xA:0)
               | (((t[2] >= 'A') && (t[2] < 'L'))?9:0);

        }
        msg &= 0xf;  /* make absolutly sure we don't exceed the array bounds */

        msgclass = (msg > 7) ? 'C':'M';   /* Message class is either standard=M or custom=C */

        /* Recover the latitude digits from the ax25 destination field */
        for (x=0;x<6;x++) {
            lx = t[x] - '0';   //subtract offset from raw latitude character
            if ((lx >= 0 ) && ( lx < strlen(lattb))) {
                lat[x] = lattb[lx]; /* Latitude output digits into lat[] array */
            } else 
                lat[x] = ' ';
        }

        if (lat[0] == '9') {
            if((lat[1] != '0')           //reject lat degrees > 90 degrees
                    || (lat[2] != '0')
                    || (lat[3] != '0')
                    || (lat[4] != '0')
                    || (lat[5] != '0')) 
                return 0;
        }

        if (lat[2] > '5') 
            return 0;      //reject lat minutes >= 60

        if ((lat[0] == ' ') || (lat[1] == ' '))
            return 0;   //Reject ambiguious degrees

        north = t[3] >= 'P' ? 'N':'S';
        west  = t[5] >= 'P' ? 'W':'E';
        lon1  = t[4] >= 'P' ;             /* +100 deg offset */

        /* Process info field bytes 1 through 6; the Longitude, speed and course bits */
        lonDD = i[1] - 28;
        lonMM = i[2] - 28;
        lonHH = i[3] - 28;

#ifdef DEBUG
        fprintf(stderr,"raw lon 0x%02x(%d) 0x%02x(%d) 0x%02x(%d) 0x%02x(%d)\n",
              i[1],i[1],i[2],i[2],i[3],i[3],i[4],i[4]);
        fprintf(stderr,"before: lonDD=%d, lonMM=%d, lonHH=%d lon1=0x%0x\n",lonDD,lonMM,lonHH,lon1);
#endif

        if (rev >= REV1) {
            if (lon1)         /* do this first? */
                lonDD += 100;
         
            if (180 <= lonDD && lonDD <= 189)
                lonDD -= 80;
         
            if (190 <= lonDD && lonDD <= 199)
                lonDD -= 190;
         
            if (lonMM >= 60)
                lonMM -= 60;
        }
#ifdef DEBUG
        fprintf(stderr,"after: lonDD=%d, lonMM=%d, lonHH=%d lon1=0x%0x\n",lonDD,lonMM,lonHH,lon1);
#endif
        sp = i[4] - 28;
        dc = i[5] - 28;
        se = i[6] - 28;
        buf2_n = 6;         /* keep track of where we are */
#ifdef DEBUG
        fprintf(stderr,"sp=%02x dc==%02x se=%02x\n",sp,dc,se);
#endif
        spd = sp*10+dc/10;
        cse = (dc%10)*100+se;
        if (rev >= REV1) {
            if (spd >= 800)
                spd -= 800;
            
            if (cse >= 400)
                cse -= 400;
        }

        if (cse > 360) 
            return 0; // Reject invalid course value
      
        if (spd > 999) 
            return 0; // reject invalid speed value

#ifdef DEBUG
        fprintf(stderr,"symbol=0x%02x, symtbl=0x%02x\n",i[7],i[8]);
#endif

        /* Process information field bytes 7 and 8 (Symbol Code and Sym Table ID) */

        symtbl = (l >= 8 && rev >= REV1)? i[8] : '/';
        /* rev1 bug workaround:  sends null symbol/table bytes */

        symbol = i[7];
        if (!isprint(symbol)) 
            return 0; /* Reject packet if invalid symbol code */

        if ((symtbl != '/' && symtbl != '\\')
                && !('0' <= symtbl && symtbl <= '9')
                && !('a' <= symtbl && symtbl <= 'j')
                && !('A' <= symtbl && symtbl <= 'Z')
                && (symtbl != '*' && symtbl != '!')) {
            
            //printf("Bad symtbl code = %c\n",symtbl);  //Debug
            return 0;       /* Reject packet if invalid symtbl code */
        }

        buf2_n = (rev == BETA)?8:9;
        if (l >= 10) {
            buf2_n = (rev == BETA)?8:9;
            
            switch (i[buf2_n]) {
                case 0x60:     /* two-channel telemetry (Hex data)*/
                    sprintf((char*)buf2,"T#MIC,%03d,%03d",
                        hex2i(i[buf2_n+1],i[buf2_n+2]),
                        hex2i(i[buf2_n+3],i[buf2_n+4]));
                    buf2_n += 5;
                    *l2 = strlen((char*)buf2);
                    break;

                case 0x27:     /* five-channel telemetry (Hex data)*/
                    sprintf((char*)buf2,"T#MIC,%03d,%03d,%03d,%03d,%03d",
                        hex2i(i[buf2_n+1],i[buf2_n+2]),
                        hex2i(i[buf2_n+3],i[buf2_n+4]),
                        hex2i(i[buf2_n+5],i[buf2_n+6]),
                        hex2i(i[buf2_n+7],i[buf2_n+8]),
                        hex2i(i[buf2_n+9],i[buf2_n+10]));
                    buf2_n += 11;
                    *l2 = strlen((char*)buf2);
                    break;

                case 0x1d:     /* five-channel telemetry (This works only if the input device supports raw binary data)*/
                    sprintf((char*)buf2,"T#MIC,%03d,%03d,%03d,%03d,%03d",
                        i[buf2_n+1],i[buf2_n+2],i[buf2_n+3],i[buf2_n+4],i[buf2_n+5]);
                    buf2_n += 6;
                    *l2 = strlen((char*)buf2);
                    break;

                default:
                    break;         /* no telemetry */
            } /* end switch */
        }

        /* This sprintf statement creates the output string */
        sprintf((char*)buf1,"%c%c%c%c%c.%c%c%c%c%03d%02d.%02d%c%c%03d/%03d/Mic-E/%c%d/%s",
            adti,
            lat[0],lat[1],lat[2],lat[3],lat[4],lat[5],
            north,symtbl,
            lonDD,lonMM,lonHH,
            west,
            symbol,
            cse,spd,
            msgclass,
            ~msg&0x7,
            msgname[msg]);

        bp = (char*)&buf1[(*l1 = strlen((char*)buf1))];
        if (buf2_n < l) {      /* additional comments/btext */
            *bp++ = ' ';      /* add a space */
            (*l1)++;
            memcpy(bp,&i[buf2_n],l-buf2_n);
            *l1 += l-buf2_n;
        }
    }

    // printf("buf2_n = %d l=%d  %s\n",buf2_n,l,buf1);
    return ((*l1>0)+(*l2>0));
}

/* The following is not used by aprsd */

/*
 * here's another APRS device that get's mistreated by some APRS
 * implementations:  A TNC2 running TheNet X1J4.
 *
 *          1         2
 * 1234567890123456789012
 * TheNet X-1J4  (RELAY)
 */
static u_char x1j4prefix[] = "TheNet X-1J4";
#define MINX1J4LEN 14		/* min length w/TheNet X1J4 prefix */
#define MAXX1J4LEN 22		/* max length w/(alias) */
int
fmt_x1j4(const u_char *t,  /* tocall */
         const u_char *i, /* info */
         const int l,     /* length of info */
         u_char *buf1,    /* output buffer */
         int *l1,      /* length of output buffer */
         u_char *buf2,    /* 2nd output buffer */
         int *l2,      /* length of 2nd output buffer */
         time_t tick)     /* received timestamp of the packet */
{
   const u_char *cp;
   *l1 = *l2 = 0;
   if (l < MINX1J4LEN || memcmp(i,x1j4prefix,sizeof(x1j4prefix)-1) != 0)
      return 0;
   /* it's a TheNet beacon: either has ()'s w/alias name or no ()'s
      and the btext starts after some white space! */
   cp = (u_char*)memchr(&i[MINX1J4LEN],')',MAXX1J4LEN-MINX1J4LEN);
   if (cp == NULL)    /* no parens (no alias call) */
      cp = &i[MINX1J4LEN];
   else
      cp++;         /* skip over the ) */
   while (cp <= &i[l-1] && *cp == ' ') cp++; /* skip blanks */
   memcpy(buf1,cp,*l1=&i[l-1]-cp);
   return 1;
}

/*------------------------------------------------------------------------*/

unsigned int hex2i(u_char a,u_char b)
{
    unsigned int r = 0;

    if (a >= '0' && a <= '9') {
        r = (a-'0') << 4;
    } else if (a >= 'A' && a <= 'F') {
        r = (a-'A'+10) << 4;
    }
    
    if (b >= '0' && b <= '9') {
        r += (b-'0');
    } else if (b >= 'A' && b <= 'F') {
        r += (b-'A'+10);
    }
    return r;
}
/*------------------------------END---------------------------------------------*/


#endif