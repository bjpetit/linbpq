#define _USE_32BIT_TIME_T

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#else
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>


#define SOCKET int

#endif

void Debugprintf(const char * format, ...);

#define TRUE 1
#define true 1
#define FALSE 0
#define false 0

int ReloadMaps = 0;

int SetBaseX = 0;				// Lowest Tiles in currently loaded set
int SetBaseY = 0;

int Zoom = 3;

int MaxZoom = 18;

int cxWinSize = 1000, cyWinSize = 1000;
int cxImgSize, cyImgSize;

int ScrollX;
int ScrollY;

int MapCentreX = 0;
int MapCentreY = 0;

int MouseX, MouseY;
int PopupX, PopupY;

double Lat, Lon;

#define M_PI       3.14159265358979323846

struct STATIONRECORD ** StationRecords = NULL;
struct STATIONRECORD * ControlRecord;

struct OSMQUEUE
{
	struct OSMQUEUE * Next;
	int	Zoom;
	int x;
	int y;
};

#define TRACKPOINTS 100

struct STATIONRECORD
{
	struct STATIONRECORD * Next;
	char Callsign[12];
	char Path[120];
	char Status[256];
	char LastPacket[392];		// Was 400. 8 bytes used for Approx Location Flag and Qt Icon pointer

	char Approx;
	char spare1;
	char spare2;
	char spare3;
	void * image;

	char LastWXPacket[256];
	int LastPort;
	double Lat;
	double Lon;
	double Course;
	double Speed;
	double Heading;
	double LatIncr;
	double LongIncr;
	double LastCourse;
	double LastSpeed;
	double Distance;
	double Bearing;
	double LatTrack[TRACKPOINTS];	// Cyclic Tracklog
	double LonTrack[TRACKPOINTS];
	time_t TrackTime[TRACKPOINTS];
	int Trackptr;					// Next record in Tracklog
	int Moved;						// Moved since last drawn
	time_t TimeAdded;
	time_t TimeLastUpdated;
	unsigned char Symbol;
	int iconRow;
	int iconCol;					// Symbol Pointer
	char IconOverlay;
	int DispX;						// Position in display buffer
	int DispY;
	int Index;						// List Box Index
	int NoTracks;					// Suppress displaying track
	//TrackColour;
	char ObjState;					// Live/Killed flag. If zero, not an object
	char LastRXSeq[6];				// Seq from last received message (used for Reply-Ack system)
	int SimpleNumericSeq;			// Station treats seq as a number, not a text field
	struct STATIONRECORD * Object;	// Set if last record from station was an object
	time_t TimeLastTracked;			// Time of last trackpoint
	int NextSeq;

} StationRecord;


char APRSCall[10];
char BaseCall[10];
char LoppedAPRSCall[10];

int multiple = 0;

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;

	return ptr;
}


int long2tilex(double lon, int z)
{
	return (int)(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

int lat2tiley(double lat, int z)
{
	return (int)(floor((1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, z)));
}

double long2x(double lon, int z)
{
	return (lon + 180.0) / 360.0 * pow(2.0, z);
}

double lat2y(double lat, int z)
{
	return (1.0 - log(tan(lat * M_PI / 180.0) + 1.0 / cos(lat * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, z);
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

int GetLocPixels(double Lat, double Lon, int * X, int * Y)
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

int CentrePosition(double Lat, double Lon)
{
	// Positions the centre of the map at the specified location

	int X, Y;

	SetBaseX = long2tilex(Lon, Zoom) - 4;
	SetBaseY = lat2tiley(Lat, Zoom) - 4;				// Set Location at middle

	GetLocPixels(Lat, Lon, &X, &Y);

	ScrollX = X - cxWinSize / 2;
	ScrollY = Y - cyWinSize / 2;

	while (SetBaseX < 0)
	{
		SetBaseX++;
		ScrollX -= 256;
	}

	while (SetBaseY < 0)
	{
		SetBaseY++;
		ScrollY -= 256;
	}

	return TRUE;
}




//	OSM Tile Hnadling Stuff



struct OSMQUEUE OSMQueue = { NULL,0,0,0 };

int OSMQueueCount;

void getMutex();
void releaseMutex();


void OSMGet(int x, int y, int zoom)
{
	if (x < 0 || y < 0)
		return;


	// check if already on queue

	getMutex();

	struct OSMQUEUE * Rec = &OSMQueue;

	while (Rec)
	{
		if (Rec->x == x && Rec->y == y && Rec->Zoom == zoom)
		{
			// Leave on queue but set so OSMThread ignores them

			Rec->Zoom = 0;
		}
		Rec = Rec->Next;
	}

	struct OSMQUEUE * OSMRec = malloc(sizeof(struct OSMQUEUE));

	OSMQueueCount++;

	OSMRec->Next = OSMQueue.Next;
	OSMQueue.Next = OSMRec;
	OSMRec->x = x;
	OSMRec->y = y;
	OSMRec->Zoom = zoom;

	releaseMutex();

//	Debugprintf("Getting Tile %d %d Zoom %d", x, y, zoom);

}

// Shared Memory Code

typedef void *HANDLE;

struct APRSMESSAGE
{
	struct APRSMESSAGE * Next;
	struct STATIONRECORD * ToStation;	// Set on messages we send
	char FromCall[12];
	char ToCall[12];
	char Text[104];
	char Seq[8];
	int Acked;
	int Retries;
	int RetryTimer;
	int Port;
	char Time[6];
	int Cancelled;
};

struct APRSConnectionInfo			// Used for Web Server for thread-specific stuff
{
	struct STATIONRECORD * SelCall;	// Station Record for individual staton display
	HANDLE hPipe;
	SOCKET sock;
	char Callsign[12];
	int WindDirn, WindSpeed, WindGust, Temp, RainLastHour, RainLastDay, RainToday, Humidity, Pressure; //WX Fields
};

// This defines the layout of the first few bytes of shared memory to simplify access
// from both node and gui application

struct SharedMem
{
	unsigned char Version;				// For compatibility check
	unsigned char NeedRefresh;			// Messages Have Changed
	unsigned char ClearRX;
	unsigned char ClearTX;
	int SharedMemLen;			// So Client knows size to map

	struct APRSMESSAGE * Messages;
	struct APRSMESSAGE * OutstandingMsgs;

	int Arch;					 // to detect running on 64 bit system.
#pragma pack(1)
	unsigned char SubVersion;

};


HANDLE hMapFile;


struct SharedMem * SMEM;

unsigned char * Shared;
unsigned char * StnRecordBase;

#define APRSSHAREDMEMORYBASE 0x43000000		// Base of shared memory segment

#define MAXSTATIONS 5000
#define MAXMESSAGES 1000 


int GetSharedMemory()
{
#ifdef WIN32
	int Retries = 5;
	char Error[80];
	int SharedSize;

	while (Retries--)
	{
		hMapFile = CreateFileMapping(
			INVALID_HANDLE_VALUE, // use paging file
			NULL,				   // default security
			PAGE_READWRITE,       // read/write access
			0,                    // maximum object size (high-order DWORD)
			1024,				   // maximum object size (low-order DWORD)
			"BPQAPRSStationsMappingObject"); // name of mapping object

		if (hMapFile == NULL)
		{
			sprintf(Error, "Could not create file mapping object (%d).\n", GetLastError());
			MessageBox(NULL, Error, "BPQAPRS", MB_ICONERROR);
			return FALSE;
		}

		Shared = (LPTSTR)MapViewOfFileEx(hMapFile,   // handle to map object
			FILE_MAP_ALL_ACCESS, // read/write permission
			0,				// Offset Hi
			0,				// Offset Lo
			0,				// Map All
			(LPVOID)APRSSHAREDMEMORYBASE);

		if (Shared == NULL)
		{
			sprintf(Error, "Could not map view of file (%d).\n", GetLastError());
			MessageBox(NULL, Error, "BPQAPRS", MB_ICONERROR);
			CloseHandle(hMapFile);
			return FALSE;
		}

		// Get size then remap

		SMEM = (struct SharedMem *)Shared;

		SharedSize = SMEM->SharedMemLen;

		if (SharedSize > 0)
			break;				// Not intialised yet - wait

		UnmapViewOfFile(Shared);
		CloseHandle(hMapFile);

		// Try again

		Sleep(2000);

	}

	if (SharedSize == 0)
	{
		sprintf(Error, "Could not get size of Mapping object\n\n"
			"is APRS configured in BPQ32.cfg?");
		MessageBox(NULL, Error, "BPQAPRS", MB_ICONERROR);
		return FALSE;
	}

	UnmapViewOfFile(Shared);
	CloseHandle(hMapFile);

	// Reopen

	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		SharedSize,			 // maximum object size (low-order DWORD)
		"BPQAPRSStationsMappingObject"); // name of mapping object

	if (hMapFile == NULL)
	{
		sprintf(Error, "Could not create file mapping object (%d).\n", GetLastError());
		MessageBox(NULL, Error, "BPQAPRS", MB_ICONERROR);

		return FALSE;

	}

	Shared = (LPTSTR)MapViewOfFileEx(hMapFile,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		0,
		(LPVOID)APRSSHAREDMEMORYBASE);

	if (Shared == NULL)
	{
		sprintf(Error, "Could not map view of file (%d).\n", GetLastError());
		MessageBox(NULL, Error, "BPQAPRS", MB_ICONERROR);
		CloseHandle(hMapFile);
		return FALSE;
	}

	if (Shared != (void *)APRSSHAREDMEMORYBASE)
	{
		sprintf(Error, "File is mapped in Wrong Place %p %x.\n", Shared, APRSSHAREDMEMORYBASE);
		MessageBox(NULL, Error, "BPQAPRS", MB_ICONERROR);
		UnmapViewOfFile(Shared);
		CloseHandle(hMapFile);
		return FALSE;
	}

	if (SMEM->Version != 1)
	{
		sprintf(Error, "Version Mismatch with bpq32.dll");
		MessageBox(NULL, Error, "BPQAPRS", MB_ICONERROR);
		UnmapViewOfFile(Shared);
		CloseHandle(hMapFile);
		return FALSE;
	}

	StnRecordBase = Shared + 32;

	StationRecords = (struct STATIONRECORD**)StnRecordBase;

	ControlRecord = (struct STATIONRECORD*)StnRecordBase;

	memset(APRSCall, 0x20, 9);
	memcpy(APRSCall, ControlRecord->Callsign, strlen(ControlRecord->Callsign));

	printf("BPQ32 Configured with MaxStations %d APRSCall %s\n",
		ControlRecord->LastPort, APRSCall);

	memcpy(BaseCall, APRSCall, 10);		// Get call less SSID
	strlop(BaseCall, ' ');
	strlop(BaseCall, '-');


#else

	char BPQDirectory[256];
	char SharedName[256];
	char * ptr1;
	int fd;

	int SharedSize;

	// Get shared memory

	// Append last bit of current directory to shared name

	getcwd(BPQDirectory, 256);
	ptr1 = BPQDirectory;

	while (strchr(ptr1, '/'))
	{
		ptr1 = strchr(ptr1, '/');
		ptr1++;
	}

	if (multiple)
		sprintf(SharedName, "/BPQAPRSSharedMem%s", ptr1);
	else
		strcpy(SharedName, "/BPQAPRSSharedMem");

	printf("Using Shared Memory %s\n", SharedName);


	fd = shm_open(SharedName, O_RDWR, S_IRUSR | S_IWUSR);
	if (fd == -1)
	{
		printf("Open APRS Shared Memory %s Failed\n", SharedName);
		return 0;
	}
	else
	{
		// Map shared memory object

		Shared = mmap((void *)APRSSHAREDMEMORYBASE, 8192 * 4096,
			PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

		if (Shared == MAP_FAILED)
		{
			printf("Map APRS Shared Memory Failed\n");
			return 0;
		}
	}

	if (Shared != (void *)APRSSHAREDMEMORYBASE)
	{
		printf("Map APRS Shared Memory Allocated at wrong address %p Should be 0x43000000\n", Shared);
		return 0;
	}

	printf("Map APRS Shared Memory Allocated at %p\n", Shared);


	SMEM = (struct SharedMem *)Shared;
	SharedSize = SMEM->SharedMemLen;

	printf("Shared Memory Size %d Max %d\n", SharedSize, 8192 * 4096);

	if (SharedSize > 8192 * 4096)
	{
		printf("MAXSTATIONS too high\n");
		return 0;
	}

	StnRecordBase = Shared + 32;
	StationRecords = (struct STATIONRECORD**)StnRecordBase;

	ControlRecord = (struct STATIONRECORD*)StnRecordBase;

	memset(APRSCall, 0x20, 9);
	memcpy(APRSCall, ControlRecord->Callsign, strlen(ControlRecord->Callsign));

	printf("LinBPQ Configured with MaxStations %d APRSCall %s\n",
		ControlRecord->LastPort, APRSCall);

	memcpy(BaseCall, APRSCall, 10);		// Get call less SSID
	strlop(BaseCall, ' ');
	strlop(BaseCall, '-');

	//	Remap with Server's view of MaxStations

	//	munmap(APRSStationMemory, 4096 * 4096);

	//	perror("munmap");

	//	Shared = mmap((void *)APRSSHAREDMEMORYBASE, SharedSize,
	//		PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	//	printf("Map APRS Shared Memory Allocated at %x\n", Shared);

	if (Shared == MAP_FAILED)
	{
		printf("Extend APRS Shared Memory Failed\n");
		return 0;
	}
#endif

	memcpy(LoppedAPRSCall, APRSCall, 10);
	strlop(LoppedAPRSCall, ' ');


	SMEM->NeedRefresh = 1;		// Initial Load of messages

	return true;
}

char *  GetAPRSLatLon(double * PLat, double * PLon)
{
	*PLat = ControlRecord->Lat;
	*PLon = ControlRecord->Lon;

	return 0;
}

