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

/*
**   F_STRRGT - right justify and blank fill a string.
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
**	18-dec-86 (grant)	changed to operate on text strings instead of
**				null-terminated ones.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

f_strrgt(bufin, inlen, bufout, outlen)
u_char	*bufin;
i4	inlen;
u_char	*bufout;
i4	outlen;
{
	/* internal declarations */

	u_char	*in;
	u_char	*out;
	i4	trmlen;
	i4	l;

	/* start of routine */

	/* skip trailing blanks */
	trmlen = f_lentrmwhite(bufin, min(inlen, outlen));
	in = bufin+trmlen-1;
	out = bufout+outlen-1;

	if (in != out || trmlen < outlen)
	{
	    /* copy bufin into bufout right to left */
	    for (l = trmlen; l > 0; l--)
	    {
		    *out-- = *in--;
	    }

	    /* pad the rest with blanks */
	    for (l = outlen-trmlen; l > 0; l--)
	    {
		    *out-- = ' ';
	    }
	}
}
