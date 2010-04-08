/*
**  Copyright (c) 1983, 2002 Ingres Corporation
*/

#include <compat.h>
#include <clconfig.h>
#include <meprivate.h>
#include <me.h>

/*
**  Name:
**	MEsize.c
**
**  Function:
**	MEsize
**
**  Arguments:
**	i4		block;
**	SIZE_TYPE	*size;
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
**	08-aug-1999 (mcgem01)
**	    Changed longnat to i4.
**	21-mar-2001 (mcgem01)
**	    Instead of using i4's for pointer sizes, use SIZE_TYPE.
**	21-jun-2002 (somsa01)
**	    Sync'ed up with UNIX. Rely on ME_NODE rather than the ptr
**	    UNION.
*/

STATUS
MEsize(
	PTR		block,
	SIZE_TYPE	*size)
{
    register ME_NODE	*blknode;
    STATUS		status = OK;

    if (block == NULL)
	status = ME_00_PTR;
    else
    {
	blknode = (ME_NODE *)((char *)block - sizeof(ME_NODE));
	*size = blknode->MEsize - sizeof(ME_NODE);
    }

    return(status);
}
