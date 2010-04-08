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
**   F_STRCTR - center and blank fill a string.
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
**	18-dec-86 (grant)	changed to operate on text strings instead of
**				null-terminated ones.
**	07-aug-87 (grant)	stolen from fmvright.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

f_strctr(bufin, inlen, bufout, outlen)
u_char	*bufin;
i4	inlen;
u_char	*bufout;
i4	outlen;
{
	/* internal declarations */

	u_char	*in;
	u_char	*out;
	i4	trmlen;
	i4	lft_blanks;
	i4	rgt_blanks;

	/* start of routine */

	/* skip leading blanks */
	for (in = bufin; inlen > 0 && CMspace(in);
	     CMbytedec(inlen, in), CMnext(in))
	    ;

	/* skip trailing blanks */
	trmlen = f_lentrmwhite(in, min(inlen, outlen));

	out = bufout;

	if (in != out || trmlen < outlen)
	{
	    lft_blanks = (outlen - trmlen) / 2;
	    rgt_blanks = outlen - trmlen - lft_blanks;

	    if (in < out+lft_blanks)
	    {
		/* copy bufin into bufout right to left 
		** to avoid overwriting original string */

		in += trmlen - 1;
		out += outlen - 1;

		/* copy right blanks */
		for (; rgt_blanks > 0; rgt_blanks--)
		{
			*out-- = ' ';
		}

		/* copy actual string */
		for (; trmlen > 0; trmlen--)
		{
			*out-- = *in--;
		}

		/* copy left blanks */
		for (; lft_blanks > 0; lft_blanks--)
		{
			*out-- = ' ';
		}
	    }
	    else
	    {
	        /* copy bufin into bufout left to right
	        ** to avoid overwriting original string */

		/* copy left blanks */
		for (; lft_blanks > 0; lft_blanks--)
		{
			*out++ = ' ';
		}

		/* copy actual string */
		for (; trmlen > 0; trmlen--)
		{
			*out++ = *in++;
		}

		/* copy right blanks */
		for (; rgt_blanks > 0; rgt_blanks--)
		{
			*out++ = ' ';
		}
	    }
	}
}
