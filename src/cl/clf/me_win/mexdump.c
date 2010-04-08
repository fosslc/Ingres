/*
**	Copyright (c) 1983 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<er.h>
# include	<me.h>
# include	<si.h>

/*
**
**	MExdump.c
**		contains the routines
**			MExdump     -- print the specified list
**			MExnodeDump -- print a node
**			MExheadDump -- print head of list
**
** History:
**		2-Feb-1987 (daveb) Abstracted from medump/mebdump
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
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	06-dec-1999 (somsa01)
**	    Use SIZE_TYPE for lengths. Also, moved MEerror into here as
**	    a static function as well as the define for the REPORT_ERR
**	    macro.
*/


/*
**
**	Name:
**		MEdump.c
**
**	Function:
**		MEdump
**
**	Arguments:
**		i4 chain;
**		i4 (*printf)();	printf-like function for messages,
**					must be a VARARGS function
**
**	Result:
**		Shows the allocated and free lists for this process using
**		the supplied format/print function.
**
**		Returns STATUS: OK, ME_00_CMP, ME_BD_CHAIN, ME_OUT_OF_RANGE.
**
**	Side Effects:
**		None
**
**	History:
**		03/83 - (gb)
**			written
**	21-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**	05-feb-93 (smc)
**	    Forward declared MExnodedump & MExheaddump.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in MExnodedump(),
**	    and MExheaddump().
**	28-oct-93 (swm)
**	    Bug #56448
**	    Added comments to guard against future use of a non-varargs
**	    function in (*fmt) function pointers.
*/
 

/* forward declarations */
static STATUS MExnodedump();
static STATUS MExheaddump();
static STATUS MEerror(STATUS error_status);

/*
** Macro used to print errors (if any) from the given function when
** executed.
*/
# define REPORT_ERR(routine)	if ((MEstatus = (routine)) != OK) \
					(VOID) MEerror(MEstatus)

STATUS
MExdump(
	i4	chain,
	i4	(*fmt)())	/* must be a VARARGS function */
{
    register ME_HEAD	*head;
    register ME_NODE	*blk;
    char		*which;

    STATUS	mexnodedump();
    STATUS	MExheaddump();

    MEstatus = OK;

    switch(chain)
    {
	case ME_D_BOTH:
	    REPORT_ERR(MExdump(ME_D_ALLOC, fmt));
	    if (MEstatus == OK)
		REPORT_ERR(MExdump(ME_D_FREE, fmt));
	    break;

	case ME_D_ALLOC:
	    head = &MElist;
	    which = "Allocated";
	    break;

	case ME_D_FREE:
	    head = &MEfreelist;
	    which = "Free";
	    break;

	default:
	    MEstatus = ME_BD_CHAIN;
	    break;
    }

    if ((chain != ME_D_BOTH) && (MEstatus == OK))
    {
	(void)(*fmt)("\n'%s' chain begins at 0x%08x\n", which,  head);

	REPORT_ERR( MExheaddump( head, fmt ) );
	if (MEstatus == OK)
	{
	    blk = head->MEfirst;
	    while ( blk != (ME_NODE *)head && MEstatus == OK )
	    {
		REPORT_ERR( MExnodedump( blk, fmt ) );
		if (MEstatus == OK)
		{
		    if( blk == blk->MEnext )
		    {
			(void)(*fmt)("\tLOOP DETECTED\n");
			break;
		    }
		    blk = blk->MEnext;
		}
	    }
	}
    }

    return(MEstatus);
}


static STATUS
MExnodedump(
	register ME_NODE *node,
	i4 (*fmt)())		/* must be a VARARGS function */
{
    MEstatus = OK;

    if (node == NULL)
    {
	MEstatus = ME_00_DUMP;
	(void)(*fmt)("MExnodedump: node is NULL\n" );
    }	
    else if ( OUT_OF_DATASPACE(node) )
    {
	MEstatus = ME_OUT_OF_RANGE;
	(void)(*fmt)("MExnodedump: node 0x%p out of range\n", node );
    }	
    else
	(void)(*fmt)("0x%08x->{ next 0x%08x  prev 0x%08x  size %d  tag %d }\n",
		node, node->MEnext, node->MEprev, node->MEsize, node->MEtag);

    return( MEstatus );
}


static STATUS
MExheaddump(
	register ME_HEAD *head,
	i4 (*fmt)())		/* must be a VARARGS function */
{
    MEstatus = OK;

    if (head == NULL)
    {
	MEstatus = ME_00_DUMP;
	(void)(*fmt)("MExheaddump: head is NULL\n");
    }
    else if ( OUT_OF_DATASPACE(head) )
    {
	MEstatus = ME_OUT_OF_RANGE;
	(void)(*fmt)("MEXheaddump: head 0x%p is out of range\n", head );
    }
    else
	(*fmt)( "0x%08x->{ first 0x%08x last 0x%08x }\n",
		head, head->MEfirst, head->MElast );

    return(MEstatus);
}


SIZE_TYPE
MEtotal(
	i4 which)
{
    register SIZE_TYPE tot;
    register ME_NODE *node;
    ME_HEAD *list;
    
    list = which == ME_D_FREE ? &MEfreelist : &MElist;
    tot = 0;
    for(node = list->MEfirst; node != (ME_NODE *)list; node = node->MEnext)
    	tot += node->MEsize;

    return( tot );
}




i4
MEobjects(
	i4 which)
{
    register i4 tot;
    register ME_NODE *node;
    ME_HEAD *list;
    
    list = which == ME_D_FREE ? &MEfreelist : &MElist;
    tot = 0;
    for(node = list->MEfirst; node != (ME_NODE *)list; node = node->MEnext)
    	tot++;

    return( tot );
}

static STATUS
MEerror(
	STATUS	error_status)
{
    char	ME_err_buf[ER_MAX_LEN];

    ERreport(error_status, ME_err_buf);
    SIfprintf(stderr, "%s\n", ME_err_buf);
    return( OK );
}

