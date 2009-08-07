// Mail and Chat Server for BPQ32 Packet Switch
//
// UI Handling Routines

#include "stdafx.h"

char UIDEST[10] = "FBB";
char AXDEST[7];
char MYCALL[7];

#pragma pack(1)

UINT UIPortMask = 0;

typedef struct _MESSAGE
{
//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

	struct _MESSAGE * CHAIN;

	UCHAR	PORT;
	USHORT	LENGTH;

	UCHAR	DEST[7];
	UCHAR	ORIGIN[7];

//	 MAY BE UP TO 56 BYTES OF DIGIS

	UCHAR	CTL;
	UCHAR	PID;
	UCHAR	DATA[256];

}MESSAGE, *PMESSAGE;

#pragma pack()


VOID SetupUIInterface()
{
	char * ptr;
	int i, NumPorts = GetNumberofPorts();
	char Tempstring[101];
	struct _EXCEPTION_POINTERS exinfo;

	ConvToAX25(GetApplCall(BBSApplNum), MYCALL);
	MYCALL[6];				// Set End of Call (only used as origin)

	ConvToAX25(UIDEST, AXDEST);

	UIPortMask = 0;

	memcpy(Tempstring, UIPortString, 100);

	ptr = strtok(Tempstring, " ,\t\n\r");
		
	while (ptr != NULL)
	{
		i=atoi(ptr);

		if (i > 0 && i <= NumPorts)
			UIPortMask |= 1 << (i-1);

		ptr = strtok(NULL, " ,\t\n\r");
	}

	__try
	{
		SendLatestUI();
	}
	My__except_Routine("SendLatestUI");

}

struct MsgInfo * FindMessageByNumber(int msgno)
 {
	int m=NumberofMessages;

	struct MsgInfo * Msg;

	do
	{
		Msg=MsgHddrPtr[m];

		if (Msg->number == msgno)
			return Msg;

		if (Msg->number < msgno)
			return NULL;			// Not found

		m--;

	} while (m > 0);

	return NULL;
}

VOID SendMsgUI(struct MsgInfo * Msg)
{
	char msg[100];
	int len, i;
	int Mask = UIPortMask;
	int NumPorts = GetNumberofPorts();

	//12345 B 2053 TEST@ALL F6FBB 920325 This is the subject

	struct tm *tm = gmtime(&Msg->datecreated);	
	
	len = wsprintf(msg,"%-6d %c %6d %-13s %-6s %02d%02d%02d %s\r",
		Msg->number, Msg->type, Msg->length, Msg->to,
		Msg->from, tm->tm_year-100, tm->tm_mon+1, tm->tm_mday, Msg->title);

	for (i=1; i <= NumPorts; i++)
	{
		if (Mask & 1)
			Send_AX_Datagram(msg, len, i, AXDEST);
		
		Mask>>=1;
	}
}

VOID SendDummyUI(int num)
{
	char msg[100];
	int len, i;
	int Mask = UIPortMask;
	int NumPorts = GetNumberofPorts()
;
	len = wsprintf(msg,"%-6d #\r", num);

	for (i=1; i <= NumPorts; i++)
	{
		if (Mask & 1)
			Send_AX_Datagram(msg, len, i, AXDEST);
		
		Mask>>=1;
	}
}

VOID SendLatestUI()
{
	char msg[20];
	int len, i;
	int Mask = UIPortMask;
	int NumPorts = GetNumberofPorts();
	
	len = wsprintf(msg,"%-6d !!\r", LatestMsg);

	for (i=1; i <= NumPorts; i++)
	{
		if (Mask & 1)
			Send_AX_Datagram(msg, len, i, AXDEST);
		
		Mask>>=1;
	}
}

VOID Send_AX_Datagram(UCHAR * Msg, DWORD Len, UCHAR Port, UCHAR * HWADDR)
{
	MESSAGE AXMSG;

	// Block includes the Msg Header (7 bytes), Len Does not!

	memcpy(AXMSG.DEST, HWADDR, 7);
	memcpy(AXMSG.ORIGIN, MYCALL, 7);
	AXMSG.DEST[6] &= 0x7e;			// Clear End of Call
	AXMSG.DEST[6] |= 0x80;			// set Command Bit
	AXMSG.ORIGIN[6] |= 1;			// Set End of Call
	AXMSG.CTL = 3;		//UI
	AXMSG.PID = 0xf0;
	memcpy(AXMSG.DATA, Msg, Len);

	SendRaw(Port, (char *)&AXMSG.DEST, Len + 16);

	return;

}

VOID ProcessUItoMe(char * msg, int len)
{
	msg[len] = 0;
	return;
}

VOID ProcessUItoFBB(char * msg, int len)
{
	// ? 0000006464
	// The first 8 digits are the hexadecimal number of the requested start of the list
	// (here 00002EE0 -> 12000) and the last two digits are the sum of the four bytes anded with FF (0E).

	int Number, Sum;
	char cksum[3];
	struct _EXCEPTION_POINTERS exinfo;

	
	if (msg[0] == '?')
	{
		memcpy(cksum, &msg[10], 2);
		msg[10]=0;

		sscanf(&msg[1], "%X", &Number);
		sscanf(cksum, "%X", &Sum);

		if (Number <= LatestMsg)
		{
			struct MsgInfo * Msg = FindMessageByNumber(Number);
	
			if (Msg)
			{			__try
				{
					SendMsgUI(Msg);
				}
				My__except_Routine("SendMsgUI");
			}
			else
			{
				__try
				{				
					SendDummyUI(Number);
				}
				My__except_Routine("SendDummyUI");	
			}

		}

	}
	
	return;
}


VOID SeeifBBSUIFrame(PMESSAGE buff, int len)
{
	if (buff->CTL != 3)
		return;

	if (buff->PID != 0xf0)
		return;

	if (memcmp(buff->ORIGIN, MYCALL,6) == 0)		// From me?
		if (buff->ORIGIN[6] == (MYCALL[6] | 1))		// Set End of Call
			return;

	if (memcmp(buff->DEST, MYCALL, 6) == 0)
		if ((buff->DEST[6] & 0x7e) == MYCALL[6])	// Just SSID Bits
		{
			ProcessUItoFBB(buff->DATA, len-23);
			return;
		}
	if (memcmp(buff->DEST, AXDEST, 6) == 0)
	{
		ProcessUItoFBB(buff->DATA, len-23);
		return;
	}

	len++;

	return;
}




//	ConvToAX25(GetNodeCall(), MYCALL);
//				len=ConvFromAX25(Routes->NEIGHBOUR_DIGI1,Portcall);
//			Portcall[len]=0;










/*
20:09:00R GM8BPQ-10>FBB Port=1 <UI C>:
103    !!
20:10:06R GM8BPQ-10>FBB Port=1 <UI C>:
19-Jul 21:08 <<< Mailbox GM8BPQ Skigersta >>> 2 active messages.
Messages for
 ALL

20:11:11R GM8BPQ-10>FBB Port=1 <UI C>:
104    P      5 G8BPQ         GM8BPQ 090719 ***
20:12:17R GM8BPQ-10>FBB Port=1 <UI C>:
105    B      5 ALL           GM8BPQ 090719 test

12345 B 2053 TEST@ALL F6FBB 920325 This is the subject



20:13:23R GM8BPQ-10>FBB Port=1 <UI C>:
? 0000006464

20:15:34R GM8BPQ-10>FBB Port=1 <UI C>:
105    !!
20:15:45T GM8BPQ-10>MAIL Port=2 <UI C>:

20:16:40R GM8BPQ-10>FBB Port=1 <UI C>:
19-Jul 21:15 <<< Mailbox GM8BPQ Skigersta >>> 4 active messages.
Messages for
 ALL G8BPQ
20:17:46R GM8BPQ-10>FBB Port=1 <UI C>:
106    P      5 GM8BPQ        GM8BPQ 090719 ***
20:20:54R GM8BPQ-10>FBB Port=1 <UI C>:
? 0000006464
20:21:05T GM8BPQ-10>FBB Port=2 <UI C>:
? 0000006464
*/
