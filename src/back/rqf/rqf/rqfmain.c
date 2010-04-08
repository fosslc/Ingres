/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <lg.h>
#include    <ulm.h>
#include    <gca.h>
#include    <st.h>
#include    <cm.h>
#include    <cv.h>
#include    <er.h>
#include    <me.h>
#include    <nm.h>
#include    <pm.h>
#include    <tm.h>
#include    <tr.h>
#include    <pc.h>
#include    <lk.h>
#include    <cx.h>
#include    <scf.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <rqf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmccb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <rqfprvt.h>
#include    <generr.h>
#include    <sqlstate.h>

/**
**
**  Name: rqf.C - remote query facility primitives
**
**  Description:
**      This files contains RQF routines. The only entry point to all
**	these routines is rqf_call.
**
**          rqf_abort       - transaction abort
**          rqf_begin       - transaction begin
**          rqf_close       - cursor close
**	    rqf_cleanup	    - send interrupts to all assoc of a session
**          rqf_commit      - transaction commit
**	    rqf_query       - executing a query.
**          rqf_define      - repeat or cursor define
**          rqf_open_cursor - open cursor
**          rqf_delete      - cursor delete
**          rqf_end         - transaction end
**          rqf_fetch       - cursor fetch
**	    rqf_s_begin     - session intialization*
**	    rqf_s_end	    - session termination
**	    rqf_interrupt   - send interrupt to db server
**          rqf_invoke      - repeat query execution
**	    rqf_continue    - read or write to direct connection
**          rqf_secure      - transaction prepare commit
**	    rqf_shutdown    - facility shutdown
**	    rqf_startup	    - facility initialization
**	    rqf_t_fetch	    - STAR internal single tuple fetch
**	    rqf_nt_fetch    - fetch n tuples for client
**	    rqf_endret	    - retrieve loop interrupt
**	    rqf_transfer    - table transfer
**	    rqf_write	    - data write
**	    rqf_connect	    - make a direct connection
**	    rqf_disconnect  - direct disconnect
**	    rqf_check_interrupt - check for client interrupt
**	    rqf_set_func    - set options of SQL/QUEL
**	    rqf_modify	    - set +/-Y priv to the user CDB assoc.
**	    rqf_ldb_arch    - return architecture id.
**	    rqf_reassociate - reestablish a lost association.
**	    rqf_cluster_info- get the nodes in the cluster 
**	    rqf_invproc     - invoke a local database procedure
**	    rqf_call	    - entry point of RQF
**
**  History:
**      25-may-1988 (alexh)
**          created
**	30-jun-1988 (alexh)
**	    added rqf_t_fetch and rqf_t_flush
**	12-Oct-1988 (alexh)
**	    added rqf_query, rqf_endret, rqf_nt_fetch and
**	    deleted rqf_t_flush, rqf_normal
**	02-feb-1989 (georgeg)
**	    added rqf_set_option
**      14-may-1989 (carl)
**	    fixed rqf_invoke to return E_RQ0040_UNKNOWN_REPEAT_Q for 
**	    case of GCA_RESPONSE
**      02-apr-1990 (carl)
**	    changed rqf_transfer to call rqu_cp_get if BYTE_ALIGN is defined
**	    or rqf-cp_get otherwise
**	27-apr-1990 (georgeg)
**	    merged the 2pc and the titan63 code.
**	02-jun-1990 (georgeg)
**	    Added num_parms parameter to rqu_error
**	17-sep-1990 (fpang)
**	    Byte alignment fixes for 63 sun4 unix port.
**	26-sep-1990 (fpang)
**	    Added stuff left out from integration.
**	01-nov-1990 (fpang)
**	    Fixed byte-alignment in rqf_dump_td().
**	10-nov-1990 (georgeg)
**	    added rqf_cluster_info() to get the nodes in the cluster.
**	06-mar-1991 (fpang)
**	    1. Changed TRdisplays to RQTRACE. 
**	    2. Make sure association id is in all trace messages.
**	    3. Support levels of tracing.
**	18-apr-1991 (fpang)
**	    1. When declaring session memory from ulf, get a little
**	       for the ULF header.
**	    2. Partnername, and 'set queries' now use scf/rqf memory,
**	       so deallocate them accordingly at session shutdown.
**	    Fixes B36746.
**	01-may-1991 (fpang)
**	    In rqf_transfer(), td_data may not be aligned after coming
**	    back from rqu_cp_get(). Use MEcopy instead of assignments.
**	    Fixes B34428.
**	09-may-1991 (fpang)
**	    Fixed rqf_t_fetch() to handle cases where length of expected
**	    equals actual length including the null. Basically, always take 
**	    care of the null character if it is not expected.
**	    Fixes B37148.
**	30-may-1991 (fpang)
**	    RQTRACE(0) logs only errors. Reduces amount of fluff in
**	    log files. 
**	    Fixes B37195.
**	13-aug-1991 (fpang)
**	    1. RQTRACE modifications for rqf trace points.
**	    2. Added rqf_set_trace(), rqf_format() and rqf_trace() for
**	       rqf trace point processing.
**	17-sep-1991 (fpang)
**	    In rqf_query(), if rqu_get_reply() returns GCA_RELEASE
**	    ( happens when local is killed ), fail the query.
**	    Fixes B41063.
**	17-sep-1992 (fpang)
**	    In rqf_t_fetch(), call adf to convert float to int, and int to 
**	    float. Should eventually change entire function to call adf for 
**	    all conversions.
**	    Fixes B45116.
**	15-oct-1992 (fpang)
**	    Sybil Phase II
**	    1. Eliminated sc0m_rqf_allocate/deallocate() calls and replaced
**	       them with ULM calls.
**	    2. Removed RQTRACE of db_datavalues because rqu_putdbval() now
**	       traces db_datavalues.
**	    3. Moved rqf_dump_td() to rqu_dmp_tdesc() and changed all calls
**	       accordingly.
**	    4. rqf_continue() support SDC_CHK_INT mode to check for async
**	       interrupts during copy.
**	    5. rqf_invoke(), call rqu_putcid() to build GCA_ID.
**	    6. rqf_t_fetch(), fixed coersion to handle nullability better.
**	    7. rqf_call(), recognize DMC constants when setting timeout.
**	    8. rqf_cp_get(), handle new GCA1_C_FROM/INTO message types.
**	       Also, fixed to compute address of tuple descriptor in a portable
**	       fashion, which eliminates neccessity for rqu_cp_get().
**	19-oct-92 (teresa)
**	    Prototyped.
**	18-nov-1992 (fpang)
**	    In rqf_transfer(), changed copy table name to 'IISTAR'.
**	08-dec-1992 (fpang)
**	    New functions rqu_put_qtxt(), rqu_put_parms() and rqu_put_iparms()
**	    to modularize how qeuery text and parameters are processed.
**	    Also fixed minor bug in rqf_set_options() where it was getting
**	    the RQS_CB from the wrong place.
**	14-dec-92 (anitap)
**	    Added #include <psfparse.h> and <qsf.h> for CREATE SCHEMA project.
**	18-jan-1993 (fpang)
**	    Register database procedure changes :
**	    1. Rqf_invproc to execute a registered database procedure when
**	       rqf_call() opcode is RQR_INVPROC.
**	    2. Pass back LDB gca protocol in SCC_SDC_CB.sdc_peer_protocol for 
**	       direct connect and direct execute immediate.
**	    3. Encode sqlstates SS5000G_DBPROC_MESSAGE and 
**	       SS50009_ERROR_RAISED_IN_DBPROC to detect messages and raised
**	       errors when executing registered database procedures.
**	    4. Minor code cleanup. Shortened long lines, eliminated unused
**	       variables, etc.
**	05-feb-1993 (fpang)
**	    Added support for detecting commits/aborts when executing a 
**	    a procedure or in direct execute immediate.
**	12-apr-1993 (fpang)
**	    Added support for GCA1_DELETE and GCA1_INVPROC, new types that
**	    support owner.table and owner.procedure.
**	26-may-93 (vijay)
**	    Include st.h.
**	30-jun-93 (shailaja)
**	    Fixed prototype incompatibility warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	22-jul-1993 (fpang)
**	    1. Fixed various function prototype errors with proper coersion.
**	    2. Included missing CL/GL header files.
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**       6-Oct-1993 (fred)
**          Added byte string datatype support.
**	08-oct-1992 (fpang)
**	    Fixed B54385, (Create ..w/journaling fails). Recognize
**	    SS000000_SUCCESS as a successful status message.
**	18-oct-1993 (rog)
**	    In rqf_call(), we should not call rqf_check_interrupt() if a
**	    repeat query needs to be redefined (bug 54767).
**	04-Nov-1993 (fpang)
**	    In rqf_t_fetch(), don't set NULL indicator if at end-of-data, 
**	    because 'nullb' may be wrong.
**	    Fixes B56429, Select from aggregate where data is nullable may
**	    return inconsistant results.
**	19-nov-1993 (fpang)
**	    In rqf_call(), changed default timeout to 15 min.
**	    Fixes B57105, Star timesout in 4 hrs, not 15 minutes.
**	11-oct-93 (swm)
**	    Bug #56448
**	    The RQTRACE() macro has chnaged to make it portable. The change
**	    was necessary because rqf_trace() requires a variable number of,
**	    and variable sized arguments. It cannot become a varargs function
**	    as this practice is outlawed in main line code.
**	    Moved check for server tracing or session tracing to determine
**	    destination of output from rqf_trace() to rqf_format(), and
**	    deleted the rqf_trace() function, as RQTRACE() now calls TRformat
**	    directly.
**	    Added new rqf_neednl() function to determine whether a TRformat
**	    format string is terminated with a newline, as TRformat removes
**	    these. A pointer to the correct flag is returned by rqf_neednl().
**	    (The pointer gets passed to TRformat, which in turn passes it
**	    to rqf_format).
**	10-mar-94 (swm)
**	    Bug #59883
**	    Changed all RQTRACE calls to use newly-added rqs_trfmt format
**	    buffer and rqs_trsize in RQS_CB, and eliminated use automatically
**	    allocated buffers; this reduces the probability of stack overflow.
**	20-apr-94 (swm)
**	    Bug #62667
**	    Last change to this file inadvertently repeated a block of
**	    code which does a conditional return, twice. Removed offending
**	    code.
**	16-dec-96 (kch)
**	    In the function rqf_continue(), we now assign assoc->rql_timeout
**	    to rcb->rqr_timeout - rql_timeout was unassigned and was causing
**	    CSsuspend() to time out later on. This change fixes bug 79767.
**      06-mar-1996 (nanpr01)
**          Added pagesize parameter in the rqu_create_tmp call. 
**	23-sep-1996 (canor01)
**	    Move global data definitions to rqfdata.c.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE: use DB_COPY macro.
**      05-Dec-2001 (hanal04) Bug 105723, INGSTR 42.
**          Added support for DB_DEC_TYPE to DB_DEC_TYPE conversion
**          in rqf_t_fetch().
**	12-apr-2002 (devjo01)
**	    Add headers needed to get CX_MAX_NODE_NAME_LEN for 'rqfprvt.h'.
**	    Correct questionable calculation of 'rqf_len_vnode'.
**	27-May-2002 (hanje04)
**	    Replace nats brought in  by X-Integ with i4's.
**	18-sep-2003 (abbjo03)
**	    Add include of pc.h (required by lg.h).
**	06-Nov-2006 (kiria01) b117042
**	    Conform CMdigit macro calls to relaxed bool form
**      09-jan-2009 (stial01)
**          Fix buffers that are dependent on DB_MAXNAME
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**/

GLOBALREF	RQF_CB	*Rqf_facility;
GLOBALREF       i4  ss5000g_dbproc_message;
GLOBALREF	i4  ss50009_error_raised_in_dbproc;
GLOBALREF	i4  ss00000_success;

FUNC_EXTERN VOID rqf_trace();
/*
**  Number of associations per seeeion before RQF runs out of memory, 
**  this needs to be changed in the future and receive the info as a
**  server startup parameter.
*/
#define	RQF_MAX_ASSOC	    32


/*{
** Name: rqf_abort            - transaction abort (RQR_ABORT)
**
** Description:
**         abort a LDB SQL or QUEL transaction. If the site is not
**	active, nothing is done and normal status is returned.
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_1_site           database server id
**
** Outputs:
**      rqr_cb->
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_RQ0020_COMMIT_FAILED
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-may-1988 (alexh)
**          created
**	28-dec-1988 (alexh)
**	    inherit session language type
*/
DB_ERRTYPE
rqf_abort(
	RQR_CB	    *rqr_cb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 0);
  i4		abort_msg;

  if (assoc == NULL)
    /* no op */
    return(E_RQ0000_OK);

  /* determine the transaction type */
  if ( ((RQS_CB *)rqr_cb->rqr_session)->rqs_lang == DB_QUEL)
    abort_msg = GCA_Q_BTRAN;
  else
    abort_msg = GCA_ROLLBACK;

    if (assoc->rql_turned_around == FALSE)
    {
	/*
	** QEF is confused, the protocol has not turned around yet, we must clean
	** the association first and put the remote partner in a read state.
	*/
	(VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
    }
  /* send the request */
  rqr_cb->rqr_error.err_code = rqu_send(assoc, (i4)0, TRUE, abort_msg);

  if (rqr_cb->rqr_error.err_code == E_RQ0000_OK &&
      rqu_get_reply(assoc, -1) == GCA_DONE)
    return(E_RQ0000_OK);

  return(E_RQ0020_ABORT_FAILED);
}

/*{
** Name: rqf_begin            - transaction begin (RQR_BEGIN)
**
** Description:
**         Request LDBMS QUEL transaction begin.
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_1_site           database server id
**
** Outputs:
**          rqr_txn_id         STAR transaction id
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_RQ0021_BEGIN_FAILED
**	    E_RQ0006_CANNOT_GET_ASSOCIATION
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-may-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqf_begin(
	RQR_CB	    *rqr_cb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 1);

  if (assoc == NULL)
    /* no op */
    return(E_RQ0006_CANNOT_GET_ASSOCIATION);

  /* send the request */
  rqr_cb->rqr_error.err_code = rqu_send(assoc, (i4)0, TRUE, GCA_Q_BTRAN);
  if (rqr_cb->rqr_error.err_code == E_RQ0000_OK &&
      rqu_get_reply( assoc, -1 ) == GCA_S_BTRAN)
    {
      /* return the transaction id */
      MEcopy((PTR)assoc->rql_rv.gca_data_area, sizeof(GCA_TX_DATA),
	     (PTR)&assoc->rql_txn_id);
      return(E_RQ0000_OK);
    }

  return(E_RQ0021_BEGIN_FAILED);
}

/*{
** Name: rqf_close            - cursor close (RQR_CLOSE)
**
** Description:
**         close LDB cursor. Note that GCA_ID is not an atomic data.
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_1_site         database server id
**         rqr_qid            this required for query type RQT_EXEC 
**			      RQT_FETCH RQT_DELETE RQT_CLOSE
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-may-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqf_close(
	RQR_CB	    *rqr_cb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 0);

  if (assoc == NULL)
    /* no op */
    return(E_RQ0006_CANNOT_GET_ASSOCIATION);

  rqu_putinit(assoc, GCA_CLOSE);
  rqu_putcid(assoc, &rqr_cb->rqr_qid);
  if ( rqu_putflush(assoc, TRUE) == E_RQ0000_OK)
  {
    if (rqu_get_reply(assoc, -1) == GCA_RESPONSE)
          return(E_RQ0000_OK);
  }
  /*
  ** if we get here something is wrong clean up the the association
  */
  (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
  return(E_RQ0027_CURSOR_CLOSE_FAILED);
}

/*{
** Name: rqf_commit           - transaction commit (RQR_COMMIT)
**
** Description:
**         generic (QUEL or SQL) transaction commit
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_1_site           database server id
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_RQ0019_COMMIT_FAILED
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-may-1988 (alexh)
**          created
**	28-dec-1988 (alexh)
**	    inherit session language type
*/
DB_ERRTYPE
rqf_commit(
	RQR_CB	    *rqr_cb)
{
    RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 0);
    i4		commit_msg;

    if (assoc == NULL)
    {
	return(E_RQ0000_OK);
    }

    /*
    ** messages for committing transactions are different, depending on lang
    */
    if ( ((RQS_CB *)rqr_cb->rqr_session)->rqs_lang == DB_QUEL)
    {
	commit_msg = GCA_Q_ETRAN;
    }
    else
    {
	commit_msg = GCA_COMMIT;
    }

    if (assoc->rql_turned_around == FALSE)
    {
	/*
	** QEF or TPF is confused, the protocol has not turned around yet, we must clean
	** the association first and put the remote partner in a read state.
	*/
	(VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
	return(E_RQ0019_COMMIT_FAILED);
    }
  
    /*
    ** execute the commit request 
    */
    rqr_cb->rqr_error.err_code = rqu_send(assoc, (i4)0, TRUE, commit_msg);

    /*
    ** note that the reply message for different types of commit are different 
    */
    if (rqr_cb->rqr_error.err_code == E_RQ0000_OK)
    {
	if (commit_msg == GCA_Q_ETRAN)
	{
	    if (rqu_get_reply(assoc, -1) == GCA_S_ETRAN)
	    /*
	    ** positive QUEL end transaction reply 
	    */
	    return(E_RQ0000_OK);
	}
	else if (commit_msg == GCA_COMMIT)
	{
	    if (rqu_get_reply(assoc, -1) == GCA_DONE)
	    {
		/*
		** positive SQL commit reply 
		*/
		return(E_RQ0000_OK);
	    }
	}
    }
    /*
    ** did not get postively reply from commit 
    */
    (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
    return(E_RQ0019_COMMIT_FAILED);
}

/*{
** Name: rqf_query	- execute a query (RQR_QUERY)
**
** Description:
** 	The execution of a query is request at specific database
**	server. The caller is responsible for subsequent data
**	exchanges. If no association exists for the request site, a
**	new associate will be requested. Error and trace data will be
**	send to client. This routine should not be called by non-RQF
**	routines to execute cursor or copy commands. The caller is
**	also responsible for flushing the return when it is desirable
**	to end the data exchange prematurely.
**
**	This routine basically constructs GCA_Q_DATA structure.
**
**	This routine does handle DD_PACKET_LIST and binary data array.
**	The caller is required to set the count of binary data.
**
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_msg            query text
**         rqr_1_site         database server id
**	   rqr_dv_cnt	      binary data value count. 0 if not used.
**	   rqr_dv_p	      pointer to DB_DATA_VALUE if rqr_dv_cnt != 0
**
** Outputs:
**	  rqr_error		error reported by RQF and GCA
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_RQ0006_CANNOT_GET_ASSOCIATION
**	    E_RQ0014_QUERY_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-Oct-1988 (alexh)
**          created
**      01-aug-1989 (georgeg)
**          added het net support.
**	17-sep-1991 (fpang)
**	    If rqu_get_reply() returns GCA_RELEASE (if local is killed)
**	    fail the query. 
**	15-oct-1992 (fpang)
**	    Removed RQTRACE of db datatypes because rqu_put_dbval()
**	    does it now.
**	08-dec-1992 (fpang)
**	    Call rqu_put_qtxt(), rqu_put_parms() and rqu_put_iparms()
**	    to output query text and parameters.
*/
DB_ERRTYPE
rqf_query(
	RQR_CB	    *rqr_cb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 1);
  DB_ERRTYPE	status;
  RQS_CB	*rqs = (RQS_CB *)rqr_cb->rqr_session;

  if (assoc == NULL)
  {
    /* required association cannot not be gotten */
    return(E_RQ0006_CANNOT_GET_ASSOCIATION);
  }

  assoc->rql_described = FALSE;	/* need column description */
  ++assoc->rql_nostatments;

  /* put the query text out to LDB */
  status = rqu_put_qtxt(assoc, GCA_QUERY, rqr_cb->rqr_q_language, 
		(rqr_cb->rqr_no_attr_names == FALSE) ? GCA_NAMES_MASK : 0,
		&rqr_cb->rqr_msg, rqr_cb->rqr_tmp);
  /* put any binary data of the query to LDB */
  status = rqu_put_parms(assoc, rqr_cb->rqr_dv_cnt, rqr_cb->rqr_dv_p);
  /* flush the put stream if previous puts are OK */
  if (status == E_RQ0000_OK)
      status = rqu_putflush(assoc, TRUE);

  /* get the reply */
  if (status == E_RQ0000_OK)
  {
      switch( rqu_get_reply(assoc, -1))
      {
	case GCA_ERROR:
	case GCA_RELEASE:
	  status = E_RQ0014_QUERY_ERROR;
	  break;
	case GCA_RESPONSE:
	{
	    GCA_RE_DATA	*response = (GCA_RE_DATA *)assoc->rql_rv.gca_data_area;
	    rqr_cb->rqr_tupcount = response->gca_rowcount;
	    /* this gets around QEF's bug on multi-site SELECT */
	    if (rqr_cb->rqr_tupcount == -1)
	      rqr_cb->rqr_tupcount = 0;
	    status = E_RQ0000_OK;
	}
	   break;
	default:
	  status = E_RQ0000_OK;
	  break;
      }
  }  

  return(status);
}

/*{
** Name: rqf_define           - repeat or cursor query definition
**				(RQR_DEFINE)
** Description:
**        Repeat or cursor query definition.
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_msg            query text msg
**         rqr_1_site           database server id
**
** Outputs:
**      rqr_cb->
**         rqr_qid      	query id for query
**	   rqr_error		error reported by RQF
**        
**	Returns:
**	    E_DB_OK		request completed successfully
**	    E_DB_ERROR		request failed
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-May-1988 (alexh)
**          created
**	25-Jan-1989 (alexh)
**	    adjusted the interpretation of parameters.
**	15-oct-1992 (fpang)
**	    Removed RQTRACE of db datatypes because rqu_put_dbval()
**	    does it now.
**	08-dec-1992 (fpang)
**	    Call rqu_put_qtxt(), rqu_put_parms() and rqu_put_iparms()
**	    to output query text and parameters.
*/
DB_ERRTYPE
rqf_define(
	RQR_CB	    *rqr_cb,
	bool	    gca_qry)
{
  RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 1);
  DB_ERRTYPE	status;
  QEF_PARAM	*inv_parms = (QEF_PARAM *)rqr_cb->rqr_inv_parms;
  RQS_CB	*rqs = (RQS_CB *)rqr_cb->rqr_session;

  if (assoc == NULL) 
    /* required association cannot not be gotten */
    return(E_RQ0006_CANNOT_GET_ASSOCIATION);

  /* execute the query definition */
  rqr_cb->rqr_dv_cnt = 0;
  assoc->rql_described = FALSE;	/* need column description */

  /* put the query text out to LDB */
  status = rqu_put_qtxt(assoc, (gca_qry == TRUE) ? GCA_QUERY : GCA_DEFINE, 
	        rqr_cb->rqr_q_language, 0, &rqr_cb->rqr_msg, rqr_cb->rqr_tmp );
  /* insert query id here */
  if (rqr_cb->rqr_qid_first)
  {
    status = rqu_putqid(assoc, &rqr_cb->rqr_qid);
  }
  if ( (DB_DATA_VALUE **)inv_parms != NULL )
  {
    /* put all the parameter data values. Note db value array starts at 1 */
    status = rqu_put_iparms(assoc, inv_parms->dt_size,
			    ((DB_DATA_VALUE **)inv_parms->dt_data)+1);
  }


  if ( (!rqr_cb->rqr_qid_first) && (status == E_RQ0000_OK) )
  {
    status = rqu_putqid(assoc, &rqr_cb->rqr_qid);
  }
  /* flush the put stream if previous puts are OK */
  if (status == E_RQ0000_OK)
  {
    status = rqu_putflush(assoc, TRUE);
  }

  /* get the reply and it should GCA_QC_ID */
  if (status == E_RQ0000_OK)
  {
    if (rqu_get_reply(assoc, -1) != GCA_QC_ID)
    {
      return(E_RQ0026_QID_EXPECTED);
    }
    else
    {
      rqu_getqid(assoc, &rqr_cb->rqr_qid);
      (VOID)rqu_get_reply(assoc, -1);	/* get the GCA_RESPONSE */
      return(E_RQ0000_OK);
    }
  }
  else
    return(status);
}

/*{
** Name: rqf_udate           - cursor update
**				(RQR_UPDATE)
** Description:
**        cursor update.
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**
** Outputs:
**      rqr_cb->
**         rqr_qid      	query id for query
**	   rqr_error		error reported by RQF
**        
**	Returns:
**	    E_DB_OK		request completed successfully
**	    E_DB_ERROR		request failed
**	    E_RQ0043_CURSOR_UPDATE_FAILED
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-feb-1989 (georgeg)
**          created from rqf_define
**	15-oct-1992 (fpang)
**	    Removed RQTRACE of db datatypes because rqu_put_dbval()
**	    does it now.
**	08-dec-1992 (fpang)
**	    Call rqu_put_qtxt(), rqu_put_parms() and rqu_put_iparms()
**	    to output query text and parameters.
*/
DB_ERRTYPE
rqf_update(
	RQR_CB	    *rqr_cb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 1);
  DB_ERRTYPE	status;
  RQS_CB	*rqs = (RQS_CB *)rqr_cb->rqr_session;

  if (assoc == NULL) 
    /* required association cannot not be gotten */
    return(E_RQ0006_CANNOT_GET_ASSOCIATION);

  /* put the query text out to LDB */
  status = rqu_put_qtxt(assoc, GCA_QUERY, rqr_cb->rqr_q_language, 0,
			&rqr_cb->rqr_msg, rqr_cb->rqr_tmp);

  if ( rqr_cb->rqr_qid_first == TRUE )
  {
    status = rqu_putqid(assoc, &rqr_cb->rqr_qid);
    if ( status != E_RQ0000_OK )
      return(status);
  }

    /* put all the parameter data values. Note db value array starts at 0 */

  status = rqu_put_parms(assoc, rqr_cb->rqr_dv_cnt, rqr_cb->rqr_dv_p);
  if ( status != E_RQ0000_OK )
      return( E_RQ0043_CURSOR_UPDATE_FAILED );

  /*
  ** put in the QID
  */  
  if (rqr_cb->rqr_qid_first == FALSE )
  {
    status = rqu_putqid(assoc, &rqr_cb->rqr_qid);
    if ( status != E_RQ0000_OK )
      return(E_RQ0043_CURSOR_UPDATE_FAILED);
  }

  /* 
  ** flush the put stream if previous puts are OK 
  */
  if (status == E_RQ0000_OK)
  {
    status = rqu_putflush(assoc, TRUE);
  }

  if (status != E_RQ0000_OK) 
  {
    (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
    return(E_RQ0043_CURSOR_UPDATE_FAILED);
  }

  /*
  ** get the reply and it should a GCA_RESPONCE 
  */
  if (rqu_get_reply(assoc, -1) == GCA_RESPONSE)
  {
    GCA_RE_DATA	*response = (GCA_RE_DATA *)assoc->rql_rv.gca_data_area;
    rqr_cb->rqr_tupcount = response->gca_rowcount;
  }
  else
  {
    (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
    return(E_RQ0043_CURSOR_UPDATE_FAILED);
  }

  return( E_DB_OK );

}

/*{
** Name: rqf_open_cursor	- open cursor query definition
**				(RQR_OPEN)
** Description:
**        open cursor query definition.
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**
** Outputs:
**      rqr_cb->
**         rqr_qid      	query id for query
**	   rqr_error		error reported by RQF
**        
**	Returns:
**	    E_DB_OK		request completed successfully
**	    E_DB_ERROR		request failed
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-feb-1989 (alexh)
**          created
**      01-aug-1989 (georgeg)
**          added hetnet support, save the tuple descriptor.
**	15-oct-1992 (fpang)
**	    Removed RQTRACE of db datatypes because rqu_put_dbval()
**	    does it now.
**	08-dec-1992 (fpang)
**	    Call rqu_put_qtxt(), rqu_put_parms() and rqu_put_iparms()
**	    to output query text and parameters.
*/
DB_ERRTYPE
rqf_open_cursor(
	RQR_CB	    *rqr_cb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 1);
  DB_ERRTYPE	status;
  RQS_CB	*rqs = (RQS_CB *)rqr_cb->rqr_session;

  if (assoc == NULL) 
    /* required association cannot not be gotten */
    return(E_RQ0006_CANNOT_GET_ASSOCIATION);

  /* execute the query definition */
  assoc->rql_described = FALSE;	/* need column description */

  /* put the query text out to LDB */
  status = rqu_put_qtxt(assoc, GCA_QUERY, rqr_cb->rqr_q_language, 0,
			&rqr_cb->rqr_msg, rqr_cb->rqr_tmp);

  /* 
  ** insert query id here if it wants to go before the parameters
  */
  if ( rqr_cb->rqr_qid_first )
  {
    status = rqu_putqid(assoc, &rqr_cb->rqr_qid);
    if ( status != E_RQ0000_OK )
    {
      (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
      return( E_RQ0044_CURSOR_OPEN_FAILED );
    } 
  }

  /* put any binary data of the query to LDB */
  status = rqu_put_parms(assoc, rqr_cb->rqr_dv_cnt, rqr_cb->rqr_dv_p);

  if ( !rqr_cb->rqr_qid_first )
  {
    status = rqu_putqid(assoc, &rqr_cb->rqr_qid);
  }


  /*
  ** flush the put stream if previous puts are OK 
  */
  if (status == E_RQ0000_OK)
  {
    status = rqu_putflush(assoc, TRUE);
  }

  /* get the reply and it should GCA_QC_ID */
  if (status == E_RQ0000_OK)
  {
    if (rqu_get_reply(assoc, -1) != GCA_QC_ID)
    {
      return(E_RQ0026_QID_EXPECTED);
    }
    else
    {
      rqu_getqid(assoc, &rqr_cb->rqr_qid);
      
      /*
      **    get the TDDESC
      */
      if (rqu_get_reply(assoc, -1) != GCA_TDESCR)
      {
	(VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
	return( E_RQ0002_NO_TUPLE_DESCRIPTION );
      }
      /* 
      ** Get and save the tuple descriptor.
      */
      if (rqf_td_get( rqr_cb, assoc ) != E_DB_OK )
      {
            return( E_RQ0044_CURSOR_OPEN_FAILED);
      }
      assoc->rql_described = TRUE;	/* got column description */
      (VOID)rqu_get_reply(assoc, -1);	/* get the GCA_RESPONSE */
      return(E_RQ0000_OK);
    }
  }
  else
  {
    (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
    return( E_RQ0044_CURSOR_OPEN_FAILED);
  }
}

/*
** {
** Name: rqf_td_get             - Get the typle description. 
**
** Description:
**       This routine will read the tuple description and copiy it to the
**       space provided by QEF.
** Inputs:
**      rqr_cb->              pointer to RQF request control block
**      assoc                 the LDB association.
**
** Outputs:
**          rqr_error           error reported by RQF
**          
**        
**      Returns:
**          E_DB_OK             request completed successfully
**          E_DB_ERROR          request failed
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      12-may-1989 (georgeg)
**          created
**      25-may-1990 (georgeg)
**	    from the tuple descriptor supplied by the local extruct the
**	    DB_VALUES and copy them in the descriptor prepared by STAR,
**	    leave the attribute names alone as they could be different
**	    in the LDB and in STAR.
**	15-oct-1992 (fpang)
**	    Call rqu_dmp_tdesc() instead of rqf_dump_td(), the latter has
**	    moved to rqutil.
**	22-jul-1993 (fpang)
**	    Explicitly set gca_l_attnam to 0 because rqu_col_desc_fetch()
**	    always snarfs it.
**      14-Oct-1993 (fred)
**          Add (unused) parameters to rqu_data_fetch().
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE: use DB_COPY macro.
*/
DB_ERRTYPE
rqf_td_get(
	RQR_CB      *rqr_cb,
	RQL_ASSOC   *assoc)
{
    GCA_TD_DATA	    *td_data = (GCA_TD_DATA *)assoc->rql_rv.gca_data_area,
		    *ldb_td_desc, *star_td_desc;
    VOID            rqu_dmp_tdesc();
    SCF_TD_CB	    *scf_td_info;
    bool	    eod, dump_td = FALSE;
    i4		    i;
    GCA_COL_ATT	    *star_col_att_p, *ldb_col_att_p;
    DB_ERRTYPE	    status;
    i4		    att_value;
    RQS_CB	    *rqs = (RQS_CB *)rqr_cb->rqr_session;

    /*
    ** tuple description, if any, should have been fetched during
    ** query execution.
    */
    if (assoc->rql_rv.gca_message_type != GCA_TDESCR)
    {
	/*
	** has to be a GCA_TDESCR message 
	*/
	(VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
	return(E_RQ0002_NO_TUPLE_DESCRIPTION);
    }

    /*
    ** register the fact that is has been described 
    */
    assoc->rql_described = TRUE;
    assoc->rql_b_ptr = (char *)assoc->rql_rv.gca_data_area;
    assoc->rql_b_remained = (u_i2)assoc->rql_rv.gca_d_length;

    /*
    ** flush the remainging tuple description 
    */
    scf_td_info = (SCF_TD_CB *)rqr_cb->rqr_tupdesc_p;
    /*
    ** this is the descriptor constucted from STAR's parser.
    ** the number of collumns of the STAR tuple descriptor
    ** and that of the local need to be the same. 
    */
    star_td_desc = (GCA_TD_DATA *)scf_td_info->scf_star_td_p;
    ldb_td_desc = (GCA_TD_DATA *)scf_td_info->scf_ldb_td_p;
    if (td_data->gca_l_col_desc != star_td_desc->gca_l_col_desc)
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:Assn %d. Tuple size mismatch. Ldb = %d, Star = %d\n"),
		    assoc->rql_assoc_id,
		    td_data->gca_l_col_desc, star_td_desc->gca_l_col_desc);
	return(E_RQ0032_DIFFERENT_TUPLE_SIZE);

    }

    /* 
    ** look in the tuple descriptor of the local extract all the
    ** GCA_DATA_VALUE's and copy them in the STAR tuple desriptor,
    ** leave the attribute name alone as STAR could have a different
    ** name than the local, throw away the header of the local descriptor
    ** star has copied, keep the tuple size only.
    */
    status = rqu_fetch_data(assoc, sizeof(ldb_td_desc->gca_tsize), 
			    (PTR)&ldb_td_desc->gca_tsize, &eod, 0, 0, 0);
    if (status != E_RQ0000_OK || eod == TRUE)
    {
        return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }
 
    status = rqu_fetch_data(assoc, sizeof(ldb_td_desc->gca_l_col_desc),
			    (PTR)NULL, &eod, 0, 0, 0);
    if (status != E_RQ0000_OK || eod == TRUE)
    {
        return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }

    status = rqu_fetch_data(assoc, sizeof(ldb_td_desc->gca_result_modifier),
                (PTR)NULL, &eod, 0, 0, 0);
    if (status != E_RQ0000_OK || eod == TRUE)
    {
        return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }

    status = rqu_fetch_data(assoc, sizeof(ldb_td_desc->gca_id_tdscr),
                (PTR)NULL, &eod, 0, 0, 0);
    if (status != E_RQ0000_OK || eod == TRUE)
    {
        return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }

    star_col_att_p = (GCA_COL_ATT *)star_td_desc->gca_col_desc;
    ldb_col_att_p = (GCA_COL_ATT *)ldb_td_desc->gca_col_desc;

    for (i = 0; i < star_td_desc->gca_l_col_desc; ++i)
    {
	DB_DATA_VALUE	dv;

        status = rqu_col_desc_fetch(assoc, &dv);
        if (status != E_RQ0000_OK)
        {
            return(E_RQ0035_BAD_COL_DESC_FORMAT);
        }

	DB_COPY_DV_TO_ATT( &dv, ldb_col_att_p );
	MEcopy((PTR) &ldb_col_att_p->gca_l_attname,
	       sizeof(ldb_col_att_p->gca_l_attname),
	       (PTR) &att_value);
        /*
        ** advance and align for fetching the next column descriptor
        */

        ldb_col_att_p = (GCA_COL_ATT *)((PTR)ldb_col_att_p + 
					sizeof(ldb_col_att_p->gca_attdbv) + 
					sizeof(ldb_col_att_p->gca_l_attname) +
					att_value);
    }

    rqu_dmp_tdesc( assoc, (GCA_TD_DATA *)scf_td_info->scf_ldb_td_p );

    return ( E_DB_OK );
}    

/*{
** Name: rqf_delete             - cursor delete (RQR_DELETE)
**
** Description:
**         Delete the tuple referenced by cursor. Note that GCA_ID
**	is not an atomic object.
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_1_site         database server id
**         rqr_qid            query id
**         rqr_msg	      cursor delete table specification
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_DB_OK		request completed successfully
**	    E_DB_ERROR		request failed
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-may-1988 (alexh)
**          created
**	12-apr-1993 (fpang)
**	    Added support for GCA1_DELETE.
*/
DB_ERRTYPE
rqf_delete(
	RQR_CB	    *rqr_cb)
{
    RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 0);
    char	*tablename = NULL;
    i4		len = 0;    /* Must be same type as GCA_NAME.gca_l_name */

    if (assoc == NULL)
    {
	return(E_RQ0006_CANNOT_GET_ASSOCIATION);
    }

    /*
    ** If LDB protocol < 60, remove 'owner.' if present and use GCA_DELETE.
    ** If LDB protocol >= 60, use GCA1_DELETE.
    */
    if (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_60)
        rqu_putinit(assoc, GCA_DELETE);
    else
	rqu_putinit(assoc, GCA1_DELETE);

    rqu_putcid(assoc, &rqr_cb->rqr_qid);

    if (rqr_cb->rqr_tabl_name == NULL)
    {
	return(E_RQ0030_CURSOR_DELETE_FAILED);
    }

    if (assoc->rql_peer_protocol >= GCA_PROTOCOL_LEVEL_60)
    {
	/* Submit owner */
	len = 0;
	if (rqr_cb->rqr_own_name != NULL)
	    len = STlength((char *)rqr_cb->rqr_own_name);
	rqu_put_data(assoc, (PTR)&len, sizeof(len));
	rqu_put_data(assoc, (PTR)rqr_cb->rqr_own_name, len);
	tablename = (char *)rqr_cb->rqr_tabl_name;
    }
    else
    {
	/* Protocol < 60. Remove 'owner.' if present */
	tablename = STindex((char *)rqr_cb->rqr_tabl_name, (char *)".",
			    (i4)sizeof(*rqr_cb->rqr_tabl_name));
	if (tablename != (char *)NULL)
	    tablename++;    /* skip over '.' */
	else
	    tablename = (char *)rqr_cb->rqr_tabl_name;    /* not found */
    }    
    
    /* Submit name. */
    if ((len = STlength(tablename)) <= 0)
	return(E_RQ0030_CURSOR_DELETE_FAILED);

    rqu_put_data(assoc, (PTR)&len, sizeof(len));
    rqu_put_data(assoc, tablename, len); 

    if ( rqu_putflush(assoc, TRUE) == E_RQ0000_OK)
    {
	if (rqu_get_reply(assoc, -1) == GCA_RESPONSE)
	{
	    GCA_RE_DATA	*response = (GCA_RE_DATA *)assoc->rql_rv.gca_data_area;
	    rqr_cb->rqr_tupcount = response->gca_rowcount;
	    return(E_RQ0000_OK);
	}
    }
    (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
    return(E_RQ0030_CURSOR_DELETE_FAILED);
}

/*{
** Name: rqf_end              - transaction end (RQR_END)
**
** Description:
**         QUEL transaction commit and end.
**
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_1_site           database server id
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_RQ0022_END_FAILED
**	    E_RQ0006_CANNOT_GET_ASSOCIATION
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-may-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqf_end(
	RQR_CB	    *rqr_cb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 1);

  if (assoc == NULL)
    /* no op */
    return(E_RQ0006_CANNOT_GET_ASSOCIATION);

  rqr_cb->rqr_error.err_code = rqu_send(assoc, (i4)0, TRUE, GCA_Q_ETRAN);
  if (rqr_cb->rqr_error.err_code == E_RQ0000_OK 
	&& rqu_get_reply(assoc, -1) == GCA_S_ETRAN)
  {
      return(E_RQ0000_OK);
  }

  (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
  return(E_RQ0022_END_FAILED);
}

/*
** {
** Name: rqf_fetch            - cursor fetch (RQR_FETCH)
**
** Description:
**         Fetch tuple referenced by cursor.
**	   This routine currently does NOT handle multi-message tuple
**	   correctly.
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_1_site         database server id
**         rqr_qid            this required for query type RQT_EXEC 
**                            RQT_FETCH RQT_DELETE RQT_CLOSE
**
** Outputs:
**      rqr_cb->
**         rqr_msg            tuple buffer for fetch
**	   rqr_error	      error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK		request completed successfully
**	    E_RQ0028_CURSOR_FETCH_FAILED
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-may-1988 (alexh)
**          created
**      14-Oct-1993 (fred)
**          Add handling for large objects.
*/
DB_ERRTYPE
rqf_fetch(
	RQR_CB	    *rqr_cb)
{
  DB_STATUS	status;
  RQL_ASSOC	*assoc = rqu_check_assoc(rqr_cb, 0);
  QEF_DATA	*qef_data = (QEF_DATA *)rqr_cb->rqr_tupdata;
  i4	gca_return;
  i4            amount_returned;
  i4            eo_tuple = TRUE;

  rqr_cb->rqr_tupcount = 0;
  rqr_cb->rqr_count = 0;
  rqr_cb->rqr_end_of_data = FALSE;
    
  if (assoc == NULL)
    /* no op */
    return(E_RQ0006_CANNOT_GET_ASSOCIATION);

  rqu_putinit(assoc, GCA_FETCH);
  rqu_putcid(assoc, &rqr_cb->rqr_qid);
  if (rqu_putflush(assoc, TRUE) == E_RQ0000_OK)
  {  
    gca_return = rqu_get_reply(assoc, -1);
    if (gca_return == GCA_TUPLES)
    {
      /* let's hope we will force a read */
      assoc->rql_b_ptr = assoc->rql_rv.gca_data_area;
      assoc->rql_b_remained = assoc->rql_rv.gca_d_length;
      /* fetch 0 or 1 tuple */
      status = rqu_fetch_data(assoc,
			      qef_data->dt_size,
			      qef_data->dt_data,
			      &rqr_cb->rqr_end_of_data,
			      1, /* Asking for whole tuple */
			      &amount_returned,
			      &eo_tuple);
      if (status == E_DB_OK)
      {
	  if (rqr_cb->rqr_end_of_data == FALSE)
	  {
	    ++rqr_cb->rqr_count;
	    qef_data->dt_size = amount_returned;
	    if (eo_tuple)
		++rqr_cb->rqr_tupcount;
	    (VOID)rqu_get_reply(assoc, -1);	
 	  }
      }
      else
      {
        (VOID)rqu_get_reply(assoc, -1);	
      }
      if ((rqr_cb->rqr_tupcount > 0) || (rqr_cb->rqr_count > 0))
	  rqr_cb->rqr_end_of_data = FALSE;
      if (status == E_DB_OK)
	return(E_RQ0000_OK);
    }
    else if (gca_return == GCA_RESPONSE)
      return(E_RQ0000_OK);
  }

  (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
  return(E_RQ0028_CURSOR_FETCH_FAILED);
}

/*{
** Name: rqf_s_begin	- session initiate (RQR_S_BEGIN)
**
** Description:
**      Initialize a RQF session control block. An ulm stream
**	is opened.
**
** Inputs:
**      rqr_cb       pointer to RQF request control block
**
** Outputs:
**      rqr_cb->
**        rqr_session       RQF internal control block is initialized
**        rqs_assoc_list    NULL
**        rqs_stream_id     ulm stream id for the session
**	  rqr_error	    error reported by RQF
**        
**	Returns:
**	    E_RQ000_OK			request completed successfully
**	    E_RQ0010_ULM_OPEN_FAILED	cannot open session stream
**	    E_RQ0008_SCU_MALLOC_FAILED	cannot allocate RQS_CB
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-may-1988 (alexh)
**          created
**	2-aug-1988 (alexh)
**	    allocate session control block from SCF.
**	27-jul-1990 (georgeg)
**	    initialized session.rqs_cdb_name.
**	15-oct-1992 (fpang)
**	    Replaced sc0m_rqf_allocate() calls with ULM calls.
*/
DB_ERRTYPE
rqf_s_begin(
	RQR_CB	*rcb)
{
    RQS_CB	*session = NULL;
    ULM_RCB	ulm;	/* session ulm */

    char	*tran_place;
    RQS_CB	*rqs = (RQS_CB *)NULL; /* For RQTRACE */


    /*
    ** There are no timing issues here, we whant to turn it on and off at will
    */
    NMgtAt("II_RQF_LOG", &tran_place);
    if ((tran_place) && (*tran_place))
    {
	if (!CMdigit(tran_place) ||
	    (CVan(tran_place,&Rqf_facility->rqf_dbg_lvl) != OK)
	   )
	{
	    /* Default to old fashioned debug level, 4 */
	    Rqf_facility->rqf_dbg_lvl = 4;
	}
    }
    else
    {
	Rqf_facility->rqf_dbg_lvl = 0;	
    }

    /* allocate stream for the session */
    /* borrow QEF's id */
    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Rqf_facility->rqf_pool_id;
    ulm.ulm_blocksize = sizeof(RQL_ASSOC);
    ulm.ulm_memleft = &Rqf_facility->rqf_ulm_memleft;
    ulm.ulm_streamid_p = &ulm.ulm_streamid;
    /* Allocate a thread-safe, private stream */
    /* Open streamd and allocate RQS_CB with one effort */
    ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    ulm.ulm_psize = sizeof(RQS_CB);

    if (ulm_openstream(&ulm) != E_DB_OK)
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:rqf_s_begin. Stream open failed\n"));
	return(E_RQ0010_ULM_OPEN_FAILED);
    }

    session = (RQS_CB *)(rcb->rqr_session = ulm.ulm_pptr);

    MEfill(sizeof(session->rqs_cdb_name), '\0', 
	    (PTR)&session->rqs_cdb_name);
    session->rqs_streamid = ulm.ulm_streamid;
    session->rqs_current_site = session->rqs_assoc_list = NULL;
    session->rqs_set_options = NULL;
    session->rqs_dcnct_assoc = (RQL_ASSOC *)NULL;
    session->rqs_dc_mode = FALSE;
    session->rqs_user_cdb = (RQL_ASSOC *)NULL;
    session->rqs_dbg_lvl = 0;
    session->rqs_cpstream = NULL;
    session->rqs_setstream = NULL;
    session->rqs_trfmt = rcb->rqr_trfmt;
    session->rqs_trsize = rcb->rqr_trsize;
    if ( rcb->rqr_user == NULL )
    {
	*session->rqs_user.dd_u1_usrname = EOS;
    }
    else 
    {
	STRUCT_ASSIGN_MACRO( *rcb->rqr_user, session->rqs_user );
    }

    STRUCT_ASSIGN_MACRO( *rcb->rqr_com_flags, session->rqs_com_flags );

    STRUCT_ASSIGN_MACRO( *rcb->rqr_mod_flags, session->rqs_mod_flags );
    if ((rcb->rqr_q_language == DB_NOLANG) || (rcb->rqr_q_language != DB_SQL) ) 
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqf_s_begin. Language type is forced to DB_SQL\n"));
	session->rqs_lang = DB_SQL;
    }
    else
    {
	session->rqs_lang = rcb->rqr_q_language;
    }


    return(E_RQ0000_OK);
}

/*{
** Name: rqf_s_end	- session termination (RQR_S_END)
**
** Description:
**	Terminate all LDB associations of the session, and release all
**	memory resources held by it.
**      
**
** Inputs:
**      rqr_cb       pointer to RQF request control block
**      
**
** Outputs:
**	None
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_RQ0013_ULM_CLOSR_FAILED
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jul-1988 (alexh)
**          created
**	15-oct-1992 (fpang)
**	    Replace sc0m_rqf_allocate() calls with ULM calls.
**	10-jul-1997 (nanpr01)
**	    You have to deallocate all the memory hanging from this control
**	    block before deallocating the control block.
**	07-Aug-1997 (jenjo02)
**	    Clear streamids before deallocating memory.
*/
DB_ERRTYPE
rqf_s_end(
	RQR_CB	    *rqr_cb)
{
    RQS_CB	*session = (RQS_CB *)rqr_cb->rqr_session;
    ULM_RCB	ulm;
    DD_PACKET   *pktptr = session->rqs_set_options;
    DD_PACKET	*nxtpkt;
    RQS_CB	*rqs = (RQS_CB *)rqr_cb->rqr_session; /* For RQTRACE */

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_memleft = &Rqf_facility->rqf_ulm_memleft;
    if (session->rqs_setstream != NULL)
    {
        ulm.ulm_streamid_p = &session->rqs_setstream;
        if (ulm_closestream(&ulm) != E_DB_OK)
        {
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqs_s_end. Stream closure failed\n"));
	    return(E_RQ0013_ULM_CLOSE_FAILED);
        }
	/* ULM has nullified rqs_setstream */
    }

    /*
    ** release all associations 
    */
    (VOID)rqu_end_all_assoc(rqr_cb, session->rqs_assoc_list);


    /*
    ** close ulm memory stream 
    */
    ulm.ulm_streamid_p = &session->rqs_streamid;
    rqr_cb->rqr_session = NULL; 
    if (ulm_closestream(&ulm) != E_DB_OK)
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:rqs_s_end. Stream closure failed\n"));
	return(E_RQ0013_ULM_CLOSE_FAILED);
    }
    /* ULM has nullified rqs_streamid */
    return(E_RQ0000_OK);
}

/*{
** Name: rqf_interrupt	- send interrupt to database server
**			  (RQR_INTERRUPT RQR_T_FLUSH)
** Description:
**      interrupt is send to the designated database server.
**      Interrupt cannot be asynch. 
**
** Inputs:
**      rqr_cb	    pointer to RQF request control block
**        rpr_site  database server id
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_RQ0017_NO_ACK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-may-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqf_interrupt(
	RQR_CB		*rqr_cb,
	i4		interrupt_type)
{
    return(rqu_interrupt(rqu_check_assoc(rqr_cb, 0), interrupt_type));
}

/*{
** Name: rqf_invoke           - execute repeat query (RQR_EXECUTE)
**
** Description:
**         execute a defined repeat query.
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_1_site         database server id
**         rqr_qid            id of the query
**         rqr_msg            param value block buffer
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK		request completed successfully
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-may-1988 (alexh)
**          created
**      14-may-1989 (carl)
**	    fixed to return E_RQ0040_UNKNOWN_REPEAT_Q for case of GCA_RESPONSE
**	24-sep-1990 (fpang)
**	    return GCA_ERROR on a GCA error.
**	15-oct-1992 (fpang)
**	    Removed RQTRACE of parameters because rqu_putdbval() does it now.
**	    Call rqu_putcid() to construct GCA_ID instead of doing it here.
*/
DB_ERRTYPE
rqf_invoke(
	RQR_CB	    *rqr_cb)
{
    RQL_ASSOC	    *assoc = rqu_check_assoc(rqr_cb, 0);
    QEF_PARAM	    *inv_parms = (QEF_PARAM *)rqr_cb->rqr_inv_parms;
    DB_ERRTYPE	    status = E_RQ0000_OK;
    GCA_ID	    gca_qid;
    GCA_RE_DATA	    *response;
    RQS_CB	    *rqs = (RQS_CB *)rqr_cb->rqr_session; 


    if (assoc == NULL)
    {
    	return(E_RQ0006_CANNOT_GET_ASSOCIATION);
    }

    rqu_putinit(assoc, GCA_INVOKE);
    rqu_putcid(assoc, &rqr_cb->rqr_qid);

    /*
    ** put all the parameter data values. Note db value array starts at 1 
    */
    status = rqu_put_iparms(assoc, inv_parms->dt_size,
			   ((DB_DATA_VALUE **)inv_parms->dt_data)+1);
    assoc->rql_described = FALSE;
    /*
    ** execute the query 
    */
    if (status == E_RQ0000_OK && rqu_putflush(assoc, TRUE) == E_RQ0000_OK)
    {
	switch (rqu_get_reply(assoc, -1))
	{
	    case GCA_QC_ID:
		if (rqu_get_reply(assoc, -1) != GCA_RESPONSE)
		{
		    (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
		    return(E_RQ0029_CURSOR_EXEC_FAILED);
		}
		else
		{
		    response = (GCA_RE_DATA *)assoc->rql_rv.gca_data_area;
		    rqr_cb->rqr_tupcount = response->gca_rowcount;
		}
		break;
	    case GCA_RESPONSE:
	    {
		response = (GCA_RE_DATA *)assoc->rql_rv.gca_data_area;

		rqr_cb->rqr_tupcount = response->gca_rowcount;
		if (response->gca_rqstatus & GCA_RPQ_UNK_MASK)
		{
		    return(E_RQ0040_UNKNOWN_REPEAT_Q);
		}
	    }
		break;
	    case GCA_TDESCR:
		break;
	    case GCA_ERROR:
	    {
		response = (GCA_RE_DATA *)assoc->rql_rv.gca_data_area;

		rqr_cb->rqr_tupcount = response->gca_rowcount;
		if (response->gca_rqstatus & GCA_RPQ_UNK_MASK)
		{
		    return(E_RQ0040_UNKNOWN_REPEAT_Q);
		}
		else
		{
		    return(E_RQ0029_CURSOR_EXEC_FAILED);
		}
	    }
		break;
	    default:
		(VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
		return(E_RQ0029_CURSOR_EXEC_FAILED);
		break;
	}
	return(E_RQ0000_OK);
    }
    (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
    return(E_RQ0029_CURSOR_EXEC_FAILED);
}

/*{
** Name: rqf_continue	- continue message exchange
**
** Description:
**	This routine reads(or writes) a message from(or to) a 
**	directly connected LDB. This call is only valid after
**	rqf_connect. If rqr_dc_txn is true, this implies a
**	direct execute immediate call which requires construction
**	of a GCA_QUERY message. Else a message is relayed.
**
** Inputs:
**	rcb->
**	  rqr_dc_blkp		data exchar struct
**	    sdc_mode		SDC_READ or SDC_WRITE
**	    sdc_msg_type	message type being sent
**	    sdc_ldb_msg		message from LDB if sdc_mode  is
**				SDC_READ
**	    sdc_rv		sdc_ldb_msg
**	    sdc_client_msg	message from client if sdc_mode is
**				SDC_WRITE
**
** Outputs:
**	rcb->
**	  rqr_dc_blkp		data exchar struct
**	    sdc_msg_type	message type receiveed
**	    sdc_ldb_msg		message from LDB if sdc_mode  is
**				SDC_READ
**	    sdc_rv		sdc_ldb_msg
**        
** Returns:
**      E_RQ0000_OK	
**      E_RQ0011_INVALUD_READ		read is rejected.
**	E_RQ0031_INVALID_CONTINUE	no rqf_connect context.
**	E_RQ0012_INVALID_WRITE		write is rejected.
**
** Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-Aug-1988 (alexh)
**          created
**	15-oct-1992 (fpang)
**	    1. Support SDC_CHK_INT mode. This mode checks for out of bandwidth
**	       interrupts during copy processing.
**	    2. Removed tracing if DEI queries because rqu_put_dbval() does it
**	       now.
**	08-dec-1992 (fpang)
**	    1. Call rqu_put_qtxt() to output query text and parameters.
**	    2. Set sdc_ldb_status to SDC_LOST_ASSOC if LDB disconnects.
**	05-feb-1993 (fpang)
**	    Added support for detecting commits/aborts when executing a 
**	    a procedure or in direct execute immediate.
**	26-apr-93 (vijay)
**	    Add explicit type cast for assignment for silencing AIX compiler.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	16-dec-96 (kch)
**	    Assign assoc->rql_timeout to rcb->rqr_timeout - rql_timeout was
**	    unassigned and was causing CSsuspend() to time out later on.
**	    This change fixes bug 79767.
*/
DB_ERRTYPE
rqf_continue(
	RQR_CB	*rcb)
{
    RQL_ASSOC	*assoc = ((RQS_CB *)rcb->rqr_session)->rqs_current_site;
    SCC_SDC_CB	*dc = (SCC_SDC_CB *)rcb->rqr_dc_blkp;
    GCA_RV_PARMS	*receive;
    STATUS	status;
    RQS_CB	*rqs = (RQS_CB *)rcb->rqr_session;

    if (assoc == NULL)
    {
	return(E_RQ0031_INVALID_CONTINUE);
    }

    assoc->rql_rqr_cb = rcb;
    assoc->rql_timeout = (rcb->rqr_timeout > 0 ? rcb->rqr_timeout : 0);

    if (assoc && dc)
	dc->sdc_peer_protocol = assoc->rql_peer_protocol;

    if (dc->sdc_mode == SDC_CHK_INT)
    {
	/* This is usually copy processing. Check if there are interrupts */
        return (rqu_sdc_chkintr(rcb));
    }
    else if (dc->sdc_mode == SDC_READ)
    {
	/* a direct connect read */
	receive = &assoc->rql_rv;


	if (assoc->rql_interrupted == TRUE)
	{
	    dc->sdc_ldb_status = SDC_INTERRUPTED;
	    assoc->rql_interrupted = FALSE;
	    return(E_RQ0011_INVALID_READ);
	}


	if (assoc->rql_comm.rqc_status == RQC_INTERRUPTED) 
	{
	    status = rqu_rxiack(GCA_RECEIVE, (PTR)receive, &receive->gca_status, assoc);
	}
	else
	{
	    status = rqu_gccall(GCA_RECEIVE, (PTR)receive, &receive->gca_status, assoc);
	}
	
	if (status != E_GC0000_OK)
	{
	    if ( 
		(assoc->rql_comm.rqc_status == RQC_INTERRUPTED) 
		    || 
		(assoc->rql_interrupted == TRUE)
	       )
	    {
		dc->sdc_ldb_status = SDC_INTERRUPTED;
		assoc->rql_interrupted = FALSE;
	    }
	    else if (assoc->rql_comm.rqc_status == RQC_ASSOC_TERMINATED)
	    {
		dc->sdc_ldb_status = SDC_LOST_ASSOC;
	    }
	    return(E_RQ0011_INVALID_READ);
	}
	/* unconditional trace */
	rqu_dump_gca_msg(assoc);

	(VOID)rqu_gcmsg_type(GCA_RECEIVE, receive->gca_message_type,
				assoc);

	if (receive->gca_message_type == GCA_RESPONSE)
	{
	    GCA_RE_DATA *re = (GCA_RE_DATA *) receive->gca_data_area;
	    if (re->gca_rqstatus & GCA_FAIL_MASK)
		assoc->rql_re_status |= RQF_01_FAIL_MASK;
	    if (re->gca_rqstatus & GCA_INVSTMT_MASK)
		assoc->rql_re_status |= RQF_02_STMT_MASK;
	    if (re->gca_rqstatus & GCA_LOG_TERM_MASK)
		assoc->rql_re_status |= RQF_03_TERM_MASK;
	    if (re->gca_rqstatus & GCA_ILLEGAL_XACT_STMT)
		assoc->rql_re_status |= RQF_04_XACT_MASK;
	}
	dc->sdc_msg_type = receive->gca_message_type;
	dc->sdc_ldb_msg = (PTR)receive->gca_buffer;
	dc->sdc_rv = (PTR)receive;
    }
    /* else must be a SCD_WRITE */
    else if (rcb->rqr_dc_txn)	/* this indicates DEI */
    {
	DB_ERRTYPE	status;
	u_i4	qmodifier = GCA_OK_MASK;

	if (rcb->rqr_2pc_txn == TRUE)
	    qmodifier |= GCA_Q_NO_XACT_STMT;

	/* construct the GCA_QUERY message using RQF send buffer */
	assoc->rql_described = FALSE;

	status = rqu_put_qtxt(assoc, GCA_QUERY, rcb->rqr_q_language, qmodifier, 
			(DD_PACKET *)dc->sdc_client_msg, (DD_NAME *)NULL);
	if (status == E_RQ0000_OK)
	    status = rqu_putflush(assoc, TRUE);

	if ( 
	     (status != E_RQ0000_OK)
		&& 
	     ((assoc->rql_interrupted == TRUE) || 
	      (assoc->rql_comm.rqc_status == RQC_INTERRUPTED))
	   )
	{
	    dc->sdc_ldb_status = SDC_INTERRUPTED;
	    return(E_RQ0000_OK);
	}

	return(status);
    }
    else	/* just a plain write */
    {
	GCA_SD_PARMS	send;
	GCA_RV_PARMS	*rv = (GCA_RV_PARMS *)dc->sdc_client_msg;

        if (rv->gca_message_type == GCA_TDESCR)
        {
            ++assoc->rql_nostatments;
            ((GCA_TD_DATA *)dc->sdc_td2_ldesc)->gca_id_tdscr = 
                            (u_i4)assoc->rql_nostatments;
        }
        send.gca_buffer = rv->gca_buffer;
        send.gca_message_type = rv->gca_message_type;
        send.gca_msg_length = rv->gca_d_length;
        send.gca_end_of_data = rv->gca_end_of_data;
        send.gca_descriptor = dc->sdc_td2_ldesc;
	send.gca_modifiers = 0;
	if (rqu_xsend(assoc, &send) != E_GC0000_OK)
	{
	    if (assoc->rql_interrupted == TRUE)
	    {
		dc->sdc_ldb_status = SDC_INTERRUPTED;
	    }
	    return(E_RQ0000_OK);
	}
    }

  return(E_RQ0000_OK);
}

/*{
** Name: rqf_secure           - prepare commit
**
** Description:
**         2pc transaction prepare commit
** Inputs:
**      rqr_cb->	      pointer to RQF request control block
**         rqr_1_site           database server id
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK		request completed successfully
**	    E_RQ0019_SECURE_FAILED
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-may-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqf_secure(
	RQR_CB	    *rqr_cb)
{
    RQL_ASSOC	    *assoc = rqu_check_assoc(rqr_cb, 0);
    i4		    message,
		    secure_size = sizeof(GCA_TX_DATA);
    GCA_TX_DATA	    dx_data;
    DB_ERRTYPE	    status;
    RQS_CB	    *rqs = (RQS_CB *)rqr_cb->rqr_session;

    if (assoc == NULL)
    {
    	return(E_RQ0006_CANNOT_GET_ASSOCIATION);
    }

    if (assoc->rql_turned_around == FALSE)
    {
	/*
	** QEF ot TPF is confused, the protocol has not turned around yet, we must clean
	** the association first and put the remote partner in a read state.
	*/
	(VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
	return(E_RQ0048_SECURE_FAILED);
    }

    /*
    ** place the dx id in the output buffer and execute the secure request. 
    ** reconstruct the dx id because of potential struct differences. 
    */
    dx_data.gca_tx_id.gca_index[0] = (i4)rqr_cb->rqr_2pc_dx_id->dd_dx_id1;
    dx_data.gca_tx_id.gca_index[1] = (i4)rqr_cb->rqr_2pc_dx_id->dd_dx_id2;

    /*
    ** blank the name LDBMS does not need the name
    */
    MEfill(sizeof(dx_data.gca_tx_id.gca_name), ' ', 
	    (PTR)dx_data.gca_tx_id.gca_name);
    dx_data.gca_tx_type = GCA_DTX;
    RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. Secure Dx id Hi:%d Lo:%d\n"), 
		assoc->rql_assoc_id,
		dx_data.gca_tx_id.gca_index[0], dx_data.gca_tx_id.gca_index[1]);

    /*
    ** put the GCA_ID structure to the stream 
    */
    status = rqu_put_data(assoc, (PTR)&dx_data, secure_size);
    
    if (status == E_RQ0000_OK)
    {
	status = rqu_send(assoc, (i4) secure_size, TRUE, GCA_SECURE);
    }

    if (status != E_RQ0000_OK)
    {
	(VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
	rqr_cb->rqr_dx_status = RQF_2ERROR_DX;
	return(E_RQ0048_SECURE_FAILED);
    }

    message = rqu_get_reply(assoc, -1);
    if (message == GCA_DONE )
    {
	rqr_cb->rqr_dx_status = RQF_0DONE_DX;
	return(E_RQ0000_OK);
    }
    else if (message == GCA_REFUSE)
    {
	rqr_cb->rqr_dx_status = RQF_1REFUSE_DX;
	return(E_RQ0048_SECURE_FAILED);
    }
    else
    {
    	rqr_cb->rqr_dx_status = RQF_2ERROR_DX;
	return(E_RQ0048_SECURE_FAILED);
    }
}

/*{
** Name: rqf_shutdown	- facility shutdown (RQR_SHUTDOWN)
**
** Description:
**      Make sure all resources and associations are freed. ULM memory
**      pool is freed.
**
** Inputs:
**      rqr_cb	    pointer to RQF request control block
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0018_SHUTDOWN_FAILED
**	    E_RQ0000_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-may-1988 (alexh)
**          created
**	15-oct-1992 (fpang)
**	    Replace sc0m_rqf_allocate() with ULM calls.
*/
DB_ERRTYPE
rqf_shutdown(
	RQR_CB	    *rqr_cb)
{
  DB_STATUS	ulm_status;
  DB_STATUS	scf_status;
  SCF_CB	scf_cb;
  ULM_RCB	ulm;

  /* borrow QEF's id */
  ulm.ulm_facility = DB_QEF_ID;
  ulm.ulm_poolid = Rqf_facility->rqf_pool_id;
  ulm.ulm_memleft = &Rqf_facility->rqf_ulm_memleft;
  ulm_status = ulm_shutdown(&ulm);

  /* deallocate the RQF control block from SCF */
  scf_cb.scf_length = sizeof(SCF_CB);
  scf_cb.scf_type = SCF_CB_TYPE;
  /* borrow QEF's id */
  scf_cb.scf_facility = DB_QEF_ID;
  scf_cb.scf_session = DB_NOSESSION;
  scf_cb.scf_scm.scm_in_pages = sizeof(RQF_CB)/SCU_MPAGESIZE + 1;
  scf_cb.scf_scm.scm_addr = (char *)Rqf_facility;
  scf_status = scf_call(SCU_MFREE, &scf_cb);
  if (ulm_status != E_DB_OK || scf_status != E_DB_OK)
    return(E_RQ0018_SHUTDOWN_FAILED);

  return(E_RQ0000_OK);
}

/*{
** Name: rqf_startup	- facility startup (RQR_STARTUP)
**
** Description:
**      facility startup. Facility control block is allocated. ULM
**      memory pool is allocated for the facility.
**      
** Inputs:
**	rcb		request control block
**
** Outputs:
**	rcb->
**	    rqf_ulm_cb	  ulm memory pool
**	    rqr_memleft	  memory pool remained
**        
**	Returns:
**	    E_RQ0008_SCU_MALLOC_FAILED
**	    E_RQ0009_ULM_STARTUP_FAILED
**	    E_RQ0000_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-may-1988 (alexh)
**          created
**      13-feb-1989 (georgeg)
**          added RQF_MAX_ASSOC, maximum associations per session.
**      27-feb-1989 (georgeg)
**          added II_RQF_LOG to control output to TR... log file.
**	15-oct-1992 (fpang)
**	    Replace sc0m_rqf_allocate() with ULM calls.
**	18-jan-1993 (fpang)
**	    Encode sqlstates SS5000G_DBPROC_MESSAGE and
**	    SS50009_ERROR_RAISED_IN_DBPROC to detect messages and raised errors 
**	    when executing registered database procedures.
**	08-oct-1992 (fpang)
**	    Fixed B54385, (Create ..w/journaling fails). Recognize
**	    SS000000_SUCCESS as a successful status message.
**	1-mar-94 (robf)
**          Initialize flagsmask to indicate B1 server.
**	9-may-94 (robf)
**          Updated tracing to match swm's interface.
**	12-apr-2002 (devjo01)
**	    Correct questionable calculation of 'rqf_len_vnode'.
**	14-Jun-2004 (schka24)
**	    Safe handling of environment variable.
**	21-Sep-2009 (rajus01) SD issues: 130145 and 130521. Bug #:120803
**	    Use the local_vnode from config.dat instead of II_GCNXX_LCL_VNODE
**	    symbol table entry as this had been deprecated since II 2.5.
*/
DB_ERRTYPE
rqf_startup(
	RQR_CB	*rcb)
{
    SCF_CB	    scf_cb;
    ULM_RCB	    ulm;
    char	    *tran_place;
    char    *vnode = NULL;
    char    *installation = NULL;
    char    log_name[LG_MAX_CTX+2];
    RQS_CB  *rqs = (RQS_CB *)NULL; 	/* For RQTRACE */
    SCF_SCI       sci_list[1];
    DB_STATUS     status=E_DB_OK;

    /* set up scf control block to allocate memory */
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    /* borrow QEF's id */
    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_scm.scm_functions = SCU_MZERO_MASK;
    scf_cb.scf_scm.scm_in_pages = sizeof(RQF_CB)/SCU_MPAGESIZE + 1;

    /* request memory allocation to SCf */
    if (scf_call(SCU_MALLOC, &scf_cb) != E_DB_OK)
	return(E_RQ0008_SCU_MALLOC_FAILED);

    /* 
        initialize facility server control block. ulm memory pool is
	is requested.
    */
    Rqf_facility = (RQF_CB *)scf_cb.scf_scm.scm_addr;

    /* borrow QEF's id */
    ulm.ulm_facility = DB_QEF_ID;

    /* just a random number */
    ulm.ulm_sizepool = 100 *			    /* 100 sessions */
	(  sizeof(RQS_CB) +		            /* Session CB */
	   RQF_MAX_ASSOC * sizeof(RQL_ASSOC) +      /* 32 Associations */
	   1024 +				    /* 'Set' querires */
	   4096			                    /* copy map */
	);

    ulm.ulm_blocksize = sizeof(RQS_CB);
    RQTRACE(rqs,2)(RQFMT(rqs,
	"RQF:rqf_startup. Sizepool %x.\n"),ulm.ulm_sizepool);
    if (ulm_startup(&ulm) != E_DB_OK)
      return(E_RQ0009_ULM_STARTUP_FAILED);
    Rqf_facility->rqf_pool_id = ulm.ulm_poolid;
    Rqf_facility->rqf_cluster_p = (PTR)NULL;
    Rqf_facility->rqf_num_nodes = 0;
    Rqf_facility->rqf_dbg_lvl = 0;
    Rqf_facility->rqf_ulm_memleft = ulm.ulm_sizepool;
    Rqf_facility->rqf_flagsmask =0;

    MEfill(sizeof(log_name), ' ', log_name);

    /*
    ** get the installation, vnode name
    */
    NMgtAt("II_INSTALLATION", &installation);
    if ( installation == NULL || *installation == EOS )
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF: Environment II_INSTALLATION is not defined\n"));
    }
    else
    {
	/*
        ** get the node name
        */
	if ( PMget( "!.local_vnode",  &vnode) != OK )
        {
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqf_startup. LOCAL_VNODE is not defined\n"));
	    Rqf_facility->rqf_vnode[0] = EOS;
	    Rqf_facility->rqf_len_vnode = 0;
        }
        else
        {
	    STncpy( Rqf_facility->rqf_vnode, vnode,
			 sizeof(Rqf_facility->rqf_vnode));
	    Rqf_facility->rqf_len_vnode =
	    		 STnlength( sizeof(Rqf_facility->rqf_vnode),
	    		 Rqf_facility->rqf_vnode );
        }

    }
   
    /*
    ** Control of the print flag for RQF log messages.
    */
    NMgtAt("II_RQF_LOG", &tran_place);
    if ((tran_place) && (*tran_place))
    {
	if (!CMdigit(tran_place) ||
	    (CVan(tran_place,&Rqf_facility->rqf_dbg_lvl) != OK)
	   )
	{
	    /* Default to old fashioned debug level, 4 */
	    Rqf_facility->rqf_dbg_lvl = 4;
	}
    }
    else
    {
       Rqf_facility->rqf_dbg_lvl = 0;
    }

    /* Encode SQLSTATEs. */
    ss5000g_dbproc_message = ss_encode(SS5000G_DBPROC_MESSAGE);
    ss50009_error_raised_in_dbproc = ss_encode(SS50009_ERROR_RAISED_IN_DBPROC);
    ss00000_success = ss_encode(SS00000_SUCCESS);
    
    return(E_RQ0000_OK);
}

/*
** {
** Name: rqf_t_fetch	-  fetch a tuple
**
** Description:
**	fetch a result tuple from previous requer execution.
**	Return column bind addresses must be specified.
**	Fetch must immediately follow query execution, and cannot
**	intersperse with other operations to the same site.
**	Fetch should be repeated until end of data flag is TRUE
**	
**	It is assumed that the column size specified in the rqr_bind_addr
**	is correct. No verification agains GCA_TD_DATA is done.
**
**	Because GCA tuple data message is not always alinged on tuple
**	boundary, the fetch logic handle tuple breakage.
**
** Inputs:
**      rqr_cb	    pointer to RQF request control block
**        rqr_1_site          database server id
**	  rqr_col_count	    number of columns being fetched
**	  rqr_bind_addrs    return column address bindings
**
** Outputs:
**	rqr_cb
**	  rqr_end_of_data	indicate whether a tuple is fetched
**	  rqr_error		error reported by RQF and GCA
**	  rqr_bind_addrs    	return column address bindings
**		rqb_length	set to actual length if requested type 
**				is DB_DBV_TYPE
**      
**	Returns:
**		E_RQ0006_CANNOT_GET_ASSOCIATION
**		E_RQ0015_UNEXPECTED_MESSAGE
**		E_RQ0016_CONVERSION_ERROR
**		E_RQ0001_OK
**      	E_RQ0002_NO_TUPLE_DESCRIPTION
**		E_RQ0001_TOO_FEW_COLUMNS
**		E_RQ0004_BIND_BUFFER_TOO_SMALL
**		E_RQ0003_TOO_MANY_COLUMNS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jun-1988 (alexh)
**          created
**	28-aug-1990 (teresa and fpang)
**	    added support for nullable data types
**	09-may-1991 (fpang)
**	    Fixed rqf_t_fetch() to handle cases where length of expected
**	    equals actual length including the null. Basically, always take 
**	    care of the null character if null is not expected.
**	    Fixes B37148.
**	17-sep-1992 (fpang)
**	    Call adf to convert float to int, and int to float. Should
**	    eventually change entire function to call adf for all conversions.
**	    Fixes B45116.
**	15-oct-1992 (fpang)
**	    Rewrote NULL byte handling more reliably.
**	09-sep-1993 (barbara)
**	    When reading varchar or text types, length from the count field
**	    (leading two bytes of data) may be less than db_length.  If so,
**	    skip remaining bytes.  Fixes B54508.
**       6-Oct-1993 (fred)
**          Added byte string datatype support.
**      14-Oct-1993 (fred)
**          Added (unused in this routine) parameters for rqu_data_fetch().
**	04-Nov-1993 (fpang)
**	    Don't set NULL indicator if at end-of-data, because 'nullb' may
**	    be wrong.
**	    Fixes B56429, Select from aggregate where data is nullable may
**	    return inconsistant results.
**	25-april-1997(angusm)
**	    bug 81471: clean up INT/FLOAT coercion case.
**      05-Dec-2001 (hanal04) Bug 105723, INGSTR 42.
**          Added support for DB_DEC_TYPE to DB_DEC_TYPE conversion.
**          ADF routines could not be used as the remote response
**          was from a Gateway that did not have standard Ingres length
**          and precision.
**	1-nov-05 (inkdo01)
**	    Add support for bigint/i8.
**	17-Dec-2008 (kiria01) SIR120473
**	    Initialise the ADF_FN_BLK.adf_pat_flags.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
*/
DB_ERRTYPE
rqf_t_fetch(
	RQR_CB	    *rqr_cb)
{
    RQL_ASSOC	    *assoc = rqu_check_assoc(rqr_cb, 0);
    RQB_BIND	    *bind = rqr_cb->rqr_bind_addrs;
    i4		    col_count = rqr_cb->rqr_col_count;
    DB_ERRTYPE	    status;
    u_i2	    len;
    i4	    r_length, length;
    DB_DT_ID	    r_dt_id, dt_id;
    RQS_CB	    *rqs = (RQS_CB *)rqr_cb->rqr_session;
    bool	    convert_ok = TRUE;
    unsigned char   fill_char;

    if (assoc == NULL)
    {
	return(E_RQ0006_CANNOT_GET_ASSOCIATION);
    }

    if (assoc->rql_described == FALSE)	/* no tuple description yet */
    {
	/*
	** initialize fetch stream 
	*/
	assoc->rql_described = TRUE;
	assoc->rql_b_ptr = assoc->rql_rv.gca_data_area;
	assoc->rql_b_remained = assoc->rql_rv.gca_d_length;

	/*
	** store tuple description in bind 
	*/
	if ( 
	    (rqr_cb->rqr_error.err_code = 
		rqu_td_save(col_count, bind, assoc)
	    ) != E_RQ0000_OK
	   )
	{
	    return(rqr_cb->rqr_error.err_code);
	}
	/*
	** this forces a read on the association 
	*/
	assoc->rql_b_remained = 0;
    }

    /*
    ** transfer each column individually 
    */
    for (rqr_cb->rqr_end_of_data = FALSE;
	 col_count && rqr_cb->rqr_end_of_data == FALSE;
	 --col_count, ++bind)
    {
        convert_ok = TRUE;
	length = abs(bind->rqb_length);
	dt_id   = bind->rqb_dt_id;

	r_length = (bind->rqb_r_dt_id < 0) ?
		   bind->rqb_r_length - 1 : bind->rqb_r_length;
	length   = (bind->rqb_dt_id < 0)  ?
		   bind->rqb_length - 1 : bind->rqb_length;

	r_dt_id  = abs( bind->rqb_r_dt_id );
	dt_id    = abs( bind->rqb_dt_id );


	/* check to see if coercion is required */
	if (  ((length != r_length) || (dt_id != r_dt_id))
	    && (dt_id != DB_DBV_TYPE)
	   )
	{
	    if ((dt_id == DB_CHA_TYPE) || (dt_id == DB_CHR_TYPE) ||
	        (dt_id == DB_VCH_TYPE) || (dt_id == DB_TXT_TYPE) ||
		(dt_id == DB_BYTE_TYPE) || (dt_id == DB_VBYTE_TYPE) ||
		(dt_id == DB_LTXT_TYPE))
	    {
		fill_char = ' ';
		if ((r_dt_id == DB_BYTE_TYPE) || (r_dt_id == DB_VBYTE_TYPE))
		    fill_char = '\0';
		
		/* Blank fill destination */
		MEfill(length, fill_char, 
		       (PTR)((char *)bind->rqb_addr));

		convert_ok = FALSE;

		/* If source is a varchar, snarf the length */
		if (( r_dt_id == DB_VCH_TYPE ) || ( r_dt_id == DB_TXT_TYPE ) ||
		    (r_dt_id == DB_VBYTE_TYPE) || ( r_dt_id == DB_LTXT_TYPE ))
		{
		    if( rqr_cb->rqr_end_of_data == FALSE )
		        status = rqu_fetch_data(assoc, sizeof(len), (PTR)&len, 
					        (PTR)&rqr_cb->rqr_end_of_data,
						0, 0, 0);
		    r_length -= 2;
		    convert_ok = TRUE;
		}
		else if ((r_dt_id == DB_CHA_TYPE)  ||
			 (r_dt_id == DB_CHR_TYPE)  ||
			 (r_dt_id == DB_BYTE_TYPE) ||
			 (r_length <= length))
		{
		    len = r_length;
		    convert_ok = TRUE;
		}

		if (convert_ok == TRUE)
		{
		    PTR  addr = bind->rqb_addr;

		    if (( dt_id == DB_VCH_TYPE ) ||
			( dt_id == DB_TXT_TYPE ) ||
			( dt_id == DB_VBYTE_TYPE) ||
		        ( dt_id == DB_LTXT_TYPE ))
		    {
		        /* Fill in length part, and adjust length accordingly */
		        MEcopy((PTR)&len, sizeof(len), bind->rqb_addr);
		        length -= 2;
			addr   += 2;
		    }

		    if( rqr_cb->rqr_end_of_data == FALSE )
		    {
		        /* Truncate if source > destination */
			if (length < (i4)len)
			    len = length;

			status = rqu_fetch_data(assoc, len, addr,
						&rqr_cb->rqr_end_of_data,
						0, 0, 0);

			/* Skip the remaining space if bytes read < db_length */
		        if ((i4)len < r_length)
		        {
			    /* skip the remaining space, if truncated */
			    if( rqr_cb->rqr_end_of_data == FALSE )
				status = rqu_fetch_data(assoc, 
						    r_length - (i4)len,
						    NULL, 
						    &rqr_cb->rqr_end_of_data,
							0, 0, 0);
			}
		    }
		} /* convert_ok == TRUE */ 
	    } /* any string type */
	    else if ((dt_id == DB_INT_TYPE) || (dt_id == DB_FLT_TYPE) ||
                     (dt_id == DB_DEC_TYPE))
	    {
		i1  i_1 = 0;
		i2  i_2 = 0;
		i4  i_4 = 0;
		i8  i_8 = 0;

                if ((dt_id == DB_DEC_TYPE) && (r_dt_id == DB_DEC_TYPE))
                {
                    i2          pr = DB_P_DECODE_MACRO(bind->rqb_r_prec);
                    i2          sc = DB_S_DECODE_MACRO(bind->rqb_r_prec);
		    char	r_data[DB_MAX_DECLEN + DB_ALIGN_SZ + 1]; 

		    convert_ok = FALSE;
		    MEfill(sizeof(r_data), '\0', r_data);
		    
		    do 
		    {	/* Something to break out of */
			if(( status = rqu_fetch_data( assoc, r_length, 
						      &r_data[0],
						      &rqr_cb->rqr_end_of_data,
						     0, 0, 0)
			   ) != E_DB_OK )
			    break;

			if (rqr_cb->rqr_end_of_data)
			{
			    convert_ok = TRUE;
			    break;
			}

                        if (CVpkpk( (PTR)&r_data, pr, sc,
                             (i4)DB_P_DECODE_MACRO(bind->rqb_prec),
                             (i4)DB_S_DECODE_MACRO(bind->rqb_prec),
                             (PTR)bind->rqb_addr) != E_DB_OK )
                            break;

                        convert_ok = TRUE;

		    } while (0);
                }
		else if ((dt_id == DB_INT_TYPE) && (r_length == 8))
		{
		    if( rqr_cb->rqr_end_of_data == FALSE )
		        status = rqu_fetch_data(assoc, sizeof(i_8), (PTR)&i_8, 
						(PTR)&rqr_cb->rqr_end_of_data,
						0, 0, 0);
		    /* Zero fill destination */
		    MEfill ( length, '\0', bind->rqb_addr);
		    if ((length == 4) && ( i_8 <= MAXI4 && i_8 >= MINI4 ))
		        *((i4 *)bind->rqb_addr) = (i4)i_8;
		    else if ((length == 2) && ( i_8 <= MAXI2 && i_8 >= MINI2 ))
		        *((i2 *)bind->rqb_addr) = (i2)i_8;
		    else if ((length == 1) && ( i_8 <= MAXI1 && i_8 >= MINI1 ))
			*((i1 *)bind->rqb_addr) = (i1)i_8;
		    else
			convert_ok = FALSE;
		}
		else if ((dt_id == DB_INT_TYPE) && (r_length == 4))
		{
		    if( rqr_cb->rqr_end_of_data == FALSE )
		        status = rqu_fetch_data(assoc, sizeof(i_4), (PTR)&i_4, 
						(PTR)&rqr_cb->rqr_end_of_data,
						0, 0, 0);
		    /* Zero fill destination */
		    MEfill ( length, '\0', bind->rqb_addr);
		    if (length == 8)
		        *((i8 *)bind->rqb_addr) = (i8)i_4;
		    else if ((length == 2) && ( i_4 <= MAXI2 && i_4 >= MINI2 ))
		        *((i2 *)bind->rqb_addr) = (i2)i_4;
		    else if ((length == 1) && ( i_4 <= MAXI1 && i_4 >= MINI1 ))
			*((i1 *)bind->rqb_addr) = (i1)i_4;
		    else
			convert_ok = FALSE;
		}
		else if ((dt_id == DB_INT_TYPE) && (r_length == 2))
		{
		    if( rqr_cb->rqr_end_of_data == FALSE )
		        status = rqu_fetch_data(assoc, sizeof(i_2), (PTR)&i_2, 
						(PTR)&rqr_cb->rqr_end_of_data,
						0, 0, 0);
		    /* Zero fill destination */
		    MEfill ( length, '\0', bind->rqb_addr);
		    if (length == 8)
			    *((i8 *)bind->rqb_addr) = (i8)i_2;
		    else if (length == 4)
			    *((i4 *)bind->rqb_addr) = (i4)i_2;
		    else if ((length == 1) && ( i_2 <= MAXI1 && i_2 >= MINI1 ))
			    *((i1 *)bind->rqb_addr) = (i1)i_2;
		    else
			convert_ok = FALSE;
		}
		else if ((dt_id == DB_INT_TYPE) && (r_length == 1))
		{
		    if( rqr_cb->rqr_end_of_data == FALSE )
		        status = rqu_fetch_data(assoc, sizeof(i_1), 
					        &i_1, &rqr_cb->rqr_end_of_data,
						0, 0, 0);
		    /* Zero fill destination */
		    MEfill ( length, '\0', bind->rqb_addr);
		    *((i1 *)bind->rqb_addr) = i_1;
		}
		else if (((dt_id == DB_INT_TYPE) && (r_dt_id == DB_FLT_TYPE)) ||
			 ((dt_id == DB_FLT_TYPE) && (r_dt_id == DB_INT_TYPE)) ||
			 ((dt_id == DB_FLT_TYPE) && (r_dt_id == DB_FLT_TYPE)))
		{
		    SCF_CB	scf_cb;
		    SCF_SCI	sci_list[1];
		    ADF_CB	*adf_cb;
		    ADF_FN_BLK	adffn;
		    char	r_data[16];  /* Largest possible result 
						rounded up to power of 2 */

		    convert_ok = FALSE;
		    MEfill(sizeof(r_data), '\0', r_data);
		    
		    do 
		    {	/* Something to break out of */
			if(( status = rqu_fetch_data( assoc, r_length, 
						      &r_data[0],
						      &rqr_cb->rqr_end_of_data,
						     0, 0, 0)
			   ) != E_DB_OK )
			    break;
			if (rqr_cb->rqr_end_of_data)
			{
			    convert_ok = TRUE;
			    break;
			}
			scf_cb.scf_type = SCF_CB_TYPE;
			scf_cb.scf_length = sizeof(SCF_CB);
			scf_cb.scf_facility = DB_ADF_ID;
			scf_cb.scf_session  = DB_NOSESSION;
			scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *)sci_list;
			scf_cb.scf_len_union.scf_ilength = 1;
			sci_list[0].sci_length = sizeof(adf_cb);
			sci_list[0].sci_code = SCI_SCB;
			sci_list[0].sci_aresult = (char *) &adf_cb;
			sci_list[0].sci_rlength = NULL;
			
			/* Get adf_cb from SCF */
			if(( status = scf_call( SCU_INFORMATION, &scf_cb )
			   ) != E_DB_OK )
			    break;

			/* Get function instance id to do conversion */
			if (( status = adi_ficoerce( adf_cb, r_dt_id, 
						     bind->rqb_dt_id,
					             &adffn.adf_fi_id )
			    ) != E_DB_OK )
			    break;

			/* Get description of function instance from id */
			if(( status = adi_fidesc( adf_cb, adffn.adf_fi_id, 
						  &adffn.adf_fi_desc )
			   ) != E_DB_OK )
			    break;

			/* Fill in result db_data_value */
		        MEfill(bind->rqb_length, '\0', bind->rqb_addr);
			adffn.adf_r_dv.db_datatype = dt_id;
			adffn.adf_r_dv.db_length   = length;
			adffn.adf_r_dv.db_data     = bind->rqb_addr;
				    
			/* Fill in first operand db_data_value */
			adffn.adf_dv_n		   = 1;
			adffn.adf_1_dv.db_datatype = r_dt_id;
			adffn.adf_1_dv.db_length   = r_length;
			adffn.adf_1_dv.db_data     = r_data;

			adffn.adf_pat_flags = AD_PAT_NO_ESCAPE;

			/* Perform function instance to convert */
			if(( status = adf_func( adf_cb, &adffn )
			   ) != E_DB_OK )
			    break;

			convert_ok = TRUE;
		    } while (0);
		}
		else
		    convert_ok = FALSE;
	    } /* any integer type */
	}
	else
	{
	    /* Type and length are the same, or dt_id is a DB_DBV_TYPE */
	    if( rqr_cb->rqr_end_of_data == FALSE )
	        status = rqu_fetch_data(assoc, r_length, bind->rqb_addr,
			                &rqr_cb->rqr_end_of_data, 0,
					0, 0);
	    convert_ok = TRUE;
	}

	if ((rqr_cb->rqr_end_of_data == FALSE) &&
	    ((bind->rqb_dt_id < 0) || (bind->rqb_r_dt_id < 0)))
	{
	    char nullb = 0;
	    bool null1, null2;

	    null1 = (bind->rqb_dt_id < 0) ? TRUE : FALSE;
	    null2 = (bind->rqb_r_dt_id < 0) ? TRUE : FALSE;

	    if ((bind->rqb_r_dt_id < 0) && (rqr_cb->rqr_end_of_data == FALSE))
	    {
		/* Get the null indicator */
		status = rqu_fetch_data(assoc, sizeof(nullb),
				&nullb, &rqr_cb->rqr_end_of_data, 0,
					0, 0);
	    }

	    if ((null1 == TRUE) && (null2 == TRUE))
	    {
		/* Both nullable */
		bind->rqb_addr[bind->rqb_length-1] = nullb;
	    }
	    else if ((null1 == TRUE) && (null2 == FALSE))
	    {
		/* Dest nullable, src not, mark as NOT NULL */
		bind->rqb_addr[bind->rqb_length] = 0x0;
	    }
	    else if ((null1 == FALSE) && (null2 == TRUE))
	    {
		/* Src nullable, dest not, error if src IS NULL */
		if (nullb & 0x1)
		    convert_ok = FALSE;
	    }
	} /* if nullable */

	if (convert_ok == FALSE)
	{
	    RQTRACE(rqs,1)(RQFMT(rqs,
			    "RQF:Assn %d. Data conversion Error"),
			    assoc->rql_assoc_id);
	    RQTRACE(rqs,1)(RQFMT(rqs,
			    "\t Expected length %d, type %d\n"),
			    bind->rqb_length, bind->rqb_dt_id);
	    RQTRACE(rqs,1)(RQFMT(rqs,
			    "\t Returned length %d, type %d\n"),
			    bind->rqb_r_length, bind->rqb_r_dt_id);
	    (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
	    return(E_RQ0016_CONVERSION_ERROR);
	}
    } /* For each column */
    return(status);
}

/*
** {
** Name: rqf_nt_fetch	-  fetch n tuples (RQR_GET)
**
** Description:
**	fetch up to n tuples.
**	Because GCA tuple data message is not always alinged on tuple
**	boundary, the fetch logic handle tuple spanning GCA messages.
**
** Inputs:
**      rqr_cb	    pointer to RQF request control block
**        rqr_1_site          	database server id
**	  rqr_tupcount		number of tuples desired
**	  rqr_tupdata		QEF_DATA array
**
** Outputs:
**	rqr_cb
**	  rqr_tupcount		number of tuples fetched. Note that 0
**				implies rqr_end_of_data is TRUE.
**	  rqr_end_of_data	end of data indicator. Note that TRUE does
**				NOT imply rqr_tup_count is 0.
**	  rqr_error		error reported by RQF and GCA
**      
**	Returns:
**		E_RQ0006_CANNOT_GET_ASSOCIATION
**		E_RQ0015_UNEXPECTED_MESSAGE
**		E_RQ0016_CONVERSION_ERROR
**		E_RQ0001_OK
**      	E_RQ0002_NO_TUPLE_DESCRIPTION
**		E_RQ0001_TOO_FEW_COLUMNS
**		E_RQ0004_BIND_BUFFER_TOO_SMALL
**		E_RQ0003_TOO_MANY_COLUMNS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11_Oct-1988 (alexh)
**          created
**      02-aug-1989 (georgeg)
**          Added support for HETNET, kepp track of the tuple descriptor.
**      14-Oct-1993 (fred)
**          Added large object support.
*/
DB_ERRTYPE
rqf_nt_fetch(
	RQR_CB	    *rqr_cb)
{
    RQL_ASSOC	    *assoc = rqu_check_assoc(rqr_cb, 0);
    DB_ERRTYPE	    status = E_RQ0000_OK;
    QEF_DATA	    *qef_data = (QEF_DATA *)rqr_cb->rqr_tupdata;
    i4		    tup_asked = rqr_cb->rqr_tupcount;
    i4              amount_returned;
    i4              eo_tuple = TRUE;

    if (assoc == NULL)
    {
	return(E_RQ0006_CANNOT_GET_ASSOCIATION);
    }
  
    /*
    ** make sure tuple description has been taken care of 
    */
    if (assoc->rql_described == FALSE)
    {
        /*
        ** If the TDSCR has not been delivered yet get it and 
        ** return to QEF that there are more data comming but
        ** 0 rows are returned, this will forse the sequencer to 
        ** return for more rows. The reasone for all this is that in HETNET
        ** the local can return different tuple size than STAR's tuple size. 
        ** We also assume that that number of collums is the same.
        */
        if (assoc->rql_rv.gca_message_type != GCA_TDESCR)
        {
            /*
            ** has to be a GCA_TDESCR message 
            */
            (VOID)rqu_interrupt( assoc, (i4)GCA_INTALL);
            return(E_RQ0002_NO_TUPLE_DESCRIPTION);
        }

        /*
        ** Finaly we can get the tuple descriptor
        */
        status = rqf_td_get(rqr_cb, assoc);
        if ( status != E_DB_OK )
        {
            return( status );
        }
        
        rqr_cb->rqr_tupcount = -1;
        rqr_cb->rqr_end_of_data = FALSE;

        assoc->rql_described = TRUE;
        return( status );
    }

    rqr_cb->rqr_tupcount = 0;
    rqr_cb->rqr_count = 0;
    rqr_cb->rqr_end_of_data = FALSE;

    /*
    ** fetch tuples until full or end of tuples 
    */
    while (rqr_cb->rqr_tupcount < tup_asked &&
           rqr_cb->rqr_end_of_data == FALSE &&
	   qef_data != NULL)
    {
	status = rqu_fetch_data(assoc,
				qef_data->dt_size,
				qef_data->dt_data,
				&rqr_cb->rqr_end_of_data,
				1, /* Asking for whole tuple */
				&amount_returned,
				&eo_tuple);
	if (rqr_cb->rqr_end_of_data == FALSE)
	{
	    ++rqr_cb->rqr_count;
	    if (eo_tuple)
		++rqr_cb->rqr_tupcount;
	    qef_data->dt_size = amount_returned;
	    qef_data = qef_data->dt_next;
	}
    }
    return(status);
}
/*{
** Name: rqf_endret	- flush retrieve result (RQR_ENDRET)
**
** Description:
**      send an interrupt message to the specified database server.
**
** Inputs:
**      rqr_cb	    	pointer to RQF request control block
**        rqr_1_site    database server id
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_RQ0017_NO_ACK
**      
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-Oct-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqf_endret(
	RQR_CB	    *rqr_cb)
{
    return(rqf_interrupt(rqr_cb, (i4)GCA_ENDRET));
}

/*{
** Name: rqf_transfer	- transfer a query result to another site.
**			  (RQR_XFR)
** Description:
**	retrieve the data of a transfer query and copy the data into
**	another site. Both the transfer query and the copy into query
**	must be supplied. The destination table is created according to
**	the tuple description of the transfer query.
** Inputs:
**      rqr_cb->	      	pointer to RQF request control block
**		rqr_xfr		table transfer info (QEQ_D3_XFR *)
**		rqr_tmp		name table ( DD_NAME *) This contains
**				the target column names.
**
** Outputs:
**      rqr_cb->
**        
**	Returns:
**	    	E_RQ0000_OK
**		E_RQ0002_NO_TUPLE_DESCRIPTION
**		E_RQ0023_COPY_FROM_EXPECTED
**		E_RQ0024_COPY_DEST_FAILED
**		E_RQ0025_COPY_SOURCE_FAILED
**		E_RQ0034_COPY_CREATE_FAILED
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jul-1988 (alexh)
**          created
**	24-oct-1988 (alexh)
**	    rewrite for the new RQR_XFR interface
**      27-mar-1990 (georgeg)
**	    Tell QEF that the tmp table has been created.
**      02-apr-1990 (carl)
**	    changed to call rqu_cp_get if BYTE_ALIGN is defined or rqf-cp_get 
**	    otherwise
**	24-mar-1990 (carl)
**	    changed to call rqu_cp_get instead of rqf_cp_get 
**	11-jun-1990 (georgeg)
**	    NULL terminate the transfer table name string before it gets passed to 
**	    STprintf.
**	    Fixes B36746.
**	01-may-1991 (fpang)
**	    In rqf_transfer(), td_data may not be aligned after coming
**	    back from rqu_cp_get(). Use MEcopy() rather than assignment.
**	15-oct-1992 (fpang)
**	    Replace sc0m_rqf_allocate() calls with ULM calls.
**	    Eliminated tmp_mem because ULM calls eliminates its purpose. 
**	    Rqf_cp_get() can handle aligned and unaligned data, so eliminated
**	    call to rqu_cp_get() when unaligned.
**	    Support GCA1_C_FROM/INTO message types.
**	18-nov-1992 (fpang)
**	    Changed copy table name to 'IISTAR'.
**       7-Oct-1993 (fred)
**          Changed to terminate 'tmp_table' correctly -- at
**          DB_MAXNAME -1 rather than DB_MAXANME.
**      06-mar-1996 (nanpr01)
**          Added pagesize parameter in the rqu_create_tmp call. 
*/
DB_ERRTYPE
rqf_transfer(
	RQR_CB	    *src_cb)
{
    RQL_ASSOC	    *src_assoc;
    RQL_ASSOC	    *dest_assoc;
    QEQ_D3_XFR	    *qeq_xfr	= (QEQ_D3_XFR *)src_cb->rqr_xfr;
    QEQ_D1_QRY	    *src_q	= &qeq_xfr->qeq_x1_srce;
    QEQ_D1_QRY	    *copy_q	= &qeq_xfr->qeq_x3_sink;
    QEQ_D1_QRY	    *create_q	= &qeq_xfr->qeq_x2_temp;
    DB_ERRTYPE	    status;
    GCA_AK_DATA	    *ack;
    DD_NAME	    *table_name;
    RQR_CB	    rqr_block;
    RQR_CB	    *dest_cb	= &rqr_block;
    i4		    reply, reply_type;
    GCA_TD_DATA     *td_data	= (GCA_TD_DATA *)NULL;
    DD_NAME	    tmp_table;
    RQS_CB	    *rqs = (RQS_CB *)src_cb->rqr_session;
    ULM_RCB 	    ulm;


    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Rqf_facility->rqf_pool_id;
    ulm.ulm_blocksize = sizeof(i4);

    src_cb->rqr_crttmp_ok = FALSE; /* the tmp table has not been created yet */
    if (!(src_cb->rqr_q_language == DB_SQL || src_cb->rqr_q_language == DB_QUEL))
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:rqf_transfer. Bad language type %d\n"), 
		    src_cb->rqr_q_language);
	/*
	** QEF is setting to DB_NOLANG 
	*/
	src_cb->rqr_q_language = DB_SQL;
    }

    /*
    ** set up site ids 
    */
    src_cb->rqr_1_site = src_q->qeq_q5_ldb_p;

    /*
    ** execute tuple source query 
    */
    STRUCT_ASSIGN_MACRO(*src_q->qeq_q4_seg_p->qeq_s2_pkt_p, src_cb->rqr_msg);
    status = rqf_query(src_cb);
    if (status != E_RQ0000_OK)
    {
	return(status);
    }
    /*
    ** save the source association value for later references 
    */
    src_assoc = ((RQS_CB *)src_cb->rqr_session)->rqs_current_site;

    /*
    ** make sure the transfer query returns tuple description 
    */
    if (src_assoc->rql_rv.gca_message_type != GCA_TDESCR)
    {
	/*
	** has to be a GCA_TDESCR message 
	*/
	(VOID)rqu_interrupt( src_assoc, (i4)GCA_INTALL);
	return(E_RQ0002_NO_TUPLE_DESCRIPTION);
    }
    /*
    ** create the temproary transfer table create query and
    ** execute the create statement synthesized from tuple description 
    */

    /*
    ** need two RQF contexts to do the trasnfer. Clone one. 
    */
    MEcopy((PTR)src_cb, sizeof(RQR_CB), (PTR)dest_cb);
    dest_cb->rqr_1_site = copy_q->qeq_q5_ldb_p;

    /*
    ** make sure there is an destination association 
    */
    if ((dest_assoc = rqu_check_assoc(dest_cb, 1)) == (RQL_ASSOC *)NULL)
    {
	(VOID)rqu_interrupt( src_assoc, (i4)GCA_INTALL);
	return(E_RQ0034_COPY_CREATE_FAILED);
    }
  
    {
	/*
	** see QEF interface. Don't ask why. 
	*/
	QEQ_TXT_SEG	*text_segs = create_q->qeq_q4_seg_p;

	(dest_assoc->rql_nostatments)++;
	table_name = dest_cb->rqr_tmp + 
		    text_segs->qeq_s2_pkt_p->dd_p3_nxt_p->dd_p4_slot;

	MEcopy((PTR)table_name, sizeof(tmp_table), tmp_table); 
	/*
	** QEF produces a table name from a timestamp, it is 
	** always less that DB_MAXNAME, we can safely terminate.
	*/
	tmp_table[DB_MAXNAME - 1] = EOS;
	status = rqu_create_tmp(src_assoc, dest_assoc,
				dest_cb->rqr_q_language,
				create_q->qeq_q7_col_pp,
				(DD_NAME *)tmp_table,
				create_q->qeq_q14_page_size);
    }

    if (status != E_RQ0000_OK)
    {
	(VOID)rqu_interrupt( src_assoc, (i4)GCA_INTALL);
	(VOID)rqu_interrupt( dest_assoc, (i4)GCA_INTALL);
	return(status);
    }
    /*
    ** get the reply of the create table query 
    */
    if (rqu_get_reply(dest_assoc, -1) == GCA_ERROR)
    {
	(VOID)rqu_interrupt( src_assoc, (i4)GCA_INTALL);
	(VOID)rqu_interrupt( src_assoc, (i4)GCA_INTALL);
	return(E_RQ0034_COPY_CREATE_FAILED);
    }

    src_cb->rqr_crttmp_ok = TRUE; /* the tmp table has been created */
    /*
    ** execute destination "copy from" query 
    */
    {
	char	copy_qry[100 + sizeof(DD_NAME)];
	char	format[80];

	STprintf(format,"copy table %%.%ds () from 'IISTAR';", sizeof(DD_NAME));
	STprintf(copy_qry, format, tmp_table);
	dest_cb->rqr_dv_cnt = 0;
	dest_cb->rqr_msg.dd_p1_len = (i4)STlength(copy_qry);
	dest_cb->rqr_msg.dd_p2_pkt_p = copy_qry;
	dest_cb->rqr_msg.dd_p3_nxt_p = NULL;
	dest_assoc->rql_nostatments++;
	status = rqf_query(dest_cb);
    }
    if (status != E_RQ0000_OK)
    {	
	return(status);
    }

    /* 
    ** Extract the TD_DATA from the assoc and save it. 
    ** Until RQF gets its own memory we will keep the TD_DESC in
    ** SCF's memory. This needs to be fixed soon. 
    */
    if ((dest_assoc->rql_rv.gca_message_type != GCA_C_FROM) &&
        (dest_assoc->rql_rv.gca_message_type != GCA1_C_FROM))
    {
        (VOID)rqu_interrupt(dest_assoc, GCA_INTALL);
        (VOID)rqu_interrupt(src_assoc, GCA_INTALL);
        return(E_RQ0023_COPY_FROM_EXPECTED);
    }
    else
    {

	reply_type = dest_assoc->rql_rv.gca_message_type;

        status = rqf_cp_get( dest_assoc, (PTR *)&td_data );

        if ( status != E_RQ0000_OK )
        {
            (VOID)rqu_interrupt(src_assoc, GCA_INTALL);
            (VOID)rqu_interrupt(dest_assoc, GCA_INTALL);
            if (rqs->rqs_cpstream != (PTR) NULL )
            {
		ulm.ulm_streamid_p = &rqs->rqs_cpstream;
		ulm.ulm_memleft = &Rqf_facility->rqf_ulm_memleft;
                if (ulm_closestream(&ulm) != E_DB_OK)
                {
                    RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:rqf_transfer. Stream closure failed\n"));
                    return(E_DB_ERROR);
                }
		/* ULM has nullified rqs_cpstream */
            }
            return(E_RQ0024_COPY_DEST_FAILED);
        }       

    }

    /*
    ** a protocol acknowledgement 
    */
    ack = (GCA_AK_DATA *)dest_assoc->rql_sd.gca_buffer;
    ack->gca_ak_data = 0;	/* got a better idea? */
    (VOID)rqu_send(dest_assoc, (i4)sizeof(GCA_AK_DATA), TRUE, GCA_ACK);

    /*
    ** link the receive and the send buffer
    ** the send buffer must be restored back to rql_send_buffer 
    */
    dest_assoc->rql_sd.gca_buffer = src_assoc->rql_rv.gca_buffer;

    status = E_RQ0000_OK;
    /*
    ** transfer GCA_TUPLES message from source site to the copy site 
    */

    /*
    ** td_data may not be aligned, so MEcopy it
    */

    MEcopy((PTR)&dest_assoc->rql_nostatments,
	   sizeof(dest_assoc->rql_nostatments),
           (PTR)&td_data->gca_id_tdscr);
    dest_assoc->rql_sd.gca_descriptor = (PTR)td_data;
    while (status == E_RQ0000_OK && 
	    rqu_get_reply(src_assoc, -1) == GCA_TUPLES)
    {
	/*
	** a tuple message successfully read from source. send it to copy site 
	*/
	status = rqu_send(dest_assoc, 
			  src_assoc->rql_rv.gca_d_length,
			  src_assoc->rql_rv.gca_end_of_data,
			  GCA_CDATA);
	/* 
	** see if the server has reported error.
	** Continue to send in this case may cause synchronous blocking
	**Interrupt must be handle here as well.
	*/
	reply = rqu_get_reply(dest_assoc, 0);
	if (! (( reply == GCA_MAXMSGTYPE+1) || /* time out */
               ( reply == reply_type )) )
	    status = E_RQ0024_COPY_DEST_FAILED;
    }

    if (rqs->rqs_cpstream != (PTR) NULL )
    {
	ulm.ulm_streamid_p = &rqs->rqs_cpstream;
	ulm.ulm_memleft = &Rqf_facility->rqf_ulm_memleft;
        if (ulm_closestream(&ulm) != E_DB_OK)
        {
            RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqf_transfer. Stream closure failed\n"));
            return(E_DB_ERROR);
        }
	/* ULM has nullified rqs_cpstream */
    }

    /*
    ** if destination server crapped out 
    */
    if (status != E_RQ0000_OK)
    {
	(VOID)rqu_interrupt( src_assoc, (i4)GCA_INTALL);
	(VOID)rqu_interrupt(dest_assoc, GCA_INTALL);
	status = E_RQ0024_COPY_DEST_FAILED;
    }
    /*
    ** if source site gave up then back out all the copy 
    */
    else if (src_assoc->rql_rv.gca_message_type != GCA_RESPONSE )
    {
	status = E_RQ0025_COPY_SOURCE_FAILED;
    }
    /*
    ** else everything is ok. give the server a response 
    */
    else
    {
	status = rqu_send(dest_assoc,
			  src_assoc->rql_rv.gca_d_length,
			  src_assoc->rql_rv.gca_end_of_data,
			  GCA_RESPONSE);
	/* 
	** this undocument GCA_RESPONSE is expecteded.
	*/
	if (rqu_get_reply(dest_assoc, -1) != GCA_RESPONSE)
	{
	    status = E_RQ0015_UNEXPECTED_MESSAGE;
	}
    }

    /*
    ** restore the association's regular write buffer 
    */
    dest_assoc->rql_sd.gca_buffer = dest_assoc->rql_write_buff;
    if (status != E_RQ0000_OK)
    {
	(VOID)rqu_interrupt(src_assoc, (i4)GCA_ENDRET);
	(VOID)rqu_interrupt(dest_assoc, (i4)GCA_COPY_ERROR);
    }
    return(status);
}

/*{
** Name: rqf_cp_get     - get the tuple descriptor from the association
**                          input sream from the GCA_CP_MAP.
** Description:
**
** Inputs:
**      assoc->         pointer to the associaton 
**      td_data         address to return a poinet to the tuple descriptor.
**
** Outputs:
**        
**      Returns:
**              E_RQ0000_OK
**              E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      16-may-1989 (georgeg)
**          created
**	15-oct-1992 (fpang)
**	    Replace sc0m_rqf_allocate() calls with ULM calls.
**	    Eliminated tmp_mem parameter because ULM calls eliminates its 
**	    purpose.
**	    Compute address of tuple descriptor in a portable way, this
**	    eliminates need for rqu_cp_get() function.   
*/
DB_ERRTYPE
rqf_cp_get(
	RQL_ASSOC       *assoc,
	PTR             *td_data)
{
    GCA_RV_PARMS    *rv;
    i4         size;
    PTR             ptr1, ptr2;
    DB_STATUS       alloc_status, status = E_DB_OK;
    RQS_CB          *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;
    ULM_RCB	    ulm;
    PTR		    strm1 = NULL, strm2 = NULL;
    i4		    msg_type;

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Rqf_facility->rqf_pool_id;
    ulm.ulm_blocksize = sizeof(i4);
    ulm.ulm_streamid_p = &ulm.ulm_streamid;

    rv = (GCA_RV_PARMS  *)&assoc->rql_rv;
    ptr1 = ptr2 = (PTR)NULL;
    size = 0;
    msg_type = rv->gca_message_type;

    do
    {
	ulm.ulm_memleft = &Rqf_facility->rqf_ulm_memleft;
	/* Get a private, thread-safe stream */
	/* Open stream and allocate memory with one effort */
	ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    	ulm.ulm_psize = size + rv->gca_d_length;

    	if (ulm_openstream(&ulm) != E_DB_OK)
    	{
            RQTRACE(rqs,1)(RQFMT(rqs,
				"RQF:rqf_cp_get. Stream open failed\n"));
            return(E_DB_ERROR);
    	}
    	strm2 = ulm.ulm_streamid;
	ptr2 = ulm.ulm_pptr;

	if (ptr1 != (PTR) NULL)
	{
            MEcopy( (PTR)ptr1, size, (PTR)ptr2 );
	    ulm.ulm_streamid = strm1;
	    ulm.ulm_memleft  = &Rqf_facility->rqf_ulm_memleft;
	    if (ulm_closestream(&ulm) != E_DB_OK)
	    {
		RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:rqf_cp_get. Stream closure failed\n"));
	        return(E_DB_ERROR);
	    }
	}

        ptr1 = ptr2;
	strm1 = strm2;

	/*
	** now copy the new data in
	*/
	ptr2 = ptr1 + size;
	MEcopy( (PTR)rv->gca_data_area, rv->gca_d_length, (PTR)ptr2);
	size += rv->gca_d_length;
    } while (rv->gca_end_of_data == FALSE && 
             (rqu_get_reply(assoc, -1) == msg_type) );

    if (rv->gca_message_type != msg_type)
    {
        if (ptr1 != (PTR)NULL)
        {
            ulm.ulm_streamid = strm1;
            ulm.ulm_memleft  = &Rqf_facility->rqf_ulm_memleft;
            if (ulm_closestream(&ulm) != E_DB_OK)
            {
                RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:rqf_cp_get. Stream closure failed\n"));
                return(E_DB_ERROR);
            }
        }
        return(E_DB_ERROR);
    }

    rqs->rqs_cpstream = strm1;

    /*
    ** We got the CP_MAP at hand, extract the TD_DESC
    */

    /* Point ptr1 at gca_l_fname_cp */
    ptr1 += sizeof(i4) +		    /* gca_status_cp */
	    sizeof(i4) +		    /* gca_direction_cp */
	    sizeof(i4);		    /* gca_maxerrs_cp */

    /* Copy out gca_l_fname_cp, and increment over it and gca_fname_cp */
    MEcopy(ptr1, sizeof(i4), (PTR)&size);    /* gca_l_fname_cp */
    ptr1 += sizeof(i4) + size;		    /* gca_fname_cp[] */

    /* Copy out gca_l_logname_cp, and increment over it and gca_logname_cp */
    MEcopy(ptr1, sizeof(i4), (PTR)&size);    /* gca_l_logname_cp */
    ptr1 += sizeof(i4) + size;		    /* gca_logname_cp[] */

    /* Should be pointing at gca_tup_desc_cp now */

    *td_data = ptr1;
    return(E_DB_OK);    
}

/*{
** Name: rqf_write	- write to database server
**
** Description:
**      send a fromatted GCA message to a LDB server.
**
** Inputs:
**      rqr_cb->
**        rpr_msg.dd_p2_pkt_p    message data buffer to be written
**			      This message must be a GCA_SD_PARMS
**			      struct.
**
** Outputs:
** 	None
**        
**	Returns:
**	    E_RQ0012_INVALID_WRITE
**	    E_RQ0000_OK
**
** Side Effects:
**	    none
**
** History:
**      26-jul-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqf_write(
	RQR_CB	    *rcb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc(rcb, 1);

  if (assoc == NULL)
    return(rcb->rqr_error.err_code);

  if (rqu_xsend(assoc, (GCA_SD_PARMS *)rcb->rqr_msg.dd_p2_pkt_p) != 
      E_GC0000_OK)
    return(E_RQ0012_INVALID_WRITE);

  return(E_RQ0000_OK);
}

/*{
** Name: rqf_connect - make sure a direct connection to desired LDB exists.
**
** Description:
**      If an association does not already exist, a new one will be
**	created.
**
** Inputs:
**      rqr_cb->
**	  rqr_1_site	site description
**
** Outputs:
** 	None
**        
**	Returns:
**	    E_RQ0006_CANNOT_GET_ASSOCIATION
**	    E_RQ0000_OK
**
** Side Effects:
**	    none
**
** History:
**      26-jul-1988 (alexh)
**          created
**	18-jan-1993 (fpang)
**	    Pass back LDB gca protocol in SCC_SDC_CB.sdc_peer_protocol for 
**	    direct connect and direct execute immediate.
*/
DB_ERRTYPE
rqf_connect(
	RQR_CB	    *rcb)
{

    RQL_ASSOC	*assoc;
    RQS_CB	*rqs = (RQS_CB *)rcb->rqr_session;
    SCC_SDC_CB	*dc  = (SCC_SDC_CB *)rcb->rqr_dc_blkp;

    if ((assoc = rqu_check_assoc(rcb, 1)) == NULL)
    {
	return(E_RQ0006_CANNOT_GET_ASSOCIATION);
    }

    if (dc)
	dc->sdc_peer_protocol = assoc->rql_peer_protocol;

    RQTRACE(rqs,1)(RQFMT(rqs,
	"RQF:Assn %d. Direct connect\n"), assoc->rql_assoc_id);

  return(E_RQ0000_OK);
}

/*{
** Name: rqf_disconnect - logically terminates a connection
**
** Description:
**      if the direction connect is under the transaction then do
**	nothing, else the transaction is ended. Since what transpired
**	during direct connnect is not known, both type of default 
**	transation ending should be tried - abort for QUEL and commit for
**	SQL.
**
**
** Inputs:
**      rqr_cb->
**	  rqr_dc_txn		direct connect transaction mode
**
** Outputs:
** 	None
**        
**	Returns:
**	    E_RQ0019_COMMIT_FAILED
**	    E_RQ0000_OK
**
** Side Effects:
**	    none
**
** History:
**      26-jul-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqf_disconnect(
	RQR_CB	    *rcb)
{

    RQL_ASSOC	*assoc = rqu_check_assoc(rcb, 0);
    RQS_CB	*rqs = (RQS_CB *)rcb->rqr_session;

    RQTRACE(rqs,1)(RQFMT(rqs,
	"RQF:Assn %d. Direct disconnect\n"), assoc->rql_assoc_id);

    /* this is the direct connect case,open transaction must be ended */

    if (rcb->rqr_dc_txn == FALSE)
    {
	DB_LANG	q_lang_save = rcb->rqr_q_language;
	DB_ERRTYPE status;

	/* if no language type specified then try SQL first */
	if (rcb->rqr_q_language == DB_NOLANG)
	{	
	    rcb->rqr_q_language = DB_SQL;
	}

	/* first try to end a transaction */
	if (rcb->rqr_q_language == DB_SQL)
	{
	    status = rqf_commit(rcb);
	}
	else
	{
	    status = rqf_abort(rcb);
	}

	if ( status != E_RQ0000_OK)
	{
	    /* try another language mode if the initial try failed */
	    if (rcb->rqr_q_language == DB_SQL)
		rcb->rqr_q_language = DB_QUEL;
	    else
		rcb->rqr_q_language = DB_SQL;

	    /* try again */
	    if (rcb->rqr_q_language == DB_SQL)
		status = rqf_commit(rcb);
	    else
		status = rqf_abort(rcb);
	}

	/* restore the language mode */
	rcb->rqr_q_language = q_lang_save;
	return(status);
    }
    else
    {
	return(E_RQ0000_OK);
    }
}

/*{
** Name: rqf_check_interrupt - check and process interrupt
**
** Description:
**      This routine is called to when an abnormal completion of an RQF
**	call has occured. If interrupt has occure, by STAR client, then
**	interrupt is sent to current association.
**
**
** Inputs:
**      rcb		current request control block
**
** Outputs:
** 	rcb.rqr_error.err_code	E_RQ0039_INTERRUPTED if interrupt occured
**				while GCA request is in progress.
**        
**	Returns:
**	    E_RQ0000_OK
**
** Side Effects:
**	    none
**
** History:
**      5-Dec-1988 (alexh)
**          created
*/
VOID	
rqf_check_interrupt(
	RQR_CB	*rcb)
{
    RQL_ASSOC	*assoc = ((RQS_CB *)rcb->rqr_session)->rqs_current_site;
    DB_ERRTYPE	save;
    RQS_CB	*rqs = (RQS_CB *)rcb->rqr_session;

    if (assoc == NULL)
	return;

    assoc->rql_rqr_cb = rcb;
    if (assoc->rql_interrupted == TRUE)
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. Interrupted.\n"),assoc->rql_assoc_id);
	assoc->rql_interrupted = FALSE;
	rcb->rqr_error.err_code = E_RQ0039_INTERRUPTED;
    }
    
    /*
    ** else check to see if interrupt cleanup is necessary 
    */
    switch (save = rcb->rqr_error.err_code)
    {
	/*
	** no message sequence active 
	*/
	case E_RQ0006_CANNOT_GET_ASSOCIATION:
	case E_RQ0017_NO_ACK:
	case E_RQ0018_SHUTDOWN_FAILED:
	case E_RQ0008_SCU_MALLOC_FAILED:
	case E_RQ0010_ULM_OPEN_FAILED:
	case E_RQ0013_ULM_CLOSE_FAILED:
	case E_RQ0009_ULM_STARTUP_FAILED:
	case E_RQ0034_COPY_CREATE_FAILED:
	    /*
	    ** already cleaned up 
	    */
	case E_RQ0023_COPY_FROM_EXPECTED:
	case E_RQ0024_COPY_DEST_FAILED:
	case E_RQ0025_COPY_SOURCE_FAILED:
	    break;
	default:
	    /*
	    ** by default, interrupt will be sent to cleanup 
	    */
	    (VOID)rqu_interrupt(assoc,(i4)GCA_INTALL);
	    /*
	    ** restore the original error code 
	    */
	    rcb->rqr_error.err_code = save;
	break;
    }
}

/*{
** Name: rqf_set_options- send the set options query to
**			    all LDB associations of a session.
**
** Description:
**      This routine is called by QEF to when the client issues a set
**	option query. The query is send to all associations, it is
**	also saved in the context of the session for future
**	associations. The set function is only send to INGRES LDB's.
**
**
** Inputs:
**      rcb		current request control block
**
** Outputs:
**        
**	Returns:
**	    E_RQ0000_OK always.
**
** Side Effects:
**	    none
**
** History:
**      02-feb-1989 (georgeg)
**          created
**	15-oct-1992 (fpang)
**	    Elimanted sc0m_rqf_allocate() call. Call ULM instead.
**	08-dec-1992 (fpang)
**	    Get session context from rcb, not assoc. The latter points to 
**	    the assoc list, and the session pointer may not always be current.
**	06-oct-1993 (barbara)
**	    Fixed bug 54469.  To propagate SET statements to all existing
**	    associations, Star loops through list of associations (RQL_ASSOC);
**	    however it was not updating the stale RQR_CB pointer in each
**	    RQL_ASSOC and this caused an AV in functions called by
**	    rqu_set_option(). 
*/
DB_ERRTYPE
rqf_set_option(
	RQR_CB	*rcb)
{
    RQL_ASSOC	*assoc = ((RQS_CB *)rcb->rqr_session)->rqs_assoc_list;
    DD_PACKET	*packet_ptr;
    DB_LANG	lang = ((RQS_CB *)rcb->rqr_session)->rqs_lang;
    DB_ERRTYPE	status;  
    PTR		place = NULL;
    RQS_CB	*rqs = (RQS_CB *)rcb->rqr_session;
    ULM_RCB	ulm;

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Rqf_facility->rqf_pool_id;
    ulm.ulm_blocksize = sizeof(i4);
    ulm.ulm_memleft = &Rqf_facility->rqf_ulm_memleft;

    if (rcb->rqr_msg.dd_p2_pkt_p == NULL || assoc == NULL)
    {
	return(E_DB_ERROR);
    }

    /*
    ** send it to all active associations.
    */
    while (assoc != NULL )
    {
	/*
	** Update stale pointers by resetting pointer to caller's RQR_CB in
	** each assoc.
	*/ 
        assoc->rql_rqr_cb = rcb;
	/*
	** if it is an INGRES LBDMS send the query else move to the next.
	*/
#define	INGRES_STR  "INGRES"
	if ( MEcmp( assoc->rql_lid.dd_l4_dbms_name,
		INGRES_STR, 
		STlength(INGRES_STR) ) == 0 ) 

	{
	    status = rqu_set_option( assoc, (i4)rcb->rqr_msg.dd_p1_len,
				(char *)rcb->rqr_msg.dd_p2_pkt_p, lang );
	    if ( status != E_RQ0000_OK )
	    {
		return( E_RQ0014_QUERY_ERROR  );
	    }
	}
	assoc = assoc->rql_next;

    } /* end while */

    /*
    ** save it in the context of the session.
    */

    /* Tell ULM where to place and/or expect the streamid */
    ulm.ulm_streamid_p = &rqs->rqs_setstream;
    ulm.ulm_psize = sizeof(DD_PACKET) + rcb->rqr_msg.dd_p1_len + 2;
    
    if (rqs->rqs_setstream == NULL)
    {
	/* Get a stream that's private to this session */
	/* Open stream and allocate memory in one call */
	ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	if ((status = ulm_openstream(&ulm)) != E_DB_OK)
	{
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqf_set_option. Stream open failed\n"));
	    rqu_error(0, E_RQ0038_ULM_ALLOC_FAILED, (PTR)NULL, 
			(i4)1,
			sizeof(status), &status,
			(i4)0, (PTR)NULL, 
			(i4)0, (PTR)NULL,
			(i4)0, (PTR)NULL );
	    return(E_DB_ERROR);
	}
    }
    else
    {
	if ((status = ulm_palloc(&ulm)) != E_DB_OK)
	{
	    RQTRACE(rqs,1)(RQFMT(rqs,
				    "RQF:rqf_set_option. Stream open failed\n"));
	    rqu_error(0, E_RQ0038_ULM_ALLOC_FAILED, (PTR)NULL, 
			    (i4)1,
			    sizeof(status), &status,
			    (i4)0, (PTR)NULL, 
			    (i4)0, (PTR)NULL,
			    (i4)0, (PTR)NULL );
	    return(E_DB_ERROR);
	}
    }

    packet_ptr = (DD_PACKET *)ulm.ulm_pptr;
    place = ulm.ulm_pptr + sizeof(DD_PACKET);

    STRUCT_ASSIGN_MACRO( rcb->rqr_msg, *packet_ptr ); 
    /*
    ** copy in the set option query
    */
    STncpy( place, rcb->rqr_msg.dd_p2_pkt_p, rcb->rqr_msg.dd_p1_len );
    place[ rcb->rqr_msg.dd_p1_len ] = '\0';
    /* 
    ** link in with the rest of the "set option" session queries
    */
    packet_ptr->dd_p2_pkt_p = (char *)place;
    packet_ptr->dd_p3_nxt_p = ((RQS_CB *)rcb->rqr_session)->rqs_set_options;
    ((RQS_CB *)rcb->rqr_session)->rqs_set_options = packet_ptr;
    
    return( E_RQ0000_OK );
}

/*{
** Name: rqf_modify - set correct ingres priviledge for the user CDB association.
**
** Description:
**	set CCA_SUPSYS on or off for the user CDB association. Currently
**	we have two associations to the CBD, one of them is used by QEF to
**	bypass CBD access restrictions on CREATE LINK etc. and one to
**	to be used primarily by the FE to access catalog data. Hopefully this
**	mess will be changed soon.
**
** Inputs:
**      rcb->rqr_session->rqs_mod_flags.
**
** Outputs:
**
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_RQ0006_CANNOT_GET_ASSOC.
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      03-Mar-1989 (georgeg)
**          created
*/
DB_ERRTYPE
rqf_modify(
	RQR_CB		*rcb)
{
    RQL_ASSOC		*assoc;
    i4		i4_buff;
    GCA_SESSION_PARMS	*parms;
    RQS_CB		*session = (RQS_CB *)rcb->rqr_session;
    DD_COM_FLAGS	*flags = (DD_COM_FLAGS *)&session->rqs_com_flags;
    RQS_CB		*rqs = (RQS_CB *)rcb->rqr_session;

    assoc =  (RQL_ASSOC *)session->rqs_user_cdb;

    if ( flags->dd_co29_supsys == -1 )
    {
	flags->dd_co0_active_flags++;
    }
    flags->dd_co29_supsys = 1;
    flags->dd_co29_supsys = rcb->rqr_mod_flags->dd_m2_y_flag;
 
    if ( (RQL_ASSOC *)session->rqs_user_cdb == NULL )
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqf_modify. MODIFY_ASSN. User CDB assoc not found.\n"));
	return( E_RQ0000_OK );
    }


    assoc->rql_rqr_cb = rcb;
    parms = (GCA_SESSION_PARMS *)assoc->rql_sd.gca_buffer;
    parms->gca_l_user_data = 1;
    rqu_putinit(assoc, GCA_MD_ASSOC);
    (VOID)rqu_put_data(assoc, (PTR)&parms->gca_l_user_data, 
			sizeof(parms->gca_l_user_data));
    session->rqs_mod_flags.dd_m2_y_flag = rcb->rqr_mod_flags->dd_m2_y_flag;	

    i4_buff = 
	(session->rqs_mod_flags.dd_m2_y_flag == DD_1MOD_ON) ? GCA_ON : GCA_OFF;
    rqu_put_moddata(assoc, GCA_SUPSYS, DB_INT_TYPE, 0, 
			(i4)sizeof(i4_buff), (PTR)&i4_buff);
    rqu_putflush(assoc, TRUE);

    RQTRACE(rqs,1)(RQFMT(rqs,
	"RQF:Assn %d. MODIFY_ASSN. GCA_SUPSYS -> %s.\n"), 
		assoc->rql_assoc_id, 
		(i4_buff == GCA_ON) ? "GCA_ON" : "GCA_OFF" );

    /* check for association modification request acceptance */
    if (rqu_get_reply(assoc, -1) != GCA_ACCEPT)
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. MODIFY_ASSN. Cannot modify assn.\n"),
		    assoc->rql_assoc_id);
	return( E_RQ0006_CANNOT_GET_ASSOCIATION );
    }
    return(E_RQ0000_OK);
}

/*{
** Name: rqf_cleanup - flush out all pending messages in every association
**			owned by this session. 
**
** Description:
**      This routine is called by QEF  when QEF needs to flush all associations
**	from pending receiver reads and transmiter writes. Those pending
**	operations could be eighter because of protocal, LDB, user,
**	or errors. We hope this will always work.
**
** Inputs:
**      rcb		current request control block
**
** Outputs:
**        
**	Returns:
**	    E_RQ0000_OK always.
**
** Side Effects:
**	    none
**
** History:
**      25-mar-1989 (georgeg)
**          created
*/
DB_ERRTYPE
rqf_cleanup(
	RQR_CB	*rcb)
{
    RQL_ASSOC	*assoc = ((RQS_CB *)rcb->rqr_session)->rqs_assoc_list;
    RQS_CB	*rqs = (RQS_CB *)rcb->rqr_session;

    /*
    **	send it to all active associations.
    */
    while (assoc != NULL )
    {
	assoc->rql_rqr_cb = rcb;
	if ( rqu_interrupt( assoc, GCA_INTALL  ) != E_RQ0000_OK )
	{
	    RQTRACE(rqs,1)(RQFMT(rqs,
				"RQF:Assn %d. IACK not received.\n"),
			assoc->rql_assoc_id);
	}
	assoc = assoc->rql_next;
    } /*end while */

    return(E_RQ0000_OK);

}

/*{
** Name: rqf_term_assoc - close the association for the ddbdb.
**
** Description:
**      This routine will close the association to the ddbdb data base.
**	created.
**
** Inputs:
**      rqr_cb->
**	  rqr_1_site	site description
**
** Outputs:
** 	None
**        
**	Returns:
**	    E_RQ0006_CANNOT_GET_ASSOCIATION
**	    E_RQ0000_OK
**
** Side Effects:
**	    none
**
** History:
**      26-mar-1989 (georgeg)
**          created
*/
DB_ERRTYPE
rqf_term_assoc(
	RQR_CB	    *rcb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc( rcb, 0 );
  RQS_CB	*session = (RQS_CB *)rcb->rqr_session;
  RQL_ASSOC	*temp_assoc = (RQL_ASSOC *)session->rqs_assoc_list;
  
  if ( assoc == NULL)
  {
	return(E_RQ0006_CANNOT_GET_ASSOCIATION);
  }
  
  (VOID)rqu_release_assoc( assoc );
  
  /*
  ** Remove the assocciation from the list of associations.
  */

  if ( assoc == temp_assoc )
  {
    session->rqs_assoc_list = assoc->rql_next;
  }
  else
  {
    while ( temp_assoc != NULL )
    {
	if (assoc == temp_assoc->rql_next)
	{
	    temp_assoc->rql_next = assoc->rql_next;
	    break;
	}
	else
	{
	    temp_assoc = temp_assoc->rql_next;
	}	

    }  /*end while */
  } /* end if */
  return(E_RQ0000_OK);
}

/*{
** Name: rqf_ldb_arch - RQF Architecture id.
**
** Description:
**      This routine returns a boolean to determine wether both partnes of
**	an association  share the same h/w and s/w architecture.
**
** Inputs:
**      rqr_cb->
**	  rqr_1_site	site description
**
** Outputs:
** 	None
**        
**	Returns:
**	    E_RQ0006_CANNOT_GET_ASSOCIATION
**	    E_RQ0000_OK
**
** Side Effects:
**	    none
**
** History:
**      15-apr-1989 (georgeg)
**          created
*/  
DB_ERRTYPE
rqf_ldb_arch(
	RQR_CB	    *rcb)
{
  RQL_ASSOC	*assoc = rqu_check_assoc( rcb, 1 );
  
  if ( assoc == NULL)
  {
	return(E_RQ0006_CANNOT_GET_ASSOCIATION);
  }
  rcb->rqr_diff_arch = assoc->rql_diff_arch;

  return(E_RQ0000_OK);
}

/*{
** Name: rqf_restart - RQF restart a lost association.
**
** Description:
**      This routine looks up the list of active associations for the specified
**	association, if the association is present it will close it and reestablish
**	one with the specified reassiciation data.
**
** Inputs:
**      rqr_cb->
**	    rqr_1_site	    site description
**	    rqr_2pc_dx_id   transaction id
** Outputs:
** 	None
**        
**	Returns:
**	    E_RQ0049_RESTART_FAILED
**	    E_RQ0000_OK
**
** Side Effects:
**	    none
**
** History:
**      07-nov-1989 (georgeg)
**          created
*/  
DB_ERRTYPE
rqf_restart(
	RQR_CB	    *rcb)
{
    RQL_ASSOC	    *assoc = rqu_check_assoc(rcb, 0);
    RQS_CB	    *session = (RQS_CB *)rcb->rqr_session;
    DD_LDB_DESC	    *site = rcb->rqr_1_site;
  
    /*
    ** If the association is alive close it and the reopen one with
    ** the same control block
    */
    if (assoc != NULL)
    {
	(VOID)rqu_release_assoc(assoc);
	assoc->rql_turned_around = FALSE;
    }

    session->rqs_current_site = (RQL_ASSOC *)rqu_new_assoc(rcb, session, site,
                            (DD_USR_DESC *)&session->rqs_user, 
                            (DD_COM_FLAGS *)&session->rqs_com_flags,
			    assoc,
			    rcb->rqr_2pc_dx_id
			);

    /*
    ** see if connection to the requested site has been opened 
    */
    if (session->rqs_current_site == NULL)
    {
	rcb->rqr_error.err_code = E_RQ0049_RESTART_FAILED;
        rqu_error(0, rcb->rqr_error.err_code, (PTR) NULL,
		    (i4)3,
		    (i4)((char *)STindex(site->dd_l2_node_name, " ",
                                    sizeof(site->dd_l2_node_name)) 
                            - (char *)site->dd_l2_node_name),
                    site->dd_l2_node_name,
                    (i4)((char *)STindex(site->dd_l3_ldb_name, " ", 
                                    sizeof(site->dd_l3_ldb_name) ) 
                            - (char *)site->dd_l3_ldb_name),
                    site->dd_l3_ldb_name,
                    (i4)((char *)STindex(site->dd_l4_dbms_name, " ",
                                    sizeof(site->dd_l4_dbms_name)) 
                            - (char *)site->dd_l4_dbms_name),
                    site->dd_l4_dbms_name,
                    (i4)0, (PTR)NULL
                  );

	return(E_RQ0049_RESTART_FAILED);
    }
    else
    {
	return(E_RQ0000_OK);
    }
}

/*{
** Name: rqf_cluster_info - Get the names of the nodes in the cluster.
**
** Description:
**	This routine will get the nmaes of the nodes in the cluster.
**
** Inputs:
**      rqr_cb->
**	    rqr_cluster_p   a list of RDD_CLUSTER_INFO node names
**	    rqr_nodes	    number of nodes in the cluster.
** Outputs:
** 	None
**        
**	Returns:
**	    E_RQ0000_OK
**
** Side Effects:
**	    none
**
** History:
**      10-nov-1990 (georgeg)
**          created
*/  
DB_ERRTYPE
rqf_cluster_info(
	RQR_CB	    *rcb)
{

    if  (rcb->rqr_nodes <= 0)
    {
	Rqf_facility->rqf_cluster_p = (PTR)NULL;
	Rqf_facility->rqf_num_nodes = 0;
    }
    else
    {
	Rqf_facility->rqf_cluster_p = (PTR)rcb->rqr_cluster_p;
	Rqf_facility->rqf_num_nodes = rcb->rqr_nodes;
    }
    return(E_RQ0000_OK);
}


/*{
** Name: rqf_set_trace - Set/Unset an rqf trace point.
**
** Description:
**	Called with RQR_TRACE to set/unset a rqf session trace point.
**
** Inputs:
**	rcb		    		- request control block
**	    ->rqr_dbg_cb
**		->db_trace_point	- the trace point to set/unset
**	    ->rqr_session		- current session context
**	    
** Outputs:
**	None.
**        
** Returns:
**	E_RQ0000_OK	- always
**
** Side Effects:
**	None
**
** History:
**      13-aug-1991 (fpang)
**          Created
*/  
DB_ERRTYPE
rqf_set_trace(
	RQR_CB *rcb)
{
    i4	tr_pt;
    i4	val1 = 0L;
    i4	val2 = 0L;
    RQS_CB	*rqs = (RQS_CB *)rcb->rqr_session;

    tr_pt = rcb->rqr_dbgcb_p->db_trace_point;

    if (rcb->rqr_dbgcb_p->db_value_count > 0 )
    {
	val1 = rcb->rqr_dbgcb_p->db_vals[0];
    }

    if (rcb->rqr_dbgcb_p->db_value_count > 1 )
    {
	val2 = rcb->rqr_dbgcb_p->db_vals[1];
    }

    switch (rcb->rqr_dbgcb_p->db_trswitch)
    {

	case DB_TR_ON:
	    switch (tr_pt)
	    {
		case RQT_128_LOG:
		    /* If server level logging alread on, ignore */
		    if (Rqf_facility->rqf_dbg_lvl == 0)
		    {
			/* Should be between 0..4 */
			if (val1 == 0)
			    rqs->rqs_dbg_lvl = 1;	/* default to 1 */
			else if (val1 < 0 || val1 > 4)
			    rqs->rqs_dbg_lvl = 4;	/* garbage, do max */
			else
			    rqs->rqs_dbg_lvl = val1;	
		    }
		    break;
		default:
		    break;
	    }
	    break;
	case DB_TR_OFF:
	    switch (tr_pt)
	    {
		case RQT_128_LOG:
		    rqs->rqs_dbg_lvl = 0;
		    break;
		default:
		    break;
	    }
	    break;
	case DB_TR_NOCHANGE:
	default:
	    break;
    }

    return(E_RQ0000_OK);
}

/*{
** Name: rqf_invproc - invoke a local database procedure. (RQR_INVPROC)
**
** Description:
**	Initiate the execution of a database procedure on the requested LDB,
**	passing along the user's parameters. Communication with the LDB will
**	be driven until a tuple descriptor or response block is returned.
**	Any midstream messages or errors will be queued to the current
**	session.  The caller of the routine is responsible for continuing
**	the communication with the LDB, by subsequent reading of the
**	tuples until a response block is returned.
**
** Inputs:
**	rcb->		    		- request control block
**	    rqr_session			- session context.
**	    rqr_timeout			- timeout length
**	    rqr_1_site			- LDB to execute procedure on
**	    rqr_q_language		- query language, SQL
**	    rqr_inv_parms		- Users parameters.
**	    rqr_qid.			- LDB procedure id, DB_CURSOR_ID, 
**		db_cursor_id		- LDB procedure ID.
**		db_cur_name		- LDB procedure name.
**	    rqr_tupdesc_p->		- Tuple descriptor, NULL if not byref
**		scf_ldb_td_p		- LDB TDESCR, *GCA_TD_DATA
**		scf_star_td_p		- SCF TDESCR, *GCA_TD_DATA
** Outputs:
**	rcb->
**	    rqr_qid.			- LDB procedure
**		db_cursor_id		- LDB procedure ID	
**		db_cur_name		- LDB procudure name
**	    rqr_dbp_rstat		- LDB return status (GCA_RETPROC)
**	    rqr_ldb_abort		- True if LDB aborted
**	    rqr_ldb_status		- LDB statement status mask 
**        
** Returns:
**	E_RQ0000_OK	- sucessful initiation
**	E_RQ006_CANNOT_GET_ASSOCIATION
**			- ldb not reachable
**	E_RQ0014_QUERY_ERROR
**			- Ldb error occured.
**
** Side Effects:
**	None
**
** History:
**      21-dec-1992 (fpang)
**          Created
**	05-feb-1993 (fpang)
**	    Added support for detecting commits/aborts when executing a 
**	    a procedure or in direct execute immediate.
**	31-mar-93 (swm)
**	    Corrected NULL cast in if-statement == comparison from
**	    (SCF_TD_CB *) to (PTR) because rqr_tupdesc_p in an RQR_CB
**	    is declared as a PTR, even though it is intended for use
**	    in scf.
**	12-apr-1993 (fpang)
**	    Added support for GCA1_INVPROC.
*/  
DB_ERRTYPE
rqf_invproc(
	RQR_CB	*rqr_cb)
{
    RQL_ASSOC	*assoc = rqu_check_assoc( rqr_cb, 1 );
    DB_ERRTYPE	status = E_RQ0000_OK;
    RQS_CB	*rqs = (RQS_CB *)rqr_cb->rqr_session;
    i4  	msg;
    i4	proc_mask = GCA_PARAM_NAMES_MASK;
    i4		len;	/* Must be same type as GCA_NAME.gca_l_name */

    if (assoc == NULL)
	return (E_RQ0006_CANNOT_GET_ASSOCIATION);

    /* Can only execute procedures on LDB's with protocol > 60. */

    if (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_60)
	return (E_RQ0014_QUERY_ERROR); 		/* Use better message later */

    if (rqr_cb->rqr_2pc_txn == TRUE)
	proc_mask |= GCA_IP_NO_XACT_STMT;

    do
    {
	assoc->rql_described = FALSE;
	++assoc->rql_nostatments;

	rqu_putinit( assoc, GCA1_INVPROC );

	/* Put the procedure ID */
	if ((status = rqu_putcid( assoc, &rqr_cb->rqr_qid )) != E_RQ0000_OK)
	    break;

	/* Put the owner name */
	if (rqr_cb->rqr_own_name != NULL)
	    len = STlength((char *)rqr_cb->rqr_own_name);
	else
	    len = 0;

	if ((status = rqu_put_data(assoc, (PTR) &len, 
					    sizeof(len))) != E_RQ0000_OK)
	    break;

	if ((status = rqu_put_data(assoc, (PTR) rqr_cb->rqr_own_name, 
						    len)) != E_RQ0000_OK)
	    break;

	/* Put the proc mask */
	if ((status = rqu_put_data( assoc, (PTR) &proc_mask, 
					    sizeof(proc_mask))) != E_RQ0000_OK)
	    break;

	/* Put any parameters */
	if ((status = rqu_put_pparms( assoc, 
				      rqr_cb->rqr_dv_cnt,
				      rqr_cb->rqr_dbp_param )) != E_RQ0000_OK)
	    break;

	/* Flush, send it */
	if ((status = rqu_putflush( assoc, TRUE )) != E_RQ0000_OK)
	    break;

	assoc->rql_noerror = FALSE;

	/* Next message must be a retproc. */
	if ((msg = rqu_get_reply(assoc, -1)) != GCA_RETPROC)
	{
	    status = E_RQ0014_QUERY_ERROR;
	    break;
	}

	/* Get LDB procedure ID and return status from GCA_RP_DATA. */
	if ((status = rqu_get_rpdata( assoc, 
				      &rqr_cb->rqr_qid, 
				      &rqr_cb->rqr_dbp_rstat )) != E_RQ0000_OK)
	    break;

	/* Return if no byref parameters. */
	if (rqr_cb->rqr_tupdesc_p == (PTR)NULL)
	    break;

	/* Next message must be a tuple descriptor */
	if ((msg = rqu_get_reply(assoc, -1)) != GCA_TDESCR)
	{
	    status = E_RQ0014_QUERY_ERROR;
	    break;
	}

	/* Read the tuple desciptor. */
	if ((status = rqf_td_get ( rqr_cb, assoc )) != E_RQ0000_OK)
	    break;
    }
    while (0);

    if (status != E_RQ0000_OK)
    {
	/* Interrupt the association. */
	(VOID)rqu_interrupt( assoc, (i4)GCA_INTALL );
	return (E_RQ0014_QUERY_ERROR); 	/* Use better message later */
    }

    assoc->rql_described = TRUE;	/* To fake out RQR_GET later */
    return ( E_RQ0000_OK );
}

/*{
** Name: rqf_format - Send a formatted trace message to SCF.
**
** Description:
**	Called by TRformat(). The formatted trace message is sent to SCF
**	to be queued or output, or output via TRdisplay. A newline is added
**	to the string if removed by TRformat().
**
** Inputs:
**	arg		- pointer to i4, if 1, output a '\n'
**	len		- len of buffer
**	buffer		- the formatted trace message.
**	    
** Outputs:
** 	None
**        
** Returns:
**	VOID
**
** Side Effects:
**	    none
**
** History:
**      13-aug-1991 (fpang)
**          Created
**	10-oct-93 (swm)
**	    Bug #56448
**	    Added check for server tracing or session tracing to determine
**	    destination of output. This check used to be done by rqf_trace()
**	    which no longer exists.
*/  
i4
rqf_format(
	PTR	arg,
	i4	len,
	char	buffer[])
{
    SCF_CB	scf_cb;

    /* if *arg == 1, add a newline */

    if (arg == (PTR)1)
    {
	buffer[len++] = '\n';
    }

    if (len == 0)
	return 0;

    if (Rqf_facility->rqf_dbg_lvl != 0)
    {
	/* Server tracing, just output */

	TRdisplay(buffer);
    }
    else
    {
	/* Session tracing, send to scf to relay to FE */

	scf_cb.scf_length = sizeof(scf_cb);
	scf_cb.scf_type   = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_QEF_ID;
	scf_cb.scf_len_union.scf_blength = len;
	scf_cb.scf_ptr_union.scf_buffer  = buffer;
	scf_cb.scf_session = DB_NOSESSION;

	scf_call(SCC_TRACE, &scf_cb);
    }
    return 0;
}

/*{
** Name: rqf_neednl - Determine newline flag for TRformat format string
**
** Description:
**	Determine whether a format string terminates with a '\n', and return
**	a pointer to a flag for use with TRformat.
**
** Inputs:
**	fmt	- the format string.
**	    
** Outputs:
** 	None
**        
** Returns:
**      i4 * - pointer to i4 flag
**
** Side Effects:
**	    none
**
** History:
**      11-oct-93 (swm)
**	    Bug #56448
**          Created
*/  

static i4 neednl = (i4)1;
static i4 dontneednl = (i4)0;

i4 *
rqf_neednl(fmt)
char	*fmt;
{

    i4      length;

    length = STlength(fmt);

    if (fmt[length-1] == '\n')
	return &neednl;
    else return &dontneednl;
}


/*{
** Name: rqf_call	- RQF entry point
**
** Description:
**      This the entry point for RQF. All RQF request must be made through 
**      this routines. 
**
** Inputs:
**      rqr_cb	    pointer to RQF request control block
**      rqr_type    RQF request type. See RQR_TYPE in rqf.h
[@PARAM_DESCR@]...
**
** Outputs:
**      [@param_name@] [@comment_text@]#41$
[@PARAM_DESCR@]...
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-may-1988 (alexh)
**          created
**	13-aug-1991 (fpang)
**	    Added support for RQR_TRACE.
**	15-oct-1992 (fpang)
**	    Handle rqr_timeout correctly. Previously, the DMC constants
**	    were not checked.
**	18-jan-1993 (fpang)
**	    Added support for RQR_INVPROC.
**	05-feb-1993 (fpang)
**	    Added support for detecting commits/aborts when executing a 
**	    a procedure or in direct execute immediate.
**	06-oct-1993 (rog)
**	    Don't call rqf_check_interrupt() if a repeat query needs to be
**	    redefined (bug 54767).
**	19-nov-1993 (fpang)
**	    Changed default timeout to 15 min.
**	    Fixes B57105, Star timesout in 4 hrs, not 15 minutes.
**	20-apr-94 (swm)
**	    Bug #62667
**	    Last change to this file inadvertently repeated a block of
**	    code which does a conditional return, twice. Removed offending
**	    code.
*/
DB_STATUS
rqf_call(
	i4         rqr_type,
	RQR_CB	    *rqr_cb)
{
    bool	    gca_qry;	/* TRUE if GCA_QUERY, FALSE if GCA_DEFINE */
    RQS_CB	    *rqs = (RQS_CB *)rqr_cb->rqr_session;
    RQL_ASSOC       *current_site = (RQL_ASSOC *)NULL;

    /*
    ** initialize return status 
    */
    rqr_cb->rqr_gca_status = NULL;
    rqr_cb->rqr_opcode = rqr_type;

    if (rqr_type != RQR_STARTUP)
    {
	/*
	** qef will set to -1 if user specified 0.
	** in qef_call():
	**    dds2_p->qed_d10_timeout = 
	**       (ddr2_p->qef_d14_ses_timeout == 0) ? -1 : 
	**                                      ddr_p->qef_d14_ses_timeout;
	** DMC_C_SYSTEM, -1 =   no timeout
	** DMC_C_SESSION, 0 =   default
	** 1..n	            =   set by user
	*/
	if ((rqr_cb->rqr_timeout == DMC_C_SESSION) || 
	    (rqr_cb->rqr_timeout == 0))
	{
	    /* default */
	    rqr_cb->rqr_timeout = 900;  /* 15 mins */
	}
	else if ((rqr_cb->rqr_timeout == DMC_C_SYSTEM) ||
		 (rqr_cb->rqr_timeout == -1))
	{
	    rqr_cb->rqr_timeout = 0;     /* no timeout */
	}
    }

    switch (rqr_type)
    {
	case RQR_NORMAL:/* this one should phase out */
	case RQR_QUERY:
	case RQR_DATA_VALUES:
	    rqr_cb->rqr_error.err_code = rqf_query(rqr_cb);
	    break;
	case RQR_GET:
	    rqr_cb->rqr_error.err_code = rqf_nt_fetch(rqr_cb);
	    break;
	case RQR_FETCH:
	    rqr_cb->rqr_error.err_code = rqf_fetch(rqr_cb);
	    break;
	case RQR_OPEN:
	    rqr_cb->rqr_error.err_code = rqf_open_cursor(rqr_cb);
	    break;
	case RQR_UPDATE:
	    rqr_cb->rqr_error.err_code = rqf_update(rqr_cb);
	    break;
	case RQR_DEFINE:
	    if (rqr_type == RQR_DEFINE)
	    {
		gca_qry = FALSE;			/* GCA_DEFINE */
	    }
	    else
	    {
		gca_qry = TRUE;
	    }
	    rqr_cb->rqr_error.err_code = rqf_define(rqr_cb, gca_qry);
	    break;
	case RQR_CLOSE:
	    rqr_cb->rqr_error.err_code = rqf_close(rqr_cb);
	    break;
	case RQR_DELETE:
	    rqr_cb->rqr_error.err_code = rqf_delete(rqr_cb);
	    break;
	case RQR_EXEC:
	    rqr_cb->rqr_error.err_code = rqf_invoke(rqr_cb);
	    break;
	case RQR_CONNECT:
	    rqr_cb->rqr_error.err_code = rqf_connect(rqr_cb);
	    break;
	case RQR_DISCONNECT:
	    rqr_cb->rqr_error.err_code = rqf_disconnect(rqr_cb);
	    break;
	case RQR_CONTINUE:
	    rqr_cb->rqr_error.err_code = rqf_continue(rqr_cb);
	    if ( ((SCC_SDC_CB *)rqr_cb->rqr_dc_blkp)->sdc_ldb_status 
							== SDC_INTERRUPTED)
	    {
		return( rqr_cb->rqr_error.err_code );
	    }
	    break;
	case RQR_CLEANUP:
	    rqr_cb->rqr_error.err_code = rqf_cleanup(rqr_cb);
	    break;
	case RQR_XFR:
	    rqr_cb->rqr_error.err_code = rqf_transfer(rqr_cb);
	    break;
	case RQR_T_FETCH:
	    rqr_cb->rqr_error.err_code = rqf_t_fetch(rqr_cb);
	    break;
	case RQR_T_FLUSH:/* this one is obsolete */
	case RQR_ENDRET:
	    rqr_cb->rqr_error.err_code = rqf_endret(rqr_cb);
	    break;
	case RQR_COMMIT:
	    rqr_cb->rqr_error.err_code = rqf_commit(rqr_cb);
	    break;
	case RQR_ABORT:
	    rqr_cb->rqr_error.err_code = rqf_abort(rqr_cb);
	    break;
	case RQR_SECURE:
	    rqr_cb->rqr_error.err_code = rqf_secure(rqr_cb);
	    break;
	case RQR_BEGIN:
	    rqr_cb->rqr_error.err_code = rqf_begin(rqr_cb);
	    break;
	case RQR_S_BEGIN:
	    rqr_cb->rqr_error.err_code = rqf_s_begin(rqr_cb);
	    break;
	case RQR_S_END:
	    rqr_cb->rqr_error.err_code = rqf_s_end(rqr_cb);
	    break;
	case RQR_STARTUP:
	    rqr_cb->rqr_error.err_code = rqf_startup(rqr_cb);
	    break;
	case RQR_SHUTDOWN:
	    rqr_cb->rqr_error.err_code = rqf_shutdown(rqr_cb);
	    break;
	case RQR_SET_FUNC:
	    rqr_cb->rqr_error.err_code = rqf_set_option(rqr_cb);
	    break;
	case RQR_MODIFY:
	    rqr_cb->rqr_error.err_code = rqf_modify(rqr_cb);
	    break;
	case RQR_TERM_ASSOC:
	    rqr_cb->rqr_error.err_code = rqf_term_assoc(rqr_cb);
	    break;
	case RQR_LDB_ARCH:
	    rqr_cb->rqr_error.err_code = rqf_ldb_arch(rqr_cb);
	    break;
	case RQR_RESTART:
	    rqr_cb->rqr_error.err_code = rqf_restart(rqr_cb);
	    break;
	case RQR_CLUSTER_INFO:
	    rqr_cb->rqr_error.err_code = rqf_cluster_info(rqr_cb);
	    break;
	case RQR_TRACE :
	    rqr_cb->rqr_error.err_code = rqf_set_trace(rqr_cb);
	    break;
	case RQR_INVPROC :
	    rqr_cb->rqr_error.err_code = rqf_invproc(rqr_cb);
	    break;
	default:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqf_call. Unknown opcode %d\n"), rqr_type);
	    rqr_cb->rqr_error.err_code = E_RQ0007_BAD_REQUEST_CODE;
	    break;
    }


    rqs = (RQS_CB *)rqr_cb->rqr_session;
    if ( ( rqs != (RQS_CB *)NULL ) && 
	 ( (RQL_ASSOC *)rqs->rqs_current_site != (RQL_ASSOC*)NULL ))
    { 
        current_site = (RQL_ASSOC *)rqs->rqs_current_site;
	rqr_cb->rqr_ldb_status = (i4) current_site->rql_re_status;
	if ((current_site->rql_re_status & RQF_03_TERM_MASK) ||
	    (current_site->rql_re_status & RQF_04_XACT_MASK))
	    rqr_cb->rqr_ldb_abort = TRUE;
    }

    if (rqr_cb->rqr_error.err_code == E_RQ0000_OK )
    {
	return (E_RQ0000_OK);
    }

    /*
    ** If a repeat query needs to be redefined, catch the error here
    ** and return E_DB_ERROR so that QEF knows to redefine it.  We
    ** don't want to send an interrupt for this error because it
    ** might close open transactions.
    */
    if (rqr_cb->rqr_error.err_code == E_RQ0040_UNKNOWN_REPEAT_Q)
    {
	return (E_DB_ERROR);
    }

    if ( current_site != (RQL_ASSOC*)NULL )
    {	

	if ( current_site->rql_status == RQL_TERMINATED )
	{
	    rqr_cb->rqr_error.err_code = E_RQ0045_CONNECTION_LOST;
	}    
	else if ( current_site->rql_status == RQL_TIMEDOUT )
	{
	    current_site->rql_status = RQL_OK;
	    current_site->rql_timeout = 600;
	    (VOID)rqf_check_interrupt(rqr_cb);
	    if ( 
		current_site->rql_status == RQL_TIMEDOUT
		    ||
		current_site->rql_status == RQL_TERMINATED
	       )
	    {
		RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. No response from GCA_INTERRUPT\n"),
                            current_site->rql_assoc_id );
		RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. Will destroy association\n"),	
                            current_site->rql_assoc_id );
		current_site->rql_timeout = 0;
		(VOID)rqu_release_assoc( current_site );
		current_site->rql_status = RQL_TERMINATED;
		rqr_cb->rqr_error.err_code = E_RQ0045_CONNECTION_LOST;
	    }		
	    else
	    {
		rqr_cb->rqr_error.err_code = E_RQ0046_RECV_TIMEOUT;
	    }
	}
	else if (current_site->rql_status == RQL_LDB_ERROR)
	{
	    rqr_cb->rqr_1ldb_severity = current_site->rql_ldb_severity;
	    rqr_cb->rqr_2ldb_error_id = current_site->rql_id_error;
	    rqr_cb->rqr_3ldb_local_id = current_site->rql_local_error;
	    current_site->rql_status = RQL_OK;
	    (VOID)rqf_check_interrupt(rqr_cb);
	}
	else
	{
	    (VOID)rqf_check_interrupt(rqr_cb);
	}
	    
    }
    
    RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:rqf_call. RQF error dec:%d hex:%x\n"), 
	       rqr_cb->rqr_error.err_code - E_RQ_MASK, 
	       rqr_cb->rqr_error.err_code - E_RQ_MASK);

    return (E_DB_ERROR);
}
