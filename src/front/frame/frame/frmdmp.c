/*
**	frmdmp.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	frmdmp.c - Dump a FRAME structure.
**
** Description:
**	This file contains a routine to dump a FRAME structure.
**	The file is:
**	- FDfrmdmp - Dump a FRAME structure.
**
** History:
**	2/16/82 (ps)	written.
**	02/15/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	"fdfuncs.h"
# include	<si.h>
# include	<er.h>




/*{
** Name:	FDfrmdmp - Dump a FRAME structure.
**
** Description:
**	Print out a frame structure to "stdout", used in debugging.
**
** Inputs:
**	f	FRAME to dump.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/16/82 (ps) - Initial version.
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDfrmdmp(f)
register FRAME	*f;
{
	register FIELD		**fd;
	register TRIM		**t;
	i4			i;

	SIprintf(ERx("\tAddress of FRAME structure:%p\r\n"),f);

	if (f==NULL)
		return;

	if (f->frname!=NULL)
	{
		SIprintf(ERx("\t\tfrname:%s\r\n"),f->frname);
	}

	SIprintf(ERx("\t\tfrfld add:%p\r\n"), f->frfld);
	SIprintf(ERx("\t\tfrfldno:%d\r\n"), f->frfldno);

	for(fd = f->frfld, i = 1; i <= f->frfldno; i++, fd++)
	{
		SIprintf(ERx("\t\t\tfield %d address:%p\r\n"), i, *fd);
	}

	SIprintf(ERx("\t\tfrnsfld add:%p\r\n"), f->frnsfld);
	SIprintf(ERx("\t\tfrnsno:%d\r\n"), f->frnsno);

	for(fd = f->frnsfld, i = 1; i <= f->frnsno; i++, fd++)
	{
		SIprintf(ERx("\t\t\tn.s. field %d address:%p\r\n"), i, *fd);
	}

	SIprintf(ERx("\t\tfrtrim add:%p\r\n"), f->frtrim);
	SIprintf(ERx("\t\tfrtrimno:%d\r\n"), f->frtrimno);

	for(t = f->frtrim, i = 1; i <= f->frtrimno; i++, t++)
	{
		SIprintf(ERx("\t\t\ttrim %d address:%p\r\n"), i, *t);
	}

	SIprintf(ERx("\t\tfrscr add:%p\r\n"), f->frscr);
	SIprintf(ERx("\t\tunscr add:%p\r\n"), f->unscr);
	SIprintf(ERx("\t\tuntwo add:%p\r\n"), f->untwo);
	SIprintf(ERx("\t\tfrintrp:%d\r\n"), f->frintrp);
	SIprintf(ERx("\t\tfrmaxx:%d"), f->frmaxx);
	SIprintf(ERx("\t\tfrmaxy:%d\r\n"), f->frmaxy);
	SIprintf(ERx("\t\tfrposx:%d"), f->frposx);
	SIprintf(ERx("\t\tfrposy:%d\r\n"), f->frposy);
	SIprintf(ERx("\t\tfrmode:%d"), f->frmode);
	SIprintf(ERx("\t\tfrchange:%d\r\n"), f->frchange);
	SIprintf(ERx("\t\tfrnumber:%d"), f->frnumber);
	SIprintf(ERx("\t\tfrcurfld:%d\r\n"), f->frcurfld);

	if (f->frcurnm != NULL)
	{
		SIprintf(ERx("\t\tfrcurnm:%s\r\n"), f->frcurnm);
	}

	SIprintf(ERx("\t\tfrmflags:%d\r\n"), f->frmflags);

	SIflush(stdout);
	return;
}
