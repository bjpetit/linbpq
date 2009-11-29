// Mail and Chat Server for BPQ32 Packet Switch
//
//	TCP access module - POP and SMTP

#include "stdafx.h"

#define MaxSockets 64

SocketConn * Sockets=NULL;

int CurrentConnections;

int CurrentSockets=0;

#define MAX_PENDING_CONNECTS 4

#define VERSION_MAJOR         2
#define VERSION_MINOR         0

SOCKADDR_IN local_sin;  /* Local socket - internet style */

PSOCKADDR_IN psin;

SOCKET smtpsock, pop3sock;

char szBuff[80];

int SMTPInPort;
int POP3InPort;
BOOL RemoteEmail;			// Set to listen on INADDR_ANY rather than LOCALHOST

BOOL ISP_Gateway_Enabled;

char MyDomain[50];			// Mail domain for BBS<>Internet Mapping

char ISPSMTPName[50];
int ISPSMTPPort;

char ISPPOP3Name[50];
int ISPPOP3Port;

char ISPAccountName[50];
char ISPAccountPass[50];
char EncryptedISPAccountPass[50];
int EncryptedPassLen;

BOOL SMTPAuthNeeded;

BOOL GMailMode = FALSE;
char GMailName[50];

int POP3Timer=9999;							// Run on startup
int ISPPOP3Interval;

BOOL SMTPMsgCreated=FALSE;					// Set to cause SMTP client to send messages to ISP
BOOL SMTPActive=FALSE;						// SO we don't try every 10 secs!

char mycd64[256];
static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char cd64[]="|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

char *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char *dat[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void decodeblock( unsigned char in[4], unsigned char out[3] );


int SendSock(SocketConn * sockptr, char * msg)
{
	int len = strlen(msg), sent;
	char * newmsg = malloc(len+10);

 	WriteLogLine(NULL, '>',msg,  len, LOG_TCP);

	strcpy(newmsg, msg);

	strcat(newmsg, "\r\n");

	len+=2;

	if (sockptr->SendBuffer)
	{
		// Already queued, so add to end

		if ((sockptr->SendSize + len) > sockptr->SendBufferSize)
		{
			sockptr->SendBufferSize += (10000 + len);
			sockptr->SendBuffer = realloc(sockptr->SendBuffer, sockptr->SendBufferSize);
		}

		memcpy(&sockptr->SendBuffer[sockptr->SendSize], newmsg, len);
		sockptr->SendSize += len;
		free (newmsg);
		return len;
	}
	
	sent = send(sockptr->socket, newmsg, len, 0);
		
	if (sent < len)
	{
		int error, remains;

		// Not all could be sent - queue rest

		if (sent == SOCKET_ERROR)
		{
			error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK)
				sent=0;

			//	What else??
		}

		remains = len - sent;

		sockptr->SendBufferSize += (10000 + remains);
		sockptr->SendBuffer = malloc(sockptr->SendBufferSize);

		memcpy(sockptr->SendBuffer, &newmsg[sent], remains);

		sockptr->SendSize = remains;
		sockptr->SendPtr = 0;

	}

	free (newmsg);

	return sent;
}

VOID __cdecl sockprintf(SocketConn * sockptr, const char * format, ...)
{
	// printf to a socket

	char buff[1000];
	va_list(arglist);
	
	va_start(arglist, format);
	vsprintf_s(buff, 1000, format, arglist);

	SendSock(sockptr, buff);
}

VOID TCPTimer()
{

	POP3Timer+=10;

	if (POP3Timer > ISPPOP3Interval)			// 5 mins
	{
		POP3Timer=0;

		if (ISPSMTPPort && ISP_Gateway_Enabled)
			SendtoISP();
		
		if (ISPPOP3Port  && ISP_Gateway_Enabled)
			POP3Connect(ISPPOP3Name, ISPPOP3Port);
	}
	else
	{
		if (SMTPMsgCreated && ISPSMTPPort && ISP_Gateway_Enabled)
			SendtoISP();
	}
}
BOOL InitialiseTCP()
{
	int			  Error;              // catches return value of WSAStartup
    WORD          VersionRequested;   // passed to WSAStartup
    WSADATA       WsaData;            // receives data from WSAStartup
	int i,j;


	for (i=0;i<64; i++)
	{
		j=cb64[i];
		mycd64[j]=i;
	}

    VersionRequested = MAKEWORD(VERSION_MAJOR, VERSION_MINOR);

	Error = WSAStartup(VersionRequested, &WsaData);
    
	if (Error)
	{
        MessageBox(NULL,
            "Could not find high enough version of WinSock",
            "BPQMailChat", MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        return FALSE;
	}

//	Create listening sockets


	if (SMTPInPort)
		smtpsock = CreateListeningSocket(SMTPInPort, WSA_ACCEPT);

	if (POP3InPort)
		pop3sock = CreateListeningSocket(POP3InPort, WSA_ACCEPT);

	if (ISP_Gateway_Enabled)
	{
		// See if using GMail

			char * ptr = strchr(ISPAccountName, '@');

		if (ptr)
		{
			if (_stricmp(&ptr[1], "gmail.com") == 0 || _stricmp(&ptr[1], "googlemail.com") == 0)
			{
				strcpy(GMailName, ISPAccountName);
				strlop(GMailName, '@');
				GMailMode = TRUE;
				SMTPAuthNeeded = TRUE;
			}
		}
	}

	return TRUE;

}


SOCKET CreateListeningSocket(int Port, int Message)
{
	SOCKET sock;
	int status;
	
	sock = socket( AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET)
	{
        sprintf(szBuff, "socket() failed error %d", WSAGetLastError());
		MessageBox(MainWnd, szBuff, "BPQMailChat", MB_OK);
		return FALSE;
        
	}
 
	psin=&local_sin;

	psin->sin_family = AF_INET;
	psin->sin_addr.s_addr = htonl(RemoteEmail ? INADDR_ANY  : INADDR_LOOPBACK);	// Local Host Olny
	
	psin->sin_port = htons(Port);        /* Convert to network ordering */

    if (bind( sock, (struct sockaddr FAR *) &local_sin, sizeof(local_sin)) == SOCKET_ERROR)
	{
         sprintf(szBuff, "bind(%d) failed Error %d", Port, WSAGetLastError());

         MessageBox(MainWnd, szBuff, "BPQMailChat", MB_OK);
         closesocket( sock );

		 return FALSE;
	}

    if (listen( sock, MAX_PENDING_CONNECTS ) < 0)
	{

		sprintf(szBuff, "listen(%d) failed Error %d", Port, WSAGetLastError());

		MessageBox(MainWnd, szBuff, "BPQMailChat", MB_OK);

		return FALSE;
	}
   
	if ((status = WSAAsyncSelect( sock, MainWnd, Message, FD_ACCEPT)) > 0)
	{

		sprintf(szBuff, "WSAAsyncSelect failed Error %d", WSAGetLastError());

		MessageBox(MainWnd, szBuff, "BPQMailChat", MB_OK);

		closesocket( sock );
		
		return FALSE;

	}

	return sock;
}

int Socket_Accept(int SocketId)
{
	int addrlen;
	SocketConn * sockptr;
	SOCKET sock;

//   Allocate a Socket entry

	sockptr=malloc(sizeof(SocketConn));
	memset(sockptr, 0, sizeof (SocketConn));

	sockptr->Next=Sockets;
	Sockets=sockptr;

	addrlen=sizeof(struct sockaddr);

	sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

	if (sock == INVALID_SOCKET)
	{
		sprintf(szBuff, " accept() failed Error %d", WSAGetLastError());
		MessageBox(MainWnd, szBuff, "BPQMailChat", MB_OK);
		return FALSE;
	}

	if (SocketId == pop3sock)
		sockptr->Type = POP3SLAVE;
	else
		sockptr->Type = SMTPServer;

	WSAAsyncSelect(sock, MainWnd, WSA_DATA,
			FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

	sockptr->socket = sock;


//			ShowApps();

	return 0;
}

int Socket_Connect(SOCKET sock, int Error)
{
	SocketConn * sockptr;

	//	Find Connection Record

	sockptr=Sockets;
		
	while (sockptr)
	{
		if (sockptr->socket == sock)
		{
			if (Error == 0)
			{
				sockptr->State = WaitingForGreeting;
			
				WSAAsyncSelect(sock, MainWnd, WSA_DATA,
						FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
			}
			else
				sockptr->State = 0;

			return 0;

		}

		sockptr = sockptr->Next;
	}


	return 0;
}


ReleaseSock(SOCKET sock)
{
	// remove and free the socket record

	SocketConn * sockptr, * lastptr;

	sockptr=Sockets;
	lastptr=NULL;
		
	while (sockptr)
	{
		if (sockptr->socket == sock)
		{
			if (lastptr)
				lastptr->Next=sockptr->Next;
			else
				Sockets=sockptr->Next;

			if (sockptr->POP3User)
				sockptr->POP3User->POP3Locked = FALSE;

			if (sockptr->State == WaitingForGreeting)
			{
				Logprintf(LOG_TCP, NULL, '|', "Premature Close on Socket %d", sock);
	
				if (sockptr->Type == SMTPClient)
					SMTPActive = FALSE;	
			}

			free(sockptr);
			return  0;
		}
		else
		{
			lastptr=sockptr;
			sockptr=sockptr->Next;
		}
	}

	return 0;
}


int Socket_Data(int sock, int error, int eventcode)
{
	SocketConn * sockptr;

	//	Find Connection Record

	sockptr=Sockets;
		
	while (sockptr)
	{
		if (sockptr->socket == sock)
		{
			switch (eventcode)
			{
				case FD_READ:

					return DataSocket_Read(sockptr,sock);

				case FD_WRITE:

					// Either Just connected, or flow contorl cleared

					if (sockptr->SendBuffer)
						// Data Queued
						SendFromQueue(sockptr);
					else
					{
						if (sockptr->Type == SMTPServer)
							SendSock(sockptr, "220 BPQMail SMTP Server ready");
						else
						{
							if (sockptr->Type == POP3SLAVE)
							{
								SendSock(sockptr, "+OK POP3 server ready");
								sockptr->State = GettingUser;
							}
						}
					}
					return 0;

				case FD_OOB:

					return 0;

				case FD_ACCEPT:

					return 0;

				case FD_CONNECT:

					return 0;

				case FD_CLOSE:

					closesocket(sock);
					ReleaseSock(sock);
					return 0;
				}
			return 0;
		}
		else
			sockptr=sockptr->Next;
	}

	return 0;
}

int DataSocket_Read(SocketConn * sockptr, SOCKET sock)
{
	int InputLen, MsgLen;
	char * ptr, * ptr2;
	char Buffer[2000];

	// May have several messages per packet, or message split over packets

	if (sockptr->InputLen > 1000)	// Shouldnt have lines longer  than this in text mode
	{
		sockptr->InputLen=0;
	}
				
	InputLen=recv(sock, &sockptr->TCPBuffer[sockptr->InputLen], 1000, 0);

	if (InputLen == 0)
		return 0;					// Does this mean closed?

	sockptr->InputLen += InputLen;

loop:
	
	ptr = memchr(sockptr->TCPBuffer, '\n', sockptr->InputLen);

	if (ptr)	//  CR in buffer
	{
		ptr2 = &sockptr->TCPBuffer[sockptr->InputLen];
		ptr++;				// Assume LF Follows CR

		if (ptr == ptr2)
		{
			// Usual Case - single meg in buffer
	
			if (sockptr->Type == SMTPServer)
				ProcessSMTPServerMessage(sockptr, sockptr->TCPBuffer, sockptr->InputLen);
			else
			if (sockptr->Type == POP3SLAVE)
				ProcessPOP3ServerMessage(sockptr, sockptr->TCPBuffer, sockptr->InputLen);
			else
			if (sockptr->Type == SMTPClient)
				ProcessSMTPClientMessage(sockptr, sockptr->TCPBuffer, sockptr->InputLen);
			else
			if (sockptr->Type == POP3Client)
				ProcessPOP3ClientMessage(sockptr, sockptr->TCPBuffer, sockptr->InputLen);

			sockptr->InputLen=0;
		
		}
		else
		{
			// buffer contains more that 1 message

			MsgLen = sockptr->InputLen - (ptr2-ptr);

			memcpy(Buffer, sockptr->TCPBuffer, MsgLen);


			if (sockptr->Type == SMTPServer)
				ProcessSMTPServerMessage(sockptr, Buffer, MsgLen);
			else
			if (sockptr->Type == POP3SLAVE)
				ProcessPOP3ServerMessage(sockptr, Buffer, MsgLen);
			else
			if (sockptr->Type == SMTPClient)
				ProcessSMTPClientMessage(sockptr, Buffer, MsgLen);
			else
			if (sockptr->Type == POP3Client)
				ProcessPOP3ClientMessage(sockptr, Buffer, MsgLen);


			memmove(sockptr->TCPBuffer, ptr, sockptr->InputLen-MsgLen);

			sockptr->InputLen -= MsgLen;

			goto loop;

		}
	}
	return 0;
}

VOID ProcessSMTPServerMessage(SocketConn * sockptr, char * Buffer, int Len)
{
	SOCKET sock;
	int i;
	time_t Date = 0;

	sock=sockptr->socket;

	WriteLogLine(NULL, '<',Buffer, Len-2, LOG_TCP);

	if (sockptr->Flags == GETTINGMESSAGE)
	{
		if(memcmp(Buffer, ".\r\n", 3) == 0)
		{
			// File Message

			char * ptr1, * ptr2;
			int linelen, MsgLen;
			char Msgtitle[62];

			// Scan headers for a Subject: or Date: Line (Headers end at black line)

			ptr1 = sockptr->MailBuffer;
		Loop:
			ptr2 = strchr(ptr1, '\r');

			if (ptr2 == NULL)
			{
				SendSock(sockptr, "500 Eh");
				return;
			}

			linelen = ptr2 - ptr1;

			if (_memicmp(ptr1, "Subject:", 8) == 0)
			{
				if (linelen > 68) linelen = 68;
				memcpy(Msgtitle, &ptr1[9], linelen-9);
				Msgtitle[linelen-9]=0;
			}

			if (_memicmp(ptr1, "Date:", 5) == 0)
			{
				struct tm rtime;
				char * Context;
				char seps[] = " ,\t\r";
				char Offset[10] = "";
				int i, HH, MM;

				memset(&rtime, 0, sizeof(struct tm));

				// Date: Tue, 9 Jun 2009 20:54:55 +0100

				ptr1 = strtok_s(&ptr1[5], seps, &Context);	// Skip Day
				ptr1 = strtok_s(NULL, seps, &Context);		// Day

				rtime.tm_mday = atoi(ptr1);

				ptr1 = strtok_s(NULL, seps, &Context);		// Month

				for (i=0; i < 12; i++)
				{
					if (strcmp(month[i], ptr1) == 0)
					{
						rtime.tm_mon = i;
						break;
					}
				}
		
				sscanf(Context, "%04d %02d%:%02d:%02d%s",
					&rtime.tm_year, &rtime.tm_hour, &rtime.tm_min, &rtime.tm_sec, Offset);

				rtime.tm_year -= 1900;

				Date = _mkgmtime(&rtime);
	
				if (Date == (time_t)-1)
					Date = 0;
				else
				{
					if ((Offset[0] == '+') || (Offset[0] == '-'))
					{
						MM = atoi(&Offset[3]);
						Offset[3] = 0;
						HH = atoi(&Offset[1]);
						MM = MM + (60 * HH);

						if (Offset[0] == '+')
							Date -= (60*MM);
						else
							Date += (60*MM);


					}
				}



			}

			if (linelen)			// Not Null line
			{
				ptr1 = ptr2 + 2;		// Skip crlf
				goto Loop;
			}

			ptr1 = sockptr->MailBuffer;

			MsgLen = sockptr->MailSize - (ptr2 - ptr1);

			// We Just want the from call, not the full address.
			
			TidyString(sockptr->MailFrom);
			
			strlop(sockptr->MailFrom, '@');
			if (strlen(sockptr->MailFrom) > 6) sockptr->MailFrom[6]=0;


			for (i=0; i < sockptr->Recipients; i++)
			{
				CreateSMTPMessage(sockptr, i, Msgtitle, Date, ptr2, MsgLen);

			}

			free(sockptr->RecpTo);
			free(sockptr->MailFrom);
			free(sockptr->MailBuffer);

			sockptr->MailBufferSize=0;
			sockptr->MailBuffer=0;
			sockptr->MailSize = 0;

	
			SendSock(sockptr, "250 Ok");
			
			//else			
			//	send(sock, "450 Ok\r\n", 8,0);

			sockptr->Flags = 0;
			sockptr->Recipients = 0;

			return;
		}

		if ((sockptr->MailSize + Len) > sockptr->MailBufferSize)
		{
			sockptr->MailBufferSize += 10000;
			sockptr->MailBuffer = realloc(sockptr->MailBuffer, sockptr->MailBufferSize);
	
			if (sockptr->MailBuffer == NULL)
			{
				CriticalErrorHandler("Failed to extend Message Buffer");
				shutdown(sock, 0);
				return;
			}
		}

		memcpy(&sockptr->MailBuffer[sockptr->MailSize], Buffer, Len);
		sockptr->MailSize += Len;

		return;
	}

	if (sockptr->State == GettingUser)
	{
		char Out[30];
		
		Buffer[Len-2]=0;

		decodeblock(Buffer, Out);
		decodeblock(&Buffer[4], &Out[3]);
		decodeblock(&Buffer[8], &Out[6]);
		decodeblock(&Buffer[12], &Out[9]);

		if (strlen(Out) > 10) Out[10] = 0;

		strcpy(sockptr->CallSign, Out);
		
		sockptr->State = GettingPass;
		SendSock(sockptr, "334 UGFzc3dvcmQ6");
		return;
	}

	if (sockptr->State == GettingPass)
	{
		struct UserInfo * user = NULL;
		char Out[30];

		Buffer[Len-2]=0;

		decodeblock(Buffer, Out);
		decodeblock(&Buffer[4], &Out[3]);
		decodeblock(&Buffer[8], &Out[6]);
		decodeblock(&Buffer[12], &Out[9]);
		decodeblock(&Buffer[16], &Out[12]);
		decodeblock(&Buffer[20], &Out[15]);

		user = LookupCall(sockptr->CallSign);

		if (user)
		{
			if (strcmp(user->pass, Out) == 0)
			{
				sockptr->State = Authenticated;
				SendSock(sockptr, "235 2.0.0 OK Authenticated"); //535 authorization failed
				return;
			}
		}

		SendSock(sockptr, "535 authorization failed");
		sockptr->State = 0;
		return;
	}



/*AUTH LOGIN

334 VXNlcm5hbWU6
a4msl9ux
334 UGFzc3dvcmQ6
ZvVx9G1hcg==
235 2.0.0 OK Authenticated
*/


	if(memcmp(Buffer, "AUTH LOGIN", 10) == 0)
	{
		sockptr->State = GettingUser;
		SendSock(sockptr, "334 VXNlcm5hbWU6");
		return;
	}

	if(memcmp(Buffer, "EHLO",4) == 0)
	{
		SendSock(sockptr, "250-BPQ Mail Server");
		SendSock(sockptr, "250 AUTH LOGIN");

		//250-8BITMIME

		return;
	}

	if(memcmp(Buffer, "AUTH LOGIN", 10) == 0)
	{
		sockptr->State = GettingUser;
		SendSock(sockptr, "334 VXNlcm5hbWU6");
		return;
	}


	if(memcmp(Buffer, "HELO",4) == 0)
	{
		SendSock(sockptr, "250 Ok");
		return;
	}
	
	if(_memicmp(Buffer, "MAIL FROM:", 10) == 0)
	{
		sockptr->MailFrom = zalloc(Len);
		memcpy(sockptr->MailFrom, &Buffer[10], Len-12);
			
		SendSock(sockptr, "250 Ok");

		return;
	}

	if(_memicmp(Buffer, "RCPT TO:", 8) == 0)
	{
		sockptr->RecpTo=realloc(sockptr->RecpTo, (sockptr->Recipients+1)*4);
		sockptr->RecpTo[sockptr->Recipients] = zalloc(Len);

		memcpy(sockptr->RecpTo[sockptr->Recipients++], &Buffer[8], Len-10);
			
		SendSock(sockptr, "250 Ok");
		return;
	}

	if(memcmp(Buffer, "DATA\r\n", 6) == 0)
	{
		sockptr->MailBuffer=malloc(10000);
		sockptr->MailBufferSize=10000;

		if (sockptr->MailBuffer == NULL)
		{
			CriticalErrorHandler("Failed to create SMTP Message Buffer");
			SendSock(sockptr, "250 Failed");
			shutdown(sock, 0);

			return;
		}
	
		sockptr->Flags |= GETTINGMESSAGE;

		SendSock(sockptr, "354 End data with <CR><LF>.<CR><LF>");
		return;
	}

	if(memcmp(Buffer, "QUIT\r\n", 6) == 0)
	{
		SendSock(sockptr, "221 OK");
		Sleep(500);
		shutdown(sock, 0);
		return;
	}

	if(memcmp(Buffer, "RSET\r\n", 6) == 0)
	{
		SendSock(sockptr, "250 Ok");
		sockptr->State = 0;
		sockptr->Recipients;
//		Sleep(500);
//		shutdown(sock, 0);
		return;
	}


	return;
}


CreateSMTPMessage(SocketConn * sockptr, int i, char * MsgTitle, time_t Date, char * MsgBody, int MsgLen)
{
	struct MsgInfo * Msg;
	BIDRec * BIDRec;

	char * via;

	// Allocate a message Record slot

	Msg = AllocateMsgRecord();
		
	// Set number here so they remain in sequence
		
	Msg->number = ++LatestMsg;
	MsgnotoMsg[Msg->number] = Msg;
	Msg->length = MsgLen;

	sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

	Msg->type = 'P';
	Msg->status = 'N';

	BIDRec = AllocateBIDRecord();

	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);


	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	if (Date)
		Msg->datecreated = Date;

	TidyString(sockptr->RecpTo[i]);

	via = strlop(sockptr->RecpTo[i], '@');

	if (via)
	{
		if (strlen(via) > 40) via[40] = 0;

		strcpy(Msg->via, via);		// Save before messing with it

		strlop(via, '.');			// Get first part of address

		if (_stricmp(via, BBSName) == 0)
		{
			// sent via us - clear the name

			Msg->via[0] = 0;
		}
	}

	if (strlen(sockptr->RecpTo[i]) > 6) sockptr->RecpTo[i][6]=0;

	strcpy(Msg->to, sockptr->RecpTo[i]);

	strcpy(Msg->from, sockptr->MailFrom);

	strcpy(Msg->title, MsgTitle);

	free(sockptr->RecpTo[i]);

	// Set up forwarding bitmap

	MatchMessagetoBBSList(Msg, 0);

	return CreateSMTPMessageFile(MsgBody, Msg);
		
}


BOOL CreateSMTPMessageFile(char * Message, struct MsgInfo * Msg)
{
	char MsgFile[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;
	int WriteLen=0;
	char Mess[255];
	int len;

	// Remove lf chars

	/*

	ptr1 = ptr2 = Message;
	len = Msg->length;

	while (len-- > 0)
	{
		*ptr2 = *ptr1;
	
		if (*ptr1 == '\r')
			if (*(ptr1+1) == '\n')
			{
				ptr1++;
				len--;
			}
		ptr1++;
		ptr2++;

	}

	Msg->length = ptr2 - Message;
	*/

	sprintf_s(MsgFile, sizeof(MsgFile), "%s\\m_%06d.mes", MailDir, Msg->number);
	
	hFile = CreateFile(MsgFile,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		WriteFile(hFile, Message, Msg->length, &WriteLen, NULL);
		CloseHandle(hFile);
	}

	if (WriteLen != Msg->length)
	{
		len = sprintf_s(Mess, sizeof(Mess), "Failed to create Message File\r");
		CriticalErrorHandler(Mess);

		return FALSE;
	}

	SaveMessageDatabase();
	SaveBIDDatabase();

	return TRUE;
}

TidyString(char * Address)
{
	// Cleans up a From: or To: Address

	// May have leading or trailing spaces, or be enclosed by <>,  or have a " " part

	// From: "John Wiseman" <john.wiseman@ntlworld.com>

	char * ptr1, * ptr2;

	_strupr(Address);

	ptr1 = Address;

	while (*ptr1 == ' ') ptr1++;

	if (*ptr1 == '"')
	{
		ptr1++;
		ptr1=strlop(ptr1, '"');
		ptr2=strlop(ptr1, ' ');
		ptr1=ptr2;
	}

	if (*ptr1 == '<') ptr1++;

	ptr2 = strlop(ptr1, '>');
	strlop(ptr1, ' ');

	strcpy(Address, ptr1);

	return 0;
}
/*
+OK POP3 server ready
USER john.wiseman
+OK please send PASS command
PASS gb7bpq
+OK john.wiseman is welcome here
STAT
+OK 6 115834

UIDL
+OK 6 messages
1 <4A0DC6E0.5020504@hb9bza.net>
2 <gul8gb+of9r@eGroups.com>
3 <1085101c9d5d0$09b15420$16f9280a@phx.gbl>
4 <gul9ms+qkht@eGroups.com>
5 <B0139742084@email.bigvalley.net>
6 <20090516011401.53DB013804@panix1.panix.com>
.
LIST
+OK 6 messages
1 7167
2 10160
3 52898
4 4746
5 20218
6 20645
.

*/

VOID ProcessPOP3ServerMessage(SocketConn * sockptr, char * Buffer, int Len)
{
	SOCKET sock;
	int i;
	struct MsgInfo * Msg;

	sock=sockptr->socket;

	WriteLogLine(NULL, '<',Buffer, Len-2, LOG_TCP);

	if(memcmp(Buffer, "CAPA",4) == 0)
	{
		SendSock(sockptr, "+OK Capability list follows");
		SendSock(sockptr, "UIDL");
		SendSock(sockptr, "EXPIRE 30");
		SendSock(sockptr, ".");
		return;
	}  

	if(memcmp(Buffer, "AUTH",4) == 0)
	{
		SendSock(sockptr, "-ERR");
		return;
	}  
	if (sockptr->State == GettingUser)
	{
		
		Buffer[Len-2]=0;
		if (Len > 15) Buffer[15]=0;

		strcpy(sockptr->CallSign, &Buffer[5]);
		
		sockptr->State = GettingPass;
		SendSock(sockptr, "+OK please send PASS command");
		return;
	}

	if (sockptr->State == GettingPass)
	{
		struct UserInfo * user = NULL;

		Buffer[Len-2]=0;
		user = LookupCall(sockptr->CallSign);

		if (user)
		{
			if (strcmp(user->pass, &Buffer[5]) == 0)
			{
				if (user->POP3Locked)
				{
					SendSock(sockptr, "-ERR Mailbox Locked");
					sockptr->State = 0;
					return;
				}

				sockptr->State = Authenticated;
				SendSock(sockptr, "+OK Authenticated");

				sockptr->POP3User = user;
				user->POP3Locked = TRUE;

				// Get Message List

				for (i=0; i<=NumberofMessages; i++)
				{
					Msg = MsgHddrPtr[i];
					
					if ((_stricmp(Msg->to, sockptr->CallSign) == 0) ||
						((_stricmp(Msg->to, "SYSOP") == 0) && (user->flags & F_SYSOP) && (Msg->type == 'P')))
					{
						if (Msg->status != 'K')
						{
							sockptr->POP3Msgs = realloc(sockptr->POP3Msgs, (sockptr->POP3MsgCount+1)*4);
							sockptr->POP3Msgs[sockptr->POP3MsgCount++] = MsgHddrPtr[i];
						}
					}
				}

				return;
			}
		}

		SendSock(sockptr, "-ERR Authentication failed");
		sockptr->State = 0;
		return;
	}

	if (memcmp(Buffer, "QUIT",4) == 0)
	{
		SendSock(sockptr, "+OK Finished");

		if (sockptr->POP3User)
			sockptr->POP3User->POP3Locked = FALSE;

		return;
	}


	if (memcmp(Buffer, "NOOP",4) == 0)
	{
		SendSock(sockptr, "+OK ");
		return;
	}

	if (sockptr->State != Authenticated)
	{
		SendSock(sockptr, "-ERR Need Authentication");
		sockptr->State = 0;
		return;
	}

	if (memcmp(Buffer, "STAT",4) == 0)
	{
		char reply[40];
		int i, size=0;

		for (i=0; i< sockptr->POP3MsgCount; i++)
		{
			size+=sockptr->POP3Msgs[i]->length;
		}

		sprintf_s(reply, sizeof(reply), "+OK %d %d", sockptr->POP3MsgCount, size);

		SendSock(sockptr, reply);
		return;
	}

	if (memcmp(Buffer, "UIDL",4) == 0)
	{
		char reply[40];
		int i, count=0, size=0;
		int MsgNo=1;

		SendSock(sockptr, "+OK ");

		for (i=0; i< sockptr->POP3MsgCount; i++)
		{
			sprintf_s(reply, sizeof(reply), "%d %s", i+1, sockptr->POP3Msgs[i]->bid);
			SendSock(sockptr, reply);	
		}

		SendSock(sockptr, ".");
		return;
	}

	if (memcmp(Buffer, "LIST",4) == 0)
	{
		char reply[40];
		int i, count=0, size=0;
		int MsgNo=1;

		SendSock(sockptr, "+OK ");

		for (i=0; i< sockptr->POP3MsgCount; i++)
		{
			sprintf_s(reply, sizeof(reply), "%d %d", i+1, sockptr->POP3Msgs[i]->length);
			SendSock(sockptr, reply);	
		}

		SendSock(sockptr, ".");
		return;
	}

	if (memcmp(Buffer, "RETR",4) == 0)
	{
		char * ptr;		
		char Header[120];
		int i, count=0, size=0;
		int MsgNo=1;
		char * msgbytes;
		struct MsgInfo * Msg;

		ptr=strlop(Buffer, ' ');			// Get Number

		i=atoi(ptr);

		if ((i > sockptr->POP3MsgCount)  || (i == 0))
		{
			SendSock(sockptr, "-ERR no such message");
			return;
		}

		Msg = sockptr->POP3Msgs[i-1];

		msgbytes = ReadMessageFile(Msg->number);

		if (msgbytes == NULL)
		{
			SendSock(sockptr, "-ERR no such message");
			return;
		}

		SendSock(sockptr, "+OK ");

		// Build an RFC822 ish header

//Received: from [69.147.65.148] by n15.bullet.sp1.yahoo.com with NNFMP; 16 May 2009 02:30:47 -0000
//Received: from [69.147.108.192] by t11.bullet.mail.sp1.yahoo.com with NNFMP; 16 May 2009 02:30:47 -0000

		sprintf_s(Header, sizeof(Header), "To: %s", Msg->to);
		SendSock(sockptr, Header);
		
		sprintf_s(Header, sizeof(Header), "Message-ID: %s", Msg->bid);
		SendSock(sockptr, Header);
		sprintf_s(Header, sizeof(Header), "From: %s", Msg->from);
		SendSock(sockptr, Header);
//Sender: shipplotter@yahoogroups.com
//		sprintf_s(Header, sizeof(Header), "Date: %s", Msg->date);
//		SendSock(sockptr, Header);
		sprintf_s(Header, sizeof(Header), "Subject: %s", Msg->title);
		SendSock(sockptr, Header);
//Content-Type: text/plain; charset=ISO-8859-1
//Content-Transfer-Encoding: 7bit

		SendSock(sockptr, "");							// Blank line before body

		SendSock(sockptr, msgbytes);
		SendSock(sockptr, "");
		SendSock(sockptr, ".");

		free(msgbytes);

		return;

	}


	if (memcmp(Buffer, "DELE",4) == 0)
	{
		char * ptr;		
		int i;
		struct MsgInfo * Msg;

		ptr=strlop(Buffer, ' ');			// Get Number

		i=atoi(ptr);

		if ((i > sockptr->POP3MsgCount)  || (i == 0))
		{
			SendSock(sockptr, "-ERR no such message");
			return;
		}

		Msg = sockptr->POP3Msgs[i-1];

		FlagAsKilled(Msg);

		SendSock(sockptr, "+OK ");
		return;
	}


	if (memcmp(Buffer, "QUIT",4) == 0)
	{
		SendSock(sockptr, "+OK Finished");

		if (sockptr->POP3User)
			sockptr->POP3User->POP3Locked = FALSE;

		return;
	}

	SendSock(sockptr, "-ERR Unrecognised Command");

}



/* jer:
 * This is the original file, my mods were only to change the name/semantics on the b64decode function
 * and remove some dependencies.
 */
/*
	LibCGI base64 manipulation functions is extremly based on the work of Bob Tower,
	from its projec http://base64.sourceforge.net. The functions were a bit modicated. 
	Above is the MIT license from b64.c original code:

LICENCE:        Copyright (c) 2001 Bob Trower, Trantor Standard Systems Inc.

                Permission is hereby granted, free of charge, to any person
                obtaining a copy of this software and associated
                documentation files (the "Software"), to deal in the
                Software without restriction, including without limitation
                the rights to use, copy, modify, merge, publish, distribute,
                sublicense, and/or sell copies of the Software, and to
                permit persons to whom the Software is furnished to do so,
                subject to the following conditions:

                The above copyright notice and this permission notice shall
                be included in all copies or substantial portions of the
                Software.

                THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
                KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
                WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
                PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
                OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
                OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
                OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
                SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

*/
void encodeblock( unsigned char in[3], unsigned char out[4], int len )
{
    out[0] = cb64[ in[0] >> 2 ];
    out[1] = cb64[ ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4) ];
    out[2] = (unsigned char) (len > 1 ? cb64[ ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6) ] : '=');
    out[3] = (unsigned char) (len > 2 ? cb64[ in[2] & 0x3f ] : '=');
}

void decodeblock( unsigned char in[4], unsigned char out[3] )
{   
    in[0]=mycd64[in[0]];
    in[1]=mycd64[in[1]];
    in[2]=mycd64[in[2]];
    in[3]=mycd64[in[3]];

	out[0] = (unsigned char ) (in[0] << 2 | in[1] >> 4);
    out[1] = (unsigned char ) (in[1] << 4 | in[2] >> 2);
    out[2] = (unsigned char ) (((in[2] << 6) & 0xc0) | in[3]);
}

/** 
* @ingroup libcgi_string
* @{
*/

/**
* Encodes a given tring to its base64 form.
* 
* @param *str String to convert
* @return Base64 encoded String
* @see str_base64_decode
**/
char *str_base64_encode(char *str)
{
    unsigned int i = 0, j = 0, len = strlen(str);
	char *tmp = str;
	char *result = (char *)zalloc((len+1)*4);
	
	if (!result)
		return NULL;

	while (len  > 2 )
	{
		encodeblock(&str[i], &result[j],3);
		i+=3;
		j+=4;
		len -=3;
	}
	if (len)
	{
		encodeblock(&str[i], &result[j], len);
	}

	return result;
}

BOOL SMTPConnect(char * Host, int Port, struct MsgInfo * Msg, char * MsgBody)
{
	int err, status;
	u_long param=1;
	BOOL bcopt=TRUE;

	SocketConn * sockptr;

	SOCKADDR_IN sinx; 
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;

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
 			Logprintf(LOG_TCP, NULL, '|', "Resolve Failed for SMTP Server %s", Host);
			SMTPActive = FALSE;
			return FALSE;			// Resolve failed
		 }
		 memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

//   Allocate a Socket entry

	sockptr=malloc(sizeof(SocketConn));
	memset(sockptr, 0, sizeof (SocketConn));

	sockptr->Next=Sockets;
	Sockets=sockptr;

	sockptr->socket=socket(AF_INET,SOCK_STREAM,0);

	if (sockptr->socket == INVALID_SOCKET)
	{
  	 	return FALSE; 
	}

	sockptr->Type = SMTPClient;
	
	sockptr->SMTPMsg = Msg;
	sockptr->MailBuffer = MsgBody;

	ioctlsocket (sockptr->socket, FIONBIO, &param);
 
	setsockopt (sockptr->socket, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sockptr->socket, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
  	 	return FALSE; 
	}

	if ((status = WSAAsyncSelect(sockptr->socket, MainWnd, WSA_CONNECT, FD_CONNECT)) > 0)
	{
		closesocket(sockptr->socket);
		return FALSE;
	}


	if (connect(sockptr->socket,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		sockptr->State = WaitingForGreeting;

		return TRUE;
	}
	else
	{
		err=WSAGetLastError();

		if (err == WSAEWOULDBLOCK)
		{
			//
			//	Connect in Progress
			//

			return TRUE;
		}
		else
		{
			//
			//	Connect failed
			//

			closesocket(sockptr->socket);

			sockptr->State =0;

			return FALSE;
		}
	}
	return FALSE;

}

VOID ProcessSMTPClientMessage(SocketConn * sockptr, char * Buffer, int Len)
{
	SOCKET sock;

	sock=sockptr->socket;

	WriteLogLine(NULL, '<',Buffer, Len-2, LOG_TCP);

	Buffer[Len] = 0;

	if (sockptr->State == WaitingForGreeting)
	{
		if (memcmp(Buffer, "220 ",4) == 0)
		{
			sockprintf(sockptr, "HELO %s", BBSName);
			sockptr->State = WaitingForHELOResponse;
		}
		else
		{
			SendSock(sockptr, "QUIT");
			sockptr->State = 0;
		}

		return;
	}

	if (sockptr->State == WaitingForHELOResponse)
	{
		if (memcmp(Buffer, "250 ",4) == 0)
		{
			if (SMTPAuthNeeded)
			{
				sockprintf(sockptr, "AUTH LOGIN");
				sockptr->State = WaitingForAUTHResponse;
			}
			else
			{
				sockprintf(sockptr, "MAIL FROM: <%s@%s>", sockptr->SMTPMsg->from, MyDomain);
				sockptr->State = WaitingForFROMResponse;
			}
		}
		else
		{
			SendSock(sockptr, "QUIT");
			sockptr->State = 0;
			SMTPActive = FALSE;

		}

		return;
	}

	if (sockptr->State == WaitingForAUTHResponse)
	{
		if (memcmp(Buffer, "334 VXN", 7) == 0)
		{
			char * Msg = str_base64_encode(ISPAccountName);
			SendSock(sockptr, Msg);
			free(Msg);
			return;
		}
		else if (memcmp(Buffer, "334 UGF", 7) == 0)
		{
			char * Msg = str_base64_encode(ISPAccountPass);
			SendSock(sockptr, Msg);
			free(Msg);
			return;
		}
		else if (memcmp(Buffer, "235 ", 4) == 0)
		{
			sockprintf(sockptr, "MAIL FROM: <%s@%s>", sockptr->SMTPMsg->from, MyDomain);
//			sockprintf(sockptr, "MAIL FROM: <%s@%s.%s>", sockptr->SMTPMsg->from, BBSName, HRoute);
			sockptr->State = WaitingForFROMResponse;
		}

		else
		{
			SendSock(sockptr, "QUIT");
			sockptr->State = 0;
			SMTPActive = FALSE;
		}

		return;

	}


	if (sockptr->State == WaitingForFROMResponse)
	{
		if (memcmp(Buffer, "250 ",4) == 0)
		{
			sockprintf(sockptr, "RCPT TO: <%s>", sockptr->SMTPMsg->via);
			sockptr->State = WaitingForTOResponse;
		}
		else
		{
			sockptr->SMTPMsg->status = 'H';			// Hold for review
			SendSock(sockptr, "QUIT");
			sockptr->State = 0;
			SMTPActive = FALSE;
		}

		return;
	}

	if (sockptr->State == WaitingForTOResponse)
	{
		if (memcmp(Buffer, "250 ",4) == 0)
		{
			SendSock(sockptr, "DATA");
			sockptr->State = WaitingForDATAResponse;
		}
		else
		{
			sockptr->SMTPMsg->status = 'H';			// Hold for review
			SendSock(sockptr, "QUIT");
			sockptr->State = 0;
			SMTPActive = FALSE;
		}

		return;
	}

	if (sockptr->State == WaitingForDATAResponse)
	{
		if (memcmp(Buffer, "354 ",4) == 0)
		{
			sockprintf(sockptr, "To: %s", sockptr->SMTPMsg->via);
			sockprintf(sockptr, "From: %s <%s@%s>", sockptr->SMTPMsg->from, sockptr->SMTPMsg->from, MyDomain);
			sockprintf(sockptr, "Sender: %s@%s", sockptr->SMTPMsg->from, MyDomain);
			if (GMailMode)
				sockprintf(sockptr, "Reply-To: %s+%s@%s", GMailName, sockptr->SMTPMsg->from, MyDomain);
			else
				sockprintf(sockptr, "Reply-To: %s@%s", sockptr->SMTPMsg->from, MyDomain);

				
			sockprintf(sockptr, "Subject: %s", sockptr->SMTPMsg->title);
			SendSock(sockptr, "");

			SendSock(sockptr, sockptr->MailBuffer);
			SendSock(sockptr, ".");

			sockptr->State = WaitingForBodyResponse;
		}
		else
		{
			SendSock(sockptr, "QUIT");
			sockptr->State = 0;
			SMTPActive = FALSE;
		}

		return;
	}

	if (sockptr->State == WaitingForBodyResponse)
	{
		if (memcmp(Buffer, "250 ",  4) == 0)
		{
			sockptr->SMTPMsg->status = 'F';
		}


		SendSock(sockptr, "QUIT");
		sockptr->State = 0;

		SMTPActive = FALSE;

		SMTPMsgCreated=TRUE;					// See if any more

		return;
	}


}	

BOOL SendtoISP()
{
	// Find a message intended for the Internet and send it

	int m = NumberofMessages;
	char * Body;

	struct MsgInfo * Msg;

	if (SMTPActive)
		return FALSE;

	do
	{
		Msg=MsgHddrPtr[m];

		if ((Msg->status == 'N') && (Msg->to[0] == 0) && (Msg->from[0] != 0))
		{
			// Make sure message exists

			Body = ReadMessageFile(Msg->number);

			if (Body == NULL)
			{
				FlagAsKilled(Msg);
				return FALSE;
			}

			Logprintf(LOG_TCP, NULL, '|', "Connecting to Server %s to send Msg %d", ISPSMTPName, Msg->number);

			SMTPConnect(ISPSMTPName, ISPSMTPPort, Msg, Body);

			SMTPActive = TRUE;

			return TRUE;
		}

		m--;

	} while (m> 0);

	return FALSE;

}

BOOL POP3Connect(char * Host, int Port)
{
	int err, status;
	u_long param=1;
	BOOL bcopt=TRUE;

	SocketConn * sockptr;

	SOCKADDR_IN sinx; 
	SOCKADDR_IN destaddr;
	int addrlen=sizeof(sinx);
	struct hostent * HostEnt;

	Logprintf(LOG_TCP, NULL, '|', "Connecting to POP3 Server %s", Host);

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
			Logprintf(LOG_TCP, NULL, '|', "Resolve Failed for POP3 Server %s", Host);
			return FALSE;			// Resolve failed
		 }
		 memcpy(&destaddr.sin_addr.s_addr,HostEnt->h_addr,4);
	}

//   Allocate a Socket entry

	sockptr=malloc(sizeof(SocketConn));
	memset(sockptr, 0, sizeof (SocketConn));

	sockptr->Next=Sockets;
	Sockets=sockptr;

	sockptr->socket=socket(AF_INET,SOCK_STREAM,0);

	if (sockptr->socket == INVALID_SOCKET)
	{
  	 	return FALSE; 
	}

	sockptr->Type = POP3Client;
	
	ioctlsocket (sockptr->socket, FIONBIO, &param);
 
	setsockopt (sockptr->socket, SOL_SOCKET, SO_REUSEADDR, (const char FAR *)&bcopt,4);

	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;
	sinx.sin_port = 0;

	if (bind(sockptr->socket, (LPSOCKADDR) &sinx, addrlen) != 0 )
	{
		//
		//	Bind Failed
		//
	
  	 	return FALSE; 
	}

	if ((status = WSAAsyncSelect(sockptr->socket, MainWnd, WSA_CONNECT, FD_CONNECT)) > 0)
	{
		closesocket(sockptr->socket);
		return FALSE;
	}


	if (connect(sockptr->socket,(LPSOCKADDR) &destaddr, sizeof(destaddr)) == 0)
	{
		//
		//	Connected successful
		//

		sockptr->State = WaitingForGreeting;

		return TRUE;
	}
	else
	{
		err=WSAGetLastError();

		if (err == WSAEWOULDBLOCK)
		{
			//
			//	Connect in Progressing
			//

			return TRUE;
		}
		else
		{
			//
			//	Connect failed
			//

			closesocket(sockptr->socket);

			sockptr->State =0;

			return FALSE;
		}
	}
	return FALSE;
}

VOID ProcessPOP3ClientMessage(SocketConn * sockptr, char * Buffer, int Len)
{
	SOCKET sock;
	time_t Date;

	sock=sockptr->socket;

	WriteLogLine(NULL, '<',Buffer, Len-2, LOG_TCP);

	if (sockptr->Flags == GETTINGMESSAGE)
	{
		if(memcmp(Buffer, ".\r\n", 3) == 0)
		{
			// File Message

			char * ptr1, * ptr2;
			int linelen, MsgLen;
			char MsgFrom[62], MsgTo[62], Msgtitle[62];

			// Scan headers for From: To: and Subject: Line (Headers end at blank line)

			ptr1 = sockptr->MailBuffer;
		Loop:
			ptr2 = strchr(ptr1, '\r');

			if (ptr2 == NULL)
			{
				SendSock(sockptr, "500 Eh");
				return;
			}

			linelen = ptr2 - ptr1;

			// From: "John Wiseman" <john.wiseman@ntlworld.com>
			// To: <G8BPQ@g8bpq.org.uk>
			//<To: <gm8bpq+g8bpq@googlemail.com>


			if (_memicmp(ptr1, "From:", 5) == 0)
			{
				if (linelen > 65) linelen = 65;
				memcpy(MsgFrom, ptr1, linelen);
				MsgFrom[linelen]=0;
			}
			else
			if (_memicmp(ptr1, "To:", 3) == 0)
			{
				if (linelen > 63) linelen = 63;
				memcpy(MsgTo, &ptr1[4], linelen-4);
				MsgTo[linelen-4]=0;
			}
			else
			if (_memicmp(ptr1, "Subject:", 8) == 0)
			{
				if (linelen > 68) linelen = 68;
				memcpy(Msgtitle, &ptr1[9], linelen-9);
				Msgtitle[linelen-9]=0;
			}
			else
			if (_memicmp(ptr1, "Date:", 5) == 0)
			{
				struct tm rtime;
				char * Context;
				char seps[] = " ,\t\r";
				char Offset[10] = "";
				int i, HH, MM;

				memset(&rtime, 0, sizeof(struct tm));

				// Date: Tue, 9 Jun 2009 20:54:55 +0100

				ptr1 = strtok_s(&ptr1[5], seps, &Context);	// Skip Day
				ptr1 = strtok_s(NULL, seps, &Context);		// Day

				rtime.tm_mday = atoi(ptr1);

				ptr1 = strtok_s(NULL, seps, &Context);		// Month

				for (i=0; i < 12; i++)
				{
					if (strcmp(month[i], ptr1) == 0)
					{
						rtime.tm_mon = i;
						break;
					}
				}
		
				sscanf(Context, "%04d %02d%:%02d:%02d%s",
					&rtime.tm_year, &rtime.tm_hour, &rtime.tm_min, &rtime.tm_sec, Offset);

				rtime.tm_year -= 1900;

				Date = _mkgmtime(&rtime);
				
				if (Date == (time_t)-1)
					Date = 0;
				else
				{
					if ((Offset[0] == '+') || (Offset[0] == '-'))
					{
						MM = atoi(&Offset[3]);
						Offset[3] = 0;
						HH = atoi(&Offset[1]);
						MM = MM + (60 * HH);

						if (Offset[0] == '+')
							Date -= (60*MM);
						else
							Date += (60*MM);


					}
				}
			}

			
			if (linelen)			// Not Null line
			{
				ptr1 = ptr2 + 2;		// Skip crlf
				goto Loop;
			}

			ptr1 = sockptr->MailBuffer;

			// Put the From Address into the message, and set from to smtp:
			
			// From: "John Wiseman" <john.wiseman@cantab.net>

			*(--ptr2) = '\n';
			*(--ptr2) = '\r';

			ptr2 -= strlen(MsgFrom);
			memcpy(ptr2, MsgFrom, strlen(MsgFrom));

			*(--ptr2) = '\n';
			*(--ptr2) = '\r';

			MsgLen = sockptr->MailSize - (ptr2 - ptr1);
				
			CreatePOP3Message("SMTP:", MsgTo, Msgtitle, Date, ptr2, MsgLen);

			free(sockptr->MailBuffer);
			sockptr->MailBufferSize=0;
			sockptr->MailBuffer=0;
			sockptr->MailSize = 0;

			sockptr->Flags &= ~GETTINGMESSAGE;

			if (sockptr->POP3MsgCount > sockptr->POP3MsgNum++)
			{
				sockprintf(sockptr, "RETR %d", sockptr->POP3MsgNum);

				sockptr->State = WaitingForRETRResponse;
			}
			else
			{
				sockptr->POP3MsgNum = 1;
				sockprintf(sockptr, "DELE %d", sockptr->POP3MsgNum);;
				sockptr->State = WaitingForDELEResponse;
			}

			return;
		}

		if ((sockptr->MailSize + Len) > sockptr->MailBufferSize)
		{
			sockptr->MailBufferSize += 10000;
			sockptr->MailBuffer = realloc(sockptr->MailBuffer, sockptr->MailBufferSize);
	
			if (sockptr->MailBuffer == NULL)
			{
				CriticalErrorHandler("Failed to extend Message Buffer");
				shutdown(sock, 0);
				return;
			}
		}

		memcpy(&sockptr->MailBuffer[sockptr->MailSize], Buffer, Len);
		sockptr->MailSize += Len;

		return;
	}

	if (sockptr->State == WaitingForGreeting)
	{
		if (memcmp(Buffer, "+OK", 3) == 0)
		{
			sockprintf(sockptr, "USER %s", ISPAccountName);
			sockptr->State = WaitingForUSERResponse;
		}
		else
		{
			SendSock(sockptr, "QUIT");
			sockptr->State = 0;
		}

		return;
	}

	if (sockptr->State == WaitingForUSERResponse)
	{
		if (memcmp(Buffer, "+OK", 3) == 0)
		{
			sockprintf(sockptr, "PASS %s", ISPAccountPass);
			sockptr->State = WaitingForPASSResponse;
		}
		else
		{
			SendSock(sockptr, "QUIT");
			sockptr->State = WaitingForQUITResponse;
		}

		return;
	}

	if (sockptr->State == WaitingForPASSResponse)
	{
		if (memcmp(Buffer, "+OK", 3) == 0)
		{
			SendSock(sockptr, "STAT");
			sockptr->State = WaitingForSTATResponse;
		}
		else
		{
			shutdown(sock, 0);
			sockptr->State = 0;
		}

		return;
	}

	if (sockptr->State == WaitingForSTATResponse)
	{
		if (memcmp(Buffer, "+OK", 3) == 0)
		{
			int Msgs = atoi(&Buffer[3]);
			
			if (Msgs > 0)
			{
				sockptr->POP3MsgCount = Msgs;
				sockptr->POP3MsgNum = 1;
				SendSock(sockptr, "RETR 1");

				sockptr->State = WaitingForRETRResponse;

			}
			else
			{
				SendSock(sockptr, "QUIT");
				sockptr->State = WaitingForQUITResponse;
			}
		}
		else
		{
			SendSock(sockptr, "QUIT");
			sockptr->State = WaitingForQUITResponse;
		}

		return;
	}

	if (sockptr->State == WaitingForRETRResponse)
	{
		if (memcmp(Buffer, "+OK", 3) == 0)
		{
			sockptr->MailBuffer=malloc(10000);
			sockptr->MailBufferSize=10000;

			if (sockptr->MailBuffer == NULL)
			{
				CriticalErrorHandler("Failed to create POP3 Message Buffer");
				SendSock(sockptr, "QUIT");
				sockptr->State = WaitingForQUITResponse;
				shutdown(sock, 0);

				return;
			}
	
			sockptr->Flags |= GETTINGMESSAGE;
		
		}
		else
		{
			SendSock(sockptr, "QUIT");
			sockptr->State = WaitingForQUITResponse;
		}

		return;
	}
	if (sockptr->State == WaitingForDELEResponse)
	{
		if (memcmp(Buffer, "+OK", 3) == 0)
		{
			if (sockptr->POP3MsgCount > sockptr->POP3MsgNum++)
			{
				sockprintf(sockptr, "DELE %d", sockptr->POP3MsgNum);;
			}
			else
			{
				SendSock(sockptr, "QUIT");
				sockptr->Flags = WaitingForQUITResponse;
			}
		}
		else
		{
			shutdown(sock,0);
			sockptr->State = 0;
		}
		return;
	}

	if (sockptr->State == WaitingForQUITResponse)
	{
		shutdown(sock,0);
		sockptr->State = 0;
		return;
	}

	SendSock(sockptr, "QUIT");
	shutdown(sock,0);
	sockptr->State = 0;

}

CreatePOP3Message(char * From, char * To, char * MsgTitle, time_t Date, char * MsgBody, int MsgLen)
{
	struct MsgInfo * Msg;
	BIDRec * BIDRec;

	// Allocate a message Record slot

	Msg = AllocateMsgRecord();
		
	// Set number here so they remain in sequence
		
	Msg->number = ++LatestMsg;
	MsgnotoMsg[Msg->number] = Msg;
	Msg->length = MsgLen;


	sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

	Msg->type = 'P';
	Msg->status = 'N';
	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	if (Date)
		Msg->datecreated = Date;

	BIDRec = AllocateBIDRecord();

	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);


	TidyString(To);
	strlop(To, '@');

	if (GMailMode)
	{
		// + separates our address and the target user

		char * GMailto;;
		GMailto = strlop(To,'+');
		if (GMailto)
			strcpy(To, GMailto);
	}

	if (strlen(To) > 6) To[6]=0;

	strcpy(Msg->to, To);
	strcpy(Msg->from, From);
	strcpy(Msg->title, MsgTitle);

	// Set up forwarding bitmap

	MatchMessagetoBBSList(Msg, 0);

	return CreateSMTPMessageFile(MsgBody, Msg);
		
}

