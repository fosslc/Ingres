!
! Copyright (c) 2004 Ingres Corporation
!

# if defined(su4_us5)
#define _ASM
#include <sys/asm_linkage.h> 
#include <sys/trap.h>


!
!  Name: CSLL.SOL.ASM - Assembler code for sun microsystem solaris 2.x ports.
!
!  Description:
!      Assembler code for sun microsystem solaris 2.x OS.
!
!  History:
!      07-nov-92 (mikem/bonobo)
!          su4_us5 port. Created (started with su4_u42 and made some changes
!	   to get assembler to take it.
!      23-aug-93 (mikem)
!	   Starting with Solaris SunOS 5.3 alpha 2.5, files included by
!	   <sys/asm_linkage.h> require _ASM to be defined otherwise C typedef's
!	   are defined and cause the assembler to fail on this file.
!      24-apr-95 (hanch04,nick)
!           Code assembler versions of CS_su4_setup() and CS_su4_eradicate()
!           to overcome size and instruction variability in the compiled up
!           C versions of these between V2 and V3 of the Solaris 2 compiler.
!      19-sep-95 (nick)
!           Add CS_su4_stack_base as sane stopping point for stack traces.
!      18-feb-1997 (hanch04)
!	    As part of merging of Ingres-threaded and OS-threaded servers,
!	    Added CS_aclr
!      02-dec-2003 (devjo01)
!           Added CScas4, and CScasptr.
!      31-aug-2004 (stephenb)
!	    Fix up typing for external functions
!	12-Nov-2010 (kschendel) SIR 124685
!	    Moved inkernel and current to front of CS_SYSTEM, fix here.
!


CS_CURRENT		= 0x14	! Cs_srv_block.cs_current (the current thread)
CS_INKERNEL		= 0x10	! Cs_srv_block.cs_kernel (is switching on?)

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
	.global	CS_swuser
	.type CS_swuser,#function
_CS_swuser:
CS_swuser:
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
	sethi	%hi(Cs_srv_block),%l1     ![Cs_srv_block+CS_INKERNEL]->%l0
	add	%l1,%lo(Cs_srv_block),%l1
	ld	[%l1 + CS_INKERNEL],%l0

	cmp	%l0,0x0
	bne,a	sw_ret
	nop
!
! If there is no current session, don't save registers, just
! pick a new thread.  This happens when a thread terminates.
!

	sethi	%hi(Cs_srv_block),%l1     ![Cs_srv_block+CS_CURRENT]->%l0
	add	%l1,%lo(Cs_srv_block),%l1
	ld	[%l1 + CS_CURRENT],%l0

	cmp	%l0,0x0
	be,a	CS_pick_new
	nop
!
! Save the state of the current thread.
!
	st	%fp, [%l0 + CS_SP]	!store stack pointer

	sethi	%hi(ex_sptr),%l1	!store exception stack pointer
	ld	[%l1 + %lo(ex_sptr)], %l2
	st	%l2, [%l0 + CS_EXSP]

	st	%i7, [%l0 + CS_PC]	!store return address of caller
!
! Call the scheduler to pick a new thread.  %l0 contains
! the addr of current scb, and is passed as arg[0].
!
CS_pick_new:
	call	CS_xchng_thread
	add	%l0,0x0,%o0

!
! Resume the new thread.  %o0 is the return code of
! CS_xchng_thread, which is the addr of the new scb.
!
	ta	ST_FLUSH_WINDOWS	!flush reg windows of current thread

	ld	[%o0 + CS_SP], %fp	!restore stack pointer of new thread

	ld	[%o0 + CS_PC], %i7	!restore return address of new thread

	ld	[%o0 + CS_EXSP], %l0	!restore EX stack ptr of new thread
	sethi	%hi(ex_sptr), %l1
	st	%l0, [%l1 + %lo(ex_sptr)]

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
	.global CS_tas
	.type CS_tas,#function
_CS_tas:
CS_tas:
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
	.global _CS_getspin, _CS_relspin, _CS_aclr
	.global CS_getspin, CS_relspin, CS_aclr
	.type CS_aclr,#function
_CS_getspin:
CS_getspin:
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

_CS_aclr:
CS_aclr:
_CS_relspin:
CS_relspin:
        clr     %o1
        swap    [%o0],%o1       !atomically swap in a zero
        retl
        clr     %o0

!{
! Name: CScas4()        - Atomically Compare & Swap 4 byte value.
!
! Description:
!      This routine performs an atomic compare and swap of an
!      aligned four byte value.   Logically this behaves like the
!      following 'C' code, but special instructions are used to
!      assure that 'newval' is not written if the contents of
!      the target address have changed.
!
! STATUS CScas4( i4 *addr, i4 oldval, i4 newval )
! {
!     if ( *addr == oldval )
!     {
!        *addr = newval;
!        return OK;
!     }
!     *oldval = *addr;
!     return FAIL;  // any non-zero value.
! }
!
!      This is used as follows:
!
!
! do
! {
!      // dirty read oldval
!      oldval = *addr;
!      // Calculate new value based on old val
!      newval = ...;
!
! } while ( CScas4( addr, oldval, newval ) );
!
! // continue processing knowing that newval was written
! // before oldval changed, and that all other writers using
! // compare and swap will see your update.

        .global CScas4
        .global CScasptr
CScas4:
CScasptr:
        cas     [%o0], %o1, %o2
        retl
        sub     %o1, %o2, %o0

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
.global	_CS_getsp
.global	CS_getsp
.type CS_getsp,#function

_CS_getsp:
CS_getsp:	ta	ST_FLUSH_WINDOWS
	retl
	mov	%sp, %o0

!{
! Name: CS_su4_stack_base()
!
! Description:
!
!	Null function to give sane stopping point for stack
!	traces in DBX etc.
!
! Inputs:
!      none.
!
! Outputs:
!      none.
!
! Returns:
!	never called
!
! History:
!      19-sep-95 (nick)
!          Created
!
	.section	".text"
	.align	4 
	.global	CS_su4_stack_base
	.type	CS_su4_stack_base,#function

CS_su4_stack_base:
	retl
	nop
!
!{
! Name: CS_su4_eradicate()	- call CS_eradicate()
!
! Description:
!
!	This routine exists to enable us to bootstrap a thread's
!	stack without worrying about the stack size and compiled
!	object code of CS_eradicate().
!
! Inputs:
!      none.
!
! Outputs:
!      none.
!
! Returns:
!	never
!
! Exceptions:
!	    none
!
! Side Effects:
!	    The thread running this will cease to exist in CS_eradicate()
!
! History:
!      24-jan-95 (nick)
!          Created
!
	.section	".text"
	.global	CS_su4_eradicate
	.type	CS_su4_eradicate,#function

CS_su4_eradicate:
!
! minimum stack - note we should never run this as we only ever return into 
! this function - the stack is allocated in CS_fudge_stack() on thread 
! creation.
!
	save	%sp,-SA(MINFRAME),%sp
!
! CS_eradicate() shouldn't return
!
	call	CS_eradicate
	nop
!
	ret
	restore
!
!{
! Name: CS_su4_setup()	- call CS_setup()
!
! Description:
!
!	This routine exists to enable us to bootstrap a thread's
!	stack without worrying about the stack size and compiled
!	object code of CS_setup().
!
! Inputs:
!      none.
!
! Outputs:
!      none.
!
! Returns:
!	STATUS ( from CS_setup() )
!
! Exceptions:
!	    none
!
! Side Effects:
!	    none
!
! History:
!      24-jan-95 (nick)
!          Created
!
	.section	".text"
	.global	CS_su4_setup
	.type	CS_su4_setup,#function

CS_su4_setup:
!
! minimum stack - note we should never run this as we only ever return into 
! this function - the stack is allocated in CS_fudge_stack() on thread creation.
!
	save	%sp,-SA(MINFRAME),%sp
!
	call	CS_setup
	nop
! 
	mov	%o0,%i0
	ret
	restore

# endif /* su4_us5 */
