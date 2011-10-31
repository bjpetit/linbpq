		TITLE	DISPLAY FRAME

	INCLUDE	ENVIRON.ASM

DATA		SEGMENT PUBLIC 'DATA'

	IF	TNC2

NULL		EQU	00H
CR		EQU	0DH
LF		EQU	0AH
NETROM_PID	EQU	0CFH
NODES_SIG	EQU	0FFH
;

		INCLUDE STRUCS.ASM

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


	EXTRN	NORMCALL:BYTE

	ENDIF

DATA	ENDS

CODE	SEGMENT PUBLIC 'CODE'
	ASSUME  CS:CODE,DS:DATA,ES:DATA,SS:DATA

	PUBLIC	DISPLAYFRAME,DISPLAYFRAMETX


	IF	TNC2

	PUBLIC	DISPLAY_BYTE_1,NORMSTR,BYTE_TO_HEX
	EXTRN	PUTCHARINBUFFER:NEAR,CONVFROMAX25:NEAR

	ENDIF

DISPLAYFRAMETX:

	IF	TNC2

	CMP	MTXFORCE[BX],0
	JNE	TRACEANYWAY

	ENDIF

DISPLAYFRAME:

	IF	TNC2

;-----------------------------------------------------------------------------;
;       Is the trace switched on ?                                            ;
;-----------------------------------------------------------------------------;

	cmp	TRACEFLAG[BX],0
	jne	TRACEOK


TRACERET:

	ENDIF

	ret

	IF	TNC2

TRACEOK:
;
;	CHECK FOR PORT SELECTIVE MONITORING
;
	MOV	CL,MSGPORT[DI]
	DEC	CL
	MOV	AL,1
	SHL	AL,CL			; SHIFT BIT UP

	TEST	MMASK[BX],AL
	JZ	TRACERET

;-----------------------------------------------------------------------------;
;	See if connected and MCON set                                         ;
;-----------------------------------------------------------------------------;

	cmp	MCON[BX],0
	jne	TRACEANYWAY

	test	VMSR[BX],80H
	jz	TRACEANYWAY		; NOT CONNECTED

	ret

TRACEANYWAY:
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
	CMP	MCOM[BX],0
	JNE	OKTOTRACE

	RET

;-----------------------------------------------------------------------------;
;       Check for MALL                                                        ;
;-----------------------------------------------------------------------------;

IFRAME:
	cmp	MALL[BX],0
	jne	OKTOTRACE

	ret

OKTOTRACE:
;
;-----------------------------------------------------------------------------;
;       Get the port number of the received frame                             ;
;-----------------------------------------------------------------------------;

	mov	AL,MSGPORT[DI]
	mov	PORT_NO,AL

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
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER

	mov	SI,OFFSET NORMCALL
	call	DISPADDR

	pop	AX

	test	AH,80H
	jz	NOT_REPEATED

	mov	AL,'*'
	call	PUTCHARINBUFFER

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
	add	AL,30H
	call	PUTCHARINBUFFER
;
;	IF BBS MONITOR MODE, STOP HERE - IT HAS ENOUGH FOR THE J LIST
;
	CMP	BBSMON[BX],0
	JE	NORMAL_MON

	mov	AL,CR
	call	PUTCHARINBUFFER

	POP	DI
	RET

NORMAL_MON:

	mov	AL,' '
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER
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
	call	PUTCHARINBUFFER
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
	call	PUTCHARINBUFFER
	jmp	SHORT END_OF_TYPE

NOT_SABM_FRAME:
	cmp	AL,DISC
	jne	NOT_DISC_FRAME

	mov	AL,'D'
	call	PUTCHARINBUFFER
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
	call	PUTCHARINBUFFER

END_OF_TYPE:

;----------------------------------------------------------------------------;
;       If Version 2 Then display P/F C/R Information                        ;
;----------------------------------------------------------------------------;

	cmp	VERSION_NO,2
	jne	NOT_VERSION_2

	mov	AL,' '
	call	PUTCHARINBUFFER

	mov	AL,COMM_RESP		; Print Command/Response Flag
	call	PUTCHARINBUFFER

	cmp	POLL_FINAL,0
	je	NO_POLL_FINAL

	mov	AL,' '
	call	PUTCHARINBUFFER

	mov	AL,POLL_FINAL		; Print Poll/Final Flag if Set
	call	PUTCHARINBUFFER

NO_POLL_FINAL:
NOT_VERSION_2:

;----------------------------------------------------------------------------;
;       Display sequence numbers if applicable                               ;
;----------------------------------------------------------------------------;

	cmp	NS,1
	jne	NOT_NS_DATA

	mov	AL,' '
	call	PUTCHARINBUFFER

	mov	AL,'S'
	call	PUTCHARINBUFFER

	mov	AL,FRAME_TYPE
	ror	AL,1

	call	DISPLAYSEQ

NOT_NS_DATA:
	cmp	NR,1
	jne	NOT_NR_DATA

	mov	AL,' '
	call	PUTCHARINBUFFER

	mov	AL,'R'
	call	PUTCHARINBUFFER

	mov	AL,FRAME_TYPE
	rol	AL,1
	rol	AL,1
	rol	AL,1

	call	DISPLAYSEQ

NOT_NR_DATA:
	mov	AL,'>'
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER

	XOR	AL,AL			; IN CASE EMPTY

	sub	CX,21
	JCXZ	NO_INFO			; EMPTY I FRAME

	TEST	HEADERLN[BX],1
	JZ	SAMELN
;
;	PUT TEXT ON A NEW LINE
;
	MOV	AL,0DH
	CALL	PUTCHARINBUFFER
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
	call	PUTCHARINBUFFER
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
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER
	mov	AL,' '
	call	PUTCHARINBUFFER

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

	call	PUTCHARINBUFFER

	loop	MNEMONIC_LOOP

END_MNEMONIC:
	mov	AL,':'
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER
	mov	AL,'R'
	call	PUTCHARINBUFFER

	mov	AL,-1[SI]		; Get receive sequence number
	call	DISPLAY_BYTE_1

	mov	AL,'>'
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER

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
	CALL	PUTCHARINBUFFER
L4F010:
	TEST	OPCODE,L4NAK
	JZ	L4F020

	MOV	AL,'N'
	CALL	PUTCHARINBUFFER
L4F020:
	TEST	OPCODE,L4MORE
	JZ	L4F030

	MOV	AL,'M'
	CALL	PUTCHARINBUFFER
L4F030:
	RET

;----------------------------------------------------------------------------;
;       Display ASCIIZ strings                                               ;
;----------------------------------------------------------------------------;

NORMSTR:
	lodsb
	cmp	AL,NULL		; End of String ?
	je	NORMSTR_RET	; Yes
	call	PUTCHARINBUFFER
	jmp	SHORT NORMSTR

NORMSTR_RET:
	ret

;-----------------------------------------------------------------------------;
;       Display sequence numbers                                              ;
;-----------------------------------------------------------------------------;

DISPLAYSEQ:
	and	AL,7
	add	AL,30H
	call	PUTCHARINBUFFER
	ret

;-----------------------------------------------------------------------------;
;       Display Callsign pointed to by SI                                     ;
;-----------------------------------------------------------------------------;

DISPADDR:
	jcxz	DISPADDR_RET

	lodsb
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER
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
	call	PUTCHARINBUFFER
	pop	AX

UNITS_1:
	add	AL,30H
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER
	pop	AX

UNITS_2:
	add	AL,30H
	call	PUTCHARINBUFFER

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
	call	PUTCHARINBUFFER
	ret

	ENDIF

CODE	ENDS
	END
