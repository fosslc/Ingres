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
** Name:	cucjoind.c	- CURECORD structure for joindef
**
** Description:
**	This file contains the CURECORD structures for details
**	lines for CU_JOINDEF
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

GLOBALDEF CUTLIST   IICUjdtJoinDefTlist[] =
{
    {CUFILE,	1,	ERx("qtype"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	2,	ERx("qinfo1"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	3,	ERx("qinfo2"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	4,	ERx("qinfo3"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	5,	ERx("qinfo4"),	{NULL, 32, DB_CHR_TYPE}},
    {CUID,	1,	ERx("object_id"),	{NULL, 4, DB_INT_TYPE}}
};

static CUDELLIST IICUjdtJoindefDelTab [] =
{
    {ERx("ii_joindefs"), ERx("object_id")},
    {NULL, NULL}
};

STATUS	IICUcjoCopyJoindefOut();

GLOBALDEF CURECORD IICUjdrJoinDefRecord =
{
    sizeof(IICUjdtJoinDefTlist)/sizeof(IICUjdtJoinDefTlist[0]),
    IICUjdtJoinDefTlist,
    ERx("ii_joindefs"),
    FALSE,
    FALSE,
    NULL,		/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudelete */
    IICUjdtJoindefDelTab,  /* cudeltab */
    IICUcjoCopyJoindefOut,	/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};
