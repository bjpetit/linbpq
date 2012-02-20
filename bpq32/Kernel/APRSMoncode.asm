	PAGE    56,132
;

.386
;
;  SEGMENT definitions and order
;

;	 July 2008
;		Add basic IP decoding

;	Feb 2012
;		Created from MONDECODE to provide a transparent decode for MIC-E frames 


;*	32 Bit code
_TEXT		SEGMENT DWORD USE32 PUBLIC 'CODE'
_TEXT		ENDS

;*	Contains 32 Bit data
_BPQDATA		SEGMENT DWORD PUBLIC 'DATA'
_BPQDATA		ENDS

	ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT

_BPQDATA	SEGMENT

CR	EQU	0DH
LF	EQU	0AH

D10		DW	10
D60		DD	60
D3600	DD	3600
D86400	DD	86400

TIMESTAMP	DD	0

NEWVALUE	DW	0
WORD10		DW	10
WORD16		DW	16

;
;	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT
;
MESSAGE	STRUC

MSGCHAIN	DD	?		; CHAIN WORD
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

;
PORT_MSG	DB	' Port=',NULL
UI_MSG		DB	'UI',NULL


;-----------------------------------------------------------------------------;
;          Parameter area for received frame                                  ;
;-----------------------------------------------------------------------------;

PORT_NO		DB	0		; Received port number 0 - 256
VERSION_NO	DB	0		; Version 1 or 2       1,2
POLL_FINAL	DB	0		; Poll or Final ?      P,F
COMM_RESP	DB	0		; Command or Response  C,R
FRAME_TYPE	DB	0		; Frame Type           UI etc in Hex
PID		DB	0		; Protocol ID
FRAME_LENGTH	DD	0		; Length of frame      0 - 65...
NR		DB	0		; N(R) Flag
NS		DB	0		; N(S) Flag
INFO_FLAG	DB	0		; Information Packet ? 0 No, 1 Yes
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
NORMLEN		DD	0		; LENGTH OF CALL IN NORMCALL	
;
TENK	DD	10000
	DD	1000
WORD100	DD	100
DWORD10	DD	10
;
ACCUM		DB	4 DUP (0)
CONVFLAG	DB	0
SUPPRESS	DB	0		; ZERO SUPPRESS FLAG

SAVEDI		DD	0
SAVESI		DD	0


_BPQDATA	ENDS

_TEXT   SEGMENT PUBLIC 'CODE'

	PUBLIC	_APRSMONDECODE
	
	EXTRN DISPLAY_BYTE_1:NEAR, DISPLAY_BYTE_2:NEAR, DISPADDR:NEAR, NORMSTR:NEAR

_APRSMONDECODE:
;
;	esi=message, edi=buffer
;

	MOV	SAVESI,ESI
	MOV	SAVEDI,EDI

	MOV	TIMESTAMP,EAX

;
;	GET THE CONTROL BYTE, TO SEE IF THIS FRAME IS TO BE DISPLAYED. We Should only get UI frames
;
	PUSH	ESI
	MOV	ECX,9			; MAX DIGIS
CTRLLOOP:
	TEST	BYTE PTR MSGCONTROL-1[ESI],1
	JNZ	CTRLFOUND

	ADD	ESI,7
	LOOP	CTRLLOOP
;
;	INVALID FRAME
;
	POP	ESI
	MOV	ECX,0
	RET

CTRLFOUND:

	MOV	AL,MSGCONTROL[ESI]
	POP		ESI
;
	TEST	AL,1			; I FRAME
	JZ	IFRAME

	AND	AL,NOT PFBIT		; CLEAR P/F
	CMP	AL,3			; UI
	JE	OKTOTRACE		; ALWAYS DO UI
IFRAME:
	MOV	ECX,0
	RET

OKTOTRACE:
;
;-----------------------------------------------------------------------------;
;       Get the port number of the received frame                             ;
;-----------------------------------------------------------------------------;

	mov	CL,MSGPORT[ESI]
	mov	PORT_NO,CL

	mov	AH,MSGDEST+6[ESI]
	mov	AL,MSGORIGIN+6[ESI]

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

	MOV	EAX,TIMESTAMP
	mov	EDX,0
	DIV	D86400

	MOV	EAX,EDX
	MOV	edx,0
	DIV	D3600

	CALL	DISPLAY_BYTE_2

	MOV	AL,':'
	CALL	PUTCHAR

	MOV	EAX,EDX
	MOV	EDX,0
	DIV	D60			; MINS IN AX, SECS IN DX

	PUSH	DX

	CALL	DISPLAY_BYTE_2

	MOV	AL,':'
	CALL	PUTCHAR

	POP	AX			; SECS
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

	PUSH	ESI

	lea	ESI,MSGORIGIN[ESI]
	call	CONVFROMAX25

	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	POP	ESI

	PUSH	ESI

	mov	AL,'>'
	call	PUTCHAR

;-----------------------------------------------------------------------------;
;       Display Destination Callsign                                          ;
;-----------------------------------------------------------------------------;

	lea	ESI,MSGDEST[ESI]
	call	CONVFROMAX25

	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	pop	ESI

	movzx	EAX,MSGLENGTH[ESI]
	mov	FRAME_LENGTH,EAX
	mov	CX,8			; Max number of digi-peaters

;-----------------------------------------------------------------------------;
;       Display any Digi-Peaters                                              ;
;-----------------------------------------------------------------------------;

NEXT_DIGI:

	test	MSGORIGIN+6[ESI],1
	jnz	NO_MORE_DIGIS

	add	ESI,7
	sub	FRAME_LENGTH,7		; Reduce length

	push	ECX	
	push	ESI

	lea	ESI,MSGORIGIN[ESI]
	call	CONVFROMAX25		; Convert to call

	push	EAX			; Last byte is in AH

	mov	AL,','
	call	PUTCHAR

	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	pop	EAX

	test	AH,80H
	jz	NOT_REPEATED
	
;	We should only put a * on the last repeated

	test	AH,1		; if last address must be last repeated
	jnz	NEEDSTAR
	
	pop	ESI
	test MSGORIGIN+13[ESI],80H
	push ESI
	jnz NOSTAR			; Repeated by next

NEEDSTAR:
	mov	AL,'*'
	call	PUTCHAR

NOSTAR:
NOT_REPEATED:

	pop	ESI
	pop	ECX

	loop	NEXT_DIGI

NO_MORE_DIGIS:	

;----------------------------------------------------------------------------;
;       Display the Port No of the frame                                     ;
;----------------------------------------------------------------------------;

	mov	EBX,OFFSET PORT_MSG
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

	mov	AL,MSGCONTROL[ESI]	; Get control byte

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

	cmp	AL,UI
	je	UI_FRAME
	
	MOV 	ECX,0
	RET

UI_FRAME:

	mov	EBX,OFFSET UI_MSG
	call	NORMSTR

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

	mov	AL,'>'
	call	PUTCHAR

;----------------------------------------------------------------------------;
;       Find the PID if an information frame                                 ;
;----------------------------------------------------------------------------;

	mov	AL,0

	lea	ESI,MSGPID[ESI]
	lodsb

	mov	PID,AL
	
	MOV	ECX,FRAME_LENGTH


;----------------------------------------------------------------------------;
;       Display the rest of the frame (If Any)                               ;
;----------------------------------------------------------------------------;

	mov	AL,':'
	call	PUTCHAR

	XOR	AL,AL			; IN CASE EMPTY

	sub	ECX,23
	JZ	NO_INFO			; EMPTY I FRAME
	JNS	@F
	
; Length Negative

	PUSH	EAX
	push	edi
	
	mov	[edi],0			; Null Terminate
	
	JMP	NO_INFO

@@:

;
;	PUT TEXT ON A NEW LINE
;
	PUSH	ECX
	MOV	AL,0DH
	PUSH	ESI
	CALL	PUTCHAR
	POP	ESI
	POP	ECX

	cmp	ECX,257
	jl	LENGTH_OK

	mov	ECX,256

LENGTH_OK:

	push	ECX
	lodsb
	call	PUTCHAR				; APRS requires all chars to be passed

	pop	ECX
	loop	LENGTH_OK

NO_INFO:
;
;	ADD CR UNLESS DATA ALREADY HAS ONE
;
	CMP	AL,CR
	JE	NOTANOTHER

	mov	AL,CR
	call	PUTCHAR

NOTANOTHER:

	MOV	ECX,EDI
	SUB	ECX,SAVEDI

	RET

PUTCHAR:
	STOSB
	RET
	
	
CONVFROMAX25:
;
;	CONVERT AX25 FORMAT CALL IN [SI] TO NORMAL FORMAT IN NORMCALL
;	   RETURNS LENGTH IN CX AND NZ IF LAST ADDRESS BIT IS SET
;
	PUSH	EDI		; SAVE BUFFER

	PUSH	ESI			; SAVE
	MOV	EDI,OFFSET NORMCALL
	MOV	ECX,10			; MAX ALPHANUMERICS
	MOV	AL,20H
	REP STOSB			; CLEAR IN CASE SHORT CALL
	MOV	EDI,OFFSET NORMCALL
	MOV	CL,6
CONVAX50:
	LODSB
	CMP	AL,40H
	JE	CONVAX60		; END IF CALL - DO SSID

	SHR	AL,1
	STOSB
	LOOP	CONVAX50
CONVAX60:
	POP	ESI
	ADD	ESI,6			; TO SSID
	LODSB
	MOV	AH,AL			; SAVE FOR LAST BIT TEST
	SHR	AL,1
	AND	AL,0FH
	JZ	CONVAX90		; NO SSID - FINISHED
;
	MOV	BYTE PTR [EDI],'-'
	INC	EDI
	CMP	AL,10
	JB	CONVAX70
	SUB	AL,10
	MOV	BYTE PTR [EDI],'1'
	INC	EDI
CONVAX70:
	ADD	AL,30H			; CONVERT TO DIGIT
	STOSB
CONVAX90:
	MOV	ECX,EDI
	SUB	ECX,OFFSET NORMCALL
	MOV	NORMLEN,ECX		; SIGNIFICANT LENGTH

	TEST	AH,1			; LAST BIT SET?

	POP	EDI
	RET

_TEXT	ENDS

	END
             
