/*
**Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	  <compat.h>  
#include	  <gl.h>  
#include	  <cm.h>
# include	  <st.h>

/**
** Name:	stbcompar.c -	Compatability Library String Module
**					Prefix/Case-less Comparison Routine.
** Description:
**	Contains the Compatability Library String module routine that compares
**	two strings lexically, possibly case-less and possibly upto only a
**	prefix of either string.  Defines:
**
**	STbcompare()	prefix/case-less compare of two strings.
**
** History:
**	Revision 6.1  88/07/29  wong
**	Optimized.
**
**	Revision 6.0  87/10  bobm
**	Integrate 5.0 (correct arg types to CL spec)
**	9/87 Convert to CM use for scanning strings.
**
**	Revision 2.0  83/05/13  john
**	Initial revision.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	STbcompare() -	Prefix/Case-less Compare of Two Strings.
**
** Description:
**	Compare two strings lexically.  If either length argument is non-zero,
**	only examine the strings up to a minimum prefix of either string.  (The
**	prefix should be the minimum of the length arguments and the lengths
**	of the strings.)  If both of the length arguments are zero, the strings
**	must agree in length before being equal.
**
**	Depending on the setting of argument 'nocase', case is significant in
**	the comparison.  Sense of test is a?b.
**
**	Note:  So that this routine can be efficiently implemented on all
**	architectures (using `nat's,) the maximum length of either string
**	should be no more than MAXI2 bytes each for portability.  This is
**	enforced:  Strings that differ beyond MAXI2 bytes will be equal.
**
** Inputs:
**	a	{char *}  The first string.
**	al	{nat}  The first minumum length (in characters, not bytes.)
**	b	{char *}  The second string.
**	bl	{nat}  The second minumum length (in characters, not bytes.)
**
** Returns:
**	{nat}	== 0	on match.
**		< 0	if first string is lexically less than second.
**		> 0	if second string is lexically less than first.
**
** History:
**	2-15-83   - Written (jen)
**	5-6-83   - handle case where length(a) < al, etc. (cfr)
**	5-13-83  - Changed to do full string compare if both
**		   al and bl are zero.
**	9/87 (bobm)	Convert to CM use for scanning strings
**	10/87 (bobm)	Integrate 5.0 (correct arg types to CL spec)
**	07/29/88 (jhw)  Optimized with modified algorithm for about 50%
**			improvement in performance (mostly by removing calls
**			to 'STlength()' and 'STnlength()'!!!)
**      06/19/92 (stevet)
**			Fixed B44916: STbcompare does not decrement length
**			correctly, should use CMbytedec().
**	30-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**	08-feb-93 (pearl)
**		Add #include of st.h.
**      8-jun-93 (ed)
**              Changed to use CL_PROTOTYPED
**	11-aug-93 (ed)
**	    unconditional prototypes
*/
i4
STbcompare(
	const char	*a,
	i4	al,
	const char	*b,
	i4	bl,
	i4	nc)
{
	const char	*ap = a;
	const char	*bp = b;
	register i4	length;		/* in characters, not bytes */
	register i4	result = 0;

	/*
	** If both input lengths are non-positive, then make the length very
	** large so the loop below will run until the strings do not match or
	** an EOS is found in one of the strings.  Otherwise, make the length
	** the non-zero minimum of the input lengths.
	*/

	if ( al > 0 && ( bl <= 0 || al < bl ) )
		length = al;
	else if ( bl > 0 )
		length = bl;
	else
		length = MAXI2;	/* max. `nat' on all machines */

	/*
	** Compare strings lexically with value in 'result' until finished.
	** Note that if either string reaches EOS (but not both), then 'result'
	** is non-zero.  So, the only time EOS terminates the loop is when
	** both strings reach EOS with 'result' being zero as a consequence
	** (and of course, 'length' is positive.)
	**
	**	assert (length > 0)
	*/

	while ( length > 0 &&
			(result = (nc ? CMcmpnocase(ap, bp) : CMcmpcase(ap, bp))) == 0 &&
				*ap != EOS )
	{
		CMbytedec(length,ap);	/* one less character to compare (double or single byte) */
		CMnext(ap);
		CMnext(bp);
	}

	/*
	** assert:  result != 0 ==> length > 0
	**
	**	In this case (the only one of interest since result == 0 ==> match,)
	**	a special condition may hold:  One of the strings has reached EOS,
	**  (*ap == EOS OR *bp == EOS,) and a prefix comparison was in effect,
	**  (al != 0 OR bl != 0.)   If this is true, the strings are equal.
	*/
	return ( result != 0 &&
				(( *ap != EOS && *bp != EOS ) || ( al == 0 && bl == 0 ))
			) ? result : 0;
}
