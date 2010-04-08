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
# include	<si.h>
# include	"ercu.h"


/**
** Name:	cucvig.c	- CURECORD structure for vig objects
**
** Description:
**	This file contains the CURECORD structures for details
**	lines for vigraph graph objects.
**
** History:
**	10-aug-1987 (Joe)
**		Initial Version
**	2-dec-1986 (peter)
**		Add cudelrecord for Vigraph.
**	09-nov-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-nov-2000 (wansh01)
**	    replace i4* to i4 in line 60  
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      13-Oct-2008 (hanal04) Bug 120290
**          Initialise new cuupdabfsrc field in CURECORD to FALSE.
**/

/* # define's */


/*
** Since graphs are wholly contained in the object manager's tables,
** there is no detail lines for graphs, consequently no target list.
*/

static	CUDELLIST	IICUvdtVigDelTab [] =
{
	{ERx("ii_encodings"), ERx("encode_object")},
	{NULL, NULL}
};

FUNC_EXTERN STATUS	IICUcvoCopyVigraphOut();

GLOBALDEF CURECORD IICUvirVIgraphRecord =
{
    (i4 ) 0,
    NULL,
    ERx(""),
    FALSE,
    FALSE,
    NULL,		/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudelete */
    IICUvdtVigDelTab,	/* cudeltab */
    IICUcvoCopyVigraphOut,	/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};
