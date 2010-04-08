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
** Name:	cucform.c	- CURECORD structure for forms objects
**
** Description:
**	This file contains the CURECORD structures for details
**	lines for OC_FORM, CUC_FIELD and CUC_TRIM.
**
** History:
**	4-jul-1987 (Joe)
**		Initial Version
**	30-nov-1987 (peter)
**		Add ii_encoded_forms to list of tables to delete.
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

GLOBALDEF CUTLIST   IICUfotFOrmTlist[] =
{
    {CUFILE,   1,	ERx("frmaxcol"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   2,	ERx("frmaxlin"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   3,	ERx("frposx"),		{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   4,	ERx("frposy"),		{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   5,	ERx("frfldno"),		{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   6,	ERx("frnsno"),		{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   7,	ERx("frtrimno"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   8,	ERx("frversion"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   9,	ERx("frscrtype"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   10,	ERx("frscrmaxx"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   11,	ERx("frscrmaxy"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   12,	ERx("frscrdpix"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   13,	ERx("frscrdpiy"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   14,	ERx("frflags"),		{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   15,	ERx("fr2flags"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,   16,	ERx("frtotflds"),	{NULL, 4, DB_INT_TYPE}},
    {CUID,	1,	ERx("object_id"),	{NULL, 4, DB_INT_TYPE}}
};

static CUDELLIST IICUfdtFormDelTab [] =
{
    {ERx("ii_forms"), ERx("object_id")},
    {ERx("ii_fields"), ERx("object_id")},
    {ERx("ii_trim"), ERx("object_id")}, 
    {ERx("ii_encoded_forms"), ERx("object_id")},
    {NULL, NULL}
};

STATUS	IICUcfoCopyFormOut();
STATUS    	IICUpfaPostFormAdd();

GLOBALDEF CURECORD IICUforFOrmRecord =
{
    sizeof(IICUfotFOrmTlist)/sizeof(IICUfotFOrmTlist[0]),
    IICUfotFOrmTlist,
    ERx("ii_forms"),
    FALSE,
    FALSE,
    IICUpfaPostFormAdd,		/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudelete */
    IICUfdtFormDelTab,  /* cudeltab */
    IICUcfoCopyFormOut,	/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};

GLOBALDEF CUTLIST   IICUfitFIeldTlist[] =
{
    {CUFILE,	1,	ERx("flseq"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	2,	ERx("fldname"),	{NULL, 32, DB_CHR_TYPE}},
    {CUFILE,	3,	ERx("fldatatype"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	4,	ERx("fllength"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	5,	ERx("flprec"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	6,	ERx("flwidth"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	7,	ERx("flmaxlin"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	8,	ERx("flmaxcol"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	9,	ERx("flposy"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	10,	ERx("flposx"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	11,	ERx("fldatawidth"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	12,	ERx("fldatalin"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	13,	ERx("fldatacol"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	14,	ERx("fltitle"),	{NULL, 50, DB_CHR_TYPE}},
    {CUFILE,	15,	ERx("fltitcol"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	16,	ERx("fltitlin"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	17,	ERx("flintrp"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	18,	ERx("fldflags"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	19,	ERx("fld2flags"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	20,	ERx("fldfont"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	21,	ERx("fldptsz"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	22,	ERx("fldefault"),	{NULL, 50, DB_CHR_TYPE}},
    {CUFILE,	23,	ERx("flformat"),	{NULL, 50, DB_CHR_TYPE}},
    {CUFILE,	24,	ERx("flvalmsg"),	{NULL, 100, DB_CHR_TYPE}},
    {CUFILE,	25,	ERx("flvalchk"),	{NULL, 255, DB_CHR_TYPE}},
    {CUFILE,	26,	ERx("fltype"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	27,	ERx("flsubseq"),	{NULL, 4, DB_INT_TYPE}},
    {CUID,	1,	ERx("object_id"),	{NULL, 4, DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUfirFIeldRecord =
{
    sizeof(IICUfitFIeldTlist)/sizeof(IICUfitFIeldTlist[0]),
    IICUfitFIeldTlist,
    ERx("ii_fields"),
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

GLOBALDEF CUTLIST   IICUtrtTRimTlist[] =
{
    {CUFILE,	1,	ERx("trim_col"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	2,	ERx("trim_lin"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	3,	ERx("trim_trim"),	{NULL, 150, DB_CHR_TYPE}},
    {CUFILE,	4,	ERx("trim_flags"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	5,	ERx("trim2_flags"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	6,	ERx("trim_font"),	{NULL, 4, DB_INT_TYPE}},
    {CUFILE,	7,	ERx("trim_ptsz"),	{NULL, 4, DB_INT_TYPE}},
    {CUID,	1,	ERx("object_id"),	{NULL, 4, DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUtrrTRimRecord =
{
    sizeof(IICUtrtTRimTlist)/sizeof(IICUtrtTRimTlist[0]),
    IICUtrtTRimTlist,
    ERx("ii_trim"),
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
