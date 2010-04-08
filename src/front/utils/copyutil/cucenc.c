/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<cu.h>
# include	<er.h>
# include	"ercu.h"


/**
** Name:	cucenc.c	- CURECORD structure for encoded objects
**
** Description:
**	This file contains the CURECORD structures for encoded
**	objects.  Since the encoding contains all the information,
**	and the object manager will do everything to them,
**	these don't have much info.
**
** History:
**	10-aug-1987 (Joe)
**		Initial Version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Nov-2000 (wansh01)
**	    replace i4* with i4 in line 40 
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      13-Oct-2008 (hanal04) Bug 120290
**          Initialise new cuupdabfsrc field in CURECORD to FALSE.
**/

/* # define's */


GLOBALDEF CURECORD IICUenrENcodedRecord =
{
    (i4 ) 0,
    NULL,
    ERx(""),
    FALSE,
    FALSE,
    NULL,		/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudelete */
    NULL,	/* cudeltab */
    NULL,	/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};
