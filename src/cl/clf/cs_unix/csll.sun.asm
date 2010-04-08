! Copyright (c) 2004 Ingres Corporation
!
! CSLL.SUN.ASM
!
! Assembler code for Sun Microsystems ports
!
! History:
!	4-june-90 (blaise)
!	    Integrated changes from termcl code line:
!		Change sparc to support interlocked clr and avoid spinlocking
!		on the bus.
!	8-Aug-90 (GordonW)
!	    For Sun4 collide counter is 4 bytes into spinlock structure not 2.
!	12-feb-91 (mikem)
!	    Added new cl internal routine CS_getsp() for use on sun4's to print
!	    out stack trace of a thread.
!	9-aug-93 (robf)
!	    Add su4_cmw, based on su4_u42
!

# ifdef sun_u42

CS__MACHINE_SPECIFIC	= 0x20
CS__CCSAVE              = 0
CS__PCSAVE		= 0x4
CS__REGSET		= 0x8
CS__CURRENT		= 0xb8
CS__INKERNEL		= 0xcc
CS_INITIALIZING		= 0x10
CS__SP			= CS__REGSET + (0xf * 4)

.globl	_CS_swuser, _CS_tas, _CS_getspin

_CS_swuser:
| If server in kernel state, no task switching
	tstl	_Cs_srv_block+CS__INKERNEL
	bnes	tas_ret
| if there is no current session, don't save registers
	movl	_Cs_srv_block+CS__CURRENT, d0
	movl	d0,a0
	beqs	CS__pick_new
| at this point a0 points to the session control block
| save the state
| we keep the exception stack from EX in d0 in the register save array
	movl	_ex_sptr,d0
	moveml	#0xffff,a0@(CS__MACHINE_SPECIFIC+CS__REGSET)
CS__pick_new:
| run xchng thread on idle thread's stack
	movl	_Cs_idle_scb+CS__MACHINE_SPECIFIC+CS__SP, sp
| and use idle thread's exception stack as well
	movl	_Cs_idle_scb+CS__MACHINE_SPECIFIC+CS__REGSET, _ex_sptr
| new_scb = CS_xchng_thread(old_scb)
	pea	a0@
	bsrl	_CS_xchng_thread
| don't bother to pop arguement off stack since we change the sp anyway
| at return time, d0 has new current SCB, move it and go
	movl	d0,a0
	moveml	a0@(CS__MACHINE_SPECIFIC+CS__REGSET),#0xffff
| recover the exception stack
	movl	d0,_ex_sptr
	rts

_CS_tas:
| get the argument value into a0
	movl	sp@(4),a0
| clear the return value
	clrl	d0
| do the test and set of the byte that a0 points to
	tas	a0@
| if the was previously set, i.e. the test and set failed, return
	bnes	tas_ret
| the bit was not previously set, i.e. the test and set worked, return 1
	moveq	#1, d0
tas_ret: rts

_CS_getspin:
| get arguement which should be a pointer to CS_SPIN into a0
	movl	sp@(4),a0
| do a test and set on the cssp_bit member
	tas	a0@
| if the test and set succeded, then we have the lock, return
	beqs	tas_ret
| we failed to get the lock on the first attempt,
| now we spin trying to get the lock.
csspinit:
	tas	a0@
	bnes	csspinit
| finally we have the lock, bump the collision counter to indicate
| that we did have to spin
	addw	#1,a0@(2)
| return
	rts
# endif  /* sun_u42 */

# if defined(su4_u42) || defined(su4_cmw)
#include <sun4/asm_linkage.h>
#include <sparc/trap.h>

CS_CURRENT		= 0xb8	! Cs_srv_block.cs_current (the current thread)
CS_INKERNEL		= 0xcc	! Cs_srv_block.cs_kernel (is switching on?)

CS_MACHINE_SPECIFIC	= 0x20	! offset of machine dependent information
				! within the scb

CS_SP			= CS_MACHINE_SPECIFIC		! stack pointer
CS_PC			= CS_MACHINE_SPECIFIC + 0x4	! program counter
CS_EXSP			= CS_MACHINE_SPECIFIC + 0x8	! EX stack pointer

!
! CS_swuser. Da, da, dummmmm...
!
	.seg	"text"
	.global	_CS_swuser
_CS_swuser:
!
! Get a new register window for ourselves and allocate
! a minimum stack.  CS_swuser() runs as a "heavyweight"
! function so that it can call other functions safely
! (namely CS_xchng_thread). "-SA(MINFRAME)" comes from
! <sun4/asm_linkage.h>.
!
	save	%sp,-SA(MINFRAME),%sp
!
! If server is in kernel state, no task switching (return immediately).
! NOTE: _Cs_srv_block is an externally defined global.
!
	sethi	%hi(_Cs_srv_block),%l1     ![Cs_srv_block+CS_INKERNEL]->%l0
	add	%l1,%lo(_Cs_srv_block),%l1
	ld	[%l1 + CS_INKERNEL],%l0

	cmp	%l0,0x0
	bne,a	sw_ret
	nop
!
! If there is no current session, don't save registers, just
! pick a new thread.  This happens when a thread terminates.
!

	sethi	%hi(_Cs_srv_block),%l1     ![Cs_srv_block+CS_CURRENT]->%l0
	add	%l1,%lo(_Cs_srv_block),%l1
	ld	[%l1 + CS_CURRENT],%l0

	cmp	%l0,0x0
	be,a	CS_pick_new
	nop
!
! Save the state of the current thread.
!
	st	%fp, [%l0 + CS_SP]	!store stack pointer

	sethi	%hi(_ex_sptr),%l1	!store exception stack pointer
	ld	[%l1 + %lo(_ex_sptr)], %l2
	st	%l2, [%l0 + CS_EXSP]

	st	%i7, [%l0 + CS_PC]	!store return address of caller
!
! Call the scheduler to pick a new thread.  %l0 contains
! the addr of current scb, and is passed as arg[0].
!
CS_pick_new:
	call	_CS_xchng_thread
	add	%l0,0x0,%o0

!
! Resume the new thread.  %o0 is the return code of
! CS_xchng_thread, which is the addr of the new scb.
!
	ta	0x3			!flush reg windows of current thread

	ld	[%o0 + CS_SP], %fp	!restore stack pointer of new thread

	ld	[%o0 + CS_PC], %i7	!restore return address of new thread

	ld	[%o0 + CS_EXSP], %l0	!restore EX stack ptr of new thread
	sethi	%hi(_ex_sptr), %l1
	st	%l0, [%l1 + %lo(_ex_sptr)]

!
! Return.
!
sw_ret:
	ret
	restore	%g0,0x0,%o0		!return 0

!
! Atomic test and set.
!
	.global _CS_tas
_CS_tas:
        mov     0x1,%o1
        swap    [%o0],%o1       !set mem to 1, current val of mem -> %o1
        tst     %o1             !compare tas bit with 0
        bne,a   gs_ret          !if not equel to zero, return zero
	nop			!no op for branch

        retl
	mov	0x1,%o0		!return 1

!
! Get spinlock.
!
	.global _CS_getspin, _CS_relspin
_CS_getspin:
!
! Perform initial tas.  If not set, return.
!
        mov     0x1,%o1
        swap    [%o0],%o1       !set mem to 1, current val of mem -> %o1
        tst     %o1             !compare tas bit with 0
	be,a	gs_ret		!if equel, goto
	nop			!no op for branch
!
! Spin, spin, spin!!
!
	ld      [%o0],%o1       !first, just test to avoid locking bus
csspinit:
	tst     %o1
        bne,a   csspinit
        ld      [%o0],%o1       !tight loop - delay instruction

        mov     0x1,%o1         ! real test-and-set
        swap    [%o0],%o1       !set mem to 1, current val of mem -> %o1
        tst     %o1             !compare tas bit with 0
	bne,a	csspinit	!if not equal, goto
	ld      [%o0],%o1       !tight loop - delay instruction
!
! Had to spin; increment cssp_collide.
!
	lduh	[%o0 + 0x4],%o1	!load unsigned halfword (collide) into %o1
	inc	%o1		!add 1 to %o1
	sth	%o1,[%o0 + 0x4]	!store %o1 to collide
!
! Return.
!
gs_ret:
        retl                    !return 0
        clr     %o0

!
! CS_relspin (and CS_ACLR)
!

_CS_relspin:
        clr     %o1
        swap    [%o0],%o1       !atomically swap in a zero
        retl
        clr     %o0

!{
! Name: CS_getsp()	- get stack point on sun4
!
! Description:
!      Special assembly code to return the stack pointer on a sun4 machine.
!      Also takes care of flushing the register cache so that calling "c"
!      code can walk the stack of any thread (given that it already has the
!      the stack pointer) printing out a stack trace with no additional 
!      assembly language necessary.
!
! Inputs:
!      none.
!
! Outputs:
!      none.
!
!	Returns:
!	    returns 
!	    E_DB_OK
!
!	Exceptions:
!	    none
!
! Side Effects:
!	    none
!
! History:
!      12-feb-91 (mikem)
!          Created (code provide jeff anton).
!
.text
.globl	_CS_getsp

_CS_getsp:	ta	ST_FLUSH_WINDOWS
	retl
	mov	%sp, %o0

# endif /* su4_u42 */
