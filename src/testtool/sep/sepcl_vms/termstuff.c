#include <compat.h>
#include <termdr.h>
#include <kst.h>
#include <si.h>
#include <me.h>
#include <er.h>
#include <dbms.h>
#include <fe.h>
#include <ug.h>
#include <ex.h>
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
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**	14-sep-1992 (donj)
**	    Took out FEreqmem() and FEfree(). They were obsolete.
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
*/

KEYSTRUCT                 *TDgetch () ;
/* PTR                        _tag_ptrs [256] ; */
GLOBALDEF    bool          FEtesting = FALSE ;

FUNC_EXTERN  char         *SEP_MEalloc () ;

/*
** SEP sends a ^W each time it wants
** to redraw the screen
*/
#define SEP_REDRAW '\027'

VOID
getstr (w, s, peditor)
WINDOW *w;
char   *s;
bool    peditor;
{
    KEYSTRUCT             *k ;
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
IIUGbmaBadMemoryAllocation (str)
char *str;
{
    SIprintf(ERx("SEP: Memory allocation failed: %s\n"), str);
    PCexit(FAIL);

} /* IIUGbmaBadMemoryAllocation */

/* *********************************************************
**  FEreqmem not needed. try it without it.
PTR
FEreqmem(tag, size, clear, status)
i4      tag;
i4      size;
bool    clear;
STATUS *status;
{
    PTR p;

    p = (PTR) SEP_MEalloc(tag, size, clear, status);
    _tag_ptrs[tag] = p;
    return(p);

} ** FRreqmem **

STATUS
FEfree(tag)
i4  tag;
{
    return(MEfree(_tag_ptrs[tag]) );

} ** FEfree */

#define UG_LDNAME 8


VOID
syserr(msg)
char *msg;
{
    SIprintf(ERx("FATAL ERROR: %s.\n"), msg);
    SIflush(stdout);
    PCexit(FAIL);

} /* syserr */

