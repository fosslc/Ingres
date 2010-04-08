/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<cu.h>
# include	<si.h>
# include	<er.h>
# include	"ercu.h"

/**
** Name:	cuprint.c	- Print out structures.
**
** Description:
**	The routines in this file are intended for debugging or
**	testing.  The print the structures of the copyutl library.
**
**	IICUpcaPrintCurecordArgv
**
** History:
**	5-jul-1987 (joe)
**		Initial Version.
**	1-dec-1993 (smc)
**		Bug #56449
**		Changed printing of address to use %p.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

/*{
** Name:	IICUpcaPrintCurecordArgv	- Print a CURECORD's argv.
**
** Description:
**	Print out the values pointed to by a CURECORD's argv.
**
** Inputs:
**	fp		File to print to using SIfprintf.
**
**	curec		The CURECORD.
**
** History:
**	5-jul-1987 (Joe)
**		Initial Version.
**	19-oct-88 (kenl)
**		Adapted routine to use SQLDA.  This has not been tested.
*/
IICUpcaPrintCurecordArgv(fp, curec)
FILE		*fp;
CURECORD	*curec;
{
    IISQLDA	*a;
    CUTLIST	*t;
    i4		i;

    if (curec->cuinit)
    {
	SIfprintf(fp, ERx("The CURECORD at address %p has not been inited\n"),
			curec);
	return;
    }
    SIfprintf(fp, ERx("CURECORD: # = %d, table = %s\n"), curec->cunoelms,
			curec->cutable);
    SIfprintf(fp, ERx("\tinsert=%s\n"), curec->cuinsstr);

    a = curec->cuargv;

    for (i = 0, t = curec->cutlist;
	 i < curec->cunoelms;
	 i++, t++)
    {
	SIfprintf(fp, ERx("\tTlist[%d], source=%d, offset=%d, name=%s "),
			i, t->cuinssrc, t->cuoffset, t->cuname);
	switch(t->cudbv.db_datatype)
	{
	  case DB_INT_TYPE:
	    SIfprintf(fp, ERx("integer: %d\n"),
			*((i4 *) *(a->sqlvar[i].sqldata)));
	    break;

	  case DB_FLT_TYPE:
	    SIfprintf(fp, ERx("float: %f\n"),
			*((f8 *) *(a->sqlvar[i].sqldata)));
	    break;

	  case DB_CHR_TYPE:
	    SIfprintf(fp, ERx("character: %s\n"), *(a->sqlvar[i].sqldata));
	    break;

	}
    }
}
