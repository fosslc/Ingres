/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
#include "ioistd.h"

/**
** Name:	ioicls.c -  Close image file 
**
** Description:
**	close image file, writing rtt / header.
*/

/*{
** Name:	IIOIicImgClose - close image file.
**
** Description:
**	Close image file after writing the run time table and header
**	information onto the end of it.
**
** Inputs:
**	ohdr	image file information
**
** Outputs:
**
**	return:
**		OK		success, file closed
**				on failure, image file is still open.
**
** History:
**	9/86 (rlm)	written
**
*/

STATUS
IIOIicImgClose(ohdr)
OLIMHDR *ohdr;
{
	STATUS rc;
	i4 pos;

	/*
	** write the run-time table.
	*/
	if ((rc = rtt_write(ohdr->rtt,ohdr->fp,&pos)) != OK)
		return(rc);

	/*
	** write image file trailer, and close file.  Interaction type
	** is IORI_IMAGE.
	*/
	if ((rc = trail_write(pos,IORI_IMAGE,ohdr->fp)) != OK)
		return(rc);

	SIclose(ohdr->fp);
	return(OK);
}
