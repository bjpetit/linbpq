 	PAGE    62,132
;
CODE   SEGMENT PUBLIC 'CODE'

	ASSUME  CS:CODE,DS:CODE,ES:CODE
;
;	DED HOST MODE SUPPORT ROUTINE FOR BPQ SWITCH
;
;	THIS USES A SPECIAL HOST MODE INTERFACE ROUTINE IN THE SWITCH
;
;	THE INTERFACE IS CALLED USING INT 7FH
;
;

	INCLUDE	ENVIRON.ASM

	ORG	100H

GO:

	JMP	INIT

APPLFLAGS	DB	42H		; REQUEST AUTOTIMERS, MSG TO USER
DEDMODE		DB	'C'		; CLUSTER MODE - DONT ALLOW DUP CONNECTS

	EVEN

RXCOUNT		DW	0
RXPOINTER	DW	OFFSET RXBUFFER
PUTPTR		DW	OFFSET RXBUFFER

RXBUFFER	DB	512 DUP (0)	; BUFFER TO PC APPLICATION 
ENDRXBUFFER	LABEL	BYTE


CURSOR		DW	OFFSET	LINEBUFFER
LINEBUFFER	DB	300 DUP (0)	; MSG FROM PC APPL


NODEBUFFER	DB	300 DUP (0)	; MESSAGE FROM NODE
WORKAREA	DB	300 DUP (0)	; UNPACK AREA FOR CHAINED BUFFERS
;
ECHOFLAG	DB	0		; ECHO ENABLED
MODE		DB	0		; INITIALLY TERMINAL MODE
HOSTSTATE	DB	0		; HOST STATE MACHINE 
MSGCOUNT	DW	0		; LENGTH OF MESSAGE EXPECTED
MSGLENGTH	DW	0
MSGTYPE		DB	0
MSGCHANNEL	DB	0

APPLMASK	DB	0
STREAMBASE	DB	0
FRIENDLY	DB	1

HOSTSTREAMS	DW	32		; MAX STREAMS

PARAMREPLY	DB	'* '
PARAMPORT	db	'0 0 64 10 4 4 10 100 18000 30 2 0 2',0DH,0AH

LPARAMREPLY	EQU	$-PARAMREPLY

BADCMDREPLY	DB	02,'INVALID COMMAND',0
LBADCMDREPLY	EQU	$-BADCMDREPLY

ICMDREPLY	DB	02,'         ',0
LICMDREPLY	EQU	$-ICMDREPLY

DATABUSYMSG	DB	02,'TNC BUSY - LINE IGNORED',0
LDATABUSY	EQU	$-DATABUSYMSG

BADCONNECT	DB	02,'INVALID CALLSIGN',0
LBADCONNECT	EQU	$-BADCONNECT

BUSYMSG		DB	'BUSY fm SWITCH',0

CONSWITCH	DB	3,'(1) CONNECTED to 0:'
CONCALL		DB	10 DUP (20H),0
LCONSWITCH	EQU	$-CONSWITCH

DISCMSG		DB	3,'(1) DISCONNECTED fm 0:SWITCH',0
LDISC		EQU	$-discmsg

SWITCH		DB	1,'0:'
CHECKCALL	DB	'SWITCH    ',0
LSWITCH		EQU	$-SWITCH
	
NOTCONMSG	DB	1,'CHANNEL NOT CONNECTED',0
LNOTCON		EQU	$-NOTCONMSG


ALREADYCONMSG	DB	'You are already connected on another port',0dh
ALREADYLEN	EQU	$-ALREADYCONMSG

D10		DB	10
D100		DW	100

MONITORDATA	DB	350 DUP (0)	; RAW FRAME FROM NODE

MONBUFFER	DB	6,0,256 DUP (0)
MONLENGTH	DW	0

MONCURSOR	DW	0

MONHEADER	DB	256 DUP (0)

MONFLAG		DB	0

CCT_STRUC	STRUC
;
;	CHANNEL CONTROL TABLE - ONE ENTRY PER STREAM
;
CHAN_TXQ	DW	?		; FRAMES QUEUED TO NODE
;
CCT_STRUC	ENDS

CCT		DB	33 * TYPE CCT_STRUC DUP (0)

CCTLEN		DB	TYPE CCT_STRUC

	EVEN
;
;	BUFFER POOL USES A SYSTEM OF CHAINED SMALL BUFFERS
;
;	FIRST WORD OF FIRST IS USED FOR CHAINING MESSAGES
;	SECOND WORD IS LENGTH OF DATA
;	LAST WORD IS POINTER TO NEXT BUFFER
;
;	ALL BUT LAST TWO BYTES OF SUBSEQUENT BUFFERS ARE USED FOR DATA
;
;	BUFFERS ARE ONLY USED TO STORE OUTBOUND MESSAGES WHEN SESSION IS
;	BUSY, OR NODE IS CRITICALLY SHORT OF BUFFERS
;
BUFFERS		EQU	1099
BUFFLEN		EQU	32

FREE_Q		DW	0

QCOUNT		DW	0
MINBUFFCOUNT	DW	BUFFERS

NOBUFFCOUNT	DW	0


INTFF:

	ASSUME	ES:NOTHING
;
;	THIS LOT IS CHECKED BY THS
;

	PUSH	BX			; MUST BE FIRST INSTRUCTION 
	PUSH	DS

PATCHDS	EQU	$+1

	MOV	BX,00			; PATCH IN ON LOAD
	MOV	DS,BX

	TEST	AH,AH
	JZ	L12B
;
;	CAN NOW DO WHAT WE LIKE!
;
	JMP SHORT L12B

	DB	1EH DUP (90H)

L12B:

	STI

	PUSH	CX
	PUSH	DX
	PUSH	ES
	PUSH	BP
	PUSH	SI
	PUSH	DI

	MOV	ES,BX

	CMP	AH,0
	JNE	PUTBYTE

	JMP	GETCHAR

PUTBYTE:
;
;	PUT CHAR
;
;	push	ax
;	call	printit
;	pop	ax
;
	CMP	MODE,1
	JNE	CHARMODE
;
;	HOST MODE
;
	CMP	HOSTSTATE,0
	JE	SETCHANNEL

	CMP	HOSTSTATE,1
	JE	SETMSGTYPE

	CMP	HOSTSTATE,2
	JE	SETLENGTH
;
;	RECEIVING COMMAND/DATA
;
	MOV	BX,CURSOR
	MOV	[BX],AL
	INC	BX
	MOV	CURSOR,BX
;
	DEC	MSGCOUNT
	JNZ	GETCHAR				; MORE TO COME

	CALL	PROCESSHOSTPACKET

	MOV	HOSTSTATE,0
	MOV	CURSOR,OFFSET LINEBUFFER

	MOV	AH,0
	JMP SHORT NOCHARS

SETLENGTH:

	MOV	AH,0
	INC	AX
	MOV	MSGCOUNT,AX			; WORKING FIELD
	MOV	MSGLENGTH,AX

	JMP SHORT HOSTREST

SETMSGTYPE:

	MOV	MSGTYPE,AL
	JMP SHORT HOSTREST

SETCHANNEL:

	MOV	MSGCHANNEL,AL

HOSTREST:

	INC	HOSTSTATE
	JMP SHORT GETCHAR

CHARMODE:

	CMP	AL,11H
	JE	IGNORE

	CMP	AL,18H
	JNE	NOTCANLINE
;
;	CANCEL INPUT
; 
	MOV	CURSOR,OFFSET LINEBUFFER
	JMP SHORT IGNORE

NOTCANLINE:

	MOV	BX,CURSOR
	MOV	[BX],AL
	INC	BX

	CMP	BX,OFFSET LINEBUFFER+300
	JE	OVERFLOW

	MOV	CURSOR,BX
;
OVERFLOW:

;	PUSH	AX
;	CALL	PRINTIT
;	POP	AX

	CMP	AL,0DH
	JNE	NOTCR

;	PUSH	AX
;	MOV	AL,0AH
;	CALL	PRINTIT
;	POP	AX

;
;	PROCESS COMMAND (UNLESS HOST MODE
;
	CALL	DOCOMMAND

	MOV	AH,0
	JMP SHORT NOCHARS

NOTCR:
IGNORE:
GETCHAR:

	MOV	AH,0			; NOTHING DOING 

	CMP	RXCOUNT,0
	JE	NOCHARS

	MOV	BX,RXPOINTER
	MOV	AL,[BX]
	INC	BX

	CMP	BX,OFFSET ENDRXBUFFER
	JNE	GETRET

	MOV	BX,OFFSET RXBUFFER
GETRET:
	MOV	RXPOINTER,BX

	DEC	RXCOUNT

	MOV	AH,1

;	PUSH	AX
;	CALL	HEXOUT
;	POP	AX

NOCHARS:

	POP	DI
	POP	SI
	POP	BP
	POP	ES
	POP	DX
	POP	CX
	POP	DS
	POP	BX
;
;	STATUS/CHAR RETURNED IN AX
;
	IRET

DOCOMMAND:
;
;	PROCESS NORMAL MODE COMMAND
;
;
 	CMP	LINEBUFFER,1BH
	JNE	NOTCOMMAND		; DATA IN NORMAL MODE - IGNORE

;
;	IF ECHO ENABLED, ECHO IT
;
;	ECHO IF ENABLED
;
	CMP	ECHOFLAG,1
	JNE	NO_ECHO

	MOV	SI,OFFSET LINEBUFFER

ECHOLOOP:

	LODSB
	CMP	AL,1BH
	JNE	NORMECHO

	MOV	AL,'*'
	CALL	PUTCHAR

	MOV	AL,20H			; ECHO ESC AS '* '

NORMECHO:

	CALL	PUTCHAR
	CMP	AL,0DH
	JNE	ECHOLOOP

	PUSH	AX
	MOV	AL,0AH
	CALL	PUTCHAR
	POP	AX
;
NO_ECHO:

 	MOV	AL,LINEBUFFER+1
	AND	AL,0FFH-20H		; MASK TO LOWER CASE

	CMP	AL,'E'
	JE	ECHOCMD

	CMP	AL,'I'
	JE	SETCALLCMD

	CMP	AL,'J'
	JE	HOST??

	CMP	AL,'P'
	JNE	IGNORECOMMAND		; IGNORE OTHERS
;
;	PARAMS COMMAND - RETURN FIXED STRING
;
	MOV	AL,LINEBUFFER+2
	MOV	PARAMPORT,AL

	MOV	SI,OFFSET PARAMREPLY
	MOV	CX,LPARAMREPLY

PARAMLOOP:

	LODSB
	CALL	PUTCHAR

	LOOP	PARAMLOOP

	JMP	SHORT ENDCOMMAND

HOST??:

;	MOV	AL,LINEBUFFER+6
;	CMP	AL,0DH
;	JNE	HOST???

;	MOV	AL,0			; DEFAULT TO OFF
;HOST???:
;	AND	AL,1
;	MOV	MODE,AL			; SET HOST MODE (PROBABLY OFF)
;
;
	MOV	MODE,1			; ENTER HOST MODE
;
;	ENABLE SWITCH HOST MODE INTERFACE
;
	MOV	DL,APPLMASK
	CALL	SETAPPLMASK

	JMP	SHORT ENDCOMMAND

ECHOCMD:

	MOV	AL,LINEBUFFER+2
	AND	AL,1
	MOV	ECHOFLAG,AL

SETCALLCMD:
ENDCOMMAND:
NOTCOMMAND:
IGNORECOMMAND:

	MOV	CURSOR,OFFSET LINEBUFFER
	RET


ATCOMMAND:

	CMP	LINEBUFFER+1,'B'
	JE	BUFFSTAT

	CMP	LINEBUFFER+1,'M'
	JE	BUFFMIN

	CMP	LINEBUFFER+1,'S'
	JE	SETMASK

	JMP	BADCMD

SETMASK:

	MOV	SI,CURSOR
	MOV	WORD PTR [SI],2020H	; SPACE ON END

	MOV	SI,OFFSET LINEBUFFER+2
	CALL	GETVALUE
	JC	MASKBAD

	MOV	AX,NEWVALUE
	MOV	MMASK,AX

	CALL	GETVALUE
	JC	MASKBAD

	MOV	AX,NEWVALUE
	MOV	MCOM,AL

MASKBAD:

	PUSH	MMASK
	JMP SHORT BUFFCOMM

BUFFMIN:

	PUSH	MINBUFFCOUNT
	JMP SHORT BUFFCOMM
;
BUFFSTAT:

	PUSH	QCOUNT

BUFFCOMM:

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

	MOV	AL,1
	CALL	PUTCHAR
;
;	GET BUFFER COUNT
;
	POP	AX
	CALL	CONV_5DIGITS

	MOV	AL,0
	CALL	PUTCHAR

	RET

ICMD:

	MOV	SI,OFFSET ICMDREPLY
	MOV	CX,LICMDREPLY

	JMP	SENDCMDREPLY

PROCESSHOSTPACKET:

	MOV	AL,MSGCHANNEL
	CALL	GETCCT			; POINT BX TO CORRECT ENTRY

	CMP	MSGTYPE,0
	JNE	NOTDATA

	JMP	HOSTDATAPACKET

HOSTCMDS:
	DW	'G','I', 'J', 'C', 'D', 'L', '@',      'Y', 'M'
	DW	POLL,ICMD,JCMD,CCMD,DCMD,LCMD,ATCOMMAND,YCMD,HOSTMON

NOTDATA:

	MOV	AL,LINEBUFFER
	AND	AL,0FFH-20H		; MASK TO UPPER CASE
	CBW
	MOV	DI,OFFSET HOSTCMDS
	MOV	CX,9
	REPNE	SCASW
	JNE	DUFFHOSTCMD

	JMP	16[DI]


HOSTMON:

	CMP	LINEBUFFER+1,'N'
	JE	MONITOROFF

	JMP	ENABLEMONITOR

MONITOROFF:

	JMP	DISABLEMONITOR

DUFFHOSTCMD:

	MOV	SI,OFFSET LINEBUFFER
	MOV	CX,MSGLENGTH

DIAG00:

	LODSB
;	CALL	PRINTIT

	LOOP	DIAG00

BADCMD:

	MOV	SI,OFFSET BADCMDREPLY
	MOV	CX,LBADCMDREPLY

SENDCMDREPLY:

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

CMDREPLYLOOP:

	LODSB
	CALL	PUTCHAR

	LOOP	CMDREPLYLOOP

	RET

CCMD:
;
;	CONNECT REQUEST - WE ONLY ALLOW CONNECT TO SWITCH, BUT DONT CHECK
;
	CMP	MSGCHANNEL,0
	JE	JCOMM00			; SETTING UNPROTO ADDR - JUST ACK IT

	CMP	MSGLENGTH,1
	JNE	REALCALL
;
;	STATUS REQUEST - IF CONNECTED, GET CALL
;
	MOV	DI,OFFSET CHECKCALL	; FOR ANY RETURNED DATA
	MOV	BYTE PTR [DI],0		; FIDDLE FOR CONNECTED CHECK

	MOV	AH,8
	MOV	AL,MSGCHANNEL		; GET STATUS, AND CALL IF ANY
	CALL	NODE
;
	CMP	CHECKCALL,0
	JE	NOTCONNECTED

	MOV	SI,OFFSET SWITCH
	MOV	CX,LSWITCH

	JMP SHORT SENDCMDREPLY

NOTCONNECTED:

	MOV	SI,OFFSET NOTCONMSG
	MOV	CX,LNOTCON

	JMP SHORT SENDCMDREPLY

REALCALL:

	MOV	CX,1			; CONNECT
	JMP SHORT CDCOMMAND

DCMD:
;
;	DISCONNECT REQUEST
;
	MOV	CX,2			; DISCONNECT
CDCOMMAND:
	MOV	AL,MSGCHANNEL
	MOV	AH,6
	CALL	NODE
;
;	CONNECT/DISCONNECT WILL BE REPORTED VIA NORMAL STATUS CHANGE
;
	JMP	SENDHOSTOK

JCMD:

	MOV	AL,LINEBUFFER+5
	AND	AL,1
	MOV	MODE,AL			; SET HOST MODE (PROBABLY OFF)
;
	JNZ	JCOMM00			; HOST ON??
;
;	DISABLE SWITCH HOST MODE INTERFACE
;
	MOV	DL,0			; APPL MASK
	CALL	SETAPPLMASK
JCOMM00:
	JMP	SENDHOSTOK

LCMD:

	CALL	CHECKTXQUEUE		; SEE IF ANYTHING TO SEND ON THIS CHAN

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

	MOV	AL,1
	CALL	PUTCHAR
;
;	GET STATE AND QUEUED BUFFERS
;
	MOV	AL,MSGCHANNEL
	OR	AL,AL
	JNZ	NORM_L
;
;	TO MONITOR CHANNEL ??
;
	MOV	DH,30H			; RETURN TWO ZEROS
	JMP SHORT MON_L

NORM_L:

	PUSH	BX			; SAVE CCT

	MOV	AH,4			; SEE IF CONNECTED
	CALL	NODE
;
REST_L:

	MOV	DH,'0'			; AX.25 STATE
	OR	CX,CX
	JZ	NOTCON

	MOV	DH,'4'			; CONNECTED

NOTCON:

	MOV	AL,DL			; STATUS MSGS ON RX Q
	CALL	CONV_DIGITS

	MOV	AL,20H
	CALL	PUTCHAR
;
;	GET OTHER QUEUE COUNTS
;
	PUSH	DX

	MOV	AH,7
	MOV	AL,MSGCHANNEL
	CALL	NODE

	POP	DX

	MOV	AL,BL			; DATA ON RX Q
	CALL	CONV_DIGITS

	MOV	AL,20H
	CALL	PUTCHAR
;
;	NOT SENT IS NUMBER ON OUR QUEUE, NOT ACKED NUMBER FROM SWITCH
;

	POP	BX			; CCT
;
;	SEE HOW MANY BUFFERS ATTACHED TO Q HEADER IN BX
;
 	XOR	AL,AL

COUNT_Q_LOOP:

	CMP	WORD PTR [BX],0
	JE	COUNT_RET

	INC	AL
	MOV	BX,[BX]			; FOLLOW CHAIN
	JMP	COUNT_Q_LOOP

COUNT_RET:

	OR	AL,AL
	JZ	LCOMM05			; NOT BUSY

	MOV	DH,'8'			; BUSY

LCOMM05:

	CALL	CONV_DIGITS		; FRAMES NOT SENT (MAY BE > 10)
;
	MOV	AL,20H
	CALL	PUTCHAR

	MOV	AL,CL
	ADD	AL,30H
	CALL	PUTCHAR			; NOT ACKED

	MOV	AL,20H
	CALL	PUTCHAR

MON_L:

	MOV	AL,30H			; TRIES
	CALL	PUTCHAR

	MOV	AL,20H
	CALL	PUTCHAR

	MOV	AL,DH			; STATE
	CALL	PUTCHAR

	MOV	AL,0
	CALL	PUTCHAR

	RET

HOSTDATAPACKET:

	MOV	SI,OFFSET LINEBUFFER
	MOV	CX,MSGLENGTH
;
;	IF WE ALREADY HAVE DATA QUEUED, ADD THIS IT QUEUE
;
	CMP	CHAN_TXQ[BX],0
	JE	NOTHINGQUEUED
;
;	COPY MESSAGE TO A CHAIN OF BUFFERS
;
	CMP	QCOUNT,10
	JB	CANTSEND		; NO SPACE - RETURN ERROR (?)

QUEUEFRAME:

	CALL	COPYMSGTOBUFFERS	; RETURNS DI = FIRST (OR ONLY) FRAGMENT

	LEA	SI,CHAN_TXQ[BX]
	CALL	Q_ADD

	JMP	SENDHOSTOK

NOTHINGQUEUED:
;
;	MAKE SURE NODE ISNT BUSY
;
	cmp	MSGCHANNEL,0		; UNPROTO Channel
	je	SendUnproto

	PUSH	SI
	PUSH	CX
	PUSH	BX

	MOV	AH,7
	MOV	AL,MSGCHANNEL
	CALL	NODE

	POP	BX
	CMP	CX,4
	JA	MUSTQUEUEIT		; ALREADY BUSY

	CMP	DX,40
	JB	MUSTQUEUEIT
;
;	OK TO PASS TO NODE
;
	POP	CX
	POP	SI
	
	MOV	AH,2			; SEND DATA

	MOV	AL,MSGCHANNEL
	CALL	NODE
;
SENDHOSTOK_J:

	JMP	SENDHOSTOK

SendUnproto:

	MOV	AH,2			; SEND DATA
	MOV	AL,0			; unproto to all ports
	CALL	INTVAL

	JMP	SENDHOSTOK

MUSTQUEUEIT:

	POP	CX
	POP	SI

	JMP	QUEUEFRAME

CANTSEND:


	MOV	SI,OFFSET DATABUSYMSG
	MOV	CX,LDATABUSY

	JMP	SENDCMDREPLY

POLL:
	CALL	CHECKTXQUEUE		; SEE IF ANYTHING TO SEND
	CALL	PROCESSPOLL
	RET

YCMD:
SENDHOSTOK:

	MOV	AL,MSGCHANNEL		; REPLY ON SAME CHANNEL
	CALL	PUTCHAR

	MOV	AL,0
	CALL	PUTCHAR			; NOTHING DOING

	RET

PROCESSPOLL:
;
;	ASK SWITCH FOR STATUS CHANGE OR ANY RECEIVED DATA
;
	MOV	CL,0			; POLL TYPE

	CMP	MSGLENGTH,1
	JE	GENERALPOLL
;
;	HE'S BEING AWKWARD, AND USING SPECIFIC DATA/STATUS POLL
;
	CMP	LINEBUFFER+1,'0'
	JE	DATAONLY

	CALL	STATUSPOLL
	JMP SHORT POLLEND

GENERALPOLL:

	CALL	STATUSPOLL
	JNZ	POLLRET

DATAONLY:
	CALL	DATAPOLL

POLLEND:
	JNZ	POLLRET			; GOT DATA

	CALL	SENDHOSTOK		; NOTHING DOING

POLLRET:
	RET

DATAPOLL:

	MOV	DI,OFFSET NODEBUFFER	; FOR ANY RETURNED DATA
	MOV	AH,3
	MOV	AL,MSGCHANNEL
	OR	AL,AL
	JNZ	NOTMONITOR
;
	CMP	FRIENDLY,0
	JE	SKIPDV

	PUSH	AX
	MOV	AX,1000H
	INT	15H

	MOV	AX,1680H
	INT	2FH			; WINDOWS RELEASE TIMESLICE

	POP	AX

	STI				; KEEP SYSTEM FRIENDLY!

SKIPDV:
;
;	POLL FOR MONITOR DATA
;
	CMP	MONFLAG,0
	JE	NOMONITOR
;
;	HAVE ALREADY GOT DATA PART OF MON FRAME OT SEND
;
	MOV	MONFLAG,0

	MOV	SI,OFFSET MONBUFFER
	MOV	CX,MONLENGTH

	JMP	SENDCMDREPLY


NOMONITOR:
;
;	SEE IF ANYTHING TO MONITOR
;
	MOV	AL,1			; FIRST PORT
	MOV	AH,11
	MOV	DI,OFFSET MONITORDATA
	CALL	NODE

	CMP	CX,0
	JE	DATAPOLLRET

	MOV	DI,OFFSET MONITORDATA
	CALL	DISPLAYFRAME		; DISPLAY TRACE FRAME

	CMP	MONCURSOR,OFFSET MONHEADER+1
	JE	DATAPOLLRET		; NOTHING DOING

	MOV	DI,MONCURSOR
	MOV	BYTE PTR [DI],0		; NULL TERMINATOR

	MOV	CX,DI
	SUB	CX,OFFSET MONHEADER-1	; LENGTH

	MOV	SI,OFFSET MONHEADER

	CALL	SENDCMDREPLY

	OR	AL,1			; HAVE SEND SOMETHING
	RET

NOTMONITOR:

	CALL	NODE
;
	CMP	CX,0
	JE	DATAPOLLRET
;
;	SEND DATA
;
	MOV	AL,MSGCHANNEL
	CALL	PUTCHAR

	MOV	AL,7
	CALL	PUTCHAR

	DEC	CX
	MOV	AL,CL
	CALL	PUTCHAR			; LENGTH-1
	INC	CX

	MOV	SI,OFFSET NODEBUFFER

SENDDATLOOP:

 	LODSB
	CALL	PUTCHAR

	LOOP	SENDDATLOOP

DATAPOLLRET:

	RET

STATUSPOLL:

	MOV	AH,4
	MOV	AL,MSGCHANNEL
	CMP	AL,0
	JE	NOSTATECHANGE		; ?? Channel Zero For Unproto ??

	CALL	NODE
;
	CMP	DX,0
	JE	NOSTATECHANGE
;
;	PORT HAS CONNECTED OR DISCONNECTED - SEND STATUS CHANGE TO PC
;
	PUSH	CX			; SAVE

	MOV	AH,5
	MOV	AL,MSGCHANNEL

	CALL	NODE			; ACK THE STATUS CHANGE

	POP	CX
	CMP	CX,0
	JNE	SENDHOSTCON
;
;	DISCONNECTED
;
	MOV	SI,OFFSET DISCMSG
	MOV	CX,LDISC

	JMP SHORT STATUSPOLLEND

SENDHOSTCON:
;
;	GET CALLSIGN
;
	MOV	DI,OFFSET CONCALL	; FOR ANY RETURNED DATA
	MOV	AH,8
	MOV	AL,MSGCHANNEL		; GET STATUS, AND CALL IF ANY
	CALL	NODE
;
;	IF IN CLUSTER MODE, DONT ALLOW DUPLICATE CONNECTS
;
	CMP	DEDMODE,'C'
	JNE	DONTCHECK

	MOV	AL,1
	MOV	CX,HOSTSTREAMS

CHECKLOOP:

	PUSH	CX
	PUSH	AX

	CMP	AL,MSGCHANNEL
	JE	CHECKNEXT		; DONT CHECK OUR STREAM!

	MOV	DI,OFFSET CHECKCALL	; FOR ANY RETURNED DATA
	MOV	AH,8
	CALL	NODE
;
	CMP	AH,0
	JE	CHECKNEXT		; NOT CONNECTED (AH HAS SESSION FLAGS)

	MOV	SI,OFFSET CHECKCALL
	MOV	DI,OFFSET CONCALL
	MOV	CX,10
	REP CMPSB

	JNE	CHECKNEXT
;
;	ALREADY CONNECTED - KILL NEW SESSION
;
	MOV	CX,ALREADYLEN
	MOV	SI,OFFSET ALREADYCONMSG
	
	MOV	AH,2			; SEND DATA
	MOV	AL,MSGCHANNEL
	CALL	NODE
;
	MOV	CX,2			; DISCONNECT

	MOV	AL,MSGCHANNEL
	MOV	AH,6
	CALL	NODE
;
	MOV	AH,5
	MOV	AL,MSGCHANNEL
	CALL	NODE			; ACK THE STATUS CHANGE

	POP	AX
	POP	CX

	XOR	AL,AL			; SET Z

NOSTATECHANGE:

	RET


CHECKNEXT:

	POP	AX
	POP	CX

	INC	AL
	LOOP	CHECKLOOP

DONTCHECK:

	MOV	SI,OFFSET CONSWITCH
	MOV	CX,LCONSWITCH

STATUSPOLLEND:

	CALL	SENDCMDREPLY
	OR	AL,1			; SET NZ
	RET

CHECKTXQUEUE:
;
;	IF ANYTHING TO SEND, AND NOT BUSY, SEND IT
;
	CMP	CHAN_TXQ[BX],0
	JNZ	SOMETHINGONQUEUE

	RET

SOMETHINGONQUEUE:
;
;	MAKE SURE NODE ISNT BUSY
;
	PUSH	BX			; SAVE CCT

	MOV	AH,7
	MOV	AL,MSGCHANNEL
	CALL	NODE

	POP	BX
	CMP	CX,4
	JA	STILLBUSY		; ALREADY BUSY

	CMP	DX,25
	JB	STILLBUSY
;
;	OK TO PASS TO NODE
;
	LEA	SI,CHAN_TXQ[BX]
	CALL	Q_REM

	PUSH	BX

	MOV	SI,DI			; BUFFER CHAIN
	MOV	DI,OFFSET WORKAREA

	MOV	CX,2[SI]		; LENGTH
	PUSH	CX
	CALL	COPYBUFFERSTOSTRING	; UNPACK CHAIN
	POP	CX

	MOV	SI,OFFSET WORKAREA

	MOV	AH,2			; SEND DATA
	MOV	AL,MSGCHANNEL
	CALL	NODE
;
	POP	BX

STILLBUSY:

	RET





GETCCT:

	MUL	CCTLEN
	MOV	BX,OFFSET CCT
	ADD	BX,AX

	RET


SETAPPLMASK:

	MOV	CX,HOSTSTREAMS		; STREAMS SUPPORTED
	MOV	AH,1			; SET APPL MASK
	MOV	AL,1			; FIRST PORT

ENABLEHOST:

	PUSH	AX
	PUSH	CX

	MOV	CL,APPLFLAGS
	CALL	NODE

	POP	CX
	POP	AX
	INC	AL			; NEXT PORT

	LOOP	ENABLEHOST

	RET


TENK	DW	10000
	DW	1000
	DW	100
WORD10	DW	10
;
CONV_DIGITS:

	MOV	AH,0

CONV_5DIGITS:

	PUSH	DX
	PUSH	BX

	MOV	BX,OFFSET TENK		; 10000

	CMP	AX,10
	JB	UNITS			; SHORT CUT AND TO STOP LOOP

START_LOOP:

	cmp	AX,WORD PTR [BX]
	JAE	STARTCONV

	ADD	BX,2

	JMP SHORT START_LOOP
;
STARTCONV:

	MOV	DX,0
	DIV	WORD PTR [BX]		; 
	ADD	AL,30H			; MUST BE LESS THAN 10
	CALL	PUTCHAR
;
	MOV	AX,DX			; REMAINDER

	ADD	BX,2
	CMP	BX,OFFSET WORD10+2	; JUST DIVIDED BY 10?
	JNE	STARTCONV		; NO, SO ANOTHER TO DO
;
;	REST MUST BE UNITS
;
UNITS:

	add	AL,30H
	CALL	PUTCHAR

	POP	BX
	POP	DX

	ret

NODE:

	ADD	AL,STREAMBASE
INTVAL:
	INT	7FH

	RET

PUTCHARX:

	CALL	PRINTIT

PUTCHAR:

	PUSH	BX

	MOV	BX,PUTPTR
	MOV	[BX],AL
	INC	BX
	CMP	BX,OFFSET ENDRXBUFFER
	JNE	PUTRET

	MOV	BX,OFFSET RXBUFFER
PUTRET:
	MOV	PUTPTR,BX

	INC	RXCOUNT

;	PUSH	AX			; TESTING
;	CALL	PRINTIT
;	POP	AX

	POP	BX
	RET


RELBUFF:

	MOV	SI,OFFSET FREE_Q
	CALL	Q_ADDF			; RETURN BUFFER TO FREE QUEUE
	RET

COPYMSGTOBUFFERS:
;
;	COPIES MESSAGE IN SI, LENGTH CX TO A CHAIN OF BUFFERS
;
;	RETURNS DI = FIRST (OR ONLY) FRAGMENT
;
	MOV	BP,SP			; SAVE IN CASE ERROR

	PUSH	SI			; SAVE MSG

	CALL	GETBUFF			; GET FIRST
	JZ	BUFFERERROR		; NONE - SHOULD NEVER HAPPEN

	POP	SI			; RECOVER DATA

	MOV	DX,DI			; SAVE FIRST BUFFER

	MOV	2[DI],CX		; SAVE LENGTH
	ADD	DI,4

	CMP	CX,BUFFLEN-6		; MAX DATA IN FIRST
	JA	NEEDCHAIN
;
;	IT WILL ALL FIT IN ONE BUFFER
;
COPYLASTBIT:

	REP MOVSB

	MOV	DI,DX			; FIRST BUFFER
	RET

NEEDCHAIN:

	PUSH	CX
	MOV	CX,BUFFLEN-6
	REP MOVSB			; COPY FIRST CHUNK
	POP	CX
	SUB	CX,BUFFLEN-6
;
;	DI NOW POINTS TO CHAIN WORD OF BUFFER
;
COPYMSGLOOP:
;
;	GET ANOTHER BUFFER
;
	PUSH	SI			; MESSAGE 
	PUSH	DI			; FIRST BUFFER CHAIN WORD

	CALL	GETBUFF
	JZ	BUFFERERROR		; NONE - SHOULD NEVER HAPPEN

	POP	SI			; PREVIOUS BUFFER
	MOV	[SI],DI			; CHAIN NEW BUFFER

	POP	SI			; MESSAGE

	CMP	CX,BUFFLEN-2		; MAX DATA IN REST
	JBE	COPYLASTBIT

	PUSH	CX
	MOV	CX,BUFFLEN-2
	REP MOVSB			; COPY FIRST CHUNK
	POP	CX
	SUB	CX,BUFFLEN-2
;
	JMP	COPYMSGLOOP


BUFFERERROR:

	MOV	SP,BP			; RESET STACK
	MOV	DI,0			; INDICATE NO BUFFER
	RET

COPYBUFFERSTOSTRING:
;
;	UNPACKS CHAIN OF BUFFERS IN SI TO DI
;
;
	MOV	CX,2[SI]		; LENGTH

	MOV	DX,SI

	ADD	SI,4

	CMP	CX,BUFFLEN-6
	JA	MORETHANONE
;
;	ITS ALL IN ONE BUFFER
;
	REP MOVSB

	MOV	DI,DX			; BUFFER
	CALL	RELBUFF

	RET

MORETHANONE:

	PUSH	CX
	MOV	CX,BUFFLEN-6
	REP MOVSB

	POP	CX
	SUB	CX,BUFFLEN-6

UNCHAINLOOP:

	PUSH	DI			; SAVE TARGET

	MOV	DI,DX			; OLD BUFFER

	MOV	DX,[SI]			; NEXT BUFFER

	CALL	RELBUFF

	POP	DI
	MOV	SI,DX

	CMP	CX,BUFFLEN-2
	JBE	LASTONE


	PUSH	CX
	MOV	CX,BUFFLEN-2
	REP MOVSB
	POP	CX
	SUB	CX,BUFFLEN-2


	JMP	UNCHAINLOOP


LASTONE:

	REP MOVSB

	MOV	DI,DX
	CALL	RELBUFF

	RET




Q_REM:
	MOV	DI,[SI]			; GET ADDR OF FIRST BUFFER
	CMP	DI,0
	JE	Q_RET			; EMPTY

	MOV	AX,[DI]			; CHAIN FROM BUFFER
	MOV	[SI],AX			; STORE IN HEADER

	POP	[DI]			; CALLERS ADDR 
	PUSH	[DI]
Q_RET:
	RET

;               
Q_ADD:
Q_ADD05:
	CMP	WORD PTR [SI],0		; END OF CHAIN
	JE	Q_ADD10

	MOV	SI,[SI]			; NEXT IN CHAIN
	JMP	Q_ADD05
Q_ADD10:
	MOV	WORD PTR [DI],0		; CLEAR CHAIN ON NEW BUFFER
	MOV	[SI],DI			; CHAIN ON NEW BUFFER

	RET

;
;	ADD TO FRONT OF QUEUE - MUST ONLY BE USED FOR FREE QUEUE
;
Q_ADDF:

	MOV	AX,[SI]			; OLD FIRST IN CHAIN
	MOV	[DI],AX
	MOV	[SI],DI			; PUT NEW ON FRONT

	INC	QCOUNT
	RET

GETBUFF:

	MOV	SI,OFFSET FREE_Q
	CALL	Q_REM
;
	JZ	NOBUFFS

	DEC	QCOUNT
	MOV	AX,QCOUNT
	CMP	AX,MINBUFFCOUNT
	JA	GETBUFFRET
	MOV	MINBUFFCOUNT,AX

GETBUFFRET:

	OR	AL,1			; SET NZ
 	RET

NOBUFFS:

	INC	NOBUFFCOUNT
	XOR	AL,AL
	RET
;
;	MONITOR SUPPORT CODE
;
ENABLEMONITOR:

	MOV	CL,APPLFLAGS
	OR	CL,80H			; REQUEST MONITORING
	JMP SHORT MONCOM

DISABLEMONITOR:

	MOV	CL,APPLFLAGS		; CANCEL MONITORING

MONCOM:

	MOV	DL,APPLMASK
	MOV	AL,1			; FIRST PORT
	MOV	AH,1			; SET APPL MASK

	CALL	NODE

	JMP	SENDHOSTOK

;
;	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT
;
MESSAGE	STRUC

MSGCHAIN	DW	?		; CHAIN WORD
MSGPORT		DB	?		; PORT 
MMSGLENGTH	DW	?		; LENGTH

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
PORT_MSG	DB	' P=',NULL
NODES_MSG	DB	' NODES broadcast from ',NULL
NETROM_MSG	DB	' L3: ',NULL
TO_MSG		DB	'>',NULL
TTL_MSG		DB	' t=',NULL
AT_MSG		DB	' at ',NULL
VIA_MSG		DB	' via ',NULL
QUALITY_MSG	DB	' qlty=',NULL

MYCCT_MSG	DB	' my'
CCT_MSG		DB	' c=',0
TIM_MSG		DB	' t/o=',0

WINDOW		DB	' w=',0
CONN_REQ_MSG	DB	' <CON REQ>',NULL
CONN_ACK_MSG	DB	' <CON ACK>',NULL
CONN_NAK_MSG	DB	' <CON NAK> - BUSY',NULL
DISC_REQ_MSG	DB	' <DISC REQ>',NULL
DISC_ACK_MSG	DB	' <DISC ACK>',NULL
INFO_MSG	DB	' <I S',NULL
INFO_ACK_MSG	DB	' <I ACK R',NULL
DUFF_NET_MSG	DB	' <????>',NULL
IP_MSG		DB	' <TCP/IP>',NULL
 
UA_MSG		DB	'UA',NULL
DM_MSG		DB	'DM',NULL
RR_MSG		DB	'RR',NULL
RNR_MSG		DB	'RNR',NULL
UI_MSG		DB	'UI',NULL
FRMR_MSG	DB	'FRMR',NULL
REJ_MSG		DB	'REJ',NULL

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

MCOM		DB	1
MALL		DB	1
HEADERLN	DB	1
MMASK		DW	0FFFFH
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


MONPUTCHAR:

	PUSH	DI
	MOV	DI,MONCURSOR
	STOSB
	MOV	MONCURSOR,DI
	POP	DI

	RET

DISPLAYFRAMETX:
DISPLAYFRAME:

	MOV	MONHEADER,4		; NO DATA FOLLOWS
	MOV	MONCURSOR,OFFSET MONHEADER+1
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
;
;	CHECK FOR PORT SELECTIVE MONITORING
;
	MOV	CL,MSGPORT[DI]
	mov	PORT_NO,CL

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
;       Display Origin Callsign                                               ;
;
	MOV	AL,'0'
	CALL	MONPUTCHAR
	MOV	AL,':'
	CALL	MONPUTCHAR

	lea	SI,MSGORIGIN[DI]
	call	CONVFROMAX25
	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	pop	DI
	push	DI

	mov	AL,'>'
	call	MONPUTCHAR

;
;       Display Destination Callsign                                          ;
;

	lea	SI,MSGDEST[DI]
	call	CONVFROMAX25
	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	pop	DI
	push	DI

	mov	AX,MMSGLENGTH[DI]
	mov	FRAME_LENGTH,AX
	mov	CX,8			; Max number of digi-peaters

;
;       Display any Digi-Peaters                                              ;
;

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
	call	MONPUTCHAR

	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	pop	AX

	test	AH,80H
	jz	NOT_REPEATED

	mov	AL,'*'
	call	MONPUTCHAR

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
	call	MONPUTCHAR

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
	call	MONPUTCHAR

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
	call	MONPUTCHAR
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
	jmp	SHORT END_OF_TYPE

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
	call	MONPUTCHAR
	jmp	SHORT END_OF_TYPE

;
;       Process all other frame types                                         ;
;

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
	call	MONPUTCHAR
	jmp	SHORT END_OF_TYPE

NOT_SABM_FRAME:
	cmp	AL,DISC
	jne	NOT_DISC_FRAME

	mov	AL,'D'
	call	MONPUTCHAR
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
	call	MONPUTCHAR

END_OF_TYPE:

;----------------------------------------------------------------------------;
;       If Version 2 Then display P/F C/R Information                        ;
;----------------------------------------------------------------------------;

	cmp	VERSION_NO,2
	jne	NOT_VERSION_2

	mov	AL,' '
	call	MONPUTCHAR

	mov	AL,COMM_RESP		; Print Command/Response Flag
	call	MONPUTCHAR

	cmp	POLL_FINAL,0
	je	NO_POLL_FINAL

	mov	AL,' '
	call	MONPUTCHAR

	mov	AL,POLL_FINAL		; Print Poll/Final Flag if Set
	call	MONPUTCHAR

NO_POLL_FINAL:
NOT_VERSION_2:

;----------------------------------------------------------------------------;
;       Display sequence numbers if applicable                               ;
;----------------------------------------------------------------------------;

	cmp	NS,1
	jne	NOT_NS_DATA

	mov	AL,' '
	call	MONPUTCHAR

	mov	AL,'S'
	call	MONPUTCHAR

	mov	AL,FRAME_TYPE
	ror	AL,1

	call	DISPLAYSEQ

NOT_NS_DATA:
	cmp	NR,1
	jne	NOT_NR_DATA

	mov	AL,' '
	call	MONPUTCHAR

	mov	AL,'R'
	call	MONPUTCHAR

	mov	AL,FRAME_TYPE
	rol	AL,1
	rol	AL,1
	rol	AL,1

	call	DISPLAYSEQ

NOT_NR_DATA:
	mov	AL,'>'
	call	MONPUTCHAR

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
	call	MONPUTCHAR

	XOR	AL,AL			; IN CASE EMPTY

	sub	CX,21
	JCXZ	NO_INFO			; EMPTY I FRAME
;
;	PUT DATA IN MONBUFFER, LENGTH IN MONLENGTH
;
	MOV	MONFLAG,1
	MOV	MONHEADER,5		; DATA FOLLOWS

	cmp	CX,257
	jb	LENGTH_OK
;
	mov	CX,256
;
LENGTH_OK:
;
	MOV	MONBUFFER+1,CL
	DEC	MONBUFFER+1

	MOV	MONLENGTH,CX
	ADD	MONLENGTH,2

	MOV	DI,OFFSET MONBUFFER+2

MONCOPY:
	LODSB
	CMP	AL,7			; REMOVE BELL
	JNE	MONC00

	MOV	AL,20H
MONC00:
	STOSB

	LOOP	MONCOPY

	POP	DI
	RET

NO_INFO:
;
;	ADD CR UNLESS DATA ALREADY HAS ONE
;
	CMP	AL,CR
	JE	NOTANOTHER
ADD_CR:
	mov	AL,CR
	call	MONPUTCHAR

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
	call	MONPUTCHAR

	pop	SI
	pop	CX

	add	SI,6			; Point to first destination
	sub	CX,28			; Header, mnemonic and signature length

NODES_LOOP:

	JMP	NO_INFO			; CANT DECODE SENSIBLY

	IF	0

	cmp	CX,0
	jbe	NO_INFO

	push	CX
	push	SI			; Global push for each node

	push	SI			; Local push 

	mov	AL,' '
	call	MONPUTCHAR
	mov	AL,' '
	call	MONPUTCHAR

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

	call	MONPUTCHAR

	loop	MNEMONIC_LOOP

END_MNEMONIC:
	mov	AL,':'
	call	MONPUTCHAR

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
	call	MONPUTCHAR

	pop	SI
	pop	CX
	add	SI,21			; Point to next destination
	sub	CX,21			; Remove length of each 

	jmp	SHORT NODES_LOOP

	ENDIF

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
	call	MONPUTCHAR

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
	call	MONPUTCHAR
	mov	AL,'R'
	call	MONPUTCHAR

	mov	AL,-1[SI]		; Get receive sequence number
	call	DISPLAY_BYTE_1

	mov	AL,'>'
	call	MONPUTCHAR

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
	call	MONPUTCHAR

	CALL	DOL4FLAGS

	JMP SHORT END_NETROM

NOT_L4IACK:

	OR	AL,AL
	JNZ	NOTIP
;
;	TCP/IP DATAGRAM
;
	mov	SI,OFFSET IP_MSG
	call	NORMSTR
;
;	MAY DECODE IT LATER
;
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
	CALL	MONPUTCHAR
L4F010:
	TEST	OPCODE,L4NAK
	JZ	L4F020

	MOV	AL,'N'
	CALL	MONPUTCHAR
L4F020:
	TEST	OPCODE,L4MORE
	JZ	L4F030

	MOV	AL,'M'
	CALL	MONPUTCHAR
L4F030:
	RET

;----------------------------------------------------------------------------;
;       Display ASCIIZ strings                                               ;
;----------------------------------------------------------------------------;

NORMSTR:
	lodsb
	cmp	AL,NULL		; End of String ?
	je	NORMSTR_RET	; Yes
	call	MONPUTCHAR
	jmp	SHORT NORMSTR

NORMSTR_RET:
	ret

;-----------------------------------------------------------------------------;
;       Display sequence numbers                                              ;
;-----------------------------------------------------------------------------;

DISPLAYSEQ:
	and	AL,7
	add	AL,30H
	call	MONPUTCHAR
	ret

;-----------------------------------------------------------------------------;
;       Display Callsign pointed to by SI                                     ;
;-----------------------------------------------------------------------------;

DISPADDR:
	jcxz	DISPADDR_RET

	lodsb
	call	MONPUTCHAR

	loop	DISPADDR

DISPADDR_RET:
	ret


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
	call	MONPUTCHAR
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
	call	MONPUTCHAR
	pop	AX

UNITS_1:
	add	AL,30H
	call	MONPUTCHAR

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
	call	MONPUTCHAR
	pop	AX

UNITS_2:
	add	AL,30H
	call	MONPUTCHAR

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
	call	MONPUTCHAR
	ret

AX25CALL	DB	7 DUP (0)	; WORK AREA FOR AX25 <> NORMAL CALL CONVERSION
NORMCALL	DB	10 DUP (0)	; CALLSIGN IN NORMAL FORMAT
NORMLEN		DW	0		; LENGTH OF CALL IN NORMCALL	



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
	MOV	BYTE PTR [DI],'-'
	INC	DI
	CMP	AL,10
	JB	CONVAX70
	SUB	AL,10
	MOV	BYTE PTR [DI],'1'
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
	mov	al,20h
	call	printit
	POP	AX
	ret
hexout1:
	and	al,0fh
	cmp	al,10
	jl	hexout5
	add	al,7
hexout5:
	add	al,30h
	call	printit
;
	ret
;
printit:
	PUSH	AX
	PUSH	BX
	mov	AH,14
	int	10h
	POP	BX
	POP	AX
	ret
;


NEWVALUE	DW	0
D16		DW	16

GETVALUE:
;
;	EXTRACT NUMBER (HEX OR DECIMAL) FROM INPUT STRING
;
	MOV	NEWVALUE,0

	LODSB
	CMP	AL,'$'			; HEX?
	JE	DECODEHEX
	DEC	SI
VALLOOP:
	LODSB
	CMP	AL,' '
	JE	ENDVALUE
	CMP	AL,0DH
	JE	ENDVALUE
	CMP	AL,','
	JE	ENDVALUE
;
;	ANOTHER DIGIT - MULTIPLY BY 10
;
	MOV	AX,NEWVALUE
	MUL	D10
	MOV	NEWVALUE,AX

	MOV	AL,-1[SI]
	SUB	al,'0'
	JC	DUFFVALUE
	CMP	AL,10
	JNC	DUFFVALUE

	MOV	AH,0
	ADD	NEWVALUE,AX

	JC	DUFFVALUE
	JMP	VALLOOP

DECODEHEX:
HEXLOOP:
	LODSB
	CMP	AL,' '
	JE	ENDVALUE
	CMP	AL,0DH
	JE	ENDVALUE
	CMP	AL,','
	JE	ENDVALUE
;
;	ANOTHER DIGIT - MULTIPLY BY 16
;
	MOV	AX,NEWVALUE
	MUL	D16
	MOV	NEWVALUE,AX

	MOV	AL,-1[SI]
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


BUILDPOOL:
;
;	BUILD BUFFER POOL
;
	MOV	DI,OFFSET BUFFERPOOL

BUFF000:

	MOV	SI,OFFSET FREE_Q

	CALL	Q_ADDF

	ADD	DI,BUFFLEN
	LOOP	BUFF000

	MOV	AX,QCOUNT
	MOV	MINBUFFCOUNT,AX


	MOV	DX,DI			; LAST USED ADDRESS
	INT	27H


BUFFERPOOL	LABEL	BYTE

;
;	REST OVERLAID BY BUFFER POOL  AFTER LOAD
;
MSG	DB	'WA8DED Host Mode Interface for G8BPQ switch version '
	db	VERSIONSTRING,' ',DATESTRING
	db	0dh,0ah,'$'

ERRORMSG	DB	0AH,'Parameter error:',0dh,0ah,0ah
		db	'Format is DEDHOST numberofports startport applmask bufferpoolsize interrupt'
		db	0dh,0ah,'Program NOT installed'
		DB	0dh,0ah,0AH,'$'

NOTBPQERROR	DB	'Switch not found$'
VERSERROR	DB	'Version not compatible with Node Software$'

POOLSIZE	DW	8		; SIZE IN K	


BADCONFIGMSG	DB	'Configuration file read error',0DH,0AH,'$'
CONFIGFILENAME	DB	CONFIGNAME,0
NOCONFIGMSG	DB	'Configuration file ',CONFIGNAME,' not found',0DH,0AH,'$'
;
;	BPQCFG FIRST 128 BYTES
;
CONFIGTABLE	LABEL	BYTE
;
;	CONFIGURATION DATA STRUCTURE
;
;	DEFINES LAYOUT OF CONFIG RECORD PRODUCED BY CONFIGURATION PROG
;
;	LAYOUT MUST MATCH THAT IN CONFIG.C SOURCE
;
C_NODECALL	DB	10 DUP (0)	 ; OFFSET = 0 
C_NODEALIAS	DB	10 DUP (0)	 ; OFFSET = 10
C_BBSCALL	DB	10 DUP (0)	 ; OFFSET = 20
C_BBSALIAS	DB	10 DUP (0)	 ; OFFSET = 30
;
C_OBSINIT	DW	0		 ; OFFSET = 40
C_OBSMIN	DW	0		 ; OFFSET = 42
C_NODESINTERVAL	DW	0		 ; OFFSET = 44
C_L3TIMETOLIVE	DW	0		 ; OFFSET = 46
C_L4RETRIES	DW	0		 ; OFFSET = 48
C_L4TIMEOUT	DW	0		 ; OFFSET = 50
C_BUFFERS	DW	0		 ; OFFSET = 52
C_PACLEN	DW	0		 ; OFFSET = 54
C_TRANSDELAY	DW	0		 ; OFFSET = 56
C_T3		DW	0		 ; OFFSET = 58
   		DW	0		 ; OFFSET = 60
		DW	0		 ; OFFSET = 62
C_IDLETIME	DW	0		 ; OFFSET = 64
C_EMSFLAG	DB	0		 ; OFFSET = 66
		DB	0
C_BBS		DB	0		 ; OFFSET = 68
C_NODE		DB	0		 ; OFFSET = 69
C_HOSTINTERRUPT	DB	0		 ; OFFSET = 70
C_DESQVIEW	DB	0		 ; OFFSET = 71
C_MAXLINKS	DW	0		 ; OFFSET = 72
C_MAXDESTS	DW	0
C_MAXNEIGHBOURS	DW	0
C_MAXCIRCUITS	DW	0		; 78
 
C_TNCPORTLISTO	DB	16 DUP (0)	 ; OFFSET = 80

C_IDINTERVAL	DW	0		; 96
C_XXXXXXXX	DW	0		; 98    ; SPARE (WAS DIGIFLAG)
C_MINQUAL	DW	0		; 100
C_HIDENODES	DB	0		; 102
C_L4DELAY	DW	0		; 103
C_L4WINDOW	DW	0		; 105
C_BTINTERVAL	DW	0		; 106
X_UNPROTO	DB	9 DUP (0)	; 108	; NOW SPARE
C_BBSQUAL	DW	0		; 117
		DB	(CONFIGTABLE+128-$) DUP (0)



CONFIGHANDLE	DW	0

INIT:

	MOV	WORD PTR PATCHDS,CS	; AT INT FF ENTRY POINT

	MOV	DX,OFFSET MSG
	MOV	AH,9
	INT	21H
;
;	GET NODE INTERRUPT NUMBER FROM CONFIG FILE
;
	MOV	DX,OFFSET CONFIGFILENAME
	MOV	AH,3DH
	MOV	AL,0			; READ ONLY
	INT	21H			; OPEN IT

	JC	NOCONFIGFILE

	MOV	CONFIGHANDLE,AX

	MOV	BX,AX
	MOV	DX,OFFSET CONFIGTABLE
	MOV	CX,128
	MOV	AH,3FH

	INT	21H			; READ
	CMP	AX,CX
	JNE	SHORTCONFIG

	JMP SHORT PROCESSCONFIG

NOCONFIGFILE:

	MOV	DX,OFFSET NOCONFIGMSG
	JMP SHORT CONFIGERR

SHORTCONFIG:

	MOV	DX,OFFSET BADCONFIGMSG

CONFIGERR:

	MOV	AH,9
	INT	21H

	MOV	AX,4C01H
	INT	21H			; EXIT

PROCESSCONFIG:

	MOV	BX,CONFIGHANDLE
	MOV	AH,3EH
	INT	21H			; CLOSE IT

	MOV	AL,C_HOSTINTERRUPT	; INTERRUPT
	MOV	BYTE PTR INTVAL+1,AL	; PATCH NODE CALL INSTRUCTION
;
;	GET NODE VERSION
;
	PUSH	DS
	MOV	AH,0
	ADD	AX,AX
	ADD	AX,AX
	MOV	SI,AX

	XOR	AX,AX
	MOV	DS,AX
;
;	DS:SI POINTS TO VECTOR
;
	CMP	WORD PTR DS:2[SI],0
	POP	DS

	JE	NOSWITCH

	MOV	AH,0
	CALL	NODE			; CHECK IF LOADED AND GET VERSIONS

	CMP	AX,'PB'			; NOT BPQ
	JNE	NOSWITCH

	CMP	BX,' Q'
	JE	SWITCHOK

NOSWITCH:

	MOV	DX,OFFSET NOTBPQERROR

	MOV	AH,9
	INT	21H

	JMP SHORT EXIT

SWITCHOK:
;
;	DONT CHECK VERSION - ANYTHING VAGUELY CURRENT SHOULD DO
;
;	CMP	BYTE PTR DS:[SI],MAJORVERSION
;	JNE	BADVERSION
;
;	CMP	BYTE PTR DS:1[SI],MINORVERSION

	JMP	VERSIONOK


BADVERSION:

	MOV	DX,OFFSET VERSERROR
	MOV	AH,9
	INT	21H

EXIT:
	MOV	AX,4C00H
	INT	21H

BADPORTVAL:

	MOV	AH,9
	MOV	DX,OFFSET ERRORMSG
	INT	21H

	RET

VERSIONOK:
;
;	COPY NODE CALLSIGN TO I COMMAND AREA
; 
	MOV	SI,OFFSET C_NODECALL
	MOV	DI,OFFSET ICMDREPLY+1
	MOV	CX,9
	REP MOVSB

 	MOV	SI,82H			; PARAM STRING

	CALL	GETVALUE		; NUMBER OF PORTS
	JC	BADPORTVAL

	MOV	AL,BYTE PTR NEWVALUE
	OR	AL,AL
	JZ	BADPORTVAL

	CMP	AL,32
	JA	BADPORTVAL

	MOV	AL,BYTE PTR NEWVALUE
	MOV	BYTE PTR HOSTSTREAMS,AL

	CALL	GETVALUE
	JC	BADPORTVAL		; FIRST STREAM NUMBER 

	MOV	AL,BYTE PTR NEWVALUE
	OR	AL,AL
	JZ	BADPORTVAL

	CMP	AL,64
	JA	BADPORTVAL

	ADD	AL,BYTE PTR HOSTSTREAMS
	CMP	AL,65
	JA	BADPORTVAL

	MOV	AL,BYTE PTR NEWVALUE
	DEC	AL
	MOV	STREAMBASE,AL

	CALL	GETVALUE		; APPL MASK
	JC	BADPORTVAL

	MOV	AL,BYTE PTR NEWVALUE
	MOV	APPLMASK,AL

	CALL	GETVALUE		; BUFFER POOL

BADPORTVAL_1:

	JC	BADPORTVAL

	MOV	AL,BYTE PTR NEWVALUE
	CMP	AL,8
	JB	LEAVESIZE

	CMP	AL,60
	JA	BADPORTVAL

	MOV	BYTE PTR POOLSIZE,AL

	MOV	AX,POOLSIZE
	MOV	CL,10			; * 1024
	SHL	AX,CL

	ADD	AX,OFFSET BUFFERPOOL
	JNC	LEAVESIZE

	MOV	AX,0
	SUB	AX,OFFSET BUFFERPOOL

	MOV	CL,10			; / 1024
	SHR	AX,CL

	MOV	POOLSIZE,AX

LEAVESIZE:

	CALL	GETVALUE		; INTERRUPT
	JC	BADPORTVAL_1

	MOV	AL,BYTE PTR NEWVALUE

	MOV	DX,OFFSET INTFF
	MOV	AH,25H
	INT	21H			; SET VECTOR
;
;	CHECK FOR 6TH PARAM
;
	CMP	BYTE PTR [SI],'N'
	JE	NOTIMER

	CMP	BYTE PTR [SI],'n'
	JNE	LEAVETIMER

NOTIMER:

	ADD 	SI,2
	AND	APPLFLAGS,NOT 40H

LEAVETIMER:
;
;	CHECK FOR DV PARAM
;
	CMP	BYTE PTR [SI],'/'
	JNE	NO_NODVPARM

	CMP	WORD PTR 1[SI],'on'
	JE	NODV

	CMP	WORD PTR 1[SI],'ON'
	JNE	NO_NODVPARM


NODV:

	MOV	FRIENDLY,0

NO_NODVPARM:
;
;	ALLOCATE ALL STREAMS
;
	MOV	AL,1			; ALLOCATE STREAMS 1-N
	MOV	CX,HOSTSTREAMS

ALLOCLOOP:

	PUSH	CX
	PUSH	AX

;	AH = 13 Allocate/deallocate stream
;		If AL=0, return first free stream and allocate
;		If AL>0, CL=1, Allocate stream. If aleady allocated,
;		   return CX nonzero, else allocate, and return CX=0
;		If AL>0, CL=2, Release stream
;
	MOV	AH,13
	MOV	CL,1

	CALL	NODE
;
	POP	AX
	POP	CX

	INC	AL
	LOOP	ALLOCLOOP
;
;	CONVERT BUFFER POOL SIZE FROM KB TO BUFFERS
;
	MOV	AX,POOLSIZE
	MOV	CX,5			; * 32
	SHL	AX,CL

	MOV	CX,AX
	JMP	BUILDPOOL

CODE	ENDS
;               
        END     GO

