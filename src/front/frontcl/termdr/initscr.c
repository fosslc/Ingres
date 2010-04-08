# include       <compat.h>
# include       <cv.h>
# include       <st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <fe.h>
# include       <lo.h>
# include       <nm.h>
# include       <te.h>
# include       <si.h>
# include       <ug.h>
# include       <er.h>
# include       "ertd.h"
# include       <termdr.h>


GLOBALREF bool  vt100;

GLOBALREF i4    IITDAllocSize;

/*
**  initscr.c
**
**  This routine initializes the current and standard screen.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**      08/14/87 (dkh) - ER changes.
**      08/21/87 (dkh) - Changed terminal restoring code.
**      08/28/87 (dkh) - Changed error numbers from 8000 series to local ones.
**      09/04/87 (dkh) - Added new argument to TDrestore() call.
**      10-sep-87 (bab)
**              Changed from use of ERR to FAIL (for DG.)
**      12/18/87 (dkh) - Fixed jup bug 1632.
**      07/13/89 (dkh) - Changed ifdefs to make job control work again on UNIX.
**      08/04/89 (dkh) - Added support for 80/132 runtime switching.
**      01/12/90 (dkh) - Moved some error message to TDsetterm().
**      30-jan-90 (brett) - Added call to IITDfalsestop() for job control
**                          support in window view.
**      03/04/90 (dkh) - Integrated above change (ingres61b3ug/2334).
**      08/28/90 (dkh) - Integrated porting change ingres6202p/131215.
**      03/24/91 (dkh) - Integrated changes from PC group.
**      16-apr-91 (fraser)
**          Fixed stdmsg window to use last column.
**	15-nov-91 (leighb) DeskTop Porting Change: Cleanup previous change.
**	26-may-92 (leighb) DeskTop Porting Change:
**		Changed PMFE ifdefs to MSDOS ifdefs to ease OS/2 port.
**      24-sep-96 (mcgem01)
**              Global data moved to termdrdata.c
**      04-may-97 (mcgem01)
**              Now that all hard-coded references to TERM_INGRES have
**              been removed from the message files, make the appropriate 
**		system terminal type substitutions in the error messages.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/


/*
** 1) set keypad mode if possible.
** 2) if LD != NULL then set G1 as graphic character set.
*/

# ifdef UNIX
void
TDjcstop()
{
        /*
        **  Set WindowView up for the suspend.
        */
        IITDfalsestop();

        /*
        **  Added for fix to BUG 6166. (dkh)
        */
        TDrstcur(0, COLS - 1, LINES -1, 0);
        TDrestore(TE_NORMAL, FALSE);
}


void
TDjcrestart()
{
        reg     WINDOW  *screen = curscr;
                i4      ox;
                i4      oy;

        TDrestore(TE_FORMS, TRUE);
        ox = screen->_curx;
        oy = screen->_cury;
        TDrefresh(screen);
        screen->_cury = oy;
        screen->_curx = ox;
}
# endif /* UNIX */

/*
**  Value for argc should ONLY be 1 or 0, so the single
**  declaration for "args" is OK.
*/
i4
TDerrmsg(errnum, argc, args)
ER_MSGID        errnum;
i4              argc;
char            *args;
{
        IIUGerr(errnum, UG_ERR_ERROR, argc, args);
        return(FAIL);
}


TDinitscr()
{
        char            *sp;
        i4              TDtstp();
        char            outbuf[TDBUFSIZE];
        char            termbuf[100];

        TDobptr = outbuf;
        TDoutbuf = outbuf;
        TDoutcount = TDBUFSIZE;

        TDgettmode();
        NMgtAt(ERx("TERM_INGRES"), &sp);
        if (sp == NULL || *sp == '\0')
        {
                NMgtAt(TERM_TYPE, &sp);
        }
        if (sp == NULL || *sp == '\0')
        {
		IIUGerr (E_TD001C_NOTERM, UG_ERR_ERROR, 2, SystemTermType,
				SystemTermType);
		return(FAIL);
        }
        STlcopy(sp, termbuf, sizeof(termbuf)-1);
        if(TDsetterm(termbuf) == FAIL)
        {
                return(FAIL);
        }
        CVlower(termbuf);
        if(STcompare(termbuf, ERx("vt100")) == 0)
                vt100 = TRUE;
        _puts(IS);                      /* initialize terminal */
        _puts(TI);
        _puts(VS);
        if(vt100)
        {
                /* used to initialize vt100 */
                TDsmvcur((i4) 0, COLS, LINES, (i4)0);
        }
        _puts(CL);
        if (LD != NULL && !XD)
        {
                _puts(LD);
        }
        if (KY && KS != NULL)
        {
                _puts(KS);
        }
        TEwrite(TDoutbuf, (i4)(TDBUFSIZE - TDoutcount));
        TEflush();
        if (curscr != NULL)
        {
                TDdelwin(curscr);
        }
        if ((curscr = TDnewwin(LINES, IITDAllocSize, (i4) 0,
                (i4) 0)) == (WINDOW *) NULL)
        {
                return(TDerrmsg(E_TD001F_CURINIT, 0, NULL));
        }
        curscr->_maxx = COLS;
        curscr->_clear = TRUE;
        curscr->_flags |= (_FULLWIN | _SCROLLWIN | _ENDLINE);
        curscr->_scroll = TRUE;


# ifdef KDFD
        if (lastlnwin != NULL)
        {
                TDdelwin(lastlnwin);
        }
        if ((lastlnwin = TDnewwin((i4) 1, IITDAllocSize - 1, LINES - 1,
                (i4) 0)) == (WINDOW *) NULL)
        {
                return(TDerrmsg(E_TD0020_MSGINIT, 0, NULL));
        }
        lastlnwin->_relative = FALSE;
# endif
        if (stdmsg != NULL)
        {
                TDdelwin(stdmsg);
        }
# ifdef MSDOS
        if ((stdmsg = TDnewwin((i4) 1, IITDAllocSize, LINES - 1,
# else
        if ((stdmsg = TDnewwin((i4) 1, IITDAllocSize - 1, LINES - 1 ,
# endif /* MSDOS */
                (i4) 0)) == (WINDOW *) NULL)
        {
                return(TDerrmsg(E_TD0020_MSGINIT, 0, NULL));
        }
# ifdef PMFEDOS
        stdmsg->_maxx = COLS;
# else
        stdmsg->_maxx = COLS - 1;
# endif
        stdmsg->_relative = FALSE;

# ifdef UNIX
        TEjobcntrl(TDjcstop, TDjcrestart);
# endif /* UNIX */

        /* setup box/line drawing characters so that we can
           fixup lines when they meet and cross. */
        TDboxsetup();

        return(OK);
}
