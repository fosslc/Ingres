/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BTset -	set a bit in a byte stream
**
** Description:
**		set bit 'pos' in 'vector'
**
** Inputs:
**	pos		bit to set in 'vector'
**	vector		byte stream to set bit in
**
** Outputs:
**	vector		result of setting bit 'pos' in 'vector'
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
**		Rewrote to use tables, cleaned up conditional compilation.
**	15-jan-86 (greg)
**		Use BT_XXX_MACROs.  Conform to coding standards.
**      25-aug-92 (ed)
**          Changed to use prototypes, and include bt.h
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
*/

/*ARGSUSED*/

VOID
BTset(
	i4	pos,
	char	*vector)
{
	vector[ BT_FULLBYTES_MACRO(pos) ] |= bt_bitmsk[ BT_MOD_MACRO(pos) ];
}
