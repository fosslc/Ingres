
/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1CNV2.H - "Large" page accessors (V2 page and newer)
**
** Description:
**      This header describes the visible standard accessors so that they
**	can also be used in other formats. These are the compression
**	accessors.
**
** History: 
**	06-may-1996 (thaju02)
**	    Created for New Page Format Project. Header file for > 2K pages.
**	    created from dm1cn.h.
**	    Add page_size argument to dm1cn_isnew.
**	12-may-1996 (thaju02)
**	    Neglected to remove history at time of creation.
**	    Cleanup after-the-fact.
**	21-may-1996 (ramra01)
**	    Added new param tup_info to load accessor
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**      05-oct-98 (stial01)
**          Updated dmpp_dput parameters, Added dmpp_put_hdr
**      05-dec-1999 (nanpr01)
**          Put the missing prototypes and pass the tuple header as parameter
**	    for performance.
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      30-oct-2000 (stial01)
**          Changed prototype for dmpp_allocate, dmpp_isnew page accessors
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      28-may-2003 (stial01)
**          Added line_start parameter to dmpp_allocate accessor
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**	23-Oct-2009 (kschendel) SIR 122739
**	    Remove unused record type param from put.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add lg_id parameter to
**	    dm1cnv2_delete, dm1cnv2_get, dm1cnv2_put prototypes.
**/

FUNC_EXTERN DB_STATUS   dm1cnv2_allocate(
			    DMPP_PAGE               *data,
			    DMP_RCB                 *r,
			    char		    *record,
			    i4                      record_size,
			    DM_TID                  *tid,
			    i4			    line_start);
FUNC_EXTERN bool	dm1cnv2_isempty(
			    DMPP_PAGE               *data,
			    DMP_RCB                 *r);
FUNC_EXTERN DB_STATUS	dm1cnv2_check_page(
			    DM1U_CB                 *dm1u,
			    DMPP_PAGE               *page,
			    i4                      page_size);
FUNC_EXTERN VOID	dm1cnv2_delete(
			    i4		    page_type,	
			    i4		    page_size,
			    DMPP_PAGE	    *page,
			    i4              update_mode,
			    i4         reclaim_space,
			    DB_TRAN_ID      *tran_id,
			    u_i2	    lg_id,
			    DM_TID          *tid,
			    i4              record_size);

FUNC_EXTERN VOID	dm1cnv2_format(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE       *page,
			    DM_PAGENO       pageno,
			    i4              stat,
			    i4		    fill_option);
FUNC_EXTERN DB_STATUS  dm1cnv2_get(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE	    *page,
			    DM_TID          *tid,
			    i4      *record_size,
			    char            **record,
			    i4		    *row_version,
			    u_i4	    *row_low_tran,
			    u_i2	    *row_lg_id,
			    DMPP_SEG_HDR    *seg_hdr);

FUNC_EXTERN i4          dm1cnv2_get_offset(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE       *page,
			    i4             item);
FUNC_EXTERN i4		dm1cnv2_getfree(
			    DMPP_PAGE       *page,
			    i4              page_size);

FUNC_EXTERN i4 	dm1cnv2_isnew(
			    DMP_RCB	    *r,
			    DMPP_PAGE       *page,
			    i4             line_num);

FUNC_EXTERN i4		dm1cnv2_ixsearch(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE       *page,
			    DB_ATTS         **keyatts,
			    i4              keys_given,
			    char            *key,
			    bool            partialkey,
			    i4              direction,
			    i4              relcmptlvl,
			    ADF_CB          *adf_cb);
FUNC_EXTERN DB_STATUS    dm1cnv2_load(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE       *data,
			    char            *record,
			    i4         record_size,
			    i4         record_type,
			    i4         fill,
			    DM_TID          *tid,
			    u_i2            current_table_version,
			    DMPP_SEG_HDR    *seg_hdr);
FUNC_EXTERN VOID dm1cnv2_put(
			    i4		    page_type,
			    i4		    page_size,
			    DMPP_PAGE	    *page,
			    i4      update_mode,
			    DB_TRAN_ID      *tran_id,
			    u_i2	    lg_id,
			    DM_TID          *tid,
			    i4              record_size,
			    char            *record,
			    u_i2            current_table_version,
			    DMPP_SEG_HDR    *seg_hdr);

FUNC_EXTERN DB_STATUS	dm1cnv2_reallocate(
			    i4		page_type,
			    i4		page_size,
			    DMPP_PAGE	*page,
			    i4		line_num,
			    i4		reclen);

FUNC_EXTERN VOID         dm1cnv2_reclen(
			    i4		page_type,
			    i4		page_size,
			    DMPP_PAGE	*page,
			    i4         lineno,
			    i4         *record_size);

FUNC_EXTERN i4		 dm1cnv2_tuplecount(
			    DMPP_PAGE       *page,
			    i4              page_size);
                        
FUNC_EXTERN i4	         dm1cnv2_dput(
			    DMP_RCB	*rcb,
			    DMPP_PAGE	*page, 
			    DM_TID	*tid,
			    i4		*err_code);

FUNC_EXTERN DB_STATUS	dm1cnv2_clean(
			    i4		page_type,
			    i4		page_size,
			    DMPP_PAGE	*page,
			    DB_TRAN_ID  *tranid,
			    i4		lk_type,
			    i4		*avail_space);
