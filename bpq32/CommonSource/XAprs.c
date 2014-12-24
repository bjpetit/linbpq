// OpenGLAPRS.cpp : Defines the entry point for the console application.
//


#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif	

#define LINBPQ

#include "compatbits.h"

#include "bpqaprs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <X11/Xlib.h>
#include <X11/X.h>
#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <setjmp.h>
#include </usr/include/jpeglib.h>

#include <unistd.h>
#include <sys/mman.h>

#define LIBCONFIG_STATIC
#include "libconfig.h"

#define VOID void
#define UCHAR unsigned char
#define BOOL int
#define BYTE unsigned char
#define UINT unsigned int
#define TRUE 1
#define FALSE 0

unsigned long _beginthread(void(*start_address)(), unsigned stack_size, VOID * arglist)
{
	pthread_t thread;

	if (pthread_create(&thread, NULL, (void * (*)(void *))start_address, (void*) arglist) != 0)
		perror("New Thread");
	else
		pthread_detach(thread);

	return thread;
}

int Sleep(int ms)
{
	usleep(ms * 1000);
	return 0;
}

struct OSMQUEUE OSMQueue = {NULL,0,0,0};

int OSMQueueCount = 0;

static int cxWinSize = 788, cyWinSize = 788;
static int cxImgSize = 768, cyImgSize = 768;
static int xBorder = 10, yBorder = 10;

static int cImgChannels = 3;
static int ImgChannels;

int Bytesperpixel = 4;

int ExpireTime = 120;
int TrackExpireTime = 1440;
BOOL SuppressNullPosn = FALSE;
BOOL DefaultNoTracks = FALSE;
BOOL LocalTime = TRUE;


Display * display;
Window root, win;
GC gc;
XImage * image, * popupimage;

int SetBaseX = 0;				// Lowest Tiles in currently loaded set
int SetBaseY = 0;

int TileX = 0;
int TileY = 0;	

int Zoom = 2;

int MaxZoom = 16;

int MapCentreX = 256;
int MapCentreY = 256;

int MouseX, MouseY;
int PopupX, PopupY;

double MouseLat, MouseLon;

BOOL NeedRefresh = FALSE;
int NeedRedraw = 0;

int ScrollX = 128;
int ScrollY = 128;

int WindowX = 100, WindowY = 100;			// Position of window on screen
int WindowWidth = 788;
int WindowHeight = 788;

BOOL popupActive = FALSE;
BOOL selActive = FALSE;

char OSMDir[256] = "BPQAPRS/OSMTiles";

struct STATIONRECORD ** StationRecords = NULL;
struct STATIONRECORD * ControlRecord;

int MaxStations = 5000;
int StationCount;

// Image chunks are 256 rows of 3 * 256 bytes

// Read 8 * 8 files, and copy to a 2048 * 3 * 2048 array. The display scrolls over this area, and
// it is refreshed when window approaches the edge of the array.

int WIDTH;
int HEIGHT;

int WIDTHTILES = 4;
int HEIGHTTILES = 4;

UCHAR * Image = NULL;
UCHAR * iconImage = NULL;
UCHAR * PopupImage = NULL;

BOOL ImageChanged = 0;

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




struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	char buffer[JMSG_LENGTH_MAX];
	/* Create the message */
	(*cinfo->err->format_message) (cinfo, buffer);

	/* Always display the message. */
	printf("JPEG Fatal Error");


	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

// Return coorinates in tiles.

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

int long2tilex(double lon, int z) 
{ 
	return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z))); 
}
 
int lat2tiley(double lat, int z)
{ 
	return (int)(floor((1.0 - log( tan(lat * M_PI/180.0) + 1.0 / cos(lat * M_PI/180.0)) / M_PI) / 2.0 * pow(2.0, z))); 
}


BOOL CentrePositionToMouse(double Lat, double Lon)
{
	// Positions  specified location at the mouse

	int X, Y;

	SetBaseX = long2tilex(Lon, Zoom) - 2;
	SetBaseY = lat2tiley(Lat, Zoom) - 2;				// Set Location at middle

	if (GetLocPixels(Lat, Lon, &X, &Y) == FALSE)
		return FALSE;							// Off map

	ScrollX = X - cxWinSize/2;
	ScrollY = Y - cyWinSize/2;


//	Map is now centered at loc cursor was at

//  Need to move by distance mouse is from centre

	// if ScrollX, Y are zero, the centre of the map corresponds to 1024, 1024
	
//	ScrollX -= 1024 - X;				// Posn to centre
//	ScrollY -= 1024 - Y;

	ScrollX += cxWinSize/2 - MouseX;
	ScrollY += cyWinSize/2 - MouseY;

	// May Need to move image

	while(ScrollX < 0)
	{
		SetBaseX--;
		ScrollX += 256;
	}

	while(ScrollY < 0)
	{
		SetBaseY--;
		ScrollY += 256;
	}

	while(ScrollX > 255)
	{
		SetBaseX++;
		ScrollX -= 256;
	}

	while(ScrollY > 255)
	{
		SetBaseY++;
		ScrollY -= 256;
	}
	
	return TRUE;
}

SOCKADDR_IN destaddr = {0};

unsigned int ipaddr = 0;

//char Host[] = "tile.openstreetmap.org";

//char Host[] = "oatile1.mqcdn.com";		//SAT
char Host[] = "otile1.mqcdn.com";

char HeaderTemplate[] = "Accept: */*\r\nHost: %s\r\nConnection: close\r\nContent-Length: 0\r\nUser-Agent: BPQ32(G8BPQ)\r\n\r\n";


VOID ResolveThread()
{
	struct hostent * HostEnt;
	int err;

//	while (TRUE)
	{
		// Resolve Name if needed

		HostEnt = gethostbyname(Host);
		 
		if (!HostEnt)
		{
			err = WSAGetLastError();
			printf("Resolve Failed for %s %d %x", Host, err, err);
		}
		else
		{
			memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);	
		}
//		Sleep(60 * 15 * 1000);
	}
}



VOID OSMGet(int x, int y, int zoom)
{
	struct OSMQUEUE * OSMRec = malloc(sizeof(struct OSMQUEUE));
	
	OSMQueueCount++;

	OSMRec->Next = OSMQueue.Next;
	OSMQueue.Next = OSMRec;
	OSMRec->x = x;
	OSMRec->y = y;
	OSMRec->Zoom = zoom;
}

VOID RefreshTile(char * FN, int TileZoom, int Tilex, int Tiley);

VOID OSMThread()
{
	// Request a page from OSM

	char FN[256];
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
	char Header[256];
	UCHAR Buffer[200000];
	int Len, InputLen = 0;
	UCHAR * ptr;
	int inptr = 0;
	struct stat STAT;
	FILE * Handle;

	destaddr.sin_family = AF_INET;
	destaddr.sin_port = htons(80);

	while (TRUE)
	{
	while (OSMQueue.Next)
	{
		OSMRec = OSMQueue.Next;
		OSMQueue.Next = OSMRec->Next;

		OSMQueueCount--;

		x = OSMRec->x;
		y = OSMRec->y;
		Zoom = OSMRec->Zoom;

		free(OSMRec);

//		wsprintf(Tile, "/%02d/%d/%d.png", Zoom, x, y);
//		wsprintf(Tile, "/tiles/1.0.0/sat/%02d/%d/%d.jpg", Zoom, x, y);
		sprintf(Tile, "/tiles/1.0.0/osm/%02d/%d/%d.jpg", Zoom, x, y);

		sprintf(FN, "%s/%02d/%d/%d.jpg", OSMDir, Zoom, x, y);

		if (stat(FN, &STAT) == 0)
		{
			printf(" File %s Exists - skipping\n", FN);
			continue;
		}

		printf("Getting %s\n", FN);

		Len = sprintf(Request, "GET %s HTTP/1.0\r\n", Tile);

	//   Allocate a Socket entry

		sock=socket(AF_INET,SOCK_STREAM,0);

		if (sock == INVALID_SOCKET)
  		 	return; 
 
		setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

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
		sprintf(Header, HeaderTemplate, Host);
		send(sock, Header, strlen(Header), 0);

		while (InputLen != -1)
		{
			InputLen = recv(sock, &Buffer[inptr], 200000 - inptr, 0);

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
							char Dir[256];

							if (FileLen == inptr - (ptr - Buffer))
							{
								// File is OK

								int cnt;
								
								Handle = fopen(FN, "wb");

								if (Handle)
								{
									fwrite(ptr, 1, FileLen, Handle);
									fclose(Handle);
									printf("Tile %s Loaded\n", FN);
									RefreshTile(FN, Zoom, x, y);
									break;
								}

								if (errno != 2)			// Bad Path
								{
									printf("Create %s failed %d\n", FN, errno);
									perror("fopen");
									break;
								}
	
								sprintf(Dir, "%s/%02d/%d", OSMDir, Zoom, x);		
									
								if (mkdir(Dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
								{
									printf("Error Creating %s\n", FN);
									perror("mkdir");
									break;
								}
								
								// Retry Create

								Handle = fopen(FN, "wb");

								if (Handle)
								{
									fwrite(ptr, 1, FileLen, Handle);
									fclose(Handle);

									printf("Tile %s Loaded\n", FN);
									RefreshTile(FN, Zoom, x, y);
									break;
								}

								printf("Create %s falled\n", FN);
								perror("fopen");
								break;
							}
						}
					}
				}
				printf("OSM GET Bad Response %s ", Buffer);
				sprintf(FN, "%s/DummyTile.jpg", OSMDir);
				RefreshTile(FN, Zoom, x, y);
		
				break;
			}
		}
		closesocket(sock);
	}

	// Queue is empty

	sleep(1);
}
}

double radians(double Degrees)
{
    return M_PI * Degrees / 180;
}
double degrees(double Radians)
{
	return Radians * 180 / M_PI;
}



double Distance(double laa, double loa)
{
	double lah = ControlRecord->Lat;
	double loh = ControlRecord->Lon;

/*

'Great Circle Calculations.

'dif = longitute home - longitute away


'      (this should be within -180 to +180 degrees)
'      (Hint: This number should be non-zero, programs should check for
'             this and make dif=0.0001 as a minimum)
'lah = latitude of home
'laa = latitude of away

'dis = ArcCOS(Sin(lah) * Sin(laa) + Cos(lah) * Cos(laa) * Cos(dif))
'distance = dis / 180 * pi * ERAD
'angle = ArcCOS((Sin(laa) - Sin(lah) * Cos(dis)) / (Cos(lah) * Sin(dis)))

'p1 = 3.1415926535: P2 = p1 / 180: Rem -- PI, Deg =>= Radians
*/

loh = radians(loh); lah = radians(lah);
loa = radians(loa); laa = radians(laa);

return 60*degrees(acos(sin(lah) * sin(laa) + cos(lah) * cos(laa) * cos(loa-loh))) * 1.15077945;

}

double Bearing(double lat2, double lon2)
{
	double lat1 = ControlRecord->Lat;
	double lon1 = ControlRecord->Lon;
	double dlat, dlon, TC1;

	lat1 = radians(lat1);
	lat2 = radians(lat2);
	lon1 = radians(lon1);
	lon2 = radians(lon2);

	dlat = lat2 - lat1;
	dlon = lon2 - lon1;

	if (dlat == 0 || dlon == 0) return 0;
	
	TC1 = atan((sin(lon1 - lon2) * cos(lat2)) / (cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(lon1 - lon2)));
	TC1 = degrees(TC1);
		
	if (fabs(TC1) > 89.5) if (dlon > 0) return 90; else return 270;

	if (dlat > 0)
	{
		if (dlon > 0) return -TC1;
		if (dlon < 0) return 360 - TC1;
		return 0;
	}

	if (dlat < 0)
	{
		if (dlon > 0) return TC1 = 180 - TC1;
		if (dlon < 0) return TC1 = 180 - TC1; // 'ok?
		return 180;
	}

	return 0;

}



VOID DecodeWXReport(struct APRSConnectionInfo * sockptr, char * WX)
{
	UCHAR * ptr = strchr(WX, '_');
	char Type;
	int Val;

	if (ptr == 0)
		return;

	sockptr->WindDirn = atoi(++ptr);
	ptr += 4;
	sockptr->WindSpeed = atoi(ptr);
	ptr += 3;
WXLoop:

	Type = *(ptr++);

	if (*ptr =='.')	// Missing Value
	{
		while (*ptr == '.')
			ptr++;

		goto WXLoop;
	}

	Val = atoi(ptr);

	switch (Type)
	{
	case 'c': // = wind direction (in degrees).	
		
		sockptr->WindDirn = Val;
		break;
	
	case 's': // = sustained one-minute wind speed (in mph).
	
		sockptr->WindSpeed = Val;
		break;
	
	case 'g': // = gust (peak wind speed in mph in the last 5 minutes).
	
		sockptr->WindGust = Val;
		break;

	case 't': // = temperature (in degrees Fahrenheit). Temperatures below zero are expressed as -01 to -99.
	
		sockptr->Temp = Val;
		break;

	case 'r': // = rainfall (in hundredths of an inch) in the last hour.
		
		sockptr->RainLastHour = Val;
		break;

	case 'p': // = rainfall (in hundredths of an inch) in the last 24 hours.

		sockptr->RainLastDay = Val;
		break;

	case 'P': // = rainfall (in hundredths of an inch) since midnight.

		sockptr->RainToday = Val;
		break;

	case 'h': // = humidity (in %. 00 = 100%).
	
		sockptr->Humidity = Val;
		break;

	case 'b': // = barometric pressure (in tenths of millibars/tenths of hPascal).

		sockptr->Pressure = Val;
		break;

	default:

		return;
	}
	while(isdigit(*ptr))
	{
		ptr++;
	}

	if (*ptr != ' ')
		goto WXLoop;
}


VOID CreateStationPopup(struct STATIONRECORD * ptr, int x, int y)
{
	char Msg[80];
	int Len = 130;
	int Line = 12;
	struct tm * TM;


//	CurrentPopup = ptr;
		
	if (LocalTime)
		TM = localtime(&ptr->TimeLastUpdated);
	else
		TM = gmtime(&ptr->TimeLastUpdated);

	Len = sprintf(Msg, "Last Heard: %.2d:%.2d:%.2d on Port %d",
		TM->tm_hour, TM->tm_min, TM->tm_sec, ptr->LastPort);

	XDrawImageString(display, win, gc, x + 2, y + Line, ptr->Callsign, strlen(ptr->Callsign));
	Line += 12;

	XDrawImageString(display, win, gc, x + 2, y + Line, ptr->Path, strlen(ptr->Path));
	Line += 12;

//	XDrawImageString(display, win, gc, x + 2, y + Line, ptr->Status, 40);
//	Line += 12;

	XDrawImageString(display, win, gc, x + 2, y + Line, Msg, Len);
	Line += 12;


//	Item.pszText = ptr->LastPacket;

	Len = sprintf(Msg, "Distance %6.1f Bearing %3.0f Course %1.0f Speed %3.1f",
		Distance(ptr->Lat, ptr->Lon),
		Bearing(ptr->Lat, ptr->Lon), ptr->Course, ptr->Speed);

	XDrawImageString(display, win, gc, x + 2, y + Line, Msg, Len);
	Line += 12;


	if (ptr->LastWXPacket[0])
	{
		//display wx info

		struct APRSConnectionInfo temp;

		memset(&temp, 0, sizeof(temp));

		DecodeWXReport(&temp, ptr->LastWXPacket);

		Len = sprintf(Msg, "Wind Speed %d MPH", temp.WindSpeed);
		XDrawImageString(display, win, gc, x + 2, y + Line, Msg, Len);
		Line += 12;

		Len = sprintf(Msg, "Wind Gust %d MPH", temp.WindGust);
		XDrawImageString(display, win, gc, x + 2, y + Line, Msg, Len);
		Line += 12;

		Len = sprintf(Msg, "Wind Direction %d°", temp.WindDirn);
		XDrawImageString(display, win, gc, x + 2, y + Line, Msg, Len);
		Line += 12;

		Len = sprintf(Msg, "Temperature %d°F", temp.Temp);
		XDrawImageString(display, win, gc, x + 2, y + Line, Msg, Len);
		Line += 12;

		Len = sprintf(Msg, "Pressure %05.1f", temp.Pressure /10.0);
		XDrawImageString(display, win, gc, x + 2, y + Line, Msg, Len);
		Line += 12;

		Len = sprintf(Msg, "Humidity %d%%", temp.Humidity);
		XDrawImageString(display, win, gc, x + 2, y + Line, Msg, Len);
		Line += 12;
	}

/*
<td>Rain last hour</td><td>##RAIN_HOUR_IN##"</td></tr>
<tr><td>Rain today</td><td>##RAIN_TODAY_IN##"</td></tr>
<tr><td>Rain last 24 hours</td><td>##RAIN_24_IN##"</td></tr>
</table>
*/

}


VOID FindStationsByPixel(int MouseX, int MouseY)
{
	int j=0;

	struct STATIONRECORD * ptr = *StationRecords;
	struct STATIONRECORD * List[1000];

	while(ptr && j < 999)
	{	
		if (abs((ptr->DispX - MouseX)) < 4 && abs((ptr->DispY - MouseY)) < 4)
			List[j++] = ptr;

		ptr = ptr->Next;
	}

	if (j == 0)
	{
		if (popupActive)
		{
//			DestroyWindow(popupActive);
			XPutImage (display, win, gc, image, ScrollX, ScrollY, 10, 10, cxImgSize, cyImgSize);
			DrawTracks(TRUE);

			popupActive = 0;
		}

		if (selActive)
		{
//			DestroyWindow(selActive);
			selActive = 0;
		}		
		return;
	}

	//	If only one, display info popup, else display selection popup 

	if (popupActive || selActive)
		return;						// Already on display


//	if (j == 1)
	{
		int PopupLeft = MouseX - ScrollX - 10;
		int PopupTop = MouseY - ScrollY - 30;

		if (PopupLeft + 400 > cxWinSize)
			PopupLeft = cxWinSize - 405;

		if (PopupTop + 150> cyWinSize)
			PopupTop= cyWinSize - 165;

		popupActive = TRUE;

		XClearArea(display, win, PopupLeft, PopupTop, 350, 200, FALSE);
		XDrawRectangle(display, win, gc, PopupLeft, PopupTop, 350, 200);
 		CreateStationPopup(List[0], PopupLeft, PopupTop);
	}
/*
	else
	{
		PopupX = MouseX - ScrollX - 10;
		PopupY = MouseY - ScrollY - 30;

		if (PopupX + 150 > cxWinSize)
			PopupX = cxWinSize - 155;

		if (PopupY + 150 > cyWinSize)
			PopupY = cyWinSize - 155;
		
		selActive = CreateWindow("LISTBOX", "", WS_CHILD | WS_BORDER | WS_VSCROLL |
			WS_HSCROLL | LBS_NOTIFY,
			PopupX, PopupY, 150, 150, hMapWnd, NULL, hInst, NULL);

		for (; j > 0; j--)
		{	
			SendMessage(selActive, LB_ADDSTRING, 0, (LPARAM)List[j-1]->Callsign);
		}
		ShowWindow(selActive, SW_SHOWNORMAL);

		PopupX = MouseX - ScrollX - 10;

		if (PopupX + 400 > cxWinSize)
			PopupX = cxWinSize - 405;
	}
*/

}


int DrawTrack(struct STATIONRECORD * ptr, BOOL AllStations)
{
	int X, Y, Pointer, i, c, index, bit, mask, calllen;
	UINT j;
	char Overlay;
	char * nptr;
	time_t AgeLimit = time(NULL ) - (TrackExpireTime * 60);
	int SavePointer;

//	if (ptr->Moved == 0 && AllStations == 0)
//		return 0;				// No need to repaint

	if (SuppressNullPosn && ptr->Lat == 0.0)
		return 0;

	if (ptr->ObjState == '_')	// Killed Object
		return 0;

	if (GetLocPixels(ptr->Lat, ptr->Lon, &X, &Y))
	{
		if (X < 12 || Y < 12 || X > (WIDTH - 36) || Y > (HEIGHT - 36))
			return 0;				// Too close to edges

		ptr->Moved = 0;

		if (ptr->LatTrack[0] && ptr->NoTracks == FALSE)
		{
			// Draw Track

			int Index = ptr->Trackptr;
			int i, n;
			int X, Y;
			int LastX = 0, LastY = 0;

			for (n = 0; n < TRACKPOINTS; n++)
			{
				if (ptr->LatTrack[Index] && ptr->TrackTime[Index] > AgeLimit)
				{
					if (GetLocPixels(ptr->LatTrack[Index], ptr->LonTrack[Index], &X, &Y))
					{
						X -= ScrollX;
						Y -= ScrollY;
						X += 10;
						Y += 10;
						
						if (LastX)
						{
							if (abs(X - LastX) < 600 && abs(Y - LastY) < 600)
								XDrawLine(display, win, gc, LastX, LastY, X, Y);
						}

						LastX = X;
						LastY = Y;
					}
				}
				Index++;
				if (Index == TRACKPOINTS)
					Index = 0;
		
			}
			XFlush(display);
		}
		return 1;
	}
	return 0;
}

int DrawStation(struct STATIONRECORD * ptr, BOOL AllStations)
{
	int X, Y, Pointer, i, c, index, bit, mask, calllen, calllenpixels;
	UINT j;
	char Overlay;
	char * nptr;
	time_t AgeLimit = time(NULL ) - (TrackExpireTime * 60);
	int SavePointer;

	if (ptr->Moved == 0 && AllStations == 0)
		return 0;				// No need to repaint

	if (SuppressNullPosn && ptr->Lat == 0.0)
		return 0;

	if (ptr->ObjState == '_')	// Killed Object
		return 0;

	if (GetLocPixels(ptr->Lat, ptr->Lon, &X, &Y))
	{
		if (X < 12 || Y < 12 || X > (WIDTH - 36) || Y > (HEIGHT - 36))
			return 0;				// Too close to edges

		ptr->Moved = 0;

		ptr->DispX = X;
		ptr->DispY = Y;					// Save for mouse over checks

		// X and Y are offsets into the pixel data in array Image. Actual Bytes are at Y * 2048 * 3 + (X * 3)

		// Draw Icon

		if (Y < 8) Y = 8;
		if (X < 8) X = 8;
		
		nptr = &Image[(((Y - 8) * WIDTH) + X - 8) * Bytesperpixel]; // Center icon on station

		j =  (ptr->iconRow * 21 * 337 * Bytesperpixel)
			+ (ptr->iconCol * 21 * Bytesperpixel) 
			+ 3 * Bytesperpixel + (337 * 3 * Bytesperpixel);
	
		for (i = 0; i < 16; i++)
		{
			memcpy(nptr, &iconImage[j], 16 * Bytesperpixel);
			nptr += WIDTH * Bytesperpixel;
			j += 337 * Bytesperpixel;
		}

		// If an overlay is specified, add it

		Overlay = ptr->IconOverlay;

		if (Overlay)
		{
			Pointer = (((Y - 4) * WIDTH) + (X - 3)) * Bytesperpixel;
			mask = 1;

			for (index = 0 ; index < 7 ; index++)
			{
				Image[Pointer++] = 255;				// Blank line above chars 
				Image[Pointer++] = 255;
				if (Bytesperpixel == 4)
				{
					Image[Pointer++] = 255;
					Pointer++;
				}
			}
			Pointer += (WIDTH - 7) * Bytesperpixel;

			for (i = 0; i < 7; i++)
			{
				Image[Pointer++] = 255;				// Blank col 
				Image[Pointer++] = 255;
				if (Bytesperpixel == 4)
				{
					Image[Pointer++] = 255;
					Pointer++;
				}

				for (index = 0 ; index < 5 ; index++)
				{
					c = ASCII[Overlay - 0x20][index];	// Font data
					bit = c & mask;


				if (bit)
				{
					Image[Pointer++] = 0;
					Image[Pointer++] = 0;
					if (Bytesperpixel == 4)
					{
						Image[Pointer++] = 0;
						Pointer++;
					}
				}
				else
				{
					Image[Pointer++] = 255;
					Image[Pointer++] = 255;
					if (Bytesperpixel == 4)
					{
						Image[Pointer++] = 255;
						Pointer++;
					}
				}
				}

				Image[Pointer++] = 255;				// Blank col 
				Image[Pointer++] = 255;
				if (Bytesperpixel == 4)
				{
					Image[Pointer++] = 255;
					Pointer++;
				}

				mask <<= 1;
				Pointer += (WIDTH - 7) * Bytesperpixel;
			}
			for (index = 0 ; index < 7 ; index++)
			{
				Image[Pointer++] = 255;				// Blank line below chars 
				Image[Pointer++] = 255;
				if (Bytesperpixel == 4)
				{
					Image[Pointer++] = 255;
					Pointer++;
				}
			}
			Pointer += (WIDTH - 6) * Bytesperpixel;
		}
		
		calllen = strlen(ptr->Callsign);

		while (calllen && ptr->Callsign[calllen - 1] == ' ')		// Remove trailing spaces
			calllen--;

		calllenpixels = (calllen + 1) * 6;

		// Draw Callsign Box

		Pointer = ((Y - 7) * WIDTH * Bytesperpixel) + ((X + 9) * Bytesperpixel);

		// Draw | at each end

		for (j = 0; j < 13; j++)
		{
			Image[Pointer] = 0;
			Image[Pointer++ + calllenpixels * Bytesperpixel] = 0;
			Image[Pointer] = 0;
			Image[Pointer++ + calllenpixels * Bytesperpixel] = 0;
			if (Bytesperpixel == 4)
			{
				Image[Pointer] = 0;
				Image[Pointer++ + calllenpixels * Bytesperpixel] = 0;
				Pointer++;
			}
			Pointer += (WIDTH - 1) * Bytesperpixel;
		}

		// Draw Top Line

		for (i = 0; i < calllenpixels; i++)
		{
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
			if (Bytesperpixel == 4)
			{
				Image[Pointer++] = 0;
				Pointer++;
			}
		}

		// Draw Bottom Line

		Pointer = ((Y - 7) * WIDTH * Bytesperpixel) + ((X + 9) * Bytesperpixel);

		for (i = 0; i < calllenpixels; i++)
		{
			Image[Pointer++] = 0;
			Image[Pointer++] = 0;
			if (Bytesperpixel == 4)
			{
				Image[Pointer++] = 0;
				Pointer++;
			}
		}

		// Draw Callsign. 

		// Font is 5 bits wide x 8 high. Each byte of font contains one column, so 5 bytes per char

		for (j = 0; j < calllen; j++)
		{

		Pointer = ((Y - 5) * WIDTH * Bytesperpixel) + ((X + 11) * Bytesperpixel) + (j * 6 * Bytesperpixel);

		mask = 1;

		for (i = 0; i < 2; i++)
		{
			for (index = 0 ; index < 6 ; index++)
			{
				Image[Pointer++] = 255;				// Blank lines above chars 
				Image[Pointer++] = 255;
				if (Bytesperpixel == 4)
				{
					Image[Pointer++] = 255;
					Pointer++;
				}
			}

			Pointer += (WIDTH - 6) * Bytesperpixel;
		}

	//	Pointer = ((Y - 3) * 2048 * 3) + (X * 3) + 36 + (j * 18);

		for (i = 0; i < 7; i++)
		{
			Image[Pointer++] = 255;				// Blank col between chars
			Image[Pointer++] = 255;
			if (Bytesperpixel == 4)
			{
				Image[Pointer++] = 255;
				Pointer++;
			}
			for (index = 0 ; index < 5 ; index++)
			{
				c = ASCII[ptr->Callsign[j] - 0x20][index];	// Font data
				bit = c & mask;

				if (bit)
				{
					Image[Pointer++] = 0;
					Image[Pointer++] = 0;
					if (Bytesperpixel == 4)
					{
						Image[Pointer++] = 0;
						Pointer++;
					}
				}
				else
				{
					Image[Pointer++] = 255;
					Image[Pointer++] = 255;
					if (Bytesperpixel == 4)
					{
						Image[Pointer++] = 255;
						Pointer++;
					}
				}
			}
			mask <<= 1;
			Pointer += (WIDTH - 6) * Bytesperpixel;
		}
		
	//	Pointer = ((Y - 3) * 2048 * 3) + (X * 3) + 36 + (j * 18);

		mask = 1;

		for (i = 0; i < 2; i++)
		{
			for (index = 0 ; index < 6 ; index++)
			{
				Image[Pointer++] = 255;				// Blank lines below chars between chars
				Image[Pointer++] = 255;
				if (Bytesperpixel == 4)
				{
					Image[Pointer++] = 255;
					Pointer++;
				}
			}
			Pointer += (WIDTH - 6) * Bytesperpixel;
		}
	}
		ImageChanged = TRUE;
		return 1;
	}
	else
	{
		ptr->DispX = 0;
		ptr->DispY = 0;			// Off Screen
	}
	return 0;
}


int DrawTracks(BOOL AllStations)
{
	struct STATIONRECORD * ptr = *StationRecords;
	int blackColor = BlackPixel(display, DefaultScreen(display));
	int whiteColor = WhitePixel(display, DefaultScreen(display));
	int Changed = 0;

	int i = 0;

	while (ptr)
	{
		Changed += DrawTrack(ptr, AllStations);
		i++;
		ptr = ptr->Next;
	}

//	NeedRefresh = FALSE;
//	LastRefresh = time(NULL);

//	if (RecsDeleted)
//		RefreshStationList();
	return Changed;
}




int RefreshStationMap(BOOL AllStations)
{
	struct STATIONRECORD * ptr = *StationRecords;
	int blackColor = BlackPixel(display, DefaultScreen(display));
	int whiteColor = WhitePixel(display, DefaultScreen(display));
	int Changed = 0;

	int i = 0;

	while (ptr)
	{
		Changed += DrawStation(ptr, AllStations);
		i++;
		ptr = ptr->Next;
	}

//	NeedRefresh = FALSE;
//	LastRefresh = time(NULL);

//	if (RecsDeleted)
//		RefreshStationList();

	StationCount = i;
	return Changed;
}



void j_putRGBScanline(BYTE *jpegline, 
					 int widthPix,
					 unsigned char *outBuf,
					 int row, int XOffset, int YOffset)
{
	// Offsets are in tiles, not pixels
	
	int offset = row * WIDTH * Bytesperpixel;	//widthPix
	int count;
	unsigned int val;
	
	offset += XOffset * 256 * Bytesperpixel;
	offset += YOffset * 256 * WIDTH * Bytesperpixel;
	
	for (count = 0; count < 256; count++) 
	{
		if (Bytesperpixel == 2)
		{
			val = (*(jpegline + count * 3 + 2) >> 3);
			val |= ((*(jpegline + count * 3 + 1) >> 2) << 5);
			val |= ((*(jpegline + count * 3 + 0) >> 3) << 11);
			*(outBuf + offset++) = (val & 0xff);
			*(outBuf + offset++) = (unsigned char)(val >> 8);
		}
		else
		{
			*(outBuf + offset++) = *(jpegline + count * 3 + 2);		// Blue
			*(outBuf + offset++) = *(jpegline + count * 3 + 1);		// Green
			*(outBuf + offset++) = *(jpegline + count * 3 + 0);		// Red	
			offset++;
		}
	}
}

//
//	stash a gray scanline
//

void j_putGrayScanlineToRGB(BYTE *jpegline, 
							 int widthPix,
							 BYTE *outBuf,
							 int row)
{
	int offset = row * widthPix * 3;
	int count;
	for (count=0;count<widthPix;count++) {

		BYTE iGray;

		// get our grayscale value
		iGray = *(jpegline + count);

		*(outBuf + offset + count * 3 + 0) = iGray;
		*(outBuf + offset + count * 3 + 1) = iGray;
		*(outBuf + offset + count * 3 + 2) = iGray;
	}
}


//
//	read a JPEG file
//

BYTE * JpegFileToRGB(char * fileName, UINT *width, UINT *height, int XOffset, int YOffset)
{
	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct my_error_mgr jmerr;
	struct jpeg_error_mgr jerr;
 
 
 /* More stuff */
	FILE * infile=NULL;		/* source file */

	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */
	char buf[250];
//	BYTE *dataBuf;

	// basic code from IJG Jpeg Code v6 example.c

	*width=0;
	*height=0;

	/* In this example we want to open the input file before doing anything else,
	* so that the setjmp() error recovery below can assume the file is open.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to read binary files.
	*/

	if ((infile = fopen(fileName, "rb")) == NULL) {
		return NULL;
	}
	

    cinfo.err = jpeg_std_error(&jerr);
 

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
//	cinfo.err = jpeg_std_error(&jerr.pub);
//	jerr.pub.error_exit = my_error_exit;


	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jmerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */

		jpeg_destroy_decompress(&cinfo);

		if (infile!=NULL)
			fclose(infile);
		return NULL;
	}
	

	/* Now we can initialize the JPEG decompression object. */

	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void) jpeg_read_header(&cinfo, TRUE);
	
	/* We can ignore the return value from jpeg_read_header since
	*   (a) suspension is not possible with the stdio data source, and
	*   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	* See libjpeg.doc for more info.
	*/

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	* jpeg_read_header(), so we do nothing here.
	*/

	/* Step 5: Start decompressor */

	(void) jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	* In this example, we need to make an output work buffer of the right size.
	*/ 

	// get our buffer set to hold data

	////////////////////////////////////////////////////////////
	// alloc and open our new buffer
	
//	dataBuf = malloc(cinfo.output_width * 4 * cinfo.output_height);
	
//	if (dataBuf==NULL) {
//
//		jpeg_destroy_decompress(&cinfo);
		
//		fclose(infile);

//		return NULL;
//	}

	// how big is this thing gonna be?
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */

		// asuumer all 3-components are RGBs
		if (cinfo.out_color_components==3) {
			
			j_putRGBScanline(buffer[0], 
								*width,
								Image,
								cinfo.output_scanline-1, XOffset, YOffset);

		} else if (cinfo.out_color_components==1) {

			// assume all single component images are grayscale
			j_putGrayScanlineToRGB(buffer[0], 
								*width,
								Image,
								cinfo.output_scanline-1);

		}

	}

	/* Step 7: Finish decompression */

	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	* Here we postpone it until after no more JPEG errors are possible,
	* so as to simplify the setjmp error logic above.  (Actually, I don't
	* think that jpeg_destroy can do an error exit, but why assume anything...)
	*/
	fclose(infile);

	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/

	return 0;
}
// store a scanline to our data buffer

void j_putRGBScanline(BYTE *jpegline, 
						 int widthPix,
						 BYTE *outBuf,
						 int row, int X, int Y);

void j_putGrayScanlineToRGB(BYTE *jpegline, 
						 int widthPix,
						 BYTE *outBuf,
						 int row);

VOID LoadImageTile(int Zoom, int startx, int starty, int x, int y);

VOID RefreshTile(char * FN, int TileZoom, int Tilex, int Tiley)
{
	// Called when a new tile has been diwnloaded from OSM

	int StartRow, StartCol;
	UCHAR * pbImage = NULL;
	int x, y, i, j;
	int ImgChannels;

	if (TileZoom != Zoom)
		return;					// Zoom level has changed

	x = Tilex - SetBaseX;
	y = Tiley - SetBaseY;

	if (x < 0 || x > WIDTHTILES -1 || y < 0 || y > HEIGHTTILES - 1)	
		return;					// Tile isn't part of current image;

	LoadImageTile (Zoom, Tilex, Tiley, x, y);
	NeedRedraw = 1;

//	XPutImage (display, win, gc, image, ScrollX, ScrollY, 10, 10, cxImgSize, cyImgSize);
}


VOID LoadImageTile(int Zoom, int startx, int starty, int x, int y)
{
	char FN[100];
	int i, j;
	int StartRow;
	int StartCol;
	char Tile[100];
	UCHAR * pbImage = NULL;
	int ImgChannels;
	BOOL JPG=FALSE;
	struct stat STAT;
	int cx, cy;

	int Limit = (int)pow(2, Zoom);
/*
	printf("LoadImage %d %d %d\n", Limit, startx, startx);

	if (startx < 0)
		startx = startx + WIDTHTILES;
	else
		if (startx > WIDTHTILES - 1)
			startx = WIDTHTILES - startx;

	if (starty < 0)
		starty = starty + HEIGHTTILES;
	else
		if (starty > HEIGHTTILES -1 )
			starty = HEIGHTTILES - starty;

	if (startx < 0 || startx > WIDTHTILES)
		x = WIDTHTILES / 2;

	if (starty < 0 || y > HEIGHTTILES)
		starty = HEIGHTTILES /2;
	
	printf("LoadImage %d %d %d\n", Limit, startx, starty);
*/
	if ((startx) >= Limit || (starty) >= Limit || startx< 0 || starty < 0)
	{
//		printf("Not Loading %d %d %d\n",Limit, startx, startx );
		return; //goto NoFile;
	}

	sprintf(Tile, "/%02d/%d/%d.jpg", Zoom, startx, starty);
	sprintf(FN, "%s%s", OSMDir, Tile);

	JPG = TRUE;

	if (stat(FN, &STAT))
	{
		OSMGet(startx, starty, Zoom);
		return;
	}

gotfile:
			
	JpegFileToRGB(FN, &cx, &cy, x, y);
	ImgChannels = 3;

}


VOID LoadImageSet(int Zoom, int TileX, int TileY)
{
	int x, y;

	if (SetBaseX != TileX || SetBaseY != TileY)
	{
		// Only Load if changed

		SetBaseX = TileX;				// Lowest Tiles in currently loaded set
		SetBaseY = TileY;

		memset(Image, 0, WIDTH * Bytesperpixel * HEIGHT);
		XClearWindow(display, win);

		for (y = 0; y < HEIGHTTILES; y++)
		{
			for (x = 0; x < WIDTHTILES; x++)
			{
				LoadImageTile(Zoom, TileX + x, TileY + y, x, y);
			}
		}
		RefreshStationMap(TRUE);
	}
	XPutImage (display, win, gc, image, ScrollX, ScrollY, 10, 10, cxImgSize, cyImgSize);
	DrawTracks(TRUE);

}

BYTE * ReadIcons(char * fileName, UINT *width, UINT *height)
{
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jmerr;
	struct jpeg_error_mgr jerr;
	FILE * infile=NULL;		/* source file */

	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */
	char buf[250];
	BYTE *dataBuf;

	*width=0;
	*height=0;

	if ((infile = fopen(fileName, "rb")) == NULL) {
		return NULL;
	}
	
    cinfo.err = jpeg_std_error(&jerr);
 
	if (setjmp(jmerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */

		jpeg_destroy_decompress(&cinfo);

		if (infile!=NULL)
			fclose(infile);
		return NULL;
	}
	

	jpeg_create_decompress(&cinfo);

	jpeg_stdio_src(&cinfo, infile);

	(void) jpeg_read_header(&cinfo, TRUE);
	
	(void) jpeg_start_decompress(&cinfo);
	
	dataBuf = malloc(cinfo.output_width * 4 * cinfo.output_height);
	memset(dataBuf, 0, cinfo.output_width * 4 * cinfo.output_height); 
	
	if (dataBuf==NULL) 
	{
		jpeg_destroy_decompress(&cinfo);	
		fclose(infile);
		return NULL;
	}

	// how big is this thing gonna be?
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	while (cinfo.output_scanline < cinfo.output_height)
	{
		int offset; 
		int count;
		unsigned int val;

		(void) jpeg_read_scanlines(&cinfo, buffer, 1);

		offset = (cinfo.output_scanline-1) * cinfo.output_width * Bytesperpixel;

		for (count = 0; count < cinfo.output_width; count++) 
		{
			if (Bytesperpixel == 2)
			{
				val = (*(buffer[0] + count * 3 + 2) >> 3);
				val |= ((*(buffer[0] + count * 3 + 1) >> 2) << 5);
				val |= ((*(buffer[0] + count * 3 + 0) >> 3) << 11);
				*(dataBuf + offset++) = (val & 0xff);
				*(dataBuf + offset++) = (unsigned char)(val >> 8);
			}
			else
			{
				*(dataBuf + offset++) = *(buffer[0] + count * 3 + 2);		// Blue
				*(dataBuf + offset++) = *(buffer[0] + count * 3 + 1);		// Green
				*(dataBuf + offset++) = *(buffer[0] + count * 3 + 0);		// Red	
				offset++;
			}
		}
	}

	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
	return dataBuf;
}
// store a scanline to our data buffer

void ZoomIn()
{
	if (Zoom < 16)
	{
		Zoom ++;
		CentrePositionToMouse(MouseLat, MouseLon);			
		TileX = SetBaseX;
		TileY = SetBaseY;
		NeedRefresh = TRUE;	
	}
}
ZoomOut()
{
	if (Zoom > 1)
	{
		Zoom --;		
		CentrePositionToMouse(MouseLat, MouseLon);	
		TileX = SetBaseX;
		TileY = SetBaseY;
		if (Zoom == 1)
			ScrollX = ScrollY = 0;

		NeedRefresh = TRUE;
	}
}

config_t cfg;
config_setting_t *croot, *group;

int GetIntValue(config_setting_t * group, char * name, int defaultval)
{
	config_setting_t *setting;

	setting = config_setting_get_member (group, name);
	if (setting)
		return config_setting_get_int (setting);

	return defaultval;
}

VOID SaveIntValue(config_setting_t * group, char * name, int value)
{
	config_setting_t *setting;
	
	setting = config_setting_add(group, name, CONFIG_TYPE_INT);
	if(setting)
		config_setting_set_int(setting, value);
}


int main(void)
{
    UCHAR * pbImage = NULL;
 	char FN[256];
	int fd;
	int x, y;
	BOOL Running = TRUE;

    double vals[10];	
	int screen_number, depth, bitmap_pad, status;
	unsigned long white;
	unsigned long black;
	Visual * visual;
	unsigned int i, j;
	Pixmap pixmap, popuppixmap;

	int x11_fd;
	fd_set in_fds;

	struct timeval tv;
	XEvent event;
	XConfigureEvent xce;
	XKeyEvent xkeyev;
	char text[256];
	int key;

	int LastX, LastY;			// Saved mouse position when button down
	int MovedX, MovedY;

	double sx, sy;
	UCHAR * APRSStationMemory;
	int SlowTimer = 0;
	Atom wmDeleteMessage;

	printf("G8BPQ APRS Client for Linux Version 0.0.0.3\n");
  	printf("Copyright © 2014 John Wiseman G8BPQ\n");
	printf("APRS is a registered trademark of Bob Bruninga.\n");
	printf("Mapping from OpenStreetMap (http://openstreetmap.org)\n");
	printf("Map Tiles Courtesy of MapQuest (www.mapquest.com)\n\n");

	config_init(&cfg);

	/* Read the file. If there is an error, report it and exit. */
	
	if(!config_read_file(&cfg, "BPQAPRS.cfg"))
	{
		fprintf(stderr, "%d - %s\n",
			config_error_line(&cfg), config_error_text(&cfg));
		config_destroy(&cfg);
	}
	else
	{
		group = config_lookup (&cfg, "APRS");

		if (group)
		{
			Zoom = GetIntValue(group, "Zoom", 2);
			TileX = GetIntValue(group, "SetBaseX", 0);
			TileY = GetIntValue(group, "SetBaseY", 0);
			ScrollX = GetIntValue(group, "ScrollX", 0);
			ScrollY = GetIntValue(group, "ScrollY", 0);
			WindowX = GetIntValue(group, "WindowX", 100);
			WindowY = GetIntValue(group, "WindowY", 100);
			WindowWidth = GetIntValue(group, "WindowWidth", 788);
			WindowHeight = GetIntValue(group, "WindowHeight", 788);
			HEIGHTTILES = GetIntValue(group, "HEIGHTTILES", 4);
			WIDTHTILES = GetIntValue(group, "WIDTHTILES", 4);
		}
	}

	if (Zoom == 0)
		Zoom = 2;

	HEIGHT = HEIGHTTILES * 256;
	WIDTH = WIDTHTILES * 256;
		
	printf("DISPLAY is set to %s\n", getenv("DISPLAY"));
	
	if (strstr(getenv("DISPLAY"), "localhost:1"))
		printf("!!! WARNING !!! X session seems to be tunneled over an SSH session\nThis progam will run much faster if you set DISPLAY to the Host running your X Server\n");

	// Get shared memory

	fd = shm_open("/BPQAPRSSharedMem", O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1)
	{
		printf("Open APRS Shared Memory Failed\n");
		return;
	}
	else
	{
		// Map shared memory object

		APRSStationMemory = mmap((void *)0x43000000, sizeof(struct STATIONRECORD) * 2,
		     PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

		if (APRSStationMemory == MAP_FAILED)
		{
			printf("Extend APRS Shared Memory Failed\n");
			APRSStationMemory = NULL;
			return;
		}
	}

	StationRecords = (struct STATIONRECORD**)APRSStationMemory;
	ControlRecord = (struct STATIONRECORD*)APRSStationMemory;

	MaxStations = ControlRecord->LastPort;
	printf("LinBPQ Configured with MaxStations %d\n", MaxStations);

	if (MaxStations == 0)
		MaxStations = 1500;			// for old LinBPQ

	//	Remap with Server's view of MaxStations
	
	munmap(APRSStationMemory, sizeof(struct STATIONRECORD) * 2);

	APRSStationMemory = mmap((void *)0x43000000, sizeof(struct STATIONRECORD) * (MaxStations + 1),
		PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

	if (APRSStationMemory == MAP_FAILED)
	{
		printf("Extend APRS Shared Memory Failed\n");
		APRSStationMemory = NULL;
	}

	ResolveThread();
	_beginthread(OSMThread, 0, NULL);

	display = XOpenDisplay(NULL);

	if (! display)
	{
		printf("Couldn't open X display\n");
		return 1;
	}

	screen_number = DefaultScreen (display);
	depth = DefaultDepth (display, screen_number);
	visual = DefaultVisual (display, screen_number);
	gc = DefaultGC (display, screen_number);
	bitmap_pad = BitmapPad (display);
	white = WhitePixel (display, screen_number);
	black = BlackPixel (display, screen_number);
	root = DefaultRootWindow (display);

	if (depth == 16)
		Bytesperpixel = 2;

	Image = malloc(WIDTH * Bytesperpixel * HEIGHT + 100);	// Seems past last byte gets corrupt

	if (mkdir(OSMDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
	{
		if (errno != 17)			// File exists
		{
			printf("Error Creating %s\n", OSMDir);
			perror("mkdir");
		}
	}

	for (i = 0; i < 20; i++)
	{
		sprintf(FN, "%s/%02d", OSMDir, i);
		if (mkdir(FN, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
		{
			if (errno != 17)			// FIle exists
			{
				printf("Error Creating %s\n", FN);
				perror("mkdir");
			}
		}
	}
	
	// Read Icons

	iconImage = ReadIcons("BPQAPRS/Symbols.jpg", &x, &y);

	if (x == 0)
		printf("Couldn't load Icons\n");

//	win = XCreateSimpleWindow (display, root, 50, 50, 800, 800, 0, black, white);
	win = XCreateWindow (display, root, 50, 50, 788, 788, 0, depth, CopyFromParent, CopyFromParent, 0, 0);
	XStoreName(display, win, "BPQAPRS Map");
	
	wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(display, win, &wmDeleteMessage, 1);

//	XSelectInput(display, win, KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask);

	XSelectInput(display, win, ExposureMask | KeyPressMask | PointerMotionMask |
		ButtonPressMask | ButtonReleaseMask | StructureNotifyMask);
	
	XSetWindowBackground(display, win, 0xffffff);
	XClearWindow(display, win);
	XMapWindow (display, win);

	XMoveResizeWindow(display, win, WindowX, WindowY, WindowWidth, WindowHeight);

	image = XCreateImage (display, visual, depth, ZPixmap, 0, NULL, WIDTH, HEIGHT, bitmap_pad, 0);

	printf("depth : %d\nbitmap_pad : %d\nimage bpp : %d\n", depth, bitmap_pad, image->bits_per_pixel);

	image->data = Image;

	pixmap = XCreatePixmap (display, root, WIDTH, HEIGHT, depth);

	XSetLineAttributes(display, gc, 2, LineSolid, CapNotLast, JoinMiter);

	SetBaseX = -1;			// force reload
	SetBaseY = -1;

	LoadImageSet(Zoom, TileX, TileY);	// Loads 1024 * 1024 Block
		
	x11_fd = ConnectionNumber(display);

    // Main loop

   while(Running)
   {
	   FD_ZERO(&in_fds);
	   FD_SET(x11_fd, &in_fds);

	   // Set our timer.  One second sounds good.

	   tv.tv_usec = 0;
	   tv.tv_sec = 3;

	   // Wait for X Event or a Timer
	   
	   if (select(x11_fd+1, &in_fds, 0, 0, &tv))
	   {
		   // X event, but pick up later
	   }
	   else
	   {
			// Handle timer here
			
			NeedRedraw += RefreshStationMap(FALSE);			// Draw new or moved stations

			// Do a full redraw at least evey 2 mins if anything has changed

			SlowTimer++;
			if (SlowTimer > 40)				// 2 Mins
				if (NeedRedraw)
					NeedRefresh = TRUE;
	   }

        // Handle XEvents and flush the input 

        while(XPending(display))
		{
		XNextEvent(display, &event);
		
		MouseX = event.xbutton.x - 10;
		MouseY = event.xbutton.y - 10;

		GetMouseLatLon(&MouseLat, &MouseLon);
		
		switch (event.type)
		{
			  
		case ClientMessage:
          
			if (event.xclient.data.l[0] == wmDeleteMessage)
                Running = FALSE;
            break;

		case KeyPress:

			xkeyev = event.xkey;

			XLookupString(&event.xkey, text, 255, &key, 0);

			printf("Key %c Hex %x Code %d %x\n", text[0], text[0], key, key);

			switch(key)
			{
			case XK_Left:

				WindowX -= 8;
				XMoveWindow(display, win, WindowX, WindowY);
				break;

			case XK_Up:

				WindowY -= 8;
				XMoveWindow(display, win, WindowX, WindowY);
				break;

			case XK_Right:

				WindowX += 8;
				XMoveWindow(display, win, WindowX, WindowY);
				break;

			case XK_Down:

				WindowY += 8;
				XMoveWindow(display, win, WindowX, WindowY);
				break;

			case '-':

				ZoomOut();
				break;

			case '=':
			case '+':

				ZoomIn();
				break;
			}

			break;

		case Expose:

			XPutImage (display, win, gc, image, ScrollX, ScrollY, 10, 10, cxImgSize, cyImgSize);
			DrawTracks(TRUE);
			break;

		case ConfigureNotify:

            xce = event.xconfigure;

            // This event type is generated for a variety of
            //  happenings, so check whether the window has been
            //   resized. 
				
			WindowX = xce.x;
			WindowY = xce.y;
			WindowWidth = xce.width;
			WindowHeight = xce.height;

			if (xce.width != cxWinSize || xce.height != cyWinSize)
			{
                cxWinSize = xce.width;
                cyWinSize = xce.height;
				cxImgSize = cxWinSize - xBorder * 2;
				cyImgSize = cyWinSize - yBorder * 2;

				XClearWindow(display, win);
				XPutImage (display, win, gc, image, ScrollX, ScrollY, 10, 10, cxImgSize, cyImgSize);
				DrawTracks(TRUE);
			}

			break;

		case MotionNotify:

			FindStationsByPixel(MouseX + ScrollX, MouseY + ScrollY);
			break;

			
		case ButtonPress:
			
			switch (event.xbutton.button)
			{
			case 1:					// Left Button
				
				LastX = MouseX;
				LastY = MouseY;
				break;
				
			case 4:					// Scrollup
			
				ZoomIn();
				break;
				
			case 5:					// Scrolldown
			
				ZoomOut();			
				break;
			}

			break;

		case ButtonRelease:

			switch (event.xbutton.button)
			{
			case 1:				// Left Button
				
				MovedX = MouseX - LastX;
				MovedY = MouseY - LastY;

				if (MovedX == 0 && MovedY == 0)
					break;

				ScrollX -= (MovedX);
				ScrollY -= (MovedY);

				while (ScrollX < 0)
				{
					TileX--;
					ScrollX += 256;
				}

				while (ScrollX > 255)
				{
					TileX++;
					ScrollX -= 256;
				}

				if (TileX < 0)
					TileX = 0;

				while (ScrollY < 0)
				{
					TileY--;
					ScrollY += 256;
				}

				while (ScrollY > 255)
				{
					TileY++;
					ScrollY -= 256;
				}

				if (TileY < 0)
					TileY = 0;
	
				NeedRefresh = TRUE;
				break;
			}
			break;
		} 
		}	// end of while xpending

		if (NeedRefresh)
		{
			SetBaseX = -1;
			LoadImageSet(Zoom, TileX, TileY);
		 	NeedRefresh = FALSE;
			SlowTimer = 0;
		}
		if (NeedRedraw)
		{
			NeedRedraw = 0;
			XPutImage (display, win, gc, image, ScrollX, ScrollY, 10, 10, cxImgSize, cyImgSize);
			DrawTracks(TRUE);
		}
	}

	memset((void *)&cfg, 0, sizeof(config_t));

	config_init(&cfg);

	croot = config_root_setting(&cfg);

	group = config_setting_add(croot, "APRS", CONFIG_TYPE_GROUP);

	SaveIntValue(group, "Zoom", Zoom);
	SaveIntValue(group, "SetBaseX", SetBaseX);
	SaveIntValue(group, "SetBaseY", SetBaseY);
	SaveIntValue(group, "ScrollX", ScrollX);
	SaveIntValue(group, "ScrollY", ScrollY);
	SaveIntValue(group, "WindowX", WindowX);
	SaveIntValue(group, "WindowY", WindowY);
	SaveIntValue(group, "WindowWidth", WindowWidth);
	SaveIntValue(group, "WindowHeight", WindowHeight);
	SaveIntValue(group, "WIDTHTILES", WIDTHTILES);
	SaveIntValue(group, "HEIGHTTILES", HEIGHTTILES);

	if(! config_write_file(&cfg, "BPQAPRS.cfg"))
		printf("Error while writing config file.\n");
	else
		printf("Config Saved\n");

	config_destroy(&cfg);

	status = XDestroyImage (image);
	return 0;
}


