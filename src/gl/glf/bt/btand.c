/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include       <bt.h>
# include	"bttab.h"

/*{
** Name:	BTand -- 'and' two byte streams
**
** Description:
**		Perform C 'and' operation (&) on byte streams
**
** Inputs:
**	size		number of bits in byte streams to 'and'.
**			Rounded up to BITSPERBYTE.
**	vector1		input vector to 'and' with vector2
**	vector2		'and'ed with vector1
**
** Outputs:
**	vector2		result of 'and'ing vector1 and vector2
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
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      21-apr-95 (tutto01)
**          Take advantage of 32 bit architectures.
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
**	5-Aug-2004 (schka24)
**	    Update special-case unrolling; add i8-at-a-time on LP64;
**	    help compiler out with unsigned, --i loops.
*/

/*ARGSUSED*/

VOID
BTand(
	i4	size,
	char	*vector1,
	char	*vector2)

        /*
        ** use i4's or i8's to grab 32 or 64 bits at a time.
	** Common cases of "size" are 126/128 and 1024.
	** 126 is # of range variable bits in OPF bitmaps.
	** 1024 is # of column bits in domain bitmaps.
	** Note that many machines like --i loops better than i-- loops...
        */

{

        register char   *b1;
        register char   *b2;
        register u_i4	bytes = BT_NUMBYTES_MACRO(size);
 
#ifdef LP64
        /*
        ** test for long alignment, so that we may work with 64 bit values
        */

        if (bytes >= sizeof(u_i8)
	  && ((SCALARP)vector1 & 7) == 0 && ((SCALARP)vector2 & 7) == 0 )
        {
	    register u_i8 *l1 = (u_i8 *)vector1;
	    register u_i8 *l2 = (u_i8 *)vector2;

	    /* Ok, vectors are long-aligned.
	    ** Useful size special case is 16 bytes == 2 longs.
	    ** The 1024 common-case is 16 longs, maybe a little
	    ** too much for unrolling.  (** Untested assumption!)
	    */

	    if (bytes == 2 * sizeof(u_i8))
	    {
		*l2++ &= *l1++;
		*l2 &= *l1;
		return;
	    }
	    else
	    {
		/* At least one long, do long and leftovers */
		register u_i4 lnum;    /*  Must 'and' 'lnum' longs  */
 
		lnum = bytes / sizeof(u_i8);	/* Nonzero */
		bytes &= sizeof(u_i8)-1;	/* Bytes left over */
 
		do {
		    *l2++ &= *l1++;
		} while (--lnum != 0);
 
		if (bytes != 0)
		{
		    b1 = (char*)l1;   /* And this takes care of the bytes */
		    b2 = (char*)l2;
 
		    do {
			*b2++ &= *b1++;
		    } while (--bytes != 0);
		}
		return;
	    }
 
 
	} /* end of long-aligned cases */
        else
#endif /* LP64 */
 
        /*
        ** test for word alignment, so that we may work with 32 bit values
        */

        if (bytes >= sizeof(u_i4)
	  && ((SCALARP)vector1 & 3) == 0 && ((SCALARP)vector2 & 3) == 0 )
        {
	    register u_i4 *w1 = (u_i4 *)vector1;
	    register u_i4 *w2 = (u_i4 *)vector2;

	    /* Ok, vectors are i4-aligned.
	    ** Useful size special case is 16 bytes == 4 words.
	    ** The 1024 common-case is 32 words, probably a little
	    ** too much for unrolling.
	    */

	    if (bytes == 4 * sizeof(u_i4))
	    {
		*w2++ &= *w1++;
		*w2++ &= *w1++;
		*w2++ &= *w1++;
		*w2 &= *w1;
		return;
	    }
	    else
	    {
		/* At least one word, do words and leftovers */
		register u_i4 wnum;    /*  Must 'and' 'wnum' words  */
 
		wnum = bytes / sizeof(u_i4);	/* Nonzero */
		bytes &= sizeof(u_i4)-1;	/* Bytes left over */
 
		do {
		    *w2++ &= *w1++;
		} while (--wnum != 0);
 
		if (bytes != 0)
		{
		    b1 = (char*)w1;   /* And this takes care of the bytes */
		    b2 = (char*)w2;
 
		    do {
			*b2++ &= *b1++;
		    } while (--bytes != 0);
		}
		return;
	    }
 	} /* word-aligned pointers */

	/*
	**  No alignment, or short size, so degrade to simply a byte at a time.
	**  this is not expected to happen, but if it does, this code is the
	**  catch-all that will take care of it.  Dealing with unaligned
	**  addresses would have unnecessarily complicated the code.
	*/

	b1 = vector1;
	b2 = vector2;
	if (bytes != 0)
	    do {
		*b2++ &= *b1++;
	    } while (--bytes != 0);
	return;
} 
