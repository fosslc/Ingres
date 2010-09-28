/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <erglf.h>
#include    <gl.h>
#include    <pc.h>
#include    <cv.h>
#include    <cs.h>
#include    <lk.h>
#include    <sl.h>
#include    <er.h>
#include    <me.h>
#include    <st.h>
#include    <tr.h>
#include    <ex.h>
#include    <pc.h>
#include    <tm.h>
#include    <st.h>
#include    <sp.h>
#include    <mo.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <adp.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <gca.h>
#include    <ddb.h>
#include    <qefrcb.h>
#include    <psfparse.h>

#include    <duf.h>
#include    <dudbms.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmxcb.h>

#include <copy.h>
#include <qefmain.h>
#include <qefqeu.h>
#include <qefcopy.h>
#include <qefcb.h>

#include    <sc.h>
#include    <scserver.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>

#include    <sc0e.h>
#include    <sc0m.h>
#include    <scfcontrol.h>
#include    <sxf.h>

#include    <raat.h>
/*
** Name: scsraat.c - scs RAAT API call interface
**
** Description:
**	This file contains the server support for the RAAT API.  The scs_raat_call
**	function is the sequencer's entry point into the api code.  scs_raat_call
**	reads input data directly from a gca buffer stream.  The input data
**	consists of an opcode and required arguments to execute the operation.
**	The input data is tightly packed into a character buffer, so caution
**	should be exercised upon manipulation.  Format of the data MUST be
**	exactly as expected by the backend and errors will not be handled. 
**	It is therefore critcal that both front and back-end gca buffer 
**	management modifications be coordinated.  
**
**	Responses from the server always take the form of a GCA_RESPONSE
**	message type, although the data component is not the expected GCA_RE_DATA,
**	but is an API error code packed with other returned data.  Large 
**	amounts of response data, as in the case of table attribute and index
**	information needed by the front end upon table open, will span a
**	linked list of SCS_RAAT_MSG structures.  The SCS_RAAT_MSG structure
**	list is anchored off of the SCS_SSCB control block.
**
**	NOTE:  Although the GCA_QUERY message type is used to make requests of 
**	the server, data is not stored according to the gca protocol and thus
**	no claim can be made regarding compatibility with INGRES/NET.	
**
**
** History:
**      4-apr-95 (lewda02/stephenb)
**          First written.
**	11-apr-95 (lewda02)
**	    Code DMT_SHOW and DMT_OPEN calls (roughly).
**	13-apr-95 (lewda02)
**	    Work to return results from table open.
**	12 to 16-apr-95 (stephenb)
**	    Add functionality for RAAT_RECORD_POSITION, RAAT_RECORD_GET,
**	    RAAT_RECORD_PUT, RAAT_RECORD_DEL and RAAT_RECORD_REPLACE.
**	19-apr-95 (lewda02)
**	    Get and return full table info to FE upon table open.
**	27-apr-95 (stephenb/lewda02)
**	    Change the way that we get GCA info, no longer use troot, we will
**	    get the data directly from GCA block hanging off scb. Also
**	    fix up GCA return info for all operations so that we only
**	    return just enough bytes to fit data, all GCA returns are now
**	    GCA_RESPONSE.
**	1-may-95 (lewda02)
**	    Improved table name handling in table open.
**	3-may-95 (lewda02/stephenb)
**	    Added error handling.
**	4-may-95 (stephenb)
**	    Added more functionallity to record operations to perform 
**	    operations by tid, last, prev, first etc. Also fixed some
**	    last problems with code after testing.
**	10-may-95 (lewda02/shust01)
**	    Streamline code.
**	    Eliminate overhead of allocating/deallocating GCA message.
**	    Change naming convention.
**	15-may-95 (shust01)
**	    Fixed bug.  When Abort, dmx_option had to be initialized
**	    to 0, otherwise would get an error.
**	19-may-95 (lewda02/shust01)
**	    Add control over lock mode in RAAT_TABLE_OPEN. 
**	    Add RAAT_LOCK_TABLE to give locking control on an open table.
**	26-may-95 (nanpr01/shust01)
**	    Add multiple record fetch when reading sequentially. 
**	7-jun-95 (shust01)
**	    Added support for read-only transaction. check flag being passed
**          from front end.
**	    Add RAAT_LOCK_TABLE to give control of locking on an open table.
**	8-jun-1995 (lewda02/thaju02)
**	    Added support for multiple communication buffers.
**	    Optimized GCA buffer usage.
**	15-jun-95 (shust01)
**	    fixed bug involving prefetch buffers when only one record is 
**	    found.
**	15-jun-95 (thaju02)
**	    Fixed bug in returning table open info with new message usage.
**	19-jun-95 (shust01/thaju02)
**	    Fixed bug for indexes spanning more than one buffer. 
**	21-jun-95 (thaju02)
**	    Added assignment of qef_cb->qef_stat to QEF_NOTRAN during
**	    commit of transaction case, to avoid getting abort transaction
**	    errors.
**	6-jul-95 (stephenb)
**	    Add security check to ensure user has access to tables.
**	 7-jul-95 (shust01)
**	    ifdef'd out (will remove later) the case statements for TX_BEGIN,
**	    TX_ABORT, TX_COMMIT.  Now being done by quel statements.
**	17-jul-95 (stephenb)
**	    Added correct error handling, got rid of those 999 error codes.
**	11-aug-95 (shust01)
**	    fixed problem of setting current record (repositioning) when   
**	    doing a GET.  RCB was being retrieved too late.
**	14-sep-95 (shust01/thaju02)
**	    did major changes to implement BLOB interface.  Also added
**	    routines scs_raat_load(), scs_raat_parse() and 
**	    allocate_big_buffer().
**	23-oct-95 (thaju02)
**	    Cleaned up 999 error codes.
**	9-nov-95 (thaju02)
**	    Attempting to open a permanent table with temp table bitflag
**	    set, should result in a nonexistent table error.  Added temptbl.
**	10-nov-95 (thaju02)
**	    Modified table open, such that when 'tableowner.tablename' 
**	    is supplied in the raat_cb, privileges granted on table to 
**	    raat api user who is not table owner, or dba are recognized.
**	08-jul-96 (toumi01)
**	    Modified to support axp.osf (64-bit Digital Unix).
**	    Most of these changes are to handle that fact that on this
**	    platform sizeof(PTR)=8, sizeof(long)=8, sizeof(i4)=4.
**	    MEcopy is used to avoid bus errors moving unaligned data.
**	    There is also some code to allow for the need for a longer
**	    coupon for blob support, and for the fact that the coupon is
**	    not contiguous with the header in the ADP_PERIPHERAL block.
**	    The latter code is particularly nasty and sometimes involves
**	    addressing the slack (on axp.osf) after the ADP header.
**	    This is because come code in Ingres assumes (incorrectly)
**	    that 4-byte alignment will prevail anywhere and so looks just
**	    after the header for the coupon start. 
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      19-aug-1996 (sweeney)
**          Allocate GC header too, else much badness ensues.
**	16-oct-1996 (cohmi01)
**	    Resolve secondary index scan performance problem (#78981)
**	    with addition of RAAT_BAS_RECDATA flag to request that base
**	    table data be returned upon a get next/prev on secondary.
**	06-nov-1996 (cohmi01)
**	    Handle CS_OUTPUT error cases such as Force Abort. (#78797)
**	11-feb-1997 (cohmi01)
**	    Add scs_raat_term_msg() for use when terminating session (#80665)
**	    Correctly cast IIGCa_call parms to remove compiler warnings.
**	10-mar-1997 (cohmi01)
**	    Additional fix for FORCE_ABORT - set SCS_FORCE_ABORT stat so
**	    we dont get early completion in scc_gcomplete. Reset gca_status
**	    to OK so next RAAT rqst is read and purged. Remove references 
**	    SQL related items that might not be set.  (#78797) 
**	    
**      10-apr-97 (dilma04)
**          Pass RAAT caller's lock mode through dmt_lock_mode of DMT_CB
**          instead of putting it to dmt_char.char_value.
**      28-may-97 (dilma04)
**          Added handling of row locking flags.
**      01-dec-97 (stial01)
**          Added get_bkey, needed for RAAT_INTERNAL_REPOS of btree
**	24-Mar-1998 (thaal01)
**	    BLOBS & RAAT broken, this is a side effect of the fix for bug
**	    87880, E_LC0031 and session hangs. Fix provided by (thaju02)
**	    Logged as bug 89166.
**      25-May-00 (shust01)
**	    When using MK, it is possible to not receive a row level lock,
**	    when one was expected. bug #101373.
**	10-sep-1998 (schte01)
**          Modified I4ASSIGN for axp_osf to pass the value, not the address
**          of the value, to be assigned. This worked under the -oldc 
**          compiler option and got compile errors under -newc.    
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	28-dec-1999 (somsa01)
**	    Init pop_info to NULL. Also, removed unused variables.
**      02-may-2000 (stial01)
**          Set DMR_RAAT with DMR_PREV
**      03-may-2000 (hweho01)
**          Extended the changes for axp_osf to ris_u64 (AIX 64-bit).
**      05-may-2000 (stial01 for shust01)
**          Changes for B101373
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-apr-1999 (hanch04)
**	    Changes for 64 bit pointers
**	07-sept-2000 (hanje04)
**	    Added axp_lnx (Alpha Linux) to 64-bit axp_osf changes.
**      22-Mar-2001 (stial01)
**          Call adu_free_objects to delete ALL temp etabs, instead of calling
**          adu_lo_delete for a specific temp etab. adu_free_objects will 
**          destroy all temp etabs (except session temp table etabs). (b104317)
**	13-oct-2001 (somsa01)
**	    Fixed up 64-bit compiler warnings.
**	05-nov-2001 (somsa01)
**	    Due to problems on UNIX platforms with LP64 defined, migrated
**	    64-bit Windows changes into #ifdef blocks.
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/* Forward declarations */


static STATUS scs_raat_bigbuffer(SCD_SCB *scb, i4 buf_size);

/*
** Name: scs_raat_call - scs entry to raat thread.
**
** Description:
**      This route is the entry point to the raat thread processing code.
**      SCF will call this when the language type in the query is DB_NDMF.
**
** Inputs:
**      op_code         SCF present state
**      scb		Session control block pointer
**
** Outputs:
**	next_op		SCF next state (output)
**
** Returns:
**      DB_STATUS 
**
** History:
**      4-apr-95 (lewda02/stephenb)
**          First written.
**	11-apr-95 (lewda02)
**	    Code DMT_SHOW and DMT_OPEN calls (roughly).
**	13-apr-95 (lewda02)
**	    Place some result data in message queue for SCF.
**	    Thus far, we only return an RCB pointer.
**	    Repair a few miscellaneous bugs.
**	12 to 16-apr-95 (stephenb)
**	    Add functionality for RAAT_RECORD_POSITION, RAAT_RECORD_GET,
**	    RAAT_RECORD_PUT, RAAT_RECORD_DEL and RAAT_RECORD_REPLACE.
**	19-apr-95 (lewda02)
**	    Get full table info upon table open and return to FE.
**	    Free memory when done.
**	1-may-95 (lewda02)
**	    Improved table name handling in table open.  Table name can
**	    be provided as a null terminated string with an optional
**	    owner name and dot precedeing it.  The standard algorithm is
**	    used to locate the table via its owner and name.
**	3-may-95 (lewda02/stephenb)
**	    Added error handling.
**	4-may-95 (stephenb)
**	    Added more functionallity to record operations to perform 
**	    operations by tid, last, prev, first etc. Also fixed some
**	    last problems with code after testing.
**	15-may-95 (shust01)
**	    Fixed bug.  When Abort, dmx_option had to be initialized
**	    to 0, otherwise would get an error.
**	19-may-95 (lewda02/shust01)
**	    Add control over lock mode in RAAT_TABLE_OPEN. 
**	    Add RAAT_LOCK_TABLE to give locking control on an open table.
**	26-may-95 (nanpr01/shust01)
**	    Add multiple record fetch when reading sequentially. 
**	    Add RAAT_LOCK_TABLE to give control of locking on an open table.
**      8-jun-1995 (lewda02/thaju02)
**          Added support for multiple communication buffers.
**          Optimized GCA buffer usage.
**	15-jun-95 (shust01)
**	    fixed bug involving prefetch buffers when only one record is 
**	    found.
**	15-jun-95 (thaju02)
**	    Fixed bug in returning table open info with new message usage.
**	21-jun-95 (thaju02)
**	    Added assignment of qef_cb->qef_stat to QEF_NOTRAN during
**	    commit of transaction case.
**	3-jul-95 (stephenb)
**	    ics_sessid is fixed length not null terminated, this causes problems
**	    with CVahxl() when groups are used (group name comes directly
**	    after session id in memory). Null terminated this string before
**	    calling CVahxl().
**	6-jul-95 (stephenb)
**	    Add security check to ensure user has access to tables.
**	7-jul-95 (shust01)
**	    ifdef'd out (will remove later) the case statements for TX_BEGIN,
**	    TX_ABORT, TX_COMMIT.  Now being done by quel statements.
**	17-jul-95 (stephenb)
**	    Added correct error handling, got rid of those 999 error codes.
**	11-aug-95 (shust01)
**	    fixed problem of setting current record (repositioning) when   
**	    doing a GET.  RCB was being retrieved too late.
**	12-sep-95 (stephenb)
**	    Fixed security checking, we can't check access to an index since
**	    the user cannot use grant on an index, in this case we realy should
**	    check access to the relavent base table. Since the DBMS is not
**	    geared around users opening indecies directly getting the base
**	    table info isn't easy, we probably need to check the relid of the
**	    index table and go get info on table with the same relid and 
**	    a relidx of 0 (nasty!), for now we just won't check access to 
**	    indecies. @FIX_ME@ this need to be fixed properly sometime
**	    in the furure.
**	14-sep-95 (shust01/thaju02)
**	    did major changes to implement BLOB interface.  Also added
**	    routines scs_raat_load(), scs_raat_parse() and 
**	    scs_raat_bigbuffer().
**	29-sep-95 (thaju02)
**	    Implemented using temporary tables.  Modified table open
**	    such that if table to be opened is a temporary table the
**	    temp table owner must be the session identifier found in
**	    the PSS_SESSBLK pointed to by scb->scb_sscb.sscb_psscb.
**      23-oct-95 (thaju02)
**          Cleaned up 999 error codes.
**      9-nov-95 (thaju02)
**          Attempting to open a permanent table with temp table bitflag
**          set, should result in a nonexistent table error.  Added temptbl.
**      10-nov-95 (thaju02)
**          Modified table open, such that when 'tableowner.tablename' 
**          is supplied in the raat_cb, privileges granted on table to 
**          raat api user who is not table owner, or dba are recognized.
**	16-oct-1996 (cohmi01)
**	    Resolve secondary index scan performance problem (#78981)
**	    with addition of RAAT_BAS_RECDATA flag to request that base
**	    table data be returned upon a get next/prev on secondary.
**	06-nov-1996 (cohmi01)
**	    Handle CS_OUTPUT error cases such as Force Abort. (#78797)
**	10-mar-1997 (cohmi01)
**	    Additional fix for FORCE_ABORT - set SCS_FORCE_ABORT stat so
**	    we dont get early completion in scc_gcomplete. Reset gca_status
**	    to OK so next RAAT rqst is read and purged. Remove references 
**	    SQL related items that might not be set.  (#78797) 
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      10-apr-97 (dilma04)
**          Pass RAAT caller's lock mode through dmt_lock_mode of DMT_CB
**          instead of putting it to dmt_char.char_value.
**      28-may-97 (dilma04)
**          Added handling of row locking flags.
**      24-Mar-1998 (thaal01)
**          Bug #89166 - changed ADP_POP_LONG_TEMP to ADP_POP_SHORT_TEMP.
**	12-jun-1998 (thaju02)
**	    changed pop_temporary=ADP_POP_LONG_TEMP, regression bug
**	    (B91469)
**	10-sep-1998 (schte01)
**          Modified I4ASSIGN for axp_osf to pass the value, not the address
**          of the value, to be assigned. This worked under the -oldc 
**          compiler option and got compile errors under -newc.    
**	28-oct-1998 (somsa01)
**	    In the case of Global Session Temporary Tables, make sure that
**	    DMF knows about this.  (Bug #94059)
**	28-dec-1999 (somsa01)
**	    Init pop_info to NULL.
**      1-Jul-2000 (wanfr01)
**        Bug 101521, INGSRV 1226 - Pass DMR_RAAT flag to to tell dmf that
**	  this query came from RAAT
**      10-Jan-2002 (thaju02/bolke01)
**         Bug 106696,INGSRV   Problem #: 1637 - prior to calling dmt_open() via
**	   dmf_call(), dmt_cb.dmt_mustlock (boolean) must be initialized to FALSE.
**	   If dmt_cb.dmt_mustlock is not FALSE, then we will revert to page level
**	   locking. This is determine in dmt_set_lock_values(). 
**	11-May-2004 (schka24)
**	    Name change for pop-temporary setting.
**	19-Jul-2004 (schka24)
**	    Don't allow open of partitioned tables from RAAT.
**	28-Aug-2009 (kschendel) b121804
**	    Rename allocate-big-buffer to avoid conflict with
**	    front-end routine.  Fix a broken call to it.
**	26-Jul-2010 (hweho01) SIR 123485
**	    Add dmf_call with DMPE_QUERY_END after the DMR_PUT operation, to  
**	    close the load table.  
**	
*/
DB_STATUS
scs_raat_call(i4  op_code,
	    SCD_SCB  *scb,
	    i4  *next_op )
{
    i4		query_type;
    char		*inbuf;
    char		*outbuf, *savebuf;
    SCC_GCMSG           *message;
    STATUS		status = OK;
    DB_STATUS           error = E_DB_OK;
    i4		err_code = 0;
    i4 		flag;
    i4		i, j;
    SCS_BIG_BUF 	*blob_mptr;
    SCS_RAAT_MSG	*raat_mptr;
    char		tmp_str[CV_HEX_PTR_SIZE + 1];





    *next_op = CS_EXCHANGE;
    scb->scb_sscb.sscb_state = SCS_INPUT;

    switch (op_code)
    {
	case CS_INITIATE:
            *next_op = CS_INPUT;
            break;
 
	case CS_TERMINATE:
	    break;

	case CS_INPUT:
	    /*
	    ** Grab RAAT op code and subsequent data from buffer
	    */
	    blob_mptr = scb->scb_sscb.sscb_big_buffer;
	    inbuf = (char *)(((GCA_Q_DATA *)
	       ((SCC_GCMSG *)&blob_mptr->message)->scg_marea)->gca_qdata);
    
            query_type = *((i4 *)inbuf);
	    /*
	    ** Need to null terminate session ID here before using
	    ** CVahxl().
	    */
	    MEcopy(scb->scb_sscb.sscb_ics.ics_sessid, 
	    CV_HEX_PTR_SIZE, tmp_str);
	    tmp_str[CV_HEX_PTR_SIZE] = '\0';
	    STtrmwhite(tmp_str);

	    switch (query_type)
	    {
#ifdef DONE_BY_QUEL
		case RAAT_TX_BEGIN:
		{
		    DMX_CB	        dmx_cb;
		    QEF_CB		*qef_cb;
		    i4             flags;
		    
		    /*
		    ** get flag from front end.  
		    ** Either RAAT_TX_READ or RAAT_TX_WRITE. 
		    */
		    inbuf += sizeof(i4);
		    flags = *(i4 *)inbuf;
		    /*
		    ** information.
		    */
		    dmx_cb.type = DMX_TRANSACTION_CB;
		    dmx_cb.length = sizeof(DMX_CB);

		    if (flags & RAAT_TX_READ)
			dmx_cb.dmx_flags_mask = DMX_READ_ONLY;
		    else if (flags & RAAT_TX_WRITE)
			dmx_cb.dmx_flags_mask = 0;
		    else
		    {
			/* FIX_ME: report bad operation error */
			err_code = E_SC0371_NO_TX_FLAG;
			break;
		    }

#if defined(LP64)
		    CVahxi64(tmp_str, &dmx_cb.dmx_session_id);
#else
		    CVahxl(tmp_str, &dmx_cb.dmx_session_id);
#endif  /* LP64 */
		    dmx_cb.dmx_db_id = scb->scb_sscb.sscb_ics.ics_opendb_id;
		    dmx_cb.dmx_option = DMX_USER_TRAN;

		    status = dmf_call(DMX_BEGIN, &dmx_cb);
		    if (status)
		    {
			err_code = dmx_cb.error.err_code;
			break;
		    }
		    qef_cb = scb->scb_sscb.sscb_qescb;
		    qef_cb->qef_dmt_id = dmx_cb.dmx_tran_id;
		    STRUCT_ASSIGN_MACRO(dmx_cb.dmx_phys_tran_id,
			qef_cb->qef_tran_id);
		    qef_cb->qef_stmt = 1;
		    qef_cb->qef_defr_curs = FALSE;

		    /*
		    ** convert single statement transaction to multi
		    ** statement if autocommit is off.
		    */
		    if (qef_cb->qef_auto == QEF_OFF && qef_cb->qef_rcb &&
			qef_cb->qef_rcb->qef_modifier == QEF_SSTRAN)
			qef_cb->qef_stat = QEF_MSTRAN;
		    else
			if (qef_cb->qef_rcb)
			    qef_cb->qef_stat = qef_cb->qef_rcb->qef_modifier;
			else
			    qef_cb->qef_stat = QEF_MSTRAN;
 
		    /*
		    ** Must return info to client.
		    */
		    if (scb->scb_sscb.sscb_raat_message == NULL)
		    {
			status = allocate_message(scb,
			    &scb->scb_sscb.sscb_raat_message);
			if (status)
			{
			    err_code = E_SC0372_NO_MESSAGE_AREA;
			    break;
			}
		    }
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
		    message->scg_msize = sizeof(i4);
		    message->scg_mask = SCG_NODEALLOC_MASK;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send only response, nothing additional.
		    */
		    *((i4 *)outbuf) = err_code;

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		case RAAT_TX_COMMIT:
		{
		    DMX_CB	dmx_cb;
		    QEF_CB	*qef_cb;

		    qef_cb = scb->scb_sscb.sscb_qescb;
		    dmx_cb.type = DMX_TRANSACTION_CB;
		    dmx_cb.length = sizeof(DMX_CB);
		    dmx_cb.dmx_tran_id = qef_cb->qef_dmt_id;

		    status = dmf_call(DMX_COMMIT, &dmx_cb);
		    if (status)
		    {
			err_code = dmx_cb.error.err_code;
			break;
		    }
		    if (qef_cb->qef_rcb)
			STRUCT_ASSIGN_MACRO(dmx_cb.dmx_commit_time,
			qef_cb->qef_rcb->qef_comstamp);

                    qef_cb->qef_stat = QEF_NOTRAN;

		    /*
		    ** Must return info to client.
		    */
		    if (scb->scb_sscb.sscb_raat_message == NULL)
		    {
			status = allocate_message(scb,
			    &scb->scb_sscb.sscb_raat_message);
			if (status)
			{
			    err_code = E_SC0372_NO_MESSAGE_AREA;
			    break;
			}
		    }
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
		    message->scg_msize = sizeof(i4);
		    message->scg_mask = SCG_NODEALLOC_MASK;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send only response, nothing additional.
		    */
		    *((i4 *)outbuf) = err_code;

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		case RAAT_TX_ABORT:
		{
		    DMX_CB	dmx_cb;
		    QEF_CB	*qef_cb;

		    qef_cb = scb->scb_sscb.sscb_qescb;
		    dmx_cb.type = DMX_TRANSACTION_CB;
		    dmx_cb.length = sizeof(DMX_CB);
		    dmx_cb.dmx_tran_id = qef_cb->qef_dmt_id;
		    dmx_cb.dmx_option  = 0;

		    status = dmf_call(DMX_ABORT, &dmx_cb);
		    if (status)
		    {
			err_code = dmx_cb.error.err_code;
			break;
		    }
		    qef_cb->qef_open_count = 0;     /* no cursors opened */
		    qef_cb->qef_defr_curs = FALSE;  /* no deferred cursor */
		    qef_cb->qef_abort = FALSE;
		    if (!(qef_cb->qef_rcb && qef_cb->qef_rcb->qef_spoint &&
			qef_cb->qef_abort == FALSE))
			qef_cb->qef_stat = QEF_NOTRAN;


		    /*
		    ** Must return info to client.
		    */
		    if (scb->scb_sscb.sscb_raat_message == NULL)
		    {
			status = allocate_message(scb,
			    &scb->scb_sscb.sscb_raat_message);
			if (status)
			{
			    err_code = E_SC0372_NO_MESSAGE_AREA;
			    break;
			}
		    }
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
		    message->scg_msize = sizeof(i4);
		    message->scg_mask = SCG_NODEALLOC_MASK;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send only response, nothing additional.
		    */
		    *((i4 *)outbuf) = err_code;

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}
#endif /* DONE_BY_QUEL */
		    
		case RAAT_TABLE_OPEN:
		{
		    char		*tname;
		    i4			owner_given = FALSE;
		    DMT_SHW_CB		dmt_shw_cb;
		    DMT_TBL_ENTRY	*dmt_tbl;
    		    DMT_CB		dmt_cb;
		    DMT_CHAR_ENTRY      dmt_char;
		    DM_PTR		*dm_ptr;
		    i4                  attr_count;
		    i4                  index_count;
		    i4			num_attr;
		    i4			num_index;
		    i4			attr_per_buf;
		    i4			index_per_buf;
		    i4		flags;
		    i4			buf_used;
		    i4			buf_avail;
		    i4			num_buf = 1;
		    PSQ_CB		psq_cb;
		    i4			temptbl = FALSE;


		    if (scb->scb_sscb.sscb_raat_message == NULL)
		    {
			status = allocate_message(scb,
			    &scb->scb_sscb.sscb_raat_message);
			if (status)
			{
			    err_code = E_SC0372_NO_MESSAGE_AREA;
			    break;
			}
		    }

		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
	    	    outbuf = (char *)message->scg_marea;
		    buf_used = 0;

		    /*
		    ** Build control block for DMT_SHOW to get table
		    ** information.
		    */
		    inbuf += sizeof(i4);
		    flags = *(i4 *)inbuf;
		    dmt_shw_cb.length = sizeof(DMT_SHW_CB);
	            dmt_shw_cb.type = DMT_SH_CB;
#if defined(axp_osf) || defined(ris_u64) || defined(axp_lnx)
		    CVahxl8(tmp_str, (u_i4 *)&dmt_shw_cb.dmt_session_id);
#else
#if defined(LP64)
		    CVahxi64(tmp_str, &dmt_shw_cb.dmt_session_id);
#else
		    CVahxl(tmp_str, (u_i4 *)&dmt_shw_cb.dmt_session_id);
#endif  /* LP64 */
#endif
		    dmt_shw_cb.dmt_db_id = 
			scb->scb_sscb.sscb_ics.ics_opendb_id;
		    inbuf += sizeof(i4);
		    tname = STchr(inbuf, '.');
		    if (tname != NULL)
		    {
			*tname = '\0';
			tname++;
			STmove(inbuf, ' ', DB_OWN_MAXNAME,
			    (char *)&dmt_shw_cb.dmt_owner);
			owner_given = TRUE;
		    }
		    else
		    {
			tname = inbuf;
		        dmt_shw_cb.dmt_owner = 
			    *scb->scb_sscb.sscb_ics.ics_eusername;
		    }

		    /*
		    ** if temporary table specified, modify the
		    ** table owner to that of the session identifier	
		    ** specified in the PSS_SESSBLK.
		    */
		    if (flags & RAAT_TEMP_TBL)
		    {
                        status = psq_call(PSQ_SESSOWN, &psq_cb, NULL);
			
                        if (status) 
                        {
                            err_code = psq_cb.psq_error.err_code;
                            break;
                        }
			STmove(psq_cb.psq_owner, ' ', 
			    DB_OWN_MAXNAME, (char *)&dmt_shw_cb.dmt_owner);
			temptbl = TRUE;
		    }

		    STmove(tname, ' ', DB_TAB_MAXNAME,
			(char *)&dmt_shw_cb.dmt_name);
		    dmt_shw_cb.dmt_flags_mask = DMT_M_TABLE | DMT_M_NAME;
		    dmt_shw_cb.dmt_record_access_id = NULL;
		    dmt_shw_cb.dmt_char_array.data_address = NULL;
#if defined(i64_win)
		    buf_used += 2 * sizeof(i4) + sizeof(OFFSET_TYPE);
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx)
		    buf_used += 2 * sizeof(i4) + sizeof(long);
#else
		    buf_used += 3 * sizeof(i4);
#endif
		    dmt_shw_cb.dmt_table.data_address = outbuf + buf_used;
		    buf_used += sizeof(DMT_TBL_ENTRY);
		    dmt_shw_cb.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
		    status = dmf_call(DMT_SHOW, &dmt_shw_cb);
		    if (status != E_DB_OK)
		    {
			if (!owner_given  && !temptbl && 
			    dmt_shw_cb.error.err_code ==
			    E_DM0054_NONEXISTENT_TABLE)
			{
			    dmt_shw_cb.dmt_owner =
				scb->scb_sscb.sscb_ics.ics_dbowner;
			    status = dmf_call(DMT_SHOW, &dmt_shw_cb);
			}
			if (status != E_DB_OK)
			{
			   err_code = dmt_shw_cb.error.err_code;
			   break;
			}
		    }

		    dmt_tbl =
			(DMT_TBL_ENTRY *)dmt_shw_cb.dmt_table.data_address;

		    /* 
		    ** Due to addition of new request type (BAS_RECDATA)
		    ** we need to provide feedback to client prog as to
		    ** whether this is available in current server or not,
		    ** in a way that is backwards compatible with older
		    ** clients. Since no spot in the message area was ever
		    ** allocated for this purpose, overload the view queryid
		    ** field for this purpose, since RAAT is incompatible
		    ** with views (and everything else) anyway. The front
		    ** end code will change it back to zero just for form.
		    */
		    dmt_tbl->tbl_qryid.db_qry_low_time = RAAT_VERSION_3;

		    /* If partitioned, just say no access to table.
		    ** It's not worth inventing a new error code.
		    */
		    if (dmt_tbl->tbl_status_mask & DMT_IS_PARTITIONED)
		    {
			status = E_DB_WARN;
			err_code = W_SC0374_NO_ACCESS_TO_TABLE;
			break;
		    }

		    /*
		    ** Check user's access to table using PSF
		    ** for base tables only.
		    ** @FIX_ME@:
		    ** we should realy check access for index tables as well
		    ** but grant cannot be used on index tables and they
		    ** therefore have no privileges on them, the normal
		    ** state of affiars is to allow users access to 
		    ** indecies if they have access to the base table. This
		    ** is normally all dealt with neatly in DMF since we always 
		    ** have to open the base table first for all normal queries,
		    ** the RAAT realy screws this up since a user can
		    ** indipendently open an index. So at this point what
		    ** we should realy do is somehow find the base table name
		    ** and do the check on the base table. That's not too easy 
		    ** from this position. The only possible answer is to
		    ** get the relid of the index and look for a table with
		    ** the same relid and a relidx of 0 (nasty!).
		    */
		    if (((dmt_tbl->tbl_status_mask & DMT_IDX) == 0) &&
			((flags & RAAT_TEMP_TBL) == 0))
		    {
		    	psq_cb.psq_next = (PSQ_CB *)NULL;
		    	psq_cb.psq_prev = (PSQ_CB *)NULL;
		    	psq_cb.psq_length = sizeof(PSQ_CB);
		    	psq_cb.psq_type = PSQCB_CB;
		    	psq_cb.psq_ascii_id = PSQCB_ASCII_ID;
		    	STRUCT_ASSIGN_MACRO(dmt_shw_cb.dmt_name, 
			    psq_cb.psq_tabname);
			if (owner_given)
			    STRUCT_ASSIGN_MACRO(dmt_shw_cb.dmt_owner,
				psq_cb.psq_user.db_tab_own);
			else
		    	    psq_cb.psq_user.db_tab_own.db_own_name[0] = '\0';
		    	status = psq_call(PSQ_RESOLVE, &psq_cb, NULL);
		    	if (status == E_DB_WARN) /* user has no access */
		    	{
			    err_code = W_SC0374_NO_ACCESS_TO_TABLE;
			    break;
		        }
		        else if (status != E_DB_OK) /* PSF error */
		        {
			    err_code = E_SC0373_PSF_RESOLVE_ERROR;
			    break;
		    	}
		    }

		    /*
		    ** Build control block for DMT_OPEN
		    */
		    dmt_cb.type			= DMT_TABLE_CB;
		    dmt_cb.length		= sizeof(DMT_CB);
		    dmt_cb.dmt_db_id		= 
			scb->scb_sscb.sscb_ics.ics_opendb_id;
		    dmt_cb.dmt_tran_id		= scb->scb_sscb.sscb_qescb->qef_dmt_id;
		    dmt_cb.dmt_id		= dmt_tbl->tbl_id;
		    dmt_cb.dmt_sequence		= scb->scb_sscb.sscb_qescb->qef_stmt;
		    dmt_cb.dmt_flags_mask	= 0;
		    if (flags & RAAT_TEMP_TBL)
			dmt_cb.dmt_flags_mask |= DMT_SESSION_TEMP;
		    dmt_cb.dmt_update_mode = DMT_U_DIRECT;
		    dmt_cb.dmt_lock_mode = DMT_IS;
		    dmt_cb.dmt_mustlock = FALSE;
		    dmt_cb.dmt_char_array.data_address = (PTR)&dmt_char;
		    dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
		    
		    dmt_char.char_id = DMT_C_LOCKMODE;
		    /*
		    ** Figure out locking characteristics.
		    */
		    switch (flags & RAAT_LK_MASK) 
		    {
			case RAAT_LK_XT:
			    dmt_cb.dmt_lock_mode = DMT_X;
			    dmt_cb.dmt_access_mode = DMT_A_WRITE;
			    break;

			case RAAT_LK_ST:
			    dmt_cb.dmt_lock_mode  = DMT_S;
			    dmt_cb.dmt_access_mode = DMT_A_READ;
			    break;

			case RAAT_LK_XP:
			    dmt_cb.dmt_lock_mode = DMT_IX;
			    dmt_cb.dmt_access_mode = DMT_A_WRITE;
			    break;

			case RAAT_LK_SP:
			    dmt_cb.dmt_lock_mode = DMT_IS;
			    dmt_cb.dmt_access_mode = DMT_A_READ;
			    break;

                        case RAAT_LK_XR:
                            dmt_cb.dmt_lock_mode = DMT_RIX;
                            dmt_cb.dmt_access_mode = DMT_A_WRITE;
                            break;
 
                        case RAAT_LK_SR:
                            dmt_cb.dmt_lock_mode = DMT_RIS;
                            dmt_cb.dmt_access_mode = DMT_A_READ;
                            break;

			case RAAT_LK_NL:
			    dmt_cb.dmt_lock_mode = DMT_N;
			    dmt_cb.dmt_access_mode = DMT_A_READ;
			    break;

			default:
			    dmt_cb.dmt_char_array.data_address = NULL;
			    dmt_cb.dmt_access_mode = DMT_A_READ;
			    break;
		    }


		    status = dmf_call(DMT_OPEN, &dmt_cb);
		    if (status)
		    {
			err_code = dmt_cb.error.err_code;
			break;
		    }

		    /*
		    ** Now finish getting table info.  This time use RCB.
		    */
		    dmt_shw_cb.dmt_flags_mask =
                        DMT_M_ATTR | DMT_M_ACCESS_ID | DMT_M_MULTIBUF;
                    raat_mptr = scb->scb_sscb.sscb_raat_message;
                    raat_mptr->message.scg_mask =
			(SCG_NODEALLOC_MASK | SCG_NOT_EOD_MASK);
		    attr_count = num_attr = dmt_tbl->tbl_attr_count;
		    dm_ptr = &dmt_shw_cb.dmt_attr_array;
		    dm_ptr->ptr_address = outbuf + buf_used;
		    dm_ptr->ptr_in_count = 0;
		    dm_ptr->ptr_out_count = 0;
		    dm_ptr->ptr_size = 0;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
		    dm_ptr->ptr_size_filler = 0;
#endif

		    while (num_attr)
		    {
/* FIX ME !!!!! what should be subtracted from comm_size that
		will equal 4040 ? */
		    	buf_avail = scb->scb_cscb.cscb_comm_size - buf_used;
		        attr_per_buf = (num_attr >
			    buf_avail/sizeof(DMT_ATT_ENTRY)) ?
			    (buf_avail/sizeof(DMT_ATT_ENTRY)) : num_attr;

		        if (attr_per_buf == 0)
		        {
		    	    /* unable to fit any attributes in the leftover
		    	    ** buffer space, need to find/allocate more buffers.
			    */
			    message->scg_msize = buf_used;
                            if (raat_mptr->next == NULL)
                            {
                                status = allocate_message(scb,
				    &raat_mptr->next);
                                if (status)
                                {
                                    err_code = E_SC0372_NO_MESSAGE_AREA;
                                    break;
                                }
                            }
			    raat_mptr = raat_mptr->next;
			    message = (SCC_GCMSG *)&raat_mptr->message;
			    message->scg_mask =
				(SCG_NODEALLOC_MASK | SCG_NOT_EOD_MASK);
			    outbuf = (char *)message->scg_marea;
			    buf_used = 0;
#if defined(i64_win)
			    {
				OFFSET_TYPE axp_temp = (OFFSET_TYPE)(outbuf +
				    scb->scb_cscb.cscb_comm_size);
				MEcopy(&axp_temp, sizeof(OFFSET_TYPE),
				    &dm_ptr->ptr_size);
			    }
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx)
			    {
				long axp_temp = (long)(outbuf +
				    scb->scb_cscb.cscb_comm_size);
				MEcopy(&axp_temp, sizeof(long),
				    &dm_ptr->ptr_size);
			    }
#else
		            dm_ptr->ptr_size = (i4)(outbuf +
			        scb->scb_cscb.cscb_comm_size);
#endif

			    /* Working on new dm_ptr */
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
			    MEcopy(&dm_ptr->ptr_size, sizeof(PTR), &dm_ptr);
#else
			    dm_ptr = (DM_PTR *)dm_ptr->ptr_size;
#endif
			    dm_ptr->ptr_address = outbuf;
			    dm_ptr->ptr_out_count = 0;
			    dm_ptr->ptr_size = 0;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
			    dm_ptr->ptr_size_filler = 0;
#endif

			    num_buf++;
		        }
		        else
		        {
			    /* able to fit at least one attribute in buffer. */
		            dm_ptr->ptr_in_count = attr_per_buf;
		        }

		        num_attr -= attr_per_buf;
			buf_used += attr_per_buf * sizeof(DMT_ATT_ENTRY);
		    }

		    index_count = num_index = dmt_tbl->tbl_index_count;
		    dm_ptr = &dmt_shw_cb.dmt_index_array;
		    dm_ptr->ptr_address = outbuf + buf_used;
                    dm_ptr->ptr_in_count = 0;
                    dm_ptr->ptr_size = 0;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
                    dm_ptr->ptr_size_filler = 0;
#endif


		    if (dmt_tbl->tbl_index_count)
		    {
			dmt_shw_cb.dmt_flags_mask |= DMT_M_INDEX;
		        while (num_index)
		        {
/* FIX ME !!!!! what should be subtracted from comm_size that
		will equal 4040 ? */
		    	    buf_avail = scb->scb_cscb.cscb_comm_size - buf_used;
		            index_per_buf = (num_index >
			        buf_avail/sizeof(DMT_IDX_ENTRY)) ?
			        (buf_avail/sizeof(DMT_IDX_ENTRY)) : num_index;
    
		            if (index_per_buf == 0)
		            {
		    	        /* unable to fit any indices in the leftover
		    	        ** buffer space, need to find/allocate
				** more buffers.
			        */
			        message->scg_msize = buf_used;
                                if (raat_mptr->next == NULL)
                                {
                                    status = allocate_message(scb,
				        &raat_mptr->next);
                                    if (status)
                                    {
                                        err_code = E_SC0372_NO_MESSAGE_AREA;
                                        break;
                                    }
                                }
			        raat_mptr = raat_mptr->next;
			        message = (SCC_GCMSG *)&raat_mptr->message;
			        message->scg_mask =
				    (SCG_NODEALLOC_MASK | SCG_NOT_EOD_MASK);
			        outbuf = (char *)message->scg_marea;
			        buf_used = 0;
#if defined(i64_win)
				{
				    OFFSET_TYPE axp_temp =
					(OFFSET_TYPE)(outbuf +
					scb->scb_cscb.cscb_comm_size);
				    MEcopy(&axp_temp, sizeof(OFFSET_TYPE),
					&dm_ptr->ptr_size);
				}
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx)
				{
				    long axp_temp =
					(long)(outbuf +
					scb->scb_cscb.cscb_comm_size);
				    MEcopy(&axp_temp, sizeof(long),
					&dm_ptr->ptr_size);
				}
#else
		                dm_ptr->ptr_size = (i4)(outbuf +
			            scb->scb_cscb.cscb_comm_size);
#endif
    
			        /* Working on new dm_ptr */
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
				MEcopy(&dm_ptr->ptr_size, sizeof(PTR), &dm_ptr);
#else
			        dm_ptr = (DM_PTR *)dm_ptr->ptr_size;
#endif
			        dm_ptr->ptr_address = outbuf;
			        dm_ptr->ptr_out_count = 0;
			        dm_ptr->ptr_size = 0;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
				dm_ptr->ptr_size_filler = 0;
#endif

				num_buf++;
		            }
		            else
		            {
			        /* able to fit at least one attribute in buffer. */
		                dm_ptr->ptr_in_count = index_per_buf;
		            }

		            num_index -= index_per_buf;
			    buf_used += index_per_buf * sizeof(DMT_IDX_ENTRY);
		        }
		    }
		    message->scg_msize = buf_used;
		    message->scg_mask &= ~SCG_NOT_EOD_MASK;

		    dmt_shw_cb.dmt_record_access_id =
			dmt_cb.dmt_record_access_id;
		    dmt_shw_cb.dmt_char_array.data_address = NULL;

		    status = dmf_call(DMT_SHOW, &dmt_shw_cb);
                    if (status != E_DB_OK)
                    {
			err_code = dmt_shw_cb.error.err_code;
			break;
                    }

		    /*
		    ** Must return info to client.
		    */
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send full table info.
		    */
		    *((i4 *)outbuf) = err_code;

		    outbuf += sizeof(i4);
#if defined(i64_win)
		    MEcopy(&dmt_cb.dmt_record_access_id, sizeof(OFFSET_TYPE),
			   outbuf);
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx)
		    MEcopy(&dmt_cb.dmt_record_access_id, sizeof(long),
			   outbuf);
#else
		    *((i4 *)outbuf) = (i4)dmt_cb.dmt_record_access_id;
#endif


		    /* send access mode flag back. TRUE if read mode */
#if defined(i64_win)
		    outbuf += sizeof(OFFSET_TYPE);
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx)
		    outbuf += sizeof(long);
#else
		    outbuf += sizeof(i4);
#endif
		    *((i4 *)outbuf) =
                       (dmt_cb.dmt_access_mode == DMT_A_WRITE) ? FALSE : TRUE;

		    /*
		    ** Queue up message(s) for SCF to return to FE.
		    */
		    for ( ; num_buf>0;
			num_buf--, raat_mptr = raat_mptr->next,
			message = (SCC_GCMSG *)&raat_mptr->message)
		    {
	                message->scg_next =
			    (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		        message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		        scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		        scb->scb_cscb.cscb_mnext.scg_prev = message;
		    }

		    return (E_DB_OK);
		}

		case RAAT_TABLE_LOCK:
		{
		    DMR_CB		dmr_cb;
		    DMR_CHAR_ENTRY	dmr_char[4];
		    i4		flags;
		    i4		rcb_lowtid  = 0;
		    i4		current_tid = 0;
		    i4		access_mode;

		    /*
		    ** Build control block for DMR_ALTER
		    */
		    dmr_cb.type			= DMR_RECORD_CB;
		    dmr_cb.length		= sizeof(DMR_CB);
		    dmr_cb.dmr_flags_mask	= 0;

		    inbuf += sizeof(i4);
#if defined(i64_win)
		    MEcopy(inbuf, sizeof(OFFSET_TYPE), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(OFFSET_TYPE);
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx) 
		    MEcopy(inbuf, sizeof(long), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(long);
#else
		    dmr_cb.dmr_access_id	= *((char **)inbuf);
		    inbuf += sizeof(i4);
#endif
		    flags = *((i4 *)inbuf);
		    inbuf += sizeof(i4);

		    dmr_cb.dmr_char_array.data_address = (char *)dmr_char;
		    dmr_cb.dmr_char_array.data_in_size =
			sizeof(DMR_CHAR_ENTRY);
		    
		    dmr_char[0].char_id = DMR_LOCK_MODE;
		    switch (flags & RAAT_LK_MASK)
                    {
                        case RAAT_LK_XT:
                            dmr_char[0].char_value = DMT_X;
                            break;
 
                        case RAAT_LK_ST:
                            dmr_char[0].char_value = DMT_S;
                            break;

			case RAAT_LK_XP:
                            dmr_char[0].char_value = DMT_IX;
                            break;
 
                        case RAAT_LK_SP:
                            dmr_char[0].char_value = DMT_IS;
                            break;

                        case RAAT_LK_XR:
                            dmr_char[0].char_value = DMT_RIX;
                            break;
 
                        case RAAT_LK_SR:
                            dmr_char[0].char_value = DMT_RIS;
                            break;
 
                        case RAAT_LK_NL:
                            dmr_char[0].char_value = DMT_N;
                            break;
 
                        default:
			    err_code = W_SC0375_INVALID_LOCK_MODE;
                            break;
                    }
		    if (err_code)
			break;

		    /*
		    ** if RAAT_INTERNAL_REPOS flag set, possibly doing 
		    ** a reposition due to escalating to a write-lock.
		    ** get the rcb_lowtid and current_tid
		    */
		    if (flags & RAAT_INTERNAL_REPOS)
		    {
		       DB_TID *low, *cur;

		       rcb_lowtid = *((i4 *)inbuf);
		       inbuf += sizeof(i4);
		       current_tid = *((i4 *)inbuf);
		       inbuf += sizeof(i4);

		       low = (DB_TID *)&rcb_lowtid;
		       cur = (DB_TID *)&current_tid;

			/*
			** Internal reposition for btree needs lowkey
			*/
			if ( flags & RAAT_INTERNAL_BKEY)
			{
			    status = get_bkey(&inbuf, &dmr_cb, &err_code);
			    if (status != E_DB_OK)
				break;
			}
		    }

		    /* set access mode. TRUE if read mode */
		    if (dmr_char[0].char_value == DMT_X || 
			dmr_char[0].char_value == DMT_IX || 
			dmr_char[0].char_value == DMT_RIX)
			access_mode = FALSE;
		    else
			access_mode = TRUE;

		    /* if lowtid passed, this is a prefetch query which
		    ** means the previous lock was shared (or none).
		    ** we want to reposition the current record to
		    ** where the user thinks we are, not at the end of
		    ** a prefetch buffer.
		    ** Also, we want to plug the current_tid into the rcb,
		    ** in case the user requests a GET CURRENT next, since
		    ** GET CURRENT depends on the current_tid
                    */
		    if (rcb_lowtid)
		    {
		       dmr_char[1].char_id = DMR_SET_RCB_LOWTID;
                       MEcopy(&rcb_lowtid, sizeof(i4),
                  			      (char *)&dmr_char[1].char_value);
		       dmr_char[2].char_id = DMR_SET_CURRENTTID;
                       MEcopy(&current_tid, sizeof(i4),
                  			      (char *)&dmr_char[2].char_value);
		       dmr_cb.dmr_char_array.data_in_size +=
		                               	2 * sizeof(DMR_CHAR_ENTRY);

		       if ( flags & RAAT_INTERNAL_BKEY)
		       {
			   dmr_char[3].char_id = DMR_SET_LOWKEY;
			   dmr_char[3].char_value = DMR_C_ON;
			   dmr_cb.dmr_char_array.data_in_size +=
		                               	    sizeof(DMR_CHAR_ENTRY);
		       }

		    }

		    status = dmf_call(DMR_ALTER, &dmr_cb);

		    if (status)
		    {
			err_code = dmr_cb.error.err_code;
			break;
		    }

		    /*
		    ** Must return info to client.
		    */
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
		    message->scg_msize = sizeof(i4) + sizeof(i4);
		    message->scg_mask = SCG_NODEALLOC_MASK;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send error code back
		    */
		    *((i4 *)outbuf) = err_code;

		    /* send access mode flag back. TRUE if read mode */
		    outbuf += sizeof(i4);
		    *((i4 *)outbuf) = access_mode;
		    /*
		    ** Queue up message for SCF to return to FE.
		    */
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		case RAAT_TABLE_CLOSE:
		{
		    DMT_CB		dmt_cb;
		    /*
		    ** Build control block for DMT_CLOSE
		    */
		    dmt_cb.type			= DMT_TABLE_CB;
		    dmt_cb.length		= sizeof(DMT_CB);
		    dmt_cb.dmt_flags_mask	= 0;
		    inbuf += sizeof(i4);

		    dmt_cb.dmt_record_access_id	= *((char **)inbuf);

		    status = dmf_call(DMT_CLOSE, &dmt_cb);
		    if (status)
		    {
			err_code = dmt_cb.error.err_code;
			break;
		    }

		    /*
		    ** Must return info to client.
		    */
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
		    message->scg_msize = sizeof(i4);
		    message->scg_mask = SCG_NODEALLOC_MASK;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send only response, nothing additional.
		    */
		    *((i4 *)outbuf) = err_code;

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		case RAAT_RECORD_GET:
		{
		    DMR_CB		dmr_cb;
		    DMR_CB		bas_dmr_cb;
		    DMR_CHAR_ENTRY	dmr_char[4];
		    DMR_CHAR_ENTRY	bas_dmr_char;
		    i4             act_fetch_rows;
		    i4		fetch_rows;
		    i4		rcb_lowtid = 0;
		    i4             current_tid = 0;
		    i4		buf_size;
		    i4		outrec_width;
		    SCS_BIG_BUF		*big_mptr;

		    inbuf += sizeof(i4);

		    flag = *((i4 *)inbuf);
		    inbuf += sizeof(i4);

		    /*
		    ** Fill out dmr_cb
		    */
		    dmr_cb.type = DMR_RECORD_CB;
		    dmr_cb.length=sizeof(DMR_CB);
		    if (flag & RAAT_REC_CURR)
			dmr_cb.dmr_flags_mask = DMR_CURRENT_POS;
		    else if (flag & RAAT_REC_NEXT)
			dmr_cb.dmr_flags_mask = DMR_NEXT;
		    else if (flag & RAAT_REC_PREV)
			dmr_cb.dmr_flags_mask = DMR_PREV | DMR_RAAT;
		    else if (flag & RAAT_REC_BYNUM)
			dmr_cb.dmr_flags_mask = DMR_BY_TID;
		    else
		    {
			/* FIX_ME: report bad operation error */
			err_code = E_SC0376_INVALID_GET_FLAG;
			break;
		    }

		    /*
		    ** if RAAT_INTERNAL_REPOS flag set, doing 
		    ** a reposition due to going beyond the beginning
		    ** of a buffer.  get the lowtid
		    */
		    if (flag & RAAT_INTERNAL_REPOS)
		    {
			rcb_lowtid = *((i4 *)inbuf); 
			inbuf += sizeof(i4);

			/*
			** btree needs currenttid, lowkey
			*/
			if ( flag & RAAT_INTERNAL_BKEY)
			{
			    current_tid = *((i4 *)inbuf);
			    inbuf += sizeof(i4);

			    status = get_bkey(&inbuf, &dmr_cb, &err_code);
			    if (status != E_DB_OK)
				break;
			}
		    }

                    /* Get RCB */
#if defined(i64_win)
		    MEcopy(inbuf, sizeof(OFFSET_TYPE), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(OFFSET_TYPE);
#else
		    MEcopy(inbuf, sizeof(long), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(long);
#endif

		    /* now that we got the RCB, set the lowtid */
		    if (rcb_lowtid)
		    {
		       dmr_char[0].char_id = DMR_SET_RCB_LOWTID;
                       MEcopy(&rcb_lowtid, sizeof(i4),
                  			      (char *)&dmr_char[0].char_value);
		       dmr_cb.dmr_char_array.data_address = (char *)&dmr_char;
		       dmr_cb.dmr_char_array.data_in_size =
		                               	sizeof(DMR_CHAR_ENTRY);

		       if ( flag & RAAT_INTERNAL_BKEY)
		       {
			   dmr_char[1].char_id = DMR_SET_CURRENTTID;
			   MEcopy(&current_tid, sizeof(i4),
					      (char *)&dmr_char[1].char_value);
			   dmr_char[2].char_id = DMR_SET_LOWKEY;
			   dmr_char[2].char_value = DMR_C_ON;
			   dmr_cb.dmr_char_array.data_in_size +=
		                               	    2 * sizeof(DMR_CHAR_ENTRY);
		       }

		       status = dmf_call(DMR_ALTER, &dmr_cb);
                    }

		    if (status)
		    {
			err_code = dmr_cb.error.err_code;
			break;
		    }

		    /* Get row width */
		    MEcopy(inbuf, sizeof(i4), 
				&dmr_cb.dmr_data.data_in_size);
		    inbuf += sizeof(i4);

		    /* Get tid if needed */
		    if (flag & RAAT_REC_BYNUM) 
		    {
		       MEcopy(inbuf, sizeof(i4), &dmr_cb.dmr_tid);
		       inbuf += sizeof(i4);
                    }
		    else
		    { 
		       dmr_cb.dmr_tid = 0;
		    }

		    /* User requested # of rows to fetch */
		    if ((flag & RAAT_REC_NEXT) || (flag & RAAT_REC_PREV))
		    {
		      MEcopy(inbuf, sizeof(i4), &fetch_rows);
		      if (fetch_rows < 1)
			fetch_rows = 0;
		    }
		    else
		      fetch_rows = 1;
		    inbuf +=sizeof(i4);

		    /* 
		    ** If access to base table data of secondary index was
		    ** requested, init another dmr_cb and obtain base rcb ptr.
		    */
		    if (flag & RAAT_BAS_RECDATA)
		    {
			bas_dmr_cb.type = DMR_RECORD_CB;
			bas_dmr_cb.length = sizeof (bas_dmr_cb);
			bas_dmr_cb.dmr_flags_mask = DMR_BY_TID;
			MEcopy(inbuf, sizeof(PTR), &bas_dmr_cb.dmr_access_id);
			/* sharing same buffer space, can overlay seci tup */
			bas_dmr_cb.dmr_data.data_in_size = 
			    dmr_cb.dmr_data.data_in_size;
			/* set up in case we need to check if rdlock=nolock */
			bas_dmr_char.char_id = DMR_GET_DIRTYREAD;
			bas_dmr_cb.dmr_char_array.data_address = 
			    (char *)&bas_dmr_char;
			bas_dmr_cb.dmr_char_array.data_in_size =
		         	sizeof(DMR_CHAR_ENTRY);
		    }

		    /* 
		    ** Now modify it to make sure that we can send it in
		    ** one GCA_SEND
		    */
		    buf_size = max(scb->scb_cscb.cscb_comm_size, 4088);
		    status = scs_raat_bigbuffer(scb, buf_size);
		    if (status)
		    {
		       sc0e_0_put(status, 0);
		       err_code = E_SC0372_NO_MESSAGE_AREA;
		       break;
		    }
		    if (fetch_rows != 1) 
		    {
		      /*
			 the '- 2 * sizeof(i4)' is for the error code and
			 actual rows sent, which precedes the data.
			 the '+ 2 * sizeof(i4)' is for the rcb lowtid and
			 the tid, which accompanies each record.
                      */
/* FIX ME !!!!! what should be subtracted from comm_size that
		will equal 4040 ? */    
			j = (buf_size - 
			   2 * sizeof(i4)) / 
			   (dmr_cb.dmr_data.data_in_size +
			   2 * sizeof(i4));
		        /* 
			** if user specified more records than can fit
			** or we are to calculate # of rows
			** (fetch_rows = 0), then set fetch_rows to
			** calculated values.
			*/
			if (j <= fetch_rows || fetch_rows == 0)
			  fetch_rows = j;
		    }
		    /*
		    ** Must return info to client.
		    */
		    big_mptr = scb->scb_sscb.sscb_big_buffer;
		    message = (SCC_GCMSG *)&big_mptr->message;

	    	    outbuf = (char *)message->scg_marea;
		    /* save the address of output buffer for error code */
		    savebuf = outbuf;
		    /* Skip error code & actual no of rows sent */
		    outbuf += (2 * sizeof(i4));
		    /*
		    ** Now loop fetch_row times until error
		    */
		    dmr_char[0].char_id = DMR_GET_RCB_LOWTID;
		    dmr_cb.dmr_char_array.data_address = (char *)&dmr_char;
		    dmr_cb.dmr_char_array.data_in_size =
		         	sizeof(DMR_CHAR_ENTRY);
		    for (i = 0, act_fetch_rows = 0; i < fetch_rows; i++)
		    {
		      dmr_cb.dmr_data.data_address = outbuf + 2 * sizeof(i4);

		      /*
		      ** Now make the call
		      */
		      status = dmf_call(DMR_GET, &dmr_cb);

		      if (status)
		      {
			  err_code = dmr_cb.error.err_code;
			  break;
		      }
		      else
		      {
			 /* get the RCB lowtid for this record */
                         status = dmf_call(DMR_ALTER, &dmr_cb);
			 if (status)
			 {
			     err_code = dmr_cb.error.err_code;
			     break;
			 }
                         /* save rcb lowtid here  */
		         MEcopy((char *)&dmr_char[0].char_value,
				sizeof(i4), outbuf); 
		         outbuf += sizeof(i4);

			 if (flag & RAAT_BAS_RECDATA)
			 {
			     /* having read secondary, use tid to read base */
			     MEcopy(outbuf + dmr_cb.dmr_data.data_out_size,
				sizeof(i4), &bas_dmr_cb.dmr_tid);
			     bas_dmr_cb.dmr_data.data_address = 
				outbuf + sizeof(i4);
			     if (status = dmf_call(DMR_GET, &bas_dmr_cb))
			     {
				err_code = bas_dmr_cb.error.err_code;
				/* find out if dirty read - some errors OK */
				if (status = dmf_call(DMR_ALTER, &bas_dmr_cb))
				{
				    err_code = bas_dmr_cb.error.err_code;
				    break;   /* error finding out if error! */
				}
				if (bas_dmr_char.char_value == DMR_C_ON &&
				    (err_code == E_DM003C_BAD_TID ||
				    err_code == E_DM0047_CHANGED_TUPLE ||
				    err_code == E_DM0044_DELETED_TID))
				{
				    err_code = 0;  /* no error if dirty read */
				    outbuf -= sizeof(i4);   /* back up */
				    continue;  /* get next record */
				}
				else
				{
				    break;  /* err_code set from DMR_GET call */
				}
			     }
			     /* save tid of base table record */
			     MEcopy((char *)&bas_dmr_cb.dmr_tid, 
				sizeof(i4), outbuf);
			     outbuf += sizeof(i4);

			     /* pass over data record */
			     outbuf += bas_dmr_cb.dmr_data.data_out_size;
			 }
			 else
			 {
			     /* save tid here  */
			     MEcopy((char *)&dmr_cb.dmr_tid, sizeof(i4),
				outbuf);
			     outbuf += sizeof(i4);

			     /* save data here  */
			     outbuf += dmr_cb.dmr_data.data_out_size;
			 }
			 act_fetch_rows++;
                      }
		    } /* end for loop */

		    /* 
		    ** Calc msg size based on size and number of records. 
		    ** first i4 is for rcb lowtid (one per record). 
		    ** 2nd   i4 is for tid (one per record). 
		    ** 3rd i4 is for err_code, 4th is for act_fetch_rows 
		    */
		    outrec_width = (flag & RAAT_BAS_RECDATA) ?
			bas_dmr_cb.dmr_data.data_out_size :
			dmr_cb.dmr_data.data_out_size;
		    message->scg_msize = ((outrec_width + 
			sizeof(i4) + sizeof(i4)) * act_fetch_rows) + 
			sizeof(i4) + sizeof(i4);
		    message->scg_mask = SCG_NODEALLOC_MASK;
	    	    /*
	    	    ** Send response plus status and message (if any).
	    	    */
	    	    *((i4 *)savebuf) = err_code;
		    savebuf += sizeof(i4);
		    /* if we tried to fetch more than 1 row, set flag */
		    /* reuse RAAT_INTERNAL_REPOS flag for now */
		    /* this is only necessary if we actually got data */
		    if ((fetch_rows > 1) && act_fetch_rows) 
		       act_fetch_rows |= RAAT_INTERNAL_REPOS;

	    	    *((i4 *)savebuf) = act_fetch_rows;


	    	    /*
	    	    ** Queue up message for SCF to return to FE.
	    	    */
	    	    message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
	    	    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	    	    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
	    	    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		case RAAT_RECORD_POSITION:
		{
		    DMR_CB	dmr_cb;
		    i4	num_of_keys;
		    char	*value_ptr;
		    i4	key_size;
		    char	*attr_ptr;
		    i4		i;

		    dmr_cb.type = DMR_RECORD_CB;
		    dmr_cb.length = sizeof(DMR_CB);
		    dmr_cb.dmr_flags_mask = 0;
		    inbuf += sizeof(i4);
		    dmr_cb.dmr_access_id = *((char **)inbuf);
		    if (dmr_cb.dmr_access_id == 0)
		    {
			err_code = E_SC0377_NULL_RECORD_ID;
			break;
		    }
		    dmr_cb.dmr_q_fcn = 0;
		    dmr_cb.dmr_q_arg = 0;
#if defined(i64_win)
		    inbuf += sizeof(OFFSET_TYPE);
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx)
		    inbuf += sizeof(long);
#else
		    inbuf += sizeof(i4);
#endif
		    flag = *((i4 *)inbuf);
		    if (flag & RAAT_REC_BYNUM)
		    {
			dmr_cb.dmr_position_type = DMR_TID;
		        inbuf += sizeof(i4);
			dmr_cb.dmr_tid = *((i4 *)inbuf);
			dmr_cb.dmr_attr_desc.ptr_address = 0;
			dmr_cb.dmr_attr_desc.ptr_size = 0;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
			dmr_cb.dmr_attr_desc.ptr_size_filler = 0;
#endif
			dmr_cb.dmr_attr_desc.ptr_in_count = 0;
		    }
		    else if (flag & RAAT_REC_REPOS)
		    {
			dmr_cb.dmr_position_type = DMR_REPOSITION;
			dmr_cb.dmr_tid = 0;
			dmr_cb.dmr_attr_desc.ptr_address = 0;
			dmr_cb.dmr_attr_desc.ptr_size = 0;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
			dmr_cb.dmr_attr_desc.ptr_size_filler = 0;
#endif
			dmr_cb.dmr_attr_desc.ptr_in_count = 0;
		    }
		    else if (flag & RAAT_REC_FIRST)
		    {
			dmr_cb.dmr_position_type = DMR_ALL;
			dmr_cb.dmr_tid = 0;
			dmr_cb.dmr_attr_desc.ptr_address = 0;
			dmr_cb.dmr_attr_desc.ptr_size = 0;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
			dmr_cb.dmr_attr_desc.ptr_size_filler = 0;
#endif
			dmr_cb.dmr_attr_desc.ptr_in_count = 0;
		    }
		    else if (flag & RAAT_REC_LAST)
		    {
			dmr_cb.dmr_position_type = DMR_LAST;
                        dmr_cb.dmr_tid = 0;
                        dmr_cb.dmr_attr_desc.ptr_address = 0;
                        dmr_cb.dmr_attr_desc.ptr_size = 0;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
                        dmr_cb.dmr_attr_desc.ptr_size_filler = 0;
#endif
                        dmr_cb.dmr_attr_desc.ptr_in_count = 0;
		    }
		    else 
		    {
		        inbuf += sizeof(i4);
		    	num_of_keys = *((i4 *)inbuf);
			if (num_of_keys < 1)
			{
			    err_code = E_SC0378_NO_KEY_VALUES;
			    break;
			}
		    	dmr_cb.dmr_position_type = DMR_QUAL;
		    	dmr_cb.dmr_tid = 0;
		    	dmr_cb.dmr_attr_desc.ptr_address = MEreqmem(0, 
			    num_of_keys * sizeof(char *), TRUE, &status);
		   	dmr_cb.dmr_attr_desc.ptr_size = sizeof(DMR_ATTR_ENTRY);
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
			dmr_cb.dmr_attr_desc.ptr_size_filler = 0;
#endif
		    	dmr_cb.dmr_attr_desc.ptr_in_count = num_of_keys;
		        inbuf += sizeof(i4);
			attr_ptr = inbuf;
		        inbuf += num_of_keys * sizeof(DMR_ATTR_ENTRY);
		    	value_ptr = inbuf;
		    	for (i = 0; i < num_of_keys; i++)
		    	{
			    ((char **)dmr_cb.dmr_attr_desc.ptr_address)[i] 
				= attr_ptr;
			    MEcopy(value_ptr,sizeof(i4),(char *)&key_size);
			    if (key_size < 1)
			    {
				status = E_DB_ERROR;
				err_code = E_SC0379_INVALID_KEYSIZE;
				break;
			    }
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
			    value_ptr += sizeof(i4);
#else
			    value_ptr += 4;
#endif
			    attr_ptr += sizeof(DMR_ATTR_ENTRY);
			    (((DMR_ATTR_ENTRY **)dmr_cb.dmr_attr_desc.
			    	ptr_address)[i])->attr_value = value_ptr;
			    value_ptr += key_size;
		    	}
			if (status)
			   break;
		    }

		    status = dmf_call(DMR_POSITION, &dmr_cb);
		    if (dmr_cb.dmr_attr_desc.ptr_address)
			MEfree(dmr_cb.dmr_attr_desc.ptr_address);
		    if (status)
		    {
			err_code = dmr_cb.error.err_code;
			break;
		    }
		    /*
		    ** Must return info to client.
		    */
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
		    message->scg_msize = sizeof(i4);
		    message->scg_mask = SCG_NODEALLOC_MASK;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send only response, nothing additional.
		    */
	      	    *((i4 *)outbuf) = err_code;

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		case RAAT_RECORD_PUT:
		{
    		    DMR_CB	dmr_cb;
                    i4             has_extension;
                    i4             blobcnt = 0;
                    typedef struct
                    {
                        i4         blobtype;
                        i4         offset;
                    } BLOBOFF;
                    BLOBOFF             *bloboff;
                    char                *record;
                    char                *tmpbuf;

		    dmr_cb.type = DMR_RECORD_CB;
		    dmr_cb.length = sizeof(DMR_CB);
		    dmr_cb.dmr_flags_mask = DMR_RAAT;
		    inbuf += sizeof(i4);
#if defined(i64_win)
		    MEcopy(inbuf, sizeof(OFFSET_TYPE), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(OFFSET_TYPE);
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx)
		    MEcopy(inbuf, sizeof(long), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(long);
#else
		    dmr_cb.dmr_access_id = *((char **)inbuf);
		    inbuf += sizeof(i4);
#endif

		    dmr_cb.dmr_data.data_in_size = *((i4 *)inbuf);
		    inbuf += sizeof(i4);
                    /*
                    ** blob support -
                    ** if record contains blobs then parse for
                    ** blob count, and the array of blob datatype,offset.
                    */
		    bloboff = NULL;
		    tmpbuf  = NULL;
                    has_extension = *((i4 *)inbuf);
                    inbuf += sizeof(i4);
		    /* if this table has extensions, then find (and save)
		       temporary coupons.  After record is inserted, delete
		       them. */
                    if (has_extension)
                    {
                        /*
                        ** number of blobs in record
                        */
                        blobcnt = *((i4 *)inbuf);
                        inbuf += sizeof(i4);
 
                        /*
                        ** setup blob offset array
                        */
                        bloboff = (BLOBOFF *)MEreqmem(0,
                                     (blobcnt * 2 * sizeof(i4)),
                                     TRUE, &status);
                        for (i = 0; i < blobcnt; i++)
                        {
                            (bloboff + i)->blobtype = *((i4 *)inbuf);
                            inbuf += sizeof(i4);
                            (bloboff + i)->offset = *((i4 *)inbuf);
                            inbuf += sizeof(i4);
                        }
                    }
 
		    dmr_cb.dmr_data.data_address = (char *)inbuf;

                    if (has_extension)
                    {
                        /*
                        ** blob support - take copy of record containing
                        ** temporary coupons.
                        */
                        tmpbuf = MEreqmem(0, dmr_cb.dmr_data.data_in_size
				    + sizeof(ADP_PERIPHERAL) + 1,
                                    TRUE, &status);
			record = tmpbuf + sizeof(ADP_PERIPHERAL) + 1;
                        MEcopy(dmr_cb.dmr_data.data_address,
                            dmr_cb.dmr_data.data_in_size, record);
                    }

		    status = dmf_call(DMR_PUT, &dmr_cb);
		    if (status)
		    {
			err_code = dmr_cb.error.err_code;
			break;
		    }

		    dmr_cb.dmr_flags_mask = 0;
		    status = dmf_call(DMPE_QUERY_END, &dmr_cb);
		    if (status)
		    {
			err_code = dmr_cb.error.err_code;
			break;
		    }

                    /*
                    ** blob support - delete temporary tables
                    */
		    adu_free_objects(scb->scb_sscb.sscb_ics.ics_opendb_id, 0);
 
                    if (bloboff)
                        MEfree((PTR)bloboff);
 
                    if (tmpbuf)
                        MEfree((PTR)tmpbuf);

		    /*
		    ** Must return info to client.
		    */
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
		    message->scg_msize = 2 * sizeof(i4); 
		    message->scg_mask = SCG_NODEALLOC_MASK;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send only response, plus TID
		    */
	    	    *((i4 *)outbuf) = err_code;
    		    outbuf += sizeof(i4);
                    *((i4 *)outbuf) = dmr_cb.dmr_tid;

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		case RAAT_RECORD_DEL:
		{
    		    DMR_CB	dmr_cb;

		    inbuf += sizeof(i4);
		    dmr_cb.type = DMR_RECORD_CB;
		    dmr_cb.length = sizeof(DMR_CB);
#if defined(i64_win)
		    MEcopy(inbuf, sizeof(OFFSET_TYPE), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(OFFSET_TYPE);
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx)
		    MEcopy(inbuf, sizeof(long), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(long);
#else
		    dmr_cb.dmr_access_id = *((char **)inbuf);
		    inbuf += sizeof(i4);
#endif

		    flag = *((i4 *)inbuf);
		    if (flag & RAAT_REC_BYNUM)
		    {
		        inbuf += sizeof(i4);
			dmr_cb.dmr_tid = *((i4 *)inbuf);
			dmr_cb.dmr_flags_mask = DMR_BY_TID | DMR_RAAT;
		    }
		    else
		    {
		    	dmr_cb.dmr_tid = 0;
		    	dmr_cb.dmr_flags_mask = DMR_CURRENT_POS | DMR_RAAT;
		    }

		    status = dmf_call(DMR_DELETE, &dmr_cb);
		    if (status)
		    {
			err_code = dmr_cb.error.err_code;
			break;
		    }
		    /*
		    ** Must return info to client.
		    */
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
		    message->scg_msize = sizeof(i4);
		    message->scg_mask = SCG_NODEALLOC_MASK;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send only response, nothing additional.
		    */
	            *((i4 *)outbuf) = err_code;

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		case RAAT_RECORD_REPLACE:
		{
    		    DMR_CB	dmr_cb;

		    dmr_cb.type = DMR_RECORD_CB;
		    dmr_cb.length = sizeof(DMR_CB);
		    inbuf += sizeof(i4);
#if defined(i64_win)
		    MEcopy(inbuf, sizeof(OFFSET_TYPE), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(OFFSET_TYPE);
#elif defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
      defined(axp_lnx)
		    MEcopy(inbuf, sizeof(long), &dmr_cb.dmr_access_id);
		    inbuf += sizeof(long);
#else
		    dmr_cb.dmr_access_id = *((char **)inbuf);
		    inbuf += sizeof(i4);
#endif

		    flag = *((i4 *)inbuf);
		    if (flag & RAAT_REC_BYNUM)
		    {
		        inbuf += sizeof(i4);
			dmr_cb.dmr_tid = *((i4 *)inbuf);
			dmr_cb.dmr_flags_mask = DMR_BY_TID | DMR_RAAT;
		    }
		    else
		    {
		    	dmr_cb.dmr_tid = 0;
		    	dmr_cb.dmr_flags_mask = DMR_CURRENT_POS | DMR_RAAT;
		    }
		    inbuf += sizeof(i4);
		    dmr_cb.dmr_data.data_in_size = *((i4 *)inbuf);
		    inbuf += sizeof(i4);
		    dmr_cb.dmr_data.data_address = (char *)inbuf;
		    dmr_cb.dmr_attset = (char *) 0;

		    status = dmf_call(DMR_REPLACE, &dmr_cb);
		    if (status)
		    {
			err_code = dmr_cb.error.err_code;
			break;
		    }
		    /*
		    ** Must return info to client.
		    */
		    raat_mptr = scb->scb_sscb.sscb_raat_message;
		    message = (SCC_GCMSG *)&raat_mptr->message;
		    message->scg_msize = 2 * sizeof(i4);
		    message->scg_mask = SCG_NODEALLOC_MASK;

	    	    outbuf = (char *)message->scg_marea;

		    /*
		    ** Send only response, nothing additional.
		    */
	    	    *((i4 *)outbuf) = err_code;
		    outbuf += sizeof(i4);
		    *((i4 *)outbuf) = dmr_cb.dmr_tid;

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		case RAAT_BLOB_GET:
		{
		    ADF_CB	*adf_scb;
		    DB_DATA_VALUE res, coup, work;
		    char	*resbuf, *workptr, *cpnptr, *resptr;
		    int		reslen, cont, pad, header, datatype;
		    i4	space_needed;
		    short	actual;
		    SCS_BIG_BUF *bptr;

		    inbuf += sizeof(i4);

		    cont   = *((i4 *)inbuf);	/* continuation flag */
		    inbuf += sizeof(i4);
		    datatype  = *((i4 *)inbuf);	/* datatype */
		    inbuf += sizeof(i4);
		    cpnptr = (char *)inbuf;		/* pointer to coupon */
		    inbuf += sizeof(ADP_PERIPHERAL);
		    reslen = *((i4 *)inbuf);	/* amount to get */
		    inbuf += sizeof(i4);

		    /* 
		       if not allocated, allocate workarea.  This is passed
		       back and forth to/from the front end.  We only want
		       to allocate this once, since it will slow down things
		       to allocate it every time we come into BLOB_PUT or
		       BLOB_GET.  It will be freed when scs_terminate called
		    */
		    if (scb->scb_sscb.sscb_raat_workarea == NULL)
		    {
                       	status = sc0m_allocate(0, 
			    (i4)(sizeof(ADP_LO_WKSP) +
			    		DB_MAXTUP + sizeof(SCC_GCMSG)),
			    DB_SCF_ID, (PTR)SCS_MEM,
			    CV_C_CONST_MACRO('r','a','a','t'), 
			    (PTR *)&scb->scb_sscb.sscb_raat_workarea);

			if (status)
			{
			    sc0e_0_put(status, 0);
			    err_code = E_SC0372_NO_MESSAGE_AREA;
			    break;
			}
		    }
		    /* save off copy of workarea */
		    workptr =
		       ((char *)&scb->scb_sscb.sscb_raat_workarea->message + 
		       sizeof(SCC_GCMSG));
		    MEcopy((char *)inbuf, sizeof(ADP_LO_WKSP) + DB_MAXTUP,
	               workptr);

		    /* how much space is needed? */
		    space_needed = 3 * sizeof(i4) + DB_MAXTUP + 
			sizeof(ADP_LO_WKSP);
		    /* if buffer isn't big enough.  Will be recreated after */
		    status = scs_raat_bigbuffer(scb, reslen+space_needed);
		    if (status)
		    {
			sc0e_0_put(status, 0);
			err_code = E_SC0372_NO_MESSAGE_AREA;
			break;
	 	    }
		    bptr = scb->scb_sscb.sscb_big_buffer;
		    message = (SCC_GCMSG *)&bptr->message;

	    	    resbuf = (char *)message->scg_marea;

		    adf_scb = scb->scb_sscb.sscb_adscb;

		    /* set up for adu_redeem() call */
		    res.db_data = resbuf + space_needed;
		    coup.db_data = cpnptr;
		    work.db_data = workptr;
		    res.db_datatype = (DB_DT_ID)datatype;
		    coup.db_datatype = (DB_DT_ID)datatype;
		    work.db_datatype = 0;
		    res.db_length = reslen;
		    coup.db_length = sizeof(ADP_PERIPHERAL);
		    work.db_length = DB_MAXTUP + sizeof(ADP_LO_WKSP);

		    res.db_prec  = 0;
		    coup.db_prec  = 0;
		    work.db_prec  = 0;
		     
		    /* redeem the coupon for (part of) data */
		    status = adu_redeem(adf_scb, &res, &coup, &work, cont);
		    
		    if (status > E_DB_INFO)
		    {
			err_code = adf_scb->adf_errcb.ad_errcode;
			break;
		    }
		    /*
		    ** Must return info to client.
		    */
	    	    outbuf = (char *)message->scg_marea;

	    	    *((i4 *)outbuf) = err_code;
		    outbuf += sizeof(i4);

	    	    *((i4 *)outbuf) = status;
		    outbuf += sizeof(i4);

		    /* 
		       The pad is the header that precedes the data.
		       The first time through, it is 16.  All other
		       times, it is 4.
		    */
		    pad = (cont) ? sizeof(i4) : 4 * sizeof(i4);

		    /* check for special case of no data coming back */
#if defined(i64_win)
		    MEcopy(resbuf+space_needed+pad-sizeof(i4),
			   sizeof(OFFSET_TYPE), &header); 
#else
		    MEcopy(resbuf+space_needed+pad-sizeof(i4),
			   sizeof(long), &header); 
#endif
		    if (header == 0)
		    {
		       actual = 0;
		    }
		    else
		    {
		       resptr = resbuf + space_needed + pad; /* skip header */
		       /* actual #bytes fetched */
		       MEcopy(resptr, sizeof(short), &actual); 
		    }
	    	    *((i4 *)outbuf) = (i4)actual;
		    outbuf += sizeof(i4);

		    /* 
		       the workarea must be returned to the user, since it
		       gets updated everytime with current position (among
		       other info) which we will need next time through.
		    */
		    MEcopy((char *)workptr, DB_MAXTUP + sizeof(ADP_LO_WKSP),
			outbuf); /* work area */

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
		    bptr = scb->scb_sscb.sscb_big_buffer;
		    message = (SCC_GCMSG *)&bptr->message;

		    /* short is for length prefix */
		    message->scg_msize = actual + pad + sizeof(short) +
			space_needed;
		    message->scg_mask = SCG_NODEALLOC_MASK;
		    message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev           = message;

		    return (E_DB_OK);
		}

		case RAAT_BLOB_PUT:
		{
		    ADP_PERIPHERAL *ptrp, cpnbuf;
		    ADF_CB	*adf_scb;
		    DB_DATA_VALUE lobj, coup;
		    ADP_LO_WKSP *workptr;
		    char	*lobjbuf, *wptr;
		    int		datatype, reslen, cont, pad, eod;
		    SCS_BIG_BUF *bptr;

		    inbuf += sizeof(i4);

		    cont   = *((i4 *)inbuf);	/* continuation flag */
		    inbuf += sizeof(i4);
		    eod    = *((i4 *)inbuf);	/* end-of-data flag */
		    inbuf += sizeof(i4);
		    reslen = *((i4 *)inbuf);	/* how much data ins. */
		    inbuf += sizeof(i4);
		    datatype  = *((i4 *)inbuf);	/* datatype */
		    inbuf += sizeof(i4);
		    MEcopy(inbuf, sizeof(ADP_PERIPHERAL), (char *)&cpnbuf);
		    inbuf += sizeof(ADP_PERIPHERAL);

		    /* 
		       if not allocated, allocate workarea.  This is passed
		       back and forth to/from the front end.  We only want
		       to allocate this once, since it will slow down things
		       to allocate it every time we come into BLOB_PUT or
		       BLOB_GET.  It will be freed when scs_terminate called
		    */
		    if (scb->scb_sscb.sscb_raat_workarea == NULL)
		    {
                       	status = sc0m_allocate(0, 
			    (i4)(sizeof(ADP_LO_WKSP) +
					DB_MAXTUP + sizeof(SCC_GCMSG)),
			    DB_SCF_ID, (PTR)SCS_MEM,
			    CV_C_CONST_MACRO('r','a','a','t'), 
			    (PTR *)&scb->scb_sscb.sscb_raat_workarea);
			if (status)
			{
			    sc0e_0_put(status, 0);
			    err_code = E_SC0372_NO_MESSAGE_AREA;
			    break;
			}
		    }
		    /* save off copy of workarea */
		    wptr =
		       ((char *)&scb->scb_sscb.sscb_raat_workarea->message + 
		       sizeof(SCC_GCMSG));
		    MEcopy((char *)inbuf, sizeof(ADP_LO_WKSP) + DB_MAXTUP,
                       wptr);

		    inbuf += sizeof(ADP_LO_WKSP) + DB_MAXTUP;

		    pad = (cont) ? sizeof(i4) : 4 * sizeof(i4);

		    adf_scb = scb->scb_sscb.sscb_adscb;

		    /* set up work area */
		    workptr = (ADP_LO_WKSP *)wptr;
		    workptr->adw_fip.fip_work_dv.db_data = (PTR) (workptr + 1);
		    workptr->adw_fip.fip_work_dv.db_length = DB_MAXTUP;
		    workptr->adw_fip.fip_work_dv.db_prec = 0;
		    workptr->adw_fip.fip_pop_cb.pop_info = NULL;
		    workptr->adw_fip.fip_pop_cb.pop_temporary = ADP_POP_TEMPORARY;

		    /* 
		    ** we are going to be using the input buffer to send
		    ** the data to adu_couponify() (since we don't want to
		    ** allocate another big buffer).  Set lobjbuf to 
		    ** 4 or 16 bytes prior from the beginning of the len/data
		    ** buffer, for the ADP_PERIPHERAL header.
		    */
		    lobjbuf = inbuf - pad;
		    
		    if (cont)
		    {
		       *(i4 *)lobjbuf = 1;  /* segment indicator */
		    }
		    else
		    {
		       ptrp = (ADP_PERIPHERAL *)lobjbuf;   
		       ptrp->per_tag = ADP_P_GCA_L_UNK;
		       ptrp->per_length0 = 0;
		       ptrp->per_length1 = 0;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
		       {
			   int axp_one = 1;
			   I4ASSIGN_MACRO(axp_one,*((char *)ptrp + ADP_HDR_SIZE));
		       }
#else
		       ptrp->per_value.val_nat = 1;
#endif
		    }

		    /* set up structures for adu_couponify() */
		    lobj.db_data = lobjbuf;
		    coup.db_data = (char *)&cpnbuf;
		    lobj.db_datatype = (DB_DT_ID)datatype;
		    coup.db_datatype = (DB_DT_ID)datatype;
		    lobj.db_length = reslen + pad + sizeof(short);
		    coup.db_length = sizeof(ADP_PERIPHERAL);

		    lobj.db_prec  = 0;
		    coup.db_prec  = 0;
		    
		    /* if this is the last part of the insert */
		    if (eod)
		    {
		       lobj.db_length += sizeof(i4);
		       MEfill(sizeof(i4), '\0', inbuf + sizeof(short) +
		      	   reslen);
		    }
		    /*
		       build a coupon from the data being send in
		    */
		    status = adu_couponify(adf_scb, cont, &lobj, workptr, &coup);
		    /* status will either be E_DB_INFO (meaning that the data
		       is still incomplete) or E_DB_OK (done)
		    */
		    if (status > E_DB_INFO)
		    {
			err_code = adf_scb->adf_errcb.ad_errcode;
			break;
		    }
		    /* Should never happen. if incomplete code when end of data set */
		    if ((status == E_DB_INFO) && eod)
		    {
			err_code = 999;
			break;
		    }
		    /*
		    ** Must return info to client.
		    */
		    bptr = scb->scb_sscb.sscb_big_buffer;
		    message = (SCC_GCMSG *)&bptr->message;

	    	    outbuf = (char *)message->scg_marea;

	    	    *((i4 *)outbuf) = err_code;
		    outbuf += sizeof(i4);

	    	    *((i4 *)outbuf) = status;
		    outbuf += sizeof(i4);

		    /*
		       send copy of coupon to front end
		    */
		    MEcopy((char *)&cpnbuf, sizeof(ADP_PERIPHERAL), outbuf);
		    outbuf += sizeof(ADP_PERIPHERAL);

		    /* 
		       the workarea must be returned to the user, since it
		       gets updated everytime with current position (among
		       other info) which we will need next time through.
		    */
		    MEcopy((char *)workptr, DB_MAXTUP + sizeof(ADP_LO_WKSP),
			outbuf); /* work area */

		    /*
		    ** Queue up message for SCF to return to FE.
		    */
		    message->scg_msize = 2 * sizeof(i4) +
		       sizeof(ADP_PERIPHERAL) + DB_MAXTUP + sizeof(ADP_LO_WKSP);
		    message->scg_mask = SCG_NODEALLOC_MASK;
	            message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
		    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
		    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
		    scb->scb_cscb.cscb_mnext.scg_prev = message;

		    return (E_DB_OK);
		}

		default:
		{
		    TRdisplay("\nBad RAAT opcode!!");
		    /* FIX_ME: Write RAAT error to error log. */
		    err_code = E_SC037A_INVALID_OPCODE;
		    break;
		}
            }

	    /*
	    ** We come here only on error
	    */
	    if (scb->scb_sscb.sscb_raat_message == NULL)
	    {
		status = sc0m_allocate(0,
		    (i4)(sizeof(SCC_GCMSG) +
		    sizeof(i4)), DB_SCF_ID, (PTR)SCS_MEM,
		    CV_C_CONST_MACRO('n','d','m','f'), (PTR *)&message);
		if (status)
		{
		    sc0e_0_put(status, 0);
		    return (E_DB_ERROR);
		}

	        message->scg_mask = 0;
	        message->scg_mtype = GCA_RESPONSE;
	        message->scg_mdescriptor = NULL;
		message->scg_marea = outbuf = ((char *)message + sizeof(SCC_GCMSG)
				+sizeof(i4));
	    }
	    else
	    {
		raat_mptr = scb->scb_sscb.sscb_raat_message;
		message = (SCC_GCMSG *)&raat_mptr->message;
	        message->scg_mask = SCG_NODEALLOC_MASK;

	    	outbuf = (char *)message->scg_marea;

	    }

	    message->scg_msize = sizeof(i4);

	    /*
	    ** Send only response, nothing additional.
	    */
	    *((i4 *)outbuf) = err_code;

	    /*
	    ** Queue up message for SCF to return to FE.
	    */
	    message->scg_next = (SCC_GCMSG *)&scb->scb_cscb.cscb_mnext;
	    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
	    scb->scb_cscb.cscb_mnext.scg_prev = message;

	    break;

	case CS_OUTPUT:
	    /* sequencer will set this state on interrupt or force abort */
	    if (scb->scb_sscb.sscb_force_abort == SCS_FAPENDING)
	    {
		DMX_CB	dmx_cb;
		QEF_CB	*qef_cb = scb->scb_sscb.sscb_qescb;
		/*
		** FORCE ABORT condition has been raised for this session.
		** Emulate here what is done for SQL by the sequencer,
		** so that we have control over return code and session
		** status, else CS_setup will put us through CS_TERMINATE
		** which will also abort tran, but is not gracefull regarding
		** return code and session status (session should remain).
		*/ 
		scb->scb_sscb.sscb_force_abort = SCS_FORCE_ABORT;
		scc_dispose(scb);       /* Clear out any leftover messages */
		CScancelled(0);
		CSintr_ack();

		dmx_cb.type = DMX_TRANSACTION_CB;
		dmx_cb.length = sizeof(DMX_CB);
		dmx_cb.dmx_tran_id = qef_cb->qef_dmt_id;
		dmx_cb.dmx_option  = 0;     /* entire transaction */

		status = dmf_call(DMX_ABORT, &dmx_cb);
		if (status)
		{
		    /*
		    ** Couldn't abort, so leave scb's status as SCS_FAPENDING,
		    ** and treat as fatal error, allow session to terminate.
		    */
		    TRdisplay("SCS_RAAT: FABORT failure\n");
		    *next_op = CS_TERMINATE;
		    return E_DB_ERROR;
		}
		else
		{
		    /* Clean up potential SQL resources */
		    if (qef_cb)
		    {
			qef_cb->qef_open_count = 0;     /* no cursors opened */
			qef_cb->qef_defr_curs = FALSE;  /* no deferred cursor */
			qef_cb->qef_abort = FALSE;
			qef_cb->qef_stat = QEF_NOTRAN;
		    }

		    /* Ensure message block availability for scc_fa_notify() */
		    if (scb->scb_sscb.sscb_raat_message == NULL)
		    {
			status = allocate_message(scb,
			    &scb->scb_sscb.sscb_raat_message);
			if (status)
			{
			    *next_op = CS_TERMINATE;   /* absolute mayhem */
			    return E_DB_ERROR;
			}
		    }
		    /* mark as complete, sequencer will call scc_fa_notify */
		    scb->scb_sscb.sscb_force_abort = SCS_FACOMPLETE;
		    scb->scb_sscb.sscb_ics.ics_l_act1 = 0; /* clear activity */
		    scb->scb_cscb.cscb_gci.gca_status = E_GC0000_OK;           
		}

		*next_op = CS_INPUT;	/* everything back to normal */
		return (E_DB_OK);
	    }
	    else
	    {
		/*
		** Other situations for which sequencer will call us in
		** CS_OUTPUT mode could be handled here. For now, it does not
		** appear necessary to handle these other situations here since
		** 'interrupt' for RAAT is treated as session termination at 
		** a higher level, and deadlocks are also handled elsewhere.
		*/
		TRdisplay("SCS_RAAT: no handler for CS_OUTPUT %d %d\n",
		    scb->scb_sscb.sscb_force_abort,
		    scb->scb_sscb.sscb_interrupt);
		*next_op = CS_TERMINATE;
		return E_DB_ERROR;
	    }

	default:
	    TRdisplay("\nSCS_RAAT: Bad state %d\n", op_code);
	    return (E_DB_ERROR);
	    break;
    }
    return (E_DB_OK);
}


/*
** Name: allocate_message - local function to allocate RAAT message area.
**
** Description:
**
** Inputs:
**      scb		Session control block pointer
**	msg		The address of the address of the message area.
**
** Outputs:
**	msg		The address of the allocated message area.
**
** Returns:
**      DB_STATUS 
**
** History:
**	10-May-95 (lewda02/shust01)
**	    Created.
**      8-jun-1995 (lewda02/thaju02)
**          Added support for multiple communication buffers.
**          Optimized GCA buffer usage.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      19-aug-1996 (sweeney)
**          Allocate GC header too, else much badness ensues.
*/
DB_STATUS
allocate_message(
    SCD_SCB		*scb,
    SCS_RAAT_MSG	**raat_mptr)
{
    STATUS	gca_stat;
    DB_STATUS 	status;
    SCC_GCMSG	*msg;


#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
    status = sc0m_allocate(0, sizeof(PTR) + sizeof(SCC_GCMSG) +
#else
    status = sc0m_allocate(0, (i4)sizeof(i4) + sizeof(SCC_GCMSG) +
#endif
        scb->scb_cscb.cscb_comm_size + sizeof(DM_PTR), DB_SCF_ID,
	(PTR)SCS_MEM, CV_C_CONST_MACRO('r','a','a','t'), (PTR *)raat_mptr);

    if (status)
    {
	sc0e_0_put(status, 0);
	return (status);
    }

    msg = (SCC_GCMSG *)&(*raat_mptr)->message;
#if defined(axp_osf) || defined(ris_u64) || defined(LP64) || \
    defined(axp_lnx)
	msg->scg_marea = ((char *)msg + sizeof(SCC_GCMSG)+sizeof(PTR));
#else
	msg->scg_marea = ((char *)msg + sizeof(SCC_GCMSG)+sizeof(i4));
#endif
    (*raat_mptr)->next = NULL;

    msg->scg_mtype = GCA_RESPONSE;
    msg->scg_mdescriptor = NULL;

    return (E_DB_OK);
}


/*
** Name: scs_raat_load - take multiple GCA buffers and add to link list
**
** Description:
**
** Inputs:
**      scb		Session control block pointer
**
** Outputs:
**	none
**
** Returns:
**      DB_STATUS 
**
** History:
**	15-Sep-95 (thaju02/shust01)
**	    Created.
*/
DB_STATUS
scs_raat_load(SCD_SCB *scb)
{
    STATUS	status = OK;
    i4	err_code = 0;
    SCS_RAAT_MSG *raat_mptr;
    SCC_GCMSG	*message;
    i4	i;

     
    /* first time only */
    if (scb->scb_sscb.sscb_raat_buffer_used == 0)
    {
       if (scb->scb_sscb.sscb_raat_message == NULL) 
       {
	  status = allocate_message(scb, &scb->scb_sscb.sscb_raat_message);
	  if (status)
	  {
	     sc0e_0_put(status, 0);
	     return (status);
	  }
       } /* else, will reuse first buffer */
       raat_mptr = scb->scb_sscb.sscb_raat_message;
    }
    else
    {
    	raat_mptr = scb->scb_sscb.sscb_raat_message;
    	for (i = 1; i < scb->scb_sscb.sscb_raat_buffer_used; i++)
    	{
	   raat_mptr = raat_mptr->next;
    	}
	if (raat_mptr->next == NULL)
	{
	   status = allocate_message(scb, &raat_mptr->next);
	   if (status)
	   {
	       sc0e_0_put(status, 0);
	       return (status);
	   }
	}
	raat_mptr = raat_mptr->next;
    }
    message = (SCC_GCMSG *)&raat_mptr->message;
    message->scg_msize = scb->scb_cscb.cscb_gci.gca_d_length;
    MEcopy((char *)
	scb->scb_cscb.cscb_gci.gca_data_area,
	message->scg_msize, message->scg_marea);
    scb->scb_sscb.sscb_raat_buffer_used++;
    return (E_DB_OK);
}
/*
** Name: scs_raat_parse - take GCA buffer link list and concatenate them
**
** Description:
**
** Inputs:
**      scb		Session control block pointer
**
** Outputs:
**	none
**
** Returns:
**      DB_STATUS 
**
** History:
**	15-Sep-95 (thaju02/shust01)
**	    Created.
*/
DB_STATUS
scs_raat_parse(SCD_SCB *scb)
{
    STATUS	status = OK;
    i4	err_code = 0;
    i4	size = 0;
    SCS_RAAT_MSG *raat_mptr;
    SCS_BIG_BUF *blob_mptr;
    SCC_GCMSG	*message;
    char	*bptr;
    i4	i;

     
    raat_mptr = scb->scb_sscb.sscb_raat_message;

    /* add up to get total size */
    for (i = 0; i < scb->scb_sscb.sscb_raat_buffer_used; i++)
    {	
       message = (SCC_GCMSG *)&raat_mptr->message;
       size += message->scg_msize;
       raat_mptr = raat_mptr->next;
    }	

    /* do allocation */
    status = scs_raat_bigbuffer(scb, size);
    if (status)
    {
	sc0e_0_put(status, 0);
	return(status);;
    }
    blob_mptr = scb->scb_sscb.sscb_big_buffer;
    raat_mptr = scb->scb_sscb.sscb_raat_message;
    bptr = ((SCC_GCMSG *)&blob_mptr->message)->scg_marea;
    
    for (i = 0; i < scb->scb_sscb.sscb_raat_buffer_used; i++)
    {	
       message = (SCC_GCMSG *)&raat_mptr->message;
       MEcopy(message->scg_marea, message->scg_msize, bptr);
       bptr += message->scg_msize;
       raat_mptr = raat_mptr->next;
    }	
    scb->scb_sscb.sscb_raat_buffer_used = 0; /* reset buffer used */
    return (E_DB_OK);
}

/*
** Name: scs_raat_bigbuffer - allocate scb->scb_sscb.sscb_big_buffer 
**
** Description:
**
** Inputs:
**      scb		Session control block pointer
**      buf_size	size to be allocated
**
** Outputs:
**	none
**
** Returns:
**      STATUS 
**
** History:
**	15-Sep-95 (thaju02/shust01)
**	    Created.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	28-Aug-2009 (kschendel) b121804
**	    Rename to avoid conflict with fembed's routine.
*/
static STATUS
scs_raat_bigbuffer(SCD_SCB *scb, i4 buf_size)
{
    SCS_BIG_BUF *bptr;
    SCC_GCMSG	*msg;
    STATUS	status = OK;
    STATUS	gca_stat;

    /* if buffer isn't big enough.  Will be recreated after */
    if ( ((bptr = scb->scb_sscb.sscb_big_buffer) != NULL) &&
          (bptr->length < buf_size))
    {
	sc0m_deallocate(0, (PTR *)&bptr);
	bptr = scb->scb_sscb.sscb_big_buffer = NULL; /* so alloc will be done */

    }

    if (bptr == NULL)
    {
        status = sc0m_allocate(0, (i4)(
	    sizeof(i4) + sizeof(SCC_GCMSG) + buf_size),
	    DB_SCF_ID, (PTR)SCS_MEM,
	    CV_C_CONST_MACRO('r','a','a','t'), 
	    (PTR *)&scb->scb_sscb.sscb_big_buffer);
	if (status)
	{
	    return(status);
	}
        bptr = scb->scb_sscb.sscb_big_buffer;
	bptr->length = buf_size;
    	msg = (SCC_GCMSG *)&(bptr)->message;
	msg->scg_marea = ((char *)msg + sizeof(SCC_GCMSG)
		+sizeof(i4));
	msg->scg_mtype = GCA_RESPONSE;
	msg->scg_mdescriptor = NULL;

    }
    return(status);
}


/*
** Name: scs_raat_term_msg - send termination message back to RAAT prog
**
** Description:
**	This function is called to send back a RAAT formatted message
**	to a front end program in the event that the session is being
**	removed, eith thru IPM/IIMONITOR or due to the server being
**	stopped.
**	This should only be used where it is not the front end session
**	that has caused the termination. 
**
** Inputs:
**      scb		Session control block pointer
**	err_code	The desired error code to give back to RAAT prog
**
** Outputs:
**	none
**
** Returns:
**      DB_STATUS 
**
** History:
**	11-feb-97 (cohmi01)
**	    Created.
*/
DB_STATUS
scs_raat_term_msg(SCD_SCB *scb, i4 err_code)
{
    STATUS	status = OK;
    STATUS	gc_error;

    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_association_id =
			    scb->scb_cscb.cscb_assoc_id;
    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_buffer = (PTR)&err_code;
    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_message_type = GCA_RELEASE;
    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_msg_length = sizeof(err_code);
    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_end_of_data = TRUE;
    scb->scb_cscb.cscb_gcp.gca_sd_parms.gca_modifiers = 0;
    status = IIGCa_call(GCA_SEND, 
	(GCA_PARMLIST *)&scb->scb_cscb.cscb_gcp.gca_sd_parms,
	GCA_NO_ASYNC_EXIT, (PTR)0, -1, &gc_error);

    return(status);  /* will probably be ignored at this point */
}

DB_STATUS get_bkey(
char		**inbuf,
DMR_CB          *dmr_cb,
i4		*err_code)
{
    i4	        status = E_DB_OK;
    char                *input = *inbuf;
    i4             tot_key_size = 0;
    i4             num_of_keys;

    MEcopy(input, sizeof(i4), (char *)&num_of_keys);
    input += sizeof(i4);

    MEcopy(input, sizeof(i4), (char *)&tot_key_size);
    input += sizeof(i4);

    if (num_of_keys < 1)
    {
	*err_code = E_SC0378_NO_KEY_VALUES;
	return (E_DB_ERROR);
    }

    dmr_cb->dmr_data.data_address = input;
    dmr_cb->dmr_data.data_in_size = tot_key_size;
    input += tot_key_size;

    *inbuf = input;
    return (status);
}
