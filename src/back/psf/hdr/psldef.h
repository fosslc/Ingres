/*
**Copyright (c) 2004 Ingres Corporation
*/
/**
** Name: PSLDEF.H - Function prototypes for default-manipulation functions.
**
** Description:
**      This file contains the prototypes for functions dealing with
**      defaults.  These prototypes were put here instead of in PSHPARSE.H
**      because they use DMU_ATTR_ENTRY from DMUCB.H, which is not included by
**      all the PSF files that include PSHPARSE.H.
**
** History: 
**      25-feb-93 (rblumer)
**          created for psl_col_default prototype.
**      20-apr-93 (rblumer)
**          added prototype for psl_2col_ingres_default.
**	    changed psl_col_default to psl_1col_default (per coding standard).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
*/

FUNC_EXTERN DB_STATUS
psl_1col_default(
		 PSS_SESBLK	*sess_cb,
		 i4	col_type,
		 DMF_ATTR_ENTRY	*attr,
		 DB_TEXT_STRING	*def_txt,
		 PST_QNODE	*def_node,
		 DB_ERROR	*err_blk);

FUNC_EXTERN DB_STATUS
psl_2col_ingres_default(
			PSS_SESBLK	*sess_cb,
			DMF_ATTR_ENTRY	*attr,
			DB_ERROR	*err_blk);

