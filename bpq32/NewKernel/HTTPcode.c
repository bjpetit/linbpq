
//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#define _CRT_SECURE_NO_DEPRECATE

#define _USE_32BIT_TIME_T

#include <stdio.h>
#include <stdlib.h>
#include "winsock2.h"
#include "WS2tcpip.h"
#include <windows.h>
#include "asmstrucs.h"
#include "tncinfo.h"
#include "time.h"
#include "bpq32.h"
#include "telnetserver.h"

extern char BPQDirectory[];
extern char MYNODECALL;	// 10 chars,not null terminated
extern struct _DATABASE * DataBase;
extern short NUMBEROFPORTS;
extern int MAXBUFFS, QCOUNT, MINBUFFCOUNT, NOBUFFCOUNT, BUFFERWAITS, SEMGETS, SEMRELEASES, SEMCLASHES, L3FRAMES;
extern int NUMBEROFNODES, MAXDESTS, L4CONNECTSOUT, L4CONNECTSIN, L4FRAMESTX, L4FRAMESRX, L4FRAMESRETRIED, OLDFRAMES;
extern int STATSTIME;
extern  struct TRANSPORTENTRY * L4TABLE;
extern WORD MAXCIRCUITS;
extern struct BPQVECSTRUC BPQHOSTVECTOR[];
extern BOOL APRSApplConnected;  
extern char VersionString[];
VOID FormatTime(char * Time, time_t cTime);

extern COLORREF Colours[256];

char * strlop(char * buf, char delim);
VOID sendandcheck(SOCKET sock, const char * Buffer, int Len);

struct HTTPConnectionInfo		// Used for Web Server for thread-specific stuff
{
	struct HTTPConnectionInfo * Next;
	struct STATIONRECORD * SelCall;	// Station Record for individual statond display
	char Callsign[12];
	int WindDirn, WindSpeed, WindGust, Temp, RainLastHour, RainLastDay, RainToday, Humidity, Pressure; //WX Fields
	char * ScreenLines[100];	// Screen Image for Teminal access mode - max 100 lines (cyclic buffer)
	int ScreenLineLen[100];		// Length of each lime
	int LastLine;				// Pointer to last line of data
	BOOL PartLine;				// Last line does not have CR on end
	char HTTPCall[10];			// Call of HTTP user
	BOOL Changed;				// Changed since last poll. If set, reply immediately, else set timer and wait
	SOCKET sock;				// Socket for pending send
	int ResponseTimer;			// Timer for delayed response
	int KillTimer;				// Clean up timer (no activity timeout)
	int Stream;					// BPQ Stream Number
	char Key[20];				// Session Key
	BOOL Connected;
};

struct HTTPConnectionInfo * SessionList;	// active term mode sessions

char Mycall[10];

char PipeFileName[] = "\\\\.\\pipe\\BPQAPRSWebPipe";

char Index[] = "<html><head><title>%s's BPQ32 Web Server</title></head><body><P align=center>"
	"<table border=2 cellpadding=2 cellspacing=2 bgcolor=white>"
	"<tr><td align=center><a href=/Node/NodeMenu.html>Node Pages</a></td>"
	"<td align=center><a href=/aprs/all.html>APRS Pages</a></td></tr></table></body></html>";

char IndexNoAPRS[] = "<meta http-equiv=\"refresh\" content=\"0;url=/Node/NodeIndex.html\">"
	"<html><head></head><body></body></html>";

char NodeMenu[] = "<html><head><title>%s's BPQ32 Web Server</title></head>"
	"<body background=\"/background.jpg\"><h1 align=center>BPQ32 Node %s</h1><P>"
	"<P align=center><table border=1 cellpadding=2 bgcolor=white><tr>"
	"<td><a href=/Node/Routes.html>Routes</a></td>"
	"<td><a href=/Node/Nodes.html>Nodes</a></td>"
	"<td><a href=/Node/Ports.html>Ports</a></td>"
	"<td><a href=/Node/Links.html>Links</a></td>"
	"<td><a href=/Node/Users.html>Users</a></td>"
	"<td><a href=/Node/Stats.html>Stats</a></td>"
	"<td><a href=/Node/Terminal.html>Terminal (Requires user and password)</a></td>%s%s";
char APRSBit[] = "<td><a href=../aprs/all.html>APRS Pages</a></td>";
char NodeTail[] = "</tr></table>";
	
char Tail[] = "</body></html>";

char RouteHddr[] = "<h2 align=center>Routes</h2><table align=center border=2 style=font-family:monospace bgcolor=white>"
	"<tr><th>Port</th><th>Call</th><th>Quality</th><th>Node Count</th><th>Frame Count</th><th>Retries</th><th>Percent</th><th>Maxframe</th><th>Frack</th><th>Last Heard</th><th>Queued</th></tr>";

char RouteLine[] = "<tr><td>%s%d</td><td>%s%c</td><td>%d</td><td>%d</td><td>%d</td><td>%d</td><td>%d%</td><td>%d</td><td>%d</td><td>%02d:%02d</td><td>%d</td></tr>";

char xNodeHddr[] = "<align=center><form align=center method=get action=/Node/Nodes.html>"
	"<table align=center  bgcolor=white>"
	"<tr><td><input type=submit name=a value=\"Nodes Sorted by Alias\"></td><td>"
	"<input type=submit name=c value=\"Nodes Sorted by Call\"></td><td>"
	"<input type=submit name=t value=\"Nodes with traffic\"></td></tr></form></table>"
	"<h2 align=center>Nodes %s</h2><table style=font-family:monospace align=center border=2 bgcolor=white><tr>";

char NodeHddr[] = "<center><form method=get action=/Node/Nodes.html>"
	"<input type=submit name=a value=\"Nodes Sorted by Alias\">"
	"<input type=submit name=c value=\"Nodes Sorted by Call\">"
	"<input type=submit name=t value=\"Nodes with traffic\"></form></center>"
	"<h2 align=center>Nodes %s</h2><table style=font-family:monospace align=center border=2 bgcolor=white><tr>";

char NodeLine[] = "<td><a href=NodeDetail?%s>%s:%s</td>";

char PortsHddr[] = "<h2 align=center>Ports</h2><table align=center border=2 bgcolor=white>"
	"<tr><th>Port</th><th>Driver</th><th>ID</th></tr>";

char PortLine[] = "<tr><td>%d</td><td><a href=PortStats?%d&%s>&nbsp;%s</a></td><td>%s</td></tr>";
char SessionPortLine[] = "<tr><td>%d</td><td>%s</td><td>%s</td></tr>";

char StatsHddr[] = "<h2 align=center>Node Stats</h2><table align=center cellpadding=2 bgcolor=white>"
	"<col width=250 /><col width=80 /><col width=80 /><col width=80 /><col width=80 /><col width=80 />";

char PortStatsHddr[] = "<h2 align=center>Stats for Port %d</h2><table align=center border=2 cellpadding=2 bgcolor=white>";

char PortStatsLine[] = "<tr><td> %s </td><td> %d </td></tr>";

char LinkHddr[] = "<h2 align=center>Links</h2><table align=center border=2 bgcolor=white>"
	"<tr><th>Far Call</th><th>Our Call</th><th>Port</th><th>ax.25 state</th><th>Link Type</th><th>ax.25 Version</th></tr>";

char LinkLine[] = "<tr><td>%s</td><td>%s</td><td>%d</td><td>%s</td><td>%s</td><td align=center >%d</td></tr>";

char UserHddr[] = "<h2 align=center>Sessions</h2><table align=center border=2 cellpadding=2 bgcolor=white>";

char UserLine[] = "<tr><td>%s</td><td>%s</td><td>%s</td></tr>";

char TermSignon[] = "<html><head><title>BPQ32 Node %s Terminal Access</title></head><body background=\"/background.jpg\">"
	"<h2 align=center>BPQ32 Node %s Terminal Access</h2>"
	"<h3 align=center>Please enter username and password to access the node</h3>"
	"<form method=post action=TermSignon>"
	"<table align=center  bgcolor=white>"
	"<tr><td>User</td><td><input type=text name=user tabindex=1 size=20 maxlength=50 /></td></tr>" 
	"<tr><td>Password</td><td><input type=password name=password tabindex=2 size=20 maxlength=50 /></td></tr></table>"  
	"<p align=center><input type=submit value=Submit /><input type=submit value=Cancel name=Cancel /></form>";

char PassError[] = "<p align=center>Sorry, User or Password is invalid - please try again</p>";

char BusyError[] = "<p align=center>Sorry, No sessions available - please try later</p>";

char LostSession[] = "<html><body>Sorry, Session had been lost - refresh page to sign in again";

char TermPage[] = "<html><head><title>BPQ32 Node %s</title></head>"
	"<body><h3 align=center>BPQ32 Node %s</h3>"
	"<form method=post action=/Node/TermClose?%s>"
	"<p align=center><input type=submit value=\"Close and return to Node Page\" /></form>"
    "<iframe src=OutputScreen.html?%s width=100%% height=80%%></iframe>"
	"<iframe src=InputLine.html?%s width=100%% height=60></iframe>"
	"</body>";

char TermOutput[] = "<html><head>"
	"<meta http-equiv=cache-control content=no-cache>"
	"<meta http-equiv=pragma content=no-cache>"
	"<meta http-equiv=expires content=0>" 
	"<meta http-equiv=refresh content=2>"
	"<script type=\"text/javascript\">\r\n"
	"function ScrollOutput()\r\n"
	"{window.scrollBy(0,document.body.scrollHeight)}</script>"
	"</head><body id=Text onload=\"ScrollOutput()\">"
	"<p style=font-family:monospace>";
//<body onLoad="pageScroll()">
//char TermOutputTail[] = "</p><script>scrollElementToEnd(document.getElementById(\"Text\"));</script>";
//char TermOutputTail[] = "</p><script>document.getElementById(\"Text\").scrollTo(0,1500)</script>";
//char TermOutputTail[] = "</p><script>window.scrollBy(0,500);</script></body></html>";
char TermOutputTail[] = "</p></script></body></html>";

char InputLine[] = "<html><head></head><body>"
	"<form name=inputform method=post action=/TermInput?%s>"
	"<input type=text size=105 name=input />"
	"<script>document.inputform.input.focus();</script></form>";

extern int COUNTNODES();
extern int COUNT_AT_L2();


VOID PollSession(struct HTTPConnectionInfo * Session)
{
	int state, change;
	int count, len;
	char Msg[400] = "";
	char Formatted[2048];
	char * ptr1, * ptr2;
	char c;
	int Line;

	// Poll Node

	SessionState(Session->Stream, &state, &change);
		
	if (change == 1)
	{
		int Line = Session->LastLine++;
		free(Session->ScreenLines[Line]);

		if (state == 1)// Connected
			Session->ScreenLines[Line] = _strdup("*** Connected<br>\r\n");
		else
			Session->ScreenLines[Line] = _strdup("*** Disconnected<br>\r\n");
	
		if (Line == 99)
		Session->LastLine = 0;
		
		Session->Changed = TRUE;
	}
	
	if (RXCount(Session->Stream) > 0)
	{
		do
		{
			GetMsg(Session->Stream, &Msg[0], &len, &count);

			// replace cr with <br> and space with &nbsp;

			ptr1 = Msg;
			ptr2 = &Formatted[0];

			if (Session->PartLine)
			{
				// Last line was incomplete - append to it

				Line = Session->LastLine - 1;

				if (Line < 0)
					Line = 99;

				strcpy(Formatted, Session->ScreenLines[Line]);
				ptr2 += strlen(Formatted);

				Session->LastLine = Line;
				Session->PartLine = FALSE;
			}

			while (len--)
			{
				c = *(ptr1++);
				if (c == 13)
				{
					int LineLen;
					
					strcpy(ptr2, "<br>\r\n");
	
					// Write to screen

					Line = Session->LastLine++;
					free(Session->ScreenLines[Line]);

					LineLen = strlen(Formatted);

					// if line starts with a colour code, process it

					if (Formatted[0] ==  0x1b && LineLen > 1)
					{
						int ColourCode = Formatted[1] - 10;
						COLORREF Colour = Colours[ColourCode];
						char ColString[30];

						memmove(&Formatted[20], &Formatted[2], LineLen);
						sprintf(ColString, "<font color=#%02X%02X%02X>",  GetRValue(Colour), GetGValue(Colour), GetBValue(Colour));
						memcpy(Formatted, ColString, 20);
						strcat(Formatted, "</font>");
						LineLen =+ 28;
					}
					
					Session->ScreenLineLen[Line] = LineLen;
					Session->ScreenLines[Line] = _strdup(Formatted);
					
					if (Line == 99)
						Session->LastLine = 0;
					
					ptr2 = &Formatted[0];
					
				}
				else if (c == 32)
				{
					memcpy(ptr2, "&nbsp;", 6);
					ptr2 += 6;
				}
				else
					*(ptr2++) = c;
			}

			*ptr2 = 0;

			if (ptr2 != &Formatted[0])
			{
				// Incomplete line

				// Save to screen

				Line = Session->LastLine++;
				free(Session->ScreenLines[Line]);
		
				Session->ScreenLines[Line] = _strdup(Formatted);

				if (Line == 99)
					Session->LastLine = 0;
				
				Session->PartLine = TRUE;
			}

		//	strcat(Session->ScreenBuffer, Formatted);
			Session->Changed = TRUE;
		
		} while (count > 0);
	}
}


VOID HTTPTimer()
{
	// Run every tick. Check for status change and data available

	struct HTTPConnectionInfo * Session = SessionList;	// active term mode sessions
	struct HTTPConnectionInfo * PreviousSession = NULL;

	while (Session)
	{
		Session->KillTimer++;

		if (Session->KillTimer > 3000)		// Around 5 mins
		{
			int i;
			int Stream = Session->Stream;

			for (i = 0; i < 100; i++)
			{
				free(Session->ScreenLines[i]);
			}

			SessionControl(Stream, 2, 0);
			SessionState(Stream, &i, &i);
			DeallocateStream(Stream);

			if (PreviousSession)
				PreviousSession->Next = Session->Next;		// Remove from chain
			else
				SessionList = Session->Next;

			free(Session);

			break;
		}
		
		PollSession(Session);

//		if (Session->ResponseTimer == 0 && Session->Changed)
//			Debugprintf("Data to send but no outstanding GET");

		if (Session->ResponseTimer)
		{
			Session->ResponseTimer--;

			if (Session->ResponseTimer == 0 || Session->Changed)
			{
				SOCKET sock = Session->sock;
				char ReplyBuffer[100000];
				int ReplyLen;
				char Header[256];
				int HeaderLen;
				int Last = Session->LastLine;
				int n;
	
				strcpy(ReplyBuffer, TermOutput);

				for (n = Last;;)
				{
					strcat(ReplyBuffer, Session->ScreenLines[n]);

					if (n == 99)
						n = -1;
	
					if (++n == Last)
						break;
				}

				ReplyLen = strlen(ReplyBuffer);
				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], TermOutputTail);
		
				HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen);
				sendandcheck(sock, Header, HeaderLen);
				sendandcheck(sock, ReplyBuffer, ReplyLen);

				Session->ResponseTimer = Session->Changed = 0;
			}
		}
		PreviousSession = Session;
		Session = Session->Next;
	}
}

struct HTTPConnectionInfo * AllocateSession(SOCKET sock)
{
	int KeyVal;
	struct HTTPConnectionInfo * Session = zalloc(sizeof(struct HTTPConnectionInfo));
	int i;
	
	if (Session == NULL)
		return NULL;

	for (i = 0; i < 20; i++)
		Session->ScreenLines[i] = _strdup("Scroll to end<br>");
		
	for (i = 20; i < 100; i++)
		Session->ScreenLines[i] = _strdup("<br>\r\n");

	Session->Stream = FindFreeStream();

	if (Session->Stream == 0)
		return NULL;

	SessionControl(Session->Stream, 1, 0);

	KeyVal = (int)sock * time(NULL);

	sprintf(Session->Key, "%012X", KeyVal);

	if (SessionList)
		Session->Next = SessionList;

	SessionList = Session;

	return Session;
}

struct HTTPConnectionInfo * FindSession(char * Key)
{
	struct HTTPConnectionInfo * Session = SessionList;

	while (Session)
	{
		if (strcmp(Session->Key, Key) == 0)
			return Session;

		Session = Session->Next;
	}

	return NULL;
}

ProcessTermInput(SOCKET sock, char * MsgPtr, int MsgLen, char * Key)
{
	char ReplyBuffer[1024];
	int ReplyLen = sprintf(ReplyBuffer, InputLine, Key, Key);
	char Header[256];
	int HeaderLen;
	int State;
	struct HTTPConnectionInfo * Session = FindSession(Key);
	int Stream;

	if (Session == NULL)
	{
		ReplyLen = sprintf(ReplyBuffer, LostSession);
	}
	else
	{
		char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
		char * end = &MsgPtr[MsgLen];
		int Line = Session->LastLine++;
		char * ptr1, * ptr2;
		char c;
		UCHAR hex;

		Stream = Session->Stream;

		input += 10;
		ptr1 = ptr2 = input;

		// Convert any %xx constructs

		while (ptr1 != end)
		{
			c = *(ptr1++);
			if (c == '%')
			{
				c = *(ptr1++);
				if(isdigit(c))
					hex = (c - '0') << 4;
				else
					hex = (tolower(c) - 'a' + 10) << 4;

				c = *(ptr1++);
				if(isdigit(c))
					hex += (c - '0');
				else
					hex += (tolower(c) - 'a' + 10);

				*(ptr2++) = hex;
			}
			else if (c == '+')
				*(ptr2++) = 32;
			else
				*(ptr2++) = c;
		}

		end = ptr2;
		
		*ptr2 = 0;

		strcat(input, "<br>\r\n");

		free(Session->ScreenLines[Line]);
		
		Session->ScreenLines[Line] = _strdup(input);

		if (Line == 99)
			Session->LastLine = 0;
		
		*end++ = 13;
		*end = 0;

		SessionStateNoAck(Stream, &State);

		if (State == 0)
		{
			char AXCall[10];
			SessionControl(Stream, 1, 0);
			ConvToAX25(Session->HTTPCall, AXCall);
			ChangeSessionCallsign(Stream, AXCall);
		}

		SendMsg(Stream, input, end - input);
		Session->Changed = TRUE;
		Session->KillTimer = 0;
	}

	HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + strlen(Tail));
	send(sock, Header, HeaderLen, 0);
	send(sock, ReplyBuffer, ReplyLen, 0);
	send(sock, Tail, strlen(Tail), 0);
}


ProcessTermClose(SOCKET sock, char * MsgPtr, int MsgLen, char * Key)
{
	char ReplyBuffer[1024];
	int ReplyLen = sprintf(ReplyBuffer, InputLine, Key, Key);
	char Header[256];
	int HeaderLen;
	struct HTTPConnectionInfo * Session = FindSession(Key);

	if (Session)
	{
		Session->KillTimer = 99999;
	}

	ReplyLen = wsprintf(ReplyBuffer, NodeMenu, Mycall, Mycall, (APRSApplConnected)?APRSBit:"", NodeTail);
	HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + strlen(Tail));
	send(sock, Header, HeaderLen, 0);
	send(sock, ReplyBuffer, ReplyLen, 0);
	send(sock, Tail, strlen(Tail), 0);
}


ProcessTermSignon(struct TCPINFO * TCP, SOCKET sock, char * MsgPtr, int MsgLen)
{
	char ReplyBuffer[1024];
	int ReplyLen;
	char Header[256];
	int HeaderLen;
	char * input = strstr(MsgPtr, "\r\n\r\n");	// End of headers
	char * user, * password, * Context;

	if (input)
	{
		int i;
		struct UserRec * USER;

		if (strstr(input, "Cancel=Cancel"))
		{
			ReplyLen = wsprintf(ReplyBuffer, NodeMenu, Mycall, Mycall, (APRSApplConnected)?APRSBit:"", NodeTail);
			goto Sendit;
		}
		user = strtok_s(&input[9], "&", &Context);
		password = strtok_s(NULL, "=", &Context);
		password = Context;

		for (i = 0; i < TCP->NumberofUsers; i++)
		{
			USER = TCP->UserRecPtr[i];

			if (_stricmp(user, USER->UserName) == 0)
			{
 				if (strcmp(password, USER->Password) == 0)
				{
					// ok

					struct HTTPConnectionInfo * Session = AllocateSession(sock);

					if (Session)
					{
						char AXCall[10];
						ReplyLen = sprintf(ReplyBuffer, TermPage, Mycall, Mycall, Session->Key, Session->Key, Session->Key);
						strcpy(Session->HTTPCall, USER->Callsign);
						ConvToAX25(Session->HTTPCall, AXCall);
						ChangeSessionCallsign(Session->Stream, AXCall);
					}
					else
					{
						ReplyLen = wsprintf(ReplyBuffer, NodeMenu, Mycall, Mycall, (APRSApplConnected)?APRSBit:"", NodeTail);
						ReplyLen += sprintf(&ReplyBuffer[ReplyLen], BusyError);
					}
					break;
				}
			}
		}

		if (i == TCP->NumberofUsers)
		{
			//   Not found

			ReplyLen = sprintf(ReplyBuffer, TermSignon, Mycall, Mycall);
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PassError);
		}
  
	}

Sendit:

	HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + strlen(Tail));
	send(sock, Header, HeaderLen, 0);
	send(sock, ReplyBuffer, ReplyLen, 0);
	send(sock, Tail, strlen(Tail), 0);

	return 1;
}

char * LookupKey(char * Key)
{
	if (strcmp(Key, "##MY_CALLSIGN##") == 0)
	{
		char Mycall[10];
		memcpy(Mycall, &MYNODECALL, 10);
		strlop(Mycall, ' ');

		return _strdup(Mycall);
	}
	return NULL;
}


int ProcessSpecialPage(char * Buffer, int FileSize)
{
	// replaces ##xxx### constructs with the requested data

	char * NewMessage = malloc(100000);
	char * ptr1 = Buffer, * ptr2, * ptr3, * ptr4, * NewPtr = NewMessage;
	int PrevLen;
	int BytesLeft = FileSize;
	int NewFileSize = FileSize;
	char * StripPtr = ptr1;

	// strip comments blocks

	while (ptr4 = strstr(ptr1, "<!--"))
	{
		ptr2 = strstr(ptr4, "-->");
		if (ptr2)
		{
			PrevLen = (ptr4 - ptr1);
			memcpy(StripPtr, ptr1, PrevLen);
			StripPtr += PrevLen;
			ptr1 = ptr2 + 3;
			BytesLeft = FileSize - (ptr1 - Buffer);
		}
	}

	memcpy(StripPtr, ptr1, BytesLeft);
	StripPtr += BytesLeft;

	BytesLeft = StripPtr - Buffer;

	FileSize = BytesLeft;
	NewFileSize = FileSize;
	ptr1 = Buffer;
	ptr1[FileSize] = 0;

loop:
	ptr2 = strstr(ptr1, "##");

	if (ptr2)
	{
		PrevLen = (ptr2 - ptr1);			// Bytes before special text
		
		ptr3 = strstr(ptr2+2, "##");

		if (ptr3)
		{
			char Key[80] = "";
			int KeyLen;
			char * NewText;
			int NewTextLen;

			ptr3 += 2;
			KeyLen = ptr3 - ptr2;

			if (KeyLen < 80)
				memcpy(Key, ptr2, KeyLen);

			NewText = LookupKey(Key);
			
			if (NewText)
			{
				NewTextLen = strlen(NewText);
				NewFileSize = NewFileSize + NewTextLen - KeyLen;					
			//	NewMessage = realloc(NewMessage, NewFileSize);

				memcpy(NewPtr, ptr1, PrevLen);
				NewPtr += PrevLen;
				memcpy(NewPtr, NewText, NewTextLen);
				NewPtr += NewTextLen;

				free(NewText);
				NewText = NULL;
			}
			else
			{
				// Key not found, so just leave

				memcpy(NewPtr, ptr1, PrevLen + KeyLen);
				NewPtr += (PrevLen + KeyLen);
			}

			ptr1 = ptr3;			// Continue scan from here
			BytesLeft = Buffer + FileSize - ptr3;
		}
		else		// Unmatched ##
		{
			memcpy(NewPtr, ptr1, PrevLen + 2);
			NewPtr += (PrevLen + 2);
			ptr1 = ptr2 + 2;
		}
		goto loop;
	}

	// Copy Rest

	memcpy(NewPtr, ptr1, BytesLeft);
	NewMessage[NewFileSize] = 0;

	strcpy(Buffer, NewMessage);
	free(NewMessage);
	
	return NewFileSize;
}

int SendMessageFile(SOCKET sock, char * FN, BOOL OnlyifExists)
{
	int FileSize, Sent, Loops = 0;
	char * MsgBytes;
	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	int ReadLen;
	BOOL Special = FALSE;
	int Len;
	int HeaderLen;
	char Header[256];

	FILETIME LastWriteTime;
	LARGE_INTEGER ft;
	time_t ctime;
	char TimeString[64];
	char FileTimeString[64];

	if (strcmp(FN, "/") == 0)
		if (APRSApplConnected)
			wsprintf(MsgFile, "%s\\HTML\\index.html", BPQDirectory);
		else
			wsprintf(MsgFile, "%s\\HTML\\indexnoaprs.html", BPQDirectory);
	else
		wsprintf(MsgFile, "%s\\HTML%s", BPQDirectory, FN);
	
	hFile = CreateFile(MsgFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		if (OnlyifExists)					// Set if we dont want an error response if missing
			return -1;
		
		Len = sprintf(Header, "HTTP/1.0 404 Not Found\r\nContent-Length: 16\r\n\r\nPage not found\r\n");
		send(sock, Header, Len, 0);
		return 0;
	}
	
	FileSize = GetFileSize(hFile, NULL);

	MsgBytes = malloc(FileSize + 1);

	ReadFile(hFile, MsgBytes, FileSize, &ReadLen, NULL); 

	GetFileTime(hFile, NULL, NULL, &LastWriteTime);

	ft.HighPart = LastWriteTime.dwHighDateTime;
	ft.LowPart = LastWriteTime.dwLowDateTime;

	CloseHandle(hFile);

	ft.QuadPart -=  116444736000000000;
	ft.QuadPart /= 10000000;

	ctime = ft.LowPart;

	FormatTime(TimeString, ctime);

	// if HTML file, look for ##...## substitutions

	if ((strcmp(FN, "/") == 0 || strstr(FN, "htm" ) || strstr(FN, "HTM")) &&  strstr(MsgBytes, "##" ))
	{
		FileSize = ProcessSpecialPage(MsgBytes, FileSize); 
		ctime = time(NULL);
	}

	FormatTime(FileTimeString, ctime);
	FormatTime(TimeString, time(NULL));

	HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
		"Content-Type: text/html\r\n"
		"Date: %s\r\n"
		"Last-Modified: %s\r\n" 
		"\r\n", FileSize, TimeString, FileTimeString);

	send(sock, Header, HeaderLen, 0);

	Sent = send(sock, MsgBytes, FileSize, 0);
		
	while (Sent != FileSize && Loops++ < 3000)				// 100 secs max
	{	
		if (Sent > 0)					// something sent
		{
			FileSize -= Sent;
			memmove(MsgBytes, &MsgBytes[Sent], FileSize);
		}
					
		Sleep(30);
		Sent = send(sock, MsgBytes, FileSize, 0);
//		Debugprintf("%d out of %d sent", Sent, FileSize);
	}

	free (MsgBytes);

	return 0;
}

VOID sendandcheck(SOCKET sock, const char * Buffer, int Len)
{
	int Loops = 0;
	int Sent = send(sock, Buffer, Len, 0);
	char * Copy = NULL;
		
	while (Sent != Len && Loops++ < 300)					// 10 secs max
	{	
//		Debugprintf("%d out of %d sent %d Loops", Sent, Len, Loops);
				
		if (Copy == NULL)
		{
			Copy = malloc(Len);
			memcpy(Copy, Buffer, Len);
		}

		if (Sent > 0)					// something sent
		{
			Len -= Sent;
			memmove(Copy, &Copy[Sent], Len);
		}		
		
		Sleep(30);
		Sent = send(sock, Copy, Len, 0);
	}

	if (Copy)
		free(Copy);

	return;
}

int RefreshTermWindow(struct HTTPConnectionInfo * Session, char * ReplyBuffer)
{
	char Msg[400] = "";
	int HeaderLen, ReplyLen;
	char Header[256];
	
	PollSession(Session);			// See if anything received 

	if (Session->Changed)
	{
				int Last = Session->LastLine;
				int n;
	
				strcpy(ReplyBuffer, TermOutput);

				for (n = Last;;)
				{
					strcat(ReplyBuffer, Session->ScreenLines[n]);

					if (n == 99)
						n = -1;
	
					if (++n == Last)
						break;
				}

				Session->Changed = 0;

				ReplyLen = strlen(ReplyBuffer);
				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], TermOutputTail);

				HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen);
				sendandcheck(Session->sock, Header, HeaderLen);
				sendandcheck(Session->sock, ReplyBuffer, ReplyLen);

				return 1;
	}
	else
		return 0;
}

VOID DISPLAYCIRCUIT(struct TRANSPORTENTRY * L4, char * Buffer)
{
	UCHAR Type = L4->L4CIRCUITTYPE;
	struct PORTCONTROL * PORT;
	struct _LINKTABLE * LINK;
	struct BPQVECSTRUC * VEC;
	struct DEST_LIST * DEST;

	char Normcall[11] = "";
	char Normcall2[11] = "";
	char Alias[11] = "";

	Buffer[0] = 0;

	switch (Type)
	{
	case PACTOR+UPLINK:

		PORT = L4->L4TARGET;
		
		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');
		
		wsprintf(Buffer, "%s %d/%d(%s)", "TNC Uplink Port", PORT->PORTNUMBER, L4->KAMSESSION, Normcall);

		return;
	
		
	case PACTOR+DOWNLINK:

		PORT = L4->L4TARGET;
		wsprintf(Buffer, "%s %d/%d", "Attached to Port", PORT->PORTNUMBER, L4->KAMSESSION);
		return;


	case L2LINK+UPLINK:

		LINK = L4->L4TARGET;

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		wsprintf(Buffer, "%s %d(%s)", "Uplink", LINK->LINKPORT, Normcall);
		return;

/*
	MOV	ESI,L4TARGET[ESI]		; NODE
	MOV	AL,LINKPORT[ESI]
	CALL	CONV_DIGITS

	POP	EAX

	jmp	SHORT CMDS17

	PUBLIC	CMDS11
CMDS11:
*/
	case L2LINK+DOWNLINK:

		LINK = L4->L4TARGET;
			
		ConvFromAX25(LINK->OURCALL, Normcall);
		strlop(Normcall, ' ');

		ConvFromAX25(LINK->LINKCALL, Normcall2);
		strlop(Normcall2, ' ');

		wsprintf(Buffer, "%s %d(%s %s)", "Downlink", LINK->LINKPORT, Normcall, Normcall2);
		return;

	case BPQHOST + UPLINK:
	case BPQHOST + DOWNLINK:

		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');
		VEC = L4->L4TARGET;
		wsprintf(Buffer, "%s%02d(%s)", "Host", (VEC - BPQHOSTVECTOR) + 1, Normcall);
		return;

	case SESSION + DOWNLINK:
	case SESSION + UPLINK:
	
		ConvFromAX25(L4->L4USER, Normcall);
		strlop(Normcall, ' ');

		DEST = L4->L4TARGET;

		ConvFromAX25(DEST->DEST_CALL, Normcall2);
		strlop(Normcall2, ' ');

		memcpy(Alias, DEST->DEST_ALIAS, 6);
		strlop(Alias, ' ');

		wsprintf(Buffer, "Circuit(%s:%s %s)", Alias, Normcall2, Normcall);

		return;
	}
}

int CompareAlias(const void *a, const void *b) 
{ 
	struct DEST_LIST * x;
	struct DEST_LIST * y;

	_asm 
	{
		push eax
		mov eax, a
		mov eax, [eax]
		mov	x, eax
		mov eax, b
		mov eax, [eax]
		mov	y, eax
		pop eax
}

	return memcmp(x->DEST_ALIAS, y->DEST_ALIAS, 6);
	/* strcmp functions works exactly as expected from
	comparison function */ 
} 


int CompareNode(const void *a, const void *b) 
{ 
	struct DEST_LIST * x;
	struct DEST_LIST * y;

	_asm 
	{
		push eax
		mov eax, a
		mov eax, [eax]
		mov	x, eax
		mov eax, b
		mov eax, [eax]
		mov	y, eax
		pop eax
}

	return memcmp(x->DEST_CALL, y->DEST_CALL, 7);
	/* strcmp functions works exactly as expected from
	comparison function */ 
} 

static char EXCEPTMSG[80] = "";

ProcessHTTPMessage(struct ConnectionInfo * conn)
{
	struct TCPINFO * TCP = conn->TCP;
	SOCKET sock = conn->socket;
	char * MsgPtr = conn->InputBuffer;
	int MsgLen = conn->InputLen;
	int InputLen = 0;
	int OutputLen = 0;
	int Bufferlen;
	struct HTTPConnectionInfo CI;
	struct HTTPConnectionInfo * sockptr = &CI;

	HANDLE hPipe;
	char URL[4096];
	char * ptr;
	char * Context, * Method, * NodeURL;
	int ReplyLen;
	char Header[256];
	int HeaderLen;
	char TimeString[64];
	struct _EXCEPTION_POINTERS exinfo;

	strcpy(EXCEPTMSG, "ProcessHTTPMessage");

	__try {

	strcpy(URL, MsgPtr);

	ptr = strstr(URL, " HTTP");

	if (ptr)
		*ptr = 0;

	Method = strtok_s(URL, " ", &Context);

	memcpy(Mycall, &MYNODECALL, 10);
	strlop(Mycall, ' ');

	if (_memicmp(Context, "/APRS/", 6) == 0)
	{
		// If for APRS, Pass to APRS Server via Named Pipe

		char ReplyBuffer[8192];

		hPipe = CreateFile(PipeFileName, GENERIC_READ | GENERIC_WRITE,
                  0,                    // exclusive access
                  NULL,                 // no security attrs
                  OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL, 
                  NULL );

		if (hPipe == (HANDLE)-1)
		{
			InputLen = wsprintf(ReplyBuffer, "HTTP/1.0 404 Not Found\r\nContent-Length: 28\r\n\r\nAPRS Data is not available\r\n");
			send(sock, ReplyBuffer, InputLen, 0);
		}
		else
		{
			int Sent;
			int Loops = 0;
			WriteFile(hPipe, MsgPtr, MsgLen, &InputLen, NULL);

			while(TRUE)
			{
				ReadFile(hPipe, ReplyBuffer, 8192, &InputLen, NULL);
				if (InputLen <= 0)
				{
					InputLen = GetLastError();
					break;
				}
				Sent = send(sock, ReplyBuffer, InputLen, 0);
		
				while (Sent != InputLen && Loops++ < 3000)					// 100 secs max
				{	
//					Debugprintf("%d out of %d sent %d Loops", Sent, InputLen, Loops);
				
					if (Sent > 0)					// something sent
					{
						InputLen -= Sent;
						memmove(ReplyBuffer, &ReplyBuffer[Sent], InputLen);
					}
					
					Sleep(30);
					Sent = send(sock, ReplyBuffer, InputLen, 0);
				}
			}
			CloseHandle(hPipe);
		}
		return 0;
	}

	NodeURL = strtok_s(NULL, "?", &Context);

	if (strcmp(Method, "POST") == 0)
	{
		char ReplyBuffer[8192];

		if (_stricmp(NodeURL, "/TermInput") == 0)
		{
			ProcessTermInput(sock, MsgPtr, MsgLen, Context);
			return 0;
		}

		if (_stricmp(NodeURL, "/Node/TermSignon") == 0)
		{
			ProcessTermSignon(TCP, sock, MsgPtr, MsgLen);
		}

		if (_stricmp(NodeURL, "/Node/TermClose") == 0)
		{
			ProcessTermClose(sock, MsgPtr, MsgLen, Context);
			return 0;
		}

		send(sock, ReplyBuffer, InputLen, 0);
		return 0;
	}

	if (_stricmp(NodeURL, "/") == 0 || _stricmp(NodeURL, "/Index.html") == 0)
	{		
		// Send if present, else use default

		Bufferlen = SendMessageFile(sock, NodeURL, TRUE);		// return -1 if not found
		
		if (Bufferlen != -1)
			return 0;						// We've sent it
		else
		{
			char ReplyBuffer[1000];
	
			if (APRSApplConnected)
				ReplyLen = wsprintf(ReplyBuffer, Index, Mycall, Mycall);
			else
				ReplyLen = wsprintf(ReplyBuffer, IndexNoAPRS, Mycall, Mycall);

			HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n", ReplyLen + strlen(Tail));	
			send(sock, Header, HeaderLen, 0);
			send(sock, ReplyBuffer, ReplyLen, 0);
			send(sock, Tail, strlen(Tail), 0);

			return 0;
		}
	}

	else if (_stricmp(NodeURL, "/NodeMenu.html") == 0 || _stricmp(NodeURL, "/Node/NodeMenu.html") == 0)
	{		
		// Send if present, else use default

		Bufferlen = SendMessageFile(sock, "/NodeMenu.html", TRUE);		// return -1 if not found
		
		if (Bufferlen != -1)
			return 0;											// We've sent it
	}

	else if (_memicmp(NodeURL, "/NODE/", 6))
	{
		// Not Node, See if a local file
		
		Bufferlen = SendMessageFile(sock, NodeURL, FALSE);		// Send error if not found
		return 0;
	}

	// Node URL

	{

	char ReplyBuffer[100000];
	ReplyLen = wsprintf(ReplyBuffer, NodeMenu, Mycall, Mycall, (APRSApplConnected)?APRSBit:"", NodeTail);

	if (_stricmp(NodeURL, "/Node/Stats.html") == 0)
	{
		struct tm * TM;
		char UPTime[50];
		time_t szClock = STATSTIME * 60;

		TM = gmtime(&szClock);

		wsprintf(UPTime, "%.2d:%.2d:%.2d", TM->tm_yday, TM->tm_hour, TM->tm_min);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], StatsHddr);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td>%s</td><td align=right>%s</td></tr>",
			"Version", VersionString);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td>%s</td><td align=right>%s</td></tr>",
			"Uptime (Days Hours Mins)", UPTime);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td><td align=right>%d</td></tr>",
		"Semaphore: Get/Rel/Clashes", SEMGETS, SEMRELEASES, SEMCLASHES);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td><td align=right>%d</td><td align=right>%d</td align=right><td align=right>%d</td></tr>",
			"Buffers: Max/Cur/Min/Out/Wait", MAXBUFFS, QCOUNT, MINBUFFCOUNT, NOBUFFCOUNT, BUFFERWAITS);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td></tr>",
		"Known Nodes/Max Nodes", NUMBEROFNODES, MAXDESTS);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td></tr>",
		"L4 Connects Sent/Rxed ", L4CONNECTSOUT, L4CONNECTSIN);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td>%s</td><td align=right>%d</td><td align=right>%d</td><td align=right>%d</td align=right><td align=right>%d</td></tr>",
			"L4 Frames TX/RX/Resent/Reseq", L4FRAMESTX, L4FRAMESRX, L4FRAMESRETRIED, OLDFRAMES);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td>%s</td><td align=right>%d</td></tr>",
		"L3 Frames Relayed", L3FRAMES);

	}
	if (_stricmp(NodeURL, "/Node/PortStats") == 0)
	{
		struct _EXTPORTDATA * Port;

		char * PortChar = strtok_s(NULL, "&", &Context);
		int PortNo = atoi(PortChar);
		int Protocol;
		int PortType;
		
		//		char PORTTYPE;	// H/W TYPE
		// 0 = ASYNC, 2 = PC120, 4 = DRSI
					// 6 = TOSH, 8 = QUAD, 10 = RLC100
					// 12 = RLC400 14 = INTERNAL 16 = EXTERNAL

#define KISS 0
#define NETROM 2
#define HDLC 6
#define L2 8
#define WINMOR 10


		// char PROTOCOL;	// PORT PROTOCOL
					// 0 = KISS, 2 = NETROM, 4 = BPQKISS
					//; 6 = HDLC, 8 = L2

	
		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsHddr, PortNo);
		
		Port = (struct _EXTPORTDATA *)GetPortTableEntryFromPortNum(PortNo);
		Protocol = Port->PORTCONTROL.PROTOCOL;
		PortType = Port->PORTCONTROL.PROTOCOL;

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "L2 Frames Digied", Port->PORTCONTROL.L2DIGIED);
		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "L2 Frames Heard", Port->PORTCONTROL.L2FRAMES);
		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "L2 Frames Recieved", Port->PORTCONTROL.L2FRAMESFORUS);
		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "L2 Frames Sent", Port->PORTCONTROL.L2FRAMESSENT);
		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "L2 Timeouts", Port->PORTCONTROL.L2TIMEOUTS);
		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "REJ Frames Received", Port->PORTCONTROL.L2REJCOUNT);
		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "RX out of Seq", Port->PORTCONTROL.L2OUTOFSEQ);
//		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "L2 Resequenced", Port->PORTCONTROL.L2RESEQ);
		if (Protocol == HDLC)
		{
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "Underrun", Port->PORTCONTROL.L2URUNC);
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "RX Overruns", Port->PORTCONTROL.L2ORUNC);
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "RX CRC Errors", Port->PORTCONTROL.RXERRORS);
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "Frames abandoned", Port->PORTCONTROL.L1DISCARD);
		}
		else if ((Protocol == KISS && Port->PORTCONTROL.KISSFLAGS) || Protocol == NETROM)
		{
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "Poll Timeout", Port->PORTCONTROL.L2URUNC);
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "RX CRC Errors", Port->PORTCONTROL.RXERRORS);
		}

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "FRMRs Sent", Port->PORTCONTROL.L2FRMRTX);
		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortStatsLine, "FRMRs Received", Port->PORTCONTROL.L2FRMRRX);

//		DB	'Link Active %   '
//		DD	AVSENDING

	}

	if (_stricmp(NodeURL, "/Node/Ports.html") == 0)
	{
		struct _EXTPORTDATA * ExtPort;
		struct PORTCONTROL * Port;

		int count;
		char DLL[20];

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortsHddr);
		
		for (count = 1; count <= NUMBEROFPORTS; count++)
		{
			Port = GetPortTableEntryFromSlot(count);
			ExtPort = (struct _EXTPORTDATA *)Port;
	
			if (Port->PORTTYPE == 0x10)
			{	
				strcpy(DLL, ExtPort->PORT_DLL_NAME);
				strlop(DLL, '.');
			}
			else if (Port->PORTTYPE == 0)
				strcpy(DLL, "ASYNC");

			else if (Port->PORTTYPE == 14)
				strcpy(DLL, "INTERNAL");
	
			else if (Port->PORTTYPE > 0 && Port->PORTTYPE < 14)
				strcpy(DLL, "HDLC");

			if (Port->PROTOCOL == 10)		// Pactor-Like driver
				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], SessionPortLine, Port->PORTNUMBER, DLL, Port->PORTDESCRIPTION);
			else
				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], PortLine, Port->PORTNUMBER, Port->PORTNUMBER,
					DLL, DLL, Port->PORTDESCRIPTION);
		}
	}

	if (_stricmp(NodeURL, "/Node/Nodes.html") == 0)
	{
		struct DEST_LIST * Dests = DataBase->DESTS;
		int count, i;
		char Normcall[10];
		char Alias[10];
		int Width = 5;
		int x = 0, n = 0;
		struct DEST_LIST * List[1000];

		_strupr(Context);

		for (count = 0; count < MAXDESTS; count++)
		{
			if (Dests->DEST_CALL[0] != 0)
			{
				if (Context[0] != 'T' || Dests->DEST_COUNT)
					List[n++] = Dests;

				if (n > 999)
					break;
			}

			Dests++;
		}

		if (n > 1)
		{
			if (Context[0] == 'C') 
				qsort(List, n, 4, CompareNode);
			else
				qsort(List, n, 4, CompareAlias);
		}
		
		Alias[6] = 0; 

		if (Context[0] == 'T')
		{
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], NodeHddr, "with traffic");
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<td>Call</td><td>Frames</td><td>RTT</td><td>BPQ?</td><td>Hops</td></tr><tr>");
		}
		else if (Context[0] == 'C') 
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], NodeHddr, "sorted by Call");
		else
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], NodeHddr, "sorted by Alias");

		for (i = 0; i < n; i++)
		{
			int len = ConvFromAX25(List[i]->DEST_CALL, Normcall);
			Normcall[len]=0;
		
			memcpy(Alias, List[i]->DEST_ALIAS, 6);
			strlop(Alias, ' ');

			if (Context[0] == 'T')
			{
				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<td>%s:%s</td><td align=center>%d</td><td align=center>%d</td><td align=center>%c</td><td align=center>%.0d</td></tr><tr>",
					Normcall, Alias, List[i]->DEST_COUNT, List[i]->DEST_RTT /16,
					(List[i]->DEST_STATE & 0x40)? 'B':' ', (List[i]->DEST_STATE & 63));

			}
			else
			{
				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], NodeLine, Normcall, Alias, Normcall);

				if (++x == Width)
				{
					x = 0;
					ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "</tr><tr>");
				}
			}
		}

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "</tr>");
	}

	if (_stricmp(NodeURL, "/Node/NodeDetail") == 0)
	{
		UCHAR AXCall[8];
		struct DEST_LIST * Dest = DataBase->DESTS;
		struct NR_DEST_ROUTE_ENTRY * NRRoute;
		struct ROUTE * Neighbour;
		char Normcall[10];
		int i, len, count, Active;
		char Alias[7];

		Alias[6] = 0;

		_strupr(Context);

		ConvToAX25(Context, AXCall);

		for (count = 0; count < MAXDESTS; count++)
		{
			if (memcmp(Dest->DEST_CALL, AXCall, 7) == 0)
			{
				break;
			}
			Dest++;
		}
		
		if (count == MAXDESTS)
		{
			ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<h3 align = center>Call %s not found</h3>", Context);
			goto SendResp;
		}

		memcpy(Alias, Dest->DEST_ALIAS, 6);
		strlop(Alias, ' ');

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen],
			"<h3 align=center>Info for Node %s:%s</h3><p style=font-family:monospace align=center>", Alias, Context);

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<table border=1 bgcolor=white><tr><td>Frames</td><td>RTT</td><td>BPQ?</td><td>Hops</td></tr>");	

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td align=center>%d</td><td align=center>%d</td><td align=center>%c</td><td align=center>%.0d</td></tr></table>",
				Dest->DEST_COUNT, Dest->DEST_RTT /16,
				(Dest->DEST_STATE & 0x40)? 'B':' ', (Dest->DEST_STATE & 63));

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<h3 align=center>Neighbours</h3>");

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], 
			"<table border=1 style=font-family:monospace align=center bgcolor=white>"
			"<tr><td> </td><td> Qual </td><td> Obs </td><td> Port </td><td> Call </td></tr>");	

		NRRoute = &Dest->NRROUTE1;

		Active = Dest->DEST_ROUTE;

		for (i = 1; i < 4; i++)
		{
			Neighbour = NRRoute->ROUT_NEIGHBOUR;

			if (Neighbour)
			{
				len = ConvFromAX25(Neighbour->NEIGHBOUR_CALL, Normcall);
				Normcall[len] = 0;

				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "<tr><td>%c&nbsp;</td><td>%d</td><td>%d</td><td>%d</td><td>%s</td></tr>",
					(Active == i)?'>':' ',NRRoute->ROUT_QUALITY, NRRoute->ROUT_OBSCOUNT, Neighbour->NEIGHBOUR_PORT, Normcall);
			}
			NRRoute++;
		}

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], "</table>");

		goto SendResp;

	}
/*

	MOV	ESI,OFFSET32 NODEROUTEHDDR
	MOV	ECX,11
	REP MOVSB

	LEA	ESI,DEST_CALL[EBX]
	CALL	DECODENODENAME		; CONVERT TO ALIAS:CALL
	REP MOVSB

	CMP	DEST_RTT[EBX],0
	JE SHORT @f			; TIMER NOT SET - DEST PROBABLY NOT USED

	MOVSB				; ADD SPACE
	CALL	DORTT
	
@@:
 	MOV	AL,CR
	STOSB

	MOV	ECX,3
	MOV	DH,DEST_ROUTE[EBX]	; CURRENT ACTIVE ROUTE
	MOV	DL,1
	
	push ebx
	
	PUBLIC	CMDN110
CMDN110:

	MOV	ESI,ROUT1_NEIGHBOUR[EBX]
	CMP	ESI,0
	JE CMDN199


	MOV	AX,'  '
	CMP	DH,DL
	JNE SHORT CMDN112			; NOT CURRENT DEST
	MOV	AX,' >'

CMDN112:

	STOSW

	PUSH	ECX

	MOV	AL,ROUT1_QUALITY[EBX]
	CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

	mov	AL,' '
	stosb

	MOV	AL,ROUT1_OBSCOUNT[EBX]
	CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

	mov	AL,' '
	stosb

	MOV	AL,NEIGHBOUR_PORT[ESI]	; GET PORT
	CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

	mov	AL,' '
	stosb


	PUSH	EDI
	CALL	CONVFROMAX25		; CONVERT TO CALL
	POP	EDI

	MOV	ESI,OFFSET32 NORMCALL
	REP MOVSB
	
	MOV	AL,CR
	STOSB

	ADD	EBX,ROUTEVECLEN
	INC	DL			; ROUTE NUMBER

	POP	ECX
	DEC	ECX
	JNZ	CMDN110

	PUBLIC	CMDN199
CMDN199:

	POP	EBX
	
; DISPLAY  INP3 ROUTES

	MOV	ECX,3
	MOV	DL,4

	PUBLIC	CMDNINP3
CMDNINP3:

	MOV	ESI,INPROUT1_NEIGHBOUR[EBX]
	CMP	ESI,0
	JE CMDNINPEND

	MOV	AX,'  '
	CMP	DH,DL
	JNE SHORT @F			; NOT CURRENT DEST
	MOV	AX,' >'

@@:

	STOSW

	PUSH	ECX
	
	MOV	AL, Hops1[EBX]
	CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

	mov	AL,' '
	stosb

	MOVZX	EAX, SRTT1[EBX]
	
	MOV	EDX,0
	MOV ECX, 100
	DIV ECX	
	CALL	CONV_5DIGITS
	MOV	AL,'.'
	STOSB
	MOV EAX, EDX
	CALL	PRINTNUM
	MOV	AL,'s'
	STOSB
	MOV	AL,' '
	STOSB
	
	MOV	AL,NEIGHBOUR_PORT[ESI]	; GET PORT
	CALL	CONV_DIGITS		; CONVERT AL TO DECIMAL DIGITS

	mov	AL,' '
	stosb

	PUSH	EDI
	CALL	CONVFROMAX25		; CONVERT TO CALL
	POP	EDI

	MOV	ESI,OFFSET32 NORMCALL
	REP MOVSB
	

	MOV	AL,CR
	STOSB

	ADD	EBX,INPROUTEVECLEN
	INC	DL			; ROUTE NUMBER

	POP	ECX
	LOOP	CMDNINP3

CMDNINPEND:

	ret
	
*/


	if (_stricmp(NodeURL, "/Node/Routes.html") == 0)
	{
		struct ROUTE * Routes = DataBase->NEIGHBOURS;
		int MaxRoutes = DataBase->MAXNEIGHBOURS;
		int count;
		char Normcall[10];
		char locked;
		int NodeCount;
		int Percent = 0;
		int Iframes, Retries;
		char Active[10];
		int Queued;

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], RouteHddr);
		
		for (count=0; count<MaxRoutes; count++)
		{
			if (Routes->NEIGHBOUR_CALL[0] != 0)
			{
				int len = ConvFromAX25(Routes->NEIGHBOUR_CALL, Normcall);
				Normcall[len]=0;

				if ((Routes->NEIGHBOUR_FLAG & 1) == 1)
					locked = '!';
				else
					locked = ' ';

				_asm
				{
					pushad
					mov esi, Routes
					call COUNTNODES
					mov NodeCount, eax
					popad
				}

				if (Routes->NEIGHBOUR_LINK)
				{
					struct _LINKTABLE * Link = Routes->NEIGHBOUR_LINK;
						
					_asm
					{
						pushad

						mov EBX, Link;
						MOV	AH,0
						CALL	COUNT_AT_L2		; SEE HOW MANY QUEUED
						movzx EAX, AL
						mov Queued, EAX

						popad
					}
				}
				else
					Queued = 0;

				Iframes = Routes->NBOUR_IFRAMES;
				Retries = Routes->NBOUR_RETRIES;

				if (Routes->NEIGHBOUR_LINK && Routes->NEIGHBOUR_LINK->L2STATE >= 5)
					strcpy(Active, ">");
				else
					strcpy(Active, "&nbsp;");

				if (Iframes)
					Percent = (Retries * 100) / Iframes;
				else
					Percent = 0;
		
				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], RouteLine, Active, Routes->NEIGHBOUR_PORT, Normcall, locked, 
					Routes->NEIGHBOUR_QUAL,	NodeCount, Iframes, Retries, Percent, Routes->NBOUR_MAXFRAME, Routes->NBOUR_FRACK,
					Routes->NEIGHBOUR_TIME >> 8, LOBYTE(Routes->NEIGHBOUR_TIME), Queued);

			Routes+=1;
			}
		}
	}

	if (_stricmp(NodeURL, "/Node/Links.html") == 0)
	{
		struct _LINKTABLE * Links = DataBase->LINKS;
		int MaxLinks = DataBase->MAXLINKS;
		int count;
		char Normcall1[10];
		char Normcall2[10];
		char State[12] = "", Type[12] = "Uplink";
		int axState;
		int cctType;

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], LinkHddr);
		
		for (count=0; count<MaxLinks; count++)
		{
			if (Links->LINKCALL[0] != 0)
			{
				int len = ConvFromAX25(Links->LINKCALL, Normcall1);
				Normcall1[len] = 0;

				len = ConvFromAX25(Links->OURCALL, Normcall2);
				Normcall2[len] = 0;

				axState = Links->L2STATE;

				if (axState == 2)
					strcpy(State, "Connecting");
				else if (axState == 3)
					strcpy(State, "FRMR");
				else if (axState == 4)
					strcpy(State, "Closing");
				else if (axState == 5)
					strcpy(State, "Active");
				else if (axState == 6)
					strcpy(State, "REJ Sent");

				cctType = Links->LINKTYPE;

				if (cctType == 1)
					strcpy(Type, "Uplink");
				else if (cctType == 2)
					strcpy(Type, "Downlink");
				else if (cctType == 3)
					strcpy(Type, "Node-Node");

				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], LinkLine, Normcall1, Normcall2, Links->LINKPORT, 
					State, Type, 2 - Links->VER1FLAG );
						
			Links+=1;
			}
		}
	}

	if (_stricmp(NodeURL, "/Node/Users.html") == 0)
	{
		struct TRANSPORTENTRY * L4 = L4TABLE;
		struct TRANSPORTENTRY * Partner;
		int MaxLinks = DataBase->MAXLINKS;
		int count;
		char State[12] = "", Type[12] = "Uplink";
		char LHS[50] = "", MID[10] = "", RHS[50] = "";

		ReplyLen += sprintf(&ReplyBuffer[ReplyLen], UserHddr);
		
		for (count=0; count < MAXCIRCUITS; count++)
		{
			if (L4->L4USER[0])
			{
				RHS[0] = MID[0] = 0;

				if ((L4->L4CIRCUITTYPE & UPLINK) == 0)   //SHORT CMDS10A		; YES
				{
					// IF DOWNLINK, ONLY DISPLAY IF NO CROSSLINK

					if (L4->L4CROSSLINK == 0)	//jne 	CMDS60			; WILL PROCESS FROM OTHER END
					{
						// ITS A DOWNLINK WITH NO PARTNER - MUST BE A CLOSING SESSION
						// DISPLAY TO THE RIGHT FOR NOW

						strcpy(LHS, "(Closing) ");
						DISPLAYCIRCUIT(L4, RHS);
						goto CMDS50;
					}
					else
						goto CMDS60;		// WILL PROCESS FROM OTHER END
				}
			
				if (L4->L4CROSSLINK == 0)
				{
					// Single Entry

					DISPLAYCIRCUIT(L4, LHS);
				}
				else
				{
					DISPLAYCIRCUIT(L4, LHS);

					Partner = L4->L4CROSSLINK;

					if (Partner->L4STATE == 5)
						strcpy(MID, "<-->");
					else
						strcpy(MID, "<~~>");
					
					DISPLAYCIRCUIT(Partner, RHS);
				}
CMDS50:
				ReplyLen += sprintf(&ReplyBuffer[ReplyLen], UserLine, LHS, MID, RHS);
			}
CMDS60:			
		L4++;	
		}
	}
/*
	PUBLIC	CMDUXX_1
CMDUXX_1:
	push	EBX
	push	ESI
	PUSH	ECX
	push	EDI

	call	_FINDDESTINATION
	pop	EDI

	jz SHORT NODE_FOUND

	push	EDI			; NET/ROM not found
	call	CONVFROMAX25		; CONVERT TO CALL
	pop	EDI
	mov	ESI,OFFSET32 NORMCALL
	rep movsb

	jmp	SHORT END_CMDUXX

	PUBLIC	NODE_FOUND
NODE_FOUND:

	lea	ESI,DEST_CALL[EBX]
	call	DECODENODENAME

	REP 	MOVSB

	PUBLIC	END_CMDUXX
END_CMDUXX:

	POP	ECX
	pop	ESI
	pop	EBX
	ret

}}}
*/

	else if (_stricmp(NodeURL, "/Node/Terminal.html") == 0)
	{
		ReplyLen = sprintf(ReplyBuffer, TermSignon, Mycall, Mycall);
	}

	else if (_stricmp(NodeURL, "/Node/OutputScreen.html") == 0)
	{
		struct HTTPConnectionInfo * Session = FindSession(Context);

		if (Session == NULL)
		{
			ReplyLen = sprintf(ReplyBuffer, LostSession);
		}
		else
		{
			Session->sock = sock;				// socket to reply on
			ReplyLen = RefreshTermWindow(Session, ReplyBuffer);
			
			if (ReplyLen == 0)		// Nothing new
			{
//				Debugprintf("GET with no data avail - response held");
				Session->ResponseTimer = 1200;		// Delay response for up to a minute
			}
			else
			{
//				Debugprintf("GET - outpur sent, timer was %d, set to zero", Session->ResponseTimer);
				Session->ResponseTimer = 0;
			}
			
			Session->KillTimer = 0;
			return 0;				// Refresh has sent any available output
		}
	}

	else if (_stricmp(NodeURL, "/Node/InputLine.html") == 0)
	{
		ReplyLen = sprintf(ReplyBuffer, InputLine, Context, Context);
	}

SendResp:

	FormatTime(TimeString, time(NULL));

	HeaderLen = sprintf(Header, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n"
		"Date: %s\r\n\r\n", ReplyLen + strlen(Tail), TimeString);
	sendandcheck(sock, Header, HeaderLen);
	sendandcheck(sock, ReplyBuffer, ReplyLen);
	sendandcheck(sock, Tail, strlen(Tail));
	}
	return 0;

	}
	#include "StdExcept.c"
	}
}

char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char *dat[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};


VOID FormatTime(char * Time, time_t cTime)
{
	struct tm * TM;
	TM = gmtime(&cTime);

	wsprintf(Time, "%s, %02d %s %3d %02d:%02d:%02d GMT", dat[TM->tm_wday], TM->tm_mday, month[TM->tm_mon],
		TM->tm_year + 1900, TM->tm_hour, TM->tm_min, TM->tm_sec);

}

// Sun, 06 Nov 1994 08:49:37 GMT