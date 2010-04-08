/*
**	stskip.c - Skip over a character.
**
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	stskip.c - Skip over blank characters.
**
** Description:
**	File contains routine STskipblank().
**
** History:
**	11/01/83 (xxx) - Written by front-end group.
**	12/09/83 (bab) - Default C-code written.
**	12/01/84 (jhw) - Added conditional compilation to force
**			 compiling of the C default routine for testing.
**	10/14/87 (dkh) - Renamed to STskipblank.
**	03/11/91 (rudyw) Comment out text following #endif
**	31-jan-92 (bonobo)
**	    Replaced double-question marks; ANSI compiler thinks they
**	    are trigraph sequences.
**	30-nov-92 (pearl)
**	    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**	    headers.
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
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# include	<compat.h>
# include	<gl.h>
# include	<st.h>


# ifdef TEST_CDEFAULT
# undef VAX
# endif


/*{
** Name:	STskip - Skip blank characters in a string.
**
** Description:
**	Given the start of a string, find the first character
**	in the string that is NOT the "blank" character.
**	Restrict search to number of bytes specified in
**	input parameter "len".
**
** Inputs:
**	string	Pointer to a character string to search.
**	len	Number of bytes to search.
**
** Outputs:
**	Returns:
**		NULL	If all characters in string were the "blank"
**			character.
**		ptr	Pointer to first byte in string that
**			is NOT the "blank" character.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	11/01/83 (xxx) - Written by front-end group.
**	12/09/83 (bab) - Default C-code written.
**	12/01/84 (jhw) - Added conditional compilation to force
**			 compiling of the C default routine for testing.
**	10/14/87 (dkh) - Renamed to STskipblank.
*/
char *
STskipblank(
	char	*string,
	size_t	len)
{
# if defined(VAX) && defined(BSD)
	asm("skpc $32, 8(ap), *4(ap)");
	asm("beql NOTFOUND");
	asm("movl r1, r0");
	asm("ret");
	asm("NOTFOUND:");
	asm("clrl r0");
	asm("ret");
# else
	char		ch = ' ';
	register char	*terminator = string + len;
	char		*retval = NULL;

	for (; (string < terminator) && (*string == ch); string++)
		;
	if (string < terminator)
		retval = string;

	return(retval);
# endif /* VAX and BSD */
}
