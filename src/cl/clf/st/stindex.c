/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include 	<cm.h>
# include	<st.h>

/*{
** Name:	STindex - Find first occurrence of character in string.
**
** Description:
**	Find first ocurrence of a character within another string. 
**	This must be an exact match.
**
** Inputs:
**	str	pointer to source string.	
**	mstr	pointer to matching character.  (Note that this is
**		a string pointer to accomodate one or two byte
**		characters).  The first	occurrence of this character
**		within 'str' will be returned.
**	len	If this is nonzero, limit search to first n bytes of
**		'str'.  If 0, 'str' must be NULL terminated.
**
** Outputs:
**	Returns:
**		pointer to starting point of 'c' within 'str' if found,
**		or Null if not found.
**
**	Exceptions:
**		None
** Side Effects:
**	None
**
** History:
**	2-15-83   - Written (jen)
**	9-22-83 (wood) -- rewritten in VAX-11 MACRO
**	6-28-84 (xxx) -- rewritten correctly
**	27-oct-1986 (yamamoto)
**		rewritten for double byte character.
**	30-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**	08-feb-93 (pearl)
**		Add #include of st.h.
**      8-jun-93 (ed)
**              Changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      21-apr-1999 (hanch04)
**          replace i4 with size_t
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimizations for single byte
*/

char *
STindex_DB(
	const char	*str,
	const char	*mstr,
	size_t	len)
{
	if (str == NULL || mstr == NULL)
		return (NULL);

	if (len <= 0)
		len = 32767;

	while (len > 0 && *str != '\0')
	{
		if (CMcmpcase(str, mstr) == 0)
			return ((char *)str);

		CMbytedec(len, str);
		CMnext(str);
	}

	return (NULL);
}

char *
STindex_SB(
	const char	*str,
	const char	*mstr,
	size_t	len)
{
	if (str == NULL || mstr == NULL)
		return (NULL);

	if (len <= 0)
		len = 32767;

	while (len > 0 && *str != '\0')
	{
		if (CMcmpcase_SB(str, mstr) == 0)
			return ((char *)str);

		CMbytedec_SB(len, str);
		CMnext_SB(str);
	}

	return (NULL);
}

