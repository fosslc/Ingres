/*
** Copyright (c) 2001, 2003 Ingres Corporation
*/

/**
** Name: SAXIngres.hpp - defines and prototypes to support impxml utility.
**
** Description:
**	This header file includes the ingres headers and function protypes
**	for the use of impxml utility. 
**
** History:
**      07-jun-2001 (gupsh01)
**          written
**	12-jun-2001 (somsa01)
**	    Removed 'extern "C"', as we've modified GLOBALREF for C++ to
**	    include this.
**	14-jun-2001 (gupsh01)
**	    Added reference to xfmodify. Added MAXFIELD definition.
**	09-oct-2003 (schka70/abbjo03)
**	    Remove unnecessary include of si.h, which causes FILE typedef
**	    conflict on VMS.
**	31-Oct-2006 (kiria01) b116944
**	   Increase options passable to xfmodify().
**	28-Aug-2009 (kschendel) b121804
**	    Remove (incorrect) afe_cancoerce definition, and others
**	    now properly defined in afe.h.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Add extern "C" to include CV/ST globals from C files
*/

extern "C" 
{
# include       <compat.h>
# include       <pc.h>
# include       <cv.h>
# include       <ex.h>
# include       <me.h>
# include       <st.h>
# include       <lo.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <er.h>
# include       <fe.h>
# include       <fedml.h>
# include       <uigdata.h>
# include       <ug.h>
# include       <ui.h>
# include       <adf.h>
# include       <afe.h>
# include       <xf.h>
}

/* Prototypes for the functions */

void 	set_meta_ingres_attributes (char **, char **, i2, char *);
void 	set_meta_table_attributes (XF_TABINFO *, char **, char **, i2);
void 	set_tabcol_attributes (XF_COLINFO  *, char **, char **, i2);
void 	set_meta_index_attributes (XF_TABINFO *, char **, char **, i2);
void 	set_indcol_attributes (XF_COLINFO  *, char **, char **, i2);
void 	col_free (XF_COLINFO *);
void 	tab_free (XF_TABINFO *);
void 	get_icols ();
void 	put_tab_col (XF_TABINFO *, XF_COLINFO  *);
void 	put_ind_col (XF_TABINFO *, XF_COLINFO  *);
bool 	get_cols (char *, char *, char *, char *, i4 , i4 ,  XF_COLINFO  *);
void    addOtherLocs(char buf[XF_INTEGLINE + 1], char add_loc[FE_MAXNAME + 1]);
void    norm(char *value);


XF_COLINFO 	*get_empty_column();
XF_TABINFO 	*get_empty_table();
XF_COLINFO 	*mkcollist (XF_COLINFO  *cp);


/* defines and Externs */
#define MAX_DB_NAME     FE_MAXNAME + 1
#define MAXSTRING       XF_INTEGLINE + 1
#define MAXFIELD	240 

FUNC_EXTERN     void FEafeerr(ADF_CB  *cb);
FUNC_EXTERN	void xfmodify (XF_TABINFO *t, i4 output_flags, i4 output_flags2);
