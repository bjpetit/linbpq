// Mail and Chat Server for BPQ32 Packet Switch
//
// MBL-Style Forwarding Routines

#include "stdafx.h"

VOID ProcessMBLLine(CIRCUIT * conn, struct UserInfo * user, UCHAR* Buffer, int len)
{
	Buffer[len] = 0;

	if (Buffer[0] == 6 && Buffer[1] == 5)
	{
		// ?? Sally send there after a failed tranfer

		memmove(Buffer, &Buffer[2], len);
		len-=2;
	}


	if (_memicmp(Buffer, "F< ", 3) == 0)
	{
		// FBB COonpressed request from system using UI Messages

		int Number = atoi(&Buffer[3]);
		struct MsgInfo * Msg = FindMessageByNumber(Number);
		char ErrMsg[80];
		int ErrLen;

		if (Msg == 0)
		{
			ErrLen = wsprintf(&ErrMsg[2], "Msg $%d does not exist!\r>", Number);
			ErrMsg[0] = 0x18;
			ErrMsg[1] = ErrLen;

			BBSputs(conn, ErrMsg);
			BBSputs(conn, ">\r");
			return;
		}
		
		Msg = FindMessage(user->Call, Number, conn->sysop);

		if (Msg)
		{
			SendCompressed(conn, Msg);
			BBSputs(conn, ">\r");
		}
		else
		{
			ErrLen = wsprintf(&ErrMsg[2], "Msg $%d not available to you!\r>", Number);
			ErrMsg[0] = 0x18;
			ErrMsg[1] = ErrLen;

			BBSputs(conn, ErrMsg);
			BBSputs(conn, ">\r");
		}
		return;
	}


	if (Buffer[0] == 'S')				//Send
	{
		// SB WANT @ ALLCAN < N6ZFJ $4567_N0ARY

		char * Cmd;
		char * To = NULL;
		char * From = NULL;
		char * BID = NULL;
		char * ATBBS = NULL;
		char * ptr, * Context;
		char seps[] = " \t\r";	
	
		Cmd = strtok_s(Buffer, seps, &Context);

		if (Cmd[1] == 0) Cmd[1] = 'P';

		To = strtok_s(NULL, seps, &Context);

		ptr = strtok_s(NULL, seps, &Context);

		while (ptr)
		{
			if (strcmp(ptr, "@") == 0)
			{
				ATBBS = _strupr(strtok_s(NULL, seps, &Context));
			}
			else if(strcmp(ptr, "<") == 0)
			{
				From = strtok_s(NULL, seps, &Context);
			}
			else if (ptr[0] == '$')
				BID = &ptr[1];
			else
			{
				nodeprintf(conn, "*** Error: Invalid Format\r");
				return;
			}

			ptr = strtok_s(NULL, seps, &Context);
		}

		if (!From)
		{
			nodeprintf(conn, "*** Error: Invalid Format\r");
			return;
		}

		CreateMessage(conn, From, To, ATBBS, toupper(Cmd[1]), BID, NULL);	

		return;
	}


	if (Buffer[0] == 'N')				// Not wanted
	{
		if (conn->FwdMsg)
		{
			// Zap the entry

			clear_fwd_bit(conn->FwdMsg->fbbs, user->BBSNumber);
			conn->UserPointer->ForwardingInfo->MsgCount--;
		}

		return;
	}

	if (Buffer[0] == 'O')				// Need it (OK)
	{
		struct tm * tm;
		time_t now;
		char * MsgBytes;

		if (!conn->FwdMsg)
			return;

		nodeprintf(conn, "%s\r\n", conn->FwdMsg->title);

		MsgBytes = ReadMessageFile(conn->FwdMsg->number);
		
		if (MsgBytes == 0)
		{
			MsgBytes = _strdup("Message file not found\r\n");
			conn->FwdMsg->length = strlen(MsgBytes);
		}

		now = time(NULL);

		tm = gmtime(&now);	
	
		nodeprintf(conn, "R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r\n",
			tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
			conn->FwdMsg->number, BBSName, HRoute, RlineVer);

		if (memcmp(MsgBytes, "R:", 2) != 0)    // No R line, so must be our message
			BBSputs(conn, "\r\n");

		QueueMsg(conn, MsgBytes, conn->FwdMsg->length);
		free(MsgBytes);
			
		nodeprintf(conn, "%c\r", 26);

		conn->FBBMsgsSent = TRUE;

		return;
	}

	if (Buffer[len-2] == '>')
	{
		// If we have just sent a nessage, Flag it as sent

		if (conn->FBBMsgsSent)
		{
			conn->FBBMsgsSent = FALSE;

			clear_fwd_bit(conn->FwdMsg->fbbs, user->BBSNumber);
			set_fwd_bit(conn->FwdMsg->forw, user->BBSNumber);

			//  Only mark as forwarded if sent to all BBSs that should have it
			
			if (memcmp(conn->FwdMsg->fbbs, zeros, NBMASK) == 0)
			{
				conn->FwdMsg->status = 'F';			// Mark as forwarded
				conn->FwdMsg->datechanged=time(NULL);
			}

			conn->UserPointer->ForwardingInfo->MsgCount--;
		}

		// Send Message or request reverse using MBL-style forwarding

		if (FindMessagestoForward(conn))
		{
			struct MsgInfo * Msg;
				
			// Send S line and wait for response - SB WANT @ USA < W8AAA $1029_N0XYZ 

			Msg = conn->FwdMsg;
		
			nodeprintf(conn, "S%c %s @ %s < %s $%s\r", Msg->type, Msg->to,
					(Msg->via[0]) ? Msg->via : conn->UserPointer->Call, 
					Msg->from, Msg->bid);

		}
		else
		{
			BBSputs(conn, "F>\r");
		}
	}

	Buffer[len] = 0;

	if (_stricmp(Buffer, "F>\r") == 0)
	{
		// Reverse forward request

		// If we have just sent a nessage, Flag it as sent

		if (conn->FBBMsgsSent)
		{
			conn->FBBMsgsSent = FALSE;
			clear_fwd_bit(conn->FwdMsg->fbbs, user->BBSNumber);
			set_fwd_bit(conn->FwdMsg->forw, user->BBSNumber);

			//  Only mark as forwarded if sent to all BBSs that should have it
			
			if (memcmp(conn->FwdMsg->fbbs, zeros, NBMASK) == 0)
			{
				conn->FwdMsg->status = 'F';			// Mark as forwarded
				conn->FwdMsg->datechanged=time(NULL);
			}

			conn->UserPointer->ForwardingInfo->MsgCount--;
		}



		// Send Message or Disconnect

		if (FindMessagestoForward(conn))
		{
			struct MsgInfo * Msg;
				
			// Send S line and wait for response - SB WANT @ USA < W8AAA $1029_N0XYZ 

			Msg = conn->FwdMsg;
		
			nodeprintf(conn, "S%c %s @ %s < %s $%s\r", Msg->type, Msg->to,
					(Msg->via[0]) ? Msg->via : conn->UserPointer->Call, 
					Msg->from, Msg->bid);

			conn->BBSFlags |= MBLFORWARDING;
			return;
		}

		nputs(conn, "*** DONE\r");
		Flush(conn);
		Sleep(400);
		Disconnect(conn->BPQStream);
		return;
	}
	
	nputs(conn, ">\r");

}

