	PAGE	62,132

	INCLUDE	ENVIRON.ASM
;
	EXTRN	BPQHOSTPORT:FAR

CODE   SEGMENT PUBLIC 'CODE'

	EXTRN	STOPNODE:NEAR,SYSTEM:BYTE,REFRESH:BYTE,BWFLAG:BYTE
	EXTRN	CLOSEIP:NEAR,BUFFLEN:ABS
	
	PUBLIC	TERMINAL,TERMINIT,TERMTRACE,LOGNAME,CLOSETERM

	ASSUME  CS:CODE,DS:TERMDATA,ES:TERMDATA
;
;	A SIMPLE PACKET RADIO TERMINAL PROGRAM
;
;	By John Wiseman, G8BPQ
;

CR	EQU	0DH
LF	EQU	0AH


	EVEN


TERMDATA	SEGMENT	PUBLIC	'TERMDATA'

DATABYTE	LABEL	BYTE

HEADER		DB	NAMESTRING,' PACKET SYSTEM              '
CALLSIGN	DB	25 DUP (20H)
		DB	HEADER+60-$ DUP (20H)
LOGFLAG		DB	0,' '			; DEFAULT = LOGGING OFF
DATE		DB	'DD/MM/YY  '
TIME		DB	'HH:MM:SS'

FILLER1		DB	80 DUP (205)

TRAILER		DB	80 DUP (205)

KBBUFF		DB	82 DUP (0)
		DB	'ESC=DISC F1=MON-O'
MONOFFON	DB	'FF F2=MCOM-O'
MCOMOFFON	DB	'N  F3=MMASK-'
MMASKVAL	DB	'FFFF'	
		DB	' Buffs:'
BUFFS		DB	'999 '
LASTRTM		DB	'LAST 00:00 '
THISRTM		DB	'THIS 00:00'

SCREENBUFFERLEN	EQU	15*80*12

	EVEN


SCREENBUFFER	DB	SCREENBUFFERLEN-80 DUP (20h)	; SCROLL BUFFER
BOTTOMLINE	DB	80 DUP (0)	; OUTPUT LINE

COLOUR		DW	14		; ATTRIBUTE
NORMATTR	DW	14		; FOR NORMAL WRITES

KBPTR		DW	OFFSET KBBUFF

DISCONNMSG	DB	CR,'*** DISCONNECTED - Bytes Sent/RXed = '
LDISCON		EQU	$-DISCONNMSG

SENDCOUNT	DW	0,0
RXCOUNT		DW	0,0

SCROLLPOINTER	DW	OFFSET BOTTOMLINE-(19*80)

CHAR		DW	0
PORT		DB	1

LASTSEC		DB	0

LOGNAME		DB	'SWITCH.LOG',0
		DB	'  '			; LONGER NAME FOR PAC4
HANDLE		DW	0

MMCHAR		DB	'$'
MMCHAR1		DB	0
MMCHAR2		DB	0
MMCHAR3		DB	0
MMCHAR4		DB	0
		DB	0,0DH

STOPMSG		DB	' Program Terminated',0dh,0ah
lstopmsg	EQU	$-stopmsg

BADPORTMSG	DB	'Port specified is invalid',0dh,0ah,'$'

RTMFLAG		DB	0		; RTM RUNNING
RTMSTART	DW	0		; START TIME

D10		DW	10
D182		DW	182
D60		DW	60

D1093		DW	1093

TIMESTAMP	DW	0

NODELINE	DB	350 DUP (0)

NEWVALUE	DW	0
WORD10		DW	10
WORD16		DW	16
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


NULL		EQU	00H
CR		EQU	0DH
LF		EQU	0AH
NETROM_PID	EQU	0CFH
NODES_SIG	EQU	0FFH
;
PORT_MSG	DB	' Port=',NULL
NODES_MSG	DB	' NODES broadcast from ',NULL
NETROM_MSG	DB	' NET/ROM',CR,'  ',NULL
TO_MSG		DB	' to ',NULL
TTL_MSG		DB	' ttl=',NULL
AT_MSG		DB	' at ',NULL
VIA_MSG		DB	' via ',NULL
QUALITY_MSG	DB	' qlty=',NULL

MYCCT_MSG	DB	' my'
CCT_MSG		DB	' cct=',0
TIM_MSG		DB	' t/o=',0

WINDOW		DB	' w=',0
CONN_REQ_MSG	DB	' <CON REQ>',NULL
CONN_ACK_MSG	DB	' <CON ACK>',NULL
CONN_NAK_MSG	DB	' <CON NAK> - BUSY',NULL
DISC_REQ_MSG	DB	' <DISC REQ>',NULL
DISC_ACK_MSG	DB	' <DISC ACK>',NULL
INFO_MSG	DB	' <INFO S',NULL
INFO_ACK_MSG	DB	' <INFO ACK R',NULL
DUFF_NET_MSG	DB	' <????>',NULL
IP_MSG		DB	' <IP> ',NULL
 
UA_MSG		DB	'UA',NULL
DM_MSG		DB	'DM',NULL
RR_MSG		DB	'RR',NULL
RNR_MSG		DB	'RNR',NULL
UI_MSG		DB	'UI',NULL
FRMR_MSG	DB	'FRMR',NULL
REJ_MSG		DB	'REJ',NULL
;
;	IP AND TCP BITS
;
IPMSG	STRUC
;
;	FORMAT OF IP HEADER
;
;	NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)
;

VERLEN		DB	0		; 4 BITS VERSION, 4 BITS LENGTH
TOS		DB	0		; TYPE OF SERVICE
IPLENGTH	DW	0		; DATAGRAM LENGTH
IPID		DW	0		; IDENTIFICATION
FRAGWORD	DW	0		; 3 BITS FLAGS, 13 BITS OFFSET
IPTTL		DB	0
IPPROTOCOL	DB	0		; HIGHER LEVEL PROTOCOL
IPCHECKSUM	DW	0		; HEADER CHECKSUM
IPSOURCE	DD	0
IPDEST		DD	0
;
;	FORMAT OF TCP HEADER WITHIN AN IP DATAGRAM
;
;	NOTE THESE FIELDS ARE STORED HI ORDER BYTE FIRST (NOT NORMAL 8086 FORMAT)
;

SOURCEPORT	DW	0
DESTPORT	DW	0

SEQNUM		DD	0
ACKNUM		DD	0

TCPCONTROL	DB	0	; 4 BITS DATA OFFSET 4 RESERVED
TCPFLAGS	DB	0	; (2 RESERVED) URG ACK PSH RST SYN FIN

TCPWINDOW	DW	0
CHECKSUM	DW	0
URGPTR		DW	0
;
;	OPTIONS AND/OR DATA MAY FOLLOW
;
TCPOPTIONS	DB	4 DUP (0)

IPMSG	ENDS
;
;	TCPFLAGS BITS
;
FIN	EQU	1B
SYN	EQU	10B
RST	EQU	100B
PSH	EQU	1000B
ACK	EQU	10000B
URG	EQU	100000B


TCP		DB	'TCP: ',0
ICMP		DB	'ICMP: ',0
LEN		DB	' LEN: ',0

SEQTEXT		DB	' SEQ: ',0
ACKTEXT		DB	'ACK: ',0
WINDTEXT	DB	'WIN: ',0
SYNTEXT		DB	'SYN ',0
FINTEXT		DB	'FIN ',0
RSTTEXT		DB	'RST ',0
PSHTEXT		DB	'PSH ',0
ACKTEXT1	DB	'ACK ',0
URGTEXT		DB	'URG ',0

BADSUM		DB	' CHECKSUM ERROR ',0

;-----------------------------------------------------------------------------;
;          Parameter area for received frame                                  ;
;-----------------------------------------------------------------------------;

PORT_NO		DB	0		; Received port number 0 - 256
VERSION_NO	DB	0		; Version 1 or 2       1,2
POLL_FINAL	DB	0		; Poll or Final ?      P,F
COMM_RESP	DB	0		; Command or Response  C,R
FRAME_TYPE	DB	0		; Frame Type           UI etc in Hex
PID		DB	0		; Protocol ID
FRAME_LENGTH	DW	0		; Length of frame      0 - 65...
NR		DB	0		; N(R) Flag
NS		DB	0		; N(S) Flag
INFO_FLAG	DB	0		; Information Packet ? 0 No, 1 Yes
OPCODE		DB	0		; L4 FRAME TYPE
FRMRFLAG	DB	0

MMASK		DW	0FFFFH

MCOM		DB	1
TRACEFLAG	DB	0
MALL		DB	1
HEADERLN	DB	1
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


CRLF		DB	0DH,0AH

AX25CALL	DB	7 DUP (0)	; WORK AREA FOR AX25 <> NORMAL CALL CONVERSION
NORMCALL	DB	10 DUP (0)	; CALLSIGN IN NORMAL FORMAT
NORMLEN		DW	0		; LENGTH OF CALL IN NORMCALL	
;
;	POWERS OF 10 - 8 4-BYTE NUMBERS FROM 100,000,000 TO 10
;
POWERS	DD	100000000
	DD	10000000
	DD	1000000
	DD	100000
TENK	DD	10000
	DD	1000
WORD100	DD	100
DWORD10	DD	10
;
ACCUM		DB	4 DUP (0)
CONVFLAG	DB	0
SUPPRESS	DB	0		; ZERO SUPPRESS FLAG

TERMDATA	ENDS

TEN	DB	10
B100	DB	100

CODEBYTE	LABEL	BYTE

TERMINIT:

	MOV	BX,TERMDATA
	MOV	DS,BX
	MOV	ES,BX

	CMP	BWFLAG,4DH
	JNE	COLMODE

	MOV	NORMATTR,15		; NORMAL CHARS

COLMODE:

	CALL	GETTIME

	MOV	DX,OFFSET LOGNAME
	MOV	AL,2
	MOV	AH,3DH
	INT	21H

	JNC	OPENOK

	MOV	AH,3CH			; CREATE
	MOV	CX,0
	INT	21H

OPENOK:

	MOV	HANDLE,AX

	MOV	BX,AX
	MOV	CX,0
	MOV	DX,0
	MOV	AL,2			; SKIP TO EOF
	MOV	AH,42H
	INT	21H
;
;
	MOV	DX,OFFSET DATE
	MOV	CX,18
	MOV	BX,HANDLE
	MOV	AH,40H
	INT	21H			; WRITE DATE/TIME TO FILE

	CALL	PUTCRLF


	CALL	REFRESHTL
	CALL	REFRESHBL
;
;	CLEAR SCROLL WINDOW
;
	MOV	AH,6
	MOV	AL,0
	MOV	CH,2
	MOV	CL,0
	MOV	DH,21
	MOV	DL,79
	MOV	BH,2

	INT	10H
;
	MOV	AH,1
	MOV	DX,2			; APPL MASK
	MOV	CX,80H			; APPL FLAGS

	CALL	NODE			; SET APPL MASK TO ENABLE MONITORING
;
; Main background routine - searching first for keyboard and then for Comms characters
; keyboard characters are xmitted up the Comms
; RX comms characters are written to the screen
;
	RET


TERMINAL:

	MOV	BX,TERMDATA
	MOV	DS,BX
	MOV	ES,BX

	CMP	SYSTEM,'I'
	JE	BG1			; IPGATE HAS KEYBOARD

	CMP	REFRESH,0
	JE	BG
;
;	BACK ON DISPLAY
;
	MOV	REFRESH,0
	CALL	STOPSCROLL

BG:
	MOV	AH,1
	INT	16H
	JZ	BG1			; NO KEYBOARD CHAR
;
	MOV	AH,0
	INT	16H			; GET THE CHAR

	CMP	AL,19H			; CTRL/Y
	JNE	KB000
	JMP	CLOSEDOWN

KB000:
	CMP	AL,1BH
	JNE	KB001
;
;	ESC -DISCONNECT
;
	MOV	AH,6
	MOV	CX,2
	CALL	NODE

	JMP SHORT BG1

KB001:

	OR	AL,AL			; TWO KEY PAIR?
	JNZ	NORMKEYBD		; NO

	MOV	AL,AH
;
	CALL	SPECIALKEY
	JMP SHORT BG1

NORMKEYBD:

	CALL	WRKB			; SEND UP THE COMMS

BG1:

	CALL	POLLFORCHAR
;
;	REFRESH CLOCK DISPLAY IF SECOND HAS CHANGED
;
 	MOV	AH,2CH
	INT	21H

	CMP	DH,LASTSEC
	JNE	NEWSEC

	RET				; SECOND NOT CHANGED

NEWSEC:

	MOV	LASTSEC,DH
	CALL	REFTIME			; UPDATE CLOCK ON SCREEN
;
;	GET BUFFERS
;
	MOV	DX,0

	MOV	AH,7
	CALL	NODE

	MOV	DI,OFFSET BUFFS
	MOV	AX,DX			; FREE BUFFERS IN AX
	DIV	B100
	ADD	AL,30H
	STOSB

	MOV	AL,AH
	CBW
	DIV	TEN
	ADD	AX,3030H
	STOSW

	CALL	REFRESHBL

;	UPDATE RTM IF ACTIVE
;
	CMP	RTMFLAG,0
	JE	CHECKCON

	CALL	DISPLAYRTM

CHECKCON:
;
;	CHECK CONN STATUS
;
;
;	AH = 4	Get stream status.  Returns:
;
;		CX = 0 if stream disconnected or CX = 1 if stream connected
;		DX = 0 if no change of state, or DX = 1 if
;		       the connected/disconnected state has changed.
;
;	AH = 5	ACK stream status change.
;

	MOV	AH,4
	CALL	NODE

	OR	DX,DX
	JNZ	CHANGED

	CALL	REFRESHTL
	RET

CHANGED:

	OR	CX,CX
	JNZ	CONNECTED

	MOV	SI,OFFSET DISCONNMSG
	MOV	CX,LDISCON

DISLOOP:

	LODSB
	PUSH	CX
	PUSH	SI
	CALL	PUTCHAR
	POP	SI
	POP	CX
	LOOP	DISLOOP
;
	MOV	AX,SENDCOUNT
	MOV	DX,SENDCOUNT+2
	CALL	CONVBIGNUMBER

	MOV	AL,'/'
	CALL	PUTCHAR

	MOV	AX,RXCOUNT
	MOV	DX,RXCOUNT+2
	CALL	CONVBIGNUMBER

	MOV	AX,0
	MOV	RXCOUNT,AX
	MOV	RXCOUNT+2,AX

	MOV	SENDCOUNT,AX
	MOV	SENDCOUNT+2,AX

	MOV	AL,CR
	CALL	PUTCHAR

;	STOP RTM IF RUNNING
;
	CALL	STOPRTM

CONNECTED:

	MOV	AH,5
	CALL	NODE			; ACK CHANGE

	MOV	DI,OFFSET CALLSIGN
	MOV	CX,5
	MOV	AX,2020H
	REP STOSW

	MOV	DI,OFFSET CALLSIGN
	MOV	AH,8
	CALL	NODE
;
;	DISPLAY SESSION PARAMS
;
	MOV	DI,OFFSET CALLSIGN+10

	PUSH	BX
	CALL	HEXOUT		; PORT
	PUSH	AX
	MOV	AL,20H
	STOSB

	POP	AX
	MOV	AL,AH		; SESSION TYPE
	CALL	HEXOUT
	MOV	AL,20H
	STOSB

	POP	AX
	CALL	HEXOUT		; PACLEN
	MOV	AL,20H
	STOSB

	MOV	AL,CL
	CALL	HEXOUT		; MAXFRAME
	MOV	AL,20H
	STOSB

	MOV	AL,DL
	CALL	HEXOUT		; L4 WINDOW
		
NOCHANGE:

	CALL	REFRESHTL

	RET


SPLBS:
	CALL	KBBSPACE
	RET

SPLCUR:
	CALL	CUR
	CALL	WRITECURSORADDR
	RET

SPLINS:
	CALL	INSRT
	CALL	REFRESHBL
	RET

SPLDEL:
	CALL	DELET
	CALL	REFRESHBL
	RET


F10:

	XOR	SYSTEM,'I'
	MOV	REFRESH,1

	RET

ENDKEY:

	CMP	SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JE	NOSCROLL

	MOV	SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JMP	STOPSCROLL


SPECIALKEY:				; PROCESS EXTENDED CODE KEYS

	CMP	AL,68
	JE	F10

	CMP	AL,59
	JE	F1

	CMP	AL,60
	JE	F2

	CMP	AL,61
	JE	F3

	CMP	AL,62
	JE	F4

	CMP	AL,49H
	JE	PGUP

	CMP	AL,51H
	JE	PGDWN

	CMP	AL,4FH
	JE	ENDKEY

	CMP	AL,75
	JE	SPLBS			; CUR LEFT

	CMP	AL,77
	JE	SPLCUR

	CMP	AL,82
	JE	SPLINS

	CMP	AL,83
	JE	SPLDEL

	RET

PGUP:

	CMP	SCROLLPOINTER,OFFSET SCREENBUFFER+15*80
	JB	NOSCROLL

	SUB	SCROLLPOINTER,15*80
	JMP SHORT SCROLLOK

PGDWN:

	CMP	SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JE	NOSCROLL

	ADD	SCROLLPOINTER,15*80

	CMP	SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JE	STOPSCROLL

SCROLLOK:

	MOV	COLOUR,14
	CALL	REFSCROLL
	RET

STOPSCROLL:
;
;	AT END OF BUFFER, SO CHANGE COLOUR
;
	MOV	COLOUR,3		; NORMAL
	CALL	REFSCROLL

NOSCROLL:
	RET

F1:
	XOR	TRACEFLAG,1
	MOV	AL,TRACEFLAG
	MOV	DI,OFFSET MONOFFON
	CALL	SETMON
	RET

F2:
	XOR	MCOM,1
	MOV	AL,MCOM
	MOV	DI,OFFSET MCOMOFFON
	CALL	SETMON
	RET

F3:
	CALL	GETNEWMMASK
	RET

F4:
;
;	KICK OFF 'NODES'
;
	MOV	AH,12
	MOV	DX,2

	CALL	NODE

	RET

DISPLAYRTM:

	MOV	AH,0
	INT	1AH			; GET TIME

	MOV	AX,DX
	SUB	AX,RTMSTART		; GET TIME

	MOV	DX,0
	MUL	D10
	DIV	D182			; CONVERT TO SECS

	MOV	DX,0
	DIV	D60			; MINS IN AX, SECS IN DX

	OR	AL,30H			; ASSUME LESS THAN 10 MINS! 
	MOV	THISRTM+6,AL

	MOV	AX,DX
	DIV	BYTE PTR D10		; 8BIT DIVIDE - AL = TENS, AH = SECS
	ADD	AX,3030H
	MOV	WORD PTR THISRTM+8,AX

	CALL	REFRESHRTM

	RET
;
GETNEWMMASK:

	MOV	DI,OFFSET MMCHAR1
	MOV	CX,5

MMLOOP:

	MOV	AH,0
	INT	16H			; GET NEXT CHAR

	STOSB

	CMP	AL,0DH
	JE	ENDMM

	LOOP	MMLOOP

ENDMM:

	MOV	SI,OFFSET MMCHAR
	CALL	GETVALUE

	MOV	AX,NEWVALUE
	MOV	MMASK,AX

 	MOV	DI,OFFSET MMASKVAL
	MOV	AL,BYTE PTR MMASK+1
	CALL	HEXOUT
	MOV	AL,BYTE PTR MMASK
	CALL	HEXOUT

	CALL	REFRESHBL
	RET




SETMON:
;
;	SET ON OR OFF ON STATUS LINE
;
;	AL = FLAG, DI=STATUS FIELD
;
  	TEST	AL,1
	MOV	AX,'FF'

	JZ	SETMONOFF

	MOV	AX,' N'

SETMONOFF:

	STOSW
	CALL	REFRESHBL
	RET


WRKB:
	CMP	AL,20H
	JL	KBCTRL
KBNORM:
	MOV	DI,KBPTR
	MOV	[DI],AL

	MOV	BL,7
	MOV	AH,14
	INT	10H

	CALL	CUR
	RET


KBCTRL:

	CMP	AL,7
	JE	KBNORM			; SO WE CAN SEND BELL

	CMP	AL,8
	JE	KBBSPACE

	CMP	AL,0DH
	JE	KBCR

	CMP	AL,12			; CTRL/L
	JE	LOGTOG

	CMP	AL,1AH			; CTRL/Z
	JE	KBNORM			; FOR MBX TERMINATOR

	RET

KBBSPACE:
	CALL	CUL
	CALL	WRITECURSORADDR
	RET

KBCR:
	MOV	DI,KBPTR
	MOV	[DI],AL
	INC	KBPTR
;
;	MAKE SURE CONNECTED
;
	MOV	AH,6
	MOV	CX,1
	CALL	NODE

	MOV	SI,OFFSET KBBUFF
	MOV	CX,KBPTR
	SUB	CX,SI

	ADD	SENDCOUNT,CX
	ADC	SENDCOUNT+2,0

	PUSH	SI
	PUSH	CX

	MOV	AH,2
	CALL	NODE

	POP	CX
	POP	SI

	DEC	CX			; DONT WANT CR
	JZ	KBCR01

KBCR00:

	LODSB

	PUSH	SI
	PUSH	CX

	CALL	NORMCHR			; ECHO

	POP	CX
	POP	SI

	LOOP	KBCR00


	CALL	NEWLNE

KBCR01:

	MOV	KBPTR,OFFSET KBBUFF

	MOV	DI,OFFSET KBBUFF
	MOV	CX,40
	MOV	AX,2020H
	REP STOSW

	MOV	RTMFLAG,1

	MOV	AH,0
	INT	1AH
	MOV	RTMSTART,DX

	CALL	REFRESHBL
	RET

LOGTOG:
	XOR	LOGFLAG,'L'		; FLIP LOGGING FLAG
	CALL	REFRESHTL
	RET

;
;
PUTCRLF:
	MOV	DX,OFFSET CRLF
	MOV	BX,HANDLE
	MOV	CX,2
	MOV	AH,40H
	INT	21H

	RET


CLOSEDOWN:

	CALL	CLOSETERM
	CALL	CLOSEIP

	CALL	STOPNODE

	MOV	AX,4C00H
	INT	21H
;
CLOSETERM:
;
	MOV	AX,TERMDATA
	MOV	DS,AX
	MOV	ES,AX

	MOV	BX,HANDLE
	MOV	AH,3EH
	INT	21H			; CLOSE LOG FILE

	RET

GETTIME:

	MOV	AH,2AH
	INT	21H

	MOV	AL,DL
	CBW
	DIV	TEN
	ADD	AX,3030H
	MOV	WORD PTR DATE,AX
	MOV	AL,DH
	CBW
	DIV	TEN
	ADD	AX,3030H
	MOV	WORD PTR DATE+3,AX

	SUB	CX,1900
	MOV	AL,CL
        CMP     AL,99
        JBE     @F
        SUB     AL,100
@@:
	CBW
	DIV	TEN
	ADD	AX,3030H
	MOV	WORD PTR DATE+6,AX

	MOV	AH,2CH
	INT	21H

	MOV	AL,CH
	CBW
	DIV	TEN
	ADD	AX,3030H
	MOV	WORD PTR TIME,AX

	MOV	AL,CL
	CBW
	DIV	TEN
	ADD	AX,3030H
	MOV	WORD PTR TIME+3,AX

	MOV	AL,DH
	CBW
	DIV	TEN
	ADD	AX,3030H
	MOV	WORD PTR TIME+6,AX

	RET

REFTIME:

	MOV	COLOUR,4
	CALL	GETTIME

	MOV	CX,20			; TIMESTAMP LENGTH
	MOV	SI,OFFSET DATE
	MOV	DX,62

	JMP SHORT REFCOM

REFRESHBL:

	MOV	COLOUR,4
	MOV	DH,22
	MOV	DL,0			; POSITION

	MOV	CX,239			; LENGTH
	MOV	SI,OFFSET TRAILER
	JMP SHORT REFCOM

REFRESHRTM:

	MOV	COLOUR,14
	MOV	DH,24
	MOV	DL,58

	MOV	CX,21			; LENGTH
	MOV	SI,OFFSET LASTRTM
	JMP SHORT REFCOM

REFSCROLL:

	MOV	DH,2
	MOV	DL,0

	MOV	CX,1600
	MOV	SI,SCROLLPOINTER

	JMP SHORT REFCOM

REFRESHTL:

	MOV	COLOUR,4

	CALL	GETTIME

	MOV	DX,0

	MOV	CX,160			; SCREEN LENGTH
	MOV	SI,OFFSET HEADER

REFCOM:

	CMP	SYSTEM,'I'
	JE	SKIP_REFRESH		; NOT OUR DISPLAY

	CMP	BWFLAG,'M'
	JNE	REFLOOP

	MOV	COLOUR,7		; FORCE MONO MODE

REFLOOP:

	MOV	AH,2
	MOV	BH,0
	INT	10H			; SET CURSOR

	LODSB

	PUSH	CX

	MOV	AH,9
	MOV	BX,COLOUR
	MOV	CX,1

	INT	10H

	POP	CX

	INC	DL
	CMP	DL,80
	JNE	DLOK

	INC	DH
	MOV	DL,0

DLOK:

	LOOP	REFLOOP

	CALL	WRITECURSORADDR

SKIP_REFRESH:

	RET
; 
NORMCHR:

	CMP	AL,1AH
	JNE	NORMOK

	MOV	AL,20H			; TO KEEP FILE TIDY
NORMOK:
	CMP	CHAR,80
	JL	NORM000

	PUSH	AX
	PUSH	CX
	PUSH	SI
	CALL	RACK
	POP	SI
	POP	CX
	POP	AX

NORM000:
	MOV	DI,CHAR
	ADD	DI,OFFSET BOTTOMLINE
	STOSB

	CMP	SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JNE	NOPUT

	CALL	PUTDMA

NOPUT:
	INC	CHAR

	RET

RACK:
	CMP	LOGFLAG,'L'
	JNE	RACK30			; LOGGING OFF

	MOV	DX,OFFSET BOTTOMLINE	; BOTTOM LINE
	MOV	BX,HANDLE
	MOV	CX,80

	MOV	SI,OFFSET BOTTOMLINE+79
RACK10:
	CMP	BYTE PTR [SI],20H
	JNE	RACK20				; WRITE LINE

	DEC	SI
	LOOP	RACK10

	JMP	RACK30

RACK20:
	MOV	AH,40H
	INT	21H				; WRITE

	CALL	PUTCRLF

RACK30:
;
	MOV	CX,(SCREENBUFFERLEN-80)/2
	MOV	SI,OFFSET SCREENBUFFER+80
	MOV	DI,OFFSET SCREENBUFFER
	REP MOVSW

	MOV	CX,40
	MOV	AX,2020H
	REP STOSW				; CLEAR BOTTOM LINE

	MOV	CHAR,0
;
	CMP	SYSTEM,'I'
	JE	NOSCROLLDMA

	CMP	SCROLLPOINTER,OFFSET BOTTOMLINE-(19*80)
	JNE	NOSCROLLDMA

;	SCROLL DMA IMAGE
;
	MOV	AH,6
	MOV	AL,1
	MOV	CH,2
	MOV	CL,0
	MOV	DH,21
	MOV	DL,79
	MOV	BH,2

	INT	10H

NOSCROLLDMA:

	RET

WRITECURSORADDR:

	CMP	SYSTEM,'I'
	JE	NO_CURS

	MOV	DX,KBPTR
	SUB	DX,OFFSET KBBUFF	; GIVES CHAR

	MOV	DH,23

	MOV	AH,2
	MOV	BH,0
	INT	10H			; SET CURSOR

NO_CURS:
	RET
;
INSRT:					; INSERT
	MOV	DI,KBPTR
	MOV	CX,OFFSET KBBUFF+80
	SUB	CX,KBPTR
	JCXZ	INSRTEX
INSRT0:
	DEC	CX
	JZ	INSRTEX			; ONLY ONE LEFT
	ADD	DI,CX
INSRT2:
	MOV	SI,DI
	DEC	SI
	STD
	REP MOVSB
	CLD
INSRTEX:
	MOV	BYTE PTR [DI],20H
	RET
;
DELET:					; DELETE 
	MOV	DI,KBPTR
	MOV	CX,OFFSET KBBUFF+80
	SUB	CX,DI
	JCXZ	DELETEX

	DEC	CX
	JZ	DELETEX			; ONLY ONE LEFT
DELET2:
	MOV	SI,DI
	INC	SI
	REP MOVSB
DELETEX:
	MOV	BYTE PTR [DI],20H
	RET
;
NEWLNE:
	CMP	CHAR,0
	JE	NL010			; ALREADY AT START
;
	MOV	CX,80
	SUB	CX,CHAR
	JCXZ	NL010
NL000:
	MOV	AL,20H			; SPACE TO END OF LINE
	PUSH	CX
	CALL	NORMCHR
	POP	CX
	LOOP	NL000

	RET

NL010:
	CALL	RACK
	RET
;
NEWLINE:				; ERASE TO END OF LINE
	MOV	DI,CHAR
	ADD	DI,OFFSET BOTTOMLINE

	MOV	BYTE PTR [DI],20H
	MOV	CX,80
	SUB	CX,CHAR
	JE	DLNEEX			; END OF LINE

	MOV	AL,20H
	REP STOSB
DLNEEX:
	RET
;
CUR:					; CURSOR RIGHT 
	CMP	KBPTR,OFFSET KBBUFF+80
	JE	CURRET

	INC	KBPTR
CURRET:
	RET
;
CUL:					; CURSOR LEFT
	CMP	KBPTR,OFFSET KBBUFF
	JE	CURRET

	DEC	KBPTR
	RET
;

PUTDMA:
;
;	ONLY WRITE TO BOTTOM LINE OF SCREEN
;
	CMP	SYSTEM,'I'
	JE	SKIPPUT			; IPGATE HAS DISPLAY

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
POLLFORCHAR:

	MOV	CX,100
	MOV	AX,0
	MOV	DI,OFFSET NODELINE
	REP STOSW			; CLEAR BUFFER

	MOV	AH,3
	MOV	DI,OFFSET NODELINE
	CALL	NODE

	CMP	CX,0
	JNE	GOTONE
;
	RET

GOTONE:

	MOV	SI,OFFSET NODELINE

	CMP	WORD PTR [SI],0AAAAH
	JNE	GOTDATA
;
;	WRITE TO FILE
;
	MOV	DX,OFFSET NODELINE
	MOV	CX,200
	MOV	BX,HANDLE

	MOV	AH,40H
	INT	21H

	MOV	AX,WORD PTR NODELINE+2

	MOV	DI,OFFSET NODELINE

	PUSH	AX
	MOV	AL,AH
	CALL	HEXOUT
	POP	AX
	CALL	HEXOUT

	MOV	AL,20H
	STOSB

	MOV	CX,5

	JMP SHORT GOTLOOP
	
GOTDATA:

	ADD	RXCOUNT,CX
	ADC	RXCOUNT+2,0

GOTLOOP:

	LODSB

	PUSH	CX
	PUSH	SI

	PUSH	AX
;
;	CHAR RECEIVED - IF RTM RUNNING, CALCULATE AND DISPLAY RESULT
;
	CALL	STOPRTM

	POP	AX

	CMP	AL,20H
	JB	CTRLCHAR
;
	CALL	NORMCHR

BGJJ:

	POP	SI
	POP	CX

	LOOP	GOTLOOP

	RET
;
STOPRTM:

	CMP	RTMFLAG,0
	JE	NORTM

;	CMP	AL,0DH
;	JE	NORTM			; PROBABLY ECHO OF CR

	MOV	RTMFLAG,0

	CALL	DISPLAYRTM

	MOV	SI,OFFSET THISRTM+5
	MOV	DI,OFFSET LASTRTM+5
	MOV	CX,5
	REP MOVSB

NORTM:
	RET

CTRLCHAR:

	CMP	AL,08H
	JE	XBSPACE

	CMP	AL,7
	JE	XBELL

	CMP	AL,0DH
	JNE	BGJJ

	CALL	NEWLNE

	JMP	BGJJ
;
XBSPACE:
	CMP	CHAR,0
	JE	BGJJ

	DEC	CHAR
	CALL	WRITECURSORADDR

	JMP	BGJJ
;
XBELL:
	MOV	AH,14			; TTY OUTPUT
	INT	10H

	JMP	BGJJ

;
GETVALUE:
;
;	EXTRACT NUMBER (HEX OR DECIMAL) FROM INPUT STRING
;
	MOV	NEWVALUE,0

	MOV	AL,ES:[SI]
	CMP	AL,'$'			; HEX?
	JE	DECODEHEX

VALLOOP:

	MOV	AL,ES:[SI]
	CMP	AL,' '
	JBE	ENDVALUE
;
;	ANOTHER DIGIT - MULTIPLY BY 10
;
	MOV	AX,NEWVALUE
	MUL	WORD10
	MOV	NEWVALUE,AX

	MOV	AL,ES:[SI]
	INC	SI
	SUB	al,'0'
	JC	DUFFVALUE
	CMP	AL,10
	JNC	DUFFVALUE

	MOV	AH,0
	ADD	NEWVALUE,AX

	JC	DUFFVALUE
	JMP	VALLOOP

DECODEHEX:

	INC	SI

HEXLOOP:

	MOV	AL,ES:[SI]
	CMP	AL,' '
	JBE	ENDVALUE
;
;	ANOTHER DIGIT - MULTIPLY BY 16
;
	MOV	AX,NEWVALUE
	MUL	WORD16
	MOV	NEWVALUE,AX

	MOV	AL,ES:[SI]
	CMP	AL,'Z'
	JBE	CHAROK

	SUB	AL,20H			; TO LC

CHAROK:

	INC	SI
	SUB	al,'0'
	JC	DUFFVALUE
	CMP	AL,10
	JC	HEXOK
	SUB	AL,7
	CMP	AL,10
	JC	DUFFVALUE

	CMP	AL,16
	JNC	DUFFVALUE
HEXOK:
	MOV	AH,0
	ADD	NEWVALUE,AX
	JMP	HEXLOOP

ENDVALUE:
	CLC
	RET

DUFFVALUE:
	STC
	RET

PUTCHAR:

	PUSH	DI
	CMP	AL,0DH
	JNE	NOTNL

	CALL	NEWLNE
	POP	DI
	RET

NOTNL:

	CALL	NORMCHR
	POP	DI

	RET


TERMTRACE:

	MOV	BX,TERMDATA
	MOV	ES,BX

	MOV	ES:TIMESTAMP,AX

	cmp	ES:TRACEFLAG,0
	jne	TRACEOK


TRACERET:

	ret

TRACEOK:

	MOV	SI,DI
	MOV	DI,OFFSET NODELINE
	MOV	CX,MSGLENGTH[SI]
	REP MOVSB

	MOV	DI,OFFSET NODELINE
	MOV	DS,BX
;
;	GET THE CONTROL BYTE, TO SEE IF THIS FRAME IS TO BE DISPLAYED
;
	PUSH	DI
	MOV	CX,8			; MAX DIGIS
CTRLLOOP:
	TEST	BYTE PTR MSGCONTROL-1[DI],1
	JNZ	CTRLFOUND

	ADD	DI,7
	LOOP	CTRLLOOP
;
;	INVALID FRAME
;
	POP	DI
	RET

CTRLFOUND:
	MOV	AL,MSGCONTROL[DI]
	POP	DI
;
	TEST	AL,1			; I FRAME
	JZ	IFRAME

	AND	AL,NOT PFBIT		; CLEAR P/F
	CMP	AL,3			; UI
	JE	OKTOTRACE		; ALWAYS DO UI

	CMP	AL,FRMR
	JE	OKTOTRACE		; ALWAYS DO FRMR

;
;	USEQ/CONTROL - TRACE IF MCOM ON
;
	CMP	MCOM,0
	JNE	OKTOTRACE

	RET

;-----------------------------------------------------------------------------;
;       Check for MALL                                                        ;
;-----------------------------------------------------------------------------;

IFRAME:
	cmp	MALL,0
	jne	OKTOTRACE

	ret

OKTOTRACE:
;
;-----------------------------------------------------------------------------;
;       Get the port number of the received frame                             ;
;-----------------------------------------------------------------------------;

	mov	CL,MSGPORT[DI]
	mov	PORT_NO,CL

	AND	CL,7FH			; MASK T/R BIT

	DEC	CL
	MOV	AX,1
	SHL	AX,CL			; SHIFT BIT UP

	TEST	MMASK,AX
	JNZ	TRACEOK1

	RET

TRACEOK1:

	MOV	FRMRFLAG,0
	push	DI
	mov	AH,MSGDEST+6[DI]
	mov	AL,MSGORIGIN+6[DI]

	mov	COMM_RESP,0			; Clear Command/Response Flag

;-----------------------------------------------------------------------------;
;       Is it a Poll/Final or Command/Response                                ;
;-----------------------------------------------------------------------------;

	test	AH,80H
	mov	COMM_RESP,'C'
	jnz	NOT_RESPONSE
	mov	COMM_RESP,'R'

NOT_RESPONSE:

;-----------------------------------------------------------------------------;
;       Is this version 1 or 2 of AX25 ?                                      ;
;-----------------------------------------------------------------------------;

	xor	AH,AL
	test	AH,80H
	mov	VERSION_NO,1
	je	VERSION_1
	mov	VERSION_NO,2

VERSION_1:
;
;	DISPLAY TIMESTAMP AND T/R FLAG
;
	PUSH	DS
	MOV	AX,40H
	MOV	DS,AX

	MOV	AL,DS:6EH
	POP	DS			; HOURS

	CALL	DISPLAY_BYTE_2

	MOV	AL,':'
	CALL	PUTCHAR

	MOV	AL,BYTE PTR TIMESTAMP

;	+X,TIMESTAMP
;	MOV	DX,0
;	MOV	DX,0
;	MUL	D10
;	DIV	D182			; CONVERT TO SECS

;	MOV	DX,0
;	DIV	D60			; MINS IN AX, SECS IN DX

;	PUSH	DX

	CALL	DISPLAY_BYTE_2

	MOV	AL,':'
	CALL	PUTCHAR

;	POP	AX			; SECS

	MOV	AL,BYTE PTR TIMESTAMP+1
 	CALL	DISPLAY_BYTE_2

	MOV	AL,'R'
	TEST	PORT_NO,80H
	JZ	TR

	MOV	AL,'T'

TR:

	CALL	PUTCHAR

	MOV	AL,' '
	CALL	PUTCHAR

;-----------------------------------------------------------------------------;
;       Display Origin Callsign                                               ;
;-----------------------------------------------------------------------------;

	lea	SI,MSGORIGIN[DI]
	call	CONVFROMAX25

	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	pop	DI
	push	DI

	mov	AL,'>'
	call	PUTCHAR

;-----------------------------------------------------------------------------;
;       Display Destination Callsign                                          ;
;-----------------------------------------------------------------------------;

	lea	SI,MSGDEST[DI]
	call	CONVFROMAX25

	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	pop	DI
	push	DI

	mov	AX,MSGLENGTH[DI]
	mov	FRAME_LENGTH,AX
	mov	CX,8			; Max number of digi-peaters

;-----------------------------------------------------------------------------;
;       Display any Digi-Peaters                                              ;
;-----------------------------------------------------------------------------;

NEXT_DIGI:
	test	MSGORIGIN+6[DI],1
	jnz	NO_MORE_DIGIS

	add	DI,7
	sub	FRAME_LENGTH,7		; Reduce length

	push	DI
	push	CX
	lea	SI,MSGORIGIN[DI]
	call	CONVFROMAX25		; Convert to call

	push	AX			; Last byte is in AH

	mov	AL,','
	call	PUTCHAR

	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	pop	AX

	test	AH,80H
	jz	NOT_REPEATED

	mov	AL,'*'
	call	PUTCHAR

NOT_REPEATED:
	pop	CX
	pop	DI
	loop	NEXT_DIGI

NO_MORE_DIGIS:	

;----------------------------------------------------------------------------;
;       Display the Port No of the frame                                     ;
;----------------------------------------------------------------------------;

	mov	SI,OFFSET PORT_MSG
	call	NORMSTR

	mov	AL,PORT_NO
	AND	AL,7FH
	call	DISPLAY_BYTE_1
;
	mov	AL,' '
	call	PUTCHAR

;-----------------------------------------------------------------------------;
;       If this is Version 2 get the Poll/Final Bit                           ;
;-----------------------------------------------------------------------------;

	mov	POLL_FINAL,0		; Clear Poll/Final Flag

	mov	AL,MSGCONTROL[DI]	; Get control byte

	cmp	COMM_RESP,'C'
	jne	NOT_COMM

	test	AL,PFBIT
	je	NOT_POLL

	mov	POLL_FINAL,'P'

NOT_POLL:
NOT_COMM:

	cmp	COMM_RESP,'R'
	jne	NOT_RESP

	test	AL,PFBIT
	je	NOT_FINAL

	mov	POLL_FINAL,'F'

NOT_FINAL:
NOT_RESP:

;-----------------------------------------------------------------------------;
;       Start displaying the frame information                                ;
;-----------------------------------------------------------------------------;

	and	AL,NOT PFBIT		; Remove P/F bit
	mov	FRAME_TYPE,AL

	mov	AL,'<'			; Print "<"
	call	PUTCHAR

	mov	NR,0			; Reset all the flags
	mov	NS,0
	mov	INFO_FLAG,0

	mov	AL,FRAME_TYPE

	test	AL,1
	jne	NOT_I_FRAME

;-----------------------------------------------------------------------------;
;       Information frame                                                     ;
;-----------------------------------------------------------------------------;

	mov	AL,'I'
	call	PUTCHAR
	mov	INFO_FLAG,1
	mov	NR,1
	mov	NS,1
	jmp	END_OF_TYPE

NOT_I_FRAME:

;-----------------------------------------------------------------------------;
;       Un-numbered Information Frame                                         ;
;-----------------------------------------------------------------------------;

	cmp	AL,UI
	jne	NOT_UI_FRAME

	mov	SI,OFFSET UI_MSG
	call	NORMSTR
	mov	INFO_FLAG,1
	jmp	END_OF_TYPE

NOT_UI_FRAME:
	test	AL,10B
	jne	NOT_R_FRAME

;-----------------------------------------------------------------------------;
;       Process supervisory frames                                            ;
;-----------------------------------------------------------------------------;

	mov	NR,1			; All supervisory frames have N(R)

	and	AL,0FH			; Mask the interesting bits
	cmp	AL,RR	
	jne	NOT_RR_FRAME

	mov	SI,OFFSET RR_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_RR_FRAME:
	cmp	AL,RNR
	jne	NOT_RNR_FRAME

	mov	SI,OFFSET RNR_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_RNR_FRAME:
	cmp	AL,REJ
	jne	NOT_REJ_FRAME

	mov	SI,OFFSET REJ_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_REJ_FRAME:
	mov	NR,0			; Don't display sequence number
	mov	AL,'?'			; Print "?"
	call	PUTCHAR
	jmp	SHORT END_OF_TYPE

;-----------------------------------------------------------------------------;
;       Process all other frame types                                         ;
;-----------------------------------------------------------------------------;

NOT_R_FRAME:
	cmp	AL,UA
	jne	NOT_UA_FRAME

	mov	SI,OFFSET UA_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_UA_FRAME:
	cmp	AL,DM
	jne	NOT_DM_FRAME

	mov	SI,OFFSET DM_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_DM_FRAME:
	cmp	AL,SABM
	jne	NOT_SABM_FRAME

	mov	AL,'C'
	call	PUTCHAR
	jmp	SHORT END_OF_TYPE

NOT_SABM_FRAME:
	cmp	AL,DISC
	jne	NOT_DISC_FRAME

	mov	AL,'D'
	call	PUTCHAR
	jmp	SHORT END_OF_TYPE

NOT_DISC_FRAME:
	cmp	AL,FRMR
	jne	NOT_FRMR_FRAME

	mov	SI,OFFSET FRMR_MSG
	call	NORMSTR
	MOV	FRMRFLAG,1
	jmp	SHORT END_OF_TYPE

NOT_FRMR_FRAME:
	mov	AL,'?'
	call	PUTCHAR

END_OF_TYPE:

;----------------------------------------------------------------------------;
;       If Version 2 Then display P/F C/R Information                        ;
;----------------------------------------------------------------------------;

	cmp	VERSION_NO,2
	jne	NOT_VERSION_2

	mov	AL,' '
	call	PUTCHAR

	mov	AL,COMM_RESP		; Print Command/Response Flag
	call	PUTCHAR

	cmp	POLL_FINAL,0
	je	NO_POLL_FINAL

	mov	AL,' '
	call	PUTCHAR

	mov	AL,POLL_FINAL		; Print Poll/Final Flag if Set
	call	PUTCHAR

NO_POLL_FINAL:
NOT_VERSION_2:

;----------------------------------------------------------------------------;
;       Display sequence numbers if applicable                               ;
;----------------------------------------------------------------------------;

	cmp	NS,1
	jne	NOT_NS_DATA

	mov	AL,' '
	call	PUTCHAR

	mov	AL,'S'
	call	PUTCHAR

	mov	AL,FRAME_TYPE
	ror	AL,1

	call	DISPLAYSEQ

NOT_NS_DATA:
	cmp	NR,1
	jne	NOT_NR_DATA

	mov	AL,' '
	call	PUTCHAR

	mov	AL,'R'
	call	PUTCHAR

	mov	AL,FRAME_TYPE
	rol	AL,1
	rol	AL,1
	rol	AL,1

	call	DISPLAYSEQ

NOT_NR_DATA:
	mov	AL,'>'
	call	PUTCHAR

	CMP	FRMRFLAG,0
	JE	NOTFRMR
;
;	DISPLAY FRMR BYTES
;
	lea	SI,MSGPID[DI]
	MOV	CX,3			; TESTING
FRMRLOOP:
	lodsb
	CALL	BYTE_TO_HEX

	LOOP	FRMRLOOP

	JMP	NO_INFO

NOTFRMR:
;----------------------------------------------------------------------------;
;       Find the PID if an information frame                                 ;
;----------------------------------------------------------------------------;

	mov	AL,0

	cmp	INFO_FLAG,1
	jne	NO_PID

	lea	SI,MSGPID[DI]
	lodsb

NO_PID:
	mov	PID,AL

;----------------------------------------------------------------------------;
;       Is this a NET/ROM message of any sort ?                              ;
;----------------------------------------------------------------------------;

	MOV	CX,FRAME_LENGTH

	cmp	PID,NETROM_PID
	je	DISPLAY_NETROM

;----------------------------------------------------------------------------;
;       Display the rest of the frame (If Any)                               ;
;----------------------------------------------------------------------------;

DISPLAY_INFO:

	cmp	INFO_FLAG,1		; Is it an information packet ?
	jne	NO_INFO

	mov	AL,':'
	call	PUTCHAR

	XOR	AL,AL			; IN CASE EMPTY

	sub	CX,21
	JCXZ	NO_INFO			; EMPTY I FRAME

	TEST	HEADERLN,1
	JZ	SAMELN
;
;	PUT TEXT ON A NEW LINE
;
	PUSH	CX
	MOV	AL,0DH
	PUSH	SI
	CALL	PUTCHAR
	POP	SI
	POP	CX

SAMELN:

	cmp	CX,257
	jb	LENGTH_OK

	mov	CX,256

LENGTH_OK:

	push	CX
	lodsb
	
	cmp	AL,0AH
	JE	MONOK
	CMP	AL,0DH
	JE	MONOK

	CMP	AL,20H
	JB	SKIP_MON		; IGNORE OTHER CONTROLS

MONOK:
	PUSH	AX
	PUSH	SI
	call	PUTCHAR
	POP	SI
	POP	AX

SKIP_MON:

	pop	CX
	loop	LENGTH_OK

NO_INFO:
;
;	ADD CR UNLESS DATA ALREADY HAS ONE
;
	CMP	AL,CR
	JE	NOTANOTHER
ADD_CR:
	mov	AL,CR
	call	PUTCHAR

NOTANOTHER:
;
	pop	DI
	ret

;----------------------------------------------------------------------------;
;       Display NET/ROM data                                                 ;
;----------------------------------------------------------------------------;

DISPLAY_NETROM:

	lodsb
	cmp	AL,NODES_SIG		; Check NODES message
	je	DISPLAY_NODES
	jmp	DISPLAY_NETROM_DATA

DISPLAY_NODES:

;----------------------------------------------------------------------------;
;       Display NODES broadcast                                              ;
;----------------------------------------------------------------------------;

	push	CX
	push	SI

	mov	SI,OFFSET NODES_MSG
	call	NORMSTR

	pop	SI
	push	SI

	mov	CX,6
	call	DISPADDR

	mov	AL,CR
	call	PUTCHAR

	pop	SI
	pop	CX

	add	SI,6			; Point to first destination
	sub	CX,28			; Header, mnemonic and signature length

NODES_LOOP:
	cmp	CX,0
	jbe	NO_INFO

	push	CX
	push	SI			; Global push for each node

	push	SI			; Local push 

	mov	AL,' '
	call	PUTCHAR
	mov	AL,' '
	call	PUTCHAR

	pop	SI
	push	SI

	add	SI,7			; Display destination mnemonic

	cmp	BYTE PTR [SI],' '
	je	NO_MNEMONIC

	mov	CX,6			; Max length

MNEMONIC_LOOP:
	lodsb				; Get character

	cmp	AL,' '			; Short mnemonic ?
	je	END_MNEMONIC

	call	PUTCHAR

	loop	MNEMONIC_LOOP

END_MNEMONIC:
	mov	AL,':'
	call	PUTCHAR

NO_MNEMONIC:
	pop	SI
	push	SI

	call	CONVFROMAX25		; Display dest callsign
	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	mov	SI,OFFSET VIA_MSG
	call	NORMSTR

	pop	SI
	add	SI,13			; Point to neighbour callsign
	push	SI

	call	CONVFROMAX25
	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	mov	SI,OFFSET QUALITY_MSG
	call	NORMSTR

	pop	SI
	add	SI,7			; Point to quality byte

	mov	AL,[SI]
	call	DISPLAY_BYTE_1

	mov	AL,CR
	call	PUTCHAR

	pop	SI
	pop	CX
	add	SI,21			; Point to next destination
	sub	CX,21			; Remove length of each 

	jmp	SHORT NODES_LOOP

;----------------------------------------------------------------------------;
;       Display normal NET/ROM transmissions                                 ;
;----------------------------------------------------------------------------;

DISPLAY_NETROM_DATA:

	DEC	SI			; BACK TO DATA
	PUSH	SI

	mov	SI,OFFSET NETROM_MSG
	call	NORMSTR

	pop	SI
	PUSH	SI

	call	CONVFROMAX25
	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	mov	SI,OFFSET TO_MSG
	call	NORMSTR

	pop	SI
	add	SI,7
	push	SI

	call	CONVFROMAX25
	mov	SI,OFFSET NORMCALL
	call	DISPADDR

;
;       Display Time To Live number
;
	mov	SI,OFFSET TTL_MSG
	call	NORMSTR

	pop	SI
	add	SI,7			; Point to TTL counter

	lodsb
	call	DISPLAY_BYTE_1

;
;	DISPLAY CIRCUIT ID
;
	PUSH	SI

	MOV	SI,OFFSET CCT_MSG
	CALL	NORMSTR

	POP	SI
	LODSB
	CALL	BYTE_TO_HEX

	LODSB
	CALL	BYTE_TO_HEX

	INC	SI
	INC	SI		; TO OPCODE

;-----------------------------------------------------------------------------;
;       Determine type of Level 4 frame                                       ;
;-----------------------------------------------------------------------------;

	mov	AL,[SI]
	MOV	OPCODE,AL		; SAVE
	AND	AL,0FH			; Strip off flags

	cmp	AL,L4CREQ
	jne	NOT_L4CREQ

	PUSH	SI
	mov	SI,OFFSET CONN_REQ_MSG
	call	NORMSTR

	MOV	SI,OFFSET WINDOW
	CALL	NORMSTR

	POP	SI
	INC	SI
	LODSB			; WINDOW SIZE

	CALL	DISPLAY_BYTE_1

	mov	AL,' '
	call	PUTCHAR

	PUSH	SI

	call	CONVFROMAX25
	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	mov	SI,OFFSET AT_MSG
	call	NORMSTR

	pop	SI
	add	SI,7
	PUSH	SI

	call	CONVFROMAX25
	mov	SI,OFFSET NORMCALL
	call	DISPADDR


	POP	SI
	CMP	FRAME_LENGTH,56
	JE	NOT_BPQ
;
;	BPQ EXTENDED CON REQ - DISPLAY TIMEOUT
;
	PUSH	SI
	MOV	SI,OFFSET TIM_MSG
	CALL	NORMSTR

	POP	SI
	MOV	AX,7[SI]		; TIMEOUT

	CALL	DISPLAY_BYTE_1
;
NOT_BPQ:

	JMP	ADD_CR

NOT_L4CREQ:

	cmp	AL,L4CACK
	jne	NOT_L4CACK

	TEST	OPCODE,L4BUSY
	JZ	L4CRQ00
;
;	BUSY RETURNED
;
	MOV	 SI,OFFSET CONN_NAK_MSG
	CALL	NORMSTR

	JMP	END_NETROM

L4CRQ00:
	PUSH	SI

	MOV	SI,OFFSET CONN_ACK_MSG
	CALL	NORMSTR

	MOV	SI,OFFSET WINDOW
	CALL	NORMSTR

	POP	SI
	MOV	AL,1[SI]			; WINDOW SIZE

	CALL	DISPLAY_BYTE_1

	PUSH	SI
	MOV	SI,OFFSET MYCCT_MSG
	CALL	NORMSTR

	POP	SI
	MOV	AL,-2[SI]
	CALL	BYTE_TO_HEX

	MOV	AL,-1[SI]
	CALL	BYTE_TO_HEX

	JMP	ADD_CR

NOT_L4CACK:

	cmp	AL,L4DREQ
	jne	NOT_L4DREQ

	mov	SI,OFFSET DISC_REQ_MSG
	call	NORMSTR

	JMP	ADD_CR

NOT_L4DREQ:

	cmp	AL,L4DACK
	jne	NOT_L4DACK

	mov	SI,OFFSET DISC_ACK_MSG
	call	NORMSTR

	jmp	add_cr

NOT_L4DACK:

	cmp	AL,L4INFO
	jne	NOT_L4INFO

	push	SI

	mov	SI,OFFSET INFO_MSG
	call	NORMSTR

	pop	SI

	mov	AL,-2[SI]			; Get send sequence number
	call	DISPLAY_BYTE_1

	mov	AL,' '
	call	PUTCHAR
	mov	AL,'R'
	call	PUTCHAR

	mov	AL,-1[SI]		; Get receive sequence number
	call	DISPLAY_BYTE_1

	mov	AL,'>'
	call	PUTCHAR

	INC	SI			; TO DATA
	MOV	CX,FRAME_LENGTH
	sub	CX,20
	
	CALL	DOL4FLAGS

	jmp	DISPLAY_INFO


NOT_L4INFO:

	cmp	AL,L4IACK
	jne	NOT_L4IACK

	push	SI

	mov	SI,OFFSET INFO_ACK_MSG
	call	NORMSTR

	pop	SI

	mov	AL,-1[SI]			; Get receive sequence number
	call	DISPLAY_BYTE_1

	mov	AL,'>'
	call	PUTCHAR

	CALL	DOL4FLAGS

	JMP SHORT END_NETROM

NOT_L4IACK:

	OR	AL,AL
	JNZ	NOTIP
;
;	TCP/IP DATAGRAM
;
	PUSH	SI
	mov	SI,OFFSET IP_MSG
	call	NORMSTR
	POP	SI
;
	INC	SI			; NOW POINTING TO IP HEADER
;
	PUSH	SI

	LEA	SI,IPSOURCE[SI]
	CALL	PRINT4			; PRINT IF ADDR IN 'DOTTED DECIMAL' FORMAT

	POP	SI

	MOV	AL,'>' 
	CALL	PUTCHAR

	PUSH	SI

	LEA	SI,IPDEST[SI]
	CALL	PRINT4			; PRINT IF ADDR IN 'DOTTED DECIMAL' FORMAT

	MOV	SI,OFFSET LEN
	CALL	NORMSTR

	POP	SI

	MOV	AL,BYTE PTR IPLENGTH[SI]
	CALL	BYTE_TO_HEX

	MOV	AL,BYTE PTR IPLENGTH+1[SI]
	CALL	BYTE_TO_HEX

	MOV	AL,20H
	CALL	PUTCHAR

	MOV	AL,IPPROTOCOL[SI]
	CALL	DISPLAY_BYTE_1		; DISPLAY PROTOCOL TYPE

;	mov	AL,CR
;	call	PUTCHAR
;
;	MOV	CX,39			; TESTING
;IPLOOP:
;	lodsb
;	CALL	BYTE_TO_HEX
;
;	LOOP	IPLOOP

	JMP	ADD_CR

NOTIP:

	mov	SI,OFFSET DUFF_NET_MSG
	call	NORMSTR


END_NETROM:

	jmp	add_cr

DOL4FLAGS:
;
;	DISPLAY BUSY/NAK/MORE FLAGS
;
	TEST	OPCODE,L4BUSY
	JZ	L4F010

	MOV	AL,'B'
	CALL	PUTCHAR
L4F010:
	TEST	OPCODE,L4NAK
	JZ	L4F020

	MOV	AL,'N'
	CALL	PUTCHAR
L4F020:
	TEST	OPCODE,L4MORE
	JZ	L4F030

	MOV	AL,'M'
	CALL	PUTCHAR
L4F030:
	RET

;----------------------------------------------------------------------------;
;       Display ASCIIZ strings                                               ;
;----------------------------------------------------------------------------;

NORMSTR:
	lodsb
	cmp	AL,NULL		; End of String ?
	je	NORMSTR_RET	; Yes
	call	PUTCHAR
	jmp	SHORT NORMSTR

NORMSTR_RET:
	ret

;-----------------------------------------------------------------------------;
;       Display sequence numbers                                              ;
;-----------------------------------------------------------------------------;

DISPLAYSEQ:
	and	AL,7
	add	AL,30H
	call	PUTCHAR
	ret

;-----------------------------------------------------------------------------;
;       Display Callsign pointed to by SI                                     ;
;-----------------------------------------------------------------------------;

DISPADDR:
	
	jcxz	DISPADDR_RET

	LODSB
	call	PUTCHAR

	loop	DISPADDR

DISPADDR_RET:
	ret


PRINT4:
;
;	DISPLAY IP ADDR IN DOTTED DECIMAL FORMAT
;

	LODSB
	CALL	DISPLAY_BYTE_1
	MOV	AL,'.'
	CALL	PUTCHAR

	LODSB
	CALL	DISPLAY_BYTE_1
	MOV	AL,'.'
	CALL	PUTCHAR

	LODSB
	CALL	DISPLAY_BYTE_1
	MOV	AL,'.'
	CALL	PUTCHAR

	LODSB
	CALL	DISPLAY_BYTE_1

	RET


	
;-----------------------------------------------------------------------------;
;       Convert byte in AL to nnn, nn or n format                             ;
;-----------------------------------------------------------------------------;

DISPLAY_BYTE_1:

	cmp	AL,100
	jb	TENS_1

	mov	AH,0

HUNDREDS_LOOP_1:
	cmp	AL,100
	jb	HUNDREDS_LOOP_END_1

	sub	AL,100
	inc	AH
	jmp	SHORT HUNDREDS_LOOP_1

HUNDREDS_LOOP_END_1:
	push	AX
	mov	AL,AH
	add	AL,30H
	call	PUTCHAR
	pop	AX
	jmp	SHORT TENS_PRINT_1

TENS_1:
	cmp	AL,10
	jb	UNITS_1

TENS_PRINT_1:
	mov	AH,0

TENS_LOOP_1:
	cmp	AL,10
	jb	TENS_LOOP_END_1

	sub	AL,10
	inc	AH
	jmp	SHORT TENS_LOOP_1

TENS_LOOP_END_1:
	push	AX
	mov	AL,AH
	add	AL,30H
	call	PUTCHAR
	pop	AX

UNITS_1:
	add	AL,30H
	call	PUTCHAR

	ret

;-----------------------------------------------------------------------------;
;       Convert byte in AL to nn format                                       ;
;-----------------------------------------------------------------------------;

DISPLAY_BYTE_2:
	cmp	AL,100
	jb	TENS_2

	sub	AL,100
	jmp	SHORT DISPLAY_BYTE_2

TENS_2:
	mov	AH,0

TENS_LOOP_2:
	cmp	AL,10
	jb	TENS_LOOP_END_2

	sub	AL,10
	inc	AH
	jmp	SHORT TENS_LOOP_2

TENS_LOOP_END_2:
	push	AX
	mov	AL,AH
	add	AL,30H
	call	PUTCHAR
	pop	AX

UNITS_2:
	add	AL,30H
	call	PUTCHAR

	ret

;-----------------------------------------------------------------------------;
;       Convert byte in AL to Hex display                                     ;
;-----------------------------------------------------------------------------;

BYTE_TO_HEX:
	push	AX
	shr	AL,1
	shr	AL,1
	shr	AL,1
	shr	AL,1
	call	NIBBLE_TO_HEX
	pop	AX
	call	NIBBLE_TO_HEX
	ret

NIBBLE_TO_HEX:
	and	AL,0FH
	cmp	AL,10

	jb	LESS_THAN_10
	add	AL,7

LESS_THAN_10:
	add	AL,30H
	call	PUTCHAR
	ret



CONVFROMAX25:
;
;	CONVERT AX25 FORMAT CALL IN [SI] TO NORMAL FORMAT IN NORMCALL
;	   RETURNS LENGTH IN CX AND NZ IF LAST ADDRESS BIT IS SET
;
	PUSH	SI			; SAVE
	MOV	DI,OFFSET NORMCALL
	MOV	CX,10			; MAX ALPHANUMERICS
	MOV	AL,20H
	REP STOSB			; CLEAR IN CASE SHORT CALL
	MOV	DI,OFFSET NORMCALL
	MOV	CL,6
CONVAX50:
	LODSB
	CMP	AL,40H
	JE	CONVAX60		; END IF CALL - DO SSID

	SHR	AL,1
	STOSB
	LOOP	CONVAX50
CONVAX60:
	POP	SI
	ADD	SI,6			; TO SSID
	LODSB
	MOV	AH,AL			; SAVE FOR LAST BIT TEST
	SHR	AL,1
	AND	AL,0FH
	JZ	CONVAX90		; NO SSID - FINISHED
;
	MOV	BYTE PTR ES:[DI],'-'
	INC	DI
	CMP	AL,10
	JB	CONVAX70
	SUB	AL,10
	MOV	BYTE PTR ES:[DI],'1'
	INC	DI
CONVAX70:
	ADD	AL,30H			; CONVERT TO DIGIT
	STOSB
CONVAX90:
	MOV	CX,DI
	SUB	CX,OFFSET NORMCALL
	MOV	NORMLEN,CX		; SIGNIFICANT LENGTH

	TEST	AH,1			; LAST BIT SET?
	RET

NODE:
	MOV	AL,PORT

	PUSHF				; RETURN IS VIA IRET

	CALL	BPQHOSTPORT
	RET


HEXOUT:

	PUSH	AX
	PUSH	AX
	sar	al,1
	sar	al,1
	sar	al,1
	sar	al,1
	call	hexout1
	pop	ax
	call	hexout1
	POP	AX
	ret

hexout1:
	and	al,0fh
	cmp	al,10
	jl	hexout5
	add	al,7
hexout5:
	add	al,30h
	STOSB
;
	ret
CONVBIGNUMBER:

	MOV	SUPPRESS,1		; ZERO SUPRESS;
	JMP 	SHORT DEC000
;
;	THIS ROUTINE CONVERTS A 32 BIT BINARY NUMBER IN DX,AX
;	TO PRINTABLE DECIMAL CHARS
;
CONVTO10DIGITS:

	MOV	SUPPRESS,0
DEC000:
	PUSH	BX
	PUSH	SI
	PUSH	CX

	MOV	CONVFLAG,0
	MOV	CX,8
	MOV	WORD PTR ACCUM,AX
	MOV	WORD PTR ACCUM+2,DX

	MOV	SI,OFFSET POWERS	; LOW ORDER OF FIRST
DEC010:
	MOV	BL,0			; LOOP COUNT FOR SUBTRACT
DEC020:
	CALL	SUB4
	JC	DEC030			; TOO SMALL
	INC	BL			; NUMBER OF SUBTRACTS
	JMP	DEC020
;
DEC030:
	CALL	ADD4			; PUT BACK
	MOV	AL,BL
	ADD	AL,'0'			; TO CHARACTER FORM
	TEST	CONVFLAG,1
	JNZ	DEC040			; ALREADY HAVE A SIGNIFICANT CHAR
	CMP	AL,'0'			; IS IT A ZERO
	JE	DEC036			; YES, SO CHANGE TO SPACE
	OR	CONVFLAG,1		; SET NON-ZERO CHAR FOUND
	JMP SHORT DEC040		; AND PRINT IT
;
DEC036:
	CMP	SUPPRESS,1
	JE	DEC044			; SUPPRESS LEADING SPACES

	MOV	AL,' '
DEC040:
	PUSH	CX
	PUSH	SI
	CALL	PUTCHAR
	POP	SI
	POP	CX
DEC044:
	ADD	SI,4
	LOOP	DEC010

	MOV	AL,ACCUM		; REMAINDER
	ADD	AL,'0'
	CALL	PUTCHAR			; PRINT IT
;
	POP	CX
	POP	SI
	POP	BX

	RET
;
;	ROUTINE TO ADD TWO 4 BYTE NUMBERS
;
;
ADD4:
	MOV	AL,[SI]
	ADD	ACCUM,AL
	MOV	AL,[SI+1]
	ADC	ACCUM+1,AL
	MOV	AL,[SI+2]
	ADC	ACCUM+2,AL
	MOV	AL,[SI+3]
	ADC	ACCUM+3,AL

	RET
;
;	ROUTINE TO SUBTRACT TWO 4 BYTE NUMBERS
;
;
SUB4:
	MOV	AL,[SI]
	SUB	ACCUM,AL
	MOV	AL,[SI+1]
	SBB	ACCUM+1,AL
	MOV	AL,[SI+2]
	SBB	ACCUM+2,AL
	MOV	AL,[SI+3]
	SBB	ACCUM+3,AL

	RET

CODE	ENDS
;               
	END



