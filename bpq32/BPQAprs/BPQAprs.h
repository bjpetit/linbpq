struct STATIONRECORD
{  
    char Callsign[12];
	char Path[120];
	char Comment[256];
	char LastPacket[400];
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
    time_t TimeAdded;
    time_t TimeLastUpdated;
	int iconRow;
	int iconCol;			// Symbol Pointer
	int DispX;				// Position in display buffer
	int DispY;

} StationRecord;

struct OSMQUEUE
{
	struct OSMQUEUE * Next;
	int	Zoom;
	int x;
	int y;
};

#define BPQBASE                        WM_USER
//
//	Port monitoring flags use BPQBASE -> BPQBASE+16

#define BPQMTX	      BPQBASE+40
#define BPQMCOM	      BPQBASE+41
#define BPQCOPY       BPQBASE+42
