/*
**      set_sep_state
**
**      Description:
*/

/*
** History:
**
**    29-dec-1993 (donj)
**	Split this function out of septool.c
**      13-apr-1994 (donj)
**          Fix some return typing. Making sure that all funtions return an
**          acceptable value in every case.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#include <compat.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>

#define setstate_c

#include <sepdefs.h>
#include <fndefs.h>

GLOBALREF    i4            sep_state ;
GLOBALREF    i4            sep_state_old ;
GLOBALREF    char         *lineTokens [] ;

STATUS
set_sep_state(i4 tokens)
{
    STATUS                 ret_val = OK ;
    i4                     lt_0 = 0 ;

    sep_state_old = sep_state;

    if (lineTokens[0] == NULL)
    {
	if ((sep_state&(IN_COMMENT_STATE|IN_FILL_STATE|IN_CANON_STATE)) == 0)
	    sep_state |= BLANK_LINE_STATE;
    }
    else
    {
	if (sep_state&BLANK_LINE_STATE)
	    sep_state &= (~BLANK_LINE_STATE);

	lt_0 = STlength(lineTokens[0]);
	if (sep_state&IN_CANON_STATE) /* if inside a canon */
	{
	    if (STcompare(lineTokens[0],CLOSE_CANON) == 0)
	    {
		sep_state &= (~(IN_CANON_STATE|IN_SKIPV_STATE|
				IN_SKIP_STATE|OPEN_CANON_ARGS));
		if (tokens == 1)
		    sep_state &= (~CLOSE_CANON_ARGS);
		else
		    sep_state |= CLOSE_CANON_ARGS;
	    }
	    else
	    if (STcompare(lineTokens[0],SKIP_SYMBOL) == 0)
		sep_state |= IN_SKIP_STATE;
	    else
	    if (STcompare(lineTokens[0],SKIP_VER_SYMBOL) == 0)
		sep_state |= IN_SKIPV_STATE;
	}
	else
	if (sep_state&IN_COMMENT_STATE) /* if in comment */
	{
	    if ((STcompare(lineTokens[0],CLOSE_COMMENT) == 0)
		&& (tokens == 1))
		sep_state &= (~IN_COMMENT_STATE);
#if 0
	/* don't include this code as of now. */
	    else
	    if (STcompare(lineTokens[tokens-1],CLOSE_COMMENT) == 0)
		sep_state &= (~IN_COMMENT_STATE);
#endif /* 0 */
	}
	else
	if (sep_state&IN_FILL_STATE) /* if inside a fill command */
	{
	    if (STcompare(lineTokens[0],FILL_PROMPT) == 0)
		sep_state &= (~IN_FILL_STATE);
	}
	else
	{
	    if (sep_state&IN_ENDING_STATE)
		sep_state &= (~IN_ENDING_STATE);

	    if (STcompare(lineTokens[0],OPEN_CANON) == 0)
	    {
		sep_state |= IN_CANON_STATE;
		sep_state &= (~CLOSE_CANON_ARGS);
		if (tokens == 1)
		    sep_state &= (~OPEN_CANON_ARGS);
		else
		    sep_state |= OPEN_CANON_ARGS;
	    }
	    else
	    if ((STcompare(lineTokens[0],OPEN_COMMENT) == 0)
		&& (tokens == 1))
		sep_state |= IN_COMMENT_STATE;
	    else
	    if (STcompare(lineTokens[0],FILL_PROMPT) == 0)
		sep_state |= IN_FILL_STATE;
	    else
	    if (STcompare(lineTokens[0],ENDING_PROMPT) == 0)
		sep_state |= IN_ENDING_STATE;
	    else
	    if (STbcompare(lineTokens[0],lt_0,CONTROL_PROMPT,1,0) == 0)
		sep_state |= IN_CONTROL_STATE;
	    else
	    if (STcompare(lineTokens[0],KFE_PROMPT) == 0)
	    {
		sep_state &= (~(IN_OS_STATE|IN_TM_STATE));
		sep_state |= IN_FE_STATE;
	    }
	    else
	    if (STcompare(lineTokens[0],SEP_PROMPT) == 0)
	    {
		sep_state &= (~(IN_FE_STATE|IN_TM_STATE));
		sep_state |= IN_OS_STATE;
	    }
	    else
	    if (STcompare(lineTokens[0],TM_PROMPT) == 0)
	    {
		sep_state &= (~(IN_OS_STATE|IN_FE_STATE));
		sep_state |= IN_TM_STATE;
	    }
	}
    }
    return (ret_val);
}   /* End of set_sep_state() */
