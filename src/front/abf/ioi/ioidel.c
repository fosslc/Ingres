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
** Name:	ioidel.c - delete open image file
**
** Description:
**	delete open image file
*/

/*{
** Name:	IIOIidImgDelete - del image file
**
** Description:
**	Close and delete image file.
**
** Inputs:
**	ohdr	image file information (location and file pointer)
**
** Outputs:
**
**	return:
**		OK		success
**
** History:
**	9/86 (rlm)	written
**
*/

STATUS
IIOIidImgDelete (ohdr)
OLIMHDR *ohdr;
{
	SIclose(ohdr->fp);
	return(LOdelete(&(ohdr->loc)));
}
