/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1R.H - Definition for the record level services.
**
** Description:
**      This file contains the types and constants used by the record 
**	processing
**	services of DMF.
**
** History:
**	 7-July-1992 (rmuth)
**	    Created during prototyping DMF exercise.
**	26-oct-1992 (rogerk)
**	    Reduced Logging: Changed prototype definition for dm1r_replace.
**	14-dec-1992 (rogerk)
**	    Reduced Logging: Changed prototype definition for dm1r_replace.
**	26-apr-1993 (bryanp)
**	    Add prototype for dm1r_lock_row().
**      10-aug-1995 (cohmi01)
**          Add prototype for dm1r_build_dplist() used by prefetch.
**      21-aug-1995 (cohmi01)
**          Prototype for dm1r_count().
**      22-jul-1996 (ramra01 for bryanp)
**          Add prototype for dm1r_cvt_row().
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add prototype for: dm1r_allocate(), dm1r_lock_sc_row(),
**          dm1r_unlock_row(), dm1r_lock_value(), dm1r_unlock_value()
**          Changed prototype (and usage of) dm1r_lock_row()
**	    Add DM1R_LK_NOWAIT, DM1R_LK_TUPLE, DM1R_LK_PHYSICAL.
**      22-jan-97 (dilma04)
**          Add DM1R_LK_NOLOCK.
**      10-mar-97 (stial01)
**          Added record to prototype for dm1r_allocate
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Added DM1R_LK_CHECK and DM1R_LK_CONVERT flags.
**      29-jul-97 (stial01)
**          Changed prototype for dm1r_delete, dm1r_put.
**      07-jul-98 (stial01)
**          Removed DM1R_LK_CHECK (call dm1r_check_lock instead)
**          Added ROW_ACC* defines.
**      22-sep-98 (stial01)
**          Added prototype for dm1r_lk_convert
**      16-nov-98 (stial01)
**          Added DM1R_LK_PKEY_PROJ
**      18-feb-99 (stial01)
**          Changed prototype for dm1r_allocate
**	19-feb-99 (nanpr01)
**	    Added dm1r_lock_downgrade for update mode lock.
**	01-apr-99 (nanpr01)
**	    Added DM1R_CLEAR_LOCKED_TID for clearing the locked_tid in RCB and
**	    DM1R_ALLOC_TID for not tampering with locked_tid.
**      05-may-1999 (nanpr01)
**          Removed unused DM1R_LK_TUPLE
**      31-jan-2000 (stial01)
**          Removed unecessary defines
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Removed unecessary defines (Variable Page Type SIR 102955)
**      10-jul-2003 (stial01)
**          changed prototype for dm1r_allocate
**	22-Jan-2004 (jenjo02)
**	    dm1r_lock_row() callers must supply (LK_LOCK_KEY*)
**	    into which the row lock key is returned.
**	    dm1r_unlock_row() is now passed (LK_LOCK_KEY*) instead
**	    of (DM_TID*), all for row locking in a Global Index.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**	16-mar-2004 (gupsh01)
**	    Added rcb_adf_cb to the parameter list of dm1r_cvt_row to
**	    support alter table alter column.
**      08-jul-2004 (stial01)
**          changed prototype for dm1r_get_segs
**	11-Apr-2008 (kschendel)
**	    Update prototype for dm1r_cvt_row
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2a_?, dm1?_count functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1r_? functions converted to DB_ERROR *
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add function prototype for dm1r_crow_lock().
**	    Replace DMPP_PAGE** with DMP_PINFO in dm1r_allocate, 
**	    dm1r_lk_convert, dm1r_crow_lock prototypes.
**      12-Jul-2010 (stial01) (SIR 121619 MVCC, B124077)
**          Added row_is_consistent() prototype.
**      15-Sep-2010 (stial01) (SIR 121619 MVCC, SD 146756, B124453)
**          Moved row_is_consistent() prototype to dmp.h
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Prototype dbg_dm723()
**/

/*
** Lock flags
*/
#define DM1R_LK_NOWAIT     0x01
#define DM1R_LK_PHYSICAL   0x02
#define DM1R_LK_NOLOCK     0x08
#define DM1R_LK_PKEY_PROJ  0x10
#define DM1R_LK_CONVERT    0x20
#define DM1R_CLEAR_LOCKED_TID 0x80
#define DM1R_ALLOC_TID 	   0x100
#define DM1R_PHANPRO_VALUE 0x200

/*
** Row Locking access check
*/
#define ROW_ACC_NONKEY     ((i4)2)
#define ROW_ACC_NEWDATA    ((i4)3)
#define ROW_ACC_RETURN     ((i4)4)


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS	dm1r_allocate(
				DMP_RCB         *rcb,
				DMP_PINFO	*pinfo,
				char            *record,
				i4		record_size,
				DM_TID          *tid,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS	dm1r_delete(
				DMP_RCB		*rcb,
				DM_TID		*tid,
				i4		flag,
				i4         opflag,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS	dm1r_get(
				DMP_RCB         *rcb,
				DM_TID          *tid,
				char            *record,
				i4         opflag,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS	dm1r_put(
				DMP_RCB     	*rcb,
				DMP_PINFO   	*dataPinfo,
				DM_TID     	*tid,
				char        	*record,
				char        	*key,
				i4     	record_size,
				i4         opflag,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS	dm1r_replace(
				DMP_RCB		*rcb, 
				DM_TID		*old_tid, 
				char		*old_row,
				i4		old_record_size,
				DMP_PINFO	*newdataPinfo,
				DM_TID		*new_tid, 
				char		*new_row,
				i4		new_record_size,
				i4		opflag,
				i4		delta_start,
				i4		delta_end,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS	dm1r_rcb_update(
   				DMP_RCB        	*rcb,
				DM_TID         	*tid,
				DM_TID         	*newtid,
				i4        	opflag,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS	dm1r_lock_sc_row(
				DMP_RCB		*rcb,
				DM_TID		*tid,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS	dm1r_crow_lock(
				DMP_RCB		*rcb,
				i4		flags,
				DM_TID		*tid,
				DMP_PINFO	*pinfo,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS	dm1r_lock_row(
				DMP_RCB		*rcb,
				i4         flags,
				DM_TID		*tid,
				i4         *new_lock,
				LK_LOCK_KEY	*lock_key,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS   dm1r_unlock_row(
                                DMP_RCB         *rcb,
                                LK_LOCK_KEY     *lock_key,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS   dm1r_lock_value( 
                                DMP_RCB         *rcb,
				i4         flags,
				char            *tuple,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS   dm1r_unlock_value(
                                DMP_RCB         *rcb,
                                char            *tuple,
				DB_ERROR        *dberr);

FUNC_EXTERN i4  dm1r_build_dplist(
	                        DMP_PFREQ       *pfreq,
      		                DM_PAGENO       datapages[]);

FUNC_EXTERN DB_STATUS dm1r_count(
                        DMP_RCB         *rcb,
                        i4         *countptr,
			i4         direction,
			void            *record,
                        DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS	dm1r_cvt_row(
			    	DB_ATTS    	**atts_array,
			    	i4     	atts_count,
			    	char		*src,
			    	char		*dst,
			    	i4		dst_maxlen,
			    	i4     	*dst_size,
			    	i4		row_version,
				ADF_CB		*rcb_adf_cb);

FUNC_EXTERN DB_STATUS dm1r_lk_convert(
				DMP_RCB     *r,
				DM_PAGENO    pageno,
				DMP_PINFO    *pinfo,
				LK_LKID      *lkid,
				DB_ERROR        *dberr);
FUNC_EXTERN DB_STATUS dm1r_lock_downgrade(
				DMP_RCB     *r,
				DB_ERROR        *dberr);
FUNC_EXTERN DB_STATUS dm1r_lock_downgrade(
				DMP_RCB     *r,
				DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS dm1r_get_segs(
		DMP_RCB		*rcb,
		DMPP_PAGE	*data,
		DMPP_SEG_HDR	*first_seg_hdr,
		char		*first_seg,
		char		*record,
		DB_ERROR        *dberr);

FUNC_EXTERN DB_STATUS dm1r_check_lock(
				DMP_RCB		*rcb,
				DM_TID		*tid);

FUNC_EXTERN DB_STATUS dbg_dm723(
				DMP_RCB		*rcb,
				DM_TID		*tid,
				DM_TID		*base_tid,
				i4		dm0l_flags,
				i4		*err_code);
