/*
**Copyright (c) 2004 Ingres Corporation
*/
 
#include    <compat.h>
#include    <gl.h>
#include    <mh.h>
#include    <ex.h>
#include    <me.h>
#include    <iicommon.h>
 
/**
**
**  Name: MHPK.C - MH routines for packed decimal.
**
**  Description:
**      This file contains all of the MH routines necessary to perform 
**      mathematical operations on packed decimal values.
**
**      This file defines the following routines:
**
**          MHpkint() - Take the integer part of a packed decimal.
**          MHpkceil() - Take the ceiling of a packed decimal.
**          MHpkabs() - Take the absolute value of a packed decimal.
**          MHpkneg() - Negate a packed decimal.
**          MHpkadd() - Add two packed decimals.
**          MHpksub() - Subtract two packed decimals.
**          MHpkmul() - Multiply two packed decimals.
**          MHpkdiv() - Divide two packed decimals.
**          MHpkcmp() - Compare two packed decimals.
**
**  and one static routine:
**	    MH0decexc() - Signal a decimal exception
**
**  History:    $Log-for RCS$
**      03-may-88 (thurston)
**          Initial creation.
**	31-may-90 (blaise)
**	    Added include <me.h> to pick up redefinition of MEfill().
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-Aug-2002 (huazh01)
**          Modify the formula for computing the result scale in
**          MHpkdiv().
**	16-jan-2007 (dougi)
**	    Changed 31 digit max to 39 digit and replaced constants by
**	    CL_MAX_DECPREC.
**/
 

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
*/

/*
**  Definition of static variables and forward static functions.
*/

static  VOID	MH0decexc();	    /* exception signaller */

/*
[@static_variable_or_function_definition@]...
*/
 

/*{
** Name: MHpkint() - Take the integer part of a packed decimal.
**
** Description:
**      Takes the integer part of a packed decimal and returns it as another
**      packed decimal with the same precision and scale.
**
** Inputs:
**      pkin                            Pointer to the input decimal value.
**      prec                            Its precision:  1 <= prec <= 39.
**      scale                           Its scale:  0 <= scale <= prec.
**
** Outputs:
**      pkout                           The resulting packed decimal value is
**                                      placed here.  It will have the same
**                                      precision and scale as the input.
**
**      Returns:
**          VOID
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      16-may-88 (jrb)
**          Initial creation and coding.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/
 
VOID
MHpkint(pkin, prec, scale, pkout)
PTR         pkin;
i4          prec;
i4          scale;
PTR         pkout;
{
    u_char      *po;            /* pointer to output */
    i4		sc = scale;	/* scale counter */
 
    MEcopy(pkin, (prec/2 + 1), pkout);
            
    /* make po point to byte containing leftmost digit of fractional part */
    po = (u_char *)pkout + (prec / 2) - (scale / 2);
 
    /* now zero out all nybbles in fractional part */
    while (sc > 0)
        if (sc-- & 1)
            *po &= (u_char)0xf;
        else
            *po++ &= (u_char)0xf0;
}
 

/*{
** Name: MHpkceil() - Take the ceiling of a packed decimal.
**
** Description:
**      Takes the ceiling of a packed decimal and returns it as another
**      packed decimal with the same precision and scale.  The ceiling is
**      the smallest integer not less than the input.
**
** Inputs:
**      pkin                            Pointer to the input decimal value.
**      prec                            Its precision:  1 <= prec <= 39.
**      scale                           Its scale:  0 <= scale <= prec.
**
** Outputs:
**      pkout                           The resulting packed decimal value is
**                                      placed here.  It will have the same
**                                      precision and scale as the input.
**
**      Returns:
**          VOID
**
**      Exceptions:
**          EXDECOVF                    Result has too many non-fractional
**                                      digits; i.e. more than (prec - scale)
**                                      of them.
**
** Side Effects:
**          none
**
** History:
**      20-may-88 (jrb)
**          Initial creation and coding.
*/
 
VOID
MHpkceil(pkin, prec, scale, pkout)
PTR         pkin;
i4          prec;
i4          scale;
PTR         pkout;
{
    u_char      *spo;                   /* walks thru scale digits */
    u_char      *ppo;                   /* for adding one to pkout */
    u_char      record = (u_char)0;     /* for finding any non-zero bits */
    i4          digcnt = scale;
 
    MEcopy(pkin, (prec/2 + 1), pkout);
            
    /* make po's point to byte containing leftmost digit of fractional part */
    spo = ppo = (u_char *)pkout + (prec / 2) - (digcnt / 2);
 
    /* now zero out all nybbles in fractional part */
    while (digcnt > 0)
    {
        if (digcnt-- & 1)
        {
            record |= *ppo & 0xf0;
            *ppo &= (u_char)0xf;
        }
        else
        {
            record |= *ppo & 0xf;
            *ppo++ &= (u_char)0xf0;
        }
    }
 
    /* Now ppo points to last byte of pkout.  If sign is positive and there */
    /* were some non-zero digits, we add 1.  Otherwise we're done.          */
 
    if (((*ppo & 0xf) != MH_PK_MINUS  &&  (*ppo & 0xf) != MH_PK_AMINUS)
	&& record != (u_char)0)
    {
        i4      carry = 1;
 
        /* first set pointers for integral part of pkout */
        if (((digcnt = scale + 1) & 1) == 0)
            spo--;
 
	/* now loop through integral part and add one until carry goes away */
        while (digcnt <= prec  &&  carry--)
            if (digcnt++ & 1)
                if ((*spo & 0xf0) == (u_char)0x90)
                {
                    *spo-- &= (u_char)0xf;      /* zero out high nybble */
                    carry = 1;
                }
                else
                {
                    *spo-- += (u_char)0x10;
                }
            else
                if ((*spo & 0xf) == (u_char)0x9)
                {
                    *spo &= (u_char)0xf0;       /* zero out low nybble */
                    carry = 1;
                }
                else
                {
                    (*spo)++;
                }
 
	/* if there is still a carry then we overflowed */
        if (carry > 0)
	    MH0decexc(EXDECOVF, pkout, prec);
    }
}
 

/*{
** Name: MHpkabs() - Take the absolute value of a packed decimal.
**
** Description:
**      Takes the absolute value of a packed decimal and returns it as another
**      packed decimal with the same precision and scale.
**
** Inputs:
**      pkin                            Pointer to the input decimal value.
**      prec                            Its precision:  1 <= prec <= 39.
**      scale                           Its scale:  0 <= scale <= prec.
**
** Outputs:
**      pkout                           The resulting packed decimal value is
**                                      placed here.  It will have the same
**                                      precision and scale as the input, but
**                                      a different sign.
**
**      Returns:
**          VOID
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      16-may-88 (jrb)
**          Initial creation and coding.
*/
 
VOID
MHpkabs(pkin, prec, scale, pkout)
PTR         pkin;
i4          prec;
i4          scale;
PTR         pkout;
{
    u_char      *signptr = (u_char *)pkout + prec / 2;

    /* copy the number and make the sign positive */
    MEcopy(pkin, (prec/2 + 1), pkout);
    *signptr = (*signptr & (u_char)0xf0) | MH_PK_PLUS;
}
 

/*{
** Name: MHpkneg() - Negate a packed decimal.
**
** Description:
**      Changes the sign of a packed decimal and returns it as another
**      packed decimal with the same precision and scale.
**
** Inputs:
**      pkin                            Pointer to the input decimal value.
**      prec                            Its precision:  1 <= prec <= 39.
**      scale                           Its scale:  0 <= scale <= prec.
**
** Outputs:
**      pkout                           The resulting packed decimal value is
**                                      placed here.  It will have the same
**                                      precision and scale as the input, but
**                                      a different sign.
**
**      Returns:
**          VOID
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      16-may-88 (jrb)
**          Initial creation and coding.
*/
 
VOID
MHpkneg(pkin, prec, scale, pkout)
PTR         pkin;
i4          prec;
i4          scale;
PTR         pkout;
{
    u_char      *signptr = (u_char *)pkout + prec / 2;
 
    /* make a copy and complement the sign */
    MEcopy(pkin, (prec/2 + 1), pkout);
 
    /* NOTE: This may cause negative 0, but we should always be ready to    */
    /* handle it in all routines which know about the internals of decimal  */
    if ((*signptr & 0xf) == MH_PK_MINUS || (*signptr & 0xf) == MH_PK_AMINUS)
        *signptr = (*signptr & (u_char)0xf0) | MH_PK_PLUS;
    else
        *signptr = (*signptr & (u_char)0xf0) | MH_PK_MINUS;
}
 

/*{
** Name: MHpkadd() - Add two packed decimals.
**
** Description:
**      Adds two packed decimals and returns the result in another
**      packed decimal.  The precision and scale of the result are
**      also returned, and are calculated thus:
**
**          pout = min(39, (max(p1-s1, p2-s2) + max(s1, s2) + 1))
**          sout = max(s1, s2)
**
** Inputs:
**      pk1                             Pointer to the 1st addend.
**      p1                              Its precision:  1 <= p1 <= 39.
**      s1                              Its scale:  0 <= s1 <= p1.
**      pk2                             Pointer to the 2nd addend.
**      p2                              Its precision:  1 <= p2 <= 39.
**      s2                              Its scale:  0 <= s2 <= p2.
**
** Outputs:
**      pkout                           The resulting packed decimal value is
**                                      placed here.
**      pout                            Its precision will be calculated based
**                                      on the above formula, and placed here.
**      sout                            Its scale will be calculated based
**                                      on the above formula, and placed here.
**
**      Returns:
**          VOID
**
**      Exceptions:
**          EXDECOVF                    Sum has too many non-fractional
**                                      digits; i.e. more than (pout - sout)
**                                      of them.
**
** Side Effects:
**          none
**
** History:
**      25-may-88 (jrb)
**          Initial creation and coding.
**	14-sep-2007 (dougi)
**	    One more change for new decimal precision.
*/
 
VOID
MHpkadd(pk1, p1, s1, pk2, p2, s2, pkout, pout, sout)
PTR         pk1;
i4          p1;
i4          s1;
PTR         pk2;
i4          p2;
i4          s2;
PTR         pkout;
i4          *pout;
i4          *sout;
{
    u_char      *ptr1 = (u_char *)pk1 + p1 / 2;    /* ptr's walk thru addends */
    u_char      *ptr2 = (u_char *)pk2 + p2 / 2;    /* and result from end to  */
    u_char      *ptr3;				   /* start */
    u_char      **gptr;    /* generic ptr for pointing at ptr1 or ptr2 */
    i4          *gcnt;     /* generic cnt for counting digits in pk1 or pk2 */
    i4          gp;        /* generic precision counter */
    i4          neg1;
    i4          neg2;
    i4          p3;            /* precision of result */
    i4          s3;            /* scale of result */
    i4          sum;           /* sum of two digits */
    i4          xdigs;         /* nonoverlapping-digits counter */
    i4          p1cnt = 1;     /* digit counters for addends and result */
    i4          p2cnt = 1;
    i4          p3cnt = 1;
    i4          carry = 0;

    /* find out if either addend is negative */
    neg1 = ((*ptr1 & 0xf) == MH_PK_MINUS  ||  (*ptr1 & 0xf) == MH_PK_AMINUS);
    neg2 = ((*ptr2 & 0xf) == MH_PK_MINUS  ||  (*ptr2 & 0xf) == MH_PK_AMINUS);
 
    /* if signs are different we make a temporary copy of one addend, change */
    /* its sign, and call MHpksub with proper arguments			    */
    if (neg1 != neg2)
    {
	char	pktmp[(CL_MAX_DECPREC+1)/2];

        if (neg1)
        {
	    MEcopy(pk1, (p1 / 2 + 1), (PTR)pktmp);
            pktmp[p1 / 2] = (pktmp[p1 / 2] & (u_char)0xf0) | MH_PK_PLUS;
            MHpksub(pk2, p2, s2, pktmp, p1, s1, pkout, pout, sout);
            return;
        }
        else
        {
	    MEcopy(pk2, (p2 / 2 + 1), (PTR)pktmp);
            pktmp[p2 / 2] = (pktmp[p2 / 2] & (u_char)0xf0) | MH_PK_PLUS;
            MHpksub(pk1, p1, s1, pktmp, p2, s2, pkout, pout, sout);
            return;
        }
    }
 
    /* calcuate result's precision and scale using proper formulas */
    p3 = *pout = min(CL_MAX_DECPREC, (max(p1-s1, p2-s2) + max(s1, s2) + 1));
    s3 = *sout = max(s1, s2);

    /* if we got this far, the addends' signs were the same */
    /* set sign on result to whatever addends' signs were */
    ptr3 = (u_char *)pkout + p3 / 2;
    if (neg1)
        *ptr3 = MH_PK_MINUS;
    else
        *ptr3 = MH_PK_PLUS;
 
    /* set generic pointer and counter to number with larger scale */
    if (s1 > s2)
    {
        gptr  = &ptr1;
        gcnt  = &p1cnt;
        xdigs = s3 - s2;
    }
    else
    {
        gptr  = &ptr2;
        gcnt  = &p2cnt;
        xdigs = s3 - s1;
    }
 
    /* now copy any excess scale digits to the result */
    while (xdigs--)
        if (p3cnt++ & 1)
            *ptr3-- |=
                (u_char)((((*gcnt)++ & 1) ? *(*gptr)-- : **gptr << 4) & 0xf0);
        else
            *ptr3 =
                (u_char)((((*gcnt)++ & 1) ? *(*gptr)-- >> 4 : **gptr) & 0x0f);
 
    /* here is the heart of this routine: it adds the two addends' digits */
    /* until there are no more digits in either or both of them (or there */
    /* is no more room in the result)                                     */
    while (p1cnt <= p1  &&  p2cnt <= p2  &&  p3cnt <= p3)
    {
        carry = (
            (sum = (((p1cnt++ & 1)  ?  *ptr1-- >> 4  :  *ptr1) & 0xf)
                 + (((p2cnt++ & 1)  ?  *ptr2-- >> 4  :  *ptr2) & 0xf)
                 + carry)
        > 9);
 
        if (p3cnt++ & 1)
            *ptr3-- |= (u_char)(((carry  ?  sum - 10  :  sum) << 4) & 0xf0);
        else
            *ptr3    = (u_char)(carry  ?  sum - 10  :  sum);
    }
 
    /* now set generic pointer, counter, and precision holder to number with */
    /* the larger number of integral digits                                  */
    if (p1-p1cnt > p2-p2cnt)
    {
        gptr = &ptr1;
        gcnt = &p1cnt;
        gp   = p1;
    }
    else
    {
        gptr = &ptr2;
        gcnt = &p2cnt;
        gp   = p2;
    }
 
    /* then copy excess precision digits (if any) from number with larger */
    /* precision into result and add in carry from above addition loop    */
    while (*gcnt <= gp  &&  p3cnt <= p3)
    {
        carry = (
            (sum = ((((*gcnt)++ & 1)  ?  *(*gptr)-- >> 4  :  **gptr) & 0xf)
                 + carry)
        > 9);
 
        if (p3cnt++ & 1)
            *ptr3-- |= (u_char)(((carry  ?  sum - 10  :  sum) << 4) & 0xf0);
        else
            *ptr3    = (u_char)(carry  ?  sum - 10  :  sum);
    }    
 
    /* now check for overflow; if any preceeding loop was terminated by the */
    /* result's becoming too large, control should end up here; now check   */
    /* if either there is an outstanding carry or if there are any non-zero */
    /* digits left in the excess precision digits we are copying            */
    if (p3cnt > p3)
        if (carry)
	{
	    MH0decexc(EXDECOVF, pkout, *pout);
	    return;
	}
        else
	{
            while (*gcnt <= gp)
                if (((((*gcnt)++ & 1) ? *(*gptr)-- >> 4 : **gptr) & 0xf) != 0)
                {
		    MH0decexc(EXDECOVF, pkout, *pout);
		    return;
                }
	}
 
    /* finally, we put zeros into all remaining positions (if any) in the  */
    /* result, taking into account a possible carry left over from above   */
    while (p3cnt <= p3)
    {
        if (p3cnt++ & 1)
            *ptr3-- |= carry ? (u_char)0x10 : (u_char)0;
        else
            *ptr3    = carry ? (u_char)0x01 : (u_char)0;
        carry = 0;
    }
}
 

/*{
** Name: MHpksub() - Subtract two packed decimals.
**
** Description:
**      Subtracts two packed decimals and returns the result in another
**      packed decimal.  The precision and scale of the result are
**      also returned, and are calculated thus:
**
**          pout = min(39, (max(p1-s1, p2-s2) + max(s1, s2) + 1))
**          sout = max(s1, s2)
**
** Inputs:
**      pk1                             Pointer to the minuend.
**      p1                              Its precision:  1 <= p1 <= 39.
**      s1                              Its scale:  0 <= s1 <= p1.
**      pk2                             Pointer to the subtrahend.
**      p2                              Its precision:  1 <= p2 <= 39.
**      s2                              Its scale:  0 <= s2 <= p2.
**
** Outputs:
**      pkout                           The resulting packed decimal value is
**                                      placed here.
**      pout                            Its precision will be calculated based
**                                      on the above formula, and placed here.
**      sout                            Its scale will be calculated based
**                                      on the above formula, and placed here.
**
**      Returns:
**          VOID
**
**      Exceptions:
**          EXDECOVF                    Result has too many non-fractional
**                                      digits; i.e. more than (pout - sout)
**                                      of them.
**
** Side Effects:
**          none
**
** History:
**      01-jun-88 (jrb)
**          Initial creation and coding.
**	14-sep-2007 (dougi)
**	    One more change for new decimal precision.
*/
 
VOID
MHpksub(pk1, p1, s1, pk2, p2, s2, pkout, pout, sout)
PTR         pk1;
i4          p1;
i4          s1;
PTR         pk2;
i4          p2;
i4          s2;
PTR         pkout;
i4          *pout;
i4          *sout;
{
    u_char      *ptr1 = (u_char *)pk1 + p1 / 2;    /* ptr's walk thru numbers */
    u_char      *ptr2 = (u_char *)pk2 + p2 / 2;	   /* and result from end to  */
    u_char      *ptr3;				   /* start */
    u_char      *tmpptr;	/* used for swapping */
    i4          temp;
    i4          neg1;
    i4          neg2;
    i4          p3;             /* precision of result */
    i4          s3;             /* scale of result */
    i4          diff;           /* difference of two digits */
    i4		xdigs;
    i4          p1cnt = 1;      /* digit counters for addends and result */
    i4          p2cnt = 1;
    i4          p3cnt = 1;
    i4          borrow = 0;
 
    /* find out if either number is negative */
    neg1 = ((*ptr1 & 0xf) == MH_PK_MINUS  ||  (*ptr1 & 0xf) == MH_PK_AMINUS);
    neg2 = ((*ptr2 & 0xf) == MH_PK_MINUS  ||  (*ptr2 & 0xf) == MH_PK_AMINUS);
 
    /* if signs are different we make a temporary copy of one number, change */
    /* its sign, and call MHpkadd with proper arguments			    */
    if (neg1 != neg2)
    {
	char	pktmp[(CL_MAX_DECPREC+1)/2];

        if (neg1)
        {
	    MEcopy(pk2, (p2 / 2 + 1), (PTR)pktmp);
            pktmp[p2 / 2] = (pktmp[p2 / 2] & (u_char)0xf0) | MH_PK_MINUS;
            MHpkadd(pk1, p1, s1, pktmp, p2, s2, pkout, pout, sout);
            return;
        }
        else
        {
	    MEcopy(pk2, (p2 / 2 + 1), (PTR)pktmp);
            pktmp[p2 / 2] = (pktmp[p2 / 2] & (u_char)0xf0) | MH_PK_PLUS;
            MHpkadd(pk1, p1, s1, pktmp, p2, s2, pkout, pout, sout);
            return;
        }
    }

    /* calcuate result's precision and scale using proper formulas */
    p3 = *pout = min(CL_MAX_DECPREC, (max(p1-s1, p2-s2) + max(s1, s2) + 1));
    s3 = *sout = max(s1, s2);
 
    /* set result pointer to end of its space so we can set sign next */
    ptr3 = (u_char *)pkout + p3 / 2;

    /* find out which number is larger in absolute value; swap ptr's and     */
    /* associated precision  and scale if necessary to ensure ptr1 is larger */
    /* in absolute value; also, set result's sign depending on neg flag and  */
    /* which number was larger in absolute value */
    if (MHpkcmp(pk1, p1, s1, pk2, p2, s2) == (neg1 ? 1 : -1))
    {
        tmpptr = ptr1;
        ptr1   = ptr2;
        ptr2   = tmpptr;

        temp   = p1;
        p1     = p2;
        p2     = temp;

        temp   = s1;
        s1     = s2;
        s2     = temp;

        *ptr3 = (neg1  ?  MH_PK_PLUS  :  MH_PK_MINUS);
    }
    else
    {
        *ptr3 = (neg1  ?  MH_PK_MINUS  :  MH_PK_PLUS);
    }
 
    /* loop through any excess scale digits; if excess digits belong to */
    /* minuend we just copy them--if they belong to the subtrahend we   */
    /* complement them by 9 (10 for the rightmost) and copy them        */
    if (s1 >= s2)
    {
        xdigs = s3 - s2;
        while (xdigs--)
            if (p3cnt++ & 1)
                *ptr3-- |=
                    (u_char)(((p1cnt++ & 1) ? *ptr1-- : *ptr1 << 4) & 0xf0);
            else
                *ptr3 =
                    (u_char)(((p1cnt++ & 1) ? *ptr1-- >> 4 : *ptr1) & 0x0f);
    }
    else
    {
        xdigs = s3 - s1;
        while (xdigs--)
        {
            borrow = (
                (diff = - (((p2cnt++ & 1)  ?  *ptr2-- >> 4  : *ptr2) & 0xf)
                        - borrow)
            < 0);

            if (p3cnt++ & 1)
                *ptr3-- |=
                    (u_char)(((borrow  ?  diff + 10  :  diff) << 4) & 0xf0);
            else
                *ptr3    = (u_char)(borrow  ?  diff + 10  :  diff);
        }
    }
 
    /* and this is the heart of the subtraction routine: we subtract one   */
    /* digit from another and record the borrow;  this continues until any */
    /* of the numbers exceeds its precision 				   */
    while (p1cnt <= p1  &&  p2cnt <= p2  &&  p3cnt <= p3)
    {
        borrow = (
            (diff = (((p1cnt++ & 1)  ?  *ptr1-- >> 4  :  *ptr1) & 0xf)
                  - (((p2cnt++ & 1)  ?  *ptr2-- >> 4  :  *ptr2) & 0xf)
                  - borrow)
        < 0);
 
        if (p3cnt++ & 1)
            *ptr3-- |= (u_char)(((borrow  ?  diff + 10  :  diff) << 4) & 0xf0);
        else
            *ptr3    = (u_char)(borrow  ?  diff + 10  :  diff);
    }
 
    /* since we guaranteed that ptr1 is larger than ptr2, we know that if */
    /* there is anything left it must be in ptr1 (it is not too hard to   */
    /* convince yourself that if p3cnt > p3 then there cannot be digits   */
    /* remaining in BOTH ptr1 and ptr2); now we copy any remaining digits */
    /* from ptr1 to ptr3 (allowing for a borrow from above)		  */
    while (p1cnt <= p1  &&  p3cnt <= p3)
    {
        borrow = (
            (diff = (((p1cnt++ & 1)  ?  *ptr1-- >> 4  :  *ptr1) & 0xf)
                  - borrow)
        < 0);
 
        if (p3cnt++ & 1)
            *ptr3-- |= (u_char)(((borrow  ?  diff + 10  :  diff) << 4) & 0xf0);
        else
            *ptr3    = (u_char)(borrow  ?  diff + 10  :  diff);
    }    

    /* now check for overflow;  if there is an outstanding borrow we must */
    /* have a 1 in ptr1 right now (anything larger will mean more digits  */
    /* are necessary in the result, and a 0 means that there are non-zero */
    /* digits somewhere to the left in ptr1 (because ptr1 is greater than */
    /* ptr2) so there can't be a borrow if all digits remaining in ptr1   */
    /* are zeros!)							  */
    if (p3cnt > p3)
    {
	/* a borrow is ok as long as the next ptr1 digit is a one (in */
	/* which case it cancels out and everthing is peachy)	      */
        if (borrow)
	    if ((((p1cnt++ & 1) ? *ptr1-- >> 4  :  *ptr1) & 0xf) != 1)
	    {
		MH0decexc(EXDECOVF, pkout, *pout);
		return;
	    }

	while (p1cnt <= p1)
	    if ((((p1cnt++ & 1) ? *ptr1-- >> 4 : *ptr1) & 0xf) != 0)
	    {
		MH0decexc(EXDECOVF, pkout, *pout);
		return;
	    }
    }

    /* finally, we put zeros into all remaining positions (if any) */
    while (p3cnt <= p3)
        if (p3cnt++ & 1)
            ptr3--;
        else
            *ptr3 = (u_char)0;
}

 

/*{
** Name: MHpkmul() - Multiply two packed decimals.
**
** Description:
**      Multiplies two packed decimals and returns the result in another
**      packed decimal.  The precision and scale of the result are
**      also returned, and are calculated thus:
**
**          pout = min(39, (p1+p2))
**          sout = min(39, (s1+s2))
**
** Inputs:
**      pk1                             Pointer to the multiplicand.
**      p1                              Its precision:  1 <= p1 <= 39.
**      s1                              Its scale:  0 <= s1 <= p1.
**      pk2                             Pointer to the multiplier.
**      p2                              Its precision:  1 <= p2 <= 39.
**      s2                              Its scale:  0 <= s2 <= p2.
**
** Outputs:
**      pkout                           The resulting packed decimal value is
**                                      placed here.
**      pout                            Its precision will be calculated based
**                                      on the above formula, and placed here.
**      sout                            Its scale will be calculated based
**                                      on the above formula, and placed here.
**
**      Returns:
**          VOID
**
**      Exceptions:
**          EXDECOVF                    Result has too many non-fractional
**                                      digits; i.e. more than (pout - sout)
**                                      of them.
**
** Side Effects:
**          none
**
** History:
**      13-sep-88 (jrb)
**          Initial creation.
**	16-feb-89 (jrb)
**	    Rewrote using base-10000 digits to improve performance.
**	20-feb-89 (jrb)
**	    Corrected overflow handling by not generating overflow when excess
**	    fractional digits resulted; just truncate the digits.
**	03-jul-97 (i4jo01)
**	    Minor fix to overflow handling. Use the precision value to
**	    calculate acutal boundary for truncating digits.
**	23-oct-02 (drivi01)
**	    Added code to truncate some of the fractional digits if scale gets 
**	    too high and an overflow happens because digits to the left of the
**	    decimal point are unable to display.
**	18-nov-02 (drivi01)
**	    Added some code to my last change to prevent negative numbers for 
**	    scale or significant digits being cut off unneccessarily when two 
**	    decimals with high scale and/or precision being multiplied.
**	19-dec-02 (drivi01)
**	    Backed out changes #460603,460686,460173 due to disagreement between 
**	    Development and Level2 Support on decimal functionality.
**	31-jul-2006 (kibro01)
**	    Backed out change 431041 (original bug 83402) which caused a
**	    precision of (p1+p2)%31 as opposed to documented behaviour
**	    of min(p1+p2,31).
*/

VOID
MHpkmul(pk1, p1, s1, pk2, p2, s2, pkout, pout, sout)
PTR         pk1;
i4          p1;
i4          s1;
PTR         pk2;
i4          p2;
i4          s2;
PTR         pkout;
i4          *pout;
i4          *sout;
{
    u_char	*ptr1 = (u_char *)pk1 + p1 / 2;	/* end of multiplicand */
    u_char	*ptr2 = (u_char *)pk2 + p2 / 2;	/* end of multiplier */
    u_char	*ptr3;				/* product pointer   */
    i4		pka1[(CL_MAX_DECPREC + 3) / 4];		/* temp buffers */
    i4		pka3[(2*CL_MAX_DECPREC + 3) / 4];
    i4		pkc1;				/* indexes into buffers */
    i4		pkc3 = 0;
    i4		pkc3sav = 0;			/* holder for pkc3	    */
    i4		pc2 = 0;			/* precision counter	    */
    i4		pka1used = (p1 + 3) / 4;	/* space needed in pka1	    */
						/* buffer for pk1 (storing  */
						/* 4 digits per nat)	    */
    i4		pka3used;
    i4		dig;
    i4		carry;
    i4		tens;				/* for converting into and  */
						/* out of base 10000	    */
    i4		skip = 0;			/* # of digits to truncate  */
    i4	tmp;				/* temp product of two digits */
    i4		t;
    i4		i;

    /* set resultant precision and scale according to proper formulas */
    *pout = min(CL_MAX_DECPREC, (p1 + p2));
    *sout = min(CL_MAX_DECPREC, (s1 + s2));

    /* set ptr3 to end of result buffer */
    ptr3 = (u_char *)pkout + *pout / 2;

    /* set result's sign;  if signs are same then set result pos else neg */
    *ptr3 = ((*ptr1 & 0xf) == MH_PK_MINUS  ||  (*ptr1 & 0xf) == MH_PK_AMINUS)
         == ((*ptr2 & 0xf) == MH_PK_MINUS  ||  (*ptr2 & 0xf) == MH_PK_AMINUS)
	 ?  MH_PK_PLUS  :  MH_PK_MINUS;

    /* Algorithm: 
    **	We will put the multiplicand (pk1) into a temp array of nats (pka1)
    **	four digits at a time.  Then we multiply using base 10000 digits; this
    **	cuts down on the number of iterations on the inner loop.  The number
    **	10000 was chosen because we are guaranteed to be able to hold at least
    **	10 digits using a i4 (i4s are at least 32 bits) and we know
    **	that the product of two 4 digit numbers can never be longer than 10
    **	digits.  So base 10000 means we are dealing with 4 digits at a time.
    **	The (x + 3) / 4 that you see all over this code finds the number of
    **	base-10000 digits necessary to hold x base-10 digits.
    */

    /* zero out temp result buffer */
    pka3used = (p1 + p2 + 3) / 4;
    for (i=0; i < pka3used; i++)
	pka3[i] = 0;

    /* move pk1 into temp multiplicand buffer four digits at a time */
    t = -1;
    for (i=0; i < p1; i++)
    {
	if (i % 4 == 0)
	{
	    pka1[++t] = (*ptr1-- >> 4) & 0xf;
	    tens = 10;
	}
	else
	{
	    pka1[t] = pka1[t] +
			tens * ((i & 1  ?  *ptr1  :  (*ptr1-- >> 4)) & 0xf);
	    tens *= 10;
	}
    }
    

    /* now get a digit from the multiplier and if it's non-zero then multiply */
    /* it by each digit of the multiplicand and add the product to the result */
    while (pc2++ < p2)
    {
	/* convert four digits (or less) of multiplier into dig */
	tens = 10;
	dig = (*ptr2-- >> 4) & 0xf;

	for (i=0; i < 3  &&  pc2 < p2; i++, tens *= 10)
	    dig = dig + tens * ((pc2++ & 1  ?  *ptr2  :  (*ptr2-- >> 4)) & 0xf);

	if (dig != 0)
	{
	    /* now multiply pka1 digits by dig and store in pka3 */
	    pkc1 = 0;
	    carry = 0;
	    while (pkc1 < pka1used)
	    {
		tmp = pka1[pkc1++] * dig + pka3[pkc3] + carry;
		pka3[pkc3++] = tmp % 10000L;
		carry = tmp / 10000L;
	    }

	    pka3[pkc3] = carry;
	}
	pkc3  = ++pkc3sav;
    }

    /* if there are more than 39 fractional digits we truncate some of them */
    t = -1;
    if (*sout == CL_MAX_DECPREC)
    {
	t = (skip = s1 + s2 - CL_MAX_DECPREC) / 4;
	if (skip % 4 == 0)
	{
	    t--;
	}
	else
	{
	    dig = pka3[skip / 4];
	    for (i = skip % 4; i > 0; i--)
		dig /= 10;
	}
    }

    /* now move answer from temp result buffer into pout */
    for (i = 0; i < *pout; i++)
    {
	if ((i + skip) % 4 == 0)
	    dig = pka3[++t];

	if (i & 1)
	    *ptr3 = (u_char)(dig % 10);
	else
	    *ptr3-- |= (u_char)((dig % 10) << 4);

	dig /= 10;
    }

    if (dig != 0)
    {
	MH0decexc(EXDECOVF, pkout, *pout);
	return;
    }

    while (++t < pka3used)
    {
	if (pka3[t] != 0)
	{
	    MH0decexc(EXDECOVF, pkout, *pout);
	    return;
	}
    }
}
 

/*{
** Name: MHpkdiv() - Divide two packed decimals.
**
** Description:
**      Divides two packed decimals and returns the result in another
**      packed decimal.  The precision and scale of the result are
**      also returned, and are calculated thus:
**
**          pout = 39
**          sout = max(0, (39-p1+s1-s2))
**
** Inputs:
**      pk1                             Pointer to the dividend.
**      p1                              Its precision:  1 <= p1 <= 39.
**      s1                              Its scale:  0 <= s1 <= p1.
**      pk2                             Pointer to the divisor.
**      p2                              Its precision:  1 <= p2 <= 39.
**      s2                              Its scale:  0 <= s2 <= p2.
**
** Outputs:
**      pkout                           The resulting packed decimal value is
**                                      placed here.
**      pout                            Its precision will be calculated based
**                                      on the above formula, and placed here.
**      sout                            Its scale will be calculated based
**                                      on the above formula, and placed here.
**
**      Returns:
**          VOID
**
**      Exceptions:
**          EXDECOVF                    Result has too many non-fractional
**                                      digits; i.e. more than (pout - sout)
**                                      of them.
**          EXDECDIV                    Attempt to divide by zero.  The
**                                      denominator (2nd input decimal value)
**                                      is equal to zero. 
**
** Side Effects:
**          none
**
** History:
**	20-feb-89 (jrb)
**	    Initial creation and coding.
**      15-Aug-2002 (huazh01)
**          Modify the formula for computing the result scale to:
**          sout = max(min(s1, s2), (31-p1+s1-s2)), and replace 
**          the constant 31 to CL_MAX_DECPREC.
*/
 
VOID
MHpkdiv(pk1, p1, s1, pk2, p2, s2, pkout, pout, sout)
PTR         pk1;
i4          p1;
i4          s1;
PTR         pk2;
i4          p2;
i4          s2;
PTR         pkout;
i4          *pout;
i4          *sout;
{
    u_char	*ptr1 = (u_char *)pk1 + p1 / 2;	/* end of dividend */
    u_char	*ptr2 = (u_char *)pk2 + p2 / 2;	/* end of divisor */
    u_char	*ptr3;
    i4		pka1[(2*CL_MAX_DECPREC + 3) / 4 + 1];	/* temp buffers */
    i4		pka2[(CL_MAX_DECPREC + 3) / 4];
    i4		pka3[(2*CL_MAX_DECPREC + 3) / 4 + 1];
    i4		pk1frac;
    i4		pka1used;
    i4		pka2used = 0;
    i4		carry;
    i4		borrow;
    i4		offset;
    i4		tens;				/* for converting into and  */
						/* out of base 10000	    */
    i4	tmp;				/* temp product of two digits */
    i4		dig;
    i4		t;
    i4		i;
    i4		j;

    /* set precision, scale, and fractional digits of result */
    *pout = CL_MAX_DECPREC;
    *sout = max(min(s1, s2), ( CL_MAX_DECPREC - p1 + s1 - s2));
    pk1frac = *sout + s2;

    /* set ptr3 to end of result buffer */
    ptr3 = (u_char *)pkout + *pout / 2;

    /* set result's sign;  if signs are same then set result pos else neg */
    *ptr3 = ((*ptr1 & 0xf) == MH_PK_MINUS  ||  (*ptr1 & 0xf) == MH_PK_AMINUS)
         == ((*ptr2 & 0xf) == MH_PK_MINUS  ||  (*ptr2 & 0xf) == MH_PK_AMINUS)
	 ?  MH_PK_PLUS  :  MH_PK_MINUS;

    /* now put ptr2 into pka2 using base-10000 digits */
    t = -1;
    for (i=0; i < p2; i++)
    {
	if (i % 4 == 0)
	{
	    tens = 10;
	    pka2[++t] = (*ptr2-- >> 4) & 0xf;
	    if (t > 0  &&  pka2[t-1] != 0)
		pka2used = t;
	}
	else
	{
	    pka2[t] += tens * (((i & 1)  ?  *ptr2  :  (*ptr2-- >> 4)) & 0xf);
	    tens *= 10;
	}
    }
    if (pka2[t] != 0)
        pka2used = t+1;

    /* if pka2used never moved then the divisor was 0 */
    if (pka2used == 0)
    {
	MH0decexc(EXDECDIV, pkout, *pout);
	return;
    }

    /* now we have to set up the dividend so that is has at least (39 + p2) */
    /* base-10 digits (so that the quotient will have 39 digits) and scale  */
    /* pk1frac.  First set up the scale part (fractional part).		    */
    if (s1 <= pk1frac)
    {
	i4	pad = pk1frac - s1;

	offset = pad % 4;
	for (t=0; t < (pad + 3) / 4; t++)
	    pka1[t] = 0;
	t--;

	tens = 1;
	for (i=0; i < offset; i++)
	    tens *= 10;

	i=0;
    }
    else
    {
	offset = 0;
	t = -1;
	i = s1 - pk1frac;
	ptr1 -= (i + 1) / 2;
    }

    /* now move dividend into pka1 buffer using base-10000 digits */
    for (; i < p1; i++)
    {
	if ((i + offset) % 4 == 0)
	{
	    tens = 10;
	    pka1[++t] = ((i & 1)  ?  *ptr1  :  (*ptr1-- >> 4)) & 0xf;
	}
	else
	{
	    pka1[t] += tens * (((i & 1)  ?  *ptr1  :  (*ptr1-- >> 4)) & 0xf);
	    tens *= 10;
	}
    }

    /* now we've filled (p1 - s1 + pk1frac) base-10 places in pka1; zero    */
    /* fill (if necessary) until we have (10 + pka2used) base-10000 digits   */
    /* so that when we divide by pka2 we will have 10 base-10000 digits left */
    /* which will give us the 39 base-10 digits we need to return.	    */
    while (++t < 10 + pka2used)
	pka1[t] = 0;
    pka1used = t;

    /* now we're ready to divide; we fast-path the case where pka2used == 1 */
    /* and use the normalization method when it's larger; note that these   */
    /* algorithms are just doing division of nonnegative integers--all	    */
    /* accounting for signs and decimal point placement has been done above */
    carry = 0;
    if (pka2used == 1)
    {
	i = pka2[0];
	
	/* this is easy... just divide in one pass */
	for (t = pka1used-1; t >= 0; t--)
	{
	    tmp = pka1[t] + carry * 10000L;
	    pka3[t] = tmp / i;
	    carry = tmp % i;
	}
    }
    else
    {
	/* the following algorithm is based on Knuth Vol. II, pp. 255-260   */
	/* and the associated exercises					    */
	i4	    d = 10000L / (pka2[pka2used-1] + 1);
	i4	    v1, v2;
	i4	    q;

	/* first we normalize pka1 and pka2 by multiplying by d; this will  */
	/* ensure that the leading digit of the divisor (pka2) is at least  */
	/* 5000 which is necessary for the algorithm's later assumptions.   */
	if (d == 1)
	{
	    pka1[pka1used] = 0;
	}
	else
	{
	    /* multiply divisor by d */
	    for (i=0; i < pka2used; i++)
	    {
		tmp = (i4)pka2[i] * d + carry;
		pka2[i] = tmp % 10000L;
		carry = tmp / 10000L;
	    }

	    /* multiply dividend by d */
	    for (i=0; i < pka1used; i++)
	    {
		tmp = (i4)pka1[i] * d + carry;
		pka1[i] = tmp % 10000L;
		carry = tmp / 10000L;
	    }
	    pka1[pka1used] = carry;
	}

	/* we used an additional position in pka1 here so account for it */
	pka1used++;

	v1 = pka2[pka2used-1];
	v2 = pka2[pka2used-2];
	for (t = pka1used-1; t >= pka2used; t--)
	{
	    /* first we try to get an approximation for our quotient digit; */
	    /* this approximation either be correct or one too large	    */
	    if (pka1[t] == v1)
	    {
		q = 9999;
	    }
	    else
	    {
		i4	    f = pka1[t] * 10000L + pka1[t-1];

		q = f / v1;

		/* this will loop at most twice */
		while ((i4)v2*q > (f-q*(i4)v1)*10000L+pka1[t-2])
		    q--;
	    }

	    /* now subtract pka2*q from pka1[t] through pka1[t-pka2used] */
	    borrow = 0;
	    for (i = t-pka2used, j=0; i < t; i++, j++)
	    {
		tmp = pka1[i] - (i4)pka2[j] * q + borrow;

		if (tmp < 0L)
		{
		    pka1[i] = tmp % 10000L;
		    borrow = tmp / 10000L;
		    if (pka1[i] < 0)
		    {
			pka1[i] += 10000;
			borrow--;
		    }
		}
		else
		{
		    pka1[i] = tmp;
		    borrow = 0;
		}
	    }
	    pka1[t] += borrow;
	    pka3[t-pka2used] = q;

	    /* the next test will be true with probability on the order of  */
	    /* one in 5000 (Knuth, 4.3.1, exercise 21).  Note that Knuth's  */
	    /* solution to this exercise is base 10, and therefore will not */
	    /* work for this routine.  An example of input that will cause  */
	    /* the following code to execute is:			    */
	    /*       4121633300000000 [31,0] / 588888888888 [16,0]	    */
	    /* In this case q will be 6999, which is one too large.	    */
	    /* Note: This code will never be executed if pka2used < 3.	    */
	    if (pka1[t] < 0)
	    {
		pka3[t-pka2used]--;	/* was one too many */

		/* add divisor back once */
		carry = 0;
		for (i = t-pka2used, j=0; i < t; i++, j++)
		{
		    carry = (pka1[i] += pka2[j] + carry) > 10000;

		    if (carry)
			pka1[i] -= 10000;
		}
		pka1[t]++;  /* there will always be a carry left over here  */
	    }
	}
    }

    /* put result into pkout */
    t = -1;
    for (i=0; i < *pout; i++)
    {
	if (i % 4 == 0)
	    dig = pka3[++t];

	if (i & 1)
	    *ptr3 = (u_char)(dig % 10);
	else
	    *ptr3-- |= (u_char)((dig % 10) << 4);

	dig /= 10;
    }

    if (dig != 0)
    {
	MH0decexc(EXDECOVF, pkout, *pout);
	return;
    }

    while (++t < pka1used-pka2used)
    {
	if (pka3[t] != 0)
	{
	    MH0decexc(EXDECOVF, pkout, *pout);
	    return;
	}
    }
}
 

/*{
** Name: MHpkcmp() - Compare two packed decimals.
**
** Description:
**      Compares two packed decimals and returns 1 if the first is greater than
**      the second, -1 if the first is less than the second, and 0 if they
**      are equal.
**
** Inputs:
**      pk1                             Pointer to the 1st input decimal value.
**      p1                              Its precision:  1 <= p1 <= 39.
**      s1                              Its scale:  0 <= s1 <= p1.
**      pk2                             Pointer to the 2nd input decimal value.
**      p2                              Its precision:  1 <= p2 <= 39.
**      s2                              Its scale:  0 <= s2 <= p2.
**
** Outputs:
**      none
**
**      Returns:
**           1                          First is greater than second.
**           0                          First is equal to second.
**          -1                          First is less than second.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      24-may-88 (jrb)
**          Initial creation and coding.
*/
 
i4
MHpkcmp(pk1, p1, s1, pk2, p2, s2)
PTR         pk1;
i4          p1;
i4          s1;
PTR         pk2;
i4          p2;
i4          s2;
{
    u_char      *ptr1 = (u_char *)pk1;
    u_char      *ptr2 = (u_char *)pk2;
    i4		pc1   = p1;	/* precision counters */
    i4		pc2   = p2;
    i4          ipdiff;		/* difference in integral part lengths */
    i4          neg1;
    i4          neg2;
    i4          ares = 0;	/* absolute result (disregarding signs) */
    i4          digbits = 0;	/* used for tracking non-zero bits */
    i4          d = 0;
 
    /* get signs of ptr1 and ptr2 decimal numbers */
    neg1 = (   (ptr1[p1 / 2] & 0xf) == MH_PK_MINUS
            || (ptr1[p1 / 2] & 0xf) == MH_PK_AMINUS);
    neg2 = (   (ptr2[p2 / 2] & 0xf) == MH_PK_MINUS
	    || (ptr2[p2 / 2] & 0xf) == MH_PK_AMINUS);
 
    /* check excess integral digits (if any) for non-zero digits; if any */
    /* exist, we can stop now with the proper absolute result		 */
    if (p1 - s1 > p2 - s2)
    {
        ipdiff = (p1 - s1) - (p2 - s2);
        while (ipdiff--  &&  !ares)
            ares = (pc1-- & 1  ?  (*ptr1 & 0xf0)  :  (*ptr1++ & 0xf));
    }
    else
    {
        ipdiff = (p2 - s2) - (p1 - s1);
        while (ipdiff--  &&  !ares)
            ares = -(pc2-- & 1  ?  (*ptr2 & 0xf0)  :  (*ptr2++ & 0xf));
    }
 
    /* loop while result is still unknown and compare digits as we go;     */
    /* d records digits of ptr2 (it doesn't matter which one) and ORs them */
    /* into digbits... this is all done for one special case: comparing    */
    /* positive zero to negative zero (a positive number is always greater */
    /* than a negative number except here where they are equal, so I have  */
    /* to know whether there were any non-zero digits if the ares is 0)    */
    while (!ares  &&  pc1  &&  pc2)
    {
        ares =  (     ((pc1-- & 1)  ?  (*ptr1 >> 4)  :  *ptr1++) & 0xf) -
                (d = (((pc2-- & 1)  ?  (*ptr2 >> 4)  :  *ptr2++) & 0xf));
 
        digbits |= d;
    }
    
    /* if there is still no absolute result check whichever number has */
    /* digits remaining to see if there are any non-zero digits        */
    if (!ares)
        if (pc1 > 0)
            while (pc1 && !ares)
                ares =  (((pc1-- & 1  ?  (*ptr1 >> 4)  :  *ptr1++) & 0xf) != 0);
        else
            while (pc2 && !ares)
                ares = -(((pc2-- & 1  ?  (*ptr2 >> 4)  :  *ptr2++) & 0xf) != 0);
 
    /* finally use the absolute result to determine the final result based */
    /* on neg1 and neg2 						   */
    if (ares == 0)
        if (digbits)
            return(neg2 - neg1);
        else
            return(0);
    else
        if (ares > 0)
            return(neg1 ? -1 :  1);
        else
            return(neg2 ?  1 : -1);
}


/*{
** Name: MH0decexc() - Decimal exception occurred so signal it
**
** Description:
**	This routine just zeros out the result and signals the exception
**	that it is handed.
**	
** Inputs:
**	exc			Exception to be signalled
**	pkbuff			Output decimal buffer to be zeroed
**	prec			Precision of the output decimal buffer
**	
** Outputs:
**      pkbuff			Is filled with 0's
**
**      Returns:
**	    VOID
**  
**      Exceptions:
**          EXDECOVF
**	    EXDECDIV
**
** Side Effects:
**          none
**
** History:
**      17-feb-89 (jrb)
**          Initial creation and coding.
*/
static VOID
MH0decexc(exc, pkbuff, prec)
EX	exc;
PTR	pkbuff;
i4	prec;
{
    /* zero out all but the last byte; then set the last byte with a + sign */
    MEfill((prec / 2), (u_char)0, pkbuff);
    *((u_char *)pkbuff + prec / 2) = MH_PK_PLUS;

    EXsignal(exc, 0);
}
