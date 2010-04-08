/*
** Copyright (c) 1985, 2001 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<st.h>

/*
**
**	STcat - String concatenate.
**
**	Concatenate two null-terminated strings.  Return pointer.
**
**	History:
**		2-15-83   - Written (jen)
**
**		12-15-83  - Added VAX assembler routine from VMS. (sat)
**
**		12/84 (jhw) -- Added conditional compilation to force compiling
**				of the C default routine for testing.
**
**		08/85 (roger) -- Added 3B5/3B20 assembly code.
**
**		08/86 (cmr) -- Removed VAX assembly code.
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
**	23-mar-2001 (mcgem01)
**	    The IISTcat function is required for backward compatability.
*/

# define STcat_default

char	*
IISTcat(
	char	*pre,
	char	*post)
{

# ifdef x3B5
	asm("    movw	0(%ap),%r0");	/* First pointer to %r0. */
	asm("    STREND");		/* %r0 now points to end of
					**	first string. */
	asm("    movw	%r0,%r1");	/* Move it to destination register
					**	for STRCPY. */
	asm("    movw	4(%ap),%r0");	/* Second string is source for STRCPY */
	asm("    STRCPY");		/* Move second string after first. */
	asm("    movw	0(%ap),%r0");	/* Return ptr to concatenated string. */

# undef	STcat_default
# endif

# ifdef	u3b	/* 3B20 */

	asm("	movw	0(%ap),%r0");	/* s1 */
	asm("	movw	&0,%r2");	/* null char to search for */
	asm("	locce	%r0,%r2,%r2");	/* let your fingers do the walking */
	asm("	movw	4(%ap),%r1");	/* s2 */
	asm("	movce	%r1,%r0,%r2");	/* let your fingers do the walking */
	asm("	movw	0(%ap),%r0");	/* return s1 */

# undef	STcat_default
# endif

# ifdef	STcat_default
	register char *pre1 = pre;
	register char *post1 = post;

	while (*pre1 != '\0')
		++pre1;

	while ((*pre1++ = *post1++) != '\0')
		;

	return (pre);
# endif /* STcat_default */
}
