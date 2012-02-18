// Mail and Chat Server for BPQ32 Packet Switch
//
//	Utility Routines

#include "stdafx.h"
#include "Winspool.h"

int SEMCLASHES = 0;

VOID __cdecl Debugprintf(const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	WriteLogLine(NULL, '!',Mess, Len, LOG_DEBUG);
//	#ifdef _DEBUG 
	strcat(Mess, "\r\n");
	OutputDebugString(Mess);
//	#endif
	return;
}

VOID __cdecl Logprintf(int LogMode, CIRCUIT * conn, int InOut, const char * format, ...)
{
	char Mess[1000];
	va_list(arglist);int Len;

	va_start(arglist, format);
	Len = vsprintf_s(Mess, sizeof(Mess), format, arglist);
	WriteLogLine(conn, InOut, Mess, Len, LogMode);

	return;
}

void GetSemaphore(struct SEM * Semaphore)
{
	//
	//	Wait for it to be free
	//
	
	if (Semaphore->Flag != 0)
	{
		Semaphore->Clashes++;
//		Debugprintf("MailChat Semaphore Clash");
	}
loop1:

	while (Semaphore->Flag != 0)
	{
		Sleep(10);
	}

	//
	//	try to get semaphore
	//

	_asm{

	mov	eax,1
	mov ebx, Semaphore
	xchg [ebx],eax		// this instruction is locked
	
	cmp	eax,0
	jne loop1			// someone else got it - try again
;
;	ok, we've got the semaphore
;
	}

	return;
}
void FreeSemaphore(struct SEM * Semaphore)
{
	Semaphore->Flag=0;

	return; 
}
VOID BBSputs(CIRCUIT * conn, char * buf)
{
	// Sends to user and logs

	WriteLogLine(conn, '>',buf,  strlen(buf) -1, LOG_BBS);

	QueueMsg(conn, buf, strlen(buf));
}

VOID __cdecl nodeprintf(ConnectionInfo * conn, const char * format, ...)
{
	char Mess[1000];
	int len;
	va_list(arglist);

	
	va_start(arglist, format);
	len = vsprintf_s(Mess, sizeof(Mess), format, arglist);

	QueueMsg(conn, Mess, len);

	WriteLogLine(conn, '>',Mess, len-1, LOG_BBS);

	return;
}

int compare( const void *arg1, const void *arg2 );

VOID SortBBSChain()
{
	struct UserInfo * user;
	struct UserInfo * users[100]; 
	int i = 0, n;

	// Get array of addresses

	for (user = BBSChain; user; user = user->BBSNext)
	{
		users[i++] = user;
		if (i > 99) break;
	}

	qsort((void *)users, i, 4, compare );

	BBSChain = NULL;

	// Rechain (backwards, as entries ate puton front of chain)

	for (n = i-1; n >= 0; n--)
	{
		users[n]->BBSNext = BBSChain;
		BBSChain = users[n];
	}
}

int compare(const void *arg1, const void *arg2)
{
   // Compare Calls. Fortunately at start of stuct

   return _stricmp(*(char**)arg1 , *(char**)arg2);
}

int CountMessagesTo(struct UserInfo * user, int * Unread)
{
	int i, Msgs = 0;
	UCHAR * Call = user->Call;

	*Unread = 0;

	for (i = NumberofMessages; i > 0; i--)
	{
		if (MsgHddrPtr[i]->status == 'K')
			continue;

		if (_stricmp(MsgHddrPtr[i]->to, Call) == 0)
		{
			Msgs++;
			if (MsgHddrPtr[i]->status == 'N')
				*Unread = *Unread + 1;

		}
	}
	
	return(Msgs);
}



// Costimised message handling routines.
/*
	Variables - a subset of those used by FBB

 $C : Number of the next message.
 $I : First name of the connected user.
 $L : Number of the latest message.
 $N : Number of active messages.
 $U : Callsign of the connected user.
 $W : Inserts a carriage return.
 $Z : Last message read by the user (L command).
 %X : Number of messages for the user.
 %x : Number of new messages for the user.
*/

VOID ExpandAndSendMessage(CIRCUIT * conn, char * Msg, int LOG)
{
	char NewMessage[10000];
	char * OldP = Msg;
	char * NewP = NewMessage;
	char * ptr, * pptr;
	int len;
	char Dollar[] = "$";
	char CR[] = "\r";
	char num[20];
	int Msgs = 0, Unread = 0;


	ptr = strchr(OldP, '$');

	while (ptr)
	{
		len = ptr - OldP;		// Chars before $
		memcpy(NewP, OldP, len);
		NewP += len;

		switch (*++ptr)
		{
		case 'I': // First name of the connected user.

			pptr = conn->UserPointer->Name;
			break;

		case 'L': // Number of the latest message.

			wsprintf(num, "%d", LatestMsg);
			pptr = num;
			break;

		case 'N': // Number of active messages.

			wsprintf(num, "%d", NumberofMessages);
			pptr = num;
			break;


		case 'U': // Callsign of the connected user.

			pptr = conn->UserPointer->Call;
			break;

		case 'W': // Inserts a carriage return.

			pptr = CR;
			break;

		case 'Z': // Last message read by the user (L command).

			wsprintf(num, "%d", conn->UserPointer->lastmsg);
			pptr = num;
			break;

		case 'X': // Number of messages for the user.

			Msgs = CountMessagesTo(conn->UserPointer, &Unread);
			wsprintf(num, "%d", Msgs);
			pptr = num;
			break;

		case 'x': // Number of new messages for the user.

			Msgs = CountMessagesTo(conn->UserPointer, &Unread);
			wsprintf(num, "%d", Unread);
			pptr = num;
			break;

		default:

			pptr = Dollar;		// Just Copy $
		}

		len = strlen(pptr);
		memcpy(NewP, pptr, len);
		NewP += len;

		OldP = ++ptr;
		ptr = strchr(OldP, '$');
	}

	strcpy(NewP, OldP);

	len = RemoveLF(NewMessage, strlen(NewMessage));

	WriteLogLine(conn, '>', NewMessage,  len, LOG);
	QueueMsg(conn, NewMessage, len);
}

BOOL isdigits(char * string)
{
	// Returns TRUE id sting is decimal digits

	int i, n = strlen(string);
	
	for (i = 0; i < n; i++)
	{
		if (isdigit(string[i]) == FALSE) return FALSE;
	}
	return TRUE;
}

BOOL wildcardcompare(char * Target, char * Match)
{
	// Do a compare with string *string string* *string*

	// Strings should all be UC

	char Pattern[100];
	char * firststar;

	strcpy(Pattern, Match);
	firststar = strchr(Pattern,'*');

	if (firststar)
	{
		int Len = strlen(Pattern);

		if (Pattern[0] == '*' && Pattern[Len - 1] == '*')		// * at start and end
		{
			Pattern[Len - 1] = 0;
			return (BOOL)(strstr(Target, &Pattern[1]));
		}
		if (Pattern[0] == '*')		// * at start
		{
			// Compare the last len - 1 chars of Target

			int Targlen = strlen(Target);
			int Comparelen = Targlen - (Len - 1);

			if (Len == 1)			// Just *
				return TRUE;

			if (Comparelen < 0)	// Too Short
				return FALSE;

			return (memcmp(&Target[Comparelen], &Pattern[1], Len - 1) == 0);
		}

		// Must be * at end - compare first Len-1 char

		return (memcmp(Target, Pattern, Len - 1) == 0);

	}

	// No WildCards - straight strcmp
	return (strcmp(Target, Pattern) == 0);
}

VOID SaveFwdParams(char * Call, struct BBSForwardingInfo * ForwardingInfo)
{
	HKEY hKey=0;
	int disp;
	char Key[100] =  "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\BBSForwarding\\";

	strcat(Key, Call);

	RegCreateKeyEx(REGTREE, Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	RegSetValueEx(hKey, "Enabled", 0, REG_DWORD, (BYTE *)&ForwardingInfo->Enabled, 4);

	RegSetValueEx(hKey, "RequestReverse", 0, REG_DWORD, (BYTE *)&ForwardingInfo->ReverseFlag, 4);
	
	RegSetValueEx(hKey, "FWDInterval", 0, REG_DWORD, (BYTE *)&ForwardingInfo->FwdInterval, 4);
	
	RegSetValueEx(hKey, "FWD New Immediately", 0, REG_DWORD, (BYTE *)&ForwardingInfo->SendNew, 4);

	
	RegCloseKey(hKey);
}
PrintMessage(HDC hDC, struct MsgInfo * Msg);

PrintMessages(HWND hDlg, int Count, int * Indexes)
{
	int i, CurrentMsgIndex;
	char MsgnoText[10];
	int Msgno;
	struct MsgInfo * Msg;
	int Len = MAX_PATH;
	BOOL hResult;
    PRINTDLG pdx = {0};
	HDC hDC;

//	CHOOSEFONT cf; 
	LOGFONT lf; 
    HFONT hFont; 
 
 
    //  Initialize the PRINTDLG structure.

    pdx.lStructSize = sizeof(PRINTDLG);
    pdx.hwndOwner = hWnd;
    pdx.hDevMode = NULL;
    pdx.hDevNames = NULL;
    pdx.hDC = NULL;
    pdx.Flags = PD_RETURNDC | PD_COLLATE;
    pdx.nMinPage = 1;
    pdx.nMaxPage = 1000;
    pdx.nCopies = 1;
    pdx.hInstance = 0;
    pdx.lpPrintTemplateName = NULL;
    
    //  Invoke the Print property sheet.
    
    hResult = PrintDlg(&pdx);

	memset(&lf, 0, sizeof(LOGFONT));

 /*
 
	// Initialize members of the CHOOSEFONT structure.  
 
    cf.lStructSize = sizeof(CHOOSEFONT); 
    cf.hwndOwner = (HWND)NULL; 
    cf.hDC = pdx.hDC; 
    cf.lpLogFont = &lf; 
    cf.iPointSize = 0; 
    cf.Flags = CF_PRINTERFONTS | CF_FIXEDPITCHONLY; 
    cf.rgbColors = RGB(0,0,0); 
    cf.lCustData = 0L; 
    cf.lpfnHook = (LPCFHOOKPROC)NULL; 
    cf.lpTemplateName = (LPSTR)NULL; 
    cf.hInstance = (HINSTANCE) NULL; 
    cf.lpszStyle = (LPSTR)NULL; 
    cf.nFontType = PRINTER_FONTTYPE; 
    cf.nSizeMin = 0; 
    cf.nSizeMax = 0; 
 
    // Display the CHOOSEFONT common-dialog box.  
 
    ChooseFont(&cf); 
 
    // Create a logical font based on the user's  
    // selection and return a handle identifying  
    // that font. 
*/

	lf.lfHeight =  -56;
	lf.lfWeight = 600;
	lf.lfOutPrecision = 3;
	lf.lfClipPrecision = 2;
	lf.lfQuality = 1;
	lf.lfPitchAndFamily = '1';
	strcpy (lf.lfFaceName, "Courier New");

    hFont = CreateFontIndirect(&lf); 

    if (hResult)
    {
        // User clicked the Print button, so use the DC and other information returned in the 
        // PRINTDLG structure to print the document.

		DOCINFO pdi;

		pdi.cbSize = sizeof(DOCINFO);
		pdi.lpszDocName = "BBS Message Print";
		pdi.lpszOutput = NULL;
		pdi.lpszDatatype = "RAW";
		pdi.fwType = 0;

		hDC = pdx.hDC;

		SelectObject(hDC, hFont);

		StartDoc(hDC, &pdi);
		StartPage(hDC);

		for (i = 0; i < Count; i++)
		{
			SendDlgItemMessage(hDlg, 0, LB_GETTEXT, Indexes[i], (LPARAM)(LPCTSTR)&MsgnoText);
	
			Msgno = atoi(MsgnoText);

			for (CurrentMsgIndex = 1; CurrentMsgIndex <= NumberofMessages; CurrentMsgIndex++)
			{
				Msg = MsgHddrPtr[CurrentMsgIndex];
	
				if (Msg->number == Msgno)
				{
					PrintMessage(hDC, Msg);
					break;
				}
			}
		}
		
		EndDoc(hDC);
	}

    if (pdx.hDevMode != NULL) 
        GlobalFree(pdx.hDevMode); 
    if (pdx.hDevNames != NULL) 
        GlobalFree(pdx.hDevNames); 

    if (pdx.hDC != NULL) 
        DeleteDC(pdx.hDC);

}

PrintMessage(HDC hDC, struct MsgInfo * Msg)
{
	int Len = MAX_PATH;
	char * MsgBytes;
	char * Save;
  	int Msglen;
 
	StartPage(hDC);

	Save = MsgBytes = ReadMessageFile(Msg->number);

	Msglen = Msg->length;

	if (MsgBytes)
	{
		char Hddr[1000];
		char FullTo[100];
		int HRes, VRes;
		char * ptr1, * ptr2;
		int LineLen;

		RECT Rect;

		if (_stricmp(Msg->to, "RMS") == 0)
			 wsprintf(FullTo, "RMS:%s", Msg->via);
		else
		if (Msg->to[0] == 0)
			wsprintf(FullTo, "smtp:%s", Msg->via);
		else
			strcpy(FullTo, Msg->to);


		wsprintf(Hddr, "From: %s%s\r\nTo: %s\r\nType/Status: %c%c\r\nDate/Time: %s\r\nBid: %s\r\nTitle: %s\r\n\r\n",
			Msg->from, Msg->emailfrom, FullTo, Msg->type, Msg->status, FormatDateAndTime(Msg->datecreated, FALSE), Msg->bid, Msg->title);


		if (Msg->B2Flags)
		{
			// Remove B2 Headers (up to the File: Line)
			
			char * ptr;
			ptr = strstr(MsgBytes, "Body:");
			if (ptr)
			{
				Msglen = atoi(ptr + 5);
				ptr = strstr(ptr, "\r\n\r\n");
			}
			if (ptr)
				MsgBytes = ptr + 4;
		}

		HRes = GetDeviceCaps(hDC, HORZRES) - 50;
		VRes = GetDeviceCaps(hDC, VERTRES) - 50;

		Rect.top = 50;
		Rect.left = 50;
		Rect.right = HRes;
		Rect.bottom = VRes;

		DrawText(hDC, Hddr, strlen(Hddr), &Rect, DT_CALCRECT | DT_WORDBREAK);
		DrawText(hDC, Hddr, strlen(Hddr), &Rect, DT_WORDBREAK);

		// process message a line at a time. When page is full, output a page break

		ptr1 = MsgBytes;
		ptr2 = ptr1;

		while (Msglen-- > 0)
		{	
			if (*ptr1++ == '\r')
			{
				// Output this line

				// First check if it will fit

				Rect.top = Rect.bottom;
				Rect.right = HRes;
				Rect.bottom = VRes;

				LineLen = ptr1 - ptr2 - 1;
			
				if (LineLen == 0)		// Blank line
					Rect.bottom = Rect.top + 40;
				else
					DrawText(hDC, ptr2, ptr1 - ptr2 - 1, &Rect, DT_CALCRECT | DT_WORDBREAK);

				if (Rect.bottom >= VRes)
				{
					EndPage(hDC);
					StartPage(hDC);

					Rect.top = 50;
					Rect.bottom = VRes;
					if (LineLen == 0)		// Blank line
						Rect.bottom = Rect.top + 40;
					else
						DrawText(hDC, ptr2, ptr1 - ptr2 - 1, &Rect, DT_CALCRECT | DT_WORDBREAK);
				}

				if (LineLen == 0)		// Blank line
					Rect.bottom = Rect.top + 40;
				else
					DrawText(hDC, ptr2, ptr1 - ptr2 - 1, &Rect, DT_WORDBREAK);

				if (*(ptr1) == '\n')
				{
					ptr1++;
					Msglen--;
				}

				ptr2 = ptr1;
			}
		}
	
		free(Save);
	
		EndPage(hDC);

		}
		return 0;
}

VOID ImportMessages()
{
	char FileName[MAX_PATH] = "Messages.in";
	int Files = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	int WriteLen=0;
	OPENFILENAME Ofn; 
	FILE *in;
	char Buffer[100000];
	char *buf = Buffer;

	memset(&Ofn, 0, sizeof(Ofn));
 
	Ofn.lStructSize = sizeof(OPENFILENAME); 
	Ofn.hInstance = hInst;
	Ofn.hwndOwner = MainWnd; 
	Ofn.lpstrFilter = NULL; 
	Ofn.lpstrFile= FileName; 
	Ofn.nMaxFile = sizeof(FileName)/ sizeof(*FileName); 
	Ofn.lpstrFileTitle = NULL; 
	Ofn.nMaxFileTitle = 0; 
	Ofn.lpstrInitialDir = BaseDir; 
	Ofn.Flags = OFN_SHOWHELP | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST; 
	Ofn.lpstrTitle = NULL;//; 

	if (!GetOpenFileName(&Ofn))
		return;

	in = fopen(FileName, "r");

	if (!(in))
	{
		char msg[500];
		sprintf_s(msg, sizeof(msg), "Failed to open %s", FileName);
		MessageBox(NULL, msg, "BPQMailChat", MB_OK);
		return;
	}
	while(fgets(Buffer, 99999, in))
	{
		// First line should start SP/SB ?ST?

		char * From = NULL;
		char * BID = NULL;
		char * ATBBS = NULL;
		char seps[] = " \t\r";
		struct MsgInfo * Msg;
		char SMTPTO[100]= "";
		int msglen;
		CIRCUIT conn;
		char * Context;
		char * Arg1, * Cmd;

		memset(&conn, 0, sizeof(CIRCUIT));

		conn.UserPointer = LookupCall(SYSOPCall);
		conn.sysop = TRUE;
		conn.BBSFlags = BBS;
		strcpy(conn.Callsign, "IMPORT");

NextMessage:

		Sleep(100);

		strlop(Buffer, 10);
		strlop(Buffer, 13);				// Remove cr and/or lf

		WriteLogLine(&conn, '>', Buffer, strlen(Buffer), LOG_BBS);

		Cmd = strtok_s(Buffer, seps, &Context);

		if (Cmd == NULL)
		{
			fclose(in);
			return;
		}

		Arg1 = strtok_s(NULL, seps, &Context);

		if (Arg1 == NULL)
		{
			Debugprintf("Bad Import Line %s", Buffer);
			fclose(in);
			return;
		}

		if (DecodeSendParams(&conn, Context, &From, &Arg1, &ATBBS, &BID))
		{
			if (CreateMessage(&conn, From, Arg1, ATBBS, toupper(Cmd[1]), BID, NULL))
			{
				Msg = conn.TempMsg;

				// SP is Ok, read message;

				ClearQueue(&conn);

				fgets(Buffer, 99999, in);
				strlop(Buffer, 10);
				strlop(Buffer, 13);				// Remove cr and/or lf
				strcpy(Msg->title, Buffer);

				// Read the lines
		
				conn.Flags |= GETTINGMESSAGE;

				Buffer[0] = 0;

				fgets(Buffer, 99999, in);

				while ((conn.Flags & GETTINGMESSAGE) && Buffer[0])
				{
					strlop(Buffer, 10);
					strlop(Buffer, 13);				// Remove cr and/or lf
					msglen = strlen(Buffer);
					Buffer[msglen++] = 13;
					ProcessMsgLine(&conn, conn.UserPointer,Buffer, msglen);
	
					Buffer[0] = 0;
					fgets(Buffer, 99999, in);
				}

				// Message completed (or off end of file)

				ClearQueue(&conn);

				if (Buffer[0])
					goto NextMessage;		// We have read the SP/SB line;
				else
				{
					fclose(in);
					return;
				}
			}
		}

		// Search for next message 

		Buffer[0] = 0;
		fgets(Buffer, 99999, in);

		while (Buffer[0])
		{
			strlop(Buffer, 10);
			strlop(Buffer, 13);				// Remove cr and/or lf

			if (_stricmp(Buffer, "/EX") == 0)
			{
				// Found end

				Buffer[0] = 0;
				fgets(Buffer, 99999, in);
						
				ClearQueue(&conn);

				if (Buffer[0])
					goto NextMessage;		// We have read the SP/SB line;

			}
			
			Buffer[0] = 0;
			fgets(Buffer, 99999, in);
		}
	}

	fclose(in);
}