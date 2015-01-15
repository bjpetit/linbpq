	PAGE    56,132
;
 
.386
;
;  SEGMENT definitions and order
;


;*	32 Bit code
_TEXT		SEGMENT DWORD USE32 PUBLIC 'CODE'
_TEXT		ENDS



;*	Contains 32 Bit data
_BPQDATA		SEGMENT DWORD PUBLIC 'DATA'
_BPQDATA		ENDS


	ASSUME CS:FLAT, DS:FLAT, ES:FLAT, SS:FLAT

OFFSET32 EQU <OFFSET FLAT:>

_BPQDATA	SEGMENT

	extern _APISemaphore:DWORD

ApiEAX DD 0;
ApiEBX DD 0;
ApiECX DD 0;
ApiEDX DD 0;
ApiESI DD 0;
ApiEDI DD 0;

_BPQDATA	ENDS

_TEXT	SEGMENT
;
	EXTRN	_CHOSTAPI:NEAR
MARKER	DB	'G8BPQ'			; MUST BE JUST BEFORE INT 7F ENTRY
	DB	4 ; MAJORVERSION
	DB	9 ; MINORVERSION


	PUBLIC	_BPQHOSTAPI
_BPQHOSTAPI:
;
;	SPECIAL INTERFACE, MAINLY FOR EXTERNAL HOST MODE SUPPORT PROGS
;
	extrn	_GetSemaphore:near
	extrn	_FreeSemaphore:near
	extrn	_Check_Timer:near


	pushad
	call	_Check_Timer
	push	offset _APISemaphore
	call	_GetSemaphore
	add		esp, 4
	popad
	
;	Params are 16 bits

	movzx	eax,ax
	movzx	ebx,bx
	movzx	ecx,cx
	movzx	edx,dx
	
	mov	ApiEAX, eax
	mov	ApiEBX, ebx
	mov	ApiECX, ecx
	mov	ApiEDX, edx
	mov	ApiESI, esi	
	mov	ApiEDI, edi

	lea		eax,ApiEDI
	push	eax
	lea		eax,ApiESI
	push	eax
	lea		eax,ApiEDX
	push	eax
	lea		eax,ApiECX
	push	eax
	lea		eax,ApiEBX
	push	eax
	lea		eax,ApiEAX
	push	eax
	
	call	_CHOSTAPI
	add		esp, 24
	
	mov	eax,ApiEAX
	mov	ebx,ApiEBX
	mov	ecx,ApiECX
	mov	edx,ApiEDX
	mov	esi,ApiESI	
	mov	esi,ApiEDI

	
	pushad
	push	offset _APISemaphore
	call	_FreeSemaphore
	add		esp, 4
	popad

	ret

_TEXT	ENDS

	END	
