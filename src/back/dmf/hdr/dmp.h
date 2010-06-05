/*
** Copyright (c) 1985, 2007 Ingres Corporation
*/

#include    <qu.h>
#include    <dmrcb.h>
#include    <dmtcb.h>

/**
** Name:  DMP.H - Typedefs for the physical layer of DMF.
**
** Description:
**      This file contains all the structs, typedefs, and symbolic constants
**      used by the physical layer of DMF.  The physical layer is resposible
**      for the access method dependent operations required to read and
**      write records on database pages.  It includes all locking and logging
**      services needed for concurrency and recovery. 
**
** History:
**      01-sep-85 (jennifer)
**         Created for Jupiter.
**      16-nov_87 (jennifer)
**         Added multi-location table support.
**	23-sep-88 (sandyh)
**	   Changed tcb_chr_atts_count to tcb_comp_atts_count for C & CHAR
**	30-sep-1988 (rogerk)
**	    Added rcb_hash_nofull field to keep track of the free space
**	    status on a hash overflow chain as the chain is traversed.
**	21-jan-1989 (mikem)
**	    Added DCB_NOSYNC_MASK.
**	 6-feb-1989 (rogerk)
**	    Added dcb_bm_served field.  This describes whether the database
**	    can be served by one (DCB_SINGLE) or many (DCB_MULTIPLE) buffer
**	    managers.  Added tcb_status field to DMP_TCB. This is set to BUSY
**	    while the tcb is being built.
**	06-mar-89 (neil)
**          Rules: Added TCB_RULE (for table) and ATT_RULE (for column).
**	10-apr-89 (mikem)
**          Logical key development.  Added tcb_{high,low}_logical_key fields
**	    to the DMP_TCB.  Added ATT_WORM and ATT_SYS_MAINTAINED.
**	    Added tcb_{high,low}_logical_key fields to keep track of an 8 
**	    byte unique id.  Also added a new relstat status bit: 
**	    TCB_SYS_MAINTAINED.
**	 2-may-89 (anton)
**	    local collation support
**      20-jun-89 (jennifer)
**          Security development.  Added TCB_SECURE to relstat bit.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateways.
**	    Added new fields to TCB and RCB.
**	18-sep-89 (rogerk)
**	    Added dcb_lx_id to keep track of transaction for open database.
**	    This is added to avoid requiring an LGbegin call to close a
**	    database.
**	 6-dec-1989 (rogerk)
**	    Added rcb_bt_bi_logging field to indicate that btree leaf pages
**	    should use Before Image logging.
**	29-sep-89 (rogerk)
**	    Added DCB_S_BACKUP status to DCB control block to indicate that
**	    database is undergoing online backup.
**	10-jan-90 (rogerk)
**	    Added DCB_S_OFFLINE_CSP status to show that the CSP has the
**	    database open during startup recovery.  Using this and the
**	    DCB_S_CSP (which is set during node failure) indicates whether
**	    the database is open by the CSP.
**	23-jan-1990 (fred)
**	    Added large object support to TCB's and RCB's.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	29-apr-1991 (Derek)
**	    Added new DMP_RELATION fields and RCB performance counters.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-1990 (rogerk)
**		Added an ADF_CB to the Record Control Block.  This is now used
**		by DMF instead of allocated adf control blocks on the stack
**		when they are needed.  This makes it easier for DMF to keep
**		track of and set ADF variables associated with each session.
**		Added timezone value to the Database Control Block to make
**		it possible to attach timezone values to databases the same
**		way that collation sequences are handled.
**		This was done as part of the DBMS Timezone changes.
**	    17-dec-90 (jas)
**		Smart Disk project integration:
**		Add Smart Disk fields to RCB and TCB definitions.
**	     4-feb-1991 (rogerk)
**		Added support for fixed cache priorities.
**		Added tcb_bmpriority to show table's cache priority.
**	    12-feb-1991 (mikem)
**		Added two new fields rcb_val_logkey and rcb_logkey to DMP_RCB
**		structure to support returning logical key values to the client
**		which caused the insert of the tuple and thus caused them to be
**		assigned.
**	    25-mar-1991 (bryanp)
**		Added support for Btree Index Compression. Changes to the TCB:
**		    New field tcb_data_atts which points to list of pointers to
**		    attributes (i.e., it's equivalent to tcb_key_atts, but lists
**		    all data attributes) tcb_data_atts and tcb_key_atts are used
**		    when performing data or key compression, respectively.
**		    New relstat bit, TCB_INDEX_COMP, to indicate that table uses
**		    index compression.  New tcb_comp_katts_count field in TCB.
**	    23-jul-91 (markg)
**		Added new RCB_F_USER_ERROR return status for qualification
**		function in DMP_CB. (b38679)
**	    30-jan-1992 (bonobo)
**		Removed the redundant typedefs to quiesce the ANSI C 
**		compiler warnings.
**	18-feb-1992 (bryanp)
**	    Temporary table enhancements: Added TCB_PEND_DESTROY flag to TCB.
**	    Added fcb_mutex to DMP_FCB.
**	    Added FCB_DEFERRED_IO and FCB_MATERIALIZING flags to fcb_state.
**	08-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**      26-may-92 (schang)
**          GW merge
**	18-apr-1991 (rickh)
**	    Documented the curious coincidence of RCB_USER_INTR and 
**	    SCB_USER_INTR.
**	28-jul-92 (rickh)
**	    FIPS CONSTRAINTS improvements.
**	31-aug-92 (kwatts)
**	    Added a DMP_INDEX_SIZE because 'sizeof(DMP_INDEX)' word aligns
**	    on Sun/DRS6000
**    	29-August-1992 (rmuth)
**          Removing on-the-fly 6.4->6.5 tab;e conversion means we can
**          remove TCB_FHDR_VALID.
**	05-oct-92 (rogerk)
**	    Reduced Logging Project:
**	    Added support for partial TCB's.  Added TCB_PARTIAL tcb status
**	    to indicate that a TCB may not have all its underlying files
**	    open and may not contain sufficient logical information to
**	    allow logical table access.
**	    Added new tcb_valid_count field to keep count of the number of
**	    tcb fixes which verified the correctness of the tcb at fix time.
**	    Created above mentioned DMP_TABLE_IO structure and added support
**	    to DMP_LOCATION's and DMP_FCB's for partially open tables.
**	    Added tcb_table_io control block which contains information
**	    necessary for IO operations.  Moved existing tcb_loc_count,
**	    tcb_location_array, tcb_lalloc, tcb_lpageno, tcb_bmpriority,
**	    tcb_checkeof and tcb_valid_stamp into the table_io control block.
**	    Added new tcb_valid_count field to keep count of the number of
**	    tcb fixes which verified the correctness of the tcb at fix time.
**	22-oct-92 (rickh)
**	    Excised DMP_ATTS.  All references to this structure have been
**	    replaced with references to the identical structure DB_ATTS.
**	    Moved _DMP_COLUMN to DMF.H, where it's now called _DM_COLUMN.
**	    This makes this structure visible to GWF so that facility
**	    doesn't have to declare its own field by field equivalent.
**	26-oct-92 (rogerk & jnash)
**	    Reduced Logging Project:  Added loc_config_id to dmp_location.
**	    Added new TCB status value TCB_CLOSE_ERROR.  Added system
**	    catalog page type TCB_PG_SYSCAT.
**	30-October-1992 (rmuth)
**	    - Change tbio_lpageno and tbio_lalloc to i4's so that
**	      we can initialise them to -1 which is an invalid pageno.
**	    - Add tbio_tmp_hw_pageno to track the the highest written
**	      pageno for a temp table to DMP_TABLE_IO.
**	05-nov-92 (jnash)
**	    Temp hack of recovery bitfield to bypass tid locking 
**	    until new recovery code integrated.
**	04-nov-92 (jrb)
**	    Changed DCB_SORT to DCB_WORK.
**	18-nov-92 (jnash)
**	    Reduced Logging Project (phase 2):
**	      - Add LOC_UNAVAIL to reflect locations that are not recovered.
**	      - Took out temporary use of rcb_recovery field.
**	      - Add rcb_usertab_jnl for system catalog rcbs to indicate the
**		journal status of the usertable being accessed.
**	05-dec-1992 (kwatts)
**	    Smart Disk enhancement, rcb_sdtsbbuffer becomes a char.
**	18-jan-1993 (bryanp)
**	    Added TCB_TMPMOD_WAIT to fix temporary table modify bug.
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project (pase 3):  Additions to dcb status field.
**	16-feb-1993 (rogerk)
**	    Added tcb_open_count to count the number of open RCB's on the table.
**	15-mar-1993 (jnash)
**	    Reassign value of EXT_CB from 20 to 27 to eliminate duplicate.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed the rcb_abt_sp_addr field used when BI's were logged
**		for undo recovery.
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Add rcb_state flag RCB_IN_MINI_TRAN to allow tracking when a
**		    particular RCB is in the process of performing a mini-
**		    transaction.
**		Added dcb_backup_lsn so that we can track the Log Sequence
**		    Number of the start of the backup.
**		Remove the DCB_S_CSP status flag for a database, since we no
**		    longer need to know when a database is being processed by
**		    the CSP.
**		Added a dcb_add_count to track number of users who think they
**		    have added this database.
**	15-may-1993 (rmuth)
**	    Add relstat2 bits for the following:
**		- to indicate that a table is READONLY
**		- If table is an index to indicate that it is being built 
**		  in "CONCURRENT_ACCESS" mode.
**	   
**	26-jul-1993 (bryanp)
**	    The dcb_lx_id field is no longer needed, since we no longer log a
**		DM0L_CLOSEDB log record when closing a database.
**	    Replace all uses of DM_LKID with LK_LKID.
**	    Also, just a comment: sprinkled throughout this file are various
**		comments which attempt to describe, for each major control
**		block, the offset in bytes, in decimal, of that field in that
**		control block. While making the DM0L_CLOSEDB log record changes
**		I noticed that these fields have not been kept up to date; in
**		particular, the DB_MAXNAME change from 24-32 bytes in length
**		is not reflected in these comments. SO DO NOT TRUST THESE OFFSET
**		COMMENTS! Probably, we should just delete the comments, since
**		they're very hard to keep up to date.
**	23-aug-1993 (rogerk)
**	    Renamed DCB_S_RCP to DCB_S_ONLINE_RCP as it is set only when doing
**	    online recovery processing.
**	    Added definition for DCB_S_RECOVER.
**      18-oct-93 (jrb)
**          Added dcb_wrkspill_idx to DCB; MLSorts project.
**	18-oct-1993 (rogerk)
**	    Removed PEND_DESTROY tcb state.
**	18-oct-93 (swm)
**	    Added tcb_unique_id after standard DMF header, for use as a lock
**	    event value. This value is used instead of a DMP_TCB pointer
**	    because the size of a pointer can be bigger than the size of a
**	    u_i4 on some platforms.
**	    This field is currently unused, it has been added as a hook for
**	    my next integration.
**      19-oct-93 (tomm)
**	    remove erroneous comment fragment which caused syntax errors.
**	19-oct-93 (swm)
**	    Moved TCB_ASCII_ID define to immediately follow tcb_ascii_id
**	    field to which it relates, rather than tcb_unique_id.
**      27-sep-1993 (smc)
**          Removed byte offset comments as these were non-portable.
**	26-oct-93 (swm)
**	    Bug #56440
**	    Recently added tcb_unique_id field now in use.
**	20-jun-1994 (jnash)
**	    fsync project:
**	     - Add tbio_sync_close, used when building tcb to request async 
**	       writes and table force at dm2t_close.
**	     - Add DCB_S_SYNC_CLOSE dcb_status.  Database level sync-at-
**	       close, currently used only by rollforwarddb.  Generates
**	       table level sync-at-close open requests.
**           - Eliminate obsolete tcb_logical_log.
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Added New bitflags to relstat2:
**                  TCB2_PHYS_INCONSISTENT          
**                  TCB2_LOGICAL_INCONSISTENT       
**                  TCB2_TBL_RECOVERY_DISALLOWED   
**      7-dec-1994 (andyw)
**	    Added comment for changes made by Steve Wonderly and 
**	    Roger Sheets
**		added TCB_C_HICOMPRESS  to the DMP_RELATION structure
**		to handle new compression algorithms.
**	19-jan-1995 (cohmi01)
**	    Add DMP_RAWEXT struct for raw extents. Define various new bits.
**      04-apr-1995 (dilma04)
**          Add support for SET TRANSACTION ISOLATION LEVEL.
**	10-may-1995 (cohmi01)
**	    Add items for prefetch, readahead thread.
**	26-may-1995 (wonst02)
**	    Add tcb_dimension, tcb_hilbertsize
**	07-jun-1995 (cohmi01)
**	    Update prefetch structs for multiple threads/events
**	17-jul-1995 (cohmi01)
**	    Add RCB_RAHEAD_INTR, add interrupt vector to DMP_PFREQ.
**      23-jan-1996 (angusm)
**          add TBIO_CACHE_NO_GROUPREAD, to signal no group read
**          on internally created temp tables spread over multiple
**          locations, since extent cannot be guaranteed (bug 72806).
**	06-mar-1996 (stial01 for bryanp)
**	    Add relpgsize to iirelation; reduce relfree accordingly.
**	    Added DMP_NUM_IIRELATION_ATTS to dimension the dm2umod.c array.
**	    Add tbio_page_size to the DMP_TABLE_IO structure.
**	29-apr-1996 (wonst02)
**	    Add new op_types for R-tree access: OVERLAPS, INTERSECTS, INSIDE,
**	    and CONTAINS.
**	05-may-1996 (wonst02)
**	    Add new RCB fields for Rtree.
**	15-may-96 (stephenb)
**	    add DMP_DD_REGIST_TABLES, DMP_REP_SHAD_INFO and DMP SHAD_REC_QUE
**	    structures to support DBMS replication, also add fields to
**	    DCB and TCB for replication support
**	26-jul-1996 (sarjo01)
**	    For bug 77690: Add ETL_INVALID_MASK for etl_status in
**	    DMP_ET_LIST
**      01-apr-1996 (ramra01)
**	    Added relversion to iirelation; reduce relfree accordingly.
**          Added DMP_NUM_IIRELATION_ATTS to dimension the dm2umod.c array.
**      22-jul-1996 (ramra01 for bryanp)
**          Add relversion, reltotwid  to iirelation; 
**	    reduce relfree accordingly.
**          Define TCB2_ALTERED to track when a table is altered.
**	21-aug-96 (stephenb)
**	   Add replicator cdds lookup info to RCB.
**	23-aug-96 (nick)
**	    Added TCB_INCONS_IDX to DMP_TCB. #77429
**	27-feb-1997 (cohmi01)
**	    Add items to limit number of base TCBs allocated. Bug 80242.
**	06-sep-1996 (canor01)
**	    Add rcp_tupbuf to DMP_RCB for compression.  Allocate buffer once.
**	24-Oct-1996 (jenjo02)
**	    Removed hcb_mutex, which blocked all hash buckets, replacing it 
**	    with a semaphore per hash bucket (hash_mutex). 
**	    Added hcb_tq_mutex to protect the TCB free queue.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add RCB_K_ROW value to indicate row level locking granularity;
**          Add fields to RCB for lsn,clean_count for CURRENT/STARTING position
**          Add some fields to RCB for rcb_update for replace as delete/put
**          Add row_locking macro to check if row locking is enabled.
**      12-dec-96 (dilma04)
**          Remove unneeded check for rcb_lk_mode value from row_locking macro.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          - add rcb_iso_level field to DMP_RCB;
**          - add serializable() macro. 
**	24-Feb-1997 (jenjo02 for somsa01)
**	    Cross-integrated 428939 from main.
**	    Added tbio_extended, which is set if the table is an extended
**	    table.
**	13-mar-97 (stephenb)
**	    Define DCB_S_REPCATS to indicate that replicator catalogs have 
**	    been created.
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added reltcpri (Table Cache priority), reduced relfree,
**	    bumped DMP_NUM_IIRELATION_ATTS.
**      21-apr-97 (stial01)
**          Added rcb_alloc_bid, rcb_alloc_lsn, rcb_alloc_clean_count,
**          and rcb_alloc_tidp to the RCB.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Added new fields and flags to DMP_RCB to support lock release
**          protocols for CS and RR isolation levels.
**      29-jul-97 (stial01)
**          Added set_row_locking macro.
**          DMP_RCB: Removed temporary fields no longer needed for rcb update
**	25-Aug-1997 (jenjo02)
**	    Added fcb_log_id to DMP_FCB.
**      29-Sep-1997 (stial01)
**          B86295: Added RCB_P_QUAL_NEXT
**      12-nov-97 (stial01)
**          Consolidated rcb fields into rcb_csrr_flags.
**          Added rcb fields to rcb_locked_data, rcb_locked_leaf.
**      21-Apr-1998 (stial01)
**          Added tcb_extended for B90677
**	07-May-1998 (jenjo02)
**	    Added tbio_cache_ix to TBIO.
**	    Added hcb_cache_tcbcount to HCB.
**	23-jul-1998 (nanpr01)
**	    Added flag to position at the end of qualification range.
**	28-jul-1998 (somsa01)
**	    In DMP_ET_LIST, added ETL_LOAD to tell us that there is a load
**	    operation being performed on this extension table, and also
**	    variables to keep track of the extension table's dmrcb and
**	    dmtcb (in the case of a bulk load operation).  (Bug #92217)
**	11-Aug-1998 (jenjo02)
**	    Added rcb_tup_adds, rcb_page_adds, rcb_reltups, rcb_relpages.
**	19-Aug-1998 (jenjo02)
**	    Added rcb_fix_lkid.
**	23-oct-1998 (somsa01)
**	    In DMP_TCB, added TCB_SESSION_TEMP as an option to relstat.
**	03-Dec-1998 (jenjo02)
**	    Added fcb_rawpgsize (table's page size in raw extent) to FCB.
**      07-Dec-1998 (stial01)
**          Added tcb_kperleaf to distinguish from tcb_kperpage
**	19-feb-1999 (nanpr01)
**	    Support update mode locks.
**	21-mar-1999 (nanpr01)
**	    Support raw locations.
**      12-apr-1999 (stial01)
**          Different att/key info in DMP_TCB for (btree) LEAF vs. INDEX pages
**	16-Apr-1999 (jenjo02
**	    Removed old RAW structures, replaced fcb_rawpgsize with
**	    fcb_rawpages.
**	10-may-1999 (walro03)
**	    Remove obsolete version string i39_vme.
**	29-Jun-1999 (jenjo02)
**	    Added DMP_CLOSURE structure for GatherWrite.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint. 
**      31-aug-1999 (stial01)
**          Added tcb_et_dmpe_len
**      20-oct-1999 (nanpr01)
**          Added tcb_tperpage for tuple per page value storage for
**	    read-ahead override calculation.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      05-dec-1999 (nanpr01)
**          Added the rcb_oldest_lsn field to compare with the page lsn
**	    to check if the page contains uncommited tuples.
**      21-dec-1999 (stial01)
**          Added TCB_PG_V3
**      10-jan-2000 (stial01)
**          Added RCB_NO_CPN
**	10-Jan-2000 (jenjo02)
**	    Replaced direct reference to obsolete bm_fcount in
**	    dm0p_buffers_full() macro.
**      12-jan-2000 (stial01)
**          Added TCB2_BSWAP (Sir 99987)
**      27-jan-2000 (stial01)
**          Renamed rcb btree reposition fields (rcb_repos*)
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026) 
**	21-oct-1999 (hayke02)
**	    17-Aug-1998 (wanya01)
**	     Add DCB_SKIP to DMP_LOC_ENTRY.
**      06-Mar-2000 (hanal04) Bug 100742 INGSRV 1122.
**          Added DCB_S_MLOCKED as a new dcb_status value.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**      01-feb-2001 (stial01)
**          Added new defines,macros for Variable Page Type SIR 102955
**          Also, new fields in rcb for deferred update processing 
**	09-Feb-2001 (jenjo02)
**	    Added RCB_RECORD_PTR_VALID flag to rcb_state.
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      01-may-2001 (stial01)
**          Added rdf_ucollation
**      10-may-2001 (stial01)
**          Added rdf_*_nextleaf fields to optimize btree reposition
**      08-Oct-2002 (hanch04)
**          Replace the i4 for length with a SIZE_TYPE (size_t) to allow
**          64-bit memory allocation.
**      16-sep-2003 (stial01)
**          Added rcb_galloc*, DMP_LO_HEAP (SIR 110923)
**	18-Dec-2003 (schka24)
**	    Added partitioned-table stuff to iirelation.
**      02-jan-2004 (stial01)
**          Removed temporary rcb_galloc fields, Added new rcb fields for 
**          blob bulk put. Added new field to DMP_ET_LIST to for blob etab
**          journaling status, which can now be different from base table.
**	14-Jan-2004 (schka24)
**	    Add pp array to tcb, add might-be-updating lists to rcb.
**	28-Jan-2004 (fanra01)
**	    Add include of qu.h for definition of QUEUE now required.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          iirelation changes for 256k rows, relwid, reltotwid i2->i4
**          iiattribute changes for 256k rows, attoff i2->i4
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**      8-jul-2004 (stial01)
**          Added rcb_3rnl_page
**	30-aug-2004 (thaju02)
**	    Added #define RNL_READ_ASSOC for online modify.
**	14-Feb-2005 (gorvi01)
**		Modified HCB_MUTEX_HASH_MACRO with braces for correct sequence
**		of operations in the macro to give OR operation higher 
**		priority. Change made for 64 bit windows porting. 
**      30-jun-2005 (horda03) Bug 114498/INGSRV3299
**          Added tcb status TCB_BEING_RELEASED to signify that the
**          TCB is being reclaimed for used for another table. This
**          TCB should not be "located".
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	    Define DCB_CB as DM_DCB_CB
**	  	   EXT_CB as DM_EXT_CB
**	  	   FCB_CB as DM_FCB_CB
**	  	   HCB_CB as DM_HCB_CB
**	  	   RCB_CB as DM_RCB_CB
**	  	   TBIO_CB as DM_TBIO_CB
**	  	   TCB_CB as DM_TCB_CB
**	  	   REP_CB as DM_REP_CB
**	  	   ETL_CB as DM_ETL_CB
**	  	   PFH_CB as DM_PFH_CB
**      15-sep-2006 (stial01)
**          create (non_unique) index with 32 keys overflows tcb_ikey_map
**          because ingres adds tidp to this key (b116673).
**      28-sep-2006 (stial01)
**          Added tcb_lrng* fields to support old/new btree range entry formats.
**	03-jul-2007 (joea)
**	    Add FCB_STATES string #define.
**	19-Feb-2008 (kschendel) SIR 122739
**	    Add DMP_ROWACCESS structure to bundle up stuff needed for
**	    row access (att array, row decompression).  We need it here
**	    so that it can be included in the TCB.
**      24-nov-2009 (stial01)
**          Added tcb_seg_rows
**      11-jan-2010 (stial01)
**          Consolidate rcb btree reposition information into a structure
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: DMP_PINFO structure added.
**	    Add crow_locking, RootPageIsInconsistent, MVCC_ENABLED macros.
**	10-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Modified RootPageIsInconsistent to return FALSE
**	    when in a constraint; constraints are always run on Root pages.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Add NeedPhysLock macro.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      10-Feb-2010 (maspa05) bug 122651, trac 442
**          Added TCB2_PHYSLOCK_CONCUR - Table uses physical locks but is not
**          a TCB_CONCUR table. Implies that the table must remain hash 
**          (otherwise recovery could fail) and this is enforced in dm2u_modify
**          Note that dmt_set_lock_values uses the table id instead of relstat2 
**          since relstat2 is not available at the time. 
**          If you add a new table with this flag, or add this flag to an 
**          existing table, YOU MUST add equivalent checks in 
**          dmt_set_lock_values also. You'll also need to update/add to 
**          dub_mandflags in dubdata.c which is the list of tables with 
**          mandatory flags
*/

/*
**  This section includes all forward references required for defining
**  the DMP structures.
*/

typedef struct _DMP_DEVICE	    	DMP_DEVICE;
typedef struct _DMP_LOCATION	    	DMP_LOCATION;
typedef struct _DM_COLUMN	    	DMP_ATTRIBUTE;
typedef struct _DMP_HASH_ENTRY	    	DMP_HASH_ENTRY;
typedef struct _DMP_INDEX	    	DMP_INDEX;
typedef struct _DMP_LOC_ENTRY	    	DMP_LOC_ENTRY;
typedef struct _DMP_RELATION	    	DMP_RELATION;
typedef	struct _DMP_ET_LIST	    	DMP_ET_LIST;
typedef	struct _DMP_TABLE_IO	    	DMP_TABLE_IO;
typedef struct _DMP_RANGE	    	DMP_RANGE;
typedef struct _DMP_REP_INFO	    	DMP_REP_INFO;
typedef struct _DMP_DD_REGIST_TABLES	DMP_DD_REGIST_TABLES;
typedef struct _DMP_REP_SHAD_INFO	DMP_REP_SHAD_INFO;
typedef struct _DMP_SHAD_REC_QUE	DMP_SHAD_REC_QUE;
typedef struct _DMP_CLOSURE    		DMP_CLOSURE;
typedef struct _DMP_RNL_LSN		DMP_RNL_LSN;
typedef struct _DMP_RNL_ONLINE		DMP_RNL_ONLINE;
typedef struct _DMP_POS_INFO		DMP_POS_INFO;
typedef struct _DMP_PINFO		DMP_PINFO;

/*
**  DCB - SECTION
**
**       This section defines all structures needed to build
**       and access the information in a Database Control Block (DCB).
**       The following structs are defined:
**           
**           DMP_DCB                 Database Control Block.
*/

/*}
** Name:  DMP_DCB - DMF internal control block for static database information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about databases that is independent of the opener.
**
**      Most of the information in the DCB comes from runtime information
**      provided on the call to DMF to add a database to the server and
**      from the database configuration file.  This control block varies in 
**      size for each database since any number of locations can be added 
**      by the owner.  The DCB includes such information as the pointers
**      to the TCB's for the primary system tables (i.e. RELATION, ATTRIBUTE,
**      and INDEX), the name and identifier of the log file associated with 
**      this database, a valid stamp used to determine if this copy of 
**      the DCB is still valid in between opens of the database, the primary
**      and secondary locations (physical disk locations) used to store
**      tables for this database, the owner of the database, the database name,
**      and a reference count.
**
**	Notes on DCB status values:
**
**	    DCB_S_JOURNAL:	Database is journaled.  All updates require
**				logical log records to be written which will
**				then be moved to the database journal files
**				by the archiver.
**	    DCB_S_EXCLUSIVE:	Database is opened and locked exclusively by
**				a single session in this server.
**	    DCB_S_ROLLFORWARD:	This DCB was built by the rollforward process.
**	    DCB_S_ONLINE_RCP:	This DCB was built by the recovery process.
**	    DCB_S_RECOVER:	This DCB was built by the RCP or CSP for
**				recovery processing.
**	    DCB_S_FASTCOMMIT:	Database is open with fast commit protocols.
**				All access to this database must go through
**				a common buffer manager.  All updates require
**				logical log records to be written which can
**				be used for REDO recovery.  Requires server to
**				have a fast Commit Thread which flushes dirty
**				pages as part of Consistency Point protocols.
**          DCB_S_DMCM:         Database is open with Distributed Multi-Cache
**                              Management (DMCM) protocol. This is
**                              effectively FastCommit in a Multi-Cache
**                              environment.
**	    DCB_S_INVALID:	DCB is corrupted and cannot be used.  This
**				database will not be served by this server
**				anymore.  The server should be shut down and
**				a new one started.  This error only occurs when
**				the server fails trying to clean up and close
**				a database.
**	    DCB_S_BACKUP:	Database is undergoing online backup. While
**				backup is going on, all updates to the database
**				must be logged with physical log records - this
**				includes system catalog and btree index updates.
**	    DCB_S_SYNC_CLOSE:	Database files are opened sync-at-close.
**				Currently used only by rollforwarddb, this
**				uses Unix fsync() function to bypass O_SYNC 
**				writes and force file via fsync() at close.
**          DCB_S_PRODUCTION    Database is operating in production mode. All
**                              operations that attempt to write to system
**                              catalogs are not allowed
**
**      DCBs are queued to the server control block(SVCB) and are pointed to 
**      by table control blocks(TCBs) and open database control blocks (ODCBs)
**      associated with this database.  A pictorial representation of DCBs in 
**      an active server enviroment is:
**   
**           SVCB      
**       +-----------+ 
**       |           |                                    DCB
**       |           |                       +----+--->+--------+
**       |  DCB  Q   |-----------------------|----|--->| DCB Q  |
**       |           |                       |    |    |        |
**   +---|  SCB  Q   |                       |    |    +--------+  
**   |   +-----------+                       |    |
**   |      SCB                              |    |        TCB    
**   |   +--------+                          |    |     +--------+
**   +-->| SCB Q  |                          |    |     |        |
**       |        |                          |    +-----|DCB PTR |
**       | ODCB Q |--+                       |          +--------+  
**       |        |  |                       |  
**       +--------+  |           ODCB        |
**                   |         +---------+   |
**                   +-------->| ODCB Q  |   |
**                             |         |   |
**                             | DCB PTR |---+
**                             +---------+               
**

**     The DCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | dcb_q_next                       |  Initial bytes are
**                     | dcb_q_prev                       |  standard DMF CB
**                     | dcb_length                       |  header. 
**                     | dcb_type   | dcb_s_reserved      |
**                     | dcb_l_reserved                   |
**                     | dcb_owner                        |
**                     | dcb_ascii_id                     |
**                     |------ End of Header -------------|
**                     | dcb_status                       |
**                     | dcb_sync_flag                    |
**                     | dcb_access_mode                  |
**		       | dcb_served                       |
**		       | dcb_bm_served                    |
**                     | dcb_db_type                      |
**		       | dcb_ref_count                    |
**                     | dcb_opn_count                    |
**                     | dcb_lock_id                      |
**                     | dcb_tti_lock_id                  |
**                     | dcb_odb_lock_id                  |
**                     | dcb_valid_stamp                  |
**                     | dcb_rel_tcb_ptr                  |
**                     | dcb_att_tcb_ptr                  |
**                     | dcb_idx_tcb_ptr                  |
**                     | dcb_nam_id                       |
**                     | dcb_own_id                       |
**                     | dcb_root                         |
**                     | dcb_jnl                          |
**                     | dcb_ckp                          |
**                     | dcb_ext                          |
**                     | dcb_id                           |
**                     | dcb_log_id                       |
**                     | dcb_lx_id                        |
**                     | dcb_name                         |
**                     | dcb_db_owner                     |
**		       | dcb_collation			  |
**		       | dcb_colname			  |
**                     | dcb_mutex			  |
**                     | dcb_tzcb			  |
**                     |-------End of Fixed Portion ------|
**                     | Area containing location array.  |
**                     | Each entry contains the logical  |
**                     | location (12 bytes) a flag field |
**		       | of 4 bytes, and the              |
**                     | physical location (64 bytes).    |
**                     +----------------------------------+
**                     
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	 9-dec-1988 (rogerk)
**	    Added DCB_S_INVALID status.  This specifies that the database
**	    control block cannot be used.  It is not built correctly and
**	    cannot be cleaned up properly.
**	21-jan-1989 (mikem)
**	    Added DCB_NOSYNC_MASK.
**	24-jan-1989 (Edhsu)
**	    Added dcb_dmp to support online backup.
**	 6-feb-1989 (rogerk)
**	    Added dcb_bm_served field.  This describes whether the database
**	    can be served by one (DCB_SINGLE) or many (DCB_MULTIPLE) buffer
**	    managers.
**	 2-may-1989 (anton)
**	    local collation support (add collation field)
**	18-sep-89 (rogerk)
**	    Added dcb_lx_id to keep track of transaction for open database.
**	    This is added to avoid requiring an LGbegin call to close a
**	    database.
**	29-sep-89 (rogerk)
**	    Added DCB_S_BACKUP status which indicates that the database is
**	    undergoing online backup.  This status is used by the server to
**	    determine if special logging is necessary.
**	10-jan-90 (rogerk)
**	    Added DCB_S_OFFLINE_CSP status to show that the CSP has the
**	    database open during startup recovery.  Using this and the
**	    DCB_S_CSP (which is set during node failure) indicates whether
**	    the database is open by the CSP.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-90 (rogerk)
**		Added dcb_timezone field for future handling of timezones.
**		The field is not currently used, but could be used to record
**		the timezone in which the database resides.
**		This was done as part of the DBMS Timezone changes.
**      24-sep-92 (stevet)
**          Changed scb_timezone of DMP_DCB to scb_tzcb, which is a pointer 
**          the timezone structure TM_TZ_CB.
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project: removed old OFFLINE_CSP status.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Added dcb_backup_lsn so that we can track the Log Sequence
**		    Number of the start of the backup.
**		Remove the DCB_S_CSP status flag for a database, since we no
**		    longer need to know when a database is being processed by
**		    the CSP.
**		Added a dcb_add_count to track number of users who think they
**		    have added this database.
**	24-may-1993 (robf)
**	    Secure 2.0: Add dcb_last_secid field to track last security
**	    id allocated for this database.
**	26-jul-1993 (bryanp)
**	    The dcb_lx_id field is no longer needed, since we no longer log a
**		DM0L_CLOSEDB log record when closing a database.
**	    Replace all uses of DM_LKID with LK_LKID.
**	23-aug-1993 (rogerk)
**	    Renamed DCB_S_RCP to DCB_S_ONLINE_RCP as it is set only when doing
**	    online recovery processing.
**	    Added definition for DCB_S_RECOVER.
**      18-oct-93 (jrb)
**          Added dcb_wrkspill_idx to indicate which location is to be used 
**          next for this db;  this field is typically updated without
**          semaphoring.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**	11-apr-96 (stephenb)
**	    Add DCB_S_REPLICATE flag value and rep_* fields to support
**	    DBMS replication.
**     01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced dcb_status of DCB_S_DMCM to indicate that the database
**          is opened with the Distributed Multi-Cache Management (DMCM)
**          protocol.
**	20-jun-97 (stephenb)
**	    add field dcb_bad_cdds to record non-replicating CDDSs
**	21-aug-1997 (nanpr01)
**	    Store the version info for FIPS upgrade.
**	21-mar-1999 (nanpr01)
**	    Support raw locations.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint. dcb_eback_lsn keeps the last lsn from
**	    the online checkpoint. 
**      06-Mar-2000 (hanal04) Bug 100742 INGSRV 1122.
**          Added DCB_S_MLOCKED as a new dcb_status value. This is an optional
**          flag used to indicate that the DCB mutex is held. If set it MUST
**          be unset immediately prior to releasing the DCB mutex.
**	16-mar-2001 (stephenb)
**	    Add unicode collation table, dcb_ucollation.
**	05-Mar-2002 (jenjo02)
**	    Added dcb_seq, list of opened Sequence Generators.
**	14-Jan-2004 (schka24)
**	    Add list of open partition master TCB's.
**      07-Jun-2005 (inifa01) b111211 INGSRV2580
**          Verifydb reports error; S_DU1619_NO_VIEW iirelation indicates that
**          there is a view defined for table a (owner ingres), but none exists,
**          when view(s) on a table are dropped.
**          Were making DMT_ALTER call on DMT_C_VIEW, to turn on iirelation.relstat
**          TCB_VBASE bit, indicating view is defined over table(s).  TCB_VBASE
**          not tested anywhere in 2.6 and not turned off when view is dropped.
**          qeu_cview() call to dmt_alter() to set bit has been removed,
**          removing references to TCB_VBASE here.
**	09-Oct-2007 (jonj)
**	    Add DCB_STATUS, DCB_LOC_FLAGS defines.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Added DCB_S_MUST_LOG to flag DBs that this DBMS must log all
**          activity. SET NOLOGGING is blocked.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Add has-raw-location convenience flag to status.
**	12-Nov-2009 (kschendel) SIR 122882
**	    dbcmptlvl is now an integer.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DCB_S_MVCC, DCB_S_MVCC_DISABLED
**	    DCB_S_MVCC_TRACE, DCB_S_MVCC_JTRACE
*/

struct _DMP_DCB
{
    DMP_DCB         *dcb_q_next;            /* Queue of DCBs off SVCB. */
    DMP_DCB         *dcb_q_prev;
    SIZE_TYPE	    dcb_length;		    /* Length of the control block. */
    i2              dcb_type;               /* Type of control block for
                                            ** memory manager. */
#define                 DCB_CB              DM_DCB_CB
    i2              dcb_s_reserved;         /* Reserved for future use. */
    PTR             dcb_l_reserved;         /* Reserved for future use. */
    PTR             dcb_owner;              /* Owner of control block for
                                            ** memory manager.  DCB will be
                                            ** owned by the server. */
    i4         dcb_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 DCB_ASCII_ID        CV_C_CONST_MACRO('#', 'D', 'C', 'B')
    i4	    dcb_status;		    /* DCB status bits. See above for
					    ** detailed explanations. */
#define			DCB_S_JOURNAL	    0x01L   /* Database is journaled */
#define			DCB_S_EXCLUSIVE	    0x02L
#define			DCB_S_ROLLFORWARD   0x04L
#define			DCB_S_ONLINE_RCP    0x08L
#define			DCB_S_RECOVER	    0x10L
#define			DCB_S_FASTCOMMIT    0x20L   /* Fast Commit in use */
#define			DCB_S_INVALID	    0x40L   /* DCB is corrupted */
#define			DCB_S_BACKUP	    0x80L   /* Database is undergoing
						    ** online backup */
#define			DCB_S_SYNC_CLOSE    0x100L  /* Database is opened
						    ** without file sync'ing.
						    ** Sync must be done 
						    ** during closedb. */
#define			DCB_S_PRODUCTION    0x200L  /* Database is in
						    ** production mode */
#define			DCB_S_NOBACKUP      0x400L  /* Online checkpoint
						    ** is disabled */
#define			DCB_S_REPLICATE	    0x800L  /* Database is replicated */
#define                 DCB_S_DMCM          0x1000L  /* DMCM in use */
#define			DCB_S_REPCATS	    0x2000L /* replicator catalogs 
						    ** exist */
#define 		DCB_S_MLOCKED       0x4000L /* dcb_mutex held if true.
                                                    */
#define 		DCB_S_MVCC  	    0x8000L /* DB is eligible for MVCC
  						    ** protocols */
#define 		DCB_S_MVCC_DISABLED 0x10000L/* MVCC disabled by alterdb */
#define 		DCB_S_MVCC_TRACE    0x20000L /* Trace all MVCC CR activity */
#define 		DCB_S_MVCC_JTRACE   0x40000L /* Only Trace MVCC CR 
						     ** jnl activity */
#define                 DCB_S_MUST_LOG      0x80000L /* All TXs must be logged
                                                    ** for this DB for the given
                                                    ** server class
                                                    */
#define			DCB_S_HAS_RAW	    0x100000L /* DB has a raw location,
						    ** set at DB open time,
						    ** not at ADD time. */
/* String values of above bits */
#define	DCB_STATUS	"\
JOURNAL,EXCLUSIVE,ROLLFORWARD,ONLINE_RCP,RECOVER,FASTCOMMIT,INVALID,\
BACKUP,SYNC,PRODUCTION,NOBACKUP,REPLICATE,DMCM,REPCATS,MLOCKED,\
MVCC,MVCC_DISABLED,MVCC_TRACE,MVCC_JTRACE,MUST_LOG,HAS_RAW"

    LG_LA	    dcb_backup_addr;	    /* On-line backup starting log
					    ** address. */
    LG_LSN	    dcb_backup_lsn;	    /* On-line backup starting Log
					    ** Sequence Number */
    LG_LSN	    dcb_ebackup_lsn;	    /* On-line backup ending Log
					    ** Sequence Number */
    i4         dcb_sync_flag;
#define			DCB_NOSYNC_MASK	    0x0001  /* don't sync if set */
           
    i4	    dcb_access_mode;        /* Type of access allowed to 
                                            ** database. */
#define                 DCB_A_READ          1L
#define                 DCB_A_WRITE         2L
    i4	    dcb_served;		    /* Server access mode. */
#define			DCB_MULTIPLE	    1L
#define			DCB_SINGLE	    2L
    i4	    dcb_bm_served;	    /* Cache access mode. */
    i4	    dcb_db_type;	    /* Type of database. */
#define                 DCB_PRIVATE         1L
#define                 DCB_PUBLIC          2L
#define                 DCB_DISTRIBUTED     3L
    short	    dcb_ref_count;          /* Count of the number of active
                                            ** users of this database. */
    short	    dcb_opn_count;	    /* Count of users attempting to
					    ** open this database. */
    i4		    dcb_add_count;	    /* Count of users adding this 
					    ** database. */
    i4	    dcb_lock_id[2];	    /* Server database lock id. */
    i4	    dcb_tti_lock_id[2];	    /* Temporary table lock id. */
    i4	    dcb_odb_lock_id[2];	    /* Open Table lock id. */
    i4         dcb_valid_stamp;        /* Value used to determine if DCB
                                            ** is valid when not in use. */
    DMP_TCB        *dcb_rel_tcb_ptr;        /* Pointer to the TCB of the
                                            ** RELATION system table.  */
    DMP_TCB        *dcb_att_tcb_ptr;        /* Pointer to the TCB of the
                                            ** ATTRIBUTE system table.  */
    DMP_TCB        *dcb_idx_tcb_ptr;        /* Pointer to the TCB of the
                                            ** INDEX system table.  */
    DMP_TCB	   *dcb_relidx_tcb_ptr;	    /* pointer to the TCB of the
					    ** IIREL_IDX system table */
    LK_LKID	    dcb_nam_id;		    /* Name lock identifier. */
    LK_LKID	    dcb_own_id;		    /* Owner lock identifier. */
    DMP_LOC_ENTRY   *dcb_root;		    /* Pointer to root location. */
    DMP_LOC_ENTRY   *dcb_jnl;		    /* Pointer to journal location. */
    DMP_LOC_ENTRY   *dcb_dmp;
    DMP_LOC_ENTRY   *dcb_ckp;		    /* Pointer to checkpoint loc. */
    DMP_EXT	    *dcb_ext;		    /* Pointer to extent block.*/
    i4         dcb_id;                 /* Identifier for database. */
    i4         dcb_log_id;             /* Log identifier for this db. */
    i4		dcb_jnl_block_size;	/* For online modify sidefile */
    DB_DB_NAME      dcb_name;               /* Name of database. */
    DB_OWN_NAME     dcb_db_owner;           /* Owner of database. */
    PTR		    dcb_collation;	    /* collation descriptor */
    PTR		    dcb_ucollation;	    /* Unicode collation table */
    PTR		    dcb_uvcollation;	    /* Unicode collation table 
					    ** (variable part) */
# define    DCB_COL 666		/* collation tag for allocator */
    char	    dcb_colname[DB_COLLATION_MAXNAME];	  /* collation name */
    char	    dcb_ucolname[DB_COLLATION_MAXNAME]; /* unicode collation */
    DM_MUTEX	    dcb_mutex;		    /* Synchronization mutex. */
    PTR	            dcb_tzcb;	            /* Timezone in which DB resides */
    i4	    dcb_wrkspill_idx;	    /* Which work loc to use next */
    struct _DMP_LOC_ENTRY		    /* Location entry for the root 
					    ** directory of a database.  This
					    ** entry is used to locate the 
					    ** database at open time. */
    {
      DB_LOC_NAME   logical;                /* Logical location name. */
      i4	    flags;		    /* Location flags. */
#define			DCB_ROOT	    0x0001L
#define			DCB_DATA	    0x0002L
#define			DCB_JOURNAL	    0x0004L
#define			DCB_CHECKPOINT	    0x0008L
#define			DCB_ALIAS	    0x0010L
#define			DCB_WORK	    0x0020L
#define			DCB_DUMP	    0x0040L
#define			DCB_AWORK	    0x0080L
#define			DCB_RAW		    0x0100L		
#define			DCB_SKIP	    0x0200L
/* String values of above bits */
#define	DCB_LOC_FLAGS	"\
ROOT,DATA,JOURNAL,CHECKPOINT,ALIAS,WORK,DUMP,AWORK,RAW,SKIP"

      i4	    phys_length;	    /* Length of physical 
                                            ** location name. */
      DM_PATH       physical;		    /* Physical location name. */
      i4   	    raw_start;	    	    /* Starting block of raw loc */
      i4   	    raw_blocks;	    	    /* Number of blocks for raw loc */
      i4	    raw_total_blocks;	    /* Total blocks in raw area */
    }               dcb_location;
# define DCB_SEC_ID_INCREMENT 100
    DB_TAB_ID	    rep_regist_tab;	   /* ID of dd_regist_tables */
    DB_TAB_ID	    rep_regist_tab_idx;	   /* ID of dd_reg_tbl_idx */
    DB_TAB_ID	    rep_regist_col;	   /* ID of dd_regist_columns */
    DB_TAB_ID	    rep_input_q;	   /* ID of dd_input_queue */
    DB_TAB_ID	    rep_dd_paths;	   /* ID of dd_paths */
    DB_TAB_ID	    rep_dd_db_cdds;	   /* ID of dd_db_cdds */
    DB_TAB_ID	    rep_dist_q;		   /* ID of dd_distrib_queue */
    i2		    dcb_rep_db_no;	   /* db no for replication */
    i2		    *dcb_bad_cdds;	   /* list of non-replicating CDDSs */
    u_i4	    dcb_dbcmptlvl;	   /* DB Major version No */
    i4	    dcb_1dbcmptminor;	   /* DB Minor version No */
    i4	    dcb_dbservice;	   /* FIPS related translation flags */

    DML_SEQ	    *dcb_seq;	   	    /* List of DML_SEQ's */
};

/*}
** Name: DMP_EXT - Extent Control Block.
**
** Description:
**      This extent control block contains the list of extents that the database
**	currently resides upon.  This control block is pointer at by the DCB.
**	The contents of the control block come from the configuration files list
**	of extents.
**
** History:
**      07-nov-1986 (Derek)
**          Created for Jupiter.
**	15-mar-1993 (jnash)
**	    Reassign value of EXT_CB from 20 to 27 to eliminate duplicate.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
[@history_template@]...
*/
struct _DMP_EXT
{
    DMP_EXT	    *ext_q_next;            
    DMP_EXT	    *ext_q_prev;
    SIZE_TYPE	    ext_length;		    /* Length of the control block. */
    i2              ext_type;               /* Type of control block for
                                            ** memory manager. */
#define                 EXT_CB              DM_EXT_CB
    i2              ext_s_reserved;         /* Reserved for future use. */
    PTR             ext_l_reserved;         /* Reserved for future use. */
    PTR             ext_owner;              /* Owner of control block for
                                            ** memory manager.  EXT will be
                                            ** owned by the DCB. */
    i4         ext_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 EXT_ASCII_ID        CV_C_CONST_MACRO('#', 'E', 'X', 'T')
    i4	    ext_count;		    /* Number of extents. */
    DMP_LOC_ENTRY   ext_entry[3];	    /* Array of extents.  The minimum
					    ** number of 3 is allocated with
					    ** the control block.  Any extras
					    ** are located contiguous with this
					    ** array. */
};

/*
**  FCB - SECTION
**
**       This section defines all structures needed to build
**       and access the information in a File Control Block (FCB).
**       The following structs are defined:
**           
**           DMP_FCB                 File Control Block.
*/

/*}
** Name:  DMP_FCB - DMF internal control block for open file information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about open files associated with database tables. 
**
**      Most of the information in the FCB comes from runtime information
**      provided from call to the compatability library DIopen
**      routine.  This control block varies in size for each operating
**      system.  The DI_FCBSIZE constant is used to determine how large
**      this control block should be.  All FCB's have a fixed size portion
**      which contains the control block header, a pointer to the table
**      control block (TCB)  associated with this control block, a
**      state variable indicating if the file is currently open or closed, 
**      and the file name.
**
**      FCBs are only associated with table control blocks.  
**      A pictorial representation of FCBs in an active transaction enviroment
**      is:
**   
**	    TCB			 LOCATION ARRAY      
**	+-------------+          +-----------+ 
**	|             |  +------>|LOC ID  (0)|	      FCB (LOC 00)   
**	|LOC ARRAY PTR|--+       |LOC NAME   |        +--------+
**	|             |          |LOC FCB PTR|------->|TCB PTR |
**	+-------------+          |           |        |        |
**                               |           |        +--------+
**                               |===========|
**                               |LOC ID  (1)|	      FCB (LOC 01)   
**                               |LOC NAME   |        +--------+
**                               |LOC FCB PTR|------->|TCB PTR |
**                               |           |        |        |
**                               +-----------+        +--------+
**

**     The FCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | fcb_q_next                       |  Initial bytes are
**                     | fcb_q_prev                       |  standard DMF CB
**                     | fcb_length                       |  header. 
**                     | fcb_type   | fcb_s_reserved      |
**                     | fcb_l_reserved                   |
**                     | fcb_owner                        |
**                     | fcb_ascii_id                     |
**                     |------ End of Header -------------|
**                     | fcb_tcb_ptr                      |
**                     | fcb_state                        |
**		       | fcb_last_page                    |
**                     | fcb_namelength                   |
**                     | fcb_filename                     |
**                     | fcb_di_ptr                       |
**                     | fcb_location  (pointer)          |
**                     |-------End of Fixed Portion ------|
**                     | area containing DI file context. |
**                     |                                  |
**                     +----------------------------------+
**                     
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	18-feb-1992 (bryanp)
**	    Temporary table enhancements:
**	    Added FCB_DEFERRED_IO and FCB_MATERIALIZING flags to fcb_state,
**	    and added fcb_locklist and fcb_deferred_allocation field.
**	    Added fcb_mutex to DMP_FCB.
**	10-sep-1992 (rogerk)
**	    Reduced Logging Project: Added support for partial tables.
**	    Added FCB_NOTINIT flag which indicates that an fcb has not
**	    been initialized with location information and should not
**	    be opened on the next dm2f_open_file call.
**	    Also added fcb_physical_allocation and fcb_logical_allocation
**	    fields to assist in dm2f_sense_file/dm2f_alloc_file protocols.
**	    Changed fcb_tcb_ptr to be a pointer to a TableIO control block
**	    rather than a TCB (temporarily left named fcb_tcb_ptr rather
**	    than fcb_tbio_ptr to avoid code merge conflicts).
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	25-Aug-1997 (jenjo02)
**	    Added fcb_log_id.
**	03-Dec-1998 (jenjo02)
**	    Added fcb_rawpgsize (table's page size in raw extent) to FCB.
**	06-Apr-1999 (jenjo02
**	    Removed old RAW structures, replaced fcb_rawpgsize with
**	    fcb_rawpages.
*/

struct _DMP_FCB
{
    DMP_FCB         *fcb_q_next;            /* Queue of FCBs off TCB. */
    DMP_FCB         *fcb_q_prev;
    SIZE_TYPE	    fcb_length;		    /* Length of control block. */
    i2              fcb_type;               /* Type of control block for
                                            ** memory manager. */
#define                 FCB_CB              DM_FCB_CB
    i2              fcb_s_reserved;         /* Reserved for future use. */
    PTR             fcb_l_reserved;         /* Reserved for future use. */
    PTR             fcb_owner;              /* Owner of control block for
                                            ** memory manager.  FCB will be
                                            ** owned by the server. */
    i4         fcb_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 FCB_ASCII_ID        CV_C_CONST_MACRO('#', 'F', 'C', 'B')
    DMP_TABLE_IO    *fcb_tcb_ptr;	    /* Pointer to the tableio control
                                            ** block associated with this
                                            ** FCB. */
    i4         fcb_state;              /* State of FCB. */
#define                 FCB_CLOSED          0L
#define                 FCB_OPEN            1L
#define                 FCB_IO              2L
#define                 FCB_FLUSH           4L
#define			FCB_DEFERRED_IO	    8L
#define			FCB_MATERIALIZING   16L
#define			FCB_NOTINIT   	    32L
#define			FCB_TEMP   	    64L	/* File is for a GTT temp */
/*
** String version of above flags for use in dumps
*/
#define FCB_STATES "OPEN,IO,FLUSH,DEFERRED,MATERIALIZING,NOTINIT,TEMP"

    i4	    fcb_locklist;	    /* saved locklist from build_fcb
					    ** call to be used when FCB is
					    ** actually materialized
					    */
    i4	    fcb_log_id;		    /* saved log_id from build_fcb
					    ** call to be used when FCB is
					    ** actually materialized
					    */
    i4	    fcb_deferred_allocation;/* Number of pages to allocate 
					    ** if/when this file changes from
					    ** deferred IO state to a real
					    ** file. */
    i4	    fcb_physical_allocation;/* Number of "real" pages in the
					    ** file - obtained from last
					    ** DIsense call done. */
    i4	    fcb_logical_allocation; /* Number of logical pages in the 
					    ** file.  This is the number of
					    ** pages given by the return value
					    ** of the last dm2f_sense_file
					    ** call and represents the number
					    ** of pages which may legally be
					    ** counted as part of the logical
					    ** table.
					    */
    i4	    fcb_rawstart;	    /* for RAW files, the raw file 
					    ** block # where this 'logical'
					    ** file starts, relative to
					    ** table's pagesize.
					    */
    i4	    fcb_rawpages;	    /* for RAW files, the number of
					    ** pages in the raw file
					    */
    DM_MUTEX	    fcb_mutex;		    /* mutex used for serializing
					    ** materialization of deferred
					    ** allocation temporary tables.
					    */
    i4	    fcb_last_page;	    /*	Last used page in the file. */
    i4	    fcb_namelength;	    /* Length of the filename. */
    DM_FILE         fcb_filename;
    struct _DI_IO
                    *fcb_di_ptr;            /* Pointer to the variable
                                            ** portion of the FCB used
                                            ** to contain the DI context. */
    DMP_LOC_ENTRY   *fcb_location;	    /* Location of this file. */
};

/*}
** Name: DMP_HCB - DMF internal control block for TCB hashing.
**
** Description:
**      This typedef defines the control block required to keep information
**	about the TCB's that are currently in memory.
**
**	The HCB is actually just a array of hash buckets to queues of TCB that
**	that hash into these buckets.  The information in each hash queue entry
**	includes the next and previous pointers.  Secondary indexes only hash on
**	the base-id, so they will always hash to (and find) the base table TCB;
**	Partition TCB's hash using both base and index-id's, because otherwise
**	the hash chain would be as long as the number of partitions (which can
**	be well into the 10000's for TPC-H type things).  Partitions share the
**	same base-id as the partitioned master, though, so finding the master
**	is easy enough.
**
**	Hash chains need mutex protection in a multi-threaded environment.  A
**	mutex for every hash chain won't work, though, because of partitioned
**	tables.  The master and its partitions may be on different chains, and
**	at times we may need to mutex both.  To avoid deadlock, what's done
**	instead is to access a mutex based solely on the base-id (even for
**	partitions).  We should only need a few dozen mutexes to suitably
**	parallelize access to unrelated	tables;  the mutex is only needed
**	when finding and creating TCB's.
**
**	I have rather arbitrarily decreed that there will be 7 bits of
**	mutex-hash, and another 7-bits of additional hash chain hash.  Thus,
**	each mutex protects 128 hash chains, and there are 16384 hash buckets
**	altogether.  (Buckets are small, and this only adds up to 150K or so
**	of memory, depending on how big	a mutex is in any given implementation.)
**
**	The hcb_tq mutex protects the entire hash table, and is taken when
**	manipulating the free queue (hcb_ftcb_list).
**	If taking both, a hash entry mutex precedes the hcb-tq mutex.
**	Also one can take both hash and tcb mutexes;  again, take the hash
**	entry mutex first.
**
**	A pictorial representation of the HCB in an active server 
**      environment is:
**
**
**	The HCB layout is as follows:
**
**                     +----------------------------------+
**     Byte            | hcb_q_next                       |  Initial bytes are
**                     | hcb_q_prev                       |  standard DMF CB
**                     | hcb_length                       |  header. 
**                     | hcb_type  | hcb_s_reserved       |
**                     | hcb_l_reserved                   |
**                     | hcb_owner                        |
**                     | hcb_ascii_id                     |
**                     |------ End of Header -------------|
**		       | hcb_buckets_hash                 |
**                     | hcb_tq_next                      |
**                     | hcb_tq_prev                      |
**                     | hash_q_next                      |
**                     | hash_q_prev                      |
**                     | hash_mutex                       |
**		       | hash_mutex_array                 |-->mutex-array
**                     | hcb_hash_array                   |
**                     |   .                              |
**                     |   .                              |
**                     |   .                              |
**                     +----------------------------------+
**		       | the mutex array                  |
**		       +----------------------------------+
**
** History:
**     30-oct-1985 (derek)
**          Converted to control block from SVCB.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	27-feb-1997 (cohmi01)
**	    Add items to limit number of base TCBs allocated. Bug 80242.
**	24-Oct-1996 (jenjo02)
**	    Removed hcb_mutex, which blocked all hash buckets, replacing it 
**	    with a semaphore per hash bucket (hash_mutex);
**	    Added hcb_tq_mutex to protect the TCB free queue.
**	07-May-1998 (jenjo02)
**	    Added hcb_cache_tcbcount.
**	15-Jan-2004 (schka24)
**	    Separate hash mutexes from hash entries thanks to partitioning.
**	5-Mar-2004 (schka24)
**	    Make tcb free list a real queue object.
*/

/* Some sizing constants */

#define HCB_MUTEX_ARRAY_SIZE	128		/* 7 bits */
#define HCB_HASH_ARRAY_SIZE	(128*128)	/* 14 bits */

/* Hash calculation macros -- empirically tested for decent results.
** Multiplicative hashes seem to work well:  multiply and grab the top 7 bits.
** The hash bucket hash consists of the mutex hash extended by more bits;  this
** ensures that all buckets that a base-id hashes to are covered by the same
** mutex.  It also implies that given a bucket index, just shift right 7
** (divide by 128) to get the mutex index;  although this operation is not
** frequently needed.
*/

#define HCB_MUTEX_HASH_MACRO(db_id,tab_base) \
    ((((u_i4)((db_id)+(tab_base))*1970050171)>>25)&(HCB_MUTEX_ARRAY_SIZE-1))

#define HCB_HASH_MACRO(db_id,tab_base,tab_ix) \
    ( (((HCB_MUTEX_HASH_MACRO(db_id,tab_base))<<7) | \
      ((( (u_i4)((db_id)+((tab_ix)<0?(tab_ix)&DB_TAB_MASK:(tab_base))) * 933855867)>>25))) \
     &(HCB_HASH_ARRAY_SIZE-1))

#define HCB_HASH_TO_MUTEX_MACRO(hash_ix) ((hash_ix)/ (HCB_HASH_ARRAY_SIZE/HCB_MUTEX_ARRAY_SIZE))

struct _DMP_HCB
{
    DMP_HCB	    *hcb_q_next;            /* Not used. */
    DMP_HCB	    *hcb_q_prev;	    /* Not used. */
    SIZE_TYPE	    hcb_length;	            /* Length of control block. */
    i2              hcb_type;               /* Type of control block for
                                             ** memory manager. */
#define                 HCB_CB              DM_HCB_CB
    i2              hcb_s_reserved;         /* Reserved for future use. */
    PTR             hcb_l_reserved;         /* Reserved for future use. */
    PTR             hcb_owner;              /* Owner of control block for
                                             ** memory manager.  HCB will be
                                             ** owned by the server. */
    i4         hcb_ascii_id;           /* Ascii identifier for aiding
                                             ** in debugging. */
#define                 HCB_ASCII_ID        CV_C_CONST_MACRO('#', 'H', 'C', 'B')
    i4	    hcb_tcblimit;	    /* maximum number of base TCBs */
    i4	    hcb_tcbcount;	    /* number of base TCBs allocated */
    i4	    hcb_cache_tcbcount[DM_MAX_CACHE]; /* TCBs allocated per cache */
    i4	    hcb_tcbreclaim;	    /* # of times TCB reclaimed */
    DM_MUTEX	    hcb_tq_mutex;	    /* Multi-thread synchronization. */
    DM_MUTEX	    *hcb_mutex_array;	    /* Synchronizers for hash chain access */
    /* Note that this queue points to the actual queue object, i.e. into
    ** the middle of the TCB, not the start of the TCB.  As usual.
    */
    QUEUE	    hcb_ftcb_list;	    /* Queue of TCBs that can be 
                                            ** released. */
    struct _DMP_HASH_ENTRY
    {
     DMP_TCB	    *hash_q_next;	    /* Queue of table control blocks. */
     DMP_TCB	    *hash_q_prev;
    }		    hcb_hash_array[1];	    /* Array of hash buckets. */
};


/*}
** Name: DMP_PINFO - Page information container.
**
** Description:
**
**	Replaces DMPP_PAGE **page as the "page" parameter communicated
**	to and from dm0p cache functions:
**
**	dm0p_fix_page()
**	dm0p_unfix_page()
**	dm0p_cachefix_page()
**	dm0p_uncache_fix()
**	dm0pMakeCRpage()
**	dm0pPinBuf()
**	dm0pUnpinBuf()
**	dm0pMutex()
**	dm0pUnmutex()
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Created.
*/
struct _DMP_PINFO
{
    DMPP_PAGE	*page;		/* Pointer to fixed page */
    DMPP_PAGE	*CRpage;	/* Pointer to saved CRpage, as needed */
    DMPP_PAGE	*Rootpage;	/* Pointer to Root page, always */
    i4		pincount;	/* Count of transaction's pins on page */
};

/* Macro to initialize a DMP_PINFO so you don't have to know what's inside */
#define DMP_PINIT(p) \
    (p)->page = (p)->CRpage = (p)->Rootpage = NULL, (p)->pincount = 0


/*
** Name: DMP_ROWACCESS - Control info needed for compressed row accessing
**
** Description:
**
**	This structure bundles up all the info needed to access a row,
**	possibly with (row) compression.
**
**	The DMP_ROWACCESS structure contains:
**	- Attribute pointer array and number of attributes
**	- the compression type (TCB_C_xxx)
**
**	The rowaccess user has to initialize the above, and then it calls
**	a setup routine to finish the rowaccessor.  Setup calculates the
**	additional structure members:
**
**	- the worst-case row expansion in bytes, to be added to relwid to
**	  get the worst-case row width.  (All compression schemes can make
**	  the row bigger in some cases.)
**	- (De)compression control array, if used, and number of entries.
**	  The control array is compression scheme specific;  e.g. for
**	  standard row compression, it's a set of directives that eliminate
**	  the need to examine the row attribute by attribute.
**	- Compression and decompression call vector.
**	- A flag saying whether the compression method affects LOB
**	  coupons (the ADP_PERIPHERAL object stored in the row).
**
**	There are typically a few of these things floating around, e.g.
**	one for the base row, one for leaf entries, maybe one for index
**	entries, etc.
**
**	FOR FUTURE:  at the moment, the rowaccessor is entirely concerned
**	with attributes and compression.  It might be nice to somehow
**	encapsulate the need for row versioning here too...????
**
** Edit History:
**
** 	19-Feb-2008 (kschendel)
**	    Created to better encapsulate row compression.
*/

typedef struct _DMP_ROWACCESS DMP_ROWACCESS;

/* Typedef some of the function dispatches needed by row-access related
** stuff.  This makes declarations in the code somewhat easier to read.
*/
typedef DB_STATUS (*DMP_RACFUN_CMP)(	/* Compress function pointer */
	DMP_ROWACCESS *rac,
	char *rec,
	i4 rec_size,
	char *crec,
	i4 *crec_size);

typedef DB_STATUS (*DMP_RACFUN_UNCMP)(	/* Decompress function pointer */
	DMP_ROWACCESS *rac,
	char *src,
	char *dst,
	i4 dst_maxlen,
	i4 *dst_size,
	char **buffer,
	i4 row_version, 
	ADF_CB *adf_cb);

/* these aren't in the ROWACCESS struct but are related */
typedef i4 (*DMP_RACFUN_XPN)(		/* worstcase expansion amount */
	DB_ATTS **atts,
	i4 att_count);

typedef DB_STATUS (*DMP_RACFUN_CSETUP)(	/* Control array setup */
	DMP_ROWACCESS *rac);

typedef i4 (*DMP_RACFUN_CSIZE)(		/* Control array size estimate */
	i4 att_count,
	i4 relversion);



struct _DMP_ROWACCESS
{
    DB_ATTS	**att_ptrs;		/* Array of pointers to attributes;
					** zero origin.  (Some att arrays
					** count from 1, but not this pointer
					** array.)
					*/
    PTR		cmp_control;		/* Base of compression control array,
					** if needed by compression type
					*/
    DMP_RACFUN_CMP dmpp_compress;	/* Compress function pointer */
    DMP_RACFUN_UNCMP dmpp_uncompress;	/* Decompress function pointer */

    i4		worstcase_expansion;	/* Worst-case expansion in bytes */
    i4		control_count;		/* Number of entries in cmp_control.
					** Prior to array setup, will be size
					** of caller provided array space
					** in bytes.  (hence the i4)
					*/
    i2		att_count;		/* Number of attributes in att_ptrs */
    i2		compression_type;	/* Compression type code, see
					** tcbrel.relcomptype for codes.
					** TCB_C_xxx */
    bool	compresses_coupons;	/* TRUE if compression affects coupons
					** (LOB columns).  Coupons are inserted
					** after the data row is initially
					** allocated, and if this can change the
					** row size, allocation needs to know.
					*/
};

struct _DMP_POS_INFO {
    DM_TID	bid;			/* pageno, line of the key */
    DM_TID	tidp;			/* tidp */
    LG_LSN	lsn;			/* page lsn */
    u_i4	clean_count;		/* page clean count */
    DM_PAGESTAT page_stat;		/* page stat */
    DM_PAGENO	nextleaf;		/* page nextpage */
    i4		line;			/* line number for diagnostics */
    i1		valid;
};


/*
**  RCB - SECTION
**
**       This section defines all structures needed to build
**       and access the information in a Record access Control Block (RCB).
**       The following structs are defined:
**           
**           DMP_RCB                 Record access Control Block.
*/

/*}
** Name:  DMP_RCB - DMF internal control block for open table information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about accesing records from a table.  This information varies on
**      each open of a table.  
**      
**      Most of the information in the RCB comes from runtime information
**      provided on the open call to DMF.  All opens must be within a 
**      transaction context, therefore the RCB contains a transaction 
**      identifier and is also queued to a Transaction Control Block (XCB).
**      The RCB also contains information relating this RCB to a specific 
**      transaction savepoint and a specific transaction lock list.
**      Other open information includes database and table identifiers to
**      uniquely identify the table being opened, locking parameters which
**      may have been previously specified by a user using setlock commands,
**      the access mode,  and the update mode.  
**
**      The RCB also maintains positioning information needed for succesive
**      record retrieval.  This information is provided at runtime on
**      a position call to DMF and includes the lowest and highest TIDs 
**      needed for this retrieval(this allows partial and exact scans
**      of a table), the current TID which points to the record last 
**      returned with a get call to DMF, the qualifying function and
**      argument list which provides a mechanism for DMF to fully 
**      qualify records returned with a get call to DMF, and two records
**      used for storing the record pointed to by current TID and one for
**      a work area for creating keys, compressing records, etc.  
**      

**      The RCB is primarily used by the record access routines of the
**      physical layer (i.e. routines to get, delete, replace, and
**      put records).  However the low level routines for page management
**      (including page level locking and escalation of locks to table
**      level) and recovery management (including logging of updates,
**      page allocations, btree splits or joins, etc.) need to access and
**      update certain information contained in the RCB.
**
**      A RCB is associated with a transaction through the Transaction
**      Control Block (XCB) queue as well as a table through the Table
**      Control Block (TCB) queue.  Since the size of an RCB is dependent
**      on the table it is associated with (i.e. the  record length 
**      is differs per table), once an RCB is allocated, it is not
**      deallocated until the associated TCB is deallocated.  Instead
**      it is queued off a TCB queue for free RCBs.  This significantly
**      shortens open table time for repeat operations.  
**
**	A summary of the lists that an RCB can be found on:
**	Each TCB has an active-RCB list headed at tcb_rq_next and linked
**	through rcb_q_next and _prev.
**	Each TCB has a free-TCB list headed at tcb_fq_next and linked
**	through rcb_q_next and _prev.  An RCB is either active or free.
**	All RCB's for a given TCB and transaction are linked in a singly-
**	linked ring, linked through rcb_rl_next and generally found
**	via xcb_rq_next.  This ring is only walked or manipulated by
**	the session owning the transaction, it's not to be messed with
**	by other threads.
**	All RCB's for a table that might possibly be updating are connected by
**	a doubly-linked list through rcb_upd_list starting at tcb_upd_rcbs.
**	All RCB's for a partitioned master that might possibly be updating
**	any partition are similarly linked through the master tcb_upd_rcbs
**	and rcb_mupd_list.  These two "possibly updating" lists are merely for
**	gathering reasonably accurate row and page counts that include
**	in-flight queries.  Since partitioned masters are never updated
**	directly (there's nothing there to update!), an RCB cannot be
**	on both lists for the same TCB.
**
**	The TCB mutex must be held to manipulate any of the lists except
**	for the rl_next ring.
**	
**       
**      A pictorial representation of RCBs in an active transaction enviroment
**      is:
**   
**            XCB                RCB                      RCB
**       +-----------+        +--------+               +--------+
**       |           |        | TCB Q  |<----+         | TCB Q  |<-----+ 
**       |           |        |        |     |         |        |      |
**       |RCB Tran Q |------->| Tran Q |-----|-------->| Tran Q |      |
**       |           |        |        |     |         |        |      |
**       |           |        +--------+     |         +--------+      |
**       +-----------+                       |                         |
**                               +-----------+                         |
**                               |                                     |
**                   TCB         |                         TCB         |
**              +-------------+  |                    +-------------+  |
**              |RCB Active Q |--+      RCB           |RCB Active Q |-- 
**              |             |      +---------+      |             |  
**              |RCB Free Q   |----->| TCB Q   |      |             |
**              |             |      |         |      |             |
**              +-------------+      |         |      +-------------+ 
**                                   +---------+
**                                       

**     The RCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | rcb_q_next                       |  Initial bytes are
**                     | rcb_q_prev                       |  standard DMF CB
**                     | rcb_length                       |  header. 
**                     | rcb_type   | rcb_s_reserved      |
**                     | rcb_l_reserved                   |
**                     | rcb_owner                        |
**                     | rcb_ascii_id                     |
**                     |------ End of Header -------------|
**                     | rcb_tcb_ptr                      |
**                     | rcb_tran_id                      |
**                     | rcb_seq_number                   |
**                     | .... lots more stuff in the      |
**                     | fixed size portion of the RCB    |
**		       | currently ending at rcb_pcb      |
**                     |------ End of Header -------------|
**                     | area containing table record     |    
**                     |----------------------------------|
**                     | area containing scratch work     |
**                     | area at least as large as a      |
**                     | table record                     |  
**                     |----------------------------------|
**                     | area containing scratch work     |
**                     | area at least as large as a      |
**                     | table key                        |  
**                     +----------------------------------+
**                     
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added rcb_hash_nofull field to keep track of the free space
**	    status on a hash overflow chain as the chain is traversed.
**	 7-nov-1988 (rogerk)
**	    Added rcb_compress field.  This gives the compression type
**	    for compressed tables.
**	 2-may-1989 (anton)
**	    added collation field
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Added rcb_gateway_rsb.  This
**	    is the gateway request block that is returned on a gateway table
**	    open.  It must be passed to the gateway for all record level calls
**	    on that table.
**	 6-dec-1989 (rogerk)
**	    Added rcb_bt_bi_logging field to indicate that btree leaf pages
**	    should use Before Image logging.  This is used during online
**	    backup so that leaf updates can be backed out.
**	14-jun-90 (linda, bryanp)
**	    Add RCB_A_OPEN_NOACCESS access code for rcb, indicating open which
**	    does not allow access other than a close.  For gateways support of
**	    remove table command where underlying file does not exist.
**	18-jun-90 (linda, bryanp)
**	    Add RCB_A_MODIFY access code, indicating that we are opening the
**	    table only to get statistical information.  For Gateway tables,
**	    this means that it is okay if the file does not exist; default
**	    statistics will be used in that case.
**	29-apr-1991 (Derek)
**	    Added open duration performance counters.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	    19-nov-1990 (rogerk)
**		Added an ADF_CB to the Record Control Block.  This is now used
**		by DMF instead of allocated adf control blocks on the stack
**		when they are needed.  This makes it easier for DMF to keep
**		track of and set ADF variables associated with each session.
**		This was done as part of the DBMS Timezone changes.
**	    12-feb-1991 (mikem)
**		Added two new fields rcb_val_logkey and rcb_logkey to DMP_RCB
**		structure to support returning logical key values to the client
**		which caused the insert of the tuple and thus caused them to be
**		assigned.
**	    23-jul-91 (markg)
**		Added new RCB_F_USER_ERROR return status for qualification
**		function. Also included some explanatory comments about the 
**		actions performed for each qualification function return 
**		status. (b38679)
**	05-jun-92 (kwatts)
**	    Added comment giving updated values for rcb_compress field.
**	05-nov-92 (jnash)
**	    Temp hack of recovery bitfield to bypass tid locking 
**	    until new recovery code integrated.
**	18-nov-92 (jnash)
**	    Reduced Logging Project (phase 2):
**	      - Took out temporary use of rcb_recovery field.
**	      - Add rcb_usertab_jnl for system catalog rcbs to indicate the
**		journal status of the usertable being accessed.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed obsolete rcb_bt_bi_logging and rcb_merge_logging fields.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Add rcb_state flag RCB_IN_MINI_TRAN to allow tracking when a
**		    particular RCB is in the process of performing a mini-
**		    transaction. Technically, I suppose, the RCB is not the
**		    correct place for this flag, since it's really more of an
**		    XCB state than an RCB state, but the rcb_state field is
**		    convenient since the RCB happens to be the control block
**		    that is available to the buffer manager, which wishes to
**		    know this information (for enforcing cluster recovery
**		    protocols).
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	8-may-96 (stephenb)
**	    Add rep_* fileds to support replication.
**	06-sep-1996 (canor01)
**	    Add rcp_tupbuf for compression.  Allocate buffer once.
**     01-aug-1996 (nanpr01 for ICL phil.p)
**          Added new rcb_states of RCB_RECOVER and RCB_FLUSHED for support 
**          of DMCM protocol. Both states used when causing all caches to
**          flush dirty pages for a table prior to smart disk scan / modify.
**	21-aug-96 (stephenb)
**	    Add RCB pointer fpr replicator cdds lookup table (rep_cdds_rcb)
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add RCB_K_ROW value to indicate row level locking granularity;
**          For reposition to CURRENT cursor position in BTREE added:
**              rcb_low_lsn, rcb_low_clean_count, rcb_low_key_ptr, rcb_low_tidp
**          For reposition to FETCHED row in BTREE added:
**              rcb_fet_lsn, rcb_fet_clean_count, rcb_fet_key_ptr, rcb_fet_tidp
**          For reposition to STARTING position in BTREE added:
**              rcb_pos_bt_lsn, rcb_pos_bt_clean_count
**              rcb_p_flag, rcb_p_lk_type, rcb_p_hk_type
**          Add the following for rcb_update for replace as delete/put:
**              rcb_bt_put_lsn, rcb_bt_put_cc, rcb_bt_del_lsn, rcb_bt_del_cc
**              These need not be in the rcb, but is was just convenient. 
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Add rcb_iso_level field for isolation level.
**      21-apr-97 (stial01)
**          Added rcb_alloc_bid, rcb_alloc_lsn, rcb_alloc_clean_count,
**          and rcb_alloc_tidp to the RCB.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          - added RCB_CSRR_LOCK, RCB_*_HAD_RECORDS and RCB_*_UPDATED 
**          flags for rcb_state;
**          - added new fields to store lock ids and flags indicating that
**          locks should be released.
**      29-jul-97 (stial01)
**          DMP_RCB: Removed temporary fields no longer needed for rcb update
**	23-jul-1998 (nanpr01)
**	    Added flag to position at the end of qualification range.
**	11-Aug-1998 (jenjo02)
**	    Added rcb_tup_adds, rcb_page_adds, rcb_reltups, rcb_relpages.
**	19-Aug-1998 (jenjo02)
**	    Added rcb_fix_lkid.
**      05-may-1999 (nanpr01)
**          Added rcb_val_lkid.
**	09-Feb-2001 (jenjo02)
**	    Added RCB_RECORD_PTR_VALID flag to rcb_state.
**	14-Mar-2003 (jenjo02)
**	    Added RCB_ABORTING, set by dmx_abort() so all concerned
**	    may know (B109842).
**      24-apr-2003 (stial01)
**          Added new fields to rcb to support projection of columns (b110061)
**	14-Jan-2004 (schka24)
**	    Add new lists for row/page counting; trim obsolete and wrong
**	    rcb layout diagram.
**	22-Jan-2004 (jenjo02)
**	    For Parallel/Partitioning project, added rcb_dmrAgent,
**	    rcb_siAgents flags, rcb_reltid, rcb_partno, 
**	    rcb_locked_tid_reltid, rcb_base_rcb, rcb_rtb_ptr, 
**	    rcb_si_oldtid, rcb_si_newtid, rcb_si_flags, rcb_compare,
**	    rcb_change_set, rcb_value_set.
**      30-Mar-2004 (hanal04) Bug 107828 INGSRV2701
**          Added bit-field rcb_temp_iirelation. If set to true the
**          table is being used as a temporary copy of iirelation and
**          will be used to recreate a clean version of iirelation (as
**          seen in a sysmod).
**	07-Apr-2004 (jenjo02)
**	    Added RCB_SI_VALUE_LOCKED flag.
**	20-Apr-2004 (jenjo02)
**	    Changed RCB_FORCE_ABORT to match SCB_RCB_FABORT
**	12-May-2004 (schka24)
**	    Add ET parent ID for closing dmpe-opened etabs when parent
**	    is closed.
**      04-oct-2005 (bolke01)
**          Corrected bitmap reserved for future use.
**	13-Feb-2007 (kschendel)
**	    Added session ID for cancel checking.
**	09-Oct-2007 (jonj)
**	    Add RCB_STATE, RCB_SI_FLAGS defines.
**	04-Apr-2008 (jonj)
**	    Deleted RCB_RECOVER, RCB_FLUSHED state bits, no longer used
**	    by DMCM, add RCB_CSRR_FLAGS, RCB_ISO_LEVEL, RCB_LK_TYPE,
**	    RCB_UPDATE_MODE, RCB_ACCESS_MODE, RCB_K_DURATION defines.
**	11-Apr-2008 (kschendel)
**	    Expand RCB qualification function call to match up better with
**	    CX execution, which is the time critical and most common case.
**      01-Dec-2008 (coomi01) b121301
**          Add bitflag for context information from DSH
**      21-may-2009 (huazh01)
**          add rcb_hashbuf and define HASH_BUFLEN.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add  RCB_K_CROW rcb_lk_type.
**	    Add RCB_READ_COMMITTED isolation mode
**	    (same as RCB_CURSOR_STABILITY), crow_locking() macro,
**	    RCB_CONSTRAINT.
**	    Add rcb_dmr_opcode, rcb_crow_tid, rcb_mvcc_lkid.
**	    Replaced DMPP_PAGE *rcb_data_page_ptr, *rcb_other_page_ptr
**	    with DMP_PINFO *rcb_data, *rcb_other.
**	26-Feb-2010 (jonj)
**	    Add rcb_crib_ptr, change RootPageIsInconsistent macro
**	    to reference this rather than xcb_crib_ptr.
*/

struct _DMP_RCB
{
    DMP_RCB         *rcb_q_next;            /* Queue of active or
                                            ** or free RCBs off TCB. */
    DMP_RCB         *rcb_q_prev;
    SIZE_TYPE	    rcb_length;		    /* Length of control block. */
    i2              rcb_type;               /* Type of control block for
                                            ** memory manager. */
#define                 RCB_CB              DM_RCB_CB
    i2              rcb_s_reserved;         /* Reserved for future use. */
    PTR             rcb_l_reserved;         /* Reserved for future use. */
    PTR             rcb_owner;              /* Owner of control block for
                                            ** memory manager.  RCB will be
                                            ** owned by a transaction. */
    i4         	    rcb_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 RCB_ASCII_ID        CV_C_CONST_MACRO('#', 'R', 'C', 'B')
    CS_SID	    rcb_sid;		    /* Session ID that this RCB belongs
					    ** to, for cancel checks. Same as
					    ** rcb->xcb->scb->scb_sid except
					    ** that the latter has to be null-
					    ** checked which is a PITA.
					    ** rcb_sid might be 0 if the RCB is
					    ** not associated with a real user
					    ** session!
					    */
    DMP_TCB         *rcb_tcb_ptr;           /* Pointer to table control
                                            ** block associated with this 
                                            ** RCB. */

    DMP_ROWACCESS   *rcb_data_rac;	    /* Row accessor info for base
					    ** (data) row;  usually ptr to
					    ** the TCB data rac, but might be
					    ** pointer to projection info.
					    */

    /* Projection returns a subset of the normal data row.  It does this
    ** via a cunning trick:  it builds a copy of the normal data att list,
    ** and marks the ones not wanted as "dropped"!  proj_relversion is
    ** made high enough to make unwanted columns look dropped.
    */
    DMP_MISC	    *rcb_proj_misc;	    /* Misc-cb area for proj */
    DB_ATTS	    *rcb_proj_atts_ptr;     /* jiggered atts list */
    i4		    rcb_proj_relwid;        /* projected row width */
    u_i2	    rcb_proj_relversion;    /* used for projection */

    DB_TRAN_ID      rcb_tran_id;            /* Unique transaction id. */
    DM_TID          *rcb_new_tids;          /* New tid info (row locking)
					    ** Typically will point to 
					    ** rcb_tmp_tids... unless we
					    ** escalate
					    */
    i4               rcb_new_max;           /* Max tids                   */
    i4               rcb_new_cnt;
#define RCB_TMP_TID_MAX 20
    DM_TID           rcb_tmp_tids[RCB_TMP_TID_MAX]; 

    i4         	    rcb_seq_number;         /* Unique sequence number per
                                            ** deferred cursor within a 
                                            ** transaction. */
    DMP_PINFO	    rcb_data;          	    /* A saved fixed data page */
    DMP_PINFO	    rcb_other;   	    /* Another saved fixed page. */
    DMP_MISC	    *rcb_bulk_misc;

    i4		    rcb_bulk_cnt;	    /* # segs on rcb_etab_disassoc */
    i4		    rcb_et_parent_id;	    /* Parent table base-ID iff rcb
					    ** was opened by dmpe and it's
					    ** for a blob etab.
					    */
    i4		    rcb_opt_extend;
    i4	    	    rcb_log_id;             /* A unique identifier to the
                                            ** logger to indicate the
                                            ** transaction log id.
		                            */
    u_i2	    rcb_slog_id_id;	    /* Shared log id_id, from rcb_log_id
    					    ** unless shared transaction */
    BITFLD	    rcb_logging:1;	    /* A flag to indicate whether the
					    ** logging is needed. */
#define			RCB_LOGGING	    1L
    BITFLD	    rcb_journal:1;	    /* A flag to inform the logging
					    ** routines to mark the put/delete/
					    ** replace log records of system 
					    ** catalogs as journalled. */
#define			RCB_JOURNAL	    1L
    BITFLD          rcb_internal_req:1;
#define                 RCB_INTERNAL        1L
                                            /* A flag to inform the lower
                                            ** level code that this is 
                                            ** an internally generated rcb
                                            ** used during such operations 
                                            ** as modifying a table, creating
                                            ** an index.  
                                            */
    BITFLD	    rcb_usertab_jnl:1;	    /* Flag used on system catalog
					    ** RCB's only to relate that 
					    ** a user table is journaled */
    BITFLD          rcb_temp_iirelation:1;  /* Flag used to indicate this
                                            ** relation is a temporary copy
                                            ** of iirelation (e.g. as seen
                                            ** in a sysmod).
                                            */
    BITFLD	    rcb_dmrAgent:1;	    /* Do DMR operations using Agent */
    BITFLD	    rcb_siAgents:1;	    /* Indexes are updated
					    ** by SI agents */
    BITFLD          rcb_dsh_isdbp:1;        /* Reflect QEQP_ISDBP Flag From DSH */
    BITFLD	    rcb_bits_free:24;	    /* reserved for future use */
    i4         	    rcb_iso_level;          /* Isolation level. */         
#define                  RCB_READ_UNCOMMITTED    1L
#define                  RCB_CURSOR_STABILITY    2L
#define                  RCB_READ_COMMITTED      RCB_CURSOR_STABILITY
#define                  RCB_REPEATABLE_READ     3L
#define                  RCB_SERIALIZABLE        4L
/* String values of above bits */
#define	RCB_ISO_LEVEL "\
,READ_UNCOMMITTED,READ_COMMITTED,REPEATABLE_READ,SERIALIZABLE"

    i4         	    rcb_lk_id;              /* A unique identifier to the
                                            ** locker to indicate which
                                            ** lock list (one per transaction)
                                            ** to use for locking. */
    i4         	    rcb_lk_type;            /* Current lock type, must be 
                                            ** row, page or table.  Can be
                                            ** escalated from page or row to
                                            ** table by low level routines. */
#define                  RCB_K_TABLE       1L    
#define                  RCB_K_PAGE        2L
#define                  RCB_K_ROW	   3L
#define                  RCB_K_CROW	   4L /* MVCC row lock protocols
					      ** (no covering page locks) */
/* String values of above bits */
#define RCB_LK_TYPE "\
,TABLE,PAGE,ROW,CROW"
    i4         	    rcb_lk_limit;           /* Maximum number of page locks
                                            ** to acquire before escalating
                                            ** to table level locking. */
    i4         	    rcb_lk_count;           /* Current number of page locks
                                            ** held. */
    i4         	    rcb_lk_mode;            /* Current Table lock mode. */
#define                  RCB_K_N            LK_N
                                            /* Means No Lock. */
#define                  RCB_K_IS           LK_IS
                                            /* Means Read Shared Page Lock. */
#define                  RCB_K_IX           LK_IX
                                            /* Means Write Exclusive Page 
                                            ** Lock. */
#define                  RCB_K_S            LK_S
                                            /* Means Read Shared Table Lock. */
#define                  RCB_K_SIX          LK_SIX
                                            /* Not used. */
#define                  RCB_K_U            LK_U
					    /* Update mode lock - not used */
#define                  RCB_K_X            LK_X
                                            /* Means Write Exclusive Table 
                                            ** Lock. */
    i4         	    rcb_timeout;            /* Amount of time to wait for a
                                            ** lock.  Zero means wait 
                                            ** forever. */
    i4         	    rcb_update_mode;        /* Update mode. */    
#define                 RCB_U_DEFERRED      1L
#define                 RCB_U_DIRECT        2L
#define                 RCB_U_PAGE          3L   
/* String values of above bits */
#define	RCB_UPDATE_MODE "\
,DEFERRED,DIRECT,PAGE"
     i4        	    rcb_access_mode;        /* Access mode. */
#define                 RCB_A_READ          1L
#define                 RCB_A_WRITE         2L
#define			RCB_A_OPEN_NOACCESS 5L
#define			RCB_A_MODIFY	    6L
/* String values of above bits */
#define	RCB_ACCESS_MODE "\
,READ,WRITE,3,4,OPEN_NOACCESS,MODIFY"
    DMP_RCB	    *rcb_rl_next;	    /* Next RCB for the same table. */
    DML_XCB         *rcb_xcb_ptr;           /* Pointer to the transaction 
                                            ** control block this RCB is
                                            ** queued to. */
    DMP_RCB         *rcb_xq_next;           /* Queue of RCBs associated
                                            ** with a transaction. */
    DMP_RCB         *rcb_xq_prev;
    QUEUE	    rcb_upd_list;	    /* Possibly-updating RCB list for the rcb's table */
    QUEUE	    rcb_mupd_list;	    /* Possibly-updating RCB for the rcb's
					    ** partitioned master table.
					    ** NOTE: these two queue lists point at the queue
					    ** structs, not the rcb or tcb itself
					    */
    i4         	    rcb_k_type;             /* Lock type specified at open
                                            ** and not modified by lower
                                            ** level routines. Must be 
                                            ** any of rcb_lk_type values */
    i4         	    rcb_k_mode;             /* Table lock mode specified at open
                                            ** and not modified by lower
                                            ** level routines.  Must be
                                            ** RCB_K_N, RCB_K_IS, RCB_K_IX,
                                            ** RCB_K_S, or RCB_K_X. */
    i4         	    rcb_k_duration;         /* Duration of lock. Read/nolcok,
					    ** during access or entire 
					    ** transaction.  If the user has 
					    ** "set" NOREADLOCK, there is no
					    ** lock held for reading. If the
					    ** table is a system catalog, the
                                            ** duration is only during access, 
                                            ** otherwise it is transaction. */
#define                 RCB_K_READ	    0x01L
#define                 RCB_K_TRAN          0x02L 
#define                 RCB_K_PHYSICAL      0x04L
#define			RCB_K_TEMPORARY	    0x08L
/* String values of above bits */
#define	RCB_K_DURATION "\
READ,TRAN,PHYSICAL,TEMPORARY"
    i4         	    rcb_state;              /* Current state of this RCB. */
#define			RCB_OPEN	        0x00000001L
#define			RCB_POSITIONED	        0x00000002L
#define			RCB_WAS_POSITIONED      0x00000004L
#define			RCB_LSTART              0x00000008L
					    /* Load started. */
#define			RCB_LEND                0x00000010L
					    /* Load finished. */
#define                 RCB_FETCH_REQ           0x00000020L
					    /* For multiple replaces
                                            ** on same record, used to
                                            ** indicate need to refetch
                                            ** the record previously read.
					    */
#define			RCB_READAHEAD	        0x00000040L
					    /* Need to perform read ahead
					    ** buffer management.  
					    */ 
#define			RCB_LRESTART            0x00000080L 
					    /* This was a load table which was 
					    ** dumped in order to reload it.
					    */
#define			RCB_ABORTING            0x00000100L 
					    /* The RCB is being aborted */
#define			RCB_CONSTRAINT          0x00000200L 
					    /* The RCB is being used by
					    ** a constraint
					    */
#define			RCB_IN_MINI_TRAN        0x00000400L
					    /* This RCB has called dm0l_bm but
					    ** has not yet called dm0l_em. This
					    ** happens to be the one case where
					    ** it's OK for a transaction to fix
					    ** multiple system catalog pages
					    ** at once.
					    */
#define			RCB_CURSOR              0x00000800L 
					    /* The RCB is being used by
					    ** a cursor
					    */
#define 		RCB_PREFET_PENDING      0x00001000L  
					    /*
					    ** A prefetch event was scheduled
					    ** for this rcb, but hasn't been
					    ** completed by readahead thread.
					    */
#define			RCB_PREFET_REQUEST      0x00002000L	
					    /*
					    ** This request is on behalf of
					    ** a readahead thread in response
					    ** to a pending prefetch request.
					    */
#define			RCB_PREFET_AVAILABLE    0x00004000L	
					    /*
					    ** Indicates that this table
					    ** qualifies for prefetch requests,
					    ** and that readahead threads are
					    ** present. set in rcb_allocate().
					    */
#define                 RCB_CSRR_LOCK           0x00020000L 
                                            /*  Cursor_Stability/Repeatable_
                                            **  Read locking mode is ON.
                                            */
#define                 RCB_TABLE_HAD_RECORDS   0x00040000L
                                            /* Record returned from the table 
                                            ** related to this RCB.
                                            */
#define                 RCB_DPAGE_HAD_RECORDS   0x00080000L 
                                            /* Record returned from the data
                                            ** page fixed in this RCB. 
                                            */
#define                 RCB_LPAGE_HAD_RECORDS   0x00100000L 
                                            /* Record returned from the leaf
                                            ** page fixed in this RCB.
                                            */
#define                 RCB_TABLE_UPDATED       0x00200000L 
                                            /* Record updated in the table 
                                            ** related to this RCB.
                                            */
#define                 RCB_DPAGE_UPDATED       0x00400000L 
                                            /* Record updated on the data    
                                            ** page fixed in this RCB. 
                                            */
#define                 RCB_LPAGE_UPDATED       0x00800000L 
                                            /* Record updated on the leaf      
                                            ** page fixed in this RCB. 
                                            */
#define                 RCB_ROW_UPDATED         0x01000000L 
                                            /* Current row locked by this
                                            ** RCB has been updated.
                                            */
#define                 RCB_UPD_LOCK            0x02000000L 
					    /* Get update mode lock */
#define                 RCB_NO_CPN              0x04000000L
#define                 RCB_RECORD_PTR_VALID    0x08000000L
					    /* *rcb_record_ptr contains
					    ** an (uncompressed) copy
					    ** of record from page
					    */
/* String values of above bits */
#define	RCB_STATE	"\
OPEN,POSITIONED,WAS_POSITIONED,LSTART,LEND,FETCH_REQ,READAHEAD,\
LRESTART,ABORTING,CONSTRAINT,IN_MINI,CURSOR,PREFET_PEND,PREFET_REQ,PREFET_AVAIL,\
8000,10000,CSRR_LOCK,TABLE_HAD_RECS,DPAGE_HAD_RECS,LPAGE_HAD_RECS,\
TABLE_UPDATED,DPAGE_UPDATED,LPAGE_UPDATED,ROW_UPDATED,UPD_LOCK,NO_CPN,\
REC_PTR_VALID"

    i4        	    rcb_sp_id;              /* Savepoint this RCB is associated
                                            ** with.  When abort to savepoint
                                            ** happens all RCB's initialized
                                            ** after that savepoint must be
                                            ** freed. */
    i4        	    rcb_p_flag;
#define                 RCB_P_LAST	    1L
#define                 RCB_P_ALL	    2L 
#define                 RCB_P_QUAL          3L
#define                 RCB_P_BYTID         4L
#define                 RCB_P_QUAL_NEXT     5L
#define                 RCB_P_ENDQUAL       6L
    i4        	    rcb_p_lk_type;
    i4        	    rcb_p_hk_type;
#define			RCB_P_LTE	    1L
#define			RCB_P_EQ	    2L
#define                 RCB_P_GTE           3L

    DM_TID         rcb_p_lowtid;            /* The record (tuple) identifier
                                            ** used to start a retrieve scan.
                                            ** This is kept in case a 
                                            ** reposition is requested. */
    DM_TID         rcb_p_hightid;           /* The record (tuple) identifier
                                            ** used to determine the end of a 
                                            ** retrieve scan.  This is always
                                            ** exactly the last qualifying 
                                            ** record. Kept for reposition. */
    DM_TID         rcb_lowtid;              /* The record (tuple) identifier
                                            ** used to start a retrieve scan.
                                            ** This is incremented as reecords
                                            ** are returned.  It is always one
                                            ** less than the start value, since
                                            ** a get always increments this 
                                            ** before reading the record. 
                                            ** For BTREE this is an identifier 
                                            ** of the index record. */   
    DM_TID         rcb_hightid;             /* The record (tuple) identifier
                                            ** used to determine the end of a 
                                            ** retrieve scan.  This is always
                                            ** exactly the last qualifying 
                                            ** record. 
                                            ** For BTREE this is an identifier 
                                            ** of the index record. */   
    DM_TID          rcb_currenttid;         /* This is the identifier of the
                                            ** record returned. */
    DM_TID          rcb_fetchtid;           /* This is the identifier of the
                                            ** where a replaced record 
                                            ** moved so multiple replaces
                                            ** without a get next can occur
					    ** for cursors. Cleared by 
                                            ** get next. */

    /* See the DMR_CB definition for more details on the qualification
    ** function and how it's called.
    */
    PTR		    *rcb_f_rowaddr;	    /* Address of row to be qualified
					    ** is stashed here before calling
					    ** the qual function. */

    DB_STATUS	    (*rcb_f_qual)(void *,void *);
					    /* Function to call to qualify
					    ** some particular row. */

    void	    *rcb_f_arg;		    /* Argument to qualifcation function. */
    i4		    *rcb_f_retval;	    /* Qual func sets *retval to
					    ** ADE_TRUE if row qualifies */

    char            *rcb_record_ptr;        /* Pointer to area in this RCB 
                                            ** used to hold a record. */
    char            *rcb_srecord_ptr;       /* Pointer to area in this RCB
                                            ** used to create keys, compress
                                            ** a record, etc. */
    char	    *rcb_hl_ptr;	    /* Key used for scans. */
    i4	    	    rcb_hl_given;	    /* Number of key fields given. */
    i4	    	    rcb_hl_op_type;	    /* Type of comparison 
                                            ** for high key. */
#define			RCB_LTE		    1L
#define			RCB_EQ		    2L
    char	    *rcb_ll_ptr;	    /*	Low key for scans. */
    i4	    	    rcb_ll_given;	    /*	Number fo key fields given. */
					    /*  For isam, if this is set,
					    **  we test lower limit for
					    **  each record we GET.
					    **  For BTREE, if this is set,
					    **  we use it for REPOSITION 
					    **  logic, and for lower key
					    **  limit defined when scanning
					    **  BACKWARDS
					    */
    i4	    	    rcb_ll_op_type;	    /*	Type of comparison 
                                            **  for low key. */
#define			RCB_GTE		    3L
#define			RCB_OVERLAPS	    4L	/* Rtree overlaps search */
#define			RCB_INTERSECTS	    5L	/* Rtree intersects search */
#define			RCB_INSIDE	    6L	/* Rtree is inside search */
#define			RCB_CONTAINS	    7L	/* Rtree contains search */
    char	    *rcb_s_key_ptr;	    /* Scratch key area. */
    i4         	    *rcb_uiptr;             /* Pointer to a i4 which
                                            ** indicates if any interrupt
                                            ** event has occurred.
					    */
#define                 RCB_USER_INTR	    0x01L /* must = SCB_USER_INTR */
#define                 RCB_FORCE_ABORT	    0x08L /* must = SCB_RCB_FABORT */			    
#define			RCB_RAHEAD_INTR     0x04L /* for readahead rcbs only */	
    i4         	    rcb_uidefault;          /* Area which user interrupt pointer
                                            ** can point to when rcb does not
                                            ** want to monitor interrupts. */
    DMP_MISC	    *rcb_rnl_cb;	    /* Pointer to MISC CB containing
					    ** the readnolock pages. */
    DMPP_PAGE	    *rcb_1rnl_page;	    /* Pointer to first rnl page. */
    DMPP_PAGE	    *rcb_2rnl_page;	    /* Pointer to 2nd rnl page. */
    DMPP_PAGE	    *rcb_3rnl_page;	    /* Pointer to 3rd rnl page */
    i4	    	    rcb_hash_nofull;	    /* Last primary page visited that
					    ** was not marked FULLCHAIN.  This
					    ** is used for optimizing the
					    ** reseting of CHAINFULL status
					    ** following a delete. */
    DMP_RNL_ONLINE  *rcb_rnl_online;	    /* For online operations */
    PTR		    rcb_collation;	    /* collation descriptor */
    PTR		    rcb_ucollation;	    /* Unicode collation descriptor */
    PTR		    rcb_gateway_rsb;	    /* Gateway record control block -
					    ** used if this is an open gateway
					    ** table. */
    ADF_CB	    *rcb_adf_cb;	    /* Pointer to ADF cb specific to
					    ** this RCB, for ADF exceptions,
					    ** etc. */
    char            *rcb_repos_key_ptr;     /* BTREE leaf key (RCB_P_GET)   */
    char            *rcb_fet_key_ptr;       /* BTREE leaf key (RCB_P_FETCH) */

#define			RCB_P_MIN	0L
#define                 RCB_P_START     0L   /* start position */
#define                 RCB_P_GET       1L   /* get position */
#define                 RCB_P_FETCH     2L   /* fetch/delete/replace position */
#define                 RCB_P_ALLOC     3L   /* alloc position */
#define                 RCB_P_TEMP	4L   /* temp position */
#define			RCB_P_MAX	4L
    DMP_POS_INFO rcb_pos_info[RCB_P_MAX+1];  /* for reposition to btree key */

    DM_TID		rcb_alloc_tidp;	    /* during allocate */


    LG_LSN          rcb_oldest_lsn;         /* first lsn of the oldest active 
					    ** transaction
					    */
    DMP_RCB	    *rep_shad_rcb;	    /* RCB for replicator shadow tab */
    DMP_RCB	    *rep_arch_rcb;	    /* RCB for replicator archive tab */
    DMP_RCB	    *rep_shadidx_rcb;	    /* RCB for replicator shadow
					    ** index */
    DMP_RCB	    *rep_prio_rcb;	    /* RCB for replicator priority
					    ** lookup table */
    DMP_RCB	    *rep_cdds_rcb;	    /* RCB for replicator cdds lookup
					    ** table */
    PTR		    rcb_tupbuf;		    /* tuple buffer for compression */
    PTR		    rcb_segbuf;		    /* buffer when 
					    ** DMPP_VPT_PAGE_HAS_SEGMENTS */
    PTR             rcb_hashbuf;            /* buffer used in dm1h_hash, 
                                            ** dm1h_keyhash, dm1h_newhash, 
                                            ** dm1h_hash_uchar
                                            */
#define HASH_BUFLEN (DB_MAXSTRING + DB_CNTSIZE + 2)
                                            /* size of rcb_hashbuf to be 
                                            ** allocated on either dm1h_hash,
                                            ** dm1h_newhash, dm1h_keyhash or
                                            ** dm1h_hash_uchar
                                            */
    i4 	    	    rcb_val_logkey;         /* rcb_logkey is a valid logkey 
					    ** assigned by an insert if either
					    ** RCB_TABKEY or RCB_OBJKEY is
					    ** asserted.
					    */
#define                 RCB_TABKEY	    0x01L			    
					    /* asserted if a table_key was
					    ** assigned by the current query.
					    */
#define                 RCB_OBJKEY	    0x02L			    
					    /* asserted if an object_key was
					    ** assigned by the current query.
					    */
    DB_OBJ_LOGKEY_INTERNAL rcb_logkey;      /* logical key last assigned - to
					    ** be used to pass this info up
					    ** through the interface and 
					    ** eventually to the client which
					    ** caused the insert.
					    */
    i4	    	    rcb_ra_idxupd;	    /* Bitmap of indexes that were
					    ** affected by the previous update.
					    ** Used for prefetch calculation.
					    */
    DMP_RCB	    *rcb_updstats_rcb;	    /* set by DMR_POSITION to point
					    ** either to this rcb, or to one
					    ** passed in to DMR_POSITION, which
					    ** would be the rcb used by QEF
					    ** just for updating table. Needed
					    ** for sec indx prefetch calc.
					    */
    i4         	    rcb_row_version;        /* Row version number of last record
					    ** retreived.  Needed primarily for
					    ** 'replace' operations where a 
					    ** check is required to see if the 
					    ** row being updated (replaced) is
					    ** added or dropped columns that may
					    ** alter the record length.
					    */

    i4         	    rcb_csrr_flags;         /* CursorStability,RepeatableRead */
#define RCB_CS_LEAF      		0x01
#define RCB_CS_DATA      		0x02
#define RCB_CS_ROW       		0x04
#define RCB_CS_BASEDATA  		0x08
/* String values of above bits */
#define	RCB_CSRR_FLAGS	"\
LEAF,DATA,ROW,BASEDATA"

    DB_TAB_ID	    *rcb_reltid;	    /* Override tcb_rel.relid
					    ** with this table id
					    ** in dm0p_trans_page_lock
					    ** when DM0P_RCB_RELTID
					    ** is set */
    i4	    	    rcb_partno;		    /* A place to store a
					    ** partition number */
    DM_PAGENO       rcb_locked_data;
    LK_LKID         rcb_data_lkid;          /* data page's lock id */

    DM_PAGENO       rcb_locked_leaf;
    LK_LKID         rcb_leaf_lkid;          /* leaf page's lock id */

    DM_TID          rcb_locked_tid;         /* tid of locked row */
    DB_TAB_ID	    rcb_locked_tid_reltid;  /* ...and its reltid */
    LK_LKID         rcb_row_lkid;           /* lock id of current row */
    LK_LKID         rcb_base_lkid;     	    /* lock id of intended lock
                                            ** acquired on base table data
                                            ** page when reading 2nd-ry index
                                            */
    LK_LKID         rcb_fix_lkid;           /* lock id of last page locked
					    ** by dm0p_fix_page()
					    */

    LK_LKID	    rcb_val_lkid;	    /* A unique identifier to the
					    ** to store the value lock id    
					    */
    i4		    rcb_dmr_opcode;	    /* The DMR_? operation currently
    					    ** using this RCB
					    */
    DM_TID	    rcb_crow_tid;	    /* TID of last crow locked row */
    LK_LKID	    rcb_mvcc_lkid;	    /* lock id of TBL_MVCC lock, if any */
    LG_CRIB	    *rcb_crib_ptr;	    /* This RCB's CRIB */

/*
**  Section for Rtree.
*/
                        /* Maximum levels of Rtree index:
                        ** for example, a 4-byte TID can address at most
                        ** 2**32 tuples, and each index level can hold at
                        ** least 2**6 entries, so (4 * 8 + 5)/6 = 37/6 = 6. */
#define                 RCB_MAX_RTREE_LEVEL ((sizeof(DM_TID) * 8 + 5) / 6)

    DM_TID	    rcb_ancestors[RCB_MAX_RTREE_LEVEL];
					    /* Array of TIDPs - one per index
					    ** level - to save position. */
    i4		    rcb_ancestor_level;	    /* Current ancestor level. */

/*
** Tuple and page count deltas, rolled into TCB when RCB is released,
** unless transaction is aborted.
*/
    i4	    rcb_tup_adds;	    /* Count of tuples added/removed */
    i4	    rcb_page_adds;	    /* Count of pages added/removed */
    i4	    rcb_reltups;	    /* Tuples in table when RCB alloc'd */
    i4	    rcb_relpages;	    /* Pages in table when RCB alloc'd */

/*
**  Statistics for an OPEN RCB.  These statistics counters are maintained
**  for the lifetime of an open RCB.  Leave these at the end.
*/
    i4	    rcb_s_fix;		    /* Fix page calls. */
    i4	    rcb_s_io;		    /* Read/Write page calls. */
    i4	    rcb_s_rep;		    /* Replace calls. */
    i4	    rcb_s_ins;		    /* Insert calls. */
    i4	    rcb_s_del;		    /* Delete calls. */
    i4	    rcb_s_get;		    /* Tuples returned. */
    i4	    rcb_s_qual;		    /* Tuples qualified. */
    i4	    rcb_s_scan;		    /* Tuples scanned. */
    i4	    rcb_s_load;		    /* Load calls. */
    PTR		    rcb_pcb;		    /* link to any peripheral ops*/
    /* Stuff used to sync dmrAgents, siAgents, dm2r */
    DMP_RCB	*rcb_base_rcb;	    /* SI pointer to indexed RCB */
    PTR		rcb_rtb_ptr;	    /* If threaded, thread's RTB */
    DM_TID	rcb_si_oldtid;	    /* Base oldTID for SI */
    DM_TID	rcb_si_newtid;	    /* Base newTID for SI */
    i4		rcb_si_flags;	    /* Various flags: */
#define		RCB_SI_UPDATE		0x0001 /* This RCB used exclusively
					       ** for SI updates */
#define		RCB_SI_BUSY		0x0002		
#define		RCB_SI_DUP_PRECHECK	0x0004
#define		RCB_SI_DUP_CHECKED	0x0008
#define		RCB_SI_VALUE_LOCKED	0x0010
/* String values of above bits */
#define	RCB_SI_FLAGS	"\
UPDATE,BUSY,DUP_PRECHECK,DUP_CHECKED,VALUE_LOCKED"
    i4		rcb_compare;	    /* Result of security check */
    /* Following fields are used by dm2r_replace/si_replace */
    i4		rcb_change_set[(DB_MAX_COLS+31) / 32];
    i4		rcb_value_set[(DB_MAX_COLS+31) / 32];
};

/*
** Name: DMP_TABLE_IO - Table IO Control Block
**
** Description:
**	This structure contains information needed to perform IO operations
**	on a table.  It is normally contained within a TCB and groups together
**	those table control block fields which are needed for IO actions.
**
**	The control block is used by low-level portions of the system (mainly
**	the buffer manager) which need to perform IO actions but which do
**	not wish to know about logical attributes of the table contained in
**	the TCB.
**
** History:
**      10-sep-1992 (rogerk)
**          6.5 Recovery Project: Created to collect together the TCB fields
**	    needed by the buffer manager so that a smaller structure than
**	    a TCB could be passed to fix/unfix page routines.
**	20-jun-1994 (jnash)
**	    Add tbio_sync_close, used to request async writes and that
**	    sync be performed during dm2t_close.
**	06-mar-1996 (stial01 for bryanp)
**	    Add tbio_page_size to the DMP_TABLE_IO structure.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced extra tbio_cache_flag TBIO_CACHE_DMCM to indicate
**          that Distributed Multi-Cache Management (DMCM) protocol is
**          in use and that other buffer manager protocols will be
**          sidestepped.
**	24-Feb-1997 (jenjo02 for somsa01)
**	    Cross-integrated 428939 from main.
**	    Added tbio_extended, which is set if the table is an extended
**	    table.
**	07-May-1998 (jenjo02)
**	    Added tbio_cache_ix to TBIO.
**	09-Oct-2007 (jonj)
**	    Add TBIO_STATUS, TBIO_CACHE_FLAGS defines.
**	1-Sep-2008 (kibro01) b120843
**	    Add status for being in recovery so we can ignore a flush of a
**	    page which was beyond the end of the physical file due to UNDO.
[@history_template@]...
*/
struct _DMP_TABLE_IO
{
    PTR			tbio_q_next;		/* Not Used */
    PTR			tbio_q_prev;		/* Not Used */
    i4			tbio_length;		/* Not Used */
    i2			tbio_type;
#define			    TBIO_CB		DM_TBIO_CB
    i2			tbio_reserved;		/* Not Used */
    i4			tbio_status;		/* Status information */
#define			    TBIO_VALID		0x0001
#define			    TBIO_OPEN		0x0002
						    /* Table Control Block
						    ** has been opened. */
#define			    TBIO_PARTIAL_CB	0x0004
						    /* Not all files in the
						    ** location array are
						    ** open and accessable.  */
/* String values of above bits */
#define	TBIO_STATUS	"\
VALID,OPEN,PARTIAL_CB"

    i4			tbio_cache_valid_stamp; /* TCB validation value which
						** is used to correlate this
						** version of the table info
						** and file descriptors with the
						** most recent known state. */
    i4			tbio_loc_count;		/* Count of locations which
						** make up table (and the number
						** of entries in the following
						** array. */
    DMP_LOCATION	*tbio_location_array;	/* Array of location descriptors
						** which make up the table's
						** underlying files. */
    i4			tbio_lalloc;		/* Number of pages allocated
						** to the table.  It is updated
						** from dm2f_sense for normal 
						** files and managed internally
						** for non-shared temp files. */
    i4			tbio_cache_priority;	/* Table's cache priority */
    i4			tbio_lpageno;		/* High water mark - this is 
						** what we believe to be the
						** highest numbered formatted
						** pg in the table. It reflects
						** the true value in the FHDR
						** page and is updated only
						** periodically. */
    i4	    		tbio_tmp_hw_pageno;     /* Highest pageno ever written
					    	** to disc for this temporary
					    	** table. Needs to be 
						** initialised to -1 as zero
						** is a valid value */

    /*
    ** Fields which hold copies of table/database information used 
    ** by low level Buffer Manager modules.
    */
    i4			tbio_dcbid;		/* Database Id */
    DB_TAB_ID		tbio_reltid;		/* Table Id */
    i4			tbio_table_type;	/* Table storage structure */
    i4			tbio_page_type;         /* Table's page type */
    i4			tbio_page_size;		/* Table's page size */
    i4			tbio_cache_ix;		/* Table's cache index */
    i4			tbio_cache_flags;	/* Buffer manager protocol
						** flags for this database */
#define			    TBIO_CACHE_READONLY_DB	0x0001
#define			    TBIO_CACHE_FASTCOMMIT	0x0002
#define			    TBIO_CACHE_MULTIPLE		0x0004
#define			    TBIO_CACHE_RCP_RECOVERY	0x0008
#define			    TBIO_CACHE_ROLLDB_RECOVERY	0x0010
#define             	    TBIO_CACHE_NO_GROUPREAD     0x0020
#define                     TBIO_CACHE_DMCM             0x0040
/* String values of above bits */
#define	TBIO_CACHE_FLAGS	"\
READONLY,FASTCOMMIT,MULTIPLE,RCP_RECOVERY,ROLLDB_RECOVERY,NO_GROUPREAD,DMCM"

    BITFLD		tbio_temporary:1;	/* Table is a temporary table */
    BITFLD		tbio_sysrel:1;		/* Table is a core catalog */
    BITFLD		tbio_checkeof:1;	/* Indicates that we have done
						** or attempted a read past
						** our current lpageno value.
						** We need to update lpageno
						** on the next table open. */
    BITFLD              tbio_sync_close:1;      /* Perform async i/o, sync
						** on dm2t_close. */
    BITFLD              tbio_extended:1;        /* Table is an extended table */
    BITFLD		tbio_in_recovery:1;	/* Database is being recovered*/
    BITFLD		tbio_bits_free:26;	/* Reserved for future use. */

    /*
    ** Fields which hold copies of table/database information used 
    ** so the buffer manager can give more useful debug and trace messages.
    */
    DB_DB_NAME		*tbio_dbname;		/* Pointer to Database name */
    DB_TAB_NAME		*tbio_relid;		/* Pointer to Table name */
    DB_OWN_NAME		*tbio_relowner;		/* Pointer to Table Owner */
};

/*
**  TCB - SECTION
**
**       This section defines all structures needed to build
**       and access the information in a Table Control Block (TCB).
**       The following structs are defined:
**           
**           DMP_ATTRIBUTE           Attribute record structure.
**           DB_ATTS                Attribute information stored in TCB.
**           DMP_DEICE               Device record structure.
**	     DMP_ETAB_CATALOG	     Extended Table Catalog record structure
**	     DMP_ET_LIST	     List of known extended tables.
**           DMP_INDEX               Index record structure.
**           DMP_RELATION            Relation record structure.
**           DMP_RINDEX              Index record structure.
**           DMP_TCB                 Table Control Block.
**	     DMP_REP_INFO	     Replication information for table 
**					(dd_regist_tables record)
**           DMP_RANGE               Range record structure.
**
*/

/*}
** Name:  DMP_TCB - DMF internal control block for database table information.
**
** Description:
**      This typedef defines the control block required to keep information
**      about a table that is not associated with an open of the table, but
**      remains the same regardless of the number of times it is opened by 
**      one or more sessions (server-threads).  
**      
**      Most of the information in the TCB comes from the three main system
**      tables: RELATION table, ATTRIBUTE table, and INDEX table.  For each
**      table in a database there is a record in the RELATION table describing
**      the structure of the table and the file.  This record is included
**      in a TCB.  For each attribute of a table there is a record in the 
**      the ATTRIBUTE table describing the characteristics of that attribute.
**      Information from each of these records is included in the variable
**      portion of the TCB.  This information is also used to build the
**      primary key information required  for keyed searches.
**      For each secondary index of a table there is a record in the INDEX 
**      table describing which attributes of the base table are used to 
**      create the index key.  A TCB used to describe an index table will 
**      contain information from the index table, TCB's used to describe base 
**      tables does not contain this information.
**      If a table resides on multiple locations, then the IIDEVICE table
**      contains information describing each additional locations for
**      this table.  If a table resides at only one locations this 
**      inforamtion is contained in the relation record.  Only additional
**      locations are recorded in the IIDEVICE table.
**
**      Other information contained in the TCB includes such things as the 
**      counts of the number of tuples or pages added while it is cached 
**      (note these are accumulative for all threads of a server and are 
**      only updated periodically), the current state of TCB,
**      a queue of associated index TCB's, and many pointers to associated
**      DMF internal control blocks, such as Record Access Control Block
**      (RCB) used for each open of a table, the File Control Block (FCB)
**      used to open the physical file associated with a table, and the
**      Database Control Block(DCB) used to identify which database this
**      TCB belongs to.
**
**      When a TCB is allocated for a table, all associated control blocks
**      are allocated and initialized at the same time.  This means that all
**      TCB's for all index tables associated with a base table are cached
**      with the base TCB.  Index table TCB's are not queued directly to
**      hash buckets, but are known only through the base TCB.  Partitions of
**	a partitioned table are handled differently;  while the entire PP array
**	is initialized with the master TCB, partition TCB's are only opened as
**	needed.  Partition TCB's are hashed independently of the master TCB.
**
**      The Table Control Block (TCB) is cached by the DMF.  To be able to 
**      rapidly determine if a TCB exists for a specified table, a hashing 
**      scheme exists to minimize the amount of time needed to locate this TCB.
**      Each table has a unique identifier, nameley (database_id,table_id).  
**      The table-id for all tables is in two parts:
**
**          table_id.base   (unique identifier for base table)
**          table_id.index  (zero for base tables, +unique identifier
**                           for index tables, id | sign-bit for partitions).

**
**      Since index tables are always associated with their base tables
**      (e.g. all table locks are obtained on the base table, not the index
**      table even if the index table is opened directly), The TCB caching 
**      scheme uses the base ID and partition ID, but not any index ID.
**      Each server has a TCB hash array pointed to be the Server Control
**      Block (SVCB).  The number of buckets in the hash array is also stored
**      in the SVCB.  The TCB identifier is hashed (divided by number of
**      buckets) to determine which bucket it would be queued to if it existed.
**      The bucket queue is searched matching on (database_id,table_id).
**      If it is found, it is checked to determine if the information is
**      still accurate, this is done by checking a value lock (valid stamp)
**      obtained when the TCB was built.  If it is still good, it can be
**      used, otherwise it must be thrown away since a major change to the 
**      table occurred, such as deleted, modified, etc. while this server 
**      had the TCB chached but was not using it.  
**
**	Partitioned tables introduce their own set of complications.
**	The partitioned table itself is called the master, or Partitioned
**	table.  The actual partitions carrying the data are the partition
**	tables.  The master table must always be opened if a partition is
**	open, as only the master carries iiattribute rows.  The master
**	TCB includes a physical-partition array, often abbreviated to
**	the PP array.  This array is indexed by physical partition number
**	and maps partitions to their partition table tabids.  Each
**	partition TCB points back to the master.  A master or non-partitioned
**	table points to itself.  As mentioned above, partitions share the
**	master table base ID, but use an "index" ID with the sign-bit set
**	as well.  Sharing the master base ID a) makes it trivial for a
**	partition to associate itself to its master, and b) makes it easy
**	to escalate from partition page-or-row locking to master table-level
**	locks.
**
**      Many TCB fields are protected against multi-thread updates by
**      the TCB's hash mutex, rather than by a TCB specific mutex.
**      (Part of the reason for this is that many relevant status bits are
**      only in the master/base TCB, and the hash mutex protects all partition
**      and index TCB's simultaneously.)  Some fields specifically not
**      alterable without holding the hash mutex are:
**          tcb_status
**          tcb_ref_count
**          tcb_q_next and _prev if the TCB is on a hash chain
**      Note that this is not an exhaustive list.
**
**      A pictorial representation of this scheme is:
**   
**           SVCB
**       +--------------+        Hashed by (database-id,table_id.base)
**       |              |        +--------+--------+      +--------+
**       |              |        | TCB Q  | TCB Q  |      | TCB Q  |
**       |TCB_HASH_ARRAY|------->|  |     |        | .....|        |
**       |TCB_HASH_COUNT|        |bucket 1|bucket 2|      |bucket n|
**       |              |        +--|-----+--------+      +--------+
**       +--------------+           |
**                                  +-->+----------+
**                                      |          |
**                                      |   TCB    |
**                                      |  (base)  |
**                                      |          |   
**                                      | Index Q  |----->+----------+
**                                      +----------+      |          |
**                                                        |   TCB    |
**                                                        | (index)  |
**                                                        |          |   
**                                                        +----------+ 
**                                       

**     The TCB layout is as follows:
**                                                        
**                     +----------------------------------+
**     Byte            | tcb_q_next                       |  Initial bytes are
**                     | tcb_q_prev                       |  standard DMF CB
**                     | tcb_length                       |  header. 
**                     | tcb_type    | tcb_s_reserved     |
**                     | tcb_l_reserved                   |
**                     | tcb_owner                        |
**                     | tcb_ascii_id                     |
**                     |------ End of Header -------------|
**                     | tcb_unique_id                    |
**                     | tcb_dcb_ptr                      |
**                     | tcb_mutex                        |
**                     | tcb_hash_bucket_ptr              |
**                     | tcb_ref_count                    |
**                     | tcb_iq_next                      |
**                     | tcb_iq_prev                      |
**                     | tcb_fq_next                      |
**                     | tcb_fq_prev                      |
**                     | tcb_rq_next                      |
**                     | tcb_rq_prev                      |
**                     | tcb_tq_next                      |
**                     | tcb_tq_prev                      |
**                     | tcb_parent_tcb_ptr               |
**                     |   bit fields:                    |
**                     |      tcb_temporary       1 bit   |
**                     |      tcb_sysrel          1 bit   |
**		       |      tcb_loadfile        1 bit   |
**                     |      tcb_logging         1 bit   |
**                     |      tcb_nofile          1 bit   |
**                     |      tcb_update_idx      1 bit   |
**                     |      tcb_no_tids         1 bit   |
**                     |      tcb_replicate       1 bit   |
**                     |      tcb_extended        1 bit   |   
**                     |      tcb_stmt_unique     1 bit   |   
**                     |      tcb_reptab          1 bit   |
**                     |      tcb_dups_on_ovfl    1 bit   |
**                     |      tcb_rng_has_nonkey  1 bit   |
**                     |      tcb_unique_index    1 bit   |
**                     |      tcb_bits_free      18 bits  |
**                     | tcb_index_count                  |
**                     | tcb_ikey_map                     |
**                     | tcb_lk_id                        |
**                     | tcb_valid_stamp                  |
**                     | tcb_tup_adds                     |
**                     | tcb_page_adds                    |
**                     | tcb_keys                         |
**                     | tcb_att_key_ptr                  |
**                     | tcb_klen                         |
**                     | tcb_kperpage                     |
**		       | tcb_key_atts                     |
**                     | tcb_atts_ptr                     |
**                     | tcb_lct_ptr                      |
**                     | tcb_lpageno		          |
**                     | tcb_lalloc			  |
**		       | tcb_tmp_hw_pageno		  |
**                     | tcb_location_array               |
**		       | tcb_loc_count                    |
**		       | tcb_status                       |
**		       | tcb_high_logical_key		  |
**		       | tcb_low_logical_key		  |
**                     | tcb_table_type                   |
**                     | tcb_bmpriority                   |
**		       | tcb_et_mutex			  |
**		       | tcb_et_list			  |
**                     | tcb_acc_plv			  |
**		       | ... lots more stuff ...          |
**                     | tcb_rel (relation record)        |
**                     |----- End of Fixed Portion -------| 
**                     |  Numerous optionally allocated   |
**                     | or variable size areas ...       |
**                     | FIXME diagram them properly!     |
**		       | Old diagram wrong, removed...    |
**                     |----------------------------------|
**		       | ... etc ...                      |
**                     +----------------------------------+
**		       | PP array if TCB is a partitioned |
**		       | master TCB                       |
**		       +----------------------------------+
**                   
**                     
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**      16-nov-87 (jennifer)
**          Added mulitple locations per table support.
**      19-Oct-88 (teg)
**	    chgd relstat constants to hex and put documented their meaning,
**	    added TCB_VIEW.
**	10-Feb-89 (rogerk)
**	    Added tcb_status field in order to mark TCB busy while being
**	    built or released.  This way we can avoid requiring a lock
**	    in order to release a tcb.
**	06-mar-89 (neil)
**          Rules: Added TCB_RULE.
**	10-apr-89 (mikem)
**	    Logical Key project.  
**		Added tcb_{high,low}_logical_key fields to keep track of an 8 
**		byte unique id.  Also added a new relstat status bit 
**	        TCB_SYS_MAINTAINED.
**      20-jun-89 (jennifer)
**          Security development.  Added TCB_SECURE to relstat bit.
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	    Added tcb_logging, tcb_logical_log, tcb_update_idx, tcb_nofile,
**	    tcb_no_tids bitfields.  These are setup when the tcb is built and
**	    describe attributes of the table to many of the dmf routines.  In
**	    this way, attributes - such as whether a table should be logged, or
**	    whether TIDS are supported - are decided at a single point, rather
**	    than at many places in the code.
**	    Added tcb_table_type which is used to decide which record level
**	    routines to call for the table.  For normal Ingres tables, the
**	    table type is set to the storage structure - thus the appropriate
**	    record level routine for that structure is used.  For special
**	    tables, such as gateway tables, this table type is set to something
**	    different.  Can't just use the storage structure since some special
**	    tables also have storage structure types.
**	23-jan-1990 (fred)
**	    Added large object support.  Added a bunch more bit fields to
**	    indicate various parameters to other parts of the code.  This should
**	    help by figuring out these things once.
**	31-jan-90 (teg)
**	    added TCB_COMMENT and TCB_SYNONYM relstat bits for the comment text
**	    and synonym projects.
**	02-mar-90 (andre)
**	    defined TCB_VGRANT_OK for the FIPS project.
**	18-jun-1990 (linda, bryanp)
**	    Added TCB_GW_NOUPDT flag value for relstat field, to enforce the
**	    default non-updateability of gateway tables.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.  Moved over DMT_GW_NOUPDT relstat
**	    value from 6.3 code.  Had to change value for DMT_SECURE relstat
**	    value because it clashed with GW_NOUPDT.
**	29-apr-1991 (Derek)
**	    Added new DMP_RELATION fields for allocation project.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**
**	     4-feb-1991 (rogerk)
**		Added support for fixed cache priorities.
**		Added tcb_bmpriority to show table's cache priority.
**	    25-mar-1991 (bryanp)
**		New field tcb_data_atts to point to a list of pointers to data
**		attributes in the table. Either tcb_data_atts or tcb_key_atts
**		should be used when performing data or key compression.
**		New relstat bit, TCB_INDEX_COMP, to indicate that a table uses
**		index compression. 
**		New field tcb_comp_katts_count to store the adjustment count of
**		worst case overhead added by index compression to index entry
**		of the table.
**	15-nov-1991 (fred)
**	    Added relstat2 field to tcb_rel.  This is a temporary fix until the
**	    real project to add it is done.  Peripheral datatype/extension table
**	    support is indicated (currently only) in this field.  Bit mask names
**	    changed to make sure we don't have screw ups...  They are now called
**	    TCB2_HAS_EXTENSIONS and TCB2_EXTENSION to indicate whether a table
**	    has extension tables or is one, respectively.
**	18-feb-1992 (bryanp)
**	    Temporary table enhancements: Added TCB_PEND_DESTROY flag .
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project. 
**	    Increased size of relfree field in DMP_RELATION to 64 bytes.
**	05-jun-92 (kwatts)
**	    Added the MPF fields in DMP_RELATION, relcomptype and 
**	    relpgtype. Added tcb_acc_plv and tcb_acc_tlv to TCB structure.
**	28-jul-92 (rickh)
**	    Carved relstat2 out of 4 relfree bytes.  This is where FIPS
**	    CONSTRAINTS keeps its tasty bits.
**	10-sep-92 (rogerk)
**	    Added support for partial TCB's.  Added TCB_PARTIAL tcb status
**	    to indicate that a TCB may not have all its underlying files
**	    open and may not contain sufficient logical information to
**	    allow logical table access.
**	    Added tcb_table_io control block which contains information
**	    necessary for IO operations.  Moved existing tcb_loc_count,
**	    tcb_location_array, tcb_lalloc, tcb_lpageno, tcb_bmpriority,
**	    tcb_checkeof and tcb_valid_stamp into the table_io control block.
**	    Added new tcb_valid_count field to keep count of the number of
**	    tcb fixes which verified the correctness of the tcb at fix time.
**	    Added new tcb_status wait types which indicate the type of wait
**	    operation being performed when clients wait for other than TCB_BUSY
**	    status.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Added new page type for system catalogs,
**	    TCB_PG_SYSCAT.  Added status for tcb's that have encountered
**	    close errors, TCB_CLOSE_ERROR.
**	9-nov-92 (ed)
**	    removed 24 bytes from relfree for DB_MAXNAME name increases
**	18-jan-1993 (bryanp)
**	    Added TCB_TMPMOD_WAIT to fix temporary table modify bug.
**	16-feb-1993 (rogerk)
**	    Added tcb_open_count to count the number of open RCB's on the table.
**	15-may-1993 (rmuth)
**	    Add relstat2 bits for the following:
**		- to indicate that a table is READONLY
**		- If table is an index to indicate that it is being built 
**		  in "CONCURRENT_ACCESS" mode.
**	    of table.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	18-oct-1993 (rogerk)
**	    Removed PEND_DESTROY tcb state.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**	8-dec-93 (robf)
**          Added TCB_ALARM for security alarms on tables.
**     20-jun-1994 (jnash)
**          Eliminate obsolete tcb_logical_log, bumped free bit count.
**      14-sep-1994 (andyw)
**          Partial Backup/Recovery Project:
**              Added New bitflags to relstat2:
**                  TCB2_PHYS_INCONSISTENT
**                  TCB2_LOGICAL_INCONSISTENT
**                  TCB2_TBL_RECOVERY_DISALLOWED
**      7-dec-1994 (andyw)
**          Added comment for changes made by Steve Wonderly and
**          Roger Sheets
**              added TCB_C_HICOMPRESS  to the DMP_RELATION structure
**              to handle new compression algorithms.
**	06-mar-1996 (stial01 for bryanp)
**	    Add relpgsize to iirelation; reduce relfree accordingly.
**	    Added DMP_NUM_IIRELATION_ATTS to dimension the dm2umod.c array.
**	17-may-96 (stephenb)
**	    Add tcb_replicate and tcb_rep_info fields to support DBMS 
**	    replication.
**      22-jul-1996 (ramra01 for bryanp)
**          Add relversion, reltotwid to iirelation; reduce relfree 
**	    accordingly.
**          Define TCB2_ALTERED to track when a table is altered.
**	23-aug-96 (nick)
**	    Added TCB_INCONS_IDX - base table has inconsistent secondaries.
**     01-aug-1996 (nanpr01 for ICL phil.p)
**          Introduced new tcb_statuses of TCB_FLUSH and TCB_VALIDATED to
**          support TCB callbacks under the DMCM protocol.
**	21-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added reltcpri (Table Cache priority), reduced relfree,
**	    bumped DMP_NUM_IIRELATION_ATTS.
**      21-Apr-1998 (stial01)
**          Added tcb_extended for B90677
**	23-oct-1998 (somsa01)
**	    Added TCB_SESSION_TEMP as an option for relstat.
**	 3-mar-1999 (hayke02)
**	    Move TCB_SESSION_TEMP from relstat to relstat2 to avoid overflow
**	    of i4 (signed 4 byte int).
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint. tcb_iirel_lsn keeps the lsn from the
**	    the iirelation page when TCB was built. 
**      17-Apr-2001 (horda03) Bug 104402
**          Added TCB_NOT_UNIQUE
**	04-May-2000 (jenjo02)
**	    Added tcb_relid_len, tcb_relowner_len so that logging and recovery
**	    don't have to constantly strip trailing blanks to compute
** 	    these lengths.
**	08-Nov-2000 (jenjo02)
**	    Added DML_XCCB *tcb_temp_xccb, pointer to pending destroy 
**	    XCCB if XCB_SESSION_TEMP temporary table.
**	18-Dec-2003 (schka24)
**	    New iirelation columns and flags for partitioning.
**	12-Jan-2004 (schka24)
**	    More partitioning stuff for tracking reltups/relpages for
**	    partitioned tables.  Trim obsolete/wrong part of tcb picture.
**	22-Jan-2004 (jenjo02)
**	    For Parallel/Partitioning project, added tcb_stmt_unique flag,
**	    tcb_unique_count, tcb_rtb_next, tcb_partno.
**	5-Mar-2004 (schka24)
**	    Remove unused field I added earlier.
**	    Define tcb free-list as a real queue, not a fake one;  easier
**	    to read, and removes a dependency on tcb_q_next being first.
**	30-Jun-2004 (schka24)
**	    Unused relsecid -> relfree1.
**	15-Mar-2005 (schka24)
**	    Introduce show-only TCB status.
**	14-Apr-2006 (jenjo02)
**	    Added relstat TCB_CLUSTERED bit for Clustered Btrees.
**	30-May-2006 (jenjo02)
**	    tcb_ikey_map changed to DB_MAXIXATTS for indexes on
**	    clustered tables.
**	    Removed goofy DMP_NUM_IIRELATION_ATTS define. dm2u_modify
**	    allocates needed memory based on relatts rather than
**	    using the stack.
**	23-Jun-2006 (kschendel)
**	    Relnparts should have been unsigned to allow >32K partitions.
**	23-nov-2006 (dougi)
**	    Re-established TCB_VIEW as 0x40 in relstat. Flag was being
**	    used in iitables view, which may well have been used in
**	    user applications.
**      29-jan-2007 (huazh01)
**          Add TCB_REPTAB bit to indicate this is a replicator
**          specific DMP_TCB. 
**          This fixes bug 117355.
**	09-Oct-2007 (jonj)
**	    Add RELSTAT, RELSTAT2 defines.
**      01-may-2008 (horda03) bug 120349
**          Add TCB_STORAGE define.
**	14-Aug-2008 (kschendel) b120791
**	    Comment update only re mutexing
**	23-Oct-2009 (kschendel) SIR 122739
**	    Integrate rowaccessor changes.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Change relcmptlvl from i4 to u_i4, don't want signed compares.
**	    Note TCB_BINARY unused, may become TCB_CLUSTER.
*/
struct _DMP_TCB
{
    DMP_TCB         *tcb_q_next;            /* TCB hash queue for base 
                                            ** tables, and TCB index queue
                                            ** for index tables. */
    DMP_TCB         *tcb_q_prev;
    SIZE_TYPE	    tcb_length;		    /* Length of control block. */
    i2              tcb_type;               /* Type of control block for 
                                            ** memory manager. */
#define                 TCB_CB              DM_TCB_CB
    i2              tcb_s_reserved;         /* Reserved for future use. */
    PTR             tcb_l_reserved;         /* Reserved for future use. */
    PTR             tcb_owner;              /* Owner of control block for
                                            ** memory manager.  TCB will be
                                            ** owned by the server. */
    i4         	    tcb_ascii_id;           /* Ascii identifier for aiding
					    ** in debugging. */
#define                 TCB_ASCII_ID        CV_C_CONST_MACRO('#', 'T', 'C', 'B')
    u_i4       	    tcb_unique_id;          /* Unique identifier for use as
                                            ** a lock event value. */
    DMP_DCB         *tcb_dcb_ptr;           /* Pointer to database control
                                            ** block associated with this
                                            ** TCB. */
    DM_MUTEX	    tcb_mutex;		    /* Multi-thread synchronization. */
    DMP_HASH_ENTRY  *tcb_hash_bucket_ptr;   /* Pointer to hash bucket this 
                                            ** TCB is queued to. */
    DM_MUTEX	    *tcb_hash_mutex_ptr;    /* Pointer to hash mutex for this TCB */
    i4         	    tcb_ref_count;          /* Count of number of active fixers
                                            ** of this TCB.
                                            ** NOTE: ref-count may not be
                                            ** changed without holding the hash
                                            ** mutex for the TCB! */
    i4         	    tcb_valid_count;        /* Count of number of active fixers
                                            ** of this TCB which verified that
					    ** it was valid when fixed. If a fix
					    ** request is made on a TCB which 
					    ** has a non-zero valid_count then
					    ** no verify action needs be done */
    i4         	    tcb_open_count;	    /* Count of number of open rcb's */

    /* Note that these queues (whether real QUEUE objects or fake ones)
    ** point to the queue member, not to the start of the TCB.
    */
    DMP_TCB         *tcb_iq_next;           /* Queue of associated index table
                                            ** TCBs. */
    DMP_TCB         *tcb_iq_prev;
    DMP_RCB         *tcb_fq_next;           /* Queue of free record control
                                            ** blocks associated with this
                                            ** TCB. */
    DMP_RCB         *tcb_fq_prev;
    DMP_RCB         *tcb_rq_next;           /* Queue of active record control
                                            ** blocks associated with this
                                            ** TCB. */
    DMP_RCB         *tcb_rq_prev;
    QUEUE	    tcb_ftcb_list;	    /* Queue of free base TCBs
                                            ** which can be considered to
                                            ** be released when resources
                                            ** are low. Queue these to
                                            ** hcb->hcb_ftcb_list QUEUE. */
    DMP_TCB         *tcb_parent_tcb_ptr;    /* Pointer to parent (base table)
                                            ** TCB for an index TCB. */
    BITFLD          tcb_temporary:1;        /* Variable used to indicate if
                                            ** this TCB is associated with a
                                            ** temporary table. */
#define                 TCB_PERMANENT       0L
#define                 TCB_TEMPORARY       1L
    BITFLD          tcb_sysrel:1;           /* Variable used to indicate if 
                                            ** this TCB is a system relation
                                            ** requiring special locking. 
                                            ** Only relation table, attribute
                                            ** table and index table are 
                                            ** considered system relations. */
#define                 TCB_USER_REL        0L
#define                 TCB_SYSTEM_REL      1L
    BITFLD          tcb_loadfile:1;         /* Variable used to indicate if
                                            ** this TCB is associated with a
                                            ** load type table. */
#define                 TCB_NOLOAD          0L
#define                 TCB_LOAD            1L
    BITFLD	    tcb_logging:1;	    /* Table should be logged.  If not
					    ** set, then changes to this table
					    ** will not be backed out - this
					    ** should always be set except for
					    ** some special types of TCB's
					    ** (temp tables, views, gateways) */
    BITFLD	    tcb_nofile:1;	    /* Table has no underlying physical
					    ** file, true only for special tcb's
					    ** like views, gateways. */
    BITFLD	    tcb_update_idx:1;	    /* Table has secondary indexes which
					    ** must be updated when base is.
					    ** This is normally set if table
					    ** has secondary indexes, although
					    ** some special secondary indexes,
					    ** (gateways) are not managed by
					    ** Ingres and are not updated. */
    BITFLD	    tcb_no_tids:1;	    /* Table does not support tids. */
    BITFLD	    tcb_replicate:1;	    /* table is replicated */
#define			TCB_NOREP	    0L
#define			TCB_REPLICATE	    1L
    BITFLD          tcb_extended:1;         /* Table is an extended table */

    BITFLD          tcb_stmt_unique:1;      /* True if -any- SI are
					    ** STATEMENT_LEVEL_UNIQUE */
    BITFLD          tcb_reptab:1;           /* This is a replicator specific
                                            ** table, i.e., shadow, shadow idx,
                                            ** or archive table */    
#define                 TCB_REPTAB          1L
    BITFLD	    tcb_dups_on_ovfl:1;	    /* Btrees use overflow pages for
					    ** duplicates at the leaf level. */
    BITFLD	    tcb_rng_has_nonkey:1;   /* Btree leaf range keys include
					    ** non-key columns */
    BITFLD	    tcb_unique_index:1;	    /* Table has unique indexes */
    BITFLD	    tcb_seg_rows:1;	    /* Table has segmented rows */
    BITFLD          tcb_bits_free:17;       /* Reserved for future use. */

    LK_LKID	    tcb_lk_id;		    /* Holds the TCB lock used to
					    ** validate the contents of
					    ** the table control block. The
					    ** value of this lock is used
					    ** for the validation and is stored
					    ** in the table_io block below. */
    LK_LKID	    *tcb_lkid_ptr;	    /* Pointer to proper lock-id to use, which is:
					    ** this TCB's tcb_lk_id normally
					    ** base table's tcb_lk_id for indexes
					    ** master's tcb_lk_id for partitions */
    i4		    tcb_tup_adds;	    /* Accumulative count of tuples
                                            ** added by all users of this 
                                            ** TCB since it was allocated.
                                            ** This value will periodically
                                            ** be added to one on disk. */
    i4		    tcb_page_adds;    	    /* Accumulative count of pages
                                            ** added by all users of this
                                            ** TCB since it was allocated.
                                            ** This value will be periodically
                                            ** added to one on disk. */
    i4		    tcb_tperpage;	    /* (Data) Tuples per page. */



    /* Info about all attributes, key and non-key: */
    PTR		    tcb_atts_names;
    i4		    tcb_atts_size;
    i4		    tcb_atts_used;

    DMP_MISC	    *tcb_atts_o_misc;
    PTR		    tcb_atts_o_names;
    i4		    tcb_atts_o_size;
    i4		    tcb_atts_o_used;

    DMP_ROWACCESS   tcb_data_rac;	    /* Row access info for base data
					    ** row;  including att pointer
					    ** array and number of atts
					    */
    DB_ATTS         *tcb_atts_ptr;          /* Pointer to  variable portion of
                                            ** this control block containing
                                            ** a list of attribute descriptors
                                            ** for each attribute in a record
                                            ** of this table.
					    ** tcb_rel.relatts is the number
					    ** of them.
					    */
    DB_ATTS	    *tcb_sec_key_att;	    /* Pointer to security key 
					    ** attribute */

    /* Info about key attributes: */
    DB_ATTS         **tcb_key_atts;         /* Array of pointers to key attrs,
					    ** as they appear in a data row.
                                            */
    i4		    tcb_keys;		    /* Number of attributes in key. */
    i4		    tcb_klen;               /* Length of key in bytes, sort of;
					    ** in btree, length of leaf entry
					    ** including any non-key bytes.
					    */
    i2              tcb_index_count;        /* Number of secondary indices
                                            ** attached to this TCB. */
    i2              tcb_ikey_map[DB_MAXIXATTS+1];
                                            /* A list of attribute numbers
                                            ** from base relation which 
                                            ** correspond to each index
                                            ** attribute.  Used only for 
                                            ** index TCBs. */
    i2		    *tcb_att_key_ptr;	    /* Pointer to variable portion of
                                            ** this control block containing
                                            ** a list of attributes in a key
                                            ** in key order.  The list 
                                            ** contains the attribute ordinal
                                            ** number. */

    /* Attribute info for Btree LEAF pages: */
    DB_ATTS         **tcb_leafkeys;	    /* Array of LEAF key attribute ptrs */
    DMP_ROWACCESS   tcb_leaf_rac;	    /* Access info for LEAF "row";
					    ** this may mimic the data RAC
					    ** or the index RAC depending on
					    ** the btree type (pri vs SI, etc).
					    ** It has to be separate because if
					    ** leaf == data, the leaf may have
					    ** a different compression setting
					    ** from the actual data row.
					    */
    i4		    tcb_kperleaf;           /* Max btree entries per LEAF */
					    /* The length of a LEAF is tcb_klen */

    /* Attribute info for INDEX pages: */
    DMP_ROWACCESS   tcb_index_rac;	    /* Access info for INDEX "row";
					    ** this is typically just the keys,
					    ** like tcb_key_atts, but possibly
					    ** pointing to a copy with altered
					    ** offsets (e.g. clustered btree)
					    */
    DB_ATTS	    *tcb_ixatts_ptr;	    /* Pointer to array of INDEX attributes */
    DB_ATTS         **tcb_ixkeys;	    /* Array of INDEX key attribute ptrs
					    ** which seems redundant, and is
					    ** usually the same as att_ptrs in
					    ** the index RAC;  but in the case
					    ** of ancient-style btree 2ary
					    ** indexes, non-key columns are
					    ** carried into the index!
					    */
    i4		    tcb_kperpage;	    /* Max btree entries per INDEX page */
    i4		    tcb_ixklen;             /* Length of btree INDEX entry  */

    DMP_ROWACCESS   *tcb_rng_rac;	    /* Access info for LEAF range-key
					    ** entries.  This may point to
					    ** the index-rac or the leaf-rac
					    ** (depends on table version).
					    */
    DB_ATTS	    **tcb_rngkeys;	    /* LEAF range keys */
    i4		    tcb_rngklen;	    /* LEAF range klen */
					    /* We need the range entry keys/klen
					    ** only for old-style indexes where
					    ** the range entries carried non-
					    ** key atts.
					    */


    PTR             tcb_lct_ptr;            /* Pointer to sort control block
                                            ** used to load tables. */
    u_i4	    tcb_status;		    /* TCB status bits 
                                            ** DO NOT change without holding
                                            ** the TCB hash mutex!
                                            */
#define			TCB_FREE	    0x0
#define			TCB_VALID	    0x1 /* Valid Table Control Block */
#define			TCB_BUSY	    0x2 /* Being built or released */
#define			TCB_WAIT	    0x4 /* Thread is waiting for TCB */
#define			TCB_INVALID	    0x8 /* Thread is waiting for TCB */
#define			TCB_PARTIAL	    0x20
					    /* Indicates that the TCB holds
					    ** only enough information to allow
					    ** physical access to the underlying
					    ** files - but holds no logical 
					    ** information and can't be used for
					    ** a logical open open operation. 
					    ** It also indicates that all of the
					    ** underlying physical files may
					    ** not be open.
					    */
#define			TCB_PURGE_WAIT	    0x40
					    /* Thread is waiting for exclusive
					    ** access to the tcb inside the tcb
					    ** purge routine.  In this mode,
					    ** the waiting thread has the tcb
					    ** fixed itself and is waiting for
					    ** all other fixers to go away.
					    */
#define			TCB_TOSS_WAIT	    0x80
					    /* Thread is waiting for all fixers
					    ** of a tcb to release it so the
					    ** tcb can be tossed.
					    */
#define			TCB_BUILD_WAIT	    0x100
					    /* Thread is waiting for all fixers
					    ** of a tcb to release it so a new
					    ** index TCB can be linked to it.
					    */
#define			TCB_XACCESS_WAIT    0x200
					    /* Thread outside the TCB manager
					    ** has requested exclusive access
					    ** to a TCB (presumably so location
					    ** information can be updated).
					    */
#define			TCB_CLOSE_ERROR	    0x400
					    /* An error was encountered trying
					    ** to close this tcb.
					    */
#define			TCB_TMPMOD_WAIT	    0x800
					    /* Thread is waiting for exclusive
					    ** access to the tcb in order to
					    ** complete a modify operation on a
					    ** temporary table.  In this mode,
					    ** the waiting thread has the tcb
					    ** fixed itself and is waiting for
					    ** all other fixers to go away.
					    */
#define			TCB_INCONS_IDX	    0x1000
					    /* Inconsistent index in 
					    ** tcb_list.
					    */
#define                 TCB_FLUSH           0x2000
                                            /*
                                            ** (ICL phil.p)
                                            ** The last unfixer of this tcb
                                            ** should flush all the tables
                                            ** dirty pages from the cache.
                                            */
#define                 TCB_VALIDATED       0x4000
                                            /*
                                            ** (ICL phil.p)
                                            ** Indicate that the TCB
                                            ** is currently locked shared. 
                                            */
#define			TCB_SHOWONLY	    0x8000
					    /* The TCB is for DMT_SHOW only,
					    ** and may not be used for table
					    ** access.  (e.g. because the
					    ** required page-size cache does
					    ** not exist.)
					    */
#define                 TCB_BEING_RELEASED  0x10000
                                            /*
                                            ** TCB is being used to update
                                            ** the record and page count
                                            ** in the database, prior to the
                                            ** TCB being used for another
                                            ** table.
                                            */
/* NOTE - Update the following list when tcb_status flag change */

#define TCB_FLAG_MEANING "\
VALID,BUSY,WAIT,INVALID,10,PARTIAL,PURGE_WAIT,TOSS_WAIT,BUILD_WAIT,XACCESS_WAIT,\
CLOSE_ERROR,TMPMOD_WAIT,INCONS_IDX,FLUSH,VALIDATED,SHOWONLY,BEING_RELEASED"

    u_i4           tcb_high_logical_key;   /* Most significant i4 of the 8
                                           ** byte integer which is used to
                                           ** generate logical keys.
                                           */
    u_i4           tcb_low_logical_key;    /* Least significant i4 of the 8
                                           ** byte integer which is used to
                                           ** generate logical keys.
                                           */
#define                 TCB_LOGICAL_KEY_INCREMENT         1024
    i4	    	    tcb_table_type;	    /* Dictates access methods to use
					    ** for record level operations. For
					    ** most tables, this will be the
					    ** same as the storage structure
					    ** type (legal values are TCB_HEAP,
					    ** TCB_HASH, TCB_ISAM, TCB_BTREE).
					    ** But also allows for special
					    ** tables which require different
					    ** access methods than the normal
					    ** Ingres storage structures. */
#define			TCB_TAB_GATEWAY	    14	/* Gateway table */

    DM_MUTEX	    tcb_et_mutex;	    /* Mutex of extended table list */
    i4              tcb_et_dmpe_len;        /* Segment size for etabs */
    DMP_ET_LIST	    *tcb_et_list;	    /* List of extended tables */
    DMPP_ACC_PLV    *tcb_acc_plv;           /* MPF Page accessor routines */
    DMPP_ACC_KLV    *tcb_acc_klv;	    /* MPF key accessor routines */
    DMP_TABLE_IO    tcb_table_io;	    /* Table information needed during
					    ** IO operations.  This structure
					    ** contains all physical file and 
					    ** location information */
    DMP_REP_INFO    *tcb_rep_info;	    /* replication info */
    LG_LSN	    tcb_iirel_lsn;	    /* LSN of iirel page when TCB 
					    ** is built */
    u_i2	    tcb_dimension;	    /* Dimension of the Rtree index */
    u_i2	    tcb_hilbertsize;	    /* Size(Hilbert) in Rtree index */
    DB_DT_ID	    tcb_rangedt;	    /* Range's data type	*/
    u_i2	    tcb_rangesize;	    /* Range's size		*/
    f8*		    tcb_range;		    /* Range of the Rtree index */

    i2		    tcb_relid_len;	    /* Blank-purged length of relid */
    i2		    tcb_relowner_len;	    /* Blank-purged length of relowner */
    DML_XCCB	    *tcb_temp_xccb;	    /* Pending destroy XCCB of 
					    ** session temp table (master/base
					    ** table's XCCB if SI or partition)
					    */
    DMP_TCB	    *tcb_pmast_tcb;	    /* Pointer to partition master TCB
					    ** if this table is a physical
					    ** partition (TCB2_PARTITION).
					    ** Pointer-to-self otherwise.
					    */
    DMT_PHYS_PART   *tcb_pp_array;	    /* Pointer to physical partition
					    ** array if partitioned master */
    /* This doubly-linked list is mostly for quickly finding the
    ** latest row and page count deltas for queries in progress.  Note that
    ** in each case the queue pointer point to the QUEUE struct, not the TCB
    ** or RCB itself.
    ** Be aware that an RCB might be on TWO upd-rcbs lists, one for its
    ** table and one for the partitioned master.
    */
    QUEUE 	    tcb_upd_rcbs;	    /* Possibly-updating RCB list
					    ** for this TCB's table */
    i4		    tcb_unique_count;	    /* Number of unique indexes */
    PTR		    tcb_rtb_next;	    /* List of idle RTBs */
    i4		    tcb_partno;	    	    /* If TCB2_PARTITION, this
					    ** TCB's partition number. */

    /* iirelation is keyed only on the db_tab_base.  This is OK for
    ** non-partitioned tables, since we generally want to get at any
    ** secondary index info any time we work with the base table.  (Including
    ** updating the row/page counts.)  This is a disaster in the presence
    ** of partitions, though.  There can be many thousands of partitions.
    ** If we want to update the row/page counts for one of them, we don't
    ** want to crawl through a thousand iirelation pages looking for the
    ** right row.
    ** The (temporary?) answer is to remember the tid of the iirelation row
    ** that built this TCB.  We can use the tid to directly update the proper
    ** row.  This works as long as sysmod is careful, and as long as iirelation
    ** remains hash, and as long as rows don't move around once placed.
    ** This is JonJ's idea, and he gets full credit/ridicule/whatever...
    */
    DM_TID	    tcb_iirel_tid;	    /* Tid of iirelation row for updating */

    struct _DMP_RELATION                    /* Relation tuple structure. */
    {
#define			DM_1_RELATION_KEY  1
     DB_TAB_ID	     reltid;		    /* Table identifier. */
     DB_TAB_NAME     relid;                 /* Table name. */
     DB_OWN_NAME     relowner;              /* Table owner. */
     i2              relatts;               /* Number of attributes in table. */
     i2		     reltcpri;		    /* Table Cache priority */
     i2		     relkeys;		    /* Number of attributes in key. */
     i2              relspec;               /* Storage mode of table. */
#define                  TCB_HEAP           3
#define                  TCB_ISAM           5
#define                  TCB_HASH           7
#define                  TCB_BTREE          11                             
#define                  TCB_RTREE          13                             
#define			 TCB_TPROC	    17  /* not seen in tables */

#define TCB_STORAGE ",,,HEAP,,ISAM,,HASH,,,,BTREE,,RTREE"

     u_i4            relstat;               /* Status bits of table. */
#define                  TCB_CATALOG        0x00000001L
                                    	    /* TCB is system table. (dbms or extended)*/
#define                  TCB_NOUPDT         0x00000002L
                                            /* Table cannot be updated. */
#define                  TCB_PRTUPS         0x00000004L
                                            /* Protections exist for table. */
#define                  TCB_INTEGS         0x00000008L
                                            /* Integrities exist for table. */
#define                  TCB_CONCUR         0x00000010L
                                            /* System tables with special 
                                            ** locking. -- DBMS core catalog. */
#define                  TCB_VIEW           0x00000020L
                                            /* Table is a view. */
#define			 TCB_VBASE	    0x00000040L
					    /* Table is a base for a view. */
#define                  TCB_INDEX          0x00000080L
                                            /* Table is an index. */
#define			 TCB_CLUSTERED	    0x00000000L
					    /* A primary Btree, but Clustered */
#define                  TCB_BINARY         0x00000100L
                                            /* NOT USED, forcibly turned off
					    ** by catalog-v8 to v9 converter.
					    ** Can steal for e.g. TCB_CLUSTERED
                                            */
#define                  TCB_COMPRESSED     0x00000200L
                                            /* Table has compressed records. */
#define			 TCB_INDEX_COMP	    0x00000400L
					    /* will be set at table creation
					    ** time if this table (or index)
					    ** uses index compression. Currently
					    ** only BTREE tables will support
					    ** this (12/90).
					    */
#define			 TCB_IS_PARTITIONED 0x00000800L
					    /* Table is horizontally partitioned */
#define                  TCB_PROALL         0x00001000L
                                            /* indicates ALL to ALL permission.
					    ** implemented useing negative
					    ** logic.  If set then no ALL TO ALL
					    */
#define                  TCB_PROTECT        0x00002000L
                                            /* indicates RETRIEVE to ALL permission.
					    ** implemented useing negative
					    ** logic.  If set then no ALL TO ALL
					    */
#define                  TCB_EXTCATALOG     0x00004000L
                                            /* Table is extended system 
                                            ** table. */
#define                  TCB_JON            0x00008000L
                                            /* Table will be journaled after
                                            ** next checkpoint. */
#define                  TCB_UNIQUE         0x00010000L
                                            /* Table key must be unique. */
#define                  TCB_IDXD           0x00020000L
                                            /* Table has secondary indices. */ 
#define                  TCB_JOURNAL        0x00040000L
                                            /* Table is journaled. */        
#define                  TCB_NORECOVER      0x00080000L
                                            /* Table may not be recoverable. */
#define                  TCB_ZOPTSTAT       0x00100000L
					    /* Table has statistics 
                                            ** for optimizer. */
#define                  TCB_DUPLICATES     0x00200000L
					    /* duplicate tuples are permitted*/
#define                  TCB_MULTIPLE_LOC   0x00400000L
					    /* this table has been extended to
					    ** reside in multiple INGRES
					    ** locations.  Look in iidevices
					    ** for the specific locations.
					    ** ALSO, relloccount will indicate
					    ** number of locations.
					    */
#define			 TCB_GATEWAY	    0x00800000L
					    /* table is a gateway table, and
					    ** therefore has no underlaying
					    ** physical data file.
					    */
#define			 TCB_RULE	    0x01000000L
					    /* Table has rules applied to it */
#define                  TCB_SYS_MAINTAINED 0x02000000L
                                            /* will be set at table creation
                                            ** time iff any of the attributes
                                            ** in the relation have the
                                            ** ATT_SYS_MAINTAINED qualifier.
                                            */
#define			TCB_GW_NOUPDT	    0x04000000L
					    /* set for gateway tables by default
					    ** unless user specifies update in
					    ** register table command.
					    */
#define			TCB_COMMENT	    0x08000000L
					    /* will be set when a comment is
					    ** created for a table or any 
					    ** column in that table.  Used to
					    ** determine whether to check the
					    ** iidbms_comment catalog - does not
					    ** guarentee tuples are there, but
					    ** if this bit is NOT set, guarentees
					    ** there are no iidbms_comment tuples
					    ** for this table. */
#define			TCB_SYNONYM	    0x10000000L
					    /* set if server should check 
					    ** iisynonym table to see if any
					    ** synonyms exist for this table.
					    ** If not set, guarentees there are
					    ** no synonyms for table.  If set
					    ** server must query iisynonym table
					    ** to see if any tuples exist. */
#define			TCB_VGRANT_OK	    0x20000000L
					    /*
					    ** set if this is a view and the
					    ** owner may grant permission on it
					    */
#define			TCB_SECURE	    0x40000000L
                                            /* Will be set at table creation
                                            ** time if this table can only 
                                            ** be modified by a person 
                                            ** with security privilege.
                                            */
					    /* 0x80000000L
					    ** cannot be used. It breaks
					    ** the iitables view.
					    */
/* String values of above bits */
#define	RELSTAT	"\
CATALOG,NOUPDT,PRTUPS,INTEGS,CONCUR,VIEW,VBASE,INDEX,BINARY,\
COMPRESSED,INDEX_COMP,PARTITIONED,PROALL,PROTECT,EXTCATALOG,\
JON,UNIQUE,IDXD,JOURNAL,NORECOVER,ZOPTSTAT,DUPLICATES,\
MULTIPLE_LOC,GATEWAY,RULE,SYS_MAINTAINED,GW_NOUPDT,COMMENT,\
SYNONYM,VGRANT_OK,SECURE,80000000"

     i4              reltups;               /* Approximate number of records
                                            ** in a table. */
     i4              relpages;              /* Approximate number of pages
                                            ** in a table. */
     i4              relprim;               /* Number of  primary pages 
                                            ** in a table. */
     i4              relmain;               /* Number of non-index(isam only)
                                            ** pages in a table. */          
     i4              relsave;               /* Unix time to indicate how
                                            ** long to save table. */
     DB_TAB_TIMESTAMP
                     relstamp12;            /* Create or modify Timestamp for 
                                            ** table. */
     DB_LOC_NAME     relloc;                /* ingres location of a table. */
     u_i4            relcmptlvl;	    /* DMF version of table. */
/* Following added for Jupiter. */
     i4              relcreate;             /* Date table created. */
     i4              relqid1;               /* Query id if table is a view. */
     i4              relqid2;               /* Query id if table is a view. */ 
     i4              relmoddate;            /* Date table last modified. */
/* Watch the alignment here. !!! */
     i2              relidxcount;           /* Number of indices on table. */ 
     i2              relifill;              /* Modify index fill factor. */
     i2              reldfill;              /* Modify data fill factor. */
     i2              rellfill;              /* Modify leaf fill factor. */
     i4              relmin;                /* Modify min pages value. */
     i4              relmax;                /* Modify max pages value. */
     i4		     relpgsize;		    /* page size of this relation */
     i2              relgwid;	            /* identifier of gateway that owns
					    ** the table. */
     i2              relgwother;            /* identifier of gateway that owns
					    ** the table. */
     u_i4            relhigh_logkey;        /* Most significant i4 of the 8
					    ** byte integer which is used to
					    ** generate logical keys. */
     u_i4            rellow_logkey;         /* Least significant i4 of the 8
					    ** byte integer which is used to
					    ** generate logical keys. */
     u_i4	     relfhdr;
     u_i4	     relallocation;
     u_i4	     relextend;
     i2		     relcomptype;	    /* Type of compression employed */
#define 		TCB_C_NONE	    0
#define 		TCB_C_STANDARD	    1
#define 		TCB_C_OLD	    2	/* See below */
#define                 TCB_C_HICOMPRESS    7
#define 		TCB_C_DEFAULT	    TCB_C_STANDARD /* for normal */

	/* Note, the TCB_C_OLD compression type is not really stored
	** in relcomptype.  Somewhere in ancient times, the standard
	** compression algorithm got changed (to include null compression),
	** but the type code didn't change.  When the server used to
	** support reading ancient tables, it used TCB_C_OLD internally.
	** For safety, and to avoid confusion, please treat TCB_C_OLD
	** as "reserved";  don't reuse it if you add a new compression
	** type number.  (kschendel Oct 2009)
	*/

     i2		     relpgtype;		    /* Page format */
#define			TCB_PG_INVALID      DM_PG_INVALID /* 0 */
#define                 TCB_PG_V1           DM_PG_V1 /* 1 */
#define                 TCB_PG_V2           DM_PG_V2 /* 2 */
#define                 TCB_PG_V3           DM_PG_V3 /* 3 */
#define                 TCB_PG_V4           DM_PG_V4 /* 4 */
#define			TCB_PG_V5	    DM_PG_V5 /* 5 */
#define			TCB_PG_V6	    DM_PG_V6 /* 6 */
#define			TCB_PG_V7	    DM_PG_V7 /* 7 */

     u_i4	     relstat2;
#define			 TCB2_EXTENSION	    0x00000001L
					    /*
					    ** Set at table creation time if
					    ** this table is an extension of
					    ** another table -- currently for
					    ** support of peripheral attributes.
					    */
#define			 TCB2_HAS_EXTENSIONS  0x00000002L
					    /*
					    ** Set at table creation time if
					    ** this table has any extensions
					    */
#define			TCB_PERSISTS_OVER_MODIFIES 0x00000004L
					    /* used for indices.  modifies
					    ** will not blow away this index.
					    ** QEF will recreate this index
					    ** if the base table is modified.
					    */
#define 		TCB_SUPPORTS_CONSTRAINT	0x00000008L
					    /* the clustering mechanism of
					    ** this index (or base table)
					    ** supports a constraint.
					    */
#define 		TCB_SYSTEM_GENERATED	0x00000010L
					    /* the system created this table */
#define			TCB_NOT_DROPPABLE	0x00000020L
					    /* for indices enforcing FIPS
					    ** constraints. users cannot
					    ** explicitly drop the index with
					    ** "DROP INDEX".  Instead, they
					    ** must DROP the underlying
					    ** constraint.  users can modify
					    ** this table/index.
					    */
#define			TCB_STATEMENT_LEVEL_UNIQUE 0x00000040L
					    /* the clustering structure of
					    ** this base table (or index) supports
					    ** statement level uniqueness.
					    ** that is, at end of statement,
					    ** there will be no duplicates in
					    ** this table (or index).
					    */

#define 		TCB2_READONLY		0x00000080L
					    /*
					    ** This table can only be read
					    */
#define 		TCB2_CONCURRENT_ACCESS	0x00000100L
					    /*
					    ** Used by an Index, to indicate that
					    ** it is being created in concurrent
					    ** access mode.
					    */
#define 		TCB2_PHYS_INCONSISTENT	0x00000200L
                                            /*
                                            ** Table is physical inconsistent
                                            */
#define 		TCB2_LOGICAL_INCONSISTENT 0x00000400L
                                            /*
                                            ** Table is logically inconsistent,
                                            ** used by point-in-time rollforward
                                            */
#define 		TCB2_TBL_RECOVERY_DISALLOWED 0x00000800L
                                            /*
                                            ** Table level rollforward is
                                            ** disallowed on this table
                                            */
#define 		TCB2_ALTERED		0x00001000L
					    /*
					    ** set whenever "alter table" stmt is
					    ** issued.  Unset by "modify" that chg
					    ** each tuple in relation. (ie modify
					    ** to merge will not unset, but modify
					    ** to isam/btree/heap/hash will unset.
					    */
#define 		TCB2_PARALLEL_IDX	0x00002000L
					    /* Indexes created in parallel */
#define 		TCB_NOT_UNIQUE		0x00004000L
					    /* Index is for a non-unique constraint
					    ** (such as a foreign key).
					    */
#define 		TCB2_PARTITION		0x00008000L
					    /* Table is a physical partition.
					    ** relid will be something fake.
					    ** master can be found via iipartition.
					    */
#define 		TCB2_GLOBAL_INDEX	0x00010000L
					    /* Table is a global secondary
					    ** index.  tidp is an i8 and will
					    ** include a partition #.  Base
					    ** table is partitioned.
					    */
#define 		TCB_ROW_AUDIT		0x00800000L
					    /*
					    ** This table has per-row security 
					    ** auditing
					    */
#define 		TCB_ALARM		0x01000000L
					    /*
					    ** This table has security alarms
					    */
#define 		TCB2_PHYSLOCK_CONCUR	0x00020000L
					    /* Table uses physical locks 
					    ** Implies that the table must 
					    ** remain hash and this is enforced 
					    ** in dm2u_modify
					    ** Note dmt_set_lock_values uses the
					    ** table id instead of relstat2 
					    ** since it's not available at the 
					    ** time. If you add a new
					    ** table with this flag, or add this
					    ** flag to an existing table, YOU 
					    ** MUST change dmt_set_lock_values 
					    ** accordingly and change 
					    ** dub_mandflags in dubdata.c 
					    */
#define 		TCB_SESSION_TEMP	0x02000000L
					    /* This table is a session-wide
					    ** temporary table.
					    */
#define 		TCB2_BSWAP		0x04000000L
					    /* Byte swap peripheral per_key
					    ** for better performance */
/* String values of above bits */
#define	RELSTAT2	"\
EXTENTION,HAS_EXTENSIONS,PERSISTENT,SUPPORTS_CONSTRAINT,SYS_GEN,\
NOT_DROPPABLE,STMT_LEVEL_UNIQUE,READONLY,CONCURRENT,PHYS_INCONSIST,\
LOG_INCONSIST,NO_TBL_RECOV,ALTERED,PARALLEL_IDX,NOT_UNIQUE,PARTITION,\
GLOBAL_INDEX,PHYSLOCK_CONCUR,40000,80000,100000,200000,400000,ROW_AUDIT,ALARM,\
SESSION_TEMP,BSWAP"
     char	     relfree1[8];	    /* Reserved for future use */
     i2              relloccount;	    /* Number of locations. */
     u_i2	     relversion;	    /* metadata version #; incr when column
					    ** layouts are altered
					    */
     i4              relwid;		    /* Width in bytes of table record.*/
     i4              reltotwid;             /* Tot Width in bytes of table record.*/
     u_i2	     relnparts;		    /* Total physical partitions, zero
					    ** (not 1) for unpartitioned tables
					    ** Overloaded to mean physical
					    ** partition # for partitions.
					    */
     i2		     relnpartlevels;	    /* Number of partitioning levels, zero
					    ** (not 1) for unpartitioned tables.
					    */
     char            relfree[8];            /* Reserved for future use. */

     }		    tcb_rel;                   
};

/*{
** Name:  DMP_DEVICE - DMF internal structure for device record.
**
** Description:
**      This typedef defines the device record used to describe 
**      additional locations for a table.
**
** History:
**      16-nov-87 (jennifer)
**          Created for Jupiter.
*/

struct _DMP_DEVICE
{
#define			DM_1_DEVICE_KEY  1
#define			DM_2_DEVICE_KEY  2
	 DB_TAB_ID       devreltid;	    /* Identifier associated with 
                                            ** this table. */
	 i4              devlocid;	    /* ID associated with this 
                                            ** additional location.*/
	 DB_LOC_NAME     devloc;            /* Location name. */
};

/*{
** Name:  DMP_INDEX - DMF internal structure for index record.
**
** Description:
**      This typedef defines the index record used to describe 
**      indices of a table.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	31-aug-92 (kwatts)
**	    Added a DMP_INDEX_SIZE because 'sizeof(DMP_INDEX)' word aligns
**	    on Sun/DRS6000
**	30-May-2006 (jenjo02)
**	    "idom" expanded to DB_MAXIXATTS to contain cluster key
**	    attributes for indexes on clustered tables.
**	    DMP_INDEX_SIZE correspondingly changed from 74 to 138
**	23-Jun-2006 (kschendel)
**	    Compute DMP_INDEX_SIZE instead of making it a magic number.
*/

struct _DMP_INDEX
{
#define			DMP_INDEX_SIZE  (4+4+2+2*DB_MAXIXATTS)
#define			DM_1_INDEX_KEY  1
     i4	     	     baseid;                /* Identifier associated with 
                                            ** the base table. */
     i4	     	     indexid;               /* Identifier associated with
                                            ** the index table. */
     i2              sequence;              /* Sequence number of index. */
     i2              idom[DB_MAXIXATTS];    /* Attribute number of base 
                                            ** table which corresponds to each
                                            ** attribute of index. */
};
/*{
** Name:  DMP_RINDEX - DMF internal structure for relation index record.
**
** Description:
**      This typedef defines the index record used to index the relation
**      table.
**
** History:
**      01-sep-85 (jennifer)
**          Created for Jupiter.
**	16-mar-93 (andre)
**	    moved definition of IIREL_IDX tuple into DBMS.H
*/
typedef DB_IIREL_IDX	    DMP_RINDEX;

/*}
** Name: DMP_LOCATION  Location description.
**
** Description:
**      This structure contains the internal information used by DMF
**      to access location information.
**
** History:
**      16-nov-1987 (Jennifer)
**          Created for Jupiter.
**	26-oct-92 (rogerk)
**	    Reduced Logging Project:
**	    Added loc_status field to support partial TCB's and to allow
**	    a table to be partially open (to have some of its locations 
**	    physically open and others not).  Added loc_config_id field.
**	18-nov-92 (jnash)
**	    Reduced Logging Project.  Add LOC_UNAVAIL to reflect locations
**	    that are not recovered.
**	09-Oct-2007 (jonj)
**	    Add LOC_STATUS define.
[@history_template@]...
*/
struct _DMP_LOCATION
{
      i4		loc_status;
#define                     LOC_VALID	0x0001  /* Location Information Valid */
#define                     LOC_OPEN	0x0002  /* Location FCB is open */
#define                     LOC_UNAVAIL	0x0004  /* Location not recovered */
#define                     LOC_RAW     0x0008  /* Location is a RAW file */
/* String values of above bits */
#define	LOC_STATUS	"\
VALID,OPEN,UNAVAIL,RAW"

      i4		loc_id;		/* Additional location id. */
      DB_LOC_NAME	loc_name;	/* Location logical name. */
      i4		loc_config_id;	/* The location number for this loc
					** in this database. This is defined
					** by this location's position in the
					** database config file. */
      DMP_LOC_ENTRY	*loc_ext;	/* Pointer to DCB loc entry 
					** associated with this location. */
      DMP_FCB		*loc_fcb;	/* Pointer to FCB associated with
					** this location. */
};
/*
** Name: DMP_REP_INFO - replication information for table
**
** Description:
**	This structure contains all the replication information for
**	a given table, it is populated a table open time and attached to
**	the tcb, the structure contains the dd_regist_tables record for
**	this table
**
** History:
**	24-apr-96 (stephenb)
**	    Created for phase 1 of DBMS replication, this should be a
**	    temporary measure which disapears in the final architecture
*/
struct _DMP_REP_INFO
{
    i2		rep_type;
#define		REP_CB		DM_REP_CB
    i4	rep_ascii_id;
#define                 REP_ASCII_ID        CV_C_CONST_MACRO('R', 'E', 'C', 'B')
    struct _DMP_DD_REGIST_TABLES
    {
	i4	tabno;
	char	tab_name[DB_TAB_MAXNAME];
	char	tab_owner[DB_OWN_MAXNAME];
	char	reg_date[25];
	char	sup_obs_cre_date[25];
	char	rules_cre_date[25];
	i2	cdds_no;
	char	cdds_lookup_table[DB_TAB_MAXNAME];
	char	prio_lookup_table[DB_TAB_MAXNAME];
	char	index_used[DB_TAB_MAXNAME];
    } dd_reg_tables;
};

/*
** Name: DMP_REP_SHAD_INFO - fixed collumns in a replicator shadow table
** 
** Description:
**	This contains information from the fixed part of any shadow table,
**	needed for future refference
**
** History:
**	25-apr-96 (stephenb)
**	    Created for phase 1 of the DBMS replication project, this should
**	    be a temporary structure which goes away in the final architecture
*/
struct _DMP_REP_SHAD_INFO
{
    i2		database_no; 
    i4		transaction_id;
    i4		sequence_no;
    char	trans_time[12];
    char	dist_time[12];
    i1		in_archive;
    i2		cdds_no;
    i2		trans_type;
    i2		old_source_db;
    i4		old_transaction_id;
    i4		old_sequence_no;
};

/*
** Name: DMP_SHAD_REC_QUE
**
** Description:
**	Linked list of shadow records, used in data capture routines
**
** History:
**	1-may-96 (stephenb)
**	    Initial createion for DBMS replication phase 1
*/
struct _DMP_SHAD_REC_QUE
{
    DMP_SHAD_REC_QUE	*next_rec;
    DMP_SHAD_REC_QUE	*prev_rec;
    DM_TID		tid;
    DMP_REP_SHAD_INFO	shad_info;
    char		shad_rec[1];
};


/*}
** Name:  DMP_RANGE - DMF internal structure for the range record.
**
** Description:
**      This typedef defines the range record used to describe 
**      range per dimension of an RTree index. 
**
** History:
**      08-may-95 (shero03)
**          Created for RTree.
**	24-Sep-1997 (kosma01)
**	    removed a embedded comment starter (slash-splat).
[@history_template@]...
*/

struct _DMP_RANGE
{
     i4	     range_baseid;          /* Identifier associated with 
                                            ** the base table. */
     i4	     range_indexid;         /* Identifier associated with
                                            ** the index table. */

     f8              range_ll[DB_MAXRTREE_DIM]; /* Lower left of range box*/
     f8              range_ur[DB_MAXRTREE_DIM]; /* Upper right of range box */
     u_i2	     range_dimension;	    /* Dimension of this range set */
     u_i2	     range_hilbertsize;	    /* HilbertSize of this range set */
     i2		     range_datatype;	    /* data type of range box	*/
     char	     range_type;	    /* I:integers, F:floats	*/

};

/*
** Name: DMP_ETAB_CATALOG - Tuple definition for extended table catalog
**
** Description:
**      This structure defines the table used to store extended table
**	definitions.  This catalog stores the base and extension, the reason the
**	base was extended, and the attribute for which this applies.  The reason
**	is included in case there is another reason to extend tables in the
**	future.
**
**	This table "could be created" as
**	
**	    create table iiextended_relation
**	    (
**		etab_base	integer,
**		etab_extension	integer,
**		etab_type	integer,
**		etab_att_id	integer,
**		etab_reserved	char(16)
**	    );
**	    Modify iiextended_relation to hash on etab_base;
**
**	Each entry describes an extension table (its table id is
**	etab_extension) which is an extension for table (etab_base).  The type
**	of extension is etab_type, extended for attribute etab_att_id.
**
**	Thus, when a table's extensions are required, one postions by base id,
**	then scans for matches by type and attribute.  Multiple matches are
**	likely.
**
** History:
**      23-Jan-1990 (fred)
**          Created.
[@history_template@]...
*/
typedef struct _DMP_ETAB_CATALOG
{
    u_i4            etab_base;          /* The base id of the base table */
#define			DMP_ET_BASE_NAME   "etab_base"
#define			DMP_ET_BASE_SIZE   sizeof(u_i4)
#define			DMP_ET_BASE_TYPE   DB_INT_TYPE
#define			DMP_ET_BASE_PREC   0
    u_i4	    etab_extension;	/* The id of the extension */
#define			DMP_ET_EXT_NAME    "etab_extension"
#define			DMP_ET_EXT_SIZE    sizeof(u_i4)
#define			DMP_ET_EXT_TYPE    DB_INT_TYPE
#define			DMP_ET_EXT_PREC    0
    u_i4	    etab_type;		/* The type of extension */
#define			DMP_LH_ETAB    0x0 /* List header */
#define                 DMP_LO_ETAB    0x1 /* Large Object Support */
#define			DMP_LO_HEAP    0x2 /* Heap etab */

#define			DMP_ET_TYPE_NAME   "etab_type"
#define			DMP_ET_TYPE_SIZE   sizeof(u_i4)
#define			DMP_ET_TYPE_TYPE   DB_INT_TYPE
#define			DMP_ET_TYPE_PREC   0
    u_i4	    etab_att_id;	/* Data to support the reason */
#define			DMP_ET_ATT_NAME    "etab_att_id"
#define			DMP_ET_ATT_SIZE    sizeof(u_i4)
#define			DMP_ET_ATT_TYPE    DB_INT_TYPE
#define			DMP_ET_ATT_PREC    0
    char	    etab_filler[16];	/* For future expansion */
#define			DMP_ET_FILL_NAME   "etab_reserved"
#define			DMP_ET_FILL_SIZE   16
#define			DMP_ET_FILL_TYPE   DB_CHA_TYPE
#define			DMP_ET_FILL_PREC   0
}   DMP_ETAB_CATALOG;

#define                 DMP_ETAB_ATT_COUNT 5
#define			DMP_ET_KEY_COUNT 1

/*}
** Name: DMP_ET_LIST - List of extended tables
**
** Description:
**      This structure defines the in core list of extended tables defined for
**	any given table.  These are chained off the tcb as they are found and
**	needed.  This list is chosen rather than a TCB since there may be a very
**	large number of extended tables, which are rarely used.  It seems
**	prudent to allow them to be materialized on demand.
**
** History:
**      24-Jan-1990 (fred)
**          Created.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**          In this instance owner was missing from the 'standard header'.
**      26-jul-1996 (sarjo01)
**          For bug 77690: Add ETL_INVALID_MASK for etl_status in
**          DMP_ET_LIST
**	28-jul-1998 (somsa01)
**	    Added ETL_LOAD to tell us that a load operation is being
**	    performed on this extension table. In this situation, also added
**	    new parameters to save the dmrcb and dmtcb of an opened extension
**	    table.  (Bug #92217)
[@history_template@]...
*/
struct _DMP_ET_LIST
{
    DMP_ET_LIST     *etl_next;          /* Next in the queue off the TCB */
    DMP_ET_LIST	    *etl_prev;
    SIZE_TYPE	    etl_length;		/* Size of this structure */
    i2		    etl_type;
#define                 ETL_CB		DM_ETL_CB
    i2		    etl_s_reserved;
    PTR  	    etl_l_reserved;
    PTR             etl_l_owner;
    i4	    etl_ascii_id;
#define                 ETL_ASCII_ID	CV_C_CONST_MACRO('#', 'E', 'T', 'L')
    i4		    etl_status;		/* Known volatile information */
#define                 ETL_FULL_MASK		0x02
#define                 ETL_INVALID_MASK	0x04
#define                 ETL_LOAD		0x08
    DMR_CB	    etl_dmrcb;		/* The DMR_CB of an opened extension
					** table for a load operation.
					*/
    DMT_CB	    etl_dmtcb;		/* The DMT_CB of an opened extension
					** table for a load operation.
					*/
    DMP_ETAB_CATALOG
		    etl_etab;		/* Record describing table */
    i4		    etl_etab_journal;   /* journaling on/off (-1 not init) */
};

/*}
** Name: DMP_PFREQ   - Prefetch request block  
**
** Description:
**	This structure is used to represent a prefetch (readahead) request
**	that has been made by a user thread (eg via dm2r_schedule_prefet())
**	and that is to be serviced by one of the readahead threads.
**	One or more of these structures may exist, and are arranged in
**	a circular queue with forward and backward pointers. The queue
**	is anchored by two pointers found in the DMP_PFH (prefetch queue
**	header) block, which contains other control and statistical
**	info about the prefetch system.
**
** History:
**	04-may-1995 (cohmi01)
**          Created.
**	07-jun-1995 (cohmi01)
**	    Added items for queue mngmt, multiple readahead threads.
[@history_template@]...
*/

/* special args to cancel a prefetch request */
#define PFR_CANCEL_ALL		-1		/* cancel all req for an rcb */
#define PFR_CANCEL_IX 		-2		/* cancel all ix req for rcb */

/* boundary limits between user threads and readahead threads for concurrent */
/* operations against secondary indexes, to reduce collisions (lagbehind).   */
/* These numbers refer to ordinal position of secind TCB chain (sorted)      */
#define PFR_PREFET_MIN		0	/* RA thr can do all ix, for prefet. */
#define PFR_PREALLOC_MIN	1	/* cushion, prealloc has no idletime */

#define PFR_MAX_ITEMS		32	/* max items (eg sec indxs) in req   */

typedef struct _DMP_PFREQ  DMP_PFREQ;

struct _DMP_PFREQ
{
    u_i2 	pfr_type;			/* request type */
#define		  PFR_PREFET_SECIDX	1	/* pre-fetch sec idx tups */
#define		  PFR_PREALLOC_SECIDX	2	/* pre-alloc sec idx tup (ins)*/
#define		  PFR_PREFET_PAGE       3	/* pre-fetch particular page */
#define		  PFR_PREFET_OVFCHAIN   4	/* pre-fetch overflow chain  */
    u_i2   	pfr_status;
#define		  PFR_FREE      	0x01	/* this block is free        */
#define		  PFR_WAITED_ON		0x02	/* Someone wants rcb         */
#define		  PFR_STALE_REQ		0x04	/* Entire request is stale   */
    DMP_PFREQ	*pfr_next;			/* ptr to next request       */
    DMP_PFREQ	*pfr_prev;			/* ptr to prev request       */
    DB_TAB_ID	pfr_table_id;			/* crosscheck/debug only     */
    i4	handle;				/* unique handle of request  */
    u_i2   	pfr_pincount;			/* # of threads processing   */
						/* this request concurrently */
    i4	pfr_intrupt[PFR_MAX_ITEMS];	/* areas for rcb_uiptr       */
	
    union	/* items that vary base on pfr_type  */
    {
	struct	{
	    DMP_RCB	*rcb;			/* rcb for this request      */
	    u_i2	totix;  		/* total # of ix to process  */
    	    u_i2	userix;			/* ix# user thread is upto   */
    	    u_i2	raix;       		/* next ix# for readahead thr*/
        } si;				/* si == Prefet/Prealloc Sec Index   */
	struct	{
	    DMP_RCB	*rcb;			/* rcb for this request      */
	    DM_PAGENO	pageno;			/* desired page number      */
	} dp;				/* dp == Prefetch Data Page 	    */
	struct	{
	    DMP_RCB	*rcb;			/* rcb for this request      */
	    DM_PAGENO	startpage;		/* 1st page of ovflo chain  */
	    i4  	userpcnt;		/* # pages read by user     */
	    i4  	rapcnt;			/* # pages read by RA thread*/
	} oc;				/* oc == Prefetch Overflow Chain    */
	    
    }t;
};



/*}
** Name: DMP_PFH  - Prefetch request queue header
**
** Description:
**	This structure is the anchor of the prefetch request queue, and
**	is pointed to by the dmf_svcb, if there are any readahead threads.
**
** History:
**	04-may-1995 (cohmi01)
**          Created.
**	07-jun-1995 (cohmi01)
**	    Added items for queue mngmt, multiple readahead threads.
[@history_template@]...
*/

struct _DMP_PFH
{
/* standard DM_OBJECT header */
    DMP_PFH         *pfh_q_next;
    DMP_PFH         *pfh_q_prev;
    SIZE_TYPE       pfh_length;             /* Length of the control block. */
    i2              pfh_type;             
#define                 PFH_CB              DM_PFH_CB
    i2              pfh_s_reserved;         /* Reserved for future use. */
    PTR             pfh_l_reserved;         /* Reserved for future use. */
    PTR             pfh_owner;              /* Owner of control block for
                                            ** memory manager.  EXT will be
                                            ** owned by the DCB. */
    i4         pfh_ascii_id;           /* Ascii identifier for aiding
                                            ** in debugging. */
#define                 PFH_ASCII_ID        CV_C_CONST_MACRO('#', 'P', 'F', 'H')

    i4		pfh_status; 
#define		DMP_PFWAIT		0x01
/* items for maintenance of doubly linked list of request blocks */
    u_i2   		pfh_numreqs;	    /* total num of request blocks */
    u_i2   		pfh_numsched;       /* num of requests scheduled    */
    u_i2   		pfh_numactive;      /* num of requests in execution */
    DMP_PFREQ		*pfh_next;	    /* next request to execute */
    DMP_PFREQ		*pfh_freehdr;  	    /* next item of free list */
    CS_SEMAPHORE	pfh_mutex;
/* readahead statistics */
    i4		pfh_scheduled;	    /* total num scheduled */
    i4		pfh_racomplete;	    /* # requests that completed */
    i4		pfh_cancelled;      /* # req fully cancelled  */
    i4		pfh_lagbehind;	    /* # req partially cancelled */
    i4		pfh_fallbehind;	    /* # errs due to no-idx tup, */
					    /* means we fell behind (bad) */
    i4		pfh_lkcontend;	    /* # of lock_no_avail errors  */	
    i4		pfh_bufstress;	    /* # times not enough bufs for RA */
    i4		pfh_no_blocks;	    /* # times no reqblock avail  */
};

/*}
** Name: DMP_CLOSURE  - GatherWrite request closure structure.
**
** Description:
**	This structure is used by dm2f to encapsulate the information
**	to schedule and complete a single GatherWrite request.
**
** History:
**	19-May-1999 (jenjo02)
**	    Created.
[@history_template@]...
*/
struct _DMP_CLOSURE {
    DMP_LOCATION   *clo_l;                  /* Ptr to current location */
    DB_DB_NAME     *clo_db_name;            /* Database name for logging */
    DB_TAB_NAME    *clo_tab_name;           /* Table name for logging */
    i4        	   clo_pageno;              /* Page number */
    i4        	   clo_pg_cnt;              /* Number of pages */
    VOID      	   (*clo_evcomp)();         /* Callers completion handler */
    PTR            clo_data;                /* Completion data ptr */
    PTR		   clo_di;		    /* DI's work area */
};

struct  _DMP_RNL_LSN
{
    DM_PAGENO	rnl_page;
    LG_LSN	rnl_lsn;
};

struct _DMP_RNL_ONLINE {
    LG_LSN	    rnl_bsf_lsn;
    DMP_MISC	    *rnl_xmap;	    /* memory allocated for page map */
    char	    *rnl_map;	    /* the page map */
    i4		    rnl_xmap_cnt;   /* size of xmap */
    DM_PAGENO	    rnl_lastpage;   /* page number of last page in file */
    DMP_MISC	    *rnl_xlsn;	    /* memory allocated for LSNs */
    DMP_RNL_LSN	    *rnl_lsn;	    /* LSN info for pages updated after bsf */
    i4		    rnl_lsn_cnt;    /* Number of DM2U_MXLSN entries */
    i4		    rnl_lsn_max;    /* Maximum number of DM2U_MXLSN entries */
    i4		    rnl_read_cnt;   /* Number of pages read */
    i4		    rnl_dm1p_max;   /* Max pages for this page type/size */
    u_i4	    rnl_btree_flags;
#define		RNL_NEED_LEAF		0x0001
#define		RNL_NEXT		0x0002
#define		RNL_NEED_KEYTID		0x0004
#define		RNL_READ_ASSOC		0x0008
#define		RNL_INIT		(RNL_NEED_LEAF | RNL_NEED_KEYTID)
    DMP_MISC	    *rnl_btree_xmap;
    char	    *rnl_btree_map;
    i4  	    rnl_btree_xmap_cnt;
    DMP_RNL_LSN	    *rnl_lsn_wait;    /* for online modify rollforward */
};
 
/* macro used to map page size into array index */
#define DM_CACHE_IX(pgsize) (pgsize == 2048 || pgsize == 0 ? 0 :      \
			pgsize == 4096 ? 1 : pgsize == 8192 ? 2 : \
			pgsize == 16384 ? 3 : pgsize == 32768 ? 4: 5)

/* macro used by prefetch to decide if there is room in buffer pool */
#define dm0p_buffers_full(pgsz, fcount) \
 (fcount < \
(dmf_svcb->svcb_lbmcb_ptr->lbm_bmcb[DM_CACHE_IX(pgsz)]->bm_flimit \
 * 2) )

/*
** macro used to see if buffers exist for specified page size 
** Return yes if cache exists and page array allocated
*/
#define dm0p_has_buffers(pgsz) \
  (dmf_svcb->svcb_lbmcb_ptr->lbm_bmcb[DM_CACHE_IX(pgsz)]          \
 && dmf_svcb->svcb_lbmcb_ptr->lbm_pages[DM_CACHE_IX(pgsz)] ? 1 : 0)

/*
** macro used to see if record locking is enabled for rcb
*/
#define row_locking(rcb)                                   \
  ((rcb->rcb_lk_type == RCB_K_ROW) ? 1 : 0)

/*
** macro used to see if record locking was ever enabled for rcb
*/
#define set_row_locking(rcb)                                   \
  ((rcb->rcb_k_type == RCB_K_ROW) ? 1 : 0)

/*
** macro used to see if Consistent read row locking is enabled for rcb
*/
#define crow_locking(rcb)                                   \
  ((rcb->rcb_lk_type == RCB_K_CROW) ? 1 : 0)


/*
** macro used to see if rcb is opened by serializable transaction
*/
#define serializable(rcb)                                   \
  ((rcb->rcb_iso_level == RCB_SERIALIZABLE) ? 1 : 0)
/*
** This macro finds out whether we need to journal the table
** Since the RCB/TCB can be stale (meaning the journalling can say JON
** yet we need to journal the table), we need to check this to see
** if a backup is in progress or a backup has occurred since we built
** the TCB.
*/
#define is_journal(rcb)                                                     \
  ((rcb->rcb_journal ||                                                     \
    ((rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_status & DCB_S_JOURNAL) && 	    \
     (rcb->rcb_tcb_ptr->tcb_rel.relstat & TCB_JON) &&			    \
     ((rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_status & DCB_S_BACKUP) ||	    \
      (((rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_status & DCB_S_BACKUP) == 0) && \
       (LSN_LT(&rcb->rcb_tcb_ptr->tcb_iirel_lsn,		  	    \
	     &rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_ebackup_lsn)))))) ? 1 : 0)	

/*
** Macro to test if a transaction is active - the quick way using the xid_array
** when the low_tran's lg_id is known (V6 tuple header), 
** or the slow way (dmxe_xn_active()) when it's not.
*/
#define IsTranActive(lg_id, low_tran) \
	((low_tran == 0) ? FALSE : \
	 (lg_id == 0) \
	    ? dmxe_xn_active(low_tran) \
	    : (lg_id <= dmf_svcb->svcb_xid_lg_id_max) \
	        ? (dmf_svcb->svcb_xid_array_ptr[(u_i2)lg_id] == low_tran) \
		    ? TRUE \
		    : FALSE \
		: FALSE)

/*
** Deleted SKIP_DELETED_ROW_MACRO, DUPCHECK_NEED_ROW_LOCK_MACRO as these
** are unreferenced and have DMPP_ counterparts.
*/

#define MVCC_ENABLED_MACRO(r)					\
	(((r->rcb_tcb_ptr->tcb_rel.relpgtype == TCB_PG_V1) ||	\
        ((r->rcb_tcb_ptr->tcb_dcb_ptr->dcb_status & DCB_S_MVCC) == 0)) \
	? 0 : 1)


/*
** Macro to check if Root page is inconsistent with CRIB
**
** Returns TRUE if all are true:
**	crow_locking
**	not in a constraint (constraints are always run on Root pages)
**	not a blob extension opened for write
**	some "get" operation
**	Root page LSN has changed since transaction last fixed/updated it
**	Root page LSN is greater than crib_low_lsn
*/
#define RootPageIsInconsistent(r, pinfo) \
    (crow_locking(r) && !(r->rcb_state & RCB_CONSTRAINT) && \
     !(r->rcb_tcb_ptr->tcb_extended && r->rcb_access_mode == RCB_A_WRITE) && \
     (r->rcb_dmr_opcode == DMR_GET || r->rcb_dmr_opcode == DMR_POSITION || \
      r->rcb_dmr_opcode == DMR_AGGREGATE) && \
      LSN_GT(\
       (DMPP_VPT_GET_PAGE_STAT_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype, \
       		((DMP_PINFO*)pinfo)->page) & DMPP_DATA) \
        ? DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype, \
		((DMP_PINFO*)pinfo)->page) \
        : DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(r->rcb_tcb_ptr->tcb_rel.relpgtype, \
		((DMP_PINFO*)pinfo)->page) \
       , &r->rcb_crib_ptr->crib_low_lsn) \
     ? TRUE : FALSE)

/*
** Macro to check if crow X lock needed for get,
** only if update cursor or constraint
*/
#define NeedCrowLock(r) \
    ( crow_locking(r) && \
      ((r->rcb_state & RCB_CURSOR && r->rcb_access_mode == RCB_A_WRITE) || \
        r->rcb_state & RCB_CONSTRAINT) )

/*
** Macro to check if physical page lock is needed, TRUE if
**
**	o system relation or
**	o blob extension table opened with a lock level
**	  other than Table or MVCC
*/
#define NeedPhysLock(r) \
    ( r->rcb_tcb_ptr->tcb_sysrel || \
     (r->rcb_tcb_ptr->tcb_extended && \
      !crow_locking(r) && \
      r->rcb_lk_type != RCB_K_TABLE) )
