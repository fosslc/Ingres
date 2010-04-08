/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adulcol.h>
#include    "adustrcmp.h"

/*
**  FILE:   ADUPATCMP.C	- Pattern matching & local collation
**
**  Description:
**	    This file contains pattern matching routines which
**	    we modified to use local collation routines instead
**	    of standard machine code compare macros.
**	    The original pattern match code remains
**	    in adustrcmp.c
**
**  Function prototypes defined in ADULCOL.H file.
**  
**  History:
**      22-May-89 (anton)
**	    Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	17-Jul-89 (anton)
**	    bug fix in sql like pattern matching
**	14-May-90 (anton)
**	    Fix bug 21600 & 21639 sql like '%v' pattern match
**	02-Oct-90 (anton)
**	    Use precomputed collation values where possible
**	    Fix some readability
**      25-mar-92 (stevet)
**          Fixed B43320.  Pattern matching fails when AD_1LIKE_ONE ('_')
**          is used.  The problem is ad0_llkqmatch() routine does not
**          increment string pointer correctly.
**      04-jan-1993 (stevet)
**          Added function prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      10-feb-1994 (pearl)
**          Bug 47818.  In ad0_llklmatch(), always advance pointer to next
**          character in pattern string before calling ad0_llike().
**	08-July-1999 (thaal01)
**	    Bug 95904
**	    When the '%' or '_' characters are given custom collation
**	    values, the SQL LIKE operator does not work. ad0_llike()
**	    was incorrestly calling adultrans(), when the comparison
**	    should be with the raw value.
**	14-March-2000 (consi01) Bug 99608 INGSRV 1064
**          Previous change removed a call to adultrans() and replaced
**          it with a direct test of the pointer contents. A side effect
**          of the call to adultrans() was to increment a pointer which
**          is no longer being done. Fix is to reinstate call to adultrans()
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-Feb-2007 (kiria01) b117544
**	    Coalesce runs of MATCH-ONE and MATCH_ANY chars into equivalent
**	    patterns that avoid unnecessary cpu cycles.
*/
/*
**  Defines of other constants.
*/

#define		AD_BLANK_CHAR	    ((u_char) ' ')  /* A blank */
#define		AD_DASH_CHAR 	    ((u_char) '-')  /* A dash */


/*
**  Definition of static variables and forward static functions.
*/
static bool ad0_cpmchk(ADULcstate  *str,
		       u_char      *endstr);

static i4  ad0_cqmatch(ADULcstate  *pat,
		       u_char      *endpat,    
		       ADULcstate  *str,	    
		       u_char      *endstr,    
		       bool        bignore);

static i4  ad0_cpmatch(ADULcstate  *pat,
		       u_char      *endpat,    
		       ADULcstate  *str,	    
		       u_char      *endstr,    
		       bool        bignore);

static i4  ad0_clmatch(ADULcstate  *pat,
		       u_char      *endpat,    
		       ADULcstate  *str,	    
		       u_char      *endstr,    
		       bool        bignore);

static	DB_STATUS ad0_llkqmatch(ADF_CB      *adf_scb,
				ADULcstate  *sst,
				u_char      *ends,
				ADULcstate  *pst,
				u_char      *endp,
				ADULcstate  *est,
				bool        bignore,
				i4	    n,
				i4         *rcmp);

static	DB_STATUS ad0_llkpmatch(ADF_CB      *adf_scb,
				ADULcstate  *sst,
				u_char      *ends,
				ADULcstate  *pst,
				u_char      *endp,
				ADULcstate  *est,
				bool        bignore,
				i4	    n,
				i4         *rcmp);

static	DB_STATUS ad0_llklmatch(ADF_CB      *adf_scb,
				ADULcstate  *sst,
				u_char      *ends,
				ADULcstate  *pst,
				u_char      *endp,
				ADULcstate  *est,
				bool        bignore,
				i4         *rcmp);






/*
** Name: ad0_3clexc_pm() - Recursive routine used by adu_lexcomp() to compare
**		           two strings using QUEL pattern matching semantics.
**
**  Description:
**
**  History:
**      03/11/83 (lichtman) -- changed the way switch statements are
**                  handled due to a Whitesmith's C bug:
**                  characters are sign-extended to i4
**                  when switching on them.  Since the
**                  pattern matching characters now have
**                  the sign bit on, they are sign extended
**                  to negative numbers which don't compare
**                  right with the constants.  Also did
**                  same thing with if statements since
**                  those didn't seem to work either.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	13-aug-87 (thurston)
**	    Added code to allow a range with a blank or null char specifically
**	    in it to be skipped if blanks and null chars are to be ignored.
**	20-nov-87 (thurston)
**	    Major changes in order to avoid doing blank
**	    padding when pattern match characters are present (even when `char'
**	    or `varchar' are involved).  This is consistent with the LIKE
**	    operator in SQL, and makes a lot of sense.  A couple of other minor
**	    bugs came out in the wash, also; e.g. "ab" now matches "ab[]".
**	14-jul-88 (jrb)
**	    Changed for Kanji support.
**	09-dec-88 (jrb)
**	    Fixed problem where bracket pattern matching didn't work if they
**	    were in the first string passed to this routine.
**	22-May-89 (anton)
**	    Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**      04-jan-1993 (stevet)
**          Fixed wrong argument being pass to ad0_clmatch() when string1 >
*           string2 and 'case DB_PAT_LBRAC'.
*/

i4
ad0_3clexc_pm(
register ADULcstate *str1,
u_char		    *endstr1,
register ADULcstate *str2,
u_char		    *endstr2,
bool		    bpad,
bool		    bignore)
{
    u_i4	    ch1;
    u_i4	    ch2;
    i4		    stat;
    bool	    have_bpadded = FALSE;
    u_char	    blank = AD_BLANK_CHAR;


loop:
    while (adulptr(str1) < endstr1)
    {
	ch1 = adultrans(str1);

	switch (ch1/COL_MULTI)
	{
	  case DB_PAT_ONE:
	    if (have_bpadded)
		return (1);	/* must have bpadded str2 */
	    else
		return (ad0_cqmatch(str1, endstr1, str2, endstr2, bignore));

	  case DB_PAT_ANY:
	    if (have_bpadded)
		return (1);	/* must have bpadded str2 */
	    else
		return (ad0_cpmatch(str1, endstr1, str2, endstr2, bignore));

	  case DB_PAT_LBRAC:
	    if (have_bpadded)
		return (1);	/* must have bpadded str2 */
	    else
		return (ad0_clmatch(str1, endstr1, str2, endstr2, bignore));

	  case NULLCHAR:
	    if (bignore)
		break;

	    /* else, fall thru to `default:' */

	  default:
	    if (bignore && adulspace(str1))
		break;
		
            while (adulptr(str2) < endstr2)
            {
		ch2 = adultrans(str2);

		switch (ch2/COL_MULTI)
		{
		  case DB_PAT_ONE:
		    if (have_bpadded)
			return (-1);	/* must have bpadded str1 */
		    else
			return (-ad0_cqmatch(str2,
					    endstr2, str1, endstr1, bignore));

		  case DB_PAT_ANY:
		    if (have_bpadded)
			return (-1);	/* must have bpadded str1 */
		    else
			return (-ad0_cpmatch(str2,
					    endstr2, str1, endstr1, bignore));

		  case DB_PAT_LBRAC:
		    if (have_bpadded)
			return (-1);	/* must have bpadded str1 */
		    else
			return (-ad0_clmatch(str2,
					    endstr2, str1, endstr1, bignore));

		  case NULLCHAR:
		    if (bignore)
		    {
			adulnext(str2);
			continue;
		    }
		    /* else, fall thru to `default:' */

		  default:
		    if (bignore && adulspace(str2))
		    {
			adulnext(str2);
			continue;
		    }
		    
                    if ((stat = ch1 - ch2) == 0)
		    {
			adulnext(str1);
			adulnext(str2);
                        goto loop;
		    }
		    else
		    {
			return (stat);
		    }
                }
            }

	    /* string 2 is out of characters, string 1 still has some */
	    /* examine remainder of string 1 for any characters */
	    while (adulptr(str1) < endstr1)
	    {
		ch1 = adultrans(str1);
		switch (ch1/COL_MULTI)
		{
		  case DB_PAT_ONE:
		    return (1);		/* must have bpadded str2 */

		  case DB_PAT_ANY:
		    if (have_bpadded)
			return (1);	/* must have bpadded str2 */
		    else
			bpad = FALSE;
		    continue;

		  case DB_PAT_LBRAC:
		    if (have_bpadded)
			return (1);	/* must have bpadded str2 */
		    else
			return (ad0_clmatch(str1, endstr1, str2, endstr2, 
					    bignore));

		  case NULLCHAR:
		    adulnext(str1);
		    if (bignore)
			continue;
		    if (bpad  &&  !ad0_cpmchk(str1, endstr1))
			return (-1);	/* nullchar < padded blank */
		    else
			return (1);	/* str1 longer than str2 */

		  default:
		    if (adulspace(str1))
		    {
			adulnext(str1);
			if (bignore)
			    continue;
			if (bpad)
			{
			    have_bpadded = TRUE;
			    continue;	/* blank = blank */
			}
			else
			{
			    return (1);	/* str1 longer than str2 */
			}
		    }
		    else
		    {
			if (    bpad
			    &&  adulccmp(str1, (u_char *)" ") < 0
			    &&  (adulnext(str1), !ad0_cpmchk(str1, endstr1))
			   )
			    return (-1);
			else
			    return (1);	/* str1 longer than str2, or
					** ch1 greater than a blank.
					*/
		    }
		}
	    }
			
	    return (0);	    /* str1 = str2 */
        }
	adulnext(str1);
    }

    /* string 1 is out of characters */
    /* examine remainder of string 2 for any characters */
    while (adulptr(str2) < endstr2)
    {
	ch2 = adultrans(str2);
	switch (ch2/COL_MULTI)
	{
	  case DB_PAT_ONE:
	    return (-1);	/* must have bpadded str1 */

	  case DB_PAT_ANY:
	    adulnext(str2);
	    if (have_bpadded)
		return (-1);	/* must have bpadded str1 */
	    else
		bpad = FALSE;
	    continue;

	  case DB_PAT_LBRAC:
	    if (have_bpadded)
		return (-1);	/* must have bpadded str1 */
	    else
		return (-ad0_clmatch(str2,
			    endstr2, str1, endstr1, bignore));

	  case NULLCHAR:
	    adulnext(str2);
	    if (bignore)
		continue;
	    if (bpad  &&  !ad0_cpmchk(str2, endstr2))
		return (1);	/* blank > nullchar */
	    else
		return (-1);	/* str1 shorter than str2 */

	  default:
	    if (adulspace(str2))
	    {
		adulnext(str2);
		if (bignore)
		    continue;
		if (bpad)
		{
		    have_bpadded = TRUE;
		    continue;	/* blank = blank */
		}
		else
		{
		    return (-1);	/* str1 shorter than str2 */
		}
	    }
	    else
	    {
		if (   bpad
		    && adulccmp(str2, (u_char *)" ") < 0
		    && (adulnext(str2), !ad0_cpmchk(str2, endstr2))
		   )
		    return (1);
		else
		    return (-1);	/* str1 shorter than str2, or
					** ch2 greater than a blank.
					*/
	    }
	}
    }
		
    return (0);	    /* str1 = str2 */
}


/*
** Name: ad0_cqmatch()
**
**  History:
**	20-nov-87 (thurston)
**	    Created to solve the "don't do blank padding with pattern match"
**	    problem (even when `char' or `varchar' are involved).
**	14-jul-88 (jrb)
**	    Changes for Kanji support.
**      22-May-89 (anton)
**          Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
*/

static  i4
ad0_cqmatch(
ADULcstate	*pat,	    /* the string holding the pattern matching char */
u_char		*endpat,    /* pointer to end of char string */
ADULcstate	*str,	    /* the other string */
u_char		*endstr,    /* pointer to the end */
bool		bignore)    /* If TRUE, ignored blanks and null chars */
{
    adulnext(pat);

    /* find a non-blank, non-null char in str, if ignoring these */
    if (bignore)
    {
	while (adulptr(str) < endstr)
	{
	    if ((!adulspace(str)  &&  adultrans(str) != 0))
		break;

	    adulnext(str);
	}
    }
    
    if (adulptr(str) < endstr)
    {
	_VOID_ adultrans(str);
	adulnext(str);
	return (ad0_3clexc_pm(pat, endpat, str, endstr, FALSE, bignore));
    }
    else
    {
	return (1);
    }
}


/*
**  Name: ad0_cpmatch() 
**
**  History:
**      03/11/83 (lichtman) -- changed to get around Whitesmith C bug
**                  mentioned above in ad0_3clexc_pm().
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	20-nov-87 (thurston)
**	    Some changes in order to avoid doing blank
**	    padding when pattern match characters are present (even when `char'
**	    or `varchar' are involved).  This is consistent with the LIKE
**	    operator in SQL, and makes a lot of sense.
**	14-jul-88 (jrb)
**	    Changes for Kanji support.
**      22-May-89 (anton)
**          Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
*/

static  i4
ad0_cpmatch(
ADULcstate      *pat,       /* the string holding the pattern matching char */
u_char	    	*endpat,    /* pointer to end of pattern char string */
ADULcstate	*str,       /* the string to be checked */
u_char	    	*endstr,    /* pointer to end of string */
bool		bignore)    /* if TRUE, we ignore blanks and null chars */
{
    ADULcstate	    psave;
    ADULcstate	    ssave;
    u_i4	    c;
    u_i4	    d;

    adulnext(pat);

    /*
    ** Skip over blanks and null chars if requested -- This fixes a
    ** bug where "a* " would not match "aaa".  The extra space after
    ** the '*' is often put in by EQUEL programs.
    */
    while (    adulptr(pat) < endpat
	    && bignore
	    && (adulspace(pat)  ||  (c = adultrans(pat)) == 0)
	  )
    {
        adulnext(pat);
    }

    if (adulptr(pat) >= endpat)
        return  (0);    /* a match if no more chars in p */

    /*
    ** If the next character in "pat" is not another
    ** pattern matching character, then scan until
    ** first matching char and continue comparison.
    */
    c = adultrans(pat)/COL_MULTI;
    if (c != DB_PAT_ANY && c != DB_PAT_LBRAC && c != DB_PAT_ONE)
    {
        while (adulptr(str) < endstr)
        {
            d = adultrans(str)/COL_MULTI;
            if (    adulcmp(pat, str) == 0
		||  d == DB_PAT_ANY
		||  d == DB_PAT_LBRAC
		||  d == DB_PAT_ONE
	       )
            {
		STRUCT_ASSIGN_MACRO(*pat, psave);
		STRUCT_ASSIGN_MACRO(*str, ssave);
                if (ad0_3clexc_pm(&psave, endpat, &ssave, endstr,
				  FALSE, bignore) == 0)
                    return (0);
            }
            adulnext(str);
        }
    }
    else
    {
        while (adulptr(str) < endstr)
	{
	    STRUCT_ASSIGN_MACRO(*pat, psave);
	    STRUCT_ASSIGN_MACRO(*str, ssave);
            if (ad0_3clexc_pm(&psave, endpat, &ssave, endstr,
			      FALSE, bignore) == 0)
                return (0); /* match */
	    _VOID_ adultrans(str);
	    adulnext(str);
	}
    }
    return (-1);    /* no match */
}


/*
** Name: ad0_clmatch()
**
**  History:
**      05/23/85 (lichtman) -- fixed bug #5445.  For some reason, ad0_1lexc_pm()
**                  was called with last parameter 0 instead of bignore.
**                  This caused [] type pattern matching not to work
**                  on C type strings with trailing blanks.
**      12/17/85 (eda) --   change if statement that included logical operators
**                  in conjunction with *p++.  If the early part of the
**                  test failed, p++ was not happening. (see K&R p.190)
**
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	13-aug-87 (thurston)
**	    Fixed bug for the following compare:
**
**		"[ ab]c" = "  c      "
**		\______/   \_________/
**	           |            |
**		  text          c
**
**	    In this type of compare, blanks and null chars are to be ignored,
**	    so the compare should return TRUE.  It was not.  It does now.
**	20-nov-87 (thurston)
**	    Some changes in order to avoid doing blank
**	    padding when pattern match characters are present (even when `char'
**	    or `varchar' are involved).  This is consistent with the LIKE
**	    operator in SQL, and makes a lot of sense.
**	14-jul-88 (jrb)
**	    Changes for Kanji support.
**      22-May-89 (anton)
**          Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	11-Jun-2008 (kiria01) b120498
**	    Corrected sense of range compare.
*/

static  i4
ad0_clmatch(
ADULcstate	*pat,	    /* the string holding the pattern matching char */
u_char		*endpat,    /* end of the pattern string */
ADULcstate	*str,	    /* the other string */
u_char		*endstr,    /* end of the string */
bool		bignore)    /* If TRUE, ignore blanks and null chars */
{
    i4			oldc;
    i4			found;
    i4			bfound;
    i4			ret_val = -1;
    auto ADULcstate	sst, pst;

    oldc = 0;

    adulnext(pat);

    /* Look for an empty range, if found, just ignore it */
    while (adulptr(pat) < endpat)
    {
	if (adultrans(pat)/COL_MULTI == DB_PAT_RBRAC)
	{
	    /* empty range, ignore it */
	    adulnext(pat);
	    return (ad0_3clexc_pm(pat, endpat, str, endstr, FALSE, bignore));
	}
	else
	{
	    if (bignore && (adulspace(pat)  || adultrans(pat) == 0))
		adulnext(pat);
	    else
		break;
	}
    }


    /* find a non-blank, non-null char in s, if ignoring these */
    while (adulptr(str) < endstr)
    {
	if (bignore && (adulspace(str)  || adultrans(str) == 0))
	{
	    adulnext(str);
	    continue;
	}

	/* search for a match on 'c' */
	found = 0;	/* assume failure */
	bfound = 0;	/* assume failure */

	while (adulptr(pat) < endpat)
	{
	    switch(adultrans(pat)/COL_MULTI)
	    {
	      case DB_PAT_RBRAC:
		{
		    adulnext(pat);
		    if (bfound)
		    {   /*
			** Since we found a blank or null char in the pattern
			** range, and blanks and null chars are being ignored,
			** try this first to see if the two are equal by
			** ignoring the range altogether.
			*/
			STRUCT_ASSIGN_MACRO(*pat, pst);
			STRUCT_ASSIGN_MACRO(*str, sst);
			ret_val = ad0_3clexc_pm(&pst, endpat,
						&sst, endstr, FALSE, bignore);
		    }

		    if (ret_val != 0  && found)
		    {
			adulnext(str);
			ret_val = ad0_3clexc_pm(pat, endpat,
						str, endstr, FALSE, bignore);
		    }
		    return (ret_val);
		}

	      case AD_DASH_CHAR:
		if (oldc == 0 || (adulnext(pat), adulptr(pat) >= endpat))
		    return (-1);    /* not found ... really an error */
	    
		if (oldc - adultrans(str) <= 0 && adulcmp(pat, str) >= 0)
		    found++;
		break;

	      default:
		oldc = adultrans(pat);

		if (bignore && (adulspace(pat) || oldc == 0))
		    bfound++;

		if (adultrans(str) == oldc)
		    found++;
	    }
	    adulnext(pat);
	}
	return (-1);    /* no match ... this should actually be an error,
			** because if we reach here, we would have an LBRAC
			** without an associated RBRAC.
			*/
    }
    return (1);
}


/*
** Name: ad0_pmchk() - Returns TRUE if string has PM chars, FALSE otherwise.
**
**  History:
**	20-nov-87 (thurston)
**	    Created to solve the "don't do blank padding with pattern match"
**	    problem (even when `char' or `varchar' are involved).
**	15-jul-88 (jrb)
**	    Changes for Kanji support.
**      22-May-89 (anton)
**          Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
*/

static  bool
ad0_cpmchk(
ADULcstate	*str,	    /* string to look for PM chars in */
u_char		*endstr)    /* end of str */
{
    ADULcstate	st;

    STRUCT_ASSIGN_MACRO(*str, st);

    while (adulptr(&st) < endstr)
    {
	switch (adultrans(&st)/COL_MULTI)
	{
	  case DB_PAT_ONE:
	  case DB_PAT_ANY:
	  case DB_PAT_LBRAC:	/* Don't want to look for RBRAC */
	    return (TRUE);

	  default:
	    adulnext(&st);
	    break;
	}
    }
    
    return (FALSE);
}

/*
** Name: ad0_like()
**
**  Description:
**	Main routine for supporting SQL's like operator.
**
**  History:
**      01-oct-86 (thurston)
**	    Initial creation.
**	14-sep-87 (thurston)
**	    Fixed bug that caused a '_' at the end of the pattern to match TWO
**	    characters.
**	06-jul-88 (thurston)
**	    Added support for the escape character, and the new `[ ... ]' type
**	    pattern matching allowed by INGRES.  This involved re-writing the
**	    entire algorithm, including the new static routines ad0_lkqmatch(),
**	    ad0_lkpmatch() (replaces the former ad0_lkmatch()), and
**	    ad0_lklmatch().
**	15-jul-88 (jrb)
**	    Changes for Kanji support.
**	24-aug-88 (thurston)
**	    Commented out a `"should *NEVER* get here" return with error' since
**	    the Sun compiler produced a warning that you could never get there.
**      22-May-89 (anton)
**          Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**      08-July-1999 (thaal01) Bug 95904
**	    When the '%' or '_' characters are give custom collation
**	    values, the SQL LIKE operator does not work. ad0_llike()
**	    was incorrectly calling adultrans() to get the collation
**	    value, when the comparison should be with the raw value.
**	14-March-2000 (consi01) Bug 99608 INGSRV 1064
**          Previous change removed a call to adultrans() and replaced
**          it with a direct test of the pointer contents. A side effect
**          of the call to adultrans() was to increment a pointer which
**          is no longer being done. Fix is to reinstate call to adultrans()
**	11-Jun-2008 (kiria01) b120328
**	    Correct '_' run handling character advance.
*/

DB_STATUS
ad0_llike(
ADF_CB		    *adf_scb,
register ADULcstate *sst,
u_char		    *ends,
register ADULcstate *pst,
u_char		    *endp,
ADULcstate	    *est,
bool		    bignore,
i4		    *rcmp)
{
    i4			cc;	/* the `character class' for pch */
    i4			stat;
    u_char		match;  /* the untranslated character */

    for (;;)	/* loop through pattern string */
    {
	int count = 0;
	DB_STATUS (*llkmatch)(ADF_CB *,ADULcstate *,u_char*,ADULcstate*,u_char*,
	                      ADULcstate*,bool,i4,i4*) = ad0_llkpmatch;
	/*
	** Get the next character from the pattern string,
	** handling escape sequences, and ignoring blanks if required.
	** -----------------------------------------------------------
	*/
	if (adulptr(pst) >= endp)
	{
	    /* end of pattern string */
	    cc = AD_CC6_EOS;
	}
	else
	{
	    if (est != NULL && !adulcmp(pst, est))
	    {
		adulnext(pst);
		/* we have an escape sequence */
		if (adulptr(pst) >= endp)
		{
		    /* ERROR:  escape at end of pattern string not allowed */
		    return (adu_error(adf_scb, E_AD1017_ESC_AT_ENDSTR, 0));
		}
		match = *(pst->lstr);
		_VOID_ adultrans(pst); /* increment pointer, ignore return */
		switch (match) /* the RAW character value */
		{
		  case AD_1LIKE_ONE:
		  case AD_2LIKE_ANY:
		  case '-':
		    cc = AD_CC0_NORMAL;
		    break;

		  case AD_3LIKE_LBRAC:
		    cc = AD_CC4_LBRAC;
		    break;

		  case AD_4LIKE_RBRAC:
		    cc = AD_CC5_RBRAC;
		    break;

		  default:
		    if (!adulcmp(pst, est))
			cc = AD_CC0_NORMAL;
		    else
			/* ERROR:  illegal escape sequence */
			return (adu_error(adf_scb, E_AD1018_BAD_ESC_SEQ, 0));
		    break;
		}
	    }
	    else
	    {
		/* not an escape character */
		match = *(pst->lstr);
		_VOID_ adultrans(pst); /* increment pointer, ignore return */
		switch (match) /* the RAW character value */
		{
		  case AD_1LIKE_ONE:
		    cc = AD_CC2_ONE;
		    break;

		  case AD_2LIKE_ANY:
		    cc = AD_CC3_ANY;
		    break;

		  case '-':
		    cc = AD_CC1_DASH;
		    break;

		  case AD_3LIKE_LBRAC:
		  case AD_4LIKE_RBRAC:
		  default:
		    cc = AD_CC0_NORMAL;
		    break;
		}
	    }
	}

	if (	bignore
	    &&	cc == AD_CC0_NORMAL
	    &&	(adulspace(pst)  || adulisnull(pst))
	   )
	{
	    adulnext(pst);
	    continue;	/* ignore blanks and null chars for the C datatype */
	}


	/* Now we have the next pattern string character and its class */
	/* ----------------------------------------------------------- */

	switch (cc)
	{
	  case AD_CC0_NORMAL:
	  case AD_CC1_DASH:
	    for (;;)
	    {
		if (adulptr(sst) >= ends)
		{
		    *rcmp = -1;	/* string is shorter than pattern */
		    return (E_DB_OK);
		}
		if (!bignore  ||  (!adulspace(sst)  && !adulisnull(sst)))
		    break;
		    
		adulnext(sst);
	    }

	    if ((stat = adulcmp(sst, pst)) != 0)
	    {
		*rcmp = stat;
		return (E_DB_OK);
	    }
	    break;

	  case AD_CC2_ONE:
		count = 1;
		adulnext(pst);
		llkmatch = ad0_llkqmatch;
		/*FALLTHROUGH*/
	  case AD_CC3_ANY:
	    while (adulptr(pst) < endp)
	    {
		match = *(pst->lstr);
		if (match == AD_1LIKE_ONE)
		{
		    _VOID_ adultrans(pst); /* increment pointer, ignore return */
		    count++;
		}
		else if (match == AD_2LIKE_ANY)
		{
		    _VOID_ adultrans(pst); /* increment pointer, ignore return */
		    llkmatch = ad0_llkpmatch;
		}
		else if (!bignore || !adulspace(pst) && !adulisnull(pst))
		    break;
		adulnext(pst);
	    }
	    return llkmatch(adf_scb, sst, ends, pst, endp,
					est, bignore, count, rcmp);

	  case AD_CC4_LBRAC:
	    return (ad0_llklmatch(adf_scb, sst, ends, pst, endp,
					est, bignore, rcmp));

	  case AD_CC5_RBRAC:
	    /*
	    ** ERROR:  bad range specification.
	    */
	    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

	  case AD_CC6_EOS:
	    /*
	    ** End of pattern string.  Check for rest of other string.
	    */
	    while (adulptr(sst) < ends)
	    {
		if (!bignore  ||  (!adulspace(sst)  && !adulisnull(sst)))
		{
		    *rcmp = 1;	    /* string is longer than pattern */
		    return (E_DB_OK);
		}
		adulnext(sst);
	    }
	    *rcmp = 0;
	    return (E_DB_OK);

	  default:
	    /*
	    ** ERROR:  should *NEVER* get here.
	    */
	    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	}
	adulnext(pst);
    	adulnext(sst);

    }
}


/*
**  Name: ad0_lkqmatch()
**
**  Description:
**	Process 'n' MATCH-ONE characters for LIKE operator.
**
**  History:
**	06-jul-88 (thurston)
**	    Inital creation and coding.
**	18-jul-88 (jrb)
**	    Conversion for Kanji support.
**      22-May-89 (anton)
**          Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**      25-mar-92 (stevet)
**          Fixed B43320.  The string pointer sst is not incrementing,
**          which cause incorrect result.
**	11-Jun-2008 (kiria01) b120328
**	    Correct '_' run handling character advance.
*/

static	DB_STATUS
ad0_llkqmatch(
ADF_CB		    *adf_scb,
register ADULcstate *sst,
u_char		    *ends,
register ADULcstate *pst,
u_char		    *endp,
ADULcstate	    *est,
bool		    bignore,
i4		    n,
i4		    *rcmp)
{
    adulnext(sst);

    while (adulptr(sst) < ends)
    {
	if (bignore  &&  (adulspace(sst)  || adulisnull(sst)))
	{
	    _VOID_ adultrans(sst);
	    adulnext(sst);
	}
	else
	{
	    _VOID_ adultrans(sst);
	    adulnext(sst);
	    if (!--n)
		return (ad0_llike(adf_scb, sst, ends, pst, endp, est,
							bignore, rcmp));
	}
    }
    *rcmp = -1;	/* string is shorter than pattern */
    return (E_DB_OK);
}


/*
**  Name: ad0_lkpmatch() 
**
**  Description:
**	Process a 'n' MATCH-ONE and a MATCH-ANY character for LIKE operator.
**
**  History:
**	06-jul-88 (thurston)
**	    Inital creation and coding.
**	18-jul-88 (jrb)
**	    Conversion for Kanji support.
**      22-May-89 (anton)
**          Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**	17-Jul-89 (anton)
**	    Fix pattern match bug
**	14-May-90 (anton)
**	    Fix bug 21600 & 21639 sql like '%v' pattern match
**	11-Jun-2008 (kiria01) b120328
**	    Correct end of '_' run handling.
*/

static	DB_STATUS
ad0_llkpmatch(
ADF_CB		    *adf_scb,
register ADULcstate *sst,
u_char		    *ends,
register ADULcstate *pst,
u_char		    *endp,
ADULcstate	    *est,
bool		    bignore,
i4		    n,
i4		    *rcmp)
{
    DB_STATUS		db_stat;
    ADULcstate		nsst, npst;

    adulnext(sst);

    while (n && adulptr(sst) < ends)
    {
	if (!bignore ||  !adulspace(sst) && !adulisnull(sst))
	    n--;
	_VOID_ adultrans(sst);
	adulnext(sst);
    }

    if (n > 0)
    {
	/* String too short for pattern */
	*rcmp = -1;
	return (E_DB_OK);
    }

    if (adulptr(pst) >= endp)
    {
	/* Last relevant char in pattern was `MATCH-ANY' so we have a match. */
	*rcmp = 0;
	return (E_DB_OK);
    }


    while (adulptr(sst) <= ends)	    /* must be `<=', not just `<' */
    {
	STRUCT_ASSIGN_MACRO(*sst, nsst);
	STRUCT_ASSIGN_MACRO(*pst, npst);

	if (db_stat = ad0_llike(adf_scb, &nsst, ends, &npst, endp,
				    est, bignore, rcmp)
	   )
	    return (db_stat);

	if (*rcmp == 0)
	    return (E_DB_OK);	/* match */

	_VOID_ adultrans(sst);
	adulnext(sst);
    }

    /* Finished with string and no match found ... call it `<' by convention. */
    *rcmp = -1;
    return (E_DB_OK);
}


/*
**  Name: ad0_lklmatch() 
**
**  Description:
**	Process a range-sequence for LIKE operator.
**
**  History:
**	06-jul-88 (thurston)
**	    Inital creation and coding.
**	18-jul-88 (jrb)
**	    Conversion for Kanji support.
**	24-aug-88 (thurston)
**	    Commented out a `"should *NEVER* get here" return with error' since
**	    the Sun compiler produced a warning that you could never get there.
**      22-May-89 (anton)
**          Copied and modified from adustrcmp.c
**	17-Jun-89 (anton)
**	    Moved local collation routines to ADU from CL
**      10-feb-1994 (pearl)
**          Bug 47818.  In ad0_llklmatch(), always advance pointer to next
**          character in pattern string before calling ad0_llike().
*/

static	DB_STATUS
ad0_llklmatch(
ADF_CB		    *adf_scb,
register ADULcstate *sst,
u_char		    *ends,
register ADULcstate *pst,
u_char		    *endp,
ADULcstate	    *est,
bool		    bignore,
i4		    *rcmp)
{
    ADULcstate		savep;
    i4			cc;
    i4			cur_state = AD_S1_IN_RANGE_DASH_NORM;
    bool		empty_range = TRUE;
    bool		match_found = FALSE;

    adulnext(pst);
    for (;;)
    {
	/*
	** Get the next character from the pattern string,
	** handling escape sequences, and ignoring blanks if required.
	** -----------------------------------------------------------
	*/
	if (adulptr(pst) >= endp)
	{
	    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));
	}
	else
	{
	    if (est != NULL  && !adulcmp(pst, est))
	    {
		/* we have an escape sequence */
		adulnext(pst);
		if (adulptr(pst) >= endp)
		{
		    /* ERROR:  escape at end of pattern string not allowed */
		    return (adu_error(adf_scb, E_AD1017_ESC_AT_ENDSTR, 0));
		}
		switch (adultrans(pst)/COL_MULTI)
		{
		  case AD_1LIKE_ONE:
		  case AD_2LIKE_ANY:
		  case '-':
		    cc = AD_CC0_NORMAL;
		    break;

		  case AD_3LIKE_LBRAC:
		    cc = AD_CC4_LBRAC;
		    break;

		  case AD_4LIKE_RBRAC:
		    cc = AD_CC5_RBRAC;
		    break;

		  default:
		    if (!adulcmp(pst, est))
			cc = AD_CC0_NORMAL;
		    else
			/* ERROR:  illegal escape sequence */
			return (adu_error(adf_scb, E_AD1018_BAD_ESC_SEQ, 0));
		    break;
		}
	    }
	    else
	    {
		/* not an escape character */
		switch (adultrans(pst)/COL_MULTI)
		{
		  case AD_1LIKE_ONE:
		    cc = AD_CC2_ONE;
		    break;

		  case AD_2LIKE_ANY:
		    cc = AD_CC3_ANY;
		    break;

		  case '-':
		    cc = AD_CC1_DASH;
		    break;

		  case AD_3LIKE_LBRAC:
		  case AD_4LIKE_RBRAC:
		  default:
		    cc = AD_CC0_NORMAL;
		    break;
		}
	    }
	}

	if (cc == AD_CC6_EOS)
	    return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

	if (cc == AD_CC2_ONE  ||  cc == AD_CC3_ANY  ||  cc == AD_CC4_LBRAC)
	    return (adu_error(adf_scb, E_AD1016_PMCHARS_IN_RANGE, 0));

	if (	bignore
	    &&	cc == AD_CC0_NORMAL
	    &&	(adulspace(pst)  || adulisnull(pst))
	   )
	{
	    adulnext(pst);
	    continue;	/* ignore blanks and null chars for the C datatype */
	}


	/*
	** Now, we have the next pattern character.  Switch on the current
	** state, then do something depending on what character class
	** the next pattern character falls in.  Note, that we should be
	** guaranteed at this point to have either `NORMAL', `DASH', or `RBRAC'.
	** All other cases have been handled above as error conditions.
	*/

	switch (cur_state)
	{
	  case AD_S1_IN_RANGE_DASH_NORM:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
	      case AD_CC1_DASH:
		empty_range = FALSE;
		STRUCT_ASSIGN_MACRO(*pst, savep);
		cur_state = AD_S2_IN_RANGE_DASH_IS_OK;
	        break;

	      case AD_CC5_RBRAC:
		/* end of the range spec, range *MUST* have been empty. */
		adulnext(pst);
		return (ad0_llike(adf_scb, sst, ends, pst, endp,
					    est, bignore, rcmp));

	      default:
		/* should *NEVER* get here */
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	    break;

	  case AD_S2_IN_RANGE_DASH_IS_OK:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		if (adulptr(sst) < ends && adulcmp(sst, &savep) == 0)
		    match_found = TRUE;
		STRUCT_ASSIGN_MACRO(*pst, savep);
		/* cur_state remains AD_S2_IN_RANGE_DASH_IS_OK */
	        break;

	      case AD_CC1_DASH:
		/* do nothing, but change cur_state */
		cur_state = AD_S3_IN_RANGE_AFTER_DASH;
	        break;

	      case AD_CC5_RBRAC:
		/* end of the range spec, and we have a saved char so range */
		/* was not empty.					    */
		if (adulptr(sst) < ends && adulcmp(sst, &savep) == 0)
		    match_found = TRUE;

		if (match_found)
		{
		    adulnext(sst);
		    adulnext(pst);
		    return (ad0_llike(adf_scb, sst, ends, pst, endp,
					    est, bignore, rcmp));
		}
		*rcmp = -1;	/* if string not in range, call it < pat */
		return (E_DB_OK);

	      default:
		/* should *NEVER* get here */
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	    break;

	  case AD_S3_IN_RANGE_AFTER_DASH:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		if (adulcmp(&savep, pst) <= 0)
		{
		    if (    adulptr(sst) < ends
			&&  adulcmp(sst, &savep) >= 0
		        &&  adulcmp(sst, pst) <= 0
		       )
		    {
			match_found = TRUE;
		    }
		    cur_state = AD_S4_IN_RANGE_DASH_NOT_OK;
		    break;
		}
		/* fall through to BAD-RANGE ... x-y, where x > y */

	      case AD_CC1_DASH:
	      case AD_CC5_RBRAC:
		return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

	      default:
		/* should *NEVER* get here */
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	    break;

	  case AD_S4_IN_RANGE_DASH_NOT_OK:
	    switch (cc)
	    {
	      case AD_CC0_NORMAL:
		STRUCT_ASSIGN_MACRO(*pst, savep);
		cur_state = AD_S2_IN_RANGE_DASH_IS_OK;
	        break;

	      case AD_CC1_DASH:
		return (adu_error(adf_scb, E_AD1015_BAD_RANGE, 0));

	      case AD_CC5_RBRAC:
		/* end of the range spec, no saved char, and range was not  */
		/* empty.						    */
		if (match_found)
		{
		    adulnext(sst);
		    adulnext(pst);
		    return (ad0_llike(adf_scb, sst, ends, pst, endp,
					    est, bignore, rcmp));
		}
		*rcmp = -1;	/* if string not in range, call it < pat */
		return (E_DB_OK);

	      default:
		/* should *NEVER* get here */
		return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	    }
	    break;

	  default:
	    /* should *NEVER* get here */
	    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	}
	adulnext(pst);
    }
}
