/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"


/*{
** Name: BTNOT	- Perform a bit-wise 'not' operation on a given bit vector.
**
** Description:
**      BTnot() performs a bit-wise logical not operation on 'vector' of size 
**	'size' bits. Each bit that was one before the BTnot() call will be zero 
**      afterward; each bit that was zero will be one. It is assumed that the
**	memory allocated to 'vector' is a multiple of BITS_PER_BYTE bits in
**	size.
**
** Inputs:
**	size -
**	    'size' is the size of 'vector' in bits.
**	vector -
**	    'vector' is a pointer to a array of bits.
**
** Outputs:
**	vector -
**	    Each bit in the array is inverted. Ie. each bit that was one is now
**	    zero; each bit that was zero is now one.
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-may-86 (eric)
**          defined
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	06-jan-88 (mmm)
**	    Fixed bug shown up by opf usage of BTnot.  Opf calls BTnot with
**	    number of bits that is not evenly divisible into a byte.  This
**	    change insures that the bits left over are set to 0 so that the
**	    other BT routines will work properly on the bit stream.
**      4-oct-92 (ed)
**          add prototypes, and move bt.h to GLHDR
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
*/
VOID
BTnot(
	i4	size,
	char	*vector)
{
	register u_char	*ptr	= (u_char *)vector;
	register i4	bytes  	= BT_NUMBYTES_MACRO(size);
	register i4     leftover_bits = size % BITSPERBYTE;
	
	/*
	** NOTE: BT_NUMBYTES_MACRO rounds up to insure we
	**	 we look at the byte containing bit `size'
	*/

	for ( ; bytes-- ; ptr++)
		*ptr = ~(*ptr);
	
	/* clear bits not used in the last byte */
	if (leftover_bits)
	{
	    --ptr;
	    *ptr &= (1 << leftover_bits) - 1;
	}
}
