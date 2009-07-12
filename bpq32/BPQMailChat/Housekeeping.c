// Mail and Chat Server for BPQ32 Packet Switch
//
//	Housekeeping Module

#include "stdafx.h"


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

VOID DoHouseKeeping()
{

	RemoveKilledMessages();
	ExpireMessages();
	ExpireBIDs();

	if (LatestMsg > MaxMsgno)
		Renumber_Messages();

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

	NewMsgHddrPtr = zalloc((NumberofMessages+1) * 4);
	NewMsgHddrPtr[0] = MsgHddrPtr[0];		// Copy Control Record

	i = 0;

	for (n = 1; n <= NumberofMessages; n++)
	{
		Msg = MsgHddrPtr[n];

		if (Msg->status == 'K')
		{
			wsprintf(MsgFile, "%s\\m_%06d.mes", MailDir, Msg->number);
			DeleteFile(MsgFile);
			Removed++;
		}
		else
			NewMsgHddrPtr[++i] = Msg;
	}

	NumberofMessages = i;
	NewMsgHddrPtr[0]->number = i;

	free(MsgHddrPtr);

	MsgHddrPtr = NewMsgHddrPtr;

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

		if ((now - BID->timestamp) < BidLifetime)
			NewBIDRecPtr[++i] = BID;
	}

	BIDSRemoved = NumberofBIDs - i;

	NumberofBIDs = i;
	NewBIDRecPtr[0]->msgno = i;

	free(BIDRecPtr);

	BIDRecPtr = NewBIDRecPtr;

	SaveBIDDatabase();

	return TRUE;

}
