/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Test international stuff
**
NEEDLIBS = COMPAT 
**
** History:
**		2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	27-dec-94 (forky01)
**	    Added more tests for CVfa
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<cv.h>
# include	<si.h>
# include	<st.h>

main()
{
	f8	value;
	char	buf[256];
	i2	x;
	STATUS	rv1,
		rv2;

	/* Use the ingres allocator instead of malloc/free default (daveb) */
	MEadvise( ME_INGRES_ALLOC );

	STcopy("2345.4", buf);
	rv1 = CVaf(buf, '.', &value);
	rv2 = CVfa(value, 20, 3, 'f', '.', buf, &x);
	SIprintf("(.) CVaf = %d, CVfa = %d, 2345.4 = %f = %s\n", rv1, rv2, value, '.', buf);
	STcopy("2345,4", buf);
	rv1 = CVaf(buf, '.', &value);
	rv2 = CVfa(value, 20, 3, 'f', '.', buf, &x);
	SIprintf("(.) CVaf = %d, CVfa = %d, 2345,4 = %f = %s\n", rv1, rv2, value, '.', buf);
	STcopy("2345,4", buf);
	rv1 = CVaf(buf, ',', &value);
	rv2 = CVfa(value, 20, 3, 'f', ',', buf, &x);
	SIprintf("(,) CVaf = %d, CVfa = %d, 2345,4 = %f = %s\n", rv1, rv2, value, ',', buf);

	rv2 = CVfa(123.456, 10, 2, 'g', '.', buf, &x);
	SIprintf("\nCVfa(g10.2) 123.456, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(123.456, 10, 2, 'e', '.', buf, &x);
	SIprintf("CVfa(e10.2) 123.456, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(123.456, 10, 2, 'n', '.', buf, &x);
	SIprintf("CVfa(n10.2) 123.456, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(123.456, 10, 2, 'f', '.', buf, &x);
	SIprintf("CVfa(f10.2) 123.456, rc = %d, result = <%s>\n", rv2, buf);

	rv2 = CVfa(123456.0, 10, 2, 'G', '.', buf, &x);
	SIprintf("\nCVfa(G10.2) 123456, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(123456.0, 10, 2, 'E', '.', buf, &x);
	SIprintf("CVfa(E10.2) 123456, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(123456.0, 10, 2, 'N', '.', buf, &x);
	SIprintf("CVfa(N10.2) 123456, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(123456.0, 10, 2, 'F', '.', buf, &x);
	SIprintf("CVfa(F10.2) 123456, rc = %d, result = <%s>\n", rv2, buf);

	rv2 = CVfa(12345678.0, 10, 2, 'G', '.', buf, &x);
	SIprintf("\nCVfa(G10.2) 12345678, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(12345678.0, 10, 2, 'E', '.', buf, &x);
	SIprintf("CVfa(E10.2) 12345678, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(12345678.0, 10, 2, 'N', '.', buf, &x);
	SIprintf("CVfa(N10.2) 12345678, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(12345678.0, 10, 2, 'F', '.', buf, &x);
	SIprintf("CVfa(F10.2) 12345678, rc = %d, result = <%s>\n", rv2, buf);

	rv2 = CVfa(-134.65, 8, 2, 'g', '.', buf, &x);
	SIprintf("\nCVfa(g8.2) -134.65, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(-134.65, 8, 2, 'e', '.', buf, &x);
	SIprintf("CVfa(e8.2) -134.65, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(-134.65, 8, 2, 'n', '.', buf, &x);
	SIprintf("CVfa(n8.2) -134.65, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(-134.65, 8, 2, 'f', '.', buf, &x);
	SIprintf("CVfa(f8.2) -134.65, rc = %d, result = <%s>\n", rv2, buf);

	rv2 = CVfa(134.65, 8, 2, 'g', '.', buf, &x);
	SIprintf("\nCVfa(g8.2) 134.65, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(134.65, 8, 2, 'e', '.', buf, &x);
	SIprintf("CVfa(e8.2) 134.65, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(134.65, 8, 2, 'n', '.', buf, &x);
	SIprintf("CVfa(n8.2) 134.65, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(134.65, 8, 2, 'f', '.', buf, &x);
	SIprintf("CVfa(f8.2) 134.65, rc = %d, result = <%s>\n", rv2, buf);

	rv2 = CVfa(-123.0, 10, 0, 'g', '.', buf, &x);
	SIprintf("\nCVfa(g10) -123, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(-123.0, 10, 0, 'e', '.', buf, &x);
	SIprintf("CVfa(e10) -123, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(-123.0, 10, 0, 'n', '.', buf, &x);
	SIprintf("CVfa(n10) -123, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(-123.0, 10, 0, 'f', '.', buf, &x);
	SIprintf("CVfa(f10) -123, rc = %d, result = <%s>\n", rv2, buf);

	rv2 = CVfa(12.45, 10, 2, 'g', '.', buf, &x);
	SIprintf("\nCVfa(g10.2) 12.45, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(12.45, 10, 2, 'e', '.', buf, &x);
	SIprintf("CVfa(e10.2) 12.45, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(12.45, 10, 2, 'n', '.', buf, &x);
	SIprintf("CVfa(n10.2) 12.45, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(12.45, 10, 2, 'f', '.', buf, &x);
	SIprintf("CVfa(f10.2) 12.45, rc = %d, result = <%s>\n", rv2, buf);

	rv2 = CVfa(0.0008, 10, 3, 'G', '.', buf, &x);
	SIprintf("\nCVfa(G10.3) 0.0008, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(0.0008, 10, 3, 'E', '.', buf, &x);
	SIprintf("CVfa(E10.3) 0.0008, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(0.0008, 10, 3, 'N', '.', buf, &x);
	SIprintf("CVfa(N10.3) 0.0008, rc = %d, result = <%s>\n", rv2, buf);
	rv2 = CVfa(0.0008, 10, 3, 'F', '.', buf, &x);
	SIprintf("CVfa(F10.3) 0.0008, rc = %d, result = <%s>\n", rv2, buf);
	return OK;
}
