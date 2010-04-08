# include 	<compat.h>
# include 	<gl.h>
# include 	<me.h>
# include	<st.h>

/*	SCCS ID = %W% %G%	*/

/*
**Copyright (c) 2004 Ingres Corporation
**
**	STmove.C	- padded move from string to memory
**
**	Purpose:
**		Move characters from a padded to an area of memory padded with
**		the pad character if needed.
**
**	History:
**		12/84 (jhw) -- Added conditional compilation to force compiling
**				of the C default routine for testing.
**		08-mar-91 (rudyw)
**			Comment out text following #endif
**		30-nov-92 (pearl)
**			Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped 
**			function headers.
**		08-feb-93 (pearl)
**			Add #include of st.h.
**		31-mar-93 (swm)
**			Altered STmove function declaration to correspond to
**			the CL specification by making the dlen parameter a
**			i4 rather than an i2. This fix allows compilation
**			in back/opc to run without errors on Alpha AXP OSF.
**      8-jun-93 (ed)
**              Changed to use CL_PROTOTYPED

**	Log:	stmove.c,v $
**		Revision 1.2  86/04/25  10:13:12  daveb
**		Don't use assembler on system v Vax --
**		the optimizer eats it.
**		
**		Revision 1.3  86/02/13  22:08:58  perform
**		rewrite C default code, avoiding call to STlength.
** 
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
**	13-Jul-2006 (kschendel)
**	    While on i2 mefill/mecopy campaign, remove 3B5 stuff.
**	    I haven't seen a WE32000 for years.
*/

# ifdef TEST_CDEFAULT
# undef VAX
# endif

VOID
STmove(
	const char *source,
	char	padchar,
	size_t	dlen,
	char	*dest)
{
# if defined(VAX) && defined(BSD)
	asm("pushl	4(ap)");
	asm("calls	$1,_STlength");
	asm("movc5	r0,*4(ap),8(ap),12(ap),*16(ap)");
# else
	const char	*src_p = source;
	char	*dst_p = dest;
		 size_t	slen   = dlen;
		 size_t	cnt;

	for (cnt = 0 ; slen-- && (*dst_p++ = *src_p++) != '\0' ; cnt++)
	{
			;
	}

	while (cnt < dlen)
		dest[cnt++] = padchar;

# endif /* VAX and BSD */
	return ;
}
