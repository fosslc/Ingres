/*
**  Copyright (c) 1983, 2002 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<qu.h>
# include	<me.h>
# include	<meprivate.h>

/*
**  Name: MEinitLists.c
**
**  Function:
**	MEinitLists
**
**  Arguments:
**	None
**
**  Result:
**	Initialize the allocate and free list headers.
**
**	Called when process attempts first MEreqmem.
**
**	Also sets the global MEsetup which prevents process from
**	attempting to free memory before ever having allocated any.
**
**  Side Effects:
**	None
**
**  History:
**	03/83 (gb)
**	    written
**	24-aug-87 (boba)
**	    Changed memory allocation to use MEreqmem.
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**      08-mar-91 (rudyw)
**          Comment out text following #endif
**	20-nov-92 (pearl)
**	    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**	    headers.  Delete forward and external function references;
**	    these are now all in either me.h or meprivate.h.
**	21-jan-93 (pearl)
**	    Add #include of me.h.  Need it for references to MEfree and MEsize.	
**      8-jun-93 (ed)
**	    changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**	05-jun-2002 (somsa01)
**	    Synched up with UNIX.
*/

/* Externs */
GLOBALREF CS_SYNCH	MEtaglist_mutex;

STATUS
MEinitLists(
	VOID)
{
    QUinit((QUEUE *) &MElist);
    QUinit((QUEUE *) &MEfreelist);
    MEstatus = OK;
    MEsetup = TRUE;

    CS_synch_init(&MEtaglist_mutex);

    return(OK);
}

/*
** This below to ensure that all relevant parts get loaded
** when this module gets loaded. (daveb)
*/


static STATUS (*yanktbl[])() =
{
    MEfree,
    MEsize,
}; 

extern char *MEmemalign();

static
MEnotcalled(
	VOID)
{
    MEcopy((PTR)NULL, (i2)0, (PTR)NULL );
    (void)MEmemalign( 0, 0 );
    (void)MEreqmem( 0, 0, FALSE, (STATUS*)NULL );
}
