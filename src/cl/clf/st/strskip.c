# include	<compat.h>
# include	<gl.h>
# include	<st.h>

/*	SCCS ID = %W% %G%	*/

/*
**Copyright (c) 2004 Ingres Corporation
**
**	STrskip
**
**	Given a string, a character, and a length, find
**	the last char (in the first 'len' positions) in that
**	string that is NOT the specified character.
**
**	Return:
**		pointer to the 'found' char.
**		NULL if no such char exists.
**
**	History:
**		Nov? 83		Written by front-end types.
**		12/9/83		Default C-code written.  (bb)
**
**		12/84 (jhw) -- Added conditional compilation to force compiling
**				of the C default routine for testing.
**		08-mar-91 (rudyw) - Comment out text following #endif
**		30-nov-92 (pearl)
**			Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped 
**			function headers.
**		08-feb-93 (pearl)
**			Add #include of st.h.
**      8-jun-93 (ed)
**              Changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      21-apr-1999 (hanch04)
**          replace i4 with size_t
*/

# ifdef TEST_CDEFAULT
# undef VAX
# endif

char *
STrskip(
	char	*addr,
	char	ch,
	size_t	len)
{
# if defined(VAX) && defined(BSD)
	asm("	movl 4(ap), r0");
	asm("	incl r0");
	asm("	movq 8(ap), r1");
	asm("SKIP:");
	asm("	decl r2");
	asm("	blss NOTFOUND");
	asm("	cmpb -(r0), r1");
	asm("	beql SKIP");
	asm("	ret");
	asm("NOTFOUND:");
	asm("	clrl r0");
	asm("	ret");
# else
	register char	*ptr = addr + len - 1;

	for (; (ptr >= addr) && (*ptr == ch); --ptr)
		;
	
	return (ptr >= addr ? ptr : (char *)NULL);
# endif /* VAX and BSD */
}
