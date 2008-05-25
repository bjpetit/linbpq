
#include <gmodule.h>

//	HDLC COMMANDS (WITHOUT P/F)


#define SABM	0x2F
#define DISC	0x43
#define DM		0x0F
#define UA		0x63
#define FRMR	0x87

#define PFBIT	0x10

#define UI 3
#define RR 1
#define RNR 5
#define REJ 9

static guint8 FRAME_TYPE;
static guint8 FRMRFLAG;
static guint8 COMM_RESP;		//Print Command/Response Flag
static guint8 POLL_FINAL;



static guint8 VERSION_NO;		// Version 1 or 2       1,2
static guint8 PID;				// Protocol ID
static guint32 FRAME_LENGTH;	// Length of frame      0 - 65...
static guint8 NR;				// N(R) Flag
static guint8 NS;				// N(S) Flag
guint8 INFO_FLAG;		// Information Packet ? 0 No, 1 Yes
static guint8 OPCODE;			// L4 FRAME TYPE
static guint8 FRMRFLAG;


static char UA_MSG[]="UA";

static char DM_MSG	[]="DM";
static char RR_MSG	[]="RR";
static char RNR_MSG[]="RNR";
static char UI_MSG[]="UI";
static char FRMR_MSG[]="FRMR";
static char REJ_MSG[]="REJ";

static char AX25CALL[8];		// WORK AREA FOR AX25 <> NORMAL CALL CONVERSION
static char NORMCALL[11];	// CALLSIGN IN NORMAL FORMAT
static int NORMLEN;			// LENGTH OF CALL IN NORMCALL	


int DecodeDigiString(guint8 * pkt,char * digiString)
{
	int NumberofDigis=0;

	_asm{

	mov	esi,pkt
	mov edi,digiString

	mov	ECX,8			; Max number of digi-peaters

;-----------------------------------------------------------------------------;
;       Display any Digi-Peaters                                              ;
;-----------------------------------------------------------------------------;

NEXT_DIGI:

	push	ESI

	push	ECX
	call	CONVFROMAX25		; Convert to call

	push	EAX			; Last byte is in AH

	mov	AL,','
	stosb

	mov	ESI,OFFSET NORMCALL
	rep movsb

	pop	EAX

	test	AH,80H
	jz	NOT_REPEATED

	mov	AL,'*'
	stosb

NOT_REPEATED:

	pop	ECX
	pop	ESI

	inc	NumberofDigis

	TEST	6[ESI],1
	jnz	NO_MORE_DIGIS

	ADD	ESI,7
	loop	NEXT_DIGI

NO_MORE_DIGIS:	

	xor	al,al
	stosb


	}

	return NumberofDigis;



	_asm{

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

}

}


const gchar * DecodeControlByte(guint8 ctrl,char * CtrlString,guint8 DestFlag,guint8 SrcFlag)
{

	gchar * retval;

	_asm{


	mov edi,CtrlString

	MOV	FRMRFLAG,0

	mov	AH,DestFlag
	mov	AL,SrcFlag

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
;       If this is Version 2 get the Poll/Final Bit                           ;
;-----------------------------------------------------------------------------;

	mov	POLL_FINAL,0		; Clear Poll/Final Flag

	mov	AL,ctrl	; Get control byte

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

	mov	EBX,OFFSET UI_MSG
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

	mov	EBX,OFFSET RR_MSG
	call	NORMSTR
	jmp	END_OF_TYPE

NOT_RR_FRAME:
	cmp	AL,RNR
	jne	NOT_RNR_FRAME

	mov	EBX,OFFSET RNR_MSG
	call	NORMSTR
	jmp	END_OF_TYPE

NOT_RNR_FRAME:
	cmp	AL,REJ
	jne	NOT_REJ_FRAME

	mov	EBX,OFFSET REJ_MSG
	call	NORMSTR
	jmp	END_OF_TYPE

NOT_REJ_FRAME:
	mov	NR,0			; Don't display sequence number
	mov	AL,'?'			; Print "?"
	call	PUTCHAR
	jmp	END_OF_TYPE

;-----------------------------------------------------------------------------;
;       Process all other frame types                                         ;
;-----------------------------------------------------------------------------;

NOT_R_FRAME:
	cmp	AL,UA
	jne	NOT_UA_FRAME

	mov	EBX,OFFSET UA_MSG
	call	NORMSTR
	jmp	SHORT END_OF_TYPE

NOT_UA_FRAME:
	cmp	AL,DM
	jne	NOT_DM_FRAME

	mov	EBX,OFFSET DM_MSG
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

	mov	EBX,OFFSET FRMR_MSG
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
;	lea	ESI,MSGPID[ESI]
	MOV	ECX,3			; TESTING
FRMRLOOP:
	lodsb
	CALL	BYTE_TO_HEX

	LOOP	FRMRLOOP

NOTFRMR:

	mov	retval,edi

	}

	return retval;

	_asm{



NORMSTR:
	MOV	AL,[EBX]
	INC	EBX
	cmp	AL,0		; End of String ?
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

PUTCHAR:
	STOSB
	RET

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



	}
}

/*
L3MESSAGE	STRUC
;
;	NETROM LEVEL 3 MESSAGE - WITHOUT L2 INFO 
;
L3HEADER	DB	7 DUP (?)	; CHAIN, PORT, LENGTH
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

*/


//	L4FLAGS DEFINITION

#define L4BUSY 0x80 //		; BNA - DONT SEND ANY MORE
#define L4NAK  0x40 //		; NEGATIVE RESPONSE FLAG
#define L4MORE 0x20 //		; MORE DATA FOLLOWS - FRAGMENTATION FLAG

#define L4CREQ	1		//; CONNECT REQUEST
#define L4CACK	2		//; CONNECT ACK
#define L4DREQ	3		//; DISCONNECT REQUEST
#define L4DACK	4		//; DISCONNECT ACK
#define L4INFO	5		//; INFORMATION
#define L4IACK	6		//; INFORMATION ACK
;


#define CR 0x0D
#define LF 0x0A

#define NETROM_PID 0xCF
#define NODES_SIG  0xFF
;
static char PORT_MSG	[]=" Port=";
static char NODES_MSG	[]=" NODES broadcast from ";
static char TO_MSG		[]=" to ";
static char TTL_MSG		[]=" ttl=";
static char AT_MSG		[]=" at ";
static char VIA_MSG		[]=" via ";
static char QUALITY_MSG	[]=" qlty=";

static char MYCCT_MSG[]=" my";
static char CCT_MSG	[]=" cct=";
static char TIM_MSG[]=" t/o=";

static char WINDOW[]=" w=";
static char CONN_REQ_MSG[]=" <CON REQ>";
static char CONN_ACK_MSG[]=" <CON ACK>";
static char CONN_NAK_MSG[]=" <CON NAK> - BUSY";
static char DISC_REQ_MSG[]=" <DISC REQ>";
static char DISC_ACK_MSG[]=" <DISC ACK>";
static char INFO_MSG[]=" <INFO S";
static char INFO_ACK_MSG[]=" <INFO ACK R";
static char DUFF_NET_MSG[]=" <????>";
static char IP_MSG[]=" <IP> ";


guint8 NR_INFO_FLAG;		// Information Packet ? 0 No, 1 Yes

char * DecodeNetrom(guint8 * pkt,char * nrString)
{

	_asm{

	mov	esi,pkt
	mov edi,nrString

		MOV		NR_INFO_FLAG,0		// Information Packet ? 0 No, 1 Yes

;----------------------------------------------------------------------------;
;       Display NET/ROM data                                                 ;
;----------------------------------------------------------------------------;


	lodsb
	cmp	AL,NODES_SIG		; Check NODES message

	JNE	DISPLAY_NETROM_DATA

;----------------------------------------------------------------------------;
;       Display NODES broadcast                                              ;
;----------------------------------------------------------------------------;

	push	ECX

	mov	EBX,OFFSET NODES_MSG
	call	NORMSTR

	mov	ECX,6
	REP MOVSB

//	mov	AL,CR
//	call	PUTCHAR

	pop	ECX

	sub	ECX,30			; Header, mnemonic and signature length
/*
NODES_LOOP:

	cmp	ECX,0
	jbe	NO_INFO

	push	ECX
	push	ESI			; Global push for each node

	mov	AL,' '
	call	PUTCHAR
	mov	AL,' '
	call	PUTCHAR

	push	ESI

	add	ESI,7			; Display destination mnemonic

	cmp	BYTE PTR [ESI],' '
	je	NO_MNEMONIC

	mov	ECX,6			; Max length

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

	pop	ESI
	push	ESI

	call	CONVFROMAX25		; Display dest callsign
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	mov	EBX,OFFSET VIA_MSG
	call	NORMSTR

	pop	ESI
	add	ESI,13			; Point to neighbour callsign
	push	ESI

	call	CONVFROMAX25
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	mov	EBX,OFFSET QUALITY_MSG
	call	NORMSTR

	pop	ESI
	add	ESI,7			; Point to quality byte

	mov	AL,[ESI]
	call	DISPLAY_BYTE_1

	mov	AL,CR
	call	PUTCHAR

	pop	ESI
	pop	ECX
	add	ESI,21			; Point to next destination
	sub	ECX,21			; Remove length of each 

	jmp	NODES_LOOP


*/

	xor	al,al
	stosb

	}

	return 0 ;

	_asm{

//---------------------------------------------------------------------------;
//       Display normal NET/ROM transmissions                                 ;
//----------------------------------------------------------------------------;

DISPLAY_NETROM_DATA:

	DEC	ESI			; BACK TO DATA


	PUSH	ESI

	call	CONVFROMAX25
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	mov	EBX,OFFSET TO_MSG
	call	NORMSTR

	pop	ESI
	add	ESI,7

	push	ESI

	call	CONVFROMAX25
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR
;
;       Display Time To Live number
;
	mov	EBX,OFFSET TTL_MSG
	call	NORMSTR

	pop	ESI
	add	ESI,7			; Point to TTL counter

	lodsb
	call	DISPLAY_BYTE_1

;
;	DISPLAY CIRCUIT ID
;
	MOV	EBX,OFFSET CCT_MSG
	CALL	NORMSTR

	LODSB
	CALL	BYTE_TO_HEX

	LODSB
	CALL	BYTE_TO_HEX

	INC	ESI
	INC	ESI		; TO OPCODE

;-----------------------------------------------------------------------------;
;       Determine type of Level 4 frame                                       ;
;-----------------------------------------------------------------------------;

	mov	AL,[ESI]
	MOV	OPCODE,AL		; SAVE
	AND	AL,0FH			; Strip off flags

	cmp	AL,L4CREQ
	jne	NOT_L4CREQ

	mov	EBX,OFFSET CONN_REQ_MSG
	call	NORMSTR

	MOV	EBX,OFFSET WINDOW
	CALL	NORMSTR

	INC	ESI
	LODSB			; WINDOW SIZE

	CALL	DISPLAY_BYTE_1

	mov	AL,' '
	call	PUTCHAR

	PUSH	ESI

	call	CONVFROMAX25
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR

	mov	EBX,OFFSET AT_MSG
	call	NORMSTR

	pop	ESI
	add	ESI,7
	PUSH	ESI

	call	CONVFROMAX25
	mov	ESI,OFFSET NORMCALL
	call	DISPADDR


	POP	ESI
	CMP	FRAME_LENGTH,58
	JE	NOT_BPQ
;
;	BPQ EXTENDED CON REQ - DISPLAY TIMEOUT
;
	MOV	EBX,OFFSET TIM_MSG
	CALL	NORMSTR

	MOV	AX,7[ESI]		; TIMEOUT

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
	MOV	EBX,OFFSET CONN_NAK_MSG
	CALL	NORMSTR

	JMP	END_NETROM

L4CRQ00:

	MOV	EBX,OFFSET CONN_ACK_MSG
	CALL	NORMSTR

	MOV	EBX,OFFSET WINDOW
	CALL	NORMSTR

	MOV	AL,1[ESI]			; WINDOW SIZE

	CALL	DISPLAY_BYTE_1

	MOV	EBX,OFFSET MYCCT_MSG
	CALL	NORMSTR

	dec	esi
	dec esi
	
	lodsb
	CALL	BYTE_TO_HEX

lodsb
	CALL	BYTE_TO_HEX

	JMP	ADD_CR

NOT_L4CACK:

	cmp	AL,L4DREQ
	jne	NOT_L4DREQ

	mov	EBX,OFFSET DISC_REQ_MSG
	call	NORMSTR

	JMP	ADD_CR

NOT_L4DREQ:

	cmp	AL,L4DACK
	jne	NOT_L4DACK

	mov	EBX,OFFSET DISC_ACK_MSG
	call	NORMSTR

	jmp	add_cr

NOT_L4DACK:

	cmp	AL,L4INFO
	jne	NOT_L4INFO

	mov	EBX,OFFSET INFO_MSG
	call	NORMSTR

	dec esi
	dec esi
	lodsb		; Get send sequence number
	call	DISPLAY_BYTE_1

	mov	AL,' '
	call	PUTCHAR
	mov	AL,'R'
	call	PUTCHAR

	lodsb		; Get receive sequence number
	call	DISPLAY_BYTE_1

	mov	AL,'>'
	call	PUTCHAR

	INC	ESI			; TO DATA
	MOV	ECX,FRAME_LENGTH
	sub	ECX,20
	
	CALL	DOL4FLAGS

	MOV		NR_INFO_FLAG,1		// Information Packet ? 0 No, 1 Yes

	jmp	DISPLAY_INFO


NOT_L4INFO:

	cmp	AL,L4IACK
	jne	NOT_L4IACK

	mov	EBX,OFFSET INFO_ACK_MSG
	call	NORMSTR

	dec esi
	lodsb
	inc	esi			; Get receive sequence number
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
	mov	EBX,OFFSET IP_MSG
	call	NORMSTR

/*
;
	INC	ESI			; NOW POINTING TO IP HEADER
;
	PUSH	ESI

	LEA	ESI,IPSOURCE[ESI]
	CALL	PRINT4			; PRINT IF ADDR IN 'DOTTED DECIMAL' FORMAT

	POP	ESI

	MOV	AL,'>' 
	CALL	PUTCHAR

	PUSH	ESI

	LEA	ESI,IPDEST[ESI]
	CALL	PRINT4			; PRINT IF ADDR IN 'DOTTED DECIMAL' FORMAT

	MOV	EBX,OFFSET LEN
	CALL	NORMSTR

	POP	ESI

	MOV	AL,BYTE PTR IPLENGTH[ESI]
	CALL	BYTE_TO_HEX

	MOV	AL,BYTE PTR IPLENGTH+1[ESI]
	CALL	BYTE_TO_HEX

	MOV	AL,20H
	CALL	PUTCHAR

	MOV	AL, byte ptr IPPROTOCOL[ESI]
	CALL	DISPLAY_BYTE_1		; DISPLAY PROTOCOL TYPE

;	mov	AL,CR
;	call	PUTCHAR
;
;	MOV	ECX,39			; TESTING
;IPLOOP:
;	lodsb
;	CALL	BYTE_TO_HEX
;
;	LOOP	IPLOOP

	JMP	ADD_CR

*/

NOTIP:

	mov	EBX,OFFSET DUFF_NET_MSG
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


ADD_CR:
DISPLAY_INFO:

	xor	al,al
	stosb

	}

	return 0 ;

	_asm{


;-----------------------------------------------------------------------------;
;       Display sequence numbers                                              ;
;-----------------------------------------------------------------------------;

DISPLAYSEQ:
	and	AL,7
	add	AL,0x30
	call	PUTCHAR
	ret

;-----------------------------------------------------------------------------;
;       Display Callsign pointed to by SI                                     ;
;-----------------------------------------------------------------------------;

DISPADDR:
	
	jcxz	DISPADDR_RET

	LODS	NORMCALL
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
	add	AL,0x30
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
	add	AL,0x30
	call	PUTCHAR
	pop	AX

UNITS_1:
	add	AL,0x30
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
	add	AL,0x30
	call	PUTCHAR
	pop	AX

	add	AL,0x30
	call	PUTCHAR

	ret


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



NORMSTR:
	MOV	AL,[EBX]
	INC	EBX
	cmp	AL,0		; End of String ?
	je	NORMSTR_RET	; Yes
	call	PUTCHAR
	jmp	SHORT NORMSTR

NORMSTR_RET:
	ret



PUTCHAR:
	STOSB
	RET



}

}