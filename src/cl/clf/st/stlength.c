# include	<compat.h>
# include	<gl.h>
# include	<st.h>

/*
**Copyright (c) 2004 Ingres Corporation
**
**	STlength - Find the length of a null terminated string.
**
**	History:
**		2-15-83   - Written (jen)
**
**		12-15-83  - Added VAX assembler routine from VMS. (sat)
**		1-10-84	  - Changed the VAX ass. routine. (tw/bb)
**
**		12/84 (jhw) -- Added conditional compilation to force compiling
**				of the C default routine for testing.
**		08/86 (cmr) -- Removed VAX assembly-language version.
**		03/06/87 (dkh) - Added STnlength().
**
**		9/87 (bobm)	Convert to CM use for scanning strings
**
**		10/87 (bobm)	Integrate 5.0 (correct arg types to CL spec)
**		5/89 (daveb)	Back to being i4  for Release 6.
**				Add register variables.
**		30-nov-92 (pearl) Add #ifdef for "CL_BUILD_WITH_PROTOS" and 
**				prototyped function headers.
**		08-feb-93 (pearl)
**			Add #include of st.h.
**      8-jun-93 (ed)
**              Changed to use CL_PROTOTYPED
**		15-jun-93 (terjeb)
**			If STlength is defined in st.h, then undefine it
**			in this module. Machines which defines STlength in
**			stcl.h also defines STLENGTH_WRAPPED_STRLEN to indicate
**			that we will use a libc function instead.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	21-apr-1999 (hanch04)
**	    replace i4 with size_t
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

size_t
STnlength(
	size_t	count,
	const char	*string)
{
	const char	*cp = string;
	register size_t	length;

	for (length = 0; length < count; length++, cp++)
	{
		if (!*cp)
			return (length);
	}

	return (count);
}
