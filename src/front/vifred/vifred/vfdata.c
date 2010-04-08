/*
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       "vfunique.h"
# include       <ds.h>
# include       <er.h>
# include       <uf.h>
# include       "decls.h"
# include       <ooclass.h>
# include       <oocat.h>

/*
** Name:        vfdata.c
**
** Description: Global data for vifred utility.
**
** History:
**
**      26-sep-96 (mcgem01)
**          Created.
**	18-feb-97 (kosma01)
**	    Header file /usr/include/sys/m_types.h received an error
**	    for using Storage class register with external data.
**	    This happed because ft.h does a #defines reg register.
**	    I moved #include decls.h after both ds.h and uf.h because
**	    the both can #include m_types.h. ft.h is included by decls.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* cursor.qsc */

GLOBALDEF       i4      tnumfld = 0;
GLOBALDEF       i4      vfrealx = 0;
GLOBALDEF       FT_TERMINFO     IIVFterminfo = {0};

/* edit.c */
GLOBALDEF       i4      vfedtxpand = 0;

/* fasttodb.qsc */
GLOBALDEF       LOCATION        VFFLD = {0};
GLOBALDEF       LOCATION        VFTRD = {0};
GLOBALDEF       bool            vfuseiicomp = TRUE;
GLOBALDEF       char            *fname = NULL;
GLOBALDEF       OOID            vffrmid = 0;

/* getfrmnm.qsc */
GLOBALDEF       bool    IIVFneedclr = FALSE;

/* lines.c */
GLOBALDEF       Sec_List *vfrfsec = NULL;
GLOBALDEF       char    *vfvtrim = NULL;
GLOBALDEF       i4 vfvattrtrim = 0;
GLOBALDEF       POS     *IIVFvcross = NULL;
GLOBALDEF       POS     *IIVFhcross = NULL;
GLOBALDEF       bool    IIVFxh_disp = FALSE;
GLOBALDEF       bool    IIVFru_disp = FALSE;

/* trcreate.qsc */
GLOBALDEF POS   edtfps = {0};
GLOBALDEF POS   *ntfps = NULL;
GLOBALDEF FIELD *edtffld = NULL;

/* undo.c */
GLOBALDEF i4    vfoldundo = 0;
GLOBALDEF bool  vfinundo = FALSE;

/* vfcat.qsc */
GLOBALDEF OO_CATREC     vfoocatrec = {0};

/* vfdlock.c */
GLOBALDEF       bool    vfdlock = FALSE;

/* vfexit.c */
GLOBALDEF       bool    vftkljmp = FALSE;

/* vftofrm.c */
GLOBALDEF FIELD **fldlist = NULL;
GLOBALDEF TRIM  **trmlist = NULL;
GLOBALDEF i4    tnumfield = 0;

/* writefrm.qsc */
GLOBALDEF       bool    IIVFnnNewName = FALSE;

