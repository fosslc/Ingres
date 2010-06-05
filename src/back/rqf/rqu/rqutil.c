/*
**Copyright (c) 2004, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cm.h>
#include    <cs.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <tm.h>
#include    <tr.h>
#include    <st.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <adf.h>
#include    <adp.h>
#include    <qsf.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <generr.h>
#include    <sqlstate.h>
#include    <ulh.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <rdf.h>
#include    <tmtz.h>
#include    <rqf.h>
#include    <rqfprvt.h>
#if defined(hp3_us5)
#pragma OPT_LEVEL 2
#endif /* hp3_us5 */

/**
**
**  Name: rqu.C - remote query facility utility primitives
**
**  Description:
**      This files contains RQF internal utility routines.
**
**	rqu_gcmsg_type - log GCA read/write info
**	rqu_dbg_error - RQF GCA error debug hook
**	rqu_client_trace - send trace to client
**	rqu_relay - relay a GCA_ERROR, GCA_EVENT or GCA_TRACE to STAR client
**	rqu_error - send error to client
**	rqu_find_lid - search a lid in a association list
**	rqu_send - send a message
**	rqu_release_assoc - release association
**	rqu_end_all_assoc - release a list of associations
**	rqu_prepare_assoc - prepare association buffers
**	rqu_dump_gca_msg - dump a GCA error message
**	rqu_putinit - initialize a put stream
**	rqu_putflush - flush a message stream
**	rqu_put_data - put data value into GCA data area
**	rqu_putdb_val - put a GCA_DATA_VALUE to a message stream
**	rqu_putcid - put a GCA_ID as a structure to a message stream
**	rqu_putqid - put a GCA_ID as 3 parameters to a message stream
**	rqu_putformat - put a CREATE statement format to stream
**	rqu_new_assoc - allocate a association for site
**	rqu_ingres_priv - modify association ingres priviledge
**	rqu_check_assoc - ascertain association
**	rqu_get_reply - get a response or return data
**	rqu_create_tmp - synthesize a CREATE TABLE statement to stream
**	rqu_xsend - send a externally formatted message
**	rqu_mod_assoc - construct and send current association flags
**	rqu_fetch_data - fetch data from assoc stream
**	rqu_getqid - get a GCA_ID from a message stream
**	rqu_col_desc_fetch - fetch column description
**	rqu_td_save - save tuple description in bind table
**	rqu_convert_data - perform TITAN catalog column data conversion
**	rqu_interrupt	- send interrupt to an association
**	rqu_set_option	- set a lang option
**	rqu_gca_error	- trap and report to the user a GCA_ERROR block
**	rqu_put_qtxt    - send query text to an association
**	rqu_put_parms	- send query parameters to an association
**	rqu_put_iparms	- send query parameters to an association
**	rqu_put_moddata	- send modify association values
**	rqu_put_pparms	- put procedure parameters to the output stream.
**	rqu_get_rpdata	- get a GCA_RP_DATA from a message stream.
**
**  History:
**      27-Oct-1988 (alexh)
**          created
**      21-may-1989 (carl)
**	    modified rqu_get_reply to detect and accept LDB warning as 
**	    good status 
**	30-aug-1989 (carl)
**	    fixed access violation problem in rqu_mod_assoc.
**	08-nov-1989 (georgeg)
**	    if the peer protocol is <= GCA_PROTOCOL_LEVEL_2 then convert errors
**	    and generic errors to GCA_PROTOCOL_LEVEL.
**      02-apr-1990 (carl)
**          1.  added rqu_cp_get for alignment machines
**          2.  extended rqu_col_desc_fetch to accept GCA_C_FROM so that
**		rqu_cp_get can become a client
**      02-apr-1990 (georgeg)
**	    added GCA_C_INTO to rqu_col_fetch.
**	30-apr-1990 (georgeg)
**	    added 2pc support, merged titan63 and 2pc code paths.
**	01-nov-1990 (fpang)
**	    Removed timeout from async GCA_RECEIVE. 
**	06-nov-1990 (georgeg)
**	    Added support clusters.
**	12-dec-1990 (fpang)
**	    When calling GCA_REQUEST allocate storage for partner name,
**	    user name, and password instead of using stack storage. The
**	    gca_call could die if we CSsuspend.
**	17-dec-1990 (fpang)
**	    In rqu_new_assoc(), make sure password is required
**	    (conflags & RQF_PASS_REQUIRED) and user did not give a
**	    password (gca_req.gca_password == NULL) before invalidating
**	    an association.
**	11-jan-1991 (fpang)
**	    Fixed uninititalized variable in rqu_new_assoc().
**	31-jan-1991 (fpang)
**	    Added GCA_EVENT support.
**      06-mar-1991 (fpang)
**          1. Changed TRdisplays to RQTRACE.
**          2. Make sure association id is in all trace messages.
**          3. Support levels of tracing.
**	18-apr-1991 (fpang)
**	    For partnername memory allocation, use scf memory. 
**      19-mar-91 (dchan)
**          The decstation compiler didn't like the
**          construct " & (PTR) t_pd " in rqu_cp_get(), so change it
**          to "(PTR)(&t_pd)".
**	01-may-91 (fpang)
**	    Rewrote rqu_cp_get() to match description. 
**	09-may-1991 (fpang)
**	    Allow -A application flags to be sent to priviledged cdb.
**	30-may-91 (fpang)
**	    RQTRACE(0) displays only for errors. Cuts down on amount
**	    of fluff in logs.
**	    Fixes B37195.
**	25-jul-91 (johnr)
**	    hp3 requires pragma for optimization level reduction.
**	03-aug-1991 (fpang)
**	    In rqu_mod_assoc(), GCA_EXCLUSIVE needs a constant 0 passed,
**	    and make sure count is correct if -A is present.
**	13-aug-1991 (fpang)
**	    1. rqu_dmp_gcaqry() written to dump direct connect queries.
**	    2. Added missing GCA_MESSAGES to rqu_gcmsg_type().
**	    3. RQTRACE modifications for rqf trace points.
**	    4. rqu_mod_assoc() changes to support GCA_GW_PARMS and GCA_TIMEZONE
**	04-sep-1991 (fpang)
**	    In rqu_mod_assoc(), if GCA_XUPSYS is requested, change it to
**	    GCA_SUPSYS for the user-cdb association. It will always fail
**	    because of the already established priv-cdb association.
**	    Fixes B39553.
**	29-oct-1991 (fpang)
**	    In rqu_mod_assoc(), send GCA_GW_PARMS and GCA_TIMEZONE to ldb only 
**	    if it's protocol level is greater than 2.
**	13-sep-1991 (fpang)
**	    In rqu_get_reply() if gca_error block has generic error of
**	    GE_OK or GE_WARNING, treat it like an informational or warning
**	    message. Also, don't output the 'Preceding message..' message
**	    if the message is a GCA_MESSAGE.
**	    This fixes B39443 and B40051.
**	18-sep-1991 (fpang)
**	    In rqu_gca_error(), log RQ004A instead of RQ0042 to the error log.
**	    RQ0042 is confusing in the error log because the local does not
**	    always log errors. RQ004A will contain the local error and make
**	    things less confusing. Fixes B39727.
**	18-sep-1991 (fpang)
**	    In rqu_new_assoc() issued RQ004B instead of RQ0047. The only
**	    time that we do not have a password is if local authorization
**	    is missing. Fixes B39731.
**	27-dec-1991 (fpang)
**	    GCA_TIMEZONE if and only if GCA_PROTOCOL_LEVEL >= 31.
**	    Fixes B41601.
**	04-feb-1992 (fpang)
**	    In rqu_dbg_error(), removed tracing of GCN errors, they are no
**	    longer visible to mainline code.
**	25-Feb-1992 (fpang)
**	    In rqu_putqid(), when putting the gca_name field as a 
**	    db_data_value, it's type should be DB_CHA_TYPE rather than 
**	    DB_TXT_TYPE. 
**	    Fixes B42194 and B42237.
**	09-APR-1992 (fpang)
**	    In rqu_new_assoc(), if re-using storage (assocmem_ptr != NULL), 
**	    don't reset the linked list pointers, rql_next, and rqs_assoc_list.
**	    Reseting them will result in an inconsistent association list.
**	    Fixed B42810 (SEGV is LDB is killed between SECURE and COMMIT).
**	15-oct-1992 (fpang)
**	    Sybil Phase 2
**	    1. General cleanup of trace messages.
**	    2. New trace display routines, rqu_dmp_tdesc() and rqu_dmp_dbdv().
**	    3. New routine rqu_sdc_chkintr() to check for async interrupts from 
**	       the ldb.
**	    4. Support from cs_client_type wrt. to scc_gcomplete() changes.
**	    5. Remove sc0m_rqf_alloc() calls in favor of ULM calls.
**	    6. Replace recursion with interation in rqu_fetch_data(), 
**	       rqu_find_lid(), rqu_end_all_assoc(), and rqu_put_data().
**	    7. Support GCA_RESUME protocol for GCA_REQUEST and GCA_DISASSOC.
**	    8. Support GCA1_C_FROM/INTO.
**	    9. rqu_putdb_val() now dumps db-datavalues.
**	   10. Eliminated rqu_cp_get() because rqf_cp_get() was modified to be
**	       portable.
**	19-oct-92 (teresa)
**	    Implemented prototypes.
**      16-nov-92 (terjeb)
**          rqu_release_assoc(), rqu_dmp_dbdv() and rqu_dmp_tdes() are 
**          defined in rqfprvt.h, so no need to do it here.
**	08-dec-92 (fpang)
**	    New function rqu_put_qtxt(), rqu_put_parms() and rqu_put_iparms()
**	    to modularize the processing of query text and query parameters.
**	14-dec-92 (anitap)
**	    Added #include <qsf.h> for CREATE SCHEMA project.
**	13-jan-1993 (fpang)
**	    Added temp table support. In rqu_create_tmp() if LDB 
**	    supports temp table, use 'declare global temporary table..'
**	    instead of 'create table..'.
**	13-jan-1993 (fpang)
**	    Turn on GCA_TIMEZONE_NAME code.
**	13-jan-1993 (fpang)
**	    Register database procedure support.
**	    1. New function rqu_put_pparms() outputs function parameters when
**	       executing a registered database procedure.
**	    2. New function rqu_get_rpdata() reads a downstream GCA_RP_DATA 
**	       message.
**	    3. Rewrote rqu_gcmsg_type() to use a table instead of a big switch 
**	       statement. Also added missing GCA messages.
**	    4. Fixed rqu_gca_error() to recognize sqlstates that are returned 
**	       when a registered database procedure returns a message or raises 
**	       an error.
**	    5. Interoperatability issue WRT to DB_DEC_TYPE. When ending 
**	       DB_DATA_VALUES to LDBs, if LDB protocol does not support 
**	       DB_DEC_TYPE, convert DB_DEC_TYPE to B_FLT_TYPE.
**	    6. Fixed rqu_new_assoc() to try to establish an LDB connection to 
**	       LDB at the highest protocol level, GCA_PROTOCOL_LEVEL_60.
**	    7. Minor code cleanup. Shortened long lines, eliminated unused 
**	       variables.
**	19-jan-1993 (wolf)
**	    Fix a typo in rqu_mod_assoc(); tz_zec should be tz_sec
**	20-jan-1993 (wolf)
**	    And change TMtx_search() to TMtz_search()
**	05-feb-1993 (fpang)
**	    Added support for detecting commits/aborts when executing a 
**	    a procedure or in direct execute immediate.
**	8-apr-93 (robf)
**	    Updated protocol level to GCA_PROTOCOL_LEVEL_61 to handle agent
**	    connections.
**	12-apr-1993 (fpang)
**	    Added support for GCA1_DELETE and GCA1_INVPROC.
**	25-apr-1993 (fpang)
**	    Added support for intallation passwords.
**	    In rqu_cluster_info() - added function header comments and cleaned
**				  up code.
**	    In rqu_new_assoc() - stop requiring a password. The name server
**				  will always supply one. Also cleaned up
**				  the code.
**      19-may-1993 (fpang)
**          In rqu_mod_asoc(), pass the GCA_DECIMAL value in a char instead
**          of an i4.
**          Fixes B51918 (setting GCA_DECIMAL gets ldb connect errors.)
**	25-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	30-jun-1993 (shailaja)
**	    Fixed argument incompatibility in CSget_scb .
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-jul-1993 (fpang)
**	    In rqu_new_assoc(), declare the strings 'dbname' and 'dbms_type' 
**	    to have one more byte to accommodate the EOS. Also use MEmove() to 
**	    fill dd_co21_euser instead of STcopy() because dd_co21_euser 
**	    should not be EOS terminated.
**	    Also fixed up some prototype errors with proper coersion and 
**	    included some missing header files.
**       12-aug-1993 (stevet)
**          Added support for GCA_STRTRUNC to detect string truncation.
**	07-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	15-sep-1993 (fpang)
**	    In rqu_new_assoc(), recovered 'lost' fix for B42810 (AV when 
**	    restarting LDB's)..
**	16-Sep-1993 (fpang)
**	    Added support for -G (GCA_GRPID) and -R (GCA_APLID).
**	21-Sep-1993 (fpang)
**	    Fixed typo's from previous integration.
**      08-oct-1992 (fpang)
**          Fixed B54385, (Create ..w/journaling fails). Recognize
**          SS000000_SUCCESS as a successful status message.
**	11-Oct-1993 (fpang)
**	    Added support for -kf (GCA_DECFLOAT).
**	    Fixes B55510 (Star server doesn't support -kf flag).
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in rqu_gccall().
**	16-dec-1993 (fpang)
**	    Resuspend if we are resumed with gca_status E_GCFFFF_IN_PROCESS.
**	    Fixes B57764, Help Table in Direct Connect causes spurious Star 
**	    server SEGV's  
**	01-Fed-1994 (fpang)
**	    In rqu_gccall(), fixed compile error.
**	10-mar-94 (swm)
**	    Bug #59883
**	    Changed all RQTRACE calls to use newly-added rqs_trfmt format
**	    buffer and rqs_trsize in RQS_CB, and eliminated use automatically
**	    allocated buffers; this reduces the probability of stack overflow.
**      20-Apr-1994 (daveb) 33593
**          Always send gw_parms if proto > 2, even if INGRES LDB.
**  	    This sends the client info provided by libq as part of
**  	    the referenced bug fix to the ldb.
**	    allocated buffers; this reduces the probability of stack overflow.
**       25-May-1994 (heleny)
**          Bug46139: make sure the length for varchar is at least one.
**	25-jul-95 (canor01)
**	    Replaced GCA_SYNC_FLAG with 0 on NT to cure a hang in the
**	    GCN when the STAR server is started.
**	14-nov-1995 (nick)
**	    Add support for II_DATE_CENTURY_BOUNDARY / GCA_YEAR_CUTOFF
**      28-Oct-1996 (hanch04)
**          Changed SS50000_MISC_ING_ERRORS to SS50000_MISC_ERRORS
**	01-07-97 (sarjo01)
**	    Bug 76353: Removed 25-jul-95 (canor01) change. No longer needed
**	    and was causing memory leak due to un-removed associations.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**  29-may-98 (kitch01)
**		Bug 90540. If II_DATE_CENTURY_BOUNDARY is set on the client but the peer
**		does not support this flag via GCA subtract 1 from number of parms
**		(gca_l_user_data). This prevents the peer from reporting E_US0022 error.
**		Amended rqu_mod_assoc.
**	11-Aug-1998 (hanch04)
**	    Removed declaration of ule_format
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	20-jun-2001 (somsa01)
**	    In rqu_new_assoc(), when constructing the partner name as
**	    NODE::DATABASE/SERVER, properly place the EOS value in
**	    full_name so that STtrmwhite() will work.
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
**      05-Dec-2001 (hanal04) Bug 105723, INGSTR 42.
**          Copy precision details into the bind table in rqu_td_save().
**	12-apr-2002 (devjo01)
**	    Add cx.h so rqfprvt.h acn use CX_MAX_NODE_NAME_LEN.
**	22-Nov-2002 (hanal04) Bug 107159 INGSRV 1696
**	    Prevent spurious exceptions in GCA and associated GC routines
**          by ensuring GCA calls complete with either success or
**          failure before making further GCA calls.
**	30-nov-2005 (abbjo03)
**	    Remove globavalue SS$_DEBUG declarations which cause unexplained
**	    compilation errors.
**      09-jan-2009 (stial01)
**          Fix buffers that are dependent on DB_MAXNAME
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	 9-Oct-09 (gordy)
**	    Update GCA message list.
**      12-mar-2010 (joea)
**          In rqu_new_assoc, set gca_peer_protocol to GCA_PROTOCOL_LEVEL_68
**          if AD_BOOLEAN_PROTO bit set in adf_proto_level.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

GLOBALREF i4  ss5000g_dbproc_message;
GLOBALREF i4  ss50009_error_raised_in_dbproc;
GLOBALREF i4  ss00000_success;


/*{
** Name:  rqu_gcmsg_type - log GCA read/write info
**
** Description:
**
** 	log GCA_SEND/RECEIVE message type and association id
**
** Inputs:
**
** 	direction - GCA_SEND/GCA_RECEIVE
** 	typ - GCA message type
** 	id - association id
**
** Outputs:
**          none
**        
**	Returns:
** 		none
**
**	Exceptions:
**	    	none
**
** Side Effects:
**	    none
**
** History:
**      1-Jul-1988 (alexh)
**          created
**     31-Jan-1991 (fpang)
**	    Added support for GCA_EVENT.
**     13-Aug-1991 (fpang)
**	    Added support for GCA_INVPROC, GCA_DRPPROC, and GCA_CREPROC.
**     18-jan-1993 (fpang)
**	    Rewrote to use a table instead of a big switch statement.
**	    Also added missing messages.
**	12-apr-1993 (fpang)
**	    Added support for GCA1_DELETE and GCA1_INVPROC.
**	11-nov-93 (robf)
**          Added  support for GCA_PROMPT and GCA_PRREPLY 
**	 1-Oct-09 (gordy)
**	    Updated message list: GCA1_FETCH, GCA2_FETCH, GCA2_INVPROC.
*/
VOID
rqu_gcmsg_type(
	i4	direction,
	i4	typ,
	RQL_ASSOC *assoc)
{
    static char  *rqu_gcmsgtab[ GCA_MAXMSGTYPE + 2 ] =
        { "Internal", "Internal",
/*02-05*/ "GCA_MD_ASSOC", "GCA_ACCEPT",   "GCA_REJECT",	"GCA_RELEASE",
/*06-09*/ "GCA_Q__BTRAN", "GCA_S_BTRAN",  "GCA_ABORT",  "GCA_SECURE", 	
/*0A-0D*/ "GCA_DONE",  	  "GCA_REFUSE",   "GCA_COMMIT", "GCA_QUERY",  	
/*0E-11*/ "GCA_DEFINE",   "GCA_INVOKE",   "GCA_FETCH",  "GCA_DELETE",   
/*12-15*/ "GCA_CLOSE",    "GCA_ATTENTION","GCA_QC_ID",  "GCA_TDESCR",   
/*16-19*/ "GCA_TUPLES",   "GCA_C_INTO",   "GCA_C_FROM", "GCA_CDATA",  	
/*1A-1D*/ "GCA_ACK", 	  "GCA_RESPONSE", "GCA_ERROR",  "GCA_TRACE",  	
/*1E-21*/ "GCA_Q_ETRAN",  "GCA_S_ETRAN",  "GCA_IACK",   "GCA_NP_INTERRUPT", 
/*22-25*/ "GCA_ROLLBACK", "GCA_Q_STATUS", "GCA_CREPROC","GCA_DRPPROC",	
/*26-29*/ "GCA_INVPROC",  "GCA_RETPROC",  "GCA_EVENT",  "GCA1_C_INTO", 	
/*2A-2D*/ "GCA1_C_FROM",  "GCA1_DELETE",  "GCA_XA_SECURE", "GCA1_INVPROC",
/*2E-31*/ "GCA_PROMPT",   "GCA_PRREPLY",  "GCA1_FETCH", "GCA2_FETCH",
/*32-32*/ "GCA2_INVPROC", 
	  "GCA_UNKNMSGTYPE" };
    static char  *rqu_dirtab[ 2 ] = { "GCA_SEND", "GCA_RECEIVE" };

    RQS_CB  *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

    if (typ > GCA_MAXMSGTYPE)
	typ = GCA_MAXMSGTYPE + 1;
    
    RQTRACE(rqs,3)(RQFMT(rqs,
		   "RQF:Assn %d. %s, %s.\n"),
		   assoc->rql_assoc_id, 
		   rqu_dirtab[ direction - GCA_SEND ], rqu_gcmsgtab[ typ ]);
}

/*{
** Name: rqu_dbg_error - RQF GCA error debug hook
**
** Description:
**
**	This is a convenient hook of setting breaking point from debugger.
**	GCA errors are very abnormal status and often indicates a bug.
**
** Inputs:
**
**	erroe	-	GCA error
**
** Outputs:
**          none
**        
**	Returns:
**		none
**
**	Exceptions:
**	    none
**
** Side **	    none
**
** History:
**      1-Sep-1988 (alexh)
**	    created
**	04-feb-1992 (fpang)
**	    Removed tracing of GCN errors, they are no
**	    longer visible to mainline code.
*/
VOID
rqu_dbg_error(
	RQL_ASSOC  *assoc,
	i4	   error)
{

    RQS_CB *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

    if (! RQF_DBG_LVL(rqs,1))
	return;

    RQTRACE(rqs,1)(RQFMT(rqs,
    	"GCA reports error: "));
    switch( error )
    {
	case E_GC0001_ASSOC_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0001_ASSOC_FAIL"));
	    break;
	case E_GC0002_INV_PARM:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0002_INV_PARM"));
	    break;
	case E_GC0003_INV_SVC_CODE:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0003_INV_SVC_CODE"));
	    break;
	case E_GC0004_INV_PLIST_PTR:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0004_INV_PLIST_PTR"));
	    break;
    	case E_GC0005_INV_ASSOC_ID:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0005_INV_ASSOC_ID"));
	    break;
	case E_GC0006_DUP_INIT:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0006_DUP_INIT"));
	    break;
	case E_GC0007_NO_PREV_INIT:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0007_NO_PREV_INIT"));
	    break;
	case E_GC0008_INV_MSG_TYPE:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0008_INV_MSG_TYPE"));
	    break;
	case E_GC0009_INV_BUF_ADDR:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0009_INV_BUF_ADDR"));
	    break;
	case E_GC000A_INT_PROT_LVL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC000A_INT_PROT_LVL"));
	    break;
	case E_GC000B_RMT_LOGIN_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC000B_RMT_LOGIN_FAIL"));
	    break;
	case E_GC0010_BUF_TOO_SMALL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0010_BUF_TOO_SMALL"));
	    break;
	case E_GC0011_INV_CONTENTS:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0011_INV_CONTENTS"));
	    break;
	case E_GC0012_LISTEN_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0012_LISTEN_FAIL"));
	    break;
	case E_GC0013_ASSFL_MEM:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0013_ASSFL_MEM"));
	    break;
	case E_GC0014_SAVE_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0014_SAVE_FAIL"));
	    break;
	case E_GC0015_BAD_SAVE_NAME:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0015_BAD_SAVE_NAME"));
	    break;
	case E_GC0016_RESTORE_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0016_RESTORE_FAIL"));
	    break;
	case E_GC0017_RSTR_OPEN:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0017_RSTR_OPEN"));
	    break;
	case E_GC0018_RSTR_READ:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0018_RSTR_READ"));
	    break;
	case E_GC0019_RSTR_CLOSE:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0019_RSTR_CLOSE"));
	    break;
	case E_GC0020_TIME_OUT:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0020_TIME_OUT"));
	    break;
	case E_GC0021_NO_PARTNER:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0021_NO_PARTNER"));
	    break;
	case E_GC0022_NOT_IACK:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0022_NOT_IACK"));
	    break;
	case E_GC0023_ASSOC_RLSED:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0023_ASSOC_RLSED"));
	    break;
	case E_GC0024_DUP_REQUEST:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0024_DUP_REQUEST"));
	    break;
	case E_GC0025_NM_SRVR_ID_ERR:	
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0025_NM_SRVR_ID_ERR"));
	    break;
	case E_GC0026_NM_SRVR_ERR:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0026_NM_SRVR_ERR"));
	    break;
	case E_GC0027_RQST_PURGED:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0027_RQST_PURGED"));
	    break;
	case E_GC0028_SRVR_RESOURCE:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0028_SRVR_RESOURCE"));
	    break;
	case E_GC0029_RQST_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0029_RQST_FAIL"));
	    break;
	case E_GC002A_RQST_RESOURCE:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC002A_RQST_RESOURCE"));
	    break;
	case E_GC002B_SND1_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC002B_SND1_FAIL"));
	    break;
	case E_GC002C_SND2_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC002C_SND2_FAIL"));
	    break;
	case E_GC002D_SND3_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC002D_SND3_FAIL"));
	    break;
	case E_GC002E_RCV1_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC002E_RCV1_FAIL"));
	    break;
	case E_GC002F_RCV2_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC002F_RCV2_FAIL"));
	    break;
	case E_GC0030_RCV3_FAIL:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GC0030_RCV3_FAIL"));
	    break;
	case E_GCFFFF_IN_PROCESS:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"E_GCFFFF_IN_PROCESS"));
	    break;
	default:
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"UNKNOWN ERROR"));
	    break;
    }
    RQTRACE(rqs,1)(RQFMT(rqs,
			"   0x%x \n"), error);
}

/*{
** Name:  rqu_client_trace - send trace to STAR client
**
** Description:
**
**	send a ASCII trace message to client. This message is
**	actually queued by SCF. This is often used by RQF to relay
**	GCA_TRACE message.
**
** Inputs:
**
**	msg_length	message length
**	msg_buffer	message pointer
**
** Outputs:
**          none
**        
**	Returns:
**		none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-Oct-1988 (alexh)
**          created
*/
VOID
rqu_client_trace(
	i4	    msg_length,
	char        *msg_buffer)
{
  DB_STATUS   scf_status;
  SCF_CB      scf_cb;
  RQS_CB      *rqs = (RQS_CB *)NULL; /* For RQTRACE */

  scf_cb.scf_length = sizeof(scf_cb);
  scf_cb.scf_type = SCF_CB_TYPE;
  scf_cb.scf_facility = DB_QEF_ID;
  scf_cb.scf_len_union.scf_blength = (u_i4)msg_length;
  scf_cb.scf_ptr_union.scf_buffer = (PTR)msg_buffer;
  scf_cb.scf_session = DB_NOSESSION;	    /* print to current session */

  scf_status = scf_call(SCC_TRACE, &scf_cb);

  if (scf_status != E_DB_OK)
    {
        RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqu_client_trace. SCC_TRACE error %d\n"),
		    scf_cb.scf_error.err_code);
    }
}

/*{
** Name:  rqu_relay - relay a GCA_ERROR, GCA_EVENT or GCA_TRACE to STAR client
**
** Description:
**
**	A GCA_ERROR, GCA_EVENT or GCA_TRACE message is relayed to STAR client.
**	SCC_RELAY, which was written for STAR, is used to queue the
**	message. It can be enhanced to relay other types of messages.
**
** Inputs:
**
**	rv	a interpreted message block
**
** Outputs:
**          none
**        
**	Returns:
**		none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-Jan-1989 (alexh)
**          created
**	31-Jan=1991 (fpang)
**	    Fixed error message.
*/
VOID
rqu_relay(
	RQL_ASSOC	*assoc,
	GCA_RV_PARMS	*rv)
{
  DB_STATUS   scf_status;
  SCF_CB      scf_cb;
  RQS_CB      *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;
   static   bool	    test = 0;

    if (test)
	return;

  scf_cb.scf_length = sizeof(scf_cb);
  scf_cb.scf_type = SCF_CB_TYPE;
  scf_cb.scf_facility = DB_QEF_ID;
  scf_cb.scf_len_union.scf_blength = 
    rv->gca_d_length + (rv->gca_data_area-rv->gca_buffer);
  scf_cb.scf_ptr_union.scf_buffer = (PTR)rv;
  scf_cb.scf_session = DB_NOSESSION;	    /* print to current session */

  scf_status = scf_call(SCC_RELAY, &scf_cb);

  if (scf_status != E_DB_OK)
    {
        RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:rqu_relay. SCC_RELAY error %d\n"),
		    scf_cb.scf_error.err_code);
    }
}

/*{
** Name:  rqu_error - report error to STAR client
**
** Description:
**
**	Report severe error encountered by RQU to STAR client.
**	This is currently sent as a trace. Should be changed to
**	error.
**
** Inputs:
**
**	log_error_only  if this flag is set this routinr will log the error
**			only, if not it log and send to the user.
**	rqf_err		registered RQU error code
**	num_parms	number of parameters associated with the error.
**	os_error	operating system error.
**	p_len's, p's	address and length error parameters
**
** Outputs:
**          none
**        
**	Returns:
**		none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-Oct-1988 (alexh)
**          created
**	2-jun-1990  (georgeg)
**	    added parmameter num_parms
**	10-nov-1990 (georgeg)
**	    added lof_error parm, if TRUE rqu_error will only log the error 
*/
VOID
rqu_error(
	i4		log_error_only,
	i4		rqf_err,
	PTR		os_error,
	i4		num_parms,
	i4		p1_len,
	i4		*p1,
	i4		p2_len,
	i4		*p2,
	i4		p3_len,
	i4		*p3,
	i4		p4_len,
	i4		*p4)
{
    

    i4	    msglen;
    i4	    ulecode;
    char	    errbuf[DB_ERR_SIZE];
    DB_STATUS	    scf_status, status;
    SCF_CB	    scf_cb;
    RQS_CB	    *rqs = (RQS_CB *)NULL; /* For RQTRACE */


    if ((num_parms < 0) || (num_parms >4))
    {
	num_parms=0;
    }

    if ( p1_len <= (i4)0 )
    {
        p1_len = (i4)0;
        p1 = (i4 *)NULL;
    }

    if ( p2_len <= (i4)0 )
    {
        p2_len = (i4)0;
        p2 = (i4 *)NULL;
    }

    if ( p3_len <= (i4)0 )
    {
        p3_len = (i4)0;
        p3 = (i4 *)NULL;
    }

    if ( p4_len <= (i4)0 )
    {
        p4_len = (i4)0;
        p4 = (i4 *)NULL;
    }
      
    status = ule_format(rqf_err, (CL_ERR_DESC *) os_error, ULE_LOG,
		 &scf_cb.scf_aux_union.scf_sqlstate,
		 errbuf, (i4)sizeof(errbuf),
		 &msglen, &ulecode, num_parms,
		 p1_len, p1, p2_len, p2, p3_len, p3, p4_len, p4);
    if (status != E_DB_OK)
    {
	char	    *misc_err_sqlstate = SS50000_MISC_ERRORS;
	i4	    i;
	
	/*
	** The message could not be formated 
	*/
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqu_error. Ule_format error: %d(0x%x)\n"), 
		    rqf_err, rqf_err);
    	STprintf(errbuf, "Star could not lookup error message <0x%x>", rqf_err);
	msglen = (i4)STlength(errbuf);

	for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
	    scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate[i] =
		misc_err_sqlstate[i];
    } 

    if (log_error_only)
    {
	return;
    }
    /*
    ** send to client 
    */
    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_len_union.scf_blength = (u_i4)msglen;
    scf_cb.scf_ptr_union.scf_buffer = (PTR)errbuf;
    scf_cb.scf_nbr_union.scf_local_error = rqf_err;
    scf_cb.scf_session = DB_NOSESSION;	/* print to current session */
    scf_status = scf_call(SCC_ERROR, &scf_cb);

    if (scf_status != E_DB_OK)
    {
        RQTRACE(rqs,1)(RQFMT(rqs,
		   "RQF:rqu_error. SCC_ERROR error %d\n"),
		   scf_cb.scf_error.err_code);
    }
}
/*{
** Name:  rqu_gca_error - report error to STAR client
**
** Description:
**
**	Report severe error reported by the LDB to the FE.
**	This function will look up the error only when nessaccery to pick up
**	the generic error, it reports whatever
**	GCA_ERROR block contains. It will also put out the sourse of the error.
** Inputs:
**
**	assoc		the current association.
**	gca_error_block the error as delivered by GCF
**
** Outputs:
**          none
**        
**	Returns:
**		none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-mar-1989 (georgeg)
**          created from rqu_error
**	30-may-1990 (georgeg)
**	    Fixed bug 20697, if the local is sending a message or a 
**	    user raised error echo it as is do not alter the severity.
**	18-sep-1991 (fpang)
**	    Log RQ004A instead of RQ0042 to the error log.
**	    RQ0042 is confusing in the error log because the local does not
**	    always log errors. RQ004A will contain the local error and make
**	    things less confusing. Fixes B39727.
**	06-nov-1991 (fpang)
**	    Don't output the 'Preceding message..' message if the message is 
**	    a GCA_MESSAGE.
**	    This fixes B39443 and B40051.
**	18-jan-1993 (fpang)
**	    If LDB returns sqlstates, look for the 'special' states that are
**	    returned when database procedures return messages and raise
**	    errors.
**      08-oct-1992 (fpang)
**          Fixed B54385, (Create ..w/journaling fails). Recognize
**          SS000000_SUCCESS as a successful status message.
*/
VOID
rqu_gca_error(
	RQL_ASSOC	*assoc,
	GCA_ER_DATA	*gca_error_blk)
{
    SCF_CB	    scf_cb;
    i4	    i = gca_error_blk->gca_l_e_element;
    GCA_E_ELEMENT   *element = gca_error_blk->gca_e_element;
    i4	    operation, error_severity,
		    msglen, ulecode, generr;
    char	    errbuf[DB_ERR_SIZE];
    DB_STATUS	    status;
    i4	    p1_len, p2_len, p3_len;
    PTR		    p1, p2, p3;
    RQS_CB	    *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

    if ( (gca_error_blk->gca_l_e_element <= 0) || 
	  (gca_error_blk->gca_l_e_element >20))
    {
        RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. GCA reported gca_l_e_element %d\n"),
		   assoc->rql_assoc_id, gca_error_blk->gca_l_e_element);
	return;
    }

    if (
	(element->gca_error_parm[0].gca_l_value > DB_ERR_SIZE) 
	|| 
	(element->gca_error_parm[0].gca_l_value < 1)
       )
    {
	/*
	** There is a problem currently with GCA errors, sometimes the
	**  size of the error buffer has an false length.
	*/
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. Enormous gca_l_value (%d) with GCA_ERROR\n"), 
		 assoc->rql_assoc_id, element->gca_error_parm[0].gca_l_value);
	return;
    }

    /* 
    ** Find out how to format the error.
    */
    error_severity = element->gca_severity;
    if (error_severity == GCA_ERDEFAULT)
    {
	operation = SCC_ERROR;
    }
    else if (error_severity & GCA_ERFORMATTED)
    {
	operation = SCC_RSERROR;
    }
    else if (error_severity & GCA_ERMESSAGE)
    {
	operation = SCC_MESSAGE;
    }
    else if (error_severity & GCA_ERWARNING)
    {
	operation = SCC_WARNING;
    }
    else
    {
	operation = SCC_ERROR;
    }

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_QEF_ID; 
    scf_cb.scf_len_union.scf_blength = 
 	    (u_i4)element->gca_error_parm[0].gca_l_value;
    scf_cb.scf_ptr_union.scf_buffer = 
	    element->gca_error_parm[0].gca_value;
    /*
    ** If the is already formated/GCA_ERMESSAGE just echo it.
    */

    /*
    ** if the LDB server does not "know" about SQLSTATEs, look up the SQLSTATE
    ** status code corresponding to this error message
    */
    if (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_60)
    {
	if (assoc->rql_peer_protocol <= GCA_PROTOCOL_LEVEL_2)
	    scf_cb.scf_nbr_union.scf_local_error = element->gca_id_error;
	else
	    scf_cb.scf_nbr_union.scf_local_error = element->gca_local_error;

	status = ule_format(scf_cb.scf_nbr_union.scf_local_error, NULL,
			    ULE_LOG, &scf_cb.scf_aux_union.scf_sqlstate,
			    errbuf, (i4)sizeof(errbuf),  
			    &msglen, &ulecode, (i4)12,
			    0, NULL, 0, NULL,
			    0, NULL, 0, NULL,
			    0, NULL, 0, NULL,
			    0, NULL, 0, NULL,
			    0, NULL, 0, NULL,
			    0, NULL, 0, NULL
			   );
	if (status != E_DB_OK)
	{
	    char    *misc_err_sqlstate = SS50000_MISC_ERRORS;
	    i4	    i;

	    for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
		scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate[i] =
		    misc_err_sqlstate[i];
	}
    }
    else
    {
	scf_cb.scf_nbr_union.scf_local_error = element->gca_local_error;

	/* decode sqlstate and store the result into SCF_CB */
	ss_decode(scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate,
	    element->gca_id_error);
    }

    if (assoc->rql_noerror == FALSE)
    {
	scf_cb.scf_session = DB_NOSESSION;	/* print to current session */
	status = scf_call(operation, &scf_cb);
	if (status != E_DB_OK)
	{
	    RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. SCC_ERROR error %d.\n"),
		        assoc->rql_assoc_id, scf_cb.scf_error.err_code);
	    return;
	}
	else
	{
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. Received LDB: count = %d\n"),
			assoc->rql_assoc_id, i);
	    RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. LDB error msg: %t\n"),
			assoc->rql_assoc_id,
		        element->gca_error_parm[0].gca_l_value,
		        element->gca_error_parm[0].gca_value);
	}
    }
    /*
    ** If it is a message, don't ouput the 'preceeding message..' stuff.
    **
    ** Peer protocol < 60
    **	    generic error (gca_id_error)  GE_OK 
    **  and local error (gca_local_error) 0
    **
    ** Peer protocol >= 60
    **	    sqlstate (gca_id_error) SS5000G_DBPROC_MESSAGE
    **   or sqlstate (gca_id_error) SS50009_ERROR_RAISED_IN_DBPROC
    **   or sqlstate (gca_id_error) SS00000_SUCCESS
    */

    if ( ((assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_60) && 
          (element->gca_id_error == GE_OK) && (element->gca_local_error == 0))
        ||
	 ((assoc->rql_peer_protocol >= GCA_PROTOCOL_LEVEL_60)  &&
	  (   (element->gca_id_error == ss5000g_dbproc_message)
	   || (element->gca_id_error == ss50009_error_raised_in_dbproc)
	   || (element->gca_id_error == ss00000_success)) ) )
	return ;

    /*
    ** Tell the FE whitch LDB is sending the error.
    ** We need to do all this just in case the LDB is iidbdb in which
    ** case there is no node name and dbms name.
    */
    p1_len = (i4)
		((char *)STindex(assoc->rql_lid.dd_l3_ldb_name,
				 " ",
				 sizeof(assoc->rql_lid.dd_l3_ldb_name)) - 
		    (char *)assoc->rql_lid.dd_l3_ldb_name);
    if (p1_len <= 0)
    {
	p1_len = 0;
	p1 = (PTR)NULL;
    }
    else
    {
    	p1 = assoc->rql_lid.dd_l3_ldb_name;
    }
    
    p2_len = (i4)
		((char *)STindex(assoc->rql_lid.dd_l2_node_name,
				 " ",
				 sizeof(assoc->rql_lid.dd_l2_node_name)) - 
		 (char *)assoc->rql_lid.dd_l2_node_name);

    if (p2_len <= 0)
    {
	p2_len = 0;
	p2 = (PTR)NULL;
    }
    else
    {
    	p2 = (char *)assoc->rql_lid.dd_l2_node_name;
    }

    p3_len = (i4)
		((char *)STindex(assoc->rql_lid.dd_l4_dbms_name,
				 " ",
				 sizeof(assoc->rql_lid.dd_l4_dbms_name)) - 
		 (char *)assoc->rql_lid.dd_l4_dbms_name);
    if (p3_len <= 0)
    {
	p3_len = 0;
	p3 = (PTR)NULL;
    }
    else
    {
    	p3 = (PTR)assoc->rql_lid.dd_l4_dbms_name;
    }

    
    status = ule_format(E_RQ0042_LDB_ERROR_MSG,
			NULL,
			0,
			&scf_cb.scf_aux_union.scf_sqlstate,
			errbuf,
			(i4)sizeof(errbuf),
			&msglen,
			&ulecode,
			3,
			(i4)p1_len, (PTR)p1,
			(i4)p2_len, (PTR)p2,
			(i4)p3_len, (PTR)p3 
	    );
    if (status != E_DB_OK)
    {
	char    *misc_err_sqlstate = SS50000_MISC_ERRORS;
	i4	    i;

	/*
	** The message could not be formated 
	*/
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. Ule_format error: %d(0x:%x)\n"),
		    assoc->rql_assoc_id,
		    E_RQ0042_LDB_ERROR_MSG, E_RQ0042_LDB_ERROR_MSG );
    	STprintf(errbuf, "Star could not lookup error message <0x%x>",
		    E_RQ0042_LDB_ERROR_MSG);
	msglen = (i4)STlength(errbuf);
	scf_cb.scf_nbr_union.scf_local_error = E_RQ0042_LDB_ERROR_MSG;

	for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
	    scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate[i] =
		misc_err_sqlstate[i];
    } 

    /*
    ** send to client 
    */
    if (assoc->rql_noerror == FALSE)
    {
	scf_cb.scf_len_union.scf_blength = (u_i4)msglen;
	scf_cb.scf_ptr_union.scf_buffer = (PTR)errbuf;
	status = scf_call(SCC_WARNING, &scf_cb);
    }
    if (status != E_DB_OK)
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:Assn %d. SCC_ERROR error %d.\n"),
		    assoc->rql_assoc_id, scf_cb.scf_error.err_code);
    }

    /*
    ** call ss_2_ge to derive the generic error based on sqlstate and possibly
    ** the DBMS error
    */
    generr = ss_2_ge(scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate,
	scf_cb.scf_nbr_union.scf_local_error);
	
    /*
    ** Log the message to the error log. Ignore status.
    */

    status = ule_format(E_RQ004A_LDB_ERROR_MSG, NULL, ULE_LOG,
		      (DB_SQLSTATE *) NULL,
		      errbuf, (i4)sizeof(errbuf),  &msglen, &ulecode, 5,
		      (i4)sizeof(generr), (PTR)&generr,
		      (i4)sizeof(scf_cb.scf_nbr_union.scf_local_error),
		      (PTR)&scf_cb.scf_nbr_union.scf_local_error,
		      (i4)p1_len, (PTR)p1,
		      (i4)p2_len, (PTR)p2,
		      (i4)p3_len, (PTR)p3);
}

/*{
** Name: rqu_find_lid - search a lid in a association list
**
** Description:
**         search ther association list for the association with the
**         specified lid. If no such association is found, NULL is returned.
**	   Note that dd_l1_ingres_b is ignored
** Inputs:
**      site              target lid
**      list              association list
**
** Outputs:
**          none
**        
**	Returns:
**	    RQL_ASSOC * 	with rql_lid.dd_ldb_id == site's and
**				rql_lid.dd_l1_ingres_b == site->; or
**				if site's ldb_id is 0 then the description
**				struct must be identical.
**          			NULL if site is not found in list.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-jun-1988 (alexh)
**          created
**	21-jul-1988 (alexh)
**	    modify site comparison to include dd_l1_ingres_b flag.
**	15-oct-1992 (fpang)
**	    Removed recursion.
*/
RQL_ASSOC *
rqu_find_lid(
	DD_LDB_DESC     *site,
	RQL_ASSOC  	*list)
{

    while (list != NULL)
    {
	/*
	** nested ifs are used for easy debugging. It is not required. 
	*/
	if (list->rql_lid.dd_l1_ingres_b == site->dd_l1_ingres_b)
	    if (MEcmp(site->dd_l3_ldb_name, list->rql_lid.dd_l3_ldb_name,
		      sizeof(site->dd_l3_ldb_name)) == 0)
		if (MEcmp(site->dd_l2_node_name, list->rql_lid.dd_l2_node_name,
			  sizeof(site->dd_l2_node_name)) == 0)
		    if (MEcmp(site->dd_l4_dbms_name, 
			      list->rql_lid.dd_l4_dbms_name,
			      sizeof(site->dd_l4_dbms_name)) == 0)
			if ((list->rql_lid.dd_l6_flags & DD_02FLAG_2PC) ==
			    (site->dd_l6_flags & DD_02FLAG_2PC))
			{
			    break;
			}
	list = list->rql_next;
    }

    return(list);
}

/*{
** Name: rqu_send - send a message
**
** Description:
**         send a GCA message via GCA. The message to be sent has been
**	stored in the preformated send buffer of the association.
** Inputs:
**      assoc	association
**	msg_len		length of the message
**	msg_cont	message continuation
**	msg_type	GCA message type
**
** Outputs:
**      E_RQ0000_OK
**	E_RQ0012_INVALID_WRITE
**        
**	Returns:
**          none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-jun-1988 (alexh)
**          created
**      1-aug-1989 (georgeg)
**          added checks to keep trace of the association state.
*/
DB_ERRTYPE
rqu_send(
	RQL_ASSOC	*assoc,
	i4		msg_size,
	i4		msg_cont,
	i4		msg_type)
{
    GCA_SD_PARMS    *send = &assoc->rql_sd;
    STATUS	    status;
    RQS_CB	    *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

    if (assoc->rql_status != RQL_OK)
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:Assn %d. Rqu_send, invalid WRITE. "),
		    assoc->rql_assoc_id);
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "rqc_rxtx=0x%x, rqc_status=0x%x, rql_status=0x%x\n"),
		    assoc->rql_comm.rqc_rxtx,  
		    assoc->rql_comm.rqc_status, 
		    assoc->rql_status);
		
	return(E_RQ0012_INVALID_WRITE);
    }
    send->gca_message_type = msg_type;
    send->gca_msg_length = msg_size;
    send->gca_end_of_data = msg_cont;
    rqu_gcmsg_type(GCA_SEND, send->gca_message_type, assoc);

    status = rqu_gccall(GCA_SEND, (PTR)send, &send->gca_status, assoc);
    if (status == E_DB_OK)
    {
	return(E_RQ0000_OK);
    }
    else if (assoc->rql_comm.rqc_status == RQC_INTERRUPTED)
    {
	assoc->rql_status = RQL_OK;
	assoc->rql_interrupted = TRUE;
    }
    else if (assoc->rql_comm.rqc_status == RQC_TIMEDOUT)
    {
	assoc->rql_status = RQL_TIMEDOUT;
	assoc->rql_interrupted = TRUE;
    }
    else
    {
	assoc->rql_status = RQL_TERMINATED;
    }
    return(E_RQ0012_INVALID_WRITE);
}

/*{
** Name: rqu_release_assoc - release association
**
** Description:
**         release and disassociate the specified association
** Inputs:
**      assoc_id	association id
**
** Outputs:
**          none
**        
**	Returns:
**          none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-jun-1988 (alexh)
**          created
**     15-oct-1992 (fpang)
**	    Don't have to free rql_partner_name anymore.
*/
VOID
rqu_release_assoc(
	RQL_ASSOC	*assoc)
{
    GCA_ER_DATA		*er_data = (GCA_ER_DATA *)assoc->rql_sd.gca_buffer;
    GCA_DA_PARMS	disassoc_parm;
    STATUS		status;
    RQS_CB	        *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

    if ((assoc == NULL))
    {
	return;
    }

    er_data->gca_l_e_element = 0;	/* just to keep someone happy */

    assoc->rql_timeout = 900;
    if ( (assoc->rql_status == RQL_OK) && (assoc->rql_comm.rqc_status == RQC_OK) )
    {
	(VOID)rqu_send(assoc, (i4)sizeof(er_data->gca_l_e_element), 
		 TRUE, GCA_RELEASE);
    }
    if (assoc->rql_status == RQL_TIMEDOUT )
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:Assn %d. RELEASE request timed out.\n"),
	            assoc->rql_assoc_id);

    }

    if (assoc->rql_comm.rqc_status == RQC_DISASSOCIATED)
    {
	/*
	** The association has already been closed.
	*/
	assoc->rql_status = RQL_TERMINATED;
	return;
    } 

    MEfill(sizeof(GCA_DA_PARMS), 0, (PTR)&disassoc_parm);
    disassoc_parm.gca_association_id = assoc->rql_assoc_id;
    (VOID)IIGCa_call(GCA_DISASSOC, (GCA_PARMLIST *)&disassoc_parm, 
		     (i4)GCA_SYNC_FLAG, (PTR)0, (i4)-1, &status);

    if ( disassoc_parm.gca_status != E_GC0000_OK )
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		   	 "RQF:Assn %d. "), assoc->rql_assoc_id);
        (VOID)rqu_dbg_error(assoc,disassoc_parm.gca_status);
        RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. RELEASE request refused! GCA Status: 0x%x\n"),
		    assoc->rql_assoc_id, disassoc_parm.gca_status);
    }
    else
    {
        RQTRACE(rqs,2)(RQFMT(rqs,
		    "RQF:Assn %d. DISASSOCIATION complete.\n"),
		    assoc->rql_assoc_id);
    }	

    assoc->rql_status = RQL_TERMINATED;
    assoc->rql_comm.rqc_status = RQC_DISASSOCIATED;
}

/*{
** Name: rqu_end_all_assoc - release a list of associations
**
** Description:
**         release the list of associations
** Inputs:
**      assoc_id	association id
**
** Outputs:
**          none
**        
**	Returns:
**          none
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
**	    Removed recursion.
*/
VOID
rqu_end_all_assoc(
	RQR_CB		*rcb,
	RQL_ASSOC	*assoc)
{
    while (assoc != NULL)
    {
	assoc->rql_rqr_cb = rcb;
	rqu_release_assoc(assoc);
	assoc = assoc->rql_next;
    }
}

/*{
** Name: rqu_prepare_assoc	- prepare association buffers
**
** Description:
**      prepare association buffers for future write for an
**	association - SD_PARMS & RV_PARMS are initialized.
** Inputs:
**
**	assoc	-	association token
**
** Outputs:
**          none
**        
**	Returns:
**          none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-jun-1988 (alexh)
**          created
*/
static VOID
rqu_prepare_assoc(
	RQL_ASSOC	*assoc)
{
  /* format send buffer for posterity */
  assoc->rql_sd.gca_association_id = assoc->rql_assoc_id;
  assoc->rql_sd.gca_buffer = assoc->rql_write_buff;
  assoc->rql_sd.gca_descriptor = NULL;
  assoc->rql_sd.gca_modifiers = 0;

  assoc->rql_rv.gca_association_id = assoc->rql_assoc_id;
  assoc->rql_rv.gca_flow_type_indicator = GCA_NORMAL;
  assoc->rql_rv.gca_buffer = assoc->rql_read_buff;
  assoc->rql_rv.gca_b_length = sizeof(assoc->rql_read_buff);
  assoc->rql_rv.gca_descriptor = NULL;
  assoc->rql_rv.gca_modifiers = 0;
}

/*{
** Name: rqu_dump_gca_msg	- dump a GCA error message
**
** Description:
**	dump GCA message that contains GCA_ER_DATA object.
**	Currently, only GCA_RELEASe and GCA_ERROR are dumped.
**	Right now this routine assumes error message is always
**	a char string data. This should be extended in the
**	future.
** Inputs:
**
**	assoc	-	association 
**
** Outputs:
**          none
**        
**	Returns:
**          none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-jul-1988 (alexh)
**          created
*/
VOID	
rqu_dump_gca_msg(
	RQL_ASSOC	*assoc)
{

  GCA_RV_PARMS	*rv = &assoc->rql_rv;
  RQS_CB	*rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

  if (! RQF_DBG_LVL(rqs,1))
     return;

  switch (rv->gca_message_type)
  {
    case GCA_ERROR:
    case GCA_REJECT:
    case GCA_RELEASE:
    {
	GCA_ER_DATA	*release = (GCA_ER_DATA *)rv->gca_data_area;
	i4		i = release->gca_l_e_element;
	GCA_E_ELEMENT	*element = release->gca_e_element;

	if (i > 0)
	{
	    if ( (element->gca_error_parm[0].gca_l_value > 512) 
		    || 
		  (element->gca_error_parm[0].gca_l_value < 1)
		)
	    {
		/*
		** There is a problem currently with GCA errors, sometimes the
		**  size of the error buffer has an false length.
		*/
		RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. GCA_ERROR gca_l_value (%d).\n"),
			    assoc->rql_assoc_id,
		            element->gca_error_parm[0].gca_l_value );
	    }
	    else
	    {
	        RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. LDB error reply, count = %d\n"), 
			    assoc->rql_assoc_id, i);
	        RQTRACE(rqs,1)(RQFMT(rqs,
			    "RQF:Assn %d. LDB error msg: %t\n"),
			    assoc->rql_assoc_id,
			    element->gca_error_parm[0].gca_l_value,
			    element->gca_error_parm[0].gca_value);
	    }
	}
	break;
    } 
    case GCA_TRACE:
    {
	/* GCA_TR_DATA contents are not defined in GCA document */
    }
      break;
    default:
      break;
  }
}

/*{
** Name:  rqu_putinit - initialize a put stream
**
** Description:
**
**	Initialize a put stream. A stream concept isnecessary because
**	GCA's restriction on the handling of atomic objects. Also see
**	rqu_put_data, rqu_putflush.
**
** Inputs:
**
**	assoc		association of the message stream
**	msg_type	message type of the stream
**
** Outputs:
**          none
**        
**	Returns:
**		none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-Oct-1988 (alexh)
**          created
*/
VOID
rqu_putinit(
	RQL_ASSOC	*assoc,
	i4		msg_type)
{
  GCA_SD_PARMS	*send = &assoc->rql_sd;

  send->gca_message_type = msg_type;
  send->gca_msg_length = 0;
}

/*{
** Name:  rqu_putflush - flush a message stream
**
** Description:
**
**	Send out the assoc write buffer. Aslo see rqu_putinit and
**	rqu_put_data.
**
** Inputs:
**
**	assoc		association of the message stream
**	eod		end_of_data indicator
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-Oct-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqu_putflush(
	RQL_ASSOC	*assoc,
	bool		eod)
{
  GCA_SD_PARMS	*send = &assoc->rql_sd;
  DB_ERRTYPE	status;
  status = rqu_send(assoc, send->gca_msg_length, eod, 
		    send->gca_message_type);
  rqu_putinit(assoc, send->gca_message_type);
  return(status);
}

/*{
** Name: rqu_put_data	- put data value into GCA data area
**
** Description:
**	This is a utility routine for puting GCa atomic object to
**	a GCA message stream. A rqu_putinit should first be called
**	on this association. rqu_putflush should be called when put
**	data activities end. When current assoc message buffer is
**	full, the message is sent with end_of_data FALSE, and the 
**	message is re-initialized by calling rqu_putinit again.
**	
**	Atomic object cannot be chopped. But char string can be.
**	
** Inputs:
**	assoc	association of the message stream
**	data	pointer to the atomic object
**	len	size of the atomic object
**
** Outputs:
**      none
**        
**	Returns:
**	    DB_ERRTYPE		error related to sending a message
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-jun-1988 (alexh)
**          created
*	15-oct-1992 (fpang)
**	    Removed recursion.
*/
DB_ERRTYPE
rqu_put_data(
	RQL_ASSOC	*assoc,
	char		*data,
	i4		len)
{
    GCA_SD_PARMS	*send = &assoc->rql_sd;
    DB_ERRTYPE		status = E_RQ0000_OK;
    i4			space_avail;
    char		*space_ptr;
    i4			amt;

    do
    {
        space_avail = sizeof(assoc->rql_write_buff) - send->gca_msg_length;
        space_ptr   = send->gca_buffer + send->gca_msg_length;

	amt = (len > space_avail) ? space_avail : len;

	/* If there is not enough space left in the send buffer, but
	** the message can fit in a message buffer by itself, then
	** send the current buffer, and come back to get the data.
	** If message is larger than a message buffer, then fill 
	** the current buffer from the data, then come back for more data.
	*/

	if ((len > space_avail) && (len <= sizeof(assoc->rql_write_buff)))
	    amt = 0;

	if (amt)
	{
	    MEcopy((PTR)data, amt, (PTR)space_ptr);
            send->gca_msg_length += amt;
	    data += amt;
	    len  -= amt;
	}
	    
	if (len == 0)
	    break;

	status = rqu_putflush(assoc, FALSE);

    } while ((len != 0) && (status == E_RQ0000_OK));
    return(status);
}

/*{
** Name:  rqu_put_qtxt - put a GCA_QTXT to a message stream
**
** Description:
**
**	This is a utility routine to put a GCA_QTXT to a stream.
**	The source query text will consist of a ddpacket linked list
**	each containing a fragment of the query. This routine will
**	ldb message stream.
**
** Inputs:
**
**	assoc		association of the message stream
**	ddpkt		DD_PACKET list
**	tmpnm		temporary table names
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-Nov-1992 (fpang)
**          Created
*/
DB_ERRTYPE
rqu_put_qtxt(
	RQL_ASSOC	*assoc,
	i4		msgtype,
	i4		qlanguage,
	i4		qmodifier,
	DD_PACKET	*ddpkt,
	DD_TAB_NAME	*tmpnm)
{
    DB_ERRTYPE	   status = E_RQ0000_OK;
    RQS_CB	   *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;
    GCA_DATA_VALUE gdv;
    DD_PACKET	   *pkt;
    char	   *data_ptr;

    gdv.gca_type      = DB_QTXT_TYPE;
    gdv.gca_precision = 0;
    gdv.gca_l_value   = 0;
    
    for (pkt = ddpkt; pkt != NULL; pkt = pkt->dd_p3_nxt_p)
	gdv.gca_l_value += 
		((pkt->dd_p2_pkt_p) ? pkt->dd_p1_len : sizeof(DD_TAB_NAME));

    do
    {
    	rqu_putinit(assoc, msgtype);

    	if ((status = rqu_put_data(assoc, 
		(PTR)&qlanguage, sizeof(qlanguage))) != E_RQ0000_OK)
	    break;
    	if ((status = rqu_put_data(assoc, 
		(PTR)&qmodifier, sizeof(qmodifier))) != E_RQ0000_OK)
	    break;
	if ((status = rqu_put_data(assoc, (PTR)&gdv.gca_type, 
		sizeof(gdv.gca_type))) != E_RQ0000_OK)
	    break;
        if ((status = rqu_put_data(assoc, (PTR)&gdv.gca_precision, 
		sizeof(gdv.gca_precision))) != E_RQ0000_OK)
	    break;
        if ((status = rqu_put_data(assoc, (PTR)&gdv.gca_l_value, 
		sizeof(gdv.gca_l_value))) != E_RQ0000_OK)
	    break;
    } while (0);

    for (pkt = ddpkt; 
	 (status == E_RQ0000_OK) && (pkt != NULL);
	 pkt = pkt->dd_p3_nxt_p)
    {
	if (pkt->dd_p2_pkt_p)
	{
	    gdv.gca_l_value = pkt->dd_p1_len;
	    data_ptr = pkt->dd_p2_pkt_p;
	}
	else
	{
	    gdv.gca_l_value = sizeof(DD_TAB_NAME);
	    data_ptr = (char *)(tmpnm + pkt->dd_p4_slot);
	}
	status = rqu_put_data(assoc, data_ptr, gdv.gca_l_value);
        rqu_dmp_dbdv(rqs, assoc, 
		     gdv.gca_type, gdv.gca_precision, gdv.gca_l_value, 
		     data_ptr);
    }
    return(status);
}

/*{
** Name:  rqu_put_parms - put an array of DB_DATA_VALUES to a message stream
**
** Description:
**
**	This is a utility routine to put an array of DB_DATA_VALUES to a stream.
**	Each DB_DATA_VALUE is put out independently.
**
** Inputs:
**
**	assoc		association of the message stream
**	count		number of DB_DATA_VALUES to output
**	dv		Array of DB_DATA_VALUES
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-Nov-1992 (fpang)
**          Created
*/
DB_ERRTYPE
rqu_put_parms(
	RQL_ASSOC	*assoc,
	i4		count,
	DB_DATA_VALUE	*dv)
{
    DB_ERRTYPE	   status = E_RQ0000_OK;
    for (; count && status == E_RQ0000_OK; --count, ++dv)
    {
	status = rqu_putdb_val(dv->db_datatype, dv->db_prec, dv->db_length,
			       dv->db_data, assoc);
    }
    return (status);
}

/*{
** Name:  rqu_put_iparms - put an array of pointers to DB_DATA_VALUES 
**
** Description:
**
**	This is a utility routine to put an array of pointers to DB_DATA_VALUES 
**	to a stream. Each DB_DATA_VALUE is put out independently.
**
** Inputs:
**
**	assoc		association of the message stream
**	count		number of DB_DATA_VALUES to output
**	dv		Array of pointers to DB_DATA_VALUES
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-Nov-1992 (fpang)
**          Created
*/
DB_ERRTYPE
rqu_put_iparms(
	RQL_ASSOC	*assoc,
	i4		count,
	DB_DATA_VALUE	**dv)
{
    DB_ERRTYPE	   status = E_RQ0000_OK;
    for (; count && status == E_RQ0000_OK; --count, ++dv)
    {
	status = rqu_putdb_val((*dv)->db_datatype, (*dv)->db_prec, 
			       (*dv)->db_length, (*dv)->db_data, assoc);
    }
    return (status);
}

/*{
** Name:  rqu_put_pparms - put procedure parameters to the output stream.
**
** Description:
**
**	This is a utility routine to put an array of pointers to procedure
**	parameters to an output stream. Each parameter will be output as a
**	GCA_P_PARAM, which is a length, followed by a name, followed by a mask,
**	followed by a GCA_DATA_VALUE.  The length is the length of the next
**	field, name. The name is the procedure name. The mask is the parameter
**	modification mask, and the GCA_DATA_VALUE is the parameter's value.
**
** Inputs:
**
**	assoc		association of the message stream
**	count		number of procedure parameters to output 
**	prms		Array of pointers to QEF_USR_PARMs
**
** Outputs:
**          none
**        
**	Returns:
**	    DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-Dec-1992 (fpang)
**          Created
*/
DB_ERRTYPE
rqu_put_pparms(
	RQL_ASSOC	*assoc,
	i4		count,
	QEF_USR_PARAM	**prms)
{
    DB_ERRTYPE	   status = E_RQ0000_OK;
    GCA_P_PARAM	   *pp;
    RQS_CB	   *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

    for (; count ; --count, ++prms)
    {
	/* i4, gca_p_param.gca_parname.gca_l_name */
    	status = rqu_put_data(assoc, 
		  (PTR)&(*prms)->parm_nlen, sizeof(pp->gca_parname.gca_l_name));
	if (status != E_RQ0000_OK)
	    break;

	/* char[1], gca_p_param.gca_parname.gca_name */
    	status = rqu_put_data(assoc, 
		  (PTR)(*prms)->parm_name, (*prms)->parm_nlen);
	if (status != E_RQ0000_OK)
	    break;

	/* i4, gca_p_param.gca_parm_mask */
    	status = rqu_put_data(assoc, 
		  (PTR)&(*prms)->parm_flags, sizeof(pp->gca_parm_mask));
	if (status != E_RQ0000_OK)
	    break;

    	RQTRACE(rqs,4)(RQFMT(rqs,
		"RQF:Assn %d. GCA_P_PARAM l_name=%d, name=%t, mask=%x\n"),
			assoc->rql_assoc_id,
			(*prms)->parm_nlen,
			(*prms)->parm_nlen, (*prms)->parm_name,
			(*prms)->parm_flags);

	/* GCA_DATA_VALUE, gca_p_param.gca_parvalue */
	status = rqu_putdb_val((*prms)->parm_dbv.db_datatype, 
		       (*prms)->parm_dbv.db_prec, (*prms)->parm_dbv.db_length,
		       (*prms)->parm_dbv.db_data, assoc);
	if (status != E_RQ0000_OK)
	    break;
    }
    return (status);
}

/*{
** Name:  rqu_put_moddata - put a modify association parameter to the LDB.
**			    Each call represents on modify value, and is 
**			    composed of an index and a DB_DATA_VALUE.
**
** Description:
**
**	This is a utility routine to put modify association parameter to the
**	LDB. Each call will output an index and a DB_DATA_VALUE.
**	The output stream must be already initialized with a GCA_MD_ASSOC
**	before calling this routine. 
**
** Inputs:
**
**	assoc		association of the message stream
**	index		index value of parameter
**	type		type of db_data_value.
**	prec		precision of db_data_value
**	len		length of the data of the db_data_value
**	data		ptr to the data of the db_data_value
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-Nov-1992 (fpang)
**          Created
*/
DB_ERRTYPE
rqu_put_moddata(
	RQL_ASSOC	*assoc,
	i4		index,
	i4		type,		
	i4		prec,
	i4		leng,
	PTR		data)
{
    DB_ERRTYPE	   status = E_RQ0000_OK;

    do 
    {
    	if ((status = rqu_put_data(assoc, (PTR)&index, sizeof(index))) 
		!= E_RQ0000_OK)
	    break;

    	if ((status = rqu_putdb_val(type, prec, leng, data, assoc))
		!= E_RQ0000_OK)
	    break;
    } while (0);
    return (status);
}

/*
** {
** Name:  rqu_putdb_val - put a GCA_DATA_VALUE to a message stream
**
** Description:
**
**	This is a utility routine to put a GCA_DATA_VALUE to a stream.
**	This is mecessary since GCA_DATA_VALUE is not an atomic object
**	and thus cannot be sent as a whole.
**
** Inputs:
**
**	data_type	object data type
**	precision	object precision
**	data_len	object size
**	data_ptr	object pointer
**	assoc		association of the message stream
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-Oct-1988 (alexh)
**          created
**	18-Jan-1993 (fpang)
**	    If LDB does not support GCA_DECIMAL, convert it to a FLOAT.
**       6-Oct-1993 (fred)
**          Added byte stream (DB_VBTYE_TYPE) complex object support.
**      18-Oct-1993 (fred)
**          Added large object support.  Normally, it will never
**          happen (killed before it gets here), but there are a few
**          cases, and if it does, fail gracefully.
**      19-Oct-1993 (fred)
**          Fix above fix so that it will never check the query text
**          type (which is not a real ADF type).  Also changed the
**          code so that if ADF doesn't like the type, since this code
**          used to work, we let it thru.  Not totally clean, but this
**          will all get redone within a year to make blobs totally
**          work with STAR.
*/
DB_ERRTYPE
rqu_putdb_val(
	i4		data_type,
	i4		precision,
	i4		data_len,
	char		*data_ptr,
	RQL_ASSOC	*assoc)
{
  DB_ERRTYPE	status;
  RQS_CB	*rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;
  f8		flt_tmp;
  SCF_CB        scf_cb;
  SCF_SCI       sci_list[1];
  ADF_CB        *adf_cb;
  i4            dt_bits;

  if ((data_type == DB_DEC_TYPE) &&
      (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_60))
  {
    /* LDB does not know decimal data. Convert to F8 */
    CVpkf ( data_ptr, (i4)DB_P_DECODE_MACRO(precision),
		      (i4)DB_S_DECODE_MACRO(precision), &flt_tmp );
    data_ptr  = (PTR) &flt_tmp;
    data_len  = sizeof(flt_tmp);
    data_type = DB_FLT_TYPE;
    precision = 0;
  }

  rqu_dmp_dbdv(rqs, assoc, data_type, precision, data_len, data_ptr );


  if (data_type != DB_QTXT_TYPE)
  {
      for (;;)
      {
	  /* 
	  ** This code checks for a valid, non-large datatype.
	  ** Datatypes which are verifyably large, are rejected as
	  ** errors.  However, since this code "used to work" without
	  ** this check, if there is some problem finding the type, we
	  ** allow the datatype to go thru.  Most of these cases are
	  ** the query text handled above, but in case there are
	  ** others, this is the safeguard.
	  */
	 
	  scf_cb.scf_type = SCF_CB_TYPE;
	  scf_cb.scf_length = sizeof(SCF_CB);
	  scf_cb.scf_facility = DB_ADF_ID;
	  scf_cb.scf_session  = DB_NOSESSION;
	  scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) sci_list;
	  scf_cb.scf_len_union.scf_ilength = 1;
	  sci_list[0].sci_length = sizeof(adf_cb);
	  sci_list[0].sci_code = SCI_SCB;
	  sci_list[0].sci_aresult = (char *) &adf_cb;
	  sci_list[0].sci_rlength = NULL;
	  
	  /* Get adf_cb from SCF */
	  if(( status = scf_call( SCU_INFORMATION, &scf_cb )
	      ) != E_DB_OK )
	      break;
	  
	  status = adi_dtinfo(adf_cb, data_type, &dt_bits);
	  if (DB_FAILURE_MACRO(status))
	      break;
	  
	  if (dt_bits & AD_PERIPHERAL)
	  {
	      rqu_error( 0, E_RQ004C_RQF_LO_MISMATCH, (PTR)NULL,
			(i4) 0,
			(i4)0, (i4 *) NULL,
			(i4)0, (i4 *)NULL,
			(i4)0, (i4 *)NULL,
			(i4)0, (i4 *)NULL );
	      return(E_RQ004C_RQF_LO_MISMATCH);
	  }
	  break; /* Out of for (;;) loop */
      }
  }

  status = rqu_put_data(assoc, (PTR)&data_type, sizeof(data_type));
  if (status != E_RQ0000_OK)
    return(status);

  status = rqu_put_data(assoc, (PTR)&precision, sizeof(precision));
  if (status != E_RQ0000_OK)
    return(status);

  status = rqu_put_data(assoc, (PTR)&data_len, sizeof(data_len));
  if (status != E_RQ0000_OK)
    return(status);

  switch (data_type)
    {
    case DB_TXT_TYPE:
    case -DB_TXT_TYPE:
    case DB_VCH_TYPE:
    case -DB_VCH_TYPE:
    case DB_VBYTE_TYPE:
    case -DB_VBYTE_TYPE:
      /* these are complex objects. the must be sent atomically. */
      /* for var char, the length prefix must be sent separately */
      status = rqu_put_data(assoc, data_ptr, sizeof(i2));
      if (status != E_RQ0000_OK)
	return(status);
      status = rqu_put_data(assoc, data_ptr+2, data_len-sizeof(i2));
      break;
    default:
      status = rqu_put_data(assoc, data_ptr, data_len);
      break;
    }


  return(status);
}

/*{
** Name:  rqu_putqid - put a GCA_ID to a message stream
**
** Description:
**
**	This is a utility routine to put a GCA_ID to a stream.
**	This is mecessary since GCA_ID is not an atomic object
**	and thus cannot be sent as a whole. Note that the input
**	is of DB_CURSOR_ID, not GCA_ID.
**
** Inputs:
**
**	assoc		association of the message stream
**	qid		* DB_CURSOR_ID
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-Dec-1988 (alexh)
**          created
**	25-Feb-1992 (fpang)
**	    When putting the gca_name field as a db_data_value, it's type
**	    should be DB_CHA_TYPE rather than DB_TXT_TYPE. 
**	    Fixes B42194 and B42237.
*/
DB_ERRTYPE
rqu_putqid(
	RQL_ASSOC	*assoc,
	DB_CURSOR_ID	*qid)
{
  DB_ERRTYPE	status;
  GCA_ID	gca_qid;
  RQS_CB	*rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

  gca_qid.gca_index[0] = qid->db_cursor_id[0];
  gca_qid.gca_index[1] = qid->db_cursor_id[1];
  MEcopy((PTR)qid->db_cur_name, sizeof(gca_qid.gca_name),
	 (PTR)gca_qid.gca_name);
  if (sizeof(gca_qid.gca_name) > sizeof(qid->db_cur_name))
    MEfill((sizeof(gca_qid.gca_name) - sizeof(qid->db_cur_name)),
	   (unsigned char)' ', 
	   (PTR)&gca_qid.gca_name[sizeof(qid->db_cur_name)]);

  RQTRACE(rqs,2)(RQFMT(rqs,
	      "RQF:Assn %d. QID(textual): (%d, %d, %t)\n"),
	      assoc->rql_assoc_id,
	      gca_qid.gca_index[0],
	      gca_qid.gca_index[1], 
	      sizeof(gca_qid.gca_name),
	      gca_qid.gca_name );

  status = rqu_putdb_val(DB_INT_TYPE, 0, sizeof(gca_qid.gca_index[0]),
			 (PTR)&gca_qid.gca_index[0], assoc);

  if (status != E_RQ0000_OK)
	return(status);

  status = rqu_putdb_val(DB_INT_TYPE, 0, sizeof(gca_qid.gca_index[1]),
			 (PTR)&gca_qid.gca_index[1], assoc);

  if (status != E_RQ0000_OK)
	return(status);

  status = rqu_putdb_val(DB_CHA_TYPE, 0, sizeof(gca_qid.gca_name),
			 gca_qid.gca_name, assoc);

  return(status);
}

/*{
** Name:  rqu_putcid - put a GCA_ID to a message stream
**
** Description:
**
**	This is a utility routine to put a GCA_ID to a stream.
**	This is mecessary since GCA_ID is not an atomic object
**	and thus cannot be sent as a whole. Note that the input
**	is of DB_CURSOR_ID, not GCA_ID.
**
** Inputs:
**
**	assoc		association of the message stream
**	cid		* DB_CURSOR_ID
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-Dec-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqu_putcid(
	RQL_ASSOC	*assoc,
	DB_CURSOR_ID	*cid)
{
  DB_ERRTYPE	status;
  GCA_ID	gca_cid;
  RQS_CB	*rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;


  /* GCA_ID needs to be constructed because potential structural differences */
  gca_cid.gca_index[0] = cid->db_cursor_id[0];
  gca_cid.gca_index[1] = cid->db_cursor_id[1];
  /* only copy the significant part of the source */
  MEcopy((PTR)cid->db_cur_name, sizeof(gca_cid.gca_name),
	 (PTR)gca_cid.gca_name);
  /* pad blanks at the end of destination buffer, if sizes are different */
  if (sizeof(gca_cid.gca_name) > sizeof(cid->db_cur_name))
    MEfill((sizeof(gca_cid.gca_name) - 
		  sizeof(cid->db_cur_name)),
	   (unsigned char)' ', 
	   (PTR)&gca_cid.gca_name[sizeof(cid->db_cur_name)]);

  /* put the GCA_ID structure to the stream */
  status = rqu_put_data(assoc, (PTR)&gca_cid, sizeof(gca_cid));

  RQTRACE(rqs,2)(RQFMT(rqs,
	      "RQF:Assn %d. QID(encoded): (%d, %d, %t)\n"),
	      assoc->rql_assoc_id,
	      gca_cid.gca_index[0],
	      gca_cid.gca_index[1], 
	      sizeof(gca_cid.gca_name),
	      gca_cid.gca_name
    		);
  return(status);
}

/*
** {
** Name:  rqu_putformat - put a CREATE statement format to stream
**
** Description:
**
**	This is a utility routine that puts a column data type to
**	a stream in ASCII. This is used during CREATE statement
**	generation during a table transfer.
**
** Inputs:
**
**	type		data type
**	len		data length
**	lang		target language
**	buffer		message buffer. must contain a null string.
**
** Outputs:
**          buffer	buffer is filled with type string.
**        
**	Returns:
**		none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-Oct--1988 (alexh)
**          created
**       6-Oct-1993 (fred)
**          Added byte stream datatype support.  Changed interface to
**          take an entire db datavalue.  This should enable decimal
**          to function in temporary tables, fixing bug b54862.
**       25-May-1994 (heleny)
**          Bug46139: make sure the length for varchar is at least one.
**	 10-May-1995 (wilri01)
**	    Bug67740: Back out 46139. Since the tuple is copied without
**	    reformating, this simply gives an incorrect definition of the
**	    tuple.  The real fix is to represent a null string <''> as a
**	    1-byte VARCHAR with a zero length value.
**	03-aug-2006 (gupsh01)
**	    Added ANSI date/time types.
**	28-Jan-2009 (kibro01) b121521
**	    Turn off ingresdate if we are communicating with an old server
**	    which still expects "date".
*/

VOID
rqu_putformat(DB_DATA_VALUE *orig_input,
	      DB_LANG	    lang,
	      char	    *buffer,
	      bool	    use_ingresdate)
{
    bool	nullable;
    RQS_CB	*rqs = (RQS_CB *)NULL; 	/* For RQTRACE */
    ADI_DT_NAME tyname;
    i4          dt_bits;
    SCF_CB      scf_cb;
    SCF_SCI     sci_list[1];
    ADF_CB      *adf_cb;
    DB_DATA_VALUE result;
    DB_DATA_VALUE input;
    i4	    	type = orig_input->db_datatype;
    i4	len = orig_input->db_length;
    i2          prec = orig_input->db_prec;
    DB_STATUS   status;

    buffer = buffer+STlength(buffer);
    if (type < 0)
    {
	--len;	/* subtract one for the null indicator */
	type = -type;
	nullable = TRUE;
    }
    else
    {
	nullable = FALSE;
    }

    switch (type)
    {
	case DB_INT_TYPE:
	    if (lang == DB_SQL)
	    {
		STprintf(buffer, "integer%d", len);
	    }
	    else
	    {
		STprintf(buffer,"i%d", len);
	    }
	    break;
	case DB_FLT_TYPE:
	    if (lang == DB_SQL)
	    {
		STprintf(buffer, "float%d", len);
	    }
	    else
	    {
		STprintf(buffer,"f%d", len);
	    }
	    break;
	case DB_CHR_TYPE :
	    STprintf(buffer, "c%d", len);
	    break;
	case DB_TXT_TYPE :
	    len -= 2;
	    STprintf(buffer, "text(%d)", len);
	    break;
	case DB_DTE_TYPE :
	    STcat(buffer, use_ingresdate?"ingresdate":"date");
	    break;
	case DB_ADTE_TYPE :
	    STcat(buffer, "ansidate");
	    break;
	case DB_TMWO_TYPE :
	    STcat(buffer, "time without time zone");
	    break;
	case DB_TMW_TYPE :
	    STcat(buffer, "time with time zone");
	    break;
	case DB_TME_TYPE :
	    STcat(buffer, "time with local time zone");
	    break;
	case DB_TSWO_TYPE :
	    STcat(buffer, "timestamp without time zone");
	    break;
	case DB_TSW_TYPE :
	    STcat(buffer, "timestamp with time zone");
	    break;
	case DB_TSTMP_TYPE :
	    STcat(buffer, "timestamp with local time zone");
	    break;
	case DB_INDS_TYPE :
	    STcat(buffer, "interval day to second");
	    break;
	case DB_INYM_TYPE :
	    STcat(buffer, "interval year to month");
	    break;
	case DB_MNY_TYPE :
	    STcat(buffer, "money");
	    break;
	case DB_CHA_TYPE :
	    STprintf(buffer, "char(%d)", len);
	    break;
	case DB_VCH_TYPE :
	    len -= 2;
	    STprintf(buffer, "varchar(%d)", len);
	    break;

	default:
	    for (;;)
	    {
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

		status = adi_tyname(adf_cb, type, &tyname);
		if (DB_FAILURE_MACRO(status))
		    break;
		status = adi_dtinfo(adf_cb, type, &dt_bits);
		if (DB_FAILURE_MACRO(status))
		    break;

		input.db_datatype = type;
		input.db_data = (char *) 0;

		if ((dt_bits & AD_PERIPHERAL) == 0)
		{
		    input.db_length = len;
		
		    if (type == DB_DEC_TYPE)
			input.db_prec = prec;
		    else
			input.db_prec = 0;
		}
		else
		{
		    input.db_length = sizeof(ADP_PERIPHERAL);
		    input.db_prec = 0;
		}
			
		status = adc_lenchk(adf_cb, 0, &input, &result);
		if (DB_FAILURE_MACRO(status))
		    break;
		if (type == DB_DEC_TYPE)
		{
		    STprintf(buffer, "%s(%d,%d)",
			     tyname.adi_dtname,
			     DB_P_DECODE_MACRO(result.db_prec),
			     DB_S_DECODE_MACRO(result.db_prec));
		}
		else
		{
		    STprintf(buffer, "%s(%d)",
			     tyname.adi_dtname,
			     result.db_length);
		}
		break;
	    }
	    if (DB_FAILURE_MACRO(status))
	    {
	        RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:rqu_putformat. Unknown data type %d.\n"), type);
	    }
	    break;
	}

	if (nullable == FALSE)
	{
	    STcat(buffer, " not null");
	}
}

/*{
** Name: rqu_cluster_info - Set flags based on cluster information
**
** Description:
**    Given a node name, return a flag indicating whether a password
**    is required, and a flag indicating whether an alias node exists.
**    In the latter case, return an alias node to try.
**
** Inputs:
**    node_name       target node
**
** Outputs:
**    flags           bit field
**        RQF_PASS_REQUIRED   - On if required, Off if not required.
**        RQF_NOALIAS_NODE    - On if alias exists, Off if none exists.
**
**    alt_node_name   alias node to try.
**
**    Returns:
**        VOID
**    Exceptions:
**        none
**
** Side Effects:
**        none
**
** History:
**    05-Apr-1993 (fpang)
**        Wrote function header, rewrote comments and cleaned up code.
*/
VOID
rqu_cluster_info(
	i4	    *flags,
	DD_NODE_NAME *alt_node_name,
	DD_NODE_NAME *node_name)
{
    DD_NODENAMES	*cluster_info;
    i4			string_flag;
    i4			i;


    if (  (STbcompare( (char *) node_name, (u_i4)sizeof(*node_name),
		    Rqf_facility->rqf_vnode, 
		    Rqf_facility->rqf_len_vnode, TRUE ) == 0)
	||  (STbcompare( (char *) node_name, (u_i4)sizeof(DD_NODE_NAME),
		    DB_ABSENT_NAME, (u_i4)sizeof(DB_ABSENT_NAME)-1, TRUE ) == 0))
    {
	/* Matches current node or is absent. NO PASS RQD, and NO ALIAS */
	*flags = (*flags & ~RQF_PASS_REQUIRED) | RQF_NOALIAS_NODE;
    }
    else if (  (Rqf_facility->rqf_cluster_p == (PTR)NULL)
	  || (Rqf_facility->rqf_num_nodes <= 0) )
    {
	/* No cluster to check. Must be remote. PASS RQD, and NO ALIAS */
	*flags |= (RQF_NOALIAS_NODE | RQF_PASS_REQUIRED);
    }
    else
    {
	/* Now check cluster. */
	for( i = 0,
		 cluster_info = (DD_NODENAMES *)Rqf_facility->rqf_cluster_p;
	     (i < Rqf_facility->rqf_num_nodes)
		  && (cluster_info != (DD_NODENAMES *)NULL);
	     i++, cluster_info = cluster_info->dd_nextnode )
	{
	    if(! STbcompare( (char *) node_name, (i4)sizeof(*node_name),
			    (char *) cluster_info->dd_nodename,
			    (i4)sizeof(cluster_info->dd_nodename), TRUE))
	    {
		/* Node is in the cluster. NO PASS RQD, and ALIAS */
		MEmove(Rqf_facility->rqf_len_vnode,
		       (PTR)Rqf_facility->rqf_vnode,
		       ' ',
		       sizeof(*alt_node_name),
		       (PTR)alt_node_name);
		*flags = (*flags & ~RQF_PASS_REQUIRED) & ~RQF_NOALIAS_NODE;
		break;
	    }
	}
	/* No match. PASS RQD and NO ALIAS */
	if ((i >= Rqf_facility->rqf_num_nodes) ||
	    (cluster_info == (DD_NODENAMES *)NULL))
	    *flags = *flags | RQF_NOALIAS_NODE | RQF_PASS_REQUIRED;
    }
    return;
}


/*{
** Name: rqu_new_assoc - allocate an association for a site
**
** Description:
**    Given a target node, database, and server type, establish a connection
**    and allocate a RQL_ASSOC control block for the association. The
**    allocated block is chained into the session's list of associations,
**    and a pointer to the allocated block is returned to the caller. The
**    connection establishment is initiated with a GCA_REQUEST call. After
**    the request is accepted, GCA_MD_ASSOC messages are exchanged, and
**    'set' queries, if the exists,  are issued. If the connection to the
**    requested node fails, an 'alias' node from the cluster will be tried.
**    If the parameter assocmem_ptr is specified, it will be used to store
**    the association, instead of allocating a new one. If the restart_dx_id
**    parameter is specified, it will be sent with a GCA_RESTARTi, when the
**    GCA_MD_ASSOC messages are exchanged. If the connection cannot be made
**    to the specified node or the alias, then NULL is returned.
**
**    Usernames and passwords.
**    + Both supplied. Connect as username with password.
**    + Neither supplied, or ingres_b is true. Will let GCA decide who to
**        connect as. Usually it will be the owner of the STAR process.
**    + Only username supplied. Like previous, but will modify to username
**        (like -u).
**
**    II_LDB_SERVER - if defined, the Name Server (GCN) will be bypassed.
**
** Inputs:
**	rqr_cb            request control block
**      session           session context
**	ldb		  target ldb
**	user_name         username and password
**	flags             mod assoc flags
**	assocmem_ptr      put assoc here, don't allocate
**	restart_dx_id     distributed transaction id to restart
**
** Outputs:
**          none
**        
**	Returns:
**	    RQL_ASSOC *   address of the new association
**		NULL      if unsuccessful.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-jun-1988 (alexh)
**          created
**	21-jul-1988 (alexh)
**	    spinned off the rqu_mod_assoc.
**      17-jan-1989 (georgeg)
**          added username and effective username
**      07-nov-1989 (georgeg)
**          added support for 2pc
**	08-nov-1989 (georgeg)
**	    Saved the gca peer protocol levwel for the association.
**	07-dec-1989 (georgeg)
**	    passed a null terminated string "dbname" to rqu_mod_assoc(), it is a miracle
**	    this thing was working.
**	06-nov-1990 (georgeg)
**	    Added support for clusters.
**	17-dec-1990 (fpang)
**	    In rqu_new_assoc(), make sure password is required
**	    (conflags & RQF_PASS_REQUIRED) and user did not give a
**	    password (gca_req.gca_password == NULL) before invalidating
**	    an association.
**	18-sep-1991 (fpang)
**	    Issue RQ004B instead of RQ0047. The only
**	    time that we do not have a password is if local authorization
**	    is missing. Fixes B39731.
**	09-APR-1992 (fpang)
**	    If re-using storage (assocmem_ptr != NULL), don't reset the
**	    linked list pointers, rql_next, and rqs_assoc_list. Reseting
**	    them will result in an inconsistent association list.
**	    Fixed B42810 (SEGV if LDB is killed between SECURE and COMMIT).
**	15-oct-1992 (fpang)
**	    1. Replace sc0m_rqf_alloc() with ULM calls.
**	    2. Suppport cs_client_type wrt. to scc_gcomplete() change.
**	    3. Set gca_peer_protocol to 50 because we can support new copy 
**	       message types.
**	19-oct-92 (teresa)
**	    a call to rqu_error was missing some null parameters.  I added them.
**	06-apr-1993 (fpang)
**	    1. Support for installation passwords. Don't required password
**	       anymore. GCN/GCC take care of it now.
**	    2. Cleaned up code. Shortened lines, fixed comments, removed
**	       redundant and/or dead code.
**	8-apr-93 (robf)
**	    Added handling for security labels at GCA_PROTOCOL_LEVEL_61,
**	    including making a request as an agent and passing the security
**	    label in for a secure server.
**	13-jul-1993 (fpang)
**	    Declare the strings 'dbname' and 'dbms_type' to have one more byte
**	    to accommodate the EOS. Also use MEmove() to fill dd_co21_euser
**	    instead of STcopy() because dd_co21_euser should not be EOS 
**	    terminated.
**	15-sep-1993 (fpang)
**	    Recovered 'lost' fix for B42810 (AV when restarting LDB's)..
**	20-jun-2001 (somsa01)
**	    When constructing the partner name as NODE::DATABASE/SERVER,
**	    properly place the EOS value in full_name so that
**	    STtrmwhite() will work.
**	19-mar-2003 (gupsh01)
**	    when running the register as link query to register a gateway 
**	    table against a star database error	E_PS090D is given out.     
**	    Fixed overwriting the dbms class, causing incorrect query lookup.
**      12-Nov-2007 (hanal04) Bug 119426
**          Resolve all manner of i8 disasters by setting the protocol level
**          the 65 unless the end client has a protocol level that does not
**          support i8s in which case we continue to use 62. It should be
**          Noted that 62 has never blocked attempts to use unicode through
**          Star as was intended.
*/
RQL_ASSOC  *
rqu_new_assoc(
	RQR_CB		*rqr_cb,
	RQS_CB		*session,
	DD_LDB_DESC     *ldb,
	DD_USR_DESC	*user_name,
	DD_COM_FLAGS	*flags,
	RQL_ASSOC	*assocmem_ptr,
	DD_DX_ID	*restart_dx_id)
{
    char	    *partner_name;
    GCA_RQ_PARMS    gca_req;
    RQL_ASSOC	    temp_assoc;
    RQL_ASSOC	    *assoc = &temp_assoc;
    char	    *full_name;
    char	    dbms_type[ sizeof(ldb->dd_l4_dbms_name) + 1 ];
    char   	    dbname[ sizeof(ldb->dd_l3_ldb_name) + 1 ];
    char	    *uname, *passw;
    STATUS	    status;
    DD_NODE_NAME    alt_node_name, *node_name;
    i4	    con_flags=0;
    i4		    log_error_only;
    RQS_CB	    *rqs = (RQS_CB *)rqr_cb->rqr_session;
    ULM_RCB	    ulm;
    DD_LDB_DESC	    *cdb;
    i4		    it = 0;
    SCF_CB          scf_cb;
    SCF_SCI         sci_list[1];
    ADF_CB          *adf_cb;

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_streamid_p = &session->rqs_streamid;
    ulm.ulm_memleft  = &Rqf_facility->rqf_ulm_memleft;

    MEfill( sizeof(*assoc), 0, (PTR)assoc);
    assoc->rql_assoc_id = -1;
    assoc->rql_rqr_cb = rqr_cb;
    MEfill( sizeof(gca_req), 0, (PTR)&gca_req);

    NMgtAt("II_LDB_SERVER", &partner_name); /* get the server name */

    STncpy(dbname, ldb->dd_l3_ldb_name, sizeof(ldb->dd_l3_ldb_name));
    dbname[ sizeof(ldb->dd_l3_ldb_name) ] = '\0';
    STtrmwhite(dbname);
    node_name = (DD_NODE_NAME *)ldb->dd_l2_node_name;

    /* Allocate most needed for node::dbname/type,username,password */
    ulm.ulm_psize = sizeof(ldb->dd_l2_node_name)    /* node */
                  + 2                             /* :: */
                  + sizeof(ldb->dd_l3_ldb_name)   /* dbname */
                  + 1                             /* / */
                  + sizeof(ldb->dd_l4_dbms_name)  /* type */
                  + 1                             /* eos */
                  + sizeof(user_name->dd_u1_usrname)  /* username */
                  + 1                                 /* eos */
                  + sizeof(user_name->dd_u3_passwd)   /* password */
                  + 10;                               /* eos & extra */

    if (ulm_palloc(&ulm) != E_DB_OK)
    {
	rqu_error( 0, E_RQ0038_ULM_ALLOC_FAILED, (PTR)NULL,
		  (i4)1,
		  sizeof(ulm.ulm_error.err_code),
		  &ulm.ulm_error.err_code,
		  (i4)0, (i4 *)NULL, (i4)0, (i4 *)NULL,
		  (i4)0, (i4 *)NULL );
	return (NULL);
    }

    assoc->rql_partner_name = ulm.ulm_pptr;
    assoc->rql_user_name = assoc->rql_partner_name
			 + sizeof(ldb->dd_l2_node_name)  /* node */
			 + 2                             /* :: */
			 + sizeof(ldb->dd_l3_ldb_name)   /* dbname */
			 + 1                             /* / */
			 + sizeof(ldb->dd_l4_dbms_name)  /* type */
			 + 1;                                  /* eos */
    assoc->rql_password  = assoc->rql_user_name
			 + sizeof(user_name->dd_u1_usrname)  /* username */
			 + 1;                                /* eos */

    /*
    ** If there is a username and password, use it to establish an association
    ** When username is null, GCA will use the owner of the Star process as the
    ** user. In most cases the user is $ingres. So when the privileged CDB
    ** association is established, username and password are set to null so
    ** that the user becomes $ingres.
    */
    uname = passw = NULL;
    if ((ldb->dd_l1_ingres_b == FALSE)            /* Not Privileged CDB */
	&& (user_name != NULL)             	  /* and username supplied */
	&& (*(user_name->dd_u1_usrname) != EOS))
    {
	STncpy( assoc->rql_user_name, user_name->dd_u1_usrname,
                 sizeof(user_name->dd_u1_usrname));
	assoc->rql_user_name[ sizeof(user_name->dd_u1_usrname) ] = '\0';
	STtrmwhite( assoc->rql_user_name );
	if (user_name->dd_u2_usepw_b == TRUE)
	{
	    uname = assoc->rql_user_name;
	}
	else
	{
	    /*
	    ** If the password is not present then copy the user name to the
	    ** effective user so that star can inpersonate the user with -u.
	    */
	    if ( flags->dd_co21_euser[0] == EOS )
	    	flags->dd_co22_len_euser =
				(i4)STlength( assoc->rql_user_name );
	    MEmove(flags->dd_co22_len_euser, assoc->rql_user_name,
		   (char)' ',
		   sizeof(flags->dd_co21_euser), flags->dd_co21_euser);
	    flags->dd_co0_active_flags++;
	} /* dd_u2_usepw_b, -u */
    } /* password */	

    /* Compute number of attempts. If an alias exists 2, 1 otherwise. */
    rqu_cluster_info(&con_flags, (DD_NODE_NAME *)alt_node_name, node_name);
    it = (con_flags & RQF_NOALIAS_NODE) ? 1 : 2;
    while (it > 0)
    {
	/* Release assoc if established. */
	if (assoc->rql_assoc_id != -1)
	    rqu_release_assoc(assoc);

	if ( it == 1 )
	{
	    /* Use actual node and print errors. */
	    assoc->rql_noerror = FALSE;
	    node_name = (DD_NODE_NAME *)ldb->dd_l2_node_name;
	    log_error_only = FALSE;
	}
	else /* it == 2 */
	{
	    /* Use alias node but do not print errors. */
	    assoc->rql_noerror = TRUE;
	    node_name = (DD_NODE_NAME *)alt_node_name;
	    log_error_only = TRUE;
	}
	it--;

	if ((partner_name != NULL) && (*partner_name != EOS))
	{
	    /* II_LDB_SERVER specified. Bypass name server */
	    gca_req.gca_partner_name = partner_name;
	    gca_req.gca_modifiers = GCA_NO_XLATE;
	}
	else
	{
	    i4	oldlen;

	    /* Construct partner name as NODE::DATABASE/SERVER */
	    full_name = assoc->rql_partner_name;
	    STncpy( full_name, (char *)node_name, sizeof(*node_name));
	    full_name[ sizeof(*node_name) ] = '\0';
	    if (STtrmwhite(full_name) > 0)
		(VOID) STcat(full_name,"::");
	    oldlen = STlength(full_name);
	    STncpy(&full_name[STlength(full_name)], ldb->dd_l3_ldb_name,
			    sizeof(ldb->dd_l3_ldb_name));
	    full_name[oldlen + sizeof(ldb->dd_l3_ldb_name)] = '\0';
	    STtrmwhite(full_name);
	    STncpy( dbms_type, ldb->dd_l4_dbms_name,
			    sizeof(ldb->dd_l4_dbms_name) );
	    dbms_type[ sizeof(ldb->dd_l4_dbms_name) ] = '\0';
	    if (STtrmwhite(dbms_type) > 0)
	    {
		i4 dblen;
		STcat(full_name, "/");
		dblen = STlength( full_name );
		STncpy( &full_name[ dblen ], dbms_type, STlength( dbms_type ));
		full_name[ dblen + STlength(dbms_type) ] = '\0';
	    }
	    gca_req.gca_partner_name = full_name;
	    gca_req.gca_modifiers = 0;
	}
	RQTRACE(rqs,2)(RQFMT(rqs,
		"RQF:rqu_new_assoc. Partnername %s, Modifiers 0x%x\n"),
			gca_req.gca_partner_name,gca_req.gca_modifiers);

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
        if(( status = scf_call( SCU_INFORMATION, &scf_cb )) != E_DB_OK )
        {
            rqu_error(TRUE, scf_cb.scf_error.err_code, (PTR) NULL, (i4)0,
                (i4)0, (i4 *)NULL, (i4)0, (i4 *)NULL,
                (i4)0, (i4 *)NULL, (i4)0, (i4 *)NULL );
            break;
        }

	/* Fill in the rest of the gca request block. */
	gca_req.gca_account_name = NULL;
	gca_req.gca_user_name = uname;
	gca_req.gca_password = passw;
	gca_req.gca_assoc_id = -1;
        if (adf_cb->adf_proto_level & AD_I8_PROTO)
        { 
	    gca_req.gca_peer_protocol = GCA_PROTOCOL_LEVEL_65;
        }
        else
        {
            gca_req.gca_peer_protocol = GCA_PROTOCOL_LEVEL_62;
        }
        if (adf_cb->adf_proto_level & AD_BOOLEAN_PROTO)
            gca_req.gca_peer_protocol = GCA_PROTOCOL_LEVEL_68;
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqu_new_assoc. Connecting for user: %s "),
			( gca_req.gca_user_name == NULL ) ?
			"NULL" : gca_req.gca_user_name );
	RQTRACE(rqs,1)(RQFMT(rqs,
		"With %s.\n"), ( gca_req.gca_password != NULL ) ?
			"password" : "NO password" );

	/* Removed check for local authorization. It is no longer required.
	** Installation passwords remove this restriction.
	*/

	/* Initialize assoc */
	STRUCT_ASSIGN_MACRO(*ldb, assoc->rql_lid);
	CSget_scb(&assoc->rql_scb);
	assoc->rql_next = NULL;
	assoc->rql_interrupted = FALSE;
	assoc->rql_status = RQL_OK;
	assoc->rql_id_error = 0;
	assoc->rql_comm.rqc_rxtx = RQC_IDDLE;
	assoc->rql_comm.rqc_status = RQC_OK;
	assoc->rql_comm.rqc_gca_status = E_DB_OK;
	assoc->rql_comm.rqc_gca_status = E_DB_OK;
	assoc->rql_timeout = 0;
	assoc->rql_turned_around = FALSE;

	/* Attempt request. */
	if (rqu_gccall(GCA_REQUEST, (PTR)&gca_req, &gca_req.gca_status, assoc)
	        != E_GC0000_OK)
	{
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:rqu_new_assoc. Request for association  %s "),
			gca_req.gca_partner_name );
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"-- request refused! gca status 0x%x.\n"),
	                gca_req.gca_status);
	    /* Log ( and display ) error. */
	    rqu_error( log_error_only,
	        gca_req.gca_status, (PTR)&gca_req.gca_os_status, (i4) 0,
	        (i4) 0, (i4 *) NULL, (i4) 0, (i4 *) NULL,
	        (i4) 0, (i4 *) NULL, (i4) 0, (i4 *) NULL );

	    rqr_cb->rqr_1ldb_severity = 0;
	    rqr_cb->rqr_2ldb_error_id = 0;
	    rqr_cb->rqr_3ldb_local_id = 0;

	    /* Connection established, but failed for some other reason.
	    ** Must still disassociate connection.
	    */
	    if ( gca_req.gca_assoc_id != -1 )
	    {   
		GCA_DA_PARMS    disassociate;

		disassociate.gca_association_id = gca_req.gca_assoc_id;
	        /* Ignore status */
	        (VOID)IIGCa_call(GCA_DISASSOC,
				(GCA_PARMLIST *)&disassociate,
				(i4)GCA_SYNC_FLAG, (PTR)0,
				(i4)-1, &status);
	    }
	    continue; /* Request failed, try alias */
	}
	else
	{
	    /* Connection request succeedded */
	    assoc->rql_peer_protocol = gca_req.gca_peer_protocol;
	    assoc->rql_assoc_id = gca_req.gca_assoc_id;
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. New Association to %s. Protocol %d\n"),
			gca_req.gca_assoc_id,
			gca_req.gca_partner_name,
			gca_req.gca_peer_protocol); 
	    RQTRACE(rqs,1)(RQFMT(rqs,
		"\tldb_id %d ingres_b %d ldb_flags 0x%x.\n"),
			ldb->dd_l5_ldb_id,
			ldb->dd_l1_ingres_b, 
			ldb->dd_l6_flags);
	} /* gca_request */

	/* Format temporary read and write buffers */
	rqu_prepare_assoc(assoc);

	/* Check for association request acceptance */
	assoc->rql_rv.gca_data_area = NULL;
	assoc->rql_status = RQL_OK;
	assoc->rql_id_error = 0;
	assoc->rql_comm.rqc_rxtx = RQC_IDDLE;
	assoc->rql_comm.rqc_status = RQC_OK;
	assoc->rql_comm.rqc_gca_status = E_DB_OK;
	assoc->rql_turned_around = FALSE;
	assoc->rql_timeout = 0;

	if (rqu_get_reply(assoc, -1) != GCA_ACCEPT)
	{
	    if (assoc->rql_rv.gca_data_area)
	    {
	        GCA_ER_DATA     *er;
	        er = (GCA_ER_DATA *)assoc->rql_rv.gca_data_area;
	        rqu_error( log_error_only, er->gca_e_element[0].gca_id_error,
	            (PTR) NULL, (i4)0,
	            (i4)0, (i4 *)NULL, (i4)0, (i4 *)NULL,
	            (i4)0, (i4 *)NULL, (i4)0, (i4 *)NULL );
		RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:Assn %d. GCA_REQUEST rejected, gca_id_error 0x%x.\n"),
	                        gca_req.gca_assoc_id,
	                        er->gca_e_element[0].gca_id_error );
	    }
	    continue;       /* Got wrong reply, try alias */
	} /* reply not GCA_ACCEPT */

	/* Is this the privileged CBD ? */
	if ( (ldb->dd_l1_ingres_b == TRUE)
	  && (session->rqs_user_cdb == NULL)
	  && (STncasecmp("iidbdb", dbname, STlength(dbname)) != 0))
	{
	    /* copy the name, site, and dbms */
	    STRUCT_ASSIGN_MACRO(*ldb, session->rqs_cdb_name);
	    session->rqs_cdb_name.dd_l1_ingres_b = FALSE;
	}

	/* Is this the user CDB ? */
	cdb = &session->rqs_cdb_name;
	if ( (session->rqs_user_cdb == NULL)
	    && (ldb->dd_l1_ingres_b == FALSE)
	    && (ldb->dd_l1_ingres_b == cdb->dd_l1_ingres_b)
	    && (MEcmp(ldb->dd_l2_node_name, cdb->dd_l2_node_name,
	              sizeof(ldb->dd_l2_node_name)) == 0)
	    && (MEcmp(ldb->dd_l3_ldb_name, cdb->dd_l3_ldb_name,
	              sizeof(ldb->dd_l3_ldb_name)) == 0)
	    && (MEcmp(ldb->dd_l4_dbms_name, cdb->dd_l4_dbms_name,
	              sizeof(ldb->dd_l4_dbms_name)) == 0))
	{
	    RQTRACE(rqs,2)(RQFMT(rqs,
		"RQF:rqu_new_assoc. User CDB assoc found.\n"));
	    session->rqs_user_cdb = assoc;	/* Mark with temporary assoc */
	}

    
	/* modify association parameters, e.g DB name, GCA version, etc. */
	if (rqu_mod_assoc(session, assoc, dbname, flags, restart_dx_id) 
	    != E_DB_OK)
	{
	    continue;	/* mod_assoc failed, try alias. */
	}
	else
	{
	    /* MOD_ASSOC succeedded */
		
	    RQL_ASSOC		*new_assoc = NULL;
	    GCA_AT_PARMS	gca_attributes;

	    /* Don't allocate if re-using a descriptor. */
	    if (assocmem_ptr != NULL)
	    {
		/* Don't lose the next association if re-using space */
	        assoc->rql_next = assocmem_ptr->rql_next;
		new_assoc = assocmem_ptr;
	    }
	    else
	    {
		ulm.ulm_psize = sizeof(RQL_ASSOC);
		if (ulm_palloc(&ulm) != E_DB_OK)
		{
		    rqu_error( 0, E_RQ0038_ULM_ALLOC_FAILED, (PTR)NULL, 
			(i4)1,
			sizeof(ulm.ulm_error.err_code), &ulm.ulm_error.err_code,
			(i4)0, (i4 *)NULL, 
			(i4)0, (i4 *)NULL,
			(i4)0, (i4 *)NULL );
		    assoc->rql_turned_around = FALSE;
		    rqu_release_assoc(assoc);
		    break;	/* ulm_palloc() failed, return */
		}
		new_assoc = (RQL_ASSOC *)ulm.ulm_pptr;
		/* prepend the assoc token to the session assoc list */
		assoc->rql_next = session->rqs_assoc_list;
		session->rqs_assoc_list = new_assoc;
	    } /* assocmem_ptr */

	    STRUCT_ASSIGN_MACRO(*assoc, *new_assoc);

	    /* send all the SQL/QUEL set options. */
	    if (session->rqs_set_options != NULL)
	    {
		DD_PACKET   *pkt;
		assoc->rql_noerror = FALSE;
	        for (pkt = session->rqs_set_options; pkt != (DD_PACKET *)NULL;
	             pkt = pkt->dd_p3_nxt_p)
	        {
	            status = rqu_set_option( new_assoc, pkt->dd_p1_len,
	                                  pkt->dd_p2_pkt_p, session->rqs_lang);
	            if (status != E_RQ0000_OK)
	                break;
	        }
	        if ( status != E_RQ0000_OK)
	        {
	            RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. Can't set (%s)options: %t\n"),
	                        assoc->rql_assoc_id, gca_req.gca_partner_name,
	                        pkt->dd_p1_len, pkt->dd_p2_pkt_p);
	            assoc->rql_turned_around = FALSE;
	            rqu_release_assoc(assoc);
	            break;      /* rqu_set_option() failed, return */
	        }
	    } /* 'set' options */

	    /* Format permanent read and write buffers */
	    rqu_prepare_assoc(new_assoc);

	    /* Get association attributes. */
	    gca_attributes.gca_association_id = new_assoc->rql_assoc_id;
	    if (rqu_gccall(GCA_ATTRIBS, (PTR)&gca_attributes, 
			    &gca_attributes.gca_status, assoc) != E_GC0000_OK)
	    {
		RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. Can't get GCA_ATTRIBS, LDB(%s).\n"),
			    assoc->rql_assoc_id, gca_req.gca_partner_name);
		assoc->rql_turned_around = FALSE;
		rqu_release_assoc(assoc);
		break;		/* gca_attribs failed, return */
	    }

	    if ( session->rqs_user_cdb == assoc )
	        session->rqs_user_cdb = new_assoc;
	    new_assoc->rql_diff_arch =
	                            (bool)(gca_attributes.gca_flags & GCA_HET);
	    new_assoc->rql_nostatments = 0;
	    assoc->rql_turned_around = TRUE;
	    assoc->rql_alias_node = TRUE;
	    /* From here on we report all errors */
	    assoc->rql_noerror = FALSE;
	    return(new_assoc);      /* Return new association. */
	} /* mod_assoc */
    }/* while */
    return(NULL);   /* Error */
}


/*{
** Name: rqu_ingres_priv - set correct ingres priviledge for the association.
**
** Description:
**	set correct ingres priviledge for the association.
**
** Inputs:
**      assoc		association to be modified
**      ingres_b	ingres priviledge indicator
**
** Outputs:
**          assoc->rql_lib.dd_l1_ingres_b is set to ingres_b is modification
**			is successfull.
**        
**	Returns:
**	    RQL_ASSOC * of the modified association;
**          NULL if modification was not successful.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-Dec-1988 (alexh)
**          created
*/
RQL_ASSOC  *
rqu_ingres_priv(
	RQL_ASSOC	*assoc,
	bool		ingres_b)
{

  i4		i4_buff;
  GCA_SESSION_PARMS	*parms;
  RQS_CB	        *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

  parms = (GCA_SESSION_PARMS *)assoc->rql_sd.gca_buffer;
  parms->gca_l_user_data = 1;

  rqu_putinit(assoc, GCA_MD_ASSOC);
  (VOID)rqu_put_data(assoc, (PTR)&parms->gca_l_user_data, 
			sizeof(parms->gca_l_user_data));

  i4_buff = (ingres_b == TRUE ) ? GCA_ON : GCA_OFF;
  (VOID)rqu_put_moddata(assoc, GCA_SUPSYS, DB_INT_TYPE, 0,
			   sizeof(i4_buff), (PTR)&i4_buff);
  rqu_putflush(assoc, TRUE);

  RQTRACE(rqs,1)(RQFMT(rqs,
	      "RQF:Assn %d. MODIFY_ASSN. GCA_SUPSYS -> %s.\n"),
	      assoc->rql_assoc_id,
	      (i4_buff == GCA_ON) ? "GCA_ON" : "GCA_OFF" );

  /* check for association modification request acceptance */
  if (rqu_get_reply(assoc, -1) != GCA_ACCEPT)
  {
      RQTRACE(rqs,1)(RQFMT(rqs,
	"RQF:Assn %d. MODIFY_ASSN Failed.\n"), assoc->rql_assoc_id);
      return(NULL);
  }
  assoc->rql_lid.dd_l1_ingres_b = ingres_b;
  return(assoc);
}

/*{
** Name: rqu_check_assoc  - ascertain association
**
** Description:
**
**	See if the specified site already has an association in
**	the session association list. If it does not and allocation
**	is required then a new association is created.
**
** Inputs:
**      rcb->
**      	rqr_session     session context
**      	rqr_1_site      site required
**	allocate		non-zero means allocation is required
**
** Outputs:
**      rcb->
**		rqr_error.err_code    RQF error code
**        
**	Returns:
**	    RQL_ASSOC *		      association info. NULL is cannot
**				      find it.
**	Exceptions:
**	    none
**
** Side Effects:
**	    session->rqs_current_site is set to the site association.
**	    This is used internally by RQF.
**
** History:
**      27-may-1988 (alexh)
**          created
**      22-jan-1989 (georgeg)
**          modified to pass rqu_new_assoc() username and effective username
*/
RQL_ASSOC *
rqu_check_assoc(
	RQR_CB	*rcb,
	i4	allocate)
{
    RQS_CB   	*session = (RQS_CB *)rcb->rqr_session;
    DD_LDB_DESC  	*site = rcb->rqr_1_site;

    if (session==NULL)
    {
	    rcb->rqr_error.err_code = E_RQ0006_CANNOT_GET_ASSOCIATION;
	    return NULL;
    }
    /*
    ** else check to see if the requested site has been opened 
    */
    session->rqs_current_site = rqu_find_lid(site, session->rqs_assoc_list);
      
    if (session->rqs_current_site == NULL)
    {
	if (allocate)
	{
	    /*
	    ** allocate new association if allocate flag is set 
	    */
	    session->rqs_current_site = rqu_new_assoc(rcb, session, site,
			    (DD_USR_DESC *)&session->rqs_user, 
			    (DD_COM_FLAGS *)&session->rqs_com_flags,
			    (RQL_ASSOC *)NULL,
			    (DD_DX_ID *)NULL
			);

	    /* 
	    ** see if connection to the requested site has been opened 
	    */
	    if (session->rqs_current_site == NULL)
	    {
		 rcb->rqr_error.err_code = E_RQ0006_CANNOT_GET_ASSOCIATION;
		 rqu_error(0, rcb->rqr_error.err_code, (PTR) NULL,
		    (i4)3,
		    (i4)((char *)STindex(site->dd_l2_node_name, " ",
				    sizeof(site->dd_l2_node_name)) 
			    - (char *)site->dd_l2_node_name),
		    (i4 *)site->dd_l2_node_name,
		    (i4)((char *)STindex(site->dd_l3_ldb_name, " ", 
				    sizeof(site->dd_l3_ldb_name) ) 
			    - (char *)site->dd_l3_ldb_name),
		    (i4 *)site->dd_l3_ldb_name,
		    (i4)((char *)STindex(site->dd_l4_dbms_name, " ",
				    sizeof(site->dd_l4_dbms_name)) 
			    - (char *)site->dd_l4_dbms_name),
		    (i4 *)site->dd_l4_dbms_name,
		    (i4)0, (i4 *)NULL
		  );
	    }
	}
	else
	{
	  rcb->rqr_error.err_code = E_RQ0006_CANNOT_GET_ASSOCIATION;
	}
    }
    else
    {
	(( RQL_ASSOC *)session->rqs_current_site)->rql_re_status = 
			    (i4)RQF_00_NULL_STAT; 
	(( RQL_ASSOC *)session->rqs_current_site)->rql_timeout = 
			   (rcb->rqr_timeout > 0 ? rcb->rqr_timeout : 0);
	(( RQL_ASSOC *)session->rqs_current_site)->rql_rqr_cb = rcb;
    }

    return(session->rqs_current_site);
}

/*{
** Name: rqu_get_reply  - get a response or return data
**
** Description:
**	A message is read and interpreted. Message type is returned.
**	If GCA_ERROR, GCA_TRACE or GCA_EVENT is encoutered it should be sent to
**	client and the routine should call itself recursively.
**
** Inputs:
**      assoc->		association structure
**		rql_rv	received buffer
**
** Outputs:
**      assoc->
**		rql_rv	received buffer
**	*rq_status	if GCA_ERROR is recceived, *rq_status points
**			to error message.
**        
**	Returns:
**	    i4		message type of repsponse
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-jun-1988 (alexh)
**          created
**      15-feb-1989 (georgeg)
**          modified it to report LDB erorrs
**      21-may-1989 (carl)
**	    modified to detect and accept LDB warning as good status 
**	31-Jan=1991 (fpang)
**	    Added GCA_EVENT support.
**	13-sep-1991 (fpang)
**	    If gca_error block has generic error of
**	    GE_OK or GE_WARNING, treat it like an informational or warning
**	    message.
**	18-jan-1993 (fpang)
**	    GCA protocol 60 support. Retproc is no longer a turn-around 
**	    message.
**	18-jan-1993 (fpang)
**	    GCA protocol level 60 support. Detect messages and raised errors
**	    from database procedures from sqlstates.
**	05-feb-1993 (fpang)
**	    Added support for detecting commits/aborts when executing a 
**	    a procedure or in direct execute immediate.
**      08-oct-1992 (fpang)
**          Fixed B54385, (Create ..w/journaling fails). Recognize
**          SS000000_SUCCESS as a successful status message.
*/


i4
rqu_get_reply(
	RQL_ASSOC	*assoc,
	i4		timeout)
{
    GCA_RV_PARMS	*receive = &assoc->rql_rv;
    STATUS		status;
    i4			n_errs = 0;
    i4			iack_count = 0;	
    RQS_CB	        *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;
    GCA_RE_DATA		*re;
    i4		rqstat;

  /*
  ** stay in this loop and flush out all the GCA_ERROR, GCA_EVENT, and
  ** GCA_TRACE messages
  */

  if (assoc->rql_status != RQL_OK)
  {
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. COMM error, "), assoc->rql_assoc_id);
	RQTRACE(rqs,1)(RQFMT(rqs,
		"rqc_rxtx=0x%x, rqc_status=0x%x, rql_status=0x%x\n"),
	            assoc->rql_comm.rqc_rxtx,
	            assoc->rql_comm.rqc_status, 
		    assoc->rql_status);
	return(GCA_ERROR);
  }
  do
  {
	assoc->rql_turned_around = FALSE;
	receive->gca_flow_type_indicator = GCA_NORMAL;
	if (timeout == 0)	/* peeking read */
	{
	    /*
	    ** this is to see if anything is coming. 
	    */
	    IIGCa_call(GCA_RECEIVE,
		    (GCA_PARMLIST *)receive, GCA_SYNC_FLAG, 
		    (PTR)0, (i4)timeout, &status);
	    if ( receive->gca_status != E_GC0000_OK )
	    {
		switch ( receive->gca_status )
		{
		    case E_GC0020_TIME_OUT:
			/* this may be too kludgy */
			return(GCA_MAXMSGTYPE+1);
			break;
		    default:
			return(GCA_ERROR);
			break;
		} /* end switch */
	    }
	}
	else
	{
	    status = rqu_gccall(GCA_RECEIVE, (PTR)receive, &receive->gca_status,
				assoc);
	    if ( status != E_DB_OK )
	    {
		if ( 
		    (assoc->rql_comm.rqc_status == RQC_GCA_ERROR)
			||
		    (assoc->rql_comm.rqc_status == RQC_ASSOC_TERMINATED)
		   ) 
		{
		    /*
		    ** if there is an GCA communications error we mark the 
		    ** association as unusable and a higher level will 
		    ** terminate it.
		    */
		    assoc->rql_status = RQL_TERMINATED;
		}
		else if ( assoc->rql_comm.rqc_status == RQC_INTERRUPTED )
		{
		    assoc->rql_interrupted = TRUE;
		}
		else if ( assoc->rql_comm.rqc_status == RQC_TIMEDOUT )
		{
		    assoc->rql_status = RQL_TIMEDOUT;
		}
		else
		{
		    assoc->rql_status = RQL_TERMINATED;
		}		
		return( GCA_ERROR );
	    }
	} /* end if (timeout == 0) */

	/* log the GCA receipt */
	(VOID)rqu_gcmsg_type(GCA_RECEIVE, receive->gca_message_type, assoc);

	switch(receive->gca_message_type)
	{
	    case GCA_RELEASE:
	    case GCA_REJECT:
	    case GCA_ERROR:
	    	/* The datatype of these messages is GCA_ER_DATA.
	    	** GCA_ERROR is never the last reply, and there may be more than
	    	** one of them. Do another read if the message is GCA_ERROR. The
	    	** last message is usually a GCA_REAPONSE.
	    	*/
		rqu_gca_error( assoc, (GCA_ER_DATA *)receive->gca_data_area );
		if (assoc->rql_status == RQL_OK )
		{
		    GCA_ER_DATA	*er = (GCA_ER_DATA *)receive->gca_data_area;
		    /*
		    ** The LDB is reporting an error, note the first one, QEF 
		    ** probably knows how to deal with it.
		    */
		    assoc->rql_status = RQL_LDB_ERROR;
		    if ( er->gca_l_e_element > 0 )
		    {
			assoc->rql_rqr_cb->rqr_1ldb_severity 
				= assoc->rql_ldb_severity  
					= er->gca_e_element->gca_severity;
			assoc->rql_rqr_cb->rqr_2ldb_error_id 
				= assoc->rql_id_error  
					= er->gca_e_element->gca_id_error;
			assoc->rql_rqr_cb->rqr_3ldb_local_id 
				= assoc->rql_local_error 
					= er->gca_e_element->gca_local_error;
		    }
		    else
		    {
			assoc->rql_local_error = E_DB_OK;
			assoc->rql_ldb_severity = 0;
			assoc->rql_id_error = E_DB_OK;
 		    }
		}

		if (   (receive->gca_message_type == GCA_RELEASE) 
		    || (receive->gca_message_type == GCA_REJECT) )
		{
		    assoc->rql_turned_around = TRUE;
		    assoc->rql_status = RQL_TERMINATED;
		    assoc->rql_comm.rqc_status = RQC_ASSOC_TERMINATED;
		}
		else	/* GCA_ERROR */
		{
		    i4 severity = assoc->rql_ldb_severity;
		    i4 protocol = assoc->rql_peer_protocol;
		    i4 id_error = assoc->rql_id_error;
		    n_errs++;
		    /*
		    ** if the error is a warning/message, ignore it, and don't 
		    ** tell QEF and continue. If the generic error is GE_OK or 
		    ** GE_WARNING, this is an informational or warning message.
		    **
		    ** See comment in rqu_gca_error() regarding how to detect
		    ** a message or raised error coming from a database 
		    ** procedure.
		    */
		    if( (severity & GCA_ERWARNING) 
		      ||(severity & GCA_ERMESSAGE) 
		      ||( (assoc->rql_status == RQL_LDB_ERROR)
			&&(((protocol < GCA_PROTOCOL_LEVEL_60) 
			  &&((id_error == GE_OK) || (id_error == GE_WARNING)))
			 ||((protocol >= GCA_PROTOCOL_LEVEL_60) 
			  &&((id_error == ss5000g_dbproc_message)
			   ||(id_error == ss50009_error_raised_in_dbproc)
			   ||(id_error == ss00000_success))))))
		    {
			assoc->rql_local_error = E_DB_OK;
			assoc->rql_ldb_severity = 0;
			assoc->rql_id_error = E_DB_OK;
			assoc->rql_status = RQL_OK;
			n_errs--;
		    }
		}
		break;
	    case GCA_EVENT:
	    case GCA_TRACE:
		/* relay the trace message to client */
		rqu_relay(assoc, receive);
		break;
	    /*
	    ** the following reply messages imply the flow has turned around 
	    ** this is used to detect redundant interrupt messages 
	    ** There are others but they are considered relevant 
	    */
	    case GCA_RESPONSE:
	    case GCA_REFUSE:
	    case GCA_DONE:
		re =  (GCA_RE_DATA *)assoc->rql_rv.gca_data_area;
		rqstat = re->gca_rqstatus;
		assoc->rql_re_status = RQF_00_NULL_STAT;
		if ((rqstat & GCA_LOG_TERM_MASK) == GCA_LOG_TERM_MASK)
		{
		    assoc->rql_re_status |= RQF_03_TERM_MASK;
		}

		if ((rqstat & GCA_INVSTMT_MASK) == GCA_INVSTMT_MASK )
		{
		    assoc->rql_re_status |= RQF_02_STMT_MASK;
		}	

		if ((rqstat & GCA_FAIL_MASK) == GCA_FAIL_MASK )
		{
		    assoc->rql_re_status |= RQF_01_FAIL_MASK;
		}

		if ((rqstat & GCA_ILLEGAL_XACT_STMT) == GCA_ILLEGAL_XACT_STMT )
		{
		    assoc->rql_re_status |= RQF_04_XACT_MASK;
		}

		RQTRACE(rqs,3)(RQFMT(rqs,
			    "RQF:Assn %d. QUERY status 0x%x, "),
			    assoc->rql_assoc_id, rqstat);
		RQTRACE(rqs,3)(RQFMT(rqs,
			"rql_re_status 0x%x,"), assoc->rql_re_status); 
		RQTRACE(rqs,3)(RQFMT(rqs,
				"row_count %d\n"),
				re->gca_rowcount);
		/* Fall through */
	    case GCA_ATTRIBS:
	    case GCA_S_BTRAN:
	    case GCA_S_ETRAN:
	    case GCA_ACCEPT:
		assoc->rql_turned_around = TRUE;
		break;
	    case GCA_IACK:
		assoc->rql_turned_around = TRUE;
		RQTRACE(rqs,1)(RQFMT(rqs,
	"RQF:Assn %d. PROTOCOL VIOLATION, recvd GCA_IACK out of sequence.\n"),
							assoc->rql_assoc_id);
		if (++iack_count > 10)
		{
		    RQTRACE(rqs,1)(RQFMT(rqs,
	"RQF:Assn %d. PROTOCOL VIOLATION, too many GCA_IACKs.\n"), 
							assoc->rql_assoc_id);
		    return(receive->gca_message_type);
		}
		break;
	    case GCA_RETPROC:
		/* If ldb protocol < 60, then GCA_RETPROC is a turnaround
		** message. Otherwise, it's a midstream message.
		*/
		if (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_60)
		{
		    assoc->rql_turned_around = TRUE;
		}
		break;
	    default:
		break;
	} 
	/*
	** do the same if the response was an GCA_ERROR
	*/
    } while ( 
	      receive->gca_message_type == GCA_ERROR 
		|| 
	      receive->gca_message_type == GCA_TRACE 
		||
	      receive->gca_message_type == GCA_IACK
		||
	      receive->gca_message_type == GCA_EVENT
	    ); 
  
    /*
    ** If there is an GCA_ERROR detected then return GCA_ERROR
    ** if there was no GCA_ERROR detected during loop
    ** return the most resent gca_message
    */
    if ( n_errs )
    {
	return( GCA_ERROR );
    }
    else
    {
	return(receive->gca_message_type);
    }  
}


/*
** {
** Name:  rqu_create_tmp - synthesize a CREATE TABLE statement to stream
**
** Description:
**
**	Generate a CREATE TABLE statement, either SQL or QUEL, to a message
**	stream from a tuple description. The statement is sent to an LDB.
**
**	Current logic does not handle message-spanning tuple description.
**
** Inputs:
**
**	src_assoc	association which has the tuple description
**	dest_assoc	association to which the CREATE stm will be sent
**	lang		target language
**	col_name_table	table of the column names of the table.
**	table_name	table name of the tbale
**	page_size	page size of the tbale
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      27-Oct-1988 (alexh)
**          created
**	15-oct-1992 (fpang)
**	    Cleaned up RQTRACE messages.
**	13-jan-1993 (fpang)
**	    Added temp table support. If LDB supports temp tables, use
**	    'declare global temporary table..' instead of 'create 
**	    table..'.
**       6-Oct-1993 (fred)
**          Changed call to rqu_putformat() to accept an entire db
**          datavalue.  This should make decimal work.
**      14-Oct-1993 (fred)
**          Changed call to rqu_fetch_data() to add new parameters.
**      06-mar-1996 (nanpr01)
**          Changed the parameter to pass page size of the temporary table.
**	28-Jan-2009 (kibro01)
**	    Pass in ingresdate capability to rqu_putformat
*/
DB_ERRTYPE
rqu_create_tmp(
	RQL_ASSOC	*src_assoc,
	RQL_ASSOC	*dest_assoc,
	i4		lang,
	DD_ATT_NAME	**col_name_tab,
	DD_TAB_NAME	*table_name,
	i4		pagesize)
{
    GCA_TD_DATA		tup_desc;
    bool		eod;
    i4			col_idx;
    DB_ERRTYPE		status;
    char		create_header_format[100]; 
    char		col_descr_format[20];
    char		qbuffer[100 + sizeof(DD_TAB_NAME) 
			    + sizeof(DD_ATT_NAME)]; /* buf for create qry */
    RQS_CB	        *rqs = (RQS_CB *)src_assoc->rql_rqr_cb->rqr_session;
    DD_PACKET		ddpkt;
    bool		use_tmptbl = FALSE;
    bool		use_ingresdate = FALSE;

    /* Use LDB descriptor of the request block instead of the current site
    ** because the bit may not be set in the current site. QEF will set the
    ** bit in the LDB descriptor of the control block if it want's us to use
    ** temp tables.
    */
    if (dest_assoc->rql_rqr_cb->rqr_1_site->dd_l6_flags & DD_04FLAG_TMPTBL)
	use_tmptbl = TRUE;
    if (dest_assoc->rql_rqr_cb->rqr_1_site->dd_l6_flags & DD_05FLAG_INGRESDATE)
	use_ingresdate = TRUE;

    RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. TRANSFER to Assn %d.\n"),
		src_assoc->rql_assoc_id, dest_assoc->rql_assoc_id);

    /*
    ** tuple description, if any, should have been fetched during
    ** query execution.
    */
    if (src_assoc->rql_rv.gca_message_type != GCA_TDESCR)
    {
	/*
	** has to be a GCA_TDESCR message with non-zero column count 
	*/
	return(E_RQ0002_NO_TUPLE_DESCRIPTION);
    }

    /* initialize fetch stream */
    src_assoc->rql_described = TRUE;
    src_assoc->rql_b_ptr = src_assoc->rql_rv.gca_data_area;
    src_assoc->rql_b_remained = src_assoc->rql_rv.gca_d_length;

    /* fetch description header */
    status = rqu_fetch_data(src_assoc, sizeof(GCA_TD_DATA)-sizeof(GCA_COL_ATT),
			  (PTR)&tup_desc, &eod, 0, 0, 0);

    if (status != E_RQ0000_OK || eod == TRUE)
    {
	return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }

    if (tup_desc.gca_l_col_desc == 0)
    {
	return(E_RQ0001_WRONG_COLUMN_COUNT);
    }

    if (use_tmptbl == FALSE)
        STprintf(create_header_format,"create table %%.%ds(", sizeof(DD_TAB_NAME));
    else
	STprintf(create_header_format,
		 "declare global temporary table %%.%ds(",  sizeof(DD_TAB_NAME));

    if (lang == DB_QUEL)
    {
	STprintf(col_descr_format," %%.%ds = ", sizeof(DD_ATT_NAME));
    }
    else
    {
	STprintf(col_descr_format," %%.%ds ", sizeof(DD_ATT_NAME));
    }

    STprintf(qbuffer, create_header_format, table_name);

    ddpkt.dd_p1_len   = STlength(qbuffer);
    ddpkt.dd_p2_pkt_p = qbuffer;
    ddpkt.dd_p3_nxt_p = (DD_PACKET *)NULL;
    ddpkt.dd_p4_slot  = DD_NIL_SLOT;

    status = rqu_put_qtxt(dest_assoc, GCA_QUERY, lang, 0,
			  &ddpkt, (DD_TAB_NAME *)NULL);

    for (col_idx = 0;
       col_idx < tup_desc.gca_l_col_desc && status == E_RQ0000_OK;
       col_idx++)
    {
	DB_DATA_VALUE	col_desc;

	/* fetch a column description */
	status = rqu_col_desc_fetch(src_assoc, &col_desc);
	if (status != E_RQ0000_OK)
	{
	    return(status);
	}

	STprintf(qbuffer, col_descr_format, col_name_tab[col_idx]);

	rqu_putformat(&col_desc, lang, qbuffer, use_ingresdate);

	if (col_idx+1 < tup_desc.gca_l_col_desc)
	{
	    STcat(qbuffer, ",");
	}
	else
	{
	    STcat(qbuffer, ") ");
	}  

	status = rqu_putdb_val(DB_QTXT_TYPE, 0, STlength(qbuffer), 
			     qbuffer, dest_assoc);
     }

    if (use_tmptbl == TRUE)
    {
	STprintf(qbuffer, " on commit preserve rows with norecovery ");
	status = rqu_putdb_val(DB_QTXT_TYPE, 0, STlength(qbuffer),
			       qbuffer, dest_assoc);
    }

    if (pagesize > DB_MIN_PAGESIZE)
    {
      if (use_tmptbl == TRUE)
      {
	STprintf(qbuffer, " , page_size = %d ", pagesize);
	status = rqu_putdb_val(DB_QTXT_TYPE, 0, STlength(qbuffer),
			       qbuffer, dest_assoc);
      }
      else
      {
	STprintf(qbuffer, " with page_size = %d ", pagesize);
	status = rqu_putdb_val(DB_QTXT_TYPE, 0, STlength(qbuffer),
			       qbuffer, dest_assoc);
      }
    }

    if (status == E_RQ0000_OK)
    {
	status = rqu_putflush(dest_assoc, TRUE);
    }
 
    return(status);
}

/*{
** Name: rqu_xsend - send a externally formatted message
**
** Description:
**      send a externally formatted message, that is message not formated
**	by RQF. This is usually a message received from front-end.
** Inputs:
**      assoc		association
**	msg		formatted message
**	msg_len		length of the message
**	msg_cont	message continuation
**	msg_type	GCA message type
**
** Outputs:
**          STATUS
**        
**	Returns:
**          none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      6-jun-1988 (alexh)
**          created
*/
STATUS
rqu_xsend(
	RQL_ASSOC	*assoc,
	GCA_SD_PARMS	*send)
{
    STATUS	status;
    VOID	rqu_dmp_gcaqry();

    send->gca_association_id = assoc->rql_assoc_id;
    rqu_gcmsg_type(GCA_SEND, send->gca_message_type, assoc);
    if (assoc->rql_comm.rqc_status == RQC_INTERRUPTED)
    {
	/*
	** in direct connect mode and the previous message was interrupted.
	** this better be an GCA_ATTENTION message.
	*/
	(VOID)rqu_txinterrupt(GCA_SEND, (PTR)send, &send->gca_status, assoc);
	return(E_RQ0000_OK);	
    }
    rqu_dmp_gcaqry(send,assoc);
    status = rqu_gccall(GCA_SEND, (PTR)send, &send->gca_status, assoc);
    if (status == E_DB_OK)
    {
	return(E_RQ0000_OK);
    }
    else if (assoc->rql_comm.rqc_status == RQC_INTERRUPTED)
    {
	assoc->rql_status = RQL_OK;
	assoc->rql_interrupted = TRUE;
    }
    return(E_RQ0012_INVALID_WRITE);
}

/*{
** Name: rqu_mod_assoc - construct and send current association flags
**
** Description:
**         construct the standard association parameters. Catalog
**	   update access is an optional parameters.
**         In the future this routine should derive association
**	   parameters from the STAR server session parameters.
** Inputs:
**      session         session context
**      assoc		allocated association struct
**	dbname		database to be associated
**
** Outputs:
**          none
**        
**	Returns:
**	    E_DB_OK	if association parameters are accepted
**	    E_DB_ERROR	if association parametrs are NOT accepted.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-jul-1988 (alexh)
**          created
**	17-jan-1989 (georgeg)
**	    added effective user support (-u flag)
**	30-aug-1989 (carl)
**	    fixed access violation problem of flags->dd_co26_msign[0].
**	07-nov-1989 (georgeg)
**	    added 2pc support.
**	09-may-1991 (fpang)
**	    Allow -A application flags to be sent to priviledged cdb.
**	03-aug-1991 (fpang)
**	    GCA_EXCLUSIVE needs a constant 0 passed.
**	    Also, make sure count is correct if -A is present.
**	13-aug-1991 (fpang)
**	    Support for GCA_GW_PARMS, and GCA_TIMEZONE.
**	04-sep-1991 (fpang)
**	    If GCA_XUPSYS is requested on user CDB, change it to GCA_SUPSYS
**	    so catalogs can still be modified.
**	29-oct-1991 (fpang)
**	    Send GCA_GW_PARMS and GCA_TIMEZONE to ldb only if it's protocol
**	    level is greater than 2.
**	27-dec-1991 (fpang)
**	    GCA_TIMEZONE if and only if GCA_PROTOCOL_LEVEL >= 31.
**	    Fixes B41601.
**      19-may-1993 (fpang)
**          Pass the GCA_DECIMAL value in a char instead of an i4.
**          Fixes B51918 (setting GCA_DECIMAL gets ldb connect errors.)
**      12-aug-1993 (stevet)
**          Added support for GCA_STRTRUNC to detect string truncation.
**	16-Sep-1993 (fpang)
**	    Added support for -G (GCA_GRPID) and -R (GCA_APLID).
**	3-oct-93 (robf)
**	    Pass label type to LDB for secure systems
**	22-nov-93 (robf)
**          Pass GCA_RUPASS (user password) if specified from frontend
**	    Pass label type (GCA_SL_TYPE) to LDB for secure systems
**	11-Oct-1993 (fpang)
**	    Added support for -kf (GCA_DECFLOAT).
**	    Fixes B55510 (Star server doesn't support -kf flag).
**	08-Dec-1993 (fpang)
**	    Dump number of parms (gca_l_user_data) when tracing parameters.
**	1-mar-94 (robf)
**          Trigger forwarding security labels on new facility flag rather
**	    than checking SLhaslabels(). For C2 servers on installations
**	    with security labels but no B1 we shouldn't forward security labels
**      20-Apr-1994 (daveb) 33593
**          Always send gw_parms if proto > 2, even if INGRES LDB.
**  	    This sends the client info provided by libq as part of
**  	    the referenced bug fix to the ldb.
**	9-may-94 (robf)
**           Update tracing to use swm's new interface.
**	14-nov-95 (nick)
**	    Add support for II_DATE_CENTURY_BOUNDARY.
**  29-may-98 (kitch01)
**		Bug 90540. If II_DATE_CENTURY_BOUNDARY is set on the client but the peer
**		does not support this flag via GCA subtract 1 from number of parms
**		(gca_l_user_data). This prevents the peer from reporting E_US0022 error.
**	03-aug-2007 (gupsh01)
**	    Modified to support date_alias.
**	24-aug-2007 (toumi01) bug 119015
**	    Add missing decrement of gca_l_user_data for date_alias based
**	    on protocol level.
*/
DB_STATUS
rqu_mod_assoc(
	RQS_CB		*session,
	RQL_ASSOC	*assoc,
	char		*dbname,
	DD_COM_FLAGS	*flags,
	DD_DX_ID	*restart_dx_id)
{
    /*
    ** In absence of a real SCF directives, we send GCA_VERSION, GCA_QLANGUAGE,
    ** GCA_DBNAME, optional GCA_SUPSYS to the ldb server.
    */

    i4		i4_buff;
    GCA_SESSION_PARMS	*parms;
    bool		special_assoc = FALSE;
    RQS_CB	        *rqs = session;
    bool		ingres_ldb = FALSE;

    parms = (GCA_SESSION_PARMS *)assoc->rql_sd.gca_buffer;
    /* the count must be accurate and pre-calculated 
    **
    ** The connection to the iidbdb and the priv association to the CDB are 
    ** concidered special and we do not need to send all the command flags 
    ** that the FE tell as.
    */
    if ( MEcmp( dbname, "iidbdb", (sizeof("iidbdb")-1) ) == 0 )
    {
	parms->gca_l_user_data = 3;
	special_assoc = TRUE;
    }
    else if (assoc->rql_lid.dd_l1_ingres_b == TRUE) 
    {    
	parms->gca_l_user_data = 5;
	special_assoc = TRUE;
    }
    else
    {
	parms->gca_l_user_data = flags->dd_co0_active_flags + 3;

	if (MEcmp(assoc->rql_lid.dd_l4_dbms_name,"INGRES", 6) == 0)
	    ingres_ldb = TRUE;

	/*
	** If we are trying for the user CDB association, and 
	**  GCA_SUPSYS is requested set it, if we trying for
	**  some other LDB skip it.
	*/
	if ( (session->rqs_user_cdb != assoc)
	    && (flags->dd_co29_supsys != -1) 
	    )
	{
		parms->gca_l_user_data--;
	}
	/*
	** We cannot allow the user CDB association to be locked
	**  in an exclusive mode
	*/
	if ( (flags->dd_co1_exlusive == TRUE)
	    && (session->rqs_user_cdb == assoc)
	    )
	{
		parms->gca_l_user_data--;
	}
	/*
	** The -A flag taken care of below, don't count it now.
	*/
	if ( flags->dd_co2_application != (DBA_MIN_APPLICATION -1))
	{
		parms->gca_l_user_data--;
	}
	/*
	** Take care of gw parms.
	** If user specified them, then we will pass them on
	** unless ldb protocol level is <= 2
	** If user did not specify them, then we will send
	** SQL_INGNORE_BAD_FLAGS if ldb protocol level is > 2.
	*/

	if (flags->dd_co32_gw_parms != NULL)
	{
	    if ( assoc->rql_peer_protocol <= GCA_PROTOCOL_LEVEL_2 )
		parms->gca_l_user_data--;
	}
	else
	{
	    if ( assoc->rql_peer_protocol > GCA_PROTOCOL_LEVEL_2 )
		parms->gca_l_user_data++;
	}
	
	if (  ((flags->dd_co34_tz_name[0] != EOS) || 
	       (flags->dd_co31_timezone != -1)) 
	   && (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_31))
	{
	    /* When both tzname and timezone are set, they count as one. */
	    parms->gca_l_user_data--;
	}

	if (  flags->dd_co35_strtrunc[0] != EOS
	   && assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_60)
	{
	    /* Send GCA_STRTRUNC only if protocol is 60 or over */
	    parms->gca_l_user_data--;
	}

	if ((flags->dd_co36_grpid[0] != EOS) &&
	    (assoc->rql_peer_protocol <= GCA_PROTOCOL_LEVEL_2))
	{
	    /* Forward -G (GCA_GRPID) if peer protocol > 2 */
	    parms->gca_l_user_data--;
	}

	if ((flags->dd_co40_usrpwd[0] != EOS) &&
	    (assoc->rql_peer_protocol <= GCA_PROTOCOL_LEVEL_2))
	{
	    /* Forward -P (GCA_RUPASS) if peer protocol > 2 */
	    parms->gca_l_user_data--;
	}

	if ((flags->dd_co37_aplid[0] != EOS) &&
	    (assoc->rql_peer_protocol <= GCA_PROTOCOL_LEVEL_2))
	{
	    /* Forward -R (GCA_APLID) if peer protocol > 2 */
	    parms->gca_l_user_data--;
	}

	if ((flags->dd_co38_decformat[0] != EOS) &&
	    (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_60))
	{
	    /* Forward -kf (GCA_DECFLOAT) if peer protocol >= 60 */
	    parms->gca_l_user_data--;
	}
	if ((flags->dd_co39_year_cutoff != TM_DEF_YEAR_CUTOFF)
		&& (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_62))
	{
		/* year cutoff only if peer protocol >= 62 */
	    parms->gca_l_user_data--;
	}
	if ((flags->dd_co42_date_alias[0] != EOS) &&
	    (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_67))
	{
	    /* date type protocol only for >= 67 */
	    parms->gca_l_user_data--;
	}
    }
    


    /* This is not really a loop, just something to BREAK out of.. */
    do
    {
	/*
	** if we are recovering 2pc transactions send GCA_RESTART
	*/
	if (restart_dx_id != NULL)
	{
	    parms->gca_l_user_data++;
	}

	/*
	** Count -A now..
	*/
	if ( flags->dd_co2_application != (DBA_MIN_APPLICATION -1))
	{
	    parms->gca_l_user_data++;
	}

	rqu_putinit(assoc, GCA_MD_ASSOC);
	(VOID)rqu_put_data(assoc, (PTR)&parms->gca_l_user_data, 
		sizeof(parms->gca_l_user_data));

	RQTRACE(rqs,3)(RQFMT(rqs,
		"RQF:Assn %d. MODIFY_ASSN, # session parms (%d):\n"),
		    assoc->rql_assoc_id, parms->gca_l_user_data);

	rqu_put_moddata(assoc, GCA_DBNAME, DB_CHR_TYPE, 0, 
			(i4)STlength(dbname), dbname);
	RQTRACE(rqs,3)(RQFMT(rqs,
			"\tGCA_DBNAME -> %s\n"),dbname);

	i4_buff = GCA_V_60;
	rqu_put_moddata(assoc, GCA_VERSION, DB_INT_TYPE, 0, 
		     (i4)sizeof(i4_buff), (PTR)&i4_buff);
	RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_VERSION -> GCA_V_60 (%d)\n"),i4_buff);

	rqu_put_moddata(assoc, GCA_QLANGUAGE, DB_INT_TYPE, 0, 
		(i4)sizeof(i4), (PTR)&session->rqs_lang);
	RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_QLANGUAGE -> (%d)\n"),session->rqs_lang);

	if (restart_dx_id != NULL)
	{
	    char    buff1[80], buff2[80];
	    i4 len;

	    (VOID)CVla(restart_dx_id->dd_dx_id1, buff1);
	    (VOID)CVla(restart_dx_id->dd_dx_id2, buff2);
	    (VOID)STpolycat(3, buff1, ":", buff2, buff1);
	    len = (i4)STlength(buff1);
	    rqu_put_moddata(assoc, GCA_RESTART, DB_CHR_TYPE, 0, 
			  (i4)len, buff1);
	    RQTRACE(rqs,3)(RQFMT(rqs,
				"\tGCA_RESTART -> Dx id (%s)\n"),
				buff1);
	}

	if ( flags->dd_co2_application != (DBA_MIN_APPLICATION -1))
	{
	    rqu_put_moddata(assoc, GCA_APPLICATION, DB_INT_TYPE, 0, 
		    (i4)sizeof(i4), (PTR)&flags->dd_co2_application);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		    "\tGCA_APPLICATION -> (%d)\n"),
		    flags->dd_co2_application);
	}

	/* set optional catalog update access GCA_SUPSYS */
	if (assoc->rql_lid.dd_l1_ingres_b == TRUE)
	{
	    /* 
	    ** if we are trying to establish a connection to the CDB with privs
	    **  on, establish
	    **  as STAR and then trnn the -u flag to '$INGRES'. By doing that
	    **  we will be able to update the system catalogs on behalf of the 
	    **  user.
	    */
	    static  char	ingres[] = "$ingres";

	    rqu_put_moddata(assoc, GCA_EUSER, DB_CHR_TYPE, 0, 
			    (i4)STlength(ingres), ingres);
	    RQTRACE(rqs,3)(RQFMT(rqs,
				"\tGCA_EUSER -> (%s)\n"),
				ingres);

	    i4_buff = GCA_ON;
	    rqu_put_moddata(assoc, GCA_SUPSYS, DB_INT_TYPE, 0, 
			(i4)sizeof(i4_buff), (PTR)&i4_buff);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_SUPSYS -> GCA_ON (%d)\n"),
		i4_buff);
	}

	/*
	** In the case where the the association is special we want to 
	** break out, we are finished.
	*/
	if ( special_assoc == TRUE )
	{
	    break;
	}

	/*
	** If we get down here this is a user association 
	*/

	if ( (flags->dd_co1_exlusive == TRUE)
	    && (session->rqs_user_cdb != assoc)
	    )
	{
	    i4_buff = (i4)0;
	    rqu_put_moddata(assoc, GCA_EXCLUSIVE, DB_INT_TYPE, 0, 
			(i4)sizeof(i4_buff), (PTR)&i4_buff);
	    RQTRACE(rqs,3)(RQFMT(rqs,
			"\tGCA_EXCLUSIVE -> (0)\n"));
	}

	if ( flags->dd_co4_cwidth != -1 )
	{
	    rqu_put_moddata(assoc, GCA_CWIDTH, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co4_cwidth);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_CWIDTH -> (%d)\n"),
		flags->dd_co4_cwidth );
	}

	if ( flags->dd_co5_twidth != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_TWIDTH, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co5_twidth);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_TWIDTH -> (%d)\n"),
		flags->dd_co5_twidth);
	}

	if ( flags->dd_co6_i1width != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_I1WIDTH, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co6_i1width);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_I1WIDTH -> (%d)\n"),
		flags->dd_co6_i1width);
	}

	if ( flags->dd_co7_i2width != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_I2WIDTH, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co7_i2width);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_I2WIDTH -> (%d)\n"),
		flags->dd_co7_i2width);
	}

	if ( flags->dd_co8_i4width != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_I4WIDTH, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co8_i4width);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_I4WIDTH -> (%d)\n"),
		flags->dd_co8_i4width);
	}

	if ( flags->dd_co9_f4width != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_F4WIDTH, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co9_f4width);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_F4WIDTH -> (%d)\n"),
		flags->dd_co9_f4width);
	}

	if ( flags->dd_co10_f4precision != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_F4PRECISION, DB_INT_TYPE, 0, 
		    (i4)sizeof(i4), (PTR)&flags->dd_co10_f4precision);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_F4PRECISION -> (%d)\n"),
		flags->dd_co10_f4precision);
	}

	if ( flags->dd_co11_f8width != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_F8WIDTH, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co11_f8width);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_F8WIDTH -> (%d)\n"),
		flags->dd_co11_f8width);
	}

	if ( flags->dd_co12_f8precision != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_F8PRECISION, DB_INT_TYPE, 0, 
		    (i4)sizeof(i4), (PTR)&flags->dd_co12_f8precision);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_F8PRECISION -> (%d)\n"),
		flags->dd_co12_f8precision);
	}

	if ( flags->dd_co13_nlanguage != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_NLANGUAGE, DB_INT_TYPE, 0, 
		    (i4)sizeof(i4), (PTR)&flags->dd_co13_nlanguage);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_NLANGUAGE -> (%d)\n"),
		flags->dd_co13_nlanguage);
	}

	if ( flags->dd_co14_mprec != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_MPREC, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co14_mprec);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_MPREC -> (%d)\n"),
		flags->dd_co14_mprec);
	}

	if ( flags->dd_co15_mlort != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_MLORT, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co15_mlort);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_MLORT -> (%d)\n"),
		flags->dd_co15_mlort);
	}

	if ( flags->dd_co16_date_frmt != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_DATE_FORMAT, DB_INT_TYPE, 0, 
		    (i4)sizeof(i4), (PTR)&flags->dd_co16_date_frmt);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_DATE_FORMAT -> (%d)\n"),
		flags->dd_co16_date_frmt);
	}

	if ( flags->dd_co17_idx_struct[0] != EOS )                   
	{
	    rqu_put_moddata(assoc, GCA_IDX_STRUCT, DB_CHR_TYPE, 0, 
		    (i4)flags->dd_co18_len_idx, flags->dd_co17_idx_struct);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_IDX_STRUCT -> (%t)\n"),
		flags->dd_co18_len_idx, flags->dd_co17_idx_struct);
	}

	if ( flags->dd_co19_res_struct[0] != EOS )                   
	{
	    rqu_put_moddata(assoc, GCA_RES_STRUCT, DB_CHR_TYPE, 0, 
		    (i4)flags->dd_co20_len_res, flags->dd_co19_res_struct);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_RES_STRUCT -> (%t)\n"),
		flags->dd_co20_len_res, flags->dd_co19_res_struct);
	}

	if ( flags->dd_co21_euser[0] != EOS )                   
	{
	    rqu_put_moddata(assoc, GCA_EUSER, DB_CHR_TYPE, 0, 
		    (i4)flags->dd_co22_len_euser, flags->dd_co21_euser);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_EUSER -> (%t)\n"),
		flags->dd_co22_len_euser, flags->dd_co21_euser);
	}

	if ( flags->dd_co23_mathex[0] != EOS )                   
	{
	    rqu_put_moddata(assoc, GCA_MATHEX, DB_CHR_TYPE, 0, 
		    (i4)sizeof(flags->dd_co23_mathex[0]), 
		    flags->dd_co23_mathex);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_MATHEX -> (%c)\n"),
		flags->dd_co23_mathex[0]);
	}

	if ( flags->dd_co24_f4style[0] != EOS )                   
	{
	    rqu_put_moddata(assoc, GCA_F4STYLE, DB_CHR_TYPE, 0, 
		    (i4)sizeof(flags->dd_co24_f4style[0]), 
		    flags->dd_co24_f4style);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_F4STYLE -> (%c)\n"),
		flags->dd_co24_f4style[0]);
	}

	if ( flags->dd_co25_f8style[0] != EOS )                   
	{
	    rqu_put_moddata(assoc, GCA_F8STYLE, DB_CHR_TYPE, 0, 
		    (i4)sizeof(flags->dd_co25_f8style[0]), 
		    flags->dd_co25_f8style);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_F8STYLE -> (%c)\n"),
		flags->dd_co25_f8style[0]);
	}

	if ( flags->dd_co26_msign[0] != EOS )                   
	{
	    rqu_put_moddata(assoc, GCA_MSIGN, DB_CHR_TYPE, 0, 
		    (i4)STlength(flags->dd_co26_msign), 
		    flags->dd_co26_msign);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_MSIGN -> (%t)\n"),
		STlength(flags->dd_co26_msign), flags->dd_co26_msign);
	}

	if ( flags->dd_co27_decimal != -1 )                   
	{
	    i1  dec_spec;
	    dec_spec = (i1)flags->dd_co27_decimal;
	    rqu_put_moddata(assoc, GCA_DECIMAL, DB_CHR_TYPE, 0, 
		    (i4)sizeof(dec_spec), (PTR)&dec_spec);
	    RQTRACE(rqs,3)(RQFMT(rqs,
				"\tGCA_DECIMAL -> (%d)\n"),
				dec_spec);
	}

	if ( flags->dd_co28_xupsys != -1 )                   
	{
      	    /*
      	    ** GCA_XUPSYS implies an exclusive lock. If we send this to
      	    ** the user CDB assoc, it will always fail because of the
      	    ** already established priv CDB assoc. So, change it to
      	    ** GCA_SUSPSYS, so catalogs can still be modified.
      	    */
	    rqu_put_moddata(assoc, 
		    (session->rqs_user_cdb != assoc) ? GCA_XUPSYS : GCA_SUPSYS,
		    DB_INT_TYPE, 0, (i4)sizeof(i4),
		    (PTR)&flags->dd_co28_xupsys);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\t%s -> (%d)\n"),
		(session->rqs_user_cdb != assoc) ? "GCA_XUPSYS" : "GCA_SUPSYS",
		flags->dd_co28_xupsys);
	}

	if (( flags->dd_co29_supsys != -1 ) 
	    && (session->rqs_user_cdb == assoc)
	    )
	{
	    rqu_put_moddata(assoc, GCA_SUPSYS, DB_INT_TYPE, 0, 
			(i4)sizeof(i4), (PTR)&flags->dd_co29_supsys);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_SUPSYS -> (%d)\n"),
		flags->dd_co29_supsys);
	}

	if ( flags->dd_co30_wait_lock != -1 )                   
	{
	    rqu_put_moddata(assoc, GCA_WAIT_LOCK, DB_INT_TYPE, 0, 
		    (i4)sizeof(i4), (PTR)&flags->dd_co30_wait_lock);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_WAIT_LOCK -> (%d)\n"),
		flags->dd_co30_wait_lock);
	}

	if ( assoc->rql_peer_protocol > GCA_PROTOCOL_LEVEL_2 ) 
	{
	    if (flags->dd_co32_gw_parms != NULL)
	    {
	        rqu_put_moddata(assoc, GCA_GW_PARM, DB_CHR_TYPE, 0, 
		    (i4)flags->dd_co33_len_gw_parms, 
		    flags->dd_co32_gw_parms);
	        RQTRACE(rqs,3)(RQFMT(rqs,
		    "\tGCA_GW_PARM -> (%t)\n"),
		    flags->dd_co33_len_gw_parms, flags->dd_co32_gw_parms);
	    }
	    else
	    {
		char *tmp_str = "SQL_IGNORE_BAD_FLAGS";
	        rqu_put_moddata(assoc, GCA_GW_PARM, DB_CHR_TYPE, 0, 
			      STlength(tmp_str) + 1, tmp_str);
	        RQTRACE(rqs,3)(RQFMT(rqs,
		    "\tGCA_GW_PARM -> (%s)\n"),
		    "SQL_IGNORE_BAD_FLAGS");
	    }
	}

	if ((flags->dd_co39_year_cutoff != TM_DEF_YEAR_CUTOFF)
		&& (assoc->rql_peer_protocol >= GCA_PROTOCOL_LEVEL_62))
	{
	    rqu_put_moddata(assoc, GCA_YEAR_CUTOFF, DB_INT_TYPE, 0, 
		(i4)sizeof(i4), 
		(PTR)&flags->dd_co39_year_cutoff);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_YEAR_CUTOFF -> (%d)\n"),
		flags->dd_co39_year_cutoff);
	}

	/* Handle TIMEZONE.
	** Conversion issue :
	**
	**  FE60      -> (TZ_NAME) -> STAR60 -> (TZ_NAME) -> DBMS60
	**  FE60      -> (TZ_NAME) -> STAR60 -> (TZ)      -> DBMS[31-59]
	**  FE[31-59] -> (TZ)      -> STAR60 -> (TZ_NAME) -> DBMS60
	**  FE[31-59] -> (TZ)      -> STAR60 -> (TZ)      -> DBMS[31-59]
	** Conversion from TZ to TZ_NAME already handled by scsinit.
	** Conversion from TZ_NAME to TZ will use TM with ADF sess CB.
	*/

	if (  ((flags->dd_co31_timezone != -1) ||
	       (flags->dd_co34_tz_name[0] != EOS))
	   && (assoc->rql_peer_protocol >= GCA_PROTOCOL_LEVEL_31) )
	{
	    if (assoc->rql_peer_protocol < GCA_PROTOCOL_LEVEL_60)
	    {
		/* Send timezone. Convert if only tz_name is set. */
		if (flags->dd_co31_timezone == -1)
		{
		    /* Convert tz_name to timezone with TM */
		    SCF_CB	scf_cb;
		    SCF_SCI	sci_list[1];
		    ADF_CB	*adf_cb;
		    i4		tz_sec;
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
		    
		    /* Get adf_cb from SCF, and call TM to get TZ in secs. */
		    scf_call( SCU_INFORMATION, &scf_cb );
		    tz_sec = TMtz_search( adf_cb->adf_tzcb, 
					  TM_TIMETYPE_GMT, TMsecs() );
		    /* Reverse sign, convert to minutes. */
		    flags->dd_co31_timezone = -tz_sec / 60;
		}
		rqu_put_moddata(assoc, GCA_TIMEZONE, DB_INT_TYPE, 0, 
				(i4)sizeof(i4), 
				(PTR)&flags->dd_co31_timezone);
		RQTRACE(rqs,3)(RQFMT(rqs,
				"\tGCA_TIMEZONE -> (%d)\n"),
				flags->dd_co31_timezone);
	    }
	    else
	    {
		/* Send tz_name. */
		rqu_put_moddata(assoc, GCA_TIMEZONE_NAME, DB_CHR_TYPE, 0, 
			    (i4)STlength(flags->dd_co34_tz_name),
				(PTR)&flags->dd_co34_tz_name);
		RQTRACE(rqs,3)(RQFMT(rqs,
				"\tGCA_TIMEZONE_NAME -> (%t)\n"),
				(i4)STlength(flags->dd_co34_tz_name), 
				(PTR)&flags->dd_co34_tz_name);
	    }
	}

	if ( flags->dd_co35_strtrunc[0] != EOS 
	   && assoc->rql_peer_protocol >= GCA_PROTOCOL_LEVEL_60 )          
	{
	    rqu_put_moddata(assoc, GCA_STRTRUNC, DB_CHR_TYPE, 0, 
		    (i4)sizeof(flags->dd_co35_strtrunc[0]), 
		    flags->dd_co35_strtrunc);
	    RQTRACE(rqs,3)(RQFMT(rqs,
			   "\tGCA_STRTRUNC -> (%c)\n"),
			   flags->dd_co35_strtrunc[0]);
	}

	/* Handle Date_type_alias */
        if (  (flags->dd_co42_date_alias[0] != EOS) &&
              (assoc->rql_peer_protocol > GCA_PROTOCOL_LEVEL_66))
        {
                /* Send ingres date_alias. */
                rqu_put_moddata(assoc, GCA_DATE_ALIAS, DB_CHR_TYPE, 0,
                            (i4)STlength(flags->dd_co42_date_alias),
                                (PTR)&flags->dd_co42_date_alias);
                RQTRACE(rqs,3)(RQFMT(rqs,
                                "\tGCA_DATE_ALIAS-> (%t)\n"),
                                (i4)STlength(flags->dd_co42_date_alias),
                                (PTR)&flags->dd_co42_date_alias);
        }

	/* Forward -P (GCA_RUPASS) if peer protocol > 60 */
	if ((flags->dd_co40_usrpwd[0] != EOS) &&
	    (assoc->rql_peer_protocol > GCA_PROTOCOL_LEVEL_60))
	{
	    rqu_put_moddata(assoc, GCA_RUPASS, DB_CHR_TYPE, 0, 
		    (i4)STnlength(sizeof(flags->dd_co40_usrpwd), 
				       flags->dd_co40_usrpwd), 
		    flags->dd_co40_usrpwd);
	    RQTRACE(rqs,3)(RQFMT(rqs,"\tGCA_RUPASS -> (Data not traced)\n"));
	}

	/* Forward -G (GCA_GRPID) if peer protocol > 2 */
	if ((flags->dd_co36_grpid[0] != EOS) &&
	    (assoc->rql_peer_protocol > GCA_PROTOCOL_LEVEL_2))
	{
	    rqu_put_moddata(assoc, GCA_GRPID, DB_CHR_TYPE, 0, 
		    (i4)STnlength(sizeof(flags->dd_co36_grpid), 
				       flags->dd_co36_grpid), 
		    flags->dd_co36_grpid);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_GRPID -> (%s)\n"),
		flags->dd_co36_grpid);
	}

	/* Forward -R (GCA_APLID) if peer protocol > 2 */
	if ((flags->dd_co37_aplid[0] != EOS) &&
	    (assoc->rql_peer_protocol > GCA_PROTOCOL_LEVEL_2))
	{
	    rqu_put_moddata(assoc, GCA_APLID, DB_CHR_TYPE, 0, 
		    (i4)STnlength(sizeof(flags->dd_co37_aplid),
					      flags->dd_co37_aplid), 
		    flags->dd_co37_aplid);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		"\tGCA_APLID -> (%s)\n"),
		flags->dd_co37_aplid);
	}
	if ((flags->dd_co38_decformat[0] != EOS) &&
	    (assoc->rql_peer_protocol >= GCA_PROTOCOL_LEVEL_60))
	{
	    rqu_put_moddata(assoc, GCA_DECFLOAT, DB_CHR_TYPE, 0, 
				    (i4)1, flags->dd_co38_decformat);
	    RQTRACE(rqs,3)(RQFMT(rqs,"\tGCA_DECFLOAT -> (%c)\n"),
						  flags->dd_co38_decformat[0]);
	}

  } while (0);

  rqu_putflush(assoc, TRUE);

  /* 
  ** check for association modification request acceptance 
  */
  switch ( rqu_get_reply( assoc, -1 ) )
  {
    case GCA_ACCEPT:
	break;
    case GCA_RELEASE:
    {
        RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. MODIFY_ASSN failed.\n"),
		assoc->rql_assoc_id);
	return(E_DB_ERROR);
    }
    default:
	return(E_DB_ERROR);
  }

  /* 
  ** if SQL and not RESTART then set autocommit off 
  */
  if (session->rqs_lang == DB_SQL && restart_dx_id == NULL)
  {

      DD_PACKET	    ddpkt;
      char	    autocommit_str[30];
      DB_ERRTYPE    status = E_RQ0000_OK;

      /* Now force the LDB session to autocommit off */
      STprintf(autocommit_str, "set autocommit off;");
      ddpkt.dd_p1_len   = STlength(autocommit_str);
      ddpkt.dd_p2_pkt_p = autocommit_str;
      ddpkt.dd_p3_nxt_p = (DD_PACKET *)NULL;
      ddpkt.dd_p4_slot = DD_NIL_SLOT;

      do
      {
	  /* send the query */
	  status = rqu_put_qtxt(assoc, GCA_QUERY, DB_SQL, 0, &ddpkt, NULL);
	  if (status != E_RQ0000_OK)
	      break;

	  /* flush it */
	  status = rqu_putflush(assoc, TRUE);
	  if (status != E_RQ0000_OK)
	      break;

	  /* get the reply */
	  if (rqu_get_reply(assoc, -1) != GCA_RESPONSE)
	     status = E_RQ0000_OK;
      } while (0);
      if (status != E_RQ0000_OK)
	  return (E_DB_ERROR);
  }

  return(E_DB_OK);
}

/*
** {
** Name: rqu_fetch_data - fetch data from assoc stream
**
** Description:
**	fetch specified size of data from the association.
**	If only partial data is available then return NULL.
**	Beware of the fact that data may span messages.
**	Only GCA_TUPLES and GCA_TDESCR are valid.
**
** Inputs:
**	assoc->
**		rql_b_remained	number of byte in the stream that
**				has not been fetched.
**	fsize			size to be fetched
**	dest			destiniation buffer. NULL means no
**				transfer, just skip data
**	eod_ptr			point to end of data flag
**	tuple_fetch             Is this a tuple or data element fetch
**	fetched_size            Location for amount of data fetched
**	eo_tuple                Location for end of tuple indicator
**
** Outputs:
**	assoc->
**		rql_b_remained	number of byte in the stream that
**				has not been fetched.
**	*dest			filled with fetch data if not NULL
**	*eod_ptr		FALSE if data fetched successfully;
**				TRUE otherwise.
**	*fetched_size           amount of data returned if
**				fetched_size != 0.
**	*eo_tuple               TRUE if at end of tuple; FALSE otherwise.
**      
**	Returns:
**		DB_ERRTYPE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      Oct-11-1988 (alexh)
**          created
**	May-27-1990 (georgeg)
**	    beautified the layout of the code and added GCA_C_INTO/FROM as 
**	    legal messages to fetch from.
**	15-oct-1992 (fpang)
**	    Removed recursion. Also recognize GCA1_C_FROM/INTO.
**      14-Oct-1993 (fred)
**          Added large object support.  Added new parameters for
**              size of data fetched,
**              end of tuple indicator, and
**              whether this is a tuple fetch.
**      28-Oct-1993 (fred)
**          Be more careful about cutting off messages midstream.  It
**          seems that STAR doesn't try to get whole messages, which
**          messes up the works when combined with large objects.
**          Keeping all the buffers, counts, and amounts straight...
*/
DB_ERRTYPE
rqu_fetch_data(
	       RQL_ASSOC	*assoc,
	       i4		fsize,
	       PTR		dest,
	       bool		*eod_ptr,
	       i4               tuple_request,
	       i4               *fetched_size,
	       i4               *eo_tuple)
{

    RQR_CB      *rqr = (RQR_CB *) assoc->rql_rqr_cb;
    RQS_CB	*rqs = (RQS_CB *) rqr->rqr_session;
    SCF_TD_CB   *scf_td;
    GCA_TD_DATA *td;
    i4		reply;
    i4  	amt;
    i4          los_present = 0;

    if( fsize < 0 )
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. ASKED to fetch %d bytes (negative).\n"),
	 	    assoc->rql_assoc_id, fsize );
	return(E_RQ0033_FETCH_FAILED);
    }

    *eod_ptr = FALSE;
    if (fetched_size)
	*fetched_size = 0;

    if (eo_tuple)
	*eo_tuple = TRUE;

    if (fsize == 0)
    {
	/* Nothing really to do */
	return (E_RQ0000_OK);
    }

    td = (GCA_TD_DATA *) 0;
    scf_td = (SCF_TD_CB *) rqr->rqr_tupdesc_p;
    
    if (tuple_request && scf_td)
    {
	td = (GCA_TD_DATA *) scf_td->scf_ldb_td_p;
	
	if (!td)
	{
	    td = (GCA_TD_DATA *) scf_td->scf_star_td_p;
	}
	if (td)
	{
	    los_present = (td->gca_result_modifier & GCA_LO_MASK);
	}
    }

    do {

	/* Get either what's requested or what's left */
        amt = ((i4)assoc->rql_b_remained < fsize) ? 
		assoc->rql_b_remained : fsize;

	if ((dest) && (amt))
	{
	    MEcopy((PTR)assoc->rql_b_ptr, amt, (PTR)dest);
	    dest += amt;
	}

	fsize -= amt;
	assoc->rql_b_remained -= amt;
        assoc->rql_b_ptr += amt;
	if (fetched_size)
	    *fetched_size += amt;

	if (tuple_request
	    && fetched_size
	    && (*fetched_size)
	    && los_present)
	{
	    break;
	    /* If we got some data, return that as a segment */
	}

	if (fsize == 0)	/* done */
	    break;

	/* Get next message */
	switch (reply = rqu_get_reply(assoc, -1))
	{
	    case GCA_TUPLES:
	    case GCA_TDESCR:
	    case GCA_C_FROM:
	    case GCA_C_INTO:
	    case GCA1_C_FROM:
	    case GCA1_C_INTO:
		/* this means more data is available*/
		assoc->rql_b_ptr = assoc->rql_rv.gca_data_area;
		assoc->rql_b_remained = assoc->rql_rv.gca_d_length;
		if (eo_tuple && los_present)
		    *eo_tuple = assoc->rql_rv.gca_end_of_data;
		
		break;
	    case GCA_RESPONSE:
		/* this signifys end of data */
		*eod_ptr = TRUE;
		break;
	    default:
		/* unexpected message */
		*eod_ptr = TRUE;
		break;
	}
    } while ((*eod_ptr == FALSE) && (fsize));

    if ((*eod_ptr == TRUE) && (reply != GCA_RESPONSE))
    {
	return (E_RQ0033_FETCH_FAILED);
    }

    if ((*eod_ptr == TRUE) && (amt != 0))
    {
	
	if (!td || ((td->gca_result_modifier & GCA_LO_MASK) == 0))
	{
	    /* This is a partial fetch, that did not complete, fail it. */
	    return (E_RQ0033_FETCH_FAILED);
	}
	
	/* 
	** When there are large objects, two changes are necessary.
	** 
	** First, it is legal (in fact, likely) for there to be
	** partial blocks.  This is the normal scenario.
	** 
	** Second, our caller will use the eo_tuple indicator to tell
	** whether to increment the tuple count or not.  Current local
	** SCF will never start tuples in the middle of a block, so it
	** is fine to calculate tuple count based on the number of
	** complete messages.
	*/
    }

    return (E_RQ0000_OK);
}

/*{
** Name:  rqu_getqid - get a GCA_ID from a message stream
**
** Description:
**
**	This is a utility routine to gett a GCA_ID from a stream.
**	This is mecessary since GCA_ID is not an atomic object
**	and thus cannot be sent as a whole. Note that the input
**	is of DB_CURSOR_ID, not GCA_ID.
**
** Inputs:
**
**	assoc		association of the message stream
**	qid		* DB_CURSOR_ID
**
** Outputs:
**          none
**        
**	Returns:
**		DB_ERRTYPE	see rqu_send.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-Dec-1988 (alexh)
**          created
**	15-oct-1992 (fpang)
**	    Cleaned up trace output.
*/
DB_ERRTYPE
rqu_getqid(
	RQL_ASSOC	*assoc,
	DB_CURSOR_ID	*qid)
{
  GCA_ID	*gca_qid = (GCA_ID *)assoc->rql_rv.gca_data_area;
  RQS_CB	*rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

  qid->db_cursor_id[0] = gca_qid->gca_index[0];
  qid->db_cursor_id[1] = gca_qid->gca_index[1];
  MEcopy((PTR)gca_qid->gca_name,
	 sizeof(qid->db_cur_name),
	 (PTR)qid->db_cur_name);

  /* see if fillers are necessary */
  if (sizeof(gca_qid->gca_name) < sizeof(qid->db_cur_name))
    MEfill((sizeof(qid->db_cur_name) - sizeof(gca_qid->gca_name)),
	   (unsigned char)' ', 
	   (PTR)&qid->db_cur_name[sizeof(gca_qid->gca_name)]);

  RQTRACE(rqs,2)(RQFMT(rqs,
	      "RQF:Assn %d. QID(fr local) : (%d, %d, %t)\n"),
	      assoc->rql_assoc_id,
	      gca_qid->gca_index[0],
	      gca_qid->gca_index[0],
	      sizeof(gca_qid->gca_name),
	      gca_qid->gca_name);
  return(E_RQ0000_OK);
}

/*{
** Name:  rqu_get_rpdata - get a GCA_RP_DATA from a message stream
**
** Description:
**
**	This is a utility routine to get a GCA_RP_DATA from a stream.
**	A GCA_RP_DATA consists of a GCA_ID followed by the return
**	status of the procedure.
**
** Inputs:
**
**	assoc		association of the message stream
**
** Outputs:
**
**	assoc->
**	    rqr_qid	   GCA_ID, LDB procedure id.
**	    rqr_dbp_rstat  i4, return status
**        
**	Returns:
**		DB_ERRTYPE	
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-Dec-1992 (fpang)
**          created
*/
DB_ERRTYPE
rqu_get_rpdata(
	RQL_ASSOC	*assoc,
	DB_CURSOR_ID	*pid,
	i4		*dbp_stat)
{
    DB_ERRTYPE   status = E_RQ0000_OK;
    GCA_RP_DATA  *rp  = (GCA_RP_DATA *)assoc->rql_rv.gca_data_area;
    RQS_CB	 *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

    pid->db_cursor_id[0] = rp->gca_id_proc.gca_index[0];
    pid->db_cursor_id[1] = rp->gca_id_proc.gca_index[1];

    MEmove(  sizeof(rp->gca_id_proc.gca_name), rp->gca_id_proc.gca_name,
	    (char) ' ', sizeof(pid->db_cur_name), pid->db_cur_name );

    *dbp_stat = rp->gca_retstat;

    RQTRACE(rqs,2)(RQFMT(rqs,
		"RQF:Assn %d. GCA_RP_DATA (%d %d, (%t)), gca_retstat = %d\n"),
		assoc->rql_assoc_id,
		pid->db_cursor_id[0], pid->db_cursor_id[1], 
		sizeof(pid->db_cur_name), pid->db_cur_name, *dbp_stat);

    return( status );
}

/*
** {
** Name: rqu_col_desc_fetch - fetch column description data from assoc stream
**
** Description:
**	This utility routines fetchs a column description.
**	It checks when current message is GCA_TDESCR.
**
** Inputs:
**	assoc	association
**	col	pointer to DB_DATA_VALUE for column description.
**
** Outputs:
**	*col	content is filled if fetched successfully.
**      
**	Returns:
**		DB_ERRTYPE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      Oct-31-1988 (alexh)
**          created
**      Apr-02-1990 (carl)
**          1.  extended to allow GCA_C_FROM so that rqu_cp_get can become
**              a client
**          2.  changed to copy DB_DATA_VALUE by fields
**      Apr-02-1990 (georgeg)
**	    Added GCA_C_INTO just in case we ever need it
**	15-oct-1992 (fpang)
**	    Added explicit return(E_DB_OK) at the bottom.
**      14-Oct-1993 (fred)
**          changed rqu_fetch_data() parameter list.
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE: use DB_COPY macro.
**/

DB_ERRTYPE
rqu_col_desc_fetch(
	RQL_ASSOC	*assoc,
	DB_DATA_VALUE	*col)
{
    GCA_COL_ATT	att;
    bool	eod;
    DB_ERRTYPE	status;

    if (! 
	 (
	  assoc->rql_rv.gca_message_type == GCA_TDESCR
	   ||
	  assoc->rql_rv.gca_message_type == GCA_C_FROM
	   ||
	  assoc->rql_rv.gca_message_type == GCA_C_INTO
	 )
       )
    {
	return(E_RQ0036_COL_DESC_EXPECTED);
    }
    /* copy GCA_COL_ATT by fields */

    status = rqu_fetch_data(assoc,
			    sizeof(att.gca_attdbv.db_data),
			    (PTR)&att.gca_attdbv.db_data,
			    &eod,
			    0,
			    0,
			    0);
    if (status != E_RQ0000_OK && eod == TRUE)
    {
	return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }

    status = rqu_fetch_data(assoc,
			    sizeof(att.gca_attdbv.db_length),
			    (PTR)&att.gca_attdbv.db_length,
			    &eod,
			    0,
			    0,
			    0);
    if (status != E_RQ0000_OK && eod == TRUE)
    {
	return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }
   
    status = rqu_fetch_data(assoc,
			    sizeof(att.gca_attdbv.db_datatype),
			    (PTR)&att.gca_attdbv.db_datatype,
			    &eod,
			    0,
			    0,
			    0);
    if (status != E_RQ0000_OK && eod == TRUE)
    {
	return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }

    status = rqu_fetch_data(assoc,
			    sizeof(att.gca_attdbv.db_prec),
			    (PTR)&att.gca_attdbv.db_prec,
			    &eod,
			    0,
			    0,
			    0);
    if (status != E_RQ0000_OK && eod == TRUE)
    {
	return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }

    status = rqu_fetch_data(assoc,
			    sizeof(att.gca_l_attname),
			    (PTR)&att.gca_l_attname,
			    &eod,
			    0,
			    0,
			    0);
    if (status != E_RQ0000_OK && eod == TRUE)
    {
	return(E_RQ0035_BAD_COL_DESC_FORMAT);
    }

    /* flush the attname */
    (VOID)rqu_fetch_data(assoc, att.gca_l_attname, NULL, &eod, 0, 0, 0);
    if (! 
	 (
	  assoc->rql_rv.gca_message_type == GCA_TDESCR
	   ||
	  assoc->rql_rv.gca_message_type == GCA_C_FROM
	   ||
	  assoc->rql_rv.gca_message_type == GCA_C_INTO
	 )
       )
    {
	return(E_RQ0036_COL_DESC_EXPECTED);
    }

    /* return dbv */
    DB_COPY_ATT_TO_DV( &att, col );
    return(E_RQ0000_OK);
}

/*{
** Name: rqu_td_save	- save tuple description in bind table
**
** Description:
**	copy column length and type info into bind table. These
**	info are needed for data conversion by ADF. Expected
**	column count and actual column count are checked.
**	This routine does handle multiple tuple description
**	message.
**
**	This routine assumes tuple descriptions are chopped at
**	a description boundary. THIS MAY NOT BE AN ACCEPTABLE
**	assumption. The savest way is to use rqu_get_data to get
**	a tuple description at a time.
**
** Inputs:
**	col_count	expected column count
**	bind		pointer to column binding table
**      assoc		current association
**
** Outputs:
**	bind->
**		rqb_r_length	actual column length
**		rqb_r_dt_id	actual column type
**		rqb_r_prec 	actual column precision
**        
**	Returns:
**      	E_RQ0002_NO_TUPLE_DESCRIPTION
**		E_RQ0001_TOO_FEW_COLUMNS
**		E_RQ0004_BIND_BUFFER_TOO_SMALL
**		E_RQ0003_TOO_MANY_COLUMNS
**		E_RQ0000_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-jul-1988 (alexh)
**          created
**	31-oct-1988 (alexh)
**	    use rqu_col_desc_fetch to handle message-spanning correctly
**      14-Oct-1993 (fred)
**          Changed rqu_fetch_data() parameter list.
**      05-Dec-2001 (hanal04) Bug 105723, INGSTR 42.
**          Copy precision details into the bind table for DB_DEC_TYPE
**          support in rqf_t_fetch().
*/
DB_ERRTYPE
rqu_td_save(
	i4		col_count,
	RQB_BIND	*bind,
	RQL_ASSOC	*assoc)
{
  bool		eod;
  DB_ERRTYPE	status;
  GCA_TD_DATA	tup_desc;
  DB_DATA_VALUE	col_desc;

  /*
    tuple description, if any, should have been fetched during
    query execution.
  */
  if (assoc->rql_rv.gca_message_type != GCA_TDESCR)
    /* has to be a GCA_TDESCR message */
      return(E_RQ0002_NO_TUPLE_DESCRIPTION);

  /* fetch description header */
  status = rqu_fetch_data(assoc, sizeof(GCA_TD_DATA)-sizeof(GCA_COL_ATT),
			  (PTR)&tup_desc, &eod, 0, 0, 0);

  if (status != E_RQ0000_OK || eod == TRUE)
    return(E_RQ0035_BAD_COL_DESC_FORMAT);

  if (tup_desc.gca_l_col_desc != col_count)
      return(E_RQ0001_WRONG_COLUMN_COUNT);

  while (tup_desc.gca_l_col_desc--)
    {
      /* fetch a column description */
      status = rqu_col_desc_fetch(assoc, &col_desc);
      if (status != E_RQ0000_OK)
	return(status);

      /* save the LDB returned info */
      bind->rqb_r_length = col_desc.db_length;
      bind->rqb_r_dt_id = col_desc.db_datatype;
      bind->rqb_r_prec = col_desc.db_prec;

      ++bind;
    }

  return(E_RQ0000_OK);
}

/*{
** Name: rqu_convert_data - perform TITAN catalog column data conversion
**
** Description:
**	Because of the discrepency between INGRES catalog and the 
**	standard catalog, data conversion is required for a few
**	cases. This is not meant to be a general data conversion
**	routine. Unknown cases will cause E_RQ0016_CONVERSION_ERROR.
**
**	It is not clear that adf can perform identical conversion.
**
** Inputs:
**      bind->
**	    rqb_addr;	    address of unconverted value
**   	    rqb_length;	    expected length of column 
**	    rqb_dt_id;	    expected data type of column
**	    rqb_r_length;   GCA length of column, for RQF's use
**	    rqb_r_dt_id;    GCA data type of column, for RQF's use
**
** Outputs:
**	bind->rqb_addr	    address of converted value
**      
**	Returns:
**		E_RQ0016_CONVERSION_ERROR
**		E_RQ0001_OK
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-Oct-1988 (alexh)
**          created
*/
DB_ERRTYPE
rqu_convert_data(
	RQB_BIND	*bind)
{
    bool	convert_ok = TRUE;
    RQS_CB	*rqs = (RQS_CB *)NULL;	/* For RQTRACE */

    switch (bind->rqb_dt_id)
    {
	case DB_CHA_TYPE:
	case DB_CHR_TYPE:
	    /*
	    ** convert char 1 to char 8 by blank-filling 
	    */
	    if (bind->rqb_r_length == 1 && bind->rqb_length == 8)
	    {
		MEfill(7, ' ', (PTR)((char *)bind->rqb_addr+1));
	    }
	    else if (
			(bind->rqb_r_length ==  bind->rqb_length) 
			    ||
			(bind->rqb_r_length == 32 && bind->rqb_length == 24)
		    )
	    {
		/*
		** convert char 32 to char 24 by doing nothing since it was already
		** truncated 
		*/
		;
	    }
	    else
	    {
		convert_ok = FALSE;

	    }
	    break;
	case DB_VCH_TYPE:
	    /*
	    ** buffer provied must be at least 1 larger than returning size
	    ** (for NULL terminators.) 
	    */
	    if (bind->rqb_r_length+1 <= bind->rqb_length)
	    {
		i2	varlen = *(i2 *)bind->rqb_addr;
		char	*strp = ((char *)bind->rqb_addr)+2;

		MEcopy((PTR)strp, varlen, (PTR)bind->rqb_addr);
		strp[varlen] = 0;
	    }
	    else
	    {
		convert_ok = FALSE;
	    }
	    break;
	case DB_INT_TYPE:
	    if (bind->rqb_r_length == 2 && bind->rqb_length == 4)
	    {
		/*
		** i2 to i4 conversion 
		*/
		*((i4 *)bind->rqb_addr) = (i4)(*((i2 *)bind->rqb_addr));
	    }
	    else if (bind->rqb_r_dt_id == -DB_INT_TYPE)
	    {
		/*
		** this case is converted already 
		*/
		;
	    }
	    else
	    {
		convert_ok = FALSE;
	    }
	    break;
	default:
	    convert_ok = FALSE;
	    break;
    }
    if (convert_ok == TRUE)
    {
	return(E_RQ0000_OK);
    }
    else
    {
        RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:rqu_convert_data. DATA CONVERSION ERROR!\n"));
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "\texpected length %d type %d\n"),
		    bind->rqb_length, bind->rqb_dt_id);
        RQTRACE(rqs,1)(RQFMT(rqs,
		    "\treturned length %d type %d\n"),
		    bind->rqb_r_length, bind->rqb_r_dt_id);
	return(E_RQ0016_CONVERSION_ERROR);
    }
}

/*{
** Name: rqu_interrupt	- send interrupt to an association
**
** Description:
**      interrupt is send to the designated database server.
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
**      11-Jan-1989 (alexh)
**          created
**	25-jul-1989 (georgeg)
**	    Changed to call rqu_txinterrupt, and rqu_txiack for GCA_ATTENTION and GCA_IACK. 
*/
DB_ERRTYPE
rqu_interrupt(
	RQL_ASSOC   *assoc,
	i4	    interrupt_type)
{
    GCA_AT_DATA	    *attention;
    i4		    i=0;
    GCA_SD_PARMS    *send = &assoc->rql_sd;
    GCA_RV_PARMS    *receive = &assoc->rql_rv;
    STATUS	    rx_status, status;
    RQS_CB	    *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;


    if (assoc == NULL)
    {
	return(E_RQ0000_OK);	/* considered done */
    }

    assoc->rql_interrupted = FALSE;

    RQTRACE(rqs,3)(RQFMT(rqs,
		"RQF:Assn %d. Sending GCA_ATTENTION. Bytes left=%d. %s\n"),
                assoc->rql_assoc_id, assoc->rql_b_remained,
		(assoc->rql_rqr_cb->rqr_end_of_data == 1) ? "EOD" : "NOT EOD");

    if (assoc->rql_last_request == GCA_RECEIVE 
		&&
	     assoc->rql_turned_around == TRUE 
		&&
	     interrupt_type == GCA_ENDRET )
    {
        RQTRACE(rqs,3)(RQFMT(rqs,
		    "RQF:Assn %d. Interrupt optimized - not sent!.\n"),
		    assoc->rql_assoc_id);
	return(E_RQ0000_OK);
    }

    attention = (GCA_AT_DATA *)assoc->rql_sd.gca_buffer;

    attention->gca_itype = interrupt_type;
    send->gca_message_type = GCA_ATTENTION;
    send->gca_msg_length = (i4)sizeof(GCA_AT_DATA);
    send->gca_end_of_data = TRUE;

    rqu_gcmsg_type(GCA_SEND, send->gca_message_type, assoc);

    if ( rqu_txinterrupt(GCA_SEND, (PTR)send, &send->gca_status, assoc) )
    {
	return( E_RQ0012_INVALID_WRITE );
    }

    assoc->rql_turned_around = FALSE;
    receive->gca_flow_type_indicator = GCA_NORMAL;
    do 
    {
	rx_status = rqu_rxiack(GCA_RECEIVE,  (PTR)receive, &receive->gca_status, assoc);
	if ( rx_status != E_DB_OK )
	{

	    if ( assoc->rql_comm.rqc_status != RQC_OK )
	    {
		assoc->rql_status = RQL_TERMINATED;
	    }
	    RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. rxiack returned rqc_status %d(0x%x).\n"),
			assoc->rql_assoc_id,
			assoc->rql_comm.rqc_status,
			assoc->rql_comm.rqc_status);
	    break;
	}
        /* log the GCA receipt */
        rqu_gcmsg_type(GCA_RECEIVE, receive->gca_message_type, assoc);

    } while (
		receive->gca_message_type != GCA_IACK 
		    && 
		++i < 50 
		    && 
		( 
		    (assoc->rql_interrupted)
			||
		    (assoc->rql_comm.rqc_status == RQL_OK)
		)
	     );/* endwhile */

    if ( (receive->gca_message_type == GCA_IACK) && (assoc->rql_comm.rqc_status == RQC_OK) )
    {
	STATUS  rv;

	assoc->rql_status = RQL_OK;
        assoc->rql_turned_around = TRUE;
	if ( assoc->rql_interrupted )
	{
	    RQTRACE(rqs,2)(RQFMT(rqs,
			"RQF:Assn %d. Recieved GCA_IACK and INTERRUPTED.\n"),
			assoc->rql_assoc_id);
	    return(E_RQ0039_INTERRUPTED);
	}
	else
	{
	    RQTRACE(rqs,2)(RQFMT(rqs,
			"RQF:Assn %d. Received GCA_IACK.\n"),
			assoc->rql_assoc_id);
	}
	return(E_RQ0000_OK);
    }
    else if (assoc->rql_interrupted)
    {
	/*
	** if the message is not GCA_IACK we could not recover from the interrupt 
	** while doing an GCA_ENDRET, we mark the association dead.
	*/
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:Assn %d. INTERRUPTED with GCA_IACK missing.\n"),
		    assoc->rql_assoc_id);
	assoc->rql_status = RQL_TERMINATED;
	return(E_RQ0017_NO_ACK);
    }	
    else if ( (receive->gca_message_type != GCA_IACK) && (assoc->rql_comm.rqc_status == RQC_OK) )
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:Assn %d. GCA_IACK missing after (%d) messages.\n"),
		    assoc->rql_assoc_id, i);
	assoc->rql_status = RQL_TERMINATED;
	return(E_RQ0017_NO_ACK);
    }
    else
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		"RQF:Assn %d. GCA_IACK missing..terminating association.\n"),
		    assoc->rql_assoc_id);
	assoc->rql_status = RQL_TERMINATED;
	assoc->rql_comm.rqc_status = RQC_ASSOC_TERMINATED;
	return(E_RQ0017_NO_ACK);
    }
}

/*{
** Name: rqu_set_option	- set an SQL/QUEL language option
**
** Description:
**      Set an SQL/QUEL option on the specified association, among
**	the option are set RESULT_STRUCTURE etc.
**
** Inputs:
**      assoc	    the association
**	msg_len	    the length od the set string
**	msg_ptr	    a pointer to the message itself
**	lang	    the language, SQL/QUEL.
**
** Outputs:
**	    rqr_error		error reported by RQF
**        
**	Returns:
**	    E_RQ0000_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-feb-1989 (georgeg)
**          created
*/
DB_ERRTYPE
rqu_set_option(
	RQL_ASSOC   *assoc,
	i4	    msg_len,
	char	    *msg_ptr,
	DB_LANG	    lang)
{

    i4	i4_buff;
    DD_PACKET	ddpkt;
    DB_ERRTYPE  status = E_RQ0000_OK;

    /*
    ** maybe this function should not check for language
    ** and the caller should, we will probally change it latter.
    */
    if ( !((lang == DB_SQL) || ( lang == DB_QUEL)) )
    {
	return(E_DB_ERROR);
    }

    ddpkt.dd_p1_len    = msg_len;
    ddpkt.dd_p2_pkt_p  = msg_ptr;
    ddpkt.dd_p3_nxt_p  = (DD_PACKET *)NULL;
    ddpkt.dd_p4_slot   = DD_NIL_SLOT;

    do 
    {
        /* send the set option to the  LDB. */
        status = rqu_put_qtxt(assoc, GCA_QUERY, lang, 0, &ddpkt, NULL);
	if (status != E_RQ0000_OK)
	    break;

        /* flush it */
        status = rqu_putflush(assoc, TRUE);
	if (status != E_RQ0000_OK)
	    break;

	/* we succeded flushing, get the reply */
	if (rqu_get_reply(assoc, -1) != GCA_RESPONSE)
	    status = E_RQ0014_QUERY_ERROR;
    } while (0);

    if (status != E_RQ0000_OK)
	return (E_RQ0014_QUERY_ERROR);
    else
        return( E_RQ0000_OK );
}

/*{
** Name: rqu_gccall - GCA asynch call to perform GCA_RECEIVE and GCA_SEND
**
** Description:
**      This routine handles asynchronous GCA calls.
**	For the first STAR release, RQF suspends after GCA asynch
**	call until the request completes.
**	This routine will not deal with any kind of errors. It depends 
**	on a high level routine that will do the clean-up of the
**	association. This routine will be called from RQF_CALL.
**
**      When STAR goes for parallel subqueries or pipelined, RQF should
**	be carefully redesigned in area of asynch handling.
**
**	The local variable 'test' is used to force RQF to use synch
**	calls. This is useful for positively verifying a asynch problem.
**
**
** Inputs:
**      req		GCA request type
**	msg		GCA request parameters
**	poll_addr	asynch status polling address
**	assoc		the associaon on witch to do the IO
** Outputs:
**      STATUS		GCA status
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      10-jul-1989 (georgeg)
**          recreated from ALEXH original 
**      06-feb-1990 (georgeg)
**          Turned the GCA_REQUEST to a synchronous call, work around for a GCA bug. 
**      22-apr-91 (szeng)
**          Changed #ifdef VAX to #ifdef VMS to solve the confliction
**          between ulx_vax and real VMS VAX.
**	15-oct-1992 (fpang)
**	    1. Support cs_client_type  to accompany scc_gcomplete() change.
**	    2. Support asynch GCA_REQUEST and GCA_DISASSOC wrt. GCA_RESUME
**	       protocol.
**	    3. Check error condition when request is synchronous.
**	08-dec-1992 (fpang)
**	    1. Made trace messages more uniform.
**	    2. Fixed lost association case.
**	11-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	16-dec-1993 (fpang)
**	    Resuspend if we are resumed with gca_status E_GCFFFF_IN_PROCESS.
**	    Fixes B57764, Help Table in Direct Connect causes spurious Star 
**	    server SEGV's  
**	01-Fed-1994 (fpang)
**	    Fixed compile error.
**	22-Nov-2002 (hanal04) Bug 107159 INGSRV 1696
**	    Remove the CS_INTERRUPT_MASK from the call to CSsuspend().
**          This prevents the Star server session from being resumed
**          by front end interrupts whilst it is in the middle of 
**          GCA communications with the DBMS server. IIGCa_call()
**          must be allowed to complete with either a success or
**          failure.
*/
STATUS
rqu_gccall(
	i4	req,
	PTR	msg,
	STATUS	*poll_addr,
	RQL_ASSOC *assoc)
{
    STATUS	rv, status;
    i4		test = 1;
    RQC_GCA	*gca_state = (RQC_GCA *)&assoc->rql_comm;
    RQS_CB	*rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;
    i4		gca_indicator = 0;

    i4	save_scb_type;    /* For stashing cs_client_type */

    /*
    ** remember the last request type 
    */
    assoc->rql_last_request = req;

    if ( 
	 (gca_state->rqc_rxtx != RQC_IDDLE) 
	    || 
	 (gca_state->rqc_status != RQC_OK)
       )
    {

        RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:Assn %d. COMM error, Request %d(0x%x) "),
		    assoc->rql_assoc_id, req, req );
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "rqc_rxtx=%x, rqc_status=%x, rql_status=%x.\n"),
		    gca_state->rqc_rxtx, 
		    gca_state->rqc_status, 
		    assoc->rql_status);

	return(E_DB_ERROR);
    }


    if (test != 1)
    {
	/* 
	** synchrounous call 
	*/
	(VOID)IIGCa_call(req,
		(GCA_PARMLIST *)msg, (i4)GCA_SYNC_FLAG, 
		(PTR)0, (i4)-1, &status);
	return(status);
    }
    else
    {
	/*
	** asynch call 
	*/

	/*
	** remember state are we sending or receiving?  it is needed
	** when we go to service an interrupt.
	*/
	if ( req == GCA_RECEIVE )
	{
	    gca_state->rqc_rxtx = RQC_RECV;
	}
	else if (req == GCA_SEND )
	{
	    gca_state->rqc_rxtx = RQC_SEND;
	}
	else
	{
	    gca_state->rqc_rxtx = RQC_IDDLE;
	}	

	/* Stash and reset cs_client_type so completion routine knows who
	** made the call, and resumes the correct facility.
	*/
	save_scb_type = assoc->rql_scb->cs_client_type;
	assoc->rql_scb->cs_client_type = RQF_SCB_TYPE;
    	gca_indicator |= GCA_ASYNC_FLAG;
	*poll_addr = gca_state->rqc_gca_status = E_DB_OK;
	/*
	** Resuspend if :
	**     req == GCA_REQUEST , GCA_DISASSOC and 
	**     resumed with E_GCFFFE_INCOMPLETE 
	** or  req == GCA_SEND, GCA_RECEIVE and 
	**     resumed with E_GCFFFF_IN_PROCESS
	*/
	do
	{
	    if (*poll_addr == E_GCFFFE_INCOMPLETE) 
		gca_indicator |= GCA_RESUME; /* retry */
	    if (*poll_addr != E_GCFFFF_IN_PROCESS)
		(VOID)IIGCa_call(req,
				(GCA_PARMLIST *)msg, (i4)gca_indicator, 
				(PTR)assoc->rql_scb, (i4)-1, &status);
	    rv= CSsuspend(CS_BIO_MASK | CS_TIMEOUT_MASK,
			  assoc->rql_timeout, 0);
	}
	while ((rv == E_DB_OK) &&
	       ( ( (( req == GCA_REQUEST ) || ( req == GCA_DISASSOC )) &&
		   (*poll_addr == E_GCFFFE_INCOMPLETE) )
	    ||   ( ((  req == GCA_SEND ) || ( req == GCA_RECEIVE )) &&
	           (*poll_addr == E_GCFFFF_IN_PROCESS) ) ) );

	/* Restore cs_client_type */
	assoc->rql_scb->cs_client_type = save_scb_type;

	gca_state->rqc_gca_status = *poll_addr;
	if ( (rv == E_DB_OK) && (gca_state->rqc_gca_status == E_GC0000_OK) )
	{
	    /*
	    ** make the caller happy, everything seems to be ok.
	    */ 	    
	    gca_state->rqc_status = RQC_OK;
	    gca_state->rqc_rxtx = RQC_IDDLE;
	    return(E_DB_OK);  
	}
	else if ( rv == E_CS0009_TIMEOUT )
	{
	    /* Timed out. Must still cancel or drive completion routine.*/
	    RQTRACE(rqs,2)(RQFMT(rqs,
			   "RQF:Assn %d. TIMEDOUT (timeout = %d),"),
			   assoc->rql_assoc_id, assoc->rql_timeout);
	    gca_state->rqc_status = RQC_TIMEDOUT;
	}    
	else if ( rv == E_CS0008_INTERRUPTED )
	{
	    /* Interrupt. Must still cancel or drive completion routine. */
	    RQTRACE(rqs,2)(RQFMT(rqs,
		"RQF:Assn %d. INTERRUPTED, "),
		assoc->rql_assoc_id);
	    gca_state->rqc_status = RQC_INTERRUPTED;
	}
	else if ((rv == E_DB_OK) && 
		 (gca_state->rqc_gca_status == E_GC0001_ASSOC_FAIL))
	{
	    /* Connection to partner has been lost or partner is gone. */
	    RQTRACE(rqs,2)(RQFMT(rqs,
			    "RQF:Assn %d. ASSOCIATION LOST,"),
			    assoc->rql_assoc_id);
	    gca_state->rqc_status = RQC_ASSOC_TERMINATED;
	}
	else if ((rv == E_DB_OK) && (gca_state->rqc_gca_status != E_GC0000_OK))
	{
	    /* Completed w/Error. */
	    RQTRACE(rqs,2)(RQFMT(rqs,
		"RQF:Assn %d. RESUMED W/ERROR,"),
		assoc->rql_assoc_id);
	    gca_state->rqc_status = RQC_GCA_ERROR;
	    switch( req )
	    {
		case GCA_REQUEST:
		{
		    STRUCT_ASSIGN_MACRO( ((GCA_RQ_PARMS *)msg)->gca_os_status, 
					    gca_state->rqc_os_gca_status );
		    gca_state->rqc_gca_status = 
			((GCA_RQ_PARMS *)msg)->gca_status;
		    break;
		}
		case GCA_SEND:
		{
		    STRUCT_ASSIGN_MACRO( ((GCA_SD_PARMS *)msg)->gca_os_status, 
						gca_state->rqc_os_gca_status );
		    gca_state->rqc_gca_status = 
			((GCA_SD_PARMS *)msg)->gca_status;
		    break;
		}
		case GCA_RECEIVE:
		{
		    STRUCT_ASSIGN_MACRO( ((GCA_RV_PARMS *)msg)->gca_os_status, 
						gca_state->rqc_os_gca_status );
		    gca_state->rqc_gca_status = 
			((GCA_RV_PARMS *)msg)->gca_status;
		    break;
		}
		case GCA_DISASSOC:
		{
		    STRUCT_ASSIGN_MACRO( ((GCA_DA_PARMS *)msg)->gca_os_status, 
						gca_state->rqc_os_gca_status );
		    gca_state->rqc_gca_status = 
			((GCA_DA_PARMS *)msg)->gca_status;
		    break;
		}
		default:
		    break;	
	    }
	}
	RQTRACE(rqs,2)(RQFMT(rqs,
			"Req = %d, CSsuspend = %x, IIGCb_cb_call = %p,"),
		        req, rv, *poll_addr );
	RQTRACE(rqs,2)(RQFMT(rqs,
			" gca_status = %x, rqc_gca_status = %x\n"),
			gca_state->rqc_status, gca_state->rqc_gca_status);
    }
    return(E_DB_ERROR);
}

/*{
** Name: rqu_txinterrupt - send an GCA interrupt message.
**
** Description:
**      This routine will write an attention message to the ASSOCIATION.
**
**
** Inputs:
**	req		GCA_SEND
**	msg		GCA request parameters
**	poll_addr	asynch status polling address
**	assoc		the association to send it.
**
** Outputs:
**      STATUS		E_DB_OK
**
**	Exceptions:
**	    none 
**
** Side Effects:
**	    none
**
** History:
**      10-jul-1989 (georgeg)
**          created
*/
STATUS
rqu_txinterrupt(
	i4	req,
	PTR	msg,
	STATUS	*poll_addr,
	RQL_ASSOC *assoc)
{
    STATUS	status, rv;
    i4		i = 0;
    RQC_GCA	*gca_state = (RQC_GCA *)&assoc->rql_comm;

    assoc->rql_last_request = req;

    /* 
    ** We really do not know what the state of the association and CS is 
    ** we will hope for the best.
    */
    (VOID)IIGCa_call(req,
		(GCA_PARMLIST *)msg, GCA_NO_ASYNC_EXIT, 
		(PTR)assoc->rql_scb, (i4)-1, &status);
    return (E_DB_OK);
        
}

/*{
** Name: rqu_rxiack - recieve a iack from the ldb
**
** Description:
**      This routine reads from the ldb until and iack is received
**
**
** Inputs:
**	req		Request that was interrupted
**	msg		Message that was interrupted 
**	poll_addr	asynch status polling address
**	assoc		the association that was interrupted
**
** Outputs:
**      STATUS		E_DB_OK
**
**	Exceptions:
**	    none 
**
** Side Effects:
**	    none
**
** History:
**      10-jul-1989 (georgeg)
**          created
**	15-oct-1992 (fpang)
**	    Changes support cs_client_type field.
**	16-dec-1993 (fpang)
**	    Resuspend if we are resumed with gca_status E_GCFFFF_IN_PROCESS.
**	    Fixes B57764, Help Table in Direct Connect causes spurious Star 
**	    server SEGV's  
*/
STATUS
rqu_rxiack(
	i4	req,
	PTR	msg,
	STATUS	*poll_addr,
	RQL_ASSOC *assoc)
{
    STATUS	status = E_DB_ERROR,
		rv;
    i4		i = 0;
    RQC_GCA	*gca_state = (RQC_GCA *)&assoc->rql_comm;
    bool	interrupted = FALSE;
    RQS_CB	*rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;

#define		RQF_MAX_RETRY	5

    i4 save_scb_type;	/* To stash cs_client_type */

    if (
	(assoc->rql_status == RQL_NOTESTABLISHED) 
	    || 
	(assoc->rql_status == RQL_TERMINATED)
	    || 
	(assoc->rql_comm.rqc_status == RQC_ASSOC_TERMINATED)
	    || 
	(assoc->rql_comm.rqc_status == RQC_DISASSOCIATED)
       )
    {
	RQTRACE(rqs,1)(RQFMT(rqs,
		    "RQF:Assn %d. COMM error rqu_rxiack at wrong time.\n"),
		    assoc->rql_assoc_id);
	RQTRACE(rqs,1)(RQFMT(rqs,
		   "\trequest=%d(%x), rql_status=%d(%x), rqc_status=%d(%x).\n"),
		    req, req,
		    assoc->rql_status, assoc->rql_status,
		    assoc->rql_comm.rqc_status,
		    assoc->rql_comm.rqc_status);

	return(E_DB_ERROR);
    }

    assoc->rql_last_request = req;

    /* 
    ** If the previous request that we were interrupted has an event pending deal
    ** with it here. If it was a write we hope that the GCA completion routine has
    ** been driven by now, we have already send the GCA_ATT_MESSAGE. If it there was
    ** a read outstanding deal with it in a different manner.
    */

    (VOID)CScancelled((PTR)0);

    /* Stash and reset cs_client_type so completion routine knows who
    ** made the call, and resumes the correct facility.
    */
    save_scb_type = assoc->rql_scb->cs_client_type;
    assoc->rql_scb->cs_client_type = RQF_SCB_TYPE;

    if ( *poll_addr == E_GCFFFF_IN_PROCESS )
    {
        RQTRACE(rqs,2)(RQFMT(rqs,
		   "RQF:Assn %d. IIGCb_cb_call already exists, req=%d(%x).\n"),
		    assoc->rql_assoc_id, req, req);

    }
    else
    {
	(VOID)IIGCa_call(req,
		(GCA_PARMLIST *)msg, (i4)0, 
		(PTR)assoc->rql_scb, (i4)-1, &status);
    }

    rv = E_DB_OK;
    while ((++i <= RQF_MAX_RETRY ) || (*poll_addr == E_GCFFFF_IN_PROCESS))
    {

	rv= CSsuspend(CS_INTERRUPT_MASK | CS_BIO_MASK, (i4)0, 0);
	gca_state->rqc_gca_status = *poll_addr;

	if ( (rv == E_DB_OK) && (gca_state->rqc_gca_status == E_GC0000_OK) )
	{
	    /*
	    ** the caller can be happy, everything seems to be ok.
	    */
	    if (interrupted)
	    {
		/*
		** we got interrupted while sending an end-retreive.
		*/
		assoc->rql_interrupted = TRUE;
	    }
	    gca_state->rqc_rxtx = RQC_IDDLE;
	    gca_state->rqc_status = RQC_OK;
	    status = E_DB_OK;  
	    break;
	}
	else if ( (rv == E_DB_OK) && (gca_state->rqc_gca_status == E_GCFFFF_IN_PROCESS) )
	{
	    /*
	    ** it looks like we are an event out of phase with CS just do a CSsuspend.
	    */
	    RQTRACE(rqs,2)(RQFMT(rqs,
		        "RQF:Assn %d. RQU_RXIACK CSsuspend = %d, "),
		        assoc->rql_assoc_id, rv);
	    RQTRACE(rqs,2)(RQFMT(rqs,
			"gca_status %d(%x) \n"),
		        gca_state->rqc_gca_status, gca_state->rqc_gca_status);
	    continue;
	}
	else if ( rv == E_CS0008_INTERRUPTED )
	{
	    /*
	    ** We cannot realy look at the gca_status yet, GCA has not driven the completion
	    ** routine, the FE has send an interrupt, the server will not allow any more interrupts
	    ** let's wait for a GCA to drive the completion routine.
	    */
	    RQTRACE(rqs,2)(RQFMT(rqs,
		"RQF:Assn %d. RQU_RXIACK INTERRUPTED, CSsuspend = %d.\n"),
			assoc->rql_assoc_id, rv);
	    interrupted = TRUE;
	}

	if ((rv == E_DB_OK) && (gca_state->rqc_gca_status != E_GC0000_OK))
	{
	    /*
	    ** Here we assume that CS was resummed by GCA with a Buffered I/O request completed,
	    ** and there is a GCA error explaining the trouble, we hope.
	    */
	    RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. RQU_RXIACK RESUMED with GCA_ERROR. "),
			assoc->rql_assoc_id);
	    RQTRACE(rqs,1)(RQFMT(rqs,
			"CSsuspend = %x, gca_status %d(%x).\n"),
			rv,
			gca_state->rqc_gca_status,
			gca_state->rqc_gca_status);
#ifdef VMS
	    RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. RQU_RXIACK RESUMED with GCA_ERROR. "),
			assoc->rql_assoc_id);
	    RQTRACE(rqs,1)(RQFMT(rqs,
			"gca_status=%d(%x), os_status=%d(%x).\n"),
			gca_state->rqc_gca_status, 
			gca_state->rqc_gca_status, 
			gca_state->rqc_os_gca_status.error,
			gca_state->rqc_os_gca_status.error);
#endif
	
	    if (gca_state->rqc_gca_status == E_GC0024_DUP_REQUEST) 
	    { 
		/*
		** we must wait for the the previous message to get purged before 
		** it becames usable again.
		*/
	    }
	    else if ( gca_state->rqc_gca_status == E_GC0027_RQST_PURGED) 
	    { 
		/*
		** the message has been purged and we can continue happily waiting for GCA_IACK.
		*/
		(VOID)IIGCa_call(req,
				(GCA_PARMLIST *)msg, (i4)0, 
				(PTR)assoc->rql_scb, (i4)-1, &status);
	    }
	    else
	    {
		/*
		** every other case is an error case and we treat as such we concider the association dead,
		** the caller needs to deal with it.
		*/
		gca_state->rqc_rxtx = RQC_RECV;
		gca_state->rqc_gca_status = ((GCA_RV_PARMS *)msg)->gca_status;
		STRUCT_ASSIGN_MACRO( ((GCA_RV_PARMS *)msg)->gca_os_status, 
				     gca_state->rqc_os_gca_status );
		gca_state->rqc_status = RQC_GCA_ERROR;
#ifdef VMS
	        RQTRACE(rqs,1)(RQFMT(rqs,
			    "RQF:Assn %d. RQU_RXIACK RESUMED with GCA_ERROR "),
			    assoc->rql_assoc_id);
		RQTRACE(rqs,1)(RQFMT(rqs,
			    "gca_status %d(%x). os_status %d(%x)\n"),
			    gca_state->rqc_gca_status, 
			    gca_state->rqc_gca_status, 
			    gca_state->rqc_os_gca_status.error,
			    gca_state->rqc_os_gca_status.error);
#endif
		status = E_DB_ERROR;
		break;
	    }
	} /* endif rv==E_DB_OK && ... */
    }	/* end while */
	
    /* Restore cs_client_type */
    assoc->rql_scb->cs_client_type = save_scb_type;

    if (( i >= RQF_MAX_RETRY ) && (*poll_addr != E_GCFFFF_IN_PROCESS))
    {
	gca_state->rqc_status = RQC_TIMEDOUT;
	status = E_DB_ERROR;
    }	
    return(status);    
}


/*{
** Name: rqu_dmp_gcaqry - Display/trace a GCA_QUERY from FE.
**
** Description:
**	This function will display/trace a GCA_QUERY received from the FE.
**	The gca_buffer is assumed to already be interpreted. This will
**	be used to display/trace GCA_QUERY's from the FE in direct connect.
**
** Inputs:
**	sd		    - The send gca buffer.
**	assoc		    - The current ldb association.
**	    
** Outputs:
** 	None
**        
** Returns:
**      VOID
**
** Side Effects:
**	    none
**
** History:
**      13-aug-1991 (fpang)
**          Created
*/  
VOID
rqu_dmp_gcaqry(
	GCA_SD_PARMS *sd,
	RQL_ASSOC    *assoc)
{
    RQS_CB	*rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;
    PTR		p;
    i4		l,skip;
    GCA_Q_DATA  q;
    RQR_CB	*rqr;
    SCC_SDC_CB  *dcp;
    GCA_RV_PARMS *itp;

    if ((sd == (GCA_SD_PARMS *)NULL) || (assoc == (RQL_ASSOC *)NULL))
	return;

    if (sd->gca_message_type != GCA_QUERY)
	return;

    rqr = assoc->rql_rqr_cb;
    rqs = (rqr != (RQR_CB *)NULL) ? (RQS_CB *)rqr->rqr_session : (RQS_CB *)NULL;
    dcp = (rqr != (RQR_CB *)NULL) ? 
	  (SCC_SDC_CB *)rqr->rqr_dc_blkp : (SCC_SDC_CB *)NULL;
    itp = (dcp != (SCC_SDC_CB *)NULL) ? 
	  (GCA_RV_PARMS *)dcp->sdc_client_msg : (GCA_RV_PARMS *)NULL;

    if ((rqr == (RQR_CB *)NULL) || (rqs == (RQS_CB *)NULL) ||
	(dcp == (SCC_SDC_CB *)NULL) || (itp == (GCA_RV_PARMS *)NULL))
	return;

    if ((! RQF_DBG_LVL(rqs,1)) && (! RQF_DBG_LVL(rqs, 3)))
        return;

    p = itp->gca_data_area;

    if (p == NULL)
	return;

    l = sd->gca_msg_length;

    /* skip over gca_language_id, and gca_query_modifier */

    skip = sizeof(q.gca_language_id) + sizeof(q.gca_query_modifier);

    p += skip; l -= skip;

    for (q.gca_qdata[0].gca_type = 0,
	    q.gca_qdata[0].gca_precision = 0,
	    q.gca_qdata[0].gca_l_value = 0; 
	 l != 0; 
         q.gca_qdata[0].gca_type = 0,
	     q.gca_qdata[0].gca_precision = 0,
	     q.gca_qdata[0].gca_l_value = 0 )

    {
	skip = sizeof (q.gca_qdata[0].gca_type);
	MEcopy(p, skip, (PTR)&q.gca_qdata[0].gca_type); p += skip; l -= skip;

	skip = sizeof (q.gca_qdata[0].gca_precision);
	MEcopy(p, skip, (PTR)&q.gca_qdata[0].gca_precision); 
	p += skip; l -= skip;

	skip = sizeof (q.gca_qdata[0].gca_l_value);
	MEcopy(p, skip, (PTR)&q.gca_qdata[0].gca_l_value); p += skip; l -= skip;

        rqu_dmp_dbdv(rqs, assoc, 
		     q.gca_qdata[0].gca_type, 
		     q.gca_qdata[0].gca_precision, 
		     q.gca_qdata[0].gca_l_value, 
		     p );

	skip = q.gca_qdata[0].gca_l_value; p += skip; l -= skip;
    }
}

/*{
** Name: rqu_sdc_chkintr - Check for async interrupt from ldb.
**
** Description:
**	This function will perform an timed read on the LDB to check
**	for async interrupts during copy, which indicates that the LDB
**	is signaling an error.
**
** Inputs:
**	rcb		    - Request control block
**	   ->rqr_session	- Session control block
**		.rql_curent_site    - Ldb to check.
**	    
** Outputs:
**	rcb		    - Request control block
**	    ->rqr_dc_blkp	- Direct connect contol block
**		.sdc_ldb_status	    - SDC_INTERRUPTED if interrupted
**				    - 0 otherwise
**		.sdc_msg_type	    - GCA_MESSAGE type if interrupted
**		.sdc_ldb_msg	    - Interpreted LDB message if interrupted
**		.sdc_rv		    - GCA_RV_PARMS if interrupted
**        
** Returns:
**      DB_ERRTYPE
**	    E_DB_OK    - if successful
**	    E_DB_ERROR - otherwise
**
** Side Effects:
**	    none
**
** History:
**      15-oct-1992 (fpang)
**          Created
*/  
DB_ERRTYPE
rqu_sdc_chkintr(
	RQR_CB  *rcb)
{
    RQL_ASSOC   *assoc = ((RQS_CB *)rcb->rqr_session)->rqs_current_site;
    SCC_SDC_CB  *dc = (SCC_SDC_CB *)rcb->rqr_dc_blkp;
    GCA_RV_PARMS        *receive;
    STATUS      status;
    DB_STATUS   error;
    RQS_CB      *rqs = (RQS_CB *)rcb->rqr_session;

    assoc->rql_rqr_cb = rcb;

    receive = &assoc->rql_rv;
 
    receive->gca_association_id = assoc->rql_assoc_id;
    receive->gca_flow_type_indicator = GCA_EXPEDITED;
    receive->gca_buffer = assoc->rql_read_buff;
    receive->gca_b_length = sizeof(assoc->rql_read_buff);
    receive->gca_descriptor = NULL;

    status = IIGCa_call(GCA_RECEIVE,
		        (GCA_PARMLIST *)receive,
			GCA_SYNC_FLAG,
			(PTR) 0,
		        (i4) 0,
		        &error);
    

    receive->gca_flow_type_indicator = GCA_NORMAL;

    if (status == E_DB_ERROR) 
        return (error);
   
    if ( receive->gca_status == E_GC0020_TIME_OUT )
    {
        dc->sdc_ldb_status = 0;
        return (E_DB_OK);
    }
    else if ( receive->gca_status == E_GC0000_OK )
    {
       dc->sdc_msg_type = receive->gca_message_type;
       dc->sdc_ldb_msg = (PTR)receive->gca_buffer;
       dc->sdc_ldb_status = SDC_INTERRUPTED;
       dc->sdc_rv = (PTR)receive;
       return (E_DB_OK);
    }
    else
    {
        dc->sdc_ldb_status = 0;
        return (receive->gca_status);
    }
}

/*{
** Name: rqu_dmp_dbdv - Display/trace a DB_DATATYPE.
**
** Description:
**	This function will display/trace a DB_DATATYPE that was passed from the
**	FE as a parameter, or received from the LDB as a column.
**	If the datatype is DB_QTXT_TYPE it will be displayed if trace level is 
**	1 or greater, all other types will be displayed if trace level is 3
**	or greater.
**
** Inputs:
**	rqs		    - Session control block.
**	assoc		    - The current ldb association.
**	datatype	    - The datatype of parameter/column.
**	precision	    - The precision of parameter/column.
**	data_len	    - The length of the parameter/column.
**	data_ptr	    - Address of parameter/column value.
**	    
** Outputs:
** 	None
**        
** Returns:
**      VOID
**
** Side Effects:
**	    none
**
** History:
**      15-oct-1992 (fpang)
**          Created
**	18-jan-1993 (fpang)
**	    Display DB_DEC_TYPE by converting to float first.
**	04-nov-1993 (fpang)
**	    Test for NULL value was testing bit 'len' instead of bit 'len-1'.
**	    Fixes B56429, select from aggregate when value is NULL may result
**	    in inconsistent answers.
*/  
VOID
rqu_dmp_dbdv(
	RQS_CB    *rqs,
	RQL_ASSOC *assoc,
	DB_DT_ID  data_type,
	i2 	  precision,
	i4     	  data_len,
	PTR	  data_ptr)
{
    DB_DT_ID	type;
    u_i2	len = 0;
    u_i2	slen = 0;
    f8		numbuf;
    i4		skip = 0;
    char	*dst;
    char	*src;
    char	fmt[80];
    char	buf[80];
    char	typ[20];
    bool	isnull = FALSE;

    if (data_type == DB_QTXT_TYPE)
    {
	if (! RQF_DBG_LVL(rqs, 1))
	    return;

	RQTRACE(rqs,1)(RQFMT(rqs,
			"RQF:Assn %d. Query fragment: %t\n"),
			assoc->rql_assoc_id,
			data_len, data_ptr);
	return;
    }

    if (! RQF_DBG_LVL(rqs, 3))
	return;

    STprintf(fmt,
      "RQF:Assn %d. DB_DATA_VALUE: type=%%s, prec=%d, length=%d, value=%%s\n",
	     assoc->rql_assoc_id, precision, data_len);

    if(( data_type < 0 ) &&
       (((u_char *)(data_ptr))[data_len-1] & 0x01 ))
    {
	isnull = TRUE;
	STprintf(buf,"IS NULL");
    }

    type = abs(data_type);
    if (data_type > 0)
	len = data_len;
    else
	len = data_len -1;

    switch (type)
    {
	case DB_INT_TYPE:
	    if (isnull == FALSE)
	    {
		MEcopy(data_ptr, len, (PTR)&numbuf);
		switch (len)
		{
		    case 1: STprintf(buf,"%d", *(i1 *)&numbuf);
			    break;
		    case 2: STprintf(buf,"%d", *(i2 *)&numbuf);
			    break;
		    case 4: STprintf(buf,"%d", *(i4 *)&numbuf);
			    break;
		}
	    }
	    RQTRACE(rqs,3)(RQFMT(rqs,
		fmt), "DB_INT_TYPE", buf);
	    break;
	case DB_FLT_TYPE:
	    STprintf(typ, "DB_FLT_TYPE");
	case DB_MNY_TYPE:
	    if (type == DB_MNY_TYPE)
		STprintf(typ, "DB_MNY_TYPE");
	    if (isnull ==  FALSE)
	    {
		MEcopy(data_ptr, len, (PTR)&numbuf);
		switch (len)
		{
		    case 4: STprintf(buf,"%g", *(f4 *)&numbuf, '.');
			    break;
		    case 8: STprintf(buf,"%g", *(f8 *)&numbuf, '.');
			    break;
		}
	    }
	    RQTRACE(rqs,3)(RQFMT(rqs,
		fmt), typ, buf);
	    break;
	case DB_DEC_TYPE:
	    if (isnull == FALSE)
	    {
	        CVpkf ( data_ptr,
		        (i4)DB_P_DECODE_MACRO(precision),
		        (i4)DB_S_DECODE_MACRO(precision),
		        &numbuf );
		STprintf(buf, "%g", *(f8 *)&numbuf, '.');
	    }
	    RQTRACE(rqs,3)(RQFMT(rqs,
		fmt), "DB_DEC_TYPE", buf);
	    break;
	case DB_VCH_TYPE:
	    if (type == DB_VCH_TYPE)
		STprintf(typ, "DB_VCH_TYPE");
	case DB_TXT_TYPE:
	    if (type == DB_TXT_TYPE)
		STprintf(typ, "DB_TXT_TYPE");
	case DB_LTXT_TYPE:
	    if (type == DB_LTXT_TYPE)
		STprintf(typ, "DB_LTXT_TYPE");
	    len = 0;
	    skip = sizeof(len);
	    MEcopy(data_ptr, skip, (PTR)&len);
	case DB_CHR_TYPE:
	    if (type == DB_CHR_TYPE)
		STprintf(typ, "DB_CHR_TYPE");
	case DB_CHA_TYPE:
	    if (type == DB_CHA_TYPE)
		STprintf(typ, "DB_CHA_TYPE");
	    src = (PTR)(data_ptr + skip);
	    while( len != 0 )
	    {
	        slen = sizeof(fmt) - STlength(fmt) - 2;
	        dst = buf;
		while (slen > 0 && len > 0)
		{
		    CMbytedec(len, src);
		    if (CMprint(src))
		    {
			CMbytedec(slen, src);
			CMcpyinc(src,dst);
		    }
		    else
		    {
			CMnext(src);
			*dst++ = '.';
			slen--;
		    }
		}
		*dst = EOS;
		RQTRACE(rqs,3)(RQFMT(rqs,
			fmt), typ, buf);
		if (len > 0)	/* Indent next line */
    		    STprintf(fmt, "RQF:Assn %d.        : %%s\n",
			     assoc->rql_assoc_id);
	    }
	    break;
	default:
	    STprintf(typ, " %d ", type);
	    RQTRACE(rqs,3)(RQFMT(rqs,
		fmt), typ, "(not traced)");
	    break;
    }
}

/*{
** Name: rqu_dmp_tdesc - Display/trace a GCA_TDESCR from LDB.
**
** Description:
**	This function will display/trace a GCA_TDESCR received from an LDB.
**	The gca_buffer is assumed to already be interpreted. 
**	Tuple descriptor will be output at trace level 4 or higher.
**
** Inputs:
**	assoc		    - The current ldb association.
**	td_data		    - Pointer to a tuple descriptor.
**	    
** Outputs:
** 	None
**        
** Returns:
**      VOID
**
** Side Effects:
**	    none
**
** History:
**      15-oct-1992 (fpang)
**          Created
**	21-Dec-2001 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE: use DB_COPY macro.
*/  
VOID
rqu_dmp_tdesc(
	RQL_ASSOC	*assoc,
	GCA_TD_DATA	*td_data)
{
    u_i4       i;
    DB_DATA_VALUE   db_val;
    GCA_COL_ATT     *att_p;
    GCA_COL_ATT     att;
    RQS_CB	    *rqs = (RQS_CB *)assoc->rql_rqr_cb->rqr_session;
    char	    noname[20];
    u_i2	    l_noname;
    char	    *colname;
    u_i2	    l_colname;

 
    if (! RQF_DBG_LVL(rqs,4))
	return;

    STprintf(noname, "<not named>");
    l_noname = STlength(noname);

    RQTRACE(rqs,4)(RQFMT(rqs,
		 "\tTsize (%d) modifier (0x%x) id (%d) cols (%d).\n"),
		 td_data->gca_tsize,
		 td_data->gca_result_modifier,
		 td_data->gca_id_tdscr,
		 td_data->gca_l_col_desc );

    att_p = (GCA_COL_ATT *)td_data->gca_col_desc;
    MEcopy((PTR)att_p, sizeof(att), (PTR)&att);
    for ( i=0; i < td_data->gca_l_col_desc; i++ )
    {
	/* do the display */
	DB_COPY_ATT_TO_DV( &att, &db_val );
	if ((td_data->gca_result_modifier & GCA_NAMES_MASK ) &&
	    (att.gca_l_attname >= 0))
	{
	    colname = att.gca_attname;
	    l_colname = att.gca_l_attname;
	}
	else
	{
	    colname = noname;
	    l_colname = l_noname;
	}
	RQTRACE(rqs,4)(RQFMT(rqs,
			"\tCol(%d): %t(%d) : type(%d) prec(%d) length(%d)\n"),
		        i, 
			l_colname, colname, att.gca_l_attname,
			db_val.db_datatype, db_val.db_prec, db_val.db_length );

        att_p = (GCA_COL_ATT *)((char* )att_p + sizeof(att_p->gca_attdbv)
                                       + sizeof(att_p->gca_l_attname)
				       + att.gca_l_attname);
        MEcopy((PTR)att_p, sizeof(att), (PTR)&att);
    }
}
