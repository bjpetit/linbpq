	PAGE    56,132
;
;	Glue to allow apps using 16 bit BPQ API to run in XP VDM and access BPQ32
;
;	New Version November 2006 using BOP

;	March 2009
;
;		Remove config file - default Vector to 127, and allow command line override of Vector and Version

.LALL
.LFCOND

BOP_3RDPARTY	EQU	58H
BOP_UNSIMULATE	EQU	0FEH

RegisterModule macro
    db  0C4h, 0C4h, BOP_3RDPARTY, 0
        endm

UnRegisterModule macro
    db  0C4h, 0C4h, BOP_3RDPARTY, 1
        endm

DispatchCall macro
    db  0C4h, 0C4h, BOP_3RDPARTY, 2
        endm
 
VDDUnSimulate16 macro
    db	0C4h, 0C4h, BOP_UNSIMULATE
	endm


DEBUG	EQU	0

CR	EQU	0DH
LF	EQU	0AH


STACK	SEGMENT	STACK	'STACK'

	DW	8 DUP (0)

STACK	ENDS

CODE	SEGMENT	PUBLIC	'CODE'

CODE	ENDS				; TO FORCE LINK ORDER

;
;	FAR PROC EXTRNS ARE OUTSIDE SEGMENT DEFINITIONS
;
	INCLUDE	ENVIRON.ASM
;	INCLUDE STRUCS.ASM


DATA	SEGMENT	PUBLIC	'DATA'
;
;	POINTERS TO TABLES ARE AT START TO SIMPLIFY DIAGNOSTIC 
;	 DUMP ANALYSIS, AND NODES SAVE ROUTINES
;
FILLER		DB	14 DUP (0)	; PROTECTION AGENST BUFFER PROBLEMS!

MAJORVERSION1	DB	4
MINORVERSION1	DB	9	

NEXTFREEDATA LABEL BYTE

DATA	ENDS

INITSEG	SEGMENT	PUBLIC	'INITSEG'


ALREADYLOADED	DB	'BPQCODE or BPQ1632 is already loaded',cr,lf,'$'


ERRMSG			DB	'Failed to load BPQ1632.DLL',0DH,0AH,'$'


dll		db	'BPQ1632.DLL',0
API		DB	'CallFrom16',0



SIGNONMSG	LABEL	BYTE

		DB	NAMESTRING,' AX25 Packet Switch System WIN32 Interface',CR,LF
		DB	'Version '
		DB	VERSIONSTRING,' - ',DATESTRING,CR,LF
                DB      'Copyright (c) 1988-2009 John Wiseman.'
		DB	CR,LF,LF

		DB	'$'

HOSTINTERRUPT	DB	127

NEWVALUE	DW	0

D10		DW	10
D16		DB	16

loadedmsg	DB	cr,lf,lf,'BPQ32 Switch interface loaded and initialised OK',cr,lf,'$'

	EVEN

	ASSUME  CS:INITSEG,DS:NOTHING,ES:INITSEG,SS:NOTHING

START:

	PUSH	DS				; SAVE PSP ADDRESS
	
	mov	ax,INITSEG
	MOV	DS,AX
	MOV	ES,AX
;

	MOV	DX,OFFSET SIGNONMSG
	MOV	AH,9
	INT	21H

	pop		DS				; Recover PSP for param check
	push	DS				; Save Again
	
	MOV	SI,5DH				; First Param
	
	cmp byte ptr ds:[si], 32
	je short @F				; No Params - Use Default Port and Version

	CALL	GETVALUE		; Interrupt
	JC	@F

	MOV	AL,BYTE PTR NEWVALUE
	OR AL,AL
	JZ SHORT @f
	
	MOV	HOSTINTERRUPT,AL

	MOV	SI,6DH				; 2nd Param
	
	cmp byte ptr ds:[si], 32
	je short @F				; No Params - Use Default Version

	CALL	GETVALUE		; Subversion
	JC	@F

	MOV	BL,BYTE PTR NEWVALUE
	OR	BL,BL
	JZ	@F

	mov	ax,CODE
	MOV	DS,AX
	
	ASSUME	DS:CODE
	MOV	MINORVERSION2,BL 
	
	mov	ax,DATA
	MOV	DS,AX

	ASSUME	DS:DATA
	MOV	MINORVERSION1,BL 
	 
@@:

	mov	ax,INITSEG
	MOV	DS,AX

	ASSUME	DS:INITSEG
;
;	SEE IF ALREADY LOADED
;
	XOR	AX,AX
	MOV	AL,HOSTINTERRUPT
	ADD	AX,AX
	ADD	AX,AX
	MOV	SI,AX


	MOV	AX,0
	MOV	ES,AX

	MOV	AX,ES:2[SI]		; CODE SEGMENT
	MOV	ES,AX

	MOV	DI,0
	MOV	SI,OFFSET MARKER
	MOV	CX,4
	REP CMPSB

	JE	CODEFOUND

	JMP SHORT LOAD_OK

MARKER:						; Same as BPQCODE
	RET
	DB	'BPQ'

CODEFOUND:


	MOV	DX,OFFSET ALREADYLOADED
	
CONFIGERR:

	MOV	AH,9
	INT	21H

	MOV	AX,4C01H
	INT	21H			; EXIT


LOAD_OK:

	MOV	AX,INITSEG
	MOV	ES,AX

;
;*	RegisterModule results in loading the DLL (specified in DS:SI).
;*      Its Init routine (specified in ES:DI) is called. Its Dispatch
;*	routine (specified in DS:BX) is stored away and all the calls
;*      made from DispatchCall are dispacthed to this routine.
;*      If ES and DI both are null than the caller did'nt supply the init
;*      routine.


	MOV	SI,OFFSET DLL
	MOV	DI,0
	MOV	BX,OFFSET API
	
	or	al, al			; Clear Carry (Why?)
	
	push	es		
	mov	ax,0
	mov	es,ax			; No INIT Routine
	
	RegisterModule

	pop	es
;AX = 1, Carry = 1: DLL not found 
;AX = 2, Carry = 1: Dispatch routine not found 
;AX = 3, Carry = 1: Init routine not found 
;AX = 4, Carry = 1: Insufficient memory available 
;AX=MMMM,Carry = 0: Successful operation, File handle MMMM is 

	jnc @f

	mov	dx,offset errmsg
	jmp CONFIGERR

@@:

	push	es
	
	push ax


	MOV	AX,CODE
	MOV	ES,AX

	assume ES:CODE

	pop	ax
	mov es:DLL_HANDLE, ax

	pop	es

	assume ES:INITSEG


;	INSTALL INT 7F TRAPS
;
	PUSH	ES
	PUSH	DS

	MOV	AX,CODE
	MOV	DS,AX

	MOV	DX,OFFSET BPQHOSTPORT			
	MOV	AL,cs:HOSTINTERRUPT				
	MOV	AH,25H			; SET NEW VECTOR
	INT	21H				


	POP	DS
	POP	ES
	
	mov	dx,offset loadedmsg
	mov	ah,9
	int	21h


	MOV	AX,offset NEXTFREEDATA
	MOV	CL,4
	SHR	AX,CL			; CONVERT OT PARAS
	INC	AX			; ROUND UP
	INC	AX			; JUST TO BE SURE!

	POP	BX			; PSP ADDR

	MOV	DX,DATA
	SUB	DX,BX			; LENGTH TO START OF DATA SEG
	ADD	DX,AX			; INCLUDE LENGTH OF DATA

	MOV	AX,3100H		; KEEP
 	INT	21H
;

	ASSUME	DS:NOTHING
	
GETVALUE:
;
;	EXTRACT NUMBER (HEX OR DECIMAL) FROM INPUT STRING
;
	MOV	NEWVALUE,0
	
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

ENDVALUE:
	CLC
	RET

DUFFVALUE:
	STC
	RET


INITSEG	ENDS

CODE	SEGMENT PUBLIC 'CODE'

	ASSUME  CS:CODE,DS:DATA,ES:DATA,SS:DATA
;
	RET
	DB	'BPQ'		; FOR DUMP SYSTEM

DLL_HANDLE dw	0

	DB	'G8BPQ'			; MUST BE JUST BEFORE INT 7F ENTRY
MAJORVERSION2	DB	4
MINORVERSION2	DB	9	

BPQHOSTPORT:
;
;	SPECIAL INTERFACE, MAINLY FOR EXTERNAL HOST MODE SUPPORT PROGS
;
;	COMMANDS SUPPORTED ARE
;
;	AH = 0	Get node/switch version number and description.  On return
;		AH='B',AL='P',BH='Q',BL=' '
;		DH = major version number and DL = minor version number.
;
;
;	AH = 1	Set application mask to value in DL (or even DX if 16
;		applications are ever to be supported).
;
;		Set application flag(s) to value in CL (or CX).
;		whether user gets connected/disconnected messages issued
;		by the node etc.
;
;
;	AH = 2	Send frame in ES:SI (length CX)
;
;
;	AH = 3	Receive frame into buffer at ES:DI, length of frame returned
;		in CX.  BX returns the number of outstanding frames still to
;		be received (ie. after this one) or zero if no more frames
;		(ie. this is last one).
;
;
;
;	AH = 4	Get stream status.  Returns:
;
;		CX = 0 if stream disconnected or CX = 1 if stream connected
;		DX = 0 if no change of state since last read, or DX = 1 if
;		       the connected/disconnected state has changed since
;		       last read (ie. delta-stream status).
;
;
;
;	AH = 6	Session control.
;
;		CX = 0 Conneect - APPLMASK in DL
;		CX = 1 connect
;		CX = 2 disconnect
;		CX = 3 return user to node
;
;
;	AH = 7	Get buffer counts for stream.  Returns:
;
;		AX = number of status change messages to be received
;		BX = number of frames queued for receive
;		CX = number of un-acked frames to be sent
;		DX = number of buffers left in node
;		SI = number of trace frames queued for receive
;
;AH = 8		Port control/information.  Called with a stream number
;		in AL returns:
;
;		AL = Radio port on which channel is connected (or zero)
;		AH = SESSION TYPE BITS
;		BX = L2 paclen for the radio port
;		CX = L2 maxframe for the radio port
;		DX = L4 window size (if L4 circuit, or zero)
;		ES:DI = CALLSIGN

;AH = 9		Fetch node/application callsign & alias.  AL = application
;		number:
;
;		0 = node
;		1 = BBS
;		2 = HOST
;		3 = SYSOP etc. etc.
;
;		Returns string with alias & callsign or application name in
;		user's buffer pointed to by ES:SI length CX.  For example:
;
;		"WORCS:G8TIC"  or "TICPMS:G8TIC-10".
;
;
;	AH = 10	Unproto transmit frame.  Data pointed to by ES:SI, of
;		length CX, is transmitted as a HDLC frame on the radio
;		port (not stream) in AL.
;
;
;	AH = 11 Get Trace (RAW Data) Frame into ES:DI,
;			 Length to CX, Timestamp to AX
;
;
;	AH = 12 Update Switch. At the moment only Beacon Text may be updated
;		DX = Function
;		     1=update BT. ES:SI, Len CX = Text
;		     2=kick off nodes broadcast
;
;	AH = 13 Allocate/deallocate stream
;		If AL=0, return first free stream
;		If AL>0, CL=1, Allocate stream. If aleady allocated,
;		   return CX nonzero, else allocate, and return CX=0
;		If AL>0, CL=2, Release stream
;
;
;	AH = 14 Internal Interface for IP Router
;
;		Send frame - to NETROM L3 if DL=0
;			     to L2 Session if DL<>0
;
;
; 	AH = 15 Get interval timer


	ASSUME DS:NOTHING
	ASSUME ES:NOTHING

	STI
;
;	Process Get Version here, so we dont have to load BPQ32 if programs are loaded at NTVDM startup

	cmp	ah,0	
	jne	notgetvers


	MOV	AX,'PB'
	MOV	BX,' Q'
	MOV	DH,MAJORVERSION2
	MOV	DL,MINORVERSION2

	IRET

NotGetVers:

;		Call directly to API
;
;		Only snag is DLL handle goes in AX, so pass Function in BX


	mov	bx,ax
	mov	ax,cs:DLL_HANDLE

	DispatchCall

BPQRETURN:

	IRET

CODE	ENDS

	END	START

