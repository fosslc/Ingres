/*
**    Copyright (c) 1987, 2000 Ingres Corporation
*/
#include    <compat.h>
#include    <cv.h>
#include    <mh.h>
#include    <me.h>
#include    <cm.h>
#include    <stdlib.h>

/*{
** Name: CVpkl() - Convert packed decimal to long integer.
**
** Description:
**      Converts a packed decimal value into a four byte integer.
**
** Inputs:
**      pk                              Pointer to the decimal value.
**      prec                            Its precision:  1 <= prec <= 31.
**      scale                           Its scale:  0 <= scale <= prec.
**
** Outputs:
**      num                             The four byte integer result.  This is
**                                      derived by truncating the fractional
**                                      part of the packed decimal, not by
**                                      rounding. 
**
**      Returns:
**          OK                          Success.
**          CV_OVERFLOW                 Number too big to fit in an i4.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-may-88 (jrb)
**          Initial creation and coding.
**	30-dec-92 (mikem)
**	    su4_us5 port.  Added casting of u_char to longnat comparison to 
**	    get rid of acc warning "semantics of "<=" change in ANSI C;".
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	8-Jun-2004 (schka24)
**	    Use newer/safer i8 typedef instead of longlong.
*/

STATUS
CVpkl(pk, prec, scale, num)
PTR         pk;
i4         prec;
i4         scale;
i4          *num;
{
    i4     val = 0;                /* accumulator for num */
    i4     ovrchk;                 /* for checking overflow */
    u_char      *p = (u_char *)pk;      /* for walking thru pk */
    i4         pr = prec;
    u_char      sign;                   /* sign code found in pk */
    u_char      dig;                    /* digit extracted from pk */

    /* read the sign code from pk */
    sign = p[prec / 2] & 0xf;

    /* skip past leading pairs of zeros for the sake of efficiency */
    while (*p == (u_char)0 && pr > scale)
    {
        p++;
        pr -= pr & 1 ? 2 : 1;   /* skip down to next odd precision */
    }

    /* this routine could be coded more compactly, but I was shooting for */
    /* efficiency instead */
    
    /* if sign code is minus, do subtraction loop, else do addition loop */
    if (sign == MH_PK_MINUS  ||  sign == MH_PK_AMINUS)
    {
        ovrchk = MINI4 / 10;

        while (pr > scale)
        {
            dig = (pr-- & 1  ?  *p >> 4 : *p++) & 0xf;

            if ((val > ovrchk) || 
		(val == ovrchk && dig <= ((u_char) abs(MINI4 % 10))))
                val = val * 10 - dig;
            else
                return(CV_OVERFLOW);
        }
    }
    else
    {
        ovrchk = MAXI4 / 10;

        while (pr > scale)
        {
            dig = (pr-- & 1  ?  *p >> 4 : *p++) & 0xf;

            if ((val < ovrchk) || 
		(val == ovrchk && dig <= ((u_char) (MAXI4 % 10))))
                val = val * 10 + dig;
            else
                return(CV_OVERFLOW);
        }
    }
    *num = val;
    return(OK);
}

/*{
** Name: CVpkl8() - Convert packed decimal to 64-bit integer.
**
** Description:
**      Converts a packed decimal value into a eight byte integer.
**
** Inputs:
**      pk                              Pointer to the decimal value.
**      prec                            Its precision:  1 <= prec <= 31.
**      scale                           Its scale:  0 <= scale <= prec.
**
** Outputs:
**      num                             The eight byte integer result.  This is
**                                      derived by truncating the fractional
**                                      part of the packed decimal, not by
**                                      rounding. 
**
**      Returns:
**          OK                          Success.
**          CV_OVERFLOW                 Number too big to fit in an i8.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      23-dec-03 (inkdo01)
**	    Cloned from CVpkl().
**	8-Jun-2004 (schka24)
**	    Workaround Solaris 32-bit strangeness/bug re abs.
*/

STATUS
CVpkl8(pk, prec, scale, num)
PTR         pk;
i4          prec;
i4          scale;
i8          *num;
{
    i8		val = 0;                /* accumulator for num */
    i8		ovrchk;                 /* for checking overflow */
    u_char      *p = (u_char *)pk;      /* for walking thru pk */
    i4          pr = prec;
    u_char      sign;                   /* sign code found in pk */
    u_char      dig;                    /* digit extracted from pk */
    u_char	ovr_units_dig;		/* Units digit of min/max i8 */

    /* read the sign code from pk */
    sign = p[prec / 2] & 0xf;

    /* skip past leading pairs of zeros for the sake of efficiency */
    while (*p == (u_char)0 && pr > scale)
    {
        p++;
        pr -= pr & 1 ? 2 : 1;   /* skip down to next odd precision */
    }

    /* this routine could be coded more compactly, but I was shooting for */
    /* efficiency instead */
    
    /* if sign code is minus, do subtraction loop, else do addition loop */
    if (sign == MH_PK_MINUS  ||  sign == MH_PK_AMINUS)
    {
        ovrchk = MINI8 / 10;
	/* Solaris 32-bit ?bug?: abs(MINI8%10) gives the wrong answer,
	** while -(MINI8 % 10) gives the right answer as long as the
	** type context is sufficiently clear!
	*/
	ovr_units_dig = (u_char) (-(MINI8 % 10));

        while (pr > scale)
        {
            dig = (pr-- & 1  ?  *p >> 4 : *p++) & 0xf;

            if ((val > ovrchk) || 
		(val == ovrchk && dig <= ovr_units_dig))
                val = val * 10 - dig;
            else
                return(CV_OVERFLOW);
        }
    }
    else
    {
        ovrchk = MAXI8 / 10;
	ovr_units_dig = (u_char) (MAXI8 % 10);

        while (pr > scale)
        {
            dig = (pr-- & 1  ?  *p >> 4 : *p++) & 0xf;

            if ((val < ovrchk) || 
		(val == ovrchk && dig <= ovr_units_dig))
                val = val * 10 + dig;
            else
                return(CV_OVERFLOW);
        }
    }
    *num = val;
    return(OK);
}
