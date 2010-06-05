/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMFTRACE.H - DMF trace descriptions.
**
** Description:
**      The file describes the constants needed to set and test trace
**	flags.
**
**	These trace flags are set by calls to dmf_trace.  The trace point
**	number determines the type of trace operation, the trace class and
**	the trace point.  The following diagram depicts a tracepoint value
**	and it's interpretation:
**
**	  TRACE POINT ID           INTERPRETATION
**	  --------------    -----------------------------------------------
**	    DMxyyzz	    DM : is the prefix for DMF.
**				X  : is the operation.
**				    0   : trace point setting
**					YY  : is the class
**					    0   : session trace point
**					    1   : memory
**					    2   : file
**					    3   : lock
**					    4   : buffer manager
**					    5   : sort
**					    6   : access methods
**					    7   : table
**					    8   : record
**					    9   : transaction
**					    10  : utility
**					    11  : crash
**					    12  : call
**					    13  : async threads
**					    14  : test
**						ZZ  : is the specific trace
**						    point within the class.
**				       15-39: RESERVED Classes for 0.
**				    1-9: RESERVED operations
**
**	As an example:
**
**	    DM00001	is a session trace point for flag one.
**	    DM00302	is a lock trace point for flag two.
**	    DM03332	is a reserved class.
**	    DM10534	is a reserved operation.
**
** History:
**      16-jun-1986 (Derek)
**          Created for Jupiter.
**	02-sep-1988 (rogerk)
**	    Added DMZ_ASY_MACRO for trace on asyncronous threads.
**	31-oct-1988 (rogerk)
**	    Added DMZ_TST_MACRO.
**	10-apr-1989 (rogerk)
**	    Added DM1420 trace point for multi-server fast commit.
**	    Added DM421 trace point to flush buffer manager.
**	    Added class constants for session and test classes.
**	 7-jun-1989 (rogerk)
**	    Added DM801, DM802 trace points for skipping/patching bad
**	    tuples in the database.
**	29-aug-89   (teg)
**	    Added Patch Table traces (01 - 06).
**       4-sep-1989 (rogerk)
**          Added DMZ_UTL_MACRO(1) (1001) to bypass SBI logging in convert60.
**	 9-apr-1990 (sandyh)
**	    Added DMZ_SES_MACRO(11) to anable session log tracing.
**	19-apr-1990 (rogerk)
**	    Added DMZ_TST_MACRO(19) to crash logging system.
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	14-jun-1991 (Derek)
**	    New trace flags for performance profiling and allocation
**	    debugging.
**	11-sep-1991 (fred)
**	    Added DMPE (Peripheral Operations == large objects/blobs) trace
**	    points.
**	25-sep-1991 (mikem) integrated following change:  4-feb-1991 (rogerk)
**	    Added DM1305 to force consistency point.
**	    Added DM422 to toss unmodified pages from cache.
**	    Added DM425 to fix cache priority for a table.
**	25-sep-1991 (mikem) integrated following change: 25-feb-1991 (rogerk)
**	    Added archiver test trace points.
**	25-sep-1991 (mikem) integrated following change: 25-mar-1991 (bryanp)
**	    Added new DM6xx tracepoints for Btree index compression tracing
**      21-jan-92 (jnash)
**	    Add SES_30 tracepoint to generate pass abort during dmxe_abort.
**	05-mar-92 (mikem)
**	    Added DM901 to disable before image logging.  Should only be
**	    used for simulating compressed logging.  When compressed logging
**	    is implemented the trace point will not be needed anymore.
**	07-jul-1992 (ananth)
**	    Prototyping DMF.
**	29-August-1992 (rmuth)
**	    - Add DM651 to trace build routines
**	    - Add DM652 to trace 6.4->6.5 FHDR/FMAP conversion.
**	    - Add DM653 Overide the relfhdr!=invalid so do not convert check.
**	    - Add DM1430 fail after converting iirelation and iirel_idx.
**	    - Add DM1431 fail after converted core and one DBMS catalog
**	    - ADD DM1432 fail while dm2u_convert_table converting a table.
**	    - Add DM1433 fail after physically adding FHDR/FMAP to file.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Project:
**		Documented use of DM1306 and DM1307 to trigger recovery tracing.
**		Trace point DM12 triggers display of LSN values during
**		    log_trace. That is, if you just want ordinary log_trace,
**		    then set only DM11. If you want log_trace with LSN's
**		    displayed, set both DM11 AND DM12.
**		Add trace point DM1434 to force the logfile to disk. This is
**		    useful just before crashing the system, so that you know 
**		    what recovery you're going to do.
**	24-may-1993 (bryanp)
**	    Add trace point DM302. This trace point augments normal lock_trace
**		by adding the tracing of internal buffer manager cache locks.
**		If you just want ordinary lock_trace, then just set DM1 (or
**		use "set lock_trace". If you want lock_trace+buffer cache locks,
**		set both DM1 and DM302. If you want to see only buffer manager
**		cache locks traced (why would you want this?), you're out of
**		luck.
**	24-may-1993 (rogerk)
**	    - Add DM427 to toss page from cache.
**	    - Add DM428 to toss table's pages from cache.
**	    - Add DM713 to toss TCB from server.
**	    Also changed lists of current tracepoints to include the trace
**	    point class number to make it easier to search for trace points
**	    and to tell what trace point is being described (ie, in the
**	    DMZ_BUF_MACRO section, list the flush cache trace point as 421,
**	    instead of just 21).
**	21-jun-1993 (bryanp)
**	    Add trace point dm303, which is VMS only and sends a SS$_DEBUG
**		signal to the DMFCSP to ask it to enter the debugger (kind of
**		an SC905 for the CSP).
**	21-jun-1993 (rogerk)
**	    Added trace to bypass freeing of empty overflow pages.
**	21-may-1993 (jnash)
**	    Add DM1308 to extend CP duration an additional 30 seconds.
**	    Add DM1435 to mark database inconsistent (for recovery testing).
**	26-jul-1993 (rogerk)
**	    Add DM1309 - DM1313 for recovery tracing.
**	    Add DM1314 to signal archive cycle.
**      26-jul-1993 (mikem)
**          - Add DM1436 to test performance of LG calls.
**          - Add DM1437 to test performance of LGwrite() calls.
**          - Add DM1438 to test performance of ii_nap() calls (unix only).
**          - Add DM1439 to test performance of system calls from server.
**          - Add DM1441 to test performance of cross process sem calls.
**	23-aug-1993 (rogerk)
**	    Add DM1315 to enable page information tracing.
**      18-oct-93 (jnash)
**          Add DM902 to compress replace log "old" record images.
**	22-nov-1993 (jnash)
**	    Add trace point DM304, dump lockstat info to on
** 	    page or table escalation deadlock.
**	31-jan-1994 (mikem)
**	    Sir #58499
**	    Added dm1205.  After this change, when dm1205 is set then any or
**	    all of 120{1,2,3,4} and/or 1210 can be used in a non xDEBUG server.
**	    Rather than xDEBUG out the calls to dmd_call() in dmf_call(), we
**	    now test DMZ_CLL_MACRO(05) and call it if it is set.  You must still
**	    set the other trace points depending on what you want.
**	31-jan-1994 (jnash)
**	    Add DM1316 to bypass DMF memory dump during dmd_check().
**	22-feb-1994 (andys)
**	    Integrate ingres63p changes.
**          These are for dm429 and dm430.
**	21-feb-1994 (jnash)
**	    Add DM1317 to simulate CKrestore() error during rollforwarddb.
**	    Add DM1318 to simulate CKsave() error during ckpdb.
**	23-may-1994 (jnash)
**	  - Add DM431-432 documentation.  
**	  - Add DM433 to fix the cache priority of all tables (at 6).  This 
**	    takes effect on tcbs created after the tracepoint is set.  This
**	    is equivalent to "set trace point DM425" on all tables.
**	18-sep-1995 (nick)
**	    Add ingres63p tracepoint DM427 as DM434.
**	06-mar-1996 (stial01 for bryanp)
**	    Add DM714 and DM715 to control server's page size.
**	29-aug-96 (nick)
**	    Add DM305 to trace system catalog maxlock escalations.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add DM716 to force row-level locking.
**	27-jan-97 (stephenb)
**	    Add DM102 to disable replication in a database.
**	01-Apr-1997 (jenjo02)
**	    Added DM435 to disable fixed table cache priorities.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Added DM717 and DM718 to force CS and RR respectively.
**	29-aug-1997 (bobmart)
**	    Add DM306 thru DM308 to control the logging of lock escalation
**	    messages when exceeding max locks per transaction or resource
**	    for system catalogs and user tables; observed only when
**	    corresponding pm configurable is marked 'disable'.  Retained
**	    earlier meaning for DM305.
**      12-aug-1998 (stial01)
**          Add DM719 to force btree re-position on first get.
**      28-may-1999 (stial01)
**          Added DMZ_SES_MACRO(33) for row lock protocol checking
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-apr-2001 (stial01)
**          Updated for DM34, to display create table/index page type info
**	30-Apr-2001 (jenjo02)
**	    Added DM903 to control displaying of rollback trace info.
**	19-Aug-2002 (jenjo02)
**	    Added DM35 to enable production of
**	    E_DM9066_LOCK_BUSY messages and info for TIMEOUT=NOWAIT.
**	01-Apr-2004 (jenjo02)
**	    Added DM1320 to enable dmrAgents.
**	12-Apr-2004 (jenjo02)
**	    Added DM36 <n> to set session's degree of parallelism.
**	20-Apr-2004 (schka24)
**	    Added DM511 for modify statistics.  Eliminate compiler warning
**	    on macro expansion (use 1U instead of 1).
**	22-Jul-2004 (schka24)
**	    Note that only 96 of the 99 session tracepoints are allowed.
**      01-dec-2004 (stial01)
**         DM722 to validate blob coupons before insert/replace
**      30-mar-2005 (stial01)
**         Added dm103, dm104 tracepoints
**	11-Aug-2005 (jenjo02)
**	   Added DM105 tracepoint for dm0m.
**      29-sep-2005 (stial01)
**         DM723 to validate update lock protocols (B115299)
**      21-Jan-2009 (horda03) Bug 121519
**         DM1444 to set last TABLE_ID assigned in a DB.
**      04-dec-2009 (stial01)
**         DM618 to trace adt_key_incr, adt_key_decr of btree keys
**/

/*
**  Defines of other constants.
*/

/*
**  The following macros define the interface to testing a trace flag.
**  Each class of trace flags has it's own macro.  Each macro takes the
**  number of the trace flag within the class to test.  The typical
**  usage of one of these flags follows:
**
**	if (DMZ_LCK_MACRO(30))
**	    TRdisplay("Database lock request\n");
**
**  The following section is used to document the usage of each trace flag
**  by class.  When a new trace flag is added the corresponding section
**  should be updated.
*/

#define	DMZ_MACRO(v, b)		((v)[(b) >> 5] & (1U << ((b) & (BITS_IN(i4) - 1))))
#define	DMZ_SET_MACRO(v,b)	((v)[(b) >> 5] |= (1U << ((b) & (BITS_IN(i4)-1))))
#define	DMZ_CLR_MACRO(v,b)	((v)[(b) >> 5] &= ~(1U << ((b) & (BITS_IN(i4)-1))))
 
#define	DMZ_OPERATIONS		1
#define	DMZ_CLASSES		15
#define	DMZ_POINTS		100

#define	DMZ_SESCLASS		0
#define	DMZ_TSTCLASS		14

#define	DMZ_SES_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, t)
#define DMZ_MEM_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 100 + t)
#define	DMZ_FIL_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 200 + t)
#define	DMZ_LCK_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 300 + t)
#define	DMZ_BUF_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 400 + t)
#define	DMZ_SRT_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 500 + t)
#define	DMZ_AM_MACRO(t)		DMZ_MACRO(dmf_svcb->svcb_trace, 600 + t)
#define DMZ_TBL_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 700 + t)
#define	DMZ_REC_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 800 + t)
#define DMZ_XCT_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 900 + t)
#define	DMZ_UTL_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 1000 + t)
#define	DMZ_CRH_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 1100 + t)
#define	DMZ_CLL_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 1200 + t)
#define	DMZ_ASY_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 1300 + t)
#define	DMZ_TST_MACRO(t)	DMZ_MACRO(dmf_svcb->svcb_trace, 1400 + t)

/*
**  DMZ_SES_MACRO(s,t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter.
**	26-apr-1993 (bryanp)
**	    Trace point DM12 triggers display of LSN values during log_trace.
**	    That is, if you just want ordinary log_trace, then set only DM11.
**	    If you want log_trace with LSN's displayed, set both DM11 AND DM12.
**	19-Aug-2002 (jenjo02)
**	    Added DM35 to enable production of
**	    E_DM9066_LOCK_BUSY messages and info for TIMEOUT=NOWAIT.
**	12-Apr-2004 (jenjo02)
**	    Added DM36 <n> to set session's degree of parallelism.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  00-01	    USER LOCK TRACING. TRACES TABLE AND PAGE LOCKS.
**  02-09           Reserved for other lock tracing.
**  10              DI I/O TRACE 
**  11		    USER LOG TRACING. TRACES LOG WRITES (TYPE & SIZE).
**  12		    Used in addition to DM11, gives LSN values in log_trace.
**  13-19           Reserved for other I/O tracing.
**  20		    Transaction end performance numbers.
**  21		    Record close performance numbers.
**  22-29	    Reserved for other performance tracing.
**  30		    Force Pass abort during dmxe_abort.
**  31		    DMPE Session level events (Peripheral datatype support)
**  32		    Indicates that this is a replication server connection
**		    (do not maintain shadow, archive and input queue)
**  33              Protocol checking for row locking, checks that row/value
**                  lock was requested
**  34		    Display create table/index page type information
**  35		    Produce E_DM9066_LOCK_BUSY messages and information
**		    for TIMEOUT=NOWAIT blocked resources.
**  36		    Set session's degree of parallelism.
**  37-95         Not Used.
**  96-99	    NOT ALLOWED.  The server control block contains an OR
**		    of all session-level tracepoint bits.  In order to
**		    conveniently deal with the server-wide bits, we limit
**		    session tracepoint numbers to a multiple of 32 (an i4);
**		    namely 96 of them (0..95).
*/

/*
**  DMZ_MEM_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter.
**	27-jan-97 (stephenb)
**	    Add DM102 to disable replication in a database, placed here because
**	    it re-sets a flag in memory (tenuos, but there's no better place
**	    for it).
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  101		    Dump DMF memory pool to trace file.
**  102		    Disable/Enable replication in the current database.
**  103             Fill DMF memory with pad char if DM0M_ZERO not specified
**  104		    Fill DMF memofy with pad char after it is freed.
**  105		    Collect/display dm0m stats by "type".
**  106-199	    UNUSED.
*/

/*
**  DMZ_FIL_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  200-299	    UNUSED.
*/

/*
**  DMZ_LCK_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter.
**	24-may-1993 (bryanp)
**	    Add trace point DM302. This trace point augments normal lock_trace
**		by adding the tracing of internal buffer manager cache locks.
**		If you just want ordinary lock_trace, then just set DM1 (or
**		use "set lock_trace". If you want lock_trace+buffer cache locks,
**		set both DM1 and DM302. If you want to see only buffer manager
**		cache locks traced (why would you want this?), you're out of
**		luck.
**	21-jun-1993 (bryanp)
**	    Add trace point dm303, which is VMS only and sends a SS$_DEBUG
**		signal to the DMFCSP to ask it to enter the debugger (kind of
**		an SC905 for the CSP).
**	22-nov-1993 (jnash)
**	    Add trace point dm304: dump lockstat info on 
**	    page or table escalation deadlocks.
**	29-aug-96 (nick)
**	    Add dm305 - traces system catalog escalations caused by hitting
**	    maxlocks.
**	29-aug-1997 (bobmart)
**	    Add DM306 thru DM308 to control the logging of lock escalation
**	    messages when exceeding max locks per transaction or resource
**	    for system catalogs and user tables; observed only when
**	    corresponding pm configurable is marked 'disable'.  Retained
**	    earlier meaning for DM305.
**	13-Dec-1999 (jenjo02)
**	    Add DM309 to dump lock list information when deadlocks occur.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  300-301	    UNUSED.
**  302		    Buffer manager cache lock tracing.
**  303		    Send request to CSP to force it into the debugger.
**  304		    Dump lock trace information on page or table escalation
**		     deadlock.
**		    305 thru 308 pertain to the logging of lock escalations:
**  305		    Log msg when maxlocks hit on system catalog.
**  306		    Log msg when maxlocks hit on user table.
**  307		    Log msg when locks/trans. hit on system catalog.
**  308		    Log msg when locks/trans. hit on user table.
**  309		    Dump lock lists when deadlocks occur.
**  310-399	    UNUSED.
*/

/*
**  DMZ_BUF_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter.
**	10-apr-1989 (rogerk)
**	    Added DM421 trace point to flush buffer manager.
**	25-sep-1991 (mikem) integrated following change:  4-feb-1990 (rogerk)
**	    Added DM422 to toss unmodified pages from cache.
**	    Added DM425 to fix cache priority for a table.
**	25-sep-1991 (mikem) integrated following change: 25-feb-1991 (rogerk)
**	    Added DM426 to bypass group operations.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support: DM483 enables runtime enforcement of the
**	    only-fix-one-syscat-page-for-write protocol.
**	22-feb-1994 (andys)
**	    Integrate ingres63p changes.
**          Note that 6.4's trace point dm428 becomes dm430 in 6.5
**	23-may-1994 (jnash)
**	  - Add DM431-432 documentation.  
**	  - Add DM433 to fix the cache priority of all tables (at 6).  This 
**	    takes effect on tcbs created after the tracepoint is set.  This
**	    is equivalent to "set trace point DM425" on all tables.
**	18-sep-1995 (nick)
**	    6.4 tracepoint DM427 had made it to dm0p.c but clashes with
**	    other usage - rename to DM434.
**	01-Apr-1997 (jenjo02)
**	    Table priority project:
**	    DM425 left in place, tho it no longer does anything. Table priorities
**	    may now be stated when table is created/modified.
**	    Added DM435 to force dm0p to ignore fixed table priorities, for
**	    the paranoid.
**	31-Jan-2003 (jenjo02)
**	    Added DM414 to produce Cache Flush activity tracing.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DM405, DM406  to control MVCC tracing.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  400		    All.
**  401		    Buffer manager fix/unfix tracing.
**  402		    Buffer manager I/O tracing.
**  403		    Buffer manager mutex/unmutex tracing.
**  404		    Buffer manager BI generation.
**  405		    Trace MVCC CR activity
**  406		    Trace MVCC CR only journal activity
**  407-413	    UNUSED.
**  414		    Buffer manager Cache Flush activity trace.
**  415-419         Buffer manager queue consistency checks.
**  420		    Buffer manager performance statistics.
**  421		    Toss cached pages from buffer manager.
**  422		    Toss unmodified cached pages from buffer manager.
**  425		    Set buffer priority for given table (obsolete).
**  426		    Bypass group reads, only do single reads.
**  427		    Toss single page from cache.
**  428		    Force pages for single table from cache.
**  429-430         Check for loop in transaction queues for bug 55079.
**  431		    Disable page checksumming.
**  432		    Checks page checksum, but only writes warning if failure
**  433		    Fix the priority of all tables in the cache (obsolete).
**  434		    dmd_fmt_cb on BM CBs if dm0p_check_consistency fails.
**  435		    Ignore table cache priorities.
**  436-482	    UNUSED.
**  483		    Enforce the rule that only one syscat page can be fixed.
**  484-499	    UNUSED.
*/

/*
**  DMZ_SRT_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter.
**	24-feb-98 (inkdo01)
**	    Added 502, 503 for parallel sort machinations.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  500		    All.
**  501		    Simple sort statistics.
**  502		    Force use of parallel sort.
**  503		    Prevent use of parallel sort.
**  504-506	    Force 1/2/3 parallel sort threads
**  507		    UNUSED
**  508-510	    Force parallel sort exch buf to 500/5000/25000 rows.
**  511		    MODIFY statistics.
**  512		    Prevent use of online modify 
**  513		    Online modify diagnostics
**  514		    PCsleep during online modify
**  515		    PCsleep during online modify load phase
**  515-599	    UNUSED.
*/

/*
**  DMZ_AM_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter.
**	25-sep-1991 (mikem) integrated following change: 25-mar-1991 (bryanp)
**	    Added new btree traces (11,12,13,14,15,16)
**	21-jun-1993 (rogerk)
**	    Added trace to bypass freeing of empty overflow pages (DM617).
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  600		    ALL.
**  601		    BTREE-GET
**  602		    BTREE-DEL
**  603		    BTREE-PUT
**  604		    BTREE-REPLACE
**  605		    BTREE-SPLIT
**  606		    BTREE-MERGE
**  607		    BTREE_EXTEND
**  608		    BTREE-ALLOCATE
**  609		    BTREE-SEARCH
**  610		    BTREE-BUILD page dumps during build
**  611		    BTREE-DM1CIDX.C call trace
**  612		    BTREE-DM1CIDX.C extra edit checks enabled
**  613		    BTREE-DM1BBUILD.C simple call trace
**  614		    BTREE-print bad index page when logging compressed errmsg
**  615		    BTREE-call dmd_prall on table (dm615 <base> <index>)
**  616		    BTREE-Force use of index compression
**  617		    BTREE-bypass freeing of empty overflow pages.
**  618		    BTREE incr/decr key trace
**  619-620         reserved for BTREE
**  621		    HASH/ISAM/SEQ(HEAP)-GET
**  622		    HASH/ISAM/SEQ(HEAP)-DEL
**  623		    HASH/ISAM/SEQ(HEAP)-PUT
**  624		    HASH/ISAM/SEQ(HEAP)-REPLACE
**  625		    HASH/ISAM/SEQ(HEAP)-EXTEND
**  626		    HASH/ISAM/SEQ(HEAP)-ALLOCATE
**  627		    HASH/ISAM/SEQ(HEAP)-SEARCH
**  628-640	    reserved for HASH/ISAM/SEQ(HEAP)
**  650		    Page allocation/deallocation tracing.
**  651		    Build routine tracing.
**  652		    6.4->6.5 Conversion to new space management scheme.
**  653		    Overide the relfhdr!=invalid so convert test.
**  660		    Abort/Abort save log record tracing.
**  
*/

/*
**  DMZ_TBL_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter .
**	29-aug-89   (teg)
**	    Added Patch Table traces (01 - 06).
**	20-nov-89   (teg)
**	    Added check table trace (07) or DM707
**	06-dec-89   (teg)
**	    added check/patch fix/unfix page trace point (08) or DM708
**	11-sep-1991 (fred)
**	    Added DMPE trace to spread blobs out amongst extensions (DM711/11)
**	    and one to set the DMPE segment size to be quite small so that we
**	    can emulate large blobs without much data (DM712/12)
**	25-sep-1991 (mikem) integrated following change: 25-jun-1990 (bryanp)
**	    Documented trace point 10 for dumping page when detected invalid
**	06-mar-1996 (stial01 for bryanp)
**	    Add DM714 and DM715 to control server's page size.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add DM716 to force row-level locking.
**      05-may-99 (nanpr01,stial01)
**          Add DM720 to force duplicate checking before base table is updated
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  700		    UNUSED.
**  701		    Trace on EX_DMFMSG exceptions
**  702		    Show path through DM1U Code
**  703		    Dump the structural portions of a data page from dm1u_datapg
**  704		    Memory Allocation/deallocation trace.
**  705		    Show ADF Data Value blocks for each attribute in tuple, as
**			initialized by dm1u_init.
**  706		    Override the ABORT mechanism that kicks in if dm1u_talk
**			cannot format a message or cannot forward that message
**			to the FE.
**  707		    Show path through btree check table code
**  708		    Show when pages are fixed and when they are unfixed
**  709		    show path through isam check table code.
**  710		    When find_next() decides whole page is invalid, dump it.
**  711		    Make DMPE place only one tuple per table extension.  This,
**		    in conjunction with trace point 30 tests extended DMPE
**		    functionality without consuming huge amounts of disk space.
**  712		    Set Size allowed in peripheral support to a small value.
**		    This forces the use of many tuples per blob object on
**		    arbitrarily small blobs.
**  713		    Toss TCB from server.
**  714		    Set server's default page size (dm714 <page_size>)
**  715		    Automatically choose random page size (for testing)
**  716             Force row-level locking.
**  717             Force cursor stability isolation level.
**  718             Force repeatable read isolation level.
**  719             Force btree reposition on first get.
**  720             Force duplicate checking before base table is updated
**  721             Set server's default page type (dm721 <page_type>)
**  722		    Trace point to validate blob coupons before insert/replace
**  723		    Trace point to validate update lock protocols
**  724-799	    UNUSED.
*/

/*
**  DMZ_REC_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter .
**	08-apr-2010 (toumi01) SIR 122403
**	    Dump AES encryption records to trace file.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  800		    UNUSED.
**  801		    Skip tuples that cannot be returned by the dm1c routines.
**  802		    Return garbage tuple rather than error if dm1c routine
**		    cannot return tuple.
**  803		    Allow dm1c_p{delete,replace} to succeed in spite of dmpe
**		    errors.
**  804		    Show actions that will be taken by DMF Aggregate Processor.
**  805		    Dump AES ENcrypt buffers to trace file.
**  806		    Dump AES DEcrypt buffers to trace file.
**  807-899	    UNUSED.
*/

/*
**  DMZ_XCT_MACRO(t)
**
**  T              Description
**  __________     -----------------------------------------------------------
**  900   	    UNUSED.
**  901		    Disable before image logging for performance testing.  This
**		    tracepoint is used to simulate expected log throughput
**		    when the compressed logging project is completed.
**  902		    Compress replace log "old" record images. 
**  903-999	    UNUSED.
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter .
**      05-mar-92 (mikem)
**          Added DM901 to disable before image logging.  Should only be
**          used for simulating compressed logging.  When compressed logging
**          is implemented the trace point will not be needed anymore.
**      18-oct-93 (jnash)
**          Add DM902 to compress replace log "old" record images.
**	30-Apr-2001 (jenjo02)
**	    Added DM903 to control displaying of rollback trace info.
*/

/*
**  DMZ_UTL_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter .
**	26-may-1998 (nanpr01)
**	    Added 1002 and 1003 to change the exchange buffer size and number
**	    of buffers for parallel index creation.
**
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  1000	    UNUSED.
**  1001	    CONVERT60 - bypass SBI logging in APPEND to system catalog.
**  1002	    Change the exchange buffer rows for parallel index.
**  1003	    Change the number of exchange buffers for parallel index.
**  1004-1099	    UNUSED.
*/

/*
**  DMZ_CRH_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter .
**      18-may-1988 (teg)
**	    Added #22 and #23.  Also, #10 not in code, so I removed
**	    it from the list.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  1100 	    UNUSED.
**  1101	    BTREE-Delete Secondary Index, Crash before Leaf written.
**  1102	    BTREE-Delete Secondary Index, Crash before return.
**  1103	    BTREE-Delete, Crash before Leaf written.
**  1104	    BTREE-Delete, Crash before return.
**  1105	    BTREE-Put, Crash after writing data but before writing leaf.
**  1106	    BTREE-Put, Crash before return.
**  1107	    BTREE-Replace, Crash after delete, before insert.
**  1108	    BTREE-Replace, Crash before return.
**  1109	    BTREE-Replace Secondary Index, Crash after delete, before
** 			insert.
**  1110	    not used  RESERVED FOR BTREE (-- teg)
**  1111	    BTREE-Allocate, Crash after splits.
**  1112	    BTREE-File Extend, Crash during extend.
**  1113	    BTREE-File Extend, Crash before chaining of pages.
**  1114	    BTREE-File Extend, Crash during chaining of pages.
**  1115	    BTREE-File Extend, Crash before log write.
**  1116	    BTREE-File Extend, Crash after header written.
**  1117	    BTREE-File Extend, Crash during freeing of page.
**  1118	    BTREE-Split, Crash before commit of root split.
**  1119	    BTREE-Split, Crash during split before log write.
**  1120	    BTREE-Split, Crash during split, middle of writing pages.
**  1121	    BTREE-Split, Crash during split, prior to commit.
**  1122	    BTREE-ROOT Split, crash during split, before page unfixed
**  1123	    BTREE-ROOT Split, crash during split, after page unfixed
**  1124-1140	    reserved for BTREE
**  1141	    HASH/ISAM/SEQ(HEAP)-File Extend, Crash during extend.
**  1142	    HASH/ISAM/SEQ(HEAP)-File Extend, Crash before link.
**  1143-1150	    reserved for HASH/ISAM/SEQ(HEAP)
**  1151-1199	    unused
*/

/*
**  DMZ_CLL_MACRO(t)	- Call interface tracing.
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter .
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  1200	    ALL.
**  1201	    Call operation.
**  1202	    Call return status.
**  1203	    Call input parameters.
**  1204	    Call output parameters.
**  1205	    Must be set to get 1201, 1202, 1203, 1204, or 1210
**  1206-1209
**  1210	    Call timing enable.
**  1211	    Call timing display.
**  1212	    Call histogram timing display.
**  1213-1219
**  1220	    Call memory checking
**  1221-1229
**  1230-1299
**
**  History:
**	31-jan-1994 (mikem)
**	    Sir #58499
**	    Added dm1205.  After this change, when dm1205 is set then any or
**	    all of 120{1,2,3,4} and/or 1210 can be used in a non xDEBUG server.
**	    Rather than xDEBUG out the calls to dmd_call() in dmf_call(), we
**	    now test DMZ_CLL_MACRO(05) and call it if it is set.  You must still
**	    set the other trace points depending on what you want.
*/

/*
**  DMZ_ASY_MACRO(t)	- Asynchronous thread tracing.
**
**  These trace flags turn on tracing for the Fast Commit, Write Behind,
**  and Recovery threads.
**
**  They are also used for many recovery trace options.
**
**  History:
**	02-sep-1988 (rogerk)
**	    Created.
**	25-sep-1991 (mikem) integrated following change:  4-feb-1991 (rogerk)
**	    Added DM1305 to force consistency point.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Project:
**		Documented use of DM1306 and DM1307 to trigger recovery tracing.
**	21-may-1993 (jnash)
**	    Add DM1308 to extend length of CP.
**	23-aug-1993 (rogerk)
**	    Add DM1315 to enable page information tracing.
**	31-jan-1994 (jnash)
**	    Add DM1316 to bypass DMF memory dump during dmd_check().
**	01-Apr-2004 (jenjo02)
**	    Added DM1320 to enable dmrAgents.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  1301	    Print Statistics for Fast Comit Thread.
**  1302	    Print Statistics for Write Behind Thread.
**  1303	    Give stats on time to execute Consistency Point.
**  1305	    Signal Consistency Point.
**  1306	    Triggers rcp_verbose in dmfrecover.c
**  1307	    Triggers Log Sequence Number displays in reovery passes.
**			This is useful primarily for cluster testing, where
**			LSN's are interesting values to observe.
**  1308	    Extend CP duration an additional 30 seconds.
**  1309	    Use verbose method of log record tracing in RCP recovery.
**  1310	    Give page header information of pages used during recovery.
**  1311	    Hex dump pages used in recovery.
**  1312	    Bypass Log File dumping when errors occur in recovery.
**  1313	    Triggers acb_verbose in dmfarchive.c
**  1314	    Signal Archive Cycle.
**  1315	    Enable verbose tracing of page information in dmve routines.
**			Causes page LSN's to be traced during recovery.
**			Must be set to use DM1310 or DM1311.
**  1316	    Bypass DMF memory dump during dmd_check().
**  1317	    Triggers error following rollforwarddb CKrestore().
**  1318	    Triggers error following ckpdb CKbackup().
**  1319            UNUSED.
**  1320	    Enable dmrAgents: do SI updates in parallel rather
**		    than serially.
**  1321-1399	    UNUSED.
*/

/*
**  DMZ_TST_MACRO(t)	- Test trace points
**
**  These trace points are generally for immediately executing some test
**  function.
**
**  History:
**	31-oct-1988 (rogerk)
**	    Created.
**	10-apr-1989 (rogerk)
**	    Added DM1420 trace point for multi-server fast commit.
**	19-apr-1990 (rogerk)
**	    Added DM1419 to crash logging system.
**	25-sep-1991 (mikem) integrated following change: 25-feb-1991 (rogerk)
**	    Added archiver test trace points.
**	29-August-1992 (rmuth)
**	    - Add DM1430 fail after converting iirelation and iirel_idx.
**	    - Add DM1431 fail after converted core and one DBMS catalog
**	    - ADD DM1432 fail while dm2u_convert_table converting a table.
**	    - Add DM1433 fail after physically adding FHDR/FMAP to file.
**	26-apr-1993 (bryanp)
**	    Add trace point DM1434 to force the logfile to disk. This is useful
**		just before crashing the system, so that you know what recovery
**		you're going to do.
**	26-jul-1993 (mikem)
**	    - Add DM1436 to test performance of LG calls.
**	    - Add DM1437 to test performance of LGwrite() calls.
**	    - Add DM1438 to test performance of ii_nap() calls (unix only).
**	    - Add DM1439 to test performance of system calls from server.
**	    - Add DM1441 to test performance of cross process sem calls.
**	21-Mar-2006 (jenjo02)
**	    Add DM1442 to enable/disable optimized log writes.
**	    Add DM1443 to enable/disable LG optimized write tracing.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  1400-1418	    UNUSED.
**  1419	    Crash Logging system.
**  1420    	    Get lock to support syncing in fast commit test driver.
**  1421	    Write Archiver test enable log record.
**  1422	    Write Archiver diskfull test log record.
**  1423	    Write Archvier bad-jnl-record test log record.
**  1424	    Write Archiver AV test log record.
**  1425	    Write Archiver stop-at-next-cp test log record.
**  1426	    Write Archiver online backup diskfull test log record.
**  1427	    Write Archiver memory allocate test log record.
**  1429	    Write Archiver test disable log record.
**  1430	    see above.
**  1431	    see above
**  1432	    see above
**  1433	    see above
**  1434	    Force the logfile to disk, to ensure all log recs are out.
**  1435	    Mark database inconsistent.
**  1436	    Add DM1436 to test performance of LG calls.
**  1437	    Add DM1437 to test performance of LGwrite() calls.
**  1438	    Add DM1438 to test perf of ii_nap() calls (unix only).
**  1439	    Add DM1439 to test performance of system calls from server.
**  1440	    display current set of work locations
**  1441	    Add DM1441 to test performance of cross process sem calls.
**  1442	    Enable/disable optimized log writes.
**  1443	    Enable/disable optimized log write tracing.
**  1444            Set the last TABLE_ID assigned in a DB (in CNF file).
**  1445-1499	    UNUSED.
*/

/*
**  DMZ_XXX_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter .
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  1500-1599	    UNUSED.
*/

/*
**  DMZ_YYY_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter.
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  1600-1699	    UNUSED.
*/

/*
**  DMZ_ZZZ_MACRO(t)
**
**  History:
**	16-jun-1986 (derek)
**	    Created for Jupiter .
**
**  T              Description
**  __________     -----------------------------------------------------------
**
**  1700-1799	    UNUSED.
*/

/* Funtion prototypes definitions. */

FUNC_EXTERN DB_STATUS dmf_trace(
    DB_DEBUG_CB	    *debug_cb);

FUNC_EXTERN void dmf_trace_recalc(void);

FUNC_EXTERN VOID dmf_scc_trace(
    char    *msg_buffer);

FUNC_EXTERN i4 dmf_perftst(
    i4             tracepoint_num,
    i4             iterations,
    i4             arg);
