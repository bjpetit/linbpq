  PAGE	58,132
;******************************************************************************
; Serial.ASM	VxD to provide VxD level COMM services for NS8250/16550AFN.
;******************************************************************************
;
; TITLE:	SERIAL.ASM
;
; Version:	1.0
;
; Date:		11/19/92
;
; Author:	Sandeeps
;
;==============================================================================
;
; Change log:
;
;******************************************************************************

	.386p

;******************************************************************************
;			INCLUDE files
;******************************************************************************

Create_SERIAL_Service_Table	equ	TRUE

SERIAL_Dynamic	equ	1

	.xlist
	include	VMM.INC
	include	VCOMM.INC
	include	DEBUG.INC
	include	OPTTEST.INC
	include	VPICD.INC
	include	INTERNAL.INC
	include	INS8250.INC
	include	VCD.INC
	.list
	
VxD_Pageable_Code_Seg

	EXTRN	Serial_Dynamic_Device_Exit:NEAR

VxD_Pageable_Code_Ends


;******************************************************************************
;			Virtual Device Declaration
;******************************************************************************

Declare_Virtual_Device	BPQVCOMM, SERIAL_Major_Version,SERIAL_Minor_Version,\ 
		SERIAL_Control, Undefined_Device_ID, VCD_Init_Order,,, ;BPQVXD_PM_API_HANDLER

;******************************************************************************
;			Locked Data
;******************************************************************************
VxD_Locked_Data_Seg

		PUBLIC	Serial_Functions
		PUBLIC	PortInfoHandle
		PUBLIC	SysVMHandle
		PUBLIC	BaseAddr
		PUBLIC	pGetModemStatus_PrevHook
		PUBLIC	pEscapeCommFunction_PrevHook

	align	4

Serial_Functions	label	DWORD
	dd	OFFSET32 _PortSetState
	dd	OFFSET32 _PortGetState
	dd	OFFSET32 _PortSetup
	dd	OFFSET32 _PortTransmitChar
	dd	OFFSET32 _PortClose
	dd	OFFSET32 _PortGetQueueStatus
	dd	OFFSET32 _PortClearError
	dd	OFFSET32 _PortSetModemStatusShadow
	dd	OFFSET32 _PortGetProperties
	dd	OFFSET32 _PortEscapeFunction
	dd	OFFSET32 _PortPurge
	dd	OFFSET32 _PortSetEventMask
	dd	OFFSET32 _PortGetEventMask
	dd	OFFSET32 _PortWrite
	dd	OFFSET32 _PortRead
	dd	OFFSET32 _PortEnableNotification
	dd	OFFSET32 _PortSetReadCallBack
	dd	OFFSET32 _PortSetWriteCallBack
	dd	OFFSET32 _PortGetModemStatus
	dd	OFFSET32 _PortGetCommConfig
	dd	OFFSET32 _PortSetCommConfig
	dd	OFFSET32 _PortGetWin32Error
	dd	0

.errnz	($ - Serial_Functions - SIZE _PortFunctions)

	align	4

PortInfoHandle	dd	0
SysVMHandle		dd	0
BaseAddr		dd	0

pGetModemStatus_PrevHook      dd 0    ; Addresses of previous service handlers
pEscapeCommFunction_PrevHook      dd 0    ; Addresses of previous service handlers

ExtTab	label	dword
	dd	OFFSET32 ExtComDummy	; function 0: never mind
	dd	OFFSET32 ExtCom_FN1	; Set X-Off
	dd	OFFSET32 ExtCom_FN2	; clear X-Off
	dd	OFFSET32 ExtCom_FN3	; Set RTS
	dd	OFFSET32 ExtCom_FN4	; Clear RTS
	dd	OFFSET32 ExtCom_FN5	; Set DSR
	dd	OFFSET32 ExtCom_FN6	; Clear DSR
	dd	OFFSET32 ExtCom_FN7	; Reset Printer
	dd	OFFSET32 ExtComDummy	; Get Max LPT port
	dd	OFFSET32 ExtComDummy	; Get Max COM Port
	dd	OFFSET32 ExtCom_FN10	; Get COM port base and IRQ
	dd	OFFSET32 ExtCom_FN10	; Get COM port base & IRQ
	dd	OFFSET32 ExtCom_FN12	; set break
	dd	OFFSET32 ExtCom_FN13	; clear break

BaudRateByIndexTable label word
	dw 1047     ; CBR_110
	dw 384	    ; CBR_300
	dw 192	    ; CBR_600
	dw 96	    ; CBR_1200
	dw 48	    ; CBR_2400
	dw 24	    ; CBR_4800
	dw 12	    ; CBR_9600
	dw 8	    ; CBR_14400
	dw 6	    ; CBR_19200
	dw 0	    ;	 0FF19h  (reserved)
	dw 0	    ;	 0FF1Ah  (reserved)
	dw 3	    ; CBR_38400
	dw 0	    ;	 0FF1Ch  (reserved)
	dw 0	    ;	 0FF1Dh  (reserved)
	dw 0	    ;	 0FF1Eh  (reserved)
	dw 2	    ; CBR_56000

VxD_Locked_Data_Ends

;******************************************************************************
;			Locked code
;******************************************************************************

VxD_Locked_Code_Seg

;******************************************************************************
;
; SERIAL_Control
;
; Description:		Dispatcher for system messages.
;
; Entry:
; Exit:
; Uses:
;******************************************************************************

BeginProc SERIAL_Control

	Control_Dispatch VM_Not_Executeable, Serial_Not_Executeable
	Control_Dispatch SYS_DYNAMIC_DEVICE_INIT, Serial_Device_Init
	Control_Dispatch DEVICE_INIT, Serial_Device_Init
	Control_Dispatch SYS_DYNAMIC_DEVICE_EXIT, Serial_Dynamic_Device_Exit
IFDEF	DEBUG
	Control_Dispatch Debug_Query,	Serial_Debug_Query
ENDIF
    Control_Dispatch W32_DEVICEIOCONTROL, VXD_IOCONTROL

	clc
	ret

EndProc SERIAL_Control

VxD_Locked_Code_Ends

;******************************************************************************
;		PAGEABLE DATA
;==============================================================================

VxD_My_Pageable_Data_Seg


		PUBLIC	Serial_Name

Serial_Name	db	'SERIAL',0

VxD_My_Pageable_Data_Ends

;******************************************************************************
;		PAGEABLE CODE
;==============================================================================

VxD_My_Pageable_Code_Seg

		EXTRN	VXD_IOCONTROL:NEAR
		EXTRN	GetCOMPort:NEAR
		EXTRN	ReleaseCOMPort:NEAR
		EXTRN	Serial_Device_Init:NEAR
		EXTRN	Terminate:NEAR
		EXTRN	MaskIRQ:NEAR
		EXTRN	UnMaskIRQ:NEAR
		EXTRN	KickTx:NEAR
		EXTRN	MSRWait:NEAR
		EXTRN	Notify_Owner:NEAR
IFDEF	DEBUG
		EXTRN	Serial_Debug_Query:NEAR
ENDIF
		EXTRN	ExtComDummy:NEAR
		EXTRN	ExtCom_FN1:NEAR
		EXTRN	ExtCom_FN2:NEAR
		EXTRN	ExtCom_FN3:NEAR
		EXTRN	ExtCom_FN4:NEAR
		EXTRN	ExtCom_FN5:NEAR
		EXTRN	ExtCom_FN6:NEAR
		EXTRN	ExtCom_FN7:NEAR
		EXTRN	ExtCom_FN10:NEAR

 		EXTRN	_PortSetState:NEAR
 		EXTRN	_PortGetState:NEAR
 		EXTRN	_PortSetup:NEAR
 		EXTRN	_PortTransmitChar:NEAR
 		EXTRN	_PortClose:NEAR
 		EXTRN	_PortGetQueueStatus:NEAR
 		EXTRN	_PortClearError:NEAR
 		EXTRN	_PortSetModemStatusShadow:NEAR
 		EXTRN	_PortGetProperties:NEAR
 		EXTRN	_PortEscapeFunction:NEAR
 		EXTRN	_PortPurge:NEAR
 		EXTRN	_PortSetEventMask:NEAR
 		EXTRN	_PortGetEventMask:NEAR
 		EXTRN	_PortWrite:NEAR
 		EXTRN	_PortRead:NEAR
 		EXTRN	_PortEnableNotification:NEAR
 		EXTRN	_PortOpen:NEAR
 		EXTRN	_PortSetReadCallBack:NEAR
 		EXTRN	_PortSetWriteCallBack:NEAR
 		EXTRN	_PortGetModemStatus:NEAR
		EXTRN	_PortGetCommConfig:NEAR
		EXTRN	_PortSetCommConfig:NEAR
		EXTRN	_PortGetWin32Error:NEAR

VxD_My_Pageable_Code_Ends

VxD_Locked_Code_Seg

;***
; SetCom100
;
; Description:
;	Copy a given DCB into the appropriate DEB. 
;	This does it selectively.
;
; Entry:
;		EBX -> DCB
;		ESI -> PortInformation
;		ECX = mask of relevant fields.
;
; Exit:		ESI -> PortInformation
;
; Uses:
;		EAX,ECX,FLAGS
;
BeginProc SetCom100, PUBLIC

	push	esi			; save ESI
	lea	esi,[esi.ComDCB]	; to reduce code size

	mov	[esi._DCB.DCBLength], SIZE _DCB
	TestReg	ecx,fBaudRate		; is caller interested in baud rate?
	jz	@F
	mov	eax,[ebx._DCB.BaudRate]
	mov	[esi._DCB.BaudRate],eax
@@:
	TestReg	ecx,fBitMask
	jz	@F
	mov	eax,[ebx._DCB.Bitmask]
	mov	[esi._DCB.Bitmask],eax
@@:
	TestReg	ecx,fXonLim
	jz	@F
	mov	eax,[ebx._DCB.XonLim]
	mov	[esi._DCB.XonLim],eax
@@:
	TestReg	ecx,fXoffLim
	jz	@F
	mov	eax,[ebx._DCB.XoffLim]
	mov	[esi._DCB.XoffLim],eax
@@:
	TestReg	ecx,fLCR
	jz	@F
	mov	al,[ebx._DCB.ByteSize]
	mov	[esi._DCB.ByteSize],al
	mov	al,[ebx._DCB.Parity]
	mov	[esi._DCB.Parity],al
	mov	al,[ebx._DCB.StopBits]
	mov	[esi._DCB.StopBits],al
@@:
	TestReg	ecx,fXonChar
	jz	@F
	mov	al,[ebx._DCB.XonChar]
	mov	[esi._DCB.XonChar],al
@@:
	TestReg	ecx,fXoffChar
	jz	@F
	mov	al,[ebx._DCB.XoffChar]
	mov	[esi._DCB.XoffChar],al
@@:
	TestReg	ecx,fErrorChar
	jz	@F
	mov	al,[ebx._DCB.ErrorChar]
	mov	[esi._DCB.ErrorChar],al
@@:
	TestReg	ecx,fEofChar
	jz	@F
	mov	al,[ebx._DCB.EofChar]
	mov	[esi._DCB.EofChar],al
@@:
	TestReg	ecx,fEvtChar1
	jz	@F
	mov	al,[ebx._DCB.EvtChar1]
	mov	[esi._DCB.EvtChar1],al
@@:
	TestReg	ecx,fEvtChar2
	jz	@F
	mov	al,[ebx._DCB.EvtChar2]
	mov	[esi._DCB.EvtChar2],al
@@:
	TestReg	ecx,fRlsTimeout
	jz	@F
	mov	eax,[ebx._DCB.RlsTimeout]
	mov	[esi._DCB.RlsTimeout],eax
@@:
	TestReg	ecx,fDsrTimeout
	jz	@F
	mov	eax,[ebx._DCB.DsrTimeout]
	mov	[esi._DCB.DsrTimeout],eax
@@:
	TestReg	ecx,fCtsTimeout
	jz	@F
	mov	eax,[ebx._DCB.CtsTimeout]
	mov	[esi._DCB.CtsTimeout],eax
@@:
	TestReg	ecx,fTxDelay
	jz	@F
	mov	eax,[ebx._DCB.TxDelay]
	mov	[esi._DCB.TxDelay],eax
@@:
	mov	ax,[ebx._DCB.wReserved]
	mov	[esi._DCB.wReserved],ax

	pop	esi
	ret

EndProc SetCom100

;***
; SetCom200
;
; Description:	Based on whether or not a timeout has been specified for
;		each signal, set up a mask byte which is used to mask off
;		lines for which we wish to detect timeouts. 0 indicates that
;		the line is to be ignored.
;
;		Also set up mask to indicate those lines which are set for
;		infinite timeout. -1 indicated that.
;
; Entry:	EBX -> DCB
;		ESI -> PortInformation
; Exit:
;		EBX -> DCB32
;		AH = lines to check
;		AL = lines with infinite timeout
;
; Uses:
;		EAX,ECX,FLAGS
;***************************************************************************** 
BeginProc SetCom200, PUBLIC
	xor	eax,eax		; get mask of lines with timeout = 0
	xor	ecx,ecx
	call	SetCom210
	not	al		; invert result to get lines to check
	and	al,ACE_CTS+ACE_DSR+ACE_RLSD
	xchg	ah,al
	dec	ecx		; get mask of infinite timeouts

SetCom210:
	cmp	[ebx._DCB.RlsTimeout],ecx ; timeout set to passed value ?
	jne	SetCom220		; No
	or	al,ACE_RLSD		; Yes, show checking line

SetCom220:
	cmp	[ebx._DCB.CtsTimeOut],ecx ; Timeout set to passed value ?
	jne	SetCom230		; No
	or	al,ACE_CTS		; Yes, show checking line

SetCom230:
	cmp	[ebx._DCB.DsrTimeOut],ecx ; Timeout set to passed value ?
	jne	SetCom240		; No
	or	al,ACE_DSR		; Yes, show checking line

SetCom240:
	ret

EndProc SetCom200

;***
; SetCom300
;
; Description:
;	Calculate the correct BAUDRATE divisor for the COMM chip
;
;	Note that the baudrate is allowed to be any integer in the
;	range 2-19200. The divisor is computed as 115,200/baudrate.
;
; Entry:
;	EBX -> DCB32
; Exit:
;	EBX -> DCB32
;	ECX = baudrate (0 if error AX = error code if invalid baud rate)
; Uses:
;	EAX,ECX, flags
;
BeginProc SetCom300, PUBLIC

	push	edx
	mov	ecx,[ebx._DCB.BaudRate]	; get requested baud rate
	xor	eax,eax			; assume error
	cmp	ecx,1C200h
	je	SetCom115200
	cmp	ecx,CBR_110		; Q: baudrate specified as an index?
	jae	by_index		;  Y:
	cmp	ecx,2			;  N: by value (range check it)
	jnae	SetCom310		; Below range

SetCom115200:
	xor	edx,edx			; EDX:EAX = 115,200
	mov	eax,01C200h
	div	ecx			; (EAX) = 115,200/baud

SetCom310:
	mov	ecx,eax			; (ECX) = baud rate, or error code (0)
	mov	eax,IE_BAUDRATE		; set error code incase bad baud
	pop	edx
	ret

by_index:
	cmp	ecx,CBR_56000		; Q: above supported?
	ja	SetCom115200		;  Y: return error
	push	ebx
	mov	ebx,ecx
	sub	ebx,CBR_110
	shl	ebx,1
	movzx	eax,WORD PTR [ebx+BaudRateByIndexTable] ; get divisor
	pop	ebx
	jmp	SetCom310		; Y: return error

EndProc SetCom300

;***
; SetCom400
;
; Description:	Check the line config. (parity,stop,byte size)
;
; Entry:	EBX -> DCB32
; Exit:
;		EBX -> DCB32
;		NC -> OK, else error (AX = error code)
;		AL = LCR, AH = RxMask
;		EDI (Bits 15:8) = flags mask (to remove parity checking)
;		EDI (Bits 7:0) = Error mask (to remove parity error)
; Uses:
;		EAX,ECX,EDI,FLAGS
;
BeginProc SetCom400, PUBLIC

	movzx	eax,WORD PTR [ebx._DCB.ByteSize] ; al = byte size, ah = parity
	cmp	ah,SpaceParity		; parity out of range >
	ja	SetCom470		;  Yes, return error
	mov	edi,0FF00h+ACE_OR+ACE_PE+ACE_FE+ACE_BI
	or	ah,ah			; is parity 'NONE'?
	jnz	SetCom410		;  No, something is there for parity
	xor	edi,(fParity*256)+ACE_PE ; disable parity checking

SetCom410:
	cmp	al,8			; Byte size out of range?
	ja	SetCom460		;  Yes, return error

SetCom420:
	sub	al,5			; Shift byte size to bits 0,1
	.errnz	ACE_WLS-00000011b	; Word length must be these bits
	jc	SetCom460		; Byte size is illegal, return error
	add	ah,ah			; map parity to ACE bits
	jz	SetCom430		; 0=>0,1=>1,2+.3,3=>5,4=>7
	dec	ah

SetCom430:
	shl	ah,3			; align with 8250 parity bits
	or	al,ah			; add to byte size

	.errnz	NoParity-0
	.errnz	OddParity-1
	.errnz EvenParity-2
	.errnz MarkParity-3
	.errnz SpaceParity-4
	.errnz ACE_PEN-00001000b
	.errnz ACE_PSB-00110000b
	.errnz ACE_EPS-00010000b
	.errnz	ACE_SP-00100000b

	or	al,ACE_2SB		; Assume 2 stop bits
	mov	ah,[ebx._DCB.StopBits]	; get # of stop bits 0=1,1/2= .GT. 1
	or	ah,ah			; out of range ?
	js	SetCom470		;  Yes, return error
	jz	SetCom440		; One stop bit
	sub	ah,2
	jz	SetCom450		; 2 stop bits
	jns	SetCom470		; Not 1.5, return error
	test	al,ACE_WLS		; 1.5 stop bits, 5 bit words?
	jnz	SetCom470		;  No, illegal

	.errnz OneStopBit-0
	.errnz One5StopBits-1
	.errnz TwoStopBits-2
	.errnz ACE_5BW

SetCom440:
	and	al,NOT ACE_2SB		;Show 1 (or 1.5) stop bit(s)

 ;
 ; From the byte size, get a mask to be used for stripping
 ; off unused bits as the characters are received.
 ;
SetCom450:
	push	edx
	mov	cl,[ebx._DCB.ByteSize]	;Get data byte size
	mov	edx,00FFh		;Turn into mask by shifting bits
	shl	edx,cl
	mov	ah,dh			;Return mask in ah
	pop	edx
	clc				;Show all is fine
	ret

SetCom460:
	mov	eax,IE_ByteSize		;Show byte size is wrong
	stc				;Show error
	ret

SetCom470:
	mov	eax,IE_Default		;Show something is wrong
	stc				;Show error
	ret

EndProc SetCom400

VxD_Locked_Code_Ends

VxD_My_Pageable_Code_Seg

BeginDoc
;******************************************************************************
;
; RecCom:
;
; Description:
;		Receive a byte from channel (if available).
;
; Entry:
;	ESI -> PortInformation struct
; Exit:
;	'Z' clear if data available
;	AL = byte
;	Else if 'Z' set, no data/ error, AX = error code AX = 0 => no data
;
; Uses:
;	C standard
;
;==============================================================================
EndDoc
BeginProc RecCom, Public

	push	esi
	push	edi
 ;
 ; Before removing any characters from the input queue, check to see
 ; if XON needs to be issued. If it needs to be issued, set the
 ; flag that will force it and arm transmit interrupts.
 ;
	TestMem	[esi.ComDCB.BitMask],<fEnqAck+fEtxAck>	; Enq ot Etx ack?
	jz	RecCom32		; No
	test	[esi.HSFlag],EnqReceived+HHSDropped ; Enq recvd/lines dropped?
	jz	RecCom60		; No
	jmp	RecCom34

RecCom32:
	test	[esi.HSFlag],HSSent	; handshake sent?
	jz	RecCom60		;  No XOFF sent & no lines dropped

RecCom34:
	mov	eax,[esi.pData.QInCount] ; get current count of input chars
	cmp	eax,[esi.ComDCB.XonLim]	; see if at Xon limit
	ja	RecCom60		; Not yest

 ;
 ; If any hardware lines are down, then raise them. Then see
 ; about sending XON.
 ;
	mov	edx,[esi.Port]		; get the port
	mov	ah,[esi.HHSLines]	; get hardware lines mask
	cli				; handle this as a critical section
	mov	cl,[esi.HSFlag]		; get handshaking flags
	or	ah,ah			; any hw lines to play with ?
	jz	RecCom40		;  NO
	add	dl,ACE_MCR		;  --> modem control register
	in	al,dx
	or	al,ah			; Turn on the hardware bits
ifdef  NEC_98
	IO_Delay_NEC 1
else  ;NEC_98
	IO_Delay
	IO_Delay
endif ;NEC_98
	out	dx,al
ifdef  NEC_98
	IO_Delay_NEC 1
endif ;NEC_98
	and	cl,NOT HHSDropped	; show hardware lines back up

RecCom40:
	TestMem	[esi.ComDCB.BitMask],<fEnqAck+fEtxAck> ; enq or Etx Ack?
	jz	RecCom47		; No
	test	cl,EnqReceived		; did we receive Enq?
	jz	RecCom55		; No
	and	cl,NOT EnqReceived
	jmp	RecCom50

RecCom47:
	test	cl,XOffSent		; Did we send XOFF?
	jz	RecCom55		;  No
	and	cl,NOT XoffSent		; remove XOFF sent flag

RecCom50:
	or	cl,XONPending		; show XON or ACK must be sent
	call	KickTx			; kick xmit if needed

RecCom55:
	mov	[esi.HSFlag],cl		; store handshaking flag
	sti				; can allow interrupts now

 ;
 ; Now we can get down to the business at hand, and remove a character
 ; from the receive queue. If a communications error exists, we return
 ; that and nothing else.
 ;
RecCom60:
	xor	eax,eax
	or	eax,[esi.pData.dwCommError] ; any errors ?
	jnz	RecCom100		;  Yes, return error code
	or	eax,[esi.pData.QInCount] ; get current input char count
	jz	RecCom90		; No chars in q
	mov	edi,[esi.pData.QInAddr]	; get q pointer

	mov	ebx,[esi.pData.QInGet]	; also get the index to head
	mov	al,[ebx+edi]		; finally get the byte from queue
	inc	ebx			; update q index
	cmp	ebx,[esi.pData.QInSize]	; wrap-around ?
	jc	RecCom70		;  No wrap
	xor	ebx,ebx			; wrap by zeroing the index

RecCom70:
	mov	[esi.pData.QInGet],ebx	; save new head pointer
	dec	[esi.pData.QInCount]		; dec # of bytes in queue

	mov	ecx,[esi.pData.QInCount]; Q: have we read below trigger
	jae	RecCom80		;  N:
	and	[esi.NotifyFlagsHI], NOT CN_RECEIVE ;allow timeout notify again

RecCom80:
	or	esp,esp			; reset PSW.Z
	pop	edi
	pop	esi
	ret

;
; No characters in the input queue. Check to see if EOF was received, and
; return it if it was. Otherwise show no characters.
;
RecCom90:
	TestMem	[esi.ComDCB.BitMask],fBinary	; are we doing binary stuff?
	jnz	RecCom95		;  Yes, show no characters
	mov	al,[esi.ComDCB.EofChar]	; assume eof
	test	[esi.EFlags],fEOF	; has end of file char been received?
	jnz	RecCom80		; show no more characters

RecCom95:
	xor	eax,eax			; Show no more characters

; Return with 'Z' to show error or no characters

RecCom100:
	xor	ecx,ecx			; set PSW.Z
	pop	edi
	pop	esi
	ret

EndProc RecCom

VxD_My_Pageable_Code_Ends

VxD_Locked_Code_Seg

;***
; TXI - xmit a char immediately.
;	Places the char in a location that guarantees it to be the next
;	char transmitted.
; Entry:
;	AH = char
;	ESI -> DEB
; Exit:
;	None
; Uses:
;	C standard
;
BeginProc TXI, PUBLIC, NO_PROLOG

	or	[esi.EFlags],fTxImmed	; show char to xmit
	mov	[esi.ImmedChar],ah	; set char
	jmp	KickTx			; kick transmit interrupt just in case

EndProc TXI

BeginDoc
;******************************************************************************
;
; StaCom:
;
; Description:
;		returns status of open channel.
;		Returns the number of bytes in both queues.
; Entry:
;		ESI = Portinformation struct
;		EBX -> Ptr to status structure to be updated.
; Exit:
;		AX = error word
;		status structure is updated.
; Uses:
;		C standard
;
;==============================================================================
EndDoc
BeginProc StaCom, Public

	or	ebx,ebx			; Null pointer ?
	jz	StaCom25		; yes, return error code
;
; Need to get the status for a com port.  Since not all the
; status is contained within EFlags, it has to be assembled.
; Also note that currently there is no way to specify RLSD
; as a handshaking line, so fRLSDHold is always returned false.
;
	mov	ecx,[esi.pData.QInCount];Get input queue count
	mov	edx,[esi.pData.QOutCount] ;Get tx queue count
	
	
;#define	fCtsHold	0x00000001	/* Transmit is on CTS hold */
;#define	fDsrHold	0x00000002	/* Transmit is on DSR hold */
;#define	fRlsdHold	0x00000004	/* Transmit is on RLSD hold */
;#define	fXoffHold	0x00000008	/* Received handshake */
;#define	fXoffSent	0x00000010	/* Issued handshake */
;#define	fEof		0x00000020	/* EOF character found */
;#define	fTximmed	0x00000040	/* Character being transmitted */


	mov	eax,0
	mov	[ebx._COMSTAT.BitMask],eax
	mov	[ebx._COMSTAT.cbInQue],ecx
	mov	[ebx._COMSTAT.cbOutQue],edx

StaCom25:
	ret

EndProc StaCom

BeginDoc
;******************************************************************************
;
; ExtnFcn:
;
; Description:
;		some extended functions.
;		Functions currently implemented:
;
;	0: Dummy   - Ignored
;	1: SETXOFF - Exactly as if X-OFF character has been received.
;	2: SETXON  - Exactly as if X-ON character has been received.
;	3: SETRTS  - Set the RTS signal
;	4: CLRRTS  - Clear the RTS signal
;	5: SETDTR  - Set the DTR signal
;	6: CLRDTR  - Clear the DTR signal
;	7: RESETDEV- Yank on reset line if available (LPT devices)
;	8: GETLPTMAX - ignored
;	9: GETCOMMAX - ignored
;	10: GETCOMBASEIRQ - return base and IRQ of COMM port
;	11: GETCOMBASEIRQ1 - -do-
;	12: SETBREAK - set break condition
;	13: CLEARBREAK - clear break condition
;
; Entry:
;	ESI -> Port information EBX = function code.
; Exit:
;	EAX = Comm error word or return value of subfunction
; Uses:
;	C standard
;
;==============================================================================
EndDoc
BeginProc ExtnFcn, Public

	SaveReg <ad>
	Debug_Printf	"BPQVC ExtnFcn %d", <ebx>
	RestoreReg <ad>


	push	edi
	mov	edx,[esi.Port]		; get base address
	shl	ebx,2
	cli				; exclusive access.
	call	DWORD PTR [ebx+ExtTab]	; call function
	sti
	jc	ExtCom40		; jump if sub returns data in EAX
	mov	eax,[esi.pData.dwCommError]
ExtCom40:
	pop	edi
	ret

EndProc ExtnFcn

BeginDoc
;******************************************************************************
;
; ExtCom_FN12
;
; Description:
;		Sets break condition. Suspends character transmission, 
;		and places the transmission line in a break state until 
;		Clear break is called.
;		Clamps transmit data line low. Doesn't wait for the
;		transmitter holding register and shift registers to empty.
; Entry:
;		ESI -> Port data
;		EDX = port base
; Exit:
;		CLC
; Uses:
;		C standard.
;
;==============================================================================
EndDoc
BeginProc ExtCom_FN12, Public

	mov	ecx,0FF00h+ACE_SB	; will be setting break
	jmp	ClrBrk10
	.errnz	BreakSet-ACE_SB		; must be same

EndProc ExtCom_FN12

BeginDoc
;******************************************************************************
;
; ExtCom_FN13
;
; Description:
;		Clears break condition.
;		Releases any BREAK clamp on Tx data line
; Entry:
;		ESI -> port data
;		EDX -> Port base
; Exit:
;		CLC
; Uses:
;		C standard.
;
;==============================================================================
EndDoc
BeginProc ExtCom_FN13, Public

	mov	ecx,(NOT ACE_SB) SHL 8
	.errnz	BreakSet-ACE_SB		; must be same bits

ClrBrk10:
	and	[esi.HSFlag],ch		; Set or clear the BreakSet Bit
	or	[esi.HSFlag],cl
 ;
 ; CH = mask to remove bits in the line Control reg
 ; CL = mask to turn on the bits in the line control reg
 ;
	add	dl,ACE_LCR		; --> LCR
	in	al,dx			; old control value
	and	al,ch			; turn off undesired bits
	or	al,cl			; turn on desired bits
ifdef  NEC_98
	IO_Delay_NEC 1
else  ;NEC_98
	IO_Delay
endif ;NEC_98
	out	dx,al			; new LCR value
ifdef  NEC_98
	IO_Delay_NEC 1
endif ;NEC_98
	clc				; caller gets error dword
	ret

EndProc ExtCom_FN13

BeginDoc
;******************************************************************************
;
; ReadCommString
;
; Description:
;	Return immediately, read a max of n bytes.
;
; Entry:
;		ESI -> port information struct
;		EDI -> receive buffer
;		ECX = max bytes to read.
; Exit:
;		'Z' clear if data is available, AX = # bytes read.
;		'Z' set => error/no data. AX = 0 => no data.
; Uses:
;		C standard
;
;==============================================================================
EndDoc
BeginProc ReadCommString, Public

	push	esi
	push	edi

	xor	eax,eax
	TestMem	[esi.pData.MiscFlags],IgnoreCommError	; Q: ignore comm error?
	jnz	@F				; Y: no need to do this stuff.
	or	eax,[esi.pData.dwCommError]	; any errors?
	jnz	RecStr100		; Yes, return error code
@@:
	or	eax,[esi.pData.QInCount] ; any chars in input queue
	jz	RecStr90		; no chars in queue

	cmp	ecx,eax			; Q:more chars available than can read?
	jbe	RecStr30		;   N:
	mov	ecx,eax			;   Y: adjust # of chars to read
RecStr30:
	push	ecx
	mov	edx,[esi.pData.QInSize]
	mov	eax,[esi.pData.QInGet]
	sub	edx,eax			; EDX = # of bytes before end of buf
	cmp	edx,ecx			; Q: more avail than can read?
	jbe	RecStr40		;   N:
	mov	edx,ecx			;   Y: adjust avail count
RecStr40:
	xchg	ecx,edx			; ECX = # bytes for 1st copy
	sub	edx,ecx			; EDX = # bytes for 2nd copy

	push	esi
	mov	ebx,[esi.pData.QInAddr]
	mov	esi,ebx
	add	esi,eax			; esi-> first char in buffer
	cld
	rep	movsb			; do first copy
	mov	ecx,edx
	jecxz	RecStr50		; jump if no 2nd copy needed
	mov	esi,ebx			; ESI -> start of buffer
	rep	movsb
RecStr50:
	sub	esi,ebx			; esi -> new QInGet
	mov	ebx,esi
	pop	esi
	pop	ecx
	cli
	mov	[esi.pData.QInGet],ebx	; update QInGet
	sub	[esi.pData.QInCount],ecx ; update count
	mov	eax,[esi.pData.QInCount]

	and	[esi.NotifyFlagsHI],NOT CN_RECEIVE ; allow TO notify again

	sti
 RecStr80:
	mov	eax, ecx
	or	esp,esp			;Reset PSW.Z
	pop	edi
	pop	esi
	ret
 ;
 ; No characters in the input queue.  Check to see if EOF
 ; was received, and return it if it was.  Otherwise show
 ; no characters.
 ;
RecStr90:
	TestMem	[esi.ComDCB.BitMask],fBinary	;Are we doing binary stuff?
	jnz	RecStr95		;  Yes, show no characters
	mov	al,[esi.ComDCB.EofChar]	;Assume EOF
	test	[esi.EFlags],fEOF 	;Has end of file char been received?
	jnz	RecStr80		;  Yes, show end of file

RecStr95:
	xor	eax,eax			;Show no more characters

; Return with 'Z' to show error or no characters

RecStr100:
	xor	ecx,ecx			;Set PSW.Z
	pop	edi
	pop	esi
	ret

EndProc ReadCommString

BeginDoc
;******************************************************************************
;
; WriteCommString
;
; Description:
;		The given buffer is sent to the passed port if possible.
;	Once the output queue is detected as being full, a CE_TXFULL error
;	will be indicated and AX will be returned as the # of chars actually
;	queued.
;
; Entry:
;	ESI -> Portinformation struct
;	ECX = # bytes to write
;	EDI -> buffer.
; Exit:
;	EAX = # bytes queued.
; Uses:
;	ALL
;
;==============================================================================
EndDoc
BeginProc WriteCommString, Public

;	push	ecx			; save count
;	call	MSRWait			; see if lines are correct for output
;	pop	ecx
	push	ecx
;	jnz	cws_error		; timeout occured, return error
	mov	edx,[esi.pData.QOutSize] ; see if queue is full
	sub	edx,[esi.pData.QOutCount] ; edx = # of chars free in queue
	jle	scs_full		;  There is no room in the queue

scs_loop:
	push	ecx			; save count left to send
	cmp	ecx,edx			; Q: room for buffer in queue
	jbe	@F			;   Y:
	mov	ecx,edx			;   N: adjust size to send
@@:
	push	ecx			; save # of chars which will be copied
	push	esi
	push	edi
	mov	ebx,[esi.pData.QOutAddr] ; --> output queue
	mov	edx,[esi.pData.QOutSize]
	mov	edi,[esi.pData.QOutPut]	; get index into queue
	sub	edx,edi			; EDX = # of free chars befor end of q
	cmp	edx,ecx
	jbe	@F
	mov	edx,ecx
@@:
	xchg	edx,ecx			; ECX = # of chars for 1st copy
	sub	edx,ecx			; EDX = # of chars for 2nd copy
	pop	esi			; ESI -> SRC buffer
	add	edi,ebx			; EDI -> current pos in q
	cld
	rep	movsb			; copy fist section
	mov	ecx,edx
	jecxz	@F
	mov	edi,ebx			; circle back to start of queue
	rep	movsb
@@:
	sub	edi,ebx			; EDI = last index into queue
	mov	edx,edi
	mov	edi,esi			; last location in src buffer
	pop	esi			; ESI --> COMDEB
	pop	ebx			; # of chars copied
	cli
	mov	[esi.pData.QOutPut],edx	; new index into queue
	add	[esi.pData.QOutCount],ebx
	mov	edx,[esi.pData.QOutCount] ; get new count
	cmp	edx,[esi.SendTrigger]	; Q: Have we overshot the trigger ?
	jb	scs_stillbelowtrigger	;    N: do not start looking
	and	[esi.NotifyFlagsHI], NOT CN_TRANSMIT ; start looking now

scs_stillbelowtrigger:
;	call	KickTx
	sti
	pop	ecx
	
	SaveReg <ad>
	Debug_Printf	"BPQVC Write Input %d Sent %d", <ecx, ebx>
	RestoreReg <ad>

	
	sub	ecx,ebx			; # of chars left to send
	jnz	scs_full_2		;  jump if none
scs_exit:
	pop	eax

	SaveReg <ad>
	Debug_Printf	"BPQVC Write Input %d To Send %d", <eax, ecx>
	RestoreReg <ad>



	sub	eax,ecx			; EAX = # transfered
	ret

scs_full:

	SaveReg <ad>
	Debug_Printf	"BPQVC Write Q Full"
	RestoreReg <ad>

	cli
;	call	KickTx
	sti
scs_full_2:

	SaveReg <ad>
	Debug_Printf	"BPQVC Write Setting TXFULLl"
	RestoreReg <ad>


	or	[esi.pData.dwCommError],CE_TXFULL
	jmp	scs_exit

cws_error:
	pop	eax
	sub	eax,ecx			; EAX = # transfered
cws_exit:
	ret

EndProc WriteCommString

VxD_Locked_Code_Ends

VxD_My_Pageable_Code_Seg

BeginDoc
;******************************************************************************
;
; TrmCom:
;
; Description:
;		terminates (closes) a channel.
;		Waits for any outbound data to be transmitted, drop the
;		hardware handshaking lines, and disable interrupts. If the
;		output queue contained data when it was closed, an error
;		will be returned.
; Entry:
;		ESI -> PortInformation struct
; Exit:
;		EAX = 0	=> no error
;		EAX = 8000h	=> invalid device ID
;		EAX = -2 => output queue timed out.
; Uses:
;		Everything
;
;==============================================================================
EndDoc
BeginProc TrmCom, Public

	or	[esi.MiscFlags],Discard	; Show discarding serial data
	xor	ecx, ecx
	mov	[esi.pData.dwCommError],ecx	; Clear error flaggs.
	mov	[esi.pData.QInCount],ecx ; show no chars in input queue
;	call	RecCom			; send XON if needed.

;
; We have to wait for the output queue to empty.   To do this,
; a timer will be created.  If no character has been transmitted
; when the timeout occurs, then an error will be indicated and
; the port closed anyway.  If the timer cannot be created, then
; just loop until the queue empties, which will be better than
; discarding charatcers if there are any
;
	test	[esi.HSFlag],HHSAlwaysDown ; Q: handshaking ever up ?
	jnz	TermCom17		;      N: skip wait loop

TermCom10:
	mov	ecx,[esi.pData.QOutCount] ; get current queue count
	jecxz	TermCom30		; no chars in queue

	VMMCall	Get_System_Time
	mov	edi,eax

TermCom15:
	cmp	[esi.pData.QOutCount],ecx ; queue count changed ?
	jne	TermCom10		; yes, restart timeout

	VMMCall	Get_System_Time
	sub	eax,edi
	cmp	eax, Timeout		; Q: timeout reached ?
	jb	TermCom15

TermCom17:
	mov	ecx,TimeoutError	; Yes, show timeout error

TermCom30:
	mov	edx,[esi.Port]		; get port base address
	call	Terminate		; the real work is done here
	mov	eax,ecx			; set return code

	ret
EndProc TrmCom

VxD_My_Pageable_Code_Ends

;******************************************************************************
;
;		I N T E R R U P T	T I M E		C O D E
;==============================================================================

VxD_Locked_Code_Seg

BeginDoc
;******************************************************************************
;
; Flush
;
; Description:
;		Flush pending reads/writes. It does so before returning
;		to the caller.
;
; Entry:
;		ESI = PortInformation
;		EBX = 0 => transmit queue, 1=> receive queue
; Exit:
;		none.
; Uses:
;		C standard.
;
;==============================================================================
EndDoc
BeginProc Flush, Public

	push	edi

	mov	ecx,_PortData.QOutCount-_PortData.QInCount ; # of bytes to zero
	lea	edi,[esi.pData.QInCount] ; --> receive Queue data
	or	bl,bl			; xmit q ?
	jnz	Flush10			; No, input Q
	add	edi,ecx			; YES, --> xmit Q data
	mov	edx,[esi.pData.QOutCount] ; save for determining whether to
					; send Xmit notification.....
Flush10:
	cld
	shr	ecx,2			; convert to DWORDS.
	xor	eax,eax
	cli
	rep	stosd
	sti
	.errnz	_PortData.QInGet-_PortData.QInCount-4
	.errnz	_PortData.QInPut-_PortData.QInGet-4
	.errnz	_PortData.QOutCount-_PortData.QInPut-4
	.errnz	_PortData.QOutGet-_PortData.QOutCount-4
	.errnz	_Portdata.QOutPut-_PortData.QOutGet-4

        or      bl,bl                   ; Rx queue?
        jz      Flush30                 ;  No, xmit queue

;
; If the queue to be cleared is the receive queue, any
; hardware handshake must be cleared to prevent a possible
; deadlock situation.  Since we just zeroed the queue count,
; a quick call to $RecCom should do wonders to clear any
; receive handshake (i.e. send XON if needed).
;
Flush20:
	call	RecCom			;Take care of handshakes here
	and	[esi.pData.NotifyFlagsHI], NOT CN_RECEIVE
	jmp	Flush40

Flush30:
	or	edx,edx			; Q: Was there any data ?
	jz	Flush40			;  N: no need to inform anyone
	cmp	[esi.SendTrigger],0
	je	Flush40			; no need to call
	mov	eax, CN_TRANSMIT	; we have fallen below the trigger
	call	Notify_Owner		; notify the owner of the port

Flush40:
	pop	edi
	ret
EndProc Flush



;******************************************************************************
;
; Serial_Not_Executeable
;
;
; VPICD masks off the physical interrupt at VM_Not_Executeable time if no
; VM has the IRQ unmasked. Since VPICD does not keep track of how many
; times a VxD has unmasked an interrupt, it masks off the interrupt we are
; handling. So, we call VPICD_Physically_Unmask for the interrupts handled by
; us.
;
; Entry:
;	EBX = VM Handle to destroy
;
; Exit:
;	Carry set if error
; Uses:
;	ALL
;==============================================================================
BeginProc Serial_Not_Executeable,PUBLIC

	mov	esi,[PortInfoHandle]
	or	esi,esi
	jz	NNE_Done			; No active ports left
	VMMCall	List_Get_First

NNE_Loop:
	jz	NNE_Done			; No more active ports
	push	eax				; save list handle
	mov	eax,[eax.IRQHandle]		; get irq handle
	or	eax, eax
	jz	@F
	VxDCall	VPICD_Physically_Unmask		; unmask it
@@:
	pop	eax
	VMMCall	List_Get_Next
	jmp	NNE_Loop

NNE_Done:
	clc
	ret

EndProc Serial_Not_Executeable

VxD_Locked_Code_Ends

	end
