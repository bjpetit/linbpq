;
;	22/11/95 - Add second port alias for digipeating (for APRS)
;


TNCBUFFLEN	EQU	400h


MAXDIGIS	EQU	8
;
;	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT
;
MESSAGE	STRUC

MSGCHAIN	DW	?		; CHAIN WORD
MSGPORT		DB	?		; PORT 
MSGLENGTH	DW	?		; LENGTH

MSGDEST		DB	7 DUP (?)	; DESTINATION
MSGORIGIN	DB	7 DUP (?)	; ORIGIN
;
;	 MAY BE UP TO 56 BYTES OF DIGIS
;
MSGCONTROL	DB	?		; CONTROL BYTE
MSGPID		DB	?		; PROTOCOL IDENTIFIER
MSGDATA		DB	?		; START OF LEVEL 2 MESSAGE
;
MESSAGE	ENDS
;

L3MESSAGE	STRUC
;
;	NETROM LEVEL 3 MESSAGE - WITHOUT L2 INFO 
;
L3HEADER	DB	5 DUP (?)	; CHAIN, PORT, LENGTH
L3PID		DB	?		; PID

L3SRCE		DB	7 DUP (?)	; ORIGIN NODE
L3DEST		DB	7 DUP (?)	; DEST NODE
L3MONR		DB	?		; TX MONITOR FIELD - TO PREVENT MESSAGE GOING
					; ROUND THE NETWORK FOR EVER DUE TO ROUTING LOOP
;
;	NETROM LEVEL 4 DATA
;
L4INDEX		DB	?		; TRANSPORT SESSION INDEX
L4ID		DB	?		; TRANSPORT SESSION ID
L4TXNO		DB	?		; TRANSMIT SEQUENCE NUMBER
L4RXNO		DB	?		; RECEIVE (ACK) SEQ NUMBER
L4FLAGS		DB	?		; FRAGMENTATION, ACK/NAK, FLOW CONTROL AND MSG TYPE BITS

L4DATA		DB	?		; DATA
L4CALLS		DB	14 DUP (?)	; CALLS IN CONNECT REQUEST
L4_BPQ		DB	?		; THENODE EXTENDED CONNECT PARAMS

L3MESSAGE	ENDS

;
;	L4FLAGS DEFINITION
;
L4BUSY		EQU	80H		; BNA - DONT SEND ANY MORE
L4NAK		EQU	40H		; NEGATIVE RESPONSE FLAG
L4MORE		EQU	20H		; MORE DATA FOLLOWS - FRAGMENTATION FLAG

L4CREQ		EQU	1		; CONNECT REQUEST
L4CACK		EQU	2		; CONNECT ACK
L4DREQ		EQU	3		; DISCONNECT REQUEST
L4DACK		EQU	4		; DISCONNECT ACK
L4INFO		EQU	5		; INFORMATION
L4IACK		EQU	6		; INFORMATION ACK
;
;	PORT CONTROL TABLE
;
PORTCONTROL STRUC

PORTCALL	DB	7 DUP (0)
PORTALIAS	DB	7 DUP (0)	; USED FOR UPLINKS ONLY
PORTNUMBER	DB	?
PORTPOINTER	DW	?		; NEXT IN CHAIN
PORTQUALITY	DB	?		; 'STANDARD' QUALITY FOR THIS PORT
PORTRX_Q	DW	?		; FRAMES RECEIVED ON THIS PORT
PORTTX_Q	DW	?		; FRAMES TO BE SENT ON THIS PORT
PORTTXROUTINE	DW	?		; POINTER TO TRANSMIT ROUTINE FOR THIS PORT
PORTRXROUTINE	DW	?		; POINTER TO RECEIVE ROUTINE FOR THIS PORT
PORTINITCODE	DW	?		; INITIALISATION ROUTINE
PORTTIMERCODE	DW	?
PORTDESCRIPTION	DB	30 DUP (0)	; TEXT DESCRIPTION OF FREQ/SPEED ETC
PORTBBSFLAG	DB	?		; NZ MEANS PORT CALL/ALIAS ARE FOR BBS
PORTL3FLAG	DB	?		; NZ RESTRICTS OUTGOING L2 CONNECTS
;
;	CWID FIELDS
;
CWID		DW	9 DUP (0)	; 8 ELEMENTS + FLAG
ELEMENT		DW	?		; REMAINING BITS OF CURRENT CHAR
CWPOINTER	DW	?		; POINTER TO NEXT CHAR
CWIDTIMER	DW	?		; TIME TO NEXT ID
CWSTATE		DB	?		; STATE MACHINE FOR CWID
CWTYPE		DB	?		; SET TO USE ON/OFF KEYING INSTEAD OF
					; FSK (FOR RUH MODEMS)
PORTMINQUAL	DB	?		; MIN QUAL TO BRAOCAST ON THIS PORT

;	STATS COUNTERS
;
L2DIGIED	DW	?,?
L2FRAMES	DW	?,?
L2FRAMESFORUS	DW	?,?
L2FRAMESSENT	DW	?,?
L2TIMEOUTS	DW	?,?
L2ORUNC		DW	?,?		; OVERRUNS
L2URUNC		DW	?,?		; UNDERRUNS
L1DISCARD	DW	?,?		; FRAMES DISCARDED (UNABLE TO TX DUE TO DCD)
L2FRMRRX	DW	?,?
L2FRMRTX	DW	?,?
RXERRORS	DW	?,?		; RECEIVE ERRORS
L2REJCOUNT	DW	?,?		; REJ FRAMES RECEIVED
L2OUTOFSEQ	DW	?,?		; FRAMES RECEIVED OUT OF SEQUENCE
L2RESEQ		DW	?,?		; FRAMES RESEQUENCED
SENDING		DW	0		; LINK STATUS BITS
ACTIVE		DW	0

AVSENDING	DB	0		; LAST MINUTE
AVACTIVE	DB	0

PORTTYPE	DB	0		; H/W TYPE
					; 0 = ASYNC, 2 = PC120, 4 = DRSI
					; 6 = TOSH, 8 = QUAD, 10 = RLC100
					; 12 = RLC400 14 = INTERNAL 16 = EXTERNAL

PROTOCOL	DB	0		; PORT PROTOCOL
					; 0 = KISS, 2 = NETROM, 4 = BPQKISS
					; 6 = HDLC, 8 = L2

IOBASE		DW	?		; CONFIG PARAMS FOR HARDWARE DRIVERS 
INTLEVEL	DB	?		; FIRST 4 SAME FOR ALL H/W TYPES
BAUDRATE	DW	?		; SPEED
CHANNELNUM	DB	?		; ON MULTICHANNEL H/W
INTCHAIN	DW	?		; POINTER TO NEXT PORT USING THIS LEVEL
PORTWINDOW	DB	?		; L2 WINDOW FOR THIS PORT
PORTTXDELAY	DB	?		; TX DELAY FOR THIS PORT
PORTPERSISTANCE	DB	?		; PERSISTANCE VALUE FOR THIS PORT
FULLDUPLEX	DB	?		; FULL DUPLEX IF SET
SOFTDCDFLAG	DB	?		; IF SET USE 'SOFT DCD' - IF MODEM CANT GIVE A REAL ONE
PORTSLOTTIME	DB	?		; SLOT TIME
PORTTAILTIME	DB	?		; TAIL TIME
BBSBANNED	DB	?		; SET IF PORT CAN'T ACCEPT L2 CALLS TO BBS CALLSIGN 
PORTT1		DB	?		; L2 TIMEOUT
PORTT2		DB	?		; L2 DELAYED ACK TIMER
PORTN2		DB	?		; RETRIES
PORTPACLEN	DB	?		; DEFAULT PACLEN FOR INCOMING SESSIONS
PORTINTERRUPT	DW	?		; ADDRESS OF INTERRUPT HANDLER

QUAL_ADJUST	DB	?		; % REDUCTION IN QUALITY IF ON SAME PORT

PERMITTEDCALLS	DW	?		; POINTER TO PERMITED CALLS LIST
PORTUNPROTO	DW	?		; POINTER TO UI DEST AND DIGI LIST
PORTDISABLED	DB	0		; PORT TX DISABLE FLAG
DIGIFLAG	DB	0		; ENABLE/DISABLE/UI ONLY
DIGIPORT	DB	0		; CROSSBAND DIGI PORT
USERS		DB	0		; MAX USERS ON PORT
KISSFLAGS	DB	0		; KISS SPECIAL MODE BITS
PORTINTERLOCK	DB	0		; TO DEFINE PORTS WHICH CANT TX AT SAME TIME
NODESPACLEN	DB	0		; MAX LENGTH OF 'NODES' MSG 
TXPORT		DB	0		; PORT FOR SHARED TX OPERATION
PORTMHEARD	DW	0		; POINTER TO MH DATA
PARAMTIMER	DW	0		; MOVED FROM HW DATA FOR SYSOPH
PORTMAXDIGIS	DB	0		; DIGIS ALLOWED ON THIS PORT
PORTALIAS2	DB	7 DUP (0)	; 2ND ALIAS FOR DIGIPEATING FOR APRS
HARDWAREDATA	DB	62 DUP (?)	; WORK AREA FOR HARDWARE DRIVERS

PORTCONTROL	ENDS

;
;	DEFINE MAPPING FOR EXTERNAL DRIVER
;
EXTDATA	STRUC
		DB	HARDWAREDATA DUP (0)	; REMAP HARDWARE INFO

PORT_EXT_ADDR	DD	?		; ADDR OF RESIDENT ROUTINE (PDD)
PORT_EXT_DS	DW	?		; DS OF PDD

EXTDATA	ENDS

	IF	TYPE EXTDATA GT TYPE PORTCONTROL
	.ERR2	TOO MUCH PORT DATA
	ENDIF

EXTERNAL	EQU	16
L2		EQU	8
;
;	CW STATE MACHINE EQUATES
;
dot		equ	1b
dash		equ	10b
dotspace	equ	100b
letterspace	equ	1000b
IDPENDING	EQU	10000B
;
;	LEVEL 2 LINK CONTROL TABLE
;
LINKTABLE	STRUC

LINKCALL	DB	7 DUP (?)	; CALLSIGN OF STATION
OURCALL		DB	7 DUP (?)	; CALLSIGN OF OUR END
DIGIS		DB	MAXDIGIS*7 DUP (?)	; LEVEL 2 DIGIS IN PATH

LINKPORT	DB	?		; PORT NUMBER
LINKTYPE	DB	?		; 1 = UP, 2= DOWN, 3 = INTERNODE

LINKNR		DB	?
LINKNS		DB	?		; LEV 2 SEQUENCE COUNTS
LINKWS		DB	?		; WINDOW START
LINKOWS		DB	?		; OLD (LAST ACKED) WINDOW START
LINKWINDOW	DB	?		; LEVEL 2 WINDOW SIZE

L2FLAGS		DB	?		; CONTROL BITS
VER1FLAG	DB	?		; SET IF OTHER END RUNNING VERSION 1
  
RX_Q		DW	?		; PACKETS RECEIVED ON THIS LINK
L2RESEQ_Q	DW	?		; FRAMES RECEIVED OUT OF SEQUENCE

TX_Q		DW	?		; PACKETS TO SEND
FRAMES		DW	8 DUP (?)	; FRAMES WAITING ACK

L2STATE		DB	?		; PROCESSING STATE
L2TIMER		DW	?		; FRAME RETRY TIMER
L2TIME		DB	?		; RETRY TIMER INITIAL VALUE
L2SLOTIM	DW	?		; DELAY FOR LINK VALIDATION POLL
L2ACKREQ	DB	?		; DELAYED TEXT ACK TIMER
REJTIMER	DB	?		; TO TIME OUT REJ  IN VERSION 1
LAST_F_TIME	DW	?		; TIME LAST R(F) SENT
SDREJF		DB	?		; FLAGS FOR FRMR
SDTSLOT		DB	?		; POINTER TO NEXT TXSLOT TO USE

L2RETRIES	DB	?		; RETRY COUNTER

SESSACTIVE	DB	?		; SET WHEN WE ARE SURE SESSION IS UP

KILLTIMER	DW	?		; TIME TO KILL IDLE LINK

CIRCUITPOINTER	DW	?		; POINTER TO L4 CIRCUIT TABLE ENTRY
					; (IF UP/DOWN)
NEIGHBOUR	DW	?		; POINTER TO NEIGHBOUR (IF CROSSLINK)

L2FRAG_Q	DW	?		; DEFRAGMENTATION QUEUE

LINKTABLE	ENDS
;
;	L2FLAGS EQUATES
;
REJSENT		EQU	1B		; SET WHEN FIRST REJ IS SENT IN REPLY
					; TO AN I(P)
RNRSET		EQU	10B		; RNR RECEIVED FROM OTHER END
;DISCPENDING	EQU	1000B		; SEND DISC WHEN ALL DATA ACK'ED
RNRSENT		EQU	10000B		; WE HAVE SEND RNR
POLLSENT	EQU	100000B		; POLL BIT OUTSTANDING
;
;	FRMR REJECT FLAGS
;
SDINVC		EQU	1B		; INVALID COMMAND
SDNRER		EQU	1000B		; INVALID N(R)

TRANSPORTENTRY	STRUC

L4USER		DB	7 DUP (?)	; CALL OF ORIGINATING USER
L4TARGET	DW	?		; POINTER TO TARGET LINK/DEST
L4MYCALL	DB	7 DUP (0)	; CALL WE ARE USING

CIRCUITINDEX	DB	?		; OUR CIRCUIT INFO
CIRCUITID	DB	?

FARINDEX	DB	?
FARID		DB	?		; OTHER END'S INFO

L4WINDOW	DB	?		; WINDOW SIZE
L4WS		DB	?		; WINDOW START - NEXT FRAME TO ACK
TXSEQNO		DB	?
RXSEQNO		DB	?		; TRANSPORT LEVEL SEQUENCE INFO
L4LASTACKED	DB	?		; LAST SEQUENCE ACKED

FLAGS		DB	?		; TRANSPORT LEVEL FLAGS
NAKBITS		DB	?		; NAK & CHOKE BITS TO BE SENT
L4CROSSLINK	DW	?		; POINTER TO LINKED L4 SESSION ENTRY
L4CIRCUITTYPE	DB	?		; BIT SIGNIFICANT - SEE BELOW
L4TX_Q		DW	?
L4RX_Q		DW	?
L4HOLD_Q	DW	?		; FRAMES WAITING TO BE ACKED
L4RESEQ_Q	DW	?		; FRAMES RECEIVED OUT OF SEQUENCE

L4STATE		DB	?
L4TIMER		DW	?
L4ACKREQ	DB	?		; DATA ACK NEEDED
L4RETRIES	DB	?		; RETRY COUNTER
L4KILLTIMER	DW	0		; IDLE CIRCUIT TIMER 
SESSIONT1	DW	0		; TIMEOUT FOR SESSIONS FROM HERE
SESSPACLEN	DB	0		; PACLEN FOR THIS SESSION
BADCOMMANDS	DB	0		; SUCCESSIVE BAD COMMANDS
STAYFLAG	DB	0		; STAY CONNECTED FLAG
SPYFLAG		DB	0		; SPY - CONNECT TO NODE VIA BBS CALLSIGN
		
RTT_SEQ		DB	0		; SEQUENCE NUMBER BEING TIMED
RTT_TIMER	DW	0		; TIME ABOVE SEQUENCE WAS SENT

PASSWORD	DW	0		; AUTHORISATION CODE FOR REMOTE SYSOP
;
SESS_APPLFLAGS	DB	0		; APPL FLAGS FOR THIS SESSION

DUMPPTR		DW	0		; POINTER FOR REMOTE DUMP MODE

TRANSPORTENTRY	ENDS
;
;	CIRCUITTYPE EQUATES
;
L2LINK		EQU	1
SESSION		EQU	10B
UPLINK		EQU	100B
DOWNLINK	EQU	1000B
	IF	TNC2
HOST		EQU	10000B
	ENDIF
BPQHOST		EQU	100000B
;
;	FLAGS EQUATES
;
DISCPENDING	EQU	1000B		; SEND DISC WHEN ALL DATA ACK'ED
;
;	TOP 4 BITS MATCH L4FLAGS BITS - ALSO USED FOR NAKBITS
;
;L4BUSY		EQU	80H		; BNA - DONT SEND ANY MORE
;L4NAK		EQU	40H		; NEGATIVE RESPONSE FLAG
;L4MORE		EQU	20H		; MORE DATA FOLLOWS - FRAGMENTATION FLAG



DEST_LIST	STRUC

DEST_CHAIN	DW	?		; SORTED LIST CHAIN

DEST_CALL	DB	7 DUP (?)	; DESTINATION CALLSIGN (AX25 FORMAT)
DEST_ALIAS	DB	6 DUP (?)

DEST_STATE	DB	?		; CONTROL BITS - SETTING UP, ACTIVE ETC	

DEST_ROUTE	DB	?		; CURRENTY ACTIVE DESTINATION

ROUT1_QUALITY	DB	?		; QUALITY
ROUT1_OBSCOUNT	DB	?
ROUT1_NEIGHBOUR	DW	?		; POINTER TO NEXT NODE IN PATH

ROUT2_QUALITY	DB	?
ROUT2_OBSCOUNT	DB	?
ROUT2_NEIGHBOUR	DW	?

ROUT3_QUALITY	DB	?
ROUT3_OBSCOUNT	DB	?
ROUT3_NEIGHBOUR	DW	?

DEST_Q		DW	?		; QUEUE OF FRAMES FOR THIS DESTINATION

DEST_RTT	DW	?		; SMOOTHED ROUND TRIP TIMER
DEST_COUNT	DW	?		; FRAMES SENT

DEST_LIST	ENDS


NEIGHBOUR_LIST	STRUC

NEIGHBOUR_CALL	DB	7 DUP (?)	; AX25 CALLSIGN	
NEIGHBOUR_DIGI	DB	14 DUP (?)	; DIGIS EN ROUTE (MAX 2 - ?? REMOVE)

NEIGHBOUR_PORT	DB	?
NEIGHBOUR_QUAL	DB	?
NEIGHBOUR_FLAG	DB	?		; SET IF 'LOCKED' ROUTE

NEIGHBOUR_LINK	DW	?		; POINTER TO LINK FOR THIS NEIGHBOUR

NEIGHBOUR_TIME	DW	?		; TIME LAST HEARD (HH MM)

NBOUR_IFRAMES	DD	?		; FRAMES SENT/RECEIVED
NBOUR_RETRIES	DD	?		; RETRASMISSIONS

NBOUR_MAXFRAME	DB	?		; FOR OPTIMISATION CODE
NBOUR_FRACK	DB	?
NBOUR_PACLEN	DB	?


NEIGHBOUR_LIST	ENDS

	IF	TNC2

TNCDATA		STRUC

RXBUFFER	DB	TNCBUFFLEN DUP (?)	; BUFFER TO USER

BIOSPORT	DB	?		; INT 14 PORT NUMBER
;
VMSR		DB	?		; VIRTUAL MSR
RTSFLAG		DB	?		; BIT 0 SET IF RTS/DTR UP
VLSR		DB	?		; LAST RECEIVED LSR VALUE

RXCOUNT		DW	?		; BYTES IN RX BUFFER
PUTPTR		DW	?		; POINTER FOR LOADING BUFFER
GETPTR		DW	?		; POINTER FOR UNLOADING BUFFER

CURSOR		DW	?		; POSTION IN KEYBOARD BUFFER
KBBUFFER	DW	?		; BUFFER ALLOCATED TO KEYBOARD
KBSESSION	DW	?		; POINTER TO L4 SESSION ENTRY FOR CONSOLE
;		DW	14 DUP (?)	; 14 MORE SESSIONS FOR HOST MODE PORT
MSGLEN		DW	?
TRANSTIMER	DB	?		; TRANPARENT MODE SEND TIMOUT
AUTOSENDFLAG	DB	?		; SET WHEN TRANSMODE TIME EXPIRES

TRACEFLAG	DB	?		; SET NZ TO STOP TRACE
ECHOFLAG	DB	?		; ECHO ON/OFF
FLOWFLAG	DB	?		; FLOW OFF/ON

CONOK		DB	?
CBELL		DB	?		; BEEP ON CONNECT FLAG
NOMODE		DB	?		; MODE CHANGE FLAGS
NEWMODE		DB	?
CONMODEFLAG	DB	?		; CONNECT MODE - CONV OR TRANS
LFIGNORE	DB	?
MCON		DB	?		; TRACE MODE FLAGS 
MCOM		DB	?
MALL		DB	?
BBSMON		DB	?		; SPECIAL SHORT MONITOR FOR BBS
MTX		DB	?		; MONITOR TRANSMITTED FRAMES
MTXFORCE	DB	?		; MONITOR TRANSMITTED FRAMES EVEN IF M OFF
MMASK		DB	?		; MONITOR PORT MASK
HEADERLN	DB	?		; PUT MONITORED DATA ON NEW LINE FLAG
	
MODEFLAG	DB	?		; COMMAND/DATA MODE

APPLICATION	DB	?		; BIT SIGNIFICANT FIELD TO DEFINE
					; WHICH APPLICATION PORT SUPPORTS
					; BIT 0 = BBS FOR BACKWARD COMPATIBILITY
					; BIT 1 = HOST (BY CONVENTION)
					; BIT 2 = SYSOP (DITTO)
					; BITS 3 - 7 - USER DEFINABLE
;
APPLFLAGS	DB	?		; FLAGS TO CONTROL APPL SYSTEM
HOSTFLAG	DB	?		; SET WHEN RUNNING HOST MODE
;MAXHOSTSESSIONS	DB	?		; MAX SESSIONS ON HOST MODE PORT

;HOSTREPLYCURSOR DW	0
;HOSTREPLYBUFFER DW	0

SENDPAC		DB	?		; SEND PACKET CHAR
CPACTIME	DB	?		; USE PACTIME IN CONV MODE
CRFLAG		DB	?		; APPEND SENDPAC FLAG

TPACLEN		DB	?		; MAX PACKET SIZE FOR TNC GENERATED PACKETS
UNPROTO		DB	64 DUP (?)	; UNPROTO DEST AND DIGI STRING

TNCDATA		ENDS

	ENDIF

;
;	MODEFLAG DEFINITIONS
;
COMMAND		EQU	1B
TRANS		EQU	10B
CONV		EQU	100B
;
;	APPL DEFINITIONS
;
BBSAPPL		EQU	1B
HOSTAPPL	EQU	10B
SYSOPAPPL	EQU	100B
;
;	HOSTFLAG DEFINITIONS
;
;HOSTMODE	EQU	1B		; PK232 HOSTMODE ENABLED
;HOSTESCBIT	EQU	10B		; IN ESCAPE SEQUENCE
;UFQ		EQU	100B		; UFQ MODE (IE NOT AA4RE)
;POLLED		EQU	1000B		; POLL RECEIVED

;KISSMODE	EQU	10000B		; KISS MODE
;KISSESC		EQU	100000B		; IN ESCAPE SEQUENCE
;
;	APPLFLAGS BITS
;
CMD_TO_APPL	EQU	1B		; PASS COMMAND TO APPLICATION
MSG_TO_USER	EQU	10B		; SEND 'CONNECTED' TO USER
MSG_TO_APPL	EQU	100B		; SEND 'CONECTED' TO APPL
;
;	HDLC COMMANDS (WITHOUT P/F)
;
UI	EQU	3
SABM	EQU	2FH
DISC	EQU	43H
DM	EQU	0FH
UA	EQU	63H
FRMR	EQU	87H
RR	EQU	1
RNR	EQU	5
REJ	EQU	9
;
PFBIT	EQU	10H		; POLL/FINAL BIT IN CONTROL BYTE
;
;	MH DATA AREA
;
MHSTRUC	STRUC

MHCALL		DB	7 DUP (0)
MHTIME		DW	2 DUP (0)	; ? FROM BIOS	
MHDIGI		DB	0

MHSTRUC	ENDS
;
;	HARDWARE TYPE EQUATES
;
KISS		EQU	0
PC120		EQU	2
DRSI		EQU	4
TOSH		EQU	6
QUADRAM		EQU	8
RLC100		EQU	0AH
RLC400		EQU	0CH
INTERNAL	EQU	0EH
EXTERNAL	EQU	10H
BAYCOM		EQU	12H
PA0HZP		EQU	14H


