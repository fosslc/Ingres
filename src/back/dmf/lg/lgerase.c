/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/*
NO_OPTIM = r64_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>
#include    <lgclustr.h>

/**
**
**  Name: LGERASE.C - Implements the LGerase function of the logging system
**
**  Description:
**	This module contains the code which implements LGerase.
**	
**	    LGerase -- Erase the logfile
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	29-sep-1992 (nandak)
**	    Use one common transaction ID type.
**	18-jan-1993 (bryanp)
**	    The task of formatting a fresh log file has been taken over by
**	    iiconfig. LGerase now merely formats and writes the logfile header.
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (andys/bryanp)
**	    Cluster 6.5 Project I
**	    Renamed stucture members of LG_LA to new names. This means
**	    replacing lga_high with la_sequence, and lga_low with la_offset.
**	    Attempt to be more intelligent about which logs to format. If the
**	    caller says to initialize both logs, but only one log was
**	    was successfully opened, then just initialize that log.
**	21-jun-1993 (andys)
**	    Add lg_id parameter to LGerase to help rcpconfig open other
**	    logfiles on the cluster. Use lfb_status to discover which DI_IO
**	    control blocks to use.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	23-aug-1993 (bryanp)
**	    On the cluster, cram the cluster's node ID into the high part of
**		the tran ID so that tran ID's on different nodes of the cluster
**		are always unique.
**	    Also, removed a redundant extra computation of the check sum.
**	31-jan-1994 (jnash)
**	    Fix lint detected problems: remove lga in LGerase. 
**      12-jun-1995 (canor01)
**	    semaphore protect memory allocation (MCT)
**      13-jun-1995 (chech02)
**          Added LGK_misc_sem semaphore to protect critical sections.(MCT)
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**      03-jun-1996 (canor01)
**          New ME for operating-system threads does not need external
**          semaphores.
**    08-Dec-1997 (hanch04)
**        Initialize new block field in LG_LA and lgh_last_lsn in
**        the log header for support of logs > 2 gig
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Replace DIread/write with LG_READ/WRITE macros, DI_IO structures
**	    with LG_IO structures.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/

/*{
** Name: LGerase	- Zero out the log file.
**
** Description:
**      This routine erases the log file to contain only null data.
**	The log file must have been LGopen()ed prior to this call.
**
**      It is CRITICALLY important that log file sequence numbers be
**      ever-increasing. Log file addresses are stored by client code in
**      the database config file, and if a particular log file ever gets
**      re-initialized and starts to get lower log file addresses, horrid
**      things happen to the database. In order to ensure that log file
**      addresses are never set lower, this routine assigns the high-order
**      32 bits of beginning log file address (the sequence number) as
**      follows:
**          1) We get the current system time, and massage it slightly so
**              that it's roughly the number of seconds since 1970. This is
**              a rough calculation, but it does produce a greater value
**              each time it's called (so long as the user doesn't reset the
**              system clock and the seconds doesn't overflow 32 bits).
**          2) If possible, we check the previous value of the log file's
**              sequence counter. This field is not available if the log
**              file has never been initialized, or if this is FORCE_INIT, but
**              if neither of those hold we retrieve the previous sequence
**              counter value and force the new sequence field to be greater
**              than the previous value.
** Inputs:
**	lg_id				Transaction id.
**	lx_id				Not used.
**	bcnt				Buffer count in terms of block size.
**	bsize				block size.
**	force_init			Force erase the log file even if there
**					are outstanding transactions.
**	logs_to_initialize		indicator of which logs to initialize.
**					LG_ERASE_PRIMARY_ONLY
**					LG_ERASE_DUAL_ONLY
**					LG_ERASE_BOTH
**
** Outputs:
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_CANTINIT		    Can't initialize the log file.
**	    LG_INITHEADER	    There are some transactions still 
**				    outstanding in the log file and the
**				    force_init was not set.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-jan-1993 (bryanp)
**	    The task of formatting a fresh log file has been taken over by
**	    iiconfig. LGerase now merely formats and writes the logfile header.
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	26-apr-1993 (andys/bryanp)
**	    Cluster 6.5 Project I
**	    Renamed stucture members of LG_LA to new names. This means
**	    replacing lga_high with la_sequence, and lga_low with la_offset.
**	    Attempt to be more intelligent about which logs to format. If the
**	    caller says to initialize both logs, but only one log was
**	    was successfully opened, then just initialize that log.
**	21-jun-1993 (andys)
**	    Add lg_id parameter to LGerase to help rcpconfig open other
**	    logfiles on the cluster. Use lfb_status to discover which DI_IO
**	    control blocks to use. Allow header to be read from whichever
**	    logfile is open.
**	23-aug-1993 (bryanp)
**	    On the cluster, cram the cluster's node ID into the high part of
**		the tran ID so that tran ID's on different nodes of the cluster
**		are always unique.
**	    Also, removed a redundant extra computation of the check sum.
**	31-jan-1994 (jnash)
**	    Fix lint detected problems: remove unused lga, note lx_id unused.
**	19-Apr-2005 (schka24)
**	    fudge db_low_tran forward in case tx's are being used faster than
**	    once per second (for replicator).
**	10-Nov-2009 (kschendel) SIR 122757
**	    Make sure buffer is properly aligned.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Remove lx_id parameter, not used.
*/
STATUS
LGerase(
LG_LGID		    *lg_id,
i4		    bcnt,
i4		    bsize,
bool		    force_init,
u_i4		    logs_to_initialize,
CL_ERR_DESC	    *sys_err)
{
    i4	    status;
    register LG_HEADER *header;
    char	*buffer, *abuf;
    i4		size, align;
    u_i4	    sequence;
    u_i4	    timenow;
    u_i4	    prev_sequence = 0;
    char	    hold_var[ LGC_MAX_NODE_NAME_LEN ];
    i4	    err_code;
    i4		    num_pages;
    STATUS	    me_status;
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB    *lpb;
    register LFB    *lfb;
    SIZE_TYPE	    *lpbb_table;
    LG_IO	    *prim_dio = 0;	/* LG handle for primary log	*/
    LG_IO	    *dual_dio = 0;	/* LG handle for dual log	*/
    LG_IO	    *first_dio = 0;	/* LG handle to read log header	*/
    LG_ID	    *int_lgid = (LG_ID *) lg_id;		
					/* internal format log id   	*/
    bool	    first_read_from_prim; 
			/* was first LGread from the primary logfile?   */
    i4		    num_partitions;

    CL_CLEAR_ERR(sys_err);
    
    /*	Check the internal log id. */

    if (int_lgid->id_id == 0 || (i4)int_lgid->id_id > lgd->lgd_lpbb_count)
    {
	uleFormat(NULL, E_DMA48B_LGERASE_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, int_lgid->id_id, 0, lgd->lgd_lpbb_count); 
	return (LG_BADPARAM);
    }

    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[int_lgid->id_id]);

    if (lpb->lpb_type != LPB_TYPE ||
	lpb->lpb_id.id_instance != int_lgid->id_instance)
    {
	uleFormat(NULL, E_DMA48A_LGERASE_BAD_PROC, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
		    0, int_lgid->id_instance); 
	return (LG_BADPARAM);
    }

    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);

    status = OK;

    if (lfb->lfb_status & LFB_USE_DIIO)
    {
	if (lfb->lfb_status & LFB_PRIMARY_LOG_OPENED)
	{
	    prim_dio = &lfb->lfb_di_cbs[LG_PRIMARY_DI_CB];
	    num_partitions = prim_dio->di_io_count;
	}

	if (lfb->lfb_status & LFB_DUAL_LOG_OPENED)
	{
	    dual_dio = &lfb->lfb_di_cbs[LG_DUAL_DI_CB];
	    num_partitions = dual_dio->di_io_count;
	}
    }
    else
    {
	if (LG_logs_opened & LG_PRIMARY_LOG_OPENED)
	{
	    prim_dio = &Lg_di_cbs[LG_PRIMARY_DI_CB];
	    num_partitions = prim_dio->di_io_count;
	}

	if (LG_logs_opened & LG_DUAL_LOG_OPENED)
	{
	    dual_dio = &Lg_di_cbs[LG_DUAL_DI_CB];
	    num_partitions = dual_dio->di_io_count;
	}
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
	TRdisplay("LG: Can't allocate %d bytes for log erase\n",size+align);
	return (LG_CANTINIT);
    }
    abuf = buffer;
    if (align != 0)
	abuf = ME_ALIGN_MACRO(abuf, align);

    header = (LG_HEADER *)abuf;

    /*
    ** Need to figure out how I get the DI_IO structure to read with.
    */

    if (force_init == FALSE)
    {
	/*
	** see which log we will read the initial header from
	*/
	if (prim_dio)
	{
	    first_dio = prim_dio;
	    first_read_from_prim = TRUE;
	}
	else if (dual_dio)
	{
	    first_dio = dual_dio;
	    first_read_from_prim = FALSE;
	}
	else
	{
	    return (LG_CANTOPEN);
	}
	    
	/*	Read the block containing the header. If it checksums
	**	properly, or if if fails to checksum but appears to have been
	**	written by a previous version of LG, then check so see if
	**	there appear to be active transactions in the logfile and
	**	indicate that -force_init is required.
	**	If the primary log file indicates that only the dual log file
	**	is active, then read the header block from the dual log file
	**	instead, since testing the old stale primary header block doesnt
	**	test the right header information.
	*/

	num_pages = 1;
	status = LG_READ(first_dio, &num_pages,
			(i4)0, (char *)header, sys_err);
	if (status)
	{
	    (VOID) uleFormat(NULL, status, sys_err, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
	    (void) MEfree(buffer);
	    return(LG_CANTINIT);
	}

	if (LGchk_sum((PTR)header, sizeof(LG_HEADER)) == header->lgh_checksum)
	{
	    if (first_read_from_prim && 
		    (header->lgh_active_logs == LGH_DUAL_LOG_ACTIVE))
	    {
		/*
		** The primary log is not active. The header we want is the
		** header from the dual log. Go read it.
		*/
		if (LG_logs_opened & LG_DUAL_LOG_OPENED)
		{
		    /*
		    ** It's not always the case that the dual log file is opened
		    ** at this point(under some circumstances LGopen has opened
		    ** only the primary log
		    ** file). If we are unable to read the dual log file, the
		    ** code currently just sets header->lgh_status to LGH_OK 
		    ** which allows the re-initialization of the primary log.
		    */
		    num_pages = 1;
		    status = LG_READ(dual_dio, &num_pages,
				(i4)0, (char *)header, sys_err);
		    if (status)
		    {
			(VOID) uleFormat(NULL, status, sys_err, ULE_LOG,
					NULL, NULL, 0, NULL,
					&err_code, 0);
			(void) MEfree(buffer);
			return(LG_CANTINIT);
		    }
		}
		else
		{
		    header->lgh_status = LGH_OK;
		}
	    }
	}

	/*
	** Does this appear to be a properly formatted LG_HEADER? It may be
	** an LG_HEADER from a previous release of LG, so we allow for that...
	*/
	if (LGchk_sum((PTR)header, sizeof(LG_HEADER)) == header->lgh_checksum
		||
	    header->lgh_version != LGH_VERSION)
	{
	    
	    /* 
	    ** If the log header status indicates LGH_BAD or LGH_OK,
	    ** then it is OK to erase the log file. Otherwise, there
	    ** may have some transactions still outstanding in the
	    ** log file. 
	    */

	    if (header->lgh_status >= LGH_VALID &&
		header->lgh_status <= LGH_OK    &&
		header->lgh_status != LGH_BAD   &&
		header->lgh_status != LGH_OK)
	    {
		(void) MEfree(buffer);
		return(LG_INITHEADER);
	    }

	    /*
	    ** Retrieve the previous log file sequnece number so that we
	    ** can ensure that the new sequence number is at least this
	    ** large
	    */
	    prev_sequence = header->lgh_end.la_sequence;
	}
    }

    /*
    ** If they asked to initialize both logs, ensure that both logs were
    ** successfully opened. If only one log was successfully opened, then
    ** quietly initialize only that log.
    */
    if (logs_to_initialize == LG_ERASE_BOTH_LOGS)
    {
	if (!prim_dio && dual_dio)
	    logs_to_initialize = LG_ERASE_DUAL_ONLY;
	else if (prim_dio && !dual_dio)
	    logs_to_initialize = LG_ERASE_PRIMARY_ONLY;
    }

    /*	Initialize the header of the log file. */

    /*
    ** take the sequence number from the number of seconds since 1970. This is
    ** intended to ensure that each time we are called we get a higher log file
    ** sequence number than ever before.  (1 second granualarity should be
    ** fine) Just in case, though, after we retrieve the time in seconds we
    ** check it against the previous log-address sequence number; if the new
    ** number is less than the previous number we quietly bump it up to be
    ** greater.
    */
    sequence = timenow = TMsecs();
    if ( sequence < prev_sequence)
	    sequence = prev_sequence + 1;

    header->lgh_version = LGH_VERSION;
    header->lgh_checksum = 0;
    header->lgh_status = LGH_OK;
    header->lgh_size = bsize;
    header->lgh_count = bcnt;
    header->lgh_l_logfull = .8 * header->lgh_count;
    header->lgh_l_abort = .9 * header->lgh_count;
    header->lgh_l_cp = .1 * header->lgh_count;
    header->lgh_cpcnt = 2;
    header->lgh_begin.la_sequence = sequence;
    header->lgh_begin.la_block    = 0;
    header->lgh_begin.la_offset   = 0;
    header->lgh_cp.la_sequence    = sequence;
    header->lgh_cp.la_block       = 0;
    header->lgh_cp.la_offset      = 0;
    header->lgh_end.la_sequence   = sequence;
    header->lgh_end.la_block      = 0;
    header->lgh_end.la_offset     = 0;
    header->lgh_last_lsn.lsn_high      = sequence;
    header->lgh_last_lsn.lsn_low       = 0;

    /*
    ** we want ever increasing transaction id's, and we want to make it 
    ** unlikely to run out. Furthermore, on the cluster we wish each node
    ** to generate distinct transaction ID's, which is at odds with the
    ** desire to have the tran ID's ever-increasing. We compromise, on the
    ** cluster, and have each node cram its node ID into the high part of
    ** the tran ID, thus ensuring that tran IDs from separate nodes never
    ** collide.
    ** If we reset the low tran (the one Replicator uses) to "now", fudge
    ** it forward by about 100 days just in case the installation had been
    ** running faster than one tx per second before erasing the log.
    ** Note that this may turn out to be a crappy idea in 2037, but
    ** we'd better fix replicator by then anyway.  :-)
    */
    header->lgh_tran_id.db_high_tran = sequence >> 16;
    header->lgh_tran_id.db_low_tran = sequence;
    if (sequence == timenow)
    {
	header->lgh_tran_id.db_low_tran = timenow + 10000000;
	if (timenow + 10000000 < sequence)
	    ++ header->lgh_tran_id.db_high_tran;	/* Overflow */
    }

    /* File the number of partitions in the header */
    header->lgh_partitions = num_partitions;

    if (LGcluster())
    {
	status = LGcnode_info( hold_var, LGC_MAX_NODE_NAME_LEN, sys_err);
	if ( status != OK )
	{
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	    (void) MEfree(buffer);
	    return(status);
	}
	header->lgh_tran_id.db_high_tran |= (LG_cluster_node(hold_var) << 24);
    }

    switch (logs_to_initialize)
    {
	case LG_ERASE_PRIMARY_ONLY:
	    header->lgh_active_logs = LGH_PRIMARY_LOG_ACTIVE;
    
	    status = LG_write_log_headers( header, prim_dio,
				    (LG_IO *)NULL, abuf, sys_err );
	    break;

	case LG_ERASE_DUAL_ONLY:
	    header->lgh_active_logs = LGH_DUAL_LOG_ACTIVE;
    
	    status = LG_write_log_headers( header, (LG_IO *)NULL,
				    dual_dio, abuf, sys_err);
	    break;

	case LG_ERASE_BOTH_LOGS:
	    header->lgh_active_logs = LGH_BOTH_LOGS_ACTIVE;

	    status = LG_write_log_headers( header, prim_dio,
				    dual_dio, abuf, sys_err);
	    break;

	default:
	    status = OK;
	    break;
    }

    (void) MEfree(buffer);
    return(status);
}

/*
** Name: LG_write_log_headers	- checksum and write log header(s)
**
** Description:
**	This utility routine is called to write out the log file header(s) to
**	the log file(s). Each log file has the header written twice. If only
**	one log file is currently active, its channel can be passed in either
**	channel_primary or channel_dual, depending on which log file is active.
**
**	This routine check-sums the header before writing it.
**
** Inputs:
**	header		- the header to be written, already time-stamped
**	di_io_primary	- the primary log file di_io
**	di_io_dual	- the dual log file di_io, or 0.
**	abuf		- An aligned buffer for writing from (the caller
**			  probably has one available)
**
** Outputs:
**	sys_err		- detailed info about the error, if one occurred
**
** Returns:
**	OK
**	LG_WRITEERROR	- unable to write ALL of the log file headers
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	10-Nov-2009 (kschendel) SIR 122757
**	    Pass in aligned buffer so we don't have to make one here.
*/
STATUS
LG_write_log_headers(
LG_HEADER   *header,
LG_IO	    *di_io_primary,
LG_IO	    *di_io_dual,
char	    *abuf,
CL_ERR_DESC *sys_err )
{
    STATUS	    status;
    i4		    num_pages;
    i4	    err_code;
    STATUS	    ret_status = OK;
    i4		    successful_writes = 0;

    TMget_stamp( (TM_STAMP *)header->lgh_timestamp );
    header->lgh_checksum = LGchk_sum((PTR)header, sizeof(LG_HEADER));

    MEcopy( (PTR)header, sizeof(LG_HEADER), abuf );
    MEcopy( (PTR)header, sizeof(LG_HEADER), abuf + 1024 );
    
    /*	Write the header of the log file. */

    if (di_io_primary)
    {
	num_pages = 1;
	status = LG_WRITE(di_io_primary, &num_pages,
			    (i4)0, abuf, sys_err);
	if (status != OK || num_pages != 1)
	{
	    (VOID) uleFormat(NULL, status, sys_err, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
	    ret_status = LG_WRITEERROR;
	}
	else
	{
	    successful_writes++;
	}
    }


    /*
    ** If dual log is open, write its headers
    */
    if (di_io_dual)
    {
	num_pages = 1;
	status = LG_WRITE(di_io_dual, &num_pages,
			    (i4)0, abuf, sys_err);
	if (status != OK || num_pages != 1)
	{
	    (VOID) uleFormat(NULL, status, sys_err, ULE_LOG, NULL, NULL, 0, NULL,
				&err_code, 0);
	    ret_status = LG_WRITEERROR;
	}
	else
	{
	    successful_writes++;
	}
    }

    return ((successful_writes > 0) ? OK : ret_status);
}
