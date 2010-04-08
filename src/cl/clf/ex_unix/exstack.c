
# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<ex.h>
# include	"exi.h"

/*
NO_OPTIM = rs4_us5 ris_u64 i64_aix
*/

/*
**Copyright (c) 2004 Ingres Corporation
**
** exstack.c -- EX Local Stack Handling Routines.
**
**	Contains all routines dealing with the exception stack.
**	These routines should be local to EX.
**
**	Content:
**		i_EX_initfunc()
**			Routine to initialize the i_EXsetcontext and
**			i_EXgetcontext static pointers to functions.
**
**		i_EX_setcontext()
**			Default set context routine for non-threaded users.
**
**		i_EX_getcontext()
**			Default get context routine for non-threaded users.
**
**		i_EXtop()
**			Routine the topmost environment on the stack.
**
**		i_EXpush()
**			Push a new scope on the stack.
**
**		i_EXpop()
**			Pop a scope off the stack.
**
**	History:
**	
 * Revision 1.2  90/11/19  11:24:58  source
 * sc
 * 
 * Revision 1.2  89/11/16  16:23:42  source
 * sc
 * 
 * Revision 1.1  89/09/25  15:16:09  source
 * Initialized as new code.
 * 
 * Revision 1.1  89/05/26  15:49:12  source
 * sc
 * 
 * Revision 1.1  89/05/11  01:09:42  source
 * sc
 * 
 * Revision 1.1  89/01/13  17:12:05  source
 * This is the new stuff.  We go dancing in.
 * 
 * Revision 1.1  88/08/05  13:36:26  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.2  87/11/25  13:10:48  mikem
**      use new lower case hdr files.
**      
**      Revision 1.1  87/11/09  12:59:44  mikem
**      Initial revision
**      
 * Revision 1.1  87/07/30  18:04:13  roger
 * This is the new stuff.  We go dancing in.
 * 
**		Revision 1.2  86/04/25  17:02:28  daveb
**		Change comments, remove code that was never being
**		compiled in, make routines void since their 
**		return value was neither changing nor being checked.
**		
**	26-aug-89 (rexl)
**	    Add non-threaded functions to set and get current context,
**	    i_EX_setcontext() and i_EX_getcontext() respectively.
**	    Add statics i_EXsetcontext and i_EXgetcontext, which are
**	    pointers to functions that perform the desired action so that
**	    in a multi-threaded environment one can obtain the current
**	    context from the CS_SCB without knowing any details of the CS_SCB.
**	    Add i_EX_initfunc() so that the set and get functions can be
**	    specified by the user.
**	    i_EXtop(), i_EXpush() and i_EXpop() were modified to use the
**	    function calls instead of the static variable ex_sptr.
**	    ex_sptr is still used by the default, non-threaded, functions
**	    i_EX_setcontext() and i_EX_getcontext().
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	03-jun-1996 (canor01)
**	    Add thread-local storage and threaded versions of functions
**	    for use with operating-system threads.
**      22-nov-1996 (canor01)
**          Changed names of CL-specific thread-local storage functions 
**          from "MEtls..." to "ME_tls..." to avoid conflict with new 
**          functions in GL.  Included <meprivate.h> for function prototypes.
**      07-mar-1997 (canor01)
**          Allow functions to be called from programs not linked with
**          threading libraries.
**	19-Mar-1999 (ahaal01)
**	    Added NO-OPTIM for AIX (rs4_us5) to resolve run-time problem.
**	26-Mar-1999 (hweho01)
**	    Turned off optimization for ris_u64 to resolve run-time error. 
**	27-Dec-2000 (jenjo02)
**	    i_EXtop() now returns **EX_CONTEXT rather than *EX_CONTEXT,
**	    i_EXpop() takes **EX_CONTEXT as parameter.
**	    i_EX_getcontext(),i_EX_t_getcontext() now return 
**	    **EX_CONTEXT rather than *EX_CONTEXT.
**	    With OS threads, these changes reduce the number of 
**	    ME_tls_get() calls for a EXdeclare()/EXdelete() pair
**	    from 6 to 2.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**
*/

FUNC_EXTERN	VOID		i_EX_setcontext();
FUNC_EXTERN	EX_CONTEXT	**i_EX_getcontext();

EX_CONTEXT	*ex_sptr;

# ifdef OS_THREADS_USED

FUNC_EXTERN	VOID		i_EX_t_setcontext();
FUNC_EXTERN	EX_CONTEXT	**i_EX_t_getcontext();

static ME_TLS_KEY	EXcontextkey = 0;
static	VOID		 (*i_EXsetcontext)() = i_EX_t_setcontext;
static	EX_CONTEXT	**(*i_EXgetcontext)() = i_EX_t_getcontext;

# else /* OS_THREADS_USED */

static	VOID		 (*i_EXsetcontext)() = i_EX_setcontext;
static	EX_CONTEXT	**(*i_EXgetcontext)() = i_EX_getcontext;

# endif /* OS_THREADS_USED */


/*
** EX_initfunc(setfunc, getfunc)
** void		(*setfunc)();
** EX_CONTEXT	(*getfunc)();
**
**	Sets the pointers used to set and get the current exception stack
**	pointer.  If NULL, systems defaults to the non-threaded functions
**	i_EXsetcontext() and i_EXgetcontext().
**
** HISTORY
**	26-aug-89 (rexl)
**	    written.
**	02-nov-92 (bonobo/mikem)
**	    su4_us5 port. Updated to VOID instead of void on setfunc().
*/

void
i_EX_initfunc(setfunc, getfunc)
VOID		 (*setfunc)();
EX_CONTEXT	**(*getfunc)();
{

	i_EXsetcontext = setfunc ? setfunc : i_EX_setcontext;
	i_EXgetcontext = getfunc ? getfunc : i_EX_getcontext;
	return;
}


/*
** EX_CONTEXT **
** EX_getcontext()
**
**	Return a pointer to the current exception stack pointer in a 
**	non-threaded environment.
**
** HISTORY
**	26-aug-89 (rexl)
**	    written.
**	27-Dec-2000 (jenjo02)
**	    Return **EX_CONTEXT instead of *EX_CONTEXT.
*/

EX_CONTEXT **
i_EX_getcontext()
{
	return( &ex_sptr );
}

/*
** EX_setcontext( context )
** EX_CONTEXT	*context;
**
**	Set the current exception stack pointer in a non-threaded environment.
**
** HISTORY
**	26-aug-89 (rexl)
**	    written.
*/

VOID
i_EX_setcontext( context )
EX_CONTEXT	*context;
{
	ex_sptr = context;
	return;
}


/*
** EX_CONTEXT **
** EXtop()
**
**	Return the topmost scope on the exception stack.
**
** HISTORY
**	12/11/84 Written (sat).
**	27-Dec-2000 (jenjo02)
**	    Return **EX_CONTEXT instead of *EX_CONTEXT.
*/

EX_CONTEXT **
i_EXtop()
{
	return ((*i_EXgetcontext)());
}

/*
** EXpush(contest)
**	EX_CONTEXT	*context;
**
**	Push the scope on the stack.
**
** HISTORY
**	12/11/84 Written (sat).
**
**      02-sep-93 (smc)
**          Made context cast a PTR to be portable to axp_osf. 
*/
void
i_EXpush(context)
EX_CONTEXT	*context;
{
	EX_CONTEXT **ex_psptr;

	/* If this handler is already here, we're just
	** changing the setjmp return point.
	*/

	if ( ex_psptr = (*i_EXgetcontext)() )
	{
	    if ( context->address_check != (PTR)*ex_psptr )
	    {
		context->prev_context = *ex_psptr;
		*ex_psptr = context;
	    }
	}
	else
	    (*i_EXsetcontext)(context);
	return;
}

/*
** EXpop(excp)
**	EX_CONTEXT	**excp;
**
**	Removes topmost scope from stack.
**
** HISTORY
**	12/11/84 Written (sat)
**	27-Dec-2000 (jenjo02)
**	    i_EXpop() takes **EX_CONTEXT as parameter.
*/
void
i_EXpop(excp)
EX_CONTEXT	**excp;
{
	*excp = (*excp)->prev_context;
	return;
}

EX_CONTEXT	*
i_EXnext(context)
EX_CONTEXT	*context;
{
	return(context->prev_context);
}


# ifdef OS_THREADS_USED
/*
** EX_CONTEXT **
** i_EX_t_getcontext()
**
**	Return a pointer to the current exception stack pointer in a 
**	threaded environment.
**
** HISTORY
**	03-jun-1996 (canor01)
**	    written.
**	27-Dec-2000 (jenjo02)
**	    Return **EX_CONTEXT instead of *EX_CONTEXT.
*/

EX_CONTEXT **
i_EX_t_getcontext()
{
    EX_CONTEXT **ex_psptr;
    STATUS     status;

    if ( EXcontextkey == 0 )
    {
	ME_tls_createkey( &EXcontextkey, &status );
	ME_tls_set( EXcontextkey, NULL, &status );
	if ( EXcontextkey == 0 )
	{
	    /* not linked with threaded libraries */
	    EXcontextkey = -1;
	}
    }
    if ( EXcontextkey == -1 )
	/* not linked with threaded libraries */
	return(&ex_sptr);

    ME_tls_get( EXcontextkey, (PTR *)&ex_psptr, &status );
    return(ex_psptr);
}

/*
** i_EX_t_setcontext( context )
** EX_CONTEXT	*context;
**
**	Set the current exception stack pointer in a threaded environment.
**
** HISTORY
**	03-jun-1996 (canor01)
**	    written.
**	27-Dec-2000 (jenjo02)
**	    For OS threads, only called when thread local storage
**	    does not yet exist, obviating ME_tls_get() call.
*/

VOID
i_EX_t_setcontext( context )
EX_CONTEXT	*context;
{
    EX_CONTEXT **ex_psptr;
    STATUS     status;

    if ( EXcontextkey == 0 )
    {
	ME_tls_createkey( &EXcontextkey, &status );
	ME_tls_set( EXcontextkey, NULL, &status );
	if ( EXcontextkey == 0 )
	{
	    /* not linked with threaded libraries */
	    EXcontextkey = -1;
	}
    }
    if ( EXcontextkey == -1 )
    {
	/* not linked with threaded libraries */
	ex_sptr = context;
    }
    else
    {
	ex_psptr = (EX_CONTEXT **) MEreqmem( 0, sizeof(EX_CONTEXT **), 
					   TRUE, NULL );
	ME_tls_set( EXcontextkey, (PTR)ex_psptr, &status );
        *ex_psptr = context;
    }
    return;
}
# endif /* OS_THREADS_USED */

