// Mail and Chat Server for BPQ32 Packet Switch
//
//	Housekeeping Module

#include "BPQMailChat.h"

int LogAge = 7;

BOOL DeletetoRecycleBin = FALSE;
BOOL SuppressMaintEmail = FALSE;
BOOL SaveRegDuringMaint = FALSE;
BOOL OverrideUnsent = FALSE;
BOOL SendNonDeliveryMsgs = TRUE;
VOID UpdateWP();

int PR = 30;
int PUR = 30;
int PF = 30;
int PNF = 30;
int BF = 30;
int BNF = 30;
int AP;
int AB;

struct Override ** LTFROM;
struct Override ** LTTO;
struct Override ** LTAT;

int DeleteLogFiles();

VOID SendNonDeliveryMessage(struct MsgInfo * OldMsg, BOOL Forwarded, int Age);
int CreateWPMessage();

int LastHouseKeepingTime;
int LastTrafficTime;

void DeletetoRecycle(char * FN)
{
#ifdef WIN32
	SHFILEOPSTRUCT FileOp;

	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_DELETE;
	FileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR | FOF_ALLOWUNDO;
	FileOp.pFrom = FN;
	FileOp.pTo = NULL;

	SHFileOperation(&FileOp);
#endif
}

VOID FreeOverride(struct Override ** Hddr)
{
	struct Override ** Save;
	
	if (Hddr)
	{
		Save = Hddr;
		while(Hddr[0])
		{
			free(Hddr[0]->Call);
			free(Hddr[0]);
			Hddr++;
		}
		
		free(Save);
	}

}

VOID FreeOverrides()
{
	FreeOverride(LTFROM);
	FreeOverride(LTTO);
	FreeOverride(LTAT);
}

#ifdef LINBPQ

VOID * GetOverrides(config_setting_t * group, char * ValueName)
{
	char * ptr1;
	char * MultiString = NULL;
	char * ptr;
	int Count = 0;
	struct Override ** Value;
	char * Val;

	config_setting_t *setting;

	Value = zalloc(4);				// always NULL entry on end even if no values
	Value[0] = NULL;

	setting = config_setting_get_member (group, ValueName);
	
	if (setting)
	{
		ptr =  (char *)config_setting_get_string (setting);
	
		while (ptr && strlen(ptr))
		{
			ptr1 = strchr(ptr, '|');
			
			if (ptr1)
				*(ptr1++) = 0;

			Value = realloc(Value, (Count+2)*4);
			Value[Count] = zalloc(sizeof(struct Override));
			Val = strlop(ptr, ',');
			if (Val == NULL)
				break;

			Value[Count]->Call = _strupr(_strdup(ptr));
			Value[Count++]->Days = atoi(Val);
			ptr = ptr1;
		}
	}

	Value[Count] = NULL;
	return Value;
}

#else


VOID * GetOverrides(HKEY hKey, char * ValueName)
{
	int retCode,Type,Vallen;
	char * MultiString;
	int ptr, len;
	int Count = 0;
	struct Override ** Value;
	char * Val;


	Value = zalloc(4);				// always NULL entry on end even if no values

	Value[0] = NULL;

	Vallen=0;

	retCode = RegQueryValueEx(hKey, ValueName, 0, (ULONG *)&Type, NULL, (ULONG *)&Vallen);

	if ((retCode != 0)  || (Vallen == 0)) 
		return FALSE;

	MultiString = malloc(Vallen);

	retCode = RegQueryValueEx(hKey, ValueName, 0,			
			(ULONG *)&Type,(UCHAR *)MultiString,(ULONG *)&Vallen);

	ptr=0;

	while (MultiString[ptr])
	{
		len=strlen(&MultiString[ptr]);

		Value = realloc(Value, (Count+2)*4);
		Value[Count] = zalloc(sizeof(struct Override));
		Val = strlop(&MultiString[ptr], ',');
		if (Val == NULL)
			break;

		Value[Count]->Call = _strupr(_strdup(&MultiString[ptr]));
		Value[Count++]->Days = atoi(Val);
		ptr+= (len + 1);
	}

	Value[Count] = NULL;

	free(MultiString);

	return Value;
}

#endif

int Removed;
int Killed;
int BIDSRemoved;

#ifndef LINBPQ

INT_PTR CALLBACK HKDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command;
		
	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemInt(hDlg, IDC_REMOVED, Removed, FALSE);
		SetDlgItemInt(hDlg, IDC_KILLED, Killed, FALSE);
		SetDlgItemInt(hDlg, IDC_LIVE, NumberofMessages - Killed, FALSE);
		SetDlgItemInt(hDlg, IDC_TOTAL, NumberofMessages, FALSE);
		SetDlgItemInt(hDlg, IDC_BIDSREMOVED, BIDSRemoved, FALSE);
		SetDlgItemInt(hDlg, IDC_BIDSLEFT, NumberofBIDs, FALSE);

		return (INT_PTR)TRUE;

	case WM_COMMAND:

		Command = LOWORD(wParam);

		switch (Command)
		{
		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		}
		break;
	}
	
	return 0;
}
#endif

VOID DoHouseKeeping(BOOL Manual)
{
	HKEY hKey=0;
	int retCode, disp;
	time_t NOW;

	UpdateWP();

	DeleteLogFiles();

	RemoveKilledMessages();
	ExpireMessages();
	
	GetSemaphore(&AllocSemaphore);
	ExpireBIDs();
	FreeSemaphore(&AllocSemaphore);

	if (LatestMsg > MaxMsgno)
	{
		GetSemaphore(&MsgNoSemaphore);
		GetSemaphore(&AllocSemaphore);

		Renumber_Messages();
	
		FreeSemaphore(&AllocSemaphore);
		FreeSemaphore(&MsgNoSemaphore);
	}

	if (!SuppressMaintEmail)
		MailHousekeepingResults();
	
	LastHouseKeepingTime = NOW = time(NULL);

#ifndef LINBPQ
	retCode = RegCreateKeyEx(REGTREE,
          "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\HouseKeeping",0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		retCode = RegSetValueEx(hKey,"LastHouseKeepingTime",0,REG_DWORD,(BYTE *)&NOW,4);
		RegCloseKey(hKey);
	}

	if (SaveRegDuringMaint)
		CreateRegBackup();

	if (Manual)
		DialogBox(hInst, MAKEINTRESOURCE(IDD_MAINTRESULTS), hWnd, HKDialogProc);

#endif

	if (SendWP)
		CreateWPMessage();

	return;
}

VOID ExpireMessages()
{
	struct MsgInfo * Msg;
	int n;
	int PRLimit;
	int PURLimit;
	int PFLimit;
	int PNFLimit;
	int BFLimit;
	int BNFLimit;
	int BLimit;

	struct Override ** Calls;

	int now=time(NULL);
	int Future = now + (7 * 86400);

	Killed = 0;

	PRLimit = now - PR*86400;
	PURLimit = now - PUR*86400;
	PFLimit = now - PF*86400;
	PNFLimit = now - PNF*86400;
	BFLimit = now - BF*86400;
	BNFLimit = now - BNF*86400;


	for (n = 1; n <= NumberofMessages; n++)
	{
		Msg = MsgHddrPtr[n];

		// If from the future, Kill it

		if (Msg->datecreated > Future)
		{
			KillMsg(Msg);
			continue;
		}

		switch (Msg->type)
		{
		case 'P':
		case 'T':

			switch (Msg->status)
			{
			case 'N':
			case 'H':

				// Is it unforwarded or unread?

				if (memcmp(Msg->fbbs, zeros, NBMASK) == 0)
				{
					if (Msg->datecreated < PURLimit)
					{
						if (SendNonDeliveryMsgs) 
							SendNonDeliveryMessage(Msg, TRUE, PUR);

						KillMsg(Msg);
					}
				}
				else
				{
					if (Msg->datecreated < PNFLimit)
					{
						if (SendNonDeliveryMsgs) 
							SendNonDeliveryMessage(Msg, FALSE, PNF);

						KillMsg(Msg);
					}
				}
				continue;	
	
			case 'F':

				if (Msg->datechanged < PFLimit) KillMsg(Msg);

				continue;	

			case 'Y':
			case 'D':

				if (Msg->datechanged < PRLimit) KillMsg(Msg);

				continue;

			default:

				continue;

			}			
			
		case 'B':

			BLimit = BF;
			BNFLimit = now - BNF*86400;

			// Check FROM Overrides

			if (LTFROM)
			{
				Calls = LTFROM;

				while(Calls[0])
				{
					if (strcmp(Calls[0]->Call, Msg->from) == 0)
					{
						BLimit = Calls[0]->Days;
						goto gotit;
					}
					Calls++;
				}
			}

			// Check TO Overrides

			if (LTTO)
			{
				Calls = LTTO;

				while(Calls[0])
				{
					if (strcmp(Calls[0]->Call, Msg->to) == 0)
					{
						BLimit = Calls[0]->Days;
						goto gotit;
					}
					Calls++;
				}
			}

			// Check AT Overrides

			if (LTAT)
			{
				Calls = LTAT;

				while(Calls[0])
				{
					if (strcmp(Calls[0]->Call, Msg->via) == 0)
					{
						BLimit = Calls[0]->Days;
						goto gotit;
					}
					Calls++;
				}
			}

		gotit:

			BFLimit = now - BLimit*86400;

			if (OverrideUnsent)
				if (BLimit != BF)		// Have we an override?
					BNFLimit = BFLimit;

			switch (Msg->status)
			{
			case '$':
			case 'N':
			case ' ':
			case 'H':


				if (Msg->datecreated < BNFLimit)
					KillMsg(Msg);
				break;	

			case 'F':
			case 'Y':

				if (Msg->datecreated < BFLimit) 
					KillMsg(Msg);
				break;	
			}			
		}
	}
}


VOID KillMsg(struct MsgInfo * Msg)
{
	FlagAsKilled(Msg);
	Killed++;
}

BOOL RemoveKilledMessages()
{
	struct MsgInfo * Msg;
	struct MsgInfo ** NewMsgHddrPtr;
	char MsgFile[MAX_PATH];
	int i, n;

	Removed = 0;

	GetSemaphore(&MsgNoSemaphore);
	GetSemaphore(&AllocSemaphore);

	FirstMessageIndextoForward = 0;

	NewMsgHddrPtr = zalloc((NumberofMessages+1) * 4);
	NewMsgHddrPtr[0] = MsgHddrPtr[0];		// Copy Control Record

	i = 0;

	for (n = 1; n <= NumberofMessages; n++)
	{
		Msg = MsgHddrPtr[n];

		if (Msg->status == 'K')
		{
			sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes%c", MailDir, Msg->number, 0);
			if (DeletetoRecycleBin)
				DeletetoRecycle(MsgFile);
			else
				DeleteFile(MsgFile);

			MsgnotoMsg[Msg->number] = NULL;	
			free(Msg);

			Removed++;
		}
		else
		{
			NewMsgHddrPtr[++i] = Msg;
			if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
			{
				if (FirstMessageIndextoForward == 0)
					FirstMessageIndextoForward = i;
			}
		}
	}

	NumberofMessages = i;
	NewMsgHddrPtr[0]->number = i;

	if (FirstMessageIndextoForward == 0)
		FirstMessageIndextoForward = NumberofMessages;

	free(MsgHddrPtr);

	MsgHddrPtr = NewMsgHddrPtr;

	FreeSemaphore(&MsgNoSemaphore);
	FreeSemaphore(&AllocSemaphore);

	SaveMessageDatabase();

	return TRUE;

}

VOID Renumber_Messages()
{
	int NewNumber[65536] = {0};
	struct MsgInfo * Msg;
	struct UserInfo * user = NULL;
	char OldMsgFile[MAX_PATH];
	char NewMsgFile[MAX_PATH];
	int j, lastmsg, result;

	int i, n;

	for (i = 0; i < 100000; i++)
	{
		MsgnotoMsg[i] = NULL;
	}


	i = 0;		// New Message Number

	for (n = 1; n <= NumberofMessages; n++)
	{
		Msg = MsgHddrPtr[n];

		NewNumber[Msg->number] = ++i;		// Save so we can update users' last listed count

		// New will always be >= old unless somethnig hasgon horribly wrong,
		// so can rename in place without risk of losing a message

		if (Msg->number < i)
		{
#ifndef LINBPQ
			MessageBox(MainWnd, "Invalid message number detected, quitting", "BPQMailChat", MB_OK);
#else
			Debugprintf("Invalid message number detected, quitting");
#endif
			SaveMessageDatabase();
			return;
		}

		if (Msg->number != i)
		{
			sprintf(OldMsgFile, "%s/m_%06d.mes", MailDir, Msg->number);
			sprintf(NewMsgFile, "%s/m_%06d.mes", MailDir, i);
			result = rename(OldMsgFile, NewMsgFile);
			if (result)
			{
				char Errmsg[100];
				sprintf(Errmsg, "Could not rename message no %d to %d, quitting", Msg->number, i);
#ifndef LINBPQ
				MessageBox(MainWnd,Errmsg , "BPQMailChat", MB_OK);
#else
				Debugprintf(Errmsg);
#endif
				SaveMessageDatabase();
				return;
			}
			Msg->number = i;
			MsgnotoMsg[i] = Msg;
		}
		
	}

	for (n = 0; n <= NumberofUsers; n++)
	{
		user = UserRecPtr[n];
		lastmsg = user->lastmsg;

		if (lastmsg <= 0)
			user->lastmsg = 0;
		else
		{
			j = NewNumber[lastmsg];

			if (j == 0)
			{
				// Last listed has gone. Find next above

				while(++lastmsg < 65536)
				{
					if (NewNumber[lastmsg] != 0)
					{
						user->lastmsg = NewNumber[lastmsg];
						break;
					}
				}	

			}
			user->lastmsg = NewNumber[lastmsg];
		}
	}

	MsgHddrPtr[0]->length = LatestMsg = i;

	SaveMessageDatabase();
	SaveUserDatabase();

	return;

}

BOOL ExpireBIDs()
{
	BIDRec * BID;
	BIDRec ** NewBIDRecPtr;
	unsigned short now=LOWORD(time(NULL)/86400);

	int i, n;

	NewBIDRecPtr = zalloc((NumberofBIDs+1) * 4);
	NewBIDRecPtr[0] = BIDRecPtr[0];		// Copy Control Record

	i = 0;

	for (n = 1; n <= NumberofBIDs; n++)
	{
		BID = BIDRecPtr[n];

//		Debugprintf("%d %d", BID->u.timestamp, now - BID->u.timestamp);

		if ((now - BID->u.timestamp) < BidLifetime)
			NewBIDRecPtr[++i] = BID;
	}

	BIDSRemoved = NumberofBIDs - i;

	NumberofBIDs = i;
	NewBIDRecPtr[0]->u.msgno = i;

	free(BIDRecPtr);

	BIDRecPtr = NewBIDRecPtr;

	SaveBIDDatabase();

	return TRUE;

}

VOID MailHousekeepingResults()
{
	int Length=0;
	char * MailBuffer = malloc(10000);

	Length += sprintf(&MailBuffer[Length], "Killed Messsages Removed %d\r\n", Removed);
	Length += sprintf(&MailBuffer[Length], "Messages Killed          %d\r\n", Killed);
	Length += sprintf(&MailBuffer[Length], "Live Messages            %d\r\n", NumberofMessages - Killed);
	Length += sprintf(&MailBuffer[Length], "Total Messages           %d\r\n", NumberofMessages);
	Length += sprintf(&MailBuffer[Length], "BIDs Removed             %d\r\n", BIDSRemoved);
	Length += sprintf(&MailBuffer[Length], "BIDs Left                %d\r\n", NumberofBIDs);

	SendMessageToSYSOP("Housekeeping Results", MailBuffer, Length);
}

#ifdef WIN32

int DeleteLogFiles()
{
   WIN32_FIND_DATA ffd;

   char szDir[MAX_PATH];
   char File[MAX_PATH];
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError=0;
   LARGE_INTEGER ft;
   time_t now = time(NULL);
   int Age;

   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.

   strcpy(szDir, BaseDir);
   strcat(szDir, "\\Log_*.txt");

   // Find the first file in the directory.

   hFind = FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) 
   {
      return dwError;
   } 
   
   // List all the files in the directory with some info about them.

   do
   {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         OutputDebugString(ffd.cFileName);
      }
      else
      {
         ft.HighPart = ffd.ftCreationTime.dwHighDateTime;
         ft.LowPart = ffd.ftCreationTime.dwLowDateTime;

		 ft.QuadPart -=  116444736000000000;
		 ft.QuadPart /= 10000000;

		 Age = (now - ft.LowPart) / 86400; 

		 if (Age > LogAge)
		 {
			 sprintf(File, "%s/%s%c", BaseDir, ffd.cFileName, 0);
			 if (DeletetoRecycleBin)
				DeletetoRecycle(File);
			 else
				 DeleteFile(File);
		 }
      }
   }
   while (FindNextFile(hFind, &ffd) != 0);
 
   dwError = GetLastError();

   FindClose(hFind);
   return dwError;
}

#else

#include <dirent.h>

int Filter(const struct dirent * dir)
{
	return memcmp(dir->d_name, "log", 3) == 0 && strstr(dir->d_name, ".txt");
}

int DeleteLogFiles()
{
	struct dirent **namelist;
    int n;
	struct stat STAT;
	time_t now = time(NULL);
	int Age = 0, res;
     	
    n = scandir(".", &namelist, Filter, alphasort);

	if (n < 0) 
		perror("scandir");
	else  
	{ 
		while(n--)
		{
			if (stat(namelist[n]->d_name, &STAT) == 0)
			{
				Age = (now - STAT.st_mtime) / 86400;
				
				if (Age > LogAge)
				{
					printf("Deleting  %s\n",  namelist[n]->d_name);
					unlink(namelist[n]->d_name);
				}
			}
			free(namelist[n]);
		}
		free(namelist);
    }
	return;
}
#endif


VOID SendNonDeliveryMessage(struct MsgInfo * OldMsg, BOOL Unread, int Age)
{
	struct MsgInfo * Msg = AllocateMsgRecord();
	BIDRec * BIDRec;
	char MailBuffer[1000];
	char MsgFile[MAX_PATH];
	FILE * hFile;
	int WriteLen=0;
	char From[100];
	char * Via;
	struct UserInfo * FromUser;

	// Try to create a from Address. ( ? check RMS)

	strcpy(From, OldMsg->from);

	FromUser = LookupCall(OldMsg->from);

	if (FromUser)
	{
		if (FromUser->HomeBBS[0])
			sprintf(From, "%s@%s", OldMsg->from, FromUser->HomeBBS);
		else
			sprintf(From, "%s@%s", OldMsg->from, BBSName);
	}
	else
	{
		WPRecP WP = LookupWP(OldMsg->from);

		if (WP)
			sprintf(From, "%s@%s", OldMsg->from, WP->first_homebbs);
	}

	GetSemaphore(&MsgNoSemaphore);
	Msg->number = ++LatestMsg;
	MsgnotoMsg[Msg->number] = Msg;

	FreeSemaphore(&MsgNoSemaphore);
 
	strcpy(Msg->from, SYSOPCall);

	Via = strlop(From, '@');

	strcpy(Msg->to, From);
	if (Via)
		strcpy(Msg->via, Via);

	if (strcmp(From, "RMS:") == 0)
	{
		strcpy(Msg->to, "RMS");
		strcpy(Msg->via, OldMsg->emailfrom);
	}
	strcpy(Msg->title, "Non-delivery Notification");
	
	if (Unread)
		Msg->length = sprintf(MailBuffer, "Your Message ID %s Subject %s to %s has not been read for %d days.\r\nMessage had been deleted.\r\n", OldMsg->bid, OldMsg->title, OldMsg->to, Age);
	else
		Msg->length = sprintf(MailBuffer, "Your Message ID %s Subject %s to %s could not be delivered in %d days.\r\nMessage had been deleted.\r\n", OldMsg->bid, OldMsg->title, OldMsg->to, Age);


	Msg->type = 'P';
	Msg->status = 'N';
	Msg->datereceived = Msg->datechanged = Msg->datecreated = time(NULL);

	sprintf_s(Msg->bid, sizeof(Msg->bid), "%d_%s", LatestMsg, BBSName);

	BIDRec = AllocateBIDRecord();
	strcpy(BIDRec->BID, Msg->bid);
	BIDRec->mode = Msg->type;
	BIDRec->u.msgno = LOWORD(Msg->number);
	BIDRec->u.timestamp = LOWORD(time(NULL)/86400);

	sprintf_s(MsgFile, sizeof(MsgFile), "%s/m_%06d.mes", MailDir, Msg->number);
	
	hFile = fopen(MsgFile, "wb");

	if (hFile)
	{
		fwrite(MailBuffer, 1, Msg->length, hFile);
		fclose(hFile);
	}

	MatchMessagetoBBSList(Msg, NULL);
}

VOID CreateBBSTrafficReport()
{
	struct UserInfo * User;
	int i;
	char Line[200];
	int len;
	char File[MAX_PATH];
	FILE * hFile;
	HKEY hKey=0;
	int retCode, disp;
	time_t NOW = time(NULL);

	struct tm tm;
	struct tm last;

	memcpy(&tm, gmtime(&NOW), sizeof(tm));	
	memcpy(&last, gmtime((const time_t *)&LastTrafficTime), sizeof(tm));

	sprintf(File, "%s/Traffic_%02d%02d%02d.txt", BaseDir, tm.tm_year-100, tm.tm_mon+1, tm.tm_mday);
	
	hFile = fopen(File, "wb");

	if (hFile == NULL)
	{
		Debugprintf("Failed to create traffic.txt");
		return;
	}

	len = sprintf(Line, "    Traffic Report for %s From: %04d/%02d/%02d %02d:%02dz To: %04d/%02d/%02d %02d:%02dz\r\n",
		BBSName, last.tm_year+1900, last.tm_mon+1, last.tm_mday, last.tm_hour, last.tm_min,
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min);

	fwrite(Line, 1, len, hFile);

	len = sprintf(Line, "    Call    Connects  Connects  Messages   Messages   Bytes      Bytes     Rejected  Rejected\r\n");
	fwrite(Line, 1, len, hFile);
	len = sprintf(Line, "               In        Out     Rxed        Sent     Rxed       Sent         In         Out\r\n\r\n");

	fwrite(Line, 1, len, hFile);
		
	for (i=1; i <= NumberofUsers; i++)
	{
		User = UserRecPtr[i];

		len = sprintf(Line, "%s %-7s %5d %8d %10d %10d %10d %10d %10d %10d\r\n",
			(User->flags & F_BBS)? "(B)": "   ",
			User->Call, User->nbcon, User->ConnectsOut, User->MsgsReceived, User->MsgsSent, 
			User->BytesForwardedIn, User->BytesForwardedOut,
			User->MsgsRejectedIn, User->MsgsRejectedOut);
		
		fwrite(Line, 1, len, hFile);

		User->nbcon = User->ConnectsOut = User->MsgsReceived = User->MsgsSent =  
				User->BytesForwardedIn = User->BytesForwardedOut = 
				User->MsgsRejectedIn = User->MsgsRejectedOut = 0;
	}
#ifndef LINBPQ

	retCode = RegCreateKeyEx(REGTREE,
			"SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\HouseKeeping",0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		retCode = RegSetValueEx(hKey,"LastTrafficTime",0,REG_DWORD,(BYTE *)&NOW,4);
		RegCloseKey(hKey);
	}

#endif

	fclose(hFile);
}
