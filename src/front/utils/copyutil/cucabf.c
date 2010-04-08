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
** Name:	cucabf.c	- CURECORD structure for abf
**
** Description:
**	This file contains the CURECORD structures for details
**	lines for OC_APPL, OC_XFRAME and other abf objects.
**
** History:
**	4-jul-1987 (Joe)
**		Initial Version
**	09-nov-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**	89/09  (jhw)  Removed unnecessary cast for AIX compiler.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      13-Oct-2008 (hanal04) Bug 120290
**          Initialise new cuupdabfsrc field in CURECORD to FALSE.
**/

/*
** This is for OC_APPL.  Note that ii_abfobjects has applid and id
** which is the parent id and the records id.  For OC_APPL, these
** are the same.  That is why OC_APPL has a different CURECORD than
** other ABF objects.
*/
static CUTLIST   App_tlist[] =
{
    {CUFILE,	1,	ERx("abf_source"),	{NULL, 180, DB_CHR_TYPE}},
    {CUFILE,	2,	ERx("abf_symbol"),	{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUFILE,	3,	ERx("retadf_type"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	4,	ERx("rettype"),		{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUFILE,	5,	ERx("retlength"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	6,	ERx("retscale"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	7,	ERx("abf_version"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	8,	ERx("abf_arg1"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	9,	ERx("abf_arg2"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	10,	ERx("abf_arg3"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	11,	ERx("abf_arg4"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	12,	ERx("abf_arg5"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	13,	ERx("abf_arg6"),	{NULL, 48, DB_CHR_TYPE}},
    {CUOFILE,	14,	ERx("abf_flags"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUID,	1,	ERx("applid"),		{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUID,	1,	ERx("object_id"),	{NULL, sizeof(i4), DB_INT_TYPE}}
};

STATUS	IICUdaDeleteAppl();

GLOBALDEF CURECORD IICUaprAPplRecord =
{
    sizeof(App_tlist)/sizeof(App_tlist[0]),
    App_tlist,
    ERx("ii_abfobjects"),
    FALSE,
    FALSE,
    NULL,		/* cupostadd */
    NULL,		/* cuinsert */
    IICUdaDeleteAppl,	/* cudelete */
    NULL,		/* cudeltab */
    NULL,		/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};

/*
** For CUC_ADEPEND
** The ID is that of the frames that is at offset 2 since a frame is
** always a child of a parent.
*/
static CUTLIST   Depa_tlist[] =
{
    {CUFILE,	1,	ERx("abfdef_name"),	{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUFILE,	2,	ERx("abfdef_owner"),	{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUFILE,	3,	ERx("object_class"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	4,	ERx("abfdef_deptype"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUOFILE,	5,	ERx("abf_linkname"),	{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUOFILE,	6,	ERx("abf_deporder"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUID,	1,	ERx("object_id"),	{NULL, sizeof(i4), DB_INT_TYPE}}
,
    {CUID,	1,	ERx("abfdef_applid"),	{NULL, sizeof(i4), DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUapdAPplDepend =
{
    sizeof(Depa_tlist)/sizeof(Depa_tlist[0]),
    Depa_tlist,
    ERx("ii_abfdependencies"),
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

/*
** The remainder of ABF objects have the applid and the id different.
** The applid is the parent, so it is at offset 1.
*/
static CUTLIST   Obj_tlist[] =
{
    {CUFILE,	1,	ERx("abf_source"),	{NULL, 180, DB_CHR_TYPE}},
    {CUFILE,	2,	ERx("abf_symbol"),	{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUFILE,	3,	ERx("retadf_type"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	4,	ERx("rettype"),		{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUFILE,	5,	ERx("retlength"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	6,	ERx("retscale"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	7,	ERx("abf_version"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	8,	ERx("abf_arg1"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	9,	ERx("abf_arg2"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	10,	ERx("abf_arg3"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	11,	ERx("abf_arg4"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	12,	ERx("abf_arg5"),	{NULL, 48, DB_CHR_TYPE}},
    {CUFILE,	13,	ERx("abf_arg6"),	{NULL, 48, DB_CHR_TYPE}},
    {CUOFILE,	14,	ERx("abf_flags"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUID,	1,	ERx("applid"),		{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUID,	2,	ERx("object_id"),	{NULL, sizeof(i4), DB_INT_TYPE}}
};

STATUS	IICUpaaPostAbfobjectAdd();

GLOBALDEF CURECORD IICUaorAbfObjectRecord =
{
    sizeof(Obj_tlist)/sizeof(Obj_tlist[0]),
    Obj_tlist,
    ERx("ii_abfobjects"),
    FALSE,
    FALSE,
    IICUpaaPostAbfobjectAdd,	/* cupostadd */
    NULL,		/* cuinsert */
    NULL,		/* cudelete */
    NULL,		/* cudeltab */
    NULL,		/* cuoutfunc */
    NULL,		/* cuargv */
    NULL		/* cuinsstr */
};

/*
** For CUC_AODEPEND
** The id is that of the frames which is at offset 2 since a frame is
** always a child of a parent.
*/
static CUTLIST   Dep_tlist[] =
{
    {CUFILE,	1,	ERx("abfdef_name"),	{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUFILE,	2,	ERx("abfdef_owner"),	{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUFILE,	3,	ERx("object_class"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUFILE,	4,	ERx("abfdef_deptype"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUOFILE,	5,	ERx("abf_linkname"),	{NULL, FE_MAXNAME, DB_CHR_TYPE}},
    {CUOFILE,	6,	ERx("abf_deporder"),	{NULL, sizeof(i4), DB_INT_TYPE}},
    {CUID,	2,	ERx("object_id"),	{NULL, sizeof(i4), DB_INT_TYPE}}
,
    {CUID,	1,	ERx("abfdef_applid"),	{NULL, sizeof(i4), DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUadrAbfDependRecord =
{
    sizeof(Dep_tlist)/sizeof(Dep_tlist[0]),
    Dep_tlist,
    ERx("ii_abfdependencies"),
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

/*
** NOTE:
**
** We use DB_VCH_TYPE to indicate that the string is backslash encoded
** within the copy utilities to transmit possible control characters.
*/
static CUTLIST	Esc_tlist[] =
{
	{CUFILE, 1, ERx("encode_sequence"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 2, ERx("encode_estring"), {NULL, 1990, DB_VCH_TYPE}},
	{CUID, 2, ERx("encode_object"), {NULL, sizeof(i4), DB_INT_TYPE}},
	{CUID, 2, ERx("object_id"), {NULL, sizeof(i4), DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUecrEscapeCodeRecord =
{
    sizeof(Esc_tlist)/sizeof(Esc_tlist[0]),
    Esc_tlist,
    ERx("ii_encodings"),
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

static CUTLIST	Fv_tlist[] =
{
	{CUFILE, 1, ERx("fr_seq"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 2, ERx("var_field"), {NULL, FE_MAXNAME, DB_CHR_TYPE}},
	{CUFILE, 3, ERx("var_column"), {NULL, FE_MAXNAME, DB_CHR_TYPE}},
	{CUFILE, 4, ERx("var_datatype"), {NULL, 105, DB_CHR_TYPE}},
	{CUFILE, 5, ERx("var_comment"), {NULL, 60, DB_CHR_TYPE}},
	{CUID, 2, ERx("object_id"), {NULL, sizeof(i4), DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUfvFrameVarRecord =
{
    sizeof(Fv_tlist)/sizeof(Fv_tlist[0]),
    Fv_tlist,
    ERx("ii_framevars"),
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

static CUTLIST	Ma_tlist[] =
{
	{CUFILE, 1, ERx("mu_text"), {NULL, FE_MAXNAME, DB_CHR_TYPE}},
	{CUFILE, 2, ERx("mu_field"), {NULL, FE_MAXNAME, DB_CHR_TYPE}},
	{CUFILE, 3, ERx("mu_column"), {NULL, FE_MAXNAME, DB_CHR_TYPE}},
	{CUFILE, 4, ERx("mu_expr"), {NULL, 240, DB_CHR_TYPE}},
	{CUOFILE, 5, ERx("mu_seq"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUID, 2, ERx("object_id"), {NULL, sizeof(i4), DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUmaMenuArgRecord =
{
    sizeof(Ma_tlist)/sizeof(Ma_tlist[0]),
    Ma_tlist,
    ERx("ii_menuargs"),
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

static CUTLIST	Vqc_tlist[] =
{
	{CUFILE, 1, ERx("vq_seq"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 2, ERx("tvq_seq"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 3, ERx("col_name"), {NULL, FE_MAXNAME, DB_CHR_TYPE}},
	{CUFILE, 4, ERx("ref_name"), {NULL, FE_MAXNAME, DB_CHR_TYPE}},
	{CUFILE, 5, ERx("adf_type"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 6, ERx("adf_length"), {NULL, sizeof(i4), DB_INT_TYPE}},
	{CUFILE, 7, ERx("adf_scale"), {NULL, sizeof(i4), DB_INT_TYPE}},
	{CUFILE, 8, ERx("col_flags"), {NULL, sizeof(i4), DB_INT_TYPE}},
	{CUFILE, 9, ERx("col_sortorder"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 10, ERx("col_info"), {NULL, 240, DB_CHR_TYPE}},
	{CUID, 2, ERx("object_id"), {NULL, sizeof(i4), DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUvqcVQColRecord =
{
    sizeof(Vqc_tlist)/sizeof(Vqc_tlist[0]),
    Vqc_tlist,
    ERx("ii_vqtabcols"),
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

static CUTLIST	Vqt_tlist[] =
{
	{CUFILE, 1, ERx("vq_seq"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 2, ERx("vq_mode"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 3, ERx("tab_name"), {NULL, FE_MAXNAME, DB_CHR_TYPE}},
	{CUFILE, 4, ERx("tab_owner"), {NULL, FE_MAXNAME, DB_CHR_TYPE}},
	{CUFILE, 5, ERx("tab_section"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 6, ERx("tab_usage"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 7, ERx("tab_flags"), {NULL, sizeof(i4), DB_INT_TYPE}},
	{CUID, 2, ERx("object_id"), {NULL, sizeof(i4), DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUvqtVQTabRecord =
{
    sizeof(Vqt_tlist)/sizeof(Vqt_tlist[0]),
    Vqt_tlist,
    ERx("ii_vqtables"),
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

static CUTLIST	Vqj_tlist[] =
{
	{CUFILE, 1, ERx("vq_seq"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 2, ERx("join_type"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 3, ERx("join_tab1"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 4, ERx("join_tab2"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 5, ERx("join_col1"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUFILE, 6, ERx("join_col2"), {NULL, sizeof(i2), DB_INT_TYPE}},
	{CUID, 2, ERx("object_id"), {NULL, sizeof(i4), DB_INT_TYPE}}
};

GLOBALDEF CURECORD IICUvqjVQJoinRecord =
{
    sizeof(Vqj_tlist)/sizeof(Vqj_tlist[0]),
    Vqj_tlist,
    ERx("ii_vqjoins"),
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
