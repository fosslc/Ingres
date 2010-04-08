/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1S.H - Heap Access Method interface definitions.
**
** Description:
**      This file describes the interface to the Heap Access Method
**	services.
**
** History:
**	 7-jul-1992 (rogerk)
**	    Created for DMF Prototyping.
**	16-nov-1992 (jnash)
**	    Reduced logging project.  Add dm1s_empty_table.
**	26-jul-1992 (jnash)
**	    Add build_fcb_flag to dm1s_empty_table() FUNC_EXTERN.
**	23-aug-1993 (rogerk)
**	    Changed arguments to dm1s_empty_table.
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to the dm1s_empty_table prototype.
**      30-oct-2000 (stial01)
**          Changed prototype for dm1s_allocate
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for dm1s_empty_table
**	03-Dec-2008 (jonj)
**	    SIR 120874: dm1s_? functions converted to DB_ERROR *
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE** with DMP_PINFO*
**	    in dm1s_allocate, dm1s_search prototypes.
**/

FUNC_EXTERN DB_STATUS	    dm1s_allocate(
				    DMP_RCB        *rcb,
				    DMP_PINFO      *pinfo,
				    DM_TID         *tid,
				    char           *record,
				    char	   *crecord,
				    i4             need,
				    i4             dupflag,
				    DB_ERROR       *dberr);

FUNC_EXTERN DB_STATUS	    dm1s_deallocate(
				    DMP_RCB        *rcb,
				    DB_ERROR       *dberr);

FUNC_EXTERN DB_STATUS	    dm1s_search(
				    DMP_RCB         *rcb,
				    DMP_PINFO       *pinfo,
				    char            *key,
				    i4         keys_given,
				    i4         mode,
				    i4         direction,
				    DM_TID          *tid,
				    DB_ERROR       *dberr);

FUNC_EXTERN DB_STATUS 	    dm1s_empty_table(
				    DMP_DCB         *dcb,
				    DB_TAB_ID       *tbl_id,
				    i4         allocation,
				    DMPP_ACC_PLV    *loc_plv,
				    DM_PAGENO       fhdr_pageno,
				    DM_PAGENO       first_fmap,
				    DM_PAGENO       last_fmap,
 				    DM_PAGENO       first_free,
				    DM_PAGENO       last_free,
				    DM_PAGENO       first_used,
				    DM_PAGENO       last_used,
				    i4		    page_type,
				    i4	    	    page_size,
				    i4         	    loc_count,
				    DMP_LOCATION    *loc_array,
				    DB_ERROR       *dberr);

