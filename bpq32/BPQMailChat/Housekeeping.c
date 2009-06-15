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
					if (Msg->datecreated < PURLimit) KillMsg(Msg);
				else
					if (Msg->datecreated < PNFLimit) KillMsg(Msg);

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
	Msg->status = 'K';
	Msg->datechanged = time(NULL);
}

BOOL RemoveKilledMessages()
{
	struct MsgInfo * Msg;
	struct MsgInfo ** NewMsgHddrPtr;
	char MsgFile[MAX_PATH];

	int i, n;

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


