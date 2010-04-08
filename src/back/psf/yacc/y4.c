/*
** Copyright (c) 1992, 2008 Ingres Corporation
*/
/*	@(#)y4.c	1.3	*/
# include	<compat.h>

# include	<cm.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include "dextern.h"

# define a amem
# define mem mem0
# define pa indgo
# define yypact temp1
# define greed tystate

/*
**  24-nov-92 (walt)
**	Alpha compiler complains that lkst[0].lset and wsets[0].lset are used
**	in a context that requires a constant.  Change to &lkst[0].lset and
**	&wsets[0].ws.lset.
**  04-mar-93 (smc)
**	Cast initialisation assignments to ggreed and pgo GLOBALDEFs to
**	eliminate compiler warning on axp_osf.
**  04-aug-93 (shailaja)
**	Unnested comments.
**  14-oct-93 (swm)
**	Bug #56448
**	Altered ERROR calls to conform to new portable interface.
**	It is no longer a "pseudo-varargs" function. It cannot become a
**	varargs function as this practice is outlawed in main line code.
**	Instead it takes a formatted buffer as its only parameter.
**	Each call which previously had additional arguments has an
**	STprintf embedded to format the buffer to pass to ERROR.
**	This works because STprintf is a varargs function in the CL.
**  23-sep-1996 (canor01)
**	Move global data definitions to yaccdata.c.
**  26-Jun-2006 (raddo01)
**	bug 116276: SEGV on double free() when $ING_SRC/lib/ doesn't exist.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

GLOBALREF i4     *ggreed;
GLOBALREF i4     *pgo;
GLOBALREF i4     *yypgo;

GLOBALREF i4      maxspr;		/* maximum spread of any entry */
GLOBALREF i4      maxoff;		/* maximum offset into a array */
GLOBALREF i4     *pmem;
GLOBALREF i4     *maxa;
# define NOMORE -1000

GLOBALREF i4      nxdb;
GLOBALREF i4      adb;

callopt ()
{

    register i4     i,
		    *p,
		    j,
		    k,
		    *q;
    char	    stbuf[STBUFSIZE];

 /* read the arrays from tempfile and set parameters */

    if (SIopen(&temploc, "r", &finput) != OK)
    {
	char	*errbuf;

	LOtos(&temploc, &errbuf);
	ERROR(STprintf(stbuf, "optimizer cannot open tempfile '%s'", errbuf));
    }

    pgo[0] = 0;
    yypact[0] = 0;
    nstate = 0;
    nnonter = 0;

    /*
    ** '\n' used to act a a row terminator.  Now it is just an
    ** end-of-line marker, and ';' is the row terminator.  This
    ** was done because Whitesmith's can't handle long lines.
    */

    for (;;)
    {
	switch (gtnm ())
	{
	    case ';':
		yypact[++nstate] = (--pmem) - mem;

	    case ',': 
		continue;

	    case '$': 
		break;

	    default: 
		ERROR ("bad tempfile");
	}
	break;
    }

    yypact[nstate] = yypgo[0] = (--pmem) - mem;

    for (;;)
    {
	switch (gtnm ())
	{

	    case ';': 
		yypgo[++nnonter] = pmem - mem;
	    case ',': 
		continue;

	    case EOF: 
		break;

	    default: 
		ERROR ("bad tempfile");
	}
	break;
    }

    yypgo[nnonter--] = (--pmem) - mem;



    for (i = 0; i < nstate; ++i)
    {

	k = 32000;
	j = 0;
	q = mem + yypact[i + 1];
	for (p = mem + yypact[i]; p < q; p += 2)
	{
	    if (*p > j)
		j = *p;
	    if (*p < k)
		k = *p;
	}
	if (k <= j)
	{			/* nontrivial situation */
	/* temporarily, kill this for compatibility j -= k;   j is now the
	   range */
	    if (k > maxoff)
		maxoff = k;
	}
	greed[i] = (yypact[i + 1] - yypact[i]) + 2 * j;
	if (j > maxspr)
	    maxspr = j;
    }

 /* initialize ggreed table */

    for (i = 1; i <= nnonter; ++i)
    {
	ggreed[i] = 1;
	j = 0;
    /* minimum entry index is always 0 */
	q = mem + yypgo[i + 1] - 1;
	for (p = mem + yypgo[i]; p < q; p += 2)
	{
	    ggreed[i] += 2;
	    if (*p > j)
		j = *p;
	}
	ggreed[i] = ggreed[i] + 2 * j;
	if (j > maxoff)
	    maxoff = j;
    }


 /* now, prepare to put the shift actions into the a array */

    for (i = 0; i < ACTSIZE; ++i)
	a[i] = 0;
    maxa = a;

    for (i = 0; i < nstate; ++i)
    {
	if (greed[i] == 0 && adb > 1)
	    SIfprintf (ftable, "State %d: null\n", i);
	pa[i] = YYFLAG1;
    }

    while ((i = nxti ()) != NOMORE)
    {
	if (i >= 0)
	    stin (i);
	else
	    gin (-i);

    }

    if (adb > 2)
    {				/* print a array */
	for (p = a; p <= maxa; p += 10)
	{
	    SIfprintf (ftable, "%4d  ", p - a);
	    for (i = 0; i < 10; ++i)
		SIfprintf (ftable, "%4d  ", p[i]);
	    SIfprintf (ftable, "\n");
	}
    }
 /* write out the output appropriate to the language */

    aoutput ();

    osummary ();
    SIclose(finput);
	finput = (FILE *)NULL;
    ZAPFILE (&temploc);
}

gin (i)
i4	i;
{

    register i4     *p,
		    *r,
		    *s,
		    *q1,
		    *q2;
    char	    stbuf[STBUFSIZE];

 /* enter gotos on nonterminal i into array a */

    ggreed[i] = 0;

    q2 = mem + yypgo[i + 1] - 1;
    q1 = mem + yypgo[i];

 /* now, find a place for it */

    for (p = a; p < &a[ACTSIZE]; ++p)
    {
	if (*p)
	    continue;
	for (r = q1; r < q2; r += 2)
	{
	    s = p + *r + 1;
	    if (*s)
		goto nextgp;
	    if (s > maxa)
	    {
		if ((maxa = s) > &a[ACTSIZE])
		    ERROR ("a array overflow");
	    }
	}
    /* we have found a spot */

	*p = *q2;
	if (p > maxa)
	{
	    if ((maxa = p) > &a[ACTSIZE])
		ERROR ("a array overflow");
	}
	for (r = q1; r < q2; r += 2)
	{
	    s = p + *r + 1;
	    *s = r[1];
	}

	pgo[i] = p - a;
	if (adb > 1)
	    SIfprintf (ftable, "Nonterminal %d, entry at %d\n", i, pgo[i]);
	goto nextgi;

nextgp: ;
    }

    ERROR (STprintf(stbuf, "cannot place goto %d\n", i));

nextgi: ;
}

stin (i)
i4	i;
{
    register i4     *r,
		    *s,
		    n,
		    flag,
		    j,
		    *q1,
		    *q2;
    char	    stbuf[STBUFSIZE];

    greed[i] = 0;

 /* enter state i into the a array */

    q2 = mem + yypact[i + 1];
    q1 = mem + yypact[i];
 /* find an acceptable place */

    for (n = -maxoff; n < ACTSIZE; ++n)
    {

	flag = 0;
	for (r = q1; r < q2; r += 2)
	{
	    if ((s = *r + n + a) < a)
		goto nextn;
	    if (*s == 0)
		++flag;
	    else
		if (*s != r[1])
		    goto nextn;
	}

    /* check that the position equals another only if the states are identical 
    */

	for (j = 0; j < nstate; ++j)
	{
	    if (pa[j] == n)
	    {
		if (flag)
		    goto nextn;	/* we have some disagreement */
		if (yypact[j + 1] + yypact[i] == yypact[j] + yypact[i + 1])
		{
		/* states are equal */
		    pa[i] = n;
		    if (adb > 1)
			SIfprintf (ftable,
			    "State %d: entry at %d equals state %d\n",
				i, n, j);
		    return;
		}
		goto nextn;	/* we have some disagreement */
	    }
	}

	for (r = q1; r < q2; r += 2)
	{
	    if ((s = *r + n + a) >= &a[ACTSIZE])
		ERROR ("out of space in optimizer a array");
	    if (s > maxa)
		maxa = s;
	    if (*s != 0 && *s != r[1])
		ERROR (STprintf(stbuf, "clobber of a array, pos'n %d, by %d",
		    s - a, r[1]));
	    *s = r[1];
	}
	pa[i] = n;
	if (adb > 1)
	    SIfprintf (ftable, "State %d: entry at %d\n", i, pa[i]);
	return;

nextn: 	;
    }

    ERROR (STprintf(stbuf, "Error; failure to place state %d\n", i));

}

nxti ()
{				/* finds the next i */
    register i4     i,
		    maximum,
		    maxi;

    maximum = 0;

    for (i = 1; i <= nnonter; ++i)
	if (ggreed[i] >= maximum)
	{
	    maximum = ggreed[i];
	    maxi = -i;
	}

    for (i = 0; i < nstate; ++i)
	if (greed[i] >= maximum)
	{
	    maximum = greed[i];
	    maxi = i;
	}

    if (nxdb)
	SIfprintf (ftable, "nxti = %d, max = %d\n", maxi, maximum);
    if (maximum == 0)
	return (NOMORE);
    else
	return (maxi);
}

osummary ()
{
 /* write summary */

    register i4     i,
		    *p;

    if (foutput == NULL)
	return;
    i = 0;
    for (p = maxa; p >= a; --p)
    {
	if (*p == 0)
	    ++i;
    }

    SIfprintf (foutput, "Optimizer space used: input %d/%d, output %d/%d\n",
	    pmem - mem + 1, MEMSIZE, maxa - a + 1, ACTSIZE);
    SIfprintf (foutput, "%d table entries, %d zero\n", (maxa - a) + 1, i);
    SIfprintf (foutput, "maximum spread: %d, maximum offset: %d\n", maxspr, maxoff);

}

aoutput ()
{				/* this version is for C */


 /* write out the optimized parser */

    SIfprintf (ftable, "# define YYLAST %d\n", maxa - a + 1);

    arout (YYACT, a, (maxa - a) + 1);
    arout (YYPACT, pa, nstate);
    arout (YYPGO, pgo, nnonter + 1);

}

arout (s, v, n)
i4	s;
i4	*v,
        n;
{

    register i4     i;
    register FILE   *fileptr = Filespecs[s].fileptr;

    /*
    **	Append prefix to table names after stripping off the "yy"
    **	on the front.
    */
    SIfprintf (fileptr, "GLOBALDEF const\tyytabelem %s%s[]={\n",
	prefix, Filespecs[s].optname + 2);
    if (Filespecs[s].filegiven)
    {
	SIfprintf(ftable, "extern const yytabelem %s%s[];\n",
	    prefix, Filespecs[s].optname + 2);
    }

    for (i = 0; i < n;)
    {
	if (i % 10 == 0)
	    SIfprintf (fileptr, "\n");
	SIfprintf (fileptr, "%6d", v[i]);
	if (++i == n)
	    SIfprintf (fileptr, " };\n");
	else
	    SIfprintf (fileptr, ",");
    }
}


gtnm ()
{

    register i4     s,
		    val;
    i4		    c;

 /* read and convert an integer from the standard input */
 /* return the terminating character */
 /* blanks, tabs, and newlines are ignored */

    s = 1;
    val = 0;

    while ((c = SIgetc (finput)) != EOF)
    {
	if (CMdigit (&c))
	{
	    val = val * 10 + c - '0';
	}
	else if (c == '-')
	{
	    s = -1;
	}
	else if (c == '\n')
	{
	    continue;
	}
	else
	{
	    break;
	}
    }

    *pmem++ = s * val;
    if (pmem > &mem[MEMSIZE])
	ERROR ("out of space");
    return (c);

}
