/*
**Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<cv.h>
#include	<me.h>
#include	<cmcl.h>

/*
**  Name: CVAL.C - Functions used to convert ascii to long
**
**  Description:
**      This file contains the following external routines:
**    
**      CVal()             -  convert ascii to i4.
**      CVal8()            -  convert ascii to i8.
**
**  History:
**      Revision 1.3  87/11/17  16:54:13  mikem
**      changed to not use CH anymore
**      
**      Revision 1.3  87/07/27  11:23:19  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	17-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**	    added routine CVal8 for 8 byte conversion.
**	29-may-1996 (toumi01)
**	    Make CVal8 axp_osf platform-conditional and replace inclusion
**	    of <stdlib.h> with declaration "extern long int atol ();" to
**	    avoid dragging in other definitions (such as "abs()") should
**	    this code be used in the future for other 64-bit platforms.
**	    Delete unnecessary inclusion of <cm.h>.
**      22-mar-1999 (hweho01)
**          Use CVal8 for ris_u64 (AIX 64 bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-feb-2001 (somsa01)
**	    Define CVal8() for all LP64 platforms as well.
**      16-Nov-2000 (ahaal01)
**          Use CVal8 for rs4_us5 (AIX 32 bit platform).
**      04-Nov-2002 (hweho01)
**          Added change for AIX hybrid build.
**	14-oct-2003 (somsa01)
**	    On Windows 64-bit, use _atoi64() in CVal8(). Also, changed
**	    result type to be OFFSET_TYPE for CVal8 as well.
**	2-Jan-2004 (schka24)
**	    Use i8 for CVal8.
**	27-Jan-2004 (fanra01)
**	    Use strtol function on windows.
**	27-Jan-2004 (fanra01)
**	    Use _strtoi64 instead of strtol.
**	    Forward reference for stroll taken from standard C header.
**	    long long is not accepted on earlier versions of the MS C compiler.
**	04/01/2004 (gupsh01) 
**	    Fix CVal8, which now checks if the conversion was successful, 
**	    else returns CV_SYNTAX.
**	02-apr-2004 (somsa01)
**	    Use __strtoll() on HP-UX.
**	04/08/2004 (gupsh01)
**	    Fixed CVal8, it does not cover the case where input string 
**	    is character '0', which will be converted to 0 in the result.
**	04/08/2004 (gupsh01, schka24 )
**	    Fixed CVal8 again to take care of more general case. 
**	16-apr-2004 (somsa01)
**	    Include inttypes.h on HP, so that we properly pick up the
**	    definition of __strtoll().
**	22-Jun-2009 (kschendel) SIR 122138
**	    Don't need above thanks to Ian's rewrite without strtoll.
*/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name:	CVal - CHARACTER STRING TO 32-BIT INTEGER
**		       CONVERSION
**
** Description:
**    ASCII CHARACTER STRING TO 32-BIT INTEGER CONVERSION
**
**	  Convert a character string pointed to by `str' to an
**	i4 pointed to by `result'. A valid string is of the form:
**
**		{<sp>} [+|-] {<sp>} {<digit>} {<sp>}
**
**      NOTE that historicly, Ingres accepts an empty string or
**           isolated sign as zero
**
** Inputs:
**	str	char pointer, contains string to be converted.
**
** Outputs:
**	result	i4 pointer, will contain the converted i4 value
**		Only if status is OK will result be updated.
**
**   Returns:
**	OK:		succesful conversion; `result' contains the
**			integer
**	CV_OVERFLOW:	numeric overflow;
**	CV_UNDERFLOW:	numeric underflow;
**	CV_SYNTAX:	syntax error;
**	
** History:
**	03/09/83 -- (mmm)
**		brought code from vms and modified.   
**	19-june-84 (fhc) -- made work after absent history broke it
**	19-mar-87 (mgw)	Bug # 11825
**		Made it work as it does in Unix and IBM so that in particular,
**		it will now return CV_SYNTAX for "5 3 4" instead of
**		converting it to 534.
**	31-mar-87 (mgw)
**		Made it work for MINI4 (= -MAXI4 -1).
**	10-jan-89 (daveb)
**		Take (bj-icl)'s 17-nov-89 mod to use unsigned when checking
**		for overflow:
**	11-may-90 (blaise)
**		Integrated changes from ug into 63p library.
**	21-mar-91 (jrb)
**		Bug 36133. A negative number with leading zeros was causing
**		this routine to incorrectly return with CV_UNDERFLOW.
**	04-Jan-2008 (kiria01) b119694
**	    Rewritten adopting CVal8 changes but just using i4s.
**	9-Feb-2010 (kschendel) SIR 123275
**	    Revise to use CVal8 and down-cast the result.  CVal8 is
**	    now essentially as fast as CVal was anyway, since it runs
**	    in i4 mode for the first 9 digits.
*/
STATUS
CVal(char *str, i4 *result)
{
    i8 val8;
    STATUS sts;

    sts = CVal8(str, &val8);
    if (sts != OK)
	return (sts);

    if (val8 > MAXI4 || val8 < MINI4LL)
    {
	/* underflow / overflow */
	return (val8 < 0) ? CV_UNDERFLOW : CV_OVERFLOW;
    }

    *result = (i4) val8;
    return OK;
}

/*{
** Name:	CVal8 - CHARACTER STRING TO 64-BIT INTEGER
**		       CONVERSION
**
** Description:
**    ASCII CHARACTER STRING TO 64-BIT INTEGER CONVERSION
**
**	  Convert a character string pointed to by `str' to an
**	i8 pointed to by `result'. A valid string is of the form:
**
**		{<sp>} [+|-] {<sp>} {<digit>} {<sp>}
**
**      NOTE that historicly, Ingres accepts an empty string or
**           isolated sign as zero
**
**	CVal8 is the most important char -> int converter for runtime
**	performance, as all the ADF char -> int coercions use CVal8
**	regardless of result size.
**
** Inputs:
**	str	char pointer, contains string to be converted.
**
** Outputs:
**	result	i8 pointer, will contain the converted i8 value
**		Only if status is OK will result be updated.
**
**   Returns:
**	OK:		succesful conversion; `result' contains the
**			integer
**	CV_OVERFLOW:	numeric overflow;
**	CV_UNDERFLOW:	numeric underflow;
**	CV_SYNTAX:	syntax error;
**	
** History:
**	04-Jan-2008 (kiria01) b119694
**	    Rewritten to avoid use of strtoll etc. which needs
**	    errno to report range errors. This is needed to
**	    correct handling of MAXI8 and MINI8 boundary conditions.
**	9-Feb-2010 (kschendel) SIR 123275
**	    Performance tweaking:
**	    - eliminate use of CMdigit in the conversion loops;  we're
**	      assuming ascii digits anyway, so CMdigit is pointless.
**	    - Run the first 9 digits in i4 mode without overflow checking.
**	      Only switch to i8 if the number is larger than 9 digits.
*/
STATUS
CVal8(char *str, i8 *result)
{
    register u_char *p = (u_char *) str; /* Pointer to current char */
    i4 negative = FALSE;	/* Flag to indicate the sign */
    i4 i;
    register u_i4 digit;
    register u_i4 val4 = 0;
    u_i8 val8;			/* Holds the integer being formed */

    /* Skip leading blanks */
    while (CMspace(p))
        CMnext(p);

    /* Check for sign */
    switch (*p)
    {
    case '-':
	negative = TRUE;
	/*FALLTHROUGH*/
    case '+':
	CMnext(p);
        /* Skip any whitespace after sign */
	while (CMspace(p))
	    CMnext(p);
    }

    /* Run the digit loop for either 9 digits, or a non-digit, whichever
    ** happens first.  9 digits will fit in an i4 without overflow.
    */
    i = 9;
    for (;;)
    {
	digit = *p;
	if (digit < '0' || digit > '9')
	    break;
	val4 = val4 * 10 + digit - '0';
	++p;
	if (--i == 0) break;
    }
    val8 = (u_i8) val4;
    if (CMdigit(p))
    {
	/* broke out of i4 loop, finish up with i8 and overflow testing. */
	do
	{
	    digit = *p++ - '0';

	    /* check for overflow - note that in 2s complement the
	    ** magnitude of -MAX will be one greater than +MAX
	    */
	    if (val8 > MAXI8/10 ||
		    val8 == MAXI8/10 && digit > (MAXI8%10)+negative)
	    {
		/* underflow / overflow */
		return negative ? CV_UNDERFLOW : CV_OVERFLOW;
	    }
	    val8 = val8 * 10 + digit;
	} while (*p >= '0' && *p <= '9');
    }

    /* ... or trailing spaces */
    while (CMspace(p))
        CMnext(p);

    if (*p)
	return CV_SYNTAX;

    if (negative)
        *result = -(i8)val8;
    else
        *result = (i8)val8;

    return OK;
}
