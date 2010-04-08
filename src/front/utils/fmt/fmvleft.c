/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<adf.h>
# include	<afe.h>
# include	<cm.h>

/*
**   F_STRLFT - left justify and blank fill a string.
**
**	Parameters:
**		bufin	ptr to buffer containing starting string.
**		inlen	length of starting string.
**		bufout	ptr to buffer to contain converted string.
**			This can be the same as bufin.
**		outlen	The length of bufout, which will be blank filled.
**
**	Returns:
**		None
**
**	History:
**		1/12/85 (rlm) - redone to make use of CL routines.
**			NOTE: both the old code and new code reflect
**			assumptions that string copies over the same
**			memory for source and destination work as long
**			as you are shifting left.  These things won't
**			work with a destination buffer offset to the
**			right from the source by less than the length
**			of the destination string.  The same buffer
**			for each argument is OK as advertised.
**			split fstrmov.c into two source files.
**	18-dec-86 (grant)	changed to operate on text strings instead
**				of null-terminated ones.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

f_strlft(bufin, inlen, bufout, outlen)
u_char	*bufin;
i4	inlen;
u_char	*bufout;
i4	outlen;
{
	/* internal declarations */

	u_char	*bufend;

	/* start of routine */

	bufend = bufin + inlen;

	/* search past leading whitespace */
	for (; bufin <= bufend-CMbytecnt(bufin) && CMwhite(bufin);
	     CMnext(bufin))
	    ;

	if (bufin != bufout || inlen != outlen)
	{
	    /* copy bufin into bufout */
	    while (bufin <= bufend-CMbytecnt(bufin) && outlen > 0)
	    {
		CMbytedec(outlen, bufin);
		CMcpyinc(bufin, bufout);
	    }
		    
	    /* pad with blanks */
	    for (; outlen > 0; outlen--)
	    {
		*bufout++ = ' ';
	    }
	}
}
