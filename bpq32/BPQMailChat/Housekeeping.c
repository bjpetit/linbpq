// Mail and Chat Server for BPQ32 Packet Switch
//
//	Housekeeping Module

#include "stdafx.h"

int LogAge = 7;

BOOL DeletetoRecycleBin = FALSE;

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

int LastFWDTime;

DeletetoRecycle(char * FN)
{
	SHFILEOPSTRUCT FileOp;

	FileOp.hwnd = NULL;
	FileOp.wFunc = FO_DELETE;
	FileOp.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR | FOF_ALLOWUNDO;
	FileOp.pFrom = FN;
	FileOp.pTo = NULL;

	return SHFileOperation(&FileOp);
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
int Removed;
int Killed;
int BIDSRemoved;


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

VOID DoHouseKeeping(BOOL Manual)
{
	HKEY hKey=0;
	int retCode, disp;
	time_t NOW;

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

	MailHousekeepingResults();
	
	NOW = time(NULL);

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
          "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\HouseKeeping",0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{
		retCode = RegSetValueEx(hKey,"LastHouseKeepingTime",0,REG_DWORD,(BYTE *)&NOW,4);
		RegCloseKey(hKey);
	}

	if (Manual)
		DialogBox(hInst, MAKEINTRESOURCE(IDD_MAINTRESULTS), hWnd, HKDialogProc);
	
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

			switch (Msg->status)
			{
			case 'N':

				// Is it unforwarded or unread?

				if (memcmp(Msg->fbbs, zeros, NBMASK) == 0)
				{
					if (Msg->datecreated < PURLimit) KillMsg(Msg);
				}
				else
				{
					if (Msg->datecreated < PNFLimit) KillMsg(Msg);
				}
				continue;	
	
			case 'F':

				if (Msg->datechanged < PFLimit) KillMsg(Msg);

				continue;	

			case 'Y':

				if (Msg->datechanged < PRLimit) KillMsg(Msg);

				continue;

			default:

				continue;

			}			
			
		case 'B':

			BLimit = BF;

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

			switch (Msg->status)
			{
			case '$':
			case 'N':
			case 'F':

				if (Msg->datecreated < BFLimit) KillMsg(Msg);
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


	NewMsgHddrPtr = zalloc((NumberofMessages+1) * 4);
	NewMsgHddrPtr[0] = MsgHddrPtr[0];		// Copy Control Record

	i = 0;

	for (n = 1; n <= NumberofMessages; n++)
	{
		Msg = MsgHddrPtr[n];

		if (Msg->status == 'K')
		{
			sprintf_s(MsgFile, sizeof(MsgFile), "%s\\m_%06d.mes%c", MailDir, Msg->number, 0);
			if (DeletetoRecycleBin)
				DeletetoRecycle(MsgFile);
			else
				DeleteFile(MsgFile);
			
			free(Msg);
			Removed++;
		}
		else
			NewMsgHddrPtr[++i] = Msg;
	}

	NumberofMessages = i;
	NewMsgHddrPtr[0]->number = i;

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

	i = 0;		// New Message Number

	for (n = 1; n <= NumberofMessages; n++)
	{
		Msg = MsgHddrPtr[n];

		NewNumber[Msg->number] = ++i;		// Save so we can update users' last listed count

		// New will always be >= old unless somethnig hasgon horribly wrong,
		// so can rename in place without risk of losing a message

		if (Msg->number < i)
		{
			MessageBox(MainWnd, "Invalid message number detected, quitting", "BPQMailChat", MB_OK);
			SaveMessageDatabase();
			return;
		}

		if (Msg->number != i)
		{
			wsprintf(OldMsgFile, "%s\\m_%06d.mes", MailDir, Msg->number);
			wsprintf(NewMsgFile, "%s\\m_%06d.mes", MailDir, i);
			result = rename(OldMsgFile, NewMsgFile);
			if (result)
			{
				char Errmsg[100];
				wsprintf(Errmsg, "Could not rename message no %d to %d, quitting", Msg->number, i);
				MessageBox(MainWnd,Errmsg , "BPQMailChat", MB_OK);
				SaveMessageDatabase();
				return;
			}
			Msg->number = i;
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

	Length += wsprintf(&MailBuffer[Length], "Killed Messsages Removed %d\r\n", Removed);
	Length += wsprintf(&MailBuffer[Length], "Messages Killed          %d\r\n", Killed);
	Length += wsprintf(&MailBuffer[Length], "Live Messages            %d\r\n", NumberofMessages - Killed);
	Length += wsprintf(&MailBuffer[Length], "Total Messages           %d\r\n", NumberofMessages);
	Length += wsprintf(&MailBuffer[Length], "BIDs Removed             %d\r\n", BIDSRemoved);
	Length += wsprintf(&MailBuffer[Length], "BIDs Left                %d\r\n", NumberofBIDs);

	SendMessageToSYSOP("Housekeeping Results", MailBuffer, Length);
}


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
			 wsprintf(File, "%s\\%s%c", BaseDir, ffd.cFileName, 0);
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
