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
** Name:	ioirtt.c - run time table read.
**
** Description:
**	IOI routine to read the runtime table
**
**  History:
**	10/14/92 (dkh) - Removed use of ArrayOf in favor of DSTARRAY.
**			 ArrayOf confused the alpha C compiler.
**      07-May-96 (fanra01)
**          bug# 76270
**          Added a call to rtt_init which initialises the template structure.
**          This call has been added to facilitate the initialisation of
**          the template structure with items which are not constant at compile
**          time.
*/

GLOBALREF	DSTARRAY	Rtt_template;

/*{
** Name:	rtt_get - get runtime table from file
**
** Description:
**	get runtime table from file.
**
** Inputs:
**	fp	file pointer
**
** Outputs:
**	abobj	returned runtime table
**
**	return:
**		OK		success
**
** History:
**	9/86 (rlm)	adapted from abrtstab
**      07-May-96 (fanra01)
**          Added a call to rtt_init which initialises the template structure.
**
*/

STATUS
rtt_get(fp,abobj)
FILE *fp;
ABRTSOBJ **abobj;
{
	PTR	objtmp;		 /* temp required for proper casting on DG */
	STATUS	ret_val;
	SH_DESC	sh_desc;

        rtt_init();
	sh_desc.sh_type = IS_RACC;
	sh_desc.sh_reg.file.fp = fp;
	sh_desc.sh_keep = TRUE;
	sh_desc.sh_tag = 0;

	/*
	** Data General distinguishes between byte and word pointers.  On DG
	** a PTR is a CHAR *.  DSload expects a PTR * so we must therefore
	** cast abobj to a PTR (a byte pointer) before calling DSload.  We
	** must cast it back after the call (kenl).
	*/
	objtmp = (PTR) *abobj;
	ret_val = DSload(&sh_desc, &objtmp, &Rtt_template);
	*abobj = (ABRTSOBJ *) objtmp;
	return (ret_val);
}
