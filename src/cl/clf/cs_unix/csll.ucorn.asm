! Copyright (c) 2004 Ingres Corporation
!
! History:
!    15-jul-92 (swm)
!	The <sys/asm_linkage.h> file moved to <sys/sparc/asm_linkage.h>.
!	Enabled this code for dr6_uv1, also a sparce-based DRS6000.
!    13-mar-95 (smiba01)
!	Added dr6_ues to list of users of this routine.
!    19-sep-95 (nick)
!	Added CS_getsp().
!
# if defined(dr6_us5) || defined(dr6_uv1) || defined(dr6_ues)
# include <sys/sparc/asm_linkage.h>
# include <sys/trap.h>

CS_CURRENT		= 0xb8	! Cs_srv_block.cs_current (the current thread)
CS_INKERNEL		= 0xcc	! Cs_srv_block.cs_kernel (is switching on?)

CS_MACHINE_SPECIFIC	= 0x20	! offset of machine dependent information
				! within the scb

CS_SP			= CS_MACHINE_SPECIFIC		! stack pointer
CS_PC			= CS_MACHINE_SPECIFIC + 0x4	! program counter
CS_EXSP			= CS_MACHINE_SPECIFIC + 0x8	! EX stack pointer

!
! CS_swuser.
!
! Note: the ldstub operations below are atomic so we cannot get a race
! condition even on multiple CPUs.
!
	.section	".text"
	.global	_CS_swuser
_CS_swuser:
!
! Get a new register window for ourselves and allocate
! a minimum stack.  CS_swuser() runs as a "heavyweight"
! function so that it can call other functions safely
! (namely CS_xchng_thread). "-SA(MINFRAME)" comes from
! <sys/asm_linkage.h>.
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
	ta	ST_FLUSH_WINDOWS	!flush reg windows of current thread

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
	ldstub	[%o0],%o1	!set mem to all 1's, current val of mem -> %l0
	cmp	%o1,0x0		!compare tas bit with 0
	be,a	X		!if equel to zero, goto X
	nop			!no op for branch
	jmp	%o7 + 0x8
	mov	0x0,%o0		!return 0
X:
	jmp	%o7 + 0x8
	mov	0x1,%o0		!return 1

!{
! Name: CS_getsp()	- get stack pointer on sparc
!
! Description:
!	Returns the current stack pointer.
!
! Inputs:
!      none.
!
! Outputs:
!      none.
!
!	Returns:
!	    stack pointer
!
!	Exceptions:
!	    none
!
! Side Effects:
!	    none
!
! History:
!      19-sep-95 (nick)
!          Created. 
!	   Leaf procedure so don't require a stack window
!
	.section	".text"
	.global	_CS_getsp
	.global	CS_getsp

_CS_getsp:
CS_getsp:
	ta	ST_FLUSH_WINDOWS
	retl
	mov	%sp, %o0

# endif /* dr6_us5 dr6_uv1 */

