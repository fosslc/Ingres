# include	<compat.h>
# include	<cm.h>
# include	<eqscan.h>

/*
+* Filename:	SCGETKEY.C 
** Defines:	sc_getkey
**		sc_ansikey
**		sc_compare
** Purpose:	Binary search for keyword in token table for passed user key.
**
** Parameters:	tab 	- KEY_ELM [] 	- Key table to use,
**		tabnum 	- i4		- Size of table,
**		usrkey	- char *	- String value of caller's scanned 
**					  word or operator.
-* Returns:	KEY_ELM * - Pointer to Token structure if found, otherwise Null.
**
** Requires:
**	1. The token table must be sorted alphabetically for the binary search.
**
** History:
**		17-nov-1984  -	Written (ncg)
**		29-jul-1992  -  Added new routine sc_ansikey to do a binary
**				search on an ANSI SQL2 keyword table. (teresal)
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

KEY_ELM	*
sc_getkey( tab, tabnum, usrkey )
KEY_ELM	tab[];				/* Key table being used */
i4	tabnum;				/* Total number of elements in table */
char	*usrkey;			/* User's input key */
{
    register i4	probe;
    register i4	top; 
    register i4	bot; 
    i4			comp;
    i4			sc_compare();

    bot = 0;
    top = tabnum - 1;

    do 
    {
	probe = (top + bot) / 2;
	comp = sc_compare(usrkey, tab[probe].key_term);
	if (comp == 0)
	    return  &tab[probe];
	else if (comp < 0)
	    top = probe - 1;
	else /* comp > 0 */
	    bot = probe + 1;
    }  while (bot <= top);

    return KEYNULL;
}

/*
** The sc_ansikey routine is practically the same as sc_getkey except it 
** uses a simple (char *) table rather than a token key table and
** it just returns OK or FAIL depending on if the keyword was found.
*/

STATUS
sc_ansikey( tab, tabnum, usrkey )
char	*tab[];				/* Key table being used */
i4	tabnum;				/* Total number of elements in table */
char	*usrkey;			/* User's input key */
{
    register i4	probe;
    register i4	top; 
    register i4	bot; 
    i4			comp;
    i4			sc_compare();

    bot = 0;
    top = tabnum - 1;

    do 
    {
	probe = (top + bot) / 2;
	comp = sc_compare(usrkey, tab[probe]);
	if (comp == 0)
	    return  OK;
	else if (comp < 0)
	    top = probe - 1;
	else /* comp > 0 */
	    bot = probe + 1;
    }  while (bot <= top);

    return FAIL;
}

/*
/*
+* Procedure:	sc_compare
** Purpose:	Compare strings ignoring case.
**
** Parameters:	ap, bp 	- char * - Two strings to compare.
** Returns:	i4    	- if ap (lexically) > bp	> 0
**  			- if ap (lexically) = bp	= 0
-*  			- if ap (lexically) < bp	< 0
**
** Notes:
**  1. EQMERGE uses its own copy of sc_compare, which must be able to
**     translate the backslashes.  If this file is modified to use
**     STbcompare then EQMERGE must be modified too.
**  2. Either a Null byte or a diference in strings terminates the scan.
**  3. Examples:
**		"abc"  > "ab"
**		"abc" == "abc"
**		"abc"  < "abd"
** History:
**		15-nov-1984 	- Written from original (ncg)
**		26-may-1987	- Incorporated into new scanner. (ncg)
*/

i4
sc_compare(ap, bp)
register char	*ap;
char		*bp;
{
    char	a[2];
    char	b[2];
    i4		retval;

    for (;;)
    {
	/* Do inequality tests ignoring case - use lower case */
	CMtolower(ap, a);
	CMtolower(bp, b);

	retval = CMcmpcase(a, b);
	if (retval != 0)
	    return retval;
	if (*ap == '\0')		/* retval == 0 and reached null */
	    return 0;

	/* Go on to the next character */
	CMnext(ap);
	CMnext(bp);
    }
}
