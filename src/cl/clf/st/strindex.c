/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>

/*{
** Name:	STrindex - Find last occurrence of character within string.
**
** Description:
**	Find last ocurrence of a character within another string. 
**	This must be an exact match.
**
** Inputs:
**	str	pointer to source string.	
**	mstr	pointer to matching character.  (Note that this is
**		a string pointer to accomodate one or two byte
**		characters).  The last occurrence of this character
**		within 'str' will be returned.
**	len	If this is nonzero, limit search to first n bytes of
**		'str'.  If 0, 'str' must be NULL terminated.
**
** Outputs:
**	Returns:
**		pointer to starting point of 'c' within 'str' if found,
**		or Null if not found.
** Side Effects:
**	None
**
** History:
**	2-15-83   - Written (jen)
**	27-oct-1986 (yamamoto)
**		rewritten for double byte character.
**	29-apr-87 (bab)
**		changed so that '*where' is compared to EOS, not 'where'.
**	11-may-89 (daveb)
**		len is supposed to be a nat.  Take out magic constant and
**		replace with MAXI2.
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
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      21-apr-1999 (hanch04)
**          replace i4 with size_t
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

char *
STrindex(
	const char	*str,
	const char	*mstr,
	register size_t	len)
{
	char	*where = NULL;

	if (len <= 0)
		len = MAXI2;

	while (len > 0 && *str != '\0')
	{
		if (CMcmpcase(str, mstr) == 0)
			where = (char *)str;

		CMbytedec(len, str);
		CMnext(str);
	}

	if (where != NULL && *where != '\0')
		return (where);
	else
		return (NULL);
}
