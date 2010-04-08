/*
**    Copyright (c) 1987, 2000 Ingres Corporation
*/
#include    <compat.h>
#include    <cv.h>
#include    <mh.h>
#include    <me.h>
#include    <cm.h>

FUNC_EXTERN STATUS CV0decerr();

/*{
** Name: CVpkpk() - Convert packed decimal to packed decimal.
**
** Description:
**      Converts a packed decimal value with one precision and scale to another
**      packed decimal value with a possibly different precision and scale.
**      Excess fractional digits are truncated.
**
** Inputs:
**      pk_in                           Pointer to the input decimal value.
**      prec_in                         Its precision:  1 <= prec_in <= 31.
**      scale_in                        Its scale:  0 <= scale_in <= prec_in.
**      prec_out                        Precision for result:
**                                      1 <= prec_out <= 31.
**      scale_out                       Scale for result:
**                                      0 <= scale_out <= prec_out.
**
**      Return statuses are listed in order of increasing severity; the
**      most severe status encountered is always the one returned.
**
**
** Outputs:
**      pk_out                          The resulting packed decimal value is
**                                      placed here.
**
**      Returns:
**          OK                          Success.
**          CV_OVERFLOW                 Too many non-fractional digits (not
**                                      counting leading zeros)
**                                      i.e. more than (prec_out - scale_out)
**                                      of them.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      11-may-88 (jrb)
**          Initial creation and coding.
**      24-jul-89 (jrb)
**          Now zeroes result on overflow.
**      22-sep-89 (jrb)
**          Removed CV_TRUNCATE as possible return status.  It was not used
**          and had performance impacts.
**	19-jul-2000 (kinte01)
**	   Add missing external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

STATUS
CVpkpk(pk_in, prec_in, scale_in, prec_out, scale_out, pk_out)
PTR         pk_in;
i4         prec_in;
i4         scale_in;
i4         prec_out;
i4         scale_out;
PTR         pk_out;
{
    u_char      *pi = (u_char *)pk_in;
    u_char      *po = (u_char *)pk_out;
    i4         pin = prec_in;
    i4         pout = prec_out;
    i4         nlz;                            /* number of leading zeros */
    u_char      sign;

    /* get sign code of pk_in */
    sign = pi[pin/2] & (u_char)0xf;

    /* zero first byte of pk_out in case pout is even */
    *po = (u_char)0;

    /* skip leading zeros on pk_in (before decimal point) */
    while (pin > scale_in)
    {
        if (((pin & 1  ?  *pi >> 4  :  *pi) & 0xf) != (u_char)0)
            break;

        if ((pin-- & 1) == 0)
            pi++;
    }

    /* calculate nlz for pk_out and see if there's enough room */
    if ((nlz = (pout - scale_out) - (pin - scale_in)) < 0)
        return(CV0decerr(CV_OVERFLOW, pk_out, prec_out));

    /* put leading zeros into pk_out */
    while (nlz-- > 0)
        if (pout-- & 1)
            *po = (u_char)0;
        else
            po++;

    /* copy digits from pk_in to pk_out until one of them is full */
    while (pin > 0  &&  pout > 0)
        if (pout-- & 1)
            *po = (pin-- & 1  ?  *pi  :  *pi++ << 4) & 0xf0;
        else
            *po++ |= (pin-- & 1  ?  *pi >> 4  :  *pi++) & 0xf;

    /* if pk_in was full, pad pk_out with zeros to the right */
    if (pin == 0)
        while (pout > 0)
            if (pout-- & 1)
                *po = (u_char)0;
            else
                po++;
            
    /* now set the sign code on pk_out */
    *po |= sign;

    return(OK);
}
