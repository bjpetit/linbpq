// Mail and Chat Server for BPQ32 Packet Switch
//
// White Pages Database Support Routines

#include "stdafx.h"

struct NNTPRec ** NNTPRecPtr=NULL;
int NumberofNNTPRecs=0;

SOCKET nntpsock;

static SocketConn * Sockets=NULL;		// Chain of active sockets

int NNTPInPort = 119;

char *day[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};


VOID ReleaseNNTPSock(SOCKET sock);

struct NNTPRec * AllocateNNTPRecord()
{
	struct NNTPRec * REC = zalloc(sizeof (struct NNTPRec));
	
	GetSemaphore(&AllocSemaphore);

	NNTPRecPtr=realloc(NNTPRecPtr,(++NumberofNNTPRecs+1)*4);
	NNTPRecPtr[NumberofNNTPRecs] = REC;

	FreeSemaphore(&AllocSemaphore);

	return REC;
}

struct NNTPRec * LookupNNTP(char * Group)
{
	struct NNTPRec * ptr = NULL;
	int i;

	for (i=1; i <= NumberofNNTPRecs; i++)
	{
		ptr = NNTPRecPtr[i];

		if (_stricmp(ptr->NewsGroup, Group) == 0)
			return ptr;
	}

	return NULL;
}



VOID BuildNNTPList(struct MsgInfo * Msg)
{
	struct NNTPRec * REC;
	char FullGroup[100];
					
	if (Msg->type != 'B')
		return;

	wsprintf(FullGroup, "%s.%s", Msg->to, Msg->via);

	REC = LookupNNTP(FullGroup);

	if (REC == NULL)
	{
		// New Group

		REC = AllocateNNTPRecord();
		strcpy(REC->NewsGroup, FullGroup);
		REC->FirstMsg = Msg->number;
		REC->DateCreated = Msg->datecreated;
	}

	REC->LastMsg = Msg->number;
	REC->Count++;
}

char * GetPathFromHeaders(char * MsgBytes)
{
	char * Path = zalloc(10000);
	char * ptr1;

	ptr1 = MsgBytes;

nextline:

	if (memcmp(ptr1, "R:", 2) == 0)
	{		
		char * ptr4 = strchr(ptr1, '\r');
		char * ptr5 = strchr(ptr1, '.');
		ptr1 = strchr(ptr1, '@'); 

		if (!ptr1)
			return Path;

		if (*++ptr1 == ':')
			ptr1++;			// Format 2

		*(ptr5) = 0;
		
		strcat(Path, "|");
		strcat(Path, ptr1);

		*(ptr5) = '.';

		ptr1 = ptr4;

		ptr1++;
		if (*ptr1 == '\n') ptr1++;

		goto nextline;
	}

	return Path;
}

char * FormatNNTPDateAndTime(time_t Datim)
{
	struct tm *tm;
	static char Date[30];

	// Fri, 19 Nov 82 16:14:55 GMT
	// A#asctime gives Wed Jan 02 02:03:55 1980\n\0.

	tm = gmtime(&Datim);


	
	if (tm)
		sprintf_s(Date, sizeof(Date), "%s, %02d %3s %02d %02d:%02d:%02d Z",
			day[tm->tm_wday], tm->tm_mday, month[tm->tm_mon], tm->tm_year - 100, tm->tm_hour, tm->tm_min, tm->tm_sec);

	return Date;
}

VOID InitialiseNNTP()

{
	if (NNTPInPort)
		nntpsock = CreateListeningSocket(NNTPInPort, NNTP_ACCEPT);
}

CreateNNTPMessage(char * From, char * To, char * MsgTitle, time_t Date, char * MsgBody, int MsgLen)
{
	struct MsgInfo * Msg;
	BIDRec * BIDRec;
	char * Via;

	// Allocate a message Record slot

	Msg = AllocateMsgRecord();
		
	// Set number here so they remain in sequence
		
	Msg->number = ++LatestMsg;
	MsgnotoMsg[Msg->number] = Msg;
	Msg->length = MsgLen;

	sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

	Msg->type = 'B';
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
	Via = strlop(To, '@');

	if (strlen(To) > 6) To[6]=0;

	strcpy(Msg->to, To);
	strcpy(Msg->from, From);
	strcpy(Msg->title, MsgTitle);
	strcpy(Msg->via, Via);

	// Set up forwarding bitmap

	MatchMessagetoBBSList(Msg, 0);

	BuildNNTPList(Msg);				// Build NNTP Groups list

	return CreateSMTPMessageFile(MsgBody, Msg);
		
}



VOID ProcessNNTPServerMessage(SocketConn * sockptr, char * Buffer, int Len)
{
	SOCKET sock;
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
			if (_memicmp(ptr1, "Newsgroups:", 3) == 0)
			{
				char * sep = strchr(ptr1, '.');
				
				if (sep)
					*(sep) = '@';

				if (linelen > 63) linelen = 63;
				memcpy(MsgTo, &ptr1[11], linelen-11);
				MsgTo[linelen-11]=0;
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

			MsgLen = sockptr->MailSize - (ptr2 - ptr1);

			ptr1 = strchr(MsgFrom, '<');
			
			if (ptr1)
			{
				char * ptr3 = strchr(ptr1, '@');
				ptr1++;
				if (ptr3)
					*(ptr3) = 0;
			}
			else
				ptr1 = MsgFrom;

			CreateNNTPMessage(_strupr(ptr1), _strupr(MsgTo), Msgtitle, Date, ptr2, MsgLen);

			free(sockptr->MailBuffer);
			sockptr->MailBufferSize=0;
			sockptr->MailBuffer=0;
			sockptr->MailSize = 0;

			sockptr->Flags &= ~GETTINGMESSAGE;

			SendSock(sockptr, "240 OK");

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

	Buffer[Len-2] = 0;

	if(memcmp(Buffer, "GROUP", 5) == 0)
	{
		int i;
		struct NNTPRec * REC = NULL;

		for (i=1; i <= NumberofNNTPRecs; i++)
		{
			REC = NNTPRecPtr[i];

			if (_stricmp(REC->NewsGroup, &Buffer[6]) == 0)
			{
				sockprintf(sockptr, "211 %d %d %d %s", REC->Count, REC->FirstMsg, REC->LastMsg, REC->NewsGroup);
				sockptr->NNTPNum = 0;
				sockptr->NNTPGroup = REC;
				return;
			}
		}
	
		sockprintf(sockptr, "411 no such news group");
		return;
	}

	if(_memicmp(Buffer, "LISTGROUP", 9) == 0)
	{
		int i;
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgNo ;

		if (REC == NULL)
		{
			if (Buffer[10] == 0)
			{
				sockprintf(sockptr, "412 No Group Selected");
				return;
			}

			for (i=1; i <= NumberofNNTPRecs; i++)
			{
				REC = NNTPRecPtr[i];

				if (_stricmp(REC->NewsGroup, &Buffer[10]) == 0)
				{
					sockprintf(sockptr, "211 Article Numbers Follows");
					sockptr->NNTPNum = 0;
					sockptr->NNTPGroup = REC;

					for (MsgNo = REC->FirstMsg; MsgNo <= REC->LastMsg; MsgNo++)
					{
						Msg=MsgnotoMsg[MsgNo];

						if (Msg)
						{
							char FullGroup[100];
							wsprintf(FullGroup, "%s.%s", Msg->to, Msg->via );
							if (_stricmp(FullGroup, REC->NewsGroup) == 0)
							{
								sockprintf(sockptr, "%d", MsgNo);
							}
						}
					}

					SendSock(sockptr,".");
					return;
				}
			}
		}
		sockprintf(sockptr, "411 no such news group");
		return;
	}

	if(memcmp(Buffer, "MODE READER", 11) == 0)
	{
		SendSock(sockptr, "200 Hello");
		return;
	}

	if(memcmp(Buffer, "LIST",4) == 0)
	{
		int i;
		struct NNTPRec * REC = NULL;
		
		SendSock(sockptr, "215 list of newsgroups follows");

		for (i=1; i <= NumberofNNTPRecs; i++)
		{
			REC = NNTPRecPtr[i];

			sockprintf(sockptr, "%s %d %d y", REC->NewsGroup, REC->LastMsg, REC->FirstMsg);
		}

		SendSock(sockptr,".");
		return;
	}

	if(memcmp(Buffer, "HEAD",4) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgNo = atoi(&Buffer[5]);

		if (REC == NULL)
		{
			SendSock(sockptr,"412 no newsgroup has been selected");
			return;
		}

		if (MsgNo == 0)
		{
			MsgNo = sockptr->NNTPNum;

			if (MsgNo == 0)
			{
				SendSock(sockptr,"420 no current article has been selected");
				return;
			}
		}
		else
		{
			 sockptr->NNTPNum = MsgNo;
		}

		Msg=MsgnotoMsg[MsgNo];

		if (Msg)
		{
			sockprintf(sockptr, "221 %d <%s>", MsgNo, Msg->bid);

			sockprintf(sockptr, "From: %s", Msg->from);
			sockprintf(sockptr, "Date: %s", FormatNNTPDateAndTime(Msg->datecreated));
			sockprintf(sockptr, "Newsgroups: %s.s", Msg->to, Msg->via);
			sockprintf(sockptr, "Subject: %s", Msg->title);
			sockprintf(sockptr, "Message-ID: <%s>", Msg->bid);
			sockprintf(sockptr, "Path: %s", BBSName);

			SendSock(sockptr,".");
			return;
		}

		SendSock(sockptr,"423 No such article in this newsgroup");
		return;
	}

	if(memcmp(Buffer, "ARTICLE", 7) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgNo = atoi(&Buffer[8]);
		char * msgbytes;
		char * Path;

		if (REC == NULL)
		{
			SendSock(sockptr,"412 no newsgroup has been selected");
			return;
		}

		if (MsgNo == 0)
		{
			MsgNo = sockptr->NNTPNum;

			if (MsgNo == 0)
			{
				SendSock(sockptr,"420 no current article has been selected");
				return;
			}
		}
		else
		{
			 sockptr->NNTPNum = MsgNo;
		}

		Msg=MsgnotoMsg[MsgNo];

		if (Msg)
		{
			sockprintf(sockptr, "220 %d <%s>", MsgNo, Msg->bid);
			msgbytes = ReadMessageFile(Msg->number);

			Path = GetPathFromHeaders(msgbytes);

			sockprintf(sockptr, "From: %s", Msg->from);
			sockprintf(sockptr, "Date: %s", FormatNNTPDateAndTime(Msg->datecreated));
			sockprintf(sockptr, "Newsgroups: %s.%s", Msg->to, Msg->via);
			sockprintf(sockptr, "Subject: %s", Msg->title);
			sockprintf(sockptr, "Message-ID: <%s>", Msg->bid);
			sockprintf(sockptr, "Path: %s", &Path[1]);

			SendSock(sockptr,"");


			SendSock(sockptr,msgbytes);
			SendSock(sockptr,"");

			SendSock(sockptr,".");

			free(msgbytes);
			free(Path);

			return;
			
		}
		SendSock(sockptr,"423 No such article in this newsgroup");
		return;
	}

	if(memcmp(Buffer, "XHDR",4) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgStart, MsgEnd, MsgNo, fields;
		char Header[100];

		if (REC == NULL)
		{
			SendSock(sockptr,"412 no newsgroup has been selected");
			return;
		}

		// XHDR subject nnnn-nnnn

		fields = sscanf(&Buffer[5], "%s %d-%d", &Header, &MsgStart, &MsgEnd);

		if (fields > 1)
			MsgNo = MsgStart;

		if (fields == 2)
			MsgEnd = MsgStart;

		if (MsgNo == 0)
		{
			MsgStart = MsgEnd = sockptr->NNTPNum;

			if (MsgStart == 0)
			{
				SendSock(sockptr,"420 no current article has been selected");
				return;
			}
		}
		else
		{
			 sockptr->NNTPNum = MsgEnd;
		}

		sockprintf(sockptr, "221 ");

		for (MsgNo = MsgStart; MsgNo <= MsgEnd; MsgNo++)
		{
			Msg=MsgnotoMsg[MsgNo];

			if (Msg)
			{
				char FullGroup[100];
				wsprintf(FullGroup, "%s.%s", Msg->to, Msg->via );
				if (_stricmp(FullGroup, REC->NewsGroup) == 0)
				{
					if (_stricmp(Header, "subject") == 0)
						sockprintf(sockptr, "%d Subject: %s", MsgNo, Msg->title);
					else if (_stricmp(Header, "from") == 0)
						sockprintf(sockptr, "%d From: %s", MsgNo, Msg->from);
					else if (_stricmp(Header, "date") == 0)
						sockprintf(sockptr, "%d Date: %s", MsgNo, FormatNNTPDateAndTime(Msg->datecreated));
					else if (_stricmp(Header, "message-id") == 0)
						sockprintf(sockptr, "%d Message-ID: <%s>",  MsgNo, Msg->bid);
					else if (_stricmp(Header, "lines") == 0)
						sockprintf(sockptr, "%d Lines: %d",  MsgNo, Msg->length);
				}
			}
		}

		SendSock(sockptr,".");
		return;

	}

	if(memcmp(Buffer, "XOVER", 5) == 0)
	{
		struct NNTPRec * REC = sockptr->NNTPGroup;
		struct MsgInfo * Msg;
		int MsgStart, MsgEnd, MsgNo, fields;

		if (REC == NULL)
		{
			SendSock(sockptr,"412 no newsgroup has been selected");
			return;
		}

		fields = sscanf(&Buffer[6], "%d-%d", &MsgStart, &MsgEnd);

		if (fields > 0)
			MsgNo = MsgStart;

		if (fields == 1)
			MsgEnd = MsgStart;

		if (MsgNo == 0)
		{
			MsgStart = MsgEnd = sockptr->NNTPNum;

			if (MsgStart == 0)
			{
				SendSock(sockptr,"420 no current article has been selected");
				return;
			}
		}
		else
		{
			 sockptr->NNTPNum = MsgEnd;
		}

		sockprintf(sockptr, "224 ");

		for (MsgNo = MsgStart; MsgNo <= MsgEnd; MsgNo++)
		{
			Msg=MsgnotoMsg[MsgNo];

			if (Msg)
			{
				char FullGroup[100];
				wsprintf(FullGroup, "%s.%s", Msg->to, Msg->via );
				if (_stricmp(FullGroup, REC->NewsGroup) == 0)
				{
					 // subject, author, date, message-id, references, byte count, and line count. 
					sockprintf(sockptr, "%d\t%s\t%s\t%s\t%s\t%s\t%d\t%d",
						MsgNo, Msg->title, Msg->from, FormatNNTPDateAndTime(Msg->datecreated), Msg->bid,
						"", Msg->length, Msg->length);
				}
			}
		}

		SendSock(sockptr,".");
		return;

	}


 /*
 240 article posted ok
   340 send article to be posted. End with <CR-LF>.<CR-LF>
   440 posting not allowed
   441 posting failed
*/
	if(memcmp(Buffer, "POST", 4) == 0)
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
		
		SendSock(sockptr, "340 OK");
		return;
	}



	if(memcmp(Buffer, "QUIT", 4) == 0)
	{
		SendSock(sockptr, "205 OK");
		Sleep(500);
		shutdown(sock, 0);
		return;
	}

/*	if(memcmp(Buffer, "RSET\r\n", 6) == 0)
	{
		SendSock(sockptr, "250 Ok");
		sockptr->State = 0;
		sockptr->Recipients;
		return;
	}
*/
	 
	SendSock(sockptr, "500 command not recognized");

	return;
}




int NNTP_Read(SocketConn * sockptr, SOCKET sock)
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

	if (InputLen == -1)
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
	
			ProcessNNTPServerMessage(sockptr, sockptr->TCPBuffer, sockptr->InputLen);
			sockptr->InputLen=0;	
		}
		else
		{
			// buffer contains more that 1 message

			MsgLen = sockptr->InputLen - (ptr2-ptr);

			memcpy(Buffer, sockptr->TCPBuffer, MsgLen);

			ProcessNNTPServerMessage(sockptr, Buffer, MsgLen);

			memmove(sockptr->TCPBuffer, ptr, sockptr->InputLen-MsgLen);

			sockptr->InputLen -= MsgLen;

			goto loop;

		}
	}
	return 0;
}


int NNTP_Accept(int SocketId)
{
	int addrlen;
	SocketConn * sockptr;
	SOCKET sock;

//   Allocate a Socket entry

	sockptr=zalloc(sizeof(SocketConn)+100);

	sockptr->Next=Sockets;
	Sockets=sockptr;

	addrlen=sizeof(struct sockaddr);

	sock = accept(SocketId, (struct sockaddr *)&sockptr->sin, &addrlen);

	if (sock == INVALID_SOCKET)
	{
		Logprintf(LOG_TCP, NULL, '|', "NNTP accept() failed Error %d", WSAGetLastError());
		return FALSE;
	}

	sockptr->Type = NNTPServer;

	WSAAsyncSelect(sock, MainWnd, NNTP_DATA,
			FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);

	sockptr->socket = sock;

	return 0;
}

int NNTP_Data(int sock, int error, int eventcode)
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

					return NNTP_Read(sockptr,sock);

				case FD_WRITE:

					// Either Just connected, or flow contorl cleared

					if (sockptr->SendBuffer)
						// Data Queued

						SendFromQueue(sockptr);
					else
						SendSock(sockptr, "200 BPQMail NNTP Server ready");		
					
					return 0;

				case FD_OOB:

					return 0;

				case FD_ACCEPT:

					return 0;

				case FD_CONNECT:

					return 0;

				case FD_CLOSE:

					closesocket(sock);
					ReleaseNNTPSock(sock);
					return 0;
				}
			return 0;
		}
		else
			sockptr=sockptr->Next;
	}

	return 0;
}

VOID ReleaseNNTPSock(SOCKET sock)
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

			free(sockptr);
			return;
		}
		else
		{
			lastptr=sockptr;
			sockptr=sockptr->Next;
		}
	}

	return;
}

VOID SendFromQueue(SocketConn * sockptr)
{
	int bytestosend = sockptr->SendSize - sockptr->SendPtr;
	int bytessent;

	Debugprintf("TCP - Sending %d bytes from buffer", bytestosend); 

	bytessent = send(sockptr->socket, &sockptr->SendBuffer[sockptr->SendPtr], bytestosend, 0);

	if (bytessent == bytestosend)
	{
		//	All Sent

		free(sockptr->SendBuffer);
		sockptr->SendBuffer = NULL;
	}
	else
	{
		sockptr->SendPtr += bytessent;
	}

	return;
}
