#include <compat.h>
#ifdef UNIX
#include <clconfig.h>
#endif
#include <te.h>
#include <signal.h>
#include <termdr.h>

/*
**	History:
**
**	##-mar-89 (mca)
**		Created.
**	24-aug-89 (kimman)
**		Added <clsecret.h> so that TYPESIG is defined
**	24-aug-89 (kimman)
**		As per daveb, using <clconfig.h> instead of <clsecret.h>.
**	11-20-89 (eduardo)
**		Added TDclear(curscr) when CONTINUE signal comes
**	08-jul-91 (johnr)
**		Repositioned signal.h for hp3_us5 to indirectly place
**		systypes.h in front of te which indirectly uses types.h
**      26-apr-1993 (donj)
**          Finally found out why I was getting these "warning, trigrah sequence
**          replaced" messages. The question marks people used for unknown
**          dates. (Thanks DaveB).
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALREF WINDOW *mainW, *statusW;

#ifdef SIGTSTP

TYPESIG
ctl_z_handler (i4 sig)
{
    switch (sig)
    {
	case SIGTSTP:
	    TErestore(TE_NORMAL);
	    kill(getpid(), SIGSTOP);
	    break;
	case SIGCONT:
	    TErestore(TE_FORMS);
	    TDclear(curscr);
	    TDtouchwin(mainW);
	    TDtouchwin(statusW);
	    TDrefresh(mainW);
	    TDrefresh(statusW);
	    break;
    }
}

#endif

initSigHandler ()
{
#ifdef SIGTSTP
    signal(SIGTSTP, ctl_z_handler);
    signal(SIGCONT, ctl_z_handler);
#endif
}
