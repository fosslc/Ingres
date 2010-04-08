/*
**	STgetwords - Parse string to words.
**
**	Parse a string into words.  A word is a sequence of non-whitespace
**	characters, delinated by white space or the beginning or the end of
**	the string.  A sequence of characters deliminated by single or double
**	quotes and followed by white space is also a word.  The caller supplies
**	an array of character pointers, into which the starting address of each
**	word is placed.  The first (or only) white space character after each
**	word is replaced by a Null character.
**
**	History:
**		2-15-83   - Written (jen)
**		march-88  - use wordcount as a bidirectional parameter, as
**			in the spec., fix it so it doesn't return a count
**			if 1 for an empty string.  NOTE: the NULL terminator
**			feature is undocumented - fixed it so that NULL
**			terminator is after last honest token, rather than
**			a pointer to the end of the string (space permitting).
**		01-may-89 (russ) Changing '\0' to EOS.
**		30-nov-92 (pearl)
**			Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped 
**			function headers.
**		08-feb-93 (pearl)
**			Add #include of st.h.
**      8-jun-93 (ed)
**              Changed to use CL_PROTOTYPED
**	11-aug-93 (ed)
**	    unconditional prototypes
**
**Copyright (c) 2004 Ingres Corporation
*/

/* static char	*Sccsid = "@(#)STgetwrds.c	3.1  9/30/83"; */


# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>

VOID
STgetwords(
	register char	*string,
	i4		*wordcount,
	register char	**wordarray)
{
	register i4	count = 0;
	char	**org;

	while(CMwhite(string))
		CMnext(string);

	org = wordarray;

	while (count < *wordcount)
	{
		switch (*string)
		{
		  case EOS:
			break;
		  case '\'':
			string++;
			*wordarray++ = string;
			++count;
			while (*string != '\'' && *string != '\0')
				CMnext(string);
			break;

		  case '\"':
			string++;
			*wordarray++ = string;
			++count;
			while (*string != '\"' && *string != '\0')
				CMnext(string);
			break;

		  default:
			*wordarray++ = string;
			++count;
			while (!CMwhite(string) && *string != '\0')
				CMnext(string);
			break;
		}
		if (*string == EOS)
			break;
		*string = EOS;
		CMnext(string);
		while(CMwhite(string))
			CMnext(string);
		if (*string == EOS)
			break;
	}
	if (count < *wordcount)
		org[count] = NULL;
	*wordcount = count;
}
