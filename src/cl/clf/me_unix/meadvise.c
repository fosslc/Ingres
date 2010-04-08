
# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<me.h>

/*
**
**Copyright (c) 2004 Ingres Corporation
**
**	Name:
**		MEadvise.c
**
**	Function:
**		MEadvise
**
**	Arguments:
**		i4	flag;
**
**		'flag' is one of the following values from me.h
**		ME_USER_ALLOC		(Use system provided allocator)
**		ME_INGRES_ALLOC		(Use INGRES ME allocator MEreqmem etc..)
**		ME_INGRES_THREAD_ALLOC	(Identical to ME_INGRES_ALLOC
**					 except that the allocator
**					 must handle the complications of
**					 a multi-threaded address space)
**		ME_FASTPATH_ALLOC	(Use system provided allocator
**					 but allow shared memory attach)
**
**	Result:
**		Returns STATUS: OK, FAIL.
**
**	Side Effects:
**		Sets the state of the MEreqmem system.  Must be called
**		before any allocations have occurred.
**
**	History:
**		17-Dec-1986 -- (daveb)
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**	07-Jan-91 (anton)
**	    Add support for ME_INGRES_THREAD_ALLOC
**	21-Jan-91 (jkb)
**	    ifdef ME_INGRES_THREAD_ALLOC change for non-MCT 
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      18-Aug-93 (bobm)
**          Add purify_override_MEadvise to ignore setting when using
**          purify - We want the system allocator so that purify can
**          track it.
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	08-nov-1995 (canor01)
**	    Add comment on ME_FASTPATH_ALLOC
**	03-jun-1996 (canor01)
**	    For operating system thread support, MEsetup may already
**	    been set by the time an official MEadvise() is done.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
[@history_template@]...
*/


STATUS
MEadvise(
	i4 flag)
{
# ifdef OS_THREADS_USED
    if( MEgotadvice )
	return( FAIL );
    /*
    ** did the setup take place as thread library initialization? 
    ** If so, we bypassed the MO call.
    */
    if ( MEsetup && !MEgotadvice )
        ME_mo_reg();

# else /* OS_THREADS_USED */
    if( MEgotadvice || MEsetup )
	return( FAIL );
		
    if (flag == ME_INGRES_THREAD_ALLOC)
    {
	flag = ME_INGRES_ALLOC;
    }
# endif /* OS_THREADS_USED */
    MEgotadvice = TRUE;
    MEadvice = flag;
    return( OK );
}
