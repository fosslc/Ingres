/*
** Copyright (c) 1996, 2008 Ingres Corporation
*/

# include       <compat.h>
# include       <er.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <abclass.h>
# include       <adf.h>
# include       <ft.h>
# include       <fmt.h>
# include       <frame.h>
# include       <ooclass.h>
# include       <erug.h>
# include       "vqhotspo.h"

/*
** Name:        vqdata.c
**
** Description: Global data for vq facility.
**
** History:
**
**      28-sep-96 (mcgem01)
**          Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

/* ff.qsc */

GLOBALDEF APPL *IIVQappl; 
GLOBALDEF PTR IIVQcontext;
GLOBALDEF i4  IIVQrowmax = 0;
GLOBALDEF i4  IIVQcolmax = 0;
GLOBALDEF HOTSPOT IIVQhsaHotSpotArray[FF_MAXHOTSPOTS];
GLOBALDEF i4  IIVQhsiHotSpotIdx;


/* vqcurfld.qsc */

GLOBALDEF char IIVQcfnCurFldName[20];

/* vqffldlp.qsc */

/* Field listpick form and its fields.  They are '##'ed in vqffldlp.qsh  */
GLOBALDEF const char
        iivqLP0form[]           = ERx("iivqfldesclpfrm"),
        iivqLP1tblfld[]         = ERx("iivqfldesclpfrm"),
        iivqLP2actCol[]         = ERx("fldactnm"),
        iivqLP3flagsCol[]       = ERx("fldflags"),
        iivqLP4flags2Col[]      = ERx("fld2flags"),
        iivqLP5fldTypeCol[]     = ERx("fldtype");

/* vqmakfrm.qsc */

GLOBALDEF char *IIVQform = ERx("vq"); /* name of the visual query form */
GLOBALDEF char *IIVQcform = ERx("cvq"); /* name of the visual query form */
GLOBALDEF char *IIVQtfname_fmt = ERx("_%03d");
GLOBALDEF char IIVQcol_col[4] = ERx("col"); /* column names */
GLOBALDEF char IIVQdir_col[4] = ERx("dir");
GLOBALDEF char IIVQexp_col[4] = ERx("exp");
GLOBALDEF char IIVQinc_col[4] = ERx("inc");
GLOBALDEF char IIVQord_col[4] = ERx("ord");
GLOBALDEF char IIVQsrt_col[4] = ERx("srt");
GLOBALDEF i4   IIVQjtab = -2;
GLOBALDEF i4   IIVQjcol;
GLOBALDEF char *IIVQ_d_text;
GLOBALDEF char *IIVQ_a_text;
GLOBALDEF char *IIVQccCompCursor = ERx("_comp_cur");


