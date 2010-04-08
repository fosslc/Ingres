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
** Name:	cucgbf.c	- CURECORD structure for gbf objects
**
** Description:
**	This file contains the CURECORD structures for details
**	lines for gbf objects.
**
** History:
**	4-jul-1987 (Joe)
**		Initial Version
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

GLOBALDEF CUTLIST   IICUgbtGBfTlist[] =
{
    {CUFILE,	1,	ERx("graphid"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	2,	ERx("gowner"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	3,	ERx("gdate"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	4,	ERx("gtupcnt"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	5,	ERx("gvalcnt"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	6,	ERx("gseries"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	7,	ERx("gserlen"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	8,	ERx("glabels"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	9,	ERx("glbllen"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	10,	ERx("xtype"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	11,	ERx("ytype"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	12,	ERx("ztype"),	{NULL, 4, DB_INT_TYPE}},
};

STATUS	IICUcgoCopyGbfgraphOut();

GLOBALDEF CURECORD IICUgbrGBfRecord =
{
    sizeof(IICUgbtGBfTlist)/sizeof(IICUgbtGBfTlist[0]),
    IICUgbtGBfTlist,
    ERx("graphs"),
    FALSE,
    FALSE,
    NULL,		/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudelete */
    NULL,		/* cudeltab */
    IICUcgoCopyGbfgraphOut,		/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};

/*
** For CUC_GCOMMANDS
*/
GLOBALDEF CUTLIST   IICUgctGCommandsTlist[] =
{
    {CUFILE,	1,	ERx("graphid"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	2,	ERx("gowner"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	3,	ERx("gseqno"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	4,	ERx("gcontinue"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	5,	ERx("gmodule"),	{NULL, 12, DB_CHR_TYPE}},
    {CUFILE,	6,	ERx("gcommand"),	{NULL, 12, DB_CHR_TYPE}},
    {CUFILE,	7,	ERx("gint"),		{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	8,	ERx("gflt"),		{NULL, 8, DB_FLT_TYPE}},
    {CUFILE,	9,	ERx("gstr"),		{NULL, 255, DB_CHR_TYPE}},
};

GLOBALDEF CURECORD IICUgcrGCommandsRecord =
{
    sizeof(IICUgctGCommandsTlist)/sizeof(IICUgctGCommandsTlist[0]),
    IICUgctGCommandsTlist,
    ERx("gcommands"),
    FALSE,
    FALSE,
    NULL,		/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudeltab */
    NULL,		/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};
