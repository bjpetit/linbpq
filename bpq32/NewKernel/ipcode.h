// Header file for BPQ32 IP over ax.25 support

#define BPQREREAD						403
#define BPQADDARP						404

//extern struct PORTCONTROL * PORTTABLE;

#define IDI_ICON2                       123

unsigned long _beginthread( void *, unsigned stack_size, void * arglist);

#define IOCTL_NETVMINI_READ_DATA \
    CTL_CODE (FILE_DEVICE_UNKNOWN, 0, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_NETVMINI_WRITE_DATA \
    CTL_CODE (FILE_DEVICE_UNKNOWN, 1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_NETVMINI_GETMACADDR \
    CTL_CODE (FILE_DEVICE_UNKNOWN, 2, METHOD_BUFFERED, FILE_READ_ACCESS)


#pragma pack(1) 

typedef struct _ETHMSG
{
	UCHAR	DEST[6];
	UCHAR	SOURCE[6];
	USHORT	ETYPE;

} ETHMSG, *PETHMSG;


typedef struct _IPMSG
{
//       FORMAT OF IP HEADER
//
//       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)

	UCHAR	VERLEN;          // 4 BITS VERSION, 4 BITS LENGTH
	UCHAR	TOS;             // TYPE OF SERVICE
	USHORT	IPLENGTH;        // DATAGRAM LENGTH
	USHORT	IPID;            // IDENTIFICATION
	USHORT	FRAGWORD;        // 3 BITS FLAGS, 13 BITS OFFSET
	UCHAR	IPTTL;
	UCHAR	IPPROTOCOL;      // HIGHER LEVEL PROTOCOL
	USHORT	IPCHECKSUM;      // HEADER CHECKSUM
	ULONG	IPSOURCE;
	ULONG	IPDEST;

	UCHAR	Data;

} IPMSG, *PIPMSG;

typedef struct _TCPMSG
{

//	FORMAT OF TCP HEADER WITHIN AN IP DATAGRAM

//	NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)

	USHORT	SOURCEPORT;
	USHORT	DESTPORT;

	ULONG	SEQNUM;
	ULONG	ACKNUM;

	UCHAR	TCPCONTROL;			// 4 BITS DATA OFFSET 4 RESERVED
	UCHAR	TCPFLAGS;			// (2 RESERVED) URG ACK PSH RST SYN FIN

	USHORT	WINDOW;
	USHORT	CHECKSUM;
	USHORT	URGPTR;


} TCPMSG, *PTCPMSG;


//		ICMP MESSAGE STRUCTURE

typedef struct _ICMPMSG
{

//       FORMAT OF ICMP HEADER WITHIN AN IP DATAGRAM

//       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)

	UCHAR	ICMPTYPE;
	UCHAR	ICMPCODE;
	USHORT	ICMPCHECKSUM;

	USHORT	ICMPID;
	USHORT	ICMPSEQUENCE;

} ICMPMSG, *PICMPMSG;


typedef struct _BUFFHEADER
{
//	BASIC LINK LEVEL HEADER BUFFER LAYOUT

	struct _MESSAGE * CHAIN;

	UCHAR	PORT;
	USHORT	LENGTH;

	UCHAR	DEST[7];
	UCHAR	ORIGIN[7];

//	 MAY BE UP TO 56 BYTES OF DIGIS

	UCHAR	CTL;
	UCHAR	PID; 

}BUFFHEADER, *PBUFFHEADER;


typedef struct _AXARP
{
	struct _BUFFHEADER MSGHDDR;
	
	USHORT	HWTYPE;      //    DB      0,3             ; AX.25
	USHORT	PID;			//	DB      0,0CCH          ; PID

	UCHAR	HWADDRLEN;       //      7
	UCHAR	IPADDRLEN;      //      4

	USHORT	ARPOPCODE;       //      200H            ; REQUEST/REPLY

	UCHAR	SENDHWADDR[7];
	ULONG	SENDIPADDR;

	UCHAR	TARGETHWADDR[7];
	ULONG	TARGETIPADDR;

} AXARP, *PAXARP;




typedef struct _ETHARP
{
	struct _ETHMSG MSGHDDR;

	USHORT	HWTYPE;      //    DB      0,1             ; Eth
	USHORT	PID;			//	DB      8,0          ; PID

	UCHAR	HWADDRLEN;       //      6
	UCHAR	IPADDRLEN;      //      4

	USHORT	ARPOPCODE;       //      200H            ; REQUEST/REPLY

	UCHAR	SENDHWADDR[6];
	ULONG	SENDIPADDR;

	UCHAR	TARGETHWADDR[6];
	ULONG	TARGETIPADDR;

} ETHARP, *PETHARP;


/*
typedef struct _ROUTEENTRY
{

//       INITIALLY A STATIC STRUCTURE, BUILT FROM THE CONFIGURATION FILE

	ULONG	NETWORK;	// NETWORK 
	ULONG	SUBNET;		// SUBNET MASK
	ULONG	GATEWAY;	// GATEWAY IP ADDRESS
	UCHAR	PORT;		// POINTER TO PORT ENTRY
	UCHAR	RTYPE;		// TYPE (NETROM/VC/DG/ETH)
	UCHAR	METRIC;		// FOR RIP 
	UCHAR	ROUTEINFO;  // TYPE (LEARNED, SYSOP, ETC)
	UCHAR	ROUTECHANGED;// CHANGED RECENTLY FLAG
	UCHAR	RIPTIMOUT;  // FOR REMOVING ACTIVE ENTRIES
	UCHAR	GARTIMOUT;  // FOR REMOVING DEAD ENTRIES
	int		FRAMECOUNT; // FRAMES SENT TO THIS NETWORK
	UCHAR	Target[7];	// MAC Address - either enet or ax.25 (may use arp later)


} ROUTEENTRY, *PROUTEENTRY;
*/
/*
;       ARP DATA
;
;       USED TO TRANSLATE IP ADDRESSES TO MAC (Ether or ax.25) ADDDRESSES
*/
typedef struct _ARPDATA
{

//       KEEP IP ADDR AT FRONT

	ULONG	IPADDR;
	UCHAR	HWADDR[7];				// ETHERNET/ax.25 ADDR
	BOOL	ARPVALID;				// NONZERO IF ADDRESS HAS BEEN RESOLVED
	ULONG	ARPTIMER;				// TIMEOUT AFTER 5 MINS
	UCHAR	ARPINTERFACE;			// Port to use. 0= NETROM, 0xff Ethernet
	UCHAR	ARPTYPE;				// NETROM/VC/DG/ETH
	struct _MESSAGE * ARP_Q;		// CHAIN OF DATAGRAMS WAITING FOR RESOLUTION

} ARPDATA, *PARPDATA;

typedef struct _IPSTATS
{
	int	ARPEntries;
	int FramesForwared;
	int FramesDropped;
	int ARPTimeouts;

} IPSTATS, PIPSTATS;


#define MAX_ENTRIES 128

struct map_table_entry
{
	unsigned int sourceipaddr;
	unsigned short sourceport;			// bytes to compare (6 or 7)
	unsigned int mappedipaddr;
	unsigned short mappedport;
	unsigned char hostname[64];
	unsigned int error;
	BOOL ResolveFlag;			// True if need to resolve name
};




#pragma pack()

HANDLE hInstance;

//unsigned long _beginthread( void( *start_address )( void *), unsigned stack_size, char * arglist);

Dll BOOL APIENTRY Init_IP();
Dll BOOL APIENTRY Poll_IP();  
BOOL Send_ETH(VOID * Block, DWORD len);
VOID ProcessEthARPMsg(PETHARP arpptr);
VOID ProcessEthIPMsg(PVOID Buffer);
VOID ProcessAXARPMsg(PAXARP arpptr);
VOID ProcessIPMsg(PIPMSG IPptr, UCHAR * MACADDR, CHAR Type, UCHAR Port);
BOOL CheckIPChecksum(PIPMSG IPptr);
VOID ProcessICMPMsg(PIPMSG IPptr);
VOID RouteIPMsg(PIPMSG IPptr);
VOID SendIPtoBPQDEV(PIPMSG IPptr, UCHAR * HWADDR);
VOID SendIPtoAX25(PIPMSG IPptr, UCHAR * HWADDR, int Port, char Mode);
PARPDATA AllocARPEntry();
VOID SendARPMsg(PARPDATA ARPptr);
PARPDATA LookupARP(ULONG IPADDR, BOOL Add, BOOL * Found);
BOOL ReadConfigFile();
ProcessLine(char * buf);
VOID DoARPTimer();
UINT SENDNETFRAME;
VOID SendNetFrame(UCHAR * ToCall, UCHAR * FromCall, UCHAR * Block, DWORD Len, UCHAR Port);
VOID ReadARP();
BOOL ProcessARPLine(char * buf);
void IPResolveNames(void *dummy);
int CheckSumAndSend(PIPMSG IPptr, PTCPMSG TCPmsg, USHORT Len);

VOID SaveARP();
VOID WriteARPLine(PARPDATA ArpRecord);

int InitPCAP(void);
int OpenPCAP(void);