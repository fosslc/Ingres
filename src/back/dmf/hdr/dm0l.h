/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM0L.H - Logging data structures.
**
** Description:
**      This file contains the definitions of the log record
**	formats and the contants used in calls to the dm0l
**	routines.
**
** History:
**      17-may-1986 (Derek)
**          Created for Jupiter.
**      22-nov-87 (jennifer)
**          Added multiple location support for tables.
**	17-may-1988 (EdHsu)
**	    Add new structures for fast commit.
**	12-Jan-1989 (ac)
**	    Add 2PC support.
**	11-sep-1989 (rogerk)
**	    Add new dmu log record (DM0L_DMU).
**	20-sep-1989 (rogerk)
**	    Added savepoint address to DM0L_DMU record.
**	 2-oct-1989 (rogerk)
**	    Add new log record: DM0L_FSWAP.
**     25-feb-1991 (rogerk)
**	    Added DM0LTEST log record for implementing logging and journaling
**	    system tests.  Added with Archiver Stability project.
**	16-jul-1991 (bryanp)
**	    B38527: Add new DM0L_MODIFY and DM0L_INDEX log records, renaming
**	    the old ones to DM0L_OLD_MODIFY and DM0L_OLD_INDEX.
**	    Also, added new log record DM0L_SM0_CLOSEPURGE.
**      16-aug-91 (jnash) merged 1-apr-1990 (Derek)
**          Add new log records to support bitmap allocation.
**          DM0L_ASSOC,DM0L_ALLOC,DM0L_DEALLOC,DM0L_EXTEND
**      16-aug-1991 (jnash) merged 12-mar-1991 (Derek)
**          In various records, added allocation amount, multi-file position and
**          multi-file maximum.  The latter information is used to determine if
**          FHDR and FMAP pages have to be initialized.
**	18-Oct-1991 (rmuth)
**	    In records DM0L_CREATE, DM0L_MODIFY and DM0L_INDEX move the
**	    extend and allocation fields above the location array etc...
**	    as only the used part of these arrays get written to the log file.
**	    Hence extend and allocation where not getting written.
**	 3-nov-1991 (rogerk)
**	    Added new log record DM0L_OVFL. This records the linking of a
**	    new data page into an overflow chain.  This information used to
**	    be logged through the dm0l_calloc log record which went away with
**	    the file extend changes.  This new record is used only for REDO.
**	07-jul-92 (jrb)
**	    Prototyping DMF.
**	22-jul-1992 (bryanp)
**	    Add lg_id argument to dm0l_logforce().
**	14-dec-1992 (jnash & rogerk)
**        Reduced logging project.
**          - Add DM0L_CLR status.  CLRs are logged as the log record
**            type of the compensating action (e.g. a CLR of a PUT is
**            a DEL), but with the DM0L_CLR bit set.
**	    - Added DM0L_PHYS_LOCK log flag to indicate that physical page
**	      locking should be used during recovery.
**	    - Added LSN comparison macros.
**          - Update log record formats, including compressed owner
**            and table names.
**	    - Started changing log record definitions.  Most log record
**	      formats are changing for 6.5 recovery.  The log record types
**	      of all log records changed by this project have increased by
**	      100 to make it possible to recognize old journals and log recs.
**	    - Removed defines dm0l_sbi, dm0l_sput, dm0l_srep, and dm0l_sdel -
**	      system catalog updates are now logged with normal update records.
**	    - Add NOFULL log record, used in recovery of hash table nofullhain
**	      status.
**	    - Add FMAP log record, used during allocation of new fmap pages
**	      during file extend operations.
**	    - Modified dm0l_ovfl FUNC_EXTERN, add dm0l_nofull FUNC_EXTERN.
**	    - Add new btree log records: DM0L_BTPUT, DM0L_BTDEL, DM0L_BTSPLIT,
**	      DM0L_BTOVFL, DM0L_BTFREE, DM0L_BTUPDTOVFL, DM0L_DISASSOC.
**	    - Add DM0L_CRVERIFY and associated FUNC_EXTERN.
**	    - Removed dm0l_logforce routine - all callers now use dm0l_force.
**	15-jan-1993 (jnash)
**	    More reduced logging.
**	    - Remove DM0L_SPUT, DMOL_SDEL, DM0L_SREP, DM0L_SBI.  System
**	      catalog rows now are logged normally.
**	    - Eliminate prev_lsn param from dm0l_create.
**	18-sep-1993 (rogerk)
**	    Removed Abort Save log record from DMU.  The same functionality
**	    is now provided by CLR records.
**	08-feb-1993 (jnash)
**	    Add flag param to dm0l_savepoint() prototype.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed prev_lsn field in the log record header to compensated_lsn.
**	    Moved the database_id field into the dm0l_header.  The database id,
**		transaction id, and lsn fields are now updated by LGwrite when
**		log records are written to the log file.
**	    Add "flag" param to dm0l_savepoint() to allow savepoint log records
**		to be journaled.
**	    Changed EndMini log record to look like a CLR record.  The
**		compensated_lsn field now takes the place of the old
**		em_bm_address.
**	    Changed ECP to point back to the BCP via an LSN instead of a LA.
**	    Removed DM0L_ADDONLY flag.  Recovery processing now uses LGadd
**		directly (and without special LG_ADDONLY flag).
**	    Removed db_id argument from dm0l_sbackup and dm0l_ebackup routines.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Added transaction ID argument to dm0l_read.
**		Added DM0L_RECOVER flag for calls to dm0l_allocate by CSP
**		    during recovery.
**		Added DM0L_FULL_FILENAME flag for standalone logdump to indicate
**		    that the name passed to dm0l_allocate is a complete
**		    filename, not just a nodename.
**		Add field to DM0L_OPENDB log record to record whether database
**		    in question is journaled or not.
**		Remove DM0L_NOREDO. Use DM0L_NONREDO instead.
**	15-may-1993 (rmuth)
**	    Allow concurrent Indicies, add a flag field to DM0L_CREATE.
**	21-jun-1993 (rogerk)
**	    Added DM0L_SCAN_LOGFILE flag to dm0l_allocate to cause LGopen to do
**	    the logfile scan for bof_eof and to position the logfile for a full
**	    scan rather than only reading values between the logical bof and
**	    eof.  This can be used by logdump to dump entire log files.
**	26-jul-1993 (bryanp)
**	    Remove unneeded 'lx_id' arg to dm0l_opendb() and dm0l_closedb() and
**		unneeded 'flag' arg to dm0l_closedb()
**		since we no longer log the dm0l_closedb log record.
**	    Remove the DM0L_CLOSEDB log record definition.
**	26-jul-1993 (rogerk)
**	  - Added back ecp_begin_la which holds the Log Address of the Begin
**	    Consistency Point record. Now the ECP has the LA and LSN of the BCP.
**	  - Added split direction field to the split log record.
**	23-aug-1993 (rogerk)
**	    Added the prev_lsn parameter back to the dm0l_create routine.
**	    Added relstat value to dm0l_create log record.
**	    Added a prev_lsn parameter to the dm0l_index routine.
**	    Added a statement count field to the modify and index log records.
**	    Added location id and table_id information to dm0l_fcreate record.
**	20-sep-1993 (rogerk)
**	    Remove declared name,owner fields from replace log record as
**	    these are stored in compressed fashion at the end of the log
**	    record.
**	20-sep-1993 (rogerk)
**	    Added DM0L_NONJNL_TAB record flag.  This indicates a journaled
**	    log record that describes an update associated with a non-journaled
**	    user table.
**	18-oct-1993 (jnash)
**	    Add support for row compression in replace log records.
**	     - Add DM0L_COMP_REPL_OROW.
**	     - Add new DM0LREP fields.
**	     - Add dm0l_row_unpack(), dm0l_repl_comp_info() and
**	       dm0l_uncomp_replace_log() FUNC_EXTERNs.
**	18-oct-93 (jrb)
**	    Added DM0L_EXT_ALTER to log the alteration of an extent in the
**          config file (MLSorts project).
**	18-oct-1993 (rogerk)
**	    Added dum_buckets to modify log record for rollforward.
**	    Added dui_buckets to modify log record for rollforward.
**	    Added transaction owner to tx information in bcp log record.
**	    Removed unused bcp_bof field from bcp log record.
**	 3-jan-1993 (rogerk)
**	    Changed logging of compressed replace records.  Added rep_odata_len
**	    and rep_ndata_len to hold the sizes of the compressed data which
**	    describes the before and after images of the replace row.  The
**	    rep_orec_size and rep_nrec_size now describe the full sizes of
**	    the row before and after images.
**	    Changed dm0l_row_unpack and dm0l_repl_comp_info prototypes.
**      23-may-1994 (mikem)
**          Bug #63556
**          Changed interface to dm0l_rep() so that a replace of a record only
**          need call dm0l_repl_comp_info() this routine once.  Previous to
**          this change the routine was called once for LGreserve, and once for
**          dm0l_replace().
**      24-jan-1995 (stial01)
**          BUG 66473: added DM0L_TBL_CKPT to indicate table checkpoint in
**          SBACKUP records.
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for table-specific page sizes:
**		Add page_size argument to the function prototype for:
**		    dm0l_bi, dm0l_ai, dm0l_create, dm0l_crverify, dm0l_fcreate,
**		    dm0l_index, dm0l_modify, dm0l_put, dm0l_del, dm0l_rep,
**		    dm0l_assoc, dm0l_alloc, dm0l_dealloc, dm0l_extend,
**		    dm0l_ovfl, dm0l_nofull, dm0l_fmap, dm0l_btput, dm0l_btdel,
**		    dm0l_btsplit, dm0l_btovfl, dm0l_btfree, dm0l_btupdovfl,
**		    dm0l_disassoc.
**		Add XXX_page_size field to the log records corresponding to
**		    the above (DM0L_BI, DM0L_AI, etc.)
**		Made length field in DM0L_HEADER structure a i4 to handle
**		    very long log records.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**      06-mar-1996 (stial01)
**          Added page_size to DM0L_RELOCATE, prototype for dm0l_relocate()
**      01-may-1996 (stial01)
**          Variable size before/after page images follow DM0L_BI and DM0L_AI.
**          DM0L_BI: remove bi_page, DM0L_AI: remove ai_page
**	23-may-1996 (shero03)
**	    RTree: add dimension, hilbertsize, range to dm0l_index
**      22-jul-1996 (ramra01 for bryanp)
**          Add support for Alter Table:
**              Add row_version argument to dm0l_del prototype.
**              Add row_version argument to dm0l_put prototype.
**              Add orow_version and nrow_version arguments to
**                  dm0l_rep prototype.
**              Add del_row_version to the DM0L_DEL log record.
**              Add put_row_version to the DM0L_PUT log record.
**              Add rep_orow_version and rep_nrow_version to the
**                  DM0L_REP log record.
**	03-sep-1996 (pchang)
**	    Added ext_last_used to DM0L_EXTEND and fmap_last_used to DM0L_FMAP
**	    and their corresponding function prototypes as part of the fix for 
**	    B77416. 
** 	24-sep-1996 (wonst02)
** 	    Modify DM0L_RTADJ and rename to _RTREP; add _RTPUT and _RTDEL.
**	10-jan-1997 (nanpr01)
**	    A previous change has taken out the ai_page and bi_page to make
**	    the LGreserve calculation simpler but making it vulnerable to
**	    alignment issues for further development. This is also true
**	    for DM0L_BTSPLIT structure.
**	27-Feb-1997 (jenjo02)
**	    Removed dm0l_robt() prototype. Code integrated into dmxe_begin().
** 	10-jun-1997 (wonst02)
** 	    Added DM0L_READONLY_DB for readonly databases.
**      29-jul-97 (stial01)
**          Added DM0L_REPLACE to specify original mode REPLACE
**	11-Nov-1997 (jenjo02)
**	    Added DM0L_DELAYBT, passed to dm0l_bt() but not written in 
**	    log record header.
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**          Added page size information to dm0l_sm1_rename
**	15-jan-1999 (nanpr01)
**	    Pass pointer to pointer in dm0l_deallocate routine.
**      09-feb-1999 (stial01)
**          Added kperpage to BTSPLIT log record and dm0l_btsplit prototype
**	15-Mar-1999 (jenjo02)
**	    Removed DM0L_DELAYBT.
**	21-mar-1999 (nanpr01)
**	    Support raw locations.
**      21-dec-1999 (stial01)
**          Added page type to create log record
**	18-jan-2000 (gupsh01)
**	    Added flag DM0L_NONJNLTAB to indicate the records which are a 
**	    part of transactions involving non-journaled tables for a 
**	    journaled database. 
**	14-Mar-2000 (jenjo02
**	    Added *lsn parm to dm0l_bt() prototype.
**	04-May-2000 (jenjo02)
**	    Added name_len, owner_len parms to several function prototypes
**	    in which caller is required to pass the blank-stripped lengths
**	    of table_name and owner_name so that the dm0l functions don't 
**	    have to constantly compute these values.
**      25-may-2000 (stial01)
**          Added compression type to put/del/replace log records
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      30-Oct-2000 (hanal04) Bug 103037 INGSRV 1291
**          Added fmap_hw_mark to DM0L_FMAP and the prototype for 
**          dm0l_fmap().
**      28-mar-2003 (stial01)
**          Child on DMPP_INDEX page may be > 512, so it must be logged
**          separately in DM0L_BTPUT, DM0L_BTDEL records (b109936)
**      18-apr-2003 (stial01)
**          Invalidate previous BTPUT,BTDEL types so rollforward can detect
**          if it is trying to apply old version of these log records. (b109936)
**      02-jan-2004 (stial01)
**          Added DM0L_NOBLOBLOGGING, removed unused DM0L_BTREE_REC
**	6-Feb-2004 (schka24)
**	    Rename/extend stmt count in index, modify records.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Redefine length and offset fields in DM0L_PUT, DM0L_DEL, DM0L_REP
**          i2->i4 for 256k row support.
**	1-Apr-2004 (schka24)
**	    Invent "raw data" log record for logging indefinite length
**	    partition definitions.
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          On some platforms "char" is a signed value, hence converting the
**          char code to an integer may set a negative value. This causes
**          problems with INDEX/MODIFY log records where the Key's base
**          table attribute is >127. A new type DM_ATT has been declared
**          for an Attribute offset value. For OLD structures, using
**          u_i1 to ensure that journal records sizes are not changed.
**	29-Apr-2004 (gorvi01)
**		Added constant DM0LDELLOCATION
**	17-may-2004 (thaju02)
**	    Removed unnecessary dum_rnl_name from DM0L_MODIFY struct.
**	    Removed dm0l_modify() rnl_name param.
**	30-jun-2004 (thaju02)
**	    Added bsf_lsn to modify log record.
**	    Added bsf_lsn param to dm0l_modify().
**      11-may-2005 (horda03) Bug 114471/INGSRV3294
**          Added a definition to cover the possible flag values of the
**	    DM0L_HEADER so that dmd_log can display new values when added.
**	    This is needed as change 469031 (and others) have added new flags
**	    but they are not displayed during rollforwarddb (with the
**	    appropriate trace flags defined).
**      29-nov-2005 (horda03) Bug 46859/INGSRV 2716
**          Added DM0L_2ND_BT flag to indicate that this is the 2nd BT record written
**          to the TX log file for the Transaction. This one signifies the start of
**          journalled updates, the first preceded an update to a non-journalled table.
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**	13-Jul-2007 (kibro01) b118695
**	    Added comments to ensure that locations remain at the end
**	    of the DM0L_CREATE/DESTROY/MODIFY structures
**      04-mar-2008 (stial01)
**          Added DM0LJNLSWITCH, DM0L_JNL_SWITCH
**      03-jun-2008 (horda03) 120349
**          DM0L_ET_FLAGS was defined incorrectly ("," at start).
**      27-jun-2008 (stial01)
**          Added param to dm0l_opendb call.
**      14-may-2008 (stial01)
**          Added BTINFO,JEOF to DM0L_RECORD_TYPE
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**      23-Feb-2009 (hanal04) Bug 121652
**          Added dm0l_ufmap() and associated DM0L_UFMAP, DM0LUFMAP.
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	29-Oct-2009 (kschendel) SIR 122739
**	    Add flags to BTPUT, BTDEL, BTFREE log records.  Move some btree-
**	    specific flags out of the header flag word into the btree-
**	    specific part.
**      30-Oct-2009 (horda03) Bug 122823
**          Added DM0L_PARTITION_FILENAMES to indicate to dm0l_allocate
**          that the name parameter is really a list of filenames for
**          the partitioned log file.
**	5-Nov-2009 (kschendel) SIR 122739
**	    Kill old-index log record.  (I fixed a name goof in the current
**	    index log record, don't want confusion from the old one.)
**	04-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bufid parameter to dm0l_read() prototype.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Defines of other constants.
*/

/*
**      dm0l_position flag value.
*/
#define                 DM0L_BY_LGA     1L
#define			DM0L_TRANS	2L
#define			DM0L_FLGA	3L
#define			DM0L_PAGE	4L
#define			DM0L_LAST	5L
#define			DM0L_FIRST	6L

/*
**	dm0l_allocate flag values.
*/
#define			DM0L_NOLOG		0x00001L
#define			DM0L_ARCHIVER		0x00002L
#define			DM0L_RECOVER		0x00004L
#define			DM0L_FULL_FILENAME	0x00008L
#define			DM0L_SCAN_LOGFILE	0x00010L
#define                 DM0L_PARTITION_FILENAMES 0x00020L
/********** BEWARE! The following two flags are overloaded and passed to
***********         dm0l_allocate, but they are actually DM0L_HEADER flags!!
#define			DM0L_FASTCOMMIT		0x0100
#define			DM0L_CKPDB		0x0400
***********/

/*
**	dm0l_drain  flag valuse.
*/
#define			DM0L_STARTDRAIN	1
#define			DM0L_ENDDRAIN	2

/*
** Defines the maximum number of individual pages that can be logged in a
** single log record (given the log record definitions in this file).  It is
** used during recovery operations to allocate arrays of updated locations.
**
** Note: the current definition is actually greater than the largest number
** of pages logged in a single Ingres log record, which is ok.
*/
#define			DM0L_MAX_PAGE_PER_LOG_RECORD	10

#define MAX_RAWDATA_SIZE	(15*1024)	/* Max size of data chunk */

/* The ppchar logger is lazy, and doesn't want to have to split location
** arrays across rawdata chunks.  Make sure our arbitrary size is large
** enough!  (At present it's enough for some 500- locations in one array.)
*/
#if MAX_RAWDATA_SIZE < (DM_LOC_MAX * DB_LOC_MAXNAME)
    XXXX XXXX MAX_RAWDATA_SIZE is too small!
#endif



/*}
** Name: DM0L_HEADER - The standard log record header.
**
** Description:
**      This structure defines the fix portion of a log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
**     24-jan-1989 (EdHsu)
**	    Added Start and End backup log record type for online backup.
**     25-feb-1991 (rogerk)
**	    Added DM0LTEST log record for implementing logging and journaling
**	    system tests.  Added with Archiver Stability project.
**	    Added DM0L_NOLOGGING flag for dm0l_opendb.
**	16-jul-1991 (bryanp)
**	    B38527: Add new DM0L_MODIFY and DM0L_INDEX log records, renaming
**	    the old ones to DM0L_OLD_MODIFY and DM0L_OLD_INDEX.
**	    Also, added new log record DM0L_SM0_CLOSEPURGE.
**	 3-nov-1991 (rogerk)
**	    Added new log record DM0L_OVFL. This records the linking of a
**	    new data page into an overflow chain.
**	14-dec-1992 (rogerk & jnash)
**	    Reduced Logging Project:
**	      - Added new log records DM0LNOFULL, DM0LFMAP.
**	      - Add prev_lsn to header, redefine values for log record types
**		to ensure incompatibility with old log records.
**	      - Added DM0L_PHYS_LOCK log flag to indicate that physical page
**		locking should be used during recovery.
**	      - Added lsn in log record header, required for cluster merge
**		and rollforward.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed DM0L_ADDONLY flag.  Recovery processing now uses LGadd
**		directly (and without special LG_ADDONLY flag).
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Remove DM0L_NOREDO. Use DM0L_NONREDO instead.
**	20-sep-1993 (rogerk)
**	    Added DM0L_NONJNL_TAB record flag.  This indicates a journaled
**	    log record that describes an update associated with a non-journaled
**	    user table.
**	18-oct-1993 (jnash)
**	    Added DM0L_COMP_REPL_OROW in support of replace log compression.
**      24-jan-1995 (stial01)
**          BUG 66473: added DM0L_TBL_CKPT to indicate table checkpoint in
**          SBACKUP records.
**	06-mar-1996 (stial01 for bryanp)
**	    Made "length" a i4 to allow for very long log records. Adds
**	    to the size of every log record in the world, sigh. I think that
**	    this "length" is mostly redundant with the "length" that LG
**	    maintains, so perhaps somehow we could combine the two lengths and
**	    save back this space, someday, perhaps, somehow...
**      22-nov-96 (dilma04)
**          Row Locking Project:
**	    Add DM0L_ROW_LOCK flag to indicate that row locking is enabled.
**          Add DM0L_K_ROW_LOCK to specify if row lock specifed by set lockmode
**      27-feb-97 (stial01)
**          Added DM0L_BT_RECLAIM
** 	10-jun-1997 (wonst02)
** 	    Added DM0L_READONLY_DB for readonly databases.
**      17-jun-97 (stial01)
**          Added DM0L_BT_UNIQUE to specify if btree is unique
**      29-jul-97 (stial01)
**          Added DM0L_REPLACE to specify original mode REPLACE
**	11-Nov-1997 (jenjo02)
**	    Added DM0L_DELAYBT, passed to dm0l_bt() but not written in 
**	    log record header.
**	15-Mar-1999 (jenjo02)
**	    Removed DM0L_DELAYBT.
**      18-jan-2000 (gupsh01)
**          Added flag DM0L_NONJNLTAB to indicate the records which are a 
**          part of transactions involving non-journaled tables for a 
**          journaled database. 
**      30-Mar-2004 (hanal04) Bug 107828 INGSRV2701
**          Added journal flag DM0L_TEMP_IIRELATION. If set to true the
**          table is being used as a temporary copy of iirelation and
**          will be used to recreate a clean version of iirelation (as
**          seen in a sysmod).
**	1-Apr-2004 (schka24)
**	    Add raw-data record type
**	20-Apr-2004 (jenjo02)
**	    Added DM0L_LOGFULL_COMMIT flag for dm0l_et().
**	05-Sep-2006 (jonj)
**	    Add DM0L_ILOG (internal logging) flag for dm0l_bt().
**	11-Sep-2006 (jonj)
**	    Add 2BT to DM0L_HEADER_FLAGS
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add lg_id to DM0L_HEADER.
**	    Add DM0L_CR_HEADER flag to distinguish regular DM0L_HEADER 
**	    from DM0L_CRHEADER.
**	    Add definition of structure DM0L_CRHEADER.
**	    Stole unreferenced DM0L_BT_RECLAIM bit as DM0L_CROW_LOCK
*/
typedef struct
{
    i4	    	    length;		/* Length of this record. */
    u_i2	    lg_id;		/* log_id (id_id) that wrote this record,
    					** supplied by LGwrite */
    i2              type;               /* Type of this record. */

/* Note:  this "type" scheme obviously distinguished between N and 100+N,
** once upon a time, for reasons lost to the mists of antiquity.
** That distinction is no longer relevant, assuming it ever was, and
** the type code is now just a type code.
**
** If you add new types, feel free to fill in some of the gaps.
** (Also, don't forget to update the DM0L_RECORD_TYPE string!)
*/

#define                 DM0LBT          1
#define			DM0LET		2
#define			DM0LBMINI	11
#define			DM0LEMINI	12
#define			DM0LOLDMODIFY	15
#define			DM0LOLDINDEX	17
#define			DM0LBALLOC	18
#define			DM0LBDEALLOC	19
#define			DM0LCALLOC	20
#define			DM0LSAVEPOINT	21
#define			DM0LABORTSAVE	22
#define			DM0LCONFIG	25
#define			DM0LOPENDB	26
#define			DM0LRAWDATA	27
#define			DM0LP1		28
#define			DM0LBPEXTEND	29
#define			DM0LBCP		30
#define                 DM0LLOCATION    33
#define			DM0LSETBOF	34
#define			DM0LJNLEOF	35
#define			DM0LARCHIVE	36
#define			DM0LLPS 	37
#define                 DM0LEXTALTER    38
#define			DM0LECP		40
#define			DM0LSBACKUP	41
#define			DM0LEBACKUP	42
#define			DM0LFSWAP	45
#define			DM0LTEST	46
#define			DM0LINDEX	48
#define			DM0LSM0CLOSEPURGE	49
#define			DM0LBI		103
#define			DM0LPUT		104
#define			DM0LDEL		105
#define			DM0LREP		106
#define			DM0LAI		107
#define			DM0LCREATE	113
#define			DM0LDESTROY	114
#define			DM0LRELOCATE	116
#define			DM0LFRENAME	123
#define			DM0LFCREATE	124
#define                 DM0LSM1RENAME   131
#define                 DM0LSM2CONFIG   132
#define			DM0LALTER 	138
#define			DM0LLOAD	139
#define                 DM0LCRDB        143
#define			DM0LDMU		144
#define			DM0LMODIFY	147
#define                 DM0LASSOC       150
#define                 DM0LALLOC       151
#define                 DM0LDEALLOC     152
#define                 DM0LEXTEND      153
#define			DM0LOVFL	154
#define			DM0LNOFULL	155
#define			DM0LFMAP	157
#define			DM0LCRVERIFY	158
#define			DM0LBTPUTOLD	159 /* INVALID due to b109936 */
#define			DM0LBTDELOLD	160 /* INVALID due to b109936 */
#define			DM0LBTSPLIT	161
#define			DM0LBTOVFL	162
#define			DM0LBTFREE	163
#define			DM0LBTUPDOVFL	164
#define			DM0LDISASSOC	165
#define			DM0LRTDEL	166
#define			DM0LRTPUT	167
#define			DM0LRTREP	168
#define			DM0LBTPUT	169
#define			DM0LBTDEL	170
#define			DM0LBSF		171
#define			DM0LESF		172
#define			DM0LRNLLSN	173
#define         	DM0LDELLOCATION 174
#define			DM0LBTINFO	175
#define			DM0LJNLSWITCH   176
/*		unused			177 */
#define                 DM0LUFMAP       178
/* Before adding more types at the end, did you fill up the gaps? */

/* Name string for printing a log record type with %w -- update if you
** add new log record types!
*/
#define DM0L_RECORD_TYPE \
/*   0 -  10 */ ",BT,ET,OLD_BI,OLD_PUT,OLD_DEL,OLD_REP,OLD_SBI,OLD_SPUT,OLD_SDEL,OLD_SREP," \
/*  11 -  20 */ "BMINI,EMINI,OLD_CREATE,OLD_DESTROY,OLD_MODIFY,OLD_RELOCATE,OLD_INDEX,BALLOC,BDEALLOC,CALLOC," \
/*  21 -  30 */ "SAVEPOINT,ABORTSAVE,OLD_FRENAME,OLD_FCREATE,CONFIG,OPENDB,RAWDATA,P1,BPEXTEND,BCP," \
/*  31 -  40 */ "OLD_SM1RENAME,OLD_SM2CONFIG,LOCATION,SETBOF,JNLEOF,ARCHIVE,LPS,EXTALTER,OLD_LOAD,ECP," \
/*  41 -  50 */ "SBACKUP,EBACKUP,OLD_CRDB,OLD_DMU,FSWAP,TEST,OLD_MODIFY,INDEX,SM0CLOSEPURGE,OLD_ASSOC," \
/*  51 -  60 */ "OLD_ALLOC,OLD_DEALLOC,OLD_EXTEND,OLD_OVERFLOW,55,56,57,58,59,60," \
/*  61 -  70 */ "61,62,63,64,65,66,67,68,69,70," \
/*  71 -  80 */ "71,72,73,74,75,76,77,78,79,80," \
/*  81 -  90 */ "81,82,83,84,85,86,87,88,89,90," \
/*  91 - 100 */ "91,92,93,94,95,96,97,98,99,100," \
/* 101 - 110 */ "101,102,BI,PUT,DEL,REP,AI,108,109,110," \
/* 111 - 120 */ "111,112,CREATE,DESTROY,115,RELOCATE,117,118,119,120," \
/* 121 - 130 */ "121,122,FRENAME,FCREATE,125,126,127,128,129,130," \
/* 131 - 140 */ "SM1RENAME,SM2CONFIG,133,134,135,136,137,ALTER,LOAD,140," \
/* 141 - 150 */ "141,142,CRDB,DMU,145,146,MODIFY,148,149,ASSOC," \
/* 151 - 160 */ "ALLOC,DEALLOC,EXTEND,OVERFLOW,NOFULL,156,FMAP,CRVERIFY,BTPUTOLD,BTDELOLD," \
/* 161 - 170 */ "BTSPLIT,BTOVFL,BTFREE,BTUPDOVFL,DISASSOC,RT_DEL,RT_PUT,RT_REP,BTREE_PUT,BTREE_DEL," \
/* 171 - 180 */ "BSF,ESF,RNLLSN,DELLOCATION,BTINFO,JEOF,177,UFMAP,179,180"

/*
** Definition that is not really used for anything, but marks the highest
** used OLD log record type.
*/
#define			DM0L_64_LAST_USED_TYPE	55

/*
** Definitions of old pre-6.5 log record types.  Saved here in case some
** code wants to decode old journal files.
*/
#define			DM0LBI_PRE65		1
#define			DM0LPUT_PRE65		4
#define			DM0LDEL_PRE65		5
#define			DM0LREP_PRE65		6
#define			DM0LSBI_PRE65		7
#define			DM0LSPUT_PRE65		8
#define			DM0LSDEL_PRE65		9
#define			DM0LSREP_PRE65		10
#define			DM0LCREATE_PRE65	13
#define			DM0LFRENAME_PRE65	23
#define			DM0LFCREATE_PRE65	24
#define			DM0LLOAD_PRE65		39
#define			DM0LDMU_PRE65		44
#define			DM0LMODIFY_PRE65	47
#define                 DM0LASSOC_PRE65		50
#define                 DM0LALLOC_PRE65		51
#define                 DM0LDEALLOC_PRE65	52
#define                 DM0LEXTEND_PRE65	53
#define			DM0LOVFL_PRE65		54

    u_i4	    flags;		/* Flags. */
#define			DM0L_JOURNAL		0x0001
#define			DM0L_ROTRAN		0x0002
#define			DM0L_MASTER		0x0004
#define			DM0L_CR_HEADER		0x0008
#define			DM0L_SPECIAL		0x0010
#define			DM0L_NOTDB		0x0020
#define			DM0L_READONLY_DB	0x0040
#define			DM0L_MINI		0x0080
#define			DM0L_FASTCOMMIT		0x0100
#define			DM0L_NONREDO		0x0200
#define			DM0L_CKPDB		0x0400
#define			DM0L_JON		0x0800
#define			DM0L_WILLING_COMMIT 	0x1000
#define			DM0L_MVCC	 	0x2000
#define			DM0L_PRETEND_CONSISTENT 0x04000L
#define			DM0L_NOLOGGING  	0x08000L
#define                 DM0L_DUMP	     	0x10000L
#define                 DM0L_NONJNL_TAB	     	0x20000L
#define			DM0L_CLR		0x40000L
#define			DM0L_PHYS_LOCK		0x80000L
#define			DM0L_COMP_REPL_OROW	0x100000L
#define                 DM0L_TBL_CKPT           0x200000L
#define                 DM0L_ROW_LOCK           0x400000L
#define                 DM0L_SIDEFILE		0x800000L
#define                 DM0L_CROW_LOCK          0x1000000L
#define                 DM0L_NONJNLTAB		0x8000000L
#define                 DM0L_LOGFULL_COMMIT	0x10000000L
#define                 DM0L_TEMP_IIRELATION    0x20000000L
#define                 DM0L_ILOG    		0x40000000L
#define                 DM0L_2ND_BT    		0x80000000L

/* When new flags added, an appropriate message should be added
** to this list.
*/
#define DM0L_HEADER_FLAGS "JOURNAL,READONLY,MASTER,CRHEAD,SPECIAL,NOTDB,ADDONLY,MINI,FASTCOMMIT,NONREDO,\
CKP,JON,WILLING_COMMIT,MVCC,PRETEND,NOLOGGING,DUMP,NONJNLTAB,CLR,PHYSLOCK,COMP_REPL,TBL_CKPT,\
ROWLOCK,SIDEFILE,CROWLOCK,BT_UNIQUE,REPLACE,NONJNL,TEMP_IIRELATION,ILOG,2BT"

    i4	    	    database_id;	/* External Database Id. */
    DB_TRAN_ID	    tran_id;		/* Transaction identifier. */
    LG_LSN	    lsn;		/* LSN of this record */
    LG_LSN	    compensated_lsn;	/* LSN of log record that this update
					** compensates.  Valid only if the
					** DM0L_CLR flag is asserted. */
}   DM0L_HEADER;

/*}
** Name: DM0L_CRHEADER - Additional header for consistent read undo protocols
**
** Description:
**	Adds some stuff to appropriate log records
**	for Consistent Read quick deconstruction of a page.
**
**	This information lets CR undo quickly find the updates for
**	a specific page in either the active log or in the journal files.
**
**	prev_lsn		LSN of previous update to this page, extracted
**				from the page before updating.
**	prev_lga		LGA of previous update to this page, extracted
**				from the page before updating.
**	prev_jfa		JFA of previous update to this page, extracted
**				from the page before updating.
**	prev_bufid		Log buffer containing prev_lga.
**
**	***WARNING***
**
**	WHEN USED, DM0L_CRHEADER MUST BE PLACED IMMEDIATELY AFTER
**	DM0L_HEADER.
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Created
[@history_template@]...
*/
typedef struct
{
    LG_LSN	    prev_lsn;		/* LSN of previous update to page */
    LG_LA	    prev_lga;		/* LGA of previous update to page */
    LG_JFA	    prev_jfa;		/* JFA of previous update to page */
    i4		    prev_bufid;		/* Buffer containing prev_lga */
}   DM0L_CRHEADER;

/*
** Macro to set CRHEADER elements from a LG_LRI just before calling LGwrite().
**
** It also sets the DM0L_CR_HEADER flag bit in the immediately
** prefixing DM0L_HEADER so LGwrite etal knows that it's there.
*/
#define DM0L_MAKE_CRHEADER_FROM_LRI(lri, crhdr) \
    ((DM0L_HEADER*)((char*)crhdr - sizeof(DM0L_HEADER)))->flags |= DM0L_CR_HEADER; \
    (crhdr)->prev_lsn = (lri)->lri_lsn; \
    (crhdr)->prev_lga = (lri)->lri_lga; \
    (crhdr)->prev_jfa = (lri)->lri_jfa; \
    (crhdr)->prev_bufid = (lri)->lri_bufid;

/*
** Same thing, but don't set flags |= DM0L_CR_HEADER.
** This CRHEADER doesn't immediately follow the DM0L_HEADER
*/
#define DM0L_MAKE_CRHEADER2_FROM_LRI(lri, crhdr) \
    (crhdr)->prev_lsn = (lri)->lri_lsn; \
    (crhdr)->prev_lga = (lri)->lri_lga; \
    (crhdr)->prev_jfa = (lri)->lri_jfa; \
    (crhdr)->prev_bufid = (lri)->lri_bufid;

/* For DMVE undo */
#define DM0L_MAKE_LRI_FROM_LOG_RECORD(lri, hdr) \
    (lri)->lri_lsn = ((DM0L_HEADER*)(hdr))->lsn; \
    (lri)->lri_lg_id = ((DM0L_HEADER*)(hdr))->lg_id; \
    if ( ((DM0L_HEADER*)(hdr))->flags & DM0L_CR_HEADER ) \
    { \
        DM0L_CRHEADER *crhdr = (DM0L_CRHEADER*)((char*)hdr + sizeof(DM0L_HEADER)); \
	(lri)->lri_lga = (crhdr)->prev_lga; \
	(lri)->lri_jfa = (crhdr)->prev_jfa; \
	(lri)->lri_bufid = (crhdr)->prev_bufid; \
    } \
    else \
    { \
        (lri)->lri_lga.la_sequence = 0; \
	(lri)->lri_lga.la_block = 0; \
	(lri)->lri_lga.la_offset = 0; \
	(lri)->lri_jfa.jfa_filseq = 0; \
	(lri)->lri_jfa.jfa_block = 0; \
	(lri)->lri_bufid = 0; \
    }

/* FIXME if we knew record type, and page type, we could choose hdr1/hdr2 */
#define DM0L_MAKE_LRI_FROM_LOG_CRHDR2(lri, hdr) \
    (lri)->lri_lsn = ((DM0L_HEADER*)(hdr))->lsn; \
    (lri)->lri_lg_id = ((DM0L_HEADER*)(hdr))->lg_id; \
    if ( ((DM0L_HEADER*)(hdr))->flags & DM0L_CR_HEADER ) \
    { \
        DM0L_CRHEADER *crhdr = (DM0L_CRHEADER*)((char*)hdr + sizeof(DM0L_HEADER) + sizeof(DM0L_CRHEADER)); \
	(lri)->lri_lga = (crhdr)->prev_lga; \
	(lri)->lri_jfa = (crhdr)->prev_jfa; \
	(lri)->lri_bufid = (crhdr)->prev_bufid; \
    } \
    else \
    { \
        (lri)->lri_lga.la_sequence = 0; \
	(lri)->lri_lga.la_block = 0; \
	(lri)->lri_lga.la_offset = 0; \
	(lri)->lri_jfa.jfa_filseq = 0; \
	(lri)->lri_jfa.jfa_block = 0; \
	(lri)->lri_bufid = 0; \
    }
    

/*}
** Name: DM0L_ADDDB - Special structure passed in LGadd calls.
**
** Description:
**      This special structure is used in calls to LG add to pass
**	information about a database.
**
** History:
**      22-may-1987 (Derek)
**          Created for Jupiter.
[@history_template@]...
*/
typedef struct
{
    DB_DB_NAME		ad_dbname;	/* Database name. */
    DB_OWN_NAME		ad_dbowner;	/* Database owner. */
    i4             ad_dbid;        /* External DB id. */
    i4		ad_l_root;	/* Length of root location name. */
    DM_PATH		ad_root;	/* Root location for database. */
}   DM0L_ADDDB;

/*}
** Name: DM0L_SBACKUP - Start Backup Log Record.
**
** Description:
**      This structure describes the format of a start backup log
**	record.
**
** History:
**     5-jan-1989 (Edhsu)
**          Created for Terminator.
*/
typedef struct
{
    DM0L_HEADER		sbackup_header;	    /* Standard log record header. */
    i4		sbackup_dbid;
}   DM0L_SBACKUP;

/*}
** Name: DM0L_EBACKUP - End Backup Log Record.
**
** Description:
**      This structure describes the format of a end backup log
**	record.
**
** History:
**     5-jan-1989 (Edhsu)
**          Created for Terminator.
*/
typedef struct
{
    DM0L_HEADER		ebackup_header;	    /* Standard log record header. */
    i4		ebackup_dbid;
}   DM0L_EBACKUP;

/*}
** Name: DM0L_BT - Begin Transaction Log Record.
**
** Description:
**      This structure describes the format of a begin transaction log
**	record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		bt_header;	    /* Standard log record header. */
    DB_OWN_NAME		bt_name;            /* Blank padded name of the user. */
    DB_TAB_TIMESTAMP	bt_time;            /* Time transaction started. */
}   DM0L_BT;


/*}
** Name: DM0L_BTINFO - Begin Transaction INFO Log Record.
**
** Description:
**      This structure describes an open transaction, used by incremental rfp.
**
** History:
**     12-apr-2007 (stial01)
**          Created.
*/
typedef struct
{
    DM0L_HEADER		bti_header;	    /* Standard log record header. */
    DM0L_BT		bti_bt;		    /* The original BT record */
    i4			bti_rawcnt;	    /* Raw data count */
    i4			bti_reserved;
}   DM0L_BTINFO;

/*}
** Name: DM0L_ET - End Transaction.
**
** Description:
**      This structure describes the format of an end transaction log record.
**
** History:
**     17-may-1986 (Derek)
**	    Created for Jupiter.
**     01-may-2008 (horda03) Bug 120349
**          Added DM0L_ET_FLAGS
*/
typedef struct
{
    DM0L_HEADER		et_header;	/* Standard log record header. */
    DB_TAB_TIMESTAMP	et_time;	/* Time transaction ended. */
    i4		et_flag;	/* Transaction commit status. */
#define			    DM0L_COMMIT	    0x0001
#define			    DM0L_ABORT	    0x0002
#define			    DM0L_DBINCONST  0x0004

#define DM0L_ET_FLAGS "COMMIT,ABORT,DBINCONST"
     i4		et_master;	/* Who aborted the transaction. */
#define			    DM0L_AUTO	1
}   DM0L_ET;

/*}
** Name: DM0L_CRDB - Create Database.
**
** Description:
**      This structure describes the format of create database log record.
**
** History:
**     16-aug-1991 (jnash) merged 17-may-1986 (Derek)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER         cd_header;      /* Standard log record header. */
    i4             cd_service;     /* Database service code. */
    i4             cd_access;      /* Database access code. */
    i4             cd_dbid;        /* Database identifier. */
    DB_DB_NAME          cd_dbname;      /* Name of database. */
    DB_LOC_NAME         cd_location;    /* Logical location name. */
    i4             cd_l_root;      /* Length of root location name. */
    DM_PATH             cd_root;        /* Root location for database. */
}   DM0L_CRDB;

/*}
** Name: DM0L_BI - Before Image log Record.
**
** Description:
**      This structure describes the format of a before image log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
**	06-mar-1996 (stial01 for bryanp)
**	    Add bi_page_size.
**      01-may-1996 (stial01)
**          Variable size before page image follows DM0L_BI.
**          DM0L_BI: remove bi_page
**	10-jan-1997 (nanpr01)
**	    A previous change has taken out the bi_page to make
**	    the LGreserve calculation simpler but making it vulnerable to
**	    alignment issues for further development. 
**      02-may-2008 (horda03) bug 120349
**          Add DM0L_BI_OPERATION define.
*/
typedef struct
{
    DM0L_HEADER		bi_header;	/* Standard log record header. */
    DM0L_CRHEADER	bi_crhdr;	/* Header for consistent read */
    DB_TAB_ID		bi_tbl_id;	/* Table Identifier. */
    DB_TAB_NAME		bi_tblname;	/* Table name. */
    DB_OWN_NAME  	bi_tblowner;	/* Table owner. */
    i2			bi_pg_type;	/* Page type */
    i2			bi_loc_cnt;	/* Location count */
    i2			bi_loc_id;	/* Loc offset in config file */
    i2			bi_operation;   /* Operation requiring Before Image */
#define			  DM0L_BI_DUMP		1L
#define			  DM0L_BI_NEWROOT	2L
#define			  DM0L_BI_JOIN		3L
#define			  DM0L_BI_SPLIT		4L

#define DM0L_BI_OPERATION ",DUMP,NEWROOT,JOIN,SPLIT"

    DM_PAGENO		bi_pageno;	/* The page number. */
    i4			bi_page_size;	/* page size. */
    DMPP_PAGE		bi_page;	/* space holder for the page */
}   DM0L_BI;

/*}
** Name: DM0L_AI - After Image log Record.
**
** Description:
**      This structure describes the format of an after image log record.
**
** History:
**     14-dec-1992 (Derek)
**          Written for the Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add ai_page_size.
**      01-may-1996 (stial01)
**          Variable size after page image follows DM0L_AI.
**          DM0L_AI: remove ai_page
**	10-jan-1997 (nanpr01)
**	    A previous change has taken out the ai_page to make
**	    the LGreserve calculation simpler but making it vulnerable to
**	    alignment issues for further development. 
*/
typedef struct
{
    DM0L_HEADER		ai_header;	/* Standard log record header. */
    DB_TAB_ID		ai_tbl_id;	/* Table Identifier. */
    DB_TAB_NAME		ai_tblname;	/* Table name. */
    DB_OWN_NAME  	ai_tblowner;	/* Table owner. */
    i2			ai_pg_type;	/* Page type */
    i2			ai_loc_cnt;	/* Location count */
    i2			ai_loc_id;	/* Loc offset in config file */
    i2			ai_operation;   /* Operation requiring After Image */
/* Possible values are a subset of DM0L_BI_OPERATION */
    DM_PAGENO		ai_pageno;	/* The page number. */
    i4			ai_page_size;	/* page size */
    DMPP_PAGE		ai_page;	/* space holder for the page */
}   DM0L_AI;

/*}
** Name: DM0L_PUT - Put record log record.
**
** Description:
**      This structure describes a put record log record.
**
** History:
**	17-may-1986 (Derek)
**          Created for Jupiter.
**	26-oct-1992 (jnash)
**	    Modified for 6.5 recovery.
**	06-mar-1996 (stial01 for bryanp)
**	    Add put_page_size to hold table's page size.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**      22-jul-1996 (ramra01 for bryanp)
**          Add support for Alter Table:
**              Add put_row_version to the DM0L_PUT log record.
*/
typedef struct
{
    DM0L_HEADER		put_header;	/* Standard log record header. */
    DM0L_CRHEADER	put_crhdr;	/* Header for consistent read */
    DB_TAB_ID		put_tbl_id;	/* Table Identifier. */
    DM_TID		put_tid;	/* Location of new record. */
    i4			put_page_size;	/* page size of this table. */
    i2			put_pg_type;	/* page type */
    i2			put_cnf_loc_id; /* Loc offset in config file */
    i2			put_loc_cnt;	/* Location count */
    i2			put_tab_size;	/* Size of table name field */
    i2			put_own_size;	/* Size of owner name field */
    i4			put_rec_size;	/* Size of record. */
    u_i2		put_row_version;/* Row Version #*/
    DMPP_SEG_HDR	put_seg_hdr;	/* Segment header */
    i2                  put_comptype;   /* compression type */
    char		put_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME)];
}   DM0L_PUT;

/*}
** Name: DM0L_DEL - Delete record log record.
**
** Description:
**      This structure defines the format of a delete record log record.
**
** History:
**	17-may-1986 (Derek)
**          Created for Jupiter.
**	26-oct-1992 (jnash & rogerk)
**	    Modified for 6.5 recovery.
**	06-mar-1996 (stial01 for bryanp)
**	    Add del_page_size to hold table's page size.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**      22-jul-1996 (ramra01 for bryanp)
**          Add support for Alter Table:
**              Add del_row_version to the DM0L_DEL log record.
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add del_otran_id, del_olg_id.
*/
typedef struct
{
    DM0L_HEADER		del_header;     /* Standard log record header. */
    DM0L_CRHEADER	del_crhdr;	/* Header for consistent read */
    DB_TAB_ID		del_tbl_id;	/* Table Identifier. */
    DM_TID		del_tid;	/* Location of new record. */
    i4			del_page_size;	/* page size of table. */
    i2			del_pg_type;	/* page type */
    i2			del_cnf_loc_id; /* Loc offset in config file */
    i2			del_loc_cnt;	/* Location count */
    i2			del_tab_size;	/* Size of table name field */
    i2			del_own_size;	/* Size of owner name field */
    i4			del_rec_size;	/* Size of record. */
    u_i2		del_row_version;/* Version of Row */
    DMPP_SEG_HDR	del_seg_hdr;	/* Segment header */
    i2                  del_comptype;   /* compression type */
    u_i2		del_olg_id;	/* lg_id of prev change to row */
    u_i4		del_otran_id;	/* tran_id of prev change to row */
    char		del_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME)];
}   DM0L_DEL;

/*}
** Name: DM0L_REP - Replace record log record.
**
** Description:
**      This structure defines the format of a replace record log record.
**
**	Replace records are stored in a special compressed format to avoid
**	logging common portions of the old and new versions of the row.
**
**	By default, the replace log record will log a full version of the
**	row before image and then only the portion of the after image that
**	changed in the replace.  The field "rep_diff_offset" gives the offset
**	into the record where the versions differ and the field "rep_ndata_len"
**	gives the number of bytes in which the record differs.
**
**	If ultra-compressed logging (DM0L_COMP_REPL_OROW) is specified then
**	a full version of the before image is not logged.  Only the changed
**	portion of the before and after images are logged.  Again, the field
**	"rep_diff_offset" gives the offset into the record where the versions
**	differ, the field "rep_odata_len" gives the number of bytes in the
**	changed portion of the old row, and "rep_ndata_len" gives the number
**	of bytes in the changed portion of the new row.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
**	26-oct-1992 (jnash & rogerk)
**	    Modified for 6.5 recovery.
**	20-sep-1993 (rogerk)
**	    Remove declared name,owner fields from replace log record as
**	    these are stored in compressed fashion at the end of the log
**	    record.
**	18-oct-1993 (jnash)
**	    Added rep_nrec_size, rep_comp_rec_size and rep_comp_rec_offset
**	    to allow row compression.
**	 3-jan-1993 (rogerk)
**	    Changed logging of compressed replace records.  Added rep_odata_len
**	    and rep_ndata_len to hold the sizes of the compressed data which
**	    describes the before and after images of the replace row.  The
**	    rep_orec_size and rep_nrec_size now describe the full sizes of
**	    the row before and after images.
**	06-mar-1996 (stial01 for bryanp)
**	    Add rep_page_size to hold table's page size.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**      22-jul-1996 (ramra01 for bryanp)
**          Add support for Alter Table:
**              Add rep_orow_version and rep_nrow_version to the
**                  DM0L_REP log record.
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add rep_otran_id, rep_olg_id.
*/
typedef struct
{
    DM0L_HEADER		rep_header;     /* Standard log record header. */
    DM0L_CRHEADER	rep_crhdr;	/* Header for consistent read */
    DB_TAB_ID		rep_tbl_id;	/* Table Identifier. */
    DM_TID		rep_otid;	/* Location of old record. */
    DM_TID		rep_ntid;	/* Location of new record. */
    i4			rep_page_size;	/* page size of table. */
    i2			rep_pg_type;	/* page type */
    i2			rep_ocnf_loc_id; /* Old Loc offset in config file */
    i2			rep_ncnf_loc_id; /* New Loc offset in config file */
    i2			rep_loc_cnt;	/* Location count */
    i2			rep_tab_size;	/* Size of table name field */
    i2			rep_own_size;	/* Size of owner name field */
    i4			rep_orec_size;	/* Size of old record. */
    i4			rep_nrec_size;	/* Size of new record. */
    i4			rep_odata_len;	/* Length of data in rep_vbuf that forms
					** the before image of replace row. */
    i4			rep_ndata_len;	/* Length of data in rep_vbuf that forms
					** the after image of replace row. */
    i4			rep_diff_offset;/* Offset in record to spot where old
					** old and new row versions differ. */
    u_i2		rep_orow_version;/* old row version n#*/
    u_i2		rep_nrow_version;/* new row version n#*/
    i2                  rep_comptype;    /* compression type */
    u_i2		rep_olg_id;	/* lg_id of prev change to row */
    u_i4		rep_otran_id;	/* tran_id of prev change to row */
    char		rep_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME)];
}   DM0L_REP;

/*}
** Name: DM0L_BM - Begin Mini-Transaction log record.
**
** Description:
**      This structure describes a begin mini-transaction log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		bm_header;      /* Standard log record header. */
}   DM0L_BM;

/*}
** Name: DM0L_EM - End Mini-Transaction log record.
**
** Description:
**      This structure describes the end mini-transaction log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed the Log Address field from the End Mini log record which
**		pointed back to the Begin Mini record.  End Mini records now
**		look like CLR's and the log header's compensated_lsn field
**		points back to the Begin Mini.
*/
typedef struct
{
    DM0L_HEADER		em_header;      /* Standard log record header. */
}   DM0L_EM;

/*}
** Name: DM0L_BALLOC - Btree Allocate log record.
**
** Description:
**      This structure describes a btree allocate log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		ba_header;      /* Standard log record header. */
    DB_TAB_ID		ba_tbl_id;	/* The table being allocated. */
    DM_PAGENO		ba_free;	/* The page allocated. */
    DM_PAGENO		ba_next_free;	/* The next free page. */
}   DM0L_BALLOC;

/*}
** Name: DM0L_BDE   - Btree Deallocate log record.
**
** Description:
**      This structure describes a btree deallocation log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		bd_header;      /* Standard log record header. */
    DB_TAB_ID		bd_tbl_id;	/* The table being allocated. */
    DM_PAGENO		bd_free;	/* The page being freed. */
    DM_PAGENO		bd_next_free;	/* The first free page. */
}   DM0L_BDE;

/*}
** Name: DM0L_CALLOC - Common Allocate log record.
**
** Description:
**      This structure describes the common allocate log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		ca_header;      /* Standard log record header. */
    DB_TAB_ID		ca_tbl_id;	/* The table being allocated. */
    DM_PAGENO		ca_free;	/* The page being allocated. */
    DM_PAGENO		ca_root;	/* The root page number. */
    DMPP_PAGE		ca_page;	/* Image of root page. */
}   DM0L_CALLOC;

/*}
** Name: DM0L_SAVEPOINT - Savepoint log record.
**
** Description:
**      This structure descibes the format of a savepoint log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		s_header;       /* Standard log record header. */
    i4		s_id;		/* Savepoint Name. */
}   DM0L_SAVEPOINT;

/*}
** Name: DM0L_ABORT_SAVE - Abort to savepoint log record.
**
** Description:
**      This structure describes an abort to savepoint log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		as_header;      /* Standard log record header. */
    i4 		as_id;		/* Savepoint Id. */
}   DM0L_ABORT_SAVE;

/*}
** Name: DM0L_FRENAME - Rename file log record.
**
** Description:
**      This structure describes the format of a rename file log record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
**	28-oct-1992 (jnash)
**	    Reduced logging project.  Add table_id, loc_id and
**	    cnf_loc_id.
**      06-mar-1996 (stial01)
**          Added page size
**	21-mar-1999 (nanpr01)
**	    removed the redundant page_size.
*/
typedef struct
{
    DM0L_HEADER		fr_header;      /* Standard log record header. */
    i4		fr_l_path;	/* Length of path. */
    DM_PATH		fr_path;	/* Path to files. */
    DM_FILE		fr_oldname;	/* Old file id. */
    DM_FILE		fr_newname;	/* New file name. */
    DB_TAB_ID		fr_tbl_id;  	/* Table Identifier. */
    i2			fr_loc_id; 	/* Offset in location array */
    i2			fr_cnf_loc_id; 	/* Config file offset */
}   DM0L_FRENAME;

/*}
** Name: DM0L_FCREATE - Create File log record.
**
** Description:
**      This structure describes the format of a create file log record.
**
** History:
**      17-may-1986 (Derek)
**          Created for Jupiter.
**	16-aug-1991 (jnash) merged 12-mar-1991 (Derek)
**          Added allocation amount, multi-file position and multi-file
**          maximum.  The latter information is used to determine if a
**          a FHDR and FMAP pages have to be initialized.
**	28-oct-1992 (jnash)
**	    Reduced logging project.  Eliminate fc_flag,
**	    fc_mf_max and allocation.
**	23-aug-1993 (rogerk)
**	    Added location id and table_id information for partial recovery
**	    considerations.
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for table-specific page sizes:
**		Add fc_page_size to the DM0L_FCREATE log record.
**	21-mar-1999 (nanpr01)
**	    Added support for raw location.
*/
typedef struct
{
    DM0L_HEADER		fc_header;      /* Standard log record header. */
    DB_TAB_ID		fc_tbl_id;  	/* Table Identifier. */
    i2			fc_l_path;	/* Length of path. */
    i2			fc_padding;
    i2			fc_loc_id; 	/* Offset in location array */
    i2			fc_cnf_loc_id; 	/* Config file offset */
    i4			fc_page_size;	/* page size for file(s) */
    i4			fc_loc_status;	/* status of location .. raw ?*/
    DM_PATH		fc_path;	/* Path to files. */
    DM_FILE		fc_file;	/* New file name. */
}   DM0L_FCREATE;

/*}
** Name: DM0L_OPENDB - Open Database log record.
**
** Description:
**      This structure defines the format of a open database log record.
**
**	In 6.4, this log record was sometimes marked journalled and sometimes
**	not marked journalled. If the log record was marked journalled, then
**	the interpretation was that the database it described was journaled.
**	However, this log record was never actually copied to the journal
**	files. This led to some strange places in the code where we had to
**	treat this record as a special case. In 6.5, this log record is never
**	marked journaled, and is never copied to the journal files. If the
**	database being opened is journalled, then the o_dbjournaled field is
**	non-zero; else it is zero.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
**	26-apr-1993 (bryanp)
**	    Added o_dbjournaled so that we can record whether a database is
**	    journaled or not. This is used by CSP archiving to detect journaled
**	    databases by examing the OPENDB records in the log file.
*/
typedef struct
{
    DM0L_HEADER		o_header;       /* Standard log record header. */
    DB_DB_NAME		o_dbname;	/* Database name. */
    DB_OWN_NAME		o_dbowner;	/* Database owner. */
    i4             o_dbid;         /* External Database Id. */
    i4		o_isjournaled;	/* database "is-journaled" status:
					** 0 ==> database is not journaled.
					** 1 ==> database IS journaled.
					*/
    i4		o_l_root;	/* Length of root location name. */
    DM_PATH		o_root;		/* Root location for database. */
}   DM0L_OPENDB;

/*}
** Name: DM0L_CONFIG - Configuration File change log record.
**
** Description:
**      This structure defines the format of a configuration file change record.
**
** History:
**     17-may-1986 (Derek)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		cnf_header;         /* Standard log record header. */
    i4		cnf_type;	    /* Type of configuration change. */
#define			    CNF_EXT	    1
#define			    CNF_CNF	    2
#define			    CNF_DSC	    3
#define			    CNF_TAB	    4
    i4		cnf_operation;	    /* Operation of changes. */
#define			    CNF_ADD	    1
#define			    CNF_DEL	    2
#define			    CNF_REP	    3
    i4		cnf_offset;	    /* Offset in config file to
                                            ** CNF entry. */
    i4		cnf_size;	    /* Size of CNF entry in
                                            ** log record. */
    i4		cnf_entry;	    /* Marks the position of
                                            ** the old and
					    ** the new CNF entry values. */
}   DM0L_CONFIG;

/*}
** Name: DM0L_CREATE - Create table log record.
**
** Description:
**      This structure describes the format of a create table log record.
**
** History:
**     18-may-1986 (Derek)
**          Created for Jupiter.
**      22-nov-87 (jennifer)
**          Added multiple location support for tables.
**     16-aug-1991 (jnash)
**          Added allocation and extend for 6.5 merge.
**	18-Oct-1991 (rmuth)
**	    Move the extend and allocation fields above the location array.
**	13-nov-1992 (jnash)
**	    Reduced logging project.  Recreated for use with new recovery
**	    algorithms.  This log now contains all information required
**	    to recreate a table.
**	15-may-1993 (rmuth)
**	    Allow concurrent Indicies, add a flag field to DM0L_CREATE.
**	23-aug-1993 (rogerk)
**	    Added duc_status field.
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for table-specific page sizes:
**		Add duc_page_size to the DM0L_CREATE log record.
*/
typedef struct
{
    DM0L_HEADER		duc_header;         /* Standard log record header. */
    DB_TAB_ID		duc_tbl_id;	    /* Table Identifier. */
    DB_TAB_NAME		duc_name;	    /* Table name (not used) */
    DB_OWN_NAME  	duc_owner;	    /* Table owner (not used) */
    i2			duc_allocation;     /* Allocation amount. */
    i2			duc_loc_count;	    /* Number of file locations */
    i4		duc_flags;	    /* Flags field */
#define DM0L_CREATE_CONCURRENT_INDEX	0x1L
					    /* Create for a concurrent index */
    i4		duc_status;	    /* IIrelation relstat value */
    i4		duc_page_size;	    /* page size of table */
    i4             duc_page_type;      /* page type of table */
    DM_PAGENO 		duc_fhdr;	    /* FHDR page number */
    DM_PAGENO		duc_first_fmap;     /* First FMAP page number */
    DM_PAGENO		duc_last_fmap;      /* Last FMAP page number */
    DM_PAGENO		duc_first_free;	    /* First free page number */
    DM_PAGENO 		duc_last_free;	    /* Last free page number */
    DM_PAGENO		duc_first_used;     /* First used page number */
    DM_PAGENO 		duc_last_used;      /* Last used page number */
    DB_LOC_NAME		duc_location[DM_LOC_MAX]; /* Location array */
    /* DO NOT ADD ANYTHING to this structure after duc_location - if there are
    ** more than DM_LOC_MAX locations (caused by multiple partitions) this array
    ** can overrun (memory is allocated for this).  duc_location must be the
    ** last element in this structure for dm0l.c - see b118695 (kibro01)
    */
}   DM0L_CREATE;

/*}
** Name: DM0L_CRVERIFY - Verify Create table log record.
**
** Description:
**      This structure describes the format of a verify create table log
**	record.
**
** History:
**	13-nov-1992 (jnash)
**	    Created for the Reduced logging project.
**	06-mar-1996 (stial01 for bryanp)
**	    Added ducv_page_size.
*/
typedef struct
{
    DM0L_HEADER		ducv_header;        /* Standard log record header */
    DB_TAB_ID		ducv_tbl_id;	    /* Table Identifier */
    DB_TAB_NAME		ducv_name;	    /* Table name */
    DB_OWN_NAME		ducv_owner;	    /* Table owner */
    i2			ducv_allocation;    /* Allocation amount */
    i2			ducv_num_fmaps;	    /* Number of fmaps */
    i4	 	ducv_fhdr_pageno;   /* Fheader page number */
    i4		ducv_relpages;      /* Number of pages in the file */
    i4		ducv_relprim;
    i4		ducv_relmain;
    i4		ducv_page_size;	    /* page size of table */
}   DM0L_CRVERIFY;

/*}
** Name: DM0L_DESTROY - Destroy Table log record.
**
** Description:
**      The structure describes the format of a destroy table log record.
**
** History:
**     18-may-1986 (Derek)
**          Created for Jupiter.
**      22-nov-87 (jennifer)
**          Added multiple location support for tables.
**	28-nov-92 (jnash)
**	    Reduced logging project.  dud_loc_count changed to i2.
*/
typedef struct
{
    DM0L_HEADER		dud_header;         /* Standard log record header. */
    DB_TAB_ID		dud_tbl_id;	    /* Table Identifier. */
    DB_TAB_NAME		dud_name;	    /* Table name. */
    DB_OWN_NAME  	dud_owner;	    /* Table owner. */
    i2 			dud_loc_count;      /* Number of locations. */
    i2 			dud_padding;
    DB_LOC_NAME		dud_location[DM_LOC_MAX];
                                	    /* Location array. */
    /* DO NOT ADD ANYTHING to this structure after dud_location - if there are
    ** more than DM_LOC_MAX locations (caused by multiple partitions) this array
    ** can overrun (memory is allocated for this).  dud_location must be the
    ** last element in this structure for dm0l.c - see b118695 (kibro01)
    */
}   DM0L_DESTROY;

/*}
** Name: DM0L_RELOCATE - Relocate table log record.
**
** Description:
**      This structure defines the format of a relocate table log record.
**
** History:
**     18-may-1986 (Derek)
**          Created for Jupiter.
**     26-nov-1992 (jnash)
**          Reduced logging project.  Add dur_ocnf_loc_id, dur_ncnf_loc_id
**	    and dur_file.   This log is now responsible for creating
**	    the file at the new location.
*/
typedef struct
{
    DM0L_HEADER		dur_header;         /* Standard log record header. */
    DB_TAB_ID		dur_tbl_id;	    /* Table Identifier. */
    DB_TAB_NAME		dur_name;	    /* Table name. */
    DB_OWN_NAME  	dur_owner;	    /* Table owner. */
    DB_LOC_NAME		dur_olocation;	    /* Old table. */
    DB_LOC_NAME		dur_nlocation;	    /* New location. */
    i4             dur_page_size;      /* Page size */
    i2			dur_ocnf_loc_id;    /* Old loc offset in config file */
    i2			dur_ncnf_loc_id;    /* New loc offset in config file */
    i2			dur_loc_id;         /* Location id. */
    DM_FILE             dur_file;  	    /* Name of new file. */
}   DM0L_RELOCATE;

/*}
** Name: DM0L_OLD_MODIFY - Modify Table log record.
**
** Description:
**      This structure defines the format of modify table log record.
**
**	As part of bugfix for B38527, this is now the OLD modify record
**	format. New modify log records contain the dum_reltups field.
**	Except for the dum_reltups field, the two records are identical and
**	are treated equally by auditdb, rollforwarddb, etc.
**
**	Eventually we will get rid of this log record, once we no longer need
**	to support 6.3-era journal files.
**
** History:
**     18-may-1986 (Derek)
**          Created for Jupiter.
**     18-jun-1986 (ac)
**	    Added DM0L_MERGE for the special handling of modify to merge.
**	    Added key_order arrary to log the decending/ascending order
**	    of the key.
**      22-nov-87 (jennifer)
**          Added multiple location support for tables.
**	16-jul-1991 (bryanp)
**	    B38527: Renamed to DM0L_OLD_MODIFY.
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          dum_key now defined as a u_i1, not
**          char. This addresses the simple problem of
**          char being signed on some problems. The
**          problem of having to store an attribute
**          number >255 still needs to be addressed, but
**          this fix will invalidate existing journal
**          records.
**      01-may-2008 (horda03) Bug 120349
**          Moved DM0L_REORG here and removed duplicate defs.
**          Added DM0L_DUM_STRUCTS for dum_structure meaning.
*/
typedef struct
{
    DM0L_HEADER		dum_header;         /* Standard log record header. */
    DB_TAB_ID		dum_tbl_id;	    /* Table Identifier. */
    DB_TAB_NAME		dum_name;	    /* Table name. */
    DB_OWN_NAME   	dum_owner;	    /* Table owner. */
    i4             dum_loc_count;      /* Number of locations. */
    i4		dum_structure;	    /* Storage Structure. */
#define			DM0L_MERGE	    12L
#define			DM0L_TRUNCATE	    13L
#define			DM0L_SYSMOD	    14L
#define                 DM0L_REORG          15L

   /* Note including meanings for TCB_HEAP, TCB_ISAM, TCB_HASH and TCB_BTREE */
#define DM0L_DUM_STRUCTS ",,,HEAP,,ISAM,,HASH,,,,BTREE,MERGE,TRUNCATE,SYSMOD,REORG"

    i4		dum_status;	    /* Relation status. */
    i4		dum_min;	    /* Minpages. */
    i4		dum_max;	    /* Maxpages. */
    i4		dum_ifill;	    /* Index Fill Factor. */
    i4		dum_dfill;	    /* Data Fill Factor. */
    i4		dum_lfill;	    /* Leaf Fill Factor. */
    u_i1		dum_key[DB_MAX_COLS]; /* Attribute position .vs.
					    ** key position map. */
    char		dum_order[DB_MAX_COLS];	/* Ascending/decending order
						** of the corresponding key.
						** 0 means ascending; 1 means
						** decending. */
    DB_LOC_NAME		dum_location[DM_LOC_MAX];
                                	    /* Location array. */
}   DM0L_OLD_MODIFY;

/*}
** Name: DM0L_MODIFY - Modify Table log record.
**
** Description:
**      This structure defines the format of modify table log record.
**
** History:
**     18-may-1986 (Derek)
**          Created for Jupiter.
**     18-jun-1986 (ac)
**	    Added DM0L_MERGE for the special handling of modify to merge.
**	    Added key_order arrary to log the decending/ascending order
**	    of the key.
**      22-nov-87 (jennifer)
**          Added multiple location support for tables.
**	16-jul-1991 (bryanp)
**	    B38527: Added dum_reltups field to store tuple count for sysmod.
**      16-aug-1991 (jnash) for Derek
**          Added allocation and extend for 6.5 merge.
**      24-sep-1991 (jnash) merged 30-aug-1991 (rogerk)
**          Added DM0L_REORG flag for MODIFY log record to indicate a
**          modify to reorganize operation.  Rollforward of reorganize was
**          failing since we could not distiguish this operation from other
**          types of modifies.
**	18-Oct-1991 (rmuth)
**	    Move the extend and allocation fields above the array fields.
**	23-aug-1993 (rogerk)
**	    Added a statement count field to the modify and index log records.
**	    This is used in modify rollfwd to correctly find the modify files
**	    to load that were re-created by dm0l_fcreate.
**	18-oct-1993 (rogerk)
**	    Added dum_buckets so rollforward would be able to correctly
**	    remodify a hash table with the same number of hash buckets.
**	    Changed documentation on dum_reltups - we now log the result
**	    number of rows of the modify in all structural modifications.
**	06-mar-1996 (stial01 for bryanp)
**	    Add dum_page_size field.
**	6-Feb-2004 (schka24)
**	    Extend stmt-count, rename it.
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          dum_key now defined as a DM_ATT, not
**          char. This addresses the simple problem of
**          char being signed on some problems. The
**          problem of having to store an attribute
**          number >255 still needs to be addressed, but
**          this fix will invalidate existing journal
**          records.
**	17-may-2004 (thaju02)
**	    Removed unnecessary dum_rnl_name.
**	23-Oct-2009 (kschendel) SIR 122739
**	    Don't lose hidata compression if redoing a modify.
**	9-Jul-2010 (kschendel) SIR 123450
**	    We now have yet another compression type flag.  (We're at
**	    10.0 beta release, not the time to change log record format!)
*/
typedef struct
{
    DM0L_HEADER		dum_header;         /* Standard log record header. */
    DB_TAB_ID		dum_tbl_id;	    /* Table Identifier. */
    DB_TAB_NAME		dum_name;	    /* Table name. */
    DB_OWN_NAME   	dum_owner;	    /* Table owner. */
    i4             dum_loc_count;      /* Number of locations. */
    i4		dum_structure;	    /* Storage Structure. */
       /* For permitted values see dum_structure in DM0L_OLD_MODIFY */
    i4		dum_status;	    /* Relation status. */
    i4		dum_min;	    /* Minpages. */
    i4		dum_max;	    /* Maxpages. */
    i4		dum_ifill;	    /* Index Fill Factor. */
    i4		dum_dfill;	    /* Data Fill Factor. */
    i4		dum_lfill;	    /* Leaf Fill Factor. */
    i4		dum_reltups;	    /* # of rows in modified table */
    i4             dum_allocation;     /* Allocation amount. */
    i4             dum_extend;         /* Extend amount. */
    i4		dum_page_size;	    /* page size */
    i2          dum_pg_type;        /* page type */
    i2		dum_name_id;		/* Modify .mxx filename id */
    i4		dum_name_gen;		/* Modify .mxx filename generator
					** Used along with the id to tell
					** modify what filename it has to
					** use for the temp file that has
					** already been fcreate'd.
					*/
    i4		dum_buckets;	    /* # of main hash bucket pages if
					    ** modify is to HASH structure */
    i4		dum_flag;	    /* various flags */
#define		DM0L_ONLINE			0x01
#define		DM0L_START_ONLINE_MODIFY	0x02
/* FIXME reduce dum-flag to an i2, take the other i2 for a dum_comptype.
** But for now, just pass a hidata compression flag.
*/
#define		DM0L_MODIFY_HICOMPRESS		0x04
/* See FIXME above!  More silliness! */
#define		DM0L_MODIFY_NEWCOMPRESS		0x08	/* New-standard */

    DB_TAB_ID   dum_rnl_tbl_id;
    LG_LSN	dum_bsf_lsn;
    DM_ATT		dum_key[DB_MAX_COLS]; /* Attribute position .vs.
					    ** key position map. */
    char		dum_order[DB_MAX_COLS];	/* Ascending/decending order
						** of the corresponding key.
						** 0 means ascending; 1 means
						** decending. */
    DB_LOC_NAME		dum_location[DM_LOC_MAX];
                                	    /* Location array. */
    /* DO NOT ADD ANYTHING to this structure after dum_location - if there are
    ** more than DM_LOC_MAX locations (caused by multiple partitions) this array
    ** can overrun (memory is allocated for this).  dum_location must be the
    ** last element in this structure for dm0l.c - see b118695 (kibro01)
    */
}   DM0L_MODIFY;

/*}
** Name: DM0L_INDEX - Index Table log record.
**
** Description:
**      This structure defines the format of a INDEX table log record.
**
** History:
**     18-may-1986 (Derek)
**	    Created for Jupiter.
**      22-nov-87 (jennifer)
**          Added multiple location support for tables.
**	16-jul-1991 (bryanp)
**	    B38527: Added dui_reltups field to store tuple count for sysmod.
**      16-aug-1991 (jnash)
**          Added allocation and extend for 6.5 merge.
**	18-Oct-1991 (rmuth)
**	    Move the extend and allocation fields above the array fields.
**	23-aug-1993 (rogerk)
**	    Added a statement count field to the modify and index log records.
**	    This is used in index rollfwd to correctly find the modify files
**	    to load that were re-created by dm0l_fcreate.
**	18-oct-1993 (rogerk)
**	    Added dui_buckets so rollforward would be able to correctly
**	    recreate a hash index with the same number of hash buckets.
**	    Changed documentation on dui_reltups - we now log the result
**	    number of rows of the index in all index builds.
**	23-may-1996 (shero03)
**	    Rtree: add dimension, hilbertsize, range to dm0l_index
**	23-Dec-2003 (jenjo02)
**	    Added dui_relstat2 for Global Indexes, Partitioning.
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          dui_key now defined as a DM_ATT, not
**          char. This addresses the simple problem of
**          char being signed on some problems. The
**          problem of having to store an attribute
**          number >255 still needs to be addressed, but
**          this fix will invalidate existing journal
**          records.
**	30-May-2006 (jenjo02)
**	    dui_key dimension changed from DB_MAXKEYS to DB_MAXIXATTS
**	5-Nov-2009 (kschendel) SIR 122739
**	    Rename acount to kcount since that's what it is.
**	16-Jul-2010 (kschendel) SIR 123450
**	    Squeeze in data, key compression types.
*/
typedef struct
{
    DM0L_HEADER		dui_header;         /* Standard log record header. */
    DB_TAB_ID		dui_tbl_id;	    /* Table Identifier. */
    DB_TAB_ID		dui_idx_tbl_id;	    /* Index Table Identifier. */
    DB_TAB_NAME		dui_name;	    /* Table name. */
    DB_OWN_NAME  	dui_owner;	    /* Table owner. */
    i4             	dui_loc_count;      /* Number of locations. */
    i4			dui_structure;	    /* Storage Structure. */
    u_i4		dui_status;	    /* Relation relstat. */
    u_i4		dui_relstat2;	    /* Relation relstat2 */
    i4			dui_min;	    /* Minpages. */
    i4			dui_max;	    /* Maxpages. */
    i4			dui_ifill;	    /* Index Fill Factor. */
    i4			dui_dfill;	    /* Data Fill Factor. */
    i4			dui_lfill;	    /* Leaf Fill Factor. */
    i2			dui_kcount;	    /* Key attribute count. */
    i2			dui_comptype;	    /* Data compression type if any */
    i4			dui_reltups;	    /* # of rows in secondary index */
    i4             	dui_allocation;     /* Allocation amount. */
    i4             	dui_extend;         /* Extend amount. */
    i4			dui_page_size;	    /* page size */
    i2          	dui_pg_type;        /* page type */
    i2			dui_name_id;	    /* Index .mxx filename id */
    i4			dui_name_gen;	    /* Index .mxx filename generator
					    ** Used along with the id to tell
					    ** Index what filename it has to
					    ** use for the temp file that has
					    ** already been fcreate'd.
					    */
    i4			dui_buckets;	    /* # of main hash bucket pages if
					    ** index is of HASH structure */
    i2			dui_ixcomptype;	    /* Index (key) compression type */
    i2			dui_dimension;      /* RTREE dimension */
    i4			dui_hilbertsize;    /* RTREE hilbertsize */
    DM_ATT		dui_key[DB_MAXIXATTS]; /* Attribute position .vs.
					    ** key position map. */
    f8			dui_range[DB_MAXRTREE_DIM*2]; /* ll-ur of values */
    DB_LOC_NAME		dui_location[DM_LOC_MAX];
                                	    /* Location array. */
}   DM0L_INDEX;

/*}
** Name: DM0L_BPEXTEND - Btree page extend log record.
**
** Description:
**      This structure describes the btree tree page extend log record.
**
** History:
**     30-Jul-1986 (ac)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		bp_header;      /* Standard log record header. */
    DB_TAB_ID		bp_tbl_id;	/* The table being allocated. */
}   DM0L_BPEXTEND;

/*}
** Name: DM0L_BCP - Begin Consistency point log record.
**
** Description:
**      This log record is written periodically by the logging system
**	to record the databases open and transaction in progress.
**	This record is not considered valid unless there is a corresponding
**	End Consistency Point record.
**
** History:
**      05-aug-1986 (Derek)
**          Created for Jupiter.
**	17-May-1988 (EdHsu)
**	    Added Comment for fast commit.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	18-oct-1993 (rogerk)
**	    Added transaction owner to tx information in bcp log record.
**	    Removed unused bcp_bof field from bcp log record.
**	20-Apr-2004 (jenjo02)
**	    Deleted TX_JOURNAL; TR_JOURNAL works just as well.
**       1-May-2006 (hanal04) Bug 116059
**          Added CP_FIRST flag so that dmr_get_cp() can look for the first
**          BCP log record now that BCP log records are not guaranteed to
**          be contiguous.
**	11-Sep-2006 (jonj)
**	    Added tx_lkid. tx_tran may change over time with LOGFULL_COMMIT
**	    protocols, but the lkid never does.
**      02-may-2008 (horda03) Bug 120349
**          Add DM0L_BCP_TYPE define.
[@history_line@]...
*/
typedef struct
{
    DM0L_HEADER		bcp_header;     /* Standard log record header. */
    i4		bcp_type;	/* Type of BCP record. */
#define			    CP_DB	1L
#define			    CP_TX	2L

#define DM0L_BCP_TYPE ",DB,TX"

    i4		bcp_flag;	/* Flags indicators. */
#define			    CP_LAST	0x01L
#define                     CP_FIRST    0x02L
    i4		bcp_count;	/* Count of objects. */
    union
    {
	struct	db
	{
	    i4		db_id;		/* Identifier for database. */
	    i4		db_status;	/* status of the database. */
#define			    DB_JOURNAL	0x02
	    DB_DB_NAME	db_name;	/* Database name. */
	    DB_DB_OWNER db_owner;	/* Database owner. */
	    i4     	db_ext_dbid;    /* External Database Id. */
	    i4		db_l_root;	/* Length of root location. */
	    DM_FILENAME	db_root;	/* Root location of database. */
	}		type_db[1];
	struct	tx
	{
	    i4		tx_status;	/* Transaction status indicators. */
	    DB_TRAN_ID	tx_tran;	/* Transaction Id. */
	    i4		tx_lkid;	/* Transaction's Lock list Id. */
	    i4		tx_dbid;	/* Database Id. */
	    LG_LA	tx_first;	/* First log record. */
	    LG_LA	tx_last;	/* Last log record. */
	    DB_OWN_NAME	tx_user_name;   /* Name of the user. */
	}		type_tx[1];
    }			bcp_subtype;
}   DM0L_BCP;

/*
** Name: DM0L_ECP - End Consistency point log record.
**
** Description:
**    This log record is written to verify that the system was stable at
**    the time that the Begin Consistencyk Point record was written.
** History:
**      17-May-1988 (EdHsu)
**          Created for Fast Commit.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed ECP to point back to the BCP via an LSN instead of a LA.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Added back ecp_begin_la which holds the Log Address of the Begin
**	    Consistency Point record. Now the ECP has the LA and LSN of the BCP.
[@history_line@]...
*/
typedef struct
{
    DM0L_HEADER		ecp_header;     /* Standard log record header. */
    LG_LSN		ecp_begin_lsn;	/* LSN of BCP record. */
    LG_LA		ecp_begin_la;	/* Log Address of BCP record. */
    LG_LA		ecp_bof;	/* The current archiver point */
} DM0L_ECP;

/*}
** Name: DM0L_SM1_RENAME - Rename sysmod file log record.
**
** Description:
**      This structure describes the format of a rename sysmod
**      file log record.
**
** History:
**     10-nov-1986 (Jennifer)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		sm1_header;      /* Standard log record header. */
    DB_TAB_ID           sm1_tbl_id;      /* Table you are sysmoding. */
    DB_TAB_ID           sm1_tmp_id;      /* iirtemp */
    i4		sm1_l_path;	 /* Length of path. */
    DM_PATH		sm1_path;	 /* Path to files. */
    DM_FILE		sm1_oldname;	 /* Old file name of table sysmod. */
    DM_FILE		sm1_tempname;	 /* Intermediate file name. */
    DM_FILE		sm1_newname;	 /* Old file name of new table. */
    DM_FILE		sm1_rename;	 /* New file name(replaces old file). */
    i4			sm1_oldpgtype;
    i4			sm1_newpgtype;
    i4			sm1_oldpgsize;
    i4			sm1_newpgsize;
}   DM0L_SM1_RENAME;

/*}
** Name: DM0L_SM2_CONFIG - Update configuration file log record.
**
** Description:
**      This structure describes the format of a update configuration
**      file log record.
**
** History:
**     10-nov-1986 (jennifer)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		sm2_header;     /* Standard log record header. */
    DB_TAB_ID           sm2_tbl_id;      /* Table you are sysmoding. */
    DMP_RELATION        sm2_oldrel;     /* Old relation record. */
    DMP_RELATION        sm2_newrel;     /* New relation record. */
    DMP_RELATION        sm2_oldidx;     /* Old rel_idx record. */
    DMP_RELATION        sm2_newidx;     /* New rel_idx record. */
}   DM0L_SM2_CONFIG;

/*}
** Name: DM0L_SM0_CLOSEPURGE - Completion of core catalog load
**
** Description:
**	This structure describes the log record which is logged to indicate
**	that a core catalog load operation has been completed. Rollforward
**	uses this to know that it should close and purge the temporary table
**	used during the core catalog load operation since it is about to be
**	renamed.
**
** History:
**	22-jul-1991 (bryanp)
**          Created for fix to bug B38527.
*/
typedef struct
{
    DM0L_HEADER		sm0_header;     /* Standard log record header. */
    DB_TAB_ID           sm0_tbl_id;     /* Table ID for temporary table
					** being used in the load. This table
					** is called 'iirtemp'.
					*/
}   DM0L_SM0_CLOSEPURGE;

/*}
** Name: DM0L_LOCATION - Add location log record.
**
** Description:
**      This structure describes the format of an add location
**      log record.
**
** History:
**     26-ma4-1987 (Jennifer)
**          Created for Jupiter.
**     16-aug-1991 (jnash) added DM0L_1VERSION for 6.5 merge
**	21-mar-1999 (nanpr01)
**	    Support raw locations.
*/
typedef struct
{
    DM0L_HEADER		loc_header;     /* Standard log record header. */
    i4             	loc_type;       /* Type of location adding. */
    DB_LOC_NAME		loc_name;	/* Location logical name. */
#define                     DM0L_1VERSION   0x10000000
    i4			loc_l_extent;	/* Length of path. */
    DM_PATH		loc_extent;	/* Path to files. */
    i4			loc_raw_start;		/* Start of raw location */
    i4			loc_raw_blocks;		/* Size of raw location  */
    i4			loc_raw_total_blocks;	/* Total size of raw area */
}   DM0L_LOCATION;


/*}
** Name: DM0L_EXT_ALTER - Add "extent alteration" log record.
**
** Description:
**      This structure describes the format of an extent alteration
**      log record.
**
** History:
**	15-sep-93 (jrb)
**         Created for MLSort project.
*/
typedef struct
{
    DM0L_HEADER		ext_header;     /* Standard log record header. */
    i4             ext_otype;      /* Old type of extent */
    i4             ext_ntype;      /* New type of extent */
    DB_LOC_NAME		ext_lname;	/* Location logical name. */
}	DM0L_EXT_ALTER;

/*}
** Name: DM0L_SETBOF - Set log file beginning address.
**
** Description:
**      This log record is written by the archiver when it wants to change
**	the beginning of log file address after archiving a portion of the
**	log file.
**
** History:
**      19-may-1987 (Derek)
**          Created for Jupiter.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
[@history_template@]...
*/
typedef struct
{
    DM0L_HEADER		sb_header;	/* Standard log record header. */
    LG_LA		sb_oldbof;	/* Old log address value. */
    LG_LA		sb_newbof;	/* New log address value. */
}   DM0L_SETBOF;

/*}
** Name: DM0L_JNLEOF - Journal file EOF
**
** Description:
**      This log record describes the changes to a configuration file
**	for changing the journal EOF.
**
** History:
**      22-may-1987 (Derek)
**          Created for Jupiter.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
[@history_template@]...
*/
typedef struct
{
    DM0L_HEADER		je_header;	/* Standard log record header. */
    i4		je_dbid;	/* Logging system database id. */
    DB_DB_NAME		je_dbname;	/* Database name. */
    DB_OWN_NAME		je_dbowner;	/* Database owner. */
    i4		je_l_root;	/* Length of root location name. */
    DM_PATH		je_root;	/* Root location for database. */
    i4		je_oldseq;	/* Old sequence number. */
    i4		je_newseq;	/* New sequence number. */
    i4		je_node;	/* Node number in cluster. */
    LG_LA		je_old_f_cp;	/* Previous archived log point. */
    LG_LA		je_new_f_cp;	/* New archived log point. */
}   DM0L_JNLEOF;

/*}
** Name: DM0L_ALTER - Alter table log record.
**
** Description:
**      This structure describes the format of an alter table log record.
**
** History:
**     22-jun-1987 (rogerk)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		dua_header;         /* Standard log record header. */
    DB_TAB_ID		dua_tbl_id;	    /* Table Identifier. */
    DB_TAB_NAME		dua_name;	    /* Table name. */
    DB_OWN_NAME  	dua_owner;	    /* Table owner. */
    i4		dua_count;	    /* Number of actions in alter */
    i4		dua_action;	    /* First action of alter */
}   DM0L_ALTER;

/*}
** Name: DM0L_ARCHIVER - Archiver restart log record.
**
** Description:
**      This structure describes the format of an alter table log record.
**
** History:
**     22-jun-1987 (rogerk)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		da_header;          /* Standard log record header. */
}   DM0L_ARCHIVE;

/*}
** Name: DM0L_LOAD - Load Table log record.
**
** Description:
**      This structure defines the format of load table log record.
**
**	Load table is used to load empty user tables.  For recovery,
**	the table is restored to its empty state.
**
**	If the dul_recreate field is set, then a new file was created
**	and loaded, then renamed to the relation file.  For recovery,
**	we just move the old empty file back and delete the new one.
**
** History:
**	03-aug-1987 (rogerk)
**          Created for Jupiter.
**	16-aug-1991 (jnash) Merged Jennifer
**	    Added reltups and lastpage for 6.5 merge
**	28-oct-1992 (jnash)
**	    Reduced loggin project.  Elim dul_reltups.
*/
typedef struct
{
    DM0L_HEADER		dul_header;         /* Standard log record header. */
    DB_TAB_ID		dul_tbl_id;	    /* Table Identifier. */
    DB_TAB_NAME		dul_name;	    /* Table name. */
    DB_OWN_NAME   	dul_owner;	    /* Table owner. */
    DB_LOC_NAME		dul_location;	    /* Table location. */
    i4		dul_structure;	    /* Storage Structure. */
    i2			dul_recreate;	    /* Whether new file was created */
    i2			dul_padding;
    i4             dul_lastpage;       /* Last page with data. */
}   DM0L_LOAD;

/*}
** Name: DM0L_P1 - Prepare to Complete log record.
**
** Description:
**      This structure defines the format of a prepare to commit log
**	record.
**
** History:
**	20-may-1988 (ac)
**          created.
**	10-jun-1989 (greg)
**	    use LK_BLKB_INFO rather than inventing lk_info
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
*/
typedef struct
{
    DM0L_HEADER		p1_header;       /* Standard log record header. */
    i4		p1_flag;	 /* Flags indicators. */
#define			    P1_LAST	 0x01L
#define			    P1_RELATED	 0x02L
    i4		p1_count;	 /* Count of lock resources. */
    DB_TRAN_ID		p1_tran_id;	 /* The physical transaction id of the
					 ** transaction.
					 */
    DB_DIS_TRAN_ID	p1_dis_tran_id;	 /* The distributed transaction id of
					 ** the transaction.
					 */
    DB_OWN_NAME		p1_name;         /* Blank padded name of the user. */
    LG_LA		p1_first_lga;	 /* The address of the first log record
					 ** of the transaction.
					 */
    LG_LA		p1_cp_lga;	 /* The address of the first CP log record
					 ** of the transaction.
					 */
    LK_BLKB_INFO	p1_type_lk[1];
	/*
	**	i4	       lkb_key[7];	 Lock resource name.
	**	char	       lkb_grant_mode;	 Lock grant mode.
	**	char	       lkb_attribute;	 Lock attribute.
	*/
}   DM0L_P1;

/*}
** Name: DM0L_DMU - DMU operation log record.
**
** Description:
**      This structure defines the format of a DMU operation log record.
**
**	This record marks a spot in the log file after which recovery
**	is not idempotent.  If a transaction rollback occurs on this
**	transaction, the recovering process will write a CLR for the
**	DMU record which will be recorded as NONREDO.  This prevents
**	redo processing from trying to re-recover operations that occurred
**	on the table between the DMU operation and its rollback.
**
** History:
**	11-sep-1989 (rogerk)
**	    Created for Terminator.
**	18-sep-1993 (rogerk)
**	    Removed Abort Save log record from DMU.  The same functionality
**	    is now provided by CLR records.
*/
typedef struct
{
    DM0L_HEADER		dmu_header;	/* Standard log record header. */
    i4		dmu_flag;	/* Not currently used. */
    DB_TAB_ID		dmu_tabid;	/* Table id. */
    DB_TAB_NAME		dmu_tblname;	/* Table name. */
    DB_OWN_NAME		dmu_tblowner;	/* Table owner. */
    i4		dmu_operation;	/* Type of DMU operation. */
}   DM0L_DMU;

/*}
** Name: DM0L_ASSOC - Change association of leaf index page.
**
** Description:
**
**      This log record is used to describe a change in the associated
**      data page for a leaf index btree page.
**
** History:
**      16-aug-1991 (jnash) merged 1-apr-1990 (Derek)
**          Created.
**	22-oct-1992 (jnash)
**	    Modified for Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add ass_page_size to hold table's page size.
*/
typedef struct
{
    DM0L_HEADER         ass_header;     /* Standard log record header. */
    DM0L_CRHEADER	ass_crhdrleaf;	/* Header for consistent read */
    DM0L_CRHEADER	ass_crhdrdata;	/* Header for consistent read */
    DB_TAB_ID           ass_tbl_id;	/* Table id. */
    i4			ass_page_size;	/* page size of table. */
    i2			ass_pg_type;	/* page type */
    i2			ass_loc_cnt;	/* Location count */
    i2			ass_lcnf_loc_id; /* Leaf pg offset in config file */
    i2			ass_ocnf_loc_id; /* Old ass pg offset in config file */
    i2			ass_ncnf_loc_id; /* New ass pg offset in config file */
    i2			ass_padding;
    i4             ass_leaf_page; 	/* Leaf page number. */
    i4             ass_old_data;  	/* Old associated data page. */
    i4             ass_new_data;	/* New associated data page. */
    i2			ass_tab_size;	/* Size of table name field */
    i2			ass_own_size;	/* Size of owner name field */
    char		ass_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME)];
}   DM0L_ASSOC;

/*}
** Name: DM0L_ALLOC - Allocate page log record.
**
** Description:
**
**      This log record describes the allocation of a page from the
**      free space bitmap.
**
** History:
**      16-aug-1991 (jnash) merged 1-apr-1990 (Derek)
**          Created.
**	26-oct-1992 (rogerk)
**	    Modified for Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add all_page_size to hold table's page size.
*/
typedef struct
{
    DM0L_HEADER         all_header;	    /* Standard log record header. */
    DB_TAB_ID           all_tblid;	    /* Table id */
    i4			all_page_size;	    /* page size of table. */
    i2			all_pg_type;	    /* page type */
    i2			all_loc_cnt;	    /* Location count */
    DM_PAGENO		all_fhdr_pageno;    /* Fmap page number */
    DM_PAGENO		all_fmap_pageno;    /* Fhdr page number */
    DM_PAGENO		all_free_pageno;    /* Free page number */
    i2			all_map_index;      /* Fmap sequence number */
    i2			all_fhdr_cnf_loc_id; /* Loc offset in config file */
    i2			all_fmap_cnf_loc_id; /* Loc offset in config file */
    i2			all_free_cnf_loc_id; /* Loc offset in config file */
    BITFLD		all_fhdr_hint:1;    /* True if reset fhdr free hint. */
    BITFLD		all_fhdr_hwmap:1;   /* True if set hwmap in fhdr. */
    BITFLD		all_bits_free:30;
    i2			all_tab_size;	    /* Size of table name field */
    i2			all_own_size;	    /* Size of owner name field */
    char		all_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME)]; 
}   DM0L_ALLOC;

/*}
** Name: DM0L_DEALLOC - Deallocate page log record.
**
** Description:
**
**      This log record describes the deallocation of a page from the
**      free space bitmap.
**
** History:
**      16-aug-1991 (jnash) merged 1-apr-1990 (Derek)
**          Created.
**	26-oct-1992 (rogerk)
**	    Modified for Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add dall_page_size to hold table's page size.
*/
typedef struct
{
    DM0L_HEADER         dall_header;	    /* Standard log record header. */
    DB_TAB_ID           dall_tblid;	    /* Table id */
    i4			dall_page_size;	    /* page size of table. */
    i2			dall_pg_type;	    /* page type */
    i2			dall_loc_cnt;	    /* Location count */
    DM_PAGENO		dall_fhdr_pageno;   /* Fmap page number */
    DM_PAGENO		dall_fmap_pageno;   /* Fhdr page number */
    DM_PAGENO		dall_free_pageno;   /* Free page number */
    i2			dall_map_index;	    /* Fmap sequence number */
    i2			dall_fhdr_cnf_loc_id; /* Loc offset in config file */
    i2			dall_fmap_cnf_loc_id; /* Loc offset in config file */
    i2			dall_free_cnf_loc_id; /* Loc offset in config file */
    BITFLD		dall_fhdr_hint:1;   /* True if set free hint in fhdr. */
    BITFLD		dall_bits_free:31;
    i2			dall_tab_size;	    /* Size of table name field */
    i2			dall_own_size;	    /* Size of owner name field */
    char		dall_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME)]; 
}   DM0L_DEALLOC;

/*}
** Name: DM0L_EXTEND - Extend table space log record.
**
** Description:
**
**      This log record describes the extension of the operating system
**      file for a table and the addition of the allocated pages to the
**	table's free maps.
**
** History:
**      16-aug-1991 (jnash) merged 1-apr-1990 (Derek)
**          Created.
**	26-oct-1992 (rogerk)
**	    Modified for Reduced Logging Project.
**	08-dec-1992 (jnash)
**	    Add ext_cnf_loc_array for use by rollforward.
**	06-mar-1996 (stial01 for bryanp)
**	    Add ext_page_size to hold table's page size.
**	03-sep-1996 (pchang)
**	    Added ext_last_free as part of the fix for B77416.
*/
typedef struct
{
    DM0L_HEADER         ext_header;	    /* Standard log record header. */
    DB_TAB_ID           ext_tblid;	    /* Table id */
    DB_TAB_NAME		ext_tblname;	    /* Table name. */
    DB_OWN_NAME		ext_tblowner;	    /* Table owner. */
    i4			ext_page_size;	    /* page size of table. */
    i2			ext_pg_type;	    /* page type */
    i2			ext_loc_cnt;	    /* Location count */
    DM_PAGENO		ext_fhdr_pageno;    /* Fmap page number */
    DM_PAGENO		ext_fmap_pageno;    /* Fhdr page number */
    i4		ext_map_index;      /* Fmap sequence number */
    i2			ext_fhdr_cnf_loc_id;/* Loc offset in config file */
    i2			ext_fmap_cnf_loc_id;/* Loc offset in config file */
    DM_PAGENO		ext_first_used;	    /* Pages used for new FMAPs */
    DM_PAGENO		ext_last_used;  
    DM_PAGENO		ext_first_free;	    /* Newly allocated free pages */
    DM_PAGENO		ext_last_free;
    i4             ext_old_pages;	    /* Last page before allocation. */
    i4             ext_new_pages;	    /* Last page after allocation. */
    BITFLD		ext_fhdr_hint:1;    /* True if set free hint in fhdr. */
    BITFLD		ext_fhdr_hwmap:1;   /* True if set hwmap in fhdr. */
    BITFLD		ext_fmap_update:1;  /* True if fmap was updated. */
    BITFLD		ext_bits_free:29;
    i2			ext_cnf_loc_array[DM_LOC_MAX]; /* Location array. */
}   DM0L_EXTEND;

/*}
** Name: DM0L_OVFL - Overflow Link log record.
**
** Description:
**
**	This log record describes the linking of a new data page into an
**	overflow chain.
**
**	It is used during REDO processing to replay the addition of a
**	newly allocated page into an overflow chain.
**
**	UNDO and DUMP processing do not use this record, they both use
**	BI and ALLOC records to reset the root page's ovfl pointer and
**	to restore the newly allocated page.
**
** History:
**	 3-nov-1991 (rogerk)
**          Created to fix REDO problems with File Extend.
**	 19-oct-1992 (jnash)
**          New recovery format.
**	06-mar-1996 (stial01 for bryanp)
**	    Add ovf_page_size to hold table's page size.
*/
typedef struct
{
    DM0L_HEADER         ovf_header;	/* Standard log record header. */
    DM0L_CRHEADER	ovf_crhdr;	/* Header for consistent read */
    DB_TAB_ID           ovf_tbl_id;	/* Table id. */
    i4			ovf_page_size;	/* page size of table. */
    i2			ovf_pg_type;	/* page type */
    i2			ovf_loc_cnt;	/* Location count */
    i2			ovf_cnf_loc_id; /* page offset in config file */
    i2			ovf_ovfl_cnf_loc_id;/* oflo offset in config file */
    i2			ovf_fullc;	/* full chain on/off */
    i2			ovf_padding;
    DM_PAGENO		ovf_page;	/* Where to link the new page. */
    DM_PAGENO		ovf_newpage;	/* New overflow page. */
    DM_PAGENO		ovf_ovfl_ptr;	/* Root's old ovfl pointer. */
    DM_PAGENO		ovf_main_ptr;	/* Root's old main pointer. */
    i2			ovf_tab_size;	/* Size of table name field */
    i2			ovf_own_size;	/* Size of owner name field */
    char		ovf_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME)]; 
}   DM0L_OVFL;

/*}
** Name: DM0L_NOFULL - Fullchain status log record
**
** Description:
**
**	This log record describes reseting the nofullhain bit in a
**	hash table bucket.
**
**	It is used during REDO processing to replay resetting
**	the bit, and during normal UNDO to set the bit.
**
** History:
**	 19-oct-1992 (jnash)
**          Created for 6.5 recovery.
**	06-mar-1996 (stial01 for bryanp)
**	    Add nofull_page_size to hold table's page size.
*/
typedef struct
{
    DM0L_HEADER         nofull_header;	/* Standard log record header. */
    DM0L_CRHEADER	nofull_crhdr;	/* Header for consistent read */
    DB_TAB_ID           nofull_tbl_id;	/* Table id. */
    i4			nofull_page_size;	/* page size of table. */
    i2			nofull_pg_type;	/* page type */
    i2			nofull_loc_cnt;	/* Location count */
    i2			nofull_cnf_loc_id; /* Loc offset in config file */
    i2			nofull_padding;
    DM_PAGENO		nofull_pageno;	/* page number */
    i2			nofull_tab_size;	/* Size of table name field */
    i2			nofull_own_size;	/* Size of owner name field */
    char		nofull_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME)]; 
} DM0L_NOFULL;

/*}
** Name: DM0L_FMAP - Fmap allocation log record.
**
** Description:
**
**	This log record describes the addition of a new fmap page to
**	a table during a file extend operation.
**
**	It is used during REDO processing to reallocate the fmap page.
**	It is undone only if the allocation itself fails as extend operations
**	are always performed within a mini transaction.
**
** History:
**	 26-oct-1992 (rogerk)
**          Created for 6.5 recovery.
**	06-mar-1996 (stial01 for bryanp)
**	    Add fmap_page_size to hold table's page size.
**	 03-sep-1996 (pchang)
**	    Added fmap_last_used as part of the fix for B77416.
**      30-Oct-2000 (hanal04) Bug 103037 INGSRV 1291
**          Added fmap_hw_mark to ensure the FMAP's highwater mark
**          is correctly restored during rollforwarddb.
*/
typedef struct
{
    DM0L_HEADER         fmap_header;	    /* Standard log record header. */
    DB_TAB_ID           fmap_tblid;	    /* Table id */
    DB_TAB_NAME		fmap_tblname;	    /* Table name. */
    DB_OWN_NAME		fmap_tblowner;	    /* Table owner. */
    i4			fmap_page_size;	/* page size of table. */
    i2			fmap_pg_type;	    /* page type */
    i2			fmap_loc_cnt;	    /* Location count */
    DM_PAGENO		fmap_fhdr_pageno;   /* Fmap page number */
    DM_PAGENO		fmap_fmap_pageno;   /* Fhdr page number */
    DM_PAGENO		fmap_map_index;	    /* Fmap sequence number */
    i4			fmap_hw_mark;       /* Fmap highwater mark */
    i2			fmap_fhdr_cnf_loc_id; /* Loc offset in config file */
    i2			fmap_fmap_cnf_loc_id; /* Loc offset in config file */
    DM_PAGENO		fmap_first_used;    /* Pages used for new FMAPs */
    DM_PAGENO		fmap_last_used; 
    DM_PAGENO		fmap_first_free;    /* Newly allocated free pages */
    DM_PAGENO		fmap_last_free;
} DM0L_FMAP;

/*}
** Name: DM0L_UFMAP - Update Fmap allocation log record.
**
** Description:
**
**      This log record describes an update to an FMAP page during a file
**      extend operation. It is used when an FMAP is assigned a page
**      number not covered by the current highest FMAP or the new FMAP itself.
**
**      It is used during REDO processing keep the updated FMAP in symc with
**      the corresponding DM0L_FMAP processing.
**      It is undone only if the allocation itself fails as extend operations
**      are always performed within a mini transaction.
**
** History:
**      23-Feb-2009 (hanal04) Bug 121652
**          Created.
*/
typedef struct
{
    DM0L_HEADER         fmap_header;	    /* Standard log record header. */
    DB_TAB_ID           fmap_tblid;         /* Table id */
    DB_TAB_NAME         fmap_tblname;       /* Table name. */
    DB_OWN_NAME         fmap_tblowner;      /* Table owner. */
    i4                  fmap_page_size; /* page size of table. */
    i2                  fmap_pg_type;       /* page type */
    i2                  fmap_loc_cnt;       /* Location count */
    DM_PAGENO           fmap_fhdr_pageno;   /* Fmap page number */
    DM_PAGENO           fmap_fmap_pageno;   /* Fhdr page number */
    DM_PAGENO           fmap_map_index;     /* Fmap sequence number */
    i4                  fmap_hw_mark;       /* Fmap highwater mark */
    i2                  fmap_fhdr_cnf_loc_id; /* Loc offset in config file */
    i2                  fmap_fmap_cnf_loc_id; /* Loc offset in config file */
    DM_PAGENO           fmap_first_used;    /* Pages used for new FMAPs */
    DM_PAGENO           fmap_last_used;
} DM0L_UFMAP;


/*}
** Name: DM0L_BTPUT - Btree Index Put log record.
**
** Description:
**      This structure describes an insert to btree index log record.
**
** History:
**	14-dec-1992 (rogerk)
**          Created for Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add btp_page_size to hold table's page size.
**	19-Jan-2004 (jenjo02)
**	    Added btp_partno for global indexes.
**	03-May-2006 (jenjo02)
**	    Stole btp_unused for btp_ixklen.
**	13-Jun-2006 (jenjo02)
**	    Puts of leaf records may be up to
**	    DM1B_MAXLEAFLEN, not DM1B_KEYLENGTH, bytes.
**	29-Oct-2009 (kschendel) SIR 122739
**	    Kill ixklen, add btflags.
*/
typedef struct
{
    DM0L_HEADER		btp_header;	/* Standard log record header. */
    DM0L_CRHEADER	btp_crhdr;	/* Header for consistent read */
    DB_TAB_ID		btp_tbl_id;	/* Table Identifier. */
    DM_TID		btp_bid;	/* Location of new key. */
    DM_TID		btp_tid;	/* Tid portion of new entry. */
    i4			btp_page_size;	/* page size of table. */
    i2			btp_pg_type;	/* page type */
    i2			btp_cmp_type;	/* key compression type */
    i2			btp_loc_cnt;	/* Location count */
    i2			btp_cnf_loc_id; /* Loc offset in config file */
    i2			btp_tab_size;	/* Size of table name field */
    i2			btp_own_size;	/* Size of owner name field */
    i2			btp_key_size;	/* Size of key entry. */
    i2			btp_bid_child;  /* ins pos may be > 512 on INDEX page */
    i2			btp_partno;	/* Partition number */
    u_i2		btp_btflags;	/* BTREE flags */

#define DM0L_BT_UNIQUE		0x0001		/* Btree is unique */
#define DM0L_BT_RNG_HAS_NONKEY	0x0002		/* Leaf range keys include
						** non-key columns */
#define DM0L_BT_DUPS_ON_OVFL	0x0004		/* Btree uses leaf level
						** overflow for duplicates */

    char		btp_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME) + DM1B_MAXLEAFLEN];
}   DM0L_BTPUT;

/*}
** Name: DM0L_BTDEL - Btree Index Delete log record.
**
** Description:
**      This structure describes a delete to btree index log record.
**
** History:
**	14-dec-1992 (rogerk)
**          Created for Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add btd_page_size to hold table's page size.
**	19-Jan-2004 (jenjo02)
**	    Added btd_partno for global indexes.
**	03-May-2006 (jenjo02)
**	    Stole btd_unused for btd_ixklen.
**	13-Jun-2006 (jenjo02)
**	    Deletes of leaf entries may be up to
**	    DM1B_MAXLEAFLEN, not DM1B_KEYLENGTH, bytes.
**	29-Oct-2009 (kschendel) SIR 122739
**	    Kill ixklen, add btflags.
*/
typedef struct
{
    DM0L_HEADER		btd_header;	/* Standard log record header. */
    DM0L_CRHEADER	btd_crhdr;	/* Header for consistent read */
    DB_TAB_ID		btd_tbl_id;	/* Table Identifier. */
    DM_TID		btd_bid;	/* Location of old key. */
    DM_TID		btd_tid;	/* Tid portion of old entry. */
    i4			btd_page_size;	/* page size of table. */
    i2			btd_pg_type;	/* page type */
    i2			btd_cmp_type;	/* key compression type */
    i2			btd_loc_cnt;	/* Location count */
    i2			btd_cnf_loc_id; /* Loc offset in config file */
    i2			btd_tab_size;	/* Size of table name field */
    i2			btd_own_size;	/* Size of owner name field */
    i2			btd_key_size;	/* Size of key entry. */
    i2			btd_bid_child;  /* del pos may be > 512 on INDEX page */
    i2			btd_partno;	/* Partition number */
    u_i2		btd_btflags;	/* BTREE flags, see btp_btflags */
    char		btd_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME) + DM1B_MAXLEAFLEN];
}   DM0L_BTDEL;

/*}
** Name: DM0L_BTSPLIT - Btree Index Split log record.
**
** Description:
**      This structure describes a Btree SPLIT operation.
**
** History:
**	14-dec-1992 (rogerk)
**          Created for Reduced Logging Project.
**	26-jul-1993 (rogerk)
**	  - Added split direction field to the split log record so we can
**	    tell the difference between a split where the splitpos was zero
**	    because duplicates had to be moved to the new sibling and a split
**	    where the splitpos was zero because the leaf was empty (but had
**	    a non-empty overflow chain).
**	  - Removed unused spl_desc_pos.
**	06-mar-1996 (stial01 for bryanp)
**	    Add spl_page_size to hold table's page size.
**	12-dec-1996 (shero03)
**	    Changed to support variable page size
**	09-jan-1997 (nanpr01)
**	    spl_vbuf is variable length now depending on page_size and is.
**	    really a space holder. Its size of DM1B_KEYLENGTH is a 
**	    little misleading. It contains first the page and then the key.
**	10-jan-1997 (nanpr01)
**	    A previous change has taken out the spl_page and put it in the
**	    spl_vbuf for VPS project. However since the spl_vbuf is a char
**	    field and causes vulnerability to alignment issues for further
**	    development. So the space holder spl_vbuf type was changed to
**	    DMPP_PAGE and it now contains the page and the key.
**	01-May-2006 (jenjo02)
**	    Snatched spl_padding for spl_range_klen.
*/
typedef struct
{
    DM0L_HEADER		spl_header;	/* Standard log record header. */
    DM0L_CRHEADER	spl_crhdr;	/* Header for consistent read */
    DB_TAB_ID		spl_tbl_id;	/* Table Identifier. */
    DB_TAB_NAME		spl_tblname;	/* Table name. */
    DB_OWN_NAME		spl_tblowner;	/* Table owner. */
    i4			spl_page_size;	/* page size of table. */
    i2			spl_pg_type;	/* page type */
    i2			spl_cmp_type;	/* key compression type */
    i2			spl_loc_cnt;	/* Location count */
    i2			spl_cur_loc_id; /* config file locid of split page */
    i2			spl_sib_loc_id; /* config file locid of sibling page */
    i2			spl_dat_loc_id; /* config file locid of new data page */
    DM_PAGENO		spl_cur_pageno;	/* Page # of split page */
    DM_PAGENO		spl_sib_pageno;	/* Page # of sibling page */
    DM_PAGENO		spl_dat_pageno;	/* Page # of new data page */
    i2			spl_split_pos;	/* Line # of split position */
    i2			spl_split_dir;	/* Direction of split: right or left */
    i2			spl_desc_klen;	/* Size of descriptor key */
    i2			spl_klen;	/* Maximum key length for table */
    i2                  spl_kperpage;   /* keys per page */
    i2                  spl_range_klen; /* Length of range key entry */
    DMPP_PAGE  		spl_vbuf; 	/* Variable length buffer -
						  ** holds the split page 
						  ** and the descriptor key */
}   DM0L_BTSPLIT;

/*}
** Name: DM0L_BTOVFL - Btree Leaf Overflow log record.
**
** Description:
**      This structure describes the creation of a Btree Overflow page.
**
** History:
**	14-dec-1992 (rogerk)
**          Created for Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add bto_page_size to hold table's page size.
**	19-Jan-2004 (jenjo02)
**	    Added bto_tidsize for global indexes.
*/
typedef struct
{
    DM0L_HEADER		bto_header;	/* Standard log record header. */
    DM0L_CRHEADER	bto_crhdr;	/* Header for consistent read */
    DB_TAB_ID		bto_tbl_id;	/* Table Identifier. */
    DB_TAB_NAME		bto_tblname;	/* Table name. */
    DB_OWN_NAME		bto_tblowner;	/* Table owner. */
    i4			bto_page_size;	/* page size of table. */
    i2			bto_pg_type;	/* page type */
    i2			bto_cmp_type;	/* key compression type */
    i2			bto_loc_cnt;	/* Location count */
    i2			bto_leaf_loc_id; /* Loc # in config file of leaf */
    i2			bto_ovfl_loc_id; /* Loc # in config file of overflow */
    i2			bto_klen;	/* Maximum key length for table */
    DM_PAGENO		bto_leaf_pageno;
    DM_PAGENO		bto_ovfl_pageno;
    DM_PAGENO		bto_mainpage;
    DM_PAGENO		bto_nextpage;
    DM_PAGENO		bto_nextovfl;
    i2			bto_lrange_len;	/* Size of Low range key */
    i2			bto_rrange_len;	/* Size of High range key */
    DM_TID		bto_lrtid;	/* Lrange TID value */
    DM_TID		bto_rrtid;	/* Rrange TID value */
    i2			bto_tidsize;	/* Size of TID on page */
    i2			bto_reserved;
    char		bto_vbuf[(2 * DM1B_KEYLENGTH)];
}   DM0L_BTOVFL;

/*}
** Name: DM0L_BTFREE - Btree Overflow Deallocate log record.
**
** Description:
**      This structure describes the freeing of a Btree Overflow page.
**
** History:
**	14-dec-1992 (rogerk)
**          Created for Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add btf_page_size to hold table's page size.
**	19-Jan-2004 (jenjo02)
**	    Add btf_tidsize for global indexes.
**	03-May-2006 (jenjo02)
**	    Add btf_ixklen, btf_spare.
**	29-Oct-2009 (kschendel) SIR 122739
**	    Kill ixklen, add btflags.
*/
typedef struct
{
    DM0L_HEADER		btf_header;	/* Standard log record header. */
    DB_TAB_ID		btf_tbl_id;	/* Table Identifier. */
    DB_TAB_NAME		btf_tblname;	/* Table name. */
    DB_OWN_NAME		btf_tblowner;	/* Table owner. */
    i4			btf_page_size;	/* page size of table. */
    i2			btf_pg_type;	/* page type */
    i2			btf_cmp_type;	/* key compression type */
    i2			btf_loc_cnt;	/* Location count */
    i2			btf_ovfl_loc_id; /* Loc # in config file of overflow */
    i2			btf_prev_loc_id; /* Loc # in config file of prev page */
    i2			btf_klen;	/* Maximum key length for table */
    DM_PAGENO		btf_prev_pageno;
    DM_PAGENO		btf_ovfl_pageno;
    DM_PAGENO		btf_mainpage;
    DM_PAGENO		btf_nextpage;
    DM_PAGENO		btf_nextovfl;
    i2			btf_lrange_len;	/* Size of Low range key */
    i2			btf_rrange_len;	/* Size of High range key */
    i2			btf_dupkey_len;	/* Size of chain's duplicate key */
    i2			btf_tidsize;	/* TidSize on page */
    u_i2		btf_btflags;	/* BTREE flags, see btp_btflags */
    i2			btf_spare;	/* ...for alignment */
    DM_TID		btf_lrtid;	/* Lrange TID value */
    DM_TID		btf_rrtid;	/* Rrange TID value */
    char		btf_vbuf[(3 * DM1B_KEYLENGTH)];
}   DM0L_BTFREE;

/*}
** Name: DM0L_BTUPDTOVFL - Btree Overflow Page Update log record.
**
** Description:
**      This structure describes the updates to a Btree Overflow page made
**	during a split operation.
**
** History:
**	14-dec-1992 (rogerk)
**          Created for Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add btu_page_size to hold table's page size.
*/
typedef struct
{
    DM0L_HEADER		btu_header;	/* Standard log record header. */
    DB_TAB_ID		btu_tbl_id;	/* Table Identifier. */
    DB_TAB_NAME		btu_tblname;	/* Table name. */
    DB_OWN_NAME		btu_tblowner;	/* Table owner. */
    i4			btu_page_size;	/* page size of table. */
    i2			btu_pg_type;	/* page type */
    i2			btu_cmp_type;	/* key compression type */
    i2			btu_loc_cnt;	/* Location count */
    i2			btu_cnf_loc_id; /* Loc offset in config file */
    DM_PAGENO		btu_pageno;	/* Overflow page number */
    DM_PAGENO		btu_mainpage;	/* page_main value */
    DM_PAGENO		btu_omainpage;	/* old page_main value */
    DM_PAGENO		btu_nextpage;	/* bt_nextpage value */
    DM_PAGENO		btu_onextpage;	/* old bt_nextpage value */
    i2			btu_lrange_len;	/* Size of Low range key */
    i2			btu_rrange_len;	/* Size of High range key */
    i2			btu_olrange_len;/* Size of old Low range key */
    i2			btu_orrange_len;/* Size of old High range key */
    DM_TID		btu_lrtid;	/* Lrange TID value */
    DM_TID		btu_rrtid;	/* Rrange TID value */
    DM_TID		btu_olrtid;	/* Old Lrange TID value */
    DM_TID		btu_orrtid;	/* Old Rrange TID value */
    char		btu_vbuf[(4 * DM1B_KEYLENGTH)];
}   DM0L_BTUPDOVFL;

/*}
** Name: DM0L_RTDEL - Rtree Index Delete log record.
**
** Description:
**      This structure describes a delete from an Rtree index log record.
**
** History:
** 	24-sep-1996 (wonst02)
**	    Added for Rtree.
*/
typedef struct
{
    DM0L_HEADER		rtd_header;	/* Standard log record header. */
    DB_TAB_ID		rtd_tbl_id;	/* Table Identifier. */
    DM_TID		rtd_bid;	/* Location of old key. */
    DM_TID		rtd_tid;	/* Tid portion of old entry. */
    i4			rtd_page_size;	/* page size of table. */
    i2			rtd_pg_type;	/* page type */
    i2			rtd_cmp_type;	/* key compression type */
    i2			rtd_loc_cnt;	/* Location count */
    i2			rtd_cnf_loc_id; /* Loc offset in config file */
    u_i2		rtd_hilbertsize;/* Size of Hilbert values */
    DB_DT_ID		rtd_obj_dt_id;	/* Data type of base object */
    i2			rtd_tab_size;	/* Size of table name field */
    i2			rtd_own_size;	/* Size of owner name field */
    i2			rtd_stack_size;	/* Size of ancestor stack */
    i2			rtd_key_size;	/* Size of key entry */
    char		rtd_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME) + DB_MAXRTREE_KEY +
				 (RCB_MAX_RTREE_LEVEL * sizeof(DM_TID))];
}   DM0L_RTDEL;

/*}
** Name: DM0L_RTPUT - Rtree Index Put log record.
**
** Description:
**      This structure describes a Put to Rtree index log record.
**
** History:
** 	24-sep-1996 (wonst02)
**	    Added for Rtree.
*/
typedef struct
{
    DM0L_HEADER		rtp_header;	/* Standard log record header. */
    DB_TAB_ID		rtp_tbl_id;	/* Table Identifier. */
    DM_TID		rtp_bid;	/* Location of new key. */
    DM_TID		rtp_tid;	/* Tid portion of new entry. */
    i4			rtp_page_size;	/* page size of table. */
    i2			rtp_pg_type;	/* page type */
    i2			rtp_cmp_type;	/* key compression type */
    i2			rtp_loc_cnt;	/* Location count */
    i2			rtp_cnf_loc_id; /* Loc offset in config file */
    u_i2		rtp_hilbertsize;/* Size of Hilbert values */
    DB_DT_ID		rtp_obj_dt_id;	/* Data type of base object */
    i2			rtp_tab_size;	/* Size of table name field */
    i2			rtp_own_size;	/* Size of owner name field */
    i2			rtp_stack_size;	/* Size of ancestor stack */
    i2			rtp_key_size;	/* Size of key entry */
    char		rtp_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME) + DB_MAXRTREE_KEY +
    				 (RCB_MAX_RTREE_LEVEL * sizeof(DM_TID))];
}   DM0L_RTPUT;

/*}
** Name: DM0L_RTREP - Rtree Index Replace log record.
**
** Description:
**      This structure describes a replace of Rtree index log record.
**
** History:
**	15-jul-1996 (wonst02)
**	    Added for Rtree.
** 	24-sep-1996 (wonst02)
** 	    Modify DM0L_RTADJ and rename to _RTREP.
*/
typedef struct
{
    DM0L_HEADER		rtr_header;	/* Standard log record header. */
    DB_TAB_ID		rtr_tbl_id;	/* Table Identifier. */
    DM_TID		rtr_bid;	/* Location of new key. */
    DM_TID		rtr_tid;	/* Tid portion of new entry. */
    i4			rtr_page_size;	/* page size of table. */
    i2			rtr_pg_type;	/* page type */
    i2			rtr_cmp_type;	/* key compression type */
    i2			rtr_loc_cnt;	/* Location count */
    i2			rtr_cnf_loc_id; /* Loc offset in config file */
    u_i2		rtr_hilbertsize;/* Size of Hilbert values */
    DB_DT_ID		rtr_obj_dt_id;	/* Data type of base object */
    i2			rtr_tab_size;	/* Size of table name field */
    i2			rtr_own_size;	/* Size of owner name field */
    i2			rtr_stack_size;	/* Size of ancestor stack */
    i2			rtr_okey_size;	/* Size of old key entry */
    i2			rtr_nkey_size;	/* Size of new key entry */
    char		rtr_vbuf[(DB_TAB_MAXNAME + DB_OWN_MAXNAME) + (DB_MAXRTREE_KEY * 2) +
    				 (RCB_MAX_RTREE_LEVEL * sizeof(DM_TID))];
}   DM0L_RTREP;

/*}
** Name: DM0L_DISASSOC - Disassociate Data page from Leaf
**
** Description:
**
**      This log record is used to describe the disassociation of a data
**      page during a btree join operation.
**
** History:
**	14-dec-1992 (rogerk)
**	    Written for Reduced Logging Project.
**	06-mar-1996 (stial01 for bryanp)
**	    Add dis_page_size to hold table's page size.
*/
typedef struct
{
    DM0L_HEADER         dis_header;     /* Standard log record header. */
    DB_TAB_ID           dis_tbl_id;	/* Table id. */
    DB_TAB_NAME		dis_tblname;	/* Table name. */
    DB_OWN_NAME		dis_tblowner;	/* Table owner. */
    i4			dis_page_size;	/* page size of table. */
    i2			dis_pg_type;	/* page type */
    i2			dis_loc_cnt;	/* Location count */
    i2			dis_cnf_loc_id; /* Data loc offset in config file */
    i2			dis_padding;
    DM_PAGENO		dis_pageno; 	/* Data page number. */
}   DM0L_DISASSOC;


/*}
** Name: DM0L_RAWDATA - Multi-part raw data log record
**
** Description:
**	This structure defines the "raw data" log record.
**	"Raw data" is a (potentally) multi-part log record used to
**	log more or less anything that doesn't fit into the usual
**	scheme of things.  Initially, rawdata was invented so that
**	repartitioning modify could be properly logged and replayed.
**	The necessary partition definition can be arbitrarily long,
**	encompassing potentially thousands of partitions and break
**	table values.
**
**	The rawdata log record contains a total length, offset and size
**	of this logged piece, and a type code indicating what sort of
**	raw data this is.  There's also an instance-number in case
**	some usage requires multiple occurrences of the same type of
**	rawdata item;  at present (Apr '04), the instance-number is
**	not used by anyone.
**
**	Typically, the rawdata log records would precede the operation
**	log record (e.g. partition def rawdata would precede the MODIFY
**	record).  Upon journal replay, the rawdata would be gathered up
**	and stored somewhere pending processing of the operation record.
**	(This means that rawdata is more useful for (RE)DO than UNDO,
**	although with sufficient cleverness rawdata could be used for
**	some sort of undo thingie as well.)
**
** History:
**	1-Apr-2004 (schka24)
**	    Invent for logging repartitioning modifies.
**	    Not an April Fool despite the date.
*/

typedef struct
{
    DM0L_HEADER	rawd_header;		/* Standard log record header */
    i2		rawd_type;		/* Type of raw data item (below) */
    i2		rawd_instance;		/* Occurrence #, not presently used */
    i4		rawd_total_size;	/* Total size of object */
    i4		rawd_offset;		/* Start offset of this piece */
    i4		rawd_size;		/* Size of this piece */
} DM0L_RAWDATA;

/* The type codes are presently defined to be generic across DMF, so
** they're in dm.h as DM_RAWD_xxx instead of being defined here.
** (Besides, other include files need the definition, and dm0l.h is
** too late.)
*/


/*}
** Name: DM0L_BSF - Begin Sidefile log record.
**
** Description:
**      This structure defines the format of begin sidefile log record.
**
** History:
**     19-nov-2003 (stial01)
**          Created.
*/
typedef struct
{
    DM0L_HEADER		bsf_header;         /* Standard log record header. */
    DB_TAB_ID		bsf_tbl_id;	    /* Table Identifier. */
    DB_TAB_NAME		bsf_name;	    /* Table name. */
    DB_OWN_NAME   	bsf_owner;	    /* Table owner. */
}   DM0L_BSF;


/*}
** Name: DM0L_ESF - End Sidefile log record.
**
** Description:
**      This structure defines the format of end sidefile log record.
**
** History:
**     19-nov-2003 (stial01)
**          Created.
*/
typedef struct
{
    DM0L_HEADER		esf_header;         /* Standard log record header. */
    DB_TAB_ID		esf_tbl_id;	    /* Table Identifier. */
    DB_TAB_NAME		esf_name;	    /* Table name. */
    DB_OWN_NAME   	esf_owner;	    /* Table owner. */
}   DM0L_ESF;

/*}
** Name: DM0L_RNL_LSN - Online modify rnl page lsns log record.
**
** Description:
**      This structure defines the format of online modify's rnl pg lsns log record.
**
** History:
**     06-apr-2004 (thaju02)
**          Created.
*/
typedef struct
{
    DM0L_HEADER         rl_header;         /* Standard log record header. */
    DB_TAB_ID           rl_tbl_id;         /* Table Identifier. */
    DB_TAB_NAME         rl_name;           /* Table name. */
    DB_OWN_NAME         rl_owner;          /* Table owner. */
    LG_LSN		rl_bsf_lsn;
    i4			rl_lastpage;
    i4			rl_lsn_totcnt;
    i4			rl_lsn_cnt;
#define		DM0L_MAX_LSNS_PER_REC	500
}   DM0L_RNL_LSN;

/*}
** Name: DM0L_TEST - Test log record used for logging and journaling tests
**
** Description:
**	This structure describes the DM0LTEST log record.
**	This record is used to implement logging and journaling tests.
**
**	It is not written by any "normal" ingres operation, and in
**	most processing will just be ignored by the ingres system.
**
**	Certain actions/processes of the system will take special
**	test actions - including crashing - upon encountering these
**	log records.
**
**	The test record's tst_number field allows many different tests
**	to be driven by this log record.
**
** History:
**	25-feb-1991 (rogerk)
**          Created as part of Archiver Stability Project.
*/
typedef struct
{
    DM0L_HEADER		tst_header;	/* Standard log record header. */
    i4		tst_number;	/* Test number identifier */
#define			    TST101_ACP_TEST_ENABLE	101
#define			    TST102_ACP_DISKFULL		102
#define			    TST103_ACP_BAD_JNL_REC	103
#define			    TST104_ACP_ACCESS_VIOLATE	104
#define			    TST105_ACP_COMPLETE_WORK	105
#define			    TST106_ACP_BACKUP_ERROR	106
#define			    TST107_ACP_MEMORY_FAILURE	107
#define			    TST109_ACP_TEST_DISABLE	109

    i4		tst_value;	/* Optional values specific to
					** each test type. */
    i4		tst_stuff1;
    i4		tst_stuff2;
    i4		tst_stuff3;
    i4		tst_stuff4;
    char		tst_stuff5[DB_MAXNAME];
    char		tst_stuff6[DB_MAXTUP];
}   DM0L_TEST;

/*}
** Name: DM0L_DEL_LOCATION - Delete location log record.
**
** Description:
**      This structure describes the format of a delete location
**      log record.
**
** History:
**     29-apr-2004 (gorvi01)
**          Created for Jupiter.
*/
typedef struct
{
    DM0L_HEADER		loc_header;     /* Standard log record header. */
    i4             	loc_type;       /* Type of location adding. */
    DB_LOC_NAME		loc_name;	/* Location logical name. */
#define                     DM0L_1VERSION   0x10000000
    i4			loc_l_extent;	/* Length of path. */
    DM_PATH		loc_extent;	/* Path to files. */
}   DM0L_DEL_LOCATION;


/*}
** Name: DM0L_JNL_SWITCH - Journal switch.
**
** Description:
**      This structure describes the format of a journal switch record
**      written to the end of the current journal before switching to the 
**      next journal. The only purpose of this log record is so that
**      incremental rollforwarddb can determine if a journal file was
**      copied before the switch occurred.
**
** History:
**     04-mar-2008 (stial01)
**          Created for Incremental rollforwarddb.
*/
typedef struct
{
    DM0L_HEADER		js_header;     /* Standard log record header. */
    i4			js_reserve1;
    i4                  js_reserve2;
    i4                  js_reserve3;
    i4                  js_reserve4;
    DB_TAB_TIMESTAMP	js_time;       /* Time of jnl switch */
}   DM0L_JNL_SWITCH;
/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS dm0l_bt(
			i4		    lg_id,
			i4		    flag,
			i4		    llid,
			DMP_DCB		    *dcb,
			DB_OWN_NAME	    *user_name,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_et(
			i4             lg_id,
			i4		    flag,
			i4             state,
			DB_TRAN_ID	    *tran_id,
			i4		    db_id,
			DB_TAB_TIMESTAMP    *ctime,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_bi(
			i4         log_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *name,
			DB_OWN_NAME     *owner,
			i4         pg_type,
			i4         loc_cnt,
			i4         loc_config_id,
			i4		operation,
			i4		page_number,
			i4		page_size,
			DMPP_PAGE       *page,
			LG_LSN		*prev_lsn,
			LG_LSN		*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_ai(
			i4         log_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *name,
			DB_OWN_NAME     *owner,
			i4         pg_type,
			i4         loc_cnt,
			i4         loc_config_id,
			i4		operation,
			i4		page_number,
			i4		page_size,
			DMPP_PAGE       *page,
			LG_LSN		*prev_lsn,
			LG_LSN		*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_put(
			i4         log_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *name,
			i2		name_len,
			DB_OWN_NAME     *owner,
			i2		owner_len,
			DM_TID          *tid,
			i4         pg_type,
			i4		page_size,
			i4         comp_type,
			i4         loc_config_id,
			i4         loc_cnt,
			i4         size,
			PTR             record,
			LG_LSN    	 *prev_lsn,
			i4		row_version,
			DMPP_SEG_HDR	*seg_hdr,
			LG_LRI    	 *lri,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_del(
			i4		log_id,
			i4		flag,
			DB_TAB_ID	*tbl_id,
			DB_TAB_NAME     *name,
			i2		name_len,
			DB_OWN_NAME	*owner,
			i2		owner_len,
			DM_TID		*tid,
			i4         pg_type,
			i4         comp_type,
			i4		page_size,
			i4         loc_config_id,
			i4         loc_cnt,
			i4		size,
			PTR		record,
			LG_LSN    	*prev_lsn,
			i4		row_version,
			DMPP_SEG_HDR	*seg_hdr,
			u_i4		row_tran_id,
			u_i2		row_lg_id,
			LG_LRI		*lri,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_rep(
			i4		log_id,
			i4		flag,
			DB_TAB_ID	*tbl_id,
			DB_TAB_NAME	*name,
			i2		name_len,
			DB_OWN_NAME	*owner,
			i2		owner_len,
			DM_TID		*otid,
			DM_TID		*ntid,
			i4		pg_type,
			i4		page_size,
			i4              comp_type,
			i4		oloc_config_id,
			i4		nloc_config_id,
			i4		loc_cnt,
			i4		osize,
			i4		nsize,
			PTR		orecord,
			PTR		nrecord,
			LG_LSN     	*prev_lsn,
			char            *comp_odata,
			i4         comp_odata_len,
			char            *comp_ndata,
			i4         comp_ndata_len,
			i4         diff_offset,
			bool            comp_orow,
			i4		orow_version,
			i4		nrow_version,
			u_i4		row_tran_id,
			u_i2		row_lg_id,
			LG_LRI		*lri,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_bm(
		    DMP_RCB	      *rcb,
    		    LG_LSN       *lsn,
		    DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_em(
		    DMP_RCB	    *rcb,
		    LG_LSN	    *prev_lsn,
		    LG_LSN	    *lsn,
		    DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_savepoint(
		    i4             lg_id,
		    i4		sp_id,
		    i4		flag,
		    LG_LSN          *sp_lsn,
		    DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_abortsave(
			i4             lg_id,
			i4		    sp_id,
			i4		    flags,
			LG_LSN	    *savepoint_lsn,
			LG_LSN	    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_frename(
			i4             lg_id,
			i4		    flag,
			DM_PATH		    *path,
			i4		    l_path,
			DM_FILE		    *file,
			DM_FILE		    *nfile,
			DB_TAB_ID	    *tbl_id,
			i4             loc_id,
			i4             cnf_loc_id,
			LG_LSN         *prev_lsn,
			LG_LSN         *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_fcreate(
			i4             lg_id,
			i4		    flag,
			DM_PATH		    *path,
			i4		    l_path,
			DM_FILE		    *file,
			DB_TAB_ID 	    *tbl_id,
			i4 	    loc_id,
			i4		    cnf_loc_id,
			i4		    page_size,
			i4		    loc_status,
			LG_LSN		    *prev_lsn,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_opendb(
			i4		    lg_id,
			i4		    flag,
			DB_DB_NAME          *name,
			DB_OWN_NAME	    *owner,
			i4             ext_dbid,
			DM_PATH		    *path,
			i4		    l_path,
			i4		    *db_id,
			LG_LSN		    *special_lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_closedb(
			i4		    db_id,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_create(
			i4             	lg_id,
			i4             	flag,
			DB_TAB_ID           	*tbl_id,
			DB_TAB_NAME         	*name,
			DB_OWN_NAME         	*owner,
			i4             	allocation,
			i4			create_flags,
			i4			relstat,
			DM_PAGENO		fhdr_pageno,
			DM_PAGENO		first_fmap_pageno,
			DM_PAGENO		last_fmap_pageno,
			DM_PAGENO		first_free_pageno,
			DM_PAGENO		last_free_pageno,
			DM_PAGENO		first_used_pageno,
			DM_PAGENO		last_used_pageno,
			i4			loc_count,
			DB_LOC_NAME		*location,
			i4			page_size,
			i4                      page_type,
			LG_LSN			*prev_lsn,
			LG_LSN			*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_destroy(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID	    *tbl_id,
			DB_TAB_NAME	    *name,
			DB_OWN_NAME	    *owner,
			i4             loc_count,
			DB_LOC_NAME	    *location,
			LG_LSN	    *prev_lsn,
			LG_LSN	    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_relocate(
			i4		    lg_id,
			i4		    flag,
			DB_TAB_ID	    *tbl_id,
			DB_TAB_NAME	    *name,
			DB_OWN_NAME	    *owner,
			DB_LOC_NAME	    *olocation,
			DB_LOC_NAME	    *nlocation,
			i4             page_size,
			i4		    loc_id,
			i4		    oloc_config_id,
			i4		    nloc_config_id,
			DM_FILE		    *file,
			LG_LSN	    *prev_lsn,
			LG_LSN	    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_modify(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID	    *tbl_id,
			DB_TAB_NAME	    *name,
			DB_OWN_NAME	    *owner,
			i4             loc_count,
			DB_LOC_NAME	    *location,
			i4		    structure,
			i4		    relstat,
			i4		    min_pages,
			i4		    max_pages,
			i4		    ifill,
			i4		    dfill,
			i4		    lfill,
			i4		    reltups,
			i4		    buckets,
			DM_ATT		    *key_map,
			char		    *key_order,
			i4             allocation,
			i4             extend,
			i4		    pg_type,
			i4		    page_size,
			i4		    modify_flag,
			DB_TAB_ID	    *rnl_tabid,
			LG_LSN		    bsf_lsn,
			i2		    name_id,
			i4		    name_gen,
			LG_LSN		    *prev_lsn,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_index(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID	    *tbl_id,
			DB_TAB_ID	    *idx_tbl_id,
			DB_TAB_NAME	    *name,
			DB_OWN_NAME	    *owner,
			i4             loc_count,
			DB_LOC_NAME	    *location,
			i4		    structure,
			u_i4		    relstat,
			u_i4		    relstat2,
			i4		    min_pages,
			i4		    max_pages,
			i4		    ifill,
			i4		    dfill,
			i4		    lfill,
			i4		    reltups,
			i4		    buckets,
			DM_ATT		    *key_map,
			i4		    kcount,
			i4             allocation,
			i4             extend,
			i4		    pg_type,
			i4		    page_size,
			i2		    comptype,
			i2		    ixcomptype,
			i2		    name_id,
			i4		    name_gen,
			i4		    dimension,
			i4		    hilbertsize,
			f8		    *range,
			LG_LSN		    *prev_lsn,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_alter(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID	    *tbl_id,
			DB_TAB_NAME         *name,
			DB_OWN_NAME	    *owner,
			i4		    count,
			i4		    action,
			LG_LSN	    *prev_lsn,
			LG_LSN	    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_position(
			i4             flag,
			i4		    lx_id,
			LG_LA		    *lga,
			DMP_LCTX	    *lctx,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_read(
			DMP_LCTX            *lctx,
			i4		    lx_id,
			DM0L_HEADER	    **record,
			LG_LA		    *lga,
			i4		    *bufid,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_force(
			i4             lg_id,
			LG_LSN		    *lsn,
			LG_LSN		    *nlsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_allocate(
			i4		    flag,
			i4		    size,
			i4		    lg_id,
			char                *name,
			i4             l_name,
			DMP_LCTX	    **lctx_ptr,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_deallocate(
			DMP_LCTX            **lctx,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_bcp(
			i4             lg_id,
			LG_LA		    *bcp_la,
			LG_LSN	    *bcp_lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_ecp(
			i4             lg_id,
			LG_LA		    *bcp_la,
			LG_LSN	    *bcp_lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_sm1_rename(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID           *tbl_id,
			DB_TAB_ID           *tmp_id,
			DM_PATH		    *path,
			i4		    l_path,
			DM_FILE		    *oldfile,
			DM_FILE		    *tempfile,
			DM_FILE		    *newfile,
			DM_FILE		    *renamefile,
			LG_LSN         *prev_lsn,
			LG_LSN         *lsn,
			i4		    oldpgtype,
			i4		    newpgtype,
			i4		    oldpgsize,
			i4		    newpgsize,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_sm2_config(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID           *tbl_id,
			DMP_RELATION        *oldrel,
			DMP_RELATION        *newrel,
			DMP_RELATION        *oldidx,
			DMP_RELATION        *newidx,
			LG_LSN         *prev_lsn,
			LG_LSN         *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_sm0_closepurge(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID           *tbl_id,
			LG_LSN		    *la,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_location(
			i4             lg_id,
			i4             flag,
			i4		    type,
			DB_LOC_NAME         *name,
			i4             l_extent,
			DM_PATH             *extent,
			i4		raw_start,
			i4		raw_blocks,
			i4		raw_total_blocks,
			LG_LSN	    *prev_lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_ext_alter(
			i4             lg_id,
 			i4             flag,
 			i4             otype,
 			i4             ntype,
 			DB_LOC_NAME         *name,
 			LG_LSN              *prev_lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_setbof(
			i4             lg_id,
			LG_LA	    *old_bof,
			LG_LA	    *new_bof,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_jnleof(
			i4             lg_id,
			i4		    db_id,
			DB_DB_NAME	    *dbname,
			DB_OWN_NAME	    *dbowner,
			i4             l_root,
			DM_PATH             *root,
			i4		    node,
			i4		    old_eof,
			i4		    new_eof,
			LG_LA	    *old_la,
			LG_LA	    *new_la,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_archive(DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dm0l_drain(
			DMP_DCB		    *dcb,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_load(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID	    *tbl_id,
			DB_TAB_NAME	    *name,
			DB_OWN_NAME	    *owner,
			DB_LOC_NAME	    *loc,
			i4		    type,
			i4		    recreate,
			i4             lastpage,
			LG_LSN	    	    *prev_lsn,
			LG_LSN	    	    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_secure(
			i4		   lg_id,
			i4		    lock_id,
			DB_TRAN_ID	    *tran_id,
			DB_DIS_TRAN_ID	    *dis_tran_id,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_sbackup(
			i4		    flag,
			i4             lg_id,
			DB_OWN_NAME	    *username,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_ebackup(
			i4             lg_id,
			DB_OWN_NAME	    *username,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_crdb(
			i4             lg_id,
			i4             flag,
			i4             db_id,
			DB_DB_NAME          *db_name,
			i4             db_access,
			i4             db_service,
			DB_LOC_NAME         *db_location,
			i4             db_l_root,
			DM_PATH             *db_root,
			LG_LSN	    	    *prev_lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_dmu(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID	    *tbl_id,
			DB_TAB_NAME	    *tbl_name,
			DB_OWN_NAME	    *tbl_owner,
			i4		    operation,
			LG_LSN		    *prev_lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_test(
			i4             lg_id,
			i4		    flag,
			i4		    test_number,
			i4		    p1,
			i4		    p2,
			i4		    p3,
			i4		    p4,
			char		    *p5,
			i4		    p5l,
			char		    *p6,
			i4		    p6l,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_assoc(
			i4         lg_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *name,
			i2		name_len,
			DB_OWN_NAME     *owner,
			i2		owner_len,
			i4         pg_type,
			i4		page_size,
			i4         loc_cnt,
			i4         leaf_conf_id,
			i4         odata_conf_id,
			i4         ndata_conf_id,
			DM_PAGENO       leafpage,
			DM_PAGENO       oldpage,
			DM_PAGENO	newpage,
			LG_LSN		*prev_lsn,
			LG_LRI		*lrileaf,
			LG_LRI		*lridata,
			DB_ERROR	    *dberr);


FUNC_EXTERN DB_STATUS dm0l_alloc(
			i4         lg_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *tbl_name,
			i2		name_len,
			DB_OWN_NAME     *tbl_owner,
			i2		owner_len,
			i4         pg_type,
			i4		page_size,
			i4         loc_cnt,
			DM_PAGENO       fhdr_pageno,
			DM_PAGENO       fmap_pageno,
			DM_PAGENO       free_pageno,
			i4         fmap_index,
			i4         fhdr_config_id,
			i4         fmap_config_id,
			i4         free_config_id,
			bool            hwmap_update,
			bool            freehint_update,
			LG_LSN     	*prev_lsn,
			LG_LSN     	*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_dealloc(
			i4         lg_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *tbl_name,
			i2		name_len,
			DB_OWN_NAME     *tbl_owner,
			i2		owner_len,
			i4         pg_type,
			i4		page_size,
			i4         loc_cnt,
			DM_PAGENO       fhdr_pageno,
			DM_PAGENO       fmap_pageno,
			DM_PAGENO       free_pageno,
			i4         fmap_index,
			i4         fhdr_config_id,
			i4         fmap_config_id,
			i4         free_config_id,
			bool            freehint_update,
			LG_LSN     	*prev_lsn,
			LG_LSN     	*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_extend(
			i4         lg_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *tbl_name,
			DB_OWN_NAME     *tbl_owner,
			i4         pg_type,
			i4		page_size,
			i4         loc_cnt,
			i4 	*cnf_loc_array,
			DM_PAGENO       fhdr_pageno,
			DM_PAGENO       fmap_pageno,
			i4         fmap_index,
			i4         fhdr_config_id,
			i4         fmap_config_id,
			DM_PAGENO       first_used,
			DM_PAGENO       last_used,
			DM_PAGENO       first_free,
			DM_PAGENO       last_free,
			i4         oldeof,
			i4         neweof,
			bool            hwmap_update,
			bool            freehint_update,
			bool            fmap_update,
			LG_LSN     	*prev_lsn,
			LG_LSN     	*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_ovfl(
			i4         lg_id,
			i4         flag,
			bool    	fullc,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *name,
			i2		name_len,
			DB_OWN_NAME     *owner,
			i2		owner_len,
			i4         pg_type,
			i4		page_size,
			i4         loc_cnt,
			i4         config_id,
			i4         new_config_id,
			DM_PAGENO       newpage,
			DM_PAGENO       rootpage,
			DM_PAGENO       ovfl_ptr,
			DM_PAGENO       main_ptr,
			LG_LSN     *prev_lsn,
			LG_LRI     *lri,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_nofull(
			i4         log_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *name,
			i2		name_len,
			DB_OWN_NAME     *owner,
			i2		owner_len,
			i4         pg_type,
			i4		page_size,
			DM_PAGENO       pageno,
			i4         loc_config_id,
			i4         loc_cnt,
			LG_LSN     	*prev_lsn,
			LG_LRI     	*lri,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_fmap(
			i4         lg_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *tbl_name,
			DB_OWN_NAME     *tbl_owner,
			i4         pg_type,
			i4		page_size,
			i4         loc_cnt,
			DM_PAGENO       fhdr_pageno,
			DM_PAGENO       fmap_pageno,
			i4         fmap_index,
			i4		fmap_hw_mark,
			i4         fhdr_config_id,
			i4         fmap_config_id,
			DM_PAGENO       first_used,
			DM_PAGENO       last_used,
			DM_PAGENO       first_free,
			DM_PAGENO       last_free,
			LG_LSN     	*prev_lsn,
			LG_LSN     	*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_ufmap(
			i4         lg_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *tbl_name,
			DB_OWN_NAME     *tbl_owner,
			i4         pg_type,
			i4		page_size,
			i4         loc_cnt,
			DM_PAGENO       fhdr_pageno,
			DM_PAGENO       fmap_pageno,
			i4         fmap_index,
			i4		fmap_hw_mark,
			i4         fhdr_config_id,
			i4         fmap_config_id,
			DM_PAGENO       first_used,
			DM_PAGENO       last_used,
			LG_LSN     	*prev_lsn,
			LG_LSN     	*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_crverify(
			i4             lg_id,
			i4             flag,
			DB_TAB_ID           *tbl_id,
			DB_TAB_NAME         *name,
			DB_OWN_NAME	    *owner,
			i4             allocation,
			i4             fhdr_pageno,
			i4             num_fmaps,
			i4             relpages,
			i4             relmain,
			i4		    relprim,
			i4		    page_size,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_btput(
			i4         lg_id,
			i4		flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME	*tbl_name,
			i2		name_len,
			DB_OWN_NAME	*tbl_owner,
			i2		owner_len,
			i4		pg_type,
			i4		page_size,
			i4		compression_type,
			i4		loc_cnt,
			i4		loc_config_id,
			DM_TID		*bid,
			i2		bid_child,
			DM_TID		*tid,
			i4		key_length,
			char		*key,
			LG_LSN		*prev_lsn,
			LG_LRI    	 *lri,
			i4		partno,
			u_i4		btflags,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_btdel(
			i4         lg_id,
			i4		flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME	*tbl_name,
			i2		name_len,
			DB_OWN_NAME	*tbl_owner,
			i2		owner_len,
			i4		pg_type,
			i4		page_size,
			i4		compression_type,
			i4		loc_cnt,
			i4		loc_config_id,
			DM_TID		*bid,
			i2		bid_child,
			DM_TID		*tid,
			i4		key_length,
			char		*key,
			LG_LSN		*prev_lsn,
			LG_LRI    	 *lri,
			i4		partno,
			u_i4		btflags,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_btsplit(
			i4         lg_id,
			i4		flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME	*tbl_name,
			DB_OWN_NAME	*tbl_owner,
			i4		pg_type,
			i4		page_size,
			i4              kperpage,
			i4		compression_type,
			i4		loc_cnt,
			i4		cur_loc_cnf_id,
			i4		sib_loc_cnf_id,
			i4		dat_loc_cnf_id,
			i4		cur_pageno,
			i4		sib_pageno,
			i4		dat_pageno,
			i4		split_position,
			i4		split_direction,
			i4		index_keylen,
			i4		range_keylen,
			i4		descriptor_keylength,
			char		*descriptor_key,
			DMPP_PAGE	*split_page,
			LG_LSN		*prev_lsn,
			LG_LRI		*lri,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_btovfl(
			i4         lg_id,
			i4		flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME	*tbl_name,
			DB_OWN_NAME	*tbl_owner,
			i4		pg_type,
			i4		page_size,
			i4		compression_type,
			i4		loc_cnt,
			i4		keylen,
			i4		leaf_loc_cnf_id,
			i4		ovfl_loc_cnf_id,
			i4		leaf_pageno,
			i4		ovfl_pageno,
			i4		mainpage,
			i4		nextpage,
			i4		nextovfl,
			i4		lrange_len,
			char		*lrange_key,
			DM_TID		*lrange_tid,
			i4		rrange_len,
			char		*rrange_key,
			DM_TID		*rrange_tid,
			LG_LSN		*prev_lsn,
			LG_LRI		*lri,
			i2		tidsize,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_btfree(
			i4         lg_id,
			i4		flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME	*tbl_name,
			DB_OWN_NAME	*tbl_owner,
			i4		pg_type,
			i4		page_size,
			i4		compression_type,
			i4		loc_cnt,
			i4		keylen,
			i4		ovfl_loc_cnf_id,
			i4		prev_loc_cnf_id,
			i4		ovfl_pageno,
			i4		prev_pageno,
			i4		mainpage,
			i4		nextpage,
			i4		nextovfl,
			i4		dupkey_len,
			char		*duplicate_key,
			i4		lrange_len,
			char		*lrange_key,
			DM_TID		*lrange_tid,
			i4		rrange_len,
			char		*rrange_key,
			DM_TID		*rrange_tid,
			LG_LSN		*prev_lsn,
			LG_LSN		*lsn,
			i2		tidsize,
			u_i4		btflags,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_btupdovfl(
			i4         lg_id,
			i4		flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME	*tbl_name,
			DB_OWN_NAME	*tbl_owner,
			i4		pg_type,
			i4		page_size,
			i4		compression_type,
			i4		loc_cnt,
			i4		loc_cnf_id,
			i4		pageno,
			i4		mainpage,
			i4		omainpage,
			i4		nextpage,
			i4		onextpage,
			i4		lrange_len,
			char		*lrange_key,
			DM_TID		*lrange_tid,
			i4		olrange_len,
			char		*olrange_key,
			DM_TID		*olrange_tid,
			i4		rrange_len,
			char		*rrange_key,
			DM_TID		*rrange_tid,
			i4		orrange_len,
			char		*orrange_key,
			DM_TID		*orrange_tid,
			LG_LSN		*prev_lsn,
			LG_LSN		*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_rtdel(
			i4         lg_id,
			i4		flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME	*tbl_name,
			i2		name_len,
			DB_OWN_NAME	*tbl_owner,
			i2		owner_len,
			i4		pg_type,
			i4		page_size,
			i4		compression_type,
			i4		loc_cnt,
			i4		loc_config_id,
			u_i2		hilbertsize,
			DB_DT_ID	obj_dt_id,
			DM_TID		*bid,
			DM_TID		*tid,
			i4		key_length,
			char		*key,
			i4		stack_length,
			DM_TID		*stack,
			LG_LSN		*prev_lsn,
			LG_LSN		*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_rtput(
			i4         lg_id,
			i4		flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME	*tbl_name,
			i2		name_len,
			DB_OWN_NAME	*tbl_owner,
			i2		owner_name,
			i4		pg_type,
			i4		page_size,
			i4		compression_type,
			i4		loc_cnt,
			i4		loc_config_id,
			u_i2		hilbertsize,
			DB_DT_ID	obj_dt_id,
			DM_TID		*bid,
			DM_TID		*tid,
			i4		key_length,
			char		*key,
			i4		stack_length,
			DM_TID		*stack,
			LG_LSN		*prev_lsn,
			LG_LSN		*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_rtrep(
			i4         lg_id,
			i4		flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME	*tbl_name,
			i2		name_len,
			DB_OWN_NAME	*tbl_owner,
			i2		owner_len,
			i4		pg_type,
			i4		page_size,
			i4		compression_type,
			i4		loc_cnt,
			i4		loc_config_id,
			u_i2		hilbertsize,
			DB_DT_ID	obj_dt_id,
			DM_TID		*bid,
			DM_TID		*tid,
			i4		key_length,
			char		*oldkey,
			char		*newkey,
			i4		stack_length,
			DM_TID		*stack,
			LG_LSN		*prev_lsn,
			LG_LSN		*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_disassoc(
			i4         lg_id,
			i4         flag,
			DB_TAB_ID       *tbl_id,
			DB_TAB_NAME     *name,
			DB_OWN_NAME     *owner,
			i4         pg_type,
			i4		page_size,
			i4         loc_cnt,
			i4         loc_config_id,
			DM_PAGENO	page_number,
			LG_LSN     	*prev_lsn,
			LG_LSN     	*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_rawdata(
			i4	lg_id,
			i4	flag,
			i2	type,
			i2	instance,
			i4	total_size,
			i4	this_offset,
			i4	this_size,
			PTR	data,
			LG_LSN	*prev_lsn,
			LG_LSN	*lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_bsf(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID	    *tbl_id,
			DB_TAB_NAME	    *name,
			DB_OWN_NAME	    *owner,
			LG_LSN		    *prev_lsn,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_esf(
			i4             lg_id,
			i4		    flag,
			DB_TAB_ID	    *tbl_id,
			DB_TAB_NAME	    *name,
			DB_OWN_NAME	    *owner,
			LG_LSN		    *prev_lsn,
			LG_LSN		    *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN DB_STATUS dm0l_rnl_lsn(
			i4		    lg_id,
			i4                  flag,
			DB_TAB_ID           *tbl_id,
			DB_TAB_NAME         *name,
			DB_OWN_NAME         *owner,
			LG_LSN		    bsf_lsn,
			DM_PAGENO	    lastpage,
			i4		    lsn_totcnt,
			i4		    lsn_cnt,
			PTR		    rnl_lsn,
			LG_LSN              *prev_lsn,
			LG_LSN              *lsn,
			DB_ERROR	    *dberr);

FUNC_EXTERN VOID dm0l_uncomp_replace_log(
			PTR		orecord,
			i4		l_orecord,
			PTR		nrecord,
			i4		noffset,
			i4		l_comp,
			i4		l_nrecord,
			PTR		rep_rec);

FUNC_EXTERN VOID dm0l_row_unpack(
			char		*base_record,
			i4		base_record_len,
			char		*diff_data,
			i4		diff_data_len,
			char		*res_buf,
			i4		res_row_len,
			i4		diff_offset);

FUNC_EXTERN VOID dm0l_repl_comp_info(
			PTR		orecord,
			i4		l_orecord,
			PTR		nrecord,
			i4		l_nrecord,
			DB_TAB_ID	*tbl_id,
			DM_TID		*otid,
			DM_TID		*ntid,
			i4		delta_start,
			i4		delta_end,
			char		**comp_odata,
			i4		*comp_odata_len,
			char		**comp_ndata,
			i4		*comp_ndata_len,
			i4		*diff_offset,
			bool		*comp_orow);

FUNC_EXTERN DB_STATUS dm0l_del_location(
			i4             lg_id,
			i4             flag,
			i4		    type,
			DB_LOC_NAME         *name,
			i4             l_extent,
			DM_PATH             *extent,
			LG_LSN	    *prev_lsn,
			DB_ERROR	    *dberr);

