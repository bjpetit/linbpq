// ChatForward.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <time.h>

#ifdef WIN32
#include "winsock.h"
#define ioctl ioctlsocket
#else

#define SOCKET int
#define BOOL int
#define TRUE 1
#define FALSE 0

#define strtok_s strtok_r

#define _memicmp memicmp
#define _stricmp stricmp
#define _strdup strdup
#define _strupr strupr
#define _strlwr strlwr
#define WSAGetLastError() errno
#define GetLastError() errno 

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>
#include <string.h>

#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>

#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <syslog.h>
#include <pthread.h>
#include <stdint.h>

char * strupr(char* s)
{
  char* p = s;

  if (s == 0)
	  return 0;

  while (*p = toupper( *p )) p++;
  return s;
}

char * strlwr(char* s)
{
  char* p = s;
  while (*p = tolower( *p )) p++;
  return s;
}

#endif

struct ChatNodeData
{
	char Callsign[10];
	char NAlias[10];
	double Lat;
	double Lon;
	int PopupMode;
	char Comment[256];
	int Count;
	int Deleted;
	int KillTimer;
};

struct ChatLink
{
	char Call1[10];
	char Call2[10];
	int Call1State;
	int Call2State;
	int Timeout1;
	int Timeout2;
	int KillTimer;
};

struct ChatNodeData ** ChatNodes = NULL;

struct NodeData
{
	char Call[10];
	double Lat;
	double Lon;
	int PopupMode;
	char Comment[256];
	time_t LastHeard;
};

struct NodeData ** Nodes = NULL;

int NumberOfNodes = 0;


struct NodeLink
{
	struct NodeData * Call1;
	struct NodeData * Call2;
	int Type;
	time_t LastHeard;
};

struct NodeLink ** NodeLinks = NULL;

int NumberOfNodeLinks = 0;


SOCKET sock;

time_t LastUpdate = 0;


int ConvFromAX25(unsigned char * incall, char * outcall);
void GenerateNodeLinks(time_t Now);
void UpdateHeardData(struct NodeData * Node, char * Call, char * Freq, char * LOC, char * Flags);



struct NodeData * FindNode(char * Call)
{
	struct NodeData * Node;
	int i;

	// Find, and if not found add

	for (i = 0; i < NumberOfNodes; i++)
	{
		if (strcmp(Nodes[i]->Call, Call) == 0)
			return Nodes[i];
	}
	
	Node = malloc(sizeof(struct NodeData));
	memset (Node, 0, sizeof(struct NodeData))
;
	Nodes = realloc(Nodes, (NumberOfNodes + 1) * sizeof(void *));
	Nodes[NumberOfNodes++] = Node;

	strcpy(Node->Call, Call);
	strcpy(Node->Comment, Call);

	return Node;
}

struct NodeLink * FindLink(char * Call1, char * Call2, int Type)
{
	struct NodeLink * Link;
	struct NodeData * Node;

	int i;

	// Find, and if not found add

	for (i = 0; i < NumberOfNodeLinks; i++)
	{
		// We only want one copy, whichever end it is reported from

		Link = NodeLinks[i];

		if (strcmp(Link->Call1->Call, Call1) == 0 && Link->Type == Type && strcmp(Link->Call2->Call, Call2) == 0)
			return Link;

		if (strcmp(Link->Call1->Call, Call2) == 0 && Link->Type == Type && strcmp(Link->Call2->Call, Call1) == 0)
			return Link;
	}
	
	Link = malloc(sizeof(struct NodeLink));
	memset (Link, 0, sizeof(struct NodeLink))
;
	NodeLinks = realloc(NodeLinks, (NumberOfNodeLinks + 1) * sizeof(void *));
	NodeLinks[NumberOfNodeLinks++] = Link;

	
	Node = FindNode(Call1);
	Link->Call1 = Node;
	Node = FindNode(Call2);
	Link->Call2 = Node;
	Link->Type = Type;

	return Link;
}

int FromLOC(char * Locator, double * pLat, double * pLon)
{
	double i;
	double Lat, Lon;

	_strupr(Locator);

	*pLon = 0;
	*pLat = 0;			// in case invalid


	// Basic validation for APRS positions

	// The first pair (a field) encodes with base 18 and the letters "A" to "R".
	// The second pair (square) encodes with base 10 and the digits "0" to "9".
	// The third pair (subsquare) encodes with base 24 and the letters "a" to "x".

	i = Locator[0];

	if (i < 'A' || i > 'R')
		return 0;

	Lon = (i - 65) * 20;

	i = Locator[2];
	if (i < '0' || i > '9')
		return 0;

	Lon = Lon + (i - 48) * 2;

	i = Locator[4];
	if (i < 'A' || i > 'X')
		return 0;

	Lon = Lon + (i - 65) / 12;

	i = Locator[1];
	if (i < 'A' || i > 'R')
		return 0;

	Lat = (i - 65) * 10;

	i = Locator[3];
	if (i < '0' || i > '9')
		return 0;

	Lat = Lat + (i - 48);

	i = Locator[5];
	if (i < 'A' || i > 'X')
		return 0;

	Lat = Lat + (i - 65) / 24;

	if (Lon < 0 || Lon > 360)
		Lon = 180;
	if (Lat < 0 || Lat > 180)
		Lat = 90;

	*pLon = Lon - 180;
	*pLat = Lat - 90;

	return 1;
}

char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr;
	
	if (buf == NULL) return NULL;		// Protect
	
	ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;
	return ptr;
}

FILE * logFile;

time_t Now;

int main(int argc, char * argv[])
{
	char RXBUFFER[512];
	struct NodeData * Node;
	struct NodeLink * Link;
	char * Msg = &RXBUFFER[16];

	struct sockaddr_in rxaddr, txaddr, txaddr2;
	int addrlen = sizeof(struct sockaddr_in);
	u_long param=1;
	BOOL bcopt=TRUE;
	struct sockaddr_in sinx;
	int nLength;
	
	struct hostent * HostEnt2;
	char * ptr, *Context;
	FILE * pFile;
	char Line[512];

	char * pCall;
	char * pCall2;
	char * pComment;
	char * pLat, * pLon;
	char * pPopupMode;
	char * pLastHeard;
	char * pType;

#ifdef WIN32
	WSADATA WsaData;            // receives data from WSAStartup
	WSAStartup(MAKEWORD(2, 0), &WsaData);
#endif

	logFile = fopen("nodelog.txt","ab");

	// Read Saved Nodes

	pFile = fopen("savenodes.txt","r");

	while (pFile)
	{
		if (fgets(Line, 499, pFile) == NULL)
		{
			fclose(pFile);
			pFile = NULL;
		}
		else
		{
			pCall = strtok_s(Line, ",", &Context); 
			pLat = strtok_s(NULL, ",", &Context); 
			pLon = strtok_s(NULL, ",", &Context); 
			pPopupMode = strtok_s(NULL, ",", &Context); 
			pComment = strtok_s(NULL, "|", &Context); 
			pLastHeard = strtok_s(NULL, ",", &Context); 

			if (pLastHeard == NULL)
				continue;

			Node = FindNode(pCall);
			Node->Lat = atof(pLat);
			Node->Lon = atof(pLon);
			Node->PopupMode = atoi(pPopupMode);
			Node->LastHeard = atoi(pLastHeard);
			strcpy(Node->Comment, pComment);
		}
	}


	pFile = fopen("savelinks.txt","r");

	while (pFile)
	{
		if (fgets(Line, 499, pFile) == NULL)
		{
			fclose(pFile);
			pFile = NULL;
		}
		else
		{
			pCall = strtok_s(Line, ",", &Context); 
			pCall2 = strtok_s(NULL, ",", &Context); 
			pType = strtok_s(NULL, ",", &Context); 
			pLastHeard = strtok_s(NULL, ",", &Context); 

			if (pLastHeard == NULL)
				continue;

			Link = FindLink(pCall, pCall2, atoi(pType));
			Link->LastHeard = atoi(pLastHeard);
		}
	}

	Now = time(NULL);
	GenerateNodeLinks(Now);


	sock = socket(AF_INET,SOCK_DGRAM,0);
//	ioctl(sock, FIONBIO, &param);

	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;		
	sinx.sin_port = htons(81);

	txaddr.sin_family = AF_INET;
	txaddr.sin_addr.s_addr = INADDR_ANY;		
	txaddr.sin_port = htons(81);

	txaddr2.sin_family = AF_INET;
	txaddr2.sin_addr.s_addr = INADDR_ANY;		
	txaddr2.sin_port = htons(81);
	
	//	Resolve name to address

	printf("Resolving %s\n", "guardian.no-ip.org.");
	HostEnt2 = gethostbyname ("guardian.no-ip.org.");

	if (HostEnt2)
		memcpy(&txaddr.sin_addr.s_addr,HostEnt2->h_addr,4);

	HostEnt2 = gethostbyname ("192.168.1.63");

	if (HostEnt2)
		memcpy(&txaddr2.sin_addr.s_addr,HostEnt2->h_addr,4);

	if (bind(sock, (struct sockaddr *) &sinx, sizeof(sinx)) != 0 )
	{
		//	Bind Failed

		int err = GetLastError();
		printf("Bind Failed for UDP port %d - error code = %d", 81, err);
	}


	printf("Map Update Started\n");
	fflush(stdout);

	while (1)
	{
		nLength = recvfrom(sock, RXBUFFER, 512, 0, (struct sockaddr *)&rxaddr, &addrlen);

		if (nLength < 0)
		{
			int err = WSAGetLastError();
			//			if (err != 11)
			//				printf("KISS Error %d %d\n", nLength, err);
			nLength = 0;
		}
		else
		{
			nLength = sendto(sock, RXBUFFER, nLength, 0, (LPSOCKADDR)&txaddr2, sizeof(txaddr));

			if (memcmp(&RXBUFFER[16], "LINK ", 5) != 0)
				if (HostEnt2)
					nLength = sendto(sock, RXBUFFER, nLength, 0, &txaddr, sizeof(txaddr));
		}

		Now = time(NULL);

		if ((Now - LastUpdate) > 60)
			GenerateNodeLinks(Now);

		RXBUFFER[nLength - 2] = 0;

		if (RXBUFFER[14] == 3)			// UI
		{
			char From[10], To[10];
			double Lat, Lon;
			char * Comment, * Version;


			From[ConvFromAX25((unsigned char *)&RXBUFFER[7], From)] = 0;
			To[ConvFromAX25((unsigned char *)&RXBUFFER[0], To)] = 0;

			Node = FindNode(From);
			Node->LastHeard = Now;

//			printf("%s %s %s\n", From, To, Msg);
//			fprintf(logFile, "%s %s %s\n", From, To, Msg);


			if (memcmp(Msg, "MH ", 3) == 0)
			{
				/*
	Line 4211: WA7WWC-12 DUMMY-1 MH AL1Q-12,,,I!?
	Line 4221: YU7BPQ DUMMY-1 MH YU7BPQ to APBPQ1 via WIDE2-1 ctl UI^ pid F0
	Line 4315: WA4ZKO DUMMY-1 MH WA4ZKO,,,G.?
	Line 4316: WA4ZKO DUMMY-1 MH WA4ZKO,,,G.?
	Line 4317: WA4ZKO-7 DUMMY-1 MH WA4ZKO,,,G.?
	Line 4318: WA4ZKO-7 DUMMY-1 MH WA4ZKO-7,,,G.?
	Line 4319: WA4ZKO-7 DUMMY-1 MH WA4ZKO,,,G.?
	Line 4350: YU7BPQ DUMMY-1 MH GB7CIP,14.1104,,B+?O
	Line 4351: M0IPU-2 DUMMY-1 MH G0WYG,,,I!?
	Line 4360: N3HYM DUMMY-1 MH W3JY,3.5919,,B+?O
	Line 4361: W3JY DUMMY-1 MH N3HYM,3.5919,,B+?I
	Line 4385: M0IPU-2 DUMMY-1 MH MB7IWG,,,I!?
	Line 4394: GM8BPQ-2 DUMMY-1 MH GM8BPQ-2,14.1035,IO68vl,M!?I
				                                 ' Heard or Connection Report

                                    Elements = Split(Mid(Report, 4), ",")

                                    If Elements.Length < 4 Then Continue While

                                    Index = FindNodeCall(CallFrom)

                                    HeardCall = Elements(0)
                                    Freq = Elements(1)
                                    LOC = Elements(2)
                                    Flags = Elements(3)

                                    If LOC.Length <> 6 Then LOC = ""
					
					GM8BPQ-2 DUMMY-1 MH GM8BPQ-2,14.1035,IO68vl,M!?I

*/
				 // There are 4 fields - Call, Freq, Loc, Flags

				char * Call, * Freq = 0, * LOC = 0, * Flags = 0;

				// I think strlop will work - cant use strtok with null fields

				Call = &Msg[3];
				Freq = strlop(Call, ',');
				LOC = strlop(Freq, ',');
				Flags = strlop(LOC, ',');

				if (Flags == NULL)
					continue;				// Corrupt

				UpdateHeardData(Node, Call, Freq, LOC, Flags);
				continue;
			}

			if (memcmp(Msg, "LINK ", 3) == 0)
			{
				// GM8BPQ-2 DUMMY-1 LINK LU1HVK-4,16,YU4ZED-4,16,AE5E-14,16,

				char * Call;
				int Type;
				struct NodeLink * Link;

				ptr = strtok_s(Msg, " ", &Context);

				while (Context && Context[0])
				{
					Call = strtok_s(NULL, ",", &Context);
					ptr = strtok_s(NULL, ",", &Context);

					if (ptr == 0)
						break;

					Type = atoi(ptr);

					Link = FindLink(From, Call, Type);

					Link->LastHeard = Now;
				}

				continue;
			}

			// Node Info Message

			// Location Space Version<br>Comment. Location and version may contain spaces!

			// There is always <br> between Version and Comment

			Comment = strstr(Msg, "<br>");

			if (Comment == 0)
				continue;			// Corrupt

			*(Comment)= 0;
			Comment += 4;

			// We now have Location and Version, both of which may contain spaces

			// Actually, looks like node always reformats lat and lon with colon but no spaces

			ptr = strtok_s(Msg, " ,:", &Context);

			if (strlen(ptr) == 6 && FromLOC(ptr, &Lat, &Lon))	// Valid Locator
			{
				// Rest must be version

				Version = Context;
			}
			else
			{
				// See if Space Comma or Colon Separated Lat Lon

				char * ptr2 = strtok_s(NULL, " ,:", &Context);

				if (ptr2 == NULL)
					continue;			// Invalid

				Lat = atof(ptr);
				Lon = atof(ptr2);

				if (Lat == 0.0 || Lon == 0.0)
					continue;					// Invalid

				// Rest should be version

				Version = Context;
			}

			Node->Lat = Lat;
			Node->Lon = Lon;

	//		If Locator.Length = 6 Then
    //            .Comment = .Callsign & " " & Locator & " Ver " & Version
     //       Else
     //           .Comment = .Callsign & " Ver " & Version
     //       End If


			sprintf(Node->Comment, "%s %s<br>%s", From, Version, Comment);
			continue;
		}
	}
}

int ConvFromAX25(unsigned char * incall, char * outcall)
{
        int in,out=0;
        unsigned char chr;

        memset(outcall,0x20,10);

        for (in=0;in<6;in++)
        {
                chr=incall[in];
                if (chr == 0x40)
                        break;
                chr >>= 1;
                outcall[out++]=chr;
        }

        chr = incall[6];                          // ssid

        if (chr == 0x42)
        {
                outcall[out++]='-';
                outcall[out++]='T';
                return out;
        }

        if (chr == 0x44)
        {
                outcall[out++]='-';
                outcall[out++]='R';
                return out;
        }

        chr >>= 1;
        chr     &= 15;

        if (chr > 0)
        {
                outcall[out++]='-';
                if (chr > 9)
                {
                        chr-=10;
                        outcall[out++]='1';
                }
                chr+=48;
                outcall[out++]=chr;
        }
        return (out);
}

void UpdateHeardData(struct NodeData * Node, char * Call, char * Freq, char * LOC, char * Flags)
{


}


/*
5/9/2021 2:36:18 PM - 618 Active Nodes
|N3HYM-5,-77.5036,39.42477,greenmarker.png,0,N3HYM-5 Ver 6.0.21.24 <br>BBS N3HYM&#44;CHAT N3HYM-1&#44;NODE 
|RSNOD,27.57229,61.46665,redmarker.png,0,RSNOD KP31SL Ver 6.0.12.1 <br>APRS on 27.235 ,0,
|Link,GM8BPQ-2,PE1RRR-7,58.476,-6.211,51.62416,4.950115,green
|Link,GM8BPQ-2,G8BPQ-2,58.476,-6.211,52.983812,-1.11646,red,
|
*/

static char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void GenerateNodeLinks(time_t Now)
{
	struct tm * TM;
	struct NodeData * Node;
	struct NodeLink * Link;
	int i;
	FILE * pFile;
	int activeNodes = 0;

	LastUpdate = Now;

	fclose(logFile);
	logFile = fopen("nodelog.txt","ab");


	pFile = fopen("nodestatus.txt","wb");

	if (pFile == NULL)
		return;

	TM = gmtime(&Now);

	// Count active nodes

	for (i = 0; i < NumberOfNodes; i++)
	{
		if ((Now - Nodes[i]->LastHeard) < 86400)			//  > One day old - don't show
			activeNodes++;
	}

	
	fprintf(pFile, "%04d/%02d/%.2d %02d:%02d:%02d - %d Active Nodes\r\n|",
		TM->tm_year + 1900, TM->tm_mon - 1, TM->tm_mday, TM->tm_hour,
		TM->tm_min, TM->tm_sec, activeNodes);
	
	for (i = 0; i < NumberOfNodes; i++)
	{
		Node = Nodes[i];

		if ((Now - Node->LastHeard) > 86400)			//  > One day old - don't show
			continue;

		if ((Now - Node->LastHeard) < 3600) 
			fprintf(pFile, "%s,%f,%f,green,%d,%s,0,\r\n|", Node->Call, Node->Lon, Node->Lat, Node->PopupMode, Node->Comment); 
		else	
			fprintf(pFile, "%s,%f,%f,redmarker.png,%d,%s,0,\r\n|", Node->Call, Node->Lon, Node->Lat, Node->PopupMode, Node->Comment); 
	}


	for (i = 0; i < NumberOfNodeLinks; i++)
	{		
		Link = NodeLinks[i];

		if ((Now - Link->LastHeard) >86400)			//  > One day old - don't show
			continue;

		if ((Link->Call1->Lat == 0.0 && Link->Call1->Lon == 0.0) || (Link->Call2->Lat == 0.0 && Link->Call2->Lon == 0.0))
			fprintf(pFile, "Link,%s,%s,%f,%f,%f,%f,red,\r\n|",
				Link->Call1->Call, Link->Call2->Call, Link->Call1->Lat, Link->Call1->Lon,	Link->Call2->Lat, Link->Call2->Lon);
		else if (Link->Type == 16)
			fprintf(pFile, "Link,%s,%s,%f,%f,%f,%f,green,\r\n|",
				Link->Call1->Call, Link->Call2->Call, Link->Call1->Lat, Link->Call1->Lon,	Link->Call2->Lat, Link->Call2->Lon);
		else 
			fprintf(pFile, "Link,%s,%s,%f,%f,%f,%f,blue,\r\n|",
				Link->Call1->Call, Link->Call2->Call, Link->Call1->Lat, Link->Call1->Lon,	Link->Call2->Lat, Link->Call2->Lon);
	}
	fclose(pFile);

	// Save status for restart

	pFile = fopen("savenodes.txt","wb");

	if (pFile == NULL)
		return;
 	
	for (i = 0; i < NumberOfNodes; i++)
	{
		Node = Nodes[i];
	
		if (Node->LastHeard && Node->Lat != 0.0)
			fprintf(pFile, "%s,%f,%f,%d,%s|%d\r\n", Node->Call, Node->Lat, Node->Lon, Node->PopupMode, Node->Comment, Node->LastHeard); 
	}

	fclose(pFile);

	pFile = fopen("savelinks.txt","wb");

	if (pFile == NULL)
		return;
 	
	for (i = 0; i < NumberOfNodeLinks; i++)
	{
		Link = NodeLinks[i];
		fprintf(pFile, "%s,%s,%d,%d\r\n", Link->Call1->Call, Link->Call2->Call, Link->Type, Link->LastHeard);
	}
	fclose(pFile);

}

