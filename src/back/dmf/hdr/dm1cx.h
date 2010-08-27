/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM1CX.H - Index Page interface definitions.
**
** Description:
**      This file describes the interface to the Btree Index Page services.
**
** History:
**	 7-jul-1992 (rogerk)
**	    Created during DMF Prototyping.
**	    Moved definitions from DM1C.H.
**	23-jul-92 (kwatts)
**	    dm1cxget, dm1cxformat and dm1cxcomp_rec all get a new MPF
**	    parameter.
**      22-oct-92 (rickh)
**	    Replaced all references to DMP_ATTS with DB_ATTS.
**	14-dec-92 (rogerk)
**	    Reduced Logging Project:  Added dm1cxrecptr prototype.
**	    Removed TCB parameter from dm1cxformat routine.
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to dm1cxhas_room, dm1cxmidpoint, 
**		dm1cxformat, dm1cxallocate, dm1cxdel, dm1cxlength,
**		dm1cxput, dm1cxrclaim, dm1cxlshift, dm1cxrshift, dm1cxreplace,
**		dm1cxjoinable.
**	06-may-1996 (nanpr01)
**	    Added page_size parameter in dm1cxlog_error, dm1cxget, dm1cxtget 
**	    and dm1cxtput.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added reclaim_space param to dm1cxdel()
**          Added dm1cxclean()
**      27-feb-97 (stial01)
**          Changed prototype for dm1cxclean(), dm1cxdel()
**          Added prototype for dm1cxkeycount()
**      12-jun-97 (stial01)
**          dm1cxformat() Added attr,key args to prototype
**      15-jan-98 (stial01)
**          dm1cxclean() removed unused err_code.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      07-Dec-98 (stial01)
**          Added dm1cxkperpage, Added spec to dm1cxformat prototype
**      09-feb-1999 (stial01)
**          Added relcmptlvl to dm1cxkperpage prototype
**      15-nov-1999 (stial01)
**          Added prototype for dm1cxget_deleted
**      21-nov-1999 (nanpr01)
**          Code optimization/reorganization based on quantify profiler output.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**	22-Jan-2004 (jenjo02)
**	    For Partitioned tables, Global indexes, added
**	    tidsize to dm1cxformat() dm1cxkperpage(),
**	    *partno to dm1cxget() dm1cxtget(),
**	    partno  to dm1cxput() dm1cxtput() dm1cxreplace()
**	25-Apr-2006 (jenjo02)
**	    Add Clustered to dm1cxformat(), dm1cxkperpage() for Clustered
**	    Leaf pages.
**	17-Nov-2008 (jonj)
**	    SIR 120874 Macroize dm1cxlog_error to supply source information
**	    of caller.
**	25-Aug-2009 (kschendel) 121804
**	    Add missing dm1cx_isnew prototype.
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	23-Oct-2009 (kschendel) SIR 122739
**	    Integrate changes for new row-accessor scheme.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add u_i2 lg_id to dm1cxdel, dm1cxput, dm1cxreplace,
**	    dm1cxget prototypes.
**	23-Feb-2010 (stial01)
**          Added rcb to dm1cxclean prototype
**	09-Jun-2010 (stial01)
**          Added prototype for dm1cx_txn_get
*/

/*
**  Forward and/or External function references.
*/

/*
**  Defines of other constants.
*/

/*
** Compression modes for Btree Index Compression:
** Note!  These definitions are relatively misleading.  The "compression type"
** passed around in dm1cidx is really more of a bool, is_compressed,
** because in general all the algorithms care about is whether any kind
** of compression is in use.
** Any place that needs to know the *specific* compression type has to
** be passed a DMP_ROWACCESS structure, which embeds all of the compression
** type specific information.
**
** Ultimately this should be cleaned up the rest of the way, such that
** a true bool is passed to anyone that only needs to care about "is it
** compressed in any manner".
*/
#define			DM1CX_UNCOMPRESSED  0L
#define			DM1CX_COMPRESSED    1L

/*
** Function Prototypes
*/

FUNC_EXTERN DB_STATUS	dm1cxallocate(i4 page_type, i4 page_size, DMPP_PAGE *b,
			    i4 update_mode,
			    i4 compression_type, DB_TRAN_ID *tran_id,
			    i4 sequence_number, i4 child,
			    i4 record_size);
FUNC_EXTERN DB_STATUS	dm1cxcomp_rec(DMP_ROWACCESS *rac,
			    char *rec, i4 rec_size, 
			    char *crec, i4 *crec_size);
FUNC_EXTERN DB_STATUS	dm1cxdel(i4 page_type, i4 page_size, DMPP_PAGE *b,
			    i4 update_mode, i4 compression_type,
			    DB_TRAN_ID *tran_id, u_i2 lg_id, i4 sequence_number,
			    i4 flags,
#define DM1CX_RECLAIM         0x01
			    i4 child);
FUNC_EXTERN DB_STATUS	dm1cxformat(i4 page_type, i4 page_size, DMPP_PAGE *b,
			    DMPP_ACC_PLV *loc_plv, 
			    i4 compression_type,
			    i4 kperpage, i4 klen, 
			    DB_ATTS **atts_array, i4 atts_count,
			    DB_ATTS **key_array, i4 key_count,
			    DM_PAGENO page, 
			    i4 index_page_type,
			    i4 spec,
			    i2 tidsize,
			    i4 Clustered);
FUNC_EXTERN DB_STATUS	dm1cxget(i4 page_type, i4 page_size, DMPP_PAGE *b,
			    DMP_ROWACCESS *rac,
			    i4 child,
			    char **entry, 
			    DM_TID *tidp, i4 *partno,
			    i4 *entry_size, 
			    u_i4 *row_low_tran,
			    u_i2 *row_lg_id,
			    ADF_CB *adfcb);
FUNC_EXTERN DB_STATUS	dm1cxget_deleted(i4 page_type, i4 page_size, 
			    DMPP_PAGE *b,
			    i4 *child, i4 *start_child,
			    DMPP_TUPLE_HDR *ixtup_info);
FUNC_EXTERN bool	dm1cxhas_room(i4 page_type, i4 page_size, DMPP_PAGE *b,
			    i4 compression_type,
			    i4 fill_factor,
			    i4 kperpage, i4 entry_size);
FUNC_EXTERN bool	dm1cx_isnew(DMP_RCB *r, DMPP_PAGE *leaf, i4 child);
FUNC_EXTERN bool	dm1cxjoinable(DMP_RCB *r, i4 compression_type,
			    DMPP_PAGE *curpage, DMPP_PAGE *sibpage,
			    i4 page_size,
			    i4 fill);
FUNC_EXTERN VOID	dm1cx_klength(i4 page_type, i4 page_size, DMPP_PAGE *b,
			    i4 child, i4 *key_size);

FUNC_EXTERN VOID	dm1cxLogErrorFcn(i4 errorno, DMP_RCB *rcb,
			    DMPP_PAGE *page, i4 page_type, i4 page_size,
			    i4 child, PTR FileName, i4 LineNumber);
#define	dm1cxlog_error(errorno,rcb,page,page_type,page_size,child) \
	dm1cxLogErrorFcn(errorno,rcb,page,page_type,page_size,child,\
				__FILE__, __LINE__)
			
FUNC_EXTERN DB_STATUS	dm1cxlshift(i4 page_type, i4 page_size,
			    DMPP_PAGE *cur, DMPP_PAGE *sib,
			    i4 compression_type,
			    i4 entry_len);
FUNC_EXTERN i4	dm1cxmax_length(DMP_RCB *r, DMPP_PAGE *b);
FUNC_EXTERN VOID	dm1cxmidpoint(i4 page_type, i4 page_size,
			    DMPP_PAGE *b,
			    i4 compression_type,
			    i4 *split_point);
FUNC_EXTERN DB_STATUS	dm1cxput(i4 page_type, i4 page_size, DMPP_PAGE *b,
			    i4 compression_type, 
			    i4 update_mode, DB_TRAN_ID *tran_id, u_i2 lg_id,
			    i4 sequence_number, i4 child,
			    char *key, i4 klen,
			    DM_TID *tidp, i4 partno);
FUNC_EXTERN DB_STATUS	dm1cxrclaim(i4 page_type, i4 page_size, DMPP_PAGE *b,
			    i4 child);
FUNC_EXTERN DB_STATUS	dm1cxreplace(i4 page_type, i4 page_size, DMPP_PAGE *b,
			    i4 update_mode,
			    i4 compression_type, DB_TRAN_ID *tran_id, u_i2 lg_id,
			    i4 sequence_number,
			    i4 position, char *key, i4 klen,
			    DM_TID *tidp, i4 partno);
FUNC_EXTERN DB_STATUS	dm1cxrshift(i4 page_type, i4 page_size,
			    DMPP_PAGE *current, DMPP_PAGE *sibling,
			    i4 compression_type, i4 split, i4 entry_len);
FUNC_EXTERN VOID	dm1cxtget(i4 page_type, i4 page_size, DMPP_PAGE *b,
			          i4 child, DM_TID *tidp, i4 *partno);
FUNC_EXTERN DB_STATUS	dm1cxtput(i4 page_type, i4 page_size, DMPP_PAGE *b,
			          i4 child, DM_TID *tidp, i4 partno);
FUNC_EXTERN VOID	dm1cxrecptr(i4 page_type, i4 page_size, DMPP_PAGE *b,
				  i4 child, char **key);
FUNC_EXTERN DB_STATUS   dm1cxclean(i4 page_type, i4 page_size, DMPP_PAGE *b, 
			    i4 compression_type,
			    i4 update_mode, DMP_RCB *r);
FUNC_EXTERN i4     dm1cxkeycount(i4 page_type, i4 page_size, DMPP_PAGE *b);
FUNC_EXTERN i4     dm1cxkperpage(i4 page_type, i4 page_size,
			    i4      relcmptlvl,
			    i4 spec,
			    i4 atts_cnt,
			    i4 key_len,
			    i4 indexpagetype,
			    i2	    tidsize,
			    i4	    Clustered);
FUNC_EXTERN DB_STATUS dm1cx_dput(DMP_RCB *r,
			    DMPP_PAGE *b,
			    i4	pos,
			    DM_TID *tid,
			    i4 *err_code);
FUNC_EXTERN DB_STATUS dm1cx_txn_get(i4 pgtype,DMPP_PAGE *b, i4 child, 
				i4 *row_low_tran, u_i2 *row_lg_id);
