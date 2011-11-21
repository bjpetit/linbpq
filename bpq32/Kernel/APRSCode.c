
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

#define MAXAGE 7200		// 2 Hours
#define MAXCALLS 20		// Max Flood, Trace and Digi
#define GATETIMELIMIT 1800 // Don't gate to RF if station not heard for this time

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
VOID SendBeacon(int toPort);
VOID ProcessAPRSISMsg(char * APRSMsg);
static VOID SendtoDigiPorts(PDIGIMESSAGE Block, DWORD Len, UCHAR Port);
struct APRSSTATIONRECORD * LookupStation(char * call);


BOOL ProcessConfig();
BOOL FreeConfig();

// All data should be initialised to force into shared segment

static char ConfigClassName[]="CONFIG";

extern struct BPQVECSTRUC APRSMONVEC;
extern int MONDECODE();
extern VOID * zalloc(int len);
extern BOOL StartMinimized;
extern BOOL MinimizetoTray;
extern char * PortConfig[];

extern short NUMBEROFPORTS;

extern byte	MCOM;
extern char	MTX;
extern ULONG MMASK;


static int SecTimer = 10;

UINT APRSPortMask = 0;

char * APRSCall = NULL;
UCHAR AXCall[7] = "";

char CallPadded[10] = "         ";

int GPSPort = 0;
int GPSSpeed = 0;

char LAT[] = "0000.00N";	// in standard APRS Format      
char LON[] = "00000.00W";	//in standard APRS Format

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

short ISPort = 0;
char ISHost[256] = "";
int ISPasscode = 0;

char * StatusMsg = 0;
int StatusMsgLen = 0;

char * BeaconPath[33] = {0};

char CrossPortMap[33][33] = {0};

UCHAR BeaconHeader[33][10][7] = {""};	//	Dest, Source and up to 8 digis 
int BeaconHddrLen[33] = {0};			// Actual Length used

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

// Heard Station info

int HEARDENTRIES = 0;
int MAXHEARDENTRIES = 0;
int MHLEN = sizeof(struct APRSSTATIONRECORD);

// Area is allocated as needed

struct APRSSTATIONRECORD * MHDATA = NULL;

SOCKET sock = (SOCKET) NULL;

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

	ConvToAX25(GetNodeCall(), MYCALL);

	ConvToAX25("TCPIP", axTCPIP);
	ConvToAX25("RFONLY", axRFONLY);
	ConvToAX25("NOGATE", axNOGATE);

	// Clear in case reconfig

	if (FloodCalls)
		{free(FloodCalls); FloodCalls = NULL;}
	if (TraceCalls)
		{free(TraceCalls); TraceCalls = NULL;}
	if (DigiCalls)
		{free(TraceCalls); TraceCalls = NULL;}

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
	MAXHEARDENTRIES = 50;

	MHDATA = zalloc(MAXHEARDENTRIES * sizeof(struct APRSSTATIONRECORD));

	APRSMONVEC.HOSTAPPLFLAGS = 0x80;		// Request Monitoring

	if (ISPort)
		_beginthread(APRSISThread, 0, (VOID *) TRUE);

	WritetoConsole("\nAPRS Digi/Gateway Enabled\n");
	return TRUE;
}

#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

VOID APRSClose()
{
	free (MHDATA);

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
		// Except General Queries, and Messages Addressed to us

		// or should we process Query frames locally ??

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
			len = wsprintf(ISMsg, "%s>%s,qAC,%s:%s", ptr1, ptr4, APRSCall, Payload);

			send(sock, ISMsg, len, 0);
	
			ptr1 = strchr(ISMsg, 13);
			if (ptr1) *ptr1 = 0;
			Debugprintf(ISMsg);
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
			default:

				// Not to an APRS Destination
			
				ReleaseBuffer(monbuff);
				continue;
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
		
//	DigiAX = zalloc(50);

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
	int i;

	ptr = strtok(buf, "= \t\n\r");

	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment


	if (_stricmp(ptr, "STATUSMSG") == 0)
	{
		p_value = strtok(NULL, ";\t\n\r");
		StatusMsg = _strdup(p_value);
		StatusMsgLen = strlen(p_value);
		return TRUE;
	}

	p_value = strtok(NULL, " \t\n\r");

	if (p_value == NULL)
		return FALSE;

	if (_stricmp(ptr,"APRSPorts") == 0)
	{
		char * p_port = strtok(p_value, " ,\t\n\r");
		
		while (p_port != NULL)
		{
			i=atoi(p_port);
			if (i == 0) return FALSE;
			if (i > NUMBEROFPORTS) return FALSE;

			APRSPortMask |= 1 << (i-1);
			p_port = strtok(NULL, " ,\t\n\r");
		}
		return (TRUE);
	}

	if (_stricmp(ptr, "APRSCALL") == 0)
	{
		APRSCall = _strdup(_strupr(p_value));
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

		if (Port < 1 || Port > NUMBEROFPORTS)
			return FALSE;

		APRSPortMask |= 1 << (Port - 1);

		if (Context == NULL)
			return TRUE;					// No dest - a receive-only port

		BeaconPath[Port] = _strdup(_strupr(Context));
	
		ptr = strtok_s(NULL, ",\t\n\r", &Context);

		if (ptr == NULL)
			return FALSE;

		ConvToAX25(APRSCall, &BeaconHeader[Port][1][0]);

		ConvToAX25(ptr, &BeaconHeader[Port][0][0]);			// First is Dest
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

		if (Port < 1 || Port > NUMBEROFPORTS)
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

		if (BeaconInterval)
			BeaconCounter = 60;				// Send first after 30 secs
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
		DigiCalls = _strdup(_strupr(p_value));
		ConvertCalls(DigiCalls, &DigiAX[0][0], &DigiLen[0]);
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
		SYMBOL = p_value[0];
		return TRUE;
	}

	if (_stricmp(ptr, "SYMSET") == 0)
	{
		SYMSET = p_value[0];
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



	if (toPort && BeaconHddrLen[toPort])
	{
		memcpy(Msg.DEST, &BeaconHeader[toPort][0][0],  BeaconHddrLen[toPort] + 1);
		Msg.PID = 0xf0;
		Msg.CTL = 3;
		Len = wsprintf(Msg.L2DATA, "%s", Message);
		Send_AX_Datagram(&Msg, Len + 2, toPort);

		return;
	}

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
}


VOID ProcessSpecificQuery(char * Query, int Port, char * Origin, char * DestPlusDigis)
{
	if (memcmp(Query, "APRSS", 5) == 0)
	{
		SendBeacon(Port);
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
VOID SendBeacon(int toPort)
{
	int Port;
	DIGIMESSAGE Msg;
	
	int Len;

	Len = wsprintf(Msg.L2DATA, "!%s%c%s%c %s", LAT, SYMSET, LON, SYMBOL, StatusMsg);
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
		
			Len = wsprintf(Msg.L2DATA, "!%s%c%s%c %s", LAT, SYMSET, LON, SYMBOL, StatusMsg);
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

		Len = wsprintf(ISMsg, "%s>APRS,TCPIP*:!%s%c%s%c %s\r\n", APRSCall, LAT, SYMSET, LON, SYMBOL, StatusMsg);
		send(sock, ISMsg, Len, 0);
		Debugprintf(ISMsg);

		IStatusCounter = 5;
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

		Len = wsprintf(Msg.L2DATA, "<IGATE,MSG_CNT=%d,LOC_CNT=%d", 0 , HEARDENTRIES);

		for (Port = 1; Port <= NUMBEROFPORTS; Port++)
		{
			if (BeaconHddrLen[Port])		// Only send to ports with a DEST defined
			{
				memcpy(Msg.DEST, &BeaconHeader[Port][0][0],  BeaconHddrLen[Port] + 1);
				Send_AX_Datagram(&Msg, Len + 2, Port);
			}
		}

		Len = wsprintf(Msg.L2DATA, "%s>APRS,TCPIP*:<IGATE,MSG_CNT=%d,LOC_CNT=%d\r\n", APRSCall, 0 , HEARDENTRIES);
		send(sock, Msg.L2DATA, Len, 0);
		Debugprintf(Msg.L2DATA);
	}
}

VOID DoSecTimer()
{
	if (ISPort && APRSISOpen == 0)
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
			SendBeacon(0);
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
}

char APRSMsg[300];

VOID APRSISThread(BOOL Report)
{
	// Receive from core server

	char Signon[100];

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

	wsprintf(Signon, "user %s pass %d vers BPQ32 2011/08/20\r\n",
			APRSCall, ISPasscode);

	// Resolve Name if needed

	destaddr.sin_family = AF_INET; 
	destaddr.sin_port = htons(ISPort);

	destaddr.sin_addr.s_addr = inet_addr(ISHost);

	if (destaddr.sin_addr.s_addr == INADDR_NONE)
	{
	//	Resolve name to address

		HostEnt = gethostbyname (ISHost);
		 
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

				Debugprintf(APRSMsg);
				ProcessAPRSISMsg(APRSMsg);

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


//}WB4APR-14>APRS,RELAY,TCPIP,G9RXG*::G3NRWVVVV:Hi Ian{001
//KE7XO-2>hg,TCPIP*,qAC,T2USASW::G8BPQ-14 :Path - G8BPQ-14>APU25N
//IGATECALL>APRS,GATEPATH}FROMCALL>TOCALL,TCPIP,IGATECALL*:original packet data

	if (APRSMsg[0] == '#')		// Comment
		return;
	
	Payload = strchr(APRSMsg, ':');

	// Gate call of originating Igate

	ptr = Payload;
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

	if ((NOW - MH->LASTMSG) < 900)
	{
		wsprintf(Message, "}%s>%s,TCPIP,%s*:%s", Source, Dest, APRSCall, Payload);
		SendAPRSMessage(Message, 0);		// Send to all APRS Ports
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

	// We keep call in ascii format, space padded, as that is what we get from APRS-IS, and we have it in that form

	for (i = 0; i < MAXHEARDENTRIES; i++)
	{
		if (memcmp(Call, MH->MHCALL, 10) == 0)
		{
			// if from APRS-IS, only update if record hasn't been heard via RF
			
			if (Port == 0 && MH->Port) 
				return MH;					// Don't update RF with IS

			if (Port == MH->Port)
				goto DoMove;
		}

		if (MH->MHCALL[0] == 0)		// Spare entry
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
		HEARDENTRIES = i + 1;

	memcpy (MHBASE->MHCALL, Call, 10);
	MHBASE->Port = Port;
	MHBASE->MHTIME = NOW;

	return MH;
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

		if (Len == DupInfo[i].DupLen && memcmp(Call, DupInfo[i].DupUser, 7) == 0 && (memcmp(Msg, DupInfo[i].DupText, DupInfo[i].DupLen) == 0))
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

	wsprintf(MHLine, "%s %d %.2d:%.2d:%.2d:%.2d %s\r",
		MH->MHCALL, MH->Port, TM->tm_yday, TM->tm_hour, TM->tm_min, TM->tm_sec, (MH->IGate) ? "Igate" : "");

	return MHLine;

 }