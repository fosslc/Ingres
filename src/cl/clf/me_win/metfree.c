/******************************************************************************
**
**	Copyright (c) 1983 Ingres Corporation
**
******************************************************************************/

#include <compat.h>
#include <clconfig.h>
#include <cs.h>
#include <meprivate.h>
#include <me.h>
#include <qu.h>

/* Externs */
GLOBALREF CS_SEMAPHORE	ME_stream_sem;

/******************************************************************************
**	Name:
**		MEtfree.c
**
**	Function:
**		MEtfree
**
**	Arguments:
**		i2	tag;
**
**	Result:
**		Free all blocks of memory on allocated list with
**		MEtag value of 'tag'.
**
**		Returns STATUS: OK, ME_00_PTR, ME_OUT_OF_RANGE, ME_BD_TAG,
**				ME_FREE_FIRST, ME_TR_TFREE, ME_NO_TFREE.
**
**	Side Effects:
**		None
**  History:
**	03-jun-1996 (canor01)
**	    Internally, store the tag as an i4 instead of an i2. This makes
**	    for more efficient code on byte-aligned platforms that do fixups.
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**
******************************************************************************/
STATUS
MEtfree(u_i2 tag)
{
	STATUS status;

	if (tag == 0)
		return ME_ILLEGAL_USAGE;

# ifdef MCT
	gen_Psem(&ME_stream_sem);
# endif /* MCT */
	status = IIME_ftFreeTag((i4)tag);
# ifdef MCT
	gen_Vsem(&ME_stream_sem);
# endif /* MCT */
	return (status);
}
