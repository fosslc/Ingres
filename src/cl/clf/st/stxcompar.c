/*
**Copyright (c) 2004 Ingres Corporation
*/
/* static char Sccsid[]= "@(#)scompare.c	1.1	3/14/83";	*/

# include	<compat.h>
# include	<gl.h>
# include	<cm.h>
# include	<st.h>


/*
**  STRING COMPARE
**
**	The strings 'a_ptr' and 'b_ptr' are compared.  
**	The first string may be no longer than 'a_len'
**	bytes, and the second string may be no longer than
**	'b_len' bytes.  If either length is zero, it is taken
**	to be very long.  A null byte also terminates the scan.
**	If 'ic' is TRUE we ignore case in the comparison.
**	If 'sb' is TRUE we skip blanks in the comparison.
**
**	Compares are based on the ascii ordering.
**
**	Shorter strings are less than longer strings.
**
**	Return value is positive one for a > b, minus one for a < b,
**	and zero for a == b.
**
**	History:
**
**		Revision 30.2  85/09/27  17:58:36  daveb
**		International changes from 3.0/23ibs; use unsigned chars.
**
**	   10/87 (bobm) - changed to use CM macros
**	   30-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**	   08-feb-93 (pearl)
**		Add #include of st.h.
**      8-jun-93 (ed)
**              Changed to use CL_PROTOTYPED
**		
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
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Optimizations for single byte
*/


i4
STxcompare(
	char	*a_ptr,
	size_t	a_len,
	char	*b_ptr,
	size_t	b_len,
	bool	ic,
	bool	sb)
{
	unsigned char		*ap;
	unsigned char		*bp;
	register size_t	al;
	register size_t	bl;
	i4		ret_val = -2;
	i4		cmp;


	ap = (unsigned char *) a_ptr;
	bp = (unsigned char *) b_ptr;
	al = a_len;

	if (al == 0)
		al = MAXI2;

	bl = b_len;

	if (bl == 0)
		bl = MAXI2;

	if (CMGETDBL)
	{
	    while (ret_val == -2)
	    {
		/* supress blanks in both strings */

		if (sb)
		{
			while (al > 0 && CMspace(ap))
			{
				al -= CMbytecnt(ap);
				CMnext(ap);
			}

			while (bl > 0 && CMspace(bp))
			{
				bl -= CMbytecnt(bp);
				CMnext(bp);
			}
		}


		if (al <= 0)
			ap = (unsigned char *) "";

		if (bl <= 0)
			bp = (unsigned char *) "";

		/* do inequality tests */

		if (ic)
			cmp = CMcmpnocase(ap,bp);
		else
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
	}
	else
	{
	    while (ret_val == -2)
	    {
		/* supress blanks in both strings */

		if (sb)
		{
			while (al > 0 && CMspace_SB(ap))
			{
				al -= CMbytecnt_SB(ap);
				CMnext_SB(ap);
			}

			while (bl > 0 && CMspace_SB(bp))
			{
				bl -= CMbytecnt_SB(bp);
				CMnext_SB(bp);
			}
		}


		if (al <= 0)
			ap = (unsigned char *) "";

		if (bl <= 0)
			bp = (unsigned char *) "";

		/* do inequality tests */

		if (ic)
			cmp = CMcmpnocase_SB(ap,bp);
		else
			cmp = CMcmpcase_SB(ap,bp);

		if (cmp < 0)
			ret_val = -1;
		else if (cmp > 0)
			ret_val = 1;
		else if (*ap == '\0')
			ret_val = 0;
		else
		{
			/* go on to the next character */

			al -= CMbytecnt_SB(ap);
			CMnext_SB(ap);
			bl -= CMbytecnt_SB(bp);
			CMnext_SB(bp);
		}
	    }
	}

	return(ret_val);
}
