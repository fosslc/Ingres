#include <compat.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>
#include <cv.h>

#include <sepdefs.h>
#include <edit.h>
#include <token.h>

/*
** miller.c - Contains code which performs the Miller/Myers differencing
** algorithm on two token lists. This code was adapted from the code in
** spiff, whose authors in turn got it from Miller and/or Myers. If you
** want to get a comfortable understanding of what it does, read the paper
** "A File Comparison Program" by W. Miller and E.W. Myers, in "Software -
** Practice & Experience", vol. 15 number 11, November 1985, pp. 1025-1040.
** (what can I say?)
**
** History:
** 	05-oct-89 (mca)
**		Major code cleanup. Removed mallocs/frees; replaced with
**		routines which will recycle memory structures as is
**		possible. Fixed boundary-condition problems in the
**		algorithm. Reduced size of diagonal arrays - they were
**		twice as big as necessary.
**	1-dec-1989 (eduardo)
**		Added flag to report that memory limit was reached
**	30-Apr-90 (davep)
**		Added ming hint for hp8
**	11-jun-90 (rog)
**		Minor cleanup for VMS external variables.
**	13-jun-90 (rog)
**		Add register declaration.
**	13-jun-1990 (rog)
**		Fold in the contents.c of compare.c since they were only
**		called from this file.
**	09-Jul-1990 (owen)
**		Use angle brackets on sep header files in order to search
**		for them in the include search path.
**	16-aug-91 (kirke)
**		Removed ming hint for hp8.  HP has improved their optimizer
**		so we can now optimize this file.
**	30-aug-91 (donj)
**		Took out #IFDEFs for LEX_DEBUG and replaced with trace feature
**		set from 'SEPSET TRACE LEX' or 'SEPSET TRACE' commands. Also
**		added A_RETCOLUMN to compare function so that minor variations
**		in table construction (column size) don't create diffs.
**	14-jan-1992 (donj)
**	    Reformatted variable declarations to one per line for clarity.
**	    Removed smatterings of code that had been commented out long ago.
**	    Added code for numeric comparisons of integer and floating point
**	    numbers (A_NUMBER, A_FLOAT), if diff_numerics is set. Added
**	    explicit declarations for all submodules called.
**      02-apr-1992 (donj)
**          Added A_DBNAME token type.
**      04-feb-1993 (donj)
**          Included lo.h because sepdefs.h now references LOCATION type.
**	    Add cases for masking of token types: A_DESTROY, A_DROP, A_RULE,
**	    A_PROCEDURE, A_OWNER, A_USER, A_TABLE and A_COLUMN.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	15-oct-1993 (donj)
**	    Rewrite compare() function for faster execution. Remove a useless
**	    unused flag from compare(), (flags), and from G_do_miller(), (comflags).
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/
                          
GLOBALREF    FILE_TOKENS  *list1 ;
GLOBALREF    FILE_TOKENS  *list2 ;
GLOBALREF    FILE_TOKENS **tokarray1 ;
GLOBALREF    FILE_TOKENS **tokarray2 ;
GLOBALREF    E_edit       *script ;
GLOBALREF    i4            totaldiffs ;
GLOBALREF    i4            numtokens1 ;
GLOBALREF    i4            numtokens2 ;
GLOBALREF    i4           *last_d ;
GLOBALREF    bool          ignoreCase ;
GLOBALREF    bool          diff_numerics ;
GLOBALREF    f8            fuzz_factor ;

GLOBALREF    long          A_always_match ;
GLOBALREF    long          A_word_match ;

GLOBALREF    i4            tracing ;
GLOBALREF    FILE         *traceptr ;

FUNC_EXTERN  E_edit        get_edit () ;

FUNC_EXTERN  VOID          size_diag ( i4  nelem ) ;
    bool		   compare ( i4  token1, i4  token2 ) ;

E_edit
G_do_miller( i4  m, i4  n, i4  max_d, i4  *merr)
{
    register i4            row ;
    register i4            col ;
    register i4            d ;
    register i4            k ;
    register i4            origin ;
    register i4            lower ;
    register i4            upper ;
    E_edit                 new ;

    /* main program */

    *merr = 0;
    origin = m > n ? m : n;
    size_diag(2*origin + 1);

    for (row=0; row < m && row < n && compare(row+1,row+1) == 0;
	 ++row) ;

    if (tracing & TRACE_LEX)
    {
	SIfprintf(traceptr, ERx("\n\n   Inside Miller's Spiff...\n\n"));
	SIfprintf(traceptr, ERx("M=%d N=%d ROW=%d COMPARE(%d,%d)=%d\n")
			  , m, n, row, row+1, row+1
			  , compare(row+1, row+1));
    }
    last_d[origin] = row;
    script[origin] = E_NULL;
    lower = (row == m) ? origin + 1 : origin - 1;
    upper = (row == n) ? origin - 1 : origin + 1;
    if (lower > upper)
    {
	/*
	**	the files are identical
	*/
	totaldiffs = 0;
	if (tracing & TRACE_LEX)
	    SIfprintf(traceptr,ERx("--------------------\nNo Diffs\n"));

	return(E_NULL);
    }
    for (d = 1; d <= max_d; ++d)
    {
	for (k = lower; k<= upper; k+= 2)
	{
	    if ((new = get_edit()) == NULL)
	    {
		*merr = 1;
		return(E_NULL);
	    }
	    if (k == origin - d || k!= origin + d && 
		last_d[k+1] >= last_d[k-1]) 
	    {
		row = last_d[k+1]+1;
		E_setnext(new,script[k+1]);
		E_setop(new,E_DELETE);
	    }
	    else
	    {
		row = last_d[k-1];
		E_setnext(new,script[k-1]);
		E_setop(new,E_INSERT);
	    }
	    E_setl1(new,row);
	    col = row + k - origin;
	    E_setl2(new,col);
	    script[k] = new;

	    while (row < m && col < n && 
		    compare(row+1,col+1) == 0) 
	    {
		++row;
		++col;
	    }
	    last_d[k] = row;
	    if ((row == m && col == n) || (d == max_d))
	    {
		if (tracing & TRACE_LEX)
		{
		    new = script[k];
		    SIfprintf(traceptr, ERx("--------------------\n"));
		    while (new)
		    {
			SIfprintf(traceptr, ERx("OP=%s LINE1=%d LINE2=%d\n"),
				  (new->op == E_DELETE ? ERx("E_DELETE")
						       : ERx("E_INSERT")),
				  new->line1, new->line2);
			new = new->link;
		    }
		}
		new = script[k];
		totaldiffs = d;
		return(new);
	    }
	    if (row == m)
		lower = k+2;
	    if (col == n)
		upper = k-2;
	}
	--lower;
	++upper;
    }
    totaldiffs = max_d;
    if (tracing & TRACE_LEX)
    {
	new = script[k];
	SIfprintf(traceptr, ERx("--------------------\n"));
	while (new)
	{
	    SIfprintf(traceptr, ERx("OP=%s LINE1=%d LINE2=%d\n"),
		      (new->op == E_DELETE ? ERx("E_DELETE") : ERx("E_INSERT")),
		      new->line1, new->line2);
	    new = new->link;
	}
    }
    new = script[origin];
    return(script[origin]);
}

/*
** The token comparison function for the SEP differ.
**
** History:
**	##-XXX-89 (eduardo)
**		Created.
**	05-oct-89 (mca)
**		Two tokens of type UNKNOWN should not automatically be
**		counted as equal.
**	30-Nov-89 (eduardo)
**		Added support for case sensitiveness while diffing
**	13-jun-1990 (rog)
**		Minor cleanup.
**	 9-oct-2008 (wanfr01)
**		Bug 121023
**	 	A_UNKNOWN character tokens are always length 1, and should be
**		compared with that in mind.
*/

bool
compare(i4 token1, i4  token2)
{
    FILE_TOKENS           *tptr1 = tokarray1 [token1] ;
    FILE_TOKENS           *tptr2 = tokarray2 [token2] ;
    i4                     ret_val ;

    /*
    ** ******************************************************** **
    ** Do the easy comparisons first
    ** ******************************************************** **
    */
    if (token1 > numtokens1)
	return(-1);

    if (token2 > numtokens2)
	return(1);

    if (tptr1->tokenID != tptr2->tokenID)
	return(1);

    if (tptr1->tokenID & A_always_match)
	return(0);

    /*
    ** ******************************************************** **
    ** Do character comparison, it's value may be used several
    ** places.
    ** ******************************************************** **
    */
    if (tptr1->tokenID == A_UNKNOWN)
        ret_val = STbcompare(tptr1->token, 1, tptr2->token, 1, ignoreCase);
    else
        ret_val = STbcompare(tptr1->token, 0, tptr2->token, 0, ignoreCase);

    if (tptr1->tokenID & A_word_match)
	return (ret_val);

    /*
    ** ******************************************************** **
    ** Now check for Numeric comparisons.
    ** ******************************************************** **
    */
    if (tptr1->tokenID & (A_FLOAT|A_NUMBER))
    {
	STATUS             CV_err ;

        if (!diff_numerics || (ret_val == 0))
            return (ret_val);

	if (tptr1->tokenID == A_FLOAT)
	{
	    f8             float_1 ;
	    f8             float_2 ;
	    f8             diff_float ;

	    if (((CV_err = CVaf(tptr1->token,'.',&float_1)) != OK) ||
		((CV_err = CVaf(tptr2->token,'.',&float_2)) != OK))
		return(CV_err);	/* if ascii to float conversion fails, */
				/* return an error indicator.          */

	    if (float_1 == float_2)
		return(0);	/* If the two floats match exactly, no */
				/* need to worry about any fuzz_factor.*/

	    if (float_1 > float_2)
		diff_float  = (float_1 - float_2)/float_1;
	    else
		diff_float  = (float_2 - float_1)/float_1;
				/* Find difference. */

	    if (diff_float > 0)
		return(diff_float > fuzz_factor);
	    else
		return(diff_float < -fuzz_factor);
				/* Difference might be negative, so    */
				/* compare to fuzz_factor differently. */
	}
	else /* tptr1->tokenID == A_NUMBER */
	{
	    i4        long_1 ;
	    i4        long_2 ;

	    if (((CV_err = CVal(tptr1->token,&long_1)) != OK) ||
		((CV_err = CVal(tptr2->token,&long_2)) != OK))
		return(CV_err);	/* if ascii to long conversion fails,  */
				/* return an error indicator.          */

	    if (long_1 == long_2)
		return(0);
	    else
		return(1);	/* Do the numeric comparison. */
	}
    }

    /*
    ** ******************************************************** **
    ** All other types are implicitly A_word_match types
    ** ******************************************************** **
    */
    return (ret_val);
}
