/*
** testparams.c
**
** History:
**
**	09-Jul-1990 (owen)
**		Start history for this file.
**		Use angle brackets on sep header files in order to search
**		for them in the default include search path.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Simplified
**	    and clarified some control loops.
**	30-nov-1992 (donj)
**	    Mark Rewrite_cfg flag if user changes types, levels or parts.
**
**
*/

/*
    Include files
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <nm.h>
#include <er.h>
#include <cm.h>
#include <te.h>
#include <cv.h>
#include <kst.h>
#include <termdr.h>

#include <listexec.h>
#include <params.h>

#define	PARTmssg1   ERx("(%d) Select %s tests to be executed")
#define PARTmssg2   ERx("SELECTING %s TESTS TO BE EXECUTED")

GLOBALREF    bool          Rewrite_Cfg ;
FUNC_EXTERN  i4            get_menu_choice () ;

testparams(prompt)
WINDOW *prompt;
{
    WINDOW                *awin ;
    i4                     choice ;
    i4                     row ;
    i4                     toquit ;
    i4                     totype ;
    i4                     tolevel ;

    awin = TDnewwin(LINES - 2, COLS, 0, 0);
    toquit = display_menu(awin,&row);
    totype = toquit - 2;
    tolevel = toquit - 1;
    for (;;)
    {
	choice = get_menu_choice(awin,prompt,row,19,1,toquit);
	if (choice == toquit)
		break;
	else
	{
	    if (choice == totype)
		select_subs(prompt,ing_types);
	    else if (choice == tolevel)
		select_subs(prompt,ing_levels);
	    else
		select_subs(prompt,ing_parts[choice-1]);
	}
        TDtouchwin(awin);
        TDrefresh(awin);
        TDerase(prompt);
        TDrefresh(prompt);
    }
    TDerase(awin);
    TDdelwin(awin);
}



display_menu(win,prow)
WINDOW  *win;
i4	*prow;
{
    i4                     count = 0 ;
    i4                     row = 2 ;

    TDerase(win);
    TDmvwprintw(win,0,24,ERx("SELECTING TESTS TO BE EXECUTED"));
    while (ing_parts[count])
    {
	TDmvwprintw(win,row++,19,PARTmssg1,count+1,ing_parts[count]->partname);
	count++;
    }
    TDmvwprintw(win,row++,19,ERx("(%d) Select test type to be executed"),++count);
    TDmvwprintw(win,row++,19,ERx("(%d) Select test level to be executed"),++count);
    TDmvwprintw(win,++row,19,ERx("(%d) QUIT"),++count);
    TDrefresh(win);
    *prow = row + 2;
    return(count);
}

select_subs(pwin,part)
WINDOW	    *pwin;
PART_NODE   *part;
{
    WINDOW                *win ;
    char                   num [10] ;
    char                 **subs = part->subs ;
    char                  *torun = part->runkey ;
    i4                     count = 0 ;
    i4                     row = 2 ;
    i4                     col = 10 ;
    i4                     num_subs = 0 ;
    i4                     choice ;

    win = TDnewwin(LINES - 2, COLS, 0, 0);
    TDerase(win);
    TDmvwprintw(win,0,20,PARTmssg2,part->partname);
    for ( ; subs[count]; count++ )
    {
	CVna(++num_subs,num);	
	TDmvwprintw(win,++row,col,ERx("(%s) %s"),num,subs[count]);
	TDmvwprintw(win,row,col+20,ERx("to be run ? %c"),torun[count]);
    }
    row += 2;
    CVna(num_subs+1,num);	
    TDmvwprintw(win,row,col,ERx("(%s) QUIT"),num);
    TDrefresh(win);
    
    for (row += 2; ; )
    {
	choice = get_menu_choice(win,pwin,row,col,1,num_subs+1);
	if (choice == (num_subs + 1))
	    break;
	count = choice - 1;
	Rewrite_Cfg = TRUE;
	torun[count] = (torun[count] == 'Y') ? 'N' : 'Y';
	TDmvwprintw(win,choice+2,col+20,ERx("to be run ? %c"),torun[count]);
	TDrefresh(win);
    }
    TDerase(win);
    TDdelwin(win);
}

