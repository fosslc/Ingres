/* *******************************************************************
**      disp_line
**
**      Description:
**      subroutine to display a line in the main window,
**      parameters:
**      lineptr -   pointer to message to de displayed
**      row     -   row where message will be display
**      column  -   column where message will be display
** *******************************************************************
*/

/*
** History:
**
**    29-dec-1993 (donj)
**	Split this function out of septool.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
**	1-Dec-2010 (kschendel)
**	    Compiler warning fix.
*/

#include <compat.h>
#include <cm.h>
#include <er.h>
#include <lo.h>
#include <si.h>
#include <st.h>

#include <termdr.h>

#define displine_c

#include <sepdefs.h>
#include <fndefs.h>

VOID
Disp_Line(WINDOW *mainW, bool batch_mode, i4  *scr_line,
          bool paging, char *lineptr, i4  row, i4  column)
{
    i4                     c_col = FIRST_MAIN_COL ;
    i4                     i ;
    char                  *cp = NULL ;
    char                  *cptr1 = NULL ;
    char                   c1 [3] = { EOS, EOS, EOS } ;

    if (!batch_mode)
    {
	cp = lineptr;
 	if (row)
	{
	    *scr_line = row;
	    c_col = column;
	}

	for (;;)
	{
	    for (i=0; i < 3; i++)
		c1[i] = EOS;

	    if (SEP_CMstlen(cp,0) >= MAIN_COLS)
	    {
		for (cptr1 = cp,i=0; i < MAIN_COLS; i++)
		    CMnext(cptr1);
		CMcpychar(cptr1,c1);
		*cptr1 = EOS;
	    }

	    if (*scr_line >= FIRST_MAIN_ROW && *scr_line <= LAST_MAIN_ROW)
		SEPprintw(mainW, (*scr_line)++, c_col, cp);
	    else if (paging)
	    {
		*scr_line = FIRST_MAIN_ROW;
		TDerase(mainW);
		SEPprintw(mainW, (*scr_line)++, c_col, cp);
	    }
	    else
	    {
		TDscroll(mainW);
		SEPprintw(mainW, LAST_MAIN_ROW, FIRST_MAIN_COL, cp);
	    }

	    if (c1[0] == EOS)
		break;
	    else
	    {
		cp = cptr1;
		CMcpychar(c1,cp);
	    }
	}
	TDrefresh(mainW);
    }
    else
    {
	SIprintf(ERx("%s\n"),lineptr);
	SIflush(stdout);
    }
}

/*
**      disp_line_no_refresh() is the same as disp_line(),
**      except it doesn't do a TDrefresh(). This routine is called only
**      by disp_res(). Its need became apparent when testing of net
**	started in early july. Since net testing involved handling of huge
**	tables, test engineer spent a lot of time just watching the screen
**	scrolling the results.
**        disp_line() and disp_line_no_refresh() should be one function.
*/
VOID
Disp_Line_no_refresh(WINDOW *mainW, bool batch_mode, i4  *scr_line, char *lineptr,i4 row,i4 column)
{

    if (!batch_mode)
    {
	i4  c_col = FIRST_MAIN_COL;
	char *cp = NULL ;
	char c = EOS ;

	cp = lineptr;
 	if (row)
	{
	    *scr_line = row;
	    c_col = column;
	}
	for (;;)
	{
	    if (STlength(cp) >= MAIN_COLS)
	    {
		c = cp[MAIN_COLS - 1];
		cp[MAIN_COLS - 1] = EOS;
	    }
	    else
		c = EOS;
	    if (*scr_line >= FIRST_MAIN_ROW && *scr_line <= LAST_MAIN_ROW)
	    {
		SEPprintw(mainW, *scr_line, c_col, cp);
		(*scr_line)++;
	    }
	    else
	    {
		TDscroll(mainW);
		SEPprintw(mainW, LAST_MAIN_ROW, FIRST_MAIN_COL, cp);
	    }
	    
	    if (c == EOS)
		break;
	    else
	    {
		cp += STlength(cp);
		*cp = c;
	    }
	}
    }
    else
    {
	SIprintf(ERx("%s\n"),lineptr);
	SIflush(stdout);
    }

}
