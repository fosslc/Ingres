/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Intel Itanium ia64 machine-specific assembly language code.
** History:
**	23-jul-2001 (toumi01)
**	    Created.
**	15-mar-2004 (somsa01)
**	    Added ";;" to the end of mov and cmpxchg lines in
**	    CS_tas() to prevent function from not properly setting variable.
**	    This creates a "defined stop", which forbids instruction pipeline
**	    reordering across those syntax elements.
*/

.text
.globl	CS_swuser
.globl	CS_tas
.globl	CS_aclr

/*
 * dummy routine - no ingres threading support yet !!
 */
CS_swuser:
	br.ret.sptk.clr	b0

/* Atomic Test and Set - based on Intel OS sample code
 * Input:
 *	memory address (first argument)
 * Return:
 *	if (value == 0) {
 *		value = 1;
 *		return 0;
 *	} else {
 *		return value;
 *	}
 * NOTE: this return value has the opposite value of the standard Ingres
 * implementation of CS_tas, and so is flipped by the definition of the
 * CS_TAS and CS_getspin macros in csnormal.h
 */ 

CS_tas:
	mov		ar.ccv = r0
	mov		r29 = 1;;
	ld8		r3 = [r32]
	cmpxchg8.acq	r8 = [r32], r29, ar.ccv;;
	br.ret.sptk.clr	b0

CS_aclr:
	st8.rel		[r32] = r0
	br.ret.sptk.clr	b0
