
// Module to implement APRS "New Paradigm" Digipeater and APRS-IS Gateway

// First Version, November 2011


#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE 
#define _USE_32BIT_TIME_T	// Until the ASM code switches to 64 bit time

#define Dll	__declspec(dllexport)
#define DllImport __declspec(dllimport)

#include "windows.h"
#include <stdio.h>
#include "AsmStrucs.h"
#include "bpq32.h"
#include <time.h>
#include "kernelresource.h"

#define MAXAGE 3600 * 12	  // 12 Hours
#define MAXCALLS 20			  // Max Flood, Trace and Digi
#define GATETIMELIMIT 40 * 60 // Don't gate to RF if station not heard for this time (40 mins)

static BOOL APIENTRY  GETSENDNETFRAMEADDR();
static VOID DoSecTimer();
static ProcessLine(char * buf);
static BOOL ReadConfigFile();
VOID APRSISThread(BOOL Report);
unsigned long _beginthread( void( *start_address )(BOOL Report), unsigned stack_size, void * arglist);
VOID __cdecl Debugprintf(const char * format, ...);
VOID __cdecl Consoleprintf(const char * format, ...);
BOOL APIENTRY  Send_AX(PMESSAGE Block, DWORD Len, UCHAR Port);
static VOID Send_AX_Datagram(PDIGIMESSAGE Block, DWORD Len, UCHAR Port);
char * strlop(char * buf, char delim);
VOID * APIENTRY GetBuff();
UINT ReleaseBuffer(UINT *BUFF);
UINT * Q_REM(UINT *Q);
int C_Q_ADD(UINT *Q,UINT *BUFF);
int TelDecodeFrame(char * msg, char * buffer, int Stamp);		// Unsemaphored DecodeFrame
struct APRSSTATIONRECORD * UpdateHeard(UCHAR * Call, int Port);
BOOL CheckforDups(char * Call, char * Msg, int Len);
VOID ProcessQuery(char * Query);
VOID ProcessSpecificQuery(char * Query, int Port, char * Origin, char * DestPlusDigis);
VOID CheckandDigi(DIGIMESSAGE * Msg, int Port, int FirstUnused, int Digis, int Len);		
Dll VOID APIENTRY SendBeacon(int toPort, char * Msg);
Dll BOOL APIENTRY PutAPRSMessage(char * Frame, int Len);
VOID ProcessAPRSISMsg(char * APRSMsg);
static VOID SendtoDigiPorts(PDIGIMESSAGE Block, DWORD Len, UCHAR Port);
struct APRSSTATIONRECORD * LookupStation(char * call);
BOOL OpenGPSPort();
void PollGPSIn();
int CountLocalStations();
BOOL SendAPPLAPRSMessage(char * Frame);

BOOL ProcessConfig();
BOOL FreeConfig();

void GetSemaphore();
void FreeSemaphore();

extern int SemHeldByAPI;

// All data should be initialised to force into shared segment

static char ConfigClassName[]="CONFIG";

extern struct BPQVECSTRUC APRSMONVEC;
extern int MONDECODE();
extern VOID * zalloc(int len);
extern BOOL StartMinimized;
extern BOOL MinimizetoTray;
extern char * PortConfig[];
extern char TextVerstring[];

extern short NUMBEROFPORTS;

extern byte	MCOM;
extern char	MTX;
extern ULONG MMASK;
extern HWND hWnd;
extern HKEY REGTREE;

static int SecTimer = 10;

BOOL ApplConnected = FALSE;  

UINT APPL_Q = 0;				// Queue of frames for APRS Appl
UINT APPLTX_Q = 0;				// Queue of frames from APRS Appl
UINT APRSPortMask = 0;

char APRSCall[10] = "";
char APRSDest[10] = "APBPQ1";

UCHAR AXCall[7] = "";

char CallPadded[10] = "         ";

int GPSPort = 0;
int GPSSpeed = 0;

BOOL GPSOK;

char LAT[] = "0000.00N";	// in standard APRS Format      
char LON[] = "00000.00W";	//in standard APRS Format

double Lat = 0.0;
double Lon = 0.0;

BOOL PosnSet = FALSE;
/*
The null position should be include the \. symbol (unknown/indeterminate
position). For example, a Position Report for a station with unknown position
will contain the coordinates …0000.00N\00000.00W.…
*/
char * FloodCalls = 0;			// Calls to relay using N-n without tracing
char * TraceCalls = 0;			// Calls to relay using N-n with tracing
char * DigiCalls = 0;			// Calls for normal relaying

UCHAR FloodAX[MAXCALLS][7] = {0};
UCHAR TraceAX[MAXCALLS][7] = {0};
UCHAR DigiAX[MAXCALLS][7] = {0};

int FloodLen[MAXCALLS];
int TraceLen[MAXCALLS];
int DigiLen[MAXCALLS];

int ISPort = 0;
char ISHost[256] = "";
int ISPasscode = 0;
char NodeFilter[1000] = "";		// Filter when the isn't an application
char ISFilter[1000] = "";		// Current Filter
char APPLFilter[1000] = "";		// Filter when an Applcation is running

extern BOOL IGateEnabled;

char StatusMsg[256] = "";				// Must be in shared segment
int StatusMsgLen = 0;

char * BeaconPath[33] = {0};

char CrossPortMap[33][33] = {0};

UCHAR BeaconHeader[33][10][7] = {""};	//	Dest, Source and up to 8 digis 
int BeaconHddrLen[33] = {0};			// Actual Length used

char CFGSYMBOL = '=';
char CFGSYMSET = '/';

char SYMBOL = '=';
char SYMSET = '/';

BOOL TraceDigi = FALSE;					// Add Trace to packets relayed on Digi Calls

int MaxTraceHops = 2;
int MaxFloodHops = 2;

int BeaconInterval = 0;
int BeaconCounter = 0;
int IStatusCounter = 0;					// Used to send ?ISTATUS? Responses

BOOL APRSISOpen = FALSE;
int ISDelayTimer = 0;					// Time before trying to reopen APRS-IS link

char APRSDESTS[][7] = {"AIR*", "ALL*", "AP*", "BEACON", "CQ*", "GPS*", "DF*", "DGPS*", "DRILL*",
				"DX*", "ID*", "JAVA*", "MAIL*", "MICE*", "QST*", "QTH*", "RTCM*", "SKY*",
				"SPACE*", "SPC*", "SYM*", "TEL*", "TEST*", "TLM*", "WX*", "ZIP"};

UCHAR AXDESTS[30][7] = {""};
int AXDESTLEN[30] = {0};

UCHAR axTCPIP[7];
UCHAR axRFONLY[7];
UCHAR axNOGATE[7];

int MessageCount;

// Heard Station info

#define MAXHEARD 1000

int HEARDENTRIES = 0;
int MAXHEARDENTRIES = 0;
int MHLEN = sizeof(struct APRSSTATIONRECORD);

// Area is allocated as needed

struct APRSSTATIONRECORD MHTABLE[MAXHEARD] = {0};

struct APRSSTATIONRECORD * MHDATA = &MHTABLE[0];

static SOCKET sock = (SOCKET) NULL;

//Duplicate suppression Code

#define MAXDUPS 100			// Number to keep
#define DUPSECONDS 28		// Time to Keep

typedef struct DUPINFO
{
	time_t DupTime;
	int DupLen;
	char  DupUser[8];		// Call in ax.35 format
	char  DupText[100];
};

struct DUPINFO DupInfo[MAXDUPS];

Dll BOOL APIENTRY Init_APRS()
{
	int i;
	char * DCall;
	int retCode, Type, Vallen;
	HKEY hKey=0;

	retCode = RegOpenKeyEx (REGTREE,
                "SOFTWARE\\G8BPQ\\BPQ32",    
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen = 4;
		retCode = RegQueryValueEx(hKey, "IGateEnabled", 0, &Type, (UCHAR *)&IGateEnabled, &Vallen);
	}

	ConvToAX25(GetNodeCall(), MYCALL);

	ConvToAX25("TCPIP", axTCPIP);
	ConvToAX25("RFONLY", axRFONLY);
	ConvToAX25("NOGATE", axNOGATE);

	memset(&FloodAX[0][0], 0, sizeof(FloodAX));
	memset(&TraceAX[0][0], 0, sizeof(TraceAX));
	memset(&DigiAX[0][0], 0, sizeof(DigiAX));

	APRSPortMask = 0;

	memset(&CrossPortMap[0][0], 0, sizeof(CrossPortMap));

	for (i = 1; i <= NUMBEROFPORTS; i++)
	{
		CrossPortMap[i][i] = TRUE;			// Set Defaults - Same Port
		CrossPortMap[i][0] = TRUE;			// and APRS-IS
	}

	PosnSet = 0;

	if (ReadConfigFile() == 0)
		return FALSE;

	if (PosnSet == 0)
	{
		SYMBOL = '.';
		SYMSET = '\\';				// Undefined Posn Symbol
	}
	else
	{
		// Convert posn to floating degrees

		char LatDeg[3], LonDeg[4];
		memcpy(LatDeg, LAT, 2);
		LatDeg[2]=0;
		Lat=atof(LatDeg) + (atof(LAT+2)/60);
	
		if (LAT[7] == 'S') Lat=-Lat;
		
		memcpy(LonDeg, LON, 3);
		LonDeg[3]=0;
		Lon=atof(LonDeg) + (atof(LON+3)/60);
       
		if (LON[8]== 'W') Lon=-Lon;

		SYMBOL = CFGSYMBOL;
		SYMSET = CFGSYMSET;
	}

	// Convert Dest ADDRS to AX.25

	for (i = 0; i < 26; i++)
	{
		DCall = &APRSDESTS[i][0];
		if (strchr(DCall, '*'))
			AXDESTLEN[i] = strlen(DCall) - 1;
		else
			AXDESTLEN[i] = 6;

		ConvToAX25(DCall, &AXDESTS[i][0]);
	}

	// Setup Heard Data Area

	HEARDENTRIES = 0;
	MAXHEARDENTRIES = MAXHEARD;

	APRSMONVEC.HOSTAPPLFLAGS = 0x80;		// Request Monitoring

	if (ISPort && IGateEnabled)
	{
		_beginthread(APRSISThread, 0, (VOID *) TRUE);
	}

	if (GPSPort)
		OpenGPSPort();

	WritetoConsole("APRS Digi/Gateway Enabled\n");
	return TRUE;
}

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

VOID APRSClose()
{
	if (sock)
	{		
		shutdown(sock, SD_BOTH);
		Sleep(50);

		closesocket(sock);
	}
}

Dll VOID APIENTRY Poll_APRS()
{
	SecTimer--;

	if (SecTimer == 0)
	{
		SecTimer = 10;
		DoSecTimer();
	}

	if (GPSPort)
		PollGPSIn();

	if (APPLTX_Q)
	{
		UINT * buffptr = Q_REM(&APPLTX_Q);
			
		SendAPPLAPRSMessage((char *)&buffptr[2]);
		ReleaseBuffer(buffptr);
	}

	while (APRSMONVEC.HOSTTRACEQ)
	{
		int stamp, len;
		BOOL MonitorNODES = FALSE;
		UINT * monbuff;
		UCHAR * monchars;
		MESSAGE * Orig;
		int Digis = 0;
		MESSAGE * AdjBuff;		// Adjusted for digis
		BOOL FirstUnused = FALSE;
		DIGIMESSAGE Msg = {0};
		int Port, i;
		char * DEST;
		unsigned char buffer[1024];
		char ISMsg[500];
		char * ptr1;
		char * Payload;
		char * ptr3;
		char * ptr4;
		ULONG SaveMMASK = MMASK; 
		BOOL ThirdParty = FALSE;
		BOOL NoGate = FALSE;
		struct APRSSTATIONRECORD * MH;

		monbuff = Q_REM((UINT *)&APRSMONVEC.HOSTTRACEQ);

		monchars = (UCHAR *)monbuff;
		AdjBuff = Orig = (MESSAGE *)monchars;	// Adjusted for digis

		Port = Orig->PORT;
		
		if (Port & 0x80)		// TX
		{
			ReleaseBuffer(monbuff);
			continue;
		}

		if ((APRSPortMask & (1 << (Port - 1))) == 0)// Port in use for APRS?
		{
			ReleaseBuffer(monbuff);
			continue;
		}

		stamp = monbuff[88];

		if ((UCHAR)monchars[4] & 0x80)		// TX
		{
			ReleaseBuffer(monbuff);
			continue;
		}

		// See if digipeaters present. 

		while ((AdjBuff->ORIGIN[6] & 1) == 0 && Digis < 9)
		{
			_asm {
				mov eax, AdjBuff
				add eax, 7
				mov AdjBuff, eax
			}

			if (memcmp(AdjBuff->ORIGIN, axTCPIP, 6) == 0)
				ThirdParty = TRUE;

			Digis ++;

			if (FirstUnused == FALSE && (AdjBuff->ORIGIN[6] & 0x80) == 0)
			{
				// Unused Digi - see if we should digi it

				FirstUnused = Digis;
		//		CheckDigi(buff, AdjBuff->ORIGIN);
			}

		}

		if (Digis > 8)
		{
			ReleaseBuffer(monbuff);
			continue;					// Corrupt
		}

		if (Digis)
		{
			if (memcmp(AdjBuff->ORIGIN, axNOGATE, 6) == 0 || memcmp(AdjBuff->ORIGIN, axRFONLY, 6) == 0)

				// Last digis is NOGATE or RFONLY - dont send to IS

				NoGate = TRUE;
		}
		if (AdjBuff->CTL != 3 || AdjBuff->PID != 0xf0)				// Only UI
		{
			ReleaseBuffer(monbuff);
			continue;			
		}
		if (CheckforDups(Orig->ORIGIN, AdjBuff->L2DATA, Orig->LENGTH - Digis * 7 - 23))
		{	
			ReleaseBuffer(monbuff);
			continue;			
		}

		// Decode Frame to TNC2 Monitor Format

		MMASK = 0xffff;
		len = TelDecodeFrame((char *)monchars,  buffer, stamp);
		MMASK = SaveMMASK;

		if(len == 0)
		{
			// Couldn't Decode

			ReleaseBuffer(monbuff);
			continue;			
		}

		// if APRS Appl is atttached, queue message to it

		if (ApplConnected)
		{
			UINT * buffptr = GetBuff();
				
			if (buffptr)
			{
				buffptr[1] = len;
				buffptr[2] = Port;
				memcpy(&buffptr[3], buffer, len);
				C_Q_ADD(&APPL_Q, buffptr);
			}
		}

		buffer[len++] = 10;
		buffer[len] = 0;
		ptr1 = &buffer[10];				// Skip Timestamp
		Payload = strchr(ptr1, ':') + 2; // Start of Payload
		ptr3 = strchr(ptr1, ' ');		// End of addresses
		*ptr3 = 0;

		// if digis, remove any unactioned ones

		if (Digis)
		{
			ptr4 = strchr(ptr1, '*');		// Last Used Digi

			if (ptr4)
			{
				// We need header up to ptr4

				*(ptr4) = 0;
			}
			else
			{
				// No digis actioned - remove them all

				ptr4 = strchr(ptr1, ',');		// End of Dest
				*ptr4 = 0;
			}
		}

		ptr4 = strchr(ptr1, '>');		// End of Source
		*ptr4++ = 0;

		MH = UpdateHeard(ptr1, Port);

		if (ThirdParty)
			MH->IGate= TRUE;			// if we've seen msgs to TCPIP, it must be an Igate

		if (NoGate)
			goto NoIS;

		// I think all PID F0 UI frames go to APRS-IS,
		// Except General Queries, Frames Gated from IS to RF, and Messages Addressed to us

		// or should we process Query frames locally ??

		if (Payload[0] == '}')
			goto NoIS;

		if (Payload[0] == '?')
		{
			// General Query

			ProcessQuery(&Payload[1]);

			// ?? Should we pass addressed Queries to IS ??
	
			goto NoIS;
		}

		if (Payload[0] == ':' && memcmp(&Payload[1], CallPadded, 9) == 0)
		{
			// Message for us

			if (Payload[11] == '?')			// Only queries - the node doesnt do messaging
				ProcessSpecificQuery(&Payload[12], Port, ptr1, ptr4);

			goto NoIS;
		}

		if (APRSISOpen && CrossPortMap[Port][0])	// No point if not open
		{
			len = wsprintf(ISMsg, "%s>%s,qAR,%s:%s", ptr1, ptr4, APRSCall, Payload);

			send(sock, ISMsg, len, 0);
	
			ptr1 = strchr(ISMsg, 13);
			if (ptr1) *ptr1 = 0;
			Debugprintf(">%s", ISMsg);
		}	
	
	NoIS:
	
		// See if it is an APRS frame

		// If MIC-E, we need to process, whatever the destination

		DEST = &Orig->DEST[0];

		for (i = 0; i < 26; i++)
		{
			if (memcmp(DEST, &AXDESTS[i][0], AXDESTLEN[i]) == 0)
				goto OK;
		}

		switch(AdjBuff->L2DATA[0])
		{
			case '`':
			case 0x27:					// '
			case 0x1c:
			case 0x1d:					// MIC-E

				break;
		//	default:

				// Not to an APRS Destination
			
//				ReleaseBuffer(monbuff);
//				continue;
		}

OK:
		// If there are unused digis, we may need to digi it.

		if (Digis == 0 || FirstUnused == 0)
		{
			// No Digis, so finished

			ReleaseBuffer(monbuff);
			continue;
		}

		// Copy frame to a DIGIMessage Struct

		memcpy(&Msg, monbuff, 21 + (7 * Digis));		// Header, Dest, Source, Addresses and Digis

		len = Msg.LENGTH - 21 - (7 * Digis);			// Payload Length (including CTL and PID

		memcpy(&Msg.CTL, &AdjBuff->CTL, len);

		// Pass to Digi Code

		CheckandDigi(&Msg, Port, FirstUnused, Digis, len);		// Digi if necessary		

		ReleaseBuffer(monbuff);
	}

	return;
}

VOID CheckandDigi(DIGIMESSAGE * Msg, int Port, int FirstUnused, int Digis, int Len)
{
	UCHAR * Digi = &Msg->DIGIS[--FirstUnused][0];
	UCHAR * Call;
	int Index = 0;
	int SSID;

	// Check ordinary digi first

	Call = &DigiAX[0][0];
	SSID = Digi[6] & 0x1e;

	while (*Call)
	{
		if ((memcmp(Digi, Call, 6) == 0) && ((Call[6] & 0x1e) == SSID))
		{
			// Trace Call if enabled

			if (TraceDigi)
				memcpy(Digi, AXCall, 7);
	
			// mark as used;
		
			Digi[6] |= 0x80;	// Used bit

			SendtoDigiPorts(Msg, Len, Port);
			return;
		}
		Call += 7;
		Index++;
	}

	Call = &TraceAX[0][0];
	Index = 0;

	while (*Call)
	{
		if (memcmp(Digi, Call, TraceLen[Index]) == 0)
		{
			// if possible move calls along
			// insert our call, set used
			// decrement ssid, and if zero, mark as used;

			SSID = (Digi[6] & 0x1E) >> 1;

			if (SSID == 0)	
				return;					// Shouldn't have SSID 0 for Rrace/Flood

			if (SSID > MaxTraceHops)
				SSID = MaxTraceHops;	// Enforce our limit

			SSID--;

			if (SSID ==0)				// Finihed with it ?
				Digi[6] = (SSID << 1) | 0xe0;	// Used and Fixed bits
			else
				Digi[6] = (SSID << 1) | 0x60;	// Fixed bits

			if (Digis < 8)
			{
				memmove(Digi + 7, Digi, (Digis - FirstUnused) * 7);
			}
				
			memcpy(Digi, AXCall, 7);
			Digi[6] |= 0x80;

			SendtoDigiPorts(Msg, Len, Port);

			return;
		}
		Call += 7;
		Index++;
	}

	Index = 0;
	Call = &FloodAX[0][0];

	while (*Call)
	{
		if (memcmp(Digi, Call, FloodLen[Index]) == 0)
		{
			// decrement ssid, and if zero, mark as used;

			SSID = (Digi[6] & 0x1E) >> 1;

			if (SSID == 0)	
				return;					// Shouldn't have SSID 0 for Trace/Flood

			if (SSID > MaxFloodHops)
				SSID = MaxFloodHops;	// Enforce our limit

			SSID--;

			if (SSID ==0)						// Finihed with it ?
				Digi[6] = (SSID << 1) | 0xe0;	// Used and Fixed bits
			else
				Digi[6] = (SSID << 1) | 0x60;	// Fixed bits

			SendtoDigiPorts(Msg, Len, Port);

			return;
		}
		Call += 7;
		Index++;
	}
}


  
static VOID SendtoDigiPorts(PDIGIMESSAGE Block, DWORD Len, UCHAR Port)
{
	//	Can't use API SENDRAW, as that tries to get the semaphore, which we already have
	//  Len is the Payload Length (from CTL onwards)
	// The message can contain DIGIS - The payload must be copied forwards if there are less than 8

	// We send to all ports enabled in CrossPortMap

	UCHAR * EndofDigis = &Block->CTL;
	int i = 0;
	int toPort;

	while (Block->DIGIS[i][0] && i < 8)
	{
		i++;
	}

	EndofDigis = &Block->DIGIS[i][0];
	*(EndofDigis -1) |= 1;					// Set End of Address Bit

	if (i != 8)
		memmove(EndofDigis, &Block->CTL, Len);

	Len = Len + (i * 7) + 14;					// Include Source, Dest and Digis

//	Block->DEST[6] &= 0x7e;						// Clear End of Call
//	Block->ORIGIN[6] |= 1;						// Set End of Call

//	Block->CTL = 3;		//UI

	for (toPort = 1; toPort <= NUMBEROFPORTS; toPort++)
	{
		if (CrossPortMap[Port][toPort])
		Send_AX((PMESSAGE)Block, Len, toPort);
	}
	return;

}

static VOID Send_AX_Datagram(PDIGIMESSAGE Block, DWORD Len, UCHAR Port)
{
	//	Can't use API SENDRAW, as that tries to get the semaphore, which we already have

	//  Len is the Payload Length (from CTL onwards)

	// The message can contain DIGIS - The payload must be copied forwards if there are less than 8

	UCHAR * EndofDigis = &Block->CTL;

	int i = 0;

	while (Block->DIGIS[i][0] && i < 8)
	{
		i++;
	}

	EndofDigis = &Block->DIGIS[i][0];
	*(EndofDigis -1) |= 1;					// Set End of Address Bit

	if (i != 8)
		memmove(EndofDigis, &Block->CTL, Len);

	Len = Len + (i * 7) + 14;					// Include Source, Dest and Digis

//	Block->DEST[6] &= 0x7e;						// Clear End of Call
//	Block->ORIGIN[6] |= 1;						// Set End of Call

//	Block->CTL = 3;		//UI

	Send_AX((PMESSAGE)Block, Len, Port);
	return;

}

static BOOL ReadConfigFile()
{
	char * Config;
	char * ptr1, * ptr2;

	char buf[256],errbuf[256];

	Config = PortConfig[34];		// Config fnom bpq32.cfg

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
	
			if (!ProcessLine(buf))
			{
				WritetoConsole("APRS Bad config record ");
				strcat(errbuf, "\r\n");
				WritetoConsole(errbuf);
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL ConvertCalls(char * DigiCalls, UCHAR * AX, int * Lens)
{
	int Index = 0;
	char * ptr;
	char * Context;
	UCHAR Work[MAXCALLS][7] = {0};
	int Len[MAXCALLS] = {0};
		
	ptr = strtok_s(DigiCalls, ", ", &Context);

	while(ptr)
	{
		if (Index == MAXCALLS) return FALSE;

		ConvToAX25(ptr, &Work[Index][0]);
		Len[Index++] = strlen(ptr);
		ptr = strtok_s(NULL, ", ", &Context);
	}

	memcpy(AX, Work, sizeof(Work));
	memcpy(Lens, Len, sizeof(Len));
	return TRUE;
}



static ProcessLine(char * buf)
{
	PCHAR ptr, p_value;

	ptr = strtok(buf, "= \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment


	if (_stricmp(ptr, "STATUSMSG") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");
		memcpy(StatusMsg, p_value, 255);	// Just in case too long
		StatusMsgLen = strlen(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "ISFILTER") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");
		strcpy(ISFilter, p_value);
		strcpy(NodeFilter, ISFilter);
		return TRUE;
	}

	if (_stricmp(ptr, "ReplaceDigiCalls") == 0)
	{
		TraceDigi = TRUE;
		return TRUE;
	}

	p_value = strtok(NULL, " \t\n\r");

	if (p_value == NULL)
		return FALSE;

	if (_stricmp(ptr, "APRSCALL") == 0)
	{
		strcpy(APRSCall, p_value);
		memcpy(CallPadded, APRSCall, strlen(APRSCall));	// Call Padded to 9 chars for APRS Messaging

		// Convert to ax.25

		return ConvToAX25(APRSCall, AXCall);
	}

	if (_stricmp(ptr, "APRSPATH") == 0)
	{
		int Digi = 2;
		int Port;
		char * Context;

		p_value = strtok_s(p_value, "=\t\n\r", &Context);

		Port = atoi(p_value);

		if (GetPortTableEntryFromPortNum(Port) == NULL)
			return FALSE;

		APRSPortMask |= 1 << (Port - 1);

		if (Context == NULL || Context[0] == 0)
			return TRUE;					// No dest - a receive-only port

		BeaconPath[Port] = _strdup(_strupr(Context));
	
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		if (ptr == NULL)
			return FALSE;

		ConvToAX25(APRSCall, &BeaconHeader[Port][1][0]);

		if (_stricmp(ptr, "APRS") == 0)			// First is Dest
			ConvToAX25(APRSDest, &BeaconHeader[Port][0][0]);
		else if (_stricmp(ptr, "APRS-0") == 0)
			ConvToAX25("APRS", &BeaconHeader[Port][0][0]);
		else
			ConvToAX25(ptr, &BeaconHeader[Port][0][0]);
		
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		while (ptr)
		{
			ConvToAX25(ptr, &BeaconHeader[Port][Digi++][0]);
			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}

		BeaconHddrLen[Port] = Digi * 7;

		return TRUE;
	}

	if (_stricmp(ptr, "DIGIMAP") == 0)
	{
		int DigiTo;
		int Port;
		char * Context;

		p_value = strtok_s(p_value, "=\t\n\r", &Context);

		Port = atoi(p_value);

		if (GetPortTableEntryFromPortNum(Port) == NULL)
			return FALSE;

		if (Context == NULL)
			return FALSE;

		CrossPortMap[Port][Port] = FALSE;	// Cancel Default mapping
		CrossPortMap[Port][0] = FALSE;		// Cancel Default APRSIS
	
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		while (ptr)
		{
			DigiTo = atoi(ptr);				// this gives zero for IS
	
			if (DigiTo > NUMBEROFPORTS)
				return FALSE;

			CrossPortMap[Port][DigiTo] = TRUE;	
			ptr = strtok_s(NULL, " ,\t\n\r", &Context);
		}

		return TRUE;
	}


	if (_stricmp(ptr, "BeaconInterval") == 0)
	{
		BeaconInterval = atoi(p_value);

		if (BeaconInterval < 5)
			BeaconInterval = 5;

		if (BeaconInterval)
			BeaconCounter = 30;				// Send first after 30 secs

		return TRUE;
	}

	if (_stricmp(ptr, "TRACECALLS") == 0)
	{
		TraceCalls = _strdup(_strupr(p_value));
		ConvertCalls(TraceCalls, &TraceAX[0][0], &TraceLen[0]);
		return TRUE;
	}

	if (_stricmp(ptr, "FLOODCALLS") == 0)
	{
		FloodCalls = _strdup(_strupr(p_value));
		ConvertCalls(FloodCalls, &FloodAX[0][0], &FloodLen[0]);
		return TRUE;
	}

	if (_stricmp(ptr, "DIGICALLS") == 0)
	{
		char AllCalls[1024];
		
		DigiCalls = _strdup(_strupr(p_value));
		strcpy(AllCalls, APRSCall);
		strcat(AllCalls, ",");
		strcat(AllCalls, DigiCalls);
		ConvertCalls(AllCalls, &DigiAX[0][0], &DigiLen[0]);
		return TRUE;
	}

	if (_stricmp(ptr, "GPSPort") == 0)
	{
		GPSPort = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "GPSSpeed") == 0)
	{
		GPSSpeed = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "LAT") == 0)
	{
		if (strlen(p_value) != 8)
			return FALSE;

		memcpy(LAT, _strupr(p_value), 8);
		PosnSet = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "LON") == 0)
	{
		if (strlen(p_value) != 9)
			return FALSE;

		memcpy(LON, _strupr(p_value), 9);
		PosnSet = TRUE;
		return TRUE;
	}

	if (_stricmp(ptr, "SYMBOL") == 0)
	{
		CFGSYMBOL = p_value[0];
		return TRUE;
	}

	if (_stricmp(ptr, "SYMSET") == 0)
	{
		CFGSYMSET = p_value[0];
		return TRUE;
	}

	if (_stricmp(ptr, "MaxTraceHops") == 0)
	{
		MaxTraceHops = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "MaxFloodHops") == 0)
	{
		MaxFloodHops = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "ISHOST") == 0)
	{
		strncpy(ISHost, p_value, 250);
		return TRUE;
	}

	if (_stricmp(ptr, "ISPORT") == 0)
	{
		ISPort = atoi(p_value);
		return TRUE;
	}

	if (_stricmp(ptr, "ISPASSCODE") == 0)
	{
		ISPasscode = atoi(p_value);
		return TRUE;
	}

	//
	//	Bad line
	//
	return (FALSE);	
}

VOID SendAPRSMessage(char * Message, int toPort)
{
	int Port;
	DIGIMESSAGE Msg;
	
	int Len;

	// toPort = -1 means all tadio ports. 0 = IS

	if (toPort == -1)
	{
		for (Port = 1; Port <= NUMBEROFPORTS; Port++)
		{
			if (BeaconHddrLen[Port])		// Only send to ports with a DEST defined
			{
				memcpy(Msg.DEST, &BeaconHeader[Port][0][0],  BeaconHddrLen[Port] + 1);
				Msg.PID = 0xf0;
				Msg.CTL = 3;
				Len = wsprintf(Msg.L2DATA, "%s", Message);
				Send_AX_Datagram(&Msg, Len + 2, Port);
			}
		}

		return;
	}

	if (toPort == 0 && APRSISOpen)
	{
		char ISMsg[300];

		Len = wsprintf(ISMsg, "%s>%s,TCPIP*:%s\r\n", APRSCall, APRSDest, Message);
		send(sock, ISMsg, Len, 0);
		Len = GetLastError();
		Debugprintf(">%s", ISMsg);
	}

	if (toPort && BeaconHddrLen[toPort])
	{
		memcpy(Msg.DEST, &BeaconHeader[toPort][0][0],  BeaconHddrLen[toPort] + 1);
		Msg.PID = 0xf0;
		Msg.CTL = 3;
		Len = wsprintf(Msg.L2DATA, "%s", Message);
		Send_AX_Datagram(&Msg, Len + 2, toPort);

		return;
	}
}


VOID ProcessSpecificQuery(char * Query, int Port, char * Origin, char * DestPlusDigis)
{
	if (memcmp(Query, "APRSS", 5) == 0)
	{
		char Message[255];
	
		wsprintf(Message, ":%-9s:%s", Origin, StatusMsg);
		SendAPRSMessage(Message, Port);

		return;
	}

	if (_memicmp(Query, "APRST", 5) == 0 || _memicmp(Query, "PING?", 5) == 0)
	{
		// Trace Route
		//:KH2ZV   :?APRST :N8UR     :KH2Z>APRS,DIGI1,WIDE*:
		//:G8BPQ-14 :Path - G8BPQ-14>APU25N

		char Message[255];
	
		wsprintf(Message, ":%-9s:Path - %s>%s", Origin, Origin, DestPlusDigis);
		SendAPRSMessage(Message, Port);

		return;
	}
}

VOID ProcessQuery(char * Query)
{
	if (memcmp(Query, "IGATE?", 6) == 0)
	{
		IStatusCounter = (rand() & 31) + 5;			// 5 - 36 secs delay
		return;
	}

	if (memcmp(Query, "APRS?", 5) == 0)
	{
		BeaconCounter = (rand() & 31) + 5;			// 5 - 36 secs delay
		return;
	}
}
Dll VOID APIENTRY SendBeacon(int toPort, char * StatusText)
{
	int Port;
	DIGIMESSAGE Msg;
	char * StMsg = StatusText;
	int Len;

	if (StMsg == NULL)
		StMsg = StatusMsg;
	
	Len = wsprintf(Msg.L2DATA, "%c%s%c%s%c %s", (ApplConnected) ? '=' : '!',
		LAT, SYMSET, LON, SYMBOL, StMsg);
	Msg.PID = 0xf0;
	Msg.CTL = 3;

	if (toPort && BeaconHddrLen[toPort])
	{
		memcpy(Msg.DEST, &BeaconHeader[toPort][0][0],  BeaconHddrLen[toPort] + 1);
		Send_AX_Datagram(&Msg, Len + 2, toPort);

		return;
	}

	for (Port = 1; Port <= NUMBEROFPORTS; Port++)
	{
		if (BeaconHddrLen[Port])		// Only send to ports with a DEST defined
		{
			Len = wsprintf(Msg.L2DATA, "%c%s%c%s%c %s", (ApplConnected) ? '=' : '!',
					LAT, SYMSET, LON, SYMBOL, StMsg);
			Msg.PID = 0xf0;
			Msg.CTL = 3;

			memcpy(Msg.DEST, &BeaconHeader[Port][0][0],  BeaconHddrLen[Port] + 1);
			Send_AX_Datagram(&Msg, Len + 2, Port);
		}
	}

	// Also send to APRS-IS if connected

	if (APRSISOpen)
	{
		char ISMsg[300];

		Len = wsprintf(ISMsg, "%s>%s,TCPIP*:%c%s%c%s%c %s\r\n", APRSCall, APRSDest,
			(ApplConnected) ? '=' : '!', LAT, SYMSET, LON, SYMBOL, StMsg);
		send(sock, ISMsg, Len, 0);
		Debugprintf(">%s", ISMsg);

		IStatusCounter = 5;
	}

	// and to Application

	if (ApplConnected)
	{
		UINT * buffptr = GetBuff();
				
		if (buffptr)
		{
			buffptr[1] = wsprintf((char *)&buffptr[3], "xx xx xx  %s>%s <UI>:\r!%s%c%s%c %s\r\n",
				APRSCall, APRSDest, LAT, SYMSET, LON, SYMBOL, StMsg);
			buffptr[2] = 255;
			C_Q_ADD(&APPL_Q, buffptr);
		}
	}
}

VOID SendIStatus()
{
	int Port;
	DIGIMESSAGE Msg;
	int Len;

	if (APRSISOpen)
	{
		Msg.PID = 0xf0;
		Msg.CTL = 3;

		Len = wsprintf(Msg.L2DATA, "<IGATE,MSG_CNT=%d,LOC_CNT=%d", MessageCount , CountLocalStations());

		for (Port = 1; Port <= NUMBEROFPORTS; Port++)
		{
			if (BeaconHddrLen[Port])		// Only send to ports with a DEST defined
			{
				memcpy(Msg.DEST, &BeaconHeader[Port][0][0],  BeaconHddrLen[Port] + 1);
				Send_AX_Datagram(&Msg, Len + 2, Port);
			}
		}

		Len = wsprintf(Msg.L2DATA, "%s>%s,TCPIP*:<IGATE,MSG_CNT=%d,LOC_CNT=%d\r\n", APRSCall, APRSDest, 0, CountLocalStations());
		send(sock, Msg.L2DATA, Len, 0);
		Debugprintf(">%s", Msg.L2DATA);
	}
}

VOID DoSecTimer()
{
	if (ISPort && APRSISOpen == 0 && IGateEnabled)
	{
		ISDelayTimer++;

		if (ISDelayTimer > 60)
		{
			ISDelayTimer = 0;
			_beginthread(APRSISThread, 0, (VOID *) TRUE);
		}
	}

	if (BeaconCounter)
	{
		BeaconCounter--;

		if (BeaconCounter == 0)
		{
			BeaconCounter = BeaconInterval * 60;
			SendBeacon(0, StatusMsg);
		}
	}

	if (IStatusCounter)
	{
		IStatusCounter--;

		if (IStatusCounter == 0)
		{
			SendIStatus();
		}
	}

	if (GPSOK)
	{
		GPSOK--;
		if (GPSOK == 0)
			SetDlgItemText(hWnd, IDC_GPS, "No GPS");
	}
}

char APRSMsg[300];

VOID APRSISThread(BOOL Report)
{
	// Receive from core server

	char Signon[500];

	SOCKADDR_IN sinx; 
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;
	int len, err;
	u_long param=1;
	BOOL bcopt=TRUE;
	char Buffer[1000];
	int InputLen = 1;		// Non-zero
	char errmsg[100];
	char * ptr;
	int inptr = 0;
	char APRSinMsg[1000];

	Debugprintf("BPQ32 APRS IS Thread");
	SetDlgItemText(hWnd, IGATESTATE, "IGate State: Connecting");

	if (ISFilter[0])
		wsprintf(Signon, "user %s pass %d vers BPQ32 %s filter %s\r\n",
			APRSCall, ISPasscode, TextVerstring, ISFilter);
	else
		wsprintf(Signon, "user %s pass %d vers BPQ32 %s\r\n",
			APRSCall, ISPasscode, TextVerstring);

	// Resolve Name if needed

	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(ISPort);

	destaddr.sin_addr.s_addr = inet_addr(ISHost);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
	//	Resolve name to address

		HostEnt = gethostbyname(ISHost);
		 
		if (!HostEnt)
		{
			err = WSAGetLastError();

			wsprintf(errmsg, TEXT("APRS IS Resolve %s Failed %d\r\n"), ISHost, err);
			
			if (Report)
				WritetoConsole(errmsg);

			Debugprintf(errmsg);

			ISDelayTimer = -60;	// Delay longer if can't resolve

			return;			// Resolve failed
		}
		
		memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

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

		SetDlgItemText(hWnd, IGATESTATE, "IGate State: Connect Failed");

		return;
	}

	InputLen=recv(sock, Buffer, 5500, 0);

	if (InputLen > 0)
	{
		Buffer[InputLen] = 0;
		Debugprintf(Buffer);
	}

	send(sock, Signon, strlen(Signon), 0);

	InputLen=recv(sock, Buffer, 500, 0);

	if (InputLen > 0)
	{
		Buffer[InputLen] = 0;
		Debugprintf(Buffer);
	}
 
	InputLen=recv(sock, Buffer, 500, 0);

	if (InputLen > 0)
	{
		Buffer[InputLen] = 0;
		Debugprintf(Buffer);
	}

	APRSISOpen = TRUE;

	SetDlgItemText(hWnd, IGATESTATE, "IGate State: Connected");

	while (InputLen > 0 && IGateEnabled)
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

				if (len < 300)							// Ignore if way too long
				{
					memcpy(&APRSMsg, APRSinMsg, len);	
					APRSMsg[len - 2] = 0;

//					Debugprintf("<%s", APRSMsg);
					ProcessAPRSISMsg(APRSMsg);
				}

				inptr -= len;						// bytes left

				if (inptr > 0)
				{
					memmove(APRSinMsg, ptr, inptr);
					ptr = memchr(APRSinMsg, 0x0a, inptr);
				}
				else
					ptr = 0;

				if (inptr < 0)
					break;
			}
		}
	}

	closesocket(sock);

	APRSISOpen = FALSE;

	Debugprintf("BPQ32 APRS IS Thread Exited");

	if (IGateEnabled)
		SetDlgItemText(hWnd, IGATESTATE, "IGate State: Disconnected");
	else
		SetDlgItemText(hWnd, IGATESTATE, "IGate State: Disabled");

	ISDelayTimer = 30;		// Retry pretty quickly
	return;
}

VOID ProcessAPRSISMsg(char * APRSMsg)
{
	char * Payload;
	char * Source;
	char * Dest;
	char IGateCall[10] = "         ";
	char * ptr;
	char Message[255];
	struct APRSSTATIONRECORD * MH;
	time_t NOW = time(NULL);

	if (APRSMsg[0] == '#')		// Comment
		return;

	// if APRS Appl is atttached, queue message to it

	if (ApplConnected)
	{
		UINT * buffptr = GetBuff();
				
		if (buffptr)
		{
			buffptr[1] = strlen(APRSMsg);
			buffptr[2] = 0;
			memcpy(&buffptr[3], APRSMsg, buffptr[1]);
			C_Q_ADD(&APPL_Q, buffptr);
		}
	}

//}WB4APR-14>APRS,RELAY,TCPIP,G9RXG*::G3NRWVVVV:Hi Ian{001
//KE7XO-2>hg,TCPIP*,qAC,T2USASW::G8BPQ-14 :Path - G8BPQ-14>APU25N
//IGATECALL>APRS,GATEPATH}FROMCALL>TOCALL,TCPIP,IGATECALL*:original packet data
	
	Payload = strchr(APRSMsg, ':');

	// Gate call of originating Igate

	ptr = Payload;

	if (Payload == NULL)
		return;

	*(Payload++) = 0;

	while (ptr[0] != ',')
		ptr--;

	ptr++;

	memcpy(IGateCall, ptr, strlen(ptr));

	MH = LookupStation(IGateCall);

	if (MH)
		MH->IGate = TRUE;						// If we have seen this station on RF, set it as an Igate

	Source = APRSMsg;
	Dest = strchr(APRSMsg, '>');

	if (Dest == NULL)
		return;

	*(Dest++) = 0;				// Termainate Source
	ptr = strchr(Dest, ',');
	*ptr = 0;

	MH = UpdateHeard(Source, 0);

	// See if we should gate to RF. 

	// Have we heard dest recently? (use the message dest (not ax.25 dest) - does this mean we only gate Messages?
	// Not if it is an Igate (it will get a copy direct)
	// Have we recently sent a message from this call - if so, we gate the next Position

	if (Payload[0] == ':')		// Message
	{
		char MsgDest[10];
		struct APRSSTATIONRECORD * STN;

		memcpy(MsgDest, &Payload[1], 9);
		MsgDest[9] = 0;

		if (strcmp(MsgDest, CallPadded) == 0) // to us
			return;

		STN = LookupStation(MsgDest);

		if (STN && STN->Port && !STN->IGate && (NOW - STN->MHTIME) < GATETIMELIMIT) 
		{
			wsprintf(Message, "}%s>%s,TCPIP,%s*:%s", Source, Dest, APRSCall, Payload);
			SendAPRSMessage(Message, STN->Port);

			MH->LASTMSG = NOW;

			return;
		}
	}

	// Not a message. Only gate if have sent a message recently

	if ((NOW - MH->LASTMSG) < 900 && MH->Port)
	{
		wsprintf(Message, "}%s>%s,TCPIP,%s*:%s", Source, Dest, APRSCall, Payload);
		SendAPRSMessage(Message, -1);		// Send to all APRS Ports
	}
}

struct APRSSTATIONRECORD * LookupStation(char * Call)
{
	struct APRSSTATIONRECORD * MH = MHDATA;
	int i;

	// We keep call in ascii format, as that is what we get from APRS-IS, and we have it in that form

	for (i = 0; i < HEARDENTRIES; i++)
	{
		if (memcmp(Call, MH->MHCALL, 9) == 0)
			return MH;

		MH++;
	}

	return NULL;
}

struct APRSSTATIONRECORD * UpdateHeard(UCHAR * Call, int Port)
{
	struct APRSSTATIONRECORD * MH = MHDATA;
	struct APRSSTATIONRECORD * MHBASE = MH;
	int i;
	time_t NOW = time(NULL);
	time_t OLDEST = NOW - MAXAGE;
	char CallPadded[10] = "         ";
	BOOL SaveIGate = FALSE;
	time_t SaveLastMsg = 0;

	// We keep call in ascii format, space padded, as that is what we get from APRS-IS, and we have it in that form

	// Make Sure Space Padded

	memcpy(CallPadded, Call, strlen(Call));

	for (i = 0; i < MAXHEARDENTRIES; i++)
	{
		if (memcmp(CallPadded, MH->MHCALL, 10) == 0)
		{
			// if from APRS-IS, only update if record hasn't been heard via RF
			
			if (Port == 0 && MH->Port) 
				return MH;					// Don't update RF with IS

			if (Port == MH->Port)
			{
				SaveIGate = MH->IGate;
				SaveLastMsg = MH->LASTMSG;
				goto DoMove;
			}
		}

		if (MH->MHCALL[0] == 0 || MH->MHTIME < OLDEST)		// Spare entry
			goto DoMove;

		MH++;
	}

	//	TABLE FULL AND ENTRY NOT FOUND - MOVE DOWN ONE, AND ADD TO TOP

	i = MAXHEARDENTRIES - 1;
		
	// Move others down and add at front
DoMove:
	if (i != 0)				// First
		memmove(MHBASE + 1, MHBASE, i * sizeof(struct APRSSTATIONRECORD));

	if (i >= HEARDENTRIES) 
	{
		char Status[80];
	
		HEARDENTRIES = i + 1;

		wsprintf(Status, "IGATE Stats: Msgs %d  Local Stns %d", MessageCount , CountLocalStations());
		SetDlgItemText(hWnd, IGATESTATS, Status);
	}

	memcpy (MHBASE->MHCALL, CallPadded, 10);
	MHBASE->Port = Port;
	MHBASE->MHTIME = NOW;
	MHBASE->IGate = SaveIGate;
	MHBASE->LASTMSG = SaveLastMsg;

	return MHBASE;
}

int CountLocalStations()
{
	struct APRSSTATIONRECORD * MH = MHDATA;
	int i, n = 0;

	// We keep call in ascii format, as that is what we get from APRS-IS, and we have it in that form

	for (i = 0; i < HEARDENTRIES; i++)
	{
		if (MH->Port)			// DOn't count IS Stations
			n++;

		MH++;
	}

	return n;
}


BOOL CheckforDups(char * Call, char * Msg, int Len)
{
	// Primitive duplicate suppression - see if same call and text reeived in last few seconds
	
	time_t Now = time(NULL);
	time_t DupCheck = Now - DUPSECONDS;
	int i, saveindex = -1;
	char * ptr1;

	for (i = 0; i < MAXDUPS; i++)
	{
		if (DupInfo[i].DupTime < DupCheck)
		{
			// too old - use first if we need to save it 

			if (saveindex == -1)
			{
				saveindex = i;
			}

			if (DupInfo[i].DupTime == 0)		// Off end of used area
				break;

			continue;	
		}

		if ((Len == DupInfo[i].DupLen || (DupInfo[i].DupLen == 99 && Len > 99)) && memcmp(Call, DupInfo[i].DupUser, 7) == 0 && (memcmp(Msg, DupInfo[i].DupText, DupInfo[i].DupLen) == 0))
		{
			// Duplicate, so discard

			Msg[Len] = 0;
			ptr1 = strchr(Msg, 13);
			if (ptr1)
				*ptr1 = 0;

			Debugprintf("Duplicate Message supressed %s", Msg);
			return TRUE;					// Duplicate
		}
	}

	// Not in list

	if (saveindex == -1)  // List is full
		saveindex = MAXDUPS - 1;	// Stick on end	

	DupInfo[saveindex].DupTime = Now;
	memcpy(DupInfo[saveindex].DupUser, Call, 7);

	if (Len > 99) Len = 99;

	DupInfo[saveindex].DupLen = Len;
	memcpy(DupInfo[saveindex].DupText, Msg, Len);

	return FALSE;
}

char * FormatAPRSMH(struct APRSSTATIONRECORD * MH)
 {
	 // Called from CMD.ASM

	struct tm * TM;
	static char MHLine[50];
	time_t szClock = MH->MHTIME;

	szClock = (_time32(NULL) - szClock);
	TM = gmtime(&szClock);

	wsprintf(MHLine, "%-10s %d %.2d:%.2d:%.2d:%.2d %s\r",
		MH->MHCALL, MH->Port, TM->tm_yday, TM->tm_hour, TM->tm_min, TM->tm_sec, (MH->IGate) ? "Igate" : "");

	return MHLine;

 }

// GPS Handling Code

void SelectSource(BOOL Recovering);
void DecodeRMC(char * msg, int len);

void PollGPSIn();

struct PortInfo
{ 
	int Index;
	int ComPort;
	char PortType[2];
	BOOL NewVCOM;				// Using User Mode Virtual COM Driver
	int ReopenTimer;			// Retry if open failed delay
	int RTS;
	int CTS;
	int DCD;
	int DTR;
	int DSR;
	char Params[20];				// Init Params (eg 9600,n,8)
	char PortLabel[20];
	HANDLE hDevice;
	BOOL Created;
	BOOL PortEnabled;
	int LastError;
	int FLOWCTRL;
	int gpsinptr;
	OVERLAPPED Overlapped;
	OVERLAPPED OverlappedRead;
	char GPSinMsg[160];
	int GPSTypeFlag;					// GPS Source flags
	BOOL RMCOnly;						// Only send RMC msgs to this port
};



struct PortInfo InPorts[1] = {0};

UINT GPSType = 0xffff;		// Source of Postion info - 1 = Phillips 2 = AIT1000. ffff = not posn message

int RecoveryTimer;			// Serial Port recovery

double PI = 3.1415926535;
double P2 = 3.1415926535 / 180;

double Latitude, Longtitude, SOG, COG, LatIncrement, LongIncrement;
double LastSOG = -1.0;

BOOL Check0183CheckSum(char * msg,int len)
{
	BOOL retcode=TRUE;
	char * ptr;
	UCHAR sum,xsum1,xsum2;

	sum=0;
	ptr=++msg;	//	Skip $

loop:

	if (*(ptr)=='*') goto eom;
	
	sum ^=*(ptr++);

	len--;

	if (len > 0) goto loop;

	return TRUE;		// No Checksum

eom:

	xsum1=*(++ptr);
	xsum1-=0x30;
	if (xsum1 > 9) xsum1-=7;

	xsum2=*(++ptr);
	xsum2-=0x30;
	if (xsum2 > 9) xsum2-=7;

	xsum1=xsum1<<4;
	xsum1+=xsum2;

	return (xsum1==sum);
}

BOOL OpenGPSPort()
{
	char szPort[15];
	BOOL fRetVal ;
	COMMTIMEOUTS CommTimeOuts ;
	struct PortInfo * portptr = &InPorts[0];
	DCB	dcb;
	
	wsprintf(szPort, "//./COM%d", GPSPort) ;

	// open COMM device

	portptr->hDevice =
      CreateFile( szPort, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, 
                  NULL );
				  
	if (portptr->hDevice == (HANDLE) -1)
	{
		portptr->LastError=GetLastError();
		return FALSE;
	}

    // setup device buffers

      SetupComm(portptr->hDevice, 4096, 4096 ) ;

      // purge any information in the buffer

      PurgeComm(portptr->hDevice, PURGE_TXABORT | PURGE_RXABORT |
                                      PURGE_TXCLEAR | PURGE_RXCLEAR ) ;

      // set up for overlapped I/O
	  
      CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF ;
      CommTimeOuts.ReadTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.ReadTotalTimeoutConstant = 0 ;
      CommTimeOuts.WriteTotalTimeoutMultiplier = 0 ;
      CommTimeOuts.WriteTotalTimeoutConstant = 0 ;
      SetCommTimeouts(portptr->hDevice, &CommTimeOuts ) ;

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
	
	dcb.DCBlength = sizeof(DCB);
	GetCommState(portptr->hDevice, &dcb);

	 // setup hardware flow control

	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;

	dcb.BaudRate = GPSSpeed;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;

	dcb.fInX = dcb.fOutX = 0;
	dcb.XonChar = 0;
	dcb.XoffChar = 0;
	dcb.XonLim = 0;
	dcb.XoffLim = 0;

	// other various settings

	dcb.fBinary = TRUE;
	dcb.fParity = TRUE;

	fRetVal = SetCommState(portptr->hDevice, &dcb);

//	conn->RTS = 1;
//	conn->DTR = 1;

	EscapeCommFunction(portptr->hDevice,SETDTR);
	EscapeCommFunction(portptr->hDevice,SETRTS);
	
	return fRetVal;
}

void PollGPSIn()
{
	int len;
	char GPSMsg[2000];
	char * ptr;
	struct PortInfo * portptr;
	COMSTAT    ComStat ;
	DWORD      dwErrorFlags;
	DWORD      dwLength;

	if (RecoveryTimer)
	{
		RecoveryTimer--;
		if (RecoveryTimer == 0)
		{
			SelectSource(TRUE);		// Try to re-open ports
		}
	}

	portptr = &InPorts[0];
	
	if (!portptr->hDevice)
		return;

	getgpsin:

		// only try to read number of bytes in queue 
	
		len=0;

		if (!ClearCommError(portptr->hDevice, &dwErrorFlags, &ComStat))
		{
			// Comm Error - probably lost USB Port. Try closing and reopening after a delay

			if (RecoveryTimer == 0)
			{
				RecoveryTimer = 100;			// 10 Secs
				return;
			}
		}

		dwLength = min(80, ComStat.cbInQue ) ;

		if (portptr->gpsinptr + dwLength > 160)	// Corrupt data 
			portptr->gpsinptr = 0;

		if (dwLength > 0)
		{
			memset(&portptr->OverlappedRead, 0, sizeof(portptr->OverlappedRead));
			ReadFile(portptr->hDevice, &portptr->GPSinMsg [portptr->gpsinptr] ,dwLength,&len,&portptr->OverlappedRead);
		}
		if (len > 0)
		{
			portptr->gpsinptr+=len;

			ptr = memchr(portptr->GPSinMsg, 0x0a, portptr->gpsinptr);

			while (ptr != NULL)
			{
				ptr++;									// include lf
				len=ptr-(char *)&portptr->GPSinMsg;					
				memcpy(&GPSMsg,portptr->GPSinMsg,len);	

				GPSMsg[len] = 0;

				if (Check0183CheckSum(GPSMsg, len))
					if (memcmp(&GPSMsg[1], "GPRMC", 5) == 0)
						DecodeRMC(GPSMsg, len);	

				portptr->gpsinptr-=len;			// bytes left

				if (portptr->gpsinptr > 0)
				{
					memmove(portptr->GPSinMsg,ptr, portptr->gpsinptr);
					ptr = memchr(portptr->GPSinMsg, 0x0a, portptr->gpsinptr);
				}
				else
					ptr=0;
			}
			
			goto getgpsin;
	}
	
	return;
}


void ClosePorts()
{
	if (InPorts[0].hDevice)
	{
		CloseHandle(InPorts[0].hDevice);
		InPorts[0].hDevice=0;
	}

	return;
}

void SelectSource(BOOL Recovering)
{
	struct PortInfo * portptr;

	RecoveryTimer = 0;
	
	if (InPorts[0].hDevice)
	{
		CloseHandle(InPorts[0].hDevice);
		InPorts[0].hDevice=0;
	}
    portptr = &InPorts[0];
			
	if (portptr->PortEnabled)
	{                
		if (OpenGPSPort())
		{
			portptr->RTS = 0;
			portptr->DTR = 0;
			EscapeCommFunction(portptr->hDevice,SETDTR);
			EscapeCommFunction(portptr->hDevice,SETRTS);
		}
		else
		{
			if (Recovering)
			{
				RecoveryTimer = 100;			// 10 Secs
			}
		}       
	}
	return;
}


void DecodeRMC(char * msg, int len)
{
	char * ptr1;
	char * ptr2;
	char TimHH[3], TimMM[3], TimSS[3];
	char OurSog[5], OurCog[4];
	char LatDeg[3], LonDeg[4];

	char Day[3];

	ptr1 = &msg[7];
        
	len-=7;
	
	ptr2=(char *)memchr(ptr1,',',15);

	if (ptr2 == 0) return;	// Duff

	*(ptr2++)=0;

	memcpy(TimHH,ptr1,2);
	memcpy(TimMM,ptr1+2,2);
	memcpy(TimSS,ptr1+4,2);
	TimHH[2]=0;
	TimMM[2]=0;
	TimSS[2]=0;

	ptr1=ptr2;
	
	if (*(ptr1) != 'A') // ' Data Not Valid
	{
		SetDlgItemText(hWnd, IDC_GPS, "No GPS Fix");
		return;
	}
        
	ptr1+=2;

	ptr2=(char *)memchr(ptr1,',',15);
		
	if (ptr2 == 0) return;	// Duff

	*(ptr2++)=0;
 
	memcpy(LAT, ptr1 , 7);
	memcpy(LatDeg,ptr1,2);
	LatDeg[2]=0;
	Lat=atof(LatDeg) + (atof(ptr1+2)/60);
	
	if (*(ptr1+7) > '4') if (LAT[6] < '9') LAT[6]++;

	ptr1=ptr2;

	LAT[7] = (*ptr1);
	if ((*ptr1) == 'S') Lat=-Lat;
	
	ptr1+=2;

	ptr2=(char *)memchr(ptr1,',',15);
		
	if (ptr2 == 0) return;	// Duff
	*(ptr2++)=0;

	memcpy(LON, ptr1, 8);
	
	memcpy(LonDeg,ptr1,3);
	LonDeg[3]=0;
	Lon=atof(LonDeg) + (atof(ptr1+3)/60);
       
	if (*(ptr1+8) > '4') if (LON[7] < '9') LON[7]++;

	ptr1=ptr2;

	LON[8] = (*ptr1);
	if ((*ptr1) == 'W') Lon=-Lon;

	// Now have a valid posn, so stp sending Undefined LOC Sysbol
	
	SYMBOL = CFGSYMBOL;
	SYMSET = CFGSYMSET;

	if (GPSOK == 0)
	{
		GPSOK = 30;
		SetDlgItemText(hWnd, IDC_GPS, "GPS OK");
	}

	ptr1+=2;

	ptr2 = (char *)memchr(ptr1,',',15);
	
	if (ptr2 == 0) return;	// Duff

	*(ptr2++)=0;

	memcpy(OurSog, ptr1, 4);
	OurSog[4] = 0;

	ptr1=ptr2;

	ptr2 = (char *)memchr(ptr1,',',15);
	
	if (ptr2 == 0) return;	// Duff

	*(ptr2++)=0;

	memcpy(OurCog, ptr1, 3);
	OurCog[3] = 0;

	memcpy(Day,ptr2,2);
	Day[2]=0;
}

VOID SendFilterCommand()
{
	char Msg[2000];
	int n;

	n = wsprintf(Msg, ":%-9s:filter %s{1", "SERVER", "m/0");

	if (ISFilter[0])
		n = wsprintf(Msg, ":%-9s:filter %s{1", "SERVER", ISFilter);

	PutAPRSMessage(Msg, n);

	n = wsprintf(Msg, ":%-9s:filter?{1", "SERVER");
	PutAPRSMessage(Msg, n);
}


Dll VOID APIENTRY APRSConnect(char * Call, char * Filter)
{
	// Request APRS Data from Switch (called by APRS Applications)

	ApplConnected = TRUE;

	strcpy(APPLFilter, Filter);

	if (APPLFilter[0])
	{
		strcpy(ISFilter, APPLFilter);
		SendFilterCommand();
	}
	strcpy(Call, CallPadded);
}

Dll VOID APIENTRY APRSDisconnect()
{
	// Stop requesting APRS Data from Switch (called by APRS Applications)

	UINT * buffptr;

	ApplConnected = FALSE;

	strcpy(ISFilter, NodeFilter);

	SendFilterCommand();

	while (APPL_Q)
	{
		buffptr = Q_REM(&APPL_Q);
		ReleaseBuffer(buffptr);
	}

}

Dll BOOL APIENTRY GetAPRSFrame(char * Frame, int * Len, int * Port)
{
	// Request APRS Data from Switch (called by APRS Applications)

	UINT * buffptr;
	struct _EXCEPTION_POINTERS exinfo;

	GetSemaphore();

	SemHeldByAPI = 10;

	__try 
	{
		if (APPL_Q)
		{
			buffptr = Q_REM(&APPL_Q);

			*Len = buffptr[1];
			*Port = buffptr[2];

			if (buffptr[1] > 400 || buffptr[1] < 0)
			{
				Debugprintf ("Corrupt APRS Frame Len = %d",  buffptr[1]);
				ReleaseBuffer(buffptr);
				FreeSemaphore();
				return FALSE;
			}

			memcpy(Frame, &buffptr[3], buffptr[1]);
			ReleaseBuffer(buffptr);
			FreeSemaphore();
			return TRUE;
		}
	}

	#define EXCEPTMSG "GetAPRSFrame"
	#include "StdExcept.c"
	}
	FreeSemaphore();

	return FALSE;
}

Dll BOOL APIENTRY PutAPRSFrame(char * Frame, int Len, int Port)
{
	SendAPRSMessage(Frame, Port);
	return TRUE;
}
Dll BOOL APIENTRY PutAPRSMessage(char * Frame, int Len)
{
	// Called from BPQAPRS App
	// Message has to be queued so it can be sent by Timer Process (IS sock is not valid in this context)

	UINT * buffptr;

	MessageCount++;

	buffptr = GetBuff();

	GetSemaphore();
	SemHeldByAPI = 11;

	if (buffptr)
	{
		buffptr[1] = ++Len;					// Len doesn't include Null
		memcpy(&buffptr[2], Frame, Len);
		C_Q_ADD(&APPLTX_Q, buffptr);
	}

	FreeSemaphore();

	return TRUE;
}

BOOL SendAPPLAPRSMessage(char * Frame)
{
	// Send an APRS Message from Appl Queue. If call has been heard,
	// send to the port it was heard on,
	// otherwise send to all ports (including IS). Messages to SERVER only go to IS

	struct APRSSTATIONRECORD * STN;
	char ToCall[10] = "";
	
	memcpy(ToCall, &Frame[1], 9);

	if (_stricmp(ToCall, "SERVER   ") == 0)
	{
		SendAPRSMessage(Frame, 0);			// IS
		return TRUE;
	}
	
	STN = LookupStation(ToCall);

	if (STN)
		SendAPRSMessage(Frame, STN->Port);
	else
	{
		SendAPRSMessage(Frame, -1);			// All RF ports
		SendAPRSMessage(Frame, 0);			// IS
	}
	
	return TRUE;
}

Dll BOOL APIENTRY GetAPRSLatLon(double * PLat,  double * PLon)
{
	*PLat = Lat;
	*PLon = Lon;

	return GPSOK;
}

Dll BOOL APIENTRY GetAPRSLatLonString(char * PLat,  char * PLon)
{
	strcpy(PLat, LAT);
	strcpy(PLon, LON);

	return GPSOK;
}

