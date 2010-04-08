/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include "ioistd.h"

/**
** Name:	ioitcrt.c - create test file
**
** Description:
**	Create test file with rtt and hdr
*/

/*{
** Name:	IIOItcTstfilCreate - create test file
**
** Description:
**	Create test file containing run-time table for ABF to pass to
**	interpreter.  This is essentially an empty image file.
**
** Inputs:
**	fname	file name
**	rts	run time table
**
** Outputs:
**
**	return:
**		OK		success
**		ILE_FILOPEN	cannot open file
**
** Side Effects:
**
** History:
**	9/86 (rlm)	written
**
*/

STATUS
IIOItcTstfilCreate(fname,rts)
char *fname;
ABRTSOBJ *rts;
{
	STATUS rc;
	OLIMHDR ohdr;
	i4 pos;

	/*
	** open "image" file.
	*/
	if (IIOIioImgOpen(fname,ERx("w"),&ohdr) != OK)
		return (ILE_FILOPEN);

	/*
	** write the run-time table.  Returned position should be zero,
	** in this case.
	*/
	if ((rc = rtt_write(rts,ohdr.fp,&pos)) != OK)
	{
		IIOIidImgDelete(&ohdr);
		return(rc);
	}

	/*
	** write image file trailer, and close file.  Interaction type
	** is IORI_DB.
	*/
	if ((rc = trail_write(pos,IORI_DB,ohdr.fp)) != OK)
	{
		IIOIidImgDelete(&ohdr);
		return(rc);
	}
	SIclose(ohdr.fp);
	return(OK);
}
