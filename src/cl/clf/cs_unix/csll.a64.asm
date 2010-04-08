/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** AMD x86-64 machine-specific assembly language code.
** History:
**	31-Oct-2002 (hanje04)
**	    Created. Just a dummy routine for CS_swuser
**	    for CS_tas etc we use system __inline__ calls in
**	    /usr/include/asm/bitops.h
**	16-Jun-2006 (kschendel)
**	    Add compare-and-swap routines, same as a64sol.  a64_lnx can now
**	    be a CAS_ENABLED platform.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/

.text
.globl	CS_swuser
.globl	CScas4
.globl	CScasptr

/*
 * dummy routine - no ingres threading support yet !!
 */
CS_swuser:
	retq	


/* Compare-and-swap routines.
**
** CScas4(i4 *addr, i4 oldval, i4 newval)
** compares *addr and oldval, and if they are equal, sets *addr = newval
** and returns OK (zero).  The comparison and update is done atomically.
** If *addr <> oldval, FAIL (nonzero) is returned.
**
** CScasptr(PTR *addr, PTR oldval, PTR newval)
** does the same thing for pointer-sized things.
**
*/

#ifdef BUILD_ARCH32

/* The 32-bit versions.
** Standard x86 call sequence.
*/
CScas4:
CScasptr:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	pushl	%esi
	movl	8(%ebp), %ebx		/* addr */
	movl	0xc(%ebp), %eax		/* oldval */
	movl	0x10(%ebp), %esi	/* newval */
	lock
	cmpxchgl %esi, (%ebx)		/* Sets ZF if *addr = newval */
	movl	$0, %eax		/* Clear return value */
	setnz	%al			/* Return !ZF */
	popl	%esi
	popl	%ebx
	movl	%ebp, %esp
	popl	%ebp
	ret

#else

/* The 64-bit versions.
** %rdi = addr, %esi = oldval, %edx = newval
** For CScasptr, use %rsi and %rdx (full 64-bit value)
*/
CScas4:
	movl	%esi, %eax
	lock
	cmpxchgl %edx, (%rdi)	/* Sets ZF if *addr = newval */
	movl	$0, %eax	/* Clear return value */
	setnz	%al		/* Return !ZF */
	ret

CScasptr:
	movq	%rsi, %rax
	lock
	cmpxchgq %rdx, (%rdi)	/* Sets ZF if *addr = newval */
	movl	$0, %eax	/* Clear return value */
	setnz	%al		/* Return !ZF */
	ret

#endif
