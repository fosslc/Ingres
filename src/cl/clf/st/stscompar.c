/*
**Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>

/*
**  STRING COMPARE
**
**	The strings 'a_ptr' and 'b_ptr' are compared.  Blanks are
**	ignored.  The first string may be no longer than 'a_len'
**	bytes, and the second string may be no longer than 'b_len'
**	bytes.  If either length is zero, it is taken to be very
**	long.  A null byte also terminates the scan.
**
**	Compares are based on the ascii ordering.
**
**	Shorter strings are less than longer strings.
**
**	Return value is positive one for a > b, minus one for a < b,
**	and zero for a == b.
**
**	Examples:
**		"abc"		>  "ab"
**		"  a bc  "	== "ab  c"
**		"abc"		<  "abd"
**
**	History:
**		11/04/87 (dkh) - 
**			Fixed to compare "*ap", rather than "ap", to '\0'.
**		11-may-89 (daveb)
**			Supposed to be i4, not u_i4.
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
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


i4
STscompare(
	const char	*ap,
	size_t	a_len,
	const char	*bp,
	size_t	b_len)
{
	register size_t	al;
	register size_t	bl;
	i4		ret_val = -2;
	i4		cmp;


	al = a_len;

	if (al == 0)
		al = MAXI2;

	bl = b_len;

	if (bl == 0)
		bl = MAXI2;

	while (ret_val == -2)
	{

		/* supress blanks in both strings */
		while (al > 0 && CMspace(ap))
		{
			al -= CMbytecnt(ap);
			CMnext(ap);
		}

		if (al <= 0)
			ap = "";

		while (bl > 0 && CMspace(bp))
		{
			bl -= CMbytecnt(bp);
			CMnext(bp);
		}

		if (bl <= 0)
			bp = "";

		/* do inequality tests */
		cmp = CMcmpcase(ap,bp);

		if (cmp < 0)
			ret_val = -1;
		else if (cmp > 0)
			ret_val = 1;
		else if (*ap == '\0')
			ret_val = 0;
		else
		{
			/* go on to the next character */

			al -= CMbytecnt(ap);
			CMnext(ap);
			bl -= CMbytecnt(bp);
			CMnext(bp);
		}
	}

	return(ret_val);
}
