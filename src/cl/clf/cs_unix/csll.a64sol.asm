/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** AMD x86-64 machine-specific assembly language code.
** History:
**	10-Dec-2004 (bonro01)
**	    Created. Just a dummy routine for CS_swuser
**	31-May-2005 (schka24)
**	    Add compare-and-swap routines.
**	20-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/

.text
.globl	CS_swuser
.globl   CS_tas
.globl	CScas4
.globl	CScasptr

/*
 * dummy routine - no ingres threading support yet !!
 */
CS_swuser:
	ret


#ifdef BUILD_ARCH64

CS_tas:
        movl    $0,%eax         /* Clear return value */
        movb    $1, %al
        xchgb    %al, (%rdi)
        xorb    $1,%al
endtas:
        ret

#else

CS_tas:
        movl    $0,%eax         /* Clear return value */
        movb    $1, %al
        movl    4(%esp), %ecx
        xchgb   %al, (%ecx)
        xorb    $1,%al
endtas:
        ret

#endif



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

#ifdef BUILD_ARCH64
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

#else

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
#endif
