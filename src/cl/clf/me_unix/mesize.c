/*
**  Copyright (c) 1983, 2002 Ingres Corporation
*/

# include    	<compat.h>
# include    	<gl.h>
# include  	<clconfig.h>
# include	<meprivate.h>
# include	<me.h>

/*
**  Name:
**	MEsize.c
**
**  Function:
**	MEsize
**
**  Arguments:
**	i4	block;
**	i4	*size;
**
**  Result:
**	return in 'size' the number of (usable) bytes in block
**	of memory whose address is stored in 'block' which must
**	have been allocated by ME[t]alloc().
**
**	Returns STATUS: OK, ME_00_PTR, ME_TR_SIZE.
**
**  Side Effects:
**	None
**
**  History:
**	03/83 - (gb)
**	    written
**	26-aug-89 (rexl)
**	    changed MEstatus to a local status variable.
** 	21-may-90 (blaise)
**          Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**      23-may-90 (blaise)
**          Add "include <compat.h>."
**	20-nov-92 (pearl)
**	    Add #ifdef for "CL_BUILD_WITH_PROTOS" and prototyped function
**	    headers.
**      08-feb-1993 (pearl)
**          Add #include of me.h.
**      8-jun-93 (ed)
**	    changed to use CL_PROTOTYPED
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	11-aug-93 (ed)
**	    unconditional prototypes
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	11-oct-2002 (somsa01)
**	    In the case of LP64, make sure we're dealing with an aligned
**	    blknode.
**	31-oct-2002 (devjo01)
**	    Add cast to previous change to supress compiler warning.
*/



STATUS
MEsize(
	PTR		block,
	SIZE_TYPE	*size)
{
    register ME_NODE *blknode;

    STATUS	status = OK;

	if (block == NULL)
	{
	status = ME_00_PTR;
    }
		else
    {
	blknode = (ME_NODE *)((char *)block - sizeof(ME_NODE));
#ifdef LP64
	blknode = (ME_NODE *)ME_ALIGN_MACRO(blknode, sizeof(ALIGN_RESTRICT));
#endif  /* LP64 */
	*size  = blknode->MEsize - sizeof(ME_NODE);
	}

    return((status == ME_CORRUPTED) ? ME_TR_SIZE : status);
}

