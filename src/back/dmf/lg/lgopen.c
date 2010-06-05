/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <pm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <dbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <cv.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>
#include    <lgclustr.h>

/**
**
**  Name: LGOPEN.C - Implements the LGopen function of the logging system
**
**  Description:
**	This module contains the code which implements LGopen.
**	
**	    LGopen -- Connect this process to the logging system
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	12-oct-1992 (bryanp)
**	    Only the master must be given the block_size arg to LGopen. 
**	    Others can read the block_size from the LG_HEADER.
**	16-oct-1992 (jnash) for rogerk
**	    Reduced logging project.  Removed references to obsolete 
**	    lgd->lgd_n_servers counter
**	18-jan-1993 (bryanp)
**	    LG_logs_opened is now a GLOBALDEF.
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (andys/bryanp/keving)
**	    Cluster 6.5 Project I:
**	    On the VAXCluster, we must append the nodename to the logfile name.
**	    When the process opens the log, call LGlsn_init.
**	    Renamed stucture members of LG_LA to new names. This means
**		replacing lga_high with la_sequence, and lga_low with la_offset.
**	    Set DI_SYNC_MASK when opening log files.
**	    Pass the block size to LG_open so that it can create buffers
**		of the correct size
**	    Initialize lpb_gcmt_sid.
**	24-may-1993 (andys)
**	    In form_path_and_file, use LO routines.
**	    In LGopen, deal better with the case where the primary logfile
**	    has vanished by using the dual logfile to find the logfile
**	    blocksize. This allows LGopen to (later) disable the primary 
**	    logfile correctly.
**	    In LGopen, correct off by one error where DIsense returns page 
**	    number and we want number of pages.
**	25-may-1993 (bryanp)
**	    If II_DUAL_LOG or II_DUAL_LOG_NAME isn't defined, this means the
**	    user isn't using dual logging. Don't complain, just use the primary.
**	21-jun-1993 (andys/bryanp)
**	    CSP/RCP process linkage: call LGc_csp_enqueue when opening log.
**	    Replace LG_NOHEADER with LG_LOG_INIT.
**	    If LG_LOG_INIT set, still try to open both logfiles...
**	21-jun-1993 (rogerk)
**	    Added LG_IGNORE_BOF flag to cause verify_header to do the logfile
**	    scan for bof_eof even if the logfile header status indicates that
**	    the bof/eof values in the header are valid.  This can be used by
**	    logdump to dump entire logfiles.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    When the master opens a remote log, don't reset master_lpb if it's
**		already set -- this was causing the CSP to overwrite master_lpb
**		when it opened a remote log during node failure recovery.
**	    Include <tr.h>
**	26-jul-1993 (rogerk)
**	    Initialize new archive window fields.
**	23-aug-1993 (jnash)
**	    Include time and date in many TRdisplays.
**	18-oct-1993 (rogerk)
**	    Initialize new lpb_cache_factor field.
**	31-jan-1994 (mikem)
**	    Sir #57044
**	    Changed debugging logging of log file scans to print messages
**	    every 1000 buffers rather than every 100 buffers.  This makes
**	    the rcp log a little more readable.
**	31-jan-1994 (jnash)
**	  - In LG_verify_header(), verify that log page size passed in
**	    (if any) matches that cataloged in log file header.
**	  - Fix lint detected problem: remove unused "channel" variable in 
**  	    reconstruct_header().
**	15-feb-1994 (andyw)
**	    Removed iosb TRdisplay's from scan_bof_eof() & check_uneven_eof().
**	20-apr-1994 (mikem)
**	    bug #62034
**	    Fixed LG_verify_header to not return an error in the case of a
**	    mismatched blocksize between the on disk copy and the config.dat
**	    copy, if it was being called as part of "rcpconfig -init" (ie. to
**	    change the blocksize of the log file).
**	25-apr-1994 (bryanp) B61064
**	    Set lgk_mem.mem_creator_pid to the master's PID when the master
**		opens the log. This greatly reduces the problems associated
**		with the shared memory re-initialization code in LGK_initialize.
**		Under some circumstances, due to the timing of the various
**		subprocesses fired off by ingstart, the ingstart processing
**		completed leaving the installation up with the shared memory
**		owned by one of ingstart's subprocesses (one which was used to
**		run rcpstat, usually). By forcing the mem_creator_pid to be
**		set to the RCP's pid (the CSP's, in a cluster), we ensure that
**		the LGK_initialize re-formatting the shared memory code doesn't
**		accidentally fire.
**	23-may-1994 (bryanp) B58497
**	    Don't allow multiple local logfile opens with the same process PID.
**	12-jul-1995 (duursma)
**	    on VMS, call the new function NMgtIngAt() when translating 
**	    II_DUAL_LOG or II_DUAL_LOG_NAME
**      12-jun-1995 (canor01)
**          semaphore protect memory allocation (MCT)
**      13-jun-1995 (chech02)
**          Added LGK_misc_sem semaphore to protect critical sections.(MCT)
**	16-oct-1995 (nick)
**	    Add extra parameter to scan_bof_eof() to catch bug #71906.
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named when calling LKMUTEX functions. Many semaphores added
**	    to suppliment the singular lgd_mutex.
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores.
**	23-sep-1996 (canor01)
**	    Move global data definitions to lgdata.c.
**	23-sep-1997 (kosma01)
**	    The rs4_us5 platform keeps its pthread mutexes and condition
**	    variables on a chain and the chain is broken if a Ingres
**	    semaphore is moved as is done here for the DI ingres_log 
**	    semaphore.
**	14-Nov-1997 (hanch04)
**	    Reorder math to prevent overflow for files > 2 gig
**      08-Dec-1997 (hanch04)
**          Initialize new la_block field in LG_LA,
**	    Use new la_block field when block number is needed instead
**	    of calculating it from the offset
**	    for support of logs > 2 gig
**	18-Dec-1997 (jenjo02)
**	    Added LG_my_pid GLOBAL. Removed GLOBALREFs and defines which 
**	    are already defined in lgdef.h.
**    30-Dec-1997 (hanch04)
**        lgh_begin should be lgh_end plus one block
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Changed references to DI_IO to LG_IO, di_io to lg_io. Replaced
**	    DIread/write with LG_READ/WRITE macros.
**	    Replaced lpb_gcmt_sid with lpb_gcmt_lxb, removed lpb_gcmt_asleep.
**	01-sep-1998 (hanch04)
**	    Remove fall through for II_LOG_FILE from symbol table.  Only read
**	    it from config.dat or fail.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	24-Aug-1999 (jenjo02)
**	    Replaced SHARE lock of lgd_lpb_q_mutex with EXCL.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-may-2001 (devjo01)
**	    Add lgclustr.h for CXcluster_enabled() macro.
**	    remove call to LGc_rcp_enqueue. (s103715)
**	11-feb-2002 (devjo01)
**	    Correct file open logic for when nodes have logs in
**	    different locations.
**	27-may-2002 (devjo01)
**	    Zero lfb_buf_cnt when initializing LFB.
**	24-oct-2003 (devjo01)
**	    replace LGcluster with CXcluster_enabled
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**      14-Jan-2010 (horda03) Bug 123153
**          Don't try to allocate buffers for node recovery
**          when nodename specified if the lfb's lgh_size
**          is 0. Prevents rcpconfig from initialising a
**          TX log file on another node.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

/*
** Prototypes for internal functions:
*/
static	STATUS	LG_open(i4 flag, PTR nodename, i4  l_nodename, LFB *lfb, 
			u_i4 logs_opened, LG_HEADER **hdr_ptr,
			LG_ID *lg_id, i4 logpg_size, CL_ERR_DESC *sys_err);
static	STATUS	LG_verify_header(i4 flag, i4  blocksize, u_i4 blocks,
				i4 log_to_disable,
				LG_IO *primary_lg_io, LG_IO *dual_lg_io,
				u_i4 logs_opened,
				LG_HEADER *header, CL_ERR_DESC *sys_err);
static STATUS	read_headers(i4 flag, i4 disabled_log,
				LG_IO *primary_lg_io, LG_IO *dual_lg_io,
				u_i4 logs_opened,
				LG_HEADER *header, CL_ERR_DESC *sys_err);
static STATUS	reconstruct_header( u_i4 flag, i4 blocks,
				LG_IO *lg_io,
				LG_HEADER *header,
				CL_ERR_DESC *sys_err);
static STATUS	scan_bof_eof(u_i4 flag, LG_HEADER *header,
				STATUS header_status,
				u_i4 logs_opened,
				LG_IO *primary_lg_io, LG_IO *dual_lg_io,
				CL_ERR_DESC *sys_err);
static STATUS	check_uneven_eof( LG_HEADER *header, i4 seq,
				char *buffer_primary, i4 *eof_block,
				LG_IO *primary_lg_io, LG_IO *dual_lg_io,
				CL_ERR_DESC *sys_err);
static STATUS	cross_check_logs( LG_HEADER *header_primary,
				LG_HEADER *header_dual );
static STATUS	form_path_and_file(
				char *nodename,
				char *file,
				i4 *l_file,
				char *path, 
				i4 *l_path);

/*{
** Name: LGopen	- Open log file.
**
** Description:
**      This routine opens the log file for the current process establishing
**      a connection to the logger. If the caller specified the LG_MASTER flag,
**	then the log file can be initialized by the caller and the number of 
**	allocated blocks in the log file is returned.  Otherwise, if the log
**	file has not been initialized the caller will be returned an error.
**
**	The lg_id returned by this call is used in further calls to 
**	LGclose and LGadd.
**
**	If, prior to calling LGopen, the logging system has been set into
**	dual-logging mode via the proper LGalter() call, LGopen will open both
**	the primary log file and the dual log file and then (if this is the
**	master) will verify the two log files in tandem. In this process,
**	LGopen may discover that one of the two log files has previously
**	encountered an error (this will be marked in the log file headers). If
**	this is the case, or if any new errors are encountered during the dual
**	log opening and EOF scanning, LGopen will dynamically degrade the
**	system into dual-logging-with-single-active-log mode. All of this
**	special processing is performed by the LG_MASTER. Subsequent slave
**	opens will detect that only one log is active and will open only that
**	log.
**
**	Testpoint(s) implemented:
**	    LG_T_PRIMARY_OPEN_FAIL	if set, causes open of primary/only
**					log file to fail.
**	    LG_T_DUAL_OPEN_FAIL		if set, causes open of dual log file
**					to fail.
**	    LG_T_FILESIZE_MISMATCH	if set, causes log files to appear to
**					be different sized
** Inputs:
**      flag                            LG_MASTER
**					    This is the MASTER process. Only one
**					    such is allowed, and it is given
**					    special privileges. In addition,
**					    LGopen will perform special checks
**					    upon the log file header to ensure
**					    it is consistent with the log file.
**					LG_SLAVE
**					    Not a 'real' flag. Any process other
**					    than the MASTER is a slave.
**					LG_ARCHIVER
**					    This is the ACP. At most one of
**					    these is around.
**					LG_LOG_INIT
**					    It's ok to open the log file even
**					    if it can't be read. This flag
**					    should be used only by RCPCONFIG.
**					LG_PRIMARY_ERASE
**					LG_DUAL_ERASE
**					    Only used in conjunction with 
**					    LG_LOG_INIT, these flags indicate
**					    that only the specified logfile
**					    is to be opened.
**					LG_CKPDB
**					    This is a checkpoint process.
**					LG_FCT
**					    This process is a server which can
**					    use Fast Commit on databases, hence
**					    if it dies, REDO may (will) be done.
**					LG_HEADER_ONLY
**					    This process wishes to read the
**					    primary log's log file header but
**					    does not plan to actually use the
**					    log system. Therefore, the log file
**					    scan need not be performed, and the
**					    log file header should NOT be
**					LG_FULL_FILENAME
**					    nodename contains a fully-qualified
**					    filename. Used by standalone
**					    logdump.
**					LG_IGNORE_BOF
**					    Used by logdump to dump entire log
**					    files in standalone processing.
**					    Directs the logging system to ignore
**					    the BOF in the log header and to
**					    scan the log file to find the log
**					    sequence number transition point
**					    to use as the starting BOF.
**                                      LG_PARTITION_FILENAMES
**                                          Used to indicate that nodename is an
**                                          array of LG_PARTITIONs, signifying that
**                                          the LGopen is to open a partitioned Logfile.
**                                          l_nodename will hold the number of partitons.
**
**	nodename			Pointer to file name or LG_PARTITION.
**	l_nodename			Length of file name or number of partitons.
**	block_size			Blocksize to open the file with.
**
** Outputs:
**      lg_id                           Log file identifier.
**	blks				Number of blocks allocated if called
**					by the master, dmfrcp.
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_RESOURCE		    No resources available.
**	    LG_NOTLOADED	    LG system is not available.
**	    LG_CANTOPEN		    Error opening log file.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	12-oct-1992 (bryanp)
**	    Only the master must be given the block_size arg to LGopen. Others can read
**	    the block_size from the LG_HEADER.
**	18-jan-1993 (bryanp)
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	16-feb-1993 (bryanp)
**	    When the master opens the local log, call LGlsn_init.
**	26-apr-1993 (bryanp)
**	    Set DI_SYNC_MASK when opening log files.
**	24-may-1993 (andys)
**	    Deal better with the case where the primary logfile
**	    has vanished by using the dual logfile to find the logfile
**	    blocksize. This allows us to (later) disable the primary 
**	    logfile correctly.
**	    Correct off by one error where DIsense returns page number
**	    and we want number of pages.
**	25-may-1993 (bryanp)
**	    If II_DUAL_LOG or II_DUAL_LOG_NAME isn't defined, this means the
**	    user isn't using dual logging. Don't complain, just use the primary.
**	21-jun-1993 (andys/bryanp)
**	    CSP/RCP process linkage: call LGc_csp_enqueue when opening log.
**	    Replace LG_NOHEADER with LG_LOG_INIT.
**	    If LG_LOG_INIT set, still try to open both logfiles...
**	    Initialize local_status to zero.
**	23-aug-1993 (andys)
**	    Correct number of args in ule_format(E_DMA47F_LGOPEN_SIZE_MISMATCH..
**	31-jan-1994 (jnash)
**	    Add block_size parameter to LG_verify_header() call.
**	25-Jan-1996 (jenjo02)
**	    Removed acquisition and freeing of lgd_mutex.
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Added code to handle multiple log partitions.
**	24-aug-1998 (hanch04)
**	    Partitioned Log File Project:
**	    Changed from log file names to log file locations.
**	28-Mar-2005 (jenjo02)
**	    For clusters, delay buffer allocation until after
**	    the log header has been tidied up; LG_allocate_buffers()
**	    presumes the header is valid. Also increase the number
**	    of buffer from 3 to the worst case, based on LG_MAX_RSZ.
**      13-jun-2005 (stial01)
**          For clusters, store node name in lfb
**	22-Nov-2005 (jenjo02)
**	    Add DI_DIRECTIO_MASK to open logs with
**	    directio when available.
**      30-Oct-2009 (horda03) Bug 122823
**          logdump standalone can be used to view a TX log file, but
**          then the TX log file is partitioned, need a way of supplying
**          the partiton file names. If LG_PARTITION_FILENAMES set, then
**          nodename and l_nodename will refer to the partiton filenames.
**	23-Jul-2008 (kschendel) SIR 122757
**	    Control log file directIO with direct_io_log param.
**	10-Feb-2010 (kschendel) SIR 122757
**	    Default for direct_io_log should be OFF.
*/
STATUS
LGopen(
u_i4               flag,
char                *nodename,
u_i4		    l_nodename,
LG_LGID		    *lg_id,
i4		    *blks,
i4		    block_size,
CL_ERR_DESC	    *sys_err)
{
    LG_HEADER	    header;
    LG_HEADER	    *hdr_ptr;
    LG_IO	    *primary_lg_io, *dual_lg_io;
    u_i4	    logs_opened = 0;
    i4	    blocks_partition, blocks_first;
    i4	    blocks_primary = 0;
    i4	    blocks_dual = 0;
    STATUS	    status;
    i4	    alter_args[2];
    i4	    lgd_active_log;
    i4	    lgd_status;
    i4	    length;
    i4	    test_val;
    i4	    log_to_disable;
    STATUS	    status_primary, status_dual;
    STATUS	    local_status = 0;
    bool	    disable_dual_logging = FALSE;
    char	    path [LG_MAX_FILE][128];
    char	    file [LG_MAX_FILE][128];
    char	    node [128];
    char	    alt_path [LG_MAX_FILE][128];
    char	    alt_file [LG_MAX_FILE][128];
    char	    *my_node;
    char	    *pm_str;
    i4		    l_path [LG_MAX_FILE];
    i4		    l_file [LG_MAX_FILE];
    i4		    alt_l_path [LG_MAX_FILE];
    i4		    alt_l_file [LG_MAX_FILE];
    i4		    i;
    i4              partition;
    i4		    sync_mask;
    i4	    err_code;
    LG_HDR_ALTER_SHOW	alter_struct;
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LFB	*lfb;

    CL_CLEAR_ERR(sys_err);

    if (flag & (LG_PRIMARY_ERASE | LG_DUAL_ERASE))
    {
	if (flag & LG_LOG_INIT)
	{
	    /*
	    ** rcpconfig is calling LGopen to open just one logfile.
	    ** Set the appropriate logfile and proceed.
	    */
	    flag |= LG_ONELOG_ONLY;
	    if (flag & LG_PRIMARY_ERASE)
		lgd_active_log = LG_PRIMARY_ACTIVE;
	    else
		lgd_active_log = LG_DUAL_ACTIVE;
	}
	else
	{
    	    return (LG_BADPARAM);
	}
    }
    if (flag & (LG_FULL_FILENAME|LG_LOG_INIT))
    {
	/*
	** If called by rcpconfig or standalone logdump then don't
	** check for dual logging etc., just open the specified logfile(s)
	*/
	if (flag & LG_FULL_FILENAME)
	{
	    /* 
	    ** If called by standalone logdump, then
	    ** open the indicated file, pretending it's the primary logfile.
	    */
	    lgd_active_log = LG_PRIMARY_ACTIVE;
	    flag |= LG_ONELOG_ONLY;
	}
    }
    else
    {
	/* See if dual logging is currently enabled. If it is,
	** we will attempt to open both log files. If it has
	** not been enabled, and we are the master, we will open the
	** primary (only) log file. If it has not been enabled, and we are
	** NOT the master, then perhaps it was previously enabled but has
	** been dynamically disabled, so we must ask the logging system
	** which log file to open.
	*/

	status = LGshow(LG_S_LGSTS, (PTR)&lgd_status, sizeof(lgd_status),
		    &length, sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    return(LG_CANTOPEN);
	}
	status = LGshow(LG_S_DUAL_LOGGING, (PTR)&lgd_active_log,
			    sizeof(lgd_active_log), &length, sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    return(LG_CANTOPEN);
	}
	if (((flag & LG_MASTER) != 0) && ((flag & LG_LOG_INIT) == 0))
	{
	    /* reset active log mask -- it was set wrong if not in rcpconfig */ 
	    lgd_active_log = LG_BOTH_LOGS_ACTIVE;
	}
	else
	{
	    /*
	    ** lgd_active_log is set to the 'right' log file to open.
	    */
	}
    }

    if (my_node = nodename)
    {
        if (! (flag & LG_PARTITION_FILENAMES) )
        {
	   /* File name build routines expect null terminated node name */
	   STncpy( node, nodename, l_nodename);
	   node[ l_nodename ] = '\0';
	   my_node = node;
        }

        if ((lfb = (LFB *)LG_allocate_cb(LFB_TYPE)) == 0)
        {
            return (LG_EXCEED_LIMIT);
        }
        /*
        ** Returns with lfb_mutex locked... will be unlocked in LG_open
        */
        primary_lg_io = &lfb->lfb_di_cbs[LG_PRIMARY_DI_CB];
        dual_lg_io = &lfb->lfb_di_cbs[LG_DUAL_DI_CB];
    }
    else
    {
        lfb = &lgd->lgd_local_lfb;
        primary_lg_io = &Lg_di_cbs[LG_PRIMARY_DI_CB];
        dual_lg_io = &Lg_di_cbs[LG_DUAL_DI_CB];

    }

    if ((flag & LG_FULL_FILENAME) == 0)
    {
	/*
	** Get the file name for the log file - 
	*/

	if (lgd_active_log == LG_BOTH_LOGS_ACTIVE)
	{
	    status = LG_build_file_name( LG_PRIMARY_LOG, my_node, 
				    file, l_file, path, l_path,
				    &primary_lg_io->di_io_count );
	    local_status = LG_build_file_name( LG_DUAL_LOG, my_node, 
				    alt_file, alt_l_file,
				    alt_path, alt_l_path,
				    &dual_lg_io->di_io_count );
	    if (local_status != OK && (flag & LG_MASTER) != 0)
	    {
		/*
		** We assumed they were using dual logging, but upon attempting
		** to build the path and file name for the dual log we
		** encountered an error. This means that in fact they are not
		** using dual logging, so the primary is the only active log.
		** This is not an error.
		*/
		local_status = OK;
		lgd_active_log = LG_PRIMARY_ACTIVE;
	    }
	}
	else if (lgd_active_log == LG_DUAL_ACTIVE)
	{
	    status = LG_build_file_name( LG_DUAL_LOG, my_node,
				    file, l_file, path, l_path,
				    &dual_lg_io->di_io_count );
	}
	else    /* not dual logging, and primary is only active log */
	{
	    status = LG_build_file_name( LG_PRIMARY_LOG, my_node,
				    file, l_file, path, l_path,
				    &primary_lg_io->di_io_count );
	}
	if (status || local_status)
	{
	    if (!status)
		status = local_status;
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    return (LG_CANTOPEN);
	}
    }
    else
    {
        if (flag & LG_PARTITION_FILENAMES) 
        {
           LG_PARTITION *part = (LG_PARTITION *) nodename;

           primary_lg_io->di_io_count = min(LG_MAX_FILE, l_nodename);

           for( partition = 0; partition < primary_lg_io->di_io_count; partition++ )
           {
              /* File name build routines expect null terminated node name */
              STncpy( node, part [partition].lg_part_name, part [partition].lg_part_len);
              node[ part [partition].lg_part_len ] = '\0';

              status = form_path_and_file(node,
				   file[partition], &l_file[partition],
				   path[partition], &l_path[partition]);

              if (status) break;
           }
        }
        else
        {
	   status = form_path_and_file(my_node,
				   file[0], &l_file[0],
				   path[0], &l_path[0]);
	   primary_lg_io->di_io_count = 1;
        }
    }

    if (block_size == 0 && (flag & LG_MASTER) == 0)
    {
	status = LGshow(LG_A_HEADER, (PTR)&header, sizeof(header), &length, 
		sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    return(LG_CANTOPEN);
	}
	block_size = header.lgh_size;
    }

    if (block_size == 0)
    {
	status = LG_get_log_page_size(path[0], file[0], &block_size, sys_err);
	/*
	** If both logs are active then it may be that the primary logfile
	** has vanished. Use the other logfile to get the blocksize.
	** This allows later sections of code to disable the primary log
	*/
	if (status && (lgd_active_log == LG_BOTH_LOGS_ACTIVE)
		   && ((flag & LG_FULL_FILENAME) == 0))
	    status = LG_get_log_page_size(alt_path[0], alt_file[0], 
			&block_size, sys_err);
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    return(LG_CANTOPEN);
	}
    }


    /*
    ** 'file' is now set to the correct log file to open: to the primary
    ** log file name if we'll be opening both log files, to the active log
    ** file name if we're in dual-logging-single-active-log mode, and to the
    ** sole log file name if we're not dual logging.
    */

    /*
    ** Roughly, the processing here is as follows:
    **	1) Open the primary log file, so long as it is active.
    **	2) If dual-logging, open the secondary log file
    **	3) Register our process and its log file channels with the device
    **	    driver (LG_open() ).
    **	4) If we are the log master, verify the header, EOF, and consistency
    **	    of the log file(s).
    */

    /* Transaction log runs in sync mode, also direct IO mode if requested.
    ** This is the only place that cares about the direct IO parameter, so
    ** just read it here rather than taking the long way 'round thru lgk.
    ** The setting is:
    ** ii.$.config.direct_io_log (ON or OFF, default is OFF)
    **
    ** This is a "config" setting, not a dbms or recovery setting, because
    ** direct I/O ought to apply to ALL dmf's, even the archiver, fastload,
    ** etc.  Failure to agree on direct I/O causes the OS extra work,
    ** and can expose a linux kernel bug in a few 2.6's.
    */
    sync_mask = DI_SYNC_MASK | DI_LOG_FILE_MASK;
    status = PMget("ii.$.config.direct_io_log", &pm_str);
    if (status == E_DB_OK)
    {
	if (STcasecmp(pm_str, "ON" ) == 0)
	    sync_mask |= DI_DIRECTIO_MASK;
    }
    /* ignore error status here */

    for (;;)
    {
	if (lgd_active_log != LG_DUAL_ACTIVE)
	{
	    /*
	    ** The primary log is active, so open it, after checking the LG
	    ** testpoint to see if we should simulate a failure for testing.
	    */
	    log_to_disable = DISABLE_NO_LOG;

	    length = LG_T_PRIMARY_OPEN_FAIL;
	    status_primary = LGshow(LG_S_TP_VAL, (PTR)&test_val,
				    sizeof(test_val), &length, sys_err);
	    if (status_primary == OK)
	    {
		if (test_val == 0)
		{
		    for (i = 0, blocks_first = 0; 
			 i < primary_lg_io->di_io_count && 
			     status_primary == OK;
			 i++)
		    {
			status_primary = DIopen(
				    &primary_lg_io->di_io[i],
				    path[i], l_path[i],
				    file[i], l_file[i],
				    block_size, DI_IO_WRITE,
				    sync_mask,
				    sys_err);
			if (status_primary == OK)
			    status_primary = DIsense(&primary_lg_io->di_io[i],
					    &blocks_partition, sys_err);
			if (status_primary == OK)
			{
			    /* All partitions must be the same block size */
			    if (blocks_first == 0)
				blocks_first = blocks_partition;

			    if (blocks_partition == blocks_first)
				/*
				** DIsense returns last page number; increment
				** to get number of pages.
				*/
				blocks_primary += blocks_partition + 1;
			    else
				status_primary = E_DMA810_LOG_PARTITION_MISMATCH;
			}
		    }

		    if (status_primary == OK && i > 0)
		    {
			logs_opened |= LG_PRIMARY_LOG_OPENED;
		    }
		    else
		    {
			uleFormat(NULL, status_primary, (CL_ERR_DESC *)sys_err,
				    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    }
		}
		else
		{
		    status_primary = LG_CANTOPEN; /* simulated failure */
		}
	    }
	}

	if (lgd_active_log != LG_PRIMARY_ACTIVE)
	{
	    /* Open the dual log.	    */

	    if (lgd_active_log == LG_BOTH_LOGS_ACTIVE)
		status_dual = LG_build_file_name(LG_DUAL_LOG, my_node,
						file, l_file, path, l_path,
						&dual_lg_io->di_io_count);
	    else
		status_dual = OK;

	    if (status_dual == OK)
	    {
		length = LG_T_DUAL_OPEN_FAIL;
		status_dual = LGshow(LG_S_TP_VAL, (PTR)&test_val,
				    sizeof(test_val), &length, sys_err);
	    }

	    if (status_dual == OK)
	    {
		if (test_val == 0)
		{
		    for (i = 0, blocks_first = 0; 
			 i < dual_lg_io->di_io_count && 
			     status_dual == OK;
			 i++)
		    {
			status_dual = DIopen(
				    &dual_lg_io->di_io[i],
				    path[i], l_path[i],
				    file[i], l_file[i],
				    block_size, DI_IO_WRITE,
				    sync_mask,
				    sys_err);
			if (status_dual == OK)
			    status_dual = DIsense(&dual_lg_io->di_io[i],
					    &blocks_partition, sys_err);
			if (status_dual == OK)
			{
			    /* All partitions must be the same block size */
			    if (blocks_first == 0)
				blocks_first = blocks_partition;

			    if (blocks_partition == blocks_first)
				/*
				** DIsense returns last page number; increment
				** to get number of pages.
				*/
				blocks_dual += blocks_partition + 1;
			    else
				status_dual = E_DMA810_LOG_PARTITION_MISMATCH;
			}
		    }
		    if (status_dual == OK && i > 0)
			logs_opened |= LG_DUAL_LOG_OPENED;
		    else
			uleFormat(NULL, status_dual, (CL_ERR_DESC *)sys_err,
				    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		}
		else
		{
		    status_dual = LG_CANTOPEN; /* simulated failure */
		}
	    }
	}

	if (lgd_active_log == LG_BOTH_LOGS_ACTIVE)
	{
	    /*
	    ** If both logs were active, but we managed to open only one of the
	    ** two files, then we must now remember that fact and gracefully
	    ** degrade to single-file logging.  If we could not get EITHER file
	    ** open, we must give up.  Note that if we got one file open but
	    ** not the other, we'll actually register both logs with LG_open(),
	    ** and will later (at the end of the routine) disable the one bad
	    ** log.
	    */
	    if (status_primary != OK && status_dual != OK)
	    {
		/* failed to open either file. Big time disaster. */
		uleFormat(NULL, status_dual, (CL_ERR_DESC *)sys_err,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		status = status_primary;
		break;
	    }
	    else if (status_primary)
	    {
		/* opened dual, but failed to open primary. */
		disable_dual_logging = TRUE;
		log_to_disable = DISABLE_PRIMARY_LOG;
	    }
	    else if (status_dual)
	    {
		/* opened primary, but failed to open dual */
		disable_dual_logging = TRUE;
		log_to_disable = DISABLE_DUAL_LOG;
	    }
	    else
	    {
		/*
		** both log files opened OK. Both logs must be the same size.
		*/
		length = LG_T_FILESIZE_MISMATCH;
		status = LGshow(LG_S_TP_VAL, (PTR)&test_val, sizeof(test_val),
				&length, sys_err);
		if (status == OK)
		{
		    if (test_val == 0)
		    {
			if (blocks_primary != blocks_dual)
			{
			    uleFormat(NULL, E_DMA47F_LGOPEN_SIZE_MISMATCH,
				    (CL_ERR_DESC *)NULL,
				    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
				    0, blocks_primary, 0, blocks_dual);
			    status = LG_LOGSIZE_MISMATCH;
			}
		    }
		    else
		    {
			status = LG_LOGSIZE_MISMATCH;
		    }
		}
		if (status)
		    break;

		flag |= LG_DUAL_LOG;
	    }
	}
	else if (lgd_active_log == LG_PRIMARY_ACTIVE)
	{
	    /*
	    ** Only the primary log was active. test if it was opened.
	    */
	    if (status_primary)
	    {
		status = status_primary;
		break;
	    }
	}
	else /* if (lgd_active_log == LG_DUAL_ACTIVE) */
	{
	    /*
	    ** Only the dual log was active. test if it was opened.
	    */
	    if (status_dual)
	    {
		status = status_dual;
		break;
	    }
	}
	/*
	** When we get here, (at least) one of the log file(s) has been
	** opened. If both have been opened, LG_DUAL_LOG is set in 'flag'.
	** If we will need to de-activate one of the logs, disable_dual_logging
	** is set to TRUE and log_to_disable is set to indicate which log
	** should be disabled.
	*/

	status = LG_open(flag, nodename, l_nodename, lfb,
			logs_opened, &hdr_ptr,
			(LG_ID *)lg_id, block_size, sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    break;
	}

	if ( CXcluster_enabled() )
	{
	    if ( NULL == my_node || '\0' == *my_node )
	    {
		STlcopy(CXnode_name(NULL), lfb->lfb_nodename, sizeof(lfb->lfb_nodename));
		lfb->lfb_l_nodename = STlen(lfb->lfb_nodename);
	    }
	    else
	    {
		STcopy(my_node, lfb->lfb_nodename);
		lfb->lfb_l_nodename = l_nodename;
	    }
	    CVupper(lfb->lfb_nodename);
	}

	if ((lgd_active_log == LG_BOTH_LOGS_ACTIVE) && disable_dual_logging)
	{
	    /*
	    ** If a problem was encountered opening one of the logs, failover
	    ** to the other log and close the bad log, if it was opened. From
	    ** here on out we'll use only 'channel_primary' and 'blocks_primary'
	    */
	    if (log_to_disable == DISABLE_PRIMARY_LOG)
	    {
#if 0
		if (channel_primary != -1)
		    (VOID) close( channel_primary );
		channel_primary = channel_dual;
		channel_dual = -1;
#endif
		blocks_primary = blocks_dual;
	    }
	    else /* log_to_disable == DISABLE_DUAL_LOG) */
	    {
#if 0
		if (channel_dual != -1)
		    (VOID) close( channel_dual );
		channel_dual = -1;
#endif
	    }
	}
	else if (lgd_active_log == LG_DUAL_ACTIVE)
	{
	    /*
	    ** Only the dual log was active, and so only it was opened and
	    ** registered with the driver. However, for ease of coding the
	    ** rest of LGopen, we'll now switch over to using just the variable
	    ** 'channel_primary' and 'blocks_primary'.
	    */
#if 0
	    channel_primary = channel_dual;
	    channel_dual = -1;
#endif
	    blocks_primary = blocks_dual;
	}

	if (status == OK && CXcluster_enabled() != 0)
	{
	    status = LGlsn_init(sys_err);
	}

	if ((flag & LG_MASTER) == 0)
	{
	    break;
	}

	/*
	** Only the LG_MASTER gets down to here. Return the file's size to
	** the master in units of min log block size (LGD_MIN_BLK_SIZE).
	*/

	if (blks)
	    *blks = blocks_primary * (block_size / LGD_MIN_BLK_SIZE);

	/*	Verify and return the log file header. */
    
	status = LG_verify_header(flag, block_size, 
		    blocks_primary * (block_size / LGD_MIN_BLK_SIZE),
		    log_to_disable, 
		    primary_lg_io, dual_lg_io,
		    logs_opened, hdr_ptr,
		    sys_err);

	if (status)
	{
	    if (flag & LG_DUAL_LOG)
	    {
		/*
		** We successfully opened both logs, but verify_header
		** instructs us that one of the logs is bad, so take note of
		** that fact and close the indicated log.
		**
		** NOTE: Currently, the driver assumes that if either of the
		** master's channels are non-(-1), then that channel can be
		** used for writing the log file. This means that we cannot
		** just close the master's channels without calling the driver
		** to reset its copies of those channels to -1. FIX_ME!
		*/
		if (status == LG_DISABLE_PRIM)
		{
		    disable_dual_logging = TRUE;
		    log_to_disable = DISABLE_PRIMARY_LOG;
#if 0
		    if (channel_primary != -1)
		    {
			(VOID) close( channel_primary );
			channel_primary = -1;
		    }
#endif
		    status = OK;
		}
		else if (status == LG_DISABLE_DUAL)
		{
		    disable_dual_logging = TRUE;
		    log_to_disable = DISABLE_DUAL_LOG;
#if 0
		    if (channel_dual != -1)
		    {
			(VOID) close( channel_dual );
			channel_dual = -1;
		    }
#endif
		    status = OK;
		}
		else
		    break;
	    }
	    else
	    {
		break;
	    }
	}

	/*
        **      Set the log file header again. All this really does is run some
	**	code in LGalter which does things like maintain the
	**	lfb_forced_lga field, etc. hdr_ptr is already pointing at the
	**	correct lfb_header.
	*/

	alter_struct.lg_hdr_lg_id = *lg_id;
	alter_struct.lg_hdr_lg_header = *hdr_ptr;
        status = LGalter(LG_A_NODELOG_HEADER, (PTR)&alter_struct,
			    sizeof(alter_struct), sys_err);
        if (status)
        {
            uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
                                    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
            status = LG_INITHEADER; 
            break;
        }

	/* 
	** Now that the header and LFB are consistent, allocate and 
	** initialize a sufficient number of buffers for node recovery.
	** LG_allocate_buffers() needs a correct header to properly
	** initialize the buffers.
	** This was being done prematurely in LG_open, before the
	** header was validated.
	*/

	if ( (nodename != 0) && lfb->lfb_header.lgh_size)
	{
	    status = LG_allocate_buffers(lgd, lfb,
				     lfb->lfb_header.lgh_size,
				     3 + (LG_MAX_RSZ /
					  (lfb->lfb_header.lgh_size -
					   sizeof(LBH))));
	    if (status)
		break;
	}

	break;
    }

    /*
    ** Error recovery: We need to distinguish between
    **	    1) complete success
    **	    2) Partial success (dual_logging was requested, but something
    **		is wrong with one of the logs)
    **		As part of the processing here, we must tell the device driver
    **		which log file to use. We return OK as long as we are able to
    **		notify the driver successfully.
    **	    3) Abject failure
    */
    if (status)
    {
#if 0
	/*
	** LGopen has failed. Close the log(s) if opened and return.
	*/
        TRdisplay("%@ LGopen: failing, returning status %d (%x)\n",
			status, status );
	TRdisplay("LGopen: log file open failed, errno %d\n", errno);

	if (channel_primary != -1)
	    (VOID) close( channel_primary );
	if (channel_dual != -1)
	    (VOID) close( channel_dual );
#endif
    }
    else if (disable_dual_logging)
    {
	/*
	** Translate from our local equates for log_to_disable into the
	** values expected by LGalter -- 0 means disable the primary log,
	** 1 means disable the dual log:
	*/
	if (log_to_disable == DISABLE_PRIMARY_LOG)
	    log_to_disable = 0;
	else
	    log_to_disable = 1;

	alter_args[0] = *lg_id;
	alter_args[1] = log_to_disable;
	status = LGalter(LG_A_DISABLE_DUAL_LOGGING, (PTR)alter_args, 
			sizeof(alter_args), sys_err);
    }

    return (status);
}

/*{
** Name: LG_open	- Open log file
**
** Description:
**      This routine creates the internal data structures that
**	describe a process opening a log file.  The caller that
**	passes the LG_MASTER flag is consider the owner of the
**	file.  It is the responsiblity of the master to assure the
**	consistency of the log information.
**
**	    (Actually, the notion of "master" is flawed and overloaded. Once
**	    the RCP is up and running, the RCP is the master of the logging
**	    system, and the RCP's state is tracked closely. The lgd_master_lpb
**	    field always points to the RCP's LPB, and various places in the
**	    logging system use this to locate the RCP's information. When the
**	    RCP is NOT up, things are more iffy. In particular, various
**	    processes such as rcpconfig, dmfcsp, and logdump may at times
**	    masquerade as the master in order to trick the logging system into
**	    allowing them to connect and open log files when the RCP is not up.
**	    In fact, the dmfcsp process may open multiple log files as the
**	    "master"; in fact, it may do this while the RCP is already up, in
**	    a node failure recovery scenario!
**
**	    We treat the LG_MASTER flag specially if a non-zero node name is
**	    passed, for this is the case where the dmfcsp process is opening a
**	    remote log. If this occurs before the RCP is up, then the dmfcsp
**	    process temporarily assumes the role of master; if it occurs after
**	    the RCP is up, then the dmfcsp does not disturb the RCP's proper
**	    role as master. Temporarily assuming the role of master is ugly,
**	    but it is necessary in order for the rest of the logging system to
**	    operate properly.)
**
**	If LG_DUAL_LOG is set in 'flag', then both log files have been opened
**	by this process. Otherwise, we assume that only one file (we don't
**	know whether primary or dual) has been opened.
**
**	We set lgd_active_log at this point to indicate the current active
**	log(s); this may be changed later either dynamically or through a
**	call to LGalter().
**
** Inputs:
**      flag                            Either:
**					    LG_SLAVE: Normal requestor.
**					    LG_ARCHIVER: Archiver process.
**					    LG_MASTER: Recovery process.
**					        (or rcpconfig or logdump...)
**					    LG_CKPDB: Ckpdb process.
**					    LG_DUAL_LOG : open the dual log.
**	nodename			Pointer to file name.
**	l_nodename			Length of file name.
**
** Outputs:
**      lg_id                           The assigned log indentifier.
**	Returns:
**	    LG_I_NORMAL			Successful.
**	    LG_I_BADPARAM		Bad parameter.
**	    LG_I_INSFMEM			Out of LPB's.
**	    LG_I_SHUT			Logging system shutdown.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) for rogerk
**	    Reduced logging project.  Removed references to obsolete 
**	    lgd->lgd_n_servers counter
**	23-oct-1992 (bryanp)
**	    I'm not sure how useful the LG_BADFILENAME check is, and it's not
**	    working correctly in a dual logging world (if the master opens both
**	    logs and sets lgd_filename to primary, then primary fails, then any
**	    subsequent server starts are refused because they try to pass the
**	    dual filename in and it doesn't match the primary filename). Thus,
**	    I have taken out the LG_BADFILENAME return for now.
**	26-apr-1993 (bryanp)
**	    Initialize lpb_gcmt_sid.
**	26-jul-1993 (bryanp)
**	    When the master opens a remote log, don't reset master_lpb if it's
**	    already set -- this was causing the CSP to overwrite master_lpb
**	    when it opened a remote log during node failure recovery. Instead,
**	    only the local master will set master_lpb.
**	26-jul-1993 (rogerk)
**	    Initialize new archive window fields.
**	18-oct-1993 (rogerk)
**	    Initialize new lpb_cache_factor field.
**	25-apr-1994 (bryanp) B61064
**	    Set lgk_mem.mem_creator_pid to the master's PID when the master
**		opens the log. This greatly reduces the problems associated
**		with the shared memory re-initialization code in LGK_initialize.
**		Under some circumstances, due to the timing of the various
**		subprocesses fired off by ingstart, the ingstart processing
**		completed leaving the installation up with the shared memory
**		owned by one of ingstart's subprocesses (one which was used to
**		run rcpstat, usually). By forcing the mem_creator_pid to be
**		set to the RCP's pid (the CSP's, in a cluster), we ensure that
**		the LGK_initialize re-formatting the shared memory code doesn't
**		accidentally fire.
**	23-may-1994 (bryanp) B58497
**	    Don't allow multiple local logfile opens with the same process PID.
**	25-Jan-1996 (jenjo02)
**	    Multiple semaphores replace singular lgd_mutex. Also relocated
**	    code linking new LPB into active queue until AFTER its been
**	    completely initialized:)
**	    Added lgd_archiver_lpb to hold pointer(offset) of Archiver's
**	    LPB.
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    We may now be opening multiple physical files per log.
**	27-may-2002 (devjo01)
**	    Zero lfb_buf_cnt when initializing LFB (cannot just zero entire
**	    structure, since certain portions (e.g. mutexes) are preserved
**	    between uses).
**	30-Apr-2003 (jenjo02)
**	    Removed 25-apr-1994 (bryanp) B61064 change to assign
**	    mem_creator_pid to the RCP's pid, no longer needed
**	    due to changes made for BUG 110121.
**	28-Mar-2005 (jenjo02)
**	    For clusters, delay buffer allocation until after
**	    the log header has been tidied up; LG_allocate_buffers()
**	    presumes the header is valid.
*/
/* ARGSUSED */
static STATUS
LG_open(
i4			flag,
PTR			nodename,
i4			l_nodename,
LFB			*lfb,
u_i4		logs_opened,
LG_HEADER		**hdr_ptr,
LG_ID			*lg_id,
i4			logpg_size,
CL_ERR_DESC		*sys_err)
{
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB	*lpb;
    SIZE_TYPE		end_offset;
    SIZE_TYPE		lpb_offset;
    LPB			*master_lpb;
    LPB			*next_lpb;
    STATUS		status;
    i4			initialize_lfb = FALSE;

    LG_WHERE("LG_open");

    /*
    ** Make a number of simple sanity tests:
    **	Have we already received an rcpconfig -shutdown request? Then no
    **		no processes may connect.
    **	Only one RCP at a time may be running, and no ordinary server
    **		processes may start up until the RCP is up.
    **	Only one Archiver at a time may be running.
    **	Only ordinary server processes may request Fast Commit.
    */

    if (lgd->lgd_status & (LGD_START_SHUTDOWN | LGD_IMM_SHUTDOWN))
    {
	return (LG_SHUTTING_DOWN);
    }

    /* Can only have one master, and must have one master. */

    if (lgd->lgd_master_lpb)
    {
	if ((flag & LG_MASTER) != 0 && nodename == 0)
	{
	    /*
	    ** The user tried to start two RCP's, or otherwise tried to start
	    ** an RCP while some other process was temporarily assuming the
	    ** role of log master.
	    */
	    return (E_DMA44B_LG_MULT_MASTER);
	}
	else
	{
	    master_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_master_lpb);
	    if (PCis_alive(master_lpb->lpb_pid) == FALSE)
	    {
		return(LG_NOMASTER);
	    }
	}
    }
    else if ((flag & LG_MASTER) == 0)
    {
	return(LG_NOMASTER);
    }

    /* Make sure only the slave process can request fast commit thread */
    if ((flag & LG_FCT) && (flag & LG_MASTER || flag & LG_ARCHIVER))
    {
	return (LG_BADFCT);
    }

    /* Make sure we can't have two archivers running at once */

    if ((flag & LG_ARCHIVER) && lgd->lgd_archiver_lpb)
    {
	return (LG_MULTARCH);
    }

    if (nodename == 0)
    {
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
	    return(status);
	end_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);
	for (lpb_offset = lgd->lgd_lpb_prev;
	    lpb_offset != end_offset;
	    lpb_offset = lpb->lpb_prev)
	{
	    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb_offset);
	    if (lpb->lpb_pid == LG_my_pid)
	    {
		(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);
		return (LG_MULTARCH);	/* not a very good message here */
	    }
	}
	(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);
    }

    /*
    **	Slaves can't connect until log is ONLINE.
    **	We used to check the filename here, but with the advent of dual logging
    **	a slave may open a different file than is stored in lgd_filename, so
    **	we no longer make that check.
    */

    if ((flag & (LG_MASTER | LG_ARCHIVER)) == 0 &&
	(lgd->lgd_status & LGD_ONLINE) == 0)
    {
	return (LG_NOT_ONLINE);
    }

    /*
    **  Allocate an LPB. Returns with lpb_mutex held.
    */
    if ((lpb = (LPB *)LG_allocate_cb(LPB_TYPE)) == 0)
    {
	return (LG_EXCEED_LIMIT);
    }

    /*	Initialize the rest of the LPB. */

    lpb->lpb_pid = LG_my_pid;
    lpb->lpb_status = LPB_ACTIVE;

    lpb->lpb_cond = LPB_CPREADY;
    lpb->lpb_lpd_next = lpb->lpb_lpd_prev =
			    LGK_OFFSET_FROM_PTR(&lpb->lpb_lpd_next);

    lpb->lpb_lpd_count = 0;
    lpb->lpb_bufmgr_id = 0;
    lpb->lpb_cache_factor = 0;
    lpb->lpb_gcmt_lxb = 0;
    lpb->lpb_stat.readio = 0;
    lpb->lpb_stat.write = 0;
    lpb->lpb_stat.force = 0;
    lpb->lpb_stat.begin = 0;
    lpb->lpb_stat.end = 0;
    lpb->lpb_stat.wait = 0;


    /*
    ** Determine whether this process will share in standard access to the
    ** local log or whether it has a process-specific connection to a private
    ** "node log". If the nodename was passed in, then this is a connection to
    ** a private log, and the LG_IO structures are copied into a freshly-
    ** allocated LFB. If this is just a connection to the local log, then the
    ** lgd's local LFB is used, and the LG_IO structures are copied into the
    ** known global variables.
    **
    ** When the LG_IO control blocks to be used are contained in the LFB, the
    ** lfb_status field has the LFB_USE_DIIO flag set. Otherwise, the proces
    ** uses its Lg_di_cbs[] control blocks to write the (local) log.
    */

    if (nodename == 0)
    {
	LG_logs_opened = logs_opened;
	lpb->lpb_lfb_offset = LGK_OFFSET_FROM_PTR(&lgd->lgd_local_lfb);

	if (flag & LG_MASTER)
	{
	    initialize_lfb = TRUE;
	    (VOID)LG_mutex(SEM_EXCL, &lfb->lfb_mutex);
	}
    }
    else
    {
	/*
	** lfb_mutex locked when it was allocated in LGopen...
	*/

	initialize_lfb = TRUE;
    }

    if (initialize_lfb)
    {
	*hdr_ptr = &lfb->lfb_header;
	lfb->lfb_type = LFB_TYPE;
	if (nodename == 0)
	{
	    /* logs_opened status is recorded in global variables */
	    lfb->lfb_status = 0;
	}
	else
	{
	    lfb->lfb_status = LFB_USE_DIIO;
	    if (logs_opened & LG_PRIMARY_LOG_OPENED)
		lfb->lfb_status |= LFB_PRIMARY_LOG_OPENED;
	    if (logs_opened & LG_DUAL_LOG_OPENED)
		lfb->lfb_status |= LFB_DUAL_LOG_OPENED;
	}
	lfb->lfb_forced_lga.la_sequence = 0;
	lfb->lfb_forced_lga.la_block    = 0;
	lfb->lfb_forced_lga.la_offset   = 0;
	lfb->lfb_forced_lsn.lsn_high = lfb->lfb_forced_lsn.lsn_low = 0;
	lfb->lfb_archive_window_start.la_sequence  = 0;
	lfb->lfb_archive_window_start.la_block     = 0;
	lfb->lfb_archive_window_start.la_offset    = 0;
	lfb->lfb_archive_window_end.la_sequence    = 0;
	lfb->lfb_archive_window_end.la_block       = 0;
	lfb->lfb_archive_window_end.la_offset      = 0;
	lfb->lfb_archive_window_prevcp.la_sequence = 0;
	lfb->lfb_archive_window_prevcp.la_block    = 0;
	lfb->lfb_archive_window_prevcp.la_offset   = 0;

	lfb->lfb_reserved_space = 0;

	lfb->lfb_current_lbb = 0;
	lfb->lfb_buf_cnt = lfb->lfb_fq_count = lfb->lfb_wq_count = 0;
	lfb->lfb_fq_next = lfb->lfb_fq_prev =
			LGK_OFFSET_FROM_PTR(&lfb->lfb_fq_next);

	lfb->lfb_wq_next = lfb->lfb_wq_prev =
			LGK_OFFSET_FROM_PTR(&lfb->lfb_wq_next);
    
	MEfill(sizeof(lfb->lfb_stat), 0, &lfb->lfb_stat);

	if (nodename != 0)
	{
	    /*
	    ** Defer buffer allocation until after the header
	    ** is fetched and validated.
	    */
	    lpb->lpb_lfb_offset = LGK_OFFSET_FROM_PTR(lfb);
	}

	/*
	** Set the dual logging information in the LFB based on which logs
	** were successfully opened by the caller. Later, the caller may analyze
	** the log headers and decide that one of the logs is not usable.
	*/
	switch (logs_opened)
	{
	    case (LG_DUAL_LOG_OPENED | LG_PRIMARY_LOG_OPENED):
		lfb->lfb_status |= LFB_DUAL_LOGGING;
		lfb->lfb_active_log = (LGD_II_LOG_FILE | LGD_II_DUAL_LOG);
		break;

	    case (LG_PRIMARY_LOG_OPENED):
		lfb->lfb_status &= ~(LFB_DUAL_LOGGING);
		lfb->lfb_active_log = LGD_II_LOG_FILE;
		break;

	    case (LG_DUAL_LOG_OPENED):
		lfb->lfb_status &= ~(LFB_DUAL_LOGGING);
		lfb->lfb_active_log = LGD_II_DUAL_LOG;
		break;

	    default:
		TRdisplay(
		    "%@ LG: unexpected logs_opened mask=%x\n", logs_opened);
		LG_deallocate_cb(LPB_TYPE, (PTR)lpb);
		if (lfb->lfb_status == LFB_USE_DIIO)
		    LG_deallocate_cb(LFB_TYPE, (PTR)lfb);
		return (LG_BADPARAM);
	}
	(VOID)LG_unmutex(&lfb->lfb_mutex);
    }

    if (flag & LG_MASTER)
    {
	lpb->lpb_status |= LPB_MASTER;
	if (lgd->lgd_master_lpb == 0)
	    lgd->lgd_master_lpb = LGK_OFFSET_FROM_PTR(lpb);
	
    }
    else if (flag & LG_ARCHIVER)
    {
	lpb->lpb_status |= LPB_ARCHIVER;
	lgd->lgd_status &= ~LGD_START_ARCHIVER;
	lgd->lgd_archiver_lpb = LGK_OFFSET_FROM_PTR(lpb);
    }
    else if (flag & LG_CKPDB)
    {
	lpb->lpb_status |= LPB_CKPDB;
    }
    else
    {
	if (flag & LG_FCT)
	{
	    lpb->lpb_status |= LPB_FCT;
	}
	else
	{
	    lpb->lpb_status |= LPB_SLAVE;
	}
    }

    /*	Queue LPB to active queue. */

    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
	return(status);
    lpb->lpb_next = lgd->lgd_lpb_next;
    lpb->lpb_prev = LGK_OFFSET_FROM_PTR(&lgd->lgd_lpb_next);
    next_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpb_next);
    next_lpb->lpb_prev = lgd->lgd_lpb_next = LGK_OFFSET_FROM_PTR(lpb);
    (VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

    /*	Change various counters. */

    *lg_id = lpb->lpb_id;

    /*
    ** Release the LPB mutex.
    */
    return(LG_unmutex(&lpb->lpb_mutex));
}

/*{
** Name: LG_verify_header	- Read , verify and rebuild the log header.
**
** Description:
**      This routine verifies that the log header is intact.  If not
**	it attempts to reconstruct relevant portions of the log header
**	by reading the log file.
**
**	This routine is invoked only by the log master (typically the RCP, but
**	also by RCPCONFIG in 'LG_LOG_INIT' mode).
**
**	If the system has only one active log, our job here is fairly straight
**	straightforward:
**	    1) Read the header, if possible. It's recorded twice.
**	    2) If we can't read the header, re-construct it.
**	    3) If the EOF position of the file is in doubt (due to a previous
**		system crash), scan the log file and determine the correct EOF.
**		If there is a partially-written log record at the end of the
**		log, back up to before that record.
**
**	If there are dual logs, then our job is complicated by the following:
**	    1) Header file analysis must check both log file headers, and must
**		use the one with the greater timestamp (the system may have
**		crashed after writing one but before writing the other).
**	    2) The header of the log file(s) may indicate that one of the log
**		files is disabled due to having encountered I/O errors. This
**		log file should then be dynamically disabled.
**	    3) The system may have crashed in the middle of writing one of the
**		log files (i.e., after writing one log but before/during
**		writing of the other log). In this case, we should complete the
**		partial write by copying the good record to the damaged file to
**		bring the files back into sync.
**	    4) During the header reads and EOF scan, an I/O error on one file
**		should dynamically fail-over to the other file. HOWEVER, if we
**		get I/O errors or bad check-sums on BOTH log files, we will
**		refuse to re-construct the header. This situation is judged to
**		be SO dire that header re-construction is not desirable. Note
**		that the headers are written synchronously, and thus we never
**		have an opening where a power failure/system crash could damage
**		both headers in ordinary processing.
**
**	LG_verify_header can be logically divided into three parts:
**	1) header reading and checking.
**	2) header re-construction (never performed for dual-logging).
**	3) EOF determination.
**
** Inputs:
**	flag		- the LGopen flags, possibly augmented. Most of the
**			  flags are ignored, but these are interesting:
**			LG_LOG_INIT - log file will be re-initialized. Don't
**				      scan the log file; it's not needed.
**			LG_DUAL_LOG - dual logging is active, and both log
**				      files have been opened.
**			LG_HEADER_ONLY - log files must be opened, but only the
**				      header need be processed.
**			LG_ONELOG_ONLY- only one logfile is being opened, so
**				      the dual logging cross-checks are not
**				      needed (this is used by standalone
**				      logdump).
**			LG_IGNORE_BOF - ignore the BOF value in the log header
**				      and always scan the log file to find
**				      the log sequence number transition point
**				      to use as the new BOF.
**
**	blocks		- size of the log file(s), in LGD_MIN_BLK_SIZE units.
**	blocksize	- log file block size, in bytes.
**	log_to_disable	- passed through to read_headers to allow it to
**			  perform better edit checking on the log file headers.
**
** Outputs:
**	header		- the (updated and verified) log file header.
**
** Returns:
**	OK		- all went well, header is usable.
**	LG_DISABLE_PRIM - primary log is unusable, and needs to be de-activated
**	LG_DISABLE_DUAL - dual log is unusable, and needs to be de-activated.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	21-jun-1993 (rogerk)
**	    Added LG_IGNORE_BOF flag to cause verify_header to do the logfile
**	    scan for bof_eof even if the logfile header status indicates that
**	    the bof/eof values in the header are valid.  This can be used by
**	    logdump to dump entire logfiles.
**	31-jan-1994 (jnash)
**	    Verify that log page size passed in (if any) matches that in log 
**	    file header.  Add blocksize parameter.
**	20-apr-1994 (mikem)
**	    bug #62034
**	    Fixed LG_verify_header to not return an error in the case of a
**	    mismatched blocksize between the on disk copy and the config.dat
**	    copy, if it was being called as part of "rcpconfig -init" (ie. to
**	    change the blocksize of the log file).
**	16-oct-1995 (nick)
**	    scan_bof_eof() now needs logs_opened passed to it. #71906
**	29-Jan-1998 (jenjo02)
**	    Partitioned log files:
**	    Test that number of partitions in the log header matches the
**	    number of log partitions opened.
*/
static STATUS
LG_verify_header(
i4		    flag,
i4	    	    blocksize,
u_i4	    blocks,
i4		    log_to_disable,
LG_IO		    *primary_lg_io,
LG_IO		    *dual_lg_io,
u_i4	    logs_opened,
LG_HEADER	    *header,
CL_ERR_DESC	    *sys_err)
{
    STATUS	    status, header_status;

    for (;;)
    {
	status = read_headers( flag, log_to_disable, primary_lg_io,
				dual_lg_io, logs_opened, header, sys_err );

	if (status == LG_WRONG_VERSION && flag & (LG_LOG_INIT|LG_HEADER_ONLY))
	{
	    /*
	    ** If the logfile headers appear to be the wrong logfile version,
	    ** but this is either logdump or rcpconfig -init, then we can
	    ** take an early return; no additional logfile processing is
	    ** required at this time.
	    */
	    status = OK;
	    break;
	}

	if (status == LG_RECONSTRUCT)
	{
	    status = reconstruct_header( flag, blocks, 
					primary_lg_io,
					header, sys_err);
	}
	else
   	{
	    /*
	    ** Verify that log page size and number of partitions is the same 
	    ** as configured, unless we are changing the log page size,
	    ** or reinitializing the log.
	    */
	    if ((blocksize != header->lgh_size) && 
		!(flag & (LG_LOG_INIT|LG_HEADER_ONLY)))

	    {
		TRdisplay("%@ LG: Log file header blocksize (%d) does not match configured blocksize (%d).\n", 
		    header->lgh_size, blocksize);
		status = E_DMA492_LGOPEN_BLKSIZE_MISMATCH;
		break;
	    }

	    /* Both primary and dual logs have the same number of partitions */
	    if (primary_lg_io->di_io_count != header->lgh_partitions)
	    {
		if ((flag & (LG_LOG_INIT | LG_HEADER_ONLY)) == 0)
		{
		    TRdisplay("%@ LG: Log file header partitions (%d) does not match configured partitions (%d).\n", 
			header->lgh_partitions, primary_lg_io->di_io_count);
		    status = E_DMA810_LOG_PARTITION_MISMATCH;
		    break;
		}
		header->lgh_partitions = primary_lg_io->di_io_count;
	    }
	}

	if (status)
	{
	    if (status == LG_DISABLE_PRIM || status == LG_DISABLE_DUAL)
	    {
		/*
		** if read_headers decides to disable dual logging, it must pass
		** that back to us. We must then update 'flag' and remember that
		** dual logging has been disabled so we can return it at the
		** end.
		*/
		TRdisplay("%@ LG: Log file header(s) indicate that %s is bad.\n",
			    (status == LG_DISABLE_PRIM) ? "II_LOG_FILE" :
			    "II_DUAL_LOG" );
		header_status = status;
		flag &= ~(LG_DUAL_LOG);
	    }
	    else
	    {
		TRdisplay("%@ LG: error (%x) reading log file headers.\n",
				status );
		break;
	    }
	}
	else
	{
	    header_status = OK;
	}

	/*
	**  If the header we found is in the recover state then just return.
	**  Recover state means that the CP, EOF and blocksize have already
	**  been determined on a previous restart failure.  If header is found
	**  in the EOF OK state then just return with the EOF and blocksize
	**  correct and the CP unknown.
	**
	**  If the LG_NOHEADER of LG_HEADER_ONLY flags are passed then bypass
	**  the bof_eof scan even if the header status indicates that the
	**  current values may not be valid.
	**
	**  If the LG_IGNORE_BOF flag is passed, then do the bof_eof scan even
	**  if the header status indicates that the current values are OK.
	*/

	if ((flag & (LG_LOG_INIT|LG_HEADER_ONLY)) ||
	    (((flag & LG_IGNORE_BOF) == 0) &&
		(header->lgh_status == LGH_RECOVER ||
		 header->lgh_status == LGH_EOF_OK ||
		 header->lgh_status == LGH_OK)))
	{
	    status = header_status;
	    break;
	}

	status = scan_bof_eof( flag, header, header_status, logs_opened,
				primary_lg_io, dual_lg_io, sys_err );

	TRdisplay("%@ LG: Completed machine failure log file scan.\n");

	if (status == OK && header_status != OK)
	{
	    status = header_status;
	}
	else if (header_status != OK)
	{
	    /*(
	    ** scan_bof_eof failed, and we want to disable dual logging as
	    ** well. This is a problematic case. When we scanned the logfile
	    ** headers, we detected that one of the logfiles was apparently
	    ** unusable, and we remembered which logfile to disable in
	    ** "header_status". However, the scan_bof_eof code has failed
	    ** as well, indicating that there is an additional problem with the
	    ** remaining logfile. We are unable to recover from both of these
	    ** failures, and hence we'll just have to return "status", which
	    ** will cause the RCP to fail to come up.
	    */
	    TRdisplay("%@ LG: Logfile header status was %x; returning %x\n",
			header_status, status);
	}

	break;
    }

    return (status);
}

/*
** Name: read_headers	- header reading and checking for LG_verify_header()
**
** Description:
**	This routine is called from LG_verify_header to read and check the
**	log file header(s).
**
**	If dual logging is enabled, we will check the headers to see if one
**	of the log files has encountered an I/O error in a previous lifetime.
**	If so, we will tell our caller to disable that log.
**
**	Note that if dual logging is enabled, LG_RECONSTRUCT will never be
**	returned. This is a conscious design decision.
**
**	If dual_logging is disabled, our work is simple:
**	    1) read the header from block 1 (virtual blocks count from 1)
**	    2) if that header can't be read or fails to checksum properly,
**		read the header from block 3
**	    3) if neither header could be read properly, return LG_RECONSTRUCT,
**		else check to see that the header says "active log is primary"
**		and then return OK.
**
**	If dual_logging is enabled, our work is more complicated:
**	    1) read the primary header from block 1 or 3 if possible.
**	    2) read the dual header from block 1 or 3 if possible.
**	    3) if both headers are OK, pick the one with higher timestamp and
**		return either OK or a disable code, depending on what the
**		header information says.
**	    4) if neither header was readable, return LG_CANTOPEN
**	    5) pick the header which was readable
**	    6) If the unusable header was due to an I/O error, disable that
**		log by returning the appropriate disable code, else return
**		OK (thus recovering, in effect, from a bad header write in the
**		previous crash.
**
**	If dual logging was previously enabled, but we were only able to read
**	one log file, then we proceed as for the single-logging case; however,
**	if the log file header indicates that only the "other" log file was
**	previously active, we return CANTOPEN. This case, a highly special
**	one, is encountered when the RCP cannot open one of the log files after
**	being brought back up following a period of dual logging. This is the
**	reason for the 'disabled log' parameter.
**
**	We attempt to verify that the version number field in the logfile
**	header matches the version number with which we were compiled. However,
**	the checksum algorithm uses the size of the LG_HEADER structure in its
**	checksum calculation. This means that if you change the size of the
**	LG_HEADER structure (for example, by adding a new member), then the new
**	code will compute a checksum using the new LG_HEADER size, but the
**	logfile headers written with the old LG_HEADER version will not
**	checksum properly. Therefore, attempts to use version numbers to catch
**	changes in the size of the structure are problematic. This is why we
**	check the version number even when the checksum fails.
**
** Inputs:
**	flag		    Many ignored flags, but in particular:
**			    LG_DUAL_LOG	    - dual logging is enabled, so read
**					      and check both log files.
**			    LG_HEADER_ONLY  - don't reconstruct header.
**			    LG_ONELOG_ONLY  - allow open of just 1 log file in
**					      a dual logging system (for
**					      standalone logdump usage).
**	disabled_log	    This field is only relevant if LG_DUAL_LOG is off,
**			    and serves to distinguish between 3 cases:
**			    DISABLE_NO_LOG	- we are truly single logging,
**				and the header should confirm this.
**			    DISABLE_PRIMARY_LOG	- we were previously dual
**				logging, but the II_LOG_FILE could not be
**				opened, and so the header should indicate
**				BOTH ACTIVE or DUAL ACTIVE -- PRIMARY ACTIVE
**				is an error.
**			    DISABLE_DUAL_LOG	- we were previously dual
**				logging, but the II_DUAL_LOG could not be
**				opened, and so the header should indicate
**				BOTH ACTIVE or PRIMARY ACTIVE -- DUAL ACTIVE
**				is an error.
**
** Outputs:
**	header		    the log file header to use
**
** Returns:
**	OK		    no problems encountered.
**	LG_RECONSTRUCT	    header is unusable, and must be re-constructed.
**	LG_DISABLE_PRIM	    header is ok, but primary log cannot be used.
**	LG_DIABLE_DUAL	    header is ok, but dual log cannot be used.
**	LG_CANTOPEN	    dual logging enabled, but neither header usable.
**			    OR, dual logging disabled, but primary log header
**			    indicates dual logging was ENabled.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	10-Nov-2009 (kschendel) SIR 122757
**	    Dynamically allocate (and align) the buffer for reading the
**	    header.  It's potentially large, and this is only done at
**	    open time.
*/
static STATUS
read_headers(
i4		flag,
i4		disabled_log,
LG_IO		*primary_lg_io,
LG_IO		*dual_lg_io,
u_i4	logs_opened,
LG_HEADER	*header,
CL_ERR_DESC	*sys_err)
{	
    char	    *abuf;		/* Aligned for i/o */
    char	    *buffer;
    LG_HEADER	    *bh;
    char	    *hdr_ptr;
    i4		    primary_ok;
    i4		    primary_io_errors;
    LG_HEADER	    header_primary;
    i4		    dual_ok;
    i4		    dual_io_errors;
    LG_HEADER	    header_dual;
    i4		    stamp_compare;
    STATUS	    status;
    i4		    one_page;
    i4		    size, align;
    i4	    err_code;
    LG_IO	    *lg_io;

    /* Allocate an aligned I/O buffer.  Note that we won't bother returning
    ** the buffer on all error-return paths;  an error return implies that
    ** the server isn't going to come up anyway.
    ** Don't have the log block size conveniently handy, use max.
    */
    size = LGD_MAX_BLK_SIZE;
    align = DIget_direct_align();
    if (align < DI_RAWIO_ALIGN)
	align = DI_RAWIO_ALIGN;
    buffer = MEreqmem(0, size+align, FALSE, &status);
    if (buffer == NULL)
    {
	TRdisplay("LG: Can't allocate %d bytes for reading log header\n",size+align);
	return (LG_CANTOPEN);
    }
    abuf = buffer;
    if (align != 0)
	abuf = ME_ALIGN_MACRO(abuf, align);
    bh = (LG_HEADER *) abuf;
    if ((flag & LG_DUAL_LOG) == 0)
    {
	/*
	** Not dual logging. This is the easy case
	*/
	if (logs_opened == LG_PRIMARY_LOG_OPENED)
	    lg_io = primary_lg_io;
	else
	    lg_io = dual_lg_io;

	for (;;)
	{
	    /*	Read the block containing the header. */

	    one_page = 1;
	    status = LG_READ(lg_io, &one_page,
			    (i4)0,
			    abuf, sys_err);
	    if (status)
		uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    if (status == OK)
	    {
		if (LGchk_sum(abuf, sizeof(LG_HEADER)) == bh->lgh_checksum)
		{
		    if (bh->lgh_version == LGH_VERSION)
		    {
			*header = *bh;
			break;
		    }
		}
		if (bh->lgh_version != LGH_VERSION)
		    return (LG_WRONG_VERSION);

		/* go to the part of the block containing the backup header. */

		hdr_ptr = &abuf[(LOG_HEADER_IO_SIZE * 2)];
		bh = (LG_HEADER *) hdr_ptr;

		if (LGchk_sum(hdr_ptr, sizeof(LG_HEADER)) == bh->lgh_checksum)
		{
		    if (bh->lgh_version == LGH_VERSION)
		    {
			*header = *bh;
			break;
		    }
		}
		if (bh->lgh_version != LGH_VERSION)
		    return (LG_WRONG_VERSION);
	    }
	    /*
	    ** Neither header could be read properly.
	    */
	    TRdisplay("%@ LG: Neither PRIMARY header could be read.\n");

	    if (flag & LG_HEADER_ONLY)
		return (LG_CANTOPEN);
	    else
		return (LG_RECONSTRUCT);
	}
	if ((flag & LG_LOG_INIT) != 0)
	{
	    (void) MEfree(buffer);
	    return (OK);		/* we don't care what the header says */
	}
	else if (disabled_log == DISABLE_NO_LOG)
	{
	    /*
	    ** We were told that dual logging is not active, so the header must
	    ** say PRIMARY ACTIVE only. However, if this is a cluster open,
	    ** we can bypass this check (e.g., standalone logdump uses this).
	    */
	    if (header->lgh_active_logs == LGH_PRIMARY_LOG_ACTIVE
	      || flag & LG_ONELOG_ONLY)
	    {
		(void) MEfree(buffer);
		return (OK);
	    }
	    else
	    {
		TRdisplay("%@ LG: Active logs are %v, but we expected PRIMARY\n",
			"PRIMARY,DUAL", header->lgh_active_logs );
		return (LG_CANTOPEN);
	    }
	}
	else if (disabled_log == DISABLE_PRIMARY_LOG)
	{
	    /*
	    ** We were told that dual logging was active, but that the primary
	    ** log could not be opened, so if the header says BOTH ACTIVE or
	    ** DUAL ACTIVE things are OK; otherwise the dual log is no good
	    ** and we cannot open it.
	    */
	    if (header->lgh_active_logs == LGH_DUAL_LOG_ACTIVE ||
		header->lgh_active_logs == LGH_BOTH_LOGS_ACTIVE)
	    {
		(void) MEfree(buffer);
		return (OK);
	    }
	    else
	    {
		TRdisplay("%@ LG: Active logs are %v, but we expected DUAL.\n",
			"PRIMARY,DUAL", header->lgh_active_logs );
		return (LG_CANTOPEN);
	    }
	}
	else /* (disabled_log == DISABLE_DUAL_LOG) */
	{
	    /*
	    ** We were told that dual logging was active, but that the dual
	    ** log could not be opened, so if the header says BOTH ACTIVE or
	    ** PRIMARY ACTIVE things are OK; otherwise the primary log is no
	    ** good and we cannot open it.
	    */
	    if (header->lgh_active_logs == LGH_PRIMARY_LOG_ACTIVE ||
		header->lgh_active_logs == LGH_BOTH_LOGS_ACTIVE)
	    {
		(void) MEfree(buffer);
		return (OK);
	    }
	    else
	    {
		TRdisplay("%@ LG: Active logs are %v, but we expected PRIM.\n",
			"PRIMARY,DUAL", header->lgh_active_logs );
		return (LG_CANTOPEN);
	    }
	}
	/*NOTREACHED*/
    }

    /*
    ** Fall-through means that this is the dual logging code
    */

    for (;;)
    {
	primary_ok = TRUE;
	primary_io_errors = 0;

	one_page = 1;
	bh = (LG_HEADER *) abuf;
	status = LG_READ(primary_lg_io, &one_page,
			    (i4)0,
			    abuf, sys_err);
	if (status)
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	if (status == OK)
	{
	    if (LGchk_sum(abuf, sizeof(LG_HEADER)) == bh->lgh_checksum)
	    {
		if (bh->lgh_version == LGH_VERSION)
		{
		    header_primary = *bh;  /* struct assign */
		    break;
		}
	    }
	    if (bh->lgh_version != LGH_VERSION)
		return (LG_WRONG_VERSION);
	    TRdisplay(
		"%@ LG: 1st PRIMARY header failed to checksum properly.\n");

	    /* go to the part of the block containing the backup header. */

	    hdr_ptr = &abuf[(LOG_HEADER_IO_SIZE * 2)];
	    bh = (LG_HEADER *) hdr_ptr;

	    if (LGchk_sum(hdr_ptr, sizeof(LG_HEADER)) == bh->lgh_checksum)
	    {
		if (bh->lgh_version == LGH_VERSION)
		{
		    header_primary = *bh;
		    break;
		}
	    }
	    if (bh->lgh_version != LGH_VERSION)
		return (LG_WRONG_VERSION);
	    TRdisplay(
		"%@ LG: 2nd PRIMARY header failed to checksum properly.\n");
	}
	else
	{
	    primary_io_errors++;
	}

	primary_ok = FALSE;
	break;
    }

    for (;;)
    {
	dual_ok = TRUE;
	dual_io_errors = 0;

	one_page = 1;
	bh = (LG_HEADER *) abuf;
	status = LG_READ(dual_lg_io, &one_page,
			    (i4)0,
			    abuf, sys_err);
	if (status)
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	if (status == OK)
	{
	    if (LGchk_sum(abuf, sizeof(LG_HEADER)) == bh->lgh_checksum)
	    {
		if (bh->lgh_version == LGH_VERSION)
		{
		    header_dual = *bh;  /* struct assign */
		    break;
		}
	    }
	    if (bh->lgh_version != LGH_VERSION)
		return (LG_WRONG_VERSION);
	    TRdisplay("%@ LG: 1st DUAL header failed to checksum properly.\n");

	    /* go to the part of the block containing the backup header. */

	    hdr_ptr = &abuf[(LOG_HEADER_IO_SIZE * 2)];
	    bh = (LG_HEADER *) hdr_ptr;

	    if (LGchk_sum(hdr_ptr, sizeof(LG_HEADER)) == bh->lgh_checksum)
	    {
		if (bh->lgh_version == LGH_VERSION)
		{
		    header_dual = *bh;  /* struct assign */
		    break;
		}
	    }
	    if (bh->lgh_version != LGH_VERSION)
		return (LG_WRONG_VERSION);
	    TRdisplay("%@ LG: 2nd DUAL header failed to checksum properly.\n");
	}
	else
	{
	    dual_io_errors++;
	}

	dual_ok = FALSE;
	break;
    }

    /* Return the I/O buffer since we don't need it (or bh) any more */
    (void) MEfree(buffer);

    if (primary_ok == FALSE && dual_ok == FALSE)
    {
	TRdisplay("%@ LG: Neither logfile header could be read.\n");
	return (LG_CANTOPEN);	    /* dire circumstances! */
    }

    if (primary_ok == TRUE && dual_ok == TRUE)
    {
	/*
	** pick the one with the higher timestamp and return either OK or
	** a disable code, depending on the info in the header
	*/
	if ((stamp_compare = 
		TMcmp_stamp((TM_STAMP *) header_primary.lgh_timestamp,
			    (TM_STAMP *) header_dual.lgh_timestamp)) >= 0)
	    *header = header_primary;   /* struct assign */
	else
	    *header = header_dual;	/* struct assign */

	/*
	** If we are opening the log files under the control of rcpconfig and
	** rcpconfig is preparing to re-write the headers, then we don't need
	** to check the previous header information regarding which log is
	** active. Instead we just return at this point and rcpconfig will
	** proceed on to re-write the headers by calling LGerase.
	*/
	if (flag & LG_HEADER_ONLY)
	{
	    return (OK);
	}

	if ((header->lgh_active_logs == LGH_BOTH_LOGS_ACTIVE))
	{
	    if (stamp_compare == 0)
	    {
		return (cross_check_logs( &header_primary, &header_dual ) );
	    }
	    else
	    {
		return (OK); /* both logs active, one header is old */
	    }
	}
	else if (header->lgh_active_logs == LGH_PRIMARY_LOG_ACTIVE)
	{
	    return (LG_DISABLE_DUAL);
	}
	else if (header->lgh_active_logs == LGH_DUAL_LOG_ACTIVE)
	{
	    return (LG_DISABLE_PRIM);
	}
	else
	{				/* No logs are active? */
	    TRdisplay("%@ LG: active log mask is not right (%x).\n",
			header->lgh_active_logs);
	    return (LG_CANTOPEN);
	}
    }

    if (primary_ok == TRUE)
    {
	TRdisplay("%@ LG: Using PRIMARY header since DUAL was unreadable.\n");
	*header = header_primary;   /* struct assign */

	/*
	** why was the dual unusable? I/O error or just a bad checksum?
	** if I/O error, return LG_DISABLE_DUAL. else return OK
	*/

	if (dual_io_errors > 0)
	    return (LG_DISABLE_DUAL);
	else
	    return (OK);
    }

    if (dual_ok == TRUE)
    {
	TRdisplay("%@ LG: Using DUAL header since PRIMARY was unreadable.\n");
	*header = header_dual;   /* struct assign */

	/*
	** why was the primary unusable? I/O error or just a bad checksum?
	** if I/O error, return LG_DISABLE_PRIM. else return OK
	*/
	if (primary_io_errors > 0)
	    return (LG_DISABLE_PRIM);
	else
	    return (OK);
    }

    /*
    ** NOTREACHED -- but complain if it is, just in case?
    */
    TRdisplay("%@ LG: Internal coding error in read_headers().\n");
    return (LG_CANTOPEN);
}

/*
** Name: reconstruct_header	- perform single-log header reconstruction.
**
** Description:
**	If LG_LOG_INIT is on, this routine is called with an empty log file
**	and should simply build a default LG_HEADER structure and return.
**
**	Otherwise, this routine is called to attempt recovery from a bad file.
**	When dual logging is not enabled, we run the risk that a power failure
**	or system crash during a header write may leave the header useless. If
**	that occurs, we attempt to re-construct the header information. For
**	most of the information, we can simply pick a sensible default. For
**	the log file page size, we make an attempt to deduce the log file
**	page size by testing each of the possible page sizes until we get one
**	that matches.
**
**	This routine is NOT currently called for dual logging. If dual logging
**	is enabled but neither file's header is readable, we decline to attempt
**	any reconstruction of the headers. This decision may need to be
**	re-thought.
**
** Inputs:
**	flag	    - if LG_LOG_INIT is on, just build default header.
**	blocks	    - size of the log file.
**
** Outputs:
**	header	    - log file header
**
** Returns:
**	OK	    - header rebuilt successfully
**	LG_READERROR- log file is unreadable.
**
** History:
**	5-jun-1990 (bryanp)
**	    Spun out of verify_header for dual_logging support.
**	5-jul-1990 (bryanp)
**	    Set lgh_timestamp and lgh_active_logs in default header.
**	12-oct-1990 (bryanp)
**	    Updates for Unix support.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	31-jan-1994 (mikem)
**	    Sir #57044
**	    Changed debugging logging of log file scans to print messages
**	    every 1000 buffers rather than every 100 buffers.  This makes
**	    the rcp log a little more readable.
**	31-jan-1994 (jnash)
**	    Fix lint detected problem: remove unused "channel" parameter. 
**	19-Apr-2005 (schka24)
**	    Fudge low half of tran-id forward from "now" in case tx had
**	    been running more than one per second; for Replicator.
**	10-Nov-2009 (kschendel) SIR 122757
**	    Make sure I/O buffer is aligned.
*/
static STATUS
reconstruct_header( 
u_i4		flag,
i4		blocks,
LG_IO		*lg_io,
LG_HEADER	*header,
CL_ERR_DESC	*sys_err)
{
    i4	    size = LGD_MIN_BLK_SIZE;
    i4	    cbsize = 0;
    i4	    bsize = 0;
    i4	    seq = 0;
    char	    *b;
    i4		    i,j;
    TM_STAMP	    stamp;
    u_i4	    sequence;
    STATUS	status;
    char	*buffer, *abuf;
    i4		bufsize, align;
    i4		one_page;
    i4	    loop_count;
    i4	    err_code;

    /* Allocate an aligned I/O buffer.
    ** Don't have the log block size conveniently handy, use max.
    */
    bufsize = LGD_MAX_BLK_SIZE;
    align = DIget_direct_align();
    if (align < DI_RAWIO_ALIGN)
	align = DI_RAWIO_ALIGN;
    buffer = MEreqmem(0, bufsize+align, FALSE, &status);
    if (buffer == NULL)
    {
	TRdisplay("%@ LG: Can't allocate %d bytes for log file reconstruction\n",bufsize+align);
	return (LG_CANTINIT);
    }
    abuf = buffer;
    if (align != 0)
	abuf = ME_ALIGN_MACRO(abuf, align);

    for (;;)
    {
	/*
	** Start by picking some 'reasonable' defaults.
	*/

	MEfill(sizeof(*header), 0, header);
	header->lgh_version = LGH_VERSION;
	header->lgh_checksum = 0;
	header->lgh_status = LGH_BAD;
	header->lgh_size = LGD_MAX_BLK_SIZE;
	header->lgh_count = blocks / (header->lgh_size / LGD_MIN_BLK_SIZE);
	header->lgh_l_logfull = .8 * blocks;
	header->lgh_l_abort = .9 * blocks;
	header->lgh_l_cp = .05 * blocks;
	header->lgh_cpcnt = 4;
	header->lgh_begin.la_sequence = 2;
	header->lgh_begin.la_block    = 0;
	header->lgh_begin.la_offset   = 0;
	header->lgh_cp.la_sequence    = 2;
	header->lgh_cp.la_block       = 0;
	header->lgh_cp.la_offset      = 0;
	header->lgh_end.la_sequence   = 2;
	header->lgh_end.la_block      = header->lgh_count - 1;
	header->lgh_end.la_offset     = 0;

	/* Fudge the low part of the tran-id forward by about 100 days.
	** That's the part that Replicator uses.  This is a (probably
	** pointless) attempt at ensuring that transaction ID's don't
	** run in reverse if a transaction log got reinitialized.
	*/
	sequence = TMsecs();
	header->lgh_tran_id.db_high_tran = sequence >> 16;
	header->lgh_tran_id.db_low_tran = sequence + 10000000;
	if (header->lgh_tran_id.db_low_tran < sequence)
	    ++ header->lgh_tran_id.db_high_tran;	/* Overflow */

	/* Set the number of partitions */
	header->lgh_partitions = lg_io->di_io_count;

	MEcopy((PTR)&stamp, sizeof(stamp), (PTR)header->lgh_timestamp);

	header->lgh_active_logs = LGH_PRIMARY_LOG_ACTIVE;

	if (flag & LG_LOG_INIT)
	    break;

	/*
	**  Start the process of rebuilding the header.  
	**
	**  The first step is to determine the block size used to write to
	**  the file.
	**
        ** The blocksize is the input to the checksum routine -- that is,
        ** the checksum routine returns a different value depending on
        ** the blocksize which is input to it. The idea in this section
        ** of code is that we read each block in the log, one at a time.
        ** For each such block, we try each of the possible block sizes,
        ** from smallest to largest. If at any instant we find a block
        ** and a blocksize where the block's checksum matches the LGchk_sum
        ** result for that blocksize, we have a potential blocksize. If
        ** we ever confirm that blocksize by observing a second correct
        ** checksum (on a subsequent block), then we accept that as the
        ** correct blocksize.
        **
        ** Variables:
        **    seq   - current position in the log file (we advance
        **            LGD_MAX_BLK_SIZE bytes at a time).
        **    cbsize- candidate blocksize, awaiting confirmation.
        **    bsize - the thing we're trying to calculate.
        **    size  - a blocksize variable.
        **    j     - the number of 'little' blocks to overlay on the
        **            current 'big' block.
        **    i     - the current 'little' block counter.
        **    b     - the current 'little' block pointer, used to overlay
        **            a potential little block on this big block.
	**
	** Initially, we assume the block size is LGD_MAX_BLK_SIZE.
	**
	** This algorithm HOPES to terminate by setting 'bsize' to a likely
	** blocksize. It may, however, read all the blocks in the file, at
	** which point 'seq' will become too big and we'll attempt an out of
	** bounds read and return with LG_READERROR. I don't think that this
	** has ever happened in practice. (one case in which it does: the
	** ENTIRE log file, including the header, is filled with binary 0's)
        */

	TRdisplay("%@ LG: Beginning logfile header reconstruction.\n");
	loop_count = 0;

	for (;;)
	{
	    if (loop_count++ > 1000)
	    {
		TRdisplay("%@ LG: 1000 iterations of block size analysis.\n");
		loop_count = 0;
	    }

	    /*  Read the next block for analysis. */

	    one_page = 1;
	    status = LG_READ(lg_io, &one_page,
			    seq,
			    abuf, sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		(void) MEfree(buffer);
		return(LG_READERROR);
	    }

	    /*
	    **	If there is an idea about the block size that needs more 
	    **	information from the next block to confirm, make this
	    **	check now. Figure out how many 'cbsize' blocks there are in
	    **	'buffer' and try checksumming each one. If any block matches,
	    **	then we will announce that 'cbsize' is the right blocksize.
	    **	If, however, none of the blocks checksum correctly, then
	    **	'cbsize' is no good as a candidate and should be discarded.
	    */

	    if (cbsize)
	    {
		j = LGD_MAX_BLK_SIZE / cbsize;

		for (b = abuf, i = 0; i < j; i++, b += cbsize)
		{
		    if (((LBH *)b)->lbh_checksum &&
			LGchk_sum(b, cbsize) == ((LBH *)b)->lbh_checksum)
		    {
			bsize = cbsize;	/* we have a winner */
			break;
		    }
		}
		if (bsize)
		    break;  /* we've decided to pick this blocksize. */
		cbsize = 0;
	    }

	    /*
	    **	At this point we have no candidate blocksize. Try each possible
	    **	blocksize (in 'size') from LGD_MIN_BLK_SIZE to LGD_MAX_BLK_SIZE.
	    **	For each such size, try each of the blocks of that size in
	    **	'buffer' (there are 'j' of them), and take that block's
	    **	checksum. If we get a matching checksum, try to verify it
	    **	with the next block in the file, if there is a next block.
	    **	If there is no next block, then call this size a candidate
	    **	blocksize ('cbsize') and proceed to read the next block.
	    */

	    for (j = LGD_MAX_BLK_SIZE / LGD_MIN_BLK_SIZE, size = LGD_MIN_BLK_SIZE;
	        size <= LGD_MAX_BLK_SIZE; j /= 2, size *= 2)
	    {
		for (b = abuf, i = 0; i < j; i++, b += size)
		{
		    if (((LBH *)b)->lbh_checksum &&
			    LGchk_sum(b, size) == ((LBH *)b)->lbh_checksum)
		    {
			if (i < j - 1)
			{
			    /* 
			    ** Checksum matches and there is a next block in
			    ** this buffer. If this is the right pagesize,
			    ** then the next block will begin with a block
			    ** header, and the header will have a log address
			    ** whose pagenumber and byte offset (in la_offset)
			    ** are 'reasonable' (>= our current page number,
			    ** which is (j*seq + i), plus 1).
			    */

			    if (((LBH *)(b + size))->lbh_address.la_block >=
			        (j * seq + i + 1))
			    {
			        bsize = size; /* yes, this is right blocksize */
			    }
			}
			else
			{
			    /*
			    ** we'll have to wait for the next physical block
			    ** to confirm this candidate. So just remember it.
			    */
			    cbsize = size;
			}
			break;
		    }
		}
		/*
		** If we've picked a blocksize, we're done. If we've found
		** a candidate blocksize (one where the checksum matches), then
		** we will stop processing possible blocksizes here and
		** proceed IMMEDIATELY to the next block to confirm or deny
		** this.
		*/
		if (bsize || cbsize)
		    break;
	    }
	    if (bsize)
		break;
	    seq++;
	}

	/*
	** If we get here, we've managed to deduce a reasonable blocksize.
	** update the header fields which are affected by the changed blocksize
	*/
	header->lgh_size = bsize;
	header->lgh_count *= (LGD_MAX_BLK_SIZE / header->lgh_size);
	header->lgh_end.la_block  = (header->lgh_count - 1);
	header->lgh_end.la_offset = 0;

	TRdisplay("%@ LG: After header reconstruction, blocksize set to %d\n",
		    bsize);

	break;
    }

    /* free up the memory */
    (void) MEfree(buffer);
    return (OK);
}

/*
** Name: scan_bof_eof	    - Determine the log file's true begin and end
**
** Description:
**	If the system crashes, or the RCP dies, the log file's header may be
**	out of sync with the log file contents. In particular, the log file's
**	header may have erroneous values for the begin or end of the log file.
**
** Inputs:
**	flag		    - flag values of interest:
**				LG_DUAL_LOG
**	header		    - current version of the log file header
**	header_status	    - OK -- primary is OK to use (and so is
**				dual, if (flag & LG_DUAL_LOG))
**			    - LG_DISABLE_PRIM -- use dual only.
**			    - LG_DISABLE_DUAL -- use prim only.
**	logs_opened	    - Either 0 or bit mask of ...
**				LG_DUAL_LOG_OPENED, LG_PRIMARY_LOG_OPENED
**
** Outputs:
**	header		    - updated with new BOF, EOF, and status values.
**
** Returns:
**	OK		    - header analysis completed successfully.
**	LG_DISABLE_PRIM	    - error encountered on primary log. disable it.
**	LG_DISABLE_DUAL	    - error encountered on dual log. disable it.
**	LG_CANTOPEN	    - log file has become unusuable.
**
** History:
**	5-jun-1990 (bryanp)
**	    Spun out of old verify_header and re-worked for dual_logging.
**	12-oct-1990 (bryanp)
**	    Updates for Unix support.
**	25-oct-1990 (bryanp)
**	    Added header_status argument and used it to set 'channel'
**	    correctly, so that scanning is done on the proper log file.
**	31-jan-1994 (mikem)
**	    Sir #57044
**	    Changed debugging logging of log file scans to print messages
**	    every 1000 buffers rather than every 100 buffers.  This makes
**	    the rcp log a little more readable.
**	15-feb-1994 (andyw)
**	    Removed iosb and from TRdisplay() as never used.
**	16-oct-1995 (nick)
**	    Added extra parameter logs_opened ; we can get here when the
**	    primary logfile failed to open but header_status is OK.  This
**	    would cause us to fail here as we attempted to use the primary
**	    log.  Only use the chosen log if logs_opened says it's open.
**	    #71906
**	20-Apr-2005 (jenjo02)
**	    Corrected test of "empty" page when scanning backwards;
**	    "offset" is la_offset, not la_block. Artifact of change to
**	    3-element log addresses back in '97.
**	10-Nov-2009 (kschendel) SIR 122757
**	    Make sure I/O buffer is aligned.
*/
static STATUS
scan_bof_eof(
u_i4		flag,
LG_HEADER	*header,
STATUS		header_status,
u_i4	logs_opened,
LG_IO		*primary_lg_io,
LG_IO		*dual_lg_io,
CL_ERR_DESC	*sys_err)
{
    i4	    eof_block;
    STATUS	return_status = OK;
    i4	    channel;
    i4	    seq;
    STATUS	status;
    i4	    loop_count;
    i4	    blocks_read;
    char	*buffer, *abuf;
    i4		size, align;
    i4		one_page;
    i4		err_code;
    LG_IO	*lg_io;

    /*
    ** There is one HIGHLY obscure case which must be noted here: if the
    ** header is valid but there's never been a consistency point, then the
    ** log file might be a freshly-initialized log file (the header was set to
    ** VALID by the RCP but the RCP died before ever writing any log pages.)
    ** In this case we can just set the BOF and EOF positions to indicate an
    ** empty log file (BOF == EOF == first byte of first page of log).
    **
    ** Until 2/93, we used to have LGerase completely wipe the logfile
    ** clear with zeros. This enabled us to test for a totally empty logfile
    ** by looking at the page we're on to see if the page was all zeros.
    ** We no longer wipe the logfile clean, so to test for a completely
    ** empty never written to logfile we now test for urine specimens.
    */
    if (header->lgh_end.la_block == 0 && header->lgh_begin.la_block == 0)
    {
	header->lgh_status = LGH_OK;

	TRdisplay("%@ LG: Logfile header is valid. No CP has been taken.\n");
	TRdisplay("  : Resetting status to LGH_OK; no recovery needed.\n");
	return (OK);
    }

    /*
    ** Figure out which channel to use. If primary is OK, use it,
    ** else we'll be scanning ONLY on dual (this occurs only when
    ** primary is recognized to be bad when the headers are read).
    **
    ** Assert: we set channel to dual if and only if:
    **		1) flag & LG_DUAL_LOG is OFF
    **		2) both log files were opened successfully
    **		3) read_headers returned LG_DISABLE_PRIM to verify_header.
    **
    ** Check that the logfile chosen has actually been opened ; if neither
    ** have then we probably wouldn't be here anyway but return LG_CANTOPEN
    ** just in case.
    */
    if ((header_status != LG_DISABLE_PRIM) &&
		(logs_opened & LG_PRIMARY_LOG_OPENED))
    {
	channel = LG_PRIMARY_DI_CB;
	lg_io = primary_lg_io;
    }
    else if ((header_status != LG_DISABLE_DUAL) &&
		(logs_opened & LG_DUAL_LOG_OPENED))
    {
	channel = LG_DUAL_DI_CB;
	lg_io = dual_lg_io;
    }
    else
    {
	/*
	** combination of header_status and logs_opened says nothing
	** is usable
	*/
	return (LG_CANTOPEN);
    }

    /* Allocate an aligned I/O buffer.
    ** Don't have the log block size conveniently handy, use max.
    */
    size = LGD_MAX_BLK_SIZE;
    align = DIget_direct_align();
    if (align < DI_RAWIO_ALIGN)
	align = DI_RAWIO_ALIGN;
    buffer = MEreqmem(0, size+align, FALSE, &status);
    if (buffer == NULL)
    {
	TRdisplay("LG: Can't allocate %d bytes for reading log header\n",size+align);
	return (LG_CANTOPEN);
    }
    abuf = buffer;
    if (align != 0)
	abuf = ME_ALIGN_MACRO(abuf, align);

    /*	Compute the block used to start searching for EOF. We start at the
    **	very first non-header page (page 0 contains the LG_HEADER, twice), or
    **	we start at the last Consistency Point, if that is known.
    */

    eof_block = 1;  /* first non-header block */

    if (header->lgh_status == LGH_VALID && header->lgh_cp.la_block)
    {
	/*
	** If the RCP was up and running previously, then there (hopefully)
	** was at least one good Consistency Point address. The EOF of the
	** log file is guaranteed to be no earlier than that position, so
	** start there (rounding down to the start of that page).
	*/
	eof_block = header->lgh_cp.la_block;

	TRdisplay(
    "%@ LG: Start of EOF scan: BOF:<%d,%d,%d> CP:<%d,%d,%d> EOF:<%d,%d,%d>\n",
		header->lgh_begin.la_sequence,
		header->lgh_begin.la_block,
		header->lgh_begin.la_offset,
		header->lgh_cp.la_sequence,
		header->lgh_cp.la_block,
		header->lgh_cp.la_offset,
		header->lgh_end.la_sequence,
		header->lgh_end.la_block,
		header->lgh_end.la_offset);
    }

    /*	Now read looking for the EOF. The log addresses on each log page are
    **	composed of two parts: the sequence number, and the physical address.
    **	The sequence number, (often called the "lap counter"), is incremented
    **	each time we wrap around the log file to the beginning. This means
    **	that the last log block written will have the property that the next
    **	log block in the file will have a LOWER sequence number. Thus, we
    **	read the log file until we find a place where the sequence number
    **	is less than the previous block. When we find that block, we have
    **	found the place 1 block past the EOF (i.e., we have to back up one
    **	block).
    */

    TRdisplay("%@ LG: Scan forward from block %d to locate true EOF.\n",
		eof_block);
    loop_count = 0;

    for (seq = header->lgh_begin.la_sequence;;)
    {
	if (loop_count++ > 1000)
	{
	    TRdisplay("%@ LG: 1000 iterations of EOF forward scan.\n");
	    loop_count = 0;
	}

	/*  Read the next block. */

	one_page = 1;
	status = LG_READ(lg_io, &one_page, eof_block, abuf, sys_err);
	if (status != OK)
	{
	    /*
	    ** Read operation failed. Attempt to failover to the dual log if
	    ** possible.
	    */
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    if ((flag & LG_DUAL_LOG) && channel == LG_PRIMARY_DI_CB)
	    {
		return_status = LG_DISABLE_PRIM;
		channel = LG_DUAL_DI_CB;
		lg_io = dual_lg_io;
		flag &= ~(LG_DUAL_LOG);
		TRdisplay("%@ LG: II_LOG_FILE read fail: blk=%d, stat=%x.\n",
			    eof_block + 1, status );
		continue;
	    }
	    else
	    {
		/*
		** Fatal error reading log.
		*/
		(void) MEfree(buffer);
		return (LG_CANTOPEN);
	    }
	}

	if (((LBH *)abuf)->lbh_address.la_sequence <= seq - 1)
	    break;

	seq = ((LBH *)abuf)->lbh_address.la_sequence;

	eof_block++;
	if (eof_block >= header->lgh_count )
	{
	    /*
	    ** Wrap around to the beginning of the log file.
	    */
	    eof_block = 1;
	    seq++;
	}
    }
    TRdisplay("%@ LG: Initial EOF scan completed.\n");

    /*
    ** At this point, we have a hypothesis about the EOF position (i.e., that
    ** it's the page just before the one pointed to by 'eof_block'). However,
    ** if dual logging was on and the system crashed after writing one log
    ** file but before writing the other log file, the two log files may
    ** actually have an 'uneven' EOF. The last block may have been written to
    ** primary, but not to dual, or written to dual, but not to primary.
    **
    ** If either of these have occurred, we can actually 'repair' the damage
    ** by copying that last block from one log file to the other. After we
    ** do so, we can advance our EOF pointer, if necessary, so that the
    ** EOF pointer still points just past the last written log page.
    */
    if (flag & LG_DUAL_LOG)
    {
	status = check_uneven_eof( header, seq, abuf,
				    &eof_block, primary_lg_io,
				    dual_lg_io, sys_err );
	if (status != OK)
	{
	    if (status == LG_DISABLE_PRIM)
	    {
		channel = LG_DUAL_DI_CB;
		lg_io = dual_lg_io;
		return_status = status;
		flag &= ~(LG_DUAL_LOG);
	    }
	    else if (status == LG_DISABLE_DUAL)
	    {
		return_status = status;
		flag &= ~(LG_DUAL_LOG);
	    }
	    else
	    {
		(void) MEfree(buffer);
		return ( status );
	    }
	}
    }

    TRdisplay("%@ LG: End of file has been located at block %d.\n",
		eof_block);

    /*
    ** At this point, 'eof_block' is pointing at the log page just AFTER the
    ** last written log page. Back up one page, wrapping around backwards to
    ** the end if needed, and (re)read that last written log page. Check that
    ** the log page was written correctly (i.e., that it checksums properly).
    ** If it doesn't checksum properly, it was partially written. If it
    ** checksums properly, but contains only part of a single log record, the
    ** recovery of the partial log record won't work. In either case, we keep
    ** reading backward until we find a page which was written correctly and
    ** does not contain only part of a single log record.
    */
    
    for (blocks_read = 0; blocks_read <= header->lgh_count; blocks_read++)
    {
	/*  Read back one block. */

	eof_block--;
	if (eof_block == 0)
	    eof_block = header->lgh_count - 1;

	one_page = 1;
	status = LG_READ(lg_io, &one_page, eof_block, abuf, sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    /*
	    ** Read operation failed. Attempt to failover to the dual log if
	    ** possible.
	    */
	    if ((flag & LG_DUAL_LOG) && channel == LG_PRIMARY_DI_CB)
	    {
		return_status = LG_DISABLE_PRIM;
		channel = LG_DUAL_DI_CB;
		lg_io = dual_lg_io;
		flag &= ~(LG_DUAL_LOG);
		TRdisplay(
		    "%@ LG: II_LOG_FILE read fail(2): blk=%d, stat=%x.\n",
			    eof_block + 1, status );
		continue;
	    }
	    else
	    {
		/*
		** Fatal error reading log.
		*/
		(void) MEfree(buffer);
		return (LG_CANTOPEN);
	    }
	}

	/*	Check that the whole block was written correctly. */

	if (LGchk_sum(abuf, header->lgh_size)==((LBH *)abuf)->lbh_checksum)
	{
	    /*
	    **	Keep reading backwards if this page only contains part of a 
	    **	single log record. The log address of the the last full record
	    **	written to the page is in 'lbh_address'. If the OFFSET portion
	    **	of the log address is zero, then either nothing was written to
	    **	the page or a very long log record that spans multiple pages
	    **	was written to the page. Either way we continue scanning back.
	    */

	    if ((((LBH *)abuf)->lbh_address.la_offset) == 0) 
	    {
		TRdisplay("%@ LG: Page <%d,%d,%d> was empty or incomplete.\n",
			((LBH *)abuf)->lbh_address.la_sequence,
			((LBH *)abuf)->lbh_address.la_block,
			((LBH *)abuf)->lbh_address.la_offset);
		continue;
	    }
	    /*
	    ** This block, which contains the address of the last completely
	    ** written log record in the log file, is the one we want as the EOF
	    */
	    header->lgh_end = ((LBH *)abuf)->lbh_address;
	    break;
	}
	else
	{
	    TRdisplay("%@ LG: Page <%d,%d,%d> was partially written.\n",
			((LBH *)abuf)->lbh_address.la_sequence,
			((LBH *)abuf)->lbh_address.la_block,
			((LBH *)abuf)->lbh_address.la_offset );
	}
    }
    if (blocks_read > header->lgh_count)
    {
	TRdisplay("%@ LG: Runaway loop ignoring partial log pages (%d > %d)\n",
		    blocks_read, header->lgh_count);
	(void) MEfree(buffer);
	return (LG_CANTOPEN);
    }

    /* Set the BOF to be one block away from the EOF for now. That is, one
    ** log page AFTER the EOF. Effectively, that means that the ENTIRE log
    ** is available for scans. The RCP may do some scans of its own and
    ** reset the beginning of file to some higher value, but for now we want
    ** to have the whole log file available for possible recovery use.
    **
    ** Note that even though we're AFTER the EOF, we get a lower sequence
    ** counter, thus we are a lower log address.
    */
    
    header->lgh_begin.la_sequence = header->lgh_end.la_sequence - 1;
    header->lgh_begin.la_block = header->lgh_end.la_block + 1;
    header->lgh_begin.la_offset = 0; 

    if (header->lgh_begin.la_block > (header->lgh_count - 1))
    {
	/*
	** wraparound to the beginning. Increment the seq #, thus resulting in
	** the same value for lgh_begin.la_sequence as for lgh_end.la_sequence. Same
	** sequence counter! The entire log file is in use, but BOF and EOF
	** are such that they have the same sequence number.
	*/
	header->lgh_begin.la_sequence++;
	header->lgh_begin.la_block = 1;
	header->lgh_begin.la_offset = 0;
    }

    /*
    ** Tell the RCP that we have certified the EOF position in the log file
    ** so that this work need not be re-done if the system crashes again
    ** while the RCP is recovering transactions.
    */
    header->lgh_status = LGH_EOF_OK;

    TRdisplay(
	"%@ LG: End of EOF scan: BOF:<%d,%d,%d> CP:<%d,%d,%d> EOF:<%d,%d,%d>\n",
		header->lgh_begin.la_sequence,
		header->lgh_begin.la_block,
		header->lgh_begin.la_offset,
		header->lgh_cp.la_sequence,
		header->lgh_cp.la_block,
		header->lgh_cp.la_offset,
		header->lgh_end.la_sequence,
		header->lgh_end.la_block,
		header->lgh_end.la_offset );

    /* Verify whether the last record is written completely. If this buffer is
    ** completely full and the address of the last completely-written log
    ** record is somewhere in the middle of the page, then the end of this
    ** page contains the first "chunk" of a partially written log record which
    ** we are ignoring. We will adjust the lbh_used and lbh_checksum values
    ** down in order to "erase" the partially written log record and then we
    ** rewrite the log page.
    */

    if (((LBH *)abuf)->lbh_used == header->lgh_size && 
	(((LBH *)abuf)->lbh_address.la_offset) <
	(header->lgh_size - sizeof(i4)) 
       )
    {
	/* 
	** Set the number of bytes used in the page to exclude the
	** imcomplete record.
	*/

	TRdisplay("%@ LG: Removing incomplete record from page <%d,%d,%d>.\n",
			((LBH *)abuf)->lbh_address.la_sequence,
			((LBH *)abuf)->lbh_address.la_block,
			((LBH *)abuf)->lbh_address.la_offset );

	((LBH *)abuf)->lbh_used = ((LBH *)abuf)->lbh_address.la_offset 
	    + sizeof(i4);

	((LBH *)abuf)->lbh_checksum = LGchk_sum(abuf, header->lgh_size);

	/*	Issue the QIO to write the block. */

	one_page = 1;
	status = LG_WRITE(lg_io, &one_page, eof_block, abuf, sys_err);
	if (status != OK)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    if ((flag & LG_DUAL_LOG) && channel == LG_PRIMARY_DI_CB)
	    {
		return_status = LG_DISABLE_PRIM;
		channel = LG_DUAL_DI_CB;
		lg_io = dual_lg_io;
		/* flag &= ~(LG_DUAL_LOG); (this is a special case) */
		TRdisplay("%@ LG: II_LOG_FILE write fail: blk=%d, stat=%x.\n",
			    eof_block + 1, status );
	    }
	    else
	    {
		/*
		** Fatal error writing log.
		*/
		(void) MEfree(buffer);
		return (LG_CANTOPEN);
	    }
	}

	if (flag & LG_DUAL_LOG)
	{
	    /* write to the dual log */
	    one_page = 1;
	    status = LG_WRITE(dual_lg_io, &one_page, eof_block, abuf, sys_err);
	    if (status != OK)
	    {
		uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		/*
		** Failed to write to dual, but primary is still OK
		*/
		flag &= ~(LG_DUAL_LOG);
		return_status = LG_DISABLE_PRIM; /* ?? DISABLE_DUAL ?? */
		TRdisplay("%@ LG: II_DUAL_LOG write fail: blk=%d, stat=%x.\n",
			    eof_block + 1, status );
	    }
	}
    }

    (void) MEfree(buffer);
    return (return_status);
}

/*
** Name: check_uneven_eof	- handle EOF inconsistencies in dual logging
**
** Description:
**	This routine is called to deal with recovering from failures in which
**	one log got written but the other did not. In the future this routine
**	might be expanded to deal with a much tougher problem if we ever
**	allow multiple pending I/O's on the logs.
**
**	At the point we are called we have located the apparent EOF in the
**	primary log file. There are 3 possibilities we must deal with:
**	1) The dual log has precisely the same EOF. In this case, all is fine,
**	    and we simply return to our caller.
**	2) The dual file was written further than the primary file. For some
**	    period of time before the system came down, the dual log was in
**	    use but the primary was not. In this case, we need to advance down
**	    the dual file until we reach its EOF. As we go, we must copy the
**	    dual pages over to the primary, thus completing write(s) which were
**	    not performed before the crash. There is the high probability that
**	    we will encounter an error writing the primary in this case, since
**	    might have disabled the primary just before the crash occurred.
**	3) The opposite of (2) -- the primary was written farther than the dual.
**
**	The algorithm as currently implemented allows for a multi-block
**	discrepancy between the primary and dual EOF positions, and repairs
**	such as discrepancy (by copying blocks from one log to the other), but
**	it CANNOT handle "holes" in the log file. That's OK -- we never have
**	multiple outstanding I/O's against a single log file, but you should
**	keep this limitation in mind.
**
**	We DO checksum some of the  pages we read in this routine. A page which
**	fails to checksum properly is treated as though it didn't get written
**	at all.  Not only does this simplify the logic, it may allow us to
**	recover from more I/O errors.
**
** Inputs:
**	header		    - current log header
**	seq		    - seq number of previous page of primary log.
**	buffer_primary	    - current log page from primary log
**	eof_block	    - current position in the primary log (i.e., block
**			      number of the page in the primary log which had
**			      a lower sequence number than the previous page)
**
** Outputs:
**	buffer_primary	    - overwritten. Don't count on its contents.
**	eof_block	    - possibly incremented to next log "page"
**
** Returns:
**	OK		    - EOF's have been reconciled
**	LG_DISABLE_PRIM	    - primary has become unusable, disable it
**	LG_DISABLE_DUAL	    - dual has become unusable, disable it.
**	LG_CANTOPEN	    - unrecoverable error
**
** History:
**	8-jun-1990 (bryanp)
**	    Began.
**	7-aug-1990 (bryanp)
**	    Enhancements to allow a multi-block discrepancy in the EOF to be
**	    detected and repaired.
**	5-sep-1990 (bryanp)
**	    Checksum some pages we read in this routine, and treat a page which
**	    doesn't checksum as though it didn't get written at all.
**	12-oct-1990 (bryanp)
**	    Updates for Unix support.
**	31-jan-1994 (mikem)
**	    Sir #57044
**	    Changed debugging logging of log file scans to print messages
**	    every 1000 buffers rather than every 100 buffers.  This makes
**	    the rcp log a little more readable.
**	15-feb-1994 (andyw)
**	    Removed iosb as this is never used/assigned
**	10-Nov-2009 (kschendel) SIR 122757
**	    Make sure I/O buffer is aligned.
*/
static STATUS
check_uneven_eof( 
LG_HEADER   *header,
i4	    seq,
char	    *buffer_primary,
i4	    *eof_block,
LG_IO	    *primary_lg_io,
LG_IO	    *dual_lg_io,
CL_ERR_DESC *sys_err)
{
    i4	status;
    i4	prev_block;
    STATUS	ret_status;
    i4	loop_count;
    char	*buffer_dual, *mem;
    i4		size, align;
    i4		one_page;
    i4	    err_code;

    /* Allocate an aligned I/O buffer.
    ** Don't have the log block size conveniently handy, use max.
    */
    size = LGD_MAX_BLK_SIZE;
    align = DIget_direct_align();
    if (align < DI_RAWIO_ALIGN)
	align = DI_RAWIO_ALIGN;
    mem = MEreqmem(0, size+align, FALSE, &status);
    if (mem == NULL)
    {
	TRdisplay("LG: Can't allocate %d bytes for reading log header\n",size+align);
	return (LG_CANTOPEN);
    }
    buffer_dual = mem;
    if (align != 0)
	buffer_dual = ME_ALIGN_MACRO(buffer_dual, align);

    /*
    ** Read this log page from the DUAL log and see if its seq # is > seq-1.
    ** If it is, then the DUAL got written but the PRIMARY did not, so
    ** copy that dual page over on top of this primary page and advance
    ** our EOF pointer to point to the next block. Continue advancing until the
    ** dual's sequence number drops.
    **
    ** If the DUAL's seq # is <= seq-1, then we want to read back one block
    ** from the dual and primary and see if we need to copy primary on
    ** top of dual. In this case, however, we don't need to reset the EOF
    ** pointer; it's correct as is. But we keep moving back and copying until
    ** we find a block where the dual and the primary match.
    */
    one_page = 1;
    status = LG_READ(dual_lg_io, &one_page, *eof_block, buffer_dual, sys_err);
    if (status)
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	/*
	** The EOF position on primary is fine. Just discard the dual log.
	*/
	TRdisplay("%@ LG: II_DUAL_LOG unreadable: blk=%d, stat=%x\n",
			    *eof_block + 1, status );
	(void) MEfree(mem);
	return (LG_DISABLE_DUAL);
    }


    if ((LGchk_sum(buffer_dual, header->lgh_size) ==
					((LBH *)buffer_dual)->lbh_checksum) &&
	((LBH *)buffer_dual)->lbh_address.la_sequence > (seq - 1))
    {
	/*
	** DUAL got written, primary did not. What we want to do is to
	** switch over to the dual and read along until we find the true EOF
	** on the dual. As we go, we'll write the dual pages back over to the
	** primary. We fully expect to encounter a write error on the primary
	** because a likely scenario for being here is that the primary
	** was dynamically disabled shortly before the system crashed, and we
	** were unable to stamp the header of the log indicating the disabling
	** of the primary before the crash.
	**
	** Our scan stops when we reach a DUAL page which has a lower sequence
	** number than primary.
	*/

	TRdisplay(
	    "%@ LG: Scan from block %d for true EOF on DUAL log file.\n",
		    *eof_block);
	ret_status = OK;
	loop_count = 0;

	for (;;)
	{
	    if (loop_count++ > 1000)
	    {
		TRdisplay("%@ LG: 1000 iterations reading forward on DUAL.\n");
		loop_count = 0;
	    }

	    TRdisplay("%@ LG: Re-writing log page <%d,%d,%d> to PRIMARY log.\n",
			((LBH *)buffer_dual)->lbh_address.la_sequence,
			((LBH *)buffer_dual)->lbh_address.la_block,
			((LBH *)buffer_dual)->lbh_address.la_offset);

	    one_page = 1;
	    status = LG_WRITE(primary_lg_io, &one_page,
                    *eof_block, buffer_dual, sys_err);

	    *eof_block++;
	    if (*eof_block >= header->lgh_count)
		*eof_block = 1;

	    if (status)
	    {
		/*
		** primary suddenly developed a WRITE error! disable it, but
		** advance EOF for dual and let dual keep cranking.
		*/

		uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		ret_status = LG_DISABLE_PRIM;
		TRdisplay("%@ LG: II_LOG_FILE write error: blk=%d, stat=%x.\n",
			    *eof_block + 1, status );
	    }

	    one_page = 1;
	    status = LG_READ(dual_lg_io, &one_page,
                    *eof_block, buffer_dual, sys_err);
	    if (status)
	    {
		/*
		** Gack! Now we can't read DUAL. If this is the first problem
		** encountered in this scan, then we can fail over to primary
		** and be done with things. Else, we have encountered I/O
		** errors on both primary and dual and must refuse to open the
		** log file.
		*/
		uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		if (ret_status == OK)
		    ret_status = LG_DISABLE_DUAL;
		else
		    ret_status = LG_CANTOPEN;
		break;
	    }

	    /*
	    ** We don't checksum this buffer, we just check its sequence
	    ** number. If there's been a partial DUAL write, then it won't
	    ** checksum, but we'll copy it over to PRIMARY anyway, then the
	    ** next buffer will have a lower sequence number and we'll break.
	    */

	    if (((LBH *)buffer_dual)->lbh_address.la_sequence <= (seq - 1))
	    {
		/*
		** Successful end of the forward scan along the DUAL log.
		*/
		break;
	    }
	    /*
	    ** loop back around, write this block to primary, and advance EOF
	    */
	}

	TRdisplay("%@ LG: Completed EOF reconciliation at block %d.\n",
		*eof_block);

	/*
	** EOF's are reconciled, and eof_block has been advanced.
	*/
	(void) MEfree(mem);
	return (ret_status);
    }

    /*
    ** Fall through to here means that the primary was written as far or
    ** farther than the dual. We read back one block on both files to see if the
    ** previous block was written to PRIMARY but not to DUAL. If it was, then
    ** we scan backwards, copying blocks from primary to dual, until we find
    ** a place where they match. During the scan, we expect a high probability
    ** of an I/O error on DUAL. This scan differs in two basic ways from the
    ** forward scan: (1) we're scanning backward, and (2) we don't change the
    ** EOF pointer as we go.
    **
    ** If, on a given page, one of the log files checksums properly but the
    ** other does not, then we need to copy the correctly-written page over
    ** to the incorrectly-written file.
    */
    prev_block = *eof_block;
    TRdisplay("%@ LG: Beginning EOF check from block %d on DUAL log.\n",
		prev_block);
    loop_count = 0;

    for (;;)
    {
	if (loop_count++ > 1000)
	{
	    TRdisplay("%@ LG: 1000 iterations analyzing DUAL EOF position.\n");
	    loop_count = 0;
	}

	prev_block--;
	if (prev_block == 0)
	    prev_block = header->lgh_count - 1;

	if (prev_block == *eof_block)
	{
	    /*
	    ** What? We have scanned the ENTIRE log file in this loop? This
	    ** cannot be right. We should have encountered a place where the
	    ** primary and the dual matched. Give up and disable the dual.
	    */
	    TRdisplay("%@ LG: Internal error -- entire DUAL log was scanned.\n");
	    (void) MEfree(mem);
	    return (LG_DISABLE_DUAL);
	}

	one_page = 1;
	status = LG_READ(dual_lg_io, &one_page,
                    prev_block, buffer_dual, sys_err);
	if (status)
	{
	    /*
	    ** The EOF position on primary is fine. Just discard the dual log.
	    */
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    TRdisplay("%@ LG: Internal error -- DUAL log cannot be read.\n");
	    (void) MEfree(mem);
	    return (LG_DISABLE_DUAL);
	}

	one_page = 1;
	status = LG_READ(primary_lg_io, &one_page,
                    prev_block, buffer_primary, sys_err);
	if (status)
	{
	    /*
	    ** VERY unexpected...we were able to read this same block just a
	    ** little while ago during our EOF scan, but now it's unreadable?
	    ** No sense in trying to fault over to the dual, this set of log
	    ** files STINKS!!! (If we tried to fault over to the dual at this
	    ** point, we'd need to re-do our EOF scan, since the original value
	    ** of *eof_block came from the primary scan.)
	    */
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    TRdisplay(
		"%@ LG: Internal error -- PRIMARY log became unreadable.\n");
	    (void) MEfree(mem);
	    return (LG_CANTOPEN);
	}

	/*
	** If the seq number in the dual is less than the seq number in the
	** prim, then the primary got written but the dual did not. Copy the
	** primary block to the dual.
	*/
	if ( ((LBH *)buffer_dual)->lbh_address.la_sequence <
				((LBH *)buffer_primary)->lbh_address.la_sequence)
	{
	    TRdisplay("%@ LG: Re-writing log page <%d,%d,%d> to DUAL log.\n",
			((LBH *)buffer_primary)->lbh_address.la_sequence,
			((LBH *)buffer_primary)->lbh_address.la_block,
			((LBH *)buffer_primary)->lbh_address.la_offset);

	    status = LG_WRITE(dual_lg_io, &one_page,
                        prev_block, buffer_primary, sys_err);
	    if (status)
	    {
		/*
		** Dual has a WRITE error. EOF is ok, just disable DUAL.
		*/
		uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		TRdisplay("%@ LG: Internal error -- DUAL log write error.\n");
		(void) MEfree(mem);
		return (LG_DISABLE_DUAL);
	    }
	    /*
	    ** Now loop around and try again. We want to keep going until we
	    ** encounter a place where the primary and the dual match.
	    */
	    continue;
	}
	/*
	** Once we get here, we have scanned backward in the DUAL log to locate
	** a place where the primary and dual logs appear to match. At this
	** point, they should have IDENTICAL lbh addresses. If so, we announce
	** successful completion of EOF reconciliation. If the addresses
	** are NOT identical, then we have detected a dual logging corruption
	** and we disable the DUAL log.
	*/
	if ( (((LBH *)buffer_dual)->lbh_address.la_sequence !=
	     ((LBH *)buffer_primary)->lbh_address.la_sequence) ||
	         (((LBH *)buffer_dual)->lbh_address.la_block !=
		 ((LBH *)buffer_primary)->lbh_address.la_block ) ||
			(((LBH *)buffer_dual)->lbh_address.la_offset !=
			((LBH *)buffer_primary)->lbh_address.la_offset))
	{
	    TRdisplay("%@ LG: Dual log file is corrupt.\n");
	    TRdisplay("  : Primary <%d,%d,%d> != Dual <%d,%d,%d>\n",
			((LBH *)buffer_primary)->lbh_address.la_sequence,
			((LBH *)buffer_primary)->lbh_address.la_block,
			((LBH *)buffer_primary)->lbh_address.la_offset,
			((LBH *)buffer_dual)->lbh_address.la_sequence,
			((LBH *)buffer_dual)->lbh_address.la_block,
			((LBH *)buffer_dual)->lbh_address.la_offset);

	    (void) MEfree(mem);
	    return (LG_DISABLE_DUAL);
	}
	/*
	** log buffer addresses match, so we have reached end of our scan.
	** If both buffers checksum correctly, we have nothing more to do.
	** If only one of the buffers checksums correctly, we must copy that
	** buffer over to the file which was partially written. That way, we
	** ensure that the two log files have identical, correctly-checksumming
	** copies of this page.
	**
	** If neither buffer checksums correctly, then both logs were partially
	** written. In this case, neither page is preferable to the other; our
	** caller will discard this page in any case and will reset EOF to the
	** previous log page, so we do not copy either page.
	*/
	if ((LGchk_sum(buffer_dual, header->lgh_size) ==
					((LBH *)buffer_dual)->lbh_checksum) &&
	    (LGchk_sum(buffer_primary, header->lgh_size) ==
					((LBH *)buffer_primary)->lbh_checksum))
	{
	    break;	/* nothing more to do */
	}
	else if (LGchk_sum(buffer_dual, header->lgh_size) !=
					((LBH *)buffer_dual)->lbh_checksum &&
		 LGchk_sum(buffer_primary, header->lgh_size) ==
					((LBH *)buffer_primary)->lbh_checksum)
	{
	    /*
	    ** dual was written partially, and primary was written completely.
	    ** Fix up dual by writing the primary block there.
	    */
	    TRdisplay(
		"%@ LG: Completing partial log page <%d,%d,%d>, DUAL log.\n",
			((LBH *)buffer_primary)->lbh_address.la_sequence,
			((LBH *)buffer_primary)->lbh_address.la_block,
			((LBH *)buffer_primary)->lbh_address.la_offset);

	    status = LG_WRITE(dual_lg_io, &one_page,
                        prev_block, buffer_primary, sys_err);
	    if (status)
	    {
		/*
		** Dual has a WRITE error. EOF is ok, just disable DUAL.
		*/
		uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		TRdisplay("LG: Internal error -- DUAL log write error.\n");
		(void) MEfree(mem);
		return (LG_DISABLE_DUAL);
	    }
	    break;
	}
	else if (LGchk_sum(buffer_primary, header->lgh_size) !=
				    ((LBH *)buffer_primary)->lbh_checksum &&
		 LGchk_sum(buffer_dual, header->lgh_size) ==
					((LBH *)buffer_dual)->lbh_checksum)
	{
	    /*
	    ** primary was written partially, and dual was written completely.
	    ** Fix up primary by writing the dual block there.
	    */
	    TRdisplay("%@ LG: Complete partial page <%d,%d,%d>, PRIMARY log.\n",
			((LBH *)buffer_dual)->lbh_address.la_sequence,
			((LBH *)buffer_dual)->lbh_address.la_block,
			((LBH *)buffer_dual)->lbh_address.la_offset);

	    status = LG_WRITE(primary_lg_io, &one_page,
                        prev_block, buffer_dual, sys_err);
	    if (status)
	    {
		/*
		** VERY unexpected...we were able to read this same block just
		** a little while ago during our EOF scan, but now it's
		** unwriteable? No sense in trying to fault over to the dual,
		** this set of log files STINKS!!! (If we tried to fault over
		** to the dual at this point, we'd need to re-do our EOF scan,
		** since the original value of *eof_block came from the primary
		** scan.)
		*/
		uleFormat(NULL, status, (CL_ERR_DESC *)sys_err,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		TRdisplay("LG: Internal error -- PRIMARY log write failure.\n");
		(void) MEfree(mem);
		return (LG_CANTOPEN);
	    }
	    break;
	}
	/*
	** The 4th case is that neither buffer checksums. Do no copying, just
	** break, for we have reached the correctly EOF position.
	*/
	break;
    }

    /*
    ** EOF's are reconciled.
    */
    TRdisplay("%@ LG: Completed EOF reconciliation at block %d.\n",
		prev_block);

    (void) MEfree(mem);
    return (status);
}

/*{
** Name: cross_check_logs	- sanity checking when dual logging is enabled
**
** Description:
**	This routine is called when the LG_MASTER opens the logging system
**	with dual logging support enabled. We have opened both files and have
**	already verified that the files are the same size; the point of this
**	routine is to make more sophisticated internal checks. We do the
**	following:
**
**	As a safety check, we demand that the following fields be
**	    identical in the two headers:
**
**	    lgh_version	    ( version of the log file )
**	    lgh_size	    ( log file page size      )
**	    lgh_count	    ( number of pages in log  )
**	    lgh_status	    ( log file status         )
**	    lgh_tran_id	    ( last transaction ID used)
**	    lgh_begin	    ( last begin of file      )
**	    lgh_end	    ( last end of file        ) -- checked in (1)
**	    lgh_cp	    ( last consistency point  )
**	    lgh_active_logs ( mask of active logfiles )
**
**	    If the two log files differ in any of these, something must be
**	    DREADFULLY wrong!
**
**	    These values may not be *correct*, but they should be the same.
**	    Subsequent code will potentially update the header.
**
** Inputs:
**	header_primary	    - LG_HEADER from the primary log file
**	header_dual	    - LG_HEADER from the dual log file
**
** Outputs:
**	None
**
** Returns:
**	OK		    - headers cross-check without error
**	LG_DUALLOG_MISMATCH - headers don't match. Something is hosed.
**
** History:
**	31-may-1990 (bryanp)
**	    Created.
*/
static STATUS
cross_check_logs( LG_HEADER *header_primary, LG_HEADER	*header_dual )
{
    /*
    ** Cross check certain critical fields in the log headers:
    */
    if (header_primary->lgh_version != header_dual->lgh_version ||
	header_primary->lgh_size    != header_dual->lgh_size    ||
	header_primary->lgh_count   != header_dual->lgh_count   ||
	header_primary->lgh_status  != header_dual->lgh_status  ||
	MEcmp((PTR)&header_primary->lgh_tran_id,
	      (PTR)&header_dual->lgh_tran_id,
	      sizeof(header_dual->lgh_tran_id) ) != 0     ||
	MEcmp((PTR)&header_primary->lgh_begin,
	      (PTR)&header_dual->lgh_begin,
	      sizeof(header_dual->lgh_begin) ) != 0       ||
	MEcmp((PTR)&header_primary->lgh_end,
	      (PTR)&header_dual->lgh_end,
	      sizeof(header_dual->lgh_end) ) != 0         ||
	MEcmp((PTR)&header_primary->lgh_cp,
	      (PTR)&header_dual->lgh_cp,
	      sizeof(header_dual->lgh_cp) ) != 0          ||
	header_primary->lgh_active_logs != header_dual->lgh_active_logs)
    {
	TRdisplay("%@ LG: Headers of PRIMARY and DUAL logs are mismatched.\n");
	return (LG_DUALLOG_MISMATCH);
    }

    return (OK);
}

/*
** Name: LG_build_file_name	    - construct the logfile name
**
** Description:
**	This routine constructs the log file's filename into "file_name",
**	and places its length into "l_file_name". It builds either
**
**	    $II_LOG_FILE/ingres/log/ingres_log
**	or
**	    $II_DUAL_LOG/ingres/log/iidual_log
**
**	based on the setting of "which_log".
**
**	If CXcluster_enabled() returns non-zero, we append the node name to the 
**	II_LOG_FILE_NAME value, resulting in something like (on VMS):
**
**	    II_LOG_FILE:[INGRES.LOG]INGRES_LOG.SYSTEM_WS79D
**
**	where "ws79d" was returned from LGCn_name()
**
** Inputs:
**	which_log	    - which log file name should be built:
**			      LG_PRIMARY_LOG
**			      LG_DUAL_LOG
**	nodename	    - name of the node whose logfile is needed, on
**			      a clustered installation, or NULL if the
**			      local logfile is needed.
**
** Outputs:
**	file_name	    - filled in with logfile name
**	l_file_name	    - filled in with filename length
**	path		    - filled in with just the "path", if non-zero
**	l_path		    - filled in with the pathname length.
**
** Returns:
**	STATUS
**
** History:
**	12-oct-1990 (bryanp)
**	    Invented for dual logging support.
**	16-feb-1993 (bryanp)
**	    On the VAXCluster, we must append the nodename to the logfile name.
**	    Removed the never-used support for II_LOG_FILE_PATH.
**	12-jul-1995 (duursma)
**	    on VMS, call the new function NMgtIngAt() when translating 
**	    II_DUAL_LOG or II_DUAL_LOG_NAME
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Log file name may now derived from the PM file and may take many
**	    names (one for each log file partition).
**	01-sep-1998 (hanch04)
**	    Remove fall through for II_LOG_FILE from symbol table.  Only read
**	    it from config.dat or fail.
*/
STATUS
LG_build_file_name(
i4	    which_log,
char	    *nodename,
char	    file_name[][128],
i4	    l_file_name[],
char	    path[][128],
i4	    l_path[],
i4	    *num_files)
{
    char	*fstring;
    char	*nstring;
    char	resource[MAX_LOC];
    LOCATION    path_loc;
    char	file_ext[CX_MAX_NODE_NAME_LEN+2];
    i4	err_code;
    STATUS	status;
    i4		i;

    /* Construct the filename(s) -- if not defined it is an error */
    /*
    ** This may now be an array of partitioned filenames, derived from the PM
    ** variables ii.<node>.rcp.log.log_file_N. For now we will
    ** force the filenames to be in a contiguous list, i.e. we will
    ** expect to see log_file_1, log_file_2 etc. the user will not be 
    ** allowed to skip log_file numbers, any files not in contiguous
    ** order will be ignored
    **
    ** If partitioned log files are not defined, fall back to using
    ** the NM variables II_LOG_FILE/II_LOG_FILE_NAME, etc., to construct
    ** a single non-partitioned log file name.
    */
    file_ext[0] = '\0';
    if ( CXcluster_enabled() )
    {
	if ( NULL == nodename || '\0' == *nodename )
	{
	    nodename = CXnode_name(NULL);
	}
        CXdecorate_file_name( file_ext, nodename );
    }
    else
    {
	nodename = "$";
    }

    if (which_log == LG_PRIMARY_LOG)
    {
	STprintf( resource, "%s.%s.rcp.log.log_file_name", 
		  SystemCfgPrefix, nodename );
    }
    else /* Dual log */
    {
	STprintf( resource, "%s.%s.rcp.log.dual_log_name",
		  SystemCfgPrefix, nodename );
    }
    status = PMget(resource, &nstring);
    if (status == PM_NOT_FOUND || !nstring || !*nstring)
    {
	if (which_log == LG_PRIMARY_LOG)
	    return(E_DMA477_LGOPEN_LOGFILE_NAME);
	else
	    return(E_DMA47B_LGOPEN_DUALLOG_NAME);
    }

    for  (i = 0; i < LG_MAX_FILE; i++)
    {
	if (which_log == LG_PRIMARY_LOG)
	{
	    STprintf( resource, "%s.%s.rcp.log.log_file_%d",
		SystemCfgPrefix, nodename, i + 1);
	}
	else /* Dual log */
	{
	    STprintf( resource, "%s.%s.rcp.log.dual_log_%d",
		SystemCfgPrefix, nodename, i + 1);
	}
	STprintf(file_name[i], "%s.l%02d%s", nstring, i + 1,file_ext);
	file_name[i][DI_FILENAME_MAX] = '\0';
        l_file_name[i] = STlength(file_name[i]);

	status = PMget(resource, &fstring);
	
	if (status == PM_NOT_FOUND || !fstring || !*fstring)
	{
	    break;
	}

	STcopy(fstring, resource);
	/* Construct the path and file names using LO magic. */
	status = LOfroms(PATH, resource, &path_loc);
	if (status == OK)
	{
	    status = LOfaddpath(&path_loc, "ingres", &path_loc);

	    if (status == OK)
	    {
		status = LOfaddpath(&path_loc, "log", &path_loc);
	    }
	}
	if (status)
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    uleFormat(NULL, E_DMA479_LGOPEN_PATH_INFO, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 1,
		    0, fstring);
	    return (E_DMA47A_LGOPEN_PATH_ERR);
	}

	LOtos(&path_loc, &fstring);

        STcopy(fstring, path[i]);
        l_path[i] = STlength(path[i]);

#ifdef xDEBUG
	if (which_log == LG_PRIMARY_LOG)
	    TRdisplay("LG: Primary log file %d is %s in directory %s\n",
		    i, file_name[i], path[i]);
	else
	    TRdisplay("LG: Dual log file %d is %s in directory %s\n",
		    i, file_name[i], path[i]);
#endif
    }

    /*
    ** If the filenames could not be found via PM variables,
    ** construct a single filename the old NM way.
    */
    if (i == 0)
    {
	    if (which_log == LG_PRIMARY_LOG)
		return(E_DMA478_LGOPEN_LOGFILE_PATH);
	    else
		return(E_DMA47C_LGOPEN_DUALLOG_PATH);
    }

    *num_files = (u_i2)i;

    /*
    ** Null out the last log file name to indicate the end of the list
    */
    if (i < LG_MAX_FILE) 
       l_path[i] = l_file_name[i] = *path[i] = *file_name[i] = 0;

    return (OK);
}

static STATUS
form_path_and_file(
				char *nodename,
				char *file,
				i4 *l_file,
				char *path, 
				i4 *l_path)
{
    char	tmpbuf[MAX_LOC];
    char	device[MAX_LOC];
    char	path_buf[MAX_LOC];
    char	fprefix[MAX_LOC];
    char	fsuffix[MAX_LOC];
    char	version[MAX_LOC];
    char	*tmpptr;
    LOCATION	tmploc;

    STcopy(nodename, tmpbuf);
    if (LOfroms(FILENAME & PATH, tmpbuf, &tmploc) != OK ||
	LOdetail(&tmploc, device, path_buf, fprefix, fsuffix, version) != OK)
    {
	*file = *path = '\0';
	*l_file = *l_path = 0;
	return (LG_CANTOPEN);
    }

    if (fsuffix[0] != '\0')
	STpolycat(3, fprefix, ".", fsuffix, file);
    else
	STcopy(fprefix, file);

    *l_file = STlength(file);

    if (LOcompose(device, path_buf, (char *)NULL, (char *)NULL, (char *)NULL, &tmploc)
		!= OK)
    {
	TRdisplay("LG: Can't recompose path.\n");
	*file = *path = '\0';
	*l_file = *l_path = 0;
	return (LG_CANTOPEN);
    }

    LOtos(&tmploc, &tmpptr);
    STcopy(tmpptr, path);
    *l_path = STlength(path);

#ifdef xDEBUG
    TRdisplay("LG: logfile is %s in path %s\n", file, path);
#endif

    return (OK);
}
