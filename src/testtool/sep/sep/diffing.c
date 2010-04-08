/*
**    Include file
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <er.h>
#include <cm.h>
#include <me.h>
#include <pc.h>
#include <termdr.h>

#define diffing_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <seperrs.h>
#include <fndefs.h>

/*
** History:
**	26-apr-1994 (donj)
**	    Created.
**	24-may-1994 (donj)
**	    Improve readability, remove a local variable.
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	07-jun-1994 (donj)
**	    Modularized diffing() function to improve readability. Created
**	    functions: Do_open_args(), Do_close_args(), Do_Skip_Canon(),
**	    Do_Failed_Match() and Do_Good_Match().
**	    Fixed problem with a silent abort when SEP confronts a missing
**	    canon. Now it reports it correctly as a missing canon.
**      07-jun-1994 (donj)
**          Fixed a comment problem.
**      05-Dec-94 (nive)
**              For dg8_us5, used the keyword __const__ in place of const,
**              as expected by gcc with -traditional flag ( note that cc
**              is gcc -traditional )
**	22-jun-95 (allmi01)
**		Added dgi_us5 support for DG-UX on Intel based platforms following dg8_us5.
**      05-feb-96 (musro02)
**              For non-dg., changed const to READONLY
**              (this could probably be done for dg. also).
**	28-aug-96 (rosga02) if VMS, do not use READONLY, it causing
**		compiling errors - invalid modifier for local storage class.
**	02-feb-97 (kamal03) VAX C precompiler doesn't honor #elif. Changed
** 		to nested #if - #else - #if
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Jan-2001 (hanje04)
**	    Shorted sleep time between diffs to 10ms from 250ms.
**	07-Apr-2001 (hanje04)
**	    Change the time spent sleeping from a fixed value to the 
**	    value of the environment variable DIFF_SLEEP. If DIFF_SLEEP
**	    is not set, the sleep time will default to the original
**	    value of 250ms. DIFF_SLEEP must be set in milliseconds.
**	25-May-2001 (hanje04)
**	    To please the honorable Mr Rogers (QA Manager Extrordinaire)
**	    change DIFF_SLEEP to SEP_DIFF_SLEEP.
**	16-Jun-2005 (bonro01)
**	    Moved some sleep() calls so that they are only called in
**	    interactive mode.
*/

/*
**  Reference global variables
*/

GLOBALREF    WINDOW       *mainW ;
GLOBALREF    WINDOW       *statusW ;

GLOBALREF    char          buffer_2 [] ;
GLOBALREF    char          msg [] ;
GLOBALREF    char         *open_args ;
GLOBALREF    char         *close_args ;
GLOBALREF    char         *open_args_array [] ;
GLOBALREF    char         *close_args_array [] ;
GLOBALREF    char         *rbanner ;
GLOBALREF    char         *rbannerend ;
GLOBALREF    char         *cmtFile ;

GLOBALREF    OPTION_LIST  *open_canon_opt ;
GLOBALREF    OPTION_LIST  *close_canon_opt ;

GLOBALREF    LOCATION     *cmtLoc ;

GLOBALREF    bool          verboseMode ;
GLOBALREF    bool          abort_if_matched ;
GLOBALREF    bool          batchMode ;
GLOBALREF    bool          updateMode ;

GLOBALREF    SEPFILE      *sepResults ;
GLOBALREF    SEPFILE      *sepGCanons ;
GLOBALREF    SEPFILE      *sepDiffer ;

GLOBALREF    FILE         *traceptr ;

GLOBALREF    i4            tracing ;
GLOBALREF    i4            verboseLevel ;
GLOBALREF    i4            diffLevel ;
GLOBALREF    i4            testLevel ;
GLOBALREF    i4            testGrade ;
GLOBALREF    i4            diff_sleep;

bool                       wrong_canon ;

/*
**	diffing
*/

STATUS
diffing( FILE *testFile, i4  prtRes, i4  getprompt )
{
    STATUS                 diffing_init ( FILE *testFile, i4  prtRes ) ;

    STATUS                 Do_open_args ( char *opn_args, char **opn_array,
					  bool *wr_canon, char **if_exp,
					  i4  prtRes ) ;

    VOID                   Do_close_args ( char *cl_args, char **cls_array,
					   i4  prtRes ) ;

    VOID                   Do_Skip_Canon ( STATUS acomment, char *opn_args,
					   char *cl_args, i4  prtRes ) ;

    STATUS                 Do_Failed_Match ( STATUS acomment, i4  prtRes,
					     i4  diffound, i4  canons ) ;

    STATUS                 Do_Good_Match ( STATUS acomment, i4  prtRes,
					   FILE *testFile, i4  canons ) ;

    STATUS                 return_val = OK ;
    STATUS                 acanon ;
    STATUS                 acomment ;
    STATUS                 matched = FAIL ;

    i4                     canons = 0 ;
    i4                     diffound = 0 ;

    char                  *if_expression ;

    bool                   append_results = TRUE ;


    acomment = diffing_init(testFile, prtRes);

    do	/* each canon */
    {
	MEtfree(SEP_ME_TAG_CANONS); /* free up stuff from previous loops. */

	if_expression       = NULL;
	wrong_canon         = FALSE;
	abort_if_matched    = FALSE;

	if ((acanon = form_canon(testFile, &canons, &open_args, &close_args))
	     == OK)
	{
	    return_val = (open_args == NULL) ? OK :
		         Do_open_args(open_args, open_args_array, &wrong_canon,
				      &if_expression, prtRes);

	    if ((close_args != NULL)&&(wrong_canon == FALSE))
	    {
		(VOID)Do_close_args(close_args, close_args_array, prtRes);
	    }

	    if (!batchMode)
	    {
		Prt_Diff_msg(wrong_canon, if_expression, canons, buffer_2,
			     prtRes, getprompt);
	    }

	    if (wrong_canon == FALSE)
	    {
		append_results = ((matched = diff_canon(&diffound)) != OK)
			       ? FALSE : TRUE;
	    }
	}
	else if ((canons == 0)&&(acanon != SKIP_CANON))
	{
	    char *c_missing = ERx("ERROR: Canon missing for this command") ;

	    if (prtRes)
	    {
		disp_line(c_missing,0,0);
	    }

	    append_line(c_missing,1);
	    testGrade = SEPerr_ABORT_canon_missing;
	    return_val = FAIL;
	}
    }
    while (((matched != OK)||(wrong_canon == TRUE))&&(acanon == OK)
	   &&(return_val == OK));

    if (!updateMode && append_results && verboseMode && (verboseLevel >= 1))
    {
	append_line(rbanner,1);
	append_sepfile(sepResults);
	append_line(rbannerend,1);
	SEPrewind(sepResults,TRUE);
    }
    else
    {
	/*
	** to solve the sep hanging problem. The symptom for this was
	** sep would hang at the very end of some of the fe tests, or
	** sometimes get out of sync with FRS.
	*/
	PCsleep(diff_sleep);
    }
    
    if (return_val == OK)
    {
	if (acanon == SKIP_CANON)
	{
	    Do_Skip_Canon(acomment, open_args, close_args, prtRes);
	}
	else
	{
	    if (updateMode)
	    {
		SEPputc(SEPENDFILE, sepGCanons);
		SEPrewind(sepGCanons, TRUE);
	    }

	    return_val = (matched == FAIL)
		       ? Do_Failed_Match(acomment, prtRes, diffound, canons)
		       : Do_Good_Match  (acomment, prtRes, testFile, canons);
	}
    }
/*
**  clean out 
*/
    if (acomment == OK)
    {
	del_floc(cmtLoc);
    }

    MEtfree(SEP_ME_TAG_CANONS);

    if (abort_if_matched == TRUE)
    {
	testGrade = SEPerr_ABORT_canon_matched;
    }

    return (return_val);

} /* End of Diffing. */

STATUS
diffing_init(FILE *testFile, i4  prtRes)
{
    if (prtRes && !batchMode)
    {
	disp_res(sepResults);
    }

    if (updateMode)
    {
	SEPrewind(sepGCanons, FALSE);
    }

    /*
    **  check if there are some comments attached to this command
    */

    return (form_comment(testFile));
}

STATUS
Prt_Diff_msg(bool wrong, char *exp, i4  cnr, char *buf, i4  prt, i4  getp)
{
    STATUS                 return_val = OK ;
#if defined(dg8_us5) || defined(dgi_us5)
    __const__ char            *SorD = wrong ? ERx("Skipping canon ")
                                        : ERx("Diffing  canon ") ;
#else
#ifdef VMS
    	      char            *SorD = wrong ? ERx("Skipping canon ")
                                        : ERx("Diffing  canon ") ;
#else
    READONLY  char            *SorD = wrong ? ERx("Skipping canon ")
                                        : ERx("Diffing  canon ") ;
#endif
#endif

    if (wrong||exp)
    {
	STprintf(buf, (SEP_CMstlen(exp,0) > 38) ? ERx("%s#%d, (%38s ...)%s")
						: ERx("%s#%d, (%s)%s"),
		      SorD, cnr, exp, wrong ? ERx(" is FALSE...")
					    : ERx(" is TRUE ..."));
    }
    else
    {
        STprintf(buf,ERx("%s#%d ..."), SorD, cnr);
    }

    if (prt)
    {
	put_message(statusW, buf);
	if (wrong)
	{                       /* pause a sec so they can read */
	     PCsleep(1000);	/* the message.			*/
	}
    }

    if (getp)
    {
	if ((prt && wrong) == FALSE)
	{
	    PCsleep(500);	/* if pause above occurred,we don't */
	}			/* need to pause here.		    */

	disp_prompt(buf,NULL,NULL);
    }

    return (return_val);
}

STATUS
Do_open_args( char *opn_args, char **opn_array, bool *wr_canon, char **if_exp,
	      i4  prtRes )
{
    char                  *temp_line = NULL ;
    STATUS                 return_val = OK ;
    i4                     open_count ;
    register i4            i ;


    temp_line = SEP_CMlastchar(opn_args,0);

    if (CMcmpcase(temp_line,ERx("\n")) == 0)
    {
	*temp_line = EOS;
    }

    open_count = SEP_CMgetwords(SEP_ME_TAG_CANONS, opn_args, WORD_ARRAY_SIZE,
				opn_array);

    SEP_cmd_line(&open_count, opn_array, open_canon_opt, ERx("-"));

    for (i = 1; (i < open_count)&&(*wr_canon == FALSE); i++)
    {
	if (STbcompare(opn_array[i], 0, ERx("IF"), 0, TRUE) == 0)
	{
	    i++;
	    *if_exp = SEP_CMpackwords(SEP_ME_TAG_CANONS, (open_count-i),
	    &opn_array[i]);
	    *wr_canon = (bool)(SEP_Eval_If(*if_exp, &return_val, msg) == FALSE);

	    if ((return_val != OK)&&(prtRes))
	    {
		disp_line(msg,0,0);
	    }
	    i = open_count;
	}
	else if (prtRes)
	{
	    disp_line(STpolycat(3, ERx("ERROR: Invalid open canon option <"),
				opn_array[i], ERx(">"), msg), 0, 0);
	}
    }

    return (return_val);
}

VOID
Do_close_args( char *cl_args, char **cls_array, i4  prtRes )
{
    register i4            i ;
    i4                     close_count ;
    char                  *temp_line = NULL ;

    
    temp_line = SEP_CMlastchar(cl_args,0);

    if (CMcmpcase(temp_line,ERx("\n")) == 0)
    {
	*temp_line = EOS;
    }

    temp_line = STtalloc(SEP_ME_TAG_CANONS, cl_args);
    close_count = SEP_CMgetwords(SEP_ME_TAG_CANONS, cl_args, WORD_ARRAY_SIZE,
				 cls_array);
    cl_args = temp_line;

    SEP_cmd_line(&close_count, cls_array, close_canon_opt, ERx("-"));

    for (i = 1; i < close_count; i++)
    {
	if (STbcompare( cls_array[i],0, ERx("ABORT"),0, TRUE) == 0)
	{
	    abort_if_matched = TRUE;
	}
	else if (prtRes)
	{
	    disp_line(STpolycat(3, ERx("ERROR: Invalid close canon option <"),
				cls_array[i], ERx(">"), msg), 0, 0);
	
	}
    }
}

VOID
Do_Skip_Canon( STATUS acomment, char *opn_args, char *cl_args, i4  prtRes )
{
    if (updateMode)
    {
	if (acomment == OK)
	{
	    append_line(OPEN_COMMENT, 1);
	    append_file(cmtFile);
	    append_line(CLOSE_COMMENT, 1);
	}
	append_line(opn_args  ? opn_args  : OPEN_CANON,  1);
	append_line(SKIP_SYMBOL,1);
	append_line(cl_args ? cl_args : CLOSE_CANON, 1);
    }
    else
    {
	char *res_not_compare = ERx("*** Results were not compared to Canons ***") ;
	append_line(res_not_compare,1);
	if (prtRes && !batchMode)
	{
	    disp_line(res_not_compare,0,0);
	}
    }
}

STATUS
Do_Failed_Match( STATUS acomment, i4  prtRes, i4  diffound, i4  canons )
{
    STATUS                 return_val = OK ;


    /* update test level */

    testcase_logdiff(diffound);
    if ((diffound >= diffLevel) && !updateMode)
    {
	testLevel--;
    }

    testGrade++;	/* keep track of commands that failed */
    if (!batchMode)	/* display diff file */
    {
	if (disp_diff(acomment,canons) != OK)
	{
	    return_val = ABORT_TEST;
	}
    }

    /* append to log file */
    if (!updateMode)
    {
	append_sepfile(sepDiffer);
    }
    
    if (prtRes && !batchMode)
    {
	TDtouchwin(mainW);
	TDrefresh(mainW);
	if (!updateMode)
	{
	    disp_line(ERx("*** Did not match any canon ***"),0,0);
	}
    }

    return (return_val);
}

STATUS
Do_Good_Match( STATUS acomment, i4  prtRes, FILE *testFile, i4  canons )
{
    STATUS                 return_val = OK ;

    if (!updateMode)
    {
	skip_canons(testFile, (bool)FALSE);
	STprintf(buffer_2,ERx("*** Matched canon # %d ***"),canons);
	append_line(buffer_2,1);
	if (prtRes && !batchMode)
	{
	    disp_line(buffer_2,0,0);
	}
    }
    else
    {
	if (acomment == OK)
	{
	    append_line(OPEN_COMMENT, 1);
	    append_file(cmtFile);
	    append_line(CLOSE_COMMENT, 1);
	}
	
	/*
	** copy all canons to UPD file
	*/
	
	/* copy canons processed so far */
	append_sepfile(sepGCanons);

	/* copy remaining canons */
	skip_canons(testFile, (bool)TRUE);
    }

    if (abort_if_matched == TRUE)
    {
	char *ctmp = ERx("*** Executing Close Canon ABORT Command ***") ;

	append_line(ctmp,1);
	if (prtRes && !batchMode)
	{
	    PCsleep(1000);
	    disp_line(ctmp,0,0);
	    PCsleep(1000);
	}
	return_val = ABORT_TEST;
    }

    return (return_val);
}
