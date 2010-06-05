
/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

/**
** Name: DM1C.H - Definitions of common constants and functions.
**
** Description:
**      This header describes the common routines at the 1 level.
**	It also defines the common constants that are used in calls
**	to all one level routines independent of access method type.
**
** History:
**      28-oct-1985 (derek)
**          Created new for Jupiter.
**	29-may-1989 (rogerk)
**	    Changed dm1c_get, dm1c_comp_rec, dm1c_uncomp_rec to not be VOID.
**	5-dec-1990 (bryanp)
**	    Added #define's and FUNC_EXTERNS for the dm1cx* routines used with
**	    Btree index pages. It seems silly to create a <dm1cx.h>,
**	    since the two sets of routines are logically a single layer.
**	14-jun-1991 (Derek)
**	    Minor changes to return status.
**	 7-jul-1992 (rogerk)
**	    Prototyping DMF.
**	    Moved bryanp's dm1cx prototype definitions to dm1cx.h to avoid
**	    having to include dm1b.h everyplace that makes dm1c calls.
**	16-jul-1992 (kwatts)
**	    MPF project. Removed externs for procedures now replaced by
**	    accessors and introduced procedure dm1c_getaccessors.
**	11-oct-1992 (jnash)
**	    Reduced logging project: Added support for new System Catalog
**	    Page Type and its associated page accessors.
**	     -  Add new record_type, dbid, tabid and indxid params to
**		dmpp_allocate() prototype.
**	     -  Elim dmpp_get params atts_array and atts_count.  Tuples
**		are now never uncompressed by dmpp_get.
**	     -  Add "record_type" and "tid" params to dmpp_uncompress and
**		dmppxuncompress routines, required to support old-style
**		uncompression within dm1cn_uncompress().   This is
**		admittedly strange.
**	     -  Elim dmpp_isempty.  Callers now use dmpp_tuplecount to
**		determine if a page is empty.
**	22-oct-1992 (rickh)
**	    Replaced all references to DMP_ATTS with DB_ATTS.
**	14-dec-1992 (rogerk)
**	    Reduced logging project: Added DM1C_REALLOCATE flag.
**      19-Jul-1993 (fred)
**          Added dm1c_p*() routine's prototypes.
**	26-jul-1993 (rogerk)
**	    Added new operation mode: DM1C_MDEALLOC. Used for dm1b_rcb_update.
**      19-apr-1995 (stial01)
**          Added DM1C_LAST to perform btree select last for MANMANX.
**	12-may-1995 (shero03)
**	    Add Key Level Vectors
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to some accessor prototypes:
**		dmpp_allocate, dmpp_check_page, dmpp_format, dmpp_getfree,
**		dmpp_reclen, dmpp_tuplecount, dmpp_load, dmpp_put.
**	25-apr-1996 (wonst02)
**	    Add new "directions" for Rtree access.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Support:
**		Add page_size argument to dmpp_isnew accessor prototype.
**      22-jul-1996 (ramra01 for bryanp)
**          Add row_version argument to dmpp_uncompress prototype.
**          Add row_version(tup_info) argument to dmpp_get prototype.
**          Add table_version(tup_info) argument to dmpp_put, 
**		dmpp_load prototypes.
**	13-sep-1996 (canor01)
**	    Add buffer argument to dmpp_uncompress prototype.
**	24-sep-1996 (shero03)
**	    Add object data type and hilbert size to the klv
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Converted get/delete record modes to bit flags;
**          Delete accessor: added reclaim_space 
**          Put/load accessor: changed DMPP_TUPLE_INFO param to table_version
**      22-jan-97 (dilma04)
**          Change DM1C_BYTID_SI_UPD to DM1C_SI_UPD.
**      27-jan-97 (dilma04)
**          Added DM1C_SI_UPDATE search mode.
**	15-apr-1997 (shero03)
**	    Remove dimension as an input parm to get_klv.
**      29-jul-1997 (stial01)
**          Added DM1C_MCLEAN, DM1C_MRESERVE
**      07-jul-1998 (stial01)
**          Added DM1C_IX_DEFERRED
**	28-jul-1998 (somsa01)
**	    Added another parameter to dm1c_pput() which describes whether
**	    or not we are bulk loading blobs into extension tables.
**	    (Bug #92217)
**      05-oct-98 (stial01)
**          Updated dmpp_dput parameters, Added dmpp_put_hdr
**      13-Nov-98 (stial01)
**          Renamed DM1C_SI_UPD->DM1C_GET_SI_UPD, 
**          Renamed DM1C_SI_UPDATE->DM1C_FIND_SI_UPD
**      16-nov-98 (stial01)
**          Added DM1C_PKEY_PROJECTION
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      09-feb-1999 (stial01)
**          Added relcmptlvl to dmpp_ixsearch prototype.
**      05-may-1999 (nanpr01,stial01)
**          Added DM1C_DUP_ROLLACK, DM1C_DUP_CONTINUE
**      05-dec-1999 (nanpr01)
**          Added tuple header as parameter to the prototype.
**      02-may-2000 (stial01)
**          Added DM1C_RAAT
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format prototype,
**	    DM1C_ZERO_FILL, DM1C_ZERO_FILLED defines.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Changed prototype for dmpp_allocate, dmpp_isnew page accessors
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      26-nov-2002 (stial01)
**          Added prototype for dm1c_get_tlv()
**	30-mar-03 (wanfr01)
**          Bug 109914, INGSRV 2156
**	    Add DM1C_DUP_NONKEY flag to indicate si_dupcheck did not need
**          to perform duplicate checking (so no control lock grabbed)
**      28-may-2003 (stial01)
**          Added line_start parameter to dmpp_allocate accessor
**	15-Jan-2004 (schka24)
**	    Expose logical key utility.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      25-jun-2004 (stial01)
**          Added prototype for dm1c_get
**      07-jan-2005 (stial01)
**          Fixed dm1c_getpgtype flags
**      21-feb-2005 (stial01)
**          Cross integrate 470401 (inifa01) for INGSRV2569 b111886
**          Added DM1C_DMVE_COMP
**	13-Apr-2006 (jenjo02)
**	    Add DM1C_CREATE_CLUSTERED
**	12-Apr-2008 (kschendel)
**	    Pass ADF-CB, not ptr, to uncompress
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm1c_? functions converted to DB_ERROR *
**	    macroized dm1c_badtid().
**	23-Oct-2009 (kschendel) SIR 122739
**	    Integrate various changes for new rowaccessor scheme.
**      09-dec-2009 (stial01)
**          Added DM1C_SPLIT_DUPS
**	14-Apr-2010 (kschendel) SIR 123485
**	    dm1c LOB related routines changed prototypes, fix here.
**/

/*
**  Forward and/or External function references.
*/

/*
**  Defines of other constants.
*/

/*
**      Search modes.
*/
#define                 DM1C_SPLIT      1L
#define			DM1C_JOIN	2L
#define			DM1C_FIND	3L
#define			DM1C_EXTREME	4L
#define			DM1C_RANGE	5L
#define			DM1C_OPTIM	6L
#define			DM1C_TID	7L
#define			DM1C_POSITION	8L
#define			DM1C_SETTID	9L
#define                 DM1C_LAST      10L
#define                 DM1C_FIND_SI_UPD 11L
#define                 DM1C_SPLIT_DUPS  12L

/*
**      Record Type & Record Access Type
*/

#define                 DM1C_DEFERRED      0x01L    /* Same as RCB_U_DEFERRED */
#define                 DM1C_DIRECT        0x02L    /* Same as RCB_U_DIRECT   */
#define                 DM1C_ROWLK         0x04L    /* Set if row locking     */
#define			DM1C_DMVE_COMP     0x08L    /* Rollback, compressed   */

/* Ordinary vs isam-index record type for load-put accessor. */

#define			DM1C_LOAD_NORMAL	0	/* Normal put */
#define			DM1C_LOAD_ISAMINDEX	1	/* Isam-index put */

/*
**      Tuple Header Access Type
*/
#define                 DM1C_CLEAN         0x01     /* Clear tuple header */
#define                 DM1C_MODIFY        0x02     /* Set page modified */

/*
**      Search direction.
*/
#define                 DM1C_EXACT	1L
#define			DM1C_NEXT	2L
#define			DM1C_PREV	3L
#define			DM1C_OVERLAPS	4L	/* Rtree overlaps search */
#define			DM1C_INTERSECTS	5L      /* Rtree intersects search */
#define			DM1C_INSIDE	6L      /* Rtree is inside search */
#define			DM1C_CONTAINS	7L      /* Rtree contains search */

/*
**      Insert directions.
*/
#define                 DM1C_LEFT       1L
#define			DM1C_RIGHT	2L

/*
**      Get/delete record modes.
*/
#define                 DM1C_GETNEXT         0x01L
#define                 DM1C_BYTID           0x02L
#define                 DM1C_BYPOSITION      0x04L
#define                 DM1C_GETPREV         0x08L /* get previous */
#define                 DM1C_GET_SI_UPD      0x10L /* get to update sec index*/
#define                 DM1C_PKEY_PROJECTION 0x20L /* Primary key projection */
#define                 DM1C_RAAT            0x40L /* Record at a time op */

/*
** Duplicate check flag values.
*/
#define                 DM1C_NODUPLICATES 0L
#define                 DM1C_DUPLICATES   1L
#define                 DM1C_ONLYDUPCHECK 2L
#define                 DM1C_REALLOCATE   3L

/*
** Duplicate checking optimization
**
** DM1C_DUP_ROLLBACK is set only in cases where we know for certain that
** a duplicate error condition will result in a rollback, in which
** case DMF can dupcheck in indices after updating the base table.
**
** Otherwise DM1C_DUP_CONTINUE is set, and we must do all duplicate checking
** before updating the base table. That is we must assume the worst case,
** that duplicate errors will be ignored by QEF (or RAAT)
*/
#define                 DM1C_DUP_CONTINUE 0x01L
#define                 DM1C_DUP_ROLLBACK 0x02L
#define                 DM1C_DUP_NONKEY   0x04L

/*
** Operation mode.
*/
#define                 DM1C_MALLOCATE    1L
#define                 DM1C_MDELETE      2L
#define                 DM1C_MGET         3L
#define                 DM1C_MPUT         4L
#define                 DM1C_MREPLACE     5L
#define                 DM1C_MSPLIT       6L
#define                 DM1C_MDEALLOC     7L
#define                 DM1C_MCLEAN       8L
#define                 DM1C_MRESERVE     9L

/*
** RTree constants
** (In the future this will be a quality of the object data type)
*/

#define                 DM1C_RTREE_DIM	  2L

/* dmpp_format() directive to MEfill page or not */
#define			DM1C_ZERO_FILL    0L
#define			DM1C_ZERO_FILLED  1L

/*
** Constants used for flag argument to dm1c_getpgtype
*/
#define DM1C_CREATE_DEFAULT	0x01
#define DM1C_CREATE_INDEX	0x02
#define DM1C_CREATE_ETAB	0x04
#define DM1C_CREATE_TEMP	0x08
#define DM1C_CREATE_CORE	0x10
#define DM1C_CREATE_CATALOG	0x20
#define DM1C_CREATE_CLUSTERED	0x40

/*
** Function Prototypes
*/

FUNC_EXTERN VOID	dm1c_get_plv(
				    i4			relpgtype,
				    DMPP_ACC_PLV	**plv);

FUNC_EXTERN DB_STATUS dm1c_getpgtype(
				    i4 page_size,
				    i4 create_flags,
				    i4 rec_len,
				    i4 *page_type);

FUNC_EXTERN DB_STATUS	dm1c_getklv(
                                    ADF_CB	        *adf_scb,
				    DB_DT_ID		obj_dt_id,
				    DMPP_ACC_KLV	*klv);

FUNC_EXTERN DB_STATUS	dm1c_sys_init(
				    DB_OBJ_LOGKEY_INTERNAL  *objkey,
				    DB_ATTS		    *atts,
				    i4		    atts_count,
				    char		    *rec,
				    i4		    *logkey_type,
				    DB_ERROR		    *dberr);

FUNC_EXTERN DB_STATUS	dm1c_get_high_low_key(
				DMP_TCB *tcb,
				DMP_RCB *rcb,
				u_i4 *high,
				u_i4 *low,
				DB_ERROR *dberr);

FUNC_EXTERN VOID	dm1cBadTidFcn(
				DMP_RCB *rcb, 
				DM_TID *tid,
				PTR	FileName,
				i4	LineNumber);
#define dm1c_badtid(rcb,tid) \
	dm1cBadTidFcn(rcb,tid,__FILE__,__LINE__)

FUNC_EXTERN DB_STATUS   dm1c_pdelete(DB_ATTS	*atts,
				     DMP_RCB	*rcb,
				     char       *record,
				     DB_ERROR	*dberr);

FUNC_EXTERN DB_STATUS   dm1c_pget(DB_ATTS   *atts,
				  DMP_RCB   *rcb,
				  char	    *record,
				  DB_ERROR  *dberr);

FUNC_EXTERN DB_STATUS   dm1c_pput(DB_ATTS   *atts,
				  DMP_RCB   *rcb,
				  char      *record,
				  DB_ERROR  *dberr);

FUNC_EXTERN DB_STATUS   dm1c_preplace(DB_ATTS	*atts,
				      DMP_RCB	*rcb,
				      char	*old,
				      char      *new,
				      DB_ERROR  *dberr);

FUNC_EXTERN DB_STATUS	dm1c_get(DMP_RCB	*rcb,
				DMPP_PAGE	*data,
				DM_TID		*tid,
				PTR		record,
			        DB_ERROR  	*dberr);

FUNC_EXTERN i4 dm1c_cmpcontrol_size(i4 compression_type,
				i4 att_count,
				i4 relversion);

FUNC_EXTERN i4 dm1c_compexpand(i4 compression_type,
				DB_ATTS **att_ptrs,
				i4 att_count);

FUNC_EXTERN DB_STATUS dm1c_rowaccess_setup(
				DMP_ROWACCESS *rac);


/*}
** Name: ACCESSOR definitions.
**
** Description:
**	This section describes the vector that contains page accessor
**	functions.
**
**          Note that these accessor functions are the only way to access page
**        contents safely since they map the various page formats onto a
**        consistent interface.
**
** History:
**      18-jun-91 (paull)
**	   Created.
**	04-jun-92 (kwatts)
**	   New for 6.5 project.
**	21-may-96 (ramra01)
**	   Added new param to get, put and load accessor routine 
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space 
**          Put/load accessor: changed DMPP_TUPLE_INFO param to table_version
**	13-aug-99 (stephenb)
**	    Alter def for dmpp_uncompress, buffer should be a char** since it
**	    is freed in the called function.
**	16-mar-2004 (gupsh01)
**	    Added rcb_adf_cb to the parameter list.
**	25-Aug-2009 (kschendel) b121804
**	    Uncompress and xuncompress have to have the same call sequence!
**	23-Oct-2009 (kschendel) SIR 122739
**	    "transfer" level accessors deleted for new rowaccessor scheme.
**	    Put call doesn't need record type.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add lg_id parameter to
**	    dmpp_delete, dmpp_get, dmpp_put prototypes.
*/
/*
** Name: DMPP_ACC_PLV -- Page Level Vector Accessor functions.
**
** Description:
**
**   The structure contains the list of access functions for page level access.
**   The 'tcb_acc_plv" field in the table control block point at one of
**   these structures.
*/

struct _DMPP_ACC_PLV {

	DB_STATUS (*dmpp_allocate)(
                        DMPP_PAGE       *data,
			DMP_RCB         *r,
			char		*record,
                        i4         record_size,
                        DM_TID          *tid,
			i4		line_start);
	bool      (*dmpp_isempty)(
			DMPP_PAGE       *data,
			DMP_RCB		*rcb);
	DB_STATUS (*dmpp_check_page)(
			DM1U_CB			*dm1u,
			DMPP_PAGE		*page,
			i4			page_size);
	VOID (*dmpp_delete)(
			i4	   page_type, 
			i4	   page_size,
			DMPP_PAGE  *page,
                        i4         update_mode,
			i4         reclaim_space,
                        DB_TRAN_ID      *tran_id,
			u_i2		lg_id,
                        DM_TID          *tid,
                        i4         record_size);
	VOID (*dmpp_format)(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DM_PAGENO       pageno,
			i4		stat,
			i4		fill_option);
	DB_STATUS (*dmpp_get)(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DM_TID     	*tid,
			i4    	*record_size,
			char       	**record,
			i4		*row_version,
			u_i4		*row_low_tran,
			u_i2		*row_lg_id,
			DMPP_SEG_HDR	*seg_hdr);
	i4 (*dmpp_getfree)(
			DMPP_PAGE	*page,
			i4		page_size);
	i4 (*dmpp_get_offset)(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4		item);
	i4 (*dmpp_isnew)(
			DMP_RCB		*r,
			DMPP_PAGE	*page,
			i4		line_num);
	i4 (*dmpp_ixsearch)(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DB_ATTS		**keyatts,
			i4		keys_given,
			char		*key,
			bool		partialkey,
			i4		direction,
			i4              relcmptlvl,
			ADF_CB 		*adf_cb);
	DB_STATUS (*dmpp_load)(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE       *data,
			char		*record,
			i4         record_size,
			i4		record_type,
			i4		fill,
			DM_TID		*tid,
			u_i2		current_table_version,
			DMPP_SEG_HDR    *seg_hdr);
	VOID (*dmpp_put)(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4     	update_mode,
			DB_TRAN_ID  	*tran_id,
			u_i2		lg_id,
			DM_TID	    	*tid,
			i4	    	record_size,
			char	    	*record,
			u_i2		current_table_version,
			DMPP_SEG_HDR	*seg_hdr);
	DB_STATUS (*dmpp_reallocate)(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE 	*page,
			i4 		line_num,
			i4 		reclen);
	VOID (*dmpp_reclen)(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			i4       	lineno,
			i4     	*record_size);
	i4 (*dmpp_tuplecount)(
			DMPP_PAGE	*page,
			i4		page_size);
	DB_STATUS (*dmpp_dput)(
			DMP_RCB		*rcb,
			DMPP_PAGE       *page,
			DM_TID          *tid,
			i4		*err_code);
	DB_STATUS (*dmpp_clean)(
			i4		page_type,
			i4		page_size,
			DMPP_PAGE	*page,
			DB_TRAN_ID	*tranid,
			i4		lk_type,
			i4		*avail_space);
};

/*
** Name: DMPP_ACC_KLV -- Key level vector functions.
**
** Description:
**   This structure contains a list of access functions for Key construction.
**   The 'tcb_acc_klv' field in the table control block points to one of
**   these structures.
*/

struct _DMPP_ACC_KLV {

	u_i2		dimension;	/* dimension of base object */
	u_i2		hilbertsize;	/* sizeof a hilbert value */
	DB_DT_ID	obj_dt_id;	/* data type of base object */
	DB_DT_ID	nbr_dt_id;	/* data type of an nbr */
	DB_DT_ID	box_dt_id;	/* data type of the approp box */

	char		range_type;	/* I - integers, F - floats */

	/*
	** This is an invalid, uniqe nbr
	*/
	char		null_nbr[DB_MAXRTREE_NBR];

	/*
	**  Compute an NBR given a datatype and its range
	*/
	DB_STATUS (*dmpp_nbr)(
            		ADF_CB	  	*adf_scb,
			DB_DATA_VALUE   *dv_obj,     /* object */
			DB_DATA_VALUE   *dv_range,   /* BOX */
			DB_DATA_VALUE   *dv_result); /* NBR */

	/*
	**  Compute a Hilbert value given an NBR
	**  Note:  the resultant binary length must be set
	**         to the desired Hilbert size
	*/
	DB_STATUS (*dmpp_hilbert)(
            		ADF_CB	  	*adf_scb,
			DB_DATA_VALUE   *dv_obj,     /* NBR */
			DB_DATA_VALUE   *dv_result); /* BINARY */

	/*
	**  Test if the two NBRs overlap each other (0=False, 1=True)
	*/
	DB_STATUS (*dmpp_overlaps)(
            		ADF_CB	  	*adf_scb,
			DB_DATA_VALUE   *dv_obj1,    /* NBR */
			DB_DATA_VALUE   *dv_obj2,    /* NBR */
			DB_DATA_VALUE   *dv_result); /* I2  */

	/*
	**  Test if the a NBR is inside another NBR (0=False, 1=True)
	*/
	DB_STATUS (*dmpp_inside)(
            		ADF_CB	  	*adf_scb,
			DB_DATA_VALUE   *dv_obj1,    /* NBR */
			DB_DATA_VALUE   *dv_obj2,    /* NBR */
			DB_DATA_VALUE   *dv_result); /* I2  */

	/*
	**	Compute the smallest nbr that totally includes both
	**	input nbrs.
	*/
	DB_STATUS (*dmpp_unbr)(
            		ADF_CB	  	*adf_scb,
			DB_DATA_VALUE   *dv_obj1,    /* NBR */
			DB_DATA_VALUE   *dv_obj2,    /* NBR */
			DB_DATA_VALUE   *dv_result); /* NBR */

} ;
