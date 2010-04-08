/*
**Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		PCexit.c
 *
 *	Function:
 *		PCexit
 *
 *	Arguments:
 *		i4	status;
 *
 *	Result:
 *		Program exit.
 *
 *		Status is value returned to shell or command interpreter.
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83	--	(gb)
 *			written
 *		09-feb-88	(marge)
 *		    Added the following for ATEX-only "error recovery" feature: 
 *			Storage added: PCexitfuncs, PClastfunc 
 *			PCexit change: Call to functions in PCexitfuncs array
 *			PCatexit added: new function
 *		    Note that behavior of user applications will not change
 *		    unless the user (ATEX only) has initialized PCexitfuncs with
 *		    the user's exit function.
 *
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
 *              9/93 (bobm)     defined purify_override_PCexit().
**	08-may-1997 (canor01)
**	    Prevent more than one thread from calling the PCexitfuncs when
**	    running with OS threads.
**	27-jun-97 (muhpa01)
**	    Removed doingexit exit variable to control synch_lock for OS_THREADS
**	    and instead keyed the locking/unlocking on whether we had initialized
**	    the lock (exitinit).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
 */

# include	<compat.h>
# include	<gl.h>
# include	<cs.h>
# include	<ex.h>

# define	MAXFUNCS	32

static VOID (*PCexitfuncs[ MAXFUNCS ])();
static VOID (**PClastfunc)() = PCexitfuncs;

# ifdef OS_THREADS_USED
static CS_SYNCH exitlock;
static bool exitinit = FALSE;
# endif /* OS_THREADS_USED */

VOID
PCexit(status)
i4	status;
{
	register VOID (**f)() = PClastfunc;

# ifdef OS_THREADS_USED
	/* if we've initialized, then we can lock/unlock exitlock */
        if ( exitinit )
	{
	    CS_synch_lock( &exitlock );
	}

	while( f-- != PCexitfuncs )
	    (**f)();

	if ( exitinit )
	    CS_synch_unlock( &exitlock );
# else /* OS_THREADS_USED */
	while( f-- != PCexitfuncs )
	    (**f)();
# endif /* OS_THREADS_USED */

	(void)exit( (int)status );
	/*NOTREACHED*/
}

/*{
** Name:        purify_override_PCexit
**
** Description:
**      When linked with purify, this entry point will be called in place
**      of PCexit().  Does the same thing PCexit() does, but also calls
**      IIME_purify_tag_exit(), which clears the ME alloc list for purify's
**      leak report.
**
** Inputs:
**      status -        exit status
**
** History:
**      8/93 (bobm)     First Written
*/

VOID
purify_override_PCexit(status)
{
        register VOID (**f)() = PClastfunc;

        while( f-- != PCexitfuncs )
                (**f)();

        IIME_purify_tag_exit();

        (void)exit( (int)status );
        /*NOTREACHED*/
}

VOID
PCatexit( func )
VOID (*func)();
{
# ifdef OS_THREADS_USED
	if ( !exitinit )
	{
	    exitinit = TRUE;
	    CS_synch_init( &exitlock );
	}
# endif /* OS_THREADS_USED */

	if( PClastfunc < &PCexitfuncs[ MAXFUNCS ] )
		*PClastfunc++ = func;
}
