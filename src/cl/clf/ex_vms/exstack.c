/*
** Copyright (c) 1984, 2009 Ingres Corporation
*/
# include	<compat.h>
# include   <clconfig.h>
# include	<gl.h>
#ifdef OS_THREADS_USED
# include   <me.h>
# include   <meprivate.h>
#endif
# include	<ex.h>
# include	<exinternal.h>

/*
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
**		i_EX_getcontext() - static
**			Default get context routine for non-threaded users.
**
**		i_EX_setcontext() - static
**			Default set context routine for non-threaded users.
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
**		i_EXnext()
**			Return next context on the stack.
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
**	04-oct-95 (duursma)
**	    Incorporated this UNIX CL module into alpha vms CL.
**	    Made changes so we conform to coding standard.
**	05-jun-2009 (joea)
**	    Conditionally reinstate previous Alpha vms version.
**	17-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
*/

/* Forward declarations */

static void i_EX_setcontext(EX_CONTEXT *context);
static EX_CONTEXT **i_EX_getcontext(void);

static  EX_CONTEXT	*ex_sptr;

#ifdef OS_THREADS_USED

static VOID        i_EX_t_setcontext(EX_CONTEXT *);
static EX_CONTEXT  **i_EX_t_getcontext(void);

static ME_TLS_KEY   EXcontextkey = 0;
static  VOID         (*i_EXsetcontext)(EX_CONTEXT *) = i_EX_t_setcontext;
static  EX_CONTEXT  **(*i_EXgetcontext)(void) = i_EX_t_getcontext;

# else /* OS_THREADS_USED */

static void (*i_EXsetcontext)(EX_CONTEXT *) = i_EX_setcontext;
static EX_CONTEXT **(*i_EXgetcontext)(void) = i_EX_getcontext;

# endif

/*
** EX_initfunc(
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
i_EX_initfunc(
    void (*setfunc)(EX_CONTEXT *),
    EX_CONTEXT **(*getfunc)(void))
{
	i_EXsetcontext = setfunc ? setfunc : i_EX_setcontext;
	i_EXgetcontext = getfunc ? getfunc : i_EX_getcontext;
}


/*
** EX_getcontext
**
**	Return a pointer to the current exception stack in a non-threaded
**	environment.
**
** HISTORY
**	26-aug-89 (rexl)
**	    written.
*/
static EX_CONTEXT **
i_EX_getcontext()
{
	return( &ex_sptr );
}

/*
** EX_setcontext
**
**	Set the current exception stack pointer in a non-threaded environment.
**
** HISTORY
**	26-aug-89 (rexl)
**	    written.
*/
static void
i_EX_setcontext(EX_CONTEXT *context)
{
	ex_sptr = context;
}


/*
** EXtop()
**
**	Return the topmost scope on the exception stack.
**
** HISTORY
**	12/11/84 Written (sat).
*/
EX_CONTEXT **
i_EXtop()
{
	return ((*i_EXgetcontext)());
}

/*
** EXpush(contest)
**
**	Push the scope on the stack.
**
** HISTORY
**	12/11/84 Written (sat).
**
**      02-sep-93 (smc)
**          Made context cast a PTR to be portable to axp_osf.
**	13-oct-95 (duursma)
**	    Changed that back to u_i4 since that's the type of address_check.
*/
void
i_EXpush( EX_CONTEXT *context )
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
    {
        (*i_EXsetcontext)(context);
    }
}

/*
** EXpop()
**
**	Removes topmost scope from stack.
**
** HISTORY
**	12/11/84 Written (sat)
*/
void
i_EXpop(EX_CONTEXT  **excp)
{
        *excp = (*excp)->prev_context;
}

EX_CONTEXT	*
i_EXnext( EX_CONTEXT *context )
{
	return(context->prev_context);
}

# ifdef OS_THREADS_USED
/*
** EX_CONTEXT **
** i_EX_t_getcontext()
**
**  Return a pointer to the current exception stack pointer in a
**  threaded environment.
**
** HISTORY
**  03-jun-1996 (canor01)
**      written.
**  27-Dec-2000 (jenjo02)
**      Return **EX_CONTEXT instead of *EX_CONTEXT.
*/

static EX_CONTEXT **
i_EX_t_getcontext(void)
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
** EX_CONTEXT   *context;
**
**  Set the current exception stack pointer in a threaded environment.
**
** HISTORY
**  03-jun-1996 (canor01)
**      written.
**  27-Dec-2000 (jenjo02)
**      For OS threads, only called when thread local storage
**      does not yet exist, obviating ME_tls_get() call.
*/

static VOID
i_EX_t_setcontext( EX_CONTEXT *context )
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
