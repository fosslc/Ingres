#include <compat.h>
#include <kst.h>
#include <si.h>
#include <me.h>
#include <er.h>
#include <gl.h>
#include <sl.h>
#include <iicommon.h>
#include <fe.h>
#include <ug.h>
#include <ex.h>
#include <termdr.h>
#include <mapping.h>

/*
** History:
**	16-aug-1989 (mca)
**		Fix peditor stuff.
**	9-Nov-1989  (eduardo)
**		Disable interrupts for VMS sake
**	15-may-1990  (rog)
**		Minor code clean-up.
**	24-may-1990 (rog)
**		Remove stuff defined in FTLIB.
**	11-jun-1990 (rog)
**		Remove stuff defined in FTLIB.
**	27-nov-1990 (rog)
**		A for loop had an '=' in its conditional statement instead of
**		a '<'.
**	21-feb-1991 (gillnh2o)
**		Removed the IIUGerr function because the "real" one is in
**		libingres.a.
**	25-apr-1991 (jonb)
**		Removed the FEgettag function because the "real" one is in
**		libingres.a.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Simplified
**	    and clarified some control loops.
**	 5-mar-1993 (donj)
**	    Take out er.h and add minimal ERx("") macro.
**	27-apr-1993 (donj)
**	    Replace include of dbms.h with the four includes of gl.h, sl.h,
**	    iicommon.h and dbdbms.h.
**      27-apr-1993 (donj)
**          Took out dbdbms.h, not needed and I was getting errors.
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**	6-oct-93 (vijay)
**	    include termdr.h after including some cl files. This is needed
**	    because we need to include sys/types.h first. sys/types.h has
**	    reg as a variable name on AIX and reg is 'defined' in termdr as
**	    register. This causes syntax errors.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN  KEYSTRUCT    *TDgetch             ( WINDOW *awin ) ;
FUNC_EXTERN  char         *SEP_MEalloc         ( u_i4 tag, u_i4 size, bool clear, STATUS *ME_status ) ;

GLOBALDEF    bool          FEtesting = FALSE ;

/*
** SEP sends a ^W each time it wants
** to redraw the screen
*/
#define SEP_REDRAW '\027'

VOID
getstr (WINDOW *w,char *s,bool peditor)
{
    KEYSTRUCT             *k = NULL ;
    char                  *orig_s = s ;

    EXinterrupt(EX_OFF);
    while (1)
    {
	k = TDgetch(w);
	if (peditor && (k->ks_ch[0] == SEP_REDRAW))
	{
	    *s++ = k->ks_ch[0];
	    break;
	}
	if (((k->ks_ch[0] & 0377) == (EOF & 0377))
	    || (k->ks_ch[0] == '\n') || (k->ks_ch[0] == '\r'))
	    break;

	if ((k->ks_ch[0] == '\b') || (k->ks_ch[0] == '\177'))
	{
	    if (s > orig_s)
	    {
		TDmove(w, w->_cury, w->_curx-1);
		TDadch(w, ' ');
		TDmove(w, w->_cury, w->_curx-1);
		--s;
	    }
	}
	else
	{
	    *s++ = k->ks_ch[0];
	    TDadch(w, k->ks_ch[0]);
	}
	TDrefresh(w);
    }
    *s = '\0';
    EXinterrupt(EX_ON);

} /* getstr */

VOID
IIUGbmaBadMemoryAllocation (char *str)
{
    SIprintf(ERx("SEP: Memory allocation failed: %s\n"), str);
    PCexit(FAIL);

} /* IIUGbmaBadMemoryAllocation */

#define UG_LDNAME 8

VOID
syserr(char *msg)
{
    SIprintf(ERx("FATAL ERROR: %s.\n"), msg);
    SIflush(stdout);
    PCexit(FAIL);

} /* syserr */

