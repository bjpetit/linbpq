; KISS-TNC-HEX-LOADER.MAC -  KISS TNC Intel Hex Loader v0.3
; k3mc 8 Aug 86 v0.1
;      4 Oct 86 v0.2
;     18 Oct 86 v0.2a
;     18 Oct 86 v0.3
; v0.2 correctly sets the SP for either 16k or 32k of memory, and strips
; the parity bit from incoming chars.
; v0.2a allows 8k of RAM
; v0.3 try horrible hack to allow use w/o cutting JMP6

;	org	4000h

RAM	equ	8000h		;Where the RAM begins

ENQ	equ	5h
ACK	equ	6h
NAK	equ	15h
colon	equ	':'

rr_eof	equ	1		;Record Type for End of File / begin execution


; SIO equates

;SIO	equ	0dch		;actually, only A5 is used for SIO -cs

A_dat	equ	3 ;SIO+0		;Modem port
A_ctl	equ	1 ;SIO+1		;Modem port

B_dat	equ	2 ;SIO+2		;user serial port
B_ctl	equ	0 ;SIO+3		;user serial port

RR0_RXR	equ	1		;Receiver Char Avail bit
RR0_TBE	equ	4		;TX Buffer Empty bit



;
; The general form of an Intel HEX record is as follows:
;
; :LLaaaaRRdddd..ddCC
;
; where LL is the number of bytes of data between RR and CC,
; not including RR or CC.
; aaaa is an address (see below),
; RR is a record type,
; dd are data bytes,
; and CC is a checksum, calculated as follows:
;   CC = - ( LL + aaaa + RR + dd + dd + ... ( modulo 256 ) )
; That is, adding all the bytes LL to CC (inclusive), you should get zero.
;
; These are the record types needed:
; RR = 00 record type = data. aaaa is the load address for the data.
; RR = 01 record type = end of file.
;  aaaa is the beginning execution address. LL is 00.
;  :00aaaa01CC
;  So, for example, if the start address is 8000, the last line of the file is
;  :008000017F
;
; In this implementation, trailing checksums are ignored; you don't even have
; to include them.
;-----------------------------------------------------------------------------
start:
	di			;for this test, NO INTERRUPTS!

	ld	sp,0c000h	;force stack value.


;init SIO for async

	ld	b,nb		;n bytes for init
	ld	c,B_ctl		;to B port
	ld	hl,binit	;with these bytes

	outi
	outi			; send reset

	push	af		; wait a bit	
	pop	af
	push	af
	pop	af

	otir			; send rest!

	ld	a,5
	out	(A_ctl),a	;Ready WR5
	ld	a,80h
	out	(A_ctl),a	;turn off STATUS LED

	call	dispmsg

loop:
	call	getchar		;returns char into A reg
	cp	ENQ		;is it Control-E character?
	jr	z,ENQCHR	;yes, deal with it
	cp	colon
	jr	z,saw_colon	;Go into Intel Hex download mode
	call	putchar		;if neither, just echo it
	jr	loop

ENQCHR:
	call	dispmsg
	jr	loop

dispmsg:
	ld	hl,ENQ_string
ENQ_loop:
	ld	a,(hl)
	or	a
	ret	z
	call	putchar
	inc	hl
	jr	ENQ_loop

saw_colon:
	call	rdbyte
	ex	af,af'
		;save in other register set for a sec
	call	rdbyte
	ld	h,a
	call	rdbyte
	ld	l,a
	call	rdbyte
	cp	rr_eof		;is it record type 1 (begin execution)?
	jr	nz,data_record	;no, it's just another data record
;else we give machine to downloaded program
	jp	(hl)		;and go do it!

data_record:
	ex	af,af'
		;get length value back
	ld	b,a		;and ready loop index
load_loop:
	call	rdbyte
	ld	(hl),a
	inc	hl
	djnz	load_loop	;load 'em up!

; Note, we ignore checksums completely

find_colon:
	call	getchar
	cp	colon
	jr	nz,find_colon	;spin for a colon 
	jr	saw_colon	;when we find colon, deal with next record

;*** Reads 2 characters from SIO, converts them to binary, and returns value
;*** into A reg.  Disturbs no registers except AF.
rdbyte:
	push	bc
	call	getchar
	call	mk_binary
	rlca
	rlca
	rlca
	rlca
	ld	b,a		;save hi part
	call	getchar
	call	mk_binary
	or	b		;get hi + lo parts
	pop	bc		;be tidy
	ret

;*** Convert the ASCII character into Binary character (src & dest is A reg)
mk_binary:
	push	hl
	push	de
	call	makeUC		;if anybody uses lower case, it's OK
	sub	'0'		;convert ASCII -> binary (sorta)
	ld	d,0
	ld	e,a
	ld	hl,btable	;base of translation table
	add	hl,de		;produce pointer into table
	ld	a,(hl)		;get corresponding binary
	pop	de
	pop	hl		;cleanliness is next to Godliness
	ret

btable:	defb	0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0,0ah,0bh,0ch,0dh,0eh,0fh

;*** If the character in A reg is lower case, this makes it upper case.
makeUC:
	cp	'a'
	ret	c		;if less than an 'a' we're done
	cp	'z'+1
	ret	nc		;if > than 'z', not a letter, so we're done
	and	5fh		;else force to UPPER CASE
	ret

;*** Get a char from user TTY (Port B), no interrupt mode. Return it in A reg.
getchar:
	in	a,(B_ctl)
	and	RR0_RXR
	jr	z,getchar	;wait for a character to be typed
	in	a,(B_dat)
	and	7fh		;strip parity bit (Phil's suggestion)
	ret


;*** Put a character to user TTY (Port B), no interrupts.  Char is in A reg.
putchar:
	push	af		;we will need A
ploop:
	in	a,(B_ctl)
	and	RR0_TBE
	jr	z,ploop		;Wait for Transmitter buffer to become empty

	pop	af
	out	(B_dat),a
	ret


binit	defb	9,0c0h		; reset all
	defb	4,44h		; 16x 1 stop no parity
	defb	3,0c3h		; 8 bit rx enable
	defb	5,0eeh		; dtr 8 bit tx en rts
	defb	1,0		; no interrupts
	defb	11,50h		; txc, rxc = brg out
	defb	14,02h		; brg source = pclk
	defb	15,0h		; ext int enables - none
	defb	28h		; reset tx int pending
	defb	10h,10h		; ext/status (*2?)
	defb	12,62,13,0	; 2400 ?? 
	defb	14,03h		; brg enable

;binit:	defb	1ah,0,14h,44h,3,0c3h,5,0eeh,11h,0	;SIO inits

bi_end	equ	$
nb	equ	bi_end-binit	;Number of bytes in previous string

ENQ_string:
	defm	'TNC220 Intel Hex Loader v0.3 18 Oct 86'
	defb	13,10,0

	end	start
