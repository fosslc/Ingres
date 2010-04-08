/*
** Copyright (c) 1986, 2004 Ingres Corporation
**
**
*/

/**CL_SPEC
** Name: LG.H - Logging Primitives.  (GL version)
**
** Description:
**      This file contains the definitions of types, functions and constants
**	needed to make calls on the compatibility library LG functions.  These
**	routines performs all logging services for the DMF. Furthermore, non-DMF
**	uses of the LG routines are prohibited by definition.
**
**  NOTE: This file is now in the GL! This means that a common version of this
**	  file is shared among ALL systems. Changes to this file must be
**	  submitted to the CL committee for approval.
**
** Specification:
**
**   Description:
**	Log file operations required for support of transaction logging.
**
**   Intended Uses:
**	The LG services are intended for use by the DMF code to manage the
**	log file and its associated memory resident data structures.  The log
**	file contains a sequence of log records that describe the operations
**	performed on database objects by the DMF code.  These log file records
**      are used to recover database objects which are left in an inconsistent
**	state in the event of a software, hardware or user initiatated failure.
**      The log records can also be used to represent a history of changes to the
**	database.
**	
**   Assumptions:
**	It is assumed that the reader is familar with the basic concepts of
**	a write-ahead log (WAL).
**
**	The host operating system needs to have the capability of guaranteeing
**	that the log data has been safely written to the log file.
**
**	The actual log file can be implemented using a DI type file of the
**	appropriate block size, or by a specific implementation for LG type
**	files.
**
**	The performance of the LG routines is paramount to the performance of
**	the transaction logging, archiving and recovery system.  Special care
**	should be taken to insure that the LG routines, especially LGwrite,
**	are implemented in a efficent manner.
**
**   Definitions:
**   
**(	Archive		    - This describes the process of purging the log
**			      file of log records which are stale or that have
**			      been safely copied to secondary permanent storage.
**			      This secondary permanent storage is called a 
**			      journal file and is managed by the JF CL routines.
**			      The program that performs these operations is called
**			      the 'archiver'.
**			      
**
**	Consistency point   - A consistency point is a special set of log
**			      records that are written to the log file at
**			      periodic intervals.  The consistency point is used
**			      in the crash recovery process to aid in locating
**			      the last log page written, the set of open
**			      databases, and the set of active transactions.
**			      Without a consistency point, the whole log file
**			      would have to be searched to locate the last page
**			      written, and to recalculate the set of databases
**			      and transactions.
**
**			      The interval between consistency points can be
**			      set by the client.  For example: the DMF recovery
**			      process sets the consistency point interval when
**			      it is started.
**
**	Database	    - The logging system maintains a small set of
**			      information about an object which it calls a 
**			      database (equivalent to an INGRES database.)
**			      This information is used to track usage of a
**			      database, to tag log records with the database that
**			      they belong to, to find the disk location of the
**			      database for the recovery system.
**
**			      An LG call exists to return all the internal
**			      information about a database.  The structure
**			      LG_DATABASE describes the form of this
**			      information.
**
**	Database_id	    - The LG system assigns a unique identifer to every
**			      database it handles. This identifer is used
**			      by other LG calls to identify a specific database.
**
**	Event		    - The logging system remembers when certain events
**			      have occured that a client should handle.  These
**			      events include: the need to write a consistency
**			      point, archivable data exists in the log file,
**			      a process terminated leaving a transaction in-
**			      progress.  The client code is expected to handle
**			      the event and then clear the event with a call
**			      to LGalter().  For example:  the DMF recovery
**			      process waits for the event associated with
**			      consistency point generation.  The following 
**                            events are defined.
**                            
**				LG_E_ARCHIVE
**				LG_E_BCP
**				LG_E_CPFLUSH
**				LG_E_ECP	
**				LG_E_LOGFULL
**				LG_E_RECOVER
**				LG_E_SHUTDOWN	
**				LG_E_IMM_SHUTDOWN	
**				LG_E_ACP_SHUTDOWN
**				LG_E_PURGEDB
**				LG_E_ONLINE
**				LG_E_OPENDB
**				LG_E_CLOSEDB
**				LG_E_ECPDONE
**				LG_E_CPWAKEUP
**				LG_E_FCSHUTDOWN
**				LG_E_SBACKUP
**				LG_E_M_ABORT
**			        LG_E_M_COMMIT
**			        LG_E_RCPRECOVER
**			        LG_E_START_ARCHIVER
**			        LG_E_BACKUP_FAILURE
**				LG_E_EMER_SHUTDOWN
**				LG_E_ARCHIVE_DONE
**				
**
**	Log Address	    - When log records are written to the log file, the
**			      log record is assigned a number called the log
**			      address.  The log address represents the location
**			      in the log file where the record was written.
**			      The log address points to the offset of the last
**			      log word containing data for this log record.
**
**			      The log address is composed of three components:
**			      the log sequence number, the log page number, and
**			      the log word offset.  The log sequence number
**			      counts the number of times that log addresses have
**			      wrapped around to the beginning of the circular
**			      log file.  The log page number, is a number from
**			      1 to the number of pages in the log file.  The log
**			      word offset is a number from 0 to the size of a
**			      log page minus 4 (i.e. if log page size = 4096 then
**			      maximum log word offset would be 4092).
**
**			      The log address is designed to fit into a 64-bit
**			      word.  The sequence number always uses 32-bits.
**			      The page number and the offset vary in size
**			      depending on the size of a log page.  The offset
**			      uses as many bits as needed to represent an offset
**			      for a log page size of 4096..32768 bytes.  The
**			      page uses the remaining bits after accounting for
**			      the number of bits in the offset.
**
**			      +-----------------------------------------------+
**			      | Sequence:32 | Page:(17..21) | Offset:(15..11) |
**			      +-----------------------------------------------+
**
**			      The minimum sequence number is 2.  The minimum
**			      page number is 1.
**
**			      The structure LG_LA defines a log address.
**
**	Log File	    - The log file is an operating system file that is 
**			      used to contain the log pages.  The log file
**			      if logically composed of two parts: the log file
**			      header and the log file data.
**
**			      The log file header contains information about the
**			      log file.  See 'Log Header'.
**
**			      The log file data area encompasses log pages 1
**			      through the number of pages in the log file.  When
**			      the last physical log page has been written,
**			      physical page 1 becomes the next log page 
**			      (I.E. a circular file.)  The log address has
**			      its sequence number field incremented when this
**			      wraparound occurs.
**
**
**			      +---------------------------------------------+
**			      | Page 0  | Page 1 | Page 2 | Page 3 | Page 4 |
**			      |         |        |        |        |        |
**			      |   Log   | (2,1)  | (2,2)  | (2,3)  | (2,4)  |
**			      | Header  |        |        |        |        |
**			      +---------------------------------------------+
**
**	Log Header	    - The first page (page 0) of the log file is called
**			      the log header.  The log header contains
**			      information that describes the log file.  This
**			      information includes: the log file format version,
**			      the log header checksum, the log page size, the
**			      number of log pages, the log file status
**			      indicators, the log address of the beginning of
**			      file, the log address of the end of file, and the
**			      log address of the last consistency point.
**
**			      Various LG calls exist to read, change and write
**			      the log file header.
**
**			      The structure LG_HEADER describes the log header.
**
**	Log Page	    - A log page is composed of two components: the log
**			      page header and the log page data area.  The log
**			      page header contains the following information:
**			      the log address of the last complete log
**			      record written to the page, the checksum of the
**			      log page, the number of bytes used on this log page,
**			      and an extra field that is initialized to zero.
**
**			      The log address of the last full log record is 
**			      used to position to a full record on an arbitary
**			      page.  If the log address offset is zero then
**			      either nothing was written to the page or a
**			      very long log record that spans multiple pages
**			      was written to this page.
**
**			      The checksum is used to guarantee that the log
**			      was successfully written and subsequently read
**			      from the log file.  The checksum includes the log
**			      page header and chunks of data from various sections
**			      of the log page.  The contents of the whole log
**			      are not included because of the cost in CPU
**			      resources.
**
**			      The internal format of the log page is known to
**			      the client.  The crash recovery code uses the
**			      log page header information to locate the last
**			      log page written.
**
**	Log Record	    - A log record is composed of two components: the
**			      log record header(LRH) and the log record data area.
**
**			      The client log record is encapsulated in a log
**			      record header constructed by the LG routines.
**			      This header contains the following information:
**			      the length of the encapsulated log record, the
**			      database identifier for the database this log
**			      record was written for, the transaction identifer
**			      of the transaction which wrote the log record,
**			      the sequence number of this log record relative
**			      to the beginning of the transaction, the log
**			      address of the previous log record written by
**			      this transaction, the client log record, and
**			      again the length of the encapsulated log record.
**                            Both LRH and the length at the end are returned
**                            to the caller as well as the record.
**
**			      The length of the log record is stored on both
**			      ends of the log record to facilitate forward and
**			      backward traversal of the log records.  This
**                            information is for use of the LG routines for
**                            reading forward or backward and for the use
**                            of the client.
**
**			      Encapsulated log records are always rounded up
**			      to a multiple of a log word (see below).
**
**			      Client log records have a maximum size denoted by
**			      the constant LG_MAX_RSZ (Currently 4096).
**
**			      The part of the log record header that occurs
**			      before the client data cannot span a log page.
**
**	Log Word	    - This is the minimum unit of allocation for a log
**			      page and is 4 bytes.  All log records are 
**                            padded to be a multiple of log words so that 
**                            information is stored on
**			      aligned boundaries in the log file, thus allowing
**			      direct access to structures stored in the log
**			      file.  This also guarantees that each of the
**			      length fields in a log record never spans a page.
**                            The pad bytes are sent back to the client.
**
**	Logging_id	    - The LG system assigns a unique identifer to every
**			      process that opens the log file. This identifer
**			      is used by other LG calls to determine which
**			      process is making the request.
** 
**	Master		    - The logging system recognizes one process that
**			      has the log file open as the master process.  The
**			      master process is given the ability to alter any
**			      of the logging system data structures.
**
**			    - The master is responsible for initializing the
**			      logging system and for determining the
**			      operational policies.
**
**	Position Context    - Read operations from the log file require a
**			      context area to maintain position information
**			      between successive requests.  The LGposition()
**			      call initializes a client supplied data area
**			      and successive LGread() calls update this context.
**			      The size of the context area is denoted by the
**			      constant LG_MAX_CTX.
**
**	Transaction	    - The logging system maintains a small set of 
**			      information about transactions that are in
**			      progress.  This information is used to track
**			      the progress of an active transaction. The
**			      information recorded about a transaction 
**			      includes:  the physical transaction identifier,
**			      the database the transaction is using, the number
**			      of log records written by this transaction, the
**			      log address of the first log record for the
**			      transaction, the log address of the last log
**			      record for the transaction.
**
**			      An LG call exists to return all the internal
**			      information about a transaction.  The structure
**			      LG_TRANSACTION describes the form of this
**			      information.
**
**	Transaction_id	    - The LG system assigns a unique identifer to every
**			      transaction that is started.  This identifier is
**			      used by other LG calls to determine the transaction
**			      that is being referenced.  This is different than
**			      the external transaction identifer that is also
**			      assigned at the same time.  The external
**			      transaction identifer is used by distributed
**			      to remember the transactions at each site which
**			      comprise a distributed transaction.
**
**)
**   Concepts:
**
**	The LG routines provide the low level support for maintaining the
**	memory resident data structures associated with the log file.
**	The actions that these routines perform handle the majority of the
**	normal processing performed by the logging system.  These actions
**	include:  reading the log file, writing the log file, writing the
**	log file header, maintaining the logging system data structures.
**
**	The client is responsible for rare, time consuming, and complicated
**	processing.  This type of processing include; verifing the integrity
**	of the log file, initializing the memory resident copy of the log
**	file header, writing consistency points, resolving transactions
**	that have been orphaned, initializing the log file.  These actions
**	require that decisions be made and the appropriate changes sent
**	to the logging system.
**
**	The logging system notifies the client of certain events that the
**	client may wish to handle.  The logging system records that a
**	specific event has occured. The client can recognize an event
**	one of three ways: the client can wait for a specific event to
**	occur, the client can wait for any event in a set of events to
**	occur or the client can poll the current event settings.
**  
**	In the following example, the division of labor between the client
**	and the logging system is outlined.
**
**(	    o  The client initializes the logging system and sets the beginning
**	       of file and the end of file addresses.
**
**	    o  The logging system will allow writes to occur to the log file as
**	       long as space exists in the log file.
**
**	    o  When the log file is full, the logging system posts an event
**             (LG_E_LOGFULL) that signifies that the log file is full.  
**             All further writes
**	       to the log will be stalled until the enough free space has
**             been obtained to bring the file below its full indicator.
**             The recovery process and archiver are responsible for
**	       freeing up space in the log file.
**
**	    o  The client must free up space in the log file and reset the
**	       beginning of file address.  Once the beginning of file address
**	       is changed, the logging system will restart any log writes in-
**	       progress and allow new log writes.
**
**	    o  When the client wishes to stop the logging system, the client
**	       sets the shutdown flag.
**
**	    o  When the logging system notices the shutdown flag has been set
**	       it disallows any new transactions from starting.  When the last
**	       transaction has completed, the logging system will post the event
**	       signifying that shutdown is complete.
**)
**	The logging system is the mechanism for manipulating the log file.  The
**	client is responsible for setting the policy on how the log file should
**	be manipulated.
**
**	The logging system recognizes one process that has the log file open
**	as a master process.  The master process is the special process that is
**	allowed to change any of the logging systems internal data structures.
**	The master process is the process that sets the policy.  The master
**	process must exist before any other process can start.  If the master
**	process terminates, all other processes will receive errors from
**	succeeding logging system calls.
**
**	The logging system data structures can be displayed by using the
**	LGshow() routine.  This routine can return any of the information that
**	the logging system maintains.
**
**	The logging system data structures can be changed by using the
**	LGalter() routine.  This routine can change most of the information
**	that the logging system maintains. For example the log page can be 
**      set to one of the following sizes: 4096, 8192, 16384,
**	32768.
**
**	The log file can be up to 4G bytes in size.
**
**	The log file can be mirrored (duplexed, shadowed) across multiple
**	physical devices so as to eliminate a single point of failure, and
**	the real threat of lost data.  The log file contains the most recent
**	state of the database.  If the log file is lost the databases can be
**	restored to a previous point in time, but some amount of work will be
**	lost.
**
** History:
**      17-jun-1986 (Derek)
**	    Created.
**	09-aug-1988 (mmm)
**	    Brought forward lg.h from 6.1.
**	30-sep-1988 (rogerk)
**	    Added new flag to LGwrite.
**	24-oct-1988 (mmm)
**	    Brought forward changes to lg.h from vms 9-aug-88 -> 9-oct-88.
**	09-Nov-1988 (edHsu)
**	    Added LG_A_ACPOFFLINE to tell logging system to turn on start
**	    archiver event flag
**	15-may-1989 (mikem)
**	    Added internal error defines: LG_BADMUTEX, E_CL0F30_LG_NO_MASTER,
**	    and E_CL0F31_LG_CANT_OPEN.
**      28-feb-1989 (rogerk)
**          Added LGalter option LG_A_BUFMGR_ID.
**	15-may-1989 (EdHsu)
**	    Integrate online backup feature.
**	26-may-1989 (rogerk)
**	    Add LG_EXCEED_LIMIT return value.
**      15-may-1989 (sandyh)
**          Added tr_sess_id to LG_TRANSACTION struct to return session
**          id for transaction info.
**	20-Jan-1989 (ac)
**	    Added 2PC support.
**	11-sep-1989 (rogerk)
**	    Added LG_A_DMU alter option.
**	25-sep-1989 (rogerk)
**	    Added LG_A_RCPRECOVER alter option.
**	    Added LG_E_RCPRECOVER event option.
**	03-dec-1989 (mikem)
**	    Added LG_E_START_ARCHIVER event option.
**	    Also added 2 new show options: LG_S_FDMPCP, LG_S_LDMPCP
**	02-jan-1990 (mikem & rogerk)
**	    Added LG_E_BACKUP_FAILURE, LG_A_CKPERROR in order to signal to the
**	    ckpdb process from the ACP that the online checkpoint has failed.
**	    Added LG_E_1PURGE_ACTIVE, LG_E_2PURGE_COMPLETE events and
**	    LG_A_SONLINE_PURGE, LG_A_JCP_ADDR_SET alter calls for new online
**	    checkpoint protocol.
**	10-jan-1989 (mikem)
**          Added internal error defines: LG_BADMUTEX, E_CL0F30_LG_NO_MASTER,
**          and E_CL0F31_LG_CANT_OPEN.  (to keep unix and vms defines in step, 
**	    since the two cl's share an error message file).
**	30-jan-1990 (ac)
**          Added LG_S_STAMP to return the commit timestamp for read only
**	    transaction.
**	12-feb-1990 (rogerk)
**	    Added LG_DBID_MATCH macro which can be used to compare two
**	    database id's to see if they are different instances of the
**	    same id.
**	23-mar-1990 (mikem)
**	    Added lgs_stat_misc stat field to allow easier access to new stats.
**	22-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Remove superfluous "typedef" before structures.
**	31-may-90 (blaise)
**	    Remove superfluous "typedef" before structures (did I forget to 
**	    it above?);
**	    Integrated changes from termcl:
**		Add LG_DBINFO_MAX constant to define the largest database
**		info buffer that can be put into an LG_DATABASE structure;
**		Change the LG_DATABASE definition to use this constant;
**		Add new error E_CL0F32_LG_SEM_OWNER_DEAD, to be returned
**		when the LG cross process semaphore is held by a process
**		which has exited.
**      04-feb-1991 (rogerk)
**          Added lgs_bcp_stall_wait and took bcp stalls out of the
**          lgs_stall_wait count statistic.
**      15-aug-91 (jnash) merged 10-oct-90 (pholman)
**          Integrated dual logging support (ac) from VMS (LG_DUAL_OPEN_FAIL,
**          LG_A_COPY etc).
**      25-sept-91 (jnash) 
**          As part of 6.4/6.5 merge, make VMS and UNIX copies of this file
**          identical except for lgs_stat_misc field of LG_STAT.
**	30-oct-1991 (bryanp)
**	    Added LG_DUALLOG_MISMATCH as a return code from LGopen().  Also
**	    added LG_DISABLE_PRIM and LG_DISABLE_DUAL, which are internal to
**	    LG.  Added two new fields to the LG_HEADER: lgh_timestamp and
**	    lgh_active_logs. See the LG_HEADER section for details.  Added
**	    LG_A_SERIAL_IO to set serial I/O mode on or off.  Added
**	    LG_A_TP_ON/LG_A_TP_OFF/LG_S_TP_VAL and the LG_T_* section in
**	    support of automated testing.  Added LG_S_TEST_BADBLK for automated
**	    testing.  Added LG_E_EMER_SHUTDOWN for automated testing.  Added
**	    prototypes for LG functions.
**	28-jan-1992 (bryanp)
**	    Resolved final differences between VMS & Unix versions and placed
**	    this file into the GL.
**	03-aug-1992 (jnash)
**          Reduced logging project.
**	      - Add LG_LSN definition.
**	18-sep-92 (ed)
**	    Made changes related to increasing DB_MAXNAME to 32
**	16-oct-1992 (jnash) merged 10-oct-1992 (rogerk)
**	    Reduced logging project.
**	      Added PR_CPAGENT process status for LG_PROCESS structure.
**	      Added LG_A_CPAGENT mode for LGalter calls.
**	12-oct-1992 (bryanp)
**	    Fix tr_status bit definitions following LG/LK mainline changes.
**	20-oct-1992 (bryanp)
**	    Add new LGalter codes: LG_A_GCMT_THRESHOLD, LG_A_GCMT_NUMTICKS.
**	2-nov-1992 (bryanp)
**	    Add LG_N_BUFFER and the LG_BUFFER type for logstat use.
**	17-nov-1992 (bryanp)
**	    Add prototype for LGidnum_from_id().
**	11-jan-1993 (jnash)
**	    Reduced logging project.  Add LG_A_RES_SPACE.
**	18-jan-1993 (rogerk)
**	    Added LGalter option LG_A_LXB_OWNER.
**	08-feb-1993 (jnash)
**	    Added LGreserve FUNC_EXTERN.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:  Added external database id information
**		to the LG_DATABASE and LG_TRANSACTION structures.  Also included
**		last-journal-address in LG_DATABASE info block.
**	    Removed LG_ADDONLY flag.  Recovery processing now uses normal LGadd.
**	    Removed LG_A_JCP_ADDR_SET, LG_A_EBACKUP, LG_A_PURGEDB
**		and LG_A_SONLINE_PURGE alter cases.  These are no longer used.
**	    Removed LG_S_CDATABASE, LG_S_LDATABASE, LG_S_ABACKUP, LG_S_JNLDB,
**		and LG_S_PURJNLDB show cases.  These are no longer used.
**		Changed LG_S_DATABASE case to now return db information for a 
**		database identified by the lpd id.
**	    Removed LG_E_1PURGE_ACTIVE, LG_E_2PURGE_COMPLETE event states.
**	    Removed DB_EBACKUP database state.
**	    Added LG_A_DBCONTEXT alter event.
**	    Added LG_A_ARCHIVE_DONE alter event.
**	    Added LG_NORESERVE flag to LGbegin.
**	15-mar-1993 (jnash)
**	    B49941. Bump LG_MAX_RSZ from 4132 to 4232.  Apparently this 
**	    number can be increased until it approaches twice the minimum log 
**	    block size (currently 4096).
**	26-apr-1993 (bryanp)
**	    6.5 Cluster support:
**		Renamed the LG_LA components to la_sequence and la_offset, as
**		    part of replacing the DM_LOG_ADDR type with the LG_LA type.
**		Modified prototype for LGforce since it now works with LSNs.
**		Removed LPS field from LGread, since it no longer returns a
**		    Log Page Stamp.
**		Deleted prototypes for LGC* routines which no longer exist,
**		    including: LGCalter, LGCopen, LGCclose, LGCforce,
**		    LGCposition, LGCread, LGCshow, LGCwrite.
**		Added prototype for LGCn_name().
**		Added new LG_FULL_FILENAME and LG_ONELOG_ONLY flags to LGopen
**		    for use by standalone logdump.
**		Added new LGalter/LGshow code LG_A_NODELOG_HEADER to set and
**		    examine the header for a particular node's log.
**		Don't automatically cast the arguments in the LSN and LGA
**		    comparison macros. This helps ensure that callers pass the
**		    correct argument types to this macros (i.e., they don't pass
**		    the address of an LG_LA to an LSN comparison macro, etc.).
**		Add db_sback_lsn to the LG_DATABASE for showing the LSN of the
**		    start of the backup.
**		Add LG_S_CSP_PID to show the CSP Process ID.
**		Added the gross argument-passing structure LG_HDR_ALTER_SHOW to
**		    support showing and altering the header for a non-local log
**		    file.
**	21-jun-1993 (andys)
**	    Add FUNC_EXTERN LG_cluster_node. 
**	    Change LGcopy, LGerase interfaces.
**	    Replace LG_NOHEADER with LG_LOG_INIT.
**	21-jun-1993 (rogerk)
**	    Added LG_IGNORE_BOF flag to cause LGopen to do the logfile
**	    scan for bof_eof even if the logfile header status indicates that
**	    the bof/eof values in the header are valid.  This can be used by
**	    logdump to dump entire logfiles.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Use new journal and dump log address fields.  Changed LG_DATABASE
**	    struct fields to reflect this.  Changed LG show options to find
**	    the system dump and journal windows - they are now combined
**	    into a single archive window.
** 	26-jul-1993 (jnash)
**	    Add LGinit_lock() FUNC EXTERN.  
**      09-sep-1993 (smc)
**          Changed LG_DBID to be a PTR so that it can continue to be
**          used to hold addresses on 64 bit machines.
** 	20-sep-1993 (jnash)
**	    Add LGreserve() "flag" param and LG_RS_FORCE flag value.
**	18-oct-93 (rogerk)
**	    Added new information for LGshow transaction calls:
**	        TR_ORPHAN, TR_NORESERVE transaction status flags.
**		tr_reserved_space - amount of log file space reserved by xact.
**          Changed LG_DBID back to be a i4.  Change will probably
**	    conflict with 64 bit integration which is planning the same change.
**	    Added alter options: LG_A_CACHE_FACTOR, LG_A_UNRESERVE_SPACE
**      23-may-1994 (jnash)
**          Add LGalter LG_A_LFLSN option and the LG_LFLSN_ALTER structure, 
**	    used to initialize the logging system "last forced lsn".
**	08-Dec-1995 (jenjo02)
**	    Added LG_S_MUTEX function to LGshow(), LG_MUTEX structure in
**	    which logging system semaphore stats are returned.
**	06-mar-1996 (stial01 for bryanp)
**        Increased LG_MAX_RSZ to allow logging 64K before images for the
**            multiple page size project.
**	22-Apr-1996 (jenjo02)
**	    Added lgs_no_logwriter, count of times we needed a logwriter
**	    thread but none were available.
**	01-Oct-1996 (jenjo02)
**	    Added lgs_max_wq, the maximum number of log buffers queued
**	    on a write queue, and lgs_max_wq_count, the number of time
**	    that max was encountered.
**	06-Nov-1996 (jenjo02)
**	    Changed LG_E_xxxxxxxx values to match those in in lgdstatus.h
**	    to avoid a lengthy hold of the lgd_status_mutex while 
**	    transforming from LGD to LG_E_ values in LG_event().
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Added LG_S_ETRANSACTION for getting transaction information
**          for a specific external transaction id
**	13-jun-1997 (wonst02)
** 	    Added DB_READONLY status for readonly databases.
**	01-Jul-1997 (shero03)
**	    Added LG_A_JWITCH to support journal switching
**	15-Oct-1997 (jenjo02)
**	    Added LG_NOT_RESERVED flag for LGwrite().
**	13-Nov-1997 (jenjo02)
**	    Added LG_DELAYBT, LG_A_SBACKUP_STALL, DB_STALL_RW for online backup.
**	03-Dec-1997 (jenjo02)
**	    Added LGrcp_pid() function prototype.
**      08-Dec-1997 (hanch04)
**          Added lgh_last_lsn for Log sequence number.
**          Added la_block field for block number.
**          Keep track of block number and offset into the current block.
**          for support of logs > 2 gig
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Added LG_MAX_FILE define.
**	    Added lgh_partitions to LG_HEADER.
**	02-feb-1998 (hanch04)
**	    Created LG_OLA from LG_LA for upgrade of CNF_V4 and earlier
**	15-Mar-1999 (jenjo02)
**	    Removed LG_A_SBACKUP_STALL, DB_STALL_RW, LG_DELAYBT defines.
**	    Added LG_CKPDB_STALL return status.
**	16-aug-1999 (nanpr01)
**	    Implementation of SIR 92299. Enabaling journalling on a
**	    online checkpoint.
**	30-Sep-1999 (jenjo02)
**	    Added LG_S_OLD_LSN to LGshow() to return first LSN of oldest
**	    active txn.
**	15-Dec-1999 (jenjo02)
**	    Support for SHARED log transactions.
**	    Added DB_DIS_TRAN_ID* to LGbegin() prototype.
**	    Added tr_handle_count, tr_handle_preps,
**	    tr_shared_id to LG_TRANSACTION.
**	    Added LG_SHARED_TXN, LG_CONNECT_TXN return status  
**	    codes from LGbegin, LG_CONTINUE return status
**	    from LGalter.
**	    Added LG_S_STRANSACTION, LG_A_PREPARE.
**	14-Mar-2000 (jenjo02)
**	    Added LG_S_LASTLSN.
**	26-Apr-2000 (thaju02)
**	    Added LG_A_LBACKUP. (B101303)
**      12-Jul-2000 (zhahu02)
**      Increased LG_MAX_RSZ from 66000 to 66100. (B102098)
**	29-Sep-2000 (jenjo02)
**	    Added LG_S_SAMP_STAT LGshow function for the
**	    sampler.
**      15-Nov-2000 (hweho01)
**          Increased LG_MAX_RSZ from 66100 to 66112, make it 
**          in the 8-byte boundary, prevent unaligned access fault   
**          in Compaq 64-bit platform (axp_osf).
**	12-apr-2002 (devjo01)
**	    Remove obsolete constants related to NODES.
**      11-jul-2002 (stial01)
**          Increased LG_MAX_RSZ to 66152... required for DM0L_BTSPLIT records
**          when page size = 64k and key size is 440.
**	17-sep-2003 (abbjo03)
**	    Correct prototype of LGrcp_pid to return PID.
**	17-Dec-2003 (jenjo02)
**	    Added LGconnect() functionality for Partitioned
**	    Table, Parallel Query project.
**      18-feb-2004 (stial01)
**          Increased LG_MAX_RSZ for 256k row support. (Now that the i2 limit
**          row size limit has been lifted, the maximum row size for 64k pages
**          has been increased - so DM0L_REP log records can be bigger.)
**	08-Apr-2004 (jenjo02)
**	    Added LG_A_BMINI, LG_A_EMINI, TR_IN_MINI
**	20-Apr-2004 (jenjo02)
**	    Added LG_LOGFULL_COMMIT flag for LGend().
**      20-sep-2004 (devjo01)
**          Add LG_LOCAL_LSN for LGwrite().  Remove depreciated comments
**          re VAX clusters.
**      05-may-2005 (stial01)
**          Added LG_CSP_LDB
**      13-jun-2005 (stial01)
**          Renamed LG_CSP_LDB to LG_CSP_RECOVER
**	15-Mar-2006 (jenjo02)
**	    Added LG_S_FORCED_LGA, LG_S_FORCED_LSN to LGshow.
**	    Moved transaction wait reason defines here from lgdef.h
**	21-Mar-2006 (jenjo02)
**	    Add LG_A_OPTIMWRITE, LG_A_OPTIMWRITE_TRACE
**      31-mar-2006 (stial01)
**          Renamed LG_E_JSWITCHDONE to LG_E_ARCHIVE_DONE. The archiver always 
**          sends LG_E_ARCHIVE_DONE, and JSP alterdb -next_jnl_file verifies
**          if the archiver did the switch or not. Removed LG_A_DJSWITCH.
**	21-Jun-2006 (jonj)
**	    Add LG_A_LOCKID, LG_A_TXN_STATE, LG_DUPLICATE_TXN,
**	    LG_NO_TRANSACTION, LG_XA_START_XA, LG_XA_START_JOIN
**	    for XA integration.
**	28-Aug-2006 (jonj)
**	    Add LG_A_FILE_DELETE, LG_FDEL structure to deal with
**	    XA transaction branch file deletes.
**	05-Sep-2006 (jonj)
**	    Add LG_ILOG (internal logging) flag for LGwrite(),
**	    TR_ONEPHASE, TR_ILOG status bits for LG_TRANSACTION.
**	11-Sep-2006 (jonj)
**	    Add LG_WAIT_COMMIT for LOGFULL COMMIT.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**      20-Jun-2007 (hanal04) Bug 118531
**          Explicitly define LGbegin's flag values as i4's. On i64.hpu
**          the define's were not automatically compiled as i4's and this
**          lead to spurious behaviour in LGbegin().
**      11-May-2007 (stial01)
**          Added LG_S_COMMIT_WILLING, LG_S_ROLLBACK_WILLING
**      28-jan-2008 (stial01)
**          Redo/undo error handling during offline recovery via rcpconfig.
**      31-jan-2008 (stial01)
**          Added -rcp_stop_lsn, to set II_DMFRCP_STOP_ON_INCONS_DB
**      02-may-2008 (horda03) Bug 120349
**          Added LG_WAIT_REASON define.
**      30-Oct-2009 (horda03) Bug 122823
**          Added LG_PARTITION_FILENAMES to signal to LGopen() that the
**          filename paramter is a pointer to an array of LG_PARTIONs.
**          Added LG_PARTITON structure to hold details on the filenames
**          of a partitioned TX log file. When LG_PARTITION_FILENAMES flag
**          set, l_filename refers to the number of partitons.
**	10-Nov-2009 (kschendel) SIR 122757
**	    Remove l_context param from LGread.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Remove lx_id param from LGerase.  Delete LGidnum_from_id.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC:
**	    Define new structure LG_LRI as output param to LGwrite() instead
**	    of LG_LSN. LGwrite populates it with everything about the write,
**	    lg_id, LSN, LGA, journal sequence and offset.
**	    Define new structure LG_CRIB to be populated by LGshow(LG_S_CRIB)
**	    to establish point-in-time values for consistent read.
**	    Add LGshow(LG_S_XID_ARRAY_SIZE) to return size of active xid array.
**	    LG_S_XID_ARRAY_PTR to return a pointer to it.
**	    Formalize JFA (Journal File Address) as a LG_JFA structure.
**	04-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add bufid to LGread() prototype.
**	08-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add LG_LGID_INTERNAL structure for use by
**	    LOG_ID_ID macro to extract just the id_id, avoiding endian issues.
**	17-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Add LG_A_RBBLOCKS, LG_A_RFBLOCKS, LG_S_RBBLOCKS,
**	    LG_S_RFBLOCKS for buffered log reads.
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _LG_LA		LG_LA;
typedef struct _LG_OLA		LG_OLA;
typedef struct _LG_LSN		LG_LSN;
typedef struct _LG_RECORD	LG_RECORD;
typedef struct _LG_HEADER	LG_HEADER;
typedef struct _LG_STAT		LG_STAT;
typedef struct _LG_DATABASE	LG_DATABASE;
typedef struct _LG_PROCESS	LG_PROCESS;
typedef struct _LG_PURGE	LG_PURGE;
typedef struct _LG_TRANSACTION	LG_TRANSACTION;
typedef struct _LG_OBJECT	LG_OBJECT;
typedef struct _LG_CONTEXT	LG_CONTEXT;
typedef i4			LG_LGID;
typedef i4			LG_DBID;
typedef i4			LG_LXID;
typedef struct _LG_BUFFER	LG_BUFFER;
typedef struct _LG_MUTEX	LG_MUTEX;
typedef struct _LG_FDEL		LG_FDEL;
typedef struct _LG_LRI		LG_LRI;
typedef struct _LG_CRIB		LG_CRIB;
typedef struct _LG_JFIB		LG_JFIB;
typedef struct _LG_JFA		LG_JFA;

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN STATUS LGinitialize( CL_ERR_DESC *sys_err, char *lgk_info );
FUNC_EXTERN STATUS LGinit_lock( CL_ERR_DESC *sys_err, char *lgk_info );
FUNC_EXTERN STATUS LGadd( LG_LGID lg_id, i4 flag, char *buffer,
			    i4  l_buffer, LG_DBID *db_id, CL_SYS_ERR *sys_err);
FUNC_EXTERN STATUS LGalter( i4	flag, PTR buffer, i4  l_buffer,
			    CL_SYS_ERR *sys_err );
FUNC_EXTERN STATUS LGbegin( i4 flag, LG_DBID db_id, DB_TRAN_ID *tran_id,
			    LG_LXID *lx_id, i4 l_user_name, char *user_name,
			    DB_DIS_TRAN_ID *dis_tran_id,
			    CL_SYS_ERR *sys_err);
FUNC_EXTERN STATUS LGconnect( LG_LXID clx_id, 
			      LG_LXID *hlx_id,
			      CL_SYS_ERR *sys_err);
FUNC_EXTERN STATUS  LGclose( LG_LGID lg_id, CL_SYS_ERR *sys_err);
FUNC_EXTERN STATUS  LGcopy( i4  from_log, char * nodename, 
			    CL_SYS_ERR *sys_err );
FUNC_EXTERN STATUS  LGend( LG_LXID lx_id, i4 flag, CL_SYS_ERR *sys_err );
FUNC_EXTERN STATUS LGerase( LG_LGID *lg_id, i4 bcnt, 
			    i4 bsize,
			    bool force_init, u_i4 logs_to_initialize,
			    CL_SYS_ERR *sys_err );
FUNC_EXTERN STATUS  LGevent( i4  flag, LG_LXID lx_id, i4  event_mask,
			    i4  *events, CL_SYS_ERR *sys_err );
FUNC_EXTERN STATUS  LGforce( i4  flag, LG_LXID lx_id, LG_LSN *lga,
			    LG_LSN *nlga, CL_SYS_ERR *sys_err );
FUNC_EXTERN STATUS  LGopen( u_i4 flag, char *filename, u_i4 l_filename,
			    LG_LGID *lg_id, i4 *blks,
			    i4  block_size, CL_SYS_ERR *sys_err);
FUNC_EXTERN STATUS  LGposition( LG_LXID lx_id, i4  position, i4  direction,
			    LG_LA *lga, PTR context, i4 l_context,
			    CL_SYS_ERR *sys_err );
FUNC_EXTERN STATUS  LGread( LG_LXID lx_id, PTR context,
			    LG_RECORD **record, LG_LA *lga,
			    i4	*bufid,
			    CL_SYS_ERR *sys_err );
FUNC_EXTERN STATUS  LGremove( LG_DBID db_id, CL_SYS_ERR *sys_err);
FUNC_EXTERN STATUS  LGreserve(i4 flag, LG_LXID lx_id, i4  num_records,
			    i4 res_space, CL_ERR_DESC *sys_err);
FUNC_EXTERN STATUS  LGshow( i4  flag, PTR buffer, i4  l_buffer,
			    i4  *length, CL_SYS_ERR *sys_err);
FUNC_EXTERN STATUS  LGwrite( i4  flag, LG_LXID lx_id, i4  num_objects,
			    LG_OBJECT *objects, LG_LRI *lri,
			    CL_SYS_ERR *sys_err );
FUNC_EXTERN i4 LGcluster( VOID );
FUNC_EXTERN i4	    LGcnode_info( char *name, i4  l_name, CL_SYS_ERR *sys_err);
FUNC_EXTERN VOID    LGCn_name(char *node_name);
FUNC_EXTERN STATUS  LGCnode_names(i4 node_mask, char *node_names,
			i4 length, CL_SYS_ERR *sys_err);
FUNC_EXTERN i4 LG_cluster_node(char * node_name);
FUNC_EXTERN PID LGrcp_pid( void );

/*
**  Defines of other constants.
*/

/* Reasons a transaction had to wait: */

#define			LG_WAIT_ALL	    0
#define			LG_WAIT_FORCE	    1
#define			LG_WAIT_SPLIT	    2
#define			LG_WAIT_HDRIO	    3
#define			LG_WAIT_CKPDB	    4
#define			LG_WAIT_OPENDB	    5
#define			LG_WAIT_BCP    	    6
#define			LG_WAIT_LOGFULL     7
#define			LG_WAIT_FREEBUF     8
#define			LG_WAIT_LASTBUF     9
#define			LG_WAIT_FORCED_IO   10
#define			LG_WAIT_EVENT	    11
#define			LG_WAIT_WRITEIO	    12
#define			LG_WAIT_MINI        13
#define			LG_WAIT_COMMIT      14

#define LG_WAIT_REASON "<not waiting>,FORCE,SPLIT,HDRIO,CKPDB,OPENDB,BCPSTALL,\
LOGFULL,FREEBUF,LASTBUF,FORCED_IO,EVENT,WRITEIO,MINI,COMMIT"

#define			LG_WAIT_REASON_MAX    LG_WAIT_COMMIT

/*
**	Special constant related to maximum record size.
**
**      WARNING!!! 
**      LG_MAX_RSZ must be on 8 byte boundary for Compaq 64-bit (axp_osf)
**
**      LG_MAX_RSZ is NOT arbitrary, it must be large enough for the maximum
**      log record to be written by the server PLUS size of LG_RECORD
*/

#define			LG_MAX_RSZ	    131080

#define			LG_MAX_CTX	    256

/*
**	Maximum number of parts a log record can be broken into for a call
**	to LGwrite.
*/

#define			LG_MAX_OBJECTS	    16

/*	Maximum number of log file partitions in an installation */
#define			LG_MAX_FILE	    16

/*
**      Maximum database info size.  This defines the maximum size database
**      info that LGadd will accept.  The size is based on the maximum legal
**      path/filename/owner combination.  These are determined by DB_MAXNAME
**      and DB_AREA_MAX, but since those cannot be referenced by the CL, we
**      must define them here.
**
**      This constant allows room for the following information in the
**      add database info block.
**
**          Database name       DB_MAXNAME
**          Database Owner      DB_MAXNAME
**          Database Path       DB_AREA_MAX
**          Length of pathname
**          Database ID
*/
#define                 LG_DBINFO_MAX       (DB_MAXNAME+DB_MAXNAME+DB_AREA_MAX+24)

/*
**	Return status constants.
*/

#define	    LG_CANTINIT		(E_CL_MASK + E_LG_MASK + 0x01)
#define	    LG_CANTOPEN		(E_CL_MASK + E_LG_MASK + 0x02)
#define	    LG_CANTCLOSE	(E_CL_MASK + E_LG_MASK + 0x03)
#define	    LG_BADPARAM		(E_CL_MASK + E_LG_MASK + 0x04)
#define	    LG_READERROR	(E_CL_MASK + E_LG_MASK + 0x05)
#define	    LG_BADHEADER	(E_CL_MASK + E_LG_MASK + 0x06)
#define	    LG_INITHEADER	(E_CL_MASK + E_LG_MASK + 0x07)
#define	    LG_ENDOFILE		(E_CL_MASK + E_LG_MASK + 0x08)
#define	    LG_OFFLINE		(E_CL_MASK + E_LG_MASK + 0x09)
#define	    LG_BADFORMAT	(E_CL_MASK + E_LG_MASK + 0x0a)
#define	    LG_NOTLOADED	(E_CL_MASK + E_LG_MASK + 0x0b)
#define	    LG_UNEXPECTED	(E_CL_MASK + E_LG_MASK + 0x0c)
#define	    LG_CHK_SUM_ERROR	(E_CL_MASK + E_LG_MASK + 0x0d)
#define	    LG_DB_INCONSISTENT	(E_CL_MASK + E_LG_MASK + 0x0f)
#define	    LG_WRITEERROR	(E_CL_MASK + E_LG_MASK + 0x10)
#define	    LG_NOCLUSTERINFO	(E_CL_MASK + E_LG_MASK + 0x11)
#define	    LG_CEXCEPTION	(E_CL_MASK + E_LG_MASK + 0x12)
#define	    LG_INTERRUPT	(E_CL_MASK + E_LG_MASK + 0x13)
#define	    LG_EXCEED_LIMIT	(E_CL_MASK + E_LG_MASK + 0x14)
#define     LG_DUAL_OPEN_FAIL   (E_CL_MASK + E_LG_MASK + 0x15)
#define	    LG_SYNCHRONOUS	(E_CL_MASK + E_LG_MASK + 0x20)
#define	    LG_BADMUTEX		(E_CL_MASK + E_LG_MASK + 0x21)
#define	    LG_DUALLOG_MISMATCH	(E_CL_MASK + E_LG_MASK + 0x22)
#define	    LG_LOGSIZE_MISMATCH (E_CL_MASK + E_LG_MASK + 0x23)
#define	    LG_DISABLE_PRIM	(E_CL_MASK + E_LG_MASK + 0x24)
#define	    LG_DISABLE_DUAL	(E_CL_MASK + E_LG_MASK + 0x25)
#define	    LG_RECONSTRUCT	(E_CL_MASK + E_LG_MASK + 0x26)
#define	    LG_ERROR_INITIATING_IO (E_CL_MASK + E_LG_MASK + 0x27)
#define	    LG_WRONG_VERSION	(E_CL_MASK + E_LG_MASK + 0x28)
#define	    LG_NO_BUFFER	(E_CL_MASK + E_LG_MASK + 0x29L)
#define	    LG_CKPDB_STALL	(E_CL_MASK + E_LG_MASK + 0x2aL)
#define	    LG_SHARED_TXN	(E_CL_MASK + E_LG_MASK + 0x2bL)
#define	    LG_CONNECT_TXN	(E_CL_MASK + E_LG_MASK + 0x2cL)
#define	    LG_CONTINUE		(E_CL_MASK + E_LG_MASK + 0x2dL)
#define	    LG_DUPLICATE_TXN	(E_CL_MASK + E_LG_MASK + 0x2eL)
#define	    LG_NO_TRANSACTION	(E_CL_MASK + E_LG_MASK + 0x2fL)
/* internal errors returned through CL_ERR_DESC */

#define	    E_CL0F30_LG_NO_MASTER		(E_CL_MASK + E_LG_MASK + 0x30)
#define	    E_CL0F31_LG_CANT_OPEN		(E_CL_MASK + E_LG_MASK + 0x31)
#define     E_CL0F32_LG_SEM_OWNER_DEAD          (E_CL_MASK + E_LG_MASK + 0x32)

/*
**	Flag values for LGevent.
*/

#define		    LG_NOWAIT		0x01
#define		    LG_WAIT_ABORT	0x02
#define		    LG_INTRPTABLE	0x04

/*
**	Flag values for LGerase(logs_to_initialize)
*/
#define		    LG_ERASE_PRIMARY_ONLY   1
#define		    LG_ERASE_DUAL_ONLY	    2
#define		    LG_ERASE_BOTH_LOGS	    3

/*
**	Event values for LGevent.
**
**	   These must match the values found in lgdstatus.h
*/

#define		    LG_E_ONLINE			0x00000001
#define		    LG_E_BCP			0x00000002
#define		    LG_E_LOGFULL		0x00000004
/*	LGD_FORCE_ABORT				0x00000008 */
#define		    LG_E_RECOVER		0x00000010
#define		    LG_E_ARCHIVE		0x00000020
#define		    LG_E_ACP_SHUTDOWN		0x00000040
#define		    LG_E_IMM_SHUTDOWN		0x00000080
#define             LG_E_START_ARCHIVER 	0x00000100
#define		    LG_E_PURGEDB		0x00000200
#define		    LG_E_OPENDB			0x00000400
#define		    LG_E_CLOSEDB		0x00000800
/*	LGD_START_SHUTDOWN			0x00001000 */
/*	LGD_BCPDONE				0x00002000 */
#define		    LG_E_CPFLUSH		0x00004000
#define		    LG_E_ECP			0x00008000
#define		    LG_E_ECPDONE    		0x00010000
#define		    LG_E_CPWAKEUP		0x00020000
/*	LGD_BCPSTALL				0x00040000 */
/*	LGD_CKP_SBACKUP				0x00080000 */

/* The following 2 values are derived from ldb_status by LGevent(): */
#define		    LG_E_SBACKUP		0x00100000
#define             LG_E_BACKUP_FAILURE 	0x00200000

#define		    LG_E_M_ABORT		0x00400000
#define		    LG_E_M_COMMIT		0x00800000
#define		    LG_E_RCPRECOVER		0x01000000
/*	LGD_JSWITCH				0x02000000 */
#define		    LG_E_ARCHIVE_DONE		0x04000000
/*	LGD_DUAL_LOGGING			0x08000000 */
#define             LG_E_DISABLE_DUAL_LOGGING   0x10000000
#define		    LG_E_EMER_SHUTDOWN		0x20000000
/*	LGD_CLUSTER_SYSTEM			0x80000000 */

#define		    LG_E_FCSHUTDOWN		0x00000000 /* not used */

/*
**	Flags values for LGopen.
*/

#define		    LG_SLAVE		0x0000
#define		    LG_MASTER		0x0001
#define		    LG_ARCHIVER		0x0002
#define		    LG_LOG_INIT		0x0010
#define		    LG_CKPDB		0x0020
/* #define	    LG_FCT		0x0040 defined later for LGbegin */
#define		    LG_DUAL_LOG		0x0080
#define		    LG_HEADER_ONLY	0x0100
#define		    LG_ONELOG_ONLY	0x0200
			/* LG_ONELOG_ONLY is passed to verify_header to
			** distinguish a standalone open of a single logfile
			** by the standalone logdump program
			*/
#define		    LG_FULL_FILENAME	0x0400
			/* LG_FULL_FILENAME is used by standalone logdump to
			** pass an explicit fully-qualified filename to LGopen
			*/
#define		    LG_PRIMARY_ERASE	0x0800
#define		    LG_DUAL_ERASE	0x1000
			/* Only used in conjunction with LG_LOG_INIT, these 
			** flags indicate that only the specified logfile 
			** is to be opened.
			*/
#define		    LG_IGNORE_BOF	0x2000
			/* Used by standalone logdump to direct LGopen to
			** ignore the log header BOF value and to always scan
			** the file to find the log sequence number transition
			** point to use as the BOF.  This allows logdump to
			** scan the entire log file rather than just those
			** records situated between the logical bof and eof.
			*/
#define		    LG_PARTITION_FILENAMES  0x4000
			/* Used by standalone logdump to instruct LGopen that
			** the filename parameter is really an array of partitions
			** and l_filename refers to the number of partitions.
			*/

/*
**	Flags values for LGcopy. LG_PRIMARY_LOG is an obsolete synonym for
**	LG_COPY_PRIM_TO_DUAL.
*/

#define		    LG_PRIMARY_LOG	    0x01L
#define		    LG_COPY_PRIM_TO_DUAL    0x01L
#define		    LG_COPY_DUAL_TO_PRIM    0x02L

/*
** LGshow(LG_S_DUAL_LOGGING) returns a mask of the log files which are active:
*/
#define		    LG_PRIMARY_ACTIVE	    0x01L
#define		    LG_DUAL_ACTIVE	    0x02L
#define		    LG_BOTH_LOGS_ACTIVE	    (LG_PRIMARY_ACTIVE|LG_DUAL_ACTIVE)


/*
**	Position values for LGposition
*/

#define		    LG_P_FIRST		    1
#define		    LG_P_LAST		    2
#define		    LG_P_LGA		    3
#define		    LG_P_PAGE		    4
#define		    LG_P_TRANS		    5
#define		    LG_P_MAX		    5

/*
**	Direction values for LGposition
*/

#define		    LG_D_BACKWARD	    1
#define		    LG_D_FORWARD	    2
#define		    LG_D_PREVIOUS	    3
#define		    LG_D_MAX		    3

/*
**	Read-mode argument values for LGposition
*/
#define		    LG_R_READ		    1
#define		    LG_R_COMPARE	    2
#define		    LG_R_MAX		    2

/*
**	Flag values for LGreserve.
*/

#define		    LG_RS_FORCE		    0x01

/*
**	Flag values for LGwrite.
*/

#define		    LG_FORCE		    0x01
/* #define	    LG_LAST		    0x02  - defined below */
#define		    LG_FIRST		    0x04
#define		    LG_NOT_RESERVED	    0x08
/* #define	    LG_LOGFULL_COMMIT	    0x10  - defined below */
#define             LG_LOCAL_LSN            0x20 
#define             LG_ILOG                 0x40  /* Internal logging */
#define             LG_2ND_BT         	    0x80  /* 2ND BT of txn */

/*
**	Flag values for LGforce.
*/

#define		    LG_HDR		    0x01
#define		    LG_LAST		    0x02

/*
**	Flag values for LGbegin.
*/

#define		    LG_NOPROTECT	    0x0001L
#define		    LG_JOURNAL		    0x0002L
#define		    LG_NOABORT		    0x0004L
#define		    LG_READONLY		    0x0008L
#define		    LG_MVCC		    0x0010L
#define		    LG_NOTDB		    0x0020L
#define		    LG_FCT		    0x0040L
#define		    LG_PRETEND_CONSISTENT   0x0080L
#define		    LG_LOGWRITER_THREAD	    0x0200L
#define		    LG_NORESERVE	    0x0400L
#define		    LG_CONNECT	    	    0x0800L
#define		    LG_CSP_RECOVER	    0x1000L
#define		    LG_XA_START_XA	    0x2000L
#define		    LG_XA_START_JOIN	    0x4000L

/*
**	Flag values for LGend.
*/

#define		    LG_SESSION_ABORT	    0x01
#define		    LG_RCP		    0x02
#define		    LG_START		    0x04
#define		    LG_LOGFULL_COMMIT	    0x10

/*
**      Constants that describe item codes for LGshow and LGalter.
*/

#define                 LG_S_STAT	   	 1
#define			LG_S_TRANSACTION   	 2
#define			LG_S_DATABASE	   	 3
#define			LG_S_PROCESS	   	 4
#define			LG_S_ADATABASE	   	 5
#define			LG_N_DATABASE	   	 6
#define			LG_N_PROCESS	   	 7
#define			LG_N_TRANSACTION   	 8
#define			LG_S_APROCESS	   	 9
#define			LG_A_HEADER		10
#define			LG_A_BCNT		11
#define			LG_A_BLKS		12
#define			LG_A_LDBS		13
#define			LG_A_EOF		14
#define			LG_A_BOF		15
#define			LG_A_CPA		16
#define			LG_A_ONLINE		17
#define			LG_A_BCPDONE		18
#define			LG_A_RECOVERDONE	19
#define			LG_A_SHUTDOWN		20
#define			LG_A_DB			21
#define			LG_S_ATRANSACTION   	22
#define			LG_S_MUTEX   		23
#define                 LG_S_ETRANSACTION       24
#define                 LG_S_STRANSACTION       25
#define			LG_A_NODEID	    	26
#define			LG_S_ACP_START	    	27
#define			LG_S_ACP_END	    	28
#define			LG_S_LGSTS	    	29
#define			LG_S_OPENDB	    	30
#define			LG_S_CLOSEDB	    	31
#define			LG_A_OPENDB	    	32
#define			LG_A_CLOSEDB	    	33
#define			LG_A_SERVER_ABORT   	34
#define			LG_A_OFF_SERVER_ABORT   35
#define                 LG_A_PASS_ABORT     	36
#define			LG_A_STAMP	    	37
#define			LG_A_FORCE_ABORT    	38
#define			LG_S_FORCE_ABORT    	39
#define			LG_A_ECPDONE	    	40
#define			LG_A_CPFDONE	    	41
#define			LG_A_CPWAITDONE	    	42
#define			LG_A_DBRECOVER	    	43
#define			LG_A_ARCHIVE	   	44
#define			LG_A_ACPOFFLINE	    	45
#define			LG_A_SBACKUP	    	46	/* ckpdb */
#define			LG_A_RESUME	    	48	/* ckpdb */
#define			LG_S_BACKUP	    	49	/* acp */
#define			LG_A_FBACKUP	    	51	/* acp */
#define			LG_A_DBACKUP	    	52	/* ckpdb */
#define			LG_A_WILLING_COMMIT 	55
#define			LG_A_CONTEXT	    	56
#define			LG_S_DIS_TRAN	    	57
#define			LG_A_REASSOC	    	58
#define			LG_A_ABORT	    	59
#define			LG_A_COMMIT	    	60
#define			LG_A_OWNER	    	61
#define			LG_A_M_ABORT	    	62
#define			LG_A_M_COMMIT	    	63
#define			LG_S_MAN_COMMIT	    	64
#define			LG_A_BUFMGR_ID	    	65
#define			LG_A_IDLE_SERVER    	66
#define			LG_A_NOTIDLE_SERVER 	67
#define			LG_S_DBID	    	68
#define			LG_A_DMU	    	69
#define			LG_A_RCPRECOVER	    	70
#define			LG_S_ACP_CP	    	71
#define			LG_A_CPNEEDED	    	73
#define			LG_A_CKPERROR	    	74
#define			LG_A_TXN_STATE		76
#define			LG_S_STAMP		77
#define			LG_A_LOCKID		78
#define                 LG_A_DISABLE_DUAL_LOGGING   79
#define                 LG_A_ENABLE_DUAL_LOGGING    80
#define                 LG_S_DUAL_LOGGING       81
#define                 LG_A_DUAL_LOGGING       82
#define			LG_A_TP_ON		83
#define			LG_A_TP_OFF		84
#define			LG_S_TP_VAL		85
#define			LG_S_TEST_BADBLK	86
#define			LG_A_SERIAL_IO		87
#define			LG_A_FABRT_SID		88
#define			LG_A_GCMT_SID		89
#define                 LG_A_CPAGENT            90
#define			LG_A_GCMT_THRESHOLD	91
#define			LG_A_GCMT_NUMTICKS	92
#define			LG_S_FBUFFER		93
#define			LG_S_WBUFFER		94
#define			LG_S_CBUFFER		95
#define			LG_A_RES_SPACE          96
#define			LG_A_LXB_OWNER	    	97
#define			LG_A_DBCONTEXT	    	98
#define			LG_A_ARCHIVE_DONE   	99
#define			LG_A_NODELOG_HEADER	100
#define			LG_S_CSP_PID		101
#define			LG_A_CACHE_FACTOR	102
#define			LG_A_UNRESERVE_SPACE	103
#define			LG_A_LFLSN		104
#define			LG_A_SJSWITCH		105
#define			LG_S_OLD_LSN		107
#define			LG_A_PREPARE		108
#define			LG_S_LASTLSN		109

#define			LG_A_LBACKUP		110
#define			LG_S_SAMP_STAT		111
#define			LG_A_BMINI		112
#define			LG_A_EMINI		113

#define			LG_S_FORCED_LGA		114
#define			LG_S_FORCED_LSN		115
#define			LG_A_OPTIMWRITE		116
#define			LG_A_OPTIMWRITE_TRACE	117
#define			LG_S_XA_PREPARE		118
#define			LG_S_XA_COMMIT		119
#define			LG_S_XA_COMMIT_1PC	120
#define			LG_S_XA_ROLLBACK	121
#define			LG_A_XA_DISASSOC	122
#define			LG_A_FILE_DELETE	123
#define			LG_S_COMMIT_WILLING	124
#define			LG_S_ROLLBACK_WILLING	125
#define			LG_A_RCP_IGNORE_DB	126
#define			LG_A_RCP_IGNORE_TABLE   127
#define			LG_A_RCP_IGNORE_LSN     128
#define			LG_A_INIT_RECOVER	129
#define			LG_A_RCP_STOP_LSN	130
#define			LG_S_SHOW_RECOVER	131

/* Added for MVCC: */
#define			LG_S_CRIB_BT		132
#define			LG_S_CRIB_BS		133
#define			LG_S_CRIB_BS_LSN	134
#define			LG_S_XID_ARRAY_PTR	135
#define			LG_S_XID_ARRAY_SIZE	136
#define			LG_A_JFIB		137
#define			LG_S_JFIB		LG_A_JFIB
#define			LG_A_JFIB_DB		138
#define			LG_S_JFIB_DB		LG_A_JFIB_DB
#define			LG_A_ARCHIVE_JFIB	139
#define			LG_A_RBBLOCKS		140
#define			LG_S_RBBLOCKS		LG_A_RBBLOCKS
#define			LG_A_RFBLOCKS		141
#define			LG_S_RFBLOCKS		LG_A_RFBLOCKS


/*}
** Name: LG_DBID_MATCH_MACRO - Compare database id's for matching database.
**
** Description:
**      This macro compares two database id's to check if they are
**	actually instances of the same database id.
**
**	Internally, the logging system reuses database id's by incrementing
**	an instance_id counter that is contained in the upper 2 bytes of
**	the database id.
**
** History:
**     12-feb-1990 (rogerk)
**          Created.
**	19-nov-1992 (bryanp)
**	    The "portable" logging system only has ID's one way; no more
**	    ifdef BYTE_SWAP needed...
*/
typedef struct
{
    u_i2	    id_instance;
    u_i2	    id_id;
} LG_DBID_INTERNAL;

#define	    LG_DBID_MATCH_MACRO(a,b)	(((LG_DBID_INTERNAL *)&a)->id_id == \
					 ((LG_DBID_INTERNAL *)&b)->id_id)

/*}
** Name: LG_OLA - Log system log address.
**
** Description:
**      This structure describes the format of a log system record
**	address.
**
** History:
**	02-feb-1998 (hanch04)
**	    Created from LG_LA for upgrade of CNF_V4 and earlier
*/
struct _LG_OLA
{
    u_i4	    la_sequence;	    /* Most significant part of LA. */
    u_i4	    la_offset;		    /* Byte offset in current block. */
};
/*}
** Name: LG_LA - Log system log address.
**
** Description:
**      This structure describes the format of a log system record
**	address.
**
** History:
**     23-jun-1986 (Derek)
**          Created.
**	15-mar-1993 (bryanp)
**	    6.5 Recovery Project:
**	    - Renamed the components to la_sequence and la_offset. All 
**	      references to the DM_LOG_ADDR should now be replaceable by 
**	      references to LG_LA.
**	    - Moved LG_LA macros to lg.h from lgdef.h
**	    - changed the macros to enforce type correctness.
*/
struct _LG_LA
{
    u_i4	    la_sequence;	    /* Most significant part of LA. */
    u_i4       la_block;               /* Block offset into log */
    u_i4	    la_offset;		    /* Byte offset in current block. */
};
/*
** Macro definitions used to compare log addresses. Please note that we do
** NOT cast the caller's arguments to (LG_LA *) pointers. This means that the
** caller is REQUIRED to pass the correct types to these macros, or the caller
** will fail to compile. Making this requirement helps ensure that DMF code
** keeps log addresses and log sequence numbers straight.
*/
#define LGA_EQ(lga1, lga2)\
  (((lga1)->la_sequence == (lga2)->la_sequence) &&\
   ((lga1)->la_block == (lga2)->la_block) &&\
   ((lga1)->la_offset == (lga2)->la_offset))

#define LGA_GTE(lga1, lga2)\
    (((lga1)->la_sequence > (lga2)->la_sequence) ||\
	(((lga1)->la_sequence == (lga2)->la_sequence) &&\
	 ((lga1)->la_block > (lga2)->la_block)) ||\
	    (((lga1)->la_sequence == (lga2)->la_sequence) &&\
	     ((lga1)->la_block == (lga2)->la_block) &&\
	     ((lga1)->la_offset >= (lga2)->la_offset)))

#define LGA_LTE(lga1, lga2)\
    (((lga1)->la_sequence < (lga2)->la_sequence) ||\
	(((lga1)->la_sequence == (lga2)->la_sequence) &&\
	 ((lga1)->la_block < (lga2)->la_block )) ||\
	    (((lga1)->la_sequence == (lga2)->la_sequence) &&\
	     ((lga1)->la_block == (lga2)->la_block) &&\
	     ((lga1)->la_offset <= (lga2)->la_offset)))

#define LGA_GT(lga1, lga2)\
    (((lga1)->la_sequence > (lga2)->la_sequence) ||\
	(((lga1)->la_sequence == (lga2)->la_sequence) &&\
	 ((lga1)->la_block > (lga2)->la_block)) ||\
	    (((lga1)->la_sequence == (lga2)->la_sequence) &&\
	     ((lga1)->la_block == (lga2)->la_block) &&\
	     ((lga1)->la_offset > (lga2)->la_offset)))

#define LGA_LT(lga1, lga2)\
    (((lga1)->la_sequence < (lga2)->la_sequence) ||\
	(((lga1)->la_sequence == (lga2)->la_sequence) &&\
	 ((lga1)->la_block < (lga2)->la_block)) ||\
	    (((lga1)->la_sequence == (lga2)->la_sequence) &&\
	     ((lga1)->la_block == (lga2)->la_block) &&\
	     ((lga1)->la_offset < (lga2)->la_offset)))


/*}
** Name: LG_LSN - Log system log sequence number.
**
** Description:
**	This structure describes the format of a logging system 
**	"log sequence number".
**
** History:
**     07-aug-1992 (jnash)
**          Created as part of reduced logging project.
*/
struct _LG_LSN
{
    u_i4	    lsn_high;		    /* Most significant part */
    u_i4	    lsn_low;		    /* Least significant part */
};
/*
** Macro definitions used to compare log record LSN's. Please note that we do
** NOT cast the caller's arguments to (LG_LSN *) pointers. This means that the
** caller is REQUIRED to pass the correct types to these macros, or the caller
** will fail to compile. Making this requirement helps ensure that DMF code
** keeps log addresses and log sequence numbers straight.
*/
#define LSN_EQ(lsn1, lsn2)\
  (((lsn1)->lsn_high == (lsn2)->lsn_high) &&\
   ((lsn1)->lsn_low == (lsn2)->lsn_low))

#define LSN_GTE(lsn1, lsn2)\
  (((lsn1)->lsn_high > (lsn2)->lsn_high) ||\
   (((lsn1)->lsn_high == (lsn2)->lsn_high) &&\
	((lsn1)->lsn_low >= (lsn2)->lsn_low)))

#define LSN_LTE(lsn1, lsn2)\
  (((lsn1)->lsn_high < (lsn2)->lsn_high) ||\
   (((lsn1)->lsn_high == (lsn2)->lsn_high) &&\
	((lsn1)->lsn_low <= (lsn2)->lsn_low)))

#define LSN_GT(lsn1, lsn2)\
  (((lsn1)->lsn_high > (lsn2)->lsn_high) ||\
   (((lsn1)->lsn_high == (lsn2)->lsn_high) &&\
	((lsn1)->lsn_low > (lsn2)->lsn_low)))

#define LSN_LT(lsn1, lsn2)\
  (((lsn1)->lsn_high < (lsn2)->lsn_high) ||\
   (((lsn1)->lsn_high == (lsn2)->lsn_high) &&\
	((lsn1)->lsn_low < (lsn2)->lsn_low)))


/*}
** Name: LG_JFA - Log system Journal File Address.
**
** Description:
**	This structure describes the format of a logging system 
**	journal file address, the physical location of a log
**	record in a journal file.
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Created for Consistent Read.
*/
struct _LG_JFA
{
    u_i4	    jfa_filseq;		    /* Journal file sequence */
    i4		    jfa_block;		    /* Block within that file */
};

/*}
** Name: LG_LRI - Log Record Information
**
** Description:
**	This structure is passed as an output parameter to LGwrite
**	and populated with information about the log record just
**	written.
**
**	It replaces the singular LGwrite LSN *lsn output parameter.
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Created for Consistent Read undo.
*/
struct _LG_LRI
{
    u_i2	    lri_lg_id;		/* Log id_id that wrote */
    LG_LSN	    lri_lsn;		/* LSN of the log record */
    LG_LA	    lri_lga;		/* LGA of the log record */
    LG_JFA	    lri_jfa;		/* JFA of the log record */
    i4	    	    lri_bufid;		/* Log buffer containing lri_lga */
};

/* Macro to return only the "id_id" portion of a LG_LGID */
typedef struct
{
    u_i2	    id_instance;
    u_i2	    id_id;
} LG_LGID_INTERNAL;

#define LOG_ID_ID(log_id) \
	(u_i2)(((LG_LGID_INTERNAL*)&log_id)->id_id)

/*}
** Name: LG_CRIB - Consistent Read Information Block
**
** Description:
**
**	This structure is passed to LGshow(LG_S_CRIB_?) to snapshot 
**	the point-in-time values of a database for consistent 
**	read protocols.
**
**	crib_next		next cursor CRIB
**	crib_prev		prev cursor CRIB
**	crib_cursid		Cursor identifier from QEF
**	crib_inuse		Number of cursor RCBs opened on CRIB
**	crib_log_id		is the caller's log handle.
**	crib_low_lsn		initialized to the first LSN of the oldest 
**				active transaction in the DB; if 
**				there is no active transaction, this
**				will be the same as crib_last_commit.
**	crib_last_commit	initialized to the LSN of the last COMMIT 
**				record for the DB.
**	crib_bos_lsn		the transaction's last-written LSN
**				at the time a statement began.
**	crib_bos_tranid		transaction id basis for CR. This may be
**				different than the actual transaction id
**				when isolation level is read-committed.
**	crib_sequence		The QEF statement number, maintained by
**				dmt_open, not LG.
**	crib_xid_array		a copy of the open transaction ldb_xid_array.
**				The memory is supplied by the caller and
**				must be of size lgd_xid_array_size.
**	crib_lgid_low		the lowest open lg_id in xid_array
**	crib_lgid_high		the highest open lg_id in xid_array
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Created for Consistent Read
**	26-Feb-2010 (jonj)
**	    Added crib_xcb_next, crib_xcb_prev, crib_cursid, crib_inuse
**	    for cursor CRIBs.
**	    Cursor CRIBs are allocated when a cursor is opened, and linked
**	    from the XCB's primary CRIB.
*/
struct _LG_CRIB
{
#define			CRIB_CB		DM_CRIB_CB
#define			CRIB_ASCII_ID	CV_C_CONST_MACRO('C', 'R', 'I', 'B')
    LG_CRIB	    *crib_next;		/* Next cursor CRIB */
    LG_CRIB	    *crib_prev;		/* Prev cursor CRIB */
    PTR		    crib_cursid;	/* Cursor id abstraction, NULL
    					** if the XCB's CRIB */
    i4		    crib_inuse;		/* Number of RCBs open on this
    					** cursor CRIB */
    LG_LGID	    crib_log_id;	/* The caller's log handle */
    LG_LSN	    crib_low_lsn;	/* Oldest uncommitted LSN in DB */
    LG_LSN	    crib_last_commit;	/* LSN of last commit in DB */
    LG_LSN	    crib_bos_lsn;	/* Txn's last LSN at start of statement */
    u_i4	    crib_bos_tranid;	/* Tranid at start of statement */
    i4		    crib_sequence;	/* CRIB basis statement number */
    u_i4    	    *crib_xid_array;	/* Array of open tranids */
    u_i2	    crib_lgid_low;	/* Lowest open lg_id entry */
    u_i2	    crib_lgid_high;	/* Highest open lg_id entry */
};

/* Macro to check crib "c" at LG id_id for matching tranid */
#define CRIB_XID_WAS_ACTIVE(c, lg_id, tranid) \
    ((c)->crib_xid_array[(u_i2)lg_id] == tranid)

/* Macro to set crib_xid_array element lg_id to a value */
#define SET_CRIB_XID(c, lg_id, value) \
    ((c)->crib_xid_array[(u_i2)lg_id] = value)


/*}
** Name: LG_JFIB - Journal File Information Block
**
** Description:
**
**	This structure is embedded in each database (LDB)
**	and is used by LGwrite to predict the journal file and block
**	of each journaled log record for MVCC.
**
** History:
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Created for Consistent Read
*/
struct _LG_JFIB
{
    LG_LGID	    jfib_db_id;		/* The LG_A_JFIB caller's lpd_id */
    i4		    jfib_nodeid;	/* Cluster node id */
    LG_JFA	    jfib_jfa;		/* Journal file address */
    i4		    jfib_cycles;	/* # archive cycles on JFA filseq */
    i4		    jfib_ckpseq;	/* Journal checkpoint sequence */
    i4		    jfib_bksz;		/* Block size of journal file */
    i4		    jfib_maxcnt;	/* Max blocks in jnl file */
    i4		    jfib_next_byte;	/* Next avail byte on jnl page */
    i4		    jfib_bytes_left;	/* Bytes left on jnl page */
    LG_JFA	    jfib_cnf_jfa;	/* JFA from config file */
};


/*}
** Name: LG_RECORD - Log system record header format.
**
** Description:
**      This structure describes the format of a log record header that is
**	returned from a call to LGread().
**
** History:
**     23-jun-1986 (Derek)
**          Created.
**	15-mar-1993 (rogerk)
**	    Removed lgr_dbid, lgr_tran_id. These are now in the DM0L_HEADER.
*/
struct _LG_RECORD
{
    i4		    lgr_length;         /* Length of log record. */
    i2		    lgr_sequence;	/* Record sequence number. */
    i2		    lgr_extra;
    LG_LA	    lgr_prev;		/* Previous LG_RECORD. */
    char	    lgr_record[4];	/* The actual record. */
};

/*}
** Name: LG_HEADER - LG log file header information.
**
** Description:
**      This structure can be passed to LGshow() and LGalter() to inquire
**	about the contents of the header, or to change the contents of the
**	header in memory.  LGforce actually writes the changed header page
**      to disk.
**
**	The lgh_status field is used primarily during startup processing by
**	the RCP, ACP, and CSP. While the logging system (& RCP) are up and
**	running, the header status is LGH_VALID.
**
**	The various LGH_* values for lgh_status have the following meaning:
**
**	    LGH_VALID	--  The logging system is up and running. The RCP (or
**			    the CSP) has completed machine failure recovery.
**			    If the header is in this state when the RCP starts
**			    up, the RCP must have crashed, so the logging system
**			    scans from the last CP to find the correct EOF.
**	    LGH_RECOVER	--  Machine failure recovery is needed, but the BOF,
**			    EOF, and last-CP addresses in the log file header
**			    have all be verified. This value is set by the RCP
**			    if it is shutdown with "rcpconfig imm_shutdown" and
**			    is also set during machine failure recovery after
**			    the RCP has scanned backward and verified the last
**			    Consistency Point in the log file (dmr_get_cp()).
**	    LGH_EOF_OK	--  The logging system has scanned the log file and has
**			    located the true EOF position. The RCP can now
**			    proceed to verify the BOF and last-CP addresses.
**	    LGH_BAD	--  The log file is unusable. An error occurred while
**			    reading the log file header, or an error occurred
**			    while scanning backward to find the last CP. The
**			    only thing that can be done with this log file is
**			    to re-initialize it.
**	    LGH_OK	--  The RCP is not (yet) up, but the log file has been
**			    recovered and archived, if necessary. This
**			    state is similar to LGH_VALID, but indicates a
**			    certainty that no recovery action is needed. This
**			    state is set by the RCP during normal shutdown. It
**			    is also set by LGerase() after erasing the log file.
**			    Finally, it is set by the CSP in two different ways:
**			    1) After startup processing and performed both
**			    recovery AND archiving of this node's log, and
**			    2) After performing node-failure recovery of this
**			    node's log.
**
**			    LGH_OK is the only log file state (other than
**			    LGH_BAD) in which "rcpconfig init" is allowed.
**
**	During typical recovery from a system failure, the log file progresses
**	through its various stages as follows:
**	1) log file is in LGH_VALID state at startup.
**	2) logging system scans forward from last CP and determines true EOF.
**	    Logging system sets status to LGH_EOF_OK
**	3) RCP notices that state is EOF_OK and scans backward from EOF to find
**	    last Consistency Point. RCP then sets state to LGH_RECOVER.
**	4) RCP performs machine failure recovery, and then sets LGH_VALID, and
**	    system is now ready for use again.
**
** History:
**     23-jun-1986 (Derek)
**          Created.
**	30-oct-1991 (bryanp)
**	    Added documentation of header status field.
**	    Added lgh_timestamp and lgh_active_logs. These fields are used by
**	    the dual logging support to keep information about the current
**	    dual logging status in the log files. The time-stamping allows
**	    discrepancies between headers to be resolved in favor of the one
**	    with the greater timestamp.
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Added lgh_partitions to LG_HEADER.
*/
struct _LG_HEADER
{
    i4	    lgh_version;	/* Version of log file format. */
#define			LGH_VERSION_1	    0x10001
#define			LGH_VERSION_2	    0x10002	/* added dual logging */
#define			LGH_VERSION	    LGH_VERSION_2
    i4	    lgh_checksum;	/* Checksum of file header. */
    i4	    lgh_size;		/* Size of a log page. */
    i4	    lgh_count;		/* Number of log pages. */
    i4	    lgh_partitions;	/* Number of log file partitions */
    i4	    lgh_status;		/* Current status. */
#define			LGH_VALID	    1
#define			LGH_RECOVER	    2
#define			LGH_EOF_OK	    3
#define			LGH_BAD		    4
#define			LGH_OK		    5
    i4	    lgh_l_logfull;	/* When to stall everyone for logfull
					** event. */
    i4	    lgh_l_abort;	/* When to abort. */
    i4	    lgh_l_cp;		/* When to generate a cp. */
    i4	    lgh_cpcnt;		/* Maximum CP interval for invoking
					** archiver. */
    DB_TRAN_ID	    lgh_tran_id;	/* Last Transaction Id used. */
    LG_LA	    lgh_begin;		/* Last begin of file. */
    LG_LA	    lgh_end;		/* Last end of file. */
    LG_LA	    lgh_cp;		/* Last consistency point. */
    LG_LSN	    lgh_last_lsn;	/* Most recently assigned Log Sequence
					** Number for this logfile.
					*/
    i4	    lgh_timestamp[2];
			/* lgh_timestamp contains a TMget_stamp() timestamp.
			** The header is timestamped when it is written. This
			** allows recovery code to analyze the headers of the
			** dual logs and choose the one with the greater
			** timestamp. For single logging systems, it is
			** informational only.
			*/
    i4	    lgh_active_logs;
			/* lgh_active_logs contains information about which
			** log files are active in a dual-logging system. If
			** the system is NOT dual-logging, this field
			** should contain LGH_PRIMARY_LOG_ACTIVE.
			*/
#define	    LGH_PRIMARY_LOG_ACTIVE  0x01L
#define	    LGH_DUAL_LOG_ACTIVE	    0x02L
#define	    LGH_BOTH_LOGS_ACTIVE    (LGH_PRIMARY_LOG_ACTIVE|LGH_DUAL_LOG_ACTIVE)
};

/*}
** Name: LG_STAT - Statistics information.
**
** Description:
**      This structure describes the statistics return structure as returned
**	by the LGshow() call.
**
** History:
**      13-jul-1986 (Derek)
**          Created.
**	31-oct-1988 (rogerk)
**	    Added free_wait and stall_wait.
**	23-mar-1990 (mikem)
**	    Added lgs_stat_misc stat field to allow easier access to new stats.
**      04-feb-1991 (rogerk)
**          Added lgs_bcp_stall_wait and took bcp stalls out of the
**          lgs_stall_wait count.
**      15-aug-91 (jnash) merge 10-Oct-90 (pholman)
**          Added lgs_log_readio, lgs_dual_readio, lgs_log_writeio, lgs_dual_writeio.
**	28-jan-1992 (bryanp)
**	    Made lgs_stat_misc common to all systems as part of moving lg.h
**	    into the GL.
**	22-Apr-1996 (jenjo02)
**	    Added lgs_no_logwriter, count of times we needed a logwriter
**	    thread but none were available.
**	10-Jul-1996 (jenjo02)
**	    Added lgs_timer_write_idle, lgs_timer_write_time, lgs_buf_util.
**	01-Oct-1996 (jenjo02)
**	    Added lgs_max_wq, the maximum number of log buffers queued
**	    on a write queue, and lgs_max_wq_count, the number of time
**	    that max was encountered.
**	15-Mar-2006 (jenjo02)
**	    Deleted lgs_free_wait, lgs_stall_wait, lgs_bcp_stall_wait,
**	    add lgs_wait[] array.
**	21-Mar-2006 (jenjo02)
**	    Add lgs_optimwrites, lgs_optimpages.
**		
[@history_template@]...
*/
struct _LG_STAT
{
    i4      lgs_add;            /* Database adds. */
    i4	    lgs_remove;		/* Database removes. */
    i4	    lgs_begin;		/* Begin transaction. */
    i4	    lgs_end;		/* End transaction. */
    i4	    lgs_readio;		/* Read I/O's. */
    i4	    lgs_writeio;	/* Write I/O's. */
    i4	    lgs_write;		/* Write record calls. */
    i4	    lgs_force;		/* Force calls. */
    i4	    lgs_split;		/* Log record splits. */
    i4	    lgs_group;		/* Number of group commits. */
    i4	    lgs_group_count;	/* Group commit count. */
    i4	    lgs_inconsist_db;	/* Number of nconsistent database 
				** occurs. */
    i4	    lgs_check_timer;    /* gcmt buffer checks */
    i4	    lgs_timer_write;    /* gcmt buffer writes */
    i4	    lgs_timer_write_time; /* Writes due to numticks exceeded */
    i4	    lgs_timer_write_idle; /* Writes due to idle buffer */
    i4	    lgs_kbytes;
    i4      lgs_log_readio;     /* Number of read from the II_LOG_FILE. */
    i4      lgs_dual_readio;    /* Number of read from the II_DUAL_LOG. */
    i4      lgs_log_writeio;    /* Number of write complete to the II_LOG_FILE. */
    i4      lgs_dual_writeio;   /* Number of write complete to the II_DUAL_LOG. */
    i4	    lgs_optimwrites;	/* Number of optimized writes */
    i4	    lgs_optimpages;	/* Number of optimized pages */
    i4	    lgs_no_logwriter;   /* Number of times lw unavailable */
    i4	    lgs_max_wq;		/* Max # buffers on write queue */
    i4	    lgs_max_wq_count;	/* # times that max was hit */
    i4	    lgs_buf_util_sum;   /* Sum of all buckets */
    i4	    lgs_buf_util[10];   /* Buffer utilization histogram buckets */
    i4	    lgs_wait[LG_WAIT_REASON_MAX+1]; /* Waits */
    i4	    lgs_stat_misc[72];  /* system-specific statistics */
};

/*}
** Name: LG_DATABASE - Database description.
**
** Description:
**      This structure is returned by the LGshow() call to describe an
**	active database.
**
** History:
**      21-aug-1986 (Derek)
**          Created.
**      21-jan-1989 (EdHsu)
**          Adding DB status, and dump information to support
**          online backup feature.
***     15-may-1989 (EdHsu)
**          Integrate online backup.
**      14-may-1990 (rogerk)
**          Increased buffer size from 128 bytes to LG_DBINFO_MAX as the
**          old buffer size may not have been large enough for databases
**          with long root path descriptions.
**	31-may-90 (blaise)
**	    Integrated changes from termcl:
**		Increase buffer size from 128 bytes to LG_DBINFO_MAX.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Added external database id information.
**	    Added DB_CKPDB_ERROR database status.
**	    Removed obsolete EBACKUP, PRG_PEND, ONLINE_PURGE states.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Support:
**	    Add db_sback_lsn to the LG_DATABASE for showing the LSN of the start
**		of the backup.
**	26-jul-1993 (rogerk)
**	    Changed journal and dump window tracking in the logging system.
**	    Journal and dump windows are now tracked using log address values
**	    rather then the CP addresses.
**	13-jun-1997 (wonst02)
** 	    Added DB_READONLY status for readonly databases.
**	03-Jul-1997 (shero03)
**	    Added DB_JSWITCH for journal switching in dmfcpp
**	13-Nov-1997 (jenjo02)
**	    Added DB_STALL_RW for online backup.
**	26-Apr-2000 (thaju02)
**	    Added DB_CKPLK_STALL. (B101303)
**      01-may-2008 (horda03) Bug 120349
**          Add DB_STATUS_MEANING define
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Added db_last_commit, db_jfib, db_first_la,
**	    db_last_lsn, db_lgid_low, db_lgid_high.
*/
struct _LG_DATABASE
{
    LG_DBID         db_id;              /* Database identifier. */
    i4	    db_l_buffer;	/* Length of database information. */
    char	    db_buffer[LG_DBINFO_MAX];
					/* Database information buffer. */
    i4	    db_status;		/* Status of the database. */
#define		    DB_INVALID			0x00000001
#define		    DB_JNL			0x00000002
#define		    DB_NOTDB			0x00000004
#define		    DB_PURGE			0x00000008
#define		    DB_OPENDB_PEND		0x00000010
#define		    DB_CLOSEDB_PEND		0x00000020
#define		    DB_RECOVER			0x00000040
#define		    DB_FAST_COMMIT		0x00000080
#define		    DB_PRETEND_CONSISTENT 	0x00000100
#define		    DB_CLOSEDB_DONE		0x00000200
#define		    DB_BACKUP			0x00000400
#define		    DB_STALL			0x00000800
#define		    DB_READONLY			0x00001000
#define		    DB_FBACKUP			0x00002000
#define		    DB_CKPDB_PEND		0x00004000
#define		    DB_JSWITCH			0x00008000


#define		    DB_CKPDB_ERROR		0x00020000
#define		    DB_ACTIVE			0x00040000L
#define		    DB_OPN_WAIT			0x00080000L
#define		    DB_CKPLK_STALL		0x00100000
#define		    DB_MVCC			0x00200000

#define DB_STATUS_MEANING "\
INVALID,JOURNAL,NOTDB,PURGE,OPENDB_PEND,CLOSEDB_PEND,\
RECOVER,FAST_COMMIT,PRETEND_CONSISTENT,CLOSEDB_DONE,BACKUP,\
STALL,READONLY,FBACKUP,CKPDB_PEND,JSWITCH,0x00010000,\
CKPDB_ERROR,ACTIVE,OPN_WAIT,CKPLK_STALL,MVCC"

    i4	    db_database_id;	/* External Database Id */
    LG_LA           db_sbackup;         /* Start backup lga     */
    LG_LSN	    db_sback_lsn;	/* Start Backup LSN     */
    LG_LSN	    db_eback_lsn;	/* End Backup LSN       */
    u_i2	    db_lgid_low;	/* Lowest open LXB in DB */
    u_i2	    db_lgid_high;	/* Highest open LXB in DB */
    LG_LSN	    db_last_commit;	/* LSN of last commit in DB */
    LG_LSN	    db_last_lsn;	/* LSN of last commit in DB */
    LG_LA	    db_first_la;	/* First LGA since DB opened */
    LG_LA           db_f_dmp_la;        /* Start of dump window. */
    LG_LA           db_l_dmp_la;        /* End of dump window. */
    LG_LA	    db_f_jnl_la;	/* Start of journal window. */
    LG_LA	    db_l_jnl_la;	/* End of journal window. */
    LG_JFIB	    db_jfib;		/* DB's CR journal info */
    struct
    {
	i4	    trans;		/* Active transaction count. */
	i4	    read;		/* Read I/O for this database. */
	i4	    write;		/* Write records for this database. */
	i4	    begin;		/* Begin transactions for this db. */
	i4	    end;		/* End transaction for this db. */
	i4	    force;		/* Forces for this db. */
	i4	    wait;		/* Waits for this db. */
    }		    db_stat;
};

/*}
** Name: LG_TRANSACTION - Transaction description.
**
** Description:
**      This structure is returned by the LGshow() call to describe an
**	active transaction.
**
** History:
**      21-aug-1986 (Derek)
**          Created.
**	12-oct-1992 (bryanp)
**	    Fix tr_status bit definitions following LG/LK mainline changes.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:  Added external database id information.
**	08-sep-93 (swm)
**	    Changed tr_sess_id type from i4 to CS_SID to match recent CL
**	    interface revision.
**	18-oct-93 (rogerk)
**	    Added new information for LGshow transaction calls:
**	        TR_ORPHAN, TR_NORESERVE transaction status flags.
**		tr_reserved_space - amount of log file space reserved by xact.
**	15-Dec-1999 (jenjo02)
**	    Support for SHARED log transactions.
**	    Added new fields to LG_TRANSACTION, updated tr_status.
**	5-Jan-2006 (kschendel)
**	    Make reserved-space larger for giant logs.
**	21-Jun-2006 (jonj)
**	    Add tr_lock_id, tr_txn_state for XA integration.
**	11-Sep-2006 (jonj)
**	    Add tr_handle_wts, tr_handle_ets for LOGFULL_COMMIT.
**	    Deleted unused TR_SAVEABORT to free up a bit.
**      01-may-2008 (horda03) Bug 120349
**          Add TR_STATUS_MEANING define
[@history_template@]...
*/
struct _LG_TRANSACTION
{
    i4	    	    tr_status;		/* Transaction status. */
#define			TR_INACTIVE	    0x0001
#define			TR_ACTIVE	    0x0002
#define			TR_CKPDB_PEND	    0x0004
#define			TR_PROTECT	    0x0008
#define			TR_JOURNAL	    0x0010
#define			TR_READONLY	    0x0020
#define			TR_NOABORT	    0x0040
#define			TR_RECOVER	    0x0080
#define			TR_FORCE_ABORT	    0x0100
#define			TR_SESSION_ABORT    0x0200
#define			TR_SERVER_ABORT	    0x0400
#define			TR_PASS_ABORT	    0x0800
#define			TR_DBINCONST	    0x1000
#define			TR_WAIT		    0x2000
#define			TR_DISTRIBUTED	    0x4000
#define			TR_WILLING_COMMIT   0x8000
#define			TR_REASSOC_WAIT	    0x10000
#define			TR_RESUME	    0x20000
#define			TR_MAN_ABORT	    0x40000
#define			TR_ABORT (TR_RECOVER | TR_FORCE_ABORT | TR_SESSION_ABORT | TR_SERVER_ABORT | TR_PASS_ABORT | TR_MAN_ABORT)
#define			TR_MAN_COMMIT	    0x80000
#define			TR_LOGWRITER	    0x100000
#define			TR_NORESERVE	    0x400000
#define			TR_ORPHAN	    0x800000
#define			TR_WAIT_LBB	    0x100000
#define			TR_SHARED	    0x2000000
#define			TR_SHARED_HANDLE    0x4000000
#define			TR_ET    	    0x8000000
#define			TR_PREPARED    	    0x10000000
#define			TR_IN_MINI    	    0x20000000
#define			TR_ONEPHASE    	    0x40000000
#define			TR_ILOG    	    0x80000000

#define TR_STATUS_MEANING "\
INACTIVE,ACTIVE,CKPDB_PEND,PROTECT,JOURNAL,READONLY,NOABORT,\
RECOVER,FORCE_ABORT,SESSION_ABORT,SERVER_ABORT,PASS_ABORT,DBINCONSISTENT,WAIT,\
DISTRIBUTED,WILLING_COMMIT,RE-ASSOC,RESUME,MAN_ABORT,MAN_COMMIT,LOGWRITER,0x00200000,\
NORESERVE,ORPHAN,WAIT_LBB,SHARED,HANDLE,ET,PREPARED,IN_MINI,1PC,ILOG"

    i4	    	    tr_wait_reason;	/* reason why this transaction is
					** waiting.
					*/
    LG_LXID         tr_id;              /* Internal transaction id. */
    DB_TRAN_ID	    tr_eid;		/* External transaction id. */
    LG_DBID	    tr_db_id;		/* Internal Dbid for this xact. */
    i4	    	    tr_database_id;	/* External Database Id. */
    i4	    	    tr_pr_id;		/* Process for this transaction. */
    CS_SID	    tr_sess_id;         /* Session for this transaction. */
    i4	    	    tr_lpd_id;		/* Process database reference for this 
					** transaction. */
    LG_LA	    tr_first;		/* First log record address. */
    LG_LA	    tr_last;		/* Last log record address. */
    LG_LA	    tr_cp;		/* CP interval of first log record. */
    LG_LSN	    tr_first_lsn;	/* First LSN of txn */
    LG_LSN	    tr_last_lsn;	/* Last LSN of txn */
    LG_LSN	    tr_wait_lsn;	/* LSN to wait upon */
    i4		    tr_wait_buf;	/* Buffer if TR_WAIT_LBB */
    i4	    	    tr_l_user_name;	/* Length of the user name. */
    char	    tr_user_name[DB_MAXNAME]; /* User name of the transaction. */
    DB_DIS_TRAN_ID  tr_dis_id;		/* Distributed transaction id. */
    u_i8	    tr_reserved_space;	/* Logfile space reserved by xact. */
    i4		    tr_handle_count;	/* if TR_SHARED, number of connected
					** TR_HANDLEs */
    i4		    tr_handle_preps;	/* if TR_SHARED, number of TR_HANDLEs
					** prepared for WILLING_COMMIT */
    i4		    tr_handle_wts;	/* if TR_SHARED, number of TR_HANDLEs
					** doing LGwrite */
    i4		    tr_handle_ets;	/* if TR_SHARED, number of handle_wts
					** doing ETs */
    i4		    tr_shared_id;	/* if TR_HANDLE, id of SHARED LXB */
    i4		    tr_lock_id;		/* Lock list id associated with
					** this log context. */
    i4		    tr_txn_state;	/* XA txn state at time of xa_end */
    struct
    {
	i4	    write;		/* Write records. */
	i4	    split;		/* Write splits. */
	i4	    force;		/* Force log. */
	i4	    wait;		/* Log waits. */
    }		    tr_stat;
};

/*}
** Name: LG_PROCESS - Process description.  
** 
** Description: 
**      This structure is returned by LGshow to describe a 
**      process attached to the logging system.
**
** History:
**      21-aug-1986 (Derek)
**          Created.
**      22-jan-1989 (EdHsu)
**          Add PR_CKPDB to identify the ckpdb server.
**      27-mar-1989 (rogerk)
**          Added process types and buffer manager id for multi-serveR
**          fast commit.
**	15-may-1989 (EdHsu)
**	    Integrate online backup feature.
**	10-jul-1989 (annie)
**	    Added internal process id, pr_ipid
**	16-oct-1992 (jnash) merged 10-oct-1992 (rogerk)
**	    Reduced logging Project: Added PR_CPAGENT process status.
**	    This indicates that the process participates in Consistency Points.
**      01-may-2008 (horda03) Bug 120349
**          Add PR_TYPE define.
[@history_template@]...
*/
struct _LG_PROCESS
{
    i4         pr_id;              /* Process identifier. */
    i4	    pr_pid;		/* Process id. */
    i4	    pr_type;		/* Status information. */
#define                 PR_MASTER       0x001
#define                 PR_ARCHIVER     0x002
#define                 PR_FCT          0x004
#define                 PR_RUNAWAY      0x008
#define                 PR_SLAVE        0x010
#define                 PR_CKPDB        0x020
#define			PR_VOID		0x040
#define                 PR_SBM          0x080
#define                 PR_IDLE         0x100
#define                 PR_DEAD         0x200
#define                 PR_DYING        0x400
#define			PR_CPAGENT	0x800

#define PR_TYPE "\
MASTER,ARCHIV,FCT,RUNAWAY,SLAVE,CKPDB,VOID,SBM,IDLE,DEAD,DYING,CPAGENT"

    i4         pr_bmid;            /* Shared buffer manager id */
    struct
    {
	i4	    database;		/* Open databases. */
	i4	    write;		/* Writes. */
	i4	    force;		/* Force log. */
	i4	    wait;		/* Log waits. */
	i4	    begin;		/* Begin transactions. */
	i4	    end;		/* End transactions. */
    }		    pr_stat;
    i4	    pr_ipid;		/* Internal process id. */
};

/*}
** Name: LG_BUFFER - Buffer description.  
** 
** Description: 
**      This structure is returned by LGshow to describe a 
**      page buffer in the logging system.
**
** History:
**	2-nov-1992 (bryanp)
**	    Created.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: add buf_id
*/
struct _LG_BUFFER
{
    i4		    buf_id;		/* Buffer id */
    i4	    	    buf_state;		/* State. */
#define			LG_BUF_FREE			    0x0001L
#define			LG_BUF_MODIFIED			    0x0002L
#define			LG_BUF_FORCE			    0x0004L
#define			LG_BUF_PRIMARY_LXB_ASSIGNED	    0x0008L
#define			LG_BUF_DUAL_LXB_ASSIGNED	    0x0010L
#define			LG_BUF_PRIM_IO_INPROG		    0x0020L
#define			LG_BUF_DUAL_IO_INPROG		    0x0040L
#define			LG_BUF_PRIM_IO_CMPLT		    0x0080L
#define			LG_BUF_DUAL_IO_CMPLT		    0x0100L
#define			LG_BUF_PRIM_IO_NEEDED		    0x0200L
#define			LG_BUF_DUAL_IO_NEEDED		    0x0400L
    i4	    	    buf_offset;
    i4	    	    buf_next_offset;
    i4	    	    buf_prev_offset;
    i4	    	    buf_next_byte;	/* offset of next byte in buffer. */
    i4	    	    buf_bytes_used;	/* Bytes used in buffer. */
    i4	    	    buf_n_waiters;	/* number of waiters */
    i4	    	    buf_n_writers;	/* number of concurrent writers */
    i4	    	    buf_n_commit;	/* number of commits */
    LG_LA	    buf_lga;		/* Starting LGA for this page. */
    LG_LA	    buf_forced_lga;	/* Resulting forced lga when this page
					** completes.
					*/
    LG_LSN	    buf_first_lsn;	/* First LSN in buffer */
    LG_LSN	    buf_last_lsn;	/* Last LSN in buffer */
    LG_LSN	    buf_forced_lsn;	/* Resulting LSN when this
					** page write completes */
    i4	    	    buf_resume_cnt;	/* Number of writes complete before 
					** resuming the waiters.
					*/
    i4	    	    buf_assigned_owner;
    i4	    	    buf_prim_owner;
    i4	    	    buf_prim_task;
    i4	    	    buf_dual_owner;
    i4	    	    buf_dual_task;

    /* These 4 fields are valid only for the current buffer (LG_S_CBUFFER): */

    i4	    	    buf_timer_lbb;
    i4	    	    buf_last_used;
    i4	    	    buf_total_ticks;
    i4	    	    buf_tick_max;
};

/*}
** Name: LG_MUTEX - LG semaphore statistics.
**
** Description:
**      This structure describes the semaphore statistictics structure
**	returned by the LGshow(LG_S_MUTEX) call.
**
** History:
**	08-Dec-1995 (jenjo02)
**          Created.
**		
[@history_template@]...
*/
struct _LG_MUTEX
{
    i4         count;            /* number of blocks returned */
#define LG_MUTEX_MAX 32
    CS_SEM_STATS    stats[LG_MUTEX_MAX]; /* stats array */
};

/*}
** Name: LG_FDEL - Post-commit File Delete info
** 
** Description: 
**	This structure is passed by DMF to LGalter to define
**	an XA Transaction Branch's file that must be deleted
**	post-commit of the Global Transaction.
**
** History:
**	25-Aug-2006 (jonj)
**	    Invented.
*/
struct _LG_FDEL
{
    LG_LXID         fdel_id;			/* Branch's log id. */
    i4	    	    fdel_plen;			/* Length of Path */
    i4	    	    fdel_flen;			/* Length of Filename */
    char	    fdel_path[DB_AREA_MAX];	/* Path */
    char	    fdel_file[DB_FILE_MAX];	/* Filename */
};

/*}
** Name: LG_PURGE - Information about a purged database.
**
** Description:
**      This structure is used to pass information about a purged
**	database.  Currently, this information is the last successful
**	archived CP address.
**
** History:
**      22-may-1987 (Derek)
**          Created.
[@history_template@]...
*/
struct _LG_PURGE
{
    LG_DBID		lgp_dbid;	/*  Database identification. */
    LG_LA		lgp_f_cp;	/*  First journaled CP address. */
};

/*}
** Name: LG_OBJECT - Describes Object being passed to LG_WRITE to be logged.
**
** Description:
**	This structure is used to describe an object to be logged in an
**	LG_WRITE call.  An array of LG_OBJECT structs is passed to LG_WRITE
**	giving the size and address of a number of objects which together
**	make up a log record.
**
** History:
**      08-oct-1987 (rogerk)
**          Created.
[@history_template@]...
*/
struct _LG_OBJECT
{
    i4	    lgo_size;		/* Length of the data section. */
    PTR		    lgo_data;		/* Pointer to data section. */
};

/*}
** Name: LG_CONTEXT - Describes the context of a transaction.
**
** Description:
**	This structure is used to describe the first log address and the
**	last log address of a WILLING COMMIT transaction.
**
** History:
**      20-Jan-1989 (ac)
**          Created.
*/
struct _LG_CONTEXT
{
    LG_LXID         lg_ctx_id;          /* Internal transaction id. */    
    DB_TRAN_ID	    lg_ctx_eid;		/* External transaction id. */
    DB_DIS_TRAN_ID  lg_ctx_dis_eid;	/* Distributed transaction id. */
    LG_LA	    lg_ctx_first;	/* First log record address. */
    LG_LA	    lg_ctx_last;	/* Last log record address. */
    LG_LA	    lg_ctx_cp;		/* First CP log record address. */
};

/*
** The LG_RECOVER structure describes the parameter format
** for LGalter(LG_A_RCP_IGNORE_DB
** and LGalter(LG_A_RCP_IGNORE_TABLE
** and LGalter(LG_A_RCP_IGNORE_LSN
** 
** and LGalter(LG_A_RCP_IGNORE_INIT
*/
typedef struct
{
    i4		    lg_flag;
    LG_LSN	    lg_lsn;
    char	    lg_tabname[DB_MAXNAME];
    char	    lg_dbname[DB_MAXNAME];
} LG_RECOVER;

/*
** The LG_HDR_ALTER_SHOW structure describes the parameter format when
** altering or showing a log file's header.
*/
typedef struct
{
    LG_LGID	    lg_hdr_lg_id;
    LG_HEADER	    lg_hdr_lg_header;
} LG_HDR_ALTER_SHOW;

/*
** The LG_LFLSN_ALTER structure describes the parameter format when
** altering the last forced lsn (LG_A_LFLSN) of the log file.
*/
typedef struct
{
    LG_LGID	    lg_lflsn_lg_id;
    LG_LSN	    lg_lflsn_lsn;
} LG_LFLSN_ALTER;

/*
** Name: LG Testpoints	- definitions of LG testpoints.
**
** Description:
**	LG testpoints allow for the implementation of automated tests.
**
** History:
**	18-jul-1990 (bryanp)
**	    Created.
**	29-aug-1990 (bryanp)
**	    Write failure testpoints.
*/
#define	    LG_T_PRIMARY_OPEN_FAIL	0
		/*
		** Causes LGopen to fail to open the primary (or only) logfile.
		*/
#define	    LG_T_DUAL_OPEN_FAIL		1
		/*
		** Causes LGopen to fail to open the dual log file
		*/
#define	    LG_T_FILESIZE_MISMATCH	2
		/*
		** Causes log files sizes to appear to be different
		*/
#define	    LG_T_NEXT_READ_FAILS	3
		/*
		** Causes the next LGread to encounter a 'transient' error.
		** This testpoint is reset after it fires.
		*/
#define	    LG_T_READ_FROM_PRIMARY	4
		/*
		** If both log files are active, forces a read from the
		** primary log file only.
		*/
#define	    LG_T_READ_FROM_DUAL		5
		/*
		** If both log files are active, forces a read from the
		** dual log file only.
		*/
#define	    LG_T_DATAFAIL_PRIMARY	6
#define	    LG_T_DATAFAIL_DUAL		7
		/*
		** Selects a random log page, not the header page, on the
		** indicated log, and causes the next write to that page to
		** corrupt the page.
		*/
#define	    LG_T_PWRBTWN_PRIMARY	8
#define	    LG_T_PWRBTWN_DUAL		9
		/*
		** Selects a random non-header page, on the primary or dual log
		** as indicated, and simulates a power failure where the power
		** fails after writing to the indicated log and before writing
		** to the other log.
		*/
#define	    LG_T_PARTIAL_AFTER_PRIMARY	10
#define	    LG_T_PARTIAL_AFTER_DUAL	11
		/*
		** Selects a random non-header page, on the primary or dual log
		** as indicated, and simulates a power failure where the power
		** fails after writing to the indicated log and during the
		** write to the other log.
		*/
#define	    LG_T_PARTIAL_DURING_PRIMARY	12
#define	    LG_T_PARTIAL_DURING_DUAL	13
		/*
		** Selects a randome non-header page, on the primary or dual log
		** as indicated, and simulates a power failure where the power
		** fails during the write to the indicated log and before the
		** write to the other log.
		*/
#define	    LG_T_HDRFAIL_PRIMARY	14
#define	    LG_T_HDRFAIL_DUAL		15
		/*
		** Causes both copies of the header on the indicated log to be
		** altered to be unreadable.
		*/
#define	    LG_T_HDRFAIL_BETWEEN	16
		/*
		** Simulates a power failure after writing the primary log's
		** headers and before writing the dual log's headers.
		*/
#define	    LG_T_HDR_PARTIAL_DUAL	17
		/*
		** Simulates a power failure after writing the primary log's
		** headers and during the writing of the dual log's headers.
		*/
#define	    LG_T_HDR_PARTIAL_PRIMARY	18
		/*
		** Simulates a power failure during the writing of the primary
		** log's headers.
		*/
#define	    LG_T_PARTIAL_DURING_BOTH	19
		/*
		** Selects a random non-header page and simulates a power
		** failure where the power fails during the write to both logs,
		** leaving both logs with partial last page written.
		*/
#define	    LG_T_QIOFAIL_PRIMARY	20
#define	    LG_T_QIOFAIL_DUAL		21
		/*
		** Selects a random non-header log page, on the primary or dual
		** log file as indicated, and causes the next write to that
		** page to have a QIO failure.
		*/
#define	    LG_T_IOSBFAIL_PRIMARY	22
#define	    LG_T_IOSBFAIL_DUAL		23
		/*
		** Selects a random non-header log page, on the primary or dual
		** log file as indicated, and causes the next write to that
		** page to have a failed I/O, as seen by an erroneous IOSB
		** (I/O Status Block).
		*/
#define	    LG_T_QIOFAIL_BOTH		24
		/*
		** Selects a random non-header log page, and causes the next
		** write to that page to have BOTH QIO's fail.
		*/
#define	    LG_T_IOSBFAIL_BOTH		25
		/*
		** Selects a random non-header log page, and causes the next
		** write to that page to have BOTH IOSB's fail.
		*/
#define	    LG_T_PARTIAL_SPAN		26
		/*
		** Simulates a power failure in which the last log page
		** written checksums correctly but contains a partial log
		** record ((lbh_address.lga_low & (header->lgh_size-1))==0)
		*/
#define	    LG_T_QIOFAIL_HDRPRIM	27
#define	    LG_T_QIOFAIL_HDRDUAL	28
		/*
		** Causes the next write to the primary or dual header, as
		** indicated, to have a QIO failure.
		*/
#define	    LG_T_IOSBFAIL_HDRPRIM	29
#define	    LG_T_IOSBFAIL_HDRDUAL	30
		/*
		** Causes the next write to the primary or dual header, as
		** indicated, to have an IOSB write completion failure.
		*/
#define	    LG_T_PARTIAL_LAST		31
		/*
		** Simulates a power failure in which the last log page is
		** full but the last log record on the page is partially
		** written (i.e., it spans pages).
		*/


/*}
** Name: LG_PARTION - Name of log file partition
**
** Description:
**      This structure is used to specify the name of a Log file
**      partition
**      
** History:
**      30-Oct-2009 (horda03)
**          Created.
*/          
typedef struct
{
  i4   lg_part_len;
  char lg_part_name [MAX_LOC+1];
} LG_PARTITION;

