/*
**Copyright (c) 2004 Ingres Corporation
** All Rights Reserved
*/

/**
** Name: DMPECPN.H - Define coupon for peripheral Objects
**
** Description:
**      This file contains the definitions for coupons and related items for
**	support of peripheral datatypes within DMF.
**
** History:
**      17-Jan-1990 (fred)
**          Created.
**      24-Jun-1993 (fred)
**          Added function prototypes for dmpe functions.
**	28-jul-1998 (somsa01)
**	    Added another parameter to dmpe_move() which is set if a bulk
**	    load operation is going on with blobs into extension tables.
**	    (Bug #92217)
**	20-aug-1998 (somsa01)
**	    Added more parameters to dmpe_modify() to help us properly
**	    modify a base table's extension tables.
**      03-aug-1999 (stial01)
**          Added prototype for dmpe_create_extension
**      12-aug-1999 (stial01)
**          Added prototype for dmpe_default_pagesize
**      31-aug-1999 (stial01)
**          Added prototype for dmpe_get_per_value, 
**          Removed dmpe_default_pagesize
**      08-dec-1999 (stial01)
**          Removed unecessary prototype
**      10-jan-2000 (stial01)
**          Added support for 64 bit RCB pointer in DMPE_COUPON
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      20-mar-2002 (stial01)
**          Updates to changes for 64 bit RCB pointer in DMPE_COUPON   
**          Removed att_id from coupon, making room for 64 bit RCB ptr in
**          5 word coupons. Also added macros to copy/compare coupon rcb
**          which may be unaligned. (b103156)
**      02-jan-2004 (stial01)
**          Changes for CREATE TABLE WITH [NO]BLOBJOURNALING
**          Changes for MODIFY to ADD_EXTEND with BLOB_EXTEND
**	6-May-2004 (schka24)
**	    Carry TCB in coupon instead of RCB.  RCB's belong to
**	    individual threads, but TCB's don't.
**     16-oct-2006 (stial01)
**          Added DMPE_LLOC_CXT. 
**      09-feb-2007 (stial01)
**          Added prototype for dmpe_free_locator.
**      17-may-2007 (stial01)
**          Changed prototype for dmpe_free_locator
**	05-Nov-2008 (jonj)
**	    SIR 120874 Standarize function output error parameter to
**	    DB_ERROR * rather than i4 *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dmpe_free_temps() function converted to DB_ERROR *
[@history_template@]...
**/

/*}
** Name: DMPE_COUPON - DMF's idea of the coupon structure
**
** Description:
**      This data structure describes the information stored and provided by DMF
**	for its manipulation of peripheral datatypes.  DMF stores an
**	ADF_PERIPHERAL data structure on disk for each attribute of each table
**	which contains a peripheral data element.  The header portion of this is
**	fixed, and set by ADF.  DMF knows little about it.  The coupon portion,
**	however, ADF does not understand.  DMF is in complete control of this
**	portion of the data value.
**
**	The DMPE_COUPON contains enough information for DMF to reconstruct the
**	data as needed.  The coupon contains some long term information which
**	appears on and is valid on disk, and some short term information, which
**	is valid only while the coupon is held, and the base table to which the
**	coupon belongs is open (in some thread).
**	These two portions are marked below as LT and ST, respectively.
**
**	The TCB portion of the coupon has the following meanings:
**
**	0  - (DMPE_NULL_TCB) Empty, null, or uninitialized coupon
**
**	-1 - (DMPE_TEMP_TCB) Coupon references an anonymous holding temp
**	     table.  The base table may or may not be known (but clearly
**	     you can't tell the base table from this coupon).
**
**	-2 - (DMPE_PUT_OPTIM_TCB) Coupon describes an object that the
**	     upper level machinery managed to route directly to its final
**	     resting place in an etab.  The usual routing is to a holding
**	     temp, and at base record-put or record-replace time the value
**	     is moved from holding temp to etab.  The -2 value indicates
**	     that the move can be skipped.
**
**	a TCB address - Coupon references a value in some known etab for
**	a known base table (the TCB is for the base table).
**
**	Note that the coupon's etab ID and logical key are sufficient to
**	retrieve a blob value, the TCB isn't needed just for that.  The
**	TCB address is used for puts to real etabs so that we can do things
**	like read the etab catalog for the base table, determine if the
**	base table is a session temp, etc.
**
** History:
**      17-Jan-1990 (fred)
**          Created.
**	6-May-2004 (schka24)
**	    RCB's belong to threads, and passing RCB's around fails in the
**	    face of parallel query.  Use TCB instead.  Attempt to doc.
[@history_template@]...
*/
typedef struct _DMPE_COUPON
{
    DB_TAB_LOGKEY_INTERNAL
		    cpn_log_key; /*
				 ** LT --
				 **  Logical key identifying this large object
				 */
    i4	    cpn_etab_id; /* LT -- table id of first seg */
    char    cpn_tcb[sizeof (ADP_COUPON) - 
		sizeof(DB_TAB_LOGKEY_INTERNAL) - sizeof(i4)];
}   DMPE_COUPON;

#define DMPE_TCB_ASSIGN_MACRO(src, dst)			\
(MEcopy( ((char *)&(src)), sizeof(PTR), ((char *)&(dst)) ))
#define DMPE_TCB_COMPARE_NE_MACRO(a, b)			\
(MEcmp(((char *)&(a)), ((char *)&(b)), sizeof(PTR)))

/* Fake TCB values used as flags.
** *NOTE* dmpe.c uses 0 and -1 internally
** The exact contents here don't matter specifically, it's neither
** 0 nor -1 nor a valid pointer, and works with the above macros.
** Define fake TCB flag saying that the value-put machinery managed to
** get the value into its final etab instead of a holding temp.
*/
#define DMPE_TCB_PUT_OPTIM "\377\377\377\376\377\377\377\376"



/*}
** Name: DMPE_LLOC_CXT  - Transaction Locator context.
**
** Description:
**      For locator support, an array of coupons will be allocated.
**      When this array fills, the coupons will spill into a temp table.
**
** History:
**      16-oct-2006 (stial01)
**          Created.
*/

typedef struct _DMPE_LLOC_CXT
{
    i4			lloc_1_reserved;
    i4			lloc_2_reserved;
    SIZE_TYPE		lloc_length;		/* Length of control block. */
    i2			lloc_type;		/* Type of control block */
#define LLOC_CB		DM_LLOC_CB
    i2			lloc_s_reserved;
    PTR			lloc_l_reserved;
    PTR			lloc_owner;
    i4			lloc_ascii_id;		/* Ascii id for debugging */
#define	LLOC_ASCII_ID	CV_C_CONST_MACRO('L', 'L', 'O', 'C')
    PTR			lloc_odcb;
    PTR			lloc_tbl;	/* array overflows to temp table */
    i4			lloc_mem_pgcnt;
    i4			lloc_max;
    i4			lloc_cnt;
    ADP_PERIPHERAL	lloc_adp[1]; /* array of ADP_PERHIPHERALs */
} DMPE_LLOC_CXT;


/*
** DMPE Function prototypes.
*/
DB_STATUS
dmpe_call(i4        op_code ,
           ADP_POP_CB     *pop_cb );

DB_STATUS
dmpe_delete(ADP_POP_CB     *pop_cb );

DB_STATUS
dmpe_move(ADP_POP_CB	*pop_cb,
	  i4		load_blob,
	  bool		cleanup_source);

DB_STATUS
dmpe_replace(ADP_POP_CB	*pop_cb );

DB_STATUS
dmpe_destroy(DMU_CB	  *base_dmu ,
              DB_ERROR	  *dberr );

DB_STATUS
dmpe_modify(DMU_CB	  *base_dmu ,
             DMP_DCB	  *dcb ,
             DML_XCB	  *xcb ,
             DB_TAB_ID	  *tbl_id ,
             i4	  db_lockmode ,
             i4	  temporary ,
             i4	  truncated ,
	     i4	  base_page_size,
	     i4   blob_add_extend,
             DB_ERROR	  *dberr );
DB_STATUS
dmpe_relocate(DMU_CB	  *base_dmu ,
               DB_ERROR	  *dberr );

DB_STATUS
dmpe_create_extension(
	DML_XCB                 *xcb,
	DMP_RELATION            *rel_record,
	i4                      etab_pagesize,
	DMP_LOCATION            *loc_array,
	i4                      loc_count,
	i4                      attr_id,
	i2                      attr_type,
	DMP_ETAB_CATALOG        *etab_record,
	DB_ERROR                *dberr);

DB_STATUS
dmpe_journaling(
	i4			blob_journaling,
	DML_XCB			*xcb,
	DB_TAB_ID		*base_tab_id,
	DB_ERROR		*dberr);

DB_STATUS dmpe_free_temps(
	DML_ODCB	*odcb,
	DB_ERROR	*dberr);

DB_STATUS dmpe_free_locator(ADP_POP_CB	*pop_cb);
