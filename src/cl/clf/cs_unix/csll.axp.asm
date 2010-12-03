/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** DEC AXP Alpha/OSF machine-specific assembly language code.
** 
** History:
**  12-jan-93 (swm)
**	Initial version.
**  10-nov-93 (swm)
**      Bug #58635
**	Altered offsets into CS structs because they have been changed 
**	to be consistent with the "dmf standard header".
**	Added .prologue directives so that dbx can locate stack frames.
**  16-jan-96 (toumi01; from 1.1 axp_osf port) (muhpa01)
**	Changed CS_tas() to loop on unsuccessful attempt to store.
**  01-feb-96 (toumi01)
**	Restore sp before restoreregs() so that we get back the correct
**	context (for the scb returned by the CS_xchng_thread call).
**  13-Nov-1997 (merja01)
**	Fix offset for CS__INKERNEL.  A change to the CS_SYSTEM struct
**	caused this offset to change, which caused the installation to
**	hang during shutdown when using internal threads.  The 
**	cs_inkernel setting was non-zero causing CS_swuser to return
**	without calling CS_xchng_thread.
**  03-Dec-1998 (merja01)
**      Update the CS__INKERNEL offset for Ingres 2.5.
**  12-Feb-2001 (hanje04)
**	Alpha Linux (axp_lnx) will also use this assemble file.
**  17-Nov-2003 (devjo01)
**	Add CScas4, and CScasptr.
**  23-Feb-2009 (hweho01)
**	Fixed offsets for CS__PC, CS__PV and CS__SP. A change to CS_SCB 
**	struct caused these offsets to change, so SEGV occurred in 
**	CS_swuser.
**	12-Nov-2010 (kschendel) SIR 124685
**	    Moved inkernel and current to front of CS_SYSTEM, fix here.
**
*/
#if defined(axp_osf) || defined(axp_lnx)

# ifndef axp_lnx

# include <regdef.h>
# include <asm.h>

# else

#include <alpha/regdef.h>

#endif /* !axp_lnx */
/*
** The C program following can be used to re-determine certain offsets
** into CS structs
*/
#if !defined(axp_osf) && !defined(axp_lnx)
/*
** import CL_OFFSETOF() macro and system types
*/
#include <compat.h>
#include <systypes.h>

/*
** import declaration of PID type (required by other include files below)
*/
#include <pc.h>

/*
** import declaration of CS_SCB (to calculate offsets into it)
*/
#include <cs.h>

/*
** import declaration of CS_SYSTEM (to calculate offsets into it)
*/
#include <csinternal.h>

main()
{
	printf("# define %s %d\n", "CS__CURRENT",
		CL_OFFSETOF(CS_SYSTEM, cs_current));
	printf("# define %s %d\n", "CS__INKERNEL",
    	CL_OFFSETOF(CS_SYSTEM, cs_inkernel));
	printf("# define %s %d\n", "CS__EXSPTR", CL_OFFSETOF(CS_SCB, cs_exsp));
	printf("# define %s %d\n", "CS__PC", CL_OFFSETOF(CS_SCB, cs_pc));
	printf("# define %s %d\n", "CS__PV", CL_OFFSETOF(CS_SCB, cs_pv));
	/*
	printf("# define %s %d\n", "CS__AT", CL_OFFSETOF(CS_SCB, cs_at));
	printf("# define %s %d\n", "CS__GP", CL_OFFSETOF(CS_SCB, cs_gp));
	*/
	printf("# define %s %d\n", "CS__SP", CL_OFFSETOF(CS_SCB, cs_sp));
}
# endif /* !axp_osf */

/*
** the following definitions are offsets into CS struct, their values
** can be re-determined by executing the C program included in the comment
** above
*/
# define CS__CURRENT 20
# define CS__INKERNEL 16
# define CS__EXSPTR 64
# define CS__PC 72
# define CS__PV 88
# define CS__SP 80

/*
** Registers are saved on the stack for context switches between threads.
** Some of these must be duplicated in the SCB also. Stack space requirements
** are defined as follows:
** 
** The special registers saved on the stack are ra, AT, gp, pv: 32 bytes
** The integer registers, s0 to s6:				56 bytes
** The floating point registers, f2 to f9:			64 bytes
**
** The offsets into the stack frame are therefore:
**	special registers					  0 bytes
**	integer registers					 32 bytes
**	floating point registers		32 + 56 =	 88 bytes
**	largest offset required			88 + 64 =	152 bytes
**
** But the frame offset (from sp) must be aligned on a 16 byte boundary, so
**	frame offset size					160 bytes
*/
#define	INT_REG_OFFSET		 32
#define	FP_REG_OFFSET		 88
#define SAVEREG_FRAME_SIZE	160

/*
** masks for the .frame, .mask and .fmask directives (for debugger use)
*/
#define SAVEREG_MASK	M_RA|M_AT|M_GP|M_T12|M_S0|M_S1|M_S2|M_S3|M_S4|M_S5|M_S6
#define SAVEFPREG_MASK	0x3FD

/*
** macro to save necessary registers on the stack prior to a context switch
*/
# define saveregs()					\
	stq	ra, 0(sp);				\
	.set	noat;					\
	stq	AT, 8(sp);				\
	.set	at;					\
	stq	gp, 16(sp);				\
	stq	pv, 24(sp);				\
	stq	s0, INT_REG_OFFSET(sp);			\
	stq	s1, INT_REG_OFFSET+8(sp);		\
	stq	s2, INT_REG_OFFSET+16(sp);		\
	stq	s3, INT_REG_OFFSET+24(sp);		\
	stq	s4, INT_REG_OFFSET+32(sp);		\
	stq	s5, INT_REG_OFFSET+40(sp);		\
	stq	s6, INT_REG_OFFSET+48(sp);		\
	stt	$f2, FP_REG_OFFSET(sp);			\
	stt	$f3, FP_REG_OFFSET+8(sp);		\
	stt	$f4, FP_REG_OFFSET+16(sp);		\
	stt	$f5, FP_REG_OFFSET+24(sp);		\
	stt	$f6, FP_REG_OFFSET+32(sp);		\
	stt	$f7, FP_REG_OFFSET+40(sp);		\
	stt	$f8, FP_REG_OFFSET+48(sp);		\
	stt	$f9, FP_REG_OFFSET+56(sp)

/*
** macro to restore registers saved on the stack prior to a context switch
*/
# define restoreregs()					\
	ldq	ra, 0(sp);				\
	.set	noat;					\
	ldq	AT, 8(sp);				\
	.set	at;					\
	ldq	gp, 16(sp);				\
	ldq	pv, 24(sp);				\
	ldq	s0, INT_REG_OFFSET+0(sp);		\
	ldq	s1, INT_REG_OFFSET+8(sp);		\
	ldq	s2, INT_REG_OFFSET+16(sp);		\
	ldq	s3, INT_REG_OFFSET+24(sp);		\
	ldq	s4, INT_REG_OFFSET+32(sp);		\
	ldq	s5, INT_REG_OFFSET+40(sp);		\
	ldq	s6, INT_REG_OFFSET+48(sp);		\
	ldt	$f2, FP_REG_OFFSET(sp);			\
	ldt	$f3, FP_REG_OFFSET+8(sp);		\
	ldt	$f4, FP_REG_OFFSET+16(sp);		\
	ldt	$f5, FP_REG_OFFSET+24(sp);		\
	ldt	$f6, FP_REG_OFFSET+32(sp);		\
	ldt	$f7, FP_REG_OFFSET+40(sp);		\
	ldt	$f8, FP_REG_OFFSET+48(sp);		\
	ldt	$f9, FP_REG_OFFSET+56(sp)

/*
** void
** CS_swuser(void)
**   22-Apr-97 (merja01)
**    Change assembler directive .prologue from 0 to 1 to prevent 
**    segmentation violation running csphil when built on Digital 
**    Unix 4.0.  With the .prologue set to 0 the gp was getting
**    corrupted.  The following is taken from the Assembly Language
**    Programmer's guide:
**    "A flag of zero indicates that the procedure does not use $gp;
**     the caller does not need to set up $pv prior to calling the 
**	   procedure or restore $gp on return from the procedure. 
**     A flag of one indicates that the procedure does use $gp; the
**     caller must set up $pv prior to calling the procedure and restore
**	   $gp on return from the procedure." 
*/
	.extern Cs_srv_block
	.extern Cs_idle_scb
	.extern ex_sptr
	.text
	.align	4

	.set 	noreorder
	.globl	CS_swuser
	.ent 	CS_swuser

CS_swuser:
	ldgp    gp, 0(pv)

	/*
	** grow stack for saved registers
	*/
	lda	sp, -SAVEREG_FRAME_SIZE(sp)
	stq	ra, 0(sp)

	/*
	** frame, mask and fmask directives for the debugger
	*/
# ifndef axp_lnx

	.frame	sp, SAVEREG_FRAME_SIZE, ra
	.mask	SAVEREG_MASK, 0
	.fmask	SAVEFPREG_MASK, -FP_REG_OFFSET
	.prologue 1
# endif
	/*
	** if (Cs_srv_block.cs_inkernel)
	** 	return;
	*/
	ldl	t0, Cs_srv_block+CS__INKERNEL
	bne	t0, swuser_ret

	/*
	** scb = Cs_srv_block.cs_current;
	*/
	ldq	t1, Cs_srv_block+CS__CURRENT

	/*
	** if (scb != NULL)
	*/
	beq	t1, swuser_exchange

	/*
	** scb->cs_exsp = ex_sptr;
	*/
	ldq	t2, ex_sptr
	stq	t2, CS__EXSPTR(t1)

	/*
	** mb and trapb to force any exceptions outstanding before
	** changing the exception stack (below)
	*/
	mb
	trapb

	/*
	** save registers on the stack
	*/
	saveregs()

	/*
	** save pc (ie. ra), sp and pv in scb
	*/
	stq	ra, CS__PC(t1)
	stq	sp, CS__SP(t1)
	stq	pv, CS__PV(t1)

	/*
	** ex_sptr = Cs_idle_scb.cs_exsp;
	*/
swuser_exchange:
	ldq	t3, Cs_idle_scb+CS__EXSPTR
	stq	t3, ex_sptr

	/*
	** scb = CS_xchng_thread(scb);
	*/
	mov	t1, a0
	jsr	ra, CS_xchng_thread
	ldgp	gp, 0(ra)
	mov	v0, t1

	/*
	** ex_sptr = scb->cs_exsp;
	*/
	ldq	t2, CS__EXSPTR(t1)	
	stq	t2, ex_sptr

	/*
	** restore stack pointer (sp) from scb
	*/
	ldq	sp, CS__SP(t1)

	/*
	** restore registers from the stack
	*/
	restoreregs()

	/*
	** restore pc (ie. ra) and pv from scb
	*/
	ldq	ra, CS__PC(t1)
	ldq	pv, CS__PV(t1)

	/*
	** release stack space for saved registers and return
	*/
swuser_ret:
	lda	sp, SAVEREG_FRAME_SIZE(sp)
	ret
	.end	CS_swuser

/*
** CS_tas(CS_ASET *lock_variable)
**
** Description:
**      Atomic test and set quadword.
**
**      The lock variable contains
**	0	-	not locked
**	1	-	locked
**
**      Atomicity is guaranteed by the stq_c (store quadword conditional)
**      instruction which succeeds only if the lock variable address has
**      not been modified since taking a local CPU lock with the ldq_l
**      instruction. If the lock variable address has been modified
**      elsewhere, the local CPU lock will have been cleared causing the
**      stq_c to fail.
**
**      After a successful stq_c the value of the original lock variable
**      (copied by the ldq_l instruction) is checked to ensure that it had
**	not been set by an ldq_l, stq_c pair elsewhere.
**
**      The final mb (memory barrier) instruction prevents any reads
**      from being pre-fetched before the (INGRES) lock has been acquired.
**
**      Return
**	0	-	lock not acquired
**	1	-	lock acquired
**
**   03-Dec-1998 (merja01)
**     Changed .prologue from 0 to 1 because according to the Digital
**     UNIX Assembly Language Programmer's Guide this flag should
**     always be 1 if the prologue contains a load global pointer
**     instruction (ldgp).
**
*/
        .text
        .align  4

        .set    noreorder
        .globl  CS_tas
        .ent    CS_tas

CS_tas:
        ldgp    gp, 0(pv)
	.frame	sp, 0, ra
	.prologue 1
	clr	v0			/* return value, initially 0 (fail) */
CS_tas_loop:
	ldq_l	t0, 0(a0)		/* t0 = *lock_variable */
	blbs	t0, tas_ret		/* already set, return 0 */
	mov	1,  t0
	stq_c	t0, 0(a0)		/* conditionally, *lock_variable = 1 */
	beq	t0, tas_fail		/* t0 == 0 means unsuccessful stq_c */
	mov	1, v0			/* return value 1 (success) */
	mb				/* lock acquired, prevent race
					   conditions due to pipelining */
tas_ret:
	ret
tas_fail:
	br	CS_tas_loop
	.end	CS_tas

/*
** CS_aclr(CS_ASET *lock_variable)
**
** Description:
**      Atomic clear of a quadword.
**
**      A pre-condition is that this function is called only by a process
**      holding the specified lock.
**
**      There is no need to use the ldq_l, stq_c pair here, if another
**      process is contending for the lock this atomic clear should have
**      priority.
**
**      The stq instruction here will clear the processor lock flag in
**      another process (if any) contending for the lock.
**
**      The initial mb (memory barrier) instruction prevents any reads
**      prefetching or writes from being delayed past the clearing of the lock.
**
**      Always return 1 to indicate success.
**
**   03-Dec-1998 (merja01)
**     Changed .prologue from 0 to 1 because according to the Digital
**     UNIX Assembly Language Programmer's Guide this flag should
**     always be 1 if the prologue contains a load global pointer
**     instruction (ldgp).
*/
        .text
        .align  4

        .set    noreorder
        .globl  CS_aclr
        .ent    CS_aclr

CS_aclr:
        ldgp    gp, 0(pv)
	.frame	sp, 0, ra
	.prologue 1
	mb				/* lock acquired, prevent race
					   conditions due to pipelining */
	stq	zero, 0(a0)		/* *lock_variable = 0 (atomic) */
	mov	1, v0			/* return value 1 (success) */
	ret
	.end	CS_aclr


/*
** CScas4 -- Perform an Atomic Compare & Swap for four bytes.
**
** Description:
**	This routine performs an atomic compare and swap of an
**	aligned four byte value.   Logically this behaves like the
**	following 'C' code, but special instructions are used to
**	assure that 'newval' is not written if the contents of
**	the target address have changed.
**
** STATUS CScas4( i4 *addr, i4 oldval, i4 newval )
** {
**     if ( *addr == oldval )
**     {
** 	  *addr = newval;
**	  return OK;
**     }
**     *oldval = *addr;
**     return FAIL;
** }
**
**	This is used as follows:
**
**
** do 
** {
**      // dirty read oldval
**      oldval = *addr;
**	// Calculate new value based on old val 
**	newval = ...;
**
** } while ( CScas4( addr, oldval, newval ) );
**
** // continue processing knowing that newval was written
** // before oldval changed, and that all other writers using
** // compare and swap will see your update.
*/

	.set noat
	.set noreorder
	.text
	.arch	generic
	.align 4

	.globl  CScas4
	.ent 	CScas4

CScas4:

	.frame  $sp, 0, $26
	.prologue 0

	ldl	$2, ($16)	# Get current value of '*addr'.  

	xor	$2, $17, $0	# Quick check for stale 'oldval'.
	bne	$0, 1f		# Alas, 'oldval' is obsolete.

	mb			# Make sure all in-flight updates land.
				# Not 100% sure this is needed.

	ldl_l	$2, ($16)	# "lock" 'addr'.     
	xor	$2, $17, $0	# Check "locked" value against 'oldval'.
	bne	$0, 1f		# Someone got in after "quick" check

	stl_c	$18, ($16)	# Conditionally store 'newval' into address.
	bne	$18, 2f		# 'newval' has lock flag after store.  If 0,
				# then store failed.
1:
	mov	1, $0
2:
	ret	($26)

	.end 	CScas4


/*
** CScasptr  -- Perform an Atomic Compare & Swap on a pointer.
**
** Description:
**	This routine performs an atomic compare and swap of an
**	aligned EIGHT byte value.   Logically this behaves like the
**	following 'C' code, but special instructions are used to
**	assure that 'newval' is not written if the contents of
**	the target address have changed.
**
** STATUS CScasptr( PTR *addr,
**		     PTR oldval,
**                   PTR newval )
** {
**     if ( *addr == *oldval )
**     {
** 	  *addr = newval;
**	  return OK;
**     }
**     *oldval = *addr;
**     return FAIL;
** }
**
**	This is used as follows:
**
** do 
** {
**      // dirty read oldval
**      oldval = *addr;
**	// Calculate new value based on old val 
**	newval = ...;
**
** } while ( CScasptr( addr, oldval, newval ) );
**
** // continue processing knowing that newval was written
** // before oldval changed, and that all other writers using
** // compare and swap will see your update.
*/

	.set noat
	.set noreorder
	.text
	.arch	generic
	.align 4

	.globl  CScasptr
	.ent	CScasptr

CScasptr:

	.frame  $sp, 0, $26
	.prologue 0

	ldq	$2, ($16)	# Get current value of '*addr'.  

	xor	$2, $17, $0	# Quick check for stale 'oldval'.
	bne	$0, 1f		# Alas, 'oldval' is obsolete.

    	mb			# Make sure all in-flight updates land.
				# Not 100% sure this is needed.

	ldq_l	$2, ($16)	# "lock" 'addr'.     
	xor	$2, $17, $0	# Check "locked" value against 'oldval'.
	bne	$0, 1f		# Someone got in after "quick" check

	stq_c	$18, ($16)	# Conditionally store 'newval' into address.
	bne	$18, 2f		# 'newval' has lock flag after store.  If 0,
				# then store failed.
1:
	mov	1, $0		# Indicate failure.
2:
	ret	($26)

	.end 	CScasptr

# endif /* axp_osf */
