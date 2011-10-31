version	equ	3

	include	defs.asm

;  Copyright, 1988, 1989, Russell Nelson

;   This program is free software; you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, version 1.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program; if not, write to the Free Software
;   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


;	MAY 1990
;
;	Modified by John Wiseman, G8BPQ to provide a COMBIOS interface
;	to his Node software. 
;
;	MARCH 91
;
;	and again, to use the BPQ HOST interface
;
code	segment byte public
	assume	cs:code, ds:code

	org	2ch

phd_environ	dw	?

	org	80h

phd_dioa	label	byte

	org	100h
start:
	jmp	start_1


packet_int_no	db	?,?,?,?		; interrupt to communicate.

functions	label	word
	dw	f_driver_info		;function 1
	dw	f_access_type
	dw	f_release_type
	dw	f_send_pkt
	dw	f_terminate
	dw	f_get_address
	dw	f_reset_interface	;function 7
	dw	f_set_rcv_mode		;function 20
	dw	f_get_rcv_mode
	dw	f_set_multicast_list
	dw	f_get_multicast_list
	dw	f_get_statistics
	dw	f_set_address		;function 25



per_handle	struc
in_use		db	?		;non-zero if this handle is in use.
packet_type	db	MAX_P_LEN dup(?);associated packet type.
packet_type_len	dw	?		;associated packet type length.
receiver	dd	?		;receiver handler.
		db	?		;padding to get the length to 16.
per_handle	ends

  if (size per_handle) NE 16
	err	The size of the handle must be 16.
  endif

handles		db	MAX_HANDLE*(size per_handle) dup(0)
end_handles	label	byte

multicast_count	dw	0		;count of stored multicast addresses.
multicast_addrs	db	MAX_MULTICAST*EADDR_LEN dup(?)

have_my_address	db	0		;nonzero if our address has been set.
my_address	db	MAX_ADDR_LEN dup(?)
my_address_len	dw	?

rcv_mode_num	dw	3

free_handle	dw	?		;->a handle not in use.
found_handle	dw	?		;->the handle for our packet.
receive_ptr	dd	?		;->the receive routine.

savess		dw	?		;saved during the stack swap.
savesp		dw	?

regs	struc
_ES	dw	?
_DS	dw	?
_BP	dw	?
_DI	dw	?
_SI	dw	?
_DX	dw	?
_CX	dw	?
_BX	dw	?
_AX	dw	?
_IP	dw	?
_CS	dw	?
_F	dw	?		;flags
regs	ends

CY	equ	0001h
EI	equ	0200h


bytes	struc
	dw	?
	dw	?
	dw	?
	dw	?
	dw	?
_DL	db	?
_DH	db	?
_CL	db	?
_CH	db	?
_BL	db	?
_BH	db	?
_AL	db	?
_AH	db	?
bytes	ends

their_isr	dd	?

our_isr:
	jmp	our_isr_0		;the required signature.
	db	'PKT DRVR',0

statistics_list	label	dword
packets_in	dw	?,?
packets_out	dw	?,?
bytes_in	dw	?,?
bytes_out	dw	?,?
errors_in	dw	?,?
errors_out	dw	?,?
packets_dropped	dw	?,?		;dropped due to no type handler.

linc	macro	n
	local	a
	inc	n			;increment the low word
	jne	a			;go if not overflow
	inc	n+2			;increment the high word
a:
	endm

count_in_err:
	assume	ds:nothing
	linc	errors_in
	ret

count_out_err:
	assume	ds:nothing
	linc	errors_out
	ret

our_isr_0:
	assume	ds:nothing
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	bp
	push	ds
	push	es
	cld
	mov	bx,cs			;set up ds.
	mov	ds,bx
	assume	ds:code
	mov	bp,sp			;we use bp to access the original regs.
	and	_F[bp],not CY		;start by clearing the carry flag.
	test	_F[bp],EI		;were interrupts enabled?
	jz	our_isr_ei		;no.
;	sti				;yes - re-enable them.
our_isr_ei:
	mov	dh,BAD_COMMAND		;in case we find a bad number.
	mov	bl,ah			;jump to the correct function.
	mov	bh,0
	cmp	bx,7			;highest function is 7.
	jbe	our_isr_ok
	cmp	bx,20
	jb	our_isr_error
	cmp	bx,25
	ja	our_isr_error
	sub	bx,20-7-1		;map 20 right after 7.
our_isr_ok:
	add	bx,bx			;*2
	call	functions-2[bx]		;table starts at 1.
	jnc	our_isr_return
our_isr_error:
	mov	_DH[bp],dh
	or	_F[bp],CY		;return their carry flag.
our_isr_return:
	pop	es
	pop	ds
	pop	bp
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	iret


f_driver_info:
;	Under 1.08, the handle is optional, so we no longer verify it.
;	call	verify_handle
	mov	_BX[bp],majver		;version
	mov	al,driver_class
	mov	_CH[bp],al
	mov	al,driver_type
	cbw
	mov	_DX[bp],ax
	mov	_CL[bp],0		;number zero.
	mov	_DS[bp],ds
	mov	_SI[bp],offset driver_name
	mov	_AL[bp],2		;extended driver
	clc
	ret


f_set_rcv_mode:
	call	verify_handle
	mov	cx,_CX[bp]		;Tell them how much room they have.
	cmp	cx,rcv_modes		;do they have this many modes?
	jae	f_set_rcv_mode_1	;no - must be a bad mode for us.
	mov	bx,cx
	add	bx,bx			;we're accessing words, not bytes.
	mov	ax,rcv_modes[bx]+2	;get the handler for this mode.
	or	ax,ax			;do they have one?
	je	f_set_rcv_mode_1	;no - must be a bad mode for us.
	mov	rcv_mode_num,cx		;yes - remember the number and
	call	ax			;  call it.
	clc
	ret
f_set_rcv_mode_1:
	mov	dh,BAD_MODE
	stc
	ret


f_get_rcv_mode:
	call	verify_handle
	mov	ax,rcv_mode_num		;return the current receive mode.
	mov	_AX[bp],ax
	clc
	ret


f_set_multicast_list:
;	mov	cx,_CX[bp]		;Tell them how much room they have.

;verify that they supplied an even number of EADDR's.
	mov	ax,cx
	xor	dx,dx
	mov	bx,EADDR_LEN
	div	bx
	or	dx,dx			;zero remainder?
	jne	f_set_multicast_list_2

	cmp	cx,MAX_MULTICAST*EADDR_LEN	;is this too many?
	jbe	f_set_multicast_list_1
	mov	cx,MAX_MULTICAST*EADDR_LEN	;yes - store only this many.
f_set_multicast_list_1:
	mov	multicast_count,cx
	push	cs
	pop	es
	mov	di,offset multicast_addrs
	push	ds
	mov	ds,_ES[bp]		; get ds:si -> new list.
	mov	si,_DI[bp]
	rep	movsb
	pop	ds

	mov	es,_ES[bp]		; get new ones
	mov	di,_DI[bp]		; get pointer, es:di is ready
	mov	cx,_CX[bp]		;Tell them how much room they have.
	call	set_multicast_list
	ret
f_set_multicast_list_2:
	mov	dh,BAD_ADDRESS
	stc
	ret


f_get_multicast_list:
	call	get_multicast_list	;Can they do it themselves?
	jnc	f_get_multicast_list_1	;yes - we're all set.
	cmp	dh,NO_ERROR		;do they implement multicast at all?
	stc
	jne	f_get_multicast_list_1	;no - return an error.
	mov	_ES[bp],ds		;we have to return what we have stored.
	mov	_DI[bp],offset multicast_addrs
	mov	cx,multicast_count
	mov	_CX[bp],cx
	clc
f_get_multicast_list_1:
	ret


f_get_statistics:
	call	verify_handle		;just in case.
	mov	_DS[bp],ds
	mov	_SI[bp],offset statistics_list
	clc
	ret


access_type_class:
	mov	dh,NO_CLASS
	stc
	ret

access_type_type:
	mov	dh,NO_TYPE
	stc
	ret

access_type_number:
	mov	dh,NO_NUMBER
	stc
	ret

access_type_bad:
	mov	dh,BAD_TYPE
	stc
	ret

access_type_space:
	mov	dh,NO_SPACE
	stc
	ret

f_access_type:
	mov	al,driver_class
	cmp	_AL[bp],al		;our class?
	jne	access_type_class	;no.
	cmp	_BX[bp],-1		;generic type?
	je	access_type_2		;yes.
	mov	al,driver_type
	cbw
	cmp	_BX[bp],ax		;our type?
	jne	access_type_type	;no.
access_type_2:
	cmp	_DL[bp],0		;generic number?
	je	access_type_3
	cmp	_DL[bp],1		;our number?
	jne	access_type_number
access_type_3:
	cmp	_CX[bp],MAX_P_LEN	;is the type length too long?
	ja	access_type_bad		;yes - can't be ours.
access_type_7:

; now we do two things--look for an open handle, and check the existing
; handles to see if they're replicating a packet type.

	mov	free_handle,0		;remember no free handle yet.
	mov	bx,offset handles
access_type_4:
	cmp	[bx].in_use,0		;is this handle in use?
	je	access_type_5		;no - don't check the type.
	mov	es,_DS[bp]		;get a pointer to their type.
	mov	di,_SI[bp]
	mov	cx,_CX[bp]		;get the minimum of their length
					;  and our length.  As currently
					;  implemented, only one receiver
					;  gets the packets, so we have to
					;  ensure that the shortest prefix
					;  is unique.
	cmp	cx,[bx].packet_type_len	;Are we less specific than they are?
	jb	access_type_8		;no.
	mov	cx,[bx].packet_type_len	;yes - use their count.
access_type_8:
	lea	si,[bx].packet_type
	or	cx,cx			;in case cx is zero.
	repe	cmpsb
	jne	short access_type_6	;go look at the next one.
access_type_inuse:
	mov	dh,TYPE_INUSE
	stc
	ret
access_type_5:
	cmp	free_handle,0		;found a free handle yet?
	jne	access_type_6		;yes.
	mov	free_handle,bx		;remember a free handle
access_type_6:
	add	bx,(size per_handle)	;go to the next handle.
	cmp	bx,offset end_handles
	jb	access_type_4

	mov	bx,free_handle		;did we find a free handle?
	or	bx,bx
	je	access_type_space	;no - return error.

	mov	[bx].in_use,1		;remember that we're using it.

	mov	ax,_DI[bp]		;remember the receiver type.
	mov	[bx].receiver.offs,ax
	mov	ax,_ES[bp]
	mov	[bx].receiver.segm,ax

	push	ds
	mov	ax,ds
	mov	es,ax
	mov	ds,_DS[bp]		;remember their type.
	mov	si,_SI[bp]
	mov	cx,_CX[bp]
	mov	es:[bx].packet_type_len,cx	;remember the length.
	lea	di,[bx].packet_type
	rep	movsb
	pop	ds

	sub	bx,offset handles	;compute the handle from the offset.
	mov	cl,4
	shr	bx,cl
	mov	ax,cs			;add our phd in.
	add	bx,ax
	mov	_AX[bp],bx		;return the handle to them.

	clc
	ret


f_release_type:
	call	verify_handle		;mark this handle as being unused.
	mov	[bx].in_use,0
	clc
	ret


f_send_pkt:
	linc	packets_out
	add	bytes_out.offs,cx	;add up the received bytes.
	adc	bytes_out.segm,0

	push	ds		; set up proper ds for the buffer
	mov	ds,_DS[bp]	; address for buffer

;following two instructions not needed because si and cx haven't been changed.
;	mov	si,_SI[bp]
;	mov	cx,_CX[bp]	; count of bytes

	call	send_pkt
	pop	ds
	ret


f_terminate:
;
; 	DONT ALLOW RELEASE FOR NOW
;
;	CAN WHEN WE UNHOOK TIMER
;
;
	mov	cl,0			; cancle monitoring
	MOV	DL,0
	MOV	AL,byte ptr CS:IO_ADDR
	MOV	AH,1			; SET APPL MASK

	CALL	NODE
;
; Now return the interrupt to their handler.
;
	mov	al,packet_int_no	;release our_isr.
	mov	ah,25h
	push	ds
	lds	dx,their_isr
	int	21h
	pop	ds

;
; Now free our memory
;
	push	cs
	pop	es
	mov	ah,49h
;	int	21h

	clc
	ret


f_get_address:
	call	verify_handle
;	mov	es,_ES[bp]		; get new one
;	mov	di,_DI[bp]		; get pointer, es:di is ready
	mov	cx,_CX[bp]		;Tell them how much room they have.
	cmp	have_my_address,0	;Did we get our address set?
	jne	get_address_set
	call	get_address
	jc	get_address_space	;no.
	mov	_CX[bp],cx		;Tell them how long our address is.
	clc
	ret
get_address_set:
	cmp	cx,my_address_len	;is there enough room?
	jb	get_address_space	;no.
	mov	cx,my_address_len	;yes - get our address length.
	mov	_CX[bp],cx		;Tell them how long our address is.
	mov	si,offset my_address	;copy it into their area.
	rep	movsb
	clc
	ret

get_address_space:
	mov	dh,NO_SPACE
	stc
	ret


f_set_address:
	mov	bx,offset handles
	mov	dh,CANT_SET		;if a handle in use, we can't set it.
f_set_address_1:
	cmp	[bx].in_use,0		;is this handle in use?
	stc
	jne	f_set_address_exit	;yes - we can't set the address
	add	bx,(size per_handle)	;go to the next handle.
	cmp	bx,offset end_handles
	jb	f_set_address_1

	mov	ds,_ES[bp]		; set new one
	assume	ds:nothing
	mov	si,_DI[bp]		; set pointer, ds:si is ready
;	mov	cx,_CX[bp]		;Tell them how much address is being set.
	call	set_address
;set_address restores ds.
	jc	f_set_address_exit	;Did it work?
	mov	_CX[bp],cx		;yes - return our address length.

	cmp	cx,MAX_ADDR_LEN		;is it too long for us to remember?
	ja	f_set_address_too_long	;yes, return a too-long error.

	mov	ds,_ES[bp]		; set new one
	mov	si,_DI[bp]		; set pointer, ds:si is ready
	mov	ax,cs
	mov	es,ax
	mov	my_address_len,cx	;remember how long our address is.
	mov	di,offset my_address
	rep	movsb
	mov	have_my_address,1
	mov	ds,ax			;restoer ds.
	assume	ds:code
	clc
	jmp	short f_set_address_exit

f_set_address_too_long:
	mov	dh,NO_SPACE
	stc
f_set_address_exit:
	ret


f_reset_interface:
	call	verify_handle
	call	reset_interface
	clc
	ret


compute_handle:
;enter with the external representation of our handle in _BX[bp],
;exit with the offset of the handle in bx.
	mov	bx,_BX[bp]		;get the handle they gave us
	mov	cx,cs
	sub	bx,cx			;subtract off our phd
	mov	cl,4
	shl	bx,cl			;multiply by the size of the struct,
	add	bx,offset handles	;  and add the offset of the table.
	ret


verify_handle:
;Ensure that their handle is real.  If it isn't, we pop off our return
;address, and return to *their* return address with cy set.
	call	compute_handle
	cmp	bx,offset handles
	jb	verify_handle_bad	;no - must be bad.
	cmp	bx,offset end_handles
	jae	verify_handle_bad	;no - must be bad.
	cmp	[bx].in_use,0		;if it's not in use, it's bad.
	je	verify_handle_bad
	ret
verify_handle_bad:
	mov	dh,BAD_HANDLE
	add	sp,2			;pop off our return address.
	stc
	ret



recv_find:
;called when we want to determine what to do with a received packet.
;enter with cx = packet length, es:di -> packet type.
	assume	ds:code, es:nothing
	push	cx

	mov	bx,offset handles
recv_find_1:
	cmp	[bx].in_use,0		;is this handle in use?
	je	recv_find_2		;no - don't check the type.
	mov	ax,[bx].receiver.offs	;do they have a receiver?
	or	ax,[bx].receiver.segm
	je	recv_find_2		;no - they're not serious about it.
	mov	cx,[bx].packet_type_len	;compare the packets.
	lea	si,[bx].packet_type
	or	cx,cx			;in case cx is zero.
	push	di
	repe	cmpsb
	pop	di
	je	recv_find_3		;we've got it!
recv_find_2:
	add	bx,(size per_handle)	;go to the next handle.
	cmp	bx,offset end_handles
	jb	recv_find_1

	linc	packets_dropped

	pop	cx			;we didn't find it -- discard it.
	xor	di,di			;"return" a null pointer.
	mov	es,di
	jmp	short recv_find_4
recv_find_3:
	pop	cx			; the packet_length

	linc	packets_in
	add	bytes_in.offs,cx	;add up the received bytes.
	adc	bytes_in.segm,0

	mov	ax,[bx].receiver.offs
	mov	receive_ptr.offs,ax
	mov	ax,[bx].receiver.segm
	mov	receive_ptr.segm,ax

	sub	bx,offset handles	;compute the handle number from
	shr	bx,1			;  the handle offset.
	shr	bx,1
	shr	bx,1
	shr	bx,1
	mov	ax,cs			;add our phd in.
	add	bx,ax

	mov	found_handle,bx		;remember what our handle was.
	mov	ax,0			;allocate request.
	call	receive_ptr		;ask the client for a buffer.
recv_find_4:
	ret



recv_copy:
;called after we have copied the packet into the buffer.
;enter with ds:si ->the packet, cx = length of the packet.

	assume	ds:nothing, es:nothing
	push	bx
	mov	bx,found_handle
	mov	ax,1			;store request.
	call	receive_ptr		;ask the client for a buffer.
	pop	bx
	ret

	assume	cs:code, ds:code


	assume	cs:code, ds:code


;Slip Definitions

FR_END		equ	0c0h		;Frame End
FR_ESC		equ	0dbh		;Frame Escape
T_FR_END	equ	0dch		;Transposed frame end
T_FR_ESC	equ	0ddh		;Transposed frame escape

	public	int_no

int_no		db	4,0,0,0		; interrupt number.
io_addr		dw	0,0		; used for combios port number
port		dw	0,0		; node radio port to use
 
driver_class	db	9,0,0,0	;from the packet spec
driver_type	db	0,0,0,0		;from the packet spec
driver_name	db	'NODEDRV4',0	;name of the driver.

recv_pkt_ready	dw	0		; flag indicating a packet is ready


buffer		db	350 dup (0)	; temp storage for packet from node

  ifdef debug
	public packet_sem, pkt_send_sem, xmit_time
  endif

packet_sem	dw	0		; semaphore for	packets received
pkt_send_sem	dw	0		; semaphore for	packets xmitted
asyrxint_cnt	dw	0		; loop counter in asyrxint
xmit_time	dw	0		; loop timer for asyrxint

	public	rcv_modes

rcv_modes	dw	4		;number	of receive modes in our table.
		dw	0,0,0,rcv_mode_3


;
;	BASIC LINK LEVEL MESSAGE BUFFER LAYOUT
;
MESSAGE	STRUC

MSGCHAIN	DW	?		; CHAIN WORD
MSGPORT		DB	?		; PORT 
MSGLENGTH	DW	?		; LENGTH
MSGDEST		DB	?

MESSAGE	ENDS

send_pkt:

;enter with ds:si -> packet, cx = packet length.
;exit with nc if ok, or else cy if error, dh set to error number.
;called from telnet layer via software interrupt

	assume	ds:nothing

	sti				; enable interrupts

	push	ds
	pop	es			; node wants es:si

	mov	ah,10
	mov	al,byte ptr port	; port 

;	dec	cx
;	inc	si			; remove control byte

	call	node

	clc
	ret



get_address:
;get the address of the interface.
;enter with es:di -> place to get the address, cx = size of address buffer.
;exit with nc, cx = actual size of address, or cy if buffer not big enough.
	assume	ds:code
	mov	cx,0
	clc
	ret



set_address:
;set the address of the interface.
;enter with es:di -> place to get the address, cx = size of address buffer.
;exit with nc, cx = actual size of address, or cy if buffer not big enough.
	assume	ds:nothing
	clc
	ret


rcv_mode_3:
;receive mode 3 is the only one we support, so we don't have to do anything.
	ret


set_multicast_list:
;enter with es:di ->list of multicast addresses, cx = number of bytes.
;return nc if we set all of them, or cy,dh=error if we didn't.
	mov	dh,NO_MULTICAST
	stc
	ret


get_multicast_list:
;return with nc, es:di ->list of multicast addresses, cx = number of bytes.
;return	cy, NO_ERROR if we don't remember all of the addresses ourselves.
;return cy, NO_MULTICAST if we don't implement multicast.
	mov	dh,NO_MULTICAST
	stc
	ret


reset_interface:
;reset the interface.
	assume	ds:code
	ret


;called	when we	want to determine what to do with a received packet.
;enter with cx = packet length, es:di -> packet type.



;called after we have copied the packet into the buffer.
;enter with ds:si ->the packet, cx = length of the packet.



;
; RX processing has been rewritten for use with BPQ node. A complete
;	packet is always passed through the interface.
;

background:
;
;	see if anything is available - if not, exit
;
	MOV	AL,BYTE PTR IO_ADDR
	MOV	AH,11			; get monitor data
	MOV	DI,OFFSET BUFFER

	CALL	NODE

	CMP	CX,0
	JE	RECV_FRAME_END

	MOV	AL,MSGPORT[DI]
	CMP	AL,BYTE PTR PORT
	JNE	RECV_FRAME_END		; WRONG PORT

	SUB	CX,5			; REMOVE HEADER
;	mov	byte ptr buffer+4,0

	PUSH	CX

	mov	di,0			;but we avoid any segment end bullshit.
	call	recv_find		;look up our type.

	POP	CX

	mov	ax,es			;is this pointer null?
	or	ax,di
	je	recv_frame_3		;yes - just free the frame.

	push	cx
	push	es			;remember where the buffer pointer is.
	push	di

	mov	si,offset buffer+5	; skip header
	rep movsb			; copy data to it

	pop	si			;now give the frame to the client.
	pop	ds
	pop	cx

	assume	ds:nothing

	call	recv_copy

	push	cs
	pop	ds

	assume	ds:code

recv_frame_3:

recv_frame_end:

	ret


STACKINUSE	DB	0

HWTIMERLINK	DD	0		; LINK TO OLD H/W TIMER ROUTINE
	
SAVESTACKSEG	DW	0
SAVESTACKPTR	DW	0


		DW	128 DUP (0)
INTSTACKTOP	LABEL	WORD

HWTIMERINT:
;
;	ENTERED FROM 8253 TIMER 0 INTERRUPT (LEVEL 0 - INT 8)
;
;	CALL SYSTEM TIMER CODE, AND IF NOT ALREADY ACTIVE, CALL MAIN ROUTINE
;
;	save stack
;
	PUSHF				; RETURN IS VIA IRET
	CALL	CS:HWTIMERLINK
;
;	BY NOW INT CONTROLLER HAS BEEN RESET, SO OTHER INTERRUPTS MAY OCCUR -
;	   INTS ARE DISABLED
;
;
	CLD

;	CALL MAIN PROCESSING ROUTINE IF NOT ACTIVE

	CMP	CS:STACKINUSE,0
	JNE	ALREADYACTIVE
;
	MOV	CS:STACKINUSE,1		; FORCE SERIALISATION
;
;	SAVE COMPLETE STATUS
;
;	save stack
;
	PUSH	AX

	MOV	CS:SAVESTACKSEG,SS
	MOV	CS:SAVESTACKPTR,SP
;
	MOV	AX,CS
	MOV	SS,AX
	MOV	SP,OFFSET INTSTACKTOP
;
	STI
;
	PUSH	ES
	PUSH	DS
	PUSH	SI
	PUSH	DI
	PUSH	BP
	PUSH	BX
	PUSH	CX
	PUSH	DX
;
	MOV	DS,AX
	MOV	ES,AX

	CALL	BACKGROUND

	POP	DX
	POP	CX
	POP	BX
	POP	BP
	POP	DI
	POP	SI
	POP	DS
	POP	ES
;
;	RESTORE OLD STACK
;
	CLI

	MOV	SS,CS:SAVESTACKSEG
	MOV	SP,CS:SAVESTACKPTR
;
	POP	AX
	MOV	CS:STACKINUSE,0

ALREADYACTIVE:

	IRET
;
NODE:
	INT	7FH
	RET


endofcode	label	byte

;any code after this will not be kept after initialization.


usage_msg	db	"usage: NODEDRV4 packet_int_no bpqhost_port radio_port ",CR,LF
		DB	CR,LF,'$'


copyright_msg	db	"Packet driver for G8BPQ Node, version ",'0'+majver,".",'0'+version,CR,LF
		db	"Portions Copyright 1988 Phil Karn",CR,LF,'$'


io_addr_name	db	"BPQHOST Port number ",'$'
port_name	db	"Switch Radio Port number ",'$'


;enter with si -> argument string, di -> word to store.
;if there is no number, don't change the number.

;enter with si -> argument string.
;skip spaces and tabs.  Exit with si -> first non-blank char.


parse_args:

	call	skip_blanks

	mov	di,offset io_addr
	mov	bx,offset io_addr_name
	call	get_number

	mov	di,offset port
	mov	bx,offset port_name
	call	get_number

	ret

etopen:
;
	MOV	CL,80H			; REQUEST MONITORING
	MOV	DL,0
	MOV	AL,byte ptr CS:IO_ADDR
	MOV	AH,1			; SET APPL MASK

	CALL	NODE

gotnode:
;
;	hook the system timer - we need to be able to poll the rx buffer
;
	push	ds
	PUSH	ES

	MOV	AX,0
	MOV	DS,AX
;
	MOV	SI,8*4		; ADDR OF H/W TIMER INTERRUPT (INT 8)
;
	MOV	AX,[SI]			; SYSTEM TIMER ROUTINE OFFSET
	MOV	WORD PTR cs:HWTIMERLINK,AX
	MOV	AX,2[SI]
	MOV	WORD PTR cs:HWTIMERLINK+2,AX
;
	MOV	[SI+2],CS
	MOV	AX,OFFSET HWTIMERINT
	MOV	[SI],AX
;
	POP	ES
	POP	DS

	mov	dx,offset endofcode

	clc				; clear carry
	RET
;
;usage_msg is of the form "usage: driver <packet_int_no> <args>"

;copyright_msg is of the form:
;"Packet driver for the foobar",CR,LF
;"Portions Copyright 19xx, J. Random Hacker".

copyleft_msg	label	byte
 db "Packet driver skeleton copyright 1988-89, Russell Nelson.",CR,LF
 db "This program is free software; see the file COPYING for details.",CR,LF
 db "NO WARRANTY; see the file COPYING for details.",CR,LF
crlf_msg	db	CR,LF,'$'

;parse_args should parse the arguments.
;called with ds:si -> immediately after the packet_int_no.
location_msg	db	"Packet driver loaded at segment ",'$'

packet_int_no_name	db	"Packet interrupt number ",'$'

signature	db	'PKT DRVR',0
signature_len	equ	$-signature

already_msg	db	CR,LF,"There is already a packet driver at ",'$'
packet_int_msg	db	CR,LF
		db	"Error: <packet_int_no> should be in the range 0x60 to 0x80"
		db	'$'
int_msg		db	CR,LF
		db	"Error: <int_no> should be no larger than "
int_msg_num	label	word
		db	"xx"
		db	'$'

nonodemsg	db	cr,lf,'Specified COM port not defined to Node',cr,lf,'$'

etopen_diagn	db	0		; errorlevel from etopen if set


NOTBPQERROR	DB	'Switch not found$'
VERSERROR	DB	'Version not compatible with Node Software$'
G8BPQ		DB	'G8BPQ'


BADCONFIGMSG	DB	'Configuration file read error',0DH,0AH,'$'
CONFIGFILENAME	DB	'BPQCFG.BIN',0
NOCONFIGMSG	DB	'Configuration file BPQCFG.BIN not found',0DH,0AH,'$'
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








;etopen should initialize the device.  If it needs to give an error, it
;can issue the error message and quit to dos.

;get the address of the interface.
;enter with es:di -> place to get the address, cx = size of address buffer.
;exit with nc, cx = actual size of address, or cy if buffer not big enough.

already_error:
	mov	dx,offset already_msg
	mov	di,offset packet_int_no
	call	print_number
	mov	ax,4c05h		; give errorlevel 5
	int	21h

usage_error:
	mov	dx,offset usage_msg
error:
	mov	ah,9
	int	21h
	mov	ax,4c0ah		; give errorlevel 10
	int	21h

start_1:

	mov	dx,offset copyright_msg
	mov	ah,9
	int	21h

	mov	dx,offset copyleft_msg
	mov	ah,9
	int	21h

	CALL	CHECKNODE


	mov	si,offset phd_dioa+1
	cmp	byte ptr [si],CR	;end of line?
	je	usage_error

;print the location we were loaded at.
	mov	dx,offset location_msg
	mov	ah,9
	int	21h

	mov	ax,cs			;print cs as a word.
	call	wordout

	mov	dx,offset crlf_msg
	mov	ah,9
	int	21h

	mov	di,offset packet_int_no	;parse the packet interrupt number
	mov	bx,offset packet_int_no_name
	call	get_number		;  for them.

	call	parse_args

	cmp	byte ptr [si],CR	;end of line?
	jne	usage_error

	mov	dx,offset packet_int_msg;make sure that the packet interrupt
	cmp	packet_int_no,60h	;  number is in range.
	jb	error
	cmp	packet_int_no,80h
	ja	error

	mov	ah,35h			;get their packet interrupt.
	mov	al,packet_int_no
	int	21h

	lea	di,3[bx]		;see if there is already a signature
	mov	si,offset signature	;  there.
	mov	cx,signature_len
	repe	cmpsb
	je	already_error		;yes, so we can't go there.

	mov	dx,offset int_msg	;make sure that the packet interrupt
	cmp	int_no,al		;  number is in range.
	jbe	int_ok
	jmp	error
int_ok:

	call	etopen			;init the driver.  If any errors,
					;this routine returns cy.
	jc	no_resident

	push	dx			;remember where they want to end.

	mov	ah,35h			;remember their packet interrupt.
	mov	al,packet_int_no
	int	21h
	mov	their_isr.offs,bx
	mov	their_isr.segm,es

	mov	ah,25h			;install our packet interrupt
	mov	dx,offset our_isr
	int	21h


	mov	ah,49h			;free our environment, because
	mov	es,phd_environ		;  we won't need it.
	int	21h

	mov	bx,1			;get the stdout handle.
	mov	ah,3eh			;close it in case they redirected it.
	int	21h

	pop	dx			;get their ending address.
	add	dx,0fh			;round up to next highest paragraph.
	mov	cl,4
	shr	dx,cl
	mov	ah,31h			;terminate, stay resident.
	mov	al,etopen_diagn		; errorlevel (0 - 9, just diagnostics)
	int	21h

no_resident:
	mov	ax,4c00h + 32		; give errorlevel 32
	cmp	al,etopen_diagn
	ja	no_et_diagn		; etopen gave specific reason?
	mov	al,etopen_diagn		; yes, use that for error level
no_et_diagn:
	int	21h

; 			Suggested errorlevels:
;
; _____________________  0 = normal
; 			 1 = unsuitable memory address given; corrected
; In most cases every-	 2 = unsuitable IRQ level given; corrected
; thing should work as	 3 = unsuitable DMA channel given; corrected
; expected for lev 1-5	 4 = unsuitable IO addr given; corrected (only 1 card)
; _____________________	 5 = packet driver for this int # already loaded
; External errors, when	20 = general cable failure (but pkt driver is loaded)
; corrected normal	21 = network cable is open             -"-
; operation starts	22 = network cable is shorted          -"-
; _____________________ 23 = 
; Packet driver not	30 = usage message
; loaded. A new load	31 = arguments out of range
; attempt must be done	32 = unspecified device initialization error
;			33 = 
;			34 = suggested memory already occupied
;			35 = suggested IRQ already occupied
;			36 = suggested DMA channel already occupied
;			37 = could not find the network card at this IO address

	public	get_number
get_number:
	mov	bp,10			;we default to 10.
	jmp	short get_number_0

	public	get_hex
get_hex:
	mov	bp,16
;get a hex number, skipping leading blanks.
;enter with si->string of digits,
;	bx -> dollar terminated name of number,
;	di -> dword to store the number in.  [di] is not modified if no
;		digits are given, so it acts as the default.
;return cy if there are no digits at all.
;return nc, bx:cx = number, and store bx:cx at [di].
get_number_0:
	push	bx			;remember the name of this number.
	call	skip_blanks
	call	get_digit		;is there really a number here?
	jc	get_number_3
	or	al,al			;Does the number begin with zero?
	jne	get_number_4		;no.
	mov	bp,8			;yes - they want octal.
get_number_4:

	xor	cx,cx			;get a hex number.
	xor	bx,bx
get_number_1:
	lodsb
	cmp	al,'x'			;did they really want hex?
	je	get_number_5		;yes.
	cmp	al,'X'			;did they really want hex?
	je	get_number_5		;yes.
	call	get_digit		;convert a character into an int.
	jc	get_number_2		;not a digit (neither hex nor dec).
	xor	ah,ah
	cmp	ax,bp			;larger than our base?
	jae	get_number_2		;yes.

	push	ax			;save the new digit.

	mov	ax,bp			;multiply the low word by ten.
	mul	cx
	mov	cx,ax			;keep the low word.
	push	dx			;save the high word for later.
	mov	ax,bp
	mul	bx
	mov	bx,ax			;we keep only the low word (which is our high word)
	pop	dx
	add	bx,ax			;add the high result from earlier.

	pop	ax			;get the new digit back.
	add	cx,ax			;add the new digit in.
	adc	bx,0
	jmp	get_number_1
get_number_5:
	mov	bp,16			;change the base to hex.
	jmp	get_number_1
get_number_2:
	dec	si
	mov	[di],cx			;store the parsed number.
	mov	[di+2],bx
	clc
	jmp	short get_number_6
get_number_3:
	mov	cx,-1
	mov	bx,-1
	cmp	al,'?'			;did they ask for the default?
	je	get_number_2		;yes - give them -1.
	stc
get_number_6:
	pop	dx			;get the name of the number back.

	pushf				;save some stuff.
	push	bx
	push	cx
	push	si
	push	di
	call	print_number
	pop	di
	pop	si
	pop	cx
	pop	bx
	popf
	ret


print_number:
;enter with dx -> dollar terminated name of number, di ->dword.
;exit with the number printed and the cursor advanced to the next line.
	mov	ah,9			;print the name of the number.
	int	21h
	mov	al,'0'
	call	chrout
	mov	al,'x'
	call	chrout
	mov	ax,[di]			;print the number in hex.
	mov	dx,[di+2]
	call	hexout
	mov	al,' '
	call	chrout
	mov	al,'('
	call	chrout
	mov	ax,[di]			;print the number in decimal.
	mov	dx,[di+2]
	call	decout
	mov	al,')'
	call	chrout
	mov	al,CR
	call	chrout
	mov	al,LF
	call	chrout
	ret


	public	skip_blanks
skip_blanks:
	lodsb				;skip blanks.
	cmp	al,' '
	je	skip_blanks
	cmp	al,HT
	je	skip_blanks
	dec	si
	ret


get_digit:
;enter with al = character
;return nc, al=digit, or cy if not a digit.
	cmp	al,'0'			;decimal digit?
	jb	get_digit_1		;no.
	cmp	al,'9'			;. .?
	ja	get_digit_2		;no.
	sub	al,'0'
	clc
	ret
get_digit_2:
	or	al,20h
	cmp	al,'a'			;hex digit?
	jb	get_digit_1
	cmp	al,'f'			;hex digit?
	ja	get_digit_1
	sub	al,'a'-10
	clc
	ret
get_digit_1:
	stc
	ret


	public	hexout
hexout:
	mov	cl,'0'			;prepare to eliminate leading zeroes.
	xchg	ax,dx			;just output 32 bits in hex.
	call	wordout			;output dx.
	xchg	ax,dx
	jmp	wordout			;output ax.


	public	decout
decout:
	mov	si,ax			;get the number where we want it.
	mov	di,dx
	or	ax,dx			;is the number zero?
	jne	decout_nonzero
	mov	al,'0'			;yes - easier to just print it, then
	jmp	chrout			;  to eliminate all but the last zero.
decout_nonzero:

	xor	ax,ax			;start with all zeroes in al,bx,bp
	mov	bx,ax
	mov	bp,ax

	mov	cx,32			;32 bits in two 16 bit registers.
decout_1:
	shl	si,1
	rcl	di,1
	xchg	bp,ax
	call	addbit
	xchg	bp,ax
	xchg	bx,ax
	call	addbit
	xchg	bx,ax
	adc	al,al
	daa
	loop	decout_1

	mov	cl,'0'			;prepare to eliminate leading zeroes.
	call	byteout			;output the first two.
	mov	ax,bx			;output the next four
	call	wordout			;output the next four
	mov	ax,bp
wordout:
	push	ax
	mov	al,ah
	call	byteout
	pop	ax
byteout:
	mov	ah,al
	shr	al,1
	shr	al,1
	shr	al,1
	shr	al,1
	call	digout
	mov	al,ah
digout:
	and	al,0fh
	add	al,90h	;binary digit to ascii hex digit.
	daa
	adc	al,40h
	daa
	cmp	al,cl			;leading zero?
	je	return
	mov	cl,-1			;no more leading zeros.
chrout:
	push	ax			;print the char in al.
	xchg	al,dl
	mov	ah,2
	int	21h
	xchg	al,dl
	pop	ax
return:
	ret


addbit:	adc	al,al
	daa
	xchg	al,ah
	adc	al,al
	daa
	xchg	al,ah
	ret

CHECKNODE:
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
	MOV	BYTE PTR NODE+1,AL	; PATCH NODE CALL INSTRUCTION
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

	LDS	SI,DS:[SI]		; GET POINTER TO HOST SUPPORT CODE

	SUB	SI,7
	MOV	DI,OFFSET G8BPQ
	MOV	CX,5
	REP CMPSB			; MAKE SURE SWITCH IS LOADED

	JE	SWITCHOK

	POP	DS

	MOV	DX,OFFSET NOTBPQERROR

	MOV	AH,9
	INT	21H

EXIT:
	MOV	AX,4C00H
	INT	21H

switchok:

	POP	DS

	RET
;
code	ends

	end	start
