# include	<compat.h>
# include	<gl.h>
# include	<st.h>

/*
** Copyright (c) 1985, 2001 Ingres Corporation
**
**	STcompare - Compare 2 strings.
**
**	Compare 2 strings lexically.  Sense of test is a?b.
**
**	Returns:
**		-1, 1, 0
**
**	History:
**	02-apr-2001 (mcgem01)
**	    File added back in for backward compatability.
*/
#ifdef STcompare
#undef STcompare
#endif 

i4
STcompare(
	char	*a,
	char	*b)
{
	return (strcmp(a, b));
}
