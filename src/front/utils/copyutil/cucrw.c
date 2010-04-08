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
** Name:	cucrw.c	- CURECORD structure for report objects
**
** Description:
**	This file contains the CURECORD structures for details
**	lines for report objects.
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

GLOBALDEF CUTLIST   IICUrwtReportWriterTlist[] =
{
    {CUFILE,	1,	ERx("reptype"),	{NULL, 1, DB_CHR_TYPE}},
    {CUFILE,	2,	ERx("repacount"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	3,	ERx("repscount"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	4,	ERx("repqcount"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	5,	ERx("repbcount"),	{NULL, 4, DB_INT_TYPE}},
    {CUID,	1,	ERx("object_id"),	{NULL, 4, DB_INT_TYPE}}
};

static CUDELLIST IICUrdtReportDelTab [] =
{
    {ERx("ii_reports"), ERx("object_id")},
    {ERx("ii_rcommands"), ERx("object_id")},
    {NULL, NULL}
};

STATUS	IICUcroCopyReportOut();

GLOBALDEF CURECORD IICUrwrReportWriterRecord =
{
    sizeof(IICUrwtReportWriterTlist)/sizeof(IICUrwtReportWriterTlist[0]),
    IICUrwtReportWriterTlist,
    ERx("ii_reports"),
    FALSE,
    FALSE,
    NULL,		/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudelete */
    IICUrdtReportDelTab,  /* cudeltab */
    IICUcroCopyReportOut,	/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};

/*
** For CUC_RCOMMANDS
*/
GLOBALDEF CUTLIST   IICUrctRCommandsTlist[] =
{
    {CUFILE,	1,	ERx("rcotype"),	{NULL, 2, DB_CHR_TYPE}},
    {CUFILE,	2,	ERx("rcosequence"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	3,	ERx("rcosection"),	{NULL, 12, DB_CHR_TYPE}},
    {CUFILE,	4,	ERx("rcoattid"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	5,	ERx("rcocommand"),	{NULL, 12, DB_CHR_TYPE}},
    {CUFILE,	6,	ERx("rcotext"),	{NULL, 100, DB_CHR_TYPE}},
    {CUID,	1,	ERx("object_id"),	{NULL, 4, DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUrcrRCommandsRecord =
{
    sizeof(IICUrctRCommandsTlist)/sizeof(IICUrctRCommandsTlist[0]),
    IICUrctRCommandsTlist,
    ERx("ii_rcommands"),
    FALSE,
    FALSE,
    NULL,		/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudelete */
    NULL,		/* cudeltab */
    NULL,		/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};
