/*
** output.c - contains the routine which takes a list of token-level edits
** and generates appropriate output in the format of diff(1). Adapted from
** code in spiff.
**
** History:
**	05-oct-89 (mca)
**		Tossed about 50% of this file, which was never being used.
**		Fixed the algorithm for determining whether a change is an
**		add, a delete or a change - now, if a line is changed by
**		adding or deleting symbols (as opposed to changing symbols
**		within the line) the difference will be correctly reported
**		as a change, not as an add or delete.
**	late-Oct-1989 (eduardo)
**		added support for SEPFILES & took out blinking
**	16-nov-89 (mca)
**		In K_getline(), if token number is invalid, pass back a null
**		token pointer as well as a 0 line number. Some routines that
**		call K_getline don't check the line number.
**	22-dec-89 (rog)
**		Fixed highlighting of canon in diff output.  The token after
**		the first differing token was highlighted instead of the first
**		differing token.
**	30-Apr-90 (davep)
**		Added ming hint for hp8
**	18-may-1990 (rog)
**		Renamed sepDiffer to diffFile to avoid conflicts with global
**		variables.
**	13-jun-1990 (rog)
**		Minor cleanup and register declarations.
**	13-jun-1990 (rog)
**		More performance improvements.
**	21-jun-1990 (rog)
**		Can't take the address of register variables.
**	16-aug-91 (kirke)
**		Removed ming hint for hp8.  HP has improved their optimizer
**		so we can now optimize this file.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Simplified 
**	    and clarified some control loops.
**      04-feb-1993 (donj)
**          Included lo.h because sepdefs.h now references LOCATION type.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**	 7-may-1993 (donj)
**	    K_getline() needs to be a (i4) to return a value.
**	20-aug-1993 (donj)
**	    K_getline() had it's two first args switched in prototype
**	    mode.
**      26-aug-1993 (donj)
**          Change STprintf() to IISTprintf(), STmove() to IISTmove() and
**          MEcopy() to IIMEcopy() due to changes in CL.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-sep-2000 (mcgem01)
**          More nat and longnat to i4 changes.
*/
#include <compat.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>

#include <sepdefs.h>
#include <sepfiles.h>
#include <flagdefs.h>
#include <edit.h>
#include <token.h>

#define	_O_APP		1
#define _O_DEL		2
#define _O_CHA		3
#define _O_TYPE_E	4

#define ON_BLINKING	ERx("\33[5m")
#define OFF_BLINKING	ERx("\33[m")

static i4                  _O_need_init = 1 ;
static i4                  _O_st_ok = 0 ;
static i4                  _O_doing_ul = 0 ;
static char               *_O_st_tmp ;

/*
** Ed
*/
#define Z_WORDLEN	4
#define Z_LINELEN	4

GLOBALREF    FILE_TOKENS  *list1 ;
GLOBALREF    FILE_TOKENS  *list2 ;
GLOBALREF    FILE_TOKENS **tokarray1 ;
GLOBALREF    FILE_TOKENS **tokarray2 ;
GLOBALREF    i4            numtokens1 ;
GLOBALREF    i4            numtokens2 ;

FUNC_EXTERN  VOID          free_edits ( E_edit edit_list ) ;


char                       diffBuff [SCR_LINE] ;
char                       diffOut [TEST_LINE + 1] ;

static VOID
_O_do_lines(i4 start,i4 end,i4 file,SEPFILE	*diffFile)
{
    FILE_TOKENS           *temp = NULL ;
    FILE_TOKENS           *temp1 = NULL ;
    register i4            lineno = 0 ;
/*
**  Eduardo 10/23/89 to fix printing of result file
*/
    if (file && !start)
	start++;

/*
**  Eduardo 11/29/89; Added initialization
*/
    temp = temp1 = NULL;

    K_getline(file,start,&temp);
    K_getline(file,end,&temp1);

    if (temp1)
    {
	while (temp && temp->lineno <= temp1->lineno)
	{
	    if (lineno != temp->lineno)
	    {
		lineno = temp->lineno;
		IISTmove(diffBuff, ' ', TEST_LINE, diffOut);
		diffOut[TEST_LINE] = '\0';
		SEPputrec(diffOut, diffFile);
		STcopy(((0 == file) ? ERx("< ") : ERx("> ")), diffBuff);
	    }

/*
**	Changed by Eduardo 10/23/89
**	not blinking anymore
**
**	    if (temp->token_no == start+1 || temp->token_no == end+1)
**		SIfprintf(outfile,ERx("%s%s%s "),ON_BLINKING,temp->token,OFF_BLINKING);
*/
	    STpolycat(3, diffBuff, temp->token, ERx(" "), diffBuff);
	    temp = temp->next;
	}
    }

    IISTmove(diffBuff, ' ', TEST_LINE, diffOut);
    diffOut[TEST_LINE] = '\0';
    SEPputrec(diffOut, diffFile);
}

VOID
O_output(E_edit start,i4 flags,i4 no_tokens,SEPFILE	*diffFile)
{
    register i4            t_beg1 ; /* token numbers */
    register i4            t_beg2 ;
    register i4            t_end1 ;
    register i4            t_end2 ;
    i4                     type ;
    i4                     first_line ;
    i4                     last_line ;
    register E_edit        ep ;
    register E_edit        ahead ;
    E_edit                 behind ;
    E_edit                 first_edit ;
    E_edit                 last_edit ;
    E_edit                 prev_edit ;
    /*
    **	print diff header
    */
    IISTprintf(diffBuff,ERx("(%d) ====  DIFF FILE  ===="),no_tokens);
    IISTmove(diffBuff,' ',TEST_LINE,diffOut);
    diffOut[TEST_LINE] = '\0';
    SEPputrec(diffOut, diffFile);
    /*
    **	set token numbers intentionally out of range
    **		as boilerplate
    */
    t_beg1 = t_beg2 = t_end1 = t_end2 = -1;
    /*
    **	reverse the list of edits
    */
    ep = E_NULL;
    for (ahead = start; ahead != E_NULL; )
    {
    	/* Edit script is 1 origin. All of our routines are zero origin. */

	behind = ep;
	ep = ahead;
	ahead = E_getnext(ahead);
	E_setnext(ep,behind);
    }
    /*
    **	now run down the list and collect the following information
    **	type of change (_O_APP, _O_DEL or _O_CHA)
    **	start and length for each file
    */
    for (ahead = ep; ep != E_NULL; )
    {
	register i4        first1 ;
	register i4        last1 ;
	register i4        first2 ;
	register i4        last2 ;
	char               tempBuff [24] ;

	first_edit = ep;
	if (E_getop(first_edit) == E_INSERT)
	    prev_edit = first_edit;
	else
	{
	    do
	    {
		prev_edit = ep;
		ep = E_getnext(ep);
	    } while ((ep != NULL) && (E_getop(ep) == E_DELETE)
		     && (E_getl1(ep) == E_getl1(prev_edit)+1));
	}
	while ((ep != NULL) && (E_getop(ep) == E_INSERT)
		&& (E_getl1(ep) == E_getl1(prev_edit)))
	{
	    prev_edit = ep;
	    ep = E_getnext(ep);
	}
	last_edit = prev_edit;
	type = _O_CHA;
	first_line = E_getl1(first_edit);
	last_line = E_getl1(last_edit);
	if ( (numtokens2 == 0) || ( (E_getop(first_edit) == E_DELETE)
	     && (E_getop(last_edit) == E_DELETE)
	     && ( K_getline(0, first_line, NULL)
	         != K_getline(0, first_line-1, NULL) )
	     && ( K_getline(0, last_line, NULL)
	          != K_getline(0, last_line+1, NULL)) ) )
	    type = _O_DEL;
	else
	{
	    first_line = E_getl2(first_edit);
	    last_line = E_getl2(last_edit);
	    if ((numtokens1 == 0) || 
	        ((E_getop(first_edit) == E_INSERT)
		&& (E_getop(last_edit) == E_INSERT)
		&& (K_getline(1, first_line, NULL)
		    != K_getline(1, first_line-1, NULL) )
		&& (K_getline(1, last_line, NULL)
		    != K_getline(1, last_line+1, NULL) )) )
		type = _O_APP;
	}
	t_beg1 = E_getl1(first_edit);
	t_end1 = E_getl1(last_edit);
	t_beg2 = E_getl2(first_edit);
	t_end2 = E_getl2(last_edit);
	/*
	**	we are printing differences in terms of lines
	**	so find the beginning and ending lines of the
	**	changes and print header in those terms
	*/
	first1 = (t_beg1 >= 0) ? K_getline(0,t_beg1,NULL) : t_beg1;
	last1  = (t_end1 >= 0) ? K_getline(0,t_end1,NULL) : t_end1;
	first2 = (t_beg2 >= 0) ? K_getline(1,t_beg2,NULL) : t_beg2;
	last2  = (t_end2 >= 0) ? K_getline(1,t_end2,NULL) : t_end2;
	/*
	**	print the header for this difference
	*/
	IISTprintf(diffBuff,ERx("%d"),first1 + 1);
	tempBuff[0] = '\0';
	switch (type)
	{
	   case _O_APP :
		IISTprintf(diffOut,ERx("a%d"),first2 + 1);
		if (last2 > first2)
		    IISTprintf(tempBuff,ERx(",%d"),last2 + 1);

		STpolycat(3, diffBuff, diffOut, tempBuff, diffBuff);
		break;
	   case _O_DEL :
		if (last1 > first1)
		    IISTprintf(tempBuff,ERx(",%d"),last1 + 1);

		IISTprintf(diffOut,ERx("d%d"),first2 + 1);
		STpolycat(3, diffBuff, tempBuff, diffOut, diffBuff);
		break;
	   case _O_CHA :
		if (last1 > first1)
		    IISTprintf(tempBuff,ERx(",%d"),last1 + 1);

		IISTprintf(diffOut,ERx("c%d"),first2 + 1);
		STpolycat(3, diffBuff, tempBuff, diffOut, diffBuff);
		if (last2 > first2)
		{
		    IISTprintf(diffOut,ERx(",%d"),last2 + 1);
		    STcat(diffBuff, diffOut);
		}
		break;
	   default:
		IISTprintf(diffBuff, ERx("type in O_output wasn't set\n"));
	}
	/*
	** Subtract 1 from t_beg so that the first differing
	** token is highlighted.
	*/
	if (_O_DEL == type || _O_CHA == type)
	    _O_do_lines(t_beg1 - 1,t_end1,0,diffFile);
	if (_O_CHA == type)
	    IISTprintf(diffBuff, ERx("---"));
	if (_O_APP == type || _O_CHA == type)
	    _O_do_lines(t_beg2,t_end2,1,diffFile);
    }

    /* free space taken by edit list */
    free_edits(ahead);

    return;
}

i4
K_getline(i4 file,i4 token_no,FILE_TOKENS **kptr)
{
    register i4            i ;
    register FILE_TOKENS **tokarr = NULL ;
    register i4            result_line ;
    register i4            toklimit ;

    if (file)
    {
	toklimit = numtokens2;
	tokarr = tokarray2;
    }
    else
    {
	toklimit = numtokens1;
	tokarr = tokarray1;
    }
    result_line = (token_no > toklimit || token_no <= 0
		    ? 0 : tokarr[token_no]->lineno);
    if (kptr)
    {
	if (result_line)
	{
	    for (i = token_no-1; (i > 0) && (tokarr[i]->lineno == result_line);
		 i--) {} ;
	    *kptr = tokarr[i+1];
	}
	else
	    *kptr = NULL;
    }
    return(result_line - 1);
}
