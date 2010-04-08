/*
**Copyright (c) 2008 Datallegro, assignable to Ingres Corporation
*/

# include       <compat.h>
# include       <gl.h>
# include       <bt.h>
# include       "bttab.h"

/*{
** Name:        BTclearmask -- use a mask on a bit stream
**
** Description:
**      Perform clear mask operation on byte streams.
**      The C equivalent would be vec &= ~mask.
**      This is a "turn off corresponding bits" operation, or for
**      oldsters, the equivalent of the Dec-10 ANDCAM instruction,
**      or the PDP-11 BIC.
**
** Inputs:
**      size            number of bits in byte streams to 'nand'.
**                      Rounded up to BITSPERBYTE.
**      mask            input vector to not-and with vector
**      vector          'and'ed with complement of mask
**
** Outputs:
**      vector          result of 'and'ing ~mask and vector
**
**      Returns:
**              None
**      Exceptions:
**              None
**
** Side Effects:
**              None
**
** History:
**      9-Jan-2008 (kschendel)
**	    Added logical bitclear , since it is effectively already being
**	    used, but as a multi-step process.
*/

/*ARGSUSED*/

VOID
BTclearmask(
        i4      size,
        char    *mask,
        char    *vector)

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
        register u_i4   bytes = BT_NUMBYTES_MACRO(size);
 
#ifdef LP64
        /*
        ** test for long alignment, so that we may work with 64 bit values
        */

        if (bytes >= sizeof(u_i8)
          && ((SCALARP)mask & 7) == 0 && ((SCALARP)vector & 7) == 0 )
        {
            register u_i8 *l1 = (u_i8 *)mask;
            register u_i8 *l2 = (u_i8 *)vector;

            /* Ok, vectors are long-aligned.
            ** Useful size special case is 16 bytes == 2 longs.
            ** The 1024 common-case is 16 longs, maybe a little
            ** too much for unrolling.  (** Untested assumption!)
            */

            if (bytes == 2 * sizeof(u_i8))
            {
                *l2++ &= ~(*l1++);
                *l2 &= ~(*l1);
                return;
            }
            else
            {
                /* At least one long, do long and leftovers */
                register u_i4 lnum;    /*  Must 'nand' 'lnum' longs  */
 
                lnum = bytes / sizeof(u_i8);    /* Nonzero */
                bytes &= sizeof(u_i8)-1;        /* Bytes left over */
 
                do {
                    *l2++ &= ~(*l1++);
                } while (--lnum != 0);
 
                if (bytes != 0)
                {
                    b1 = (char*)l1;   /* And this takes care of the bytes */
                    b2 = (char*)l2;
 
                    do {
                        *b2++ &= ~(*b1++);
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
          && ((SCALARP)mask & 3) == 0 && ((SCALARP)vector & 3) == 0 )
        {
            register u_i4 *w1 = (u_i4 *)mask;
            register u_i4 *w2 = (u_i4 *)vector;

            /* Ok, vectors are i4-aligned.
            ** Useful size special case is 16 bytes == 4 words.
            ** The 1024 common-case is 32 words, probably a little
            ** too much for unrolling.
            */

            if (bytes == 4 * sizeof(u_i4))
            {
                *w2++ &= ~(*w1++);
                *w2++ &= ~(*w1++);
                *w2++ &= ~(*w1++);
                *w2 &= ~(*w1);
                return;
            }
            else
            {
                /* At least one word, do words and leftovers */
                register u_i4 wnum;    /*  Must 'nand' 'wnum' words  */
 
                wnum = bytes / sizeof(u_i4);    /* Nonzero */
                bytes &= sizeof(u_i4)-1;        /* Bytes left over */
 
                do {
                    *w2++ &= ~(*w1++);
                } while (--wnum != 0);
 
                if (bytes != 0)
                {
                    b1 = (char*)w1;   /* And this takes care of the bytes */
                    b2 = (char*)w2;
 
                    do {
                        *b2++ &= ~(*b1++);
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

        b1 = mask;
        b2 = vector;
        if (bytes != 0)
            do {
                *b2++ &= ~(*b1++);
            } while (--bytes != 0);
        return;
} 

