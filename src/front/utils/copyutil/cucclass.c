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
# include	"ercu.h"

/**
** Name:	cucclass.c	- CURECORD structure for class objects
**
** Description:
**	This file contains the CURECORD structures for details
**	lines for OC_RECORD (user-defined classes) and OC_RECMEM (class
**	attributes).
**
** History:
**	90/01  (billc)  First written.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      13-Oct-2008 (hanal04) Bug 120290
**          Initialise new cuupdabfsrc field in CURECORD to FALSE.
**/

GLOBALDEF CUTLIST   IICUattATtributeTlist[] =
{
    {CUID,	1,	ERx("appl_id"),		{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUID,	2,	ERx("class_id"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUID,	3,	ERx("catt_id"),		{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	1,	ERx("class_order"),	{NULL, sizeof(i2), DB_INT_TYPE}},
    {CUFILE,	2,	ERx("adf_type"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	3,	ERx("type_name"),	{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUFILE,	4,	ERx("adf_length"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	5,	ERx("adf_scale"),	{NULL, sizeof(i4), DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUatrATtributeRecord =
{
    sizeof(IICUattATtributeTlist)/sizeof(IICUattATtributeTlist[0]),
    IICUattATtributeTlist,
    ERx("ii_abfclasses"),
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

