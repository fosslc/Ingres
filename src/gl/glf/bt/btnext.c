/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	<si.h>
# include	"bttab.h"

/*{
** Name:	BTnext -- find next set bit in a byte stream
**
** Description:
**		return position of next set bit in a byte stream
**
** Inputs:
**	prev_bit	start looking for set bits after this bit
**	vector		byte stream to search
**	size		number of bits in byte stream, limits search
**
** Outputs:
**	Returns:
**		position of next bit set after postion 'prev_bit'
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
**		Use BT_HIBYTE_MACRO to avoid continuing one byte too far.
**		One exit point.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      4-oct-92 (ed)
**          add prototypes, and move bt.h to GLHDR
**	15-feb-93 (ed)
**	    do not return bit number larger than specified by caller
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
*/

/*ARGSUSED*/

i4
BTnext(
	i4	prev_bit,		/* one before place to start searching */
	char	*vector,
	i4	size)
{
	register u_char *last;		/* last byte to examine */
	register u_char	*v;		/* byte to examine */
	register i4	pos;
		 i4	start   = prev_bit + 1;
		 i4	ret_val = -1;

	/*
	** Check remaining high bits of the first byte.
	**
	**	bt_highmsk[ ]	mask to eliminate bits in the first byte
	**	bt_ffstab[ ]	if any bits set, the index of the first,
	**			  	otherwise a value > BITSPERBYTE - 1.
	*/

	v   = (u_char *) vector + BT_FULLBYTES_MACRO(start);
	pos = bt_ffstab[ *v & bt_highmsk[ BT_MOD_MACRO(start) ] ];

	if (pos <= BITSPERBYTE - 1)
	{
		ret_val = BT_BIT0_MACRO(start) + pos;
	}
	else
	{
		/* check remaining bytes--must look at ``partial'' bytes */

		last = (u_char *)vector + BT_HIBYTE_MACRO(size);

		for ( ; ++v <= last ; )
		{
			if ( *v )
			{
				ret_val = (BT_NUMBITS_MACRO(v -
							    (u_char *)vector) +
					   bt_ffstab[ *v ] );
				break;
			}
		}
	}
	if (ret_val >= size)
	    ret_val = -1;	    /* do not return bit number larger than
				    ** max specified by caller */
	return ( ret_val );
}
