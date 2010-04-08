/*
** Copyright (c) 2004 Ingres Corporation
*/

# ifndef SHMLOCK_INCLUDED
# define SHMLOCK_INCLUDED

/**
** Name: SHMLOCK.H - Shared memory locking mechanisms
**
** Description:
**	Contains definitions for use of shared memory locks, in particular
**	spinlocks.  Spinlocks require hardware support for atomic test-and-set
**	and clear operations.  These locks are intended for use by the internal
**	CL routines of CS and its clients for situations where a low-overhead
**	synchronization mechanism is required.  Spinlocks should only be held
**	for short periods of time.  This implies that a client should never
**	suspend or block while holding a spinlock.  The lock functions are
**	very machine dependent and may be implemented as C macros, inline
**	assembly language, or external assembly language.
**	
**	    SHM_ATOMIC - Typedef for atomic data items.   
**	    SHM_ASET() - Sets atomic data item.
**	    SHM_ISSET() - Tests if atomic data item is set.
**	    SHM_ACLR() - Clears atomic data item.
**	    SHM_TAS() - Test-and-set atomic data item.
**	    SHM_SPIN - Typedef for spinlock.
**	    SHM_SPININIT() - Initializes a spinlock for use.
**	    SHM_GETSPIN() - Acquire a spinlock.
**	    SHM_TESTSPIN() - Attempt to acquire a spinlock.
**	    SHM_ISLOCKED() - Tests if a spinlock is locked.
**	    SHM_RELSPIN() - Release a spinlock.
**
**	The following defines are intended to be used for MCT (Multiple
**	Concurrent Threads) locking.  They are conditionally defined to
**	their SHM equivalent for MCT implemetations or null for non MCT
**	implementations.  This allows the locking overhead to be eliminated
**	from implementations that don't support MCT.
**
**	    MCT_ATOMIC - Typedef for atomic data items.   
**	    MCT_ASET() - Sets atomic data item.
**	    MCT_ISSET() - Tests if atomic data item is set.
**	    MCT_ACLR() - Clears atomic data item.
**	    MCT_TAS() - Test-and-set atomic data item.
**	    MCT_SPIN - Typedef for spinlock.
**	    MCT_SPININIT() - Initializes a spinlock for use.
**	    MCT_GETSPIN() - Acquire a spinlock.
**	    MCT_TESTSPIN() - Attempt to acquire a spinlock.
**	    MCT_ISLOCKED() - Tests if a spinlock is locked.
**	    MCT_RELSPIN() - Release a spinlock.
**
** History:
**	06-jun-90 (fls-sequent)
**	    Created from the various CS lock related definitions.
**	10-may-1999 (walro03)
**	    Remove obsolete version string sqs_us5.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**/


#if defined(sparc_sol)
# include	<systypes.h>
# include	<thread.h>
#endif

/*
** SHM_ATOMIC - Atomic data item definition
*/

# ifdef sqs_ptx
#define gotatomic
typedef  volatile char		SHM_ATOMIC;
# define MCT_ATOMIC		SHM_ATOMIC
#endif /* sqs_ptx */

# if defined(sparc_sol)
# define gotatomic
typedef long                    SHM_ATOMIC;        /* atomicly setable item */
# define MCT_ATOMIC		SHM_ATOMIC
#endif

#ifndef gotatomic
typedef  char			SHM_ATOMIC;
# define MCT_ATOMIC		SHM_ATOMIC
# endif /* ! gotatomic */

#define	L_UNLOCKED	0
#define	L_LOCKED	1

/*
** SHM_ASET - Sets an atomic data item
*/
# if defined(sqs_ptx)
#define gotshmaset
asm void SHM_ASET(laddr) 
{
/*
** Note: On Symmetry processors we must use an atomic set to ensure
**       cache coherrency.
*/
%reg laddr;
	movb	$L_LOCKED, %al
	xchgb	%al, (laddr)		/* set lock, "atomically" */
%mem laddr;
	movb	$L_LOCKED, %al
	movl	laddr, %ecx
	xchgb	%al, (%ecx)		/* set lock, "atomically" */
}

# define MCT_ASET(a)		SHM_ASET(a)
#endif /* sqs_ptx */

# if defined(sparc_sol)
# define gotshmaset
# define SHM_ASET(a)		CS_tas(a)
# define gotmctisset
# define MCT_ISSET(a)           (*(a) != L_UNLOCKED)
# define gotshmaclr
# define MCT_ACLR(a)		SHM_ACLR(a)
# define SHM_ACLR(a)            (CS_relspin(a))
# define gotshmtas
# define MCT_TAS(a)             (CS_tas(a))
# define SHM_TAS(a)             MCT_TAS(a)
# define gotgetspin
/*# define SHM_GETSPIN(s)		CS_getspin(s) */
# define SHM_GETSPIN(s)		mutex_lock(s)
# define MCT_GETSPIN(s)		SHM_GETSPIN(s)
# define gotmctspin
/*# define MCT_SPININIT(s)	SHM_SPININIT(s)*/
# define MCT_SPININIT(s)	mutex_init(s, 0, NULL)
# define gottestspin
/*# define MCT_TESTSPIN(s)	SHM_TESTSPIN(s)*/
# define MCT_TESTSPIN(s)	(!mutex_trylock(s))
# define gotislocked
/*# define MCT_ISLOCKED(s)	SHM_ISLOCKED(s)*/
# define gotrelspin
# define MCT_RELSPIN(s)		SHM_RELSPIN(s)
/*# define	SHM_RELSPIN(s)		(SHM_ACLR(&(s)->sp_bit))*/
# define SHM_RELSPIN(s)		mutex_unlock(s)
# endif /* sparc */

# ifndef gotshmaset
# define SHM_ASET(a)		(*(a) = L_LOCKED)
# define MCT_ASET(a)
# endif

/*
** SHM_ISSET - Tests if atomic data item is set
*/
# define SHM_ISSET(a)		(*(a) != L_UNLOCKED)

# if defined(sqs_ptx)
# define gotmctisset
# define MCT_ISSET(a)		SHM_ISSET(a)
# endif /* sqs_ptx */

# ifndef gotmctisset
# define MCT_ISSET(a)		0
# endif

/*
** SHM_ACLR - Clears an atomic data item
*/
# if defined(sqs_ptx)
# define gotshmaclr
asm void SHM_ACLR(laddr) 
{
/*
** Note: On Symmetry processors we must use an atomic clear to ensure
**       cache coherrency.
*/
%reg laddr;
	movb	$L_UNLOCKED, %al
	xchgb	%al, (laddr)		/* clear lock, "atomically" */
%mem laddr;
	movb	$L_UNLOCKED, %al
	movl	laddr, %ecx
	xchgb	%al, (%ecx)		/* clear lock, "atomically" */
}

# define MCT_ACLR(a)		SHM_ACLR(a)
# endif /* sqs_ptx */

# ifndef gotshmaclr
# define SHM_ACLR(a)		(*(a) = L_UNLOCKED)
# define MCT_ACLR(a)
# endif

/*
** SHM_TAS - Test-and-set an atomic data item
*/
# if defined(sqs_ptx)
# define gotshmtas
asm int SHM_TAS(laddr)
{
%mem laddr;
/PEEPOFF
/*  Set lock set code */ 
	movl	$0,%eax		/* Clear return value */
	movb 	$L_LOCKED, %al
/* load the address of lock into register */
	movl	laddr, %ecx  
/* Atomically swap current value in lock with our set 
 * If we get a 0 back, we set the lock. If we get a 1 back,
 * lock was already set.
 */
	xchgb   %al, (%ecx)	
/* flip result bit to get the correct return code, and return in %eax  */
	xorb	$L_LOCKED,%al
/PEEPON
}

# define MCT_TAS(a)		SHM_TAS(a)
#endif /* sqs_ptx */

# ifndef gotshmtas
/* SHM_tas is in assembler in mtll.s */
# define SHM_TAS(a)		(SHM_tas(a))
# define MCT_TAS(a)		1
# endif

/*}
** Name: SHM_SPIN - Spinlock structure
**
** Description:
**	This structure holds all information assocated with a spin lock
**	Spinlocks are used as a last resort syncronizing method to prevent
**	two processes accessing the same data structure in an improper
**	mannor.  Spinlocks should only be held for VERY short sections
**	of code.
**
**	NOTICE: This structure must match the assembly code in mtll.s
**
*/
#if defined(sparc_sol)
#define gotshmspin
#define SHM_SPIN mutex_t
#endif

#ifndef gotshmspin
typedef struct _SHM_SPIN
{
    SHM_ATOMIC	sp_bit;		/* bit to spin on */
    u_i2	sp_collide;	/* collision counter */
} SHM_SPIN;
#endif

# define MCT_SPIN		SHM_SPIN

# define SHM_COLLIDE	1    /* offsets into SHM_SPIN structure */

/*
** SHM_SPININIT() - Initializes a spinlock for use
*/
# define SHM_SPININIT(s)	(MEfill(sizeof(*s),'\0',(char *)s))

# if defined(sqs_ptx)
# define gotmctspin
# define MCT_SPININIT(s)	SHM_SPININIT(s)
# endif /* sqs_ptx */

# ifndef gotmctspin
# define MCT_SPININIT(s)
# endif

/*
** SHM_GETSPIN() - Acquire a spinlock
*/
# if defined(sqs_ptx)
# define gotgetspin
asm void SHM_GETSPIN(laddr)
{
%mem laddr; lab spinit, spin, endspin;
/PEEPOFF
	movl	laddr, %ecx  		/* get pointer to SHM_SPIN */
	movl	%ecx, %edx  		/* get pointer to SHM_COLLIDE */
	addl	$SHM_COLLIDE, %edx 	/* equals lock location plus offset */
spinit:
	movb 	$L_LOCKED, %al	 	/* Set lock code. */ 
	xchgb   %al, (%ecx)		/* Atomically swap lower byte of %eax */
				        /* and lock */
/* If we swapped UNLOCKED we got the lock.  If we have LOCKED, lock was 
   already set so try again */
	cmpb	$L_UNLOCKED, %al
	je	endspin 
	incw	(%edx)	 		/* Increment spinlock counter */
spin:					/* Spin in cache until unlocked */
	cmpb	$L_UNLOCKED, (%ecx)	/* Reads don't flood the bus */
	je	spinit 			/* Then try and get lock again */
	jmp	spin
endspin:
/PEEPON
}

# define MCT_GETSPIN(s)		SHM_GETSPIN(s)
# endif /* sqs_ptx */

# ifndef gotgetspin
/* SHM_getspin is in assembler in mtll.s */
# define SHM_GETSPIN(s)		(SHM_getspin(s))
# define MCT_GETSPIN(s)
# endif

/*
** SHM_TESTSPIN() - Attempt to acquire a spinlock
**		    Returns 1 if lock acquired, 0 if lock currently held.
*/
# define SHM_TESTSPIN(s)	(SHM_TAS(&(s)->sp_bit))

# if defined(sqs_ptx)
# define gottestspin
# define MCT_TESTSPIN(s)	SHM_TESTSPIN(s)
# endif /* sqs_ptx */

# ifndef gottestspin
# define MCT_TESTSPIN(s)	1
# endif

/*
** SHM_ISLOCKED() - Test if spinlock is locked.
**		    Returns 1 if locked, 0 if unlocked.
*/
# define SHM_ISLOCKED(s)	(SHM_ISSET(&(s)->sp_bit))

# if defined(sqs_ptx)
# define gotislocked
# define MCT_ISLOCKED(s)	SHM_ISLOCKED(s)
# endif /* sqs_ptx */

# ifndef gotislocked
# define MCT_ISLOCKED(s)	0
# endif

/*
** SHM_RELSPIN() - Release a spinlock
*/

# if defined(sqs_ptx)
# define gotrelspin
# define	SHM_RELSPIN(s)		(SHM_ACLR(&(s)->sp_bit))
# define MCT_RELSPIN(s)		SHM_RELSPIN(s)
# endif /* sqs_ptx */

# ifndef gotrelspin
# define MCT_RELSPIN(s)
# endif

# endif

