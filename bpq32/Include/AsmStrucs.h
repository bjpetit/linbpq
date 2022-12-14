// C equivalents of ASM STRUCS

// Aug 2010 Extend Applmask to 32 bit

#include "winsock.h"

#define _USE_32BIT_TIME_T	// Until the ASM code switches to 64 bit time

#define BPQICON 2

#define BUFFLEN	360

#define ApplStringLen 48	// Length of each config entry
#define NumberofAppls 32	// Max APPLICATIONS= values
#define ALIASLEN 32
#define MHENTRIES 30		// Entries in MH List



#define NRPID 0xcf			// NETROM PID

#define L4BUSY	0x80		// BNA - DONT SEND ANY MORE
#define L4NAK	0x40		// NEGATIVE RESPONSE FLAG
#define L4MORE	0x20		// MORE DATA FOLLOWS - FRAGMENTATION FLAG

#define L4CREQ	1		// CONNECT REQUEST
#define L4CACK	2		// CONNECT ACK
#define L4DREQ	3		// DISCONNECT REQUEST
#define L4DACK	4		// DISCONNECT ACK
#define L4INFO	5		// INFORMATION
#define L4IACK	6		// INFORMATION ACK


extern char MYCALL[];	// 7 chars, ax.25 format
extern char MYALIAS[];	// 6 chars, not null terminated
extern char L3RTT[7];	// 7 chars, ax.25 format
extern char L3KEEP[7];	// 7 chars, ax.25 format
//extern int SENDNETFRAME();
extern struct _DATABASE * DataBase;
//extern struct APPLCALLS  APPLCALLTABLE[8];

extern int MAXDESTS;

extern VOID * GETBUFF();
extern VOID SETUPNODEHEADER();
extern VOID POSTDATAAVAIL();
extern VOID FINDDESTINATION();
extern VOID L2SETUPCROSSLINK();
extern int REMOVENODE();

extern int DATABASE;
extern int ENDOFDATA;
extern int L3LIVES;
extern int NUMBEROFNODES;

#pragma pack(1) 

#define ApplOffset 80000			// Applications offset in config buffer
#define InfoOffset 85000			// Infomsg offset in config buffer
#define InfoMax	2000				// Max Info
#define HFCTextOffset 88000;		// HF modes CTEXT

struct APPLCONFIG
{
	char Command[12];
	char CommandAlias[48];
	char ApplCall[10];
	char ApplAlias[10];
	int ApplQual;
};


typedef struct _LINKTABLE
{
//;
//;	LEVEL 2 LINK CONTROL TABLE
//;

	UCHAR	LINKCALL[7];	// CALLSIGN OF STATION
	UCHAR	OURCALL[7];		// CALLSIGN OF OUR END
	UCHAR	DIGIS[56];		// LEVEL 2 DIGIS IN PATH

	UCHAR	LINKPORT;		// PORT NUMBER
	UCHAR	LINKTYPE;		// 1 = UP, 2= DOWN, 3 = INTERNODE

	UCHAR	LINKNR;
	UCHAR	LINKNS;			// LEV 2 SEQUENCE COUNTS
	UCHAR	LINKWS;			// WINDOW START
	UCHAR	LINKOWS;		// OLD (LAST ACKED) WINDOW START
	UCHAR	LINKWINDOW;		// LEVEL 2 WINDOW SIZE

	UCHAR	L2FLAGS;		// CONTROL BITS
	UCHAR	VER1FLAG;		// SET IF OTHER END RUNNING VERSION 1
  
	VOID *	RX_Q;			// PACKETS RECEIVED ON THIS LINK
	VOID *	L2RESEQ_Q;		// FRAMES RECEIVED OUT OF SEQUENCE

	VOID *	TX_Q;			// PACKETS TO SEND
	VOID *	FRAMES[8];		// FRAMES WAITING ACK

	UCHAR	L2STATE;		// PROCESSING STATE
	USHORT	L2TIMER;		// FRAME RETRY TIMER
	UCHAR	L2TIME;			// RETRY TIMER INITIAL VALUE
	USHORT	L2SLOTIM;		// DELAY FOR LINK VALIDATION POLL
	UCHAR	L2ACKREQ;		// DELAYED TEXT ACK TIMER
	UCHAR	REJTIMER;		// TO TIME OUT REJ  IN VERSION 1
	USHORT	LAST_F_TIME;	// TIME LAST R(F) SENT
	UCHAR	SDREJF	;		// FLAGS FOR FRMR
	UCHAR	SDTSLOT	;		// POINTER TO NEXT TXSLOT TO USE

	UCHAR	L2RETRIES;		// RETRY COUNTER

	UCHAR	SESSACTIVE;		// SET WHEN WE ARE SURE SESSION IS UP

	USHORT	KILLTIMER;		// TIME TO KILL IDLE LINK

	VOID *	CIRCUITPOINTER;	// POINTER TO L4 CIRCUIT TABLE ENTRY
							// (IF UP/DOWN)
	VOID *	NEIGHBOUR;		// POINTER TO NEIGHBOUR (IF CROSSLINK)

	VOID *	L2FRAG_Q;		// DEFRAGMENTATION QUEUE

} LINKTABLE;



typedef struct _L3MESSAGE
{
//
//	NETROM LEVEL 3 MESSAGE - WITHOUT L2 INFO 
//
//	UCHAR	L3HEADER[7];		// CHAIN, PORT, LENGTH
//	UCHAR	L3PID;				// PID

	UCHAR	L3SRCE[7];			// ORIGIN NODE
	UCHAR	L3DEST[7];			// DEST NODE
	UCHAR	L3TTL;				// TX MONITOR FIELD - TO PREVENT MESSAGE GOING
								// ROUND THE NETWORK FOR EVER DUE TO ROUTING LOOP
//
//	NETROM LEVEL 4 DATA
//
	UCHAR	L4INDEX;			// TRANSPORT SESSION INDEX
	UCHAR	L4ID;				// TRANSPORT SESSION ID
	UCHAR	L4TXNO;				// TRANSMIT SEQUENCE NUMBER
	UCHAR	L4RXNO;				// RECEIVE (ACK) SEQ NUMBER
	UCHAR	L4FLAGS;			// FRAGMENTATION, ACK/NAK, FLOW CONTROL AND MSG TYPE BITS

	UCHAR	L4DATA[236]	;		//DATA

}L3MESSAGE, *L3PMESSAGE;


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

	union 
	{                   /*  array named screen */
		UCHAR L2DATA[256];
		struct _L3MESSAGE L3MSG;

	};

}MESSAGE, *PMESSAGE;

typedef struct HDDRWITHDIGIS
{
//	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT

	struct _MESSAGE * CHAIN;

	UCHAR	PORT;
	USHORT	LENGTH;

	UCHAR	DEST[7];
	UCHAR	ORIGIN[7];
	UCHAR	DIGIS[8][7];

	UCHAR	CTL;
	UCHAR	PID; 

	union 
	{                   /*  array named screen */
		UCHAR L2DATA[256];
		struct _L3MESSAGE L3MSG;

	};

} DIGIMESSAGE, *PDIGIMESSAGE;


//
//	BPQHOST MODE VECTOR STRUC
//

struct BPQVECSTRUC
{
	struct TRANSPORTENTRY *	HOSTSESSION;	// Pointer to Session
	UCHAR	HOSTFLAGS;		// ALLOCATED AND STATE CHANGE FLAGS
	ULONG	HOSTAPPLMASK;
	UCHAR	HOSTAPPLFLAGS;
	UCHAR	HOSTSTREAM;		// STREAM NUMBER
	PMESSAGE	HOSTTRACEQ;
	HWND	HOSTHANDLE;		// HANDLE FOR POSTING MSGS TO
	ULONG	HOSTAPPLNUM;	// Application Number
	ULONG	STREAMOWNER;	//	PID of Process owning stream
	char	PgmName[32];	// Program Name;


}  bpqvecstruc;

//
//	Session Record
//
struct TRANSPORTENTRY
{
	UCHAR	L4USER[7];				// CALL OF ORIGINATING USER
	VOID *	L4TARGET;				// POINTER TO TARGET LINK/DEST
	UCHAR	L4MYCALL[7];			// CALL WE ARE USING

	UCHAR	CIRCUITINDEX;			// OUR CIRCUIT INFO
	UCHAR	CIRCUITID;			//

	UCHAR	FARINDEX;			//
	UCHAR	FARID;			//	OTHER END'S INFO

	UCHAR	L4WINDOW;			// WINDOW SIZE
	UCHAR	L4WS;				// WINDOW START - NEXT FRAME TO ACK
	UCHAR	TXSEQNO;			//
	UCHAR	RXSEQNO;			// TRANSPORT LEVEL SEQUENCE INFO
	UCHAR	L4LASTACKED;		// LAST SEQUENCE ACKED

	UCHAR	FLAGS;				// TRANSPORT LEVEL FLAGS
	UCHAR	NAKBITS;			// NAK & CHOKE BITS TO BE SENT
	struct TRANSPORTENTRY * L4CROSSLINK; // POINTER TO LINKED L4 SESSION ENTRY
	UCHAR	L4CIRCUITTYPE;		// BIT SIGNIFICANT - SEE BELOW
	UCHAR	KAMSESSION;			// Session Number on KAM Host Mode TNC
	VOID *	L4TX_Q;
	VOID *	L4RX_Q;	
	VOID *	L4HOLD_Q;			// FRAMES WAITING TO BE ACKED
	VOID *	L4RESEQ_Q;			// FRAMES RECEIVED OUT OF SEQUENCE

	UCHAR	L4STATE;
	USHORT	L4TIMER;
	UCHAR	L4ACKREQ;			// DATA ACK NEEDED
	UCHAR	L4RETRIES;			// RETRY COUNTER
	USHORT	L4KILLTIMER;		// IDLE CIRCUIT TIMER 
	USHORT	SESSIONT1;			// TIMEOUT FOR SESSIONS FROM HERE
	UCHAR	SESSPACLEN;			// PACLEN FOR THIS SESSION
	UCHAR	BADCOMMANDS;		// SUCCESSIVE BAD COMMANDS
	UCHAR	STAYFLAG;			// STAY CONNECTED FLAG
	UCHAR	SPYFLAG;			// SPY - CONNECT TO NODE VIA BBS CALLSIGN
		
	UCHAR	RTT_SEQ;			// SEQUENCE NUMBER BEING TIMED
	USHORT	RTT_TIMER;			// TIME ABOVE SEQUENCE WAS SENT

	USHORT	PASSWORD;			// AUTHORISATION CODE FOR REMOTE SYSOP

	UCHAR	SESS_APPLFLAGS;		// APPL FLAGS FOR THIS SESSION

	UCHAR	Secure_Session;		// Set if Host session from BPQTerminal or BPQMailChat

	VOID *	DUMPPTR;			// POINTER FOR REMOTnE DUMP MODE
	VOID *	PARTCMDBUFFER;		//  Save area for incomplete commmand

	int Frequency;				// If known - for CMS Reporting Hz
	char RMSCall[10];
	UCHAR Mode;					// ditto

	int UNPROTO;				// Unproto Mode flag - port number if in unproto mode
	int UAddrLen;				//
	char UADDRESS[64];			// Unproto Address String - Dest + Digis
};

//
//	CIRCUITTYPE EQUATES
//

#define L2LINK		1
#define SESSION		2
#define UPLINK		4
#define DOWNLINK	8
#define BPQHOST		32
#define PACTOR		64


struct APPLCALLS
{

//	Application Calls/Alias Supports multiple L4 application calls

	UCHAR APPLCALL[7];				// ax.25
	char APPLALIAS_TEXT[10];		// TEXT, WITH APPENDED SPACE

	char APPLCALL_TEXT[10];
	UCHAR APPLALIAS[6];
	char Filler;					// So we can use ConvtoAX25 on Alias
	USHORT APPLQUAL;
	struct DEST_LIST * NODEPOINTER;	// Pointer to "NODES" entry for this App (if L4)

	char APPLCMD[13];				//
	BOOL APPLHASALIAS;
	int APPLPORT;					// Port used if APPL has an Alias
	char * APPLALIASPTRl;			// Pointer to Alias if defined

} applcalls;

//
//	We store the Time Received from our neighbour. Before using it we add our current route SRTT
//  This way our times adjust to changes of neighbour SRTT. We can't cater for changes to other hop RTTs,
//	But if these are significant (say 25% or 100 ms) they will be retransmitted

typedef struct NR_DEST_ROUTE_ENTRY
{
	struct ROUTE * ROUT_NEIGHBOUR;	// POINTER TO NEXT NODE IN PATH
	UCHAR ROUT_QUALITY;		// QUALITY
	UCHAR ROUT_OBSCOUNT;
	UCHAR Padding[5];		// SO Entries are the same lenght
};

typedef struct DEST_ROUTE_ENTRY
{
	struct ROUTE * ROUT_NEIGHBOUR;	// POINTER TO NEXT NODE IN PATH
	USHORT LastRTT;					// Last Value Reported
	USHORT RTT;						// Current	
	USHORT SRTT;					// Smoothed RTT
	UCHAR Hops;
};

struct DEST_LIST
{
	//	L4 Destinations (NODES)

	struct DEST_LIST * DEST_CHAIN;	// SORTED LIST CHAIN

	UCHAR DEST_CALL[7];			// DESTINATION CALLSIGN (AX25 FORMAT)
	UCHAR DEST_ALIAS[6];	

	UCHAR DEST_STATE;			// CONTROL BITS - SETTING UP, ACTIVE ETC	

	UCHAR DEST_ROUTE;			// CURRENTY ACTIVE DESTINATION
	UCHAR INP3FLAGS;

	struct NR_DEST_ROUTE_ENTRY NRROUTE1;// Besrt 3 NR neighbours for this dest
	struct NR_DEST_ROUTE_ENTRY NRROUTE2;
	struct NR_DEST_ROUTE_ENTRY NRROUTE3;

	struct DEST_ROUTE_ENTRY ROUTE1;		// Besrt 3 INP neighbours for this dest
	struct DEST_ROUTE_ENTRY ROUTE2;
	struct DEST_ROUTE_ENTRY ROUTE3;

	long DEST_Q;				// QUEUE OF FRAMES FOR THIS DESTINATION

	short DEST_RTT;				// SMOOTHED ROUND TRIP TIMER
	short DEST_COUNT;			//  FRAMES SENT

} dest_list;

// IMNP3FLAGS Equates

#define NewNode 1			// Just added to table, so need to pass on


typedef struct ROUTE
{
	// Adjacent Nodes

	UCHAR NEIGHBOUR_CALL[7];	// AX25 CALLSIGN	
	UCHAR NEIGHBOUR_DIGI1[7];	// DIGIS EN ROUTE (MAX 2 - ?? REMOVE)
	UCHAR NEIGHBOUR_DIGI2[7];	// DIGIS EN ROUTE (MAX 2 - ?? REMOVE)

	UCHAR NEIGHBOUR_PORT;
	UCHAR NEIGHBOUR_QUAL;
	UCHAR NEIGHBOUR_FLAG;		// SET IF 'LOCKED' ROUTE

	struct _LINKTABLE * NEIGHBOUR_LINK;		// POINTER TO LINK FOR THIS NEIGHBOUR

	USHORT NEIGHBOUR_TIME;		// TIME LAST HEARD (HH MM)

	int NBOUR_IFRAMES;			// FRAMES SENT/RECEIVED
	int NBOUR_RETRIES;			// RETRASMISSIONS

	UCHAR NBOUR_MAXFRAME;		// FOR OPTIMISATION CODE
	UCHAR NBOUR_FRACK;
	UCHAR NBOUR_PACLEN;

	BOOL INP3Node;
	BOOL NoKeepAlive;		// Suppress Keepalive Processing

	int Status;			// 
	int LastRTT;			// Last Value Reported
	int RTT;				// Current	
	int SRTT;				// Smoothed RTT
	int NeighbourSRTT;		// Other End SRTT
//	int RTTIncrement;		// Average of Ours and Neighbours SRTT in 10 ms
	int BCTimer;			// Time to next L3RTT Broadcast
	int Timeout;			// Lost Response Timer
	int Retries;			// Lost Response Count
	struct _MESSAGE * Msg;	// RIF being built
};

// Status Equates

#define GotRTTRequest 1		// Other end has sent us a RTT Packet
#define GotRTTResponse 2	// Other end has sent us a RTT Response
#define GotRIF 4			// Other end has sent RIF, so is probably an INP3 Node
							//	(could just be monitoring RTT for some reason
#define SentOurRIF 16		// Set when we have sent a rif for our Call and any ApplCalls
							//  (only sent when we have seen both a request and response)

typedef struct PORTCONTROL
{
	char PORTCALL[7];
	char PORTALIAS[7];		//USED FOR UPLINKS ONLY
	char PORTNUMBER;
	struct PORTCONTROL * PORTPOINTER; // NEXT IN CHAIN

	UCHAR PORTQUALITY;		// 'STANDARD' QUALITY FOR THIS PORT
	char * PORTRX_Q;		// FRAMES RECEIVED ON THIS PORT
	char * PORTTX_Q;		// FRAMES TO BE SENT ON THIS PORT

	UINT * PORTTXROUTINE;	// POINTER TO TRANSMIT ROUTINE FOR THIS PORT
	UINT * PORTRXROUTINE;	// POINTER TO RECEIVE ROUTINE FOR THIS PORT
	UINT * PORTINITCODE;		// INITIALISATION ROUTINE
	UINT * PORTTIMERCODE;	//
	int (FAR * PORTCLOSECODE) ();	// CLOSE ROUTINE
	int (FAR * PORTTXCHECKCODE) ();	// OK to TX Check

	char PORTDESCRIPTION[30];// TEXT DESCRIPTION OF FREQ/SPEED ETC

	char PORTBBSFLAG;		// NZ MEANS PORT CALL/ALIAS ARE FOR BBS
	char PORTL3FLAG;			// NZ RESTRICTS OUTGOING L2 CONNECTS
//
//	CWID FIELDS
//

	USHORT CWID[9];			// 8 ELEMENTS + FLAG
	USHORT ELEMENT;			// REMAINING BITS OF CURRENT CHAR
	char * CWPOINTER;		// POINTER TO NEXT CHAR
	USHORT CWIDTIMER;		// TIME TO NEXT ID
	char CWSTATE;			// STATE MACHINE FOR CWID
	char CWTYPE;				// SET TO USE ON/OFF KEYING INSTEAD OF
							// FSK (FOR RUH MODEMS)
	UCHAR PORTMINQUAL;		// MIN QUAL TO BRAOCAST ON THIS PORT

//	STATS COUNTERS

	int L2DIGIED;
	int L2FRAMES;
	int L2FRAMESFORUS;
	int L2FRAMESSENT;
	int L2TIMEOUTS;
	int L2ORUNC;			// OVERRUNS
	int L2URUNC;			// UNDERRUNS
	int L1DISCARD;			// FRAMES DISCARDED (UNABLE TO TX DUE TO DCD)
	int L2FRMRRX;
	int L2FRMRTX;
	int RXERRORS;			// RECEIVE ERRORS
	int L2REJCOUNT;			// REJ FRAMES RECEIVED
	int L2OUTOFSEQ;			// FRAMES RECEIVED OUT OF SEQUENCE
	int L2RESEQ;			// FRAMES RESEQUENCED

	USHORT SENDING;			// LINK STATUS BITS
	USHORT ACTIVE;

	UCHAR AVSENDING;			// LAST MINUTE
	UCHAR AVACTIVE;

	char PORTTYPE;	// H/W TYPE
					// 0 = ASYNC, 2 = PC120, 4 = DRSI
					// 6 = TOSH, 8 = QUAD, 10 = RLC100
					// 12 = RLC400 14 = INTERNAL 16 = EXTERNAL

	char PROTOCOL;	// PORT PROTOCOL
					// 0 = KISS, 2 = NETROM, 4 = BPQKISS
					//; 6 = HDLC, 8 = L2

	USHORT IOBASE;		// CONFIG PARAMS FOR HARDWARE DRIVERS 

	char INTLEVEL;		// NEXT 4 SAME FOR ALL H/W TYPES
	USHORT BAUDRATE;	// SPEED
	char CHANNELNUM;	// ON MULTICHANNEL H/W
	struct PORTCONTROL * INTCHAIN; // POINTER TO NEXT PORT USING THIS LEVEL
	UCHAR PORTWINDOW;	// L2 WINDOW FOR THIS PORT
	USHORT PORTTXDELAY;	// TX DELAY FOR THIS PORT
	UCHAR PORTPERSISTANCE;	// PERSISTANCE VALUE FOR THIS PORT
	UCHAR FULLDUPLEX;	// FULL DUPLEX IF SET
	UCHAR SOFTDCDFLAG;	// IF SET USE 'SOFT DCD' - IF MODEM CANT GIVE A REAL ONE
	UCHAR PORTSLOTTIME;	// SLOT TIME
	UCHAR PORTTAILTIME;	// TAIL TIME
	UCHAR BBSBANNED;	// SET IF PORT CAN'T ACCEPT L2 CALLS TO BBS CALLSIGN 
	UCHAR PORTT1;		// L2 TIMEOUT
	UCHAR PORTT2;		// L2 DELAYED ACK TIMER
	UCHAR PORTN2;		// RETRIES
	UCHAR PORTPACLEN;	// DEFAULT PACLEN FOR INCOMING SESSIONS

	UINT * PORTINTERRUPT; // ADDRESS OF INTERRUPT HANDLER

	UCHAR QUAL_ADJUST;	// % REDUCTION IN QUALITY IF ON SAME PORT

	char * PERMITTEDCALLS;	// POINTER TO PERMITED CALLS LIST
	char * PORTUNPROTO;		// POINTER TO UI DEST AND DIGI LIST
	UCHAR PORTDISABLED;		// PORT TX DISABLE FLAG
	UCHAR DIGIFLAG;			// ENABLE/DISABLE/UI ONLY
	UCHAR DIGIPORT;			// CROSSBAND DIGI PORT
	USHORT DIGIMASK;			// CROSSBAND DIGI MASK
	UCHAR USERS;			// MAX USERS ON PORT
	UCHAR KISSFLAGS;		// KISS SPECIAL MODE BITS
	UCHAR PORTINTERLOCK;	// TO DEFINE PORTS WHICH CANT TX AT SAME TIME
	UCHAR NODESPACLEN;		// MAX LENGTH OF 'NODES' MSG 
	UCHAR TXPORT;			// PORT FOR SHARED TX OPERATION
	struct MHSTRUC * PORTMHEARD;		// POINTER TO MH DATA

	USHORT PARAMTIMER;		// MOVED FROM HW DATA FOR SYSOPH
	UCHAR PORTMAXDIGIS;		// DIGIS ALLOWED ON THIS PORT
	char PORTALIAS2[7];		// 2ND ALIAS FOR DIGIPEATING FOR APRS
	char PORTBCALL[7];		// Source call for Beacon
	char PortNoKeepAlive;	// Default to no Keepalives
	char PortUIONLY;		// UI only port - no connects
	char UICAPABLE;			// Pactor-style port that can do UI

	struct WL2KInfo * WL2KInfo; // WL2K Report for this Port
	struct in_addr PORTIPADDR;		// IP address for "KISS over UDP"

}	PORTCONTROLX;

typedef struct _EXTPORTDATA
{
	struct PORTCONTROL PORTCONTROL	;	// REMAP HARDWARE INFO

	int (FAR * PORT_EXT_ADDR) ();		// ADDR OF RESIDENT ROUTINE
	char PORT_DLL_NAME[16];	
	UCHAR EXTRESTART;					// FLAG FOR DRIVER REINIT
	HINSTANCE DLLhandle;
	int MAXHOSTMODESESSIONS;			// Max Host Sessions supported (Used for KAM Pactor + ax.25 support)
	struct TRANSPORTENTRY * ATTACHEDSESSIONS[27];	// For PACTOR. etc
	BOOL PERMITGATEWAY;				//  Set if ax.25 ports can change callsign (ie SCS, not KAM
	int SCANCAPABILITIES;			//Type of scan control Controller supports (None, Simple, Connect Lock)
#define NONE 0
#define SIMPLE 1
#define CONLOCK 2

	UINT UI_Q;						// Unproto Frames for Session Mode Drivers (TRK, etc)

} EXTPORTDATA, *PEXTPORTDATA;

typedef struct _HDLCDATA
{
	struct PORTCONTROL PORTCONTROL	;	// REMAP HARDWARE INFO
//
//	Mapping of VXD fields (mainly to simplify debugging
//

	ULONG ASIOC;			// A CHAN ADDRESSES
	ULONG SIO;				// OUR ADDRESSES (COULD BE A OR B) 
	ULONG SIOC;
	ULONG BSIOC;			// B CHAN CONTROL

	struct _HDLCDATA * A_PTR; // PORT ENTRY FOR A CHAN
	struct _HDLCDATA * B_PTR; // PORT ENTRY FOR B CHAN

	VOID (FAR * VECTOR[4]) (); // INTERRUPT VECTORS

//	UINT * IOTXCA;				// INTERRUPT VECTORS
//	UINT * IOTXEA;
//	UINT * IORXCA;
//	UINT * IORXEA;	

	UCHAR LINKSTS;

	UINT * SDRNEXT;
	UINT * SDRXCNT;
	UINT * CURALP;
	UCHAR OLOADS;				// LOCAL COUNT OF BUFFERS SHORTAGES
	USHORT FRAMELEN;
	UINT * SDTNEXT;				// POINTER to NEXT BYTE to TRANSMIT
	USHORT SDTXCNT;				// CHARS LEFT TO SEND
	UCHAR RR0;					// CURRENT RR0
	UINT * TXFRAME;				// ADDRESS OF FRAME BEING SENT

	UCHAR SDFLAGS;				// GENERAL FLAGS

	UINT * PCTX_Q;				// HDLC HOLDING QUEUE
	UINT * RXMSG_Q;				// RX INTERRUPT TO SDLC BG


//;SOFTDCD		DB	0		; RX ACTIVE FLAG FOR 'SOFT DC
	UCHAR TXDELAY;				// TX KEYUP DELAY TIMER
	UCHAR SLOTTIME;				// TIME TO WAIT IF WE DONT SEND
	UCHAR FIRSTCHAR;			// CHAR TO SEND FOLLOWING TXDELAY
	USHORT L1TIMEOUT;			// UNABLE TO TX TIMEOUT
	UCHAR PORTSLOTIMER;

	USHORT TXBRG;				// FOR CARDS WITHOUT /32 DIVIDER
	USHORT RXBRG;	

	UCHAR WR10	;				// NRZ/NRZI FLAG

	int	IRQHand;

	ULONG IOLEN;				// Number of bytes in IO Space

	struct PORTCONTROL * DRIVERPORTTABLE;	// ADDR OF PORT TABLE ENTRY IN VXD
											// Used in NT Driver for Kernel Device Pointer

}HDLCDATA, * PHDLCDATA;


typedef struct _DATABASE
{
	char FILLER[14];
	char MAJORVERSION;
	char MINORVERSION;
	char FILLER1[16];

	struct ROUTE * NEIGHBOURS;
	short  ROUTE_LEN;
	short  MAXNEIGHBOURS;

	struct DEST_LIST * DESTS;				// NODE LIST
	short  DEST_LIST_LEN;
	short  MAXDESTS;			// MAX NODES IN SYSTEM

	struct _LINKTABLE * LINKS;
	short	LINK_TABLE_LEN; 
	short	MAXLINKS;
/*
L4TABLE		DD	0
MAXCIRCUITS	DW	50		; NUMBER OF L4 CIRCUITS

NUMBEROFPORTS	DW	0

TNCTABLE	DD	0
NUMBEROFSTREAMS	DW	0

ENDDESTLIST	DD	0		; NODE LIST+1
*/
};




typedef struct MHSTRUC
{
	UCHAR MHCALL[7];
	__time32_t MHTIME;
	BYTE MHDIGI;
	char MHFreq[12];
	char MHLocator[6];
};

typedef struct APRSSTATIONRECORD
{
	UCHAR MHCALL[10];				// Stored with space padding
	time_t MHTIME;					// Time last heard
	time_t LASTMSG;					// Time last message sent from this station (via IS)
	int Port;						// Port last heard on (zero for APRS-IS)
	BOOL IGate;						// Set if station is an IGate;
//	BYTE MHDIGI[56];				// Not sure if we need this
};


#pragma pack()

