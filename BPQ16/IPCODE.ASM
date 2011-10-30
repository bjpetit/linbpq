	PAGE    62,132

	INCLUDE	ENVIRON.ASM

	ASSUME  CS:CODE,DS:IPDATA,ES:IPDATA
;
;
;       IP GATEWAY ROUTINE FOR G8BPQ PACKET SWITCH
;
;       By John Wiseman, G8BPQ
;
;
;	VERSION 4.0 NOVEMBER 1995
;
;		ADD SUPPORT FOR OPERATION OVER ETHERNET
;		(IN NORMAL E_II FORMAT)
;
NEEDTCP	EQU	0

	PUBLIC	MYIPADDR,RELEASEBUFFER,SUMANDSEND,IPRELBUFF
	PUBLIC	IP_Q,IPGETBUFF,GETVALUE,NEWVALUE,IPNODE

	EXTRN	BPQHOSTPORT:FAR		; ENTRY TO BPQHOST INTERFACE

IPDATA	SEGMENT	PUBLIC	'DATA'

	PUBLIC	IPGATEINIT,IPGATEBG,CLOSEIP

	IF	NEEDTCP

	EXTRN	TCPSENDBUFFER:BYTE

	ENDIF

STARTOFCODE     LABEL   BYTE

CR      EQU     0DH
LF      EQU     0AH
;
MAXRIP	EQU	56		; LIFE OF RIP ENTRY (REMEMBER TICK RATE IS
				; 3 HRS, SO THIS MEANS 7 DAYS)

TTL	DB	10
ICMPTTL	DB	10		; TTL ON MSGS

INC4    MACRO   X

	ADD     WORD PTR X,1
	ADC     WORD PTR X&+2,0
	ENDM

ADD4    MACRO   X,Y

	ADD     WORD PTR X,Y
	ADC     WORD PTR X&+2,0

	ENDM
;
SUB4    MACRO   X,Y

	SUB     WORD PTR X,Y
	SBB     WORD PTR X&+2,0

	ENDM
;
;       COPY4 COPIES ALLOWING FOR BYTE AND WORD REVERSAL
;
COPY4   MACRO   X,Y

	MOV     AX,WORD PTR Y
	MOV     DX,WORD PTR Y&+2

	XCHG    AH,AL
	XCHG    DH,DL

	MOV     WORD PTR X&+2,AX
	MOV     WORD PTR X,DX

	ENDM
;
;
;       COPY2 COPIES ALLOWING FOR BYTE REVERSAL
;
COPY2   MACRO   X,Y

	MOV     AX,WORD PTR Y
	XCHG    AH,AL
	MOV     WORD PTR X,AX

	ENDM
;
;       CMP4 COMPARES 2 4-BYTE NUMBERS IN MACHINE ORDER
;
CMP4    MACRO   X,Y
	LOCAL   EXIT

	MOV     AX,WORD PTR Y&+2
	CMP     WORD PTR X&+2,AX        
	JNE     EXIT
;
;       HI ORDER THE SAME - COMPARE LOW ORDER
;
	MOV     AX,WORD PTR Y
	CMP     WORD PTR X,AX   
EXIT:
	ENDM

;
;       CMP4X COMPARES ALLOWING FOR BYTE REVERSAL - 1ST OPERAND IS IN
;       MACHINE ORDER, SECOND IN HI-LO ORDER
;

CMP4X   MACRO   X,Y
	LOCAL   EXIT

	MOV     AX,WORD PTR Y
	XCHG    AH,AL
	CMP     WORD PTR X&+2,AX        
	JNE     EXIT
;
;       HI ORDER THE SAME - COMPARE LOW ORDER
;
	MOV     AX,WORD PTR Y&+2
	XCHG    AH,AL
	CMP     WORD PTR X,AX   
EXIT:
	ENDM

;
;       MOV4 MOVES 4 BYTES
;
MOV4    MACRO   X,Y

	MOV     AX,WORD PTR Y
	MOV     WORD PTR X,AX

	MOV     AX,WORD PTR Y&+2
	MOV     WORD PTR X&+2,AX

	ENDM
;
;
;       MOV2 MOVES 2 BYTES
;
MOV2    MACRO   X,Y

	MOV     AX,WORD PTR Y
	MOV     WORD PTR X,AX

	ENDM
;
BUFFSTRUC       STRUC

BUFFCHAIN       DW      0
BUFFLEN         DW      0
BUFFPTR         DW      0
BUFFDATA        DW      594 DUP (0)

BUFFSTRUC       ENDS


NEIGHBOUR	STRUC

NBOURIPADDR	DB	4 DUP (0)
NBOURFLAG	DB	0		; OLD/NEW FLAG

NEIGHBOUR	ENDS

NEIGHBOURS		DB	10 * TYPE NEIGHBOUR DUP (0)

NUMBEROFNEIGHBOURS	DW	0

	EVEN

		DW      128 DUP (0)
STACKTOP        LABEL   WORD

HEADER          DB      NAMESTRING,' IP Router/Gateway Version 3.0'
CALLSIGN        DB      HEADER+60-$ DUP (20H)
LOGFLAG         DB      0,' '                   ; DEFAULT = LOGGING OFF
DATE            DB      'DD/MM/YY  '
TIME            DB      'HH:MM:SS'

FILLER1         DB      80 DUP (205)

TRAILER         DB      80 DUP (205)


KBBUFF          DB      84 DUP (0)
		DB      'ESC=DISC  F1=MON-O'
MONOFFON        DB      'N   F2=MCOM-O'
MCOMOFFON       DB      'N   F3=MMASK-'
MMASKVAL        DB      'FFFF'    
		DB      6 DUP (20H)
LASTRTM         DB      'LAST 00:00 '
THISRTM         DB      'THIS 00:00'

SCREENBUFFERLEN EQU     15*80*12

	EVEN


SCREENBUFFER    DB      SCREENBUFFERLEN-80 DUP (20h)    ; SCROLL BUFFER
BOTTOMLINE      DB      80 DUP (0)      ; OUTPUT LINE

COLOUR          DW      14              ; ATTRIBUTE
NORMATTR	DW	14

KBPTR           DW      OFFSET KBBUFF

DISCONNMSG      DB      CR,'*** DISCONNECTED',CR

SCROLLPOINTER   DW      OFFSET BOTTOMLINE-(19*80)

CHAR    DW      0
BPQPORT DB      65

LASTSEC DB      0

LOGNAME DB      'LOGXXXX.XXX',0

LOGHANDLE       DW      0

MMCHAR          DB      '$'
MMCHAR1         DB      0
MMCHAR2         DB      0
		DB      20H

STOPMSG         DB      ' Program Terminated',0dh,0ah
lstopmsg        EQU     $-stopmsg

RTMFLAG         DB      0               ; RTM RUNNING
RTMSTART        DW      0               ; START TIME

D10             DW      10
D182            DW      182
D60             DW      60

CANTROUTECOUNT  DD      0
ARPFULLCOUNT    DD      0
BADIPSUM        DD      0               ; IP CHECKSUM ERRORS
BYTESSENT	DD	0
FRAMESSENT	DD	0
MINSACTIVE	DD	0

MYCALL          DB      7 DUP (0)

ROUTEENTRY      STRUC
;
;       INITIALLY A STATIC STRUCTURE, BUILT FROM THE CONFIGURATION FILE
;
NETWORK         DD      ?               ; NETWORK 
SUBNET          DD      ?               ; SUBNET MASK
GATEWAY         DD      ?               ; GATEWAY IP ADDRESS
PORT            DB      ?               ; POINTER TO PORT ENTRY
RTYPE           DB      ?               ; TYPE (NETROM/VC/DG/ETH)
METRIC          DB      ?               ; FOR RIP 
ROUTEINFO	DB	?		; TYPÅ (LEARNED¬ SYSOP¬ ETC)
ROUTECHANGED    DB      ?               ; CHANGED RECENTLY FLAG
RIPTIMOUT       DB      ?               ; FOR REMOVING ACTIVE ENTRIES
GARTIMOUT       DB      ?               ; FOR REMOVING DEAD ENTRIES
FRAMECOUNT      DD      ?               ; FRAMES SENT TO THIS NETWORK
;
ROUTEENTRY      ENDS

LEARNED		EQU	01H
LOCKED		EQU	02H

DEFAULTGATEWAY  DD      0
		DD      0               ; SUBNET
DEFAULTADDR     DD      0
		DB      0,0,0,0,0,0,0
		DD      0

MAXROUTES       EQU     128

ROUTETAB	DB      (MAXROUTES+1) * TYPE ROUTEENTRY DUP (0)

ENDOFROUTES     LABEL   BYTE

ROUTEENTRYLEN   DW      TYPE ROUTEENTRY

NUMBEROFROUTES  DW      0

TEMPENTRY	DB	TYPE ROUTEENTRY DUP (0) ; USED TO BUILD LINE TO INSERT

TCPDATAMSG	DB	500 DUP (0)

;
;       ARP DATA
;
;       USED TO TRANSLATE IP ADDRESSES TO ETHERNET ADDDRESSES
;
ARPDATA STRUC
;
;       KEEP IP ADDR AT FRONT
;
TARGETIPADDR    DB      4 DUP (?)
TARGETHWADDR    DB      7 DUP (?)       ; ETHERNET ADDR
ARPVALID        DB      ?               ; NONZERO IF ADDRESS HAS BEEN RESOLVED
ARPTIMER        DW      ?               ; TIMEOUT AFTER 5 MINS
ARPINTERFACE    DB      0               ; INTERFACE TO USE
ARPTYPE         DB      0               ; NETROM/VC/DG/ETH
ARP_Q           DW      0               ; CHAIN OF DATAGRAMS WAITING FOR RESOLUTION

ARPDATA ENDS

MAXARP          EQU     50


ARPTABLE        LABEL	BYTE

		DB	1,2,3,4
		DB	0,0,1BH,44H,55H,66H,0
		DB	1
		DW	30*60
		DB	0
		DB	'E'
		DW	0

		DB      MAXARP * TYPE ARPDATA DUP (0) 

ARPTABLELEN     EQU     $-ARPTABLE
	
SAVE		DW      0

IP_Q            DW      0               ; QUEUE OF MESSAGES FOR ROUTER
FREE_Q          DW      0

VERFLAG		DB	1		; VERSION 1 OR 2 FLAG FOR RIP MSGS
PHSUM           DW      0

FILEIT          DB      0               ; FILE/DISPLAY SWITCH FOR PRINTIT


PORT_NO         DB      0               ; Received port number 0 - 256
AX25_CTRL	DB	0		; RECEIVED CONTROL BYTE

FRAGFLAG	DW	0
FRAGSAVE	DW	0

BCFLAG		DB	0		; SET IF CURRENT MESSAGE IS A BROADCAST

RIPENTRY	DB	4 DUP (0)
RIPMASK		DB	0
RIPMETRIC	DB	0

DUFF00          DW      0
DUFF02          DW      0

ICMPHEADER	DB	8 DUP (0)
FIRST8		DB	8 DUP (0)	; FIRST 8 BYTES OF USER DATAGRAM;

;       DISPLAY SUPPORT
;
;
;       BASIC LINK LEVEL MESSAGE BUFFER LAYOUT
;
MESSAGE STRUC

MSGCHAIN        DW      ?               ; CHAIN WORD
MSGPORT         DB      ?               ; PORT 
MSGLENGTH       DW      ?               ; LENGTH

MSGPID          DB      ?               ; PROTOCOL IDENTIFIER
MSGDATA         DB      ?               ; START OF LEVEL 2 MESSAGE
;
MESSAGE ENDS
;
NULL            EQU     00H
CR              EQU     0DH
LF              EQU     0AH
NETROM_PID      EQU     0CFH
NODES_SIG       EQU     0FFH
;
PORT_MSG        DB      ' Port=',NULL
ARP_MSG         DB      0DH,'I ARP:',NULL
IP_MSG          DB      0DH,'I IP:',NULL

TO_MSG          DB      ' to ',NULL
TTL_MSG         DB      ' ttl=',NULL

BADLENMSG       DB      ' Invalid length',0

WINDOW          DB      ' w=',0
 
;
;       IP AND TCP BITS
;
IPMSG   STRUC
;
;       FORMAT OF IP HEADER
;
;       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)
;

VERLEN          DB      0               ; 4 BITS VERSION, 4 BITS LENGTH
TOS             DB      0               ; TYPE OF SERVICE
IPLENGTH        DW      0               ; DATAGRAM LENGTH
IPID            DW      0               ; IDENTIFICATION
FRAGWORD        DW      0               ; 3 BITS FLAGS, 13 BITS OFFSET
IPTTL           DB      0
IPPROTOCOL      DB      0               ; HIGHER LEVEL PROTOCOL
IPCHECKSUM      DW      0               ; HEADER CHECKSUM
IPSOURCE        DD      0
IPDEST          DD      0
;

IPMSG           ENDS

;       FORMAT OF TCP HEADER WITHIN AN IP DATAGRAM
;
;       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)
;

TCPSTRUC        STRUC

TCPSOURCEPORT   DW      0
TCPDESTPORT     DW      0

SEQNUM          DD      0
ACKNUM          DD      0

TCPCONTROL      DB      0       ; 4 BITS DATA OFFSET 4 RESERVED
TCPFLAGS        DB      0       ; (2 RESERVED) URG ACK PSH RST SYN FIN

TCPWINDOW       DW      0
CHECKSUM        DW      0
URGPTR          DW      0
;
;       OPTIONS AND/OR DATA MAY FOLLOW
;
TCPOPTIONS      DB      4 DUP (0)

TCPSTRUC        ENDS
;
;       TCPFLAGS BITS
;
FIN     EQU     1B
SYN     EQU     10B
RST     EQU     100B
PSH     EQU     1000B
ACK     EQU     10000B
URG     EQU     100000B
;
;       ICMP MESSAGE STRUCTURE
;
ICMPMSGS        STRUC
;
;       FORMAT OF ICMP HEADER WITHIN AN IP DATAGRAM
;
;       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)
;

ICMPTYPE        DB      0
ICMPCODE        DB      0
ICMPCHECKSUM    DW      0

ICMPID          DW      0               ; IN ECHO REQ/REPLY
ICMPSEQUENCE    DW      0
;
;       DATA MAY FOLLOW
;

ICMPMSGS        ENDS
;

;
;       UDP MESSAGE STRUCTURE
;
UDPMSGS         STRUC
;
;       FORMAT OF UDP HEADER WITHIN AN IP DATAGRAM
;
;       NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)
;

SOURCEPORT      DW      0
DESTPORT        DW      0
UDPLENGTH       DW      0
UDPCHECKSUM     DW      0
;
;       DATA FOLLOWS
;
UDPDATA         DB      0

UDPMSGS         ENDS
;
;
;       ARP REQUEST (AX.25)
;
ARPHEADER       DW      0
		DW      46
		DW      OFFSET ARPREQUESTMSG

ARPREQUESTMSG   LABEL   BYTE

QSTCALL         DB      'Q'+'Q','S'+'S','T'+'T',40h,40h,40h,11100000B; QST IN AX25
MYADDR1         DB      7 DUP (0)       ; AX25 SOURCE   
		DB      03H             ; UI

		DB      0CDH            ; TYPE
		DB      0,3
		DB      0,0CCH
		DB      7,4             ; LENGTHS
		DB      0,1             ; REQUEST
MYADDR2         DB      7 DUP (0)
MYIPADDR        DD      0
		DB      7 DUP (0)       ; ADDRESS WE WANT
ARPIPTARGET     DB      44,131,4,18     ; 4 DUP (0)

;
BROADCAST	DB	6 DUP (0FFH)	; MAC BROADCAST ADDR

EARPFRAME	DB      0,1		; HW TYPE
		DB      8,0		; PROTO 
		DB      6,4             ; LENGTHS
		DB      0,1             ; REQUEST
MYETHADDR       DB      4,5,6,7,8,9 ; 6 DUP (0)
MYETHIPADDR     DD      0
		DB      6 DUP (0)       ; ADDRESS WE WANT
EARPIPTARGET    DB      4 DUP (0)

NEXTIPIDENT	DW	1

;
;       ARP PACKET IN LINK FORMAT (WITH ETHERNET HEADER)
;
ARP             STRUC

ARPTARGET       DB      7 DUP (0)
ARPSOURCE       DB      7 dup (0)

		DB      03H             ; UI

		DB      0CDH            ; TYPE

HWTYPE          DB      0,3             ; AX.25
		DB      0,0CCH          ; PID

HWADDRLEN       DB      7
IPADDRLEN       DB      4

ARPOPCODE       DW      200H            ; REQUEST/REPLY

SENDHWADDR      DB      7 DUP (0)
SENDIPADDR      DB      4 DUP (0)

REPLYHWADDR     DB      7 DUP (0)
REPLYIPADDR     DB      4 DUP (0)

ARP     ENDS
;
;       ARP REPLY PACKET
;
ARPREPLYMSG     DW      0
		DW      46
		DW      OFFSET ARPREPLYBUFFER

ARPREPLYBUFFER  ARP     <>

SECS		DB	60
RIPTIM		DB	1

TEMP            DB      7 DUP (0)       ; SWAP FIELD FOR IP AND ETHER ADDRESSES
;
TXBUFFER        DW      0               ; CURRENT TX BUFFER ADDR

TCP_MSG         DB      0DH,'  TCP:',0
UDP_MSG         DB      0DH,'  UDP:',0

LEN             DB      ' Len: ',0
PROT            DB      ' Prot: ',0
SEQTEXT         DB      ' SEQ: ',0
ACKTEXT         DB      'ACK: ',0
WINDTEXT        DB      'WIN: ',0
SYNTEXT         DB      'SYN ',0
FINTEXT         DB      'FIN ',0
RSTTEXT         DB      'RST ',0
PSHTEXT         DB      'PSH ',0
ACKTEXT1        DB      'ACK ',0
URGTEXT         DB      'URG ',0

UDPDESTPORT     DW      0

ICMP_MSG        DB      0DH,'  ICMP:',0

ECHOMSG         DB      'Echo Request ',0
ECHOREPLY       db      'Echo Reply ',0
FROMMSG		DB	'from ',0
ECHOSENT	DB	'Echo Request sent',cr,0

BADSUM          DB      ' CHECKSUM ERROR ',0

RSPF_MSG        DB      0DH,'  RSPF:',0

RRH_MSG         DB      'Router-Router Hello ',0

;-----------------------------------------------------------------------------;
;          Parameter area for received frame                                  ;
;-----------------------------------------------------------------------------;


VERSION_NO      DB      0               ; Version 1 or 2       1,2
POLL_FINAL      DB      0               ; Poll or Final ?      P,F
COMM_RESP       DB      0               ; Command or Response  C,R
FRAME_TYPE      DB      0               ; Frame Type           UI etc in Hex
PID             DB      0               ; Protocol ID
FRAME_LENGTH    DW      0               ; Length of frame      0 - 65...
NR              DB      0               ; N(R) Flag
NS              DB      0               ; N(S) Flag
INFO_FLAG       DB      0               ; Information Packet ? 0 No, 1 Yes
OPCODE          DB      0               ; L4 FRAME TYPE
FRMRFLAG        DB      0
PROTOCOL        DB      0               ; IP PROTOCOL
; 
MMASK           DW      0FFFFH

MCOM            DB      0
TRACEFLAG       DB      1
MALL            DB      1
HEADERLN        DB      1
;
;       HDLC COMMANDS (WITHOUT P/F)
;
UI      EQU     3
SABM    EQU     2FH
DISC    EQU     43H
DM      EQU     0FH
UA      EQU     63H
FRMR    EQU     87H
RR      EQU     1
RNR     EQU     5
REJ     EQU     9


PFBIT   EQU     10H             ; POLL/FINAL BIT IN CONTROL BYTE

FRAGBUFFER	DB	512 DUP (0)
FRAGPTR		DW	0

LEARNFLAG	DB	'Y'		; SET TO 'N' TO DISABLE LEARNING

ETHINT		DB	125		; INTERRUPT FOR ODI DRIVER

ACCUM           DW      2 DUP (0)
CONVFLAG        DB      0
SUPPRESS        DB      0               ; ZERO SUPPRESS FLAG

DECNUM  DB      5 DUP (0)

W18	DW	18

W10     DW      10
;
;       POWERS OF 10 - 8 4-BYTE NUMBERS FROM 100,000,000 TO 10
;
POWERS  DD      100000000
	DD      10000000
	DD      1000000
	DD      100000
TENK    DD      10000
	DD      1000
WORD100 DD      100
DWORD10 DD      10
;
AX25CALL        DB      7 DUP (0)       ; WORK AREA FOR AX25 <> NORMAL CALL CONVERSION
NORMCALL        DB      10 DUP (0)      ; CALLSIGN IN NORMAL FORMAT
NORMLEN         DW      0               ; LENGTH OF CALL IN NORMCALL    

	INCLUDE ENVIRON.ASM


CONFIGHANDLE    DW      0


;
;       CONFIG FILE PROCESSING ROUTINE
;
FILENAME        DB      'IPGATE.CFG',0

READBLOCK       DB      128 DUP (0)
READCOUNT       DW      128
READPOINTER     DW      OFFSET READBLOCK
;
KEYWORD         DB      20 DUP (0)
PARAMS          DB      40 DUP (0)


PARAMPOINTER    DW      0

KEYIPADDRESS    DB      'IPADDRESS '
KEYETHIPADDR    DB      'ETHIPADDR '
KEYIPDEST       DB      'IPDEST    '
KEYDRIVER       DB      'DRIVER    '
KEYROUTE        DB      'ROUTE     '
KEYARP          DB      'ARP       '
KEYRIP		DB	'RIP       '
KEYDEFAULT	DB	'DEFAULT   '
KEYLEARN	DB	'LEARN     '
KEYTTL		DB	'TTL       '
; 
OPENERROR       DB      CR,'Configuration file IPGATE.CFG not found',0
INVALIDMSG      DB      CR,'Following config record not recognised:',cr,0
ROUTEERROR      DB      CR,'Invalid Gateway Address in ROUTE',CR,0

LENGTHOFROUTE   DW      TYPE ROUTEENTRY

VALERROR        DB      0
INITERROR       DB      0
	
HANDLE		DW	0

ARPINITPTR      DW      ARPTABLE

DMPNAME DB      'IPGATE.DMP',0
TABNAME DB      'IPGATE.CFG',0

DHANDLE DW      0
FILEBUFFER      DB      80 DUP (0)

CRLF	    DB      0DH,0AH

TEN	     DB      10
;
NEWVALUE        DW      0
WORD10          DW      10
WORD16          DW      16

UDPSOURCE       DB      0,0,0,0

STATSMINS	DB	'Time Active (Mins)  ',0
STATBUFFS       DB      'Buffers cur/min     ',0        
STATSUMERROR    DB      'IP Checksum Errors  ',0
STATARPFULL     DB      'ARP Table fulls     ',0
STATCANTROUTE   DB      'Unknown Route Count ',0
STATSFRAMES	DB	'Frames Transmitted  ',0
STATSBYTES	DB	'Bytes Transmitted   ',0
STATSROUTES	DB	'Known Routes        ',0

;
;       BUFFER POOL
;
PACKETBUFFERLEN         EQU     400
NUMBEROFRXBUFFERS       EQU     50

TCPRXBUFFERS    DB      NUMBEROFRXBUFFERS * PACKETBUFFERLEN DUP (0)

BUFFERCOUNT     DB      0
MINBUFFS        DB      0FFH

BUFFER          DW      0               ; CURRENT BUFFER

IPDATA	ENDS

CODE    SEGMENT PUBLIC 'CODE'

	EXTRN	SYSTEM:BYTE,REFRESH:BYTE,BWFLAG:BYTE,CLOSETERM:NEAR,STOPNODE:NEAR

	IF	NEEDTCP

	EXTRN	PROCESS_TCP_MSG:NEAR	; IN BPQTCP
	EXTRN	TCPINIT:NEAR,TCPBG:NEAR

	ENDIF
;
; Main background routine - searching first for keyboard and then for Comms characters
; keyboard characters are xmitted up the Comms
; RX comms characters are written to the screen
;
IPGATEBG:

	MOV	AX,IPDATA
	MOV	DS,AX
	MOV	ES,AX
;
;	CHECK KEYBOARD IF WE ARE ACTIVE SYSTEM
;
	CMP	SYSTEM,'I'
	JNE	BG1			; NOT OURS

	CMP	REFRESH,0
	JE	BG
;
;	BACK ON DISPLAY
;
	MOV	REFRESH,0
	CALL	STOPSCROLL

BG:

	MOV     AH,1
	INT     16H
	JZ      BG1                     ; NO KEYBOARD CHAR
;
	MOV     AH,0
	INT     16H                     ; GET THE CHAR

	CMP     AL,19H                  ; CTRL/Y
	JNE     KB000
	JMP     CLOSEDOWN

KB000:

	OR      AL,AL                   ; TWO KEY PAIR?
	JNZ     NORMKEYBD               ; NO

	MOV     AL,AH
;
	CALL    SPECIALKEY
	JMP SHORT BG1

NORMKEYBD:

	CALL    WRKB

BG1:

	CMP	BYTE PTR MYIPADDR,0
	JE	BG1_02
;
	CALL    POLLFORMSGS             ; PROCESS STUFF FROM SWITCH

	MOV     SI,OFFSET IP_Q
	CALL    Q_REM
	JZ      NOWTTOROUTE

	CALL    ROUTEFRAME

BG1_02:
NOWTTOROUTE:

	IF	NEEDTCP
	CALL	TCPBG
	ENDIF
;
;       REFRESH CLOCK DISPLAY IF SECOND HAS CHANGED
;
	MOV     AH,2CH
	INT     21H

	CMP     DH,LASTSEC
	JE      BGRET                   ; SECOND NOT CHANGED

NEWSEC:

	MOV     LASTSEC,DH

	CALL    REFTIME                 ; UPDATE CLOCK ON SCREEN
;
	CALL    TIMER_TICK

CHECKCON:
		
	CALL    REFRESHTL

BGRET:

	RET

OPENLOG:

	MOV     DX,OFFSET LOGNAME
	MOV     AL,2
	MOV     AH,3DH
	INT     21H

	JNC     OPENOK

	MOV     AH,3CH                  ; CREATE
	MOV     CX,0
	INT     21H

OPENOK:

	MOV     LOGHANDLE,AX

	MOV     BX,AX
	MOV     CX,0
	MOV     DX,0
	MOV     AL,2                    ; SKIP TO EOF
	MOV     AH,42H
	INT     21H

	RET
;


SPLBS:
	CALL    KBBSPACE
	RET

SPLCUR:
	CALL    CUR
	CALL    WRITECURSORADDR
	RET

SPLINS:
	CALL    INSRT
	CALL    REFRESHBL
	RET

SPLDEL:
	CALL    DELET
	CALL    REFRESHBL
	RET


ENDKEY:

	CMP     SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JE      NOSCROLL

	MOV     SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JMP     STOPSCROLL


SPECIALKEY:                             ; PROCESS EXTENDED CODE KEYS

	CMP	AL,68
	JNE	NOTF10

	XOR	SYSTEM,'I'
	MOV	REFRESH,1

	RET

NOTF10:

	CMP     AL,49H
	JE      PGUP

	CMP     AL,51H
	JE      PGDWN

	CMP     AL,4FH
	JE      ENDKEY

	CMP     AL,72
	JE      LINEUP

	CMP     AL,75
	JE      SPLBS                   ; CUR LEFT

	CMP     AL,77
	JE      SPLCUR

	CMP     AL,80
	JE      LINEDOWN

	CMP     AL,82
	JE      SPLINS

	CMP     AL,83
	JE      SPLDEL

	CMP     AL,59
	JE      F1

	CMP     AL,61
	JE      F3

NOSCROLL:

	RET

PGUP:

	CMP     SCROLLPOINTER,OFFSET SCREENBUFFER+15*80
	JB      NOSCROLL

	SUB     SCROLLPOINTER,15*80
	JMP SHORT SCROLLOK

LINEUP:

	CMP     SCROLLPOINTER,OFFSET SCREENBUFFER+80
	JB      NOSCROLL

	SUB     SCROLLPOINTER,80
	JMP SHORT SCROLLOK

PGDWN:

	CMP     SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JE      NOSCROLL

	ADD     SCROLLPOINTER,15*80

	CMP     SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JE      STOPSCROLL

SCROLLOK:

	MOV     COLOUR,14
	CALL    REFSCROLL
	RET

LINEDOWN:

	CMP     SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JE      NOSCROLL

	ADD     SCROLLPOINTER,80

	CMP     SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JE      STOPSCROLL

	MOV     COLOUR,14
	CALL    REFSCROLL
	RET

STOPSCROLL:
;
;       AT END OF BUFFER, SO CHANGE COLOUR
;
	MOV     COLOUR,3                ; NORMAL
	CALL    REFSCROLL

	RET

F1:
	XOR     TRACEFLAG,1
	MOV     AL,TRACEFLAG
	MOV     DI,OFFSET MONOFFON
	CALL    SETMON
	RET

F3:
	CALL    GETNEWMMASK
	RET

GETNEWMMASK:

	MOV     AH,0
	INT     16H                     ; GET NEXT CHAR

	MOV     MMCHAR1,AL

	MOV     AH,0
	INT     16H                     ; GET NEXT CHAR

	MOV     MMCHAR2,AL

	MOV     SI,OFFSET MMCHAR
	CALL    GETVALUE

	MOV     AX,NEWVALUE
	MOV     MMASK,AX

	MOV     DI,OFFSET MMASKVAL
	CALL	HEXOUT
	MOV	AL,BYTE PTR MMASK+1
	CALL    HEXOUT

	CALL    REFRESHBL
	RET




SETMON:
;
;       SET ON OR OFF ON STATUS LINE
;
;       AL = FLAG, DI=STATUS FIELD
;
	TEST    AL,1
	MOV     AX,'FF'

	JZ      SETMONOFF

	MOV     AX,' N'

SETMONOFF:

	STOSW
	CALL    REFRESHBL
	RET


WRKB:
	CMP     AL,20H
	JL      KBCTRL
KBNORM:
	MOV     DI,KBPTR
	MOV     [DI],AL

	MOV     BL,7
	MOV     AH,14
	INT     10H

	CALL    CUR
	RET


KBCTRL:

	CMP     AL,7
	JE      KBNORM                  ; SO WE CAN SEND BELL

	CMP     AL,8
	JE      KBBSPACE

	CMP     AL,0DH
	JE      KBCR

	CMP     AL,12                   ; CTRL/L
	JE      LOGTOG

	CMP     AL,1AH                  ; CTRL/Z
	JE      KBNORM                  ; FOR MBX TERMINATOR

	RET

KBBSPACE:

	CALL    CUL
	CALL    WRITECURSORADDR
	RET

LOGTOG:

	XOR     LOGFLAG,'L'             ; FLIP LOGGING FLAG

	CALL	CLOSELOG

	CMP	LOGFLAG,'L'
	JNE	DONTOPEN

	CALL	OPENLOG

DONTOPEN:

	CALL    REFRESHTL
	RET

KBCR:
;
;       PASS TO COMMAND HANDLER
;
	MOV     SI,OFFSET KBBUFF
	MOV     CX,KBPTR
	SUB     CX,SI

	LODSW

	OR      AX,2020H
	CMP     AX,' r'
	JNE     NOTR

	CALL    DISPROUTES

	JMP     KBEND

NOTR:

	CMP     AX,' a'
	JNE     NOTA

	CALL    DISPARP

	JMP     KBEND

NOTA:

	CMP     AX,' s'
	JNE     NOTS

	CALL    DISPSTATS

	JMP     KBEND

NOTS:

	CMP     AX,' t'
	JNE     NOTT

	CALL	SENDRIP

	JMP     KBEND

NOTT:

	CMP     AX,' n'
	JNE     NOTN

	CALL	DISPRIP

	JMP     KBEND

NOTN:

	CMP     AX,' d'
	JNE     NOTD

	CALL   DUMPCODE

	JMP     KBEND

NOTD:

	CMP     AX,'ip'
	JNE     NOTP

	CALL	SENDPING

	JMP     KBEND

NOTP:

	MOV     SI,OFFSET KBBUFF
	CALL    PROCESSCOMMAND

	JMP     KBEND

NOT?:

KBEND:
KBCR01:

	MOV     KBPTR,OFFSET KBBUFF

	MOV     DI,OFFSET KBBUFF
	MOV     CX,40
	MOV     AX,2020H
	REP STOSW

	CALL    REFRESHBL
	RET

;
;
PUTCRLF:

	MOV     DX,OFFSET CRLF
	MOV     BX,LOGHANDLE
	MOV     CX,2
	MOV     AH,40H
	INT     21H

	RET

CLOSEDOWN:

	CALL	CLOSETERM
	CALL	CLOSEIP

	CALL	STOPNODE

	MOV	AX,4C00H
	INT	21H
;

CLOSEIP:

	MOV	AX,IPDATA
	MOV	DS,AX
	MOV	ES,AX

	CALL	DUMPTABLES
	CALL	CLOSELOG

	MOV     AL,BPQPORT
	MOV     AH,1

	MOV     DX,0                    ; APPL MASK
	MOV     CX,0

	CALL    NODE                    ; CLEAR APPL MASK TO STOP MONITORING
;
	RET
;
CLOSELOG:

	CMP	LOGHANDLE,0
	JE	NOTOPEN

	MOV     BX,LOGHANDLE
	MOV     AH,3EH
	INT     21H                     ; CLOSE LOG FILE

	MOV	LOGHANDLE,0

NOTOPEN:

	RET

GETTIME:
	MOV     AH,2AH
	INT     21H

	MOV     AL,DL
	CBW
	DIV     TEN
	ADD     AX,3030H
	MOV     WORD PTR DATE,AX
	MOV     AL,DH
	CBW
	DIV     TEN
	ADD     AX,3030H
	MOV     WORD PTR DATE+3,AX

	SUB     CX,1900
	MOV     AL,CL
	CBW
	DIV     TEN
	ADD     AX,3030H
	MOV     WORD PTR DATE+6,AX

	MOV     AH,2CH
	INT     21H

	MOV     AL,CH
	CBW
	DIV     TEN
	ADD     AX,3030H
	MOV     WORD PTR TIME,AX

	MOV     AL,CL
	CBW
	DIV     TEN
	ADD     AX,3030H
	MOV     WORD PTR TIME+3,AX

	MOV     AL,DH
	CBW
	DIV     TEN
	ADD     AX,3030H
	MOV     WORD PTR TIME+6,AX

	RET

REFTIME:

	MOV     COLOUR,4
	CALL    GETTIME

	MOV     CX,20                   ; TIMESTAMP LENGTH
	MOV     SI,OFFSET DATE
	MOV     DX,62

	JMP SHORT REFCOM

REFRESHBL:

	MOV     COLOUR,4
	MOV     DH,22
	MOV     DL,0                    ; POSITION

	MOV     CX,239                  ; LENGTH
	MOV     SI,OFFSET TRAILER
	JMP SHORT REFCOM

REFSCROLL:

	MOV     DH,2
	MOV     DL,0

	MOV     CX,1600
	MOV     SI,SCROLLPOINTER

	JMP SHORT REFCOM

REFRESHTL:

	MOV     COLOUR,4

	CALL    GETTIME

	MOV     DX,0

	MOV     CX,160                  ; SCREEN LENGTH
	MOV     SI,OFFSET HEADER

REFCOM:

	CMP	BWFLAG,'M'
	JNE	REFLOOP

	MOV	COLOUR,7		; FORCE MONO MODE

REFLOOP:

	CMP	SYSTEM,'I'
	JNE	REFX			; SOMEONE ELSE HAS SCREEN

	MOV     AH,2
	MOV     BH,0
	INT     10H                     ; SET CURSOR

	LODSB

	PUSH    CX

	MOV     AH,9
	MOV     BX,COLOUR
	MOV     CX,1

	INT     10H

	POP     CX

	INC     DL
	CMP     DL,80
	JNE     DLOK

	INC     DH
	MOV     DL,0

DLOK:

	LOOP    REFLOOP

	CALL    WRITECURSORADDR

REFX:

	RET
; 
NORMCHR:

	
	CMP     AL,1AH
	JNE     NORMOK

	MOV     AL,20H                  ; TO KEEP FILE TIDY
NORMOK:
	CMP     CHAR,80
	JL      NORM000

	PUSH    AX
	PUSH    CX
	PUSH    SI
	CALL    RACK
	POP     SI
	POP     CX
	POP     AX

NORM000:
	MOV     DI,CHAR
	ADD     DI,OFFSET BOTTOMLINE
	STOSB

	CMP     SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JNE     NOPUT

	CALL    PUTDMA

NOPUT:
	INC     CHAR

	RET

RACK:
	CMP     LOGFLAG,'L'
	JNE     RACK30                  ; LOGGING OFF

	MOV     DX,OFFSET BOTTOMLINE    ; BOTTOM LINE
	MOV     BX,LOGHANDLE
	MOV     CX,80

	MOV     SI,OFFSET BOTTOMLINE+79
RACK10:
	CMP     BYTE PTR [SI],20H
	JNE     RACK20                          ; WRITE LINE

	DEC     SI
	LOOP    RACK10

	JMP     RACK30

RACK20:
	MOV     AH,40H
	INT     21H                             ; WRITE

	CALL    PUTCRLF

RACK30:
;
	MOV     CX,(SCREENBUFFERLEN-80)/2
	MOV     SI,OFFSET SCREENBUFFER+80
	MOV     DI,OFFSET SCREENBUFFER
	REP MOVSW

	MOV     CX,40
	MOV     AX,2020H
	REP STOSW                               ; CLEAR BOTTOM LINE

	MOV     CHAR,0
;
	CMP	SYSTEM,'I'
	JNE	NOSCROLLDMA

	CMP     SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JNE     NOSCROLLDMA

;       SCROLL DMA IMAGE
;
	MOV     AH,6
	MOV     AL,1
	MOV     CH,2
	MOV     CL,0
	MOV     DH,21
	MOV     DL,79
	MOV     BH,2

	INT     10H

NOSCROLLDMA:

	RET

WRITECURSORADDR:

	MOV     DX,KBPTR
	SUB     DX,OFFSET KBBUFF        ; GIVES CHAR

	MOV     DH,23

	MOV     AH,2
	MOV     BH,0
	INT     10H                     ; SET CURSOR

	RET
;
INSRT:                                  ; INSERT
	MOV     DI,KBPTR
	MOV     CX,OFFSET KBBUFF+80
	SUB     CX,KBPTR
	JCXZ    INSRTEX
INSRT0:
	DEC     CX
	JZ      INSRTEX                 ; ONLY ONE LEFT
	ADD     DI,CX
INSRT2:
	MOV     SI,DI
	DEC     SI
	STD
	REP MOVSB
	CLD
INSRTEX:
	MOV     BYTE PTR [DI],20H
	RET
;
DELET:                                  ; DELETE 
	MOV     DI,KBPTR
	MOV     CX,OFFSET KBBUFF+80
	SUB     CX,DI
	JCXZ    DELETEX

	DEC     CX
	JZ      DELETEX                 ; ONLY ONE LEFT
DELET2:
	MOV     SI,DI
	INC     SI
	REP MOVSB
DELETEX:
	MOV     BYTE PTR [DI],20H
	RET
;
NEWLNE:
	CMP     CHAR,0
	JE      NL010                   ; ALREADY AT START
;
	MOV     CX,80
	SUB     CX,CHAR
	JCXZ    NL010
NL000:
	MOV     AL,20H                  ; SPACE TO END OF LINE
	PUSH    CX
	CALL    NORMCHR
	POP     CX
	LOOP    NL000

	RET

NL010:
	CALL    RACK
	RET
;
NEWLINE:                                ; ERASE TO END OF LINE
	MOV     DI,CHAR
	ADD     DI,OFFSET BOTTOMLINE

	MOV     BYTE PTR [DI],20H
	MOV     CX,80
	SUB     CX,CHAR
	JE      DLNEEX                  ; END OF LINE

	MOV     AL,20H
	REP STOSB
DLNEEX:
	RET
;
CUR:                                    ; CURSOR RIGHT 
	CMP     KBPTR,OFFSET KBBUFF+80
	JE      CURRET

	INC     KBPTR
CURRET:
	RET
;
CUL:                                    ; CURSOR LEFT
	CMP     KBPTR,OFFSET KBBUFF
	JE      CURRET

	DEC     KBPTR
	RET
;

PUTDMA:
;
;	ONLY WRITE TO BOTTOM LINE OF SCREEN
;
	CMP	SYSTEM,'I'
	JNE	SKIPPUT			; SOMEONE ELSE HAS SCREEN

	PUSH	AX

	MOV	DX,CHAR
	MOV	DH,21

	MOV	AH,2
	MOV	BH,0
	INT	10H			; SET CURSOR
;
	POP	AX
;
;	PUT CHAR IN DMA SCREEN
;
	PUSH	CX

	MOV	AH,9
	MOV	BX,NORMATTR
	MOV	CX,1
	INT	10H

	POP	CX

SKIPPUT:

	RET
;
;
POLLFORMSGS:
;
;       LOOK FOR IP DATA (USES BPQ HOST MODE GET RAW FRAME CALL)
;
	CALL    IPGETBUFF
	JZ      NOBUFFERS

	PUSH    DI                      ; SAVE

	ADD     DI,12                   ; 6 BYTES HEADER, MAY ALSO NEED EXTRA
					; SPACE FOR NETROM HEADER

	PUSH    DI

	MOV     AH,11
	CALL    NODE

	POP     DI

	CMP     CX,0
	JE      NOMONITOR
;
;       POLL NODE FOR DATA
;
	POP     BX                      ; BUFFER HEADER

	MOV     BUFFLEN[BX],CX
	MOV     BUFFPTR[BX],DI

	CALL    PROCESSFRAME

	RET

NOMONITOR:

	POP     DI
	CALL    IPRELBUFF                 ; DONT NEED BUFFER

NOBUFFERS:

	MOV     AX,1000H
	INT     15H                     ; FOR DESQVIEW

	STI                             ; FOR IBM XT BIOS BUG

	RET

GETVALUE:
;
;       EXTRACT NUMBER (HEX OR DECIMAL) FROM INPUT STRING
;
	MOV     NEWVALUE,0

	MOV     AL,ES:[SI]
	CMP     AL,'$'                  ; HEX?
	JE      DECODEHEX

VALLOOP:
	MOV     AL,ES:[SI]
	CMP     AL,' '
	JE      ENDVALUE
	CMP     AL,'.'
	JE      ENDVALUE
	CMP     AL,'/'
	JE      ENDVALUE
;
;       ANOTHER DIGIT - MULTIPLY BY 10
;
	MOV	AX,NEWVALUE
	MUL     WORD10
	MOV     NEWVALUE,AX

	MOV     AL,ES:[SI]
	INC     SI
	SUB     al,'0'
	JC      DUFFVALUE
	CMP     AL,10
	JNC     DUFFVALUE

	MOV	AH,0
	ADD     NEWVALUE,AX

	JC      DUFFVALUE
	JMP     VALLOOP

DECODEHEX:

	INC     SI

HEXLOOP:

	MOV     AL,ES:[SI]
	CMP     AL,' '
	JE      ENDVALUE
;
;       ANOTHER DIGIT - MULTIPLY BY 16
;
	MOV     AX,NEWVALUE
	MUL     WORD16
	MOV     NEWVALUE,AX

	MOV     AL,ES:[SI]
	INC     SI
	SUB     al,'0'
	JC      DUFFVALUE
	CMP     AL,10
	JC      HEXOK
	SUB     AL,7
	CMP     AL,10
	JC      DUFFVALUE

	CMP     AL,16
	JNC     DUFFVALUE
HEXOK:
	MOV	AH,0
	ADD     NEWVALUE,AX
	JMP     HEXLOOP

ENDVALUE:
	CLC
	RET

DUFFVALUE:
	MOV     VALERROR,1
	STC
	RET


;
IP_INPUT:

	MOV     SI,BUFFPTR[BX]          ; IP HEADER 
;
;       GET DATAGRAM LENGTH (REMEMBER ETHERNET PADS TO 64 BYTES MIN)
;
	MOV     AX,IPLENGTH[SI]
	XCHG    AH,AL

	CMP     AX,BUFFLEN[BX]
	JBE     IPLENOK

	JMP     IP_DISCARD              ; MALFORMED LENGTH FIELD

IPLENOK:

	MOV     BUFFLEN[BX],AX

	ADD     SI,AX                   ; STICK ZERO BYTE ON END
	MOV     BYTE PTR[SI],0          ; IN CASE NEEDED FOR TCP CHECKSUM
	SUB     SI,AX
;
;       CHECK CHECKSUM
;
	MOV     SI,BUFFPTR[BX]
	MOV     CX,10                   ; ONLY ACCEPT MSGS WITH NO OPTIONS

	CALL    DO_CHECKSUM

	CMP     DX,0FFFFH
	JE      IPSUMOK

	INC4    BADIPSUM

	JMP     IP_DISCARD              ; SCRAP IT

IPSUMOK:
;
;	IF ON A VC INTERFACE, AND STATION ISNT IN ROUTE TABLE, ADD IT
;
	CMP	AX25_CTRL,UI
	JE	SKIP_AUTO		; NOT CONNECTED

	CMP	AX25_CTRL,0FFH
	JE	SKIP_AUTO		; LEAVE NETROM FOR NOW

	MOV     SI,BUFFPTR[BX]
	LEA	SI,IPSOURCE[SI]		; TO ADDR

	CALL	DO_AUTOROUTE

SKIP_AUTO:
;
;       SEE IF ADDRESSED TO OUR IP ADDR - IF NOT, PASS TO ROUTER CODE
; 
	MOV     SI,BUFFPTR[BX]
;
	MOV	BCFLAG,0

	CMP     WORD PTR IPDEST+2[SI],0H        ; EITHER 0 OR 255,255 ARE
;                                               ; USED FOR BROADCAST
	JE      BC_MSG
;
	CMP     WORD PTR IPDEST+2[SI],0FFFFH    ; BROADCAST?
	JNE     NOT_BC

BC_MSG:
;
	MOV	BCFLAG,1
	JMP     MSGFORME

NOT_BC:

	CMP4    MYIPADDR,IPDEST[SI]
	JE      MSGFORME
;
;       UPDATE TTL, AND IF NOT EXPIRED, RE-CHECKSUM AND ADD TO ROUTER QUEUE
;
	DEC     IPTTL[SI]
	JNZ	SUMANDSEND
;
;
;	CONVERT TO ICMP DEST UNREACHABLE
;
;	SI (AND BUFFPTR) POINTS TO IP HEADER
;
	MOV	DI,OFFSET ICMPHEADER
	MOV	AL,11			; TIME EXCEEDED
	STOSB
	MOV	AL,0			; SUBYTPE = TTL EXCEEDED
	MOV	CX,7
	REP STOSB			; CLEAR REST OF HEADER 
;
	CALL	SENDICMPREPORT

	RET

SUMANDSEND:
;
	MOV     IPCHECKSUM[SI],0

	MOV     CX,10                   ; LENGTH

	CALL    DO_CHECKSUM
	NOT     DX

	MOV     SI,BUFFPTR[BX]
	MOV     IPCHECKSUM[SI],DX

	MOV     DI,BX
	MOV     SI,OFFSET IP_Q
	CALL    Q_ADD

	RET


MSGFORME:
;
;       PROCESS ICMP (ECHO) AND MANAGEMENT MESSAGES ONLY
;
	MOV     SI,BUFFPTR[BX]          ; IP HEADER 
	
	MOV     AL,VERLEN[SI]           ; HEADER LENGTH (LOWER 4 BITS)
	AND     AX,0FH                  ; MASK
	ADD     AX,AX
	ADD     AX,AX                   ; *4
	ADD     BUFFPTR[BX],AX          ; ADJUST BUFFER
	SUB     BUFFLEN[BX],AX          ; AND SAVE FOR UDP/ICMP
;
;       SEE IF UDP, MGMT OR ICMP - IGNORE OTHERS
;
	CMP     IPPROTOCOL[SI],11H
	JE      IP_UDP                  ; FOR ROUTING

	CMP	BCFLAG,1
	JE	IP_DISCARD		; DONT HANDLE TCP OR ICMP BROADCASTS

	CMP	IPPROTOCOL[SI],6
	JE	IP_TCP			; TCP MSG

	CMP     IPPROTOCOL[SI],1	; ICMP
	JNE     IP_DISCARD

	CALL    IP_ICMP

	RET

IP_DISCARD:

	MOV     DI,BX

	CALL    IPRELBUFF
	RET

IP_UDP:
;
;       CALCULATE PSEUDO-HEADER CHECKSUM (EASIER HERE THAN ANYWHERE ELSE)
;
	MOV     DX,1100H                ; PROTOCOL (REVERSED)
	MOV     AX,BUFFLEN[BX]          ; TCP LENGTH
	XCHG    AH,AL
	ADD     DX,AX
	MOV     AX,WORD PTR IPSOURCE[SI]
	ADC     DX,AX
	MOV     AX,WORD PTR IPSOURCE+2[SI]
	ADC     DX,AX
	MOV     AX,WORD PTR IPDEST[SI]
	ADC     DX,AX
	MOV     AX,WORD PTR IPDEST+2[SI]
	ADC     DX,AX
	ADC     DX,0

	MOV     PHSUM,DX
;
	MOV4    UDPSOURCE,IPSOURCE[SI]  ; SAVE SOURCE ADDR

;       PASS TO UDP
;
	CALL    UDP_INPUT
	RET

IP_TCP:
;
;	CALCULATE PSEUDO-HEADER CHECKSUM (EASIER HERE THAN ANYWHERE ELSE)
;
	MOV	DX,600H			; PROTOCOL (REVERSED)
	MOV	AX,BUFFLEN[BX]		; TCP LENGTH
	XCHG	AH,AL
	ADD	DX,AX
	MOV	AX,WORD PTR IPSOURCE[SI]
	ADC	DX,AX
	MOV	AX,WORD PTR IPSOURCE+2[SI]
	ADC	DX,AX
	MOV	AX,WORD PTR IPDEST[SI]
	ADC	DX,AX
	MOV	AX,WORD PTR IPDEST+2[SI]
	ADC	DX,AX
	ADC	DX,0

	MOV	PHSUM,DX
;
;	PASS TO TCP
;
	MOV	BUFFER,BX		; SAVE MESSAGE BASE
;
;	CHECK TCP CHECKSUM
;
	MOV	SI,BUFFPTR[BX]
	MOV	CX,BUFFLEN[BX]

	INC	CX			; ROUND UP
	SHR	CX,1			; WORD COUNT

	CALL	DO_CHECKSUM

	ADD	DX,PHSUM
	ADC	DX,0
	
	CMP	DX,0FFFFH
	JE	TCP_SUM_OK

	JMP	BAD_SUM

TCP_SUM_OK:

	MOV	CX,BUFFLEN[BX]		; LENGTH LEFT
	MOV	SI,BUFFPTR[BX]		; DATA LEFT

	IF	NEEDTCP

	CALL	PROCESS_TCP_MSG
	RET

	ENDIF

	TEST	TCPFLAGS[SI],RST
	JZ	RESET_00

BAD_SUM:

	JMP	RELEASEBUFFER

RESET_00:
;
;	IF ACK IS SET, THEN THE OTHER END THINKS THERE IS A VALID SESSION -
;	SEND JUST RST, SEQ=SEG.ACK, ACK=0
;
;	IF ACK IS NOT SET (SO SYN SHOULD BE) ACK THE SYN, AND 
;	SET RST  (SEQ=0,ACK=SEG.SEQ+SEG.LEN)
;

	MOV	TCPCONTROL[SI],50H	; 5 WORDS (IN TOP 4 BITS)

	MOV	AX,TCPDESTPORT[SI]
	MOV	CX,TCPSOURCEPORT[SI]
	MOV	TCPDESTPORT[SI],CX
	MOV	TCPSOURCEPORT[SI],AX

	MOV	WINDOW[SI],0
	MOV	URGPTR[SI],0

	MOV	CHECKSUM[SI],0		; WILL FILL IN LATER
;
	TEST	TCPFLAGS[SI],ACK
	JZ	ACKNOTSET
;
;	ACK BIT IS SET
;
	MOV	TCPFLAGS[SI],RST
	MOV4	SEQNUM[SI],ACKNUM[SI]

	MOV	WORD PTR ACKNUM[SI],0
	MOV	WORD PTR ACKNUM+2[SI],0

	JMP SHORT RESET_COMMON

ACKNOTSET:

	MOV	TCPFLAGS[SI],RST+ACK

	MOV4	ACKNUM[SI],SEQNUM[SI]
	ADD	BYTE PTR ACKNUM+3[SI],1
	ADC	BYTE PTR ACKNUM+2[SI],0
	ADC	BYTE PTR ACKNUM+1[SI],0
	ADC	BYTE PTR ACKNUM[SI],0

	MOV	WORD PTR SEQNUM[SI],0
	MOV	WORD PTR SEQNUM+2[SI],0

RESET_COMMON:
;
;	SWAP ADDRESSES AND SEND BACK
;
;	SI POINTS TO TCP HEADER
;
	MOV	BUFFLEN[BX],20
;
;       CHECKSUM IT
;
;
;	DO PSEUDO HEADER FIRST
;
	SUB	SI,20			; TO IP HEADER
	
	MOV	DX,600H			; PROTOCOL (REVERSED)
	MOV	AX,20			; TCP LENGTH
	XCHG	AH,AL
	ADD	DX,AX
	MOV     AX,WORD PTR IPSOURCE[SI]
	ADC     DX,AX
	MOV     AX,WORD PTR IPSOURCE+2[SI]
	ADC     DX,AX
	MOV     AX,WORD PTR IPDEST[SI]
	ADC     DX,AX
	MOV     AX,WORD PTR IPDEST+2[SI]
	ADC	DX,AX
	ADC	DX,0

	MOV	PHSUM,DX

	MOV     SI,BUFFPTR[BX]
	MOV     CX,BUFFLEN[BX]

	INC     CX                      ; ROUND UP
	SHR     CX,1                    ; WORD COUNT

	MOV     CHECKSUM[SI],0

	CALL    DO_CHECKSUM

	ADD	DX,PHSUM
	ADC	DX,0
	
	NOT     DX

	MOV     SI,BUFFPTR[BX]
	MOV     CHECKSUM[SI],DX
;
	JMP	IPSUMANDSEND

CHUCKIT:

RELEASEBUFFER:

	MOV     DI,BX
	CALL    IPRELBUFF
	RET



UDP_INPUT:

	MOV     BUFFER,BX               ; SAVE MESSAGE BASE
;
;       CHECK CHECKSUM
;
	MOV     SI,BUFFPTR[BX]
	MOV     CX,BUFFLEN[BX]

	INC     CX                      ; ROUND UP
	SHR     CX,1                    ; WORD COUNT

	CALL    DO_CHECKSUM

	ADD     DX,PHSUM
	ADC     DX,0
	
	CMP     DX,0FFFFH
	JE      UDPCKSUMOK

	MOV     BX,BUFFER
	JMP     IP_DISCARD

UDPCKSUMOK:

	MOV     CX,BUFFLEN[BX]          ; LENGTH LEFT
	MOV     SI,BUFFPTR[BX]          ; DATA LEFT

	MOV     AX,DESTPORT[SI]
	XCHG    AH,AL
;
	CMP     AX,520                  ; RIP SOCKET
	JNE     NOTRIP

	CALL    PROCESS_RIP_MESSAGE

	MOV     BX,BUFFER
	JMP     IP_DISCARD

NOTRIP:
;
UDPDISCARD:

	MOV     BX,BUFFER
	JMP     IP_DISCARD


PROCESS_RIP_MESSAGE:
;
;       RIP MESSAGE
;
	LEA     SI,UDPDATA[SI]          ; TO RIP PACKET

	CMP     BYTE PTR 1[SI],1
	JE      VERSOK

	MOV	VERFLAG,1

	CMP     BYTE PTR 1[SI],'B'
	JE      BPQVERSOK

	MOV	VERFLAG,2

	CMP     BYTE PTR 1[SI],'b'	; NEW FORMAT BPQ
	JE      BPQVERSOK

	RET                             ; WRONG VERSION


VERSOK:
;
;	DONT PROCESS STANDARD RIP FOR NOW
;
	RET

BPQVERSOK:
;
;	BPQ VERSION OF RIP - BASICALLY THE SAME, BUT WITHOUT ALL THE GAPS!
;
	CMP     BYTE PTR [SI],2
	JE      RIPRESP

	RET                             ; DONT HANDLE REQUESTS YET

RIPRESP:
;
;	MAKE SURE IT IS FROM A VALID NEIGHBOUR
;
	PUSH	CX

	MOV	CX,NUMBEROFNEIGHBOURS
	JCXZ	IGNORERIP

	MOV	DI,OFFSET NEIGHBOURS

RIPCHKLOOP:

	CMP4	0[DI],UDPSOURCE
	JE	RIPOK

	ADD	DI,TYPE NEIGHBOUR
	LOOP	RIPCHKLOOP

IGNORERIP:

	POP	CX
	RET

RIPOK:

	POP	CX

	ADD     SI,4                    ; TO IP ADDR
	SUB     CX,12                   ; UDP AND RIP HEADERS
	
RIPOUTERLOOP:

	CMP     CX,6
	JAE     MORE_RIP

	JMP     RIPFINISHED

MORE_RIP:
;
;	EXTRACT NEXT ENTRY
;
	MOV	DI,OFFSET RIPENTRY
	MOVSW
	MOVSW

	CMP	VERFLAG,1
	JE	OLDRIP			; OLD SYSTEM
;
	SUB	CX,6			; ENTRY LENGTH
	MOVSB

	JMP SHORT GETMET

OLDRIP:

	SUB	CX,5			; ENTRY LENGTH
	MOV	AL,32			; MASK FOR FULL ADDR

	CMP	RIPENTRY+3,0
	JNE	NOTSUBNET
;
;	ITS A SUBNET ROUTE
;
	MOV	AL,24

NOTSUBNET:

	STOSB

GETMET:

	MOVSB			; COPY METRIC
;
;	SI NOW POINTS TO NEXT ENTRY
;
	PUSH	CX

	MOV     DI,OFFSET ROUTETAB
	MOV     CX,NUMBEROFROUTES

RIPFINDLOOP:

	MOV     AX,WORD PTR NETWORK+2[DI]
	CMP     AX,WORD PTR RIPENTRY+2
	JNE     RIPFINDNEXT
;
	MOV     AX,WORD PTR NETWORK[DI]
	CMP     AX,WORD PTR RIPENTRY
	JNE     RIPFINDNEXT
;
;       ENTRY FOUND - IS THIS THE ONE WE ARE ALREADY USING?
;
	CMP4    GATEWAY[DI],UDPSOURCE   ; WHERE THIS MSG CAME FROM
	JNE	RIP_DIFF

	JMP     REFRESH_RIP             ; WE ARE ALREADY USING THIS ROUTE

RIP_DIFF:
;
;       SEE IF THIS ONE IS BETTER
;
	MOV     AL,METRIC[DI]
	DEC     AL

	CMP     AL,RIPMETRIC           	; METRIC IN RIP MSG

	JA      UPDATE_ENTRY_J          ; THIS ONE IS BETTER
;
	JNE     NEXTRIPJMP		; WORSE
;
;       SAME AS CURRENT. IF IT IS GETTING TOWARDS TIMING OUT, USE THIS ONE
;	 !! NOT FOR NOW - MAY AFFECT STABILITY!!
;
;
;	CMP     RIPTIMOUT[DI],3
;	JBE     UPDATE_ENTRY            ; START USING THIS ONE
;
NEXTRIPJMP:

	JMP	NEXTRIPENTRY

UPDATE_ENTRY_J:

	JMP	UPDATE_ENTRY

RIPFINDNEXT:

	ADD     DI,TYPE ROUTEENTRY
;
	LOOP    RIPFINDLOOP
;
;       NOT FOUND, SO ADD
;
	CMP	RIPMETRIC,15

	JAE     NEXTRIPJMP            ; DONT ADD UNREACHABLE ROUTES

	CMP     NUMBEROFROUTES,MAXROUTES
	JE      NEXTRIPJMP            ; NO ROOM
;
;	COPY TO TEMPENTRY, THEN USE INSERT ROUTINE TO PUT IN IN RIGHT PLACE
;
	MOV	DI,OFFSET TEMPENTRY
	MOV	CX,TYPE ROUTEENTRY
	MOV	AL,0
	REP STOSB
	SUB	DI,TYPE ROUTEENTRY

	MOV4    NETWORK[DI],RIPENTRY

	MOV	AL,RIPMASK
;
	CALL	BITSTOMASK
;
	XCHG	DH,DL
	MOV     WORD PTR SUBNET[DI],DX
	XCHG	AL,AH
	MOV     WORD PTR SUBNET+2[DI],AX
;
	MOV4    GATEWAY[DI],UDPSOURCE   ; WHERE THIS MSG CAME FROM

	CMP	AX25_CTRL,0FFH
	JE	RIP_NET			; VIA NETROM
;
;	VIRTUAL CIRCUIT MODE
;
	MOV	AL,PORT_NO
	MOV     PORT[DI],AL

	MOV	RTYPE[DI],'V'
	JMP SHORT RIP_GOT_TYPE

RIP_NET:
;
	MOV	RTYPE[DI],'N'
	MOV	PORT[DI],0
;
RIP_GOT_TYPE:

	MOV	ROUTEINFO[DI],LEARNED

	MOV     AL,RIPMETRIC            ; INCOMING METRIC
	INC     AL
	CMP     AL,15
	JBE     MET_OK
;
;       HE IS TELLING US ITS UNREACHABLE - START DELETE TIMER
;
	MOV     METRIC[DI],16
	MOV     RIPTIMOUT[DI],0
;
	CMP     GARTIMOUT[DI],0
	JNE     NEXTRIPENTRY            ; TIMER ALREADY RUNNING
;
	MOV     GARTIMOUT[DI],4         ; SET WAITING TO DELETE
;
	JMP SHORT PUTINLIST

MET_OK:

	MOV     METRIC[DI],AL

	MOV     RIPTIMOUT[DI],MAXRIP
	MOV     GARTIMOUT[DI],0         ; IN CASE WAITING TO DELETE
;
;	NOW INSERT
;
PUTINLIST:

	PUSH	SI
;
;	IF ENTRY IS A SUBNET ROUTE TO OUR SUBNET, IGNORE IT
;
	PUSH	SI

	MOV	SI,OFFSET TEMPENTRY
	MOV     AX,WORD PTR MYIPADDR+2
	AND	AX,WORD PTR SUBNET+2[SI]
	CMP     AX,WORD PTR NETWORK+2[SI]
	JNE	NOTOURNET1
;
	MOV     AX,WORD PTR MYIPADDR
	AND	AX,WORD PTR SUBNET[SI]
	CMP     AX,WORD PTR NETWORK[SI]
	JNE	NOTOURNET1

	POP	SI
	JMP	SKIPADD
;
NOTOURNET1:

	POP	SI
;
;	ENTRY IS OK - INSERT IT
;
	CMP	NUMBEROFROUTES,MAXROUTES
	JE	SKIPADD			; NO ROOM

	MOV     ROUTECHANGED[DI],1

	CALL	SCANLIST

	MOV	SI,OFFSET TEMPENTRY
	MOV	CX,TYPE ROUTEENTRY
	REP MOVSB

	INC     NUMBEROFROUTES

SKIPADD:

	POP	SI

NEXTRIPENTRY:

	POP     CX
	JMP     RIPOUTERLOOP

RIPFINISHED:

	CALL	TIDY_LIST		; TRY TO MERGE ENTRIES 
;
;	SEE IF ANYTHING HAS CHANGED (DONE HERE SO ENTRIES REMOVED BY
;	TIDY AREN'T INCLUDED
;
	MOV     DI,OFFSET ROUTETAB
	MOV     CX,NUMBEROFROUTES
	MOV	AL,0

RIPTESTLOOP:

	OR	AL,ROUTECHANGED[DI]
	MOV	ROUTECHANGED[DI],0

	ADD     DI,TYPE ROUTEENTRY
;
	LOOP    RIPTESTLOOP
;
	OR	AL,AL
	JZ	NOCHANGE

	MOV	RIPTIM,2		; DO ANOTHER BROADCAST

NOCHANGE:

	MOV	AL,RIPTIM
	CALL	BYTE_TO_HEX

	RET

UPDATE_ENTRY:

	MOV4    GATEWAY[DI],UDPSOURCE   ; WHERE THIS MSG CAME FROM

	CMP	AX25_CTRL,0FFH
	JE	RIP_U_NET			; VIA NETROM
;
;	VIRTUAL CIRCUIT MODE
;
	MOV	AL,PORT_NO
	MOV     PORT[DI],AL

	MOV	RTYPE[DI],'V'
	JMP SHORT RIP_U_GOT_TYPE

RIP_U_NET:
;
	MOV	RTYPE[DI],'N'
	MOV	PORT[DI],0
;
RIP_U_GOT_TYPE:

	MOV	ROUTEINFO[DI],LEARNED
	MOV     ROUTECHANGED[DI],1

REFRESH_RIP:

	MOV     AL,RIPMETRIC		; INCOMING METRIC
	INC     AL
	CMP     AL,16
	JBE     U_MET_OK
;
;       HE IS TELLING US ITS UNREACHABLE - START DELETE TIMER
;
	MOV     METRIC[DI],16
	MOV     RIPTIMOUT[DI],0
;
	CMP     GARTIMOUT[DI],0
	JNE     NEXTRIPENTRY            ; TIMER ALREADY RUNNING
;
	MOV     GARTIMOUT[DI],4         ; SET WAITING TO DELETE
;
	JMP	NEXTRIPENTRY

U_MET_OK:

	MOV     METRIC[DI],AL

	MOV     RIPTIMOUT[DI],MAXRIP
	MOV     GARTIMOUT[DI],0         ; IN CASE WAITING TO DELETE
;
	JMP	NEXTRIPENTRY


DO_AUTOROUTE:
;
;       SEE IF ALREADY IN TABLE
;
	CMP	LEARNFLAG,'N'
	JE	AR_EXIT			; LEARING DISABLED

	CMP     NUMBEROFROUTES,MAXROUTES
	JE      AR_EXIT			; NO ROOM, SO NO POINT CHECKING

	MOV4	TEMPENTRY,0[SI]

	CALL	CHECKLIST
	CMP	AL,0
	JE	AR_CHECK		; NOT THERE - FIND WHERE TO ADD
;
;	REFRESH ROUTE
; 
	CMP     RIPTIMOUT[DI],0
	JE	AR_CHECK		; LOCKED

	MOV     RIPTIMOUT[DI],MAXRIP
	MOV     GARTIMOUT[DI],0         ; IN CASE WAITING TO DELETE

	RET

AR_CHECK:

	CALL	SCANLIST		; FIND PLACE FOR IT
;
;	IF FOUND, DI=ENTRY. IF NOT, DI=SPARE PLACE
;
	CMP	AL,0
	JE	AR_ADD
;
;	'SHOULDNT HAPPEN'
;
AR_RET:
;
;	IT IS IN TABLE SO LEAVE (MAY LATER TRY TO SEE IF 'BETTER')
;
	RET

AR_ADD:
;
;       NOT FOUND, SO ADD
;
	INC     NUMBEROFROUTES

	MOV4    NETWORK[DI],0[SI]

	MOV     WORD PTR SUBNET[DI],0FFFFH
	MOV     WORD PTR SUBNET+2[DI],0FFFFH

	MOV	AL,PORT_NO
	MOV	PORT[DI],AL

;	MOV	AL,AX25_CTRL
;	CMP	AL,0FFH
;	JE	AR_NETROM		; VIA NETROM

	MOV	RTYPE[DI],'V'
	JMP SHORT GOT_TYPE

;AR_NETROM:
;
;	MOV	RTYPE[DI],'N'
;	MOV	PORT[DI],0
;
;	MOV	METRIC[DI],14		; CANT BE DIRECT - SO DONT KNOW METRIC
;	MOV	ROUTEINFO[DI],LEARNED
;
;	ALSO NEED TO PUT IN GATEWAY
;
AR_EXIT:

	CALL	TIDY_LIST		; TRY TO MERGE ENTRIES 
	RET

GOT_TYPE:
;
;	ALSO NEED TO UPDATE ARP ENTRY
;
	MOV	METRIC[DI],1		; DIRECT
	MOV	ROUTEINFO[DI],LEARNED
	MOV	RIPTIMOUT[DI],MAXRIP
	MOV	RIPTIM,2		; TELL OTHERS ABOUT IT

AR_ADDTOOURTABLE:
;
;       LOOK FOR ENTRY IN ARP TABLE - IF NOT THERE, ADD
;
	MOV     DI,OFFSET ARPTABLE
	MOV     CX,MAXARP
	MOV     DX,0

AR_ARPREPLYLOOP:

	CMP4    [DI],[SI]
	JNE     AR_NOTTHIS

	RET				; ALREADY THERE (? SHOULDNT BE)

AR_NOTTHIS:

	CMP     BYTE PTR [DI],0
	JNE     AR_GOTSPARE

	CMP     DX,0
	JNE     AR_GOTSPARE

	MOV     DX,DI                   ; SAVE FIRST SPARE

AR_GOTSPARE:

	ADD     DI,TYPE ARPDATA
	LOOP    AR_ARPREPLYLOOP
;
;       NOT FOUND - ADD IT IF ANY SPARE ENTRIES
;
	CMP     DX,0
	JE      AR_ADDRET

	MOV     DI,DX
	MOV4    [DI],[SI]

	MOV     AL,PORT_NO
	MOV     ARPINTERFACE[DI],AL

	MOV	ARPTYPE[DI],'V'

	MOV     ARPVALID[DI],1
	
	MOV     ARPTIMER[DI],0FFFFH	; LOCKED

	LEA     DI,TARGETHWADDR[DI]
	MOV	SI,BUFFPTR[BX]		; TO IP HEADER
	SUB	SI,9			; ADDR CTRL PID
	MOV     CX,7
	REP MOVSB                       ; PUT ADDR INTO TABLE   
;
AR_ADDRET:

	RET
;




IPGETBUFF:

	CMP     FREE_Q,0
	JE      NOBUFF

	DEC     BUFFERCOUNT

	MOV     AL,BUFFERCOUNT
	CMP     AL,MINBUFFS
	JAE     NOTMIN

	MOV     MINBUFFS,AL

NOTMIN:

	MOV     DI,FREE_Q
	MOV     AX,[DI]                 ; NEXT BUFFER
	MOV     FREE_Q,AX

	OR      AL,1                    ; SET NZ

NOBUFF:

	RET

IPRELBUFF:

	MOV     SI,OFFSET FREE_Q

	CMP     DI,OFFSET TCPRXBUFFERS
	JB      DUFFADD

	CALL    Q_ADDF
	RET

DUFFADD:

	MOV     DUFF02,DI               ; BAD ADDRESS
	POP     DUFF00
	PUSH    DUFF00                  ; SAVE RETURN ADDR

	RET

;
;       ICMP ROUTINES - FOR NOW JUST HANDLE ECHO
;
IP_ICMP:
;
;       IF ECHO REQUEST, SEND ECHO REPLY
;
	MOV     CX,BUFFLEN[BX]
	MOV     DI,BUFFPTR[BX]

	CMP     ICMPTYPE[DI],8
	JE      ICMP_ECHO
;
	CMP     ICMPTYPE[DI],0
	JE      ICMP_REPLY
;
;       DISCARD IT
;
ICMP_DISCARD:

	MOV     DI,BX

	CALL    IPRELBUFF
	RET

ICMP_REPLY:
;
	PUSH	BX

	PUSH	BX

	MOV     SI,OFFSET ECHOREPLY
	CALL    NORMSTR

	CALL	NORMSTR

	POP	BX

	MOV	SI,BUFFPTR[BX]
	SUB	SI,20			; BACK OVER IP HEADER

	LEA     SI,IPSOURCE[SI]
	CALL    PRINT4                  ; PRINT IP ADDR IN 'DOTTED DECIMAL' FORMAT

	CALL	SPACE

	PUSH	DS
	MOV	AX,40H
	MOV	DS,AX
	MOV	AX,DS:[6CH]		; GET TIME
	POP	DS

	SUB	AX,ICMPID[DI]
	MOV	DX,0
	DIV	W18
	CALL	CONV_5DIGITS

	mov     AL,CR
	call    PUTCHAR

	POP	BX

	JMP	ICMP_DISCARD

ICMP_ECHO:
;
;       ECHO REQUEST
;
	MOV     ICMPCHECKSUM[DI],0
	MOV     ICMPTYPE[DI],0          ; CONVERT TO ECHO REPLY
;
;       CHECKSUM IT
;
	MOV     SI,BUFFPTR[BX]
	MOV     CX,BUFFLEN[BX]

	INC     CX                      ; ROUND UP
	SHR     CX,1                    ; WORD COUNT

	CALL    DO_CHECKSUM
	NOT     DX
	MOV     ICMPCHECKSUM[DI],DX
;
IPSUMANDSEND:
;
;       ADD IP HEADER AND SEND
;
	ADD     BUFFLEN[BX],20          ; IP HDDR LEN
	SUB     BUFFPTR[BX],20          ; DITTO

	MOV     CX,BUFFLEN[BX]          ; GET LENGTH
	MOV     DI,BUFFPTR[BX]
;
;       DI NOW POINTER TO IP BIT
;
	XCHG	CH,CL
	MOV	IPLENGTH[DI],CX

	MOV	AL,TTL
	MOV     IPTTL[DI],AL            ; RESET TTL
	MOV     IPCHECKSUM[DI],0        ; WILL CALCULATE LATER

	MOV4    TEMP,IPDEST[DI]
	MOV4    IPDEST[DI],IPSOURCE[DI]
	MOV4    IPSOURCE[DI],TEMP       ; SWAP IP ADDRESS
;
;       HEADER COMPLETE - ADD CHECKSUM AND TRACE IF NECESSARY
;
	MOV     SI,BUFFPTR[BX]
	MOV     CX,10                   ; LENGTH

	CALL    DO_CHECKSUM
	NOT     DX
	MOV     IPCHECKSUM[DI],DX
;
;       FEED BACK THROUGH ROUTER
;
	MOV     DI,BX
	MOV     SI,OFFSET IP_Q
	CALL    Q_ADD

	RET
;
;	BUILD ICMP ECHO REQUEST
;
SENDPING:

	LEA     DI,TEMP
	ADD	SI,3
	CALL    GETIPADDR          ; GET A 4 DIGIT ADDRESS

	CALL	IPGETBUFF
	JNZ	PINGOK

	RET

PINGOK:

	MOV	BX,DI
	MOV	TXBUFFER,DI		; SAVE
;
;	SET POINTER TO ICMP HEADER POSITION
;
	MOV	BUFFLEN[DI],0		; CURRENTLY NO DATA

	LEA	AX,BUFFDATA+21+20[DI]	; ALLOW ROOM FOR NETROM AND IP HDDR
	MOV	BUFFPTR[DI],AX		; UDP HEADER POSN
;
	MOV	DI,AX			; ADDR OF ICMP HEADER
;
	MOV     ICMPCHECKSUM[DI],0
	MOV     ICMPTYPE[DI],8          ; ECHO REQUEST
	MOV     ICMPCODE[DI],0

	PUSH	DS
	MOV	AX,40H
	MOV	DS,AX
	MOV	AX,DS:[6CH]		; GET TIME
	POP	DS

	MOV     ICMPID[DI],AX
	MOV     ICMPSEQUENCE[DI],0

	MOV	BUFFLEN[BX],30		; REASONABLE LENGTH
;
;       CHECKSUM IT
;
	MOV     SI,BUFFPTR[BX]
	MOV     CX,BUFFLEN[BX]

	INC     CX                      ; ROUND UP
	SHR     CX,1                    ; WORD COUNT

	CALL    DO_CHECKSUM
	NOT     DX
	MOV     ICMPCHECKSUM[DI],DX
;
;       ADD IP HEADER AND SEND
;
	ADD     BUFFLEN[BX],20          ; IP HDDR LEN
	SUB     BUFFPTR[BX],20          ; DITTO

	MOV     CX,BUFFLEN[BX]          ; GET LENGTH
	MOV     DI,BUFFPTR[BX]
;
;	DI NOW POINTER TO IP BIT, CX=IP LENGTH
;
	MOV	VERLEN[DI],45H		; VERSION 4, LEN 5 WORDS
	MOV	TOS[DI],0

	XCHG	CH,CL
	MOV	IPLENGTH[DI],CX

	INC	NEXTIPIDENT

	COPY2	IPID[DI],NEXTIPIDENT

	MOV	FRAGWORD[DI],0

	MOV	AL,TTL
	MOV	IPTTL[DI],AL

	MOV	IPPROTOCOL[DI],1	; ICMP

	MOV	IPCHECKSUM[DI],0	; WILL CALCULATE LATER

	MOV4	IPSOURCE[DI],MYIPADDR
	MOV4	IPDEST[DI],TEMP		; TESTING !!
;
;       HEADER COMPLETE - ADD CHECKSUM
;
	MOV     SI,BUFFPTR[BX]
	MOV     CX,10                   ; LENGTH

	CALL    DO_CHECKSUM
	NOT     DX
	MOV     IPCHECKSUM[DI],DX
;
;       FEED BACK THROUGH ROUTER
;
	MOV     DI,BX

	MOV     SI,OFFSET IP_Q
	CALL    Q_ADD

	MOV     SI,OFFSET ECHOSENT
	CALL    NORMSTR

	RET
;

ROUTEFRAME:
;
;       DI = BUFFER HEADER
;
	MOV     BX,DI

	MOV     SI,BUFFPTR[BX]          ; IP HEADER 

	PUSH    BX
	PUSH	SI

	MOV	IP_MSG+1,'O'		; INPUT
	CALL    IPTRACE

	POP	SI
	POP     BX                      ; HEADER STRUCT
	INC4	FRAMESSENT

        MOV     AX,BUFFLEN[BX]
        ADD     WORD PTR BYTESSENT,AX
        ADC     WORD PTR BYTESSENT+2,0
;
;	SEARCH LIST BACKWARDS (SO SPECIFIC ROUTES OVERRIDE SUBNET ONES)
;
	MOV     BP,OFFSET ROUTETAB

	MOV     CX,NUMBEROFROUTES
	JCXZ	NOROUTES

	MOV	AX,CX
	DEC	AX
	MUL	ROUTEENTRYLEN
	ADD	BP,AX			; POINT TO LAST

ROUTELOOP:

	CMP     DS:METRIC[BP],16
	JE      ROUTENEXT               ; DEAD ROUTE

	MOV     AX,WORD PTR IPDEST+2[SI]
	AND	AX,WORD PTR DS:SUBNET+2[BP]
	CMP     AX,WORD PTR DS:NETWORK+2[BP]
	JNE     ROUTENEXT

	MOV     AX,WORD PTR IPDEST[SI]
	AND	AX,WORD PTR DS:SUBNET[BP]
	CMP     AX,WORD PTR DS:NETWORK[BP]
	JE      FOUNDROUTE

ROUTENEXT:

	SUB     BP,TYPE ROUTEENTRY
;
	LOOP    ROUTELOOP

NOROUTES:
;
;       NO ENTRY - TRY DEFAULT??
;
	MOV     BP,OFFSET DEFAULTGATEWAY
	CMP     DS:RTYPE[BP],0
        JNE     FOUNDROUTE
;
;       DEFAULT NOT DEFINED, SO CHUCK IT
;
CHUCKFRAME:

	INC4    CANTROUTECOUNT
;
;	CONVERT TO ICMP DEST UNREACHABLE
;
;	UNLESS MESSAGE IS AN ICMP - DONT WANT TO REPORT ERRORS IN ERROR REPORTS
;	(BUT WOT ABOUT PING - IT SEEMS USEFUL TO BE ABLE TO GET DEST UNREACHABLE
;	FOR PING TESTING
;
;	SI (AND BUFFPTR) POINTS TO IP HEADER
;
	MOV	DI,OFFSET ICMPHEADER
	MOV	AL,3			; DEST UNREACHABLE
	STOSB
	MOV	AL,0			; SUBYTPE = NET UNREACHABLR
	MOV	CX,7
	REP STOSB			; CLEAR REST OF HEADER 
;
	CALL	SENDICMPREPORT

	RET

FOUNDROUTE:

	INC4    DS:FRAMECOUNT[BP]

	MOV     TXBUFFER,BX             ; SAVE
;
;       SEE IF DIRECT, OR VIA ANOTHER ROUTER
;
	CMP     BYTE PTR DS:GATEWAY[BP],0
	JE      SENDDIRECT
;
;       NEED TO SEND TO GATEWAY
;
	LEA     BX,GATEWAY[BP]
	JMP SHORT RESOLVE

SENDDIRECT:
;
	MOV     BX,BUFFPTR[BX]
	LEA     BX,IPDEST[BX]           ; BX POINTS TO REQUIRED IP ADDR

RESOLVE:
;
;       RESOLVE IP TO AX25 ADDRESS BEFORE PLACING ON TX QUEUE
;
;       LOOK FOR IP ADDR IN [BX] IN ARP TABLE
;
	MOV     SI,OFFSET ARPTABLE
	MOV     DI,0                    ; FIRST SPARE ENTRY
	MOV     CX,MAXARP

RESARPLOOP:

	CMP4    TARGETIPADDR[SI],0[BX]
	JE      FOUNDARP

	CMP     TARGETIPADDR[SI],0
	JNE     NOTSPARE
;
;       SPARE ENTRY - IF FIRST, REMEMBER IT
;
	CMP     DI,0
	JNE     NOTSPARE                ; ALREADY HAVE A SPARE

	MOV     DI,SI

NOTSPARE:

	ADD     SI,TYPE ARPDATA
	LOOP    RESARPLOOP
;
;       ENTRY NOT FOUND - IF ANY SPARE ENTRIES, USE ONE
;
	CMP     DI,0
	JNE     OKTOADD
;
;       ARP TABLE IS FULL - SHOULD REALLY REUSE OLDEST, BUT FOR NOW JUST DISCARD MSG
;
	MOV     DI,TXBUFFER

	CALL    IPRELBUFF

	INC4    ARPFULLCOUNT

	RET                             ; JUST THROW AWAY!

OKTOADD:

	PUSH    DI

	MOV     SI,BX
	LEA     DI,TARGETIPADDR[DI]
	MOVSW
	MOVSW

	POP     SI
;
;       DROP THROUGH TO SEND REQUEST
;
FOUNDARP:
;
;       SI = ARP TABLE ENTRY
;
	MOV     AL,DS:PORT[BP]
	MOV     ARPINTERFACE[SI],AL     ; REFRESH

	MOV     AL,DS:RTYPE[BP]
	MOV     ARPTYPE[SI],AL

	CMP     ARPVALID[SI],0
	JZ	PUTONARPQ

	JMP     GOTETADDR
;
;       ADDRESS NOT VALID - SAVE MESSAGE, AND SEND ARP REQUEST
;       TO TRY TO FIND IT
;
PUTONARPQ:

	PUSH    SI

	MOV     DI,TXBUFFER
	LEA     SI,ARP_Q[SI]
	CALL    Q_ADD

	POP     SI

	CMP     ARPTIMER[SI],0
	JNE     ARPACTIVE               ; REQUEST OUTSTANDING
;
DOARPREQUEST:

	MOV     ARPTIMER[SI],30         ; 30 SEC TIMEOUT
;
;	NEED DIFFERENT PATHS FOR ETHERNET OR AX.25
;
	CMP	ARPTYPE[SI],'E'
	JE	DOARPETH

	LEA     SI,TARGETIPADDR[SI]
	MOV     DI,OFFSET ARPIPTARGET
	MOV     CX,4
	REP MOVSB

	MOV     SI,OFFSET MYCALL
	MOV     DI,OFFSET MYADDR1
	MOV     CX,7
	REP MOVSB

	MOV     SI,OFFSET MYCALL
	MOV     DI,OFFSET MYADDR2
	MOV     CX,7
	REP MOVSB

	MOV	SI,OFFSET ARPREQUESTMSG+16
	MOV	ARP_MSG+1,'O'
	CALL    ARPTRACE

;
	MOV     SI,OFFSET ARPREQUESTMSG
	OR      BYTE PTR 13[SI],1       ; SET SEND OF CALL
	MOV     CX,46
;
	MOV     AH,10                   ; SEND DATAGRAM
	MOV     AL,DS:PORT[BP]             
	CALL    NODE
;
ARPACTIVE:

	RET

DOARPETH:

	LEA     SI,TARGETIPADDR[SI]
	MOV     DI,OFFSET EARPIPTARGET
	MOV     CX,4
	REP MOVSB

;	MOV	SI,OFFSET ARPREQUESTMSG+16
;	MOV	ARP_MSG+1,'O'
;	CALL    ARPTRACE

;
	MOV	AH,22H
	MOV	SI,OFFSET BROADCAST
	MOV     DI,OFFSET EARPFRAME
	MOV	CX,28
	CALL	ODI

	RET

GOTETADDR:

	TEST    ARPTIMER[SI],8000H
	JNZ     ARPLOCKED

	MOV     ARPTIMER[SI],30*60      ; 16 MINS TIMEOUT

ARPLOCKED:

	CMP     ARP_Q[SI],0
	JE	@F			; NOTHING QUEUED

	JMP     PUTONARPQ               ; STUFF ALREADY QUEUED, SO ADD ON END

 @@:
	MOV     BX,TXBUFFER

SENDTONODE:
;
;       BX=BUFFER HEADER SI=ARP TABLE ENTRY
;
	CMP     ARPTYPE[SI],'N'
	JE	NETSEND

	JMP     NOTNETSEND

NETSEND:
;
L4MORE		EQU	20H		; MORE DATA FOLLOWS - FRAGMENTATION FLAG
;
;       SEND USING NETROM
;
;	IF LENGTH IS ABOVE 236 (NETROM MAX) THEN FRAGMENT
;

	MOV	DL,0			; FRAG FLAG

	IF	0

	MOV	AX,BUFFLEN[BX]
	CMP	AX,150 ; TEST 236
	MOV	FRAGFLAG,AX

	JBE	NETLENOK
;
	MOV     DI,BUFFPTR[BX]		; START OF DATA (IP PID?)
	ADD	DI,128			; POINTS TO DATA WE WONT SEND
	MOV	FRAGSAVE,DI
;
	MOV	DL,L4MORE
	MOV	BUFFLEN[BX],128
;
	ENDIF

NETLENOK:

	ADD	BUFFLEN[BX],21          ; NETROM HEADER
	SUB     BUFFPTR[BX],21          ; DITTO

NR_SEND_COM:

	MOV     DI,BUFFPTR[BX]

	MOV     AL,0CFH
	STOSB                           ; NETROM PID

	PUSH	SI			; SAVE ARP

	PUSH    SI

	MOV     SI,OFFSET MYCALL
	MOV     CX,7
	REP MOVSB

	POP     SI

	LEA     SI,TARGETHWADDR[SI]
	MOV     CX,7
	REP MOVSB

	MOV     AL,10                   ; TTL
	STOSB                           ; NETROM TYPE
	MOV     AL,0CH
	STOSB
	STOSB                           ; CIRCUIT

	MOV     AL,0
	STOSB
	STOSB				; SEQ
	MOV	AL,DL
	STOSB                           ; CTRL

	PUSH    BX

	MOV     SI,BUFFPTR[BX]
	MOV     CX,BUFFLEN[BX]
;
;       SEND TO NODE
;
	ADD     CX,15
	SUB     SI,15                   ; NODE REMOVES THIS MUCH!

	MOV     DX,0                    ; NETROM
;
	MOV     AH,14
	CALL    NODE
;
	POP     DI			; BUFFER
	POP	SI			; ARP

	IF	0

	CMP	FRAGFLAG,150
	JBE	DIDNTFRAG
;
;	NOW HAVE TO SEND REST OF INFO
;
	MOV	BX,DI
	MOV	DI,FRAGSAVE
	MOV	BUFFPTR[BX],DI

	MOV	AX,FRAGFLAG
	SUB	AX,128
	MOV	BUFFLEN[BX],AX

	MOV	DL,0

	MOV	FRAGFLAG,0
	JMP	NETLENOK

DIDNTFRAG:

	ENDIF

	CALL    IPRELBUFF

	RET
;

NOTNETSEND:

	CMP     ARPTYPE[SI],'E'
	JE	ETHSEND

	JMP     NOTETHSEND

ETHSEND:
;
;	SEND TO ETHERNET IN NORMAL E_II FORMAT
;
;	WE ARE USING ODI SEND, WHICH WILL ADD THE PROTO HEADER.
;	SO WE ONLY SEND DATA. BUT WE MUST SUPPLY POINTER TO MAC TARGET
;	ADDRESS
;
; 	CALLS RESIDENT ODI INTERFACE WITH:
;
;	AH = 12h  Send IP frame. DI = ADDR
;				 CX = LENGTH
;			         SI = ETH ADDR
;
;	SI POINTS TO ARP ENTRY
;
	PUSH	BX

	MOV     DI,BUFFPTR[BX]
	MOV	CX,BUFFLEN[BX]		
	LEA     SI,TARGETHWADDR[SI]
	MOV	AH,12H

	CALL	ODI

	POP	DI
	CALL    IPRELBUFF

	RET
;
ODI:
	CMP	CS:ODIINT+1,0
	JE SHORT @F			; NOT SET

ODIINT	LABEL	BYTE

	INT	0			; PATCHED BY INIT ROUTINE
@@:
	RET

NOTETHSEND:

	ADD     BUFFLEN[BX],16          ; AX25 HEADER
	SUB     BUFFPTR[BX],16          ; DITTO

	MOV     DI,BUFFPTR[BX]

	PUSH    SI                      ; SAVE ARP TABLE

	LEA     SI,TARGETHWADDR[SI]
	MOV     CX,7
	REP MOVSB
	AND     BYTE PTR -1[DI],0FEH    ; CLEAR END OF CALL

	MOV     SI,OFFSET MYCALL
	MOV     CX,7
	REP MOVSB

	OR      BYTE PTR -1[DI],1       ; SET END OF CALL

	MOV     AL,03H
	STOSB                           ; UI IF DATAGRAM

	MOV     AL,0CCH
	STOSB                           ; PID

	POP     SI                      ; ARP

	PUSH    BX

	MOV     AH,ARPTYPE[SI]
	MOV     AL,ARPINTERFACE[SI]     ; PORT NUMBER

	MOV     SI,BUFFPTR[BX]
	MOV     CX,BUFFLEN[BX]

	CMP     AH,'V'
	JE      SENDVC

	MOV     AH,10                   ; SEND AS DATAGRAM
	CALL    NODE
;
	POP     DI

	CALL    IPRELBUFF

	RET
;
SENDVC:
;
;       SEND AS L2
;
	MOV     DL,AL                   ; PORT
	MOV     DH,0
;
	MOV     AH,14
	CALL    NODE
;
	POP     DI

	CALL    IPRELBUFF

	RET


SENDTOETH:
;
;	PASS TO ODI DRIVER
;
	RET



TIMER_TICK:

	CMP	BYTE PTR MYIPADDR,0
	JNE	TICK_00
;
;	NOT INITIALISED
;
	RET

TICK_00:
;
;       CHECK ARP INFO
;
	DEC	SECS
	JNZ	NOTMINUTE

	INC4	MINSACTIVE

	MOV	SECS,60
;
;	EACH MINUTE
;
	DEC	RIPTIM
	JNZ	NOTMINUTE

	CALL	CHECKRIPTIMERS

	CALL	SENDRIP

	CALL	DUMPTABLES

	MOV	RIPTIM,180		; 3 HOURS

NOTMINUTE:

	MOV     DI,OFFSET ARPTABLE
	MOV     CX,MAXARP

ARPLOOP:

	CMP     TARGETIPADDR[DI],0
	JE      SPAREENTRY

	TEST    ARPTIMER[DI],8000H
	JNZ     ARPOK                   ; LOCKED

	DEC     ARPTIMER[DI]
	JNZ     ARPOK
;
;       TIMER HAS EXPIRED - IF ENTRY IS ACTIVE, DELETE IT
;       IF NOT, WE HAVE NOT RECEIVED A REPLY, SO TRY AGAIN
;
	CMP     ARPVALID[DI],1
	JE      REMOVEARP
;
	CMP     ARP_Q[DI],0
	JE      REMOVEARP               ; NOTHING QUEUED
;
;       REMOVE FIRST QUEUED FRAME TO AVOID INDEFINITE LOOP
;
	PUSH    DI

	LEA     SI,ARP_Q[DI]
	CALL    Q_REM
	JZ      NOTHINGTOREMOVE

	CALL    IPRELBUFF

NOTHINGTOREMOVE:
;
	POP     DI

	PUSH    CX
	PUSH    DI

	MOV     SI,DI

	CALL    DOARPREQUEST            ; TRY AGAIN

	POP     DI
	POP     CX

	JMP SHORT ARPOK

REMOVEARP:

	PUSH    CX
	PUSH    DI

	MOV     CX,TYPE ARPDATA
	XOR     AL,AL
	REP STOSB

	POP     DI
	POP     CX

ARPOK:
SPAREENTRY:

	ADD     DI,TYPE ARPDATA
	LOOP    ARPLOOP

	RET

CHECKRIPTIMERS:

	MOV     SI,OFFSET ROUTETAB
	MOV     CX,NUMBEROFROUTES

RIPTIMLOOP:

	MOV     ROUTECHANGED[SI],0      ; ABOUT TO SEND A REGULAR UPDATE

	CMP     GARTIMOUT[SI],0
	JE      NOT_TIMING_OUT

	DEC     GARTIMOUT[SI]
	JNZ     RIPTIMNEXT
;
;       TIME TO REMOVE ENTRY
;
	CALL	REMOVEROUTE

	JCXZ	RIPNOMORE

	LOOP    RIPTIMLOOP		; SI NOT CHANGED, BUT IS NOW POINTING 
					; AT NEXT ENTRY

RIPNOMORE:

	RET

NOT_TIMING_OUT:

	CMP     RIPTIMOUT[SI],0
	JE      RIPTIMNEXT              ; STATIC ENTRY

	DEC     RIPTIMOUT[SI]
	JNZ     RIPTIMNEXT
;
;       ITS FAILED
;
	MOV     METRIC[SI],16
	MOV     GARTIMOUT[SI],1         ; KEEP ANOTHER 3 HOURS

RIPTIMNEXT:

	ADD     SI,TYPE ROUTEENTRY
	LOOP    RIPTIMLOOP

NORIP:

	RET


REMOVEROUTE:

	PUSH    SI

	MOV     DI,SI
	ADD     SI,TYPE ROUTEENTRY      ; POINT TO NEXT ENTRY

	PUSH    CX

	MOV     AX,CX                   ; NUMBER FOLLOWING IN LIST (INCLUDING
					; A BLANK ENTRY )

	MUL     ROUTEENTRYLEN

	MOV     CX,AX
	REP MOVSB                       ; MOVE OTHER ENTRIES UP

	POP     CX
;	DEC	CX			; REMOVED ONE

	POP     SI

	DEC     NUMBEROFROUTES

	RET



;
;	RIP TRANSMIT ROUTINES
;
SENDRIP:

	MOV	CX,NUMBEROFNEIGHBOURS
	JCXZ	NORIP

	MOV	BP,OFFSET NEIGHBOURS

RIPTXLOOP:

	PUSH	CX
	CALL	BUILDRIPREQUEST
	POP	CX

	ADD	BP,TYPE NEIGHBOUR
	LOOP	RIPTXLOOP

	RET

BUILDRIPREQUEST:

	CALL	IPGETBUFF
	JNZ	RIPBUFOK

	RET

RIPBUFOK:

	MOV	SI,OFFSET ROUTETAB
	MOV	CX,NUMBEROFROUTES

DONEXTBIT:

	MOV	BX,DI
	MOV	TXBUFFER,DI		; SAVE
;
;	SET POINTER TO UDP HEADER POSITION
;
	MOV	BUFFLEN[DI],0		; CURRENTLY NO DATA

	LEA	AX,BUFFDATA+21+20[DI]	; ALLOW ROOM FOR NETROM AND IP HDDR
	MOV	BUFFPTR[DI],AX		; UDP HEADER POSN
;
	MOV	DI,AX			; ADDR OF UDP HEADER
;
	MOV	AX,520			; SOCKET
	XCHG	AL,AH

	STOSW
	STOSW

	XOR	AX,AX
	STOSW				; LENGTH
	STOSW				; CHECKSUM
;
;	NOW THE RIP STUFF
;
	MOV	AL,2
	STOSB				; COMMAND=RESPONSE

	MOV	AL,DS:NBOURFLAG[BP]	; OLD/NEW FLAG
	STOSB				; VERSION

	XOR	AX,AX
	STOSW				; MUST BE ZERO

	MOV	DX,0			; ENTRIES IN THIS MESSAGE

RIPLOOP:
;
;	IF ROUTE IS VIA THIS ROUTER DONT INCLUDE IT
;
	MOV	AX,WORD PTR DS:NBOURIPADDR[BP]
	CMP	AX,WORD PTR GATEWAY[SI]
	JNE	SENDIT

	MOV	AX,WORD PTR DS:NBOURIPADDR+2[BP]
	CMP	AX,WORD PTR GATEWAY+2[SI]
	JE	DONTSENDIT

SENDIT:

	CMP     RIPTIMOUT[SI],0
	JE	SEND_IT2		; LOCKED, SO ADVERTISE
;
;	ONLY ADVERTISE LEARNED STUFF WHICH IS PRETTY FRESH
;
	CMP     RIPTIMOUT[SI],MAXRIP-2
	JB	DONTSENDIT

SEND_IT2:

	CMP	METRIC[SI],15
	JAE	DONTSENDIT		; WILL BE INFINITE WHEN RECEIVED
;
;	BUILD AN ENTRY IN RIP MESSAGE

	PUSH	BX
	PUSH	CX

	MOV     AX,0
	MOV     BX,WORD PTR SUBNET[SI]
	CALL    COUNTBITS
	MOV     BX,WORD PTR SUBNET+2[SI]
	CALL    COUNTBITS

	POP	CX
	POP	BX

	MOVSW
	MOVSW				; IP ADDR

	CMP	DS:NBOURFLAG[BP],'B'	; OLD TYPE?
	JE	NOMASK

	STOSB				; SUBNET MASK

NOMASK:

	SUB	SI,4
	MOV	AL,METRIC[SI]
	STOSB

	INC	DX

DONTSENDIT:

	ADD	SI,TYPE ROUTEENTRY

	CMP	DX,30	
	JE	SENDFULLFRAME		; CURRENT MSG IS FULL
;
	LOOP	RIPLOOP

	CMP	DX,0
	JE	EMPTYFRAME		; MESSAGE IS EMPTY

SENDNORM:

	CALL	SENDRIPFRAME		; SEND LAST FRAGMENT
	RET

EMPTYFRAME:
;
;	RETURN BUFFER
;
	MOV	DI,TXBUFFER
	MOV	SI,OFFSET FREE_Q

	CLI
	CALL	Q_ADDF
	STI
	RET

SENDFULLFRAME:
;
;	CURRENT FRAME IS FULL - SEND IT AND START ANOTHER
;
	DEC	CX
	JZ	SENDNORM		; NO MORE TO FOLLOW, SO JUST SEND NORMALLY

	PUSH	SI
	PUSH	CX

	CALL	SENDRIPFRAME

	CALL	IPGETBUFF

	POP	CX
	POP	SI

	JNZ	NEXTBUFOK

	RET

NEXTBUFOK:

	JMP	DONEXTBIT		; LOOP BACK AND DO NEXT

SENDRIPFRAME:
;
;	CALCULATE LENGTH
;
	MOV	BYTE PTR [DI],0		; FOR CHECKSUM

	MOV	AX,DI
	SUB	AX,BUFFPTR[BX]		; LENGTH
	MOV	BUFFLEN[BX],AX		; CURRENTLY NO DATA

	MOV	DI,BUFFPTR[BX]
	XCHG	AH,AL
	MOV	UDPLENGTH[DI],AX
;
;	ADD IP HEADER TO UDP MESSAGE IN TXBUFFER
;
	ADD	BUFFLEN[BX],20		; IP HDDR LEN
	SUB	BUFFPTR[BX],20		; DITTO

	MOV	CX,BUFFLEN[BX]		; GET LENGTH
	MOV	DI,BUFFPTR[BX]
;
;	DI NOW POINTER TO IP BIT
;
	MOV	VERLEN[DI],45H		; VERSION 4, LEN 5 WORDS
	MOV	TOS[DI],0

	XCHG	CH,CL
	MOV	IPLENGTH[DI],CX

	INC	NEXTIPIDENT

	COPY2	IPID[DI],NEXTIPIDENT

	MOV	FRAGWORD[DI],0

	MOV	AL,TTL
	MOV	IPTTL[DI],AL

	MOV	IPPROTOCOL[DI],11H	; UDP

	MOV	IPCHECKSUM[DI],0	; WILL CALCULATE LATER

	MOV4	IPSOURCE[DI],MYIPADDR
	MOV4	IPDEST[DI],DS:NBOURIPADDR[BP]
;
;	NOW NEED TO DO UDP CHECKSUM
;
;	DO PSEUDO HEADER FIRST
;
	MOV	DX,1100H		; PROTOCOL (REVERSED)

	MOV	AX,WORD PTR IPDEST[DI]
	ADC	DX,AX
	MOV	AX,WORD PTR IPDEST+2[DI]
	ADC	DX,AX
	MOV	AX,WORD PTR IPSOURCE[DI]
	ADC	DX,AX
	MOV	AX,WORD PTR IPSOURCE+2[DI]
	ADC	DX,AX
	ADC	DX,0
;
;	NOW ADD UDP LENGTH
;
	ADD	DI,20			; TO UDP HEADER
	
	ADD	DX,UDPLENGTH[DI]
	ADC	DX,0

	MOV	PHSUM,DX

	MOV	CX,UDPLENGTH[DI]
	MOV	SI,DI

	XCHG	CH,CL
	INC	CX			; ROUND UP
	SHR	CX,1			; WORD COUNT

	CALL	DO_CHECKSUM

	ADD	DX,PHSUM
	ADC	DX,0
	NOT	DX

	MOV	UDPCHECKSUM[DI],DX
;
;	HEADER COMPLETE - ADD CHECKSUM
;
	MOV	SI,BUFFPTR[BX]
	MOV	DI,SI

	MOV	CX,10		; LENGTH

	CALL	DO_CHECKSUM
	NOT	DX
	MOV	IPCHECKSUM[DI],DX
;
;	ADD ETHERNET HEADER, AND SEND IT
;
	MOV	DI,TXBUFFER
	MOV	SI,OFFSET IP_Q
	CALL	Q_ADD			; PASS TO ROUTER

	RET



PUTCHAR:

	PUSH    DI
	CMP     AL,0DH
	JNE     NOTNL

	PUSH    CX
	PUSH    SI
	CALL    NEWLNE
	POP     SI
	POP     CX
	POP     DI
	RET

NOTNL:

	CALL    NORMCHR
	POP     DI

	RET

PROCESSFRAME:
;
;       BX = BUFFER HEADER, DI=NODE HEADER
;
	MOV     AL,MSGPORT[DI]
	MOV     PORT_NO,AL              ; 
;
;       DATA FROM NODE HAS 5 BYTE INTERNAL HEADER
;
	ADD     BUFFPTR[BX],5           ; ADJUST BUFFER
	SUB     BUFFLEN[BX],5
;
;       NOW POINTING TO PROTOCOL HEADER (AX.25 OR NETROM)
;
	MOV     SI,BUFFPTR[BX]
	MOV     CX,BUFFLEN[BX]

	CMP     BYTE PTR [SI],0CFH
	JNE     NOTNETROM
;
;       A NETROM ENCAPSULATED DATAGRAM
;
	ADD     BUFFPTR[BX],20          ; REMOVE NETROM HEADER
	SUB     BUFFLEN[BX],20
;
	MOV     BYTE PTR 20[SI],0CCH    ; IP
;
	MOV	AX25_CTRL,0FFH
	JMP     NOTL2

NOTNETROM:

	ADD     BUFFPTR[BX],15          ; REMOVE L2 ADDR/CONTROL
	SUB     BUFFLEN[BX],15

	MOV     SI,BUFFPTR[BX]
	DEC	SI			; TO AX25 CONTROL
	LODSB
	MOV	AX25_CTRL,AL

NOTL2:

	MOV     SI,BUFFPTR[BX]
	MOV     CX,BUFFLEN[BX]          ; NOW POINTING TO PID

	LODSB
	CMP     AL,0CDH
	JNE     NOTARP

	JMP     ARPMSG

NOTARP:
;
;       LOOK FOR ODD PIDS
;
	CMP     AL,0CCH
	JE      IP_PID_JMP
;
	CMP	AL,8
	JNE	NOTFRAG
;
;	A FRAGMENTED MESSAGE 
;
	CMP	FRAGPTR,0
	JE	FIRSTFRAG
;
;	SUBSEQUENT FRAGMENT
;
	MOV	DI,FRAGPTR
	MOV	AX,DI
	ADD	AX,CX
	CMP	AX,OFFSET FRAGBUFFER+512
	JA	FRAG_TOO_BIG

	LODSB				; HEADER
	DEC	CX
	DEC	CX			; REMOVE PID AND CONTROL

	JCXZ	FRAG?			; SHOULDNT HAPPEN??

	REP MOVSB

FRAG?:

	MOV	FRAGPTR,DI

	CMP	AL,0
	JNE	FRAGEXIT		; NOT LAST
;
;	LAST FRAG - PROCESS IT
;
FRAG??:

	MOV	SI,OFFSET FRAGBUFFER
	MOV	CX,DI
	SUB	CX,SI

	MOV	FRAGPTR,0

	CMP	CX,300
	JA	FRAG_TOO_BIG		; CHUCK AWAY

	MOV	BUFFLEN[BX],CX
	MOV	DI,BUFFPTR[BX]
	REP MOVSB

	MOV     SI,BUFFPTR[BX]
	MOV     CX,BUFFLEN[BX]          ; NOW POINTING TO PID

	LODSB

IP_PID_JMP:

	JMP	IP_PID			; SHOULDNT BE ANYTHING ELSE

FRAG_TOO_BIG:

	MOV	FRAGPTR,0
	
	JMP	FRAGEXIT

FIRSTFRAG:

	LODSB				; FRAG HEADER 

	TEST	AL,80H
	JZ	NOTFRAG			; 1ST FRAMENT SHOULD BE 8?H
;
	MOV	DI,OFFSET FRAGBUFFER
	DEC	CX			; REMOVE OUTER PID
	DEC	CX
	REP MOVSB
	MOV	FRAGPTR,DI
	
	JMP	FRAGEXIT

NOTFRAG:
;
;       FOR NOW JUST DISPLAY
;
	PUSH    BX

	CMP	CX,300
	JB	XIPLOOP

	MOV	CX,300

XIPLOOP:

	lodsb
	CALL    BYTE_TO_HEX

	LOOP    XIPLOOP

	POP     DI

	CALL	IPRELBUFF

	RET

FRAGEXIT:

	MOV	DI,BX
	CALL    IPRELBUFF
	RET

IP_PID:
;
;       MUST BE IP
;
	PUSH    BX
	MOV	IP_MSG+1,'I'		; INPUT
	CALL    IPTRACE
	POP     BX                      ; HEADER STRUCT
;
	INC     BUFFPTR[BX]
	DEC     BUFFLEN[BX]             ; REMOVE PID

	CALL    IP_INPUT

TRACERET:

	RET

IPTRACE:

	CMP     TRACEFLAG,0
	JE      TRACERET

	PUSH    SI
	mov     SI,OFFSET IP_MSG
	call    NORMSTR
	POP     SI
;
	PUSH    SI

	LEA     SI,IPSOURCE[SI]
	CALL    PRINT4                  ; PRINT IP ADDR IN 'DOTTED DECIMAL' FORMAT

	POP     SI

	MOV     AL,'>' 
	CALL    PUTCHAR

	PUSH    SI

	LEA     SI,IPDEST[SI]
	CALL    PRINT4                  ; PRINT IP ADDR IN 'DOTTED DECIMAL' FORMAT

	MOV     SI,OFFSET LEN
	CALL    NORMSTR

	POP     SI

	MOV     AL,BYTE PTR IPLENGTH+1[SI]
	CALL    DISPLAY_BYTE_1

	PUSH    SI
	MOV     SI,OFFSET TTL_MSG
	CALL    NORMSTR
	POP     SI

	MOV     AL,IPTTL[SI]
	CALL    DISPLAY_BYTE_1          ; DISPLAY TTL

	PUSH    SI
	MOV     SI,OFFSET PROT
	CALL    NORMSTR
	POP     SI

	MOV     AL,IPPROTOCOL[SI]
	MOV     PROTOCOL,AL

	CALL    DISPLAY_BYTE_1          ; DISPLAY PROTOCOL TYPE

	MOV     AL,0DH
	CALL    PUTCHAR

	MOV     CH,BYTE PTR IPLENGTH[SI]
	MOV     CL,BYTE PTR IPLENGTH+1[SI]

	CMP     CX,20
	JBE     NO_INFO                 ; DUFF

	sub     CX,20
	JCXZ    NO_INFO                 ; EMPTY I FRAME
;
	ADD     SI,20                   ; SKIP IP HDDR

	CMP     CX,256
	JB      IPDLENOK

	MOV     CX,256

IPDLENOK:

	CMP     PROTOCOL,6
	JE      TCPMSG

	CMP     PROTOCOL,1
	JE      ICMPMSG

	CMP     PROTOCOL,73
	JE      RSPFMSG

	CMP     PROTOCOL,17
	JNE     NOTUDP

	JMP     UDPMSG

NOTUDP:

IPLOOP:

	lodsb
	CALL    BYTE_TO_HEX

	LOOP    IPLOOP

NO_INFO:
;
;       ADD CR UNLESS DATA ALREADY HAS ONE
;
	CMP     AL,CR
	JE      NOTANOTHER
ADD_CR:
	mov     AL,CR
	call    PUTCHAR

NOTANOTHER:
;
	ret

ICMPMSG:

	PUSH    SI
	MOV     SI,OFFSET ICMP_MSG
	CALL    NORMSTR
	POP     SI

	PUSH    SI

	CMP     BYTE PTR [SI],0
	JE      ICMP_ECHO_REPLY

	CMP     BYTE PTR [SI],8
	JE      ICMP_ECHO_REQUEST

ICMPEND:

	POP     SI

	JMP     IPLOOP

ICMP_ECHO_REQUEST:

	MOV     SI,OFFSET ECHOMSG
	CALL    NORMSTR

	JMP     ICMPEND                 ; DISPLAY IN HEX

ICMP_ECHO_REPLY:

	MOV     SI,OFFSET ECHOREPLY
	CALL    NORMSTR

	JMP     ICMPEND                 ; DISPLAY IN HEX

RSPFMSG:

	JMP     RSPFPROC

TCPMSG:
;
;       IP POINTS TO TCP HEADER, CX = LENGTH
;
	PUSH    SI
	MOV     SI,OFFSET TCP_MSG
	CALL    NORMSTR

	POP     SI

	MOV     AL,0CH[SI]              ; LENGTH (WORDS) TOP 4 BITS
	SHR     AL,1
	SHR     AL,1
	AND     AL,11111100B
	MOV     AH,0

	SUB     CX,AX
	TEST    CH,80H
	JZ      TCPLENOK
;
;       LENGTH IS NEGATIVE
;
BADTCPLEN:

	MOV     SI, OFFSET BADLENMSG
	CALL    NORMSTR

	JMP     TCP_END

TCPLENOK:

	PUSH    CX                      ; SAVE LENGTH AFTER HEADER

	MOV     CX,AX
;
;       DISPLAY TCP HEADER IN HEX
;
	MOV     AX,SOURCEPORT[SI]
	XCHG    AH,AL
	CALL    CONV_5DIGITS

	MOV     AL,'>'
	CALL    PUTCHAR

	MOV     AX,DESTPORT[SI]
	XCHG    AH,AL
	CALL    CONV_5DIGITS

	CALL    SPACE

	ADD     SI,4

	CALL    HEX4                    ; SEQNO

	CALL    HEX4                    ; ACKNO

	LODSB
	CALL    BYTE_TO_HEX             ; CONTROL
	CALL    SPACE

	LODSB
	CALL    BYTE_TO_HEX             ; FLAGS
	CALL    SPACE

	LODSW
	XCHG    AH,AL                   ; WINDOW

	CALL    CONV_5DIGITS
	CALL    SPACE

	CALL    HEX2                    ; CHECKSUM
	CALL    HEX2                    ; URG
	
	SUB     CX,20
	JCXZ    TCP_HDDR_END

	CMP	CH,0
	JNE	TCP_HDDR_END		; BAD LENGTH

TCPLOOP:
;
;       PRINT REST OF HEADER (TCP OPTIONS)
;
	LODSB
	CALL    BYTE_TO_HEX             ; OPTIONS

	LOOP    TCPLOOP

TCP_HDDR_END:

	POP     CX                      ; DATA LENGTH
	JCXZ    TCP_END

	CMP     CH,0
	JNE     TCP_END                 ; DUFF - SHOULDNT BE MORE THAN 256
;
;       INFO FOLLOWS - DISPLAY IT 
;
	MOV     AL,20H
	CALL    PUTCHAR

	MOV     AL,CL
	CALL    CONV_DIGITS

	MOV     AL,20H
	CALL    PUTCHAR

TCPLOOP1:

	LODSB
	PUSH    CX
	CALL    PUTCHAR
	POP     CX

	LOOP    TCPLOOP1

TCP_END:

	JMP     ADD_CR


UDPMSG:
;
;       IP POINTS TO UDP HEADER, CX = LENGTH
;
	PUSH    SI
	MOV     SI,OFFSET UDP_MSG
	CALL    NORMSTR
	POP     SI

	PUSH    CX                      ; SAVE LENGTH AFTER HEADER
;
;       DISPLAY UDP HEADER IN HEX
;
	MOV     AX,SOURCEPORT[SI]
	XCHG    AH,AL
	CALL    CONV_5DIGITS

	MOV     AL,'>'
	CALL    PUTCHAR

	MOV     AX,DESTPORT[SI]
	XCHG    AH,AL
	MOV     UDPDESTPORT,AX          ; SAVE

	CALL    CONV_5DIGITS

	CALL    SPACE

	ADD     SI,4

	CALL    HEX2                    ; LENGTH

	CALL    HEX2                    ; CHECKSUM

	POP     CX                      ; DATA LENGTH
	SUB     CX,8                    ; HEADER

	JCXZ    UDP_END

	CMP     CH,0
	JNE     UDP_END                 ; DUFF - SHOULDNT BE MORE THAN 256

	CALL    SPACE

	CMP     UDPDESTPORT,520
	JE      RIPMSG
;
;       INFO FOLLOWS - DISPLAY IT 
;
	MOV     AL,CL
	CALL    CONV_DIGITS

	MOV     AL,20H
	CALL    PUTCHAR

UDPLOOP1:

	LODSB
	PUSH    CX
	CALL    BYTE_TO_HEX
	POP     CX

	LOOP    UDPLOOP1

UDP_END:

	JMP     ADD_CR


RIPMSG:

	CMP	BYTE PTR 1[SI],'b'
	JNE	NOMORERIP

	SUB     CX,4
	ADD     SI,4                    ; REMOVE HEADER

DRIPLOOP:

	CMP     CX,6
	JB      NOMORERIP

	CALL    PRINT4                  ; DISPLAY ADDR
	MOV	AL,'/'
	CALL    PRINTIT
	LODSB                           ; SUBNET
	CALL    CONV_DIGITS

	CALL    SPACE

	LODSB                           ; METRIC
	CALL    CONV_DIGITS

	CALL    SPACE
	CALL    SPACE

	SUB     CX,6
	JMP     DRIPLOOP

NOMORERIP:

	JMP     ADD_CR

ARPMSG:

	CMP     TRACEFLAG,1
	JE      DOARPTRACE

	RET

DOARPTRACE:

	PUSH    SI
	PUSH    BX

	MOV	ARP_MSG+1,'I'
	CALL    ARPTRACE

	POP     BX
	POP     SI
;
	SUB     SI,16                   ; REPLACE HEADER

;       PROCESS MESSAGE
;
	PUSH    BX                      ; SAVE BUFFER
	MOV     BX,SI                   ; BASE OF MSG

	PUSH    BX
	CALL    ADDTOOURTABLE
	POP     BX

	CMP     ARPOPCODE[BX],100H
	JE      ARPREQUEST

ARPCOMPLETE:
;
;       RELEASE BUFFER
;
	POP     DI

	CALL    IPRELBUFF
	RET

ARPREQUEST:
;
;       IF FOR OUR IP ADDRESS, REPLY TO IT
;
	CMP4    MYIPADDR,REPLYIPADDR[BX]
	JNE     ARPCOMPLETE

SENDARPREPLY:
;
;       SEND REPLY TO INCOMING ARP REQUEST
;
	LEA     SI,ARPSOURCE[BX]        ; SEND ACK TO REQUESTING STN

	LEA     DI,ARPREPLYBUFFER.ARPTARGET
	MOV     CX,7
	REP MOVSB
	AND     BYTE PTR -1[DI],0FEH    ; CLEAR END OF CALL

	LEA     SI,MYCALL
	LEA     DI,ARPREPLYBUFFER.ARPSOURCE
	MOV     CX,7
	REP MOVSB
	OR      BYTE PTR -1[DI],1       ; SET SEND OF CALL
;
	LEA     SI,MYCALL
	LEA     DI,ARPREPLYBUFFER.SENDHWADDR
	MOV     CX,7
	REP MOVSB

	LEA     SI,REPLYIPADDR[BX]      ; WANTED ADDR
	MOV     CX,4
	REP MOVSB

	LEA     SI,SENDHWADDR[BX]       ; ORIGINATORS ADDR
	MOV     CX,11
	REP MOVSB

;       INC4    ARP_REPLIES

	MOV     SI,OFFSET ARPREPLYBUFFER
	MOV     CX,46
;
	MOV     AH,10
	MOV     AL,PORT_NO
	CALL    NODE                    ; SEND DATAGRAM
;
	JMP     ARPCOMPLETE

ADDTOOURTABLE:
;
;       LOOK FOR ENTRY IN ARP TABLE - IF THERE UPDATE, ELSE ADD
;
	MOV     DI,OFFSET ARPTABLE
	MOV     CX,MAXARP
	MOV     DX,0

ARPREPLYLOOP:

	CMP4    [DI],SENDIPADDR[BX]
	JE      FOUNDENTRY

	CMP     BYTE PTR [DI],0
	JNE     GOTSPARE

	CMP     DX,0
	JNE     GOTSPARE

	MOV     DX,DI                   ; SAVE FIRST SPARE

GOTSPARE:

	ADD     DI,TYPE ARPDATA
	LOOP    ARPREPLYLOOP
;
;       NOT FOUND - ADD IT IF ANY SPARE ENTRIES
;
	CMP     DX,0
	JE      ADDRET

	MOV     DI,DX
	MOV4    [DI],SENDIPADDR[BX]

	MOV     AL,PORT_NO
	MOV     ARPINTERFACE[DI],AL

	MOV	ARPTYPE[DI],'V'

FOUNDENTRY:

	MOV     ARPVALID[DI],1
	
	TEST    ARPTIMER[DI],8000H
	JNZ     ARPLOCKED2

	MOV     ARPTIMER[DI],30*60      ; 16 MINS TIMEOUT

ARPLOCKED2:

	PUSH    DI                      ; SAVE ARP ENTRY

	LEA     DI,TARGETHWADDR[DI]
	LEA     SI,SENDHWADDR[BX]
	MOV     CX,7
	REP MOVSB                       ; PUT ADDR INTO TABLE   
;
;       IF ANY STUFF WAITING TO GO, SEND IT
;
	POP     DI

ARPSENDLOOP:

	PUSH    DI                      ; SAVE ARP ENTRY

	LEA     SI,ARP_Q[DI]
	CALL    Q_REM
	JZ      NOTHINGTOSEND
;
	MOV     BX,DI                   ; BUFFER

	POP     SI                      ; ARP ENTRY
	PUSH    SI                      ; SAVE AGAIN

	CALL    SENDTONODE              ; ADD ADDRESS AND PUT ON TX QUEUE
	POP     DI
;
	JMP     ARPSENDLOOP
;
NOTHINGTOSEND:

	POP     DI

ADDRET:

	RET
;

ARPTRACE:

	PUSH    SI
	MOV     SI,OFFSET ARP_MSG
	CALL    NORMSTR
	POP     SI

	MOV     CX,8

ARPLOOP1:

	LODSB
	CALL    BYTE_TO_HEX
	LOOP    ARPLOOP1                ; HEADER INFO
;
;       ORIGINATORS HW ADDR
;
	PUSH    SI

	call    CONVFROMAX25
	mov     SI,OFFSET NORMCALL
	call    DISPADDR

	POP     SI
	ADD     SI,7

	MOV     AL,'/'
	CALL    PUTCHAR

	CALL    PRINT4                  ; IP ADDR

	MOV     AL,20H
	CALL    PUTCHAR
;
;       TARGET CALL/ADDR
;
	PUSH    SI

	call    CONVFROMAX25
	mov     SI,OFFSET NORMCALL
	call    DISPADDR

	POP     SI
	ADD     SI,7

	MOV     AL,'/'
	CALL    PUTCHAR

	CALL    PRINT4                  ; IP ADDR

	JMP     ADD_CR



RSPFPROC:


	PUSH    SI
	MOV     SI,OFFSET RSPF_MSG
	CALL    NORMSTR
	POP     SI

	PUSH    SI

	CMP     BYTE PTR 1[SI],3
	JE      RSPF_RRH

	POP     SI

	JMP     IPLOOP

RSPF_RRH:

	MOV     SI,OFFSET RRH_MSG
	CALL    NORMSTR

	POP     SI

	ADD     SI,4

	CALL    PRINT4

	MOV     AL,20H
	CALL    PUTCHAR

	LODSB
	CALL    BYTE_TO_HEX
	LODSB
	CALL    BYTE_TO_HEX                     ; SEQ

	MOV     AL,20H
	CALL    PUTCHAR

	LODSB
	CALL    BYTE_TO_HEX             ; FLAGS

	MOV     AL,20H
	CALL    PUTCHAR

	SUB     CX,11

	JCXZ	NORRHTXT

CHARLOOP:

	lodsb
	CALL    PUTCHAR

	LOOP    CHARLOOP

NORRHTXT:

	JMP     NO_INFO


;15  03    AE7D  2C 83 04 43  010A  00    52 53 50 46 20 72 6F 75 74 69 6E 67 
;VER TYPE  SUM   44.131.4.67  SEQ   FLAGS R  S  P  F     r  o  u  t  i  n  g  
; 
;20 69 6E 20 75 73 65 2E 20 20 52 49 50 20 64 69 73 61 62 6C 65 64 0D0A
;   i  n     u  s  e  .        R  I  P     D  I  S  A  B  L  E  D



DISPROUTES:

	MOV     BP,OFFSET DEFAULTGATEWAY
	MOV     CX,NUMBEROFROUTES
	INC     CX                      ; INCLUDE DEFAULT

ROUTLOOP1:

	MOV     AL,0DH
	CALL    PUTCHAR

	PUSH    CX

	LEA     SI,NETWORK[BP]
	CALL    DISPLAYIPADDR

	MOV     AL,'/'
	CALL    PUTCHAR

	MOV     AX,0
	MOV     BX,WORD PTR DS:SUBNET[BP]
	CALL    COUNTBITS
	MOV     BX,WORD PTR DS:SUBNET+2[BP]
	CALL    COUNTBITS

	CALL    CONVTODIGITS
	CALL    PRINTDEC

	MOV     DL,20
	CALL    POSN

	LEA     SI,GATEWAY[BP]
	CALL    DISPLAYIPADDR

	MOV     DL,40
	CALL    POSN

	MOV     AL,DS:PORT[BP]
	ADD     AL,'0'
	CALL    PRINTIT

	CALL	SPACE
	CALL	SPACE

	MOV     AL,DS:RTYPE[BP]
	CALL    PRINTIT

	CALL	SPACE
	CALL	SPACE

	MOV     AL,DS:METRIC[BP]
	CBW
	CALL    CONVTODIGITS
	CALL    PRINTDEC

	CALL	SPACE
	CALL	SPACE

	MOV	AL,DS:ROUTEINFO[BP]
	CALL	BYTE_TO_HEX

	CALL	SPACE
	CALL	SPACE

	MOV     AL,DS:RIPTIMOUT[BP]
	CALL	BYTE_TO_HEX
;
	CALL	SPACE
	CALL	SPACE

	LEA     SI,FRAMECOUNT[BP]
	CALL    CONVTO10DIGITS

	POP     CX

	ADD     BP,TYPE ROUTEENTRY
	DEC	CX
	JZ	ROUTLOOPEND

	JMP	ROUTLOOP1

ROUTLOOPEND:

	RET

COUNTBITS:
;
;       COUNT NUBER OF 1 BITS IN BX
;
	MOV     CX,16

COUNTL:

	TEST    BX,1
	JZ      CSKIP

	INC     AL

CSKIP:
	SHR     BX,1
	LOOP    COUNTL

	RET

DISPSTATS:

	MOV     AL,0DH
	CALL    PUTCHAR

	MOV     SI,OFFSET STATSMINS
	CALL    NORMSTR

	LEA     SI,MINSACTIVE
	CALL    CONVTO10DIGITS

	MOV     AL,0DH
	CALL    PRINTIT

	MOV     SI,OFFSET STATSFRAMES
	CALL    NORMSTR

	LEA     SI,FRAMESSENT
	CALL    CONVTO10DIGITS

	MOV     AL,0DH
	CALL    PRINTIT

	MOV     SI,OFFSET STATSBYTES
	CALL    NORMSTR

	LEA     SI,BYTESSENT
	CALL    CONVTO10DIGITS

	MOV     AL,0DH
	CALL    PRINTIT

	MOV     SI,OFFSET STATCANTROUTE
	CALL    NORMSTR

	LEA     SI,CANTROUTECOUNT
	CALL    CONVTO10DIGITS

	MOV     AL,0DH
	CALL    PRINTIT

	MOV     SI,OFFSET STATARPFULL
	CALL    NORMSTR

	LEA     SI,ARPFULLCOUNT
	CALL    CONVTO10DIGITS

	MOV     AL,0DH
	CALL    PRINTIT

	MOV     SI,OFFSET STATSUMERROR
	CALL    NORMSTR

	LEA     SI,BADIPSUM
	CALL    CONVTO10DIGITS

	MOV     AL,0DH
	CALL    PRINTIT

	MOV     SI,OFFSET STATBUFFS
	CALL    NORMSTR

	MOV     AL,BUFFERCOUNT
	CALL    CONV_DIGITS

	MOV     AL,'/'
	CALL    PRINTIT

	MOV     AL,MINBUFFS
	CALL    CONV_DIGITS

	MOV     AL,0DH
	CALL    PRINTIT

	MOV     SI,OFFSET STATSROUTES
	CALL    NORMSTR

	MOV     AX,NUMBEROFROUTES
	CALL    CONV_DIGITS

	MOV     AL,0DH
	CALL    PRINTIT

	RET

DISPARP:

	MOV     CX,MAXARP
	MOV     SI,OFFSET ARPTABLE

DISPARPLOOP:

	CMP	BYTE PTR [SI],0
	JE	NEXTDISP

	PUSH	SI

	MOV     AL,0DH
	CALL    PRINTIT

	PUSH    CX

	PUSH    SI

	ADD     SI,4
	call    CONVFROMAX25
	mov     SI,OFFSET NORMCALL
	MOV     CX,10
	call    DISPADDR

	POP     SI

	MOV     AL,20H
	CALL    PRINTIT
	MOV     AL,20H
	CALL    PRINTIT

	CALL    DISPLAYIPADDR

	MOV     DL,35
	CALL    POSN

	ADD     SI,8
	LODSW                           ; TIMER

	CALL    CONVTODIGITS
	CALL    PRINTDEC

	MOV     AL,20H
	CALL    PRINTIT

	LODSB
	CALL    BYTE_TO_HEX

	MOV     AL,20H
	CALL    PRINTIT

	LODSB
	CALL    PUTCHAR

	POP     CX
	POP	SI

NEXTDISP:

	ADD     SI,TYPE ARPDATA

	LOOP    DISPARPLOOP

	RET
;
DISPRIP:

	MOV     AL,0DH
	CALL    PRINTIT

	MOV	CX,NUMBEROFNEIGHBOURS
	JCXZ	NODISPRIP

	MOV	SI,OFFSET NEIGHBOURS

RIPDISPLOOP:

	PUSH	CX

	CALL	DISPLAYIPADDR

	CALL	SPACE
	LODSB
	CALL	PRINTIT
	CALL	SPACE
	CALL	SPACE

	POP	CX

	LOOP	RIPDISPLOOP

NODISPRIP:

	RET






POSN:

	CMP     BYTE PTR CHAR,DL
	JA      POSNOK

	MOV     AL,20H
	CALL    PRINTIT

	JMP     POSN

POSNOK:
	RET


DISPLAYIPADDR:
;
;       DISPLAY 4 BYTE IP ADDRS IN 'DOTTED DECIMAL' FORM
; 
	MOV     CX,4
	JMP SHORT DISPIP01
DISPIP00:
	MOV     AL,'.'
	CALL    PRINTIT
DISPIP01:
	LODSB
	MOV     AH,0
	CALL    CONVTODIGITS
	CALL    PRINTDEC
	LOOP    DISPIP00

	RET


PRINTIT:
	
	CMP     FILEIT,0
	JE      DISPIT
;        
;       PUT IN DISK BUFFER
;
	STOSB
	RET
	
DISPIT:
	
	PUSH    DX
	CALL    PUTCHAR
	POP     DX
	RET

;
CONVTODIGITS:
;
;       CONVERT AX TO 5 DECIMAL DIGITS AT DECNUM
;
	PUSH    CX
	PUSH    DI
	MOV     DI,OFFSET DECNUM+4
	MOV     CX,5

CONVD00:

	MOV     DX,0
	DIV     W10

	ADD     DL,30H
	MOV     [DI],DL
	DEC     DI

	LOOP    CONVD00

	POP     DI
	POP     CX
	RET

PRINTDEC:
;
;       DISPLAY SIGNICICANT DIGITS FROM DECNUM
;
	PUSH    SI
	PUSH    CX

	MOV     SI,OFFSET DECNUM
	MOV     CX,5

PRINTD00:

	LODSB
	CMP     AL,'0'
	JNZ     PRINTD20                ; ALL LEADING ZEROS SKIPPED

	LOOP    PRINTD00
	CALL    PRINTIT 
	POP     CX
	POP     SI
	RET                             ; JUST A ZERO

PRINTD10:
	LODSB
PRINTD20:
	CALL    PRINTIT
	LOOP    PRINTD10

	POP     CX
	POP     SI
	RET

CONVTO10DIGITS:
;
;       CONVERT DOUBLE WORD COUNTER AT [SI]
;
	LODSW
	MOV     ACCUM,AX
	LODSW
	MOV     ACCUM+2,AX

	MOV     SUPPRESS,1              ; ZERO SUPRESS;

	PUSH    BX
	PUSH    SI
	PUSH    CX

	MOV     CONVFLAG,0
	MOV     CX,8

	MOV     SI,OFFSET POWERS        ; LOW ORDER OF FIRST
DEC010:
	MOV     BL,0                    ; LOOP COUNT FOR SUBTRACT
DEC020:
	CALL    SUB4X
	JC      DEC030                  ; TOO SMALL
	INC     BL                      ; NUMBER OF SUBTRACTS
	JMP     DEC020
;
DEC030:
	CALL    ADD4X                   ; PUT BACK
	MOV     AL,BL
	ADD     AL,'0'                  ; TO CHARACTER FORM
	TEST    CONVFLAG,1
	JNZ     DEC040                  ; ALREADY HAVE A SIGNIFICANT CHAR
	CMP     AL,'0'                  ; IS IT A ZERO
	JE      DEC036                  ; YES, SO CHANGE TO SPACE
	OR      CONVFLAG,1              ; SET NON-ZERO CHAR FOUND
	JMP SHORT DEC040                ; AND PRINT IT
;
DEC036:
	CMP     SUPPRESS,1
	JE      DEC044                  ; SUPPRESS LEADING SPACES

	MOV     AL,' '
DEC040:
	CALL    PRINTIT
DEC044:
	ADD     SI,4
	LOOP    DEC010

	MOV     AL,BYTE PTR ACCUM       ; REMAINDER
	ADD     AL,'0'
	CALL    PRINTIT
;
	POP     CX
	POP     SI
	POP     BX

	RET
;
;       ROUTINE TO ADD TWO 4 BYTE NUMBERS
;
;
ADD4X:
	MOV     AX,[SI]
	ADD     ACCUM,AX
	MOV     AX,[SI+2]
	ADC     ACCUM+2,AX

	RET
;
;       ROUTINE TO SUBTRACT TWO 4 BYTE NUMBERS
;
SUB4X:
	MOV     AX,[SI]
	SUB     ACCUM,AX
	MOV     AX,[SI+2]
	SBB     ACCUM+2,AX
	RET

NORMSTR:

	lodsb
	cmp     AL,NULL         ; End of String ?
	je      NORMSTR_RET     ; Yes
	call    PUTCHAR
	jmp     SHORT NORMSTR

NORMSTR_RET:
	ret

;-----------------------------------------------------------------------------;
;       Display sequence numbers                                              ;
;-----------------------------------------------------------------------------;

DISPLAYSEQ:
	and     AL,7
	add     AL,30H
	call    PUTCHAR
	ret

;-----------------------------------------------------------------------------;
;       Display Callsign pointed to by SI                                     ;
;-----------------------------------------------------------------------------;

DISPADDR:

	JCXZ    DISPADDR_RET

	lodsb
	call    PRINTIT

	loop    DISPADDR

DISPADDR_RET:

	ret


PRINT4:
;
;       DISPLAY IP ADDR IN DOTTED DECIMAL FORMAT
;

	LODSB
	CALL    DISPLAY_BYTE_1
	MOV     AL,'.'
	CALL    PUTCHAR

	LODSB
	CALL    DISPLAY_BYTE_1
	MOV     AL,'.'
	CALL    PUTCHAR

	LODSB
	CALL    DISPLAY_BYTE_1
	MOV     AL,'.'
	CALL    PUTCHAR

	LODSB
	CALL    DISPLAY_BYTE_1

	RET

HEX4:

	REPT    2
	LODSB
	CALL    BYTE_TO_HEX
	ENDM

HEX2:
	REPT    2
	LODSB
	CALL    BYTE_TO_HEX
	ENDM

SPACE:

	MOV     AL,20H
	CALL    PRINTIT

	RET

CONV_DIGITS:

	MOV     AH,0

CONV_5DIGITS:

	PUSH    DX
	PUSH    BX

	MOV     BX,OFFSET TENK          ; 10000

	CMP     AX,10
	JB      UNITS                   ; SHORT CUT AND TO STOP LOOP

START_LOOP:

	cmp     AX,WORD PTR [BX]
	JAE     STARTCONV

	ADD     BX,4                    ; DWORD ENTRIES

	JMP SHORT START_LOOP
;
STARTCONV:

	MOV     DX,0
	DIV     WORD PTR [BX]           ; 
	ADD     AL,30H                  ; MUST BE LESS THAN 10

	PUSH    DX
	PUSH    BX
	CALL    PUTCHAR
	POP     BX
	POP     DX
;
	MOV     AX,DX                   ; REMAINDER

	ADD     BX,4
	CMP     BX,OFFSET DWORD10+4     ; JUST DIVIDED BY 10?
	JNE     STARTCONV               ; NO, SO ANOTHER TO DO
;
;       REST MUST BE UNITS
;
UNITS:

	add     AL,30H
	CALL    PUTCHAR

	POP     BX
	POP     DX

	ret

	
;-----------------------------------------------------------------------------;
;       Convert byte in AL to nnn, nn or n format                             ;
;-----------------------------------------------------------------------------;

DISPLAY_BYTE_1:

	cmp     AL,100
	jb      TENS_1

	mov     AH,0

HUNDREDS_LOOP_1:

	cmp     AL,100
	jb      HUNDREDS_LOOP_END_1

	sub     AL,100
	inc     AH
	jmp     SHORT HUNDREDS_LOOP_1

HUNDREDS_LOOP_END_1:

	push    AX
	mov     AL,AH
	add     AL,30H
	call    PUTCHAR
	pop     AX
	jmp     SHORT TENS_PRINT_1

TENS_1:
	cmp     AL,10
	jb      UNITS_1

TENS_PRINT_1:
	mov     AH,0

TENS_LOOP_1:
	cmp     AL,10
	jb      TENS_LOOP_END_1

	sub     AL,10
	inc     AH
	jmp     SHORT TENS_LOOP_1

TENS_LOOP_END_1:
	push    AX
	mov     AL,AH
	add     AL,30H
	call    PUTCHAR
	pop     AX

UNITS_1:
	add     AL,30H
	call    PUTCHAR

	ret

;-----------------------------------------------------------------------------;
;       Convert byte in AL to nn format                                       ;
;-----------------------------------------------------------------------------;

DISPLAY_BYTE_2:
	cmp     AL,100
	jb      TENS_2

	sub     AL,100
	jmp     SHORT DISPLAY_BYTE_2

TENS_2:
	mov     AH,0

TENS_LOOP_2:
	cmp     AL,10
	jb      TENS_LOOP_END_2

	sub     AL,10
	inc     AH
	jmp     SHORT TENS_LOOP_2

TENS_LOOP_END_2:
	push    AX
	mov     AL,AH
	add     AL,30H
	call    PUTCHAR
	pop     AX

UNITS_2:
	add     AL,30H
	call    PUTCHAR

	ret

;-----------------------------------------------------------------------------;
;       Convert byte in AL to Hex display                                     ;
;-----------------------------------------------------------------------------;

BYTE_TO_HEX:
	push    AX
	shr     AL,1
	shr     AL,1
	shr     AL,1
	shr     AL,1
	call    NIBBLE_TO_HEX
	pop     AX
	call    NIBBLE_TO_HEX
	ret

NIBBLE_TO_HEX:
	and     AL,0FH
	cmp     AL,10

	jb      LESS_THAN_10
	add     AL,7

LESS_THAN_10:
	add     AL,30H
	call    PUTCHAR
	ret

TIDY_LIST:
;
;	REMOVE ANY ENTRIES WHICH ARE COVERED BY A SUBNET ROUTE
;
	MOV     SI,OFFSET ROUTETAB
	MOV     CX,NUMBEROFROUTES
	JCXZ	TIDYRET

TIDYL00:

	CMP	WORD PTR SUBNET+2[SI],0FFFFH
	JE	TIDYL60			; NOT A SUBNET ROUTE
;
;	SEE IF ANY FOLLOWING ENTRIES MATCH THIS ONE
;
	MOV	BX,SI			; SUBNET ENTRY TO COMPARE WITH

	ADD	SI,TYPE ROUTEENTRY

TIDYL20:

	MOV     AX,WORD PTR NETWORK+2[SI]
	AND	AX,WORD PTR SUBNET+2[BX]
	CMP     AX,WORD PTR NETWORK+2[BX]
	JNE	TIDYL40

	MOV     AX,WORD PTR NETWORK[SI]
	AND	AX,WORD PTR SUBNET[BX]
	CMP     AX,WORD PTR NETWORK[BX]
	JNE	TIDYL40
;
;	SAME GATEWAY?
;
	MOV	AX,WORD PTR GATEWAY+2[SI]
	CMP	AX,WORD PTR GATEWAY+2[BX]
	JNE	TIDYL50			; DIFFERENT GATEWAY, SO LEAVE IN

	MOV	AX,WORD PTR GATEWAY[SI]
	CMP	AX,WORD PTR GATEWAY[BX]
	JNE	TIDYL50			; DIFFERENT GATEWAY, SO LEAVE IN
;
	MOV	AL,PORT[SI]
	CMP	AL,PORT[BX]
	JNE	TIDYL50			; DIFERENT PORT, SO LEAVE
;
;	ROUTE USES SAME GATEWAY, SO CAN REMOVE IT
;
	CALL	REMOVEROUTE		; DELETE IT
	DEC	CX
;
;	SI POINTS TO 'NEW' NEXT, CX HAS BEEN ADJUSTED
;
	JCXZ	TIDYL90			; JUST DELETED LAST ENTRY

	JMP	TIDYL20			; SEE IF THIS ONE CAN BE REMOVED

TIDYL40:
;
;	A NEW SUBNET GROUP - RESUME SCAN FOR NEXT SUBNET ENTRY
;
	LOOP	TIDYL00
	RET

TIDYL50:

	ADD	SI,TYPE ROUTEENTRY
	LOOP	TIDYL20

TIDYRET:

	RET

TIDYL60:

	ADD	SI,TYPE ROUTEENTRY
	LOOP	TIDYL00

TIDYL90:

	RET


CONVFROMAX25:
;
;       CONVERT AX25 FORMAT CALL IN [SI] TO NORMAL FORMAT IN NORMCALL
;          RETURNS LENGTH IN CX AND NZ IF LAST ADDRESS BIT IS SET
;
	PUSH    SI                      ; SAVE
	MOV     DI,OFFSET NORMCALL
	MOV     CX,10                   ; MAX ALPHANUMERICS
	MOV     AL,20H
	REP STOSB                       ; CLEAR IN CASE SHORT CALL
	MOV     DI,OFFSET NORMCALL
	MOV     CL,6
CONVAX50:
	LODSB
	CMP     AL,40H
	JE      CONVAX60                ; END IF CALL - DO SSID

	SHR     AL,1
	STOSB
	LOOP    CONVAX50
CONVAX60:
	POP     SI
	ADD     SI,6                    ; TO SSID
	LODSB
	MOV     AH,AL                   ; SAVE FOR LAST BIT TEST
	SHR     AL,1
	AND     AL,0FH
	JZ      CONVAX90                ; NO SSID - FINISHED
;
	MOV     BYTE PTR [DI],'-'
	INC     DI
	CMP     AL,10
	JB      CONVAX70
	SUB     AL,10
	MOV     BYTE PTR [DI],'1'
	INC     DI
CONVAX70:
	ADD     AL,30H                  ; CONVERT TO DIGIT
	STOSB
CONVAX90:
	MOV     CX,DI
	SUB     CX,OFFSET NORMCALL
	MOV     NORMLEN,CX              ; SIGNIFICANT LENGTH

	TEST    AH,1                    ; LAST BIT SET?
	RET

NODE:
	CMP     AH,10                   ; RAW SEND?
	JE      NODEINT                 ; PORT ALREADY SET

	MOV     AL,BPQPORT

IPNODE:
NODEINT:

	PUSHF				; RETURN IS VIA IRET
	CALL	BPQHOSTPORT
	RET







DOCRLF:

	MOV     AL,CR
	CALL    PRINTIT
	MOV     AL,LF
	CALL    PRINTIT

	RET


IPGATEINIT:
;
;	DS:SI POINTS TO MYCALL
;
	MOV	AX,IPDATA
	MOV	ES,AX

	MOV     DI,OFFSET MYCALL
	MOV     CX,7
	REP MOVSB
;
	MOV	DS,AX

	CMP	BWFLAG,4DH
	JNE	COLMODE
;
	MOV	NORMATTR,15		; NORMAL CHARS
;
COLMODE:
;
;       BUILD BUFFER POOL
;
	MOV     CX,NUMBEROFRXBUFFERS
	MOV     DI,OFFSET TCPRXBUFFERS

BUFFERLOOP:

	MOV     SI,OFFSET FREE_Q
	CALL    Q_ADDF

	ADD     DI,PACKETBUFFERLEN
	LOOP    BUFFERLOOP

	CALL    GETTIME

	MOV     AX,WORD PTR DATE
	MOV     WORD PTR LOGNAME+3,AX

	MOV     AX,WORD PTR DATE+3
	MOV     WORD PTR LOGNAME+5,AX

	CALL	OPENLOG

;
	MOV     DX,OFFSET DATE
	MOV     CX,18
	MOV     BX,LOGHANDLE
	MOV     AH,40H
	INT     21H                     ; WRITE DATE/TIME TO FILE

	CALL    PUTCRLF

	CALL	CLOSELOG

	CALL    REFRESHTL
	CALL    REFRESHBL
;
;       CLEAR SCROLL WINDOW
;
	MOV     AH,6
	MOV     AL,0
	MOV     CH,2
	MOV     CL,0
	MOV     DH,21
	MOV     DL,79
	MOV     BH,2

	INT     10H
;
;       READ ROUTER CONFIG FILE
;
	MOV     DX,OFFSET FILENAME
	MOV     AH,3DH
	MOV     AL,0                    ; READ ONLY
	INT     21H
	JNC     FILEOPENOK

	MOV     SI,OFFSET OPENERROR
	CALL    NORMSTR

	MOV     AX,4C00H
	RET

FILEOPENOK:

	MOV     HANDLE,AX
;
INITLOOP:

	MOV     VALERROR,0

	CALL    GETRECORD
	CMP     AL,1AH
	JNE     NOTINITEND

	MOV     BX,HANDLE
	MOV     AH,3EH
	INT     21H                     ; CLOSE FILE


	MOV     SI,OFFSET MYCALL
	MOV     DI,OFFSET MYADDR1
	MOV     CX,7
	REP MOVSB

	OR      BYTE PTR -1[DI],1       ; END OF CALL BIT

	MOV     SI,OFFSET MYCALL
	MOV     DI,OFFSET MYADDR2
	MOV     CX,7
	REP MOVSB

	CALL	TIDY_LIST

	CMP	BYTE PTR MYIPADDR,0
	JE	DONT_INIT

	MOV     AH,1
	MOV     DX,2                    ; APPL MASK
	MOV     CX,80H                  ; APPL FLAGS

	CALL    NODE                    ; SET APPL MASK TO ENABLE MONITORING
;
	IF	NEEDTCP

	CALL	TCPINIT

	ENDIF

	CMP	BYTE PTR MYETHIPADDR,0
	JE	DONT_INIT		; DONT ACTIVATE E'NET DRIVER

	MOV	AL,ETHINT
	MOV	CS:ODIINT+1,AL		; PATCH IN INTERRUPT NUMBER

DONT_INIT:

	MOV	AX,0
	RET

NOTINITEND:

	MOV     SI,OFFSET KBBUFF
	CALL    PROCESSCOMMAND
	
	JMP     INITLOOP        
	
PROCESSCOMMAND:        

	MOV     DI,OFFSET KEYWORD       ; CLEAR KEYWORD
	MOV     CX,10
	MOV     AX,2020H
	REP STOSW

	MOV     DI,OFFSET PARAMS        ; CLEAR PARAMS
	MOV     CX,20
	MOV     AX,2020H
	REP STOSW

	MOV     CX,78
INITXX:
	LODSB
	CMP     AL,';'
	JE      PROCRET                ; IGNORE COMMENTS

	CMP     AL,'#'
	JE      PROCRET                ; IGNORE COMMENTS

	CMP     AL,20H
	JNE     FOUNDFIRST

	LOOP    INITXX

PROCRET:        
	RET
	
FOUNDFIRST:
;
;       IDENTIFY COMMAND
;
	MOV     CX,10
	MOV     DI,OFFSET KEYWORD
INITYY:
	STOSB
	LODSB
	CMP     AL,'='
	JE      INITZZ                  ; END OF KEYWORD

	CMP     AL,' '
	JE      INITZZ                  ; END OF KEYWORD

	LOOP    INITYY

INVALIDPARAM_J:

	JMP     INVALIDPARAM            ; INVALID

INITZZ:

	MOV     PARAMPOINTER,SI         ; SAVE POINTER TO PARAMETER


	MOV     SI,OFFSET KEYWORD
	MOV     DI,OFFSET KEYIPADDRESS
	MOV     CX,5
	REP CMPSW
	JNE     NOTIPADDR
;
;       PROCESS MY IPADDRESS PARAM
;
	MOV     SI,PARAMPOINTER
	LEA     DI,MYIPADDR
	
	CALL    GETIPADDR          ; GET A 4 DIGIT ADDRESS

	RET
	
GETIPADDR:        

	MOV     CX,4

IPADLOOP:

	CALL    GETVALUE

	MOV     AX,NEWVALUE
	STOSB

	INC     SI
	LOOP    IPADLOOP
;
	RET        

NOTIPADDR:

	MOV     SI,OFFSET KEYWORD
	MOV     DI,OFFSET KEYETHIPADDR
	MOV     CX,5
	REP CMPSW
	JNE     NOTETHIPADDR
;
;       IP ADDRESS ON E'NET PORT
;
	MOV     SI,PARAMPOINTER
	LEA     DI,MYETHIPADDR
	
	CALL    GETIPADDR          ; GET A 4 DIGIT ADDRESS

	RET
	
NOTETHIPADDR:

	MOV     SI,OFFSET KEYWORD
	MOV     DI,OFFSET KEYLEARN
	MOV     CX,5
	REP CMPSW
	JNE     NOTLEARN
;
	MOV     SI,PARAMPOINTER
	LODSB
	MOV	LEARNFLAG,AL
	RET
	
NOTLEARN:


	MOV     SI,OFFSET KEYWORD
	MOV     DI,OFFSET KEYTTL
	MOV     CX,5
	REP CMPSW
	JNE     NOTTTL
;
	MOV     SI,PARAMPOINTER
	CALL	GETVALUE
	MOV	AX,NEWVALUE

	MOV	TTL,AL
	MOV	ICMPTTL,AL

	RET
	
NOTTTL:

	MOV     SI,OFFSET KEYWORD
	MOV     DI,OFFSET KEYROUTE
	MOV     CX,5
	REP CMPSW
	JE      PROCROUTE

	MOV     SI,OFFSET KEYWORD
	MOV     DI,OFFSET KEYARP
	MOV     CX,5
	REP CMPSW
	JE      PROCARP

	MOV     SI,OFFSET KEYWORD
	MOV     DI,OFFSET KEYRIP
	MOV     CX,5
	REP CMPSW
	JE      PROCRIP

	JMP     INVALIDPARAM

PROCROUTE:

	CALL    PROCROUTECODE
	CMP     VALERROR,0
	JE      INITLOOP_J

	JMP     INVALIDPARAM

PROCRIP:

	CALL    PROCRIPCODE
	CMP     VALERROR,0
	JE      INITLOOP_J

	JMP     INVALIDPARAM

PROCARP:

	CALL    PROCARPCODE

	CMP     VALERROR,0
	JNE     INVALIDPARAM

INITLOOP_J:

	RET       

INVALIDPARAM:
;
	MOV     SI,OFFSET INVALIDMSG
	CALL    NORMSTR

	MOV     SI,OFFSET KBBUFF
	MOV     CX,78

INVLOOP:

	LODSB

	CALL    PRINTIT

	LOOP    INVLOOP

	MOV     AL,0DH
	CALL    PRINTIT        

	MOV     INITERROR,1

	RET        
;
;       PROCESS ROUTE COMMAND
;
;       THERE ARE 4 PARAMETERS - NETWORK GATEWAY PORT AND TYPE
;
PROCROUTECODE:
;
	MOV     SI,PARAMPOINTER
	MOV     CX,20
	MOV     DI,OFFSET KEYWORD
INITR00:
	LODSB

	CMP     AL,'='
	JE      INITR10                 ; END OF KEYWORD

	CMP     AL,' '
	JE      INITR10                 ; END OF KEYWORD

	STOSB
	LOOP    INITR00

	JMP     INVALIDPARAM            ; INVALID

INITR10:

	MOV     AL,20H
	REP STOSB                       ; CLEAR REST

	MOV     PARAMPOINTER,SI

	CMP     WORD PTR KEYWORD,'ED'   ; DEFAULT?
	JNE	NOTDEFAULT


	MOV     DI,OFFSET DEFAULTGATEWAY
	MOV	SAVE,DI

	MOV     SI,PARAMPOINTER
	MOV     CX,4

	LEA     DI,GATEWAY[DI]

	CMP     BYTE PTR [SI],'*'       ; DIRECT?
	JNE     GETDGATEWAY
	
	ADD     SI,2

	JMP SHORT GETDPORT
		
GETDGATEWAY:                

	CALL     GETIPADDR  
	
GETDPORT:        

	MOV	DI,SAVE

	CALL    GETVALUE

	MOV     AX,NEWVALUE        
	MOV	PORT[DI],AL

	INC     SI
	LODSB

	CMP     AL,'E'                  ; ETHERNET
	JE      DTYPEOK

	CMP     AL,'V'                  ; VC
	JE      DTYPEOK
	CMP     AL,'D'                  ; DG
	JE      DTYPEOK
	CMP     AL,'N'                  ; NETROM
	JNE     DBADROUTE 
	
	CMP     NEWVALUE,0              ; NETROM MUST USE PORT 0

	JE      DTYPEOK

DBADROUTE:

	MOV     VALERROR,1

DTYPEOK:

	MOV	RTYPE[DI],AL

	MOV	METRIC[DI],1		; METRIC

	CMP	BYTE PTR GATEWAY[DI],0
	JE	DGOTMETRIC

	INC	METRIC[DI]		; INDIRECT

DGOTMETRIC:

	MOV	ROUTEINFO[DI],LOCKED	; ROUTE TYPE

	RET

NOTDEFAULT:
;
;       ADD TO ROUTE TABLE
;
	MOV     SI,OFFSET KEYWORD
;
;       GET NETWORK/HOST
;
	MOV	DI,OFFSET TEMPENTRY
	MOV	CX,TYPE ROUTEENTRY
	MOV	AL,0
	REP STOSB
	SUB	DI,TYPE ROUTEENTRY

	CALL    GETIPADDR               ; GET 4 DIGIT ADDRESS        
;
	CMP	BYTE PTR -1[SI],'/'
	JNE	NOSUBMASK
;
;	USER HAS SUPPLIED A SUBNET MASK
;
	CALL	GETVALUE

	MOV	AX,NEWVALUE
	CALL	BITSTOMASK

	MOV	[DI],DH
	INC	DI
	MOV	[DI],DL
	INC	DI
	MOV	[DI],AH
	INC	DI
	STOSB

	JMP SHORT GOTMASK
	
NOSUBMASK:
;
;	SET SUBNET MASK
;
	MOV	AL,0FFH
	STOSB	
	STOSB
	STOSB
	CMP	NEWVALUE,0
	JNE	HOSTROUTE
;
;	SUBNET ROUTE
;
	MOV	AL,0

HOSTROUTE:

	STOSB

GOTMASK:

	MOV     DI,OFFSET TEMPENTRY
	LEA     DI,GATEWAY[DI]

	MOV     SI,PARAMPOINTER
	
	CMP     BYTE PTR [SI],'*'       ; DIRECT?
	JNE     GETGATEWAY
	
	ADD     SI,2

	JMP SHORT GETPORT
		
GETGATEWAY:                

	CALL     GETIPADDR  
	
GETPORT:        

	MOV     DI,OFFSET TEMPENTRY

	CALL    GETVALUE

	MOV     AX,NEWVALUE        
	MOV	PORT[DI],AL

	INC     SI
	LODSB

	CMP     AL,'V'                  ; VC
	JE      TYPEOK
	CMP     AL,'D'                  ; DG
	JE      TYPEOK
	CMP     AL,'N'                  ; NETROM
	JNE     BADROUTE 
	
	CMP     NEWVALUE,0              ; NETROM MUST USE PORT 0

	JE      TYPEOK

BADROUTE:

	MOV     VALERROR,1

TYPEOK:

	MOV	RTYPE[DI],AL

	INC	SI
	CALL    GETVALUE

	MOV	AX,NEWVALUE
	MOV	RIPTIMOUT[DI],AL

	CMP	AL,0
	JNE	GOTRIPTO

	MOV	RIPTIMOUT[DI],MAXRIP

GOTRIPTO:

	INC	SI
	CALL    GETVALUE

	MOV	AX,NEWVALUE
	MOV	METRIC[DI],AL

	OR	AL,AL
	JNZ	GOTMETRIC		; METRIC FROM FILE
;
;	SET DEFAULT METRIC
;
	MOV	METRIC[DI],1		; METRIC

	CMP	BYTE PTR GATEWAY[DI],0
	JE	GOTMETRIC

	INC	METRIC[DI]		; INDIRECT

GOTMETRIC:

	CMP	METRIC[DI],15
	JBE	METVALOK
;
;	UNREACHABLE - SET GAR TIMER
;
	MOV	RIPTIMOUT[DI],0
	MOV	GARTIMOUT[DI],1


METVALOK:

	INC	SI
	CALL    GETVALUE

	MOV	AX,NEWVALUE
	MOV	ROUTEINFO[DI],AL

	CMP	ROUTEINFO[DI],0
	JNE	GOTROUTINFO

	MOV	ROUTEINFO[DI],LOCKED	; ROUTE TYPE

GOTROUTINFO:

	CMP	ROUTEINFO[DI],LOCKED
	JNE	NOTLOCKED

	MOV	RIPTIMOUT[DI],0		; DONT TIME OUT

NOTLOCKED:

	CMP     VALERROR,0
	JNE     LEAVEOUT
;
;	IF ENTRY IS A SUBNET ROUTE TO OUR SUBNET, IGNORE IT
;
	MOV	SI,OFFSET TEMPENTRY
	MOV     AX,WORD PTR MYIPADDR+2
	AND	AX,WORD PTR SUBNET+2[SI]
	CMP     AX,WORD PTR NETWORK+2[SI]
	JNE	NOTOURNET
;
	MOV     AX,WORD PTR MYIPADDR
	AND	AX,WORD PTR SUBNET[SI]
	CMP     AX,WORD PTR NETWORK[SI]
	JNE	NOTOURNET
;
	RET
;
NOTOURNET:
;
;	ENTRY IS OK - INSERT IT
;
;
;	APPLY SUBNET MASK TO ADDRESS
;
	MOV	SI,OFFSET TEMPENTRY

	MOV     AX,WORD PTR NETWORK+2[SI]
	AND	AX,WORD PTR SUBNET+2[SI]
	MOV     WORD PTR NETWORK+2[SI],AX
;
	MOV     AX,WORD PTR NETWORK[SI]
	AND	AX,WORD PTR SUBNET[SI]
	MOV     WORD PTR NETWORK[SI],AX
;
	CMP     NUMBEROFROUTES,MAXROUTES
	JE 	FULLUP			; NO ROOM

	CALL	SCANLIST

	CMP	AL,0
	JE	OK_TO_INSERT		; NOT PRESENT

FULLUP:

	JMP	INVALIDPARAM

OK_TO_INSERT:

	MOV	SI,OFFSET TEMPENTRY
	MOV	CX,TYPE ROUTEENTRY
	REP MOVSB

	INC     NUMBEROFROUTES

LEAVEOUT:

	RET

BITSTOMASK:

	MOV	CH,AL
	MOV	AX,-1
	MOV	DX,AX

	CMP	CH,32
	JAE	MASKRET

	CMP	CH,16
	JAE	MASKLOWER
;
;	16 BITS OR LESS - LOWER BITS ALL ZERO
;
	MOV	AX,0
	MOV	CL,16
	SUB	CL,CH
	SHL	DX,CL

	RET

MASKLOWER:

	MOV	CL,32
	SUB	CL,CH
	SHL	AX,CL

MASKRET:

	RET

SCANLIST:
;
;	SEARCH ROUTE TABLE FOR IP ADDRESS IN TEMPENTRY
;
;	IF FOUND RETURN AL=-1
;
;	IF NOT FOUND, RETURN AL=0, AND DI=PLACE TO INSERT IT
;
	MOV	DI,OFFSET ROUTETAB
	MOV	CX,NUMBEROFROUTES
	JCXZ	ROU_ADD			; LIST EMPTY

ROUFINDLOOP:
;
;	CHECK ORDER - REMEMBER ADDRESSES ARE STORED HI BYTE FIRST
;
	MOV     AL,BYTE PTR NETWORK[DI]
	CMP	AL,TEMPENTRY
	JA	INSERTHERE

	JNE	ROUFINDNEXT		; MISMATCH
;
;	TRY NEXT DIGIT
;
	MOV     AL,BYTE PTR NETWORK+1[DI]
	CMP	AL,TEMPENTRY+1
	JA	INSERTHERE

	JNE	ROUFINDNEXT		; MISMATCH
;
	MOV     AL,BYTE PTR NETWORK+2[DI]
	CMP	AL,TEMPENTRY+2
	JA	INSERTHERE

	JNE	ROUFINDNEXT		; MISMATCH
;
	MOV     AL,BYTE PTR NETWORK+3[DI]
	CMP	AL,TEMPENTRY+3
	JA	INSERTHERE

	JNE	ROUFINDNEXT		; MISMATCH
;
;	ENTRY FOUND
;
	MOV	AL,-1
	RET

INSERTHERE:
;
;	MOVE TABLE DOWN TO MAKE SPACE
;
	CMP	NUMBEROFROUTES,MAXROUTES
	JE	ROU_ADD			; NO ROOM

	PUSH	SI

	MOV	CX,OFFSET ENDOFROUTES
	SUB	CX,DI			; GIVES BYTES TO END
	SUB	CX,TYPE ROUTEENTRY

	PUSH	DI

	MOV	DI,OFFSET ENDOFROUTES-1
	MOV	SI,DI
	SUB	SI,TYPE ROUTEENTRY

	STD
	REP MOVSB
	CLD

	POP	DI
	MOV	CX,TYPE ROUTEENTRY
	MOV	AL,0
	REP STOSB

	SUB	DI,TYPE ROUTEENTRY

	POP	SI

	JMP SHORT ROU_ADD

ROUFINDNEXT:

	ADD     DI,TYPE ROUTEENTRY
;
	LOOP	ROUFINDLOOP
;
;	NOT THERE, SO ADD
;
ROU_ADD:

	MOV	AL,0
	RET




CHECKLIST:
;
;	SEE IF ANY ENTRY IN ROUTE TABLE MATCHES IP ADDRESS IN TEMPENTRY
;	(SUBNET ENTRIES MATCH)
;
;	IF FOUND RETURN AL=-1
;
;	IF NOT FOUND, RETURN AL=0
;
	MOV	DI,OFFSET ROUTETAB
	MOV	CX,NUMBEROFROUTES
	JCXZ	CHECK_X			; LIST EMPTY

ROUCHKLOOP:
;
;	CHECK ORDER - REMEMBER ADDRESSES ARE STORED HI BYTE FIRST
;
	MOV     AX,WORD PTR TEMPENTRY+2
	AND	AX,WORD PTR SUBNET+2[DI]
	CMP     AX,WORD PTR NETWORK+2[DI]
	JNE	ROUCHKNEXT
;
	MOV     AX,WORD PTR TEMPENTRY
	AND	AX,WORD PTR SUBNET[DI]
	CMP     AX,WORD PTR NETWORK[DI]
	JNE	ROUCHKNEXT
;
;	ENTRY FOUND
;
	MOV	AL,-1
	RET

ROUCHKNEXT:

	ADD     DI,TYPE ROUTEENTRY
;
	LOOP	ROUCHKLOOP
;
;	NOT THERE
;
CHECK_X:

	MOV	AL,0
	RET




SENDICMPREPORT:
;
;	HEADER HAS BEEN BUILT (IN ICMPHEADER)
;	BX IS BUFFER HEADER, BUFFPTR POINTS TO IP HEADER
;	BP IS PORT WE ARE REPORTING FROM
;
	MOV	SI,BUFFPTR[BX]

	MOV4	TEMP,IPSOURCE[SI]	; SAVE ORIGINATOR

	CMP	WORD PTR TEMP,0
	JE	NOICMP			; NOT IF ORIGINATOR = 0

	MOV	AX,FRAGWORD[SI]
	AND	AX,1FFFH
	JNZ	NOICMP			; ONLY REPORT ON FRAGMENT ZERO

	MOV	AL,VERLEN[SI]		; HEADER LENGTH (LOWER 4 BITS)
	AND	AX,0FH			; MASK
	ADD	AX,AX
	ADD	AX,AX			; *4
	ADD	BUFFPTR[BX],AX		; ADJUST BUFFER
;
	MOV	DI,BUFFPTR[BX]		; TO DATAGRAM
;
;	ONLY REPORT ON ICMP MESSAGE IF IT IS AN ECHO REQUEST
;
	CMP	IPPROTOCOL[SI],1
	JNE	ICMP_OK

	CMP	BYTE PTR [DI],8
	JNE	NOICMP
;
ICMP_OK:
;
;	DI = USER DATA - SAVE FIRST 8 BYTES
;
	PUSH	SI

	MOV	SI,DI
	MOV	DI,OFFSET FIRST8
	MOV	CX,8
	REP MOVSB

	POP	SI
;
;	OUR HEADER WILL ONLY BE 20 BYTES
;
	SUB	BUFFPTR[BX],AX		; BACK TO IP HEADER
	ADD	BUFFPTR[BX],20		; TO ICMP HEADER

	MOV	DI,BUFFPTR[BX]		; TO ICMP HEADER

	PUSH	AX			; SAVE HEADER LENGTH

	PUSH	SI
	MOV	SI,OFFSET ICMPHEADER
	MOV	CX,8
	REP MOVSB
	POP	SI
;
;	COPY ORIGINAL IP HEADER, PLUS FIRST 8 BYTES OF DATAGRAM
;
	POP	CX			; IP HEADER LENGTH
	ADD	CX,8			; FIRST 8 BYTES OF DATAGRAM

	MOV	BUFFLEN[BX],CX
	ADD	BUFFLEN[BX],8		; ICMP HEADER

	SUB	CX,8
	REP MOVSB

	MOV	SI,OFFSET FIRST8
	MOV	CX,8
	REP MOVSB

	CALL	ICMPSETUP		; CHECKSUM AND ADD IP HEADER

	MOV	DI,BX
	MOV	SI,OFFSET IP_Q

	CALL	Q_ADD

	RET

NOICMP:

	MOV	DI,BX
	MOV	SI,OFFSET FREE_Q

	CLI
	CALL	Q_ADDF
	STI

	RET

ICMPSETUP:
;
;	CALCULATE ICMP CHECHSUM AND ADD IP HEADER
;
	MOV	SI,BUFFPTR[BX]
	PUSH	SI			; ICMP HEADER

	MOV	CX,BUFFLEN[BX]

	INC	CX			; ROUND UP
	SHR	CX,1			; WORD COUNT

	CALL	DO_CHECKSUM
	NOT	DX

	POP	DI
	MOV	ICMPCHECKSUM[DI],DX

;	CALL	ICMP_TRACE_OUT
;
;	ADD IP HEADER AND SEND
;
	ADD	BUFFLEN[BX],20		; IP HDDR LEN
	SUB	BUFFPTR[BX],20		; DITTO

	MOV	CX,BUFFLEN[BX]		; GET LENGTH
	MOV	DI,BUFFPTR[BX]
;
;	DI NOW POINTER TO IP BIT
;
	MOV	VERLEN[DI],45H		; VERSION 4, LEN 5 WORDS
	MOV	TOS[DI],0

	XCHG	CH,CL
	MOV	IPLENGTH[DI],CX

	INC	NEXTIPIDENT

	COPY2	IPID[DI],NEXTIPIDENT

	MOV	FRAGWORD[DI],0

	MOV	AL,ICMPTTL
	MOV	IPTTL[DI],AL

	MOV	IPPROTOCOL[DI],1	; ICMP

	MOV	IPCHECKSUM[DI],0	; WILL CALCULATE LATER

	MOV4	IPSOURCE[DI],MYIPADDR; REPORT FROM CURRENT PORT ADDRESS
	MOV4	IPDEST[DI],TEMP		; SAVED ORIGINATOR
;
;	HEADER COMPLETE - ADD CHECKSUM AND TRACE IF NECESSARY
;
	MOV	SI,BUFFPTR[BX]
	MOV	CX,10			; LENGTH

	CALL	DO_CHECKSUM
	NOT	DX
	MOV	IPCHECKSUM[DI],DX

;	CALL	IP_TRACE_OUT

	RET
;








;
;       PROCESS ARP COMMAND
;
;       THERE ARE 4 PARAMETERS - IP ADDR CALLSIGN PORT TYPE
;
;
PROCARPCODE:
;
	MOV     SI,PARAMPOINTER
	MOV     DI,ARPINITPTR
;
;       GET HOST
;
	CALL    GETIPADDR
;
	PUSH    DI
	CALL    CONVTOAX25
	POP     DI

	PUSH    SI
	MOV     SI,OFFSET AX25CALL

	MOV     CX,7
	REP MOVSB

	POP     SI

	MOV     AL,1
	STOSB                           ; VALID

	MOV     AX,-1
	STOSW                           ; TIMER (LOCKED

	CMP     BYTE PTR [SI],20H
	JE      ARPBAD
		
	CALL    GETVALUE
	MOV     AX,NEWVALUE
	STOSB                           ; PORT

	INC     SI
	LODSB

	CMP     AL,'E'                  ; ETHERNET
	JE      ARPTYPEOK

	CMP     AL,'V'                  ; VC
	JE      ARPTYPEOK
	CMP     AL,'D'                  ; DG
	JE      ARPTYPEOK
	CMP     AL,'N'                  ; NETROM
	JNE     ARPBAD 
	
	CMP     NEWVALUE,0              ; NETROM MUST USE PORT 0
	JE      ARPTYPEOK

ARPBAD:

	MOV     VALERROR,1

ARPTYPEOK:

	STOSB

	CMP     VALERROR,0
	JNE     ARPLEAVEOUT

	ADD     ARPINITPTR, TYPE ARPDATA

	RET
	
ARPLEAVEOUT:
;
;       GET RID OF IT AGAIN
;
	MOV     DI,ARPINITPTR
	MOV     CX,TYPE ARPDATA
	MOV     AL,0
	REP STOSB
	
	RET

PROCRIPCODE:
;
	MOV	VALERROR,0

	MOV     SI,PARAMPOINTER
	MOV     DI,OFFSET TEMP
;
	CALL    GETIPADDR
;
;	SEE IF IN LIST ALREADY
;
	CMP     VALERROR,0
	JNE     RIPLEAVEOUT

	MOV	CX,NUMBEROFNEIGHBOURS
	MOV	DI,OFFSET NEIGHBOURS

	JCXZ	PUTIN			; WAS EMPTY SO MUST BE OK

	CMP	CX,10
	JE	RIPLEAVEOUT		; FULL
;
RIPADDLOOP:

	CMP4	TEMP,0[DI]
	JE	RIPLEAVEOUT		; ALREADY IN 

	ADD	DI,TYPE NEIGHBOUR
	LOOP	RIPADDLOOP
;
;	NOT FOUND, SO OK TO ADD
;
PUTIN:

	MOV	NBOURFLAG[DI],'b'	; NEW TYPE	

	CMP	BYTE PTR -1[SI],'/'
	JNE	NOTOLDPARAM

	CMP	BYTE PTR [SI],'O'
	JNE	NOTOLDPARAM

	MOV	NBOURFLAG[DI],'B'

NOTOLDPARAM:

	MOV4	0[DI],TEMP
	INC	NUMBEROFNEIGHBOURS

	RET

RIPLEAVEOUT:
	
	MOV	VALERROR,1
	RET


CONVTOAX25:
;
;       CONVERT CALLSIGN IN [SI] TO AX25 FORMAT IN AX25CALL
;       RETURN NZ IF CALLSIGN INVALID
;
	MOV     DI,OFFSET AX25CALL
	MOV     CX,6                    ; MAX ALPHANUMERICS
	MOV     AL,40H
	REP STOSB                       ; CLEAR IN CASE SHORT CALL
	MOV     AL,01100000B            ; DEFAULT SSID
	STOSB
	MOV     DI,OFFSET AX25CALL
	MOV     CL,7
CONVAX00:
	LODSB
	CMP     AL,'-'
	JE      CONVAX20                ; SSID FOLLOWS
	CMP     AL,' '
	JE      CONVAX40                ; END OF NAME
;
	CMP     AL,'#'
	JNE     NOTHASH

	CMP     CL,7
	JE      CONVAX10                ; OK AS FIRST CHAR
NOTHASH:
	CMP     AL,'/'                  ; ALLOW / AND DIGITS (FOR W0RLI!)
	JB      CONVAX49                ; INVALID
	CMP     AL,'9'
	JBE     CONVAX10                ; OK

	CMP     AL,'A'
	JB      CONVAX49                ; INVALID
	CMP     AL,'Z'
	JBE     CONVAX10                ; OK


	CMP     AL,'a'
	JB      CONVAX10                ; OK
	CMP     AL,'z'
	JA      CONVAX49                ; INVALID

	AND     AL,NOT 20H              ; CONVERT TO UPPER CASE
CONVAX10:
	SAL     AL,1                    ; SHIFT UP
	STOSB
	LOOP    CONVAX00
;
;       HAVE PROCESSED 7 CHARS - NAME IS INVALID
;
	JMP SHORT CONVAX49

CONVAX20:
	LODSB                           ; FIRST CHAR IF SSID
	CMP     AL,'0'
	JB      CONVAX49                ; INVALID
	CMP     AL,'9'
	JA      CONVAX49                ; OK
;
	MOV     AH,AL                   ; SAVE
	LODSB
	CMP     AL,' '
	JE      CONVAX30                ; END OF SSID

	INC     SI                      ; OTHER CODE EXPECTS SI TO POINT TO NEXT FIELD
	CMP     AL,'0'
	JB      CONVAX49                ; INVALID
	CMP     AL,'5'                  ; LIMIT 10 - 15
	JA      CONVAX49                ; OK
;
	SUB     AL,30H
	CMP     AH,'1'
	JNE     CONVAX49                ; IF 2 DIGITS, FIRST MUST BE 1

	ADD     AL,10
	MOV     AH,AL
CONVAX30:
	SAL     AH,1                    ; UP 1 BIT
	OR      AH,01100000B            ; DEFAULT RR BITS
	MOV     AX25CALL+6,AH

CONVAX40:
;
;       NO SSID SUPPLIED
;
	XOR     AL,AL                   ; SET ZERO
	RET

CONVAX49:
	OR      AL,1                    ; SET NZ
	RET




;       GET RECORD FROM INPUT STREAM
;
GETRECORD:

	MOV     DI,OFFSET KBBUFF
	MOV     CX,40
	MOV     AX,2020H
	REP STOSW

	MOV     DI,OFFSET KBBUFF
	MOV     CX,0
	
GETRECORDLOOP:

	CMP     DI,OFFSET KBBUFF+80
	JA      GETRET

	PUSH    CX      
	CALL    GETDISK
	POP     CX

	CMP     AL,0DH
	JE      GETRET

	CMP     AL,1AH
	JNE     GET000

GETRET:
	RET

GET000:

	CMP     AL,0AH
	JE      GETRECORDLOOP
;
	CMP     AL,09H                  ; TAB
	JNE     TABEND
;
CONVERTTAB:                             ; REPLACE TAB WITH SPACES

	MOV     AL,CL
	AND     AL,07H                  ; MASK TO MULTIPLE OF 8
	MOV     AH,8
	SUB     AH,AL
	MOV     AL,20H
	DEC     AH
	JZ      TABEND                  ; ONLY ONE

TABLOOP:
	STOSB
	INC     CX
	DEC     AH
	JNZ     TABLOOP
;
TABEND:
	CMP     AL,'a'        
	JB      CASEOK
	CMP     AL,'z'        
	JA      CASEOK
	SUB     AL,20H                  ; CONVERT TO UPPER CASE

CASEOK:        
	STOSB
	INC     CX
	JMP     GETRECORDLOOP
;
;
GETDISK:                                ; GET A BYTE FROM DISK BUFFER

	CMP     READCOUNT,128
	JE      READIT                  ; GET NEW BLOCK
;
	MOV     SI,READPOINTER          ; GET NEXT CHAR POINTER
READNEXT:
	LODSB
	MOV     READPOINTER,SI

	INC     READCOUNT               ; CHARS USED (AND SETS NONZERO FLAG)
;
	RET                             ; WITH ZERO FLAG NOT SET
;
READIT:
;
	MOV     DX,OFFSET READBLOCK
	MOV     AH,3FH                  ; READ
	MOV     BX,HANDLE
	MOV     CX,128
	INT     21H
;
	CMP     AX,0                    ; PART RECORD READ
	JNE     READOK
;
ENDOFINPUT:
;
;       END OF FILE
;
	MOV     READBLOCK,1AH
;
READOK:

	SUB     READCOUNT,AX
	MOV     SI,OFFSET READBLOCK
	JMP     READNEXT
;
HEXOUT:

	PUSH    AX
	PUSH    AX
	sar     al,1
	sar     al,1
	sar     al,1
	sar     al,1
	call    hexout1
	pop     ax
	call    hexout1
	POP     AX
	ret

hexout1:
	and     al,0fh
	cmp     al,10
	jl      hexout5
	add     al,7
hexout5:
	add     al,30h
	STOSB
;
	ret
;
HEXOUTD:

	PUSH    AX
	PUSH    AX
	sar     al,1
	sar     al,1
	sar     al,1
	sar     al,1
	call    hexout1D
	pop     ax
	call    hexout1D
	POP     AX
	ret

hexout1D:
	and     al,0fh
	cmp     al,10
	jl      @F
	add     al,7
@@:
	add     al,30h
	MOV	AH,14
	INT	10H
;
	ret
;
;       CHECKSUM CALCULATION ROUTINE
;
DO_CHECKSUM:
;
;       CHECKSUM CX WORDS, STARTING AT SI
;
	XOR     DX,DX           ; CLEAR CHECKSUM AND CARRY
CSUMLOOP:
	LODSW
	ADC     DX,AX

	LOOP    CSUMLOOP

	ADC     DX,0            ; MAY BE CARRY FLOATING AROUND
	RET

;
;
;       QUEUE HANDLING ROUTINES 
;
;           EACH QUEUE CONSISTS OF A TWO BYTE HEADER AND A NUMBER OF ENTRIES,
;           THE FIRST TWO BYTES OF EACH IS A FORWARD CHAIN POINTER
;               THE POINTER IS THE ADDRESS OF THE NEXT ENTRY, STORED WITH THE
;               HIGH ORDER BYTE FIRST [IE NOT THE NORMAL Z80 WAY ROUND]
;               A ZERO FIRST BYTE INDECATES END OF CHAIN
;
;           THE FOLLOWING ROUTINES ARE PROVIDED:-
;
;               ADD ENTRY TO QUEUE - ON RETURN A CONTAINS NUMBER OF ENTRIES
;                                    WHICH WERE ON THE QUEUE PRIOR TO THE CALL
;
;               GET FIRST ENTRY    - Z FLAG SET IF QUEUE EMPTY
;
;               REMOVE FIRST ENTRY - Z FLAG SET IF QUEUE EMPTY
;
;
;           ALL ROUTINES REQUIRE SI SET TO QUEUE HEADER ON ENTRY
;           DI CONTAINS THE ADDRESS OF THE BLOCK FOUND OR TO BE ADDED
;
Q_ADDF:

	CMP     DI,0
	JNE     XXX

	NOP                             ; FOR CODEVIEW

XXX:
	INC     BUFFERCOUNT             ; ONLY USED FOR FREE Q

	MOV     AX,[SI]                 ; OLD QUEUE HEADER
	MOV     [DI],AX                 ; CHAIN TO NEW BUFFER
;
	MOV     [SI],DI                 ; CHAIN NEW BUFFER TO QUEUE HEADER
	RET
;
Q_ADD:
	MOV     WORD PTR [DI],0         ; CLEAR CHAIN POINTER IN NEW BUFFER
Q_ADD1:
	MOV     AX,[SI]
	OR      AX,AX
	JZ      Q_ADD5                  ; END
;
	MOV     SI,AX
	JMP     Q_ADD1                  ; LOOP TILL END FOUND
;
Q_ADD5:
	MOV      [SI],DI                ; CHAIN NEW BLOCK
	RET
;
;
;
Q_FIRST:
	MOV     DI,[SI]
	CMP     DI,0
Q_RET:
	RET
;
;
;
Q_REM:
	CALL    Q_FIRST         ; SET FIRST ENTRY
	JZ      Q_RET           ; QUEUE EMPTY
;
	MOV     AX,[DI]
	MOV     [SI],AX         ; SI = HEADER
;
	RET
;
DUMPCODE:

	MOV     DX,OFFSET DMPNAME
	MOV     CX,0
	MOV     AH,3CH
	INT     21H

	MOV     DHANDLE,AX
;
	MOV     DX,0
	MOV     CX,65535

	MOV     AH,40H                  ; WRITE
	MOV     BX,DHANDLE

	INT     21H

	MOV     BX,DHANDLE
	MOV     AH,3EH
	INT     21H                     ; CLOSE LOG FILE
;
        RET

DUMPTABLES:

	MOV     FILEIT,1                ; FILE INSTEAD OF DISPLAY        
	
	MOV     DX,OFFSET TABNAME
	MOV     CX,0
	MOV     AH,3CH
	INT     21H

	MOV     DHANDLE,AX
;
;       OUR IP ADDR        
;
	MOV     DI,OFFSET FILEBUFFER
	
	MOV     SI,OFFSET KEYIPADDRESS
	MOV     CX,10
	REP     MOVSB
	
	MOV     SI,OFFSET MYIPADDR        
	CALL    DISPLAYIPADDR

	CALL    WRITELINE
;
	MOV     DI,OFFSET FILEBUFFER
	
	MOV     SI,OFFSET KEYETHIPADDR
	MOV     CX,10
	REP     MOVSB
	
	MOV     SI,OFFSET MYETHIPADDR
	CALL    DISPLAYIPADDR

	CALL    WRITELINE
;
	MOV     SI,OFFSET KEYLEARN
	MOV	DI,OFFSET FILEBUFFER
	MOV     CX,6
	REP     MOVSB
	
	MOV	AL,LEARNFLAG
	STOSB

	CALL    WRITELINE
;
	MOV     SI,OFFSET KEYTTL
	MOV	DI,OFFSET FILEBUFFER
	MOV     CX,4
	REP     MOVSB
	
	MOV     AL,TTL
	MOV     AH,0
	CALL    CONVTODIGITS
	CALL    PRINTDEC
	
	CALL    WRITELINE
;
;	DEFAULT ROUTER
;
	MOV     DI,OFFSET FILEBUFFER
	
	MOV     SI,OFFSET DEFAULTGATEWAY
	CMP     RTYPE[SI],0
        JE      DUMPNODEF

	MOV     SI,OFFSET KEYROUTE
	MOV     CX,6
	REP     MOVSB
	
	MOV     SI,OFFSET KEYDEFAULT
	MOV     CX,8
	REP     MOVSB
	
	MOV     SI,OFFSET DEFAULTGATEWAY

	PUSH	SI

	ADD	SI,8
	CALL    DISPLAYIPADDR
	
	POP     SI
	
	CALL    SPACE        
	
	MOV     AL,PORT[SI]
	MOV     AH,0
	CALL    CONVTODIGITS
	CALL    PRINTDEC
	
	CALL    SPACE
	
	MOV     AL,RTYPE[SI]
	CALL    PRINTIT
	
	CALL    WRITELINE        
	
DUMPNODEF:


	MOV     SI,OFFSET ROUTETAB
	MOV     CX,NUMBEROFROUTES

	MOV	CH,0

	JCXZ	NOROUTESX		; IN CASE CORRUPT - MAX 256

DUMPTABLOOP:        

	PUSH	CX

	MOV     DI,OFFSET FILEBUFFER
	
	PUSH    SI        
	
	MOV     SI,OFFSET KEYROUTE
	MOV     CX,6
	REP     MOVSB
	
	POP     SI
	
	PUSH    SI
	
	CALL    DISPLAYIPADDR

	MOV     AL,'/'
	CALL    PRINTIT

	MOV     AX,0
	MOV     BX,WORD PTR 2[SI]
	CALL    COUNTBITS
	MOV     BX,WORD PTR [SI]
	CALL    COUNTBITS

	CALL    CONVTODIGITS
	CALL    PRINTDEC

	CALL    SPACE
	
	ADD     SI,4                    ; SKIP SUBNET        
	CALL    DISPLAYIPADDR
	
	POP     SI
	
	CALL    SPACE        
	
	MOV     AL,PORT[SI]
	MOV     AH,0
	CALL    CONVTODIGITS
	CALL    PRINTDEC
	
	CALL    SPACE
	
	MOV     AL,RTYPE[SI]
	CALL    PRINTIT
	
	CALL    SPACE        
	
	MOV     AL,RIPTIMOUT[SI]
	MOV     AH,0
	CALL    CONVTODIGITS
	CALL    PRINTDEC
	
	CALL    SPACE        
	
	MOV     AL,METRIC[SI]
	MOV     AH,0
	CALL    CONVTODIGITS
	CALL    PRINTDEC
	
	CALL	SPACE

	MOV     AL,ROUTEINFO[SI]
	MOV     AH,0
	CALL    CONVTODIGITS
	CALL    PRINTDEC
	
	CALL    WRITELINE        
	
	POP     CX
	
	ADD     SI,TYPE ROUTEENTRY        
	
	LOOP    DUMPTABLOOP

NOROUTESX:
;
;       NOW DO ARP TABLE
;
	MOV     CX,MAXARP
	MOV     SI,OFFSET ARPTABLE

DUMPARPLOOP:
	
	CMP     BYTE PTR [SI],0
	JE      DUMPARPNEXT
       
	TEST    ARPTIMER[SI],8000H
	JZ	DUMPARPNEXT			; ONLY LOCKED ENTRIES

	MOV     DI,OFFSET FILEBUFFER
	
	PUSH    SI        
	PUSH    CX
	
	MOV     SI,OFFSET KEYARP
	MOV     CX,4
	REP     MOVSB
	
	POP     CX        
	POP     SI
	
	PUSH    SI
	PUSH    CX

	PUSH    SI
	
	CALL    DISPLAYIPADDR

	MOV     AL,20H
	STOSB
	
	PUSH    DI        
	
	call    CONVFROMAX25
	mov     SI,OFFSET NORMCALL
	POP     DI
	REP MOVSB        

	POP     SI

	MOV     AL,20H
	STOSB

	MOV     AL,ARPINTERFACE[SI]
	MOV     AH,0        
	CALL    CONVTODIGITS
	CALL    PRINTDEC

	MOV     AL,20H
	STOSB
	
	MOV     AL,ARPTYPE[SI]
	CALL    PRINTIT

	CALL    WRITELINE
		
	POP     CX
	POP     SI

DUMPARPNEXT:        

	ADD     SI,TYPE ARPDATA

	LOOP    DUMPARPLOOP
;
;	AND RIP TABLE
;
	MOV	CX,NUMBEROFNEIGHBOURS
	JCXZ	NODUMPRIP

	MOV	SI,OFFSET NEIGHBOURS

DUMPRIPLOOP:        

	MOV     DI,OFFSET FILEBUFFER
	
	PUSH    SI        
	PUSH    CX
	
	MOV     SI,OFFSET KEYRIP
	MOV     CX,4
	REP     MOVSB
	
	POP     CX        
	POP     SI
	
	PUSH    CX        
	PUSH    SI
	
	CALL    DISPLAYIPADDR
	POP	SI
	
	MOV	AL,NBOURFLAG[SI]
	CMP	AL,'B'			; OLD RIP
	JNE	DUMPNEW

	MOV	AL,'/'
	CALL	PRINTIT
	MOV	AL,'O'
	CALL	PRINTIT

DUMPNEW:

	CALL    WRITELINE        
	
	POP     CX
	
	ADD     SI,TYPE NEIGHBOUR        
	
	LOOP    DUMPRIPLOOP
;

NODUMPRIP:

	MOV     BX,DHANDLE
	MOV     AH,3EH
	INT     21H                     ; CLOSE FILE
;
	MOV     FILEIT,0        
	RET

WRITELINE:        

	MOV     AL,0DH        
	CALL    PRINTIT
	
	MOV     AL,0AH        
	CALL    PRINTIT
	MOV     DX,OFFSET FILEBUFFER        
	MOV     CX,DI
	SUB     CX,DX
		
	MOV     AH,40H                  ; WRITE
	MOV     BX,DHANDLE

	INT     21H
	
	RET
	
CODE    ENDS
;               
	END

	char type;

#define RSPF_FULLPKT    1               /* Routing update */
#define RSPF_RRH        3               /* Router-Router Hello */


G0CYB-5>G0BSX-5 Port=1 <UI C>: IP 44.131.4.67>44.131.2.27 Len: 101 Prot: 73
15  01   01    01      8C8D 04   03    0006
VER TYPE FRAGN FRAGTOT SUM  SYNC NODES ENVID

 2C830443 071B 0001 2000 0802 20 2C830437 20 2C83021B
 2C830237 0720 0001 1F00 0803 20 2C830251 20 2C83022D 20 2C830443 
 2C83021B 00A3 0001 1F00 0802 00 00000000 20 2C830443


G0CYB-5>QST Port=1 <UI C>: IP 44.131.4.67>44.131.0.0 Len: 67 Prot: 73

15  03    AE7D  2C 83 04 43  010A  00    52 53 50 46 20 72 6F 75 74 69 6E 67 
VER TYPE  SUM   44.131.4.67  SEQ   FLAGS R  S  P  F     r  o  u  t  i  n  g  
 
20 69 6E 20 75 73 65 2E 20 20 52 49 50 20 64 69 73 61 62 6C 65 64 0D0A
   i  n     u  s  e  .        R  I  P     D  I  S  A  B  L  E  D


G0CYB-5>QST Port=1 <UI C>: IP 44.131.4.67>44.131.0.0 Len: 101 Prot: 73

1501 0101 8EA5 04   03    0008
	  SUM  SYNC NODES ENVID

 2C830443 071C 0001 20 0008 02 20 2C830437 20 2C83021B 
 2C830237 0720 0001 1F 0008 03 20 2C830251 20 2C83022D 20 2C830443 
 2C83021B 00A3 0001 1F 0008 02 00 00000000 20 2C830443 


G0CYB-5>QST Port=1 <UI C>: IP 44.131.4.67>44.131.0.0 Len: 47 Prot: 73

1501 0101 6939 04   01   0005

 2C830443 071A0101200008 01 20 2C83021B



#ifndef RSPFPKTLEN
struct rspfpacketh {
    char version;
    char type;
    unsigned char fragn;
    unsigned char fragtot;
    int16 csum;
    unsigned char sync;
    unsigned char nodes;
    int16 envid;
};
#define RSPFPKTLEN      10
struct rspfnodeh {
    int32 addr;
    short seq;                  /* 16 bit signed int */
    unsigned char subseq;
    unsigned char links;
};
#define RSPFNODELEN     8
struct rspflinkh {
    unsigned char horizon;
    unsigned char erp;
    unsigned char cost;
    unsigned char adjn;
};
#define RSPFLINKLEN     4
struct rspfrouter {
    struct rspfrouter *next;
    char sent;                  /* True if the data has already been sent */
    char subseq;                /* Sub-sequence number of latest update */
    int32 time;                 /* Time when data was last received */
    struct mbuf *data;          /* Latest data, starting with node header */
};
#define NULLRROUTER     (struct rspfrouter *)0
struct rspfreasm {
    struct rspfreasm *next;
    int32 addr;                 /* Address of originating station */
    int32 time;                 /* Time when a fragment was last received */
    struct mbuf *data;          /* A queue of fragments */
};
#define NULLRREASM      (struct rspfreasm *)0

#define RSPF_RTIME      30              /* Reassembly timeout in seconds */

/* RRH header, host format */
struct rrh {
    char version;
    char type;
    int16 csum;
    int32 addr;
    int16 seq;
    char flags;
#define RSPFMODE        1       /* Tells whether we want a VC link or not */
};
#define RRHLEN          11
union rspf {
    struct {
    } hdr;
    struct rspfpacketh pkthdr;
    struct rrh rrh;
};

#define RSPF_PROCMAX    5               /* Maximum number of processes handling
					   suspect adjacencies */

struct rspfadj {
    struct rspfadj *next;
    int32 addr;
    unsigned char cost;         /* Cost to reach this adjacency */
    int16 seq;                  /* Number of AX.25 frames it has sent */
    int32 heard;                /* Number of heard AX.25 frames */
    struct timer timer;         /* Adjacency turns suspect if timer expires */
    char tos;                   /* Preferred type of service */
    char added;                 /* Used by the SPF algorithm */
    void *scratch;              /* also used by SPF (contains the interface) */
    struct proc *pinger;        /* Pointer to rspfping process */
    char state;
#define RSPF_TENTATIVE  0
#define RSPF_OK         1
#define RSPF_SUSPECT    2
#define RSPF_BAD        3
    int okcnt;                  /* Times adjacency has entered OK state */
};
#define NULLADJ (struct rspfadj *)0

struct rspfiface {
    struct rspfiface *next;
    struct iface *iface;
    unsigned char quality;      /* Default quality for this interface */
    unsigned char horizon;      /* Default horizon value */
    unsigned char erp;          /* Default ERP factor */
};
#define NULLRIFACE (struct rspfiface *)0

struct rspf_stat {
     unsigned rrhin;            /* RRH's received */
     unsigned rrhout;           /* RRH's sent */
     unsigned updatein;         /* Updates received */
     unsigned updateout;        /* Updates sent */
     unsigned badcsum;          /* Bad checksums */
     unsigned badvers;          /* Bad versions */
     unsigned norspfiface;      /* RSPF packets from non RSPF interfaces */
     unsigned oldreport;        /* Node headers with old sequence numbers */
     unsigned outpolls;         /* Poll packets sent */
     unsigned noadjupdate;      /* Updates received from non-adjacencies */
};

/* Event types in main loop */
#define RSPFE_RRH       1       /* It is time to send a new RRH */
#define RSPFE_CHECK     2       /* An adjacency has becomed suspect */
#define RSPFE_UPDATE    3       /* Time to send a new routing updates */
#define RSPFE_ARP       4       /* An ARP reply was received */
#define RSPFE_PACKET    5       /* A packet was received (RRH or Update) */
     
extern struct rspf_stat Rspf_stat;
extern struct rspfreasm *Rspfreasmq;
extern struct rspfiface *Rspfifaces;
extern struct rspfadj *Adjs;
extern struct rspfrouter *Rspfrouters;
extern struct mbuf *Rspfinq;
extern struct timer Rspfreasmt, Susptimer;
extern char *Rrh_message;
extern int Rspfownmode;
extern unsigned short Rspfpingmax;

void rspfmain __ARGS((int v,void *v1,void *v2));
void rspf_input __ARGS((struct iface *iface,struct ip *ip,struct mbuf *bp,int rxbroadcast));
void rspfarpupcall __ARGS((int32 addr,int16 hardware,struct iface *iface));
void rspfrouteupcall __ARGS((int32 addr,unsigned bits,int32 gateway));
void rspfevent __ARGS((void *t));
void rspfsuspect __ARGS((void *t));
struct mbuf *makeownupdate __ARGS((int32 dest,int new));
int ntohrspf __ARGS((union rspf *rspf,struct mbuf **bpp));
int ntohrspfnode __ARGS((struct rspfnodeh *nodeh,struct mbuf **bpp));
int ntohrspflink __ARGS((struct rspflinkh *linkh,struct mbuf **bpp));
struct mbuf *htonrrh __ARGS((struct rrh *rrh,struct mbuf *data,struct pseudo_header *ph));
struct mbuf *htonrspf __ARGS((struct rspfpacketh *pkth,struct mbuf *data));
struct mbuf *htonrspfnode __ARGS((struct rspfnodeh *nodeh,struct mbuf *data));
struct mbuf *htonrspflink __ARGS((struct rspflinkh *linkh,struct mbuf *data));
void rspfnodedump __ARGS((FILE *fp,struct mbuf **bpp,int adjcnt));
#endif
	IF      0

	CMP     BYTE PTR [SI],2
	JE      RIPRESP

	RET                             ; DONT HANDLE REQUESTS YET

RIPRESP:

	ADD     SI,8                    ; TO IP ADDR

	SUB     CX,12                   ; UDP AND RIP HEADERS
	
RIPOUTERLOOP:

	CMP     CX,20
	JAE     MORE_RIP

	JMP     RIPFINISHED

MORE_RIP:

	PUSH    CX
;
;       SEE IF ALREADY IN TABLE
;
	MOV     DI,OFFSET ROUTETAB
	MOV     CX,NUMBEROFROUTES

RIPFINDLOOP:

	MOV     AX,WORD PTR NETWORK+2[DI]
	CMP     AX,WORD PTR 2[SI]
	JNE     RIPFINDNEXT
;
	MOV     AX,WORD PTR NETWORK[DI]
	CMP     AX,WORD PTR [SI]
	JNE     RIPFINDNEXT
;
;       IF GATEWAY IS ADVERTISING ITS OWN NETWORK, IGNORE IT - WE HAVE MATCHED
;       OUR ENTRY FOR OUR NETWORK
;
	CMP     AX,WORD PTR UDPSOURCE
	JE      NEXTRIPJMP

;       ENTRY FOUND - IS THIS THE ONE WE ARE ALREAY USING?
;
	CMP4    GATEWAY[DI],UDPSOURCE   ; WHERE THIS MSG CAME FROM
	JE      REFRESH_RIP             ; WE ARE ALREADY USING THIS ROUTE
;
;       SEE IF THIS ONE IS BETTER
;
	MOV     AL,METRIC[DI]
	DEC     AL
	CMP     AL,15[SI]               ; METRIC IN RIP MSG
	JA      UPDATE_ENTRY            ; THIS ONE IS BETTER
;
	JNE     NEXTRIPENTRY            ; WORSE
;
;       SAME AS CURRENT. IF IT IS GETTING TOWARDS TIMING OUT, USE THIS ONE
;
	CMP     RIPTIMOUT[DI],3
	JBE     UPDATE_ENTRY            ; START USING THIS ONE

NEXTRIPJMP:

	JMP SHORT NEXTRIPENTRY

RIPFINDNEXT:

	ADD     DI,TYPE ROUTEENTRY
;
	LOOP    RIPFINDLOOP
;
;       NOT FOUND, SO ADD
;
	CMP     BYTE PTR 15[SI],15
	JAE     NEXTRIPENTRY            ; DONT ADD UNREACHABLE ROUTES

	CMP     NUMBEROFROUTES,MAXROUTES
	JE      NEXTRIPENTRY            ; NO ROOM

	INC     NUMBEROFROUTES

	MOV4    NETWORK[DI],0[SI]

	MOV     WORD PTR SUBNET[DI],0FFFFH; UNTIL WE USE SUBNETS

UPDATE_ENTRY:

	MOV4    GATEWAY[DI],UDPSOURCE   ; WHERE THIS MSG CAME FROM
	MOV     PORT[DI],BP
	MOV     ROUTECHANGED[DI],1

REFRESH_RIP:

	MOV     AL,15[SI]               ; INCOMING METRIC
	INC     AL
	CMP     AL,16
	JBE     MET_OK
;
;       HE IS TELLING US ITS UNREACHABLE - START DELETE TIMER
;
	MOV     METRIC[DI],16
	MOV     RIPTIMOUT[DI],0

	CMP     GARTIMOUT[DI],0
	JNE     NEXTRIPENTRY            ; TIMER ALREADY RUNNING

	MOV     GARTIMOUT[DI],4         ; SET WAITING TO DELETE

	JMP SHORT NEXTRIPENTRY

MET_OK:

	MOV     METRIC[DI],AL

	MOV     RIPTIMOUT[DI],MAXRIP
	MOV     GARTIMOUT[DI],0         ; IN CASE WAITING TO DELETE


NEXTRIPENTRY:

	POP     CX

	ADD     SI,20                   ; TO NEXT
	SUB     CX,20

	JMP     RIPOUTERLOOP

RIPFINISHED:

	ENDIF

;	AH = 11h  Poll for IP data. Returns CX = 0 if none available,
;		  otherwise CX= length, DS:SI=Data
;
; 	AH = 12h  Send IP frame. DI = ADDR
;				 CX = LENGTH
;			         SI = ETH ADDR
;
;	AH = 13h  Status - Return AX=0 If OK to send an IP frame.
;			          CX=0 If no IP frame available to receive
;
;	AH = 21h  Poll for ARP data. Returns CX = 0 if none available,
;		  otherwise CX= length, DS:SI=Data
;
; 	AH = 22h  Send ARP frame. DS:DI = Buffer header.
;
;	AH = 23h  Status - Return AX=0 If OK to send an ARP frame.
;			          CX=0 If no ARP frame available to receive
;
