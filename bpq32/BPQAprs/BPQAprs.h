
#define OurSetItemText(hwndLV, i, iSubItem_, pszText_) \
{ LV_ITEM _ms_lvi;\
  _ms_lvi.iSubItem = iSubItem_;\
  _ms_lvi.pszText = pszText_;\
  SNDMSG((hwndLV), LVM_SETITEMTEXT, (WPARAM)i, (LPARAM)(LV_ITEM FAR *)&_ms_lvi);\
}

#define TRACKPOINTS 100

struct STATIONRECORD
{  
	struct STATIONRECORD * Next;
	char Callsign[12];
	char Path[120];
	char Comment[256];
	char LastPacket[400];
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
	int Trackptr;					// Next record in Tracklog
	BOOL Moved;						// Moved since last drawn
    time_t TimeAdded;
    time_t TimeLastUpdated;
	int iconRow;
	int iconCol;					// Symbol Pointer
	char IconOverlay;
	int DispX;						// Position in display buffer
	int DispY;
	int Index;						// List Box Index

} StationRecord;

struct OSMQUEUE
{
	struct OSMQUEUE * Next;
	int	Zoom;
	int x;
	int y;
};

struct APRSMESSAGE
{
	struct APRSMESSAGE * Next;
	char FromCall[12];
	char ToCall[12];
	char Text[104];
	char Seq[8];
	BOOL Acked;
	int Retries;
	int RetryTimer;
	int Port;
};

#define BPQBASE                        WM_USER
//
//	Port monitoring flags use BPQBASE -> BPQBASE+16

#define BPQMTX	      BPQBASE+40
#define BPQMCOM	      BPQBASE+41
#define BPQCOPY       BPQBASE+42
