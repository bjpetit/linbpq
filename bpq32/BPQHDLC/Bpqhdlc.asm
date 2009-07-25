	PAGE 58,132

;******************************************************************************
TITLE BPQHDLC - VxD TO Drive HDLC cards for BPQ32 Switch
;
;	BASED ON MICROSOFT'S GENERIC VxD SAMPLE
;
        .386p

DEBUG EQU 1

SETRVEC	MACRO    A			;MACRO TO CHANGE THE  RX INT VECTOR
	MOV	IORXCA[EBX],OFFSET32 A
	ENDM
SETTVEC	MACRO	A			;MACRO TO CHANGE THE  TX INT VECTOR
	MOV	IOTXCA[EBX],OFFSET32 A
	ENDM

SIOR	MACRO
	MOV	DX,SIO[EBX]
	IN	AL,DX
	ENDM
;
SIOW	MACRO				; OUT	(SIO),A
	MOV	DX,SIO[EBX]
	OUT	DX,AL
	ENDM
;
SIOCR	MACRO				; IN	A,(SIOCMD)
	MOV	DX,SIOC[EBX]
	IN	AL,DX
	ENDM
;
SIOCW	MACRO				; OUT	(SIOCMD),A
	MOV	DX,SIOC[EBX]
	OUT	DX,AL
	ENDM
;
SIOCAD	MACRO				; LD	C,(SIOCMD)
	MOV	DX,SIOC[EBX]
	ENDM
;
DELAY	MACRO
	JMP	$+2
	JMP	$+2
	JMP	$+2
	JMP	$+2
ENDM



;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

        INCLUDE VMM.Inc
        INCLUDE Debug.Inc
		INCLUDE V86MMGR.INC
		include vpicd.inc
		
TNC2		EQU	0		; NO SUPPORT FOR TNC2 MODE

		include ..\include\strucs.inc

;******************************************************************************
;              V I R T U A L   D E V I C E   D E C L A R A T I O N
;------------------------------------------------------------------------------
; The VxD declaration statement defines the VxD name, version number,
; control proc entry point, VxD ID, initialization order, and VM API 
; entry points. What follows is a minimal VxD declaration, defining 
; only the name, version, control proc, and an undefined ID. Depending
; on the requirements of the VxD, the following may be added:
;
; - Defined VxD ID: See VXDID.TXT for more information
; - Init order: If your Vxd MUST load before or after a specific VxD,
;               the init order can be defined. See VMM.INC for the
;               definition of the init order of the standard devices.
; - V86,PM API: You may wish to allow application or library code running
;               in a virtual machine to communicate with your VxD directly.
;               See the chapter entitled "VxD APIs (Call-Ins)" in the
;               Virtual Device Adaptation Guide.
;               
;******************************************************************************

Declare_Virtual_Device  BPQHDLC, 1, 0,   BPQVXD_Control


;******************************************************************************
;                                D A T A
;******************************************************************************

;VxD_DATA_SEG
;
;VxD_DATA_ENDS


VxD_LOCKED_DATA_SEG

;       Pagelocked data here - try to keep this to a minimum.

;
;	DEFINE MAPPING FOR HDLC DRIVER
;
HDLCDATA	STRUC
		DB	HARDWAREDATA DUP (0)	; REMAP HARDWARE INFO

ASIOC		DW	0		; A CHAN ADDRESSES
SIO			DW	0		; OUR ADDRESSES (COULD BE A OR B) 
SIOC		DW	0
BSIOC		DW	0		; B CHAN CONTROL

A_PTR		DD	0		; PORT ENTRY FOR A CHAN
B_PTR		DD	0		; PORT ENTRY FOR B CHAN

IOTXCA		DD	0		; INTERRUPT VECTORS
IOTXEA		DD	0
IORXCA		DD	0
IORXEA		DD	0	
;
LINKSTS		DB	0
;
SDRNEXT		DD	0
SDRXCNT		DD	0
CURALP		DD	0
OLOADS		DB	0		; LOCAL COUNT OF BUFFERS SHORTAGES
FRAMELEN	DW	0
SDTNEXT		DD	0		; POINTER to NEXT BYTE to TRANSMIT
SDTXCNT		DW	0		; CHARS LEFT TO SEND
RR0			DB	0		; CURRENT RR0
TXFRAME		DD	0		; ADDRESS OF FRAME BEING SENT
;
SDFLAGS		DB	0		; GENERAL FLAGS

PCTX_Q		DD	0		; HDLC HOLDING QUEUE
RXMSG_Q		DD	0		; RX INTERRUPT TO SDLC BG
;

;SOFTDCD		DB	0		; RX ACTIVE FLAG FOR 'SOFT DC
XXTXDELAY	DB	0		; TX KEYUP DELAY TIMER
SLOTTIME	DB	0		; TIME TO WAIT IF WE DONT SEND
FIRSTCHAR	DB	0		; CHAR TO SEND FOLLOWING TXDELAY
L1TIMEOUT	DW	0		; UNABLE TO TX TIMEOUT
PORTSLOTIMER	DB	0

TXBRG		DW	0		; FOR CARDS WITHOUT /32 DIVIDER
RXBRG		DW	0	

WR10		DB	0		; NRZ/NRZI FLAG

IRQHand		DD	0
TimeOut_Handle	dd	0

HDLCDATA		ENDS


	IF	TYPE HDLCDATA GT TYPE PORTCONTROL
	.ERR2	TOO MUCH PORT DATA
	ENDIF


;	SDFLAGS
;
 SDTINP		EQU	10B		; 1 = TRANSMISSION in PROGRESS
 SDRINP		EQU	1000000B	; 1 = RX IN PROGRESS
 ABSENT		EQU	1B		; 1 = ABORT SENT

VECTORS	EQU	IOTXCA

MAXPORTS	EQU	16

MARKER	DB	'BPQ '
TRACE_Q	DD	0
FREE_Q	DD	0
QCOUNT	DD	0

PORTS		DB	MAXPORTS * TYPE PORTCONTROL DUP (0)

BUFFLEN		EQU	360		; ??

dot			equ	1b
dash		equ	10b
dotspace	equ	100b
letterspace	equ	1000b
IDPENDING	EQU	10000B
;
NEEDSLOT	DB	0
RANDOM		DB	0

B20			DB	60			; multiplier for CW element

NUMBEROFPORTS	DD	0

PORTTABLE		DD	OFFSET PORTS

NEXTPORTTABLE	DD	OFFSET PORTS
INITDONE	DB	0

BUFFS	EQU	20

POOL	DB	BUFFS*BUFFLEN DUP (0)

vid		VPICD_IRQ_Descriptor <0>

VxD_LOCKED_DATA_ENDS



;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;------------------------------------------------------------------------------
;    Code in the initialization segment is discarded after Init_Complete
;******************************************************************************

VxD_ICODE_SEG

;******************************************************************************
;
;   VXD_Device_Init
;
;   DESCRIPTION:
;       This is a shell for a routine that is called at system BOOT.
;       Typically, a VxD would do its initialization in this routine.
;
;   ENTRY:
;       EBX = System VM handle
;
;   EXIT:
;       Carry clear to indicate load success
;       Carry set to abort loading this VxD
;
;   USES:
;       flags
;
;==============================================================================

BeginProc VXD_Device_Init

	    Debug_Printf	"BPQHDLC Device Init\n"
	    
		clc                             ;no error - load VxD 
        ret

EndProc VXD_Device_Init

BEGINPROC	BPQVXD_SYS_CRITICAL_INIT
;
;	SET UP TRAP FOR INTERRUPT PASSED IN EDX
;
	CLC
	RET


ENDPROC		BPQVXD_SYS_CRITICAL_INIT

VxD_ICODE_ENDS



;******************************************************************************
;                               C O D E
;------------------------------------------------------------------------------
; The 'body' of the VxD would typically be in the standard code segment.
;******************************************************************************

VxD_CODE_SEG


;******************************************************************************
;
;   VXD_Create_VM
;
;   DESCRIPTION:
;       This is a shell for a routine that is called when a virtual
;       machine is created. A typical VxD might perform some action
;       here to prepare for handling the new VM in the system.
;
;   ENTRY:
;       EBX = VM handle
;
;   EXIT:
;       Carry clear to continue creating VM
;       Carry set to abort the creation of the VM
;
;   USES:
;       flags
;
;==============================================================================

BeginProc VXD_Create_VM

        clc                             ;no error - continue
		ret

EndProc VXD_Create_VM

DIOCParams	STRUC
Internal1	DD	?
VMHandle	DD	?
Internal2	DD	?
dwIoControlCode	DD	?
lpvInBuffer	DD	?
cbInBuffer	DD	?
lpvOutBuffer	DD	?
cbOutBuffer	DD	?
lpcbBytesReturned	DD	?
lpoOverlapped	DD	?
hDevice	DD	?
tagProcess	DD	?
DIOCParams	ENDS

VWIN32_DIOC_GETVERSION	EQU	<DIOC_GETVERSION>
VWIN32_DIOC_DOS_IOCTL	EQU	1
VWIN32_DIOC_DOS_INT25	EQU	2
VWIN32_DIOC_DOS_INT26	EQU	3
VWIN32_DIOC_DOS_INT13	EQU	4
VWIN32_DIOC_SIMCTRLC	EQU	5
VWIN32_DIOC_CLOSEHANDLE	EQU	<DIOC_CLOSEHANDLE>


BeginProc VXD_IOCONTROL

        mov     EAX,dwIoControlCode[ESI]

 ;       PUSHAD
 ;       Debug_Printf	"hdlc98 IOCONTROL %x", <eax>
 ;       POPAD
        
        cmp		EAX,VWIN32_DIOC_GETVERSION
		je		getvers

		cmp		EAX,'T'
		jne short nottimer

		push	ESI

		mov		EBX,lpvInBuffer[ESI]	; port table

		call	HDLCTIMER

		pop		ESI

		xor		eax,eax

		ret

nottimer:

		cmp		EAX,'S'
		je short senddata

		cmp		EAX,'G'
		je short getdata

		cmp		EAX, 'I'
		je		INITPORT
				
		jmp		skip					; Unknown
senddata:
		push	ESI

		mov		ECX,cbInBuffer[ESI]		; length
		mov		ESI,lpvInBuffer[ESI]	; buffer

		mov		EBX,[ESI]			; PORT ADDRESS

		push	ESI					; save buffer

		mov		ESI,offset FREE_Q
		CLI
		call	Q_REM
		STI
			
		pop		ESI
		jz short txnobuffs

		dec		QCOUNT

		push	EDI
		rep movsb				; copy msg to buffer

		pop		EDI				; buffer

		lea		ESI,PCTX_Q[EBX]
		CLI
		call	Q_ADD
		STI

		CMP	L1TIMEOUT[EBX],0
		JNE SHORT QTXRET			; TIMER ALREADY RUNNING

		MOV	L1TIMEOUT[EBX],10*55	; GIVE IT NEARLY A MINUTE

QTXRET:

txnobuffs:

		pop		ESI		

        xor      eax,eax
        ret

getdata:

		push	ESI

		mov		EBX,lpvInBuffer[ESI]	; port table
		mov		ECX,cbInBuffer[ESI]		; random number

		mov		RANDOM,cl

		lea		ESI,RXMSG_Q[EBX]
		CLI
		call	Q_REM
		STI

		jz short nomsg
	
		mov		EDX,EDI		; save buffer
		movzx	ECX,word ptr 5[EDI] ; length
		
		pop		ESI			; control info

		mov     edi,lpcbBytesReturned[ESI]
		mov     dword ptr [edi],ECX                
		   
        mov     edi,lpvOutBuffer[ESI]
		mov		esi,edx
		rep movsb
	
		push	ESI
		
		mov		ESI,offset FREE_Q
		mov		EDI,EDX
		CLI
		CALL	Q_ADDF				; return buffer
		STI	
nomsg:		
		pop		ESI
		 
		call	HDLC_CHECK_TX		; checks for send
		
        xor      eax,eax
        ret


getvers:

        cmp    cbOutBuffer[ESI],0
        je short skip
   
        mov     edi,lpvOutBuffer[ESI]
        mov     dword ptr [edi],42500407h
        mov     edi,lpcbBytesReturned[ESI]
        mov     dword ptr [edi],4                
skip:

        xor      eax,eax
        ret





CLOCKFREQ	DD	76800		; 4,915,200 HZ /(32*2)

TOSHCLOCKFREQ	DD	57600


SDLCCMD	DB	0,0

	DB	2,00000000B		; BASE VECTOR 
	DB	4,00100000B		; SDLC MODE
	DB	3,11001000B		; 8BIT,  CRC ENABLE, RX DISABLED
	DB	7,01111110B		; STANDARD FLAGS
	DB	1,00010011B		; INT ON ALL RX, TX INT EN, EXT INT EN
	DB	5,11100001B		; DTR, 8BIT, SDLC CRC,TX CRC EN
;
	DB	10,10100000B		; CRC PRESET TO 1

	DB	9,00001001B		; ENABLE INTS

	DB	11,01100110B		; NO XTAL, RXC = DPLL, TXC = RTXC, TRXC = BRG (NEEDS /32 BETWEEN TRXC AND RTXC)

	DB	14,10000011B
	DB	14,00100011B
	DB	15,11000000B		; EXT INT ONLY ON TX UND AND ABORT RX

SDLCLEN	EQU	$-SDLCCMD
;

TOSHR11	DB	01111000B		; NO XTAL, RXC = DPLL, TXC = DPLL, NO CLK OUTPUT

CIOPARAMS	LABEL	BYTE

	DB	2BH,0FFH		; B DIRECTION - ALL IN
	DB	23H,0FFH		; A DIRECTION - ALL IN

	DB	1DH,0E2H		; C/T 2 MODE - CONT, EXT IN, EXT O, SQUARE
	DB	1CH,0E2H		; C/T 1 MODE   		"" 

	DB	19H,10H			; C/T 2 LSB - 16 = /32 FOR SQUARE WAVE
	DB	18H,0			;       MSB

	DB	17H,10H			; C/T 1 LSB
	DB	16H,0			;       MSB

	DB	0BH,04H			; CT2   ""    - GATE
	DB	0AH,04H			; CT1   ""    - GATE

	DB	06H,0FH			; PORT C DIRECTION - INPUTS

	DB	1,84H			; ENABLE PORTS A AND B

	DB	0,0			; INTERRUPT CONTROL

CIOLEN	EQU	$-CIOPARAMS

SDTXUND	EQU	0C0H		; RESET TX UNDERRUN/EOM LATCH

SDABTX	EQU	18H
SDUNDER	EQU	40H
SDTXCRC	EQU	80H		; RESET TX CRC GEN
SDABORT	EQU	80H		; ABORT DETECTED
SDEXTR	EQU	10H		; RESET EXT/STATUS INTS
SDRPEND	EQU	28H		; RESET TX INT PENDING


INITPORT:

	PUSHAD
	Debug_Printf	"hdlc98 Init"
	POPAD


	push	ESI
	mov		ECX,TYPE PORTCONTROL
	mov		ESI,lpvInBuffer[ESI]
;
;	GET NEXT PORT TABLE ENTRY
;
	mov		EDI,[NEXTPORTTABLE]
	rep movsb
		
;	BUILD BUFFER POOL IF FIRST CALL
;
	CMP		INITDONE,0
	JNE		POOLDONE

	MOV	INITDONE,1			

	MOV	EDI,OFFSET POOL
	MOV	ECX,BUFFS

BUFF000:

	MOV	ESI,OFFSET32 FREE_Q
	PUSH	EDI
	CALL	Q_ADDF
	POP	EDI

	ADD	EDI,BUFFLEN
	LOOP	BUFF000

POOLDONE:

	POP		ESI				; PARAM LIST
;
;	GET NEXT PORT ENTRY
;
	MOV		EBX,NEXTPORTTABLE
	ADD		NEXTPORTTABLE,TYPE PORTCONTROL
	INC		NUMBEROFPORTS
;
;	RETURN PORT TABLE ADDRESS TO CALLER
;
	mov     edi,lpcbBytesReturned[ESI]
	mov     dword ptr [edi],EBX                
		  
	mov     edi,lpvOutBuffer[ESI]
	mov		[EDI],EBX
	

	MOVZX	EAX,PORTTYPE[EBX]
	
	PUSHAD
	Debug_Printf	"hdlc98 PortType %x", <eax>
	POPAD

	
	CMP	AL,2
	JE	PC120INIT
	
	CMP	AL,4
	JE	DRSIINIT

	CMP	AL,6
	JE	TOSHINIT
	
	CMP	AL,10
	JE	RLC100INIT
					
	CMP	AL,12
	JE	RLC100INIT
	
	CMP	AL,18
	JE	BAYCOMINIT

	CMP	AL,20
	JE	PA0INIT

	PUSHAD
	Debug_Printf	"hdlc98 Init Unknown Type"
	POPAD

	xor eax,eax
	
	RET

DRSIINIT:				; INTERRUPT INITIALISATION CODE

	PUSHAD
	movzx eax, IOBASE[EBX]
	Debug_Printf	"hdlc98 DRSIInit IOADDR  %x", <eax>
	POPAD

	PUSHAD
	Debug_Printf	"hdlc98 Calling INITPART1"
	POPAD

	MOV	DX,IOBASE[EBX]		; SCC ORIGIN
	CALL	INITPART1
	
	PUSHAD
	Debug_Printf	"hdlc98 Calling INITCIO"
	POPAD


	PUSH	EDX
	CALL	INITCIO			; SET UP CIO FOR /32
	POP		EDX
	
	PUSHAD
	Debug_Printf	"hdlc98 Calling INITPART2"
	POPAD

	CALL	INITPART2
;
;	SET UP COUNTER UNLESS EXTERNAL CLOCK
;
	CMP	BAUDRATE[EBX],0
	JNE SHORT STARTCOUNTER

	JMP	EXTCLOCK
	
	PUSHAD
	Debug_Printf	"hdlc98 Starting BRG"
	POPAD


STARTCOUNTER:
;
	MOV	DX,IOBASE[EBX]
	ADD	DX,7			; TO CIO PORT
;
;	B CHANNEL
;
;	SET COUNTER OUTPUT BIT ACTIVE
;	
	MOV	AL,2BH			; PORT B DIRECTION
	OUT	DX,AL

	DELAY

	IN	AL,DX			; GET IT

	CMP	CHANNELNUM[EBX],'A'
	JE SHORT INITACOUNTER
;
	AND	AL,0EFH			; SET BIT 4 AS OUTPUT

	PUSH	EAX

	DELAY

	MOV	AL,2BH
	OUT	DX,AL

	DELAY

	POP	EAX

	DELAY

	OUT	DX,AL			; UPDATE PORT B DIRECTION

	DELAY
;
;	ENABLE COUNTER
;
	MOV	AL,1			; MASTER CONFIG
	OUT	DX,AL

	DELAY

	IN	AL,DX			; GET IT
	OR	AL,40H			; ENABLE CT1

	PUSH	EAX

	DELAY

	MOV	AL,1
	OUT	DX,AL

	DELAY

	POP	EAX

	DELAY

	OUT	DX,AL			; UPDATE MASTER CONFIG

	DELAY
;
;	START COUNTER
;
	MOV	AL,0AH			; CT1 CONTROL
	OUT	DX,AL

	DELAY

	MOV	AL,6
	OUT	DX,AL			; START CT1
	DELAY

;
	JMP SHORT EXTCLOCK

INITACOUNTER:

	AND	AL,0FEH			; SET BIT 0 AS OUTPUT

	PUSH	EAX

	DELAY

	MOV	AL,2BH
	OUT	DX,AL

	DELAY

	POP	EAX

	DELAY

	OUT	DX,AL			; UPDATE PORT B DIRECTION

	DELAY
;
;	ENABLE COUNTER
;
	MOV	AL,1			; MASTER CONFIG
	OUT	DX,AL

	DELAY

	IN	AL,DX			; GET IT
	OR	AL,20H			; ENABLE CT2

	PUSH	EAX

	DELAY

	MOV	AL,1
	OUT	DX,AL

	DELAY

	POP	EAX

	DELAY

	OUT	DX,AL			; UPDATE MASTER CONFIG

	DELAY
;
;	START COUNTER
;
	MOV	AL,0BH			; CT2 CONTROL
	OUT	DX,AL

	DELAY

	MOV	AL,6
	OUT	DX,AL			; START CT2
	DELAY

EXTCLOCK:

	PUSHAD
	Debug_Printf	"hdlc98 Calling INITREST"
	POPAD


	CALL	INITREST
	
	PUSHAD
	Debug_Printf	"hdlc98 DRSIInit Exit"
	POPAD

        xor      eax,eax

 	RET

RLC100INIT:

	MOV	AX,4			; BASE ADJUST
	CALL	CHECKCHAN		; SEE IF C OR D

	MOV	DX,IOBASE[EBX]		; SCC ORIGIN
	CALL	INITPART1
	CALL	INITPART2
	CALL	INITREST

 	RET

PC120INIT:

	MOV	DX,IOBASE[EBX]
	ADD	DX,4			; TO SCC ADDRESS

	CALL	INITPART1
	CALL	INITPART2
	CALL	INITMODEM
	CALL	INITREST
	RET

TOSHINIT:				; INTERRUPT INITIALISATION CODE

	MOV	DX,IOBASE[EBX]		; SCC ORIGIN
	CALL	INITPART1
;
;	SET UP ADDRESS LIST
;
	MOV	AX,IOBASE[EBX]
	MOV	BSIOC[EBX],AX		; B CHAN ADDR
	
	INC	AX

	MOV	ASIOC[EBX],AX		; A ADDR
;
;	SEE WHICH CHANNEL TO USE
;
	CMP	CHANNELNUM[EBX],'A'
	JE SHORT INITACHANT
;
;	B CHANNEL
;
	DEC	AX			; BACK TO B ADDRESSES

	MOV	B_PTR[EBX],EBX		; POINT TO OUR ENTRY

	CMP	EDI,0
	JE SHORT INITCOMT		; NO OTHER CHANNEL TO LINK

	MOV	A_PTR[EBX],EDI		; CROSSLINK CHANNELS
	MOV	B_PTR[EDI],EBX

	JMP SHORT INITCOMT

INITACHANT:

	MOV	A_PTR[EBX],EBX		; POINT TO OUR ENTRY

	CMP	EDI,0
	JE SHORT INITCOMT		; NO OTHER CHANNEL TO LINK

	MOV	B_PTR[EBX],EDI		; CROSSLINK CHANNELS
	MOV	A_PTR[EDI],EBX


INITCOMT:

	MOV	SIOC[EBX],AX
	INC	AX
	INC	AX
	MOV	SIO[EBX],AX
;
;	SET UP VECTORS
;
	MOV	IOTXCA[EBX],OFFSET32 IGNORE	; TX CHANNEL A
	MOV	IOTXEA[EBX],OFFSET32 EXTINT
	MOV	IORXCA[EBX],OFFSET32 SDADRX	; RX CHANNEL A
	MOV	IORXEA[EBX],OFFSET32 SPCLINT	
;
;
;	INITIALISE COMMS CHIP

	cli

	CMP	EDI,0			; OTHER CHAN ALREADY SET UP?
	JNE SHORT TSIOSECOND	; YES
;
;	DO A HARD RESET OF THE SCC
;
	MOV	DX,ASIOC[EBX]
	MOV	AL,0
	OUT	DX,AL
	DELAY
	OUT	DX,AL
	MOV	AL,9
	OUT	DX,AL
	DELAY
	MOV	AL,0C0H
	OUT	DX,AL

	MOV	ECX,256

TSIODELAY:

	DELAY

	LOOP	TSIODELAY

TSIOSECOND:

;
	MOV	DX,SIOC[EBX]
	MOV	ESI,OFFSET32 SDLCCMD	; PARAMS
	MOV	ECX,SDLCLEN

SIOLOOPT:

	LODS	SDLCCMD
	OUT	DX,AL

	DELAY

	LOOP	SIOLOOPT
;
;	SET UP BRG FOR REQUIRED SPEED
;
	MOV	DX,WORD PTR TOSHCLOCKFREQ+2
	MOV	AX,WORD PTR TOSHCLOCKFREQ

	DIV	BAUDRATE[EBX]

	SUB	AX,2			; GIVES BRG PARAM
	MOV	CX,AX			; SAVE

	MOV	DX,SIOC[EBX]
	MOV	AL,12
	OUT	DX,AL

	DELAY
	MOV	AL,CL
	OUT	DX,AL			; SET LSB

	DELAY
	MOV	AL,13
	OUT	DX,AL

	DELAY
	MOV	AL,CH
	OUT	DX,AL			; SET MSB

	MOV	AL,11
	OUT	DX,AL

	MOV	AL,TOSHR11		; REDEFINE TX CLOCK FROM BRG
	OUT	DX,AL

	STI

	CALL	INITREST
	RET

INITPART1:
;
;	SEE IF ANOTHER PORT IS ALREADY USING THE OTHER CHANNEL ON THIS CARD
;
	MOV	EDI,PORTTABLE
	MOV	ECX,NUMBEROFPORTS
INIT00:
	CMP	EDI,EBX
	JE SHORT INIT10			; NONE BEFORE OURS

	MOV	AX,IOBASE[EBX]
	CMP	AX,IOBASE[EDI]
	JE SHORT INIT20			; ANOTHER FOR SAME ADDRESS

	ADD		EDI,TYPE PORTCONTROL

	LOOP	INIT00
INIT10:
	MOV	EDI,0			; FLAG NOT FOUND
	JMP SHORT INIT30
INIT20:
;
;	ENSURE ENTRIES ARE FOR DIFFERENT CHANNELS
;
	MOV	AL,CHANNELNUM[EBX]
	CMP	AL,CHANNELNUM[EDI]
	JNE SHORT INIT30

INIT25:
;
;	CHANNEL DEFINITION ERROR
;
	IFDEF	PCSWITCH
	MOV	EDX,OFFSET32 ERRORMSG
	MOV	AH,9
	INT	21H
	ENDIF


	RET

INIT30:
;
;	MAKE SURE ONLY A OR B
;
	CMP	CHANNELNUM[EBX],'A'
	JE SHORT INIT31

	CMP	CHANNELNUM[EBX],'B'
	JNE SHORT INIT25

INIT31:
	RET

INITPART2:
;
;	SET UP ADDRESS LIST - THIS PATH FOR CARDS WITH 'NORMAL'
;	ADDRESSING - C/D=A0, A/B=A1, SO ORDER IS BCTRL BDATA ACTRL ADATA
;	OR DE, WHICH USES WORD ADDRESSES C/D=A1, A/B=A2 
;
	MOV	AX,DX

	MOV	BSIOC[EBX],AX		; B CHAN ADDR
	INC	AX
	INC	AX
	MOV	ASIOC[EBX],AX		; A CHAN ADDR

;
;	SEE WHICH CHANNEL TO USE
;
	CMP	CHANNELNUM[EBX],'A'
	JE SHORT INITACHAN
;
;	MUST BE B - CHECKED EARLIER
;
	DEC	AX
	DEC	AX			; BACK TO B ADDRESSES

	MOV	B_PTR[EBX],EBX		; POINT TO OUR ENTRY

	CMP	EDI,0
	JE SHORT INITCOM			; NO OTHER CHANNEL TO LINK

	MOV	A_PTR[EBX],EDI		; CROSSLINK CHANNELS
	MOV	B_PTR[EDI],EBX

	JMP SHORT INITCOM

INITACHAN:

	MOV	A_PTR[EBX],EBX		; POINT TO OUR ENTRY

	CMP	EDI,0
	JE SHORT INITCOM			; NO OTHER CHANNEL TO LINK

	MOV	B_PTR[EBX],EDI		; CROSSLINK CHANNELS
	MOV	A_PTR[EDI],EBX


INITCOM:

PINITREST:

	MOV	SIOC[EBX],AX

	INC	AX			; DATA 1 ABOVE CONTROL

	MOV	SIO[EBX],AX
;
;	SET UP VECTORS
;
	MOV	IOTXCA[EBX],OFFSET32 IGNORE	; TX CHANNEL A
	MOV	IOTXEA[EBX],OFFSET32 EXTINT
	MOV	IORXCA[EBX],OFFSET32 SDADRX	; RX CHANNEL A
	MOV	IORXEA[EBX],OFFSET32 SPCLINT	
;
;
;	INITIALISE COMMS CHIP
;

	PUSHAD
	Debug_Printf	"hdlc98 Initialising SCC"
	POPAD

	cli

	CMP	EDI,0			; OTHER CHAN ALREADY SET UP?
	JNE SHORT SIOSECOND	; YES
;
;	DO A HARD RESET OF THE SCC
;
	MOV	DX,ASIOC[EBX]
	MOV	AL,0
	OUT	DX,AL
	DELAY
	OUT	DX,AL
	MOV	AL,9
	OUT	DX,AL
	DELAY
	MOV	AL,0C0H
	OUT	DX,AL

	MOV	ECX,256

SIODELAY:

	DELAY

	LOOP	SIODELAY

SIOSECOND:

	MOV	DX,SIOC[EBX]
	MOV	ESI,OFFSET32 SDLCCMD	; PARAMS
	MOV	ECX,SDLCLEN

SIOLOOP:


	LODS	SDLCCMD
	OUT	DX,AL

	DELAY

	LOOP	SIOLOOP
;
	MOV	WR10[EBX],00100000B	; NRZI
;
;	SET UP BRG FOR REQUIRED SPEED
;
	CMP	BAUDRATE[EBX],0
	JNE SHORT NORMCLOCK
;
;	SET EXTERNAL CLOCK
;
	MOV	DX,SIOC[EBX]
	MOV	AL,11
	OUT	DX,AL

	DELAY
	MOV	AL,00100000B		; RX = TRXC TX = RTXC
	OUT	DX,AL			; SET WR11

	sti
	RET


NORMCLOCK:

	MOV	DX,WORD PTR CLOCKFREQ+2
	MOV	AX,WORD PTR CLOCKFREQ

	CMP	PORTTYPE[EBX],12		; RLC400
	JNE SHORT NOTRLC400		; NORMAL CLOCK SYSTEM
;
;	RLC 400 USES SAME CLOCK AS TOSH
;
	MOV	DX,WORD PTR TOSHCLOCKFREQ+2
	MOV	AX,WORD PTR TOSHCLOCKFREQ

NOTRLC400:

	DIV	BAUDRATE[EBX]

	SUB	AX,2			; GIVES BRG PARAM
	MOV	CX,AX			; SAVE

	MOV	DX,SIOC[EBX]
	MOV	AL,12
	OUT	DX,AL

	DELAY
	MOV	AL,CL
	OUT	DX,AL			; SET LSB

	DELAY
	MOV	AL,13
	OUT	DX,AL

	DELAY
	MOV	AL,CH
	OUT	DX,AL			; SET MSB

	DELAY
;
	MOV	AL,01110000B		; RXC=DPLL, TXC=BRG
	OUT	DX,AL

	sti
	
	RET


INITCIO:
;
;	INITIALISE CIO - DRSI ONLY
;
	CMP	EDI,0
	JNE SHORT CIODONE			; ALREADY SET UP

	MOV	DX,IOBASE[EBX]
	ADD	DX,7			; TO CIO PORT

	IN	AL,DX
	MOV	AL,0
	DELAY
	OUT	DX,AL
	DELAY

	IN	AL,DX
	MOV	AL,0
	DELAY
	OUT	DX,AL
	DELAY

	MOV	AL,1
	OUT	DX,AL			; FORCE RESET
	DELAY

	MOV	AL,0
	OUT	DX,AL			; CLEAR RESET
	DELAY
;

	MOV	ESI,OFFSET32 CIOPARAMS	; PARAMS
	MOV	ECX,CIOLEN
CIOLOOP:
	LODS	CIOPARAMS
	OUT	DX,AL

	DELAY

	LOOP	CIOLOOP
;
CIODONE:
	RET

INITMODEM:
;
;	SETUP MODEM - PC120 ONLY
;
	CMP	EDI,0
	JNE SHORT MODDONE			; ALREADY SET UP

	MOV	DX,IOBASE[EBX]		; BASE ADDR OF CARD
	MOV	AL,0AH
	OUT	DX,AL			; SET MODEM CONTROL LATCH

MODDONE:
	RET
;
;	BAYCOM CARD
;
;SCC	DB	0

CHECKCHAN:
;
;	IF CHANNEL =  C OR D SET TO SECOND SCC ADDRESS, AND CHANGE TO A OR B
;
; 	MOV	SCC,1
	CMP	CHANNELNUM[EBX],'B'
	JBE SHORT CHANOK
;
;	SECOND SCC
;
;	MOV	SCC,2
	SUB	CHANNELNUM[EBX],2
	ADD	IOBASE[EBX],AX

CHANOK:
	RET


BAYCOMINIT:

	MOV	AX,2			; BASE ADJUST
	CALL	CHECKCHAN		; SEE IF C OR D

	MOV	DX,IOBASE[EBX]		; SCC ORIGIN
	CALL	INITPART1
	CALL	BINITPART2
	CALL	INITREST
	RET

BINITPART2:
;
;	SET UP ADDRESS LIST
;
	MOV	AX,DX

	ADD	AX,4
	MOV	ASIOC[EBX],AX		; A CHAN ADDR
	INC	AX
	MOV	BSIOC[EBX],AX		; B CHAN ADDR
;
;	SEE WHICH CHANNEL TO USE
;
	MOV	AX,DX			; A CHAN DATA

	CMP	CHANNELNUM[EBX],'A'
	JE SHORT BINITACHAN
;
;	MUST BE B - CHECKED EARLIER
;
	INC	AX			; TO B DATA

	MOV	B_PTR[EBX],EBX		; POINT TO OUR ENTRY

	CMP	EDI,0
	JE SHORT BINITCOM		; NO OTHER CHANNEL TO LINK

	MOV	A_PTR[EBX],EDI		; CROSSLINK CHANNELS
	MOV	B_PTR[EDI],EBX

	JMP SHORT BINITCOM

BINITACHAN:

	MOV	A_PTR[EBX],EBX		; POINT TO OUR ENTRY

	CMP	EDI,0
	JE SHORT BINITCOM		; NO OTHER CHANNEL TO LINK

	MOV	B_PTR[EBX],EDI		; CROSSLINK CHANNELS
	MOV	A_PTR[EDI],EBX


BINITCOM:

	MOV	SIO[EBX],AX
	ADD	AX,4			; TO CONTROL
	MOV	SIOC[EBX],AX
;
;	SET UP VECTORS
;
	MOV	IOTXCA[EBX],OFFSET32 IGNORE	; TX CHANNEL A
	MOV	IOTXEA[EBX],OFFSET32 EXTINT
	MOV	IORXCA[EBX],OFFSET32 SDADRX	; RX CHANNEL A
	MOV	IORXEA[EBX],OFFSET32 SPCLINT	
;
;	INITIALISE COMMS CHIP
;
	cli

	CMP	EDI,0			; OTHER CHAN ALREADY SET UP?
	JNE SHORT BSIOSECOND	; YES
;
;	DO A HARD RESET OF THE SCC
;
	MOV	DX,ASIOC[EBX]
	MOV	AL,0
	OUT	DX,AL
	DELAY
	OUT	DX,AL
	MOV	AL,9
	OUT	DX,AL
	DELAY
	MOV	AL,0C0H
	OUT	DX,AL

	MOV	ECX,256

BSIODELAY:

	DELAY

	LOOP	BSIODELAY

BSIOSECOND:

	MOV	DX,SIOC[EBX]
	MOV	ESI,OFFSET32 SDLCCMD	; PARAMS
	MOV	ECX,SDLCLEN

BSIOLOOP:

	LODS	SDLCCMD
	OUT	DX,AL

	DELAY

	LOOP	BSIOLOOP
;
;	SET UP BRG FOR REQUIRED SPEED
;
	CMP	BAUDRATE[EBX],0
	JNE SHORT BNORMCLOCK
;
;	SET EXTERNAL CLOCK
;
	MOV	DX,SIOC[EBX]
	MOV	AL,11
	OUT	DX,AL

	DELAY
	MOV	AL,00100000B		; RX = TRXC TX = RTXC
	OUT	DX,AL			; SET WR11
;
;	BAYCOM RUH PORT USES NRZ
;
	MOV	WR10[EBX],00000000B	; NRZ

	STI
	
	RET


BNORMCLOCK:
;
	MOV	WR10[EBX],00100000B	; NRZI
;
;	THERE IS NO /32 ON THE BAYCOM BOARD, SO FOR THE MOMENT WILL USE BRG
;	FOR TRANSMIT. THIS REQUIRES IT TO BE REPROGRAMMED BETWEEN TX AND RX,
;	AND SO PREVENTS LOOPBACK OR FULLDUP OPERATION
;

	MOV	DX,WORD PTR CLOCKFREQ+2
	MOV	AX,WORD PTR CLOCKFREQ

	DIV	BAUDRATE[EBX]

	SUB	AX,2			; GIVES BRG PARAM
	MOV	CX,AX			; SAVE

	MOV	RXBRG[EBX],AX
;
;	CALC TX RATE
;
	ADD	AX,2
	SHL	AX,1
	SHL	AX,1
	SHL	AX,1
	SHL	AX,1
	SHL	AX,1			; *32
	SUB	AX,2

	MOV	TXBRG[EBX],AX

	MOV	DX,SIOC[EBX]
	MOV	AL,12
	OUT	DX,AL

	DELAY
	MOV	AL,CL
	OUT	DX,AL			; SET LSB

	DELAY
	MOV	AL,13
	OUT	DX,AL

	DELAY
	MOV	AL,CH
	OUT	DX,AL			; SET MSB
;
;	IF 7910/3105 PORTS, SET TXC=BRG, RXC=DPLL
;
	MOV	DX,SIOC[EBX]
	MOV	AL,11
	OUT	DX,AL

	DELAY
;	MOV	AL,00000110B		; TXC=RXC=RTXC, TRXC=BRG 

;	CMP	SCC,2
;	JE SHORT SET11
;
;	IT SEEMS THE 3RD PORT IS MORE LIKELY TO BE USED WITH A SIMPLE
;	MODEM WITHOUT CLOCK GERERATION (EG BAYCOM MODEM), SO SET ALL 
;	PORTS THE SAME
;
;
;	FIRST SCC
;
	MOV	AL,01110000B		; RXC=DPLL, TXC=BRG
	OUT	DX,AL

	STI

	RET

SET11:
;
;	SECOND SCC - SET FOR NORMAL EXT CLOCKS
;
;	OUT	DX,AL
;
;	MOV	TXBRG[EBX],0		; DONT WANT TO CHANGE	
;	MOV	RXBRG[EBX],0
;
;	STI
;	RET
;

PA0INIT:

	MOV	AX,4			; BASE ADJUST
	CALL	CHECKCHAN		; SEE IF C OR D

	MOV	DX,IOBASE[EBX]		; SCC ORIGIN
	CALL	INITPART1
	CALL	INITPART2
	CALL	INITREST
	RET



INITREST:

	mov     IRQHand[EBX],0	; in case already hooked by another port
	
	MOV	PORTINTERRUPT[EBX],OFFSET32 SIOINT

	CMP	EDI,0
	JNE SHORT INTDONE		; ALREADY SET UP

	CALL	HOOKINT			; INTERRUPT

INTDONE:

	CALL	RXAINIT

	SIOCR
	MOV	RR0[EBX],AL		; GET INITIAL RR0
;
	RET




EndProc VXD_IOCONTROL


VxD_CODE_ENDS



;******************************************************************************
;                      P A G E   L O C K E D   C O D E
;------------------------------------------------------------------------------
;       Memory is a scarce resource. Use this only where necessary.
;******************************************************************************

VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VXD_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation. Every VxD needs this function
;       regardless if messages are processed or not. The control proc must
;       be in the LOCKED code segment.
;
;       The Control_Dispatch macro used in this procedure simplifies
;       the handling of messages. To handle a particular message, add
;       a Control_Dispatch statement with the message name, followed
;       by the procedure that should handle the message. 
;
;       The two messages handled in this sample control proc, Device_Init
;       and Create_VM, are done only to illustrate how messages are
;       typically handled by a VxD. A VxD is not required to handle any
;       messages.
;
;   ENTRY:
;       EAX = Message number
;       EBX = VM Handle
;
;==============================================================================

BeginProc BPQVXD_Control

    Control_Dispatch Device_Init, VXD_Device_Init
    Control_Dispatch Create_VM, VXD_Create_VM
    Control_Dispatch SYS_CRITICAL_INIT, BPQVXD_SYS_CRITICAL_INIT
    Control_Dispatch W32_DEVICEIOCONTROL, VXD_IOCONTROL
    Control_Dispatch SYS_DYNAMIC_DEVICE_INIT,VXD_INIT
    Control_Dispatch SYS_DYNAMIC_DEVICE_EXIT,VXD_EXIT
    clc
	ret

EndProc BPQVXD_Control


BeginProc VXD_INIT

	PUSHAD
	Debug_Printf	"hdlc98 SYS_DYNAMIC_DEVICE_INIT"
	POPAD
	
	clc
	ret


EndProc VXD_INIT
       
BeginProc VXD_EXIT

	PUSHAD
	Debug_Printf	"hdlc98 SYS_DYNAMIC_DEVICE_EXIT"
	POPAD
	
	MOV	ebx,PORTTABLE
	MOV	ecx,NUMBEROFPORTS

exit00:

	push	ecx
	push	ebx
;
;	make sure RTS is dropped
;
	cli

	MOV	AL,5
	SIOCW
	DELAY
	MOV	AL,0
	SIOCW   

	MOV	AL,9
	SIOCW
	DELAY
	MOV	AL,0C0H			; FORCE RESET (BOTH CHANNELS)
	SIOCW   

	sti

	mov     eax, IRQHand[EBX]
	cmp		eax,0
	je	exit10					; Not hooked to this port
	
	VxDcall VPICD_Physically_Mask

	mov     eax, IRQHand[EBX]
	VxDcall VPICD_Force_Default_Behavior

exit10:

    xor     esi, esi
	xchg    TimeOut_Handle[EBX], esi   ; time-out handle
	VMMcall Cancel_Time_Out

	pop	ebx
	pop	ecx

	add		ebx,TYPE PORTCONTROL
	loop	exit00

	clc
	ret


RXAINIT:

	SETRVEC	SDADRX

	MOV	AL,33H
	SIOCW				; ERROR RESET WR3

	DELAY
	MOV	AL,11011001B		; 8BIT, CRC EN, RX EN
	SIOCW   
;
	RET


TXDTIMER:
;
;	CALLED BY TIMER CALLBACK AFTER REQUIRED TIME
;
;	TXDELAY HAS EXPIRED - START SENDING
;
	mov	ebx,edx			; Reference data = Port vector

	mov	TimeOut_Handle[EBX],0

	MOV	AL,SDTXCRC
	SIOCW				; RESET TX CRC GENERATOR
;
	DELAY
	MOV	AL,FIRSTCHAR[EBX]
	SIOW

	CLI
	DELAY
	MOV	AL,SDTXUND
	SIOCW				; RESET TX UNDERRUN LATCH

	OR	SDFLAGS[EBX],SDTINP
	AND	RR0[EBX],NOT SDUNDER

	STI

	RET

;
HDLCTIMER:
;
;	RUN FROM 10 HZ CALL FROM DLL
;
;	RECORD CHANNEL ACTIVITY
;
	CMP	LINKSTS[EBX],0
	JE SHORT NOTSENDING

	INC	SENDING[EBX]

	JMP SHORT NOTDCD

NOTSENDING:

	SIOCR				; GET RR0

	CMP	SOFTDCDFLAG[EBX],0
	JE SHORT CHECKRR0_X

	TEST	AL,10H			; SYNC BIT
	JZ SHORT DCDHIGH_X

	JMP SHORT NOTDCD

CHECKRR0_X:

	TEST	AL,08H			; HARD DCD
	JZ SHORT NOTDCD

DCDHIGH_X:

	INC	ACTIVE[EBX]

NOTDCD:

	CMP	L1TIMEOUT[EBX],0
	JE SHORT TIMERRET

	DEC	L1TIMEOUT[EBX]
	JNZ SHORT TIMERRET
;
;	WE HAVE BEEN UNABLE TO TRANSMIT FOR 60 SECS - CLEAR TX Q
;
	PUSH	ESI
	PUSH	EDI

CLEARLOOP:

	LEA	ESI,PCTX_Q[EBX]

	CLI
	CALL	Q_REM
	STI

	JZ SHORT DISCARDEND

	ADD	L1DISCARD[EBX],1		; FOR STATS
	ADC	L1DISCARD+2[EBX],0
;
;	MOV	ESI,OFFSET32 TRACE_Q
	MOV	ESI,OFFSET32 FREE_Q

	CLI
	CALL	Q_ADDF
	STI

	JMP	CLEARLOOP

DISCARDEND:
;
;	IF IN TX STATE, RESET SCC - INDICATES STUCK IN SEND MODE
;
	CMP	LINKSTS[EBX],0
	JE SHORT NOTSTUCK
;
;	IF A FRAME IS BEING SENT, RELEASE IT
;
 	MOV	EDI,TXFRAME[EBX]
	MOV	TXFRAME[EBX],0

	CMP	EDI,0
	JE SHORT NO_STUCK_FRAME

	MOV	ESI,OFFSET32 FREE_Q
;	MOV	ESI,OFFSET32 TRACE_Q	; TRACE IT
	CALL	Q_ADDF

NO_STUCK_FRAME:

	CLI
	CALL	DROPRTS			; CLEAR DOWN SCC
	STI

NOTSTUCK:

	POP	EDI
	POP	ESI

TIMERRET:
;
;	DO CWID BITS
;
;	cmp	CWSTATE[EBX],0
;	jne	CWIDTIMRET			; busy doing an id

	dec	CWIDTIMER[EBX]
	jnz SHORT CWIDTIMRET


;	CHANGE TO NEXT STATE
;
	test	CWSTATE[EBX],dot + dash
	jnz SHORT senddotspace

	test	CWSTATE[EBX],letterspace
	jnz sendnextletter

	test	CWSTATE[EBX],dotspace
	jnz SHORT sendnextelement
;

;	NEED A NEW ID
;
	cmp	cwid[Ebx],0
	je SHORT CWIDTIMRET		; NO ID DEFINED

	CMP	PORTDISABLED[EBX],0
	JNE SHORT DISABLED
;
	MOV	CWSTATE[EBX],IDPENDING

CWIDTIMRET:
	ret

DISABLED:
	MOV	CWIDTIMER[EBX],10*60	; TRY AGAIN IN A MINUTE
	RET


	public	cwfasttimer,sendnextelement,senddotspace,sendnextletter


cwfasttimer:
;
;	Called from VMM timer - CHANGE TO NEXT STATE
;
	mov	ebx,edx			; Reference data = Port vector

	mov	TimeOut_Handle[EBX],0

	test	CWSTATE[EBX],dot + dash
	jnz SHORT senddotspace

	test	CWSTATE[EBX],letterspace
	jnz SHORT sendnextletter

	test	CWSTATE[EBX],dotspace
	jnz SHORT sendnextelement
;
	ret						; Finished

StartCWIDTimer:
;
;	Start a timer to do next state change
;

	movzx   eax,CWIDTIMER[EBX]
	mul		B20					; 20 ms intervals

	mov     edx, EBX
	mov     esi, offset cwfasttimer

	cli

	VMMcall Set_Async_Time_Out
	mov     TimeOut_Handle[EBX], esi
	
	sti

	RET


senddotspace:

	mov	CWIDTIMER[EBX],1
	mov	CWSTATE[EBX],dotspace
	jmp 	HIGHTONE


sendnextelement:

	mov	ax,ELEMENT[EBX]
	mov	cl,2
	shr	ELEMENT[EBX],cl

	and	al,3
	jz SHORT sendletspace
;
;	1 = dot, 3 = dash
;
	CBW
	mov	CWIDTIMER[EBX],ax
	mov	CWSTATE[EBX],dot

	jmp 	LOWTONE

sendletspace:

	mov	CWSTATE[EBX],letterspace
	mov	CWIDTIMER[EBX],2
	jmp	HIGHTONE

sendnextletter:

	PUSH	ESI
	mov	Esi,CWPOINTER[EBX]
	lodsw
	mov	CWPOINTER[EBX],Esi
	POP	ESI

	mov	ELEMENT[EBX],ax
	or	ax,ax
	jnz	sendnextelement
;
;	finished - set to repeat later
;
	lea	Eax,CWID[EBX]
	mov	CWPOINTER[EBX],EAX
	mov	CWIDTIMER[EBX],10*29*60
	MOV	CWSTATE[EBX],0

	CLI

	MOV	AL,5
	SIOCW
	DELAY
	MOV	AL,11100001B		; DROP RTS
	SIOCW
;
	MOV	LINKSTS[EBX],0

	STI

	ret

senddotdash:

	cbw
	mov	CWIDTIMER[EBX],ax
	mov	CWSTATE[EBX],dot
	jmp	LOWTONE




HDLC_CHECK_TX:
;
;	SEE IF WAITING FOR DCD TO DROP BEFORE SENDING
;
	CMP	FULLDUPLEX[EBX],1
	JE SHORT DCDLOW			; DONT CHECK IF FULL DUPLEX

	SIOCR				; GET RR0

	CMP	SOFTDCDFLAG[EBX],0
	JE SHORT CHECKRR0

	TEST	AL,10H			; SYNC BIT
	JZ 	DCDHIGH

	JMP SHORT  DCDLOW

CHECKRR0:

	TEST	AL,08H			; HARD DCD
	JNZ 	DCDHIGH

DCDLOW:
;
;	IF ANYTHING QUEUED, AND NOT TRANSMITTING, START TX
;
	TEST	CWSTATE[EBX],IDPENDING
	JNZ SHORT NEED_ID

	CMP	PCTX_Q[EBX],0
	JE 	DCDHIGH			; NOTHING QUEUED

NEED_ID:

	CMP	LINKSTS[EBX],0
	JNE 	DCDHIGH			; RETURN
;
;	CALLED EVERY 100 MS (I HOPE) - THIS IS NEAR ENOUGH TO SLOTTIME!!
;
;	SEE IF OUR TURN!
;
	MOV	AL,RANDOM

	CMP	PORTPERSISTANCE[EBX],AL
	JNC SHORT OURTURNTOSEND

	JMP SHORT DCDHIGH

OURTURNTOSEND:
;
;	CHECK FOR PORT INTERLOCK
;
	CMP	PORTINTERLOCK[EBX],0
	JE SHORT NO_INTERLOCK		; NOT SET

	MOV	AL,PORTINTERLOCK[EBX]

	MOV	EDI,PORTTABLE
	MOV	ECX,NUMBEROFPORTS

LOCK00:

	CMP	PORTINTERLOCK[EDI],AL
	JNE SHORT LOCK40			; NOT SAME SET

	CMP	LINKSTS[EDI],0
	JNE SHORT DCDHIGH			; ANOTHER PORT IN SAME SET IS ACTIVE

LOCK40:

	add		EDI,TYPE PORTCONTROL
	
	LOOP	LOCK00
;
;	NO OTHER PORT IN SAME SET IS ACTIVE - OK TO SEND
;
NO_INTERLOCK:
;
	
	MOV	L1TIMEOUT[EBX],0		; STOP DCD TIMEOUT
	TEST	CWSTATE[EBX],IDPENDING
	JNZ SHORT STARTID

	CLI

	LEA	ESI,PCTX_Q[EBX]
	CALL	Q_REM
	JZ SHORT DCDHIGH			; NOTHING THERE??

	STI
	JMP	SENDFRAME

DCDHIGH:

	STI
	RET

STARTID:
;
;	KICK OFF CW ID SEQUENCE
;
	MOV	LINKSTS[EBX],1		; SET ACTIVE

	MOV	CWIDTIMER[EBX],3		; KEYUP TIME (300 ms)
	MOV	CWSTATE[EBX],LETTERSPACE	; START NEW LETTER ON TIMEOUT 

	CALL	HIGHTONE

	RET
;
;	TONE CONTROL ROUTINES
;
LOWTONE:				; USED AS MARK ON FSK SYSTEMS

	CLI

	MOV	AL,5
	SIOCW
	DELAY

	MOV	AL,11110011B		; RAISE RTS, SEND BREAK, DROP TXENABLE
	SIOCW
;
	STI

;	call	StartCWIDTimer
	
	ret

HIGHTONE:

	CLI

	MOV	AL,5
	SIOCW
	DELAY

	CMP	CWTYPE[EBX],'O'		; IF ONOFF KEYING, DROP RTS
	JE SHORT KEYUP

	MOV	AL,11100011B		; TXEN OFF BREAK OFF RTS
	SIOCW
;
	STI

;	call	StartCWIDTimer

	RET

KEYUP:

	MOV	AL,11100001B		; DROP RTS
	SIOCW
;
	STI

;	call	StartCWIDTimer

	RET


SENDFRAME:
;
	MOV	TXFRAME[EBX],EDI		; SAVE ADDRESS OF FRAME

	MOV	AX,5[EDI]
	SUB	AX,8
	MOV	SDTXCNT[EBX],AX			; GET MESSAGE LENGTH FROM BUFFER

	LEA	EDI,8[EDI]
	MOV	SDTNEXT[EBX],EDI		; SET NEXT BYTE POINTER
;
	OR	LINKSTS[EBX],1		; SET LINK ACTIVE

	SETTVEC	SDDTTX			; SET VECTOR TO 'TX DATA'

	CMP	TXBRG[EBX],0
	JE SHORT DONTCHANGE
;
;	NEED TO RESET BRG FOR TRANSMIT
;
	MOV	CX,TXBRG[EBX]

	MOV	DX,SIOC[EBX]
	MOV	AL,12

	CLI

	OUT	DX,AL

	DELAY
	MOV	AL,CL
	OUT	DX,AL			; SET LSB

	DELAY
	MOV	AL,13
	OUT	DX,AL

	DELAY
	MOV	AL,CH
	OUT	DX,AL			; SET MSB
;
	STI

	NOP	

DONTCHANGE:

	CLI

	MOV	AL,5
	SIOCW
	DELAY

	MOV	AL,11101011B		; RAISE RTS TO START SENDING FLAGS
	SIOCW
;
	MOV	AL,[EDI-1]		; FIRST BYTE
	MOV	FIRSTCHAR[EBX],AL	; SAVE

	MOV	AL,10
	DELAY
	SIOCW   

	DELAY
	MOV	AL,10000100B		; SET TO ABORT ON UNDERRUN
	OR	AL,WR10[EBX]
	SIOCW   

	STI
;
;	Start TXDelay Timer
;
	movzx   eax,PORTTXDELAY[EBX]
	mov     edx, EBX
	mov     esi, offset TXDTIMER
	VMMcall Set_Async_Time_Out
	mov     TimeOut_Handle[EBX], esi

	RET

SDADRX:

;	MOV	SOFTDCD[EBX],4		; SET RX ACTIVE

	MOV	ESI,OFFSET32 FREE_Q
	cli
	CALL	Q_REM
	sti
	JNZ SHORT GETB00

;	CALL	NOBUFFERCHECK		; CHECK IF POOL IS CORRUPT
	JMP SHORT NOBUFFERS

GETB00:

	DEC	QCOUNT

	MOV	DWORD PTR [EDI],OFFSET32 GETB00	; FLAG FOR DUMP ANALYSER

	SIOR				; GET FIRST BYTE OF ADDRESS

	MOV	CURALP[EBX],EDI		; SAVE ADDR
	ADD	EDI,7
	MOV	[EDI],AL			; ADDR TO BUFFER
	INC	EDI
	MOV	SDRNEXT[EBX],EDI		; AND NEXT BYTE POINTER
;
	MOV	FRAMELEN[EBX],7

	OR	SDFLAGS[EBX],SDRINP	; SET RX IN PROGRESS

	SETRVEC	SDIDRX			; SET VECTOR TO 'GET DATA'

	RET
;
NOBUFFERS:
;
;	NO BUFFER FOR RECEIVE - SET TO DISCARD
;
	INC	OLOADS[EBX]

	SETRVEC	SDOVRX
	SIOR				; CLEAR INT PENDING
	RET

SDIDRX:
;
;	NOW READ CHARACTER FROM SIO AND STORE
;
;	MOV	SOFTDCD[EBX],4		; SET RX ACTIVE
	INC	FRAMELEN[EBX]
;
	SIOR	SIO1			; GET CHAR
;
	CMP	FRAMELEN[EBX],BUFFLEN-10
	JNE SHORT SDID00			; SIZE OK
	SETRVEC	SDOVRX			; CANT TAKE ANY MORE
SDID00:
	MOV	EDI,SDRNEXT[EBX]		; GET NEXT BYTE POINTER
	STOSB
	MOV	SDRNEXT[EBX],EDI		; STORE UPDATED BYTE POINTER

	RET
;
SDOVRX:
;
;	DISCARD REST OF MESSAGE
;
;	MOV	SOFTDCD[EBX],4		; SET RX ACTIVE
	SIOR	SIO1			; READ CHAR AND DISCARD
	RET

;***	RX SPECIAL CHARACTER INTERRUPT
;
SPCLINT:
	
	SIOR				; READ CHAR AND DISCARD
	MOV	AL,1
	DELAY

	SIOCW				; SELECT RR1
	DELAY

	SIOCR				; INPUT RR1

	MOV	AH,AL

	SIOCAD
	MOV	AL,30H
	OUT	DX,AL			; RESET SIO ERROR LATCHES

	TEST	AH,10000000B		; END OF FRAME?
	JNZ SHORT SDEOF			; YES
;
;	NOT END OF FRAME - SHOULD BE OVERRUN - DISCARD FRAME
;
	INC	L2ORUNC[EBX]		; FOR STATS
;
	TEST	SDFLAGS[EBX],SDRINP
	JZ SHORT SDSP07			; IF NOT SET, NO BUFFER IS ALLOCATED

	AND	SDFLAGS[EBX],NOT SDRINP

	MOV	ESI,OFFSET32 FREE_Q
	MOV	EDI,CURALP[EBX]
	CALL	Q_ADDF

SDSP07:
	SETRVEC SDOVRX			; DISCARD REST OF MESSAGE

	JMP	SPCLINTEXIT

SDEOF:
;
;	IF RESIDUE IS NONZERO, IGNORE FRAME
;
	MOV	AL,AH
	AND	AL,1110B		; GET RESIDUE BITS
	CMP	AL,0110B
;	JNE SHORT DONTCOUNT		; NOT MULTIPLE OF 8 BITS
;
;	END OF FRAME - SEE IF FCS OK
;
	TEST	AH,01000000B
	JZ SHORT SDSP10			; J IF GOOD END-OF-FRAME
;
;	FCS ERROR
;
;	CMP	FRAMELEN[EBX],14H
;	JB SHORT DONTCOUNT		; TOO SHORT
	ADD	RXERRORS[EBX],1

DONTCOUNT:

	TEST	SDFLAGS[EBX],SDRINP
	JZ SHORT SDSP09			; IF NOT SET, NO BUFFER IS ALLOCATED

	AND	SDFLAGS[EBX],NOT SDRINP

DISCARDFRAME:

	MOV	ESI,OFFSET32 FREE_Q
	MOV	EDI,CURALP[EBX]
	CALL	Q_ADDF

SDSP09:
	SETRVEC SDADRX

	JMP SHORT SPCLINTEXIT
;
;	GOOD FRAME RECEIVED
;
SDSP10:
	TEST	SDFLAGS[EBX],SDRINP
	JZ SHORT SDSP11			; IF NOT SET, NO BUFFER IS ALLOCATED

	AND	SDFLAGS[EBX],NOT SDRINP

	MOV	EDI,SDRNEXT[EBX]		; GET NEXT BYTE POINTER
	DEC	EDI
	MOV	BYTE PTR [EDI],0FEH	; OVERWRITE FIRSTS FCS BYTE WITH MARKER

	MOV	AX,FRAMELEN[EBX]		; GET LENGTH
	CMP	AX,14H
	JB SHORT DISCARDFRAME		; TOO SHORT


	MOV	EDI,CURALP[EBX]		; BUFFER ADDR
	MOV	5[EDI],AX		; PUT IN LENGTH

	LEA	ESI,RXMSG_Q[EBX]
	CALL	Q_ADD			; QUEUE MSG FOR B/G
SDSP11:
	SETRVEC SDADRX			; READY FOR NEXT FRAME

SPCLINTEXIT:
	RET

;	EXTERNAL/STATUS INTERRUPT
;
;		 - TRANSMIT UNDERRUN/EOM - IF AT END OF FRAME IGNORE
;					      ELSE SEND ABORT
;		 - ABORT START/END - IF RECEIVING, CANCEL MESSAGE
;
;
EXTINT:

	MOV	AH,RR0[EBX]		; GET OLD RR0
	SIOCR
	MOV	RR0[EBX],AL		; SAVE

	XOR	AH,AL			; GET CHANGES
	AND	AH,0C0H
	JZ SHORT SDST40			; NO INTERESTING CHANGES

SDST00:
	TEST	SDFLAGS[EBX],SDTINP
	JZ SHORT SDST10			; J IF 'TX IN PROGRESS' NOT SET
;
;	WE ARE TRANSMITTING - CHECK FOR UNDERRUN
;
	TEST	AL,SDUNDER		; TX UNDERRUN?
	JZ SHORT SDST10			; ONLY INTERESTING ONE

	TEST	SDTXCNT[EBX],8000H	; IS IDP CHAR COUNT -VE?
	JNZ SHORT SDST10			; J IF YES (NORMAL END-OF-FRAME)
;
;	UNDERRUN IN MID-FRAME
;	ABORT THE TRANSMISSION
;
	MOV	AL,SDABTX
	SIOCW				; SEND ABORT SEQUENCE
	SETTVEC	SDCMTX			; SET VECTOR TO 'TX COMPLETE'
	OR	SDFLAGS[EBX],ABSENT	; SET ABORT SENT

	ADD	L2URUNC[EBX],1		; UNDERRUNS

	JMP SHORT SDST10		; SEE IF ANY RX ERROR BITS ALSO SET
;
;	IS RX IN PROGRESS?
;
SDST10:
	TEST	SDFLAGS[EBX],SDRINP
	JZ SHORT SDST40			; NO, SO PROBABLY ABORT FOLLOWING MSG 

	TEST	RR0[EBX],SDABORT		; IS ABORT STATUS BIT SET?
	JZ SHORT SDST40			; J IF NOT - ? ABORT TERMINATON

	MOV	EDI,CURALP[EBX]		; BUFFER ADDR

	MOV	ESI,OFFSET32 FREE_Q
	CALL	Q_ADDF			; RELEASE BUFFER

	AND	SDFLAGS[EBX],NOT SDRINP	; CLEAR RX IN PROGRESS

	SETRVEC SDADRX			; READY FOR NEXT FRAME

SDST40:
	MOV	AL,SDEXTR
	SIOCW				; RESET EXTERNAL/STATUS INTERRUPT

	RET
;
;***	TX DATA
;
SDDTTX:
	
	DEC	SDTXCNT[EBX]		; DECREMENT CURRENT IDP COUNT
;
	JNS SHORT SDDT40			; J IF IDP NOT YET EMPTY
;
;	NO MORE DATA TO TRANSMIT
;
	MOV	AL,10
	SIOCW   
	DELAY
	MOV	AL,10000000B		; SET TO SEND CRC ON UNDERRUN
	OR	AL,WR10[EBX]
	SIOCW   

	MOV	AL,SDRPEND		; RESET TX UNDERRUN/EOM LATCH +
	DELAY
	SIOCW				;       TX INTERRUPT PENDING
	SETTVEC	SDCMTX			; SET TX VECTOR TO 'TX COMPLETION'

	RET
;
;	TRANSMIT NEXT BYTE OF DATA
;
SDDT40:
	MOV	ESI,SDTNEXT[EBX]		; GET NEXT BYTE POINTER
	LODSB
	SIOW				; TRANSMIT NEXT BYTE
	MOV	SDTNEXT[EBX],ESI		; STORE UPDATED BYTE POINTER

	RET
;
;	*** F11 - TX COMPLETION
;
SDCMTX:
	
	MOV	AL,28H
	SIOCW				; RESET TX INT PENDING

	TEST	SDFLAGS[EBX],ABSENT
	JZ SHORT NOABORT
;
;	FRAME WAS ABORTED - SEND AGAIN
;
;	AND	SDFLAGS[EBX],NOT ABSENT
;	MOV	EDI,TXFRAME[EBX]
;	JMP SHORT SENDAGAIN

NOABORT:

	MOV	EDI,TXFRAME[EBX]
	MOV	TXFRAME[EBX],0

;	MOV	ESI,OFFSET32 TRACE_Q	; TRACE IT
	MOV	ESI,OFFSET32 FREE_Q	; TRACE IT
	CALL	Q_ADDF

	AND	SDFLAGS[EBX],NOT SDTINP
;
;	SEE IF MORE TO SEND
;
	LEA	ESI,PCTX_Q[EBX]
	CALL	Q_REM

	JZ	NOMORETOSEND		; J IF QUEUE EMPTY

	MOV	TXFRAME[EBX],EDI		; SAVE ADDRESS OF FRAME

SENDAGAIN:

	MOV	AX,5[EDI]
	SUB	AX,8
	MOV	SDTXCNT[EBX],AX			; GET MESSAGE LENGTH FROM BUFFER

	LEA	EDI,8[EDI]
	MOV	SDTNEXT[EBX],EDI		; SET NEXT BYTE POINTER
;
	OR	SDFLAGS[EBX],SDTINP

	SETTVEC	SDDTTX			; SET VECTOR TO 'TX DATA'

	MOV	AL,SDTXCRC
	SIOCW				; RESET TX CRC GENERATOR
;
	DELAY
	MOV	AL,[EDI-1]		; FIRST BYTE
	SIOW				; TRANSMIT IT

	DELAY
	MOV	AL,SDTXUND
	SIOCW				; RESET TX UNDERRUN LATCH

	AND	RR0[EBX],NOT SDUNDER	; KEEP STORE COPY

	DELAY
	MOV	AL,10
	SIOCW   

	DELAY
	MOV	AL,10000100B		; SET TO ABORT ON UNDERRUN
	OR	AL,WR10[EBX]
	SIOCW   

	RET

NOMORETOSEND:
;
;	SEND A FEW PADDING CHARS TO ENSURE FLAG IS CLEAR OF SCC
;
	SETTVEC	SENDDUMMY1

	XOR	AL,AL			; FIRST DUMMY BYTE
	SIOW	SIO1			; TRANSMIT IT

	RET

SENDDUMMY1:

	XOR	AL,AL
	SIOW				; SECOND DUMMY

	SETTVEC	SENDDUMMY2

	RET

SENDDUMMY2:

	XOR	AL,AL
	SIOW				; SECOND DUMMY

	SETTVEC	SENDDUMMY3

	RET

SENDDUMMY3:

	XOR	AL,AL
	SIOW				; SECOND DUMMY

	SETTVEC	SENDDUMMY4

	RET

SENDDUMMY4:

	XOR	AL,AL
	SIOW				; SECOND DUMMY

	SETTVEC	DROPRTS

	RET

DROPRTS:

	AND	LINKSTS[EBX],NOT 1	; SET NOT TRANSMITTING

	MOV	AL,5
	SIOCW

	DELAY

	MOV	AL,11100001B		; DROP RTS AND TXEN
	SIOCW

	SETTVEC	IGNORE

	CMP	RXBRG[EBX],0
	JE SHORT DONTCHANGERX
;
;	NEED TO RESET BRG FOR RECEIVE
;
	MOV	CX,RXBRG[EBX]

	MOV	DX,SIOC[EBX]
	MOV	AL,12

	OUT	DX,AL
	DELAY
	MOV	AL,CL
	OUT	DX,AL			; SET LSB

	DELAY
	MOV	AL,13
	OUT	DX,AL

	DELAY
	MOV	AL,CH
	OUT	DX,AL			; SET MSB
;
DONTCHANGERX:

	RET
;

;***	TX INTERRUPT IGNORE
;
IGNORE:

	MOV	AL,SDRPEND
	SIOCW				; RESET TX INTERRUPT PENDING

	RET
;
	EVEN

	public	SIOINT

;
SIOINT: 					; ENTERED FROM HARDWARE INTERRUPT
;
;	EDX has pointer to port that hooked the interrupt
;
	pusha

	mov	ebx,edx			; Ref Data


SIOI10:

	MOV	DX,ASIOC[EBX]
	MOV	AL,3
	OUT	DX,AL			; SELECT RR3

	DELAY

	IN	AL,DX			; GET PENDING INTS
	OR	AL,AL
	JZ SHORT NOINTS			; NONE

	push	ebx			; for EOI

	MOV	DX,BSIOC[EBX]
	MOV	AL,2
	OUT	DX,AL			; SELECT RR2

	DELAY

	MOV	EAX,0
	IN	AL,DX			; GET VECTOR

	CMP	AL,8
	JAE SHORT ACHAN

	MOV	EBX,B_PTR[EBX]		; GET DATA FOR B CHANNEL
	JMP SHORT INT_COMMON

ACHAN:

	SUB	AL,8
	MOV	EBX,A_PTR[EBX]

INT_COMMON:


	LEA	ESI,VECTORS[EBX]

	ADD	ESI,EAX
	ADD	ESI,EAX				; 4 BYTE POINTERS

	CALL	DWORD PTR [ESI]

	MOV	DX,ASIOC[EBX]
	MOV	AL,00111000B		; RESET IUS
	OUT	DX,AL

	POP	EBX

	JMP	SIOI10			; SEE IF ANY MORE 

NOINTS:

	mov     eax, IRQHand[EBX]
	VxDCall	VPICD_Phys_EOI			; eoi it

	clc
	popa
	
	RET
;
Q_REM:

	MOV	EDI,[ESI]			; GET ADDR OF FIRST BUFFER
	CMP	EDI,0
	JE SHORT Q_RET			; EMPTY

	MOV	EAX,[EDI]			; CHAIN FROM BUFFER
	MOV	[ESI],EAX			; STORE IN HEADER

;	POP	[EDI]			; CALLERS ADDR 
;	PUSH	[EDI]
Q_RET:
	RET

;               
Q_ADD:

	CMP	EDI,0
	JNE SHORT BUFOK

	RET

BUFOK:

Q_ADD05:
	CMP	DWORD PTR [ESI],0		; END OF CHAIN
	JE SHORT Q_ADD10

	MOV	ESI,[ESI]			; NEXT IN CHAIN
	JMP	Q_ADD05
Q_ADD10:
	MOV	DWORD PTR [EDI],0		; CLEAR CHAIN ON NEW BUFFER
	MOV	[ESI],EDI			; CHAIN ON NEW BUFFER

	RET

;
;	ADD TO FRONT OF QUEUE - MUST ONLY BE USED FOR FREE QUEUE
;
Q_ADDF:
	CMP	EDI,0
	JNE SHORT BUFOK1

	RET

BUFOK1:

	MOV	EAX,[ESI]			; OLD FIRST IN CHAIN
	MOV	[EDI],EAX
	MOV	[ESI],EDI			; PUT NEW ON FRONT

	INC	QCOUNT
	RET

GETBUFF:

	MOV	ESI,OFFSET32 FREE_Q
	CALL	Q_REM

	JZ SHORT NOBUFFS

	DEC	QCOUNT
;
;	MOV	AX,QCOUNT
;	CMP	AX,MINBUFFCOUNT
;	JA SHORT GETBUFFRET
;	MOV	MINBUFFCOUNT,AX

GETBUFFRET:
	OR	AL,1			; SET NZ
;
 	RET

NOBUFFS:
NOBUFFERCHECK:

;	INC	NOBUFFCOUNT
	XOR	AL,AL
	RET
;

HOOKINT:

	PUSHAD
	movzx eax, INTLEVEL[EBX]
	Debug_Printf	"hdlc98 Hooking Interupt %x", <eax>
	POPAD

	mov     edi, OFFSET32 vid
	
	movzx	ax,INTLEVEL[EBX]
	mov	VID_IRQ_Number[EDI],ax
	
	mov	eax,PORTINTERRUPT[EBX]
	mov	VID_Hw_Int_Proc[EDI],EAX

	mov	VID_Options[EDI],VPICD_OPT_REF_DATA
	MOV	VID_Hw_Int_Ref[EDI],ebx

	push	ebx
	VxDcall VPICD_Virtualize_IRQ
	pop		ebx
	jc      errorhandler
	
	mov     IRQHand[EBX], eax

	VxDCall	VPICD_Physically_Unmask
	jc      errorhandler2

	ret


errorhandler:

	PUSHAD
	Debug_Printf	"hdlc98 Hook Interupt Failed"
	POPAD

	ret
	
errorhandler2:

	PUSHAD
	Debug_Printf	"hdlc98 Unmask Interupt Failed"
	POPAD

	ret


EndProc VXD_EXIT

VxD_LOCKED_CODE_ENDS



;******************************************************************************
;                       R E A L   M O D E   C O D E
;******************************************************************************

;******************************************************************************
;
;       Real mode initialization code
;
;   DESCRIPTION:
;       This code is called when the system is still in real mode, and
;       the VxDs are being loaded.
;
;       This routine as coded shows how a VxD (with a defined VxD ID)
;       could check to see if it was being loaded twice, and abort the 
;       second without an error message. Note that this would require
;       that the VxD have an ID other than Undefined_Device_ID. See
;       the file VXDID.TXT more details.
;
;   ENTRY:
;       AX = VMM Version
;       BX = Flags
;               Bit 0: duplicate device ID already loaded 
;               Bit 1: duplicate ID was from the INT 2F device list
;               Bit 2: this device is from the INT 2F device list
;       EDX = Reference data from INT 2F response, or 0
;       SI = Environment segment, passed from MS-DOS
;
;   EXIT:
;       BX = ptr to list of pages to exclude (0, if none)
;       SI = ptr to list of instance data items (0, if none)
;       EDX = DWORD of reference data to be passed to protect mode init
;
;==============================================================================

VxD_REAL_INIT_SEG

BeginProc VxD_Real_Init_Proc

	test	bx, Duplicate_Device_ID ; check for already loaded
	jnz short duplicate 	        ; jump if so
;
	MOV	DX,OFFSET SIGNON
	MOV	AH,9
	INT	21H
;
        xor     bx, bx                  ; no exclusion table
        xor     si, si                  ; no instance data table
        xor     edx, edx                ; 
        mov     ax, Device_Load_Ok
        ret

duplicate:

        mov     ax, Abort_Device_Load + No_Fail_Message
        ret

CR	EQU	0DH
LF	EQU	0AH

SIGNON	DB	0DH,0AH
	DB	'G8BPQ HDLC Port Interface for BPQ32 Version 1.0  Jan 2000',CR,LF
	DB	CR,LF,'$'


EndProc VxD_Real_Init_Proc


VxD_REAL_INIT_ENDS


        END

