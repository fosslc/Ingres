/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/
#include <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include <fe.h>
#include "ioistd.h"
#include "ioifil.h"

# ifdef CDOS

/**
** Name:	ioiswap.c - close / reopen files for swap operations
**
** Description:
**	handles close / reopen of image files for concurrent DOS
**	process swaps.
*/

/*{
** Name:	IIOIisImgSwap - close image file for swap.
**
** Description:
**	Closes open image file.
**
** Inputs:
**	ohdr	image file context block.
**
**	return:
**		OK		success
**				or status from failed SI routine.
**
** History:
**	6/87 (rlm)	written
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
IIOIisImgSwap (ohdr)
OLIMHDR *ohdr;
{
	return (SIclose(ohdr->fp));
}

/*{
** Name:	IIOIiuImgUnswap - reopen closed image file.
**
** Description:
**	Reopens image file.  Location is saved in ohdr, but caller must
**	pass in mode again.  Caller could actually change mode and get
**	away with it.  The actions taken for "rw" mode in IIOIioImgOpen
**	do not apply here because we are REOPENING the file.
**
** Inputs:
**	ohdr	image file context block.
**	mode	file open mode.
**
**	return:
**		OK		success
**		ILE_FOPEN	failed to open image file.
**
** History:
**	6/87 (rlm)	written
**
*/

STATUS
IIOIiuImgUnswap (ohdr,mode)
OLIMHDR *ohdr;
char *mode;
{
	if (SIfopen(&(ohdr->loc),mode,SI_RACC,IMRECLEN,&(ohdr->fp)) != OK)
		return (ILE_FILOPEN);
	return (OK);
}
# else
static i4	unused;
# endif
