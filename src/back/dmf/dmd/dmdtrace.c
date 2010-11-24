/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <scf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmftrace.h>
#include    <dmd.h>
#include    <dm1c.h>

#include    <dmpepcb.h>

/**
**
**  Name: DMDTRACE.C - Session trace routines.
**
**  Description:
**      This module contains the routines to format trace requests.
**
**          dmd_iotrace - Format DI/SR I/O requests.
**	    dmd_lock - Format Lock requests.
**	    dmd_logtrace - Format Log records.
**	    dmd_petrace - Format DMPE put request
**	    dmd_reptrace - check for replication server connections
**
**	Session trace points are sort of *BOTH* session specific AND
**	server-wide. Consider, for example, "set lock_trace" (DM0001). There
**	are two tracepoint bit vectors in the world, the server-wide tracepoint
**	bit vector (in the dmf_svcb), and the session-specific tracepoint bit
**	vector (in each session's SCB). Setting trace point DM1 causes your
**	session's tracepoint bit vector to have bit 1 set, AND ALSO CAUSES THE
**	SERVER-WIDE TRACEPOINT VECTOR TO HAVE BIT 1 SET! This is because the
**	code which is sprinkled throughout DMF to test DMZ_SES_MACRO(1) doesn't
**	want to be bothered looking up the session's control block. So what
**	happens is that as soon as you set lock_trace on in your session, all
**	sessions in the server begin calling dmd_lock(). dmd_lock, in turn,
**	looks up the session control block for each session and checks the value
**	of bit 1 in the session tracepoint vector. For all sessions except your
**	session, the bit is off, so dmd_lock just returns. For your session,
**	the bit is on, so dmd_lock does its thing.
**
**	When your session dies, or you do a "set notrace point dm1", we
**	really don't want the server forever-after calling the tracing
**	wrappers, doing get-SID calls, etc.  So we'll recompute the session
**	tracepoint section of the server trace vector.  This is the first
**	96 bits (although DMZ_POINTS is 100, we'll hardcode in 96 since
**	that way we can manage that part of the vector by hand as 3 i4's).
**	Recalculating this portion of the server trace vector is a PITA,
**	but it's better than inducing the server overhead indefinitely -
**	regardless of how minor that overhead might be.
**
**
**  History:
**      17-feb-1987 (jennifer)
**          Created for Jupiter.
**      01-mar-1990 (fred)
**          Created dmd_petrace().
**	 9-apr-1990 (sandyh)
**	    Added dmd_logtrace().
**	25-feb-1991 (rogerk)
**	    Added support for DM0LTEST log records to dmd_logtrace.
**	    Added as part of Archiver Stability project.
**	16-jul-1991 (bryanp)
**	    B38527: New log record types DM0L_OLD_MODIFY & DM0L_OLD_INDEX &
**	    DM0L_SM0_CLOSEPURGE.
**	 3-nov-1991 (rogerk)
**	    Added handling of DM0L_OVFL log record.
**	    Added as part of fixes for REDO handling of file extends.
**	7-july-1992 (jnash)
**	    Add DMF Function prototypes.
**	29-August-1992 (rmuth)
**	    add dmd_build_iotrace.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	04-nov-92 (jrb)
**	    Changed dmd_siotrace to give more information for sort tracing
**	    (as part of multi-location sorts project).
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:  Added new recovery log records and
**	    log record type values to dmd_logtrace.  Also formatted
**	    log record flags along with type/length.
**	12-jan-1993 (mikem)
**	    Lint directed code cleanup.  Added a return(OK) to bottom of 
**	    dmd_txstat(), it would leave it's return value uninitialized in
**	    some cases.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Project:
**		Trace Log Sequence Number in log_trace.
**		Respond to more lint suggestions.
**	26-apr-1993 (jnash)
**	    Change format of "set log_trace" output, add 'reserved_space' 
**	    parameter and associated output.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	    Support tracing of various buffer manager locks for internal
**		debugging and testing use. Tracing of these locks is keyed off a
**		separate trace point, and so is not triggered by ordinary
**		set lock_trace.
**	    Also, "just in case", checked the returned SCF session control block
**		pointer to see if it's 0 (i.e., we're not "in session context").
**	    Added a comment discussing some of the interesting properties of
**		session trace points.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	20-sep-1993 (rogerk)
**	    Add new DM0L_NONJNL_TAB log record flag to log trace output.
**	22-nov-1993 (jnash)
**	    Create dmd_lkrqst_trace() from bits of dmd_lock() so that it can
**	    be used elsewhere, call if from dmd_lock().
**      04-apr-1995 (dilma04)
**          Add support for tracing table lock releases: table locks may be 
**          released when a table is closed, if that table was opened using 
**          cursor stability or repeatable read isolation level.  
**	3-jun-96 (stephenb)
**	    Add dmd_reptrace routine for trace point dm32
**      12-nov-1997 (stial01)
**          dmd_lkrqst_trace() Added case LK_ROW, LK_VAL_LOCK.
**	03-dec-1998 (nanpr01)
**	    Check for null pointer before dereferencing it.
**	16-feb-1999 (nanpr01)
**	    Support for update mode lock.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Replace SCU_INFORMATION calls to SCF to get pointer
**	    to DML_SCB with GET_DML_SCB macro.
**      06-apr-2001 (stial01)
**          Added dmd_pagetypes()
**	29-Apr-2004 (gorvi01)
**		Added UNEXTEND for log trace.This will be used for tracing log
**		for unextenddb.
**      12-may-2004 (stial01)
**          Fixed printing of log records
**	22-Jul-2004 (schka24)
**	    Header doc update.
**      03-mar-2005 (stial01)
**          Fixed lock trace output for request_flags
**      12-apr-2007 (stial01)
**          Added support for rollforwarddb -incremental
**      08-apr-2008 (horda03) Bug 120349
**          Fixed and tidied logging system status displays.
**	30-May-2008 (jonj)
**	    LSNs are 2 u_i4's; display them in hex rather than
**	    signed decimal.
*/

/*
**  Static function definitions.
*/

static	STATUS	print(  	    /* Actual print function. */
		    PTR		    arg,
		    i4		    count,
		    char	    *buffer );

/*{
** Name: dmd_lock	- Format lock requests. 
**
** Description:
**      This routine will display a lock requests.
**
** Inputs:
**      lock_key                     Key associated with lock
**      lockid                       Lock list id.
**      request_type                 Lock type, request or release.
**      request_flag                 Type of request, logical, 
**                                   physical, convert, etc.
**      lock_mode                    Lock mode, exclusive, shared, etc.
**      timeout                      Lock timeout value.
**      tran_id                      Transaction identifier.
**      table_name                   Table lock is associated with.
**      database_name                Database lock is associated with.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-feb-1987 (jennifer)
**          Created for Jupiter.
**	24-may-1993 (bryanp)
**	    Support tracing of various buffer manager locks for internal
**		debugging and testing use. Tracing of these locks is keyed off a
**		separate trace point, and so is not triggered by ordinary
**		set lock_trace.
**	    Also, "just in case", checked the returned SCF session control block
**		pointer to see if it's 0 (i.e., we're not "in session context").
**	22-nov-1993 (jnash)
**	    Move lock request tracing to dmd_lkrqst_trace() so it can be used
**	    elsewhere.
*/
STATUS
dmd_lock(
LK_LOCK_KEY  *lock_key,
i4      lockid,
i4      request_type,
i4	     request_flag,
i4      lock_mode,
i4      timeout,
DB_TRAN_ID   *tran_id,
DB_TAB_NAME  *table_name,
DB_DB_NAME   *database_name)
{
    DML_SCB	    *scb;
    CS_SID	    sid;

    /* Get the session trace vector. */
    CSget_sid(&sid);
    if ( (scb = GET_DML_SCB(sid)) && DMZ_MACRO(scb->scb_trace, 1) )
    {
	dmd_lkrqst_trace(print, lock_key, lockid, request_type, request_flag, 
	    lock_mode, timeout, tran_id, table_name, database_name);
    }

    return(E_DB_OK);
}

/*{
** Name: dmd_iotrace	- Format DI I/O requests. 
**
** Description:
**      This routine will display a DI I/O requests.
**
** Inputs:
**      io_type                      Type of DI I/O request
**                                   such as READ or WRITE.
**      first_page                   Page number of first page rquested.
**      number_pages		     Number of pages requested.
**      table_name                   Table name.
**      file_name                    File name.
**      database_name                Database name.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-feb-1987 (jennifer)
**          Created for Jupiter.
*/
STATUS
dmd_iotrace(
i4      io_type,
DM_PAGENO    first_page,
i4      number_pages,
DB_TAB_NAME  *table_name,
DM_FILE      *file_name,
DB_DB_NAME   *database_name)
{
    char	    buffer[132];
    DML_SCB	    *scb;
    CS_SID	    sid;

    /* Get the session trace vector. */
    CSget_sid(&sid);
    if ( (scb = GET_DML_SCB(sid)) && DMZ_MACRO(scb->scb_trace, 10) )
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
	    "%8* I/O  %5w File: %t (%~t,%~t,%d) Count: %d\n",
	    "READ,WRITE", io_type,
	    sizeof(*file_name), file_name, 
	    sizeof(*database_name), database_name,
	    sizeof(*table_name), table_name, 
	    first_page, number_pages);
    }

    return (E_DB_OK);
}

/*{
** Name: dmd_build_iotrace	- Format build DI I/O requests. 
**
** Description:
**      This routine will display a build DI I/O requests.
**
** Inputs:
**      io_type                      Type of DI I/O request
**                                   such as READ or WRITE.
**      first_page                   Page number of first page rquested.
**      number_pages		     Number of pages requested.
**      table_name                   Table name.
**      file_name                    File name.
**      database_name                Database name.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-June-1991 (rmuth)
**	    Created.
*/
STATUS
dmd_build_iotrace(
i4      io_type,
DM_PAGENO    first_page,
i4      number_pages,
DB_TAB_NAME  *table_name,
DM_FILE      *file_name,
DB_DB_NAME   *database_name )
{
    DML_SCB	    *scb;
    CS_SID	    sid;
    char	    buffer[132];

    /* Get the session trace vector. */
    CSget_sid(&sid);
    if ( (scb = GET_DML_SCB(sid)) && DMZ_MACRO(scb->scb_trace, 10) )
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
	    "%8* I/O  %5w BUILD File: %t (%~t,%~t,%d) Count: %d\n",
	    "READ,WRITE", io_type,
	    sizeof(*file_name), file_name, 
	    sizeof(*database_name), database_name,
	    sizeof(*table_name), table_name, 
	    first_page, number_pages);
    }

    return (E_DB_OK);
}

/*
**
** History:
**	04-nov-92 (jrb)
**	    Changed for multi-location sorting.
*/
STATUS
dmd_siotrace(
i4     io_type,
i4	    in_or_out,
i4	    block,
i4	    filenum,
i4	    location,
i4	    rblock)
{
    DML_SCB	    *scb;
    CS_SID	    sid;
    char	    buffer[132];

    /* Get the session trace vector. */
    CSget_sid(&sid);
    if ( (scb = GET_DML_SCB(sid)) && DMZ_MACRO(scb->scb_trace, 10) )
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
	    "%4* I/O  %5w of %6w SORT File; virtual block %d; file# %d; location %d; relative block %d\n",
	    "READ,WRITE", io_type, "INPUT,OUTPUT", in_or_out, block, filenum,
	    location, rblock);
    }

    return(E_DB_OK);
}

/*{
** Name: dmd_logtrace	- Format Log headers.
**
** Description:
**      This routine will display an LGwrite header's type and size.
**
** Inputs:
**	Log header
**	reserved_space			Amount of reserved space
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      9-apr-1990 (sandyh)
**          Created.
**	25-feb-1991 (rogerk)
**	    Added support for DM0LTEST log records.
**	    Added as part of Archiver Stability project.
**	16-jul-1991 (bryanp)
**	    B38527: New log record types DM0L_OLD_MODIFY & DM0L_OLD_INDEX &
**	    DM0L_SM0_CLOSEPURGE.
**	 3-nov-1991 (rogerk)
**	    Added handling of DM0L_OVFL log record.
**	    Added as part of fixes for REDO handling of file extends.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project:  Added new recovery log records and
**	    log record type values to dmd_logtrace.  Also formatted
**	    log record flags along with type/length.
**	26-apr-1993 (bryanp)
**	    6.5 Cluster Project:
**		Trace Log Sequence Number in log_trace if DM12 is set in
**		    addition to DM11.
**	26-apr-1993 (jnash)
**	    Change format of "set log_trace" output, add 'reserved_space' 
**	    parameter and associated output.
**	20-sep-1993 (rogerk)
**	    Add new DM0L_NONJNL_TAB log record flag to log trace output.
**	1-Apr-2004 (schka24)
**	    Add rawdata record type, increase size fields.
*/
STATUS
dmd_logtrace(
DM0L_HEADER	*log_hdr,
i4		reserved_space)
{
    DML_SCB	    *scb;
    CS_SID	    sid;
    char	    buffer[132];
    LG_LSN	    comp_lsn; 

    /* Get the session trace vector. */
    CSget_sid(&sid);
    if ( (scb = GET_DML_SCB(sid)) && DMZ_MACRO(scb->scb_trace, 11) )
    {
        if ( DMZ_MACRO(scb->scb_trace, 12) )
	{
	    if (log_hdr->flags & DM0L_CLR)
	    {
		comp_lsn = log_hdr->compensated_lsn;
	    }
	    else
	    {
		comp_lsn.lsn_high = 0; 
		comp_lsn.lsn_low = 0; 
	    }

	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
	"%8* LSN=(%x,%x), COMP_LSN=(%x,%x), DBID=0x%x, XID=0x%x%x\n", 
		log_hdr->lsn.lsn_high, log_hdr->lsn.lsn_low, 
		comp_lsn.lsn_high, comp_lsn.lsn_low, 
		log_hdr->database_id, 
		log_hdr->tran_id.db_high_tran, log_hdr->tran_id.db_low_tran);
	}

	TRformat(print, 0, buffer, sizeof(buffer) - 1,
	    "%4* LOG: %10w  Size written/reserved: %6d/%6d  Flags: %v",
	    DM0L_RECORD_TYPE, log_hdr->type, log_hdr->length, reserved_space,
	    DM0L_HEADER_FLAGS, log_hdr->flags);
    }

    return(E_DB_OK);
}

static STATUS 
print(
PTR	    arg,
i4	    count,
char	    *buffer)
{
    SCF_CB	scf_cb;

    buffer[count] = '\n';
    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0;
    scf_cb.scf_len_union.scf_blength = (u_i4)count + 1;
    scf_cb.scf_ptr_union.scf_buffer = buffer;
    scf_cb.scf_session = DB_NOSESSION;
    scf_call(SCC_TRACE, &scf_cb);
}

/*
** Name: dmd_rstat
**
** History:
**	26-apr-1993 (bryanp)
**	    Added return(OK) to end of routine so we don't just fall off the end
**	08-Jan-2001 (jenjo02)
**	    Get *DML_SCB from XCB instead of calling SCU_INFORMATION.
*/
STATUS
dmd_rstat(
DMP_RCB	    *rcb)
{
    DMP_RCB	    *r = rcb;
    DML_XCB	    *x = r->rcb_xcb_ptr;
    DML_SCB	    *scb = x->xcb_scb_ptr;
    char	    buffer[132];

    /* Get the session trace vector. */
    if ( DMZ_MACRO(scb->scb_trace, 21) )
    {
	TRformat(print, 0, buffer, sizeof(buffer) - 1,
"Table(%~t:%~t) FIX(%d,%d) OP=IDRL(%d,%d,%d,%d) GET=RQS(%d,%d,%d)\n",
	    sizeof(r->rcb_tcb_ptr->tcb_rel.relowner),
	    &r->rcb_tcb_ptr->tcb_rel.relowner,
	    sizeof(r->rcb_tcb_ptr->tcb_rel.relid),
	    &r->rcb_tcb_ptr->tcb_rel.relid,
	    r->rcb_s_fix, r->rcb_s_io, r->rcb_s_ins, r->rcb_s_del,
	    r->rcb_s_rep, r->rcb_s_load, r->rcb_s_get, r->rcb_s_qual,
	    r->rcb_s_scan, r->rcb_s_load);

    }

    /*	Accumulate for the transaction summary. */ 

    x->xcb_s_fix += r->rcb_s_fix;
    x->xcb_s_insert += r->rcb_s_ins;
    x->xcb_s_delete += r->rcb_s_del;
    x->xcb_s_replace += r->rcb_s_rep;
    x->xcb_s_get+= r->rcb_s_get;

    return (E_DB_OK);
}

/*
** Name: dmd_txstat
**
** History:
**	22-nov-1993 (jnash)
**	    Add minimalist function header.
**	08-Jan-2001 (jenjo02)
**	    Get *DML_SCB from XCB instead of calling SCU_INFORMATION.
*/
STATUS
dmd_txstat(
DML_XCB	    *xcb,
i4	    state)
{
    DML_XCB	    *x = xcb;
    DML_SCB	    *scb = x->xcb_scb_ptr;
    SCF_CB	    scf_cb;
    SCF_SCI	    sci_list[2];
    DB_STATUS	    status;
    i4	    cpu;
    i4	    dio;
    char	    buffer[132];

    /* Get the session trace vector. */
    if ( DMZ_MACRO(scb->scb_trace, 20) )
    {
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_DMF_ID;
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *)sci_list;
	scf_cb.scf_len_union.scf_ilength = 2;
	
	sci_list[0].sci_length = sizeof(cpu);
	sci_list[0].sci_code = SCI_CPUTIME;
	sci_list[0].sci_aresult = (char *) &cpu;
	sci_list[0].sci_rlength = 0;
	sci_list[1].sci_length = sizeof(dio);
	sci_list[1].sci_code = SCI_DIOCNT;
	sci_list[1].sci_aresult = (char *) &dio;
	sci_list[1].sci_rlength = 0;

	status = scf_call(SCU_INFORMATION, &scf_cb);
	if (status != E_DB_OK)
	    return (status);

	if (state == 0)
	{
	    x->xcb_s_open = 0;
	    x->xcb_s_fix = 0;
	    x->xcb_s_get = 0;
	    x->xcb_s_delete = 0;
	    x->xcb_s_replace = 0;
	    x->xcb_s_insert = 0;
	    x->xcb_s_cpu = cpu;
	    x->xcb_s_dio = dio;
	}
	else
	{
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
"Tran %w: OPEN:%d FIX:%d GET:%d REP:%d INS:%d DEL:%d CPU:%d IO:%d\n",
		",COMMIT,ABORT", state,
		x->xcb_s_open, x->xcb_s_fix, x->xcb_s_get, x->xcb_s_replace,
		x->xcb_s_insert, x->xcb_s_delete, cpu - x->xcb_s_cpu,
		dio - x->xcb_s_dio);
	}
    }
    return(OK);
}

/*{
** Name: dmd_petrace	- Format a DMPE operation.
**
** Description:
**      This routine will  format a dmpe_put request. 
**
** Inputs:
**	operation			The operation being performed
**      record                          The record being put.
**      base                            The id of the base table
**      extension                       The id of the extension
**
** Outputs:
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
**      01-mar-1990 (fred)
**          Created.
**	02-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    DMPE_RECORD pointer typed as PTR
[@history_template@]...
*/
VOID
dmd_petrace(
	char		   *operation,
	PTR        	   rec,
	i4		   base,
	i4		   extension)
{
    DMPE_RECORD	    *record = (DMPE_RECORD*)rec;
    DML_SCB	    *scb;
    CS_SID	    sid;
    char	    buffer[132];

    /* Get the session trace vector. */
    CSget_sid(&sid);
    if ( (scb = GET_DML_SCB(sid)) && DMZ_MACRO(scb->scb_trace, 11) )
    {
	if (record)
	{
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
	"%8* %s base: %x/Ext: %x (per_key %x:%x per_segment %x:%x per_next: %x)\n",
		operation, base, extension, record->prd_log_key.tlk_high_id,
		record->prd_log_key.tlk_low_id,
		record->prd_segment0, record->prd_segment1, record->prd_r_next);
	}
	else
	{
	    TRformat(print, 0, buffer, sizeof(buffer) - 1,
		"%s", operation);
	}
    }

    return;
}

/*{
** Name: dmd_lkrqst_trace	- Format lock requests. 
**
** Description:
**      This routine displays lock requests.
**
** Inputs:
**      format_routine		     Function used to output data
**      lock_key                     Key associated with lock
**      lockid                       Lock list id.
**      request_type                 Lock type, request or release.
**      request_flag                 Type of request, logical, 
**                                   physical, convert, etc.
**      lock_mode                    Lock mode, exclusive, shared, etc.
**      timeout                      Lock timeout value.
**      tran_id                      Transaction identifier.
**      table_name                   Table lock is associated with.
**      database_name                Database lock is associated with.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-nov-1993 (jnash)
**	    Created.  Extracted from dmd_lock() to allow usage elsewhere.
**      04-apr-1995 (dilma04)
**          Add support for tracing table lock releases: table locks may be
**          released when a table is closed, if that table was opened using
**          cursor stability or repeatable read isolation level.
**      12-nov-1997 (stial01)
**          dmd_lkrqst_trace() Added case LK_ROW, LK_VAL_LOCK.
**      19-may-1999 (stial01)
**          dmd_lkrqst_trace() UNLOCK VALUE print key4,key5 AND key6
**	09-Aug-2002 (jenjo02)
**	    Added interpretation of DMC_C_NOWAIT timeout value,
**	    brought "request_flag" bit interpretations up to date,
**	    combined case statement for LK_CONVERT with LK_REQUEST.
**      18-apr-2003 (stial01)
**          Fixed printing of DM0LBTPUT, DM0LBTDEL log records
**	26-Jan-2004 (jenjo02)
**	    Display lk_key3 (db_tab_index) without
**	    DB_TAB_PARTITION_MASK to unuglify the key, unless
**	    lock is for a temp table key. Add lk_key3 to
**	    PAGE/ROW lock displays to show possible
**	    qualifying Partition db_tab_index value, zero
**	    if not a partition.
**      22-Jul-2008 (hanal04) Bug 120674
**          Locktime was only displaying 4 characters and in the case of
**          DMC_C_NOWAIT the value of -4 was being displayed. The
**          request_flag was only display one of the settings because
**          only 4 characters were avaialble.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: added LK_TBL_MVCC
**	11-Jun-2010 (jonj) Bug 123896
**	    Add LK_SEQUENCE
*/
VOID
dmd_lkrqst_trace(
		i4	(*format_routine)(
		PTR		arg,
		i4		length,
		char 		*buffer),
LK_LOCK_KEY  *lock_key,
i4      lockid,
i4      request_type,
i4	     request_flag,
i4      lock_mode,
i4      timeout,
DB_TRAN_ID   *tran_id,
DB_TAB_NAME  *table_name,
DB_DB_NAME   *database_name)
{
    char	buffer[264];
    char        asctimeout[12];
    char	*rtype;

    if(timeout == DMC_C_NOWAIT)
    {
        /* Display NOWAIT as the timeout no as request_flag item */
        STcopy("NOWAIT", asctimeout);
    }
    else
    {
        CVla(timeout, asctimeout);
    }

    switch (request_type)
    {
    case LK_REQUEST:
    case LK_CONVERT:
       {
	  /* Lock Request/Convert. */
	rtype = (request_type == LK_REQUEST) ? "LOCK" : "CONVERT";

	if (lock_key->lk_type == LK_PAGE)
	    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
		"    %s:   PAGE  %24v Mode: %3w Timeout: %s\n            Key: (%~t,%~t,%d.%d)\n",
		rtype,
		LK_REQ_FLAG_4CH, request_flag,
		LK_LOCK_MODE_MEANING, lock_mode, asctimeout,
		sizeof(*database_name),database_name,
		sizeof(*table_name), table_name,
		(lock_key->lk_key3 & DB_TAB_PARTITION_MASK)
		    ? lock_key->lk_key3 & ~DB_TAB_PARTITION_MASK : 0,
		lock_key->lk_key4);
	else if (lock_key->lk_type == LK_TABLE)
	    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
		"    %s:   TABLE %24v Mode: %3w Timeout: %s\n            Key: (%~t,%~t)\n",
		rtype,
		LK_REQ_FLAG_4CH, request_flag,
		LK_LOCK_MODE_MEANING, lock_mode, asctimeout,
		sizeof(*database_name),database_name,
		sizeof(*table_name), table_name);
       else if (lock_key->lk_type == LK_BM_PAGE)
	    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
    "    %s:   SV_PAGE %v Mode: %3w Key: (DB=%x,TABLE=[%d,%d],PAGE=%d)\n",
		rtype,
		LK_REQ_FLAG_4CH, request_flag,
		LK_LOCK_MODE_MEANING, lock_mode,
		    lock_key->lk_key1, lock_key->lk_key2,
		    (lock_key->lk_key2 < 0)
			? lock_key->lk_key3 
			: lock_key->lk_key3 & ~DB_TAB_PARTITION_MASK,
		    lock_key->lk_key4);
       else if (lock_key->lk_type == LK_BM_DATABASE)
	    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
		    "    %s:   BM_DATABASE %v Mode: %3w Key: (DB=%x)\n",
		rtype,
		LK_REQ_FLAG_4CH, request_flag,
		    LK_LOCK_MODE_MEANING, lock_mode,
		    lock_key->lk_key1);
       else if (lock_key->lk_type == LK_BM_TABLE)
	    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
		    "    %s:   BM_TABLE %v Mode: %3w Key: (DB=%x,TABLE=%d)\n",
		rtype,
		LK_REQ_FLAG_4CH, request_flag,
		    LK_LOCK_MODE_MEANING, lock_mode,
		    lock_key->lk_key1, lock_key->lk_key2);
       else if (lock_key->lk_type == LK_ROW)
	    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
	    "    %s:   ROW   %24v Mode: %3w Timeout: %s\n            Key: (%~t,%~t,%d.%d,%d)\n",
		rtype,
		LK_REQ_FLAG_4CH, request_flag,
		LK_LOCK_MODE_MEANING, lock_mode, asctimeout,
		sizeof(*database_name),database_name,
		sizeof(*table_name), table_name,
		(lock_key->lk_key3 & DB_TAB_PARTITION_MASK)
		    ? lock_key->lk_key3 & ~DB_TAB_PARTITION_MASK : 0,
		lock_key->lk_key4, lock_key->lk_key5);
       else if (lock_key->lk_type == LK_VAL_LOCK)
	    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
       "    %s:   VALUE %v Mode: %3w Timeout: %s\n            Key: (%~t,%~t,%d,%d,%d)\n",
		rtype,
		LK_REQ_FLAG_4CH, request_flag,
		LK_LOCK_MODE_MEANING, lock_mode, asctimeout,
		sizeof(*database_name),database_name,
		sizeof(*table_name), table_name,
		lock_key->lk_key4, lock_key->lk_key5, lock_key->lk_key6);
	else if (lock_key->lk_type == LK_TBL_MVCC)
	    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
		"    %s:   MVCC  %24v Mode: %3w Timeout: %s\n            Key: (%~t,%~t)\n",
		rtype,
		LK_REQ_FLAG_4CH, request_flag,
		LK_LOCK_MODE_MEANING, lock_mode, asctimeout,
		sizeof(*database_name),database_name,
		sizeof(*table_name), table_name);
	else if (lock_key->lk_type == LK_SEQUENCE)
	    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
		"    %s:   SEQ   %24v Mode: %3w Timeout: %s\n            Key: (%~t,%~t,%d)\n",
		rtype,
		LK_REQ_FLAG_4CH, request_flag,
		LK_LOCK_MODE_MEANING, lock_mode, asctimeout,
		sizeof(*database_name),database_name,
		sizeof(*table_name), table_name,
		lock_key->lk_key2);
	}
	break;

    case LK_RELEASE:
       {
	  /* Lock Release. */

	    if (request_flag & LK_ALL)
		TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
		    "    UNLOCK: ALL     Tran-id: <%x%x>\n", 
		    tran_id->db_high_tran, tran_id->db_low_tran);
	    else if (request_flag & LK_PARTIAL)
		TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
		    "    UNLOCK: PARTIAL Key: (%~t,%~t)\n",
		    sizeof(*database_name),database_name,
		    sizeof(*table_name), table_name);
	    else
	    {
		if (lock_key->lk_type == LK_PAGE)
		    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
			"    UNLOCK: PAGE  Key: (%~t,%~t,%d.%d)\n",
			sizeof(*database_name),database_name,
			sizeof(*table_name), table_name,
			(lock_key->lk_key3 & DB_TAB_PARTITION_MASK)
			    ? lock_key->lk_key3 & ~DB_TAB_PARTITION_MASK : 0,
			lock_key->lk_key4);
                else if (lock_key->lk_type == LK_TABLE)
                    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
                        "    UNLOCK:   TABLE  Key: (%~t,%~t)\n",
                        sizeof(*database_name),database_name,
                        sizeof(*table_name), table_name);
		else if (lock_key->lk_type == LK_BM_PAGE)
		    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
    "    UNLOCK:   SV_PAGE Key: (DB=%x,TABLE=[%d,%d],PAGE=%d)\n",
		    lock_key->lk_key1, lock_key->lk_key2,
		    (lock_key->lk_key2 < 0)
			? lock_key->lk_key3 
			: lock_key->lk_key3 & ~DB_TAB_PARTITION_MASK,
		    lock_key->lk_key4);
	       else if (lock_key->lk_type == LK_BM_DATABASE)
		    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
			"    UNLOCK:   BM_DATABASE Key: (DB=%x)\n",
			lock_key->lk_key1);
	       else if (lock_key->lk_type == LK_BM_TABLE)
		    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
			"    UNLOCK:   BM_TABLE Key: (DB=%x,TABLE=%d)\n",
			lock_key->lk_key1, lock_key->lk_key2);
	       else if (lock_key->lk_type == LK_ROW)
		    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
			"    UNLOCK: ROW   Key: (%~t,%~t,%d.%d,%d)\n",
			sizeof(*database_name),database_name,
			sizeof(*table_name), table_name,
			(lock_key->lk_key3 & DB_TAB_PARTITION_MASK)
			    ? lock_key->lk_key3 & ~DB_TAB_PARTITION_MASK : 0,
			lock_key->lk_key4, lock_key->lk_key5);
	       else if (lock_key->lk_type == LK_VAL_LOCK)
		    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
			"    UNLOCK: VALUE Key: (%~t,%~t,%d,%d,%d)\n",
			sizeof(*database_name),database_name,
			sizeof(*table_name), table_name,
			lock_key->lk_key4, lock_key->lk_key5,lock_key->lk_key6);
               else if (lock_key->lk_type == LK_TBL_MVCC)
                    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
                        "    UNLOCK:   MVCC   Key: (%~t,%~t)\n",
                        sizeof(*database_name),database_name,
                        sizeof(*table_name), table_name);
               else if (lock_key->lk_type == LK_SEQUENCE)
                    TRformat(format_routine, 0, buffer, sizeof(buffer) - 1,
                        "    UNLOCK: SEQ   Key: (%~t,%~t,%d)\n",
                        sizeof(*database_name),database_name,
                        sizeof(*table_name), table_name,
			lock_key->lk_key2);
	    }
       }
       break;
    }

    return;
}

/*
** Name dmd_reptrace - check for replication server connections
**
** Description:
**	This routine simply checks wether trace point DM32 has been set in
**	this session and return TRUE or FALSE acordingly
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Retruns:
**	TRUE or FALSE
**
** History:
**	5-jun-96 (stephenb)
**	    Initial creation for DBMS replication phase 1
*/
bool
dmd_reptrace(VOID)
{
    DML_SCB     *scb;
    CS_SID	sid;

    /* Get the session trace vector. */
    CSget_sid(&sid);
    if ( (scb = GET_DML_SCB(sid)) && DMZ_MACRO(scb->scb_trace, 32) )
	return(TRUE);
    else
	return(FALSE);
}


/*
** Name dmd_pagetypes - Display create table/index page type information.
**
** Description:
**      This routine displays create table/index page type information.
**      (used by VDBA to calculate table/index disk space requirements)
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	TRUE or FALSE
**
** History:
**	6-apr-01 (stial01)
**	    created.
*/
VOID
dmd_page_types()
{
    int		i;
    i4		types[DM_MAX_CACHE];
    i4		pgsize;
    i4		pgtype;
    DB_STATUS	status;
    char	buffer[132];

    /*
    **	DM34	- Display create table/index page type information.
    **
    **  NOTE VDBA parses this output!!! If the input to dm1c_getpgtype
    **  changes, the output of DM34 must also change and the VDBA group
    **  should be notified of these changes.
    */
    for (i = 0, pgsize = 2048; pgsize <= 65536; pgsize *= 2, i++)
    {
	status = dm1c_getpgtype(pgsize, DM1C_CREATE_DEFAULT, 0, &pgtype);
	types[i] = (status == E_DB_OK) ? pgtype : DM_PG_INVALID;
    }

    TRformat(print, 0, buffer, sizeof(buffer) - 1,
	"CREATE TABLE page types 2k %d, 4k %d, 8k %d, 16k %d, 32k %d, 64k %d\n",
	types[0], types[1], types[2], types[3], types[4], types[5]);

    for (i = 0, pgsize = 2048; pgsize <= 65536; pgsize *= 2, i++)
    {
	status = dm1c_getpgtype(pgsize, DM1C_CREATE_INDEX, 0, &pgtype);
	types[i] = (status == E_DB_OK) ? pgtype : DM_PG_INVALID;
    }

    TRformat(print, 0, buffer, sizeof(buffer) - 1,
	"CREATE INDEX page types 2k %d, 4k %d, 8k %d, 16k %d, 32k %d, 64k %d\n",
	types[0], types[1], types[2], types[3], types[4], types[5]);
}

