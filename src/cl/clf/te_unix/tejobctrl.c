/*
**  Copyright (c) 1985, 2004 Ingres Corporation
**
**  TEjobcntrl.c
**
**  This code is for the purpose of allowing a user to
**  specify what to do when a job control signal happens
**  while running one of the front-ends.  It allows the
**  front-end to reset the terminal to a sane state and
**  when the front-end is back in foreground, the
**  appropriate forms mode is put back and the screen
**  is redrawn.  Both arguments to TEjobcntrl may
**  be NULL.
**
**  The code below only works for BSD 4.2 UNIX.
**  On VMS, the code does nothing.
**
**  This routine could be used by non-forms programs
**  to handle job-control signals if one really wants to.
**
**  static	char	Sccsid[] = "%W%	%G%";
**
**  History:
**	xx/xx/83 (dkh) - Initial version.
**	07/13/89 (dkh) - Changed ifdefs to make job control work again on UNIX.
**	11/13/89 (dkh) - Integrated job control fix into termcl line.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <clsigs.h>.
**	22-may-90 (rog) (from an ingres6202p change by mca)
**	    Declare TEjobhdlr() as a TYPESIG (), to quiet compiler warnings
**	    on machines where TYPESIG is void.
**	22-may-90 (rog) (from an ingres6202p change by russ)
**	    Change "ifdef SIGTSTP" to "if defined(SIGTSTP) &&
**	    defined(xCL_011_USE_SIGVEC)".  TEjobcntrl requires SIGVEC
**	    as well as job control.
**	23-may-90 (rog)
**	    Add xCL_068_USE_SIGACTION to the ifdef mentioned in the
**	    previous change.
**	29-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add sigaction code.
**	25-mar-91 (kirke)
**	    Added #include <systypes.h> because HP-UX's signal.h includes
**	    sys/types.h.
**	31-jan-92 (bonobo)
**	    Replaced double-question marks; ANSI compiler thinks they
**	    are trigraph sequences.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	24-feb-98 (toumi01)
**	    For Linux (lnx_us5) use sigprocmask, not (undefined) sigrelse.
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	14-Jun-2000 (hanje04)
**	    Added ibm_lnx to platforms using sigprocmask.
**	08-Sep-2000 (hanje04)
**	    For Alpha Linux (axp_lnx) use sigprocmask, not (undefined) sigrelse.
**	04-Dec-2001 (hanje04)
**	    Use sigprocmask on IA64 Linux
**	08-Oct-2002 (hanje04)
**	    sigrelse is now defined for all versions of Linux currently built
**	    on, so we'll use it.
**	04-Nov-2010 (miket) SIR 124685
**	    Prototype cleanup. Add cast to silence sigaction type warning.
**	17-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**	1-Dec-2010 (kschendel) SIR 124685
**	    Fix up jobctrl args to match usage (void).
**	
*/

# include	<compat.h>
# include	<gl.h>
# include	<systypes.h>
# include	<clconfig.h>
# include	<clsigs.h>
# include	<te.h>

# if defined(SIGTSTP) && defined(xCL_011_USE_SIGVEC) || \
     defined(xCL_068_USE_SIGACTION)



/*
**  Store the function pointers away so we can
**  can them when a job control signal occurs.
*/

static	void	(*TEreset)(void) = NULL;
static	void	(*TEredraw)(void) = NULL;

/*
**  This routine is internal to the CL and
**  is used by TEjobcntrl to actual process
**  the signal.  All the routine does, is call
**  the TEreset function pointer if it is not
**  NULL, stops itself and calls TEredraw (if
**  not NULL) when the program is back in
**  foreground.
*/

static TYPESIG
TEjobhdlr(int notused)
{
# ifdef xCL_011_USE_SIGVEC
	struct	sigvec	newvec;
	struct	sigvec	oldvec;
# else
# ifdef xCL_068_USE_SIGACTION
        struct  sigaction       newaction;
        struct  sigaction       oldaction;
# else
        int old;
# endif /* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */
	i4	kill_ret;
	i4	tmpmask = 0;

	/*
	**  Call routine to reset terminal,
	**  if it exists.
	*/

	if (TEreset != NULL)
	{
		(*TEreset)();
	}

# ifdef xCL_011_USE_SIGVEC
	newvec.sv_handler = SIG_DFL;
	newvec.sv_mask = 0;
	newvec.sv_onstack = 0;
# else
# ifdef xCL_068_USE_SIGACTION
        newaction.sa_handler = SIG_DFL;
        sigemptyset(&(newaction.sa_mask));
        newaction.sa_flags = 0;
# endif /* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */

	/*
	**  Save old handler so we can
	**  restore when we come back.
	*/

# ifdef xCL_011_USE_SIGVEC
	sigvec(SIGTSTP, &newvec, &oldvec);

	sigsetmask(tmpmask);	/* re-enable the signal */
# else
# ifdef xCL_068_USE_SIGACTION
        sigaction(SIGTSTP, &newaction, &oldaction);
        sigrelse(SIGTSTP);
# else
        old = signal(SIGTSTP, SIG_DFL);
# endif /* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */

	/*
	**  Suspend itself.
	*/

	kill_ret = kill(0, SIGTSTP);

	/*
	**  Restore old handler.
	*/

# ifdef xCL_011_USE_SIGVEC
	sigvec(SIGTSTP, &oldvec, (struct sigvec *)NULL);
# else
# ifdef xCL_068_USE_SIGACTION
        sigaction(SIGTSTP, &oldaction, (struct sigaction *)0);
# else
        signal(SIGTSTP, old);
# endif /* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */

	/*
	**  Call function to set terminal
	**  to forms mode and redraw the screen,
	**  if it exists.
	*/

	if (TEredraw != NULL)
	{
		(*TEredraw)();
	}
}


/*
**  Handle job control for forms based programs.
*/

VOID
TEjobcntrl(void (*reset)(void), void (*redraw)(void))
{
# ifdef xCL_011_USE_SIGVEC
	struct	sigvec	newvec;
# else
# ifdef xCL_068_USE_SIGACTION
        struct  sigaction       newaction;
# else
        int old;
# endif /* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */

	/*
	**  Save away the function pointers
	**  so we can call them when we need it.
	*/

	TEreset = reset;
	TEredraw = redraw;

# ifdef xCL_011_USE_SIGVEC
	newvec.sv_handler = TEjobhdlr;
	newvec.sv_mask = 0;
	newvec.sv_onstack = 0;
# else
# ifdef xCL_068_USE_SIGACTION
        newaction.sa_handler = TEjobhdlr;
        sigemptyset(&(newaction.sa_mask));
        newaction.sa_flags = 0;
# endif /* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */

	/*
	**  Set job control signal to call
	**  TEjobhdlr() when we get a
	**  job control signal.
	*/

# ifdef xCL_011_USE_SIGVEC
	sigvec(SIGTSTP, &newvec, (struct sigvec *)NULL);
# else
# ifdef xCL_068_USE_SIGACTION
        sigaction(SIGTSTP, &newaction, (struct sigaction *)0);
# else
        old = signal(SIGTSTP, TEjobhdlr);
# endif /* xCL_068_USE_SIGACTION */
# endif	/* xCL_011_USE_SIGVEC */
}


# else

/*
**  This is what the routine should
**  look like for VMS or other systems
**  that don't support job control.
*/

VOID
TEjobcntrl(void (*reset)(void), void (*redraw)(void))
{
}

# endif
