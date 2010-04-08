/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <st.h>
#include    <cs.h>
#include    <me.h>
#include    <mh.h>
#include    <diracc.h>
#include    <cvcl.h>
#include    <handy.h>

/**
** Name: CVFCVT.C - Float conversion routines
**
** Description:
**      This file contains the following external routines:
**    
**      CVfcvt()           -  float conversion to f format
**      CVecvt()           -  float conversion to e format
**
** History:
 * Revision 1.4  90/11/19  10:14:26  source
 * sc
 * 
 * Revision 1.3  90/10/08  07:58:06  source
 * sc
 * 
 * Revision 1.2  90/05/11  14:23:50  source
 * sc
 * 
 * Revision 1.1  90/03/09  09:14:23  source
 * Initialized as new code.
 * 
 * Revision 1.2  90/02/12  09:39:52  source
 * sc
 * 
 * Revision 1.1  89/05/26  15:45:33  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:05:17  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:11:21  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:28:47  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/10  12:37:22  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:23:35  mikem
**      Updated to conform to jupiter coding standards.
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	21-June-89 (harry)
**	    We don't need any "ifdef VAX"s in this file, since cvdecimal.s
**	    no longer exists. Because of that, we can also remove the
**	    define for CVfcvt_default.
**	11-may-90 (blaise)
**	    Integrated changes from 61 into 63p library.
**	28-sep-1990 (bryanp)
**	    Bug #33308
**	    Negative 'ndigit' arguments passed to the 'fcvt' in SunOS 4.1 do
**	    not work properly. In particular, fcvt may suffer a segmentation
**	    fault with certain floating point values (one known example is
**	    fcvt(1.0e+19,-6,&exp,&sign)). Fixed by not passing negative values.
**      1-june-90 (jkb)
**          integrate rexl's reintrant changes
**	4-nov-92 (peterw)
**	    Integrated change from ingres63p to add mh.h for MHdfinite().
**      16-apr-1993 (stevet)
**          Changed MAXPREC to 15 for IEEE platforms (B39932).
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	16-aug-93 (ed)
**	    add <st.h>
**	15-sep-1995 (canor01)
**	    calling reentrant iiCL functions means semaphore protection
**	    not needed.
**	18-sep-1995 (canor01)
**	    (71323) negative numbers less than zero that get all the
**	    decimals truncated are usually displayed as '-0'.  Check
**	    for the Microsoft NT anomaly to make it consistent.
**      17-oct-95 (emmag)
**	    Added include for me.h
**	01-may-1997 (toumi01)
**	    Add length parm to iiCLfcvt and iiCLecvt to support current
**	    POSIX revision calls to fcvt_r and ecvt_r.  For axp_osf modify
**	    conversion buffer from 128 to 400 per man page warning that
**	    370 bytes are required and 400 are recommended.
**      23-Sep-1997 (kosma01)
**          moved MAXDIGITS to cvcl.h and renamed it CV_MAXDIGITS. This was
**          so iicl.c in handy could use it also for reentrant fcvt function on
**          AIX 4.1 (rs4_us5).
**	20-Jul-2004 (lakvi01)
**		SIR# 112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**/

# ifdef IEEE_FLOAT
# define 	MAXPREC	15
# else
# define	MAXPREC 16
# endif

/* typedefs */
/* forward references */
/* externs */

/* statics */

/*{
** Name: CVfcvt - c version to call unix fcvt
**
** Description:
**    A C language version of CVfcvt that was previously written in assembly 
**    language.  This uses the standard UNIX fcvt programs.
**
** Inputs:
**	value				    value to be converted.
**	prec
**
** Output:
**	buffer				    ascii string 
**	digits
**	exp
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**	18-jun-87 (mgw)
**		Update *digits with the proper value for later use in CVfa().
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
**	28-sep-1990 (bryanp)
**	    Bug #33308
**	    Negative 'ndigit' arguments passed to the 'fcvt' in SunOS 4.1 do
**	    not work properly. In particular, fcvt may suffer a segmentation
**	    fault with certain floating point values (one known example is
**	    fcvt(1.0e+19,-6,&exp,&sign)). Fixed by not passing negative values.
**      26-aug-89 (integrated by jkb for rexl)
**          Added calls to semaphore functions to protect static return from
**          call to C library function fcvt().
**	18-may-94 (vijay)
**	    AIX and hp8 do weird stuff with fcvt. Allow for low numbers
**	    returning zero length strings.
**	30-may-95 (tutto01)
**	    Changed calls to fcvt and ecvt to the re-entrant iiCL versions.
*/
STATUS
CVfcvt(value, buffer, digits, exp, prec)
f8	value;
char	*buffer;
i4	*digits;
i4	*exp;
i4	prec;
{
	i4	sign;
	char	*temp;
	i4	length;
	char	*fcvt();
	char    fcvt_buf[CV_MAXDIGITS+2];
	char	*ecvt();
	char	ecvt_buf[CV_MAXDIGITS+2];
	i4	i;

        if (*digits > MAXPREC)
                *digits = MAXPREC;

	temp = iiCLfcvt(value, prec, exp, &sign, fcvt_buf, sizeof(fcvt_buf));

	/* Don't allow fcvt to give us more precision than
	** is possible. At least on the pyramid machines, the
	** precision specified is the number of digits
	** beyond the decimal point. If the number is, say, 3000,
	** fcvt will give us a number implying a precision of 19
	** (4 prior to the decimal, 15 after).
	**
	** The above-mentioned bug on Pyramid appears no longer to be
	** true. In particular, passing 3000 to fcvt no longer returns
	** a string of length 19. However, fcvt DOES have very different
	** results on different machines. The intention of the following
	** code is as follows: if the string returned from our first fcvt
	** call is longer than MAXPREC ("more precision than is possible"),
	** then if the user specified a VERY long precision (e.g., 15), then
	** 'prec - length + MAXPREC' should be the longest possible precision
	** without exceeding MAXPREC. If that is greater than (or equal to) 0,
	** then make another fcvt call with that precision to try to get
	** the 'max reasonable' precision. If 'prec - length + MAXPREC'
	** is < 0, then this value cannot be formatted in 'f' format, so we
	** must go directly to 'e' format. We don't try to call 'fcvt' in
	** the case of a negative ndigit value because various machines do
	** WILDLY different things in this case (e.g., SunOS4.1 gets a SIGSEGV).
	**
	** Several miscellaneous notes, as long as I'm here:
	** 1) MAXPREC is NOT system dependent, but rather is based off of
	**	IEEE_FLOAT. That seems wrong, but I don't know what's right.
	** 2) Some machines will happily format a double to arbitrary
	**	precision (e.g., fcvt(1.0e+19,22,&arg,&sign) will return
	**	a string of length 42) whereas others seem to have an internal
	**	'max fcvt length' longer than which a returned string will
	**	never be. This internal max length, however, ALSO varies from
	**	machine to machine...
	** 3) Machines vary HIDEOUSLY in their behavior with negative
	**	ndigit values. In particular, SunOS 4.1 on a Sparc when given
	**	fcvt(3000,-1,&exp,&sign) produces '0000'!
	** 4) see the note below.
	*/
	if ((length = STlength(temp)) > MAXPREC)
	{
		if ((prec - length + MAXPREC) >= 0)
			temp = iiCLfcvt(value, prec - length + MAXPREC, exp,
				&sign, fcvt_buf, sizeof(fcvt_buf));
		if ((length = STlength(temp)) > MAXPREC)
			temp = iiCLecvt(value, MAXPREC, exp,
				&sign, ecvt_buf, sizeof(ecvt_buf));
	}

	if (sign)
		STcopy("-", buffer);
	else
		STcopy(" ", buffer);

	/*
	** On AIX and hp8, the string returned by fcvt starts with the first
 	** nonzero digit in the number. ndigit specifies the max precision. If
	** the value is less than 1.0, the number of digits in the returned
	** string is equal to (ndigit - position of the first non-zero digit).
	** If the position of the first nonzero digit in the number is greater
	** than ndigit, a null string is returned.
	**	eg: fcvt(2300.002, 5, &dec, &sign) -->returns "230000200",dec=4
	**	    fcvt(0.002, 5, &dec, &sign) --> returns "200", dec=-2
	**	    fcvt(0.0002, 5, &dec, &sign)-> returns "20", dec=-3
	**	    fcvt(0.00002, 5, &dec, &sign)-> returns "2", dec=-4
	**	    fcvt(0.000002, 5, ..      ) --> returns "", dec= -5
	**	    fcvt(0.0000002, 5, ..      ) --> returns "", dec= -6
	**
	** If this is the case, we will round off the number to all zeros.
	** ie. the number is insignificant. This is the behavior on SunOS.
	** SunOS and Solaris return "00000", dec = 0.
	*/

	if ( length == 0 )
	{
	     *exp = 0;
	     i = min(prec, MAXPREC);
	     MEfill(i, '0', buffer + 1); 
	     *(buffer + i + 1) = '\0';
	}
	else STcat(buffer, temp);

	*digits = STlength(temp);

	if (*digits == 0 && sign != 0)
		*digits = 1;

}


/*{
** Name: CVecvt - c version to call unix ecvt
**
** Description:
**    A C language version of CVfcvt that was previously written in assembly 
**    language.  This uses the standard UNIX fcvt programs.
**
** Inputs:
**	value				    value to be converted.
**
** Output:
**	buffer				    ascii string 
**	digits
**	exp
**
**      Returns:
**          OK                         	    Function completed normally. 
**          FAIL                            Function completed abnormally.
**
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.
**      26-aug-89 (rexl)
**          Added calls to semaphore functions to protect static return from
**          call to C library ecvt().
**      30-may-95 (tutto01)
**          Changed call to ecvt to the re-entrant iiCL version.
*/
STATUS
CVecvt(value, buffer, digits, exp)
f8	value;
char	*buffer;
i4	*digits;
i4	*exp;
{
	i4	sign;
	char	*temp;

	char	*ecvt();
	char    ecvt_buf[CV_MAXDIGITS+2];

	if (*digits > MAXPREC)
		*digits = MAXPREC;

	temp = iiCLecvt(value, *digits, exp, &sign, ecvt_buf, sizeof(ecvt_buf));

	if (sign)
		STcopy("-", buffer);
	else
		STcopy(" ", buffer);

	STcat(buffer, temp);
	*digits = STlength(temp);

}
