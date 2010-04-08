

; Define some constants

; Offset of 'machine specifics' within CS_SCB

	def CS__MACHINE_SPECIFIC, 	0x20

; Offset of cs_registers within CS__MACHINE_SPECIFIC

	def CS__REG_SET,  		0x4

; Offsets to the sp (r31) and fp (r30)

	def CS__REG_FP,			0x44
	def CS__REG_SP,			0x48


; Offset of cs_current and cs_inkernel within Cs_srv_block

	def CS__CURRENT,      		0xb8
	def CS__INKERNEL,	   	0xcc

; Declare externals
; Imports

	global _Cs_srv_block
	global _ex_sptr
	global _CS_xchng_thread
	global _Cs_idle_scb

; Exports

	global _CS_swuser
	global _CS_tas
	global _CS_getspin
	global _CSget_stk_pages
	global _CSinit_pg_base
        global _CS_aclr
        global _CS_isset

; Declare the text section
; Ensure alignment

	text
	align	4

; On the 88K, the stack grows from high address to low.
; On this machine, a word is 4 bytes, and BIG ended.
; R0 is always 0, on this machine.

_CS_swuser :

; Take care of stack discipline first. Whenever a subroutine is 
; called, at least 0x28 bytes must be reserved on the stack. 0x8 for the
; return pc, and the old fp, and 0x20 for parameters (even if 
; less than 8 are passed.)

; Allocate the space by moving the sp (r31)

	subu	r31,r31,0x28

; Save the return pc (r1) and fp (r30) before all of the parameters

	st	r1,r31,0x24
	st	r30,r31,0x20

; Set the new fp (r30) to point to the one just saved.

	addu	r30,r31,0x20

; If server in kernel state, no task switching, (return immediatly)
; r10 = _Cs_srv_block

; 32 bit addresses cannot be placed in registers directly. The high half
; word must be moved first, then the lower half word.

; 	or	r10, r0, _Cs_srv_block
	or.u	r10, r0, hi16(_Cs_srv_block)
	or	r10, r10,lo16(_Cs_srv_block)

; If in kernel state return.

	ld	r11, r10, CS__INKERNEL
	cmp	r12, r11, 0x0
	bb1	ne,  r12, sw_ret

; Get the address of _ex_sptr. Keep it around so that we can assign to it
; later. Use r11 to save it.

;	or	r11, r0,  _ex_sptr
	or.u	r11, r0,  hi16(_ex_sptr)
	or	r11, r11, lo16(_ex_sptr)

; If there is no current session, don't save registers.
; r2 has the current scb, keep it there so it can be passed to CS_xchng_thread.

	ld	r2,  r10, CS__CURRENT
	cmp	r12, r2,  0x0
	bb1	eq,  r12, CS__pick_new

; Save _ex_sptr.

	ld	r12, r11, 0x0
	st	r12, r2,  CS__MACHINE_SPECIFIC + 0x0

; Save the rest of the registers, one at a time.

; Save r1, aka: the return pc.

	st	r1,  r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x0

; Save r14..r29

	st	r14, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x04
	st	r15, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x08
	st	r16, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x0c
	st	r17, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x10
	st	r18, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x14
	st	r19, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x18
	st	r20, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x1c
	st	r21, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x20
	st	r22, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x24
	st	r23, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x28
	st	r24, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x2c
	st	r25, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x30
	st	r26, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x34
	st	r27, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x38
	st	r28, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x3c
	st	r29, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x40

; Save r30, aka: the frame pointer

	st	r30, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + CS__REG_FP

; Save r31, aka: the stack pointer

	st	r31, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + CS__REG_SP

CS__pick_new:

; Use idle thread's sp, fp  and exception stack.
; r12 -> idle thread

; 	or	r12, r0,  _Cs_idle_scb
	or.u	r12, r0,  hi16(_Cs_idle_scb)
	or	r12, r12, lo16(_Cs_idle_scb)

	ld	r31, r12, CS__MACHINE_SPECIFIC + CS__REG_SET + CS__REG_SP
	ld	r30, r12, CS__MACHINE_SPECIFIC + CS__REG_SET + CS__REG_FP
	ld	r10, r12, CS__MACHINE_SPECIFIC + 0x0
	st	r10, r11, 0x0

; Call _CS_xchng_thread
; Stack is adjusted for parameters above.
	
 	bsr	_CS_xchng_thread

; New_scb is in r2
; Recover everything saved.

; Recover saved pc, aka. r1	
	
	ld	r1,  r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x0

; Recover r14..r29
	
	ld	r14, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x04
	ld	r15, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x08
	ld	r16, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x0c
	ld	r17, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x10
	ld	r18, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x14
	ld	r19, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x18
	ld	r20, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x1c
	ld	r21, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x20
	ld	r22, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x24
	ld	r23, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x28
	ld	r24, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x2c
	ld	r25, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x30
	ld	r26, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x34
	ld	r27, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x38
	ld	r28, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x3c
	ld	r29, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + 0x40

; Recover saved fp, aka. r30

	ld	r30, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + CS__REG_FP

; Recover saved sp, r31

	ld	r31, r2, CS__MACHINE_SPECIFIC + CS__REG_SET + CS__REG_SP

; Recover exception stack

;	or	r11, r0,  _ex_sptr
	or.u	r11, r0,  hi16(_ex_sptr)
	or	r11, r11, lo16(_ex_sptr)

	ld	r10, r2, CS__MACHINE_SPECIFIC + 0x0
	st	r10, r11, 0x0

; Now clean up the stack and return.
; Reset sp (r31) to be safe. Use fp (r30) and move over parameters.

sw_ret:
	subu	r31, r30, 0x20

; Restore the saved pc (r1), fp (r30), and sp (r31)
; Save r1 into r2 first

	or	r2,  r1,  r0
	ld 	r1,  r31, 0x24
	ld	r30, r31, 0x20
	addu	r31, r31, 0x28

; Return
; Use r2 to return, not r1. If r1 is used, we will never be able to
; get back to Cseradicate

	jmp 	r2



        align 4
_CS_isset:
;
; Atomically determine if an 8 bit byte is zero.  The byte pointer is held in
; r2 originally.  r2 also holds the return value on exit.  Our strategey is
; to simply return the value pointed to by r2 since that means our function
; will return true if that byte is set.
; 30-sep-91 (jillb/kirke)
; Change ld.bu to ld to use word pointer instead of byte pointer. With
; 64, isset must be int to prevent core dump under DG/UX.
;
; Save the byte pointer to r3 since r2 will hold return value
        or      r3, r2, 0x0

; Zero out r2
        or      r2, r0, 0x0

; Load the value pointed to by r3 and use it as the return
        ld      r2, r3, 0x0

; Return
        jmp     r1



        align 4
_CS_aclr:
;
; 24-sep-91 (jillb/jro--DGC) (from 6.3)
; Atomically clear an 8 bit value.   Take advantage of the fact that r0 is
; always zero.  r2 contains a pointer to the word to clear.
; Change st.b to st to use word pointer instead of byte pointer to prevent
; looping in csspinit.
;

; Clear the byte that r2 points to
        st      r0, r2, 0x0

; Return
        jmp     r1



	align 4
_CS_tas:
; The 'xmem' instruction is the only non interruptable, atomic  88k instruction.
; Algorithm :
;    1. Set a scratch register
;    2. Using 'xmem' exchange the scratch with the argument.
;    3. If the scratch is 0, then the argument was not set, and is now
;		set as a result of the 'xmem'. Return 1
;    4. If the scratch is 1, then the argument was set, and is still 
;		set as a result of the 'xmem'. Return 0
; r2 - Argument and return value. Move argument to r4, and set default
;      return value (1).
; r3 - Scratch. Prime with 0x1.
; r4 - Move argument (r2) here so that r2 can hold return value.
; r5 - Use to do comparison.
;
; 25-sep-1991 (jillb--DGC)
; With DG/UX 5.4, xmem.bu produces a warning:
;	immediate form of third op not allowed in newer 88k processors
; Changed all references of:	xmem.bu	r3, 	r4,	0x0
; 			 to: 	xmem	r3,	r4,	r0

; Move r2 to r4, 
	or	r4,	r2,	0x0

; Prime r3 with 0x1, r2 (return value) with 0x1.
	or	r3,	r0,	0x1
	or	r2,	r0,	0x1

; Call 'xmem'.
	xmem	r3, 	r4,	r0

; Look at the result, and put result in r5.
	cmp	r5,	r3,	0x0

; If (r5 == 0), it was not set, return 1.
; Remember, return value already set, so branch to return.
	bb1	eq,	r5,	tas_ret

; If (r5 == 1), it was not set, return 0. Set return value to 0.
	or	r2,	r0,	0x0

; Return. Return value is already in r2.
tas_ret:
	jmp 	r1



	align 4

_CS_getspin:

; Dereference r2, into r4
	

; 'tas' r4, use r3 as scratch, r5 for cmp

	or	r3,	r0,	0x1
	xmem	r3,	r2,	r0
	cmp	r5,	r3,	0x0

; If not set return.

	bb1	eq,	r5,	gs_ret

; Failed, so spin...........

csspinit:

	or	r3,	r0,	0x1
	xmem	r3,	r2,	r0
	cmp	r5,	r3,	0x0
	bb1	ne,	r5,	csspinit

; Done spinning.  
; Bump collision counter Use r4. 

	ld.hu	r4,	r2,	0x2
	add.ci	r4,	r4,	0x1
	st.h	r4,	r2,	0x2

; Return
gs_ret:
	jmp 	r1


	align 4

_CSget_stk_pages:

; Take care of stack stuff first.

	subu	r31,	r31,	0x28
	st	r1,	r31,	0x24
	st	r30,	r31,	0x20
	addu	r30,	r31,	0x20
	or	r10,	r2,	0x0

; Check if CSstk_pg_base is initialized

	or.u	r11,	r0,	hi16(CSstk_pg_base)
	or	r11,	r11,	lo16(CSstk_pg_base)
	ld	r3,	r11,	0x0
	cmp	r4,	r3,	0x0
	bb0	eq,	r4,	initialized
	bsr	_CSinit_pg_base

initialized:

; Figure out how many bytes are needed, (npages * 8192)

	mak	r3,	r10,	0<13>

; Save current sp, and point sp to CSstk_base

	or	r12,	r31,	r0
	ld	r31,	r11,	0x0

; Zero out the stack.

loop2:
	subu	r31,	r31,	0x4
	st	r0,	r31,	0x0
	subu	r3,	r3,	0x4
	cmp	r13,	r3,	0x0
	bb1	gt,	r13,	loop2

; Set new CSstk_pg_base

	st	r31,	r11,	0x0

; Get ready to return

	or	r2,	r31,	0x0

; Restore sp

	or	r31,	r12,	0x0

; Fix up the stack

	ld	r30,	r31,	0x20
	ld	r1,	r31,	0x24
	addu	r31,	r31,	0x28

; Bye
	jmp	r1

	align 4

_CSinit_pg_base:

; Take care of stack stuff

	subu	r31,	r31,	0x28
	st	r1,	r31,	0x24
	st	r30,	r31,	0x20
	addu	r30,	r31,	0x20

; Figure out next boundary, using ME_MPAGESIZE (8192)

	subu	r2,	r31,	0x2000
	and	r2,	r2,	0xe000

; Figure out how many bytes need to be zero'd out.

	subu	r3,	r31,	r2

; Save the stack pointer

	or	r4,	r31,	0x0

; Zero out

loop1:
	subu	r31,	r31,	0x4
	st	r0,	r31,	0x0
	subu	r3,	r3,	0x4
	cmp	r5,	r3,	0x0
	bb1	gt,	r5,	loop1

; Restore stack, and initialize CSstk_base

	or.u	r2,	r0,	hi16(CSstk_pg_base)
	or	r2,	r2,	lo16(CSstk_pg_base)
	st	r31,	r2,	r0
	or	r31,	r4,	0x0

; Fix up the stack

	ld	r30,	r31,	0x20
	ld	r1,	r31,	0x24
	addu	r31,	r31,	0x28

; bye
	
	jmp r1


	data
	align 4
CSstk_pg_base:	word	0x0
