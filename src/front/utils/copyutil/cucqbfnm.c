/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<cu.h>
# include	<si.h>
# include	"ercu.h"

/**
** Name:	cucqbfnm.c	- CURECORD structure for QBFnames
**
** Description:
**	This file contains the CURECORD structures for details
**	lines for OC_QBFNAME
**
** History:
**	4-jul-1987 (Joe)
**		Initial Version
**	09-nov-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**	89/09  (jhw)  Removed unnecessary cast for AIX compiler.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      13-Oct-2008 (hanal04) Bug 120290
**          Initialise new cuupdabfsrc field in CURECORD to FALSE.
**/

GLOBALDEF CUTLIST   IICUqntQbfNameTlist[] =
{
    {CUFILE,	1,	ERx("relname"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	2,	ERx("frname"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	3,	ERx("qbftype"),	{NULL, 4, DB_INT_TYPE}},
    {CUID,	1,	ERx("object_id"),	{NULL, 4, DB_INT_TYPE}}
};

static CUDELLIST IICUqdtQbfnameDelTab [] =
{
    {ERx("ii_qbfnames"), ERx("object_id")},
    {NULL, NULL}
};

STATUS	IICUcqoCopyQbfnameOut();

GLOBALDEF CURECORD IICUqnrQbfNameRecord =
{
    sizeof(IICUqntQbfNameTlist)/sizeof(IICUqntQbfNameTlist[0]),
    IICUqntQbfNameTlist,
    ERx("ii_qbfnames"),
    FALSE,
    FALSE,
    NULL,		/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudelete */
    IICUqdtQbfnameDelTab,  /* cudeltab */
    IICUcqoCopyQbfnameOut,	/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};
