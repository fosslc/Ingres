/*
**  Copyright (c) 1983, 2002 Ingres Corporation
*/

#include   	<compat.h>
#include   	<me.h>
#include   	<clconfig.h>
#include   	<meprivate.h>
#include   	<qu.h>

/******************************************************************************
**	Name:
**		MEdoAlloc.c
**
**	Function:
**		MEdoAlloc
**
**	Arguments:
**		i2		tag;
**		SIZE_TYPE	size;
**		PTR		*block;
**		bool		zero;
**
**	Result:
**		Sets 'block' to pointer to memory which can hold
**		an object of 'size' bytes, tagged by 'tag'.
**
**		If 'zero' initialize 'block' to all 0's.
**
**		If something goes wrong 'block' will be set to NULL.
**
**		Returns STATUS: OK, ME_NO_ALLOC, ME_GONE,
**				ME_00_PTR, ME_CORRUPTED.
**
**		called by ME[t][c]alloc() and MEreqmem().
**
**	Side Effects:
**		None
**
******************************************************************************/

/******************************************************************************
**
**  Forward and/or External declarations
**
******************************************************************************/

GLOBALREF SIZE_TYPE  i_meactual;
GLOBALREF SIZE_TYPE  i_meuser;

/******************************************************************************
**  History:
**	20-may-95 (fanra01)
**	    GlobalAlloc changed to malloc to avoid conflicts with other
**	    allocations.
**	21-mar-1996 (canor01)
**	    Get memory from calling process's heap.
**	03-jun-1996 (canor01)
**	    Internally store the tag as an i4.  This makes for more efficient
**	    code on byte-aligned platforms that do fixups.
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	07-feb-2001 (somsa01)
**	    Instead of using i4's for pointer sizes, use SIZE_TYPE. This is
**	    because, on IA64, we need to use 64-bit sizes.
**	07-jun-2002 (somsa01)
**	    Added running of MEinitLists().
**	21-jun-2002 (somsa01)
**	    Sync'ed up with UNIX. Rely on ME_NODE rather than the ptr
**	    UNION. Also, call HeapCompact() if we initially cannot grab
**	    memory via HeapAlloc().
**	05-Aug-2005 (drivi01)
**		Replaced HeapAlloc calls with malloc and calloc.
**
******************************************************************************/
STATUS
MEdoAlloc(i2        tag,
	  i4        num,
	  SIZE_TYPE size,
	  PTR       *block,
	  bool      zero)
{
    SIZE_TYPE		nsize;
    register ME_NODE	*node;

    if (size <= 0)
	return ME_NO_ALLOC;

    if (!MEsetup)
	MEinitLists();

    nsize = SIZE_ROUND(size);

	if (zero)
		node = calloc(nsize, 1);
	else
		node = malloc(nsize);
    if (!node)
    {
	/*
	** Compact the heap and try again.
	*/
	HeapCompact(GetProcessHeap(), 0);
	if (zero)
		node = calloc(nsize, 1);
	else
		node = malloc(nsize);
	if (!node)
	{
	    return ME_GONE;
	}
	}

    if (tag) 
	IIME_atAddTag(tag, node);

    node->MEtag = tag;
    node->MEsize = nsize;
    node->MEaskedfor = size;

    /* Fill in the returned pointer */
    *block = (PTR)((char *)node + sizeof(ME_NODE));

    return OK;
}
