/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BTxor -- 'xor' two byte streams
**
** Description:
**		Perform C 'xor' operation (^) on byte streams
**
** Inputs:
**	size		number of bits of byte streams to 'or'
**	vector1,
**	vector2		byte streams to 'xor'
**
** Outputs:
**	vector2		result of 'xor'ing vector1 and vector2
**
**	Returns:
**		None
**	Exceptions:
**		None
**
** Side Effects:
**		None
**
** History:
**	29-dec-85 (daveb)
**		Cleaned up conditional compilation, 
**		removed size check form C not present in asm.
**	15-jan-86 (greg)
**		Use BT_XXX_MACROs and substitute `--' for `=- BITSPERBYTE'.
**		Conform to coding standards.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
**	6-jan-92 (seputis)
**	    created from btor.c
**      25-aug-92 (ed)
**          Changed to use prototypes, and include bt.h
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
*/

/* ARGSUSED */

VOID
BTxor (
	i4		size,
	char		*vector1,
	char		*vector2)
{
	register char	*m	= vector1;
	register char	*v	= vector2;
	register i4	bytes	= BT_NUMBYTES_MACRO(size);


	for ( ; bytes-- ; )
		*v++ ^= *m++;

	return;
}
