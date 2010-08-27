/*
** Copyright (c) 1986, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <tm.h>
#include    <pc.h>
#include    <sr.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dmp.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dmftrace.h>
#include    <dma.h>
#include    <cm.h>
#include    <cui.h>

/**
**
**  Name: DM2UPIND.C - Parallel Index table utility operation.
**
**  Description:
**      This file contains routines that perform the parallel index table
**	functionality.
**      dm2u_pindex - Create parallel indices on a table.
**
**	At present, parallel index is logged as a series of regular
**	INDEX operations.  So if the index is re-done via rollforwarddb,
**	it goes to regular index, not here.  This code is unaware of
**	rollforward.
**
**  History:
**	10-april-1998 (nanpr01)
**	    created.
**	11-Aug-1998 (jenjo02)
**	    When counting tuples, include RCB counts as well.
**	23-Dec-1998 (jenjo02)
**	    TCB->RCB list must be mutexed while it being read.
**	28-may-1999 (nanpr01)
**	    Added table name and owner name in parameter list to display
**	    sensible error messages in errlog.log.
**	05-may-2000 (stial01, gupsh01)
**	    Added rec_cnt for consistency checking
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_table_access() if neither
**	    C2 nor B1SECURE.
**	22-Feb-2001 (jenjo02)
**	    Fixed a bunch of problems in which local stack variables
**	    were being used instead of the corresponding 
**	    DM2U_INDEX_CB or DM2U_MXCB variable. (BUG 104062)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      24-apr-2003 (stial01)
**          Project index columns only into the exchange buffers (b110061)
**	22-Jan-2004 (jenjo02)
**	    Preliminary support of Global indexes and TID8s.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      14-apr-2004 (stial01)
**          E_DM007D_BTREE_BAD_KEY_LENGTH should return klen and max klen
**      11-may-2004 (stial01)
**          Removed unnecessary svcb_maxbtreekey
**      02-dec-2004 (stial01)
**          Btree key length check added tid twice (it's already included
**          in mx_kwidth), fix. (same as 19-May-2004 dm2uind.c change)
**	11-Mar-2005 (thaju02)
**	    Use $online idxs relation info. (B114069)
**	16-May-2005 (thaju02)
**	    dm2u_pindex: in building index attribute list check that 
**          attribute has not been previously dropped. (B114519)
**	13-Apr-2006 (jenjo02)
**	    Prototype change to dm1b_kperpage() for CLUSTERED
**	    primary Btree.
**	    Remove DM1B_MAXKEY_MACRO throttle from leaf entry size check.
**	25-Feb-2008 (kschendel) SIR 122739
**	    New row-accessor structure, fix here.
**	04-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_?, dmse_? functions converted 
**	    to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	01-apr-2010 (toumi01) SIR 122403
**	    Add support for column encryption.
*/

/*{
** Name: dm2u_pindex - Create parallel indices on a table.
**
** Description:
**      This routine is called from dmu_pindex() to create an index on a table.
**
** Inputs:
**	dcb				The database id.
**	xcb				The transaction id.
**	index_name			Name of the index to be created.
**	location			Location where the index will reside.
**      l_count                         Count of number of locations.
**	tbl_id				The internal table id of the base table
**					to be indexed.
**	structure			Structure of the index table.
**	i_fill				Fill factor of the index page.
**	l_fill				Fill factor of the leaf page.
**	d_fill				Fill factor of the data page.
**	compressed			TRUE if compressed records.
**	index_compressed		Specifies whether index is compressed.
**	min_pages			Minimum number of buckets for HASH.
**	max_pages			Maximum number of buckets for HASH.
**	kcount				Count of key attributes in the index.
**	acount				Count of attributes in the index.
**	key				Pointer to key array for indexing.
**	db_lockmode			Lockmode of the database for the caller.
**	allocation			Space pre-allocation amount.
**	extend				Out of space extend amount.
**	page_size			Page size for this index.
**	index_flags			Index options:
**					    DM2U_GATEWAY  - gateway index.
**	gwattr_array			Gateway attributes - valid only for
**					a gateway index.
**	gwchar_array			Gateway characteristics - valid only for
**					a gateway index.
**	gwsource			The source for the gateway object.  This
**					value is passed down to be interpreted
**					by the individual gateway.
**	reltups				This field is used only when modifying
**					the magic table 'iiridxtemp' as part of
**					a sysmod operation. In that case, it
**					contains the number of tuples in
**					iirelation and is passed on to
**					dm2u_load_table for use in bucket
**					calculation. Otherwise, it is zero.
**	char_array			Index characteristics - used only for
**					a gateway index.
**	relstat2			Flags to go into relstat2.
**	tbl_pri				Table cache priority.
**
** Outputs:
**	tup_info			Number of tuples in the indexed table
**					or the bad key length if key length
**					is too wide.
**	idx_id				The internal table id assigned to the
**					index table.
**      errcb.err_code			The reason for an error return status.
**	errcb.err_data			Additional information.
**					E_DM0007_BAD_ATTR_NAME
**					E_DM001C_BAD_KEY_SEQUENCE
**					E_DM001D_BAD_LOCATION_NAME
**					    err_data specifies location
**					E_DM005A_CANT_MOD_CORE_STRUCT
**					E_DM005D_TABLE_ACCESS_CONFLICT
**					E_DM005E_CANT_UPDATE_SYSCAT
**					E_DM005F_CANT_INDEX_CORE_SYSCAT
**					E_DM0066_BAD_KEY_TYPE
**					E_DM007D_BTREE_BAD_KEY_LENGTH
**					    err_data gives bad length value
**					E_DM009F_ILLEGAL_OPERATION
**					E_DM00D1_BAD_SYSCAT_MOD
**					E_DM0103_TUPLE_TOO_WIDE
**					E_DM010F_ISAM_BAD_KEY_LENGTH
**					    err_data gives bad length value
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-april-1998 (nanpr01)
**	    Created from dm2u_index().
**      18-mar-1999 (stial01)
**          Moved check for bad keylength back after addition of TIDP to keylist
**          Also set status = E_DB_ERROR if error is set and fixed error
**          handling to loop through the list of all MXCB's allocated.
**	21-mar-1999 (nanpr01)
**	    Get rid of extents since we support one raw table per raw location.
**      12-nov-1999 (stial01)
**          Do not add tidp to key for ANY unique secondary indexes
**	22-Feb-2001 (jenjo02)
**	    Fixed a bunch of problems in which local stack variables
**	    were being used instead of the corresponding 
**	    DM2U_INDEX_CB or DM2U_MXCB variable. (BUG 104062)
**      24-jul-2002 (hanal04) Bug 108330 INGSRV 1847
**          Initialise new mx_dmveredo values based on indxcb_dmveredo.
**	15-Apr-2003 (jenjo02)
**	    Check row counts of all indexes and report all diffs,
**	    not just the first.
**	6-Feb-2004 (schka24)
**	    Revise temp filename generation scheme, eliminate dmu count.
**	28-Feb-2004 (jenjo02)
**	    Modified to new dm2u_load_table() form with potential
**	    for multiple source/target partitions.
**	    For the time being, Global indexes on partitioned
**	    tables can't be done in parallel, so we'll do
**	    them serially.
**	04-Mar-2004 (jenjo02)
**	    Allocate tpcb_srecord buffer. When threading targets,
**	    source record is copied here.
**	08-Mar-2004 (jenjo02)
**	    Deleted tpcb_olocations, tpcb_oloc_count.
**	10-Mar-2004 (jenjo02)
**	    Init new mx_Mlocation fields in MXCB, fix
**	    computation of buf_size.
**	11-Mar-2004 (jenjo02)
**	    Added TPCB pointer array in mx_tpcb_ptrs.
**	16-Mar-2004 (jenjo02)
**	    Correct casting of locArray.
**	17-Mar-2004 (jenjo02)
**	    Keep new tuple counts in tpcb_newtup_cnt rather
**	    than mx_newtup_cnt; that's where dm2u_update_catalogs
**	    expects to find them.
**	09-Apr-2004 (jenjo02)
**	    Ensure correct alignment of SPCB/TPCB arrays
**	    to avoid 64-bit BUS errors.
**	15-apr-2004 (somsa01)
**	    Ensure proper alignment of mx_tpcb_ptrs to avoid 64-bit BUS
**	    errors.
**	20-Apr-2004 (schka24)
**	    Include dm2u record header in record arrays.
**	30-apr-2004 (thaju02)
**	    If online_index_build, use tpcb_name_id & tpcb_name_gen to construct
**	    filename.
**	22-jun-2004 (thaju02)
**	    Make key_map[] big enough to handle max 1024 columns. (B112539)
**	08-jul-2004 (thaju02)
**	    Online Modify - after sort/load, save fhdr pageno. For
**	    online_index_build (rename/swapping files), use saved fhdr
**	    pageno to update catalogs. (B112610)
**	6-Aug-2004 (schka24)
**	    Table access check now a no-op, remove.
**	25-aug-2004 (thaju02)
**	    Rolldb of online modify with persistent indices fails.
**	    If online_index_build, do not log DM0LINDEX. (B112887)
**	16-Aug-2004 (schka24)
**	    mct/mx usage cleanups.
**	16-dec-04 (inkdo01)
**	    Init. various att_collID's and cmp_collID's.
**	18-dec-2004 (thaju02)
**	    Replaced m->mx_dupchk_tabid with tp->tpcb_dupchk_tabid.
**	11-Mar-2005 (thaju02)
**	    Use $online idxs relation info. (B114069)
**	16-May-2005 (thaju02)
**	    In building index attribute list, check that attribute
**	    has not been previously dropped. (B114519)
**	26-May-2005 (jenjo02)
**	    Clean up error handling to ensure that "err_code" is
**	    not zero when returning E_DB_ERROR, which produces
**	    QE0018's.
**	21-Jul-2005 (jenjo02)
**	    Init (new) mx_phypart pointer to NULL;
**	26-May-2006 (jenjo02)
**	    Screwed all that attribute making stuff into
**	    dm2uMakeIndAtts(), add support for indexes on 
**	    Clustered tables.
**	15-Aug-2006 (jonj)
**	    Support indexes on Temporary Tables.
**      23-Apr-2007 (hanal04) Bug 118099
**          Do not take the table control lock here. Allow the phys table
**          lock to be acquired first and the control lock for the base
**          will subsequently be acquired in dm2u_create().
**          This reduces deadlock opportunities during index creation
**          caused by the use of control locks that are not held for
**          the duration of a transaction (bad locking semantics).
**      25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
**	26-Feb-2008 (kschendel) SIR 122739
**	    Extract all the MXCB setup gunk to a new common routine.
**	    Pass a real relstat to dm2u-create, not some nonsense mix.
**	7-Aug-2008 (kibro01) b120596
**	    Add flag to note that we're inside an online modify so temporary
**	    tables can be created in an alternate data location
**	14-Jul-2009 (thaju02) (B122300)
**	    If dm2uMakeIndAtts errs, set status to avoid loading index
**	    and go straight to error handling.
**     16-Sep-2009 (hanal04) Bug 122594
**          Remove the unnecessary E_DM005D check.
**	29-Sept-2009 (troal01)
**		Add geospatial support.
**	29-Sep-2009 (thaju02) B122614
**	    If an error should occur before table control lock is taken
**	    (in dm2u_create) close table NOPURGE to avoid E_DM9C5D.
**	    Also initialize m/ms ptrs to avoid SEGV in error condition. 
**	5-Nov-2009 (kschendel) SIR 122739
**	    Fix outrageous name confusion re acount vs kcount.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Recognize direct-io for load option.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Update dm2t-destroy-temp-tcb call.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Open sources no-coupon to avoid unnecessary LOB handling.
**	16-Jul-2010 (kschendel) SIR 123450
**	    Log the actual compression types.
*/
STATUS
dm2u_pindex(
DM2U_INDEX_CB   *index_cbs)
{
    DMP_DCB	     *dcb;
    DML_XCB	     *xcb;
    DML_SCB          *scb;
    DM2U_INDEX_CB    *index_cb = index_cbs;
    DB_OWN_NAME	     owner;
    DM2U_MXCB	     *m = (DM2U_MXCB *)0 , *ms = (DM2U_MXCB *)0; 
    DM2U_MXCB	     *curr_m = (DM2U_MXCB *)0;
    DM2U_MXCB        *nextm;
    DM2U_MXCB	     *mxcb_cb = (DM2U_MXCB *)0;
    DMP_RCB	     *r = (DMP_RCB *)0;
    DMP_TCB	     *t;
    i4	     	     i, j, k, ind;
    i4	     	     NumCreAtts;
    DB_TAB_TIMESTAMP timestamp;
    DMF_ATTR_ENTRY  att_entry[DB_MAXIXATTS + 1];
    DMF_ATTR_ENTRY  *att_list[DB_MAXIXATTS + 1];
    i4	     	     local_error = 0;
    DB_ERROR	     *dberr, *indxcb_dberr;
    i4		     error;
    DB_STATUS	     status, local_status;
    i4               ref_count;
    DB_TAB_NAME	     table_name;
    DB_LOC_NAME	     tbl_location;
    i4	     	     journal;
    i4	     	     journal_flag;
    i4               log_relstat;
    i4               setrelstat;
    i4               tbl_lk_mode;
    i4	     	     tbl_access_mode = DM2T_A_READ_NOCPN;
    i4               timeout = 0, numberOfIndices;
    u_i4	     db_sync_flag;
    ADF_CB	     *adf_cb;
    LG_LSN	     lsn;
    bool             syscat;
    bool             sconcur;
    bool             concurrent_index  = FALSE;
    bool             modfile_open = FALSE;
    i4               rec_cnt;
    bool	     key_projection = FALSE;
    i4		     proj_col_map[(DB_MAX_COLS + 31) /32];
    DM2U_TPCB	     *tp;
    i4		     online_index_build;
    bool	     TempTable, control_lk_held = FALSE;
    i4		     TableLockList;
    DB_ERROR	     local_dberr;

    for ( numberOfIndices = 1, index_cb = index_cbs; index_cb->q_next;
	  index_cb = (DM2U_INDEX_CB *) index_cb->q_next)
	numberOfIndices++;

    index_cb = index_cbs;

    /* Use 1st index_cb to catch "applies to all" error code */
    dberr = index_cb->indxcb_errcb;

    CLRDBERR(dberr);

    dcb = index_cb->indxcb_dcb;
    xcb = index_cb->indxcb_xcb;
    scb = xcb->xcb_scb_ptr;

    online_index_build = ((index_cb->indxcb_index_flags & DM2U_ONLINE_END) != 0);

    /* Base Table is a TempTable? */
    TempTable = (index_cb->indxcb_tbl_id->db_tab_base < 0) ? TRUE : FALSE;

    /* TempTable table locks are on session lock list */
    if ( TempTable )
	TableLockList = scb->scb_lock_list;
    else
	TableLockList = xcb->xcb_lk_id;

    /* take the ckp lock to prevent online ckp to start */
    if ( !TempTable )
    {
	status = dm2u_ckp_lock(dcb, (DB_TAB_NAME *)NULL,
			       (DB_OWN_NAME *)NULL, xcb, dberr);
	if (status != E_DB_OK)
	    return (E_DB_ERROR);
    }

    concurrent_index = 
		((index_cb->indxcb_relstat2 & TCB2_CONCURRENT_ACCESS) != 0);
    if ( TempTable )
	tbl_lk_mode = DM2T_X;
    else if ( concurrent_index )
    {
        if (index_cb->indxcb_tbl_id->db_tab_base <= DM_SCONCUR_MAX)
	    tbl_lk_mode = DM2T_IS;
        else
	    tbl_lk_mode = DM2T_S;
    }
    else
        if (index_cb->indxcb_tbl_id->db_tab_base <= DM_SCONCUR_MAX)
	    tbl_lk_mode = DM2T_IX;
        else
	    tbl_lk_mode = DM2T_X;

    timeout = dm2t_get_timeout(scb, index_cb->indxcb_tbl_id); 
						/* from set lockmode */

    /* For gateway tables we maynot call these parallel index build routines */
    if (index_cb->indxcb_index_flags & DM2U_GATEWAY)
    {
	SETDBERR(dberr, 0, E_DM009F_ILLEGAL_OPERATION);
	return(E_DB_ERROR);
    }
 
    /*  Open up the table. */
    status = dm2t_open(dcb, index_cb->indxcb_tbl_id, 
		 tbl_lk_mode, DM2T_UDIRECT, 
		 tbl_access_mode, timeout, 
		 (i4)20, (i4)0, xcb->xcb_log_id,
                 TableLockList, (i4)0, (i4)0,
                 index_cb->indxcb_db_lockmode, 
		 &xcb->xcb_tran_id, 
		 &timestamp, &r, (DML_SCB *)0, dberr);
    if (status != E_DB_OK)
	return(E_DB_ERROR);

    /* 
    ** Indexes on partitioned tables can't yet be done
    ** in parallel; do them each serially.
    */
    if ( r->rcb_tcb_ptr->tcb_rel.relstat & TCB_IS_PARTITIONED )
    {
	/* Get rid of our RCB; dm2u_index will reopen the table */
	(VOID)dm2t_close(r, DM2T_NOPURGE, &local_dberr);

	for ( i = 0, index_cb = index_cbs; 
	      i < numberOfIndices && status == E_DB_OK;
	      i++, index_cb = (DM2U_INDEX_CB*)index_cb->q_next )
	{
	    status = dm2u_index(index_cb);
	}
	return(status);
    }



    /*
    ** Project only necessary columns into the exchange buffer
    ** Init RCB for projection of columns needed for all indexes
    */
    MEfill(sizeof(proj_col_map), '\0', (PTR) proj_col_map);
    status = dm2r_init_projection(r, &local_dberr);
    if (status == E_DB_OK)
	key_projection = TRUE;
    else
    {
	key_projection = FALSE;
	TRdisplay("Warning could not initialize DMF projection\n");
	status = E_DB_OK;
    }

    for (ind = 0, index_cb = index_cbs; ind < numberOfIndices; 
	 ind++, index_cb = (DM2U_INDEX_CB *)index_cb->q_next)
    {
	modfile_open = FALSE;
	/* error code specific to this index_cb */
	indxcb_dberr = index_cb->indxcb_errcb;
	CLRDBERR(indxcb_dberr);
	*index_cb->indxcb_tup_info = 0;

	dmf_svcb->svcb_stat.utl_index++;

	for (i=0; i < DB_MAXIXATTS + 1; i++)
	    att_list[i] = &att_entry[i];

        t = r->rcb_tcb_ptr;
        r->rcb_xcb_ptr = xcb;

	/* Check for duplicate TempTable Index name */
	if ( TempTable )
	{
	    DMP_TCB	*it;

	    for ( it = t->tcb_iq_next; 
	          it != (DMP_TCB*)&t->tcb_iq_next;
		  it = it->tcb_q_next )
	    {
		if ( (MEcmp((char*)index_cb->indxcb_index_name,
			 (char*)&it->tcb_rel.relid,
			 sizeof(DB_TAB_NAME)) == 0) )
		{
		    SETDBERR(indxcb_dberr, 0, E_DM0078_TABLE_EXISTS);
		    break;
		}
	    }
	    if ( indxcb_dberr->err_code )
		break;
	}

        MEcopy((char *)&t->tcb_rel.relowner, sizeof(DB_OWN_NAME),
               (char *)&owner);

	/*
	** Check for NOLOGGING - no updates should be written to the log
	** file if this session is running without logging.
	*/
	if ( xcb->xcb_flags & XCB_NOLOGGING || TempTable )
	    r->rcb_logging = 0;

	ref_count = t->tcb_open_count;
	syscat = (t->tcb_rel.relstat & (TCB_CATALOG | TCB_EXTCATALOG)) != 0;
	sconcur = (t->tcb_rel.relstat & TCB_CONCUR);
	journal = ((t->tcb_rel.relstat & TCB_JOURNAL) != 0);
	journal_flag = journal ? DM0L_JOURNAL : 0;

	/* Close the table, if Index cannot continue.	*/

	if ( concurrent_index )
	{
	    /*
	    ** Concurrent index not allowed on system catalogs or
	    ** non READONLY base tables.
	    ** Make sure base table is in READONLY mode
	    */
	    if ( syscat || !(t->tcb_rel.relstat2 & TCB2_READONLY))
	    {
	        status = E_DB_ERROR;
		SETDBERR(indxcb_dberr, 0, E_DM0069_CONCURRENT_IDX_ERR);
		break;
	    }
	}

        /*
        ** Can't index an sconcur system catalog.
        ** Can't index a non-sconcur system catalog
        ** without update privilege.
        */

        if ( (sconcur != 0) ||
             (syscat &&
              ((scb->scb_s_type & SCB_S_SYSUPDATE) == 0)
             )
           )
        {
	    status = E_DB_ERROR;
	    SETDBERR(indxcb_dberr, 0, E_DM005F_CANT_INDEX_CORE_SYSCAT);
            break;
        }
        STRUCT_ASSIGN_MACRO(t->tcb_rel.relid, table_name);
        STRUCT_ASSIGN_MACRO(t->tcb_rel.relloc, tbl_location);
        if (index_cb->indxcb_tab_name)
            *index_cb->indxcb_tab_name = t->tcb_rel.relid;
        if (index_cb->indxcb_tab_owner)
            *index_cb->indxcb_tab_owner = t->tcb_rel.relowner;

	/* The MXCB maker wants some stuff passed in the indxcb_rfp area
	** so that we don't have a zillion routine parameters...
	*/
	index_cb->indxcb_rfp.rfp_log_id = xcb->xcb_log_id;
	index_cb->indxcb_rfp.rfp_lk_id = xcb->xcb_lk_id;
	index_cb->indxcb_rfp.rfp_name_id = dmf_svcb->svcb_tfname_id;
	index_cb->indxcb_rfp.rfp_name_gen =
	CSadjust_counter((i4 *)&dmf_svcb->svcb_tfname_gen, 1);

	/* Create and partially set up the index MXCB.
	** No sources (|| index doesn't use an SPCB), one target.
	*/

	status = dm2uMakeIndMxcb(&mxcb_cb, index_cb, r,
			timeout, tbl_lk_mode, 0, 1);
        if (status != E_DB_OK)
            break;

        m = mxcb_cb;

	/* Set up a couple things inconvenient for make-mxcb. */
	m->mx_inmemory = TempTable;
	m->mx_tranid = &xcb->xcb_tran_id;
	m->mx_new_relstat2 |= TCB2_PARALLEL_IDX;

	/* Hook up the MXCB list */
	m->mx_next = m->mx_prev = NULL;
	if (curr_m != NULL)
	    curr_m->mx_next = m;
	else
	    ms = m;		/* Starting mxcb */
	curr_m = m;

	/*
	** Build the attributes.
	**
	** Returns with m->mx_ab_count, m->mx_ai_count, m->mx_c_count,
	** NumCreAtts corrected and all the various attribute lists
	** and bits built.
	*/
	if ( status = dm2uMakeIndAtts(index_cb, m, r, att_list, &NumCreAtts, 
			key_projection,
			(key_projection) ? proj_col_map : NULL, 
			indxcb_dberr) )
	    break;

        /* Now that we know we are really going to do the
        ** modify, set the rcb_uiptr to point to the scb_ui_state
        ** value so it can check for user interrupts.
        */
	r->rcb_uiptr = &scb->scb_ui_state;

	/* The (only) target TPCB pointer */
	tp = m->mx_tpcb_next;

	/*
	** All's ready now.
	**
	** Creating a Temp Table index is easier as it doesn't involve
	** logging or catalog updates.
	**
	** Create a Temp Index TCB, open a mx_buildrcb on it to
	** be used by the dm2u_pload_table routines to create the index
	**
	** When we post-process everything, the Temp Index TCB's relation
	** will be updated with stuff from the build.
	*/
	if ( TempTable )
	{
	    m->mx_buildrcb = (DMP_RCB*)NULL;

	    /* Make a TempIndex TCB, indxcb_idx_id */
	    status = dm2uCreateTempIndexTCB(t, m, index_cb,
					NumCreAtts, att_list,
					indxcb_dberr);

	    /* ...then open it, getting mx_buildrcb */
	    if ( status == E_DB_OK )
		status = dm2t_open(dcb, 
			 	index_cb->indxcb_idx_id, 
			 	LK_X, DM2T_UDIRECT, 
			 	DM2T_A_WRITE_NOCPN,
			 	(i4)0,		/* timeout */
			 	(i4)20, 	/* maxlocks */
			 	(i4)0, 		/* savepoint id */
			 	xcb->xcb_log_id,
				TableLockList,
			 	(i4)0, 		/* sequence */
			 	(i4)0,		/* iso_level */
			 	index_cb->indxcb_db_lockmode, 
			 	&xcb->xcb_tran_id, 
			 	&timestamp, 
			 	&m->mx_buildrcb, 
			 	scb,
			 	indxcb_dberr);
	    if ( status )
		break;
	    /* Enough setup for this index, on to the next one */
	    continue;
	}


        /*  Create a table to contain the (permanent)index. */
        setrelstat = TCB_DUPLICATES;
        if (t->tcb_rel.relstat & TCB_JOURNAL) 
            setrelstat |= TCB_JOURNAL;
        if (t->tcb_rel.relstat & TCB_CATALOG)
            setrelstat |= TCB_CATALOG;
        if (t->tcb_rel.relstat & TCB_EXTCATALOG)
            setrelstat |= TCB_EXTCATALOG;

        status = dm2u_create(dcb, xcb, 
	  index_cb->indxcb_index_name, &owner, index_cb->indxcb_location, 
	  index_cb->indxcb_l_count,
          index_cb->indxcb_tbl_id, index_cb->indxcb_idx_id, 
	  (i4)1, (i4)0, setrelstat, m->mx_new_relstat2,
          index_cb->indxcb_structure, m->mx_width, m->mx_width, NumCreAtts,
          att_list, index_cb->indxcb_db_lockmode,
          DM_TBL_DEFAULT_ALLOCATION, DM_TBL_DEFAULT_EXTEND, 
	  m->mx_page_type, m->mx_page_size, index_cb->indxcb_qry_id, 
	  index_cb->indxcb_gwattr_array, index_cb->indxcb_gwchar_array, 
	  index_cb->indxcb_gw_id, (i4)0,
          index_cb->indxcb_gwsource, index_cb->indxcb_char_array, 
	  m->mx_dimension, m->mx_hilbertsize, 
	  m->mx_range, index_cb->indxcb_tbl_pri, NULL, 0, NULL /* DMU_CB */,
	  &local_dberr);
        if (status != E_DB_OK)
        {
          /*
          ** If the base table is journaled, but journaling has been turned
          ** off on this database, then create will return with a warning
          ** that journaling will be enabled at the next checkpoint
          ** (E_DM0101_SET_JOURNAL_ON).  We should ignore this warning.
          */
          if (local_dberr.err_code != E_DM0101_SET_JOURNAL_ON)
          {
            *indxcb_dberr = local_dberr;
            break;
          }
          status = E_DB_OK;
        }
	control_lk_held = TRUE;

        /*
        ** Force any pages left in buffer cache for this table.
        ** This is required for recovery as a non-redo log record immediately
        ** follows.  We must use DM0P_CLFORCE here because the table
        ** may be in use if concurrent builds are taking place.
        */
        status = dm0p_close_page(t, xcb->xcb_lk_id, 
				 xcb->xcb_log_id, 
				 DM0P_CLFORCE, indxcb_dberr);
        if (status != E_DB_OK)
            break;

        m->mx_table_id.db_tab_base = index_cb->indxcb_idx_id->db_tab_base;
        m->mx_table_id.db_tab_index = index_cb->indxcb_idx_id->db_tab_index;
	tp->tpcb_tabid.db_tab_base = m->mx_table_id.db_tab_base;
	tp->tpcb_tabid.db_tab_index = m->mx_table_id.db_tab_index;

        /*
        **  Load the index.
        **
	**  At this point the following MXCB fields should have the following
	**  values based on the type of index that was created.
	**
	**      HASH indices never include TIDP as a key.
	**      Unique indices never include TIDP as a key
	**
	**      Non-unique BTREE and ISAM indices include TIDP as a key field.
	**
	**  Data fields are supported for all index types.
        **
        **  mx_b_key_atts  - One pointer for each base table attribute named.
        **  mx_ab_count    - Count of above entries.
        **  mx_i_key_atts  - One pointer for each key for this index record plus
        **         one for the implied TIDP key on non-HASH indices.
        **         The key_offset field must be correctly set relative
        **         to beginning of the index record.
        **  mx_ai_count    - Count of above entries.
        **  mx_kwidth       - Sum of index key attributes plus TIDP is non-HASH
        **         indices.  Non-key (data) fields of a btree index
        **         are also included.
        **  mx_data_atts   - One pointer for each attribute in the table or
        **         index being modified. Used for data compression.
        **  mx_width        - Total width of index record, includes TIDP.
        **  mx_idom_map     - Map of index attributes to base table attributes
        **                for key and data fields in an index excepting TIDP.
        **  mx_att_map      - The key number for a attribute number.
        **                   For non-HASH
        **         tables this would be set for the TIDP field.
        **  mx_unique       - If a unique index is begin created,
        **                   this field should
        **         set so that the load code checks
        **                   for non-unique keys.
        **
        */
        /*
        ** Select file synchronization mode.
        */
        if (dcb->dcb_sync_flag & DCB_NOSYNC_MASK)
            db_sync_flag = 0;
	else
            db_sync_flag = DI_USER_SYNC_MASK;
	if (dmf_svcb->svcb_directio_load)
	    db_sync_flag |= DI_DIRECTIO_MASK;
	if (!online_index_build)
	    db_sync_flag |= DI_PRIVATE_MASK;

        /*
        ** Allocate fcb for each location.
	** Because we need to log the filenames we created, figure out
	** the generators here rather than letting dm2f do it.
        */
        status = dm2f_build_fcb(m->mx_lk_id, m->mx_log_id,
			      tp->tpcb_locations, tp->tpcb_loc_count, 
			      m->mx_page_size,
                              DM2F_ALLOC, DM2F_MOD, &m->mx_table_id,
			      tp->tpcb_name_id,
			      tp->tpcb_name_gen,
                              (DMP_TABLE_IO *)0, 0,
			      indxcb_dberr);
        if (status != E_DB_OK)
            break;

	if (online_index_build)
	{
	      i4	k = 0;
	      i4	nofiles = 0;
	      DML_XCCB *xccb;
	      bool  	found_xccb;
	      i4    	logging, dm0l_flag;

	      /* Need to do those file renames first */
	      while ( !nofiles)
	      {
		    DM_FILE		mod_filename; /* for modtable */

		    dm2f_filename(DM2F_MOD, &m->mx_table_id,
			tp->tpcb_locations[k].loc_id, 
			tp->tpcb_name_id, tp->tpcb_name_gen,
			&mod_filename);

		    /*
		    ** Scan the xcb list of deleted files to find the 
		    ** DM2F_DES filename for each location
		    ** NOTE We can't recreate the filenames here because they
		    ** were generated using svcb->svcb_DM2F_TempTblCnt
		    */ 
		    for ( xccb = xcb->xcb_cq_next, found_xccb = FALSE;
			  xccb != (DML_XCCB *)&xcb->xcb_cq_next;
			  xccb = xccb->xccb_q_next )
		    {
			if (!MEcmp((char *)&xccb->xccb_t_table_id,
				&index_cb->indxcb_online_tabid, 
				sizeof(DB_TAB_ID))
			    && !MEcmp((char *)&xccb->xccb_p_name,
				&tp->tpcb_locations[k].loc_ext->physical,
				tp->tpcb_locations[k].loc_ext->phys_length))
			{
			    found_xccb = TRUE;
			    break;
			}
		    }

		    if (!found_xccb)
		    {
			TRdisplay("Can't find xccb for DM2F_DES file for loc %d\n",k);
			/* *err_code = FIX_ME */
			/* Here's a QE0018 just waiting to happen! */
			status = E_DB_ERROR;
			break;
		    }

		    /* FIX ME RAW loc */

		    /*
		    ** RENAME $online_tabid_ix_indexnum.DM2F_DES
		    ** TO     modtable.DM2F_MOD
		    ** Log the rename as a DM0L_FCREATE of a DM2F_MOD file 
		    ** Remove the pending XCCB_F_DELETE XCCB 
		    */

		    if (DMZ_SRT_MACRO(13))
			TRdisplay("ONLINE MODIFY rename %~t %~t \n",
			xccb->xccb_f_len, &xccb->xccb_f_name, 
			sizeof(mod_filename), &mod_filename);

		    if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
		    {
			status = dm0l_frename(m->mx_log_id, journal_flag,
				&tp->tpcb_locations[k].loc_ext->physical,
				tp->tpcb_locations[k].loc_ext->phys_length,
				&xccb->xccb_f_name, &mod_filename,
				&t->tcb_rel.reltid, k,
				tp->tpcb_locations[k].loc_config_id,
				(LG_LSN *)0, &lsn, indxcb_dberr);
			if (status != E_DB_OK)
			    break;
		    }

		    status = dm2f_rename(m->mx_lk_id, m->mx_log_id,
			&tp->tpcb_locations[k].loc_ext->physical,
			tp->tpcb_locations[k].loc_ext->phys_length,
			&xccb->xccb_f_name, xccb->xccb_f_len,
			&mod_filename, (i4)sizeof(mod_filename),
			&m->mx_dcb->dcb_name, indxcb_dberr);
		    if (status != E_DB_OK)
			break;

		    /*  Remove from queue. */
		    xccb->xccb_q_prev->xccb_q_next = xccb->xccb_q_next;
		    xccb->xccb_q_next->xccb_q_prev = xccb->xccb_q_prev;

		    /*  Deallocate. */ 
		    dm0m_deallocate((DM_OBJECT **)&xccb);
		
		    k++;
		    if (k >= tp->tpcb_loc_count)
			break;
	      }
	      /* we want dm2u_pload_table to initialize mct block only; do not sort */
	      m->mx_flags |= MX_ONLINE_INDEX_BUILD;
	}
	else
	{
	    /*
	    ** Log and create the file(s).
	    */
	    status = dm2u_file_create(dcb, m->mx_log_id, 
		      m->mx_lk_id, &m->mx_table_id, db_sync_flag, 
		      tp->tpcb_locations, tp->tpcb_loc_count,  
		      m->mx_page_size,
                      ((xcb->xcb_flags & XCB_NOLOGGING) == 0),
                      journal, indxcb_dberr);
	}
        if (status != E_DB_OK)
	    break;

        modfile_open = TRUE;

        if (index_cb->indxcb_index_flags & DM2U_ONLINE_MODIFY)
        {
            DMT_CB              dmt_cb;
            DMT_CHAR_ENTRY      characteristics[3];
            DB_TAB_NAME         newtab_name;
	    i4			mem_needed;
	    DMP_MISC		*misc_cb = (DMP_MISC *)0;
	    i4			err_code;
	    DMF_ATTR_ENTRY	*attr_entry;
	    DMF_ATTR_ENTRY	**attr_ptrs;

            /*
            ** create $dupchk tbl here. we can't create it
            ** during load/sort, since threads all reference the
            ** same xaction, may result in $dupchk tbls with
            ** duplicate table ids.
            */
            MEfill(sizeof(DMT_CB), '\0', (PTR) &dmt_cb);
            dmt_cb.length = sizeof(DMT_CB);
            dmt_cb.type = DMT_TABLE_CB;
            dmt_cb.dmt_flags_mask = (DMT_DBMS_REQUEST | DMT_ONLINE_MODIFY);
            dmt_cb.dmt_db_id = (PTR)xcb->xcb_odcb_ptr;
            dmt_cb.dmt_tran_id = (PTR)xcb;

            STprintf(newtab_name.db_tab_name, "$dupchk_%x_%x",
                    index_cb->indxcb_idx_id->db_tab_base,
                    index_cb->indxcb_idx_id->db_tab_index);

            STmove(newtab_name.db_tab_name, ' ',
                    sizeof(newtab_name.db_tab_name),
                    newtab_name.db_tab_name);

            MEfill(sizeof(dmt_cb.dmt_owner.db_own_name),
                    (u_char)' ', (PTR)dmt_cb.dmt_owner.db_own_name);

            characteristics[0].char_id = DMT_C_PAGE_SIZE;
            characteristics[0].char_value = m->mx_page_size;
            characteristics[1].char_id = DMT_C_PAGE_TYPE;
            characteristics[1].char_value = m->mx_page_type;
            characteristics[2].char_id = DMT_C_DUPLICATES;
            characteristics[2].char_value = DMT_C_ON;

            dmt_cb.dmt_char_array.data_address = (PTR)characteristics;
            dmt_cb.dmt_char_array.data_in_size = 2 * sizeof(DMT_CHAR_ENTRY);

            dmt_cb.dmt_location.data_address = (char *)(index_cb->indxcb_location);
            dmt_cb.dmt_location.data_in_size =
                        index_cb->indxcb_l_count * sizeof(DB_LOC_NAME);

	    mem_needed = sizeof(DMP_MISC) + (m->mx_ai_count * sizeof(DMF_ATTR_ENTRY *)) +
			(m->mx_ai_count * sizeof(DMF_ATTR_ENTRY));

	    status = dm0m_allocate(mem_needed, 0, (i4)MISC_CB, 
				(i4)MISC_ASCII_ID, (char *)0,
				(DM_OBJECT **)&misc_cb, 
				indxcb_dberr);
	    if (status)
	    {
		uleFormat(indxcb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
				(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
				NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
				0, mem_needed);
		break;
	    }

	    attr_ptrs = (DMF_ATTR_ENTRY **)((char *)misc_cb + sizeof(DMP_MISC));
	    misc_cb->misc_data = (char*)attr_ptrs;
	    attr_entry = (DMF_ATTR_ENTRY *)((char *)attr_ptrs + (m->mx_ai_count * 
				sizeof(DMF_ATTR_ENTRY *)));

	    for (i = 0; i < m->mx_ai_count; i++)
	    {
		DB_ATTS		*atts;

		
		atts = m->mx_i_key_atts[i];
		attr_ptrs[i] = &attr_entry[i];
		MEmove(atts->attnmlen, atts->attnmstr, ' ',
			DB_ATT_MAXNAME, attr_entry[i].attr_name.db_att_name);
		attr_entry[i].attr_type = atts->type;
	 	attr_entry[i].attr_size = atts->length;
	 	attr_entry[i].attr_precision = atts->precision;
	 	attr_entry[i].attr_flags_mask = 0;
	 	attr_entry[i].attr_collID = -1;
	 	attr_entry[i].attr_geomtype = -1;
	 	attr_entry[i].attr_srid = -1;
	 	attr_entry[i].attr_encflags = 0;
	 	attr_entry[i].attr_encwid = 0;
		COPY_DEFAULT_ID(atts->defaultID, attr_entry[i].attr_defaultID);
	    }

            dmt_cb.dmt_attr_array.ptr_address = (PTR)attr_ptrs;
            dmt_cb.dmt_attr_array.ptr_in_count = m->mx_ai_count;
            dmt_cb.dmt_attr_array.ptr_size = sizeof(DMF_ATTR_ENTRY);

	    status = dmt_create_temp(&dmt_cb);

	    if (status != E_DB_OK)
	    {
		*indxcb_dberr = dmt_cb.error;
		break;
	    }

	    if (DMZ_SRT_MACRO(13))
		TRdisplay("%@ dm2u_pindex: %~t created dupchktbl = %~t (%d,%d)\n",
                sizeof(DB_TAB_NAME), index_cb->indxcb_index_name,
                sizeof(DB_TAB_NAME), dmt_cb.dmt_table,
                dmt_cb.dmt_id.db_tab_base, dmt_cb.dmt_id.db_tab_index);

	    STRUCT_ASSIGN_MACRO(dmt_cb.dmt_id,
			*(index_cb->indxcb_dupchk_tabid));

            m->mx_flags |= MX_ONLINE_MODIFY;
	    tp->tpcb_dupchk_tabid = index_cb->indxcb_dupchk_tabid;
        }
    }


    /*
    ** Load the index.
    */
    if (status == E_DB_OK)
    {
	/*
	** Project only necessary columns into the exchange buffer
	** Init RCB for projection of columns needed for all indexes
	*/
	if (key_projection)
	{
	    status = dm2r_init_proj_cols(r, proj_col_map, dberr);
	}

	if ( status == E_DB_OK )
	{
	    status = dm2u_pload_table(ms, &rec_cnt, dberr);
	}
    }

    if ( status == E_DB_OK )
	for (k = 0, index_cb = index_cbs, m = ms; k < numberOfIndices;
	     k++, index_cb = (DM2U_INDEX_CB *)index_cb->q_next)
    {

	indxcb_dberr = index_cb->indxcb_errcb;

	tp = m->mx_tpcb_next;

	if ((tp->tpcb_newtup_cnt != rec_cnt) && !online_index_build)
	{   
	    TRdisplay("Finish index processing for %~t (%d,%d) tups %d, not %d\n",
		sizeof(*index_cb->indxcb_index_name),
		       *index_cb->indxcb_index_name,
		index_cb->indxcb_tbl_id->db_tab_base,
		index_cb->indxcb_idx_id->db_tab_index,
		tp->tpcb_newtup_cnt, rec_cnt);
	    TRdisplay("WARNING error detected in parallel index build\n");

	    status = E_DB_ERROR;
	    SETDBERR(indxcb_dberr, 0, E_DM9100_LOAD_TABLE_ERROR);
	}

        if (status != E_DB_OK) 
            continue;

	if (online_index_build)
	{
	    DM2U_M_CONTEXT	*mct=&tp->tpcb_mct;

	    /* update index relation from $online relation info */
	    m->mx_newtup_cnt = index_cb->indxcb_on_reltups;
	    tp->tpcb_newtup_cnt = index_cb->indxcb_on_reltups;
	    mct->mct_fhdr_pageno = index_cb->indxcb_on_relfhdr;
	    mct->mct_relpages = index_cb->indxcb_on_relpages;
	    mct->mct_prim = index_cb->indxcb_on_relprim;
	    mct->mct_main = index_cb->indxcb_on_relmain;
	}

        *index_cb->indxcb_tup_info = tp->tpcb_newtup_cnt;

	if ( TempTable )
	{
	    /* Update the Temp Index TCB iirelation */

	    DM2U_M_CONTEXT	*mct = &tp->tpcb_mct;
	    DMP_TCB		*TixTCB = m->mx_buildrcb->rcb_tcb_ptr;
	    DMP_RELATION	*rel = &TixTCB->tcb_rel;

	    rel->reltups = m->mx_newtup_cnt;
	    rel->relpages = mct->mct_relpages;
	    rel->relprim = mct->mct_prim;
	    rel->relmain = mct->mct_main;
	    rel->relfhdr = mct->mct_fhdr_pageno;
	    rel->relifill = mct->mct_i_fill;
	    rel->reldfill = mct->mct_d_fill;
	    rel->rellfill = mct->mct_l_fill;

	    /* Close the TempTable Index Table */
	    status = dm2t_close(m->mx_buildrcb, DM2T_NOPURGE, indxcb_dberr);
	    m->mx_buildrcb = (DMP_RCB*)NULL;
	    if ( status )
		break;
	}
	else
	{
	    /*
	    ** Opening file DI_USER_SYNC_MASK requires force prior to close.
	    */
	    if (db_sync_flag & DI_USER_SYNC_MASK)
	    {
		status = dm2f_force_file(tp->tpcb_locations, tp->tpcb_loc_count,
				  index_cb->indxcb_tab_name, 
				  &dcb->dcb_name,
				  indxcb_dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(indxcb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		    break;
		}
	    }

	    /*
	    ** Close the modify file(s) just loaded in preparation for the
	    ** renames below (in dm2u_update_catalogs).
	    */
	    modfile_open = FALSE;
	    status = dm2f_close_file(tp->tpcb_locations, tp->tpcb_loc_count, 	
			       (DM_OBJECT*)dcb, indxcb_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(indxcb_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
		break;
	    }

	    /*
	    ** Get relstat bits for index log record.  These will be used
	    ** at build time in ROLLFORWARD to determine the compression
	    ** state of the index rows.  Note that the UNIQUE index characteristic
	    ** is not logged here.  This causes us to bypass uniqueness checking
	    ** during the index build.
	    ** Bug 108330 - We must log TCB_UNIQUE to ensure the page is
	    ** recreated correctly. Index recovery routines now identify
	    ** themselves so that we can safely bypass uniqueness checking.
	    */
	    log_relstat = 0;
	    if (index_cb->indxcb_compressed != TCB_C_NONE)
		log_relstat |= TCB_COMPRESSED;
	    if (index_cb->indxcb_index_compressed)
		log_relstat |= TCB_INDEX_COMP;
	    if (index_cb->indxcb_unique)
		log_relstat |= TCB_UNIQUE;
	    /* Gateways not allowed here... */

	    /*
	    ** Log the index operation - unless logging is disabled.
	    **
	    ** The INDEX log record is written after the actual index
	    ** operation above which builds the temporary index load files.
	    ** Note that until now, the user table (and its metadata) has
	    ** not been physically altered.  We now log the index before
	    ** updating the catalogs and swapping the underlying files.
	    */

	    if (((xcb->xcb_flags & XCB_NOLOGGING) == 0) &&
		!online_index_build)
	    {
	      status = dm0l_index(xcb->xcb_log_id, journal_flag,
		index_cb->indxcb_tbl_id, index_cb->indxcb_idx_id, &table_name, 
		&owner, index_cb->indxcb_l_count, index_cb->indxcb_location, 
		index_cb->indxcb_structure, log_relstat, 
		index_cb->indxcb_relstat2,
		index_cb->indxcb_min_pages, index_cb->indxcb_max_pages,
		index_cb->indxcb_i_fill, index_cb->indxcb_d_fill, 
		index_cb->indxcb_l_fill, tp->tpcb_newtup_cnt, m->mx_buckets,
		index_cb->indxcb_idx_key, index_cb->indxcb_kcount,
		index_cb->indxcb_allocation, index_cb->indxcb_extend, 
		m->mx_page_type, m->mx_page_size, 
		index_cb->indxcb_compressed,
		index_cb->indxcb_index_compressed ? TCB_C_STD_OLD : TCB_C_NONE,
		tp->tpcb_name_id,
		tp->tpcb_name_gen,
		m->mx_dimension, m->mx_hilbertsize,(f8 *)&m->mx_range,  
		(LG_LSN *)0, &lsn, indxcb_dberr);
	      if (status != E_DB_OK)
		break;
	    }

	    /*  Change the system tables for the index operation. */
	    status = dm2u_update_catalogs(m, journal, indxcb_dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Log the DMU operation.  This marks a spot in the log file to
	    ** which we can only execute rollback recovery once.  If we now
	    ** issue update statements against the newly-indexed table, we
	    ** cannot do abort processing for those statements once we have
	    ** begun backing out the index.
	    */
	    if ((xcb->xcb_flags & XCB_NOLOGGING) == 0)
	    {
		status = dm0l_dmu(xcb->xcb_log_id, journal_flag, 
			 index_cb->indxcb_tbl_id,
			 &table_name, &owner, (i4)DM0LINDEX, (LG_LSN *)0, 
			 indxcb_dberr);
		if (status != E_DB_OK)
		    break;
	    }
	    /*
	    ** Release file control blocks allocated for the index.
	    ** Gateways not allowed here...
	    */
	    status = dm2f_release_fcb(xcb->xcb_lk_id, 
				    xcb->xcb_log_id,
				    tp->tpcb_locations, tp->tpcb_loc_count, 
				    DM2F_ALLOC, &local_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    }
	} /* not TempTable */

	if (status == E_DB_OK)
	{
	    /* save the next pointer before freeing the control block */
	    mxcb_cb = m->mx_next;
	    dm0m_deallocate((DM_OBJECT **)&m);
	    /* reset the control block */
	    m = mxcb_cb;
	}
    } 

    /*
    ** Close the table.
    */
    /*
    ** If a concurrent index do not purge the table at close
    ** time. The new index will be made visible once the base
    ** table is modifed back to NOREADONLY.
    */
    if ( concurrent_index || TempTable || !control_lk_held)
        local_status = dm2t_close(r, DM2T_NOPURGE, &local_dberr);
    else
        local_status = dm2t_close(r, DM2T_PURGE, &local_dberr);
    if (local_status != E_DB_OK)
    {
        uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                 (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
    }
    r = 0;

    if (status == E_DB_OK)
    {
	return (E_DB_OK);
    }

    /*
    ** Handle cleanup for error recovery.
    */

    /*
    ** The index_cb in which the error occurred will have
    ** its indxcb_errcb->err_code indicating the specific error.
    */

    /*
    ** If we left the newly created modify files open, then close them.
    ** We don't have to worry about deleting them as that is done by
    ** rollback processing.
    ** Loop through all indexes processed
    */
    for ( m = ms; m != (DM2U_MXCB *)0; m = nextm)
    {
	tp = m->mx_tpcb_next;

	/* Destroy all TempTable indexes that were created */
	if ( TempTable )
	{
	    if ( m->mx_buildrcb && m->mx_buildrcb != m->mx_rcb )
	    {
		/* Close the Temp index table */
		(VOID)dm2t_close(m->mx_buildrcb, DM2T_NOPURGE, &local_dberr);
		m->mx_buildrcb = (DMP_RCB*)NULL;
	    }

	    /* If the TempTcb was created, destroy it */
	    if ( m->mx_table_id.db_tab_base && m->mx_table_id.db_tab_index )
		(VOID)dm2t_destroy_temp_tcb(TableLockList,
					    dcb,
					    &m->mx_table_id,
					    &local_dberr);
	}
	else
	{
	    /*
	    ** If not last index processed, file is open for this index
	    ** If last index processed, modfile_open flag indicates if file is open
	    */
	    if (m->mx_next || modfile_open)
	    {
		status = dm2f_close_file(tp->tpcb_locations, tp->tpcb_loc_count,
			      (DM_OBJECT*)dcb, &local_dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
			 (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		}
	    }

	    /*
	    ** Release the FCB memory allocated by dm2f_build_fcb.
	    */
	    if (tp->tpcb_locations && tp->tpcb_locations[0].loc_fcb)
	    {
		status = dm2f_release_fcb(m->mx_lk_id, m->mx_log_id,
			 tp->tpcb_locations, tp->tpcb_loc_count, DM2F_ALLOC, &local_dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		}
	    }
	}

	/*
	** Signal the sorter if we stopped mid-sort and free up our index memory
	*/
	if (tp->tpcb_srt)
	{
	    status = dmse_end(tp->tpcb_srt, &local_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    }
	}
	tp->tpcb_srt = 0;
	nextm = m->mx_next;
	dm0m_deallocate((DM_OBJECT **)&m);
    }

    return (E_DB_ERROR);
}

