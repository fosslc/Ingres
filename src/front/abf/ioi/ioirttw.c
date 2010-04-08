/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include "ioistd.h"
#include <ds.h>
#include <feds.h>

/**
** Name:	ioirtt.c - write run time table
**
** Description:
**	IOI routines to write the runtime table, using DS
**
**  History:
**	10/14/92 (dkh) - Removed use of ArrayOf in favor of DSTARRAY.
**			 ArrayOf confused the alpha C compiler.
**      07-May-96 (fanra01)
**          Bug# 76270
**          Added a call to rtt_init which initialises the template structure.
**          This call has been added to facilitate the initialisation of
**          the template structure with items which are not constant at compile
**          time.
**      29-apr-1999 (hanch04)
**          Changed ftell to return offset.
**      03-jun-1999 (hanch04)
**          Change SIfseek to use OFFSET_TYPE and LARGEFILE64 for files > 2gig
*/

GLOBALREF DSTARRAY Rtt_template;

/*{
** Name:	rtt_write - write runtime table
**
** Description:
**	Write run time table to file, returning position for trailer.
**
** Inputs:
**	abobj	runtime table
**	fp	file pointer, parked at appropriate place for write
**
** Outputs:
**	pos	position of runtime table
**
**	return:
**		OK		success
**
** History:
**	9/86 (rlm)	adapted from abrtstab
**	10-sep-93 (kchin)
**		Removed cast of 'abobj' to i4 when calling DSstore(). 
**		The cast to i4 could result in truncation of addresses
**		on 64-bit platform such as Alpha OSF/1.
**      07-May-96 (fanra01)
**          Added a call to rtt_init which initialises the template structure.
**
*/

STATUS
rtt_write(abobj,fp,pos)
ABRTSOBJ	*abobj;
FILE	*fp;
i4	*pos;
{
	SH_DESC	sh_desc;
	OFFSET_TYPE offset;

        offset = SIftell(fp);
        *pos = offset;

	sh_desc.sh_type = IS_RACC;
	sh_desc.sh_reg.file.fp = fp;
	sh_desc.sh_dstore = FALSE;
        rtt_init();
	return (DSstore(&sh_desc, (i4)DS_AROBJ, abobj, &Rtt_template));
}
