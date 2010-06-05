/*
** Copyright (c) 1985, 2009 Ingres Corporation
**
**
NO_OPTIM=dr6_us5 pym_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <nm.h>
#include    <er.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <lk.h>
#include    <cx.h>
#include    <lg.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <adulcol.h>
#include    <aduucol.h>
#include    <dmpp.h>
#include    <dmxe.h>
#include    <dmacb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dm0c.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm0s.h>
#include    <dmse.h>
#include    <dm1c.h>
#include    <dm1p.h>
#include    <dm1u.h>
#include    <dm2d.h>
#include    <dm2f.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dmftrace.h>
#include    <dm2u.h>
#include    <dm2rep.h>
#include    <dm0pbmcb.h>
#include    <dm0llctx.h>
#include    <dm0j.h>
#include    <lo.h>
#include    <rep.h>
#include    <scf.h>

/**
**
**  Name: DM2D.C - Database level services.
**
**  Description:
**      This module implements the physical database services required by
**	the logical layer.
**
**          dm2d_add_db - Add db to server.
**	    dm2d_close_db - Closed an opened db.
**	    dm2d_del_db - Delete db from server.
**	    dm2d_open_db - Open an added db.
**	    dm2d_check_db_backup_status - is this DB undergoing online ckp?
**	    dm2d_extend_db - Add a location to the config file.
**		dm2d_unextend_db	- Delete location from config file.
**
**  History:
**      25-nov-1985 (derek)    
**          Created new for jupiter.
**      25-nov-87 (jennifer)
**          Add multiple locations per table support.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb() calls.
**	 7-nov-88 (rogerk)
**	    Added update_tcb() routine to update the core system catalog
**	    TCB's to reflect more acurate tuple and pages counts.
**	04-jan-89 (mmm)
**	    altered dm2d_open_db() to not call update_tcb() in the case
**	    of the RCP.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	28-Feb-89 (rogerk)
**	    Added support for shared buffer manager.
**	    Added DM2D_BMSINGLE flag to add_db, set dcb_bm_served accordingly.
**	    Added calls to dm0p_dbcache when open or close database.
**	    Added locking of database for buffer manager, check for correct
**	    lock value when sole-cache is in use.
**	    Changed tcb hashing to use dcb_id instead of dcb pointer to form
**	    hash value.
**	 3-Mar-89 (ac)
**	    Added 2PC support.
**	 3-mar-89 (EdHsu)
**	    online backup cluster support.
**	27-mar-1989 (rogerk)
**	    Send TOSS flag to DM0P when close database in RCP.
**	    Fixed bug with server database lock mode and checking of database
**	    cache access mode in dm2d_open_db and dm2d_close_db.
**	27-mar-1989 (EdHsu)
**	    Move online backup cluster support at bt level.
**	24-Apr-89 (anton)
**	    Added local collation support
**	15-may-1989 (rogerk)
**	    Added checks for lock and log quota limits while opening database.
**	    Took out double formatting of errors if dm0c_open fails.
**	 5-jun-1989 (rogerk)
**	    Release dcb_mutex semaphore when find INVALID DCB in dm2d_open_db.
**	19-jun-1989 (rogerk)
**	    Toss pages from buffer manager when db opened or closed
**	    exclusively to make sure we don't cache pages after a db is
**	    destroyed or restored from checkpoint.
**	23-Jun-1989 (anton)
**	    Moved local collation routines to ADU from CL
**	 9-Jul-1989 (anton)
**	    Deallocate collation descriptions when a database is closed.
**	25-jul-89 (sandyh)
**	    Added E_DM0133 error for invalid dcb bug 6533.
**	 4-aug-1989 (rogerk)
**	    Put in workaround for compile bug when optimized.
**	14-aug-1989 (rogerk)
**	    Put in call to invalidate buffer manager pages when rollforward
**	    is being used.
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateways.  Define tcb fields in
**	    system catalog TCB's that define logging actions, access methods
**	    to use, secondary index update semantics, and TID support.
**	20-aug-1989 (ralph)
**	    Set DM0C_CVCFG for config file conversion
**	11-sep-1989 (rogerk)
**	    Save lx_id returned from dm0l_opendb call and use it in
**	    dm0l_closedb call.  This prevents us from having to start
**	    a transaction to close a database.
**	 2-oct-1989 (rogerk)
**	    Don't blindly zero out the dcb lockid fields in opendb.  We may
**	    be calling opendb after we already have the db locked (by a
**	    previous call with JLK mode).
**	12-dec-1989 (rogerk)
**	    Fixed unitialized variable bug in dm2d_open_db that caused us to
**	    mistakenly mark db's inconsistent sometimes when the DM2D_NLK
**	    flag was specified.
**	10-jan-1990 (rogerk)
**	    Added DM2D_OFFLINE_CSP flag to dm2d_open_db which indicates that
**	    the database is being opened by the CSP during startup recovery by
**	    the master.
**	25-apr-1990 (fred)
**	    Added initialization of new TCB fields for system catalogs.  This is
**	    done in conjunction with implementing BLOBs.
**	17-may-90 (bryanp)
**	    After we force the database consistent, make a backup copy of the
**	    config file.
**	    When opening the config file for the RFP, if the ".rfc" file
**	    cannot be found, try the ".cnf" file in the dump area.
**	20-nov-1990 (rachael)
**	    BUG 34069:
**	    Added line in dm2d_open_db to set dcb->dcb_status to not journaled
**	    if the cnf->cnf_dsc->dsc_status is not set to journaled
**	28-jan-91 (rogerk)
**	    Added DM2D_IGNORE_CACHE_PROTOCOL flag in order to allow auditdb
**	    to run on a database while it is open by a fast commit server.
**	28-jan-91 (rogerk)
**	    Moved the release of the Database lock in dm2d_close till after
**	    the release of the SV_DATABASE lock to fix "ckpdb +w".  Releasing
**	    the database lock first caused us to get LK_BUSY returned from
**	    ckpdb +w when the database became available.
**	 4-feb-91 (rogerk)
**	    Added new argument to dm0p_toss_pages call.
**	 4-feb-1991 (rogerk)
**	    Added support for fixed table priorities.
**	    Added tcb_bmpriority field.  Call buffer manager to get table
**	    cache priority when tcb is built.
**	25-feb-1991 (rogerk)
**	    Added DM2D_NOLOGGING flag for open_db.  This allows callers to
**	    open databases in READ_ONLY mode without requiring any log
**	    records to be written - so that utilities may connect to
**	    databases while the logfile is full.  This was added as part
**	    of the Archiver Stability Project.
**	    Changed dm0p_gtbl_priority call to take a regular DB_TAB_ID.
**	 7-mar-1991 (bryanp)
**	    Added support for Btree index compression: new fields tcb_data_atts
**	    and tcb_comp_katts_count in the DMP_TCB.
**	22-mar-1991 (bryanp)
**	    B36630: Added dm2d_check_db_backup_status().
**	25-mar-1991 (rogerk)
**	    Check for a database which had no recovery performed on it
**	    due to being in Set Nologging state at the time of a system
**	    crash.  If we are the first opener of a database and the
**	    DSC_NOLOGGING bit is still on, then the database was not
**	    cleaned up properly from a Set Nologging call and must be
**	    considered inconsistent.
**	10-jun-1991 (bryanp, rachael)
**	    B37190: DB ref count wrong when update_tcb() fails in dm2d_open_db.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**	    17-aug-1989 (Derek)
**		Fixed hard-coded constants used to initialize the tcb_ikey_map
**		of the iirel_idx TCB in construct_tcb().  Part of B1 changes.
**	    20-sep-1989 (Derek)
**		Added dm2d_extend_db routine for secure project.  Database
**		location extends are now done through this routine rather
**		than through a front-end program.
**	    25-apr-91 (teresa)
**		add support for flag to indicate that the extended location 
**		directory already exists and that dm2d_extend_db should bypass
**		the DIdircreate call that usually creates it.  This is used
**		primarily for support of convto60, where a v5 extended location
**		is being made known to the v6 installation.
**	    25-apr-91 (teresa)
**		fix bug where lx_id was not being passed to dm0l_closedb or
**		dm0l_opendb
**	    14-jun-1991 (Derek)
**		Added support to automatically update relation records in the
**		config file with the new allocation fields.
**	    18-jun-1991 (Derek)
**		Added new DI_ZERO_ALLOC open flag which indicates that file
**		space must really be allocated at DIalloc time.
**	    17-oct-1991 (rogerk)
**		Added handling in dm2d_check_db_backup_status for tracking
**		the current online backup address.  Moved handling here
**		from dmxe_begin where it was in the 6.5 version.
**	15-oct-1991 (mikem)
**	    Eliminated sun unix compiler warnings in dm2d_extend_db(),
**	    dm2d_check_db_backup_status(), and dm2d_close_db().
**	23-Oct-1991 (rmuth)
**	    Corrected error checking in dm2d_extend_db.
**	06-feb-1992 (bonobo)
**		Added '[0]' to fix "controlling expression must have scalar
**		type" ANSI C errors.
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	7-July-1992 (rmuth)
**	    Prototyping DMF, change name of build_tcb to construct_tcb
**	    to make unique (build_tcb routine also exists in dm2t.c
**	    and the debugger gets confused).
**      29-August-1992 (rmuth)
**          - Removing the file extend 6.4->6.5 on-the-fly conversion code
**            means we can take out all the code which initialised the new file
**            extend  fields, ( relfhdr, relallocation and relextend ).
**	    - Add dbms catalog conversion code .
**	16-jul-1992 (kwatts)
**	    MPF changes. Use of accessors for page access in creating 
**	    templates and other dm1c_xxx calls.
**	9-Sept-1992 (rmuth)
**	    Comment out call to dm2u_update_tables_relfhdr.
**	29-sep-1992 (bryanp)
**	    Release dcb_mutex properly in dm2d_check_db_backup_status.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	16-oct-1992 (rickh)
**	    Initialize attribute defaults.
**	22-oct-1992 (rickh)
**	    Replaced all references to DMP_ATTS with references to the
**	    identical structure DB_ATTS.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project.  Added support for Partial TCB's.
**	    Filled in new table_io fields of tcb.  Adjusted dm2t_release_tcb 
**	    parameters according to new dm2t routine protocols.
**	30-October-1992 (rmuth)
**	    - Remove references to file open flag DI_ZERO_ALLOC, 
**	    - prototyping some routines in DI showed some errors.
**	    - Initialise tbio_lalloc and tbio_lpageno to -1, also set
**	      tbio_checkeof.
**	    - Initialise tbio_tmp_hw_pageno.
**	    - Rename log_error to dm2d_log_error.
**	    - In dm2d_add_db release the mutex on the svcb if we get an 
**	      error converting the iirelation table to new format.
**	    - When converting the iirelation initialise the new fields
**	      relcomptype, relpgtype and relstat2.
**	    - When converting iirelation we used an #ifdef to find out what
**	      pagetype we should use, should use TCB_PG_DEFAULT.
**	02-nov-1992 (kwatts)
**	    In construct_tcb() remove the special VME compatibility code. It
**	    is not needed now we don't get table descriptions from config
**	    files.
**	15-nov-1992 (jnash)
**	    Fix xDEBUG compiler error issued as a result of prior change.
**	05-nov-92 (jrb)
**	    Create work directory for multi-location sorts.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	30-nov-1992 (rmuth)
**	    In dm2d_add_db, release the svcb_mutex if fail converting 
**	    the iiattribute table.
**	14-dec-1992 (jnash)
**	    Reduced Logging Project (phase 2).  Turn on tcb_logging field for
**	    index tcbs now - Index updates are now logged.  Also add
**	    new param to dm0l_location.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	14-dec-92 (jrb)
**	    Removed DM_M_DIREXISTS support since it isn't used.
**	18-jan-1993 (bryanp)
**	    Log LK status code when LK call fails.
**	18-jan-1992 (rmuth)
**	    Prototyping DI, showed up error.
**	18-jan-1993 (rogerk)
**	    Removed check for DCB_S_RCP from closedb processing.  We used to
**	    toss all pages here when we were the RCP or CSP.  We now do this
**	    explicitely from recovery code.
**	9-feb-92 (rickh)
**	    Changed GLOBALCONSTREFs to GLOBALREFs.
**      16-feb-1993 (rogerk)
**	    Added new tcb_open_count field.
**	5-mar-1993 (rickh)
**	    Fill in nullability/defaultability information on upgrades
**	    to 6.5.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed dmxe_abort output parameter used to return the log address
**		of the abortsave log record.
**	    Changed to use LG_S_DATABASE to show the backup state.
**	    Fix compile warnings.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Add dcb_backup_lsn to track the Log Sequence Number of the
**		    start of an online backup.
**		Add dcb parameter to dmxe_abort call.
**		Remove DM2D_CSP and DCB_S_CSP status flags, since it's no longer
**		    required that a DCB be flagged with whether it was built by
**		    a CSP process or not.
**		Added a DM2D_CANEXIST flag to dm2d_add_db. If the database has
**		    already been added we return a ref to its dcb.
**	02-apr-1993 (ralph)
**	    DELIM_IDENT
**	    Update the attribute name field for each DB_ATTS entry on the
**	    TCB in update_tcb() to ensure it is in the proper character case.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	    Correct argument in call to dm2r_release_rcb
**	17-may-1993 (ralph)
**	    DELIM_IDENT:
**	    Moved code from update_tcb() that updates tcb_atts info with data
**	    from the iiattribute catalog into a new routine, update_attdb().
**	    This was done because iiattribute could not be read reliably
**	    until update_tcb() was executed for iiattribute.  The calls to
**	    update_tcbatts() are placed after all calls to update_tcb().
**	21-jun-1993 (rogerk)
**	    Re-Correct argument in call to dm2r_release_rcb
**	21-jun-1993 (jnash)
**	    Move error checking after call to construct_tcb to
**	    avoid access violation if rel_rcb not built.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    We no longer log a closedb log record, which means that we no
**		longer need to pass a lx_id argument to dm0l_opendb and to
**		dm0l_closedb.
**	    Replace uses of DM_LKID with LK_LKID.
**	26-jul-1993 (jnash)
**	    Log complete path name in dm0l_location().  Add new error 
**	    message at failure in creating directory during a database extend.
**	23-July-1993 (rmuth)
**	    6.4->6.5 conversion needs to update the iirelation.relatts count
**	    for iiattribute. Added code to do this.
**	27-jul-1993 (rmuth)
**	    VMS could not calculate the number of elements in 
**	    DM_core_relations from another module. Add a globaldef
**	    DM_sizeof_core_rel to hold this value.
**	23-aug-1993 (rogerk)
**	    Turn on DCB_S_RECOVER flag when DM2D_RECOVER is specified in opendb.
**	    Changed DCB_S_RCP to DCB_S_ONLINE_RCP since it is set only during
**	    online recovery.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**      10-sep-1993 (pearl)
**          In dm2d_open_db(), add error E_DM0152_DB_INCONSIST_DETAIL if
**          database is inconsistent; this error will name the database in
**          question.
**	20-sep-1993 (rogerk)
**	    Fix to updating of relatts value in iiattribute's relation row in
**	    dm2d_convert_iirel_from_64 routine.
**	01-oct-93 (jrb)
**	    Added logging of warning message if we are opening the iidbdb and
**	    journaling is not enabled.
**	12-oct-93 (jrb)
**          Added dm2d_alter_db for MLSorts.
**	18-oct-1993 (jnash)
**	    Fix recovery problem where update_tcbatts() searched beyond
**	    the range of valid iiattribute pages, resulting in a 
**	    buffer manager fault page error.
**	22-nov-93 (jrb)
**	    Moved 10/01 change to dmc_open_db instead.
**	31-jan-94 (jnash)
**	    Add E_DM92A2_DM2D_LAST_TBID error.  This is xDEBUG code added
**	    because we don't understand when this condition can legally arise. 
**	21-apr-1994 (rmuth)
**	    b62335, whilst converting the iirelation table to 6.5 format
**	    take the opportunity to set the the relloccount. In earlier
**	    release this did not exist and is setup in memory on the fly
**	    while the disk version holds rubbish. The above was error
**	    prone as requires the DBMS to never read the disk tuple 
**	    straight into the the memory tcb without the fixup. Surprise,
**	    surpise this did happen with death-like results. 
**	23-may-1994 (andys)
** 	    Added dm2d_check_dir - Check if db directory is present
**	    [bug 60702]
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	20-jun-1994 (jnash)
**	    fsync project.
** 	    - Add DM2D_SYNC_CLOSE dm2d_open_db() open mode.  Thus far used 
**	      only by rollforwarddb, this causes database files to be opened 
**	      without file synchronization, permitting async writes by the 
**	      (Unix) file system.  Files are ultimately sync'd in 
**	      dm2t_release_tcb(). 
** 	    - Initialize tbio_sync_close in construct_tcb(). 
**	    - Eliminate maintenance of obsolete tcb_logical_log.
**	09-mar-1995 (shero03)
**	    Bug B67276
**	    Examine the usage when checking logical location names.
**      23-mar-1995 (chech02)
**          b67533, dm2d_convert_DBMS_cats_to_65(), update relpages of 
**          iirelation too.
**	14-nov-1995 (angusm)
**	    Modify previous change for bug 67276 so that it prevents
**	    bug 71652.
**	4-mar-1996 (angusm)
**	    modify change for bug 67276 in dm2d_extend_db to prevent bug 72050.
**	15-feb-1996 (tchdi01)
**	    Set the "production mode" flags in dm2d_open_db().
**	06-mar-1996 (stial01 for bryanp)
**	    Use tcb_rel.relpgsize, not sizeof(DMPP_PAGE) for variable page sizes
**	    If relpgsize is 0 for core catalogs, force it to be DM_PG_SIZE.
**	    Set tbio_page_size from tcb_rel.relpgsize.
**	    Add page_size argument to add_record, pass page_size to dmpp_format.
**      06-mar-1996 (stial01)
**          update_tcb: If relpgsize is 0 for core catalogs, force it to be
**          DM_PG_SIZE.
**          Pass page size to dm2f_build_fcb()
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	20-may-1996 (ramra01)
**	    Added new param to the load accessor
**	20-may-96 (stephenb)
**	    Add code for replcation: clean up and force outstanding 
**	    replications when the DB is deleted from the server in close_db,
**	    and check for replication and get info from replicator
**	    catalogs in open_db.
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**	24-jul-96 (stephenb)
**	    Fix problems shown up by integration to head rev.
**	5-aug-96 (stephenb)
**	    Add replication recovery processing on db open.
**      18-jul-1996 (ramra01 for bryanp)
**          Build new DB_ATTS fields for Alter Table support.
**          Set up tcb_total_atts when
**              building TCBs for core system catalogs. This implementation
**              assumes that core system catalogs cannot be altered, and
**              thus there are never any dropped or modified attribute records
**      01-aug-1996 (nanpr01 for ICL phil.p)
**         Introduced support for Distributed Multi-Cache Management (DMCM)
**         protocol.
**         When running DMCM, database and TCBs all marked as supporting
**         the protocol. Database marked as FastCommit for the purposes of 
**         logging.
**	15-Oct-1996 (jenjo02)
**	    Compose dcb_mutex name using dcb_name, tcb_mutex, tcb_et_mutex 
**	    using table name, to uniquify for iimonitor, sampler.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Added hcb_tq_mutex to protect TCB free queue.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**	17-Jan-1997 (jenjo02)
**	    Replaced svcb_mutex with svcb_dq_mutex (DCB queue) and
**	    svcb_sq_mutex (SCB queue).
**	21-Jan-1997 (jenjo02)
**	    Call dm2t_alloc_tcb() to allocate and initialize TCB instead of
**	    doing it ourselves.
**	23-jan-96 (hanch04)
**	    Added new info for OI2.0 upgrade
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Changed parameters in dm2t_open() call.
**      27-feb-97 (stial01)
**          If SINGLE-SERVER mode, get SV_DATABASE locks on SINGLE-SERVER
**          lock list (svcb_ss_lock_list).
**	28-feb-1997 (cohmi01)
**	    In dm2d_open_db(), keep track in hcb of tcbs created, for 
**	    bugs 80242, 80050.
** 	08-apr-1997 (wonst02)
** 	    Pass back the access mode from the .cnf file.
**	15-May-1997 (jenjo02)
**	    When requesting database lock, use an interruptable LKrequest()
**	    (dm2d_open_db()).
**	30-may-1997 (wonst02)
**	    More changes for readonly database.
** 	19-jun-1997 (wonst02)
** 	    Added DM2F_READONLY flag for dm2f_build_fcb (readonly database).
**	12-aug-1997 (wonst02)
**	    Bug 84405: ckpdb dbname fails if the database was created with 
**	    alternate location. Removed offending code from dm2d_open_db.
**	14-aug-97 (stephenb)
**	    must check for upercase local_db field when replicating an
**	    sql92 compliant installation
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**      22-Aug-1997 (wonst02)
**          Added parentheses-- '==' has higher precedence than bitwise '&':
**	    ((cnf->cnf_dsc->dsc_access_mode & DSC_READ) == 0), not:
**	    (cnf->cnf_dsc->dsc_access_mode & DSC_READ == 0)
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm0p_toss_pages(), dm2t_release_tcb() calls.
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**      23-Mar-1998 (linke01)
**          Added NO_OPTIM for pym_us5
**	06-May-1998 (jenjo02)
**	    Protect hcb_tcbcount with hcb_tq_mutex to guarantee its accuracy.
**	    Set tbio_cache_ix when initializing a TCB, track TCB allocation
**	    counts in HCB by cache.
**      23-Mar-1998 (linke01)
**          rollforwarddb failure in backend lar test, added NO_OPTIM for 
**          pym_us5.
**	1-jun-98 (stephenb)
**	    Add parm to dm2rep_qman() calls.
**      28-may-1998 (stial01)
**          Support VPS system catalogs.
**          Added recovery for DM0LSM1RENAME during database open.
**          Added dm2d_row_lock()
**      27-jul-98 (dacri01)
**          Make extenddb fail immediately if it cannot get a lock. Bug 91170.
**          dm2d_extend_db().
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	09-Mar-1999 (schte01)
**       Changed assignment of j within for loop so there is no dependency on 
**       order of evaluation (once in dm2d_close_db routine and once in 
**       dm2d_open_db).
**      15-Mar-1999 (hanal04)
**          Use DM0C_TIMEOUT to flag the config file lock request to
**          have a timeout value when adding a database. b55788.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	10-may-1999 (walro03)
**	    Remove obsolete version string i39_vme.
** 	21-may-1999 (wonst02)
** 	    In construct_tcb, pass "tcb" to dm0m_deallocate() instead of "&t"
** 	    if an error, so the caller's variable gets zeroed.
**      27-May-1999 (hanal04)
**          Removed and reworked the above change for bug 55788. Added
**          new parameter 'lock_list'. The caller supplies the lock list id 
**          and specifies DM2D_CNF_LOCKED if the CNF file is already locked. 
**          b97083.
**      21-Jul-1999 (wanya01)
**          Add parameter flag and lock_list to dm2d_convert_iirel_from_64()
**          and dm2d_convert_iiatt_from_64(). Do not try to lock CNF if it's
**          already locked. (Bug 98010) 
**      16-aug-1999 (nanpr01)
**          Implementation of SIR 92299. Enabaling journalling on a
**          online checkpoint. Initialize the dcb_eback_lsn and also
**	    in dm2d_check_backup_status routine make sure the first
**	    transaction which notices the online checkpoint, updates
**	    the dcb.
**	21-oct-1999 (nanpr01)
**	    More CPU cycle optimization. Do not pass the tuple header in
**	    dmpp_get function if not needed.
**	11-Nov-1999 (hanch04)
**	    Check for SVCB_SINGLEUSER.  We are not accessing a read only
**	    database for the server type utilities, ckpdb, rolldb, etc.
**	16-Nov-1999 (jenjo02)
**	    Include test of TCB_BUSY when deciding to wait for a TCB
**	    prior to calling dm2t_release_tcb(). dm2t_release_tcb()
**	    sets TCB_BUSY and releases the protecting mutex even
**	    when tcb_valid_count and tcb_ref_count are zero.
**	30-Nov-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID* to dmxe_begin(), LKcreate_list() prototypes.
**	15-Dec-1999 (jenjo02)
**	    Removed DB_DIS_TRAN_ID parm from LKcreate_list() prototype.
**      06-Mar-2000 (hanal04) Bug 100742 INGSRV 1122.
**          Prevent semaphore deadlock on DCB mutex using new dcb_status
**          flag DCB_S_MLOCKED.
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	    update_tcb() Remove temp code mistakenly added with change 448799
**	20-Mar-2001 (inifa01) Bug 104267 INGREP 92.
**	    Prevent E_DM003F error from occuring on db open of a
**	    replicated database due to dcb->dcb_ref_count not being
**	    bumped down when input queue processing fails in 
**	    dm2d_open_db.
**	10-May-2001 (inifa01) Bug 104696 INGREP 94.
**	    Prevent E_DM937E_TCB_BUSY from occuring during db close
**	    of replicated database due to unnecessary unfix of TCB
** 	    in dm2d_open_db via dm2t_close() call.
**	19-Jun-2001 (inifa01) Bug 105037 INGSRV 1477.
**	    Prevent E_CL2517, semaphore deadlock on DCB mutex, from occuring
**	    Check that caller doesn't already hold mutex.
**	22-mar-2001 (stial01)
**          dm2d_convert_iirel_to_V6 upgrades iirelation for ingres 2.6
**      22-mar-2001 (stial01)
**          dm2d_convert_iirel_to_V6 was using wrong pgtype,pgsize args for get
**          and using wrong pgsize arg for DIopen
**      28-mar-2001 (stial01)
**	    dm2d_convert_iirel_to_V6() pass valid tranid ptr to dmpp_put
**      24-aug-2001 (stial01)
**          dm2d_convert_iirel_to_V6() check for page size zero (B105677)
**      03-oct-2001 (stial01)
**          dm2d_convert_iirel_from_64() fixed args to dmpp_format() (b105956)
**	28-Feb-2002 (jenjo02)
**	    Removed LK_MULTITHREAD from LKrequest/release flags. 
**	    MULTITHREAD is now an attribute of the lock list.
**	01-May-2002 (bonro01)
**	   Fix memory overlay caused by using wrong length for char array
**	09-sep-2002 (devjo01)
**	    All databases opened in a clustered environment MUST use
**	    DMCM protocols.
**      20-Sep-2002 (stial01)
**         dm2d_add_db() Moved dbservice warning here from dm0c_open
**	14-aug-2003 (jenjo02)
**	   dm2t_release_tcb() *tcb parm changed to **tcb, nullified
**	   on function return.
**	02-oct-2003 (somsa01)
**	    Included a64_win in 64-bit lists.
**	18-Dec-2003 (schka24)
**	   Detect v6 databases, convert to v7; temp hack for existing v6
**	   databases.
**	16-Jan-2004 (schka24)
**	   Changes for partitioned table project.
**	6-Feb-2004 (schka24)
**	    Update calls to dm2f-filename.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	29-Apr-2004 (gorvi01)
**		Added dm2d_unextend_db. This function deletes location from 
**	        config file.
**	14-May-2004 (hanje04)
**	    Added prototype for dm2d_convert_preV7_iirel to quiet compiler 
**	    warnings about conflicts with implicit declarations.
**	25-Aug-2004 (schka24)
**	    Rework database core catalog conversions a bit to simplify,
**	    implement actual iiattribute conversion to V7.
**	29-Dec-2004 (schka24)
**	    Mods for converting to V8 core catalogs.  Since V8 is merely
**	    grabbing a little more of attfree, the changes are minor.
**      08-Mar-2005 (gorvi01)
**          Modified the removal of entries from config file to solve bug
**          related to infodb(bug#114030). Added check of loc->flags & loc_type
**	    during refind.
**      14-jul-2005 (stial01)
**          dm2d_release_user_tcbs() if DMCM, make sure page/row counts are
**          updated (b114846)
**      14-jul-2005 (stial01)
**          minor fix to previous change for b114846
**      10-aug-2005 (stial01)
**          dm2d_convert_preV7_iiatt() Fixed clearing of SEC_LBL (b115020)
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**      22-Jan-2007 (hanal04) Bug 117533
**          Silence compiler warning messages under hpb_us5.
**	13-apr-2007 (shust01)
**	    server crash when trying to access database while an external
**	    program is locking the physical database files. b118043 .
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**      01-Nov-2006 (horda03) Bug 117032
**          If the lock request for the LK_SV_DATABASE lock for a DB returns
**          LK_VAL_NOTVALID, then assume the lock request was successful.
**      02-Apr-2007 (horda03) Bug 117032
**          Amendment to above change, convert a LK_VAL_NOTVALID cl_status to
**          OK, for futher processing.
**      04-mar-2008 (stial01)
**          dm2d_open_db() Allow verifydb -oforce_consistent after incremental
**          rollforwarddb.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**      14-may-2008 (stial01)
**          dm2d_open_db() remove 04-mar-2008 change
**      16-may-2008 (stial01)
**          upgrade must check dsc_version AND dsc_status to know what to do.
**      27-jun-2008 (stial01)
**          Added param to dm0l_opendb call.
**	18-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm2rep_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**      07-apr-2009 (stial01)
**          Added new upgrade routines
**      14-apr-2009 (stial01)
**          Use new upgrade routines which separate read/write logic
**          from upgrade row logic. Distinguish rewrite vs. inplace upgrade.
**          Test upgrade core catalogs by setting II_UPGRADEDB_DEBUG
**      21-apr-2009 (stial01)
**          Use DB_OLDMAXNAME_32 instead of constant 32
**      15-jun-2009 (stial01)
**          Fixed 2.6 (V3) -> V8 upgrade of core catalogs
**       8-Jun-2008 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Set DCB_S_MUST_LOG if caller has DM2D_MUST_LOG to allow
**          SET NOLOGGING to be blocked if required.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**	21-Oct-2009 (kschendel) SIR 122739
**	    Integrate changes needed for new rowaccessor scheme
**	    (mostly, tlv's are gone, get plv's only.)
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	01-apr-2010 (toumi01) SIR 122403
**	    For encryption add attencflags and att_v9.attencwid.
**      14-Apr-2010 (hanal04) SIR 123574
**          Check if the DB was created with the new -m flag (DU_MUSTLOG
**          set in dsc_dbaccess). If so, set DCB_S_MUST_LOG in the dcb_status.
*/

/*
** globals
*/
GLOBALREF       DMC_REP         *Dmc_rep;
/*
**  Defines of other constants.
*/

/*
**  Values assigned to SV_DATABASE lock to enforce database/cache access mode.
*/
#define                 MULTI_CACHE	    1
#define                 SINGLE_CACHE	    2

/* Static definitions of replicator iirelation tables: */
typedef struct _REP_TABLE
{
    i4		RepTabId;
    char	*RepTabName;
    char	*RepTabNameU;
} REP_TABLE;

#define	IdDD_DATABASES		0
#define	IdDD_TRANSACTION_ID	1
#define	IdDD_REGIST_TABLES	2
#define	IdDD_REGIST_COLUMNS	3
#define	IdDD_REG_TBL_IDX	4
#define	IdDD_DB_NAME_IDX	5
#define	IdDD_INPUT_QUEUE	6
#define	IdDD_PATHS		7
#define	IdDD_DB_CDDS		8
#define	IdDD_DISTRIB_QUEUE	9

#define NumRepTables		10

static  REP_TABLE RepTables[NumRepTables] =
{
    IdDD_DATABASES,DD_DATABASES,DD_DATABASES_U,
    IdDD_TRANSACTION_ID,"dd_transaction_id","DD_TRANSACTION_ID",
    IdDD_REGIST_TABLES,DD_REGIST_TABLES,DD_REGIST_TABLES_U,
    IdDD_REGIST_COLUMNS,DD_REGIST_COLUMNS,DD_REGIST_COLUMNS_U,
    IdDD_REG_TBL_IDX,DD_REG_TBL_IDX,DD_REG_TBL_IDX_U,
    IdDD_DB_NAME_IDX,DD_DB_NAME_IDX,DD_DB_NAME_IDX_U,
    IdDD_INPUT_QUEUE,DD_INPUT_QUEUE,DD_INPUT_QUEUE_U,
    IdDD_PATHS,DD_PATHS,DD_PATHS_U,
    IdDD_DB_CDDS,DD_DB_CDDS,DD_DB_CDDS_U,
    IdDD_DISTRIB_QUEUE,DD_DISTRIB_QUEUE,DD_DISTRIB_QUEUE_U
};


/*}
** Name: DMP_OLDATTRIBUTE - format of old iiattribute tuple
**
** Description:
**      format of 6.4 iiattribute tuple 
**
** History:
**      28-sep-92 (ed)
**          initial creation for DB_MAXNAME conversion
[@history_template@]...
*/
typedef struct _DMP_OLDATTRIBUTE
{
     DB_TAB_ID       attrelid;              /* Identifier associated with 
                                            ** this table. */
     i2              attid;                 /* Attribute ordinal number. */
     i2		     attxtra;		    /* underlaying GATEWAY data type.
					    ** The INGRES data type may not
					    ** match the gateway data type.  The
					    ** values that are used in this
					    ** field will vary from gateway to
					    ** gateway.
					    */
     char	     attname[24];	    /* Attribute name. */
     i2              attoff;                /* Offset in bytes of attribute
                                            ** in record. */
     i2              attfmt;                /* Attribute type. */
     i2              attfml;                /* Length in bytes. */
     i2              attfmp;		    /* Precision for unsigned integer.*/
     i2              attkey;                /* Flag used to indicate this is
                                            ** part of a key. */
     i2              attflag;                  /* Used to indicate types. */
} DMP_OLDATTRIBUTE;

typedef struct _DMP_RELATION_V8	/* Ingres 2006r3 DB_MAXNAME 32 */
{
     DB_TAB_ID	     reltid;		    /* Table identifier. */
     char            relid[DB_OLDMAXNAME_32];  /* Table name. */
     char            relowner[DB_OLDMAXNAME_32];/* Table owner. */
     i2              relatts;               /* Number of attributes in table. */
     i2		     reltcpri;		    /* Table Cache priority */
     i2		     relkeys;		    /* Number of attributes in key. */
     i2              relspec;               /* Storage mode of table. */
     u_i4            relstat;               /* Status bits of table. */
     i4              reltups;               /* Approximate number of records */
     i4              relpages;              /* Approximate number of pages */
     i4              relprim;               /* Number of  primary pages */
     i4              relmain;               /* non-index(isam only) pages*/
     i4              relsave;               /* how long to save table */
     DB_TAB_TIMESTAMP relstamp12;           /* Create or modify Timestamp */
     char            relloc[DB_OLDMAXNAME_32]; /* Table location */
     i4              relcmptlvl;	    /* DMF version of table. */
     i4              relcreate;             /* Date table created. */
     i4              relqid1;               /* Query id if table is a view. */
     i4              relqid2;               /* Query id if table is a view. */ 
     i4              relmoddate;            /* Date table last modified. */
     i2              relidxcount;           /* Number of indices on table. */ 
     i2              relifill;              /* Modify index fill factor. */
     i2              reldfill;              /* Modify data fill factor. */
     i2              rellfill;              /* Modify leaf fill factor. */
     i4              relmin;                /* Modify min pages value. */
     i4              relmax;                /* Modify max pages value. */
     i4		     relpgsize;		    /* page size of this relation */
     i2              relgwid;	            /* gateway id */
     i2              relgwother;            /* gateway id */ 
     u_i4            relhigh_logkey;        /* Most significant i4 */
     u_i4            rellow_logkey;         /* Least significant i4 */
     u_i4	     relfhdr;
     u_i4	     relallocation;
     u_i4	     relextend;
     i2		     relcomptype;	    /* Type of compression employed */
     i2		     relpgtype;		    /* Page format */
     u_i4	     relstat2;
     char	     relfree1[8];	    /* Reserved for future use */
     i2              relloccount;	    /* Number of locations. */
     u_i2	     relversion;	    /* metadata version  */
     i4              relwid;		    /* Width */
     i4              reltotwid;             /* Tot Width */
     u_i2	     relnparts;		    /* Total physical partitions */
     i2		     relnpartlevels;	    /* Number of partitioning levels */
     char            relfree[8];            /* Reserved for future use. */
} DMP_RELATION_V8;        


typedef struct _DMP_ATTRIBUTE_V8	/* Ingres 2006r3 DB_MAXNAME 32 */
{
     DB_TAB_ID       attrelid;              /* Table id */ 
     i2              attid;                 /* Attribute ordinal number. */
     i2		     attxtra;		    /* underlaying GATEWAY data type */
     char	     attname[DB_OLDMAXNAME_32];/* Attribute name */
     i4              attoff;                /* Attribute offset */
     i4              attfml;                /* Attribute length */
     i2              attkey;                /* part of key */
     i2              attflag;               /* flags */
     DB_DEF_ID	     attDefaultID;	    /* index in iidefaults */
     u_i2	     attintl_id;            /* internal row identifier */
     u_i2	     attver_added;          /* version row added       */
     u_i2	     attver_dropped;        /* version row dropped     */
     u_i2	     attval_from;           /* previous intl_id value  */
     i2              attfmt;                /* Attribute type. */
     i2              attfmp;		    /* Precision for unsigned integer.*/
     u_i2	     attver_altcol;         /* version row altered */
     u_i2	     attcollID;		    /* explicitly declared Collation */
     char	     attfree[16];	    /*   F R E E   S P A C E   */
} DMP_ATTRIBUTE_V8;

typedef struct _DM2D_UPGRADE
{
    i4		upgr_create_version;
    i4		upgr_iirel;
    i4		upgr_iiatt;
    i4		upgr_iiindex;
#define UPGRADE_REWRITE 1
#define UPGRADE_INPLACE 2
#define UPGRADE_DEBUG	3
    i4		upgr_dbg_env; /* value specified by II_UPGRADEDB_DEBUG */
} DM2D_UPGRADE;


/*
**  Forward and/or External function references.
*/

static DB_STATUS 	construct_tcb(
				i4		lock_list,
				DMP_RELATION	*relation,
				DMP_ATTRIBUTE	*attribute,
				DMP_DCB		*dcb,
				DMP_TCB		**tcb,
				DB_ERROR	*dberr );

static DB_STATUS 	update_tcb(
				DMP_TCB	    	*tcb,
				i4	    	lock_list,
				i4	    	flag,
				DB_ERROR	*dberr );

static DB_STATUS 	update_tcbatts(
				DMP_TCB	    	*tcb,
				i4	    	lock_list,
				i4	    	flag,
				DB_ERROR	*dberr );

static PTR 		dm2d_reqmem(
				u_i2		tag,
				SIZE_TYPE	size,
				bool		clear,
				STATUS		*status );

static VOID 		dm2d_free(
    				PTR		*obj );


static  VOID            dm2d_cname(
                                char               *oname,
                                char               *iname,
				i4		   oname_len);

static  STATUS          add_record(
				i4		page_type,
				i4		page_size,
                                DMPP_PAGE       *page,
                                char            *record,
                                i4         record_size,
                                DM_TID          *tid,
                                DI_IO           *di_context,
                                DMPP_ACC_PLV    *local_plv,
				DB_ERROR	*dberr );

static VOID 		dm2d_log_error(
    				i4     terr_code,
				DB_ERROR     *dberr );

static DB_STATUS 	dm2d_convert_core_table_to_65(
    				DMP_TCB     *tcb,
				i4     lock_list,
				DB_ERROR	*dberr );

static DB_STATUS 	dm2d_convert_DBMS_cats_to_65 (
    				DMP_DCB             *dcb,
	    			i4             lock_list,
				DB_ERROR	*dberr );

static DB_STATUS 	dm2d_convert_core_cats_to_65(
			        DMP_TCB		*rel_tcb,
			        DMP_TCB		*relx_tcb,
			        DMP_TCB		*att_tcb,
			        DMP_TCB		*idx_tcb,
			        i4		lock_list,
				DB_ERROR	*dberr );


static VOID
makeDefaultID(
    DB_DEF_ID		*defaultID,
    DMP_OLDATTRIBUTE	*oldTuple
);

static STATUS   	list_func(VOID);	

static DB_STATUS
dm2d_convert_inplace_iirel (
			   DM2D_UPGRADE    *upgrade_cb,
 			   DMP_DCB	    *db_id,
			   i4              flag,
			   i4              lock_list,
			   DB_ERROR	   *dberr );

static DB_STATUS
dm2d_convert_preV9_iirel (
 			   DMP_DCB	    *db_id,
			   i4		   create_version,
			   i4              flag,
			   i4              lock_list,
			   DB_ERROR	   *dberr );

static DB_STATUS
dm2d_convert_preV9_iiatt (
 			   DMP_DCB	    *db_id,
			   i4		   create_version,
			   i4              flag,
			   i4              lock_list,
			   DB_ERROR	   *dberr );

static DB_STATUS
dm2d_convert_preV9_iiind (
 			   DMP_DCB	    *db_id,
			   i4		   create_version,
			   i4              flag,
			   i4              lock_list,
			   DB_ERROR	   *dberr );

static DB_STATUS dm2d_upgrade_rewrite_iiatt(
			    DM2D_UPGRADE *upgrade_cb,
			    DMP_DCB	    *db_id,
			    i4              flag,
			    i4              lock_list,
			    DB_ERROR	    *dberr );

static VOID upgrade_iiatt_row(
			    DM2D_UPGRADE *upgrade_cb,
			    char		*old_att,
			    DMP_ATTRIBUTE	*new_attp);

static DB_STATUS dm2d_upgrade_rewrite_iirel(
			    DM2D_UPGRADE *upgrade_cb,
			    DMP_DCB	    *db_id,
			    i4              flag,
			    i4              lock_list,
			    DB_ERROR	    *dberr );

static VOID upgrade_iirel_row(
			    DM2D_UPGRADE *upgrade_cb,
			    char		*input_rel,
			    DMP_RELATION	*new_relp,
			    i4			dsc_status);

static VOID upgrade_core_reltup(
			    DM2D_UPGRADE *upgrade_cb,
			    DMP_RELATION *core);


/*
**  The following tables (defined in dmmcre.c) contain the initialized records
**  for the core system tables at database creation time. They are used as a
**  bootstrap to open the system tables during a database open.
*/

GLOBALREF i4 DM_sizeof_core_rel;
GLOBALREF DMP_RELATION DM_core_relations[];
GLOBALREF DMP_ATTRIBUTE DM_core_attributes[];	/* Normal (lower) case */
GLOBALREF DMP_ATTRIBUTE DM_ucore_attributes[];	/* Uppercase */

/*}
** Name: DMP_OLDRELATION - format of old iirelation tuple
**
** Description:
**      format of 6.4 iirelation tuple 
**
** History:
**      28-sep-92 (ed)
**          initial creation for DB_MAXNAME conversion
**	30-Jun-2004 (schka24)
**	    edit lost relfree1, restore
[@history_template@]...
*/
typedef struct _DMP_OLDRELATION
{
     DB_TAB_ID	     reltid;		    /* Table identifier. */
     char	     relid[24];		    /* Table name. */
     char	     relowner[24];	    /* Table owner. */
     i2              relatts;               /* Number of attributes in table. */
     i2              relwid;                /* Width in bytes of table record.*/
     i2		     relkeys;		    /* Number of attributes in key. */
     i2              relspec;               /* Storage mode of table. */
     i4              relstat;               /* Status bits of table. */
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
     char	     relloc[24];	    /* ingres location of a table. */
     i4              relcmptlvl;            /* DMF version of table. no byte 
					    ** alignment */
     i4              relcreate;             /* Date table created. */
     i4              relqid1;               /* Query id if table is a view. */
     i4              relqid2;               /* Query id if table is a view. */ 
     i4              relmoddate;            /* Date table last modified. */
     i2              relidxcount;           /* Number of indices on table. */ 
     i2              relifill;              /* Modify index fill factor. */
     i2              reldfill;              /* Modify data fill factor. */
     i2              rellfill;              /* Modify leaf fill factor. */
     i4              relmin;                /* Modify min pages value. */
     i4              relmax;                /* Modify max pages value. */
     i4              relloccount;           /* Number of locations.--
					    ** WARNING: this iirelation attr.
					    ** shows as part of relfree in
					    ** catalogs for v6.0 and v6.1
					    */
     i2              relgwid;	            /* identifier of gateway that owns
					    ** the table.
					    ** WARNING: this iirelation attr.
					    ** shows as part of relfree in
					    ** catalogs for v6.0 and v6.1
					    */
     i2              relgwother;            /* identifier of gateway that owns
					    ** the table.
					    ** WARNING: this iirelation attr.
					    ** shows as part of relfree in
					    ** catalogs for v6.0 and v6.1
					    */
     u_i4            relhigh_logkey;        /* Most significant i4 of the 8
					    ** byte integer which is used to
					    ** generate logical keys.
					    ** WARNING: this iirelation attr.
					    ** shows as part of relfree in
					    ** catalogs for v6.0 and v6.1
					    */
     u_i4            rellow_logkey;         /* Least significant i4 of the 8
					    ** byte integer which is used to
					    ** generate logical keys.
					    ** WARNING: this iirelation attr.
					    ** shows as part of relfree in
					    ** catalogs for v6.0 and v6.1
					    */
     char            relfree[24];           /* Reserved for future use. --
					    ** WARNING: in versions v6.0
					    ** and v6.1 relfree was a 40 byte
					    ** area.  Members relloccount 
					    ** through rellow_logkey have
					    ** subsequently been claimed from
					    ** that relfree area, thus shrinking
					    ** available relfree area from 40
					    ** bytes to 24 bytes.  Without a
					    ** database conversion, the 
					    ** attribute relation does not
					    ** reflect this fact and thus only
					    ** shows the relfree columns and
					    ** does not show the added columns.
					    */
} DMP_OLDRELATION;

typedef struct _DMP_RELATION_26	/* Ingres 2.6 Relation tuple structure. */
    {
     DB_TAB_ID	     reltid;		    /* Table identifier. */
     char            relid[32];             /* Table name. */
     char            relowner[32];          /* Table owner. */
     i2              relatts;               /* Number of attributes in table. */
     i2              relwid;                /* Width in bytes of table record.*/
     i2		     relkeys;		    /* Number of attributes in key. */
     i2              relspec;               /* Storage mode of table. */
     i4              relstat;               /* Status bits of table. */
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
     char            relloc[32];            /* Table location */
     i4              relcmptlvl;	    /* DMF version of table. */
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
     i4              relloccount;           /* Number of locations. */
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
     i2		     relpgtype;		    /* Page format */
     u_i4	     relstat2;
     char	     relfree1[8];	/* Unused */
     i4		     relpgsize;		/* page size of this relation */
     u_i2	     relversion;	/* metadata version #; incr when column
					** layouts are altered
					*/
     i2              reltotwid;         /* Tot Width in bytes of table record.*/
     i2		     reltcpri;		/* Table Cache priority */
     char            relfree[14];       /* Reserved for future use. */
#define DMP_NUM_IIRELATION26_ATTS     45

     } DMP_RELATION_26;                   

/* Structure of an OI 1.x iiattribute entry. (DSC_V3).  This is the
** same as the later 2.x iiattribute except that it stops at the default
** ID.  Define it separately so that we know the size of such entries.
*/
typedef struct _DMP_ATTRIBUTE_1X
{
     DB_TAB_ID       attrelid;              /* Identifier associated with 
                                            ** this table. */
     i2              attid;                 /* Attribute ordinal number. */
     i2		     attxtra;		    /* underlaying GATEWAY data type */
     char	     attname[32];           /* Attribute name */
     i2              attoff;                /* Offset in bytes of attribute
                                            ** in record. */
     i2              attfmt;                /* Attribute type. */
     i2              attfml;                /* Length in bytes. */
     i2              attfmp;		    /* Precision for unsigned integer.*/
     i2              attkey;                /* Flag used to indicate this is
                                            ** part of a key. */
     i2              attflag;                  /* Used to indicate types. */
     DB_DEF_ID	     attDefaultID;	/* this is the index in iidefaults */
} DMP_ATTRIBUTE_1X;


/* Structure of a 2.6 iiattribute entry: */
typedef struct _DMP_ATTRIBUTE_26
{
     DB_TAB_ID       attrelid;              /* Identifier associated with 
                                            ** this table. */
     i2              attid;                 /* Attribute ordinal number. */
     i2		     attxtra;		    /* underlaying GATEWAY data type */
     char	     attname[32];           /* Attribute name */
     i2              attoff;                /* Offset in bytes of attribute
                                            ** in record. */
     i2              attfmt;                /* Attribute type. */
     i2              attfml;                /* Length in bytes. */
     i2              attfmp;		    /* Precision for unsigned integer.*/
     i2              attkey;                /* Flag used to indicate this is
                                            ** part of a key. */
     i2              attflag;                  /* Used to indicate types. */
     DB_DEF_ID	     attDefaultID;	/* this is the index in iidefaults
					** of the tuple containing the
					** user specified default for this
					** column.
					*/
     u_i2	     attintl_id;            /* internal row identifier */
     u_i2	     attver_added;          /* version row added       */
     u_i2	     attver_dropped;        /* version row dropped     */
     u_i2	     attval_from;           /* previous intl_id value  */
     char	     attfree[24];	    /*   F R E E   S P A C E   */
} DMP_ATTRIBUTE_26;

/* Structure of a 3.0 iiindex entry: */
typedef struct _DMP_INDEX_30
{
#define			DMP_INDEX_SIZE_30	74 
     i4       	     baseid;                /* Identifier associated with 
                                            ** the base table. */
     i4              indexid;               /* Identifer associated with    
                                            ** the index table. */
     i2		     sequence;		    /* Sequence number of index. */
     i2		     idom[DB_MAXKEYS];	    /* Attribute number of base
					    ** table which corresponds to
					    ** attribute of index. */
} DMP_INDEX_30;

/*{
** Name: dm2d_cname	- copy 24 character name and blank pad
**
** Description:
**      Routine will copy a 24 character name into a 32 char name
**	and blank pad 
**
** Inputs:
**      iname                           ptr to 24 char input name
**      oname_len			size of oname
**
** Outputs:
**      oname                           ptr to 'oname_len' char output name
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-sep-92 (ed)
**          initial creation for DB_MAXNAME
[@history_template@]...
*/
static VOID
dm2d_cname(
	    char               *oname,
	    char               *iname,
	    i4			oname_len)
{
    MEcopy((PTR)iname, DB_OLDMAXNAME, (PTR)oname);
    MEfill((oname_len - DB_OLDMAXNAME), ' ', (PTR)&oname[DB_OLDMAXNAME]);
}


/*{
** Name: add_record - Add a record to a page
**
** Description:
**      Routine will add a tuple to a page given the page and record.
**	If the page does not have enough free space to store the record,
**	the routine will write the current page to disk, format a new
**	page and write the record to the new page.
**
**	This routine is used to convert iirelation and iiattribute to
**	the current format.
**
** Inputs:
**	page_type	Page type 
**	page_size	Page size
**	page		Page to insert record into.
**	record		Record to be inserted.
**	record_size	size of record
**	di_context	context for open file
**	local_plv	add routine for page
** Outputs:
**	err_code	error code
**
**	Returns:
**	    STATUS	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      7-oct-92 (ananth)
**          initial creation for DB_MAXNAME
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument, pass page_size to dmpp_format.
**	06-may-1996 (thaju02)
**	    New Page Format Support:
**		Change page header references to use macros.
**	20-may-1996 (ramra01)
**	    Added new param to the load accessor
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          Pass 0 as the current table version when converting catalogs.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          load accessor: changed DMPP_TUPLE_INFO param to table_version
**	25-Aug-2004 (schka24)
**	    Might be good to use page size/type in load-put call...
**	    Also store checksum into page when writing.
*[@history_template@]...
*/
static STATUS
add_record(
i4		page_type,
i4		page_size,
DMPP_PAGE	*page,
char		*record,
i4		record_size,
DM_TID		*tid,
DI_IO		*di_context,
DMPP_ACC_PLV    *local_plv,
DB_ERROR	*dberr)
{
    i4	    page_number=0;
    i4		    write_count;
    CL_ERR_DESC	    sys_err;
    STATUS	    status=OK;
    bool	    add_status;
 

    /* 
    ** Store the record on the page. If it doesn't fit allocate an overflow
    ** page.
    */

    while ((add_status = (*local_plv->dmpp_load)(
		page_type, page_size, page,
		record, 
		record_size,
		DM1C_LOAD_NORMAL,
		0,
		tid,
		(u_i2)0, (DMPP_SEG_HDR *)0)) != E_DB_OK)
    {
	/* 
	** The record didn't fit on the page. Allocate and write the page out
	** and format the next overflow page.
	*/

	status = DIalloc(
	    di_context, 1, &page_number, &sys_err);

	if (status != OK)
	    break;

	status = DIflush(di_context, &sys_err);

	if (status != OK)
	    break;

	/* Set up the overflow page for this page. */
	/* Also clear MODIFY, set checksum */

	DMPP_VPT_SET_PAGE_OVFL_MACRO(page_type, page, page_number + 1);
	DMPP_VPT_CLR_PAGE_STAT_MACRO(page_type, page, DMPP_MODIFY);
	dm0p_insert_checksum(page, page_type, page_size);

	/*  Write current page to disk. */

	write_count = 1;

	TRdisplay("Write page %d nxtlino %d \n", 
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, page), 
	    (i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(page_type, page));

	status = DIwrite(
	    di_context, 
	    &write_count,
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, page), 
	    (char *)page, 
	    &sys_err);

	if (status != OK)
	    break;

	(*local_plv->dmpp_format)(page_type, page_size, page, ++page_number, 
				  DMPP_DATA, DM1C_ZERO_FILL);
    } /* end while */

    return (status);
}


/*{
** Name: dm2d_add_db	- Add database to server.
**
** Description:
**      This function makes a simple check for the existence of a database
**	and then enters the database name and root location into a DCB
**	that is linked off the SVCB.  A database is assumed to exist if
**	a configuration file can be located.
**
**	A note on the core catalog conversion that is done here.
**	We can't get here (normally) unless either the database is
**	up-to-date (per dbcmptlvl), or unless we're upgradedb.
**	So there's no need to check the core catalog version dsc_c_version
**	unless we're upgradedb.  (Or unless we're in the middle of a
**	development cycle and dbcmptlvl has already been bumped!!!)
**
** Inputs:
**	flag				    DM2D_JOURNAL - db is journaled
**					    D2MD_NOJOURNAL - db not journaled
**					    DM2D_SINGLE - run single-server
**					    DM2D_MULTIPLE - run multi-server
**					    DM2D_FASTCOMMIT - use fast commit
**                                          DM2D_DMCM - use DMCM protocol.
**					    DM2D_BMSINGLE - database can only
**						be opened by one buffer manager
**					    DM2D_CANEXIST - DCB for this db may
						already exist.
**	access_mode			    The requested database access mode: 
**                                          DM2D_A_READ,
**					    DM2D_A_WRITE.
**	name				    Pointer to database name.
**	owner				    Pointer to database owner.
**	loc_count			    Count of locations.
**	location			    Pointer to array of DM2D_LOC_ENTRY.
**
** Outputs:
**	access_mode			    The (updated) database access mode: 
**                                          DM2D_A_READ,
**					    DM2D_A_WRITE.
**	db_id				    The database_id assigned to this db.
**	err_code			    The reason for an error status.
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
**	25-nov-1985 (derek)
**          Created new for jupiter.
**	 9-dec-1988 (rogerk)
**	    Return a DCB INVALID error if we find an already existent
**	    dcb with this name with status DCB_S_INVALID.
**	28-feb-1989 (rogerk)
**	    Added support for shared buffer manager.  Added DM2D_BMSINGLE
**	    flag to specify that database is only served by one buffer manager.
**	15-may-1989 (rogerk)
**	    Return LOCK_QUOTA_EXCEEDED if run out of lock lists.
**	    Took out double formatting of errors if dm0c_open fails.
**	    Preserve external DMF errors when dm0c_open fails.
**	20-aug-1989 (ralph)
**	    Set DM0C_CVCFG for config file conversion
**	12-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Convert IIrelation if DM0C_CVCFG flag set.
**	30-October-1992 (rmuth)
**	    Release the mutex on the svcb if we get an error converting
**	    the iirelation table to new format.
**	30-nov-1992 (rmuth)
**	    In dm2d_add_db, release the svcb_mutex if fail converting 
**	    the iiattribute table.
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project: Changed DM2D_RCP flag to DM2D_RECOVER
**	    since it is passed from more than just the rcp process.
**	26-apr-1993 (keving)
**	    VAX Cluster Project I. The CSP attempts to add a db for each
**	    node upon which it needs recovery. It passes a DM2D_CANEXIST
**	    flag, so that if it already exists we can just return the 
**	    existing dcb.
**	    We keep a count of the number of times a db is added so that
**	    we only delete it when the last user deletes it.
**	23-aug-1993 (rogerk)
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          Mark database as supporting DMCM protocol.
**	15-Oct-1996 (jenjo02)
**	    Compose dcb_mutex name using dcb_name to uniquify for 
**	    iimonitor, sampler.
** 	08-apr-1997 (wonst02)
** 	    Pass back the access mode from the .cnf file.
**	04-jun-1997 (wonst02)
**	    More changes for readonly database.
**	21-aug-1997 (nanpr01)
**	    We need to return the versions in the control block for upgrade
**	    of FIPS database.
**      17-Aug-1998 (wanya01)
**          Add 4 bytes to array sem_name[]. This fix bug 92508
**      15-Mar-1999 (hanal04)
**          Use DM0C_TIMEOUT to flag the config file lock request to
**          have a timeout value to prevent dblist mutex / config file
**          lock deadlocks. b55788.
**      27-May-1999 (hanal04)
**          Removed and reworked the above change for bug 55788. Check
**          to see whether called has already locked the CNF file. If this
**          is the case do not create or destroy the lock list and use
**          the supplied lock list id. Also pass this information to
**          dm0c_open(). b97083.
**      21-Jul-1999 (wanya01)
**          Pass cnf_flag and temp_lock_list to dm2d_convert_iirel_from_64()
**          and dm2d_convert_iiatt_from_64(). (Bug 98010) 
**	05-Mar-2002 (jenjo02)
**	    Init (new) dmd_seq list to null.
**	09-sep-2002 (devjo01)
**	    All databases opened in a clustered environment MUST use
**	    DMCM protocols.
**	25-Aug-2004 (schka24)
**	    Unify some of the core catalog conversion stuff.
**	07-jan-2004 (devjo01)
**	    Do not force DMCM if clustered, just depend on DM2D_DMCM, as
**	    a DMCM may not be needed if clustered but node affined.
**	12-Nov-2009 (kschendel) SIR 122882
**	    cmptvl is an integer now.
*/
DB_STATUS
dm2d_add_db(
    i4             flag,
    i4		*access_mode,
    DB_DB_NAME	    	*name,
    DB_OWN_NAME	    	*owner,
    i4		loc_count,
    DM2D_LOC_ENTRY	*location,
    DMP_DCB		**db_id,
    i4		*lock_list,
    DB_ERROR		*dberr )
{
    CL_ERR_DESC      sys_err;
    DM_SVCB	    *svcb = dmf_svcb;
    DMP_DCB	    *next_dcb;
    DMP_DCB	    *end_dcb;
    DMP_DCB	    *dcb = 0;
    DM0C_CNF	    *cnf = 0;
    i4         cnf_flag = 0;
    i4	    i;
    i4	    root = 0;
    DB_STATUS	    status;
    i4         temp_lock_list = 0;
    i4         local_error;
    i4	    create_version;
    char	    sem_name[CS_SEM_NAME_LEN+4];
    char	    null_name[DB_DB_MAXNAME];
    bool	dbservice_warning = FALSE;
    i4			*err_code = &dberr->err_code;
    DM2D_UPGRADE	upgrade_cb;
    char	*dbg_upgr = NULL;

    CLRDBERR(dberr);

    svcb->svcb_stat.db_add++;

    for (;;)
    {
	 /*	Allocate a DCB for this database. */

	status = dm0m_allocate((i4)sizeof(DMP_DCB),
	    DM0M_ZERO, (i4)DCB_CB, (i4)DCB_ASCII_ID, (PTR)svcb,
	    (DM_OBJECT **)&dcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*  Copy root location name into the DCB. */

	STRUCT_ASSIGN_MACRO(*name, dcb->dcb_name);
	STRUCT_ASSIGN_MACRO(*owner, dcb->dcb_db_owner);
	dcb->dcb_location.logical = location->loc_name;
	dcb->dcb_location.phys_length = location->loc_l_extent;
	MEmove(location->loc_l_extent, location->loc_extent, ' ',
	    sizeof(dcb->dcb_location.physical),
	    (PTR)&dcb->dcb_location.physical);
	dcb->dcb_location.flags = 0;

	/*  Set the requested access mode */
	dcb->dcb_access_mode = DCB_A_READ;
	if (*access_mode == DM2D_A_WRITE)
	    dcb->dcb_access_mode = DCB_A_WRITE;

	/*
	** Check the config file for conversion requirements.
	** This is not done if the database is being opened for recovery
	** processing.
	*/
	if ((flag & DM2D_RECOVER) == 0)
	{
            if((flag & DM2D_CNF_LOCKED) == 0)
            {
	        /*  Create a temporary lock list. */

	        status = LKcreate_list(LK_ASSIGN | LK_NONPROTECT | 
			  	LK_NOINTERRUPT, (i4)0, (LK_UNIQUE *)NULL, 
				(LK_LLID *)&temp_lock_list, 0, 
			        &sys_err);
	        if (status != OK)
	        {
		    uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		        (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		    uleFormat( NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG , 
			(DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
                        &local_error, 0);
		    if (status == LK_NOLOCKS)
			SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		    else
			SETDBERR(dberr, 0, E_DM9265_ADD_DB);
		    break;
	        }
            }
            else
            {
                cnf_flag |= DM0C_CNF_LOCKED;
                temp_lock_list = *(LK_LLID *)lock_list;
            }

	    /*  As an existence check, try opening the configuration file. */

	    /* Allow config file conversion if DM2D_CVCFG was set */
	    if (flag & DM2D_CVCFG)
		cnf_flag |= DM0C_CVCFG;
	    if (*access_mode == DM2D_A_READ)
	    	cnf_flag |= DM0C_READONLY;

	    status = dm0c_open(dcb, cnf_flag, temp_lock_list, &cnf, dberr);

	    if (status == E_DB_OK)
	    {
		dcb->dcb_id = cnf->cnf_dsc->dsc_dbid;

		/* Find root location and copy into the DCB. */

		for (i = 0; i < cnf->cnf_dsc->dsc_ext_count; i++)
		    if (cnf->cnf_ext[i].ext_location.flags & DCB_ROOT)
		    {
			dcb->dcb_location = cnf->cnf_ext[i].ext_location;
			root++;
			break;
		    }

		/*
		** Keep track of the database creation version. We'll
		** use this to decide whether iirelation needs convertion.
		*/

		create_version = cnf->cnf_dsc->dsc_c_version;
		dcb->dcb_dbservice = cnf->cnf_dsc->dsc_dbservice;
		dcb->dcb_dbcmptlvl = cnf->cnf_dsc->dsc_dbcmptlvl;
		dcb->dcb_1dbcmptminor = cnf->cnf_dsc->dsc_1dbcmptminor;


		/*
		** If this is not upgradedb, check if dbservice flags look
		** like they are set correctly.
		** If not... this could mean we are going to get errors
		** reading tree catalogs
		** A message here will help decipher the RDF errors that 
		** will follow if you try to access a tree.
		*/
		if ( (flag & DM2D_CVCFG) == 0)
		{
#ifdef LP64
		    if ( (cnf->cnf_dsc->dsc_dbservice & DU_LP64) == 0)
			dbservice_warning = TRUE;
#else
		    if (cnf->cnf_dsc->dsc_dbservice & DU_LP64)
			dbservice_warning = TRUE;
#endif
		}

#if defined(axp_osf) || defined(ris_u64) || defined(i64_win) || \
	defined(i64_hp2) || defined(i64_lnx) || defined(axp_lnx) || \
	defined(a64_win)
		/* Don't issue warning on platforms that were ALWAYS lp64 */
		dbservice_warning = FALSE;
#endif
		if (dbservice_warning)
		{
		    uleFormat(NULL, E_DM9580_CONFIG_DBSERVICE_ERROR, 
			(CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 2, 0, cnf->cnf_dsc->dsc_dbservice,
			sizeof(DB_DB_NAME), &cnf->cnf_dsc->dsc_name);
		}

		status = dm0c_close(cnf, (cnf_flag & DM0C_CNF_LOCKED), dberr);
	    }
	    if ((status != E_DB_OK) && (dberr->err_code > E_DM_INTERNAL))
	    {
	        uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, NULL, 0, NULL,
			   err_code, 0);
		SETDBERR(dberr, 0, E_DM9265_ADD_DB);
	    }

            if((flag & DM2D_CNF_LOCKED) == 0)
            {
	        /*  Release the temporary lock list. */

	        status = LKrelease(LK_ALL, temp_lock_list, 
		        (LK_LKID *)NULL, (LK_LOCK_KEY *)NULL,
		        (LK_VALUE *)NULL, &sys_err);
	        if (status != OK)
	        {
		    uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		        (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		    uleFormat( NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG,
			(DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
                        &local_error, 1,
			sizeof(temp_lock_list), temp_lock_list);
		    SETDBERR(dberr, 0, E_DM9265_ADD_DB);
		    break;
	        }
            }

	    /*  Check that we found a root location in the 
            **  configuration file. */

	    if (root == 0)
	    {
		if ((dberr->err_code > E_DM_INTERNAL) || (dberr->err_code == 0))
		{
		    if (dberr->err_code)
			uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
				NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM0053_NONEXISTENT_DB);
		}
		break;
	    }
	}

	/*  Finish initializing the DCB. */

	dcb->dcb_status = 0;
	dcb->dcb_add_count = 1;
	dcb->dcb_db_type = DCB_PUBLIC;
	if (flag & DM2D_JOURNAL)
	    dcb->dcb_status |= DCB_S_JOURNAL;
	if (flag & DM2D_FASTCOMMIT)
	    dcb->dcb_status |= DCB_S_FASTCOMMIT;
	if (flag & DM2D_SINGLE)
	    dcb->dcb_served = DCB_SINGLE;
	else
	    dcb->dcb_served = DCB_MULTIPLE;
	if (flag & DM2D_BMSINGLE)
	    dcb->dcb_bm_served = DCB_SINGLE;
	else
	    dcb->dcb_bm_served = DCB_MULTIPLE;

        /*
        ** (ICL phil.p)
        */
        if (flag & DM2D_DMCM)
            dcb->dcb_status |= DCB_S_DMCM;

	/* Init the new field */
	dcb->dcb_ebackup_lsn.lsn_low = 0;
	dcb->dcb_ebackup_lsn.lsn_high = 0;

	/* Initialize Sequence Generator list */
	dcb->dcb_seq = (DML_SEQ*)NULL;

	/*  Lock the SVCB. */
	dm0s_mlock(&svcb->svcb_dq_mutex);

	/* Check max open database count for server */
	if (svcb->svcb_cnt.cur_database == svcb->svcb_d_max_database)
	{
	    dm0s_munlock(&svcb->svcb_dq_mutex);
	    SETDBERR(dberr, 0, E_DM0041_DB_QUOTA_EXCEEDED);
	    break;
	}

	/* Check for a duplicate database name before inserting on SVCB. */

	for (
	    next_dcb = svcb->svcb_d_next, 
                 end_dcb = (DMP_DCB*)&svcb->svcb_d_next;
	    next_dcb != end_dcb;
	    next_dcb = next_dcb->dcb_q_next
	    )
	{
	    if (MEcmp((char *)&next_dcb->dcb_name, (char *)name, 
                               sizeof(next_dcb->dcb_name)) == 0 &&
		MEcmp((char *)&next_dcb->dcb_db_owner, (char *)owner, 
                                   sizeof(next_dcb->dcb_db_owner)) == 0)
	    {
		break;
	    }
	}
	if (next_dcb != end_dcb)
	{
	    if (next_dcb->dcb_status & DCB_S_INVALID)
	    {
		/*
		** Log message saying that the server must be shut down in 
		** order to reaccess the database, then return an error
		** specifying that the database is not available.
		*/
		uleFormat(NULL, E_DM927F_DCB_INVALID, (CL_ERR_DESC *)NULL, ULE_LOG, 
                    (DB_SQLSTATE *)NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		    sizeof(next_dcb->dcb_name.db_db_name),
		    next_dcb->dcb_name.db_db_name);
		SETDBERR(dberr, 0, E_DM0133_ERROR_ACCESSING_DB);
	    }
	    else
	    {
		if (flag & DM2D_CANEXIST)
		{
			/* Unlock svcb and return existing dcb */
			next_dcb->dcb_add_count++;
		    	dm0s_munlock(&svcb->svcb_dq_mutex);
		    	*db_id = next_dcb;
		    	dm0m_deallocate((DM_OBJECT **)&dcb);
		    	return (E_DB_OK);
    		}
		else
		    SETDBERR(dberr, 0, E_DM0011_BAD_DB_NAME);
	    }
	    dm0s_munlock(&svcb->svcb_dq_mutex);
	    break;
	}

	/*  Initialize the DCB mutex. */
	MEcopy(dcb->dcb_name.db_db_name, DB_DB_MAXNAME, null_name);
	null_name[DB_DB_MAXNAME - 1] = 0;
	STtrmwhite(null_name);
	dm0s_minit(&dcb->dcb_mutex,
		  STprintf(sem_name, "DCB %s", null_name));

	/*  Queue DCB to SVCB. */
	dcb->dcb_q_next = svcb->svcb_d_next;
	dcb->dcb_q_prev = (DMP_DCB*)&svcb->svcb_d_next;
	svcb->svcb_d_next->dcb_q_prev = dcb;
	svcb->svcb_d_next = dcb;
        svcb->svcb_cnt.cur_database++;


	/* 
	** If the DM2D_CVCFG flag is set, we know this is upgradedb.
	*/

	if (flag & DM2D_CVCFG)
	{
	    /*
	    ** Two types of upgrade, in-place or rewrite
	    ** basically if a core catalog row gets wider its a rewrite
	    **
	    ** Adding new upgrade stuff... should mostly require 
	    ** only new code to upgrade_iirel_row, upgrade_iiatt_row
	    **
	    ** UPGRADE_REWRITE isn't always necessary, 
	    ** but it should always work!
	    */
	    MEfill(sizeof(upgrade_cb), 0, &upgrade_cb);
	    upgrade_cb.upgr_create_version = create_version;

	    /* Determine type of upgrade we need for iirelation */
	    if (create_version < DSC_V3)
		upgrade_cb.upgr_iirel = UPGRADE_REWRITE;
	    else if (create_version < DSC_V7)
		upgrade_cb.upgr_iirel = UPGRADE_INPLACE;
	    else
		upgrade_cb.upgr_iirel = 0;

	    /* Determine type of upgrade we need for iiattribute */
	    if (create_version < DSC_V7)
		upgrade_cb.upgr_iiatt = UPGRADE_REWRITE; 
	    else
	    {
		/*
		** note: currently INPLACE iiattribute upgradedb done by
		** front end upgradeb program
		*/
		upgrade_cb.upgr_iiatt = 0;
	    }

	    /*
	    ** To test the core catalog upgrade
	    ** 1) Set II_UPGRADEDB_DEBUG before starting the dbms server
	    ** 2) Run the following
	    **     sql dbname -u\$ingres +U -A6 < test_upgrade.sql
	    ** Below are the contents of test_upgrade.sql
	    **     set session authorization $ingres';
	    **     modify iirelation to hash;
	    **     modify iiattribute to hash;
	    **     modify iiindexes to hash;
	    ** Lots of trace output goes to $II_DBMS_LOG
	    */

	    NMgtAt(ERx("II_UPGRADEDB_DEBUG"), &dbg_upgr);
	    if (dbg_upgr != NULL && *dbg_upgr)
	    {
		TRdisplay("II_UPGRADEDB_DEBUG %s\n", dbg_upgr);
		if (!STbcompare(dbg_upgr, 0, ERx("rewrite"), 0, TRUE))
		{
		    /* for testing upgradedb */
		    if (!upgrade_cb.upgr_iirel && dcb->dcb_id != 1)
			upgrade_cb.upgr_iirel = UPGRADE_REWRITE;
		    if (!upgrade_cb.upgr_iiatt && dcb->dcb_id != 1)
			upgrade_cb.upgr_iiatt = UPGRADE_REWRITE;
		    upgrade_cb.upgr_dbg_env = UPGRADE_REWRITE;
		}
		else if (!STbcompare(dbg_upgr, 0, ERx("inplace"), 0, TRUE))
		{
		    /* for testing upgradedb */
		    if (!upgrade_cb.upgr_iirel && dcb->dcb_id != 1)
			upgrade_cb.upgr_iirel = UPGRADE_INPLACE;
		    if (!upgrade_cb.upgr_iiatt && dcb->dcb_id != 1)
			upgrade_cb.upgr_iiatt = UPGRADE_INPLACE;
		    upgrade_cb.upgr_dbg_env = UPGRADE_INPLACE; 
		}
		else if (!STbcompare(dbg_upgr, 0, ERx("debug"), 0, TRUE))
		{
		    upgrade_cb.upgr_dbg_env = UPGRADE_DEBUG;
		}
	    }

	    /* If database is pre-V7 (ie older than r3), we need to
	    ** convert both iirelation and iiattribute.
	    ** The iirelation conversion rewrites the table file if
	    ** the db is 6.4 or older;  it is converted in-place if
	    ** the db is newer than 6.4.
	    ** iiattribute rewrites a new table file in both cases,
	    ** as the contents change significantly for V7+.
	    ** Note that if we have to do conversion here, we actually
	    ** convert to "current" (V8 at present).
	    ** If the database is already V7, we do not need to do anything
	    ** here.  Upgradedb will take care of getting it to V8.
	    ** (V8 overlays some of attfree with attcollid, and upgradedb
	    ** can do that.)
	    */
	    if (upgrade_cb.upgr_iirel || upgrade_cb.upgr_iiatt)
		TRdisplay("upgrade %~t create version %d current version %d\n", 
		sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name,
		create_version, DSC_VCURRENT);

	    if (upgrade_cb.upgr_iirel == UPGRADE_REWRITE)
	    {
		status = dm2d_upgrade_rewrite_iirel(&upgrade_cb, dcb,
			cnf_flag, temp_lock_list, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
		    		NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9265_ADD_DB);
		    dm0s_munlock(&svcb->svcb_dq_mutex);
		    break;
		}

	    }
	    else if (upgrade_cb.upgr_iirel == UPGRADE_INPLACE)
	    {
		/* Older than V7 (r3), but newer than 6.4 */
		status = dm2d_convert_inplace_iirel(&upgrade_cb, dcb,
			cnf_flag, temp_lock_list, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
		    		NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9265_ADD_DB);
		    dm0s_munlock(&svcb->svcb_dq_mutex);
		    break;
		}
	    }

#ifdef NOT_UNTIL_CLUSTERED_INDEXES
/* 
** FIX ME remove dm2d_convert_preV9_iirel and add iirelation changes
** to upgrade_iirel_row instead
*/
	    if ( create_version < DSC_V9 )
	    {
		/* Older than V9 (Ingres2007), but newer than 6.4 */
		status = dm2d_convert_preV9_iirel(dcb, create_version,
			cnf_flag, temp_lock_list, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
		    		NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9265_ADD_DB);
		    dm0s_munlock(&svcb->svcb_dq_mutex);
		    break;
		}
		/* Convert IIINDEX to V9 format */
		status = dm2d_convert_preV9_iiind(dcb, create_version,
			cnf_flag, temp_lock_list, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
		    		NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9265_ADD_DB);
		    dm0s_munlock(&svcb->svcb_dq_mutex);
		    break;
		}
	    }
#endif /* NOT_UNTIL_CLUSTERED_INDEXES */

	    /* iiattribute conversion is the same for all pre-V7 db's */
	    if (upgrade_cb.upgr_iiatt == UPGRADE_REWRITE)
	    {
		status = dm2d_upgrade_rewrite_iiatt(&upgrade_cb, dcb,
			cnf_flag, temp_lock_list, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
		    		NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9265_ADD_DB);
		    dm0s_munlock(&svcb->svcb_dq_mutex);
		    break;
		}
	    }

#ifdef NOT_UNTIL_CLUSTERED_INDEXES
/* 
** FIX ME remove dm2d_convert_preV9_iiatt and add iiattribute changes
** to upgrade_iiatt_row instead
*/
	    /* Do iiattribute stuff for Ingres2007 */
	    if (create_version < DSC_V9)
	    {
		status = dm2d_convert_preV9_iiatt(dcb, create_version,
			cnf_flag, temp_lock_list, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
		    		NULL, 0, NULL, err_code, 0);
		    SETDBERR(dberr, 0, E_DM9265_ADD_DB);
		    dm0s_munlock(&svcb->svcb_dq_mutex);
		    break;
		}
	    }
#endif /* NOT_UNTIL_CLUSTERED_INDEXES */
	} /* if upgradedb */

	/*  Unlock the SVCB. */

	dm0s_munlock(&svcb->svcb_dq_mutex);

	*db_id = dcb;
	return (E_DB_OK);
    }

    if (dcb)
	dm0m_deallocate((DM_OBJECT **)&dcb);

    return (E_DB_ERROR);
}

/*{
** Name: dm2d_close_db	- Close a database for a session.
**
** Description:
**      This function decrements the reference in the DCB for
**	the database.  If the reference count is zero, then the  
**	the database is actually closed.
**
** Inputs:
**      db_id                           The database identifier.
**	lock_list			The lock list for the database lock.
**	flag				Special close flags:
**					    DM2D_NLK : Database is not locked.
**					    DM2D_NLG : Database is not logged.
**					    DM2D_JLK : Database is only locked.
**					    DM2D_NLK_SESS : not to release 
**						session lock on the database.
**					    DM2D_IGNORE_CACHE_PROTOCOL : Bypass
**						cache database lock which
**					        enforces sole_cache protocols.
** Outputs:
**      err_code                        The reason for an error return status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR;
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	06-dec-1985 (derek)
**          Created new for jupiter.
**	 9-dec-1988 (rogerk)
**	    If get an error closing the database, mark the dcb as invalid.
**	28-feb-1989 (rogerk)
**	    Added call to dm0p_dbcache to register close of db.
**	3-mar-89 (EdHsu)
**	    online backup cluster support.
**	27-mar-1989 (rogerk)
**	    Send TOSS flag to DM0P when close database in RCP.
**	    Pass lock value when release server database lock so it won't be
**	    marked invalid.
**	27-mar-1989 (EdHsu)
**	    Move cluster online backup support at bt level
**	15-may-1989 (rogerk)
**	    Add err_code parameter to dm0p_dbcache call.
**	19-jun-1989 (rogerk)
**	    Toss pages from buffer manager when db closed that was open
**	    exclusively to make sure we don't cache pages after a db is
**	    destroyed or restored from checkpoint.
**	 9-Jul-1989 (anton)
**	    Deallocate collation descriptors when a database is closed
**	 4-aug-1989 (rogerk)
**	    Put in workaround for compile bug when optimized.  Local variable
**	    was put into register for use in "for" loop, but register was
**	    being used for another value inside the loop.  Reset the variable
**	    value on each loop before its use.
**	11-sep-1989 (rogerk)
**	    Save lx_id returned from dm0l_opendb call and use it in
**	    dm0l_closedb call.
**	28-jan-91 (rogerk)
**	    Added DM2D_IGNORE_CACHE_PROTOCOL flag in order to allow auditdb
**	    to run on a database while it is open by a fast commit server.
**	    When this flag is enabled, we bypass the server database lock.
**	    (Unlike the NLK flag which will bypass the database lock as well)
**	28-jan-91 (rogerk)
**	    Moved the release of the Database lock until after all other
**	    locks are released so that other users which are waiting on the
**	    database lock will not immediately error when they cannot get
**	    the SV_DATABASE lock in a compatible mode.
**	 4-feb-91 (rogerk)
**	    Added new argument to dm0p_toss_pages call.
**	25-feb-1991 (rogerk)
**	    Added DM2D_NOLOGGING flag to correspond to flag for open_db.
**	    This was added as part of the Archiver Stability Project.
**	24-apr-1991 (stevet)
**          Bug#37024.  Deallocate collation descriptor ONLY when reference
**          count is zero.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings ("warning: statement not reached")
**          by changing the way "for (;xxx == E_DB_OK;)" loops were coded.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Adjusted dm2t_release_tcb parameters
**	    according to new dm2t routine protocols:  
**		Set the tcb ref counts of the core catalogs to 0 before calling
**		    dm2t to throw them away.
**		Since dm2t_release_tcb was changed to not release the hcb mutex
**		    upon its return, we no longer have to reacquire the mutex 
**		    after each call.
**		Don't have to pass the DM2T_UPDATE flag any longer as tuple/page
**		    counts will be automatically updated if tcb_tup/page_adds
**		    is non-zero.
**		Since WB threads can now fix tcb's we have to check if the
**		    TCB's we want to toss are free and wait for them if not.
**	    Fix problem with dm0p_dbcache overwriting return status from
**		previous close operations.
**	18-jan-1993 (rogerk)
**	    Removed check for DCB_S_RCP from closedb processing.  We used to
**	    toss all pages here when we were the RCP or CSP.  We now do this
**	    explicitely from recovery code.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Remove DM2D_CSP and DCB_S_CSP status flags, since it's no longer
**		    required that a DCB be flagged with whether it was built by
**		    a CSP process or not.
**	26-jul-1993 (bryanp)
**	    We no longer log a closedb log record, which means that we no
**		longer need to pass a lx_id argument to dm0l_opendb and to
**		dm0l_closedb.
**	28-may-96 (stephenb)
**	    Add last-ditch check for outstanding replications for this DB in
**	    the replicator transaction shared memory queue, we will complete
**	    them here before deleting the db from the server.
**	24-jul-96 (stephenb)
**	    Make sure Dmc_rep is set before doing above check
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          If the database being closed was opened with the Distributed
**          Multi-Cache Management (DMCM) protocol, pass TOSS flag into
**          dm2t_release_tcb to cause even clean pages to be tossed from
**          the cache.
**	24-Oct-1996 (jenjo02)
**	    Replaced hcb_mutex with hash_mutex, one for each hash entry.
**	    Added hcb_tq_mutex to protect TCB free queue.
**      27-feb-97 (stial01)
**          If SINGLE-SERVER mode, get SV_DATABASE locks on SINGLE-SERVER
**          lock list (svcb_ss_lock_list).
**	30-may-1997 (wonst02)
** 	    Do not log a readonly database close.
**	23-oct-98 (stephenb)
**	    Make sure we don't increment the replication queue seg size when
**	    we move the tail forward.
**	09-Mar-1999 (schte01)
**       Changed assignment of j within for loop so there is no dependency on 
**       order of evaluation.
**	24-mar-2001 (stephenb)
**	 Add support for Unicode collation.
**	05-Mar-2002 (jenjo02)
**	    Deallocate everything on dcb_seq list.
**	16-Jan-2004 (schka24)
**	    Rearranged hash mutexing, hash table now fixed size.
**	18-Jan-2005 (jenjo02)
**	    Ignore returned status from dm0p_dbcache(), which was
**	    being done anyway.
**	21-jul-2006 (kibro01) bug 114168
**	    Tell dm2rep_qman that this is a hijacked call, and shouldn't
**	    keep trying if deadlocks occur.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Track count of DB's with raw locns.
*/
DB_STATUS
dm2d_close_db(
    DMP_DCB	*db_id,
    i4	lock_list,
    i4     flag,
    DB_ERROR	*dberr )
{
    DM_SVCB             *svcb = dmf_svcb;
    DMP_DCB		*dcb = (DMP_DCB *)db_id;
    DM_MUTEX		*hash_mutex;
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    i4		release_error;
    CL_ERR_DESC		sys_err;
    LK_LOCK_KEY		db_key;
    LK_LOCK_KEY		db_ckey;
    LK_VALUE		lock_value;
    DMP_TCB             *t;
    PTR			null_ptr = NULL;
    i4		i;
    bool		tcb_close_error = FALSE;
    bool		invalid_tcbs_found = FALSE;
    i4             release_flag;
    i4             svcb_lock_list;
    LK_EVENT		lk_event;
    i4			*err_code = &dberr->err_code;
    i4			local_err;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (dcb->dcb_type != DCB_CB)
	dmd_check(E_DMF00D_NOT_DCB);
#endif

    svcb->svcb_stat.db_close++;

    /*	Initialize the database lock key. */

    MEcopy((PTR)&dcb->dcb_name, LK_KEY_LENGTH, (PTR)&db_key.lk_key1);

    /*	Lock the DCB. */

    dm0s_mlock(&dcb->dcb_mutex);
    
    /* If reference count goes to zero, then close the database. */

    while (--dcb->dcb_ref_count == 0)
    {
	/*  Check for database is only locked. */
	if (flag & DM2D_JLK)
	    break;

	/* Count down raw DB's */
	if (dcb->dcb_status & DCB_S_HAS_RAW)
	    CSadjust_counter(&svcb->svcb_rawdb_count, -1);

	/*
	** check for outstanding replications
	*/
	if ((dcb->dcb_status & DCB_S_REPLICATE) && Dmc_rep)
	{
	    bool	have_mutex = FALSE;
	    bool	last;
	    i4		i, j;
	    DMC_REPQ	*repq = (DMC_REPQ *)(Dmc_rep + 1);

	    /* check transaction for records for this db */
	    CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
	    have_mutex = TRUE;
	    for (i = Dmc_rep->queue_start, last = FALSE;;i++)
	    {
		if (last)
		    break;
		if (i >= Dmc_rep->seg_size)
		    i = 0;
		if (i == Dmc_rep->queue_end)
		    last = TRUE;
		if (MEcmp(dcb->dcb_name.db_db_name, 
				repq[i].dbname.db_db_name, DB_DB_MAXNAME))
		    continue;
		if (repq[i].active)
		    /*
		    ** since we are the last user of this db in this server
		    ** (including queue management threads), then this must
		    ** mean that there is another server able to process 
		    ** entries for this db, so let them do it instead
		    */
		    break;
		if (repq[i].tx_id == 0) /* already done */
		    continue;
		/*
		** something to process
		*/
		repq[i].active = TRUE;
		CSv_semaphore(&Dmc_rep->rep_sem);
		have_mutex = FALSE;
            	uleFormat(NULL, W_DM9563_REP_TXQ_WORK, (CL_ERR_DESC *)NULL, ULE_LOG,
                    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                    err_code, (i4)1, DB_DB_MAXNAME, dcb->dcb_name.db_db_name);
		status = dm2rep_qman(dcb, repq[i].tx_id, &repq[i].trans_time, 
		    NULL, dberr, TRUE);
                if (status != E_DB_OK && status != E_DB_WARN)
                {
                    /* TX will have been backed out, reset active and leave
                    ** for another thread to deal with */
                    CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
                    repq[i].active = FALSE;
                    CSv_semaphore(&Dmc_rep->rep_sem);
                    break;
                }
		/* clear the entry from the queue */
		CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
		have_mutex = TRUE;
		repq[i].active = FALSE;
		repq[i].tx_id = 0;
		if (i == Dmc_rep->queue_start)
		{
		    for (j = i; 
			 repq[j].active == 0 && repq[j].tx_id == 0 &&
			     j != Dmc_rep->queue_end; 
			 j = j % (Dmc_rep->seg_size + 1),j++);
		    Dmc_rep->queue_start = j;
		    /*
		    ** signal waiting threads
		    */
		    lk_event.type_high = REP_READQ;
		    lk_event.type_low = 0;
		    lk_event.value = REP_READQ_VAL;

		    cl_status = LKevent(LK_E_CLR | LK_E_CROSS_PROCESS, 
			lock_list, &lk_event, &sys_err);
                    if (cl_status != OK)
                    {
                        uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE*)NULL,
                            (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
                        uleFormat(NULL, E_DM904B_BAD_LOCK_EVENT, &sys_err, ULE_LOG,
                            (DB_SQLSTATE*)NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code,
                            3, 0, LK_E_CLR, 0, REP_READQ, 0, lock_list);
                        status = E_DB_ERROR;
                        break;
                    }
		}
		if (status != E_DB_OK)
		    break;
	    } /* transaction queue processing */
	    if (have_mutex)
		CSv_semaphore(&Dmc_rep->rep_sem);
	} /* if this is a replicated db */

	/*
	** Close all TCB's belonging to this database.  Core catalog TCB's
	** are not closed and must be done by hand.
	*/

	status = dm2d_release_user_tcbs(dcb, lock_list, 0, 0, dberr);

	/*
	** If there were errors encountered releasing the database's tcb's
	** then we cannot complete the database close.
	*/
	if (status != E_DB_OK)
	    break;

	/*
	** Release each of the four core catalogs.
	*/

	/*
	** Release the IIINDEX catalog.
	** Decrement the table reference count to release our fix of it.
	** If there are currently threads which reference the iiindex table,
	** then wait for them to finish.
	**
	** Also must wait if the TCB is busy. This can occur even when
	** tcb_ref_count and tcb_valid_count are zero because 
	** dm2t_release_tcb marks the TCB as TCB_BUSY, then releases
	** the protecting mutex.
	*/
	t = dcb->dcb_idx_tcb_ptr;
	hash_mutex = t->tcb_hash_mutex_ptr;
	dm0s_mlock(hash_mutex);
	while ( t->tcb_status & TCB_BUSY || t->tcb_ref_count > 1 )
	{
	    if (t->tcb_valid_count > 1)
	    {
		/*
		** Consistency Check:  since we are closing the database,
		** there should be no user threads which have fixed this tcb
		*/
		uleFormat(NULL, E_DM9C75_DM2D_CLOSE_TCBBUSY, (CL_ERR_DESC*)NULL,
		    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
		    (i4 *)NULL, err_code, (i4)6,
		    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		    sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		    0, t->tcb_ref_count, 0, t->tcb_valid_count,
		    0, t->tcb_status);
		release_error = TRUE;
	    }

	    /*
	    ** Register reason for waiting for debug/trace purposes.
	    */
	    t->tcb_status |= TCB_TOSS_WAIT;

	    /* Note that the wait call returns without the hash mutex. */
	    dm2t_wait_for_tcb(t, lock_list);
	    dm0s_mlock(hash_mutex);
	}
	t->tcb_ref_count--;
	status = dm2t_release_tcb(&t, 0, lock_list, (i4)0, dberr);
	dm0s_munlock(hash_mutex);
	if (status != E_DB_OK)
	{
	    tcb_close_error = TRUE;
	    break;
	}

	/*
	** Release the IIATTRIBUTE catalog.
	** Decrement the table reference count to release our fix of it.
	*/
	t = dcb->dcb_att_tcb_ptr;
	hash_mutex = t->tcb_hash_mutex_ptr;
	dm0s_mlock(hash_mutex);
	while ( t->tcb_status & TCB_BUSY || t->tcb_ref_count > 1 )
	{
	    if (t->tcb_valid_count > 1)
	    {
		/*
		** Consistency Check:  since we are closing the database,
		** there should be no user threads which have fixed this tcb
		*/
		uleFormat(NULL, E_DM9C75_DM2D_CLOSE_TCBBUSY, (CL_ERR_DESC*)NULL,
		    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
		    (i4 *)NULL, err_code, (i4)6,
		    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		    sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		    0, t->tcb_ref_count, 0, t->tcb_valid_count,
		    0, t->tcb_status);
		release_error = TRUE;
	    }

	    /*
	    ** Register reason for waiting for debug/trace purposes.
	    */
	    t->tcb_status |= TCB_TOSS_WAIT;

	    /* Note that the wait call returns without the hash mutex. */
	    dm2t_wait_for_tcb(t, lock_list);
	    dm0s_mlock(hash_mutex);
	}
	t->tcb_ref_count--;
	status = dm2t_release_tcb(&t, 0, lock_list, (i4)0, dberr);
	dm0s_munlock(hash_mutex);
	if (status != E_DB_OK)
	{
	    tcb_close_error = TRUE;
	    break;
	}

	/*
	** Release the IIRELATION catalog (and its index - iirel_idx).
	** Decrement the table reference count to release our fix of it.
	*/
	t = dcb->dcb_rel_tcb_ptr;
	hash_mutex = t->tcb_hash_mutex_ptr;
	dm0s_mlock(hash_mutex);
	while ( t->tcb_status & TCB_BUSY || t->tcb_ref_count > 1 )
	{
	    if (t->tcb_valid_count > 1)
	    {
		/*
		** Consistency Check:  since we are closing the database,
		** there should be no user threads which have fixed this tcb
		*/
		uleFormat(NULL, E_DM9C75_DM2D_CLOSE_TCBBUSY, (CL_ERR_DESC*)NULL,
		    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
		    (i4 *)NULL, err_code, (i4)6,
		    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
		    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		    sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		    0, t->tcb_ref_count, 0, t->tcb_valid_count,
		    0, t->tcb_status);
		release_error = TRUE;
	    }

	    /*
	    ** Register reason for waiting for debug/trace purposes.
	    */
	    t->tcb_status |= TCB_TOSS_WAIT;

	    /* Note that the wait call returns without the hash mutex. */
	    dm2t_wait_for_tcb(t, lock_list);
	    dm0s_mlock(hash_mutex);
	}
	t->tcb_ref_count--;
	status = dm2t_release_tcb(&t, 0, lock_list, (i4)0, dberr);
	dm0s_munlock(hash_mutex);
	if (status != E_DB_OK)
	{
	    tcb_close_error = TRUE;
	    break;
	}


	/* 
	** Tell the logging system we're not using this database
	** (unless 'no logging' specified).
	*/
	if (!(flag & DM2D_NLG))
	{
	    status = dm0l_closedb(dcb->dcb_log_id, dberr);
	    /* Don't leave out of date log_id hanging around */
	    dcb->dcb_log_id = 0;
	    if (status != E_DB_OK)
		break;
	}

	/*	Deallocate the EXT. */

	if (dcb->dcb_ext)
	    dm0m_deallocate((DM_OBJECT **)&dcb->dcb_ext);
	dcb->dcb_root = 0;
	dcb->dcb_jnl = 0;
	dcb->dcb_ckp = 0;
	break;
    }

    /*
    ** Call the buffer manager to decrement the reference count to this
    ** database.  Build a cache entry for this db (if there is not already
    ** one) so that the next time this db is opened we may be able to
    ** use the cached pages left from this session.
    */
    (void)dm0p_dbcache(dcb->dcb_id, 0, 0, lock_list, &local_dberr);

    /*
    ** If the database was open exclusively, then toss all pages
    ** stored in the buffer manager, as this may have been a DESTROYDB
    ** of the database.
    */
    if (dcb->dcb_status & DCB_S_EXCLUSIVE)
	dm0p_toss_pages(dcb->dcb_id, (i4)0, lock_list, (i4)0, 0);

    for (;;)
    {
    	if (status != E_DB_OK)
	    break;

	if (dcb->dcb_ref_count <= 0)
	{
	    /*  Release server locks on the database. */

	    if ((flag & DM2D_NLK) == 0)
	    {
		/*	Unlock open database table lock */

		cl_status = LKrelease((i4)0, svcb->svcb_lock_list,
		    (LK_LKID *)dcb->dcb_odb_lock_id, (LK_LOCK_KEY *)NULL,
		    (LK_VALUE *)NULL, &sys_err);

		if (cl_status)
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, 
                        (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);

		/*	Release temporary table id lock. */

		cl_status = LKrelease((i4)0, svcb->svcb_lock_list,
		    (LK_LKID *)dcb->dcb_tti_lock_id, (LK_LOCK_KEY *)NULL,
		    (LK_VALUE *)NULL, &sys_err);

		if (cl_status)
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, 
                        (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    }

	    if ((flag & (DM2D_NLK | DM2D_IGNORE_CACHE_PROTOCOL)) == 0)
	    {
		/*	Release server database lock. */

		if (dcb->dcb_served == DCB_SINGLE)
		    svcb_lock_list = svcb->svcb_ss_lock_list;
		else
		    svcb_lock_list = svcb->svcb_lock_list;

		db_key.lk_type = LK_SV_DATABASE;
		lock_value.lk_low = 0;
		lock_value.lk_high = 0;
		if (cl_status == OK)
		{
		    cl_status = LKrelease((i4)0,
			svcb_lock_list, (LK_LKID *)dcb->dcb_lock_id,
			(LK_LOCK_KEY *)NULL, &lock_value, &sys_err);
		}

		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, 
                        (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err,
			ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			err_code, 1, 0, svcb->svcb_lock_list);
		    status = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9266_UNLOCK_CLOSE_DB);
		    break;
		}
	    }

	    /* Deallocate any collation descriptors */

	    if (dcb->dcb_collation)
	    {
		dm2d_free(&dcb->dcb_collation);
		dcb->dcb_collation = NULL;
	    }
	    if (dcb->dcb_ucollation)
	    {
		dm2d_free(&dcb->dcb_ucollation);
		dcb->dcb_ucollation = NULL;
	    }
	    if (dcb->dcb_uvcollation)
	    {
		dm2d_free(&dcb->dcb_uvcollation);
		dcb->dcb_uvcollation = NULL;
	    }
	    
	    
	    /* Toss all linked DML_SEQ's */
	    if ( dcb->dcb_seq )
		dms_close_db(dcb);

	    dcb->dcb_ref_count = 0;
	}

	/*
	** Release database lock for caller, if the caller locked it.
	** Do this after completing the close operation so that
	** requesters waiting on this lock will not error if they
	** request the SV_DATABASE lock before we have released it.
	*/
	if ((flag & (DM2D_NLK | DM2D_NLK_SESS)) == 0)
	{
	    db_key.lk_type = LK_DATABASE;
	    cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)NULL, 
		(LK_LOCK_KEY *)&db_key, (LK_VALUE *)NULL, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, 
                    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    err_code, 1, 0, lock_list);
		status = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM9266_UNLOCK_CLOSE_DB);
		break;
	    }
	}


	if (status == E_DB_OK)
	{
	    /*	Unlock the DCB. */

	    dm0s_munlock(&dcb->dcb_mutex);
	    return (E_DB_OK);
	}
	break;
    }

    /*	Handle error reporting and recovery. */

    /*
    ** Mark database control block as invalid.
    ** This database will not be accessable by this server any longer.
    ** This restriction may be relaxed if we can figure out how to clean
    ** up all stuff left around when we fail trying to close the database.
    */
    dcb->dcb_status |= DCB_S_INVALID;

    dm0s_munlock(&dcb->dcb_mutex);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
                   NULL, 0, NULL, err_code, 0);
	SETDBERR(dberr, 0, E_DM9267_CLOSE_DB);
    }

    return (E_DB_ERROR);
}
/*{
** Name: dm2d_release_user_tcbs - Release non-core TCB's
**
** Description:
**	This routine releases all TCB's for non-core catalogs.  It
**	looks through the TCB hash table, finds all TCB's belonging to
**	the given databases, and releases them.
**
**	This is normally done at database close time, when no users
**	are using the database and it's being purged from the server.
**	It may also be done if the iirelation catalog is being modified.
**	If we're closing the database, nobody should be using any
**	tables;  if we're modifying iirelation, this must be createdb or
**	sysmod or some similar system utility that runs in exclusive
**	access mode, and again nobody (else) should be using any tables.
**
**	The reason for closing TCB's at database close time is reasonably
**	obvious;  nobody will be using those TCB's, and we want to
**	flush out any iirelation updates as well as update any dirty
**	cached pages for the table.  The reason for closing TCB's when
**	modifying iirelation is less obvious:  the iirelation row tid
**	is stored in the TCB, so that updating the row is made practical
**	(even if there are thousands of partitions around, sharing the
**	same base table ID!).  Clearly, if we're going to rewrite iirelation,
**	all those saved tids will change!  Rather than deal with the
**	problem, we'll make it go away, by closing all the TCB's.
**
**	The core catalog TCB's (iirelation/relidx, iiattribute, iiindex)
**	are hardwired open, and will not be changed.  The iidevices TCB
**	WILL be flushed out.
**
** Inputs:
**	dcb		Database control block for database to dump
**	lock_list	Lock list for session
**	log_id		Log ID for closing pages (0 for database close)
**	err_code	An output
**
** Outputs:
**	Returns DB_STATUS error status
**	err_code	Specific error E_xxx returned here if any
**
** History:
**	17-Jan-2004 (schka24)
**	    Extract from database close for sysmod to use.
**	5-Mar-2004 (schka24)
**	    Let release-tcb take the TCB off the TCB freelist.
**	14-Jan-2005 (jenjo02)
**	    Add "release_flag" to prototype so caller can ask for
**	    "DM2T_TOSS", for example.
**	02-Dec-2005 (jenjo02)
**	    If DMCM and tuple/page deltas, release the TCB with
**	    DM2T_KEEP to update iirelation, but cycle around
**	    again to wait for the TCB, which may now be busy/ref'd
**	    by writebehind threads to avoid DM937E_TCB_BUSY 
**	    errors. Fix to Alison's fix for B114846.
**	08-Jun-2007 (jonj)
**	    Can't DM2T_TOSS all pages in cache when DMCM is running
**	    with shared cache; some of those pages probably belong
**	    to other servers.
**	04-Apr-2008 (jonj)
**	    Preceding change re DMCM was wrong-headed. Push
**	    DMCM decisions into dm2t_release_tcb() where they belong.
*/

DB_STATUS
dm2d_release_user_tcbs(
DMP_DCB *dcb, 
i4 	lock_list, 
i4 	log_id, 
i4	release_flag,
DB_ERROR	*dberr)
{
    bool invalid_tcbs_found;
    bool release_error;		/* Error releasing this TCB */
    bool tcb_close_error;	/* Error releasing any TCB */
    DB_STATUS status;
    DMP_HASH_ENTRY *h;		/* Hash bucket address */
    DMP_HCB *hcb;		/* TCB hash control block */
    DMP_TCB *t;			/* TCB to release */
    DM_MUTEX *hash_mutex;	/* Mutex for this bucket */
    i4 i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    invalid_tcbs_found = FALSE;
    tcb_close_error = FALSE;

    /*
    ** Close all TCB's belonging to this database.  We cannot make an
    ** assumption here that all tcb's will be on the free list since
    ** it is possible that background jobs (CP, WB threads) are currently
    ** using some tcb's.  We must search through all the hash chains
    ** looking for tcb's to close.
    **
    ** Each time we free a TCB we must start our search loop back at the
    ** start of the current hash chain since we cannot trust the tcb_q_next
    ** pointers, having released the hash mutex.
    **
    ** If any errors are encountered while closing tcb's try to continue
    ** closing as many as possible to limit the resource waste and recovery
    ** needed.
    **
    ** FIXME dcb/tcb list might help here although it would have to
    ** be mutexed at the dcb level?
    */
    hcb = dmf_svcb->svcb_hcb_ptr;

    for (i = 0; i < HCB_HASH_ARRAY_SIZE; i++)
    {
	h = &hcb->hcb_hash_array[i];

	/* With the hash array larger these days, a lot of entries
	** might be empty.  Skip mutexing them if they are.
	*/
	if (h->hash_q_next != (DMP_TCB *) h)
	{
	    hash_mutex = &hcb->hcb_mutex_array[HCB_HASH_TO_MUTEX_MACRO(i)];
	    /* Lock hash entry's mutex */
	    dm0s_mlock(hash_mutex);

	    t = h->hash_q_next;

	    while (t != (DMP_TCB*) h)
	    {
		release_error = FALSE;

		/* Skip TCB's that don't belong to this database */
		if (t->tcb_dcb_ptr != dcb)
		{
		    t = t->tcb_q_next;
		    continue;
		}

		/*
		** Skip core catalog tcb's.  Note that iidevices is NOT
		** skipped, it is released here if it's open.
		*/
		if (t->tcb_rel.reltid.db_tab_base <= DM_B_INDEX_TAB_ID)
		{
		    t = t->tcb_q_next;
		    continue;
		}

		/*
		** Skip TCB's that could not be properly closed and
		** discarded.  These may be ones that returned an error
		** from the release call below or ones that became invalid
		** earlier while the database was in use.  Remember that
		** we encountered such tcb's so we can determine what
		** recovery actions (if any) are required.
		*/
		if (t->tcb_status & (TCB_INVALID | TCB_CLOSE_ERROR))
		{
		    invalid_tcbs_found = TRUE;
		    t = t->tcb_q_next;
		    continue;
		}

		/*
		** Found a TCB belonging to this database.
		** If the tcb is currently in use then wait for current fixers
		** to unfix it (note that the wait below will wake up each time
		** the tcb is unfixed, so it is possible for us to loop through
		** this case more than once before getting an unreferenced tcb.
		**
		** Also must wait if the TCB is busy. This can occur even when
		** tcb_ref_count and tcb_valid_count are zero because 
		** dm2t_release_tcb marks the TCB as TCB_BUSY, then releases
		** the protecting mutex.
		*/
		if ( t->tcb_status & TCB_BUSY ||
		    (t->tcb_ref_count > 0 && t->tcb_valid_count == 0) )
		{
		    /*
		    ** Register reason for waiting for debug/trace purposes.
		    */
		    t->tcb_status |= TCB_TOSS_WAIT;

		    /* The wait call returns without the hash mutex. */
		    dm2t_wait_for_tcb(t, lock_list);
		    dm0s_mlock(hash_mutex);
		}
		else if ((t->tcb_ref_count > 0) && (t->tcb_valid_count > 0))
		{
		    /*
		    ** Consistency Check: since we are closing the
		    ** database, there should be no user threads which have
		    ** fixed this tcb
		    */
		    uleFormat(NULL, E_DM9C75_DM2D_CLOSE_TCBBUSY, (CL_ERR_DESC*)NULL,
			ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			(i4 *)NULL, err_code, (i4)6,
			sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
			sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
			0, t->tcb_ref_count, 0, t->tcb_valid_count,
			0, t->tcb_status);
		    release_error = TRUE;
		}
		else
		{
		    status = dm2t_release_tcb(&t, release_flag, 
					lock_list, log_id, dberr);
    
		    if (status != E_DB_OK)
		    {
			uleFormat(dberr, 0, (CL_ERR_DESC*)NULL, ULE_LOG, 
			    (DB_SQLSTATE *) NULL, (char *)NULL, (i4)0,
			    (i4*)NULL, err_code, (i4)0);
			uleFormat(NULL, E_DM9C76_DM2D_CLOSE_TCBERR, (CL_ERR_DESC*)NULL,
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
			    (i4 *)NULL, err_code, (i4)6,
			    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
			    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			    sizeof(DB_OWN_NAME), 
				t->tcb_rel.relowner.db_own_name,
			    0, t->tcb_ref_count, 0, t->tcb_valid_count,
			    0, t->tcb_status);
			release_error = TRUE;
		    }
		}

		/* If an error occurred releasing the TCB then mark the
		** TCB as having encountered a close error so we do not
		** loop in this routine continuously attempting to
		** reclose the same table descriptor.
		*/
		if (release_error)
		{
		    /* Log nifty error message XXXX */
		    t->tcb_status |= TCB_CLOSE_ERROR;
		    tcb_close_error = TRUE;
		}

		/*
		** Start back at the top of the hcb queue.
		*/
		t = h->hash_q_next;
	    } /* bucket while */

	    /*
	    ** Continue around and try the next hash queue
	    */
	    dm0s_munlock(hash_mutex);
	} /* dirty-peek if */
    } /* for */

    status = E_DB_OK;
    if (tcb_close_error)
    {
	status = E_DB_ERROR;
	SETDBERR(dberr, 0, E_DM9270_RELEASE_TCB);
    }

    return (status);

} /* dm2d_release_user_tcbs */

/*{
** Name: dm2d_del_db	- Delete a database from the server.
**
** Description:
**      This function deletes a previously added database from the list of
**	databases that this server can process.  If the database is active,
**	a error message is returned, otherwise the DCB that describes the
**	database is deallocated.
**
** Inputs:
**      db_id                           The database identifier for the database
**					to delete.
**
** Outputs:
**      err_code                        The reason for a eror return status.
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
**	02-dec-1985 (derek)
**          Created new for jupiter.
**	 9-dec-1988 (rogerk)
**	    If the dcb is marked invalid, then don't delete it.  This will
**	    prevent the dcb from being rebuilt by another session.
**	26-apr-1993 (keving)
**	    We track the number of outstanding adds and only delete when
**	    the last user deletes the db.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
*/
DB_STATUS
dm2d_del_db(
    DMP_DCB	*db_id,
    DB_ERROR	*dberr )
{
    DMP_DCB             *dcb = (DMP_DCB *)db_id;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (dcb->dcb_type != DCB_CB)
	dmd_check(E_DMF00D_NOT_DCB);
#endif

    dmf_svcb->svcb_stat.db_delete++;

    /*	Lock the SVCB. */

    dm0s_mlock(&dmf_svcb->svcb_dq_mutex);

    dcb->dcb_add_count--;
    if (dcb->dcb_add_count)
    {
	/* Still outstanding users */
	dm0s_munlock(&dmf_svcb->svcb_dq_mutex);
	return (E_DB_OK);
    }

    if (dcb->dcb_ref_count || dcb->dcb_opn_count ||
	(dcb->dcb_status & DCB_S_INVALID))
    {
	/*  Unlock the SVCB. */
    
	dm0s_munlock(&dmf_svcb->svcb_dq_mutex);
	SETDBERR(dberr, 0, E_DM003F_DB_OPEN);
	return (E_DB_ERROR);
    }

    /*  Dequeue DCB from SVCB. */

    dcb->dcb_q_prev->dcb_q_next = dcb->dcb_q_next;
    dcb->dcb_q_next->dcb_q_prev = dcb->dcb_q_prev;
    dmf_svcb->svcb_cnt.cur_database--;

    dm0s_mrelease( &dcb->dcb_mutex );

    /*	Unlock the SVCB. */

    dm0s_munlock(&dmf_svcb->svcb_dq_mutex);

    /*	Deallocate DCB and return. */

    dm0m_deallocate((DM_OBJECT **)&dcb);
    return (E_DB_OK);
}

/*{
** Name: dm2d_open_db	- Open a database.
**
** Description:
**      Open a database that was previously added to the server.  If the
**	database is already open, then minimal things are required.  If the
**	database is not open, then the process of opening up a database is
**	started.  This process involves:
**	    o	Opening the configuration file.
**	    o	Read the relation, attribute and indexes description from the
**		configuration file.
**	    o   Read the list of location that this database spans and compare
**		it with the list passed in at dm2d_add_db time.  Update the list
**		to contain any new locations.
**
**
** Inputs:
**      db_id                           The database identifier to open.
**	access_mode			The access mode: DM2D_A_READ, 
**					DM2D_A_WRITE.
**	lock_mode			The database lock mode: DM2D_X, DM2D_IX.
**	lock_list			The lock list to own the database lock.
**	flag				Special open flags:
**					  DM2D_NLK : Database is not locked.
**					  DM2D_NLG : Database is not logged.
**					  DM2D_JLK : Database is only locked.
**					  DM2D_NOWAIT : Don't wait if couldn't
**						get the database lock.
**					  DM2D_ONLINE_RCP: open request by RCP.
**					  DM2D_RECOVER: open request by the
**						the CSP or RCP for recovery.
**					  DM2D_NOCK: Dont check config file
**						status or use count
**					  DM2D_FORCE: Force database consistent
**					  DM2D_NLK_SESS : not to request
**						session lock on the database.
**					  DM2D_IGNORE_CACHE_PROTOCOL : Bypass
**						cache database lock which
**					        enforces sole_cache protocols.
**						This is used by database
**						utilities to get read-only
**						access to db's open by fast
**						commit servers.
**					  DM2D_NOLOGGING : Don't write any log
**						records during the open process.
**						This is used by database
**						utilities to open db's in read-
**						only mode while the logging
**						system is frozen in LOGFULL.
**					  DM2D_CVCFG : Being opened by
**						upgradedb so convert tables if
**						necessary.
**					  DM2D_SYNC_CLOSE : Opened with 
**						file synchronization
**						during closedb.  Writes are
**						not guaranteed on disk.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-dec-1985 (derek)
**          Created new for jupiter.
**	 7-nov-88 (rogerk)
**	    Added update_tcb() routine to update the core system catalog
**	    TCB's to reflect more acurate tuple and pages counts.
**	 9-dec-1988 (rogerk)
**	    Return an error if the dcb status is DCB_S_INVALID.
**	04-jan-89 (mmm)
**	    altered to not call update_tcb() in the case of the RCP.
**	    update_tcb() can fail in that case if LKrequest returns LK_RETRY
**	    (which is possible if two servers have died, one still holding
**	    a system catalogue lock which update_tcb() attempts to acquire.
**	 6-Feb-89 (rogerk)
**	    Added shared buffer manager support.
**	    When lock the database for the server, check for correct lock
**	    value depending on the dcb_bm_served type.
**	    Call dm0p_dbcache to register open of database.
**	    Changed tcb hashing to use dcb_id instead of dcb pointer to form
**	    hash value.
**	 3-Mar-89 (ac)
**	    Added 2PC support.
**	 3-mar-89 (EdHsu)
**	    Online backup cluster support.
**	27-mar-89 (rogerk)
**	    Fixed bug with server database lock mode.  For multi-cache, lock
**	    in IS mode first and then convert to SIX only if need to set lock
**	    value.  Specify lock value when release lock.
**	27-mar-1989 (EdHsu)
**	    Move cluster online backup support at bt level
**	24-Apr-89 (anton)
**	    Added local collation support
**	15-may-1989 (rogerk)
**	    Check for lock quota exceed condition.  Check for bad return
**	    status from dm0p_dbcache.
**	 5-jun-1989 (rogerk)
**	    Release dcb_mutex semaphore when find INVALID DCB.
**	19-jun-1989 (rogerk)
**	    Toss pages from buffer manager whenever db is opened
**	    exclusively to make sure we don't cache pages after a db is
**	    destroyed or restored from checkpoint.
**	23-Jun-1989 (anton)
**	    Moved local collation routines from ADU to CL
**	14-aug-1989 (rogerk)
**	    Moved call to dm0p_dbcache up to above where we return if
**	    calling in JUST_LOCK mode.  Need to bump database cache lock
**	    when opening database for rollforwarddb.
**	11-sep-1989 (rogerk)
**	    Save lx_id returned from dm0l_opendb call and use it in
**	    dm0l_closedb call.
**	 2-oct-1989 (rogerk)
**	    Don't blindly zero out the dcb lockid fields in opendb.  We may
**	    be calling opendb after we already have the db locked (by a
**	    previous call with JLK mode).  When cleaning up after an error,
**	    don't try to release locks if called with NLK mode.
**	12-dec-1989 (rogerk)
**	    Don't check open db count for database consistency when DM2D_NLK
**	    option is specified, as the lock value is not initialized.
**	10-jan-1990 (rogerk)
**	    Added DM2D_OFFLINE_CSP flag which indicates that the database is
**	    being opened by the CSP during startup recovery by the master.
**	    If the database is opened by the CSP, then either the DM2D_CSP
**	    (which is set in node failure recovery) or the DM2D_OFFLINE_CSP
**	    flag should be set.
**	17-may-90 (bryanp)
**	    After we force the database consistent, make a backup copy of the
**	    config file.
**	    When opening the config file for the RFP, if the ".rfc" file
**	    cannot be found, try the ".cnf" file in the dump area.
**	20-nov-1990 (rachael)
**	    Added line in dm2d_open_db to set dcb->dcb_status to not journaled 
**	    if the cnf->cnf_dsc->dsc_status is not set to journaled.
**	28-jan-91 (rogerk)
**	    Added DM2D_IGNORE_CACHE_PROTOCOL flag in order to allow auditdb
**	    to run on a database while it is open by a fast commit server.
**	    When this flag is enabled, we bypass the server database lock.
**	    (Unlike the NLK flag which will bypass the database lock as well)
**	 4-feb-91 (rogerk)
**	    Added new argument to dm0p_toss_pages call.
**	25-feb-1991 (rogerk)
**	    Added DM2D_NOLOGGING flag for open_db.  This allows callers to
**	    open databases in READ_ONLY mode without requiring any log
**	    records to be written - so that utilities may connect to
**	    databases while the logfile is full.  This was added as part
**	    of the Archiver Stability Project.
**	25-mar-1991 (rogerk)
**	    Check for a database which had no recovery performed on it
**	    due to being in Set Nologging state at the time of a system
**	    crash.  If we are the first opener of a database and the
**	    DSC_NOLOGGING bit is still on, then the database was not
**	    cleaned up properly from a Set Nologging call and must be
**	    considered inconsistent.
**	10-jun-1991 (bryanp, rachael)
**	    B37190: If update_tcb() call fails, database reference count is
**	    not handled properly, leading to subsequent errors. Moved the
**	    reference count code until after the update_tcb() calls are made.
**	17-oct-1991 (rogerk)
**	    Merged changes made by Derek during the file extend project.
**	    Added support to automatically update relation records in the
**	    config file with the new allocation fields.
**	05-may-1992 (ananth)
**	    Increasing IIrelation size project.
**	    Use compile time information instead of the config file to
**	    open the database.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project.  Added support for Partial TCB's.
**	    Filled in new table_io fields of tcb.  Added some new error
**	    messages during error handling.
**	18-jan-1993 (rogerk)
**	    Reduced Logging Project: Changed DM2D_RCP flag to DM2D_RECOVER
**	    since it is passed from more than just the rcp process.  Also
**	    removed the OFFLINE_CSP flag.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Remove DM2D_CSP and DCB_S_CSP status flags, since it's no longer
**		    required that a DCB be flagged with whether it was built by
**		    a CSP process or not.
**	17-may-1993 (ralph)
**	    DELIM_IDENT:
**	    Call update_tcbatts() for each core catalog after all calls to 
**	    update_tcb().
**	26-jul-1993 (jnash)
**	    Move error checking after call to construct_tcb to avoid AV if 
**	    rel_rcb not built.
**	26-jul-1993 (bryanp)
**	    We no longer log a closedb log record, which means that we no
**		longer need to pass a lx_id argument to dm0l_opendb and to
**		dm0l_closedb.
**	23-aug-1993 (rogerk)
**	    Turn on DCB_S_RECOVER flag when DM2D_RECOVER is specified.
**	    Changed DCB_S_RCP to DCB_S_ONLINE_RCP.
**	    Changed DM2D_RCP to DM2D_ONLINE_RCP.
**	    This allows recovery code to distinguish between opening db for 
**	    recovery and openening it for online recovery.
**      10-sep-1993 (pearl)
**          In dm2d_open_db(), add error E_DM0152_DB_INCONSIST_DETAIL if
**          database is inconsistent; this error will name the database in
**          question.
**	01-oct-93 (jrb)
**	    Added logging of warning message if we are opening the iidbdb and
**	    journaling is not enabled.
**	22-nov-93 (jrb)
**	    Moved my last change to dmc_open_db instead.
**	20-jun-1994 (jnash)
** 	    Add DM2D_SYNC_CLOSE flag.  Used only by rollforwarddb (so far), 
**	    this causes files to be opened without synchronization and then 
**	    forced during the database close (actually during table close
**	    in dm2t_release_tcb()).  DCB_S_SYNC_CLOSE propagated 
**	    to dcb_status.
**	15-feb-1996 (tchdi01)
**	    Set the "production mode" flags
**	20-may-96 (stephenb)
**	    Add replication code, we will check for replicated databases
**	    here and set flag in DCB, at the same time we get the reltid and
**	    reltidx of some replcator catalogs for later use and fill out
**	    the replicator database number in the DCB.
**	24-jul-96 (stephenb)
**	    use deffered update and no readlock on replication table(s) opened
**	    in above change. Also only set DCB_S_REPLICATE if we find the
**	    local db record in dd_databases.
**	5-aug-96 (stephenb)
**	    Replication recovery processing, if we are replicating the database
**	    and we are the first user to open the DB, then clean out
**	    the input queue at this time.
**	21-aug-96 (stephenb)
**	    Check for existance for the dd_tranaction_id table before enabling
**	    replication. This will allow us to support previous versions of
**	    replicator with 2.0, since in previous versions this table will
**	    exist, but it has been removed for replicator 2.0.
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          If database opened with DMCM, write FASTCOMMIT into database
**          open log record.
**	13-jan-97 (stephenb)
**	    Temporarily remove replicator consistency check that looks to
**	    see if dd_databases record that is_local aslo contains the same
**	    db name as the database we are opening. This check causes problems
**	    with some of the replicator admin tools.
**      12-feb-97 (dilma04)
**          Set Lockmode Cleanup:
**          Changed parameters in dm2t_open() call.
**	28-feb-1997 (cohmi01)
**	    Keep track in hcb of tcbs created, for bugs 80242, 80050.
**	13-mar-97 (stephenb)
**	    When checking for replication, if the catalogs exist but we haven't
**	    yet set replication up, then set DCB_S_REPCATS so that we can
**	    start replication at a later point.
**	15-May-1997 (jenjo02)
**	    When requesting database lock, use an interruptable LKrequest().
**	30-may-1997 (wonst02)
**	    More changes for readonly database.
**	12-aug-1997 (wonst02)
**	    Bug 84405: ckpdb dbname fails if the database was created with 
**	    alternate location. Removed offending code.
**	24-aug-97 (stephenb)
**	    must check for upercase local_db field when replicating an
**	    sql92 compliant installation
**	2-sep-97 (stephenb)
**	    If we are runnning from a non-user server (RCP, ACP or CSP), then
**	    don't execute the replicator code.
**	25-sep-97 (stephenb)
**	    Add SINGLEUSER to above change for JSP servers.
**	25-oct-1997 (nanpr01)
**	    Fix the possiblity of BUS error from replication.
**	7-mar-98 (stephenb)
**	    Better error reporting, log error return from dm2rep_qman()
**      28-may-1998 (stial01)
**          dm2d_open_db() Support VPS system catalogs.
**          Added recovery for DM0LSM1RENAME,skip update_tcb* if SM_INCONSISTENT
**	23-oct-98 (stephenb)
**	    Make sure we don't increment the replication queue seg size when
**	    we move the tail forward.
**	4-dec-98 (stephenb)
**	    Replicator trans_time is now an HRSYSTIME pointer, update function
**	    calls to reflect this.
**	09-Mar-1999 (schte01)
**	    Changed assignment of j within for loop so there is no dependency 
**	    on order of evaluation.
**	29-sep-1999 (nanpr01)
**	    bug 98948 - Access to read only database is causing server crash
**	    because it is looking at wrong physical location. Read-Only data
**	    should always open the shadow cnf file in $II_DATABASE location.
**      06-Mar-2000 (hanal04) Bug 100742 INGSRV 1122.
**          The DCB mutex is held when calling dm2rep_qman() which calls
**          dmxe_begin() which calls dm2d_check_db_backup_status() which
**          tries to acquire the same DCB mutex. Set and unset the dcb_status
**          with DCB_S_MLOCKED in the surrounding dm0s_mlock() / dm0s_munlock()
**          calls.
**      20-Mar-2001 (inifa01) bug 104267, INGREP 92.
**         The dbms server becomes unresponsive after taking the following errors on
**         a db open of a replicated database: E_DM9042_PAGE_DEADLOCK (on dd_input_queue)
**         W_DM9578_REP_QMAN_DEADLOCK, E_DM9569_REP_ING_CLEANUP,E_SC0121_DB_OPEN and
**         E_DM003F. Sometimes occurs with SEGV in dmc_open_db(0x28d8ec_ @ PC 28dc84
**         and/or E_SC037C_REP_QMAN_EXIT.
**         In dm2d_open_db just before the input queue is checked for outstanding
**         records dcb->dcb_ref_count is bumped up marking the database as successfully
**         opened.  If we fail to clear out the queue due E_DM9569_REP_INQ_CLEANUP
**         the db open fails.  During cleanup we fail to bump down dcb->dcb_ref_count.
**         Bump down db open count if rep input queue cleanup fails. 
**	10-May_2001 (inifa01) bug 104696 INGREP 94.
**	   Replicator database becomes inaccesible after taking the following 
**	   errors:
**	   E_DM9569_REP_INQ_CLEANUP, E_DM937E_TCB_BUSY, E_DM9C76_DM2D_CLOSE_TCBERR 
**	   error closing Table Control Block at database close time. The TCB for table 
**	   (dd_databases, dsnhdba) of database dsnhaimp could not be released 
**	   : tcb_ref_count -1, tcb_valid_count -1, tcb_status 0x00000001.
**	   Failure is due to fact that tcb_ref_count == -1.  This shouldn't happen.
**	   Fix was to ensure that the TCB is not unnecessarily unfixed via a 
**	   dm2t_close() call during cleanup.  Set repdb_rcb to NULL if table close
**	   is successful. 
**	01-May-2002 (bonro01)
**	   Fix memory overlay caused by using wrong length for char array
**	04-Feb-2003 (jenjo02)
**	   Remove reliance on (dcb_dbservice & DU_UTYPES_ALLOWED)
**	   when deciding on whether to load Unicode code tables;
**	   it's never set in non-server (RCP,RFP,etc)
**	   environments. (B109577)
**	16-Jan-2004 (schka24)
**	   Changes for revised hash bucket and mutex stuff.
**	27-May-2004 (jenjo02)
**	   Use iirelation index to probe for "dd_?" replicator
**	   tables. A full table scan in databases with lots of
**	   partitions is a killer.
**	19-Jul-2004 (jenjo02)
**	   Fixed botched 27-May attempt to speed replicator
**	   check.
**	18-Jan-2005 (jenjo02)
**	   dm0p_dbcache() now returns E_DB_WARN if the database's
**	   cache was purged; if that's the case, purge other
**	   stuff that is related to the DB, like sequences and
**	   cached user TCBs.
**	   Don't call aduucolinit() if the config file indicates
**	   Unicode types are not supported, overriding any
**	   collation table named in dsc_ucollation.
**	02-Mar-2005 (inifa01) INGREP167 b113572
**	    Problems when using Replicator 1.1 in a 2.5 database.  
**	    Sometimes and randomly the 2.5 change recording becomes
**	    active causing several problems(duplicate information in
**	    shadow table and dd_distrib_queue).  Investigations 
**	    determined that sometimes dm2r_get() does not retrieve the
**	    dd_transaction_id table from iirelation even though it is 
**	    present.  
**	    FIX: Introduced use of system variable II_DBMS_REP to switch
**	    OFF any replication processing by the DBMS server. 
**	31-Oct-2005 (gupsh01)
**	    Fixed testing for dbms_replicate. The test below caused it
**	    not to replicate in all cases.
**	21-jul-2006 (kibro01) bug 114168
**	    Tell dm2rep_qman that this is a hijacked call, and shouldn't
**	    keep trying if deadlocks occur.
**	9-Nov-2005 (schka24)
**	    Don't fetch II_DBMS_REP over and over, check it in svcb status.
**	10-mar-2006 (jenjo02, gupsh01)
**	    Fixed for mdb create slow on solaris. If DB is opened for
**	    exclusive access, don't use synchronous writes on any
**	    tables in the database.
**	13-apr-2007 (shust01)
**	    server crash when trying to access database while an external
**	    program is locking the physical database files. b118043 .
**	15-may-2007 (gupsh01)
**	    Load the unicode collation for iidbdb if II_CHARSETxx is set to
**	    UTF8.
**	29-Nov-2007 (jonj)
**	    When opening a DB NOWAIT, wait a bit anyway to give 
**	    terminating sessions time to terminate and release their
**	    incompatible DATABASE locks.
**	13-May-2008 (kibro01) b120369
**	    Add second flags for DMC2_ADMIN_CMD check for replicator.
**	21-May-2008 (jonj)
**	    If DB fails to open, release all TCB mutexes before
**	    deallocating the core TCBs.
**      16-Jun-2009 (hanal04) Bug 122117
**          Check for new LK_INTR_FA error.
**      10-Jul-2009 (hanal04) Bug 122130
**          If restricting access to read-only on an inconsistent DB
**          send the user a warning through an SCC_MESSAGE call.
**	6-Nov-2009 (kschendel) SIR 122757
**	    Count opens of DB's with raw locations.
**	    Undo 10-Mar-2006 change.  It is too broad (applies to e.g.
**	    sysmod, offline recovery) and the lack of sync can totally
**	    destroy a database if power disappears at the wrong time.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Populate LG_JFIB from journal info in
**	    config file. After dm0l_opendb(), LGalter(LG_A_JFIB)
**	    to set that information in LG's context.
**	16-Mar-2010 (frima01) SIR 121619
**	    Make use of IIDBDB_ID macro.
*/
DB_STATUS
dm2d_open_db(
    DMP_DCB	*db_id,
    i4     access_mode,
    i4     lock_mode,
    i4     lock_list,
    i4   	flag,
    DB_ERROR		*dberr )
{
    DM_SVCB		*svcb = dmf_svcb;
    DMP_DCB		*dcb = (DMP_DCB *)db_id;
    DMP_RELATION	iirel;
    DMP_RELATION	iirel_idx;
    DMP_RELATION	iiatt;
    DMP_RELATION	iiindex;
    DMP_RELATION	rep_rel;
    DMP_RINDEX		rep_idx;
    DMP_HASH_ENTRY	*h;
    DM_MUTEX		*hash_mutex;
    DMP_HCB             *hcb;
    DM0C_CNF		*config = 0;
    DM0C_CNF		*cnf;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		loc_status;
    STATUS		cl_status;
    i4             cnf_flag = 0;
    i4		error;
    i4             local_error;
    i4             msg_error;
    DMP_TCB		*rel_tcb = 0;
    DMP_TCB		*relx_tcb = 0;
    DMP_TCB		*att_tcb = 0;
    DMP_TCB		*idx_tcb = 0;
    i4		i;
    i4             n;
    i4		lk_mode;
    LK_VALUE		odb_k_value;
    LK_VALUE		lock_value;
    LK_LOCK_KEY		db_key;
    LK_LKID		dblkid;
    CL_ERR_DESC          sys_err;
    i4		open_flag;
    i4			write_mode;
    i4			dbcache = FALSE;
    ADULTABLE		*tbl;
    ADUUCETAB		*utbl;
    char		*uvtbl;
    bool		SMS_converted_flag = TRUE;
    bool		has_raw = FALSE;
    DB_TRAN_ID		tran_id;
    DMP_RCB		*rcb = NULL;
    char		tab_name[DB_TAB_MAXNAME]; 
    DM_TID		tid;
    DB_TAB_ID		rep_db;
    DB_TAB_ID		rep_db_idx;
    DB_TAB_TIMESTAMP	timestamp;
    DMP_RCB		*repdb_rcb = NULL;
    DMP_RCB		*idx_rcb = NULL;
    DMP_TCB		*repdb_tcb = NULL;
    DM_TID		repdb_tid;
    char		*repdb_rec = NULL;
    i2 			is_local = 0;
    DM2R_KEY_DESC	key_desc;
    i4			offset;
    char		dbname[DB_DB_MAXNAME];
    i4             svcb_lock_list;
    bool                sm_inconsistent = FALSE;
    LK_EVENT		lk_event;
    bool                db_opened = FALSE;  /* added for b104267 */
    DM2R_KEY_DESC	qual_list[2];
    i4			*err_code = &dberr->err_code;
    i4			local_err;
    DB_ERROR		local_dberr;
    char		buf[DB_ERR_SIZE];
    i4			len;
    SCF_CB		scf_cb;
    LG_JFIB		jfib;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (dcb->dcb_type != DCB_CB)
	dmd_check(E_DMF00D_NOT_DCB);
#endif

    svcb->svcb_stat.db_open++;
    hcb = svcb->svcb_hcb_ptr;
    MEfill(sizeof(LK_LKID), 0, &dblkid);

    /*  Check access mode and set locks. */

    if (access_mode > dcb->dcb_access_mode)
    {
	SETDBERR(dberr, 0, E_DM003E_DB_ACCESS_CONFLICT);
	return (E_DB_ERROR);
    }

    /*  Lock the DCB. */

    dm0s_mlock(&dcb->dcb_mutex);

    /*	Initialize the lock key. */

    MEcopy((PTR)&dcb->dcb_name, LK_KEY_LENGTH,(PTR)&db_key.lk_key1);
    
    /*	Increment the open pending count. */

    dcb->dcb_opn_count++;

    /*	Release the DCB. */
    
    dm0s_munlock(&dcb->dcb_mutex);

    /*	Lock the database for the caller. */

    if ((flag & (DM2D_NLK | DM2D_NLK_SESS)) == 0)
    {
	i4	    LockTimeout = 0;

	/*  Request database lock for caller. */

	db_key.lk_type = LK_DATABASE;

	if ( flag & DM2D_NOWAIT )
	{
	    /*
	    ** If NOWAIT, wait some number of seconds anyway.
	    **
	    ** Normally, we'll never wait for the lock, but
	    ** can occur when two sessions open the DB
	    ** in incompatible modes, like a connect
	    ** IS/IX quickly following on the tails of an
	    ** X-locked DB session that is in the throes of
	    ** terminating (or vice-versa).
	    **
	    ** Waiting a bit might give enough time for ending
	    ** sessions to release their locks on the database 
	    ** (for example, the WB threads are still flushing
	    ** DB pages from the cache, thereby stalling the
	    ** closing of the session), and reduce or avoid those 
	    ** pesky US0014 Database not available
	    ** errors on quick reconnects.
	    */

	    /* Wait no more than 5 seconds */
	    LockTimeout = 5;
	}

	/* Request an interruptable lock */
	cl_status = LKrequest(LK_PHYSICAL | LK_INTERRUPTABLE, 
			lock_list, (LK_LOCK_KEY *)&db_key,
	    		lock_mode, (LK_VALUE *)NULL, (LK_LKID *)&dblkid, 
	    		LockTimeout, &sys_err);

	if ( cl_status != OK )
	{
	    if (cl_status == LK_TIMEOUT)
	    {
		/*
		** Timed out waiting for NOWAIT lock,
		** return resource busy.
		*/
		SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
	    }
	    else if (cl_status == LK_NOLOCKS)
		SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
	    else if (cl_status == LK_INTERRUPT)
		SETDBERR(dberr, 0, E_DM004E_LOCK_INTERRUPTED);
            else if (cl_status == LK_INTR_FA)
                SETDBERR(dberr, 0, E_DM016B_LOCK_INTR_FA);
	    else
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, 
		    (DB_SQLSTATE *)NULL, 0, 
		    0, 0, err_code, 2, 0, lock_mode, 0, lock_list);
		SETDBERR(dberr, 0, E_DM9268_OPEN_DB);
	    }

	    /*	Lock the DCB. */

	    dm0s_mlock(&dcb->dcb_mutex);

	    /*	Decrement open count. */

	    dcb->dcb_opn_count--;

	    /*	Unlock the DCB. */

	    dm0s_munlock(&dcb->dcb_mutex);

	    return (E_DB_ERROR);
	}
    }

    /*	Lock the DCB. */

    dm0s_mlock(&dcb->dcb_mutex);
    dcb->dcb_status |= DCB_S_MLOCKED;

    /*
    ** If the dcb is marked invalid, then the server previously encountered
    ** an error trying to close the database.  This server can no longer access
    ** that database due to invalid control blocks an cached pages that may
    ** have been left lying around the server.
    */
    if (dcb->dcb_status & DCB_S_INVALID)
    {
	/* Release the caller's database lock */
	if (dblkid.lk_common)
	{
	    cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)&dblkid,
		(LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);
	    if (cl_status)
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	}

	/*
	** Log message saying that the server must be shut down in order to
	** reaccess the database, then return an error specifying that the
	** database is not available.
	*/
        dcb->dcb_status &= ~(DCB_S_MLOCKED);
	dm0s_munlock(&dcb->dcb_mutex);
	uleFormat(NULL, E_DM927F_DCB_INVALID, (CL_ERR_DESC *)NULL, ULE_LOG, 
            (DB_SQLSTATE *)NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
	    sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name);
	SETDBERR(dberr, 0, E_DM0133_ERROR_ACCESSING_DB);
	return (E_DB_ERROR);
    }

    if (dcb->dcb_ref_count)
    {
	/*  Increase reference count, decrease pending open count. */

	dcb->dcb_ref_count++;
	dcb->dcb_opn_count--;

	/*  Unlock the mutex. */

        dcb->dcb_status &= ~(DCB_S_MLOCKED);
	dm0s_munlock(&dcb->dcb_mutex);

	return (E_DB_OK);
    }
    
    /*
    **	Database is not open, so go through the process of opening the
    **	the database.  This process involves:
    **
    **	o   Lock database for server.
    **  o   Allocate memory for configuration record.
    **  o   Read configuration record from configuration file.
    **  o   Process the configuration records.
    **  o   Build TCB's for the system tables.
    **  o   Build FCB's for these TCB's.
    **  o   Log the database open if the database is writable.
    */

    MEcopy((PTR)&DM_core_relations[0], sizeof (DMP_RELATION),(PTR)&iirel);
    MEcopy((PTR)&DM_core_relations[1], sizeof (DMP_RELATION),(PTR)&iirel_idx);
    MEcopy((PTR)&DM_core_relations[2], sizeof (DMP_RELATION),(PTR)&iiatt);
    MEcopy((PTR)&DM_core_relations[3], sizeof (DMP_RELATION),(PTR)&iiindex);

    for (;;)
    {
	/*  Lock database for server if not locked already. */

	if ((flag & (DM2D_NLK | DM2D_IGNORE_CACHE_PROTOCOL)) == 0)
	{
	    /*
	    ** Lock the database for the server and buffer manager.
	    ** This will enforce database/server access mode requested by
	    ** caller.
	    **
	    ** If using SINGLE-SERVER mode, then the database should be locked
	    ** in X mode to block all other servers from opening it.
	    ** For SINGLE-SERVER mode, we place the SV_DATABASE lock on
	    ** a protected lock list, so if this server crashes, the
	    ** RCP will inherit the X SV_DATABASE lock. (Non-protected lock
	    ** lists are released during the LK rundown of a process).
	    **
	    ** If using MULTI-SERVER, then the database is locked in IS mode.
	    ** The lock value enforces single or multiple cache access.  The
	    ** first portion of the lock value specifies the cache mode -
	    ** single or multiple.  The second part of the value gives the
	    ** buffer manager id if the mode is single.
	    */
	    db_key.lk_type = LK_SV_DATABASE;
	    lk_mode = LK_IS;
	    svcb_lock_list = svcb->svcb_lock_list;
	    if (dcb->dcb_served == DCB_SINGLE)
	    {
		lk_mode = LK_X;
		svcb_lock_list = svcb->svcb_ss_lock_list;
	    }

	    dcb->dcb_lock_id[0] = 0;
	    cl_status = LKrequest((LK_PHYSICAL | LK_NOWAIT),
		svcb_lock_list, &db_key, lk_mode, &lock_value,
		(LK_LKID *)dcb->dcb_lock_id, (i4)0, &sys_err); 
	    if ( ( (cl_status == OK) || (cl_status == LK_VAL_NOTVALID) ) && 
                 (dcb->dcb_served != DCB_SINGLE) )
	    {        
		/*
		** If the database is being opened in MULTI-SERVER mode, then
		** check the lock value to make sure our buffer manager is
		** allowed to access the database.
		**
		** If the lock value is not set, then we are the only server
		** with the database open - convert the lock to SIX and assign
		** the value according to the mode with which we are opening
		** the database.
		**
		** If the lock value is set, then make sure it is compatable
		** with the mode we are opening the database (MULTI_CACHE or
		** SINGLE_CACHE) and if SINGLE_CACHE, make sure the value
		** specifies our buffer manager's id.
		*/
		if (lock_value.lk_low == 0)
		{
		    /*
		    ** Lock value not set, convert lock to SIX to set value
		    ** according to how we are opening it.  Check the lock
		    ** value again after acquiring the lock since another server
		    ** may also be opening the db at the same time.  If the
		    ** lock value is still unset, then set it.
		    */
		    cl_status = LKrequest(
			(LK_PHYSICAL | LK_CONVERT),
			svcb_lock_list, &db_key, LK_SIX, &lock_value,
			(LK_LKID *)dcb->dcb_lock_id, (i4)0, &sys_err);
		    if ( ( (cl_status == OK) || (cl_status == LK_VAL_NOTVALID)) && 
                         (lock_value.lk_low == 0))
		    {
			lock_value.lk_low = MULTI_CACHE;
			if (dcb->dcb_bm_served == DCB_SINGLE)
			{
			    lock_value.lk_low = SINGLE_CACHE;
			    lock_value.lk_high = dm0p_bmid();
			}
		    }
		    if ( (cl_status == OK) || (cl_status == LK_VAL_NOTVALID))
		    {
			cl_status = LKrequest(
			    (LK_PHYSICAL | LK_CONVERT),
			    svcb_lock_list, &db_key, LK_IS, &lock_value,
			    (LK_LKID *)dcb->dcb_lock_id, (i4)0, &sys_err);
		    }
		}

		if (cl_status == LK_VAL_NOTVALID)
                {
                   /* The LK_VAL_NOTVALID is treated as an OK status */
                   cl_status = OK;
                }    

                /*
		** Check that we are opening the database in a compatable mode
		** with what is open in.  If we are opening in multiple-cache
		** mode, then the lock value must specify MULTI_CACHE.  If we
		** are opening in single-cache mode then the lock value must
		** specify SINGLE_CACHE and the buffer manager id of our
		** buffer manager.
		**
		** If the database is not open in a compatable mode, then set
		** the lock status to LK_BUSY.
		*/
		if (cl_status == OK)
		{
		    if (((dcb->dcb_bm_served == DCB_MULTIPLE) &&
			    (lock_value.lk_low != MULTI_CACHE)) ||
			((dcb->dcb_bm_served == DCB_SINGLE) &&
			    ((lock_value.lk_low != SINGLE_CACHE) ||
			     (lock_value.lk_high != dm0p_bmid()))))
		    {
			cl_status = LK_BUSY;
		    }
		}
	    }
            else if (cl_status == LK_VAL_NOTVALID)
            {
               /* We've got the lock exclusive, but the value isn't valid */
               cl_status = OK;
            }    
            
	    if ((cl_status == LK_BUSY) || (cl_status == LK_TIMEOUT))
	    {
		SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
		break;
	    }
	    else if (cl_status != OK)
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	}

	/*
	** Get temporary-table-id lock and first-db-opener lock.
	*/
	if ((flag & DM2D_NLK) == 0)
	{
	    dcb->dcb_odb_lock_id[0] = 0;
	    dcb->dcb_tti_lock_id[0] = 0;

	    if (cl_status == OK)
	    {
		/* Get the temporary table id lock. */
		db_key.lk_type = LK_DB_TEMP_ID;
		cl_status = LKrequest(LK_PHYSICAL,
		    svcb->svcb_lock_list, &db_key, LK_N, (LK_VALUE *)NULL, 
		    (LK_LKID *)dcb->dcb_tti_lock_id, (i4)0, &sys_err); 
	    }

	    if (cl_status == OK)
	    {
		/* Get the dbopen lock, used to determine if 
		** this is the first openner of any database. 
		*/
		db_key.lk_type = LK_OPEN_DB;
		cl_status = LKrequest(LK_PHYSICAL,
		    svcb->svcb_lock_list, &db_key, LK_X, &odb_k_value,
		    (LK_LKID *)dcb->dcb_odb_lock_id, (i4)0, &sys_err); 
	    }

	    /*	Check for any errors. */
	    if (cl_status != OK && cl_status != LK_VAL_NOTVALID)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, 
                    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    err_code, 2, 0, LK_N, 0, svcb->svcb_lock_list);
		if (cl_status == LK_NOLOCKS)
		    SETDBERR(dberr, 0, E_DM004B_LOCK_QUOTA_EXCEEDED);
		else if ((cl_status == LK_BUSY) || (cl_status == LK_TIMEOUT))
		    SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
		else
		    SETDBERR(dberr, 0, E_DM9269_LOCK_OPEN_DB);
		break;
	    }
	}

	/* If access mode is read-only, pass the cnf_flag accordingly */
       if ((access_mode == DM2D_A_READ) &&
          ((svcb->svcb_status & SVCB_SINGLEUSER) == 0) )
	    cnf_flag |= DM0C_READONLY;


	/*  Open the configuration file. If this is rollforward, and we can't
	**  open the regular config file, try to open the .rfc version which
	**  rollforward may have created, and if that fails try the .cnf
	**  backup copy in the dump directory.
	*/
	loc_status = dm0c_open(dcb, cnf_flag, lock_list, &cnf, dberr);
	if (loc_status != E_DB_OK)
	{
	    if (flag & DM2D_RFP)
	    {
		loc_status = dm0c_open(dcb, cnf_flag | DM0C_TRYRFC, lock_list,
					&cnf, dberr);
	    	if (loc_status != E_DB_OK)    	
		    loc_status = dm0c_open(dcb, cnf_flag | DM0C_TRYDUMP,
					lock_list, &cnf, dberr);
	    	if (loc_status != E_DB_OK)    	
		    break;
	    }
	    else
		break;
	}
	config = cnf;

	/* Force READONLY access if incremental rollforward started */
	if ((config->cnf_dsc->dsc_status & DSC_INCREMENTAL_RFP) 
			&& (flag & DM2D_RFP) == 0)
	{
	    if (access_mode != DCB_A_READ)
	    {
#ifdef xDEBUG
		TRdisplay("Force READONLY access during incremental rollforward\n");
#endif
		access_mode = DCB_A_READ;
		dcb->dcb_access_mode = DCB_A_READ;
	    }
	}

	/*
	** Check if SYSMOD was in progress..
	** Bypass update_tcb and update_tcbatts if DSC_SM_INCONSISTENT 
	*/
#ifdef xDEBUG
	TRdisplay("Open database %~t dcb status %d cnf_status %d\n", 
		sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name,
		dcb->dcb_status, cnf->cnf_dsc->dsc_status);
#endif
	if ( (dcb->dcb_status & (DCB_S_RECOVER | DCB_S_ONLINE_RCP))
	    && (cnf->cnf_dsc->dsc_status & DSC_SM_INCONSISTENT))
	{
	    TRdisplay("Database %~t is marked inconsistent by SYSMOD\n",
		sizeof(dcb->dcb_name.db_db_name), dcb->dcb_name.db_db_name);
	    sm_inconsistent = TRUE;
	}


	/*
	** If the database is not at the current core-catalog version,
	** check that iiattribute conversion has completed. (V7).
	** For old pre-V3 databases, make sure that iirelation conversion
	** completed as well.
	** If however we're at V7 already, nothing more needs to be done
	** here; upgradedb will take care of things and use ii_update_config
	** to fix up the core-catalog version.
	*/
	if  (cnf->cnf_dsc->dsc_c_version < DSC_VCURRENT)
	{   
	    if	(cnf->cnf_dsc->dsc_c_version < DSC_V3
	      && (cnf->cnf_dsc->dsc_status & DSC_IIREL_CONV_DONE) == 0)
	    {	
		SETDBERR(dberr, 0, E_DM93AD_IIREL_NOT_CONVERTED);
		break;
	    }
	    if	(cnf->cnf_dsc->dsc_c_version < DSC_V7)
	    {
		if ((cnf->cnf_dsc->dsc_status & DSC_IIATT_V7_CONV_DONE) == 0)
		{	
		    SETDBERR(dberr, 0, E_DM93F1_IIATT_NOT_CONVERTED);
		    break;
		}

		if ((cnf->cnf_dsc->dsc_status & 
					DSC_65_SMS_DBMS_CATS_CONV_DONE) == 0)
		{	
		    SMS_converted_flag = FALSE;
		}
	    }
#ifdef NOT_UNTIL_CLUSTERED_INDEXES
	    if	(cnf->cnf_dsc->dsc_c_version < DSC_V9)
	    {
		if ((cnf->cnf_dsc->dsc_status & DSC_IIIND_V9_CONV_DONE) == 0)
		{	
		    /* ***temp Need new error code */
		    SETDBERR(dberr, 0, E_DM93F1_IIATT_NOT_CONVERTED);
		    break;
		}
		if ((cnf->cnf_dsc->dsc_status & DSC_IIATT_V9_CONV_DONE) == 0)
		{	
		    SETDBERR(dberr, 0, E_DM93F1_IIATT_NOT_CONVERTED);
		    break;
		}

		if ((cnf->cnf_dsc->dsc_status & 
					DSC_65_SMS_DBMS_CATS_CONV_DONE) == 0)
		{	
		    SMS_converted_flag = FALSE;
		}
	    }
#endif /* NOT_UNTIL_CLUSTERED_INDEXES */
	}

	/*
	** Check the database consistency.  Don't do this check if following:
	**
	** 	- This is a readonly database.
	**	- This is rollforward.  We want rollforward to be able
	**	  to open an inconsistent database in order to restore it.
	**	- If the caller has specifically requested to bypass this
	**	  check with the DM2D_NOCK flag.
	*/
	if (cnf->cnf_dsc->dsc_access_mode == DSC_READ)
	{
    	    /* make config file look healthy */
    	    cnf -> cnf_dsc -> dsc_status |= DSC_VALID;
    	    cnf -> cnf_dsc -> dsc_open_count = 0;
	    /* set read access for readonly database */
	    dcb->dcb_access_mode = DCB_A_READ;
	}
	else if (!(flag & DM2D_FORCE) && !(flag & DM2D_RFP) &&
			!(cnf->cnf_dsc->dsc_status & DSC_VALID))
	{
	    access_mode = DCB_A_READ;
	    dcb->dcb_access_mode = DCB_A_READ;
	    flag |= DM2D_NOCK;

            /* Let the user know they have been given read only access to
            ** an inconsistent DB
            */
            ule_format(E_DM0027_DB_INCONSISTENT_RO, (CL_SYS_ERR *)0,
                       ULE_LOOKUP, NULL, buf, sizeof(buf), &len, &msg_error,
                       1, sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name);

            scf_cb.scf_length = sizeof(scf_cb);
            scf_cb.scf_type = SCF_CB_TYPE;
            scf_cb.scf_facility = DB_DMF_ID;
            scf_cb.scf_len_union.scf_blength = len;
            scf_cb.scf_ptr_union.scf_buffer = buf;
            scf_cb.scf_session = DB_NOSESSION;

            (void) scf_call( SCC_MESSAGE, &scf_cb );

	}
	else if ((flag & (DM2D_RFP | DM2D_NOCK)) == 0)
	{
	    /*  Check if the database is valid. */

	    if (!(cnf->cnf_dsc->dsc_status & DSC_VALID))
	    {
	        if (flag & DM2D_FORCE)
		{
		    dm0c_mk_consistnt( cnf ); /* force db consistent -- teg */
		    status = dm0c_close(cnf,
				    DM0C_UPDATE | DM0C_LEAVE_OPEN | DM0C_COPY,
				    dberr);
		}
		else
		{
		    SETDBERR(dberr, 0, E_DM0100_DB_INCONSISTENT);
		    break;
		}
	    }

	    /*
	    ** If this is the first openner (determined by the value of the
	    ** open db lock being returned as zero) and the config file open
	    ** count is greater than zero, this database must have been open
	    ** when recovery could not read log file to determine if recovery
	    ** needed, so it must be considered inconsistent.
	    **
	    ** We can skip this check if:
	    **
	    **    - If this is the recovery process.
	    **	  - If we are running with the DM2D_NLK option - in which
	    **	    case we have not aquired the open db lock and
	    **	    the value is not valid.
	    */
	    if (((flag & DM2D_RECOVER) == 0) && ((flag & DM2D_NLK) == 0) &&
		((odb_k_value.lk_low) == 0) && ((odb_k_value.lk_high) == 0) &&
		(cnf->cnf_dsc->dsc_open_count != 0) && 
		(status != LK_VAL_NOTVALID)
	       )
	    {
		/* Reset open count, mark database as inconsistent, 
		** log error, and return error. 
		*/
		cnf->cnf_dsc->dsc_open_count = 0;
		cnf->cnf_dsc->dsc_status &= ~DSC_VALID;
		cnf->cnf_dsc->dsc_inconst_code = DSC_DM2D_ALREADY_OPEN;
		cnf->cnf_dsc->dsc_inconst_time = TMsecs();
		status = dm0c_close(cnf, DM0C_UPDATE|DM0C_LEAVE_OPEN, dberr);
		uleFormat(NULL, E_DM9327_BAD_DB_OPEN_COUNT, 0, ULE_LOG, 
                                (DB_SQLSTATE *)NULL, NULL, 0, 
				NULL, &local_error, 0);
		SETDBERR(dberr, 0, E_DM0100_DB_INCONSISTENT);
		break;
	    }

	    /*
	    ** If this is the first opener of the database and the NOLOGGING
	    ** bit is turned on, then we must have crashed at some point
	    ** while this database was running in Set Nologging mode.
	    ** No recovery was done, since nothing was logged.  We have no
	    ** choice bug to label the database inconsistent.
	    */
	    if (((flag & DM2D_RECOVER) == 0) && ((flag & DM2D_NLK) == 0) &&
		((odb_k_value.lk_low) == 0) && ((odb_k_value.lk_high) == 0) &&
		(cnf->cnf_dsc->dsc_status & DSC_NOLOGGING) &&
		((cnf->cnf_dsc->dsc_access_mode & DSC_READ) == 0)
	       )
	    {
		cnf->cnf_dsc->dsc_status &= ~(DSC_VALID | DSC_NOLOGGING);
		cnf->cnf_dsc->dsc_inconst_code = DSC_OPN_NOLOGGING;
		cnf->cnf_dsc->dsc_inconst_time = TMsecs();
		status = dm0c_close(cnf, DM0C_UPDATE|DM0C_LEAVE_OPEN, dberr);
		uleFormat(NULL, E_DM9327_BAD_DB_OPEN_COUNT, (CL_ERR_DESC *)NULL, 
		    ULE_LOG, (DB_SQLSTATE *) NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(dberr, 0, E_DM0100_DB_INCONSISTENT);
		break;
	    }
	}	

	/*	Set the unique identifier for the database. */

	dcb->dcb_id = cnf->cnf_dsc->dsc_dbid;

	if ((flag & DM2D_NLK) == 0)
	{
	    /*
	    ** Now convert the database open lock back 
            ** to null after incrementing
	    ** the value.
	    */
	    odb_k_value.lk_low++;
	    cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT,
		svcb->svcb_lock_list, 0, LK_N,
		(LK_VALUE * )&odb_k_value, (LK_LKID *)dcb->dcb_odb_lock_id,
		(i4)0, &sys_err); 
	    if (cl_status)
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	}
    
	/*
	** Tell the Buffer Manager that we are opening this database
	** that it can verify the pages it has cached for this DB.  This
	** is necessary since this server has not had a lock on this db
	** and all the pages cached for it may have been invalidated by
	** another server.
	**
	** Note that we make this call even before we have finished initializing
	** the DCB fields.  This is OK since there is nothing in the buffer
	** manager which will require information on this DCB, since there can
	** be no modified pages for this database (since the database is not
	** open by anybody).
	**
	** This call also signals to other buffer manager's that we have
	** opened this db in this process.  For this reason, we must make
	** this call even if this is a DM2D_JLK call.
	*/
	write_mode = (dcb->dcb_access_mode == DCB_A_WRITE);
	status = dm0p_dbcache(dcb->dcb_id, 1, write_mode, lock_list, dberr);

	/* Returns E_DB_WARN if the cache was tossed */
	if ( status == E_DB_WARN )
	{
	    /* The DMF cache was tossed, toss other stuff */

	    /*
	    ** When a DB is closed, its sequences are moved from
	    ** the DCB to the server-wide svcb list.
	    ** To completely purge those sequences, first remove
	    ** any that might be left hanging around on the DCB,
	    ** then simulate what happens when a DB is destroyed 
	    ** by purging any sequences for this DB from the
	    ** Server cache.
	    */
	    if ( dcb->dcb_seq )
		dms_close_db(dcb);
	    dms_destroy_db(dcb->dcb_id);

	    /* and toss any user TCBs */
	    (void) dm2d_release_user_tcbs(dcb, lock_list, 0, DM2T_TOSS, dberr);

	    /*
	    ** ...and here's where we need to purge the DB's RDF cache
	    ** stuff, if only we knew how...
	    */
	}

	if ( DB_FAILURE_MACRO(status) )
	    break;

	/*
	** If we are opening the database exclusively, then toss all pages
	** stored in the buffer manager, as this may be a CREATEDB following
	** a destroy of the database (database could have been destroyed by
	** directly deleting the files) or this could be a restore from
	** checkpoint program.
	**
	** ...but don't toss again if dm0p_dbcache just did that
	**    for us.
	*/
	if ( dcb->dcb_status & DCB_S_EXCLUSIVE && status != E_DB_WARN )
	    dm0p_toss_pages(dcb->dcb_id, (i4)0, lock_list, (i4)0, 0);

	/*
	** Return if just locking the database.
	*/
	if (flag & DM2D_JLK)
	{
	    if (lock_mode == DM2D_X)
		dcb->dcb_status |= DCB_S_EXCLUSIVE;
	    dcb->dcb_opn_count--;
	    /*  Close the configuration file. */

	    status = dm0c_close(cnf, 0, dberr);
	    if (status != E_DB_OK)
		break;
	    config = 0;

            dcb->dcb_status &= ~(DCB_S_MLOCKED);
	    dm0s_munlock(&dcb->dcb_mutex);
	    return (E_DB_OK);
	}

	status = dm0m_allocate((i4)(sizeof(DMP_EXT) +
	    cnf->cnf_dsc->dsc_ext_count * sizeof(DMP_LOC_ENTRY)),
	    0, (i4)EXT_CB,	(i4)EXT_ASCII_ID, (char *)dcb,
	    (DM_OBJECT **)&dcb->dcb_ext, dberr);
	if (status != E_DB_OK)
	    break;

	/*  Remember journaling status
	**  BUG 34069: set status if not journaled
	*/
	if (cnf->cnf_dsc->dsc_status & DSC_JOURNAL)
	    dcb->dcb_status |= DCB_S_JOURNAL;
	else
	    dcb->dcb_status &= ~DCB_S_JOURNAL;

	/*  Check for low level locking needed. */

	dcb->dcb_status &= ~(DCB_S_EXCLUSIVE | DCB_S_ROLLFORWARD | 
				DCB_S_RECOVER | DCB_S_ONLINE_RCP);

	if (flag & DM2D_RFP)
	    dcb->dcb_status |= DCB_S_ROLLFORWARD;
	if (flag & DM2D_ONLINE_RCP)
	    dcb->dcb_status |= DCB_S_ONLINE_RCP;
	if (flag & DM2D_RECOVER)
	    dcb->dcb_status |= DCB_S_RECOVER;

	if (lock_mode == DM2D_S || lock_mode == DM2D_X)
	    dcb->dcb_status |= DCB_S_EXCLUSIVE;

	/*
	** Allow MVCC protocols when appropriate.
	**
	** DB must not be iidbdb, not locked exclusively, 
	** opened for write, logged, fast commit, not DMCM
	** and not in use by recovery of any sort.
	**
	** If DB has been alterdb'd to disable MVCC,
	** then don't use it.
	*/
	if ( cnf->cnf_dsc->dsc_dbid != IIDBDB_ID &&
	     dcb->dcb_access_mode != DCB_A_READ &&
	     dcb->dcb_status & DCB_S_FASTCOMMIT &&
	     !(flag & DM2D_NLG) &&
	     !(dcb->dcb_status & (DCB_S_ROLLFORWARD |
	     			  DCB_S_ONLINE_RCP |
				  DCB_S_RECOVER |
				  DCB_S_DMCM |
				  DCB_S_EXCLUSIVE)) )
	{
	    /* Note if explicitly disabled by alterdb */
	    if ( cnf->cnf_dsc->dsc_status & DSC_NOMVCC )
	        dcb->dcb_status |= DCB_S_MVCC_DISABLED;
	    else
		/* Note this DB is eligible for MVCC protocols */
		dcb->dcb_status |= DCB_S_MVCC;
	}

	/*
	**  Set the production mode flags
	*/
	if (cnf->cnf_dsc->dsc_dbaccess & DU_PRODUCTION)
	      dcb->dcb_status |= DCB_S_PRODUCTION;
	else
	    dcb->dcb_status &= ~DCB_S_PRODUCTION;
	if (cnf->cnf_dsc->dsc_dbaccess & DU_NOBACKUP)
	      dcb->dcb_status |= DCB_S_NOBACKUP;
	else
	    dcb->dcb_status &= ~DCB_S_NOBACKUP;
        if ((cnf->cnf_dsc->dsc_dbaccess & DU_MUSTLOG) ||
            (flag & DM2D_MUST_LOG))
        {
            /* Database was created with MustLog option or is in this
            ** DBMS's MustLog DB List
            */
            dcb->dcb_status |= DCB_S_MUST_LOG;
        }
        else
        {
            dcb->dcb_status &= ~DCB_S_MUST_LOG;
        }

	dcb->dcb_collation = NULL;
	if (status = aducolinit(cnf->cnf_dsc->dsc_collation, dm2d_reqmem, 
				&tbl, &sys_err))
	{
	    SETDBERR(dberr, 0, E_DM00A0_UNKNOWN_COLLATION);
	    status = E_DB_ERROR;
	    break;
	}
	dcb->dcb_collation = (PTR)tbl;
	MEcopy(cnf->cnf_dsc->dsc_collation, sizeof(dcb->dcb_colname),
	       dcb->dcb_colname);

	/* Fix me 
	** Unicode collation is loaded Only if Unicode types allowed.
	** Here we are Allowing collation table to be loaded for iidbdb
	** if II_CHARSET is set to UTF8. This should be evnentually done
	** at createdb, when support is added for iidbdb to be created 
	** unicode enabled if II_CHARSETxx was set to UTF8. 
	*/
	if (( cnf->cnf_dsc->dsc_dbservice & DU_UTYPES_ALLOWED ) || 
	    ((dcb->dcb_id == 1) && (CMischarset_utf8())))
	{
	    if (status = aduucolinit(cnf->cnf_dsc->dsc_ucollation, 
				 dm2d_reqmem, &utbl, &uvtbl, &sys_err))
	    {
		SETDBERR(dberr, 0, E_DM00A0_UNKNOWN_COLLATION);
		status = E_DB_ERROR;
		break;
	    }
	    dcb->dcb_ucollation = (PTR)utbl;
	    dcb->dcb_uvcollation = (PTR)uvtbl;
	}
	else
	{
	    dcb->dcb_ucollation = NULL;
	    dcb->dcb_uvcollation = NULL;
	}
	MEcopy(cnf->cnf_dsc->dsc_ucollation, sizeof(dcb->dcb_ucolname),
	       dcb->dcb_ucolname);
	/*
	** Normal database opens generate synchronous writes, but callers
	** may request an open without file synchronization.  This is
	** provides a performance improvement but callers must be
	** assured that recovery works properly.  Thus far rollforward
	** is the only user of this feature, and it assumes in the event of 
	** a failure the entire recovery is retried from from the beginning.
	*/
	dcb->dcb_sync_flag = cnf->cnf_dsc->dsc_sync_flag;

	/* NO DON'T DO THIS.  The idea has merit in some situations,
	** such as createdb, but some very crucial things (sysmod,
	** offline recovery) open a db in X mode and those absolutely
	** must not operate NOSYNC.  NOSYNC means that we are entirely
	** at the mercy of the OS to flush stuff to disk, and if it
	** doesn't before a crash, the database is likely toast.
	** FIXME need a more accurate way to apply nosync when it's safe.
	**if (lock_mode == DM2D_X)
	**    dcb->dcb_sync_flag = DCB_NOSYNC_MASK;
	*/

	if (flag & DM2D_SYNC_CLOSE)
	    dcb->dcb_status |= DCB_S_SYNC_CLOSE;

	dcb->dcb_ext->ext_count = cnf->cnf_dsc->dsc_ext_count;
	for (i = 0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	{
	    STRUCT_ASSIGN_MACRO(cnf->cnf_ext[i].ext_location,
	    		    dcb->dcb_ext->ext_entry[i]);
	    if (cnf->cnf_ext[i].ext_location.flags & DCB_JOURNAL)
	    	dcb->dcb_jnl = &dcb->dcb_ext->ext_entry[i];
	    else if (cnf->cnf_ext[i].ext_location.flags & DCB_DUMP)
	    	dcb->dcb_dmp = &dcb->dcb_ext->ext_entry[i];
	    else if (cnf->cnf_ext[i].ext_location.flags & DCB_CHECKPOINT)
	    	dcb->dcb_ckp = &dcb->dcb_ext->ext_entry[i];
	    else if (cnf->cnf_ext[i].ext_location.flags & DCB_ROOT)
	    	dcb->dcb_root = &dcb->dcb_ext->ext_entry[i];
	    if (cnf->cnf_ext[i].ext_location.flags & DCB_RAW)
		has_raw = TRUE;
	}

	/* Need jnl block size for online modify sidefile processing */
	dcb->dcb_jnl_block_size = cnf->cnf_jnl->jnl_bksz;

	/* Initialize TCB's for system tables. */

	/* Iirelation. */
	iirel.relpgsize = cnf->cnf_dsc->dsc_iirel_relpgsize;
	iirel_idx.relpgsize = cnf->cnf_dsc->dsc_iirel_relpgsize;
	iiatt.relpgsize = cnf->cnf_dsc->dsc_iiatt_relpgsize;
	iiindex.relpgsize = cnf->cnf_dsc->dsc_iiind_relpgsize;

	iirel.relpgtype = cnf->cnf_dsc->dsc_iirel_relpgtype;
	iirel_idx.relpgtype = cnf->cnf_dsc->dsc_iirel_relpgtype;
	iiatt.relpgtype = cnf->cnf_dsc->dsc_iiatt_relpgtype;
	iiindex.relpgtype = cnf->cnf_dsc->dsc_iiind_relpgtype;

	status = E_DB_OK;
	if (!dm0p_has_buffers(cnf->cnf_dsc->dsc_iirel_relpgsize))
	{
	    status = dm0p_alloc_pgarray(cnf->cnf_dsc->dsc_iirel_relpgsize, 
			dberr);
	}		    
	if (DB_SUCCESS_MACRO(status) 
		&& !dm0p_has_buffers(cnf->cnf_dsc->dsc_iiatt_relpgsize))
	{
	    status = dm0p_alloc_pgarray(cnf->cnf_dsc->dsc_iiatt_relpgsize, 
			dberr);
	}		    
	if (DB_SUCCESS_MACRO(status) 
		&& !dm0p_has_buffers(cnf->cnf_dsc->dsc_iiind_relpgsize))
	{
	    status = dm0p_alloc_pgarray(cnf->cnf_dsc->dsc_iiind_relpgsize, 
			dberr);
	}

	if (status != E_DB_OK)
	{
	    SETDBERR(dberr, 0, E_DM0157_NO_BMCACHE_BUFFERS);
	    status = E_DB_ERROR;
	}

	if (DB_SUCCESS_MACRO(status))
	{
	    status = construct_tcb(
		lock_list, 
		&iirel,
		DM_core_attributes, 
		dcb, 
		&rel_tcb, 
		dberr);
	}

	if (DB_SUCCESS_MACRO(status))
	{
	    /* 
	    ** Copy relprim (# of hash buckets) into the TCB so that we can 
	    ** open and fetch records from iirelation.
	    */

	    rel_tcb->tcb_rel.relprim = cnf->cnf_dsc->dsc_iirel_relprim;
	    rel_tcb->tcb_rel.relpgsize = cnf->cnf_dsc->dsc_iirel_relpgsize;
	    rel_tcb->tcb_rel.relpgtype = cnf->cnf_dsc->dsc_iirel_relpgtype;

	    /* Iirel_idx. */

	    /* 
	    ** The attribute records for iirel_idx start after the attribute
	    ** records for iirelation.
	    */

	    status = construct_tcb(
		lock_list, 
		&iirel_idx,
		(DM_core_attributes + iirel.relatts),
		dcb, 
		&relx_tcb, 
		dberr);
	}

	/* Iiattribute. */

	if (DB_SUCCESS_MACRO(status))
	{
	    relx_tcb->tcb_rel.relpgsize = cnf->cnf_dsc->dsc_iirel_relpgsize;
	    relx_tcb->tcb_rel.relpgtype = cnf->cnf_dsc->dsc_iirel_relpgtype;

	    /* 
	    ** The attribute records for iiattribute start after the attribute
	    ** records for iirel_idx.
	    */

	    status = construct_tcb(
		lock_list, 
		&iiatt,
		(DM_core_attributes + iirel.relatts + iirel_idx.relatts),
		dcb, 
		&att_tcb, 
		dberr);
	}

	/* Iiindex. */

	if (DB_SUCCESS_MACRO(status))
	{
	    att_tcb->tcb_rel.relpgsize = cnf->cnf_dsc->dsc_iiatt_relpgsize;
	    att_tcb->tcb_rel.relpgtype = cnf->cnf_dsc->dsc_iiatt_relpgtype;

	    /* 
	    ** The attribute records for iiindex start after the attribute
	    ** records for iiattributes.
	    */

	    status = construct_tcb(
		lock_list, 
		&iiindex,
		(DM_core_attributes + iirel.relatts + iirel_idx.relatts + 
		    iiatt.relatts),
		dcb, 
		&idx_tcb, 
		dberr);
	}

	if (DB_FAILURE_MACRO(status))
	    break;


	idx_tcb->tcb_rel.relpgsize = cnf->cnf_dsc->dsc_iiind_relpgsize;
	idx_tcb->tcb_rel.relpgtype = cnf->cnf_dsc->dsc_iiind_relpgtype;

	/*  Set handy pointers. */

	dcb->dcb_rel_tcb_ptr = rel_tcb;
	dcb->dcb_att_tcb_ptr = att_tcb;
	dcb->dcb_idx_tcb_ptr = idx_tcb;
	dcb->dcb_relidx_tcb_ptr = relx_tcb;

	/*
	** If eligible for MVCC protocols and journaled, 
	** init journal info in LG_JFIB.
	*/
	if ( dcb->dcb_status & DCB_S_MVCC && dcb->dcb_status & DCB_S_JOURNAL)
	{
	    /*
	    ** Extract the current journal file sequence, block number,
	    ** ckp_seq, block size, file size for consistent read
	    ** undo from the journals.
	    **
	    ** If this is a cluster installation, get this information 
	    ** out of the cluster node info array - as the block 
	    ** number and jnl address will be different for each node.  
	    */
	    if ( CXcluster_enabled() )
	    {
		i = CXnode_number(NULL);

		jfib.jfib_jfa.jfa_filseq = cnf->cnf_jnl->jnl_node_info[i].cnode_fil_seq;
		jfib.jfib_jfa.jfa_block = cnf->cnf_jnl->jnl_node_info[i].cnode_blk_seq;
	    }
	    else
	    {
		jfib.jfib_jfa.jfa_filseq = cnf->cnf_jnl->jnl_fil_seq;
		jfib.jfib_jfa.jfa_block = cnf->cnf_jnl->jnl_blk_seq;
	    }

	    jfib.jfib_ckpseq = cnf->cnf_jnl->jnl_ckp_seq; 
	    jfib.jfib_bksz = cnf->cnf_jnl->jnl_bksz;
	    jfib.jfib_maxcnt = cnf->cnf_jnl->jnl_maxcnt;
	}

	/* Close the configuration file. */
	
	status = dm0c_close(cnf, 0, dberr);

	if (status != E_DB_OK)
	    break;

	config = 0;

	/*  Queue relx_tcb to rel_tcb. */

	relx_tcb->tcb_q_next = (DMP_TCB*)&rel_tcb->tcb_iq_next;
	relx_tcb->tcb_q_prev = (DMP_TCB*)&rel_tcb->tcb_iq_next;
	rel_tcb->tcb_iq_next = relx_tcb;
	rel_tcb->tcb_iq_prev = relx_tcb;
	relx_tcb->tcb_parent_tcb_ptr = rel_tcb;

	/*  Insert TCB's onto the hash queue. */

	h = &hcb->hcb_hash_array[
	  HCB_HASH_MACRO(dcb->dcb_id,rel_tcb->tcb_rel.reltid.db_tab_base,0)];
	hash_mutex = &hcb->hcb_mutex_array[
	  HCB_MUTEX_HASH_MACRO(dcb->dcb_id, rel_tcb->tcb_rel.reltid.db_tab_base)];
	dm0s_mlock(hash_mutex);
	rel_tcb->tcb_q_next = h->hash_q_next;
	rel_tcb->tcb_q_prev = (DMP_TCB*)&h->hash_q_next;
	h->hash_q_next->tcb_q_prev = rel_tcb;
	h->hash_q_next = rel_tcb;
	rel_tcb->tcb_hash_bucket_ptr = h;
	rel_tcb->tcb_hash_mutex_ptr = hash_mutex;
	dm0s_munlock(hash_mutex);

	h = &hcb->hcb_hash_array[
	  HCB_HASH_MACRO(dcb->dcb_id,att_tcb->tcb_rel.reltid.db_tab_base,0)];
	hash_mutex = &hcb->hcb_mutex_array[
	  HCB_MUTEX_HASH_MACRO(dcb->dcb_id, att_tcb->tcb_rel.reltid.db_tab_base)];
	dm0s_mlock(hash_mutex);
	att_tcb->tcb_q_next = h->hash_q_next;
	att_tcb->tcb_q_prev = (DMP_TCB*)&h->hash_q_next;
	h->hash_q_next->tcb_q_prev = att_tcb;
	h->hash_q_next = att_tcb;
	att_tcb->tcb_hash_bucket_ptr = h;
	att_tcb->tcb_hash_mutex_ptr = hash_mutex;
	dm0s_munlock(hash_mutex);

	h = &hcb->hcb_hash_array[
	  HCB_HASH_MACRO(dcb->dcb_id,idx_tcb->tcb_rel.reltid.db_tab_base,0)];
	hash_mutex = &hcb->hcb_mutex_array[
	  HCB_MUTEX_HASH_MACRO(dcb->dcb_id, idx_tcb->tcb_rel.reltid.db_tab_base)];
	dm0s_mlock(hash_mutex);
	idx_tcb->tcb_q_next = h->hash_q_next;
	idx_tcb->tcb_q_prev = (DMP_TCB*)&h->hash_q_next;
	h->hash_q_next->tcb_q_prev = idx_tcb;
	h->hash_q_next = idx_tcb;
	idx_tcb->tcb_hash_bucket_ptr = h;
	idx_tcb->tcb_hash_mutex_ptr = hash_mutex;
	dm0s_munlock(hash_mutex);

	/* Increment number of active TCBs by the 3 just created */
	dm0s_mlock(&hcb->hcb_tq_mutex);
	hcb->hcb_tcbcount += 3;
	hcb->hcb_cache_tcbcount[rel_tcb->tcb_table_io.tbio_cache_ix]++;
	hcb->hcb_cache_tcbcount[att_tcb->tcb_table_io.tbio_cache_ix]++;
	hcb->hcb_cache_tcbcount[idx_tcb->tcb_table_io.tbio_cache_ix]++;
	/* Include relx tcb in cache count */
	hcb->hcb_cache_tcbcount[relx_tcb->tcb_table_io.tbio_cache_ix]++;
	dm0s_munlock(&hcb->hcb_tq_mutex);

	/*  Check access mode. */

	if (access_mode > dcb->dcb_access_mode)
	{
	    SETDBERR(dberr, 0, E_DM003E_DB_ACCESS_CONFLICT);
	    break;
	}

    	/*  
	** Log open of the database by the server, unless 'no logging' is 
	** specified. 
	*/
	if (!(flag & DM2D_NLG))
	{
	    open_flag = 0;
	    if (dcb->dcb_status & DCB_S_JOURNAL)
		open_flag |= DM0L_JOURNAL;
            if ((dcb->dcb_status & DCB_S_FASTCOMMIT) ||
                (dcb->dcb_status & DCB_S_DMCM))
                open_flag |= DM0L_FASTCOMMIT;
	    if (flag & DM2D_NOCK)
		open_flag |= DM0L_PRETEND_CONSISTENT;
	    if (flag & DM2D_NOLOGGING)
		open_flag |= DM0L_NOLOGGING;
	    if (dcb->dcb_access_mode == DCB_A_READ)
	    	open_flag |= (DM0L_READONLY_DB | DM0L_NOLOGGING);
	    if ( dcb->dcb_status & DCB_S_MVCC )
	        open_flag |= DM0L_MVCC;
	    status = dm0l_opendb(svcb->svcb_lctx_ptr->lctx_lgid,
		open_flag,
                &dcb->dcb_name, 
		&dcb->dcb_db_owner, 
		dcb->dcb_id, 
                &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length, 
		&dcb->dcb_log_id,
		(LG_LSN *)NULL,
		dberr);
	    if (status != E_DB_OK)
		break;

	    if ( dcb->dcb_status & DCB_S_MVCC && dcb->dcb_status & DCB_S_JOURNAL )
	    {
		/* With log id now in hand ... */
	        jfib.jfib_db_id = dcb->dcb_log_id;

		/* Init jnl stuff from config into DB's log context */
		cl_status = LGalter(LG_A_JFIB, (PTR)&jfib, sizeof(jfib),
					&sys_err);
		if ( cl_status != OK )
		{
		    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
				NULL, 0L, NULL, err_code, 0);
		    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG,
				NULL, NULL, 0, NULL, err_code, 
				1, 0, LG_A_JFIB);
		    SETDBERR(dberr, 0, E_DM9268_OPEN_DB);
		    break;
		}
	    }

	}

	/*
	** Fetch the iirelation tuples for these catalogs and insert them
	** into the TCB.
	*/

	if (!sm_inconsistent)
	{
	    status = update_tcb(rel_tcb, lock_list, flag, dberr);
	    if (status != E_DB_OK)
		break;

	    status = update_tcb(relx_tcb, lock_list, flag, dberr);
	    if (status != E_DB_OK)
		break;

	    status = update_tcb(att_tcb, lock_list, flag, dberr);
	    if (status != E_DB_OK)
		break;

	    status = update_tcb(idx_tcb, lock_list, flag, dberr);
	    if (status != E_DB_OK)
		break;

	    /*
	    ** Fetch the iiattribute tuples for these catalogs and
	    ** use them to update pertinent information chained on the TCB.
	    */

	    status = update_tcbatts(rel_tcb, lock_list, flag, dberr);
	    if (status != E_DB_OK)
		break;

	    status = update_tcbatts(relx_tcb, lock_list, flag, dberr);
	    if (status != E_DB_OK)
		break;

	    status = update_tcbatts(att_tcb, lock_list, flag, dberr);
	    if (status != E_DB_OK)
		break;

	    status = update_tcbatts(idx_tcb, lock_list, flag, dberr);
	    if (status != E_DB_OK)
		break;
	}

	if ( (flag & DM2D_CVCFG) &&
	    rel_tcb->tcb_rel.relfhdr == DM_TBL_INVALID_FHDR_PAGENO ||
	    relx_tcb->tcb_rel.relfhdr == DM_TBL_INVALID_FHDR_PAGENO ||
	    att_tcb->tcb_rel.relfhdr == DM_TBL_INVALID_FHDR_PAGENO ||
	    idx_tcb->tcb_rel.relfhdr == DM_TBL_INVALID_FHDR_PAGENO)
	{
	    TRdisplay("Need to rebuild SMS for core catalog %d %d %d %d\n",
		rel_tcb->tcb_rel.relfhdr, relx_tcb->tcb_rel.relfhdr,
		att_tcb->tcb_rel.relfhdr, idx_tcb->tcb_rel.relfhdr);
	    SMS_converted_flag = FALSE;
	}

	/* 
	** If this is UPGRADEDB and tables have not been converted
	** to new space management scheme then convert them
	** Note ANY upgrade that rewrites a core catalog as a 1-bucket hash
	** better unset 
	** config->cnf_dsc->dsc_status &= ~DSC_65_SMS_DBMS_CATS_CONV_DONE;
	** so we end up here
	*/
	if ( flag & DM2D_CVCFG && SMS_converted_flag == FALSE )
	{
	    /* 
	    ** Open the config file to lock anyone out whilst we are
	    ** converting, also we need to update it to indicate
	    ** conversion has completed.
	    */
	    status = dm0c_open(dcb, cnf_flag, lock_list, &cnf, dberr);

	    if ( status != E_DB_OK )
		break;

	    config = cnf;

	    /*
	    ** All upgrades that rewrite a core catalog as a 1-bucket hash
	    ** needs to fix up FHDR/FMAP here
	    */
	    status = dm2d_convert_core_cats_to_65(
			    rel_tcb, relx_tcb,
			    att_tcb, idx_tcb,
			    lock_list, dberr );
	    if ( status != E_DB_OK )
		break;
	
	    if (cnf->cnf_dsc->dsc_c_version < DSC_V3)
	    {
		/* 
		** Convert the the other DBMS catalogs 
		*/
		status = dm2d_convert_DBMS_cats_to_65( 
				dcb, lock_list, dberr );
		if ( status != E_DB_OK )
		       break;
	    }

	    /* 
	    ** Conversion of the DBMS catalogs done ok so mark
	    ** in config file so that we do not do again.
	    ** If we had to do SMS stuff we must have been coming from
	    ** pre-V7, and the converters take us right up to core
	    ** version V8.
	    */
	    cnf->cnf_dsc->dsc_status |= DSC_65_SMS_DBMS_CATS_CONV_DONE;
	    cnf->cnf_dsc->dsc_c_version = DSC_VCURRENT;

	    /* 
	    ** Close the configuration file. 
	    */
	    status = dm0c_close(cnf, DM0C_UPDATE, dberr);

	    if (status != E_DB_OK)
	         break;

	    config = 0;

	    SMS_converted_flag = TRUE;

	}


	/*
	** make sure database hase been converted to correct level 
	*/
	if ( SMS_converted_flag == FALSE )
	{
	    SETDBERR(dberr, 0, E_DM92CA_SMS_CONV_NEEDED);
	    status = E_DB_ERROR;
	    break;
	}
	/*
	** set replicate flag in DCB, we do this if the table
	** dd_regist_tables exists, i.e. it has an iirelation tuple
	*/

	/* only replicate for ordinary DBMS's (not for RCP, ACP and CSP) */
	/* Also only replicate if II_DBMS_REP is not set */
	/* ...and not if iidbdb, I'm pretty sure */
	if ((flag & DM2D_ADMIN_CMD) == 0 &&
		(svcb->svcb_status & (SVCB_RCP | SVCB_ACP | 
	    SVCB_CSP | SVCB_SINGLEUSER | SVCB_NO_REP)) == 0
		&& dcb->dcb_id != 1 )
	{
	    tran_id.db_high_tran = 0;
    	    tran_id.db_low_tran = 0;
	    /*
	    ** FIX_ME: doing a position using table name and table owner on the
	    ** iirel_idx index places the cusror on the first page in the table
	    ** because the hashing algorithmn fails due to the fact that 
	    ** tcb->tcb_rel.relprim = 1 when it should be alot more (something
	    ** like 16), this results in NONEXT and the record is not retrieved
	    ** somehow the relation record in the TCB is corrupted, need to
	    ** investigate further, for the time being we'll comment out the 
	    ** code and do a full table scan on iirelation instead.
	    **
	    ** Since we're doing a full table scan we'll get all the other 
	    ** rows we need too (maybe this is a more effecient way of doing 
	    ** it anyway).
	    **
	    ** Well, if relprim is 1, then that's a bug, but doing a 
	    ** full table scan is terrible, especially in a database
	    ** that may have thousands of iirelation rows for 
	    ** partitions.
	    **
	    ** Changed this to use the index and limit the probes
	    ** to the range of "dd_?" table names; if none are found,
	    ** then the DB is not replicated and it cost minimal
	    ** I/O to find out.
	    */

	    /*
	    ** Open RCB for iirelation index
	    **
	    */
	    status = dm2r_rcb_allocate(dcb->dcb_relidx_tcb_ptr,
			    0, &tran_id, lock_list, 0, 
			    &idx_rcb, dberr);
	    if (status != E_DB_OK)
	        break;
	    
	    /*
	    ** Indicate that access to relidx through this RCB will be
	    ** read-only, so we don't try to fix pages for WRITE.
	    */
	    idx_rcb->rcb_lk_mode = RCB_K_IS;
	    idx_rcb->rcb_k_mode = RCB_K_IS;
	    idx_rcb->rcb_access_mode = RCB_A_READ;
	
	    dcb->rep_regist_tab.db_tab_base = 0;
	    dcb->rep_regist_col.db_tab_base = 0;
	    dcb->rep_regist_tab_idx.db_tab_base = 0;
	    dcb->rep_input_q.db_tab_base = 0;
	    dcb->rep_dd_paths.db_tab_base = 0;
	    rep_db.db_tab_base = 0;
	    rep_db_idx.db_tab_base = 0;
	    dcb->rep_dd_db_cdds.db_tab_base = 0;
	    dcb->rep_dist_q.db_tab_base = 0;

	    qual_list[0].attr_number = DM_1_RELIDX_KEY;
	    qual_list[0].attr_operator = DM2R_EQ;
	    qual_list[0].attr_value = (char*)rep_idx.relname.db_tab_name;
	    qual_list[1].attr_number = DM_2_RELIDX_KEY;
	    qual_list[1].attr_operator = DM2R_EQ;
	    qual_list[1].attr_value = (char*)dcb->dcb_db_owner.db_own_name;

	    /*
	    ** Find each of the tables needed for replication.
	    ** If any do not exist, we don't replicate.
	    */
	    
	    /* Assume we'll find all tables */
	    dcb->dcb_status |= DCB_S_REPLICATE;

	    for ( i = 0; i < NumRepTables; i++ )
	    {
		if ( dcb->dcb_dbservice & (DU_NAME_UPPER | DU_DELIM_UPPER) )
		    STmove(RepTables[i].RepTabNameU, ' ', DB_TAB_MAXNAME,
			   (PTR)rep_idx.relname.db_tab_name);
		else
		    STmove(RepTables[i].RepTabName, ' ', DB_TAB_MAXNAME,
			   (PTR)rep_idx.relname.db_tab_name);

		status = dm2r_position(idx_rcb, DM2R_QUAL, qual_list, 
			(i4)2, (DM_TID *)NULL, dberr);

		while ( status == E_DB_OK )
		{
		    status = dm2r_get(idx_rcb, &tid, DM2R_GETNEXT, 
					(char *)&rep_idx, dberr);

		    if ( status == E_DB_OK )
		    {
			/*
			** at this point we will also go get the reltid and reltidx
			** of dd_regist_tables, dd_reg_tbl_idx, dd_regist_columns, 
			** dd_paths, dd_db_cdds and dd_distrib_queue, this will save
			** us some time later. Also get the database 
			** no for replication here, and the tabid of the input queu.
			** NOTE: We depend on the fact that a database lock is required
			** to enable replication in a database, otherwhise we get
			** inconcistencies by setting this flag on db open
			** FIX_ME: may need to do case sensitive checks for SQL 92
			**
			** Before running through the other checks, we'll check for the
			** existance of the dd_transaction_id table, if this table
			** exists we'll assume the user is still running replicator
			** 1.1, and will not replicate in the DB
			*/

			/* Skip if different owner */
			if ( MEcmp(rep_idx.relowner.db_own_name, 
				dcb->dcb_db_owner.db_own_name, DB_OWN_MAXNAME))
			{
			    continue;
			}

			/* Found DD_TRANSACTION_ID? Pre 2.0... */
			if ( RepTables[i].RepTabId == IdDD_TRANSACTION_ID )
			    break;

			if ( rcb == NULL )
			{
			    /* Open an RCB for iirelation */
			    status = dm2r_rcb_allocate(dcb->dcb_rel_tcb_ptr,
					    0, &tran_id, lock_list, 0, 
					    &rcb, dberr);
			    if ( status )
				break;
			    /* Access iirelation for read */
			    rcb->rcb_lk_mode = RCB_K_IS;
			    rcb->rcb_k_mode = RCB_K_IS;
			    rcb->rcb_access_mode = RCB_A_READ;
			}

			/* Fetch the iirelation record by tid */
			status = dm2r_get(rcb, (DM_TID*)&rep_idx.tidp,
					    DM2R_BYTID, (char*)&rep_rel,
					    dberr);
			if ( status )
			    break;

			switch ( RepTables[i].RepTabId )
			{
			  case IdDD_REGIST_TABLES:
			    STRUCT_ASSIGN_MACRO(rep_rel.reltid,
						dcb->rep_regist_tab);
			    break;

			  case IdDD_REG_TBL_IDX:
			    STRUCT_ASSIGN_MACRO(rep_rel.reltid,
						dcb->rep_regist_tab_idx);
			    break;

			  case IdDD_REGIST_COLUMNS:
			    STRUCT_ASSIGN_MACRO(rep_rel.reltid,
						dcb->rep_regist_col);
			    break;

			  case IdDD_DATABASES:
			    STRUCT_ASSIGN_MACRO(rep_rel.reltid, rep_db);
			    break;

			  case IdDD_DB_NAME_IDX:
			    STRUCT_ASSIGN_MACRO(rep_rel.reltid, rep_db_idx);
			    break;

			  case IdDD_INPUT_QUEUE:
			    STRUCT_ASSIGN_MACRO(rep_rel.reltid,
						dcb->rep_input_q);
			    break;

			  case IdDD_PATHS:
			    STRUCT_ASSIGN_MACRO(rep_rel.reltid,
						dcb->rep_dd_paths);
			    break;

			  case IdDD_DB_CDDS:
			    STRUCT_ASSIGN_MACRO(rep_rel.reltid,
						dcb->rep_dd_db_cdds);
			    break;

			  case IdDD_DISTRIB_QUEUE:
			    STRUCT_ASSIGN_MACRO(rep_rel.reltid,
						dcb->rep_dist_q);
			    break;
			}
		    }
		    break;
		}
		if ( RepTables[i].RepTabId == IdDD_TRANSACTION_ID )
		{
		    /* If DD_TRANSACTION_ID not found, continue checks */
		    if ( dberr->err_code == E_DM0055_NONEXT )
			continue;
		    /* DD_TRANSACTION_ID was found, pre 2.0 rep */
		    SETDBERR(dberr, 0, E_DM0055_NONEXT);
		}

		if (dberr->err_code == E_DM0055_NONEXT) /* table does not exist
					     ** do not replicate */
		{
		    dcb->dcb_status &= ~DCB_S_REPLICATE;
		    status = E_DB_OK;
		    break;
		}
	    }
	    if (status != E_DB_OK)
	        break;
	    status = dm2r_release_rcb(&idx_rcb, dberr);
	    idx_rcb = NULL;
	    if (status != E_DB_OK)
	        break;
	    if ( rcb )
		status = dm2r_release_rcb(&rcb, dberr);
	    rcb = NULL; /* (inifa01) set rcb to NULL if successfully released, to prevent 
			   unnecessary dm2r_release_rcb() call during cleanup if
			   any failures occur after this point. */
	    if (status != E_DB_OK)
	        break;


	    /* If all replicator tables were found... */
	    if ( dcb->dcb_status & DCB_S_REPLICATE )
	    {
	        /*
	        ** get replication database no
	        */
                status = dm2t_open(dcb, &rep_db, DM2T_N, DM2T_UDEFERRED,
                    DM2T_A_READ, (i4)0, (i4)10, (i4)0,
                    dcb->dcb_log_id, lock_list, (i4)0,
                    DMC_C_READ_UNCOMMITTED, 0, &tran_id, &timestamp,
                    &repdb_rcb, (DML_SCB *)NULL, dberr);
	        if (status != E_DB_OK)
		    break;
	        repdb_tcb = repdb_rcb->rcb_tcb_ptr;
	        for(i = 1; i <= repdb_tcb->tcb_rel.relatts; i++)
	        {
		    if (STcompare(repdb_tcb->tcb_atts_ptr[i].attnmstr, 
		        "local_db") == 0 || 
		        STcompare(repdb_tcb->tcb_atts_ptr[i].attnmstr, 
		        "LOCAL_DB") == 0)
		    break;
	        }
	        if (i == repdb_tcb->tcb_rel.relatts)
	        {
		    /* local_db field not found */
        	    uleFormat(dberr, E_DM9550_NO_LOCAL_DB, (CL_ERR_DESC *)NULL, ULE_LOG,
            	        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	        &local_err, (i4)0);
		    status = E_DB_ERROR;
	        }
	        /*
	        ** we have to do a full table scan on dd_databases since we have
	        ** no useful keys (only db name), may be better to change
	        ** indexing strategy on this table so we can do a key lookup
	        */
	        status = dm2r_position(repdb_rcb, DM2R_ALL, 
		    (DM2R_KEY_DESC *)NULL, 1, NULL, dberr);
	        if (status != E_DB_OK)
		    break;
	        repdb_rec = MEreqmem(0, repdb_tcb->tcb_rel.relwid,
		    TRUE, &cl_status);
	        if (repdb_rec == NULL || cl_status != OK)
	        {
        	    uleFormat(dberr, E_DM9551_REP_DB_NOMEM, (CL_ERR_DESC *)NULL, ULE_LOG,
            	        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
            	        &local_err, (i4)0);
		    status = E_DB_ERROR;
		    break;
	        }
	        for(;;)
	        {
	    	    status = dm2r_get(repdb_rcb, &repdb_tid, DM2R_GETNEXT, 
		        repdb_rec, dberr);
	            if (status != E_DB_OK)
		    {
		        if (dberr->err_code == E_DM0055_NONEXT) /* no local db entry,
						     ** don't replicate */
		        {
			    status = E_DB_OK;
			    CLRDBERR(dberr);
			    break;
		        }
		        else
			    break;
		    }
		    MEcopy(repdb_rec + repdb_tcb->tcb_atts_ptr[i].offset,
				sizeof(i2), &is_local);
		    if (is_local)
		        break;
	        }
	        if (status != E_DB_OK)
		    break;
	        /*
	        ** consistency check. dbname is field 3 in the record
	        **
	        **     13-jan-97 (stephenb)
	        **		remove this check temporarily, it's causing problems
	        **		with some of the replicator admin tools
	        */
	        /*
	        ** if (is_local && MEcmp(repdb_rec + repdb_tcb->tcb_atts_ptr[3].
	        **	offset, dcb->dcb_name.db_db_name, DB_DB_MAXNAME))
	        ** {
                **	uleFormat(dberr, E_DM955B_REP_BAD_LOCAL_DB, (CL_ERR_DESC *)NULL,
                **	    ULE_LOG, (i4 *)NULL, (char *)NULL, (i4)0, 
		** 	    (i4 *)NULL, err_code, (i4)0);
	        **	status = E_DB_ERROR;
	        **	break;
	        ** }
	        */

		/* only replicate if we found the local db record */
	        if (is_local) 
	        {
		    MEcopy(repdb_rec, sizeof(dcb->dcb_rep_db_no),
				&dcb->dcb_rep_db_no);
	    	    dcb->dcb_status |= DCB_S_REPLICATE;
	        }
	        else /* catalogs exist but no local_db record */
	        {
		    dcb->dcb_rep_db_no = 0;
		    dcb->dcb_status &= ~DCB_S_REPLICATE;
		    dcb->dcb_status |= DCB_S_REPCATS;
	        }
	        /*
	        ** clean up
	        */
	        status = dm2t_close(repdb_rcb, 0, dberr);
	        if (status != E_DB_OK)
		    break;
		repdb_rcb = NULL; /* (inifa01) b104696 set to NULL if successfully closed to 
				     prevent unnecessary dm2t_close() call during
                         	     cleanup if any failures occur after this point. */ 
	    }
	}

	/*  Bump reference count. */
	
	dcb->dcb_ref_count++;
	dcb->dcb_opn_count--;
	db_opened = TRUE;    /*(inifa01) fix bug 104267. Mark that the db
                             is open incase queue cleanup fails, so open
                             reference count is bumped down during cleanup.*/
	if (has_raw)
	{
	    CSadjust_counter(&svcb->svcb_rawdb_count, 1);
	    dcb->dcb_status |= DCB_S_HAS_RAW;	/* Decrement at close */
	}

	/*
	** if we are replicating, then clean out the input queue, we'll
	** start with any entries in the transaction queue, and then
	** clear out the leftovers.
	*/
	if ((dcb->dcb_status & DCB_S_REPLICATE) && Dmc_rep)
	{
	    bool	have_mutex = FALSE;
	    bool	last;
	    i4		i, j;
	    DMC_REPQ	*repq = (DMC_REPQ *)(Dmc_rep + 1);

	    /* check transaction for records for this db */
	    CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
	    have_mutex = TRUE;
	    for (i = Dmc_rep->queue_start, last = FALSE;;i++)
	    {
		if (last)
		    break;
		if (i >= Dmc_rep->seg_size)
		    i = 0;
		if (i == Dmc_rep->queue_end)
		    last = TRUE;
		if (MEcmp(dcb->dcb_name.db_db_name, 
			repq[i].dbname.db_db_name, DB_DB_MAXNAME))
		    continue;
		if (repq[i].active)
		    /*
		    ** since we are the first user of this db in this server
		    ** (including queue management threads), then this must
		    ** mean that there is another server able to process 
		    ** entries for this db, so let them do it instead
		    */
		    break;
		if (repq[i].tx_id == 0) /* already done */
		    continue;
		/*
		** something to process
		*/
		repq[i].active = TRUE;
		CSv_semaphore(&Dmc_rep->rep_sem);
		have_mutex = FALSE;
            	uleFormat(NULL, W_DM9563_REP_TXQ_WORK, (CL_ERR_DESC *)NULL, ULE_LOG,
                    (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                    err_code, (i4)1, DB_DB_MAXNAME, dcb->dcb_name.db_db_name);
		status = dm2rep_qman(dcb, repq[i].tx_id, &repq[i].trans_time, 
		    NULL, dberr, TRUE);
                if (status != E_DB_OK && status != E_DB_WARN)
                {
                    /* TX will have been backed out, reset active and leave
                    ** for another thread to deal with */
                    CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
                    repq[i].active = FALSE;
                    CSv_semaphore(&Dmc_rep->rep_sem);
                    break;
                }
		/* clear the entry from the queue */
		CSp_semaphore(TRUE, &Dmc_rep->rep_sem);
		have_mutex = TRUE;
		repq[i].active = FALSE;
		repq[i].tx_id = 0;
		if (i == Dmc_rep->queue_start)
		{
		    for (j = i; 
			 repq[j].active == 0 && repq[j].tx_id == 0 &&
			     j != Dmc_rep->queue_end; 
			 j = j % (Dmc_rep->seg_size + 1),j++);
		    Dmc_rep->queue_start = j;
		    /*
		    ** signal waiting threads
		    */
		    lk_event.type_high = REP_READQ;
		    lk_event.type_low = 0;
		    lk_event.value = REP_READQ_VAL;

		    cl_status = LKevent(LK_E_CLR | LK_E_CROSS_PROCESS, 
			lock_list, &lk_event, &sys_err);
                    if (cl_status != OK)
                    {
                        uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE*)NULL,
                            (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
                        uleFormat(NULL, E_DM904B_BAD_LOCK_EVENT, &sys_err, ULE_LOG,
                            (DB_SQLSTATE*)NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code,
                            3, 0, LK_E_CLR, 0, REP_READQ, 0, lock_list);
                        status = E_DB_ERROR;
                        break;
                    }
		}
		/*
		** If replication failed (with a warning), a hijacked call
		** can just continue to open the database and leave the
		** other process (the one that didn't deadlock) to finish
		** the distribution job
		*/
		if (status == E_DB_WARN)
		    break;
	    } /* transaction queue processing */
	    if (have_mutex)
		CSv_semaphore(&Dmc_rep->rep_sem);

	    /* Allow warning of "escaping repeated deadlock" */
	    if (status != E_DB_OK && status != E_DB_WARN)
		break;

	    /*
	    ** Only do the second-stage qman if we haven't already had
	    ** a deadlock in the first stage.  If there was a deadlock in
	    ** the first stage we know there's someone else doing this
	    ** and our involvement would only delay matters
	    */
	    if (status == E_DB_OK)
	    {
		/*
		** check for remaining entries (only be there if the system
		** crashed and we have no transaction queue entry for them).
		*/
		status = dm2rep_qman(dcb, -1, (HRSYSTIME *)-1, NULL, dberr,
			TRUE);
		/*
		** warning means deadlock coming out, which is OK in a 
		** hijacked call (it means someone else will finish the job
		*/
		if (status != E_DB_OK && status != E_DB_WARN)
		{
		    char	dbname[DB_DB_MAXNAME + 1];

		    /* log return error and re-set */
		    if (dberr->err_code)
		        uleFormat(dberr, 0, NULL, ULE_LOG, 
                            (DB_SQLSTATE *)NULL, NULL, 0, NULL, 
		    	    &local_error, 0);
		    MEcopy(dcb->dcb_name.db_db_name, DB_DB_MAXNAME, dbname);
		    dbname[DB_DB_MAXNAME] = 0;
                    uleFormat(dberr, E_DM9569_REP_INQ_CLEANUP, (CL_ERR_DESC *)NULL, ULE_LOG,
                        (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                        &local_err, (i4)1, STtrmwhite(dbname), dbname);
	            break;
		}
	    }
	} /* if this is a replicated db */

        dcb->dcb_status &= ~(DCB_S_MLOCKED);
	dm0s_munlock(&dcb->dcb_mutex);
	return (E_DB_OK);
    }

    /*
    **	Something wrong happened.  Cleanup the mess that is leftover.
    **	Depending on how far we got, different variables will have been
    **	set.  By checking and undoing the operations in reverse order we
    **  can back out the open of the database.
    */

    /* release rcb */
    if ( idx_rcb )
	status = dm2r_release_rcb(&idx_rcb, &local_dberr);

    if (rcb)
	status = dm2r_release_rcb(&rcb, &local_dberr);

    if (repdb_rcb)
	status = dm2t_close(repdb_rcb, 0, &local_dberr);

    /*	Close the configuration file. */

    if (config)
	status = dm0c_close(cnf, 0, &local_dberr);

    /*	Close any open files. */

    if (idx_tcb)
    {
	if (idx_tcb->tcb_table_io.tbio_status & TBIO_OPEN)
	{
	    status = dm2f_release_fcb(lock_list, (i4)0,
		idx_tcb->tcb_table_io.tbio_location_array, 
		idx_tcb->tcb_table_io.tbio_loc_count, 0, &local_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
                    (DB_SQLSTATE *)NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    }
	}
	if (idx_tcb->tcb_hash_mutex_ptr)
	{
	    dm0s_mlock(idx_tcb->tcb_hash_mutex_ptr);
	    idx_tcb->tcb_q_next->tcb_q_prev = idx_tcb->tcb_q_prev;
	    idx_tcb->tcb_q_prev->tcb_q_next = idx_tcb->tcb_q_next;
	    dm0s_munlock(idx_tcb->tcb_hash_mutex_ptr);

	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_tcbcount--;
	    hcb->hcb_cache_tcbcount[idx_tcb->tcb_table_io.tbio_cache_ix]--;
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}
	dm0s_mrelease(&idx_tcb->tcb_et_mutex);
	dm0s_mrelease(&idx_tcb->tcb_mutex);
	dm0m_deallocate((DM_OBJECT **)&idx_tcb);
    }
    if (att_tcb)
    {
	if (att_tcb->tcb_table_io.tbio_status & TBIO_OPEN)
	{
	    status = dm2f_release_fcb(lock_list, (i4)0,
		att_tcb->tcb_table_io.tbio_location_array,
		att_tcb->tcb_table_io.tbio_loc_count, 0, &local_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
                    (DB_SQLSTATE *)NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    }
	}
	if (att_tcb->tcb_hash_mutex_ptr)
	{
	    dm0s_mlock(att_tcb->tcb_hash_mutex_ptr);
	    att_tcb->tcb_q_next->tcb_q_prev = att_tcb->tcb_q_prev;
	    att_tcb->tcb_q_prev->tcb_q_next = att_tcb->tcb_q_next;
	    dm0s_munlock(att_tcb->tcb_hash_mutex_ptr);

	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_tcbcount--;
	    hcb->hcb_cache_tcbcount[att_tcb->tcb_table_io.tbio_cache_ix]--;
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}
	dm0s_mrelease(&att_tcb->tcb_et_mutex);
	dm0s_mrelease(&att_tcb->tcb_mutex);
	dm0m_deallocate((DM_OBJECT **)&att_tcb);
    }
    if (relx_tcb)
    {
	if (relx_tcb->tcb_table_io.tbio_status & TBIO_OPEN)
	{
	    status = dm2f_release_fcb(lock_list, (i4)0,
		relx_tcb->tcb_table_io.tbio_location_array, 
		relx_tcb->tcb_table_io.tbio_loc_count, 0, &local_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
                    (DB_SQLSTATE *)NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    }
	}
	if (rel_tcb->tcb_hash_mutex_ptr)
	{
	    /* Above isn't a typo - relx tcb isn't on a hash queue, but
	    ** rel-tcb is.  If we queued rel-tcb, we counted them both.
	    ** Back down the counts for the relx tcb.
	    */

	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_tcbcount--;
	    hcb->hcb_cache_tcbcount[relx_tcb->tcb_table_io.tbio_cache_ix]--;
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}
	dm0s_mrelease(&relx_tcb->tcb_et_mutex);
	dm0s_mrelease(&relx_tcb->tcb_mutex);
	dm0m_deallocate((DM_OBJECT **)&relx_tcb);
    }
    if (rel_tcb)
    {
	if (rel_tcb->tcb_table_io.tbio_status & TBIO_OPEN)
	{
	    status = dm2f_release_fcb(lock_list, (i4)0,
		rel_tcb->tcb_table_io.tbio_location_array,
		rel_tcb->tcb_table_io.tbio_loc_count, 0, &local_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
                    (DB_SQLSTATE *)NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	    }
	}
	if (rel_tcb->tcb_hash_mutex_ptr)
	{
	    dm0s_mlock(rel_tcb->tcb_hash_mutex_ptr);
	    rel_tcb->tcb_q_next->tcb_q_prev = rel_tcb->tcb_q_prev;
	    rel_tcb->tcb_q_prev->tcb_q_next = rel_tcb->tcb_q_next;
	    dm0s_munlock(rel_tcb->tcb_hash_mutex_ptr);

	    dm0s_mlock(&hcb->hcb_tq_mutex);
	    hcb->hcb_tcbcount--;
	    hcb->hcb_cache_tcbcount[rel_tcb->tcb_table_io.tbio_cache_ix]--;
	    dm0s_munlock(&hcb->hcb_tq_mutex);
	}
	dm0s_mrelease(&rel_tcb->tcb_et_mutex);
	dm0s_mrelease(&rel_tcb->tcb_mutex);
	dm0m_deallocate((DM_OBJECT **)&rel_tcb);
    }

    /*
    ** Tell the buffer manager we are closing the database so it
    ** can decrement its reference to this db.
    */
    status = dm0p_dbcache(dcb->dcb_id, 0, 0, lock_list, &local_dberr);
    if (status != E_DB_OK)
    {
	uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, 
            (DB_SQLSTATE *)NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
    }

    /*	Deallocate the EXT. */

    if (dcb->dcb_ext)
    {
	dm0m_deallocate((DM_OBJECT **)&dcb->dcb_ext);
    }
    dcb->dcb_root = 0;
    dcb->dcb_jnl = 0;
    dcb->dcb_ckp = 0;

    /*	Unlock temporary table lock and opendb lock. */

    if ((dcb->dcb_odb_lock_id[0]) && ((flag & DM2D_NLK) == 0))
    {
	cl_status = LKrelease((i4)0, svcb->svcb_lock_list,
	    (LK_LKID *)dcb->dcb_odb_lock_id, (LK_LOCK_KEY *)NULL,
	    (LK_VALUE *)NULL, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, 
                (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
		0, svcb->svcb_lock_list);
	}
	dcb->dcb_odb_lock_id[0] = 0;
    }

    if ((dcb->dcb_tti_lock_id[0]) && ((flag & DM2D_NLK) == 0))
    {
	cl_status = LKrelease((i4)0, svcb->svcb_lock_list,
	    (LK_LKID *)dcb->dcb_tti_lock_id, (LK_LOCK_KEY *)NULL,
	    (LK_VALUE *)NULL, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, 
                (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
		0, svcb->svcb_lock_list);
	}
	dcb->dcb_tti_lock_id[0] = 0;
    }

    /*	Unlock the database. */

    if ((dcb->dcb_lock_id[0]) && 
	((flag & (DM2D_NLK | DM2D_IGNORE_CACHE_PROTOCOL)) == 0))
    {
	cl_status = LKrelease((i4)0, svcb_lock_list,
            (LK_LKID *)dcb->dcb_lock_id, (LK_LOCK_KEY *)NULL, &lock_value,
	    &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG,  
                (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
		0, svcb_lock_list);
	}
	dcb->dcb_lock_id[0] = 0;
    }

    /*	Unlock database for caller. */

    if (dblkid.lk_common)
    {
	cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)&dblkid, 
	    (LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, 
                (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1,
		0, lock_list);
	}
    }

    /*	Unlock the DCB. */

    if(db_opened) /* b104267 Bump down db open count if rep input queue cleanup failed */
    {
        dcb->dcb_ref_count--;
	if (dcb->dcb_status & DCB_S_HAS_RAW)
	    CSadjust_counter(&svcb->svcb_rawdb_count, -1);
    }
    else
    	dcb->dcb_opn_count--; /* Only bump down if not done already */

    dcb->dcb_status &= ~(DCB_S_MLOCKED);
    dm0s_munlock(&dcb->dcb_mutex);

    if (dberr->err_code > E_DM_INTERNAL || dberr->err_code == E_DM0100_DB_INCONSISTENT)
    {
	/*	Log the change in error messages. */

	uleFormat(dberr, 0, NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
                   NULL, 0, NULL, &local_error, 0);
	if (dberr->err_code != E_DM0100_DB_INCONSISTENT)
	    SETDBERR(dberr, 0, E_DM9268_OPEN_DB);
        else
        {
            uleFormat (NULL, E_DM0152_DB_INCONSIST_DETAIL, (CL_ERR_DESC *)NULL, ULE_LOG,
                (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                &local_error, 1, sizeof (dcb->dcb_name), &dcb->dcb_name);
        }
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm2d_extend_db	- Add new location to config file.
**
** Description:
**	o Lock the database
**      o Open the configuration file.
**      o Read the relation, attribute and indexes description from the
**	  configuration file.
**	o Read the list of locations that this database spans and compare
**	  it with the list passed in at dm2d_add_db time.  Update the list
**	  to contain any new locations. Create data directory if nec.
**      o Log the location file update
**      o Close the config fle
**      o Unlock the database
**
** Inputs:
**	lock_list			The lock list to own the database lock.
**      name                            The database name to open.
**      db_loc                          The location of the database
**      l_db_loc                        Length of location     
**      location_name                   The location
**      area                            Database area
**      l_area                          Length of area
**      path                            Physical path name
**      l_path                          Length of pathname
**      loc_type                        Type of location
**	raw_pct				Percent of raw area to 
**					allocate to location.
**
** Outputs:
**	raw_blocks			Number of raw blocks allocated,
**					zero if not raw.
**	raw_start			Starting block of raw location.
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-sep-1989 (derek)
**          Created new for ORANGE.
**	24-sep-1990 (rogerk)
**	    Merge 6.3 fixes - copy config file to backup location after
**	    updating it.
**	25-apr-91 (teresa)
**	    add support for flag to indicate that the extended location 
**	    directory already exists and that dm2d_extend_db should bypass
**	    the DIdircreate call that usually creates it.  This is used
**	    primarily for support of convto60, where a v5 extended location
**	    is being made known to the v6 installation.
**	25-apr-91 (teresa)
**	    fix bug where lx_id was not being passed to dm0l_closedb or
**	    dm0l_opendb
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5 line.
**	    This module was merged in with the 6.4 version of the file.
**	    XXXX NEEDS ERROR HANDLING!!!
**  	15-oct-1991 (mikem)
**          Eliminated unix compile warnings ("warning: statement not reached")
**          by changing the way "for (;xxx == E_DB_OK;)" loops were coded.
**  	18-oct-1991 (jnash)
**          Add header documentation and additional error handling.
**	23-Oct-1991 ( rmuth)
**	    Change error handling as was looping if all was ok.
**	05-nov-92 (jrb)
**	    Create WORK directory when extending to work location.
**	30-nov-1992 (jnash)
**	    Reduced logging project.  New param to dm0l_location.
**	14-dec-92 (jrb)
**	    Removed DM_M_DIREXISTS support since it isn't used.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed dmxe_abort output parameter used to return the log address
**		of the abortsave log record.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Add dcb parameter to dmxe_abort call.
**	26-jul-1993 (bryanp)
**	    We no longer log a closedb log record, which means that we no
**		longer need to pass a lx_id argument to dm0l_opendb and to
**		dm0l_closedb.
**	26-jul-1993 (jnash)
**	    Log complete path name in dm0l_location().  Add new error 
**	    message at failure in creating directory during a database extend.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**      18-oct-93 (jrb)
**          Added checks to ensure dcb ptr was valid before using it in error
**          recovery at the end of this routine.
**	18-jan-95 (cohmi01)
**	    For raw locations, use bitwise op on loc_type as RAW bit may also
**	    be on.
**	24-apr-95 (cohmi01)
**	    Added dm2d_get_dcb() to obtain an open dcb ptr from svcb list.
**	29-feb-1996 (angusm)
**	    (bug 72050). Backed out change for bug 67276 as not needed.
**	    (relocatedb does not call dm2d_extend_db).
**	    Implemented different checking that location is not
**	    already in config file: under OI, locations can have
**	    multiple usage
**	12-mar-1999 (nanpr01)
**	    Fix up parameter passing for supporting raw location. 
** 	25-Mar-2001 (jenjo02)
**	    Productized for raw locations.
**	02-Apr-2001 (jenjo02)
**	    Added *raw_start to prototype into which raw_start is returned.
**	    Rewrote raw location list creation and subsequent sort.
**	11-Apr-2001 (jenjo02)
**	    Corrected handling of raw_blocks == 0 or -1, neither of which
**	    worked correctly.
**	11-May-2001 (jenjo02)
**	    Raw sizing changed to percent-of-area request instead of
**	    absolute number of blocks.
**	15-Oct-2001 (jenjo02)
**	    Return err_code from dm2f_sense_raw() rather than
**	    E_DM0193_RAW_LOCATION_EXTEND.
**	    Fixed use of *i4 instead of *DB_SQLSTATE in ule_format()'s
**	28-Dec-2004 (jenjo02)
**	    Return location's raw info even if location exists;
**	    QEF will add an IIEXTEND tuple with this info anyway,
**	    so we want it to be right.
**	28-Jul-2005 (schka24)
**	    Back out x-integration for bug 111502 (not shown), doesn't
**	    apply to r3.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
DB_STATUS
dm2d_extend_db(
    i4	    	lock_list,
    DB_DB_NAME	    	*name,
    char		*db_loc,
    i4		l_db_loc,
    DB_LOC_NAME	    	*location_name,
    char		*area,
    i4		l_area,
    char		*path,
    i4		l_path,
    i4		loc_type,
    i4		raw_pct,
    i4		*raw_start,
    i4		*raw_blocks,
    DB_ERROR		*dberr )
{
    DM_SVCB		*svcb = dmf_svcb;
    DMP_DCB		*dcb = 0;
    i4			tran_lgid = 0;
    i4			tran_llid = 0;
    DB_TRAN_ID		tran_id;
    DM0C_CNF		*config = 0;
    DM0C_CNF		*cnf;
    LK_VALUE		odb_k_value;
    LK_VALUE		lock_value;
    LK_LOCK_KEY		db_key;
    LK_LKID		dblkid;
    DB_STATUS		status;
    STATUS      	cl_status;
    CL_ERR_DESC		sys_err;
    i4			i, j;
    i4			dmxe_flag = DMXE_JOURNAL;
    DB_TAB_TIMESTAMP	ctime;
    i4             	local_error;
    i4			raw_total_blocks = 0, raw_in_use = 0;
    i4			raw_count = 0;
    DMP_LOC_ENTRY	*loc;
    DMP_LOC_ENTRY	*raw_locs[512];
    DMP_LOC_ENTRY	**raw_loc;
    char		*raw_array = (char*)NULL;
    i4			*err_code = &dberr->err_code;
    i4			local_err;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);
    
    MEfill(sizeof(LK_LKID), 0, &dblkid);
    for (;;)
    {
	 /* Allocate a DCB for this database. */

	/* Use ShortTerm memory */
	status = dm0m_allocate((i4)sizeof(DMP_DCB),
	    (DM0M_ZERO | DM0M_SHORTTERM),
	    (i4)DCB_CB, (i4)DCB_ASCII_ID, (PTR)svcb,
	    (DM_OBJECT **)&dcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*  Copy root location name into the DCB. */

	STRUCT_ASSIGN_MACRO(*name, dcb->dcb_name);
	dcb->dcb_location.phys_length = l_db_loc;
	MEmove(l_db_loc, db_loc, ' ',
	    sizeof(dcb->dcb_location.physical),
	    (PTR)&dcb->dcb_location.physical);
	dcb->dcb_location.flags = 0;
	dcb->dcb_odb_lock_id[0] = 0;
	dcb->dcb_log_id = 0;
	    
	/*  Initialize the lock key. */

	MEcopy((PTR)&dcb->dcb_name, LK_KEY_LENGTH, (PTR)&db_key.lk_key1);
	db_key.lk_type = LK_DATABASE;

	/*  Lock the database for exclusive access. */

	cl_status = LKrequest(LK_PHYSICAL | LK_NOWAIT, lock_list,
            (LK_LOCK_KEY *)&db_key, LK_X, (LK_VALUE *)NULL, (LK_LKID *)&dblkid,
            (i4)0, &sys_err);
	if (cl_status == OK)
    	{
	    /* Get the dbopen lock, used to determine if 
	    ** this is the first open of this database in any server. 
	    */
	    db_key.lk_type = LK_OPEN_DB;
	    cl_status = LKrequest(LK_PHYSICAL, lock_list, &db_key, LK_X,
		    &odb_k_value, (LK_LKID *)dcb->dcb_odb_lock_id,
		    (i4)0, &sys_err);

            /* If the lock wasn't valid treat it as a success, we'll be updating the
            ** lock value when we release it.
            */
            if (cl_status == LK_VAL_NOTVALID)
            {
               cl_status = OK;
            }
	}
	if (cl_status != OK) 
    	{
            uleFormat(NULL, cl_status, &sys_err, ULE_LOG ,
                       (DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
                       err_code, 1, 0, lock_list);
	    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, (CL_ERR_DESC *) 0, ULE_LOG , 
		       (DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 
                       2, 0, LK_X, 0, lock_list );
	    SETDBERR(dberr, 0, E_DM9269_LOCK_OPEN_DB);
            if (cl_status == LK_BUSY)
            {
                uleFormat(dberr, 0, (CL_ERR_DESC *)NULL,
                    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                    &local_error, 0);
		SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
            }
	    status = E_DB_ERROR;
	    break;
    	} 

	/*  Open the configuration file. */
	
	status = dm0c_open(dcb, 0, lock_list, &cnf, dberr);
	if (status != E_DB_OK)
	    break;
	config = cnf;

	/*
	** RAW location considerations:
	**
	** Make a list of all locations already extended to this
	** area then find a vacant slice which is large enough
	** to contain "raw_blocks" and assign the new location
	** to that slice.
	*/
	if ( loc_type & DCB_RAW )
	{
	    if ( cnf->cnf_dsc->dsc_ext_count 
		> sizeof(raw_locs)/sizeof(DMP_LOC_ENTRY*) )
	    {
		/* Allocate a bigger raw location pointer array */
		status = dm0m_allocate(
		    (cnf->cnf_dsc->dsc_ext_count * sizeof(DMP_LOC_ENTRY*))
			+ sizeof(DMP_MISC),
		    (i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
		    (char*)dcb, (DM_OBJECT**)&raw_array, dberr);
		if ( status )
		    break;
		raw_loc = (DMP_LOC_ENTRY**)(raw_array + sizeof(DMP_MISC));
		((DMP_MISC*)raw_array)->misc_data = (char*)raw_loc;
	    }
	    else
		raw_loc = (DMP_LOC_ENTRY**)&raw_locs[0];
	    raw_count = 0;
	}

	/*  Check if location already exists. */

	for (i = 0; i < cnf->cnf_dsc->dsc_ext_count; i++)
	{
	    loc = &cnf->cnf_ext[i].ext_location;

            if ( (MEcmp((char *)&loc->logical,
			(char *)location_name, DB_LOC_MAXNAME) == 0)
		&& loc->flags & loc_type )
	    {
		/* Return extant raw info, or zeroes, anyway */
		*raw_start = loc->raw_start;
		*raw_blocks = loc->raw_blocks;
		status = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM007E_LOCATION_EXISTS);
		break;
	    }

	    /* Make a list of all RAW locations extended to this area */
	    if ( loc_type & DCB_RAW && loc->flags & DCB_RAW &&
		 (MEcmp((char*)&loc->physical,
			(char*)path, l_path)) == 0 )
	    {
		raw_loc[raw_count++] = loc;
		raw_total_blocks = loc->raw_total_blocks;
		raw_in_use += loc->raw_blocks;
	    }
	}

	if (status)
	    break;

	if ( loc_type & DCB_RAW )
	{
	    /*
	    ** If total RAW blocks is unknown, "sense" it.
	    ** Note that the number of blocks must be guaranteed to
	    ** be a multiple of 64.
	    */
	    if ( raw_total_blocks == 0 &&
	         dm2f_sense_raw(path, l_path, &raw_total_blocks, 
			    lock_list, dberr) )
	    {
		status = E_DB_ERROR;
		uleFormat(dberr, 0, (CL_ERR_DESC*)NULL, ULE_LOG, 
                            (DB_SQLSTATE *)NULL,
			    (char *)NULL, 0L, (i4 *)NULL, &local_err, 0);
		break;
	    }

	    /* Sort list of RAW locations into raw_start order */
	    for ( i = 0; i < raw_count; i++ )
	    {
		for ( j = i+1; j < raw_count; j++ )
		{
		    if ( raw_loc[i]->raw_start > raw_loc[j]->raw_start )
		    {
			/* Swap i and j */
			loc = raw_loc[i];
			raw_loc[i] = raw_loc[j];
			raw_loc[j] = loc;
		    }
		}
	    }

	    /* Compute raw_blocks from requested pct */
	    if ( *raw_blocks = ((raw_total_blocks * raw_pct) / 100)
			     + ((raw_total_blocks * raw_pct) % 100
					? 1 : 0) )
	    {
		/* Ensure that the minimum number of blocks are allocated */
		if ( *raw_blocks < DM2F_RAW_MINBLKS )
		    *raw_blocks = DM2F_RAW_MINBLKS;
		    
		/* Find a vacant slice large enough to contain raw_blocks */

		*raw_start = 0;

		for ( i = 0; i < raw_count; i++ )
		{
		    /* Find the first vacant slice with sufficient space */
		    if ( raw_loc[i]->raw_start - *raw_start &&
			 raw_loc[i]->raw_start - *raw_start >= *raw_blocks )
		    {
			break;
		    }
		    *raw_start = raw_loc[i]->raw_start + raw_loc[i]->raw_blocks;
		}
		
		/* If end of locations, take from the end of the RAW area */
		if ( i == raw_count && 
			raw_total_blocks - *raw_start < *raw_blocks )
		{
		    /* Take whatever's left, if any */
		    if ( raw_total_blocks - *raw_start >= DM2F_RAW_MINBLKS )
			*raw_blocks = raw_total_blocks - *raw_start;
		    else
			status = E_DB_ERROR;
		}
	    }

	    if ( status || *raw_blocks == 0 )
	    {
		/* No space available, or raw_pct == 0 */
		uleFormat(NULL, E_DM0193_RAW_LOCATION_EXTEND,
		    (CL_ERR_DESC*)NULL,
		    ULE_LOG, (DB_SQLSTATE *)NULL, NULL, 0, NULL, err_code, 4,
		    l_area, area,
		    0, raw_total_blocks -  raw_in_use,
		    DB_LOC_MAXNAME, location_name,
		    0, *raw_blocks); 
		status = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM0193_RAW_LOCATION_EXTEND);
		break;
	    }

	    if ( raw_array )
		dm0m_deallocate((DM_OBJECT **)&raw_array);

	} /* if ( loc_type & DCB_RAW ) */
	else
	{ /* This is not a raw location */
	    *raw_start = 0;
	    *raw_blocks = 0;
	}

	/* Check if there is room. */

	if (cnf->cnf_free_bytes < sizeof(DM0C_EXT))
	{
	    status = dm0c_extend(cnf, dberr);
	    if (status != E_DB_OK)
	    {
		SETDBERR(dberr, 0, E_DM0071_LOCATIONS_TOO_MANY);
		break;
	    }
	}

	/*	Set the unique identifier for the database. */

	dcb->dcb_id = cnf->cnf_dsc->dsc_dbid;
	dcb->dcb_db_owner = cnf->cnf_dsc->dsc_owner;
	if ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) == 0)
	    dmxe_flag = 0;

	status = dm0c_close(cnf, 0, dberr);

	if (status != E_DB_OK)
	    break;
	config = 0;

	/*
	** Now convert the database open lock back 
	** to null after incrementing the value.
	*/

	odb_k_value.lk_low++;
	cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT, lock_list, 0, LK_N,
	    (LK_VALUE * )&odb_k_value, (LK_LKID*)dcb->dcb_odb_lock_id,
	    (i4)0, &sys_err); 
    	if (cl_status != OK)
    	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 0);
            uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG , 
                (DB_SQLSTATE *)NULL,
                (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_list); 
	    SETDBERR(dberr, 0, E_DM9269_LOCK_OPEN_DB);
            status = E_DB_ERROR;
            break;
    	}

	{
	    i4	open_flag = 0;

	    if (dmxe_flag & DMXE_JOURNAL)
	        open_flag |= DM0L_JOURNAL;
	    status = dm0l_opendb(svcb->svcb_lctx_ptr->lctx_lgid,
		    open_flag, &dcb->dcb_name, &dcb->dcb_db_owner, 
		    dcb->dcb_id, &dcb->dcb_location.physical,
		    dcb->dcb_location.phys_length, &dcb->dcb_log_id,
		    (LG_LSN *)NULL,
		    dberr);
	    if (status != E_DB_OK)
	        break;
	}

	/* Begin a transaction. */

	status = dmxe_begin(DMXE_WRITE, dmxe_flag, dcb, dcb->dcb_log_id,
	    &dcb->dcb_db_owner, lock_list, &tran_lgid,
	    &tran_id, &tran_llid, (DB_DIS_TRAN_ID *)NULL, dberr);
	if (status != E_DB_OK)
	    break;
	
    	for (;;)
	{
	    i4	    journal = 0;

	    /*  Open the configuration file. */
	    
	    status = dm0c_open(dcb, 0, lock_list, &cnf, dberr);

	    if (status != E_DB_OK)
	        break;
	    config = cnf;

	    /*  LOG add location if journal on. */

	    if (dmxe_flag & DMXE_JOURNAL)
	        journal = DM0L_JOURNAL;
	    status = dm0l_location(tran_lgid, journal,
	        loc_type, location_name,
	        l_path, (DM_PATH *)path, 
		*raw_start, *raw_blocks, raw_total_blocks,
		(LG_LSN *)NULL, dberr);
	    if (status != E_DB_OK)
	        break;

	    /*  Create directory if it's a data directory or work directory.
	    **  If raw location, the appropriate directories have already
	    **  been created by mkraware.sh .
	    */

	    if ((loc_type & (DCB_DATA | DCB_WORK | DCB_AWORK)) &&
		!(loc_type & DCB_RAW) )
	    {
		char	    db_buffer[sizeof(DB_DB_NAME) + 4];
		i4	    db_length;
		DI_IO  	    di_context; 

		for (i = 0; i < sizeof(DB_DB_NAME); i++) 
		    if ((db_buffer[i] = dcb->dcb_name.db_db_name[i]) == ' ')
			break;

		db_buffer[i] = 0;
		db_length = STlength(db_buffer);

		cl_status = DIdircreate(&di_context, area, l_area,
			db_buffer, db_length, &sys_err);
		if (cl_status != OK  &&  cl_status != DI_EXISTS)
		{
		    uleFormat(NULL, cl_status, &sys_err,
			   ULE_LOG, (DB_SQLSTATE *)NULL, NULL, 0, NULL, err_code, 4,
			    db_length, db_buffer, 0, "", l_area, area, 
			    db_buffer, db_length);
		    uleFormat(NULL, E_DM929D_BAD_DIR_CREATE, (CL_ERR_DESC *)NULL, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, NULL, 0, NULL, err_code, 0);

		    SETDBERR(dberr, 0, E_DM011E_ERROR_ADD_LOC);
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /*  Add location to config file. */

	    i = cnf->cnf_dsc->dsc_ext_count++;
	    cnf->cnf_ext[i].length = sizeof(DM0C_EXT);
	    cnf->cnf_ext[i].type = DM0C_T_EXT;

	    loc = &cnf->cnf_ext[i].ext_location;
	    MEcopy((char *)location_name, sizeof(DB_LOC_NAME),	 
		   (char *)&loc->logical);
	    MEmove(l_path, (char *)path, ' ', 
		    sizeof(loc->physical), (char *)&loc->physical);
	    loc->flags = loc_type;
	    loc->phys_length = l_path;
	    loc->raw_start = *raw_start;
	    loc->raw_blocks = *raw_blocks;
	    loc->raw_total_blocks = raw_total_blocks;

	    cnf->cnf_ext[i+1].length = 0;
	    cnf->cnf_ext[i+1].type = 0;

	    /*  Close the configuration file. */

	    status = dm0c_close(cnf, DM0C_UPDATE | DM0C_COPY, dberr);
	    if (status != E_DB_OK)
	        break;
	    config = 0;
	    
	    status = dmxe_commit(&tran_id, tran_lgid, tran_llid, dmxe_flag,
	   	    &ctime, dberr);
	    if (status != E_DB_OK)
	        break;
	    tran_lgid = 0;

            /* Clean up after successful completion */

    	    if (dcb->dcb_log_id)
      	    {
		status = dm0l_closedb(dcb->dcb_log_id, dberr);
                dcb->dcb_log_id = 0;    
		if (status != E_DB_OK)
		    break;
            }

	    if (dblkid.lk_unique)
    	    {
    	   	db_key.lk_type = LK_DATABASE;
    	    	cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)NULL, 
    		    (LK_LOCK_KEY *)&db_key, (LK_VALUE *)NULL, &sys_err);
    	    	if (cl_status != OK)
    	   	{ 
    		    uleFormat(NULL, cl_status, &sys_err, 
                           ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
                           (i4 *)NULL, err_code, 1, 0, lock_list);
                    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, (CL_ERR_DESC *)NULL,
                          ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
                          (i4 *)NULL, err_code, 0);
	    	    status = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9266_UNLOCK_CLOSE_DB);
                    break;
	        }
		dblkid.lk_unique = 0;
	    }

	    /*	Unlock open database table lock */

	    if (dcb->dcb_odb_lock_id[0])
	    {
	        cl_status = LKrelease(0, lock_list,
		        (LK_LKID *)dcb->dcb_odb_lock_id, (LK_LOCK_KEY *)NULL,
		        (LK_VALUE *)NULL, &sys_err);
	        if (cl_status != OK)
	        {
	            uleFormat(NULL, cl_status, &sys_err, 
                        ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		        err_code, 1, 0, lock_list);
                    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, (CL_ERR_DESC *)NULL,
                       ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                       err_code, 0);
		    status = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9266_UNLOCK_CLOSE_DB);
		    break;
	        }
		dcb->dcb_odb_lock_id[0] = 0;
	    }

	    /* Everything went ok */
	    break;

	}

	if (status != E_DB_OK)
	    break;

	dm0m_deallocate((DM_OBJECT **)&dcb);

	return (E_DB_OK);
 
    }     

    /*  Handle error recovery.
    **  We try to unravel things in the reverse order in which they were done.
    **  Depending on how far we got, different variables will be set.
    */

    if (config)
    {
        /* Close configuration file. */
	status = dm0c_close(cnf, 0, &local_dberr);
        if (status != E_DB_OK)
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 0);

    }

    if (tran_lgid)
    {
	/* Abort transaction in progress. */
	status = dmxe_abort((DML_ODCB *)NULL, dcb, &tran_id, tran_lgid,
		    tran_llid, dmxe_flag,
		    0, 0, &local_dberr);
        if (status != E_DB_OK)
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 0);

    }

    if (dcb && dcb->dcb_log_id) 
    {
	status = dm0l_closedb(dcb->dcb_log_id, &local_dberr);
	dcb->dcb_log_id = 0;    
        if (status != E_DB_OK)
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 0);
    }

    if (dcb && dcb->dcb_odb_lock_id[0]) 
    {
        /* Undo table lock */
        cl_status = LKrelease(0, lock_list,
            (LK_LKID *)dcb->dcb_odb_lock_id, (LK_LOCK_KEY *)NULL,
            (LK_VALUE *)NULL, &sys_err);
	if (cl_status != OK)
            uleFormat(NULL, cl_status, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 1, 0, lock_list);
    }

    if (dblkid.lk_unique)
    {
        /* Undo database lock */
        db_key.lk_type = LK_DATABASE;
        cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)NULL,
                        (LK_LOCK_KEY *)&db_key, (LK_VALUE *)NULL, &sys_err);
	if (cl_status != OK)
            uleFormat(NULL, cl_status, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 1, 0, lock_list);
    }

    if (dcb)
	dm0m_deallocate((DM_OBJECT **)&dcb);
    if ( raw_array )
	dm0m_deallocate((DM_OBJECT **)&raw_array);

    if (dberr->err_code > E_DM_INTERNAL)
    {
        /*      Log the change in error messages. */

        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, 
	    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
	    &local_error, 0);
	SETDBERR(dberr, 0, E_DM93A6_EXTEND_DB);
    }

    return (E_DB_ERROR);
}

/*{
** Name: dm2d_unextend_db	- Delete location from config file.
**
** Description:
**	o Lock the database
**      o Open the configuration file.
**      o Read the relation, attribute and indexes description from the
**	  configuration file.
**	o Read the list of locations that the database spans.  Update the list
**	  to delete any locations. 
**      o Log the location file update
**      o Close the config fle
**      o Unlock the database
**
** Inputs:
**	lock_list			The lock list to own the database lock.
**      name                            The database name to open.
**      db_loc                          The location of the database
**      l_db_loc                        Length of location     
**      location_name                   The location
**      area                            Database area
**      l_area                          Length of area
**      path                            Physical path name
**      l_path                          Length of pathname
**      loc_type                        Type of location
**	raw_pct				Percent of raw area to 
**					allocate to location.
**
** Outputs:
**	raw_blocks			Number of raw blocks allocated,
**					zero if not raw.
**	raw_start			Starting block of raw location.
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	15-apr-2004 (gorvi01)
**          Created new SRS section 3.1.27.
**	09-Nov-2004 (jenjo02)
**	    Neither "area" nor "path" are null-terminated on entry to this function, so 
**	    null-terminate them.
**	    Properly handle errors from LO functions to avoid returning E_DB_ERROR
**	    without a corresponding err_code, which will induce the hated QE0018.
**	11-Nov-2004 (gorvi01)
**	    Modified the error handling using ule_format.	
**	28-Dec-2004 (jenjo02)
**	    Furthur cleaned up error handling, straightend unreadable code.
**	13-Sep-2005 (drivi01)
**	    Released a search handle created by LOwcard search.  Added a 
**	    missing call to LOwend when handle is no longer required.  
**	30-Jan-2007 (kibro01) b119768
**	    Move each location in turn.  It isn't valid to memcpy the whole
**	    thing in one movement because we're actually copying the
**	    substructures rather than a whole block.
*/

DB_STATUS 
dm2d_unextend_db(
    i4 		lock_list,
    DB_DB_NAME  *name,
    char 	*db_loc,
    i4 		l_db_loc,
    DB_LOC_NAME *location_name,
    char 	*area,
    i4 		l_area,
    char 	*path,
    i4 		l_path,
    i4 		loc_type,
    i4 		raw_pct,
    i4 		*raw_start,
    i4 		*raw_blocks,
    DB_ERROR 		*dberr )
{
    DM_SVCB		*svcb = dmf_svcb;
    DMP_DCB		*dcb;
    i4			tran_lgid = 0;
    i4			tran_llid = 0;
    DB_TRAN_ID		tran_id;
    DM0C_CNF		*config = NULL;
    DM0C_CNF		*cnf;
    LK_VALUE		odb_k_value;
    LK_VALUE		lock_value;
    LK_LOCK_KEY		db_key;
    LK_LKID		dblkid;
    DB_STATUS		status;
    STATUS      	cl_status;
    STATUS		loc_status;
    CL_ERR_DESC		sys_err;
    i4			i, j;
    i4			dmxe_flag;
    i4			open_flag;
    DB_TAB_TIMESTAMP	ctime;
    i4             	local_error;
    i4			raw_total_blocks = 0, raw_in_use = 0;
    i4			raw_count = 0;
    DMP_LOC_ENTRY	*loc;
    DMP_LOC_ENTRY	*raw_locs[512];
    DMP_LOC_ENTRY	**raw_loc;
    char		*raw_array = (char*)NULL;
    i4	    		journal = 0;
    char		phys_path[sizeof(DM_PATH)];
    LOCATION		tmploc;	
    LO_DIR_CONTEXT 	lc;
    i4         		temp_lock_list = 0;
    i4			*err_code = &dberr->err_code;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);

    MEfill(sizeof(LK_LKID), 0, &dblkid);

    /* Neither of these are null-terminated, do so now */
    area[l_area] = EOS;
    path[l_path] = EOS;

    for (;;)
    {
	/* Allocate a DCB for this database. */

	/* Use ShortTerm memory */
	status = dm0m_allocate((i4)sizeof(DMP_DCB),
		(DM0M_ZERO | DM0M_SHORTTERM),
		(i4)DCB_CB, (i4)DCB_ASCII_ID, (PTR)svcb,
		(DM_OBJECT **)&dcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*  Copy root location name into the DCB. */

	STRUCT_ASSIGN_MACRO(*name, dcb->dcb_name);
	dcb->dcb_location.phys_length = l_db_loc;
	MEmove(l_db_loc, db_loc, ' ',
		sizeof(dcb->dcb_location.physical),
		(PTR)&dcb->dcb_location.physical);
	dcb->dcb_location.flags = 0;
	dcb->dcb_odb_lock_id[0] = 0;
	dcb->dcb_log_id = 0;


	/*  Initialize the lock key. */

	MEcopy((PTR)&dcb->dcb_name, LK_KEY_LENGTH, (PTR)&db_key.lk_key1);
	db_key.lk_type = LK_DATABASE;

	/*  Lock the database for exclusive access. */

	cl_status = LKrequest(LK_PHYSICAL | LK_NOWAIT, lock_list,
			(LK_LOCK_KEY *)&db_key, LK_X, (LK_VALUE *)NULL, 
			(LK_LKID *)&dblkid, (i4)0, &sys_err);
	if (cl_status == OK)
	{
	    /* Get the dbopen lock, used to determine if 
	    ** this is the first open of this database in any server. 
	    */
	    db_key.lk_type = LK_OPEN_DB;
	    cl_status = LKrequest(LK_PHYSICAL, lock_list, &db_key, LK_X,
		    &odb_k_value, (LK_LKID *)dcb->dcb_odb_lock_id,
		    (i4)0, &sys_err); 
	}
	if (cl_status != OK) 
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 
                0, lock_list);
	    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, (CL_ERR_DESC *) 0, ULE_LOG , 
		(DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, err_code, 2, 0, 
                LK_X, 0, lock_list );
	    SETDBERR(dberr, 0, E_DM9269_LOCK_OPEN_DB);
	    if (cl_status == LK_BUSY)
	    {
		uleFormat(dberr, 0, (CL_ERR_DESC *)NULL,
			ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&local_error, 0);
		SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
	    }
	    status = E_DB_ERROR;
	    break;
	} 

	/*  Open the configuration file. */
	status = dm0c_open(dcb, 0, lock_list, &cnf, dberr);
	if (status != E_DB_OK)
	    break;
	config = cnf;

	/*  Check if location exists. */

	for (i = 0, loc = NULL; i < cnf->cnf_dsc->dsc_ext_count; i++)
	{
	    loc = &cnf->cnf_ext[i].ext_location;

	    if ( (MEcmp((char *)&loc->logical,
		    (char *)location_name, DB_LOC_MAXNAME) == 0)
		    && loc->flags & loc_type )
	    {
		/* location exists */
		break;
	    }
	    loc = NULL;
	}
	if ( loc == NULL )
	{
	    /* the location does not exist */
	    SETDBERR(dberr, 0, E_DM013D_LOC_NOT_EXTENDED);
	    status = E_DB_ERROR;
	    break;
	}

	/* Construct null-terminated physical path to DB's loc */
	MEcopy(loc->physical.name, loc->phys_length, phys_path);
	phys_path[loc->phys_length] = EOS;
					
	/* Check if there are any files in the location */

	if ( LOfroms(PATH, phys_path, &tmploc) == OK &&
	     LOwcard(&tmploc,NULL,NULL,NULL,&lc) == OK )
	{
	    /* files exist so cant be deleted */
	    SETDBERR(dberr, 0, E_DM019E_ERROR_FILE_EXIST);
	    status = E_DB_ERROR;
	    break;
	}
	LOwend(&lc);
					
	/*	Set the unique identifier for the database. */
	dcb->dcb_id = cnf->cnf_dsc->dsc_dbid;
	dcb->dcb_db_owner = cnf->cnf_dsc->dsc_owner;

	/* Remember if DB is journaled */
	if ( cnf->cnf_dsc->dsc_status & DSC_JOURNAL )
	    dmxe_flag = DMXE_JOURNAL;
	else
	    dmxe_flag = 0;

	status = dm0c_close(cnf, 0, dberr);

	config = NULL;

	if (status != E_DB_OK)
	    break;

	/*
	** Now convert the database open lock back 
	** to null after incrementing the value.
	*/

	odb_k_value.lk_low++;
	cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT, lock_list, 0, LK_N,
		(LK_VALUE * )&odb_k_value, (LK_LKID*)dcb->dcb_odb_lock_id,
		(i4)0, &sys_err); 
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG , 
                    (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_list); 
	    SETDBERR(dberr, 0, E_DM9269_LOCK_OPEN_DB);
	    status = E_DB_ERROR;
	    break;
	}

	status = dm0l_opendb(svcb->svcb_lctx_ptr->lctx_lgid,
		(dmxe_flag & DMXE_JOURNAL) ? DM0L_JOURNAL : 0,
		&dcb->dcb_name, &dcb->dcb_db_owner, 
		dcb->dcb_id, &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length, &dcb->dcb_log_id,
		(LG_LSN *)NULL,
		dberr);
	if (status != E_DB_OK)
	    break;

	/* Begin a transaction. */

	status = dmxe_begin(DMXE_WRITE, dmxe_flag, dcb, dcb->dcb_log_id,
		&dcb->dcb_db_owner, lock_list, &tran_lgid,
		&tran_id, &tran_llid, (DB_DIS_TRAN_ID *)NULL, dberr);
	if (status != E_DB_OK)
	    break;
	
	for (;;)
	{
	    i4	    journal = 0;

	    /*  Open the configuration file. */
	    
	    status = dm0c_open(dcb, 0, lock_list, &cnf, dberr);
	    if (status != E_DB_OK)
		break;
	    config = cnf;

	    /* 
	    ** Refind loc entry in config, then
	    ** log and delete it.
	    */
	    for (i = 0, loc = NULL; i < cnf->cnf_dsc->dsc_ext_count; i++)
	    {	    
		loc = &cnf->cnf_ext[i].ext_location;
		if ((MEcmp((char *)&loc->logical, (char *)location_name,
			sizeof(DB_LOC_NAME)) == 0) && loc->flags & loc_type )
		    break;
		loc = NULL;
	    }

	    if ( loc == NULL )
	    {
		/*
		** No entry found, nothing to unextend.
		** Obviously this should not happen; we
		** found it earlier...
		*/
		SETDBERR(dberr, 0, E_DM013D_LOC_NOT_EXTENDED);
		status = E_DB_ERROR;
		break;
	    }

	    /*  LOG delete location if journal on. */

	    if (dmxe_flag & DMXE_JOURNAL)
		journal = DM0L_JOURNAL;
	    status = dm0l_del_location(tran_lgid, journal,
		    loc_type, location_name,
		    loc->phys_length, &loc->physical,
		    (LG_LSN *)NULL, dberr);
	    if (status != E_DB_OK)
		break;

	    /* Now shuffle the rest of the locations*/
	    while (i < (cnf->cnf_dsc->dsc_ext_count - 1))
	    {	    
		/* Shift each one down one until we reach the end */
		MEcopy((char *)&cnf->cnf_ext[i+1].ext_location.logical,
			sizeof(DMP_LOC_ENTRY),
			(char *)&cnf->cnf_ext[i].ext_location.logical);
		/* Point "i" to next entry */
		i++;
	    }

	    /* Clear vacated slot */
	    MEfill(sizeof(DMP_LOC_ENTRY), 0, 
		    (char *)&cnf->cnf_ext[i].ext_location);

	    cnf->cnf_ext[i].length = 0;
	    cnf->cnf_ext[i].type = 0;
	    cnf->cnf_dsc->dsc_ext_count--;

	    /*  Close the configuration file. */
	    status = dm0c_close(cnf, DM0C_UPDATE | DM0C_COPY, dberr);
	    config = NULL;
	    if (status != E_DB_OK)
		break;

	    status = dmxe_commit(&tran_id, tran_lgid, tran_llid, dmxe_flag,
			    &ctime, dberr);
	    if (status != E_DB_OK)
		break;
	    tran_lgid = 0;

	    /* Clean up after successful completion */

	    if (dcb->dcb_log_id)
	    {
		status = dm0l_closedb(dcb->dcb_log_id, dberr);
		dcb->dcb_log_id = 0;    
		if (status != E_DB_OK)
		    break;
	    }
	    if (dblkid.lk_unique)
	    {
		db_key.lk_type = LK_DATABASE;
		cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)NULL, 
		    (LK_LOCK_KEY *)&db_key, (LK_VALUE *)NULL, &sys_err);
		if (cl_status != OK)
		{ 
		    uleFormat(NULL, cl_status, &sys_err, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
                            (i4 *)NULL, err_code, 1, 0, lock_list);
		    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, (CL_ERR_DESC *)NULL,
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
                            (i4 *)NULL, err_code, 0);
		    status = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9266_UNLOCK_CLOSE_DB);
		    break;
		}
		dblkid.lk_unique = 0;
	    }
	    /*	Unlock open database table lock */

	    if (dcb->dcb_odb_lock_id[0])
	    {
		cl_status = LKrelease(0, lock_list,
			(LK_LKID *)dcb->dcb_odb_lock_id, (LK_LOCK_KEY *)NULL,
			(LK_VALUE *)NULL, &sys_err);
		if (cl_status != OK)
		{
		    uleFormat(NULL, cl_status, &sys_err, 
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
                            (i4 *)NULL, err_code, 1, 0, lock_list);
		    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, (CL_ERR_DESC *)NULL,
			    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, 
                            (i4 *)NULL, err_code, 0);
		    status = E_DB_ERROR;
		    SETDBERR(dberr, 0, E_DM9266_UNLOCK_CLOSE_DB);
		    break;
		}
		dcb->dcb_odb_lock_id[0] = 0;
	    }

	    /* Everything went ok */
	    break;
	}

	if (status != E_DB_OK)
	    break;

	dm0m_deallocate((DM_OBJECT **)&dcb);

	return (E_DB_OK);
    }

	
    /*  Handle error recovery.
    **  We try to unravel things in the reverse order in which they were done.
    **  Depending on how far we got, different variables will be set.
    */
	
    if (config)
    {
        /* Close configuration file. */
	status = dm0c_close(cnf, 0, &local_dberr);
        if (status != E_DB_OK)
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 0);
    }

    if (tran_lgid)
    {
	/* Abort transaction in progress. */
	status = dmxe_abort((DML_ODCB *)NULL, dcb, &tran_id, tran_lgid,
		    tran_llid, dmxe_flag,
		    0, 0, &local_dberr);
        if (status != E_DB_OK)
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 0);

    }

    if (dcb && dcb->dcb_log_id) 
    {
	status = dm0l_closedb(dcb->dcb_log_id, &local_dberr);
	dcb->dcb_log_id = 0;    
        if (status != E_DB_OK)
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 0);
    }

    if (dcb && dcb->dcb_odb_lock_id[0]) 
    {
        /* Undo table lock */
        cl_status = LKrelease(0, lock_list,
            (LK_LKID *)dcb->dcb_odb_lock_id, (LK_LOCK_KEY *)NULL,
            (LK_VALUE *)NULL, &sys_err);
	if (cl_status != OK)
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 1, 0, lock_list);
    }

    if (dblkid.lk_unique)
    {
        /* Undo database lock */
        db_key.lk_type = LK_DATABASE;
        cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)NULL,
                        (LK_LOCK_KEY *)&db_key, (LK_VALUE *)NULL, &sys_err);
	if (cl_status != OK)
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 1, 0, lock_list);
    }

    if (dcb)
	dm0m_deallocate((DM_OBJECT **)&dcb);
    if ( raw_array )
	dm0m_deallocate((DM_OBJECT **)&raw_array);

    if (dberr->err_code > E_DM_INTERNAL)
    {
        /*      Log the change in error messages. */

        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, 
	    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
	    &local_error, 0);
	SETDBERR(dberr, 0, E_DM9C92_UNEXTEND_DB);
    }

    return (E_DB_ERROR);     
}


/*{
** Name: dm2d_alter_db - Alter the database
**
** Description:
**	This routine is called from dmm to do the lower level work; currently
**	this routine's main purpose is to make config file changes.  It's kind
**	of ugly: instead of going through normal channels, we're creating a
**	dcb by hand, starting our own transaction, getting our own locks,
**	etc., in order to simulate what normally happens when opening a
**	database.  You see, we're doing something rather unusual: we have the
**	iidbdb currently open and we're in the midst of a transaction when
**	suddenly we need to go and change the config file of another database.
**	So the evil dm2d_extend_db code did all this unorthodox maneuvering
**	which now gets propogated in this routine.  When the config file goes
**	away, we'll be rid of this code.
**
**	Note that sometimes we don't do the logging and locking.  For example,
**	when this routine is called for rollforward we do neither.
**
** Inputs:
**	dm2d				A DM2D_ALTER_INFO control block
**	    .lock_list			The lock list to own the database lock.
**	    .lock_no_wait		boolean: wait for locks?
**	    .logging			boolean: do logging?
**	    .locking			boolean: do locking?
**	    .name			database name
**	    .db_loc			location of db
**	    .l_db_loc			length of db location
**	    .location_name		location we are going to alter
**	    .alter_op			what operation shall we perform
**		DM2D_TAS_ALTER		alter table, access, service fields
**		DM2D_EXT_ALTER		alter config file extent info
**	    .alter_info			a union of the following gunk
**	        .tas_info		a struct for the TAS operation
**		    .db_service		new service code to put in config file
**		    .db_access		new access code to put in config file
**		    .tbl_id		next table id to use in this db
**						OR
**		.ext_info		a struct for extent alteration
**		    .drop_loc_type	what bits to drop in extent flags
**		    .add_loc_type	what bits to add in extent flags
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-oct-1993 (jrb)
**	    Created for MLsorts.
**	31-jan-94 (jnash)
**	    Add E_DM92A2_DM2D_LAST_TBID error.  This is xDEBUG code added
**	    because we don't understand when this condition can legally arise. 
**	14-nov-1995 (angusm)
**		Modify change for 67276 to prevent bug 71652.
**	28-feb-06 (hayke02)
**	    Continue through the location config loop if we have a
**	    drop_loc_type of DCB_WORK or DCB_AWORK, but the location flags
**	    are not DCB_WORK or DCB_AWORK. This will happen if a location
**	    is used for data and work/aux work, and we have found the data
**	    usage config file entry for the location. This change fixes bug
**	    102500, problem INGSRV 1263.
*/
DB_STATUS
dm2d_alter_db(
    DM2D_ALTER_INFO	*dm2d,
    DB_ERROR		*dberr )
{
    i4	    	lock_list = dm2d->lock_list;
    i4		logging = dm2d->logging;
    i4		locking = dm2d->locking;
    DM_SVCB		*svcb = dmf_svcb;
    DMP_DCB		*dcb = 0;
    i4		tran_lgid = 0;
    i4		tran_llid = 0;
    DB_TRAN_ID		tran_id;
    DM0C_CNF		*config = 0;
    DM0C_CNF		*cnf;
    LK_VALUE		odb_k_value;
    LK_VALUE		lock_value;
    LK_LOCK_KEY		db_key;
    LK_LKID		dblkid;
    DB_STATUS		status;
    STATUS      	cl_status;
    CL_ERR_DESC		sys_err;
    i4		i;
    i4		dmxe_flag = DMXE_JOURNAL;
    DB_TAB_TIMESTAMP	ctime;
    i4             local_error;
    DM0C_EXT		*cnf_ext;
    i4	        journal = 0;
    i4		drop_loc_type;
    i4		add_loc_type;
    i4         	cnf_flag = 0;
    i4			*err_code = &dberr->err_code;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);
 
    MEfill(sizeof(LK_LKID), 0, &dblkid);
    for (;;)
    {
	 /* Allocate a DCB for this database. */

	/* Use ShortTerm memory */
	status = dm0m_allocate((i4)sizeof(DMP_DCB),
	    (DM0M_ZERO | DM0M_SHORTTERM),
	    (i4)DCB_CB, (i4)DCB_ASCII_ID, (PTR)svcb,
	    (DM_OBJECT **)&dcb, dberr);
	if (status != E_DB_OK)
	    break;

	/*  Copy root location name into the DCB. */

	STRUCT_ASSIGN_MACRO(*dm2d->name, dcb->dcb_name);
	dcb->dcb_location.phys_length = dm2d->l_db_loc;
	MEmove(dm2d->l_db_loc, dm2d->db_loc, ' ',
	    sizeof(dcb->dcb_location.physical),
	    (PTR)&dcb->dcb_location.physical);
	dcb->dcb_location.flags = 0;
	dcb->dcb_odb_lock_id[0] = 0;
	dcb->dcb_log_id = 0;
	    
	/*  Initialize the lock key. */

	MEcopy((PTR)&dcb->dcb_name, LK_KEY_LENGTH, (PTR)&db_key.lk_key1);
	db_key.lk_type = LK_DATABASE;

	/*  Lock the database for exclusive access. */

	if (locking)
	{
	    cl_status = LKrequest(LK_PHYSICAL |
		(dm2d->lock_no_wait  ?  LK_NOWAIT  :  0), lock_list,
		(LK_LOCK_KEY *)&db_key, LK_X, (LK_VALUE *)NULL,
		(LK_LKID *)&dblkid, (i4)0, &sys_err);

	    if (cl_status == LK_BUSY)
	    {
		SETDBERR(dberr, 0, E_DM004C_LOCK_RESOURCE_BUSY);
		break;
	    }

	    if (cl_status == OK)
	    {
		/* Get the dbopen lock, used to determine if 
		** this is the first open of this database in any server. 
		*/
		db_key.lk_type = LK_OPEN_DB;
		cl_status = LKrequest(LK_PHYSICAL, lock_list, &db_key, LK_X,
			&odb_k_value, (LK_LKID *)dcb->dcb_odb_lock_id,
			(i4)0, &sys_err); 

                /* If the lock wasn't valid treat it as a success, we'll be updating the
                ** lock value when we release it.
                */
                if (cl_status == LK_VAL_NOTVALID)
                {
                   cl_status = OK;
                }
	    }

	    if (cl_status != OK) 
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL, 
                    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_list);
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, (CL_ERR_DESC *) 0,
		    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
                    err_code, 0);
		SETDBERR(dberr, 0, E_DM9269_LOCK_OPEN_DB);
		status = E_DB_ERROR;
		break;
	    } 
	}

	/*  Open the configuration file. */
	
	if (dm2d->alter_info.tas_info.db_access & DU_RDONLY)
	    cnf_flag |= DM0C_READONLY;

	status = dm0c_open(dcb, cnf_flag, lock_list, &cnf, dberr);
	if (status != E_DB_OK)
	    break;
	config = cnf;

	/* Set the unique identifier for the database. */

	dcb->dcb_id = cnf->cnf_dsc->dsc_dbid;
	dcb->dcb_db_owner = cnf->cnf_dsc->dsc_owner;
	if ((cnf->cnf_dsc->dsc_status & DSC_JOURNAL) == 0)
	    dmxe_flag = 0;

	status = dm0c_close(cnf, 0, dberr);
	if (status != E_DB_OK)
	    break;
	config = 0;

	/*
	** Now convert the database open lock back to null after incrementing
        ** the value.
	*/

	if (locking)
	{
	    odb_k_value.lk_low++;
	    cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT, lock_list, 0,
		LK_N, (LK_VALUE * )&odb_k_value,
		(LK_LKID*)dcb->dcb_odb_lock_id, (i4)0, &sys_err); 

	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			    (char *)NULL, 0L, (i4 *)NULL, err_code, 0);
		uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG, 
                    (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, err_code, 1, 0, lock_list); 
		SETDBERR(dberr, 0, E_DM9269_LOCK_OPEN_DB);
		status = E_DB_ERROR;
		break;
	    }
	}

	if (logging)
	{
	    i4	open_flag = 0;

	    if (dmxe_flag & DMXE_JOURNAL)
	        open_flag |= DM0L_JOURNAL;
	    status = dm0l_opendb(svcb->svcb_lctx_ptr->lctx_lgid,
		    open_flag, &dcb->dcb_name, &dcb->dcb_db_owner, 
		    dcb->dcb_id, &dcb->dcb_location.physical,
		    dcb->dcb_location.phys_length, &dcb->dcb_log_id,
		    (LG_LSN *)NULL,
		    dberr);
	    if (status != E_DB_OK)
	        break;

	    /* Begin a transaction. */

	    status = dmxe_begin(DMXE_WRITE, dmxe_flag, dcb, dcb->dcb_log_id,
		&dcb->dcb_db_owner, lock_list, &tran_lgid, &tran_id,
		&tran_llid, (DB_DIS_TRAN_ID *)NULL, dberr);
	    if (status != E_DB_OK)
	        break;
	}

	status = dm0c_open(dcb, cnf_flag, lock_list, &cnf, dberr);
	if (status != E_DB_OK)
	    break;
	config = cnf;

	switch (dm2d->alter_op)
	{
	  case DM2D_TAS_ALTER:
            cnf->cnf_dsc->dsc_dbaccess =
	        dm2d->alter_info.tas_info.db_access;

            cnf->cnf_dsc->dsc_dbservice =
		dm2d->alter_info.tas_info.db_service;

            if (dm2d->alter_info.tas_info.tbl_id)
	    {
#ifdef xDEBUG
		if (cnf->cnf_dsc->dsc_last_table_id != 
		    dm2d->alter_info.tas_info.tbl_id)
		{
		    uleFormat(NULL, E_DM92A2_DM2D_LAST_TBID, 0, ULE_LOG, 
                        (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 3,
			sizeof(DB_DB_NAME), &dm2d->name,
			0, cnf->cnf_dsc->dsc_last_table_id, 
			0, dm2d->alter_info.tas_info.tbl_id);
	 	}
#endif

                cnf->cnf_dsc->dsc_last_table_id = 
		    dm2d->alter_info.tas_info.tbl_id;
	    }
	    break;

	  case DM2D_EXT_ALTER:
	    drop_loc_type = dm2d->alter_info.ext_info.drop_loc_type;
	    add_loc_type = dm2d->alter_info.ext_info.add_loc_type;

	    if (logging)
	    {
		if (dmxe_flag & DMXE_JOURNAL)
		    journal = DM0L_JOURNAL;
		status = dm0l_ext_alter(tran_lgid, journal, drop_loc_type,
		    add_loc_type, dm2d->location_name, (LG_LSN *)NULL, dberr);
		if (status != E_DB_OK)
		    break;
	    }

	    cnf_ext = cnf->cnf_ext;

	    /* Find the corresponding location in the config file...*/
	    for (i = cnf->cnf_c_ext; i > 0; i--, cnf_ext++)
	    {
		/*
		**  Bug B67276
		**  Only look at DATA locations
		*/
		/*
		** bug 71652:
		**  but only do this if we aren't changing WORK->AUX WORK
		**	or vice versa
		*/

		if (((drop_loc_type != DCB_WORK) && (add_loc_type != DCB_AWORK)) &&
		   ((drop_loc_type != DCB_AWORK) && (add_loc_type != DCB_WORK))) 
			if ((cnf_ext->ext_location.flags & DCB_DATA) == 0)
	    	    	continue;

		if (MEcmp((PTR)&cnf_ext->ext_location.logical,
			dm2d->location_name->db_loc_name,
			sizeof(DB_LOC_NAME)) == 0
		   )
		{
		    if ((drop_loc_type == DCB_WORK
			&&
			!(cnf_ext->ext_location.flags & DCB_WORK))
			||
			(drop_loc_type == DCB_AWORK
			&&
			!(cnf_ext->ext_location.flags & DCB_AWORK)))
			continue;
		    else
			break;
		}
	    }

	    if (i > 0)
	    {
		i4     tflags = cnf_ext->ext_location.flags;

		/*
		** It would make sense here to check to ensure that the
		** drop_loc_type is really set in the config file before
		** dropping it, but we do not do this.  Here is the reason:
		** there are cases where the iiextend catalog (in the iidbdb)
		** and the config file extent bits can become out of sync;
		** it's easy to cause this in fact; one way is to run the
		** iiqef_alter_extension procedure and then abort it.  The
		** abort will rollback the iiextend update, but not the
		** update to the config file (because that transaction gets
		** internally committed inside dm2d_alter_db).
		**
		** So we do not generate an error here, but we do log some-
		** thing to indicate that things had gotten out of sync.
		*/ 
		if ((tflags & drop_loc_type) == 0)
		{
		    /* Error: trying to drop an attribute which the extent
		    ** does not have.  Log a msg, but continue anyway.
		    */
		    uleFormat(NULL, E_DM92A1_DM2D_EXT_MISMATCH, 0, ULE_LOG, 
                        (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, err_code, 1,
			sizeof(dm2d->location_name->db_loc_name),
			dm2d->location_name->db_loc_name);
		}

		/* drop old bit(s) and set new one(s) */
		cnf_ext->ext_location.flags =
		    (tflags & ~drop_loc_type) | add_loc_type;
	    }
	    else
	    {
		/* Inconsistency: caller thinks database is extended to
		** this location, but the location isn't in the config
		** file.
		*/
		SETDBERR(dberr, 0, E_DM013D_LOC_NOT_EXTENDED);
		status = E_DB_ERROR;
		break;
	    }
	}
	if (status != E_DB_OK)
	    break;

	/*  Close the configuration file. */

	status = dm0c_close(cnf, DM0C_UPDATE | DM0C_COPY, dberr);
	if (status != E_DB_OK)
	    break;
	config = 0;
	
	if (logging)
	{
	    status = dmxe_commit(&tran_id, tran_lgid, tran_llid, dmxe_flag,
		    &ctime, dberr);
	    if (status != E_DB_OK)
		break;
	    tran_lgid = 0;

	    status = dm0l_closedb(dcb->dcb_log_id, dberr);
	    dcb->dcb_log_id = 0;    
	    if (status != E_DB_OK)
		break;
	}

	/* Unlock database lock and opendb lock */

	if (locking)
	{
	    cl_status = LKrelease(0, lock_list,
		(LK_LKID *)dcb->dcb_odb_lock_id, (LK_LOCK_KEY *)NULL,
		(LK_VALUE *)NULL, &sys_err);
	    if (cl_status != OK)
	    {
		uleFormat(NULL, cl_status, &sys_err, 
		    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    err_code, 1, 0, lock_list);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, (CL_ERR_DESC *)NULL,
		   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		   err_code, 0);
		status = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM9266_UNLOCK_CLOSE_DB);
		break;
	    }
	    dcb->dcb_odb_lock_id[0] = 0;

	    db_key.lk_type = LK_DATABASE;
	    cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)NULL, 
		(LK_LOCK_KEY *)&db_key, (LK_VALUE *)NULL, &sys_err);
	    if (cl_status != OK)
	    { 
		uleFormat(NULL, cl_status, &sys_err, 
		       ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		       err_code, 1, 0, lock_list);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, (CL_ERR_DESC *)NULL,
		      ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		      err_code, 0);
		status = E_DB_ERROR;
		SETDBERR(dberr, 0, E_DM9266_UNLOCK_CLOSE_DB);
		break;
	    }
	    dblkid.lk_unique = 0;
	}

	/* Everything went ok */
	dm0m_deallocate((DM_OBJECT **)&dcb);

	return (E_DB_OK);
    }

    /*  Handle error recovery.
    **  We try to unravel things in the reverse order in which they were done.
    **  Depending on how far we got, different variables will be set.
    */

    if (config)
    {
        /* Close configuration file. */
	status = dm0c_close(cnf, 0, &local_dberr);
        if (status != E_DB_OK)
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 0);
    }

    if (tran_lgid)
    {
	/* Abort transaction in progress. */
	status = dmxe_abort((DML_ODCB *)NULL, dcb, &tran_id, tran_lgid,
		    tran_llid, dmxe_flag,
		    0, 0, &local_dberr);

        if (status != E_DB_OK)
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL,
                   ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 0);
    }

    if (dcb && dcb->dcb_log_id) 
    {
	status = dm0l_closedb(dcb->dcb_log_id, &local_dberr);
	dcb->dcb_log_id = 0;    

        if (status != E_DB_OK)
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
                (DB_SQLSTATE *)NULL, (char *)NULL,
		(i4)0, (i4 *)NULL, &local_error, 0);
    }

    if (locking && dcb && dcb->dcb_odb_lock_id[0]) 
    {
        cl_status = LKrelease(0, lock_list, (LK_LKID *)dcb->dcb_odb_lock_id,
	    (LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);

	if (cl_status != OK)
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err, ULE_LOG, 
                (DB_SQLSTATE *)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 1, 0,
		lock_list);
    }

    if (locking && dblkid.lk_unique)
    {
        /* Undo database lock */

        db_key.lk_type = LK_DATABASE;
        cl_status = LKrelease((i4)0, lock_list, (LK_LKID *)NULL,
                        (LK_LOCK_KEY *)&db_key, (LK_VALUE *)NULL, &sys_err);

	if (cl_status != OK)
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err, ULE_LOG, 
                   (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                   &local_error, 1, 0, lock_list);
    }

    if (dcb)
	dm0m_deallocate((DM_OBJECT **)&dcb);

    if (dberr->err_code > E_DM_INTERNAL)
    {
        /*      Log the change in error messages. */

        uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, 
	    ULE_LOG, (DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
	    &local_error, 0);
	SETDBERR(dberr, 0, E_DM929F_DM2D_ALTER_DB);
    }

    return (E_DB_ERROR);
}

/*{
** Name: construct_tcb	- This routine constructs a TCB from a config 
**	description.
**
** Description:
**      This routine takes a configuration file relation description record
**	for a system table, and builds the TCB and FCB for the system table
**	and inserts them into the appropriate spots.
**
** Inputs:
**      tab                             Pointer to DM0C_REL.
**	dcb				The DCB for these tables.
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
**      12-nov-1986 (Derek)
**          Created for Jupiter.
**      25-nov-87 (jennifer)
**          Add multiple locations per table support.
**	18-oct-88 (mmm)
**	    added "sync_flag" parameter to dm2f_build_fcb() calls.
**	16-jan-89 (mmm)
**	    Reversed logic of sync flag in config file so that by default
**	    a database will be "sync'd".
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateways.  Define tcb fields in
**	    system catalog TCB's that define logging actions, access methods
**	    to use, secondary index update semantics, and TID support.
**      25-apr-1990 (fred)
**          Added initialization of new tcb fields.  In particular,
**	    tcb_et_list (empty).
**	 4-feb-1991 (rogerk)
**	    Added support for fixed table priorities.
**	    Added tcb_bmpriority field.  Call buffer manager to get table
**	    cache priority when tcb is built.
**	25-feb-1991 (rogerk)
**	    Changed dm0p_gtbl_priority call to take a regular DB_TAB_ID.
**	 7-mar-1991 (bryanp)
**	    Added support for Btree index compression: new fields tcb_data_atts
**	    and tcb_comp_katts_count in the DMP_TCB. The core system catalogs
**	    don't use key compression, so tcb_comp_katts_count is always 0.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**		Changes for file extend project.  Check for uninitialized
**		file extend catalog fields (relfhdr,relallocation,relextend).
**		If uninitialized, then set them to default values.  Also set
**		the defaults in the relation structure passed in from the
**		config file.  Return E_DB_INFO to indicate that the config
**		file should be updated to reflect this new information.
**
**		Added new DI_ZERO_ALLOC file open flag.  This specifies
**		that disk space must be actually allocated at DIalloc time
**		rather than just reserved.  This flag is required for
**		ingres database tables.
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    construct_tcb now uses the compile time information to build TCB's.
**	    The config file is not used.
**	07-july-1992 (rmuth)
**	    Routine name changed to construct_tcb.
**      29-August-1992 (rmuth)
**          Removing the file extend 6.4->6.5 on-the-fly conversion code
**          means we can take out all the code which initialised the new file
**          extend  fields, ( relfhdr, relallocation and relextend ).
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Call dm1c_getaccessors to initialise the files
**	    accessors. i39_vme needs special code for forward compatibility.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	    Also initialized some of the new table_io fields so debug/trace
**	    routines won't dump garbage.
**	16-oct-1992 (rickh)
**	    Initialize attribute defaults.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project.  Added support for Partial TCB's.
**	    Filled in new table_io fields of tcb.  Changed dm2f_build_fcb
**	    to take table_io argument rather than a tcb.  Filled in new
**	    tcb_valid_count field.  Initialize new loc_config_id field in
**	    table's location array.
**	30-October-1992 (rmuth)
**	    - Remove references to file open flag DI_ZERO_ALLOC.
**	    - Initialise tbio_lalloc and tbio_lpageno to -1.
**	    - Initialise tbio_tmp_hw_pageno.
**	02-nov-1992 (kwatts)
**	    Remove the special VME compatibility code. It
**	    is not needed now we don't get table descriptions from config
**	    files.
**	24-nov-92 (rickh)
**	    Recast default ids as table ids.  Most excellent code hygeine.
**	14-dec-1992 (jnash)
**	    Reduced Logging Project (phase 2).  Turn on tcb_logging field for
**	    index tcbs now - Index updates are now logged.
**	18-jan-1993 (rogerk)
**	    Enabled recovery flag in tbio in CSP as well as RCP since the
**	    DCB_S_RCP status is no longer turned on in the CSP.
**      16-feb-1993 (rogerk)
**	    Added new tcb_open_count field.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**		Remove DM2D_CSP and DCB_S_CSP status flags, since it's no longer
**		    required that a DCB be flagged with whether it was built by
**		    a CSP process or not. This more or less reverses Roger's
**		    18-jan-1993 change; the DCB_S_RCP status flag *is* turned
**		    on when the CSP process invokes recovery code.
**	26-may-1993 (robf)
**	    Remove ORANGE code.
**	    Build tcb_sec_lbl_att/sec_key_att pointers
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**      20-jun-1994 (jnash)
**          fsync project.  
**	    - Initialize tbio_sync_close.  Tables built by this
**	      routine are synchronously updated.
**	    - Eliminate obsolete tcb_logical_log.
**	06-mar-1996 (stial01 for bryanp)
**	    If relpgsize is 0 for core catalogs, force it to be DM_PG_SIZE.
**	    Set tbio_page_size from tcb_rel.relpgsize.
**      06-mar-1996 (stial01)
**          Pass page size to dm2f_build_fcb
**	06-may-1996 (thaju02)
**	    Pass page size to dm1c_getaccessors().
**      18-jul-1996 (ramra01 for bryanp)
**          Build new DB_ATTS fields for Alter Table support.
**          Set up tcb_total_atts when
**              building TCBs for core system catalogs. This implementation
**              assumes that core system catalogs cannot be altered, and
**              thus there are never any dropped or modified attribute records
**      01-aug-1996 (nanpr01 for ICL phil.p)
**          If DB opened with DB protocol, mark TCBs as supporting it.
**	15-Oct-1996 (jenjo02)
**	    Compose tcb_mutex, tcb_et_mutex name using table name to uniquify for 
**	    iimonitor, sampler.
**	    Initialize tcb_hash_bucket_ptr with correct hash value for table.
**	21-Jan-1997 (jenjo02)
**	    Call dm2t_alloc_tcb() to allocate and initialize TCB instead of
**	    doing it ourselves.
** 	19-jun-1997 (wonst02)
** 	    Added DM2F_READONLY flag for dm2f_build_fcb (readonly database).
**      28-may-1998 (stial01)
**          construct_tcb() Added recovery of DM0LSM1RENAME: check for DM2F_SM1 
**      12-apr-1999 (stial01)
**          construct_tcb() Changed parameters to dm2t_alloc_tcb
** 	21-may-1999 (wonst02)
** 	    Pass "tcb" to dm0m_deallocate() instead of "&t" if an error, 
** 	    so the caller's variable gets zeroed.
**      28-may-1998 (stial01)
**          construct_tcb() Added recovery of DM0LSM1RENAME: check for DM2F_SM1 
**      12-apr-1999 (stial01)
**          construct_tcb() Changed parameters to dm2t_alloc_tcb
**	15-jul-1999 (nanpr01)
**	    On error,  we should deallocate the tcb and set it to nil instead
**	    of setting the local variable to nil.
**	16-Jan-2004 (schka24)
**	    Let caller init hash pointers.
**	6-Feb-2004 (schka24)
**	    Update call to build-fcb.
**	14-dec-04 (inkdo01)
**	    Support attcollID in iiattribute.
**	29-Sept-2009 (troal01
**		Add support for geospatial.
**	23-Oct-2009 (kschendel) SIR 122739
**	    Set up rowaccessor before using the tcb.
*/
static DB_STATUS
construct_tcb(
    i4		lock_list,
    DMP_RELATION	*relation,
    DMP_ATTRIBUTE	*attribute,
    DMP_DCB		*dcb,
    DMP_TCB		**tcb,
    DB_ERROR		*dberr )
{
    DMP_RELATION    *rel = relation;
    DMP_ATTRIBUTE   *att = attribute;
    DMP_TCB	    *t;
    i4	    i;
    i4	    offset = 0;
    DB_STATUS	    status;
    DB_STATUS	    loc_status;
    u_i4	    db_sync_flag = 0;
    u_i4	    total_pages;
    i4		    *err_code = &dberr->err_code;
    DB_ERROR	    local_dberr;
    i4		    alen;
    PTR			nextattname;

    CLRDBERR(dberr);

    status = dm2t_alloc_tcb(rel, (i4)1, dcb, tcb, dberr);
    if (status != E_DB_OK)
	return (status);

    t = *tcb;

    /* Override default TCB values */
    t->tcb_ref_count = 1;
    t->tcb_valid_count = 1;
    t->tcb_open_count = 1;
    t->tcb_index_count = 0;
    t->tcb_status = TCB_FREE;
    t->tcb_sysrel = TCB_SYSTEM_REL;

    if (rel->reltid.db_tab_base == 1 && rel->reltid.db_tab_index == 0)
	t->tcb_index_count = 1;

    /*
    ** Set index attributes for iirel_idx.  Account for security labels
    ** on secure databases.
    */

    if (rel->reltid.db_tab_index == 2)
        t->tcb_ikey_map[0] = 3, t->tcb_ikey_map[1] = 4;

    STRUCT_ASSIGN_MACRO (dcb->dcb_root->logical, t->tcb_rel.relloc);

    /* 
    ** Note: since initial Jupiter databases, configuration files
    ** etc, did not support multiple locations, they to not
    ** have their relloccount value initialized.  So do here
    ** just in case. 
    */

    if (((t->tcb_rel.relstat & TCB_MULTIPLE_LOC) == 0) &&
	(t->tcb_rel.relloccount != 1))
    {
	t->tcb_rel.relloccount = 1;
    }

    /*
    ** This might eventually be replaced by database updgrade software, but
    ** for now, if relpgsize is not set, force it to be DM_PG_SIZE.
    */
    if (t->tcb_rel.relpgsize == 0)
	t->tcb_rel.relpgsize = DM_PG_SIZE;

    /*
    ** Initialize Table IO Control Block.	
    */
    t->tcb_table_io.tbio_sync_close = FALSE;
    t->tcb_table_io.tbio_sysrel = TCB_SYSTEM_REL;

    /*
    ** Note there can only be one location for sconcur system catalogs.
    */
    t->tcb_table_io.tbio_location_array[0].loc_status = LOC_VALID;
    t->tcb_table_io.tbio_location_array[0].loc_ext = dcb->dcb_root;
    t->tcb_table_io.tbio_location_array[0].loc_fcb = 0;
    t->tcb_table_io.tbio_location_array[0].loc_id = 0;
    t->tcb_table_io.tbio_location_array[0].loc_config_id = 0;
    STRUCT_ASSIGN_MACRO(t->tcb_dcb_ptr->dcb_root->logical, 
			    t->tcb_table_io.tbio_location_array[0].loc_name);

    nextattname = t->tcb_atts_names;
    for (i = 0; i < rel->relatts; i++)
    {
	for (alen = DB_ATT_MAXNAME;  
	    att[i].attname.db_att_name[alen-1] == ' ' && alen >= 1; alen--);

	t->tcb_atts_ptr[i + 1].attnmstr = nextattname; 
	t->tcb_atts_ptr[i + 1].attnmlen = alen;
	MEcopy(att[i].attname.db_att_name, alen, 
	    t->tcb_atts_ptr[i + 1].attnmstr);
	nextattname += alen;
	*nextattname = '\0';
	nextattname++;
	t->tcb_atts_used += (alen + 1);

	t->tcb_atts_ptr[i + 1].offset = att[i].attoff;
	t->tcb_atts_ptr[i + 1].type = att[i].attfmt;
	t->tcb_atts_ptr[i + 1].length = att[i].attfml;
	t->tcb_atts_ptr[i + 1].precision = att[i].attfmp;
 	t->tcb_atts_ptr[i + 1].key = att[i].attkey;
	t->tcb_atts_ptr[i + 1].flag = att[i].attflag;
	COPY_DEFAULT_ID( att[i].attDefaultID, 
					t->tcb_atts_ptr[i + 1].defaultID );
	t->tcb_atts_ptr[i + 1].defaultTuple = 0;
	t->tcb_atts_ptr[i + 1].replicated = FALSE;
	t->tcb_atts_ptr[i + 1].key_offset = 0;
	t->tcb_atts_ptr[i + 1].collID = att[i].attcollID;
	t->tcb_atts_ptr[i + 1].geomtype = att[i].attgeomtype;
	t->tcb_atts_ptr[i + 1].srid = att[i].attsrid;
	t->tcb_atts_ptr[i + 1].intl_id = att[i].attintl_id;
	t->tcb_atts_ptr[i + 1].ordinal_id = att[i].attid;
	t->tcb_atts_ptr[i + 1].ver_added = att[i].attver_added;
	t->tcb_atts_ptr[i + 1].ver_dropped = att[i].attver_dropped;
	t->tcb_atts_ptr[i + 1].val_from = att[i].attval_from;
	t->tcb_atts_ptr[i + 1].ver_altcol = att[i].attver_altcol;
	t->tcb_atts_ptr[i + 1].encflags = att[i].attencflags;
	t->tcb_atts_ptr[i + 1].encwid = att[i].attencwid;

	t->tcb_data_rac.att_ptrs[i] = &(t->tcb_atts_ptr[i + 1]);

	if (att[i].attkey)
	{
	    t->tcb_att_key_ptr[att[i].attkey - 1] = i + 1;
	    t->tcb_key_atts[att[i].attkey - 1] = &t->tcb_atts_ptr[i+1];
	    t->tcb_atts_ptr[i + 1].key_offset = offset;
	    t->tcb_klen += att[i].attfml;
	    offset += att[i].attfml;
	    t->tcb_keys++;
	}

	if (att[i].attflag & ATT_SEC_KEY)
	    t->tcb_sec_key_att = &t->tcb_atts_ptr[i+1];
    }

    if (t->tcb_atts_used > t->tcb_atts_size)
    {
	/* should NEVER happen */
	SETDBERR(dberr, 0, E_DM0075_BAD_ATTRIBUTE_ENTRY);
	return(E_DB_ERROR);
    }

    /* Do rowaccess "setup" -- data only, core catalogs don't use
    ** btree or isam.
    */
    status = dm1c_rowaccess_setup(&t->tcb_data_rac);
    if (status != E_DB_OK)
	return (status);

    /*
    ** Set DI flags : default is DI_ZERO_ALLOC and DI_SYNC_MASK.
    */
    if ((t->tcb_dcb_ptr->dcb_sync_flag & DCB_NOSYNC_MASK) == 0)
	db_sync_flag |= (u_i4) DI_SYNC_MASK;

    status = dm2f_build_fcb(
	lock_list, (i4)0, 
	t->tcb_table_io.tbio_location_array, 
	t->tcb_table_io.tbio_loc_count,
	t->tcb_table_io.tbio_page_size,
        (dcb->dcb_access_mode == DCB_A_READ) ? DM2F_OPEN | DM2F_READONLY : 
					       DM2F_OPEN, 
	DM2F_TAB, 
	&rel->reltid, 
        0, 0,
	&t->tcb_table_io, 
	db_sync_flag, 
	dberr);

    if (status == E_DB_ERROR && dberr->err_code == E_DM0114_FILE_NOT_FOUND)
    {
	DB_STATUS       local_status;
	i4         local_error;
	DM_FILE         tabname, sm1name;

	/* Check for existence of SYSMOD temp file and rename it */
	dm2f_filename(DM2F_TAB, &rel->reltid, 0,0,0, &tabname);
	dm2f_filename(DM2F_SM1, &rel->reltid, 0,0,0, &sm1name);
	local_status = dm2f_rename(lock_list, (i4)0,
		&t->tcb_dcb_ptr->dcb_root->physical, 
		t->tcb_dcb_ptr->dcb_root->phys_length, 
		&sm1name, sizeof(DM_FILE), &tabname, sizeof(DM_FILE),
		&dcb->dcb_name, &local_dberr);
	if (local_status == E_DB_OK)
	{
	    /* Try open again */
	    status = dm2f_build_fcb(
		lock_list, (i4)0, 
		t->tcb_table_io.tbio_location_array, 
		t->tcb_table_io.tbio_loc_count,
		t->tcb_table_io.tbio_page_size,
		(dcb->dcb_access_mode == DCB_A_READ) ? 
				    DM2F_OPEN | DM2F_READONLY : DM2F_OPEN,
		DM2F_TAB, 
		&rel->reltid, 
		0, 0,
		&t->tcb_table_io, 
		db_sync_flag, 
		dberr);
	}
    }
    
    if (status == E_DB_OK)
    {
	t->tcb_table_io.tbio_status |= TBIO_OPEN; 
	return( E_DB_OK );
    }
 
    dm0m_deallocate((DM_OBJECT **)tcb);
    return (status);
}

/*{
** Name: update_tcb - Update TCB information on system catalogs from database.
**
** Description:
**	This routine updates the tcb_rel information of the TCB using
**	the tuple in the iirelation catalog.  It is used to get more
**	accurate information in the TCB's of the core system catalogs which
**	are built from compile time information.
**
** Inputs:
**      tcb                             The base TCB to update.
**	flag				Is this the RCP process?
**	lock_list			Lock list to use for page locking.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	If unitialized fields are found in the iirelation structure, they
**	may be automatically initialized and updated in the iirelation table.
**	This is currently done for file extend fields.
**
** History:
**       7-nov-1988 (rogerk)
**	    Created.
**	10-feb-1989 (rogerk)
**	    Added setting of relstat value.
**	17-oct-1991 (rogerk)
**	    Great Merge of DMF : Move 6.4 changes into 6.5.  In this case,
**	    the 6.4 version of this file was moved directly to 6.5 and the
**	    6.5 changes were merged back in:
**
**		Added rcb_internal_req flag to indicate that queries on the
**		rcb are being done by DMF itself, so security checks can
**		be bypassed.
**
**		Added automatic initialization of iirelation fields used
**		for file extend project.
**	05-may-1992 (ananth)
**	    Increasing IIrelation tuple size project.
**	    Get tuple from iirelation table and insert into the TCB.
**      29-August-1992 (rmuth)
**          Removing the file extend 6.4->6.5 on-the-fly conversion code
**          means we can take out all the code which initialised the new file
**          extend  fields, ( relfhdr, relallocation and relextend ).
**	26-oct-1992 (rogerk)
**	    Added check for relloccount != 1 for core catalogs in the
**	    iirelation table and corrected the value if needed.  Old databases
**	    did not have this value initialized.
**	02-apr-1993 (ralph)
**	    DELIM_IDENT
**	    Update the attribute name field for each DB_ATTS entry on the
**	    TCB in update_tcb() to ensure it is in the proper character case.
**	24-may-1993 (bryanp)
**	    Correct argument in call to dm2r_release_rcb
**	17-may-1993 (ralph)
**	    DELIM_IDENT:
**	    Move code that updates attribute name fields in TCB to
**	    update_tcbatts().
**	21-jun-1993 (rogerk)
**	    Re-Correct argument in call to dm2r_release_rcb
**	23-aug-1993 (rogerk)
**	    Changed DM2D_RCP to DM2D_ONLINE_RCP.
**	06-mar-1996 (stial01 for bryanp)
**	    Use tcb_rel.relpgsize, not sizeof(DMPP_PAGE) for variable page sizes
**      06-mar-1996 (stial01)
**          update_tcb: If relpgsize is 0 for core catalogs, force it to be
**          DM_PG_SIZE.
**	17-Jan-2004 (schka24)
**	    Initialize the iirelation tid for the catalogs.
**	18-Mar-2004 (jenjo02)
**	    Sanity check iirelation relatts,relkeys vs what's in
**	    the TCB, just in case.
**	18-Dec-2007 (kschendel) SIR 122890
**	    Force iiattribute and iirelation to "duplicates allowed".
**	    This is 99% useless, as dmu does explicit dups-allowed puts
**	    when creating tables, but we might as well cover any other
**	    cases.  (New db's are created with these tables defined as
**	    duplicates instead of noduplicates.)
*/
static DB_STATUS
update_tcb(
    DMP_TCB	    *tcb,
    i4	    lock_list,
    i4	    flag,
    DB_ERROR	    *dberr )
{
    DB_STATUS		status;    
    DMP_RCB		*rcb = NULL;
    DM2R_KEY_DESC	qual_list[2];
    DMP_RELATION	relrecord;
    DB_TRAN_ID		tran_id;
    DM_TID		tid;
    DMP_ATTRIBUTE	iiatt_record;
    DB_STATUS		local_status;
    i4		local_error;
    i4		att_count = 0;
    i4		    *err_code = &dberr->err_code;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (tcb->tcb_type != TCB_CB)
	dmd_check(E_DM9325_DM2T_UPDATE_REL);
#endif

    tran_id.db_high_tran = 0;
    tran_id.db_low_tran = 0;

    for (;;)
    {
	status = dm2r_rcb_allocate(
	    tcb->tcb_dcb_ptr->dcb_rel_tcb_ptr, 
	    0,
	    &tran_id, 
	    lock_list, 
	    0, 
	    &rcb, 
	    dberr);

	if (status != E_DB_OK)
	    return (status);

	rcb->rcb_lk_id = lock_list;
	rcb->rcb_logging = 0;
	rcb->rcb_journal = 0;

	/* 
	** If this is the RCP process, do read nolocking. Failed server(s)
	** might be holding locks on the system catalog pages.
	*/

	if  ((flag & DM2D_ONLINE_RCP) == 0)
	{
	    rcb->rcb_k_duration |= RCB_K_TEMPORARY;
	}
	else
	{
	    rcb->rcb_k_duration |= RCB_K_READ;
            rcb->rcb_k_type = rcb->rcb_lk_type = RCB_K_TABLE;

            /*
            **  Allocate a MISC_CB to hold the two pages needed to support
            **  readlock=nolock.
            */

            status = dm0m_allocate(
		sizeof(DMP_MISC) + tcb->tcb_rel.relpgsize * 2,
                (i4)0, 
		(i4)MISC_CB, 
		(i4)MISC_ASCII_ID,
                (char *)rcb, 
		(DM_OBJECT **)&rcb->rcb_rnl_cb, 
		dberr);

            if (status != E_DB_OK)
		break;
	    rcb->rcb_rnl_cb->misc_data = (char*)rcb->rcb_rnl_cb +
			sizeof(DMP_MISC);
	} 

	rcb->rcb_internal_req = RCB_INTERNAL;

	qual_list[0].attr_number = DM_1_RELATION_KEY;
	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_value = (char*) &tcb->tcb_rel.reltid.db_tab_base;

	status = dm2r_position(
	    rcb, 
	    DM2R_QUAL, 
	    qual_list, 
	    (i4)1,
	    (DM_TID *)NULL, 
	    dberr);
	if (status != E_DB_OK)
	    break;

	for (;;)
	{
	    status = dm2r_get(
		rcb, &tid, DM2R_GETNEXT, (char *)&relrecord, dberr);

	    if (status != E_DB_OK)
		break;

	    if	((relrecord.reltid.db_tab_base == 
		    tcb->tcb_rel.reltid.db_tab_base) &&
		(relrecord.reltid.db_tab_index == 
		    tcb->tcb_rel.reltid.db_tab_index))
	    {
		/* 
		** Copy the tuple into the TCB. After this the TCB is 
		** completely current.
		*/

		/*
		** Sanity check for wacko attribute information.
		*/
		if ( relrecord.relatts > tcb->tcb_rel.relatts ||
		     relrecord.relkeys > tcb->tcb_rel.relkeys )
		{
		    /* XXXX Add better error message */
		    SETDBERR(dberr, 0, E_DM0075_BAD_ATTRIBUTE_ENTRY);
		    status = E_DB_ERROR;
		    break;
		}

		STRUCT_ASSIGN_MACRO(relrecord, tcb->tcb_rel);

		/*
		** Until database conversion is in place, all tables created 
		** before the multiple page size project have a 0 in relpgsize.
		** Treat this as meaning DM_PG_SIZE.
		*/
		if (tcb->tcb_rel.relpgsize == 0)
		    tcb->tcb_rel.relpgsize = DM_PG_SIZE;

		/* 
		** Since initial Jupiter databases, configuration files
		** etc, did not support multiple locations, they to not
		** have their relloccount value initialized.  So do here
		** just in case. 
		*/
		if (((tcb->tcb_rel.relstat & TCB_MULTIPLE_LOC) == 0) &&
		    (tcb->tcb_rel.relloccount != 1))
		{
#ifdef xDEBUG
		    TRdisplay("Warning: iirelation row for single location\n");
		    TRdisplay("  table (%t) has an invalid relloccount field\n",
			sizeof(tcb->tcb_rel.relid), 
			tcb->tcb_rel.relid.db_tab_name);
		    TRdisplay("  Setting relloccount value to 1\n");
#endif
		    tcb->tcb_rel.relloccount = 1;
		}

		/* For iirelation and iiattribute, turn on duplicates
		** allowed.  NODUPLICATES is just a waste of time.  Anyway,
		** dmu does explicit no-dup-check puts when creating.
		** Get iirel_idx and iiindex as well.
		** (Note: new db's are now created with noduplicates on
		** these catalogs.  This is just for old db's.)
		*/
		if (relrecord.reltid.db_tab_base == DM_B_RELATION_TAB_ID
		  || relrecord.reltid.db_tab_base == DM_B_ATTRIBUTE_TAB_ID
		  || relrecord.reltid.db_tab_base == DM_B_INDEX_TAB_ID)
		{
		    tcb->tcb_rel.relstat |= TCB_DUPLICATES;
		}

		/* For row/page count updating, we need the row tid: */
		STRUCT_ASSIGN_MACRO(tid, tcb->tcb_iirel_tid);

		break;
	    }
	}

	if (status != E_DB_OK)
	    break;

        /* If this was a read-nolock RCB, then release read-nolock buffers. */

        if (rcb->rcb_k_duration & RCB_K_READ)
        {
            /*  Deallocate the MISC_CB for the readlock pages. */

            if (rcb->rcb_rnl_cb)
            {
                dm0m_deallocate((DM_OBJECT **)&rcb->rcb_rnl_cb);
            }
	}

	status = dm2r_release_rcb(&rcb, dberr);
        if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    if (rcb)
    {
	local_status = dm2r_release_rcb(&rcb, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE*)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    uleFormat(dberr,
	0, 
	(CL_ERR_DESC *)NULL, 
	ULE_LOG, (DB_SQLSTATE*)NULL, 
	(char *)NULL,
	(i4)0, 
	(i4 *)NULL, 
	err_code, 
	0);

    SETDBERR(dberr, 0, E_DM927E_UPDATE_TCB);
    return (status);
}

/*{
** Name: update_tcbatts - Update TCB info on system catalogs from iiattribute.
**
** Description:
**	This routine updates the tcb_atts information of the TCB using
**	corresponding tuples in the iiattribute catalog.  It is used to get
**	more accurate information in the TCB's of the core system catalogs
**	which are built from compile time information.
**
** Inputs:
**      tcb                             The base TCB to update.
**	flag				Is this the RCP process?
**	lock_list			Lock list to use for page locking.
**
** Outputs:
**      err_code                        The reason for an error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	Replaces the attribute name field in each tcb_atts array entry
**	with the name from the corresponding iiattribute tuple.  This
**	is required to ensure that the attribute name is in the appropriate
**	case for FIPS and other non-ingres-compliant environments.
**
** History:
**	17-may-1993 (ralph)
**	    DELIM_IDENT:
**	    Moved from update_tcb().
**	23-aug-1993 (rogerk)
**	    Changed DM2D_RCP to DM2D_ONLINE_RCP.
**	18-oct-1993 (jnash)
**	    Fix recovery problem where search performed beyond
**	    the range of valid iiattribute pages, resulting in a 
**	    buffer manager fault page error.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
static DB_STATUS
update_tcbatts(
    DMP_TCB	    *tcb,
    i4	    lock_list,
    i4	    flag,
    DB_ERROR	    *dberr )
{
    DB_STATUS		status;    
    DMP_RCB		*rcb = NULL;
    DM2R_KEY_DESC	qual_list[2];
    DB_TRAN_ID		tran_id;
    DM_TID		tid;
    DMP_ATTRIBUTE	iiatt_record;
    DB_STATUS		local_status;
    i4		local_error;
    i4		att_count = 0;
    i4		    *err_code = &dberr->err_code;
    DB_ERROR		local_dberr;
    i4		    alen;

    CLRDBERR(dberr);

#ifdef xDEBUG
    if (tcb->tcb_type != TCB_CB)
	dmd_check(E_DM9325_DM2T_UPDATE_REL);
#endif

    tran_id.db_high_tran = 0;
    tran_id.db_low_tran = 0;

    for (;;)
    {
	/*
	**  Update the DB_ATTS array anchored in the TCB with
	**  info from the corresponding iiattribute records.
	*/

	status = dm2r_rcb_allocate(
	    tcb->tcb_dcb_ptr->dcb_att_tcb_ptr, 
	    0,
	    &tran_id, 
	    lock_list, 
	    0, 
	    &rcb, 
	    dberr);

	if (status != E_DB_OK)
	    return (status);

	rcb->rcb_lk_id = lock_list;
	rcb->rcb_logging = 0;
	rcb->rcb_journal = 0;

	/* 
	** If this is the RCP process, do read nolocking. Failed server(s)
	** might be holding locks on the system catalog pages.
	*/

	if  ((flag & DM2D_ONLINE_RCP) == 0)
	{
	    rcb->rcb_k_duration |= RCB_K_TEMPORARY;
	}
	else
	{
	    rcb->rcb_k_duration |= RCB_K_READ;
            rcb->rcb_k_type = rcb->rcb_lk_type = RCB_K_TABLE;

            /*
            **  Allocate a MISC_CB to hold the two pages needed to support
            **  readlock=nolock.
            */

            status = dm0m_allocate(
		sizeof(DMP_MISC) + sizeof(DMPP_PAGE) * 2,
                (i4)0, 
		(i4)MISC_CB, 
		(i4)MISC_ASCII_ID,
                (char *)rcb, 
		(DM_OBJECT **)&rcb->rcb_rnl_cb, 
		dberr);

            if (status != E_DB_OK)
                break;
	    rcb->rcb_rnl_cb->misc_data = (char*)rcb->rcb_rnl_cb +
			sizeof(DMP_MISC);
	} 

	rcb->rcb_internal_req = RCB_INTERNAL;

	qual_list[0].attr_operator = DM2R_EQ;
	qual_list[0].attr_number = DM_1_ATTRIBUTE_KEY;
	qual_list[0].attr_value = (char*) &tcb->tcb_rel.reltid.db_tab_base;
	qual_list[1].attr_operator = DM2R_EQ;
	qual_list[1].attr_number = DM_2_ATTRIBUTE_KEY;
	qual_list[1].attr_value = (char*) &tcb->tcb_rel.reltid.db_tab_index;

	status = dm2r_position(
	    rcb, 
	    DM2R_QUAL, 
	    qual_list, 
	    (i4)2,
	    (DM_TID *)NULL, 
	    dberr);

	if (status != E_DB_OK)
	    return (status);

	while (att_count < tcb->tcb_rel.relatts)
	{
	    status = dm2r_get(
		rcb, &tid, DM2R_GETNEXT, (char *)&iiatt_record, dberr);

	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
		    CLRDBERR(dberr);
		}
		break;
	    }

	    if	((iiatt_record.attrelid.db_tab_base != 
		    tcb->tcb_rel.reltid.db_tab_base) ||
		 (iiatt_record.attrelid.db_tab_index != 
		    tcb->tcb_rel.reltid.db_tab_index))
	    {
		continue;
	    }

	    /*
	    ** Check for wacko attribute information.
	    ** (borrowed from read_tcb())
	    */
	    att_count++;
	    if ((iiatt_record.attid > tcb->tcb_rel.relatts) ||
		(iiatt_record.attid <= 0) || (att_count > tcb->tcb_rel.relatts))
	    {
		/* XXXX Add better error message */
		SETDBERR(dberr, 0, E_DM0075_BAD_ATTRIBUTE_ENTRY);
		status = E_DB_ERROR;
                break;
	    }
		
	    /* 
	    ** Copy the volatile information from the iiattribute
	    ** record into the corresponding DB_ATTS entry of the TCB.
	    ** Compute blank-stripped length of attribute name
	    */
	    for (alen = DB_ATT_MAXNAME;  
		iiatt_record.attname.db_att_name[alen-1] == ' ' && alen >= 1; 
			alen--);
	    MEcopy(iiatt_record.attname.db_att_name, alen, 
		tcb->tcb_atts_ptr[iiatt_record.attid].attnmstr);
	    tcb->tcb_atts_ptr[iiatt_record.attid].attnmlen = alen;
	}

	if (status != E_DB_OK)
	    break;

        /* If this was a read-nolock RCB, then release read-nolock buffers. */

        if (rcb->rcb_k_duration & RCB_K_READ)
        {
            /*  Deallocate the MISC_CB for the readlock pages. */

            if (rcb->rcb_rnl_cb)
            {
                dm0m_deallocate((DM_OBJECT **)&rcb->rcb_rnl_cb);
            }
	}

	status = dm2r_release_rcb(&rcb, dberr);

        if (status != E_DB_OK)
	    break;

	return (E_DB_OK);
    }

    if (rcb)
    {
	local_status = dm2r_release_rcb(&rcb, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE*)NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, (i4)0);
	}
    }

    uleFormat(dberr,
	0, 
	(CL_ERR_DESC *)NULL, 
	ULE_LOG, (DB_SQLSTATE*)NULL, 
	(char *)NULL,
	(i4)0, 
	(i4 *)NULL, 
	err_code, 
	0);

    SETDBERR(dberr, 0, E_DM927E_UPDATE_TCB);
    return (status);
}

/*{
** Name: dm2d_reqmem - MEreqmem look alike
**
** Description:
**      This is a look alike function to MEreqmem which allocates memory
**	from DMF's pool.  This is for local collation support.
**	Tags need not be supported. The memory may be released via
**	dm2d_free.
**
** Inputs:
**	tag		memory tag (ignored)
**	size		amount of memory requested in bytes
**	clear		Specify whether to zero the allocated memory
**
** Outputs:
**	status		If all went well, set to OK.
**				otherwise, set to non-zero
**
**	Returns:
**	    pointer to allocated memory
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-apr-89 (anton)
**          Created for local collation support
**	 9-Jul-89 (anton)
**	    Change to allow memory to be released.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory.
**      25-Apr-2007 (kibro01) b115996
**          Ensure the misc_data pointer in a MISC_CB is set correctly and
**          that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**          since that's what we're actually using (the type is MISC_CB)
**	25-Jun-2007 (kschendel) b118561
**	    Fix mistake in fix for b115996 whereby the same pointer was used
**	    twice - we needed two separate pointers to keep the original
**	    DM_OBJECT's position.
**	11-May-2009 (kschendel) b122041
**	    Change prototype to match MEreqmem's.
*/
static PTR
dm2d_reqmem(
    u_i2	tag,
    SIZE_TYPE	size,
    bool	clear,
    STATUS	*status )
{
    DM_OBJECT	*cb;
    PTR		rptr;
    STATUS	locstat;
    DB_ERROR	local_dberr;

    if (status == NULL)
	status = &locstat;
    /* Get LongTerm memory */
    *status = dm0m_allocate(size + sizeof(DMP_MISC),
			   (i4)(clear ? (DM0M_LONGTERM | DM0M_ZERO)
				      : (DM0M_LONGTERM)),
			   (i4)MISC_CB, (i4)MISC_ASCII_ID, 
			   (char *)NULL,
			   &cb, &local_dberr);
    if (*status)
	rptr = NULL;
    else
    {
	rptr = (PTR)(sizeof (DMP_MISC) + (char *)cb);
	((DMP_MISC*)cb)->misc_data = rptr;
    }
    return(rptr);
}

/*{
** Name: dm2d_free - MEfree look alike
**
** Description:
**      This is a look alike function to MEfree which returns memory
**	to DMF's pool.  This is for local collation support.
**	The memory must have been allocated with dm2d_reqmem.
**
** Inputs:
**	obj	pointer to dm2d_reqmem memory object
**
** Outputs:
**	none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	 9-Jul-89 (anton)
**	    Added to allow memory to be released.
**	25-Jul-2007 (kibro01) b118846
**	    dm0m_allocate is allocating a MISC_CB, so this needs to be
**	    adjusted by sizeof(DMP_MISC) instead of DM_OBJECT.
*/
static VOID
dm2d_free(
    PTR		*obj )
{
    *obj = ((char *)*obj - sizeof (DMP_MISC));
    dm0m_deallocate((DM_OBJECT **)obj);
}

/*
** Name: dm2d_check_db_backup_status	- is this DB undergoing online ckp?
**
** Description:
**	During the time that a checkpoint is underway, all updates must write
**	Before Image records to the logfile. These BI records are copied by
**	the Archiver to the checkpoint's associated dump file, in order that
**	we can undo any updates which occurred while the checkpoint was
**	in progress.
**
**	Most operations write Before Image records automatically. The only
**	operations which currently need special handling are the Btree index
**	updates, which don't use Before Image recovery.
**
**	Therefore, operations on Btrees have special handling during an
**	Online Checkpoint.
**
**	This routine determines whether a database is currently undergoing
**	an Online Checkpoint, and stores that information in the DCB for use
**	by operations which need to know.
**
** Inputs:
**	dcb			Database Control Block
**
** Outputs:
**	err_code		Set if an error occurs
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR		Error in LGshow -- already has been logged.
**
** History:
**	22-mar-1991 (bryanp)
**	    Created as part of fixing B36630,
**	17-oct-1991 (rogerk)
**	    Part of 6.4 to 6.5 merge.  Added support for tracking the
**	    online backup address for a database that is undergoing
**	    a backup.  Update the backup address whenever we notice it
**	    has changed.
**      15-oct-1991 (mikem)
**          Eliminated unix compile warnings ("assignment of different 
**	    structures") by changing the arguments to STRUCT_ASSIGN_MACRO() to
**	    be cast appropriately.
**	29-sep-1992 (bryanp)
**	    Release dcb_mutex properly in dm2d_check_db_backup_status.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed to use LG_S_DATABASE to show the backup state.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Add dcb_backup_lsn to track the Log Sequence Number of the
**		    start of an online backup.
**      06-Mar-2000 (hanal04) Bug 100742 INGSRV 1122.
**          Check DCB status for new flag DCB_S_MLOCKED which indicates
**          that the caller holds the DCB mutex already.
**	19-Jun-2001 (inifa01) Bug 105037 INGSRV 1477
**	    Extended above fix. Added more checks to ensure the caller 
**	    doesn't already hold the DCB mutex before requesting it.
*/
DB_STATUS
dm2d_check_db_backup_status( 
    DMP_DCB	*dcb,
    DB_ERROR		*dberr )
{
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		sys_err;
    LG_DATABASE		db;
    i4			length;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for(;;)
    {
	/*
	** Check if the database is in backup state.  If it is, then we have to
	** log physical log records for all updates, even updates to system

	** catalogs and btree index pages.  Mark the database in BACKUP state
	** so that DMF will do this.
	*/

	/*
	** Store database id into the LG_DATABASE structure
	** used for the show operation.
	*/
	db.db_id = (LG_DBID)dcb->dcb_log_id;
	cl_status = LGshow(LG_S_DATABASE, (PTR)&db, sizeof(db),
	    &length, &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, E_DM9017_BAD_LOG_SHOW, &sys_err, ULE_LOG,
		(DB_SQLSTATE *)NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		err_code, 1, 0, LG_S_DATABASE);

	    status = E_DB_ERROR;
	    SETDBERR(dberr, 0, E_DM9500_DMXE_BEGIN);

	    break;
	}

	/*
	** If database is undergoing backup, then turn on the backup state.
	** If the DCB is already marked BACKUP, but the database is no longer
	** being backed-up, then turn off the backup state.
	**
	** When turning on online backup state, set initial backup address.
	**
	** We need to hold the DCB semaphore while changing the status.
	*/
	if (((db.db_status & DB_BACKUP) && 
		((dcb->dcb_status & DCB_S_BACKUP) == 0)) ||
	    (((db.db_status & DB_BACKUP) == 0) &&
		(dcb->dcb_status & DCB_S_BACKUP)))
	{
            if((dcb->dcb_status & DCB_S_MLOCKED) == 0)
	        dm0s_mlock(&dcb->dcb_mutex);
	    if (db.db_status & DB_BACKUP)
	    {
		dcb->dcb_backup_addr = db.db_sbackup;
		dcb->dcb_backup_lsn  = db.db_sback_lsn;
		dcb->dcb_status |= DCB_S_BACKUP;
	    }
	    else
		dcb->dcb_status &= ~DCB_S_BACKUP;
            if((dcb->dcb_status & DCB_S_MLOCKED) == 0)
	        dm0s_munlock(&dcb->dcb_mutex);
	}

	/* If not in backup, is dcb_ebackup_lsn current */
	if ((db.db_status & DB_BACKUP) == 0)
	{
	    if (LSN_EQ(&dcb->dcb_ebackup_lsn, &db.db_eback_lsn) == FALSE) 
	    {
	        /* Backup has occurred but a inactive transaction notices now */
		if((dcb->dcb_status & DCB_S_MLOCKED) == 0)
	            dm0s_mlock(&dcb->dcb_mutex);
	        if (LSN_EQ(&dcb->dcb_ebackup_lsn, &db.db_eback_lsn) == FALSE) 
	            dcb->dcb_ebackup_lsn  = db.db_eback_lsn;
		if((dcb->dcb_status & DCB_S_MLOCKED) == 0)	        
		    dm0s_munlock(&dcb->dcb_mutex);
	    }
	}

	/*
	** Check for a change in the online backup log address.
	** If there is a new online backup address, then copy it into
	** the dcb so we can tell whether Before Image logging is
	** required.
	**
	** Here, it doesn't really matter whether we check for a change in the
	** online backup log address or a change in the online backup log
	** sequence number. A change in either one means that a new backup has
	** begun.
	*/
	if ((db.db_status & DB_BACKUP) && 
	    ((dcb->dcb_backup_addr.la_sequence != db.db_sbackup.la_sequence) ||
		(dcb->dcb_backup_addr.la_block != db.db_sbackup.la_block) ||
		(dcb->dcb_backup_addr.la_offset != db.db_sbackup.la_offset)))
	{
            if((dcb->dcb_status & DCB_S_MLOCKED) == 0)
	        dm0s_mlock(&dcb->dcb_mutex);
	    dcb->dcb_backup_addr = db.db_sbackup;
	    dcb->dcb_backup_lsn  = db.db_sback_lsn;
            if((dcb->dcb_status & DCB_S_MLOCKED) == 0)
	        dm0s_munlock(&dcb->dcb_mutex);
	}

        break;
    }

    return (status);
}

/*
** Name: dm2d_convert_DBMS_cats_to_65 - Convert DBMS catalogs to the new
**					6.5 space management scheme.
**
** Description:
**	This routine scans iirelation looking for DBMS catalogs. For each
**	one found it 
**	a. Opens the table using dm2t_open
**	b. Calls DM1P to add the new FHDR/FMAP Space Management Scheme.
**	c. Updates the iirelation.relfhdr field for the table.
**
** Inputs:
**	dcb			Database Control Block
**	dcb_lockmode		The databse lock mode
**	lock_list		lock list
**
** Outputs:
**	err_code		Set if an error occurs
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR		
**
** History:
**	29-August-1992 (rmuth)
**	    Created
**	30-October-1992 (rmuth)
**	    Rename log_error to dm2d_log_error.
**      23-mar-1995 (chech02)
**          b67533, dm2d_convert_DBMS_cats_to_65(), update relpages of 
**          iirelation too.
*/
static DB_STATUS
dm2d_convert_DBMS_cats_to_65 (
    DMP_DCB	    	*dcb,
    i4		lock_list,
    DB_ERROR	    	*dberr )
{
    DB_STATUS 		status = E_DB_OK;
    DMP_RCB		*rel_rcb = 0;
    DMP_RCB		*table_rcb = 0;
    DMP_RELATION_26	relrecord;
    DB_TRAN_ID		tran_id;
    DM_TID		tid;
    DMP_TCB		*table_tcb;
    DB_TAB_TIMESTAMP	timestamp;
    i4		    *err_code = &dberr->err_code;
    DB_ERROR		local_dberr;

    CLRDBERR(dberr);

    do
    {

	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	/* 
	** Scan the iirelation table looking for DBMS catalogs 
	*/
	status = dm2r_rcb_allocate(
			dcb->dcb_rel_tcb_ptr,
			0, &tran_id, lock_list, 0 , &rel_rcb, dberr );
	if ( status != E_DB_OK )
	    break;

	rel_rcb->rcb_lk_id   = lock_list;
	rel_rcb->rcb_logging = 0;
	rel_rcb->rcb_journal = 0;

	rel_rcb->rcb_k_type = rel_rcb->rcb_lk_type = RCB_K_TABLE;
	rel_rcb->rcb_k_duration |= RCB_K_PHYSICAL;

	rel_rcb->rcb_internal_req = RCB_INTERNAL;

	/* Position iirelation for full scan */
	status = dm2r_position( rel_rcb, DM2R_ALL, (DM2R_KEY_DESC *)NULL,
				0, &tid, dberr );
	if ( status != E_DB_OK )
	    break;

	/*
	** Read ahead was causing us to read in some unformatted pages
	** so turn it off. This problem arose when trying out the conversion
	** code on 6.5 databases but it does not hurt to include this check.
	** As if there is a problem then it is easier to deal with once the
	** DB has been converted.
	** 
	*/
	rel_rcb->rcb_state ^= RCB_READAHEAD;

	do
	{
	    status = dm2r_get( rel_rcb, &tid, DM2R_GETNEXT, (char *)&relrecord,
				dberr );
	    if ( status != E_DB_OK )
		break;

	    /* 
	    ** See if table is a DBMS catalog which has not yet been
	    ** converted.
	    */
	    if ( ( relrecord.relstat & TCB_CATALOG ) && 
		 (( relrecord.relstat & TCB_EXTCATALOG ) == 0) &&
		 (relrecord.relfhdr == DM_TBL_INVALID_FHDR_PAGENO) )
	    {
    		if (DMZ_AM_MACRO(52))
		    TRdisplay(
	   		"dm2d_convert_DBMS_cats_to_65: table: %~t, db: %~t\n",
	    		sizeof(relrecord.relid),
	    		&relrecord.relid,
	    		sizeof(dcb->dcb_name),
	    		&dcb->dcb_name);

		/* dm2t open the table */
		status = dm2t_open( 
				dcb, &relrecord.reltid, DM2T_N, DM2T_UDIRECT,
				DM2T_A_WRITE, 
				(i4)0,  /* timeout */
				(i4)80, /* maxlocks */
				(i4)0,  /* savepoint */
				(i4)0,  /* Log id */
				lock_list,
				(i4)0,  /* sequence */
				(i4)0,  /* Duration */
				(i4)0,  /* db lockmode not used */
				&tran_id,
				&timestamp,
				&table_rcb,
				(DML_SCB *)NULL,
				dberr );
		if ( status != E_DB_OK )
		    break;

		table_tcb = table_rcb->rcb_tcb_ptr;

		status = dm1p_convert_table_to_65( table_tcb, dberr);
		if (status != E_DB_OK )
		    break;

		/* 
		** Update relfhdr on disc
		*/
		relrecord.relfhdr = table_tcb->tcb_rel.relfhdr;
		relrecord.relpages= table_tcb->tcb_rel.relpages;

		status = dm2r_replace( rel_rcb, &tid, 0, (char *)&relrecord, 
				       (char *)NULL, dberr);
		if ( status != E_DB_OK )
		    break;

		/* 
		** close the table 
		*/
		status = dm2t_close( table_rcb, DM2T_PURGE, dberr);
		if ( status != E_DB_OK )
		    break;

		table_rcb = 0; /* for error checking */
#ifdef xDEV_TST
		/*
		** Test failure during conversion of DBMS cats
		*/
		if (DMZ_TST_MACRO(31))
		{
		    status = E_DB_ERROR;
		    break;
		}
#endif
		
	    }
	    
	    
	} while (TRUE);

	if (( status != E_DB_OK ) && ( dberr->err_code != E_DM0055_NONEXT ))
	    break;

	status = dm2r_release_rcb( &rel_rcb, dberr );
	if ( status != E_DB_OK )
	    break;

	rel_rcb = 0; /* for error checking */

    } while (FALSE );

    if ( status == E_DB_OK )
	return( E_DB_OK );

    /* error condition */

    /* 
    ** If table to convert still open close it 
    */
    if ( table_rcb )
    {
	DB_STATUS 	s;

	s = dm2t_close( table_rcb, DM2T_PURGE, &local_dberr);

    }

    /* 
    ** if still have RCB on iirelation then release it 
    */
    if (rel_rcb)
    {
	DB_STATUS 	s;

	s = dm2r_release_rcb( &rel_rcb, &local_dberr);
    }

    dm2d_log_error( E_DM92FE_DM2D_CONV_DBMS_CATS, dberr);
    return( status );
}

/*
** Name: dm2d_converts_core_table_to_65 - Convert core table to the new
**					6.5 space management scheme.
**
** Description:
**	This routine is passed a TCB for a core DBMS cactlog it then calls
**	DM1P to convert the table. Once converted it calls DM2U to update
**	the iirelation.relfhdr field for the table.
**
** Inputs:
**	tcb			TCB for table we want to convert.
**	lock_list		lock list
**
** Outputs:
**	err_code		Set if an error occurs
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR		
**
** History:
**	29-August-1992 (rmuth)
**	    Created
**	30-October-1992 (rmuth)
**	    - Rename log_error to dm2d_log_error.
**	    - Uncomment call to update this tables relation record.
*/
static DB_STATUS
dm2d_convert_core_table_to_65(
    DMP_TCB	*tcb,
    i4	lock_list,
    DB_ERROR	*dberr )
{
    DB_STATUS		status;
    DMP_RCB		*rcb = 0;
    DM2R_KEY_DESC	qual_list[1];
    DMP_RELATION_26	relrecord;
    DB_TRAN_ID		tran_id;
    DM_TID		tid;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Check if the table has already been converted
    */
    if ( tcb->tcb_rel.relfhdr != DM_TBL_INVALID_FHDR_PAGENO )
	return( E_DB_OK );

    if (DMZ_AM_MACRO(52))
	TRdisplay(
            "dm2d_convert_core_table_to_65: table: %~t, db: %~t\n",
	    sizeof(tcb->tcb_rel.relid),
	    &tcb->tcb_rel.relid,
	    sizeof(tcb->tcb_dcb_ptr->dcb_name),
	    &tcb->tcb_dcb_ptr->dcb_name);

    do
    {
	/*
	** Build the new 6.5 space management scheme 
	*/
	status = dm1p_convert_table_to_65( tcb, dberr );
	if ( status != E_DB_OK )
	    break;


	/*
	** Update the iirelation record for this table
	*/
	tran_id.db_high_tran = 0;
	tran_id.db_low_tran = 0;

	status = dm2u_conv_rel_update( tcb, lock_list, &tran_id, dberr );
	if ( status != E_DB_OK )
	    break;

    } while (FALSE);

    if ( status != E_DB_OK )
        dm2d_log_error( E_DM92FD_DM2D_CONV_CORE_TABLE, dberr);

    return( status );
}


/*{
** Name: dm2d_log_error - Used to log trace back messages.
**
** Description:
**	This routine is used to log traceback messages. The rcb parameter
**	is set if the routine is can be seen from outside this module.
**	
** Inputs:
**	terr_code			Holds traceback message indicating
**					current routine name.
**	err_code			Holds error message generated lower
**					down in the call stack.
**	rcb				Pointer to rcb so can get more info.
**
** Outputs:
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**	29-August-1992 (rmuth)
**	   Created
**	30-October-1992 (rmuth)
**	   Renamed to dm2d_log_error.
**	
*/
static VOID
dm2d_log_error(
    i4	terr_code,
    DB_ERROR	*dberr )
{
    i4	l_err_code;

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
            (char *)NULL, (i4)0, (i4 *)NULL, &l_err_code, 0);
	dberr->err_code = terr_code;
    }
    else
    {
	/* 
	** err_code holds the error number which needs to be returned to
	** the DMF client. So we do not want to overwrite this message
	** when we issue a traceback message
	*/
	uleFormat(NULL, terr_code, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *)NULL, 
            (char *)NULL, (i4)0, (i4 *)NULL, &l_err_code, 0);
    }

    return;
}

static DB_STATUS
dm2d_convert_core_cats_to_65(
    DMP_TCB	*rel_tcb,
    DMP_TCB	*relx_tcb,
    DMP_TCB	*att_tcb,
    DMP_TCB	*idx_tcb,
    i4	lock_list,
    DB_ERROR	*dberr )
{
    DB_STATUS	status;

    do
    {
	/* 
	** Convert the iirelation table 
	*/
	status = dm2d_convert_core_table_to_65( rel_tcb, lock_list, dberr);
	if ( status != E_DB_OK )
	    break;

	/* 
	** Convert the iirelidx index 
	*/
	status = dm2d_convert_core_table_to_65( relx_tcb, lock_list, dberr);
    	if ( status != E_DB_OK )
	    break;
	 
#ifdef xDEV_TST
	/*
	** Test failing in the middle of converting core catalogs
	*/
	if ( DMZ_TST_MACRO(30))
	{
	    status = E_DB_ERROR;
	    break;
	}
#endif

	/* 
	** Convert the iiattribute table 
	*/
	status = dm2d_convert_core_table_to_65( att_tcb, lock_list, dberr);
	if ( status != E_DB_OK )
	    break;

	/* 
	** Convert the iiindex table 
	*/
	status = dm2d_convert_core_table_to_65( idx_tcb, lock_list, dberr);
	if ( status != E_DB_OK )
	    break;

    } while ( FALSE );

    if ( status != E_DB_OK )
        dm2d_log_error( E_DM92FD_DM2D_CONV_CORE_TABLE, dberr);

    return( status );
}

/*{
** Name: makeDefaultID - fill in the default ID for an IIATTRIBUTE tuple
**
** Description:
**	This routine is called by the function that converts 64 iiattribute
**	tuples to 65 format.  This function fills in the new tuple's
**	defaultID based on the old tuple's datatype and its nullability
**	and defaultability.
**	
**	We only fill in canonical default IDs here.  Therefore the second
**	half of the default ID is always set to 0.  The first half is set
**	as follows:
**	
**	DB_DEF_NOT_DEFAULT	for NOT DEFAULT columns
**	DB_DEF_NOT_SPECIFIED	for WITH NULL columns
**	DB_DEF_ID_0		for numeric WITH DEFAULT columns
**	DB_DEF_ID_BLANK		for character WITH DEFAULT columns
**	DB_DEF_ID_TABKEY	for table keys WITH DEFAULT
**	DB_DEF_ID_LOGKEY	for system logical keys WITH DEFAULT
**	DB_DEF_ID_NEEDS_CONVERTING	for UDTs WITH DEFAULT
**
**	Later on, during CREATEDB, a QEF internal procedure will be invoked
**	to seek out the DB_DEF_ID_NEEDS_CONVERTING tuples and finish
**	processing them (by allocating default IDs and writing corresponding
**	default tuples).
**	
**	For reference, here was the pre-65 logic used to fabricate
**	defaults:
**	if ( ATT_NDEFAULT )			DEFAULT = NONE;
**	else if ( datatype is NULLABLE )	DEFAULT = NULL;
**	else if ( datatype is NUMERIC )		DEFAULT = 0;
**	else if ( datatype is CHARACTER )	DEFAULT = ' ';
**	
**	
**	
**	
**	
** Inputs:
**
** Outputs:
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**          none
**
** History:
**	5-mar-93 (rickh)
**	    Created for the FIPS CONSTRAINTS projects (release 65.).
**	29-aug-2006 (gupsh01)
**	    Added new ANSI datetime types.
*/

static VOID
makeDefaultID(
    DB_DEF_ID		*defaultID,
    DMP_OLDATTRIBUTE	*oldTuple
)
{
    i2			dataType = oldTuple->attfmt;
    i2			absoluteValue;

    /* the second half of all canonical IDs is always 0 */

    defaultID->db_tab_index = 0;

    if ( oldTuple->attflag && ATT_NDEFAULT )
    {   defaultID->db_tab_base = DB_DEF_NOT_DEFAULT; }
    else if ( dataType < 0 )	/* if datatype is nullable */
    {   defaultID->db_tab_base = DB_DEF_NOT_SPECIFIED; }
    else	/* non-nullable datatype that craves a default */
    {
	switch ( dataType )
	{
	    /* numeric types */

	    case DB_INT_TYPE:
	    case DB_FLT_TYPE:
	    case DB_MNY_TYPE:
	    case DB_DEC_TYPE:

		defaultID->db_tab_base = DB_DEF_ID_0;
		break;

	    /* character types */

	    case DB_CHR_TYPE:
	    case DB_TXT_TYPE:
	    case DB_CHA_TYPE:
	    case DB_VCH_TYPE:
	    case DB_DTE_TYPE:
	    case DB_ADTE_TYPE:
	    case DB_TMWO_TYPE:
	    case DB_TMW_TYPE:
	    case DB_TME_TYPE:
	    case DB_TSWO_TYPE:
	    case DB_TSW_TYPE:
	    case DB_TSTMP_TYPE:
	    case DB_INDS_TYPE:
	    case DB_INYM_TYPE:

		defaultID->db_tab_base = DB_DEF_ID_BLANK;
		break;

	    /* novelties */

	    case DB_LOGKEY_TYPE:

		defaultID->db_tab_base = DB_DEF_ID_LOGKEY;
		break;

	    case DB_TABKEY_TYPE:

		defaultID->db_tab_base = DB_DEF_ID_TABKEY;
		break;

	    default:

		/*
		** otherwise, it had better be a UDT.  if it is, we
		** mark the default ID as needing conversion;  this will
		** flag an internal QEF procedure (called later on by
		** CREATEDB) to construct a default tuple for this
		** UDT.
		**
		** if the dataType turns out not to be a UDT, we will mark
		** the default ID as unrecognized.
		*/

		absoluteValue = abs( dataType );
		if ( ADI_DT_MAP_MACRO( absoluteValue ) != absoluteValue )
		{   defaultID->db_tab_base = DB_DEF_ID_NEEDS_CONVERTING; }
		else
		{   defaultID->db_tab_base = DB_DEF_ID_UNRECOGNIZED; }
		break;

	}	/* end switch on datatype */

    }	/* endif */

    return;
}

/*{
** Name: dm2d_check_dir	- Check if db directory is present
**
** Description:
**	This routine checks if the database directory is present.
**
** Inputs:
**      dcb                             Pointer to DCB for this database.
**      err_code                        Reason for error return status.
**					    E_DM9444_DMD_DIR_NOT_PRESENT
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
**	23-may-1994 (andys)
**          Created.
*/
static STATUS
list_func(VOID)
{
    return (FAIL);
}

DB_STATUS
dm2d_check_dir(
DMP_DCB             *dcb,
DB_ERROR	    *dberr
)
{
    STATUS              s;
    DB_STATUS		status = E_DB_OK;
    CL_ERR_DESC          sys_err;
    DI_IO 		di_io;

    CLRDBERR(dberr);

    status = DIlistfile(&di_io,
                        (char *) &dcb->dcb_location.physical, 
			dcb->dcb_location.phys_length,
                        list_func, 0, &sys_err);
    if (status == DI_DIRNOTFOUND)
    {
        /*
	** Database directory not present...
        */
	SETDBERR(dberr, 0, E_DM9444_DMD_DIR_NOT_PRESENT);
        status = E_DB_ERROR;
    }
    return (status);
}


/*{
** Name: dm2d_get_dcb	- given a db_id, find its dcb
**
** Description:
**
** Inputs:
**	db_id				The database_id to look for
**
** Outputs:
**	pdcb				Where to put address of dcb, if found
**	err_code			The reason for an error status.
**	Returns:
**	    E_DB_OK			The dbid was found, pdcb has been set.
**	    E_DB_ERROR			NOT !
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-apr-95 (cohmi01)
**	    Added for IOMASTER/write-along thread.
*/
DB_STATUS
dm2d_get_dcb(
    i4		dbid,
    DMP_DCB		**pdcb,
    DB_ERROR		*dberr )
{
    CL_ERR_DESC      sys_err;
    DM_SVCB	    *svcb = dmf_svcb;
    DMP_DCB	    *next_dcb;
    DMP_DCB	    *end_dcb;
    DB_STATUS	    status;
    i4			*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*  Lock the SVCB. */

    dm0s_mlock(&svcb->svcb_dq_mutex);


    /* scan thru list of dcbs pointed to by SVCB. */

    for (
        next_dcb = svcb->svcb_d_next, 
        end_dcb = (DMP_DCB*)&svcb->svcb_d_next;
	next_dcb != end_dcb;
	next_dcb = next_dcb->dcb_q_next
	)
    {
	if (next_dcb->dcb_id == dbid)
	    break;
    }

    dm0s_munlock(&svcb->svcb_dq_mutex);

    if (next_dcb == end_dcb)
	status = E_DB_ERROR;
    else
    if (next_dcb->dcb_status & DCB_S_INVALID)
    {
	/*
	** Log message saying that the server must be shut down in 
	** order to reaccess the database, then return an error
	** specifying that the database is not available.
	*/
	uleFormat(NULL, E_DM927F_DCB_INVALID, (CL_ERR_DESC *)NULL, ULE_LOG, 
            (DB_SQLSTATE *)NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
	    sizeof(next_dcb->dcb_name.db_db_name),
	    next_dcb->dcb_name.db_db_name);
	SETDBERR(dberr, 0, E_DM0133_ERROR_ACCESSING_DB);
    }
    else
    if (next_dcb->dcb_access_mode != DCB_A_WRITE)
	status = E_DB_ERROR;
    else
    {
	status = E_DB_OK;
	*pdcb = next_dcb;
    }

    return (status);
}


/*{
** Name: dm2d_row_lock	- Check if we can do row locking
**
** Description: Check if row locking is permitted for this database/server.
**
** Row locking is allowed only for FASTCOMMIT servers. It is disallowed
** for the following non-fast-commit configurations:
**
**       1) Single server, private cache (sole server):
**          When fast commit is off, pages updated are forced to disk
**          at the end of the transaction in dm0p_force_pages.
**          Pages updated by a transaction are chained through the buf_trq
**          field in the buffer header. This method of maintaining a 
**          list of buffers updated by a transaction does not work when
**          row locking since a page may be fixed for write by multiple
**          transactions. To solve this problem we can:
**            - Add a new queueing mechanism to the buffer manager to allow
**              a buffer to be on multiple transaction queues (and make the
**              buffer manager even more complex!)
**            - Write the buffer each time it is unfixed. (very bad solution!)
**          Since a private cache sole server would probably be defined
**          with fast commit on, this restriction seems reasonable.
**              
**       2) Multiple servers using different caches:
**          When a database is accessed concurrently by multiple servers
**          using different data caches, the buffer manager does cache
**          page locking to synchronize access to pages. Pages are not
**          guaranteed to be written/unlocked until the transaction ends.
**          Row locking is allowed only if the server is cache locking with
**          the DMCM protocol (fast commit).
**          
**       There are also some strong assumptions in the recovery code,
**       specifically in the case of online recovery that require that
**       the RCP have exlusive control of the database during online
**       recovery of a row locking transaction. ALL 'Deleted' tuples are
**       cleaned from the page in REDO recovery. This is necessary in
**       REDO recovery since page cleans are not logged.
**       
**       When a fast commit server crashes, all other fast commit servers
**       crash before online recovery is performed. This guarantees
**       exclusive access to the database during online recovery when
**       the configuration is:
**           shared cache ON, fast commit ON
**           shared cache OFF, DMCM ON, fast commit ON
**       
**       When a sole server with fast commit on crashes, the EXCL SVDB
**       server lock is inheried by the RCP, preventing another server
**       from starting before the online recovery has completed.
** 
** Row locking is disallowed when using DMCM protocols:
** 
**       Temporarily disable row locking with DMCM protocol. 
**       When LK_PH_LOCKS were replaced with mutexes, a DMCM protocol
**       violation can result in undetected deadlocks because
**       deadlock detection is not done when obtaining cache locks.
**       The DMCM protocol assumes that logical page locks have been
**       requested at the same mode as the cache locks. (SHARE/EXCL).
**       This assumption is not true when row locking.
**      
**       The following changes are required to support row locking 
**       with DMCM:
**       - Unfix pages (release cache locks) when leaving dmf
**       - Unfix base table pages before fixing secondary index pages
**       - Unfix pages (release cache locks) before waiting for a row 
**         lock
**       - Perform deadlock detection when requesting cache locks
**         since the logical page lock may be held at a lower level
**         than the cache lock.
**         The cache locks are similar to the LK_PH_PAGE locks we 
**         were using to control simultaneous access to a page
**
** Inputs:
**      dcb                     Database Control Block
**
** Outputs:
**      TRUE/FALSE
**
** Side Effects:
**	    none
**
** History:
**      28-may-98 (stial01) 
**          Moved from dml.
*/
bool
dm2d_row_lock(
DMP_DCB             *dcb)
{

    /*
    ** Disallow row locking if:
    **
    **     Database/server is not using fast commit protocols.
    **     Database is iidbdb. IIDBDB always uses non-FASTCOMMIT protocols.
    **     Database server is using DMCM protocol.
    ** 
    */
    if ( ((dcb->dcb_status & DCB_S_FASTCOMMIT) == 0)
	|| (dcb->dcb_id == IIDBDB_ID)
	|| (dcb->dcb_status & DCB_S_DMCM))
    {
	return (FALSE);
    }
    else
    {
	return (TRUE);
    }
}

/*{
** Name: dm2d_convert_inplace_iirel - convert iirelation to V7 format
**
** Description:
**      Used indirectly by upgradedb to convert iirelation to V7 format
**      V7 is DMF version for Ingres Release 3
**
**      Since this upgrade does not change the width of the iirelation catalog,
**      it can put-replace the new iirelation record onto the old iirelation 
**      record.
**      
**      Since we are not changing the width of iirelation and we are not
**      moving any rows there is no need to convert iirel_idx.
**      Since we are not changing the position of any key columns in iirelation,
**      there is no need to make any changes in iiindex.
**
**      For the 256k row project, the relwid and reltotwid columns must
**      change from i2 to i4. The new format was chosen such that the offset
**      of only a few attributes changed. The relpgsize attribute was moved
**      so that the upgrade would know if the upgrade had already been done.
**      The relloccount attribute was changed from i4 to i2 so that we could
**      still have 8 bytes available in relfree.
**   
**	Since significant core catalog (iirelation and iiattribute) changes
**	have occurred, we'll take the approach of reconstructing the
**	iiattribute rows describing the core catalog columns.  (See
**	the iiattribute conversion.)  Equivalent changes must be plugged
**	into the core catalog iirelation rows as we encounter them.
**
** Inputs:
**      db_id                           database to upgrade
**	create_version			Version converting from
**	flag				Config flags (ro, locked)
**	lock_list			Lock list if config already locked
**
** Outputs:
**      err_code                        error return code
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	18-Dec-2003 (schka24)
**	    Another convert-iirel clone, for partitioned table additions.
**      18-feb-2004 (stial01)
**          Core catalog upgrade for 256k rows, rows spanning pages
**	25-Aug-2004 (schka24)
**	    Combine V6 and V7 updates.
[@history_template@]...
*/
static DB_STATUS
dm2d_convert_inplace_iirel (
    DM2D_UPGRADE    *upgrade_cb,
    DMP_DCB	    *db_id,
    i4              flag,
    i4              lock_list,
    DB_ERROR	    *dberr )
{
    DMP_DCB	    *dcb = (DMP_DCB *)db_id;
    i4	            temp_lock_list;
    STATUS	    status = OK;
    DB_STATUS       db_status = E_DB_OK;
    DB_STATUS       local_status;
    i4		    local_error;
    CL_ERR_DESC	    sys_err;
    DM0C_CNF	    *config = 0;
    bool	    config_open = FALSE;
    bool	    iirel_open = FALSE;
    DB_TAB_ID	    iirel_tab_id;
    DM_FILE	    iirel_file_name;
    DI_IO	    iirel_di_io;
    DMPP_PAGE	    *iirel_pg = (DMPP_PAGE *)NULL;
    i4	   	    io_count;
    DMP_RELATION    new_iirel;
    DMP_RELATION    old_iirel;
    DMP_RELATION    *old_iirel_ptr;
    DM_TID	    iirel_tid;
    DMPP_ACC_PLV    *local_plv;
    DB_TRAN_ID	    upgrade_tran;
    DM_TID	    tid;
    i4	    record_size = sizeof(DMP_RELATION);
    i4      iirel_pgformat;
    i4      iirel_pgsize;
    i4	    curpagenum;
    i4	    convert_cnt = 0;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	
	/*  
	** Create a temporary lock list. This is needed to open the config file
	*/
	if ((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKcreate_list(LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT,
		(i4)0, (LK_UNIQUE *)NULL, (LK_LLID *)&temp_lock_list, 0, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, 
                    (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		break;
	    }
	}
	else
	{
	    temp_lock_list = lock_list;    
	}

	/*
	** Open the config file.
	** Note that the updating of iirelation is happening at a physical
	** file level, without locking or page buffering.
	*/
	db_status = dm0c_open(dcb, flag, temp_lock_list, &config, dberr);
	if (db_status != E_DB_OK)
	    break;

	config_open = TRUE;

	/* Forcibly turn off journaling.  We're reformatting iirelation
	** rows without benefit of logging, and it would just confuse
	** any journals.  (Actually, we would probably get away with this
	** here, but the iiattribute rewrite would toast the journals for
	** sure.)  User will have to restore journaling after the upgrade.
	** Also note that this is running at "add" time, DCB journal status
	** is set at later "open" time, will pick up the cleared flag.
	*/

	config->cnf_dsc->dsc_status &= ~DSC_JOURNAL;

	/* Determine the page format of iirelation */
	/* Some paranoia here, dm0c conversions should have set all this */
	iirel_pgsize = config->cnf_dsc->dsc_iirel_relpgsize;
	iirel_pgformat = config->cnf_dsc->dsc_iirel_relpgtype;
	if (iirel_pgsize == 0) iirel_pgsize = 2048;
	if (upgrade_cb->upgr_create_version < DSC_V6 &&
 	    (config->cnf_dsc->dsc_status & DSC_IIREL_V6_CONV_DONE) == 0)
	{
	    /* Set iirelation page type based on size */
	    iirel_pgformat = TCB_PG_V1;
	    if (iirel_pgsize != DM_COMPAT_PGSIZE)
		iirel_pgformat = TCB_PG_V2;
	}

	if ((iirel_pg = (DMPP_PAGE *)dm0m_pgalloc(iirel_pgsize)) == 0)
	{
	    /* FIX ME fix me error */
	    break;
	}

	/*  Generate the filename for iirelation table. */
	iirel_tab_id.db_tab_base = DM_B_RELATION_TAB_ID;
	iirel_tab_id.db_tab_index = DM_I_RELATION_TAB_ID;
	dm2f_filename (DM2F_TAB, &iirel_tab_id, 0, 0, 0, &iirel_file_name);

	/* Open the iirelation for write. */
	status = DIopen(&iirel_di_io,
	    dcb->dcb_location.physical.name, dcb->dcb_location.phys_length,
	    iirel_file_name.name, sizeof(iirel_file_name.name),
	    iirel_pgsize, DI_IO_WRITE, DI_SYNC_MASK, &sys_err);

	if (status != OK)
	    break;

	iirel_open = TRUE;

	dm1c_get_plv(iirel_pgformat, &local_plv);

	/* iirelation is HASH, start reading at page zero */
	curpagenum = 0;

	/* init zero tranid for dmpp_put */
	upgrade_tran.db_low_tran = 0;
	upgrade_tran.db_high_tran = 0;

	/* Now update all records in iirelation */
	for (;;)
	{
	    /* Read a page from the iirelation */
	    io_count = 1;
	    status = DIread(&iirel_di_io, &io_count,(i4)curpagenum, 
		    (char *)iirel_pg, &sys_err);
	    if (status != OK)
		break;

	    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(iirel_pgformat, iirel_pg)
			    != curpagenum)
	    {
		/* fix me error */
		break;
	    }


	    /* Read all records on current page */
	    tid.tid_tid.tid_page = curpagenum;
	    tid.tid_tid.tid_line = DM_TIDEOF;

	    /* Update each iirelation record on this page */
	    while (++tid.tid_tid.tid_line < 
		(i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(iirel_pgformat, iirel_pg))
	    {
		old_iirel_ptr = &old_iirel;
		db_status = (*local_plv->dmpp_get)(iirel_pgformat, iirel_pgsize,
		    iirel_pg, &tid, &record_size, (char **)&old_iirel_ptr,
		    (i4*)NULL, (u_i4*)NULL, (u_i2*)NULL, 
		    (DMPP_SEG_HDR *)NULL);
	
		/* If the line is empty, go to the next line. */
		if  (db_status == E_DB_WARN)
		    continue;
		if (db_status != E_DB_OK)
		    break;
		    
		if (old_iirel_ptr != &old_iirel)
		    MEcopy(old_iirel_ptr, sizeof(old_iirel), &old_iirel);

		/*
		** UPGRADE the row to the CURRENT format 
		** if this row is for a core catalog it will also sync 
		** new_rel with DM_core_relations (upgrade_core_reltup)
		*/
		upgrade_iirel_row(upgrade_cb, (char *)&old_iirel,
			&new_iirel, config->cnf_dsc->dsc_status);
			
		(*local_plv->dmpp_put)(iirel_pgformat, iirel_pgsize, iirel_pg,
			DM1C_DIRECT, &upgrade_tran, (u_i2)0, &tid,
			sizeof(new_iirel), (char *)&new_iirel,
			(u_i2)0, /* alter table version 0 */
			(DMPP_SEG_HDR *)NULL);

		convert_cnt++;

	    } /* end while */

	    if (db_status == E_DB_ERROR) 
		break;
	    else
		db_status = E_DB_OK;

	    /* clear DMPP_MODIFY, checksum page */
	    DMPP_VPT_CLR_PAGE_STAT_MACRO(iirel_pgformat, iirel_pg, DMPP_MODIFY);
	    dm0p_insert_checksum(iirel_pg, iirel_pgformat, iirel_pgsize); 

	    io_count = 1;
	    status = DIwrite(&iirel_di_io, &io_count, curpagenum,
		(char *)iirel_pg, &sys_err);
	    if (status != OK) 
		break;

	    /* Follow overflow chain */
	    if (DMPP_VPT_GET_PAGE_OVFL_MACRO(iirel_pgformat, iirel_pg) != 0)
		curpagenum = 
			DMPP_VPT_GET_PAGE_OVFL_MACRO(iirel_pgformat, iirel_pg);
	    /* No more overflow buckets. Go to the next hash bucket. */
	    else if (DMPP_VPT_GET_PAGE_MAIN_MACRO(iirel_pgformat, iirel_pg) != 0)
		curpagenum = 
			DMPP_VPT_GET_PAGE_MAIN_MACRO(iirel_pgformat, iirel_pg);
	    /* No more main and overflow buckets. We're done. */
	    else
		break;

	} /* end for */

	if (status != OK || db_status != E_DB_OK) 
	    break;

	/* Close the new iirelation file */
	status = DIclose (&iirel_di_io, &sys_err);
	if  (status != OK)
	    break;
	    
	iirel_open = FALSE;

	/* Close the configuration file. */
	/* The configuration file version is not yet updated.  We still
	** need to deal with iiattribute which the caller will do shortly.
	*/

	db_status = dm0c_close(config, (flag & DM0C_CNF_LOCKED) | DM0C_UPDATE, 
			dberr);

	if (db_status != E_DB_OK)
	    break;

	config_open = FALSE;

	/*  Release the temporary lock list. */

	if ((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKrelease(LK_ALL, temp_lock_list, (LK_LKID *)NULL,
			(LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, 
                    (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 1,
		    sizeof(temp_lock_list), temp_lock_list);
		break;
	    }
	}

	/* Return success. */
	dm0m_pgdealloc((char **)&iirel_pg);

	return (E_DB_OK);
    } /* end for */

    if (iirel_pg)
	dm0m_pgdealloc((char **)&iirel_pg);

    /* Deallocate the configuration record. */
    if (config_open == TRUE)
	local_status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

    /* Close the iirelation files if they are open. */
    if (iirel_open == TRUE)
	local_status = DIclose (&iirel_di_io, &sys_err);

    SETDBERR(dberr, 0, E_DM93AC_CONVERT_IIREL);
    return (E_DB_ERROR);
}


/*{
** Name: dm2d_convert_preV9_iirel - convert iirelation to V9 format
**
** Description:
**      Used indirectly by upgradedb to convert iirelation to V9 format
**
**      V9 is DMF version for Ingres2007
**
**      Since this upgrade does not change the width of the iirelation catalog,
**      it can put-replace the new iirelation record onto the old iirelation 
**      record.
**      
**      Since we are not changing the width of iirelation and we are not
**      moving any rows there is no need to convert iirel_idx.
**      Since we are not changing the position of any key columns in iirelation,
**      there is no need to make any changes in iiindex.
**
**	For Ingres2007 Clustered Tables and indexes on Clustered Tables,
**	TCB_CLUSTERED reuses the old TCB_VBASE bit, so make sure it's off.
**
**	The size and content of the IIINDEX table has changed, adding
**	32 more idom attributes, so the relwid and relatts of the iirelation
**	row for IIINDEX must be modified. The core catalog attributes will
**	be reconstructed in dm2d_convert_preV9_iiatt(), to follow.
**   
**	Since significant core catalog (iirelation and iiattribute) changes
**	have occurred, we'll take the approach of reconstructing the
**	iiattribute rows describing the core catalog columns.  (See
**	the iiattribute conversion.)  Equivalent changes must be plugged
**	into the core catalog iirelation rows as we encounter them.
**
** Inputs:
**      db_id                           database to upgrade
**	create_version			Version converting from
**	flag				Config flags (ro, locked)
**	lock_list			Lock list if config already locked
**
** Outputs:
**      err_code                        error return code
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-May-2006 (jenjo02)
**	    Added for any need mods to iirelation for Ingres2007.
[@history_template@]...
*/
static DB_STATUS
dm2d_convert_preV9_iirel (
    DMP_DCB	    *db_id,
    i4		    create_version,
    i4              flag,
    i4              lock_list,
    DB_ERROR	    *dberr )
{
    DMP_DCB	    *dcb = (DMP_DCB *)db_id;
    i4	            temp_lock_list;
    STATUS	    status = OK;
    DB_STATUS       db_status = E_DB_OK;
    DB_STATUS       local_status;
    i4		    local_error;
    CL_ERR_DESC	    sys_err;
    DM0C_CNF	    *config = 0;
    bool	    config_open = FALSE;
    bool	    iirel_open = FALSE;
    DB_TAB_ID	    iirel_tab_id;
    DM_FILE	    iirel_file_name;
    DI_IO	    iirel_di_io;
    DMPP_PAGE	    *iirel_pg = (DMPP_PAGE *)NULL;
    i4	   	    io_count;
    DMP_RELATION    old_iirel;
    DMP_RELATION    *old_iirel_ptr;
    DMP_RELATION    new_iirel;
    DM_TID	    iirel_tid;
    DMPP_ACC_PLV    *local_plv;
    DB_TRAN_ID	    upgrade_tran;
    DM_TID	    tid;
    i4	    	    record_size = sizeof(DMP_RELATION);
    i4      	    iirel_pgformat;
    i4      	    iirel_pgsize;
    i4	    	    curpagenum;
    i4	    	    convert_cnt = 0;
    i4		    i;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    for (;;)
    {
	
	/*  
	** Create a temporary lock list. This is needed to open the config file
	*/
	if ((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKcreate_list(LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT,
		(i4)0, (LK_UNIQUE *)NULL, (LK_LLID *)&temp_lock_list, 0, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, 
                    (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		break;
	    }
	}
	else
	{
	    temp_lock_list = lock_list;    
	}

	/*
	** Open the config file.
	** Note that the updating of iirelation is happening at a physical
	** file level, without locking or page buffering.
	*/
	db_status = dm0c_open(dcb, flag, temp_lock_list, &config, dberr);
	if (db_status != E_DB_OK)
	    break;

	config_open = TRUE;

	/* Forcibly turn off journaling.  We're reformatting iirelation
	** rows without benefit of logging, and it would just confuse
	** any journals.  (Actually, we would probably get away with this
	** here, but the iiattribute rewrite would toast the journals for
	** sure.)  User will have to restore journaling after the upgrade.
	** Also note that this is running at "add" time, DCB journal status
	** is set at later "open" time, will pick up the cleared flag.
	*/

	config->cnf_dsc->dsc_status &= ~DSC_JOURNAL;

	/* Determine the page format of iirelation */
	/* Some paranoia here, dm0c conversions should have set all this */
	iirel_pgsize = config->cnf_dsc->dsc_iirel_relpgsize;
	iirel_pgformat = config->cnf_dsc->dsc_iirel_relpgtype;
	if (iirel_pgsize == 0) iirel_pgsize = 2048;
	if (create_version < DSC_V6 &&
 	    (config->cnf_dsc->dsc_status & DSC_IIREL_V6_CONV_DONE) == 0)
	{
	    /* Set iirelation page type based on size */
	    iirel_pgformat = TCB_PG_V1;
	    if (iirel_pgsize != DM_COMPAT_PGSIZE)
		iirel_pgformat = TCB_PG_V2;
	}

	if ((iirel_pg = (DMPP_PAGE *)dm0m_pgalloc(iirel_pgsize)) == 0)
	{
	    /* FIX ME fix me error */
	    break;
	}

	/*  Generate the filename for iirelation table. */
	iirel_tab_id.db_tab_base = DM_B_RELATION_TAB_ID;
	iirel_tab_id.db_tab_index = DM_I_RELATION_TAB_ID;
	dm2f_filename (DM2F_TAB, &iirel_tab_id, 0, 0, 0, &iirel_file_name);

	/* Open the iirelation for write. */
	status = DIopen(&iirel_di_io,
	    dcb->dcb_location.physical.name, dcb->dcb_location.phys_length,
	    iirel_file_name.name, sizeof(iirel_file_name.name),
	    iirel_pgsize, DI_IO_WRITE, DI_SYNC_MASK, &sys_err);

	if (status != OK)
	    break;

	iirel_open = TRUE;

	dm1c_get_plv(iirel_pgformat, &local_plv);

	/* iirelation is HASH, start reading at page zero */
	curpagenum = 0;

	/* init zero tranid for dmpp_put */
	upgrade_tran.db_low_tran = 0;
	upgrade_tran.db_high_tran = 0;

	/* Now update all records in iirelation */
	for (;;)
	{
	    /* Read a page from the iirelation */
	    io_count = 1;
	    status = DIread(&iirel_di_io, &io_count,(i4)curpagenum, 
		    (char *)iirel_pg, &sys_err);
	    if (status != OK)
		break;

	    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(iirel_pgformat, iirel_pg)
			    != curpagenum)
	    {
		/* fix me error */
		break;
	    }


	    /* Read all records on current page */
	    tid.tid_tid.tid_page = curpagenum;
	    tid.tid_tid.tid_line = DM_TIDEOF;

	    /* Update each iirelation record on this page */
	    while (++tid.tid_tid.tid_line < 
		(i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(iirel_pgformat, iirel_pg))
	    {
		old_iirel_ptr = &old_iirel;
		db_status = (*local_plv->dmpp_get)(iirel_pgformat, iirel_pgsize,
		    iirel_pg, &tid, &record_size, (char **)&old_iirel_ptr,
		    (i4*)NULL, (u_i4*)NULL, (u_i2*)NULL, 
		    (DMPP_SEG_HDR *)NULL);
	
		/* If the line is empty, go to the next line. */
		if  (db_status == E_DB_WARN)
		    continue;
		if (db_status != E_DB_OK)
		    break;
		    
		if (old_iirel_ptr != &old_iirel)
		    MEcopy(old_iirel_ptr, sizeof(old_iirel), &old_iirel);

		MEcopy((PTR)&old_iirel, sizeof(DMP_RELATION), (PTR)&new_iirel);

		/* IIINDEX's iirelation needs relwid, relatts updated */
		if ( new_iirel.reltid.db_tab_base == DM_B_INDEX_TAB_ID )
		{
		    for ( i = 0; i < DM_sizeof_core_rel; i++ )
		    {
			if ( DM_core_relations[i].reltid.db_tab_base ==
				DM_B_INDEX_TAB_ID &&
			     DM_core_relations[i].reltid.db_tab_index ==
				DM_I_INDEX_TAB_ID )
			{
			    new_iirel.relwid     = DM_core_relations[i].relwid;
			    new_iirel.reltotwid  = DM_core_relations[i].reltotwid;
			    new_iirel.relatts    = DM_core_relations[i].relatts;

			    /* We'll be rebuilding the iiindex rows */
			    new_iirel.relprim = 1;
			    new_iirel.relfhdr       = DM_TBL_INVALID_FHDR_PAGENO;
			    new_iirel.relallocation = DM_TBL_DEFAULT_ALLOCATION;
			    new_iirel.relextend     = DM_TBL_DEFAULT_EXTEND;
			    break;
			}
		    }
		}
		else if ( new_iirel.reltid.db_tab_base == DM_B_ATTRIBUTE_TAB_ID )
		{
		    /* We'll be rebuilding the attributes */
		    new_iirel.relprim = 1;
		    new_iirel.relfhdr       = DM_TBL_INVALID_FHDR_PAGENO;
		    new_iirel.relallocation = DM_TBL_DEFAULT_ALLOCATION;
		    new_iirel.relextend     = DM_TBL_DEFAULT_EXTEND;
		}

		/* CLUSTERED reuses old VBASE bit, make sure it's off */
		new_iirel.relstat &= ~TCB_CLUSTERED;

		/* iirel_idx, iiindex unchanged since 6.4 */

		(*local_plv->dmpp_put)(iirel_pgformat, iirel_pgsize, iirel_pg,
			DM1C_DIRECT, &upgrade_tran, (u_i2)0, &tid,
			sizeof(new_iirel), (char *)&new_iirel,
			(u_i2)0, /* alter table version 0 */
			(DMPP_SEG_HDR *)NULL);

		convert_cnt++;

	    } /* end while */

	    if (db_status == E_DB_ERROR) 
		break;
	    else
		db_status = E_DB_OK;

	    /* clear DMPP_MODIFY, checksum page */
	    DMPP_VPT_CLR_PAGE_STAT_MACRO(iirel_pgformat, iirel_pg, DMPP_MODIFY);
	    dm0p_insert_checksum(iirel_pg, iirel_pgformat, iirel_pgsize); 

	    io_count = 1;
	    status = DIwrite(&iirel_di_io, &io_count, curpagenum,
		(char *)iirel_pg, &sys_err);
	    if (status != OK) 
		break;

	    /* Follow overflow chain */
	    if (DMPP_VPT_GET_PAGE_OVFL_MACRO(iirel_pgformat, iirel_pg) != 0)
		curpagenum = 
			DMPP_VPT_GET_PAGE_OVFL_MACRO(iirel_pgformat, iirel_pg);
	    /* No more overflow buckets. Go to the next hash bucket. */
	    else if (DMPP_VPT_GET_PAGE_MAIN_MACRO(iirel_pgformat, iirel_pg) != 0)
		curpagenum = 
			DMPP_VPT_GET_PAGE_MAIN_MACRO(iirel_pgformat, iirel_pg);
	    /* No more main and overflow buckets. We're done. */
	    else
		break;

	} /* end for */

	if (status != OK || db_status != E_DB_OK) 
	    break;

	/* Close the new iirelation file */
	status = DIclose (&iirel_di_io, &sys_err);
	if  (status != OK)
	    break;
	    
	iirel_open = FALSE;

	/* Close the configuration file. */
	/* The configuration file version is not yet updated.  We still
	** need to deal with iiattributes for IIINDEX which the caller 
	** will do shortly.
	*/

	db_status = dm0c_close(config, (flag & DM0C_CNF_LOCKED) | DM0C_UPDATE, 
			dberr);

	if (db_status != E_DB_OK)
	    break;

	config_open = FALSE;

	/*  Release the temporary lock list. */

	if ((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKrelease(LK_ALL, temp_lock_list, (LK_LKID *)NULL,
			(LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, 
                    (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 1,
		    sizeof(temp_lock_list), temp_lock_list);
		break;
	    }
	}

	/* Return success. */
	dm0m_pgdealloc((char **)&iirel_pg);

	return (E_DB_OK);
    } /* end for */

    if (iirel_pg)
	dm0m_pgdealloc((char **)&iirel_pg);

    /* Deallocate the configuration record. */
    if (config_open == TRUE)
	local_status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

    /* Close the iirelation files if they are open. */
    if (iirel_open == TRUE)
	local_status = DIclose (&iirel_di_io, &sys_err);

    SETDBERR(dberr, 0, E_DM93AC_CONVERT_IIREL);
    return (E_DB_ERROR);
}

/*{
** Name: dm2d_convert_preV9_iiatt - convert iiattribute to V9 format
**
** Description:
**      Used indirectly by upgradedb to convert iiattribute to V9 format
**
**	V9 is DMF version for Ingres2007
**
**	For Ingres2007, nothing in iiattribute has changed from Ingres2006,
**	so there's no "conversion" to be done.
**
**	But the core catalog IIINDEX needs 32 more "idom" attributes,
**	and the easiest way to add them is to just copy all existing
**	non-core attributes, then remake the core attributes.
**
** Inputs:
**      db_id                           database to upgrade
**
** Outputs:
**      err_code                        error return code
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-May-2006 (jenjo02)
**	    Written to expand number of IIINDEX idom attributes from
**	    32 to 64 for Indexes on Clustered Tables.
[@history_template@]...
*/
static DB_STATUS
dm2d_convert_preV9_iiatt(
    DMP_DCB	    *db_id,
    i4		    create_version,
    i4              flag,
    i4              lock_list,
    DB_ERROR	    *dberr )
{
    DMP_DCB	    *dcb = (DMP_DCB *)db_id;
    i4	    temp_lock_list, status, i=0, local_error, local_status;
    CL_ERR_DESC	    sys_err;
    DM0C_CNF	    *config = 0;
    bool	    config_open = FALSE, tmp_open = FALSE;
    bool	    iiatt_open = FALSE, add_status;
    DB_TAB_ID	    iiatt_tab_id;
    DM_FILE	    tmp_file_name = 
			{{'i','i','a','t','t','c','v','t','.','t','m','p'}};
    DM_FILE	    iiatt_file_name;
    DI_IO	    tmp_di_context, iiatt_di_context;
    DMPP_PAGE	    *iiatt_pg, *tmp_pg;
    i4	    	    iiatt_page_number=0;
    i4	    	    read_count, write_count, line, offset;
    DMP_ATTRIBUTE   new_att;
    char	    *old_att;
    DMP_ATTRIBUTE    old_att_row;	/* See below, at "get" */
    DM_TID	    iiatt_tid;
    DMPP_ACC_PLV    *local_plv;
    DM_TID	    tid;
    i4	    record_size;
    i4      iiatt_pgformat;
    i4      iiatt_pgsize;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
 
    for (;;)
    {
	/*  
	**  Create a temporary lock list. This is needed to open the config
	**  file. 
	*/
	if((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKcreate_list(
		LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT,
		(i4)0, 
		(LK_UNIQUE *)NULL, 
		(LK_LLID *)&temp_lock_list, 
		0,
		&sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL,
			E_DM901A_BAD_LOCK_CREATE, 
			&sys_err, 
			ULE_LOG, 
			(DB_SQLSTATE *)NULL, 
			(char *)NULL, 
			0L, 
			(i4 *)NULL, 
			&local_error, 
			0);

	    	break;
	    }
	}
	else
	{
	    temp_lock_list = lock_list;
	}

	/*  Open the config file. */

	status = dm0c_open(dcb, flag, temp_lock_list, &config, dberr);

	if (status != E_DB_OK)
	    break;

	config_open = TRUE;

	/*
	** Check the status of the upgrade. Its possible that upgradedb failed
	** and we're trying again. In that case upgradedb might have
	** failed after completing the convert.
	*/

	if  ((config->cnf_dsc->dsc_status & DSC_IIATT_V9_CONV_DONE) != 0)
	{
	    /* We've already completed the conversion. */

	    /* If it still exists, delete the temporary file. */

	    status = DIdelete (	
		&tmp_di_context, 
		(char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		tmp_file_name.name, 
		sizeof(tmp_file_name.name), 
		&sys_err);

	    if ((status != OK) && (status != DI_FILENOTFOUND))
		break;

	    /* Close the configuration file. */

	    status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

	    if (status != E_DB_OK)
		break;

	    /*  Release the temporary lock list. */

          if((flag & DM0C_CNF_LOCKED) == 0)
          {
	    status = LKrelease(
		LK_ALL, 
		temp_lock_list, 
		(LK_LKID *)NULL, 
		(LK_LOCK_KEY *)NULL,
		(LK_VALUE *)NULL, 
		&sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL,
		    E_DM901B_BAD_LOCK_RELEASE, 
		    &sys_err, 
		    ULE_LOG , 
		    (DB_SQLSTATE *)NULL, 
		    (char *)NULL, 
		    0L, 
		    (i4 *)NULL, 
		    &local_error, 
		    1, 
		    sizeof(temp_lock_list), 
		    temp_lock_list);

		break;
	    }
          }
	    return (E_DB_OK);
	}

	/*  Generate the filenames for iiattribute tables. */

	iiatt_tab_id.db_tab_base = DM_B_ATTRIBUTE_TAB_ID;
	iiatt_tab_id.db_tab_index = DM_I_ATTRIBUTE_TAB_ID;

	dm2f_filename (DM2F_TAB, &iiatt_tab_id, 0, 0, 0, &iiatt_file_name);

	/* 
	** Check if we've already renamed iiattribute in a previous invocation
	** of upgradedb. If the file exists, we shouldn't do any renaming.
	*/ 

	/* Determine the page format of iiattribute.  These will have
	** been set properly by the iirelation conversion.
	*/

	iiatt_pgsize = config->cnf_dsc->dsc_iiatt_relpgsize;
	iiatt_pgformat = config->cnf_dsc->dsc_iiatt_relpgtype;

	if ((iiatt_pg = (DMPP_PAGE *)dm0m_pgalloc(iiatt_pgsize)) == 0)
	{
	    /* FIX ME fix me error */
	    break;
	}
	if ((tmp_pg = (DMPP_PAGE *)dm0m_pgalloc(iiatt_pgsize)) == 0)
	{
	    /* FIX ME fix me error */
	    break;
	}

	/* Open the tmp iiattribute for read. */

 	status = DIopen(
	    &tmp_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    tmp_file_name.name, 
	    sizeof(tmp_file_name.name),
	    iiatt_pgsize, 
	    DI_IO_READ, 
	    DI_SYNC_MASK,
	    &sys_err);

	/* If the file doesn't exist, we need to rename iiattribute. */

	if (status == DI_FILENOTFOUND)
	{
	    /*  Rename iiattribute to the tmp filename. */

	    status = DIrename (
		0, 
		(char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		iiatt_file_name.name,
		sizeof(iiatt_file_name.name),	     
		tmp_file_name.name,
		sizeof(tmp_file_name.name),
		&sys_err);

	    if (status != OK)
		break;

	    /* Now open the tmp iiattribute for read. */

	    status = DIopen(
		&tmp_di_context, 
		(char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		tmp_file_name.name, 
		sizeof(tmp_file_name.name),
		iiatt_pgsize, 
		DI_IO_READ, 
		DI_SYNC_MASK,
		&sys_err);
	}

	if (status != OK)
	    break;

	tmp_open = TRUE;

	/* 
	** Delete iiattribute if it exists. We might have created this file
	** in a previous invocation of upgradedb.
	*/

	status = DIdelete (	
	    &iiatt_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    iiatt_file_name.name, 
	    sizeof(iiatt_file_name.name), 
	    &sys_err);

	/* DI_FILENOTFOUND is an ok bad status. The file might not exist. */

	if  ((status != OK) && (status != DI_FILENOTFOUND))
	    break;

	/* Create the new iiattribute file. */

	status = DIcreate(
	    &iiatt_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    iiatt_file_name.name, 
	    sizeof(iiatt_file_name.name), 
	    iiatt_pgsize, 
	    &sys_err);

	if (status != OK)
	    break;

	/* Open the new iiattribute. */

	status = DIopen(
	    &iiatt_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    iiatt_file_name.name, 
	    sizeof(iiatt_file_name.name),
	    iiatt_pgsize, 
	    DI_IO_WRITE, 
	    DI_SYNC_MASK,
	    &sys_err);

	if (status != OK)
	    break;

	iiatt_open = TRUE;

	dm1c_get_plv(iiatt_pgformat, &local_plv);

	
	/* Format the first pages of the new iiattribute */

	(*local_plv->dmpp_format)(iiatt_pgformat, iiatt_pgsize, 
			iiatt_pg, 0, DMPP_DATA, DM1C_ZERO_FILL);

	/* Set the proper size for the old rows */
	record_size = sizeof(DMP_ATTRIBUTE);

	/* 
	** Now copy the records from the tmp iiattribute to the new iiattribute.
	*/

	for (i=0;;)
	{
	    /* Read a page from the tmp iiattribute */

	    read_count = 1;

	    status = DIread(
		&tmp_di_context, 
		&read_count, 
		i,
		(char *) tmp_pg, 
		&sys_err);

	    if (status != OK)
		break;

	    tid.tid_tid.tid_page = i;
	    tid.tid_tid.tid_line = DM_TIDEOF;

	    /* Copy each line to the new iiattribute. */

	    while (++tid.tid_tid.tid_line <
		     (i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(iiatt_pgformat, 
							tmp_pg))
	    {
		/* We'll use "old_att_row" as an aligned holder; otherwise
		** the rows can be unaligned and we poof.  This is sort of
		** cheating, but "old_att_row" is defined as the largest
		** of the old formats, and they should all have identical
		** stack-alignment requirements.
		*/
		old_att = (char *) &old_att_row;
		local_status = (*local_plv->dmpp_get)(iiatt_pgformat, iiatt_pgsize,
		    tmp_pg, &tid, &record_size, &old_att,
		    0, NULL, NULL, (DMPP_SEG_HDR *)NULL);

		/* If the line is empty, go to the next line. */

		if  (local_status == E_DB_WARN)
		    continue;
		if (local_status != E_DB_OK)
		    break;

		if (old_att != (char *) &old_att_row)
		{
		    MEcopy(old_att, sizeof(old_att_row), &old_att_row);
		    old_att = (char *) &old_att_row;	/* Aligned copy */
		}

		/* Copy the old row to the new one in new_att.
		** We'll ignore att rows for iirelation, iirel_idx,
		** iiattribute, and iiindex.  Since column offsets in
		** these catalogs change around, it's easier to just
		** reconstruct the core catalog att records from the
		** builtin DM_core_attributes.
		*/
		if ( old_att_row.attrelid.db_tab_base <= DM_B_INDEX_TAB_ID )
		    continue;

		/* 
		** Do stuff to iiattribute here, if needed.
		*/


		MEcopy((PTR)&old_att_row, sizeof(DMP_ATTRIBUTE), (PTR)&new_att);


		/* 
		** Store the record on the page. 
		*/

		status = add_record(iiatt_pgformat, iiatt_pgsize,
		    iiatt_pg,
		    (char *)&new_att,
		    sizeof(new_att),
		    &iiatt_tid,
		    &iiatt_di_context,
		    local_plv,
		    dberr);

		if (status != OK)
		    break; 

	    } /* end while */

	    if (status != OK) 
		break;

	    /* Follow any overflow chains. */

	    if (DMPP_VPT_GET_PAGE_OVFL_MACRO(iiatt_pgformat, tmp_pg) != 0)
		i = DMPP_VPT_GET_PAGE_OVFL_MACRO(iiatt_pgformat, tmp_pg);

	    /* 
	    ** No more overflow buckets. We're done with this hash bucket.
	    ** Go to the next hash bucket.
	    */

	    else if (DMPP_VPT_GET_PAGE_MAIN_MACRO(iiatt_pgformat, tmp_pg) != 0) 
		i = DMPP_VPT_GET_PAGE_MAIN_MACRO(iiatt_pgformat, tmp_pg);

	    /* 
	    ** No more main and overflow buckets. We've finished copying
	    ** all the attribute records.
	    */

	    else
	    {
		/*
		** Insert the core catalog iiattribute records.
		** Remember, we earlier ignored these records.
		*/

		i4	j;
		i4 	k;
		bool upcase = (config->cnf_dsc->dsc_dbservice & DU_NAME_UPPER) != 0;

		/* 
		** Figure out the total # of columns in the core catalogs.
		** Its more elegant to do this as
		** j = sizeof(DM_core_attributes)/sizeof(DMP_ATTRIBUTE)
		** but the compiler isn't able to figure out the size of 
		** DM_core_attributes from this module.
		** So loop thru DM_core_relations and add up the relatts field.
		*/
 
		j = 0;
		for (k = 0; k < DM_sizeof_core_rel; k++)
		{
		    j += DM_core_relations[k].relatts;
		}

		for (k = 0; k < j; k++)
		{
		    char    *att_record;
		    
		    att_record = (char *)&DM_core_attributes[k];
		    if (upcase)
			att_record = (char *) &DM_ucore_attributes[k];

		    status = add_record(iiatt_pgformat, iiatt_pgsize,
			iiatt_pg,
			att_record,
			sizeof(DMP_ATTRIBUTE),
			&iiatt_tid,
			&iiatt_di_context,
			local_plv,
			dberr);

		    if (status != OK)
			break; 
		}

		if (status != OK)
		    break; 
		
		/* 
		** Allocate and write out the last iiatt pages and then exit
		** the loop.
		*/

		status = DIalloc(
		    &iiatt_di_context, 1, &iiatt_page_number, &sys_err);

		if (status != OK)
		    break;

		status = DIflush(&iiatt_di_context, &sys_err);

		if (status != OK)
		    break;

		DMPP_VPT_CLR_PAGE_STAT_MACRO(iiatt_pgformat, iiatt_pg, DMPP_MODIFY);
		dm0p_insert_checksum(iiatt_pg, iiatt_pgformat, iiatt_pgsize);

		write_count = 1;

		status = DIwrite(
		    &iiatt_di_context, 
		    &write_count,
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(iiatt_pgformat, iiatt_pg), 
		    (char *) iiatt_pg, 
		    &sys_err);

		break;
	    }
	} /* end for */

	if (status != OK) 
	    break;

	/* Close the new iiattribute file */

	status = DIclose (&iiatt_di_context, &sys_err);

	if  (status != OK)
	    break;
	    
	iiatt_open = FALSE;

	/* 
	** Mark the flag in the config file to say we've completed the 
	** conversion. We must do this before we delete the temporary table.
	** Because we hosed the space management setup for iiattribute,
	** leave the core catalog version alone.  We'll do more "stuff"
	** at database open time.  (This is database add time.)
	*/

	config->cnf_dsc->dsc_status |= DSC_IIATT_V9_CONV_DONE;
	config->cnf_dsc->dsc_status &= ~DSC_65_SMS_DBMS_CATS_CONV_DONE;


	/* Close the configuration file. */

	status = dm0c_close(config, (flag & DM0C_CNF_LOCKED) | DM0C_UPDATE, dberr);

	if (status != E_DB_OK)
	    break;

	config_open = FALSE;

	/*  Release the temporary lock list. */

	if((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKrelease(
		LK_ALL, 
		temp_lock_list, 
		(LK_LKID *)NULL, 
		(LK_LOCK_KEY *)NULL,
		(LK_VALUE *)NULL, 
		&sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL,
			E_DM901B_BAD_LOCK_RELEASE, 
			&sys_err, 
			ULE_LOG , 
			(DB_SQLSTATE *)NULL, 
			(char *)NULL, 
			0L, 
			(i4 *)NULL, 
			&local_error, 
			1, 
			sizeof(temp_lock_list), 
			temp_lock_list);

		break;
	    }
	}
	/* Close and delete the tmp iiattribute file. */

	status = DIclose (&tmp_di_context, &sys_err);

	if (status != OK)
	    break;

	tmp_open = FALSE;

	status = DIdelete (	
	    &tmp_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    tmp_file_name.name, 
	    sizeof(tmp_file_name.name), 
	    &sys_err);

	if (status != OK)
	    break;

	/* Return success. */

	dm0m_pgdealloc((char **)&iiatt_pg);
	dm0m_pgdealloc((char **)&tmp_pg);
	return (E_DB_OK);
    } /* end for */

    /* Error, clean up */

    if (iiatt_pg != NULL)
	dm0m_pgdealloc((char **)&iiatt_pg);
    if (tmp_pg != NULL)
	dm0m_pgdealloc((char **)&tmp_pg);

    /* Deallocate the configuration record. */

    if (config_open == TRUE)
	local_status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

    /* Close the iiattribute files if they are open. */

    if (tmp_open == TRUE)
	local_status = DIclose (&tmp_di_context, &sys_err);

    if (iiatt_open == TRUE)
	local_status = DIclose (&iiatt_di_context, &sys_err);

    SETDBERR(dberr, 0, E_DM93F0_CONVERT_IIATT);
    return (E_DB_ERROR);
}
/*{
** Name: dm2d_convert_preV9_iiind - convert iiindex to V9 format
**
** Description:
**      Used indirectly by upgradedb to convert iiindex to V9 format
**
**	V9 is DMF version for Ingres2007
**
**	For Ingres2007, the "idom" column of IIINDEX is expanded from
**	DB_MAXKEYS to DB_MAXIXATTS, so extant IIINDEX rows must have
**	these added idom's zero-filled.
**
** Inputs:
**      db_id                           database to upgrade
**
** Outputs:
**      err_code                        error return code
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-May-2006 (jenjo02)
**	    Written to expand number of IIINDEX idom columns from
**	    32 to 64 for Indexes on Clustered Tables.
[@history_template@]...
*/
static DB_STATUS
dm2d_convert_preV9_iiind(
    DMP_DCB	    *db_id,
    i4		    create_version,
    i4              flag,
    i4              lock_list,
    DB_ERROR   	    *dberr )
{
    DMP_DCB	    *dcb = (DMP_DCB *)db_id;
    i4	    	    temp_lock_list, status, i=0, local_error, local_status;
    CL_ERR_DESC	    sys_err;
    DM0C_CNF	    *config = 0;
    bool	    config_open = FALSE, tmp_open = FALSE;
    bool	    iiind_open = FALSE, add_status;
    DB_TAB_ID	    iiind_tab_id;
    DM_FILE	    tmp_file_name = 
			{{'i','i','i','n','d','c','v','t','.','t','m','p'}};
    DM_FILE	    iiind_file_name;
    DI_IO	    tmp_di_context, iiind_di_context;
    DMPP_PAGE	    *iiind_pg, *tmp_pg;
    i4	    	    iiind_page_number=0;
    i4	    	    read_count, write_count, line, offset;
    char    	    *old_ind;
    char	    *old_ind_ptr;
    DMP_INDEX_30    old_ind_row;
    DMP_INDEX       new_ind;
    DM_TID	    iiind_tid;
    DMPP_ACC_PLV    *local_plv;
    DM_TID	    tid;
    i4	    	    record_size;
    i4      	    iiind_pgformat;
    i4      	    iiind_pgsize;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
 
    for (;;)
    {
	/*  
	**  Create a temporary lock list. This is needed to open the config
	**  file. 
	*/
	if((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKcreate_list(
		LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT,
		(i4)0, 
		(LK_UNIQUE *)NULL, 
		(LK_LLID *)&temp_lock_list, 
		0,
		&sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL,
			E_DM901A_BAD_LOCK_CREATE, 
			&sys_err, 
			ULE_LOG, 
			(DB_SQLSTATE *)NULL, 
			(char *)NULL, 
			0L, 
			(i4 *)NULL, 
			&local_error, 
			0);

	    	break;
	    }
	}
	else
	{
	    temp_lock_list = lock_list;
	}

	/*  Open the config file. */

	status = dm0c_open(dcb, flag, temp_lock_list, &config, dberr);

	if (status != E_DB_OK)
	    break;

	config_open = TRUE;

	/*
	** Check the status of the upgrade. Its possible that upgradedb failed
	** and we're trying again. In that case upgradedb might have
	** failed after completing the convert.
	*/

	if  ((config->cnf_dsc->dsc_status & DSC_IIIND_V9_CONV_DONE) != 0)
	{
	    /* We've already completed the conversion. */

	    /* If it still exists, delete the temporary file. */

	    status = DIdelete (	
		&tmp_di_context, 
		(char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		tmp_file_name.name, 
		sizeof(tmp_file_name.name), 
		&sys_err);

	    if ((status != OK) && (status != DI_FILENOTFOUND))
		break;

	    /* Close the configuration file. */

	    status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

	    if (status != E_DB_OK)
		break;

	    /*  Release the temporary lock list. */

          if((flag & DM0C_CNF_LOCKED) == 0)
          {
	    status = LKrelease(
		LK_ALL, 
		temp_lock_list, 
		(LK_LKID *)NULL, 
		(LK_LOCK_KEY *)NULL,
		(LK_VALUE *)NULL, 
		&sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL,
		    E_DM901B_BAD_LOCK_RELEASE, 
		    &sys_err, 
		    ULE_LOG , 
		    (DB_SQLSTATE *)NULL, 
		    (char *)NULL, 
		    0L, 
		    (i4 *)NULL, 
		    &local_error, 
		    1, 
		    sizeof(temp_lock_list), 
		    temp_lock_list);

		break;
	    }
          }
	    return (E_DB_OK);
	}

	/*  Generate the filenames for iiindex tables. */

	iiind_tab_id.db_tab_base = DM_B_INDEX_TAB_ID;
	iiind_tab_id.db_tab_index = DM_I_INDEX_TAB_ID;

	dm2f_filename (DM2F_TAB, &iiind_tab_id, 0, 0, 0, &iiind_file_name);

	/* 
	** Check if we've already renamed iiindex in a previous invocation
	** of upgradedb. If the file exists, we shouldn't do any renaming.
	*/ 

	/* Determine the page format of iiindex.  These will have
	** been set properly by the iirelation conversion.
	*/

	iiind_pgsize = config->cnf_dsc->dsc_iiind_relpgsize;
	iiind_pgformat = config->cnf_dsc->dsc_iiind_relpgtype;

	if ((iiind_pg = (DMPP_PAGE *)dm0m_pgalloc(iiind_pgsize)) == 0)
	{
	    /* FIX ME fix me error */
	    break;
	}
	if ((tmp_pg = (DMPP_PAGE *)dm0m_pgalloc(iiind_pgsize)) == 0)
	{
	    /* FIX ME fix me error */
	    break;
	}

	/* Open the tmp iiindex for read. */

 	status = DIopen(
	    &tmp_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    tmp_file_name.name, 
	    sizeof(tmp_file_name.name),
	    iiind_pgsize, 
	    DI_IO_READ, 
	    DI_SYNC_MASK,
	    &sys_err);

	/* If the file doesn't exist, we need to rename iiindex. */

	if (status == DI_FILENOTFOUND)
	{
	    /*  Rename iiindex to the tmp filename. */

	    status = DIrename (
		0, 
		(char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		iiind_file_name.name,
		sizeof(iiind_file_name.name),	     
		tmp_file_name.name,
		sizeof(tmp_file_name.name),
		&sys_err);

	    if (status != OK)
		break;

	    /* Now open the tmp iiindex for read. */

	    status = DIopen(
		&tmp_di_context, 
		(char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		tmp_file_name.name, 
		sizeof(tmp_file_name.name),
		iiind_pgsize, 
		DI_IO_READ, 
		DI_SYNC_MASK,
		&sys_err);
	}

	if (status != OK)
	    break;

	tmp_open = TRUE;

	/* 
	** Delete iiindex if it exists. We might have created this file
	** in a previous invocation of upgradedb.
	*/

	status = DIdelete (	
	    &iiind_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    iiind_file_name.name, 
	    sizeof(iiind_file_name.name), 
	    &sys_err);

	/* DI_FILENOTFOUND is an ok bad status. The file might not exist. */

	if  ((status != OK) && (status != DI_FILENOTFOUND))
	    break;

	/* Create the new iiindex file. */

	status = DIcreate(
	    &iiind_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    iiind_file_name.name, 
	    sizeof(iiind_file_name.name), 
	    iiind_pgsize, 
	    &sys_err);

	if (status != OK)
	    break;

	/* Open the new iiindex. */

	status = DIopen(
	    &iiind_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    iiind_file_name.name, 
	    sizeof(iiind_file_name.name),
	    iiind_pgsize, 
	    DI_IO_WRITE, 
	    DI_SYNC_MASK,
	    &sys_err);

	if (status != OK)
	    break;

	iiind_open = TRUE;

	dm1c_get_plv(iiind_pgformat, &local_plv);

	
	/* Format the first pages of the new iiindex */

	(*local_plv->dmpp_format)(iiind_pgformat, iiind_pgsize, 
			iiind_pg, 0, DMPP_DATA, DM1C_ZERO_FILL);

	/* Set the proper size for the old rows */
	record_size = DMP_INDEX_SIZE_30;

	/* 
	** Now copy the records from the tmp iiindex to the new iiindex.
	*/

	for (i=0;;)
	{
	    /* Read a page from the tmp iiindex */

	    read_count = 1;

	    status = DIread(
		&tmp_di_context, 
		&read_count, 
		i,
		(char *) tmp_pg, 
		&sys_err);

	    if (status != OK)
		break;

	    tid.tid_tid.tid_page = i;
	    tid.tid_tid.tid_line = DM_TIDEOF;

	    /* Copy each line to the new iiindex. */

	    while (++tid.tid_tid.tid_line <
		     (i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(iiind_pgformat, 
							tmp_pg))
	    {
		/* We'll use "old_ind_row" as an aligned holder; otherwise
		** the rows can be unaligned and we poof.
		*/
		old_ind = (char *) &old_ind_row;
		local_status = (*local_plv->dmpp_get)(iiind_pgformat, iiind_pgsize,
		    tmp_pg, &tid, &record_size, &old_ind,
		    0, NULL, NULL, (DMPP_SEG_HDR *)NULL);

		/* If the line is empty, go to the next line. */

		if  (local_status == E_DB_WARN)
		    continue;
		if (local_status != E_DB_OK)
		    break;

		/* Copy the old row to the new one in new_ind,
		** padding out the new idom[32-63] with zeroes.
		** We'll include iiindex rows for the core catalogs
		** (there's only one).
		*/
		MEmove(DMP_INDEX_SIZE_30, (PTR)old_ind, 0,
		       DMP_INDEX_SIZE, (PTR)&new_ind);


		/* 
		** Store the record on the page. 
		*/

		status = add_record(iiind_pgformat, iiind_pgsize,
		    iiind_pg,
		    (char *)&new_ind,
		    DMP_INDEX_SIZE,
		    &iiind_tid,
		    &iiind_di_context,
		    local_plv,
		    dberr);

		if (status != OK)
		    break; 

	    } /* end while */

	    if (status != OK) 
		break;

	    /* Follow any overflow chains. */

	    if (DMPP_VPT_GET_PAGE_OVFL_MACRO(iiind_pgformat, tmp_pg) != 0)
		i = DMPP_VPT_GET_PAGE_OVFL_MACRO(iiind_pgformat, tmp_pg);

	    /* 
	    ** No more overflow buckets. We're done with this hash bucket.
	    ** Go to the next hash bucket.
	    */

	    else if (DMPP_VPT_GET_PAGE_MAIN_MACRO(iiind_pgformat, tmp_pg) != 0) 
		i = DMPP_VPT_GET_PAGE_MAIN_MACRO(iiind_pgformat, tmp_pg);

	    /* 
	    ** No more main and overflow buckets. We've finished copying
	    ** all the iiindex records.
	    */
	    else
	    {
		/* 
		** Allocate and write out the last iiindex pages and then exit
		** the loop.
		*/

		status = DIalloc(
		    &iiind_di_context, 1, &iiind_page_number, &sys_err);

		if (status != OK)
		    break;

		status = DIflush(&iiind_di_context, &sys_err);

		if (status != OK)
		    break;

		DMPP_VPT_CLR_PAGE_STAT_MACRO(iiind_pgformat, iiind_pg, DMPP_MODIFY);
		dm0p_insert_checksum(iiind_pg, iiind_pgformat, iiind_pgsize);

		write_count = 1;

		status = DIwrite(
		    &iiind_di_context, 
		    &write_count,
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(iiind_pgformat, iiind_pg), 
		    (char *) iiind_pg, 
		    &sys_err);

		break;
	    }
	} /* end for */

	if (status != OK) 
	    break;

	/* Close the new iiindex file */

	status = DIclose (&iiind_di_context, &sys_err);

	if  (status != OK)
	    break;
	    
	iiind_open = FALSE;

	/* 
	** Mark the flag in the config file to say we've completed the 
	** conversion. We must do this before we delete the temporary table.
	** Because we hosed the space management setup for iiindex,
	** leave the core catalog version alone.  We'll do more "stuff"
	** at database open time.  (This is database add time.)
	*/

	config->cnf_dsc->dsc_status |= DSC_IIIND_V9_CONV_DONE;
	config->cnf_dsc->dsc_status &= ~DSC_65_SMS_DBMS_CATS_CONV_DONE;


	/* Close the configuration file. */

	status = dm0c_close(config, (flag & DM0C_CNF_LOCKED) | DM0C_UPDATE, dberr);

	if (status != E_DB_OK)
	    break;

	config_open = FALSE;

	/*  Release the temporary lock list. */

	if((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKrelease(
		LK_ALL, 
		temp_lock_list, 
		(LK_LKID *)NULL, 
		(LK_LOCK_KEY *)NULL,
		(LK_VALUE *)NULL, 
		&sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL,status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL,
			E_DM901B_BAD_LOCK_RELEASE, 
			&sys_err, 
			ULE_LOG , 
			(DB_SQLSTATE *)NULL, 
			(char *)NULL, 
			0L, 
			(i4 *)NULL, 
			&local_error, 
			1, 
			sizeof(temp_lock_list), 
			temp_lock_list);

		break;
	    }
	}
	/* Close and delete the tmp iiindex file. */

	status = DIclose (&tmp_di_context, &sys_err);

	if (status != OK)
	    break;

	tmp_open = FALSE;

	status = DIdelete (	
	    &tmp_di_context, 
	    (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    tmp_file_name.name, 
	    sizeof(tmp_file_name.name), 
	    &sys_err);

	if (status != OK)
	    break;

	/* Return success. */

	dm0m_pgdealloc((char **)&iiind_pg);
	dm0m_pgdealloc((char **)&tmp_pg);
	return (E_DB_OK);
    } /* end for */

    /* Error, clean up */

    if (iiind_pg != NULL)
	dm0m_pgdealloc((char **)&iiind_pg);
    if (tmp_pg != NULL)
	dm0m_pgdealloc((char **)&tmp_pg);

    /* Deallocate the configuration record. */

    if (config_open == TRUE)
	local_status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

    /* Close the iiindex files if they are open. */

    if (tmp_open == TRUE)
	local_status = DIclose (&tmp_di_context, &sys_err);

    if (iiind_open == TRUE)
	local_status = DIclose (&iiind_di_context, &sys_err);

    SETDBERR(dberr, 0, E_DM93F0_CONVERT_IIATT);
    return (E_DB_ERROR);
}


/*{
** Name: dm2d_upgrade_rewrite_iiatt - convert iiattribute to CURRENT format
**
** Description:
** 
**      Used by upgradedb to convert iiattribute to CURRENT format
**      whenever the size of iiattribute has increased.
**      This rewrite occurs at a very low physical level,
**      avoiding all the usual DMF buffering and table handling.
**
**      - Renames iiattribute to a temp file name
**      - Reads iiattribute records from the temp file
**      - Call upgrade_iiatt_row() to convert the row to the CURRENT format
**      - Adds the CURRENT iiattribute row to a new iiattribute file
** 
**      The new iiattribute created is a single bucket HASH.
**      When iirelation is upgraded, the corresponding iirelation record
**      must be updated to have relprim = 1;
**
**      One thing to note about this routine is that when scanning the
**      temp iiattribute file, attributes from CORE catalogs are ignored
**      since the nature of the upgrade is such that the number and/or
**      size/type of core catalog attributes has changed.
**      At the end we will insert the current version of CORE catalog
**      attributes as defined in DM_core_relations used by createdb.
**
**      This routine was created from dm2d_convert_preV7_iiatt,
**      and other code in this file that does the row conversion.
**      Future iiattribute upgrades requiring iiattribute re-write
**      should require changes to upgrade_iiatt_row() only.
**  
**      V7 is DMF version for Ingres Release 3 (3.0.1).
**      V8 is DMF version for release 3.0.2.
**      As far as we're concerned, the two are identical (V8 uses an i2
**      of attfree for attcollid, but we just zero it here anyway).
**
**      For the 256K row project, iiattribute.attoff changed from i2 -> i4
**      To accommodate i4 attoff, attfmt has been moved into the attfree space.
**
** Inputs:
**      db_id                           database to upgrade
**
** Outputs:
**      err_code                        error return code
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-apr-2007
**          Created from dm2d_convert_preV7_iiatt
[@history_template@]...
*/
static DB_STATUS
dm2d_upgrade_rewrite_iiatt(
    DM2D_UPGRADE    *upgrade_cb,
    DMP_DCB	    *db_id,
    i4              flag,
    i4              lock_list,
    DB_ERROR	    *dberr )
{
    DMP_DCB	    *dcb = (DMP_DCB *)db_id;
    i4	    lk_id, status, i=0, local_error, local_status;
    CL_ERR_DESC	    sys_err;
    DM_OBJECT	    *mem = NULL;
    DM0C_CNF	    *config = 0;
    bool	    config_open = FALSE, tmp_open = FALSE;
    bool	    att_open = FALSE;
    DB_TAB_ID	    att_tab_id;
    DM_FILE	    tmp_file = 
			{{'i','i','a','t','t','c','v','t','.','t','m','p'}};
    DM_FILE	    att_file;
    DI_IO	    tmp_di_io, att_di_io;
    DMPP_PAGE	    *att_pg, *tmp_pg;
    i4	    att_pageno=0;
    i4	    read_count, write_count, line, offset;
    DMP_ATTRIBUTE   tmp_att;
    DMP_ATTRIBUTE   new_att;
    char	    *old_att;
    DM_TID	    att_tid;
    DMPP_ACC_PLV    *loc_plv;
    DM_TID	    tid;
    i4	    record_size;
    i4      pgtype;
    i4      pgsize;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
 
    for (;;)
    {
	/*  
	**  Create a temporary lock list. This is needed to open the config
	**  file. 
	*/
	if((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKcreate_list(
		LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT,
		(i4)0, (LK_UNIQUE *)NULL, (LK_LLID *)&lk_id, 0, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, 
			(DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
			&local_error, 0);
	    	break;
	    }
	}
	else
	{
	    lk_id = lock_list;
	}

	/*  Open the config file. */

	status = dm0c_open(dcb, flag, lk_id, &config, dberr);

	if (status != E_DB_OK)
	    break;

	config_open = TRUE;

	/*
	** Check the status of the upgrade. Its possible that upgradedb failed
	** and we're trying again. In that case upgradedb might have
	** failed after completing the convert.
	*/

	if  (upgrade_cb->upgr_dbg_env == UPGRADE_REWRITE)
	{
	    /* for now, always do the rewrite ! */
	    TRdisplay("UPGRADE rewrite iiattribute config status %x\n",
		config->cnf_dsc->dsc_status);
	}
	else if ((config->cnf_dsc->dsc_status & DSC_IIATT_V7_CONV_DONE) != 0)
	{
	    /* We've already completed the conversion. */

	    /* If it still exists, delete the temporary file. */

	    status = DIdelete (	&tmp_di_io, 
		(char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		tmp_file.name, sizeof(tmp_file.name), &sys_err);

	    if ((status != OK) && (status != DI_FILENOTFOUND))
		break;

	    /* Close the configuration file. */

	    status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

	    if (status != E_DB_OK)
		break;

	    /*  Release the temporary lock list. */

          if((flag & DM0C_CNF_LOCKED) == 0)
          {
	    status = LKrelease(LK_ALL, lk_id, (LK_LKID *)NULL,
		(LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err); 

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG, 
		    (DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
		    &local_error, 1, sizeof(lk_id), lk_id);
		break;
	    }
          }
	    return (E_DB_OK);
	}

	/*  Generate the filenames for iiattribute tables. */

	att_tab_id.db_tab_base = DM_B_ATTRIBUTE_TAB_ID;
	att_tab_id.db_tab_index = DM_I_ATTRIBUTE_TAB_ID;

	dm2f_filename (DM2F_TAB, &att_tab_id, 0, 0, 0, &att_file);

	/* 
	** Check if we've already renamed iiattribute in a previous invocation
	** of upgradedb. If the file exists, we shouldn't do any renaming.
	*/ 

	/* Determine the page format of iiattribute.  These will have
	** been set properly by the iirelation conversion.
	*/

	pgsize = config->cnf_dsc->dsc_iiatt_relpgsize;
	pgtype = config->cnf_dsc->dsc_iiatt_relpgtype;

	/*
	** NOTE allocate additional sizeof(DMP_ATTRIBUTE) so we can
	** safely copy sizeof(DMP_ATTRIBUTE) bytes instead of
	** worrying here about what is the correct size of the old row
	** Also note that we're NOT using direct IO here, so don't
	** worry about alignments.
	*/
	status = dm0m_allocate(2*pgsize + sizeof(DMP_ATTRIBUTE) +
		+ sizeof(DMP_MISC),
	    (i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	    (char*)dcb, (DM_OBJECT**)&mem, dberr);
	if ( status )
	    break;
	att_pg = (DMPP_PAGE *)((char *)mem + sizeof(DMP_MISC));
	tmp_pg = (DMPP_PAGE *)((char *)att_pg + pgsize);
	((DMP_MISC*)mem)->misc_data = (char*)mem + sizeof(DMP_MISC);
	
	/* Open the tmp iiattribute for read. */

 	status = DIopen(&tmp_di_io,
	    (char *) &dcb->dcb_location.physical, dcb->dcb_location.phys_length,
	    tmp_file.name, sizeof(tmp_file.name),
	    pgsize, DI_IO_READ, DI_SYNC_MASK, &sys_err);

	/* If the file doesn't exist, we need to rename iiattribute. */

	if (status == DI_FILENOTFOUND)
	{
	    /*  Rename iiattribute to the tmp filename. */

	    status = DIrename ( 0, (char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		att_file.name, sizeof(att_file.name),	     
		tmp_file.name, sizeof(tmp_file.name), &sys_err);

	    if (status != OK)
		break;

	    /* Now open the tmp iiattribute for read. */

	    status = DIopen( &tmp_di_io, (char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		tmp_file.name, sizeof(tmp_file.name),
		pgsize, DI_IO_READ, DI_SYNC_MASK, &sys_err);
	}

	if (status != OK)
	    break;

	tmp_open = TRUE;

	/* 
	** Delete iiattribute if it exists. We might have created this file
	** in a previous invocation of upgradedb.
	*/

	status = DIdelete ( &att_di_io, (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    att_file.name, sizeof(att_file.name), &sys_err);

	/* DI_FILENOTFOUND is an ok bad status. The file might not exist. */

	if  ((status != OK) && (status != DI_FILENOTFOUND))
	    break;

	/* Create the new iiattribute file. */

	status = DIcreate( &att_di_io, (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    att_file.name, sizeof(att_file.name), pgsize, &sys_err);

	if (status != OK)
	    break;

	/* Open the new iiattribute. */

	status = DIopen( &att_di_io, (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    att_file.name, sizeof(att_file.name), pgsize, 
	    DI_IO_WRITE, DI_SYNC_MASK, &sys_err);

	if (status != OK)
	    break;

	att_open = TRUE;

	dm1c_get_plv(pgtype, &loc_plv);
	
	/* Format the first pages of the new iiattribute */

	(*loc_plv->dmpp_format)(pgtype, pgsize, 
			att_pg, 0, DMPP_DATA, DM1C_ZERO_FILL);

	/* Set size for the old rows, (even though dmpp_get doesn't care) */
	if (upgrade_cb->upgr_create_version < DSC_V8)
	    record_size = sizeof(DMP_ATTRIBUTE_26);
	if (upgrade_cb->upgr_create_version < DSC_V3)
	    record_size = sizeof(DMP_OLDATTRIBUTE);
	else if (upgrade_cb->upgr_create_version == DSC_V3)
	    record_size = sizeof(DMP_ATTRIBUTE_1X);
	else
	    record_size = 0;

	/* Copy the records from the tmp iiattribute to the new iiattribute */

	for (i=0;;)
	{
	    /* Read a page from the tmp iiattribute */

	    read_count = 1;

	    status = DIread( &tmp_di_io, &read_count, i, (char *) tmp_pg, &sys_err);

	    if (status != OK)
		break;

	    tid.tid_tid.tid_page = i;
	    tid.tid_tid.tid_line = DM_TIDEOF;

	    /* Copy each line to the new iiattribute. */

	    while (++tid.tid_tid.tid_line <
		     (i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(pgtype, 
							tmp_pg))
	    {
		/* We'll use "old_att_row" as an aligned holder; otherwise
		** the rows can be unaligned and we poof.  This is sort of
		** cheating, but "old_att_row" is defined as the largest
		** of the old formats, and they should all have identical
		** stack-alignment requirements.
		*/
		old_att = (char *) &tmp_att;
		local_status = (*loc_plv->dmpp_get)(pgtype, pgsize,
		    tmp_pg, &tid, &record_size, &old_att,
		    (i4*)NULL, (u_i4*)NULL, (u_i2*)NULL, (DMPP_SEG_HDR *)NULL);

		/* If the line is empty, go to the next line. */

		if  (local_status == E_DB_WARN)
		    continue;
		if (local_status != E_DB_OK)
		    break;

		if (old_att != (char *) &tmp_att)
		{
		    /*
		    ** Note old_att may not be sizeof(DMP_ATTRIBUTE)
		    ** but we can always copy sizeof(DMP_ATTRIBUTE)
		    ** because we allocated 2*pgsize + sizeof(DMP_ATTRIBUTE) 
		    */
		    MEcopy(old_att, sizeof(DMP_ATTRIBUTE), (char *)&tmp_att);
		    old_att = (char *)&tmp_att;	/* Aligned copy */
		}

		/* UPGRADE the row to the CURRENT format */

		upgrade_iiatt_row(upgrade_cb, old_att, &new_att);

		/*
		** Ignore att rows for iirelation, iirel_idx,
		** iiattribute, and iiindex.  Since column offsets in
		** these catalogs change around, it's easier to just
		** reconstruct the core catalog att records from the
		** builtin DM_core_attributes.
		*/
	        if (new_att.attrelid.db_tab_base <= DM_B_INDEX_TAB_ID)
		    continue;

		/* Store the record on the page */

		status = add_record(pgtype, pgsize, att_pg,
		    (char *)&new_att, sizeof(new_att),
		    &att_tid, &att_di_io, loc_plv, dberr);

		if (status != OK)
		    break; 

	    } /* end while */

	    if (status != OK) 
		break;

	    /* Follow any overflow chains. */

	    if (DMPP_VPT_GET_PAGE_OVFL_MACRO(pgtype, tmp_pg) != 0)
		i = DMPP_VPT_GET_PAGE_OVFL_MACRO(pgtype, tmp_pg);

	    /* 
	    ** No more overflow buckets. We're done with this hash bucket.
	    ** Go to the next hash bucket.
	    */

	    else if (DMPP_VPT_GET_PAGE_MAIN_MACRO(pgtype, tmp_pg) != 0) 
		i = DMPP_VPT_GET_PAGE_MAIN_MACRO(pgtype, tmp_pg);

	    /* 
	    ** No more main and overflow buckets. We've finished copying
	    ** all the attribute records.
	    */

	    else
	    {
		/*
		** Insert the core catalog iiattribute records.
		** Remember, we earlier ignored these records.
		*/

		i4	j;
		i4 k;
		bool upcase = (config->cnf_dsc->dsc_dbservice & DU_NAME_UPPER) != 0;

		/* 
		** Figure out the total # of columns in the core catalogs.
		** Its more elegant to do this as
		** j = sizeof(DM_core_attributes)/sizeof(DMP_ATTRIBUTE)
		** but the compiler isn't able to figure out the size of 
		** DM_core_attributes from this module.
		** So loop thru DM_core_relations and add up the relatts field.
		*/
 
		j=0;
		for (k=0;k<4;k++)
		{
		    j += DM_core_relations[k].relatts;
		}

		for (k=0;k<j;k++)
		{
		    char    *att_record;
		    
		    att_record = (char *)&DM_core_attributes[k];
		    if (upcase)
			att_record = (char *) &DM_ucore_attributes[k];

		    status = add_record(pgtype, pgsize, att_pg, att_record,
			sizeof(DMP_ATTRIBUTE), &att_tid, &att_di_io,
			loc_plv, dberr);

		    if (status != OK)
			break; 
		}

		if (status != OK)
		    break; 
		
		/* 
		** Allocate and write out the last iiatt pages and then exit
		** the loop.
		*/

		status = DIalloc( &att_di_io, 1, &att_pageno, &sys_err);

		if (status != OK)
		    break;

		status = DIflush(&att_di_io, &sys_err);

		if (status != OK)
		    break;

		DMPP_VPT_CLR_PAGE_STAT_MACRO(pgtype, att_pg, DMPP_MODIFY);
		dm0p_insert_checksum(att_pg, pgtype, pgsize);

		write_count = 1;

		TRdisplay("Write iiattribute page %d nxtlino %d \n", 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, att_pg), 
		(i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(pgtype, att_pg));

		status = DIwrite( &att_di_io, &write_count,
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, att_pg), 
		    (char *) att_pg, &sys_err);

		break;
	    }
	} /* end for */

	if (status != OK) 
	    break;

	/* Close the new iiattribute file */

	status = DIclose (&att_di_io, &sys_err);

	if  (status != OK)
	    break;
	    
	att_open = FALSE;

	/* 
	** Mark the flag in the config file to say we've completed the 
	** conversion. We must do this before we delete the temporary table.
	*/

	config->cnf_dsc->dsc_status |= DSC_IIATT_V7_CONV_DONE;

	/*
	** Because we hosed the space management setup for iiattribute,
	** leave the core catalog version alone.  We'll do more "stuff"
	** at database open time.  (This is database add time.)
	*/
	config->cnf_dsc->dsc_status &= ~DSC_65_SMS_DBMS_CATS_CONV_DONE;

	/* Close the configuration file. */

	status = dm0c_close(config, (flag & DM0C_CNF_LOCKED) | DM0C_UPDATE, dberr);

	if (status != E_DB_OK)
	    break;

	config_open = FALSE;

	/*  Release the temporary lock list. */

	if((flag & DM0C_CNF_LOCKED) == 0)
	{
	    status = LKrelease( LK_ALL, lk_id, (LK_LKID *)NULL, 
		(LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
			(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG , 
			(DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
			&local_error, 1, sizeof(lk_id), lk_id);

		break;
	    }
	}
	/* Close and delete the tmp iiattribute file. */

	status = DIclose (&tmp_di_io, &sys_err);

	if (status != OK)
	    break;

	tmp_open = FALSE;

	status = DIdelete ( &tmp_di_io, (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length, tmp_file.name, 
	    sizeof(tmp_file.name), &sys_err);

	if (status != OK)
	    break;

	dm0m_deallocate(&mem);

	/* Return success. */
	return (E_DB_OK);

    } /* end for */

    /* Error, clean up */

    if (mem != NULL)
	dm0m_deallocate(&mem);

    /* Deallocate the configuration record. */

    if (config_open == TRUE)
	local_status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

    /* Close the iiattribute files if they are open. */

    if (tmp_open == TRUE)
	local_status = DIclose (&tmp_di_io, &sys_err);

    if (att_open == TRUE)
	local_status = DIclose (&att_di_io, &sys_err);

    SETDBERR(dberr, 0, E_DM93F0_CONVERT_IIATT);
    return (E_DB_ERROR);
}


/*{
** Name: upgrade_iiatt_row - convert iiattribute to CURRENT format
**
** Description:
** 
**      Used by upgradedb to convert iiattribute to CURRENT format
**      NOTE previously for each upgrade an iiattribute row was upgraded
**      like this:
**      64iiatt -> current iiatt
**      V3iiatt -> current iiatt
**      26iatt -> current iiatt
**
** A simpler way to upgrade would be this:
**      64iiatt -> V3iiatt
**      V3iiatt -> 26iatt
**      26iiatt -> nextversion
**
** That is, convert the row from version to next version
** New conversions should convert from previous version only
** (which should be simpler and cause fewer upgrade bugs)
**
** Inputs:
**      db_id                           database to upgrade
**
** Outputs:
**      err_code                        error return code
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-apr-2007
**          Created.
**      29-Sept-2009 (troal01)
**      	Added support for geospatial types.
[@history_template@]...
*/
static VOID
upgrade_iiatt_row(
DM2D_UPGRADE    *upgrade_cb,
char		*old_att,
DMP_ATTRIBUTE	*new_attp)
{
    DMP_OLDATTRIBUTE *old_att_64 = NULL;
    DMP_ATTRIBUTE_1X *old_att_1x = NULL;
    DMP_ATTRIBUTE_26 *old_att_26 = NULL;
    DMP_ATTRIBUTE_V8 att_v8;
    DMP_ATTRIBUTE    att_v9;
    i4		     row_version = upgrade_cb->upgr_create_version;
    char	     *tmp_att = old_att;

    MEfill(sizeof(att_v8), 0, (PTR) &att_v8);
    MEfill(sizeof(att_v9), 0, (PTR) &att_v9);

    for ( ; row_version != DSC_VCURRENT ; )
    {
	if (row_version < DSC_V3)
	{
	    old_att_64 = (DMP_OLDATTRIBUTE *) tmp_att;
	    STRUCT_ASSIGN_MACRO(old_att_64->attrelid, att_v8.attrelid);
	    att_v8.attid = old_att_64->attid;
	    att_v8.attxtra = old_att_64->attxtra;
	    dm2d_cname((char *)&att_v8.attname, old_att_64->attname,
		sizeof(att_v8.attname));
	    att_v8.attoff = old_att_64->attoff;
	    att_v8.attfmt = old_att_64->attfmt;
	    att_v8.attfml = old_att_64->attfml;
	    att_v8.attfmp = old_att_64->attfmp;
	    att_v8.attkey = old_att_64->attkey;
	    att_v8.attflag = old_att_64->attflag;
	    att_v8.attintl_id = old_att_64->attid;
	    makeDefaultID( &att_v8.attDefaultID, old_att_64 );
	    if ( old_att_64->attfmt > 0 )
		att_v8.attflag |= ATT_KNOWN_NOT_NULLABLE;

	    /* now the row is DSC_V8 */	
	    row_version = DSC_V8;
	    tmp_att = (char *)&att_v8;
	}

	if (row_version == DSC_VCURRENT)
	    break;
	else if (row_version == DSC_V3)
	{
	    old_att_1x = (DMP_ATTRIBUTE_1X *) tmp_att;
	    STRUCT_ASSIGN_MACRO(old_att_1x->attrelid, att_v8.attrelid);
	    att_v8.attid = old_att_1x->attid;
	    att_v8.attxtra = old_att_1x->attxtra;
	    MEmove(sizeof(old_att_1x->attname), (char *)&old_att_1x->attname, 
		' ', sizeof(att_v8.attname), (char *)&att_v8.attname);
	    att_v8.attoff = old_att_1x->attoff;
	    att_v8.attfmt = old_att_1x->attfmt;
	    att_v8.attfml = old_att_1x->attfml;
	    att_v8.attfmp = old_att_1x->attfmp;
	    att_v8.attkey = old_att_1x->attkey;
	    att_v8.attflag = old_att_1x->attflag;
	    STRUCT_ASSIGN_MACRO(old_att_1x->attDefaultID, att_v8.attDefaultID);
	    att_v8.attintl_id = old_att_1x->attid;

	    /* now the row is DSC_V8 */	
	    row_version = DSC_V8;
	    tmp_att = (char *)&att_v8;
	}

	if (row_version == DSC_VCURRENT)
	    break;
	else if (row_version < DSC_V8)
	{
	    old_att_26 = (DMP_ATTRIBUTE_26 *) tmp_att;
	    STRUCT_ASSIGN_MACRO(old_att_26->attrelid, att_v8.attrelid);
	    att_v8.attid = old_att_26->attid;
	    att_v8.attxtra = old_att_26->attxtra;
	    MEmove(sizeof(old_att_26->attname), (char *)&old_att_26->attname, 
		' ', sizeof(att_v8.attname), (char *)&att_v8.attname);
	    att_v8.attoff = old_att_26->attoff;
	    att_v8.attfmt = old_att_26->attfmt;
	    att_v8.attfml = old_att_26->attfml;
	    att_v8.attfmp = old_att_26->attfmp;
	    att_v8.attkey = old_att_26->attkey;
	    att_v8.attflag = old_att_26->attflag;
	    STRUCT_ASSIGN_MACRO(old_att_26->attDefaultID, att_v8.attDefaultID);
	    att_v8.attintl_id = old_att_26->attintl_id;
	    att_v8.attver_added = old_att_26->attver_added;
	    att_v8.attver_dropped = old_att_26->attver_dropped;
	    att_v8.attval_from = old_att_26->attval_from;

	    /* now the row is DSC_V8 */	
	    row_version = DSC_V8;
	    tmp_att = (char *)&att_v8;
	}

	/* more upgrade for V8, current version should be in att_v8 */
	if (row_version == DSC_V8 && upgrade_cb->upgr_create_version < DSC_V8)
	{
	    /* Make security label columns into CHAR so ADF doesn't
	    ** barf on them.
	    */
	    if (abs(att_v8.attfmt) == 60 || abs(att_v8.attfmt) == 61)
	    {
		i4 old_type = att_v8.attfmt;

		att_v8.attfmt = DB_CHA_TYPE;
		if (old_type < 0) att_v8.attfmt = -DB_CHA_TYPE;
		if (abs(old_type) == 61) att_v8.attfml = 8;
	    }
	    /* No label attributes any more */
	    att_v8.attflag &= ~(0x4000);	/* clear SEC_LBL */
	}

	if (row_version == DSC_VCURRENT)
	    break;
	else if (row_version == DSC_V8)
	{
	    /* currently the only change in v9 is longer attname */
	    STRUCT_ASSIGN_MACRO(att_v8.attrelid, att_v9.attrelid);
	    att_v9.attid = att_v8.attid;
	    att_v9.attxtra = att_v8.attxtra;
	    MEmove(sizeof(att_v8.attname), (char *)&att_v8.attname, 
		' ', sizeof(att_v9.attname), (char *)&att_v9.attname);
	    att_v9.attoff = att_v8.attoff;
	    att_v9.attfml = att_v8.attfml;
	    att_v9.attkey = att_v8.attkey;
	    att_v9.attflag = att_v8.attflag;
	    STRUCT_ASSIGN_MACRO(att_v8.attDefaultID, att_v9.attDefaultID);
	    att_v9.attintl_id = att_v8.attintl_id;
	    att_v9.attver_added = att_v8.attver_added;
	    att_v9.attver_dropped = att_v8.attver_dropped;
	    att_v9.attval_from = att_v8.attval_from;
	    att_v9.attfmt = att_v8.attfmt;
	    att_v9.attfmp = att_v8.attfmp;
	    att_v9.attver_altcol = att_v8.attver_altcol;
	    att_v9.attcollID = att_v8.attcollID;
	    MEfill(sizeof(att_v9.attfree), 0, (PTR) att_v9.attfree);

	    /* initialise geospatial columns */
	    att_v9.attgeomtype = -1;
	    att_v9.attsrid = -1;

	    /* initialize encryption columns */
	    att_v9.attencflags = 0;
	    att_v9.attencwid = 0;

	    /* now the row is DSC_V9 */	
	    row_version = DSC_V9;
	    tmp_att = (char *)&att_v9;
	}
    
	/* Add new upgrades to iiattribute here !!! */

	break;  /* ALWAYS */
    }

    /* tmp_att should point to current version ! */
    MEcopy(tmp_att, sizeof(DMP_ATTRIBUTE), new_attp);

    TRdisplay("upgrade att %~t (%d,%d) (%d %d %d %d %d %x)\n",
	sizeof(new_attp->attname), new_attp->attname.db_att_name,
	new_attp->attrelid.db_tab_base, new_attp->attrelid.db_tab_index,
	new_attp->attid, new_attp->attfmt, new_attp->attfml,
	new_attp->attfmp, new_attp->attkey, new_attp->attflag);

    return;

}


/*{
** Name: dm2d_upgrade_rewrite_iirel - convert iirelation to CURRENT format
**
** Description:
** 
**      Used by upgradedb to convert iirelation to CURRENT format
**      whenever the size of iirelation has increased.
**      This rewrite occurs at a very low physical level,
**      avoiding all the usual DMF buffering and table handling.
**
**      - Renames iirelation to a temp file name
**      - Deletes iirel_idx and creates empty iirel_idx
**      - Reads iiirelation records from the temp file 
**      - Call upgrade_iirel_row() to convert the row to the CURRENT format
**      - Adds the CURRENT iirelation row to a new iirelation file
**      - Creates/inserts iirel_idx rows to a new iirel_idx
** 
**      The new iirelation,iirel_idx is created is a single bucket HASH.
** 
**      This routine needs to know which core catalogs are being re-built
**      as single bucket hash so that it sets relprim = 1 for that core catalog
**
**      This routine was created from dm2d_convert_iirel_from_64 and 
**      other code in this file that does the row conversion.
**      Future iirelation upgrades requiring iirelation re-write
**      should require changes to upgrade_iirel_row() only.
**      
**
** Inputs:
**      db_id                           database to upgrade
**
** Outputs:
**      err_code                        error return code
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-apr-2007
**          Created from dm2d_convert_iirel_from_64
[@history_template@]...
*/
static DB_STATUS
dm2d_upgrade_rewrite_iirel (
    DM2D_UPGRADE    *upgrade_cb,
    DMP_DCB	    *db_id,
    i4              flag,
    i4              lock_list,
    DB_ERROR	    *dberr )
{
    DMP_DCB	    *dcb = (DMP_DCB *)db_id;
    i4	    lk_id, status, i=0,j, local_error, local_status;
    CL_ERR_DESC	    sys_err;
    DM0C_CNF	    *config = 0;
    bool	    config_open = FALSE, tmp_open = FALSE;
    bool	    rel_open = FALSE, rix_open = FALSE;
    DB_TAB_ID	    rel_tab_id, rix_tab_id;
    DM_FILE	    tmp_file = 
			{{'i','i','r','e','l','c','v','t','.','t','m','p'}};
    DM_FILE	    rel_file, rix_file;
    DI_IO	    tmp_di_io, rel_di_io, rix_di_io;
    DM_OBJECT	    *mem = NULL;
    DMPP_PAGE	    *tmp_pg, *rel_pg, *rix_pg;
    i4	    rel_pageno=0, rix_pageno=0;
    i4	    read_count, write_count, line, offset;
    DMP_RELATION    new_rel;
    DMP_RELATION    tmp_rel;
    char	    *old_rel;
    DMP_RINDEX	    rix_rec;
    DM_TID	    rix_tid, rel_tid;
    DMPP_ACC_PLV    *loc_plv;
    DM_TID	    tid;
    i4	    record_size;
    i4		    *err_code = &dberr->err_code;
    i4		    pgtype;
    i4		    pgsize;

    CLRDBERR(dberr);

    for (;;)
    {
      /*  Create a temporary lock list, needed to open the config */

      if((flag & DM0C_CNF_LOCKED) == 0)
      { 
	status = LKcreate_list( LK_ASSIGN | LK_NONPROTECT | LK_NOINTERRUPT,
	    (i4)0, (LK_UNIQUE *)0, (LK_LLID *)&lk_id, 0, &sys_err);

	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	    uleFormat(NULL, E_DM901A_BAD_LOCK_CREATE, &sys_err, ULE_LOG, 
		(DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
		&local_error, 0);
	    break;
	}
      }
      else
      {
        lk_id = lock_list;    
      }
	/*  Open the config file. */

	status = dm0c_open(dcb, flag, lk_id, &config, dberr);

	if (status != E_DB_OK)
	    break;

	config_open = TRUE;

	/* Note: conversion from 6.4 must go thru a config conversion,
	** which will turn off journaling for us.
	*/

	/*
	** Check the status of the upgrade. Its possible that upgradedb failed
	** and we're trying again. In that case upgradedb might have
	** failed after completing the convert.
	*/
	if  (upgrade_cb->upgr_dbg_env == UPGRADE_REWRITE)
	{
	    /* for now, always do the rewrite ! */
	    TRdisplay("UPGRADE rewrite iirelation config status %x\n",
		config->cnf_dsc->dsc_status);
	}
	else if ((config->cnf_dsc->dsc_status & DSC_IIREL_CONV_DONE) != 0)
	{
	    /* We've already completed the conversion. */

	    /* If it still exists, delete the temporary file. */

	    status = DIdelete (&tmp_di_io, (char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		tmp_file.name, sizeof(tmp_file.name), &sys_err);

	    if ((status != OK) && (status != DI_FILENOTFOUND))
		break;

	    /* Close the configuration file. */

	    status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

	    if (status != E_DB_OK)
		break;

	    /*  Release the temporary lock list. */

          if((flag & DM0C_CNF_LOCKED) == 0)
          {
	    status = LKrelease( LK_ALL, lk_id, (LK_LKID *)NULL, 
		(LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);

	    if (status != OK)
	    {
		uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		    (char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
		uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG , 
		    (DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
		    &local_error, 1, sizeof(lk_id), lk_id);
		break;
	    }
          }
	  return (E_DB_OK);
	}

	if (upgrade_cb->upgr_create_version < DSC_V3)
	{
	    pgsize = DM_COMPAT_PGSIZE; 
	    pgtype = DM_COMPAT_PGTYPE;
	}
	else
	{
	    pgsize = config->cnf_dsc->dsc_iirel_relpgsize;
	    pgtype = config->cnf_dsc->dsc_iirel_relpgtype;
	}

	/*
	** NOTE allocate additional sizeof(DMP_RELATION) so we can
	** safely copy sizeof(DMP_RELATION) bytes instead of
	** worrying here about what is the correct size of the old row
	** Also note that we're NOT using direct IO here, so don't
	** worry about alignments.
	*/
	status = dm0m_allocate(3*pgsize + sizeof(DMP_RELATION) +
		+ sizeof(DMP_MISC),
	    (i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	    (char*)dcb, (DM_OBJECT**)&mem, dberr);
	if ( status )
	    break;
	rel_pg = (DMPP_PAGE *)((char *)mem + sizeof(DMP_MISC));
	tmp_pg = (DMPP_PAGE *)((char *)rel_pg + pgsize);
	rix_pg = (DMPP_PAGE *)((char *)tmp_pg + pgsize);
	((DMP_MISC*)mem)->misc_data = (char*)mem + sizeof(DMP_MISC);

	/*  Generate the filenames for iirelation and rel_idx tables. */

	rel_tab_id.db_tab_base = DM_B_RELATION_TAB_ID;
	rel_tab_id.db_tab_index = DM_I_RELATION_TAB_ID;

	rix_tab_id.db_tab_base = DM_B_RIDX_TAB_ID;
	rix_tab_id.db_tab_index = DM_I_RIDX_TAB_ID;

	dm2f_filename (DM2F_TAB, &rel_tab_id, 0, 0, 0, &rel_file);
	dm2f_filename (DM2F_TAB, &rix_tab_id, 0, 0, 0, &rix_file);

	/* 
	** Check if we've already renamed iirelation in a previous invocation
	** of upgradedb. If the file exists, we shouldn't do any renaming.
	*/ 

	/* Open the tmp iirelation for read. */

 	status = DIopen( &tmp_di_io, dcb->dcb_location.physical.name,
	    dcb->dcb_location.phys_length,
	    tmp_file.name, sizeof(tmp_file.name),
	    pgsize, DI_IO_READ, DI_SYNC_MASK, &sys_err);

	/* If the file doesn't exist, we need to rename iirelation. */

	if (status == DI_FILENOTFOUND)
	{
	    /*  Rename iirelation to the tmp filename. */

	    status = DIrename ( 0, (char *) &dcb->dcb_location.physical,
		dcb->dcb_location.phys_length,
		rel_file.name, sizeof(rel_file.name),	     
		tmp_file.name, sizeof(tmp_file.name), &sys_err);

	    if (status != OK)
		break;

	    /* Now open the tmp iirelation for read. */

	    status = DIopen( &tmp_di_io, dcb->dcb_location.physical.name,
		dcb->dcb_location.phys_length,
		tmp_file.name, sizeof(tmp_file.name),
		pgsize, DI_IO_READ, DI_SYNC_MASK, &sys_err);
	}

	if (status != OK)
	    break;

	tmp_open = TRUE;

	/* 
	** Delete iirelation if it exists. We might have created this file
	** in a previous invocation of upgradedb.
	*/

	status = DIdelete ( &rel_di_io, (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    rel_file.name, sizeof(rel_file.name), &sys_err);

	/* DI_FILENOTFOUND is an ok bad status. The file might not exist. */

	if  ((status != OK) && (status != DI_FILENOTFOUND))
	    break;
 
	/* Create the new iirelation file. */

	status = DIcreate( &rel_di_io, (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    rel_file.name, sizeof(rel_file.name), 
	    pgsize, &sys_err);

	if (status != OK)
	    break;

	/* Open the new iirelation. */

	status = DIopen( &rel_di_io, dcb->dcb_location.physical.name,
	    dcb->dcb_location.phys_length,
	    rel_file.name, sizeof(rel_file.name),
	    pgsize, DI_IO_WRITE, DI_SYNC_MASK, &sys_err);

	if (status != OK)
	    break;

	rel_open = TRUE;

	/* 
	** Delete rel_idx if it exists. We might have created this file
	** in a previous invocation of upgradedb.
	*/

	status = DIdelete ( &rix_di_io, (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    rix_file.name, sizeof(rix_file.name), &sys_err);

	/* DI_FILENOTFOUND is an ok bad status. The file might not exist. */

	if  ((status != OK) && (status != DI_FILENOTFOUND))
	    break;
 
	/* Create the new rel_idx file. */

	status = DIcreate( &rix_di_io, (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length,
	    rix_file.name, sizeof(rix_file.name), 
	    pgsize, &sys_err);

	if (status != OK)
	    break;

	/* Open the new rel_idx. */

	status = DIopen( &rix_di_io, 
	    dcb->dcb_location.physical.name, dcb->dcb_location.phys_length,
	    rix_file.name, sizeof(rix_file.name),
	    pgsize, DI_IO_WRITE, DI_SYNC_MASK, &sys_err);

	if (status != OK)
	    break;

	rix_open = TRUE;

	/* Before we go any further, need to determine the page format of 
	** the tables. 
	*/
	dm1c_get_plv(pgtype, &loc_plv);
	
	/* Format the first pages of the new iirelation and rel_idx. */

	(*loc_plv->dmpp_format)(pgtype, pgsize, 
			rel_pg, 0, DMPP_DATA, DM1C_ZERO_FILL);
	(*loc_plv->dmpp_format)(pgtype, pgsize, 
			rix_pg, 0, DMPP_DATA, DM1C_ZERO_FILL);

	/* copy the records from the tmp iirelation to the new iirelation */

	for (i=0;;)
	{
	    /* Read a page from the tmp iirelation */

	    read_count = 1;

	    status = DIread( &tmp_di_io, &read_count, i, (char *)tmp_pg, 
		&sys_err);

	    if (status != OK)
		break;

	    tid.tid_tid.tid_page = i;
	    record_size = sizeof(DMP_OLDRELATION);
	    line = -1;

	    /* Copy each line to the new iirelation. */

	    while (++line < (i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(pgtype, 
		tmp_pg))
	    {
		tid.tid_tid.tid_line = line;
		old_rel = (char *) &tmp_rel;
		local_status = (*loc_plv->dmpp_get)(pgtype, pgsize,
		    tmp_pg, &tid, &record_size, &old_rel,
		    (i4*)NULL, (u_i4*)NULL, (u_i2*)NULL, (DMPP_SEG_HDR *)NULL);

		/* If the line is empty, go to the next line. */

		if  (local_status == E_DB_WARN)
		    continue;

		if (old_rel != (char *) &tmp_rel)
		{
		    MEcopy(old_rel, sizeof(DMP_RELATION), (char *)&tmp_rel);
		    old_rel = (char *)&tmp_rel;	/* Aligned copy */
		}

		/*
		** UPGRADE the row to the CURRENT format 
		** if this row is for a core catalog it will also sync 
		** new_rel with DM_core_relations (upgrade_core_reltup)
		*/
		upgrade_iirel_row(upgrade_cb, old_rel,
			&new_rel, config->cnf_dsc->dsc_status);

		/* 
		** Store the record on the page. If it doesn't fit
		** allocate an overflow page.
		*/

		status = add_record(pgtype, pgsize, rel_pg,
		    (char *)&new_rel, sizeof(new_rel),
		    &rel_tid, &rel_di_io, loc_plv, dberr);
		    
		if (status != OK)
		    break; 

		/* Store the rel_idx tuple corresponding to this tuple. */

		MEfill (sizeof(rix_rec), (u_char)0, (PTR)&rix_rec);

		STRUCT_ASSIGN_MACRO(new_rel.relid, rix_rec.relname);
		STRUCT_ASSIGN_MACRO(new_rel.relowner, rix_rec.relowner);
		rix_rec.tidp = rel_tid.tid_i4;

		status = add_record(pgtype, pgsize, rix_pg,
		    (char *)&rix_rec, sizeof(rix_rec),
		    &rix_tid, &rix_di_io, loc_plv, dberr);

		if (status != OK)
		    break; 

	    } /* end while */

	    if (status != OK) 
		break;

	    /* Follow any overflow chains. */

	    if (DMPP_VPT_GET_PAGE_OVFL_MACRO(pgtype, tmp_pg) != 0)
		i = DMPP_VPT_GET_PAGE_OVFL_MACRO(pgtype, tmp_pg);

	    /* 
	    ** No more overflow buckets. We're done with this hash bucket.
	    ** Go to the next hash bucket.
	    */

	    else if (DMPP_VPT_GET_PAGE_MAIN_MACRO(pgtype, tmp_pg) != 0) 
		i = DMPP_VPT_GET_PAGE_MAIN_MACRO(pgtype, tmp_pg);

	    /* 
	    ** No more main and overflow buckets. We're done. Allocate and
	    ** write out the last iirel and rel_idx pages and then exit
	    ** the loop.
	    */

	    else
	    {
		/* Allocate and flush the last iirelation page. */

		status = DIalloc( &rel_di_io, 1, &rel_pageno, &sys_err);

		if (status != OK)
		    break;

		status = DIflush(&rel_di_io, &sys_err);

		if (status != OK)
		    break;

		write_count = 1;

		TRdisplay("Write iirelation page %d nxtlino %d \n", 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, rel_pg), 
		(i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(pgtype, rel_pg));

		status = DIwrite( &rel_di_io, &write_count,
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, rel_pg), 
		    (char *)rel_pg, &sys_err);

		if (status != OK)
		    break;

		/* Allocate and flush the last rel_idx page. */

		status = DIalloc( &rix_di_io, 1, &rix_pageno, &sys_err);

		if (status != OK)
		    break;

		status = DIflush(&rix_di_io, &sys_err);

		if (status != OK)
		    break;

		write_count = 1;

		TRdisplay("Write iirelidx page %d nxtlino %d \n", 
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, rix_pg), 
		(i4)DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(pgtype, rix_pg));

		status = DIwrite( &rix_di_io, &write_count,
		    DMPP_VPT_GET_PAGE_PAGE_MACRO(pgtype, rix_pg), 
		    (char *)rix_pg, &sys_err);

		break;
	    }
	} /* end for */

	if (status != OK) 
	    break;


	/* Close the new iirelation file */

	status = DIclose (&rel_di_io, &sys_err);

	if  (status != OK)
	    break;
	    
	rel_open = FALSE;

	/* Close the rel_idx file */

	status = DIclose (&rix_di_io, &sys_err);

	if  (status != OK)
	    break;
	    
	rix_open = FALSE;

	/* 
	** Mark the flag in the config file to say we've completed the 
	** conversion. We must do this before we delete the temporary table.
	** Also make the database consistent.	
	*/

	config->cnf_dsc->dsc_status |= DSC_IIREL_CONV_DONE;

	/*
	** Because we hosed the space management setup for iirelation
	** leave the core catalog version alone.  We'll do more "stuff"
	** at database open time.  (This is database add time.)
	*/
	config->cnf_dsc->dsc_status &= ~DSC_65_SMS_DBMS_CATS_CONV_DONE;

	/* 
	** We've converted iirelation and rel_idx to hash structures with
	** exactly 1 hash bucket each. Set relprim to 1 for iirelation
	** in the config file.
	*/

 	/* Set relprim in config to note 1 bucket hash we just created */
	config->cnf_dsc->dsc_iirel_relprim = 1;

	if (upgrade_cb->upgr_create_version < DSC_V3)
	{
	    config->cnf_dsc->dsc_iirel_relpgsize = DM_PG_SIZE;
	    config->cnf_dsc->dsc_iiatt_relpgsize = DM_PG_SIZE;
	    config->cnf_dsc->dsc_iiind_relpgsize = DM_PG_SIZE;
	    config->cnf_dsc->dsc_iirel_relpgtype = TCB_PG_V1;
	    config->cnf_dsc->dsc_iiatt_relpgtype = TCB_PG_V1;
	    config->cnf_dsc->dsc_iiind_relpgtype = TCB_PG_V1;
	}

	/* Close the configuration file. */

	status = dm0c_close(config, (flag & DM0C_CNF_LOCKED) | DM0C_UPDATE, dberr);

	if (status != E_DB_OK)
	    break;

	config_open = FALSE;

	/*  Release the temporary lock list. */

      if((flag & DM0C_CNF_LOCKED) == 0)
      { 
	status = LKrelease( LK_ALL, lk_id, (LK_LKID *)NULL, 
	    (LK_LOCK_KEY *)NULL, (LK_VALUE *)NULL, &sys_err);

	if (status != OK)
	{
	    uleFormat(NULL, status, &sys_err, ULE_LOG, (DB_SQLSTATE *)NULL,
		(char *)NULL, 0L, (i4 *)NULL, &local_error, 0);
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG , 
		(DB_SQLSTATE *)NULL, (char *)NULL, 0L, (i4 *)NULL, 
		&local_error, 1, sizeof(lk_id), lk_id);
	    break;
	}
      }
	/* Close and delete the tmp iirelation file. */

	status = DIclose (&tmp_di_io, &sys_err);

	if (status != OK)
	    break;

	tmp_open = FALSE;

	status = DIdelete ( &tmp_di_io, (char *) &dcb->dcb_location.physical,
	    dcb->dcb_location.phys_length, tmp_file.name, 
	    sizeof(tmp_file.name), &sys_err);

	if (status != OK)
	    break;

	dm0m_deallocate(&mem);

	/* Return success. */
	return (E_DB_OK);

    } /* end for */

    if (mem != NULL)
	dm0m_deallocate(&mem);

    /* Deallocate the configuration record. */

    if (config_open == TRUE)
	local_status = dm0c_close(config, (flag & DM0C_CNF_LOCKED), dberr);

    /* Close the iirelation files if they are open. */

    if (tmp_open == TRUE)
	local_status = DIclose (&tmp_di_io, &sys_err);

    if (rel_open == TRUE)
	local_status = DIclose (&rel_di_io, &sys_err);

    if (rix_open == TRUE)
	local_status = DIclose (&rix_di_io, &sys_err);

    SETDBERR(dberr, 0, E_DM93AC_CONVERT_IIREL);
    return (E_DB_ERROR);
}


/*{
** Name: upgrade_iirel_row - convert iirelation to CURRENT format
**
** Description:
** 
**      Used by upgradedb to convert iirelation to CURRENT format
**      NOTE previously for each upgrade an iiattribute row was upgraded
**      like this:
**      64iirel -> current iirel
**      26iirel -> current iirel
**
** A simpler way to upgrade would be this:
**      64iirel -> 26iirel
**      26iirel -> nextversion
**
** That is, convert the row from version to next version
** New conversions should convert from previous version only
** (which should be simpler and cause fewer upgrade bugs)
**
** NOTE this routine was created using existing coversion code which converts
**      64iirel/26iirel -> v8 iirel
** Going forward new upgrades should not change the upgrade to v8. 
** New upgrades should allow the iirelation row go through multiple upgrades:
**      e.g.
**      64iirel  -> v8 iirel
**      v8rel -> next iirel
**
** Inputs:
**      db_id                           database to upgrade
**
** Outputs:
**      err_code                        error return code
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-apr-2007
**          Created.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Forcibly turn off TCB_BINARY, never used anywhere.  We may
**	    want to steal that bit for something (e.g. clustered) later.
*/
static VOID
upgrade_iirel_row(
DM2D_UPGRADE    *upgrade_cb,
char		*old_rel,
DMP_RELATION	*new_relp,
i4		dsc_status)
{
    DMP_OLDRELATION	*old_rel_64 = NULL;
    DMP_RELATION_26     *old_rel_26 = NULL;
    DMP_RELATION_V8	*old_rel_v8 = NULL;
    DMP_RELATION_V8	rel_v8;
    DMP_RELATION	rel_v9;
    i4			row_version = upgrade_cb->upgr_create_version;
    char		*tmp_rel = old_rel;
    i4			j;

    for ( ; row_version != DSC_VCURRENT ; )
    {
	if (row_version < DSC_V3)
	{
	    /* convert 6.4 iirelation row to DSC_V8 */
	    old_rel_64 = (DMP_OLDRELATION *)tmp_rel;

	    MEfill (sizeof(rel_v8), (u_char)0, (PTR)&rel_v8);

	    /* copy old info */

	    rel_v8.reltid = old_rel_64->reltid;
	    dm2d_cname((char *)&rel_v8.relid, old_rel_64->relid,
			sizeof(rel_v8.relid));
	    dm2d_cname((char *)&rel_v8.relowner, old_rel_64->relowner,
			sizeof(rel_v8.relowner));
	    rel_v8.relatts = old_rel_64->relatts;               
	    rel_v8.relwid = old_rel_64->relwid;                
	    rel_v8.relkeys = old_rel_64->relkeys;		    
	    rel_v8.relspec = old_rel_64->relspec;               
	    rel_v8.relstat = old_rel_64->relstat;               
	    rel_v8.reltups = old_rel_64->reltups;               
	    rel_v8.relpages = old_rel_64->relpages;              
	    rel_v8.relprim = old_rel_64->relprim;               
	    rel_v8.relmain = old_rel_64->relmain;               
	    rel_v8.relsave = old_rel_64->relsave;               
	    STRUCT_ASSIGN_MACRO(old_rel_64->relstamp12, rel_v8.relstamp12);
	    dm2d_cname((char *)&rel_v8.relloc, old_rel_64->relloc,
			sizeof(rel_v8.relloc));
	    rel_v8.relcmptlvl = old_rel_64->relcmptlvl;
	    rel_v8.relcreate = old_rel_64->relcreate;             
	    rel_v8.relqid1 = old_rel_64->relqid1;               
	    rel_v8.relqid2 = old_rel_64->relqid2;               
	    rel_v8.relmoddate = old_rel_64->relmoddate;            
	    rel_v8.relidxcount = old_rel_64->relidxcount;           
	    rel_v8.relifill = old_rel_64->relifill;              
	    rel_v8.reldfill = old_rel_64->reldfill;              
	    rel_v8.rellfill = old_rel_64->rellfill;              
	    rel_v8.relmin = old_rel_64->relmin;                
	    rel_v8.relmax = old_rel_64->relmax;                
	    rel_v8.relloccount = old_rel_64->relloccount;

	    /*
	    ** Note: since initial Jupiter databases, configuration files
	    ** etc, did not support multiple locations, they to not
	    ** have their relloccount value initialized.  So do here
	    ** just in case.
	    */
	    if (((old_rel_64->relstat & TCB_MULTIPLE_LOC) == 0) &&
		 (old_rel_64->relloccount != 1))
	    {
		rel_v8.relloccount = 1;
	    }

	    rel_v8.relgwid = old_rel_64->relgwid;	            
	    rel_v8.relgwother = old_rel_64->relgwother;            
	    rel_v8.relhigh_logkey = old_rel_64->relhigh_logkey;        
	    rel_v8.rellow_logkey = old_rel_64->rellow_logkey;         

	    /* 
	    ** Initialize relfhdr, relextend and relalloction to default
	    ** values. This is required by the file extend project.
	    */
	    rel_v8.relfhdr       = DM_TBL_INVALID_FHDR_PAGENO;
	    rel_v8.relallocation = DM_TBL_DEFAULT_ALLOCATION;
	    rel_v8.relextend     = DM_TBL_DEFAULT_EXTEND;

	    /* 
	    ** Initialise other new 6.5 fields
	    */
	    rel_v8.relstat2	   = 0;

	    rel_v8.relpgtype = TCB_PG_V1;

	    if ( rel_v8.relstat & TCB_COMPRESSED )
		rel_v8.relcomptype = TCB_C_STANDARD;
	    else
		rel_v8.relcomptype = TCB_C_NONE;

	    /* 
	    ** Initialise other new OI2.0 fields
	    */
	    rel_v8.relpgsize     = DM_PG_SIZE;
	    rel_v8.relversion    = 0;

	    /* Clear has-stats, upgradedb will kill iistats */
	    rel_v8.relstat &= ~TCB_ZOPTSTAT;

	    rel_v8.reltotwid = rel_v8.relwid;
	    
	    /* now the row is DSC_V8 */	
	    row_version = DSC_V8;
	    tmp_rel = (char *)&rel_v8;
	}

	if (row_version == DSC_VCURRENT)
	    break;
	else if (row_version < DSC_V8)
	{
	    /*
	    **
	    ** Convert to DSC_V8
	    **
	    ** Don't convert if this already looks like v8 format
	    ** new relpgsize is where old relloccount was so there's
	    ** little likelihood of confusion!
	    ** 
	    ** This is only possible for in-place upgrade of iirelation rows
	    ** where we are re-starting a previously incomplete upgrade
	    */
	    old_rel_v8 = (DMP_RELATION_V8 *)tmp_rel;

	    if (old_rel_v8->relpgsize == 2048 || 
		    old_rel_v8->relpgsize == 4096 ||
		    old_rel_v8->relpgsize == 8192 ||
		    old_rel_v8->relpgsize == 16384 ||
		    old_rel_v8->relpgsize == 65536)
	    {
		/* the row is already DSC_V8 */	
		row_version = DSC_V8;
	    }
	}

	if (row_version == DSC_VCURRENT)
	    break;
	else if (row_version < DSC_V8)
	{
	    old_rel_26 = (DMP_RELATION_26 *)tmp_rel;

	    /* Set page size, type if converting from 1.X, 2.0 */
	    if (row_version < DSC_V6 &&
		(dsc_status & DSC_IIREL_V6_CONV_DONE) == 0)
	    {
		if (old_rel_26->relpgsize == 0)
		{
		    old_rel_26->relpgsize = DM_COMPAT_PGSIZE;
		    old_rel_26->relpgtype = TCB_PG_V1;
		} else if (old_rel_26->relpgsize == DM_COMPAT_PGSIZE)
		    old_rel_26->relpgtype = TCB_PG_V1;
		else
		    old_rel_26->relpgtype = TCB_PG_V2;
		if (row_version < DSC_V4)
		{
		    old_rel_26->reltotwid = old_rel_26->relwid;
		    old_rel_26->relversion = 0;
		    old_rel_26->reltcpri = 0;
		}
	    }
		
	    /* Copy all fields in DMP_RELATION26 to DMP_RELATION */
	    MEfill(sizeof(rel_v8), '\0', &rel_v8);
	    STRUCT_ASSIGN_MACRO(old_rel_26->reltid, rel_v8.reltid);
	    MEmove(sizeof(old_rel_26->relid), (char *)&old_rel_26->relid, 
		' ', sizeof(rel_v8.relid), (char *)&rel_v8.relid);
	    MEmove(sizeof(old_rel_26->relowner), (char *)&old_rel_26->relowner, 
		' ', sizeof(rel_v8.relowner), (char *)&rel_v8.relowner);
	    rel_v8.relatts = old_rel_26->relatts;
	    rel_v8.relwid = old_rel_26->relwid;
	    rel_v8.relkeys = old_rel_26->relkeys;
	    rel_v8.relspec = old_rel_26->relspec;
	    rel_v8.relstat = old_rel_26->relstat;
	    rel_v8.reltups = old_rel_26->reltups;
	    rel_v8.relpages = old_rel_26->relpages;
	    rel_v8.relprim = old_rel_26->relprim;
	    rel_v8.relmain = old_rel_26->relmain;
	    rel_v8.relsave = old_rel_26->relsave;
	    STRUCT_ASSIGN_MACRO(old_rel_26->relstamp12, rel_v8.relstamp12);
	    MEmove(sizeof(old_rel_26->relloc), (char *)&old_rel_26->relloc, 
		' ', sizeof(rel_v8.relloc), (char *)&rel_v8.relloc);
	    rel_v8.relcmptlvl = old_rel_26->relcmptlvl;
	    rel_v8.relcreate = old_rel_26->relcreate;
	    rel_v8.relqid1 = old_rel_26->relqid1;
	    rel_v8.relqid2 = old_rel_26->relqid2;
	    rel_v8.relmoddate = old_rel_26->relmoddate;
	    rel_v8.relidxcount = old_rel_26->relidxcount;
	    rel_v8.relifill = old_rel_26->relifill;
	    rel_v8.reldfill = old_rel_26->reldfill;
	    rel_v8.rellfill = old_rel_26->rellfill;
	    rel_v8.relmin = old_rel_26->relmin;
	    rel_v8.relmax = old_rel_26->relmax;
	    rel_v8.relloccount = old_rel_26->relloccount;
	    rel_v8.relgwid = old_rel_26->relgwid;
	    rel_v8.relgwother = old_rel_26->relgwother;
	    rel_v8.relhigh_logkey = old_rel_26->relhigh_logkey;
	    rel_v8.rellow_logkey = old_rel_26->rellow_logkey;
	    rel_v8.relfhdr = old_rel_26->relfhdr;
	    rel_v8.relallocation = old_rel_26->relallocation;
	    rel_v8.relextend = old_rel_26->relextend;
	    rel_v8.relcomptype = old_rel_26->relcomptype;
	    rel_v8.relpgtype = old_rel_26->relpgtype;
	    rel_v8.relstat2 = old_rel_26->relstat2;
	    rel_v8.relpgsize = old_rel_26->relpgsize;
	    rel_v8.relversion = old_rel_26->relversion;
	    rel_v8.reltotwid = old_rel_26->reltotwid;
	    rel_v8.reltcpri = old_rel_26->reltcpri;

	    /*
	    ** Now initialize rel_v8 fields added for table partitioning
	    ** These were taken out of relfree so there's no need to
	    ** change iirelation's relwid.  iiattribute conversion will
	    ** adjust core catalog att entries.
	    */
	    rel_v8.relstat &= ~TCB_IS_PARTITIONED;
	    rel_v8.relstat2 &= ~(TCB2_PARTITION | TCB2_GLOBAL_INDEX);
	    rel_v8.relnparts = 0;
	    rel_v8.relnpartlevels = 0;

	    /* Adjust relatts if this is an iirelation or iiattribute
	    ** relation row.
	    */
	    if (rel_v8.reltid.db_tab_base == DM_B_RELATION_TAB_ID
	      && rel_v8.reltid.db_tab_index == DM_I_RELATION_TAB_ID)
	    {
		/* iirelation doesn't change width but does change number
		** attributes.
		*/
		rel_v8.relatts = DM_core_relations[0].relatts;
	    }
	    else if (rel_v8.reltid.db_tab_base == DM_B_ATTRIBUTE_TAB_ID)
	    {
		i4 n;

		/* We'll be changing iiattribute to a 1-bucket hash,
		** soon enough.  We'll also be hosing down its
		** FHDR.
		*/
		rel_v8.relprim = 1;
		rel_v8.relfhdr       = DM_TBL_INVALID_FHDR_PAGENO;
		rel_v8.relallocation = DM_TBL_DEFAULT_ALLOCATION;
		rel_v8.relextend     = DM_TBL_DEFAULT_EXTEND;

		/* Rather than hard code the number of columns in
		** iiattribute, find it in the core-relations array.
		*/
		for (n = 0; n < DM_sizeof_core_rel; ++n)
		{
		    if ((DM_core_relations[n].reltid.db_tab_base ==
					    DM_B_ATTRIBUTE_TAB_ID) &&
		      (DM_core_relations[n].reltid.db_tab_index ==
					    DM_I_ATTRIBUTE_TAB_ID) )
		    {
			rel_v8.relatts = DM_core_relations[n].relatts;
			rel_v8.relwid = DM_core_relations[n].relwid;
			rel_v8.reltotwid = rel_v8.relwid;
			break;
		    }
		}
	    }

	    /* now the row is DSC_V8 */	
	    row_version = DSC_V8;
	    tmp_rel = (char *)&rel_v8;
	}

	/* This conversion will be necessary when DSC_VCURRENT > DSC_V8 */
	/* right now the only catalog change is longer relid,relowner,relloc */
	if (row_version == DSC_VCURRENT)
	    break;
	else if (row_version < DSC_V9)
	{
	    old_rel_v8 = (DMP_RELATION_V8 *)tmp_rel;

	    /* Convert V8 to V9 here */
	    /* Longer relid;  turn off old TCB_BINARY (we can steal that
	    ** one for TCB_CLUSTER eventually).
	    */
	    STRUCT_ASSIGN_MACRO(old_rel_v8->reltid, rel_v9.reltid);
	    MEmove(sizeof(old_rel_v8->relid), (char *)&old_rel_v8->relid, 
		' ', sizeof(rel_v9.relid), (char *)&rel_v9.relid);
	    MEmove(sizeof(old_rel_v8->relowner), (char *)&old_rel_v8->relowner, 
		' ', sizeof(rel_v9.relowner), (char *)&rel_v9.relowner);
	    rel_v9.relatts = old_rel_v8->relatts;
	    rel_v9.reltcpri = old_rel_v8->reltcpri;
	    rel_v9.relkeys = old_rel_v8->relkeys;
	    rel_v9.relspec = old_rel_v8->relspec;
	    rel_v9.relstat = old_rel_v8->relstat & (~TCB_BINARY);
	    rel_v9.reltups = old_rel_v8->reltups;
	    rel_v9.relpages = old_rel_v8->relpages;
	    rel_v9.relprim = old_rel_v8->relprim;
	    rel_v9.relmain = old_rel_v8->relmain;
	    rel_v9.relsave = old_rel_v8->relsave;
	    rel_v9.relstamp12 = old_rel_v8->relstamp12;
	    MEmove(sizeof(old_rel_v8->relloc), (char *)&old_rel_v8->relloc, 
		' ', sizeof(rel_v9.relloc), (char *)&rel_v9.relloc);
	    rel_v9.relcmptlvl = old_rel_v8->relcmptlvl;
	    rel_v9.relcreate = old_rel_v8->relcreate;
	    rel_v9.relqid1 = old_rel_v8->relqid1;
	    rel_v9.relqid2 = old_rel_v8->relqid2;
	    rel_v9.relmoddate = old_rel_v8->relmoddate;
	    rel_v9.relidxcount = old_rel_v8->relidxcount;
	    rel_v9.relifill = old_rel_v8->relifill;
	    rel_v9.reldfill = old_rel_v8->reldfill;
	    rel_v9.rellfill = old_rel_v8->rellfill;
	    rel_v9.relmin = old_rel_v8->relmin;
	    rel_v9.relmax = old_rel_v8->relmax;
	    rel_v9.relpgsize = old_rel_v8->relpgsize;
	    rel_v9.relgwid = old_rel_v8->relgwid;
	    rel_v9.relgwother = old_rel_v8->relgwother;
	    rel_v9.relhigh_logkey = old_rel_v8->relhigh_logkey;
	    rel_v9.rellow_logkey = old_rel_v8->rellow_logkey;
	    rel_v9.relfhdr = old_rel_v8->relfhdr;
	    rel_v9.relallocation = old_rel_v8->relallocation;
	    rel_v9.relextend = old_rel_v8->relextend;
	    rel_v9.relcomptype = old_rel_v8->relcomptype;
	    rel_v9.relpgtype = old_rel_v8->relpgtype;
	    rel_v9.relstat2 = old_rel_v8->relstat2;
	    rel_v9.relloccount = old_rel_v8->relloccount;
	    rel_v9.relversion = old_rel_v8->relversion;
	    rel_v9.relwid = old_rel_v8->relwid;
	    rel_v9.reltotwid = old_rel_v8->reltotwid;
	    rel_v9.relnparts = old_rel_v8->relnparts;
	    rel_v9.relnpartlevels = old_rel_v8->relnpartlevels;
	    MEfill (sizeof(rel_v9.relfree), (u_char)0, rel_v9.relfree);

	    /* now the row is DSC_V9 */	
	    row_version = DSC_V9;
	    tmp_rel = (char *)&rel_v9;
	    if (row_version == DSC_VCURRENT)
		break;
	}

	/* Add new upgrades to iirelation here !!! */

	break;  /* ALWAYS */
    }

    if (row_version != DSC_VCURRENT)
    {
	/* FIX ME error */
	TRdisplay("upgrade_iirel_row %d %d\n", row_version, DSC_VCURRENT);
    }

    /* tmp_rel should point to current version ! */
    MEcopy(tmp_rel, sizeof(DMP_RELATION), new_relp);

    /*
    ** Special upgrade for core catalogs
    ** MUST be done when row is in current version
    */
    if (new_relp->reltid.db_tab_base <= DM_B_INDEX_TAB_ID)
	upgrade_core_reltup(upgrade_cb, (DMP_RELATION *)new_relp);

    TRdisplay("upgrade rel %~t (%d,%d) (%d %d %d %d %d %x %x)\n",
	sizeof(new_relp->relid), new_relp->relid.db_tab_name,
	new_relp->reltid.db_tab_base, new_relp->reltid.db_tab_index,
	new_relp->relpgtype, new_relp->relpgsize,
	new_relp->relatts, new_relp->relkeys, new_relp->relwid,
	new_relp->relstat, new_relp->relstat2);

    return;
    
}


/*{
** Name: upgrade_core_reltup - special upgrade for core iirelation tuple
**
** Description: Special upgrade for core iirelation tuples
**
** Inputs:
**      db_id                           database to upgrade
**
** Outputs:
**      err_code                        error return code
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-apr-2009
**          Created.
[@history_template@]...
*/
static VOID
upgrade_core_reltup(
DM2D_UPGRADE *upgrade_cb,
DMP_RELATION *core)
{
    i4	j;
    DMP_RELATION *dm_core;

    for (j = 0, dm_core = &DM_core_relations[0]; 
			j < DM_sizeof_core_rel; j++, dm_core++)
        if (dm_core->reltid.db_tab_base == core->reltid.db_tab_base
        && dm_core->reltid.db_tab_index == core->reltid.db_tab_index)
	break;

    if (j > DM_sizeof_core_rel)
	TRdisplay("Upgrade CORE Can't find %~t in DM_core\n",
		sizeof(core->relid), core->relid.db_tab_name);
    else
	TRdisplay("Upgrade CORE from %~t\n", 
		sizeof(DB_TAB_NAME), dm_core->relid.db_tab_name);

    /* Update the relwid, reltotwid, relatt from DM_core_relations */
    core->relwid = dm_core->relwid;
    core->reltotwid = dm_core->reltotwid;
    core->relatts = dm_core->relatts;

    /* Will upgrade make iirelation/iirelidx 1-bucket hash ? */
    if (core->reltid.db_tab_base == DM_B_RELATION_TAB_ID)
    {
	/* create version  tells us if this core will soon be 1-bucket */
	if (upgrade_cb->upgr_iirel == UPGRADE_REWRITE)
	{
	    core->relprim = 1;
	    core->relfhdr       = DM_TBL_INVALID_FHDR_PAGENO;
	    core->relallocation = DM_TBL_DEFAULT_ALLOCATION;
	    core->relextend     = DM_TBL_DEFAULT_EXTEND;
	}
    }

    /* Will upgrade make iiattribute 1-bucket hash ? */
    if (core->reltid.db_tab_base == DM_B_ATTRIBUTE_TAB_ID)
    {
	/* create version  tells us if this core will soon be 1-bucket */
	if (upgrade_cb->upgr_iiatt == UPGRADE_REWRITE)
	{
	    core->relprim = 1;
	    core->relfhdr       = DM_TBL_INVALID_FHDR_PAGENO;
	    core->relallocation = DM_TBL_DEFAULT_ALLOCATION;
	    core->relextend     = DM_TBL_DEFAULT_EXTEND;
	}
    }

    /* Will upgrade make iiindex 1-bucket hash ? */
    if (core->reltid.db_tab_base == DM_B_INDEX_TAB_ID)
    {
	/* create version  tells us if this core will soon be 1-bucket */
	if (upgrade_cb->upgr_iiindex == UPGRADE_REWRITE)
	{
	    core->relprim = 1;
	    core->relfhdr       = DM_TBL_INVALID_FHDR_PAGENO;
	    core->relallocation = DM_TBL_DEFAULT_ALLOCATION;
	    core->relextend     = DM_TBL_DEFAULT_EXTEND;
	}
    }

    /* Display the atts we changed */
    TRdisplay("UPDATED CORE %~t (%d %d %d %d %d %d)\n",
		sizeof(core->relid), core->relid.db_tab_name,
		core->relprim, core->relfhdr,
		core->relallocation, core->relextend,
		core->relwid, core->relatts);
}
