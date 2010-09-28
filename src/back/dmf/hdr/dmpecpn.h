/*
**Copyright (c) 2004, 2010 Ingres Corporation
** All Rights Reserved
*/

#include "adp.h"

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
**
**	LOB data is stored externally to a row, with a "coupon" in the row
**	providing a pointer to the stored LOB data.  The term "coupon" is
**	used a bit loosely, unfortunately.  The on-disk structure stored
**	in the base row is an ADP_PERIPHERAL structure,  which is
**	defined in common in adp.h.  The ADP_PERIPHERAL structure has an
**	ADP_COUPON sub-structure which is the DMF part of the coupon.
**	This ADP_COUPON is defined as 5 i4's in adp.h, but it actually has
**	more structure to it that only DMF knows about.  That additional
**	structure is what is defined here, as a DMPE_COUPON.  Thus the word
**	"coupon" might apply to the entire on-disk ADP_PERIPHERAL, or
**	to the DMF part only.  The latter is more properly called the DMF
**	or DMPE coupon.
**
**	The DMPE_COUPON contains enough information for DMF to reconstruct the
**	data as needed.  The coupon contains some long term information which
**	appears on and is valid on disk, and some short term information, which
**	is zero on disk but possibly nonzero while the coupon is being
**	passed around the server as part of a base row.  The long term part
**	is what's needed to read or write the LOB data, the short term part
**	helps provide extra context so that the LOB data access and the
**	base row access can operate in a compatible way.  (e.g. use the same
**	lock modes.)
**
**	The long term parts of the DMF coupon are:
**	- A logical key value that uniquely identifies the base table row.
**	  The same logical key appears on all etab rows containing the LOB
**	  value.  Although one could have a unique logical key for each LOB
**	  in a base row (if there are more than one), it's not necessary since
**	  the DMF coupon also contains an etab table ID which uniqueifies
**	  things.  The most efficient way is to use one logical key for all
**	  the LOBs in one base table row.
**
**	- The etab table ID of the etab which contains the first segment of
**	  the LOB.  Each LOB points to the next segment, so if the LOB
**	  crosses etabs, it can be figured out as the LOB is read.
**
**	The short term parts of the DMF coupon are:
**	- The base table ID (base ID only, LOB's are not stored specific
**	  to partitions or indexes).
**	- The attribute ID for the LOB column (1-origin).
**	- Flags:
**	  - check the SRID (used by geometry columns to verify the
**	    embedded SRID against the att definition SRID)
**	  - Data-in-final-etab, also known as put-optimization.  This flag
**	    is set when a couponify has enough context available to
**	    send the incoming LOB data directly to the proper etab instead
**	    of to a holding temp.
**	  - Data-in-holding-temp, set when LOB data is in a short term
**	    holding temp table.
**
**	The baseid/attrid parts are at present only used for outgoing
**	LOBs.  Incoming LOB's only use the flags.
**
**	On some platforms, the DMF coupon is sized in adp.h as being 6 i4's
**	instead of 5 i4's.  This is a historical accident due to the
**	DMF coupon once upon a time containing a pointer.  The DMF coupon
**	size cannot change without breaking existing data, so we'll make sure
**	that the DMPE_COUPON matches the ADP_COUPON in size.  The DMF coupon
**	may not take more than 5 i4's to be platform independent, though.
**
** History:
**      17-Jan-1990 (fred)
**          Created.
**	6-May-2004 (schka24)
**	    RCB's belong to threads, and passing RCB's around fails in the
**	    face of parallel query.  Use TCB instead.  Attempt to doc.
**	26-Feb-2010 (troal01)
**	    Added CPN_SRID_STORAGE and unionized the temporary storage area
**	    of DMPE_COUPON.
**	12-Apr-2010 (kschendel) SIR 123485
**	    Redo the short-term part (again), this time to pass base ID, attr
**	    ID, and flags.  The new blob query CB (BQCB) will supply the
**	    necessary query lifetime context.  More redoing of the doc.
**	27-Apr-2010 (kschendel)
**	    Fix above to conditionalize the coupon fill.  Some compilers
**	    don't understand zero-size arrays.
*/

/* *** Important definition note: ***
** Nothing in DMPE_COUPON may be larger than an i4.
** The DMPE_COUPON may not be larger than i4[5].
**
** The ADP_COUPON is defined in terms of N i4's.  If the DMPE_COUPON
** contains a larger object, compiler added padding might screw things up,
** at the least causing incompatibility with existing on-disk coupons.
** If you need an object larger than an i4 (which isn't going to happen
** unless it's redefined entirely), define it as a char or i2/i4 array.
*/
/*
 * Structure used in the new union to temporarily
 * store the SRID for dmpe_get
 */
typedef struct _CPN_SRID_STORAGE
{
	i4 css_has_srid;
	i4 css_srid;
} CPN_SRID_STORAGE;

typedef struct _DMPE_COUPON
{
    DB_TAB_LOGKEY_INTERNAL
		    cpn_log_key; /*
				 ** Long term:
				 ** Logical key identifying this large object
				 */
    i4	    cpn_etab_id;	/* Long term: etab table id of first seg */
    i4	    cpn_base_id;	/* Short term: Base table's ID */
    i2	    cpn_att_id;		/* Short term: LOB column's attribute ID */
    u_i2    cpn_flags;		/* Short term: flags */
#define	DMPE_CPN_CHECK_SRID	0x0001	/* Set if SRID needs to be checked */
#define DMPE_CPN_FINAL		0x0002	/* Set if data in final etab, ie
					** LOB put optimization was done
					*/
#define DMPE_CPN_TEMP		0x0004	/* Set if data is in a holding temp */

    /* Fill (if necessary) to size of ADP_COUPON */
    /* IMPORTANT: this list of conditionals has to match the one
    ** in common adp.h. Unfortunately some compilers don't allow
    ** zero length arrays, so the fill can only be defined for
    ** platforms with an i4[6] ADP_COUPON.
    */

#if defined(axp_osf) || defined(ris_u64) || defined(i64_win) ||  \
    defined(i64_hp2) || defined(i64_lnx) || defined(axp_lnx) ||  \
    defined(a64_win)
    char    cpn_fill[sizeof (ADP_COUPON) - 
		(sizeof(DB_TAB_LOGKEY_INTERNAL) + sizeof(i4) + sizeof(i4)
		 + sizeof(i2) + sizeof(u_i2)) ];
#endif
}   DMPE_COUPON;


/* Define a macro to construct a pointer to the DMF coupon given a
** pointer to a DB_DATA_VALUE:
*/

#define DMPE_CPN_FROM_DBV_MACRO(dv) \
    ((DMPE_COUPON *) &(((ADP_PERIPHERAL *) ((dv)->db_data))->per_value.val_coupon))


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
** Name: DMPE_BQCB - Blob Query Context Block
**
** Description:
**
**	LOB processing is a bit unusual in that the storing and retrieval
**	of the LOB data (couponifying and redeeming) is divorced in time
**	from the base row access.  For instance, a base row might be
**	read in a child thread, passed to a parent thread, and as the
**	row heads "out the door" to the client, the LOB coupon is redeemed.
**	At this point, the RCB used for the original base row fetch might
**	have even been closed.  In the inbound direction, similar
**	problems exist:  LOB data has to be stored as it appears, which
**	is before the base row is stored -- which in turn means before
**	bulk-load vs row-put decisions are made, etc.
**
**	The Blob Query Context block (BQCB) connects LOB data processing
**	with base row processing by existing for the life of the query.
**	(When locators have been established, the BQCB can exist until
**	all locators are freed.)  Queries can exist across transactions,
**	such as when commits/aborts are issued inside DBP's;  but in that
**	situation, QEF will re-open everything and it's OK to wipe all
**	BQCB's at transaction end and re-establish new ones.  (Things such
**	as bulk-load cannot persist across transactions in this sense.)
**
**	In the get (outgoing) direction, the BQCB is used to a) allow etabs
**	to be opened using the same lockmode (page, table, mvcc) as the
**	base table was;  and b) allow verification of the embedded SRID
**	for geospatial data.
**
**	In the put (incoming) direction, the BQCB is used to a) centralize
**	logical-key generation for the coupons, and decouple it from the
**	base row handling;  b) centralize the bulk-load decision for etab
**	loading (which can be done independently of bulk-load on the base
**	table);  and c) allow etabs to be opened using the proper lockmode,
**	just as for get.
**
**	The BQCB can also serve as a central point for linking to cached
**	data used for accessing etabs, such as PCB's, DMT/DMR CB's, etc.
**	This will be implemented later.
**
**	One BQCB exists per table per session.  It is created when a
**	table containing LOB's is opened for read or write access (but
**	not for show, noaccess, or modify).  It is normally deleted when
**	the query ends, although as noted above it might continue to
**	exist for the lifetime of any locators.  (In which case the end-
**	of-query call resets some of the BQCB but does not delete it.)
**	All BQCB's for a session are on a linked list headed in the main
**	session thread's DML_SCB.  BQCB's never appear on child thread SCB's.
**
**	Given a base table ID, the relevant BQCB can be found by scanning
**	the BQCB list.  (Normally, only a few LOB-containing tables are
**	open per query, so no hashing should be needed.)  Any RCB opened
**	for read/write access to a LOB table will contain the BQCB pointer
**	in the RCB for direct access.
**
**	The BQCB contains:
**	- the base table ID (just the db_tab_base, don't need the index)
**	- Link to next BQCB on the SCB chain
**	- Pointer to RCB opened for logical-key generation
**	- Load-etabs indicator (yes, no, unknown)
**	- Single-row vs multi-row operation flag, default is single row
**	- etab locking hint: use table locking
**	- etab locking hint: use MVCC locking, and CRIB pointer if MVCC
**	- Number of LOB attributes
**	- Array with one entry per LOB column:
**		- attribute number (1 origin)
**		- Current SRID from att definition if geometry type
**		- (Future) real-etab get and put pop-cb+pcb lists
**
** Edit History:
**	12-Apr-2010 (kschendel) SIR 123485
**	    Invented.
**	13-Jul-2010 (jonj)
**	    Add bqcb_seq_number for MVCC-opened etabs.
*/

/* First, the BQCB per-attribute entry. */
struct _DMPE_BQCB_ATT
{
    i4	bqcb_att_id;	/* One-origin attribute ID in base table */
    i4	bqcb_srid;	/* Current SRID if geometry type, undefined if not */
    /* Future: link to per-thread put, get pop-cb's so we can hold etabs
    ** open and not do a bazillion memory alloc/dealloc per row.  Maybe also
    ** include a one-segment data holder (see e.g. dmpe-move, dmpe-replace).
    */
};
typedef struct _DMPE_BQCB_ATT DMPE_BQCB_ATT;

/* Then, the actual BQCB itself */
struct _DMPE_BQCB
{
    DM_OBJECT	hdr;		/* Standard object header...
				** Only obj_next is used for singly linked
				** list, obj_prev is not used.
				** Type is DM_BQCB_CB, ascii_id is below
				*/
#define BQCB_ASCII_ID	CV_C_CONST_MACRO('B','Q','C','B')

    struct _DMP_RCB *bqcb_logkey_rcb;  /* Pointer to RCB opened for logical
				** key generation, if doing LOB puts.  Note
				** that (at present) we assume that all logkey
				** generations are done in just one thread,
				** probably the main session thread, either
				** during couponification or at the top of
				** an insert or replace QP.
				*/
    struct _LG_CRIB *bqcb_crib;	/* Open etabs with MVCC, and use this CRIB.
				** bqcb_crib and bqcb_table_lock (below) are
				** mutually exclusive.  If neither is set,
				** use page locking on the etabs.
				*/
    i4		bqcb_base_id;	/* Base table db_tab_base */
    i4		bqcb_seq_number;/* rcb_seq_number from base DMP_RCB */
    i2		bqcb_natts;	/* Number of LOB attributes in the table */
    enum {
	BQCB_LOAD_UNKNOWN,	/* don't know yet */
	BQCB_LOAD_YES,		/* bulk-load etabs (if you can bulk load one,
				** you can probably bulk load 'em all) */
	BQCB_LOAD_NO		/* Don't bulk-load etabs */
    }		bqcb_load_etabs;  /* bulk-load-etabs indicator */
    bool	bqcb_multi_row;	/* Query is likely to involve multiple rows
				** and many LOB operations, e.g COPY
				*/
    bool	bqcb_table_lock;  /* Open etabs with table locking */
    bool	bqcb_x_lock;	/* Base table was opened X (so etab load is
				** a possibility;  can't load if base is non-
				** X-locked since it gets tricky to deal with
				** other inserting sessions.)
				*/
    DMPE_BQCB_ATT bqcb_atts[1];	/* Array of LOB attribute entries */
};
typedef struct _DMPE_BQCB DMPE_BQCB;

/* Size of BQCB is sizeof(DMPE_BQCB) + (bqcb_natts-1)*sizeof(DMPE_BQCB_ATT) */

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
	  bool		cleanup_source);

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

DB_STATUS dmpe_find_or_create_bqcb(DMP_RCB *r, DB_ERROR *dberr);

void dmpe_end_row(DMP_RCB *r);

DB_STATUS dmpe_query_end(bool was_error, bool delete_temps, DB_ERROR *dberr);

void dmpe_purge_bqcb(i4 base_id);
