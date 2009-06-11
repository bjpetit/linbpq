// Mail and Chat Server for BPQ32 Packet Switch
//
//	Housekeeping Module

#include "stdafx.h"

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


