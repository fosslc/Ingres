/*
**    Include file
*/

#include <compat.h>
#include <cm.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <er.h>
#include <termdr.h>

#define getanswer_c

#include <sepdefs.h>
#include <seperrs.h>
#include <fndefs.h>

/*
** History:
**	25-may-1994 (donj)
**	    Created.
**	    Reconcile get_answer(), in sep, and get_answer2(), in listexec,
**	    into one root function SEP_Get_Answer() in file sep/getanswer.c
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

i4
SEP_Get_Answer(WINDOW *stat_win, i4  row, i4  col, char *question,
	       char *ansbuff, bool *batch_mode, char **errmsg)
{
    i4                     ret_nat = 0 ;

    if ((batch_mode != NULL)&&(*batch_mode == TRUE))
    {
	if (errmsg != NULL)
	{
	    char          *err_fmt =
	  ERx("\nERROR: question was given while workingin batch mode:\n%s\n") ;

	    *errmsg = (char *)SEP_MEalloc(SEP_ME_TAG_MISC,
					  STlength(err_fmt)+
					  STlength(question)+2, TRUE,
					  (STATUS *) NULL);

            STprintf(*errmsg, err_fmt, question);
	}
	ret_nat = SEPerr_question_in_batch;
    }
    else
    {
	TDerase(stat_win);
	SEPprintw(stat_win, row, col, question);
	TDrefresh(stat_win);
	getstr(stat_win,ansbuff,FALSE);
	TDerase(stat_win);
	TDrefresh(stat_win);
    }
    return (ret_nat);
}
