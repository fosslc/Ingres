
/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
#include "ioistd.h"
#include <ds.h>
#include <feds.h>

/**
** Name:	ioifstr.c - form store
**
** Description:
**	store form into image file
**
** History:
**	12/10/89 (dkh) - Using pointer to DS template array defined in frame
**			 as part of VMS shared lib changes.
**	09/06/90 (dkh) - Changed to use IIFDgfdGetFdDstab() instead of
**			 IIFDdstab directly.
**	10/14/92 (dkh) - Removed use of ArrayOf in favor of DSTARRAY.
**			 ArrayOf confused the alpha C compiler.
**      03-jun-1999 (hanch04)
**          Change SIfseek to use OFFSET_TYPE and LARGEFILE64 for files > 2gig
*/

FUNC_EXTERN DSTARRAY *IIFDgfdGetFdDstab();

/*{
** Name:	IIOIfsFormStore - store form into image file
**
** Description:
**	Uses DS to store form into an image file.  Seeks to end, stores
**	form and returns position.
**
** Inputs:
**	ohdr	OLIMHDR describing image file
**	addr	form to store
**	dest	flag for destructive store
**
** Outputs:
**	pos	returned file position
**
**	return:
**		OK		success
**
** Side Effects:
**
** History:
**	10/86 (rlm)	written
**      01-dec-1993 (smc)
**		Bug #58882
**		Removed unportable cast of addr in call to DSstore.
**	29-apr-1999 (hanch04)
**	    Changed ftell to return offset.
**
*/

STATUS
IIOIfsFormStore(ohdr,addr,dest,pos)
OLIMHDR *ohdr;
PTR addr;
bool dest;
i4 *pos;
{
	SH_DESC	sh_desc;
	FILE *fp;
	OFFSET_TYPE offset;

	fp = ohdr->fp;

	if (SIfseek(fp,0L,SI_P_END) != OK)
		return (ILE_FRDWR);

	offset = SIftell(fp);
	*pos = offset;

	sh_desc.sh_type = IS_RACC;
	sh_desc.sh_reg.file.fp = fp;
	sh_desc.sh_dstore = dest;

	if (DSstore(&sh_desc, (i4) DS_FRAME, addr,
	    IIFDgfdGetFdDstab()) != OK)
		return (ILE_FRDWR);
	return (OK);
}
