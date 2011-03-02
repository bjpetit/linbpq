	PAGE	58,132
;******************************************************************************
; SERFUNC.ASM	- Port driver for 8250/16550AFN.
;******************************************************************************
;
; (C) Copyright Microsoft Corp. 1992-1995
;
; TITLE:	SERFUNC.ASM
;
; Version:	1.0
;
; Date:		01/02/93
;
; Author:	sandeeps
;
;==============================================================================
;
; Change log:
;
;==============================================================================

	.386p

;******************************************************************************
;			INCLUDE files
;******************************************************************************

	.xlist
        include VMM.INC
        include VCOMM.INC
	include	DEBUG.INC
	include	OPTTEST.INC
	include	VPICD.INC
	include	INTERNAL.INC
	include	INS8250.INC
	include	SHELL.INC
        INCLUDE MSGMACRO.INC
	INCLUDE	VCOMMW32.INC    
        .list

;******************************************************************************
;			LOCKED DATA
;==============================================================================

VxD_Locked_Data_Seg

	EXTRN	PortInfoHandle:DWORD
	EXTRN	SysVMHandle:DWORD  
	EXTRN	pGetModemStatus_PrevHook:DWORD  
	EXTRN	pEscapeCommFunction_PrevHook:DWORD

Serial_IRQ_Desc	VPICD_IRQ_Descriptor <0>

	PUBLIC	TimeOutHandle

TimeOutHandle	dd	0

VxD_Locked_Data_Ends

VxD_My_Pageable_Data_Seg

myCommprop label	DWORD 
	dw	SIZE _COMMPROP			; length of struct (mult of 4)
	dw      2				; version
	dd	SP_SERIALCOMM			; services provided
	dd	0				; dwReserved1
	dd	0				; dwMaxTxQueue
	dd	0				; dwmaxRxQueue
	dd	10000000h			; dwMaxBaud
	dd	PST_RS232			; specific COMM provider type
	dd	0FFh				; flow control caps
	dd	007Fh				; dwSettable params
	dd	1000FFFFh			; dwSettableBaud
	dw	0Fh				; wSettableDataBits
	dw	01F07h				; wSettableParityStop
	dd	0				; dwCurrentTxQueue
	dd	0				; dwCurrentRxQueue
	dd	0				; dwProvSpec1
	dd	0				; dwProvSpec2
	dw	0				; wcProvChar1
	dw	0				; filler
.errnz ($ - myCommProp - SIZE _COMMPROP)

PM_Function_Dispatch label 	DWORD
	dd	OFFSET32	pmD1query
    dd	OFFSET32	pmD2query
    dd	OFFSET32	pmD3query
    dd	OFFSET32	pmD1cancel
    dd	OFFSET32	pmD2cancel
    dd	OFFSET32	pmD3cancel
    dd	OFFSET32	pmSetD0
    dd	OFFSET32	pmSetD1
    dd	OFFSET32	pmSetD2
    dd	OFFSET32	pmSetD3
    dd	OFFSET32	pmArmWakeup
    dd	OFFSET32	pmDisArmWakeup   

VxD_My_Pageable_Data_Ends

;******************************************************************************
;			PAGEABLE CODE
;==============================================================================

VxD_My_Pageable_Code_Seg

	EXTRN	GetCOMPort:NEAR
	EXTRN	ReleaseCOMPort:NEAR
	EXTRN	TXI:NEAR
	EXTRN	SetCom100:NEAR
	EXTRN	SetCom200:NEAR
	EXTRN	SetCom300:NEAR
	EXTRN	SetCom400:NEAR
	EXTRN	TrmCom:NEAR
	EXTRN	StaCom:NEAR
	EXTRN	ExtnFcn:NEAR
	EXTRN	Flush:NEAR
	EXTRN	ReadCommString:NEAR
	EXTRN	WriteCommString:NEAR    
	EXTRN	UnMaskIRQ:NEAR
	EXTRN	TimerProc:NEAR
	EXTRN	StealPort:NEAR
	EXTRN	Notify_Owner:NEAR
	EXTRN	DeAllocData:NEAR
	EXTRN	StrCmp:Near
	EXTRN	KickTx:NEAR

BeginDoc
;******************************************************************************
;
; PPortInformation
; PortOpen (char *PortName, ulong Mode, ulong VMId, ulong *lpError);
;
; Open a port. This assumes that VCOMM called here only
; when it knows that we can support the port. It does not validate 
; parameters because of this assumption.
;
; Parameters:
;		PortName	-> Non-null if open port using name
;		VMId		-> Id of the caller
;		lpError		-> location to update error in
;
; Returns:
;		PortHandle if successful, else 0
;		Updates to *lpError are
;		IE_OPEN		port already open
;		IE_HARDWARE	hardware error
;		IE_DEFAULT	generic error
;==============================================================================
EndDoc
BeginProc PortOpen, CCALL, PUBLIC

ArgVar	PortName,DWORD
ArgVar	VMId,DWORD
ArgVar	lpError,DWORD

	EnterProc
	push	esi
	push	edi
	push	ebx
	
	SaveReg <ad>
		Debug_Printf	"BPQVC PortOpen %s\n", <PortName>
	RestoreReg <ad>

	mov	edi,PortName		; we are looking for this name.
	mov	esi,[PortInfoHandle]
	VMMCall	List_Get_First

PO_SearchNameLoop:
	jz	InitCom27		; end of list...
	call	StrCmp
	jz	PO_FoundList
	VMMCall	List_Get_Next
	jmp	PO_SearchNameLoop

PO_FoundList:

	SaveReg <ad>
	Debug_Printf	"BPQVC PortOpen Found %s\n", <PortName>
	RestoreReg <ad>

	mov	esi,eax
	cmp	[esi.pData.ValidPortData],'SMTF' ; Q: Is port already open ?
	je	InitCom17		;  Y: say so

	mov	eax,VMId		; get port for this VM
	call	GetCOMPort		; ask Mr. VCOMM to asign the port
	jnc	InitCom35		; continue
	jc	InitCom17		; did not give us the port.

InitCom26:
	call	ReleaseCOMPort		; give it back to VCD

InitCom27:

	SaveReg <ad>
	Debug_Printf	"BPQVC PortOpen Falied %s\n", <PortName>
	RestoreReg <ad>
	
	mov	eax,IE_HARDWARE		; report lack of hardware
	jmp	PO_Done

InitCom17:
	mov	eax,IE_OPEN
	cmp	[esi.Port],-1		; Q: Does port exist ?
	jne	PO_Done			;   Y: return IE_OPEN
	jmp	InitCom27		;   N: return IE_HARDWARE

InitCom35:
;	mov	edx, [esi.Port]
;	add	dl, 2			; ACE_FCR
;	in	al, dx
;	test	al, 30h			; Q: Is the port there ?
;	jnz	InitCom26		;    N: give it back to VCD.

 ;
 ; Zero Queue counts and indices.
 ;
	lea	edi,[esi.pData.QInCount]
	xor	eax,eax
	mov	ecx,(_PortData.ValidPortData-_PortData.QInCount)/4
	cld
	rep	stosd

	.errnz	_PortData.QInGet-_PortData.QInCount-4
	.errnz	_PortData.QInPut-_PortData.QInGet-4
	.errnz	_PortData.QOutCount-_PortData.QInPut-4
	.errnz	_PortData.QOutGet-_PortData.QOutCount-4
	.errnz	_PortData.QOutPut-_PortData.QOutGet-4
	.errnz	_PortData.ValidPortData-_PortData.QOutPut-4

	mov	[esi.HSFlag],al		; show no handshaking yet
	mov	[esi.MiscFlags],al	; show no discarding
	mov	[esi.pData.LossByte],al	; reset who owns the port
	mov	[esi.SendTrigger],eax
	mov	[esi.NotifyFlags],eax	; reset all flags
	dec	eax
	mov	[esi.RecvTrigger],eax
	mov	[esi.ComDCB.XonLim],eax
	mov	[esi.ComDCB.XoffLim],eax
	mov	[esi.ComDCB.BitMask],fBinary

	lea	edi,[esi.pData.dwLastReceiveTime]
	mov	[esi.RxTimeAddr],edi
	lea	edi,[esi.pData.dwDetectedEvents] ; initial AddrEvtDWord
	mov	[esi.AddrEvtDword],edi	; set it
	lea	edi,[esi.pData.bMSRShadow] ; initial AddrMSRShadow
	mov	[esi.AddrMSRShadow],edi	; set it
	
	mov al, ACE_CTS + ACE_DSR +  ACE_RLSD
	mov	BYTE PTR [edi], al	; Set initial Status

	and	[esi.EFlags],fEFlagsMask ; clear internal state
	or	[esi.EFlags], fFIFOForcedOff
	xor	eax,eax			; no errors
	mov	[esi.pData.dwCommError],eax ; reset pending bogus error codes

PO_Done:
	mov	ebx,lpError
	mov	[ebx],eax		; save error for caller
	or	eax,eax			; Q: Is everything ok ?
	jnz	PO_Failure		;    N: get out
	mov	ebx,VMId		;    Y: get owner of VM
	mov	[esi.OwnerVM],ebx	; save it
	mov	[esi.pData.dwLastError],eax ; save error code
	mov	edx, [esi.Port]
	add	dl, ACE_MCR
	and	[esi.pData.dwCommError], 0
	mov	eax, esi
	SaveReg <ad>
	Debug_Printf	"BPQVC PortOpen OK %s Handle %x", <PortName, esi>
	RestoreReg <ad>
	jmp	PO_Success

PO_Failure:
	SaveReg <ad>
	Debug_Printf	"BPQVC PortOpen Falied %s", <PortName>
	RestoreReg <ad>

	xor	eax,eax			; FALSE

PO_Success:
	pop	ebx
	pop	edi
	pop	esi
	LeaveProc
	return

EndProc PortOpen


BeginDoc
;******************************************************************************
;
; BOOL
; PortGetState(HPORT hPort, DCB *pDcb);
;
; Parameters:
;		hPort	=  port handle
;		pDcb	-> DCB to fill in
; Returns:
;		TRUE if OK, else false.
;==============================================================================
EndDoc
BeginProc PortGetState, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	pDcb,DWORD

	EnterProc

	push	esi
	push	edi
	push	ebx

	xor	eax,eax				; access to 0
	mov	esi,hPort
	mov	[esi.pData.dwLastError],eax	; no error
	lea	esi,[esi.ComDCB]
	mov	edi,pDcb
	mov	ecx,(SIZE _DCB)/4
	rep	movsd

	inc	eax				; success

	pop	ebx
	pop	edi
	pop	esi

	LeaveProc
	return

EndProc PortGetState

BeginDoc
;******************************************************************************
;
; BOOL
; PortClose(HPORT hPort);
;
; Parameters:
;		hPort	= port handle
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortClose, CCALL, PUBLIC

ArgVar	hPort,DWORD

	EnterProc
	
	SaveReg <ad>
	Debug_Printf	"BPQVC PortClose %x", <hPort>
	RestoreReg <ad>

	push	esi
	push	edi
	push	ebx

	mov	esi,hPort		; esi -> port information
	call	TrmCom

PClose_Done:
	mov	[esi.pData.dwLastError],eax

	test	[esi.MiscFlags],RxQInternal	; N: free if we alloced them
	jz	PClose_RxFree
	VMMCall	_HeapFree,<[esi.pData.QInAddr],0>

PClose_RxFree:
	test	[esi.MiscFlags],TxQInternal
	jz	PClose_TxFree
	VMMCall	_HeapFree,<[esi.pData.QOutAddr],0>

PClose_TxFree:

	mov	ebx,esi			; save PortInformation struct.
	SetFlag	[esi.MiscFlags], ClrTimer ; remove this port from consideration
	call	ManageTimer		; ManageTimer SAVES EBX.

	call	DeAllocData

	or	al,1			; success

	pop	ebx
	pop	edi
	pop	esi

	LeaveProc
	return

EndProc PortClose

BeginDoc
;******************************************************************************
;
; BOOL
; PortSetModemStatusShadow(HPORT hPort, ulong EventMask, BYTE *MSRShadow);
;
; Parameters:
;		hPort	= port handle
;		EventMask = event mask
;		MSRShadow = Addr to update WIN 3.1 style MSRShadow.
;
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortSetModemStatusShadow, esp, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	MSRShadow,DWORD

	EnterProc
	
	SaveReg <ad>
	Debug_Printf	"BPQVC PortSetModemStatusShadow %x", <MSRShadow>
	RestoreReg <ad>

	mov	edx,hPort
	mov	ecx,MSRShadow
	mov	[edx.AddrMSRShadow],ecx
	
	or	ecx,ecx
	jz @f
	
	mov al, ACE_CTS + ACE_DSR +  ACE_RLSD
	mov	BYTE PTR [ecx], al	; Set initial Status
@@:
	xor	eax,eax				; access to 0
	mov	[edx.pData.dwLastError],eax
	inc	eax
	LeaveProc
	return

EndProc PortSetModemStatusShadow

BeginDoc
;******************************************************************************
;
; BOOL
; PortGetProperties(HPORT hPort, COMMPROP *pCommprop);
;
; Parameters:
;		hPort	= port handle
;		pCommprop -> property structure to fill in
;
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortGetProperties, esp, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	pCommprop,DWORD

	EnterProc

	SaveReg	<esi,edi>

	mov	esi,OFFSET32 myCommProp
	mov	edi,pCommProp
	mov	eax,edi				; save ptr
	mov	ecx,SIZE _COMMPROP
	rep	movsb
	mov	edx,hPort			; edx -> hPort
	mov	ecx,[edx.pdata.QInSize]		; return the current q sizes.
	mov	[eax._COMMPROP.dwCurrentRxQueue],ecx
	mov	ecx,[edx.pdata.QOutSize]
	mov	[eax._COMMPROP.dwCurrentTxQueue],ecx
	xor	eax,eax
	mov	[edx.pData.dwLastError],eax	; EAX is 0, no error
	inc	eax

	RestoreReg <edi,esi>
	LeaveProc
	return

EndProc PortGetProperties

BeginDoc
;******************************************************************************
;
; BOOL
; PortSetEventMask(HPORT hPort, long EventMask, WORD *EventMaskLocation);
;
; Parameters:
;		hPort			= port handle
;		EventMask		= event mask to set
;		EventMaskLocation	= location of event mask DWORD.
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortSetEventMask, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	EventMask,DWORD
ArgVar	EventMaskLoc,DWORD

	EnterProc
		
	mov	edx,hPort
	mov	eax,EventMask		; get event mask to set
	mov	[edx.EvtMask],eax	; set it
	mov	ecx,EventMaskLoc	; get new location of event dword
	jecxz	PSEM_NoNewLoc		; set it only if non-zero
	mov	[edx.AddrEvtDWord],ecx
	
	mov	eax,EV_CTS+EV_DSR+EV_RLSD
	mov	ebx,[edx.AddrEvtDWord]
	or	dword ptr [ebx],eax

PSEM_NoNewLoc:
	xor	eax,eax			; access to 0
	mov	[edx.pData.dwLastError],eax ; no error
	inc	eax

	LeaveProc
	return

EndProc PortSetEventMask

BeginDoc
;******************************************************************************
;
; BOOL
; PortEnableNotification(HPORT hPort, PFN lpFunc, ulong RefData);
;
; Parameters:
;		hPort	= port handle
;		lpFunc	-> pointer to function to call
;		RefData	= pass in this data while calling
;
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortEnableNotification, CCALL, esp, PUBLIC

ArgVar	hPort,DWORD
ArgVar	lpFunc,DWORD
ArgVar	RefData,DWORD

	EnterProc
	
	SaveReg <ad>
	Debug_Printf	"BPQVC PortEnableNotification %x %x", <hPort, lpFunc>
	RestoreReg <ad>

	mov	edx,hPort
	mov	ecx,lpFunc
	mov	eax,RefData
	and	[edx.NotifyFlagsHI], NOT CN_Notify ; assume turning off notify
	mov	[edx.ReferenceData],eax	; save reference data here
	mov	[edx.NotifyHandle],ecx	; save proc to call
	jecxz	@F			; Q: Is assumption correct ?
	cli
	or	[edx.NotifyFlagsHI], CN_Notify	;    N: say so
	mov	ecx,[edx.AddrEvtDword]	; get accumulated events address
	mov	eax,[edx.EvtMask]	; detected events client is
	and	eax,[ecx]		; is interested in
	sti
	or	eax,eax			; Q: Any events seen yet ?
	jz	@F			;    N:
	SaveReg	<esi>
	mov	esi,edx
	mov	eax,CN_EVENT
	call	Notify_Owner
	RestoreReg <esi>
@@:
	xor	eax,eax			; access to 0
	mov	[edx.pData.dwLastError],eax ; no error for this function
	inc	eax

	LeaveProc
	return

EndProc PortEnableNotification

;******************************************************************************
;
; BOOL
; PortSetReadCallBack(HPORT hPort, ulong RxTrigger, ulong RxCallBack,
;		      ulong RxRefData);
;
; Parameters:
;		hPort	   = port handle of an open port
;		RxTrigger  = high watermask for receive queue
;		RxCallBack = when receive population becomes greater than
;			     the RxTrigger this procedure is called.
;		RxRefData  = RxCallBack is called with this reference data
;
; Returns:
;		TRUE if OK, else FALSE.
;
;==============================================================================

BeginProc PortSetReadCallBack,CCALL,PUBLIC

ArgVar	hPort,DWORD
ArgVar	RxTrigger,DWORD
ArgVar	RxCallBack,DWORD
ArgVar	RxRefData,DWORD

	EnterProc
	push	esi
	push	ebx
	
	SaveReg <ad>
	Debug_Printf	"BPQVC PortSetReadCallBack %x %d", <hPort, RxTrigger>
	RestoreReg <ad>

	mov	ecx,RxTrigger
	mov	esi,hPort

	mov	eax,ecx
	inc	eax
	jz	psr_recv_ok
	cmp	ecx,[esi.pData.QInSize]	; Q: receive threshold reasonable ?
	jb	psr_recv_ok		;    Y:
	mov	ecx,[esi.pData.QInSize]	;    N:

psr_recv_ok:
	mov	ebx,RxCallBack
	or	ebx,ebx			; Q: null read callback
	jnz	psr_save_read_threshold
	or	ecx,-1

psr_save_read_threshold:
	cli
	mov	[esi.ReadNotifyHandle],ebx
	mov	edx,RxRefData
	mov	[esi.ReadNotifyRefData],edx
	mov	[esi.RecvTrigger],ecx
	sti
	cmp	[esi.pData.QInCount],0	; Q: Does input queue contain data ?
	jne	@F			;    Y: keep dwLastReceiveTime.
	mov	esi,[esi.RxTimeAddr]
	mov	DWORD PTR [esi],0
@@:
	call	ManageTimer

	or	al,1			; success

	pop	ebx
	pop	esi

	LeaveProc
	return

EndProc PortSetReadCallBack


;******************************************************************************
;
; BOOL
; PortSetWriteCallBack(HPORT hPort, ulong TxTrigger, ulong TxCallBack,
;		      ulong TxRefData);
;
; Parameters:
;		hPort	   = port handle of an open port
;		TxTrigger  = low watermask for xmit queue
;		TxCallBack = when xmit population becomes less than
;			     the TxTrigger this procedure is called.
;		TxRefData  = TxCallBack is called with this reference data
;
; Returns:
;		TRUE if OK, else FALSE.
;
;==============================================================================

BeginProc PortSetWriteCallBack,CCALL,PUBLIC

ArgVar	hPort,DWORD
ArgVar	TxTrigger,DWORD
ArgVar	TxCallBack,DWORD
ArgVar	TxRefData,DWORD

	EnterProc

	SaveReg <ad>
	Debug_Printf	"BPQVC PortSetWriteCallBack Trigger %d Callback %x ",<TxTrigger, TxCallBack>
	RestoreReg <ad>


	push	esi
	push	ebx

	mov	edx,TxTrigger
	mov	esi,hPort

	inc	edx				; TxTrigger = -1 => no notify
	jz	psw_send_ok
	dec	edx
	TestMem	[esi.MiscFlags],TxQSet	; Q: output buffer set ?
	jz	psw_send_ok		;    N:
	cmp	edx,[esi.pData.QOutSize]	; Q: send threshold reasonable?
	jbe	psw_send_ok
	mov	edx,[esi.pData.QOutSize]

psw_send_ok:
	mov	ebx,TxCallBack
	or	ebx,ebx
	jnz	psw_save_write_threshold
	xor	edx,edx

psw_save_write_threshold:
	cli
	mov	[esi.SendTrigger],edx
	mov	[esi.WriteNotifyHandle],ebx
	mov	ebx,TxRefData
	mov	[esi.WriteNotifyRefData],ebx
	cmp	[esi.pData.QOutCount],edx	; Q: Is QOutCount >= trigger ?
	jae	psw_sendXmitWhenWMHit		;   Y: send notification when
						;    we fall below the trigger
	or	[esi.NotifyFlagsHI],CN_TRANSMIT	;   N: we are below the trigger
						;      at this point, don't
						;      generate notification
						;      till we reach above it.
psw_sendXmitWhenWMHit:
	sti
	or	al,1				; success

	pop	ebx
	pop	esi
		
	LeaveProc
	return

EndProc PortSetWriteCallBack

;******************************************************************************
;
; BOOL PortGetModemStatus(HPORT hPort, DWORD *pModemStatus);
;
; Parameters:
;		hPort -> port handle
;		pModemStatus -> return current modem status
;
; Exit:
;	if True, *pModemStatus contains the status
;
;==============================================================================
BeginProc PortGetModemStatus, CCALL, esp, PUBLIC

ArgVar	hPort,DWORD
ArgVar	pModemStatus,DWORD

	EnterProc
	
	mov	edx,hPort
	mov	ecx, [edx.AddrMSRShadow]

	mov al, ACE_CTS + ACE_DSR +  ACE_RLSD

	mov	BYTE PTR [ecx], al	; save Modem status shadow.
	and	eax,MS_Modem_Status	; isolate bits of interest
	mov	edx,pModemStatus	; edx -> fill data here
	mov	dword ptr [edx],eax	; fill it
	
	SaveReg <ad>
	Debug_Printf	"BPQVC GetModemStatus %x",<eax>
	RestoreReg <ad>

	or	al,1			; success
	
	LeaveProc
	return

	
	
	
	
	
	
	

EndProc PortGetModemStatus

;******************************************************************************
;
; BOOL
; PortGetCommConfig(HPORT hPort, LPCOMMCONFIG lpCC, LPDWORD lpSize);
;
; Description:
;
;	If (*lpSize >= SIZE _COMM_CONFIG) then return the config
;	 else fail it and return SIZE _COMM_CONFIG in *lpSize.
;
;	NOTE: The _COMM_CONFIG structure includes a Win32 style DCB
;	      and NOT a ring0 DCB. So we need to call VCOMM to convert
;	      our DCB into Win32 style one.
;
; Parameters:
;		hPort = port handle
;		lpCC  -> LPCOMMCONFIG
;		lpSize -> fill it with size of DCB
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
BeginProc PortGetCommConfig,CCALL,esp,PUBLIC

ArgVar	hPort,DWORD
ArgVar	lpCC,DWORD
ArgVar	lpSize,DWORD

	EnterProc

	xor	eax,eax				; assume failure
	mov	edx,lpSize
	push	SIZE _COMM_CONFIG
	pop	ecx
	cmp	DWORD PTR [edx], ecx		; Q: Is buffer large enough?
	mov	DWORD PTR [edx], ecx

%OUT BUGBUG Not certain about error codes here (new API).

	jb	PGCC_Success			;    N: just return size
	mov	edx,lpCC			; EDX -> _COMM_CONFIG
	or	edx,edx				; Q: Is LPCOMMCONFIG 0 ?
	jz	PGCC_Success			;    Y: just return size
	mov	[edx._COMM_CONFIG.cc_dwProviderOffset],eax
	mov	[edx._COMM_CONFIG.cc_dwProviderSize],eax
	mov	[edx._COMM_CONFIG.cc_wcProviderData],al

	mov	al,SIZE _COMM_CONFIG
	.errnz	(SIZE _COMM_CONFIG) AND 0FFFFFF00h
	mov	[edx._COMM_CONFIG.cc_dwSize],eax

	mov	al,PST_RS232
	.errnz	PST_RS232 AND 0FFFFFF00h
	mov	[edx._COMM_CONFIG.cc_dwProviderSubType],eax

	shl	eax,8
	.errnz	PST_RS232 - 1
	mov	[edx._COMM_CONFIG.cc_wVersion],ax ; version = 1.0000

	mov	eax,hPort
	lea	eax,[eax.ComDCB]		; EAX -> Ring0DCB
	lea	edx,[edx._COMM_CONFIG.cc_dcb]	; EDX -> Win32 DCB.

	VxDCall	VCOMM_Map_Ring0DCB_To_Win32	; EAX -> Ring0DCB, EDX -> Win32

PGCC_Success:
	or	al,1				; success

PGCC_Done:
	LeaveProc
	return

EndProc PortGetCommConfig

;******************************************************************************
;
; BOOL
; PortSetCommConfig(HPORT hPort, LPCOMMCONFIG lpCC, DWORD dwSize);
;
;	NOTE: The _COMM_CONFIG structure includes a Win32 style DCB
;	      and NOT a ring0 DCB. So we need to call VCOMM to convert
;	      it to Ring0 style DCB.
;
; Parameters:
;		hPort = port handle
;		lpCC  -> LPCOMMCONFIG
;		dwSize -> size to copy
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
BeginProc PortSetCommConfig,CCALL,esp,PUBLIC

ArgVar	hPort,DWORD
ArgVar	lpCC,DWORD
ArgVar	dwSize,DWORD

LocalVar lDcb, %(size _DCB)

	EnterProc

	xor	eax,eax
	mov	edx,hPort
	mov	[edx._PortData.dwLastError],IE_INVALIDPARAM

	cmp	dwSize,SIZE _COMM_CONFIG	; Q: Valid lpCommConfig ?
	jb	PSCC_Done			;    N:

	lea	edx,lDcb			; EDX -> ring 0 DCB
	mov	eax,lpCC
	lea	eax,[eax._COMM_CONFIG.cc_dcb]	; EAX -> ring 3 DCB
	VxDCall	VCOMM_Map_Win32DCB_To_Ring0

	cCall	PortSetState,<hPort,edx,-1>	; call to set state

PSCC_Done:
	LeaveProc
	return

EndProc PortSetCommConfig

;******************************************************************************
;
; BOOL
; PortGetWin32Error(HPORT hPort, ulong *dwError);
;
; Parameters:
;	hPort -> Port handle
;	dwError -> SERIAL.386 fills this with Win32 style error
;
; Returns: TRUE
; Uses: C style
;==============================================================================
BeginProc PortGetWin32Error,CCALL,esp,PUBLIC

ArgVar	hPort,DWORD
ArgVar	dwError,DWORD

	EnterProc

	LeaveProc
	return

EndProc PortGetWin32Error

VxD_My_Pageable_Code_Ends

;******************************************************************************
;			LOCKED CODE
;==============================================================================

VxD_Locked_Code_Seg

BeginDoc
;******************************************************************************
;
; BOOL
; PortGetQueueStatus(HPORT hPort, COMSTAT *pComstat);
;
; Parameters:
;		hPort	= port handle
;		pComstat -> status structure of queue to fill in.
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortGetQueueStatus, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	pComstat,DWORD

	EnterProc

	push	esi
	push	ebx
	mov	esi,hPort
	mov	ebx,pComstat
	call	StaCom

	xor	eax,eax
	mov	[esi.pData.dwLastError],eax
	inc	eax

	pop	ebx
	pop	esi
	LeaveProc
	return

EndProc PortGetQueueStatus

BeginDoc
;******************************************************************************
;
; BOOL
; PortSetState(HPORT hPort, DCB *pDcb, long ActionMask);
;
; Parameters:
;		hPort		-> Port handle
;		pDcb		-> DCB to set from
;		ActionMask	= dword specifying relevant DCB fields.
; Returns:
;		TRUE if OK, else false.
;==============================================================================
EndDoc
BeginProc PortSetState, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	pDcb,DWORD
ArgVar	ActionMask,DWORD

LocalVar BaudRateChange,DWORD	

	EnterProc
	
	SaveReg <ad>
	Debug_Printf	"BPQVC PortSetState"
	RestoreReg <ad>

	SaveReg <esi>
	SaveReg <edi>
	SaveReg <ebx>

	cld
	mov	esi,hPort		; ESI -> PortInformation

	test	[esi.pData.LossByte],1	; Q: Do we own the port
	jz	PSS_SerialOwnsPort	;    Y: continue
	call	StealPort		;    N: try to steal it
	mov	eax,IE_Default		; assume failure to steal
	jz	PSS_Done		; jump if failed

PSS_SerialOwnsPort:
	mov	ebx,pDcb		; EBX -> DCB
 ;
 ; Parameters seem correct. Copy relevant DCB fields into our
 ; space and initialize ACE with relevant parameters.
 ;
	mov	ecx, [ebx._DCB.BaudRate]
	sub	ecx, [esi.ComDCB.BaudRate]
	mov	BaudRateChange, ecx

	mov	ecx,ActionMask
	call	SetCom100		; Copy DCB selectively!
	lea	ebx,[esi.ComDCB]	; set EBX -> DCB

	TestMem	ActionMask,fTimeout	; is caller interested in timeouts ?
	jz	PSS_NoTimeoutSet
	call	SetCom200		; get timeout masks, ah = check, al=inf
	xchg	al,ah			; get them in correct registers
	mov	word ptr [esi.MSRMask],ax
	.errnz	MSRInfinite-MSRMask-1

PSS_NoTimeoutSet:


; Compute the queue count where XOff should be issued (or hardware
; lines dropped).  This will prevent having to do it at interrupt
; time.

	mov	eax,[esi.pData.QInSize]	; get where they want it
	sub	eax,[esi.ComDCB.XoffLim];  and compute queue count
	mov	[esi.XoffPoint],eax

;
; Win 3.0 didn't check hardware handshaking until the line status changed.
; Allow some apps to keep that behavior.
;
	TestMem	[esi.ComDCB.BitMask], fWin30Compat
	jnz	sc_HHSup
;
; HACK FOR SOME MODEMS:  apparently some modems set CTS, but don't set DSR
; which means that COMM.DRV won't send if the app specifies that hardware
; handshaking is based on CTS & DSR being set.
;
	mov	eax,[esi.AddrMSRShadow]
	mov	al,[eax]		; get the shadow
	mov	ah,[esi.OutHHSLines]
	and	al,ah			;Only leave bits of interest
	cmp	al, ah			;Q: handshaking lines ok?
	je	sc_HHSup		;   Y:
	cmp	ah, ACE_CTS OR ACE_DSR	;Q: app looking for both high?
	jne	sc_HHSdown		;   N: skip hack
	test	[esi.EFlags], fUseDSR	;Q: DSR is always significant?
	jnz	sc_HHSdown		;   Y: skip hack
	cmp	al, ACE_CTS		;Q: DSR low & CTS high
	jne	sc_HHSdown		;   N: skip hack
	and	ah, NOT ACE_DSR 	;   Y: ignore DSR line
	mov	[esi.OutHHSLines], ah
	jmp	sc_HHSup

sc_HHSdown:
	or	[esi.HSFlag], HHSDown OR HHSAlwaysDown ; flag handshaking down

sc_HHSup:

	xor	eax,eax 		;All done

PSS_Done:
	mov	[esi.pData.dwLastError],eax ; save error
	or	eax,eax			; any errors ?
	jnz	PSS_Failed

PSS_IRQUnmasked:
	SetFlag	[esi.MiscFlags], SetComStateDone
	or	al,1
	jmp	PSS_Success

PSS_Failed:
	xor	eax,eax

PSS_Success:

	SaveReg <ad>
	Debug_Printf	"BPQVC PortSetState Returns %d", <[esi.pData.dwLastError]>
	RestoreReg <ad>

	RestoreReg <ebx>
	RestoreReg <edi>
	RestoreReg <esi>
	LeaveProc
	return

EndProc PortSetState

BeginDoc
;******************************************************************************
;
; BOOL
; PortSetup(HPORT hPort, BYTE *RxBase, ulong RxLength, BYTE *TxBase,
;	    ulong TxLength);
;
; Parameters:
;		hPort	= port handle
;		RxBase	= receive queue base
;		RxLength = receive queue's length
;		TxBase	= base of transmit queue
;		TxLength = transmit queue length
;
; Returns:
;		TRUE if OK, else false.
;==============================================================================
EndDoc
BeginProc PortSetup, CCALL, esp, PUBLIC

ArgVar	hPort,DWORD
ArgVar	RxBase,DWORD
ArgVar	RxLength,DWORD
ArgVar	TxBase,DWORD
ArgVar	TxLength,DWORD

	EnterProc
	
	mov	eax,TxLength
	
	SaveReg	<edi>
	
	SaveReg <ad>
	Debug_Printf	"BPQVC PortSetUp %x %d %x %d",<TxBase, eax, RxBase, RxLength>
	RestoreReg <ad>


	mov	edx,hPort		; esi -> PortInformation struct
	lea	edi,[edx.pData.QInCount]
	mov	ecx, (_PortData.ValidPortData - _PortData.QInCount)/4
	xor	eax,eax
	mov	[edx.pData.dwLastError],eax	; no error

	rep	stosd			; reset all counts

	.errnz	_PortData.QInGet-_PortData.QInCount-4
	.errnz	_PortData.QInPut-_PortData.QInGet-4
	.errnz	_PortData.QOutCount-_PortData.QInPut-4
	.errnz	_PortData.QOutGet - _PortData.QOutCount -4
	.errnz	_PortData.QOutPut - _PortData.QOutGet - 4
	.errnz	_PortData.ValidPortData - _PortData.QOutPut - 4

	mov	edi,edx			; EDI -> port information structure

	mov	eax,RxBase		; get base of receive queue
	or	eax,eax			; Q: external buffer ?
	jnz	PS_NoAllocRxBuffer	;  Y:
	test	[edi.MiscFlags],RxQInternal ; Q: already alloced internal buf ?
	jz	PS_AllocRxBuffer
	VMMCall	_HeapReallocate,<[edi.pData.QInAddr],RxLength,0>
	or	eax,eax
	jz	PS_Done
	jnz	PS_NoAllocRxBuffer

PS_AllocRxBuffer:
	VMMCall	_HeapAllocate,<RxLength,0>
	or	eax,eax
	jz	PS_Fail
	or	[edi.MiscFlags],RxQInternal

PS_NoAllocRxBuffer:
	mov	[edi.pData.QInAddr],eax
	mov	eax,RxLength		; get length
	mov	[edi.pData.QInSize],eax

	mov	eax,TxBase		; get base of transmit q
	or	eax,eax			; Q: external buffer ?
	jnz	PS_NoAllocTxBuffer	;  Y:
	cmp	TxLength,0		;  N: Q: Is desired length 0 ?
	jz	PS_XmitBufferEmpty	;     Y: No buffer.....

	test	[edi.MiscFlags],TxQInternal
	jz	PS_AllocTxBuffer
	VMMCall	_HeapReallocate,<[edi.pData.QOutAddr],TxLength,0>
	or	eax,eax
	jz	PS_Done
	jnz	PS_NoAllocTxBuffer

PS_AllocTxBuffer:
	VMMCall	_HeapAllocate,<TxLength,0>	; try allocation
	or	eax,eax				; Q: success ?
	jnz	PS_SetTxInternalFlag		;    Y: set flag and continue

PS_Fail:
	test	[edi.MiscFlags],RxQInternal
	jz	PS_Fail1
	and	[edi.MiscFlags],NOT RxQInternal
	VMMCall	_HeapFree,<[edi.pData.QInAddr],0>

PS_Fail1:
	xor	eax,eax
	jmp	PS_Done

PS_SetTxInternalFlag:
	or	[edi.MiscFlags],TxQInternal

PS_NoAllocTxBuffer:
	SetFlag	[edi.MiscFlags],TxQSet

PS_XmitBufferEmpty:
	mov	[edi.pData.QOutAddr],eax
	mov	eax,TxLength		; get length
	mov	[edi.pData.QOutSize],eax

	mov	eax,1
	or	eax,eax

PS_Done:
	RestoreReg <edi>

	SaveReg <ad>
	Debug_Printf	"BPQVC PortSetUp Exit %x",<eax>
	RestoreReg <ad>


	LeaveProc
	return

EndProc PortSetup

BeginDoc
;******************************************************************************
;
; BOOL
; PortTransmitChar(HPORT hPort, char cbyte);
;
; Parameters:
;		hPort	= port handle
;		cbyte	= char to transmit
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortTransmitChar, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	cbyte,BYTE

	EnterProc
	push	esi
	push	ebx

	mov	esi,hPort		; ESI -> Portinformation
	mov	eax,4000h		; in case we cannot send
	TestMem	[esi.EFlags],fTxImmed	; is another "Immediate" char waiting?
	jnz	PTC_Done		; yes, return error
	mov	ah,cbyte		; set char for TXI
	cli
	call	TXI			; set char to tx immediately
	sti
	xor	eax,eax			; ALL OK
PTC_Done:
	mov	[esi.pData.dwLastError],eax
	or	eax,eax			; all OK ?
	mov	eax,VMM_TRUE		; assume OK
	jz	PTC_Success
	xor	eax,eax			; NO

PTC_Success:
	pop	ebx
	pop	esi
	LeaveProc
	return

EndProc	PortTransmitChar

BeginDoc
;******************************************************************************
;
; BOOL
; PortClearError(HPORT hPort, COMSTAT *pComstat, ulong *pError);
;
; Parameters:
;		hPort	= port handle
;		pComstat -> status structure to fill in
;		pError	-> location to fill in error condition (reset)
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortClearError, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	pComstat,DWORD
ArgVar	pError,DWORD

	EnterProc
	push	esi
	push	ebx

	mov	esi,hPort
	mov	ebx,pComstat
	call	StaCom			; call worker

	xor	eax,eax
	mov	[esi.pData.dwLastError],eax
	xchg	eax,[esi.pData.dwCommError]	; return old error and clear
	mov	edx,pError
	mov	DWORD PTR [edx],eax		; return old error here
	or	al,1				; success

	pop	ebx
	pop	esi
	LeaveProc
	return

EndProc PortClearError

BeginProc	RestorePortState

	EnterProc
    ArgVar	hPort, DWORD
    
    push	esi
    mov		esi, hPort
    lea		esi, [esi.ComDcb]
    
	cCall	PortSetState,<hPort,esi,-1>	; call to set state   
    
    pop		esi    
    LeaveProc
    Return
    
EndProc		RestorePortState

BeginDoc
;******************************************************************************
;
; BOOL
; PortEscapeFunction(HPORT hPort, long function, long Indata, long Outdata);
;
; Parameters:
;		hPort	= port handle
;		function = escape code
;		Indata	= optional data
;		Outdata	= optional data
;
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortEscapeFunction, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	function,DWORD
ArgVar	Indata,DWORD
ArgVar	Outdata,DWORD

	EnterProc
	
	SaveReg <ad>
	Debug_Printf	"BPQVC EscapeFunction %d Handle %x", <function, hPort>
	RestoreReg <ad>


	SaveReg <esi>
	SaveReg <ebx>

	mov	ebx,function
	mov	esi,hPort		; esi -> portinformation
	xor	eax,eax			; assume error
	mov	[esi.pData.dwLastError],IE_EXTINVALID
	cmp	bl, PEEKCHAR
	jne	@F
	mov	[esi.pData.dwLastError],eax
	cmp	[esi.pData.QInCount], eax
	je	PEF_Done
	mov	eax, [esi.pData.QInAddr]
	add	eax, [esi.pData.QInGet]
	mov	cl, [eax]
	mov	eax, OutData
	mov	BYTE PTR [eax], cl
	jmp	PEF_NoOutData
@@:
	cmp	bl,ENABLETIMERLOGIC
	jnz	@F
	ClrFlag	[esi.MiscFlags],ClrTimer
	jmp	PEF_NoOutData
@@:
	cmp	bl,IGNOREERRORONREADS
	jnz	@F
	SetFlag	[esi.MiscFlags],IgnoreCommError
	jmp	PEF_NoOutData
@@:
	cmp	bl,CLRTIMERLOGIC
	jnz	@F
	SetFlag	[esi.MiscFlags],ClrTimer
	jmp	PEF_NoOutData
@@:
	cmp	bl,SETUPDATETIMEADDR
	jne	@F
	mov	eax,InData
	mov	[esi.RxTimeAddr],eax
	jmp	PEF_NoOutData
@@:
	cmp	bl,CLEARBREAK
	ja	@f

	mov	[esi.pData.dwLastError],eax	; reset error
	call	ExtnFcn
	mov	ecx,OutData
	jecxz	PEF_NoOutData
	mov	[ecx],eax			; return this information
    jmp	PEF_NoOutData


@@:		
	cmp	bl, PM_QUERY_D1
    jb	PEF_Done
    
    cmp	bl, PM_DISARM_WAKEUP
    ja	PEF_Done
    
    sub	bl, PM_QUERY_D1
    mov	eax, OutData
    mov	DWORD PTR [eax], PM_SUCCESS
    xor	eax,eax    
    lea	ebx, [PM_Function_Dispatch+4*ebx]
    jmp	DWORD PTR [ebx]
    
pmD1query:  
pmD2query:	
pmD3query:
pmD1cancel:
pmD2cancel:
pmD3cancel:
    jmp	PEF_NoOutData
        
pmSetD0:	
	pushfd
    push	esi        
    cli
    call	RestorePortState            
    add		esp,4
    popfd
    
    xor		eax,eax
    jmp		PEF_NoOutData
        
pmSetD1:       
pmSetD2:       
pmSetD3:	   
    xor		eax,eax
    jmp	PEF_NoOutData
        
pmArmWakeup:       
pmDisArmWakeup:
    jmp	PEF_NoOutData
    
    
PEF_NoOutData:
	or	al,1				; success

PEF_Done:
	RestoreReg <ebx>
	RestoreReg <esi>
	LeaveProc
	return

EndProc PortEscapeFunction

BeginDoc
;******************************************************************************
;
; BOOL
; PortGetEventMask(HPORT hPort, long EventMask, long *OldEventMask);
;
; Parameters:
;		hPort	= port handle
;		EventMask = mask of events to clear
;		OldEventMask -> location where old mask should be returned.
;
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortGetEventMask, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	EventMask,DWORD
ArgVar	OldEventMask,DWORD

	EnterProc
	push	ebx

	mov	edx,hPort		; EDX -> PortInformation
	mov	[edx.pData.dwLastError],0 ; no errors here.
	mov	edx,[edx.AddrEvtDWord]	; edx -> event dword's location
	mov	ebx,EventMask		; get mask to clear
	not	ebx

	cli
	mov	eax,[edx]		; get current events
	and	ebx,eax			; clear them
	mov	dword ptr [edx],ebx	; save results
	sti
	mov	edx,OldEventMask
	mov	dword ptr [edx],eax	; return old accumulated events.

PGEM_Done:
	or	al,1			; success
	pop	ebx
	LeaveProc
	return

EndProc PortGetEventMask

BeginDoc
;******************************************************************************
;
; BOOL
; PortWrite(HPORT hPort, char *lpBuf, ulong NumBWrite, ulong *lpNumBWritten);
;
; Parameters:
;		hPort	= port handle
;		lpBuf	-> buffer to write from
;		NumBWrite = number of bytes to write
;		lpNumBWritten -> update location for number of bytes written
;
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortWrite, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	lpBuf,DWORD
ArgVar	NumBWrite,DWORD
ArgVar	lpNumBWritten,DWORD

	EnterProc
	
	SaveReg <ad>
	Debug_Printf	"BPQVC Write %d Bytes", <NumBWrite>
	RestoreReg <ad>

	SaveReg <esi>
	SaveReg <edi>
	SaveReg <ebx>

	mov	esi,hPort
	mov	ecx,NumBWrite
	mov	edi,lpBuf
	
	TestMem	[esi.MiscFlags],TxQSet
	jnz	PW_QueueData

	cli
	mov	[esi.pData.QOutAddr],edi
	mov	[esi.pData.QOutSize],ecx
	mov	[esi.pData.QOutCount],ecx
	xor	eax,eax
	mov	[esi.pData.QOutPut],eax
	mov	[esi.pData.QOutGet],eax
	cmp	ecx,[esi.SendTrigger]	; Q: have we overshot the trigger ?
	jb	PW_StillBelowTrigger	;    N: don't start looking
	and	[esi.NotifyFlagsHI], NOT CN_TRANSMIT ; start looking now

PW_StillBelowTrigger:

	sti
	SaveReg	<ecx>			; save the #bytes written
;	cli
;	call	KickTx
;	sti
	RestoreReg <eax>		; get back #bytes written
	
	SaveReg <ad>
	Debug_Printf	"BPQVC Write NOQUEUE %d Bytes Written", <eax>
	RestoreReg <ad>

	jmp	PW_BytesQueued

PW_QueueData:

	
	push	esi
	call	WriteCommString
	pop	esi
	
PW_BytesQueued:

	mov	edx,lpNumBWritten
	mov	dword ptr [edx],eax	; # bytes written
	
	SaveReg <ad>
	mov	edx,lpNumBWritten
	mov	eax,[edx]
	Debug_Printf	"BPQVC Write %d Bytes Written", <eax>
	RestoreReg <ad>

	xor	eax,eax			; access to 0
	mov	[esi.pData.dwLastError],eax
	inc	eax			; success

	RestoreReg <ebx>
	RestoreReg <edi>
	RestoreReg <esi>
	
	LeaveProc
	return

EndProc PortWrite

BeginDoc
;******************************************************************************
;
; BOOL
; PortRead(HPORT hPort, char *lpBuf, ulong NumBToRead, ulong *lpNumBRead);
;
; Parameters:
;		hPort	= port handle
;		lpBuf	-> buffer to read into
;		NumBToRead = number of bytes to read
;		lpNumBRead -> update location for number of bytes read.
;
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortRead, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	lpBuf,DWORD
ArgVar	NumBToRead,DWORD
ArgVar	lpNumBRead,DWORD

	EnterProc
	push	esi
	push	edi
	push	ebx

	mov	esi,hPort		; esi -> portinformation
	mov	edi,lpBuf		; edi -> buffer
	mov	ecx,NumBToRead		; ecx = max bytes to read
	call	ReadCommString
	mov	ecx,0			; assume no error
	jnz	PR_DataRead
		
	mov	ecx,eax			; save error/no data here
	xor	eax,eax
	
PR_DataRead:

	cmp	eax,0
	jne	@f
	
	pushad
	VMMCall Release_Time_Slice
	popad	
	
@@:

	mov	edx,lpNumBRead
	mov	dword ptr [edx],eax
	mov	[esi.pData.dwLastError],ecx ; set error
	mov	eax,VMM_TRUE		; assume success
	jecxz	PR_Done
	xor	eax,eax
PR_Done:
	pop	ebx
	pop	edi
	pop	esi
	LeaveProc
	return

EndProc PortRead

;****
;
; ManageTimer
;
; If any COMM port is open and a receive trigger is set, then starts the
; timer is not already active, else stops it
;
; Entry:
;	NONE
; Exit:
;	NONE
; Uses:
;	Everything except EDI,EBX
;
BeginProc ManageTimer,PUBLIC

	mov	esi,[PortInfoHandle]
	VMMCall	List_Get_First

MT_Loop:
	jz	MT_Cancel_Timer		; exhausted the list...
;	TestMem	[eax.MiscFlags],ClrTimer ; Q: Timer logic to be skipped ?
;	jnz	MT_Next			;  Y:
	cmp	[eax.RecvTrigger],-1	; Q: Owner wants notification?
	jne	MT_SetTimer		;  Y: Check if
MT_Next:
	VMMCall	List_Get_Next
	jnz	MT_Loop

MT_Cancel_Timer:
	xor	esi,esi
	xchg	esi,TimeOutHandle
	VMMCall	Cancel_Time_Out
	jmp	MT_Done

MT_SetTimer:
	cmp	TimeOutHandle,0		; Q: Have we set a timer already?
	jnz	MT_Done			;  Y: get out
	mov	eax,100			;  N: set a 100 ms timer
	mov	edx,esi			; List handle is ref data
	mov	esi,OFFSET32 TimerProc
	VMMCall	Set_Global_Time_Out
	mov	TimeOutHandle,esi
	jmp	MT_Done

MT_Done:
	ret

EndProc ManageTimer

BeginDoc
;******************************************************************************
;
; BOOL
; PortPurge(HPORT hPort, DWORD dwQueue);
;
; Parameters:
;		hPort	= port handle
;		dwQueue = queue type
;
; Returns:
;		TRUE if OK, else FALSE.
;==============================================================================
EndDoc
BeginProc PortPurge, CCALL, PUBLIC

ArgVar	hPort,DWORD
ArgVar	dwQueue,DWORD

	EnterProc
	
	SaveReg <ad>
	Debug_Printf	"BPQVC Flush %d Port %x ", <dwQueue, hPort>
	RestoreReg <ad>

	push	esi
	push	ebx

	mov	esi,hPort
	mov	ebx,dwQueue
	;call	Flush
PP_Done:
	xor	eax,eax				; access to 0
	mov	[esi.pData.dwLastError],eax
	inc	eax				; success

	pop	ebx
	pop	esi
	LeaveProc
	return

EndProc PortPurge

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


W98_SERIAL_IS_COM_OPEN EQU 0800H
W98_SERIAL_GETDATA     EQU 0801H
W98_SERIAL_SETDATA     EQU 0802H

W98_SERIAL_SET_CTS     EQU 0803H
W98_SERIAL_SET_DSR     EQU 0804H
W98_SERIAL_SET_DCD     EQU 0805H

W98_SERIAL_CLR_CTS     EQU 0806H
W98_SERIAL_CLR_DSR     EQU 0807H
W98_SERIAL_CLR_DCD     EQU 0808H

W98_BPQ_ADD_DEVICE     EQU 0809H
W98_BPQ_DELETE_DEVICE  EQU 080aH
W98_BPQ_LIST_DEVICES   EQU 080bH

W98_BPQ_SET_POLLDELAY	 EQU 080cH
W98_BPQ_SET_DEBUGMASK	 EQU 080dH

W98_SERIAL_GET_COMMSTATUS  EQU  27
W98_SERIAL_GET_DTRRTS      EQU  30




BeginProc VXD_IOCONTROL


        mov     EAX,dwIoControlCode[ESI]
 
  ;// DIOC_OPEN is sent when VxD is loaded w/ CreateFile 
  ;//  (this happens just after SYS_DYNAMIC_INIT)
  
		cmp		EAX, DIOC_OPEN
		jne		@f

		xor      eax,eax
        ret
   
@@:        
              
		cmp		EAX,VWIN32_DIOC_GETVERSION
		je		getvers
			
		mov	EAX,dwIoControlCode[ESI]
		SHR	EAX,16
		MOV	EBX,EAX								; Extract Port
		ADD	EBX,100H							; Convert to Virtual HW Addr

		mov	EAX,dwIoControlCode[ESI]
		AND EAX,0FFFFH
		
;		SaveReg <ad>
;		Debug_Printf	"BPQVC IOCTL Port %x Code %x", <ebx, eax>
;		RestoreReg <ad>

      
;	See if port is currently open
		
		push	esi
			
		mov	esi,[PortInfoHandle]
		VMMCall	List_Get_First
		jz	NotOpen

IF_CheckIfknownBase:

		mov	edx,[eax.Port]
		
		cmp	edx,ebx
		je	Open
		
		VMMCall	List_Get_Next
		jnz	IF_CheckIfKnownBase

NotOpen:
	
		xor      eax,eax  
		
Open:

		pop	esi
	
		; EAX is Port Cntrol Entry if open, ESI = IOCTL Struct

		mov	EBX,dwIoControlCode[ESI]
		AND	EBX,0ffffh
 
		cmp	EBX, W98_SERIAL_IS_COM_OPEN
		jne	@f

		or	eax,eax
		jz short ReturnVal		; Zero = Not Open

		mov	eax,1

ReturnVal:

	    mov     edi,lpvOutBuffer[ESI]
        mov     dword ptr [edi],eax
        mov     edi,lpcbBytesReturned[ESI]
        mov     dword ptr [edi],4                

		xor      eax,eax
        ret
		
@@:	

		or	eax,eax
		jz short ReturnVal					; All others just return 0 if port not open
			
		cmp	EBX, W98_SERIAL_SETDATA
		jne	NotSet
		
;		SaveReg <ad>
;		Debug_Printf	"BPQVC Write Vec %x Port %x Len %d ", <eax, edx, cbInBuffer[esi]>
;		RestoreReg <ad>
		
		push	esi
		
		mov	ecx,cbInBuffer[esi]
		mov	edi,lpvInBuffer[esi]
		mov	esi,eax	 ; Control
		
		push	ecx

	mov	edx,[esi.pData.QInSize] ; see if queue is full
	sub	edx,[esi.pData.QInCount] ; edx = # of chars free in queue
	
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
	mov	ebx,[esi.pData.QInAddr] ; --> output queue
	mov	edx,[esi.pData.QInSize]
	mov	edi,[esi.pData.QInPut]	; get index into queue
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
	mov	[esi.pData.QInPut],edx	; new index into queue
	add	[esi.pData.QInCount],ebx

	sti
	
	pushad
	
	SaveReg <ad>
	Debug_Printf	"BPQVC API Send RXCount %d RX Trigger %d", <[esi.pData.QInCount], [esi.RecvTrigger]>
	RestoreReg <ad>
	
	mov	eax,[esi.pData.QInCount]
	cmp	eax, [esi.RecvTrigger]	;Q: time to call owner's callback?
	jbe	short DataAvail130	;   N:

	SaveReg <ad>
	Debug_Printf	"BPQVC API Send Checking Nofify Flag %d", <[esi.NotifyFlags]
	RestoreReg <ad>


	test	[esi.NotifyFlagsHI], CN_RECEIVE
	jnz	short DataAvail140	; jump if notify already sent and
					;   data in buffer hasn't dropped
					;   below threshold
					
	SaveReg <ad>
	Debug_Printf	"BPQVC API Send Calling RX Notify"
	RestoreReg <ad>


	push	OFFSET32 DataAvail140
	mov	eax, CN_RECEIVE
	jmp	notify_owner

DataAvail130:
	and	[esi.NotifyFlagsHI], NOT CN_RECEIVE

DataAvail140:

	popad
	
	
	pop	ecx
	sub	ecx,ebx			; # of chars left to send
	jnz	scs_full_2		;  jump if none
scs_exit:
	pop	eax
	sub	eax,ecx			; EAX = # transfered
	jmp short cws_exit

scs_full:
scs_full_2:
	or	[esi.pData.dwCommError],CE_TXFULL
	jmp	scs_exit

cws_error:
	pop	eax
	sub	eax,ecx			; EAX = # transfered
	
cws_exit:

	pop	esi
	
	mov     edi,lpvOutBuffer[ESI]
        mov     dword ptr [edi],eax
        mov     edi,lpcbBytesReturned[ESI]
        mov     dword ptr [edi],4                

		xor      eax,eax
        ret
		

NotSet:		

		cmp	EBX, W98_SERIAL_GETDATA
		jne	NotGet
		
		
		push	esi
		
		mov	ecx,cbOutBuffer[esi]
		mov	edi,lpvOutBuffer[esi]
		mov	esi,eax	 ; Control

	xor	eax,eax

	or	eax,[esi.pData.QOutCount] ; any chars in input queue
	jz	RecStr90		; no chars in queue

	SaveReg <ad>
	Debug_Printf	"BPQVC Read  Len %d ", <[esi.pData.QOutCount]>
	RestoreReg <ad>

	cmp	ecx,eax			; Q:more chars available than can read?
	jbe	RecStr30		;   N:
	mov	ecx,eax			;   Y: adjust # of chars to read
RecStr30:
	push	ecx
	mov	edx,[esi.pData.QOutSize]
	mov	eax,[esi.pData.QOutGet]
	sub	edx,eax			; EDX = # of bytes before end of buf
	cmp	edx,ecx			; Q: more avail than can read?
	jbe	RecStr40		;   N:
	mov	edx,ecx			;   Y: adjust avail count
RecStr40:
	xchg	ecx,edx			; ECX = # bytes for 1st copy
	sub	edx,ecx			; EDX = # bytes for 2nd copy

	push	esi
	mov	ebx,[esi.pData.QOutAddr]
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
	mov	[esi.pData.QOutGet],ebx	; update QInGet
	sub	[esi.pData.QOutCount],ecx ; update count
;	mov	eax,[esi.pData.QOutCount]

;	and	[esi.NotifyFlagsHI],NOT CN_RECEIVE ; allow TO notify again

	sti	
	
;	SaveReg <ad>
;	Debug_Printf	"BPQVC Read Calling Notify "
;	RestoreReg <ad>


	cli
	
	cmp	[esi.WriteNotifyHandle], 0
	je @f
	
	mov	edx,[esi.SendTrigger]
	cmp	[esi.pData.QOutCount],edx	; Q: Is QOutCount >= trigger ?
	jae	@f		;   Y: send notification when  we fall below the trigger
	
	SaveReg <ad>
	Debug_Printf	"BPQVC API Read Calling Notify TX"

	
	and	[esi.NotifyFlagsHI], NOT CN_TRANSMIT ; start looking now

	mov	eax, CN_TRANSMIT	; we have fallen below the trigger
	call	Notify_Owner		; notify the owner of the port

	RestoreReg <ad>	
	
@@:

	mov	eax, ecx		; Bytes removed
 ;
 ; No characters in the input queue. 
 ;
RecStr90:
	
	pop	esi
    mov     edi,lpcbBytesReturned[ESI]
    mov     dword ptr [edi], eax                

	ret

	
NotGet:

		cmp	EBX, W98_SERIAL_GET_COMMSTATUS
		jne	NotGetStatus 
		
		push	esi
		
		mov	esi,eax	 ; Control
		
		mov	ecx,[esi.pData.QInCount];Get input queue count
		mov	edx,[esi.pData.QOutCount] ;Get tx queue count
		
		pop	esi

;	typedef struct _SERIAL_STATUS {
 ;   ULONG Errors;
;    ULONG HoldReasons;
;    ULONG AmountInInQueue;
 ;   ULONG AmountInOutQueue;
;    BOOLEAN EofReceived;
;    BOOLEAN WaitForImmediate;


		mov	edi,lpvOutBuffer[esi]
		mov	8[edi],edx
		mov	12[edi],ecx
		
		mov	ecx,cbOutBuffer[esi]
        mov edi,lpcbBytesReturned[ESI]
        mov dword ptr [edi],ecx               

		xor      eax,eax
        ret




NotGetStatus:

		cmp	EBX, W98_SERIAL_GET_DTRRTS
		jne	NotGetDTRRTS
		
		mov	eax,1
        or	eax,eax

        ret
        
NotGetDTRRTS:

		SaveReg <ad>

        mov     EAX,dwIoControlCode[ESI]
		Debug_Printf	"BPQVC Unrecognises IOCTL Code %x BPQ Code %x", <eax, ebx>
	
		RestoreReg <ad>

		mov	eax,1
        or	eax,eax

        ret

getvers:

        cmp    cbOutBuffer[ESI],0
        je short skip
   
        mov     edi,lpvOutBuffer[ESI]
        mov     dword ptr [edi],42500407h
skip:

        xor	eax,eax

        ret

EndProc VXD_IOCONTROL

; Trap VCOMM 

BeginProc GetModemStatus_Hook, HOOK_PROC, pGetModemStatus_PrevHook, LOCKED
;MAKE_HEADER(BOOL, _cdecl, _VCOMM_GetModemStatus, (HPORT hPort, ULONG *pModemStatus))
        push ebp
        mov ebp, esp

; ebp-04h -> saved eax (from SaveReg <ad>)
; ebp+00h -> saved ebp
; ebp+04h -> return address
; ebp+08h -> pPort
; ebp+0Ch -> *pModemStatus
; ebp+10h -> *dwEvents

        SaveReg <ad>
        pushfd

  ;       mov esi, dword ptr [ebp+08h]    ; ds:[esi] = pPortName
  ;      cmp word ptr [esi], "OC"        ; Continue only if port name is "COM"
  ;      jne OpenComm_Hook_End
  
  	Debug_Printf	"BPQVC Hook Called"


        push [ebp+0Ch]                  ; *pModemStatus
        push [ebp+08h]                  ; Push pPort
        call [pGetModemStatus_PrevHook]       ; Call the old handler

        add esp, 08h
        
        SaveReg <ad>
        
        mov	eax,[ebp+0ch]                  ; dwEvents
        mov	ebx,[eax]
        
	  	Debug_Printf	"BPQVC Hook Returned %x %x",<EAX, EBX>

		RestoreReg <ad>
	
        mov [ebp-04h], eax              ; blah...

        popfd
        RestoreReg <ad>

        pop ebp
        ret                             ; Return to caller

OpenComm_Hook_End:
        popfd
        RestoreReg <ad>

        pop ebp

        jmp [pGetModemStatus_PrevHook]       ; Chain to previous hook

EndProc GetModemStatus_Hook


BeginProc EscapeCommFunction_Hook, HOOK_PROC, pEscapeCommFunction_PrevHook, LOCKED

        push ebp
        mov ebp, esp

; ebp-04h -> saved eax (from SaveReg <ad>)
; ebp+00h -> saved ebp
; ebp+04h -> return address
; ebp+08h -> pPort
; ebp+0Ch -> lFunc
; ebp+10h -> IData
; ebp+14h -> OData

        SaveReg <ad>
        pushfd

  ;       mov esi, dword ptr [ebp+08h]    ; ds:[esi] = pPortName
  ;      cmp word ptr [esi], "OC"        ; Continue only if port name is "COM"
  ;      jne OpenComm_Hook_End
  
  	Debug_Printf	"BPQVC EscapeCommFunction Hook %d Called",<[ebp+0ch]>


        push [ebp+14h]                  ; 
        push [ebp+10h]                  ; 
        push [ebp+0Ch]                  ; 
        push [ebp+08h]                  ; Push pPort
        call [pEscapeCommFunction_PrevHook]       ; Call the old handler

        add esp, 10h
        
        SaveReg <ad>
        
        mov	eax,[ebp+0ch]                  ; Funct
        cmp	eax,10
        jne @f								; not get addr
        
        mov	eax,[ebp+14h]                  ; OData
        or	eax,eax
        jz	@f
        mov	ebx,[eax]
        
	  	Debug_Printf	"BPQVC EscapeCommFunction Hook Returned Func 10 %x",<EBX>
@@:
		RestoreReg <ad>
	
        mov [ebp-04h], eax              ; blah...

        popfd
        RestoreReg <ad>

        pop ebp
        ret                             ; Return to caller

EscapeCommFunction_Hook_End:
        popfd
        RestoreReg <ad>

        pop ebp

        jmp [pEscapeCommFunction_PrevHook]       ; Chain to previous hook

EndProc EscapeCommFunction_Hook

VxD_Locked_Code_Ends

	end
