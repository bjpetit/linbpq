// Mail and Chat Server for BPQ32 Packet Switch
//
// FBB Forwarding Routines

#include "stdafx.h"

int MaxRXSize = 99999;
int MaxTXSize = 99999;

struct FBBRestartData ** RestartData = NULL;
int RestartCount = 0;

VOID ProcessFBBLine(CIRCUIT * conn, struct UserInfo * user, UCHAR* Buffer, int len)
{
	struct FBBHeaderLine * FBBHeader;	// The Headers from an FBB forward block
	int i;
	char * ptr;
	char * Context;
	char seps[] = " \r";
	int RestartPtr;
	char * Respptr;

	if (conn->Flags & GETTINGMESSAGE)
	{
		ProcessMsgLine(conn, user, Buffer, len);
		if (conn->Flags & GETTINGMESSAGE)

			// Still going
			return;

		SetupNextFBBMessage(conn);
		return;
	}

	if (conn->Flags & GETTINGTITLE)
	{
		ProcessMsgTitle(conn, user, Buffer, len);
		return;
	}

	// Should be FA FB F> FS FF FQ
	if (Buffer[0] == ';')
		return;							// winlink comment

	if (Buffer[0] != 'F')
	{
		BBSputs(conn, "*** Protocol Error - Line should start with 'F'\r");
		Flush(conn);
		Sleep(500);
		Disconnect(conn->BPQStream);

		return;
	}

	switch (Buffer[1])
	{
	case 'F':

		// Request Reverse

		if (conn->FBBMsgsSent)
			FlagSentMessages(conn, user);
		
		if (!FBBDoForward(conn))				// Send proposal if anthing to forward
		{
			BBSputs(conn, "FQ\r");

			// LinFBB needs a Disconnect Here

			conn->CloseAfterFlush = 20;			// 2 Secs
		}
		return;

	case 'S':

		//	Proposal response

		Respptr=&Buffer[2];

		for (i=0; i < conn->FBBIndex; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];

			Respptr++;
				
			if ((*Respptr == '-') || (*Respptr == 'N') || (*Respptr == 'R'))				// Not wanted
			{
				user->MsgsRejectedOut++;
				
				// Zap the entry

				if (conn->Paclink)					// Not using Bit Masks
				{
					FBBHeader->FwdMsg->status = 'F';			// Mark as forwarded
					FBBHeader->FwdMsg->datechanged=time(NULL);
				}
				else
				{
					clear_fwd_bit(FBBHeader->FwdMsg->fbbs, user->BBSNumber);

					if (memcmp(FBBHeader->FwdMsg->fbbs, zeros, NBMASK) == 0)
					{
						FBBHeader->FwdMsg->status = 'F';			// Mark as forwarded
						FBBHeader->FwdMsg->datechanged=time(NULL);
					}
				}

				memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));

				conn->UserPointer->ForwardingInfo->MsgCount--;
				continue;
			}

			if ((*Respptr == '=') || (*Respptr == 'L'))				// Defer
			{
				// Remove entry from forwarding block

				memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));
				continue;
			}

			conn->RestartFrom = 0;		// Assume Restart from
	
			if ((*Respptr == '!') || (*Respptr == 'A'))
			{
				// Restart 

				char Num[10];
				char *numptr=&Num[0];

				Respptr++;

				while (isdigit(*Respptr))
				{
					*(numptr++) = *(Respptr++);
				}
				*numptr = 0;

				conn->RestartFrom = atoi(Num);

				*(--Respptr) = '+';  // So can drop through
			}

			if ((*Respptr == '+') || (*Respptr == 'Y') || (*Respptr == 'H'))				// Need it
			{
				struct tm * tm;
				time_t now;
				char * MsgBytes;

				conn->FBBMsgsSent = TRUE;		// Messages to flag as complete when next command received

				if (conn->BBSFlags & FBBCompressed)
				{
					if (conn->BBSFlags & FBBB2Mode)
						SendCompressedB2(conn, FBBHeader);
					else
						SendCompressed(conn, FBBHeader->FwdMsg);
				}
				else
				{
					nodeprintf(conn, "%s\r\n", FBBHeader->FwdMsg->title);

					MsgBytes = ReadMessageFile(FBBHeader->FwdMsg->number);

					if (MsgBytes == 0)
					{
						MsgBytes = _strdup("Message file not found\r\n");
						FBBHeader->FwdMsg->length = strlen(MsgBytes);
					}

					now = time(NULL);

					tm = gmtime(&now);	
	
					nodeprintf(conn, "R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r\n",
						tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
						FBBHeader->FwdMsg->number, BBSName, HRoute, RlineVer);

					if (memcmp(MsgBytes, "R:", 2) != 0)    // No R line, so must be our message - put blank line after header
						BBSputs(conn, "\r\n");

					QueueMsg(conn, MsgBytes, FBBHeader->FwdMsg->length);
					free(MsgBytes);

					user->MsgsSent++;
					user->BytesForwardedOut += FBBHeader->FwdMsg->length;
			
					nodeprintf(conn, "%c\r\n", 26);
				}
				continue;
			}
			BBSputs(conn, "*** Protocol Error - Invalid Proposal Response'\r");
		}

		conn->FBBIndex = 0;		// ready for next block;
		conn->FBBChecksum = 0;

		return;

	case 'Q':

		Disconnect(conn->BPQStream);
		return;

	case 'A':			// Proposal
	case 'B':			// Proposal

		if (conn->FBBMsgsSent)
			FlagSentMessages(conn, user);			// Mark previously sent messages


		// Accumulate checksum

		for (i=0; i< len; i++)
		{
			conn->FBBChecksum+=Buffer[i];
		}

		// Parse Header

		// Find free line

		for (i = 0; i < 5; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];

			if (FBBHeader->Format == 0)
				break;
		}

		if (i == 5)
		{
			BBSputs(conn, "*** Protocol Error - Too Many Proposals\r");
			Flush(conn);
			conn->CloseAfterFlush = 20;			// 2 Secs
		}

		//FA P GM8BPQ G8BPQ G8BPQ 2209_GM8BPQ 8

		FBBHeader->Format = Buffer[1];

		ptr = strtok_s(&Buffer[3], seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) != 1) goto badparam;

		FBBHeader->MsgType = *ptr;

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 6 ) goto badparam;

		strcpy(FBBHeader->From, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 40 ) goto badparam;

		strcpy(FBBHeader->ATBBS, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 7 ) goto badparam;

		strcpy(FBBHeader->To, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		if (strlen(ptr) > 12 ) goto badparam;

		strcpy(FBBHeader->BID, ptr);

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam;

		FBBHeader->Size = atoi(ptr);

		goto ok;

badparam:

		BBSputs(conn, "*** Protocol Error - Proposal format error\r");
		Flush(conn);
		conn->CloseAfterFlush = 20;			// 2 Secs
		return;

ok:
		// If P Message, dont immediately reject on a Duplicate BID. Check if we still have the message
		//	If we do, reject  it. If not, accept it again. (do we need some loop protection ???)

		if (DoWeWantIt(FBBHeader) == FALSE)
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '-';
			user->MsgsRejectedIn++;
		}
		else if ((RestartPtr = LookupRestart(conn, FBBHeader)) > 0)
		{
			conn->FBBReplyIndex += wsprintf(&conn->FBBReplyChars[conn->FBBReplyIndex], "!%d", RestartPtr);
		}
		else if (LookupTempBID(FBBHeader->BID))
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '=';
		}
		else
		{

			//	Save BID in temp list in case we are offered it again before completion
			
			BIDRec * TempBID = AllocateTempBIDRecord();
			strcpy(TempBID->BID, FBBHeader->BID);
			TempBID->u.conn = conn;

			conn->FBBReplyChars[conn->FBBReplyIndex++] = '+';
		}

		FBBHeader->B2Message = FALSE;

		return;

	case 'C':			// B2 Proposal

		if (conn->FBBMsgsSent)
			FlagSentMessages(conn, user);			// Mark previously sent messages

		// Accumulate checksum

		for (i=0; i< len; i++)
		{
			conn->FBBChecksum+=Buffer[i];
		}

		// Parse Header

		// Find free line

		for (i = 0; i < 5; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];

			if (FBBHeader->Format == 0)
				break;
		}

		if (i == 5)
		{
			BBSputs(conn, "*** Protocol Error - Too Many Proposals\r");
			Flush(conn);
			conn->CloseAfterFlush = 20;			// 2 Secs
		}


		// FC EM A3EDD4P00P55 377 281 0

		
		/*
		
			FC Proposal code. Requires B2 SID feature.
			Type Message type ( 1 or 2 alphanumeric characters

			CM WinLink 2000 Control message
			EM Encapsulated Message
			ID Unique Message Identifier (max length 12 characters)
			U-Size Uncompressed size of message
			C-size Compressed size of message

		*/
		FBBHeader->Format = Buffer[1];

		ptr = strtok_s(&Buffer[3], seps, &Context);
		if (ptr == NULL) goto badparam2;
		if (strlen(ptr) != 2) goto badparam2;
		FBBHeader->MsgType = 'P'; //ptr[0];

		ptr = strtok_s(NULL, seps, &Context);

		if (ptr == NULL) goto badparam2;
		if (strlen(ptr) > 12 ) goto badparam;
		strcpy(FBBHeader->BID, ptr);

		ptr = strtok_s(NULL, seps, &Context);
		if (ptr == NULL) goto badparam2;
		FBBHeader->Size = atoi(ptr);

		ptr = strtok_s(NULL, seps, &Context);
		if (ptr == NULL) goto badparam2;
		FBBHeader->CSize = atoi(ptr);
		FBBHeader->B2Message = TRUE;

		goto ok2;

badparam2:

		BBSputs(conn, "*** Protocol Error - Proposal format error\r");
		Flush(conn);
		conn->CloseAfterFlush = 20;			// 2 Secs
		return;

ok2:
		if (LookupBID(FBBHeader->BID)  || (FBBHeader->Size > MaxRXSize))
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '-';
		}
		else if ((RestartPtr = LookupRestart(conn, FBBHeader)) > 0)
		{
			conn->FBBReplyIndex += wsprintf(&conn->FBBReplyChars[conn->FBBReplyIndex], "!%d", RestartPtr);
		}

		else if (LookupTempBID(FBBHeader->BID))
		{
			memset(FBBHeader, 0, sizeof(struct FBBHeaderLine));		// Clear header
			conn->FBBReplyChars[conn->FBBReplyIndex++] = '=';
		}
		else
		{
			//	Save BID in temp list in case we are offered it again before completion
			
			BIDRec * TempBID = AllocateTempBIDRecord();
			strcpy(TempBID->BID, FBBHeader->BID);
			TempBID->u.conn = conn;

			conn->FBBReplyChars[conn->FBBReplyIndex++] = 'Y';
		}

		return;

	case '>':

		// Optional Checksum

		if (len > 3)
		{
			int sum;
			
			sscanf(&Buffer[3], "%x", &sum);

			conn->FBBChecksum+=sum;

			if (conn->FBBChecksum)
			{
				BBSputs(conn, "*** Proposal Checksum Error\r");
				Flush(conn);
				conn->CloseAfterFlush = 20;			// 2 Secs
				return;
			}
		}

		// Return "FS ", followed by +-= for each proposal

		conn->FBBReplyChars[conn->FBBReplyIndex] = 0;
		conn->FBBReplyIndex = 0;

		nodeprintf(conn, "FS %s\r", conn->FBBReplyChars);

		// if all rejected, send proposals or prompt, else set up for first message

		FBBHeader = &conn->FBBHeaders[0];

		if (FBBHeader->MsgType == 0)
		{
			conn->FBBIndex = 0;						// ready for first block;
			memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));
			conn->FBBChecksum = 0;

			if (!FBBDoForward(conn))				// Send proposal if anthing to forward
			{
				conn->InputMode = 0;
				BBSputs(conn, "FF\r");
			}
		}
		else
		{
			if (conn->BBSFlags & FBBCompressed)
				conn->InputMode = 'B';
			CreateMessage(conn, FBBHeader->From, FBBHeader->To, FBBHeader->ATBBS, FBBHeader->MsgType, FBBHeader->BID, NULL);
		}

		return;

	}

	return;
}
VOID FlagSentMessages(CIRCUIT * conn, struct UserInfo * user)
{
	struct FBBHeaderLine * FBBHeader;	// The Headers from an FBB forward block
	int i;

	// Called if FBB command received after sending a block of messages . Flag as as sent.

	conn->FBBMsgsSent = FALSE;

	for (i=0; i < 5; i++)
	{
		FBBHeader = &conn->FBBHeaders[i];
				
		if (FBBHeader && FBBHeader->MsgType)				// Not a zapped entry
		{
			if (conn->Paclink || conn->RMSExpress)
			{
				// Kill Messages sent to paclink

				FlagAsKilled(FBBHeader->FwdMsg);
			}
			else
			{
				clear_fwd_bit(FBBHeader->FwdMsg->fbbs, user->BBSNumber);
				set_fwd_bit(FBBHeader->FwdMsg->forw, user->BBSNumber);

				//  Only mark as forwarded if sent to all BBSs that should have it
			
				if (memcmp(FBBHeader->FwdMsg->fbbs, zeros, NBMASK) == 0)
				{
					FBBHeader->FwdMsg->status = 'F';			// Mark as forwarded
					FBBHeader->FwdMsg->datechanged=time(NULL);
				}

				conn->UserPointer->ForwardingInfo->MsgCount--;
			}
		}
	}
	memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));

}


VOID SetupNextFBBMessage(CIRCUIT * conn)
{	
	struct FBBHeaderLine * FBBHeader;	// The Headers from an FFB forward block

	memmove(&conn->FBBHeaders[0], &conn->FBBHeaders[1], 4 * sizeof(struct FBBHeaderLine));
	
	memset(&conn->FBBHeaders[4], 0, sizeof(struct FBBHeaderLine));

	FBBHeader = &conn->FBBHeaders[0];

	if (FBBHeader->MsgType == 0)
	{
		conn->FBBIndex = 0;		// ready for next block;
		memset(&conn->FBBHeaders[0], 0, 5 * sizeof(struct FBBHeaderLine));

		conn->FBBChecksum = 0;
		conn->InputMode = 0;


		if (!FBBDoForward(conn))				// Send proposal if anthing to forward
		{
			conn->InputMode = 0;
			BBSputs(conn, "FF\r");
		}
	}
	else
	{
		if (conn->BBSFlags & FBBCompressed)
			conn->InputMode = 'B';

		CreateMessage(conn, FBBHeader->From, FBBHeader->To, FBBHeader->ATBBS, FBBHeader->MsgType, FBBHeader->BID, NULL);
	}
}

BOOL FBBDoForward(CIRCUIT * conn)
{
	int i;
	char proposal[100];
	int proplen;

	if (FindMessagestoForward(conn))
	{
		// Send Proposal Block

		struct FBBHeaderLine * FBBHeader;

		for (i=0; i < conn->FBBIndex; i++)
		{
			FBBHeader = &conn->FBBHeaders[i];

			if (conn->BBSFlags & FBBB2Mode)

				// FC EM A3EDD4P00P55 377 281 0

				proplen = wsprintf(proposal, "FC EM %s %d %d %d\r", 
					FBBHeader->BID,
					FBBHeader->Size,
					FBBHeader->CSize, 0);
			else
				proplen = wsprintf(proposal, "%s %c %s %s %s %s %d\r", 
					(conn->BBSFlags & FBBCompressed) ? "FA" : "FB",
					FBBHeader->MsgType,
					FBBHeader->From,
					(FBBHeader->ATBBS[0]) ? FBBHeader->ATBBS : conn->UserPointer->Call, 
					FBBHeader->To, 
					FBBHeader->BID,
					FBBHeader->Size);

			// Accumulate checksum

			while(proplen > 0)
			{
				conn->FBBChecksum+=proposal[--proplen];
			}

			BBSputs(conn, proposal);
		}

		conn->FBBChecksum = - conn->FBBChecksum;

		nodeprintf(conn, "F> %02X\r", conn->FBBChecksum);
		return TRUE;
	}

	return FALSE;
}

VOID UnpackFBBBinary(CIRCUIT * conn)
{
	int MsgLen, i, offset, n;
	UCHAR * ptr;

loop:

	ptr = conn->InputBuffer;

	if (conn->InputLen < 2)
		return;							//  All formats need at least two bytes

	switch (*ptr)
	{
	case 1:			// Header

		MsgLen = ptr[1] + 2;

		if (conn->InputLen < MsgLen)
			return;						// Wait for more

		strcpy(conn->TempMsg->title, &ptr[2]);

		offset = atoi(ptr+3+strlen(conn->TempMsg->title));

		ptr += MsgLen;

		memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

		conn->InputLen -= MsgLen;

		conn->FBBChecksum = 0;

		if (offset)
		{
			struct FBBRestartData * RestartRec;

			// Trying to restart - make sure we have restart data

			for (i = 1; i <= RestartCount; i++)
			{
				RestartRec = RestartData[i];
		
				if ((RestartRec->UserPointer == conn->UserPointer)
					&& (strcmp(RestartRec->TempMsg->bid, conn->TempMsg->bid) == 0))
				{
					if (RestartRec->TempMsg->length <= offset)
					{
						conn->TempMsg->length = RestartRec->TempMsg->length;
						conn->MailBuffer = RestartRec->MailBuffer;
						conn->MailBufferSize = RestartRec->MailBufferSize;

						// FBB Seems to insert  6 Byte message
						// It looks like the original csum and length - perhaps a a consistancy check

						// But Airmail Sends the Restart Data in the next packet, move the check code.

						conn->NeedRestartHeader = TRUE;

						goto GotRestart;
					}
					else
					{
						BBSputs(conn, "*** Trying to restart from invalid position.\r");
						Flush(conn);
						conn->CloseAfterFlush = 20;			// 2 Secs

						return;
					}

					// Remove Restart info

					for (n = i; n < RestartCount; n++)
					{
						RestartData[n] = RestartData[n+1];		// move down all following entries
					}
					RestartCount--;
				}
			}

			// No Restart Data

			BBSputs(conn, "*** Trying to restart, but no restart data.\r");
			Flush(conn);
			conn->CloseAfterFlush = 20;			// 2 Secs

			return;
		}
	
		// Create initial buffer of 10K. Expand if needed later

		if (conn->MailBufferSize == 0)
		{
			// Dont allocate if restarting

			conn->MailBuffer=malloc(10000);
			conn->MailBufferSize=10000;
		}

	GotRestart:

		if (conn->MailBuffer == NULL)
		{
			BBSputs(conn, "*** Failed to create Message Buffer\r");
			conn->CloseAfterFlush = 20;			// 2 Secs

			return;
		}

		goto loop;



	case 2:			// Data Block

		if (ptr[1] == 0)
			MsgLen = 256;
		else
			MsgLen = ptr[1];

		if (conn->InputLen < (MsgLen + 2))
			return;						// Wait for more

		// Waiting for Restart Header, see if it has arrived

		if (conn->NeedRestartHeader)
		{
			conn->NeedRestartHeader = FALSE;

			if (conn->InputLen > 8)
			{
				ptr = conn->InputBuffer+2;
				conn->InputLen -=8;
								
				for (i=0; i<6; i++)
				{
					conn->FBBChecksum+=ptr[0];
					ptr++;
				}

				memmove(conn->InputBuffer, ptr, conn->InputLen);

			}
			else
				BBSputs(conn, "*** Extra 8 bytes missing.\r");

			goto loop;

		}
		// Process it

		ptr+=2;

		for (i=0; i< MsgLen; i++)
		{
			conn->FBBChecksum+=ptr[i];
		}

		ptr-=2;

		if ((conn->TempMsg->length + MsgLen) > conn->MailBufferSize)
		{
			conn->MailBufferSize += 10000;
			conn->MailBuffer = realloc(conn->MailBuffer, conn->MailBufferSize);
	
			if (conn->MailBuffer == NULL)
			{
				BBSputs(conn, "*** Failed to extend Message Buffer\r");
				conn->CloseAfterFlush = 20;			// 2 Secs

				return;
			}
		}

		memcpy(&conn->MailBuffer[conn->TempMsg->length], &ptr[2], MsgLen);

		conn->TempMsg->length += MsgLen;

		MsgLen +=2;

		ptr += MsgLen;

		memmove(conn->InputBuffer, ptr, conn->InputLen-MsgLen);

		conn->InputLen -= MsgLen;

		goto loop;


	case 4:				// EOM

		// Process EOM

		conn->FBBChecksum+=ptr[1];

		if (conn->FBBChecksum == 0)
		{
			__try 
			{
				Decode(conn);
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				BBSputs(conn, "*** Program Error Decoding Message\r");
				Flush(conn);
				conn->CloseAfterFlush = 20;			// 2 Secs
				return;
			}
		}

		else
		{
			BBSputs(conn, "*** Message Checksum Error\r");
			Flush(conn);
			conn->CloseAfterFlush = 20;			// 2 Secs
			return;
		}
		ptr += 2;

		memmove(conn->InputBuffer, ptr, conn->InputLen-2);

		conn->InputLen -= 2;

		goto loop;
	
	default:

		BBSputs(conn, "*** Protocol Error - Invalid Binary Message Format (Invalid Message Type)\r");
		Flush(conn);
		conn->CloseAfterFlush = 20;			// 2 Secs

		return;
	}
}

VOID SendCompressed(CIRCUIT * conn, struct MsgInfo * FwdMsg)
{
	struct tm * tm;
	char * MsgBytes, * Save;
	UCHAR * Compressed, * Compressedptr;
	UCHAR * UnCompressed;
	char * Title;
	UCHAR * Output, * Outputptr;
	int i, OrigLen, MsgLen, CompLen, DataOffset;
	char Rline[80];
	int RLineLen;

	MsgBytes = Save = ReadMessageFile(FwdMsg->number);

	if (MsgBytes == 0)
	{
		MsgBytes = _strdup("Message file not found\r\n");
		FwdMsg->length = strlen(MsgBytes);
	}

	OrigLen = FwdMsg->length;

	Title = FwdMsg->title;

	Compressed = Compressedptr = zalloc(2 * OrigLen + 200);
	Output = Outputptr = zalloc(2 * OrigLen + 200);

	*Outputptr++ = 1;
	*Outputptr++ = strlen(Title) + 8;
	strcpy(Outputptr, Title);
	Outputptr += strlen(Title) +1;
	wsprintf(Outputptr, "%06d", conn->RestartFrom);
	Outputptr += 7;

	DataOffset = Outputptr - Output;	// Used if restarting

	if (conn->RestartFrom == 0)
	{
		// save time first sent, or checksum will be wrong when we restart
		
		FwdMsg->datechanged=time(NULL);
		conn->UserPointer->MsgsSent++;
		conn->UserPointer->BytesForwardedOut += OrigLen;
	}

	tm = gmtime(&FwdMsg->datechanged);	
	
	wsprintf(Rline, "R:%02d%02d%02d/%02d%02dZ %d@%s.%s %s\r\n",
		tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min,
		FwdMsg->number, BBSName, HRoute, RlineVer);

	if (memcmp(MsgBytes, "R:", 2) != 0)    // No R line, so must be our message
		strcat(Rline, "\r\n");

	RLineLen = strlen(Rline);

	MsgLen = OrigLen + RLineLen;
	
	UnCompressed = zalloc(MsgLen+10);
	
	strcpy(UnCompressed, Rline);

	// If a B2 Message, Remove B2 Header

	if (FwdMsg->B2Flags)
	{
		char * ptr;
		int BodyLen = OrigLen;				
		
		// Remove all B2 Headers, and all but the first part.
					
		ptr = strstr(MsgBytes, "Body:");
			
		if (ptr)
		{
			BodyLen = atoi(&ptr[5]);
			ptr= strstr(MsgBytes, "\r\n\r\n");		// Blank Line after headers
	
			if (ptr)
				ptr +=4;
			else
				ptr = MsgBytes;
			
		}
		else
			ptr = MsgBytes;

		if (memcmp(ptr, "R:", 2) == 0)    // Already have RLines, so remove blank line after new R:line
			RLineLen -= 2;

		memcpy(&UnCompressed[RLineLen], ptr, BodyLen);

		MsgLen = BodyLen + RLineLen;
	}
	else  // Not B2 Message
	{
		memcpy(&UnCompressed[RLineLen], MsgBytes, OrigLen);
	}

	CompLen = Encode(UnCompressed, Compressed, MsgLen, conn->BBSFlags & FBBB1Mode);

	conn->FBBChecksum = 0;

	// If restarting, send the checksum and length as a single record, then data from the restart point
	// The count includes the header, so adjust count and pointers

	if (conn->RestartFrom)
	{
		*Outputptr++ = 2;
		*Outputptr++ = 6;

		for (i=0; i< 6; i++)
		{
			conn->FBBChecksum+=Compressed[i];
			*Outputptr++ = Compressed[i];
		}

		for (i=conn->RestartFrom; i< CompLen; i++)
		{
			conn->FBBChecksum+=Compressed[i];
		}
		
		Compressedptr += conn->RestartFrom;
		CompLen -= conn->RestartFrom;
	}
	else
	{
		for (i=0; i< CompLen; i++)
		{
			conn->FBBChecksum+=Compressed[i];
		}
	}

	while (CompLen > 256)
	{
		*Outputptr++ = 2;
		*Outputptr++ = 0;

		memcpy(Outputptr, Compressedptr, 256);
		Outputptr += 256;
		Compressedptr += 256;
		CompLen -= 256;
	}

	*Outputptr++ = 2;
	*Outputptr++ = CompLen;

	memcpy(Outputptr, Compressedptr, CompLen);

	Outputptr += CompLen;

	*Outputptr++ = 4;
	conn->FBBChecksum = - conn->FBBChecksum;
	*Outputptr++ = conn->FBBChecksum;

	QueueMsg(conn, Output, Outputptr - Output);

	free(Save);
	free(Compressed);
	free(UnCompressed);
	free(Output);
			
}

VOID CreateB2Message(CIRCUIT * conn, struct FBBHeaderLine * FBBHeader, char * Rline)
{
	char * MsgBytes;
	UCHAR * Compressed;
	UCHAR * UnCompressed;
	int OrigLen, MsgLen, B2HddrLen, CompLen;
	char Date[20];
	struct tm * tm;
	char B2To[80];
	struct MsgInfo * Msg = FBBHeader->FwdMsg;
//	char DebugMsg[1000]="";
	char * xptr;

	MsgBytes = ReadMessageFile(Msg->number);

	if (MsgBytes == 0)
	{
		MsgBytes = _strdup("Message file not found\r\n");
		Msg->length = strlen(MsgBytes);
	}

	UnCompressed = zalloc(Msg->length+2000);
	OrigLen = Msg->length;

	// If a B2 Message  add R:line at start of Body, but otherwise leave intact.
	// Unless a message to Paclink, when we must remove any HA from the TO address

	if (Msg->B2Flags & B2Msg)
	{
		char * ptr, *ptr2;
		int BodyLen;
		int BodyLineLen, RlineLen = strlen(Rline);
		int Index;
		
		MsgLen = OrigLen + RlineLen;

		if (conn->Paclink)
		{
			// Remove any HA on the TO address
			
			ptr = strstr(MsgBytes, "To:");
			if (ptr)
			{
				ptr2 = strstr(ptr, "\r\n");
				if (ptr2)
				{
					while (ptr < ptr2)
					{
						if (*ptr == '.' || *ptr == '@')
						{
							memset(ptr, ' ', ptr2 - ptr);
							break;
						}
						ptr++;
					}
				}
			}

		}

		// Add R: Line at start of body. Will Need to Update Body Length

		ptr = strstr(MsgBytes, "Body:");
		ptr2 = strstr(ptr, "\r\n");

		Index = ptr - MsgBytes;		// Bytes Before Body: line

		BodyLen = atoi(&ptr[5]);
		BodyLineLen = (ptr2 - ptr) + 2;
		BodyLen += RlineLen;
		MsgLen -= BodyLineLen;		// Length of Body Line may change

		memcpy(UnCompressed, MsgBytes, Index);	// Up to Old Body;
		BodyLineLen = wsprintf(&UnCompressed[Index], "Body: %d\r\n", BodyLen);

		MsgLen += BodyLineLen;		// Length of Body Line may have changed
		Index += BodyLineLen;

		ptr = strstr(ptr2, "\r\n\r\n");	// Blank line before Body

		ptr+=4;
	
		ptr2 +=2;					// Line Following Original Body: Line

		memcpy(&UnCompressed[Index], ptr2, ptr - ptr2); // Stuff Between Body: Line and Body

		Index += (ptr - ptr2);

		memcpy(&UnCompressed[Index], Rline, RlineLen);
		Index += RlineLen;
		memcpy(&UnCompressed[Index], ptr, MsgLen);		// Rest of Message
		
		FBBHeader->Size = MsgLen;

		Compressed = zalloc(2 * MsgLen + 200);

		CompLen = Encode(UnCompressed, Compressed, MsgLen, TRUE);

		ptr = strstr(UnCompressed, "\r\n\r\n");	// Blank line before Body

//		memcpy(DebugMsg, UnCompressed, ptr - UnCompressed);
//		Debugprintf("Paclin Trace: %s", DebugMsg);

		FBBHeader->CompressedMsg = Compressed;
		FBBHeader->CSize = CompLen;

		free(UnCompressed);

		return;
	}


	if (memcmp(MsgBytes, "R:", 2) != 0)    // No R line, so must be our message
		strcat(Rline, "\r\n");

	MsgLen = OrigLen + strlen(Rline);


//	if (conn->RestartFrom == 0)
//	{
//		// save time first sent, or checksum will be wrong when we restart
//		
//		FwdMsg->datechanged=time(NULL);
//	}

	tm = gmtime(&Msg->datechanged);	
	
	wsprintf(Date, "%04d%/%02d/%02d %02d:%02d",
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min);

	// We create the B2 Header
/*
	MID: XR88I1J160EB
	Date: 2009/07/25 18:17
	Type: Private
	From: SMTP:john.wiseman@ntlworld.com
	To: G8BPQ
	Subject: RE: RMS Test Message
	Mbo: SMTP
	Body: 213

*/
	if (strcmp(Msg->to, "RMS") == 0)		// Address is in via
		strcpy(B2To, Msg->via);
	else
		if (Msg->via[0] && (!conn->Paclink))
			wsprintf(B2To, "%s@%s", Msg->to, Msg->via);
		else
			strcpy(B2To, Msg->to);
	
	B2HddrLen = wsprintf(UnCompressed,
			"MID: %s\r\nDate: %s\r\nType: %s\r\nFrom: %s\r\nTo: %s\r\nSubject: %s\r\nMbo: %s\r\nBody: %d\r\n\r\n",
			Msg->bid, Date, (Msg->type == 'P') ? "Private" : "Bulletin",
			Msg->from, B2To, Msg->title, BBSName, MsgLen);

	strcat(UnCompressed, Rline);
	strcat(UnCompressed, MsgBytes);

	MsgLen += B2HddrLen;

	FBBHeader->Size = MsgLen;

	xptr = strstr(UnCompressed, "\r\n\r\n");	// Blank line before Body

//	memcpy(DebugMsg, UnCompressed, xptr - UnCompressed);
//	Debugprintf("Paclin Trace: %s", DebugMsg);

	Compressed = zalloc(2 * MsgLen + 200);

	CompLen = Encode(UnCompressed, Compressed, MsgLen, TRUE);

	FBBHeader->CompressedMsg = Compressed;
	FBBHeader->CSize = CompLen;

	free(UnCompressed);
}

VOID SendCompressedB2(CIRCUIT * conn, struct FBBHeaderLine * FBBHeader)
{
	UCHAR * Compressed, * Compressedptr;
	UCHAR * Output, * Outputptr;
	int i, CompLen;

	Compressed = Compressedptr = FBBHeader->CompressedMsg;

	Output = Outputptr = zalloc(FBBHeader->CSize + 1000);

	*Outputptr++ = 1;
	*Outputptr++ = strlen(FBBHeader->FwdMsg->title) + 8;
	strcpy(Outputptr, FBBHeader->FwdMsg->title);
	Outputptr += strlen(FBBHeader->FwdMsg->title) +1;
	wsprintf(Outputptr, "%06d", conn->RestartFrom);
	Outputptr += 7;

	CompLen = FBBHeader->CSize;

	conn->FBBChecksum = 0;

	// If restarting, send the checksum and length as a single record, then data from the restart point
	// The count includes the header, so adjust count and pointers

	if (conn->RestartFrom)
	{
		*Outputptr++ = 2;
		*Outputptr++ = 6;

		for (i=0; i< 6; i++)
		{
			conn->FBBChecksum+=Compressed[i];
			*Outputptr++ = Compressed[i];
		}
		
		for (i=conn->RestartFrom; i< CompLen; i++)
		{
			conn->FBBChecksum+=Compressed[i];
		}
		
		Compressedptr += conn->RestartFrom;
		CompLen -= conn->RestartFrom;
	}
	else
	{
		for (i=0; i< CompLen; i++)
		{
			conn->FBBChecksum+=Compressed[i];
		}
		conn->UserPointer->MsgsSent++;
		conn->UserPointer->BytesForwardedOut += FBBHeader->FwdMsg->length;

	}

	while (CompLen > 256)
	{
		*Outputptr++ = 2;
		*Outputptr++ = 0;

		memcpy(Outputptr, Compressedptr, 256);
		Outputptr += 256;
		Compressedptr += 256;
		CompLen -= 256;
	}

	*Outputptr++ = 2;
	*Outputptr++ = CompLen;

	memcpy(Outputptr, Compressedptr, CompLen);

	Outputptr += CompLen;

	*Outputptr++ = 4;
	conn->FBBChecksum = - conn->FBBChecksum;
	*Outputptr++ = conn->FBBChecksum;

	QueueMsg(conn, Output, Outputptr - Output);

	free(Compressed);
	free(Output);		
}

// Restart Routines.

VOID SaveFBBBinary(CIRCUIT * conn)
{
	// Disconnected during binary transfer

	char Msg[120];
	int len;
	struct FBBRestartData * RestartRec = zalloc(sizeof (struct FBBRestartData));
	
	if (conn->TempMsg->length < 256)
		return;							// Not worth it.

	// Need to save Header in conn->TempMsg and data in conn->MailBuffer

//	if (conn->InputLen < 2)
//		return;							//  All formats need at least two bytes
	
	GetSemaphore(&AllocSemaphore);

	RestartData=realloc(RestartData,(++RestartCount+1)*4);
	RestartData[RestartCount] = RestartRec;

	FreeSemaphore(&AllocSemaphore);

	RestartRec->UserPointer = conn->UserPointer;
	RestartRec->TempMsg = conn->TempMsg;
	RestartRec->MailBuffer = conn->MailBuffer;
	RestartRec->MailBufferSize = conn->MailBufferSize;

	len = sprintf_s(Msg, sizeof(Msg), "Disconnect received from %s during Binary Transfer - %d Bytes Saved for restart",
		conn->Callsign, conn->TempMsg->length);

	WriteLogLine(conn, '|',Msg, len, LOG_BBS);
}

BOOL LookupRestart(CIRCUIT * conn, struct FBBHeaderLine * FBBHeader)
{
	int i;

	struct FBBRestartData * RestartRec;

	if ((conn->BBSFlags & FBBB1Mode) == 0)
		return FALSE;						// Only B1 & B2 support restart

	for (i = 1; i <= RestartCount; i++)
	{
		RestartRec = RestartData[i];
		
		if ((RestartRec->UserPointer == conn->UserPointer)
			&& (strcmp(RestartRec->TempMsg->bid, FBBHeader->BID) == 0))
		{
			char Msg[120];
			int len;

			RestartRec->Count++;

			if (RestartRec->Count > 1)
			{
				len = sprintf_s(Msg, sizeof(Msg), "Too many restarts for %s - Requesting restart from beginning",
					FBBHeader->BID);
				
				WriteLogLine(conn, '|',Msg, len, LOG_BBS);
				return FALSE;
			}

			len = sprintf_s(Msg, sizeof(Msg), "Restart Data found for %s - Requesting restart from %d",
				FBBHeader->BID, RestartRec->TempMsg->length);

			WriteLogLine(conn, '|',Msg, len, LOG_BBS);

			return (RestartRec->TempMsg->length);
		}
	}

	return FALSE;				// Not Found
}



BOOL DoWeWantIt(struct FBBHeaderLine * FBBHeader)
{
	struct MsgInfo * Msg;
	BIDRec * BID;
	int m;

	if (FBBHeader->Size > MaxRXSize)
		return FALSE;
	
	BID = LookupBID(FBBHeader->BID);
	
	if (BID)
	{
		if (FBBHeader->MsgType == 'B')
			return FALSE;

		m = NumberofMessages;
		
		while (m > 0)
		{
			Msg = MsgHddrPtr[m];
 
			if (Msg->number == BID->u.msgno)
			{
				// if the same TO we will assume the same message

				if (strcmp(Msg->to, FBBHeader->To) == 0)
				{
					// We have this message. If we have already forwarded it, we should accept it again

					if ((Msg->status == 'N') || (Msg->status == 'Y')|| (Msg->status == 'H'))
						return FALSE;			// Dont want it
					else
						return TRUE;			// Get it again
				}

				// Same number. but different message (why?) Accept for now

				return TRUE; 
			}

			m--;
		}

		return TRUE; // A personal Message we have had before, but don't still have.
	}
	else
	{
		// We don't know the BID

		return TRUE;	// We want it
	}
}




