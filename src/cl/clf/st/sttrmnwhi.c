/*
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>

/**
** Name:	sttrmnwhi.c -	String Module Trim n White Spaces Routine.
**
** Description:
**	Delete all trailing whitespace of string up to n characters.
**	EOS terminate.
**	Return new length.  Defines:
**
**	STtrmnwhite()	trim white space from string up to n characters.
**
** History:
**		6-june-95 (hanch04)
**		    Added from sttrmwhite()
**		8-jun-95 (hanch04)
**		    change max to max_len
**
**      21-apr-1999 (hanch04)
**          replace i4 with size_t
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-Dec-2008 (kiria01) b121410
**	    Reduce work done in the loop and guard against
**	    writing beyond the buffer end.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimizations for single byte
**/

size_t
STtrmnwhite_DB(
	char	*string,
	size_t    max_len)
{
	register char *p = string;
	register char *nw = p;
	register char *end = p + max_len;

	/*
	** after the loop, nw points to the first character beyond
	** the last non-white character.  Done this way because you
	** can't reverse scan a string with CM efficiently
	*/
	while (p < end && *p != EOS)
	{
		if (!CMwhite(p))
		{
			CMnext(p);
			nw = p;
		}
		else
			CMnext(p);
	}

	{
		register size_t nwl = nw - string;
		if (nwl < max_len)
			*nw = EOS;
		return nwl;
	}
}


size_t
STtrmnwhite_SB(
	char	*string,
	size_t    max_len)
{
	register char *p = string;
	register char *nw = p;
	register char *end = p + max_len;

	/*
	** after the loop, nw points to the first character beyond
	** the last non-white character.  Done this way because you
	** can't reverse scan a string with CM efficiently
	*/
	while (p < end && *p != EOS)
	{
		if (!CMwhite_SB(p))
		{
			CMnext_SB(p);
			nw = p;
		}
		else
			CMnext_SB(p);
	}

	{
		register size_t nwl = nw - string;
		if (nwl < max_len)
			*nw = EOS;
		return nwl;
	}
}
