/*
**Copyright (c) 2004 Ingres Corporation
**
**	STlcopy - Copy characters.
**
**	Copy characters to buffer.  Null terminate in buffer.
**	Return length of result string.
**
**	History:
**	2-15-83   - Written (jen)
**
 * 	86/02/18  17:21:15  perform
 * 		until length argument is changed to unsigned ignore 
 		negative arguments
**
**	9/29/87 (daveb) -- 
**		Update args to current CL spec (i4s).
**	5/11/89 (daveb)
**		Redo above.  Somehow they got changed back to u_i4.
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
*/

# include	<compat.h>
# include	<gl.h>
# include	<st.h>

size_t
STlcopy(
	const char	*string,
	char	*buffer,
	register size_t	l)
{
	register size_t	len = 0;

	if (l > 0)
	{
		while (l-- && (*buffer++ = *string++) != '\0')
			len++;
	}

	*buffer = '\0';

	return (len);
}
