# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<cs.h>
# include	<qu.h>
# include	<me.h>
# include	<meprivate.h>

/*
 *	Copyright (c) 1983, 1999 Ingres Corporation
 *
 *	Name:
 *		MEinitLists.c
 *
 *	Function:
 *		MEinitLists
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		Initialize the allocate and free list headers.
 *
 *		Called when process attempts first MEreqmem.
 *
 *		Also sets the global MEsetup which prevents process from
 *		attempting to free memory before ever having allocated any.
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83 - (gb) written
 *		24-aug-87 (boba) -- Changed memory allocation to use MEreqmem.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**      08-mar-91 (rudyw)
**          Comment out text following #endif
**	20-nov-92 (pearl)
**		Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**		headers.  Delete forward and external function references;
**              these are now all in either me.h or meprivate.h.
**	21-jan-93 (pearl)
**	    Add #include of me.h.  Need it for references to MEfree and MEsize.	
**	
**      8-jun-93 (ed)
**              changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**	03-jun-1996 (canor01)
**	    Made MEstatus local for use with operating-system threads.
**      25-Sep-2000 (hanal04) Bug 102725 INGSRV 1275.
**          Initialise the new MEbrk_mutex. Also formally initialise
**          the MEalloctab_mutex which was added in the fix for bug 98820.
**	18-apr-1998 (canor01)
**	    Clean up compile warnings.
**  02-dec-1998 (muhpa01)
**		Removed NO_OPTIM for hp8.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	13-Sep-2005 (schka24)
**	    need break-mutex here too.
*/

/* Externs */
# ifdef OS_THREADS_USED
GLOBALREF CS_SYNCH      MEfreelist_mutex;
GLOBALREF CS_SYNCH      MElist_mutex;
GLOBALREF CS_SYNCH      MEtaglist_mutex;
GLOBALREF CS_SYNCH      MEpage_mutex;
GLOBALREF CS_SYNCH      MEalloctab_mutex;
GLOBALREF CS_SYNCH      MEbrk_mutex;	/* May or may not be actually used */
# endif /* OS_THREADS_USED */


STATUS
MEinitLists(
	VOID)
{
    QUinit( (QUEUE *) &MElist );
    QUinit( (QUEUE *) &MEfreelist );
	MEsetup		                 = TRUE;

# ifdef OS_THREADS_USED
    CS_synch_init( &MEfreelist_mutex );
    CS_synch_init( &MElist_mutex );
    CS_synch_init( &MEtaglist_mutex );
    CS_synch_init( &MEpage_mutex );
    CS_synch_init( &MEalloctab_mutex );
    CS_synch_init( &MEbrk_mutex );
# endif /* OS_THREADS_USED */

    return( OK );
}
