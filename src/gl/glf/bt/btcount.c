/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include       <bt.h>
# include	"bttab.h"

/*{
** Name:	BTcount - count set bits in the byte stream
**
** Description:
**		count set bits in the byte stream
**
** Inputs:
**	vector		byte stream to count set bits of
**	size		number of bits in byte stream to check
**
** Outputs:
**	Returns:
**		Number of bits set in first 'size' bits of 'vector'
**	Exceptions:
**		None
**
** Side Effects:
**		None
**
** History:
**	29-dec-85 (daveb)
**		Rewrote using tables, cleaned up conditional 
**		compilation flags.  It's not clear to me that the 
**		VAX asm code is faster than the new C.  It probably
**		isn't on a micro-Vax.
**	15-jan-86 (greg)
**		Use BT_XXX_MACROs.  Conform to coding standards.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
**	4-oct-92 (ed)
**	    add prototypes, and move bt.h to GLHDR
**	20-oct-92 (andre)
**	    rewrote to count number of bits in the high-order byte using 
**	    newly defined bt_lowmsk[]; this ensures that we count set bits
**	    among the first n bits as opposed to counting them in the first
**	    BT_NUMBYTES_MACRO(n) bYtes as we have been doing until now
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
*/

/*ARGSUSED*/

i4
BTcount(
    char	*vector,
    i4		size)
{
	register u_char		*byte = (u_char *)vector;
	register i4		count = 0;
	register i4		i = BT_FULLBYTES_MACRO(size);


	for ( ; i-- ; )
		count += bt_cnttab[ *byte++ ];

	if (BT_MOD_MACRO(size))
		count += bt_cnttab[ *byte & bt_lowmsk[BT_MOD_MACRO(size)]];

	return (count);
}
