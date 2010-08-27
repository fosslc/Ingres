/*
**Copyright (c) 2004 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <scf.h>
#include    <dm.h>
#include    <dmpp.h>
#include    <dmp.h>
#include    <dm1b.h>
#include    <dm0c.h>
#include    <dm0llctx.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dmftrace.h>
#include    <dmd.h>

/**
**
**  Name: DM0L.C - Logging primitives.
**
**  Description:
**      This file contains the set of routines that define the format of
**	the log records and interface with the actual operating system dependent
**	logging primitives.
**
** 	dm0l_bt	- Begin Transaction.
** 	dm0l_et	- End Transaction.
** 	dm0l_bi	- Write Before Image log record.
** 	dm0l_ai	- Write After Image log record.
** 	dm0l_put - Write Put Record log record.
** 	dm0l_del - Write Delete Record log record.
** 	dm0l_rep - Write Replace Record log record.
** 	dm0l_bm	- Extract Begin Mini-Transaction LSN.
** 	dm0l_em	- Write End Mini-Transaction record.
** 	dm0l_savepoint - Extract Savepoint LSN.
** 	dm0l_abortsave - Write Abort savepoint record.
** 	dm0l_frename - Write a File rename record.
** 	dm0l_fcreate - Write a File create record.
** 	dm0l_opendb - Write an Open database log record.
** 	dm0l_closedb - Remove this database from this process.
** 	dm0l_create - Write Create table record.
** 	dm0l_crverify - Write Create table verify.
** 	dm0l_destroy - Write Destroy table record.
** 	dm0l_relocate - Write Relocate table record.
** 	dm0l_modify - Write Modify table record.
** 	dm0l_index - Write Index table record.
** 	dm0l_alter - Write Alter table record.
** 	dm0l_position - Position log file for reading.
** 	dm0l_read - Read next record from transaction.
** 	dm0l_force - Force log to disk.
** 	dm0l_allocate - Allocate and initialize the LCTX
** 	dm0l_deallocate - Deallocate the LCTX
** 	dm0l_bcp - Write a begin consistency point log record.
** 	dm0l_sm1_rename - Write a SYSMOD file rename record.
** 	dm0l_sm2_config	- Write a SYSMOD update config file record.
** 	dm0l_sm0_closepurge - Write SYSMOD load completion log record.
** 	dm0l_location - Write a add location record.
** 	dm0l_ext_alter - Log alteration of extent info in config file
** 	dm0l_setbof - Write a set beginning of log record.
** 	dm0l_jnleof - Write a set journal eof record.
** 	dm0l_archive - CLuster recovery archive synchronize record.
** 	dm0l_drain - Drain the log to the journals for this database.
** 	dm0l_load - Write Load table record.
** 	dm0l_sbackup - Write Start backup log record.
** 	dm0l_ebackup - Write end backup log record.
** 	dm0l_crdb - Create Database.
** 	dm0l_dmu - Log DMU operation.
** 	dm0l_test - Write a Test log record.
** 	dm0l_assoc - Write an change association log record.
** 	dm0l_alloc - Write an allocate page log record.
** 	dm0l_dealloc - Write an deallocate page log record.
** 	dm0l_extend - Write an extend table log record.
** 	dm0l_ovfl - Write an overflow page log record.
** 	dm0l_fmap - Write an add fmap log record.
** 	dm0l_ufmap - Write an update fmap log record.
** 	dm0l_btput - Write a Btree Insert log record.
** 	dm0l_btdel - Write a Btree Delete log record.
** 	dm0l_btsplit - Write a Btree Index Split log record.
** 	dm0l_btovfl - Write a Btree Allocate Overflow Chain log record.
** 	dm0l_btfree - Write a Btree Deallocate Overflow Chain log record.
** 	dm0l_btupdovfl 	- Write a Btree Update Overflow Chain log record.
** 	dm0l_rtdel - Write an Rtree Delete log record.
** 	dm0l_rtput - Write an Rtree Put log record.
** 	dm0l_rtrep - Write an Rtree Replace log record.
** 	dm0l_disassoc - Write a change association log record.
**	dm0l_rawdata - Write a raw data log record.
** 	dm0l_repl_comp_info - Find offset & length of changed bytes in row
** 	dm0l_row_unpack	- Uncompress row in replace log record.

**
**
**  History:
**      17-may-1986 (Derek)
**          Created for Jupiter.
**	17-may-1988 (EdHsu)
**	    Add dm0l_bcp and dm0l_ecp for the fast commit project.
**	30-sep-1988 (rogerk)
**	    Added LG_FIRST flags to the LGwrite calls in dm0l_bt, dm0l_opendb,
**	    dm0l_closedb, dm0l_bcp, dm0l_ecp, dm0l_setbof, dm0l_jnleof.
**	    These dm0l calls may be made before a previous dm0l_bt has been
**	    done.  This is to allow LG to make sure that all transactions
**	    begin with a Begin Transaction record.
**	30-Jan-1989 (ac)
**	    Added arguments to LGbegin().
**	21-jan-1989 (Edhsu)
**	    Add dm0l_sbackup and dm0l_ebackup routine to support online
**	    backup on the Terminator Project.
**	15-may-1989 (rogerk)
**	    Added checks for LG_EXCEED_LIMIT in dm0l_bt, dm0l_robt, dm0l_opendb,
**	    dm0l_allocate.
**	11-sep-1988 (rogerk)
**	    Added dm0l_dmu routine.
**	    Added lx_id parameter to dm0l_opendb/dm0l_closedb calls.  Keep
**	    transaction open for writing closedb record while db is open.
**	19-sep-1989 (walt)
**	    Fixed problem in dm0l_bt where the lock key for LK_CKP_CLUSTER
**	    was incompletely initialized.  Changed MEcopy to move 16 bytes
**	    rather than only 12.  The uninitialized bytes caused the lock
**	    name to nolonger match the "same" name taken out on another node.
**	20-sep-1989 (rogerk)
**	    Log savepoint record before logging DMU operation.  The savepoint
**	    address is put into the DMU record.
**	 2-oct-1989 (rogerk)
**	    Added dm0l_fswap routine.  This operation is used during MODIFY
**	    to swap new file with old one and move old file to delete file.
**	10-jan-1990 (rogerk)
**	    Made changes for Online Backup.  Added DM0L_JOURNAL flag for
**	    dm0l_sbackup call.  If database is journled, then signal the
**	    logging system before and after writing the SBACKUP record to
**	    make sure the archiver includes the SBACKUP record in the next
**	    archive window.
**	10-jan-1990 (rogerk)
**	    Bug fix for cluster DMU aborts.  Added support for calling
**	    dm0l_abortsave from CSP.  If DM0L_CLUSTER_REQ flag is passed
**	    in then call LGCwrite instead of LGwrite.
**	15-jan-1990 (rogerk)
**	    Added Log Address parameter to dm0l_sbackup and dm0l_ebackup
**	    routines to allow checkpoint to give this information in
**	    debug trace messages.
**	17-jan-1990 (rogerk)
**	    Don't call LGend if get an error in dm0l_bt.  Leave this up to
**	    the caller.
**	23-jan-1990 (rogerk)
**	    Force OPENDB log record to avoid db-inconsistent problem when
**	    machine crashes.
**	30-jan-1990 (ac)
**	    Get the commit timestamp for the read only transaction.
**	15-feb-1990 (mikem)
**	    Fixed calculation of the size of a subset of members in a structure.
**	    Previously the size of the "header" a stucture with a buffer at
**	    it's e end was calculated as:
**	         "sizeof(structure) - sizeof(struct.buffer)"
**	    The problem with this is that on some alignment conscious compilers
**	    put padding in structures at the end.  This results in the above
**	    calculation being too large by the size of that pad.  All of these
**	    calculations were changed to:
**	         "((char *)&struct.buffer) - ((char *)&struct)"
**	    This calculation is very dependent on the current layout of the
**	    structure.
**	19-feb-1990 (greg)
**	    Merge MikeM's change with previous changes made by Roger and Annie.
**	    (Mike's on vacation, his change is needed for sun4).
**	 9-apr-1990 (sandyh)
**	    Added DMZ_SES_MACRO(11) check for tracing log header type & size.
**	17-may-1990 (rogerk)
**	    Add check in dm0l_allocate for LG_DBINFO_MAX constant definition
**	    being large enough to hold maximum database name information.  This
**	    prevents us from changing the DB_MAXNAME/DB_AREA_MAX constants
**	    without updating the logging system database buffers.
**	29-aug-1990 (bryanp)
**	    Pass LG_FORCE flag to LGwrite() calls in dm0l_fswap(),
**	    dm0l_fcreate(), and dm0l_frename(). This ensures Write-Ahead
**	    Logging is satisfied.
**	19-nov-1990 (bryanp)
**	    log_error() was being called inconsistently. Fixed callers to pass
**	    the LG error code to log_error().
**	25-feb-1991 (rogerk)
**	    Added dm0l_test routine for implementing logging and journaling
**	    system tests.  Added DM0L_NOLOGGING flag to allow database to be
**	    opened while the Log File is full.  This was added for the Archiver
**	    Stability project.
**	16-jul-1991 (bryanp)
**	    Log the LG error code in log_error().
**	    B38527: Add new 'reltups' argument to dm0l_modify & dm0l_index.
**	    Also, added new subroutine dm0l_sm0_closepurge.
**      15-aug-91 (jnash) merged 04-apr-1989 (Derek)
**          Resurrect dm0l_crdb log record for create database support.
**      15-aug-1991 (jnash) merged 14-jun-1990 (Derek)
**          Removed CALLOC,BDEALLOC,BALLOC,BPEXTEND log records,
**	    added ASSOC,ALLOC,DEALLOC,EXTEND log records,
**	    added new fields to CREATE,MODIFY,INDEX and FCREATE.
**	    All changes are to support new allocation features.
**      15-aug-1991 (jnash) merged  24-sep-1990 (rogerk)
**          Merged 6.3 changes into 6.5.
**	 3-nov-1991 (rogerk)
**	    Added new log record DM0L_OVFL. This records the linking of a
**	    new data page into an overflow chain.  This information used to
**	    be logged through the dm0l_calloc log record which went away with
**	    the file extend changes.  This new record is used only for REDO.
**	5-nov-1991 (bryanp)
**	    Only log a LGclose error in dm0l_allocate if an LGclose error
**	    actually occurred (we were unconditionally calling log_error).
**      05-mar-1991 (mikem)
**          Disable before image logging based on DM901, to be used for
**          6.5 performance testing while compressed logging is not available.
**	    Recovery in a system with before image logging disabled will not
**	    work correctly.
**	27-may-1992 (rogerk)
**	    Initialized header flags field in dm0l_secure so that prepare
**	    to commit log records are not accidentally written with the
**	    JOURNAL flag.  This would cause rollforward to fail when
**	    encountering the log records in the journal history (B43916).
**	07-jul-92 (jrb)
**	    Prototyped DMF (protos in DM0L.H).
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	3-oct-1992 (bryanp)
**	    Changed meaning of DM0L_NOLOG parameter to dm0l_allocate().
**	    Added lx_id argument to dm0l_logforce -- we require that you be
**	    in a transaction of some sort in order to force the log.
**	    Prototyped LG/LK, which is somewhat of a mess, due to the
**	    redundancy of the DM_LOG_ADDR and LG_LA typedefs.
**	26-oct-1992 (rogerk & jnash)
**	    Reduced Logging Project:
**	      - Added new log information to most log record types for
**		new recovery protocols.
**	      - Added ability to log variable length name/owner values in
**		common log records and the find_var_name_len routine to
**		support this.
**	      - Removed db_id argument from dm0l_logforce.
**	      - Removed the checks for ifdef MVS and its setting of NOLGYET
**	        (not all routines checked for this flag before logging).
**	      - Removed routines dm0l_sbi, dm0l_sput, dm0l_srep, and
**		dm0l_sdel - system catalog updates are now logged with
**		normal update records.
**	      - Add dm0l_nofull log record, used in recovery of hash
**		table fullchain status.
**	      - Remove dm0l_fswap log record, use dm0l_frename instead.
**	      - Add new dm0l_crverify log record.
**	      - Removed dm0l_logforce routine - all callers now use dm0l_force.
**	      - Add new btree logging calls: dm0l_btput, dm0l_btdel,
** 		dm0l_btsplit, dm0l_btovfl, dm0l_btfree, dm0l_btupdovfl.
**	      - Add CLR handling.
**	      - Add LGreserve calls where appropriate.  The idea is to do
**	  	LGreserve's here if page mutexing operations are unaffected.
**	15-jan-1993 (jnash)
**	    Add support for zero length rows written by CLRs.
**	18-jan-1993 (rogerk)
**	    Removed WILLING_COMMIT flag from dm0l_opendb.
**	    Made CLR of DMU be a NONREDO operation.  Removed the savepoint
**	    from DMU logging.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed prev_lsn field in the log record header to compensated_lsn.
**	    Changed LGwrite to return an LSN rather than a log address.
**	    Add "flag" param to dm0l_savepoint() to allow savepoint log records
**		to be journaled.
**	    Changed BMini and EMini records to be journaled if the database is.
**	    Changed EndMini log record to look like a CLR record.  The
**		compensated_lsn field now takes the place of the old
**		em_bm_address.
**	    Changed BCP log records to contain external database id's rather
**		than the LG db_id (which is only temporary).
**	    Removed ADDONLY flag from dm0l_opendb, dm0l_closedb routines.
**	    Changed dm0l_bt to show database with LG_S_DATABASE.
**	30-feb-1993 (rmuth)
**	    Include di.h for dm0c.h
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		LGC* interface is gone; just make LG calls.
**		Changes to dm0l_allocate interface to support multiple open logs
**		You have to pass a transaction ID into dm0l_read.
**		Call LGopen with node name passed in. Check for DM0L_RECOVER
**		    flag. This causes logs to be opend with master flag.
**		Start a logreader transaction in dm0l_allocate for recovery use.
**		DM0L_FULL_FILENAME ==> LG_FULL_FILENAME -- used by standalone
**		    logdump.
**		Call dmd_logtrace AFTER writing the log record so that the
**		    proper LSN can be printed out (the LSN is generated by LG).
**		Added o_isjournaled field to DM0L_OPENDB log record to record
**		    whether a particular database is journaled or not.
**		In dm0l_position, we no longer move the lx_id to the lctx.
**		    This must be done separately.
**		Set lctx_node_id in dm0l_allocate if this is not a cluster.
**	26-apr-1993 (jnash)
**	    Include log space reservation param in dmd_logtrace calls.
**	    Note that we output what is reserved by LGreserve, not the more
**	    accurate info used here within dm0l (dm0l compresses some
**	    DB_MAXNAME information, not done at higher layers).
**	15-may-1993 (rmuth)
**	    Concurrent index support, add a flag field to dm0l_create.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	    Use LG_P_LAST in LGposition if DM0L_LAST is passed.
**	21-jun-1993 (bryanp)
**	    Clusters, too, should reserve logfile space for BT/ET pairs.
**	21-jun-1993 (rogerk)
**	    Add support in dm0l_btput for zero-length keys for btput CLRs.
**	    Added DM0L_SCAN_LOGFILE flag to dm0l_allocate to cause LGopen to do
**	    the logfile scan for bof_eof and to position the logfile for a full
**	    scan rather than only reading values between the logical bof and
**	    eof.  This can be used by logdump to dump entire log files.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Cease writing DM0L_CLOSEDB log records. These weren't of much use,
**	    because we couldn't trust them to tell us when a database was truly
**	    closed by all servers in a database (algorithms which attempt to
**	    match up OPENDB and CLOSEDB log records were doomed to failure
**	    because the logfile BOF might have already moved past some number
**	    of OPENDB log records, without any way of knowing how many such).
**	    Furthermore, leaving the $opendb transaction lying around to log the
**	    closedb log record was causing problems for 2PC situations where
**	    we wish dm2d_close_db to remove the DCB but not LGremove the
**	    database. We no longer keep the $opendb transaction lying around.
**	    Removed now-unneeded 'lx_id' and 'flag' arguments from open/closedb.
**	26-jul-1993 (rogerk)
**	  - Added back ecp_begin_la which holds the Log Address of the Begin
**	    Consistency Point record. Now the ECP has the LA and LSN of the BCP.
**	  - Enabled DM0L_DUMP flag on sbackup and ebackup log records.
**	  - Added split direction to SPLIT log records.
**	23-aug-1993 (rogerk)
**	    Added back prev_lsn param and CLR handling to dm0l_create routine.
**	    Added relstat to dm0l_create log record.
**	    Added a statement count field to the modify and index log records.
**	    Added location id information to dm0l_fcreate log record.
**	    Made SM1 log record be non-redo.
**	    Added lctx_bkcnt field to lctx structure to allow callers to
**		reference the log file size.  Initialized in dm0l_allocate.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**      10-sep-1993 (pearl)
**          Log error DM0152_DB_INCONSIST_DETAIL if database is inconsistent;
**          this error will name the database in question.  The logging has
**          been added to log_error(), which is called by all routines except
**          dm0l_opendb().  In dm0l_opendb(), call ule_format() directly;
**          the caller will log the other DMF errors.
**          Add static routine get_dbname(), which calls scf to get the
**          database name.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (jnash)
** 	    Add compression of replace log records.  Compresssion
**	    here means only changed bytes are logged.  Always compress "new
**	    rows", compress "old rows" via trace point.
**	18-oct-93 (jrb)
**	    Added dm0l_ext_alter for MLSorts.
**	18-oct-1993 (rogerk)
**	    Added buckets parameters to modify and index log records to
**	    facilitate rollforwarddb recreating tables in the same layout.
**	    Add user name to transaction information in bcp log records.
**	    Removed LG_A_DMU alter from dm0l_dmu operation.
**	30-oct-1993 (jnash)
**	  - Change pointer arithmetic to work on char *'s rather than PTR's.
**	  - Reserve space for EBACKUP log in dm0l_ebackup rather than
**	    dm0l_sbackup.  The ebackup log is not part of the same transaction.
**	  - Fix compression problem by not compressing records when new
**	    record is longer than old.
**	11-dec-1993 (rogerk)
**	    Changes to replace log record compression. Row compression now
**	    handles differing length rows more completely and handles replaces
**	    where the old and new rows match exactly.  Changed parameters and
**	    logic to dm0l_repl_comp_info and dm0l_row_unpack routines.  Also
**	    changes to dm0l_rep to support new algorithms.
**	31-jan-1994 (bryanp) B56635
**	    Remove code in dm0l_archive which caused "used before set" warnings.
**	04-15-1994  (chiku)
**	    Bug56702: enable dm0l_xxxx() to return special logfull indication
**	    to its caller.
**	23-may-1994 (mikem)
**	    Bug #63556
**	    Changed interface to dm0l_rep() so that a replace of a record only
**	    need call dm0l_repl_comp_info() this routine once.  Previous to
**	    this change the routine was called once for LGreserve, and once for
**	    dm0l_replace().
**      19-apr-1995 (nick)
**          dm0l_bt() Take a shared lock on the database LK_CKP_TXN resource
**          This is used to allow us to detect deadlock during the database
**          stall phase of an online checkpoint (bug 67888)
**	15-feb-1996 (canor01)
**	    Do not take the shared lock on the database LK_CKP_TXN
**	    resource if online checkpoint has been disallowed.
**	15-feb-96 (emmag)
**	    Fixed typo.
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for table-specific page sizes.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**	    Dynamically allocate record buffers in dm0l_bcp. Change dm0l_ecp
**		to allocate only a DM0L_ECP on the stack.
**      06-mar-96 (stial01)
**          Added page_size to dm0l_relocate
**      01-may-96 (stial01)
**          dm0l_bi() Adjustments for removal of bi_page from DM0L_BI
**          dm0l_ai() Adjustments for removal of ai_page from DM0L_AI
**	23-may-1996 (shero03)
**	    RTree - added dimension, hilbertsize, range to dm0l_index
**      15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: row_version argument to dm0l_put,del,rep.
**	16-jul-1996 (wonst02)
**	    Added dm0l_rtadj() for Rtree.
**	03-sep-1996 (pchang)
**	    Added ext_last_used to DM0L_EXTEND and fmap_last_used to DM0L_FMAP
**	    and their corresponding function prototypes as part of the fix for
**	    B77416.
** 	24-sep-1996 (wonst02)
** 	    Added dm0l_rtdel() and _rtput(). Renamed _rtadj() to _rtrep().
**	12-dec-1996 (shero03)
**	    Fixed length on page image in split log record.
**	13-Dec-1996 (jenjo02)
**	    Removed LK_CKP_TXN lock. Logging will now suspend newly-starting
**	    transactions if online checkpoint is pending and waits until all
**	    current update, protected transactions in the database are complete
**	    before starting the checkpoint.
**	02-Jan-1997 (jenjo02)
**	    In dm0l_bcp(), corrected size of DM0L_BCP CP_TX records being
**	    written to the log. sizeof(struct db) wastes 144 bytes per txn, also
**	    causes LGwrite() to copy that many excess bytes per txn to the
**	    log buffer, a complete waste of time. sizeof(struct tx) is the
**	    correct value.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Removed dm0l_robt() function, incorporating its code into dmxe_begin().
**	03-Mar-1997 (muhpa01)
**		In dm0l_index(), changed to use MEcopy() for assignment of array
**		values from range[] into log.dui_range[] as the array elements are
**		f8 values and range may not be properly aligned resulting in SIGBUS
**		errors (HP-UX port).
** 	10-jun-1997 (wonst02)
** 	    Added DM0L_READONLY_DB for readonly databases.
**	11-Nov-1997 (jenjo02)
**	  o Added DM0L_DELAYBT flag value to be passed to dm0l_bt() when writing a
**	    delayed BT. This flag is passed on to LGwrite() as LG_DELAYBT
**	    not notify LG that we're writing a delayed BT record and are
**	    therefore holding locks and should not be stalled on CKPDB.
**	  o Added LG_NOT_RESERVED flag for certain LGwrite() calls when a single
**	    log record is to be reserved and written in one call (LGwrite()) to LG
**	    instead of two (LGreserve(), LGwrite()).
**      08-Dec-1997 (hanch04)
**          Use new la_block field when block number is needed instead
**          of calculating it from the offset
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**      09-feb-1999 (stial01)
**          dm0l_btsplit() Log kperpage
**      22-Feb-1999 (bonro01)
**          Remove references to svcb_log_id.  Modify dm0l_deallocate()
**	    to zero *lctx instead of dmf_svcb->svcb_lctx_ptr.
**	15-Mar-1999 (jenjo02)
**	    in dm0l_bt(), if LG_CKPDB_STALL status returned from LGreserve(),
**	    acquire LK_S LK_CKP_TXN lock to wait for CKPDB to unstall.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	    Added support for shared log transactions in dm0l_secure().
**      21-dec-1999 (stial01)
**          Added page_type to dm0l_create
**	14-Mar-2000 (jenjo02)
**	    Instead of writing SAVEPOINT and BMINI log records, return
**	    the last written LSN of the transaction to the caller to
**	    be used to demark the log location of the SAVEPOINT or
**	    MiniTransaction. UNDO recovery will process log records
**	    back to, but not including, this LSN.
**	04-May-2000 (jenjo02)
**	    Where find_var_name_len() used to be called, instead
**	    make caller pass in the blank-stripped lengths of 
**	    DB_TAB_NAME, DB_OWNER_NAME instead so we don't have
**	    to constantly recompute it. These lengths are now 
**	    conveniently computed once and stored in the TCB.
**      25-may-2000 (stial01)
**          Added compression type to put/delete/replace log records
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-Oct-2000 (hanal04) Bug 103037 INGSRV 1291
**          Added fmap_hw_mark to dm0l_fmap().
**	30-may-2001 (devjo01)
**	    Replace LGcluster() with CXcluster_enabled(). s103715.
**      18-Mar-2003 (horda03) Bug 109857
**          To prevent a hang between the locking/logging system
**          during the Stall phase of an on-line checkpoint, the
**          session which has just acquired the CKP_TXN lock
**          should release it, before trying to reserve log
**          space. Thus, the situation where a new active tx
**          which is preventing an active tx from completing,
**          won't cause a hang because the CKPDB is waiting for
**          an X lock on CKP_TXN, but this session has acquired
**          the lock, after the DB was stalled, but before the
**          CKPDB requested the CKP_TXN lock.
**      28-mar-2003 (stial01)
**          Child on DMPP_INDEX page may be > 512, so it must be logged
**          separately in DM0L_BTPUT, DM0L_BTDEL records (b109936)
**	6-Feb-2004 (schka24)
**	    Add name-generator ID to index and modify records.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	1-Apr-2004 (schka24)
**	    Invent "raw data" for repartitioning modify logging.
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          On some platforms char is a signed commodity, this causes
**          problems when copying a char value >127 to an integer, the
**          sign will be maintained. The Attribute filed of DM0L_INDEX
**          and DM0L_MODIFY has been changed from char to DM_ATT.
**	29-Apr-2004 (gorvi01)
**		Added function dm0l_del_location. This function
**		writes a delete location record.
**	14-May-2004 (hanje04)
**	     Remove raw_start, raw_blocks & raw_total_blocks from 
**	     dm0l_del_location to correct bad X-integration from from main.
**      01-sep-2004 (stial01)
**          Support cluster online checkpoint
**      07-dec-2004 (stial01)
**          LK_CKP_TXN: key change to include db_id
**          LK_CKP_TXN: key change remove node id, LKrequest LK_LOCAL instead
**      30-dec-2005 (stial01 for huazh01)
**          Align buffer passed to LKshow
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now have 2 BT records (one journaled and one not)
**          and an ET record. Only need to reserve space for the records
**          when the first BT is required.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**      11-may-2007 (stial01)
**          dm0l_secure() Remove LGalter(LG_A_PREPARE), caller should prepare.
**	07-Feb-2008 (hanje04)
**	    SIR 119978
**	    Defining true and false locally causes compiler errors where 
**	    they are already defined. Change the variable names to tru and
**	    fals to avoid this
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**      27-jun-2008 (stial01)
**          Changes to dm0l_opendb for incremental rollforwarddb
**	17-Nov-2008 (jonj)
**	    Macroized log_error to pass source information to logErrorFcn(),
**	    use new form uleFormat throughout.
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**	18-Nov-2008 (jonj)
**	    dm0c_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      23-Feb-2009 (hanal04) Bug 121652
**          Added dm0l_ufmap() which will be used alongside dm0l_extend()
**          and dm0l_fmap() during an extend.
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	29-Oct-2009 (kschendel) SIR 122739
**	    Kill ixklen, replace with btflags for BT put, del, free records.
**      30-Oct-2009 (horda03) Bug 122823
**          dm0l_allocate convert DM0L_PARTITION_FILENAMES to
**          LG_PARTITION_FILENAMES when opening TX log file.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: replace LG_LSN i/o parameter with LG_LRI,
**	    initialize new DM0L_CRHEADER in appropriate log records.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
*/

/*
**  Definition of static variables and forward static functions.
*/

static DB_STATUS	logErrorFcn(
				DB_STATUS	err_code,
				CL_ERR_DESC	*syserr,
				i4		p,
				i4		ret_err_code,
				DB_ERROR	*ret_dberr,
				DB_STATUS	stat,
				PTR		FileName,
				i4		LineNumber);
#define log_error(err_code,syserr,p,ret_err_code,ret_dberr,stat) \
        logErrorFcn(err_code,syserr,p,ret_err_code,ret_dberr,stat,\
			__FILE__, __LINE__)

static  DB_STATUS 	get_dbname(
				DB_DB_NAME *db_name_ptr);


/*{
** Name: dm0l_bt	- Begin Transaction.
**
** Description:
**      This routine writes a begin transaction log record to the log file.
**      It returns the transaction identifier assigned to the transaction
**	and a logging system identifier for future dm0l_* calls.
**
** Inputs:
**	db_id				Logging system database identifier.
**	flag				Flags: DM0L_JOURNAL.
**	llid				lock list id.
**	dcb				dcb pointer.
**      user_name                       Pointer to the username.
**
** Outputs:
**      tran_id                         The assigned transaction identifier.
**      lg_id                           The logging system identifier.
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
**	17-may-1986 (Derek)
**          Created for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added LG_FIRST flag to the LGwrite call in dm0l_bt.  This is to
**	    allow LG to make sure that all transactions begin with a Begin
**	    Transaction record.
**	17-mar-1989 (EdHsu)
**	    Support cluster online backup.
**	15-may-1989 (rogerk)
**	    Return LOCK_QUOTA_EXCEEDED if LK_NOLOCKS returned.
**	17-jan-1990 (rogerk)
**	    Don't call LGend if get an error writing the BT record.  Leave
**	    this up to the caller.  Calling LGend causes us to not be able
**	    to execute an abort - even though this routine may be called
**	    in the middle of a transaction rather than in dmx_begin.
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	12-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	15-mar-1993 (rogerk)
**	    Reduced Logging (phase IV):
**	    Changed LGwrite to return an LSN instead of a Log Address.
**	    Changed to show database with LG_S_DATABASE.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (bryanp)
**	    Clusters, too, should reserve logfile space for BT/ET pairs.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**      24-jan-1995 (stial01)
**          BUG 66473: dm0l_sbackup() flag parm DM0L_TBL_CKPT if table ckpt.
**      19-apr-1995 (nick)
**          Take a shared lock on the database LK_CKP_TXN resource - this
**          is used to allow us to detect deadlock during the database stall
**          phase of an online checkpoint.
**	15-feb-1996 (canor01)
**	    Do not take the shared lock on the database LK_CKP_TXN
**	    resource if online checkpoint has been disallowed.
**	13-Dec-1996 (jenjo02)
**	    Removed LK_CKP_TXN lock. Logging will now suspend newly-starting
**	    transactions if online checkpoint is pending and waits until all
**	    current protected transactions in the database are complete
**	    before starting the checkpoint.
**	11-Nov-1997 (jenjo02)
**	    Added DM0L_DELAYBT flag value to be passed to dm0l_bt() when writing a
**	    delayed BT. This flag is passed on to LGwrite() as LG_DELAYBT
**	    not notify LG that we're writing a delayed BT record and are
**	    therefore holding locks and should not be stalled on CKPDB.
**	15-Mar-1999 (jenjo02)
**	    in dm0l_bt(), if LG_CKPDB_STALL status returned from LGreserve(),
**	    acquire LK_S LK_CKP_TXN lock to wait for CKPDB to unstall.
**	14-Mar-2000 (jenjo02)
**	    Added *lsn parm to prototype in which LSN of BT record is
**	    returned to caller.
**      07-Apr-2003 (horda03) Bug 109857
**          If there is a deadlock between the logging and locking
**          system, report the deadlock to the calling routine.
**	05-Sep-2006 (jonj)
**	    DM0L_ILOG input flag allows internal logging in an otherwise
**	    read-only transaction.
*/
DB_STATUS
dm0l_bt(
    i4		    lg_id,
    i4		    flag,
    i4		    llid,
    DMP_DCB	    *dcb,
    DB_OWN_NAME	    *user_name,
    LG_LSN	    *lsn,
    DB_ERROR	    *dberr)
{
    DM0L_BT	    log;
    DB_STATUS	    ret_status;
    STATUS	    status;
    CL_ERR_DESC	    error;
    LG_OBJECT	    lg_object;
    LG_DATABASE	    database;
    LG_LRI	    lri;
    LK_LOCK_KEY	    db_ckey;
    LK_LKID	    dbclkid;
    i4	    	    length;
    i4	    	    loc_error;
    i4		    lg_flags;
    i4              no_bt_records;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (flag != DM0L_SPECIAL)
    {
	if (CXcluster_enabled() &&
            !(flag & DM0L_2ND_BT))
	{
            /* If this is a 2nd BT record, then we've already started the TX. */
	    database.db_id = (LG_DBID)dcb->dcb_log_id;
	    length = 0;

	    status = LGshow(LG_S_DATABASE, (PTR)&database, sizeof(database),
		&length, &error);
	    if (status != E_DB_OK || length == 0)
	    {
		ret_status = log_error(E_DM9017_BAD_LOG_SHOW, &error, lg_id,
		    E_DM9211_DM0L_BT, dberr, status);
		return (ret_status);
	    }

	    if ((database.db_status & DB_BACKUP) == 0)
	    {
		MEcopy((PTR)&dcb->dcb_db_owner, 8, (PTR)&db_ckey.lk_key1);
		MEcopy((PTR)&dcb->dcb_name, 16, (PTR)&db_ckey.lk_key3);
		db_ckey.lk_type = LK_CKP_CLUSTER;
		MEfill(sizeof(LK_LKID), 0, &dbclkid);
		status = LKrequest(LK_PHYSICAL, llid,
		    &db_ckey, LK_S, (LK_VALUE * )0, &dbclkid, 0L, &error);
		if (status != E_DB_OK)
		{
		    ret_status = log_error(E_DM901C_BAD_LOCK_REQUEST, &error,
			lg_id, E_DM9211_DM0L_BT, dberr, status);
		    if (status == LK_NOLOCKS)
			SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		    return (ret_status);
		}
	    }
	}
    }

    /*
    ** Reserve log file space for the BT & ET logs.
    */
    do
    {
        /* If this is the 2nd BT record, then the TX is already
        ** active so don't stall nor reserve log space.
        */
        if (flag & DM0L_2ND_BT)
        {
           status = OK;

           break;
        }

        /* If this is a journaled BT record, then we don't need to reserve
        ** space for the non-journaled BT.
        */
        no_bt_records = (flag & DM0L_JOURNAL) ? 1 : 2;

	/*
	** If the database is stalled waiting for CKPDB, the LGreserve()
	** will be rejected with a status of LG_CKPDB_STALL.
	**
	** Acquire a LK_S lock on the database's LK_CKP_TXN lock, which
	** will be blocked by CKPDB's LK_X lock; when CKPDB is ready to
	** unstall the database, it'll release the LK_X lock, we'll
	** proceed, and retry the LGreserve() operation.
	*/
        status = LGreserve(0, lg_id, 2,
                           (no_bt_records * sizeof(DM0L_BT)) + sizeof(DM0L_ET), &error);

	if (status == LG_CKPDB_STALL)
	{
	    STATUS	cl_status;

	    /*
	    ** Prepare the lock key as:
	    **
	    **	type:   LK_CKP_TXN
	    **  key1-5: 1st 20 bytes of database name
	    **  key6: dbid
	    **
	    **	(see also conforming code in lgalter, dmfcpp, lkshow, dmfcsp)
	    */
	    db_ckey.lk_type = LK_CKP_TXN;
	    MEcopy((char *)&dcb->dcb_name, 20, (char *)&db_ckey.lk_key1);
	    db_ckey.lk_key6 = dcb->dcb_id;
	    MEfill(sizeof(LK_LKID), 0, &dbclkid);

	    cl_status = LKrequest(LK_LOCAL, llid, &db_ckey,
				LK_S, (LK_VALUE *)0, 
				&dbclkid, 0L, &error);
	    if (cl_status == OK)
            {
               cl_status = LKrelease(LK_PARTIAL, llid, &dbclkid, &db_ckey, (LK_VALUE *)0, &error);
            }
            
            if (cl_status != OK)
		status = cl_status;
	}
    } while (status == LG_CKPDB_STALL);

    if (status != OK)
    {
	(VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		&loc_error, 1, 0, lg_id);

        if (status == LK_DEADLOCK)
        {
           /* Pass deadlock error to the calling routine */

           return( log_error(E_DM9211_DM0L_BT, &error, lg_id,
                             E_DM0042_DEADLOCK, dberr, status));
        }

	return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	    E_DM9211_DM0L_BT, dberr, status));
    }

    lg_flags = LG_FIRST;

    /* Tell LG if internal logging to override read-only check */
    if ( flag & DM0L_ILOG )
	lg_flags |= LG_ILOG;

    /* Tell LG if this is a second BT for this txn */
    if ( flag & DM0L_2ND_BT )
	lg_flags |= LG_2ND_BT;

    /*	Initialize the log record. */

    log.bt_header.length = sizeof(DM0L_BT);
    log.bt_header.type = DM0LBT;
    log.bt_header.flags = flag;
    log.bt_name = *user_name;
    TMget_stamp((TM_STAMP *)&log.bt_time);

    /*	Write the begin transaction record to the log. */

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(lg_flags, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.bt_header, sizeof(DM0L_ET));

	return (E_DB_OK);
    }

    /*  Log error and return. */
    ret_status = log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	    E_DM9211_DM0L_BT, dberr, status);
    return(ret_status);
}

/*{
** Name: DM0L_ET	- End Transaction.
**
** Description:
**      Write an end transaction log record.  The record is
**	automatically forced to disk.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL, DM0L_ROTRAN
**					    DM0L_MASTER, 
**					    DM0L_LOGFULL_COMMIT
**      state                           Either DM0L_ABORT, DM0L_COMMIT,
**					DM0L_DBINCONST.
**	tran_id				Transaction id of the aborted
**					transaction if the transaction is
**					aborted by the recovery process.
**	db_id				DB id of the transaction if the
**					transaction is aborted by the recovery
**					process.
**
** Outputs:
**      ctime                           Commit time.
**      err_code                        Reason for error return.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	30-jan-1990 (ac)
**	    Get the commit timestamp for the read only transaction.
**	15-mar-1993 (rogerk)
**	    Reduced Logging (phase IV):
**	    Changed LGwrite to return an LSN instead of a Log Address.
**	    Removed et_tran_id and et_db_id fields as these are now correctly
**	    written in the log record header by LGwrite.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-Apr-2004 (jenjo02)
**	    Add support for on_logfull COMMIT.
**	22-Jul-2004 (schka24)
**	    To avoid confusing people (and maybe even causing support
**	    calls!), don't say we wrote an ET if we didn't (disconnect
**	    from shared txn).
*/
DB_STATUS
dm0l_et(
    i4             lg_id,
    i4		flag,
    i4             state,
    DB_TRAN_ID		*tran_id,
    i4		db_id,
    DB_TAB_TIMESTAMP	*ctime,
    DB_ERROR		*dberr)
{
    DM0L_ET	    log;
    LG_LRI	    lri;
    LG_OBJECT	    lg_object;
    i4		    action;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    /*
    ** Don't write the dm0l_et log record for read only transaction.
    */

    if ( (flag & DM0L_ROTRAN) == 0 )
    {
	log.et_header.length = sizeof(DM0L_ET);
	log.et_header.type = DM0LET;
	log.et_header.flags = flag;
	TMget_stamp((TM_STAMP *) &log.et_time);
	log.et_flag = state;
	log.et_master = 0;

	/*
	** Identify the ET type if this transaction is being aborted as
	** part of crash or restart recovery.
	*/
	if (flag & DM0L_MASTER)
	    log.et_master = DM0L_AUTO;

	/*
	** Set special LGwrite flags for the End Transaction request.
	**
	** An LG_FORCE is done to write out the ET record and insure that
	** the transaction completion is committed.
	**
	** If the transaction end is a server request, then the LG_LAST flag
	** is passed to implicitly cause an LGend to be done after writing
	** the ET record.  This is done as a performance optimization. (If the
	** ET record is written by a recovery process, the LGend will be done
	** explicitly by the caller.
	*/
	if (flag & DM0L_MASTER)
	    action = LG_FORCE;
	else
	    action = LG_FORCE | LG_LAST;

	if ( flag & DM0L_LOGFULL_COMMIT )
	    action |= LG_LOGFULL_COMMIT;


	lg_object.lgo_size = sizeof(log);
	lg_object.lgo_data = (PTR) &log;

	status = LGwrite(action, lg_id, (i4)1, &lg_object, &lri, &error);
	if (status != OK)
	{
	    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
		    E_DM9213_DM0L_ET, dberr, status));
	}

	/* LGwrite returns a zero lsn if we just disconnected */
	if (DMZ_SES_MACRO(11)
	  && (lri.lri_lsn.lsn_high != 0 || lri.lri_lsn.lsn_low != 0) )
	    dmd_logtrace(&log.et_header, 0);

	/*
	** Use the LSN of the End Transaction record to generate the
	** Commit Timestamp.
	*/
	ctime->db_tab_high_time = lri.lri_lsn.lsn_high;
	ctime->db_tab_low_time =  lri.lri_lsn.lsn_low;

    }
    else
    {
	i4		length;

	/* Only LGend for the read-only transaction. */

	status = LGshow(LG_S_STAMP, (PTR)ctime, sizeof(*ctime), &length,
			&error);
	if (status)
	    _VOID_ log_error(E_DM9017_BAD_LOG_SHOW, &error, LG_S_STAMP,
	    		E_DM9213_DM0L_ET, dberr, status);

	status = LGend(lg_id, 0, &error);
	if (status != OK)
	    return (log_error(E_DM900E_BAD_LOG_END, &error, lg_id,
		    E_DM9213_DM0L_ET, dberr, status));
    }
    return (E_DB_OK);

}

/*{
** Name: DM0L_BI	- Write Before Image log record.
**
** Description:
**      This routine write a before image log record to the log.
**
** Inputs:
**      tbl_id                          Table Identifier.
**      page                            Page.
**	page_size			Page size.
**
** Outputs:
**      la                              Log address.
**      err_code                        Reason for error status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	15-feb-1990 (mikem)
**	    Fixed calculation of the size of a subset of members in a structure.
**	    See comments at top of file for detailed explanation.
**      05-mar-1991 (mikem)
**          Disable before image logging based on DM901, to be used for
**          6.5 performance testing while compressed logging is not available.
**	    Recovery in a system with before image logging disabled will not
**	    work correctly.
**	14-dec-1992 (rogerk)
**	    Rewritten for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**      01-may-96 (stial01)
**          dm0l_bi() Adjustments for removal of bi_page from DM0L_BI
**          (Before image page immediately follows the DM0L_BI)
**	10-jan-1997 (nanpr01)
**	    Added back the bi_page to shield against alignment problem.
*/
DB_STATUS
dm0l_bi(
i4         log_id,
i4         flag,
DB_TAB_ID       *tbl_id,
DB_TAB_NAME     *name,
DB_OWN_NAME	*owner,
i4         pg_type,
i4         loc_cnt,
i4         loc_config_id,
i4         operation,
i4		page_number,
i4		page_size,
DMPP_PAGE       *page,
LG_LSN		*prev_lsn,
LG_LSN		*lsn,
DB_ERROR	*dberr)
{
    DM0L_BI	    log;
    LG_OBJECT	    lg_object[2];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    LG_LRI	    lri;

    CLRDBERR(dberr);

    log.bi_header.length = sizeof(DM0L_BI) - sizeof(DMPP_PAGE) + page_size;
    log.bi_header.type = DM0LBI;
    log.bi_header.flags = flag;
    if (flag & DM0L_CLR)
	log.bi_header.compensated_lsn = *prev_lsn;

    log.bi_tbl_id = *tbl_id;
    log.bi_pg_type = pg_type;
    log.bi_loc_cnt = loc_cnt;
    log.bi_loc_id = loc_config_id;
    log.bi_operation = operation;
    log.bi_pageno = page_number;
    log.bi_page_size = page_size;

    lg_object[0].lgo_size = ((char *)&log.bi_page) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = page_size;
    lg_object[1].lgo_data = (PTR) page;

    status = LGwrite(0, log_id, (i4)2, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.bi_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, log_id,
	E_DM9214_DM0L_BI, dberr, status));
}

/*{
** Name: DM0L_AI	- Write After Image log record.
**
** Description:
**      This routine write an after image log record to the log.
**
** Inputs:
**      tbl_id                          Table Identifier.
**	page_size			Page Size.
**      page                            Page.
**
** Outputs:
**      la                              Log address.
**      err_code                        Reason for error status.
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
**	14-dec-1992 (rogerk)
**	    Written for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**      01-may-96 (stial01)
**          dm0l_ai() Adjustments for removal of ai_page from DM0L_AI
**          (After image page immediately follows the DM0L_AI)
**	10-jan-1997 (nanpr01)
**	    Added back the ai_page to shield against alignment problem.
*/
DB_STATUS
dm0l_ai(
i4         log_id,
i4         flag,
DB_TAB_ID       *tbl_id,
DB_TAB_NAME     *name,
DB_OWN_NAME	*owner,
i4         pg_type,
i4         loc_cnt,
i4         loc_config_id,
i4         operation,
i4		page_number,
i4		page_size,
DMPP_PAGE       *page,
LG_LSN		*prev_lsn,
LG_LSN		*lsn,
DB_ERROR	*dberr)
{
    DM0L_AI	    log;
    LG_OBJECT	    lg_object[2];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    LG_LRI	    lri;

    CLRDBERR(dberr);

    log.ai_header.length = sizeof(DM0L_AI) - sizeof(DMPP_PAGE) + page_size;
    log.ai_header.type = DM0LAI;
    log.ai_header.flags = flag;
    if (flag & DM0L_CLR)
	log.ai_header.compensated_lsn = *prev_lsn;

    log.ai_tbl_id = *tbl_id;
    log.ai_pg_type = pg_type;
    log.ai_loc_cnt = loc_cnt;
    log.ai_loc_id = loc_config_id;
    log.ai_operation = operation;
    log.ai_pageno = page_number;
    log.ai_page_size = page_size;

    lg_object[0].lgo_size = ((char *)&log.ai_page) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = page_size;
    lg_object[1].lgo_data = (PTR) page;

    status = LGwrite(0, log_id, (i4)2, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.ai_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, log_id,
	E_DM9C0B_DM0L_AI, dberr, status));
}

/*{
** Name: dm0l_put	- Write Put Record log record.
**
** Description:
**      Write a put record log record to the log file.
**
** Inputs:
**	log_id				Handle used to write to LG.
**	flag				Flags: DM0L_JOURNAL.
**      tbl_id                          Table Identifier.
**      name                            Table name.
**	name_len			Blank-stripped length of table name
**	owner				Table owner.
**	owner_len			Blank-stripped length of owner name
**      tid                             Tuple Identifier.
**	page_type			Page Format Identifier.
**	page_size			Page size.
**	location_id			Physical Location Identifier.
**	location_count			Table's Location Count.
**      size                            Record size;
**      record                          The record.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**      lsn                             LSN of the logged record.
**      err_code                        Reason for error status return.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	15-feb-1990 (mikem)
**	    Fixed calculation of the size of a subset of members in a structure.
**	    See comments at top of file for detailed explanation.
**	26-oct-1992 (jnash)
**	    Reduced Logging Project:
**	     - Eliminated RCB argument. The log_id is now passed directly.
**	     - Changed references to old page and record log addresses to
**	       use Log Sequence Numbers.
**	     - Changed table name and owner values to be logged compressed.
**	     - Added pg_type, cnf_loc_id, loc_cnt log record fields.
**	15-jan-1993 (jnash)
** 	    Handle zero length rows written by CLRs.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**      15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: row_version argument to dm0l_put,del,rep.
*/
DB_STATUS
dm0l_put(
i4		log_id,
i4		flag,
DB_TAB_ID	*tbl_id,
DB_TAB_NAME     *name,
i2		name_len,
DB_OWN_NAME	*owner,
i2		owner_len,
DM_TID		*tid,
i4		pg_type,
i4		page_size,
i4              comp_type,
i4		loc_config_id,
i4		loc_cnt,
i4		size,
PTR		record,
LG_LSN		*prev_lsn,
i4		row_version,
DMPP_SEG_HDR	*seg_hdr,
LG_LRI		*lri,
DB_ERROR	*dberr)
{
    DM0L_PUT	    log;
    LG_OBJECT	    lg_object[4];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4	    	    num_objects;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length record */
    log.put_header.length = sizeof(DM0L_PUT) + size;

    log.put_header.type   = DM0LPUT;
    log.put_header.flags  = flag;
    if (flag & DM0L_CLR)
	log.put_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lri, &log.put_crhdr);

    log.put_pg_type 	  = pg_type;
    log.put_page_size	  = page_size;
    log.put_comptype      = comp_type;
    log.put_cnf_loc_id	  = loc_config_id;
    log.put_loc_cnt 	  = loc_cnt;
    log.put_rec_size 	  = size;
    log.put_tid		  = *tid;
    log.put_tbl_id	  = *tbl_id;
    log.put_row_version	  = row_version;
    if (seg_hdr)
	MEcopy(seg_hdr, sizeof(DMPP_SEG_HDR), &log.put_seg_hdr);
    else
	MEfill(sizeof(DMPP_SEG_HDR), '\0', &log.put_seg_hdr);

    lg_object[0].lgo_size = sizeof(log);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = size;
    lg_object[1].lgo_data = record;

    /*
    ** PUT record CLRs are recognized by a zero length row.
    */
    num_objects = size ? 2 : 1;
    status = LGwrite(0, log_id, num_objects, lg_object, lri, &error);

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    /*
	    ** The CLR for a PUT doen't include space for the row, just the tid.
	    */
	    dmd_logtrace(&log.put_header, (flag & DM0L_CLR) ? 0 :
		sizeof(DM0L_PUT));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, log_id,
	E_DM9215_DM0L_PUT, dberr, status));
}

/*
** Name: dm0l_nofull	- Write No Fullchain log record.
**
** Description:
** 	Write a "no fullchain" log record to the log file, used in recovery
**	of the hash table fullchain bit.  This log is used only for
**	recovery of resetting the bit, setting the bit is encoded in the
**	dm0l_ovfl log.
**
** Inputs:
**	log_id				Handle used to write to LG.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id 				Table Identifier.
**	name				Table name.
**	name_len			Blank-stripped length of table name
**	owner				Table owner.
**	owner_len			Blank-stripped length of owner name
**	tid 				Tuple Identifier.
**	page_type			Page Format Identifier.
**	page_size			Page size.
**	location_id			Physical Location Identifier.
**	location_count			Table's Location Count.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**      lsn                             LSN of the logged record.
**      err_code                        Reason for error status return.
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
**	16-oct-1992 (jnash)
**	    Created for 6.5 Recovery.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (bryanp)
**	    Added multiple page size support.
*/
DB_STATUS
dm0l_nofull(
i4		log_id,
i4		flag,
DB_TAB_ID	*tbl_id,
DB_TAB_NAME	*name,
i2		name_len,
DB_OWN_NAME	*owner,
i2		owner_len,
i4		pg_type,
i4		page_size,
DM_PAGENO	pageno,
i4		loc_config_id,
i4		loc_cnt,
LG_LSN		*prev_lsn,
LG_LRI		*lri,
DB_ERROR	*dberr)
{
    DM0L_NOFULL	    log;
    LG_OBJECT	    lg_object[3];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.nofull_header.length = sizeof(DM0L_NOFULL);

    log.nofull_header.type = DM0LNOFULL;
    log.nofull_header.flags = flag;
    if (flag & DM0L_CLR)
	log.nofull_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lri, &log.nofull_crhdr);

    log.nofull_pg_type 	  = pg_type;
    log.nofull_page_size  = page_size;
    log.nofull_loc_cnt 	  = loc_cnt;
    log.nofull_cnf_loc_id  = loc_config_id;
    log.nofull_pageno 	  = pageno;
    log.nofull_tbl_id	  = *tbl_id;

    lg_object[0].lgo_size = sizeof(log);
    lg_object[0].lgo_data = (PTR) &log;

    status = LGwrite(0, log_id, (i4)1, lg_object, lri, &error);

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.nofull_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, log_id,
	E_DM9C15_DM0L_NOFULL, dberr, status));
}

/*{
** Name: dm0l_del	- Write Delete Record log record.
**
** Description:
**      Write a delete record log record to the log file.
**
** Inputs:
**	log_id				Handle used to write to LG.
**	flag				Flags: DM0L_JOURNAL.
**      tbl_id                          Table Identifier.
**      name                            Table name.
**	name_len			Blank-stripped length of table name
**	owner				Table owner.
**	owner_len			Blank-stripped length of owner name
**      tid                             Tuple Identifier.
**	page_type			Page Format Identifier.
**	page_size			Page size.
**	location_id			Physical Location Identifier.
**	location_count			Table's Location Count.
**      size                            Record size;
**      record                          The record.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**      lsn                             LSN of the logged record.
**      err_code                        Reason for error status return.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	15-feb-1990 (mikem)
**	    Fixed calculation of the size of a subset of members in a structure.
**	    See comments at top of file for detailed explanation.
**	26-oct-1992 (jnash & rogerk)
**	    Reduced Logging Project:
**	     - Eliminated RCB argument. The log_id is now passed directly.
**	     - Changed references to old page and record log addresses to
**	       use Log Sequence Numbers.
**	     - Changed table name and owner values to be logged compressed.
**	     - Added pg_type, cnf_loc_id, loc_cnt log record fields.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**      15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: row_version argument to dm0l_put,del,rep.
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: add row_tran_id, row_lg_id to prototype,
**	    del_otran_id, del_olg_id to log record.
*/
DB_STATUS
dm0l_del(
i4		log_id,
i4		flag,
DB_TAB_ID	*tbl_id,
DB_TAB_NAME     *name,
i2		name_len,
DB_OWN_NAME	*owner,
i2		owner_len,
DM_TID		*tid,
i4		pg_type,
i4		page_size,
i4              comp_type,
i4		loc_config_id,
i4		loc_cnt,
i4		size,
PTR		record,
LG_LSN		*prev_lsn,
i4		row_version,
DMPP_SEG_HDR	*seg_hdr,
u_i4		row_tran_id,
u_i2		row_lg_id,
LG_LRI		*lri,
DB_ERROR	*dberr)
{
    DM0L_DEL	    log;
    LG_OBJECT	    lg_object[4];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length record */
    log.del_header.length = sizeof(DM0L_DEL) + size;

    log.del_header.type   = DM0LDEL;
    log.del_header.flags  = flag;
    if (flag & DM0L_CLR)
	log.del_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lri, &log.del_crhdr);

    log.del_otran_id	  = row_tran_id;
    log.del_olg_id	  = row_lg_id;
    log.del_pg_type 	  = pg_type;
    log.del_page_size	  = page_size;
    log.del_comptype      = comp_type;
    log.del_cnf_loc_id	  = loc_config_id;
    log.del_loc_cnt 	  = loc_cnt;
    log.del_rec_size 	  = size;
    log.del_tbl_id	  = *tbl_id;
    log.del_tid	  	  = *tid;
    log.del_row_version	  = row_version;
    if (seg_hdr)
	MEcopy(seg_hdr, sizeof(DMPP_SEG_HDR), &log.del_seg_hdr);
    else
	MEfill(sizeof(DMPP_SEG_HDR), '\0', &log.del_seg_hdr);

    lg_object[0].lgo_size = sizeof(log);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = size;
    lg_object[1].lgo_data = record;

    status = LGwrite(0, log_id, (i4)2, lg_object, lri, &error);

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.del_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_DEL) + size);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, log_id,
	E_DM9216_DM0L_DEL, dberr, status));
}

/*{
** Name: dm0l_rep	- Write Replace Record log record.
**
** Description:
**      Write a replace record log record to the log file.
**
** Inputs:
**	log_id				Handle used to write to LG.
**	flag				Flags: DM0L_JOURNAL.
**      tbl_id                          Table Identifier.
**      name                            Table name.
**	name_len			Blank-stripped length of table name
**	owner				Table owner.
**	owner_len			Blank-stripped length of owner name
**      otid                            Old tuple identifier.
**	ntid				New tuple identifier.
**	page_type			Page Format Identifier.
**	page_size			Page size.
**	olocation_id			Physical Location Identifier of old page
**	nlocation_id			Physical Location Identifier of new page
**	location_count			Table's Location Count.
**      l_orecord				Old Record's size.
**      l_nrecord				New Record's size.
**      orecord                         Old record.
**	nrecord				New record.
**	prev_lsn			LSN of compensated log record if CLR.
**	# the following 6 pieces of data expected to come from a call to
**	# dm0l_repl_comp_info() made by caller.
**      comp_odata                      Ptr into old row to use for building
**                                      before image in replace log record.
**      comp_odata_len                  Bytes to log for before image.
**      comp_ndata                      Ptr into new row to use for building
**                                      after image in replace log record.
**      comp_ndata_len                  Bytes to log for after image.
**      diff_offset                     Offset which gives the first byte in
**                                      which the old and new rows differ.
**      comp_orow                       Describes whether before image
**                                      compression was used.
**
** Outputs:
**      lsn                             LSN of the logged record.
**      err_code                        Reason for error status return.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	15-feb-1990 (mikem)
**	    Fixed calculation of the size of a subset of members in a structure.
**	    See comments at top of file for detailed explanation.
**	26-oct-1992 (jnash & rogerk)
**	    Reduced Logging Project:
**	     - Eliminated RCB argument. The log_id is now passed directly.
**	     - Changed references to old page and record log addresses to
**	       use Log Sequence Numbers.
**	     - Changed table name and owner values to be logged compressed.
**	     - Added pg_type, cnf_loc_id, loc_cnt log record fields.
**	15-jan-1993 (jnash)
**	    Add support for zero length "new" rows written by CLRs.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
** 	18-oct-1993 (jnash)
**	    Support row compression in replace log records.  Compressed data
**	    as defined here means only that which has changed as a result
**	    of the update.  We always compress "new row" information, while
**	    "old row" information is conditionally compressed via trace point.
**	30-oct-1993 (jnash)
**	    Fix incorrect PTR arithmetic.
**	11-dec-1993 (rogerk)
**	    Changes to replace log record compression.  Added new replace log
**	    record fields: rep_odata_len, rep_ndata_len.  Moved logic to decide
**	    what type of compression to use down into the dm0l_repl_comp_info
**	    routine.  Added new consistency check.
**      23-may-1994 (mikem)
**          Bug #63556
**          Changed interface to dm0l_rep() so that a replace of a record only
**          need call dm0l_repl_comp_info() this routine once.  Previous to
**          this change the routine was called once for LGreserve, and once for
**	    dm0l_replace().  Added parameters (comp_odata, comp_odata_len,
**	    comp_ndata, comp_ndata_len, diff_offset, and comp_orow) to
**	    dm0l_rep() which are expected to be obtained by the client by
**	    calling the new dm0l_repl_comp_info().
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**      15-jul-1996 (ramra01 for bryanp)
**          Alter Table support: row_version argument to dm0l_put,del,rep.
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: add row_tran_id, row_lg_id to prototype,
**	    rep_otran_id, rep_olg_id to log record.
*/
DB_STATUS
dm0l_rep(
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
i4		l_orecord,
i4		l_nrecord,
PTR		orecord,
PTR		nrecord,
LG_LSN		*prev_lsn,
char	    	*comp_odata,
i4	    	comp_odata_len,
char	    	*comp_ndata,
i4	    	comp_ndata_len,
i4	    	diff_offset,
bool   	    	comp_orow,
i4		orow_version,
i4		nrow_version,
u_i4		row_tran_id,
u_i2		row_lg_id,
LG_LRI		*lri,
DB_ERROR	*dberr)
{
    DM0L_REP	    log;
    LG_OBJECT	    lg_object[5];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4	    	    obj_num;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Consistency check: make sure row compression produced sensible results.
    */
    if ((comp_odata_len < 0) || (comp_odata_len > l_orecord) ||
	(comp_ndata_len < 0) || (comp_ndata_len > l_nrecord) ||
	(diff_offset < 0))
    {
	uleFormat(NULL, E_DM9442_REPLACE_COMPRESS, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
	    0, comp_odata_len, 0, comp_ndata_len, 0, diff_offset,
	    0, l_orecord, 0, l_nrecord, 0, comp_orow);
	SETDBERR(dberr, 0, E_DM9217_DM0L_REP);
	return (E_DB_ERROR);
    }

    /*
    ** Set comp_repl_orow flag based on decision made by repl_comp_info call
    ** above.  Be careful of left-over setting of this bit which may occur
    ** in CLR records.
    */
    flag &= ~DM0L_COMP_REPL_OROW;
    if (comp_orow)
	flag |= DM0L_COMP_REPL_OROW;

    /*
    ** The length of the record reflects variable length records,
    ** and compressed rows.
    */
    log.rep_header.length = sizeof(DM0L_REP) + comp_odata_len + comp_ndata_len;
    log.rep_header.type    = DM0LREP;
    log.rep_header.flags   = flag;
    if (flag & DM0L_CLR)
	log.rep_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lri, &log.rep_crhdr);


    log.rep_otran_id	   = row_tran_id;
    log.rep_olg_id	   = row_lg_id;
    log.rep_pg_type	   = pg_type;
    log.rep_page_size	   = page_size;
    log.rep_comptype       = comp_type;
    log.rep_ocnf_loc_id	   = oloc_config_id;
    log.rep_ncnf_loc_id	   = nloc_config_id;
    log.rep_loc_cnt	   = loc_cnt;
    log.rep_orec_size	   = l_orecord;
    log.rep_nrec_size	   = l_nrecord;
    log.rep_odata_len      = comp_odata_len;
    log.rep_ndata_len      = comp_ndata_len;
    log.rep_diff_offset    = diff_offset;
    log.rep_tbl_id	   = *tbl_id;
    log.rep_otid	   = *otid;
    log.rep_ntid	   = *ntid;
    log.rep_orow_version   = orow_version;
    log.rep_nrow_version   = nrow_version;

    lg_object[0].lgo_size = sizeof(log);
    lg_object[0].lgo_data = (PTR) &log;

    obj_num = 1;
    if (comp_odata_len)
    {
	lg_object[obj_num].lgo_size = comp_odata_len;
	lg_object[obj_num].lgo_data = (PTR) comp_odata;
        obj_num++;
    }

    if (comp_ndata_len)
    {
	lg_object[obj_num].lgo_size = comp_ndata_len;
	lg_object[obj_num].lgo_data = (PTR) comp_ndata;
        obj_num++;
    }

    status = LGwrite(0, log_id, obj_num, lg_object, lri, &error);

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    /*
	    ** The CLR for a replace does not include space for the new row,
	    ** just the tid.
	    */
	    dmd_logtrace(&log.rep_header, (flag & DM0L_CLR) ? 0 :
		(sizeof(DM0L_REP) + l_orecord));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, log_id,
	E_DM9217_DM0L_REP, dberr, status));
}

/*{
** Name: dm0l_bm	- Establish Begin Mini-Transaction LSN.
**
** Description:
**	This routine returns the last written LSN of the transaction,
**	which establishes the location in the log of the beginning
** 	of a mini-transaction.
**
** Inputs:
**	rcb				Pointer to RCB.
**
** Outputs:
**	lsn				Transactions's last written LSN
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	12-jan-1992 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	15-mar-1993 (rogerk)
**	    Reduced Logging (phase IV):
**	    Made BMini log records journaled if the database is journaled.
**	    Changed LGwrite to return an LSN instead of a Log Address.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Set RCB_IN_MINI_TRAN flag in the RCB to track that this RCB
**		    is inside a mini-transaction. Currently, this is used only
**		    by the buffer manager in enforcing cluster node failure
**		    recovery protocols. Arguably, the RCB is not the corret
**		    place for this flag, but it happens to be the control block
**		    which is available, and it works for our purposes.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	14-Mar-2000 (jenjo02)
**	    Don't write BMini records to log. Instead, call LGshow
**	    to get the transaction's last LSN to use as the mini transaction
**	    marker.
**	08-Apr-2004 (jenjo02)
**	    Shared log transactions cannot intermingle activity
**	    from multiple threads while one is in a mini-transaction.
**	    Added new LGalter(LG_A_BMINI/LG_A_EMINI) to 
**	    ensure single-threadedness.
*/
DB_STATUS
dm0l_bm(
    DMP_RCB	      *rcb,
    LG_LSN	      *lsn,
    DB_ERROR          *dberr)
{
    DB_STATUS	    status;
    CL_ERR_DESC	    sys_err;

    CLRDBERR(dberr);

    MEcopy((char *)&rcb->rcb_log_id, sizeof(rcb->rcb_log_id), (char *)lsn);

    /* Use transaction's last LSN as BMini marker */

    /*
    ** If SHARED transaction is already in a mini-transaction
    ** (and it's not us), LGalter will put us to sleep
    ** until the mini ends.
    */
    status = LGalter(LG_A_BMINI, (PTR)lsn, sizeof(*lsn), &sys_err);

    if (status == OK)
    {
	rcb->rcb_state |= RCB_IN_MINI_TRAN;

	if (DMZ_SES_MACRO(11))
	{
	    DM0L_BM	    log;

	    /* Fake a BMINI log record for tracing */
	    log.bm_header.length = 0;
	    log.bm_header.type = DM0LBMINI;
	    log.bm_header.flags = (is_journal(rcb) ? DM0L_JOURNAL : 0);
	    log.bm_header.lsn = *lsn;

	    /* Fake that which we don't know */
	    log.bm_header.database_id = 0;
	    log.bm_header.tran_id.db_high_tran = 0;
	    log.bm_header.tran_id.db_low_tran = 0;

	    dmd_logtrace(&log.bm_header, 0);
	}

	return (E_DB_OK);
    }

    return(log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_BMINI,
	E_DM921C_DM0L_BM, dberr, status));
}

/*{
** Name: dm0l_em	- Write End Mini-Transaction record.
**
** Description:
**      This routine writes a end mini-transaction log record to the log file.
**	This record is always forced to disk.
**
** Inputs:
**	rcb				Pointer to RCB.
**	bm_lsn				LSN of the Begin Mini
**
** Outputs:
**      lsn                             Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Made Emini log records journaled if the database is journaled.
**	    Changed EndMini log record to look like a CLR record.  The
**		compensated_lsn field now takes the place of the old
**		em_bm_address.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Clear the RCB_IN_MINI_TRAN flag to record that this RCB is no
**		    longer in the midst of a mini-transaction.
**	14-Mar-2000 (jenjo02)
**	    dm0l_bm() no longer writes BMINI records or 
**	    reserves logspace for the EMINI, so write EMINI
**	    with LG_NOT_RESERVED flag.
**	08-Apr-2004 (jenjo02)
**	    Shared log transactions cannot intermingle activity
**	    from multiple threads while one is in a mini-transaction.
**	    Added new LGalter(LG_A_BMINI/LG_A_EMINI) to 
**	    ensure single-threadedness.
*/
DB_STATUS
dm0l_em(
    DMP_RCB	    *rcb,
    LG_LSN	    *bm_lsn,
    LG_LSN	    *lsn,
    DB_ERROR	    *dberr)
{
    DM0L_EM	    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.em_header.length = sizeof(DM0L_EM);
    log.em_header.type = DM0LEMINI;
    log.em_header.flags = (is_journal(rcb) ? DM0L_JOURNAL : 0);

    /*
    ** The End Mini Transaction log record looks like a CLR which points
    ** back to the Begin Mini Transaction LSN.  If encountered during
    ** abort processing this will cause us to skip over all the updates made
    ** inside the mini transaction.
    */
    log.em_header.flags |= DM0L_CLR;
    log.em_header.compensated_lsn = *bm_lsn;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(LG_FORCE | LG_NOT_RESERVED,
			rcb->rcb_log_id, (i4)1, &lg_object,
			&lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	/* Announce that this MINI is done */
	status = LGalter(LG_A_EMINI, 
			(PTR)&rcb->rcb_log_id, 
			sizeof(rcb->rcb_log_id), &error);
	if ( status == OK )
	{
	    rcb->rcb_state &= ~RCB_IN_MINI_TRAN;

	    if (DMZ_SES_MACRO(11))
		dmd_logtrace(&log.em_header, 0);

	    return (E_DB_OK);
	}

	return(log_error(E_DM900B_BAD_LOG_ALTER, &error, LG_A_EMINI,
	    E_DM921D_DM0L_EM, dberr, status));
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, rcb->rcb_log_id,
	E_DM921D_DM0L_EM, dberr, status));
}

/*{
** Name: dm0l_savepoint	- Establish Savepoint LSN.
**
** Description:
**	This routine returns the last written LSN of the transaction,
**	which establishes the location in the log of a SAVEPOINT.
**
** Inputs:
**      lg_id                           Logging system identifier.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	12-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	08-feb-1993 (jnash)
**	    Reduced logging project.  Add flag param, savepoints may now
**	    be journaled.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
**	14-Mar-2000 (jenjo02)
**	    Don't write SAVEPOINT records to log. Instead, call LGshow
**	    to get the transaction's last LSN to use as a savepoint
**	    marker.
*/
DB_STATUS
dm0l_savepoint(
    i4             lg_id,
    i4		   sp_id,
    i4		   flag,
    LG_LSN	   *lsn,
    DB_ERROR       *dberr)
{
    DB_STATUS	    status;
    CL_ERR_DESC	    sys_err;
    i4		    length;

    CLRDBERR(dberr);

    /* Use this transaction's last LSN as the savepoint marker */
    MEcopy((char *)&lg_id, sizeof(lg_id), (char *)lsn);
    status = LGshow(LG_S_LASTLSN, (PTR)lsn, sizeof(*lsn), &length, &sys_err);

    if ( status )
	return(log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_S_LASTLSN,
		E_DM9221_DM0L_SAVEPOINT, dberr, status));

    if (DMZ_SES_MACRO(11))
    {
	DM0L_SAVEPOINT  log;
	
	/* Fake a SAVEPOINT log record for tracing */
	log.s_header.length = 0;
	log.s_header.type = DM0LSAVEPOINT;
	log.s_header.flags = flag;
	log.s_id = sp_id;
	log.s_header.lsn = *lsn;

	/* Fake that which we don't know */
	log.s_header.database_id = 0;
	log.s_header.tran_id.db_high_tran = 0;
	log.s_header.tran_id.db_low_tran = 0;

	dmd_logtrace(&log.s_header, 0);
    }

    return (E_DB_OK);
}

/*{
** Name: dm0l_abortsave	- Write Abort savepoint record.
**
** Description:
**      This routine writes an abort savepoint record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Zero or DM0L_CLUSTER_REQ.
**	name				The name of the saevpoint.
**	savepoint_lsn			LSN of the SAVEPOINT record.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	10-jan-1990 (rogerk)
**	    If DM0L_CLUSTER_REQ flag is passed in, then use LGCwrite to write
**	    the abortsave record.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed AbortSave log record to look like a CLR record.  The
**		compensated_lsn field now takes the place of the old
**		as_savepoint log address.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Remove LGCwrite call.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
*/
DB_STATUS
dm0l_abortsave(
    i4	    lg_id,
    i4	    sp_id,
    i4	    flags,
    LG_LSN	    *savepoint_lsn,
    LG_LSN	    *lsn,
    DB_ERROR	    *dberr)
{
    DM0L_ABORT_SAVE log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4	    loc_error;

    CLRDBERR(dberr);

    log.as_header.length = sizeof(DM0L_ABORT_SAVE);
    log.as_header.type = DM0LABORTSAVE;
    log.as_header.flags = flags;

    /*
    ** The Abort to Savepoint log record looks like a CLR which points
    ** back to the Savepoint record.  If encountered during abort processing
    ** this will cause us to skip over all the rollback work already done.
    */
    log.as_header.flags |= DM0L_CLR;
    log.as_header.compensated_lsn = *savepoint_lsn;

    log.as_id = sp_id;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(LG_FORCE | LG_NOT_RESERVED, lg_id, (i4)1, &lg_object,
			    &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.as_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9222_DM0L_ABORTSAVE, dberr, status));
}

/*{
** Name: dm0l_frename 	- Write a File rename record.
**
** Description:
**      This routine writes a file rename record to the log file.
**
**	Since the rename operations are permanent as soon as they are issued,
**	we need to force this log record out to disk in order to satisfy the
**	Write-Ahead Log prototocol.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	path				Pointer to path.
**	l_path				Length of path name.
**	file				Pointer to file.
**	nfile				Pointer to new file.
**	tbl_id				Table to which file belongs.
**	loc_id				Location index in table loc array.
**	cnf_loc_id			Location index in database loc array.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	29-aug-1990 (bryanp)
**	    Add LG_FORCE flag to the LGwrite() call.
**	28-oct-1992 (jnash)
**	    Reduced logging project.  Add loc_id, cnf_loc_id, tbl_id and
**	    prev_lsn params. loc_id not currently used in recovery.
**	15-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	30-apr-2004 (jenjo02)
**	    Add LG_RS_FORCE to LGreserve call. FRENAME log records
**	    are forced, potentially eating a log page during
**	    recovery.
*/
DB_STATUS
dm0l_frename(
    i4             lg_id,
    i4		flag,
    DM_PATH		*path,
    i4		l_path,
    DM_FILE		*file,
    DM_FILE		*nfile,
    DB_TAB_ID 		*tbl_id,
    i4 		loc_id,
    i4		cnf_loc_id,
    LG_LSN	   	*prev_lsn,
    LG_LSN		*lsn,
    DB_ERROR            *dberr)
{
    DM0L_FRENAME    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4	    loc_error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2, 2 * sizeof(DM0L_FRENAME), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    &loc_error, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM9223_DM0L_FRENAME, dberr, status));
	}
    }

    log.fr_header.length = sizeof(DM0L_FRENAME);
    log.fr_header.type = DM0LFRENAME;
    log.fr_header.flags = flag;
    log.fr_path = *path;
    log.fr_l_path = l_path;
    log.fr_oldname = *file;
    log.fr_newname = *nfile;
    log.fr_tbl_id = *tbl_id;
    log.fr_loc_id = loc_id;
    log.fr_cnf_loc_id = cnf_loc_id;
    if (flag & DM0L_CLR)
	log.fr_header.compensated_lsn = *prev_lsn;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(LG_FORCE, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.fr_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_FRENAME));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9223_DM0L_FRENAME, dberr, status));
}

/*{
** Name: dm0l_fcreate 	- Write a File create record.
**
** Description:
**      This routine writes a file create record to the log file.
**
**	Since the create operations is permanent as soon as it is issued,
**	we need to force this log record out to disk in order to satisfy the
**	Write-Ahead Log prototocol.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	path				Pointer to path.
**	l_path				Length of path name.
**	file				Pointer to file.
**	tbl_id				Table to which file belongs.
**	loc_id				Location index in table loc array.
**	cnf_loc_id			Location index in database loc array.
**	page_size			Page size of the file(s)
**
** Outputs:
**      lsn                             Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	29-aug-1990 (bryanp)
**	    Add LG_FORCE flag to the LGwrite() call.
**      15-aug-91 (jnash) merged 14-jun-1991 (Derek)
**          Added mf_pos and mf_max fields which describe which
**          file of a multiple file table is being created.  Used to
**          determine which file requires initialized allocation structures.
**          (Nash's note: Also added "allocation" param, assumed to be
**          from Derek.)
**	01-dec-1992 (jnash)
**	    Reduced logging project.  Add prev_lsn param, eliminate
**	    mf_pos, mf_max, allocation.
**	12-jan-1992 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-aug-1993 (rogerk)
**	    Added location id and table_id information for partial recovery
**	    considerations.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for table-specific page sizes. Add page_size argument,
**		use it to set the duc_page_size field.
**	30-apr-2004 (jenjo02)
**	    Add LG_RS_FORCE to LGreserve call. FCREATE log records
**	    are forced, potentially eating a log page during
**	    recovery.
*/
DB_STATUS
dm0l_fcreate(
    i4             lg_id,
    i4		flag,
    DM_PATH		*path,
    i4		l_path,
    DM_FILE		*file,
    DB_TAB_ID 		*tbl_id,
    i4 		loc_id,
    i4		cnf_loc_id,
    i4		page_size,
    i4		loc_status,
    LG_LSN	   	*prev_lsn,
    LG_LSN		*lsn,
    DB_ERROR            *dberr)
{
    DM0L_FCREATE    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4	    loc_error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2, 2 * sizeof(DM0L_FCREATE), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    &loc_error, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM9224_DM0L_FCREATE, dberr, status));
	}
    }

    log.fc_header.length = sizeof(DM0L_FCREATE);
    log.fc_header.type = DM0LFCREATE;
    log.fc_header.flags = flag;
    log.fc_path = *path;
    log.fc_l_path = l_path;
    log.fc_file = *file;
    log.fc_tbl_id = *tbl_id;
    log.fc_loc_id = loc_id;
    log.fc_cnf_loc_id = cnf_loc_id;
    log.fc_page_size = page_size;
    log.fc_loc_status = loc_status;
    if (flag & DM0L_CLR)
	log.fc_header.compensated_lsn = *prev_lsn;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(LG_FORCE, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.fc_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_FCREATE));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9224_DM0L_FCREATE, dberr, status));
}

/*{
** Name: dm0l_opendb	- Write an Open database log record.
**
** Description:
**      This routine writes an open database log record to the log file.
**
**	In 6.4, this log record was sometimes marked journalled and sometimes
**	not marked journalled. If the log record was marked journalled, then
**	the interpretation was that the database it described was journaled.
**	However, this log record was never actually copied to the journal
**	files. This led to some strange places in the code where we had to
**	treat this record as a special case. In 6.5, this log record is never
**	marked journaled, and is never copied to the journal files. If the
**	database being opened is journalled, then the o_isjournaled field is
**	non-zero; else it is zero.
**
** Inputs:
**      name                            Database name.
**	owner				Database owner.
**	root				Database root location.
**	l_root				Length of root location.
**	flag				Flags:
**					DM0L_JOURNAL - database is journaled.
**					DM0L_FASTCOMMIT - fast commit in use.
**					DM0L_NOTDB - not adding a database
**					DM0L_NOLOGGING - Add a database to the
**					    logging system, but don't write
**					    an opendb log record.
** Outputs:
**	db_id				Return assigned db_id.
**      err_code                        Reason for error status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added LG_FIRST flags to the LGwrite call in dm0l_opendb.
**	    This is since there is no begin/end transaction records surrounding
**	    a dm0l_opendb record.
**	20-Jan-1989 (ac)
**	    Added 2PC support.
**	15-may-1989 (rogerk)
**	    Return DM0062_TRAN_QUOTA_EXCEEDED if LG_EXCEED_LIMIT.
**	11-sep-1989 (rogerk)
**	    Added lx_id parameter to pass back transaction id.  Changed to not
**	    end transaction used for writing opendb record.  The transaction is
**	    preserved until the database is closed to avoid requiring any extra
**	    resources to execute the close.  The transaction log id is now
**	    returned to the caller to be saved until the dm0l_closedb call.
**	23-jan-1990 (rogerk)
**	    Force OPENDB log record so if system crashes, the opendb record
**	    will be found.  Not forcing the log record may cause us to not
**	    process this database in machine failure recovery.  This will
**	    not cause us to miss any work that we need to back out (since
**	    we always force the log before making any DB changes), but
**	    can cause us to not reset the open_count and we then mark the
**	    database inconsistent the first time it is opened.
**	15-feb-1990 (mikem)
**	    Fixed calculation of the size of a subset of members in a structure.
**	    See comments at top of file for detailed explanation.
**	25-feb-1991 (rogerk)
**	    Added DM0L_NOLOGGING flag to allow database to be opened while
**	    the Log File is full.  This was added for the Archiver Stability
**	    project.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	18-jan-1993 (rogerk)
**	    Removed WILLING_COMMIT flag as it is no longer used.  This flag
**	    used to do essentially the same processing as ADDONLY, which is
**	    now used instead in recovery processing.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed ADDONLY flag this routine is no longer used by recovery.
**	    Changed the OPENDB and CLOSEDB records to no longer be marked
**		as journaled log records.  They were never actually copied
**		to the journal files and it was a little weird having a
**		journaled record written from a non-journaled transaction.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Added o_isjournaled so that we can record whether a database is
**		    journaled or not. This is used by CSP archiving to detect
**		    journaled databases by examing the OPENDB records in the
**		    log file.
**		When calling LGadd, pass a DM0L_ADDDB structure, rather than
**		    passing a pointer to the middle of the DM0L_OPENDB
**		    log record. This removes the dependency on having the fields
**		    in the DM0L_OPENDB log record exactly matching the
**		    corresponding fields in the DM0L_ADDDB.
**	26-jul-1993 (bryanp)
**	    Cease writing DM0L_CLOSEDB log records. These weren't of much use,
**	    because we couldn't trust them to tell us when a database was truly
**	    closed by all servers in a database (algorithms which attempt to
**	    match up OPENDB and CLOSEDB log records were doomed to failure
**	    because the logfile BOF might have already moved past some number
**	    of OPENDB log records, without any way of knowing how many such).
**	    I cleaned up the space reservation code to no longer reserve space
**	    for the dm0l_closedb log record.
**	    Furthermore, leaving the $opendb transaction lying around to log the
**	    closedb log record was causing problems for 2PC situations where
**	    we wish dm2d_close_db to remove the DCB but not LGremove the
**	    database. We no longer keep the $opendb transaction lying around.
**	    So I removed the lx_id argument. Finally, if errors occur, I changed
**	    the code to log *all* the errors which occur, including any errors
**	    during cleanup.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
** 	10-jun-1997 (wonst02)
** 	    Added DM0L_READONLY_DB for readonly databases.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
**	09-aug-2010 (maspa05) b123189, b123960
**	    Added paramter, flag2, to dm0l_opendb in order to add a new flag
**          DM0L_RODB to indicate a readonly database, as opposed to one 
**          merely opened read-only.
*/
DB_STATUS
dm0l_opendb(
    i4		    lg_id,
    i4		    flag,
    i4              flag2,
    DB_DB_NAME		    *name,
    DB_OWN_NAME		    *owner,
    i4		    ext_dbid,
    DM_PATH		    *path,
    i4		    l_path,
    i4		    *db_id,
    LG_LSN	    *special_lsn,
    DB_ERROR	    *dberr)
{
    DM0L_OPENDB	    log;
    DB_TRAN_ID	    tran_id;
    LG_LXID	    lx_id;
    LG_LRI	    lri;
    LG_OBJECT	    lg_object;
    DB_STATUS	    status, ret_status;
    CL_ERR_DESC	    error;
    i4	    add_flag;
    DB_OWN_NAME	    user_name;
    DM0L_ADDDB	    add;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.o_header.length = sizeof(DM0L_OPENDB);
    log.o_header.type = DM0LOPENDB;
    log.o_header.flags = (flag & ~DM0L_JOURNAL);
    MEcopy((PTR) name, sizeof(*name), (PTR) &log.o_dbname);
    MEcopy((PTR) owner, sizeof(*owner), (PTR) &log.o_dbowner);
    MEcopy((PTR) path, sizeof(*path), (PTR) &log.o_root);
    log.o_l_root = l_path;
    log.o_dbid = ext_dbid;

    if (flag & DM0L_JOURNAL)
	log.o_isjournaled = 1;
    else
	log.o_isjournaled = 0;

    if ((flag & DM0L_SPECIAL) && special_lsn)
	STRUCT_ASSIGN_MACRO(*special_lsn, log.o_header.lsn);
    else
    {
	log.o_header.lsn.lsn_high = 0;
	log.o_header.lsn.lsn_low = 0;
    }

    /*	Add database entry to logging system. */

    add_flag = 0;
    if (flag & DM0L_JOURNAL)
	add_flag |= LG_JOURNAL;
    if (flag & DM0L_NOTDB)
	add_flag |= LG_NOTDB;
    if (flag & DM0L_READONLY_DB)
    	add_flag |= LG_READONLY;
    if (flag & DM0L_FASTCOMMIT)
	add_flag |= LG_FCT;
    if (flag & DM0L_PRETEND_CONSISTENT)
	add_flag |= LG_PRETEND_CONSISTENT;
    if (flag2 & DM0L_RODB)
	add_flag |= LG_RODB;
    if ( flag & DM0L_MVCC )
        add_flag |= LG_MVCC;

    /*
    ** Construct a DM0L_ADDDB structure which describes this database. This
    ** database information is stored within the logging system and retrieved
    ** by various callers using LGshow(). Since the root location is variable
    ** length, the length computation in the LGadd call is somewhat ugly.
    */
    add.ad_dbname  = log.o_dbname;
    add.ad_dbowner = log.o_dbowner;
    add.ad_dbid    = log.o_dbid;
    add.ad_l_root  = log.o_l_root;
    MEcopy((char *)path, sizeof(*path), (char *)&add.ad_root);

    status = LGadd(lg_id, add_flag, (char *)&add,
	      ((char *)&add.ad_root) - ((char *) &add.ad_dbname) +
							add.ad_l_root,
	      db_id, &error);
    if (status != OK)
    {
	(VOID) uleFormat(NULL, status, &error, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	(VOID) uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &error, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 4, 0, lg_id,
	    sizeof(log.o_dbname), &log.o_dbname,
	    sizeof(log.o_dbowner), &log.o_dbowner,
	    log.o_l_root, &log.o_root);

	if (status == LG_DB_INCONSISTENT)
        {

            uleFormat(NULL, E_DM0152_DB_INCONSIST_DETAIL, (CL_ERR_DESC *)NULL, ULE_LOG,
                (DB_SQLSTATE *)0, (char *)NULL, (i4)0, (i4 *)NULL,
                err_code, 1, sizeof (log.o_dbname), &log.o_dbname);
	    SETDBERR(dberr, 0, E_DM0100_DB_INCONSISTENT);
        }
	else if (status == LG_EXCEED_LIMIT)
	    SETDBERR(dberr, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	else
	    SETDBERR(dberr, 0, E_DM9225_DM0L_OPENDB);
	return (E_DB_ERROR);
    }

    if (flag & DM0L_NOTDB)
	return (E_DB_OK);

    /*
    ** If caller directed to not write an opendb log record, then
    ** return without writing it.
    */
    if (flag & DM0L_NOLOGGING)
	return (E_DB_OK);

    /*	Begin a transaction. */

    STmove((PTR)DB_OPENDB_USER, ' ', sizeof(DB_OWN_NAME), (PTR) &user_name);
    status = LGbegin(LG_NOPROTECT, *db_id, &tran_id, &lx_id,
		     sizeof(DB_OWN_NAME), (char *)&user_name, 
		     (DB_DIS_TRAN_ID*)NULL, &error);
    if (status != OK)
    {
	(VOID) uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &error, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, *db_id);
	if (status == LG_EXCEED_LIMIT)
	    SETDBERR(dberr, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
        else if (status == LG_DB_INCONSISTENT)
        {
            uleFormat(NULL, E_DM0152_DB_INCONSIST_DETAIL, (CL_ERR_DESC *)NULL, ULE_LOG,
                (DB_SQLSTATE *)0, (char *)NULL, (i4)0, (i4 *)NULL,
                err_code, 1, sizeof (log.o_dbname), &log.o_dbname);
	    SETDBERR(dberr, 0, E_DM0100_DB_INCONSISTENT);
        }
	else
	    SETDBERR(dberr, 0, E_DM9225_DM0L_OPENDB);

	/* Remove the LGadd db */
	status = LGremove(*db_id, &error);
	if (status)
	    (void) log_error(E_DM9016_BAD_LOG_REMOVE, &error, *db_id,
		    E_DM9225_DM0L_OPENDB, &local_dberr, status);

	return (E_DB_ERROR);
    }

    /*
    ** Reserve space for open db_log record, write it, 
    ** and end log txn with one call to LGwrite().
    */

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite((LG_FIRST | LG_FORCE | LG_NOT_RESERVED | LG_LAST), lx_id, 
			(i4)1, &lg_object, &lri, &error);
    if (status != OK)
    {
 	ret_status = log_error(E_DM9015_BAD_LOG_WRITE, &error, lx_id,
	    E_DM9225_DM0L_OPENDB, dberr, status);
	status = LGend(lx_id, 0, &error);
	if (status)
	    (void) log_error(E_DM900E_BAD_LOG_END, &error, lx_id,
		    E_DM9225_DM0L_OPENDB, &local_dberr, status);
	status = LGremove(*db_id, &error);
	if (status)
	    (void) log_error(E_DM9016_BAD_LOG_REMOVE, &error, *db_id,
		    E_DM9225_DM0L_OPENDB, &local_dberr, status);
	return (ret_status);
    }

    if ((flag & DM0L_SPECIAL) && special_lsn)
    {
	TRdisplay("DM0LOPENDB DM0L_SPECIAL (%x,%x) written with lsn (%x,%x)\n",
		special_lsn->lsn_high, special_lsn->lsn_low,
		lri.lri_lsn.lsn_high, lri.lri_lsn.lsn_low);
    }

    if (DMZ_SES_MACRO(11))
	dmd_logtrace(&log.o_header, 0);

    return (E_DB_OK);
}

/*{
** Name: dm0l_closedb	- Remove this database from this process.
**
** Description:
**      This routine notifies the logging system that this process is no longer
**	using this database.
**
**	Prior to 26-jul-1993, we used to write a log record describing this.
**	The log record didn't actually describe any updates we were doing, it
**	just recorded that this process was closing the log file. However,this
**	information isn't valuable to anyone, and previous attempts to optimize
**	recovery processing using this log record have been troublesome and
**	buggy. Hence, we have removed this log record, which simplifies the
**	code and removes the chance of mis-using this log record.
**
** Inputs:
**	db_id				Database identifier.
**
** Outputs:
**      err_code                        Reason for error status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added LG_FIRST flags to the LGwrite call in dm0l_closedb.
**	    This is necessary since there are no begin/end transaction
**	    records surrounding a dm0l_closedb record.
**	11-sep-1989 (rogerk)
**	    Added lx_id parameter to specify opendb transaction.  This
**	    should be the lx_id that was returned from the dm0l_opendb
**	    call.  The transaction will be ended here in closedb.
**	    Changed so that it will delete db and end transaction even
**	    if it encounters an error.
**	25-feb-1991 (rogerk)
**	    Added DM0L_NOLOGGING flag to allow database to be opened while
**	    the Log File is full.  This was added for the Archiver Stability
**	    project.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed ADDONLY flag this routine is no longer used by recovery.
**	    Changed the OPENDB and CLOSEDB records to no longer be marked
**		as journaled log records.  They were never actually copied
**		to the journal files and it was a little weird having a
**		journaled record written from a non-journaled transaction.
**	26-jul-1993 (bryanp)
**	    Cease writing DM0L_CLOSEDB log records. These weren't of much use,
**	    because we couldn't trust them to tell us when a database was truly
**	    closed by all servers in a database (algorithms which attempt to
**	    match up OPENDB and CLOSEDB log records were doomed to failure
**	    because the logfile BOF might have already moved past some number
**	    of OPENDB log records, without any way of knowing how many such).
**	    Furthermore, leaving the $opendb transaction lying around to log the
**	    closedb log record was causing problems for 2PC situations where
**	    we wish dm2d_close_db to remove the DCB but not LGremove the
**	    database. We no longer keep the $opendb transaction lying around.
**	    Removed the unused lx_id and flag arguments.
*/
DB_STATUS
dm0l_closedb(
    i4		    db_id,
    DB_ERROR	    *dberr)
{
    DB_STATUS	    status;
    DB_STATUS	    ret_status = E_DB_OK;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    /*	Delete database entry from logging system. */

    status = LGremove(db_id, &error);
    if (status != OK)
    {
	ret_status = log_error(E_DM9016_BAD_LOG_REMOVE, &error, db_id,
	    E_DM9226_DM0L_CLOSEDB, dberr, status);
    }
    return (ret_status);
}

/*{
** Name: dm0l_create	- Write Create table record.
**
** Description:
**      This routine writes a create table record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**      loc_count                       Count of number of locations.
**	location			Table locations.
**      allocation                      Allocation amount
**      extend                          Extend amount
**	create_flags			Flags used to control creation.
**					DM0L_CREATE_CONCURRENT_IDX -
**					  Indicate that this is a concurrent
**					  idx and should not cause a TCB
**					  purge during rollback.
**	relstat				Relation relstat value.
**	page_size			Page size for this table.
**
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**      22-nov-87 (jennifer)
**          Added support for multiple locations per table.
**      15-aug-1991 (jnash) for Derek
**          Add allocation and extend params
**      14-nov-1991 (jnash)
**          Reduced logging project.  New information added to
**	    record file creation, as well as a fhdr and fmap information.
**	13-jan-1992 (jnash)
**	    Add LGreserve call, elim prev_lsn param and CLR handling.
**	    Create has no CLR.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-may-1993 (rmuth)
**	    Concurrent index support, add a create_flag field to dm0l_create.
**	    Did not use the current flag field as this is a general field
**	    used for all log records.
**	23-aug-1993 (rogerk)
**	  - Added back prev_lsn param and CLR handling.
**	    Create has a CLR.
**	  - Added relstat to dm0l_create log record so we can determine in
**	    recovery whether the relation is a gateway table or not.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (rogerk)
**	    Add reserve for log force in undo processing.
**	    Add CLR reservation.
**	06-mar-1996 (stial01 for bryanp)
**	    Add support for table-specific page sizes. Add page_size argument,
**		use it to set the duc_page_size field.
**	13-Jul-2007 (kibro01) b118695
**	    Allocate more space if partitions take us beyond the current
**	    DM0L_CREATE limit of DM_MAX_LOC locations
*/
DB_STATUS
dm0l_create(
    i4             lg_id,
    i4             flag,
    DB_TAB_ID           *tbl_id,
    DB_TAB_NAME         *name,
    DB_OWN_NAME         *owner,
    i4             allocation,
    i4	        create_flags,
    i4		relstat,
    DM_PAGENO		fhdr_pageno,
    DM_PAGENO		first_fmap_pageno,
    DM_PAGENO		last_fmap_pageno,
    DM_PAGENO		first_free_pageno,
    DM_PAGENO		last_free_pageno,
    DM_PAGENO		first_used_pageno,
    DM_PAGENO		last_used_pageno,
    i4             loc_count,
    DB_LOC_NAME         *location,
    i4		page_size,
    i4                  page_type,
    LG_LSN		*prev_lsn,
    LG_LSN		*lsn,
    DB_ERROR		*dberr)
{
    DM0L_CREATE	    log;
    DM0L_CREATE	    *plog;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4         i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.duc_header.length = sizeof(DM0L_CREATE)-
                         ((DM_LOC_MAX - loc_count)* sizeof(DB_LOC_NAME));
    log.duc_header.type = DM0LCREATE;
    log.duc_header.flags = flag;
    log.duc_tbl_id = *tbl_id;
    log.duc_name = *name;
    log.duc_owner = *owner;
    log.duc_loc_count = loc_count;
    log.duc_allocation = allocation;
    log.duc_fhdr = fhdr_pageno;
    log.duc_first_fmap = first_fmap_pageno;
    log.duc_last_fmap = last_fmap_pageno;
    log.duc_first_free = first_free_pageno;
    log.duc_last_free = last_free_pageno;
    log.duc_first_used = first_used_pageno;
    log.duc_last_used = last_used_pageno;
    log.duc_flags     = create_flags;
    log.duc_status = relstat;
    log.duc_page_size = page_size;
    log.duc_page_type = page_type;
    if (flag & DM0L_CLR)
	log.duc_header.compensated_lsn = *prev_lsn;

    if (loc_count <= DM_LOC_MAX)
    {
	plog = &log;
    } else
    {
	/* Too many locations for a standard DM0L_CREATE structure, so
	** define a bigger one with enough additional locations
	** (kibro01) b118695
	*/
	plog = (DM0L_CREATE *)MEreqmem(0, log.duc_header.length, FALSE, NULL);
	if (plog == NULL)
	{
	    (VOID) uleFormat(NULL, E_DM911E_ALLOC_FAILURE, &error,
		ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		err_code, 1, 0, lg_id);
	    return(log_error(E_DM911E_ALLOC_FAILURE, &error,
		lg_id, E_DM9229_DM0L_CREATE, dberr, status));
	}
	MEcopy(&log, sizeof(DM0L_CREATE), plog);
    }

    for (i=0; i< loc_count; i++)
  	STRUCT_ASSIGN_MACRO(location[i],plog->duc_location[i]);

    lg_object.lgo_size = log.duc_header.length;
    lg_object.lgo_data = (PTR) plog;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    ** Include LG_RS_FORCE for log force required in undo.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2,
			    (2 * log.duc_header.length), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    if (plog != &log)
		MEfree((void*)plog);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM9229_DM0L_CREATE, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (plog != &log)
	MEfree((void*)plog);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.duc_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9229_DM0L_CREATE, dberr, status));
}

/*{
** Name: dm0l_crverify	- Write Create table verify.
**
** Description:
**      This routine writes a create table verify record to the log file.
**	The log is used to verify that recovery of a create table
**	operation reconstructs the table as it was originally built.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**      allocation                      Allocation amount
**	fhdr_pageno			Page number of the fhdr page
**	num_fmaps			Number of fmaps
**	relpages			Number of pages in the file
**	relmain				Relmain
**	relprim				Relprim
**      relpages			Number of pages in the file
**      relmain				Relmain.
**	page_size			Page size of table.
**
** Outputs:
**      lsn				Log record lsn.
**	err_code			Reason for error return status.
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
**      14-nov-1991 (jnash)
**          Created for the Reduced logging project.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-mar-1996 (stial01 for bryanp)
**	    Added page_size argument, used it to set ducv_page_size.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
*/
DB_STATUS
dm0l_crverify(
    i4             lg_id,
    i4             flag,
    DB_TAB_ID           *tbl_id,
    DB_TAB_NAME         *name,
    DB_OWN_NAME         *owner,
    i4             allocation,
    i4             fhdr_pageno,
    i4             num_fmaps,
    i4             relpages,
    i4             relmain,
    i4             relprim,
    i4		page_size,
    LG_LSN		*lsn,
    DB_ERROR		*dberr)
{
    DM0L_CRVERIFY   log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.ducv_header.length = sizeof(DM0L_CRVERIFY);
    log.ducv_header.type = DM0LCRVERIFY;
    log.ducv_header.flags = flag;
    log.ducv_tbl_id = *tbl_id;
    log.ducv_name = *name;
    log.ducv_owner = *owner;
    log.ducv_allocation = allocation;
    log.ducv_relpages = relpages;
    log.ducv_relmain = relmain;
    log.ducv_relprim = relprim;
    log.ducv_fhdr_pageno = fhdr_pageno;
    log.ducv_num_fmaps = num_fmaps;
    log.ducv_page_size = page_size;

    lg_object.lgo_size = log.ducv_header.length;
    lg_object.lgo_data = (PTR) &log;

    /*
    ** Reserve logfile space for log and write it.  CRVERIFY has no CLR.
    */

    status = LGwrite(LG_NOT_RESERVED, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.ducv_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C0C_DM0L_CRVERIFY, dberr, status));
}

/*{
** Name: dm0l_destroy	- Write Destroy table record.
**
** Description:
**      This routine writes a destroy table record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**      loc_count                       Count of number of loclations.
**	location			Table locations.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**      22-nov-87 (jennifer)
**          Added support for multiple locations per table.
**      28-nov-92 (jnash)
**          Reduced logging project.  Add CLR support, prev_lsn param.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (rogerk)
**	    Add reserve for log force in undo processing.
**	13-Jul-2007 (kibro01) b118695
**	    Allocate more space if partitions take us beyond the current
**	    DM0L_DESTROY limit of DM_MAX_LOC locations
*/
DB_STATUS
dm0l_destroy(
    i4         lg_id,
    i4	    flag,
    DB_TAB_ID	    *tbl_id,
    DB_TAB_NAME	    *name,
    DB_OWN_NAME	    *owner,
    i4         loc_count,
    DB_LOC_NAME	    *location,
    LG_LSN	    *prev_lsn,
    LG_LSN	    *lsn,
    DB_ERROR	    *dberr)
{
    DM0L_DESTROY    log;
    DM0L_DESTROY    *plog;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4         i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.dud_header.length = sizeof(DM0L_DESTROY)-
                         ((DM_LOC_MAX - loc_count) * sizeof(DB_LOC_NAME));
    log.dud_header.type = DM0LDESTROY;
    log.dud_header.flags = flag | DM0L_NONREDO;
    log.dud_tbl_id = *tbl_id;
    log.dud_name = *name;
    log.dud_owner = *owner;
    log.dud_loc_count = loc_count;
    if (flag & DM0L_CLR)
	log.dud_header.compensated_lsn = *prev_lsn;

    if (loc_count <= DM_LOC_MAX)
    {
	plog = &log;
    } else
    {
	/* Too many locations for a standard DM0L_CREATE structure, so
	** define a bigger one with enough additional locations
	** (kibro01) b118695
	*/
	plog = (DM0L_DESTROY *)MEreqmem(0, log.dud_header.length, FALSE, NULL);
	if (plog == NULL)
	{
	    (VOID) uleFormat(NULL, E_DM911E_ALLOC_FAILURE, &error,
		ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		err_code, 1, 0, lg_id);
	    return(log_error(E_DM911E_ALLOC_FAILURE, &error,
		lg_id, E_DM922A_DM0L_DESTROY, dberr, status));
	}
	MEcopy(&log, sizeof(DM0L_CREATE), plog);
    }
    for (i = 0; i < loc_count; i++)
	STRUCT_ASSIGN_MACRO(location[i], plog->dud_location[i]);

    lg_object.lgo_size = log.dud_header.length;
    lg_object.lgo_data = (PTR) plog;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    ** Include LG_RS_FORCE for log force required in undo.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2,
			    2 * log.dud_header.length, &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    if (plog != &log)
		MEfree((void*)plog);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM922A_DM0L_DESTROY, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (plog != &log)
	MEfree((void*)plog);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.dud_header,
		(flag & DM0L_CLR) ? 0 : log.dud_header.length);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM922A_DM0L_DESTROY, dberr, status));
}

/*{
** Name: dm0l_relocate	- Write Relocate table record.
**
** Description:
**      This routine writes a relocate table record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**	olocation			Old table location.
**	nlocation			New table location.
**      page_size                       Page size
**      loc_id                          Loclation identifier.
**	oloc_config_id			Old config location id
**	nloc_config_id			New config location id
**	file				The name of the file.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	26-nov-1986 (jnash)
**          Reduced logging project.  Add oloc_config_id, nlog_config_id,
**	    prev_lsn and file params, and CLR handling.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (rogerk)
**	    Add reserve for log force in undo processing.
**      06-mar-96 (stial01)
**          Added page_size argument
*/
DB_STATUS
dm0l_relocate(
    i4         lg_id,
    i4	    flag,
    DB_TAB_ID	    *tbl_id,
    DB_TAB_NAME	    *name,
    DB_OWN_NAME	    *owner,
    DB_LOC_NAME	    *olocation,
    DB_LOC_NAME	    *nlocation,
    i4         page_size,
    i4         loc_id,
    i4	    oloc_config_id,
    i4	    nloc_config_id,
    DM_FILE	    *file,
    LG_LSN	    *prev_lsn,
    LG_LSN	    *lsn,
    DB_ERROR	    *dberr)
{
    DM0L_RELOCATE   log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.dur_header.length = sizeof(DM0L_RELOCATE);
    log.dur_header.type = DM0LRELOCATE;
    log.dur_header.flags = flag | DM0L_NONREDO;
    log.dur_tbl_id = *tbl_id;
    log.dur_name = *name;
    log.dur_owner = *owner;
    log.dur_olocation = *olocation;
    log.dur_nlocation = *nlocation;
    log.dur_page_size = page_size;
    log.dur_loc_id = loc_id;
    log.dur_ocnf_loc_id = oloc_config_id;
    log.dur_ncnf_loc_id = nloc_config_id;
    log.dur_file = *file;
    if (flag & DM0L_CLR)
	log.dur_header.compensated_lsn = *prev_lsn;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    ** Include LG_RS_FORCE for log force required in undo.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2,
			    2 * sizeof(DM0L_RELOCATE), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM922B_DM0L_RELOCATE, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.dur_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_RELOCATE));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM922B_DM0L_RELOCATE, dberr, status));
}

/*{
** Name: dm0l_modify	- Write Modify table record.
**
** Description:
**      This routine writes a modify table record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**      loc_count                       Number of locations.
**	location			Table locations.
**	structure			Storage structure.
**	relstat				Relation Status.
**	min_pages			Min_pages.
**	max_pages			Max_pages.
**	ifill				Index fill.
**	dfill				Data fill.
**	lfill				Leaf fill.
**	reltups				Number of rows in the table.
**	buckets				Number of buckets if a hash modify.
**	key_map				Key position map.
**	key_order			Ascending/decending order
**					of the corresponding key.
**					0 means ascending; 1 means
**					decending.
**      allocation                      Allocation amount.
**      extend                          Extend amount.
**	page_size			Page size.
**	name_id				ID part of temp file name generator.
**	name_gen			Generator for temp filenames used.
**	prev_lsn			Prev lsn if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	22-Jul-1986 (ac)
**	    Added key_order arrary to log the decending/ascending order
**	    of the key.
**      22-nov-87 (jennifer)
**          Added support for multiple locations per table.
**	16-jul-1991 (bryanp)
**	    B38527: Add new 'reltups' argument to dm0l_modify & dm0l_index.
**      15-aug-91 (jnash) merged 10-mar-1991 (Derek)
**          Added allocation,extend values.
**	11-nov-1992 (jnash)
**	    Reduced logging project.  Add prev_lsn & CLR logic.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-aug-1993 (rogerk)
**	    Added a statement count field to the modify and index log records.
**	    This is used in modify rollfwd to correctly find the modify files
**	    to load that were re-created by dm0l_fcreate.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (rogerk)
**	    Added buckets parameter and dum_buckets field to facilitate rolldb
**	    recreating tables in the same layout.  Changed documentation of
**	    reltups parameter which is now set for all structural modifies.
**	18-oct-1993 (rogerk)
**	    Add reserve for log force in undo processing.
**	06-mar-1996 (stial01 for bryanp)
**	    Add a page_size argument and use it to set the dum_page_size field.
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          key_map declared as unsigned. Also modified the copy of key_map
**          to log.dum_key to allow for any changes in the base type size
**          of the key attribute type.
**	17-may-2004 (thaju02)
**	    Removed unnecessary rnl_name/dum_rnl_name param. 
**	30-jun-2004 (thaju02)
**	    Add bsf lsn param and use it to set dum_bsf_lsn.
**	13-Jul-2007 (kibro01) b118695
**	    Allocate more space if partitions take us beyond the current
**	    DM0L_MODIFY limit of DM_MAX_LOC locations
*/
DB_STATUS
dm0l_modify(
    i4		    lg_id,
    i4		    flag,
    DB_TAB_ID		    *tbl_id,
    DB_TAB_NAME		    *name,
    DB_OWN_NAME		    *owner,
    i4		    loc_count,
    DB_LOC_NAME		    *location,
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
    i4		    allocation,
    i4		    extend,
    i4              pg_type,
    i4		    page_size,
    i4		    modify_flag,
    DB_TAB_ID	    *rnl_tabid,
    LG_LSN	    bsf_lsn,
    i2		    name_id,
    i4		    name_gen,
    LG_LSN		    *prev_lsn,
    LG_LSN		    *lsn,
    DB_ERROR		    *dberr)
{
    DM0L_MODIFY	    log;
    DM0L_MODIFY	    *plog = NULL;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4         i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.dum_header.length = sizeof(DM0L_MODIFY)-
                         ((DM_LOC_MAX - loc_count)* sizeof(DB_LOC_NAME));
    log.dum_header.type = DM0LMODIFY;
    log.dum_header.flags = flag | DM0L_NONREDO;
    log.dum_tbl_id = *tbl_id;
    log.dum_name = *name;
    log.dum_owner = *owner;
    log.dum_loc_count = loc_count;
    log.dum_structure = structure;
    log.dum_status = relstat;
    log.dum_min = min_pages;
    log.dum_max = max_pages;
    log.dum_ifill = ifill;
    log.dum_dfill = dfill;
    log.dum_lfill = lfill;
    log.dum_reltups = reltups;
    log.dum_buckets = buckets;
    log.dum_allocation = allocation;
    log.dum_extend = extend;
    log.dum_page_size = page_size;
    log.dum_pg_type = pg_type;
    log.dum_flag = modify_flag;
    if (rnl_tabid)
	log.dum_rnl_tbl_id = *rnl_tabid;
    else
	log.dum_rnl_tbl_id.db_tab_base = log.dum_rnl_tbl_id.db_tab_index = 0;

    STRUCT_ASSIGN_MACRO(bsf_lsn, log.dum_bsf_lsn);

    log.dum_name_id = name_id;
    log.dum_name_gen = name_gen;
    MEcopy(key_map, sizeof(log.dum_key), log.dum_key);
    MEcopy(key_order, DB_MAX_COLS, log.dum_order);

    if (flag & DM0L_CLR)
	log.dum_header.compensated_lsn = *prev_lsn;

    if (loc_count <= DM_LOC_MAX)
    {
	plog = &log;
    } else
    {
	/* Too many locations for standard DM0L_MODIFY structure, so
	** define a bigger one with enough additional locations
	** (kibro01) b118695
	*/
	plog = (DM0L_MODIFY *)MEreqmem(0, log.dum_header.length, FALSE, NULL);
	if (plog == NULL)
	{
	    (VOID) uleFormat(NULL, E_DM911E_ALLOC_FAILURE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM911E_ALLOC_FAILURE, &error,
		lg_id, E_DM922C_DM0L_MODIFY, dberr, status));
	}
	MEcopy(&log, sizeof(DM0L_MODIFY), plog);
    }
    for (i=0; i< loc_count; i++)
	STRUCT_ASSIGN_MACRO(location[i], plog->dum_location[i]);

    lg_object.lgo_size = log.dum_header.length;
    lg_object.lgo_data = (PTR) plog;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    ** Include LG_RS_FORCE for log force required in undo.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2,
			    2 * log.dum_header.length, &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    if (plog != &log)
		MEfree((void*)plog);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM922C_DM0L_MODIFY, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (plog != &log)
	MEfree((void*)plog);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.dum_header,
		 (flag & DM0L_CLR) ? 0 : log.dum_header.length);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
    	E_DM922C_DM0L_MODIFY, dberr, status));
}

/*{
** Name: dm0l_index	- Write Index table record.
**
** Description:
**      This routine writes a index table record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**	idx_tbl_id			Index table identifier.
**	name				Table name.
**	owner				Table owner.
**      loc_count                       Number of locations.
**	location			Table locations.
**	structure			Storage structure.
**	relstat				Relation Status.
**	min_pages			Min_pages.
**	max_pages			Max_pages.
**	ifill				Index fill.
**	dfill				Data fill.
**	lfill				Leaf fill.
**	reltups				Number of rows in the table.
**	buckets				Number of buckets if a hash index.
**	key_map				Key position map.
**      allocation                      Allocation amount.
**      extend                          Extend amount.
**	page_size			Page size for this index.
**	name_id				ID part of temp file name generator.
**	name_gen			Generator for temp filenames used.
**	dimension			RTree index dimension
**	hilbertsize			RTree index hilbert size
**	range				RTree index range
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	22-Jul-1986 (ac)
**	    Added index table id to the log record.
**      22-nov-87 (jennifer)
**          Added support for multiple locations per table.
**	16-jul-1991 (bryanp)
**	    B38527: Add new 'reltups' argument to dm0l_modify & dm0l_index.
**      15-aug-1991 (jnash) merged 10-mar-1991 (Derek)
**          Added allocation,extend values.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-aug-1993 (rogerk)
**	    Added a statement count field to the modify and index log records.
**	    This is used in index rollfwd to correctly find the modify files
**	    to load that were re-created by dm0l_fcreate.
**	    Added prev_lsn parameter and CLR support.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (rogerk)
**	    Added buckets parameter and dui_buckets field to facilitate rolldb
**	    recreating tables in the same layout.  Changed documentation of
**	    reltups parameter which is now set for all index operations.
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument; use it to set dui_page_size.
**	23-may-1996 (shero03)
**	    RTree - added dimension, hilbertsize, range to dm0l_index
**  18-Aug-1997 (bonro01)
**      Fixed memory alignment problem of range variable by changing
**      a for() assignment loop of (f8) variables to use MEcopy()
**      for the entire structure.  The source for the copy is from
**      the log record buffer which we can't assume will be 8-byte
**      aligned.  The destination structure IS 8-byte aligned since
**      it is a local variable structure in this routine.
**	24-Sep-1997 (bonro01)
**	    Fixed problem caused by previous update.
**	23-Dec-2003 (jenjo02)
**	    Added relstat2, dui_relstat2 for Global Indexes,
**	    Partitioning.
**      07-apr-2004 (horda03) Bug 96820/INGSRV850
**          key_map declared as unsigned. Also modified the copy of key_map
**          to log.dui_key to allow for any changes in the base type size
**          of the key attribute type.
**	30-May-2006 (jenjo02)
**	    dui_key dimension expanded from DB_MAXKEYS to DB_MAXIXATTS
**	    for support of indexes on Clustered tables.
**	5-Nov-2009 (kschendel) SIR 122739
**	    Fix outrageous name confusion re acount vs kcount.
**	16-Jul-2010 (kschendel) SIR 123450
**	    Pass data/key compression type to log record.
*/
DB_STATUS
dm0l_index(
    i4		    lg_id,
    i4		    flag,
    DB_TAB_ID		    *tbl_id,
    DB_TAB_ID		    *idx_tbl_id,
    DB_TAB_NAME		    *name,
    DB_OWN_NAME		    *owner,
    i4		    loc_count,
    DB_LOC_NAME		    *location,
    i4		    structure,
    u_i4	    relstat,
    u_i4	    relstat2,
    i4		    min_pages,
    i4		    max_pages,
    i4		    ifill,
    i4		    dfill,
    i4		    lfill,
    i4		    reltups,
    i4		    buckets,
    DM_ATT	    *key_map,
    i4		    kcount,
    i4		    allocation,
    i4		    extend,
    i4              pg_type,
    i4		    page_size,
    i2		    comptype,
    i2		    ixcomptype,
    i2		    name_id,
    i4		    name_gen,
    i4			    dimension,
    i4			    hilbert_size,
    f8			    *range,
    LG_LSN		    *prev_lsn,
    LG_LSN		    *lsn,
    DB_ERROR		    *dberr)
{
    DM0L_INDEX	    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4         i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.dui_header.length = sizeof(DM0L_INDEX)-
                         ((DM_LOC_MAX - loc_count)* sizeof(DB_LOC_NAME));
    log.dui_header.type = DM0LINDEX;
    log.dui_header.flags = flag | DM0L_NONREDO;
    log.dui_tbl_id = *tbl_id;
    log.dui_idx_tbl_id = *idx_tbl_id;
    log.dui_name = *name;
    log.dui_owner = *owner;
    log.dui_loc_count = loc_count;
    for (i=0; i< loc_count; i++)
	STRUCT_ASSIGN_MACRO(location[i], log.dui_location[i]);
    log.dui_structure = structure;
    log.dui_status = relstat;
    log.dui_relstat2 = relstat2;
    log.dui_min = min_pages;
    log.dui_max = max_pages;
    log.dui_ifill = ifill;
    log.dui_dfill = dfill;
    log.dui_lfill = lfill;
    log.dui_reltups = reltups;
    log.dui_buckets = buckets;
    log.dui_kcount = kcount;
    log.dui_comptype = comptype;
    log.dui_ixcomptype = ixcomptype;
    log.dui_allocation = allocation;
    log.dui_extend = extend;
    log.dui_page_size = page_size;
    log.dui_pg_type = pg_type;
    log.dui_name_id = name_id;
    log.dui_name_gen = name_gen;
    log.dui_dimension = dimension;
    log.dui_hilbertsize = hilbert_size;

    MEcopy(range, DB_MAXRTREE_DIM*sizeof(f8)*2, log.dui_range);

    MEcopy(key_map, sizeof(log.dui_key), log.dui_key);

    if (flag & DM0L_CLR)
	log.dui_header.compensated_lsn = *prev_lsn;

    lg_object.lgo_size = log.dui_header.length;
    lg_object.lgo_data = (PTR) &log;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(0, lg_id, 2, 2 * log.dui_header.length, &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM922D_DM0L_INDEX, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.dui_header,
		 (flag & DM0L_CLR) ? 0 : log.dui_header.length);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM922D_DM0L_INDEX, dberr, status));
}

/*{
** Name: dm0l_alter	- Write Alter table record.
**
** Description:
**      This routine writes an alter table record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	22-jun-1987 (rogerk)
**          Created for Jupiter.
**	29-nov-1992 (jnash)
**          Reduced logging project.  Add CLR prev_lsn param and
**	    CLR handling, elim STRUCT_ASSIGN_MACROs.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (rogerk)
**	    Add reserve for log force in undo processing.
*/
DB_STATUS
dm0l_alter(
    i4	    lg_id,
    i4	    flag,
    DB_TAB_ID	    *tbl_id,
    DB_TAB_NAME	    *name,
    DB_OWN_NAME	    *owner,
    i4	    count,
    i4	    action,
    LG_LSN	    *prev_lsn,
    LG_LSN	    *lsn,
    DB_ERROR	    *dberr)
{
    DM0L_ALTER	    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    ** Include LG_RS_FORCE for log force required in undo.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2,
			    2 * sizeof(DM0L_ALTER), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM932F_DM0L_ALTER, dberr, status));
	}
    }

    log.dua_header.length = sizeof(DM0L_ALTER);
    log.dua_header.type = DM0LALTER;
    log.dua_header.flags = flag;
    log.dua_tbl_id = *tbl_id;
    log.dua_name = *name;
    log.dua_owner = *owner;
    log.dua_count = count;
    log.dua_action = action;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;
    if (flag & DM0L_CLR)
	log.dua_header.compensated_lsn = *prev_lsn;

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.dua_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_ALTER));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM932F_DM0L_ALTER, dberr, status));
}

/*{
** Name: dm0l_position	- Position log file for reading.
**
** Description:
**      This routine positions the log file for reading the records of
**	an aborted transaction.
**
** Inputs:
**      flag                            Either DM0L_BY_LGA, DM0L_FLGA,
**					DM0L_PAGE, DM0L_TRANS and DM0L_LAST.
**	lx_id				Logging identifer for a transaction.
**	lga				Optional log address.
**	lctx				Logging system read context.
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      24-jul-1986 (Derek)
**          Created for Jupiter.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Cluster support is now handled through ordinary LGposition.
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		In dm0l_position, we no longer move the lx_id to the lctx.
**		    This must be done separately.
**	26-may-1993 (andys)
**	    Use LG_P_LAST in LGposition if DM0L_LAST is passed.
*/
DB_STATUS
dm0l_position(
    i4         flag,
    i4	    lx_id,
    LG_LA	    *lga,
    DMP_LCTX	    *lctx,
    DB_ERROR	    *dberr)
{
    CL_ERR_DESC		sys_err;
    DB_STATUS		status;

    CLRDBERR(dberr);

    if (flag == DM0L_BY_LGA)
    {
	status = LGposition(lx_id, LG_P_LGA, LG_D_PREVIOUS,
		lga, lctx->lctx_area,
		lctx->lctx_l_area, &sys_err);
	if (status != OK)
	    return (log_error(E_DM9013_BAD_LOG_POSITION, &sys_err, lx_id,
		    E_DM922E_DM0L_POSITION, dberr, status));
    }
    else if (flag == DM0L_LAST)
    {
	status = LGposition(lx_id, LG_P_LAST, LG_D_BACKWARD,
		lga, lctx->lctx_area,
		lctx->lctx_l_area, &sys_err);
	if (status != OK)
	    return (log_error(E_DM9013_BAD_LOG_POSITION, &sys_err, lx_id,
		    E_DM922E_DM0L_POSITION, dberr, status));
    }
    else if (flag == DM0L_FLGA)
    {
	status = LGposition(lx_id, LG_P_LGA, LG_D_FORWARD,
		lga, lctx->lctx_area,
		lctx->lctx_l_area, &sys_err);
	if (status != OK)
	    return (log_error(E_DM9013_BAD_LOG_POSITION, &sys_err, lx_id,
		    E_DM922E_DM0L_POSITION, dberr, status));
    }
    else if (flag == DM0L_PAGE)
    {
	status = LGposition(lx_id, LG_P_PAGE, LG_D_FORWARD,
		lga, lctx->lctx_area,
		lctx->lctx_l_area, &sys_err);
	if (status != OK)
	    return (log_error(E_DM9013_BAD_LOG_POSITION, &sys_err, lx_id,
		    E_DM922E_DM0L_POSITION, dberr, status));
    }
    else if (flag == DM0L_FIRST)
    {
	status = LGposition(lx_id, LG_P_FIRST, LG_D_FORWARD,
		lga, lctx->lctx_area,
		lctx->lctx_l_area, &sys_err);
	if (status != OK)
	    return (log_error(E_DM9013_BAD_LOG_POSITION, &sys_err, lx_id,
		    E_DM922E_DM0L_POSITION, dberr, status));
    }
    else
    {
	/* flag == DM0L_TRANS */
	status = LGposition(lx_id, LG_P_TRANS, LG_D_PREVIOUS,
		(LG_LA *)NULL, lctx->lctx_area,
		lctx->lctx_l_area, &sys_err);
	
	if ( status != OK )
	    return (log_error(E_DM9013_BAD_LOG_POSITION, &sys_err, lx_id,
		    E_DM922E_DM0L_POSITION, dberr, status));
    }

    return (E_DB_OK);
}

/*{
** Name: dm0l_read	- Read next record from transaction.
**
** Description:
**      This routine reads the next, actually previous, log record for
**	the transaction.
**
**	You must pass in the transaction handle to use for reading. If you are
**	reading records on behalf of a particular transaction (that is, to
**	abort the transaction), then please pass in the handle of that
**	transaction. If you are reading the logfile for general purposes, not
**	on behalf of any transaction (e.g., archiving, REDO, logdump), then
**	you should use some general purpose transaction id. For example, the
**	archiver begins a special transaction for this purpose, while recovery
**	processing uses the lctx_lxid. If no other transaction handle is
**	available, the lctx_lxid may be used.
**
**	Please note carefully the distinction between a log address and a log
**	sequence number. Both attributes of a log record are returned by this
**	routine. The log address is the physical location of this log record
**	in this log file. The log address can be used to reposition to and
**	reread this log record. The log sequence number is a logical attribute
**	of this log record, which serializes all the log records in all the
**	various log files of a multi-node logging system (e.g., VAXCluster or
**	ICL Goldrush machine). The log sequence number is copied onto the
**	page header and is used to associate a particular log record with a
**	particular version of a page.
**
** Inputs:
**      lctx                            The logging system context.
**	lx_id				Transaction handle with which to read.
**	bufid				Optional pointer to a LG buffer id
**					for reading from the log buffers.
**
** Outputs:
**      record                          Address of pointer to log record.
**	lga				Pointer to log address of this record.
**	bufid				The buffer in which the log record
**					was found, zero if read from log.
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN			E_DM9238_LOG_READ_EOF
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-jul-1986 (Derek)
**          Created for Jupiter.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Added explicit lx_id argument to allow callers to specify the
**		    correct transaction handle to use.
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Cluster support is now handled through ordinary LGread.
**	10-Nov-2009 (kschendel) SIR 122757
**	    l_context arg to lgread deleted.
**	04-Feb-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add bufid as read option.
*/
DB_STATUS
dm0l_read(
    DMP_LCTX	    *lctx,
    i4	    lx_id,
    DM0L_HEADER	    **record,
    LG_LA	    *lga,
    i4		    *bufid,
    DB_ERROR	    *dberr)
{
    CL_ERR_DESC	    sys_err;
    DB_STATUS	    status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = LGread(lx_id, lctx->lctx_area,
	    (LG_RECORD **)&lctx->lctx_header, lga, bufid, &sys_err);
    if (status != OK)
    {
	if (status == LG_ENDOFILE)
	{
	    SETDBERR(dberr, 0, E_DM9238_LOG_READ_EOF);
	    return (E_DB_WARN);
	}
	(VOID) uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lx_id);

	return (log_error(E_DM9014_BAD_LOG_READ, &sys_err, lx_id,
		    E_DM922F_DM0L_READ, dberr, status));
    }
    *record = (DM0L_HEADER*)((char *)lctx->lctx_header + sizeof(LG_RECORD) - 4);
    return (E_DB_OK);
}

/*{
** Name: dm0l_force	- Force log to disk.
**
** Description:
**      Unless the 'lsn' parameter is given, this routine forces all log records
**	which have been written up to this point to be on disk.
**
**	If 'lsn' is specified, then all log records up to and including the
**	 log record specified are forced to disk.
**
**	If the 'nlsn' parameter is specified, it will be filled in with the
**	log sequence number of the last record that LG tells us is guaranteed to
**	already be on disk.
**
**	Note that LGforce actually implements the ability to force all log
**	records up to and including the last log record which has been written
**	for this transaction to be on disk. That functionality is requested by
**	passing "0" for the lsn and passing "0" for the LGforce flag. However,
**	we do not make available that functionality here, as forcing the entire
**	log file is sufficient for most DMF purposes and is not deemed to be
**	much more expensive.
**
** Inputs:
**      lg_id                           Log identifier.
**	lsn				Log sequence number of record to force
**					(optional - 0 if not desired).
**
** Outputs:
**	nlga				Last log seq num guaranteed flushed
**					to disk by logging system (optional).
**      err_code                        Reason for error return status.
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
**      25-jul-1986 (Derek)
**          Created for Jupiter.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Pass LG_LAST to LGforce if forcing the whole log file.
[@history_template@]...
*/
DB_STATUS
dm0l_force(
    i4	    lg_id,
    LG_LSN	    *lsn,
    LG_LSN	    *nlsn,
    DB_ERROR	    *dberr)
{
    CL_ERR_DESC          sys_err;
    i4		status;
    i4		force_flag = 0;

    CLRDBERR(dberr);

    if (lsn == 0)
	force_flag = LG_LAST;

    status = LGforce(force_flag, lg_id, lsn, nlsn, &sys_err);
    if (status == OK)
	return (E_DB_OK);
    return (log_error(E_DM9010_BAD_LOG_FORCE, &sys_err, lg_id,
	E_DM9230_DM0L_FORCE, dberr, status));
}

/*{
** Name: dm0l_allocate	- Allocate and initialize the LCTX
**
** Description:
**      This routine opens the logging system, allocates the LCTX
**	based on the size computed from the logging system.
**
** Inputs:
**	flag				Special options.
**					DM0L_NOLOG
**					DM0L_ARCHIVER
**					DM0L_FASTCOMMIT - server using fast
**					    commit.
**					DM0L_RECOVER - CSP opening for recovery
**					DM0L_FULL_FILENAME - 'name' is the
**					    complete filename, not just the
**					    nodename -- used for standalone
**					    logdump.
**					DM0L_SCAN_LOGFILE - used by standalone
**					    logdump to open the logfile for a
**					    full scan rather than using the
**					    bof/eof values in the log header.
**					    The flag directs dm0l_allocate
**					    to request that LGopen ignore the
**					    header bof value and to position
**					    the logfile for a full scan.
**                                      DM0L_PARTITION_FILENAMES - used by standalone
**                                          logdump when a set of file names have
**                                          been supplied.
**	size				Log file page size.
**	lg_id				Log system id.
**      name                            Log name for VAXcluster or array of filenames.
**      l_name                          Length of name or number of log partitions.
**
** Outputs:
**      err_code                        Reason for error return status.
**					E_DM0081_NO_LOGGING_SYSTEM
**					E_DM9232_DM0L_ALLOCATE
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
**      28-jul-1986 (Derek)
**          Created for Jupiter.
**	15-sep-1988 (rogerk)
**	    Added return E_DM0081_NO_LOGGING_SYSTEM for when the logging
**	    system is not active.
**	08-feb-1989 (greg)
**	    was returning stack garbage.  Made it return E_DB_ERROR.
**	    A real error return needs to be added.
**	09-feb-1989 (EdHsu)
**	    Log real error when dm0m_allocate failed and no lg_id.
**	15-may-1989 (rogerk)
**	    Check for TRAN_QUOTA_EXCEEDED on call to LGbegin.
**	17-may-1989 (rogerk)
**	    Add check for LG_DBINFO_MAX constant definition being large
**	    enough to hold maximum database name information.  This prevents
**	    us from changing the DB_MAXNAME/DB_AREA_MAX constants without
**	    updating the logging system database buffers.
**	30-oct-1991 (bryanp)
**	    Only log a LGclose error in dm0l_allocate if an LGclose error
**	    actually occurred (we were unconditionally calling log_error).
**	22-jul-1992 (bryanp)
**	    Modified the DM0L_NOLOG flag so that it specifically means "don't
**	    call LGopen". This allows a caller who has already opened the log
**	    file to call dm0l_allocate without needing to pass in the logfile
**	    page size (the recovery server uses this functionality).
**	26-oct-1992 (rogerk)
**	    Fixed log_error call which passed the error message parameter
**	    in the wrong spot of the parameter list.
**	26-apr-1993 (bryanp/keving)
**	    6.5 Cluster support:
**		Call LGopen with node name passed in. Check for DM0L_RECOVER
**		    flag. This causes logs to be opend with master flag.
**		Start a logreader transaction in dm0l_allocate for recovery use.
**		DM0L_FULL_FILENAME ==> LG_FULL_FILENAME -- used by standalone
**		    logdump.
**		Set lctx_node_id if this is not a cluster.
**	21-jun-1993 (rogerk)
**	    Added DM0L_SCAN_LOGFILE flag to cause LGopen to do the logfile
**	    scan for bof_eof and to position the logfile for a full scan rather
**	    than only reading values between the logical bof and eof.  This
**	    can be used by logdump to dump entire log files.
**	23-aug-1993 (rogerk)
**	    Added lctx_bkcnt field to allow users to reference the log file
**	    size.  This facilitates log address arithmetic.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**      30-Oct-2009 (horda03) Bug 122823
**          If DM0L_PARTITION_FILENAMES specified, set LG_PARTITION_FILENAMES.
**          name will point to an array if LG_PARTITIONs, and l_name will
**          be the number of partitions making the TX log file.
**	10-Nov-2009 (kschendel) SIR 122757
**	    Include IO alignment in LCTX size.  (LG will apply the alignment.)
**	04-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Added support for buffered log reads.
*/
DB_STATUS
dm0l_allocate(
    i4		flag,
    i4		size,
    i4		lg_id,
    char                *name,
    i4             l_name,
    DMP_LCTX		**lctx_ptr,
    DB_ERROR            *dberr)
{
    DMP_LCTX		*lctx;
    LG_HEADER		*header;
    i4		length;
    u_i4		open_flag;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    DB_OWN_NAME		user_name;
    LG_HDR_ALTER_SHOW   show_hdr;
    i4			align;
    i4		        *err_code = &dberr->err_code;
    i4			showType;
    i4			logBlocks;
    i4			CacheSize;

    CLRDBERR(dberr);

    /*	Prepare the logging system. */

    status = LGinitialize(&sys_err, ERx("logging system"));
    if (status != OK)
    {
	uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	(VOID) uleFormat(NULL, E_DM9011_BAD_LOG_INIT, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);

	SETDBERR(dberr, 0, E_DM9232_DM0L_ALLOCATE);
	return (E_DB_ERROR);
    }

    /*
    ** Check compatability of this process with the installed logging
    ** system.  In the future we should check the version id of the
    ** logging system to make sure we are compatable.  (We will have
    ** to create a version id first).
    **
    ** Check the LG_DBINFO_MAX constant size.  This is the maximum
    ** size buffer of database information that the logging system
    ** will accept for LGadd calls.  Make sure that the buffer has
    ** room for all the information the system may need to put into it.
    ** If this size does not match then it means somebody changed the
    ** DB_MAXNAME or DB_AREA_MAX constant without increasing LG_DBINFO_MAX.
    */
    if (LG_DBINFO_MAX < sizeof(DM0L_ADDDB))
    {
	(VOID) uleFormat(NULL, E_DM9426_LGADD_SIZE_MISMATCH, &sys_err,
	    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 2,
	    0, sizeof(DM0L_ADDDB), 0, LG_DBINFO_MAX);
	SETDBERR(dberr, 0, E_DM9232_DM0L_ALLOCATE);
	return (E_DB_ERROR);
    }

    {
	if ((flag & DM0L_NOLOG) == 0)
	{
	    open_flag = 0;
	    if (flag & DM0L_ARCHIVER)
		open_flag |= LG_ARCHIVER;
	    if (flag & DM0L_FASTCOMMIT)
		open_flag |= LG_FCT;
	    if (flag & DM0L_CKPDB)
		open_flag |= LG_CKPDB;
	    if (flag & DM0L_RECOVER)
		open_flag |= LG_MASTER;
	    if (flag & DM0L_FULL_FILENAME)
		open_flag |= LG_FULL_FILENAME;
	    if (flag & DM0L_SCAN_LOGFILE)
		open_flag |= LG_IGNORE_BOF;
            if (flag & DM0L_PARTITION_FILENAMES)
                open_flag |= LG_PARTITION_FILENAMES;

	    /*
	    ** Open the log on node 'name'. In non-cluster case this is NULL
	    ** and the local log is opened
	    */
	    status = LGopen(open_flag, name, l_name, &lg_id, 0, (i4)0,
			    &sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM9012_BAD_LOG_OPEN, &sys_err,
			ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);

		if (status == LG_NOTLOADED)
		    SETDBERR(dberr, 0, E_DM0081_NO_LOGGING_SYSTEM);
		else if (status == LG_EXCEED_LIMIT)
		    SETDBERR(dberr, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
		else
		    SETDBERR(dberr, 0, E_DM9232_DM0L_ALLOCATE);
		return (E_DB_ERROR);
	    }
	}
	if (flag & DM0L_ARCHIVER)
	{
	    DM0L_ADDDB		add_info;
	    i4			len_add_info;
	    i4		db_id;
	    i4		lx_id;
	    DB_TRAN_ID		tran_id;
	    i4		event;

	    STmove((PTR)DB_ARCHIVER_INFO, ' ', DB_DB_MAXNAME,
		(PTR) &add_info.ad_dbname);
	    MEcopy((PTR)DB_INGRES_NAME, DB_OWN_MAXNAME,
		(PTR) &add_info.ad_dbowner);
	    MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
	    add_info.ad_dbid = 0;
	    add_info.ad_l_root = 4;
	    len_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;
	    if (status = LGadd(lg_id, LG_NOTDB, (char *)&add_info,
				len_add_info, &db_id, &sys_err))
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 4, 0, lg_id,
		    0, "$archiver", 0, "$ingres", 0, "None");

		SETDBERR(dberr, 0, E_DM9232_DM0L_ALLOCATE);
		return (E_DB_ERROR);
	    }
	    /*	Start a transaction so that we can wait for event. */

	    STmove((PTR)DB_ALLOCATE_USER, ' ', sizeof(DB_OWN_NAME),
		    (PTR) &user_name);
	    status = LGbegin(LG_NOPROTECT, db_id, &tran_id, &lx_id,
			sizeof(DB_OWN_NAME), (char *)&user_name, 
			(DB_DIS_TRAN_ID*)NULL, &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		return (log_error(E_DM900C_BAD_LOG_BEGIN, &sys_err, db_id,
			E_DM9232_DM0L_ALLOCATE, dberr, status));
	    }

	    /*	Wait for the recovery system to come ONLINE before continuing */

	    status = LGevent(0, lx_id, LG_E_ONLINE | LG_E_IMM_SHUTDOWN, &event,
				&sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		return (log_error(E_DM900F_BAD_LOG_EVENT, &sys_err,
			LG_E_ONLINE | LG_E_IMM_SHUTDOWN,
			E_DM9232_DM0L_ALLOCATE, dberr, status));
	    }
	    if (event & LG_E_IMM_SHUTDOWN)
		return(log_error(E_DM900F_BAD_LOG_EVENT, &sys_err,
			LG_E_IMM_SHUTDOWN,
			E_DM9232_DM0L_ALLOCATE, dberr, status));

	    status = LGend(lx_id, 0, &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		return (log_error(E_DM900E_BAD_LOG_END, &sys_err, lx_id,
			E_DM9232_DM0L_ALLOCATE, dberr, status));
	    }

	    status = LGremove(db_id, &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		return (log_error(E_DM9016_BAD_LOG_REMOVE, &sys_err, db_id,
		    E_DM9232_DM0L_ALLOCATE, dberr, status));
	    }
	}
	/*
	** Make "header" an alias for the header portion of show_hdr. Get the
	** current logfile header and store it into the LCTX. If a non-zero
	** LG_ID was passed in, store it in the dmf_svcb. Otherwise, get the
	** process's logging system ID out of the dmf_svcb for use below.
	*/
	header = &show_hdr.lg_hdr_lg_header;
	if (lg_id)
	{
            show_hdr.lg_hdr_lg_id = lg_id;
	    status = LGshow(LG_A_NODELOG_HEADER, (PTR)&show_hdr,
			    sizeof(show_hdr), &length, &sys_err);
	}
	else
	{
	    lg_id = dmf_svcb->svcb_lctx_ptr->lctx_lgid;
	    status = LGshow(LG_A_HEADER, (PTR)header, sizeof(*header), &length,
            &sys_err);
	}
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    return (log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_A_HEADER,
		    E_DM9232_DM0L_ALLOCATE, dberr, status));
	}


	/*
	** Allocate a LCTX large enough to hold the configured number of
	** log blocks for buffered log reads.
	**
	** For the recovery server or jsp-y things or the archiver, allocate
	** ii.$.rcp.log.readforward_blocks,
	**
	** otherwise (a normal DBMS server) allocate
	** ii.$.rcp.log.readbackward_blocks.
	*/
	if ( dmf_svcb->svcb_status & SVCB_SINGLEUSER || flag & DM0L_RECOVER )
	    showType = LG_S_RFBLOCKS;
	else
	    showType = LG_S_RBBLOCKS;

	status = LGshow(showType, (PTR)&logBlocks, sizeof(logBlocks), &length,
			&sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    return (log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, showType,
		    E_DM9232_DM0L_ALLOCATE, dberr, status));
	}
	CacheSize = header->lgh_size * logBlocks;
    }

    for (;;)
    {
	/*  Allocate the LCTX. */

	align = dmf_svcb->svcb_directio_align;
	if (align < DI_RAWIO_ALIGN)
	    align = DI_RAWIO_ALIGN;
	status = dm0m_allocate(
		sizeof(DMP_LCTX) + LG_MAX_CTX + LG_MAX_RSZ + CacheSize + align,
		0, LCTX_CB, LCTX_ASCII_ID, (char *)&dmf_svcb,
	    (DM_OBJECT **)&lctx, dberr);
	if (status != E_DB_OK)
	    break;

	lctx->lctx_area = (char *)lctx + sizeof(DMP_LCTX);
	lctx->lctx_l_area = LG_MAX_CTX + LG_MAX_RSZ + CacheSize + align;
	lctx->lctx_lgid = lg_id;
	lctx->lctx_status = 0;
	lctx->lctx_bksz = header->lgh_size;
	lctx->lctx_lg_header = *header;

	if (lctx_ptr)
	{
	    *lctx_ptr = lctx;
	}
	else
	{
	    dmf_svcb->svcb_lctx_ptr = lctx;
	}
	if (flag & DM0L_NOLOG)
	{
	    lctx->lctx_status |= LCTX_NOLOG;
	}
	else
	{
	    lctx->lctx_eof.la_sequence = header->lgh_end.la_sequence;
	    lctx->lctx_eof.la_block    = header->lgh_end.la_block;
	    lctx->lctx_eof.la_offset   = header->lgh_end.la_offset;
	    lctx->lctx_bof.la_sequence = header->lgh_begin.la_sequence;
	    lctx->lctx_bof.la_block    = header->lgh_begin.la_block;
	    lctx->lctx_bof.la_offset   = header->lgh_begin.la_offset;
	    lctx->lctx_cp.la_sequence  = header->lgh_cp.la_sequence;
	    lctx->lctx_cp.la_block     = header->lgh_cp.la_block;
	    lctx->lctx_cp.la_offset    = header->lgh_cp.la_offset;
	    lctx->lctx_bksz            = header->lgh_size;
	    lctx->lctx_bkcnt           = header->lgh_count;
	}
	/* Get the cluster id. */

	status = LGshow(LG_A_NODEID, (PTR)&lctx->lctx_node_id,
			sizeof(lctx->lctx_node_id),
			&length, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL,
		    err_code, 0);
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 2, 0,
		    LG_A_NODEID, 0, 0);
	    SETDBERR(dberr, 0, E_DM9232_DM0L_ALLOCATE);/* FIX ME: NEED A New msg */ 
	    break;
	}

	lctx->lctx_dbid = 0;
	lctx->lctx_lxid = 0;

	/*
	**	Start a transaction so that recovery processing can use this
	**	logging system context block to read logfile records outside
	**	of the context of any particular transaction. For example, the
	**	analysis and redo passes made by dmfrecover.c code need to be
	**	able to read the logfile, so they need a transaction handle.
	*/
	if (flag & DM0L_RECOVER)
	{
	    DM0L_ADDDB		add_info;
	    i4			len_add_info;
	    i4		db_id;
	    i4		lx_id;
	    DB_TRAN_ID		tran_id;

	    STmove((PTR)DB_RECOVERY_INFO, ' ', DB_DB_MAXNAME,
			(PTR) &add_info.ad_dbname);
	    MEcopy((PTR)DB_INGRES_NAME, DB_OWN_MAXNAME,
			(PTR) &add_info.ad_dbowner);
	    MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
	    add_info.ad_dbid = 0;
	    add_info.ad_l_root = 4;
	    len_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;
	    if (status = LGadd(lg_id, LG_NOTDB, (char *)&add_info,
				len_add_info, &db_id, &sys_err))
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		(VOID) uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 4, 0, lg_id,
		    0, DB_RECOVERY_INFO, 0, DB_INGRES_NAME, 0, "None");

		SETDBERR(dberr, 0, E_DM9232_DM0L_ALLOCATE);
		return (E_DB_ERROR);
	    }

	    MEcopy((PTR)"$log_reader_transaction         ",
			    sizeof(DB_OWN_NAME), (PTR) &user_name);
	    status = LGbegin(LG_NOPROTECT, db_id, &tran_id, &lx_id,
			sizeof(DB_OWN_NAME), (char *)&user_name,
			(DB_DIS_TRAN_ID*)NULL, &sys_err);
	    if (status)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		return (log_error(E_DM900C_BAD_LOG_BEGIN, &sys_err, db_id,
			E_DM9232_DM0L_ALLOCATE, dberr, status));
	    }

	    lctx->lctx_dbid = db_id;
	    lctx->lctx_lxid = lx_id;
	}
	return (E_DB_OK);
    }

    if (lg_id)
    {
	status = LGclose(lg_id, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    return (log_error(E_DM900D_BAD_LOG_CLOSE, &sys_err, lg_id,
		    E_DM9232_DM0L_ALLOCATE, dberr, status));
	}
	else
	{
	    (VOID) uleFormat(dberr, 0, NULL, ULE_LOG, NULL,
				(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    return (E_DB_ERROR);
	}
    }
    else
    {
	(VOID) uleFormat(dberr, 0, NULL, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	return (E_DB_ERROR);
    }
}

/*{
** Name: dm0l_deallocate    - Deallocate the LCTX
**
** Description:
**      This routine close the logging system and deallocate the LCTX.
**
** Inputs:
**      lctx                            Log context.
** Outputs:
**      err_code                        Reason for error return status.
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
**      28-jul-1986 (Derek)
**          Created for Jupiter.
**	26-oct-1992 (rogerk)
**	    Fixed log_error parameter cast to match function prototype.
**	26-apr-1993 (bryanp)
**	    If we created a logreader transaction for this logfile context
**	    block, then end the transaction and remove the database as part of
**	    deallocating the logfile context block.
[@history_template@]...
*/
DB_STATUS
dm0l_deallocate(
    DMP_LCTX            **lctx,
    DB_ERROR            *dberr)
{
    DMP_LCTX		*l = *lctx;
    DB_STATUS		status;
    CL_ERR_DESC		sys_err;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    if (l->lctx_lxid)
    {
	/* Remove default transaction and database contexts */
	status = LGend(l->lctx_lxid, 0, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    return (log_error(0, &sys_err,
		    (i4)l->lctx_lxid,
		    E_DM9233_DM0L_DEALLOCATE, dberr, status));
	}
    }

    if (l->lctx_dbid)
    {
	status = LGremove(l->lctx_dbid, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    return (log_error(0, &sys_err,
		    (i4)l->lctx_dbid,
		    E_DM9233_DM0L_DEALLOCATE, dberr, status));
	}
    }

    /*	Close the log file. */

    if ((l->lctx_status & LCTX_NOLOG) == 0)
    {
	status = LGclose(l->lctx_lgid, &sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    return (log_error(E_DM900D_BAD_LOG_CLOSE, &sys_err,
		    (i4)l->lctx_lgid,
		    E_DM9233_DM0L_DEALLOCATE, dberr, status));
	}
    }

    /*	Deallocate the LCTX. */

    dm0m_deallocate((DM_OBJECT **)lctx);
    *lctx = NULL;

    return (E_DB_OK);
}

/*{
** Name: dm0l_bcp	- Write a begin consistency point log record.
**
** Description:
**      This routine formats and writes a begin consistency point log record.
**	It is assumed that the recovery process is the only process
**	that will make this call.
**
** Inputs:
**      lg_id                           Log transaction identifier.
**	bcp_la				CP address
**	bcp_lsn				LSN of BCP log record
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      12-may-1988 (EdHsu)
**          Created for RoadRunner.
**	30-sep-1988 (rogerk)
**	    Added LG_FIRST flag to the LGwrite calls in dm0l_bcp.
**	    This is necessary since there are no begin/end transaction
**	    records surrounding a dm0l_bcp record.
**	19-nov-1990 (bryanp)
**	    If an LG call fails in this routine, report the error and return
**	    E_DB_ERROR to our caller. An error writing a CP is a VERY serious
**	    error.
**	11-jan-1992 (jnash)
**	    Reduced logging project.  Add LGreserve calls.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed ECP to point back to the BCP via the LSN rather than a
**		log address.
**	    Changed BCP log records to contain external database id's rather
**		than the LG db_id (which is only temporary).
**	    Made CP records no longer be marked journaled.  While never
**		actually copied to the journal files, they were previously
**		marked journaled to make sure the ACP looked at them.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (rogerk)
**	    Add user name to transaction information.
**	06-mar-1996 (stial01 for bryanp)
**	    Dynamically allocate record buffers in dm0l_bcp.
**	02-Jan-1997 (jenjo02)
**	    Corrected size of DM0L_BCP CP_TX records being
**	    written to the log. sizeof(struct db) wastes 144 bytes per txn, also
**	    causes LGwrite() to copy that many excess bytes per txn to the
**	    log buffer, a complete waste of time. sizeof(struct tx) is the
**	    correct value.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
**       1-May-2006 (hanal04) Bug 116059
**          Flag the first log record for this BCP with CP_FIRST so that
**          dmr_get_cp() can safely ascertain the CP window.
**	11-Sep-2006 (jonj)
**	    Add transaction's lock list id to BCP tx_lkid. With LOGFULL_COMMIT
**	    protocols, the transaction id may change over time, but the
**	    lock list id won't.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	07-Feb-2008 (hanje04)
**	    SIR 119978
**	    Defining true and false locally causes compiler errors where 
**	    they are already defined. Change the variable names to tru and
**	    fals to avoid this
*/
DB_STATUS
dm0l_bcp(
    i4	    lg_id,
    LG_LA	    *bcp_la,
    LG_LSN	    *bcp_lsn,
    DB_ERROR	    *dberr)
{
    DB_STATUS           status;
    CL_ERR_DESC 		sys_err;
    i4		tru = 1;
    i4		fals = 0;
    i4		length;
    i4		firstwrite = 0;
    i4		i;
    i4		loc_error;
    LG_DATABASE		db;
    LG_TRANSACTION	tr;
    LG_LRI		lri;
    LG_OBJECT		lg_object;
    DM0L_BCP		*bcp;
    i4 		count;
    DM_OBJECT		*mem_ptr;
    i4		size_bcp;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    status = dm0m_allocate(LG_MAX_RSZ + sizeof(DMP_MISC),
	    (i4)0, (i4)MISC_CB,
	    (i4)MISC_ASCII_ID, (char *)NULL, &mem_ptr, dberr);
    if (status != OK)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		0, LG_MAX_RSZ + sizeof(DMP_MISC));
	SETDBERR(dberr, 0, E_DM9235_DM0L_BCP);
	return (E_DB_ERROR);
    }

    /*	Initialize the BCP record. */

    bcp = (DM0L_BCP*) ((char *)mem_ptr + sizeof(DMP_MISC));
    ((DMP_MISC*)mem_ptr)->misc_data = (char*)bcp;
    bcp->bcp_header.type = DM0LBCP;
    bcp->bcp_header.flags = 0;
    bcp->bcp_type = CP_DB;
    bcp->bcp_flag = CP_FIRST;
    bcp->bcp_count = 0;

    /* 
    ** Compute and save the size of a BCP without regard to
    ** attached db or tx subtypes. This allows us to
    ** accurately calculate the size of the resulting
    ** log records.
    */
    size_bcp = sizeof(DM0L_BCP) - sizeof(bcp->bcp_subtype);

    /*	Make sure logging system is stalling. */

    status = LGalter(LG_A_BCPDONE, (PTR)&fals, sizeof(fals), &sys_err);
    if (status)
    {
	dm0m_deallocate(&mem_ptr);
	return (log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_BCPDONE,
			E_DM9235_DM0L_BCP, dberr, status));
    }

    /*	Get address of last log record written. */

    status = LGshow(LG_A_EOF, (PTR)bcp_la, sizeof(*bcp_la), &length, &sys_err);
    if (status)
    {
	dm0m_deallocate(&mem_ptr);
	return (log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_A_EOF,
		    E_DM9235_DM0L_BCP, dberr, status));
    }

    /*	Get the list of active databases. */

    length = 0;
    i = 0;
    count = (LG_MAX_RSZ - size_bcp) / sizeof(struct db);
    firstwrite = 0;

    for (;;)
    {
	status = LGshow(LG_N_DATABASE, (PTR)&db, sizeof(db), &length, &sys_err);

	if (status)
	{
	    dm0m_deallocate(&mem_ptr);
	    return (log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_N_DATABASE,
			    E_DM9235_DM0L_BCP, dberr, status));
	}

	if (length == 0)
	    break;

	bcp->bcp_subtype.type_db[i].db_id = db.db_id;
	bcp->bcp_subtype.type_db[i].db_status = db.db_status;
	MEcopy((PTR) db.db_buffer, db.db_l_buffer,
		(PTR) &bcp->bcp_subtype.type_db[i].db_name);
	bcp->bcp_subtype.type_db[i].db_ext_dbid = db.db_database_id;
	i++;
	if (i >= count)
	{
	    bcp->bcp_count = i;
	    bcp->bcp_header.length = size_bcp + i * sizeof(struct db);

	    lg_object.lgo_size = bcp->bcp_header.length;
	    lg_object.lgo_data = (PTR) bcp;

	    status = LGwrite(LG_FIRST | LG_NOT_RESERVED, lg_id, (i4)1, &lg_object, &lri,
		&sys_err);
	    if (!firstwrite)
	    {
		firstwrite = 1;
		*bcp_lsn = lri.lri_lsn;
                bcp->bcp_flag = 0;
	    }
	    if (status)
	    {
		dm0m_deallocate(&mem_ptr);
		return (log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
			    E_DM9235_DM0L_BCP, dberr, status));
	    }

	    if (DMZ_SES_MACRO(11))
		dmd_logtrace(&bcp->bcp_header, 0);

	    i = 0;
	}
    }

    if (i && i < count)
    {
	bcp->bcp_count = i;
	bcp->bcp_header.length = size_bcp + i * sizeof(struct db);

	lg_object.lgo_size = bcp->bcp_header.length;
	lg_object.lgo_data = (PTR) bcp;

	status = LGwrite(LG_FIRST| LG_NOT_RESERVED, lg_id, (i4)1, &lg_object, &lri,
	    &sys_err);
	if (!firstwrite)
        {
	    firstwrite =1;
	    *bcp_lsn = lri.lri_lsn;
            bcp->bcp_flag = 0;
	}
	if (status)
	{
	    dm0m_deallocate(&mem_ptr);
	    return (log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
			    E_DM9235_DM0L_BCP, dberr, status));
	}

	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&bcp->bcp_header, 0);
    }

    /*	Get the list of active transactions. */

    /*
    ** (note from Bryan: it seems a little odd to me that the remainder of this
    ** subroutine uses "struct db" rather than "struct tx" in the maniplation
    ** of the lengths and sizes of the buffers, but I chose not to change it
    ** since I didn't really understand why it was so.)
    **
    ** (note from stial01: it actually should be sizeof bcp->bcp_subtype
    ** which is a union of struct db and struct tx. Since it is a union,
    ** the sizeof this field is the sizeof struct db which is currently larger.
    **
    ** (note from jenjo02: it really ought to be what Bryan suggested, 
    ** sizeof(struct tx). Using sizeof(struct db) wastes 144 bytes per txn
    ** and causes logging to do a lot of unecessary (and time consuming)
    ** copying of worthless memory into log buffers while the logging system
    ** is stalled waiting for CP to complete. 
    */

    length = 0;
    count = (LG_MAX_RSZ - size_bcp) / sizeof(struct tx);
    i = 0;
    bcp->bcp_type = CP_TX;

    for (;;)
    {
	status = LGshow(LG_N_TRANSACTION, (PTR)&tr, sizeof(tr),
		    &length, &sys_err);
	if (status)
	{
	    dm0m_deallocate(&mem_ptr);
	    return (log_error(E_DM9017_BAD_LOG_SHOW, &sys_err,
				LG_N_TRANSACTION,
				E_DM9235_DM0L_BCP, dberr, status));
	}

	if (length == 0)
	    break;

	/*  We are only interested in protected active transactions. */

	if ((tr.tr_status & TR_PROTECT) && (tr.tr_status & TR_ACTIVE))
	{
	    /* Copy all untranslated status bits (TX_? == TR_?) */
	    bcp->bcp_subtype.type_tx[i].tx_status = tr.tr_status;

	    MEcopy((PTR) &(tr.tr_eid), sizeof(DB_TRAN_ID),
				(PTR) &(bcp->bcp_subtype.type_tx[i].tx_tran));
	    bcp->bcp_subtype.type_tx[i].tx_dbid = tr.tr_database_id;
	    bcp->bcp_subtype.type_tx[i].tx_lkid = tr.tr_lock_id;
	    MEcopy((PTR) &(tr.tr_first), sizeof(tr.tr_first),
				(PTR) &(bcp->bcp_subtype.type_tx[i].tx_first));
	    MEcopy((PTR) &(tr.tr_last), sizeof(tr.tr_last),
				(PTR) &(bcp->bcp_subtype.type_tx[i].tx_last));
	    MEcopy((PTR) tr.tr_user_name, DB_OWN_MAXNAME,
		    (PTR) bcp->bcp_subtype.type_tx[i].tx_user_name.db_own_name);
	    i++;
	    if (i >= count)
	    {
		bcp->bcp_count = i;
		bcp->bcp_header.length = size_bcp + i * sizeof(struct tx);

		lg_object.lgo_size = bcp->bcp_header.length;
		lg_object.lgo_data = (PTR) bcp;

		status = LGwrite(LG_FIRST | LG_NOT_RESERVED, lg_id, (i4)1, &lg_object,
					&lri, &sys_err);
		if (!firstwrite)
		{
		    firstwrite = 1;
		    *bcp_lsn = lri.lri_lsn;
		}
		if (status)
		{
		    dm0m_deallocate(&mem_ptr);
		    return (log_error(E_DM9015_BAD_LOG_WRITE, &sys_err,
				lg_id,
				E_DM9235_DM0L_BCP, dberr, status));
		}

		if (DMZ_SES_MACRO(11))
		    dmd_logtrace(&bcp->bcp_header, 0);

		i = 0;
	    }
	}
    }

    if (i && i < count)
    {
	bcp->bcp_count = i;
	bcp->bcp_header.length = size_bcp + i * sizeof(struct tx);

	lg_object.lgo_size = bcp->bcp_header.length;
	lg_object.lgo_data = (PTR) bcp;

	status = LGwrite(LG_FIRST | LG_NOT_RESERVED, lg_id, (i4)1, 
				&lg_object, &lri, &sys_err);
	if (!firstwrite)
        {
	    firstwrite = 1;
	    *bcp_lsn = lri.lri_lsn;
	}
	if (status)
	{
	    dm0m_deallocate(&mem_ptr);
	    return (log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
			    E_DM9235_DM0L_BCP, dberr, status));
	}

	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&bcp->bcp_header, 0);
    }

    /*	Write the end of the BCP record marker and force to disk. */

    bcp->bcp_header.length = size_bcp;
    bcp->bcp_count = 0;
    bcp->bcp_type = 0;
    bcp->bcp_flag = CP_LAST;

    lg_object.lgo_size = bcp->bcp_header.length;
    lg_object.lgo_data = (PTR) bcp;

    status = LGwrite((LG_FORCE | LG_FIRST | LG_NOT_RESERVED), lg_id, (i4)1, &lg_object,
	&lri, &sys_err);
    if (!firstwrite)
    {
	firstwrite = 1;
	*bcp_lsn = lri.lri_lsn;
    }
    if (status)
    {
	dm0m_deallocate(&mem_ptr);
	return (log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
		    E_DM9235_DM0L_BCP, dberr, status));
    }

    if (DMZ_SES_MACRO(11))
	dmd_logtrace(&bcp->bcp_header, 0);

    dm0m_deallocate(&mem_ptr);

    /*	Notify logging system of successful BCP. */

    status = LGalter(LG_A_BCPDONE, (PTR)&tru, sizeof(tru), &sys_err);
    if (status)
	return (log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_BCPDONE,
			E_DM9235_DM0L_BCP, dberr, status));

    return (E_DB_OK);
}

/*
** Name: dm0l_ecp	- Write an end consistency point log record.
**
** Description:
**      This routine formats and writes an end consistency point log record.
**	It is assumed that the recovery process is the only process
**	that will make this call.
**
** Inputs:
**      lg_id                           Log transaction identifier.
**	bcp_la				BCP address
**	bcp_lsn				LSN of the BCP log record
**
** Outputs:
**      err_code                        Reason for error return status.
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
**      12-may-1988 (EdHsu)
**          Created for RoadRunner.
**	14-aug-1988 (roger)
**	    Shamelessly trick compiler into allowing a DM_LOG_ADDR to be
**	    assigned to a LG_LA (bcpx_la).  This was already being done
**	    just below, for bof.  A justification for the existence of
**	    the DM_LOG_ADDR type is in order.   At the very least, it should
**	    contain a LG_LA member.
**	30-sep-1988 (rogerk)
**	    Added LG_FIRST flag to the LGwrite call in dm0l_ecp.
**	    This is necessary since there are no begin/end transaction
**	    records surrounding a dm0l_ecp record.
**	19-nov-1990 (bryanp)
**	    If an LG call fails in this routine, log the error and return
**	    E_DB_ERROR. A failure writing a CP is a VERY serious error.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.  Space for
**	    CP log records is reserved just prior to the LGwrite.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed ECP to point back to the BCP via the LSN rather than a
**		log address.
**	    Made CP records no longer be marked journaled.  While never
**		actually copied to the journal files, they were previously
**		marked journaled to make sure the ACP looked at them.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Added back ecp_begin_la which holds the Log Address of the Begin
**	    Consistency Point record. Now the ECP has the LA and LSN of the BCP.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-mar-1996 (stial01 for bryanp)
**	    Declare a simple DM0L_ECP record on the stack, rather than a
**		buffer of size LG_MAX_RSZ, since the DM0L_ECP record is a
**		fixed-size object and can be simply allocated. Furthermore,
**		if LG_MAX_RSZ is large, using an LG_MAX_RSZ buffer can overflow
**		the stack.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
**	07-Feb-2008 (hanje04)
**	    SIR 119978
**	    Defining true and false locally causes compiler errors where 
**	    they are already defined. Change the variable names to tru and
**	    fals to avoid this
*/
DB_STATUS
dm0l_ecp(
    i4         lg_id,
    LG_LA	    *bcp_la,
    LG_LSN	    *bcp_lsn,
    DB_ERROR	    *dberr)
{
    DB_STATUS           status;
    CL_ERR_DESC		sys_err;
    i4		tru = 1;
    i4		fals = 0;
    i4		length;
    i4		loc_error;
    LG_LA		bof;
    LG_LRI		lri;
    LG_OBJECT		lg_object;
    DM0L_ECP		*ecp;
    DM0L_ECP		ecp_rec;

    CLRDBERR(dberr);

    /*	Initialize the ECP record. */

    ecp = (DM0L_ECP*) &ecp_rec;
    ecp->ecp_header.length = sizeof(*ecp);
    ecp->ecp_header.type = DM0LECP;
    ecp->ecp_header.flags = 0;
    STRUCT_ASSIGN_MACRO(*bcp_lsn, ecp->ecp_begin_lsn);
    STRUCT_ASSIGN_MACRO(*bcp_la, ecp->ecp_begin_la);

    /*	Make sure logging system is stalling. */

    status = LGalter(LG_A_ECPDONE, (PTR)&fals, sizeof(fals), &sys_err);
    if (status)
	return (log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_ECPDONE,
			E_DM9343_DM0L_ECP, dberr, status));

    /* Get the BOF. */

    status = LGshow(LG_A_BOF, (PTR)&bof, sizeof(bof), &length, &sys_err);
    if (status)
	return (log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_A_BOF,
			E_DM9343_DM0L_ECP, dberr, status));

    /*	Write the end of the ECP record marker and force to disk. */

    STRUCT_ASSIGN_MACRO(bof, *(LG_LA *)&ecp->ecp_bof);
    lg_object.lgo_size = ecp->ecp_header.length;
    lg_object.lgo_data = (PTR) ecp;

    status = LGwrite((LG_FORCE | LG_FIRST | LG_NOT_RESERVED), lg_id, (i4)1, &lg_object,
	&lri, &sys_err);
    if (status)
	return (log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
		    E_DM9343_DM0L_ECP, dberr, status));

    if (DMZ_SES_MACRO(11))
	dmd_logtrace(&ecp->ecp_header, 0);

    /*
    **	IN a initialized log file, the ecp address will be null.
    **	The first ECP record will cause the LG code to set the beginning address
    **	to before the first record just written.  Here we get the value of the
    **	beginning log address and use this as our ECP address.
    */

    if (bcp_la->la_block == 0)
    {
	STRUCT_ASSIGN_MACRO(bof, *bcp_la);
    }

    /*	Update the last ECP address. */

    status = LGalter(LG_A_CPA, (PTR)bcp_la, sizeof(*bcp_la), &sys_err);
    if (status)
	return (log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_CPA,
			E_DM9343_DM0L_ECP, dberr, status));

    /*	Force log header to disk. */

    status = LGforce(LG_HDR, lg_id, 0, 0, &sys_err);
    if (status)
	return (log_error(E_DM9010_BAD_LOG_FORCE, &sys_err, lg_id,
		    E_DM9343_DM0L_ECP, dberr, status));

    /*	Notify logging system of successful ECP. */

    status = LGalter(LG_A_ECPDONE, (PTR)&tru, sizeof(tru), &sys_err);
    if (status)
	return (log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_ECPDONE,
			E_DM9343_DM0L_ECP, dberr, status));

    return (E_DB_OK);
}

/*{
** Name: dm0l_SM1_rename - Write a SYSMOD file rename record.
**
** Description:
**      This routine writes a sysmod file rename record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**      tbl_id                          Table id of table modifying.
**	path				Pointer to path.
**	l_path				Length of path name.
**	oldfile				Pointer to old file.
**	tempfile			Pointer to temp file.
**	newfile				Pointer to new file.
**	renamefile			Pointer to rename of new file.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	07-nov-1986 (jennifer)
**          Created for Jupiter.
**	01-dec-1992 (jnash)
**	    Reduced logging project.  Add prev_lsn param, CLR handling.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	23-aug-1993 (rogerk)
**	    Made SM1 log record be non-redo: we can't roll forward the
**	    updates made on behalf of the sysmod operation after swapping
**	    over the new system catalog file.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**      28-may-1998 (stial01)
**          dm0l_sm1_rename() Support VPS system catalogs.
*/
DB_STATUS
dm0l_sm1_rename(
    i4             lg_id,
    i4		flag,
    DB_TAB_ID           *tbl_id,
    DB_TAB_ID           *tmp_id,
    DM_PATH		*path,
    i4		l_path,
    DM_FILE		*oldfile,
    DM_FILE		*tempfile,
    DM_FILE		*newfile,
    DM_FILE		*renamefile,
    LG_LSN		*prev_lsn,
    LG_LSN		*lsn,
    i4			oldpgtype,
    i4			newpgtype,
    i4			oldpgsize,
    i4			newpgsize,
    DB_ERROR            *dberr)
{
    DM0L_SM1_RENAME    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.sm1_header.length = sizeof(DM0L_SM1_RENAME);
    log.sm1_header.type = DM0LSM1RENAME;
    log.sm1_header.flags = flag | DM0L_NONREDO;
    log.sm1_path = *path,
    log.sm1_l_path = l_path;
    log.sm1_tbl_id = *tbl_id;
    log.sm1_tmp_id = *tmp_id;
    log.sm1_oldname = *oldfile;
    log.sm1_tempname = *tempfile;
    log.sm1_newname = *newfile;
    log.sm1_rename = *renamefile;
    log.sm1_oldpgtype = oldpgtype;
    log.sm1_newpgtype = newpgtype;
    log.sm1_oldpgsize = oldpgsize;
    log.sm1_newpgsize = newpgsize;
    if (flag & DM0L_CLR)
	log.sm1_header.compensated_lsn = *prev_lsn;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(0, lg_id, 2, 2 * sizeof(DM0L_SM1_RENAME), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM9236_DM0L_SM1_RENAME, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.sm1_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_SM1_RENAME));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
		E_DM9236_DM0L_SM1_RENAME, dberr, status));
}

/*{
** Name: dm0l_sm2_config- Write a SYSMOD update config file record.
**
** Description:
**      This routine writes a sysmod update config file
**      record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**      tbl_id                          Table id of table modifying.
**	oldrel			        Pointer to old relation record.
**	newrel				Pointer to new relation record.
**	oldidx			        Pointer to old relation record
**                                      for relation index.  Only given
**                                      if modifying the relation record
**                                      (i.e. not given for attribute or
**                                      indexes).
**	newidx			        Pointer to old relation record
**                                      for relation index.  Only given
**                                      if modifying the relation record
**                                      (i.e. not given for attribute or
**                                      indexes).
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	07-nov-1986 (jennifer)
**          Created for Jupiter.
**	01-dec-1992 (jnash)
**	    Reduced logging project.  Add CLR handling and prev_lsn param,
**	    log only change to relprim.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
*/
DB_STATUS
dm0l_sm2_config(
    i4             lg_id,
    i4		flag,
    DB_TAB_ID           *tbl_id,
    DMP_RELATION        *oldrel,
    DMP_RELATION        *newrel,
    DMP_RELATION        *oldidx,
    DMP_RELATION        *newidx,
    LG_LSN		*prev_lsn,
    LG_LSN		*lsn,
    DB_ERROR            *dberr)
{
    DM0L_SM2_CONFIG log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.sm2_header.length = sizeof(DM0L_SM2_CONFIG);
    log.sm2_header.type = DM0LSM2CONFIG;
    log.sm2_header.flags = flag;
    STRUCT_ASSIGN_MACRO(*tbl_id, log.sm2_tbl_id);
    STRUCT_ASSIGN_MACRO(*oldrel, log.sm2_oldrel);
    STRUCT_ASSIGN_MACRO(*newrel, log.sm2_newrel);
    STRUCT_ASSIGN_MACRO(*oldidx, log.sm2_oldidx);
    STRUCT_ASSIGN_MACRO(*newidx, log.sm2_newidx);
    if (flag & DM0L_CLR)
	log.sm2_header.compensated_lsn = *prev_lsn;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(0, lg_id, 2, 2 * sizeof(DM0L_SM2_CONFIG), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM9237_DM0L_SM2_CONFIG, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.sm2_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_SM2_CONFIG));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
		E_DM9237_DM0L_SM2_CONFIG, dberr, status));
}

/*{
** Name: dm0l_sm0_closepurge - Write SYSMOD load completion log record.
**
** Description:
**	This routine writes a DM0L_SM0_CLOSEPURGE log record to indicate that
**	sysmod has completed the load of the temporary table (iirtemp) which is
**	to be swapped in as the new core catalog. This log record is used at
**	rollforward time to know when the temporary table should be closed and
**	purged, since it is about to be renamed and swapped with the core
**	catalog it replaces.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**      tbl_id                          Table id of table being loaded
**					(iirtemp).
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	22-jul-1991 (bryanp)
**          Created for fix to bug B38527.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
*/
DB_STATUS
dm0l_sm0_closepurge(
    i4             lg_id,
    i4		flag,
    DB_TAB_ID           *tbl_id,
    LG_LSN		*lsn,
    DB_ERROR            *dberr)
{
    DM0L_SM0_CLOSEPURGE		log;
    LG_OBJECT			lg_object;
    LG_LRI	    lri;
    DB_STATUS	    		status;
    CL_ERR_DESC	    		error;

    CLRDBERR(dberr);

    log.sm0_header.length = sizeof(DM0L_SM0_CLOSEPURGE);
    log.sm0_header.type = DM0LSM0CLOSEPURGE;
    log.sm0_header.flags = flag;
    STRUCT_ASSIGN_MACRO(*tbl_id, log.sm0_tbl_id);

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(LG_NOT_RESERVED, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.sm0_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
		E_DM9C09_DM0L_SM0_CLOSEPURGE, dberr, status));
}

/*{
** Name: dm0l_location- Write a add location record.
**
** Description:
**      This routine writes an add location
**      record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	journal				Flags: DM0L_JOURNAL.
**      type                            Type of location.
**      name                            Name of location.
**	l_extent		        Length of extent.
**	extent 				Extent path name.
**
** Outputs:
**	err_code			Reason for error return status.
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
**	07-nov-1986 (jennifer)
**          Created for Jupiter.
**	30-nov-1992 (jnash)
**	    Reduced logging project.  Add CLR handling, change "journal" to
**	    "flag" param,  add prev_lsn.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	12-mar-1999 (nanpr01)
**	    Productize raw location.
*/
DB_STATUS
dm0l_location(
    i4             lg_id,
    i4             flag,
    i4		type,
    DB_LOC_NAME         *name,
    i4             l_extent,
    DM_PATH             *extent,
    i4		   raw_start,
    i4		   raw_blocks,
    i4		   raw_total_blocks,
    LG_LSN		*prev_lsn,
    DB_ERROR            *dberr)
{
    LG_LRI	    lri;
    DM0L_LOCATION   log;
    LG_OBJECT	    lg_object;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.loc_header.length = sizeof(DM0L_LOCATION);
    log.loc_header.type = DM0LLOCATION;
    log.loc_header.flags = flag;
    log.loc_extent = *extent;
    log.loc_name = *name;
    log.loc_type   = type;
    log.loc_l_extent   = l_extent;
    log.loc_raw_start = raw_start;
    log.loc_raw_blocks = raw_blocks;
    log.loc_raw_total_blocks = raw_total_blocks;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;
    if (flag & DM0L_CLR)
	log.loc_header.compensated_lsn = *prev_lsn;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(0, lg_id, 2, 2 * sizeof(DM0L_LOCATION), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM9277_DM0L_LOCATION, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.loc_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_LOCATION));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
		E_DM9277_DM0L_LOCATION, dberr, status));
}

/*{
** Name: dm0l_del_location- Write a delete location record.
**
** Description:
**      This routine writes a delete location
**      record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	journal				Flags: DM0L_JOURNAL.
**      type                            Type of location.
**      name                            Name of location.
**	l_extent		        Length of extent.
**	extent 				Extent path name.
**
** Outputs:
**	err_code			Reason for error return status.
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
**	29-apr-2004 (gorvi01)
**          Created for Jupiter.
*/
DB_STATUS
dm0l_del_location(
    i4             lg_id,
    i4             flag,
    i4		type,
    DB_LOC_NAME         *name,
    i4             l_extent,
    DM_PATH             *extent,
    LG_LSN		*prev_lsn,
    DB_ERROR            *dberr)
{
    LG_LRI	    lri;
    DM0L_DEL_LOCATION   log;
    LG_OBJECT	    lg_object;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.loc_header.length = sizeof(DM0L_DEL_LOCATION);
    log.loc_header.type = DM0LDELLOCATION;
    log.loc_header.flags = flag;
    log.loc_extent = *extent;
    log.loc_name = *name;
    log.loc_type   = type;
    log.loc_l_extent   = l_extent;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;
    if (flag & DM0L_CLR)
	log.loc_header.compensated_lsn = *prev_lsn;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(0, lg_id, 2, 2 * sizeof(DM0L_DEL_LOCATION), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM9277_DM0L_LOCATION, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.loc_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_DEL_LOCATION));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
		E_DM9277_DM0L_LOCATION, dberr, status));
}


/*{
** Name: dm0l_ext_alter - Log alteration of extent info in config file
**
** Description:
**      This routine adds an extent alteration log record to the log file.
**	We log the before and after values of the bit field indicating the
**	extent type (see the DMP_LOC_ENTRY field called "flags" for values)
**	along with the name of the location.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_EXT_ALTER
**      otype                           Old type of location.
**      ntype                           New type of location.
**      name                            Name of location.
**
** Outputs:
**	err_code			Reason for error return status.
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
**	15-sep-93 (jrb)
**	    Created for MLSorts.
*/
DB_STATUS
dm0l_ext_alter(
    i4             lg_id,
    i4             flag,
    i4		otype,
    i4		ntype,
    DB_LOC_NAME         *name,
    LG_LSN		*prev_lsn,
    DB_ERROR            *dberr)
{
    LG_LRI	        lri;
    DM0L_EXT_ALTER      log;
    LG_OBJECT	    	lg_object;
    DB_STATUS	    	status;
    CL_ERR_DESC	    	error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.ext_header.length = sizeof(DM0L_EXT_ALTER);
    log.ext_header.type = DM0LEXTALTER;
    log.ext_header.flags = flag;
    log.ext_lname  = *name;
    log.ext_otype = otype;
    log.ext_ntype = ntype;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;
    if (flag & DM0L_CLR)
	log.ext_header.compensated_lsn = *prev_lsn;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(0, lg_id, 2, 2 * sizeof(DM0L_EXT_ALTER), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM929E_DM0L_EXT_ALTER, dberr, status));
	}
    }

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.ext_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_EXT_ALTER));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
		E_DM929E_DM0L_EXT_ALTER, dberr, status));
}

/*{
** Name: dm0l_setbof - Write a set beginning of log record.
**
** Description:
**      This routine writes an beginning of log record
**      to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	new_bof				Pointer to new log address.
**
** Outputs:
**	err_code			Reason for error return status.
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
**	22-may-1987 (Derek)
**          Created for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added LG_FIRST flag to the LGwrite call in dm0l_setbof.
**	    This is necessary since there are no begin/end transaction
**	    records surrounding a dm0l_setbof record.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
*/
DB_STATUS
dm0l_setbof(
    i4             lg_id,
    LG_LA	    *old_bof,
    LG_LA	    *new_bof,
    DB_ERROR	    *dberr)
{
    LG_LRI	    lri;
    DM0L_SETBOF	    log;
    LG_OBJECT	    lg_object;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.sb_header.length = sizeof(DM0L_SETBOF);
    log.sb_header.type = DM0LSETBOF;
    log.sb_header.flags = 0;
    log.sb_oldbof = *old_bof;
    log.sb_newbof = *new_bof;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite((LG_FIRST | LG_FORCE | LG_NOT_RESERVED), lg_id, (i4)1, &lg_object,
	&lri, &error);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.sb_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
		E_DM9278_DM0L_SETBOF, dberr, status));
}

/*{
** Name: dm0l_jnleof - Write a set journal eof record.
**
** Description:
**      This routine writes a set journal eof record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	db_id				Database identifier.
**	dbname				Database name.
**	dbowner				Database owner.
**	l_extent		        Length of extent.
**	extent 				Extent path name.
**
** Outputs:
**	err_code			Reason for error return status.
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
**	22-may-1987 (Derek)
**          Created for Jupiter.
**	30-sep-1988 (rogerk)
**	    Added LG_FIRST flag to the LGwrite call in dm0l_jnleof.
**	    This is necessary since there are no begin/end transaction
**	    records surrounding a dm0l_jnleof record.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
*/
DB_STATUS
dm0l_jnleof(
    i4         lg_id,
    i4	    db_id,
    DB_DB_NAME	    *dbname,
    DB_OWN_NAME	    *dbowner,
    i4         l_root,
    DM_PATH	    *root,
    i4	    node,
    i4	    old_eof,
    i4	    new_eof,
    LG_LA	    *old_la,
    LG_LA	    *new_la,
    DB_ERROR	    *dberr)
{
    LG_LRI	    lri;
    DM0L_JNLEOF	    log;
    LG_OBJECT	    lg_object;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.je_header.length = sizeof(DM0L_JNLEOF);
    log.je_header.type = DM0LJNLEOF;
    log.je_header.flags = 0;
    STRUCT_ASSIGN_MACRO(*dbname, log.je_dbname);
    STRUCT_ASSIGN_MACRO(*dbowner, log.je_dbowner);
    STRUCT_ASSIGN_MACRO(*root, log.je_root);
    log.je_dbid = db_id;
    log.je_l_root = l_root;
    log.je_oldseq = old_eof;
    log.je_newseq = new_eof;
    log.je_old_f_cp = *old_la;
    log.je_new_f_cp = *new_la;
    log.je_node = node;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite((LG_FIRST | LG_FORCE | LG_NOT_RESERVED), lg_id, (i4)1, &lg_object,
	&lri, &error);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.je_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9279_DM0L_JNLEOF, dberr, status));
}

/*{
** Name: dm0l_archive - CLuster recovery archive synchronize record.
**
** Description:
**      This routine writes log record used to sync the archiver at recovery.
**
** Inputs:
**      lg_id                           Logging system identifier.
**
** Outputs:
**	err_code			Reason for error return status.
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
**	31-jul-1987 (Derek)
**          Created for Jupiter.
**	31-jan-1994 (bryanp) B56635
**	    Remove code in dm0l_archive which caused "used before set" warnings.
*/
DB_STATUS
dm0l_archive(DB_ERROR *dberr)
{
    return (E_DB_ERROR);
#if 0
    DM0L_ARCHIVE    log;
    LG_LRI	    lri;
    LG_OBJECT	    lg_object;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    log.da_header.length = sizeof(DM0L_ARCHIVE);
    log.da_header.type = DM0LARCHIVE;
    log.da_header.flags = 0;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGCwrite(0, (i4)1, &lg_object, &lri, &error);

    if (status == OK)
	return (E_DB_OK);
    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, 0,
	E_DM927A_DM0L_ARCHIVE, dberr, status));
#endif
}

/*{
** Name: dm0l_drain	- Drain the log to the journals for this database.
**
** Description:
**      This routine forces the archiver to copy all information still in
**	the log files into the journal files for this database.
**
** Inputs:
**	dcb				Pointer to DCB.
**	flag				Either DM0L_STARTDRAIN, DM0L_ENDDRAIN.
**
** Outputs:
**	lx_id				Transaction identifier.
**      err_code                        Reason for error return.
**	Returns:
**	    E_DB_OK
**	    E_DB_INFO
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-may-1987 (Derek)
**          Created for Jupiter.
**      14-June-2001 (bolke01)
**	    Reversed logic for lock_ok
**	    (104984)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: We don't update the config file,
**	    so don't DM0C_UPDATE when closing it.
**	    Remove DM0L_ADDDB stuff, which isn't used.
**
[@history_template@]...
*/
DB_STATUS
dm0l_drain(
    DMP_DCB	    *dcb,
    DB_ERROR	    *dberr)
{
    CL_ERR_DESC	    sys_err;
    DB_STATUS	    status, local_status, local_err_code;
    LK_LKID	    lkid;
    LK_LOCK_KEY	    jnl_key;
    DM0C_CNF	    *cnf = 0;
    bool            lock_ok = FALSE;
    /* bolke01 - was TRUE */
    DB_ERROR	    local_dberr;

    CLRDBERR(dberr);

    /*	Get the journal lock in null mode. */

    jnl_key.lk_type = LK_JOURNAL;
    MEcopy((PTR)&dcb->dcb_name, LK_KEY_LENGTH, (PTR)&jnl_key.lk_key1);
    MEfill(sizeof(LK_LKID), 0, &lkid);
    status = LKrequest(LK_PHYSICAL, dmf_svcb->svcb_lock_list,
	&jnl_key, LK_X, 0, &lkid, 0, &sys_err);
    if (status == OK)
    {
	/* Open the config file and check the open count.
	** If it is not zero, some node has it open, which
	** means the node holding this lock failed with
	** the database open, so it is potentially inconsistent. */


	local_status = dm0c_open(dcb, DM0C_PARTIAL, dmf_svcb->svcb_lock_list,
		&cnf, &local_dberr);
	    
	if (local_status == E_DB_OK)
	{
	    if (cnf->cnf_dsc->dsc_open_count == 0)
		lock_ok = TRUE;
		/* bolke01 - was FALSE */
	}

	/*  Close the configuration file. */

	local_status = dm0c_close(cnf, 0, &local_dberr);

	status = LKrelease(0, dmf_svcb->svcb_lock_list, &lkid, 0, 0, &sys_err);
	if (status == OK)
	{
	    if (lock_ok == FALSE)
	    {
		return (E_DB_INFO);
	    }
	    return (E_DB_OK);
	}
    }
    SETDBERR(dberr, 0, E_DM927B_DM0L_STARTDRAIN);
    return (E_DB_ERROR);

}

/*{
** Name: logErrorFcn	- Log error message and return error.
**
** Description:
**      This routine handles errors from any LG calls.  These errors
**	are logged and the return status and error code is set.
**
** Inputs:
**	err_code			The error code to look up.
**	syserr				CL reason code.
**	p				Parameter for the error code.
**	ret_err_code			returned error cdoe.
**	stat				Logging system status code. If this
**					is set to LG_DB_INCONSISTENT, then
**					we set ret_err to
**					E_DM0100_DB_INCONSISTENT
**	FileName			File name of error source
**	LineNumber			Line number of error source
**
** Outputs:
**      ret_dberr			Returned error code is set to the passed
**					in error code (ret_err_code) in most
**					cases. If stat was LG_DB_INCONSISTENT,
**					we set ret_err to
**					E_DM0100_DB_INCONSISTENT
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-may-1986 (Derek)
**          Created for Jupiter.
**	22-jul-1991 (bryanp)
**	    Log the Logging system CL status as well as the DMF error msg.
**      10-sep-1993 (pearl)
**          Log error DM0152_DB_INCONSIST_DETAIL if database is inconsistent;
**          this error will name the database in question.  The logging has
**          been added to log_error(), which is called by all routines except
**          dm0l_opendb().  In dm0l_opendb(), call ule_format() directly;
**          the caller will log the other DMF errors.
**          Add static routine get_dbname(), which calls scf to get the
**          database name.
**	04-15-1994  (chiku)
**	    Bug56702: enable dm0l_xxxx() to return special logfull indication
**	    to its caller.
**	17-Nov-2008 (jonj)
**	    Renamed to logErrorFcn(), invoked by log_error macro passing
**	    error source information.
**	25-Nov-2008 (jonj)
**	    Removed "unused" param, i4 *ret_err changed to DB_ERROR *ret_dberr.
*/
static DB_STATUS
logErrorFcn(
    DB_STATUS	    err_code,
    CL_ERR_DESC	    *syserr,
    i4	    	    p,
    i4	    	    ret_err_code,
    DB_ERROR	    *ret_dberr,
    DB_STATUS	    stat,
    PTR		    FileName,
    i4		    LineNumber)
{
    i4			temp = 0;
    DB_DB_NAME          db_name;
    i4			local_err;

    /* Populate ret_dberr with source info */

    /* NB: all error messages will appear to have originated at this source */
    ret_dberr->err_code = stat;
    ret_dberr->err_data = 0;
    ret_dberr->err_file = FileName;
    ret_dberr->err_line = LineNumber;

    (VOID) uleFormat(ret_dberr, 0, syserr, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &local_err, 0);

    if (stat == LG_DB_INCONSISTENT)
	temp = 	E_DM0100_DB_INCONSISTENT;

    if (temp)
    {
        if (get_dbname (&db_name) < E_DB_ERROR)
        {
	    ret_dberr->err_code = E_DM0152_DB_INCONSIST_DETAIL;
            (VOID) uleFormat(ret_dberr, 0, (CL_ERR_DESC *)NULL,
                        ULE_LOG, (DB_SQLSTATE *)0, (char *)NULL, (i4)0,
                        (i4 *)NULL, &local_err, 1, sizeof (db_name), &db_name);
        }
	ret_dberr->err_code = err_code;
	(VOID) uleFormat(ret_dberr, 0, syserr, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &local_err, 1, 0, p);

	ret_dberr->err_code = ret_err_code;
	(VOID) uleFormat(ret_dberr, 0, syserr, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &local_err, 1, 0, p);

	ret_dberr->err_code = temp;
	(VOID) uleFormat(ret_dberr, 0, syserr, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &local_err, 1, 0, p);

	ret_err_code = temp;
    }
    else
    {
	ret_dberr->err_code = err_code;
	(VOID) uleFormat(ret_dberr, 0, syserr, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &local_err, 1, 0, p);

	ret_dberr->err_code = ret_err_code;
	(VOID) uleFormat(ret_dberr, 0, syserr, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &local_err, 1, 0, p);
    }

    /*
     * Bug56702: enable special logfull status be returned
     *		to the caller of dm0l_xxxx().
     */
    if (stat == E_DM9070_LOG_LOGFULL)
	ret_dberr->err_code = E_DM9070_LOG_LOGFULL;
    else
    	ret_dberr->err_code = ret_err_code;
    return (E_DB_ERROR);
}

/*{
** Name: dm0l_load - Write Load table record.
**
** Description:
**      This routine writes a load table record to the log file.
**
**	No attempt is made to perform page oriented recovery on
**	load operations.  Assumptions are that:
**	- loads are never journaled,
**	- they are NOREDO,
**	- and that UNDO code can perform logical operations on the table.
**
** Inputs:
**      lg_id                           Logging system identifier.
**      flag  				0 or DM0L_CLR
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**	location			Table location.
**	type				Table structure.
**	recreate			Whether new load file was created.
**      lastpage                        For a non-empty heap, lastpage
**                                      used before starting bulk load.  In
**                                      all other cases will be 0.
**	prev_lsn			Prev lsn if CLR.
**
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	03-aug-1987 (rogerk)
**          Created for Jupiter.
**      15-aug-1991 (jnash) merge Jennifer
**          Lastpage field added for copy enhancements.
**	27-oct-1992 (jnash)
**	    Reduced logging project.  "recreate" made a i4.
**	    STRUCT_ASSIGN_MACRO's eliminated.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (rogerk)
**	    Add reserve for log force in undo processing.
*/
DB_STATUS
dm0l_load(
    i4	    lg_id,
    i4	    flag,
    DB_TAB_ID	    *tbl_id,
    DB_TAB_NAME	    *name,
    DB_OWN_NAME	    *owner,
    DB_LOC_NAME	    *loc,
    i4	    type,
    i4	    recreate,
    i4	    lastpage,
    LG_LSN	    *prev_lsn,
    LG_LSN	    *lsn,
    DB_ERROR	    *dberr)
{
    DM0L_LOAD	    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.dul_header.length = sizeof(DM0L_LOAD);
    log.dul_header.type = DM0LLOAD;
    log.dul_header.flags = flag | DM0L_NONREDO;
    log.dul_tbl_id = *tbl_id;
    log.dul_name = *name;
    log.dul_owner = *owner;
    log.dul_location = *loc;
    log.dul_structure = type;
    log.dul_recreate = recreate;
    log.dul_lastpage = lastpage;
    if (flag & DM0L_CLR)
	log.dul_header.compensated_lsn = *prev_lsn;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    ** Include LG_RS_FORCE for log force required in undo.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2,
			    2 * sizeof(DM0L_LOAD), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM927D_DM0L_LOAD, dberr, status));
	}
    }

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.dul_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_LOAD));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
    	E_DM927D_DM0L_LOAD, dberr, status));
}

/*
** Name: dm0l_secure 	- Write a prepare to commit log record.
**
** Description:
**      This function prepares to commit a transaction by writing
**	prepare commit log record, writing all the locks held by
**	the transaction to the log file, forcing all the log records
**	of the transaction to the log file and finally alter the state
**	of the transaction to be WILLING COMMIT.
**
**	NOTE: we do not actually write ALL the locks held by the transaction
**		to the log file. In fact, we only write the WRITE-MODE locks
**		which are held by the transaction. WRITE-MODE locks are defined
**		to be those locks which are held in IX, SIX, U, or X lock modes.
**		Our caller, dmxe_secure, "knows" about this behavior and takes
**		advantage of it by calling the locking system to release all
**		read locks for the transaction after successfully preparing it.
**
** Inputs:
**      lg_id                           Log transaction identifier.
**	lock_id				Transaction lock list identifier.
**	tran_id				The physical transaction id of the
**					transaction.
**	dis_tran_id			The distributed transaction id of the
**					transaction.
** Outputs:
**      err_code                        Reason for error return status.
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
**      12-Jan-1989 (ac)
**          Created.
**	27-may-1992 (rogerk)
**	    Initialized header flags field in dm0l_secure so that prepare
**	    to commit log records are not accidentally written with the
**	    JOURNAL flag.  This would cause rollforward to fail when
**	    encountering the log records in the journal history (B43916).
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	05-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve calls.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	06-mar-1996 (stial01 for bryanp)
**	    Added comment to function header about read locks vs. write locks.
**	06-mar-1996 (stial01 for bryanp)
**	    Manage the log record buffer dynamically, since LG_MAX_RSZ may be
**		enormous.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
**	15-Dec-1999 (jenjo02)
**	    SHARED log transaction support. Do the secure only if we're the
**	    last HANDLE to issue the prepare to commit.
**	30-Apr-2002 (jenjo02)
**	    Moved dm0p_force_pages() to here from dmxe_secure().
**	    Must wait to force transaction's pages until all
**	    HANDLE transactions have prepared and the global
**	    transaction is ready to be put in WILLING COMMIT
**	    to avoid DM9C01 page-still-fixed failures.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
DB_STATUS
dm0l_secure(
    i4	    lg_id,
    i4	    lock_id,
    DB_TRAN_ID	    *tran_id,
    DB_DIS_TRAN_ID  *dis_tran_id,
    DB_ERROR	    *dberr)
{
    DB_STATUS           status;
    CL_ERR_DESC 		sys_err;
    i4		length;
    i4		i;
    LG_LRI		lri;
    LG_OBJECT		lg_object;
    DM0L_P1		*p1;
    char		info_buffer[16384];
    char		*lk_buffer;
    i4			buf_size;
    i4 		count;
    i4		context;
    i4		lk_count;
    LG_TRANSACTION	tr;
    LG_CONTEXT		ctx;
    DM_OBJECT		*mem_ptr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    lk_buffer = (PTR) ME_ALIGN_MACRO(info_buffer, sizeof(ALIGN_RESTRICT));
    buf_size = sizeof(info_buffer) - (lk_buffer - info_buffer);

    ctx.lg_ctx_dis_eid = *dis_tran_id;
    ctx.lg_ctx_id = lg_id;

    {
	/*
	** All affiliated transactions have closed their tables and
	** unfixed any related pages. Now it's safe to release 
	** any pages touched by the global transaction from the
	** transaction page list. If not running Fast Commit, then
	** force the dirty pages to disk.
	*/
	status = dm0p_force_pages(lock_id, tran_id, lg_id, DM0P_TOSS, dberr);
	if ( status )
	    return(status);

	/* Get the information of the (SHARED) transaction. */

	MEcopy((char *)&lg_id, sizeof(lg_id), (char *)&tr);

	status = LGshow(LG_S_STRANSACTION, (PTR)&tr, sizeof(tr), &length, &sys_err);
	if (status)
	{
	    return(log_error(E_DM9017_BAD_LOG_SHOW, &sys_err, LG_S_STRANSACTION,
			E_DM9370_DM0L_SECURE, dberr, status));
	}

	status = dm0m_allocate(LG_MAX_RSZ + sizeof(DMP_MISC),
		(i4)0, (i4)MISC_CB,
		(i4)MISC_ASCII_ID, (char *)NULL, &mem_ptr, dberr);
	if (status != OK)
	{
	    uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    0, LG_MAX_RSZ + sizeof(DMP_MISC));
	    SETDBERR(dberr, 0, E_DM9370_DM0L_SECURE);
	    return (E_DB_ERROR);
	}

	/*	Initialize the DM0L_SECURE log record. */

	p1 = (DM0L_P1 *) ((char *)mem_ptr + sizeof(DMP_MISC));
	((DMP_MISC*)mem_ptr)->misc_data = (char*)p1;
	p1->p1_header.type = DM0LP1;
	p1->p1_header.flags = 0;
	p1->p1_flag = 0;

	/* Get the locks held in the transaction lock list. */

	length = 0;
	i = 0;
	count = (LG_MAX_RSZ - sizeof(DM0L_P1)) / sizeof(LK_BLKB_INFO);

	for (context = 0;;)
	{
	    LK_BLKB_INFO	    *lkb = (LK_BLKB_INFO *)lk_buffer;

	    if (status = LKshow(LK_S_TRAN_LOCKS, lock_id, 0, 0, buf_size,
			    lk_buffer, (u_i4 *)&length, (u_i4 *)&context,
			    &sys_err))
	    {
		dm0m_deallocate(&mem_ptr);
		return(log_error(E_DM901D_BAD_LOCK_SHOW, &sys_err, LK_S_TRAN_LOCKS,
			E_DM9370_DM0L_SECURE, dberr, status));
	    }

	    if (length < sizeof(LK_BLKB_INFO))
		break;

	    for (lk_count = (length / sizeof(LK_BLKB_INFO)); lk_count; lk_count--)
	    {
		/* Only log write locks. */

		if (lkb->lkb_grant_mode == LK_X || lkb->lkb_grant_mode == LK_SIX ||
		    lkb->lkb_grant_mode == LK_IX)
		{
		    STRUCT_ASSIGN_MACRO(*lkb, p1->p1_type_lk[i]);
		    i++;
		    if (i >= count)
		    {
			p1->p1_count = i;
			p1->p1_header.length = sizeof(*p1) + (i - 1) *
			    sizeof(LK_BLKB_INFO);
			lg_object.lgo_size = p1->p1_header.length;
			lg_object.lgo_data = (PTR) p1;

			status = LGwrite(LG_NOT_RESERVED, lg_id, (i4)1, &lg_object,
			    &lri, &sys_err);
			if (status)
			{
			    dm0m_deallocate(&mem_ptr);
			    return(log_error(E_DM9015_BAD_LOG_WRITE, &sys_err,
				    lg_id, E_DM9370_DM0L_SECURE, dberr,
				    status));
			}
			if (DMZ_SES_MACRO(11))
			    dmd_logtrace(&p1->p1_header, 0);

			i = 0;
		    }
		}
		lkb++;
	    }
	}
	if (i && i < count)
	{
	    p1->p1_count = i;
	    p1->p1_header.length = sizeof(*p1) + (i - 1) * sizeof(LK_BLKB_INFO);
	    lg_object.lgo_size = p1->p1_header.length;
	    lg_object.lgo_data = (PTR) p1;

	    status = LGwrite(LG_NOT_RESERVED, lg_id, (i4)1, &lg_object, &lri, &sys_err);
	    if (status)
	    {
		dm0m_deallocate(&mem_ptr);
		return(log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
			    E_DM9370_DM0L_SECURE, dberr, status));
	    }

	    if (DMZ_SES_MACRO(11))
		dmd_logtrace(&p1->p1_header, 0);
	}

	/* Get the locks held in the related lock list of the transction. */

	length = 0;
	i = 0;

	for (context = 0;;)
	{
	    LK_BLKB_INFO	    *lkb = (LK_BLKB_INFO *)lk_buffer;

	    if (status = LKshow(LK_S_REL_TRAN_LOCKS, lock_id, 0, 0, buf_size,
			    lk_buffer, (u_i4 *)&length, (u_i4 *)&context,
			    &sys_err))
	    {
		dm0m_deallocate(&mem_ptr);
		return(log_error(E_DM901D_BAD_LOCK_SHOW, &sys_err, LK_S_TRAN_LOCKS,
			E_DM9370_DM0L_SECURE, dberr, status));
	    }

	    if (length < sizeof(LK_BLKB_INFO))
		break;

	    for (lk_count = (length / sizeof(LK_BLKB_INFO)); lk_count; lk_count--)
	    {
		/* Only log write locks. */

		if (lkb->lkb_grant_mode == LK_X || lkb->lkb_grant_mode == LK_SIX ||
		    lkb->lkb_grant_mode == LK_IX)
		{
		    STRUCT_ASSIGN_MACRO(*lkb, p1->p1_type_lk[i]);
		    i++;
		    if (i >= count)
		    {
			p1->p1_count = i;
			p1->p1_flag = P1_RELATED;
			p1->p1_header.length = sizeof(*p1) + (i - 1) * sizeof(LK_BLKB_INFO);
			lg_object.lgo_size = p1->p1_header.length;
			lg_object.lgo_data = (PTR) p1;

			status = LGwrite(LG_NOT_RESERVED, lg_id, (i4)1, &lg_object, &lri,
					    &sys_err);
			if (status)
			{
			    dm0m_deallocate(&mem_ptr);
			    return(log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
				    E_DM9370_DM0L_SECURE, dberr, status));
			}
			if (DMZ_SES_MACRO(11))
			    dmd_logtrace(&p1->p1_header, 0);

			i = 0;
		    }
		}
		lkb++;
	    }
	}
	if (i && i < count)
	{
	    p1->p1_count = i;
	    p1->p1_flag = P1_RELATED;
	    p1->p1_header.length = sizeof(*p1) + (i - 1) * sizeof(LK_BLKB_INFO);
	    lg_object.lgo_size = p1->p1_header.length;
	    lg_object.lgo_data = (PTR) p1;

	    status = LGwrite(LG_NOT_RESERVED, lg_id, (i4)1, &lg_object, &lri, &sys_err);
	    if (status)
	    {
		dm0m_deallocate(&mem_ptr);
		return(log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
			    E_DM9370_DM0L_SECURE, dberr, status));
	    }

	    if (DMZ_SES_MACRO(11))
		dmd_logtrace(&p1->p1_header, 0);
	}

	/*	Write the end of the SECURE log record marker and force to disk. */

	p1->p1_header.length = sizeof(*p1);
	p1->p1_count = 0;
	p1->p1_flag = P1_LAST;
	STRUCT_ASSIGN_MACRO(*tran_id, p1->p1_tran_id);
	STRUCT_ASSIGN_MACRO(*dis_tran_id, p1->p1_dis_tran_id);
	MEcopy((PTR) tr.tr_user_name, sizeof(p1->p1_name), (PTR) &p1->p1_name);
	p1->p1_first_lga = tr.tr_first;
	p1->p1_cp_lga = tr.tr_cp;
	lg_object.lgo_size = p1->p1_header.length;
	lg_object.lgo_data = (PTR) p1;

	status = LGwrite(LG_FORCE | LG_NOT_RESERVED, lg_id, (i4)1, &lg_object, 
				&lri, &sys_err);
	if (status)
	{
	    dm0m_deallocate(&mem_ptr);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
			    E_DM9370_DM0L_SECURE, dberr, status));
	}

	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&p1->p1_header, 0);

	dm0m_deallocate(&mem_ptr);

	/*	Alter the state of the (shared) transaction to WILLING COMMIT. */

	status = LGalter(LG_A_WILLING_COMMIT, (PTR)&ctx, sizeof(ctx), &sys_err);
	if (status)
	{
	    return(log_error(E_DM900B_BAD_LOG_ALTER, &sys_err, LG_A_WILLING_COMMIT,
				E_DM9370_DM0L_SECURE, dberr, status));
	}
    }

    return (E_DB_OK);
}

/*{
** Name: dm0l_sbackup	- Write Start backup log record.
**
** Description:
**      This routine writes a start backup log record to the log file.
**
**	If the database is journaled, then we call LGalter to make sure
**	that the SBACKUP record is included in the window of journaled
**	records for this database.  This ensures that when the archiver
**	wakes up to purge the database, that it encounters the sbackup
**	record.  We need to do this because the SBACKUP record itself is
**	not written as part of a journaled transaction.  We have to make
**	the alter call before and after writing the SBACKUP in case a
**	consistency point is executed just before or after writing it.
**
** Inputs:
**	flag				Not used.
**      lx_id                           Transaction identifier.
**	flag				Flags: DM0L_NOTDB.
**	db_id				Database Identifier.
**
** Outputs:
**	la				Log Address record was written.
**	err_code			Reason for error return status.
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
**	20-jan-1989 (EdHsu)
**          Created for Terminator.
**	10-jan-1990 (rogerk)
**	    Added Online Backup bug fixes.  Added flag argument in order
**	    to pass in database journal status.  If database is journaled
**	    then we call LGalter to make sure the sbackup record is included
**	    in the next archive window.
**	15-jan-1990 (rogerk)
**	    Added Log Address parameter to allow checkpoint to give this
**	    information in debug trace messages.
**	08-jan-1992 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed db_id argument which used to contain the LG internal
**		database id for the db being backed up.  The sbackup log
**		record will now contain the external dbid in the record
**		header just like all other log records.
**	    Removed the LG_A_JCP_ADDR_SET LGalter calls as the record is now
**		correctly included in the database dump window.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Enabled DM0L_DUMP flag on sbackup log record.
**	    Documented that 'flag' parameter is not used since it isn't.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**	20-sep-1993 (jnash)
**	    Add unused flag param to LGreserve() calls.
**	30-oct-1993 (jnash)
**	    Reserve space for EBACKUP log in dm0l_ebackup rather than
**	    dm0l_sbackup.  The ebackup log is not part of the same transaction.
**      24-jan-1995 (stial01)
**          BUG 66473: flag field used. If table ckpt, flag == DM0L_TBL_CKPT.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
*/
DB_STATUS
dm0l_sbackup(
    i4		flag,
    i4             lg_id,
    DB_OWN_NAME		*username,
    LG_LSN		*lsn,
    DB_ERROR		*dberr)
{
    DM0L_SBACKUP    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    i4	    lx_id;
    DB_TRAN_ID	    tran_id;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    status = LGbegin(LG_NOPROTECT, lg_id, &tran_id, &lx_id,
	sizeof(DB_OWN_NAME), (char *)username,
	(DB_DIS_TRAN_ID*)NULL, &error);
    if (status != OK)
        return (log_error(E_DM900C_BAD_LOG_BEGIN, &error, lg_id,
                E_DM936C_DM0L_SBACKUP, dberr, status));

    /*
    ** Reserve space for SBACKUP log record,
    ** write it, and end the log txn in one call 
    ** to LGwrite().
    */
    log.sbackup_header.length = sizeof(DM0L_SBACKUP);
    log.sbackup_header.type = DM0LSBACKUP;
    log.sbackup_header.flags = flag | DM0L_DUMP;  /* BUG 66473 */

    lg_object.lgo_size = log.sbackup_header.length;
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(LG_FIRST|LG_LAST|LG_FORCE|LG_NOT_RESERVED, lx_id, (i4)1,
	&lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status != OK)
    {
	return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lx_id,
	    E_DM936C_DM0L_SBACKUP, dberr, status));
    }

    if (DMZ_SES_MACRO(11))
	dmd_logtrace(&log.sbackup_header, sizeof(DM0L_EBACKUP));

    return (E_DB_OK);
}

/*{
** Name: dm0l_ebackup	- Write end backup log record.
**
** Description:
**      This routine writes an end backup log record to the log file.
**
** Inputs:
**      lx_id                           Transaction identifier.
**	flag				Flags: DM0L_NOTDB.
**
** Outputs:
**	lsn				Log Sequence Number of log record.
**	err_code			Reason for error return status.
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
**	20-jan-1989 (EdHsu)
**          Created for Terminator.
**	15-jan-1990 (rogerk)
**	    Added Log Address parameter to allow checkpoint to give this
**	    information in debug trace messages.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed db_id argument which used to contain the LG internal
**		database id for the db being backed up.  The sbackup log
**		record will now contain the external dbid in the record
**		header just like all other log records.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Enabled DM0L_DUMP flag on ebackup log record.
**	30-oct-1993 (jnash)
**	    Reserve space for EBACKUP log in dm0l_ebackup rather than
**	    dm0l_sbackup.
**	11-Nov-1997 (jenjo02)
**	    Reserve log space and write log record with one call.
*/
DB_STATUS
dm0l_ebackup(
    i4             lg_id,
    DB_OWN_NAME		*username,
    LG_LSN		*lsn,
    DB_ERROR		*dberr)
{
    DM0L_EBACKUP    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    i4         lx_id;
    DB_TRAN_ID      tran_id;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    status = LGbegin(LG_NOPROTECT, lg_id, &tran_id, &lx_id,
	sizeof(DB_OWN_NAME), (char *)username,
	(DB_DIS_TRAN_ID*)NULL, &error);
    if (status != OK)
        return (log_error(E_DM900C_BAD_LOG_BEGIN, &error, lg_id,
                E_DM936D_DM0L_EBACKUP, dberr, status));

    /*
    ** Reserve space for EBACKUP log record,
    ** write it, and end log txn with one
    ** call to LGwrite().
    */

    log.ebackup_header.length = sizeof(DM0L_EBACKUP);
    log.ebackup_header.type = DM0LEBACKUP;
    log.ebackup_header.flags = DM0L_DUMP;

    lg_object.lgo_size = log.ebackup_header.length;
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(LG_FIRST|LG_LAST|LG_FORCE|LG_NOT_RESERVED, lx_id, (i4)1, &lg_object,
	&lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.ebackup_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lx_id,
	E_DM936D_DM0L_EBACKUP, dberr, status));
}

/*{
** Name: dm0l_crdb      - Create Database.
**
** Description:
**      This routine writes a create database log record to the log file.
**
** Inputs:
**      lg_id                           The logging system identifier.
**      db_id                           Logging system database identifier.
**      db_name                         Database name.
**      db_access                       Database access code.
**      db_service                      Database service code.
**      db_location                     Logical location name.
**      db_l_root                       Length of root directory name.
**      db_root                         Root directory name.
**
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      15-aug-91 (jnash) merged 04-apr-1989 (Derek)
**          Created for Jupiter.  Also, fix log_error call at end of fn.
**	30-nov-1992 (jnash)
**	    Reduced logging project.  Add CLR handling, flag and prev_lsn
**	    params.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
*/
DB_STATUS
dm0l_crdb(
    i4             lg_id,
    i4             flag,
    i4             db_id,
    DB_DB_NAME          *db_name,
    i4             db_access,
    i4             db_service,
    DB_LOC_NAME         *db_location,
    i4             db_l_root,
    DM_PATH             *db_root,
    LG_LSN		*prev_lsn,
    DB_ERROR            *dberr)
{
    DM0L_CRDB       log;
    LG_OBJECT       lg_object;
    LG_LRI	    lri;
    DB_STATUS       status;
    CL_ERR_DESC      error;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*  Initialize the log record. */

    log.cd_header.length = sizeof(DM0L_CRDB);
    log.cd_header.type = DM0LCRDB;
    log.cd_header.flags = 0;
    log.cd_dbid = db_id;
    log.cd_dbname = *db_name;
    log.cd_access = db_access;
    log.cd_service = db_service;
    log.cd_location = *db_location;
    log.cd_l_root = db_l_root;
    log.cd_root = *db_root;
    if (flag & DM0L_CLR)
	log.cd_header.compensated_lsn = *prev_lsn;

    /*  Write the record to the log. */

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(0, lg_id, 2, 2 * sizeof(DM0L_CRDB), &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM929A_DM0L_CRDB, dberr, status));
	}
    }

    status = LGwrite(LG_FIRST, lg_id, (i4)1, &lg_object, &lri, &error);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.cd_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_CRDB));
	}

        return (E_DB_OK);
    }

    return log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
                E_DM929A_DM0L_CRDB, dberr, status);
}

/*{
** Name: dm0l_dmu	- Log DMU operation.
**
** Description:
**      This routine writes a DMU log record to the log file.
**	It also informs the logging system (via LGalter) that a DMU
**	operation was performed.
**
**	This record marks a spot in the log file after which recovery
**	is not idempotent.  If a transaction rollback occurs on this
**	transaction, the recovering process will write an ABORT_SAVE
**	record to the logfile when it encounters this DMU record.
**
**	This ABORT_SAVE record will cause any other process that attempts to
**	re-recover this same transaction to skip directly to the SAVEPOINT
**	record prceeding the DMU log record and begin processing from there.
**	It cannot attempt to re-recover the operations that occured after the
**	DMU operation.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: Not currently used.
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**	operation			DMU operation.
**
** Outputs:
**	err_code			Reason for error.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	Logging system is informed (via LGalter) that a DMU operation is
**	being done.  LG must reserve space in the logfile to be able to
**	back out this operation.
**
** History:
**	11-sep-1989 (rogerk)
**          Created for Terminator.
**	20-sep-1988 (rogerk)
**	    Write Savepoint record and put address in DMU record.
**	30-oct-1992 (jnash)
**	    Reduced logging project.  Update prev_lsn if CLR.
**	17-dec-1992 (jnash)
**	    Update "flags" properly, don't write savepoint log if CLR.
**	13-jan-1993 (jnash)
**	    Reduced logging project.  Add LGreserve call.
**	18-jan-1993 (rogerk)
**	    Removed ABORTSAVE log record written during backout of DMU
**	    operations.  The same functionality is now provided by CLR's.
**	    Made CLR of DMU be a NONREDO operation.  After the backout
**	    of a DMU operation, we can no longer attempt to redo updates
**	    to the table made prior to the dmu rollback.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	20-sep-1993 (jnash)
**	    Add flag param to LGreserve() calls.
**	18-oct-1993 (rogerk)
**	    Removed LG_A_DMU alter operation.  Log space requirements for DMU
**	    operations are now tracked through normal space reservations.
*/
DB_STATUS
dm0l_dmu(
    i4	    lg_id,
    i4		    flag,
    DB_TAB_ID	    *tbl_id,
    DB_TAB_NAME	    *tbl_name,
    DB_OWN_NAME	    *tbl_owner,
    i4	    operation,
    LG_LSN	    *prev_lsn,
    DB_ERROR	    *dberr)
{
    DM0L_DMU	    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    STATUS	    status;
    CL_ERR_DESC	    sys_err;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(0, lg_id, 2, 2 * sizeof(DM0L_DMU), &sys_err);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &sys_err,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &sys_err,
		lg_id, E_DM938F_DM0L_DMU, dberr, status));
	}
    }

    /*	Initialize the log record. */

    log.dmu_header.length = sizeof(DM0L_DMU);
    log.dmu_header.type = DM0LDMU;
    log.dmu_header.flags = flag;
    log.dmu_tabid = *tbl_id;
    log.dmu_tblname = *tbl_name;
    log.dmu_tblowner = *tbl_owner;
    log.dmu_operation = operation;
    if (flag & DM0L_CLR)
    {
	log.dmu_header.compensated_lsn = *prev_lsn;
	log.dmu_header.flags |= DM0L_NONREDO;
    }

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &sys_err);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.dmu_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_DMU));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &sys_err, lg_id,
    	E_DM938F_DM0L_DMU, dberr, status));
}

/*{
** Name: dm0l_test	- Write a Test log record.
**
** Description:
**	This routine writes a test log record to the log file.
**	Test log records are written to drive logging and journaling
**	system tests.
**
**	They are not logged by any "normal" ingres operation and do not
**	mark the need for any specific recovery.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	test_number			Specifies the type of test.
**	p1-p6				Data specific to test.
**	p5l				Length of p5.
**	p6l				Length of p6.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	25-feb-1991 (rogerk)
**          Created as part of Archiver Stability project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
*/
DB_STATUS
dm0l_test(
    i4		    lg_id,
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
    DB_ERROR		    *dberr)
{
    DM0L_TEST	    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.tst_header.length = sizeof(DM0L_TEST);
    log.tst_header.type = DM0LTEST;
    log.tst_header.flags = flag;
    log.tst_number = test_number;
    log.tst_stuff1 = p1;
    log.tst_stuff2 = p2;
    log.tst_stuff3 = p3;
    log.tst_stuff4 = p4;

    if (p5 && p5l && (p5l <= sizeof(log.tst_stuff5)))
	MEcopy((PTR) p5, p5l, (PTR) log.tst_stuff5);

    if (p6 && p6l && (p6l <= sizeof(log.tst_stuff6)))
	MEcopy((PTR) p6, p6l, (PTR) log.tst_stuff6);

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	    dmd_logtrace(&log.tst_header, 0);

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM939F_DM0L_TEST, dberr, status));
}

/*{
** Name: dm0l_assoc	- Write an change association log record.
**
** Description:
**	The change association log record is written when a full assocaited
**	data is changed in favor of a new empty assocaited data page.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	name				Table name.
**	name_len			Blank-stripped length of table name
**	owner				Table owner.
**	owner_len			Blank-stripped length of owner name
**	page_size			Page size.
**	leafpage			Leaf page number
**	olddata				Old assocaited data page.
**	newdata				New associated data page.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	 15-aug-91 (jnash) merged 1-apr-1990 (Derek)
**          Created.
**	22-oct-1992 (jnash)
**	    Modified for Reduced Logging project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
*/
DB_STATUS
dm0l_assoc(
    i4         lg_id,
    i4         flag,
    DB_TAB_ID       *tbl_id,
    DB_TAB_NAME     *name,
    i2		    name_len,
    DB_OWN_NAME     *owner,
    i2		    owner_len,
    i4         pg_type,
    i4	    page_size,
    i4         loc_cnt,
    i4         leaf_conf_id,
    i4         odata_conf_id,
    i4         ndata_conf_id,
    DM_PAGENO	    leafpage,
    DM_PAGENO	    oldpage,
    DM_PAGENO	    newpage,
    LG_LSN	    *prev_lsn,
    LG_LRI	    *lrileaf,
    LG_LRI	    *lridata,
    DB_ERROR	    *dberr)
{
    DM0L_ASSOC	    log;
    LG_OBJECT	    lg_object[3];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    lg_object[0].lgo_size = sizeof(log);
    lg_object[0].lgo_data = (PTR) &log;

    log.ass_header.length = sizeof(DM0L_ASSOC);
    log.ass_header.type = DM0LASSOC;
    log.ass_header.flags = flag;
    if (flag & DM0L_CLR)
	log.ass_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lrileaf, &log.ass_crhdrleaf);
    DM0L_MAKE_CRHEADER2_FROM_LRI(lridata, &log.ass_crhdrdata);

    log.ass_tbl_id = *tbl_id;
    log.ass_pg_type  = pg_type;
    log.ass_page_size = page_size;
    log.ass_loc_cnt = loc_cnt;
    log.ass_lcnf_loc_id = leaf_conf_id;
    log.ass_ocnf_loc_id = odata_conf_id;
    log.ass_ncnf_loc_id = ndata_conf_id;
    log.ass_leaf_page = leafpage;
    log.ass_old_data = oldpage;
    log.ass_new_data = newpage;

    status = LGwrite(0, lg_id, (i4)1, lg_object, lrileaf, &error);

    if (status == OK)
    {
	STRUCT_ASSIGN_MACRO(*lrileaf, *lridata);

	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.ass_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_ASSOC));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9296_DM0L_ASSOC, dberr, status));
}

/*{
** Name: dm0l_alloc 	- Write an allocate page log record.
**
** Description:
**	The allocate page log record is written ihen a page is allocated
**	from the free bitmap.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	name				Table name.
**	name_len			Blank-stripped length of table name
**	owner				Table owner.
**	owner_len			Blank-stripped length of owner name
**	page_size			Page size.
**	page				Page number allocated.
**	flag				Log record flags.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**      lsn                             Log record address.
**	err_code			Reason for error return status.
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
**	 1-apr-1990 (Derek)
**          Created.
**	08-jul-1991 (Derek)
**	    Added flag parameter to allow special log record flags to
**	    be set.
**      15-aug-91 (jnash) function merged into 6.5
**	26-oct-92 (rogerk)
**	    Modified for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
*/
DB_STATUS
dm0l_alloc(
i4         lg_id,
i4		flag,
DB_TAB_ID       *tbl_id,
DB_TAB_NAME	*tbl_name,
i2		name_len,
DB_OWN_NAME	*tbl_owner,
i2		owner_len,
i4		pg_type,
i4		page_size,
i4		loc_cnt,
DM_PAGENO	fhdr_pageno,
DM_PAGENO	fmap_pageno,
DM_PAGENO	free_pageno,
i4		fmap_index,
i4		fhdr_config_id,
i4		fmap_config_id,
i4		free_config_id,
bool		hwmap_update,
bool		freehint_update,
LG_LSN		*prev_lsn,
LG_LSN		*lsn,
DB_ERROR	*dberr)
{
    DM0L_ALLOC	    log;
    LG_OBJECT	    lg_object[3];
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.all_header.length = sizeof(DM0L_ALLOC);
    log.all_header.type = DM0LALLOC;
    log.all_header.flags = flag;
    if (flag & DM0L_CLR)
	log.all_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.all_tblid);
    log.all_pg_type = pg_type;
    log.all_page_size = page_size;
    log.all_loc_cnt = loc_cnt;
    log.all_fhdr_pageno = fhdr_pageno;
    log.all_fmap_pageno = fmap_pageno;
    log.all_free_pageno = free_pageno;
    log.all_map_index = fmap_index;
    log.all_fhdr_cnf_loc_id = fhdr_config_id;
    log.all_fmap_cnf_loc_id = fmap_config_id;
    log.all_free_cnf_loc_id = free_config_id;

    log.all_fhdr_hwmap = FALSE;
    log.all_fhdr_hint = FALSE;
    if (hwmap_update)
	log.all_fhdr_hwmap = TRUE;
    if (freehint_update)
	log.all_fhdr_hint = TRUE;

    lg_object[0].lgo_size = sizeof(log);
    lg_object[0].lgo_data = (PTR) &log;

    status = LGwrite(0, lg_id, (i4)1, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.all_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_ALLOC));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9297_DM0L_ALLOC, dberr, status));
}

/*{
** Name: dm0l_dealloc 	- Write an deallocate page log record.
**
** Description:
**	The allocate page log record is written when a page is deallocated
**	to the free bitmap.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	name				Table name.
**	name_len			Blank-stripped length of table name
**	owner				Table owner.
**	owner_len			Blank-stripped length of owner name
**	page_size			Page size.
**	page				Page number deallocated.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	15-aug-91 (jnash) merged 1-apr-1990 (Derek)
**          Created.
**	26-oct-92 (rogerk)
**	    Modified for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
*/
DB_STATUS
dm0l_dealloc(
i4         lg_id,
i4		flag,
DB_TAB_ID       *tbl_id,
DB_TAB_NAME	*tbl_name,
i2		name_len,
DB_OWN_NAME	*tbl_owner,
i2		owner_len,
i4		pg_type,
i4		page_size,
i4		loc_cnt,
DM_PAGENO	fhdr_pageno,
DM_PAGENO	fmap_pageno,
DM_PAGENO	free_pageno,
i4		fmap_index,
i4		fhdr_config_id,
i4		fmap_config_id,
i4		free_config_id,
bool		freehint_update,
LG_LSN		*prev_lsn,
LG_LSN		*lsn,
DB_ERROR        *dberr)
{
    DM0L_DEALLOC    log;
    LG_OBJECT	    lg_object[3];
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.dall_header.length = sizeof(DM0L_DEALLOC);
    log.dall_header.type = DM0LDEALLOC;
    log.dall_header.flags = flag;
    if (flag & DM0L_CLR)
	log.dall_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.dall_tblid);
    log.dall_pg_type = pg_type;
    log.dall_page_size = page_size;
    log.dall_loc_cnt = loc_cnt;
    log.dall_fhdr_pageno = fhdr_pageno;
    log.dall_fmap_pageno = fmap_pageno;
    log.dall_free_pageno = free_pageno;
    log.dall_map_index = fmap_index;
    log.dall_fhdr_cnf_loc_id = fhdr_config_id;
    log.dall_fmap_cnf_loc_id = fmap_config_id;
    log.dall_free_cnf_loc_id = free_config_id;

    log.dall_fhdr_hint = FALSE;
    if (freehint_update)
	log.dall_fhdr_hint = TRUE;

    lg_object[0].lgo_size = sizeof(log);
    lg_object[0].lgo_data = (PTR) &log;

    status = LGwrite(0, lg_id, (i4)1, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.dall_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_DEALLOC));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9298_DM0L_DEALLOC, dberr, status));
}

/*{
** Name: dm0l_extend 	- Write an extend table log record.
**
** Description:
**	The allocate page log record is written when a table is extended.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	page_size			Page size.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	15-aug-91 (jnash) merged 1-apr-1990 (Derek)
**          Created.
**	26-oct-92 (rogerk)
**	    Modified for Reduced Logging Project.
**	08-dec-1992 (jnash)
**	    Add cnf_loc_array param for use by rollforward.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	03-sep-1996 (pchang)
**	    Added ext_last_used to DM0L_EXTEND and fmap_last_used to DM0L_FMAP
**	    and their corresponding function prototypes as part of the fix for
**	    B77416.
*/
DB_STATUS
dm0l_extend(
i4         lg_id,
i4		flag,
DB_TAB_ID       *tbl_id,
DB_TAB_NAME	*tbl_name,
DB_OWN_NAME	*tbl_owner,
i4		pg_type,
i4		page_size,
i4		loc_cnt,
i4		*cnf_loc_array,
DM_PAGENO	fhdr_pageno,
DM_PAGENO	fmap_pageno,
i4		fmap_index,
i4		fhdr_config_id,
i4		fmap_config_id,
DM_PAGENO	first_used,
DM_PAGENO	last_used,
DM_PAGENO	first_free,
DM_PAGENO	last_free,
i4		oldeof,
i4		neweof,
bool		hwmap_update,
bool		freehint_update,
bool		fmap_update,
LG_LSN		*prev_lsn,
LG_LSN		*lsn,
DB_ERROR	*dberr)
{
    DM0L_EXTEND	    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4	    i;

    CLRDBERR(dberr);

    log.ext_header.length = sizeof(DM0L_EXTEND);
    log.ext_header.type = DM0LEXTEND;
    log.ext_header.flags = flag;
    if (flag & DM0L_CLR)
	log.ext_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.ext_tblid);

    log.ext_pg_type = pg_type;
    log.ext_page_size = page_size;
    log.ext_loc_cnt = loc_cnt;
    log.ext_fhdr_pageno = fhdr_pageno;
    log.ext_fmap_pageno = fmap_pageno;
    log.ext_map_index = fmap_index;
    log.ext_fhdr_cnf_loc_id = fhdr_config_id;
    log.ext_fmap_cnf_loc_id = fmap_config_id;
    log.ext_first_used = first_used;
    log.ext_last_used = last_used;
    log.ext_first_free = first_free;
    log.ext_last_free = last_free;
    log.ext_old_pages = oldeof;
    log.ext_new_pages = neweof;
    for (i = 0; i < loc_cnt; i++)
	log.ext_cnf_loc_array[i] = cnf_loc_array[i];

    log.ext_fhdr_hwmap = FALSE;
    log.ext_fhdr_hint = FALSE;
    log.ext_fmap_update = FALSE;
    if (hwmap_update)
	log.ext_fhdr_hwmap = TRUE;
    if (freehint_update)
	log.ext_fhdr_hint = TRUE;
    if (fmap_update)
	log.ext_fmap_update = TRUE;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;
    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.ext_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_EXTEND));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9299_DM0L_EXTEND, dberr, status));
}

/*{
** Name: dm0l_ovfl	- Write an overflow page log record.
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
**	BI/ALLOC records to physically undo the operation.
**
** Inputs:
**      lg_id                           Logging system identifier.
**      flag				flag:
**					 DM0L_JOURNAL
**					 DM0L_NOFULLON - Set fullchain bit
**	tbl_id				Table id.
**	name				Table name.
**	name_len			Blank-stripped length of table name
**	owner				Table owner.
**	owner_len			Blank-stripped length of owner name
**	page_size			Page size.
**	newpage				New overflow page.
**	rootpage			Page to which the new overflow page
**					is being linked.
**	ovfl_ptr			Root page's old overflow pointer which
**					is being moved to the new overflow page.
**	main_ptr			Root page's mainpage pointer to which
**					all of its overflow page's should also
**					set their page_main fields.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	 3-nov-1991 (rogerk)
**          Created to fix REDO problems with File Extend.
**	 16-oct-1992 (jnash)
**          Modified for 6.5 recovery.
**	07-dec-1992 (jnash)
**	    Fix bug by changing newpage and rootpage order in param list.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
*/
DB_STATUS
dm0l_ovfl(
    i4         lg_id,
    i4         flag,
    bool	    fullc,
    DB_TAB_ID       *tbl_id,
    DB_TAB_NAME     *name,
    i2		    name_len,
    DB_OWN_NAME     *owner,
    i2		    owner_len,
    i4         pg_type,
    i4	    page_size,
    i4 	    loc_cnt,
    i4         config_id,
    i4	    oflo_config_id,
    DM_PAGENO	    newpage,
    DM_PAGENO	    rootpage,
    DM_PAGENO	    ovfl_ptr,
    DM_PAGENO	    main_ptr,
    LG_LSN	    *prev_lsn,
    LG_LRI	    *lri,
    DB_ERROR        *dberr)
{
    DM0L_OVFL	    log;
    LG_OBJECT	    lg_object[3];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.ovf_header.length = sizeof(DM0L_OVFL);
    log.ovf_header.type = DM0LOVFL;
    log.ovf_header.flags = flag;
    if (flag & DM0L_CLR)
	log.ovf_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lri, &log.ovf_crhdr);

    lg_object[0].lgo_size = sizeof(log);
    lg_object[0].lgo_data = (PTR) &log;

    log.ovf_fullc = (i2)fullc;
    log.ovf_newpage = newpage;
    log.ovf_page = rootpage;
    log.ovf_ovfl_ptr = ovfl_ptr;
    log.ovf_main_ptr = main_ptr;
    log.ovf_pg_type  = pg_type;
    log.ovf_page_size = page_size;
    log.ovf_loc_cnt = loc_cnt;
    log.ovf_cnf_loc_id = config_id;
    log.ovf_ovfl_cnf_loc_id = oflo_config_id;
    log.ovf_tbl_id = *tbl_id;

    status = LGwrite(0, lg_id, (i4)1, lg_object, lri, &error);

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.ovf_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_OVFL));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM929B_DM0L_OVFL, dberr, status));
}

/*{
** Name: dm0l_ufmap 	- Write an fmap update log record (associated with
**                        and fmap add see dm0l_fmap().
**
** Description:
**	Written during File Extend operations when new FMAP pages must be
**	added to the table but the current last and new fmaps do not
**      address the page into which the new FMAP is being placed.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	page_size			Page size.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	23-Feb-2009 (hanal04) Bug 121652
**	    Created.
*/
DB_STATUS
dm0l_ufmap(
i4         lg_id,
i4		flag,
DB_TAB_ID       *tbl_id,
DB_TAB_NAME	*tbl_name,
DB_OWN_NAME	*tbl_owner,
i4		pg_type,
i4		page_size,
i4		loc_cnt,
DM_PAGENO	fhdr_pageno,
DM_PAGENO	fmap_pageno,
i4		fmap_index,
i4		fmap_hw_mark,
i4		fhdr_config_id,
i4		fmap_config_id,
DM_PAGENO	first_used,
DM_PAGENO	last_used,
LG_LSN		*prev_lsn,
LG_LSN		*lsn,
DB_ERROR	*dberr)
{
    DM0L_FMAP	    log;
    LG_OBJECT	    lg_object;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    LG_LRI	    lri;

    CLRDBERR(dberr);

    log.fmap_header.length = sizeof(DM0L_UFMAP);
    log.fmap_header.type = DM0LUFMAP;
    log.fmap_header.flags = flag;
    if (flag & DM0L_CLR)
	log.fmap_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.fmap_tblid);

    log.fmap_pg_type = pg_type;
    log.fmap_page_size = page_size;
    log.fmap_loc_cnt = loc_cnt;
    log.fmap_fhdr_pageno = fhdr_pageno;
    log.fmap_fmap_pageno = fmap_pageno;
    log.fmap_map_index = fmap_index;
    log.fmap_hw_mark = fmap_hw_mark;
    log.fmap_fhdr_cnf_loc_id = fhdr_config_id;
    log.fmap_fmap_cnf_loc_id = fmap_config_id;
    log.fmap_first_used = first_used;
    log.fmap_last_used = last_used;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;
    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.fmap_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_FMAP));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C0D_DM0L_FMAP, dberr, status));
}
/*{
** Name: dm0l_fmap 	- Write an add fmap log record.
**
** Description:
**	Written during File Extend operations when new FMAP pages must be
**	added to the table.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	page_size			Page size.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	26-oct-92 (rogerk)
**	    Created for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	03-sep-1996 (pchang)
**	    Added ext_last_used to DM0L_EXTEND and fmap_last_used to DM0L_FMAP
**	    and their corresponding function prototypes as part of the fix for
**	    B77416.
**      30-Oct-2000 (hanal04) Bug 103037 INGSRV 1291
**          Added fmap_hw_mark to DM0L_FMAP and the corresponding function
**          prototypes. This allows dmv_refmap() to restore the fmap
**          highwater mark rather than generateing an erroneous value of
**          16000.
*/
DB_STATUS
dm0l_fmap(
i4         lg_id,
i4		flag,
DB_TAB_ID       *tbl_id,
DB_TAB_NAME	*tbl_name,
DB_OWN_NAME	*tbl_owner,
i4		pg_type,
i4		page_size,
i4		loc_cnt,
DM_PAGENO	fhdr_pageno,
DM_PAGENO	fmap_pageno,
i4		fmap_index,
i4		fmap_hw_mark,
i4		fhdr_config_id,
i4		fmap_config_id,
DM_PAGENO	first_used,
DM_PAGENO	last_used,
DM_PAGENO	first_free,
DM_PAGENO	last_free,
LG_LSN		*prev_lsn,
LG_LSN		*lsn,
DB_ERROR	*dberr)
{
    DM0L_FMAP	    log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    log.fmap_header.length = sizeof(DM0L_FMAP);
    log.fmap_header.type = DM0LFMAP;
    log.fmap_header.flags = flag;
    if (flag & DM0L_CLR)
	log.fmap_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.fmap_tblid);

    log.fmap_pg_type = pg_type;
    log.fmap_page_size = page_size;
    log.fmap_loc_cnt = loc_cnt;
    log.fmap_fhdr_pageno = fhdr_pageno;
    log.fmap_fmap_pageno = fmap_pageno;
    log.fmap_map_index = fmap_index;
    log.fmap_hw_mark = fmap_hw_mark;
    log.fmap_fhdr_cnf_loc_id = fhdr_config_id;
    log.fmap_fmap_cnf_loc_id = fmap_config_id;
    log.fmap_first_used = first_used;
    log.fmap_last_used = last_used;
    log.fmap_first_free = first_free;
    log.fmap_last_free = last_free;

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;
    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.fmap_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_FMAP));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C0D_DM0L_FMAP, dberr, status));
}

/*{
** Name: dm0l_btput 	- Write a Btree Insert log record.
**
** Description:
**	Written when a new key is insert to a btree index page.
**	It logs the update to the index only, not to the actual data page.
**	This record is written during both insert and replace operations.
**
**	The index Key, the table name and the table owner are logged
**	as variable length objects.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	tbl_name			Table name.
**	name_len			Blank-stripped length of table name
**	tbl_owner			Table owner.
**	owner_len			Blank-stripped length of owner name
**	pg_type				Page Format Type.
**	page_size			Page size.
**	loc_cnt				Number of locations in table.
**	loc_cnf_id			Identifier for index page's location.
**	bid				Page and Line number of new index entry.
**	tid				Tid value in new index entry.
**	key_length			Length of new key.
**	key				Key value of new index entry.
**	prev_lsn			LSN of compensated log record if CLR.
**	partno				TID's partition number.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	26-oct-92 (rogerk)
**	    Created for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	21-jun-1993 (rogerk)
**	    Add support for zero-length key in record - used for put CLRs.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	19-Jan-2004 (jenjo02)
**	    Added partno for global indexes.
**	03-May-2006 (jenjo02)
**	    Add ixklen.
**	13-Jun-2006 (jenjo02)
**	    Puts of leaf entries may be up to
**	    DM1B_MAXLEAFLEN, not DM1B_KEYLENGTH, bytes.
*/
DB_STATUS
dm0l_btput(
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
LG_LRI		*lri,
i4		partno,
u_i4		btflags,
DB_ERROR	*dberr)
{
    DM0L_BTPUT	    log;
    LG_OBJECT	    lg_object[4];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    num_objects;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length key */
    log.btp_header.length = sizeof(DM0L_BTPUT) - (DM1B_MAXLEAFLEN - key_length);

    log.btp_header.type = DM0LBTPUT;
    log.btp_header.flags = flag;
    if (flag & DM0L_CLR)
	log.btp_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lri, &log.btp_crhdr);

    STRUCT_ASSIGN_MACRO(*tbl_id, log.btp_tbl_id);
    log.btp_pg_type = pg_type;
    log.btp_page_size = page_size;
    log.btp_cmp_type = compression_type;
    log.btp_loc_cnt = loc_cnt;
    log.btp_cnf_loc_id = loc_config_id;
    log.btp_bid = *bid;
    log.btp_bid_child = bid_child;
    log.btp_tid = *tid;
    log.btp_key_size = key_length;
    log.btp_partno = partno;
    log.btp_btflags = btflags;

    num_objects = 1;
    lg_object[0].lgo_size = ((char *)&log.btp_vbuf[0]) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;

    if (key_length)
    {
	lg_object[1].lgo_size = key_length;
	lg_object[1].lgo_data = key;
	num_objects++;
    }

    status = LGwrite(0, lg_id, num_objects, lg_object, lri, &error);
    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.btp_header, (flag & DM0L_CLR) ? 0 :
		(sizeof(DM0L_BTPUT) - (DM1B_MAXLEAFLEN - key_length)));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C0E_DM0L_BTPUT, dberr, status));
}

/*{
** Name: dm0l_btdel 	- Write a Btree Delete log record.
**
** Description:
**	Written when a key is deleted from a btree index page.
**	It logs the update to the index only, not to the actual data page.
**	This record is written during both delete and replace operations.
**
**	The index Key, the table name and the table owner are logged
**	as variable length objects.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	tbl_name			Table name.
**	name_len			Blank-stripped length of table name
**	tbl_owner			Table owner.
**	owner_len			Blank-stripped length of owner name
**	pg_type				Page Format Type.
**	page_size			Page size.
**	compression_type		Key Compression Indicator.
**	loc_cnt				Number of locations in table.
**	loc_cnf_id			Identifier for index page's location.
**	bid				Page and Line number of old index entry.
**	tid				Tid value in old index entry.
**	key_length			Length of new key.
**	key				Key value of old index entry.
**	prev_lsn			LSN of compensated log record if CLR.
**	partno				TID's partition number.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	26-oct-92 (rogerk)
**	    Created for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	16-Jan-2004 (jenjo02)
**	    Added partition number for global indexes.
**	03-May-2006 (jenjo02)
**	    Add ixklen.
**	13-Jun-2006 (jenjo02)
**	    Deletes of leaf entries may be up to
**	    DM1B_MAXLEAFLEN, not DM1B_KEYLENGTH, bytes.
*/
DB_STATUS
dm0l_btdel(
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
LG_LRI		*lri,
i4		partno,
u_i4		btflags,
DB_ERROR	*dberr)
{
    DM0L_BTDEL	    log;
    LG_OBJECT	    lg_object[4];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length key */
    log.btd_header.length = sizeof(DM0L_BTDEL) - (DM1B_MAXLEAFLEN - key_length);
    log.btd_header.type = DM0LBTDEL;
    log.btd_header.flags = flag;
    if (flag & DM0L_CLR)
	log.btd_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lri, &log.btd_crhdr);

    STRUCT_ASSIGN_MACRO(*tbl_id, log.btd_tbl_id);
    log.btd_pg_type = pg_type;
    log.btd_page_size = page_size;
    log.btd_cmp_type = compression_type;
    log.btd_loc_cnt = loc_cnt;
    log.btd_cnf_loc_id = loc_config_id;
    log.btd_bid = *bid;
    log.btd_bid_child = bid_child;
    log.btd_tid = *tid;
    log.btd_partno = partno;
    log.btd_key_size = key_length;
    log.btd_btflags = btflags;

    lg_object[0].lgo_size = ((char *)&log.btd_vbuf[0]) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = key_length;
    lg_object[1].lgo_data = key;

    status = LGwrite(0, lg_id, (i4)2, lg_object, lri, &error);

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.btd_header, (flag & DM0L_CLR) ? 0 :
		(sizeof(DM0L_BTDEL) - (DM1B_MAXLEAFLEN - key_length)));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C0F_DM0L_BTDEL, dberr, status));
}

/*{
** Name: dm0l_btsplit 	- Write a Btree Index Split log record.
**
** Description:
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Log record flags:
**					    DM0L_JOURNAL - journal the update
**					    DM0L_PHYSICAL - physical locking
**						should be used during recovery
**	tbl_id				Table id.
**	tbl_name			Table Name.
**	tbl_owner			Table Owner.
**	pg_type				Page Format Type.
**	page_size			Page size.
**      kperpage                        Keys per page.
**	compression_type		Key Compression Indicator.
**	loc_cnt				Number of locations in table.
**	cur_loc_cnf_id			Identifier for old page's location.
**	loc_loc_cnf_id			Identifier for new page's location.
**	dat_loc_cnf_id			Identifier for new data page's location.
**	cur_pageno			Old Index page number.
**	sib_pageno			New Index page number.
**	dat_pageno			New data page number.
**	keylen				Index key length (non-compressed).
**	split_position			Position at which old page was split.
**	split_direction			Direction of split: left or right.
**	descriminator_length		Length of Key which defines split point.
**	descriminator_key		Key which defines split point.
**	split_page			Page being split.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**      lsn				Log record address.
**	err_code			Reason for error return status.
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
**	26-oct-92 (rogerk)
**	    Created for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Added split direction to SPLIT log records.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	12-dec-1996 (shero03)
**	    Use vbuf to hold the split page.
**	09-jan-1997 (nanpr01)
** 	    Revert back the order changed in previous integration causing
**	    BUS error.
**      09-feb-1999 (stial01)
**          dm0l_btsplit() Log kperpage
**	01-May-2006 (jenjo02)
**	    Add range_keylen, max length of range entry.
*/
DB_STATUS
dm0l_btsplit(
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
i4		descrim_keylength,
char		*descrim_key,
DMPP_PAGE	*split_page,
LG_LSN		*prev_lsn,
LG_LRI		*lri,
DB_ERROR	*dberr)
{
    DM0L_BTSPLIT    log;
    LG_OBJECT	    lg_object[3];
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length key */
    log.spl_header.length = sizeof(DM0L_BTSPLIT) - sizeof(DMPP_PAGE)
				+ page_size + descrim_keylength; 

    log.spl_header.type = DM0LBTSPLIT;
    log.spl_header.flags = flag;
    if (flag & DM0L_CLR)
	log.spl_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lri, &log.spl_crhdr);

    STRUCT_ASSIGN_MACRO(*tbl_id, log.spl_tbl_id);

    log.spl_pg_type = pg_type;
    log.spl_page_size = page_size;
    log.spl_kperpage = kperpage;
    log.spl_cmp_type = compression_type;
    log.spl_loc_cnt = loc_cnt;
    log.spl_cur_loc_id = cur_loc_cnf_id;
    log.spl_sib_loc_id = sib_loc_cnf_id;
    log.spl_dat_loc_id = dat_loc_cnf_id;
    log.spl_cur_pageno = cur_pageno;
    log.spl_sib_pageno = sib_pageno;
    log.spl_dat_pageno = dat_pageno;
    log.spl_split_pos = split_position;
    log.spl_split_dir = split_direction;
    log.spl_klen = index_keylen;
    log.spl_desc_klen = descrim_keylength;
    log.spl_range_klen = range_keylen;

    lg_object[0].lgo_size = ((char *)&log.spl_vbuf) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = page_size;
    lg_object[1].lgo_data = (PTR) split_page;
    lg_object[2].lgo_size = descrim_keylength;
    lg_object[2].lgo_data = (PTR) descrim_key;

    status = LGwrite(0, lg_id, (i4) 3, lg_object, lri, &error);

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.spl_header,
		(flag & DM0L_CLR) ? 0 : log.spl_header.length);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C10_DM0L_BTSPLIT, dberr, status));
}

/*{
** Name: dm0l_btovfl - Write a Btree Allocate Overflow Chain log record.
**
** Description:
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Log record flags:
**					    DM0L_JOURNAL - journal the update
**					    DM0L_PHYSICAL - physical locking
**						should be used during recovery
**	tbl_id				Table id.
**	tbl_name			Table Name.
**	tbl_owner			Table Owner.
**	pg_type				Page Format Type.
**	page_size			Page size.
**	compression_type		Key Compression Indicator.
**	loc_cnt				Number of locations in table.
**	leaf_loc_cnf_id			Identifier for leaf page's location.
**	ovfl_loc_cnf_id			Identifier for overflow page's location.
**	leaf_pageno			Leaf page number.
**	ovfl_pageno			Overflow page number.
**	mainpage			Page's page_main value.
**	nextpage			Page's bt_nextpage value.
**	nextovfl			Page's page_ovfl value.
**	lrange_len			Low Range Key length.
**	lrange_key			Low Range Key.
**	lrange_tid			Tid value of Low Range Key.
**	rrange_len			High Range Key length.
**	rrange_key			High Range Key.
**	rrange_tid			Tid value of High Range Key.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**      lsn				Log record address.
**	err_code			Reason for error return status.
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
**	26-oct-92 (rogerk)
**	    Created for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	19-Jan-2004 (jenjo02)
**	    Added tidsize for global indexes.
*/
DB_STATUS
dm0l_btovfl(
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
DB_ERROR	*dberr)
{
    DM0L_BTOVFL	    log;
    LG_OBJECT	    lg_object[3];
    i4		    lg_count;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length range keys */
    log.bto_header.length = sizeof(DM0L_BTOVFL) -
				(DM1B_KEYLENGTH - lrange_len) -
				(DM1B_KEYLENGTH - rrange_len);

    log.bto_header.type = DM0LBTOVFL;
    log.bto_header.flags = flag;
    if (flag & DM0L_CLR)
	log.bto_header.compensated_lsn = *prev_lsn;

    /* Stow LRI in CR header */
    DM0L_MAKE_CRHEADER_FROM_LRI(lri, &log.bto_crhdr);

    STRUCT_ASSIGN_MACRO(*tbl_id, log.bto_tbl_id);

    log.bto_pg_type = pg_type;
    log.bto_page_size = page_size;
    log.bto_cmp_type = compression_type;
    log.bto_loc_cnt = loc_cnt;
    log.bto_klen = keylen;
    log.bto_leaf_loc_id = leaf_loc_cnf_id;
    log.bto_ovfl_loc_id = ovfl_loc_cnf_id;
    log.bto_leaf_pageno = leaf_pageno;
    log.bto_ovfl_pageno = ovfl_pageno;
    log.bto_mainpage = mainpage;
    log.bto_nextpage = nextpage;
    log.bto_nextovfl = nextovfl;
    log.bto_lrange_len = lrange_len;
    log.bto_rrange_len = rrange_len;
    log.bto_lrtid = *lrange_tid;
    log.bto_rrtid = *rrange_tid;
    log.bto_tidsize = tidsize;

    lg_object[0].lgo_size = ((char *)&log.bto_vbuf[0]) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;

    /*
    ** Set up LG_OBJECT to include range keys.  If any of the range keys
    ** have zero length then exclude them.
    */
    lg_count = 1;
    if (lrange_len)
    {
	lg_object[lg_count].lgo_size = lrange_len;
	lg_object[lg_count].lgo_data = (PTR) lrange_key;
	lg_count++;
    }
    if (rrange_len)
    {
	lg_object[lg_count].lgo_size = rrange_len;
	lg_object[lg_count].lgo_data = (PTR) rrange_key;
	lg_count++;
    }

    status = LGwrite(0, lg_id, lg_count, lg_object, lri, &error);

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.bto_header,
		(flag & DM0L_CLR) ? 0 : log.bto_header.length);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C11_DM0L_BTOVFL, dberr, status));
}

/*{
** Name: dm0l_btfree - Write a Btree Deallocate Overflow Chain log record.
**
** Description:
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Log record flags:
**					    DM0L_JOURNAL - journal the update
**					    DM0L_PHYSICAL - physical locking
**						should be used during recovery
**	tbl_id				Table id.
**	tbl_name			Table Name.
**	tbl_owner			Table Owner.
**	pg_type				Page Format Type.
**	page_size			Page size.
**	compression_type		Key Compression Indicator.
**	loc_cnt				Number of locations in table.
**	ovfl_loc_cnf_id			Identifier for overflow page's location.
**	prev_loc_cnf_id			Identifier for previous page's location.
**	ovfl_pageno			Overflow page number.
**	prev_pageno			Previous page number.
**	mainpage			Page's page_main value.
**	nextpage			Page's bt_nextpage value.
**	nextovfl			Page's page_ovfl value.
**	dupkey_len			Chain's Duplicate Key length.
**	duplicate_key			Chain's Duplicate Key.
**	lrange_len			Low Range Key length.
**	lrange_key			Low Range Key.
**	lrange_tid			Tid value of Low Range Key.
**	rrange_len			High Range Key length.
**	rrange_key			High Range Key.
**	rrange_tid			Tid value of High Range Key.
**	prev_lsn			LSN of compensated log record if CLR.
**	tidsize				TidSize on page
**
** Outputs:
**      lsn				Log record address.
**	err_code			Reason for error return status.
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
**	26-oct-92 (rogerk)
**	    Created for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
**	19-Jan-2004 (jenjo02)
**	    Added tidsize parm for global indexes.
**	03-May-2006 (jenjo02)
**	    Add ixklen.
*/
DB_STATUS
dm0l_btfree(
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
DB_ERROR	*dberr)
{
    DM0L_BTFREE	    log;
    LG_OBJECT	    lg_object[4];
    LG_LRI	    lri;
    i4		    lg_count;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length range keys */
    log.btf_header.length = sizeof(DM0L_BTFREE) -
				(DM1B_KEYLENGTH - lrange_len) -
				(DM1B_KEYLENGTH - rrange_len) -
				(DM1B_KEYLENGTH - dupkey_len);

    log.btf_header.type = DM0LBTFREE;
    log.btf_header.flags = flag;
    if (flag & DM0L_CLR)
	log.btf_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.btf_tbl_id);

    log.btf_pg_type = pg_type;
    log.btf_page_size = page_size;
    log.btf_cmp_type = compression_type;
    log.btf_loc_cnt = loc_cnt;
    log.btf_klen = keylen;
    log.btf_ovfl_loc_id = ovfl_loc_cnf_id;
    log.btf_prev_loc_id = prev_loc_cnf_id;
    log.btf_ovfl_pageno = ovfl_pageno;
    log.btf_prev_pageno = prev_pageno;
    log.btf_mainpage = mainpage;
    log.btf_nextpage = nextpage;
    log.btf_nextovfl = nextovfl;
    log.btf_dupkey_len = dupkey_len;
    log.btf_lrange_len = lrange_len;
    log.btf_rrange_len = rrange_len;
    log.btf_lrtid = *lrange_tid;
    log.btf_rrtid = *rrange_tid;
    log.btf_tidsize = tidsize;
    log.btf_btflags = btflags;

    lg_object[0].lgo_size = ((char *)&log.btf_vbuf[0]) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;

    /*
    ** Set up LG_OBJECT to include range keys.  If any of the range keys
    ** have zero length then exclude them.
    */
    lg_count = 1;
    if (dupkey_len)
    {
	lg_object[lg_count].lgo_size = dupkey_len;
	lg_object[lg_count].lgo_data = (PTR) duplicate_key;
	lg_count++;
    }
    if (lrange_len)
    {
	lg_object[lg_count].lgo_size = lrange_len;
	lg_object[lg_count].lgo_data = (PTR) lrange_key;
	lg_count++;
    }
    if (rrange_len)
    {
	lg_object[lg_count].lgo_size = rrange_len;
	lg_object[lg_count].lgo_data = (PTR) rrange_key;
	lg_count++;
    }

    status = LGwrite(0, lg_id, lg_count, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.btf_header ,
		(flag & DM0L_CLR) ? 0 : log.btf_header.length);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C12_DM0L_BTFREE, dberr, status));
}

/*{
** Name: dm0l_btupdovfl - Write a Btree Update Overflow Chain log record.
**
** Description:
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Log record flags:
**					    DM0L_JOURNAL - journal the update
**					    DM0L_PHYSICAL - physical locking
**						should be used during recovery
**	tbl_id				Table id.
**	tbl_name			Table Name.
**	tbl_owner			Table Owner.
**	pg_type				Page Format Type.
**	page_size			Page size.
**	compression_type		Key Compression Indicator.
**	loc_cnt				Number of locations in table.
**	loc_cnf_id			Identifier for overflow page's location.
**	pageno				Overflow page number.
**	mainpage			Page's page_main value.
**	omainpage			Page's old page_main value.
**	nextpage			Page's bt_nextpage value.
**	onextpage			Page's old bt_nextpage value.
**	lrange_len			Low Range Key length.
**	lrange_key			Low Range Key.
**	lrange_tid			Tid value of Low Range Key.
**	olrange_len			Old Low Range Key length.
**	olrange_key			Old Low Range Key.
**	olrange_tid			Old Tid value of Low Range Key.
**	rrange_len			High Range Key length.
**	rrange_key			High Range Key.
**	rrange_tid			Tid value of High Range Key.
**	orrange_len			Old High Range Key length.
**	orrange_key			Old High Range Key.
**	orrange_tid			Old Tid value of High Range Key.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**      lsn				Log record address.
**	err_code			Reason for error return status.
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
**	26-oct-92 (rogerk)
**	    Created for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
*/
DB_STATUS
dm0l_btupdovfl(
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
DB_ERROR	*dberr)
{
    DM0L_BTUPDOVFL  log;
    LG_OBJECT	    lg_object[5];
    LG_LRI	    lri;
    i4		    lg_count;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length range keys */
    log.btu_header.length = sizeof(DM0L_BTUPDOVFL) -
				(DM1B_KEYLENGTH - lrange_len) -
				(DM1B_KEYLENGTH - rrange_len) -
				(DM1B_KEYLENGTH - orrange_len) -
				(DM1B_KEYLENGTH - olrange_len);

    log.btu_header.type = DM0LBTUPDOVFL;
    log.btu_header.flags = flag;
    if (flag & DM0L_CLR)
	log.btu_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.btu_tbl_id);

    log.btu_pg_type = pg_type;
    log.btu_page_size = page_size;
    log.btu_cmp_type = compression_type;
    log.btu_loc_cnt = loc_cnt;
    log.btu_cnf_loc_id = loc_cnf_id;
    log.btu_pageno = pageno;
    log.btu_mainpage = mainpage;
    log.btu_omainpage = omainpage;
    log.btu_nextpage = nextpage;
    log.btu_onextpage = onextpage;
    log.btu_lrange_len = lrange_len;
    log.btu_rrange_len = rrange_len;
    log.btu_olrange_len = olrange_len;
    log.btu_orrange_len = orrange_len;
    log.btu_lrtid = *lrange_tid;
    log.btu_rrtid = *rrange_tid;
    log.btu_olrtid = *olrange_tid;
    log.btu_orrtid = *orrange_tid;

    lg_object[0].lgo_size = ((char *)&log.btu_vbuf[0]) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;

    /*
    ** Set up LG_OBJECT to include range keys.  If any of the range keys
    ** have zero length then exclude them.
    */
    lg_count = 1;
    if (lrange_len)
    {
	lg_object[lg_count].lgo_size = lrange_len;
	lg_object[lg_count].lgo_data = (PTR) lrange_key;
	lg_count++;
    }
    if (rrange_len)
    {
	lg_object[lg_count].lgo_size = rrange_len;
	lg_object[lg_count].lgo_data = (PTR) rrange_key;
	lg_count++;
    }
    if (olrange_len)
    {
	lg_object[lg_count].lgo_size = olrange_len;
	lg_object[lg_count].lgo_data = (PTR) olrange_key;
	lg_count++;
    }
    if (orrange_len)
    {
	lg_object[lg_count].lgo_size = orrange_len;
	lg_object[lg_count].lgo_data = (PTR) orrange_key;
	lg_count++;
    }

    status = LGwrite(0, lg_id, lg_count, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.btu_header,
		(flag & DM0L_CLR) ? 0 : log.btu_header.length);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C13_DM0L_BTUPDOVFL, dberr, status));
}

/*{
** Name: dm0l_rtdel 	- Write an Rtree Delete log record.
**
** Description:
**	Written when an entry is deleted from an Rtree index page.
**	It logs the update to the index only, not to the actual data page.
**
**	The index Key, the table name and the table owner are logged
**	as variable length objects.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	tbl_name			Table name.
**	name_len			Blank-stripped length of table name
**	tbl_owner			Table owner.
**	owner_len			Blank-stripped length of owner name
**	pg_type				Page Format Type.
**	page_size			Page size.
**	compression_type		Key Compression Indicator.
**	loc_cnt				Number of locations in table.
**	loc_cnf_id			Identifier for index page's location.
**	bid				Page and Line number of old index entry.
**	tid				Tid value in old index entry.
**	key_length			Length of new key.
**	key				Key value of old index entry.
**	stack_length			Size (in bytes) of the ancestor stack.
**	stack				Ancestor stack of TIDPs.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
**
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
**	24-sep-1996 (wonst02)
**	    Added for Rtree.
*/
DB_STATUS
dm0l_rtdel(
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
DB_ERROR	*dberr)
{
    DM0L_RTDEL	    log;
    LG_OBJECT	    lg_object[5];
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length key, ancestor stack */
    log.rtd_header.length = sizeof(DM0L_RTDEL) -
				(RCB_MAX_RTREE_LEVEL * sizeof(DM_TID) -
				 stack_length) -
				(DB_MAXRTREE_KEY - key_length);

    log.rtd_header.type = DM0LRTDEL;
    log.rtd_header.flags = flag;
    if (flag & DM0L_CLR)
	log.rtd_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.rtd_tbl_id);
    log.rtd_pg_type = pg_type;
    log.rtd_page_size = page_size;
    log.rtd_cmp_type = compression_type;
    log.rtd_loc_cnt = loc_cnt;
    log.rtd_cnf_loc_id = loc_config_id;
    log.rtd_hilbertsize = hilbertsize;
    log.rtd_obj_dt_id = obj_dt_id;
    log.rtd_bid = *bid;
    log.rtd_tid = *tid;
    log.rtd_stack_size = stack_length;
    log.rtd_key_size = key_length;

    lg_object[0].lgo_size = ((char *)&log.rtd_vbuf[0]) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = stack_length;
    lg_object[1].lgo_data = (PTR) stack;
    lg_object[2].lgo_size = key_length;
    lg_object[2].lgo_data = key;

    status = LGwrite(0, lg_id, (i4)3, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.rtd_header, (flag & DM0L_CLR) ? 0 :
		(sizeof(DM0L_RTDEL) - (DM1B_KEYLENGTH - key_length)));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C0F_DM0L_BTDEL, dberr, status));
}

/*{
** Name: dm0l_rtput 	- Write an Rtree Put log record.
**
** Description:
**	Written when an entry is inserted into an Rtree index page.
**	It logs the update to the index only, not to the actual data page.
**
**	The index Key, the table name and the table owner are logged
**	as variable length objects.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	tbl_name			Table name.
**	name_len			Blank-stripped length of table name
**	tbl_owner			Table owner.
**	owner_len			Blank-stripped length of owner name
**	pg_type				Page Format Type.
**	page_size			Page size.
**	loc_cnt				Number of locations in table.
**	loc_cnf_id			Identifier for index page's location.
**	bid				Page and Line number of new index entry.
**	tid				Tid value in new index entry.
**	key_length			Length of new key.
**	key				Key value of new index entry.
**	stack_length			Size (in bytes) of the ancestor stack.
**	stack				Ancestor stack of TIDPs.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
**
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
**	24-sep-1996 (wonst02)
**	    Added for Rtree.
*/
DB_STATUS
dm0l_rtput(
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
DB_ERROR	*dberr)
{
    DM0L_RTPUT	    log;
    LG_OBJECT	    lg_object[5];
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    num_objects;

    CLRDBERR(dberr);

    /* The length of the record reflects variable length key, ancestor stack */
    log.rtp_header.length = sizeof(DM0L_RTPUT) -
				(RCB_MAX_RTREE_LEVEL * sizeof(DM_TID) -
				 stack_length) -
				(DB_MAXRTREE_KEY - key_length);

    log.rtp_header.type = DM0LRTPUT;
    log.rtp_header.flags = flag;
    if (flag & DM0L_CLR)
	log.rtp_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.rtp_tbl_id);
    log.rtp_pg_type = pg_type;
    log.rtp_page_size = page_size;
    log.rtp_cmp_type = compression_type;
    log.rtp_loc_cnt = loc_cnt;
    log.rtp_cnf_loc_id = loc_config_id;
    log.rtp_hilbertsize = hilbertsize;
    log.rtp_obj_dt_id = obj_dt_id;
    log.rtp_bid = *bid;
    log.rtp_tid = *tid;
    log.rtp_stack_size = stack_length;
    log.rtp_key_size = key_length;

    lg_object[0].lgo_size = ((char *)&log.rtp_vbuf[0]) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = stack_length;
    lg_object[1].lgo_data = (PTR) stack;
    num_objects = 2;

    if (key_length)
    {
	lg_object[num_objects].lgo_size = key_length;
	lg_object[num_objects++].lgo_data = key;
    }

    status = LGwrite(0, lg_id, num_objects, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.rtp_header, (flag & DM0L_CLR) ? 0 :
		(sizeof(DM0L_RTPUT) - (DB_MAXRTREE_KEY - key_length)));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C0E_DM0L_BTPUT, dberr, status));
}

/*{
** Name: dm0l_rtrep 	- Write an Rtree Replace log record.
**
** Description:
**	Written when an MBR is updated in an Rtree index page.
**	It logs the update to the index only, not to the actual data page.
**
**	The index Key, the table name and the table owner are logged
**	as variable length objects.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	tbl_name			Table name.
**	name_len			Blank-stripped length of table name
**	tbl_owner			Table owner.
**	owner_len			Blank-stripped length of owner name
**	pg_type				Page Format Type.
**	page_size			Page size.
**	loc_cnt				Number of locations in table.
**	loc_cnf_id			Identifier for index page's location.
**	bid				Page and Line number of new index entry.
**	tid				Tid value in new index entry.
**	key_length			Length of new key.
**	key				Key value of new index entry.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	16-jul-1996 (wonst02)
**	    Added for Rtree.
** 	24-sep-1996 (wonst02)
** 	    Modified and renamed to dm0l_rtrep.
*/
DB_STATUS
dm0l_rtrep(
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
DB_ERROR	*dberr)
{
    DM0L_RTREP	    log;
    LG_OBJECT	    lg_object[6];
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    num_objects;

    CLRDBERR(dberr);

    /*
    ** The length of the record reflects the ancestor stack, two variable length
    ** keys.
    */
    log.rtr_header.length = sizeof(DM0L_RTREP) -
				(RCB_MAX_RTREE_LEVEL * sizeof(DM_TID) -
				 stack_length) -
				((DB_MAXRTREE_KEY - key_length) * 2);

    log.rtr_header.type = DM0LRTREP;
    log.rtr_header.flags = flag;
    if (flag & DM0L_CLR)
	log.rtr_header.compensated_lsn = *prev_lsn;

    STRUCT_ASSIGN_MACRO(*tbl_id, log.rtr_tbl_id);
    log.rtr_pg_type = pg_type;
    log.rtr_page_size = page_size;
    log.rtr_cmp_type = compression_type;
    log.rtr_loc_cnt = loc_cnt;
    log.rtr_cnf_loc_id = loc_config_id;
    log.rtr_hilbertsize = hilbertsize;
    log.rtr_obj_dt_id = obj_dt_id;
    log.rtr_bid = *bid;
    log.rtr_tid = *tid;
    log.rtr_stack_size = stack_length;
    log.rtr_okey_size = log.rtr_nkey_size = key_length;

    lg_object[0].lgo_size = ((char *)&log.rtr_vbuf[0]) - ((char *)&log);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = stack_length;
    lg_object[1].lgo_data = (PTR) stack;
    num_objects = 2;

    if (key_length)
    {
	lg_object[num_objects].lgo_size = key_length;
	lg_object[num_objects++].lgo_data = oldkey;
	lg_object[num_objects].lgo_size = key_length;
	lg_object[num_objects++].lgo_data = newkey;
    }

    status = LGwrite(0, lg_id, num_objects, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.rtr_header, (flag & DM0L_CLR) ? 0 :
		(sizeof(DM0L_RTREP) - ((DB_MAXRTREE_KEY - key_length) * 2)));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C0E_DM0L_BTPUT, dberr, status));
}

/*{
** Name: dm0l_disassoc	- Write a change association log record.
**
** Description:
**	The disassociation log record is written when a btree leaf page
**	which has an associated data page is deallocated.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	tbl_id				Table id.
**	page_size			Page size.
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**	14-dec-1992 (rogerk)
**	    Written for Reduced Logging project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Added multiple page size support.
*/
DB_STATUS
dm0l_disassoc(
    i4         lg_id,
    i4         flag,
    DB_TAB_ID       *tbl_id,
    DB_TAB_NAME     *name,
    DB_OWN_NAME     *owner,
    i4         pg_type,
    i4	    page_size,
    i4         loc_cnt,
    i4         loc_config_id,
    DM_PAGENO	    page_number,
    LG_LSN	    *prev_lsn,
    LG_LSN	    *lsn,
    DB_ERROR	    *dberr)
{
    DM0L_DISASSOC   log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;

    CLRDBERR(dberr);

    lg_object.lgo_size = sizeof(log);
    lg_object.lgo_data = (PTR) &log;

    log.dis_header.length = sizeof(DM0L_DISASSOC);
    log.dis_header.type = DM0LDISASSOC;
    log.dis_header.flags = flag;
    if (flag & DM0L_CLR)
	log.dis_header.compensated_lsn = *prev_lsn;

    log.dis_tbl_id = *tbl_id;

    log.dis_pg_type  = pg_type;
    log.dis_page_size = page_size;
    log.dis_loc_cnt = loc_cnt;
    log.dis_cnf_loc_id = loc_config_id;
    log.dis_pageno = page_number;

    status = LGwrite(0, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.dis_header,
		(flag & DM0L_CLR) ? 0 : sizeof(DM0L_DISASSOC));
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C14_DM0L_DISASSOC, dberr, status));
}


/*{
** Name: dm0l_rawdata	- Write a raw data log record.
**
** Description:
**	The raw data log record is used to log arbitrarily large
**	"stuff".  (Where the contents of the "stuff" is presumably
**	known to whoever is doing this.)  Raw data was initially
**	designed so that repartitioning modify could log the partition
**	definition and ppchar arrays, allowing journal redo of the
**	modify.
**
**	It's up to the caller to disassemble the "stuff" into bite
**	size pieces and feed them to us, one piece at a time.  We
**	don't check that the pieces are complete, nor in order.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Standard log record header flags.
**	type				Type of raw data.
**	instance			Occurrence number in case there are
**					n different copies of the same "type"
**					of thing (not used at present)
**	total_size			Total size of the object
**	this_offset			Offset to start of this piece
**	this_size			Size of this piece
**	data				Pointer to the piece
**	prev_lsn			LSN of compensated log record if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
** History:
**	1-Apr-2004 (schka24)
**	    Written for repartitioning modify.
**	7-Jul-2004 (schka24)
**	    Add "not reserved" flag to log write.  We don't need a separate
**	    reserve call since there's never a rawdata CLR, and so we'll
**	    let LG reserve the space for the write.
*/
DB_STATUS
dm0l_rawdata(
    i4		lg_id,
    i4		flag,
    i2		type,
    i2		instance,
    i4		total_size,
    i4		this_offset,
    i4		this_size,
    PTR		data,
    LG_LSN	*prev_lsn,
    LG_LSN	*lsn,
    DB_ERROR	*dberr)
{
    DM0L_RAWDATA log;
    LG_OBJECT lg_object[2];
    LG_LRI	    lri;
    DB_STATUS status;
    CL_ERR_DESC error;

    CLRDBERR(dberr);

    /* Don't bother with log space reservation, as none of the currently
    ** defined rawdata types are ever written as CLR's.  (Apr '04)
    */

    lg_object[0].lgo_size = sizeof(log);
    lg_object[0].lgo_data = (PTR) &log;

    log.rawd_header.length = sizeof(DM0L_RAWDATA) + this_size;
    log.rawd_header.type = DM0LRAWDATA;
    log.rawd_header.flags = flag;
    if (flag & DM0L_CLR)
	log.rawd_header.compensated_lsn = *prev_lsn;

    log.rawd_type = type;
    log.rawd_instance = instance;
    log.rawd_total_size = total_size;
    log.rawd_offset = this_offset;
    log.rawd_size = this_size;

    lg_object[1].lgo_size = this_size;
    lg_object[1].lgo_data = data;

    status = LGwrite(LG_NOT_RESERVED, lg_id, (i4)2, &lg_object[0], &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.rawd_header, 0 /* no reserve */);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
	E_DM9C90_DM0L_RAWDATA, dberr, status));
}

/*{
** Name: dm0l_bsf		- Write begin sidefile log record.
**
** Description:
**      This routine writes a begin sidefile log record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**	prev_lsn			Prev lsn if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**      19-nov-2003 (stial01)
**          Created.
*/
DB_STATUS
dm0l_bsf(
    i4		    lg_id,
    i4		    flag,
    DB_TAB_ID		    *tbl_id,
    DB_TAB_NAME		    *name,
    DB_OWN_NAME		    *owner,
    LG_LSN		    *prev_lsn,
    LG_LSN		    *lsn,
    DB_ERROR		    *dberr)
{
    DM0L_BSF	     log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4         i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.bsf_header.length = sizeof(DM0L_BSF);
    log.bsf_header.type = DM0LBSF;
    log.bsf_header.flags = DM0L_NONREDO | DM0L_JOURNAL; /* FIX ME */
    log.bsf_tbl_id = *tbl_id;
    log.bsf_name = *name;
    log.bsf_owner = *owner;

    if (flag & DM0L_CLR)
	log.bsf_header.compensated_lsn = *prev_lsn;

    lg_object.lgo_size = log.bsf_header.length;
    lg_object.lgo_data = (PTR) &log;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    ** Include LG_RS_FORCE for log force required in undo.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2,
			    2 * log.bsf_header.length, &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM922C_DM0L_MODIFY, dberr, status));
	}
    }

    status = LGwrite(LG_FORCE, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.bsf_header,
		 (flag & DM0L_CLR) ? 0 : log.bsf_header.length);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
    	E_DM9CA0_DM0L_BSF, dberr, status));
}


/*{
** Name: dm0l_esf		- Write end sidefile log record.
**
** Description:
**      This routine writes a end sidefile log record to the log file.
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**	prev_lsn			Prev lsn if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**      19-nov-2003 (stial01)
**          Created.
*/
DB_STATUS
dm0l_esf(
    i4		    lg_id,
    i4		    flag,
    DB_TAB_ID		    *tbl_id,
    DB_TAB_NAME		    *name,
    DB_OWN_NAME		    *owner,
    LG_LSN		    *prev_lsn,
    LG_LSN		    *lsn,
    DB_ERROR		    *dberr)
{
    DM0L_ESF	     log;
    LG_OBJECT	    lg_object;
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4         i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.esf_header.length = sizeof(DM0L_ESF);
    log.esf_header.type = DM0LESF;
    log.esf_header.flags = DM0L_NONREDO | DM0L_JOURNAL; /* FIX ME */
    log.esf_tbl_id = *tbl_id;
    log.esf_name = *name;
    log.esf_owner = *owner;

    if (flag & DM0L_CLR)
	log.esf_header.compensated_lsn = *prev_lsn;

    lg_object.lgo_size = log.esf_header.length;
    lg_object.lgo_data = (PTR) &log;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    ** Include LG_RS_FORCE for log force required in undo.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2,
			    2 * log.esf_header.length, &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM922C_DM0L_MODIFY, dberr, status));
	}
    }

    status = LGwrite(LG_FORCE, lg_id, (i4)1, &lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.esf_header,
		 (flag & DM0L_CLR) ? 0 : log.esf_header.length);
	}

	return (E_DB_OK);
    }

    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
    	E_DM9CA1_DM0L_ESF, dberr, status));
}


/*{
** Name: dm0l_rnl_lsn		- Write log record containing rnl page lsn for
**				  online modify.
**
** Description:
**      This routine writes a log record to the log file, which contains
**	the lsn's of the rnl pages read in by online modify sort processing. 
**
** Inputs:
**      lg_id                           Logging system identifier.
**	flag				Flags: DM0L_JOURNAL.
**	tbl_id				Table identifier.
**	name				Table name.
**	owner				Table owner.
**	prev_lsn			Prev lsn if CLR.
**
** Outputs:
**	lsn				Log record LSN.
**	err_code			Reason for error return status.
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
**      06-apr-2004 (thaju02)
**          Created.
*/
DB_STATUS
dm0l_rnl_lsn(
    i4		    lg_id,
    i4		    flag,
    DB_TAB_ID	    *tbl_id,
    DB_TAB_NAME	    *name,
    DB_OWN_NAME	    *owner,
    LG_LSN	    bsf_lsn,
    DM_PAGENO	    lastpage,
    i4		    lsn_totcnt,
    i4		    lsn_cnt,
    PTR		    rnl_lsn,			
    LG_LSN	    *prev_lsn,
    LG_LSN	    *lsn,
    DB_ERROR	    *dberr)
{
    DM0L_RNL_LSN    log;
    LG_OBJECT	    lg_object[2];
    LG_LRI	    lri;
    DB_STATUS	    status;
    CL_ERR_DESC	    error;
    i4		    i;
    i4		    size = lsn_cnt * sizeof(DMP_RNL_LSN);
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    log.rl_header.length = sizeof(DM0L_RNL_LSN) + size;
    log.rl_header.type = DM0LRNLLSN;
    log.rl_header.flags = DM0L_NONREDO | DM0L_JOURNAL; /* FIX ME */
    log.rl_tbl_id = *tbl_id;
    log.rl_name = *name;
    log.rl_owner = *owner;
    STRUCT_ASSIGN_MACRO(bsf_lsn, log.rl_bsf_lsn);
    log.rl_lastpage = lastpage;
    log.rl_lsn_totcnt = lsn_totcnt;
    log.rl_lsn_cnt = lsn_cnt;

    if (flag & DM0L_CLR)
	log.rl_header.compensated_lsn = *prev_lsn;

    lg_object[0].lgo_size = sizeof(DM0L_RNL_LSN);
    lg_object[0].lgo_data = (PTR) &log;
    lg_object[1].lgo_size = size;
    lg_object[1].lgo_data = (PTR)rnl_lsn;

    /*
    ** If not CLR, reserve logfile space for log and CLR.
    ** Include LG_RS_FORCE for log force required in undo.
    */
    if ((flag & DM0L_CLR) == 0)
    {
	status = LGreserve(LG_RS_FORCE, lg_id, 2,
			    2 * sizeof(DM0L_RNL_LSN) + size, &error);
	if (status != OK)
	{
	    (VOID) uleFormat(NULL, E_DM9054_BAD_LOG_RESERVE, &error,
		    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
		    err_code, 1, 0, lg_id);
	    return(log_error(E_DM9015_BAD_LOG_WRITE, &error,
		lg_id, E_DM922C_DM0L_MODIFY, dberr, status));
	}
    }

    status = LGwrite(LG_FORCE, lg_id, (i4)2, lg_object, &lri, &error);
    *lsn = lri.lri_lsn;

    if (status == OK)
    {
	if (DMZ_SES_MACRO(11))
	{
	    dmd_logtrace(&log.rl_header,
		 (flag & DM0L_CLR) ? 0 : sizeof(DM0L_RNL_LSN));
	}

	return (E_DB_OK);
    }

    /* FIX ME - change DM9C91_DM0L_ESF to something else */
    return (log_error(E_DM9015_BAD_LOG_WRITE, &error, lg_id,
    	E_DM9CA1_DM0L_ESF, dberr, status));
}


/*{
** Name: get_dbname      - get name of database
**
** Description:
**      This routine is called from log_error() to get the name of an
**	inconsistent database for error logging.
**
** Inputs:
**	none
**
** Outputs:
**      db_name				Database name in present session.
**
**      Returns:
**	    E_DB_OK
**	    E_DB_WARN
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	10-sep-1993 (pearl)
**	    Created to implement logging of E_DM0152_DB_INCONSIST_DETAIL
**	    error message.
*/
static DB_STATUS
get_dbname (
DB_DB_NAME *db_name_ptr)
{
    SCF_CB  scf_cb;
    SCF_SCI sci_list[1];
    STATUS  status;
    i4 err_code;

	sci_list[0].sci_length = DB_DB_MAXNAME;
        sci_list[0].sci_code = SCI_DBNAME;
	sci_list[0].sci_aresult = (PTR) db_name_ptr;
	sci_list[0].sci_rlength = 0;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_DMF_ID;
	scf_cb.scf_session = DB_NOSESSION;
    	scf_cb.scf_len_union.scf_ilength = 1;
    	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) &sci_list;
    	if ((status = scf_call(SCU_INFORMATION, &scf_cb)) > E_DB_WARN)
	{
	    uleFormat(&scf_cb.scf_error, 0, (CL_ERR_DESC *)NULL,
		ULE_LOG, (DB_SQLSTATE *)0, (char *)NULL, (i4)0, (i4 *)NULL,
		&err_code, 0);
	}
	return (status);
}

/*{
** Name: dm0l_repl_comp_info - Find offset & length of changed bytes in row
**
** Description:
**	Replace log records are compressed by finding the changed bytes
**	of the log record and logging only the differing bytes rather than
**	two full row images.
**
**	By default a full before image is logged along with enough change
**	information to construct the after image (a diff offset, diff len and
**	changed bytes).  Through use of the DM902 trace point the server will
**	log only the changed bytes of the before and after images.
**
**	This routine calculates the changed bytes in a replace operation and
**	returns pointers to data segments to use in logging the before and
**	after row images.
**
** Inputs:
**	orecord				Pointer to old record
**	l_orecord			Length of old record
**	nrecord				Pointer to new record
**	l_nrecord			Length of new record
**	tbl_id				Table id of updated table
**	otid				Tid of old record
**	ntid				Tid of new record
**
** Outputs:
**	comp_odata			Ptr into old row to use for building
**					before image in replace log record.
**	comp_odata_len			Bytes to log for before image.
**	comp_ndata			Ptr into new row to use for building
**					after image in replace log record.
**	comp_ndata_len			Bytes to log for after image.
**	diff_offset			Offset which gives the first byte in
**					which the old and new rows differ.
**	comp_orow			Describes whether before image
**					compression was used.
**
**      Returns:
**	    VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	18-oct-1993 (jnash)
**	    Created to further reduce loggin.
**	30-oct-1993 (jnash)
**	  - Fix compression problem by not compressing records when new
**	    record is longer than old.
**	11-dec-1993 (rogerk)
**	    Changes to replace log record compression.  Added new routine
**	    parameters.  Routine now returns length and data pointers for
**	    both the before and after image data.  Moved logic to decide what
**	    type of compression to use down into this routine.  Made routine
**	    handle differing length rows more completely (they can now be
**	    compressed) and to correctly handle replaces where the old and
**	    new rows exactly match.
**      23-may-1994 (mikem)
**          Bug #63556
**          Changed interface to dm0l_rep() so that a replace of a record only
**          need call dm0l_repl_comp_info() this routine once.  Previous to
**          this change the routine was called once for LGreserve, and once for
**          dm0l_replace().
*/
VOID
dm0l_repl_comp_info(
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
bool		*comp_orow)
{

    char 	*optr;
    char      	*nptr;
    char   	*end_orec;
    char   	*end_nrec;

    /*
    ** If the new record length is zero (CLR) then no compression can be done.
    */
    if (l_nrecord == 0)
    {
	*comp_odata = (char *) orecord;
	*comp_odata_len = l_orecord;
	*comp_ndata_len = 0;
	*diff_offset = 0;
	*comp_orow = FALSE;
	return;
    }

    /*
    ** Determine if only differing bytes will be logged for both the row before
    ** and after images.  This is enable through trace point DM902 but is not
    ** used in the following circumstances.
    **
    **   - if the old and new records reside on different pages then we log the
    **     full before image to preserve the capability to do page/location
    **     level partial recovery.
    **
    **   - system catalogs are not compressed in this way since they are not
    **     subject to normal "replay of history" rules during rollforward.  It
    **     may turn out to be true in rollforward that the previous version of
    **     the row on disk will not match the previous version that was there
    **     during the original replace operation.
    */
    *comp_orow = FALSE;
    if ((DMZ_XCT_MACRO(2)) &&
	(otid->tid_tid.tid_page == ntid->tid_tid.tid_page) &&
	(tbl_id->db_tab_base > DM_SCONCUR_MAX))
    {
	*comp_orow = TRUE;
    }

    if (delta_start)
    {
        optr = (char *) orecord + delta_start;
        nptr = (char *) nrecord + delta_start;
        end_orec = (char *) orecord + delta_end - 1;
        end_nrec = (char *) nrecord + delta_end - 1;
    }
    else
    {
        optr = (char *) orecord;
        nptr = (char *) nrecord;
        end_orec = (char *) orecord + l_orecord - 1;
        end_nrec = (char *) nrecord + l_nrecord - 1;
    }

    /*
    ** Find the first differing byte.
    */
    while ((optr <= end_orec) && (nptr <= end_nrec))
    {
	if (*optr != *nptr)
	    break;
	optr++;
	nptr++;
    }

    /*
    ** Start from the end of the record, look for the last byte that differs.
    */
    while ((end_orec >= optr) && (end_nrec >= nptr))
    {
 	if (*end_orec != *end_nrec)
	    break;
	end_orec--;
	end_nrec--;
    }

    /*
    ** For the before image, either return just the differing bytes from the
    ** old record or the entire record.
    */
    if (*comp_orow)
    {
	*comp_odata = optr;
	*comp_odata_len = end_orec - optr + 1;
    }
    else
    {
	*comp_odata = (char *) orecord;
	*comp_odata_len = l_orecord;
    }

    /*
    ** For the after image, return just the differing bytes from the new record.
    */
    *comp_ndata = nptr;
    *comp_ndata_len = end_nrec - nptr + 1;

    *diff_offset = optr - (char *)orecord;

    return;
}

/*{
** Name: dm0l_row_unpack	- Uncompress row in replace log record.
**
** Description:
**      Replace log record 'old' and 'new' record images may be compressed.
**	This routine reconstructs the uncompressed rows from compressed data.
**
**	Note that compression here is different than normal data compression
**	applied to ingres tables modified to a compressed storage format.
**	Replace log records are compressed in their own special format.
**
**	Replace records are normally compressed by storing the full before
**	image of the replaced row and only the changed bytes of the after image.
**	However if trace point DM902 was set when the record was logged then
**	only the changed bytes of the before image are logged as well.  This
**	condition is indicated by the comp_orow parameter.
**
**	The caller must supply an input record which is a full (non-compressed)
**	version of the row.  It must represent the value of the row in EITHER
**	its exact pre or post replace state (it must be a complete before or
**	after image).  It is expected that normally the logged before image of
**	the row will be supplied as this input record.  When a full before
**	image is not logged (DM902) it is expected that this input row will be
**	read off of the current data page - which may hold the row in is
**	pre-replace state (redo recovery) or its post-replace state (undo
**	recovery).
**
**	The caller must also supply an output buffer for the uncompressed
**	version of the rows.  The reconstructed row will either be the before
**	image or after image - dependent upon whether diff_data hold the
**	old or new changed bytes.
**
** Inputs:
**	base_record			Input record - either full before or
**					    full after image of row.
**	base_record_len			Length of input record.
**	diff_data			Changed bytes data from log record.
**	diff_data_len			Length of above diff data.
**	res_buf				Buffer into which to reconstruct row.
**	res_row_len			Length of result tuple - the above
**					    res_buf must be at least this size.
**	diff_offset			Offset at which old and new rows differ.
**
** Outputs:
**	res_buf				Filled with reconstructed row image.
**
**      Returns:
**	    VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	18-oct-1993 (jnash)
**	    Created to further reduce loggin.
**	30-oct-1993 (jnash)
**	  - Fix incorrect PTR arithmetic.
**	  - Fix compression problem by not uncompressing records when new
**	    record longer than old.
**	11-dec-1993 (rogerk)
**	    Changes to replace log record compression.  Modified routine to
**	    match changes to dm0l_repl_comp_info.
**	    Row compression now handles differing length rows more completely
**	    and handles replaces where the old and new rows match exactly.
*/
VOID
dm0l_row_unpack(
char		*base_record,
i4		base_record_len,
char		*diff_data,
i4		diff_data_len,
char		*res_buf,
i4		res_row_len,
i4		diff_offset)
{
    char	    *tail_ptr = 0;
    i4	    tail_len;

    /*
    ** Find parts of input record that are equivalent in the old and new rows.
    ** The front part is from the start of the input record to the diff_offset.
    **
    ** The tail end is the number of bytes from the end of the input record
    ** needed to pad out the old and new records to their proper length.
    */
    tail_len = res_row_len - (diff_offset + diff_data_len);
    tail_ptr = base_record + (base_record_len - tail_len);

    /*
    ** Construct the row image from the diff bytes.
    ** First copy the front end of the equivalent data.
    */
    if (diff_offset > 0)
	MEcopy((PTR) base_record, diff_offset, (PTR) res_buf);

    /*
    ** Next copy the differing bytes of the row image.
    */
    if (diff_data_len)
    {
	MEcopy((PTR) diff_data, diff_data_len, (PTR) (res_buf + diff_offset));
    }

    /*
    ** Last, copy the tail end of the equivalent data.
    */
    if (tail_len)
    {
	MEcopy((PTR) tail_ptr, tail_len,
	       (PTR) (res_buf + diff_offset + diff_data_len));
    }

    return;
}
