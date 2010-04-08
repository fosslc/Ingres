/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BThigh - find last (MSB) set bit in a byte stream
**
** Description:
**		Return position of Most Significant Bit set in byte stream
**
** Inputs:
**	vector		byte stream to search for MSB set
**	size		number of bits in byte stream to search
**
** Outputs:
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
**		Rewrote using tables
**	15-jan-86 (greg)
**		Use BT_XXX_MACROs.  Conform to coding standards.
**		Use BT_HIBYTE_MACRO to avoid starting one byte too high.
**		One exit point.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      4-oct-92 (ed)
**          add prototypes, and move bt.h to GLHDR
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
*/

i4
BThigh(
	char	*vector,
	i4	size)
{
	register unsigned char	*v	= (unsigned char *)vector;
	register unsigned char	*byte	= v + BT_HIBYTE_MACRO(size);
	register i4		ret_val	= -1;


	for ( ; byte >= v ; --byte )
	{
		if ( *byte )
		{
			ret_val = (BT_NUMBITS_MACRO(byte - v) +
				  bt_flstab[ *byte ]);
			break;
		}
	}

	return ( ret_val );
}
