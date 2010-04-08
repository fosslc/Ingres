/*
**  Copyright (c) 1983, 2002 Ingres Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	<meprivate.h>
#include	<me.h>
#include	<mo.h>

/*
**  Name:
**	MEadvise.c
**
**  Function:
**	MEadvise
**
**  Arguments:
**	i4	flag;
**
**	'flag' is one of the following values from me.h
**	ME_USER_ALLOC		(Use system provided allocator)
**	ME_INGRES_ALLOC		(Use INGRES ME allocator MEreqmem etc..)
**	ME_INGRES_THREAD_ALLOC	(Identical to ME_INGRES_ALLOC
**				 except that the allocator
**				 must handle the complications of
**				 a multi-threaded address space)
**
**  Result:
**	Returns STATUS: OK, FAIL.
**
**  Side Effects:
**	Sets the state of the MEreqmem system.  Must be called
**	before any allocations have occurred.
**
**  History:
**	17-Dec-1986 (daveb)
**	    Created.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**	07-Jan-91 (anton)
**	    Add support for ME_INGRES_THREAD_ALLOC
**	21-Jan-91 (jkb)
**	    ifdef ME_INGRES_THREAD_ALLOC change for non-MCT 
**	20-nov-92 (pearl)
**	    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**	    headers.
**	08-feb-1993 (pearl)
**	    Add #include of me.h.
**	8-jun-93 (ed)
**	    changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      18-Aug-93 (bobm)
**          Add purify_override_MEadvise to ignore setting when using
**          purify - We want the system allocator so that purify can
**          track it.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	07-jun-2002 (somsa01)
**	    Synch'ed up with UNIX version.
*/



STATUS
purify_override_MEadvise(flag)
i4 flag;
{
    return OK;
}

STATUS
MEadvise(
	i4 flag)
{
    if (MEgotadvice)
	return(FAIL);

    /*
    ** did the setup take place as thread library initialization?
    ** If so, we bypassed the MO call.
    */
    if (MEsetup && !MEgotadvice)
	ME_mo_reg();

    MEgotadvice = TRUE;
    MEadvice = flag;
    return(OK);
}
