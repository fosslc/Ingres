/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1M.H - Types, Constants, Functions, and Macros for R-trees.
**
** Description:
**      This file contains all the R-tree specific typedefs, constants,
**      functions, and macros.
**
** History:
**      12-may-95 (wonst02)
**          Original (cloned from dm1b.h)
**	    === ingres!main!generic!back!dmf!hdr dm1b.h rev 9 ====
**	25-mar-96 (shero03)
**	    Added pagesize from Variable Page size project
**	20-apr-96 (wonst02)
**	    Adjusted MCT_LEAF_KPERPAGE macro to add TID size.
** 	29-apr-1996 (wonst02)
** 	    Added dm1mxmbr_compare().
** 	30-apr-1996 (wonst02)
** 	    REmoved MCT_LEAF_KLEN & MCT_LEAF_KPERPAGE macros.
** 	08-jun-1996 (wonst02)
** 	    Added dm1mxkkcmp() and removed some obsolete functions.
** 	23-jul-1996 (wonst02)
** 	    Added dm1mxhilbert() routine.
** 	24-jul-1996 (wonst02)
** 	    Added DM1M_KPERPAGE_MACRO to return the keys per page.
**	21-sep-1996 (wonst02)
**	    Added dm1mxpagembr() routine.
**	24-sep-1996 (wonst02)
**	    Added dm1m_get_range(), dm1mxdistribute().
**	23-Oct-1996 (wonst02)
**	    Added dm1m_count().
**	15-nov-1996 (wonst02)
**	    Use DM1B_OFFSEQ in the KPERPAGE calculation.
**	25-nov-1996 (wonst02)
**	    Modified parameters of dm1mxcompare() and dm1mxsearch().
**	03-dec-1996 (wonst02)
**	    Added ancestor stack parameters to dm1m_replace().
**      07-Dec-1998 (stial01)
**          Added prototype for dm1m_kperpage, removed DM1M_KPERPAGE_MACRO
**      09-feb-1999 (stial01)
**          Added relcmptlvl to dm1m_kperpage prototype
** 	25-mar-1999 (wonst02)
** 	    Fix RTREE bug B95350 in Rel. 2.5... change some data types.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for Variable Page Type SIR 102955)
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2a_?, dm1?_count functions converted to DB_ERROR *
**	03-Dec-2008 (jonj)
**	    SIR 120874: dm1m_? functions converted to DB_ERROR *
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	23-Oct-2009 (kschendel) SIR 122739
**	    Prototype changes for new rowaccessor scheme.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE** with DMP_PINFO*
**	    in external prototypes, add bridge functions dm1m_fix_page, 
**	    dm1m_unfix_page, dm1m_mutex, dm1m_unmutex, dm1m_getfree to
**	    avoid having to change a lot of convoluted code to conform
**	    to DMPP_PAGE.
**/


/*
**  R-tree external functions
*/

FUNC_EXTERN DB_STATUS dm1m_allocate(
			DMP_RCB		*rcb,
			DMP_PINFO   	*leafPinfo,
			DMP_PINFO   	*dataPinfo,
			DM_TID      	*bid,
			DM_TID      	*tid,
			char        	*record,
			i4     	need,
			char        	*key,
			i4     	dupflag,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1m_bybid_get(
			DMP_RCB         *rcb,
			DM_TID          *bid,
			DM_TID          *tid,
			char		*record,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1m_check_range(
			i4		page_type,
			i4		page_size,
    			DMPP_PAGE	*page,
    			ADF_CB	 	*adf_cb,
    			DMPP_ACC_KLV	*klv,
			char		*key,
			bool		*result,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1m_count(
    			DMP_RCB         *rcb,
    			i4	    	*countptr,
    			i4         direction,
    			void            *record,
    			DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS dm1m_delete(
			DMP_RCB        *rcb,
			DM_TID         *bid,
			DM_TID         *tid,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1m_get(
			DMP_RCB         *rcb,
			DM_TID          *tid,
			char		*record,
			i4		opflag,
			DB_ERROR     	*dberr);

FUNC_EXTERN void dm1m_get_range(
			i4		page_type,
			i4		page_size,
    			DMPP_PAGE	*page,
    			DMPP_ACC_KLV	*klv,
			char		*lowkey,
			char		*highkey);

FUNC_EXTERN DB_STATUS dm1m_put(
			DMP_RCB	    *rcb,
			DMP_PINFO   *leafPinfo,
			DMP_PINFO   *dataPinfo,
			DM_TID     *tid,
			DM_TID     *bid,
			char        *record,
			char        *key,
			i4     record_size,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1m_replace(
			DMP_RCB         *rcb,
			DM_TID          *bid,
			DM_TID          *tid,
			char            *record,
			i4         record_size,
    			DM_TID		ancestors[],
    			i4		ancestor_level,
			DMP_PINFO	*newleafPinfo,
			DMP_PINFO	*newdataPinfo,
			DM_TID          *newbid,
			DM_TID          *newtid,
			char            *nrec,
			i4         nrec_size,
			char            *newkey,
			i4		inplace_update,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1m_rcb_update(
			DMP_RCB        *rcb,
			DM_TID         *bid,
			DM_TID         *newbid,
			i4         split_pos,
			i4         opflag,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1m_search(
			DMP_RCB         *rcb,
			char		*key,
			i4         keys_given,
			i4         mode,
			i4         direction,
			DMP_PINFO       *leafPinfo,
			DM_TID          *bid,
			DM_TID          *tid,
			DB_ERROR     	*dberr);

FUNC_EXTERN VOID dm1m_badtid(
			DMP_RCB		*rcb,
			DM_TID		*bid,
			DM_TID		*tid,
			char		*key);

FUNC_EXTERN DB_STATUS dm1mxadjust(
    			DMP_RCB	    	*rcb,
    			DMP_TCB	    	*tcb,
    			DMPP_PAGE	*buffer,
    			i4	    	page_size,
    			i4	    	klen,
			char		*newkey,
    			i4		child,
    			i4	    	log_updates,
    			i4	    	mutex_required,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1mxcompare(
    			ADF_CB 	 	*adf_cb,
    			DMPP_ACC_KLV 	*klv,
    			i4      	direction,
    			bool	 	isleaf,
    			char         	*key,
    			i4	 	keylen,
    			char	 	*key_ptr,
    			bool	 	*compare);

FUNC_EXTERN DB_STATUS dm1mxdelete(
			DMP_RCB		*rcb,
			DMPP_PAGE	*buffer,
    			i4		child,
			i4		log_updates,
			i4		mutex_required,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1mxdistribute(
			DMP_RCB		*r,
			DMPP_PAGE	*sibling[],
			i4		numsiblings,
			i4		sibkids,
			LG_LSN	    	*lsn,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1mxformat(
			i4		page_type,
			i4		page_size,
			DMP_RCB		*rcb,
			DMP_TCB         *tcb,
			DMPP_PAGE      *buffer,
			i4         	kperpage,
			i4         	klen,
			DM_PAGENO       page,
			i4         	indexpagetype,
			i4		compression_type,
    			DMPP_ACC_PLV	*loc_plv,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1mxhilbert(
    			ADF_CB		*adf_cb,
    			DMPP_ACC_KLV    *klv,
    			char		*key_ptr,
    			char		*hilbert);

FUNC_EXTERN DB_STATUS dm1mxinsert(
			DMP_RCB		*rcb,
			DMP_TCB		*tcb,
			i4		page_type,
			i4		page_size,
			DMP_ROWACCESS	*rac,
			i4		klen,
			DMPP_PAGE	*buffer,
			char		*key,
			DM_TID		*tid,
    			i4		child,
			i4		log_updates,
			i4		mutex_required,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1mxjoin(
			DMP_RCB         *rcb,
			DMPP_PAGE	*parent,
			i4		pos,
			DMPP_PAGE      *current,
			DMPP_PAGE      *sibling,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1mxkkcmp(
			i4		page_type,
			i4		page_size,
    			DMPP_PAGE  	*page,
    			ADF_CB		*adf_cb,
    			DMPP_ACC_KLV 	*klv,
    			char		*key_ptr,
    			char		*key,
    			i4	    	*compare);

FUNC_EXTERN DB_STATUS dm1mxnewroot(
			DMP_RCB	    	*rcb,
			DMPP_PAGE   	**root,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1mxpagembr(
			i4		page_type,
			i4		page_size,
    			DMPP_PAGE	*page,
			char		*rangekey,
			ADF_CB		*adf_cb,
			DMPP_ACC_KLV    *klv,
			u_i2		hilbertsize,
			bool		*changed);

FUNC_EXTERN DB_STATUS dm1mxsearch(
    			DMP_RCB		*r,
    			DMPP_PAGE	*buffer,
    			i4		mode,
    			i4		direction,
    			char		*key,
    			ADF_CB	 	*adf_cb,
    			DMPP_ACC_KLV	*klv,
			i4		pgtype,
    			i4		pgsize,
    			DM_TID		*tid,
    			i4		*pos,
			DB_ERROR     	*dberr);

FUNC_EXTERN DB_STATUS dm1mxsplit(
    			DMP_RCB     	*rcb,
    			DMPP_PAGE   	**current,
    			DMPP_PAGE	**sibling,
    			char		*key,
    			DB_ATTS     	*keyatts[],
    			i4		*split,
			char		*splitkey,
			DB_ERROR     	*dberr);

FUNC_EXTERN i4    dm1m_kperpage(
			i4	page_type,
			i4	page_size,
			i4      relcmptlvl,
			i4	atts_cnt,
			i4	key_len,
			i4	indexpagetype);

FUNC_EXTERN DB_STATUS	dm1m_fix_page(
			DMP_RCB		*rcb,
			DM_PAGENO	page_number,
			i4		action,
			DMPP_PAGE	**page,
			DB_ERROR	*dberr);

FUNC_EXTERN DB_STATUS	dm1m_unfix_page(
			DMP_RCB		*rcb,
			i4		action,
			DMPP_PAGE	**page,
			DB_ERROR	*dberr);

FUNC_EXTERN VOID	dm1m_mutex(
			DMP_TABLE_IO	*tbio,
			i4		flags,
			i4		lock_list,
			DMPP_PAGE	**page);

FUNC_EXTERN VOID	dm1m_unmutex(
			DMP_TABLE_IO	*tbio,
			i4		flags,
			i4		lock_list,
			DMPP_PAGE	**page);

FUNC_EXTERN DB_STATUS	dm1m_getfree(
			DMP_RCB		*rcb,
			i4		flags,
			DMPP_PAGE	**page,
			DB_ERROR	*dberr);
