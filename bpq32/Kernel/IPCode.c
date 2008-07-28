
// Module to provide a basic Gateway between IP over AX.25 and the Internet.

// Uses BPQNET Virtual Network Interface

// Basically operates as a mac level bridge, with headers converted between ax.25 and Ethernet.
// ARP frames are also reformatted, and monitored to build a simple routing table.
// Apps may not use ARP (MSYS is configured with an ax.25 default route, rather than an IP one),
// so the default route must be configured. 

// Intended as a gateway for legacy apps, rather than a full function ip over ax.25 router.
// Suggested config is the BPQNet adapter bridged to the Internet Ethernet Adapter, behind a NAT/PAT Router.
// The ax.25 applications will appear as single addresses on the Ethernet LAN

// First Version, July 2008

/*
TODO	Read Config
		Retry Default Gateway ARP
		? Time Out ARP
		?Multiple Adapters
*/

#define _CRT_SECURE_NO_DEPRECATE 

#pragma data_seg("_BPQDATA")

#define Dll	__declspec( dllexport )


#include "windows.h"

#include <stdio.h>

#include "AsmStrucs.h"

#include "IPCode.h"

//       ARP REQUEST (AX.25)

AXARP AXARPREQMSG = {0};

//		ARP REQUEST/REPLY (Eth)

ETHARP ETHARPREQMSG = {0};

ARPDATA ** ARPRecords = NULL;				// ARP Table - malloc'ed as needed

int NumberofARPEntries=0;

extern BOOL IPActive;

HANDLE hBPQNET = INVALID_HANDLE_VALUE;


ULONG OurIPAddr = 0;
ULONG OurIPBroadcast = 0;

int FramesForwarded = 0;
int FramesDropped = 0;
int ARPTimeouts = 0;

// Following two fields used by stats to get round shared memmory problem

ARPDATA Arp={0};

int ARPFlag = -1;

// Following Buffer is used for msgs from BPQDEV Driver. Put the Enet message part way down the buffer, 
//	so there is room for ax.25 header instead of Enet header when we route the frame to ax.25
//	Enet Header ia 14 bytes, AX.25 UI is 16

#define EthOffset 10				// Should be plenty

UCHAR Buffer[1600] = {0};
DWORD IPLen = 0;
UCHAR Mac[12] = {0};						// Permanant address, Current Address


UCHAR QST[7]={'Q'+'Q','S'+'S','T'+'T',0x40,0x40,0x40,0xe0};		//QST IN AX25
UCHAR ourMACAddr[6] = {02,'B','P','Q',0,1};

ULONG DefaultIPAddr = 0;

int IPPortMask = 0;

IPSTATS IPStats = {0};

BOOL Init_IP()
{
	ARPDATA * ARPptr;

	ReadConfigFile("IPGateway.cfg");
	
	ARPRecords = NULL;				// ARP Table - malloc'ed as needed

	NumberofARPEntries=0;

    //
    // Open handle to the control device, if it's not already opened. Please
    // note that even a non-admin user can open handle to the device with 
    // FILE_READ_ATTRIBUTES | SYNCHRONIZE DesiredAccess and send IOCTLs if the 
    // IOCTL is defined with FILE_ANY_ACCESS. So for better security avoid 
    // specifying FILE_ANY_ACCESS in your IOCTL defintions. 
    // If you open the device with GENERIC_READ, you can send IOCTL with 
    // FILE_READ_DATA access rights. If you open the device with GENERIC_WRITE, 
    // you can sedn ioctl with FILE_WRITE_DATA access rights.
    // 
 
	hBPQNET = CreateFile ( 
            TEXT("\\\\.\\BPQ32NET"),
            GENERIC_READ | GENERIC_WRITE,//FILE_READ_ATTRIBUTES | SYNCHRONIZE,
            FILE_SHARE_READ,
            NULL, // no SECURITY_ATTRIBUTES structure
            OPEN_EXISTING, // No special create flags
            FILE_ATTRIBUTE_NORMAL, // No special attributes
            NULL);

    if (INVALID_HANDLE_VALUE == hBPQNET)
	{
		WritetoConsoleLocal("Failed to open BPQNET device - IP Support Disabled\n");
		IPActive=FALSE;
		return FALSE;
	} 


	DeviceIoControl (hBPQNET, IOCTL_NETVMINI_GETMACADDR,NULL, 0, &Mac, 12, &IPLen, NULL);

	// Clear old packets

	do
		DeviceIoControl (hBPQNET,
          IOCTL_NETVMINI_READ_DATA,
          NULL, 0,
          &Buffer[EthOffset], 1600,				// Leave plenty of space on front
          &IPLen, NULL) ;

	while (IPLen > 0);


	IPHOSTVECTOR.HOSTAPPLFLAGS = 0x80;

	// Set up static fields in ARP messages

	AXARPREQMSG.HWTYPE=0x0300;				//	AX25
	memcpy(AXARPREQMSG.MSGHDDR.DEST, QST, 7);
	memcpy(AXARPREQMSG.MSGHDDR.ORIGIN, MYCALL, 7);
	AXARPREQMSG.MSGHDDR.ORIGIN[6] |= 1;			// Set End of Call
	AXARPREQMSG.MSGHDDR.PID = 0xcd;			// ARP
	AXARPREQMSG.MSGHDDR.CTL = 03;				// UI

	AXARPREQMSG.PID=0xcc00;					// TYPE
	AXARPREQMSG.HWTYPE=0x0300;	
	AXARPREQMSG.HWADDRLEN = 7;
	AXARPREQMSG.IPADDRLEN = 4;

	memcpy(AXARPREQMSG.SENDHWADDR, MYCALL, 7);
	AXARPREQMSG.SENDIPADDR = OurIPAddr;

	memset(ETHARPREQMSG.MSGHDDR.DEST, 255, 6);
	memcpy(ETHARPREQMSG.MSGHDDR.SOURCE, ourMACAddr, 6);
	ETHARPREQMSG.MSGHDDR.ETYPE = 0x0608;			// ARP

	ETHARPREQMSG.HWTYPE=0x0100;				//	Eth
	ETHARPREQMSG.PID=0x0008;	
	ETHARPREQMSG.HWADDRLEN = 6;
	ETHARPREQMSG.IPADDRLEN = 4;

	// Allocate ARP Entry for Default Gateway, and send ARP for it

	ARPptr = AllocARPEntry();

	if (ARPptr != NULL)
	{
		ARPptr->ARPINTERFACE = 255;
		ARPptr->ARPTYPE = 'E';
		ARPptr->IPADDR = DefaultIPAddr;

		SendARPMsg(ARPptr);
	}
	
	WritetoConsoleLocal("IP Support Enabled\n");

	IPActive=TRUE;

	return TRUE;
}

BOOL Poll_IP()
{
	// if ARPFlag set, copy requested ARP record

	if (ARPFlag != -1)
	{
		memcpy(&Arp, ARPRecords[ARPFlag], sizeof (ARPDATA));
		ARPFlag = -1;
	}

Pollloop:

	DeviceIoControl (hBPQNET,
          IOCTL_NETVMINI_READ_DATA,
          NULL, 0,
          &Buffer[EthOffset], 1600,				// Leave plenty of space on front
          &IPLen, NULL) ;

	if (IPLen > 0)
	{
		PETHMSG ethptr = (PETHMSG)&Buffer[EthOffset];

		if (ethptr->ETYPE == 0x0008)
		{
			PIPMSG ipptr = (PIPMSG)&Buffer[EthOffset+14];
			ProcessIPMsg(ipptr);
			goto Pollloop;
		}

		if (ethptr->ETYPE == 0x0608)
		{
			ProcessEthARPMsg((PETHARP)ethptr);
			goto Pollloop;
		}

		goto Pollloop;

		// Ignore anything else
	}

	if (IPHOSTVECTOR.HOSTTRACEQ != 0)
	{
		PMESSAGE axmsg;
		PMESSAGE savemsg;

		axmsg = IPHOSTVECTOR.HOSTTRACEQ;

		IPHOSTVECTOR.HOSTTRACEQ = axmsg->CHAIN;

		savemsg=axmsg;

		// Packet from AX.25

		if (axmsg->PID == 0xcc)
		{
			// IP Message

				PIPMSG ipptr = (PIPMSG)++axmsg;

				ProcessIPMsg(ipptr);
		}
		if (axmsg->PID == 0xcd)
		{
			// ARP Message

				ProcessAXARPMsg((PAXARP)axmsg);

		}
		// Release the buffer


		savemsg->CHAIN = FREE_Q;
		FREE_Q = savemsg;

		QCOUNT++;

		return TRUE;

	}
	return TRUE;
}
  
BOOL Send_ETH(VOID * Block, DWORD len)
{
    DWORD bytes;
	
	DeviceIoControl(hBPQNET,
		IOCTL_NETVMINI_WRITE_DATA,
		Block, len,
        NULL, 0,
        &bytes, NULL);
    
    return TRUE;
}
VOID Send_AX(VOID * Block, DWORD Len, UCHAR Port)
{
	//	Can't use API SENDRAW, as that tries to get the semaphore, which we already have

	// Block includes the Msg Header (7 bytes), Len Does not!

	_asm {

	pushfd
	cld
	pushad

	mov	al,Port
	mov	ah,10
	mov	ecx,Len
	mov	esi,Block
	add	esi,7

	call	RAWTX

	popad
	popfd
	}

	return;
}


VOID ProcessEthARPMsg(PETHARP arpptr)
{
	int i=0, Mask=IPPortMask;
	PARPDATA Arp;
	BOOL Found;

	switch (arpptr->ARPOPCODE)
	{
	case 0x0100:
	
		// Add to our table, as we will almost certainly want to send back to it

		Arp = LookupARP(arpptr->SENDIPADDR, TRUE, &Found);

		if (Found)
				goto AlreadyThere;				// Already there
				
		if (Arp != NULL)
		{
			Arp->IPADDR = arpptr->SENDIPADDR;


			Arp->ARPTYPE = 'E';
			Arp->ARPINTERFACE = 255;
		}

AlreadyThere:

		memcpy(Arp->HWADDR, arpptr->SENDHWADDR ,6);
		Arp->ARPVALID = TRUE;
		Arp->ARPTIMER =  86400;


		if (arpptr->TARGETIPADDR == OurIPAddr)
		{
			arpptr->ARPOPCODE = 0x0200;
			memcpy(arpptr->TARGETHWADDR, arpptr->SENDHWADDR ,6);
			memcpy(arpptr->SENDHWADDR, ourMACAddr ,6);

			arpptr->TARGETIPADDR = arpptr->SENDIPADDR;
			arpptr->SENDIPADDR = OurIPAddr;

			memcpy(arpptr->MSGHDDR.DEST, arpptr->MSGHDDR.SOURCE ,6); 
			memcpy(arpptr->MSGHDDR.SOURCE, ourMACAddr ,6); 

			Send_ETH(arpptr,42);

			return;

		}

		// Send to all other ports enabled for IP, reformatting as necessary

		memcpy(AXARPREQMSG.MSGHDDR.DEST, QST, 7);
		memcpy(AXARPREQMSG.MSGHDDR.ORIGIN, MYCALL, 7);
		AXARPREQMSG.MSGHDDR.ORIGIN[6] |= 1;			// Set End of Call

		AXARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
		AXARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;
		memset(AXARPREQMSG.TARGETHWADDR, 0, 7);
		AXARPREQMSG.ARPOPCODE = 0x0100;

		for (i=1; i<=NUMBEROFPORTS; i++)
		{
			if (Mask & 1)
				Send_AX(&AXARPREQMSG, 46, i);

			Mask>>=1;
		}

		break;

	
	case 0x0200:

		// Update ARP Cache

		Arp = LookupARP(arpptr->SENDIPADDR, TRUE, &Found);

		if (Found)
			goto Update;

		if (Arp == NULL)
			goto SendBack;

Update:
		Arp->IPADDR = arpptr->SENDIPADDR;

		memcpy(Arp->HWADDR, arpptr->SENDHWADDR ,6);
		Arp->ARPTYPE = 'E';
		Arp->ARPINTERFACE = 255;
		Arp->ARPVALID = TRUE;
		Arp->ARPTIMER =  86400;


SendBack:

			//  Send Back to Originator of ARP Request

		Arp = LookupARP(arpptr->TARGETIPADDR, FALSE, &Found);
				
		if (Found)
		{
			if (Arp->ARPINTERFACE == 255)
			{
				ETHARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
				ETHARPREQMSG.ARPOPCODE = 0x0200;		//             ; REQUEST
				ETHARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;
					
				memcpy(ETHARPREQMSG.SENDHWADDR,ourMACAddr, 6);
				memcpy(ETHARPREQMSG.MSGHDDR.DEST, Arp->HWADDR, 6);

				Send_ETH(&ETHARPREQMSG, 42);
				return;
			}
			else
			{
				AXARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
				AXARPREQMSG.ARPOPCODE = 0x0200;		//             ; REQUEST
				AXARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;

				memcpy(AXARPREQMSG.SENDHWADDR, MYCALL, 7);
				memcpy(AXARPREQMSG.TARGETHWADDR, Arp->HWADDR, 7);
					
				memcpy(AXARPREQMSG.MSGHDDR.DEST, Arp->HWADDR, 7);

				Send_AX(&AXARPREQMSG, 46, Arp->ARPINTERFACE);
				return;
			}
		}
		break;

	default:
		break;
	}
	return;
}

VOID ProcessAXARPMsg(PAXARP arpptr)
{
	int i=0, Mask=IPPortMask;
	PARPDATA Arp;
	BOOL Found;

	switch (arpptr->ARPOPCODE)
	{
	case 0x0100:
	{
		// Add to our table, as we will almost certainly want to send back to it

		Arp = LookupARP(arpptr->SENDIPADDR, TRUE, &Found);

		if (Found)
			goto AlreadyThere;				// Already there
				
		if (Arp != NULL)
		{
			//       ENTRY NOT FOUND - IF ANY SPARE ENTRIES, USE ONE

			Arp->IPADDR = arpptr->SENDIPADDR;

			memcpy(Arp->HWADDR, arpptr->SENDHWADDR ,7);

			Arp->ARPTYPE = 'D';
			Arp->ARPINTERFACE = arpptr->MSGHDDR.PORT;
			Arp->ARPVALID = TRUE;
			Arp->ARPTIMER =  86400;

		}

AlreadyThere:

		// Send to all other ports enabled for IP, reformatting as necessary
	
		memcpy(AXARPREQMSG.MSGHDDR.DEST, QST, 7);
		memcpy(AXARPREQMSG.MSGHDDR.ORIGIN, MYCALL, 7);
		AXARPREQMSG.MSGHDDR.ORIGIN[6] |= 1;			// Set End of Call

		AXARPREQMSG.ARPOPCODE = 0x0100;
		AXARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
		AXARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;

		for (i=1; i<=NUMBEROFPORTS; i++)
		{
			if (i != arpptr->MSGHDDR.PORT)
				if (Mask & 1)
				Send_AX(&AXARPREQMSG, 46, i);

			Mask>>=1;
		}

		ETHARPREQMSG.ARPOPCODE = 0x0100;
		ETHARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
		ETHARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;
		memcpy(ETHARPREQMSG.SENDHWADDR, ourMACAddr, 6);

		Send_ETH(&ETHARPREQMSG, 42);

		break;
	}

	case 0x0200:

		// Update ARP Cache

		
		Arp = LookupARP(arpptr->SENDIPADDR, TRUE, &Found);
		
		if (Found)
			goto Update;

		if (Arp == NULL)
			goto SendBack;

Update:
		Arp->IPADDR = arpptr->SENDIPADDR;

		memcpy(Arp->HWADDR, arpptr->SENDHWADDR ,7);
		Arp->ARPTYPE = 'D';
		Arp->ARPINTERFACE = arpptr->MSGHDDR.PORT;
		Arp->ARPVALID = TRUE;
		Arp->ARPTIMER =  86400;

SendBack:

			//  Send Back to Originator of ARP Request

		Arp = LookupARP(arpptr->TARGETIPADDR, FALSE, &Found);

		if (Found)
		{
			if (Arp->ARPINTERFACE == 255)
			{
				ETHARPREQMSG.ARPOPCODE = 0x0200;		//             ; REQUEST

				ETHARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
				ETHARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;

				memcpy(ETHARPREQMSG.SENDHWADDR,ourMACAddr,6);
				memcpy(ETHARPREQMSG.TARGETHWADDR, Arp->HWADDR, 6);
				memcpy(ETHARPREQMSG.MSGHDDR.DEST, Arp->HWADDR, 6);

				Send_ETH(&ETHARPREQMSG, 42);
				return;
			}
				else
			{
				AXARPREQMSG.ARPOPCODE = 0x0200;		//             ; Reply

				AXARPREQMSG.TARGETIPADDR = arpptr->TARGETIPADDR;
				AXARPREQMSG.SENDIPADDR = arpptr->SENDIPADDR;

				memcpy(AXARPREQMSG.SENDHWADDR, MYCALL, 7);
				memcpy(AXARPREQMSG.TARGETHWADDR, Arp->HWADDR, 7);

				memcpy(AXARPREQMSG.MSGHDDR.DEST, Arp->HWADDR, 7);

				Send_AX(&AXARPREQMSG, 46, Arp->ARPINTERFACE);
				return;
			}
		}
		
		break;

	
	default:

		return;
	}
}

	

VOID ProcessIPMsg(PIPMSG IPptr)
{
	ULONG Dest;

	if (IPptr->VERLEN != 0x45) return; //Only support Type = 4, Len = 20

	if (!CheckIPChecksum(IPptr)) return;

	// See if for us - if not pass to router

	Dest = IPptr->IPDEST;

	if (Dest == OurIPAddr || Dest == 0xffffffff || Dest == OurIPBroadcast)
		goto ForUs;


	RouteIPMsg(IPptr);

	return;

ForUs:

	if (IPptr->IPPROTOCOL == 1)		// ICMP
	{
		ProcessICMPMsg(IPptr);
		return;
	}

}

BOOL CheckIPChecksum(PIPMSG IPptr)
{
	USHORT checksum=0;

	if (IPptr->IPCHECKSUM == 0)
		return TRUE; //Not used

	_asm{

	mov ESI, IPptr
	MOV ECX,10
	XOR     DX,DX           ; CLEAR CHECKSUM AND CARRY
CSUMLOOP:
	LODSW
	ADC     DX,AX

	LOOP    CSUMLOOP

	ADC     DX,0            ; MAY BE CARRY FLOATING AROUND

	mov	checksum,dx

	}

	if (checksum == 0xffff) return TRUE; else return FALSE;

}
BOOL Check_Checksum(VOID * ptr1, int Len)
{
	USHORT checksum;

	_asm{

	mov ESI, ptr1
	MOV ECX, Len
	inc	ECX
	SHR	ECX,1				; Round up and convert to words

	XOR     DX,DX           ; CLEAR CHECKSUM AND CARRY
CSUMLOOP:
	LODSW
	ADC     DX,AX

	LOOP    CSUMLOOP

	ADC     DX,0            ; MAY BE CARRY FLOATING AROUND

	mov	checksum,dx

	}

	if (checksum == 0xffff) return TRUE; else return FALSE;

}
USHORT Generate_CHECKSUM(VOID * ptr1, int Len)
{
	USHORT checksum=0;

	_asm{

	mov ESI, ptr1
	MOV ECX, Len
	inc	ECX
	SHR	ECX,1				; Round up and convert to words
	XOR     DX,DX           ; CLEAR CHECKSUM AND CARRY
CSUMLOOP:
	LODSW
	ADC     DX,AX

	LOOP    CSUMLOOP

	ADC     DX,0            ; MAY BE CARRY FLOATING AROUND

	NOT		DX

	mov	checksum,dx

	}

	return checksum ;

}

VOID ProcessICMPMsg(PIPMSG IPptr)
{
	int Len;
	PICMPMSG ICMPptr = (PICMPMSG)&IPptr->Data;

	Len = ntohs(IPptr->IPLENGTH);

	Len-=20;


	Check_Checksum(ICMPptr, Len);

	if (ICMPptr->ICMPTYPE == 8)
	{
		//	ICMP_ECHO

		ICMPptr->ICMPTYPE = 0;		// Convert to Reply

		ICMPptr->ICMPCHECKSUM = 0;

		// CHECKSUM IT

		ICMPptr->ICMPCHECKSUM = Generate_CHECKSUM(ICMPptr, Len);

		// Swap Dest to Origin

		IPptr->IPDEST = IPptr->IPSOURCE;

		IPptr->IPSOURCE = OurIPAddr;

		RouteIPMsg(IPptr);			// Send Back

	}

	if (ICMPptr->ICMPTYPE == 8)
	{
		//	ICMP_REPLY:
	}

	// Ingonre others
}

VOID RouteIPMsg(PIPMSG IPptr)
{
	PARPDATA Arp;
	BOOL Found;

	// We rely on the ARP messages generated by eiter end to route frames.
	//	If address is not in ARP cache (say call originated by MSYS), send to our default route

	// Look up ARP

	Arp = LookupARP(IPptr->IPDEST, FALSE, &Found);

	if (!Found)
		Arp = LookupARP(DefaultIPAddr, FALSE, &Found);

	if (!Found)
		return;				// No route or default
		
	if (Arp->ARPVALID)
	{
		if (Arp->ARPTYPE == 'E')
			SendIPtoBPQDEV(IPptr, Arp->HWADDR);
		else
			SendIPtoAX25(IPptr, Arp->HWADDR, Arp->ARPINTERFACE);
	}
	
	return;	

}

VOID SendIPtoBPQDEV(PIPMSG IPptr, UCHAR * HWADDR)
{	
	// AX.25 headers are bigger, so there will always be room in buffer for enet header
	
	PETHMSG Ethptr = (PETHMSG)IPptr;
	int Len;

	(UCHAR *)Ethptr--;

	Len = ntohs(IPptr->IPLENGTH);

	Len+=14;			// Add eth Header

	memcpy(Ethptr->DEST, HWADDR, 6);
	memcpy(Ethptr->SOURCE, ourMACAddr, 6);
	Ethptr->ETYPE= 0x0008;

	Send_ETH(Ethptr,Len);

	return;
}

VOID SendIPtoAX25(PIPMSG IPptr, UCHAR * HWADDR, int Port)
{
	PMESSAGE Msgptr = (PMESSAGE)IPptr;
	int Len;

	(UCHAR *)Msgptr--;

	Len = ntohs(IPptr->IPLENGTH);

	Len+=16;

	memcpy(Msgptr->DEST, HWADDR, 7);
	memcpy(Msgptr->ORIGIN, MYCALL, 7);
	Msgptr->ORIGIN[6] |= 1;						// Set End of Call
	Msgptr->CTL = 3;		//UI
	Msgptr->PID = 0xcc;		//IP

	Send_AX(Msgptr, Len, Port);

	return;
}

PARPDATA AllocARPEntry()
{
	ARPDATA * ARPptr;

	if (NumberofARPEntries == 0)

		ARPRecords=malloc(4);
	else
		ARPRecords=realloc(ARPRecords,(NumberofARPEntries+1)*4);

	ARPptr=malloc(sizeof(ARPDATA));

	if (ARPptr == NULL) return NULL;

	memset(ARPptr, 0, sizeof(ARPDATA));
	
	ARPRecords[NumberofARPEntries++]=ARPptr;
 
	return ARPptr;
}

 VOID SendARPMsg(PARPDATA Arp)
 {
	//	Send ARP. Initially used only to find default gateway

	Arp->ARPTIMER =  5;							// Retry periodically

	if (Arp->ARPINTERFACE == 255)				
	{
		ETHARPREQMSG.ARPOPCODE = 0x0100;		//             ; REQUEST

		ETHARPREQMSG.TARGETIPADDR = Arp->IPADDR;						
		memset(ETHARPREQMSG.TARGETHWADDR, 0, 6);

		ETHARPREQMSG.SENDIPADDR = OurIPAddr;
		memcpy(ETHARPREQMSG.SENDHWADDR,ourMACAddr, 6);

		memcpy(ETHARPREQMSG.MSGHDDR.SOURCE, ourMACAddr, 6);
		memset(ETHARPREQMSG.MSGHDDR.DEST, 255, 6);

		Send_ETH(&ETHARPREQMSG, 42);

		return;
	}
	else
	{
		AXARPREQMSG.TARGETIPADDR = Arp->IPADDR;
		memset(AXARPREQMSG.TARGETHWADDR, 0, 7);
		AXARPREQMSG.ARPOPCODE = 0x0100;		//             ; REQUEST
		AXARPREQMSG.SENDIPADDR = OurIPAddr;

		memcpy(AXARPREQMSG.SENDHWADDR, MYCALL, 7);
		memcpy(AXARPREQMSG.TARGETHWADDR, Arp->HWADDR, 7);
		memcpy(AXARPREQMSG.MSGHDDR.DEST, QST, 7);
		memcpy(AXARPREQMSG.MSGHDDR.ORIGIN, MYCALL, 7);
		AXARPREQMSG.MSGHDDR.ORIGIN[6] |= 1;			// Set End of Call

		Send_AX(&AXARPREQMSG, 46, Arp->ARPINTERFACE);

		return;

	}
 }
PARPDATA LookupARP(ULONG IPADDR, BOOL Add, BOOL * Found)
{
	PARPDATA Arp = NULL;
	int i;

	for (i=0; i < NumberofARPEntries; i++)
	{
		Arp = ARPRecords[i];

		if (Arp->IPADDR == IPADDR)
		{
			*Found = TRUE;
			return Arp;
		}
	}

	// Not Found

	*Found = FALSE;

	if (Add)
	{
		Arp = AllocARPEntry();
		return Arp;
	}
	else
		return NULL;
}
	
Dll int APIENTRY GetIPInfo(VOID * ARPRecs, VOID * IPStatsParam, int index)
{
	
	IPStats.ARPEntries = NumberofARPEntries;
	
	ARPFlag = index;

	_asm {
		
		mov esi, ARPRecs
		mov DWORD PTR[ESI], offset Arp
	
		mov esi, IPStatsParam
		mov DWORD PTR[ESI], offset IPStats 
	}

	return ARPFlag;
}


BOOL ReadConfigFile(char * fn)
{

// IPAddr 192.168.0.129
// IPBroadcast 192.168.0.255
// IPGateway 192.168.0.1
// IPPorts 1,4 

	FILE *file;
	char buf[256],errbuf[256];

	UCHAR Value[MAX_PATH];

	if (BPQDirectory[0] == 0)
	{
		strcpy(Value,fn);
	}
	else
	{
		strcpy(Value,BPQDirectory);
		strcat(Value,"\\");
		strcat(Value,fn);
	}
		
	
	if ((file = fopen(Value,"r")) == NULL)
	{
		wsprintf(buf,"Config file %s could not be opened ",Value);
		WritetoConsoleLocal(buf);

		return (FALSE);
	}


	while(fgets(buf, 255, file) != NULL)
	{
		strcpy(errbuf,buf);			// save in case of error
	
		if (!ProcessLine(buf))
		{
			WritetoConsoleLocal("IP Gateway bad config record ");
			WritetoConsoleLocal(errbuf);
		}
				
	}
	
	fclose(file);

	return (TRUE);

}


ProcessLine(char * buf)
{
	PCHAR ptr, p_value, p_port;
	int i;

	ptr = strtok(buf, " \t\n\r");
	p_value = strtok(NULL, " \t\n\r");


	if(ptr == NULL) return (TRUE);

	if(*ptr =='#') return (TRUE);			// comment

	if(*ptr ==';') return (TRUE);			// comment

	if (_stricmp(ptr,"IPAddr") == 0)
	{
		OurIPAddr = inet_addr("192.168.0.129");

		if (OurIPAddr == INADDR_NONE) return (FALSE);

		return (TRUE);
	}

	if (_stricmp(ptr,"IPBroadcast") == 0)
	{
		OurIPBroadcast = inet_addr("192.168.0.129");

		if (OurIPBroadcast == INADDR_NONE) return (FALSE);

		return (TRUE);
	}

	if (_stricmp(ptr,"IPGateway") == 0)
	{
		DefaultIPAddr = inet_addr("192.168.0.129");

		if (DefaultIPAddr == INADDR_NONE) return (FALSE);

		return (TRUE);
	}

	if (_stricmp(ptr,"IPPorts") == 0)
	{
		p_port = strtok(p_value, " ,\t\n\r");
		
		while (p_port != NULL)
		{
			i=atoi(p_port);
			if (i == 0) return FALSE;

			IPPortMask |= 1 << (i-1);
			p_port = strtok(NULL, " ,\t\n\r");
		}
		return (TRUE);
	}
	
	//
	//	Bad line
	//
	return (FALSE);
	
}
	


