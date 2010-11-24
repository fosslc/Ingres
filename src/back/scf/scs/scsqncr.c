/*
** Copyright (c) 1986, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <er.h>
#include    <id.h>
#include    <me.h>
#include    <cm.h>
#include    <cs.h>
#include    <lk.h>
#include    <fp.h>
#include    <ex.h>
#include    <xa.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <adp.h>
#include    <gca.h>
#include    <copy.h>
#include    <generr.h>
#include    <sqlstate.h>
#include    <usererror.h>

#include    <dmf.h>
#include    <dmacb.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>

#include    <ddb.h>
#include    <qsf.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefscb.h>
#include    <qefqeu.h>
#include    <qefcopy.h>
#include    <qefnode.h>
#include    <opfcb.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefcb.h>
#include    <rdf.h>
#include    <scf.h>
#include    <sxf.h>
#include    <duf.h>
#include    <dudbms.h>

#include    <sc.h>
#include    <sc0m.h>
#include    <sc0a.h>
#include    <sc0e.h>
#include    <scc.h>
#include    <scfscf.h>
#include    <scs.h>
#include    <scd.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <scserver.h>
#include    <cui.h>
#if defined (axp_osf)
#include    <pthread_exception.h>
#endif

#include    <erfac.h>

#if defined(hp3_us5)
#pragma OPT_LEVEL 2
#endif /* hp3_us5 */

#if defined(i64_win)
#pragma function(memcpy)	/* Turn off usage of memcpy() intrinsic. */
#endif

/*
** NO_OPTIM = ris_us5 i64_aix sgi_us5
*/

/*
** LEVEL1_OPTIM = axp_osf
*/

#define	IIXA_XID_STRING_LENGTH		351
/* 
** Optimum number of tuples for DMF load for copy
** NOTE: Testing shows this to be less than 100 regardless of row size; 
** large chains in the linked list of rows appear to affect performance
** this is something we should look at in DMF at some point
*/
#define BEST_LOAD_TUPS			100


#define XA_XID_EQL_MACRO(a,b)  \
  ( ((a)->db_dis_tran_id_type == DB_XA_DIS_TRAN_ID_TYPE &&    \
  (a)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.formatID == \
  (b)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.formatID  && \
  (a)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length == \
  (b)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length && \
  (a)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.bqual_length == \
  (b)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.bqual_length && \
  (a)->db_dis_tran_id.db_xa_extd_dis_tran_id.branch_seqnum == \
  (b)->db_dis_tran_id.db_xa_extd_dis_tran_id.branch_seqnum && \
 !MEcmp((a)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.data, \
        (b)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.data, \
   (a)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length + \
   (a)->db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.bqual_length \
   ))?1:0)

/**
**
**  Name: SCSQNCR.C - This file contains the code for the Ingres Sequencer
**
**  Description:
**      This file contains the Ingres DBMS Sequencer and associated supporting
**      routines.  The sequencer is that part of the ingres system which
**	orchestrates the processing of queries and other database activity.
**
**          scs_sequencer() - the sequencer itself
**	    scs_input() - classify input an place it in QSF.
**          scs_qef_error() - Process a qef error.  This is done quite often,
**			so it has a separate routine.
**
**	    scs_qsf_error() - Process a qsf error.  This is done quite often,
**			so it has a separate routine.
**          scs_gca_error() - Process a gcf error.  This is done quite often,
**			so it has a separate routine.
**          scs_fetch_data() - GCA data into usable format.
**	    scs_qt_fetch() - Extract query text for PSF.
**          scs_desc_send() - Send descriptor to FE.
**	    sc0e_trace()	-   Send trace string to FE.
**          scs_tput() - Callback routine to aid in above.
**          scs_allocate_byref_tdesc()	- Allocate tdesc for sending byrefs.
**	    scs_allocate_tdata()	- Allocate GCA tdata message.
[@func_list@]...
**
**
**  History:
**      10-Jul-1986 (fred)
**          Created for Jupiter.
**      15-Nov-1986 (fred)
**          Added scs_send() to make readability easier.  Also moved this code
**          where appropriate to the cursor stuff.  Made changes to allow
**          sending of variable information to the parser.
**      23-Nov-1986 (fred)
**          Added CS compatibility in preparation for going multi-user.  Also,
**          added ULC_DATA capability to allow communication with general
**	    e*q*l/libq programs.
**      2-mar-1986 (fred)
**          Added SQL processing
**      28-May-1987 (fred)
**	    Altered scs_sequencer().  Added scs_desc_send().
**      08-Jul-1987 (fred)
**          Added GCA/GCF support
**	15-Jun-1988 (rogerk)
**	    Added fast commit support.
**	14-Oct-1988 (nancy) -- bug #3655 where if cursor name is size 
**	    DB_MAXNAME, then "if print_qry" sends a neg. address
**	    to TRformat causing AV's and stack overflows.  Send length of
**	    non-blank cursor name, OR DB_MAXNAME.
**      10-Mar-1989 (ac)
**          Added 2PC support.
**	12-Mar-1989 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Add entries for new statement modes 
**	    PSQ_[CADK]GROUP and PSQ_[CAK]APLID.
**	    Add support for statement modes PSQ_[CADK]GROUP, PSQ_[CAK]APLID.
**	3-apr-89 (paul)
**	    Add initializtion for qef_pflag when invoking a database procedure.
**	15-mar-89 (neil)
**	    Added support to dispatch rule query modes and to handle
**	    procedure recreation during rule processing.  A bit of LINT
**	    cleanup.
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Add new statement modes PSQ_REVOKE and PSQ_[GR]DBPRIV.
**	24-apr-89 (neil)
**	    Modified cursor processing to support rules.
**	23-may-89 (jrb)
**	    Changed ule_format calls for new interface.
**	14-jun-89 (dds)
**	    Added NO_OPTIM ming hint.  Sun 3 compiler croaks on this BIG file.
**		iropt seg fault
**	22-jun-89 (jrb)
**	    Added checks for proper capability licensing on certain terminator
**	    phase I features.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Add new statement mode PSQ_MXQUERY
**	26-jun-89 (jrb)
**	    Add generic error return for startup errors.
**	22-aug-89 (neil)
**	    Term2: Extended CALLPROC support - is now like a regular query.
**	07-sep-89 (neil)
**	    Alerters: Provided support for new event statements.,
**	19-Sep-89 (anton)
**	    Reinstate UNIX copy fix lost long ago
**	21-sep-1989 (ralph)
**	    Fixed bug 7475 and 8120.
**	28-sep-1989 (ralph)
**	    Bypass flattening if QEF_ZEROTUP_CHK.
**	    NOTE: This assumes repeat queries will always be retried.
**		  If not, the subsequent query will be suboptimized!
**	06-oct-1989 (fred)
**	    Fixed bug 8288 -- hetnet cursor problems.  Need to set
**	    scg_mdescriptor.
**	08-oct-1989 (ralph)
**	    For B1: Add new statement modes PSQ_[CAK]USER, PSQ_[CAK]LOC,
**		    PSQ_[CK]ALARM, PSQ_ENAUDIT, PSQ_DISAUDIT.
**	26-oct-1989 (paul)
**	    Changed scs_trace to sc0e_trace (and moved out) as now shared
**	    by other directories.
**      09-Jan-1990 (fred)
**	    Fixed bugs (a few) in scs_fdbp_data().  More comments there...
**	09-Jan-1990 (anton)
**	    Integrate porting group alignment and performance changes
**		from ingresug 90020
**	21-feb-1989 (carol)
**	    Added support for PSQ_COMMENT for comments in Terminator II.
**	2-apr-90 (andre)
**	    modified scs_input() to support a new GCA message - GCA_01DELETE.
**	08-aug-1990 (ralph)
**	    Restrict new C2 statements to C2 environments.
**	    Add new statement modes PSQ_[GR]OKPRIV to allow GRANT ON DATABASE
**	    with only UPDATE_SYSCAT and ACCESS privileges in vanilla environs.
**	    Enforce TRACE permission when trace points are set, except when
**	    application code is specified.
**	    Retry without flattening when OPF returns E_OP000A_RETRY
**	19-apr-90 (andre)
**	    Added support for PSQ_CSYNONYM and PSQ_DSYNONYM for synonyms.
**	17-sep-90 (teresa)
**	    the following PSQ_CB booleans have been changed to bitflags: 
**	    PSQ_IIQRYTEXT, PSQ_ALIAS_SET, PSQ_CLSALL, PSQ_CATUPD, PSQ_ALLDELUPD
**	10-oct-1990 (ralph)
**	    6.3->6.5 merge:
**	    01-may-1990 (neil)
**		Cursor Performance I: Support read-only cursor pre-fetch.
**	    23-may-90 (linda)
**		Update names of constants in SQNCR_TBL (see comments below).
**		Update constant names once again... see comments below.
**		Fix out-of-memory bug in scs_desc_send() (see comments there).
**	30-oct-1990 (ralph)
**	    Bug 33582 -- scs_fdbp_data gets confused when reading dbp parms
**	    Changed alignment fix for byte-aligned systems.
**	08-nov-90 (stec)
**	    Changed sequencer to pass the QSF query text object handle
**	    to OPF (OPC).
**	11-dec-1990 (ralph)
**	    Don't translate op_code to CS_OUTPUT if it's CS_TERMINATE (b33024)
**	01-16-91 (andre)
**	    Instead of introducing a new message - GCA_01DELETE - we will (in
**	    the future) define a new protocol level associated with a new
**	    structure for GCA_DELETE.
**	29-Jan-1991 (anton)
**	    Add support for GCA API 2 - call GCA_RQRESP
**	2-apr-1991 (nancy)
**	    Bug 31566 -- scs_scsqncr(), cursor not opened when open returns
**          warning status of no rows. (user sees E_LQ_0055 unexpected
**          initial protocal response).
**	07-nov-1991 (ralph)
**	    6.4->6.5 merge:
**		4-feb-91 (seputis)
**		    - initialized qeu_mask field, to support the OPF ACTIVE flag
**		    - check for new retry condition E_OP000D_RETRY to support
**		      the OPF ACTIVE flag
**		20-dec-90 (rachael)
**		    Fix bug 35000. Passing in null parameter in db procedure
**		    caused access violation in scs_input() in case GCA_INVPROC.
**		    Gdv was not being checked and was pointing to garbage.
**		4-feb-91 (rogerk)
**		    Added support for Set Nologging.  Added PSQ_SLOGGING
**		    query mode.
**		12-feb-1991 (mikem)
**		    Added support to scs_seqencer() for returning logical key
**		    values the GCA response block to the FE client. This is
**		    added so that the client can query the system maintained
**		    logical key automatically assigned by the system a result
**		    of the client's insert.
**		    @FIX_ME@ Support for this feature is disabled until it is
**		    @FIX_ME@ implemented in GCA.  See subsequent FIX_ME notes.
**		25-mar-91 (mikem)
**		    Moved initialization of the qef_val_logkey field of the qef
**		    control block so that it was initialized to 0 in all queries
**		    (not just inserts).  Also removed some logkey debugging code
**		    now that libq supports printing the returned logical key as
**		    part of "printgca".
**		25-feb-91 (rogerk)
**		    Moved PSQ_SLOGGING definition from 125->142 to not collide
**		    with new 6.5 query modes.
**		    Added PSQ_SON_ERROR query mode and support for Set Session
**		    with on_error statement to define transaction error handling.
**		25-feb-91 (rogerk)
**		    Fixed code which checks return status from qec_info call in
**		    Set Lockmode statements.  It used to not break on erorrs,
**		    but would continue with set lockmode processing.  Changed to
**		    break with GCA_FAIL_MASK qry_status, but with OK ret_val so
**		    it won't terminate the thread.
**		22-apr-91 (rogerk)
**		    Added check for E_DM0147_NOLG_BACKUP_ERROR return from
**		    dmc_alter call to set NoLogging state.
**		10-May-1991 (fred)
**		    Altered interrupt processing to set state to from negative
**		    to positive if important.  This is in line with
**		    scs_i_interpret() fix not to set scb_sscb.sscb_state
**		    during interrupts.
**		25-jul-91 (ralph)
**		    Fix bug 31585 -- scs_fdbp_data error when break on
**		    gca_parm_mask
**		25-jul-91 (johnr)
**		    hp3_us5 requires pragma to reduce optimization level.
**		    Full +O3 optimization results in out of memory error.
**		25-jul-91 (ralph)
**		    Fix bug 37827 -- handle (sscb_interrupt == -SCS_INTERRUPT)
**		    properly
**		    Fix bug 31585 (sun4) -- use MECOPY_CONST_MACRO in
**		    scs_fdbp_data to obtain unaligned integers
**		9-aug-91 (stevet)
**		    Disable the adjustment of PROTOCOL LEVEL on the GCA_RQRESP
**		    call. The 'cscb_version' is the negotiated PROTOCOL LEVEL
**		    agreed by FE/GCC/BE, adjusting this value on the GCA_RQRESP
**		    call does not make sense.
**		20-aug-91 (stevet)
**		    Fix B39166: Generate too many scs_cpinterrup() on
**		    FORCE_ABORT condition during COPY FROM operation.
**	12-feb-92 (nancy)
**	    Fix bug 38565.  
**	    In scs_sequencer() check for qef return status, E_QE0301_TBL_SIZE_
**	    CHANGE where E_QE0023 is checked.  Set retry counter to 0 so that
**	    this condition cannot cause a db_reorg error.  This allows a large
**	    insert that fires a rule to proceed without bumping up the retry
**	    counter.
**	16-apr-1992 (rog)
**	    Turned optimization back on for Sun 4.
**	    Fixed bug 42392: see scs_input for detailed comments.
**	30-apr-1992 (bryanp)
**	    Add handling for E_PS0009_DEADLOCK return from parser.
**	4-may-1992 (bryanp)
**	    Add new query modes PSQ_DGTT and PSQ_DGTT_AS_SELECT.
**      20-may-92 (schang)
**          GW integration.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              12-Sep-1988 (alexh)
**                  Added STAR direct connect routines.
**              30-Sep-1988 (alexh)
**                  Added create link query mode to the sequencer. sqncr_tbl is
**                  also modified for this.
**              7-Oct-1988 (alexh)
**                  Turned direct GCA calls into ansychronous calls.
**              19-Jan-1989 (alexh)
**                  Added STAR copy
**              07-aug-89 (georgeg)
**                  Fix from jupiter, fixed by (fred)
**                  Fixed startup errors so that gca_local_error is cleared 
**		    (set to 0).  This is necessary to support the FSGW, as well
**		    as any other (user) program, which relies upon getting the 
**		    correct startup error.
**              02-oct-89 (georgeg)
**                  Added scs_tdmsg_dispose()
**              08-nov-89 (georgeg)
**                  Support generic errors only if the FE is GCA_PROTCOL_LEVEL_2
**		    or higher.
**              20-feb-90 (carl)
**                  Incorporated alignment fixes from the 62ug porting line in:
**                  scs_input, scs_fetch_data, scs_fdbp_data and scs_desc_send
**              03-apr-90 (carl)
**                  Moved scs_dconnect to scsstar.c
**              08-oct-90 (georgeg)
**                  fixed some of the tuple descriptor problems, there were 
**		    cases where the tuple descriptors were not properly 
**		    deallocated at the end of the query. Also changed 
**		    scs_tdmsg_dispose() to set ..cscb_new_stream.. properly.
**              31-jan-91 (fpang)
**                  Added support for GCA_EVENT.
**              17-jun-91 (fpang)
**                  Initialized qef_rcb->qef_modifier, causes qef to get 
**		    confused about the tranxasction state if it is not 
**		    initialized.
**                  Fixes B37913.
**              13-aug-91 (fpang)
**                  1. Call scs_sdctrace() to display trace messages if in
**                     direct connect.
**                  2. Disallow 'Execute procedure' that come in as GCA_INVPROC
**                     messages.
**              10-May-1991 (fred)
**                  Altered interrupt processing to set state to from negative 
**		    to positive if important.  This is in line with 
**		    scs_i_interpret() fix not to set scb_sscb.sscb_state during
**		    interrupts.
**          End of STAR comments.
**	25-jun-1992 (fpang)
**	    Included ddb.h and related include files for Sybil merge.
**	02-aug-92 (andre)
**	    if qmode==PSQ_GRANT call psy_call() with opcode PSY_GRANT
**	    (psy_call() will call psy_dgrant() directly instead of going through
**	    psy_dperm());
**
**	    if qmode==PSQ_REVOKE call psy_call() with opcode PSY_REVOKE
**	    (psy_call() will invoke psy_revoke() to perform revocation of
**	    specified privileges
**	17-aug-1992 (rog)
**	    Fixed bug 43905: Some QP's were not being destroyed during
**	    COPY FROM operations.
**	28-aug-1992 (pholman)
**	    C2 Security: now part of the base product, with security auditing
**	    being performed by the SXF.  Also implement ENABLE/DISABLE
**	    SECURITY_AUDIT as an atomic operation, that is not allowed within
**	    a transaction.
**	22-jul-1992 (fpang)
**	    In scs_sequencer(), added support for new RDF cache flushing
**	    protocol, which eliminates PSQ_DELRNG, but instead lets PSF know
**	    that the sequencer is retrying by setting the PSQ_REDO bit
**	    in psq_flag.
**	04-sep-1992 (fpang)
**	    In scs_sequencer(), the distributed server only supports gca_protocol_40,
**	    so negociated the FE down to 40 if it is higher. Also fixed interrupt
**	    handling in distributed 'direct connect'.	    
**	14-sep-1992 (robf)
**	    Added support for C2/GWF tracing (PSQ_GWFTRACE)
**	    Added tm.h (needed by sxf.h>
**      23-sep-92 (stevet)
**          Added the check for FE protocol level back.  If FE
**          is running at higher than GCA_PROTOCOL_LEVEL_60, server will
**          negotiate the connection down to GCA_PROTOCOL_LEVEL_60.  Als
**          set the scb->scb_cscb.cscb_version to the negotiated level
**	3-oct-1992 (nandak)
**	    New distributed transaction ID type for XA support.
**	05-oct-1992 (fred)
**	    Fix large object bug where we were storing a pointer in a bool.
**	    Not too good.
**	30-Oct-1992 (fred)
**	    Fix bug where not enough tuple buffer was allocated when queries
**	    involving large objects were allocated.
**	27-oct-1992 (fpang)
**	    Distributed. In scs_sequencer(), if errors occur before rows 
**	    are returned, remove the tuple descriptor from the queue.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	09-nov-92 (jrb)
**	    Added support for PSQ_SWORKLOC ("set work locations..." command).
**	    Removed PSQ_SEMBED since I reused this one for PSQ_SWORKLOC.
**	    This is for multi-location sorts project.
**	19-nov-1992 (fpang)
**	    Distributed. In scs_sequencer(), make sure qef_intstate is 0
**	    when doing an LDB TDESCR pre-fetches.
**	10-nov-92 (robf)
**	    Make sure SXF audit data is flushed prior to returning anything
**	    to the user. 
**	27-nov-92 (robf)
**	    Modularize security auditing in sc0a_write_audit
**	25-nov-1992 (markg)
**	    Modified PSQ_ENAUDIT/ PSQ_DISAUDIT handling in scs_sequencer to
**	    not commit the current transaction, as this functionality is
**	    now performed in SXF.
**      03-dec-92 (nandak, mani)
**          Added GCA_XA_SECURE support for Ingres65 XA project.
**	14-dec-92 (jhahn)
**	    Added more support for byrefs. Also added tracepoint SC912
**	    (to turn off byref protocol.
**      14-dec-92 (anitap)
**          Changed the order of #include <psfparse.h> for CREATE SCHEMA.
**	    <psfparse.h> should be included before <qefact.h>.
**          Added new query mode PSQ_CREATE_SCHEMA.
**      18-dec-92 (anitap)
**          Added query mode PSQ_CREATE_SCHEMA to the switch statement so that
**          QEF is called for CREATE SCHEMA statement. Check if we need to
**          parse EXECUTE IMMEDIATE text and reset "retry" counter.
**          Added support to parse E.I. qtext.
**      13-jan-93 (mikem)
**          Lint directed changes. Fixed 2 calls to scs_cpsave_err() to pass
**          the correct number of arguments (2).  The third argument was always
**          ignored by the routine.
**      18-Jan-1993 (fred)
**          Altered calls to adu_couponify() to match cleanup of that
**          routine.  Altered singleton variable setting to enable
**          large object's to participate in cursor fetches correctly.
**	16-nov-92 (rickh)
**	    CREATE TABLE statement now gets diverted to the optimizer just like
**	    DML statements.  "Another FIPS CONSTRAINTS improvement."
**	05-jan-93 (rblumer)
**	    when calling parser with RECREATE,
**	    copy the set-input table and proc ids from QEF cb to PSQ cb;
**          added new query mode PSQ_ALTERTABLE, plus placeholder PSQ_CONS;
**          also added new query mode PSQ_CRESETDBP (create set-input proc).
**	18-jan-93 (fpang)
**	    Support distributed register database procedure (PSQ_REGPROC) and
**	    distributed execute database procedure (PSQ_EXEPDB and 
**	    PSQ_DDEXECPROC).
**	18-jan-1993 (rog)
**	    Fixed 46715 in scs_input().
**	20-jan-1993 (fpang)
**	    Fixed merge problem.
**	10-feb-93 (rblumer)
**	    Added PSQ_DISTCREATE query mode, so distributed CREATE TABLES are
**	    sent through the QEF DBU processing instead of OPF processing.
**	11-feb-93 (anitap)
**	    Fixed bugs in scs_sequencer() for CREATE SCHEMA project.
**  	19-Feb-1993 (fred)
**  	    Added support for large objects in COPY.
**	19-feb-93 (andre)
**	    added code to handle PSQ_DROP_SCHEMA
**	12-Mar-1993 (daveb)
**	    Add include <st.h> <me.h>
**	22-feb-93 (rblumer)
**	    after calling psy_credbp for set-input proc, free the memory for
**	    the query tree object.
**	03-mar-93 (anitap)
**	    Added support for execute immediate processing for rules and 
**	    set-input procedures. Prototyped scs_chkretry(). 
**	16-mar-93 (ralph)
**	    DELIM_IDENT: Change interface to PSQ_ALTER to pass case
**	    translation semantics and revised username.
**	17-mar-93 (ralph)
**	    Fixed compiler problems.
**	19-mar-93 (rickh)
**	    Extended execute immediate processing to include index creation.
**	23-mar-1993 (fred)
**	    Fixed bugs in copy from file w.r.t. large object support.
**	06-apr-93 (anitap)
**	    Error handling for CREATE SCHEMA in case of rule, procedure, index
**	    and GRANT.
**      07-apr-1993 (stevet)
**          Added adg_vlup_setnull() to reset null byte to end of
**          string for VARCHAR repeat query parameters (B40257).
**	    Fixed compiler problems
**	20-apr-93 (jhahn)
**	    Added code to check to see if transaction statements should be
**	    allowed.
**	06-apr-93 (ralph)
**	    DELIM_IDENT: Don't pass case translation semantics to PSQ_ALTER
**	05-may-93 (ralph)
**	    DELIM_IDENT:
** 	    Normalize/translate identifiers in formatted GCA buffers.
**	11-may-93 (anitap)
**	    Support for "set update_rowcount" statement.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	    Deleted 2nd inclusion of <me.h> -- one time is enough.
**	25-may-93 (fpang)
**	    In scs_input(), added code to support the GCA1_INVPROC message, 
**	    which is a new GCA Execute Procedure message that contains an 
**	    owner field.
**	26-may-93 (ralph)
**	    DELIM_IDENT:
** 	    Normalize/translate identifiers in formatted GCA buffers.
**	08-jun-1993 (jhahn)
**	    Added initialization of qe_cb->qef_no_xact_stmt.
**	16-jun-1993 (robf)
**	    Add PSQ_SXFTRACE for SXF tracing
**	    Add PSQ_ALTAUDIT for ALTER SECURITY_AUDIT statement
**      28-jun-93 (tam)
**          In scs_sequencer(), added support for SET SESSION AUTHORIZATION 
**	    statement (query mode
**          == PSQ_SET_SESS_AUTH_ID) in star. Note that star will only change
**          the user authorization id of the star level control blocks (in psf
**          and qef).  This is deemed sufficient to handle the need for
**          unloaddb/reloaddb, which is the requirement for this change.  The
**          user id for rqf/tpf and the local connections for this star session
**	    is not changed.  To reset the local authorizations, the user has to
**	    do it through DIRECT CONNECT or DIRECT EXECUTE IMMEDIATE. The 
**	    general issue of star/local access control will be addressed after
**          6.5.  Leveraged on existing code for local except the dmf call to
**          reset the dmf_cb's user id is bypassed.  Star has no dmf.
**	07-jul-1993(shailaja)
**	    Fixed prototype incompatibility.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	06-jul-93 (rblumer)
**	    in scs_chkretry, must break out if already had an error BEFORE the
**	    procedure was called; else can end up calling error handler with
**	    bogus error number, which shows up as an internal error.
**	07-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	07-jul-93 (anitap)
**	    Changed QEF_SAVED_RSRC to QEF_DBPROC_QP. Removed Eric's FIXME note.
**	    Added the query mode numbers next to the query modes in SQNCR_TBL
**	    for clarity.  
**	    We will not assign the E.I. query text handle to sscb_thandle as we
**	    were stamping on the handle to the query text of CREATE SCHEMA/
**	    CREATE TABLE with constraints/CREATE VIEW(WCO). This led to the
**	    following error in the error log: "E_QS001E_ORPHANED_OBJ An 
**	    orphaned Query Text object was found and destroyed during QSF 
**	    session exit."
**	22-jul-1993 (fpang)
**	    In scs_input(), fixed byte alignment problems related to 
**	    GCA1_INVPROC and owner.procedure. 
**	26-jul-1993 (jhahn)
**	    Moved coercing of returned byref procedure parameters from the
**	    back end to the front end.
**	    Removed scs_send_back_byrefs(). Added scs_allocate_byref_tdesc()
**	    and scs_allocate_tdata().
**	27-jul-93 (ralph)
**	    DELIM_IDENT:
**	    Remove unused variables from PSQ_CREDBP case.
**	    Add comments re cursor name translation to GCA_QUERY case.
**	    Case translate owner name in GCA1_INVPROC case.
**	    Call cus_trmwhite() to determine identifier length.
**	27-jul-1993 (jhahn)
**	    Fixed integration conflicts. Replaced qef_wants_qp with
**	    qef_wants_ruleqp in scs_sequencer(). Used prototyped versions
**	    of sc0e_uput in scs_input().
**	10-Aug-1993 (jhahn)
**	    Fixed problem when sending back names in
**	    scs_allocate_byref_tdesc().
**      12-Aug-1993 (fred)
**          Set up to remove large object temps at end of query.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	16-Sep-1993 (daveb)
**	    Changes to support 'drop CONNECTION' iimonitor command and
**	    MO objects.	     
**	13-sep-1993 (dianeh)
**	    Removed NO_OPTIM setting for obsolete Sun3 and PC/RT (sun_u42,
**	    rtp_us5).
**	13-Oct-1993 (fpang)
**	    Star. Don't call opf for PSQ_SRINTO (set result structure).
**	    Fixes B55675, 'set result structure' results in internal error.
**       8-Oct-1993 (fred)
**          Fixed handling of sscb_dmm setting after an interrupt.
**          With large object handling, this was sometimes not
**          cleared.  We now clear it as we prepare to send the
**          interrupt ack message.
**	18-Oct-1993 (jnash)
**	    Fix AV encountered during COPY force-abort processing.
**      23-sep-1993 (iyer)
**          The message passed for GCA_XA_SECURE has changed, it is now
**          an extended struct DB_XA_EXTD_DIS_TRAN_ID which includes the
**          previous message struct DB_XA_DIS_TRAN_ID. In addition it has
**          two members: branch_seqnum and branch_flag. 
**	26-oct-93 (robf)
**          Add PSQ_MXSESSION
**       3-Nov-1993 (fred)
**          Altered the STAR rqf tuple descriptor storage.  These are
**          now stored in the session control block instead of on the
**          stack.  It is a long term (query lived) object that
**          shouldn't be stored as an automatic variable.
**	01-nov-93 (anitap)
**	    If qmode = PSQ_DGTT_AS_SELECT, set qef_stmt_flag in QEF_RCB so that
**	    QEF does not create implict schema for "declare global temporary
**	    table as..."
**	08-nov-93 (rblumer)
**	    Fix typo that could cause an uninitialized dl1_data to be used.
**	16-nov-93 (rickh)
**	    When unpacking repeat query parameters, we could estimate too
**	    few parameters.  This would cause us to allocate too few
**	    parameter pointers, which in turn would allow us to trash
**	    SCF's free list of memory chunks.  And that brought down the
**	    server.  Fixed this.  This closes bug 55021.
**      29-dec-93 (anitap)
**          Fixed bug 57815. The query trees for indexes, rules and permits
**          were not being destroyed for CREATE SCHEMA statements with CREATE
**          TABLE with UNIQUE CONSTRAINT, CHECK constraint, REFERENTIAL
**          constraint, GRANT statement. This bug occured only if any of the
**          above constraints appeared in pairs or with the GRANT statement.
**          On return from scs_chkretry(), we were continuing with parsing
**          of the next execute immediate statement before destroying the
**          query tree of the previous execute immediate statement. The bug
**          gave the following errors:
**          "E_QS001E_ORPHANED_OBJ     An orphaned Query Plan object was
**          found and destroyed during QSF session exit."
**          "E_QS0014_EXLOCK   QSF Object is already locked exclusively."
**      17-Jan-1994 (fred)
**          Fix bug in scs_fetch_data() which was causing blob
**          parameters to repeat queries to be set up incorrectly.
**	07-jan-94 (rblumer)	
**	    Several calls to cui_idxlate were not recording the status returned
**	    (but were trying to look at it anyways); will now write to the
**	    error log any error returned by cui_idxlate call (especially since
**	    the error sent to the user doesn't seem to get through).  B58511
**	24-jan-1994 (gmanning)
**	    Change references from CS_SID to CS_SCB and CSget_scb.
**	31-jan-31 (mikem)
**	    bug #58501
**	    Replace the TRformat call in part of scs_input() with in-line ME 
**	    code to streamline the dbp path through the code.  This is part of 
**	    the changes being made to 6.5 to try and get cpu performance of 6.5 
**          executing short dbp's to perform as well or better as 6.4. 
**	09-oct-93 (swm)
**	    Bugs #56437 #56447
**	    Put session id. (cs_self) into new dmc_session_id instead of
**	    dmc_id.
**	    Eliminate compiler warnings by removing PTR casts of cs_self.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Altered sc0e_trace calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to sc0e_trace.
**	    This works because STprintf is a varargs function in the CL.
**	31-jan-1994 (rog)
**	    Initialize qe_copy members earlier so that we don't have
**	    uninitialized variables if the copy transaction fails.  (Bug 56362)
**	25-mar-1994 (fpang)
**	    In scs_sequencer, if SCS_DROPPED_GCA is set, reset 
**	    scb->scb_sscb.sscb_state to SCS_TERMINATE before returning. Also if
**	    qmode is zero, set qe_ccb because error reporting needs it.
**	    Fixes B60599, Server gets E_SC020A_UNRECOGNIZED_QMODE if FE client
**	    exits w/o disconnecting, and B60950, Server SEGV's after E_SC020A.
**	22-feb-94 (rblumer)
**	    in scs_input and scs_fdbp_data,
**	    change dbproc name translation to use cui_f_idxlate when possible;
**	    this function is much faster than cui_idxlate.
**      13-Apr-1994 (fred)
**          In scs_blob_fetch(), if adu_couponify() fails, set system
**          up to flush remainder of large object input.  Make
**          scs_fetch_data() follow thru on that wish.
**      15-Apr-1994 (fred)
**          Added parameter to adu_free_objects.  This now takes a
**          statement number (supplied via QEF) which is used to
**          remove only the objects associated with this statement.
**          this allows us to be a bit more discriminating about
**          blowing temporaries out of the water.
**      02-Oct-1994 (Bin Li)
**          Fix bug 60932, at line 1877, add condition statement to test
**          the return value from scs_initiate file. If the return value
**          is hex 10(E_US0010), add function call 'ule_format' and pass
**          the database name.
**	19-jul-94 (robf)
**          swm's 11-oct-93 change seems to have broken SET PRINTQRY since
**          STprintf/SIprintf do not support the %#s format used. Changed
**          usage to documented %*s.
**	09-sep-1994 (mikem)
**	    bug #65094
**	    Interrupts during DBU operations could cause memory trashing/AV's.
**	    Changed the interrupt handling portion of the sequencer to make
**	    was using a QEU_CB when it wanted an QEF_RCB.
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**      20-dec-1994 (chech02)
**          removed some unnecessary casts (int to char*) that cause
**          the AIX 4.1 compiler to complain
**      20-dec-1994 (nanpr01)
**	    Bug # 62502. Field ics_act1 is not getting cleared.	
**      06-Feb-95 (liibi01)
**          Bug #66437, get rid of the bonus message.
**      04-apr-1995 (dilma04) 
**          Added support for PSQ_STRANSACTION.
**	24-apr-95 (jenjo02/lewda02)
**	    Add support for new DMF API.  Recognize the new query language
**	    type DB_NDMF and pass control to scs_api_call which calls dmf_call
**	    directly.
**	27-apr-95 (lewda02)
**	    API project: make sure the messages are queries before checking
**	    for DB_NDMF and calling scs_api_call().
**	11-may-95 (lewda02/shust01)
**	    RAAT API Project:
**	    Change naming convention.  API is now RAAT (Record At A Time) API.
**      1-june-1995 (hanch04)
**          Removed NO_OPTIM su4_us5, installed sun patch
**	15-jun-95 (thaju02)
**	    Clear the language id of DB_NDMF so that we do not make an
**	    scs_raat_call() during exceptional condition handling when using
**	    the RAAT API.
**      15-may-1995 (thoro01)
**          Added NO_OPTIM hp8_us5 to this file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch for c89
**	09-aug-1995 (canor01)
**	    for NT: after call to scc_iprime(), check for broken pipe in
**	    the expedited channel as well as the normal channel
**	14-sep-95 (shust01/thaju02)
**	    Added calls to scs_raat_load() which adds raat
**	    GCA buffers to link list, and scs_raat_parse(), which concatenates
**	    them into a large buffer before calling scs_raat_call().
**    5-sep-1995 (angusm)
**        bug 70170: in scs_sequencer, stash qp handle for repeated 
**		  SELECTs , else redundant shared QSF locks get taken,
**		  preventing proper flushing etc of QSF cache.
**	14-nov-1995 (kch)
**	    In scs_sequencer(), set the current query mode in QEU_CB,
**	    qeu.qeu_qmode to scb->scb_sscb.sscb_qmode. This change
**	    fixes bug 72504.
**	12-dec-1995 (nick)
**	    Remove NO_OPTIM for su4_us5 ; it shouldn't be here now the
**	    V3 compiler is patched.
**	15-dec-95 (pchang)
**	    In scs_def_curs(), set the query status to GCA_REMNULS_MASK after 
**	    opening the cursor, if null values were eliminated when evaluating
**	    aggregate functions.  This will ensure that the SQLSTATE warning 
**	    SS01003_NULL_ELIM_IN_SETFUNC is correctly issued to the FE (B73095).
**          Added NO_OPTIM hp8_us5 to this file.
**	10-jan-1995 (murf)
**		Added NO_OPTIM sui_us5 to this file, but the code appears to
**		be commented out for the hp8_us5 & su4_us5. If this is no
**		longer necessary why do we not just remove it ?
**		Yes, I realize it is supposed to be commented out, I must be
**		going crazy. 
**	16-Nov-95 (gordy)
**	    Cursor pre-fetch now supported through GCA1_FETCH message at
**	    GCA_PROTOCOL_LEVEL_62.
**      28-nov-95 (lewda02/thaju02/stial02)
**          Enhanced XA Support.
**	12-dec-1995 (nick)
**	    Remove NO_OPTIM for su4_us5 ; it shouldn't be here now the
**	    V3 compiler is patched.
**	15-dec-95 (pchang)
**	    In scs_def_curs(), set the query status to GCA_REMNULS_MASK after 
**	    opening the cursor, if null values were eliminated when evaluating
**	    aggregate functions.  This will ensure that the SQLSTATE warning 
**	    SS01003_NULL_ELIM_IN_SETFUNC is correctly issued to the FE (B73095).
**	5-jan-95 (hanch04)
**	    Fix bad cross integration.
**      06-mar-1996 (nanpr01)
**	    Removed the dependency of DB_MAXTUP. Required for increasing
**	    Tuple size. Replace DB_MAXTUP with Sc_main_cb->sc_maxtup.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**          Need to calculate sco_length + sco_xtra_bytes
**	04-apr-1996 (loera01)
**	    Modified scs_sequencer() and scs_desc_send() to support
**	    compressed variable-length datatypes. 
**      25-apr-1996 (loera01)
**	    Modified scs_sequencer() so that opf_names mask is initialized
**	    before the GCA_NAMES_MASK or GCA_COMP_MASK is set. 
**      14-jun-1996 (loera01)
**          In scs_sequencer(), modified setting of result modifier in tuple 
**          descriptor so that the GCA_COMP_MASK is explicitly turned off, if 
**          compression is not supported.
**	03-jun-1996 (canor01)
**	    Removed semaphores protecting global statistics. Cast parmlist
**	    on call to IIGCa_call to quiet compiler warnings about
**	    mismatch on prototype.
**	13-jun-96 (nick)
**	    LINT directed changes.
**      23-jul-1996 (ramra01 for bryanp)
**          Added support for PSQ_ATBL_ADD_COLUMN and PSQ_ATBL_DROP_COLUMN
**	12-sep-96 (kch)
**	    In scs_sequencer() and scs_input() add an additional check for the
**	    first RAAT GCA buffer (sscb_thandle == 0). This change fixes bugs
**	    78299 and 78345.
**	 9-oct-96 (nick)
**	    Ensure GCA_RETPROC returns the procedure id related to the
**	    procedure named in the corresponding GCA_INVPROC.  Some LINT
**	    directed stuff.
**	30-oct-96 (pchang)
**	    Added handling for E_PS000A_LK_TIMEOUT return from parser (B73795).
**	    Replaced magic number 4700 with E_US125C_4700_DEADLOCK.
**	27-nov-96 (kch)
**	    In scs_input(), declare l_name for both byte aligned and non byte
**	    aligned platforms.
**	10-oct-1996 (canor01)
**	    Make messages more generic.
**      21-jan-97 (loera01)
**	    Bug 79287: In scs_sequencer() and scs_desc_send(), don't compress 
**	    data if the tuple has a blob.
**	11-feb-1997(angusm)
**	    Add conditional diags to scs_input to trace query text.
**      27-feb-1997 (stial01)
**          Set lockmode: Mapped E_DM010E_SET_LOCKMODE_ROW 
**          to E_SC0380_SET_LOCKMODE_ROW
**	27-Feb-1997 (jenjo02)
**	    Extended <set session> statement to support <session access mode>
**	    (READ ONLY | READ WRITE).
**	    For qmode == PSQ_RETRIEVE queries, set qef_qacc = QEF_READONLY
**	    prior to calling QEQ_QUERY. If a single statement txn needs to be
**	    started, this will open it as READ ONLY and avoid stalling with
**	    a CKPDB-pending database.
**	03-apr-97 (nanpr01)
**	    If autocommit is on, you may get hangup on sessions because
**	    of uninitialized variable. see change # 421453.
**	09-apr-97 (thaju02)
**	    bug #81398 - If autocommit is on, blob insertion results in 
**	    E_DM9B08 and E_AD7008.  Attempting to cleanup up temporary 
**	    file(s) outside of a transaction.  Added check: if autocommit 
**	    on do not call adu_lo_delete, instead let session termination 
**	    cleanup temporary files.
**	31-may-97 (pchang and devjo01)
**	    Fixed a bad pointer assignment (*p not p) in scs_input() that
**	    have caused mayhem, especially on Digital Unix platforms.
**	01-jun-97 (pchang)
**	    Fixed incorrect capture of gca_proc_mask from GCA1_IP_DATA type
**	    messages when processing GCA1_INVPROC requests in scs_input().
**	03-apr-97 (nanpr01)
**	    If autocommit is on, you may get hangup on sessions because
**	    of uninitialized variable. see change # 421453.
**	25-Apr-1997 (shero03)
**	    Save the tuple descriptor off the sscs_cquery to be used by
**	    scs_compress.
**	29-May-1997 (shero03)
**	    Save the error code from qeu_b_copy before destroying it.
** 	05-jun-1997 (wonst02)
** 	    Set READONLY transactions for READONLY databases.
**	26-jun-97 (hayke02)
**	    In the function scs_input(), move code which aligns dbdv.db_data
**	    to before the second call to adu_2prvalue(). This change fixes
**	    bug 83430.
**      15-sep-97 (stial01)
**          scs_input() GCA_INVOKE, GCA_INVPROC: If input data comes in one
**          GCA buffer, check for blob parameters in the input, which must be
**          read with scs_fetch_data and can't be processed 'in-place' (B85214)
**	11-mar-1997 (walro03)
**	    No-optimize for AIX 3.2 (ris_us5).  Compiler hangs when this file
**	    is optimized.
**      28-aug-97 (musro02)
**          l_name is only defined for BYTE_ALIGN machines, but is referenced
**          in the "then" and in the "else" of an "ifdef BYTE_ALIGN".
**          Make the definition unconditional.
**	24-sep-1997 (hanch04)
**	    Back out previous change.  No need to redefine variables.
**      18-Sep-97 (fanra01)
**          Bug 85435
**          Ensure that the gca_result_modifier compression bit is not set in
**          a COPY FROM operation.  Varchar cmopression is not supported in
**          this case.  During a net copy gcc reacts to the compression bit
**          but libq sends uncompressed varchar causing gcc to misinterpret
**          the tuple contents and loop forever.
**      14-oct-1997 (stial01)
**          scs_set_session() Extended for set session isolation.
**	14-nov-1997 (wonst02)
** 	    Bug 86490 - Varchar compression and cursors
** 	    Save tdesc in sscb_cquery only for non-cursor queries.
**	18-nov-1997 (wonst02)
**	    Fix one more glitch in varchar comp. and cursors.
**      18-Dec-1997 (stial01)
**          Disallow blob input for GCA_DEFINE, GCA_INVOKE
**	17-feb-1998 (somsa01)
**	    When checking for GCA_COMP_MASK, we may have hit situations
**	    where we should not be checking for this bit. We were checking
**	    this bit in gca_result_modifier, which was referenced off of
**	    rd_tdesc. However, at query execution time, rd_tdesc is valid
**	    ONLY if varchar compression is turned on (since that's when we
**	    SAVE a copy of the tuple descriptor, while other times we just
**	    reference the one from the query plan which is "invalidated" at
**	    execution time). We now check this bit inside either
**	    rd_modifier or csi_state.  (Bug #89000)
**	24-Mar-1998 (thaju02)
**	    Bug #87880 - inserting or copying blobs into a temp table chews
**	    up cpu, locks and file descriptors. Modified scs_lowksp_alloc().
**	16-Mar-1998 (jenjo02)
**	    Lightweight Factotum threads. Avoid unnecessary function calls
**	    when running "special" system threads.
**	8-jun-1998(angusm)
**	    Cross-integrate mckba01's fix from ingres63p for bug 71293
**	    (repeated select with params failure on 2nd invocation)
**	26-jun-1998 (nanpr01)
**	    Fix-up the change on 17-feb since it may look at stale rd_modifier.
**	22-jun-1998 (kinte01)
**	    Cross-integrate change 436175 from oping12
**	17-jun-1998 (horda03)
**	    A previous change to incorporate Evidence Set diagnostics
**	    failed to update the query text of the current query.
**	30-jun-1998 (hanch04)
**	    Changed diag_qry to cs_diag_qry to comply with coding standards.
**      22-jun-1998 (kinte01)
**          Cross-integrate change 436175 from oping12
**          17-jun-1998 (horda03)
**            A previous change to incorporate Evidence Set diagnostics
**            failed to update the query text of the current query.
**	08-jul-1998 (ocoro02)
**	    Changed diag_qry to cs_diag_qry to comply with coding standards.
**	    Removed unnecessary end-of-comment block delimiter.
**	07-aug-1998 (bobmart)
**	    Fixed argument positions for a couple of sc0m_allocate() calls; 
**	    "type" and "owner" were transposed.  In particular, the one 
**	    introduced with the fix for 71293-- AIX 4.1 (rs4_us5) compiler
**	    treated this as a severe error.
**          Add trace point RD0013 to dump out ulf memory in II_DBMS_LOG
**	03-Sep-1998 (jenjo02)
**	    When executing "set [no]trace point" commands, leave a footprint
**	    in II_DBMS_LOG by TRdisplay-ing the command.
**	07-Dec-98 (i4jo01) 
**	    Cross-integration of change 438029 from oping12.
**	    Make sure we unset the 'session is idle' flag when receiving
**	    user input (b93619).
**	16-Nov-1998 (jenjo02)
**	    When suspending on BIO read, use CS_BIOR_MASK.
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer. Also
**	    corrected the status handling in scs_input where it was resetting
**	    the E_DB_WARD from GCA_RELEASE case and causing it to take
**	    wrong path.
**	19-jan-1999 (muhpa01)
**	    Removed NO_OPTIM for hp8_us5.
**      26-jan-1999 (horda03) X-integration of change 437979 from oping12.
**        29-Sep-1998 (horda03)
**          If a new session is terminated before it has completed initialisation,
**          an ACCVIO may occur. The ACCVIO occurs because the SCB for the session
**          is removed before an asynchronous message completes. At the
**          same time, a "ULE_FORMAT error 0" may be raised. Bug 90634.
**	29-jan-1999 (kinpa04) bug#95070
**	    There is a static char field being allocated as part of the 
**	    CS_TERMINATE code which may not be aligned at the time of its
**	    allocation and needs to be aligned when the address is being
**	    put into the GCA_FORMAT parms block.
**	16-Feb-1999 (shero03)
**	    Support set random_seed to a local value.
**   08-mar-99 (rosga02) Removed NO_OPTIM for sui_us5
**	13-apr-1999 (somsa01)
**	    In scs_blob_fetch(), when coerced to a null and an add extra byte
**	    needs to be added, initialize the null indicator bit.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	27-Apr-1999 (ahaal01)
**	    Moved "include ex.h" after "include fp.h".  The check for
**	    "ifdef EXCONTINUE" in ex.h does no good if sys/context.h
**	    (via fp.h) is included after ex.h.
**      30-jul-1999 (stial01)
**          Fixes for blob enhancements made by stephenb for SIR 97558
**      12-aug-1999 (stial01)
**          scs_blob_fetch() Always call adu_couponify with pop_info set
**          Remove optimization for COPY, as we might be doing LOAD without
**          logging
**      28-sep-1999 (chash01)
**          the constant SCS_RETRY_LIMIT in scs_sequencer() is too small (3.)
**          RMS gateway run into problem with error of "US_1265 data base
**          reorganization..." I expand it to 10. bug#99066
**      27-mar-2000 (stial01)
**          scs_blob_fetch() init pop_info to NULL
**      30-Jun-2000 (horda03) X-Integration of change 276634
**        15-may-1998 (horda03)
**          There is a problem where a thread which is suspended waiting for a
**          message from a FE application is resumed before the message is
**          delivered (Bug 90875). Under there circumstances the GCA_STATUS of
**          the message is E_GCFFFF. In SCS_INPUT, if the GCA_STATUS is not
**          success, a test is made to see if the thread has been prematurely
**          resumed. If it has, the thread returns to the suspended state.
**          Bug 90875 occurs (on AXP.VMS) if the mesasge is delivered between
**          the time at which the GCA_STATUS is tested for success and for
**          E_GCFFFF (in scs_input). On AXP when a message is delivered, an AST
**          (ASynchronous Trap) is triggered, which swops out the DBMS thread,
**          copies the message to the mesage buffer, updates the GCA_STATUS and
**          resumes the DBMS thread. It is the change to the GCA_STATUS from
**          E_GCFFFF to 0 which causes the ULE_FORMAT error 0 problem.
**      04-Jul_2000 (horda03)
**          VMS only. Only applicable while Ingres built with /NOMEMBER_ALIGNMENT.
**          If an AST fires for a mailbox which this code is checking the GCA_STATUS
**          then a corrupt value of the status may be determined; the AST must arrive
**          whilst the code is accessing the variable; because of the /NOMEMBER
**          compiler option, the variable may straddle two quadwords, so two memory
**          access operations are required. This introduces a window where the code
**          can determine one part of the value, the AST updates the variable and then
**          the code determines theother part of the variable. Temporary solution is
**          to disable ASTs while accessing the GCA_STATUS variable. (101951/INGSRV1206).
**      09-Feb-2001 (ashco01) bug#102568
**          In scs_input(), removed duplicate call of adu_2prvalue() when processing 
**          'execute statement' statements for 'set printqry' traces set via ING_SET.
**	04-jan-2000 (rigka01) bug#99199
**	    in scs_sequencer, 
**	    when processing the execute database procedure statement,
**	    set qeu_flag to a new QEF_DBPROC value so that qeu_btran() can
**	    check the flag and set qeu_modifier to prevent the transaction
**	    state from being changed before the first statement of the 
**	    db procedure has been executed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Nov-2000 (hweho01)
**          Added LEVEL1_OPTIM (-O1) for axp_osf.  
**          With option "-O2" ("-O"), the compiler generates incorrect   
**          codes that cauase unaligned access error during ingstart/ 
**          createdb.
**          Compiler version: Compaq C V6.1-019 -- V6.1-110.
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**      04-dec-2001 (rigka01) bug#106069
**	    SIGBUG invalid address alignment in adu_2prvalue() called by 
**          scs_input() when dbdv.db_data is set to the non byte-aligned
**	    opy of the data although the code is following the ifdef
**	    BYTE_ALIGNED code path. 
**	12-dec-2001 (somsa01)
**	    Added NO_OPTIM for i64_win to prevent "createdb" from causing
**	    the DBMS server to SEGV.
**	21-Dec-01 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
**	7-mar-02 (inkdo01)
**	    Add support for sequences.
**      23-apr-2002 (rigka01) bug#106069 correction 
**	    Cast gca_value to PTR rather than i4 when setting db_data
**	    Code won't compile on rs4_us5 without this correction 
**      12-jul-2002 (stial01)
**          Error handling for force abort during scs_blob_fetch, only 
**          possible when couponifying into permament etab. (b108251) 
**      13-aug-2002 (stial01)
**          Cross integration of 433533 for b83616,b104624 with some changes
**          to the implementation of the force abort 
**      30-Sep-2002 (hanch04)
**          Added RAAT_NOT_SUPPORTED error message.  Don't let a raat 
**	    program connect to a 64-bit server.
**      24-Oct-2002 (stial01)
**          scs_set_session PSQ_SET_XA_XID, for shared xa transactions,
**          need QEF_SP_SAVEPOINT every time we re-join a transaction
**          PSQ_XA_PREPARE, COMMIT, ROLLBACK, lock xa_scb, process, unlock.
**	27-Jan-2003 (devjo01)
**	    Divorce RAAT support suppression from ADD_ON_64 define.
**      14-Mar-2003 (horda03) Bug 109224
**          If a DB procedure called from another DB procedure via a rule has to be
**          loaded into QEF memory, don't set the rowcount total to -1, otherwise the
**          rowcount returned to the FE will be 1 short.
**	18-mar-2003 (devjo01) b109860
**	    Fix memory leak seen when using row producing procedures.
**      12-May-2003 (horda03) Bug 106708
**          If a User aborts a RETRIEVE query, and it is picked up by the sequencr after
**          the first message (of a multi-part message) is sent to the user, then the
**          query isn't terminated. If the user then re-issues the same "repeated" query
**          the user is sent the data from the point where the previous query was
**      15-May-2003 (inifa01) b110263 INGSRV 2270
**          When security auditing is enabled a SIGBUS occurs in adu_prvalarg()
**          followed by E_CL2514, E_GC220B, E_CLFE07 and E_GC2205 on the execution
**          of a database procedure with a parameter of type money.
**          Problem occurs on rmx.us5 when dbdv.db_data is set to non byte-aligned
**          copy of data.
**          aborted.
**      06-jun-2003 (stial01)
**          scs_blob_fetch() convert table/owner name to correct case (b110371)
**	23-jun-2003 (somsa01)
**	    Updated i64_win such that, instead of a NO_OPTIM we just turn off
**	    using the memcpy() intrinsic.
**      24-jul-2003 (stial01)
**          scs_blob_fetch() Optimize prepared inserts having blobs (b110620)
**	31-Oct-2003 (bonro01)
**	    Unoptimize for SGI to prevent abends.
**      15-sep-2003 (stial01)
**          scs_blob_fetch() Blob optimization for prepared updates (b110923)
**      02-jan-2004 (stial01)
**          Changes for SET [NO]BLOBJOURNALING, SET [NO]BLOBLOGGING
**      19-jan-2004 (horda03) Bug 111047
**          Do not allow SET SESSION <access mode> in a Multi-Statement
**          transaction.
**	19-feb-2004 (gupsh01)
**	    Made changes for set [no]unicode_substitution.
**	1-mar-2004 (gupsh01)
**	    Fixed bad cross integration. 
**	2-Mar-2004 (schka24)
**	    Restore 2 sets of changes lost by one of the above edits.
**      15-Jan-2004 (hanal04) Bug 111417 INGSRV2628
**          Prevent X DB locks being held indefinitely by an invalid user that 
**          has left a session at the password prompt.
**	28-Jan-2004 (schka24)
**	    Treat QSF-mem-full from parser like PSF-mem-full.
**      18-feb-2004 (stial01)
**          Added scs_realloc_outbuffer for 256k row support
**      13-may-2004 (stial01)
**          Removed support for SET [NO]BLOBJOURNALING ON table.
**	31-Aug-2004 (schka24)
**	    Remove SET [NO]BLOBLOGGING statement.
**      19-nov-2004 (stial01)
**          Blob optimization for PSQ_REPCURS, not PSQ_REPLACE (b113501,b112427)
**      01-dec-2004 (stial01)
**	    More diagnostics for blob optimization (b112427, b113501)
**	09-mar-2005 (wanfr01)
**          Bug 64899, INGSRV87
**          Removed duplicate include for ex.h.
**      01-jun-2005 (stial01)
**          case PSQ_XA_ROLLBACK, init qe_ccb->qef_db_id for QET_XA_ABORT
**      19-jul-2005 (stial01)
**          Add back entry 164 to sqncr_tbl, removed by change 469243 (b114876)
**	22-Jul-2005 (thaju02)
**	    Add SET SESSION WITH ON_USER_ERROR = ROLLBACK TRANSACTION.
**	24-May-2006 (kschendel)
**	    Add DESCRIBE INPUT.  Remove obsolete/bogus 5.0 SET statements.
**      24-aug-2006 (horda03) Bug 116547
**          If a query is interrupted, release the
**          scb->scb_sscb.sscb_cquery.cur_amem.
**      16-oct-2006 (stial01)
**          scs_sequencer() check for GCA_LOCATOR_MASK in query modifier
**      06-nov-2006 (stial01)
**          scs_sequencer() remove temp (and incorrect) GCA_LOCATOR_MASK code
**      08-mar-2007 (stial01)
**          scs_sequencer() Fix PSQ_XA_PREPARE,COMMIT,ROLLBACK for tux/libq pgms
**          (except qef_call QET_XA_ROLLBACK instead of deprecated QET_XA_ABORT)
**      03-may-2007 (stial01)
**          Added scs_format_xa_xid()
**      17-may-2007 (stial01)
**          Changes for FREE LOCATOR statment (PSQ_FREELOCATOR)
**      08-jun-2007 (stial01)
**          Fixed cleanup of XA sessions after force abort or deadlock.(b118397)
**          Replaced scs_format_xa_xid with scs_xatrace.
**	25-Jun-2008 (kibro01)
**	    Added SC930 tracing controlled by ult_always_trace() blocks
**      09-sep-2008 (gefei01)
**          Support table procedure.
**      20-nov-2008 (stial01)
**          DON'T assume GCA_MAXNAME > DB_MAXNAME
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      06-Feb-2009 (gefei01)
**          Fix bug 121624: Save qmode only once before any tproc recompilation.
**      12-Feb-2009 (gefei01)
**          Bug 121651: Fix SEGV which occurs when the base table of
**          a table procedure has been dropped.
**	16-Feb-2009 (kibro01) b121674
**	    Add SC930 tracing output for repeated query execution and add
**	    data types for parameters.
**      27-Feb-2009 (gefei01)
**          Support cursor select for table procedure.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	 1-Oct-09 (gordy)
**	    New GCA message GCA2_INVPROC with data object GCA2_IP_DATA.
**	26-Mar-2010 (kschendel) SIR 123485
**	    Chop COPY handling out of sequencer mainline.  Delete i4 casts
**	    on sc0m_allocate calls, let the prototype do its job.
**	    Fix indents, update some comments.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	21-Apr-2010 (kschendel) SIR 123485
**	    Always pass the blob "workspace" info to couponify, since it
**	    might take more than one call to have enough input data to
**	    make it into DMF (who is the guy who needs the workspace info).
**	    Delete rd_lo_next/prev list, has been nothing but a very
**	    expensive no-op for a long time it seems.
**/

/*
**  Forward and/or External function references.
*/
static DB_STATUS
scs_allocate_byref_tdesc(
	SCD_SCB	  *scb,
	SCC_GCMSG **descmsg);

static DB_STATUS scs_allocate_tdata(
	SCD_SCB	  *scb,
	GCA_TD_DATA *descmsg,
	SCC_GCMSG **datamsg);

static PTR
scs_save_tdesc(
	SCD_SCB	  *scb,
	PTR	  descmsg);

static DB_STATUS
scs_returntype_check(
	SCD_SCB		*scb,
	DB_DT_ID	type);

static void scs_chkretry(
	SCD_SCB         *scb,
	DB_STATUS       status,
	DB_STATUS       *ret_val,
	i4             *cont,
	i4             **next_op,
	i4             *qef_wants_eiqp);

static DB_STATUS scs_def_curs(
	SCD_SCB		*scb,
	i4		qmode,
	DB_CURSOR_ID	*curs_id,
	PTR		handle,
	bool		retry_fetch,
	QEF_QP_CB	*qp_cb,
	SCC_GCMSG	**message,
	i4		*qry_status,
	i4		*next_op,
	bool		*retry_qry,
	DB_STATUS	*ret_val);

static DB_STATUS scs_set_session(
    SCD_SCB	*scb,
    PSQ_CB	*ps_ccb);

static DB_STATUS scs_setrole(
    SCD_SCB	*scb,
    PSQ_CB	*ps_ccb);

static DB_STATUS scs_set_priv(
    SCD_SCB	*scb,
    PSQ_CB	*ps_ccb);

static DB_STATUS scs_mxsession(
    SCD_SCB	*scb,
    PSY_CB	*psy_cb);


static DB_STATUS
scs_realloc_outbuffer(
SCD_SCB		*scb);

static DB_STATUS scs_emit_rows(
	i4 qmode,
	bool rowproc,
	SCD_SCB *scb,
	QEF_RCB *qe_ccb,
	SCC_GCMSG *message);

static void scs_cleanup_output(
	i4 qmode,
	SCD_SCB *scb,
	SCC_GCMSG *message);

static DB_STATUS scs_xa_lock_scb(
SCD_SCB		*scb, 
SCD_SCB		*lock_scb,
LK_LKID		*lkid,
i4		lock_action);

static DB_STATUS scs_xa_unlock_scb(
SCD_SCB		*scb,
SCD_SCB		*lock_scb,
LK_LKID		*lkid);

static VOID scs_xatrace(SCD_SCB *scb,
i4		     qmode,
i4		     qflags,
char		     *qstr,
DB_DIS_TRAN_ID	     *dis_tran_id,
i4    xa_stat);

GLOBALREF SC_MAIN_CB *Sc_main_cb; /* server control block */

/*}
** Name: SQNCR_TBL - Contains instructions/opcode for the sequencer
**
** Description:
**      This table contains information as to what facilities need to 
**      be called to process a given query mode.
**
** History:
**      22-Jul-1986 (fred)
**          Created on Jupiter.
**	12-Mar-1989 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Add entries for new statement modes 
**	    PSQ_[CADK]GROUP and PSQ_[CAK]APLID.
**	15-mar-89 (neil)
**	    Filled in empty holes and added rule query modes into table.
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Add new statement modes PSQ_REVOKE and PSQ_[GR]DBPRIV.
**	24-apr-89 (neil)
**	    Modified cursor processing to support rules.  Cursor DELETE is
**	    now optimized.
**      22-may-89 (fred)
**	    Fixed bug wherein the internal transaction handling was producing
**	    relatively useless error messages.  Now, SCF tells QEF to dump
**	    information to the user, following with its internal error message.
**	    Also fixed error handling when QEF refuses to close off a
**	    transaction.  Now, if QEF fails to close a transaction, SCF and PSF
**	    will continue to know about any open cursors and/or prepared
**	    queries.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Add new statement mode PSQ_MXQUERY
**	07-sep-89 (neil)
**	    Alerters: New statement modes replaced old ones (see psfparse.h).
**	08-oct-89 (ralph)
**	    For B1: Add new statement modes PSQ_[CAK]USER, PSQ_[CAK]LOC,
**		    PSQ_[CK]ALARM, PSQ_ENAUDIT, PSQ_DISAUDIT.
**      07-may-1990 (fred)
**          Added code to support large objects and other compressed QEF
**	    objects.
**	10-nov-89 (jrb)
**	    Added code to deallocate QSF memory when COPY INTO completes.  This
**	    was a bug which was causing us to run out of QSF memory.
**      07-feb-1990 (fred)
**	    Added scc_dispose() call when force abort processing takes place.
**	    This is necessary to remove any messages on the queue which may
**	    be replaced by later processing.  The problems are the fixed,
**	    preallocated messages -- most notably the response block, which is
**	    placed on the queue by normal sequencer cleanup processing.
**
**	    Lack of this fix manifested itself as a infinite loop in
**	    scc_dispose() as it tried to remove the response block over and over
**	    again after processing a force abort.
**	08-aug-1990 (Ralph)
**	    Restrict new C2 statements to C2 environments.
**	    Add new statement modes PSQ_[GR]OKPRIV to allow GRANT ON DATABASE
**	    with only UPDATE_SYSCAT and ACCESS privileges in vanilla environs.
**	    Enforce TRACE permissions.
**	10-oct-1990 (ralph)
**	    6.3->6.5 merge:
**	    21-may-90 (linda)
**		Change comments which refer to constant names.  These have been
**		changed to reflect syntax change - used to be CREATE ... AS LINK
**		and REGISTER; now it's REGISTER ... AS LINK and REGISTER ... AS
**		IMPORT.  So, PSQ_0_CRT_LINK -> PSQ_REG_LINK and PSQ_REGISTER ->
**		PSQ_REG_IMPORT.
**	    23-may-90 (linda)
**		Change constant names once again.  Want to distinguish between
**		all 3 of "create...as link", "register...as link" and "register
**		... as import".  So, use:
**		     PSQ_0_CRT_LINK
**		    *PSQ_REG_LINK
**		     PSQ_REG_IMPORT
**		*NOTE* to STAR integrators:  PSQ_REG_LINK used to be
**		PSQ_REGISTER; its name was changed to reflect its real meaning,
**		"register...as link".
**	04-feb-91 (neil)
**	    Alerters: Complete support for EVENT statements.  Cooperate with
**	    	      event queue for user notification (sce_chkstate).
**       4-feb-91 (rogerk)
**          Added support for Set Nologging.  Added PSQ_SLOGGING query mode.
**	25-feb-91 (rogerk)
**	    Moved PSQ_SLOGGING definition from 125->142 to not collide with
**	    new 6.5 query modes.
**	    Added PSQ_SON_ERROR query mode and support for Set Session with
**	    on_error statement to define transaction error handling.
**	21-may-91 (andre)
**	    add support for 3 new query modes: PSQ_SETUSER, PSQ_SETGROUP, and
**	    PSQ_SETUSER
**	4-may-1992 (bryanp)
**	    Add new query modes PSQ_DGTT and PSQ_DGTT_AS_SELECT.
**      10-dec-92 (anitap)
**          Added new query mode PSQ_CREATE_SCHEMA.
**      18-dec-92 (anitap)
**          Added query mode PSQ_CREATE_SCHEMA to the switch statement so that
**          QEF is called for CREATE SCHEMA tatement. Check if we need to
**          parse EXECUTE IMMEDIATE text and reset "retry" counter after we
**          return from QEF.
**          Added support to call PSF to parse the E.I. qtext.
**	22-dec-92 (andre)
**	    CREATE/DEFINE VIEW will follow the same path as the DML queries,
**	    i.e. after PSF parses the statement and builds PST_CREATE_VIEW
**	    statement node, we will call OPF to build a query plan which will be
**	    executed by QEF
**      05-jan-92 (rblumer)
**          added new query mode PSQ_ALTERTABLE, plus placeholder PSQ_CONS.
**          also added new query mode PSQ_CRESETDBP (create set-input proc).
**	18-jan-93 (fpang)
**	    Added new query modes PSQ_REGPROC and PSQ_DDEXECPROC for distributed
**	    register and execute database procedure.
**	06-jan-93 (anitap)
**	    Fixed bugs for CREATE SCHEMA project in scs_sequencer().
**	    Added check for qef_wants_eiqp so that QEF_RCB fields are not 
**	    reset.
**	09-mar-93 (rblumer)
**	    Added PSQ_ALTERTABLE to QEF section.
**	30-mar-93 (rblumer)
**	    Swapped PSQ_CREATE and PSQ_DISTCREATE in sqncr_tbl, as their values
**	    have been swapped in psfparse.h (for easier error handling).
**	06-apr-93 (anitap)
**	    Error handling for CREATE SCHEMA in case of rule, procedure, index
**	    or GRANT. We do not want to call QEQ_QUERY to continue exeimm 
**	    processing in case of any parsing errors.
**	11-may-93 (anitap)
**	    Added support for "set update_rowcount" statement.
**	23-jun-93 (anitap)
**	    Beautify SCQNCR_TBL. Added the query mode number next to the defines
**	    so that it is obvious that the statement modes should be added in
**	    the order of their numbers.
**	    Changed qef_wants_qp to qef_wants_ruleqp to distinguish it from
**	    qef_wants_eiqp.	
**	    Removed Eric's FIXME note.
**	    Changed QEF_SAVED_RSRC to QEF_DBPROC_QP in scs_sequencer().
**	    We will not assign the E.I. query text handle to sscb_thandle as we
**	    were stamping on the handle to the query text of CREATE SCHEMA/
**	    CREATE TABLE with constraints/CREATE VIEW(WCO). This led to the
**	    following error in the error log: "E_QS001E_ORPHANED_OBJ An 
**	    orphaned Query Text object was found and destroyed during QSF 
**	    session exit."
**	2-Jul-1993 (daveb)
**	    prototyped.
**	08-oct-93 (anitap)
**	    Support for "SET UPDATE_ROWCOUNT" statement for STAR. Do not call
**	    QEF. PSF will call QEF which in turn will call RQF.
**	01-nov-93 (anitap)
**	    If qmode = PSQ_DGTT_AS_SELECT, set qef_stmt_flag in QEF_RCB so that
**	    QEF does not create implict schema for "declare global temporary
**	    table as..."
**      29-dec-93 (anitap)
**          Fixed bug 57815. The query trees for indexes, rules and permits
**          were not being destroyed for CREATE SCHEMA statements with CREATE
**          TABLE with UNIQUE CONSTRAINT, CHECK constraint, REFERENTIAL
**          constraint, GRANT statement. This bug occured only if any of the
**          above constraints appeared in pairs or with the GRANT statement.
**          On return from scs_chkretry(), we were continuing with parsing
**          of the next execute immediate statement before destroying the
**          query tree of the previous execute immediate statement. The bug
**          gave the following errors:
**          "E_QS001E_ORPHANED_OBJ     An orphaned Query Plan object was
**          found and destroyed during QSF session exit."
**          "E_QS0014_EXLOCK   QSF Object is already locked exclusively."
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**          This results in owner within sc0m_allocate having to be PTR.
**          In addition calls to sc0m_allocate within this module had the
**          3rd and 4th parameters transposed, this has been remedied.
**	3-jun-94 (robf)
**          Added missing PSQ_XXX definitions to sqncr_tbl. This was causing
**	    GRANT ROLE on HP to fail.
**	26-Jun-2006 (jonj)
**	    This is like the most stupid thing of all the stupid things
**	    in the sequencer; its only reference is when
**	    if (    (ps_ccb->psq_mode != PSQ_QRYDEFED) 
**	    but I continued the stupidity by adding
**          PSQ_GCA_XA_START
**          PSQ_GCA_XA_END
**          PSQ_GCA_XA_PREPARE
**          PSQ_GCA_XA_COMMIT
**          PSQ_GCA_XA_ROLLBACK
**	    for XA integration.
**	01-Aug-2006 (jonj)
**	    Better handling of XA errors, incorporate Gordy's
**	    GCA_XA_ERROR_MASK flag in qry_status.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Make this table ever so slightly less stupid by incorporating
**	    the PSY call op-code, so we can table drive all the bazillions
**	    of little psy-call's.
**	15-apr-2010 (dougi) BUG 121220
**	    Add "psitional parm" flag to QEF structures for nameless proc parms.
**	19-Apr-2009 (gupsh01) SIR 123444
**	    Added support for rename table/column.
**	21-Jul-2010 (kschendel) SIR 124104
**	    Add (stub) set create_compression.
*/
typedef struct _SQNCR_TBL
{
    i4		sq_mask;	    /*
				    ** A mask of bits which tell what actions
				    ** are needed
				    */
#define                 SQ_PSF_MASK	0x1
#define                 SQ_OPF_MASK	0x2
#define			SQ_PSY_MASK	0x4	/* Quick PSY call */

    i4		sq_psy_opcode;	/* psy_call opcode if SQ_PSY_MASK */
} SQNCR_TBL;

/*
**  Definition of static variables and forward static functions.
*/

static  const SQNCR_TBL   sqncr_tbl[] = {
	{0,0},
/* PSQ_RETRIEVE */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_RETINTO  */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_APPEND   */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_REPLACE  */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_DELETE   */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_COPY     */		{SQ_PSF_MASK, 0},
/* PSQ_DISTCREATE*/		{SQ_PSF_MASK, 0},
/* PSQ_DESTROY  */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_KVIEW},
/* PSQ_HELP     */		{SQ_PSF_MASK, 0},
/* 10, PSQ_INDEX */		{SQ_PSF_MASK, 0},
/* PSQ_MODIFY	*/		{SQ_PSF_MASK, 0},
/* PSQ_PRINT    */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_RANGE    */		{SQ_PSF_MASK, 0},
/* PSQ_SAVE     */		{SQ_PSF_MASK, 0},
/* PSQ_DEFCURS  */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_QRYDEFED */		{SQ_PSF_MASK, 0},
/* PSQ_VIEW     */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_ABSAVE   */		{SQ_PSF_MASK, 0},
/* PSQ_PROT     */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DPERMIT},
/* 20, PSQ_INTEG */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DINTEG},
/* PSQ_CLSCURS  */		{SQ_PSF_MASK, 0},
/* PSQ_RELOCATE */		{SQ_PSF_MASK, 0},
/* PSQ_DEFQRY   */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_EXECQRY  */		{SQ_PSF_MASK, 0},
/* PSQ_DEFREF   */		{0,0},
/* PSQ_BGNTRANS */		{SQ_PSF_MASK, 0},
/* PSQ_ENDTRANS */		{SQ_PSF_MASK, 0},
/* PSQ_SVEPOINT */		{SQ_PSF_MASK, 0},
/* PSQ_ABORT    */		{SQ_PSF_MASK, 0},
/* 30, PSQ_DSTINTEG */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_KINTEG},
/* PSQ_DSTPERM	*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_KPERMIT},
/* PSQ_DSTREF	*/		{SQ_PSF_MASK, 0},
/* PSQ_EVENT	*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DEVENT},
/* PSQ_EXCURS	*/		{SQ_PSF_MASK, 0},
/* PSQ_CALLPROC	*/		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_EVDROP	*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_KEVENT},
/* PSQ_EVREGISTER*/		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_EVDEREG	*/		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_EVRAISE	*/		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* 40, PSQ_REPCURS */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_RETCURS	*/		{SQ_PSF_MASK, 0},
/* PSQ_DESCINPUT */		{SQ_PSF_MASK, 0},
/* PSQ_REPDYN	*/		{SQ_PSF_MASK, 0},
/* PSQ_SCACHEDYN*/		{SQ_PSF_MASK, 0},
/* PSQ_SNOCACHEDYN*/		{SQ_PSF_MASK, 0},
/* PSQ_SAGGR	*/		{SQ_PSF_MASK, 0},
/* PSQ_SCPUFACT	*/		{SQ_PSF_MASK, 0},
/* PSQ_SDATEFMT	*/		{SQ_PSF_MASK, 0},
/* PSQ_SSCACHEDYN*/		{SQ_PSF_MASK, 0},
/* 50, PSQ_SDECIMAL */		{SQ_PSF_MASK, 0},
/* PSQ_SWORKLOC	*/		{SQ_PSF_MASK, 0},
/* 52	*/			{0,0},
/* PSQ_SJOURNAL */		{SQ_PSF_MASK, 0},
/* PSQ_SMNFMT	*/		{SQ_PSF_MASK, 0},
/* PSQ_SMNPREC	*/		{SQ_PSF_MASK, 0},
/* PSQ_SCARDCHK */		{SQ_PSF_MASK, 0},
/* PSQ_SNOCARDCHK */		{SQ_PSF_MASK, 0},
/* PSQ_SRINTO	*/		{SQ_PSF_MASK, 0},
/* 59	*/			{0,0},
/* 60, PSQ_SSQL	*/		{SQ_PSF_MASK, 0},
/* PSQ_SSTATS	*/		{SQ_PSF_MASK, 0},
/* PSQ_SLOCKMODE*/		{SQ_PSF_MASK, 0},
/* PSQ_DMFTRACE	*/		{SQ_PSF_MASK, 0},
/* PSQ_PSFTRACE	*/		{SQ_PSF_MASK, 0},
/* PSQ_SCFTRACE	*/		{SQ_PSF_MASK, 0},
/* PSQ_RDFTRACE	*/		{SQ_PSF_MASK, 0},
/* PSQ_QEFTRACE	*/		{SQ_PSF_MASK, 0},
/* PSQ_OPFTRACE	*/		{SQ_PSF_MASK, 0},
/* PSQ_QSFTRACE	*/		{SQ_PSF_MASK, 0},
/* 70, PSQ_ADFTRACE */		{SQ_PSF_MASK, 0},
/* PSQ_STRACE	*/		{SQ_PSF_MASK, 0},
/* PSQ_DELCURS	*/		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_SQEP	*/		{SQ_PSF_MASK, 0},
/* PSQ_COMMIT	*/		{SQ_PSF_MASK, 0},
/* PSQ_DESCRIBE	*/		{SQ_PSF_MASK, 0},
/* PSQ_PREPARE	*/		{SQ_PSF_MASK, 0},
/* PSQ_INPREPARE*/		{SQ_PSF_MASK, 0},
/* PSQ_ROLLBACK */		{SQ_PSF_MASK, 0},
/* PSQ_AUTOCOMMIT*/		{SQ_PSF_MASK, 0},
/* 80, PSQ_RLSAVE */		{SQ_PSF_MASK, 0},
/* PSQ_OBSOLETE */		{SQ_PSF_MASK, 0},
/* PSQ_JTIMEOUT */		{SQ_PSF_MASK, 0},
/* PSQ_DEFLOC   */		{SQ_PSF_MASK, 0},
/* PSQ_GRANT */			{SQ_PSF_MASK | SQ_PSY_MASK, PSY_GRANT},
/* PSQ_ALLJOURNAL*/		{SQ_PSF_MASK, 0},
/* PSQ_50DEFQRY */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_CREDBP */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_DRODBP */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DRODBP},
/* PSQ_EXEDBP */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* 90, PSQ_IF */		{SQ_PSF_MASK, 0},
/* PSQ_RETURN */		{SQ_PSF_MASK, 0},
/* PSQ_MESSAGE */		{SQ_PSF_MASK, 0},
/* PSQ_VAR  */			{SQ_PSF_MASK, 0},
/* PSQ_WHILE  */		{SQ_PSF_MASK, 0},
/* PSQ_ASSIGN  */		{SQ_PSF_MASK, 0},
/* PSQ_DIRCON  */		{SQ_PSF_MASK, 0},
/* PSQ_DIRDISCON  */		{0,0},
/* PSQ_DIREXEC  */		{SQ_PSF_MASK, 0},
/* PSQ_REMOVE  */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_KVIEW},
/* 100, PSQ_0_CRT_LINK*/	{SQ_PSF_MASK, 0},
/* PSQ_REG_LINK */		{SQ_PSF_MASK, 0},
/* PSQ_SECURE  */		{SQ_PSF_MASK, 0},
/* PSQ_REG_IMPORT*/		{SQ_PSF_MASK, 0},
/* PSQ_REG_INDEX*/		{SQ_PSF_MASK, 0},
/* PSQ_REG_REMOVE*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_KVIEW},
/* PSQ_DDCOPY  */		{SQ_PSF_MASK, 0},
/* PSQ_REREGISTER */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_REREGISTER},
/* PSQ_DDLCONCUR  */		{0,0},
/* 109  */			{0,0},
/* 110, PSQ_RULE */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DRULE},
/* PSQ_DSTRULE */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_KRULE},
/* PSQ_RS_ERROR */		{SQ_PSF_MASK, 0},
/* PSQ_CGROUP*/			{SQ_PSF_MASK | SQ_PSY_MASK, PSY_GROUP},
/* PSQ_AGROUP*/			{SQ_PSF_MASK | SQ_PSY_MASK, PSY_GROUP},
/* PSQ_DGROUP*/			{SQ_PSF_MASK | SQ_PSY_MASK, PSY_GROUP},
/* PSQ_KGROUP*/			{SQ_PSF_MASK | SQ_PSY_MASK, PSY_GROUP},
/* PSQ_CAPLID*/			{SQ_PSF_MASK | SQ_PSY_MASK, PSY_APLID},
/* PSQ_AAPLID*/			{SQ_PSF_MASK | SQ_PSY_MASK, PSY_APLID},
/* PSQ_KAPLID*/			{SQ_PSF_MASK | SQ_PSY_MASK, PSY_APLID},
/* 120, PSQ_REVOKE */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_REVOKE},
/* PSQ_GDBPRIV*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DBPRIV},
/* PSQ_RDBPRIV*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DBPRIV},
/* PSQ_IPROC  */		{0,0},
/* PSQ_MXQUERY */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_MXQUERY},
/* PSQ_CUSER   */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_USER},
/* PSQ_AUSER   */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_USER},
/* PSQ_KUSER   */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_USER},
/* PSQ_CLOC    */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_LOC},
/* PSQ_ALOC    */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_LOC},
/* 130, PSQ_KLOC */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_LOC},
/* PSQ_CALARM  */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_ALARM},
/* PSQ_KALARM  */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_ALARM},
/* PSQ_ENAUDIT */		{SQ_PSF_MASK, PSY_AUDIT},
/* PSQ_DISAUDIT*/		{SQ_PSF_MASK, PSY_AUDIT},
/* PSQ_COMMENT */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_COMMENT},
/* PSQ_GOKPRIV */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DBPRIV},
/* PSQ_ROKPRIV */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DBPRIV},
/* PSQ_CSYNONYM */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_CSYNONYM},
/* PSQ_DSYNONYM */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DSYNONYM},
/* 140, PSQ_SET_SESS_AUTH_ID */	{SQ_PSF_MASK, 0},
/* PSQ_ENDLOOP  */		{0,0},
/* PSQ_SLOGGING */		{SQ_PSF_MASK, 0},
/* PSQ_SON_ERROR */		{SQ_PSF_MASK, 0},
/* PSQ_UPD_ROWCNT */		{SQ_PSF_MASK, 0},
/* 145 */			{0,0},
/* PSQ_DGTT	*/		{SQ_PSF_MASK, 0},
/* PSQ_DGTT_AS_SELECT */	{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_GWF_TRACE */		{SQ_PSF_MASK, 0},
/* PSQ_CONS (placeholder) */	{0,0},
/* 150 PSQ_ALTERTABLE */	{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_CREATE_SCHEMA */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_REGPROC */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_DDEXECPROC*/		{SQ_PSF_MASK, 0},
/* PSQ_CRESETDBP */		{SQ_PSF_MASK, 0},
/* PSQ_CREATE   */		{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_DROP_SCHEMA*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DSCHEMA},
/* PSQ_SXFTRACE*/		{SQ_PSF_MASK, 0},
/* PSQ_SET_SESSION*/		{SQ_PSF_MASK, 0},
/* PSQ_ALTAUDIT*/		{SQ_PSF_MASK, PSY_AUDIT},
/* 160 PSQ_CPROFILE*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_USER},
/* PSQ_DPROFILE*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_USER},
/* PSQ_APROFILE*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_USER},
/* PSQ_MXSESSION*/		{SQ_PSF_MASK, 0},
/* 164 */			{0,0},
/* PSQ_SETROLE*/		{SQ_PSF_MASK, 0},
/* PSQ_RGRANT*/			{SQ_PSF_MASK | SQ_PSY_MASK, PSY_RPRIV},
/* PSQ_RREVOKE*/		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_RPRIV},
/* PSQ_STRANSACTION */		{SQ_PSF_MASK, 0},
/* PSQ_XA_ROLLBACK   */		{SQ_PSF_MASK, 0},
/* 170 PSQ_XA_COMMIT */		{SQ_PSF_MASK, 0},
/* PSQ_XA_PREPARE   */		{SQ_PSF_MASK, 0},
/* PSQ_ATBL_ADD_COLUMN */	{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_ATBL_DROP_COLUMN */	{SQ_PSF_MASK, 0},
/* PSQ_REPEAT placeholder */	{0,0},
/* PSQ_FOR placeholder */	{0,0},
/* PSQ_RETROW placeholder */	{0,0},
/* PSQ_SETRANDOM_SEED */	{SQ_PSF_MASK, 0},
/* PSQ_ALTERDB */		{SQ_PSF_MASK, 0},
/* PSQ_CSEQUENCE */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_CSEQUENCE},
/* 180 PSQ_ASEQUENCE */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_ASEQUENCE},
/* PSQ_DSEQUENCE */		{SQ_PSF_MASK | SQ_PSY_MASK, PSY_DSEQUENCE},
/* PSQ_ATBL_ALTER_COLUMN */	{0,0},
/* PSQ_SETUNICODESUB */		{SQ_PSF_MASK, 0},
/* PSQ_GCA_XA_START */		{0,0},
/* PSQ_GCA_XA_END */		{0,0},
/* PSQ_GCA_XA_PREPARE */	{0,0},
/* PSQ_GCA_XA_COMMIT */		{0,0},
/* PSQ_GCA_XA_ROLLBACK */	{0,0},
/* PSQ_FREELOCATOR */		{SQ_PSF_MASK, 0},
/* PSQ_ATBL_RENAME_COLUMN */	{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_ATBL_RENAME_TABLE */	{SQ_PSF_MASK | SQ_OPF_MASK, 0},
/* PSQ_CREATECOMP */		{SQ_PSF_MASK, 0},
	{0,0}
};
static char execproc_syntax[] = "execute procedure ";
static char execproc_syntax2[] = " = session.";

/*
** {
** Name: scs_sequencer	- the sequencer for the INGRES DBMS
**
** Description:
**      This routine performs that actions necessary to orchestrate the 
**      actions of the remaining facilities to get queries accomplished. 
**      The routine is driven by the communication blocks it receives  
**      from the dispatcher.  When communication blocks come in from 
**      the front ends, the routine figures out (possibly with some aid 
**      from other facilities such as PSF) what the appropriate action 
**      is.  Once determined, the routine passes control to other facilities 
**      as appropriate to accomplish the appropriate action.
**
**	The sequencer is insanely long and complicated, but its
**	general structure can be roughly described as:
**
**	Use opcode and state to check for setup / special situations
**	(such as session startup / termination);
**	retry loop {
**	    more setup;
**	    parse the statement;
**	    switch on statement type (qmode) {
**		case PSQ_xxx: do whatever the statement wants,
**		typically optimize/qef, or psy call, or something special;
**		if retry needed, continue;
**	    }
**	}
**	Check for error / interrupt, clean up after statement;
**
**	Many of the complications arise because Ingres can't be
**	fully recursive.  Meaning, that there are times where QEF (say)
**	wants to go off and parse and optimize something such as a
**	rule DB procedure.  QEF can't recursively invoke the
**	sequencer, partly for stack limit reasons and partly because
**	it's just not done that way.  So, when QEF or any other
**	called facility needs recursive service, it has to set
**	various magic flags and return error codes telling the
**	sequencer what it's supposed to do.  The sequencer then has
**	to maintain the existing query state while it does its thing.
**
**	The sequencer will always set *next_op to tell CS whether to
**	get more input (CS_INPUT), send queued message(s) and get
**	more input (CS_EXCHANGE), or send queued message(s) and then
**	come back into the sequencer (CS_OUTPUT).  There are also
**	special settings such as CS_TERMINATE.
**
** Inputs:
**	op_code				what sort of operation is expected.
**      scb                             session control block for this session.
**
** Outputs:
**	next_op				what operation is expected next
**	scb				State within sequencer may be altered.
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
**      10-Jul-1986 (fred)
**          Created on Jupiter.
**      23-Nov-1986 (fred)
**          Added compatibility with CS\CLF module in preparation
**          for going multi-user.
**      03-Apr-87 (fred))
**          Added name capability.
**      28-May-87 (fred)
**          Altered flow of sequencer so that descriptor (and cursor names) are
**          not sent back to the FE until it is known that the query is "doable"
**	    (no retries necessary).  Fixes problems where wrong names for things
**          are used in the FE (since the FE can't/doesn't obey the
**	    communication protocol and pay attention to things such as block
**	    types.
**
**          Also added support for cursors with target lists longer than the
**          communication block size.
**
**          Added code to detect when a retrieve is already completed, and
**          not interrupt it in this case.  Removes spurious
**	    E_QE0008_CURSOR_NOT_OPENED messages.
**      08-Jul-1987 (fred)
**          Added GCA/GCF Support.
**	21-Jul-1987 (rogerk)
**	    Set qe_ccb->qef_qacc field to QEF_UPDATE before calls to qeq_query
**	    or qeq_open.  This field is expected to be set.
**	15-Jun-1988 (rogerk)
**	    Call scs_dbms_task for sessions which are SCS_SPECIAL state to
**	    process dedicated task.
**	    In session terminate state, don't do GCA coordination if NOGCA
**	    thread.
**	    Return fatal error if SCS_VITAL type session is terminated
**	    and the server is not in shutdown state.
**      10-Mar-1989 (ac)
**          Added 2PC support.
**	12-Mar-1989 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Add support for statement modes PSQ_[CADK]GROUP, PSQ_[CAK]APLID.
**	15-mar-89 (neil)
**	    Added support to dispatch rule query modes and to handle
**	    procedure recreation during rule processing.
**      23-Mar-1989 (fred)
**	    Alter DBMS task handling to return fatal error only if VITAL task is
**	    terminated AND the task is supposed to be permanent.  The server
**	    startup task is VITAL, but is expected to terminate.
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Add new statement modes PSQ_REVOKE and PSQ_[GR]DBPRIV.
**	24-apr-89 (neil)
**	    Modified cursor processing to support rules.  Cursor DELETE and
**	    UPDATE now behave more like regular queries.  Both are sent to PSF
**	    (PSQ_DELCURS is a special case as there's no text), then OPF
**	    and then through the same code as QEQ_QUERY (to pick up rules).
**	23-may-89 (jrb)
**	    Changed ule_format calls for new interface.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Add new statement mode PSQ_MXQUERY
**	22-jun-89 (jrb)
**	    Added checks for proper capability licensing on certain terminator
**	    phase I features.
**	10-aug-89 (jrb)
**	    Added check for protocol level for gca error returns to see if we
**	    should be sending generic errors or not.
**	19-sep-89 (anton)
**	    UNIX copy bug fix - QSO object was destroyed too early in some
**	    cases due to a logic error in an IF.
**	08-oct-1989 (ralph)
**	    For B1: Add support for new statement modes PSQ_[CAK]USER,
**		    PSQ_[CAK]LOC, PSQ_[CK]ALARM, PSQ_ENAUDIT, PSQ_DISAUDIT.
**	10-nov-89 (jrb)
**	    Added code to deallocate QSF memory when COPY INTO completes.  This
**	    was a bug which was causing us to run out of QSF memory.
**	08-dec-89 (sandyh)
**	    Added FE notification on force abort during COPY_FROM, bug 9030.
**	21-feb-90 (carol)
**	    Support for new query modes for COMMENT ON statement.
**	08-aug-90 (Ralph)
**	    Restrict new C2 statements to C2 environments.
**	    Add new statement modes PSQ_[GR]OKPRIV to allow GRANT ON DATABASE
**	    with only UPDATE_SYSCAT and ACCESS privileges in vanilla environs.
**	    Enforce TRACE permissions.
**	    Retry without flattening when OPF returns E_OP000A_RETRY
**	10-oct-1990 (ralph)
**	    6.3->6.5 merge:
**	    01-may-1990 (neil)
**		Cursor Performance I: Support read-only cursor pre-fetch.
**	11-dec-1990 (ralph)
**	    Don't translate op_code to CS_OUTPUT if it's CS_TERMINATE (b33024)
**	04-feb-91 (neil)
**	    Alerters: Complete support for EVENT statements.  Cooperate with
**	    	      event queue for user notification (sce_chkstate).
**       4-feb-91 (rogerk)
**          Added support for Set Nologging.  Added PSQ_SLOGGING query moe.
**          When this statement type is received, check for open transactins
**          and call DMC_ALTER to disable/enable logging.  Put the processig
**          for this new query mode in with PSQ_SETLOCKMODE processing.
**      12-feb-1991 (mikem)
**          Added support to scs_seqencer() for returning logical key values
**          the GCA response block to the FE client. This is added so that the
**          client can query the system maintained logical key automatically
**          assigned by the system a result of the client's insert.
**      25-feb-91 (rogerk)
**          Added support for Set Session.  Added PSQ_SON_ERROR query mode.
**          This mode will generate a call to qec_alter to define the
**          statement error handling actions.
**          Changed ERUSF message added for Set Nologging to be an ERSCF
**          message: E_US1276 -> E_SC031B_SETLG_XCT_INPROG.
**      25-feb-91 (rogerk)
**          Fixed code which checks return status from qec_info call in
**          Set Lockmode statements.  It used to not break on erorrs, but
**          would continue with set lockmode processing.  Changed to break
**          with GCA_FAIL_MASK qry_status, but with OK ret_val so it won't
**          terminate the thread.
**	25-mar-91 (mikem)
**	    Moved initialization of the qef_val_logkey field of the qef
**	    control block so that it was initialized to 0 in all queries
**	    (not just inserts).  Also removed some logkey debugging code now
**	    that libq supports printing the returned logical key as part of
**      22-apr-91 (rogerk)
**          Added check for E_DM0147_NOLG_BACKUP_ERROR return from dmc_alter
**          call to set NoLogging state.  This error should be formatted
**          instead of a SC0206_CANNOT_PROCESS.
**      10-May-1991 (fred)
**          Altered interrupt processing to set state to from negative to
**          positive if important.  This is in line with scs_i_interpret() fix
**          not to set scb_sscb.sscb_state during interrupts.
**	21-may-91 (andre)
**	    add support for 3 new query modes: PSQ_SETUSER, PSQ_SETGROUP, and
**	    PSQ_SETUSER
**	21-may-91 (andre)
**	    renamed some fields in SCS_ICS to make it easier to determine by the
**	    name whether the field contains info pertaining to the system (also
**	    known as "Real"), Session ("he who invoked us"), or the Effective
**	    identifier:
**		ics_ustat      -->  ics_rustat	    real user status bits
**		ics_username   -->  ics_eusername   effective user id
**	15-jul-91 (andre)
**	    audit success of SET USER/GROUP/ROLE + disallow SET GROUP/ROLE if
**	    the user does not have Knowledge Management Extension
**	12-feb-92 (nancy)
**	    Fix bug 38565.  
**	    In scs_sequencer() check for qef return status, E_QE0301_TBL_SIZE_
**	    CHANGE where E_QE0023 is checked.  Set retry counter to 0 so that
**	    this condition cannot cause a db_reorg error.
**	30-apr-1992 (bryanp)
**	    Add handling for E_PS0009_DEADLOCK return from parser.
**	4-may-1992 (bryanp)
**	    Add new query modes PSQ_DGTT and PSQ_DGTT_AS_SELECT.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              19-Jan-1989 (alexh)
**                  Added PSQ_DDCOPY - STAR copy mode
**              28-mar-1990 (georgeg)
**                  remove saving the  parameters  of the query and pasing 
**		    them to QEF
**              27-may-1990 (georgeg)
**                  do not need to pass the size of the tuple descriptor to 
**		    RQF, RQF will copy in STAR's tuple descriptor  the collumn 
**		    data-types from the local and it will leave the collumn 
**		    names alone.
**              10-May-1991 (fred)
**                  Altered interrupt processing to set state to from negative 
**		    to positive if important.  This is in line with 
**		    scs_i_interpret() fix *not to set scb_sscb.sscb_state 
**		    during interrupts.
**          End of STAR comments.
**	02-aug-92 (andre)
**	    if qmode==PSQ_GRANT call psy_call() with opcode PSY_GRANT
**	    (psy_call() will call psy_dgrant() directly instead of going through
**	    psy_dperm());
**
**	    if qmode==PSQ_REVOKE call psy_call() with opcode PSY_REVOKE
**	    (psy_call() will invoke psy_revoke() to perform revocation of
**	    specified privileges
**	17-aug-1992 (rog)
**	    QP's might not be destroyed if a COPY FROM had to be retried, or
**	    there were errors during the COPY.  (bug 43905)
**	22-jul-1992 (fpang)
**	    Support for new RDF cache flushing protocol, which eliminates
**	    PSQ_DELRNG, but instead lets PSF know that the sequencer
**	    is retrying by setting the PSQ_REDO bit in psq_flag.
**	04-sep-1992 (fpang)
**	    The distributed server only supports gca_protocol_40,
**	    so negociate the FE down to 40 if it is higher. Also fixed interrupt
**	    handling in distributed 'direct connect'.
**	03-nov-92 (robf)
**	    For ENABLE/DISABLE SECURITY AUDIT pass in an sxf_object of
**	    "enable","disable" to prevent possible AV in SXF.
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**	19-nov-1992 (fpang)
**	    Distributed. Make sure qef_intstate is 0 when doing an LDB TDESCR 
**	    pre-fetches.
**	14-dec-92 (jhahn)
**	    Added more support for byrefs. Also added tracepoint SC912
**	    (to turn off byref protocol.
**	27-nov-92 (robf)
**	    Modularize the security auditing in SCF.
**	27-oct-1992 (fpang)
**	    Distributed. If errors occur before rows are returned, remove the
**	    tuple descriptor from the queue.
**	29-oct-92 (andre)
**	    if running at level 60 or higher, build error message containing
**	    encoded SQLSTATE +
**	    interface of ule_format() has changed so that it returns SQLSTATE
**	    status code instead of generic error message
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**	19-nov-1992 (fpang)
**	    Distributed. Make sure qef_intstate is 0 when doing an LDB TDESCR 
**	    pre-fetches.
**	14-dec-92 (jhahn)
**	    Added more support for byrefs. Also added tracepoint SC912
**	    (to turn off byref protocol.
**	13-jan-93 (robf)
**	    Add ENABLE/DISABLE SECURITY_AUDIT and CREATE/DROP SECURITY_ALARM
**	    to list of statement which require KME.
**      13-jan-93 (mikem)
**          Lint directed changes. Fixed 2 calls to scs_cpsave_err() to pass
**          the correct number of arguments (2).  The third argument was always
**          ignored by the routine.
**      18-Jan-1993 (fred)
**          Added code to fetch setup to set the cur_rdesc.rd_modifier's
**          field.  This is then used to set the singleton variable.
**          This is necessary so that singleton fetch's involving
**          large objects will not be marked singleton's.  By not
**          marking them singleton's inside, we allow the usual
**          mechanism for tuple continuation to function unchanged.
**	05-jan-93 (rblumer)
**	    when calling parser with RECREATE,
**	    copy the set-input table and proc ids from QEF cb to PSQ cb.
**	18-jan-93 (fpang)
**	    Added code to support distributed register and execute database
**	    procedures. New querymodes PSQ_REGPROC and PSQ_DDEXECPROC correspond
**	    to distributed register database procedure and execute database
**	    procedure.
**	05-feb-93 (andre)
**	    use QEC_ALTER rather than the obsolete QEC_RESET_EFF_USER_ID to
**	    inform QEF of a new effective user id
**	10-feb-93 (rblumer)
**	    Added PSQ_DISTCREATE query mode, so distributed CREATE TABLES are
**	    sent through the QEF DBU processing instead of OPF processing.
**  	19-Feb-1993 (fred)
**  	    Added support for large objects in COPY.  This support
**  	    entails having the tuple assembled and disassembled from
**  	    its communication state (with the objects embedded) to its
**  	    database state (where the objects are replaced by coupons).
**	20-feb-93 (jhahn)
**	    Removed initialization of qef_pflag
**	22-feb-93 (rblumer)
**	    after calling psy_credbp for set-input proc, free the memory for
**	    the query tree object.
**	    Added PSQ_ALTERTABLE to QEF section.
**	16-mar-93 (ralph)
**	    DELIM_IDENT: Change interface to PSQ_ALTER to pass case
**	    translation semantics and revised username.
**	20-apr-93 (jhahn)
**	    Added code to check to see if transaction statements should be
**	    allowed.
**	06-apr-93 (ralph)
**	    DELIM_IDENT: Don't pass case translation semantics to PSQ_ALTER
**	27-may-93 (andre)
**	    when processing CREATE PROCEDURE, we don't want to invoke
**	    cui_idxlate() on dbproc name supplied by PSF - PSF has already done
**	    it and doing it again serves no purpose
**	08-jun-1993 (jhahn)
**	    Added initialization of qe_cb->qef_no_xact_stmt.
**      28-jun-93 (tam)
**	    Added support for SET SESSION AUTHORIZATION statement (query mode
**	    == PSQ_SET_SESS_AUTH_ID) in star. Note that star will only change 
**	    the user authorization id of the star level control blocks (in psf 
**	    and qef).  This is deemed sufficient to handle the need for 
**	    unloaddb/reloaddb, which is the requirement for this change.  The 
**	    user id for the local connections for this star session is not 
**	    changed.  To reset those local authorizations, the user has to do 
**	    it through DIRECT CONNECT or DIRECT EXECUTE IMMEDIATE. The general 
**	    issue of star/local access control will be addressed subsequent 
**	    to 6.5.  Leveraged on existing code for local except the dmf call to
**          reset the dmf_cb's user id is bypassed.  Star has no dmf.
**	2-Jul-1993 (daveb)
**	    prototyped, use propoer PTR for allocation
**	10-jul-93 (andre)
**	    cast the third param to psy_call() as (PSF_SESSCB_PTR) to match
**	    prototype declaration in PSFPARSE.H
**	26-jul-1993 (jhahn)
**	    Moved coercing of returned byref procedure parameters from the
**	    back end to the front end.
**	29-jul-93 (andre)
**	    do not start a READONLY transaction before calling PSF to recreate a
**	    dbproc.  If the dbproc being recreated or some dbproc invoked by it
**	    was marked dormant, dependency information will be entered into
**	    IIDBDEPENDS and IIPRIV + IIPROCEDURE will be updated to reflect
**	    change in the status of dbproc(s) found to no longer be dormant
**      12-Aug-1993 (fred)
**          Set up to remove large object temps at end of query.
**	08-sep-93 (swm)
**	    Changed sid type from i4 to CS_SID to match recent CL
**	    interface revision.
**	4-Aug-1993 (daveb)
**	    Correct some arithmetic on PTRs by casting to char * first.
**	16-Sep-1993 (daveb)
**	    Changes to support 'drop CONNECTION' iimonitor command and
**	    MO objects.	     
**	13-Oct-1993 (fpang)
**	    Star. Don't call opf for PSQ_SRINTO (set result structure).
**	    Fixes B55675, 'set result structure' results in internal error.
**       8-Oct-1993 (fred)
**          Fixed handling of sscb_dmm setting after an interrupt.
**          With large object handling, this was sometimes not
**          cleared.  We now clear it as we prepare to send the
**          interrupt ack message.
**	18-Oct-1993 (jnash)
**	    Fix AV encountered during COPY force-abort processing.
**	26-oct-93 (robf)
**          Reset is_idle flag on any state other than SCS_INPUT.
**      29-Oct-1993 (fred)
**          Add resetting of STAR tuple descriptor information when
**          performing a cursor fetch.  This was being left out, but
**          pointing into left field.  The low level rqf routines now
**          need access to the tuple descriptor when reading data for
**          the case where blobs are present.
**       3-Nov-1993 (fred)
**          Altered the STAR rqf tuple descriptor storage.  These are
**          now stored in the session control block instead of on the
**          stack.  It is a long term (query lived) object that
**          shouldn't be stored as an automatic variable.   This is
**          correct fix for above.
**	5-nov-93 (robf)
**          Add handling for password prompting, new SCS_SPASSWORD and
**	    SCS_RPASSWORD states.
**	16-nov-93 (rickh)
**	    When unpacking repeat query parameters, we could estimate too
**	    few parameters.  This would cause us to allocate too few
**	    parameter pointers, which in turn would allow us to trash
**	    SCF's free list of memory chunks.  And that brought down the
**	    server.  Fixed this.  This closes bug 55021.
**	14-jan-94 (andre)
**	    before calling psq_recreate() to recreate a non-set-input dbproc,
**	    it is a good idea to ensure that ps_ccb->psq_set_input_tabid and
**	    ps_ccb->psq_set_procid are zeroed out since psq_recreate() uses
**	    ps_ccb->psq_set_input_tabid to determine whether we are processing 
**	    a set-input dbproc
**	24-jan-1994 (gmanning)
**	    Change references from CS_SID to CS_SCB and CSget_scb.
**	17-mar-94 (andre)
**	    made changes to support repeat cursors:
**	17-mar-94 (anitap)
**	    Fixed bug in scs_def_curs() for repeat cursors project. 
**	    A repeat cursor query with host variables hung
**	    while opening the repeat cursor. SCF was not passing the number of
**	    parameters to QEF. Thus QEF could not process execute opening
**	    of repeat cursor.
**	17-mar-94 (andre)
**	    Additional changes are required to handle repeat cursor parameters:
**	    unlike repeat query parameters which become uninteresting as soon 
**	    as the query completes, we must save repeat cursor parameters and 
**	    information about them so it can be made available to QEQ_FETCH.
**
**	    There are two cases to consider WRT saving repeat cursor parameters:
**	      - if the repeat cursor parameters spanned multiple messages, they 
**	    	and the array of pointers pointing at them will be placed in QSF
**	    	memory.  Ordinarily, this memory is deallocated at the end of 
**		the query, but with OPEN repeat CURSOR we need to make an 
**		exception and hold on to that memory until the cursor is closed.
**		In order to prevent SCF from deallocating that memory, if 
**		sscb_thandle is non-zero, we will (in scs_def_curs()) copy 
**		sscb_thandle into the cursor block and then reset it and 
**		sscb_troot (to prevent SCF from trying to deallocate memory 
**		through the root which will most likely fail because the memory
**		was not allocated using sc0m_allocate()).
**
**	      - if, on the other hand, cursor parameters could all fit inside 
**		the message, we will (inside scs_def_curs()) allocate QSF memory
**		to contain the array of pointers and a copy of the message, 
**		adjust copied pointers to reflect the fact that instead of 
**		pointing to the message buffer they now point inside the 
**		allocated QSF memory, and store the handle for the QSF object 
**		inside the cursor block,  
**
**	    In both cases, memory containing description of repeat cursor 
**	    parameters will be deallocated when the cursor gets closed.
**
**	    A minor change will be required in CURSOR FETCH processing:
**		if fetching through a REPEAT cursor, we will initialize
**		qef_param and qef_pcount from info stored inside the cursor 
**		block
**	17-mar-94 (andre)
**	    removed break; statement which was eroneously placed after
**	    invocation of scs_def_curs() as a part of opening a repeat cursor;
**	    this fixes the problem of locked QP objects being left around after
**	    opening a repeat cursor.
**	17-mar-94 (anitap)
**	    We need the break so that we do not fall thru' after PSQ_EXECQRY.
**	    We need to go through PSQ_RETCURS to set up QEF_RCB. Else, qe
**	    qef_rcb->qef_output points to NULL and we get AV in
**	    QEAFETCH/qea_fetch() when we have constants or expressions in the
**	    target list. These changes are related to repeat cursors.	
**	25-mar-1994 (fpang)
**	    If SCS_DROPPED_GCA is set, reset scb->scb_sscb.sscb_state to 
**	    SCS_TERMINATE before returning. Also if qmode is zero, set qe_ccb
**	    because error reporting needs it.
**	    Fixes B60599, Server gets E_SC020A_UNRECOGNIZED_QMODE if FE client
**	    exits w/o disconnecting, and B60950, Server SEGV's after E_SC020A.
**	15-mar-94 (robf)
**	    Add alarm_audit parameter to scs_check_password() and
**	    make external.
**	18-mar-94 (robf)
**          Enforce SET ROLE not allowed in an MST, per ANSI/LRC
**	13-apr-94 (robf)
**          Security audit SET TRACE POINT statement
**      15-Apr-1994 (fred)
**          Added parameter to adu_free_objects.  This now takes a
**          statement number (supplied via QEF) which is used to
**          remove only the objects associated with this statement.
**          this allows us to be a bit more discriminating about
**          blowing temporaries out of the water.
**	28-apr-94 (andre)
**	    fix for bug 62890:
**	    we will not insist that KME be turned on if processing 
**	    system-generated queries (e.g. rules created to enforce constraints)
**	29-apr-94 (robf)
**          Rework unexpected copy errors. These can happen more often
**	    on secure systems due to MAC violations loading the table.
**	    Suppress suprious E_QE0025_USER_ERROR since real error has
**	    already been queued. Also don't send a cpinterrupt in this
**	    case. This is done to temporarily workaround a problem where
**	    the frontend/server hang after a copy error, since the interrupt
**	    arrives at the frontend when it is expecting a normal flow
**	    response and the protocol gets confused. This should be 
**	    readdressed later for a more permanent solution.
**	09-sep-94 (mikem)
**	    bug #65094
**	    Interrupts during DBU operations could cause memory trashing/AV's.
**	    Changed the interrupt handling portion of the sequencer to make
**	    sure if it was using the correct control block - in some cases it
**	    was using a QEU_CB when it wanted an QEF_RCB.
**      20-dec-1994 (nanpr01)
**	    Bug # 62502. Field ics_act1 was not getting cleared.	
**	24-apr-95 (jenjo02/lewda02)
**	    Add support for new DMF API.  Recognize the new query language
**	    type DB_NDMF and pass control to scs_api_call which calls dmf_call
**	    directly.
**	27-apr-95 (lewda02)
**	    API project: make sure the messages are queries before checking
**	    for DB_NDMF and calling scs_api_call().
**	11-may-95 (lewda02/shust01)
**	    RAAT API Project:
**	    Change naming convention.  API is now RAAT (Record At A Time) API.
**	15-jun-95 (thaju02)
**	    Clear the language id of DB_NDMF so that we do not make an
**	    scs_raat_call() during exceptional condition handling when using
**	    the RAAT API.
**    5-sep-1995 (angusm)
**        bug 70170: in scs_sequencer, stash qp handle for repeated 
**		  SELECTs , else redundant shared QSF locks get taken,
**		  preventing proper flushing etc of QSF cache.
**	14-sep-95 (shust01/thaju02)
**	    Added calls to scs_raat_load() which adds raat
**	    GCA buffers to link list, and scs_raat_parse(), which concatenates
**	    them into a large buffer before calling scs_raat_call().
**	14-nov-1995 (kch)
**	    Before the call qef_call(QEU_ETRAN, ( PTR ) &qeu), set the
**	    current query mode in QEU_CB, qeu.qeu_qmode, to
**	    scb->scb_sscb.sscb_qmode. This change fixes bug 72504.
**      28-nov-95 (lewda02/thaju02/stial02)
**          Enhanced XA Support.
**      06-mar-1996 (nanpr01)
**	    Removed the dependency of DB_MAXTUP. Required for increasing
**	    Tuple size. Replace DB_MAXTUP with Sc_main_cb->sc_maxtup.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	04-apr-1996 (loera01)
**	    Modified to support compressed variable-length datatypes.
**      25-apr-1996 (loera01)
**          Initialized opf_names variable before the GCA_NAMES_MASK or  
**	    GCA_COMP_MASK is set. 
**      14-jun-1996 (loera01)
**          Modified setting of result modifier in tuple descriptor so that the 
**          GCA_COMP_MASK is explicitly turned off, if compression is not supported.
**	12-sep-96 (kch)
**	    Add an additional check for the first RAAT GCA buffer
**	    (sscb_thandle == 0). Previously, subsequent non-RAAT GCA buffers
**	    could have language_id set to DB_NDMF, causing scs_raat_parse() to
**	    be erroneously called. This change fixes bugs 78299 and 78345.
**	 9-oct-96 (nick)
**	    Fix GCA_RETPROC to always return correct procedure id.
**	11-feb-97 (cohmi01)
**	    Use macro for determining if request is RAAT. If a RAAT request
**	    comes into CS_TERMINATE case, use RAAT format for errcode. (B80665)
**	02-dec-1996 (nanpr01)
**	    Map the new error message E_PS0010_TIMEOUT for bug 79443.
**      21-jan-97 (loera01)
**	    Bug 79287: Don't compress data if the tuple has a blob.
**	27-Feb-1997 (jenjo02)
**	    For qmode == PSQ_RETRIEVE queries, set qef_qacc = QEF_READONLY
**	    prior to calling QEQ_QUERY. If a single statement txn needs to be
**	    started, this will open it as READ ONLY and avoid stalling with
**	    a CKPDB-pending database.
**	03-apr-97 (nanpr01)
**	    If autocommit is on, you may get hangup on sessions because
**	    of uninitialized variable. see change # 421453.
**	24-apr-97 (hayke02)
**	    Use stashed qp handle (rp_handle) for the STAR repeated select
**	    case in order to prevent redundant shared QSF locks being taken
**	    which stop proper reclamation of QSF cache (see fix for bug
**	    70170). This change fixes bug 79770.
**	29-May-1997 (shero03)
**	    Save the error code from qeu_b_copy before destroying it.
** 	05-jun-1997 (wonst02)
** 	    Set READONLY transactions for READONLY databases.
**      10-jun-97 (thaju02)
**          bug #83429 - (also related to bug #81398) - when autocommit is on
**          blob temporary tables are not being destroyed until session
**          termination, exhausting the range of temporary table ids
**          and file descriptors, resulting in E_DM0077,E_DM9B09,E_AD7006,etc,.
**      18-Sep-97 (fanra01)
**          Ensure that the gca_result_modifier compression bit is not set in
**          a COPY FROM operation.  Varchar cmopression is not supported in
**          this case.  During a net copy gcc reacts to the compression bit
**          but libq sends uncompressed varchar causing gcc to misinterpret
**          the tuple contents and loop forever.
**	16-dec-97 (thaju02)
**	    Initialize qe_ccb to zero and added check of qe_ccb != 0.
**	17-feb-1998 (somsa01)
**	    When checking for GCA_COMP_MASK, we may have hit situations
**	    where we should not be checking for this bit. We were checking
**	    this bit in gca_result_modifier, which was referenced off of
**	    rd_tdesc. However, at query execution time, rd_tdesc is valid
**	    ONLY if varchar compression is turned on (since that's when we
**	    SAVE a copy of the tuple descriptor, while other times we just
**	    reference the one from the query plan which is "invalidated" at
**	    execution time). We now check this bit inside either
**	    rd_modifier or csi_state.  (Bug #89000)
**	16-Mar-1998 (jenjo02)
**	    Lightweight Factotum threads. Avoid unnecessary function calls
**	    when running "special" system threads.
**	    Removed SCS_NOGCA flag in sscb_flags, utilizing (1 << DB_GCF_ID)
**	    mask in sscb_need_facilities instead.
**	8-june-1998(angusm)
**	    Cross-integrate mckba01's fix from ingres63p for bug 71293
**	    (repeated select with params failure on 2nd invocation)
**	03-Sep-1998 (jenjo02)
**	    When executing "set [no]trace point" commands, leave a footprint
**	    in II_DBMS_LOG by TRdisplay-ing the command.
**	7-Dec-1998 (i4jo01)
**	    Cross-integration of change 438029 from oping12.
**	    Make sure we unset the 'session is idle' flag when receiving
**	    user input. When we are in state CS_INPUT that means
**	    that input has been received. (b93619).
**     27-oct-98 (stial01)
**          New RDF trace point RD0013 -> PSQ_ULMCONCHK.
**	16-Nov-1998 (jenjo02)
**	    When suspending on BIO read, use CS_BIOR_MASK.
**	18-jan-1999 (nanpr01)
**	    cur_amem sometime contained allocated memory and sometime 
**	    contained stack variable. As they do, the lower level routines
**	    like scs_input will not know and if it tries to deallocate
**	    it will fail with segv on some platform in sc0m_deallocate
**	    and on some platform it will return CORRUPT_POOL. Since
**	    we donot check the status of sc0m_deallocate consequently we 
**	    return without reporting any error.
**	28-jan-99 (inkdo01)
**	    Changes to support execution of row-producing procedures.
**	26-mar-99 (stephenb)
**	    When finishing up blob inserts, don't bother to delete the 
**	    temp work tables if we inserted into the base table, there
**	    aren't any.
**	20-may-99 (stephenb)
**	    Fix for row producing procedures: don't break out of the sequence
**	    retry loop if the query plan doesn't exist, continue and
**	    try to rebuild the plan, this shows up the underlying cause of
**	    the problem (if there is one), and reports the correct error 
**	    back to the front-end. (Found during runall testing).
**      28-sep-1999 (chash01)
**          the constant SCS_RETRY_LIMIT in scs_sequencer() is too small (3.)
**          RMS gateway run into problem with error of "US_1265 data base
**          reorganization..." I expand it to 10.
**	10-Mar-2000 (wanfr01) 
**	    Bug 100849, INGSRV 1128
**	    Do not switch modes to CS_INPUT if interrupt is received, since the
**	    front end is waiting for the GCA_IACK to be sent to it.
**	04-jan-2000 (rigka01) bug#99199
**	    when processing the execute database procedure statement,
**	    set qeu_flag to a new QEF_DBPROC value so that qeu_btran() can
**	    check the flag and set qeu_modifier to prevent the transaction
**	    state from being changed before the first statement of the 
**	    db procedure has been executed.
**	10-Mar-2000 (wanfr01) 
**	    Bug 100849, INGSRV 1128
**	    Do not switch modes to CS_INPUT if interrupt is received, since the
**	    front end is waiting for the GCA_IACK to be sent to it.
**	14-sep-2000 (hayke02)
**	    If gca_message_type is GCA1_INVPROC (indicating a DB proc
**	    executed from a libq front end, e.g. an ESQLC executable),
**	    then qe_ccb->qef_stmt_type is set to QEF_GCA1_INVPROC. This
**	    change fixes bug 102320.
**	04-Jan-2001 (jenjo02)
**	    When calling a non-SCF facility, set and restore sscb_cfac.
**	10-Jan-2001 (jenjo02)
**	    Deleted qef_dmf_id, use qef_sess_id instead.
**	    Removed code which redundantly reinitialized session
**	    control blocks, already done by scsinit.
**	19-Jan-2001 (jenjo02)
**	    Stifle calls to sc0a_write_audit() if not C2SECURE.
**      22-Mar-2001 (stial01)
**          ALL Temp etabs are dropped during qeq_cleanup->adu_free_objects, 
**          all except session temp table etabs. We don't need to call 
**          adu_lo_delete for a specific temp etab. Also during SCS_CP_FROM 
**          just call adu_free_objects. (b104317)
**	24-oct-2001 (stephenb)
**	    copy current query text into ics_lqbuf (saving last query) before
**	    re-initializing
**      06-Feb-2002 (thaju02, hanal04) Bug 106986 INGSRV 1677
**          Initialise qe_ccb->qef_rule_depth to 0 to ensure qeq_cleanup()
**          aborts appropriately. Once aborted, the sequencer determines
**          that the transaction state has changed and removes the
**          cursor from the csitbl via scs_remove_cursor().
**	13-Aug-2002 (jenjo02)
**	    Let SET LOCKMODE through to DMF; it'll decide which options
**	    are valid during a transaction and which are not.
**	20-dec-02 (inkdo01)
**	    Don't free procedure parameter memory until end of row
**	    producing procedure (fixes 109337).
**	18-mar-2003 (devjo01) b109860
**	    Fix memory leak seen when using row producing procedures.
**      30-mar-03 (wanfr01)
**          Bug 108353, INGSRV 2201
**          If retrying due to E_OP000A_RETRY, try optimizing query 
**	    without combining aggregates since this may invalidate some
**	    query plans.
**	22-sep-03 (inkdo01)
**	    Set and use new cur_rowproc flag to avoid re-GETHANDLEing 
**	    row producing procs for each output buffer (fixes 110333).
**      15-Jan-2004 (hanal04) Bug 111417 INGSRV2628
**          When requesting a user password during session initialisation
**          drop the previously acquired DB locks and reaquire them once
**          we have the user input. If an invalid user has requested an
**          exclusive DB lock and refuses to enter a value for the password,
**          valid users will no longer be locked out.
**      18-Aug-2004 (hanal04) Bug 112836 INGSRV2936
**          Do not initialise XA XID values after calling QET_ABORT.
**          The TMS Server or Application Server will issue a tpbort()
**          that will expect to be able to find the same XA XID. This
**          change prevents E_SC0206 being reported when the XA XID
**          can not be found in the scan_scb list for an XA ROLLBACK.
**	2-Mar-2004 (schka24)
**	    Restore changes lost by x-integration: treat QSF memory full like
**	    PSF memory full, and don't reference the qe-ccb after it's been
**	    deallocated in copy error handling.
**	24-Apr-2004 (schka24)
**	    Fix sloppy handling of errors after COPY starts, but before
**	    we manage to make it to copy-into or copy-from state.
**	4-May-2004 (schka24)
**	    More of the same.  Terminate COPY neatly upon force-abort.
**	11-May-2004 (schka24)
**	    Don't free COPY blob objects here, as child thread may still need
**	    them.  Let lower levels deal with cleanup.
**	15-Sep-2004 (schka24)
**	    Terminate COPY in more error cases, so that child threads can
**	    be closed down more neatly.  Also fix typo in deallocate call,
**	    from a fix integration.
**	25-nov-04 (inkdo01)
**	    Fix problem with RPPs that need to be recompiled. An execution
**	    can proceed by sending GCA_TUPLES w/o corresponding GCA_TDESCR.
**	06-dec-2004 (thaju02)
**	    Fetch from a cursor involving view fails with E_PS0B04, 
**	    E_SC0215.  Added retry_fetch flag. At open cursor, store 
**	    query text object handle and lock id in csi entry, and null 
** 	    out scb->scb_sscb.sscb_thandle/sscb_troot/sscb_tlk_id to avoid 
**	    destroying object when open cursor exits sequencer.
**	    If fetch to be retried, using stored query text object handle
**	    reparse and regenerate qep with noflatten,and retry fetch. 
**	    (B113588)
**	28-feb-2005 (wanfr01) 
**          Bug 64899, INGSRV87
**          Add stack overflow handler for TRU64.
**      23-Mar-2005 (hayke02) Bug 113618 INGSRV3081
**          For RPPs do not deallocate troot unless the RPP has completed.
**    01-apr-05 (toumi01)
**        If the fe interrupts a copy e.g. on a bad output directory name
**        set the CPY_FAIL flag in qe_copy->qeu_stat so that we'll clean up
**        CUT stuff in qeu_e_copy.
**    03-Apr-2005 (jenjo02)
**        Use MEcopy rather than STcopy to copy ics_qbuf to ics_lqbuf.
**        The queries are not reliably EOS-terminated.
**    22-Jul-2005 (thaju02)
**        Add SET SESSION WITH ON_USER_ERROR = ROLLBACK TRANSACTION.
**    12-Aug-2005 (schka24)
**        Don't hurt the output buffer stuff if QEF returns after producing
**        some rows, when QEF is asking for a retry (typically a load-QP).
**        A row-producing procedure might have emitted some rows, and might
**        emit some more after we return to QEF.
**    18-aug-2005 (thaju02)
**        qsf object not being destroyed if open cursor fails due to
**        deadlock.  If open cursor fails, check scs_def_curs() status
**        to avoid nulling out sscb_thandle.
**    24-Aug-2005 (schka24)
**        12-Aug edit took out boolean var initialization, shouldn't have.
**        Caused premature cur_amem cleanup and COPY INTO failure,
**        although it only showed up on Windows.
**    14-Sep-2005 (schka24)
**        Rowprocs, again.  Track rowproc state more explicitly so that we
**        know when it's a recreate (and hence need to send TDESCR stuff)
**        vs other situations (output rows, ruleqp return-back).  The
**        12-aug changes broke Doug's 4-Nov fix.
**    15-Sep-2005 (schka24)
**        Replace long scb->scb_sscb.sscb_cquery with shorter cquery->.
**        Fix possible bad rowproc deallocate uncovered by doing so.
**    3-Feb-2006 (kschendel)
**        Param removed from scs-dbdb-info call, fix here.
**      13-Dec-2005 (hanal04) Bug 115456 INGSRV3474
**          Uninitialised storage for the unfinished tuple (qeu_sbuf) was
**          causing zero length long varchars to be corrupted during a
**          COPY FROM.
**      10-Jan-2005 (hanal04) Bug 115634
**          Extend the fix for bug 115456 to re-initialise the 
**          qe_copy->qeu_sptr buffer if it is being reused.
**      09-Mar-2006 (horda03) Bug 115826
**          Fix for Bug 115634 should only clear the
**          qe_copy->qeu_sptr buffer when it is being used for a new
**          tuple. Original change broke the blob02.sep test.
**	26-Jun-2006 (jonj)
**	    Added XA integration of new GCA messages,
**		PSQ_GCA_XA_START,
**		PSQ_GCA_XA_END,
**		PSQ_GCA_XA_PREPARE,
**		PSQ_GCA_XA_COMMIT,
**		PSQ_GCA_XA_ROLLBACK
**      20-Jul-2006 (kibro01) bug 114735
**          Calling from copyapp out in a star server, the remote cursor
**          has already been removed by EXECQRY so an extra qef_call(QEQ_CLOSE)
**          should not be called.
**	28-Jul-2006 (jonj)
**	    Fix error code ==> XA error transformation, set GCA_REFUSE
**	    response type on any PSQ_GCA_XQ error.
**	30-Aug-2006 (jonj)
**	    Fix force-abort of XA_STARTed transaction to return GCA_REFUSE
**	    with an error code of XA_RBOTHER.
**	13-Sep-2006 (kibro01) bug 116600
**	    Don't free the memory used for >15 parameters until we've finished
**	    using it and have released the QEF reference to them.
**	14-feb-2007 (dougi)
**	    Init QEF fields for potentially scrollable cursors.
**	10-apr-2007 (dougi)
**	    Add support for returned fields from scrollable cursors.
**	11-may-2007 (dougi)
**	    Auto selection of base table structure from constraints required
**	    a tiny tweak.
**	17-july-2007 (dougi)
**	    Permit row count mismatch in scrollable cursors.
**	24-Aug-2007 (kibro1) b119002 b114735
**	    In some cases we have locks pending on the remote database, so
**	    we do need to call the QEQ_CLOSE for a star database after all.
**	    Inside qeq we can determine if it is already closed (open_count=0).
**	13-nov-2007 (dougi)
**	    Changes to support repeat dynamic queries.
**	27-nov-2007 (dougi)
**	    0 rowcount for scrollable can overlay SCF pool header.
**	3-jan-2008 (dougi)
**	    Minor fix to assure cached dynamic QPs aren't left locked.
**	3-jan-2008 (dougi)
**	    Add "set [no]cache_dynamic" statement.
**	7-jan-2008 (dougi)
**	    Fix uninit'ed csi values for cached dynamic.
**	25-Feb-2008 (kiria01) b119976
**	    Allow scs_compress() and scs_emit_rows() to return an indication of
**	    an error condition relating to potentially corrupt data.
**	4-march-2008 (dougi)
**	    Add no-op's for "set [session] [no]cache_dynamic".
**	15-Sep-2008 (kibro01) b120571
**	    Use same session ID as in use in iimonitor
**      06-Feb-2009 (gefei01)
**          Fix bug 121624: Save qmode only once before any tproc recompilation.
**      12-Feb-2009 (gefei01)
**          Bug 121651: Fix SEGV which occurs when the base table of
**          a table procedure has been dropped.
**      27-Feb-2009 (gefei01)
**          Support cursor select for table procedure.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          Add check for new E_DM0201_NOLG_MUSTLOG_ERROR
**	9-sep-2009 (toumi01) SIR 119414 BUG 122583
**	    Increment sscb_nostatements for PSQ_REPDYN so that we do not
**	    feed duplicate IDs for TDESCs to GCC.
**	 1-Oct-09 (gordy)
**	    Don't return a DB procedure GCA_ID if the procedure
**	    name doesn't fit in the GCA_ID.
**	26-oct-2009 (toumi01) BUG 122583
**	    For cache dynamic query execution make sure we have a private
**	    copy of the tuple descriptor to avoid race conditions updating
**	    the descriptor with the scb statement number and GCA_COMP_MASK.
**	30-Sep-2009 (wanfr01) b122664
**	    If scs_compress gets an error and you abort a copy ... into ...
**	    inform the cut threads that the copy has failed.
**      Nov/Dec 2009/Jan 2010 (stephenb)
**          Lots of changes for batch processing
**	19-Mar-2010 (kschendel) SIR 123448
**	    Remove some unused or meaningless qeu-copy members.
**      24-mar-2010 (stephenb)
**          Minor fixes to batch processing:
**           - Set rowcound correctly in GCA message block
**           - don't deallocate copy control block in SCS_CONTINUE_COPY
**             (this causes SEGV in ULF); 
**             it gets done laterin SCS_FINISH_COPY.
**	25-Mar-2010 (kschendel) SIR 123485
**	    Hatchet the COPY handler out of the middle of the massive
**	    switch stmt.  (removes about 1600 lines.)
**	    Readability: replace scb->scb_sscb. with sscb->, likewise
**	    with scb_cscb.  Fix up some bad indents. Add comments.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Chop another 150 lines by table-driving most PSY calls.
**	    Clean out routine locals a little bit.
**	30-mar-2010 (stephenb)
**	    re-add QSF cleanup for batch processing; we need to
**	    clean out the query stuff for every query, not just at
**	    the end of the batch, otherwise we loose track.
**	19-Mar-2009 (gupsh01) SIR 123444
**	    Added support for rename table/column.
**	31-Mar-2010 (kschendel) SIR 123485
**	    More fine-tuning for COPY, handle it early and get out of the way.
**	    Use preset internal savepoint name in QEF server block.
**	29-apr-2010 (stephenb)
**	    Batch flags are now on psq_flag2. Add case for PSQ_SETBATCHCOPYOPTIM.
**	28-may-2010 (stephenb)
**	    add sanity check for no query text case; make sure the number of 
**	    parameters provided by GCA is the number expected for the saved query
**	17-jun-2010 (stephenb)
**	    Turn off copy optimization flag (SCS_INSERT_OPTIM) for errors that will
**	    terminate the batch. Remove call to QEU_E_COPY when handling
**	    batch copy optimization errors, the call to scs_copy_error
**	    does this for us already (Bug 123939)
**	22-Jun-2010 (kschendel) b123775
**	    Ensure no bogus rowcount added to running total if query returns
**	    asking for a tproc recreate, by ensuring that we go through the
**	    ERROR path and not the WARNING path.  Restore qmode once the
**	    tproc QP is loaded and before we attempt to re-enter QEF.
**	24-jun-2010 (stephenb)
**	    Add inteligence for loading groups of rows in batched insert to copy
**	    optimization. We will delay the start of the copy, and only
**	    call QEF to load the data when PSF tells us it has filled the
**	    buffer.
**	7-jul-2010 (stephenb)
**	    We need to delay sending of GCA messages for all batch
**	    processing to avoid a send/recieve deadlock. Update message
**	    handling for batches.
**	20-jul-2010 (stephenb)
**	    During a query re-try, if the sequencer incorrectly re-sets the query
**	    state, we may end up trying to parse a query that has no query text
**	    (e.g. a cursor fetch). This should never happen (i.e. it's a code
**	    error), but we would like to generate a nice error when it occurs
**	    rather than causing a SEGV; so we should not assume that sscb_troot
**	    contains anything.
**	10-aug-2010 (stephenb)
**	    7-jul change above should also apply GCA_RETPROC since we support
**	    "execute procedure" in batch. Make sure we allocate a new message
**	    when sending GCA_RETPROC in batch otherwise we may get an
**	    inconsistent queue.
**      02-sep-2010 (maspa05) SIR 124346
**          Add end-query SC930 record - with error number and rowcount.
**      04-oct-2010 (maspa05) Bug 124534
**          Rowcounts for batched queries were incorrect. Need to check the
**          list of GCA response messages to find the latest.
**	14-Oct-2010 (kschendel) SIR 124544
**	    Change "set result-structure" to a do-nothing.  PSF handles
**	    result-structure now, not OPF.
*/
DB_STATUS
scs_sequencer(i4 op_code,
	      SCD_SCB *scb,
	      i4  *next_op )
{
    DB_STATUS           ret_val = E_DB_OK;
    DB_STATUS		status;
    DB_STATUS		error;
    DB_ERROR	        dberror;
#define			    SCS_RETRY_LIMIT	    10
    i4			qry_status = SCS_OK;
    i4			deadlock_occurred = 0;
    i4			internal_retry;
    DB_SQLSTATE         sqlstate;
    i4			i;
    i4			csi_no;
    i4			qmode, qmode_saved = 0;
    i4			qsf_object = 0;
    i4			repeat_redef = 0;
    i4			recreate = 0;
    i4			abort_action = 0;
    i4			xact_state = 0;
    i4			was_xact = 0;
    i4                  singleton = 0;  /* Real singleton or a 1-row FETCH */
    i4			ule_error;
    sequencer_action_enum seq_action;
    QSF_RCB		*qs_ccb = NULL;
    DMC_CB		*dm_ccb;
    PSQ_CB		*ps_ccb;
    OPF_CB		*op_ccb;
    QEF_RCB		*qe_ccb = NULL;
    QEU_COPY		*qe_copy;
    SCS_CSITBL		*csi;
    QEF_DATA		*data_buffer;
    SCC_CSCB		*cscb;		/* Convenience: scb->scb_cscb */
    SCC_GCMSG		*message;
    SCS_CURRENT		*cquery;	/* Current query info inside scb */
    SCS_SSCB		*sscb;		/* Convenience: scb->scb_sscb */
    QEF_DATA		qef_data;
    PTR			psf_qp_handle;
    i4			psf_lkid;
    i4			psf_stuff = 0;	/* PSF cleanup:  0 = none, 1 = destroy
					** qsf obj in psq_tree.qso_handle,
					** 2 = also destroy the QP object
					** named by psq_plan */
    i4			psf_tstuff = 0;	/* 1 = destroy qsf text object with
					** qsf handle/ID psq_text */
    QSO_OBID		psq_tree;
    QSO_OBID		psq_text;
    DB_CURSOR_ID	psq_plan;
    PTR			rp_handle = NULL;
    bool		procid_updated = FALSE;
    DB_CURSOR_ID	proc_qid;
    bool		retry_fetch = FALSE;
    bool		on_user_error = FALSE;
    bool                is_tproc_load_qp = FALSE;
    bool                tproc_cur_load_qp = FALSE;
    QSO_OBID            cur_qp_saved;
    DB_CURSOR_ID        cursid_saved;
    PTR                 dsh_saved;
    bool		notext_copy_optim = FALSE;

    /*
    ** qef_wants_ruleqp and qef_psf_alias are used to handle procedure 
    ** recreation while in the middle of rule processing - this is very 
    ** similar to regular user-invoked procedure recreation 
    ** (uses query mode PSQ_EXEDBP).  The use of the 2 variables is the 
    ** following:
    **
    ** On return from QEQ_QUERY an error (LOAD or INVALID QP) indicate that
    ** QEF wants a new QP (qef_wants_ruleqp is set, and qef_psf_qpid is filled
    ** with the dbp name).  Control returns to the SCF loop and, like
    ** dbp recreation, falls into the parser (PSQ_RECREATE) with the dbp
    ** name and owner. The parser recreates the procedure tree, which is
    ** optimized, and then control returns back to QEF... but, unlike other
    ** QEF requests ONLY the qp id is reset - most other fields must be left
    ** intact.
    */
    i4			qef_wants_ruleqp = 0;
    QSO_NAME		qef_psf_alias;	/* Saved dbp id from QEF */

    PTR			dynqp_handle;
    /* indicates that QEF wants a QP for the Execute Immediate text in 
    ** qef_qsf_handle.
    */
    i4			qef_wants_eiqp = 0;
    /* indicates that an execute procedure request is on a row-producing proc */
    bool		rowproc = FALSE;
    bool		rowprocdone = FALSE;
    bool		dynqp_got = FALSE;

    /*
    ** fetch_rows is used when pre-fetching rows for a read-only cursor.
    ** inside scs_def_curs(), QEQ_OPEN may return the read-only status to the
    ** client (and improve read-only FETCH performance), and fetch_rows
    ** determines how many rows the next QEQ_FETCH operation will retrieve.
    */
    i4                  fetch_rows;
    i4			curspos = 0;		/* for scrollable cursors */
    i4			rowstat = 0;
    
    /* For distributed */
    SCF_TD_CB		*rqf_td_desc = NULL;
    GCA_RV_PARMS	*rv;
    SCC_GCMSG		*tdesc = NULL, *tdata = NULL;

    *next_op = CS_INPUT;   /* default decision is get more stuff */

    cquery = &scb->scb_sscb.sscb_cquery;	/* Easier to read */
    cscb = &scb->scb_cscb;
    sscb = &scb->scb_sscb;

    /* Set SCF as the current facility */
    sscb->sscb_cfac = DB_SCF_ID;

    if ((op_code != CS_TERMINATE)           /* Don't translate if terminating */
        &&
	((sscb->sscb_interrupt)
	 ||
	 (sscb->sscb_force_abort == SCS_FAPENDING))
       )
    {
	op_code = CS_OUTPUT;
    }

    /*
    ** Before processing any queries check for alerts to make sure we've
    ** acknowledged all event notifications.  For a detailed description
    ** of why we check here review introductory comment in scenotify.c.
    */
    /* Don't bother if internal-type thread */
    if (sscb->sscb_state != SCS_SPECIAL)
	sce_chkstate(SCS_ASNOTIFIED, scb);

    /*
    ** If receiving user input then we are not idle
    */
    if(op_code==CS_INPUT || sscb->sscb_state!=SCS_INPUT)
	    sscb->sscb_is_idle=FALSE;

    /* If in the middle of COPY FROM or COPY INTO, deal with it and get
    ** out of the way.  COPY startup has to parse and go thru the usual.
    ** Let the "error" state fall thru as well, into the normal code.
    */
    if (sscb->sscb_qmode == PSQ_COPY
      && (sscb->sscb_state == SCS_CP_FROM || sscb->sscb_state == SCS_CP_INTO)
      && ! (sscb->sscb_interrupt || (sscb->sscb_force_abort == SCS_FAPENDING)) )
    {
	xact_state = QEF_X_MACRO(sscb->sscb_qescb);
	qry_status = SCS_OK;
	psf_stuff = 0;
	if (sscb->sscb_state == SCS_CP_FROM)
	    seq_action = scs_copy_from(scb, next_op, &ret_val);
	else
	    seq_action = scs_copy_into(scb, op_code, next_op, &ret_val);
	/* Note that neither of these can return SEQUENCER_CONTINUE! */
	if (seq_action == SEQUENCER_RETURN)
	    return (ret_val);
	/* else it's SEQUENCER_BREAK.  We're supposed to be in the
	** "massive switch", so we'll set up the status and jump down
	** to exit the switch (and the "massive for" as well).
	** not-OK sets the query status.  Anything less than ERROR
	** resets ret_val to OK, although ret_val and qry_status
	** are sorta kinda the same thing but not really.
	*/
	if (ret_val != E_DB_OK)
	    qry_status = GCA_FAIL_MASK;
	if (DB_SUCCESS_MACRO(ret_val))
	    ret_val = E_DB_OK;
	goto massive_for_exit;
    }

    switch (op_code)
    {
	case CS_INITIATE:
	case CS_INPUT:
	    /* Check quickly for special system threads first: */
	    if (sscb->sscb_state == SCS_SPECIAL)
	    {
		/*
		** This is specialized dbms task - call routine to
		** do the specific task.
		*/

		/*
		** Set up thread to terminate if task returns.
		** If this thread is necessary for the server to function
		** (sscb_flags is SCS_VITAL) then it is a server fatal
		** for this thread to fail to initiate.
		*/
		*next_op = CS_TERMINATE;
		sscb->sscb_state = SCS_TERMINATE;

		ret_val = scs_initiate(scb);
		if (ret_val)
		{
		    sc0e_put(E_SC0240_DBMS_TASK_INITIATE, 0, 1,
			     sizeof(*sscb->sscb_ics.ics_eusername),
			     (PTR)sscb->sscb_ics.ics_eusername,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0);
		    if (sscb->sscb_flags & SCS_VITAL)
			return (E_DB_FATAL);
		    return (E_DB_ERROR);
		}

		/*
		** Scs_dbms_task should not return until the thread is
		** ready to exit.  If the thread exits abnormally, the
		** error should have been logged in scs_dbms_task.
		** If the thread is a vital thread and it exits before
		** server shutdown time, then we must bring down the server,
		** if the vital task is supposed to be permanent.
		*/
		ret_val = scs_dbms_task(scb);
		if (	(ret_val == E_DB_FATAL)
		    ||
			(  ( (sscb->sscb_flags & (SCS_VITAL | SCS_PERMANENT))
				    == (SCS_VITAL | SCS_PERMANENT)
			    )
			&&
			    (Sc_main_cb->sc_state != SC_SHUTDOWN)
			)
		    || ( (sscb->sscb_flags & SCS_STAR)
			  &&
			 (sscb->sscb_stype == SCS_S2PC)
			  &&
			 (Sc_main_cb->sc_state == SC_2PC_RECOVER)
		       )
		   )
		{
		    i4 session_count;
		    sc0e_put(E_SC0241_VITAL_TASK_FAILURE, 0, 1,
			     sizeof(*sscb->sscb_ics.ics_eusername),
			     (PTR)sscb->sscb_ics.ics_eusername,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0);
		    CSterminate(CS_KILL, &session_count);
		    return (E_DB_FATAL);
		}
		return (ret_val);
	    }
	    else if (sscb->sscb_state == SCS_INPUT)
	    {
		sscb->sscb_noretries = 0;
		ret_val = scs_input(scb, &sscb->sscb_state);
		if (ret_val == E_DB_OK)
		{
		    if (sscb->sscb_state != SCS_INPUT)
		    {
			if (sscb->sscb_force_abort == SCS_FACOMPLETE)
			{
			    if (sscb->sscb_thandle != 0)
			    {
				qs_ccb = sscb->sscb_qsccb;
				qs_ccb->qsf_lk_id = sscb->sscb_tlk_id;
				qs_ccb->qsf_obj_id.qso_handle = sscb->sscb_thandle;
				status = qsf_call(QSO_DESTROY, qs_ccb);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
								E_SC0210_SCS_SQNCR_ERROR);
				}
				sscb->sscb_thandle = 0;
				sscb->sscb_troot = 0;
			    }
			    else if (sscb->sscb_troot != 0)
			    {
				sc0m_deallocate(0, &sscb->sscb_troot);
			    }

			    sscb->sscb_state = SCS_INPUT;
			    sscb->sscb_qmode = 0;
			    scc_fa_notify(scb);
			    *next_op = CS_EXCHANGE;
			    sscb->sscb_force_abort = 0;
			    sscb->sscb_cfac = DB_CLF_ID;
			    return(E_DB_OK);
			}
			/*
			** About to start processing the input data.  Make sure
			** to ignore any pending send-alert-message completions
			** that occur during our processing.  For a detailed
			** description see initial comment in scenotify.c.
			*/
			sce_chkstate(SCS_ASPENDING, scb);
			break;
		    }
		    else if (cscb->cscb_gci.gca_status == E_GC0027_RQST_PURGED)
		    {
			cscb->cscb_gci.gca_status = E_SC1000_PROCESSED;
			CScancelled(0);
			if (sscb->sscb_force_abort == SCS_FACOMPLETE)
			{
			    if (sscb->sscb_thandle)
			    {
				qs_ccb = sscb->sscb_qsccb;
				qs_ccb->qsf_lk_id = sscb->sscb_tlk_id;
				qs_ccb->qsf_obj_id.qso_handle = sscb->sscb_thandle;
				status = qsf_call(QSO_DESTROY, qs_ccb);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
								E_SC0210_SCS_SQNCR_ERROR);
				}
				sscb->sscb_thandle = 0;
			    }

			    sscb->sscb_state = SCS_INPUT;
			    sscb->sscb_qmode = 0;
			    scc_fa_notify(scb);
			    *next_op = CS_EXCHANGE;
			    sscb->sscb_force_abort = 0;
			}
			else if (cscb->cscb_mnext.scg_next !=
				    (SCC_GCMSG *) &cscb->cscb_mnext)
			{
			    /*
			    ** If there is an IACK waiting to be sent home,
			    ** send it.  There could be a pending alert message
			    ** that was queued after the interrupt completed
			    ** (see details in scenotify.c).
			    */
			    if (cscb->cscb_mnext.scg_next->scg_mtype == GCA_IACK)
			    {
				*next_op = CS_EXCHANGE;
			    }
			}
			sscb->sscb_cfac = DB_CLF_ID;
			return(E_DB_OK);
		    }
		    else
		    {
			/*
			** We need more input.  We may have been called because
			** of alerts, or because of a multi-buffer user query.
			** If the read is still in progress, and there are more
			** alerts or we are already in "pending" mode then
			** make sure to send the alert output.  Otherwise
			** we need more input.  For a detailed description
			** refer to the large comment in scenotify.c.
			*/
				    /* No user input was really read and ... */
			if (   (sscb->sscb_aflags & SCS_AFINPUT) == 0
				   /* More alerts or ... */
			    && (   (sscb->sscb_alerts != NULL)
				   /* One already pending -- need to complete */
			        || (sscb->sscb_astate == SCS_ASPENDING)
			       )
			   )
			{
			    *next_op = CS_EXCHANGE;	/* Call scc_send */
			}
			else
			{
			    *next_op = CS_INPUT;	/* Get more input */
			}
			sscb->sscb_aflags &= ~SCS_AFINPUT;
			sscb->sscb_cfac = DB_CLF_ID;
			return(E_DB_OK);
		    }
		}
		else
		{
		    *next_op = CS_TERMINATE;
		    sscb->sscb_cfac = DB_CLF_ID;
		    return(E_DB_OK);
		}
	    }
	    else if (sscb->sscb_state == SCS_SPASSWORD)
	    {
		/*
		** This state means we have sent password prompt to
		** the user and are now processing the result.
		*/

                dm_ccb = sscb->sscb_dmccb;
                status = scs_dbdb_info(scb, dm_ccb, &dberror);
		if (status)
		{
		    GCA_ER_DATA         *emsg;
		    GCA_DATA_VALUE  *gca_data_value;

		    ret_val = dberror.err_code;

		    /* Log error on back end */
		    if (ret_val != E_US0010_NO_SUCH_DB )
                    {
		        sc0e_0_put(ret_val, 0);
                    }
		    else
		    {
			sc0e_put(ret_val, 0, 1,
                           sizeof(sscb->sscb_ics.ics_dbname.db_db_name),
                           (PTR) sscb->sscb_ics.ics_dbname.db_db_name,
                           0, (PTR) 0,
                           0, (PTR) 0,
                           0, (PTR) 0,
                           0, (PTR) 0,
                           0, (PTR) 0 );
		    }
		    (VOID) scs_fire_db_alarms(scb, DB_CONNECT|DB_ALMSUCCESS);
                    sc0e_0_put(E_SC0123_SESSION_INITIATE, 0);

                    /* Tell the front end why we have failed */
                    status = sc0m_allocate(0,
                            sizeof(SCC_GCMSG) + sizeof(GCA_ER_DATA)
                                    + (ret_val ? ER_MAX_LEN : 0),
                            DB_SCF_ID,
                            (PTR)SCS_MEM,
                            SCG_TAG,
                            (PTR *) &message);

                    if (status)
                    {
                        sc0e_0_put(status, 0 );
                        sc0e_0_uput(status, 0 );
                        scd_note(E_DB_SEVERE, DB_SCF_ID);
                        message = (SCC_GCMSG *) cscb->cscb_inbuffer;
                    }
                    message->scg_mask = 0;
                    message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
                    message->scg_next = (SCC_GCMSG *) &cscb->cscb_mnext;
                    message->scg_prev = cscb->cscb_mnext.scg_prev;
                    cscb->cscb_mnext.scg_prev->scg_next = message;
                    cscb->cscb_mnext.scg_prev = message;

                    emsg = (GCA_ER_DATA *)message->scg_marea;
                    gca_data_value = emsg->gca_e_element[0].gca_error_parm;

                    emsg->gca_l_e_element = 1;
                    emsg->gca_e_element[0].gca_id_server = Sc_main_cb->sc_pid;
                    emsg->gca_e_element[0].gca_severity = 0;
                    emsg->gca_e_element[0].gca_l_error_parm = 1;

		    if (ret_val != E_US0010_NO_SUCH_DB )
		    {
                        status = ule_format(ret_val, 0, ULE_LOOKUP, &sqlstate,
                                gca_data_value->gca_value, ER_MAX_LEN,
                            &gca_data_value->gca_l_value, &ule_error, 0);
		    }
		    else
		    {
			status = ule_format(ret_val, 0, ULE_LOOKUP, &sqlstate,
                          gca_data_value->gca_value, ER_MAX_LEN,
                          &gca_data_value->gca_l_value, &ule_error, 1,
                          sizeof(sscb->sscb_ics.ics_dbname.db_db_name),
                          (PTR)sscb->sscb_ics.ics_dbname.db_db_name,
                          0, (PTR)0,
                          0, (PTR)0,
                          0, (PTR)0,
                          0, (PTR)0,
                          0, (PTR)0 );
		    }

                    gca_data_value->gca_type = DB_CHA_TYPE;

                    if (cscb->cscb_version <= GCA_PROTOCOL_LEVEL_2)
                    {
                        emsg->gca_e_element[0].gca_id_error = ret_val;
                        emsg->gca_e_element[0].gca_local_error = 0;
                    }
                    else if (cscb->cscb_version < GCA_PROTOCOL_LEVEL_60)
                    {
                        emsg->gca_e_element[0].gca_local_error = ret_val;
                        emsg->gca_e_element[0].gca_id_error =
                            ss_2_ge(sqlstate.db_sqlstate, ret_val);
                    }
                    else
                    {
                        emsg->gca_e_element[0].gca_local_error = ret_val;
                        emsg->gca_e_element[0].gca_id_error =
                            ss_encode(sqlstate.db_sqlstate);
                    }

                    *next_op = CS_OUTPUT;    /* die if can't start up */
                    sscb->sscb_state = SCS_TERMINATE;
                    message->scg_mtype = GCA_RELEASE;
                    message->scg_msize = sizeof(GCA_ER_DATA)
                                        - sizeof(GCA_DATA_VALUE)
                                            + sizeof(i4)
                                            + sizeof(i4)
                                            + sizeof(i4)
                        + gca_data_value->gca_l_value;

		    sscb->sscb_cfac = DB_CLF_ID;
		    return (E_DB_OK);
                }

	    	sscb->sscb_state = SCS_RPASSWORD;
		break;
	    }
	    else if ((sscb->sscb_state == SCS_CP_FROM) ||
		     (sscb->sscb_state == SCS_CP_INTO) ||
		     (sscb->sscb_state == SCS_CP_ERROR))
	    {
		/*
		** These states mean we are in the middle of executing a
		** COPY (although something must be wrong, e.g. interrupt,
		** else we'd handle these early on).  Just skip along.
		*/
		break;
	    }
	    else if (sscb->sscb_state == SCS_DCONNECT)
	    {
		break;
	    }
	    else if ((sscb->sscb_state == SCS_ACCEPT)
				    && (op_code == CS_INITIATE))
	    {
		GCA_RR_PARMS	rrparms;
		STATUS		gca_status;

		rrparms.gca_assoc_id = cscb->cscb_assoc_id;
		rrparms.gca_local_protocol = cscb->cscb_version;
		rrparms.gca_modifiers = 0;
		rrparms.gca_request_status = OK;

                /* If there is a FE associated with the session set the
                ** DB_GCF_ID flag to ensure that the session is terminated
                ** correctly even if the session is cancelled before initialisation
                ** has completed.
                */
		sscb->sscb_facility |= (1 << DB_GCF_ID);

		status = IIGCa_call(GCA_RQRESP,
				    (GCA_PARMLIST *) &rrparms,
				    0,
				    (PTR)scb,
				    (i4)-1,
				    &gca_status);

                if (status != OK)
                {
                    /* The call to IIGCA_CALL has failed, so terminate the session.
                    */
                    sc0e_0_put(gca_status, 0);
                    *next_op = CS_TERMINATE;
                    scd_note(E_DB_SEVERE, DB_GCF_ID);
                    return(E_DB_SEVERE);
                }

                /* Message sent to FE, wait for response */

                status = CSsuspend(CS_BIOR_MASK, 0, 0);

                if (status != OK)
                {
                    /* The session was Interrupted/Aborted so log state and terminate */
                    sc0e_0_put( status, 0);

                    *next_op = CS_TERMINATE;
                    scd_note(E_DB_SEVERE, DB_SCF_ID);
                    return(E_DB_SEVERE);
                }

                /* Check the status of the sent message */

                if (rrparms.gca_status != OK)
                {
                    sc0e_0_put(rrparms.gca_status, &rrparms.gca_os_status);
                    *next_op = CS_TERMINATE;

                    if (rrparms.gca_status == E_GC0001_ASSOC_FAIL)
                    {
                          /* The FE has terminated the connection, so no need to notify that the
                          ** connection has been lost.
                          */
                          sscb->sscb_state = SCS_TERMINATE;
                    }
                    scd_note(E_DB_SEVERE, DB_SCF_ID);
                    return(E_DB_SEVERE);
                }

	        cscb->cscb_gci.gca_status = E_SC1000_PROCESSED;
		status = sc0m_allocate(0,
			    sizeof(SCC_GCMSG),
			    DB_SCF_ID,
			    (PTR)SCS_MEM,
			    SCG_TAG,
			    (PTR *) &message);
		if (status)
		{
		    sc0e_0_put(status, 0);
		    scd_note(E_DB_SEVERE, DB_SCF_ID);
		    return(E_DB_SEVERE);
		}
		message->scg_mask = 0;
		message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
		message->scg_next = (SCC_GCMSG *) &cscb->cscb_mnext;
		message->scg_prev = cscb->cscb_mnext.scg_prev;
		cscb->cscb_mnext.scg_prev->scg_next = message;
		cscb->cscb_mnext.scg_prev = message;

		message->scg_mtype = GCA_ACCEPT;
		message->scg_msize = 0;

		*next_op = CS_EXCHANGE;
		sscb->sscb_state = SCS_INITIATE;
		sscb->sscb_cfac = DB_CLF_ID;
		return(OK);
	    }
	    else if (sscb->sscb_state == SCS_INITIATE)
	    {
		ret_val = scs_initiate(scb);
		if ((!ret_val) && (sscb->sscb_state == SCS_INITIATE))
		{
		    /* In this case, we have more MD_ASSOC parms to read */
		    *next_op = CS_INPUT;
		    sscb->sscb_cfac = DB_CLF_ID;
		    return(OK);
		}
	        /*
	        ** Check for password prompt, if required send this
	        ** as well, note we send this BEFORE the ACCEPT message.
	        */

	        if(!ret_val && sscb->sscb_state == SCS_SPASSWORD)
	        {

		    status=scc_prompt(scb,
				ERx("Please enter your password:"),
				sizeof("Please enter your password:")-1,
				sizeof(DB_PASSWORD),
				TRUE,/* no echo */
				TRUE,/* password */
				0    /* no timeout */
				);

		    if(status!=E_DB_OK)
		    {
		        /* Couldn't prompt for password, so reject connection
			** this falls through to default error handling below
			** Note we call check_password to do security
			** auditing/alarms appropriately.
			*/
	    		(VOID)scs_check_password(scb, NULL, 0, TRUE);
			ret_val=E_US18FF_BAD_USER_ID;
		    }
		    else
		    {
			/* Close the DB to prevent a potentially invalid
			** user from locking out the DB while we wait for
			** the password to be entered.
			*/
			if (sscb->sscb_flags & SCS_STAR)
			{
			    scs_ddbclose(scb);
                        }
			else
			{
                            dm_ccb = sscb->sscb_dmccb;
                            status = scs_dbclose(0, scb, &dberror, dm_ccb);
			    if (status)
			    {
                                ret_val = E_SC0123_SESSION_INITIATE;
                                sc0e_0_put(ret_val, 0);
                            }
			}

			if(status == E_DB_OK)
			{
			    /* Send prompt and  get reply */
		            *next_op = CS_EXCHANGE;
			    sscb->sscb_cfac = DB_CLF_ID;
			    return OK;
			}
		    }
	        }
		status = sc0m_allocate(0,
			    sizeof(SCC_GCMSG) + sizeof(GCA_ER_DATA)
				    + (ret_val ? ER_MAX_LEN : 0),
			    DB_SCF_ID,
			    (PTR)SCS_MEM,
			    SCG_TAG,
			    (PTR *) &message);
		if (status)
		{
		    sc0e_0_put(status, 0 );
		    sc0e_0_uput(status, 0 );
		    scd_note(E_DB_SEVERE, DB_SCF_ID);
		    message = (SCC_GCMSG *) cscb->cscb_inbuffer;
		}
		message->scg_mask = 0;
		message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
		message->scg_next = (SCC_GCMSG *) &cscb->cscb_mnext;
		message->scg_prev = cscb->cscb_mnext.scg_prev;
		cscb->cscb_mnext.scg_prev->scg_next = message;
		cscb->cscb_mnext.scg_prev = message;

		if (ret_val)
		{
		    GCA_ER_DATA	    *emsg;
		    GCA_DATA_VALUE  *gca_data_value;

		    emsg = (GCA_ER_DATA *)message->scg_marea;
		    gca_data_value = emsg->gca_e_element[0].gca_error_parm;

		    emsg->gca_l_e_element = 1;
		    emsg->gca_e_element[0].gca_id_server = Sc_main_cb->sc_pid;
		    emsg->gca_e_element[0].gca_severity = 0;
		    emsg->gca_e_element[0].gca_l_error_parm = 1;

            /* --- if the error code is hex 10 (E_US0010). --- */
            /* --- check the ret_val to see whether          --- */
	    /* --- it is E_US0010, if it is, call ulee_format --- */
	    /* --- and pass the <dbname> as one argument.     --- */

		    if (ret_val != 16 )
		      status = ule_format(ret_val, 0, ULE_LOOKUP, &sqlstate,
			gca_data_value->gca_value, ER_MAX_LEN,
			&gca_data_value->gca_l_value, &ule_error, 0);

                    else
		      status = ule_format(ret_val, 0, ULE_LOOKUP, &sqlstate,
			gca_data_value->gca_value, ER_MAX_LEN,
			&gca_data_value->gca_l_value, &ule_error, 1,
			sizeof(sscb->sscb_ics.ics_dbname.db_db_name),
			(PTR)sscb->sscb_ics.ics_dbname.db_db_name,
			0, (PTR)0,
			0, (PTR)0,
                        0, (PTR)0,
		        0, (PTR)0,
		        0, (PTR)0 );


		    gca_data_value->gca_type = DB_CHA_TYPE;

		    if (cscb->cscb_version <= GCA_PROTOCOL_LEVEL_2)
		    {
			emsg->gca_e_element[0].gca_id_error = ret_val;
			emsg->gca_e_element[0].gca_local_error = 0;
		    }
		    else if (cscb->cscb_version < GCA_PROTOCOL_LEVEL_60)
		    {
			emsg->gca_e_element[0].gca_local_error = ret_val;
			emsg->gca_e_element[0].gca_id_error =
			    ss_2_ge(sqlstate.db_sqlstate, ret_val);
		    }
		    else
		    {
			emsg->gca_e_element[0].gca_local_error = ret_val;
			emsg->gca_e_element[0].gca_id_error =
			    ss_encode(sqlstate.db_sqlstate);
		    }

		    *next_op = CS_OUTPUT;    /* die if can't start up */
		    sscb->sscb_state = SCS_TERMINATE;
		    message->scg_mtype = GCA_RELEASE;
		    message->scg_msize = sizeof(GCA_ER_DATA)
			    		    - sizeof(GCA_DATA_VALUE)
						+ sizeof(i4)
						+ sizeof(i4)
						+ sizeof(i4)
			    + gca_data_value->gca_l_value;
		}
		else
		{
		    /*
		    ** Send accept message
		    */
		    message->scg_mtype = GCA_ACCEPT;
		    message->scg_msize = 0;
		    scb->cs_scb.cs_mask |= CS_NOXACT_MASK;
		    *next_op = CS_EXCHANGE;
		}
		sscb->sscb_cfac = DB_CLF_ID;
		return(OK);
	    }
	    else
	    {
		TRdisplay(">>>>>>>>20C error 1: sscb_state = %x (%<%d.)\n",
			    sscb->sscb_state);
		sc0e_0_put(E_SC020C_INPUT_OUT_OF_SEQUENCE, 0);
		return(E_DB_ERROR);
	    }

	    break;

	case CS_OUTPUT:
	    *next_op = CS_OUTPUT;	    /* assumption is to continue */
		/* These take care of themselves below */
	    if (sscb->sscb_state == SCS_TERMINATE)
	    {
		*next_op = CS_TERMINATE;
		sscb->sscb_cfac = DB_CLF_ID;
		return(E_DB_OK);
	    }
	    break;

	case CS_TERMINATE:
	    /*
	    ** Coordinate termination with GCA unless there is not front
	    ** end process associated with this thread.
	    */
	    if ( sscb->sscb_state != SCS_TERMINATE &&
		 sscb->sscb_need_facilities & (1 << DB_GCF_ID) )
	    {
		/* RAAT msgs must be formatted differently than normal */
		if (IS_RAAT_REQUEST_MACRO(scb))
		{
		    /* Do the RAAT equivalent of the code that follows */
		    status = scs_raat_term_msg(scb, E_SC0206_CANNOT_PROCESS);
		}
		else
		{
		    /* FIXME: The following static works only because the GCA
		    ** message that is placed in the buffer 'b' is the same
		    ** for all threads using it. This is no excuse however,
		    ** and we should implement a correct solution ASAP. The
		    ** static is used right now because it will work and we
		    ** are trying to go FCS.
		    ** Two possible correct solutions are:
		    ** A) allocate the message
		    ** buffer from heap memory and have CS send and dealocate
		    ** the message (there must be something wrong here tho'.
		    ** B) allocate and initialize the message at server startup
		    ** and have all threads use the common message.
		    ** Of the two, I like A.
		    **	    eric 15 Dec 88
		    */
		    static char		b[ER_MAX_LEN + sizeof(GCA_ER_DATA) +
						sizeof(ALIGN_RESTRICT)-1];
		    GCA_ER_DATA		*ebuf;
		    GCA_DATA_VALUE	*gca_data_value;

		    /* If not asked by FE to die, better tell them */
		    ebuf = (GCA_ER_DATA *)ME_ALIGN_MACRO( &b, sizeof(ALIGN_RESTRICT));
		    ebuf->gca_l_e_element = 1;
		    if (cscb->cscb_version <= GCA_PROTOCOL_LEVEL_2)
		    {
			ebuf->gca_e_element[0].gca_local_error = 0;
			ebuf->gca_e_element[0].gca_id_error =
			    E_SC0206_CANNOT_PROCESS;
		    }
		    else if (cscb->cscb_version < GCA_PROTOCOL_LEVEL_60)
		    {
			ebuf->gca_e_element[0].gca_local_error =
			    E_SC0206_CANNOT_PROCESS;
			ebuf->gca_e_element[0].gca_id_error =
			    E_GE98BC_OTHER_ERROR;
		    }
		    else
		    {
			ebuf->gca_e_element[0].gca_local_error =
			    E_SC0206_CANNOT_PROCESS;
			ebuf->gca_e_element[0].gca_id_error =
			    ss_encode(SS50000_MISC_ERRORS);
		    }
		    ebuf->gca_e_element[0].gca_id_server = Sc_main_cb->sc_pid;
		    ebuf->gca_e_element[0].gca_severity = 0;
		    ebuf->gca_e_element[0].gca_l_error_parm = 1;

		    gca_data_value = ebuf->gca_e_element[0].gca_error_parm;

		    gca_data_value->gca_type = DB_CHA_TYPE;

		    status = ule_format(E_SC0206_CANNOT_PROCESS,
			0,
			ULE_LOOKUP,
			(DB_SQLSTATE *) NULL,
			gca_data_value->gca_value,
			ER_MAX_LEN,
			&gca_data_value->gca_l_value,
			&ule_error,
			0);

		    cscb->cscb_gcp.gca_sd_parms.gca_association_id =
					    cscb->cscb_assoc_id;
		    cscb->cscb_gcp.gca_sd_parms.gca_buffer = b;
		    cscb->cscb_gcp.gca_sd_parms.gca_message_type =
			GCA_RELEASE;
		    cscb->cscb_gcp.gca_sd_parms.gca_msg_length =
					 sizeof(GCA_ER_DATA)
						- sizeof(GCA_DATA_VALUE)
						  + sizeof(i4)
						  + sizeof(i4)
						  + sizeof(i4)
						  + gca_data_value->gca_l_value;
		    cscb->cscb_gcp.gca_sd_parms.gca_end_of_data = TRUE;
		    cscb->cscb_gcp.gca_sd_parms.gca_modifiers = 0;
		    status = IIGCa_call(GCA_SEND,
			   (GCA_PARMLIST *)&cscb->cscb_gcp.gca_sd_parms,
					GCA_NO_ASYNC_EXIT,
					(PTR)0,
					-1,
					&error);
		}
	    }
	    CSintr_ack();
	    ret_val = scs_terminate(scb);

	    /*
	    ** If the thread being terminated is necessary in order for the
	    ** server to function, then signal a fatal error and shut down.
	    */
	    if (((   (sscb->sscb_flags & (SCS_VITAL | SCS_PERMANENT))
				    == (SCS_VITAL | SCS_PERMANENT)
		 )
		  &&
		 (Sc_main_cb->sc_state != SC_SHUTDOWN))
		||
		/* 2PC recovery thread better have completed successfully */
		((sscb->sscb_flags & SCS_STAR)
		 &&
		 (sscb->sscb_stype == SCS_S2PC)
		 &&
		 (Sc_main_cb->sc_state != SC_OPERATIONAL))
	       )
	    {
		i4 session_count;
		sc0e_put(E_SC0241_VITAL_TASK_FAILURE, 0, 1,
			 sizeof(*sscb->sscb_ics.ics_eusername),
			 (PTR)sscb->sscb_ics.ics_eusername,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0);
		CSterminate(CS_KILL, &session_count);
		return (E_DB_FATAL);
	    }
	    sscb->sscb_cfac = DB_CLF_ID;
	    return(ret_val);
	    break;

	default:
	    sc0e_0_put(E_SC0024_INTERNAL_ERROR, 0);
	    sc0e_put(E_SC0205_BAD_SCS_OPERATION, 0, 1,
		     sizeof (op_code), &op_code,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_0_put(E_SC021F_BAD_SQNCR_CALL, 0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	    break;
    }

    if (sscb->sscb_stype == SCS_SMONITOR)
    {
	status = scs_msequencer(op_code, scb, next_op);
	sscb->sscb_cfac = DB_CLF_ID;
	return(status);
    }

    /*
    ** If query "language" is NativeDMF, pass it along
    ** to RAAT code:
    */
    if (IS_RAAT_REQUEST_MACRO(scb))
    {
#if defined(RAAT_SUPPORT)
	status = scs_raat_parse(scb);  /* concatenate GCA buffers */
	status = scs_raat_call(op_code, scb, next_op);
	sscb->sscb_cfac = DB_CLF_ID;
	return (status);
#else
        sc0e_0_put(E_SC0024_INTERNAL_ERROR, 0);
        sc0e_put(E_SC0393_RAAT_NOT_SUPPORTED, 0, 0,
            0, (PTR)0,
            0, (PTR)0,
            0, (PTR)0,
            0, (PTR)0,
            0, (PTR)0);
	return (E_DB_SEVERE);
#endif /* RAAT_SUPPORT */
    }

    /*
    ** Having reached this point, it is known that the query is to continue.
    ** Now, the action is to figure out what to do next.
    */


    if (sscb->sscb_qmode == PSQ_EXEDBP)
    {
	/*
	** stash away the proc id as we must return this ( or an updated
	** version ) when the procedure invocation completes
	*/
	STRUCT_ASSIGN_MACRO(cquery->cur_qname, proc_qid);

	if ((proc_qid.db_cursor_id[0] == 0) && (proc_qid.db_cursor_id[1] == 0))
	    recreate = 1;
    }

    xact_state = QEF_X_MACRO(sscb->sscb_qescb);
    qef_psf_alias.qso_n_dbid = -1;			/* Initialization */

    for (   internal_retry = 0;
	    sscb->sscb_noretries < SCS_RETRY_LIMIT;
	    sscb->sscb_noretries++)
				/* goes 'til "end of massive for loop" */
    {
	if (sscb->sscb_state == SCS_IERROR)
	{
	    /*
	    ** If we found an error during input processing (i.e. in
	    ** scs_input(), then don't bother to continue processing.
	    ** The error block will have already been queued, so we
	    ** just dump out here and allow nature to take its course.
	    */

	    qry_status = GCA_FAIL_MASK;
	    break;
	}

	/* Dont' allow execute procedure to continue for star */
	if ( (sscb->sscb_flags & SCS_STAR) &&
	      (sscb->sscb_qmode == PSQ_SECURE))
	{
	    char tmp_str[20];

	    STprintf(tmp_str,"PREPARE TO COMMIT");

	    sc0e_uput(E_US13FE_NO_STAR_SUPP, 0, 1,
		      STlength(tmp_str), (PTR)tmp_str,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    qry_status = GCA_FAIL_MASK;
	    break;
	}

	if ((internal_retry) && (sscb->sscb_noretries))
	{
	    /*
	    ** Release communications buffers only if not trying to
	    ** recreate a QP internally.  QEF may have already executed a
	    ** procedure and is trying to reload another one.  We do not
	    ** want to lose the messages of the parent procedure.
	    */

	    if (!qef_wants_ruleqp)
	        (VOID) scc_dispose(scb);
	    ps_ccb = sscb->sscb_psccb;
	    ps_ccb->psq_flag |= PSQ_REDO;   /* Let PSF know that we are in
					    ** a retry. New RDF protocol
					    ** eliminates PSQ_DELRNG call.
					    */
	    if (sscb->sscb_state == SCS_INPUT)
	    {
		/* then this was a repeat query which'll be resent */
		break;
	    }
	    sscb->sscb_state = SCS_CONTINUE;

	    /* If execute DBP retry, it's a recreate.  (This includes
	    ** the qef-wants retries.)  Make sure we do the parse/optimize
	    ** stuff below even though we leave qmode alone.
	    ** Other retry, we don't know, zero out qmode.
	    */
	    if (sscb->sscb_qmode == PSQ_EXEDBP)
		recreate = 1;
	    else
		sscb->sscb_qmode = 0;
	}
	else
	{
	    /* Make sure PSF retry flag is NOT on */
	    ps_ccb = sscb->sscb_psccb;
	    ps_ccb->psq_flag &= ~PSQ_REDO;
	}
	internal_retry = 1;

	/*
	** Determine if we want to parse:
	**	EXECUTE PROCEDURE recreation;
	**	Internally invoked procedure needed;
	**	Cursor DELETE statement;
	**	Regular query to parse;
	**	Parse EXECUTE IMMEDIATE for CREATE SCHEMA statement.
	*/
	if (   (sscb->sscb_qmode == PSQ_EXEDBP && recreate)
	    || (qef_wants_ruleqp)
	    || (qef_wants_eiqp)
	    || (   sscb->sscb_state == SCS_CONTINUE
		&& (   (sscb->sscb_qmode == 0)
		    || (sscb->sscb_qmode == PSQ_DELCURS)
		   )
	       )
	   )
	{
	    QEU_CB		qeu;

	    /*
	    ** then the mode is unknown, and it will
	    ** be necessary to parse [& optimize]
	    */

	    /* First, we start an internal xact to manage the work */

	    qeu.qeu_eflag = QEF_EXTERNAL;
	    qeu.qeu_db_id = (PTR) sscb->sscb_ics.ics_opendb_id;
	    qeu.qeu_d_id = scb->cs_scb.cs_self;

	    if (   (sscb->sscb_qmode == PSQ_EXEDBP && recreate)
		|| (qef_wants_ruleqp))
	    {
		/*
		** if we are about to recreate a dbproc, we may not start a
		** read-only xact since as a part of recreating dbprocs we may
		** need to insert tuples describing dbproc's dependency
		** information into IIPRIV and IIDBDEPENDS
		*/
		/*
		** Bug 99199:
	        ** Setting qeu.qeu_flag to 0 caused transaction state to change
		** before the first statement of a db procedure had been
		** executed.  For example, when the transaction state is 0
		** immediately before the execution of the db procedure and
		** the first statement in the db procedure is
		** dbmsinfo(transaction_state), the transaction state is
		** reported back as 1. The change implemented here is to
		** set qeu.qeu_flag to a new value called QEF_DBPROC. Then
		** in qef_btran(), the flag is checked and if the value is
		** QEF_DBPROC when qef_stat is QEF_NOTRAN, then qef_modifier
	 	** is set to QEF_ITRAN instead of QEF_SSTRAN, and qeu_flag
		** is then set to 0 at that point instead of here.
		*/
		qeu.qeu_flag = QEF_DBPROC;
	    }
	    else
	    {
		qeu.qeu_flag = QEF_READONLY;
	    }

            qeu.qeu_mask = 0;
	    qeu.qeu_qmode = sscb->sscb_qmode;
	    status = qef_call(QEU_BTRAN, &qeu);
	    if (ret_val = status)
	    {
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		sc0e_0_put(qeu.error.err_code, 0);
		qry_status = GCA_FAIL_MASK;
		if (cscb->cscb_batch_count > 0)
		    cscb->cscb_eog_error = TRUE;
		break;
	    }

	    ps_ccb = sscb->sscb_psccb;
	    ps_ccb->psq_flag &= ~PSQ_IIQRYTEXT; /*psq_iiqrytext = FALSE */

	    /* We do not want to stamp on the query text handle for
	    ** for the original query text of CREATE SCHEMA/CREATE TABLE
	    ** (with with constraints)/CREATE VIEW(WCO).
	    ** Otherwise the original query text is not destroyed.
	    */
	    if (!qef_wants_eiqp)
	       ps_ccb->psq_qid = sscb->sscb_thandle;
	    else
	       ps_ccb->psq_qid = qe_ccb->qef_qsf_handle.qso_handle;

	    ps_ccb->psq_qlang = sscb->sscb_ics.ics_qlang;
	    ps_ccb->psq_flag &= ~PSQ_ALLDELUPD;	    /* psq_alldelupd = false */

	    ps_ccb->psq_flag &= ~PSQ_ALIAS_SET;	    /*
						    ** make sure that this bit
						    ** is reset;  when calling
						    ** to recreate a dbproc,
						    ** this bit gets set, and
						    ** not resetting it may
						    ** result in users being
						    ** able to execute dbprocs
						    ** for which they have no
						    ** permissions
						    */

            /* Copy over PST_INFO stuff from QEF_RCB to PSQ_CB */

            if (qef_wants_eiqp)
            {
                qe_ccb = sscb->sscb_qeccb;
		ps_ccb->psq_info = (PST_INFO *)qe_ccb->qef_info;
            }
            else
            {
                ps_ccb->psq_info = (PST_INFO *) NULL;
            }

	    sscb->sscb_adscb->adf_qlang = sscb->sscb_ics.ics_qlang;
	    sscb->sscb_cfac = DB_PSF_ID;

	    /* STAR addition */
	    if (sscb->sscb_flags & SCS_STAR)
	    {
		ps_ccb->psq_dfltldb = &sscb->sscb_dbcb->
			db_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc;
		ps_ccb->psq_ldbdesc =
		    &sscb->sscb_qeccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c1_ldb;
	    }

	    /* Regular query parsing, cursor DELETE, procedure recreation or
	    ** E.I. text parsing?
	    */
	    if (sscb->sscb_qmode == 0 && !qef_wants_ruleqp)
	    {
		if(!qef_wants_eiqp && (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
		{
			PSQ_QDESC *qdesc;
			/*
			** Security audit query text. This is done here
			** so we only audit real user input, not things like
			** DBP recreation or EI productions from create
			** schema which are implicitly performed by the
			** DBMS.
			*/
			qdesc = (PSQ_QDESC*)sscb->sscb_troot;
			/*
			** Main query text
			*/
			sc0a_qrytext(scb, qdesc->psq_qrytext,
					  qdesc->psq_qrysize-1,
					  SC0A_QT_START);
			if (qdesc->psq_dnum)
			{
			    sc0a_qrytext_data(scb,
					qdesc->psq_dnum,
					qdesc->psq_qrydata);
			}
			sc0a_qrytext(scb,"",0,SC0A_QT_END);
		}
		if (cscb->cscb_batch_count > 0)
		    ps_ccb->psq_flag2 |= PSQ_BATCH;
		if (Sc_main_cb->sc_batch_copy_optim)
		    ps_ccb->psq_flag2 |= PSQ_COPY_OPTIM;
		if (cscb->cscb_gci.gca_end_of_group &&
			ps_ccb->psq_flag2 & PSQ_BATCH)
		{
		    ps_ccb->psq_flag2 |= PSQ_LAST_BATCH;
		}
		if (sscb->sscb_cpy_qeccb)
		    ps_ccb->psq_cp_qefrcb = sscb->sscb_cpy_qeccb;
		else
		    ps_ccb->psq_cp_qefrcb = NULL;

		if (sscb->sscb_troot && ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrysize == 0)
		{
		    /*
		    ** regular query with no query text, let's see if we can handle
		    ** it. SCS_REPEAT_QUERY is a sanity check to make sure the client
		    ** really intended this.
		    */
		    if ((sscb->sscb_req_flags & SCS_REPEAT_QUERY) == 0 ||
			    sscb->sscb_cquery.cur_qtxt_len == 0 ||
			    sscb->sscb_cquery.cur_qtxt == NULL)
		    {
			/* no previous query */
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sc0e_0_put(E_SC02A0_NO_QUERY_TEXT, 0);
			qry_status = GCA_FAIL_MASK;
			break;
		    }
		    if (sscb->sscb_cquery.cur_qtxt_len
			    >= sscb->sscb_tnleft)
		    {
			/* not enough space left */
			qs_ccb = sscb->sscb_qsccb;
			qs_ccb->qsf_sz_piece =
				sscb->sscb_cquery.cur_qtxt_len + 1;
			status = qsf_call(QSO_PALLOC, qs_ccb);
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC021C_SCS_INPUT_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    qry_status = GCA_FAIL_MASK;
			    break;
			}
			((PSQ_QDESC *) sscb->sscb_troot)->psq_qrytext =
				qs_ccb->qsf_piece;
			sscb->sscb_tsize =
				sscb->sscb_tnleft = qs_ccb->qsf_sz_piece;
		    }
		    /*
		    ** Sanity check; make sure that GCA gave us the right
		    ** number of parameters for this query. If we got the query
		    ** text, this would be checked in the parser.
		    */
		    if (scb->scb_sscb.sscb_cquery.cur_qry_parms !=
			    ((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_dnum)
		    {
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sc0e_put(E_SC02A1_INCORRECT_QUERY_PARMS, 0, 2,
				 sizeof(((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_dnum), 
				 (PTR)&((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_dnum,
				 sizeof(scb->scb_sscb.sscb_cquery.cur_qry_parms), 
				 (PTR)&scb->scb_sscb.sscb_cquery.cur_qry_parms,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0);
			qry_status = GCA_FAIL_MASK;
			break;
		    }
		    /* copy the text in */
		    MEcopy(
			    sscb->sscb_cquery.cur_qtxt + sizeof(SC0M_OBJECT),
			    sscb->sscb_cquery.cur_qtxt_len,
			    ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrytext);
		    ((PSQ_QDESC *)sscb->sscb_troot)->psq_qrysize =
			    sscb->sscb_cquery.cur_qtxt_len;
		    sscb->sscb_tptr =
			    ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrytext
			    + ((PSQ_QDESC *)sscb->sscb_troot)->psq_qrysize;
			sscb->sscb_tnleft -=
				    ((PSQ_QDESC *)
					    sscb->sscb_troot)->psq_qrysize;
		    if (sscb->sscb_flags & SCS_INSERT_OPTIM)
			notext_copy_optim = TRUE;
		}
		else if (scb->scb_sscb.sscb_troot)
		    /* we got query text, so save number of parms for sanity check */
		    scb->scb_sscb.sscb_cquery.cur_qry_parms = 
			((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_dnum;

		if (notext_copy_optim)
		{
		    status = psq_call(PSQ_COPYBUF, ps_ccb, sscb->sscb_psscb);
		    if (!cscb->cscb_gci.gca_end_of_group)
			ps_ccb->psq_ret_flag |= PSQ_CONTINUE_COPY;
		    else
			ps_ccb->psq_ret_flag |= (PSQ_CONTINUE_COPY | PSQ_FINISH_COPY);
		}
		else
		    status = psq_call(PSQ_PARSEQRY, ps_ccb, sscb->sscb_psscb);
		if (ps_ccb->psq_ret_flag &
			    (PSQ_INSERT_TO_COPY | PSQ_CONTINUE_COPY))
		    sscb->sscb_flags |= SCS_INSERT_OPTIM;
		else
		    sscb->sscb_flags &= ~SCS_INSERT_OPTIM;

		/* save copy QEF_RCB */
		if (sscb->sscb_cpy_qeccb == NULL &&
			ps_ccb->psq_cp_qefrcb)
		    sscb->sscb_cpy_qeccb = ps_ccb->psq_cp_qefrcb;
	    }
	    else if (sscb->sscb_qmode == PSQ_DELCURS)
	    {
		/* Use full client-specified cursor id */
		STRUCT_ASSIGN_MACRO(cquery->cur_qname, ps_ccb->psq_cursid);
		status = psq_call(PSQ_DELCURS, ps_ccb,sscb->sscb_psscb);
	    }
	    else	/* Must be recreation - internal (QEF) or user */
	    {
		if (qef_wants_ruleqp)	/* Internal */
		{
		    /*
		    ** Indicate that a full procedure alias is there and
		    ** copy the alias that was saved when qef_wants_ruleqp was
		    ** originally set.
		    */
		    ps_ccb->psq_flag |= PSQ_ALIAS_SET;	/*psq_alias_set = true*/
		    qef_psf_alias.qso_n_id.db_cursor_id[0] = 0;
		    qef_psf_alias.qso_n_id.db_cursor_id[1] = 0;
		    STRUCT_ASSIGN_MACRO(qef_psf_alias.qso_n_id,
							ps_ccb->psq_cursid);
		    STRUCT_ASSIGN_MACRO(qef_psf_alias.qso_n_own,
							ps_ccb->psq_als_owner);

		    /*
		    ** copy the id of the set-input procedure and the id of
		    ** temp table representing the set-input parameter and
		    ** (if this is not a set-input procedure, these fields
		    **  will not be looked at by PSF)
		    */
		    STRUCT_ASSIGN_MACRO(qe_ccb->qef_dbpId,
					ps_ccb->psq_set_procid);
		    STRUCT_ASSIGN_MACRO(qe_ccb->qef_setInputId,
					ps_ccb->psq_set_input_tabid);
		}
		else			/* User invoked */
		{
		    /* Use original user-specified procedure name (no owner) */
		    cquery->cur_qname.db_cursor_id[0] = 0;
		    cquery->cur_qname.db_cursor_id[1] = 0;
		    STRUCT_ASSIGN_MACRO(cquery->cur_qname, ps_ccb->psq_cursid);

		    /*
		    ** need to initialize ps_ccb->psq_set_input_tabid to 0
		    ** because psq_recreate() examines this field to detrrmine
		    ** whether this is a set-input procedure; zero out
		    ** ps_ccb->psq_set_procid just because I don't like
		    ** uninitialized variables
		    */
		    ps_ccb->psq_set_input_tabid.db_tab_base =
			ps_ccb->psq_set_input_tabid.db_tab_index =
			ps_ccb->psq_set_procid.db_tab_base =
			ps_ccb->psq_set_procid.db_tab_index = 0;
		}
		status = psq_call(PSQ_RECREATE, ps_ccb, sscb->sscb_psscb);
	    }
	    sscb->sscb_cfac = DB_SCF_ID;
	    if (DB_FAILURE_MACRO(status))
	    {
		/* got an error */

		switch (ps_ccb->psq_error.err_code)
		{
		    case E_PS0001_USER_ERROR:
			qry_status = GCA_FAIL_MASK;
			if (sscb->sscb_flags & SCS_ON_USRERR_ROLLBACK)
			{
			    on_user_error = TRUE;
			    /* rollback means end of group processing */
			    if (cscb->cscb_batch_count > 0)
				cscb->cscb_eog_error = TRUE;
			}
			break;

		    case E_PS0003_INTERRUPTED:
			break;

		    case E_PS0009_DEADLOCK:
			sc0e_0_put(ps_ccb->psq_error.err_code, 0);
			sc0e_0_uput(ps_ccb->psq_error.err_code, 0);
			sc0e_0_uput(4700, 0);
			deadlock_occurred = 1;
			qry_status = GCA_FAIL_MASK;
			if (cscb->cscb_batch_count > 0)
			    cscb->cscb_eog_error = TRUE;
			break;

		    case E_PS0010_TIMEOUT:
			sc0e_0_put(ps_ccb->psq_error.err_code, 0);
			sc0e_0_uput(ps_ccb->psq_error.err_code, 0);
			qry_status = GCA_FAIL_MASK;
			break;

		    case E_PS0702_NO_MEM:
		    case E_PS0F01_CACHE_FULL:
		    case E_PS0F02_MEMORY_FULL:
		    case E_QS0001_NOMEM:
			sc0e_0_uput(ps_ccb->psq_error.err_code, 0);
			sc0e_0_put(ps_ccb->psq_error.err_code, 0);
			sc0e_0_uput(E_SC022A_SERVER_LOAD, 0);
			if (cscb->cscb_batch_count > 0)
			    cscb->cscb_eog_error = TRUE;
			qry_status = GCA_FAIL_MASK;
			break;

		    case E_PS0005_USER_MUST_ABORT:
			sc0e_0_put(ps_ccb->psq_error.err_code, 0);
			sc0e_0_uput(ps_ccb->psq_error.err_code, 0);
			if (!sscb->sscb_interrupt)
			    sscb->sscb_force_abort = SCS_FAPENDING;
			if (cscb->cscb_batch_count > 0)
			    cscb->cscb_eog_error = TRUE;
			qry_status = GCA_FAIL_MASK;
			break;

		    case E_PS0008_RETRY:
			/*
			** free the object which is in qsf that
			** psq_result points to.
			*/
			if (ps_ccb->psq_result.qso_handle != 0)
			{
			    qs_ccb = sscb->sscb_qsccb;
			    STRUCT_ASSIGN_MACRO(ps_ccb->psq_result,
							qs_ccb->qsf_obj_id);
			    qs_ccb->qsf_lk_state = QSO_SHLOCK;
			    status = qsf_call(QSO_LOCK, qs_ccb);
			    if (DB_FAILURE_MACRO(status))
			    {
				scs_qsf_error(status,
					qs_ccb->qsf_error.err_code,
					    E_SC0210_SCS_SQNCR_ERROR);
			    }
			    else
			    {
				status = qsf_call(QSO_DESTROY, qs_ccb);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
					    qs_ccb->qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
				}
			    }
			}

			/* Now destroy the object in psq_txtout */
			if (ps_ccb->psq_txtout.qso_handle != 0)
			{
			    qs_ccb = sscb->sscb_qsccb;
			    STRUCT_ASSIGN_MACRO(ps_ccb->psq_txtout,
						    qs_ccb->qsf_obj_id);
			    qs_ccb->qsf_lk_state = QSO_SHLOCK;
			    status = qsf_call(QSO_LOCK, qs_ccb);
			    if (DB_FAILURE_MACRO(status))
			    {
				scs_qsf_error(status,
					qs_ccb->qsf_error.err_code,
					    E_SC0210_SCS_SQNCR_ERROR);
			    }
			    else
			    {
				status = qsf_call(QSO_DESTROY, qs_ccb);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
					    qs_ccb->qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
				}
			    }
			}

			/* Mark that the qsf objects have been destroyed and
			** try parsing and executing the query again.
			*/
			qsf_object = 0;
			continue;

		    default:
			sc0e_0_put(E_SC0207_UNEXPECTED_ERROR, 0);
			/* Fall thru to other unhandle-able errors */

		    case E_PS0006_UNKNOWN_OPERATION:
		    case E_PS0205_SRV_NOT_INIT:
		    case E_PS0201_BAD_QLANG:
		    case E_PS0202_QLANG_NOT_ALLOWED:
		    case E_PS0203_NO_DECIMAL:
		    case E_PS0204_BAD_DISTRIB:
		    case E_PS0208_TOO_MANY_SESS:
		    case E_PS0601_CURSOR_OPEN:
		    case E_PS0002_INTERNAL_ERROR:
		    case E_PS0007_INT_OTHER_FAC_ERR:
		    case E_PS0101_BADERRTYPE:
		    case E_PS0B04_CANT_GET_TEXT:
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sc0e_0_put(ps_ccb->psq_error.err_code, 0);
			sc0e_0_put(E_SC0215_PSF_ERROR, 0);
			if ((status == E_DB_FATAL) || (status == E_DB_SEVERE) ||
			    (ps_ccb->psq_error.err_code != E_PS0002_INTERNAL_ERROR))
			{
			    scd_note(status, DB_PSF_ID);
			}
			if (cscb->cscb_batch_count > 0)
			    cscb->cscb_eog_error = TRUE;
			qry_status = GCA_FAIL_MASK;
			break;
		}
		(VOID) qef_call(QEU_ETRAN, &qeu); /* if interrupt   */
						    /* requires abort,	    */
						    /* it'll be handled	    */
						    /* below		    */
		break;
	    }
	    else if ((sscb->sscb_state == SCS_CONTINUE) &&
			(sscb->sscb_qmode != 0) &&
			(sscb->sscb_qmode != PSQ_EXEDBP) &&
			(sscb->sscb_qmode != PSQ_DELCURS)
		    )
	    {
		TRdisplay(">>>>>>>>20C error 2: sscb_state = %x (%<%d.), qmode = %d.\n",
			    sscb->sscb_state, sscb->sscb_qmode );
		sc0e_0_put(E_SC020C_INPUT_OUT_OF_SEQUENCE, 0);
		sc0e_0_uput(E_SC020C_INPUT_OUT_OF_SEQUENCE, 0);
		if (cscb->cscb_batch_count > 0)
		    cscb->cscb_eog_error = TRUE;
		qry_status = GCA_FAIL_MASK;
		scd_note(E_DB_SEVERE, DB_SCF_ID);
		break;
	    }
	    if (ps_ccb->psq_ret_flag &
		    (PSQ_INSERT_TO_COPY | PSQ_CONTINUE_COPY | PSQ_FINISH_COPY))
		(VOID) qef_call(QEU_ETRAN, &qeu);

	    if (ps_ccb->psq_ret_flag & PSQ_INSERT_TO_COPY)
	    {
		/* 
		** nothing to do here, we have delayed copy
		** start until we have a buffer full of rows
		** to load
		*/
		/* we already have a row, process it */
		ps_ccb->psq_ret_flag |= PSQ_CONTINUE_COPY;
	    }
	    
	    if (ps_ccb->psq_ret_flag & PSQ_CONTINUE_COPY)
	    {
		qe_ccb = (QEF_RCB *)sscb->sscb_cpy_qeccb;
		qe_copy = qe_ccb->qeu_copy;
		/*
		** check if we need to load the data. This can be:
		** 1) we are finished with the batch, or:
		** 2) PSF asked us to load because there may not be enough
		**    space left in the buffer for the next row, or:
		** 3) This is not a small batch and we already reached the
		**    optimum number of buffered rows for DMF
		*/
		if (ps_ccb->psq_ret_flag & PSQ_FINISH_COPY ||
			qe_copy->qeu_load_buf ||
			((sscb->sscb_flags & SCS_COPY_STARTED) &&
				(qe_copy->qeu_stat & CPY_SMALL) == 0 && 
				qe_copy->qeu_cur_tups >= BEST_LOAD_TUPS ))
		{
		    if ((sscb->sscb_flags & SCS_COPY_STARTED) == 0)
		    {
			/* start copy */
			qe_ccb->qef_cb = sscb->sscb_qescb;
			qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
			qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;
			qe_ccb->qef_eflag = QEF_INTERNAL;
    
			/*
			** Initialize some of the copy control block fields.
			*/
			qe_copy->qeu_sbuf = NULL;
			qe_copy->qeu_sptr = NULL;
			qe_copy->qeu_ssize = 0;
			qe_copy->qeu_sused = 0;
			qe_copy->qeu_partial_tup = FALSE;
			qe_copy->qeu_error = FALSE;
    
			qe_copy->qeu_uputerr[0] = 0;
			qe_copy->qeu_uputerr[1] = 0;
			qe_copy->qeu_uputerr[2] = 0;
			qe_copy->qeu_uputerr[3] = 0;
			qe_copy->qeu_uputerr[4] = 0;
			qe_copy->qeu_stat = CPY_INS_OPTIM;
			/*
			** Call QEF to initialize COPY processing.  This will begin
			** a single statement transaction and prepare for reading
			** or writing of tuples.
			*/
			if (ps_ccb->psq_ret_flag & PSQ_FINISH_COPY)
			{
			    /*
			    ** we want to finish the copy, but we are only just
			    ** about to start it, which means there is less than
			    ** one buffer full of rows. these very small numbers
			    ** of rows require special consideration in the copy
			    ** code, so we will set a flag to allow copy to
			    ** make smarter decisions based on the fact that
			    ** the copy size is very small
			    */
			    qe_copy->qeu_stat |= CPY_SMALL;
			}
			status = qef_call(QEU_B_COPY, qe_ccb);
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qef_error(status, qe_ccb->error.err_code,
						E_SC0210_SCS_SQNCR_ERROR, scb);
    
			    qry_status = GCA_FAIL_MASK;
			    sscb->sscb_state = SCS_INPUT;
			    *next_op = CS_EXCHANGE;
			    /* this is a terminate batch error */
			    sscb->sscb_flags &= ~SCS_INSERT_OPTIM;
			    cscb->cscb_eog_error = TRUE;
			    break;
			}
			sscb->sscb_flags |= SCS_COPY_STARTED;
		    }
		    /* load the data */
		    qe_copy->qeu_stat |= CPY_INS_OPTIM;
		    status = qef_call(QEU_R_COPY, qe_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			qe_copy->qeu_stat |= CPY_FAIL;
			/* close copy and rollback rows */
			scs_copy_error(scb, qe_ccb, 0);
			sscb->sscb_state = SCS_CP_ERROR;
			/* block sent via NO_ASYNC_EXIT */
			sscb->sscb_cfac = DB_CLF_ID;
			qry_status = GCA_FAIL_MASK;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			/* this is a terminate batch error */
			cscb->cscb_eog_error = TRUE;
			sscb->sscb_flags &= ~SCS_COPY_STARTED;
			break;
		    }
		    /* and re-set the buffer */
		    qe_copy->qeu_cleft = qe_copy->qeu_csize;
		    qe_copy->qeu_cptr = qe_copy->qeu_cbuf;
		    qe_copy->qeu_cur_tups = 0;
		    qe_copy->qeu_input = NULL;
		    qe_copy->qeu_insert_data = NULL;
		    qe_copy->qeu_load_buf = FALSE;
		}
		cquery->cur_row_count = 1;
		if ((ps_ccb->psq_ret_flag & PSQ_FINISH_COPY) == 0)
		{
		    /* clean up PSF objects */
		    qs_ccb = (QSF_RCB *) scb->scb_sscb.sscb_qsccb;
		    qs_ccb->qsf_obj_id.qso_handle =
			    ps_ccb->psq_result.qso_handle;
		    qs_ccb->qsf_lk_id = ps_ccb->psq_qso_lkid;
		    status = qsf_call(QSO_DESTROY, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				      E_SC0210_SCS_SQNCR_ERROR);
		    }		
		    *next_op = CS_INPUT;
		    sscb->sscb_state = SCS_INPUT;
		    break;
		}
	    }
	    
	    if (ps_ccb->psq_ret_flag & PSQ_FINISH_COPY)
	    {
		/* finish copy */
		qe_ccb = (QEF_RCB *)sscb->sscb_cpy_qeccb;
		qe_copy = qe_ccb->qeu_copy;
		qe_copy->qeu_stat = CPY_OK;
		/*
		** check if we have any leftover rows to load
		*/
		if (qe_copy->qeu_cur_tups > 0)
		{
		    /* load rows in the current buffer */
		    status = qef_call(QEU_R_COPY, qe_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			qe_copy->qeu_stat |= CPY_FAIL;
			/* close copy and rollback rows */
			(void)qef_call(QEU_E_COPY, qe_ccb);
			scs_copy_error(scb, qe_ccb, 0);
			sscb->sscb_state = SCS_CP_ERROR;
			/* block sent via NO_ASYNC_EXIT */
			sscb->sscb_cfac = DB_CLF_ID;
			qry_status = GCA_FAIL_MASK;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			/* this is a terminate batch error */
			cscb->cscb_eog_error = TRUE;
			break;
		    }
		    /* and re-set the buffer */
		    qe_copy->qeu_cleft = qe_copy->qeu_csize;
		    qe_copy->qeu_cptr = qe_copy->qeu_cbuf;
		    qe_copy->qeu_cur_tups = 0;
		    qe_copy->qeu_input = NULL;		    
		}
		status = qef_call(QEU_E_COPY, qe_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    scs_qef_error(status, qe_ccb->error.err_code,
				  E_SC0210_SCS_SQNCR_ERROR, scb);
		    sc0e_0_put(E_SC0251_COPY_CLOSE_ERROR, 0);
		    qe_copy->qeu_stat |= CPY_FAIL;
		    sscb->sscb_state = SCS_CP_ERROR;
		    sscb->sscb_cfac = DB_CLF_ID;
		    qry_status = GCA_FAIL_MASK;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    /* this is a terminate batch error */
		    sscb->sscb_flags &= ~SCS_INSERT_OPTIM;
		    cscb->cscb_eog_error = TRUE;
		    sscb->sscb_flags &= ~SCS_COPY_STARTED;
		    break;
		}
		sscb->sscb_flags &= ~SCS_COPY_STARTED;

		if (ps_ccb->psq_ret_flag & PSQ_CONTINUE_COPY)
		{
		    /* then the current query was the same as the last, so we're done */
		    /* clean up PSF objects */
		    qs_ccb = sscb->sscb_qsccb;
		    qs_ccb->qsf_obj_id.qso_handle =
			    ps_ccb->psq_result.qso_handle;
		    qs_ccb->qsf_lk_id = ps_ccb->psq_qso_lkid;
		    status = qsf_call(QSO_DESTROY, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				      E_SC0210_SCS_SQNCR_ERROR);
		    }

		    *next_op = CS_INPUT;
		    sscb->sscb_state = SCS_INPUT;
		    break;
		}
	    }

	    STRUCT_ASSIGN_MACRO(ps_ccb->psq_cursid, cquery->cur_qname);

	    /* Is this a procedure recreation case ? */
	    if (sscb->sscb_qmode == PSQ_EXEDBP)
	    {
		qmode = sscb->sscb_qmode = PSQ_EXEDBP;
		if (!procid_updated)
		{
		    if (MEcmp((PTR)&proc_qid.db_cur_name,
			      (PTR)&ps_ccb->psq_cursid.db_cur_name,
			      sizeof(proc_qid.db_cur_name)) == 0)
		    {
			STRUCT_ASSIGN_MACRO(ps_ccb->psq_cursid, proc_qid);
			procid_updated = TRUE;
		    }
		}

	    }
	    else if ((qef_wants_ruleqp) ||
		     (((sscb->sscb_qmode == PSQ_CREDBP) ||
		       (sscb->sscb_qmode == PSQ_CRESETDBP)) &&
		      (ps_ccb->psq_mode == PSQ_QRYDEFED)))
	    {
		qmode = sscb->sscb_qmode = PSQ_EXEDBP;
	    }
	    else
	    {
		qmode = sscb->sscb_qmode = ps_ccb->psq_mode;
	    }

	    /* Bug 72504 - set current query mode in QEU_CB */
	    qeu.qeu_qmode = sscb->sscb_qmode;
	    if (qmode == PSQ_REGPROC)
		qmode = PSQ_EXEDBP;

	    if (ps_ccb->psq_flag & PSQ_ALLDELUPD)
	    {
		cquery->cur_result = GCA_ALLUPD_MASK;
	    }
	    else
	    {
		cquery->cur_result = 0;
	    }
	    psf_stuff = 1;
	    STRUCT_ASSIGN_MACRO(ps_ccb->psq_result, psq_tree);
	    if (ps_ccb->psq_mode == PSQ_DEFQRY)
	    {
		psf_stuff = 2;
		STRUCT_ASSIGN_MACRO(ps_ccb->psq_cursid, psq_plan);
	    }
	    else if ((  (ps_ccb->psq_mode == PSQ_CREDBP)
		      || (ps_ccb->psq_mode == PSQ_CRESETDBP))
		     && (sscb->sscb_qmode != PSQ_EXEDBP))
	    {
		psf_tstuff = 1;
		STRUCT_ASSIGN_MACRO(ps_ccb->psq_txtout, psq_text);
	    }

	    if (    (ps_ccb->psq_mode != PSQ_QRYDEFED) &&
		    (sqncr_tbl[qmode].sq_mask & SQ_OPF_MASK) &&
			(!sscb->sscb_interrupt))
	    {
		/* then invoke the optimizer */
		op_ccb = sscb->sscb_opccb;
		STRUCT_ASSIGN_MACRO(ps_ccb->psq_result, op_ccb->opf_query_tree);
		op_ccb->opf_scb = (PTR *) sscb->sscb_opscb;
		if (qmode == PSQ_REPCURS || qmode == PSQ_DELCURS)
		{
                    i = scs_csr_find(scb);
		    if (i == sscb->sscb_cursor.curs_limit)
		    {
			qry_status = GCA_FAIL_MASK;
			sc0e_0_uput(E_SC021E_CURSOR_NOT_FOUND, 0);
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			(VOID) qef_call(QEU_ETRAN, &qeu);/* Done with */
							    /* query planning */
			break;
		    }
                    csi = sscb->sscb_cursor.curs_csi;
		    op_ccb->opf_parent_qep.qso_handle = csi->csitbl[i].csi_qp;
		}
		sscb->sscb_cfac = DB_OPF_ID;

		/* Turn on suboptimization bit if previous QEF_ZEROTUP_CHK */
		if (cquery->cur_subopt)
		    op_ccb->opf_smask |= OPF_SUBOPTIMIZE;

		/* Call the optimizer */

	 	/* We do not want to stamp on the handle for the
		** original query text and so the handle is not
		** assigned to sscb_thandle for E.I. processing of
		** CREATE SCHEMA, CREATE TABLE with constraints,
		** CREATE VIEW(WCO).
		*/

		if (!qef_wants_eiqp)
		   op_ccb->opf_thandle = sscb->sscb_thandle;
		else
		   op_ccb->opf_thandle = qe_ccb->qef_qsf_handle.qso_handle;

		IIEXtry
		{
		   status = opf_call(OPF_CREATE_QEP, op_ccb);
		}
		IIEXcatch(pthread_stackovf_e)
 		{
		     status = E_DB_ERROR;
		     op_ccb->opf_errorblock.err_code = E_OP009B_STACK_OVERFLOW;

		     scs_iformat(scb, 1, 0, 1);
		}
		IIEXendtry

		/* Turn off suboptimization bit */
		op_ccb->opf_smask &= ~(OPF_SUBOPTIMIZE | OPF_NOAGGCOMBINE);
		cquery->cur_subopt = FALSE;

		sscb->sscb_nostatements += 1;
		psf_stuff = 0;
		sscb->sscb_cfac = DB_SCF_ID;
		(VOID) qef_call(QEU_ETRAN, &qeu); /* Done with query */
						    /* planning.  If <abort> */
						    /* required by	    */
						    /* interrupt, it'll be  */
						    /* handled below */
		/* Reset this field as OPF likes to trash it! */
		sscb->sscb_adscb->adf_qlang = sscb->sscb_ics.ics_qlang;

		if (sscb->sscb_interrupt)
		    break;

		if (DB_FAILURE_MACRO(status))
		{
		    /*
		    ** Process the error.  This requires freeing any memory
		    ** consumed so far as well as notifying the parser that
		    ** it should do the same if the query mode requires it
		    ** (e.g. PSQ_DEFCURS requires notification since the parser
		    ** will have a saved query control block around.
		    ** (Will PSQ_REPLCURS errors close the cursor?  I think
		    ** not.)
		    */
		    scd_note(status, DB_OPF_ID);
		    if (qmode == PSQ_DEFCURS)
		    {
			/*
			** In this case, it is necessary to tell the parser to remove
			** whatever information it has around from it's cache.
			*/

			ps_ccb = sscb->sscb_psccb;
			ps_ccb->psq_flag &= ~PSQ_CLSALL; /*psq_clsall=FALSE*/
			status = psq_call(PSQ_CLSCURS, ps_ccb,
					    sscb->sscb_psscb);
			if (DB_FAILURE_MACRO(status))
			{
				sc0e_0_put(E_SC0215_PSF_ERROR, 0);
				sc0e_0_put(ps_ccb->psq_error.err_code, 0);
				sc0e_0_uput(ps_ccb->psq_error.err_code, 0);
				scd_note(status, DB_PSF_ID);
			}
		    }
                    if (op_ccb->opf_errorblock.err_code == E_OP008F_RDF_MISMATCH
                        ||
                        op_ccb->opf_errorblock.err_code == E_OP000D_RETRY)
		    {
			/* then this is a retry candidate */
			continue;
		    }
#ifdef E_OP000A_RETRY
		    if (op_ccb->opf_errorblock.err_code
				    == E_OP000A_RETRY)
		    {
			/* retry without flattening */
			/* If you have retried more than once, disable
			   combining aggregates as this may also invalidate
			   query plans */
                        if (sscb->sscb_noretries > 2)
                           op_ccb->opf_smask |= OPF_NOAGGCOMBINE;
			cquery->cur_subopt = TRUE;
			continue;
		    }
#endif
		    op_ccb->opf_repeat = (PTR *) 0;
		    if (op_ccb->opf_errorblock.err_code
				    == E_OP0010_RDF_DEADLOCK)
		    {
			sc0e_0_put(op_ccb->opf_errorblock.err_code, 0);
			sc0e_0_uput(op_ccb->opf_errorblock.err_code, 0);
			sc0e_0_uput(4700, 0);
			deadlock_occurred = 1;
			if (cscb->cscb_batch_count > 0)
			    cscb->cscb_eog_error = TRUE;
		    }
		    else if ((op_ccb->opf_errorblock.err_code !=
		    					E_OP0001_USER_ERROR)
			    || (status > E_DB_ERROR))
		    {
			sc0e_0_put(op_ccb->opf_errorblock.err_code, 0);
			sc0e_0_uput(op_ccb->opf_errorblock.err_code, 0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    }
		    qry_status = GCA_FAIL_MASK;
		    break;
		}
		op_ccb->opf_repeat = (PTR *) 0;
		cquery->cur_qp = op_ccb->opf_qep;
		if (! qef_wants_ruleqp)
		{
		    cquery->cur_state = CUR_FIRST_TIME;
		    cquery->cur_row_count = 0;
		    /* The important place for resetting rowproc is in
		    ** scs_input when we see the GCA_INVPROCx msg, but it
		    ** doesn't hurt to do it here whenever we produce a new
		    ** (non-nested) QP.
		    */
		    cquery->cur_rowproc = FALSE;
		}
		if ( (qmode == PSQ_RETRIEVE) || (qmode == PSQ_DEFCURS) )
		{
		    /*
		    ** Then retrieve the query plan to find the tuple
		    ** descriptor
		    */
		    qs_ccb = sscb->sscb_qsccb;
		    STRUCT_ASSIGN_MACRO(op_ccb->opf_qep, qs_ccb->qsf_obj_id);
		    status = qsf_call(QSO_INFO, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC0210_SCS_SQNCR_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sscb->sscb_qmode = 0;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			qry_status = GCA_FAIL_MASK;
			break;
		    }
		    /* GCA_TD_DATA */
                    cquery->cur_rdesc.rd_tdesc = (PTR)
                       ((QEF_QP_CB *) qs_ccb->qsf_root)->qp_sqlda;
		    cquery->cur_rdesc.rd_modifier =
			((GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc)->gca_result_modifier;
		    /* Don't save tdesc if define cursor... it will be, later */
        	    if (!(qmode == PSQ_DEFCURS) &&
			(cquery->cur_rdesc.rd_modifier & GCA_COMP_MASK))
		    {
			cquery->cur_rdesc.rd_tdesc =
			  scs_save_tdesc(scb, (PTR)cquery->cur_rdesc.rd_tdesc);
		    }

		    /*
		    ** For HETNET support STAR needs to retrieve the tuple
		    ** descriptor from the local, so tell QEF to open the
		    ** query with the local and retrieve teh TDESC, QEF will
		    ** copy it int QEP space.
		    */
		    cquery->cur_tup_size =
			((GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc)->gca_tsize;

		    if (qmode == PSQ_RETRIEVE ||
		    	sscb->sscb_ics.ics_db_access_mode == DMC_A_READ)
		    {
			singleton = ((QEF_QP_CB *)
			    qs_ccb->qsf_root)->qp_status & QEQP_SINGLETON;
			if (sscb->sscb_flags & SCS_STAR)
			{
			    qe_ccb = sscb->sscb_qeccb;
			    qe_ccb->qef_pcount = cquery->cur_qprmcount;
			    qe_ccb->qef_param = (QEF_PARAM *) cquery->cur_qparam;
			    qe_ccb->qef_usr_param = (QEF_USR_PARAM **) cquery->cur_qparam;

			    qe_ccb->qef_eflag = QEF_EXTERNAL;
			    /* Retrievals and READONLY databases are READ ONLY */
			    qe_ccb->qef_qacc = QEF_READONLY;

			    qe_ccb->qef_intstate = 0;

			    qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;

			    qe_ccb->qef_qso_handle = cquery->cur_qp.qso_handle;

			    rqf_td_desc = &cquery->cur_rdesc.rd_rqf_desc;
			    rqf_td_desc->scf_star_td_p = cquery->cur_rdesc.rd_tdesc;
			    status = scs_desc_send(scb, NULL);
			    rqf_td_desc->scf_ldb_td_p = cquery->cur_rdesc.rd_tdesc;
			    if (status)
			    {
				if (status != E_DB_ERROR)
				{
				    sc0e_0_put(status, 0);
				    sc0e_0_uput(status, 0);
				}
				sscb->sscb_qmode = 0;
				sscb->sscb_state = SCS_INPUT;
				*next_op = CS_EXCHANGE;
				qry_status = GCA_FAIL_MASK;
				break;
			    }

			    sscb->sscb_cfac = DB_QEF_ID;
			    qe_ccb->qef_r3_ddb_req.qer_d10_tupdesc_p =
				(PTR) rqf_td_desc;

			    status = qef_call(QEQ_QUERY, qe_ccb);
			    /*
			    ** STAR needs to clean all its associations
			    ** to the LDB.
			    */
			    if (sscb->sscb_interrupt)
			    {
				(void)qef_call(QEQ_ENDRET, qe_ccb);
			    }

			    if (DB_FAILURE_MACRO(status))
			    {
				SCC_GCMSG   *cmsg;
				qry_status = GCA_FAIL_MASK;
				scs_qef_error(status, qe_ccb->error.err_code,
					      DB_SCF_ID, scb);
				/* Remove tuple descriptor from FE queue */
				for (cmsg = cscb->cscb_mnext.scg_next;
				     cmsg && (cmsg != (SCC_GCMSG *)
						    &cscb->cscb_mnext);
				     cmsg = cmsg->scg_next)
				{
				    if (cmsg->scg_mtype != GCA_TDESCR)
					continue;
				    cmsg->scg_prev->scg_next = cmsg->scg_next;
				    cmsg->scg_next->scg_prev = cmsg->scg_prev;
				    if (( cmsg->scg_mask &
					    SCG_NODEALLOC_MASK) == 0 ||
				       ( cmsg->scg_mask &
					    SCG_QDONE_DEALLOC_MASK) != 0 )
				    {
					sc0m_deallocate(0, (PTR *)&cmsg);
				    }
				    break;
				}
				*next_op = CS_EXCHANGE;
				break;
			    }

			    if (sscb->sscb_interrupt)
			    {
				break;
			    }
			    sscb->sscb_cfac = DB_SCF_ID;

			    /*
			    ** account for the differences in the tuple size
			    ** between the LDB and STAR, LDB has the
			    ** final say.
			    */
			    cquery->cur_tup_size =
				((GCA_TD_DATA *)rqf_td_desc-> scf_ldb_td_p)->gca_tsize;
			}
			else
			{
			    status = scs_desc_send(scb, NULL);
			    if (status)
			    {
				if (status != E_DB_ERROR)
				{
			            sc0e_0_put(status, 0);
			            sc0e_0_uput(status, 0);
				}
			        sscb->sscb_qmode = 0;
			        sscb->sscb_state = SCS_INPUT;
			        *next_op = CS_EXCHANGE;
			        qry_status = GCA_FAIL_MASK;
			        break;
			    }
			}
		    }
		}
	    }
	    else	/* whether we need to optimize */
	    {
		(VOID) qef_call(QEU_ETRAN, &qeu); /* Done with query */
					    /* planning */

                /* Only reset row count if QEF hasn't requested that a DB
                ** procedure be loaded.
                */
                if (!qef_wants_ruleqp)
                {
		   /* Non optimized queries typically have no row count */
		   cquery->cur_row_count = -1;
                }
	    }
	}
	else if ((sscb->sscb_state == SCS_ADDPROT)
		|| (sscb->sscb_state == SCS_DELPROT))
	{
	    qmode = 0;		    /* remove old values */

	    if (!(sscb->sscb_flags & SCS_STAR))
	    {
		dm_ccb = sscb->sscb_dmccb;
		dm_ccb->dmc_op_type = DMC_SESSION_OP;
		dm_ccb->dmc_sl_scope = DMC_SL_SESSION;
		dm_ccb->dmc_flags_mask = DMC_SYSCAT_MODE;
	    }
	    ps_ccb = sscb->sscb_psccb;
	    ps_ccb->psq_qlang = sscb->sscb_ics.ics_qlang;
	    MEcopy((PTR)sscb->sscb_ics.ics_eusername,
			 sizeof(ps_ccb->psq_user),
			 (PTR)&ps_ccb->psq_user);

	    if (sscb->sscb_state == SCS_ADDPROT)
	    {
		if (!(sscb->sscb_flags & SCS_STAR))
		{
		    dm_ccb->dmc_s_type = DMC_S_SYSUPDATE;
		}
		ps_ccb->psq_flag |= PSQ_CATUPD; /* psq_catupd = true */
	    }
	    else
	    {
		if (!(sscb->sscb_flags & SCS_STAR))
		{
		    dm_ccb->dmc_s_type = DMC_S_NORMAL;
		}
		ps_ccb->psq_flag &= ~PSQ_CATUPD;    /* psq_catupd = false */
	    }

	    if (sscb->sscb_flags & SCS_STAR)
	    {

		qe_ccb = sscb->sscb_qeccb;
		if ( ps_ccb->psq_flag & PSQ_CATUPD )
		{
		    qe_ccb->qef_r3_ddb_req.qer_d11_modify.dd_m2_y_flag = DD_1MOD_ON;
		}
		else
		{
		    qe_ccb->qef_r3_ddb_req.qer_d11_modify.dd_m2_y_flag = DD_2MOD_OFF;
		}

		status = qef_call(QED_1SCF_MODIFY, qe_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
		    scd_note(status, DB_DMF_ID);
		}
	    }

	    if (!(sscb->sscb_flags &  SCS_STAR))
	    {
		status = dmf_call(DMC_ALTER, dm_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (dm_ccb->error.err_code != E_DM0065_USER_INTR)
		    {
			qry_status = GCA_FAIL_MASK;
			sc0e_0_put(E_SC0213_DMF_ERROR, 0);
			sc0e_0_put(dm_ccb->error.err_code, 0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			scd_note(status, DB_DMF_ID);
			break;
		    }
		}
	    }

	    status = psq_call(PSQ_ALTER, ps_ccb, sscb->sscb_psscb);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (ps_ccb->psq_error.err_code != E_PS0003_INTERRUPTED)
		{
		    qry_status = GCA_FAIL_MASK;
		    sc0e_0_put(E_SC0215_PSF_ERROR, 0);
		    sc0e_0_put(ps_ccb->psq_error.err_code, 0);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    scd_note(status, DB_PSF_ID);
		}
	    }
	    qry_status = SCS_OK;
	    sscb->sscb_state = SCS_INPUT;
	    *next_op = CS_EXCHANGE;
	    break;
	}
	else if (sscb->sscb_state == SCS_RPASSWORD)
	{
	    GCA_PRREPLY_DATA *reply;
	    GCA_DATA_VALUE   *gca_data_value;
	    /*
	    ** Reading result of password prompt, should have
	    ** a GCA_PRREPLY
	    */
	    if ((cscb->cscb_gci.gca_status == E_GC0001_ASSOC_FAIL)
	        || (cscb->cscb_gci.gca_status == E_GC0023_ASSOC_RLSED))
	    {
	        *next_op = CS_TERMINATE;
	        sscb->sscb_state = SCS_TERMINATE;
		sscb->sscb_cfac = DB_CLF_ID;
	        return(OK);
	    }
	    else if (cscb->cscb_gci.gca_status != E_GC0000_OK)
	    {
	        sc0e_0_put(cscb->cscb_gci.gca_status,
	    	    &cscb->cscb_gci.gca_os_status);
	        sc0e_0_put(E_SC022F_BAD_GCA_READ, 0);
	        sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	        sscb->sscb_qmode = 0;
	        sscb->sscb_state = SCS_TERMINATE;
		sscb->sscb_cfac = DB_CLF_ID;
	        *next_op = CS_EXCHANGE;
	        return(E_DB_SEVERE);
	    }

	    /*
	    ** Read Response
	    */

	    if (cscb->cscb_gci.gca_message_type != GCA_PRREPLY)
	    {
		/* Couldn't read the data, or not prompt reply */
	        sc0e_0_put(E_SC0225_BAD_GCA_INFO, 0);
	        sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	        sscb->sscb_state = SCS_TERMINATE;
		sscb->sscb_cfac = DB_CLF_ID;
	        *next_op = CS_TERMINATE;
	        return(E_DB_SEVERE);
	    }

	    if(sscb->sscb_interrupt)
	    {
		/* Interrupt in password processing, terminate */
	        *next_op = CS_TERMINATE;
	        sscb->sscb_state = SCS_TERMINATE;
		sscb->sscb_cfac = DB_CLF_ID;
	        return(OK);

	    }
	    /*
	    ** Got the reply, so lets process it
	    */
	    reply = (GCA_PRREPLY_DATA*)cscb->cscb_gci.gca_data_area;

	    gca_data_value= &reply->gca_prvalue[0];
	    if(gca_data_value->gca_type != DB_CHA_TYPE)
	    {
		/* Invalid type for reply, should be CHA */
	        sc0e_0_put(E_SC0225_BAD_GCA_INFO, 0);
	        sscb->sscb_state = SCS_TERMINATE;
		sscb->sscb_cfac = DB_CLF_ID;
	        *next_op = CS_TERMINATE;
	        return(E_DB_SEVERE);
	    }
	    /*
	    ** Next check the password
	    */
	    status=scs_check_password(scb,
			gca_data_value->gca_value,
			gca_data_value->gca_l_value,
			TRUE);
	    if (status != E_DB_OK)
	    {
		/* Tell user they couldn't connect */
		ret_val = E_US18FF_BAD_USER_ID;
	    }
	    else
	    {
		ret_val = 0;
	    }
	    /*
	    ** Continue processing, send the accept/error message
	    */
	    status = sc0m_allocate(0,
			    sizeof(SCC_GCMSG) + sizeof(GCA_ER_DATA)
				    + (ret_val ? ER_MAX_LEN : 0),
			    DB_SCF_ID,
			    (PTR)SCS_MEM,
			    SCG_TAG,
			    (PTR *) &message);
	    if (status)
	    {
		sc0e_0_put(status, 0 );
		sc0e_0_uput(status, 0 );
		scd_note(E_DB_SEVERE, DB_SCF_ID);
		message = (SCC_GCMSG *) cscb->cscb_inbuffer;
	    }
	    message->scg_mask = 0;
	    message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));
	    message->scg_next = (SCC_GCMSG *) &cscb->cscb_mnext;
	    message->scg_prev = cscb->cscb_mnext.scg_prev;
	    cscb->cscb_mnext.scg_prev->scg_next = message;
	    cscb->cscb_mnext.scg_prev = message;

	    if (ret_val)
	    {
		GCA_ER_DATA	    *emsg;
		GCA_DATA_VALUE  *gca_data_value;


		emsg = (GCA_ER_DATA *)message->scg_marea;
		gca_data_value = emsg->gca_e_element[0].gca_error_parm;

		emsg->gca_l_e_element = 1;
		emsg->gca_e_element[0].gca_id_server = Sc_main_cb->sc_pid;
		emsg->gca_e_element[0].gca_severity = 0;
		emsg->gca_e_element[0].gca_l_error_parm = 1;

		status = ule_format(ret_val, 0, ULE_LOOKUP, &sqlstate,
			gca_data_value->gca_value, ER_MAX_LEN,
		    &gca_data_value->gca_l_value, &ule_error, 0);

		gca_data_value->gca_type = DB_CHA_TYPE;

		if (cscb->cscb_version <= GCA_PROTOCOL_LEVEL_2)
		{
		    emsg->gca_e_element[0].gca_id_error = ret_val;
		    emsg->gca_e_element[0].gca_local_error = 0;
		}
		else if (cscb->cscb_version <
			GCA_PROTOCOL_LEVEL_60)
		{
		    emsg->gca_e_element[0].gca_local_error = ret_val;
		    emsg->gca_e_element[0].gca_id_error =
			ss_2_ge(sqlstate.db_sqlstate, ret_val);
		}
		else
		{
		    emsg->gca_e_element[0].gca_local_error = ret_val;
		    emsg->gca_e_element[0].gca_id_error =
			ss_encode(sqlstate.db_sqlstate);
		}

		*next_op = CS_OUTPUT;    /* die if can't start up */
		sscb->sscb_state = SCS_TERMINATE;
		message->scg_mtype = GCA_RELEASE;
		message->scg_msize = sizeof(GCA_ER_DATA)
				    - sizeof(GCA_DATA_VALUE)
					+ sizeof(i4)
					+ sizeof(i4)
					+ sizeof(i4)
		    + gca_data_value->gca_l_value;
	    }
	    else
	    {
		/*
		** Send accept message
		*/
		message->scg_mtype = GCA_ACCEPT;
		message->scg_msize = 0;
		scb->cs_scb.cs_mask |= CS_NOXACT_MASK;
	        cscb->cscb_gci.gca_status = E_SC1000_PROCESSED;
	        sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
	    }
	    sscb->sscb_cfac = DB_CLF_ID;
	    return (E_DB_OK);
	}
	else
	{
	    /* Restore the query mode from the last instance of this query */
	    qmode = sscb->sscb_qmode;
	}

	qry_status = SCS_OK;

	/*
	** Allow distributed 'direct connect' thorough. The LDB
	** must be interrupted before we 'ack' the client.
	*/

	if ( ( (sscb->sscb_interrupt)
	       ||
	       (sscb->sscb_force_abort == SCS_FAPENDING)
	     )
	     && (sscb->sscb_state != SCS_DCONNECT)
	   )
	    break;

	/*
	** If this is a re-connected session for a distributed transactin,
	** the only statement allowed after the re-connect statement is
	** ROLLBACK/ABORT or COMMIT.
	*/

	if ((sscb->sscb_ics.ics_dmf_flags & DMC_NLK_SESS) &&
	    (qmode != PSQ_COMMIT && qmode != PSQ_ABORT &&
	    qmode != PSQ_ROLLBACK && qmode != PSQ_XA_COMMIT &&
	    qmode != PSQ_XA_ROLLBACK)
	   )
	{
	    qry_status = GCA_FAIL_MASK;
	    sc0e_0_uput(E_US126A_4714_ILLEGAL_STMT, 0);
	    sscb->sscb_state = SCS_INPUT;
	    *next_op = CS_EXCHANGE;
	    break;
	}
	else
	{
	    /* A valid statement has given, turn off the re-connecting status. */
	    sscb->sscb_ics.ics_dmf_flags &= ~DMC_NLK_SESS;
	}

	psf_stuff = 0;

        if((sscb->sscb_flags & SCS_DROPPED_GCA) && qmode==0)
	{
	    break;
	}

        if (tproc_cur_load_qp)
        {
            /* cursor select for table procedure and
             * the table procedure has been recompiled and reloaded.
             */

            /* Restore PSQ_DEFCURS */
            qmode = sscb->sscb_qmode = qmode_saved;
            qef_wants_ruleqp = 0;

            /* Prepare to open the same cursor */
            STRUCT_ASSIGN_MACRO(cur_qp_saved, cquery->cur_qp);

            STRUCT_ASSIGN_MACRO(cursid_saved, ps_ccb->psq_cursid);
            qe_ccb->qef_cb->qef_dsh = dsh_saved;
            qe_ccb->qef_cb->qef_odsh = dsh_saved;
            qe_ccb->qef_qso_handle = cquery->cur_qp.qso_handle;

            /* Open the same cursor. Don't double count. */
            qe_ccb->qef_cb->qef_open_count--;

        }

	switch (qmode)		/* goes 'til "end of massive switch stmt" */
	{

	    case PSQ_EXEDBP:	/* EXECUTE PROCEDURE or internally requested */
		sscb->sscb_nostatements += 1;

		/* Give back the query id */
		psf_qp_handle = 0;
		qe_ccb = sscb->sscb_qeccb;
		STRUCT_ASSIGN_MACRO(cquery->cur_qname, qe_ccb->qef_qp);
		qe_ccb->qef_qso_handle = 0;
		switch( cscb->cscb_gci.gca_message_type )
		{
		case GCA1_INVPROC :
		case GCA2_INVPROC :
		    qe_ccb->qef_stmt_type |= QEF_GCA1_INVPROC;
		    break;
		}

		/*
		** Reset QEF parameters ONLY if not internally requested.
		** Otherwise the CB must be left intact.
		** Note that rowproc indicator will be false if we've just
		** now compiled the QP for the rowproc.  It will be set for
		** subsequent loops when running a rowproc (rule qp retry,
		** send output, etc).  Don't reset stuff if we're in the
		** middle of running the rowproc.
		*/

		rowproc = cquery->cur_rowproc;
		if (!qef_wants_ruleqp && !rowproc)
		{
		    qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;
		    qe_ccb->qef_flag = 0;
		    if (sscb->sscb_param)
		    {
			qe_ccb->qef_usr_param =
				    (QEF_USR_PARAM **) sscb->sscb_param;
			qe_ccb->qef_pcount = cquery->cur_qprmcount;
		    }
		    qe_ccb->qef_cb = sscb->sscb_qescb;
		    cquery->cur_qparam = (PTR) qe_ccb->qef_usr_param;
		    cquery->cur_row_count = 0;
		    cquery->cur_qp.qso_handle = 0;

		    /* Locate query plan for procedure so we can check
		    ** for row-producing procedure. */
		    qs_ccb = sscb->sscb_qsccb;
		    qs_ccb->qsf_obj_id.qso_type = QSO_QP_OBJ;
		    MEcopy(&cquery->cur_qname, sizeof(DB_CURSOR_ID),
			    qs_ccb->qsf_obj_id.qso_name);
		    qs_ccb->qsf_obj_id.qso_lname = sizeof(DB_CURSOR_ID);
		    qs_ccb->qsf_lk_state = QSO_SHLOCK;
		    status = qsf_call(QSO_GETHANDLE, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			if (qs_ccb->qsf_error.err_code == E_QS0019_UNKNOWN_OBJ)
			{
			    /* this is a re-try case, don't just break
			    ** we should continue to the start of the
			    ** retry loop and re-build the query plan,
			    ** if there is a problem re-building, the
			    ** underlying problem (e.g. table for a
			    ** procedure no longer exists) will be reported
			    ** back to the user */
			    continue;
			}
			else
			{
			    qry_status = GCA_FAIL_MASK;
			    sscb->sscb_state = SCS_INPUT;
			    *next_op = CS_EXCHANGE;
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC0210_SCS_SQNCR_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    break;
			}
		    }
		    rowproc = ((((QEF_QP_CB *)qs_ccb->qsf_root)->qp_status
					& QEQP_ROWPROC) != 0);
		    cquery->cur_rowproc = rowproc;
/* bug 70170 */
		    rp_handle = qs_ccb->qsf_obj_id.qso_handle;

		    /* Immediately free the object. */
		    status = qsf_call(QSO_UNLOCK, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			qry_status = GCA_FAIL_MASK;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC0210_SCS_SQNCR_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			break;
		    }
		}

		/* If it is a row producing procedure, prepare TDESC
		** message for caller, but only if we have not yet done so.
		*/
		if (rowproc && cquery->cur_state == CUR_FIRST_TIME)
		{
		    cquery->cur_rdesc.rd_tdesc = (PTR)
			((QEF_QP_CB *) qs_ccb->qsf_root)->qp_sqlda;
		    cquery->cur_rdesc.rd_modifier =
			((GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc)->gca_result_modifier;
		    if (cquery->cur_rdesc.rd_modifier & GCA_COMP_MASK)
		    {
			cquery->cur_rdesc.rd_tdesc =
			    scs_save_tdesc(scb, (PTR)cquery->cur_rdesc.rd_tdesc);
		    }
		    cquery->cur_tup_size = ((GCA_TD_DATA *)
		    		cquery->cur_rdesc.rd_tdesc)->gca_tsize;
		    status = scs_desc_send(scb, NULL);

		    if (status)
		    {
			if (status != E_DB_ERROR)
			{
			    sc0e_0_put(status, 0);
			    sc0e_0_uput(status, 0);
			}
			sscb->sscb_qmode = 0;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			qry_status = GCA_FAIL_MASK;
			break;
		    }
		    cquery->cur_state = CUR_TDESC_DONE;
		    cquery->cur_amem = NULL;
		}

		/* Star, execute a registered procedure.
		** To execute a registered procedure, two or three calls to qef
		** are required. The first call initiaties the execute procedure
		** on the ldb, the second optional call reads the ldb tuple
		** descriptor and tuples if there are byref parameters. The last
		** call reads the local GCA_RESPONSE.
		** The first and optional second qef calls are done here. The
		** last call is done below with the common qef calls.
		*/

		if (sscb->sscb_flags & SCS_STAR)
		{
        	    PTR    tmp_tdesc = NULL;

		    status = sc0m_allocate(0,
				sizeof(SC0M_OBJECT)+sizeof(QEF_DATA),
				DB_SCF_ID,
				(PTR)SCS_MEM, SCG_TAG,
				&cquery->cur_amem);
		    if (status)
		    {
			sc0e_0_put(status, 0);
			sc0e_0_uput(status, 0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			if (cscb->cscb_batch_count > 0)
			    cscb->cscb_eog_error = TRUE;
			qry_status = GCA_FAIL_MASK;
			break;
		    }

		    /* If STAR, pre-fetch the tuple descriptor from the LDB */
		    qe_ccb->qef_output = data_buffer =
		    cquery->cur_qef_data =
			(QEF_DATA *) ((char *)cquery->cur_amem +
				sizeof(SC0M_OBJECT));
		    status = scs_allocate_byref_tdesc(scb, &tdesc);
		    if (status)
		    {
			*next_op = CS_EXCHANGE;
			qry_status = GCA_FAIL_MASK;
			break;
		    }
		    if (tdesc != NULL)
		    {
			/* Has byrefs, allocate a second tuple descriptor */
			status = sc0m_allocate(0,
			   sizeof(SC0M_OBJECT) + tdesc->scg_msize,
			   DB_SCF_ID, (PTR)SCS_MEM, SCG_TAG, &tmp_tdesc);
			if (status)
			{
			    sc0e_0_put(status, 0);
			    sc0e_0_uput(status, 0);
			    scd_note(E_DB_SEVERE, DB_SCF_ID);
			    *next_op = CS_EXCHANGE;
			    if (cscb->cscb_batch_count > 0)
				cscb->cscb_eog_error = TRUE;
			    qry_status = GCA_FAIL_MASK;
			    break;
			}
			rqf_td_desc = &cquery->cur_rdesc.rd_rqf_desc;
			rqf_td_desc->scf_star_td_p =
			    (PTR)((char *)tmp_tdesc + sizeof(SC0M_OBJECT));
			rqf_td_desc->scf_ldb_td_p = (PTR)((char *)tdesc->scg_marea);
			MEcopy(rqf_td_desc->scf_ldb_td_p,
					 tdesc->scg_msize,
					 rqf_td_desc->scf_star_td_p);
			qe_ccb->qef_r3_ddb_req.qer_d10_tupdesc_p =
			    (PTR) rqf_td_desc;
		    }
		    else
		    {
			qe_ccb->qef_r3_ddb_req.qer_d10_tupdesc_p = NULL;
			singleton = FALSE;
			data_buffer->dt_size = 0;
			data_buffer->dt_data = NULL;
		    }

		    /* First qef call to initiate invproc on ldb */
		    sscb->sscb_cfac = DB_QEF_ID;
		    qe_ccb->qef_intstate = 0;
		    status = qef_call(QEQ_QUERY, qe_ccb);
		    sscb->sscb_cfac = DB_SCF_ID;

		    if (tmp_tdesc != NULL)
		    {
			/* Free extra tuple descriptor */
			sc0m_deallocate( 0, &tmp_tdesc );
			cquery->cur_tup_size =
			    ((GCA_TD_DATA *)rqf_td_desc->scf_ldb_td_p)->gca_tsize;
			singleton = TRUE;
			qe_ccb->qef_rowcount = 1;
		    }

		    if (DB_FAILURE_MACRO(status))
		    {
			if (tdesc)
			    sc0m_deallocate( 0, (PTR *)&tdesc );

			if (cquery->cur_amem)
			{
			    sc0m_deallocate(0, &cquery->cur_amem);
			    cquery->cur_qef_data = NULL;
			}
			if( (qe_ccb->error.err_code == E_QE0023_INVALID_QUERY)
			  ||(qe_ccb->error.err_code == E_QE0014_NO_QUERY_PLAN)
			  ||(qe_ccb->error.err_code == E_QE0119_LOAD_QP)
			  ||(qe_ccb->error.err_code == E_QE0301_TBL_SIZE_CHANGE)
                          ||(qe_ccb->error.err_code == E_QE030F_LOAD_TPROC_QP))
			{
			    /* Need qp recreated */
			    recreate = 1;
			    continue;
			}
			else
			{
			    scs_qef_error(status, qe_ccb->error.err_code,
					    DB_SCF_ID, scb);
			    *next_op = CS_EXCHANGE;
			    qry_status = GCA_FAIL_MASK;
			    break;
			}
		    }

		    /* Save local return status */
		    cscb->cscb_rpdata->gca_retstat = qe_ccb->qef_dbp_status;

		    if (tdesc)
		    {
			rqf_td_desc = &cquery->cur_rdesc.rd_rqf_desc;
			/* Second optional call if byref,
			** will read the ldb tuple descriptor
			** and tuple.
			*/
			status = scs_allocate_tdata(scb,
				      (GCA_TD_DATA *) rqf_td_desc->scf_ldb_td_p,
				      &tdata);
			if (status)
			{
			    *next_op = CS_EXCHANGE;
			    qry_status = GCA_FAIL_MASK;
			    break;
			}
			data_buffer->dt_size = cquery->cur_tup_size;
			data_buffer->dt_data = (PTR)((char *)tdata->scg_marea);
			sscb->sscb_cfac = DB_QEF_ID;
			qe_ccb->qef_intstate = 0;
			status = qef_call(QEQ_QUERY, qe_ccb);
			sscb->sscb_cfac = DB_SCF_ID;
		    }

		    /* Pre set to read LDB response block. */
		    singleton = FALSE;
		    data_buffer->dt_size = 1;
		    data_buffer->dt_data = NULL;
		} /* STAR, PSQ_EXEDBP */
		else
		{
		    if (!qef_wants_ruleqp)
		    {
			status = scs_allocate_byref_tdesc(scb, &tdesc);
			if (status)
			{
			    *next_op = CS_EXCHANGE;
			    qry_status = GCA_FAIL_MASK;
			    break;
			}
			if (tdesc != NULL)
			{
			    /* we will be returning byref parameters. */
			    qe_ccb->qef_output = &qef_data;
			    qef_data.dt_data = (PTR)((char *)tdesc->scg_marea);
			    qef_data.dt_size = tdesc->scg_msize;
			}
			else
			{
			    qe_ccb->qef_output = NULL;
			}
		    }
		}
		/* Fall thru to query processing -- will skip normal	    */
		/* execqry processing					    */

	    case PSQ_EXECQRY:
	      if (qmode == PSQ_EXECQRY)
	      {
		bool	retry_qry = FALSE;

		/* One difference between EXECQRY and EXEDBP is that this
		** code changes qmode to that of the underlying repeated
		** query.  So we only go thru EXECQRY the first time, unlike
		** EXEDBP which can get re-executed after output flush, etc.
		*/
		sscb->sscb_nostatements += 1;
		if (sscb->sscb_state == SCS_CONTINUE)
		{
		    /*
		    ** In these cases, PSF will have produced the appropriate
		    ** control block in qsf.  Simply call QEF with the right
		    ** kind of operation.
		    */

		    /*
		    ** An exclusive lock is gotten since we will need one to
		    ** the object at the end anyway.
		    */

		    qs_ccb = sscb->sscb_qsccb;
		    psf_qp_handle =
			qs_ccb->qsf_obj_id.qso_handle =
			    ps_ccb->psq_result.qso_handle;
		    qs_ccb->qsf_lk_state = QSO_EXLOCK;
		    status = qsf_call(QSO_LOCK, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			    qry_status = GCA_FAIL_MASK;
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC0210_SCS_SQNCR_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    sscb->sscb_state = SCS_INPUT;
			    *next_op = CS_EXCHANGE;
			    break;
		    }
		    psf_lkid = qs_ccb->qsf_lk_id;
		    qsf_object = 1;

		    qe_ccb = (QEF_RCB *) qs_ccb->qsf_root;
		    cquery->cur_qe_cb = NULL;
		    STRUCT_ASSIGN_MACRO(ps_ccb->psq_cursid,
					cquery->cur_qname);
		    MEcopy((PTR)qe_ccb, sizeof(QEF_RCB), (PTR) sscb->sscb_qeccb);
		    qe_ccb = sscb->sscb_qeccb;
/* bug 70170 */
		    rp_handle = qs_ccb->qsf_obj_id.qso_handle;
		}
		else
		{
		    psf_qp_handle = 0;
		    qe_ccb = sscb->sscb_qeccb;
		    qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;
		    qe_ccb->qef_flag = 0;

		    STRUCT_ASSIGN_MACRO(cquery->cur_qname, qe_ccb->qef_qp);
		    qe_ccb->qef_qso_handle = 0;
		    if (sscb->sscb_param)
		    {
			QEF_PARAM *qef_param;
/* bug 71293 */
			status = sc0m_allocate(0,
				sizeof(SC0M_OBJECT) + sizeof(QEF_PARAM),
				DB_SCF_ID, (PTR)SCS_MEM,
				SCG_TAG, (PTR *)&qef_param);

			if (status)
			{
			    sc0e_0_put(status, 0);
			    sc0e_0_uput(status, 0 );
			    scd_note(E_DB_SEVERE, DB_SCF_ID);
			}
			qef_param = (QEF_PARAM *)
                                ((char *) qef_param + sizeof(SC0M_OBJECT));
			cquery->cur_p_allocated = TRUE;
			qe_ccb->qef_param = qef_param;
			qef_param->dt_data = sscb->sscb_param;
			qef_param->dt_next = 0;
			qef_param->dt_size =
			    qe_ccb->qef_pcount = cquery->cur_qprmcount;
		    }
		}
		qe_ccb->qef_cb = sscb->sscb_qescb;
		cquery->cur_row_count = 0;
		cquery->cur_qp.qso_handle = 0;
		cquery->cur_state = CUR_FIRST_TIME;

		qs_ccb = sscb->sscb_qsccb;
		qs_ccb->qsf_obj_id.qso_type = QSO_QP_OBJ;
		MEcopy( (PTR) &cquery->cur_qname,
			    sizeof(DB_CURSOR_ID),
			    qs_ccb->qsf_obj_id.qso_name);
		qs_ccb->qsf_obj_id.qso_lname = sizeof(DB_CURSOR_ID);
		qs_ccb->qsf_lk_state = QSO_SHLOCK;
		status = qsf_call(QSO_GETHANDLE, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (qs_ccb->qsf_error.err_code == E_QS0019_UNKNOWN_OBJ)
		    {
			qry_status = GCA_FAIL_MASK | GCA_RPQ_UNK_MASK;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			break;
		    }
		    else
		    {
			qry_status = GCA_FAIL_MASK;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			break;
		    }
		}
		qmode = sscb->sscb_qmode =
			((QEF_QP_CB *) qs_ccb->qsf_root)->qp_qmode;
/* bug 70170 */
		rp_handle = qs_ccb->qsf_obj_id.qso_handle;
		if (qmode == PSQ_RETRIEVE)
		{
		    cquery->cur_rdesc.rd_tdesc = (PTR)
                       ((QEF_QP_CB *) qs_ccb->qsf_root)->qp_sqlda;
		    cquery->cur_rdesc.rd_modifier =
			((GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc)->gca_result_modifier;
        	    if (cquery->cur_rdesc.rd_modifier & GCA_COMP_MASK)
		    {
			cquery->cur_rdesc.rd_tdesc =
			  scs_save_tdesc(scb, (PTR)cquery->cur_rdesc.rd_tdesc);
		    }
		    singleton = ((QEF_QP_CB *)
			    qs_ccb->qsf_root)->qp_status & QEQP_SINGLETON;
		    cquery->cur_tup_size = ((GCA_TD_DATA *)
		       cquery->cur_rdesc.rd_tdesc)->gca_tsize;
		    /*
		    ** If Star the tuple descriptor is deferred until QEF is
		    ** called, so the descriptor provided by the loca is shipped.
		    */
		    if (sscb->sscb_flags & SCS_STAR)
		    {
			rqf_td_desc = &cquery->cur_rdesc.rd_rqf_desc;
			rqf_td_desc->scf_star_td_p = cquery->cur_rdesc.rd_tdesc;
			status = scs_desc_send(scb, (SCC_GCMSG **)NULL);
			rqf_td_desc->scf_ldb_td_p = cquery->cur_rdesc.rd_tdesc;
		    }
		    else
		    {
		        status = scs_desc_send(scb, NULL);
		    }

		    if (status)
		    {
			if (status != E_DB_ERROR)
			{
			    sc0e_0_put(status, 0);
			    sc0e_0_uput(status, 0);
			}
			sscb->sscb_qmode = 0;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			qry_status = GCA_FAIL_MASK;
			break;
		    }
		}
		else if (qmode == PSQ_DEFCURS)
		{
		    QEF_QP_CB	*qp_cb = (QEF_QP_CB *) qs_ccb->qsf_root;
                    cquery->cur_rdesc.rd_tdesc = (PTR) qp_cb->qp_sqlda;
		    cquery->cur_rdesc.rd_modifier =
			((GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc)->gca_result_modifier;
		    cquery->cur_tup_size = ((GCA_TD_DATA *) qp_cb->qp_sqlda)->gca_tsize;

		    status = scs_def_curs(scb, (i4) PSQ_EXECQRY,
			&cquery->cur_qname,
			qs_ccb->qsf_obj_id.qso_handle,
			retry_fetch,
			(QEF_QP_CB *) qs_ccb->qsf_root,
			&message, &qry_status, next_op, &retry_qry, &ret_val);

		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_0_put(status, 0);
			sc0e_0_uput(status, 0);
		  	sscb->sscb_qmode = 0;
			break;
		    }
		}
		else
		{
		    cquery->cur_tup_size = 0;
		}
		status = qsf_call(QSO_UNLOCK, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC0210_SCS_SQNCR_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    break;
		}

		if (qmode == PSQ_DEFCURS)
		{
		    /*
		    ** if defining a repeat cursor, we have done all that
		    ** needs to be done.  Here we have two options: if
		    ** scs_def_curs() determined that the query needs to be
		    ** retried, we want to return to the top of the loop,
		    ** otherwise, we just need to break out.
		    ** NOTE that if (retry_qry), we had to wait until now to
		    ** return to the top of the loop in order to ensure that
		    ** we first unlock the QP object.
		    */
		    if (retry_qry)
		        continue;
		    else
		        break;
		}

		/*
		** Star needs the LDB tuple descriptor, so call QEF to get it
		** and put it in the memory that scs_desc_send() has already
		** allocated.
		*/
		if ((qmode == PSQ_RETRIEVE) &&
		    (sscb->sscb_flags & SCS_STAR))
		{
		    qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
		    qe_ccb->qef_eflag = QEF_EXTERNAL;
		    qe_ccb->qef_qacc = QEF_UPDATE;
		    qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;
		    qe_ccb->qef_qso_handle = cquery->cur_qp.qso_handle;
		    qe_ccb->qef_output = 0;
		    qe_ccb->qef_rowcount = 0;
		    qe_ccb->qef_count = 0;
		    sscb->sscb_cfac = DB_QEF_ID;
		    qe_ccb->qef_r3_ddb_req.qer_d10_tupdesc_p =
			(PTR) rqf_td_desc;
		    status = qef_call(QEQ_QUERY, qe_ccb);
		    /* Bug 79770 */
		    if ((rp_handle) && (status == 0))
			cquery->cur_qp.qso_handle = rp_handle;
		    /*
		    ** STAR needs to clean all its associations to the LDB.
		    */
		    if (sscb->sscb_interrupt)
		    {
			(void)qef_call(QEQ_ENDRET, qe_ccb);
		    }

		    if (DB_FAILURE_MACRO(status))
		    {
			qry_status = GCA_FAIL_MASK;
			if ((qe_ccb->error.err_code == E_QE0023_INVALID_QUERY)
                            ||
			    (qe_ccb->error.err_code == E_QE0014_NO_QUERY_PLAN))
			{
			    if (qe_ccb->qef_qso_handle == 0)
			    {
				if (ult_check_macro(&sscb->sscb_trace,
						    SCS_TEVENT, NULL, NULL))
				{
				    sc0e_trace("QEP unknown or invalid -- Requesting resend");
				}
			    }
			    qry_status = GCA_FAIL_MASK | GCA_RPQ_UNK_MASK;
			    sscb->sscb_state = SCS_INPUT;
			}

			scs_qef_error(status, qe_ccb->error.err_code,
				      DB_SCF_ID, scb);
			ret_val = E_DB_OK;
			STRUCT_ASSIGN_MACRO(qe_ccb->qef_comstamp,
					    sscb->sscb_comstamp);
			cquery->cur_row_count = -1;
			*next_op = CS_EXCHANGE;
			break;
		    }

		    if (sscb->sscb_interrupt)
		    {
			break;
		    }

		    sscb->sscb_cfac = DB_SCF_ID;
		    if (!status)
			qe_ccb->error.err_code = 0;

		    /*
		    ** account for the differences in the tuple size between the
		    ** LDB and STAR, LDB has the final say.
		    */
		    cquery->cur_tup_size =
		        ((GCA_TD_DATA *)rqf_td_desc->scf_ldb_td_p)->gca_tsize;
		} /* PSQ_RETRIEVE && STAR */

		if (qsf_object)
		    qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;

		cquery->cur_qprmcount = qe_ccb->qef_pcount;
		cquery->cur_qparam = (PTR) qe_ccb->qef_param;
		/* fall thru to set query processing */
	      }

	    case PSQ_RETCURS:
		if (qmode == PSQ_RETCURS)  /* Check, as may be fall through */
		{
		    qe_ccb = sscb->sscb_qeccb;
		    if (qe_ccb)
		    {
			/* For scrollable cursors. */
			qe_ccb->qef_fetch_anchor =
					sscb->sscb_cursor.curs_anchor;
			qe_ccb->qef_fetch_offset =
					sscb->sscb_cursor.curs_offset;
		    }
		    /* When scs-input sees the GCA_FETCH, it resets the
		    ** cur_state to FIRST TIME.  If it's not first time,
		    ** we must be working on a multirow fetch.
		    */
		    if (cquery->cur_state == CUR_FIRST_TIME)
		    {
			/* Find cursor */
			csi_no = scs_csr_find(scb);
			if (csi_no == sscb->sscb_cursor.curs_limit)
			{
			    qry_status = GCA_FAIL_MASK;
			    sc0e_0_uput(E_SC021E_CURSOR_NOT_FOUND, 0);
			    sscb->sscb_state = SCS_INPUT;
			    *next_op = CS_EXCHANGE;
			    break;
			}
			/* Save cursor table index in case we return for more */
			csi = sscb->sscb_cursor.curs_csi;
			sscb->sscb_cursor.curs_curno = csi_no;
			/* Assign fields so that normal query stuff works */
			cquery->cur_qp.qso_handle = csi->csitbl[csi_no].csi_qp;
			cquery->cur_tup_size = csi->csitbl[csi_no].csi_tsize;
		        cquery->cur_rdesc.rd_tdesc = csi->csitbl[csi_no].csi_rdesc.rd_tdesc;
		        cquery->cur_rdesc.rd_modifier =
		((GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc)->gca_result_modifier;

			STRUCT_ASSIGN_MACRO(
			       csi->csitbl[csi_no].csi_rdesc.rd_rqf_desc,
			       cquery->cur_rdesc.rd_rqf_desc);

			/*
			** Setup to reference open cursor query text object
		 	** handle, should we need to retry fetch.
			** Otherwise it will be destroyed as we exit
			** sequencer.
			*/
			if (csi->csitbl[csi_no].csi_thandle)
			{
			    sscb->sscb_thandle =
				    csi->csitbl[csi_no].csi_thandle;
			    sscb->sscb_tlk_id =
				    csi->csitbl[csi_no].csi_tlk_id;
			    csi->csitbl[csi_no].csi_thandle = (PTR)NULL;
			    csi->csitbl[csi_no].csi_tlk_id = 0;
			}

			if (sscb->sscb_flags & SCS_STAR)
			{
			    qe_ccb = sscb->sscb_qeccb;
			    rqf_td_desc = &cquery->cur_rdesc.rd_rqf_desc;
			    qe_ccb->qef_r3_ddb_req.qer_d10_tupdesc_p =
				(PTR) rqf_td_desc;
			}
			cquery->cur_rdesc.rd_modifier =
			    ((GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc)->gca_result_modifier;

			cquery->cur_row_count = 0;
			/* If read-only figure out if we can pre-fetch */
			if ((csi->csitbl[csi_no].csi_state & CSI_READONLY_MASK)
			     && (sscb->sscb_cursor.curs_frows > 1)
			   )
			{
			    /*
			    ** Need to adjust # rows to request from QEQ_FETCH?
			    ** curs_frows was already figured on GCA_FETCH.
			    */
			    /* Are we expecting a singleton result row? */
			    if (  csi->csitbl[csi_no].csi_state
				& CSI_SINGLETON_MASK)
			    {
				/* 2 rows results in row + EOF in one shot */
				sscb->sscb_cursor.curs_frows = 2;
			    }
			    /* Will we exceed resource limits? */
			    else if ((sscb->sscb_ics.ics_qrow_limit > 0)
				  && (sscb->sscb_ics.ics_qrow_limit <
				      sscb->sscb_cursor.curs_frows)
			            )
			    {
				sscb->sscb_cursor.curs_frows =
				  sscb->sscb_ics.ics_qrow_limit;
			    }
			}
			else	/* Not prefetch - request just 1 row */
			{
			    /* Zero rows requested is possible for scrollable
			    ** cursors. */
			    if (sscb->sscb_cursor.curs_frows > 1)
				sscb->sscb_cursor.curs_frows = 1;
			}
			fetch_rows = sscb->sscb_cursor.curs_frows;

			/*
			** If user only wanted 1 row originally then mark it.
			** This is NOT the same as having just one row left to
			** FETCH from a multi-row FETCH.
			**
			** In the case of a fetch of a row
			** containing a large object, the fetch
			** may truly be a singleton fetch (i.e. a
			** request for one row).  However, since
			** large objects may require more than one
			** tuple buffer/message to send back, the
			** outer "loop" (sequencer being recalled
			** until it requests input) should
			** continue until the whole tuple is sent.
			** Thus, we will rely soley on the
			** rowcount being >= frows comparison to
			** determine when we are done in the case
			** of a large object query.  Therefore, we
			** require that large object queries never
			** have the singleton indicator set.
			*/

			singleton = (fetch_rows == 1) &&
			    ((cquery->cur_rdesc.rd_modifier & GCA_LO_MASK) == 0);

			/*
			** if we are fetching from a repeat cursor involving
			** parameters, initialize
			** cquery->cur_qprmcount and
			** cquery->cur_qparam using
			** information saved up in the cursor block
			*/
			if (   csi->csitbl[csi_no].csi_state & CSI_REPEAT_MASK
			    && csi->csitbl[csi_no].csi_rep_parm_handle)
			{
			    cquery->cur_qprmcount =
				csi->csitbl[csi_no].csi_rep_parms->dt_size;
			    cquery->cur_qparam =
				(PTR) csi->csitbl[csi_no].csi_rep_parms;
			}
		    }
		    else	/* Not first time through FETCH */
		    {
			/*
			** How many rows are left to fetch? Original request #
			** less the amount we've already fetched.
			*/
			fetch_rows = sscb->sscb_cursor.curs_frows -
				     cquery->cur_row_count;
			singleton = 0;	/* We must have wanted more */
		    }
		}
		/* FALL THROUGH */

	    case PSQ_RETRIEVE:
	    case PSQ_RETINTO:
	    case PSQ_DGTT_AS_SELECT:
	    case PSQ_CREATE:
	    case PSQ_APPEND:
	    case PSQ_DELETE:
	    case PSQ_REPLACE:
	    case PSQ_DELCURS:	/* Cursor DELETE & REPLACE are regular qrys */
	    case PSQ_REPCURS:
	    case PSQ_CALLPROC:	/* CALLPROC is much like a regular query too */
	    case PSQ_EVREGISTER:/* REGISTER EVENT */
	    case PSQ_EVDEREG:	/* REMOVE EVENT */
	    case PSQ_EVRAISE:	/* RAISE EVENT */
	    case PSQ_VIEW:	/* CREATE/DEFINE VIEW */
	    case PSQ_CREATE_SCHEMA: /* CREATE SCHEMA */
	    case PSQ_ALTERTABLE:
	    case PSQ_ATBL_ADD_COLUMN:
	    case PSQ_ATBL_RENAME_TABLE:
	    case PSQ_ATBL_RENAME_COLUMN:

		qe_ccb = sscb->sscb_qeccb;
		/* Do not reset any CB fields if internal resources allocated */
		if (!(qef_wants_ruleqp || qef_wants_eiqp))
		{
                    qe_ccb->qef_rule_depth = 0;
		    qe_ccb->qef_pcount =cquery->cur_qprmcount;
		    qe_ccb->qef_param = (QEF_PARAM *) cquery->cur_qparam;
		    qe_ccb->qef_usr_param = (QEF_USR_PARAM **) cquery->cur_qparam;

		    qe_ccb->qef_cb = sscb->sscb_qescb;
		    qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
		    qe_ccb->qef_eflag = QEF_EXTERNAL;
		    /* Retrievals and READONLY databases are READ ONLY */
		    if (qmode == PSQ_RETRIEVE ||
		    	sscb->sscb_ics.ics_db_access_mode == DMC_A_READ)
			qe_ccb->qef_qacc = QEF_READONLY;
		    else
			qe_ccb->qef_qacc = QEF_UPDATE;
		    qe_ccb->qef_intstate = 0;
		    qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;
		    qe_ccb->qef_no_xact_stmt =
			(sscb->sscb_req_flags & SCS_NO_XACT_STMT) != 0;
		    qe_ccb->qef_illegal_xact_stmt = FALSE;
		    if (qmode == PSQ_DGTT_AS_SELECT)
		       qe_ccb->qef_stmt_type |= QEF_DGTT_AS_SELECT;
		}
		/* Assign handle to stashed query plan */
		qe_ccb->qef_qso_handle = cquery->cur_qp.qso_handle;

                qe_ccb->qef_val_logkey = 0;

		/*
		** These are the "set" queries in Ingres.  They
		** are to be run to completion by QEF, although
		** control may return to SCF so that data may be
		** sent to the front end process.  The operation
		** will then be continued directly without any
		** any intervening queries having been run
		** as was checked above.
		*/

		/* If we are retrying after loading a table-proc QP,
		** restore the original query mode now.
		*/
		if (is_tproc_load_qp)
		{
		    STRUCT_ASSIGN_MACRO(cur_qp_saved, cquery->cur_qp);
		    qmode = sscb->sscb_qmode = qmode_saved;
		    is_tproc_load_qp = FALSE;
		}

                /* If RETRIEVE or FETCH or row producing procedure, then
		** determine output buffers.
		** Don't do this however if this is a return-back for
		** reloading a QP, output buffers are already positioned
		** and we don't want to touch them.
		*/
                if ((qmode == PSQ_RETRIEVE || qmode == PSQ_RETCURS || rowproc)
		  && !qef_wants_ruleqp)
		{
		    char *row_buf;

		    /* check output area is big enough for at least 1 row */
		    if (cquery->cur_tup_size > cscb->cscb_tuples_max)
		    {
			status = scs_realloc_outbuffer(scb);
		        if (status)
		        {
			    sc0e_0_put(status, 0);
			    sc0e_0_uput(status, 0);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    sscb->sscb_state = SCS_INPUT;
			    *next_op = CS_EXCHANGE;
			    qry_status = GCA_FAIL_MASK;
			    break;
		        }
		    }

                    /* Default to CS_OUTPUT & assume call-back for more data */
		    *next_op = CS_OUTPUT;

		    if ((sscb->sscb_flags & SCS_STAR) &&
			(qmode == PSQ_RETRIEVE))
		    {
			rqf_td_desc = &cquery->cur_rdesc.rd_rqf_desc;
			qe_ccb->qef_r3_ddb_req.qer_d10_tupdesc_p =
			    (PTR) rqf_td_desc;
		    }
                    if (    (singleton)  /* Real singleton or 1-row FETCH */
			&& ((cquery->cur_rdesc.rd_modifier & GCA_LO_MASK) == 0))
		    {
			qe_ccb->qef_rowcount = 1;
		        status = sc0m_allocate(0,
				sizeof(SC0M_OBJECT)+sizeof(QEF_DATA),
				DB_SCF_ID, (PTR)SCS_MEM, SCG_TAG,
				&cquery->cur_amem);
		        if (status)
		        {
			    sc0e_0_put(status, 0);
			    sc0e_0_uput(status, 0);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    sscb->sscb_state = SCS_INPUT;
			    *next_op = CS_EXCHANGE;
			    qry_status = GCA_FAIL_MASK;
			    if (cscb->cscb_batch_count > 0)
				cscb->cscb_eog_error = TRUE;
			    break;
		        }
			cquery->cur_qef_data =
			    (QEF_DATA *)((char *)cquery->cur_amem +
				sizeof(SC0M_OBJECT));
		    }
                    else                /* Need to allocate qef_data nodes */
		    {
			if ((cquery->cur_tup_size > 0)
			    && ((cquery->cur_rdesc.rd_modifier & GCA_LO_MASK) == 0))
			{
			    qe_ccb->qef_rowcount =
				(cscb->cscb_tuples_max / cquery->cur_tup_size);
			    /*
			    ** if row count is big, it might overflow
			    ** SC0M_ALLOC limit. So check it here
			    ** Reserve 2 times SC0M_OBJECT for alignment
			    */
			    if (((2 * sizeof(SC0M_OBJECT)) +
				(qe_ccb->qef_rowcount * sizeof(QEF_DATA)))
				> SC0M_MAXALLOC)
			      qe_ccb->qef_rowcount =
				(SC0M_MAXALLOC - 2 * sizeof(SC0M_OBJECT)) /
				  sizeof(QEF_DATA);
			}
			else
			{
			    qe_ccb->qef_rowcount = 1;
			    cquery->cur_tup_size = cscb->cscb_tuples_max;
			}
                        /*
			 ** Make sure not to exceed the user-requested FETCH
			 ** rows, even if the buffer could hold more.
			 */
                        if (   qmode == PSQ_RETCURS
                            && fetch_rows < qe_ccb->qef_rowcount)
                        {
                            qe_ccb->qef_rowcount = fetch_rows;
                        }

			/* Count the select, allocate qef-data elements if
			** needed, reset rowcount for QEF.  But only if we
			** haven't done so already.
			*/
			if (cquery->cur_state < CUR_RUNNING)
			{
                            if (qmode == PSQ_RETRIEVE || rowproc)
                              {
                                Sc_main_cb->sc_selcnt++;
                              }

			    /* If rowproc, might have set up buffers and then
			    ** had to regenerate the rowproc, don't do it
			    ** again.  Anything else, allocate buffers all
			    ** the time, even if there was junk in cur_amem
			    ** (which OUGHT to be null at this point.)
			    */
			    if ( !rowproc || NULL == cquery->cur_amem )
			    {
				status = sc0m_allocate(0,
					    sizeof(SC0M_OBJECT) +
						(qe_ccb->qef_rowcount
						  * sizeof(QEF_DATA)),
					    DB_SCF_ID,
					    (PTR)SCS_MEM,
					    CV_C_CONST_MACRO('Q','E','D','T'),
					    &cquery->cur_amem);
				if (status)
				{
				    sc0e_0_put(status, 0);
				    sc0e_0_uput(status, 0);
				    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
				    abort_action = DB_QSF_ID;
				    qry_status = GCA_FAIL_MASK;
				    sscb->sscb_state = SCS_INPUT;
				    if (cscb->cscb_batch_count > 0)
					cscb->cscb_eog_error = TRUE;
				    *next_op = CS_EXCHANGE;
				    break;
				}
			    }
                            if (qmode == PSQ_RETRIEVE || rowproc)
                                qe_ccb->qef_rowcount =
                                        min(Sc_main_cb->sc_irowcount,
                                            qe_ccb->qef_rowcount);
			}
			cquery->cur_qef_data =
			    (QEF_DATA *) ((char *)cquery->cur_amem
				   + sizeof(SC0M_OBJECT));
		    }
		    /* Format the QEF_DATA structures */
		    message = cscb->cscb_outbuffer;
		    message->scg_mask = SCG_NODEALLOC_MASK;
		    message->scg_mtype = GCA_TUPLES;
		    message->scg_msize = (qe_ccb->qef_rowcount
				    * cquery->cur_tup_size);
		    if (cscb->cscb_different == TRUE)
		    {
			message->scg_mdescriptor = cquery->cur_rdesc.rd_tdesc;
		    }
		    else
		    {
			message->scg_mdescriptor = NULL;
		    }

		    qe_ccb->qef_nextout = (QEF_DATA *)NULL;
		    qe_ccb->qef_output = data_buffer = cquery->cur_qef_data;
		    qe_ccb->qef_count = 0;

		    for (i = 0, row_buf = (char *) cscb->cscb_tuples;
			    i < qe_ccb->qef_rowcount;
			    i++, data_buffer++,
			    row_buf += cquery->cur_tup_size)
		    {
			data_buffer->dt_size = cquery->cur_tup_size;
			data_buffer->dt_data = (PTR) row_buf;
			data_buffer->dt_next =
			    (QEF_DATA *) ((char *) data_buffer + sizeof(QEF_DATA));
		    }
		    /* Scrollable cursor protocols may result in request for
		    ** 0 rows - avoid that case. */
		    if (qe_ccb->qef_rowcount > 0)
			(--data_buffer)->dt_next = 0;

		    /* End of "if row-producing query and not a QEF-wants-
		    ** nested procedure QP callback" section
		    */
		}
		else if (qef_wants_ruleqp)
		{
		    /*
		    ** QEF is mid-QP processing and needed another QP.
		    ** QEF will restore all the output-buffer-position
		    ** stuff so that it can continue where it left off.
		    ** Don't reset any QEF fields, but clear the flag
		    ** since we got qef the rule QP it wanted.
		    */
		    qef_wants_ruleqp = 0;
		}
		else if (qef_wants_eiqp)
		{
		    /*
		    ** QEF is mid-processing CREATE SCHEMA statement and needed
		    ** another QP. Don't reset any QEF fields but reset the
		    ** local flag to avoid bad error handling on return.
		    */
	     	    qef_wants_eiqp = 0;
		}

		sscb->sscb_cfac = DB_QEF_ID;
		/* Set query access mode to UPDATE */
		qe_ccb->qef_qacc = QEF_UPDATE;
		if (qmode == PSQ_REPCURS)
		    status = qef_call(QEQ_REPLACE, qe_ccb);
		else if (qmode == PSQ_DELCURS)
		    status = qef_call(QEQ_DELETE, qe_ccb);
                else if (qmode == PSQ_RETCURS)
		{
                    status = qef_call(QEQ_FETCH, qe_ccb);
		    curspos = ((QEF_RCB *)qe_ccb)->qef_curspos;
		    rowstat = ((QEF_RCB *)qe_ccb)->qef_rowstat;
		}
		else
		{
		    /* Retrievals and READONLY databases are READ ONLY */
		    if (qmode == PSQ_RETRIEVE ||
		    	sscb->sscb_ics.ics_db_access_mode == DMC_A_READ)
			qe_ccb->qef_qacc = QEF_READONLY;

		    status = qef_call(QEQ_QUERY, qe_ccb);
		}

                if (status != E_DB_OK
		  && qe_ccb->error.err_code == E_QE030F_LOAD_TPROC_QP)
                {
                    /* Remember tproc qp needs to be recompiled and reloaded */
                    if (!qmode_saved)
                        /* save the qmode before any tproc has been
                         * recompiled */
                        qmode_saved = qmode;

                    STRUCT_ASSIGN_MACRO(cquery->cur_qp, cur_qp_saved);
                    is_tproc_load_qp = TRUE;
		    status = E_DB_ERROR;	/* Eschew warn status */
                }

		/* Note, this looks slightly premature, but rowprocs can't
		** need byref tuples so it's ok.
		*/
		rowprocdone = FALSE;
		if (rowproc && status != E_DB_OK &&
			qe_ccb->error.err_code == E_QE0015_NO_MORE_ROWS)
		    rowprocdone = TRUE;

/* bug 70170 */
		if ( (qmode == PSQ_RETRIEVE || rowproc) && (rp_handle)
			&& (status == 0))
		    cquery->cur_qp.qso_handle=rp_handle;


		if (status && (qe_ccb->error.err_code ==
			       E_QE0306_ALLOCATE_BYREF_TUPLE))
		{
		    /* Allocate a GCA tuple data message and call back QEF */
		    status = scs_allocate_tdata(scb,
						(GCA_TD_DATA *)qef_data.dt_data,
						&tdata);
		    if (status)
		    {
			*next_op = CS_EXCHANGE;
			qry_status = GCA_FAIL_MASK;
			break;
		    }
		    qef_data.dt_data = (PTR)tdata->scg_marea;
		    qef_data.dt_size = tdata->scg_msize;
		    status = qef_call(QEQ_QUERY, qe_ccb);
		}

		if (qe_ccb->qef_illegal_xact_stmt)
		    qry_status = GCA_ILLEGAL_XACT_STMT;
		if (!status)
		    qe_ccb->error.err_code = 0;
		sscb->sscb_cfac = DB_SCF_ID;

		if ((sscb->sscb_flags & SCS_STAR)
		    && (qmode == PSQ_EXEDBP))
		{
		    /* Return status was stashed previously */
		    qe_ccb->qef_dbp_status =
				    cscb->cscb_rpdata->gca_retstat;
		    cquery->cur_amem = NULL;
		    cquery->cur_qef_data = NULL;
		}

		/* Note that "success" includes a WARN with no-more-rows. */
		if (DB_SUCCESS_MACRO(status))
		{
		    sscb->sscb_noretries = 0;

		    if (qe_ccb->qef_remnull)
			cquery->cur_result |= GCA_REMNULS_MASK;

		    if (qmode == PSQ_CALLPROC)
		    {
			cquery->cur_row_count = -1;
		    }
		    else
		    {
			cquery->cur_row_count += qe_ccb->qef_rowcount;

                        /* get current "val_logkey" state, and copy logkey if
                        ** necessary.
                        */
                        if ((cquery->cur_val_logkey = qe_ccb->qef_val_logkey))
                        {
                            STRUCT_ASSIGN_MACRO(qe_ccb->qef_logkey,
                                        cquery->cur_logkey);

                        }
                    }

		    /* Queue rows for output if row-returning query */
		    if (qmode == PSQ_RETRIEVE || qmode == PSQ_RETCURS
		      || rowproc)
		    {
			if (status = scs_emit_rows(qmode, rowproc, scb, qe_ccb, message))
			{
			    if (qe_ccb && !qe_ccb->error.err_code)
				qe_ccb->error.err_code = E_QE0015_NO_MORE_ROWS;
			    scs_cleanup_output(qmode, scb, message);
			    qry_status |= GCA_FAIL_MASK;	/* in case.. */
			}
		    }
		    else
		    {
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
		    }

                    /*
                    ** If no more rows, or a singleton RETRIEVE (or FETCH), or
                    ** a FETCH that has reached (or exceeded) the requested
                    ** rows ==> then this is the end of the request.
                    */

                    if (   (qe_ccb->error.err_code == E_QE0015_NO_MORE_ROWS)
                        || (singleton)
                        || (   qmode == PSQ_RETCURS
                            && (sscb->sscb_cursor.curs_frows <= cquery->cur_row_count
				|| cquery->cur_scrollable)
                           )
                       )
		    {
			STRUCT_ASSIGN_MACRO(qe_ccb->qef_comstamp, sscb->sscb_comstamp);

			if (cquery->cur_amem)
			{
			    sc0m_deallocate(0, &cquery->cur_amem);
			    cquery->cur_qef_data = NULL;
			}
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
                        /* Unexpected endret if RETRIEVE ended with more rows.
			** i.e. singleton query but qef isn't done, make it
			** be done.
			*/
                        if (   qmode == PSQ_RETRIEVE
                            && qe_ccb->error.err_code != E_QE0015_NO_MORE_ROWS)
			{
			    qef_call(QEQ_ENDRET, qe_ccb);
			}
                        /* If no more rows for FETCH mark response status */
                        if (   qmode == PSQ_RETCURS
                            && qe_ccb->error.err_code == E_QE0015_NO_MORE_ROWS)
                        {
                            qry_status = GCA_END_QUERY_MASK;
                        }
		    }

		    /* If FETCH of a repeat cursor, clear sscb_thandle so
		    ** we don't free parameters yet. */
		    if (qmode == PSQ_RETCURS)
		    {
			csi = sscb->sscb_cursor.curs_csi;
			csi_no = sscb->sscb_cursor.curs_curno;
			if ((csi->csitbl[csi_no].csi_state
				& CSI_REPEAT_MASK) &&
			csi->csitbl[csi_no].csi_rep_parm_handle != (PTR) NULL &&
			sscb->sscb_thandle != (PTR) NULL)
			{
			    /* First, free the object. */
			    qs_ccb = sscb->sscb_qsccb;
			    qs_ccb->qsf_obj_id.qso_handle = sscb->sscb_thandle;
			    status = qsf_call(QSO_UNLOCK, qs_ccb);
			    if (DB_FAILURE_MACRO(status))
			    {
				qry_status = GCA_FAIL_MASK;
				sscb->sscb_state = SCS_INPUT;
				*next_op = CS_EXCHANGE;
				scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				    E_SC0210_SCS_SQNCR_ERROR);
				sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    }

			    sscb->sscb_thandle = (PTR) NULL;
			    sscb->sscb_tlk_id = 0;
			}
		    }
		    break;
		}
		else
		{
		    /* Check for special "retry-type" errors.
		    ** These have to be handled before hosing the output
		    ** buffering, since some rows may have been emitted
		    ** already (row-producing procedures).
		    */
		    if ( !sscb->sscb_interrupt
		      && ( (qe_ccb->error.err_code == E_QE0023_INVALID_QUERY)
			|| (qe_ccb->error.err_code == E_QE0014_NO_QUERY_PLAN)
			|| (qe_ccb->error.err_code == E_QE0119_LOAD_QP)
			|| (qe_ccb->error.err_code == E_QE0301_TBL_SIZE_CHANGE)
			|| (qe_ccb->error.err_code == E_QE0303_PARSE_EXECIMM_QTEXT)
                        || is_tproc_load_qp)
		       )
		    {
			/* bug 38565 -- don't count table size increase against
			** number of total retries.
			*/
			if (qe_ccb->error.err_code == E_QE0301_TBL_SIZE_CHANGE)
			    sscb->sscb_noretries = 0;
			if (qe_ccb->qef_qso_handle == 0)
			{
			    if (ult_check_macro(&sscb->sscb_trace, SCS_TEVENT, NULL, NULL))
			    {
				if (qe_ccb->qef_intstate & QEF_DBPROC_QP)
				    sc0e_trace("QEF procedure rebuild request");
				else if (qmode == PSQ_EXEDBP)
				    sc0e_trace("DB Procedure rebuild requested");
				else
				    sc0e_trace("QEP unknown or invalid -- Requesting resend");
			    }

			    if (   (qe_ccb->qef_intstate & QEF_DBPROC_QP)
				&& (  (qe_ccb->error.err_code ==
							E_QE0023_INVALID_QUERY)
				    || (qe_ccb->error.err_code ==
							E_QE0119_LOAD_QP)
				    || (qe_ccb->error.err_code ==
							E_QE0301_TBL_SIZE_CHANGE)
                                    || is_tproc_load_qp
				   )
			       )
			    {
				/*
				** QEF wants a QP to be reloaded.  Check if this
				** is the same QP as last time (supress retry
				** counter if not) and copy it for later use.
				** Note that this flag implies a nested DBP
				** call;  it might be a rule, or it might be
				** a user nested call, we don't care which.
				*/
				qef_wants_ruleqp = 1;
				if (   (qef_psf_alias.qso_n_dbid < 0)
				    || (MEcmp((PTR)&qe_ccb->qef_dbpname,
					      (PTR)&qef_psf_alias,
					      sizeof(qef_psf_alias)) != 0)
				   )
				{
				    /*
				    ** First time through or alias name not the
				    ** same.  Reset "retry" counter.
				    */
				    sscb->sscb_noretries = 0;
				}
				/* Save full alias */
				STRUCT_ASSIGN_MACRO(qe_ccb->qef_dbpname,
						    qef_psf_alias);
			    }
			    else if (qmode == PSQ_EXEDBP)
			    {
				/* We tried to run a DBP but by the time we
				** got to QEF, it wasn't there any more.
				** Do a full cleanup and recreate/retry.
				*/
				recreate = 1;
			    }
			    else
			    {
				/* REPEAT query that's not there any more */
				op_ccb = sscb->sscb_opccb;
				if (qsf_object == 1)
				{
				    /* This is a 5.0 style repeat query. Yes,
				    ** this is not obvious, but it's true.
				    ** This means that OPF will need the repeat
				    ** query parameters when the define query
				    ** is executed. PSF won't be able to
				    ** provide the parameters in the qtree
				    ** as it does with 6.0 FE's, since 5.0
				    ** didn't send data with the define.
				    */
				    op_ccb->opf_repeat =
						    qe_ccb->qef_param->dt_data;
				}
				else
				{
				    op_ccb->opf_repeat = NULL;
				}
				/*
				**  delete qep if qef didn't - don't bother, it
				**  did
				*/
				repeat_redef = 1;
				qsf_object = 0;	    /* don't delete the QP  */
						    /* -- need to save it   */
						    /* for OPF.		    */
				qry_status = GCA_FAIL_MASK | GCA_RPQ_UNK_MASK;
				/* Tell top of retry loop to give up rather
				** than actually retry.  repeat query goes back
				** to client for re-send of the query defn.
				*/
				sscb->sscb_state = SCS_INPUT;
			    }
			}
			/*
			** Check for failure due to query flattening.
			** If so, set flag to bypass flattening on retry.
			*/
			if (qe_ccb->qef_intstate & QEF_ZEROTUP_CHK)
			{
			    cquery->cur_subopt = TRUE;
			    qe_ccb->qef_intstate &= ~QEF_ZEROTUP_CHK;

			    if ((qmode == PSQ_RETCURS) &&
				(cquery->cur_state == CUR_FIRST_TIME))
			    {
				/*
				** fetch failed due to zerotup check.
				** cursor context already removed by
				** qeq_cleanup in qeq_fetch().
				*/
				retry_fetch = TRUE;
				scs_cursor_remove(scb, &csi->csitbl[csi_no]);
			    }
			}

			/* Check if EXECUTE IMMEDIATE text needs to be parsed.
                        ** If so, reset "retry" counter.
                        */
                        if ((qe_ccb->qef_intstate & QEF_EXEIMM_PROC)
                         && (qe_ccb->error.err_code ==
                               E_QE0303_PARSE_EXECIMM_QTEXT))
                        {
                            qef_wants_eiqp = 1;
                            sscb->sscb_noretries = 0;
                        }

			/* If we decided that qef wants a rule QP, leave
			** the output buffers alone.  Anything else, clean
			** them up now -- we're liable to exit the sequencer
			** via any number of devious paths, and leftovers
			** will badly confuse things later on.  Or, if it's
			** a DBP recreate, we want a clean slate to work on.
			*/
			if (!qef_wants_ruleqp)
			{
			    scs_cleanup_output(qmode, scb, message);
			    qry_status |= GCA_FAIL_MASK;	/* in case.. */
			}

			continue;
		    }

		    /* It's not a qef-retry type of error. */
		    scs_cleanup_output(qmode, scb, message);
		    if (sscb->sscb_interrupt)
			break;
		    qry_status |= GCA_FAIL_MASK; /* qry_status may already be
						 ** set at this point */
		    scs_qef_error(status, qe_ccb->error.err_code, DB_SCF_ID, scb);
		    ret_val = E_DB_OK;
		    sscb->sscb_state = SCS_INPUT;
		    STRUCT_ASSIGN_MACRO(qe_ccb->qef_comstamp, sscb->sscb_comstamp);
		    cquery->cur_row_count = -1;
		    *next_op = CS_EXCHANGE;
		    break;
		}	    /* QEF will destroy the plan */
		break;


	    case PSQ_REPDYN:
	    {
		sscb->sscb_nostatements += 1;
		/*
		** Do stuff normally done after OPF compile for PSQ_DEFCURS.
		*/
		qs_ccb = sscb->sscb_qsccb;
		STRUCT_ASSIGN_MACRO(ps_ccb->psq_result, qs_ccb->qsf_obj_id);
		STRUCT_ASSIGN_MACRO(ps_ccb->psq_result, cquery->cur_qp);
		qs_ccb->qsf_lk_state = QSO_SHLOCK;
		status = qsf_call(QSO_GETHANDLE, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC0210_SCS_SQNCR_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    sscb->sscb_qmode = 0;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    qry_status = GCA_FAIL_MASK;
		    break;
		}
		/* GCA_TD_DATA */
		cquery->cur_rdesc.rd_tdesc = scs_save_tdesc(scb,
		    (PTR) ((QEF_QP_CB *) qs_ccb->qsf_root)->qp_sqlda);
		cquery->cur_rdesc.rd_modifier =
		    ((GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc)->gca_result_modifier;
		dynqp_handle = qs_ccb->qsf_obj_id.qso_handle;
		dynqp_got = TRUE;
	    }	/* then drop into PSQ_DEFCURS */

	    case PSQ_DEFCURS:
	    {
		bool        retry_qry;

		/*
		** To define a cursor, the query must be parsed, optimized,
		** and opened via qef.  The parse and optimization was taken
		** care of above;  therefore, at this point, we must simply
		** call qef to open the query.
		**
		** In fact, if this is PSQ_REPDYN, it is an open of an already
		** compiled repeat synamic select and, once again, the parse
		** and optimization have already been done.
		*/
		ps_ccb = sscb->sscb_psccb;
		qe_ccb = sscb->sscb_qeccb;
		qs_ccb = sscb->sscb_qsccb;

		/* See if parms need to be copied for a repeat dynamic
		** cursor. */
		if ((((QEF_QP_CB *)qs_ccb->qsf_root)->qp_status & QEQP_REPDYN)
			&& sscb->sscb_thandle)
		{
		    PSQ_QDESC	*qdesc = (PSQ_QDESC *)sscb->sscb_troot;
		    PTR		*parmp;

		    if (qdesc && qdesc->psq_dnum > 3)
		    {
			/* Make copy of db_data's in 4th through nth parms. */
			qe_ccb->qef_pcount = qdesc->psq_dnum - 3;
			qs_ccb->qsf_sz_piece = sizeof(QEF_PARAM) +
				sizeof(PTR) * (qdesc->psq_dnum - 2);
			qs_ccb->qsf_obj_id.qso_handle = sscb->sscb_thandle;
			qs_ccb->qsf_lk_id = sscb->sscb_tlk_id;
			status = qsf_call(QSO_PALLOC, qs_ccb);
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC021C_SCS_INPUT_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    break;
			}

			qe_ccb->qef_param = (QEF_PARAM *) qs_ccb->qsf_piece;
			qe_ccb->qef_param->dt_next = (QEF_PARAM *) NULL;
			qe_ccb->qef_param->dt_size = qe_ccb->qef_pcount;
			parmp = (PTR *)(qs_ccb->qsf_piece +
							sizeof(QEF_PARAM));
			qe_ccb->qef_param->dt_data = parmp;
			parmp[0] = NULL;

			for (i = 1; i <= qe_ccb->qef_pcount; i++)
			    parmp[i] = qdesc->psq_qrydata[i+2]->db_data;

		    }
		}

		if (dynqp_got)
		{
		    /* Free the lock. */
		    qs_ccb->qsf_obj_id.qso_handle = dynqp_handle;
		    status = qsf_call(QSO_UNLOCK, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC0210_SCS_SQNCR_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sscb->sscb_qmode = 0;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			qry_status = GCA_FAIL_MASK;
			break;
		    }
		    dynqp_got = FALSE;
		}

		status = scs_def_curs(scb, (i4) PSQ_DEFCURS,
		    &ps_ccb->psq_cursid,
		    cquery->cur_qp.qso_handle,
		    retry_fetch,
		    (QEF_QP_CB *) qs_ccb->qsf_root,
		    &message, &qry_status, next_op, &retry_qry, &ret_val);

	        if (ult_always_trace())
	        {
		    void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
		    if (f)
		    {
			char tmp[20+20+DB_CURSOR_MAXNAME+20+1];
			STprintf(tmp,"(ID=%d/%d)(%.*s)",
			    cquery->cur_qname.db_cursor_id[0],
			    cquery->cur_qname.db_cursor_id[1],
			    DB_CURSOR_MAXNAME, ps_ccb->psq_cursid.db_cur_name);
			ult_print_tracefile(f,SC930_LTYPE_ADDCURSORID,tmp);
			ult_close_tracefile(f);
		    }
	        }

                if (qe_ccb->error.err_code == E_QE030F_LOAD_TPROC_QP)
                {
                    /* Cursor not opened. The table procedure QP
                     * needs to be recompiled and reloaded.
                     */

                    /* Save current mode for reopening the cursor */
                    qmode_saved = qmode;

                    /* if ( qe_ccb->qef_intstate & QEF_DBPROC_QP) */
                    /* QEF wants a QP to be reloaded */
                    qef_wants_ruleqp = 1;

                    /* Reset retry counter */
                    sscb->sscb_noretries = 0;
                    sscb->sscb_state = SCS_CONTINUE;

                    /* Save the information required for reopening
                     * the cursor and callback to qef.
                     */
                    STRUCT_ASSIGN_MACRO(cquery->cur_qp, cur_qp_saved);
                    STRUCT_ASSIGN_MACRO(*(QSO_NAME *)&qe_ccb->qef_dbpname,
                                        qef_psf_alias);
                    STRUCT_ASSIGN_MACRO(ps_ccb->psq_cursid, cursid_saved);
                    dsh_saved = qe_ccb->qef_cb->qef_dsh;

                    /* Bump up open cursor count otherwise top dsh
                     * will be released.
                     */
                    qe_ccb->qef_cb->qef_open_count++;

                    /* Ready to recompile and reload tproc QP */
                    tproc_cur_load_qp = TRUE;
                    continue;
                }

		if (retry_qry)
		    continue;

		if (retry_fetch)
		{
		    /*
		    ** we have reparsed query text and generated qep
		    ** now retry fetch
		    */
		    retry_fetch = FALSE;
		    internal_retry = 0;
		    scc_dispose(scb);
		    sscb->sscb_state = SCS_PROCESS;
		    sscb->sscb_qmode = PSQ_RETCURS;
		    continue;
		}
		else if (status == E_DB_OK)
		{
		    sscb->sscb_thandle = (PTR)NULL;
		    sscb->sscb_troot = (PTR)NULL;
		    sscb->sscb_tlk_id = 0;
		}
		break;
	    }

	    case PSQ_CLSCURS:
		/*
		** For all of these calls, psf or scs_input will have
		** built the control block and placed it into
		** sscb_cquery.cur_qp
		** for us.  Simply qef_call().
		*/
		qe_ccb = sscb->sscb_qeccb;
		qe_ccb->qef_eflag = QEF_EXTERNAL;
		qe_ccb->qef_cb = sscb->sscb_qescb;
		qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
                i = scs_csr_find(scb);
		if (i == sscb->sscb_cursor.curs_limit)
		{
		    qry_status = GCA_FAIL_MASK;
		    sc0e_0_uput(E_SC021E_CURSOR_NOT_FOUND, 0);
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}

                csi = sscb->sscb_cursor.curs_csi;

		/*
		** These are set here so as to allow their easy alteration in
		** the case of incomplete tuples being returned from QEF
		*/

		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;

		qe_ccb->qef_qso_handle = csi->csitbl[i].csi_qp;

                sscb->sscb_cfac = DB_QEF_ID;

		/* If this is a STAR server, the remote cursor has already
		** been dispensed with in the EXECQRY */
		/* Actually, call close anyway and accept that the cursor
		** may have been dispensed with, so check the open_count
		** in qeq_close.  (kibro01) b119002 b114735
		*/
                status = qef_call(QEQ_CLOSE, qe_ccb);
                cquery->cur_row_count = -1;
                sscb->sscb_cfac = DB_SCF_ID;
                if (DB_FAILURE_MACRO(status))
                {
                    qry_status = GCA_FAIL_MASK;
                    if (sscb->sscb_interrupt)
                       break;
                    scs_qef_error(status, qe_ccb->error.err_code,
                                  DB_SCF_ID, scb);
		}

		/*
		** In this case, it is necessary to tell the parser to remove
		** whatever information it has around from it's cache.
		*/

		scs_cursor_remove(scb, &csi->csitbl[i]);

		if (QEF_X_MACRO(sscb->sscb_qescb) == 0)
		{
		    sscb->sscb_ciu = 0;
		}
		break;

	    case PSQ_DEFQRY:
	    case PSQ_QRYDEFED:
		message = cscb->cscb_outbuffer;
		message->scg_next = (SCC_GCMSG *) &cscb->cscb_mnext;
		message->scg_prev = cscb->cscb_mnext.scg_prev;
		cscb->cscb_mnext.scg_prev->scg_next = message;
		cscb->cscb_mnext.scg_prev = message;

		message->scg_mask = SCG_NODEALLOC_MASK;
		message->scg_mtype = GCA_QC_ID;
		message->scg_msize = sizeof(GCA_ID);
		((GCA_ID *) cscb->cscb_tuples)->gca_index[0] =
			    ps_ccb->psq_cursid.db_cursor_id[0];
		((GCA_ID *) cscb->cscb_tuples)->gca_index[1] =
			    ps_ccb->psq_cursid.db_cursor_id[1];
	        if (ult_always_trace())
	        {
                    void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
                    if (f)
                    {
			char tmp[20+20+DB_CURSOR_MAXNAME+20+1];
                        STprintf(tmp,"(ID=%d/%d)(%.*s)",
			    cquery->cur_qname.db_cursor_id[0],
			    cquery->cur_qname.db_cursor_id[1],
			    DB_CURSOR_MAXNAME, ps_ccb->psq_cursid.db_cur_name);
			ult_print_tracefile(f,SC930_LTYPE_ADDCURSORID,tmp);
                        ult_close_tracefile(f);
                    }
                }

		/* DON'T assume GCA_MAXNAME > DB_CURSOR_MAXNAME */
		MEmove(sizeof(ps_ccb->psq_cursid.db_cur_name),
		    (PTR)&ps_ccb->psq_cursid.db_cur_name, ' ',
		    sizeof(((GCA_ID *)cscb->cscb_tuples)->gca_name),
		    (PTR)&(((GCA_ID *)cscb->cscb_tuples)->gca_name));

		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    case PSQ_EXCURS:
		/* {@fix_me@} */
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;


	    /* Distributed Transaction control statements */

	    /* Note these are all parsed "with xa_id =" forms, not GCA messages */
	    case PSQ_XA_COMMIT:
	    case PSQ_XA_ROLLBACK:
	    case PSQ_XA_PREPARE:
	    {
		DB_DIS_TRAN_ID temp_db_distid;
		LK_LKID		lkid;
		SCD_SCB	       *xa_scb;
		bool           xid_found = FALSE;
		QEF_RCB	       *xa_qe_ccb;
		PSQ_CB	       *xa_ps_ccb;
		SCD_SCB        *scan_scb;
		i4            cnt;
		DB_STATUS	lock_status;
		bool		force_abort = FALSE;

		temp_db_distid.db_dis_tran_id_type = DB_XA_DIS_TRAN_ID_TYPE;
		status = conv_to_struct_xa_xid(ps_ccb->psq_distranid,
		    STlength(ps_ccb->psq_distranid),
		    &temp_db_distid.db_dis_tran_id.db_xa_extd_dis_tran_id );
		if (DB_FAILURE_MACRO( status ))
		{
		    qry_status = GCA_FAIL_MASK;
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}

		if (sscb->sscb_state == SCS_CONTINUE)
		{
		    /*
		    ** We actually expect this to be always true.
		    ** In these cases, PSF will have produced the appropriate
		    ** control block in qsf.
		    ** Get an exclusive lock so we can delete the object now,
		    ** we don't need it to process the commit for another scb.
		    */
		    qs_ccb = sscb->sscb_qsccb;
		    qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
		    qs_ccb->qsf_lk_state = QSO_EXLOCK;
		    status = qsf_call(QSO_LOCK, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			qry_status = GCA_FAIL_MASK;
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			break;
		    }
		    qe_ccb = (QEF_RCB *) qs_ccb->qsf_root;
		    cquery->cur_qe_cb = (PTR) qe_ccb;
		    status = qsf_call(QSO_DESTROY, qs_ccb);

		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status,qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
                        sscb->sscb_state = SCS_INPUT;
                        *next_op = CS_EXCHANGE;
                        break;
		    }
		}
		else
		{
		    qry_status = GCA_FAIL_MASK;
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}

		for (scan_scb = (SCD_SCB *)scb, cnt = 0;
		      !xid_found;
		       scan_scb = (SCD_SCB *)scan_scb->cs_scb.cs_next, cnt++)
		{

		    if (cnt > 0 && scan_scb == scb)
			break;

		    /*
		    ** compare global tran id specified with the global
		    ** tran ids in the scb list, find associated local
		    ** transaction.
		    */
		    if (!(XA_XID_EQL_MACRO(&temp_db_distid,
			&scan_scb->scb_sscb.sscb_dis_tran_id)))
			    continue;

		    xa_scb = scan_scb;
		    xid_found = TRUE; /* found matching xid */

		    /*
		    ** Proceed as if sscb->sscb_state was SCS_PROCESS
		    ** because we are now working with the xa_scb
		    */
		    xa_qe_ccb = xa_scb->scb_sscb.sscb_qeccb;
		    xa_qe_ccb->qef_db_id =
			xa_scb->scb_sscb.sscb_ics.ics_opendb_id;
		    xa_qe_ccb->qef_flag = 0;
		    xa_qe_ccb->qef_spoint= NULL;

		    xa_qe_ccb->qef_modifier = QEF_MSTRAN;
		    xa_qe_ccb->qef_eflag = QEF_EXTERNAL;
		    xa_qe_ccb->qef_cb = xa_scb->scb_sscb.sscb_qescb;
		    xa_qe_ccb->qef_sess_id = xa_scb->cs_scb.cs_self;

		    /*
		    ** ALWAYS set this to zero
		    ** Ignore sscb_req_flags & SCS_NO_XACT_STMT
		    ** This flag is set so that dynamic transaction
		    ** FIX ME
		    */
		    xa_qe_ccb->qef_no_xact_stmt  = 0;

		    xa_qe_ccb->qef_illegal_xact_stmt = FALSE;

		    lock_status = scs_xa_lock_scb(scb, xa_scb, &lkid, LK_NOWAIT);
		    if (lock_status != E_DB_OK && qmode != PSQ_XA_ROLLBACK)
		    {
			xid_found = 0;
			break;
		    }

		    /* Validate xa_scb after it is locked */
		    if (!(XA_XID_EQL_MACRO(&temp_db_distid,
			&xa_scb->scb_sscb.sscb_dis_tran_id)))
		    {
			scs_xa_unlock_scb(scb, xa_scb, &lkid);
			xid_found = 0;
			break;
		    }

		    if (qmode == PSQ_XA_COMMIT)
		    {
			status = qef_call(QET_SCOMMIT, xa_qe_ccb);
			scs_xa_unlock_scb(scb, xa_scb, &lkid);
		    }
		    else if (qmode == PSQ_XA_ROLLBACK)
		    {
			QEF_RCB *x = sscb->sscb_qeccb;

			/*
			** check for set trace point qe40
			** If qe40, this is XA_ROLLBACK before XA_END
			** We need to interrupt this session,
			** it may be waiting on a lock.
			** The interrupt should cause the session to rollback
			** transaction, NOT rollback to savepoint
			*/
			if (lock_status != E_DB_OK ||
				(ult_check_macro(&(x->qef_cb->qef_trace),
				QEF_XAFABORT, NULL, NULL)))
			{
			    ult_clear_macro(&(x->qef_cb->qef_trace),
				QEF_XAFABORT);

			    /* Unlock before force abort */
			    if (lock_status == E_DB_OK)
				scs_xa_unlock_scb(scb, xa_scb, &lkid);

			    TRdisplay("XA_ROLLBACK: tms session %x force aborting session %x\n",
					scb, xa_scb);

			    qe_ccb->qef_cb = sscb->sscb_qescb;
			    qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
			    qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;

			    /* Always send distributed tranid */
			    STRUCT_ASSIGN_MACRO(temp_db_distid,
					    qe_ccb->qef_cb->qef_dis_tran_id);

			    MEfill(SCS_ACT_SIZE, '\0',
					      sscb->sscb_ics.ics_act1);
			    MEcopy("Abort XA transaction", 20, sscb->sscb_ics.ics_act1);
			    sscb->sscb_ics.ics_l_act1 = 20;

			    status = qef_call(QET_XA_ROLLBACK, qe_ccb);
			    sscb->sscb_ics.ics_l_act1 = 0;
			    force_abort = TRUE;
			    status = E_DB_OK;
			}
			else
			{
			    MEfill(SCS_ACT_SIZE, '\0',
				xa_scb->scb_sscb.sscb_ics.ics_act1);
			    MEcopy("Aborting", 8,
				xa_scb->scb_sscb.sscb_ics.ics_act1);
			    xa_scb->scb_sscb.sscb_ics.ics_l_act1 = 8;
			    status = qef_call(QET_ROLLBACK, xa_qe_ccb);
			    xa_scb->scb_sscb.sscb_ics.ics_l_act1 = 0;
			    scs_xa_unlock_scb(scb, xa_scb, &lkid);
			}
		    }
		    else if (qmode == PSQ_XA_PREPARE)
		    {
			STRUCT_ASSIGN_MACRO(
			    xa_scb->scb_sscb.sscb_dis_tran_id,
			    xa_qe_ccb->qef_cb->qef_dis_tran_id);
			status = qef_call(QET_SECURE, xa_qe_ccb);
			scs_xa_unlock_scb(scb, xa_scb, &lkid);
		    }

		    if (DB_FAILURE_MACRO(status))
		    {
			if (xa_scb->scb_sscb.sscb_interrupt)
			    break;
			qry_status = GCA_FAIL_MASK;
			scs_qef_error(status, xa_qe_ccb->error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR, xa_scb);
			scd_note(status, DB_QEF_ID);
		    }
		    else if (!force_abort)
		    {
			if (qmode != PSQ_XA_PREPARE)
			{
			    xa_scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.gtrid_length = 0;
			    xa_scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.db_xa_extd_dis_tran_id.db_xa_dis_tran_id.bqual_length = 0;
			}

			xa_ps_ccb = xa_scb->scb_sscb.sscb_psccb;
			xa_scb->scb_sscb.sscb_cpy_qeccb = NULL;
			status = psq_call(PSQ_COMMIT, xa_ps_ccb,
				xa_scb->scb_sscb.sscb_psscb);
			if (DB_FAILURE_MACRO(status))
			{
			    if (xa_ps_ccb->psq_error.err_code ==
				E_PS0002_INTERNAL_ERROR)
			    {
				sc0e_0_put(E_SC0215_PSF_ERROR, 0);
				sc0e_0_put(xa_ps_ccb->psq_error.err_code, 0);
				scd_note(status, DB_PSF_ID);
				ret_val = E_DB_ERROR;
			    }
			}

			if (xa_scb->scb_sscb.sscb_ciu)
			{
			    /*
			    **  When ending a xact, make sure that all
			    **  psf cursors are closed
			    */

			    scs_cursor_remove(xa_scb, 0);
			}
		    	if (xa_scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc)
		    	{
			    scs_save_tdesc(xa_scb, (PTR)NULL);
		    	}
		    }

		    if (xa_qe_ccb->qef_illegal_xact_stmt)
			qry_status |= GCA_ILLEGAL_XACT_STMT;

		    if (!force_abort)
			STRUCT_ASSIGN_MACRO(xa_qe_ccb->qef_comstamp,
			xa_scb->scb_sscb.sscb_comstamp);

		    xa_qe_ccb = 0;  /* since it isn't the real one */

		    break; /* always */
		}

		if (!xid_found)
		{
		    qry_status = GCA_FAIL_MASK;
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    if (cscb->cscb_batch_count > 0)
			cscb->cscb_eog_error = TRUE;
		    break;
		}

		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;
	    }

	    /* GCA XA messages */
	    case PSQ_GCA_XA_START:
	    case PSQ_GCA_XA_END:
	    case PSQ_GCA_XA_PREPARE:
	    case PSQ_GCA_XA_COMMIT:
	    case PSQ_GCA_XA_ROLLBACK:
	    {
		GCA_XA_DATA		*GCAdata;
		DB_DIS_TRAN_ID		*SessDisTranId = &sscb->sscb_dis_tran_id;
		DB_DIS_TRAN_ID		DisTranId;
		DB_XA_EXTD_DIS_TRAN_ID	*ExtId = &DisTranId.db_dis_tran_id.
						db_xa_extd_dis_tran_id;
		DB_XA_DIS_TRAN_ID 	*Xid = &ExtId->db_xa_dis_tran_id;
		i4			*err_code;
		i4			*QefErrcode;
		i4			*XAerr_code = &cscb->cscb_rdata->gca_errd5;

		GCAdata = (GCA_XA_DATA*)cscb->cscb_gci.gca_data_area;

		/* Make a server-format distributed tran id from GCA form */

		DisTranId.db_dis_tran_id_type = DB_XA_DIS_TRAN_ID_TYPE;
		Xid->formatID = GCAdata->gca_xa_xid.formatID;
		Xid->gtrid_length = GCAdata->gca_xa_xid.gtrid_length;
		Xid->bqual_length = GCAdata->gca_xa_xid.bqual_length;
		MEcopy(GCAdata->gca_xa_xid.data, sizeof(Xid->data), Xid->data);
		ExtId->branch_seqnum = GCAdata->gca_xa_xid.branch_seqnum;
		ExtId->branch_flag = GCAdata->gca_xa_xid.branch_flag;

		/* Prime the QEF_RCB for the call */
		qe_ccb = sscb->sscb_qeccb;
		qe_ccb->qef_cb = sscb->sscb_qescb;
		err_code = &qe_ccb->error.err_code;
		QefErrcode = &qe_ccb->qef_cb->qef_error_qef_code;
		qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
		qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;
		qe_ccb->qef_modifier = 0; /* or qef_flag? */
		qe_ccb->qef_flag = 0;
		STRUCT_ASSIGN_MACRO(DisTranId, qe_ccb->qef_cb->qef_dis_tran_id);

		MEfill(SCS_ACT_SIZE, '\0', sscb->sscb_ics.ics_act1);
		*err_code = 0;
		*QefErrcode = 0;
		*XAerr_code = XA_OK;
		status = E_DB_OK;

		if ( qmode == PSQ_GCA_XA_START )
		{
		    /* Can't start a new one if one is already started */
		    if ( SessDisTranId->db_dis_tran_id_type )
		    {
			*err_code = E_QE006E_TRAN_IN_PROGRESS;
			status = E_DB_ERROR;
		    }
		    else
		    {
			/* xa_start(TMJOIN) ? */
			if ( GCAdata->gca_xa_flags & GCA_XA_FLG_JOIN )
			{
			    qe_ccb->qef_modifier = QET_XA_START_JOIN;

			    MEcopy("Start(join) XA transaction", 26, sscb->sscb_ics.ics_act1);
			    sscb->sscb_ics.ics_l_act1 = 26;
			}
			else
			{
			    MEcopy("Start XA transaction", 20, sscb->sscb_ics.ics_act1);
			    sscb->sscb_ics.ics_l_act1 = 20;
			}

			status = qef_call(QET_XA_START, qe_ccb);
		    }
		}
		else if ( qmode == PSQ_GCA_XA_END )
		{
		    /* Can't end what was not started */
		    if ( !(DB_DIS_TRAN_ID_EQL_MACRO(DisTranId, *SessDisTranId)) )
		    {
			*err_code = E_QE006F_TRAN_NOT_IN_PROGRESS;
			status = E_DB_ERROR;
		    }
		    else
		    {
			/* xa_end(TMFAIL) ? */
			if ( GCAdata->gca_xa_flags & GCA_XA_FLG_FAIL )
			{
			    qe_ccb->qef_modifier = QET_XA_END_FAIL;

			    MEcopy("End(fail) XA transaction", 24, sscb->sscb_ics.ics_act1);
			    sscb->sscb_ics.ics_l_act1 = 24;
			}
			else
			{
			    MEcopy("End XA transaction", 18, sscb->sscb_ics.ics_act1);
			    sscb->sscb_ics.ics_l_act1 = 18;
			}

			status = qef_call(QET_XA_END, qe_ccb);
		    }
		}
		else if ( qmode == PSQ_GCA_XA_PREPARE )
		{
		    MEcopy("Prepare XA transaction", 22, sscb->sscb_ics.ics_act1);
		    sscb->sscb_ics.ics_l_act1 = 22;

		    status = qef_call(QET_XA_PREPARE, qe_ccb);
		}
		else if ( qmode == PSQ_GCA_XA_COMMIT )
		{
		    /* xa_commit(TMONEPHASE) ? */
		    if ( GCAdata->gca_xa_flags & GCA_XA_FLG_1PC )
		    {
			qe_ccb->qef_modifier = QET_XA_COMMIT_1PC;

			MEcopy("Commit(1PC) XA transaction", 25, sscb->sscb_ics.ics_act1);
			sscb->sscb_ics.ics_l_act1 = 25;
		    }
		    else
		    {
			MEcopy("Commit XA transaction", 21, sscb->sscb_ics.ics_act1);
			sscb->sscb_ics.ics_l_act1 = 21;
		    }

		    status = qef_call(QET_XA_COMMIT, qe_ccb);
		}
		else /* Must be PSQ_GCA_XA_ROLLBACK */
		{
		    MEcopy("Rollback XA transaction", 23, sscb->sscb_ics.ics_act1);
		    sscb->sscb_ics.ics_l_act1 = 23;

		    status = qef_call(QET_XA_ROLLBACK, qe_ccb);
		}

		/* Trace the xa call, if tracepoint was set */
		if (Sc_main_cb->sc_trace_errno == E_SC0206_CANNOT_PROCESS)
		    scs_xatrace(scb, qmode, GCAdata->gca_xa_flags, (char *)0,
			&DisTranId, *err_code);

		/* Transform originating QEF error code into XA return code */
		if ( DB_FAILURE_MACRO(status) )
		{
		    /* Interrupted (or externally force-aborted) ? */
		    if ( sscb->sscb_interrupt || sscb->sscb_force_abort )
			*err_code = E_QE0022_QUERY_ABORTED;

		    if ( *QefErrcode == 0 )
			*QefErrcode = *err_code;

		    /* On the QEF error that resulted in *err_code: */
		    switch ( *QefErrcode )
		    {
			case E_QE002A_DEADLOCK:
			    *XAerr_code = XA_RBDEADLOCK;
			    break;
			case E_QE0036_LOCK_TIMER_EXPIRED:
			    *XAerr_code = XA_RBTIMEOUT;
			    break;
			case E_QE0004_NO_TRANSACTION:
			case E_QE006F_TRAN_NOT_IN_PROGRESS:
			case E_QE0054_DIS_TRAN_UNKNOWN:
			    *err_code = E_QE0025_USER_ERROR;
			    *XAerr_code = XAER_NOTA;
			    break;
			case E_QE0053_TRAN_ID_NOT_UNIQUE:
			    *XAerr_code = XAER_DUPID;
			    break;
			case E_QE0006_TRANSACTION_EXISTS:
			    *XAerr_code = XAER_OUTSIDE;
			    break;
			case E_QE006E_TRAN_IN_PROGRESS:
			    *err_code = E_QE0025_USER_ERROR;
			    *XAerr_code = XAER_PROTO;
			    break;
			case E_QE005C_INVALID_TRAN_STATE:
			    *err_code = E_QE0025_USER_ERROR;
			    *XAerr_code = XAER_PROTO;
			    break;
			case E_QE0022_QUERY_ABORTED:
			    *XAerr_code = XA_RBOTHER;
			    break;

			default:
			    *XAerr_code = XAER_RMERR;
		    }

		    /* The API expects GCA_REFUSE on any XA error */
		    sscb->sscb_rsptype = GCA_REFUSE;
		    /* Notify the API of the XAerr_code */
		    qry_status = (GCA_FAIL_MASK | GCA_XA_ERROR_MASK);
		    scs_qef_error(status, *err_code, E_SC0210_SCS_SQNCR_ERROR, scb);
		    scd_note(status, DB_QEF_ID);
		}
		else if ( qmode == PSQ_GCA_XA_START )
		{
		    /* Mark SCB as using XA transaction management */
		    sscb->sscb_flags |= SCS_XA_START;
		    STRUCT_ASSIGN_MACRO(DisTranId, *SessDisTranId);
		}
		else if ( qmode == PSQ_GCA_XA_END )
		{
		    /* Unmark SCB as using XA transaction management */
		    sscb->sscb_flags &= ~SCS_XA_START;

		    /* Invalidate Session's distributed tranid */
		    SessDisTranId->db_dis_tran_id_type = 0;

		    /* Close down all cursor stuff */
		    ps_ccb = sscb->sscb_psccb;
		    sscb->sscb_cpy_qeccb = NULL;
		    status = psq_call(PSQ_COMMIT, ps_ccb, sscb->sscb_psscb);
		    if (DB_FAILURE_MACRO(status))
		    /* On the QEF error that resulted in *err_code: */
		    {
			if (ps_ccb->psq_error.err_code == E_PS0002_INTERNAL_ERROR)
			{
			    sc0e_0_put(E_SC0215_PSF_ERROR, 0);
			    sc0e_0_put(ps_ccb->psq_error.err_code, 0);
			    scd_note(status, DB_PSF_ID);
			    ret_val = E_DB_ERROR;
			}
		    }

		    if (sscb->sscb_ciu)
		    {
			/*
			**  When ending a xact, make sure that all
			**  psf cursors are closed
			*/

			scs_cursor_remove(scb, 0);
		    }

		    if (sscb->sscb_cquery.cur_rdesc.rd_comp_tdesc)
		    {
			scs_save_tdesc(scb, (PTR)NULL);
		    }

		    if (qe_ccb->qef_illegal_xact_stmt)
			qry_status |= GCA_ILLEGAL_XACT_STMT;
		}
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;
	    }

	    case PSQ_UPD_ROWCNT:

	/* Transaction control operations */
	    case PSQ_ENDTRANS:
	    case PSQ_COMMIT:
	    case PSQ_SECURE:
	    case PSQ_ABORT:
	    case PSQ_ROLLBACK:
	    case PSQ_ABSAVE:
	    case PSQ_RLSAVE:

	    case PSQ_BGNTRANS:
	    case PSQ_SVEPOINT:
	    case PSQ_AUTOCOMMIT:

	    case PSQ_DDLCONCUR:
		if (sscb->sscb_state == SCS_CONTINUE)
		{
		    /*
		    ** In these cases, PSF will have produced the appropriate
		    ** control block in qsf.  Simply call QEF with the right
		    ** kind of operation.
		    */

		    /*
		    ** An exclusive lock is gotten since we will need one to
		    ** the object at the end anyway.
		    */

		    qs_ccb = sscb->sscb_qsccb;
		    qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
		    qs_ccb->qsf_lk_state = QSO_EXLOCK;
		    status = qsf_call(QSO_LOCK, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			    qry_status = GCA_FAIL_MASK;
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC0210_SCS_SQNCR_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    sscb->sscb_state = SCS_INPUT;
			    *next_op = CS_EXCHANGE;
			    break;
		    }
		    qsf_object = 1;
		    qe_ccb = (QEF_RCB *) qs_ccb->qsf_root;
		    cquery->cur_qe_cb = (PTR) qe_ccb;
		}
		else
		{
		    qe_ccb = sscb->sscb_qeccb;
		    qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;
		    qe_ccb->qef_flag = 0;
		    qe_ccb->qef_spoint= NULL;
		}
	        if (ult_always_trace())
	        {
                    void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
                    if (f)
                    {
	   		i2 x;
	   		switch (qmode)
	   		{
           		case PSQ_ENDTRANS:	x=SC930_LTYPE_ENDTRANS;break;
           		case PSQ_COMMIT:	x=SC930_LTYPE_COMMIT;break;
           		case PSQ_SECURE:	x=SC930_LTYPE_SECURE;break;
           		case PSQ_ABORT:		x=SC930_LTYPE_ABORT;break;
           		case PSQ_ROLLBACK:	x=SC930_LTYPE_ROLLBACK;break;
           		case PSQ_ABSAVE:	x=SC930_LTYPE_ABSAVE;break;
           		case PSQ_RLSAVE:	x=SC930_LTYPE_RLSAVE;break;

           		case PSQ_BGNTRANS:	x=SC930_LTYPE_BGNTRANS;break;
           		case PSQ_SVEPOINT:	x=SC930_LTYPE_SVEPOINT;break;
           		case PSQ_AUTOCOMMIT:	x=SC930_LTYPE_AUTOCOMMIT;break;

           		case PSQ_DDLCONCUR:	x=SC930_LTYPE_DDLCONCUR;break;
	   		default:		x=SC930_LTYPE_UNKNOWN;break;
	   		}
                        ult_print_tracefile(f,x,"");
                        ult_close_tracefile(f);
                    }
                }
		qe_ccb->qef_modifier = QEF_MSTRAN;
		qe_ccb->qef_eflag = QEF_EXTERNAL;
		qe_ccb->qef_cb = sscb->sscb_qescb;
		qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
		qe_ccb->qef_no_xact_stmt =
		    (sscb->sscb_req_flags & SCS_NO_XACT_STMT) != 0;
		qe_ccb->qef_illegal_xact_stmt = FALSE;
		if (qmode == PSQ_BGNTRANS)
		{
		    status = qef_call(QET_BEGIN, qe_ccb);
		}
		else if (qmode == PSQ_ENDTRANS)
		{
		    status = qef_call(QET_COMMIT, qe_ccb);
		}
		else if (qmode == PSQ_SVEPOINT)
		{
		    status = qef_call(QET_SAVEPOINT, qe_ccb);
		}
		else if ((qmode == PSQ_ABORT) || (qmode == PSQ_ABSAVE))
		{
		    MEfill(SCS_ACT_SIZE, '\0', sscb->sscb_ics.ics_act1);
                    MEcopy("Aborting", 8, sscb->sscb_ics.ics_act1);
                    sscb->sscb_ics.ics_l_act1 = 8;
                    status = qef_call(QET_ABORT, qe_ccb);
                    sscb->sscb_ics.ics_l_act1 = 0;
		}
		else if (qmode == PSQ_COMMIT)
		{
		    status = qef_call(QET_SCOMMIT, qe_ccb);
		}
		else if (qmode == PSQ_SECURE)
		{
		    STRUCT_ASSIGN_MACRO(sscb->sscb_dis_tran_id, qe_ccb->qef_cb->qef_dis_tran_id);
		    status = qef_call(QET_SECURE, qe_ccb);
		}
		else if ((qmode == PSQ_ROLLBACK) || (qmode == PSQ_RLSAVE))
		{
		    MEfill(SCS_ACT_SIZE, '\0', sscb->sscb_ics.ics_act1);
                    MEcopy("Aborting", 8, sscb->sscb_ics.ics_act1);
                    sscb->sscb_ics.ics_l_act1 = 8;
                    status = qef_call(QET_ROLLBACK, qe_ccb);
                    sscb->sscb_ics.ics_l_act1 = 0;
		}
		else if (qmode == PSQ_AUTOCOMMIT)
		{
		    status = qef_call(QET_AUTOCOMMIT, qe_ccb);
		}
		else if (qmode == PSQ_DDLCONCUR)
		{
		    status = qef_call(QED_CONCURRENCY, qe_ccb);
		}
		else if ((qmode == PSQ_UPD_ROWCNT) &&
			 (!(sscb->sscb_flags & SCS_STAR)))
                {
                    status= qef_call(QEQ_UPD_ROWCNT, qe_ccb);
                }
		if (DB_FAILURE_MACRO(status))
		{
		    if (sscb->sscb_interrupt)
			break;
		    qry_status = GCA_FAIL_MASK;
		    scs_qef_error(status, qe_ccb->error.err_code, E_SC0210_SCS_SQNCR_ERROR, scb);
		    scd_note(status, DB_QEF_ID);
		}
		else if ((qmode == PSQ_RLSAVE)
		      || (qmode == PSQ_ABORT)
		      || (qmode == PSQ_ABSAVE)
		      || (qmode == PSQ_ENDTRANS)
		      || (qmode == PSQ_COMMIT)
		      || (qmode == PSQ_SECURE)
		      || (qmode == PSQ_ROLLBACK)
		        )
		{
		    ps_ccb = sscb->sscb_psccb;
		    sscb->sscb_cpy_qeccb = NULL;
		    status = psq_call(PSQ_COMMIT, ps_ccb, sscb->sscb_psscb);
		    if (DB_FAILURE_MACRO(status))
		    {
			if (ps_ccb->psq_error.err_code == E_PS0002_INTERNAL_ERROR)
			{
			    sc0e_0_put(E_SC0215_PSF_ERROR, 0);
			    sc0e_0_put(ps_ccb->psq_error.err_code, 0);
			    scd_note(status, DB_PSF_ID);
			    ret_val = E_DB_ERROR;
			}
		    }

		    if (sscb->sscb_ciu)
		    {
			/*
			**  When ending a xact, make sure that all psf cursors
			**  are closed
			*/

			scs_cursor_remove(scb, 0);
		    }
		    if (cquery->cur_rdesc.rd_comp_tdesc)
		    {
			scs_save_tdesc(scb, (PTR)NULL);
		    }

		    /* Transaction control statement unexpected in XA transactions */
		    if ( sscb->sscb_dis_tran_id.db_dis_tran_id_type ==
				DB_XA_DIS_TRAN_ID_TYPE )
		    {
			if (Sc_main_cb->sc_trace_errno == E_SC0206_CANNOT_PROCESS)
			    scs_xatrace(scb, qmode, 0, "XNCTRL",
			    (DB_DIS_TRAN_ID *)NULL, qry_status);

			/* Invalidate XA dist tran id as it's gone now */
			sscb->sscb_dis_tran_id.db_dis_tran_id_type = 0;
		    }
		}
		else
		{
		    sscb->sscb_ciu = 0;
		}
		if (qe_ccb->qef_illegal_xact_stmt)
		    qry_status |= GCA_ILLEGAL_XACT_STMT;

		STRUCT_ASSIGN_MACRO(qe_ccb->qef_comstamp, sscb->sscb_comstamp);

		qe_ccb = 0;	/* since it isn't the real one */
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;


	/* QEF/DMF DBU operations are all executed in a similar manner */

	    case PSQ_ATBL_DROP_COLUMN:
	    case PSQ_INDEX:
	    case PSQ_MODIFY:
	    case PSQ_RELOCATE:
	    case PSQ_SAVE:
	    case PSQ_DISTCREATE:
	    case PSQ_DGTT:
	    case PSQ_SJOURNAL:
	    case PSQ_SSTATS:

            /* schang : these are from GW */
            case PSQ_REG_INDEX:
            case PSQ_REG_IMPORT:
	    /* case PSQ_DESTROY: in case you're looking, destroy is with DSTVIEW */
	    {
		QEU_CB		qeu;

		/* First, we start an xact to manage the work */
		qeu.qeu_eflag = QEF_EXTERNAL;
		qeu.qeu_db_id = (PTR) sscb->sscb_ics.ics_opendb_id;
		qeu.qeu_d_id = scb->cs_scb.cs_self;
		qeu.qeu_flag = 0;
                qeu.qeu_mask = 0;
		qeu.qeu_qmode = qmode;
		status = qef_call(QEU_BTRAN, &qeu);
		if (ret_val = status)
		{
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qry_status = GCA_FAIL_MASK;
		    break;
		}

		/*
		** An exclusive lock is gotten since we will need one to
		** destroy the object at the end anyway.
		*/

		qs_ccb = sscb->sscb_qsccb;
		qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
		qs_ccb->qsf_lk_state = QSO_EXLOCK;
		status = qsf_call(QSO_LOCK, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC0210_SCS_SQNCR_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    qry_status = GCA_FAIL_MASK;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    (VOID) qef_call(QEU_ATRAN, &qeu);
		    break;
		}
		qsf_object = 1;
		/*
		** the call to get the lock returns the root id;
		** therefore, there is no need to do an info
		*/

		cquery->cur_qe_cb = (PTR) (QEU_CB *) qs_ccb->qsf_root;
		((QEU_CB *) qs_ccb->qsf_root)->qeu_eflag = QEF_EXTERNAL;
		((QEU_CB *) qs_ccb->qsf_root)->qeu_d_id = scb->cs_scb.cs_self;

                ((QEU_CB *) qs_ccb->qsf_root)->qeu_mask = 0;
		sscb->sscb_cfac = DB_QEF_ID;
		status = qef_call(QEU_DBU,  qs_ccb->qsf_root);

		/*
		** logic to permit execute immediate of CREATE INDEX
		** statements
		*/

		if  (qmode == PSQ_INDEX || (qmode == PSQ_MODIFY &&
				qef_wants_eiqp))
		{
		    i4 cont = 0;
		    /*
		    ** We may need to call back QEF to continue E.I.
                    ** processing.  PSQ_INDEX follows the DBU path
		    ** and does not go thro' qeq_query().
                    ** So we need to call qeq_query() explicitly.
                    ** FIXME: When PSQ_INDEX is processed like
                    ** DML statements, this call back will not be needed.
		    **
		    ** Auto structure feature results in MODIFY doing
		    ** similar, and this code is needed to clean up.
                    */
                    scs_chkretry(scb, status, &ret_val, &cont,
                                &next_op, &qef_wants_eiqp);
        	    if (cont == 1)
		    {

		       /* Fix for bug 57815 */

                       if (qsf_object)
                       {
                          /*
                          ** Now free the qe_ccb control block which is in
			  ** qsf.
                          */

                          /* qs_ccb is set up above */
                          status = qsf_call(QSO_DESTROY, qs_ccb);

                          if (DB_FAILURE_MACRO(status))
                          {
                             scs_qsf_error(status,qs_ccb->qsf_error.err_code,
                                E_SC0210_SCS_SQNCR_ERROR);
                          }
                          qsf_object = 0;
                       }

		       continue;
		    }
		}	/* end if this is a CREATE INDEX statement */

		sscb->sscb_cfac = DB_SCF_ID;
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    if (sscb->sscb_interrupt)
			break;
		    scd_note(status, DB_QEF_ID);
		    if (((QEU_CB *) qs_ccb->qsf_root)->error.err_code
						== E_QE0023_INVALID_QUERY)
		    {
			/*
			** Now free the qe_ccb control block which is in qsf.
			*/
			status = qsf_call(QSO_DESTROY, qs_ccb);    /* qs_ccb is set up above */
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				    E_SC0210_SCS_SQNCR_ERROR);
			}
			qsf_object = 0;
			continue;
		    }
		    scs_qef_error(status,
			    ((QEU_CB *) qs_ccb->qsf_root)->error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR,
			    scb);
		    (VOID) qef_call(QEU_ATRAN, &qeu);
		}
		else
		{
		    status = qef_call(QEU_ETRAN, &qeu);
		    if (ret_val = status)
		    {
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sc0e_0_put(qeu.error.err_code, 0);
			qry_status = GCA_FAIL_MASK;
			break;
		    }
		}
		cquery->cur_row_count = ((QEU_CB *) qs_ccb->qsf_root)->qeu_count;
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;
	    }

	    case PSQ_COPY:
		/* Let the copy handlers figure out what to do next.
		** The initial state after parsing the copy statement
		** will be SCS_CONTINUE.  Copy-begin will change the
		** state to CP_FROM or CP_INTO as appropriate.  Those
		** two states are handled early, not here.  The other
		** possibility is "SCS_CP_ERROR", which we handle here
		** since we're trying to end the copy.
		*/
		if (sscb->sscb_state == SCS_CONTINUE)
		    seq_action = scs_copy_begin(scb, next_op, &ret_val);
		else
		    seq_action = scs_copy_bad(scb, next_op, &ret_val);

		if (seq_action == SEQUENCER_CONTINUE)
		    continue;
		else if (seq_action == SEQUENCER_RETURN)
		    return (ret_val);
		/* else break.  Interpret the return status.  Anything
		** not-OK sets the query status.  Anything less than ERROR
		** resets ret_val to OK, although ret_val and qry_status
		** are sorta kinda the same thing but not really.
		*/
		if (ret_val != E_DB_OK)
		    qry_status = GCA_FAIL_MASK;
		if (DB_SUCCESS_MACRO(ret_val))
		    ret_val = E_DB_OK;
		break;

	    case PSQ_INTEG:
	    case PSQ_PROT:
	    case PSQ_GRANT:
	    case PSQ_DSTINTEG:
	    case PSQ_DESTROY:   /* destroys views or tables */
	    case PSQ_REG_REMOVE: /* schang : is here the place for remove table/index ? */
	    case PSQ_DSTPERM:
	    case PSQ_REVOKE:	/*
				** revoke privilege(s) on table(s), dbproc(s),
				** or dbevent(s)
				*/
	    case PSQ_DRODBP:
	    case PSQ_CGROUP:
	    case PSQ_AGROUP:
	    case PSQ_DGROUP:
	    case PSQ_KGROUP:
	    case PSQ_CAPLID:
	    case PSQ_AAPLID:
	    case PSQ_KAPLID:
	    case PSQ_RULE:	/* CREATE RULE */
	    case PSQ_DSTRULE:	/* DROP RULE */
	    case PSQ_GDBPRIV:
	    case PSQ_RDBPRIV:
	    case PSQ_MXQUERY:
	    case PSQ_EVENT:	/* CREATE EVENT */
	    case PSQ_EVDROP:	/* DROP EVENT */
	    case PSQ_CPROFILE:
	    case PSQ_DPROFILE:
	    case PSQ_APROFILE:
	    case PSQ_CUSER:
	    case PSQ_AUSER:
	    case PSQ_KUSER:
	    case PSQ_CLOC:
	    case PSQ_ALOC:
	    case PSQ_KLOC:
	    case PSQ_ENAUDIT:
	    case PSQ_DISAUDIT:
	    case PSQ_CALARM:
	    case PSQ_KALARM:
	    case PSQ_COMMENT:
	    case PSQ_GOKPRIV:
	    case PSQ_ROKPRIV:
	    case PSQ_CSYNONYM:
	    case PSQ_DSYNONYM:
	    case PSQ_REMOVE:
	    case PSQ_REREGISTER:
	    case PSQ_DROP_SCHEMA:
	    case PSQ_ALTAUDIT:
	    case PSQ_MXSESSION:
	    case PSQ_SETROLE:
	    case PSQ_RGRANT:
	    case PSQ_RREVOKE:
	    case PSQ_ALTERDB:
	    case PSQ_CSEQUENCE:
	    case PSQ_ASEQUENCE:
	    case PSQ_DSEQUENCE:
	    {
		PSY_CB	    *psy_cb;
		i4	    internal_error;

		internal_error = TRUE;
		qs_ccb = sscb->sscb_qsccb;
		qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
		qs_ccb->qsf_lk_state = QSO_EXLOCK;
		status = qsf_call(QSO_LOCK, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    ret_val = E_DB_SEVERE;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}
		else
		{
		    qsf_object = 1;
		    /*
		    ** the call to get the lock returns the root id;
		    ** therefore, there is no need to do an info
		    */

		    psy_cb = (PSY_CB *) qs_ccb->qsf_root;


		    sscb->sscb_cfac = DB_PSF_ID;
		    if (sqncr_tbl[qmode].sq_psy_opcode == PSY_AUDIT)
		    {
			/* audit statements must act atomically, and
			** may not appear within MST's...
			*/
			qe_ccb = sscb->sscb_qeccb;
			status = qef_call(QEC_INFO, qe_ccb);

			if (DB_FAILURE_MACRO(status))
			{
			    scs_qef_error(status, qe_ccb->error.err_code,
				    E_SC0210_SCS_SQNCR_ERROR, scb);
			}
			else if (qe_ccb->qef_stat != QEF_NOTRAN)
			{
			    sc0e_0_uput(E_SC0030_CHANGESECAUDIT_IN_XACT, 0);
			    qry_status = GCA_FAIL_MASK;
			    sscb->sscb_state = SCS_INPUT;
			    *next_op = CS_EXCHANGE;
			}
			else
			{
			    status = psy_call(PSY_AUDIT, psy_cb,
					    sscb->sscb_psscb);
			}
		    }
		    else if (sqncr_tbl[qmode].sq_mask & SQ_PSY_MASK)
		    {
			/* Anything not needing special handling */
			status = psy_call(sqncr_tbl[qmode].sq_psy_opcode,
					psy_cb, sscb->sscb_psscb);
		    }
		    else if (qmode == PSQ_MXSESSION)
		    {
			/*
			** Although mxsession is handled logically as
			** part of the KME limits in PSY, they are actually
			** done is SCF, so we process directly here
			*/
			status = scs_mxsession(scb, psy_cb);
		    }
		    else if (qmode == PSQ_SETROLE)
		    {
			/*
			** The current role is a session attribute
			*/
			/* SET ROLE may not be issued within an MST
			*/
		    	qe_ccb = sscb->sscb_qeccb;
	                status = qef_call(QEC_INFO, qe_ccb);

	                if (DB_FAILURE_MACRO(status))
	                {
			    scs_qef_error(status, qe_ccb->error.err_code,
			                E_SC0210_SCS_SQNCR_ERROR, scb);
	                }
		        else if (qe_ccb->qef_stat != QEF_NOTRAN)
		        {
			    sc0e_0_uput(E_SC035D_SET_ROLE_IN_XACT, 0);
			    status=E_DB_ERROR;
			    internal_error=FALSE;
			}
		        else if (Sc_main_cb->sc_secflags&SC_SEC_ROLE_NONE)
			{
			    /*
			    ** No roles, so SET ROLE is disallowed
			    */
			    sc0e_put(E_US18E7_6375, 0, 1,
				 sizeof(ERx("SET ROLE"))-1,
				 ERx("SET ROLE"),
				     0, (PTR)0,
				     0, (PTR)0,
				     0, (PTR)0,
				     0, (PTR)0,
				     0, (PTR)0);
			    status=E_DB_ERROR;
			    internal_error=FALSE;
			}
			else
			{
			    /*
			    ** Everything is OK, so lets change role
			    */
			    status = scs_setrole(scb,ps_ccb);
			    internal_error=FALSE;
			}
		    }
                    /* We need to call back QEF to continue E.I. processing
                    ** in case of PSQ_RULE/PSQ_GRANT. PSQ_RULE follows the
                    ** qrymod path and does not go thro' qeq_query().
                    ** So we need to call qeq_query() explicitly.
                    ** FIXME: When PSQ_RULE/PSQ_GRANT is processed like
                    ** DML statement, this call back will not be needed.
                    */
                    if (qmode == PSQ_GRANT ||
                        qmode == PSQ_RULE)
                    {
			i4 cont = 0;

                        scs_chkretry(scb, status, &ret_val, &cont,
                                &next_op, &qef_wants_eiqp);
                        if (cont == 1)
                        {

                           /* Fix for bug 57815 */

                           if (qsf_object)
                           {
                              /*
                              ** Now free the qe_ccb control block
                              ** which is in qsf.
                              */

                              /* qs_ccb is set up above */
                              status = qsf_call(QSO_DESTROY, qs_ccb);

                              if (DB_FAILURE_MACRO(status))
                              {
                                 scs_qsf_error(status,
                                        qs_ccb->qsf_error.err_code,
                                        E_SC0210_SCS_SQNCR_ERROR);
                              }
                              qsf_object = 0;
                           }
                           continue;
                        }
                    }

		    sscb->sscb_cfac = DB_SCF_ID;
		    if (DB_FAILURE_MACRO(status))
		    {
			if (psy_cb->psy_error.err_code == E_PS0008_RETRY)
			{
				/* QSF_RCB to destroy psy objects */
			    QSF_RCB	    qsr_psy;

			    STRUCT_ASSIGN_MACRO(*qs_ccb, qsr_psy);
			    qsr_psy.qsf_lk_state = QSO_SHLOCK;

			    /* Now destroy the psy_qrytext object */
			    if (psy_cb->psy_qrytext.qso_handle != 0)
			    {
				qsr_psy.qsf_obj_id.qso_handle =
						psy_cb->psy_qrytext.qso_handle;
				status = qsf_call(QSO_LOCK, &qsr_psy);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
					    qsr_psy.qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
				}
				else
				{
				    status = qsf_call(QSO_DESTROY, &qsr_psy);
				    if (DB_FAILURE_MACRO(status))
				    {
					scs_qsf_error(status,
						qsr_psy.qsf_error.err_code,
						    E_SC0210_SCS_SQNCR_ERROR);
				    }
				}
			    }

			    /* Now do the same for psy_intree */
			    if (psy_cb->psy_intree.qso_handle != 0)
			    {
				qsr_psy.qsf_obj_id.qso_handle =
						psy_cb->psy_intree.qso_handle;
				status = qsf_call(QSO_LOCK, &qsr_psy);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
					    qsr_psy.qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
				}
				else
				{
				    status = qsf_call(QSO_DESTROY, &qsr_psy);
				    if (DB_FAILURE_MACRO(status))
				    {
					scs_qsf_error(status,
						qsr_psy.qsf_error.err_code,
						    E_SC0210_SCS_SQNCR_ERROR);
				    }
				}
			    }

			    /* Now do the same for psy_outtree */
			    if (psy_cb->psy_outtree.qso_handle != 0)
			    {
				qsr_psy.qsf_obj_id.qso_handle =
						psy_cb->psy_outtree.qso_handle;
				status = qsf_call(QSO_LOCK, &qsr_psy);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
					    qsr_psy.qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
				}
				else
				{
				    status = qsf_call(QSO_DESTROY, &qsr_psy);
				    if (DB_FAILURE_MACRO(status))
				    {
					scs_qsf_error(status,
						qsr_psy.qsf_error.err_code,
						    E_SC0210_SCS_SQNCR_ERROR);
				    }
				}
			    }

			    /* Now do the same for psy_textout */
			    if (psy_cb->psy_textout.qso_handle != 0)
			    {
				qsr_psy.qsf_obj_id.qso_handle =
						psy_cb->psy_textout.qso_handle;
				status = qsf_call(QSO_LOCK, &qsr_psy);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
					    qsr_psy.qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
				}
				else
				{
				    status = qsf_call(QSO_DESTROY, &qsr_psy);
				    if (DB_FAILURE_MACRO(status))
				    {
					scs_qsf_error(status,
						qsr_psy.qsf_error.err_code,
						    E_SC0210_SCS_SQNCR_ERROR);
				    }
				}
			    }

			    /* Now destroy the object in psq_txtout */
			    if (ps_ccb->psq_txtout.qso_handle != 0)
			    {
				qsr_psy.qsf_obj_id.qso_handle =
						ps_ccb->psq_txtout.qso_handle;
				status = qsf_call(QSO_LOCK, &qsr_psy);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
					    qsr_psy.qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
				}
				else
				{
				    status = qsf_call(QSO_DESTROY, &qsr_psy);
				    if (DB_FAILURE_MACRO(status))
				    {
					scs_qsf_error(status,
						qsr_psy.qsf_error.err_code,
						    E_SC0210_SCS_SQNCR_ERROR);
				    }
				}
			    }

			    /*
			    ** Now free the psy_cb control block that is pointed
			    ** to by psq_result which is in qsf
			    */
			    status = qsf_call(QSO_DESTROY, qs_ccb);
			    if (DB_FAILURE_MACRO(status))
			    {
				scs_qsf_error(status, qs_ccb->qsf_error.err_code,
						    E_SC0210_SCS_SQNCR_ERROR);
			    }

			    qsf_object = 0;
			    continue;
			}
			qry_status = GCA_FAIL_MASK;
			if ( (psy_cb->psy_error.err_code
						    != E_PS0001_USER_ERROR
			      ) &&
			     (psy_cb->psy_error.err_code
						    != E_PS0003_INTERRUPTED
			      ) &&  internal_error
			    )
			{
			    sc0e_0_put(E_SC0215_PSF_ERROR, 0);
			    sc0e_0_put(psy_cb->psy_error.err_code, 0);
			    sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    scd_note(status, DB_PSF_ID);
			    ret_val = E_DB_OK;
			}
		    }
		}
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;
	    }

	    case PSQ_FREELOCATOR:
	    {
		ps_ccb = sscb->sscb_psccb;
		for (i = 0; i < 10; i++)
		{
		    if (ps_ccb->psq_locator[i].db_datatype != DB_NODT)
		    {
			adu_free_locator((PTR)scb->cs_scb.cs_self,
				&ps_ccb->psq_locator[i]);
		    }
		}
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;
	    }

	    case PSQ_SET_SESSION:
		status=scs_set_session(scb, ps_ccb);
		if(status!=E_DB_OK)
		{
		    qry_status = GCA_FAIL_MASK;
		}
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
	        break;


	    case PSQ_SDECIMAL:
		STRUCT_ASSIGN_MACRO(ps_ccb->psq_decimal, sscb->sscb_adscb->adf_decimal);
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    case PSQ_SDATEFMT:
		sscb->sscb_adscb->adf_dfmt = ps_ccb->psq_dtefmt;
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    case PSQ_SMNFMT:
		sscb->sscb_adscb->adf_mfmt = ps_ccb->psq_mnyfmt;
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    case PSQ_SMNPREC:
		/* change ingres control information everywhere (or just adf) */
		/* {@fix_me@} */
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    case PSQ_SETUNICODESUB:
		sscb->sscb_adscb->adf_unisub_status = ps_ccb->psq_usub_stat;

		if (ps_ccb->psq_usub_stat == AD_UNISUB_ON)
		  CMcpychar((uchar *) ps_ccb->psq_usub_char,
			(uchar *) sscb->sscb_adscb->adf_unisub_char);
		else
		  *(sscb->sscb_adscb->adf_unisub_char) = '\0';

		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    case PSQ_SETRANDOMSEED:
		FPsrand(ps_ccb->psq_random_seed);
		MHsrand2(ps_ccb->psq_random_seed);
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
	        break;


	    case PSQ_SSQL:
		/* alter query language everywhere */
		/* For now, only done in parser, and he does that himself */
		sscb->sscb_adscb->adf_qlang = DB_SQL;
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    case PSQ_SLOCKMODE:
            case PSQ_SLOGGING:
	    case PSQ_DEFLOC:
	    case PSQ_SWORKLOC:
	    case PSQ_STRANSACTION:
	      {
		DM_OPERATION dmf_operation;

		if ((qmode == PSQ_SLOGGING) &&
		    (sscb->sscb_ics.ics_db_access_mode == DMC_A_READ))
		{
		    sc0e_0_uput(E_SC0381_SETLG_READONLY_DB, 0L);
		    qry_status = GCA_FAIL_MASK;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}

		if ((sscb->sscb_flags & SCS_STAR) &&
		    (qmode == PSQ_SLOCKMODE))
		{
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}

		/*
		** An exclusive lock is gotten since we will need one to
		** destroy the object at the end anyway.
		*/

		qs_ccb = sscb->sscb_qsccb;
		qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
		qs_ccb->qsf_lk_state = QSO_EXLOCK;
		status = qsf_call(QSO_LOCK, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC0210_SCS_SQNCR_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}
		qsf_object = 1;

		/*
		** the call to get the lock returns the root id;
		** therefore, there is no need to do an info
		*/

                /*
                ** Check if there is a transaction outstanding.  Set Transaction
		** Isolation Level, and Set NoLogging cannot be
                ** while in a transaction.
		**
		** Let Set Lockmode go through. DMF allows some options
		** while in a transactions, but disallows others.
                */
                if ((qmode == PSQ_SLOGGING) || (qmode == PSQ_STRANSACTION))
		{
		    qe_ccb = sscb->sscb_qeccb;
		    status = qef_call(QEC_INFO, qe_ccb);

		    if (DB_FAILURE_MACRO(status))
		    {
			qry_status = GCA_FAIL_MASK;
			if (sscb->sscb_interrupt)
			    break;
			scs_qef_error(status,
				    qe_ccb->error.err_code,
				    E_SC0210_SCS_SQNCR_ERROR,
				    scb);
                        sscb->sscb_state = SCS_INPUT;
                        *next_op = CS_EXCHANGE;
                        break;
		    }
		    if (qe_ccb->qef_stat != QEF_NOTRAN)
		    {
			sc0e_0_uput((qmode == PSQ_STRANSACTION)
				? 5932L : E_SC031B_SETLG_XCT_INPROG,
			    0L);
			qry_status = GCA_FAIL_MASK;
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			break;
		    }
		}

                if ((qmode == PSQ_SLOCKMODE) || (qmode == PSQ_DEFLOC) ||
                    (qmode == PSQ_STRANSACTION) || (qmode == PSQ_SWORKLOC) ||
                    (qmode == PSQ_SLOGGING) )
		{
                    dmf_operation = DMC_ALTER;
		}
                else
		{
                    dmf_operation = DMT_ALTER;
		}

                status = dmf_call(dmf_operation, (DMC_CB *) qs_ccb->qsf_root);
		if (DB_FAILURE_MACRO(status))
		{
		    dm_ccb = (DMC_CB *) qs_ccb->qsf_root;
		    if (dm_ccb->error.err_code == E_DM0065_USER_INTR)
		    {
			/* no problem -- ignore */
		    }
		    else if (dm_ccb->error.err_code == E_DM0064_USER_ABORT)
		    {
			sscb->sscb_force_abort = SCS_FAPENDING;
		    }
		    else if (dm_ccb->error.err_code
				    == E_DM010E_SET_LOCKMODE_ROW)
		    {
			sc0e_0_uput(E_SC0380_SET_LOCKMODE_ROW, 0);
			qry_status = GCA_FAIL_MASK;
		    }
		    else if (dm_ccb->error.err_code
				    == E_DM001D_BAD_LOCATION_NAME)
		    {
			sc0e_uput(E_SC0316_LOC_NOT_FOUND, 0, 1,
				  dm_ccb->dmc_errlen_loc,
				  (PTR)dm_ccb->dmc_error_loc,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			qry_status = GCA_FAIL_MASK;
		    }
		    else if (dm_ccb->error.err_code
				    == E_DM013B_LOC_NOT_AUTHORIZED)
		    {
			sc0e_uput(E_SC0317_LOC_NOT_AUTHORIZED, 0, 1,
				  dm_ccb->dmc_errlen_loc,
				  (PTR)dm_ccb->dmc_error_loc,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			qry_status = GCA_FAIL_MASK;
		    }
		    else if (dm_ccb->error.err_code
				    == E_DM003E_DB_ACCESS_CONFLICT)
		    {
			sc0e_0_uput(E_SC0300_EXDB_LOCK_REQ, 0);
			qry_status = GCA_FAIL_MASK;
		    }
		    else if (dm_ccb->error.err_code
				    == E_DM007E_LOCATION_EXISTS)
		    {
			sc0e_0_uput(E_SC0301_LOC_EXISTS, 0);
			qry_status = GCA_FAIL_MASK;
		    }
                    else if (dm_ccb->error.err_code
                                    == E_DM0147_NOLG_BACKUP_ERROR)
                    {
                        sc0e_0_uput(E_DM0147_NOLG_BACKUP_ERROR, 0);
                        qry_status = GCA_FAIL_MASK;
                    }
                    else if (dm_ccb->error.err_code
                                    == E_DM0201_NOLG_MUSTLOG_ERROR)
                    {
                        sc0e_0_uput(E_DM0201_NOLG_MUSTLOG_ERROR, 0);
                        qry_status = GCA_FAIL_MASK;
                    }
                    else if (dm_ccb->error.err_code
                                    == E_DM0060_TRAN_IN_PROGRESS)
                    {
			/* Should only occur for SET LOCKMODE */
			sc0e_0_uput(5922L, 0L);
			qry_status = GCA_FAIL_MASK;
                    }
		    else
		    {
			qry_status = GCA_FAIL_MASK;
			sc0e_0_put(E_SC0215_PSF_ERROR, 0);
			sc0e_0_put(dm_ccb->error.err_code, 0);
			sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			scd_note(status, DB_PSF_ID);
		    }
		}
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;
	      }

	    case PSQ_SCPUFACT:
	    case PSQ_SQEP:
	    case PSQ_JTIMEOUT:
		if ((sscb->sscb_flags & SCS_STAR) &&
		    ((qmode == PSQ_SLOCKMODE) || (qmode == PSQ_SRINTO)))
		{
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}
		/* Alter the optimizer */
		qs_ccb = sscb->sscb_qsccb;
		qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
		qs_ccb->qsf_lk_state = QSO_EXLOCK;
		status = qsf_call(QSO_LOCK, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC0210_SCS_SQNCR_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    ret_val = E_DB_SEVERE;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}
		else
		{
		    qsf_object = 1;
		    /*
		    ** the call to get the lock returns the root id;
		    ** therefore, there is no need to do an info
		    */

		    ((OPF_CB *) qs_ccb->qsf_root)->opf_scb =
				    (PTR *) sscb->sscb_opscb;
		    ((OPF_CB *) qs_ccb->qsf_root)->opf_server =
				    Sc_main_cb->sc_opvcb;
		    ((OPF_CB *) qs_ccb->qsf_root)->opf_thandle = NULL;
		    status = opf_call(OPF_ALTER, (OPF_CB *) qs_ccb->qsf_root);
		    if (DB_FAILURE_MACRO(status))
		    {
			qry_status = GCA_FAIL_MASK;
		    }
		}
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    case PSQ_DMFTRACE:
	    case PSQ_PSFTRACE:
	    case PSQ_SCFTRACE:
	    case PSQ_RDFTRACE:
	    case PSQ_QEFTRACE:
	    case PSQ_OPFTRACE:
	    case PSQ_QSFTRACE:
	    case PSQ_ADFTRACE:
	    case PSQ_GWFTRACE:
	    case PSQ_SXFTRACE:
		{
		char    tp_info[GL_MAXNAME+1];
		i4 mesgid;
		/*
		** For all trace operations, fetch the root address of the
		** object returned by PSF from QSF.  This will be the address
		** of the DB_DEBUG_CB generated by the parser for this statement.
		** Then, call the appropriate routine to set the tracepoints.
		*/

		qs_ccb = sscb->sscb_qsccb;
		qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
		qs_ccb->qsf_lk_state = QSO_EXLOCK;
		status = qsf_call(QSO_LOCK, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    ret_val = E_DB_SEVERE;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}
		qsf_object = 1;
		/*
		** Ensure the user has TRACE permissions,
		** unless an application code is set or it's the annoying
		** "set [no]cache_dynamic".
		*/
		if (((DB_DEBUG_CB *) qs_ccb->qsf_root)->db_trswitch==DB_TR_ON)
		    mesgid=I_SX2750_SET_TRACE_POINT;
		else
		    mesgid=I_SX2751_SET_NOTRACE_POINT;

		if (!(sscb->sscb_ics.ics_rustat & DU_UTRACE) &&
		     (sscb->sscb_ics.ics_appl_code == 0))
		{
		    qry_status = GCA_FAIL_MASK;
		    sc0e_0_uput(E_US1712_TRACE_PERM, 0);
		    ret_val = E_DB_ERROR;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE
			&& DB_FAILURE_MACRO(sc0a_write_audit(
			    scb,
			    SXF_E_USER,	/* Type */
			    SXF_A_FAIL | SXF_A_SESSION,  /* Action */
			    "TRACE POINT",
			    sizeof("TRACE POINT")-1,
			    NULL,	/* Object Owner */
			    mesgid,	/* Mesg ID */
			    TRUE,			/* Force record */
			    0,
			    &dberror		/* Error location */
			)) )
		    {
			/*
			**	sc0a_write_audit should already have logged
			**  any lower level errors
			*/
			sc0e_0_put(E_SC023C_SXF_ERROR, 0);
		    }
		    break;
		}

		/*
		** Now that it worked, pass the root address
		** in to the appropriate facility
		*/

  		/* But first, show this trace action in II_DBMS_LOG */
  		TRdisplay("%@ [%d,%x] %~t >>>>> %t\n",
  			scb->cs_scb.cs_pid,
  			scb->cs_scb.cs_self,
  			sizeof(sscb->sscb_ics.ics_dbname.db_db_name),
  			sscb->sscb_ics.ics_dbname.db_db_name,
  			sscb->sscb_ics.ics_l_qbuf,
  			sscb->sscb_ics.ics_qbuf);

		switch (qmode)
		{
		    case PSQ_GWFTRACE:
			status = gwf_trace((DB_DEBUG_CB *) qs_ccb->qsf_root);
			STprintf(tp_info,"GW %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			break;

		    case PSQ_DMFTRACE:
			STprintf(tp_info,"DM %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			status = dmf_trace((DB_DEBUG_CB *) qs_ccb->qsf_root);
			break;

		    case PSQ_PSFTRACE:
			STprintf(tp_info,"PS %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			status = psf_debug((DB_DEBUG_CB *) qs_ccb->qsf_root);

			/* Nasty bit to make print_qry fully functional.    */
			/* If PSF's trace bit # 1 is being set, we `happen  */
			/* to know' that this is printqry.  So enable SCF's */
			/* printqry (SCS_TPRINT_QRY) flag as well.	    */

			if (	(((DB_DEBUG_CB *)
				    qs_ccb->qsf_root)->db_trace_point != 129)
			     || (((DB_DEBUG_CB *)
				    qs_ccb->qsf_root)->db_value_count != 0)
			   )
 			    break;
			((DB_DEBUG_CB *)
				qs_ccb->qsf_root)->db_trace_point = SCS_TPRINT_QRY;


		    case PSQ_SCFTRACE:
			STprintf(tp_info,"SC %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			status = scf_trace((DB_DEBUG_CB *) qs_ccb->qsf_root);
			break;

		    case PSQ_RDFTRACE:
			STprintf(tp_info,"RD %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			if (((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point
			    == 13)
			{
			    PSQ_CB              psq_cb;

			    psq_cb.psq_type = PSQCB_CB;
			    psq_cb.psq_length = sizeof(psq_cb);
			    psq_cb.psq_ascii_id = PSQCB_ASCII_ID;
			    psq_cb.psq_owner = (PTR) DB_SCF_ID;
			    psq_cb.psq_sessid = scb->cs_scb.cs_self;
			    status = psq_call(PSQ_ULMCONCHK, &psq_cb, NULL);
			}
			else
			{
			    status = rdf_trace((DB_DEBUG_CB *) qs_ccb->qsf_root);
			}
			break;

		    case PSQ_QEFTRACE:
		     {
			DB_DEBUG_CB	*dbg = (DB_DEBUG_CB *)qs_ccb->qsf_root;

			STprintf(tp_info,"QE %d", dbg->db_trace_point);
			status = qec_trace(dbg);
			/* Check user event tracing */
			if (   (   dbg->db_trace_point == QEF_T_EVENTS
				|| dbg->db_trace_point == QEF_T_LGEVENTS)
			    && (dbg->db_value_count == 0)
			   )
			{
			    if (dbg->db_trace_point == QEF_T_EVENTS)
				dbg->db_trace_point = SCS_TEVPRINT;
			    else
				dbg->db_trace_point = SCS_TEVLOG;
			    status = scf_trace(dbg);
			}
		     }
		     break;

		    case PSQ_SXFTRACE:
			STprintf(tp_info,"SX %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			status = sxf_trace((DB_DEBUG_CB *) qs_ccb->qsf_root);
			break;

		    case PSQ_OPFTRACE:
			STprintf(tp_info,"OP %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			status = opt_call((DB_DEBUG_CB *) qs_ccb->qsf_root);
			break;

		    case PSQ_QSFTRACE:
			STprintf(tp_info,"QS %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			status = qsf_trace((DB_DEBUG_CB *) qs_ccb->qsf_root);
			break;

		    case PSQ_ADFTRACE:
			STprintf(tp_info,"AD %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			    status = adb_trace((DB_DEBUG_CB *) qs_ccb->qsf_root);
			break;

		    default:
			STprintf(tp_info,"<unknown facility> %d",
				((DB_DEBUG_CB*)qs_ccb->qsf_root)->db_trace_point);
			sc0e_0_put(E_SC0024_INTERNAL_ERROR, 0);
			ret_val = E_DB_FATAL;
		}
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    ret_val = E_DB_ERROR;
		}
		/*
		** Audit use of trace point
		*/
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE
		    && DB_FAILURE_MACRO(sc0a_write_audit(
			scb,
			SXF_E_USER,	/* Type */
			SXF_A_SUCCESS | SXF_A_SESSION,  /* Action */
			tp_info,
			STlength(tp_info),
			NULL,	/* Object Owner */
			mesgid,	/* Mesg ID */
			TRUE,			/* Force record */
			0,
			&dberror		/* Error location */
		    )) )
		{
			/*
			**  sc0a_write_audit should already have logged
			**  any lower level errors
			*/
			sc0e_0_put(E_SC023C_SXF_ERROR, 0);
		}
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;
	    }


	    /* Fiddle cached dynamic flags for dbmisinfo enquiries. */
	    case PSQ_SCACHEDYN:
		Sc_main_cb->sc_csrflags &= ~SC_NO_CACHEDYN;
		Sc_main_cb->sc_csrflags |= SC_CACHEDYN;
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    /* Fiddle cached dynamic flags for dbmisinfo enquiries. */
	    case PSQ_SNOCACHEDYN:
		Sc_main_cb->sc_csrflags &= ~SC_CACHEDYN;
		Sc_main_cb->sc_csrflags |= SC_NO_CACHEDYN;
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    /* General no-ops. Nothing to do but get next statement. */
	    case PSQ_STRACE:
	    case PSQ_SSCACHEDYN:
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;


	    case PSQ_DEFREF:
	    case PSQ_DSTREF:
		/* These are distributed operations which should be rejected */
		qry_status = GCA_FAIL_MASK;
		sc0e_0_uput(E_SC020D_NO_DISTRIBUTED, 0);
		sc0e_put(E_SC0209_PSF_REQ_DISTRIBUTED, 0, 1,
			 sizeof(qmode), (PTR)&qmode,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;


	    case PSQ_SAGGR:	    /* psf passes this in the qtree */
	    case PSQ_RANGE:
	    case PSQ_PREPARE:   /* do nothing operation -- psf handles */
	    case PSQ_ALLJOURNAL:
	    case PSQ_SCARDCHK:
	    case PSQ_SNOCARDCHK:
	    case PSQ_CREATECOMP:
	    case PSQ_SRINTO:

	    /* the following qmodes are obsolete */
	    case PSQ_OBSOLETE:

		/* do nothing, only the parser cares */
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;


	    case PSQ_DESCRIBE:
	    case PSQ_DESCINPUT:
	    case PSQ_INPREPARE:

		/*
		** In these cases, the parser has prepared an SQLDA to return to
		** user.  Do so, and then move to next query
		*/

		qs_ccb = sscb->sscb_qsccb;
		qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
		qs_ccb->qsf_lk_state = QSO_EXLOCK;
		status = qsf_call(QSO_LOCK, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    ret_val = E_DB_SEVERE;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}
		qsf_object = 1;

                cquery->cur_rdesc.rd_tdesc = (PTR) qs_ccb->qsf_root;
		cquery->cur_rdesc.rd_modifier =
		    ((GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc)->gca_result_modifier;

		if (sscb->sscb_flags & SCS_STAR)
		{
		    rqf_td_desc = &cquery->cur_rdesc.rd_rqf_desc;
		    rqf_td_desc->scf_star_td_p = cquery->cur_rdesc.rd_tdesc;
		    status = scs_desc_send(scb, (SCC_GCMSG **)NULL);
		    rqf_td_desc->scf_ldb_td_p = cquery->cur_rdesc.rd_tdesc;
		}
		else
		{
		    status = scs_desc_send(scb, NULL);
		}
		if (status)
		{
		    if (status != E_DB_ERROR)
		    {
			sc0e_0_put(status, 0);
			sc0e_0_uput(status, 0);
		    }
		    qry_status = GCA_FAIL_MASK;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;

	    case PSQ_CREDBP:
	    case PSQ_CRESETDBP:
		{
		    PSY_CB	    psy_cb;
		    i4		    cont;

		    MEfill(sizeof(psy_cb), '\0', (PTR)&psy_cb);
		    psy_cb.psy_length = sizeof(psy_cb);
		    psy_cb.psy_type = PSYCB_CB;
		    psy_cb.psy_ascii_id = PSYCB_ASCII_ID;

		    STRUCT_ASSIGN_MACRO(ps_ccb->psq_txtout, psy_cb.psy_qrytext);
		    for (i = 0;
			    i < sizeof(psy_cb.psy_tabname[0].db_tab_name);
			    i++)
		    {
			psy_cb.psy_tabname[0].db_tab_name[i] =
			    cquery->cur_qname.db_cur_name[i];
		    }

		    psf_tstuff = 0;
		    status = psy_call(PSY_CREDBP, &psy_cb, sscb->sscb_psscb);
		    if (DB_FAILURE_MACRO(status))
		    {
			if (psy_cb.psy_error.err_code == E_PS0008_RETRY)
			{
			    /*
			    ** Now free the object which is in qsf that
			    ** psq_result points to.
			    */
			    if (ps_ccb->psq_result.qso_handle != 0)
			    {
				qs_ccb = sscb->sscb_qsccb;
				STRUCT_ASSIGN_MACRO(ps_ccb->psq_result,
							    qs_ccb->qsf_obj_id);
				qs_ccb->qsf_lk_state = QSO_SHLOCK;
				status = qsf_call(QSO_LOCK, qs_ccb);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
					    qs_ccb->qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
				}
				else
				{
				    status = qsf_call(QSO_DESTROY, qs_ccb);
				    if (DB_FAILURE_MACRO(status))
				    {
					scs_qsf_error(status,
						qs_ccb->qsf_error.err_code,
						    E_SC0210_SCS_SQNCR_ERROR);
				    }
				}
			    }

			    /* Now destroy the object in psq_txtout */
			    if (ps_ccb->psq_txtout.qso_handle != 0)
			    {
				qs_ccb = sscb->sscb_qsccb;
				STRUCT_ASSIGN_MACRO(ps_ccb->psq_txtout,
							qs_ccb->qsf_obj_id);
				qs_ccb->qsf_lk_state = QSO_SHLOCK;
				status = qsf_call(QSO_LOCK, qs_ccb);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
					    qs_ccb->qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
				}
				else
				{
				    status = qsf_call(QSO_DESTROY, qs_ccb);
				    if (DB_FAILURE_MACRO(status))
				    {
					scs_qsf_error(status,
						qs_ccb->qsf_error.err_code,
						    E_SC0210_SCS_SQNCR_ERROR);
				    }
				}
			    }

			    qsf_object = 0;
			    continue;
			}
			if (psy_cb.psy_error.err_code != E_PS0001_USER_ERROR)
			{
			    qry_status = GCA_FAIL_MASK;
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    sc0e_0_put(psy_cb.psy_error.err_code, 0);
			    sc0e_0_put(E_SC0215_PSF_ERROR, 0);
			    sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
			    scd_note(status, DB_PSF_ID);
			}
		    }
		    else if (qmode == PSQ_CRESETDBP)
		    {
			/* if creating a set-input procedure, we do not
			** optimize it, so the query tree does not get freed;
			** But we can't free it before now, as PSY uses parts
			** of the session cb (sscb_psscb) that are allocated
			** from the same memory stream.
			*/
			if (ps_ccb->psq_result.qso_handle != 0)
			{
			    qs_ccb = sscb->sscb_qsccb;
			    STRUCT_ASSIGN_MACRO(ps_ccb->psq_result,
						qs_ccb->qsf_obj_id);
			    qs_ccb->qsf_lk_state = QSO_SHLOCK;
			    status = qsf_call(QSO_LOCK, qs_ccb);
			    if (DB_FAILURE_MACRO(status))
			    {
				scs_qsf_error(status,
					      qs_ccb->qsf_error.err_code,
					      E_SC0210_SCS_SQNCR_ERROR);
			    }
			    else
			    {
				status = qsf_call(QSO_DESTROY, qs_ccb);
				if (DB_FAILURE_MACRO(status))
				{
				    scs_qsf_error(status,
						  qs_ccb->qsf_error.err_code,
						  E_SC0210_SCS_SQNCR_ERROR);
				}
			    }
			} /* end if qso_handle != 0 */
		    }  /* end else (qmode == PSQ_CRESETDBP) */

		    cquery->cur_row_count = -1;
		    if (!qef_wants_eiqp)
			sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;

		    cont = 0;
                    scs_chkretry(scb, status, &ret_val, &cont, &next_op,
				 &qef_wants_eiqp);
        	    if (cont == 1)
		    {
		       continue;
		    }
		}
		break;

	    case PSQ_SET_SESS_AUTH_ID:
	    {
		DB_OWN_NAME	    *new_sess_auth_id = (DB_OWN_NAME *) NULL;
		bool		    must_copy_for_psf;

		/*
		** NOTE: if/when we add support for module language, change in
		**       the SQL-session <auth id> will not necessarily
		**	 translate into change in the individual facility's
		**	 notion of effective user id; we will request that PSF,
		**	 DMF, and QEF update their internal copies of effective
		**	 user identifier ONLY IF ics_eusername == &ics_susername
		*/

		/*
		** will get set to TRUE after PSF eff. user id has been reset;
		** if some error occurs after we have reset the PSF user id,
		** we'll try to change it to its previous value
		*/
		bool		    psf_has_new_userid = FALSE;
		/*
		** will get set to TRUE after DMF eff. user id has been reset;
		** if some error occurs after we have reset the DMF user id,
		** we'll try to change it to its previous value
		*/
		bool		    dmf_has_new_userid = FALSE;
		/*
		** will get set to TRUE after QEF eff. user id has been reset;
		** if some error occurs after we have reset the QEF user id, (I
		** know that it cannot happen now; it is here in case we decide
		** that something else must be done after we have reset all the
		** ids) we'll try to change it to its previous value
		*/
		bool		    qef_has_new_userid = FALSE;
		/*
		** TRUE if the current <auth id> is based on session <auth id>
		*/
		bool		    curr_id_based_on_sess_id;

		/*
		** For security auditing...
		*/

		/*
		** need to verify that we are not in the middle of a transaction
		*/
		qe_ccb = sscb->sscb_qeccb;
		status = qef_call(QEC_INFO, qe_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    qry_status = GCA_FAIL_MASK;
		    ret_val = E_DB_ERROR;
		    scs_qef_error(status,
				qe_ccb->error.err_code,
				E_SC0210_SCS_SQNCR_ERROR,
				scb);
		    break;
		}
		else if (qe_ccb->qef_stat != QEF_NOTRAN)
		{
		    sc0e_0_uput(E_SC002D_SET_SESS_AUTH_IN_XACT, 0);
		    qry_status = GCA_FAIL_MASK;
		    sscb->sscb_state = SCS_INPUT;
		    *next_op = CS_EXCHANGE;
		    break;
		}

		/*
		** remember whether the current <auth id> is based on
		** session <auth id>
		*/
		curr_id_based_on_sess_id =
		    (sscb->sscb_ics.ics_eusername == &sscb->sscb_ics.ics_susername);

		/*
		** if (
		**     user wants to use the initial session <auth id> and it is
		**     different from the session <auth id>
		**     OR
		**     user wants to use the system <auth id> and it is
		**     different from the session <auth id>
		**     OR
		**     user wants to use the current <auth id> and it is
		**     different from the session <auth id>
		**     OR
		**     the id explicitly specified by the user is different from
		**     the session <auth id>
		**    )
		** {
		**	if (current <auth id> is based on session <auth id>
		**	    (i.e. we are not executing a module with <module
		**	    auth id>)
		**	   )
		**	{
		**	    notify PSF, DMF if a local connection, and QEF of
		**	    the new effective user id;
		**	}
		**
		**	if (eff. user ids have been successfully updated in PSF,
		**	    DMF and QEF
		**	    or
		**	    they did not have to be updated because current
		**	    <auth id> is not based on session <auth id>
		**	   )
		**	{
		**	    update session <auth id> in SCS_ICS
		**	}
		** }
		*/

		if (ps_ccb->psq_ret_flag & PSQ_USE_INITIAL_USER)
		{
		    /* SET SESSION AUTHORIZATION INITIAL_USER */
		    new_sess_auth_id = &sscb->sscb_ics.ics_iusername;
		    must_copy_for_psf = TRUE;
		}
		else if (ps_ccb->psq_ret_flag & PSQ_USE_SYSTEM_USER)
		{
		    /* SET SESSION AUTHORIZATION SYSTEM_USER */
		    new_sess_auth_id = &sscb->sscb_ics.ics_rusername;
		    must_copy_for_psf = TRUE;
		}
		else if (ps_ccb->psq_ret_flag & PSQ_USE_CURRENT_USER)
		{
		    /* SET SESSION AUTHORIZATION { USER | CURRENT_USER } */
		    if (!curr_id_based_on_sess_id)
		    {
			new_sess_auth_id = sscb->sscb_ics.ics_eusername;
			must_copy_for_psf = TRUE;
		    }
		}
		else if (~ps_ccb->psq_ret_flag & PSQ_USE_SESSION_USER)
		{
		    /* SET SESSION AUTHORIZATION <user name> */
		    new_sess_auth_id = &ps_ccb->psq_user.db_tab_own;
		    must_copy_for_psf = FALSE;
		}


		if (  new_sess_auth_id
		    && MEcmp((PTR) &sscb->sscb_ics.ics_susername,
			     (PTR) new_sess_auth_id, sizeof(DB_OWN_NAME)))
		{
		    /*
		    ** session <auth id> is being changed - tell PSF,
		    ** DMF for local, and
		    ** QEF to update their copies of effective user id ONLY IF
		    ** the current <auth id> is based on the session <auth id>
		    */
		    if (curr_id_based_on_sess_id)
		    {
			DB_STATUS	    save_status;
			QEF_ALT		    qef_alt[1];

			/*
			** these fields will be the same whether we try to reset
			** the DMF eff. user id or return it to its old value
			*/
                        if (!(sscb->sscb_flags & SCS_STAR))
			{
			    dm_ccb = sscb->sscb_dmccb;
			    dm_ccb->dmc_op_type = DMC_SESSION_OP;
			    dm_ccb->dmc_flags_mask = 0L;
			    dm_ccb->dmc_name.data_in_size = sizeof(DB_OWN_NAME);
			}

			/*
			** the following is needed to tell QEF to alter its
			** copy of the effective user id
			*/
			qe_ccb->qef_asize = 1;
			qe_ccb->qef_alt = qef_alt;
			qef_alt[0].alt_code = QEC_EFF_USR_ID;

			/*
			** if an error is encountered when resetting one of the
			** effective user ids, we may go for a second pass
			** ("recovery pass") through the loop, this time
			** returning changed effective user id to their old
			** values
			*/
			for (save_status = E_DB_OK; ; save_status = status)
			{
			    /* is this a "recovery pass"? */
			    if (save_status != E_DB_OK)
			    {
				/*
				** if SEVERE or FATAL error occurred, just bail
				** out
				*/
				if (save_status != E_DB_ERROR)
				{
				    break;
				}

				/*
				** will reset session <auth id> to the
				** old value
				*/
				new_sess_auth_id = &sscb->sscb_ics.ics_susername;

				/*
				** make sure PSF gets its value reset
				*/
				must_copy_for_psf = TRUE;
			    }

			    /*
			    ** if (trying to reset the PSF eff. user id for the
			    **     first time
			    **     OR
			    **     some error occurred and we need to reset PSF
			    **	   eff. user id to its old value
			    **    )
			    ** {
			    **     try to change PSF eff. user id;
			    ** }
			    */

			    if (save_status == E_DB_OK || psf_has_new_userid)
			    {
				if (must_copy_for_psf)
				{
				    STRUCT_ASSIGN_MACRO(*new_sess_auth_id,
					ps_ccb->psq_user.db_tab_own);
				}

				status = psq_call(PSQ_RESET_EFF_USER_ID, ps_ccb,
				    sscb->sscb_psscb);

				if (DB_FAILURE_MACRO(status))
				{
				    qry_status = GCA_FAIL_MASK;
				    sc0e_0_put(E_SC0215_PSF_ERROR, 0);
				    sc0e_0_put(ps_ccb->psq_error.err_code, 0);
				    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
				    scd_note(status, DB_PSF_ID);

				    /*
				    ** if this is the first pass, we don't need
				    ** to reset anything, so we just leave the
				    ** loop; if an error occurred during the
				    ** "recovery pass", the session has become
				    ** inconsistent, so we'll kill it even if
				    ** the error was not SEVERE or FATAL
				    */
				    if (save_status != E_DB_OK &&
					status < E_DB_SEVERE)
				    {
					status = E_DB_SEVERE;
				    }

				    break;
				}

				/*
				** first time through the loop remember that we
				** have reset PSF eff. user id; during the
				** "recovery pass" it reverts to its old value
				*/
				psf_has_new_userid = !psf_has_new_userid;
			    }

			    /*
			    ** if ((this is a local session) and
			    **     (trying to reset the DMF eff. user id for the
			    **      first time
			    **      OR
			    **      some error occurred and we need to reset DMF
			    **	    eff. user id to its old value
			    **     ))
			    ** {
			    **     try to change DMF eff. user id;
			    ** }
			    */

			    if (!(sscb->sscb_flags & SCS_STAR) &&
			    	(save_status == E_DB_OK || dmf_has_new_userid))
			    {
				dm_ccb->dmc_name.data_address =
				    (char *) new_sess_auth_id;

				status = dmf_call(DMC_RESET_EFF_USER_ID,
				    dm_ccb);

				if (DB_FAILURE_MACRO(status))
				{
				    qry_status = GCA_FAIL_MASK;
				    sc0e_0_put(E_SC0213_DMF_ERROR, 0);
				    sc0e_0_put(dm_ccb->error.err_code, 0);
				    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
				    scd_note(status, DB_DMF_ID);

				    /*
				    ** if this is the first pass, return to the
				    ** top of the loop since we will need to
				    ** reset the PSF eff. user id; if an error
				    ** occurred during the "recovery pass",
				    ** the session has become inconsistent, so
				    ** we'll kill it even if the error was not
				    ** SEVERE or FATAL
				    */

				    if (save_status == E_DB_OK)
				    {
					continue;
				    }
				    else
				    {
					if (status < E_DB_SEVERE)
					    status = E_DB_SEVERE;
					break;
				    }
				}

				/*
				** first time through the loop remember that we
				** have reset DMF eff. user id; during the
				** "recovery pass" it reverts to its old value
				*/
				dmf_has_new_userid = !dmf_has_new_userid;
			    }

			    /*
			    ** if (trying to reset the QEF eff. user id for the
			    **     first time
			    **     OR
			    **     some error occurred and we need to reset QEF
			    **	   eff. user id to its old value
			    **    )
			    ** {
			    **     try to change QEF eff. user id;
			    ** }
			    */

			    if (save_status == E_DB_OK || qef_has_new_userid)
			    {
				qef_alt[0].alt_value.alt_own_name =
				    new_sess_auth_id;

				status = qef_call(QEC_ALTER, qe_ccb);
				if (DB_FAILURE_MACRO(status))
				{
				    qry_status = GCA_FAIL_MASK;
				    sc0e_0_put(E_SC0216_QEF_ERROR, 0);
				    sc0e_0_put(qe_ccb->error.err_code, 0);
				    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
				    scd_note(status, DB_QEF_ID);

				    /*
				    ** if this is the first pass, return to the
				    ** top of the loop since we will need to
				    ** reset the PSF and DMF eff. user id; if an
				    ** error occurred during the
				    ** "recovery pass", the session has become
				    ** inconsistent, so we'll kill it even if
				    ** the error was not SEVERE or FATAL
				    */

				    if (save_status == E_DB_OK)
				    {
					continue;
				    }
				    else
				    {
					if (status < E_DB_SEVERE)
					    status = E_DB_SEVERE;
					break;
				    }
				}

				/*
				** first time through the loop remember that we
				** have reset QEF eff. user id; during the
				** "recovery pass" it reverts to its old value
				*/
				qef_has_new_userid = !qef_has_new_userid;
			    }

			    /*
			    ** this point is reached if we have successfully
			    ** reset eff. user ids for PSF, DMF (for a local
			    ** session), and QEF or if
			    ** an error occurred and we have successfully undone
			    ** the changes made prior to the error.  In the
			    ** latter case we need to return the error status
			    ** which was saved in save_status
			    */
			    if (save_status != E_DB_OK)
			    {
				status = save_status;
			    }
			    break;
			}
		    }
		    /*
		    ** did we encounter any errors trying to reset user id?
		    */
		    if (DB_FAILURE_MACRO(status))
		    {
			/*
			**	Audit failure
			*/
			if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE
			    && DB_FAILURE_MACRO(sc0a_write_audit(
				scb,
				SXF_E_USER,	/* Type */
				SXF_A_FAIL | SXF_A_SETUSER,  /* Action */
				(char*)new_sess_auth_id,/* Object is new user */
				sizeof(*new_sess_auth_id),/* Object is new user */
				NULL,	/* Object Owner */
				I_SX2033_SET_USER_AUTH,	/* Mesg ID */
				TRUE,			/* Force record */
				sscb->sscb_ics.ics_rustat,
				&dberror		/* Error location */
			    )) )
			{
			    /*
			    **	sc0a_write_audit should already have logged
			    **  any lower level errors
			    */
			    sc0e_0_put(E_SC023C_SXF_ERROR, 0);
			}
			break;
		    }
		    /*
		    ** Indicate to FEs that the session has a new effective
		    ** user + that repeat query ids associated with this session
		    ** must be flushed
		    */
		    cquery->cur_result = GCA_NEW_EFF_USER_MASK | GCA_FLUSH_QRY_IDS_MASK;
		    /*
		    **	Copy the new effective user id into SCS_ICS
		    */
		    STRUCT_ASSIGN_MACRO((*new_sess_auth_id),
					sscb->sscb_ics.ics_susername);
		    /*
		    **	Log successful change of effective user id
		    **
		    ** NOTE: Using SXF_A_SETUSER will cause SXF to
		    ** refresh its cache after writing the record, so
		    ** we have to update the SCS_ICS *prior* to writing the
		    ** record.
		    */
		    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE
			&& DB_FAILURE_MACRO(sc0a_write_audit(
			scb,
			SXF_E_USER,	/* Type */
			SXF_A_SUCCESS | SXF_A_SETUSER,  /* Action */
			(char*)new_sess_auth_id, /* Object is new user */
			sizeof(*new_sess_auth_id),
			NULL,			/* Object Owner */
			I_SX2033_SET_USER_AUTH,	/* Mesg ID */
			TRUE,			/* Force record */
			sscb->sscb_ics.ics_rustat,
			&dberror		/* Error location */
			)))
		    {
		        /*
		        **  sc0a_write_audit should already have logged
		        **  any lower level errors
		        */
		        sc0e_0_put(E_SC023C_SXF_ERROR, 0);
		    }
		}

		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;
	    }

	    case PSQ_0_CRT_LINK:  /* STAR Create Link */
	    case PSQ_REG_LINK:    /* STAR Register    */

	        qe_ccb = sscb->sscb_qeccb;
	        qe_ccb->qef_cb = NULL;
	        qe_ccb->qef_eflag = QEF_EXTERNAL;
	        qe_ccb->qef_modifier = QEF_SSTRAN;
	        qe_ccb->qef_qso_handle = ps_ccb->psq_result.qso_handle;
	        status = qef_call(QED_CLINK, qe_ccb);
	        if (DB_FAILURE_MACRO(status))
	        {
		    qry_status = GCA_FAIL_MASK;
		    if (sscb->sscb_interrupt)
			break;
		    scs_qef_error(status, qe_ccb->error.err_code,
				  DB_SCF_ID, scb);
	        }
	        *next_op = CS_EXCHANGE;
	        sscb->sscb_state = SCS_INPUT;
	        break;

	    case PSQ_DIRCON: 	/* STAR Direct Connect */
	        status = scs_dccnct(scb, &qry_status, next_op, op_code);
	        break;

	    case PSQ_DDCOPY: 	/* STAR Copy */
	        status = scs_dccopy(scb, &qry_status, next_op, op_code);
	        break;

	    case PSQ_DIREXEC: 	/* STAR Direct Execute Immediate */
	        status = scs_dcexec(scb, &qry_status, next_op, op_code);
	        break;

	    case PSQ_DDEXECPROC: /* STAR, dynamic execute procedure */
		status = scs_dcxproc(scb, &qry_status, next_op, op_code);
		break;
		
	    case PSQ_SETBATCHCOPYOPTIM:
		/* "set batch_copy_optim"; nothing to do here
		** it's all covered in the parser.
		*/
	        *next_op = CS_EXCHANGE;
	        sscb->sscb_state = SCS_INPUT;
		status = E_DB_OK;
		break;

	    default:
		sc0e_put(E_SC020A_UNRECOGNIZED_QMODE, 0, 1,
			 sizeof(qmode), (PTR)&qmode,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);

		/* Also check for the query text handle for E.I. query text */

		qe_ccb = sscb->sscb_qeccb;
		if ((sscb->sscb_thandle) ||
				(qe_ccb->qef_qsf_handle.qso_handle))
		    sc0e_put(E_SC020B_UNRECOGNIZED_TEXT, 0, 1,
			     ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrysize,
			     (PTR)((PSQ_QDESC *) sscb->sscb_troot)->psq_qrytext,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
		else
		    sc0e_put(E_SC020B_UNRECOGNIZED_TEXT, 0, 1,
			     9, (PTR)"<UNKNOWN>",
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0);

		sc0e_uput(E_SC020A_UNRECOGNIZED_QMODE, 0, 1,
			  sizeof(qmode), (PTR)&qmode,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);

		/* Also check for the query text handle for E.I. query text */

		if ((sscb->sscb_thandle) ||
			(qe_ccb->qef_qsf_handle.qso_handle))
		    sc0e_uput(E_SC020B_UNRECOGNIZED_TEXT, 0, 1,
			((PSQ_QDESC *) sscb->sscb_troot)->psq_qrysize,
			(PTR)((PSQ_QDESC *) sscb->sscb_troot)->psq_qrytext,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
		else
		    sc0e_put(E_SC020B_UNRECOGNIZED_TEXT, 0, 1,
			     9, "<UNKNOWN>",
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0);
		qry_status = GCA_FAIL_MASK;
		sscb->sscb_state = SCS_INPUT;
		*next_op = CS_EXCHANGE;
		break;
	}   /* end of massive switch stmt */
	break;
    }	    /* end of massive for loop */

    /* Label for special statement-done, e.g. COPY got an error. */
massive_for_exit:

    if ((sscb->sscb_interrupt)
	    || deadlock_occurred != 0
	    || on_user_error
	    || (sscb->sscb_force_abort == SCS_FAPENDING) )
    {
	/* Interrupt / attn / force-abort / deadlock */

	scc_dispose(scb);	/* Clear out any leftover messages */

	if( sscb->sscb_flags & SCS_DROP_PENDING )
	{
	    /*
	    ** Drop assoc, if we are supposed to.
	    ** Leave completion on while this disassoc is running.
	    */
	    sscb->sscb_flags &= ~SCS_DROP_PENDING;
	    sscb->sscb_flags |= SCS_DROPPED_GCA;
	    (void) scs_disassoc( cscb->cscb_assoc_id );
	}

        /*
        ** Mark session so that scc_gcomplete() will not complete GCC io's
        */
        if (sscb->sscb_interrupt < 0)
            sscb->sscb_interrupt = -sscb->sscb_interrupt;

	if ((qmode && (qmode != PSQ_RETRIEVE))		/* not a retrieve   */
							/* and existent */
		    || (deadlock_occurred != 0)
		    || on_user_error
		    || (sscb->sscb_interrupt == SCS_INTERRUPT)
		    || (sscb->sscb_force_abort == SCS_FAPENDING))
	{
	    if (sscb->sscb_force_abort == SCS_FAPENDING)
            {
		sscb->sscb_force_abort = SCS_FORCE_ABORT;
		MEfill(SCS_ACT_SIZE, '\0',
		            sscb->sscb_ics.ics_act1);
                MEcopy("Performing Force Abort Processing", 33,
                            sscb->sscb_ics.ics_act1);
                sscb->sscb_ics.ics_l_act1 = 33;
            }
	    else if (deadlock_occurred)
	    {
		MEfill(SCS_ACT_SIZE, '\0',
		            sscb->sscb_ics.ics_act1);
		MEcopy("Aborting on behalf of a deadlock", 32,
			    sscb->sscb_ics.ics_act1);
		sscb->sscb_ics.ics_l_act1 = 32;
	    }
            else if (on_user_error)
	    {
                MEfill(SCS_ACT_SIZE, '\0',
                            sscb->sscb_ics.ics_act1);
                MEcopy("Aborting on behalf of user error", 34,
                            sscb->sscb_ics.ics_act1);
                sscb->sscb_ics.ics_l_act1 = 34;
	    }
	    else
            {
		MEfill(SCS_ACT_SIZE, '\0',
		            sscb->sscb_ics.ics_act1);
                MEcopy("Aborting on behalf of an Interrupt", 34,
                            sscb->sscb_ics.ics_act1);
                sscb->sscb_ics.ics_l_act1 = 34;
            }

	    CScancelled(0);
	    CSintr_ack();

	    /*
	    ** bug #65094
	    ** If you interrupt a DBU operation (modify, index, relocation, ...
	    ** then x.cur_qe_cb will be a pointer to a QEU_CB, not a QEF_RCB
	    ** and can't be used for the QEC_INFO call.  In that case you must
	    ** use the session control block in sscb->sscb_qeccb.  If
	    ** you use the QEU_CB you will trash memory causing random effects
	    ** (AV's in qsf ...).
	    */

	    if ((cquery->cur_qe_cb) &&
		(((QEF_RCB *) cquery->cur_qe_cb)->qef_type == QEFRCB_CB))
		qe_ccb = (QEF_RCB *) cquery->cur_qe_cb;
	    else
		qe_ccb = sscb->sscb_qeccb;

	    /* If this is COPY, shut down the COPY workers */
	    if (qmode == PSQ_COPY)
	    {
		qe_copy = qe_ccb->qeu_copy;
		/* Paranoia in case copy END has already run: */
		if (qe_copy != NULL && qe_copy->qeu_copy_ctl != NULL)
		{
		    qe_copy->qeu_stat |= CPY_FAIL;
		    status = qef_call(QEU_E_COPY, qe_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			ret_val = status;
			scs_qef_error(status, qe_ccb->error.err_code,
				E_SC0210_SCS_SQNCR_ERROR, scb);
		    }
		}
	    }

	    /* in case this isn't the main one */
	    qry_status = GCA_FAIL_MASK;
 	    qe_ccb->qef_cb = sscb->sscb_qescb;
	    status = qef_call(QEC_INFO, qe_ccb);

	    if (DB_FAILURE_MACRO(status))
	    {
		ret_val = E_DB_ERROR;
		scs_qef_error(status,
			    qe_ccb->error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR,
			    scb);
	    }
	    qe_ccb->qef_db_id = sscb->sscb_ics.ics_opendb_id;
	    qe_ccb->qef_flag = 0;
	    if ((qe_ccb->qef_stat == QEF_MSTRAN)
                &&
		  /* Distributed has no internal savepoint */
                (!(sscb->sscb_flags & SCS_STAR))
	       )

	    {
		qe_ccb->qef_modifier = QEF_MSTRAN;
		if ((sscb->sscb_ciu == 0)
			&& (deadlock_occurred == 0)
			&& (!on_user_error)
			&& (!sscb->sscb_force_abort))
		{
		    qe_ccb->qef_spoint = &Qef_s_cb->qef_sp_savepoint;
		}
		else
		{
		    qe_ccb->qef_spoint = NULL;
		}
	    }
	    else
	    {
		qe_ccb->qef_modifier = qe_ccb->qef_stat;
		qe_ccb->qef_spoint = NULL;
	    }

	    if (qe_ccb->qef_stat != QEF_NOTRAN)
	    {
		was_xact = 1;

		/* Don't need to abort if distributed and interrupted */
		if (! ((sscb->sscb_flags & SCS_STAR) &&
		       (sscb->sscb_interrupt == SCS_INTERRUPT)))
		{
		    qe_ccb->qef_no_xact_stmt = FALSE;

		    status = qef_call(QET_ABORT, qe_ccb);

		    if (DB_FAILURE_MACRO(status))
		    {
		        scs_qef_error(status, qe_ccb->error.err_code,
				        E_SC0210_SCS_SQNCR_ERROR, scb);
		        scd_note(status, DB_QEF_ID);
		    }
		}

		if (qmode == PSQ_DDCOPY)
		{
		    qe_ccb = sscb->sscb_qeccb;
		    qe_ccb->qef_modifier = QEF_SSTRAN;
		    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q2_lang = DB_SQL;
		    qe_ccb->qef_r3_ddb_req.qer_d4_qry_info.qed_q1_timeout = 0;
		    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c4_exchange_p =
		        (PTR)&sscb->sscb_dccb->dc_dcblk;

		    qe_ccb->qef_r3_ddb_req.qer_d6_conn_info.qed_c5_tran_ok_b
		        = FALSE;

		    status = scs_dcinterrupt(scb, &rv);
		    if (DB_FAILURE_MACRO(status))
		    {
		        sc0e_0_put(status, 0);
		        sc0e_0_uput(status, 0);
		        scd_note(E_DB_SEVERE, DB_SCF_ID);
		    }
		    status = scs_dcendcpy(scb);
		    if( DB_FAILURE_MACRO(status) )
		    {
		        scs_qef_error( status, qe_ccb->error.err_code,
		                        DB_SCF_ID, scb );
		    }
		} /*qmode == PSQ_DDCOPY*/

            } /* qe_ccb->qef_stat != QEF_NOTRAN */

	    if (qmode == PSQ_COPY)
	    {
		qe_copy = qe_ccb->qeu_copy;
		/*
		** Free up the memory allocated for COPY processing and
		** destroy the control structures allocated by PSF.
		*/
		if ((qe_copy) && (qe_copy->qeu_sbuf != NULL))
		{
		    sc0m_deallocate(0, &qe_copy->qeu_sbuf);
		}

		qs_ccb = sscb->sscb_qsccb;
		qs_ccb->qsf_obj_id.qso_handle = cquery->cur_qp.qso_handle;
		qs_ccb->qsf_lk_id = cquery->cur_lk_id;
		/*
		** Here we need to verify that we have gotten to the
		** point where we have fetched the PSF created QSF object.
		** If we have not yet managed to fetch this object (i.e. the
		** interrupt happened after PSF finished but before we got
		** around to actually dealling with the object), then we
		** need to do special things to delete the PSF created
		** object.
		*/

		if (    (qs_ccb->qsf_root == cquery->cur_qe_cb)
		     && (qs_ccb->qsf_obj_id.qso_handle))
		{
		    status = qsf_call(QSO_DESTROY, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
		    }

		    cquery->cur_qe_cb = NULL;
		    cquery->cur_qp.qso_handle = 0;
		}
	    }


	    if ((sscb->sscb_state == SCS_CP_ERROR) ||
		(sscb->sscb_state == SCS_CP_FROM))
	    {
		/*
		** If there was a force abort, set the state to CP error,
		** and let the system suck up the FE messages, until the FE says
		** that it is our turn to talk.  We will leave the force abort
		** flag set (if it was set).  That will cause the system to send
		** force abort messages (via scc_send()) the next time we get to
		** talk.  Which is what is desired.
		*/

		if (sscb->sscb_force_abort == SCS_FORCE_ABORT)
		{
		    scs_cpinterrupt(scb);
		    sc0e_0_put(E_QE0024_TRANSACTION_ABORTED, 0);
		    sc0e_0_uput(E_QE0024_TRANSACTION_ABORTED, 0);
		    sscb->sscb_force_abort = SCS_FACOMPLETE;
		    sscb->sscb_state = SCS_CP_ERROR;
		    sscb->sscb_cfac = DB_CLF_ID;
		    *next_op = CS_INPUT;
		    return(E_DB_OK);
		}
	    }

	    /* check for force abort while scs_blob_fetch in progress */
	    if (sscb->sscb_force_abort == SCS_FORCE_ABORT &&
		sscb->sscb_dmm)
	    {
		TRdisplay("Force abort during scs_blob_fetch\n");
		sscb->sscb_req_flags |= SCS_EAT_INPUT_MASK;
		sscb->sscb_dmm = 0; /* No blob in progress */
		sc0e_0_put(E_QE0024_TRANSACTION_ABORTED, 0);
		sc0e_0_uput(E_QE0024_TRANSACTION_ABORTED, 0);
		sscb->sscb_force_abort = SCS_FACOMPLETE;
		*next_op = CS_INPUT;
		return(E_DB_OK);
	    }

	    /*
	    ** STAR needs to clean all its associations to the LDB.
	    ** (schka24) used to have a broken test against cur_state, not
	    ** real sure what it was trying to do ...
	    */
	    if ( (qmode == PSQ_RETRIEVE)
		&&
		 (sscb->sscb_flags & SCS_STAR)
	       )
	    {
		(void)qef_call(QEQ_ENDRET, qe_ccb);
	    }

	    if (cquery->cur_amem)
	    {
		sc0m_deallocate(0, &cquery->cur_amem);
		cquery->cur_qef_data = NULL;
	    }
	}
	else if (qmode == PSQ_RETRIEVE)
	{
	    if (sscb->sscb_interrupt)
	    {
		sscb->sscb_state = SCS_ENDRET;
	    }
	    CScancelled(0);
	    CSintr_ack();
	    if (cquery->cur_qe_cb)
		qe_ccb = (QEF_RCB *) cquery->cur_qe_cb;
	    else
		qe_ccb = sscb->sscb_qeccb;

	    /*
	    ** If the retrieve is still ongoing w.r.t. QEF, then
	    ** tell qef to terminate the query, and deallocate any
	    ** space we may have reserved for it.  Otherwise,
	    ** there is nothing to do, and we just ``act finished.''
	    */

	    if (cquery->cur_amem != NULL)
	    {

		status = qef_call(QEQ_ENDRET, qe_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    /* This error returned if query has been completed */
		    if (qe_ccb->error.err_code !=
				    E_QE0008_CURSOR_NOT_OPENED)
		    {
			qry_status = GCA_FAIL_MASK;
			scd_note(status, DB_QEF_ID);
			scs_qef_error(status,
			    qe_ccb->error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR,
			    scb);
		    }
		}
		sc0m_deallocate(0, &cquery->cur_amem);
		cquery->cur_qef_data = NULL;
	    }
	}
        sscb->sscb_ics.ics_l_act1 = 0;
    }

    if ((qef_wants_ruleqp && !tproc_cur_load_qp) || (qef_wants_eiqp))
    {
	/*
	** If we got an error outside of QEF while in the process of reloading
	** a QP for  QEF, we must call back to QEF to ensure it cleans up any
	** saved resources.  Here too - the original RCB must be left intact.
	*/

	/* Added support for dealing with errors encountered while parsing the
        ** E.I. text.
        */
	qe_ccb = sscb->sscb_qeccb;
	if ((qe_ccb->qef_intstate & QEF_DBPROC_QP) || (qe_ccb->qef_intstate &
					QEF_EXEIMM_PROC))
	{
	    if (qe_ccb->qef_intstate & QEF_DBPROC_QP)
	       qe_ccb->qef_intstate |= QEF_CLEAN_RSRC;

	    if (qe_ccb->qef_intstate & QEF_EXEIMM_PROC)
               qe_ccb->qef_intstate |= QEF_EXEIMM_CLEAN;

	    status = qef_call(QEQ_QUERY, qe_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		qry_status = GCA_FAIL_MASK;
		scd_note(status, DB_QEF_ID);
		scs_qef_error(status, qe_ccb->error.err_code,
			      E_SC0210_SCS_SQNCR_ERROR, scb);
	    }
	}
    } /* If QEF was mid-processing */

    if (qsf_object)
    {
	/*
	** Now free the qe_ccb control block which is in qsf.
	*/
	status = qsf_call(QSO_DESTROY, qs_ccb);    /* qs_ccb is set up above */
	if (DB_FAILURE_MACRO(status))
	{
	    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		    E_SC0210_SCS_SQNCR_ERROR);
	}
    }
    else if (psf_stuff)
    {
	qs_ccb = sscb->sscb_qsccb;
	if (psq_tree.qso_handle != 0)
	{
	    qs_ccb->qsf_obj_id.qso_handle = psq_tree.qso_handle;
	    qs_ccb->qsf_lk_state = QSO_EXLOCK;
	    status = qsf_call(QSO_LOCK, qs_ccb);
	    if (DB_FAILURE_MACRO(status) &&
			(qs_ccb->qsf_error.err_code != E_QS0019_UNKNOWN_OBJ))
	    {
		qry_status = GCA_FAIL_MASK;
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC0210_SCS_SQNCR_ERROR);
		ret_val = E_DB_SEVERE;
	    }
	    else if (qs_ccb->qsf_error.err_code != E_QS0019_UNKNOWN_OBJ)
	    {
		/* qs_ccb is set up above */
		status = qsf_call(QSO_DESTROY, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
		}
	    }
	}

	if (psf_stuff == 2)
	{
	    qs_ccb->qsf_obj_id.qso_handle = 0;
	    qs_ccb->qsf_obj_id.qso_lname = sizeof(DB_CURSOR_ID);
	    MEcopy((PTR)&psq_plan, sizeof(psq_plan), qs_ccb->qsf_obj_id.qso_name);
	    qs_ccb->qsf_obj_id.qso_type = QSO_QP_OBJ;
	    qs_ccb->qsf_lk_state = QSO_SHLOCK;
	    status = qsf_call(QSO_GETHANDLE, qs_ccb);
	    if (DB_FAILURE_MACRO(status) &&
		    (qs_ccb->qsf_error.err_code != E_QS0019_UNKNOWN_OBJ))
	    {
		qry_status = GCA_FAIL_MASK;
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC0210_SCS_SQNCR_ERROR);
		ret_val = E_DB_SEVERE;
	    }
	    else if (qs_ccb->qsf_error.err_code != E_QS0019_UNKNOWN_OBJ)
	    {
		status = qsf_call(QSO_DESTROY, qs_ccb);    /* qs_ccb is set up above */
		if (DB_FAILURE_MACRO(status))
		{
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
		}
	    }
	}
    }
    if (psf_tstuff)
    {
	qs_ccb = sscb->sscb_qsccb;
	STRUCT_ASSIGN_MACRO(psq_text, qs_ccb->qsf_obj_id);
	qs_ccb->qsf_lk_state = QSO_EXLOCK;
	status = qsf_call(QSO_LOCK, qs_ccb);
	if (DB_FAILURE_MACRO(status) &&
		    (qs_ccb->qsf_error.err_code != E_QS0019_UNKNOWN_OBJ))
	{
	    qry_status = GCA_FAIL_MASK;
	    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		    E_SC0210_SCS_SQNCR_ERROR);
	    ret_val = E_DB_SEVERE;
	}
	else if (qs_ccb->qsf_error.err_code != E_QS0019_UNKNOWN_OBJ)
	{
	    status = qsf_call(QSO_DESTROY, qs_ccb);    /* qs_ccb is set up above */
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC0210_SCS_SQNCR_ERROR);
	    }
	}
    }

    if (abort_action == DB_QSF_ID)
    {
	/*
	** Then we must free the QSF resident query plan created
	*/
	qs_ccb = sscb->sscb_qsccb;
	qs_ccb->qsf_lk_id = sscb->sscb_plk_id;
	qs_ccb->qsf_obj_id.qso_handle = cquery->cur_qp.qso_handle;
	qs_ccb->qsf_lk_state = QSO_EXLOCK;
	status = qsf_call(QSO_LOCK, qs_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    qry_status = GCA_FAIL_MASK;
	    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		    E_SC0210_SCS_SQNCR_ERROR);
	    ret_val = E_DB_SEVERE;
	}
	status = qsf_call(QSO_DESTROY, qs_ccb);    /* qs_ccb is set up above */
	if (DB_FAILURE_MACRO(status))
	{
	    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		    E_SC0210_SCS_SQNCR_ERROR);
	}
    }
    if ((ret_val != E_DB_OK)
	    || (sscb->sscb_noretries >= SCS_RETRY_LIMIT))
    {
	sscb->sscb_state = SCS_INPUT;
	*next_op = CS_EXCHANGE;
	if (sscb->sscb_noretries >= SCS_RETRY_LIMIT)
	{
	    sc0e_0_uput(E_US1265_QEP_INVALID, 0);
	    sc0e_0_put(E_US1265_QEP_INVALID, 0);
	}
    }
    if (qry_status & GCA_FAIL_MASK)
    {
	if (xact_state  && (xact_state != QEF_X_MACRO(sscb->sscb_qescb)) )
	{
	    /* then an xact has ended (QEF ended it) -- close all cursors */

	    if (ult_check_macro(&sscb->sscb_trace, SCS_TEVENT, NULL, NULL))
	    {
		sc0e_trace("Transaction terminated because of query error");
	    }
	    /* Transaction ended, need to end group */
	    if (cscb->cscb_batch_count > 0)
		cscb->cscb_eog_error = TRUE;

	    if (sscb->sscb_dis_tran_id.db_dis_tran_id_type
			== DB_XA_DIS_TRAN_ID_TYPE)
	    {
		/* Unmark SCB as using XA transaction management */
		sscb->sscb_flags &= ~SCS_XA_START;

		/* Invalidate Session's distributed tranid */
		sscb->sscb_dis_tran_id.db_dis_tran_id_type = 0;

		if (Sc_main_cb->sc_trace_errno == E_SC0206_CANNOT_PROCESS)
		    scs_xatrace(scb, qmode, 0, "QEFABORT",
		    (DB_DIS_TRAN_ID *)NULL, qry_status);
	    }

	    /* When aborting xacts, make sure that all psf cursors are closed */

	    scs_cursor_remove(scb, 0);
	    ps_ccb = sscb->sscb_psccb;
	    sscb->sscb_cpy_qeccb = NULL;
	    status = psq_call(PSQ_COMMIT, ps_ccb, sscb->sscb_psscb);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (ps_ccb->psq_error.err_code == E_PS0002_INTERNAL_ERROR)
		{
		    sc0e_0_put(E_SC0215_PSF_ERROR, 0);
		    sc0e_0_put(ps_ccb->psq_error.err_code, 0);
		    scd_note(status, DB_PSF_ID);
		    ret_val = E_DB_ERROR;
		}
	    }
	}
    }

    if ((sscb->sscb_state == SCS_INPUT)
		|| (sscb->sscb_interrupt)
		|| (sscb->sscb_force_abort == SCS_FORCE_ABORT)
		|| (qry_status == GCA_FAIL_MASK))
    {
	SC0M_OBJECT	    *object;


	/*
	** save last query before re-initializing
	**
	** Use MEcopy rather than STcopy as the query
	** isn't reliably EOS-terminated.
	*/
	MEcopy((char*)&sscb->sscb_ics.ics_qbuf,
		       sscb->sscb_ics.ics_l_qbuf,
	       (char*)&sscb->sscb_ics.ics_lqbuf);
	sscb->sscb_ics.ics_l_lqbuf = sscb->sscb_ics.ics_l_qbuf;
        sscb->sscb_ics.ics_l_qbuf = 0;

	/*
	** If we allocated the QEF_PARAM block deallocate it now
	*/
	if (cquery->cur_qparam != NULL &&
		cquery->cur_p_allocated == TRUE)
	{
	    QEF_PARAM *qef_param;
	    qef_param = (QEF_PARAM *) ( (char *)
	    cquery->cur_qparam - sizeof(SC0M_OBJECT));
	    status = sc0m_deallocate(0, (PTR *)&qef_param);
	    if (status)
	    {
		sc0e_0_put(status, 0);
		sc0e_0_uput(status, 0 );
		scd_note(E_DB_SEVERE, DB_SCF_ID);
	    }
	}
	cquery->cur_p_allocated = FALSE;
	/* NOTE that this qprmcount=0 allows the buffer to be freed if
	** there were more than 15 parameters in used, since it marks that
	** we are now ready for another set of input from the client.
	** See reference to bug 116600 below.
	*/
	cquery->cur_qprmcount = 0;
	cquery->cur_qparam = 0;
	if (sscb->sscb_interrupt == 0)
	{
	    if (sscb->sscb_force_abort)
    	    {
		if (was_xact)
		{
		    sscb->sscb_force_abort = SCS_FACOMPLETE;
		    qry_status |= GCA_FAIL_MASK;
		    if (cscb->cscb_batch_count > 0)
			cscb->cscb_eog_error = TRUE;
		    /*
		    ** XA error codes must be set for GCA_XA_* messages.
		    ** Return RBOTHER XA error code for force-abort.
		    */
		    if (qmode == PSQ_GCA_XA_START || qmode == PSQ_GCA_XA_END ||
			qmode == PSQ_GCA_XA_PREPARE || qmode == PSQ_GCA_XA_COMMIT ||
			qmode == PSQ_GCA_XA_ROLLBACK)
		    {
			i4	*XAerr_code = &cscb->cscb_rdata->gca_errd5;

			*XAerr_code = XA_RBOTHER;

			/* The API expects GCA_REFUSE on any XA error */
			sscb->sscb_rsptype = GCA_REFUSE;

			/* Notify the API of the XAerr_code */
			qry_status = (GCA_FAIL_MASK | GCA_XA_ERROR_MASK);
		    }
		}
		else
		{
		    /*
		    ** In this case, the force abort condition was cleared up
		    ** before we had to do anything real.  Ignore the psuedo-
		    ** error.
		    */
		    qry_status = 0;
		    sscb->sscb_force_abort = 0;
		}
	    }
	    qry_status |= cquery->cur_result;
	    cquery->cur_result = 0;
	    if ((sscb->sscb_rsptype == GCA_RETPROC)
		    && (!(qry_status & GCA_FAIL_MASK)))
	    {
		i4 len;
		GCA_RP_DATA	*rpdata;
		
		if (cscb->cscb_in_group)
		{
		    /*
		    ** when in batch mode we will queue up the
		    ** messages on cscb_mnext until the batch is done, so we have
		    ** to allocate memory for each one.
		    ** This is here because of the non-async nature of
		    ** GCA_SEND and the way in which the DAS server works. The DAS
		    ** server might continue to send messages for a batch without
		    ** reading the receive queue. If the DBMS sends any
		    ** responses before DAS is ready to read them, we may end up
		    ** in a send/receive deadlock where the DBMS has sent a
		    ** response and is waiting for the DAS server to read it, and
		    ** the DAS server has sent more queries for the batch and is 
		    ** waiting for the DBMS to read them.
		    ** Saving up the responses is not the ideal solution since it
		    ** imposes on the DBMS processing and continues to eat through
		    ** the session's SCF memory while the batch is processing, but
		    ** since GCA has no way to queue the send request and 
		    ** asynchronously return, we have no choice. If the user's batch
		    ** is too big, we may exhaust the SCF memory pool, in which case
		    ** the batch will be terminated and the user will receive an
		    ** error. If we ever code an async GCA_SEND, we should change this 
		    ** processing.
		    */
		    status = sc0m_allocate(SCU_MZERO_MASK,
			    sizeof(SCC_GCMSG) + sizeof(GCA_RP_DATA),
			    DB_SCF_ID,
			    (PTR)SCS_MEM,
			    SCG_TAG,
			    (PTR *)&message);
		    if (status != E_DB_OK)
		    {
			cscb->cscb_eog_error = TRUE;
			sc0e_0_put(status, 0);
			sc0e_0_uput(status, 0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			qry_status |= GCA_FAIL_MASK;
			scd_note(E_DB_SEVERE, DB_SCF_ID);
		    }
		    message->scg_marea = (PTR)message + sizeof(SCC_GCMSG);
		    /* mark for deallocation after message send */
		    message->scg_mask = SCG_QDONE_DEALLOC_MASK;
		    rpdata = (GCA_RP_DATA *)message->scg_marea;
		}
		else
		{
		    message = &cscb->cscb_rpmmsg;
		    message->scg_mask = SCG_NODEALLOC_MASK;
		    rpdata = cscb->cscb_rpdata;

		}

		message->scg_mtype = sscb->sscb_rsptype;
		message->scg_next = (SCC_GCMSG *) &cscb->cscb_mnext;
		message->scg_prev = cscb->cscb_mnext.scg_prev;
		cscb->cscb_mnext.scg_prev->scg_next = message;
		cscb->cscb_mnext.scg_prev = message;
		message->scg_msize = sizeof(GCA_RP_DATA);
		rpdata->gca_retstat = qe_ccb->qef_dbp_status;
		

		/*
		** The procedure name has already been normalized and case
		** translated.  This was performed by the dbms when the
		** procedure was defined.
		** DON'T assume GCA_MAXNAME > DB_CURSOR_MAXNAME
		**
		** Don't send the procedure object ID if the name is too
		** large to fit in GCA_ID.
		*/
		for( len = sizeof(proc_qid.db_cur_name); len > 0; len-- )
		    if ( proc_qid.db_cur_name[len-1] != 0  &&
		         proc_qid.db_cur_name[len-1] != ' ' )  break;

		if ( len >
		     sizeof(rpdata->gca_id_proc.gca_name) )
		{
		    rpdata->gca_id_proc.gca_index[0] = 0;
		    rpdata->gca_id_proc.gca_index[1] = 0;

		    MEfill(
			sizeof(rpdata->gca_id_proc.gca_name),
			' ',
			(PTR)rpdata->gca_id_proc.gca_name );
		}
		else
		{
		    rpdata->gca_id_proc.gca_index[0] =
			proc_qid.db_cursor_id[0];
		    rpdata->gca_id_proc.gca_index[1] =
			proc_qid.db_cursor_id[1];

		    MEmove(sizeof(proc_qid.db_cur_name),
			(PTR)&proc_qid.db_cur_name, ' ',
			sizeof(rpdata->gca_id_proc.gca_name),
			(PTR)&rpdata->gca_id_proc.gca_name);
		}

		if ( tdesc != NULL && tdata != NULL )
		{
		    tdesc->scg_next = tdata;
		    tdesc->scg_prev = cscb->cscb_mnext.scg_prev;
		    tdata->scg_next = (SCC_GCMSG *)&cscb->cscb_mnext;
		    tdata->scg_prev = tdesc;
		    cscb->cscb_mnext.scg_prev->scg_next = tdesc;
		    cscb->cscb_mnext.scg_prev = tdata;
		}
	    }
	    if ((sscb->sscb_rsptype != GCA_RETPROC)
		|| (qry_status & GCA_FAIL_MASK)
		|| ((cscb->cscb_version >= GCA_PROTOCOL_LEVEL_60)
		    && !ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_NOBYREF,
				       NULL,NULL)))
	    {
		GCA_RE_DATA	*rdata;
		
		if (cscb->cscb_in_group)
		{
		    /*
		    ** when in batch mode we will queue up the
		    ** messages on cscb_mnext until the batch is done, so we have
		    ** to allocate memory for each one.
		    ** This is here because of the non-async nature of
		    ** GCA_SEND and the way in which the DAS server works. The DAS
		    ** server might continue to send messages for a batch without
		    ** reading the receive queue. If the DBMS sends any
		    ** responses before DAS is ready to read them, we may end up
		    ** in a send/receive deadlock where the DBMS has sent a
		    ** response and is waiting for the DAS server to read it, and
		    ** the DAS server has sent more queries for the batch and is 
		    ** waiting for the DBMS to read them.
		    ** Saving up the responses is not the ideal solution since it
		    ** imposes on the DBMS processing and continues to eat through
		    ** the session's SCF memory while the batch is processing, but
		    ** since GCA has no way to queue the send request and 
		    ** asynchronously return, we have no choice. If the user's batch
		    ** is too big, we may exhaust the SCF memory pool, in which case
		    ** the batch will be terminated and the user will receive an
		    ** error. If we ever code an async GCA_SEND, we should change this 
		    ** processing.
		    */
		    status = sc0m_allocate(SCU_MZERO_MASK,
			    sizeof(SCC_GCMSG) + sizeof(GCA_RE_DATA),
			    DB_SCF_ID,
			    (PTR)SCS_MEM,
			    SCG_TAG,
			    (PTR *)&message);
		    if (status != E_DB_OK)
		    {
			cscb->cscb_eog_error = TRUE;
			sc0e_0_put(status, 0);
			sc0e_0_uput(status, 0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			sscb->sscb_state = SCS_INPUT;
			*next_op = CS_EXCHANGE;
			qry_status |= GCA_FAIL_MASK;
			scd_note(E_DB_SEVERE, DB_SCF_ID);
		    }
		    message->scg_marea = (PTR)message + sizeof(SCC_GCMSG);
		    /* mark for deallocation after message send */
		    message->scg_mask = SCG_QDONE_DEALLOC_MASK;
		    rdata = (GCA_RE_DATA *)message->scg_marea;
		}
		else
		{
		    message = &cscb->cscb_rspmsg;
		    message->scg_mask = 0;
		    rdata = cscb->cscb_rdata;
		}

		message->scg_mtype = sscb->sscb_rsptype;
		if (message->scg_mtype == GCA_RETPROC)
		    message->scg_mtype = GCA_RESPONSE;
		message->scg_next = (SCC_GCMSG *) &cscb->cscb_mnext;
		message->scg_prev = cscb->cscb_mnext.scg_prev;
		cscb->cscb_mnext.scg_prev->scg_next = message;
		cscb->cscb_mnext.scg_prev = message;
		message->scg_mask |= SCG_NODEALLOC_MASK;
		if ((message->scg_mtype == GCA_DONE) &&
		    (qry_status & GCA_FAIL_MASK))
		{
		    sscb->sscb_rsptype = GCA_REFUSE;
		}
		if (message->scg_mtype == GCA_ACCEPT)
		{
		    message->scg_msize = 0;
		}
		else
		{
		    message->scg_msize = sizeof(GCA_RE_DATA);
		}
		rdata->gca_rqstatus = qry_status;
		rdata->gca_rowcount =
		    ((qry_status & GCA_FAIL_MASK)
		     ? -1
		     : cquery->cur_row_count);
		cquery->cur_row_count = -1;
		rdata->gca_rhistamp =
		    sscb->sscb_comstamp.db_tab_high_time;
		rdata->gca_rlostamp =
		    sscb->sscb_comstamp.db_tab_low_time;

		/* Stuff scrollable cursor info in GCA. qe_ccb may have
		** been cleared by now, in which case the fields should
		** be copied into cquery right after the QEQ_QUERY call,
		** then copied to the GCA from there. */
		rdata->gca_errd0 = rowstat;
		rdata->gca_errd1 = curspos;

		/*
		** XA support - if xa commit/rollback, send
		** pid to front end in gca response.  Overload
		** gca_errd5, which is mapped into sqlerrd(5) field
		** of sqlca.
		*/
/*
		if ((qmode == PSQ_XA_COMMIT) ||
		    (qmode == PSQ_XA_ROLLBACK))
		{
		    status = CVal(xa_pid,
				&(cscb->cscb_rdata->gca_errd5));
		}
*/
                if ((rdata->gca_rowcount == 1) &&
                    (cquery->cur_val_logkey & (QEF_TABKEY | QEF_OBJKEY)))
                {
                    /* if a system maintained logical key has been
		     ** assigned, then pass it up through the system,
		     ** to be returned to the client.
		     */
                    if (cquery->cur_val_logkey & QEF_TABKEY)
                        rdata->gca_rqstatus |= GCA_TABKEY_MASK;

                    if (cquery->cur_val_logkey & QEF_OBJKEY)
                        rdata->gca_rqstatus |= GCA_OBJKEY_MASK;


                    MEcopy( ((PTR) &cquery->cur_logkey),
			       sizeof(cquery->cur_logkey),
			       ((PTR) rdata->gca_logkey));
                }
	    }
	    sscb->sscb_qmode = 0;
	    cquery->cur_result = 0;
	    if ((sscb->sscb_force_abort)
		&& (sscb->sscb_state == SCS_INPUT)
		&& (*next_op != CS_EXCHANGE))
	    {
		*next_op = CS_INPUT;
	    }
	    else if (cscb->cscb_in_group &&
		    !cscb->cscb_gci.gca_end_of_group &&
		    qry_status != GCA_FAIL_MASK)
	    {
		/*
		** if we are in a batch, then
		** we can't return the result until the batch is done.
		** The messages are already queued, so we'll just
		** keep grabbing the next input until the batch is done
		*/
		*next_op = CS_INPUT;
	    }
	    else
	    {
		*next_op = CS_EXCHANGE;
		sscb->sscb_flags &= ~SCS_INSERT_OPTIM;
	    }
	    if (QEF_X_MACRO(sscb->sscb_qescb) == 0)
	    {
		scb->cs_scb.cs_mask |= CS_NOXACT_MASK;
		cscb->cscb_rdata->gca_rqstatus |= GCA_LOG_TERM_MASK;
	    }
	    sscb->sscb_state = SCS_INPUT;
	}
	else			/* interrupted */
	{
	    /* Then answer the interrupt request with a cancel block,
	       (unless we dropped the connection). */

	    CSintr_ack();

            /* Bug 106708. If doing a RETRIVE, end the transaction */
            if (qmode == PSQ_RETRIEVE)
            {
               sscb->sscb_state = SCS_ENDRET;

               if (cquery->cur_qe_cb)
                  qe_ccb = (QEF_RCB *) cquery->cur_qe_cb;
               else
                  qe_ccb = sscb->sscb_qeccb;

               /*
               ** If the retrieve is still ongoing w.r.t. QEF, then
               ** tell qef to terminate the query, and deallocate any
               ** space we may have reserved for it.  Otherwise,
               ** there is nothing to do, and we just ``act finished.''
               */

               if (cquery->cur_amem != NULL)
               {
                  status = qef_call(QEQ_ENDRET, qe_ccb);
                  if (DB_FAILURE_MACRO(status))
                  {
                     /* This error returned if query has been completed */
                     if (qe_ccb->error.err_code !=
                                    E_QE0008_CURSOR_NOT_OPENED)
                     {
                        qry_status = GCA_FAIL_MASK;
                         scd_note(status, DB_QEF_ID);
                         scs_qef_error(status,
                              qe_ccb->error.err_code,
                              E_SC0210_SCS_SQNCR_ERROR,
                              scb);
                     }
                  }
                  if (cquery->cur_amem != (PTR)&qef_data)
                  {
                      sc0m_deallocate(0, &cquery->cur_amem);
                      cquery->cur_amem = NULL;
                      cquery->cur_qef_data = NULL;
                  }
                  else
                  {
                      cquery->cur_amem = (PTR) NULL;
                  }
               }
            }

	    if( (sscb->sscb_flags & SCS_DROPPED_GCA) == 0 )
	    {
		scc_iprime(scb);
# ifdef OS_THREADS_USED
	    /* check for error on expedited channel for NT (and w/OS threads) */
        if( CLERROR( cscb->cscb_gce.gca_status ) )
        {
            /* Something is wrong at the CL level, possibly
               our old friend E_CLFE07_BS_READ_ERR.  It means the line
               is dead, so give up now and try not to log any more
               errors */
            sscb->sscb_flags |= SCS_DROPPED_GCA;
            (void) scs_disassoc( cscb->cscb_assoc_id );
        }
	    else
# endif /* OS_THREADS_USED */
		if (cscb->cscb_gci.gca_status == E_GC0000_OK)
		{
		    /*
		     ** Then the item was read before the interrupt,
		     ** and we are done with it.
		     */
		    cscb->cscb_gci.gca_status = E_SC1000_PROCESSED;
		}
		else if( CLERROR( cscb->cscb_gci.gca_status ) )
		{
		    /* Something is wrong at the CL level, possibly
		       our old friend E_CLFE07_BS_READ_ERR.  It means the line
		       is dead, so give up now and try not to log any more
		       errors */
		    sscb->sscb_flags |= SCS_DROPPED_GCA;
		    (void) scs_disassoc( cscb->cscb_assoc_id );
		}
	    }
	    sscb->sscb_force_abort = 0;
	    sscb->sscb_interrupt = 0;

	    if (sscb->sscb_flags & SCS_DROPPED_GCA)
	    {
		sscb->sscb_state = SCS_TERMINATE;
	    }
	    else if (sscb->sscb_qmode == PSQ_DIRCON)
	    {
		sscb->sscb_state = SCS_DCONNECT;
	    }
	    else
	    {
	        sscb->sscb_state = SCS_INPUT;
	        sscb->sscb_qmode = 0;
	    }

	    sscb->sscb_dmm = 0;	/* clean up partial data */
	    cquery->cur_result = 0;

	    if( (sscb->sscb_flags & SCS_DROPPED_GCA) == 0 )
	    {
		status = sc0m_allocate(0,
				sizeof(SCC_GCMSG) + sizeof(GCA_AK_DATA),
				DB_SCF_ID,
				(PTR)SCS_MEM,
				SCG_TAG,
				(PTR *) &message);
		if (status)
		{
		    sc0e_0_put(status, 0);
		    sc0e_0_uput(status, 0);
		    scd_note(E_DB_SEVERE, DB_SCF_ID);
		}
		scc_dispose(scb);
		message->scg_next = (SCC_GCMSG *) &cscb->cscb_mnext;
		message->scg_prev = cscb->cscb_mnext.scg_prev;
		cscb->cscb_mnext.scg_prev->scg_next = message;
		cscb->cscb_mnext.scg_prev = message;
		message->scg_mask = 0;
		message->scg_mtype = GCA_IACK;
		message->scg_msize = sizeof(GCA_AK_DATA);
		message->scg_marea = ((char *) message + sizeof(SCC_GCMSG));

		/* Avoid purging alerts (1 may already be lost) */
		sscb->sscb_aflags |= SCS_AFPURGE;

		if (cscb->cscb_gci.gca_status == E_GCFFFF_IN_PROCESS)
		{
		    CScancelled(0);
		}
		*next_op = CS_EXCHANGE;
	    }
	}
	cquery->cur_qe_cb = NULL;

        /* If the query is against a table procedure, and has failed,
         * reset cur_amem so next run of the same query won't
         * mistake cur_amem as a valid QEU_CB based on which to
         * retrieve QEF_CB.
         */
         if (is_tproc_load_qp && qry_status & GCA_FAIL_MASK)
            cquery->cur_amem = NULL;
    }
    if ((sscb->sscb_phandle || sscb->sscb_proot) &&
	(!rowproc || rowprocdone))
    {
	if (sscb->sscb_phandle)
	{
	    qs_ccb = sscb->sscb_qsccb;
	    qs_ccb->qsf_lk_id = sscb->sscb_plk_id;
	    qs_ccb->qsf_obj_id.qso_handle = sscb->sscb_phandle;
	    status = qsf_call(QSO_DESTROY, qs_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
					    E_SC0210_SCS_SQNCR_ERROR);
	    }
	    qs_ccb->qsf_lk_id = sscb->sscb_p1lk_id;
	    if (qs_ccb->qsf_obj_id.qso_handle = sscb->sscb_p1handle)
	    {
		status = qsf_call(QSO_DESTROY, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
						E_SC0210_SCS_SQNCR_ERROR);
		}
		sscb->sscb_p1handle = 0;
	    }
	    sscb->sscb_phandle = 0;
	    sscb->sscb_proot = 0;
	}
	else
	{
	    sc0m_deallocate(0, &sscb->sscb_proot);
	}
	sscb->sscb_dbparam = 0;
    }
    if (rowprocdone && NULL != cquery->cur_amem )
    {
	sc0m_deallocate(0, &cquery->cur_amem);
	cquery->cur_qef_data = NULL;
    }
    if (repeat_redef)
    {
	sscb->sscb_plk_id = sscb->sscb_tlk_id;
	sscb->sscb_phandle = sscb->sscb_thandle;
	sscb->sscb_proot = sscb->sscb_troot;
	sscb->sscb_p1handle = psf_qp_handle;
	sscb->sscb_p1lk_id = psf_lkid;
	sscb->sscb_thandle = 0;
	sscb->sscb_troot = 0;
    }
    if ((sscb->sscb_thandle || sscb->sscb_troot) &&
	(!rowproc || rowprocdone))
    {
	if (sscb->sscb_thandle)
	{
	    qs_ccb = sscb->sscb_qsccb;
	    qs_ccb->qsf_lk_id = sscb->sscb_tlk_id;
	    qs_ccb->qsf_obj_id.qso_handle = sscb->sscb_thandle;
	    status = qsf_call(QSO_DESTROY, qs_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
					    E_SC0210_SCS_SQNCR_ERROR);
	    }
	    sscb->sscb_thandle = 0;
	    sscb->sscb_troot = 0;
	}
	else
	{
	    /* Bug fix 116600
	    ** qprmcount set to >= SCS_SQPARAMS-1 means that the
	    ** troot has been used to house them.  We only want to free this
	    ** memory (references to which are stored in the QEF block) at
	    ** the same cycle as when we free the QEF block as well, which is
	    ** when we've finished with this query and are going to return
	    ** for more input (SCS_INPUT or interrupts, checked above at the
	    ** other reference to bug 116600).  That check sets qprmcount back
	    ** to 0, which means we can free this block
	    */
	    if (sscb->sscb_cquery.cur_qprmcount < (SCS_SQPARAMS - 1))
	    {
		sc0m_deallocate(0, &sscb->sscb_troot);
	    }
	}
    }

    /* SC930 - log end of query */
    if ((sscb->sscb_state == SCS_INPUT) &&
        (ult_always_trace() & SC930_TRACE))
    {

          SCC_CSCB	*cscb;		/* Convenience: scb->scb_cscb */

          void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);

          cscb = &scb->scb_cscb;

          if (f)
          {
	      char tmp[45],fac_str[5];
              u_i4 fac_code,just_err,i;
	      GCA_RE_DATA   *rdata;
	      SCC_GCMSG     *message;

	      /* batch executed queries don't use the GCA_RE_DATA that's
	       * part of the CSCB itself but create a linked list of them
	       * instead.
	       *
	       * so in the case of batch queries find the latest in order
	       * to get the correct rowcount */
	      
	      if (cscb->cscb_in_group)
	      {
		 message=scb->scb_cscb.cscb_mnext.scg_next;
		 while (message != (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext) 
		 {
		    rdata=(GCA_RE_DATA *)message->scg_marea;
		    message=message->scg_next;
		 }

	      }
	      else
		 rdata= cscb->cscb_rdata;

	      if (scb->cs_scb.cs_sc930_err == 0)
	      {
 		  STprintf(tmp,"%d:",
		  	  rdata->gca_rowcount);
	      }
	      else
	      {
 	        fac_code=(scb->cs_scb.cs_sc930_err >> 16) & 0x7fff;
 	        just_err=scb->cs_scb.cs_sc930_err & 0xffff; 
                
		/* if for any reason we can't find the facilty code then
		 * output it as hex */

 	        STprintf(fac_str,"%04X",fac_code);
 
 		for (i=0; i < NUMFAC; ++i)
 	  	  if (Factab[i].num== fac_code)
 		  {
 		    STprintf(fac_str,"E_%2s",Factab[i].fac);
 		    break;
 		  }
 
 		STprintf(tmp,"%d:%4s%04X",
			rdata->gca_rowcount,
		        fac_str,just_err);
	      }
 	      ult_print_tracefile(f,SC930_LTYPE_ENDQRY,tmp);
 	      ult_close_tracefile(f);
 	    }
	    scb->cs_scb.cs_sc930_err=0;
    }

    sscb->sscb_cfac = DB_CLF_ID;
    return(ret_val <= E_DB_ERROR ? E_DB_OK : ret_val);
}

/*
** {
** Name: print_sc930_info	- Output sc930 tracing
**
** Description:
**	Separate this segment so the large strings don't end up on the stack.
**
** History:
**	23-Jul-2009 (kibro01)
**	    Written.
**	24-Jul-2009 (kibro01) b122393
**	    Add generic message so PARM can use this as well as PARMEXEC
**	    Also ensure alignment of the gdv structure.
**	11-Aug-2009 (kibro01) b122393
**	    Ensure that newly byte-aligned values are used straight.
**	25-Aug-2009 (kschendel) b122804
**	    valuetomystr is prototyped with ptr, not adf_cb *.  Fix here.
**      30-Dec-2009 (maspa05) b123093
**          Switch to passing DB_DATA_VALUE rather than GCA_DATA_VALUE. This
**          resolves some byte-aligned issues as structure is already aligned by
**          caller
**          b123056 - moved call to adu_valuetomystr out of STprintf as this
**          causes SIGSEGV on some platforms
**      19-Oct-2010 (maspa05) b124551
**          Use adu_sc930prtdataval to output parameter values to SC930 trace
**
*/
static void
print_sc930_info(SCD_SCB *scb,
		DB_DATA_VALUE *dbdv,
		GCA_P_PARAM *gpm,
		i4 pcount,
		i2 msg_name)
{
    void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
    if (f)
    {
	i4 namelen;
	char *parm_name=NULL,tmp[DB_PARM_MAXNAME + 1];

	if (gpm)
	{
	    parm_name=&tmp[0];
	    MEcopy((PTR)&gpm->gca_parname.
	        gca_l_name, sizeof(namelen),(PTR)&namelen);
	    STprintf(parm_name,"%*s",
		namelen, gpm->gca_parname.gca_name);
	}

	adu_sc930prtdataval(msg_name,parm_name,pcount,dbdv, 
			    scb->scb_sscb.sscb_adscb,f);
	ult_close_tracefile(f);
    }
}

/*
** Use a global function pointer and indirect through it to make absolutely sure
** that no compiler can inline this function.
*/
void (*print_sc930_ptr)(SCD_SCB*,DB_DATA_VALUE*,GCA_P_PARAM*,i4,i2) =
	print_sc930_info;

/*
** {
** Name: scs_input	- Process input from the user
**
** Description:
**      This routine processes input from the user program.  If the input is 
**      textual in nature (i.e. destined for PSF) then an object is opened in 
**      QSF to handle the new object.  The object is stored in QSF.  If the 
**	input is not textual in nature, then it is interpreted and the 
**	appropriate state is set in the scb.  Control is then returned to the 
**	main sequencer routine for continued processing.
**
** Inputs:
**      scb                             Session control block for which the
**					input is destined
**
** Outputs:
**      (various state information is set up in the scb)
**	Returns:
**	    DB_STATUS
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      10-Jul-1986 (fred)
**          Created on Jupiter.
**      10-Mar-1989 (ac)
**          Added 2PC support.
**	24-apr-89 (neil)
**	    Modified cursor processing to support rules.  Cursor DELETE and
**	    UPDATE now both go through PSF.  Also fixed a problem where
**	    "printqry" was processing data from the previous request.
**	21-mar-90 (andre)
**	    Added support for a new GCA message - GCA_01DELETE.
**	10-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    05-apr-1990 (fred)
**		Added monitoring of queries to the system.
**	    01-may-1990 (neil)
**		Cursor Performance I: Modify GCA_FETCH processing.
**	    12-sep-1990 (sandyh)
**		Zero scb->scb_sscb.sscb_dmm on GCA_ATTENTION message so done
**		check does not cause premature message end and access violate
**		crashing the server. bug 32581.
**	01-16-91 (andre)
**	    Instead of introducing a new message - GCA_01DELETE - we will (in
**	    the future) define a new protocol level associated with a new
**	    structure for GCA_DELETE.
**	04-feb-91 (neil)
**	    Alerters: Cleanup GCA states that are acceptable states upon
**	    session arrival back to the sequencer.  Set the session alert
**	    flags.
**      20-dec-90 (rachael)
**          Fix bug 35000. Passing in null parameter in db procedure
**          caused access violation in scs_input() in case GCA_INVPROC.
**          Gdv was not being checked and was pointing to garbage.
**	16-apr-92 (rog)
**	    Fix bug 42392: When handling more than SCS_SQPARAMS repeat query
**	    or DB proc parameters, we weren't allocating enough memory to 
**	    store the parameters because we assumed that the param value was
**	    at least four bytes -- more below.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              19-Jan-1989 (alexh)
**                  Force scs_fetch_data calls to move data when serving for 
**		    STAR. See statements referencing symbol SCS_STAR.
**              31-mar-1990 (carl)
**                  Incorporated alignment fixes from the 62ug porting line
**          End of STAR comments.
**	05-oct-1992 (fred)
**	    When deallocating memory used in blob storage, get the pointer from
**	    the right place.
**	29-oct-92 (andre)
**	    add support for GCA1_DELETE (and get rid of an ancient #ifdef) 
**      03-dec-1992 (nandak, mani)
**          added support for GCA_XA_SECURE message.
**	18-jan-1993 (rog)
**	    Fixed 46715: we weren't calculating the correct amount of memory
**	    to allocate when storing more than 15 DB proc parameters; similar
**	    to 42392.
**	20-apr-93 (jhahn)
**	    Added code to check to see if transaction statements should be
**	    allowed.
**	24-may-93 (andre)
**	    GCA1_DELETE processing didn't take into consideration the fact that
**	    gca_owner_name and gca_table_name both contain variable-length
**	    arrays.
**	25-may-93 (fpang)
**	    Added code to support the GCA1_INVPROC message, which is a new
**	    GCA Execute Procedure message that contains an owner field.
**	09-jun-93 (andre)
**	    someone has misintegrated code dealing with GCA1_DELETE.  As a
**	    result, we were failing to account for gca_l_name field in
**	    determining the beginning of the table name
**	2-Jul-1993 (daveb)
**	    prototyped.
**	7-Jul-1993 (daveb)
**	    Take fpang's code fragment for GCA_INVPROC: and GCA1_INVPROC:
**	    that fixed type incorrectness and handled byte alignment
**	    properly.  He should be the one really submitting it.
**	22-jul-1993 (fpang)
**	    Fixed byte alignment problems related to GCA1_INVPROC, and
**	    owner.procedure. 
**      16-Aug-1993 (fred)
**          Note that errors have been found and continue to suck up 
**          blocks until we find end-of-data.  Then set the SCS_IERROR 
**          state and return.
**	4-Aug-1993 (daveb)
**	    Fixed parens around pointer arithmetic using char * and PTR.
**
**	13-Oct-1993 (larrym)
**	    Fixed bug 55872 with GCA1_INVPROC:  We were casting msg to a 
**	    GCA_IP_DATA when it should have been GCA1_IP_DATA.  
**      23-sep-1993 (iyer)
**          The message passed for GCA_XA_SECURE has changed, it is now
**          an extended struct DB_XA_EXTD_DIS_TRAN_ID which includes the
**          previous message struct DB_XA_DIS_TRAN_ID. In addition it has
**          two members: branch_seqnum and branch_flag. 
**	08-nov-93 (rblumer)
**	    Fix typo that could cause an uninitialized dl1_data to be used.
**	07-jan-94 (rblumer)
**	    Several calls to cui_idxlate were not recording the status returned
**	    (but were trying to look at it anyways); will now write to the
**	    error log any error returned by cui_idxlate call (especially since
**	    the error sent to the user doesn't seem to get through since the
**	    session is almost immediately terminated).       B58511
**	    Recording the status showed some more bugs:
**	       -in procedure name and procedure owner name translation, set l_id
**	        to actual length of name, otherwise get 'invalid character'
**	        error from cui_idxlate when it hits spaces at end of the name. 
**	       -in GCA[1]_DELETE table name and owner name xlation,
**	        don't translate if have a zero-length name.
**	31-jan-94 (mikem)
**	    bug #58501
**	    Replace the TRformat call with in-line ME code to streamline
**	    the dbp path through the code.  This is part of the changes
**	    being made to 6.5 to try and get cpu performance of 6.5 
**          executing short dbp's to perform as well or better as 6.4. 
**	22-feb-94 (rblumer)
**	    change dbproc name translation to use cui_f_idxlate when possible;
**	    this function is much faster than cui_idxlate.
**      20-Apr-1994 (fred)
**          Allow outer layers to continue (i.e. not terminate the
**          session) when this routine sets the EAT_INPUT flag.  By
**          setting this flag, the system has indicated a willingness
**          to get over the current problem.
**	25-apr-1994 (rog)
**	    If printqry is set for repeat queries or db procs, we try to print
**	    the query parameter before it has been aligned, which sometimes
**	    causes bus errors.  (Bug 57447)
**	9-may-94 (robf)
**          Move namelen outside BYTE_ALIGN declaration since its used for
**          BYTE_ALIGN and non-BYTE_ALIGN paths.
**	19-jul-94 (robf)
**          Printqry parameters could be printed twice, looks like an
**	    earlier integration problem. Updated code to remove second
**	    call.
**	24-apr-95 (lewda02)
**	    Do NOT use QSF for DMF-API queries.
**	14-sep-95 (shust01/thaju02)
**	    change raat from language_id of DB_NDMF to message_type of
**	    GCA_NDMF.  This is because when sending more than 1 GCA buffers
**	    worth of data, the language id of 2nd (and beyond) buffer is
**	    cleared.  Also added calls to scs_raat_load() which adds raat
**	    GCA buffers to link list.
**	16-Nov-95 (gordy)
**	    Cursor pre-fetch now supported through GCA1_FETCH message at
**	    GCA_PROTOCOL_LEVEL_62.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**   4-sep-96 (schte01/kinpa04)
**       To avoid alignment errors (GCA_XA_SECURE), add alignment test
**       of gca_data_area+GCA_DESC_SIZE. If not aligned, do MEcopy to
**       db_xa_extd_dis_tran_id instead of structure assignment.
**	12-sep-96 (kch)
**	    Add an additional check for the first RAAT GCA buffer
**	    (sscb_thandle == 0). Previously, subsequent non-RAAT GCA buffers
**	    could have language_id set to DB_NDMF, causing scs_raat_load() to
**	    be erroneously called. This change fixes bugs 78299 and 78345.
**	27-nov-96 (kch)
**	    Declare l_name for both byte aligned and non byte aligned
**	    platforms.
**	11-feb-1997(angusm)
**	    Add conditional query trace debug
**	6-may-97 (inkdo01)
**	    Belatedly added support for embedded invocation of procedures
**	    with global temptab parms. Due to the horrors of SCF, this requires
**	    the reconstitution of the orig exec syntax, and simulation of
**	    "execute immediate".
**	01-jun-97 (pchang)
**	    Fixed incorrect capture of gca_proc_mask from GCA1_IP_DATA type
**	    messages when processing GCA1_INVPROC requests.
**	4-jun-1997 (nanpr01 for devjo01)
**	    Pass *p instead of p. Causes random segv allover.
**	26-jun-97 (hayke02)
**	    Move code which aligns dbdv.db_data (introduced in the fix for
**	    bug 57447) to before the second call to adu_2prvalue(). This
**	    change fixes bug 83430.
**      15-sep-97 (stial01)
**          scs_input() GCA_INVOKE, GCA_INVPROC: If input data comes in one
**          GCA buffer, check for blob parameters in the input, which must be
**          read with scs_fetch_data and can't be processed 'in-place' (B85214)
**      18-Dec-1997 (stial01)
**          scs_input() Disallow blob input for GCA_DEFINE, GCA_INVOKE
**      17-jun-1998 (horda03)
**             Store the query text for the current query.
**	18-jan-1999 (nanpr01)
**	    Fix the parameter in sc0m_deallocate call.
**      30-jul-1999 (stial01)
**          scs_input() Init the first 8 bytes of query text memory
**      15-May-1998 (horda03)
**          Make a local copy of the GCA_STATUS, such that any
**          changes to it by an AST will not be considered until
**          the next time the function is invoked.
**      04-Jul-2000 (horda03)
**          Disable ASTs (on VMS) while accessing the GCA_STATUS variable.
**     04-dec-2001 (rigka01) bug#106069
**	    SIGBUG invalid address alignment in adu_2prvalue() called by 
**          scs_input() when dbdv.db_data is set to the non byte-aligned
**	    opy of the data although the code is following the ifdef
**	    BYTE_ALIGNED code path. 
**      23-apr-2002 (rigka01) bug#106069 correction 
**	    Cast gca_value to PTR rather than i4 when setting db_data
**	    Code won't compile on rs4_us5 without this correction 
**      15-May-2003 (inifa01) b110263 INGSRV 2270
**          When security auditing is enabled a SIGBUS occurs in adu_prvalarg()
**          followed by E_CL2514, E_GC220B, E_CLFE07 and E_GC2205 on the execution
**          of a database procedure with a parameter of type money.
**          Problem occurs on rmx.us5 when dbdv.db_data is set to non byte-aligned
**          copy of data.
**	16-Sep-2005 (schka24)
**	    Init query state when we see an execute procedure message.
**	    Shorten long references to "current query".
**	22-june-06 (dougi) SIR 116219
**	    Add support for GCA_IS_OUTPUT parameters.
**	26-Jun-2006 (jonj)
**	    Added XA integration support for new GCA messages:
**		GCA_XA_START
**		GCA_XA_END
**		GCA_XA_PREPARE
**		GCA_XA_COMMIT
**		GCA_XA_ROLLBACK
**	14-feb-2007 (dougi)
**	    Add support for GCA2_FETCH message for scrollable cursors.
**	30-apr-2007 (dougi)
**	    Tinkering with row count, anchor & offset of scrollable cursors.
**	9-may-2007 (dougi)
**	    Mini-tinker with row count for non-scrollable cursors.
**	2-mar-2009 (dougi) bug 121773
**	    Flag locator enabling in PSQ_CB (for cached dynamic logic).
**	11-march-2009 (dougi) b121773
**	    Dropped a tiny bit of previous change.
**	1-May-2009 (kibro01) b122024
**	    Fix file pointer leak from PARM SC930 trace.
**	16-Jul-2009 (kibro01) b122172
**	    Add in ability to retrieve timezone into date in adu_valuetomystr
**	24-Jul-2009 (kibro01) b122393
**	    Use print_sc930_info to save on stack size and allow DB_MAXSTRING.
**	11-Aug-2009 (kibro01) b122393
**	    Ensure we use byte-aligned values, as in security auditing, for
**	    BYTE_ALIGN platforms.
**	31-Aug-2009 (kiria01) b122544
**	    Output sc930 output for PARM before gdv either updated or the descriptor
**	    potentially trashed by and alignment move.
**	 1-Oct-09 (gordy)
**	    New GCA message GCA2_INVPROC with data object GCA2_IP_DATA.
**	November 2009 (stephenb)
**	    Batch processing; set fields correctly based on GCA_EOG/NOT_EOG
**      04-Jan-2010 (maspa05) b123093
**          Changed sc930_print_info to take a DB_DATA_VALUE and made sure
**          we pass an aligned value to it.
**      05-mar-2010 (joea)
**          Add case for DB_BOO_TYPE in scs_returntype_check.
**      19-Mar-2010 (maspa05) b123430
**          For PARMEXEC SC930 we were using the wrong value for the db_data
**          element of our DB_DATA_VALUE for the dbproc parameters.
**          Also needed to move SC930 (and printqry) output to after the point
**          where we've aligned the values
**      22-Mar-2010 (maspa05) b123430
**          The above fix broke the output for the non-Byte Aligned case. 
**          Moving the setting of the DB_DATA_VALUE to below the aligning code
**          caused the values we're using to be out of date. 
**          Split this code so we do the first part of DB_DATA_VALUE where we
**          did originally and the db_data part (the bit that really needs
**          the alignment) below the aligning code.
**	25-Mar-2010 (kschendel) SIR 123485
**	    Readability: replace scb->scb_sscb. with sscb->, likewise
**	    with scb_cscb.  Fix up some bad indents. Add comments.
*/
DB_STATUS
scs_input(SCD_SCB *scb,
	  i4  *next_state )

{
    DB_STATUS		ret_val = E_DB_OK;
    DB_STATUS		status;
    DB_STATUS		error;
    QSF_RCB             *qs_ccb;
    QEF_RCB		*qe_ccb;
    SCC_CSCB		*cscb;		/* Convenience: scb->scb_cscb */
    SCS_CURRENT		*cquery;	/* sscb_cquery inside scb */
    SCS_SSCB		*sscb;		/* Convenience: scb->scb_sscb */
    i4			mode;
    i4			qlang;
    i4			modifiers;
    i4			qry_size;
    i4			i;
    i4			print_qry;
    bool		done = TRUE;
    i4                  regular_query = 0;
    char		stbuf[SC0E_FMTSIZE]; /* last char for `\n' */
    bool                ad_peripheral;
    GCA_RV_PARMS	*rv;
#ifdef VMS
    STATUS              gca_status;

    /* Disable ASTs while accessing gca_status. This field could be updated
    ** while value being obtained.  This section is only applicable while
    ** Ingres is built on VMS with /NOMEMBER_ALIGNMENT.
    */

    int ast_enabled = sys$setast(0);

    gca_status = scb->scb_cscb.cscb_gci.gca_status;

    if (ast_enabled) sys$setast(1);
#else
    STATUS              gca_status = scb->scb_cscb.cscb_gci.gca_status;
#endif

#define	    GCA_DESC_SIZE   (sizeof(i4) + sizeof(i4) + sizeof(i4))

    sscb = &scb->scb_sscb;
    cscb = &scb->scb_cscb;
    cquery = &sscb->sscb_cquery;
    scb->cs_scb.cs_mask &= ~(CS_NOXACT_MASK);	/* put us artificially in a */
						/* transaction		    */
    *next_state = 0;
    if (gca_status != E_GC0000_OK)
    {
	if (gca_status == E_GC0027_RQST_PURGED ||
	    gca_status == E_GCFFFF_IN_PROCESS ||
	    gca_status == E_SC1000_PROCESSED)
	{
	    /* These are normal after an interrupt or alert processing */
	    *next_state = SCS_INPUT;
	    sscb->sscb_aflags &= ~SCS_AFINPUT; 	/* Nothing here */
	    return(E_DB_OK);
	}
	else if ((gca_status != E_GC0001_ASSOC_FAIL)
	    && (gca_status != E_GC0023_ASSOC_RLSED))
	{
	    sc0e_0_put(gca_status, &cscb->cscb_gci.gca_os_status);
	    sc0e_0_put(E_SC022F_BAD_GCA_READ, 0);
	}
	*next_state = SCS_TERMINATE;
	return(E_DB_ERROR);
    }	
    sscb->sscb_aflags |= SCS_AFINPUT;		/* Something here */
    cscb->cscb_gci.gca_status = E_SC1000_PROCESSED;
    if (sscb->sscb_interrupt == 0)
    {
	rv = &cscb->cscb_gci;
	if (cscb->cscb_gci.gca_end_of_data)
	{
	    if (cscb->cscb_eog_error)
	    {
		/* 
		** eog error is only set if we were in batch (group) processing
		** and got an error that should end group processing.
		** In this case we are ignoring the rest of the requests in
		** this group from the client.
		** Set next state to input, this way we'll just keep
		** eating the rest to the group messages without processing.
		** when we get the end of group flag, we'll re-set the eog error
		** and everything will revert to normal
		*/
		*next_state = SCS_INPUT;
		sscb->sscb_req_flags |= SCS_EAT_INPUT_MASK;
		cscb->cscb_batch_count = 0;
	    }
	    else if (!cscb->cscb_gci.gca_end_of_group)
	    {
		/* If Batch mode detected - note this and don't respond EOG */
		cscb->cscb_batch_count++;
		cscb->cscb_in_group = TRUE;
	    }
	}
    }
    else
    {
	rv = &cscb->cscb_gce;
    }
    mode = rv->gca_message_type;
    done = rv->gca_end_of_data;
    qry_size = rv->gca_d_length;
    print_qry = ult_check_macro(&sscb->sscb_trace, SCS_TPRINT_QRY, 0, 0);

    if (sscb->sscb_thandle == 0 && !cscb->cscb_eog_error)
	sscb->sscb_req_flags = 0;
    sscb->sscb_rsptype = GCA_RESPONSE;
    if ((sscb->sscb_req_flags & SCS_EAT_INPUT_MASK) == 0)
    {
	switch (mode)
	{
	case GCA_FETCH:
	case GCA1_FETCH:
	case GCA2_FETCH:
	case GCA_CLOSE:
	{
	    GCA_ID	    *id;

	    id = (GCA_ID *)rv->gca_data_area;
	    cquery->cur_qname.db_cursor_id[0] = id->gca_index[0];
	    cquery->cur_qname.db_cursor_id[1] = id->gca_index[1];

	    /*
	    ** The cursor name was sent by the FE as normalized and
	    ** translated.  This was performed by the dbms when the
	    ** cursor was opened.
	    ** DON'T assume GCA_MAXNAME > DB_CURSOR_MAXNAME
	    */
	    MEmove(sizeof(id->gca_name), (PTR)&id->gca_name, ' ',
		DB_CURSOR_MAXNAME, (PTR)&cquery->cur_qname.db_cur_name);

            if (print_qry)
	    {
		i4 length;

		length = cus_trmwhite(
		    (u_i4)DB_CURSOR_MAXNAME, cquery->cur_qname.db_cur_name);

                sc0e_trace(STprintf(stbuf,
		    "%s Cursor <%d.,%d.,%.*s> Statement\n",
                    mode == GCA_CLOSE ? "Close" : "Fetch",
                    cquery->cur_qname.db_cursor_id[0],
                    cquery->cur_qname.db_cursor_id[1],
                    length, cquery->cur_qname.db_cur_name));
            }
            TRformat(0, 0, sscb->sscb_ics.ics_qbuf,
                     sizeof(sscb->sscb_ics.ics_qbuf),
                     mode == GCA_CLOSE ? "Close %t" : "Fetch %t", 
		     DB_CURSOR_MAXNAME, cquery->cur_qname.db_cur_name);
            sscb->sscb_ics.ics_l_qbuf = 6 + DB_CURSOR_MAXNAME;
	    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	    {
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }

	    /* Pick up row count */
            if (mode == GCA_FETCH  ||  mode == GCA1_FETCH ||
			mode == GCA2_FETCH)
            {
		/* Establish defaults - single row NEXT. This corresponds
		** to GCA_FETCH with no pre-fetch. */
                sscb->sscb_cursor.curs_frows = 1;
                sscb->sscb_cursor.curs_anchor = GCA_ANCHOR_CURRENT; 
		sscb->sscb_cursor.curs_offset = 1;

                if ( mode == GCA1_FETCH )
                {
		    GCA1_FT_DATA *ft = (GCA1_FT_DATA *)rv->gca_data_area;

                    /* Row count follows id, and is aligned. */
                    sscb->sscb_cursor.curs_frows = ft->gca_rowcount;
		    if (sscb->sscb_cursor.curs_frows < 1)
			sscb->sscb_cursor.curs_frows = 1;
                }
		else if ( mode == GCA2_FETCH )
                {
		    GCA2_FT_DATA *ft = (GCA2_FT_DATA *)rv->gca_data_area;

                    /* Row count is separately stored in message. */
                    sscb->sscb_cursor.curs_frows = ft->gca_rowcount;
                    sscb->sscb_cursor.curs_anchor = ft->gca_anchor;
                    sscb->sscb_cursor.curs_offset = ft->gca_offset;
		    if (sscb->sscb_cursor.curs_frows < 1)
			sscb->sscb_cursor.curs_frows = 0;
                }
                else  if (CMdigit(&id->gca_name[DB_GW1_MAXNAME_32]))
                {
		    /* Last 1/2 of gca_name has row count encoded */
                    id->gca_name[GCA_MAXNAME-1] = EOS;
                    CVan(&id->gca_name[DB_GW1_MAXNAME_32],
                         &sscb->sscb_cursor.curs_frows);
		    if (sscb->sscb_cursor.curs_frows < 1)
			sscb->sscb_cursor.curs_frows = 1;
                }

                if (sscb->sscb_cursor.curs_frows > 1 && print_qry)
                {
                    sc0e_trace(STprintf(stbuf, " pre-fetch %d row(s)\n",
				sscb->sscb_cursor.curs_frows));
                }

                /* Initialize state for later processing */
                cquery->cur_state = CUR_FIRST_TIME;
                sscb->sscb_qmode = PSQ_RETCURS;
            }
            else        /* CLOSE */
            {
                sscb->sscb_qmode = PSQ_CLSCURS;
            }

	    if (ult_always_trace())
	    {
                void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
                if (f)
                {
                        char tmp[1000];
                        STprintf(tmp,"(ID=%d/%d):ROWCOUNT=%d",
                                cquery->cur_qname.db_cursor_id[0],
                                cquery->cur_qname.db_cursor_id[1],
				sscb->sscb_cursor.curs_frows);
                        ult_print_tracefile(f,
				(mode==GCA_CLOSE)?SC930_LTYPE_CLOSE:SC930_LTYPE_FETCH,
				tmp);
                        ult_close_tracefile(f);
                }
            }

            *next_state = SCS_PROCESS;
            break;
        }

	case GCA_DELETE:
	case GCA1_DELETE:
	{
	    GCA_ID	    *id;
	    GCA_DL_DATA	    *dldata;
	    GCA1_DL_DATA    *dl1_data;
	    u_i4	    l_id;
	    char	    tempstr[DB_MAXNAME];
	    u_i4	    templen;
	    DB_ERROR	    cui_err;

	    if (mode == GCA1_DELETE)
	    {
		/*
		** gca_table_name follows gca_owner_name which contains a
		** variable-length array of chars - as a result, we cannot
		** simply use the GCA1_DL_DATA structure definition to access
		** table name.
		*/
		i4	    tbl_name_len;
		char	    *tbl_name_p;
		char	    *gca_tab_name_p;

		dl1_data = (GCA1_DL_DATA *)rv->gca_data_area;
		id = &dl1_data->gca_cursor_id;

		/*
		** Copy the owner name (if any).
		** I am reusing a field in PSQ_CB, but this should cause no
		** conflicts
		*/
		/*
		** The owner name must be normalized and case translated!
		**
		MEmove(dl1_data->gca_owner_name.gca_l_name,
		    dl1_data->gca_owner_name.gca_name, ' ', sizeof(DB_OWN_NAME),
			&sscb->sscb_psccb->psq_als_owner);
		**
		*/
		l_id = dl1_data->gca_owner_name.gca_l_name;

		if (l_id != 0)
		{
		    templen = DB_OWN_MAXNAME;
		    status = cui_idxlate(
				   (u_char *)dl1_data->gca_owner_name.gca_name,
				   &l_id, (u_char *)tempstr, &templen,
				   sscb->sscb_ics.ics_dbxlate,
				   (u_i4 *)NULL, &cui_err);

		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_uput(cui_err.err_code, 0, 2,
				  sizeof(ERx("Owner name"))-1,
				  (PTR)ERx("Owner name"),
				  l_id, (PTR)dl1_data->gca_owner_name.gca_name,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			/* output to error log, too */
			sc0e_put(cui_err.err_code, 0, 2,
				 sizeof(ERx("Owner name"))-1,
				 (PTR)ERx("Owner name"),
				 l_id, (PTR)dl1_data->gca_owner_name.gca_name,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }	/* end if status == failed */
		}	/* end if l_id != 0) */
		else
		{
		    /* if l_id is 0, then setting templen to 0 will cause
		    ** MEmove to fill the owner name entirely with spaces
		    */
		    templen = 0;
		}

		(VOID)MEmove(templen, (PTR)tempstr, ' ',
			sizeof(DB_OWN_NAME),
			(PTR) &sscb->sscb_psccb->psq_als_owner);

		/*
		** find address of the GCA_NAME structure containing the
		** table name
		*/
		gca_tab_name_p = (char *) dl1_data +
		    sizeof(dl1_data->gca_cursor_id) +
		    sizeof(dl1_data->gca_owner_name.gca_l_name) +
		    dl1_data->gca_owner_name.gca_l_name;

#ifdef BYTE_ALIGN
		MEcopy(gca_tab_name_p, sizeof(tbl_name_len),
		    (PTR)&tbl_name_len);
#else
		tbl_name_len = *((i4 *) gca_tab_name_p);
#endif

		tbl_name_p = gca_tab_name_p + sizeof(tbl_name_len);

		/* copy the table name */
		/*
		** The table name must be normalized and case translated!
		MEmove(tbl_name_len, tbl_name_p, ' ', sizeof(DB_TAB_NAME),
		    &sscb->sscb_psccb->psq_tabname);
		**
		*/
		l_id = tbl_name_len;

		if (l_id != 0)
		{
		    templen = DB_TAB_MAXNAME;
		    status = cui_idxlate((u_char *)tbl_name_p,
					 &l_id, (u_char *)tempstr, &templen,
					 sscb->sscb_ics.ics_dbxlate,
					 (u_i4 *)NULL, &cui_err);

		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_uput(cui_err.err_code, 0, 2,
				  sizeof(ERx("Table name"))-1,
				  (PTR)ERx("Table name"),
				  l_id, (PTR)tbl_name_p,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			/* output to error log, too */
			sc0e_put(cui_err.err_code, 0, 2,
				 sizeof(ERx("(a) Table name"))-1,
				 (PTR)ERx("(a) Table name"),
				 l_id, (PTR)tbl_name_p,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }	/* end if status == failed */
		}	/* end if l_id != 0) */
		else
		{
		    /* if l_id is 0, then setting templen to 0 will cause
		    ** MEmove to fill the table name entirely with spaces
		    */
		    templen = 0;
		}

		(VOID)MEmove(templen, (PTR)tempstr, ' ',
			sizeof(DB_TAB_NAME),
			(PTR) &sscb->sscb_psccb->psq_tabname);
	    }
	    else
	    {
		dldata = (GCA_DL_DATA *)rv->gca_data_area;
		id = &dldata->gca_cursor_id;

		/*
		** user name was not specified.  Fill with blanks
		** I am reusing a field in PSQ_CB, but this should cause no
		** conflicts
		*/
		MEfill(sizeof(DB_OWN_NAME), (u_char) ' ',
		       &sscb->sscb_psccb->psq_als_owner);

		/*
		** The table name must be normalized and case translated!
		**
		MEmove(dldata->gca_table_name.gca_l_name, 
		       dldata->gca_table_name.gca_name,
		       ' ', sizeof(DB_TAB_NAME),
			&sscb->sscb_psccb->psq_tabname);
		**
		*/
		l_id = dldata->gca_table_name.gca_l_name;

		if (l_id != 0)
		{
		    templen = DB_TAB_MAXNAME;
		    status = cui_idxlate(
				     (u_char *)dldata->gca_table_name.gca_name,
					 &l_id, (u_char *)tempstr, &templen,
					 sscb->sscb_ics.ics_dbxlate,
					 (u_i4 *)NULL, &cui_err);
		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_uput(cui_err.err_code, 0, 2,
				  sizeof(ERx("Table name"))-1,
				  (PTR)ERx("Table name"),
				  l_id, (PTR)dldata->gca_table_name.gca_name,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			/* output to error log, too */
			sc0e_put(cui_err.err_code, 0, 2,
				 sizeof(ERx("(b) Table name"))-1,
				 (PTR)ERx("(b) Table name"),
				 l_id, (PTR)dldata->gca_table_name.gca_name,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }	/* end if status == failed */
		}	/* end if l_id != 0) */
		else
		{
		    /* if l_id is 0, then setting templen to 0 will cause
		    ** MEmove to fill the table name entirely with spaces
		    */
		    templen = 0;
		}

		(VOID)MEmove(templen, (PTR)tempstr, ' ',
			sizeof(DB_TAB_NAME),
			(PTR) &sscb->sscb_psccb->psq_tabname);
	    }

	    cquery->cur_qname.db_cursor_id[0] = id->gca_index[0];
	    cquery->cur_qname.db_cursor_id[1] = id->gca_index[1];
	    /*
	    ** The cursor name must be normalized and case translated
	    ** after the trace is printed.
	    ** DON'T assume GCA_MAXNAME > DB_CURSOR_MAXNAME
	    */
	    MEmove(sizeof(id->gca_name), (PTR)&id->gca_name, ' ',
		sizeof(cquery->cur_qname.db_cur_name),
		(PTR)&cquery->cur_qname.db_cur_name);

	    if (print_qry)
	    {
		i4 length;

		length = cus_trmwhite(
		    (u_i4)DB_CURSOR_MAXNAME, cquery->cur_qname.db_cur_name);

		sc0e_trace(STprintf(stbuf,
		    "Delete Cursor <%d.,%d.,%.*s> Statement\n",
		    cquery->cur_qname.db_cursor_id[0],
		    cquery->cur_qname.db_cursor_id[1],
		    length,
		    cquery->cur_qname.db_cur_name));
	    }
	    if (ult_always_trace())
	    {
                void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
                if (f)
                {
                        char tmp[1000];
                        STprintf(tmp,"(ID=%d/%d)(%.*s)",
                                cquery->cur_qname.db_cursor_id[0],
                                cquery->cur_qname.db_cursor_id[1],
                                DB_CURSOR_MAXNAME,
                                cquery->cur_qname.db_cur_name);
                        ult_print_tracefile(f,SC930_LTYPE_DELCURSOR,tmp);
                        ult_close_tracefile(f);
                }
            }

            TRformat(0, 0, sscb->sscb_ics.ics_qbuf,
                            sizeof(sscb->sscb_ics.ics_qbuf),
                    "Delete %t", DB_CURSOR_MAXNAME,
                        cquery->cur_qname.db_cur_name);
            sscb->sscb_ics.ics_l_qbuf = 7 + DB_CURSOR_MAXNAME;

	    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	    {
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }
	    sscb->sscb_qmode = PSQ_DELCURS;
	    done = TRUE;
	    break;
	}

	/*
	** NOTE NOTE NOTE *** DANGER DANGER DANGER
	** The following section of code is largely a duplication of the code
	** which is also present below for GCA_INVOKE (for repeat query rather
	** than database procedures).  Any changes to either section should be
	** checked to see whether they apply to both sections.
	*/

	case GCA_INVPROC:
	case GCA1_INVPROC:
	case GCA2_INVPROC:
	{
	    u_i4	        l_id;
	    char	        tempstr[DB_MAXNAME];
	    u_i4	        templen;
	    DB_ERROR	        cui_err;
	    i4			pcount;
	    i4			l_name;
	    PTR			*p;
	    char		*place, *new_place;
	    GCA_DATA_VALUE	*gdv;
	    GCA_DATA_VALUE      *first_gdv;
	    DB_DATA_VALUE	sc930_dbdv; /* DB_DATA_VALUE for SC930
					       and printqry output */
	    GCA_P_PARAM		*gpm;
	    GCA_P_PARAM		*sc930_gpm;
	    GCA_P_PARAM         *first_gpm;
	    char                *gca_data_end;
	    QEF_USR_PARAM	*qup;
	    i4			gca_parm_mask = 0;
	    bool                blob_input = FALSE;
	    DB_DATA_VALUE       dbdv;
	    i4                  dtbits;

	    sscb->sscb_qmode = PSQ_EXEDBP;
	    sscb->sscb_rsptype = GCA_RETPROC;
	    *next_state = SCS_PROCESS;
	    cquery->cur_state = CUR_FIRST_TIME;
	    cquery->cur_rowproc = FALSE;

	    if (sscb->sscb_thandle == 0)
	    {
		/*
		** Regarding GCA{1,2}_INVPROC which are associated with 
		** GCA{1,2}_IP_DATA: GCA1_IP_DATA adds a GCA_NAME struct 
		** that holds the owner name and GCA2_IP_DATA changes the 
		** GCA_ID struct which holds the procedure name into a 
		** GCA_NAME struct.
		**
		** Information from the three message objects are extracted 
		** into common components:
		**	proc_l_name	Length of procedure name.
		**	proc_name	Procedure name.
		**	own_l_name	Length of procedure owner, possibly 0.
		**	own_name	Procedure owner, possibly NULL.
		**	proc_mask	Invoke flags.
		**	first_gpm	Start of parameter list.
		*/
		char	*proc_name;
		i4	proc_l_name;
		char	*own_name = NULL;
		i4	own_l_name = 0;
		i4	proc_mask;
		char	*ptr;
		i4	db_cursor_id[2];

		if (mode == GCA_INVPROC)
		{
		    GCA_IP_DATA *msg = (GCA_IP_DATA *)rv->gca_data_area;

		    db_cursor_id[0] = msg->gca_id_proc.gca_index[0];
		    db_cursor_id[1] = msg->gca_id_proc.gca_index[1];

		    proc_l_name = sizeof(msg->gca_id_proc.gca_name);
		    proc_name = msg->gca_id_proc.gca_name;
		    proc_mask = msg->gca_proc_mask;
	    	    first_gpm = msg->gca_param;
		}
		else  if ( mode == GCA1_INVPROC )
		{
		    GCA1_IP_DATA *msg = (GCA1_IP_DATA *)rv->gca_data_area;

		    db_cursor_id[0] = msg->gca_id_proc.gca_index[0];
		    db_cursor_id[1] = msg->gca_id_proc.gca_index[1];

		    proc_l_name = sizeof(msg->gca_id_proc.gca_name);
		    proc_name = msg->gca_id_proc.gca_name;

		    own_l_name = msg->gca_proc_own.gca_l_name;
		    own_name = msg->gca_proc_own.gca_name;

		    ptr = (char *)msg + sizeof(msg->gca_id_proc) +
					sizeof(msg->gca_proc_own.gca_l_name) +
					own_l_name;

#ifdef BYTE_ALIGN
		    MEcopy( (PTR)ptr, sizeof(msg->gca_proc_mask), 
		    			(PTR)&proc_mask);
#else
		    proc_mask = *((i4 *)ptr);
#endif

		    first_gpm = (GCA_P_PARAM *)(ptr + 
		    				sizeof(msg->gca_proc_mask));
		}
		else  /* if ( mode == GCA2_INVPROC ) */
		{
		    GCA2_IP_DATA *msg = (GCA2_IP_DATA *)rv->gca_data_area;

		    db_cursor_id[0] = 0;
		    db_cursor_id[1] = 0;

		    proc_l_name = msg->gca_proc_name.gca_l_name;
		    proc_name = msg->gca_proc_name.gca_name;

		    ptr = (char *)msg + sizeof(msg->gca_proc_name.gca_l_name) +
					proc_l_name;

#ifdef BYTE_ALIGN
		    MEcopy( (PTR)ptr, 
		    			sizeof(msg->gca_proc_own.gca_l_name), 
		    			(PTR)&own_l_name);
#else
		    own_l_name = *((i4 *)ptr);
#endif

		    own_name = ptr + sizeof(msg->gca_proc_own.gca_l_name);
		    ptr = own_name + own_l_name;

#ifdef BYTE_ALIGN
		    MEcopy( (PTR)ptr, sizeof(msg->gca_proc_mask), 
		    			(PTR)&proc_mask);
#else
		    proc_mask = *((i4 *)ptr);
#endif

		    first_gpm = (GCA_P_PARAM *)(ptr + 
		    				sizeof(msg->gca_proc_mask));
		}

		if ( proc_mask & GCA_IP_NO_XACT_STMT)
		    sscb->sscb_req_flags |= SCS_NO_XACT_STMT;

		cquery->cur_dbp_names = proc_mask & GCA_PARAM_NAMES_MASK;
		sscb->sscb_param = 0;

		/*
		** The proc name must be normalized and case translated
		** after the trace is printed.
		** DON'T assume GCA_MAXNAME > DB_CURSOR_MAXNAME
		*/
		MEmove( proc_l_name, (PTR)proc_name, ' ',
		    sizeof(cquery->cur_qname.db_cur_name),
		    (PTR)&cquery->cur_qname.db_cur_name);
		cquery->cur_qname.db_cursor_id[0] = db_cursor_id[0];
		cquery->cur_qname.db_cursor_id[1] = db_cursor_id[1];
		
		l_id =  cus_trmwhite((u_i4)DB_CURSOR_MAXNAME,
		      (char *)cquery->cur_qname.db_cur_name);

		if (print_qry)
		{
		    sc0e_trace(STprintf(stbuf,
			"Execute Procedure <%d.,%d.,%.*s> Statement\n",
			cquery->cur_qname.db_cursor_id[0],
			cquery->cur_qname.db_cursor_id[1],
			(i4) l_id,
			cquery->cur_qname.db_cur_name));
		}

	        if (ult_always_trace())
	        {
                    void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
                    if (f)
                    {
                        char tmp[1000];
                        STprintf(tmp,"(ID=%d/%d)(%.*s)",
                                cquery->cur_qname.db_cursor_id[0],
                                cquery->cur_qname.db_cursor_id[1],
                                (u_i4)l_id,
                                cquery->cur_qname.db_cur_name);
                        ult_print_tracefile(f,SC930_LTYPE_EXEPROCEDURE,tmp);
                        ult_close_tracefile(f);
                    }
                }

	        /*
	        ** The procedure name must be case translated here
		**
		** Try to use the 'fast' idxlate function;
		** if it fails, then use the normal idxlate function.
	        */
		if (cui_f_idxlate( cquery->cur_qname.db_cur_name,
			  l_id, sscb->sscb_ics.ics_dbxlate) == FALSE)
		{
		    templen = DB_CURSOR_MAXNAME;
		    status = cui_idxlate(
		      (u_char *)cquery->cur_qname.db_cur_name,
		      &l_id, (u_char *)tempstr, &templen,
		      sscb->sscb_ics.ics_dbxlate,
		      (u_i4 *)NULL, &cui_err);
		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_uput(cui_err.err_code, 0, 2,
				  sizeof(ERx("Procedure name"))-1,
				  (PTR)ERx("Procedure name"),
				  l_id, (PTR)cquery->cur_qname.db_cur_name,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			/* output to error log, too */
			sc0e_put(cui_err.err_code, 0, 2,
				 sizeof(ERx("Procedure name"))-1,
				 (PTR)ERx("Procedure name"),
				 l_id, (PTR)cquery->cur_qname.db_cur_name,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0,
				 0, (PTR)0);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }
		    (VOID)MEmove(templen, (PTR)tempstr, ' ',
			 DB_CURSOR_MAXNAME, cquery->cur_qname.db_cur_name);
		}
		else
		{
		    /* cui_f_idxlate succeeded and translated name,
		    ** but still need to blank-fill rest of name
		    */
		    for (i = l_id; i < DB_CURSOR_MAXNAME; i++)
			cquery->cur_qname.db_cur_name[i]=' ';
		}

		/* Changed the code below to use ME* routines rather than 
		** TRformat() as cpu profile showed this TRformat() relatively 
		** high in dbp execution.  It is basically creating the 
		** string: "Execute Procedure dbp_name" for display in 
		** iimonitor.
		*/
		MEcopy("Execute Procedure ", 18, sscb->sscb_ics.ics_qbuf);
		MEcopy( cquery->cur_qname.db_cur_name,
			   DB_CURSOR_MAXNAME,
			   &sscb->sscb_ics.ics_qbuf[18]);
		sscb->sscb_ics.ics_l_qbuf = 18 + DB_CURSOR_MAXNAME;
		if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
		{
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START);
		}

		if ( ! own_l_name )
		{
		    sscb->sscb_psccb->psq_flag &= ~PSQ_OWNER_SET;
		    MEfill(sizeof(DB_OWN_NAME), (u_char) ' ',
			&sscb->sscb_psccb->psq_als_owner);
		}
		else
		{
		    PSQ_CB *pscb = sscb->sscb_psccb;

		    MEmove(own_l_name, own_name, ' ',
			    sizeof(DB_OWN_NAME), (PTR) &(pscb->psq_als_owner));

		    pscb->psq_flag &= ~PSQ_OWNER_SET;
		    if ((MEcmp(DB_ABSENT_NAME, (PTR) &pscb->psq_als_owner, 
			       sizeof(DB_OWN_NAME))) && 
		        (own_l_name > 0))
			pscb->psq_flag |= PSQ_OWNER_SET;

		    l_id = own_l_name;

		    /*
		    ** The procedure owner must be case translated here
		    */ 
		    if (pscb->psq_flag & PSQ_OWNER_SET)
		    {
			/* Try to use the 'fast' idxlate function;
			** if it fails, then use the normal idxlate function.
			*/ 
			if (cui_f_idxlate(pscb->psq_als_owner.db_own_name,
					  l_id,
					  sscb->sscb_ics.ics_dbxlate)
			        == FALSE)
			{
			    templen = DB_OWN_MAXNAME;
			    status = cui_idxlate(
				     (u_char *)pscb->psq_als_owner.db_own_name,
				     &l_id, (u_char *)tempstr, &templen,
				     sscb->sscb_ics.ics_dbxlate,
				     (u_i4 *)NULL, &cui_err);
			    if (DB_FAILURE_MACRO(status))
			    {
				sc0e_uput(cui_err.err_code, 0, 2,
					  sizeof(ERx("Procedure owner"))-1,
					  ERx("Procedure owner"),
					  l_id,
					  pscb->psq_als_owner.db_own_name,
					  0, (PTR)0,
					  0, (PTR)0,
					  0, (PTR)0,
					  0, (PTR)0);
				/* output to error log, too */
				sc0e_put(cui_err.err_code, 0, 2,
					 sizeof(ERx("Procedure owner"))-1,
					 ERx("Procedure owner"),
					 l_id,
					 pscb->psq_als_owner.db_own_name,
					 0, (PTR)0,
					 0, (PTR)0,
					 0, (PTR)0,
					 0, (PTR)0);
				sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
				ret_val = E_DB_ERROR;
				break;
			    }
			    (VOID)MEmove(templen, (PTR)tempstr, ' ',
					 sizeof(DB_OWN_NAME),
					 pscb->psq_als_owner.db_own_name);

			}  /* end if cui_f_idxlate */
		    }      /* end if PSQ_OWNER_SET */
		}          /* end if GCA1_INVPROC */

		gca_data_end = rv->gca_data_area + rv->gca_d_length;

		/*
		** If 'done', then all parameters are in the input block
		**
		** Check for blob data in the input.
		** Use the parameters in place unless blob parameter
		*/
		if (done && (sizeof(ALIGN_RESTRICT) <= GCA_DESC_SIZE))
		{
# ifdef BYTE_ALIGN
		    MEcopy((PTR)&first_gpm->gca_parname.gca_l_name,
			       sizeof(l_name), (PTR)&l_name);
# else
		    l_name = first_gpm->gca_parname.gca_l_name;
# endif
		    first_gdv = (GCA_DATA_VALUE *)
			    ((char *) first_gpm->gca_parname.gca_name +
					l_name +
					sizeof(first_gpm->gca_parm_mask));

                    for (pcount = 0, gdv = first_gdv, gpm = first_gpm;
			    gpm < (GCA_P_PARAM *)gca_data_end
			    && gdv < (GCA_DATA_VALUE *)gca_data_end;
			    pcount++)
		    {

# ifdef BYTE_ALIGN
			MEcopy((PTR)gdv, GCA_DESC_SIZE, (PTR)&sscb->sscb_gcadv);
			dbdv.db_datatype =
			    (DB_DT_ID) sscb->sscb_gcadv.gca_type;
			dbdv.db_length =
			    (i4) sscb->sscb_gcadv.gca_l_value;
# else
			dbdv.db_datatype = (DB_DT_ID) gdv->gca_type;
			dbdv.db_length = (i4) gdv->gca_l_value;
# endif

			MEcopy((char *)gdv - sizeof(gca_parm_mask),
					   sizeof(gca_parm_mask),
					   (char *) &gca_parm_mask);

			if (gca_parm_mask & GCA_IS_GTTPARM)
			{
			    /* Procedure with a temp table parameter. */
			    break;
			} /* end of GCA_IS_GTTPARM */

			if (dbdv.db_datatype != DB_QTXT_TYPE)
			{
			    status = adi_dtinfo(
					sscb->sscb_adscb,
					dbdv.db_datatype, &dtbits);
			    if (status)
			    {
				sc0e_0_put(sscb->sscb_adscb->adf_errcb.ad_errcode, 0);
				sc0e_0_put( E_SC021C_SCS_INPUT_ERROR, 0);
				sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
				ret_val = E_DB_ERROR;
				break;
			    }
			}
			else
			{
			    dtbits = 0;
			}

			if (dtbits & AD_PERIPHERAL)
			{
			    blob_input = TRUE;
			    break;
			}

			place = gdv->gca_value;
			new_place =
			    (char *) ME_ALIGN_MACRO(place, sizeof(ALIGN_RESTRICT));
			if (place == new_place)
			{
			    /*
			    ** Assume gca_l_value is aligned if gca_value is
			    ** (determined above when place == new_place).
			    */
			    gpm = (GCA_P_PARAM *)
				((char *) gdv->gca_value + gdv->gca_l_value);
			}
			else
			{
			    place += dbdv.db_length;
			    gpm = (GCA_P_PARAM *) place;
			}
# ifdef BYTE_ALIGN
			MEcopy((PTR)&gpm->gca_parname.gca_l_name,
			       sizeof(l_name), (PTR)&l_name);
			gdv = (GCA_DATA_VALUE *)
				((char *) gpm->gca_parname.gca_name +
				    l_name + sizeof(gpm->gca_parm_mask));   
# else
			gdv = (GCA_DATA_VALUE *)
				((char *) gpm->gca_parname.gca_name +
					    gpm->gca_parname.gca_l_name +
					    sizeof(gpm->gca_parm_mask));
#endif
		    }
                }

		/*
		** If 'done', then all parameters are in the input block
		** Use the parameters in place unless blob parameter found
		*/
		if (done && (sizeof(ALIGN_RESTRICT) <= GCA_DESC_SIZE)
			&& blob_input == FALSE)
		{
		    p = sscb->sscb_param = 
						sscb->sscb_ptr_vector;

		    gpm = first_gpm;
		    gdv = first_gdv;
                    if (gpm < (GCA_P_PARAM *)gca_data_end &&
                        gdv < (GCA_DATA_VALUE *)gca_data_end)
		       MEcopy((char *)gdv - sizeof(gca_parm_mask),
				sizeof(gca_parm_mask), (char *) &gca_parm_mask);
					/* grab parm[1] mask for GTT check */

		    if (gca_parm_mask & GCA_IS_GTTPARM)
		    {
			/* This is a prtocedure with a temp table parameter.
			** Because of the manner in which temp table parms
			** are verified and compiled, the usual execution
			** of procedure invocation from SCF cannot be followed. 			** Instead, we build a text buffer which contains the
			** reconstituted execute procedure syntax, and handle
			** it like a dynamic SQL statement - passing it thru
			** PSF, OPF and QEF. This allows the proper temp table
			** definition verification and binding to be performed. 
			** There is no doubt that this is a significant kludge,
			** but to have embedded procedure invocation work so
			** completely different from dynamic, and from procedure
			** calls nested in other procs is pretty poor, too. */

			i4	textlen, i, ptype;
			u_i2	plen;
			PSQ_CB	*psqcb;

			/* First thing to do is set up the QSF structures
			** in which to stick the rebuilt syntax. This logic
			** has been cloned from elsewhere in this routine, but
			** is not sufficiently generalizeable to warrant the
			** creation of a new function. */

		    /* allocate the object, store the handle, and move it in */
		    qs_ccb = sscb->sscb_qsccb;
		    qs_ccb->qsf_obj_id.qso_type = QSO_QTEXT_OBJ; /* saving the query text */
		    qs_ccb->qsf_obj_id.qso_lname = 0;	/* object has no name */
		    status = qsf_call(QSO_CREATE, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }
		    sscb->sscb_thandle = qs_ccb->qsf_obj_id.qso_handle;
		    sscb->sscb_tlk_id = qs_ccb->qsf_lk_id;
		    sscb->sscb_tsize =
			    qs_ccb->qsf_sz_piece =
				 SCS_TEXT_DEFAULT;
		    status = qsf_call(QSO_PALLOC, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }
		    sscb->sscb_troot = qs_ccb->qsf_root = qs_ccb->qsf_piece;
		    status = qsf_call(QSO_SETROOT, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code, 
				E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }
		    sscb->sscb_dnleft = 0;
		    sscb->sscb_dptr = 0;
		    sscb->sscb_tnleft = sscb->sscb_tsize
							- sizeof(PSQ_QDESC);
		    sscb->sscb_tptr = 
			((PSQ_QDESC *) sscb->sscb_troot)->psq_qrytext =
			    ((char *) sscb->sscb_troot)
							+ sizeof(PSQ_QDESC);
		    ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrysize = 0;
		    ((PSQ_QDESC *) sscb->sscb_troot)->psq_datasize = 0;
		    ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrydata = 0;
		    ((PSQ_QDESC *) sscb->sscb_troot)->psq_dnum = 0;
		    sscb->sscb_gcadv.gca_type = DB_QTXT_TYPE;
		    sscb->sscb_gcadv.gca_precision = 0;

			/* Now copy the first chunk of syntax (the "execute
			** procedure" piece). */

			MEcopy((char *)&execproc_syntax, 
			    sizeof(execproc_syntax), 
			    (char *)sscb->sscb_tptr);
			textlen = sizeof(execproc_syntax)-1;

			/* If there was an owner name specificed, copy it
			** next. */
			psqcb = sscb->sscb_psccb;
			if (psqcb->psq_flag & PSQ_OWNER_SET)
			{
			    MEcopy(own_name, own_l_name,
				(char *)sscb->sscb_tptr+textlen);
			    textlen += own_l_name;
			    sscb->sscb_tptr[textlen] = '.';
			    textlen++;
			}

			/* Determine exact length of procedure name and copy
			** it to end of current string. */
			for (i = proc_l_name; i > 0 && (proc_name[i-1] == ' '
				|| proc_name[i-1] == 0); i--);
					/* loop back 'til non-blank/zero */
			MEcopy(proc_name, i,
			    (char *)sscb->sscb_tptr+textlen);
			textlen += i;
			sscb->sscb_tptr[textlen] = '(';
			textlen++;

			/* Now copy the parameter syntax. First the parameter
			** name. */

			MEcopy((char *)&gpm->gca_parname.gca_name,
			    l_name, (char *)sscb->sscb_tptr+textlen);	
			textlen += l_name;

			/* Next the " = session." chunk. */
			MEcopy((char *)&execproc_syntax2,
			    sizeof(execproc_syntax2), 
			    (char *)sscb->sscb_tptr+textlen);
			textlen += sizeof(execproc_syntax2)-1;

			/* Finally, the table name. It is a varchar constant
			** value passed as the first (and only) parm. */
			MEcopy((char *)&gdv->gca_type,
			    sizeof(gdv->gca_type), (char *)&ptype);
			if (ptype != DB_VCH_TYPE) break;
					/* not likely, but quit if so */
			MEcopy((char *)&gdv->gca_value,
			    sizeof(u_i2), (char *)&plen);
			MEcopy((char *)&gdv->gca_value+2, plen,
			    (char *)sscb->sscb_tptr+textlen);
			textlen += plen;
			sscb->sscb_tptr[textlen] = ')';
			sscb->sscb_tptr[textlen+1] = ' ';
			sscb->sscb_tptr[textlen+2] = 0;
			textlen += 3;

			/* Syntax is composed - just fill in some stuff. */
			sscb->sscb_tptr += textlen;
			sscb->sscb_tnleft -= textlen;
			((PSQ_QDESC *)sscb->sscb_troot)->
			    psq_qrysize = textlen;
			sscb->sscb_gcadv.gca_l_value = textlen;
			sscb->sscb_qmode = 0;
			*next_state = 0;
			ret_val = E_DB_OK;
			break;		/* done this flavour of exproc */
		    }		/* end of GCA_IS_GTTPARM */

                    for (pcount = 0, gdv = first_gdv, gpm = first_gpm;
			    gpm < (GCA_P_PARAM *)gca_data_end
			    && gdv < (GCA_DATA_VALUE *)gca_data_end;
			    pcount++)
		    {
			/* We use the standard sized buffer for ``small	    */
			/* queries.''  If the size is exceeded, then build  */
			/* a real place, and move the data to there, then   */
			/* continue.					    */
			/* We have to subtract the sizeof(ALIGN_RESTRICT)   */
			/* from the sizeof(GCA_DATA_VALUE) because the data */
			/* value be less than sizeof(ALIGN_RESTRICT) and we */
			/* would end up thinking that there are fewer       */
			/* parameters than there really are.		    */

			if ((pcount >= (SCS_SQPARAMS))
				    && (!sscb->sscb_troot))
			{
			    status = sc0m_allocate(0,
					sizeof(SC0M_OBJECT) +
					    ((rv->gca_d_length
					    / (sizeof(GCA_DATA_VALUE) - sizeof(ALIGN_RESTRICT)))
					    * sizeof(PTR)),
					DB_SCF_ID,
					(PTR)SCS_MEM,
					SCG_TAG,
					&sscb->sscb_troot);
			    if (status)
			    {
				sc0e_0_put(status, 0);
				scd_note(E_DB_SEVERE, DB_SCF_ID);
				return(E_DB_SEVERE);
			    }
			    sscb->sscb_param =
				    (PTR *) ((char *) sscb->sscb_troot +
						sizeof(SC0M_OBJECT));
			    MEcopy((PTR)sscb->sscb_ptr_vector,
				    sizeof(sscb->sscb_ptr_vector),
				    (PTR)sscb->sscb_param);
			    p = (PTR *) ((char *) sscb->sscb_param
					+ sizeof(sscb->sscb_ptr_vector));

			}
			if (sscb->sscb_dbparam == 0)
			{
			    /*
			    ** Now, we have to allocate an array of
			    ** QEF_USR_PARAM's to hold all the parameters
			    ** This will be saved in sscb_dbparam slot for
			    ** deallocation later, with pointer to actual
			    ** parameters in sscb_proot...
			    */

			    status = sc0m_allocate(0,
					sizeof(SC0M_OBJECT) +
					    ((rv->gca_d_length
					      / (sizeof(GCA_P_PARAM) - sizeof(ALIGN_RESTRICT))
					      + 1)
					     * sizeof(QEF_USR_PARAM)),
					DB_SCF_ID,
					(PTR)SCS_MEM,
					SCG_TAG,
					&sscb->sscb_proot);
			    if (status)
			    {
				sc0e_0_put(status, 0);
				scd_note(E_DB_SEVERE, DB_SCF_ID);
				return(E_DB_SEVERE);
			    }
			    sscb->sscb_dbparam =
				    (PTR) ((char *) sscb->sscb_proot +
						sizeof(SC0M_OBJECT));
			    qup = (QEF_USR_PARAM *) sscb->sscb_dbparam;
			}
# ifdef BYTE_ALIGN
			MEcopy((PTR)gdv, GCA_DESC_SIZE,
					   (PTR)&sscb->sscb_gcadv);
# endif

			/* populate the DB_DATA_VALUE used for printqry/SC930
			 * tracing - apart from the db_data field which is 
			 * done after the gca_value is byte-aligned (if needed).
			 *
			 * this is done in two parts because gdv->gca_l_value
			 * may have been overwritten later on
			 */

			if (print_qry || ult_always_trace())
			{
#ifdef BYTE_ALIGN
			    /* type of namelen must equal type of gca_l_name */
			    i4                  namelen;

			    if (print_qry)
			    {
			        MEcopy((PTR)&gpm->gca_parname.gca_l_name, 
				sizeof(namelen),(PTR)&namelen);
			        sc0e_trace(STprintf(stbuf,
			  	    "Parameter Name %*s, Value:\n",
				     namelen, 
				     gpm->gca_parname.gca_name));
			    }

			    sc930_dbdv.db_datatype = 
				(DB_DT_ID) sscb->sscb_gcadv.gca_type;
			    sc930_dbdv.db_prec =
				(i2) sscb->sscb_gcadv.gca_precision;
			    sc930_dbdv.db_length =
				(i4) sscb->sscb_gcadv.gca_l_value;
# else

			    if (print_qry)
			        sc0e_trace(STprintf(stbuf,
				    "Parameter Name %*s, Value:\n",
				    gpm->gca_parname.gca_l_name,
				    gpm->gca_parname.gca_name));

			    sc930_dbdv.db_datatype = (DB_DT_ID) gdv->gca_type;
			    sc930_dbdv.db_prec = (i2) gdv->gca_precision;
			    sc930_dbdv.db_length = (i4) gdv->gca_l_value;
# endif
			}

			/*
			** Query text audit parameter
			*/
			if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
			{
			    DB_DATA_VALUE	dbdv;
			    /* type of namelen must equal type of gca_l_name */
			    i4                  namelen;
# ifdef BYTE_ALIGN			    
			    MEcopy((PTR)&gpm->gca_parname.gca_l_name, 
				sizeof(namelen),(PTR)&namelen);
			    dbdv.db_datatype = 
				(DB_DT_ID) sscb->sscb_gcadv.gca_type;
			    dbdv.db_prec =
				(i2) sscb->sscb_gcadv.gca_precision;
			    dbdv.db_length =
				(i4) sscb->sscb_gcadv.gca_l_value;
# else
			    namelen=gpm->gca_parname.gca_l_name;
			    dbdv.db_datatype = (DB_DT_ID) gdv->gca_type;
			    dbdv.db_prec = (i2) gdv->gca_precision;
			    dbdv.db_length = (i4) gdv->gca_l_value;
# endif
			    dbdv.db_data = (PTR) gdv->gca_value;

			    sc0a_qrytext_param(scb, 
				gpm->gca_parname.gca_name,
				namelen,
				&dbdv);
			}


# ifdef BYTE_ALIGN
			qup->parm_dbv.db_datatype =
			    (DB_DT_ID) sscb->sscb_gcadv.gca_type;
			qup->parm_dbv.db_prec =
			    (i2) sscb->sscb_gcadv.gca_precision;
			qup->parm_dbv.db_length =
			    (i4) sscb->sscb_gcadv.gca_l_value;
			MEcopy((PTR)&gpm->gca_parname.gca_l_name,
			    sizeof(qup->parm_nlen), (PTR)&qup->parm_nlen);
# else
			qup->parm_dbv.db_datatype =
			    (DB_DT_ID) gdv->gca_type;
			qup->parm_dbv.db_prec = (i2) gdv->gca_precision;
			qup->parm_dbv.db_length = (i4) gdv->gca_l_value;
			qup->parm_nlen = gpm->gca_parname.gca_l_name;
# endif
			qup->parm_name = gpm->gca_parname.gca_name;

			/*
			** The parm name must be normalized & case translated.
			** We will do this "in place" here.
			**
			** Try to use the 'fast' idxlate function;
			** if it fails, then use the normal idxlate function.
			*/
			l_id = qup->parm_nlen;
			if (cui_f_idxlate(qup->parm_name, l_id,
					  sscb->sscb_ics.ics_dbxlate)
			    == FALSE)
			{
			    templen = DB_PARM_MAXNAME;
			    status = cui_idxlate((u_char *)qup->parm_name,
					 &l_id, (u_char *)tempstr, &templen,
					 sscb->sscb_ics.ics_dbxlate,
					 (u_i4 *)NULL, &cui_err);
			    if (DB_FAILURE_MACRO(status))
			    {
				sc0e_uput(cui_err.err_code, 0, 2,
				      sizeof(ERx("Procedure parameter name"))-1,
				      (PTR)ERx("Procedure parameter name"),
				      l_id, (PTR)qup->parm_name,
				      0, (PTR)0,
				      0, (PTR)0,
				      0, (PTR)0,
				      0, (PTR)0);
				/* output to error log, too */
				sc0e_put(cui_err.err_code, 0, 2,
				     sizeof(ERx("Procedure parameter name"))-1,
				     (PTR)ERx("Procedure parameter name"),
				     l_id, (PTR)qup->parm_name,
				     0, (PTR)0,
				     0, (PTR)0,
				     0, (PTR)0,
				     0, (PTR)0);
				/*****************
				** for now, don't return an error here,
				** since that will terminate the session
				** without printing out the above error.
				** Instead, continue and let QEF return
				** a 'parameter not found' error.
				** 		(rblumer 07-jan-94)
				sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
				ret_val = E_DB_ERROR;
				break;
				*****************/
			    }
			    /*
			    ** NOTE: Since the normalization/translation is done
			    ** "in place," the parameter name will have trailing
			    ** blanks when delimiters were provided....
			    */
			    (VOID)MEmove(templen, (PTR)tempstr, ' ',
					 qup->parm_nlen,
					 qup->parm_name);

			}  /* end if cui_f_idxlate */

			MEcopy((char *)gdv - sizeof(gca_parm_mask),
					   sizeof(gca_parm_mask),
					   (char *) &gca_parm_mask);

			/* Set parm_flags for BYREF/OUTPUT parameters. */
			qup->parm_flags = ((gca_parm_mask & GCA_IS_BYREF) &&
					   (cscb->cscb_version >=
					    GCA_PROTOCOL_LEVEL_60)
		    && !ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_NOBYREF,
					NULL,NULL))?
						QEF_IS_BYREF : 0;
			qup->parm_flags |= ((gca_parm_mask & GCA_IS_OUTPUT) &&
					   (cscb->cscb_version >=
					    GCA_PROTOCOL_LEVEL_60)
		    && !ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_NOBYREF,
					NULL,NULL))?
						QEF_IS_OUTPUT : 0;
			/* Name-less parm must be positional. */
			if (qup->parm_nlen == 0)
			    qup->parm_flags |= QEF_IS_POSPARM;


                        /* keep old gpm for use with SC930/printqry output */
			sc930_gpm=gpm;

			place = gdv->gca_value;
			new_place =
			    (char *) ME_ALIGN_MACRO(place, sizeof(ALIGN_RESTRICT));

			if (place == new_place)
			{
			    qup->parm_dbv.db_data = place;
			    *p++ = (PTR) qup;
			    /*
			    ** Assume gca_l_value is aligned if gca_value is
			    ** (determined above when place == new_place).
			    */
			    gpm = (GCA_P_PARAM *)
				((char *) gdv->gca_value + gdv->gca_l_value);
			}
			else
			{
			    new_place -= sizeof(ALIGN_RESTRICT);
			    qup->parm_dbv.db_data = new_place;
			    *p++ = (PTR) qup /*new_place */;
			    for (i = 0; i < qup->parm_dbv.db_length; i++)
			    {
				*new_place++ = *place++;
			    }
			    gpm = (GCA_P_PARAM *) place;
			}


			/* populate the db_data field of the DB_DATA_VALUE for
			 * SC930 and printqry tracing which both need aligned
			 * copies of the data value 
			 *
			 * and output the value
			 */

			if (print_qry || ult_always_trace())
			{
			    sc930_dbdv.db_data = (PTR) qup->parm_dbv.db_data ;

			    if (print_qry)
			      adu_2prvalue(sc0e_trace, &sc930_dbdv);

			    if (ult_always_trace())
			      (*print_sc930_ptr)(scb,&sc930_dbdv,sc930_gpm,pcount,
						 SC930_LTYPE_PARMEXEC);

			}
# ifdef BYTE_ALIGN
			MEcopy((PTR)&gpm->gca_parname.gca_l_name,
			       sizeof(l_name), (PTR)&l_name);
			gdv = (GCA_DATA_VALUE *)
				((char *) gpm->gca_parname.gca_name +
				    l_name + sizeof(gpm->gca_parm_mask));   
# else
			gdv = (GCA_DATA_VALUE *)
				((char *) gpm->gca_parname.gca_name +
					    gpm->gca_parname.gca_l_name +
					    sizeof(gpm->gca_parm_mask));
#endif
			qup++;
		    }
		    cquery->cur_qprmcount = pcount;
		    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
		    {
			    sc0a_qrytext(scb, "",0, SC0A_QT_END);
		    }
		    break;
		}
		else
		{
		    for (;;)
		    {
			/* allocate the object, store the handle, and move it in */
			qs_ccb = sscb->sscb_qsccb;
			qs_ccb->qsf_obj_id.qso_type = QSO_QTEXT_OBJ; /* saving the query text */
			qs_ccb->qsf_obj_id.qso_lname = 0;	/* object has no name */
			status = qsf_call(QSO_CREATE, qs_ccb);
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				    E_SC021C_SCS_INPUT_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    ret_val = E_DB_ERROR;
			    break;
			}
			sscb->sscb_thandle = qs_ccb->qsf_obj_id.qso_handle;
			sscb->sscb_tlk_id = qs_ccb->qsf_lk_id;
			sscb->sscb_tsize =
				qs_ccb->qsf_sz_piece =
				    ((qry_size > SCS_TEXT_DEFAULT) ?
					qry_size * 2 : SCS_TEXT_DEFAULT);
			status = qsf_call(QSO_PALLOC, qs_ccb);
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC021C_SCS_INPUT_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    ret_val = E_DB_ERROR;
			    break;
			}
			sscb->sscb_troot = qs_ccb->qsf_root = qs_ccb->qsf_piece;
			status = qsf_call(QSO_SETROOT, qs_ccb);
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code, 
				    E_SC021C_SCS_INPUT_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    ret_val = E_DB_ERROR;
			    break;
			}
			qe_ccb = sscb->sscb_qeccb;
			qe_ccb->qef_usr_param = (QEF_USR_PARAM **)
							       sscb->sscb_troot;
			sscb->sscb_tnleft = 0;
			sscb->sscb_tptr = (PTR) sscb->sscb_troot;
			sscb->sscb_dnleft = sscb->sscb_tsize / sizeof(PTR);
			sscb->sscb_dptr = sscb->sscb_troot;
			break;
		    }
		    if (DB_FAILURE_MACRO(status))
			break;
		    ret_val = 
			scs_fdbp_data(scb,
				gca_data_end - (char *)first_gpm,
				(char *)first_gpm, 
				print_qry);
		     if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
		     {
	        	sc0a_qrytext(scb, "", 0, SC0A_QT_END);
		     }
		}
	    }
	    else
	    {
		ret_val = 
		    scs_fdbp_data(scb,
			    rv->gca_d_length,
			    rv->gca_data_area,
			    print_qry);
 	    }
	    if (done)
	    {
		qe_ccb = sscb->sscb_qeccb;
		sscb->sscb_param = (PTR *) qe_ccb->qef_usr_param;
     	    }
	    else
	    {
		*next_state = SCS_INPUT;
	    }
	    break;
	}


	/* MAINTAINERS NOTE:  Please see note above at GCA_INVPROC	    */

	case GCA_INVOKE:
	{
	    GCA_ID	        *buf;
	    i4			pcount;
	    PTR			*p;
	    char		*place, *new_place;
	    GCA_DATA_VALUE	*gdv;
	    DB_DATA_VALUE       dbdv;
	    bool                blob_input = FALSE;
	    i4                  dtbits;

	    sscb->sscb_qmode = PSQ_EXECQRY;
	    *next_state = SCS_PROCESS;
	    cquery->cur_state = CUR_FIRST_TIME;

	    if (sscb->sscb_thandle == 0)
	    {
		buf = (GCA_ID *)rv->gca_data_area;

		sscb->sscb_param = 0;

		cquery->cur_qname.db_cursor_id[0] = buf->gca_index[0];
		cquery->cur_qname.db_cursor_id[1] = buf->gca_index[1];
	    	/*
	    	** The cursor name must be normalized and case translated
	    	** after the trace is printed.
		** DON'T assume GCA_MAXNAME > DB_CURSOR_MAXNAME
	    	*/
		MEmove(sizeof(buf->gca_name), (PTR)&buf->gca_name, ' ',
		    sizeof(cquery->cur_qname.db_cur_name),
		    (PTR)&cquery->cur_qname.db_cur_name);

	    	if (ult_always_trace())
	    	{
                    void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
                    if (f)
                    {
                        char tmp[1000];
                        STprintf(tmp,"(ID=%d/%d)(%.*s)",
			    cquery->cur_qname.db_cursor_id[0],
			    cquery->cur_qname.db_cursor_id[1],
			    DB_CURSOR_MAXNAME, cquery->cur_qname.db_cur_name);
                        ult_print_tracefile(f,SC930_LTYPE_EXECUTE,tmp);
                        ult_close_tracefile(f);
                    }
                }

		if (print_qry)
		{
		    i4 length;

		    length = cus_trmwhite(
		    (u_i4)DB_CURSOR_MAXNAME, cquery->cur_qname.db_cur_name);

		    sc0e_trace(STprintf(stbuf,
			"Execute <%d.,%d.,%.*s> Statement\n",
			cquery->cur_qname.db_cursor_id[0],
			cquery->cur_qname.db_cursor_id[1],
			length,
			cquery->cur_qname.db_cur_name));
		}

		MEcopy("Execute ", sizeof("Execute ")-1, sscb->sscb_ics.ics_qbuf);
		MEcopy(cquery->cur_qname.db_cur_name, DB_CURSOR_MAXNAME,
			&sscb->sscb_ics.ics_qbuf[sizeof("Execute ")-1]);
		sscb->sscb_ics.ics_l_qbuf = 8 + DB_CURSOR_MAXNAME;
	        if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
		{
			sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
		}

		/*
		** If 'done', then all parameters are in the input block
		** Use the parameters in place unless: 
		**     STAR: Star always needs to copy the db values
		**     BLOB: Read/copy blob data with scs_fetch_data
		**
		** Check for blob data in the input.
		*/
		if (done && (sizeof(ALIGN_RESTRICT) <= GCA_DESC_SIZE) 
		    && ( !(sscb->sscb_flags & SCS_STAR) ))
		{
		    for (pcount = 0,
			    gdv = ((GCA_IV_DATA *)rv->gca_data_area)->gca_qparm;
			gdv < (GCA_DATA_VALUE *)
			    (rv->gca_data_area + rv->gca_d_length);
			pcount++)
		    {
# ifdef BYTE_ALIGN
			MEcopy((PTR)gdv, GCA_DESC_SIZE, (PTR)&sscb->sscb_gcadv);
			dbdv.db_datatype =
			    (DB_DT_ID) sscb->sscb_gcadv.gca_type;
			dbdv.db_length =
			    (i4) sscb->sscb_gcadv.gca_l_value;
# else
			dbdv.db_datatype = (DB_DT_ID) gdv->gca_type;
			dbdv.db_length = (i4) gdv->gca_l_value;
# endif

			if (dbdv.db_datatype != DB_QTXT_TYPE)
			{
			    status = adi_dtinfo(
					sscb->sscb_adscb,
					dbdv.db_datatype, &dtbits);
			    if (status)
			    {
				sc0e_0_put(sscb->sscb_adscb->adf_errcb.ad_errcode, 0);
				sc0e_0_put( E_SC021C_SCS_INPUT_ERROR, 0);
				sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
				ret_val = E_DB_ERROR;
				break;
			    }
			}
			else
			{
			    dtbits = 0;
			}

			if (dtbits & AD_PERIPHERAL)
			{
			    blob_input = TRUE;
			    sc0e_0_uput(E_SC0132_LO_REPEAT_MISMATCH, 0);
			    sc0e_0_put(E_SC0132_LO_REPEAT_MISMATCH, 0);
		      	    ret_val = E_DB_ERROR;
			    break;
			}
			place = gdv->gca_value;
			new_place =
			    (char *) ME_ALIGN_MACRO(place, sizeof(ALIGN_RESTRICT));
			if (place == new_place)
			{
			    /*
			    ** Assume gca_l_value is aligned if gca_value is
			    ** (determined above when place == new_place).
			    */
			    gdv = (GCA_DATA_VALUE *)
				    ((char *) gdv->gca_value + gdv->gca_l_value);
			}
			else
			{
			    place += dbdv.db_length;
			    gdv = (GCA_DATA_VALUE *) place;
			}
		    }
		}

		/*
		** If 'done', then all parameters are in the input block
		** Use the parameters in place unless:
		**     STAR: Star always needs to copy the db values
		**     BLOB: Read/copy blob data with scs_fetch_data
		*/
		if (done && (sizeof(ALIGN_RESTRICT) <= GCA_DESC_SIZE) 
		    && (blob_input == FALSE)
		    && ( !(sscb->sscb_flags & SCS_STAR) ))
		{
		    p = sscb->sscb_param = sscb->sscb_ptr_vector;
		    *p++ = 0;
		    for (pcount = 0,
			    gdv = ((GCA_IV_DATA *)rv->gca_data_area)->gca_qparm;
			gdv < (GCA_DATA_VALUE *)
			    (rv->gca_data_area + rv->gca_d_length);
			pcount++)
		    {
			/* We use the standard sized buffer for ``small	    */
			/* queries.''  If the size is exceeded, then build  */
			/* a real place, and move the data to there, then   */
			/* continue.					    */

			if ((pcount >= (SCS_SQPARAMS - 1))
				    && (!sscb->sscb_troot))
			{
			    /*
			    ** Here we allocate an array of pointers to data
			    ** values.  There should be one pointer for each
			    ** parameter in the GCA buffer.
			    **
			    ** We err on the side of allocating more pointers
			    ** than we need.  Since (sizeof(GCA_DATA_VALUE)) may
			    ** (on some machines) be aligned to a size larger 
			    ** than the space actually taken up by a 
			    ** GCA_DATA_VALUE, we subtract out the alignment
			    ** correction.  This usually results in our
			    ** allocating more pointers than we need.
			    **
			    ** This fixes bug 55021.
			    */
			    status = sc0m_allocate(0,
					sizeof(SC0M_OBJECT) +
					    ((rv->gca_d_length
					      / (sizeof(GCA_DATA_VALUE) - sizeof(ALIGN_RESTRICT)))
						* sizeof(PTR)),
					DB_SCF_ID,
					(PTR)SCS_MEM,
					SCG_TAG,
					&sscb->sscb_troot);
			    if (status)
			    {
				sc0e_0_put(status, 0);
				scd_note(E_DB_SEVERE, DB_SCF_ID);
				return(E_DB_SEVERE);
			    }
			    sscb->sscb_param =
				    (PTR *) ((char *) sscb->sscb_troot +
						sizeof(SC0M_OBJECT));
			    MEcopy((PTR)sscb->sscb_ptr_vector,
				    sizeof(sscb->sscb_ptr_vector),
				    (PTR)sscb->sscb_param);
			    p = (PTR *) ((char *) sscb->sscb_param
					+ sizeof(sscb->sscb_ptr_vector));

			}
# ifdef BYTE_ALIGN
			MEcopy((PTR)gdv, GCA_DESC_SIZE, (PTR)&sscb->sscb_gcadv);
			dbdv.db_datatype =
			    (DB_DT_ID) sscb->sscb_gcadv.gca_type;
			dbdv.db_prec =
			    (i2) sscb->sscb_gcadv.gca_precision;
			dbdv.db_length =
			    (i4) sscb->sscb_gcadv.gca_l_value;
# else
			dbdv.db_datatype = (DB_DT_ID) gdv->gca_type;
			dbdv.db_prec = (i2) gdv->gca_precision;
			dbdv.db_length = (i4) gdv->gca_l_value;
# endif
			place = gdv->gca_value;
			new_place =
			    (char *) ME_ALIGN_MACRO(place, sizeof(ALIGN_RESTRICT));
			if (place == new_place)
			{
			    *p = place;
			    /*
			    ** Assume gca_l_value is aligned if gca_value is
			    ** (determined above when place == new_place).
			    */
			    gdv = (GCA_DATA_VALUE *)
				    ((char *) gdv->gca_value + gdv->gca_l_value);
			}
			else
			{
			    new_place -= sizeof(ALIGN_RESTRICT);
			    *p = new_place;
			    for (i = 0; i < dbdv.db_length; i++)
			    {
				*new_place++ = *place++;
			    }
			    gdv = (GCA_DATA_VALUE *) place;
			}

			/* We need the aligned copy. */
			dbdv.db_data = (PTR) *p;

			if (ult_always_trace())
			{
			    (*print_sc930_ptr)(scb,&dbdv,NULL,pcount,SC930_LTYPE_PARM);
			}
			/* Move NULL byte to EOS */
			(VOID)adg_vlup_setnull(&dbdv);

			if (print_qry)
			{
			    adu_2prvalue(sc0e_trace, &dbdv);
			}
			/*
			** Print parameter for query text auditing
			*/
			if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
			{
				sc0a_qrytext_param(scb,  
					"Param", 5, &dbdv);
			}

			p++;
		    }
		    cquery->cur_qprmcount = pcount;
		    break;
		}
		else
		{
		    for (;;)
		    {
			/* allocate the object, store the handle, and move it in */
			qs_ccb = sscb->sscb_qsccb;
			qs_ccb->qsf_obj_id.qso_type = QSO_QTEXT_OBJ; /* saving the query text */
			qs_ccb->qsf_obj_id.qso_lname = 0;	/* object has no name */
			status = qsf_call(QSO_CREATE, qs_ccb);
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				    E_SC021C_SCS_INPUT_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    ret_val = E_DB_ERROR;
			    break;
			}
			sscb->sscb_thandle = qs_ccb->qsf_obj_id.qso_handle;
			sscb->sscb_tlk_id = qs_ccb->qsf_lk_id;
			sscb->sscb_tsize =
				qs_ccb->qsf_sz_piece =
				    ((qry_size > SCS_TEXT_DEFAULT) ?
					qry_size * 2 : SCS_TEXT_DEFAULT);
			status = qsf_call(QSO_PALLOC, qs_ccb);
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC021C_SCS_INPUT_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    ret_val = E_DB_ERROR;
			    break;
			}
			sscb->sscb_troot = qs_ccb->qsf_root = qs_ccb->qsf_piece;
			status = qsf_call(QSO_SETROOT, qs_ccb);
			if (DB_FAILURE_MACRO(status))
			{
			    scs_qsf_error(status, qs_ccb->qsf_error.err_code, 
				    E_SC021C_SCS_INPUT_ERROR);
			    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			    ret_val = E_DB_ERROR;
			    break;
			}
			sscb->sscb_tnleft = 0;
			sscb->sscb_tptr = (PTR) 
			    ((char *) sscb->sscb_troot + sizeof(PSQ_QDESC));
			sscb->sscb_dnleft = sscb->sscb_tsize
							    - sizeof(QEF_PARAM)
							    - sizeof(PSQ_QDESC);
			sscb->sscb_dptr = (char *)
			    ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrydata;
			((PSQ_QDESC *) sscb->sscb_troot)->psq_qrydata =
				(DB_DATA_VALUE **)
				    ((char *) sscb->sscb_troot
							    + sizeof(QEF_PARAM)
							    + sizeof(PSQ_QDESC));
			((PSQ_QDESC *) sscb->sscb_troot)->psq_qrysize = 0;
			((PSQ_QDESC *) sscb->sscb_troot)->psq_datasize = 0;
			((PSQ_QDESC *) sscb->sscb_troot)->psq_qrytext = 0;
			((PSQ_QDESC *) sscb->sscb_troot)->psq_dnum = 1;
			    /* QEF parameters start with 1 */
			break;
		    }
		    if (DB_FAILURE_MACRO(status))
			break;
		    ret_val = 
			scs_fetch_data(scb,
				0 /* not for PSF */,
				rv->gca_d_length - sizeof(GCA_ID),
				rv->gca_data_area + sizeof(GCA_ID),
				print_qry, &ad_peripheral);
		}
	    }
	    else
	    {
		ret_val = 
		    scs_fetch_data(scb,
			    0,
			    rv->gca_d_length,
			    rv->gca_data_area,
			    print_qry, &ad_peripheral);
 	    }
	    if (done)
	    {
		qe_ccb = sscb->sscb_qeccb;
		qe_ccb->qef_param = (QEF_PARAM *) sscb->sscb_tptr;
		qe_ccb->qef_param->dt_next = (QEF_PARAM *) 0;
		qe_ccb->qef_param->dt_size =
			    qe_ccb->qef_pcount =
				((PSQ_QDESC *)
				    sscb->sscb_troot)->psq_dnum - 1; 
		qe_ccb->qef_param->dt_data = (PTR *)
				((PSQ_QDESC *)
				    sscb->sscb_troot)->psq_qrydata;
		*qe_ccb->qef_param->dt_data = 0;
     	    }
	    else
	    {
		*next_state = SCS_INPUT;
	    }
	    break;
	}

	case GCA_Q_BTRAN:
	    done = TRUE;
	    sscb->sscb_rsptype = GCA_S_BTRAN;
	    sscb->sscb_qmode = PSQ_BGNTRANS;
	    *next_state = SCS_PROCESS;
	    if (print_qry)
	    {
		sc0e_trace("Begin Transaction Statement\n");
	    }
            MEcopy("Begin Transaction", 17,
                sscb->sscb_ics.ics_qbuf);
            sscb->sscb_ics.ics_l_qbuf = 17;
	    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	    {
		    sc0a_qrytext(scb, 
			sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }
	    break;

	case GCA_Q_ETRAN:
	    done = TRUE;
	    sscb->sscb_rsptype = GCA_S_ETRAN;
	    sscb->sscb_qmode = PSQ_ENDTRANS;
	    *next_state = SCS_PROCESS;
	    if (print_qry)
	    {
		sc0e_trace("End Transaction Statement\n");
	    }
            MEcopy("End Transaction", 15,
                sscb->sscb_ics.ics_qbuf);
            sscb->sscb_ics.ics_l_qbuf = 17;
	    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	    {
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }
	    break;

	case GCA_COMMIT:
	    done = TRUE;
	    sscb->sscb_rsptype = GCA_DONE;
	    sscb->sscb_qmode = PSQ_COMMIT;
	    *next_state = SCS_PROCESS;
	    if (print_qry)
	    {
		sc0e_trace("Commit [Work] Statement\n");
	    }
            MEcopy("Commit [work]", 13,
            sscb->sscb_ics.ics_qbuf);
            sscb->sscb_ics.ics_l_qbuf = 13;
	    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	    {
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }
	    break;

	case GCA_SECURE:
	{
	    GCA_TX_DATA	    *buf;

	    done = TRUE;
	    sscb->sscb_rsptype = GCA_DONE;
	    sscb->sscb_qmode = PSQ_SECURE;
	    *next_state = SCS_PROCESS;

	    /* Get the distributed transaction ID from the gca data area. */

	    buf = (GCA_TX_DATA *)rv->gca_data_area;

	    sscb->sscb_dis_tran_id.db_dis_tran_id.
                   db_ingres_dis_tran_id.db_tran_id.db_high_tran = 
			buf->gca_tx_id.gca_index[0];
	    sscb->sscb_dis_tran_id.db_dis_tran_id.
                   db_ingres_dis_tran_id.db_tran_id.db_low_tran = 
			buf->gca_tx_id.gca_index[1];
            sscb->sscb_dis_tran_id.db_dis_tran_id_type =
                                                DB_INGRES_DIS_TRAN_ID_TYPE;

	    if (print_qry)
	    {
		sc0e_trace("Prepare to Commit Statement\n");
	    }
            MEcopy("Prepare to Commit", 17,
                sscb->sscb_ics.ics_qbuf);
            sscb->sscb_ics.ics_l_qbuf = 17;
	    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	    {
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }
	    break;
	}
        case GCA_XA_SECURE:
	{
	    done = TRUE;
	    sscb->sscb_rsptype = GCA_DONE;
	    sscb->sscb_qmode = PSQ_SECURE;
	    *next_state = SCS_PROCESS;

	    /* Get the distributed transaction ID from the gca data area. */

	    if (ME_NOT_ALIGNED_MACRO((PTR)  /* if not aligned, do MEcopy  */
			rv->gca_data_area+GCA_DESC_SIZE))
	    {
		MEcopy((PTR)(rv->gca_data_area + GCA_DESC_SIZE),
			sizeof(DB_XA_EXTD_DIS_TRAN_ID),
			(PTR)&(sscb->sscb_dis_tran_id.db_dis_tran_id.db_xa_extd_dis_tran_id) ); 
	    }
	    else
	    {
		sscb->sscb_dis_tran_id.db_dis_tran_id.db_xa_extd_dis_tran_id
			= *((DB_XA_EXTD_DIS_TRAN_ID *)
			    (rv->gca_data_area + GCA_DESC_SIZE));
	    }
            sscb->sscb_dis_tran_id.db_dis_tran_id_type = DB_XA_DIS_TRAN_ID_TYPE;

	    if (print_qry)
	    {
		sc0e_trace("XA Prepare to Commit Statement\n");
	    }
            MEcopy("XA Prepare to Commit", 20,
                sscb->sscb_ics.ics_qbuf);
            sscb->sscb_ics.ics_l_qbuf = 20;
	    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	    {
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }
	    break;
	}

        case GCA_XA_START:
        case GCA_XA_END:
        case GCA_XA_PREPARE:
        case GCA_XA_COMMIT:
        case GCA_XA_ROLLBACK:
	{
	    done = TRUE;
	    sscb->sscb_rsptype = GCA_DONE;
	    *next_state = SCS_PROCESS;

	    /*
	    ** Wait to get the XID from the GCA_XA_DATA  area until
	    ** we have a place to put it, as we don't want to 
	    ** just stomp on what may be in the scb already.
	    */

	    switch (mode)
	    {
		case GCA_XA_START:
		    sscb->sscb_qmode = PSQ_GCA_XA_START;
		    if (print_qry)
			sc0e_trace("GCA xa_start\n");
		    MEcopy("GCA xa_start", 12,
			sscb->sscb_ics.ics_qbuf);
		    sscb->sscb_ics.ics_l_qbuf = 12;
		    break;
		case GCA_XA_END:
		    sscb->sscb_qmode = PSQ_GCA_XA_END;
		    if (print_qry)
			sc0e_trace("GCA xa_end\n");
		    MEcopy("GCA xa_end", 10,
			sscb->sscb_ics.ics_qbuf);
		    sscb->sscb_ics.ics_l_qbuf = 10;
		    break;
		case GCA_XA_PREPARE:
		    sscb->sscb_qmode = PSQ_GCA_XA_PREPARE;
		    if (print_qry)
			sc0e_trace("GCA xa_prepare\n");
		    MEcopy("GCA xa_prepare", 14,
			sscb->sscb_ics.ics_qbuf);
		    sscb->sscb_ics.ics_l_qbuf = 14;
		    break;
		case GCA_XA_COMMIT:
		    sscb->sscb_qmode = PSQ_GCA_XA_COMMIT;
		    if (print_qry)
			sc0e_trace("GCA xa_commit\n");
		    MEcopy("GCA xa_commit", 13,
			sscb->sscb_ics.ics_qbuf);
		    sscb->sscb_ics.ics_l_qbuf = 13;
		    break;
		case GCA_XA_ROLLBACK:
		    sscb->sscb_qmode = PSQ_GCA_XA_ROLLBACK;
		    if (print_qry)
			sc0e_trace("GCA xa_rollback\n");
		    MEcopy("GCA xa_rollback", 15,
			sscb->sscb_ics.ics_qbuf);
		    sscb->sscb_ics.ics_l_qbuf = 15;
		    break;
	    }

	    if ( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE) )
	    {
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }
	    break;
	}
	case GCA_ABORT:
	    done = TRUE;
	    sscb->sscb_rsptype = GCA_DONE;
	    sscb->sscb_qmode = PSQ_ABORT;
	    *next_state = SCS_PROCESS;
	    if (print_qry)
	    {
		sc0e_trace("Abort Statement\n");
	    }
            MEcopy("Abort", 5,
                sscb->sscb_ics.ics_qbuf);
            sscb->sscb_ics.ics_l_qbuf = 5;
	    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	    {
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }
	    break;

	case GCA_ROLLBACK:
	    done = TRUE;
	    sscb->sscb_rsptype = GCA_DONE;
	    sscb->sscb_qmode = PSQ_ROLLBACK;
	    *next_state = SCS_PROCESS;
	    if (print_qry)
	    {
		sc0e_trace("Rollback [Work] Statement\n");
	    }
            MEcopy("Rollback [work]", 15,
                sscb->sscb_ics.ics_qbuf);
            sscb->sscb_ics.ics_l_qbuf = 15;
	    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
	    {
		    sc0a_qrytext(scb, sscb->sscb_ics.ics_qbuf,
			sscb->sscb_ics.ics_l_qbuf,
			SC0A_QT_START|SC0A_QT_END);
	    }
	    break;

	case GCA_ATTENTION:
	    scs_i_interpret(scb);
	    done = TRUE;
            sscb->sscb_dmm = 0;
	    sscb->sscb_rsptype = GCA_IACK;
	    if (print_qry)
	    {
		sc0e_trace("Interrupt Requested\n");
	    }
	    break;

	case GCA_CREPROC:
	case GCA_DRPPROC:
	case GCA_DEFINE:    /* These are sent for RPQ's and DBP's.	    */
			    /* Processing is identical as for reqular qry's */
	case GCA_QUERY:
	{
	    i4			gdarea_size = 0;
	    GCA_Q_DATA		*msg;

	    /*
	    ** The cursor name must be normalized and case translated
	    ** when appropriate.  Since the sequencer cannot determine
	    ** when appropriate, this translation is done in PSF.
	    */
            regular_query = 1;
	    msg = (GCA_Q_DATA *)rv->gca_data_area;

	    /*
	    ** When using DMF RAAT "queries," QSF should be avoided.
	    ** Just break out of here.  The data will be read directly from
	    ** the cscb.
	    */
	    if ((msg->gca_language_id == DB_NDMF &&
		 sscb->sscb_thandle == 0) ||
		 sscb->sscb_raat_buffer_used != 0)
	    {
#if defined(RAAT_SUPPORT)
                scs_raat_load(scb); /* save buffers in link list until done */
		ret_val = E_DB_OK;
		break;
#else
		sc0e_0_put(E_SC0024_INTERNAL_ERROR, 0);
		sc0e_put(E_SC0393_RAAT_NOT_SUPPORTED, 0, 0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		ret_val = E_DB_ERROR;
#endif /* RAAT_SUPPORT */
	    }

	    if (sscb->sscb_thandle == 0) 
	    {
		qlang = msg->gca_language_id;
		if (qlang)
		    sscb->sscb_ics.ics_qlang = qlang;
		modifiers = msg->gca_query_modifier;
		sscb->sscb_opccb->opf_names = modifiers & (GCA_NAMES_MASK | GCA_COMP_MASK);

		if (modifiers & GCA_Q_NO_XACT_STMT)
		    sscb->sscb_req_flags |= SCS_NO_XACT_STMT;

		if (modifiers & GCA_LOCATOR_MASK)
		{
		    sscb->sscb_opccb->opf_locator = 1;
		    sscb->sscb_psccb->psq_flag2 |= PSQ_LOCATOR;
		}
	        else
		{
		    sscb->sscb_opccb->opf_locator = 0;
		    sscb->sscb_psccb->psq_flag2 &= ~PSQ_LOCATOR;
		}
		if (modifiers & GCA_REPEAT_MASK)
		    sscb->sscb_req_flags |= SCS_REPEAT_QUERY;

		gdarea_size = sizeof(msg->gca_language_id)
				+ sizeof(msg->gca_query_modifier);

		for (;;)
		{
		    /* allocate the object, store the handle, and move it in */
		    qs_ccb = sscb->sscb_qsccb;
		    qs_ccb->qsf_obj_id.qso_type = QSO_QTEXT_OBJ; /* saving the query text */
		    qs_ccb->qsf_obj_id.qso_lname = 0;	/* object has no name */
		    status = qsf_call(QSO_CREATE, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
				E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }
		    sscb->sscb_thandle = qs_ccb->qsf_obj_id.qso_handle;
		    sscb->sscb_tlk_id = qs_ccb->qsf_lk_id;
		    sscb->sscb_tsize =
			    qs_ccb->qsf_sz_piece =
				((qry_size > SCS_TEXT_DEFAULT) ?
				    qry_size * 2 : SCS_TEXT_DEFAULT);
		    status = qsf_call(QSO_PALLOC, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }
		    sscb->sscb_troot = qs_ccb->qsf_root = qs_ccb->qsf_piece;
		    status = qsf_call(QSO_SETROOT, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code, 
				E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			ret_val = E_DB_ERROR;
			break;
		    }
		    sscb->sscb_dnleft = 0;
		    sscb->sscb_dptr = 0;
		    sscb->sscb_tnleft = sscb->sscb_tsize
							- sizeof(PSQ_QDESC);
		    sscb->sscb_tptr = 
			((PSQ_QDESC *) sscb->sscb_troot)->psq_qrytext =
			    ((char *) sscb->sscb_troot)
							+ sizeof(PSQ_QDESC);
		    /* 
		    ** Init query text memory since scs_blob_fetch may
		    ** look at it before it is initialized.
		    ** (LIBQ might send blob data before the query text)
		    */
	    	    MEfill(8, 0xFE, sscb->sscb_tptr);
		    ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrysize = 0;
		    ((PSQ_QDESC *) sscb->sscb_troot)->psq_datasize = 0;
		    ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrydata = 0;
		    ((PSQ_QDESC *) sscb->sscb_troot)->psq_dnum = 0;
		    break;
		}
		if (DB_FAILURE_MACRO(status))
		    break;
	    }

	    ret_val = 
		scs_fetch_data(scb,
			1 /* for psf */,
			rv->gca_d_length - gdarea_size,
			rv->gca_data_area + gdarea_size,
			0, &ad_peripheral);

	    /* Define a Repeat Query, NO BLOB Params */
	    if (mode == GCA_DEFINE && ad_peripheral == TRUE)
	    {
		sc0e_0_uput(E_SC0132_LO_REPEAT_MISMATCH, 0);
		sc0e_0_put(E_SC0132_LO_REPEAT_MISMATCH, 0);
		ret_val = E_DB_ERROR;
		break;
	    }

	    break;
	}

	case GCA_RELEASE:
	    done = TRUE;
	    *next_state = SCS_TERMINATE;
	    ret_val = E_DB_WARN;
	    if (print_qry)
	    {
		sc0e_trace("Session Disconnect Requested\n");
	    }
	    break;

	case GCA_CDATA:
	    sc0e_0_put(E_SC0250_COPY_OUT_OF_SEQUENCE, 0);
	    sc0e_0_uput(E_SC0250_COPY_OUT_OF_SEQUENCE, 0);
	    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	    ret_val = E_DB_ERROR;
	    break;

	case GCA_MD_ASSOC:
        {
	    i4			    flag_value;
	    GCA_SESSION_PARMS	    *buf;

	    buf = (GCA_SESSION_PARMS *)rv->gca_data_area;
	    if (buf->gca_l_user_data != 1)
	    {
		sc0e_put(E_SC0230_INVALID_CHG_PROT_LEN, 0, 1,
			 sizeof(buf->gca_l_user_data),
			 (PTR)&buf->gca_l_user_data,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		sc0e_uput(E_SC0230_INVALID_CHG_PROT_LEN, 0, 1,
			  sizeof(buf->gca_l_user_data),
			  (PTR)&buf->gca_l_user_data,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			      0, (PTR)0,
			  0, (PTR)0);
		sc0e_0_put(E_SC022B_PROTOCOL_VIOLATION, 0);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		return(E_DB_SEVERE);
	    }
	    else if (buf->gca_user_data[0].gca_p_index != GCA_SUPSYS)
	    {
		sc0e_put(E_SC0231_INVALID_CHG_PROT_CODE, 0, 1,
			 sizeof(buf->gca_user_data[0].gca_p_index),
			 (PTR)&buf->gca_user_data[0].gca_p_index,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			     0, (PTR)0,
			 0, (PTR)0);
		sc0e_uput(E_SC0231_INVALID_CHG_PROT_CODE, 0, 1,
			  sizeof(buf->gca_user_data[0].gca_p_index),
			  (PTR)&buf->gca_user_data[0].gca_p_index,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
		sc0e_0_put(E_SC022B_PROTOCOL_VIOLATION, 0);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		return(E_DB_SEVERE);
	    }
	    MEcopy(&buf->gca_user_data[0].gca_p_value.gca_value[0],
			       sizeof(flag_value),
			       (PTR)&flag_value);
	    if (flag_value == GCA_ON)
	    {
		if (sscb->sscb_flags & SCS_STAR)
		{
		    sscb->sscb_qeccb->qef_r3_ddb_req.qer_d11_modify.dd_m2_y_flag = DD_1MOD_ON;
		}
		*next_state = SCS_ADDPROT;
	    }
	    else
	    {
		if (sscb->sscb_flags & SCS_STAR)
		{
		    sscb->sscb_qeccb->qef_r3_ddb_req.qer_d11_modify.dd_m2_y_flag = DD_2MOD_OFF;
		}
		*next_state = SCS_DELPROT;
	    }
	    sscb->sscb_rsptype = GCA_ACCEPT;
	    done = TRUE;
	    break;
	}

	default:
	    sc0e_put(E_SC0226_BAD_GCA_TYPE, 0, 1,
		     sizeof(mode), (PTR)&mode,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_uput(E_SC0226_BAD_GCA_TYPE, 0, 1,
		      sizeof(mode), (PTR)&mode,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	    return(E_DB_SEVERE);
        }
    }
    /*
    ** If doing querytext auditing make sure we end and flush here.
    ** This shouln't be necessary except  for error conditions
    ** where  we just break out part way through (e.g. with
    ** INVPROC)
    */
    if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
    {
	sc0a_qrytext(scb, "", 0, SC0A_QT_END);
    }

    if (done)
    {
	QEU_CB		    *qeu;

	/* If cur-amem is non-null here, it must be a QEU CB allocated to
	** create a transaction to allow couponifying inbound blobs.
	** Since the blobs are in the bag now, end the transaction and
	** toss the QEU CB.
	*/
	if ( (qeu = (QEU_CB *) cquery->cur_amem) != NULL)
	{
	    qeu->qeu_qmode = sscb->sscb_qmode;
	    status = qef_call(QEU_ETRAN, qeu);
	    if (status)
	    {
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		sc0e_0_put(qeu->error.err_code, 0);
		if (status > ret_val)
		    ret_val = status;
	    }
	    sc0m_deallocate(0, (PTR *)&cquery->cur_amem);
	    cquery->cur_qef_data = NULL;
	}
	/*
	** In the case of batch processing, eating input does not rise from
	** an input error, we just want to consume the rest of the input
	** when a previous query in the batch has caused an error that
	** requires termination of group processing. So only
	** set SCS_IERROR if if not an eog error.
	*/
	if (sscb->sscb_req_flags & SCS_EAT_INPUT_MASK &&
		!cscb->cscb_eog_error)
	{
	    *next_state = SCS_IERROR;
	    sscb->sscb_dmm = 0;
	}
	else if (sscb->sscb_dmm)
	{	
	    sc0e_put(E_SC022C_PREMATURE_MSG_END, 0, 2,
		     sizeof(rv->gca_message_type),
		     (PTR)&rv->gca_message_type,
		     sizeof(sscb->sscb_dmm),
		     (PTR)&sscb->sscb_dmm,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    sc0e_0_put(E_SC022B_PROTOCOL_VIOLATION, 0);
	    sscb->sscb_dmm = 0;
	    ret_val = E_DB_ERROR;
	}
        else if ((sscb->sscb_ics.ics_l_qbuf == 0)
            && (regular_query)
            && (sscb->sscb_thandle)
            && (sscb->sscb_troot)
            )
        {
            MEcopy((PTR) ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrytext,
                    min(sizeof(sscb->sscb_ics.ics_qbuf),
                        ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrysize),
                    (PTR) (sscb->sscb_ics.ics_qbuf));
            scb->cs_scb.cs_diag_qry = sscb->sscb_ics.ics_qbuf;
            sscb->sscb_ics.ics_l_qbuf =
                    min(sizeof(sscb->sscb_ics.ics_qbuf),
                        ((PSQ_QDESC *) sscb->sscb_troot)->psq_qrysize);

# ifdef QUERY_TRACE
		TRdisplay("%@      %x       %t\n", scb->cs_scb.cs_self, 
		sscb->sscb_ics.ics_l_qbuf, 
		sscb->sscb_ics.ics_qbuf);
# endif /*QUERY_TRACE*/


        }
    }
    if (ret_val)
    {
	/*
	** then some error occured during the above processing.
	** Destroy the stream, and return scs_input
	*/
	if (sscb->sscb_thandle)
	{
	    qs_ccb = sscb->sscb_qsccb;
	    qs_ccb->qsf_obj_id.qso_handle = sscb->sscb_thandle;
	    qs_ccb->qsf_lk_id = sscb->sscb_tlk_id;
	    status = qsf_call(QSO_DESTROY, qs_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code, 
		    E_SC021C_SCS_INPUT_ERROR);
	    }
	    sscb->sscb_thandle = 0;
	}
	/* else nothing to release */
	if (*next_state != SCS_TERMINATE)
	    *next_state = SCS_INPUT;

	if ( ((sscb->sscb_req_flags & SCS_EAT_INPUT_MASK) != 0)
	    && (ret_val <= E_DB_ERROR))
	{
	    /* 
	    ** Then the error isn't too bad, and, but setting
	    ** EAT_INPUT, the system code has indicated that it is
	    ** reasonable to handle the error directly.  That being
	    ** the case, we set the return to OK, allowing the outer
	    ** layers to continue.
	    */

	    ret_val = E_DB_OK;
	}
    }
    if (cscb->cscb_gci.gca_end_of_group &&
	    sscb->sscb_interrupt == 0)
	/* got to the end of the group, re-set error */
	cscb->cscb_eog_error = FALSE;
    if (*next_state == 0)
    {
	if (done)
	    *next_state = SCS_CONTINUE;
	else
	    *next_state = SCS_INPUT;
    }

    return(ret_val);
}

/*{
** Name: scs_qef_error	- Process error from QEF
**
** Description:
**      This routine processes errors from QeF.  If the error is the 
**      fault of the caller, then the error_blame parameter is used 
**      to identify the caller in the error log.
**
** Inputs:
**      status                          DB_STATUS returned by qsf
**      err_code                        error code "	   "  "
**      error_blame                     Error to log if the error is not
**                                      qef's fault
**	scb				Session control block
**
** Outputs:
**      none
**	Returns:
**	    none
**	Exceptions:
**	    none	    
**
** Side Effects:
**	    none
**
** History:
**      27-Mar-1987 (fred)
**          Created on Jupiter
**	2-Jul-1993 (daveb)
**	    prototyped.
**      21-Jul-1999 (wanya01)
**          Send E_SC0206_CANNOT_PROCESS to FE. (Bug 98017)
**	23-Jun-2010 (kschendel) b123775
**	    Handle E_QE0122_ALREADY_REPORTED as if it were QE0025, i.e. just
**	    eat it.  QEF usually translates QE0122 to QE0025, but some
**	    cases slip through, and it's simpler to fix here than in QEF. :-)
**      02-Sep-2010 (maspa05) sir 124346
**          Save error code for SC930 output
**      20-sep-2010 (stephenb)
**          QEF may return E_DM006A_TRAN_ACCESS_CONFLICT when update
**          operations are attempted in a read-only transaction or database.
**          This should not cause vague SCF errors in the log; add a case
**          for it here.
*/
VOID
scs_qef_error(DB_STATUS status,
	      i4 err_code,
	      i4 error_blame,
	      SCD_SCB *scb )
{
    scd_note(status, DB_QEF_ID);

    if (err_code == E_QE0025_USER_ERROR || err_code == E_QE0122_ALREADY_REPORTED
		|| err_code == E_QE0015_NO_MORE_ROWS)
    {
	/*
	** These are user errors and
	** should have been returned as such
	**
	** However save the error_blame error code for SC930 tracing
	** unless we've already got an error code - in which case assume 
	** it's more meaningful and keep it
	*/

        if (scb->cs_scb.cs_sc930_err==0)
	    scb->cs_scb.cs_sc930_err=error_blame;


    }
    else if ((err_code == E_QE000D_NO_MEMORY_LEFT)
	|| (err_code == E_QE000E_ACTIVE_COUNT_EXCEEDED))
    {
	sc0e_0_put(err_code, 0);
	sc0e_0_uput(err_code, 0);
    }
    else if (err_code == E_QE000C_CURSOR_ALREADY_OPENED)
    {
	sc0e_0_uput(err_code, 0);
    }
    else if (	(err_code == E_QE0017_BAD_CB)
	||	(err_code == E_QE0018_BAD_PARAM_IN_CB)
	||	(err_code == E_QE0014_NO_QUERY_PLAN)
       )
    {
	sc0e_0_put(E_SC0024_INTERNAL_ERROR, 0);
	sc0e_0_put(err_code, 0);
	sc0e_0_put(error_blame, 0);
	sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
	sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
    }
    else if ((err_code == E_QE0022_QUERY_ABORTED)
			&& (!scb->scb_sscb.sscb_interrupt))
    {
	sc0e_0_uput(err_code, 0);
	scb->scb_sscb.sscb_force_abort = SCS_FAPENDING;
	if (scb->scb_cscb.cscb_batch_count > 0)
	    scb->scb_cscb.cscb_eog_error = TRUE;
    }
    else if (err_code == E_DM006A_TRAN_ACCESS_CONFLICT)
    {
	/* 
	** update operation attempted in a read-only
	** transaction or database. QEF has already raised a user
	** error through SCC_ERROR, so report the original error
	** message in the log rather than falling through to vague
	** QEF error message below
	*/
	sc0e_0_put(err_code, 0);
	sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
	sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
    }
    else if (scb->scb_sscb.sscb_flags & SCS_STAR)
    {
	/*
	** IN star QEF can handle all of its errors
	** hm....
	*/
	sc0e_0_put(err_code, 0);
	sc0e_0_uput(err_code, 0);
    }
    else
    {
	/* anything else is qef's fault */
	sc0e_0_put(E_SC0216_QEF_ERROR, 0);
	sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
        sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
    }
}

/*{
** Name: scs_qsf_error	- Process error from QSF
**
** Description:
**      This routine processes errors from QSF.  If the error is the 
**      fault of the caller, then the error_blame parameter is used 
**      to identify the caller in the error log.
**
** Inputs:
**      status                          DB_STATUS returned by qsf
**      err_code                        error code "	   "  "
**      error_blame                     Error to log if the error is not
**                                      qsf's fault
**
** Outputs:
**      none
**	Returns:
**	    none
**	Exceptions:
**	    none	    
**
** Side Effects:
**	    none
**
** History:
**      23-Jul-1986 (fred)
**          Created on Jupiter
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
VOID
scs_qsf_error(DB_STATUS status,
	      i4 err_code,
	      i4 error_blame )
{
    if (err_code == E_QS0010_NO_EXLOCK)
    {
	sc0e_0_put(E_SC020F_LOST_LOCK, 0);
	sc0e_0_put(err_code, 0);
	sc0e_0_put(error_blame, 0);
	scd_note(status, DB_SCF_ID);
    }
    else if (	(err_code == E_QS000F_BAD_HANDLE)
	    ||  (err_code == E_QS0014_EXLOCK)
	    ||  (err_code == E_QS0015_SHLOCK)
	    ||  (err_code == E_QS0016_BAD_LOCKSTATE)
	    ||	(err_code == E_QS0009_BAD_OBJTYPE)	)
    {
	sc0e_0_put(E_SC0024_INTERNAL_ERROR, 0);
	sc0e_0_put(err_code, 0);
	sc0e_0_put(error_blame, 0);
	scd_note(status, DB_SCF_ID);
    }
    else /* all others are qsf's problem */
    {
	sc0e_0_put(E_SC0217_QSF_ERROR, 0);
	sc0e_0_put(err_code, 0);
	sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
	scd_note(status, DB_QSF_ID);
    }
}

/*{
** Name: scs_gca_error	- Process error from GCA
**
** Description:
**      This routine processes errors from GSA.  If the error is the 
**      fault of the caller, then the error_blame parameter is used 
**      to identify the caller in the error log.
**
** Inputs:
**      status                          DB_STATUS returned by gca
**      err_code                        error code "	   " 
**      error_blame                     Error to log if the error is not
**                                      gca's fault
**
** Outputs:
**      none
**	Returns:
**	    none
**	Exceptions:
**	    none	    
**
** Side Effects:
**	    none
**
** History:
**      17-Jul-1987 (fred)
**          Created on Jupiter
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
VOID
scs_gca_error(DB_STATUS status,
	      i4 err_code,
	      i4 error_blame )
{
	sc0e_0_put(err_code, 0);
	sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
	scd_note(E_DB_SEVERE, DB_GCF_ID);
}

/*
** {
** Name: scs_blob_fetch	- Build a large object parameter
**
** Description:
**      This routine builds a parameter whose type is that of a large object.
**	It is primarily an interface point from SCF to adu_couponify().
**
**	For large objects, SCF will always ask ADF to build a coupon to carry
**	around.
**
**	Blobs may go on and on, so it might take multiple calls to
**	scs-blob-fetch to get the entire blob couponified.  The
**	sscb_dmm indicator says whether this is the first call for
**	a given blob or not.  If couponification was not completed,
**	we'll return E_DB_INFO to tell the caller that more data
**	is needed.
**
**	The blob data has to land somewhere, and if we can't figure out
**	where, it will go into a holding temp.  If this is the
**	first call for the blob, we'll expend a little effort to try
**	to figure out where the blob is going.  (e.g. for COPY, we
**	have enough context to know.  For some API queries such as
**	execute prepared query or insert, we might be able to figure
**	it out.  For traditional LIBQ queries which send the parameters
**	before the query, it's hopeless.)  The blob context info
**	goes into the DB_BLOB_WKSP that is passed along to the
**	couponifier (and ultimately to DMF/dmpe).  IMPORTANT:  the first
**	couponify call might not have enough data available to even make
**	it to DMF, so the DB_BLOB_WKSP has to be passed along for each
**	couponify (re-)call until the blob is definitely done.
**
** Inputs:
**      scb                             Session Control Block
**      qry_size                        Address of size of current buffer
**      buffer                          Pointer to current buffer
**      coupon_dv                       Ptr to DB_DATA_VALUE to receive
**					    the coupon being created.
**	qeu_copy			If COPY FROM, pointer to QEU_COPY;
**					if not COPY, pass NULL.
**
** Outputs:
**      *qry_size                       Updated to reflect amount of buffer used
**      *coupon_dv->db_data             Coupon created here.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      17-may-1990 (fred)
**          Created.
**	05-oct-1992 (fred)
**	    Store blob pointer in amem since its a pointer.  Previous, this
**	    was stored in a bool -- which didn't work too well.
**      07-Jan-1993 (fred)
**          Altered setup and call sequence for adu_couponify() in
**          conjunction with general cleanup of large object code.
**  	23-Feb-1993 (fred)
**  	    Added `from_copy' parameter.  This omits certain
**          processing that is necessary only when we are between
**          queries.  For the case where this is called from within a
**          copy statement, this processing is not necessary.  In
**          fact, it is, in most cases, an error.  Also, altered to
**          call scs_lowksp_alloc() to obtain the workspace.
**      13-Apr-1993 (fred)
**          Fixed bug where this routine was masking an incomplete
**          status by resetting status to OK when it was E_DB_INFO.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation.
**      18-Oct-1993 (fred)
**          Added code to generate a "nice" error when called from
**          STAR.  Large objects cannot currently be input into STAR
**          because STAR lacks an ldb DMF interface in which to store
**          them. 
**      10-Nov-1993 (fred)
**          Passed from_copy parameter on to scs_lowksp_alloc()
**          routine to indicate that large objects munged form this
**          call are related to COPY processing.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for owner parameter to sc0m_allocate which has
**	    changed type to PTR.
**      13-Apr-1994 (fred)
**          If adu_couponify() fails, set SCS_EAT_INPUT mode flag.
**          This will cause the system to flush the remainder of the
**          query input, then return an error.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	nov-98,feb-mar-99 (stephenb)
**	    Blob performance enhancements. If the target table has only one
**	    blob column, then we can load the data directly into the etab,
**	    saving the overhead of creating an extra temp table.
**	1-apr-99 (stephenb)
**	    resize data-type width after coercion.
**	7-apr-99 (stephenb)
**	    Init pop_info field
**	13-apr-1999 (somsa01)
**	    When coerced to a null and an add extra byte needs to be added,
**	    initialize the null indicator bit.
**      30-jul-1999 (stial01)
**          scs_blob_fetch() Extended blob performance enhancements for
**          COPY and also for INSERT INTO ... (). (insert with column list)
**      12-aug-1999 (stial01)
**          scs_blob_fetch() Always call adu_couponify with pop_info set,
**          whether or not we know the destination table
**          Remove optimization for COPY, as we might be doing LOAD without
**          logging
**	20-oct-99 (stephenb)
**	    Updates to blob optimization: 
**		don't use the optimization if
**	    	we find a function in the input query because we can't do
**	    	the corecion in DMF.
**
**		Failure when using the blob optimization is fatal, and
**	        should reset dmm. Make sure we do that.
**	10-May-2004 (schka24)
**	    Remove transaction, session stuff from pop info.
**	31-Mar-2010 (kschendel) SIR 123485
**	    Pass COPY cb instead of just a flag, so that we can fill in
**	    blob copy optim data for the copy.
**	    Execute of prepared statement has table ID now, easier on dmpe.
**	    "Blob workspace" is always allocated with the lowksp now.
**	26-May-2010 (kschendel) b123798
**	    Repair minor goofs in INSERT detection, need to pass a blank-
**	    padded table name to DMF.
**	    (Later) Having actually gotten blob put optimization working,
**	    turn it off.  :-(  OPF likes to compile in LVCH-to-LVCH
**	    coercions, apparently for little or no reason (nullability
**	    difference is one reason I found).  If the input to dmpe_move
**	    is a "final" optimized coupon, things fall apart, because
**	    data ends up in the etab twice with the same sequence number.
**	    Turn off optimization until we can convince OPF to not do that.
**	    (blob optim for COPY can remain, as opf doesn't get its grubby
**	    paws on it.)
*/
DB_STATUS
scs_blob_fetch(SCD_SCB *scb,
                DB_DATA_VALUE  *coupon_dv,
                i4  *qry_size,
                char **buffer,
                QEU_COPY *qeu_copy )
{
    DB_STATUS           status;
    ADP_LO_WKSP		*lowksp;
    DB_DATA_VALUE	segment_dv;
    SCS_CURRENT		*cquery;	/* sscb_cquery inside scb */
    SCS_SSCB		*sscb;		/* Convenience: scb->scb_sscb */
    SC0M_OBJECT		*object = 0;
    PTR			block;
    char		*query;
    bool		name_found = FALSE;
    bool		owner_found = FALSE;
    bool		no_func = FALSE;
    DB_BLOB_WKSP	*blob_info;
    
    sscb = &scb->scb_sscb;
    cquery = &sscb->sscb_cquery;
    if (sscb->sscb_flags & SCS_STAR)
    {
	sc0e_0_uput(E_SC0131_LO_STAR_MISMATCH, 0);
	sscb->sscb_req_flags |= SCS_EAT_INPUT_MASK;
	return(E_DB_WARN);
    }

    for (;;)
    {
	/*
	**  If this is the first blob ever encountered by this session,
	**  allocate a large object workspace to allow ADF to
	**  process any other blobs encountered.  Since most sessions don't
	**  use blobs, this space is allocated only when necessary.
	**  If the workspace is already there, partially re-init it.
	*/

	status = scs_lowksp_alloc(scb, (PTR *) &lowksp);
	if (status)
	    break;

	blob_info = (DB_BLOB_WKSP *)(lowksp->adw_fip.fip_pop_cb.pop_info);

	/* If first blob this query, and no qeu-cb allocated, allocate it
	** and start a transaction so that we can write the blob data
	** into (real or holding) etabs.  We must be coming from scs-input
	** and the qeu-cb will be deallocated when scs-input is done.
	**
	** COPY doesn't do this, the copy is already started in QEF and
	** no separate transaction is needed.
	*/
	if (qeu_copy == NULL && cquery->cur_amem == NULL)
	{
	    QEU_CB		*qeu;

	    status = sc0m_allocate(0,
		    sizeof(QEU_CB),
		    DB_SCF_ID,
		    (PTR)QEUCB_CB,
		    QEUCB_ASCII_ID,
		    &block);
	    qeu = (QEU_CB *)block;
	    qeu->qeu_type = QEUCB_CB;
	    qeu->qeu_ascii_id = QEUCB_ASCII_ID;
	    /* First, we start an xact to manage the work */
	    qeu->qeu_eflag = QEF_EXTERNAL;
	    qeu->qeu_db_id = (PTR) sscb->sscb_ics.ics_opendb_id;
	    qeu->qeu_d_id = scb->cs_scb.cs_self;
	    qeu->qeu_flag = 0;
	    status = qef_call(QEU_BTRAN, qeu);
	    if (status)
	    {
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		sc0e_0_put(qeu->error.err_code, 0);
		sc0m_deallocate(0, (PTR *)&qeu);
		break;
	    }
	    /* Record so that scs-input sees we started a transaction and
	    ** ends it before the real query starts...
	    */
	    cquery->cur_amem = (PTR) qeu;
	}

	/*
	** If it's the first time for this blob, we may want to parse
	** the passed-in query text (if any!) 
	*/
	query = ((PSQ_QDESC *)sscb->sscb_troot)->psq_qrytext;

#ifdef xDEBUG
	if (sscb->sscb_dmm == 0)
	    TRdisplay("scs_blob_fetch psq_qry_size = %d\n", 
		((PSQ_QDESC *)sscb->sscb_troot)->psq_qrysize);
#endif

	if (sscb->sscb_dmm == 0)
	{
	    /* First time thru for this blob, see if we can give DMF some
	    ** direction as to where the blob should land.
	    */
	    blob_info->flags = 0;	/* Assume not */
	    lowksp->adw_fip.fip_pop_cb.pop_temporary = ADP_POP_TEMPORARY;
	    if (qeu_copy != NULL)
	    {
		/* COPY FROM can supply all the necessary info for DMF,
		** so that DMF can store directly into the etab.  (Unless
		** it's a bulk-load, in which case DMF does even better things,
		** but that isn't our problem here.)
		** Note that the base-attid is one origin.
		*/
		blob_info->access_id = qeu_copy->qeu_access_id;
		blob_info->base_attid = qeu_copy->qeu_cp_cur_att+1;
		blob_info->flags = BLOBWKSP_ACCESSID | BLOBWKSP_ATTID;
	    }
#if 0
/* ************* START OF DISABLED CODE
**	Having actually gotten blob put optimization working,
**	turn it off.  :-(  OPF likes to compile in LVCH-to-LVCH
**	coercions, apparently for little or no reason (nullability
**	difference is one reason I found).  If the input to dmpe_move
**	is a "final" optimized coupon, things fall apart; dmpe_move copies
**	the LOB to a temp (*), and the final put copies it back, and now
**	we have the data in the etab twice (with the same sequence number).
**	Turn off optimization until we can convince OPF to not do that.
**	(blob optim for COPY can remain, as opf doesn't get its grubby
**	paws on it.)
**
**	Please note that simply "fixing" opc to pass most put-optimized
**	LOB's directly to DMF is not good enough.  We must *guarantee*
**	that no coercion is compiled in, and that probably involves
**	tagging the statement in some manner to say that coercion is
**	disallowed.  (Or, in the execute prepared statement case, the
**	already-compiled statement would have to be tagged to show that
**	there is no coercion in the QP.)  Yet another alternative would be
**	for DMF (dmpe_move) to detect the situation, somehow, magically,
**	and simply copy the coupon instead of moving the data.
**
**	note (*): actually, dmpe_move falls over because the input put-
**	optimized coupon doesn't have the proper "get" information, but
**	if that were to be fixed, double data would be the result.
**
**	THE END OF THE #if 0 IS ABOUT 200 LINES DOWN ...
*/
	    else if (STncasecmp(query, "execute ", 8) == 0 &&
		    STncasecmp(query + 8, "procedure ", 11 != 0))
	    {
		char        buf[DB_MAXNAME + 1];
		char	*pos = query + 8;
		i4		offset = 0;
		i4		stmt_mode;
		PSQ_CB	psq_cb;
		DB_STATUS   psf_status;	    
		PSQ_STMT_INFO   *stmt_info = NULL;

		for (;*pos != ' ' && offset <= DB_MAXNAME; pos++, offset++)
		    buf[offset] = *pos;
		buf[offset] = '\0';
		CVlower(buf);

		psq_cb.psq_type = PSQCB_CB;
		psq_cb.psq_length = sizeof(psq_cb);
		psq_cb.psq_ascii_id = PSQCB_ASCII_ID;
		psq_cb.psq_owner = (PTR) DB_SCF_ID;
		psq_cb.psq_sessid = scb->cs_scb.cs_self;
		psq_cb.psq_qid = buf;
		psf_status = psq_call(PSQ_GET_STMT_INFO, &psq_cb, NULL);
		stmt_mode = psq_cb.psq_ret_flag;

#ifdef xDEBUG
		TRdisplay("scs_blob_fetch stmt %x %s stmt_mode %d psf_status %d\n", 
			stmt_info, buf, stmt_mode, psf_status);
#endif
		if (psf_status == E_DB_OK && 
			    (stmt_mode == PSQ_APPEND || stmt_mode == PSQ_REPCURS))
		{
		    stmt_info = psq_cb.psq_stmt_info;
		    if (stmt_info && stmt_info->psq_stmt_blob_cnt == 1
		      && stmt_info->psq_stmt_tabid.db_tab_index == 0)
		    {
			blob_info->base_id = stmt_info->psq_stmt_tabid.db_tab_base;
			blob_info->source_dt = sscb->sscb_gcadv.gca_type;
			blob_info->flags = (BLOBWKSP_TABLEID | BLOBWKSP_ATTID | BLOBWKSP_COERCE);
			blob_info->base_attid = stmt_info->psq_stmt_blob_colno;
		    }
		}
#ifdef xDEBUG
		if (stmt_info)
		    TRdisplay("%s optimize insert/update (%d) for (%d,%d) (%d,%d)\n",
			blob_info->flags ? "can" : "can't", stmt_mode, 
			stmt_info->psq_stmt_tabid.db_tab_base,
			stmt_info->psq_stmt_tabid.db_tab_index, 
			stmt_info->psq_stmt_blob_cnt,
			stmt_info->psq_stmt_blob_colno);
		else
		    TRdisplay("CANT optimize insert/update for '%s'\n", buf); 
#endif
	    }

	    else if (STncasecmp(query, "insert into ", 12) == 0)
	    {
		char	*name_start, *name_end, *pos, *putter;
		char	buf[DB_MAXNAME + 1];
		i4		offset = 0;
		i4          skip;

#ifdef xDEBUG
		TRdisplay("DEBUG %~t\n" , 
			((PSQ_QDESC *)sscb->sscb_troot)->psq_qrysize,
			query);
#endif
	    
		if (*query == 'i' || *query == 'I')
		    skip = 11;
		/* query is realy "insert into", find table name */
		MEfill(DB_OWN_MAXNAME, ' ', blob_info->table_owner.db_own_name);
		for (query += skip; *query == ' '; query++); /* skip blanks */
		pos = query;
		for (;;)
		{
		    /* check for delimited name */
		    if (*pos == '"') /* delimited name */
		    {
			name_start = ++pos;
			putter = &buf[0];
			for (;;)
			{
			    for (;*pos != '"' && offset <= DB_MAXNAME; 
				 pos++, offset++)
				*putter++ = *pos;
			    if (offset > DB_MAXNAME)
				break; /* can't find quote, bail here */
			    if (*(pos+1) == '"') /* quoted quote, skip it */
			    {
				*putter++ = '"';	/* Just one */
				pos++;
				offset++;
				continue;
			    }
			    else
			    {
				name_found = TRUE;
				break;
			    }
			}
			if (!name_found)
			    break; /* name not found, bail */
			/* skip spaces */
			for (;*pos == ' ' && offset <= DB_MAXNAME; pos++, offset++);
			/* check for owner name */
			if (offset < DB_OWN_MAXNAME && !owner_found && *pos == '.')
			{
			    owner_found = TRUE;
			    name_found = FALSE;
			    pos++;
			}
			/* ditch trailing spaces */
			--putter;
			while (*putter == ' ' && putter >= &buf[0])
			    --putter;
			*++putter = EOS;
		    }
		    else
		    {
			name_start = pos;
			while (*pos != ' ' && *pos != '.' && *pos != '(' && offset <= DB_MAXNAME)
			{
			    ++pos;  ++offset;
			}
			if (offset > DB_MAXNAME)
			    break; /* can't find space, bail here */
			else
			{
			    name_found = TRUE;
			    name_end = pos;
			}
			if (!owner_found && *pos == '.')
			{
			    owner_found = TRUE;
			    name_found = FALSE;
			    pos++;
			}
			MEcopy(name_start, name_end-name_start, buf);
			buf[name_end-name_start] = EOS;
		    }
		    if (!name_found && owner_found)
		    {

			if (sscb->sscb_ics.ics_dbserv & DU_NAME_UPPER)
			    CVupper(buf);
			else
			    CVlower(buf);
			MEmove(STlength(buf), buf, ' ',
				DB_OWN_MAXNAME, blob_info->table_owner.db_own_name);
			continue;
		    }
		    else if (name_found)
		    {
			if (sscb->sscb_ics.ics_dbserv & DU_NAME_UPPER)
			    CVupper(buf);
			else
			    CVlower(buf);
			MEmove(STlength(buf), buf, ' ',
				DB_TAB_MAXNAME, blob_info->table_name.db_tab_name);
		    }
		    else
			break;
		    /* found name, check for keyword "values" */
		    /* skip spaces */
		    for (;*pos == ' ' && offset <= DB_MAXNAME; pos++, offset++);
		    /* find "values" */
		    for (;(*pos != 'v' || *pos != 'V') && *(pos-1) != ' ' &&
			 STbcompare("values", 6, pos, 6, TRUE) && *pos != EOS;
			 pos++);
		    if (*pos == EOS)
			break;
		    /* pos must be "values", skip it */
		    pos += 6;
		    /* find paren. */
		    for (;*pos != '(' && *pos != EOS; pos++);
		    if (*pos == EOS)
			break;
		    /* skip paren */
		    pos++;
		    /*
		    ** if we find another paren after this, we have a function,
		    ** we can't deal with this so bail
		    */
		    for (;*pos != '(' && *pos != EOS; pos++);
		    if (*pos == EOS)
			no_func = TRUE;
		    else
		    {
			/* just to be safe */
			blob_info->table_owner.db_own_name[0] = EOS;
			blob_info->table_name.db_tab_name[0] = EOS;
		    }
		    break;
		}
		if (no_func && name_found)
		{
		    blob_info->flags = BLOBWKSP_TABLENAME | BLOBWKSP_COERCE;
		    blob_info->source_dt = sscb->sscb_gcadv.gca_type;
		}		
	    }
#endif
/* ********* END OF #if 0 from way above */
	} /* dmm == 0 */

	segment_dv.db_datatype = sscb->sscb_gcadv.gca_type;
	segment_dv.db_prec = sscb->sscb_gcadv.gca_precision;
	segment_dv.db_length = *qry_size;
	segment_dv.db_data = (PTR) *buffer;
	status = adu_couponify(sscb->sscb_adscb,
				sscb->sscb_dmm,
				&segment_dv,
				lowksp,
				coupon_dv);
	if (blob_info->flags & BLOBWKSP_FINAL && status == E_DB_ERROR)
	  /* this is fatal if we used the optimization, otherwise we will
	  ** try to continue with the query, and fail elsewhere */
	    status = E_DB_SEVERE; /* failed, query fatal */
	if (DB_FAILURE_MACRO(status))
	{
	    sscb->sscb_dmm = 0;	/* No blob in progress */
	    break;
	}

	if (status == E_DB_OK)
	{
	    /*
	    **	The the blob has been created.  Indicate that we're
	    **  no longer in blob gathering mode, and update the
	    **  caller query size and position.
	    **
	    ** If we managed to couponify into the final etab, and if we
	    ** asked for coercion when the blob was started, reconcile
	    ** null-ness of the coupon and the final datatype.
	    */
	    
	    if ((blob_info->flags & (BLOBWKSP_FINAL | BLOBWKSP_COERCE)) == (BLOBWKSP_FINAL | BLOBWKSP_COERCE))
	    {
		if (coupon_dv->db_datatype < 0 && blob_info->source_dt > 0)
		    /* coerced to a non-null, remove null byte */
		    coupon_dv->db_length--;
		else if (coupon_dv->db_datatype > 0 && blob_info->source_dt < 0)
		{
		    /* coerced to a null, add extra byte */
		    *((u_char *)(coupon_dv->db_data+coupon_dv->db_length)) = 0;
		    coupon_dv->db_length++;
		}
		coupon_dv->db_datatype = blob_info->source_dt;
	    }

	    sscb->sscb_dmm = 0;	/* No blob in progress */

	    *qry_size -= lowksp->adw_shared.shd_i_used;
	    *buffer += lowksp->adw_shared.shd_i_used;
	}
	else if ((status == E_DB_INFO) &&
	  (sscb->sscb_adscb->adf_errcb.ad_errcode == E_AD0002_INCOMPLETE))
	{
	    /*
	    **	This indicates that the coupon is incomplete.  Therefore,
	    **	we don't have the entire blob yet.  That being the case,
	    **	we will set scb_dmm in such a way as that our callers
	    **	will call us again with the next buffer full coming our way.
	    */

	    *qry_size = 0;
	    sscb->sscb_dmm = 1;
	}
	
	break;
    }
    if (DB_FAILURE_MACRO(status))
    {
	if (status > E_DB_FATAL)
	{
	    /* Then this is a memory allocation error */
	    sc0e_0_put(status, 0);
	    sc0e_0_uput(status, 0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	}
	else
	{
	    sc0e_0_put(sscb->sscb_adscb->adf_errcb.ad_errcode, 0);
	    scd_note(status, DB_ADF_ID);
	}
	sc0e_0_put(E_SC031A_LO_PARAM_ERROR, 0);
	sc0e_0_uput(E_SC031A_LO_PARAM_ERROR, 0);

	/* 
	** Set the EAT_INPUT flag.  This will cause scs_input() to
	** suck up blocks without further processing.  When the last
	** block is read, the errors queued will be returned.
	*/
	if (status <=  E_DB_ERROR)
	    sscb->sscb_req_flags |= SCS_EAT_INPUT_MASK;
	
	/*
	**  If the object was created & queued, it will be destroyed as the
	**  query terminates as if all was normal.
	*/
    }
    return(status);				
}

/*
** {
** Name: scs_fetch_data	- Fetch data from communication block.
**
** Description:
**	This routine fetches data from communication blocks.  All data comes
**	over as a sequence of GCA_DATA_VALUE structures.  The data is output
**	according to the value of the input argument "for_psf."  PSF always
**	takes data as DB_DATA_VALUES;  other facilities take data as data only,
**	since the datatypes of the values are already known.
**
** Inputs:
**      scb                             Scb for current session
**      for_psf                         Boolean indicating whether the
**                                      data is destined for the parser.
**	qry_size			Amount (in bytes) of data in ...
**	buf				Pointer to data received.
**	print_qry			Are we in PRINTQRY mode
**
** Outputs:
**      None, but . . .                 However, the data structure for the
**                                      destination facility is placed into
**                                      QSF.
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-Mar-1987 (fred)
**          Created.
**	03-aug-1989 (kimman)
**	    Alignment fixes.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              19-Jan-1989 (alexh)
**                  Forced for_psf flag to true when running STAR sessions
**              31-mar-1990 (carl)
**                  Incorporated alignment fixes from the 62ug porting line
**          End of STAR comments.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      17-Jan-1994 (fred)
**          Fix bug in scs_fetch_data() which was causing blob
**          parameters to repeat queries to be set up incorrectly.
**          Must provide pointer to data instead of pointer to
**          DB_DATA_VALUE when dealing with QEF.
**	21-jan-1994 (rog)
**	    If scb->scb_sscb.sscb_dmm is < 0, then we can only copy qry_size
**	    byes to fill the header: we can't assume that qry_size is larger
**	    than abs(scb->scb_sscb.sscb_dmm).  (Bug 56156)
**      13-Apr-1994 (fred)
**          In the event of a failure return from scs_blob_fetch(),
**          set the ret_val value and drop out.  Previously, we were
**          setting status, causing us to continue to try and process
**          data on subsequent calls.  Correctly setting ret_val will
**          allow for our caller to notice the failure and avoid
**          recalling this routine.
**	25-apr-1994 (rog)
**	    We need to align cdv->db_data and allocate extra memory to allow
**	    for this alignment.  (Bug 55993)
**      18-dec-1997 (stial01)
**          Return if blob data in input.
**	14-Apr-2010 (kschendel) SIR 123485
**	    Make sure blob coupon starts out clean.  Leaving junk in the DMF
**	    part can lead to bug 121046 (inconsistent results with API queries)
**	    if the blob length is zero; zero length blobs skip much of dmpe.
*/
DB_STATUS
scs_fetch_data(SCD_SCB	  *scb,
	       i4	  for_psf,
	       i4	  qry_size,
	       char	  *buf,
	       i4	  print_qry,
	       bool       *ad_peripheral )
{
    DB_STATUS		ret_val = E_DB_OK;
    DB_STATUS		status;
    QSF_RCB             *qs_ccb = scb->scb_sscb.sscb_qsccb;
    i4			got_desc = 0;
    DB_DATA_VALUE	*cdv;
    GCA_DATA_VALUE	*gdv;
    i4			dtbits;

    *ad_peripheral = FALSE;

    /* for STAR's case PSF interface is required for db values */
    if (scb->scb_sscb.sscb_flags & SCS_STAR)
	for_psf = 1;

    while ((ret_val == E_DB_OK) && (qry_size > 0))
    {
	if (scb->scb_sscb.sscb_dmm < 0)
	{
	    /* then the entire descriptor part didn't make it over */

	    /* Did we get enough to finish the descriptor? */
	    if (qry_size < -scb->scb_sscb.sscb_dmm)
	    {
		/*
		** We didn't get enough; so, copy what we got and return.
		**
		** Note: No need to adjust buf or qry_size here because we
		** are returning from the function at this point.
		*/
		MEcopy(buf, qry_size,
			 ((PTR) &scb->scb_sscb.sscb_gcadv
			 + GCA_DESC_SIZE
			 + scb->scb_sscb.sscb_dmm));
		scb->scb_sscb.sscb_dmm += qry_size;
		break;		/* exit loop as nothing left to process */
	    }
	    else
	    {
		MEcopy(buf, -scb->scb_sscb.sscb_dmm,
			((PTR) &scb->scb_sscb.sscb_gcadv + GCA_DESC_SIZE
			 + scb->scb_sscb.sscb_dmm));
		qry_size -= -scb->scb_sscb.sscb_dmm;
		buf += -scb->scb_sscb.sscb_dmm;
		scb->scb_sscb.sscb_dmm = 0;
		got_desc = 1;
		/*
		** This should really report an error because it should not
		** happen.  Also, this test must not be <= 0 because if
		** qry_size is 0 at this point, we need to fall through the
		** rest of the code so that sscb_dmm gets set to the number
		** of bytes needed for the data value.
		*/
		if (qry_size < 0)
		    break;
	    }
	}

	if (scb->scb_sscb.sscb_dmm == 0)
	{
	    if (!got_desc && (qry_size >= GCA_DESC_SIZE))
	    {
		gdv = (GCA_DATA_VALUE *) buf;
# ifdef BYTE_ALIGN
		MEcopy((PTR)gdv, GCA_DESC_SIZE, (PTR)&scb->scb_sscb.sscb_gcadv);
# else
		scb->scb_sscb.sscb_gcadv.gca_l_value = gdv->gca_l_value;
		scb->scb_sscb.sscb_gcadv.gca_type = gdv->gca_type;
		scb->scb_sscb.sscb_gcadv.gca_precision = gdv->gca_precision;
# endif
		qry_size -= GCA_DESC_SIZE;
		buf += GCA_DESC_SIZE;
	    }
	    else if (got_desc)
	    {
		got_desc = 0;
	    }
	    else
	    {
		MEcopy(buf, qry_size, (PTR)&scb->scb_sscb.sscb_gcadv);
		scb->scb_sscb.sscb_dmm =
			-(GCA_DESC_SIZE - qry_size);
		qry_size = 0;
		break;		/* exit loop as nothing left to process */
	    }

	    if (scb->scb_sscb.sscb_gcadv.gca_type == DB_QTXT_TYPE)
    	    {
		ret_val = scs_qt_fetch(scb, &qry_size, &buf);
		continue;
	    }

	    if (scb->scb_sscb.sscb_dnleft <=
		((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_dnum)
	    {
		/*
		** If we need more data space, than allocate it.
		** If this is the first allocation, take the default
		** amount.  Otherwise, double the previous amount
		*/

		scb->scb_sscb.sscb_dnleft = 
		    (((PSQ_QDESC *)
			  scb->scb_sscb.sscb_troot)->psq_dnum == 0)
			? SCS_DATA_DEFAULT
			: ((PSQ_QDESC *)
			    scb->scb_sscb.sscb_troot)->psq_dnum * 2;
		qs_ccb->qsf_sz_piece =
			scb->scb_sscb.sscb_dnleft * sizeof(PTR);
		status = qsf_call(QSO_PALLOC, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC021C_SCS_INPUT_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    ret_val = E_DB_ERROR;
		    break;
		}
		if (((PSQ_QDESC *)
			    scb->scb_sscb.sscb_troot)->psq_dnum)
		{
		    /* Copy over the old values (if any) */

		    MEcopy(
			(PTR)( ((PSQ_QDESC *)
			    scb->scb_sscb.sscb_troot)->psq_qrydata),
			((PSQ_QDESC *)
			    scb->scb_sscb.sscb_troot)->psq_dnum *
				sizeof(DB_DATA_VALUE *),
			qs_ccb->qsf_piece);
		}
		((PSQ_QDESC *)
		    scb->scb_sscb.sscb_troot)->psq_qrydata =
				(DB_DATA_VALUE **) qs_ccb->qsf_piece; 
	    }

	    /*
	    ** Since this is the first time through this data value
	    ** (no data movement mode), then allocate space for the
	    ** the datavalue descriptor and data.  If we've been here
	    ** before, than this will have already been done by us.
	    */

	    status = adi_dtinfo(scb->scb_sscb.sscb_adscb,
				    scb->scb_sscb.sscb_gcadv.gca_type,
				    &dtbits);
	    if (status)
	    {
		sc0e_0_put(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode, 0);
		sc0e_0_put( E_SC021C_SCS_INPUT_ERROR, 0);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		ret_val = E_DB_ERROR;
		break;
	    }

	    /*
	    ** We need to align the data value so we have to add 
	    ** sizeof(ALIGN_RESTRICT) - 1 to the amount of memory
	    ** we are requesting from QSF.
	    */

	    if ((dtbits & AD_PERIPHERAL) == 0)
	    {
		qs_ccb->qsf_sz_piece = scb->scb_sscb.sscb_gcadv.gca_l_value
				       + sizeof(DB_DATA_VALUE)
				       + sizeof(ALIGN_RESTRICT) - 1;
	    }
	    else
	    {
		qs_ccb->qsf_sz_piece = sizeof(ADP_PERIPHERAL)
				       + sizeof(DB_DATA_VALUE)
				       + sizeof(ALIGN_RESTRICT) - 1;
		if (scb->scb_sscb.sscb_gcadv.gca_type < 0)
		    qs_ccb->qsf_sz_piece += 1;
	    }

	    status = qsf_call(QSO_PALLOC, qs_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		    E_SC021C_SCS_INPUT_ERROR);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		ret_val = E_DB_ERROR;
		break;
	    }
	    ((PSQ_QDESC *)
		scb->scb_sscb.sscb_troot)->psq_datasize +=
		    qs_ccb->qsf_sz_piece;
	    ((PSQ_QDESC *)
		scb->scb_sscb.sscb_troot)->psq_qrydata[
		    ((PSQ_QDESC *) 
		      scb->scb_sscb.sscb_troot)->psq_dnum++] =
			    cdv = (DB_DATA_VALUE *) qs_ccb->qsf_piece;
	    cdv->db_data = 
			ME_ALIGN_MACRO(((char *) cdv + sizeof(DB_DATA_VALUE)),
				       sizeof(ALIGN_RESTRICT));
	    if ((dtbits & AD_PERIPHERAL) == 0)
	    {
		cdv->db_length = (i4) scb->scb_sscb.sscb_gcadv.gca_l_value;
	    }
	    else
	    {
		cdv->db_length = (i4) sizeof(ADP_PERIPHERAL);
		if (scb->scb_sscb.sscb_gcadv.gca_type < 0)
		    cdv->db_length += 1;
	    }
	    cdv->db_datatype = (DB_DT_ID) scb->scb_sscb.sscb_gcadv.gca_type;
	    cdv->db_prec = (i2) scb->scb_sscb.sscb_gcadv.gca_precision;

	    if (dtbits & AD_PERIPHERAL)
	    {
		*ad_peripheral = TRUE;
		/* Make sure no junk is in the coupon, especially important
		** to clear the DMF part if zero length blob!
		*/
		MEfill(sizeof(ADP_COUPON), 0,
			(PTR) &((ADP_PERIPHERAL *)cdv->db_data)->per_value.val_coupon);
		ret_val= scs_blob_fetch(scb, cdv, &qry_size, &buf, NULL);
		if (ret_val)
		    break;
		if (!for_psf)
		{
		    ((PSQ_QDESC *)
			scb->scb_sscb.sscb_troot)->psq_qrydata[
			    ((PSQ_QDESC *) 
			      scb->scb_sscb.sscb_troot)->psq_dnum - 1] =
					(DB_DATA_VALUE *) cdv->db_data;
		}
	    }
	    else if (cdv->db_length <= qry_size)
	    {
		DB_DATA_VALUE	dbdv;

		if (!for_psf)
		{
		    ((PSQ_QDESC *)
			scb->scb_sscb.sscb_troot)->psq_qrydata[
			    ((PSQ_QDESC *) 
			      scb->scb_sscb.sscb_troot)->psq_dnum - 1] =
					(DB_DATA_VALUE *) cdv->db_data;
		}
		MEcopy(buf, cdv->db_length, cdv->db_data);
		buf += cdv->db_length;
		qry_size -= cdv->db_length;
		
		dbdv.db_datatype =
		    (DB_DT_ID) scb->scb_sscb.sscb_gcadv.gca_type;
		dbdv.db_prec = (i2) scb->scb_sscb.sscb_gcadv.gca_precision;
		dbdv.db_length = (i4) scb->scb_sscb.sscb_gcadv.gca_l_value;
		dbdv.db_data = (PTR) cdv->db_data;
		/* Move NULL byte to EOS */
		(VOID)adg_vlup_setnull( &dbdv);
		
		if (print_qry)
		{
		    adu_2prvalue(sc0e_trace, &dbdv);
		}
	    }
	    else
	    {
		MEcopy(buf, qry_size, cdv->db_data);
		scb->scb_sscb.sscb_dmm = cdv->db_length - qry_size;
		qry_size = 0;
	    }
	}
	else if (scb->scb_sscb.sscb_gcadv.gca_type == DB_QTXT_TYPE)
		    /* && (dmm > 0) */
	{
	    ret_val = scs_qt_fetch(scb, &qry_size, &buf);
	}
	else /* if dmm > 0 -- less than taken care of above */
	{
	    /* then the data didn't all make it.  Finish the move */
	    cdv = 
		((PSQ_QDESC *) 
		    scb->scb_sscb.sscb_troot)->psq_qrydata[
		      ((PSQ_QDESC *)
			scb->scb_sscb.sscb_troot)->psq_dnum - 1];

	    status = adi_dtinfo(scb->scb_sscb.sscb_adscb,
				    scb->scb_sscb.sscb_gcadv.gca_type,
				    &dtbits);
	    if (status)
	    {
		sc0e_0_put(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode,0);
		sc0e_0_put( E_SC021C_SCS_INPUT_ERROR, 0);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		ret_val = E_DB_ERROR;
		break;
	    }

	    if (dtbits & AD_PERIPHERAL)
	    {
	        ret_val = scs_blob_fetch(scb, cdv, &qry_size, &buf, NULL);
		if ((ret_val == E_DB_OK) && (!for_psf))
		{
		    ((PSQ_QDESC *)
			scb->scb_sscb.sscb_troot)->psq_qrydata[
			    ((PSQ_QDESC *) 
			      scb->scb_sscb.sscb_troot)->psq_dnum - 1] =
					(DB_DATA_VALUE *) cdv->db_data;
		}
	    }
	    else if (scb->scb_sscb.sscb_dmm <= qry_size)
	    {
		DB_DATA_VALUE	dbdv;

		MEcopy(buf,
		    scb->scb_sscb.sscb_dmm,
		    (char *) cdv->db_data
			+ (cdv->db_length - scb->scb_sscb.sscb_dmm));
		buf += scb->scb_sscb.sscb_dmm;
		qry_size -= scb->scb_sscb.sscb_dmm;
		scb->scb_sscb.sscb_dmm = 0;
		if (!for_psf)
		{
		    ((PSQ_QDESC *)
			scb->scb_sscb.sscb_troot)->psq_qrydata[
			    ((PSQ_QDESC *) 
			      scb->scb_sscb.sscb_troot)->psq_dnum - 1] =
					(DB_DATA_VALUE *) cdv->db_data;
		}
		dbdv.db_datatype =
			    (DB_DT_ID) scb->scb_sscb.sscb_gcadv.gca_type;
		dbdv.db_prec = (i2) scb->scb_sscb.sscb_gcadv.gca_precision;
		dbdv.db_length = (i4) scb->scb_sscb.sscb_gcadv.gca_l_value;
		dbdv.db_data = (PTR) cdv->db_data;
		/* Move NULL byte to EOS */
		(VOID)adg_vlup_setnull( &dbdv);
		if (print_qry)
		{
		    adu_2prvalue(sc0e_trace, &dbdv);
		}
	    }
	    else
	    {
		MEcopy(buf, qry_size,
		    (char *) cdv->db_data
			+ (cdv->db_length - scb->scb_sscb.sscb_dmm));
		scb->scb_sscb.sscb_dmm -= qry_size;
		    /* cdv->db_length - qry_size; */
		qry_size = 0;
	    }
	} /* of if dmm */
    }
    if (DB_SUCCESS_MACRO(ret_val))
	ret_val = E_DB_OK;
    return(ret_val);
}

/*{
** Name: scs_fdbp_data	- Fetch database procedure parameters
**
** Description:
**      This routine is a corollary to scs_fetch_data() above.  It differs in
**	that it must deal with database procedure message parameters.
**
**	These messages come in a GCA_IP_DATA structure from GCF (from the FE).
**	This message is a GCA_ID which is the procedure to invoke,
**	followed by a mask (which is, at present, unused), followed by an array
**	of GCA_P_PARAM structures, each of which is a name (which is a length
**	then that many characters, followed by an unused mask, followed by a
**	GCA_DATA_VALUE).  This message can, unfortunately, be split at any point
**	and must be reconstructed.  That's the complexity of this routine.
**
**	There is running commentary in the code.  Basically, this routine is
**	called to extract the parameters from the message.  It takes the buffer,
**	saves as much of the current parameters as is possible, saves its state
**	(see Outputs: below), and returns.  The overall loop runs it thru the
**	current set of messages.
**
**	This routine assumes that any new message will be large enough to
**	contain any portion of the descriptor (+ name & mask) which were not
**	contained in the previous portion.  Since this is on the order of 30-40
**	bytes, and we typically read in 1K chunks, this should not be an issue.
**
** Inputs:
**      scb                             Session Control Block
**      qry_size                        The size of the buffer presented as
**      buf                             A pointer to the buffer containing this
**					segment of the message
**      print_qry                       Is "set printqry" set?
**
** Outputs:
**      scb->
**	    scb_sscb.sscb_dmm                Set to the appropriate
**					    data-movement-mode to indicate the
**					    current state of message processing.
**					    This is set to
**						0 to indicate that we are at the
**						    beginning of some known
**						    structure (a particular
**						    parameter),
**						> 0 to indicate that we are in
**						    midst of processing the data
**						    portion of the
**						    GCA_DATA_VALUE.  The value
**						    is the number of bytes which
**						    we need to see.
**						< 0 to indicate that we are in
**						    the midst of processing the
**						    descriptor.  (i.e. the
**						    name, mask, and non-data
**						    portion of the
**						    GCA_DATA_VALUE associated
**						    with the descriptor).  The
**						    value is the inverse (0 -)
**						    the number  of bytes which
**						    we need to see.
**	    scb_sscb.sscb_pstate	The state of parameter processing.  This
**					is a subclassification of dmm mentioned
**					above, and has the following meanings:
**
**					    SCS_GCNMLEN -- Awaiting remainder of
**							    name length.
**					    SCS_GCNMENTIRE -- Awaiting remainder
**							    of name.
**					    SCS_GCMASK -- Awaiting remainder of
**							    parameter mask
**							    (unused).
**					    [SCS_GCNORMAL] -- not waiting for
**							    anything.
**					This value has meaning only if dmm < 0.
**					If not, this value is ignored.
**	    scb_sscb.sscb_qeccb->qef_usr_param
**					Filled with the parameters for the
**					query.  The memory used is allocated
**					out of QSF.
**	    scb_sscb.sscb_cquery.cur_qprmcount
**					Filled with the parameter count.
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
**      Spring/Summer-1988 (fred)
**          Created for database procedure support.
**      9-jan-1990 (fred)
**          Added documentation (Oops :-()
**	    Also, fixed many bugs in the handling of partial name transfer.
**	30-oct-1990 (ralph)
**	    Bug 33582 -- scs_fdbp_data gets confused when reading dbp parms
**	    Changed alignment fix for byte-aligned systems.
**	03-jun-91 (ralph)
**	    Fix bug 31585 -- scs_fdbp_data error when break on gca_parm_mask
**	25-jul-91 (ralph)
**	    Fix bug 31585 (sun4) -- use MECOPY_CONST_MACRO in scs_fdbp_data
**	    to obtain unaligned integers
**	09-nov-92 (jhahn)
**	    Added handling for byrefs.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	07-jan-94 (rblumer)
**	    Changed cui_idxlate call to record the status returned, and
**	    now log any error returned by cui_idxlate call.  (B58511)
**	22-feb-94 (rblumer)
**	    change dbproc name translation to use cui_f_idxlate when possible;
**	    this function is much faster (for regular ids) than cui_idxlate.
**	04-mar-1994 (rog)
**	    We need to align qup->parm_dbv.db_data and add space to the name
**	    to account for this alignment.  (Related to the change in 
**	    scs_fetch_data() above for bug 55993.)
[@history_template@]...
*/
DB_STATUS
scs_fdbp_data(SCD_SCB	  *scb,
	      i4	  qry_size,
	      char	  *buf,
	      i4	  print_qry )
{
    DB_STATUS		ret_val = E_DB_OK;
    DB_STATUS		status;	/* Status of underlying calls */
    QSF_RCB             *qs_ccb = scb->scb_sscb.sscb_qsccb;
    i4			got_desc = 0;
			/* have we seen the entire descriptor for this parm ? */
    QEF_USR_PARAM	*qup;	/* Ptr into QSF mem which we are formatting */
    GCA_P_PARAM		*gpm; /* Ptr to current parameter */
    QEF_RCB		*qe_ccb;
    GCA_DATA_VALUE	*gdv; /* Ptr to current data value */

    i4			lname;	/* Temp holding place for gca_l_name */
    u_i4		l_id;
    char		tempstr[DB_MAXNAME];
    u_i4		templen;
    DB_ERROR		cui_err;
    i4                  dtbits;
    QEF_USR_PARAM	*unaligned_data;
    i4			adj_amt;
    char		stbuf[SC0E_FMTSIZE]; /* last char for `\n' */
    
    /*
    ** This is the size of the fixed header portion of a parameter message...
    ** GCA_DESC_SIZE is the fixed size of a packed GCA_DATA_VALUE,
    ** then, for dbp's, this adds the length of the name + the length
    ** of the mask.
    */
#define	GCA_PRM_SIZE	(GCA_DESC_SIZE + sizeof(gpm->gca_parname.gca_l_name) + sizeof(gpm->gca_parm_mask))

    /* b33582 (ralph)
    **
    **	Here's a graphic so I can remember what we are processing:
    **----------------------------------------------------------------
    **		    GCA_IP_DATA
    **			GCA_ID		    gca_id_proc;
    **			    i4		gca_index[2];
    **			    char		gca_name[GCA_MAXNAME];
    **			i4		    gca_proc_mask;
    **----------------------------------------------------------------
    ** dmm == 0		GCA_P_PARAM	    gca_param[*];
    ** dmm <  0		    GCA_NAME		gca_parname
    **	SCS_GCNMLEN	-->	i4		    gca_l_name;
    **	SCS_GCNMENTIRE	-->	char		    gca_name[*];
    **	SCS_GCMASK	--> i4		gca_parm_mask;
    ** 	SCS_GCNORMAL	--> GCA_DATA_VALUE  gca_parvalue;
    **				i4		gca_type;
    **				i4		gca_precision;
    **				i4		gca_l_value;
    ** dmm > 0			char		gca_value[*];
    **----------------------------------------------------------------
    */

    qe_ccb = scb->scb_sscb.sscb_qeccb;
    
    /*
    ** While we are still happy and have things to do...
    */
    
    while ((ret_val == E_DB_OK) && (qry_size > 0))
    {
	/*
	** If the last time thru, we didn't get all the way thru the descriptor,
	** (dmm < 0) but have all of the name information (pstate == GCNORMAL)
	*/
    	if ((scb->scb_sscb.sscb_dmm < 0) &&
		(scb->scb_sscb.sscb_pstate == SCS_GCNORMAL))
	{
	    /*
	    ** then the entire descriptor part didn't make it over, and dmm
	    ** holds the (additive inverse of) the amount of the descriptor
	    ** which we have seen.  Move the remainder of the descriptor in.
	    */

	    /*
	    ** b33582 (ralph)
	    ** We have already seen gca_param.gca_parname and
	    ** gca_param.gca_parm_mask.  We are processing the part
	    ** of gca_parvalue before gca_parvalue.gca_value.
	    ** Move this residual into scb_sscb.sscb_gcadv.
	    **
	    ** I think sscb_dmm holds the additive inverse of the
	    ** remaining length of the gca_parvalue structure before
	    ** gca_value to be moved into scb_sscb.sscb_gcadv.
	    */

	    MEcopy(buf, -scb->scb_sscb.sscb_dmm,
		((char *) &scb->scb_sscb.sscb_gcadv +
		    GCA_DESC_SIZE + scb->scb_sscb.sscb_dmm));
	    qry_size -= -scb->scb_sscb.sscb_dmm;
	    buf += -scb->scb_sscb.sscb_dmm;
	    scb->scb_sscb.sscb_dmm = 0;
	    got_desc = 1;
	    if (qry_size <= 0)
		break;
	}
	else if (scb->scb_sscb.sscb_dmm < 0)
	    /*
	    **	And, by implication, it was part of the way thru the name
	    **	processing...
	    */
	{
	    /*
	    **  b33582 (ralph)
	    ** We are in the midst of processing gca_param.gca_parname
	    ** or gca_param.gca_parm_mask.
	    */

	    /* If we were getting the length of the name */
	    if (scb->scb_sscb.sscb_pstate == SCS_GCNMLEN)
	    {
		/*
		** b33582 (ralph)
		** We are processing gca_param.gca_parname.gca_l_name.
		** Move the remainder of it into scb_sscb.sscb_gclname.
		*/

		/*
		**  dmm holds -(amount of length of name to be seen),
		**  so copy remainder of the length, then go grab the name
		**  itself.
		*/

#ifdef	xDEBUG
		if (ult_check_macro(&scb->scb_sscb.sscb_trace, SCS_TEVENT, NULL, NULL))
		{
		    TRdisplay("DBP_PARAMETER_EVENT>>> Found remainder of name length\n");
		}
#endif
		
		MEcopy(buf, -scb->scb_sscb.sscb_dmm,
			((char *) &scb->scb_sscb.sscb_gclname
				+ sizeof(scb->scb_sscb.sscb_gclname)
				+ scb->scb_sscb.sscb_dmm));
		buf += -scb->scb_sscb.sscb_dmm;
		MEcopy(buf, scb->scb_sscb.sscb_gclname,
		    scb->scb_sscb.sscb_gcname);
		buf += scb->scb_sscb.sscb_gclname;
		MEcopy(buf, sizeof(gpm->gca_parm_mask),
				   (PTR)&scb->scb_sscb.sscb_gcaparm_mask);
		buf += sizeof(gpm->gca_parm_mask);
		qry_size -=
		    (-scb->scb_sscb.sscb_dmm
			+ scb->scb_sscb.sscb_gclname
			+ sizeof(gpm->gca_parm_mask));
	    }
	    else if (scb->scb_sscb.sscb_pstate == SCS_GCNMENTIRE)
			/* we were a portion of the way thru the name */
	    {
		/*
                ** b33582 (ralph)
                ** We are processing gca_param.gca_parname.gca_name.
                ** Move the remainder of it into scb_sscb.sscb_gcname.
                */

		/*
		** dmm holds -(amount of name to be seen), go get remainder of
		** the name).
		*/
#ifdef	xDEBUG
		if (ult_check_macro(&scb->scb_sscb.sscb_trace, SCS_TEVENT, NULL, NULL))
		{
		    TRdisplay("DBP_PARAMETER_EVENT>>> Found remainder of name\n");
		}
#endif
		MEcopy(buf,
			    -scb->scb_sscb.sscb_dmm,
			     &scb->scb_sscb.sscb_gcname[
				scb->scb_sscb.sscb_gclname +
					scb->scb_sscb.sscb_dmm]);
		buf += -scb->scb_sscb.sscb_dmm;
		MEcopy(buf, sizeof(gpm->gca_parm_mask),
				   (PTR)&scb->scb_sscb.sscb_gcaparm_mask);
		buf += sizeof(gpm->gca_parm_mask);
		qry_size -=
			(-scb->scb_sscb.sscb_dmm + sizeof(gpm->gca_parm_mask));
	    }
	    else /* Then we need to skip the remainder of the mask */
	    {
		/*
                ** b33582 (ralph)
                ** We are processing gca_param.gca_parm_mask.
		** Just slough it.
                */

#ifdef	xDEBUG
		if (ult_check_macro(&scb->scb_sscb.sscb_trace, SCS_TEVENT, NULL, NULL))
		{
		    TRdisplay("DBP_PARAMETER_EVENT>>> Found remainder of mask\n");
		}
#endif
		MEcopy(buf, -scb->scb_sscb.sscb_dmm,
		       (char *) &scb->scb_sscb.sscb_gcaparm_mask +
		       sizeof(gpm->gca_parm_mask) +
		       scb->scb_sscb.sscb_dmm);
		buf += -scb->scb_sscb.sscb_dmm;
		qry_size -= -scb->scb_sscb.sscb_dmm;
	    }

	    /*
            ** b33582 (ralph)
	    ** At this point, we are at the beginning of gca_parvalue.
	    */

	    scb->scb_sscb.sscb_pstate = SCS_GCNORMAL;
	    gdv = (GCA_DATA_VALUE *) buf;
	    /* b31585 - use MECOPY_CONST_MACRO to get unaligned integers */
# ifdef BYTE_ALIGN
	    MEcopy((PTR)&gdv->gca_l_value,
		sizeof(scb->scb_sscb.sscb_gcadv.gca_l_value),
		(PTR)&scb->scb_sscb.sscb_gcadv.gca_l_value);
	    MEcopy((PTR)&gdv->gca_type,
		sizeof(scb->scb_sscb.sscb_gcadv.gca_type),
		(PTR)&scb->scb_sscb.sscb_gcadv.gca_type);
	    MEcopy((PTR)&gdv->gca_precision,
		sizeof(scb->scb_sscb.sscb_gcadv.gca_precision),
		(PTR)&scb->scb_sscb.sscb_gcadv.gca_precision);
# else
	    scb->scb_sscb.sscb_gcadv.gca_l_value    = gdv->gca_l_value;
	    scb->scb_sscb.sscb_gcadv.gca_type	    = gdv->gca_type;
	    scb->scb_sscb.sscb_gcadv.gca_precision  = gdv->gca_precision;
# endif
	    buf += GCA_DESC_SIZE;
	    qry_size -= (GCA_DESC_SIZE);
	    scb->scb_sscb.sscb_dmm = 0;
	    got_desc = 1;
	    if (qry_size <= 0)
		break;
	    /*
            ** b33582 (ralph)
            ** At this point, we are at gca_parvalue.gca_value.
            */
	}
	    
	if (scb->scb_sscb.sscb_dmm == 0)
	{
	    /* Then we are at a "new" parameter */

            /*
            ** b33582 (ralph)
            ** Actually, I think if got_desc, then we are pointing to
	    ** gca_parvalue.gca_value; otherwise, we are at gca_param.
            */
	    
	    gpm = (GCA_P_PARAM *) buf;
# ifdef BYTE_ALIGN
	    MEcopy((PTR)&gpm->gca_parname.gca_l_name,
			       sizeof(lname),(PTR)&lname);
# else
	    lname = gpm->gca_parname.gca_l_name;
# endif
	    if (!got_desc && (qry_size >= sizeof(gpm->gca_parname.gca_l_name))
		     &&
		    (qry_size >= (GCA_PRM_SIZE + lname)))
	    {
		/*
		**  If we haven't gotten the descriptor yet (above), and we have
		**  enough stuff to get the whole length and the its associated
		**  descriptor (" + gpm->gca_parname..." is probably an
		**  alignment problem  {@fix_me@}
		*/
		gdv = (GCA_DATA_VALUE *)
			((char *) gpm->gca_parname.gca_name +
				    lname +
				    sizeof(gpm->gca_parm_mask));
# ifdef BYTE_ALIGN
		MEcopy((PTR)&gdv->gca_l_value,
		    sizeof(scb->scb_sscb.sscb_gcadv.gca_l_value),
			  (PTR)&scb->scb_sscb.sscb_gcadv.gca_l_value);
		MEcopy((PTR)&gdv->gca_type,
		    sizeof(scb->scb_sscb.sscb_gcadv.gca_type),
			  (PTR)&scb->scb_sscb.sscb_gcadv.gca_type);
		MEcopy((PTR)&gdv->gca_precision,
		    sizeof(scb->scb_sscb.sscb_gcadv.gca_precision),
			  (PTR)&scb->scb_sscb.sscb_gcadv.gca_precision);
# else
		scb->scb_sscb.sscb_gcadv.gca_l_value	= gdv->gca_l_value;
		scb->scb_sscb.sscb_gcadv.gca_type	= gdv->gca_type;
		scb->scb_sscb.sscb_gcadv.gca_precision	= gdv->gca_precision;
# endif
		scb->scb_sscb.sscb_gclname = lname;
		MEcopy(gpm->gca_parname.gca_name,
			    lname,
			    scb->scb_sscb.sscb_gcname);
		MEcopy((char *)gdv -
				   sizeof(gpm->gca_parm_mask),
				   sizeof(gpm->gca_parm_mask),
				   (PTR)&scb->scb_sscb.sscb_gcaparm_mask);
		qry_size -= GCA_PRM_SIZE + scb->scb_sscb.sscb_gclname;
		buf += GCA_PRM_SIZE + scb->scb_sscb.sscb_gclname;
	    }
	    else if (got_desc)
	    {
		/*
		**  If we've gotten the descriptor in data-movement-mode above,
		**  then clear that fact, because we don't care anymore
		*/
		got_desc = 0;
	    }
	    else if (qry_size < sizeof(gpm->gca_parname.gca_l_name))
	    {
		/*
		** If there isn't enough room for the entire name length, then
		**	set the indicator that we are snarfing the name length,
		**	copy what portion of it we can,
		**	set dmm to the amount to be seen,
		**	note that we have no more buffer,
		**	and get out.
		*/
#ifdef	xDEBUG
		if (ult_check_macro(&scb->scb_sscb.sscb_trace, SCS_TEVENT, NULL, NULL))
		{
		    TRdisplay("DBP_PARAMETER_EVENT>>> Need remainder of name length\n");
		}
#endif
		scb->scb_sscb.sscb_pstate = SCS_GCNMLEN;
		MEcopy(buf, qry_size, (PTR)&scb->scb_sscb.sscb_gclname);
		scb->scb_sscb.sscb_dmm =
		    -(sizeof(gpm->gca_parname.gca_l_name) - qry_size);
		qry_size = 0;
		break;
	    }
	    else if (qry_size < lname + sizeof(gpm->gca_parname.gca_l_name))
	    {
		/*
		**  If there's enough room for the length of the name (from
		**  above), but not the entire name, then
		**	Snarf the length of the name,
		**	Eat what portion of the name we can get,
		**	Mark that we are getting the name itself,
		**	Set dmm to the amount of the name we still must see.
		**	Note end of message,
		**	and Get out...
		*/
#ifdef	xDEBUG
		if (ult_check_macro(&scb->scb_sscb.sscb_trace, SCS_TEVENT, NULL, NULL))
		{
		    TRdisplay("DBP_PARAMETER_EVENT>>> Need remainder of name\n");
		}
#endif
		MEcopy(buf,
			sizeof(gpm->gca_parname.gca_l_name),
			(PTR)&scb->scb_sscb.sscb_gclname);
		MEcopy(((char *) buf + sizeof(gpm->gca_parname.gca_l_name)),
			qry_size - sizeof(gpm->gca_parname.gca_l_name),
			scb->scb_sscb.sscb_gcname);
		scb->scb_sscb.sscb_pstate = SCS_GCNMENTIRE;
		scb->scb_sscb.sscb_dmm =
			-((sizeof(gpm->gca_parname.gca_l_name) + lname)
							     - qry_size);
		qry_size = 0;
		break;		/* exit loop as nothing left to process */
	    }
	    else
	    {
		/*
		** Otherwise, we can get the entire name length and name, but
		** not the entire descriptor & mask.
		*/
		
		MEcopy(buf,
			sizeof(gpm->gca_parname.gca_l_name),
			(PTR)&scb->scb_sscb.sscb_gclname);
		MEcopy(((char *) buf + sizeof(gpm->gca_parname.gca_l_name)),
			scb->scb_sscb.sscb_gclname,
			scb->scb_sscb.sscb_gcname);
		if (qry_size >=
			(scb->scb_sscb.sscb_gclname +
			    sizeof(gpm->gca_parm_mask) +	    /*@b33582@*/
			    sizeof(gpm->gca_parname.gca_l_name)))
		{
		    /*
		    ** b33582 (ralph)
		    ** We are in here because we may have some (but not all)
		    ** of the descriptor portion of gca_parvalue.
		    */
		    MEcopy((char *) buf +
					sizeof(gpm->gca_parname.gca_l_name) +
					scb->scb_sscb.sscb_gclname,
					sizeof(gpm->gca_parm_mask),
					(PTR)&scb->scb_sscb.sscb_gcaparm_mask);
		    MEcopy(((char *) buf +
			    sizeof(gpm->gca_parname.gca_l_name) +
			    sizeof(gpm->gca_parm_mask) +
			    scb->scb_sscb.sscb_gclname),
			qry_size -
			    (sizeof(gpm->gca_parname.gca_l_name)
				+ sizeof(gpm->gca_parm_mask)
				+ scb->scb_sscb.sscb_gclname),
			(PTR)&scb->scb_sscb.sscb_gcadv);
		    scb->scb_sscb.sscb_pstate = SCS_GCNORMAL;
		    scb->scb_sscb.sscb_dmm =
			 -((GCA_PRM_SIZE + scb->scb_sscb.sscb_gclname)
							     - qry_size);
		}
		else /* Then we have the name, but not all of the mask */
		{
		    /*
		    ** So set up for continue'd processing next round...
		    **	    Set state to be looking for the remainder of the
		    **		mask (which'll be ignored anyway :-)
		    **	    And set dmm to the number of bytes we still need
		    **		to see.
		    */
#ifdef	xDEBUG
		    if (ult_check_macro(&scb->scb_sscb.sscb_trace,
					SCS_TEVENT, NULL, NULL))
		    {
			TRdisplay(
			    "DBP_PARAMETER_EVENT>>> Need remainder of mask\n");
		    }
#endif

		    MEcopy((char *) buf +
			    sizeof(gpm->gca_parname.gca_l_name) +
			    scb->scb_sscb.sscb_gclname,
			   qry_size -
			   (sizeof(gpm->gca_parname.gca_l_name)
			    + scb->scb_sscb.sscb_gclname),
			   (char *) &scb->scb_sscb.sscb_gcaparm_mask);
		    
		    scb->scb_sscb.sscb_pstate = SCS_GCMASK;
		    scb->scb_sscb.sscb_dmm =
			 qry_size -				    /*@b31585@*/
			    (sizeof(gpm->gca_parname.gca_l_name)
				+ scb->scb_sscb.sscb_gclname
				+ sizeof(gpm->gca_parm_mask));
		}
		qry_size = 0;
		break;		/* exit loop as nothing left to process */
	    }
	    scb->scb_sscb.sscb_pstate = SCS_GCNORMAL;
	    if (scb->scb_sscb.sscb_dnleft <=
		scb->scb_sscb.sscb_cquery.cur_qprmcount)
	    {
		/*
		** If we need more data space, than allocate it.
		** If this is the first allocation, take the default
		** amount.  Otherwise, double the previous amount
		*/

		scb->scb_sscb.sscb_dnleft = 
		    (scb->scb_sscb.sscb_cquery.cur_qprmcount == 0)
			? SCS_DATA_DEFAULT
			: scb->scb_sscb.sscb_cquery.cur_qprmcount * 2;
		qs_ccb->qsf_sz_piece =
			scb->scb_sscb.sscb_dnleft * sizeof(PTR);
		status = qsf_call(QSO_PALLOC, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC021C_SCS_INPUT_ERROR);
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    ret_val = E_DB_ERROR;
		    break;
		}
		if (scb->scb_sscb.sscb_cquery.cur_qprmcount)
		{
		    /* Copy over the old values (if any) */

		    MEcopy(scb->scb_sscb.sscb_dptr,
			    scb->scb_sscb.sscb_cquery.cur_qprmcount
				* sizeof(PTR),
			    qs_ccb->qsf_piece);
		}
		scb->scb_sscb.sscb_dptr =
				(PTR) qs_ccb->qsf_piece;
		qe_ccb->qef_usr_param = (QEF_USR_PARAM **) qs_ccb->qsf_piece;
	    }

	    /*
	    ** Since this is the first time through this data value
	    ** (no data movement mode), then allocate space for the
	    ** the datavalue descriptor and data.  If we've been here
	    ** before, than this will have already been done by us.
	    */

	    status = adi_dtinfo(scb->scb_sscb.sscb_adscb,
				scb->scb_sscb.sscb_gcadv.gca_type,
				&dtbits);
	    if (status)
	    {
		sc0e_0_put(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode, 0);
		sc0e_0_put( E_SC021C_SCS_INPUT_ERROR, 0);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		ret_val = E_DB_ERROR;
		break;
	    }
	    else if (dtbits & AD_PERIPHERAL)
	    {
		sc0e_0_uput(E_SC0130_LO_DBP_MISMATCH, 0);
		scb->scb_sscb.sscb_req_flags |= SCS_EAT_INPUT_MASK;
		break;
	    }

	    qs_ccb->qsf_sz_piece =
		scb->scb_sscb.sscb_gcadv.gca_l_value
			    + scb->scb_sscb.sscb_gclname
			    + sizeof(QEF_USR_PARAM)
			    + sizeof(ALIGN_RESTRICT) - 1;
	    status = qsf_call(QSO_PALLOC, qs_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		    E_SC021C_SCS_INPUT_ERROR);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		ret_val = E_DB_ERROR;
		break;
	    }
	    qe_ccb->qef_usr_param[scb->scb_sscb.sscb_cquery.cur_qprmcount] =
		    (QEF_USR_PARAM *) qs_ccb->qsf_piece;
	    qup = (QEF_USR_PARAM *) qs_ccb->qsf_piece;

	    /* Fill in QEF's parameter structures */
	    
	    unaligned_data = (QEF_USR_PARAM *)((char *) qup + sizeof(*qup));
	    qup->parm_dbv.db_data =
		ME_ALIGN_MACRO(unaligned_data, sizeof(ALIGN_RESTRICT));
	    adj_amt = (char *) qup->parm_dbv.db_data - (char *) unaligned_data;
	    qup->parm_dbv.db_length =
		(i4) scb->scb_sscb.sscb_gcadv.gca_l_value;
	    qup->parm_dbv.db_datatype =
		(DB_DT_ID) scb->scb_sscb.sscb_gcadv.gca_type;
	    qup->parm_dbv.db_prec =
		(i2) scb->scb_sscb.sscb_gcadv.gca_precision;
	    qup->parm_nlen =
		(i4) scb->scb_sscb.sscb_gclname;
	    qup->parm_name = (char *)
		((char *) qup + sizeof(*qup) + qup->parm_dbv.db_length
		 + adj_amt);
	    MEcopy(scb->scb_sscb.sscb_gcname,
		    scb->scb_sscb.sscb_gclname,
		    (PTR) qup->parm_name);

	    /*
	    ** The parm name must be normalized & case translated.
	    ** We will do this "in place" here.
	    **
	    ** Try to use the 'fast' idxlate function;
	    ** if it fails, then use the normal idxlate function.
	    */
	    l_id = qup->parm_nlen;

	    if (cui_f_idxlate(qup->parm_name, l_id,
			      scb->scb_sscb.sscb_ics.ics_dbxlate) == FALSE)
	    {
		templen = DB_PARM_MAXNAME;
		status = cui_idxlate((u_char *)qup->parm_name,
				     &l_id, (u_char *)tempstr, &templen,
				     scb->scb_sscb.sscb_ics.ics_dbxlate,
				     (u_i4 *)NULL, &cui_err);
		if (DB_FAILURE_MACRO(status))
		{
		    sc0e_uput(cui_err.err_code, 0, 2,
			      sizeof(ERx("Procedure parameter name"))-1,
			      (PTR)ERx("Procedure parameter name"),
			      l_id, (PTR) qup->parm_name,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0,
			      0, (PTR)0);
		    /* output to error log, too */
		    sc0e_put(cui_err.err_code, 0, 2,
			     sizeof(ERx("Procedure parameter name"))-1,
			     (PTR)ERx("Procedure parameter name"),
			     l_id, (PTR) qup->parm_name,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0);
		    /*****************
		    ** for now, don't return an error here,
		    ** since that will terminate the session
		    ** without printing out the above error.
		    ** Instead, continue and let QEF return
		    ** a 'parameter not found' error.
		    ** 		(rblumer 07-jan-94)
		    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		    ret_val = E_DB_ERROR;
		    break;
		    *****************/
		}
		/*
		** NOTE: Since the normalization/translation is done
		** "in place," the parameter name will have trailing
		** blanks when delimiters were provided....
		*/
		(VOID)MEmove(templen, (PTR)tempstr, ' ',
			     qup->parm_nlen,
			     qup->parm_name);

	    }  /* end if cui_f_idxlate */
	    
	    /* Set parm_flags for BYREF/OUTPUT parameters. */
	    qup->parm_flags = (scb->scb_sscb.sscb_gcaparm_mask & GCA_IS_BYREF &&
					   (scb->scb_cscb.cscb_version >=
					    GCA_PROTOCOL_LEVEL_60)
		    && !ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_NOBYREF,
				       NULL,NULL))?
						QEF_IS_BYREF : 0;
	    qup->parm_flags |= (scb->scb_sscb.sscb_gcaparm_mask & GCA_IS_OUTPUT &&
					   (scb->scb_cscb.cscb_version >=
					    GCA_PROTOCOL_LEVEL_60)
		    && !ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_NOBYREF,
				       NULL,NULL))?
						QEF_IS_OUTPUT : 0;
	    /* Name-less parm must be positional. */
	    if (qup->parm_nlen == 0)
		qup->parm_flags |= QEF_IS_POSPARM;

	    if (qup->parm_dbv.db_length <= qry_size)
	    {
		DB_DATA_VALUE	dbdv;
		/*
		** If we have all the data, copy it over, and skip over it.
		** If print_qry is set, then dump the information to the user
		** and/or trace log
		*/
		
		MEcopy(buf, qup->parm_dbv.db_length, qup->parm_dbv.db_data);
		buf += qup->parm_dbv.db_length;
		qry_size -= qup->parm_dbv.db_length;
	        dbdv.db_datatype =
		    (DB_DT_ID) scb->scb_sscb.sscb_gcadv.gca_type;
	        dbdv.db_prec = (i2) scb->scb_sscb.sscb_gcadv.gca_precision;
	        dbdv.db_length = (i4) scb->scb_sscb.sscb_gcadv.gca_l_value;
		dbdv.db_data = (PTR) qup->parm_dbv.db_data;
		if (print_qry)
		{

		    sc0e_trace(STprintf(stbuf, "Parameter Name %*s, Value:\n",
			    scb->scb_sscb.sscb_gclname,
			    scb->scb_sscb.sscb_gcname));


		    adu_2prvalue(sc0e_trace, &dbdv);
		}
		/*
		** Query text trace the parameter
		*/
		if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
		{
			sc0a_qrytext_param(scb,  
				scb->scb_sscb.sscb_gcname,
				scb->scb_sscb.sscb_gclname,
				&dbdv );
		}
		scb->scb_sscb.sscb_cquery.cur_qprmcount++;
	    }
	    else    /* Not enough room for all the data ... */
	    {
		MEcopy(buf, qry_size, qup->parm_dbv.db_data);

		/*
		** Copy what we have, and set dmm to the number of bytes we
		** still need to see.
		*/
		
		scb->scb_sscb.sscb_dmm =
		    qup->parm_dbv.db_length - qry_size;
		qry_size = 0;
	    }
	}
	else /* if dmm > 0 -- less than taken care of above */
	{
	    /* then the data didn't all make it.  Finish the move */
	    qup = (QEF_USR_PARAM *)
		    qe_ccb->qef_usr_param[scb->scb_sscb.sscb_cquery.cur_qprmcount];
	    if (scb->scb_sscb.sscb_dmm <= qry_size)
	    {
		/*
		**  If all the data is in this block, then
		**	move it,
		**	skip over what we've gotten,
		**	and move onto the next parameter
		**
		**  If printing, then printout the value, now that we have it
		**  all.
		*/
		
		MEcopy(buf,
		    scb->scb_sscb.sscb_dmm,
		    (char *) qup->parm_dbv.db_data
			+ (qup->parm_dbv.db_length - scb->scb_sscb.sscb_dmm));
		buf += scb->scb_sscb.sscb_dmm;
		qry_size -= scb->scb_sscb.sscb_dmm;
		scb->scb_sscb.sscb_dmm = 0;
		scb->scb_sscb.sscb_cquery.cur_qprmcount++;

		if (print_qry)
		{
		    DB_DATA_VALUE	dbdv;

		    sc0e_trace(STprintf(stbuf, "Parameter Name %*s, Value:\n",
			    scb->scb_sscb.sscb_gclname,
			    scb->scb_sscb.sscb_gcname));

		    dbdv.db_datatype =
			    (DB_DT_ID) scb->scb_sscb.sscb_gcadv.gca_type;
		    dbdv.db_prec = (i2) scb->scb_sscb.sscb_gcadv.gca_precision;
		    dbdv.db_length = (i4) scb->scb_sscb.sscb_gcadv.gca_l_value;
		    dbdv.db_data = (PTR) qup->parm_dbv.db_data;

		    adu_2prvalue(sc0e_trace, &dbdv);
		}
		/*
		** Query text trace the parameter
		*/
		if( (Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
		{
		    DB_DATA_VALUE	dbdv;

		    dbdv.db_datatype =
			    (DB_DT_ID) scb->scb_sscb.sscb_gcadv.gca_type;
		    dbdv.db_prec = (i2) scb->scb_sscb.sscb_gcadv.gca_precision;
		    dbdv.db_length = (i4) scb->scb_sscb.sscb_gcadv.gca_l_value;
		    dbdv.db_data = (PTR) qup->parm_dbv.db_data;

		    sc0a_qrytext_param(scb,  
				scb->scb_sscb.sscb_gcname,
				scb->scb_sscb.sscb_gclname,
				&dbdv );
		}
	    }
	    else /* not all the data we need is even in this block */
	    {
		/*
		** In this case, not all the data we need is here yet.  Get what
		** there is, and reduce the count of stuff that we need.
		*/
		
		MEcopy(buf, qry_size,
		    (char *) qup->parm_dbv.db_data
			+ (qup->parm_dbv.db_length - scb->scb_sscb.sscb_dmm));
		scb->scb_sscb.sscb_dmm -= qry_size;
		    /* qup->db_length - qry_size; */
		qry_size = 0;
	    }
	} /* of if dmm */
    }
    return(ret_val);
}

/*{
** Name: scs_qt_fetch	- Fetch query text
**
** Description:
**      This routine places the query text in a special buffer area for 
**      PSF.  It updates the block size and buffer pointers it was given 
**      And sets the scb level data movement mode as appropriate.
**
** Inputs:
**      scb                             Session control block.
Ox**      p_qry_size                      Address of block size parm.
**      p_buf                           Address of address of buffer.
**
** Outputs:
**      none
**	Returns:
**	    DB_STATUS
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      09-Jul-1987 (fred)
**          Created for GCA/F adaptation.
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
DB_STATUS
scs_qt_fetch(SCD_SCB      *scb,
	     i4         *p_qry_size,
	     char	  **p_buf )
{
    DB_STATUS		ret_val = E_DB_OK;
    DB_STATUS		status;
    i4                  qry_size = *p_qry_size;
    char		*buf = *p_buf;
    GCA_DATA_VALUE      *gd = &scb->scb_sscb.sscb_gcadv;
    QSF_RCB		*qs_ccb;

    if (gd->gca_l_value >= scb->scb_sscb.sscb_tnleft)
    {
	/* if equal, won't fit because of the null terminator */
	for (;;)
	{
	    /*
	    ** here, the text won't fit.  Therefore, alloc a new object
	    ** of twice the size of the old one, move everything over
	    ** and continue.
	    */

	    qs_ccb = scb->scb_sscb.sscb_qsccb;
	    qs_ccb->qsf_sz_piece = (scb->scb_sscb.sscb_tsize * 2) + 
		    ((gd->gca_l_value > scb->scb_sscb.sscb_tsize * 2) ?
			    gd->gca_l_value : 0);
	    status = qsf_call(QSO_PALLOC, qs_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		    E_SC021C_SCS_INPUT_ERROR);
		sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
		ret_val = E_DB_ERROR;
		break;
	    }
	    MEcopy(
		((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_qrytext,
		((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_qrysize,
		qs_ccb->qsf_piece);
	    ((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_qrytext = qs_ccb->qsf_piece;
	    scb->scb_sscb.sscb_tptr = (char *) qs_ccb->qsf_piece
		+ ((PSQ_QDESC *)
		    scb->scb_sscb.sscb_troot)->psq_qrysize;
	    scb->scb_sscb.sscb_tsize = qs_ccb->qsf_sz_piece;
	    scb->scb_sscb.sscb_tnleft =
		    scb->scb_sscb.sscb_tsize - 
			((PSQ_QDESC *)
			    scb->scb_sscb.sscb_troot)->psq_qrysize;
	    break;
	} /* of for */
	if (ret_val)
	    return(ret_val);
    }

    if (scb->scb_sscb.sscb_dmm > 0)
    {
	MEcopy(buf, scb->scb_sscb.sscb_dmm, scb->scb_sscb.sscb_tptr);

	if (scb->scb_sscb.sscb_dmm <= qry_size)
	{
	    *p_buf += scb->scb_sscb.sscb_dmm;
	    *p_qry_size -= scb->scb_sscb.sscb_dmm;
	    scb->scb_sscb.sscb_tptr += scb->scb_sscb.sscb_dmm;
	    ((PSQ_QDESC *)
		scb->scb_sscb.sscb_troot)->psq_qrysize
			+= scb->scb_sscb.sscb_dmm; 
	    *scb->scb_sscb.sscb_tptr = 0;
	    scb->scb_sscb.sscb_tnleft -= scb->scb_sscb.sscb_dmm;
	    scb->scb_sscb.sscb_dmm = 0;
	}
	else
	{
	    *p_qry_size = 0;
	    *p_buf += qry_size;
	    scb->scb_sscb.sscb_tptr += qry_size;
	    ((PSQ_QDESC *)
		scb->scb_sscb.sscb_troot)->psq_qrysize += qry_size; 
	    *scb->scb_sscb.sscb_tptr = 0;
	    scb->scb_sscb.sscb_tnleft -= qry_size;
	    scb->scb_sscb.sscb_dmm -= qry_size;
	}
    }
    else if (gd->gca_l_value <= qry_size)
    {
	MEcopy(buf, gd->gca_l_value, scb->scb_sscb.sscb_tptr);
	((PSQ_QDESC *)
	    scb->scb_sscb.sscb_troot)->psq_qrysize += gd->gca_l_value; 
	scb->scb_sscb.sscb_tptr += gd->gca_l_value;
	*scb->scb_sscb.sscb_tptr = 0;
	scb->scb_sscb.sscb_tnleft -= gd->gca_l_value;
	*p_qry_size -= gd->gca_l_value;
	*p_buf += gd->gca_l_value;
    }
    else
    {
	MEcopy(buf, qry_size, scb->scb_sscb.sscb_tptr);
	((PSQ_QDESC *)
	    scb->scb_sscb.sscb_troot)->psq_qrysize += qry_size; 
	scb->scb_sscb.sscb_tptr += qry_size;
	*scb->scb_sscb.sscb_tptr = 0;
	scb->scb_sscb.sscb_tnleft -= qry_size;
	scb->scb_sscb.sscb_dmm = gd->gca_l_value - qry_size;
	*p_qry_size = 0;
	*p_buf += qry_size;
    }
    
    /* save query text in case we are next sent a query without text */
    if (scb->scb_sscb.sscb_dmm <= 0)
    {
	/* got it all */
	if (scb->scb_sscb.sscb_cquery.cur_qtxt_len <= 
		((PSQ_QDESC *)scb->scb_sscb.sscb_troot)->psq_qrysize + sizeof(SC0M_OBJECT))
	{
	    /* current buffer is too small */
	    if (scb->scb_sscb.sscb_cquery.cur_qtxt)
		status = sc0m_deallocate(0,&scb->scb_sscb.sscb_cquery.cur_qtxt);
	    
	    ret_val = sc0m_allocate(0,
		    sizeof(SC0M_OBJECT) + ((PSQ_QDESC *)scb->scb_sscb.sscb_troot)->psq_qrysize + 1,
		    DB_SCF_ID, (PTR)SCS_MEM, SCG_TAG,
		    &scb->scb_sscb.sscb_cquery.cur_qtxt);
	    if (ret_val)
	    {
		sc0e_0_put(ret_val, 0);
		sc0e_0_uput(ret_val, 0 );
		scd_note(E_DB_SEVERE, DB_SCF_ID);
		return(ret_val);
	    }
	}
	STcopy(((PSQ_QDESC *)scb->scb_sscb.sscb_troot)->psq_qrytext,
		scb->scb_sscb.sscb_cquery.cur_qtxt + sizeof(SC0M_OBJECT));
	scb->scb_sscb.sscb_cquery.cur_qtxt_len = 
		((PSQ_QDESC *)scb->scb_sscb.sscb_troot)->psq_qrysize;

    }
    return(ret_val);
}

/*{
** Name: scs_desc_send	- Send tuple descriptor to FE
**
** Description:
**      This routine performs the necessary interface to GCA to allow 
**      a tuple descriptor to be sent to the FE.
**
** Inputs:
**      scb                             Session control block
**
** Outputs:
**      none
**	Returns:
**	    GCA status
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-May-1987 (fred)
**          Created.
**	04-aug-1989 (kimman)
**	    Alignment fixes for GCA_COL_ATT.
**	10-oct-1990 (ralph)
**	    6.3->6.5 merge:
**	    21-may-90 (linda)
**		If sc0m_allocate() call fails, don't try to use the space which
**		would have been allocated.  This bug was found by Peter Ng on
**		IBM system and caused server to terminate.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              31-mar-1990 (carl)
**                  Retrofitted alignment fix from 62ug porting line.
**          End of STAR comments.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper ptr for allocation.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      4-Apr-1996 (loera01)
**	    Send GCA_COMP_MASK to front end if compression is supported and
** 	    the tuple contains a variable datatype.
**      21-jan-97 (loera01)
**	    Bug 79287: Don't compress data if the tuple has a blob.
**	25-apr-1997 (shero03)
**	    If compression, save the tuple descriptor off the cquery structure.
**      10-may-01 (loera01)
**          Added support for DB_NVCHR_TYPE (Unicode varchars).
**	21-Dec-01 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
**	2-feb-2007 (dougi)
**	    Add check to assure client supports back end types.
**     13-Oct-2009 (maspa05) SIR 122725
**          Add tuple descriptor to SC930 trace output
**          ult_print_tracefile now uses integer constants instead of string
**          for type parameter - SC930_LTYPE_PARM instead of "PARM" and so on
**          Also moved function definitions for SC930 tracing to ulf.h
**          The functions involved were - ult_always_trace, ult_open_tracefile
**          ult_print_tracefile and ult_close_tracefile
**     13-Nov-2009 (hanal04) Bug 122881
**          Tuple descriptor output in sc930 hits SIGBUS on byte aligned
**          platforms. Add local variable and byte align.
*/
DB_STATUS
scs_desc_send(SCD_SCB	  *scb,
	      SCC_GCMSG   **pmessage )
{
    i4			i;
# ifdef BYTE_ALIGN
    i4                  l_attname;
# endif
    i4			size = (sizeof (GCA_TD_DATA) - sizeof (GCA_COL_ATT));
    DB_STATUS		status;
    GCA_COL_ATT		*cur_att;
    SCC_GCMSG		*message;
    PTR			darea;
    GCA_TD_DATA		*tdesc;
    PTR			block;
    bool                check_comp;
    DB_DT_ID 		datatype;


    status = E_DB_OK;
    tdesc = (GCA_TD_DATA *) scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc;

    /* SC930 tracing */

    if (ult_always_trace())
    {
    	void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
        if (f)
        {
    		char 	tmp[100];
		GCA_DBMS_VALUE gca_dbv;
		GCA_TD_DATA gca_td;

		/* Header - TDESC id:No of cols:Tuple length:result modifier bitmask */

# ifdef BYTE_ALIGN
                MEcopy((PTR)&tdesc->gca_id_tdscr, 
                                   sizeof(gca_td.gca_id_tdscr), 
                                   (PTR)&gca_td.gca_id_tdscr);
                MEcopy((PTR)&tdesc->gca_l_col_desc, 
                                   sizeof(gca_td.gca_l_col_desc), 
                                   (PTR)&gca_td.gca_l_col_desc);
                MEcopy((PTR)&tdesc->gca_tsize, 
                                   sizeof(gca_td.gca_tsize), 
                                   (PTR)&gca_td.gca_tsize);
                MEcopy((PTR)&tdesc->gca_result_modifier, 
                                   sizeof(gca_td.gca_result_modifier), 
                                   (PTR)&gca_td.gca_result_modifier);
# else
                gca_td.gca_id_tdscr = tdesc->gca_id_tdscr;
                gca_td.gca_l_col_desc = tdesc->gca_l_col_desc;
                gca_td.gca_tsize  = tdesc->gca_tsize;
                gca_td.gca_result_modifier = tdesc->gca_result_modifier;
# endif

                STprintf(tmp,"%d:%d:%d:%d",
                                gca_td.gca_id_tdscr,
				gca_td.gca_l_col_desc,
                                gca_td.gca_tsize,
                                gca_td.gca_result_modifier);
                ult_print_tracefile(f,SC930_LTYPE_TDESC_HDR,tmp);

    		cur_att = (GCA_COL_ATT *) &tdesc->gca_col_desc[0];

    		for (i = 0; i < tdesc->gca_l_col_desc; i++)
    		{

		        /* Column - col number:datatype:length:precision */

# ifdef BYTE_ALIGN
                        MEcopy((PTR)&cur_att->gca_attdbv.db_datatype, sizeof(DB_DT_ID), (PTR)&gca_dbv.db_datatype);
                        MEcopy((PTR)&cur_att->gca_attdbv.db_length, sizeof(gca_dbv.db_length), (PTR)&gca_dbv.db_length);
                        MEcopy((PTR)&cur_att->gca_attdbv.db_prec, sizeof(gca_dbv.db_prec), (PTR)&gca_dbv.db_prec);
                        
# else
                        gca_dbv.db_datatype = cur_att->gca_attdbv.db_datatype;
                        gca_dbv.db_length = cur_att->gca_attdbv.db_length;
                        gca_dbv.db_prec = cur_att->gca_attdbv.db_prec;
# endif

  	        	STprintf(tmp,"%d:%d:%d:%d",
				i ,
                                gca_dbv.db_datatype,
                                gca_dbv.db_length,
                                gca_dbv.db_prec);
				
                	ult_print_tracefile(f,SC930_LTYPE_TDESC_COL,tmp);

# ifdef BYTE_ALIGN
    			{
        			MEcopy((PTR)&cur_att->gca_l_attname, 
							sizeof(l_attname), (PTR)&l_attname);
        			MEcopy(&cur_att->gca_l_attname, 
							sizeof(l_attname), &l_attname);
				cur_att = (GCA_COL_ATT *) &cur_att->gca_attname[l_attname];
    			}  
# else
				cur_att = (GCA_COL_ATT *)
						&cur_att->gca_attname[cur_att->gca_l_attname];
# endif
		}

                ult_close_tracefile(f);
	}
    }


    /*
    ** If tuple has a large object, do not compress varchars.
    */
    if (tdesc->gca_result_modifier & GCA_LO_MASK)
    {
        tdesc->gca_result_modifier &= ~GCA_COMP_MASK;
    }
    /*
    **  Check for a compressible datatype if the front end has requested
    **  compression in its query mask, and if the back end supports
    **  compression.
    **  If the tuple has a large object, don't compress varchar data.
    */
    check_comp = (tdesc->gca_result_modifier & GCA_COMP_MASK) ? TRUE : FALSE;
    check_comp = (check_comp && 
       (Sc_main_cb->sc_capabilities & SC_VCH_COMPRESS)) ? 
       TRUE : FALSE;
    tdesc->gca_id_tdscr = scb->scb_sscb.sscb_nostatements;

    cur_att = (GCA_COL_ATT *) &tdesc->gca_col_desc[0];
    /*
    **  Remove compression mask temporarily.  This bit is only returned to
    **  the front end if the tuple actualy contains a compressible data type.
    */
    tdesc->gca_result_modifier &= ~GCA_COMP_MASK;
    for (i = 0; i < tdesc->gca_l_col_desc; i++)
    {

       /*
       **  If the tuple contains any varchars, and the protocol
       **  supports varchar compression, set the result modifier
       **  to indicate that it will be compressed.
       */
# ifdef BYTE_ALIGN
       MEcopy((PTR)&cur_att->gca_attdbv.db_datatype,
	   sizeof(DB_DT_ID), (PTR)&datatype);
       datatype = abs(datatype);
# else
       datatype = abs(cur_att->gca_attdbv.db_datatype);
# endif
       if (check_comp)
       {
          switch (datatype)
	  {
	      case DB_NVCHR_TYPE:
	      case DB_VCH_TYPE:
	      case DB_TXT_TYPE:
	      case DB_VBYTE_TYPE: 
	      case DB_LTXT_TYPE:
              {
                   tdesc->gca_result_modifier |= GCA_COMP_MASK;
		   check_comp = FALSE;
		   break;
              }
	      default:
	      {
		  break;
              }
	   }
        }

	/* Assure that client supports this type. */
	status = scs_returntype_check(scb, datatype);
	if (status != E_DB_OK)
	    return(status);

# ifdef BYTE_ALIGN
    {
        MEcopy((PTR)&cur_att->gca_l_attname, sizeof(l_attname), (PTR)&l_attname);
        MEcopy(&cur_att->gca_l_attname, sizeof(l_attname), &l_attname);
	size += l_attname + sizeof(cur_att->gca_l_attname) + 
		sizeof(cur_att->gca_attdbv);
	cur_att = (GCA_COL_ATT *) &cur_att->gca_attname[l_attname];
    }  
# else
    {
	size += cur_att->gca_l_attname + sizeof(cur_att->gca_l_attname) +
		sizeof(cur_att->gca_attdbv);
	cur_att = (GCA_COL_ATT *)
			&cur_att->gca_attname[cur_att->gca_l_attname];
    }
# endif
    } /* end for loop */


    scb->scb_sscb.sscb_cquery.cur_rdesc.rd_modifier = 
	tdesc->gca_result_modifier;
    if (size <= scb->scb_cscb.cscb_dsize && 
	scb->scb_cscb.cscb_different != TRUE )
    {
	message = &scb->scb_cscb.cscb_dscmsg;
	message->scg_mask = SCG_NODEALLOC_MASK;
	darea = (PTR) scb->scb_cscb.cscb_darea;
	if (pmessage != NULL)
	{
	    *pmessage = (SCC_GCMSG *) NULL;
	}
    }
    else
    {
	status = sc0m_allocate(0,
		    sizeof(SCC_GCMSG) + size,
		    DB_SCF_ID,
		    (PTR)SCS_MEM,
		    SCG_TAG,
		    &block);
	message = (SCC_GCMSG *)block;
	if (status)
	{
	    sc0e_0_put(status, 0);
	    sc0e_0_uput(status, 0);
	    scd_note(E_DB_SEVERE, DB_SCF_ID);
	}
	else
	{
	    if (scb->scb_cscb.cscb_different == TRUE)
	    {
		if (pmessage != NULL)
		{
		    *pmessage = message;
		}
		message->scg_mask = 
				(SCG_NODEALLOC_MASK | SCG_QDONE_DEALLOC_MASK);
	    }
	    else
	    {
		if (pmessage != NULL)
		{
		    *pmessage = (SCC_GCMSG *) NULL;
		}
		message->scg_mask = 0;
	    }

	    message->scg_marea = darea = ((char *) message + sizeof(SCC_GCMSG));
	    if (scb->scb_cscb.cscb_different == TRUE)
	    {
	        scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc = darea;	       		        
	    }
	}
    }
    /* status value was set in call to sc0m_allocate(), above. (linda) */
    if (status == E_DB_OK)
    {
	message->scg_msize = size;
	message->scg_mtype = GCA_TDESCR;
	MEcopy((PTR)tdesc, size, darea);
	message->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
	scb->scb_cscb.cscb_mnext.scg_prev = message;
    }
    return(status);
}

/*{
** Name: scs_returntype_check	- verify that client can handle datatype
**
** Description:
**	If the client is running on an earlier protocol than the server, it
**	may not support all server types. This function verifies that the
**	client supports the types being returned and issues an error if
**	an unsupported type is being sent in the GCA_TDESCR message.
**
** Inputs:
**
** Outputs:
**      None.
**	Returns:
**	    E_DB_ERROR			Always an error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      2-feb-2007 (dougi)
**	    Written as part of ANSI date/time support.
*/
DB_STATUS
scs_returntype_check(SCD_SCB	*scb,
	DB_DT_ID	type)

{
    ADI_DT_NAME	typename;
    DB_STATUS	status;
    bool	badtype = FALSE;


    /* Check for data type conflicts with front end. It'd be nice
    ** if there were a handy field with the server protocol so we
    ** could skip the tests unless server protocol is greater than
    ** client, but I haven't found it. */
    switch (type)
    {
      case DB_TMWO_TYPE:
      case DB_TMW_TYPE:
      case DB_TME_TYPE:
      case DB_TSWO_TYPE:
      case DB_TSW_TYPE:
      case DB_TSTMP_TYPE:
      case DB_ADTE_TYPE:
      case DB_INYM_TYPE:
      case DB_INDS_TYPE:
	if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_66)
	badtype = TRUE;
	break;

      case DB_LBLOC_TYPE:
      case DB_LCLOC_TYPE:
      case DB_LNLOC_TYPE:
	if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_67)
	badtype = TRUE;
	break;
      case DB_BOO_TYPE:
        if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_68)
            badtype = TRUE;
        break;
    }

    if (badtype)
    {
	status = adi_tyname(scb->scb_sscb.sscb_adscb, type, &typename);
	if (status)
	{
	    sc0e_0_put(scb->scb_sscb.sscb_adscb->adf_errcb.ad_errcode, 0);
	    sc0e_0_put( E_SC021C_SCS_INPUT_ERROR, 0);
	    sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
	    return(E_DB_ERROR);
	}

	/* Format and display error. */
	sc0e_uput(E_US10F0_4336, 0, 2,
		4, (PTR)&scb->scb_cscb.cscb_version,
		scs_trimwhite(sizeof(ADI_DT_NAME), (PTR)&typename), 
				(PTR)&typename,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0);
	return(E_DB_ERROR);
    }
    return(E_DB_OK);
}


/*{
** Name: scs_access_error - Send error saying node cannot access this feature
**
** Description:
**	This function sends an error to the user telling him that his node is
**	not licensed for the feature he is attempting to use.  We always
**	return E_DB_ERROR so that the caller will do error processing (i.e.
**	clean up after himself).
**
** Inputs:
**	qmode				Current query mode which will determine
**					the flavor of error message
**
** Outputs:
**      None.
**	Returns:
**	    E_DB_ERROR			Always an error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      22-jun-89 (jrb)
**          Created.
**	04-feb-91 (neil)
**	    Alerters: Creation of EVENT objects also requires access.
**	15-jul-91 (andre)
**	    SET GROUP/ROLE are allowed only if knowledge management extension is
**	    turned on
**	13-jan-93 (robf)
**          ENABLE/DISABLE SECURITY_AUDIT and CREATE/DROP SECURITY_ALARM
**	    are allowed only if KME is turned on. Also create a single
**	    message for "Resource Control" rather than repeating each time.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	5-nov-93 (robf)
**	    Add SET MAXnnn session limits
**	22-feb-94 (robf)
**          Add SET ROLE
**	
[@history_template@]...
*/
DB_STATUS
scs_access_error( i4  qmode )
{
    char	*stmt;
    char	*package;
static char     *resc_control="Resource Control";

    switch (qmode)
    {
      case PSQ_RULE:
	stmt = "CREATE RULE ...";
	package = resc_control;
	break;
      case PSQ_DSTRULE:
	stmt = "DROP RULE ...";
	package = resc_control;
	break;
      case PSQ_CGROUP:
	stmt = "CREATE GROUP ...";
	package = resc_control;
	break;
      case PSQ_AGROUP:
      case PSQ_DGROUP:
	stmt = "ALTER GROUP ...";
	package = resc_control;
	break;
      case PSQ_KGROUP:
	stmt = "DROP GROUP ...";
	package = resc_control;
	break;
      case PSQ_CAPLID:
	stmt = "CREATE ROLE ...";
	package = resc_control;
	break;
      case PSQ_AAPLID:
	stmt = "ALTER ROLE ...";
	package = resc_control;
	break;
      case PSQ_KAPLID:
	stmt = "DROP ROLE ...";
	package = resc_control;
	break;
      case PSQ_GDBPRIV:
	stmt = "GRANT ON DATABASE ...";
	package = resc_control;
	break;
      case PSQ_RDBPRIV:
	stmt = "REVOKE ON DATABASE ...";
	package = resc_control;
	break;
      case PSQ_MXQUERY:
      case PSQ_MXSESSION:
	stmt = "SET [NO]MAX...";
	package = resc_control;
	break;
      case PSQ_CALARM:
        stmt = "CREATE SECURITY_ALARM ...";
	package = resc_control;
	break;
      case PSQ_KALARM:
        stmt = "DROP SECURITY_ALARM ...";
	package = resc_control;
	break;
      case PSQ_ENAUDIT:
        stmt = "ENABLE SECURITY_AUDIT ...";
	package = resc_control;
	break;
      case PSQ_ALTAUDIT:
        stmt = "ALTER SECURITY_AUDIT ...";
	package = resc_control;
	break;
      case PSQ_DISAUDIT:
        stmt = "DISABLE SECURITY_AUDIT ...";
	package = resc_control;
	break;
      case PSQ_EVENT:
	stmt = "CREATE DBEVENT ...";
	package = resc_control;
	break;
      case PSQ_EVDROP:
	stmt = "DROP DBEVENT ...";
	package = resc_control;
	break;
      case PSQ_SETROLE:
	stmt = "SET ROLE...";
	package = resc_control;
	break;
      case PSQ_RGRANT:
	stmt = "GRANT ROLE...";
	package = resc_control;
	break;
      case PSQ_RREVOKE:
	stmt = "REVOKE ROLE...";
	package = resc_control;
	break;
      default:
	stmt = "<unknown>";
	package = "<unknown>";
	break;
    }

    sc0e_uput(E_SC0313_NOT_LICENSED, 0, 2,
	      0, (PTR)stmt,
	      0, (PTR)package,
	      0, (PTR)0,
	      0, (PTR)0,
	      0, (PTR)0,
	      0, (PTR)0);

    return(E_DB_ERROR);
}

/*{
** Name: scs_csr_find	- Find a valid entry in cursor table with given ids.
**
** Inputs:
**	scb.sscb.sscb_cquery		Session current query information:
**	   cur_qname.db_cursor_id[0,1]	Ids of cursor we're searching for.
**	scb.sscb.sscb_cursor		Session cursor information:
**	   csi_state			Only check if non-zero (in use).
**	   csi_id.db_cursor_id[0,1]	Cursor ids to compare against.
**	   curs_limit			Max # of allocated cursor entries.
** Outputs:
**	Returns:
**	   i4				< curs_limit if found.
**					= curs_limit if not found.
** History:
**	26-apr-1990 (neil)
**	   Written for initial cursor performance project.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	30-may-06 (dougi)
**	    Copy cursor text to ics_qbuf if we're debugging.
*/
i4
scs_csr_find( SCD_SCB *scb )
{
    SCS_CSITBL	*csi;
    i4		i;

    csi = scb->scb_sscb.sscb_cursor.curs_csi;
    for (i = 0; i < scb->scb_sscb.sscb_cursor.curs_limit; i++)
    {
	if (   (csi->csitbl[i].csi_state)
	    && (csi->csitbl[i].csi_id.db_cursor_id[0] == 
		   scb->scb_sscb.sscb_cquery.cur_qname.db_cursor_id[0])
	    && (csi->csitbl[i].csi_id.db_cursor_id[1] == 
		   scb->scb_sscb.sscb_cquery.cur_qname.db_cursor_id[1])
	   )
	{
	    break;
	}
    }

    /* Plug cursor text into ics_qbuf. */
    if (i < scb->scb_sscb.sscb_cursor.curs_limit && 
		csi->csitbl[i].csi_textl &&
		csi->csitbl[i].csi_textp)
    {
	MEcopy(csi->csitbl[i].csi_textp, csi->csitbl[i].csi_textl,
	    (PTR)&scb->scb_sscb.sscb_ics.ics_qbuf);
	scb->scb_sscb.sscb_ics.ics_l_qbuf = csi->csitbl[i].csi_textl;
    }

    return (i);
}
/*{
** Name: scs_cursor_remove	- Remove open cursor instantiations
**
** Description:
**      This routine performs the actions necessary from SCF's point of view to
**	remove knowledge of a cursor.  This includes calling PSF to make sure
**	the cursor has been deleted, and removing the knowledge of the cursor
**	from the sessions arena.
**
** Inputs:
**      scb                             The session control block
**      cursor                          Pointer to the cursor control area in
**					the session control block.  If zero,
**					then remove knowledge of all cursors.
**
** Outputs:
**      scb				The cursor control area is cleared.
**
**	Returns:
**	    VOID
**	Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      22-May-1990 (fred)
**          Created as part of large object implementation.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	18-mar-94 (andre)
**	    added code to deallocate QSF object containing repeat cursor 
**	    parameter info.
**	14-nov-1997 (wonst02)
** 	    Bug 86490 - Varchar compression and cursors
** 	    Free tdesc when cursor closed.
**	17-feb-1998 (somsa01)
**	    When checking for GCA_COMP_MASK, we may have hit situations
**	    where we should not be checking for this bit. We were checking
**	    this bit in gca_result_modifier, which was referenced off of
**	    rd_tdesc. However, at query execution time, rd_tdesc is valid
**	    ONLY if varchar compression is turned on (since that's when we
**	    SAVE a copy of the tuple descriptor, while other times we just
**	    reference the one from the query plan which is "invalidated" at
**	    execution time). We now check this bit inside either
**	    rd_modifier or csi_state.  (Bug #89000)
**	06-dec-2004 (thaju02)
**	    Destroy stored query text object. (B113588) 
**	21-Apr-2010 (kschendel) SIR 123485
**	    Delete pointless fiddling with blob coupon lists.
*/
VOID
scs_cursor_remove(SCD_SCB *scb,
		  SCS_CSI *cursor )
{
    DB_STATUS           status;
    PSQ_CB		*ps_ccb;
    i4			i = 0;
    SC0M_OBJECT		*object;
    SCS_CSI		*c = cursor;
    QSF_RCB		*qs_ccb = scb->scb_sscb.sscb_qsccb;

    ps_ccb = scb->scb_sscb.sscb_psccb;
    if (cursor)
    {
	STRUCT_ASSIGN_MACRO(c->csi_id, ps_ccb->psq_cursid);
	ps_ccb->psq_flag &= ~PSQ_CLSALL;
    }
    else
    {
	ps_ccb->psq_flag |= PSQ_CLSALL;
    }
    status = psq_call(PSQ_CLSCURS, ps_ccb, scb->scb_sscb.sscb_psscb);
    if (DB_FAILURE_MACRO(status))
    {
	    sc0e_0_put(E_SC0215_PSF_ERROR, 0);
	    sc0e_0_put(ps_ccb->psq_error.err_code, 0);
	    sc0e_0_uput(ps_ccb->psq_error.err_code, 0);
	    scd_note(status, DB_PSF_ID);
    }

    if (!cursor)
    {
	c = &scb->scb_sscb.sscb_cursor.curs_csi->csitbl[0];
    }

    while (i < scb->scb_sscb.sscb_cursor.curs_limit)
    {
	if (c->csi_state)
	{
	    if (c->csi_tdesc_msg != NULL)
	    {
		sc0m_deallocate(0, (PTR *)&c->csi_tdesc_msg);
	    }

	    /*
	    ** if this is a repeat cursor whose definition involved 
	    ** repeat parameters, we need to destroy the QSF object 
	    ** containing the parameters and pointers to them + zero 
	    ** out csi_rep_parm_handle, csi_rep_parm_lk_id, and 
	    ** csi_rep_parms in SCS_CSI structure describing this cursor
	    */
	    if (c->csi_state & CSI_REPEAT_MASK && c->csi_rep_parm_handle)
	    {
		if ((c->csi_rep_parm_handle == c->csi_thandle) &&
		    (c->csi_rep_parm_lk_id == c->csi_tlk_id))
		{
		    c->csi_thandle = (PTR)NULL;
		    c->csi_tlk_id = 0;
		}
		qs_ccb->qsf_lk_id = c->csi_rep_parm_lk_id;
		qs_ccb->qsf_obj_id.qso_handle = c->csi_rep_parm_handle;
		status = qsf_call(QSO_DESTROY, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC0210_SCS_SQNCR_ERROR);
		    scd_note(status, DB_QSF_ID);
		}
	    
		c->csi_rep_parm_handle = (PTR) NULL;
		c->csi_rep_parm_lk_id = 0;
		c->csi_rep_parms = (QEF_PARAM *) NULL;
	    }
            if (c->csi_state & CSI_GCA_COMP_MASK)
	    {
    	    	object = (SC0M_OBJECT *)c->csi_rdesc.rd_tdesc - 1;
	    	status = sc0m_deallocate(0, (PTR *)&object);
	    }

	    if (c->csi_thandle) 
	    {
		qs_ccb->qsf_lk_id = c->csi_tlk_id;
		qs_ccb->qsf_obj_id.qso_handle = c->csi_thandle;
		status = qsf_call(QSO_DESTROY, qs_ccb);
		if (DB_FAILURE_MACRO(status))
		{
		    scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC0210_SCS_SQNCR_ERROR);
		}
		c->csi_thandle = (PTR)NULL;
		c->csi_tlk_id = 0;
	    }
	
	    c->csi_state = 0;

	}
	if (!cursor)
	{
	    c++;
	    i++;
	}
	else
	{
	    break;
	}
    }
    if (!cursor)
    {
	scb->scb_sscb.sscb_ciu = 0;
    }
    return;
}

/*{
** Name: scs_allocate_byref_tdesc     - Allocate tdesc for sending back byrefs.
**
** Description:
**      This routine allocates a GCA tuple descriptor message for sending
**      the byref parameters from a dbproc back to the FE. If there
**	any byref parameters to send back this routine will format and
**	return a tuple descriptor message. The DB_DATA_VALUE parts of the
**	message will not be filled out. Those will be left for QEF.
**
** Inputs:
**      scb                             Session control block
**	descmsg				Ptr to tuple descriptor message
**
** Outputs:
**      descmsg				Will point to allocated tdesc.
**					Will be set to NULL if no byrefs
**	Returns:
**	    GCA status
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-Jul-1993 (jhahn)
**          Created.
**	10-Aug-1993 (jhahn)
**	    Fixed problem when sending back names.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	21-Dec-01 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
**	123-july-06 (dougi)
**	    Do this in the presence of IS_OUTPUT parms, too.
**	2-feb-2007 (dougi)
**	    Verify returned types are supported by client.
*/
static DB_STATUS
scs_allocate_byref_tdesc(
	SCD_SCB	  *scb,
	SCC_GCMSG **descmsg)
{
    i4			descriptor_size;
    i4			column_count;
    i4			i;
    QEF_USR_PARAM	*parm;
    QEF_RCB		*qef_rcb = scb->scb_sscb.sscb_qeccb;
    GCA_TD_DATA		*desc_area;
    DB_STATUS		status;
    SCC_GCMSG		*desc_message;
    GCA_COL_ATT		*next_descriptor;
    PTR			block;

    /* calculate size of tuple descriptor and tuple data messages */
    descriptor_size = sizeof (GCA_TD_DATA) - sizeof(GCA_COL_ATT);
    column_count = 0;
    
    for (i = 0; i<qef_rcb->qef_pcount;i++)
    {
	parm = qef_rcb->qef_usr_param[i];
	if (parm->parm_flags &  (QEF_IS_BYREF | QEF_IS_OUTPUT))
	{
	    column_count++;
	    descriptor_size += 
		sizeof(desc_area->gca_col_desc[0].gca_attdbv) + 
		sizeof(desc_area->gca_col_desc[0].gca_l_attname);
	    if (scb->scb_sscb.sscb_cquery.cur_dbp_names)
		descriptor_size += parm->parm_nlen;

	    /* Assure returned type is supported by client. */
	    status = scs_returntype_check(scb, 
				abs(parm->parm_dbv.db_datatype));
	    if (status != E_DB_OK)
		return(status);
	}
    }

    if (column_count == 0)
    {
	*descmsg = NULL;
	return (E_DB_OK); /* no parameters to send back */
    }

    /* allocate and format tuple descriptor and tuple data messages */

    status = sc0m_allocate(0,
			   sizeof(SCC_GCMSG) + descriptor_size,
			   DB_SCF_ID,
			   (PTR)SCS_MEM,
			   SCG_TAG,
			   &block);
    desc_message = (SCC_GCMSG *)block;
    if (status)
    {
	sc0e_0_put(status, 0);
	sc0e_0_uput(status, 0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	return status;
    }
    desc_message->scg_marea = ((char *) desc_message + sizeof(SCC_GCMSG));
    desc_area = (GCA_TD_DATA *)desc_message->scg_marea;


    /* fill in tuple descriptor and message */

    MEfill((i4) descriptor_size, 0, desc_area);
    desc_area->gca_result_modifier = scb->scb_sscb.sscb_cquery.cur_dbp_names ?
    				     GCA_NAMES_MASK:0;
    desc_area->gca_id_tdscr = scb->scb_sscb.sscb_nostatements;
    desc_area->gca_l_col_desc = column_count;
    if (scb->scb_sscb.sscb_cquery.cur_dbp_names)
    {
	/* Fill out the names if they are requested. */
	next_descriptor = desc_area->gca_col_desc;
	for (i = 0; i<qef_rcb->qef_pcount;i++)
	    {
		parm = qef_rcb->qef_usr_param[i];
		if (parm->parm_flags &  QEF_IS_BYREF)
		{
		    i4  name_length = parm->parm_nlen;
#ifdef BYTE_ALIGN
		    MEcopy((PTR)&name_length, sizeof(i4),
					(PTR)&next_descriptor->gca_l_attname);
#else
		    next_descriptor->gca_l_attname = name_length;
#endif
		    if (name_length >0)
			MEcopy(parm->parm_name, name_length,
			       next_descriptor->gca_attname);
		    next_descriptor = (GCA_COL_ATT *)
			((char *) next_descriptor + name_length +
			 sizeof(next_descriptor->gca_attdbv) + 
			 sizeof(next_descriptor->gca_l_attname));
		}
	   }
	   
    }


    desc_message->scg_mask = (SCG_NODEALLOC_MASK | SCG_QDONE_DEALLOC_MASK);
    desc_message->scg_msize = descriptor_size;
    desc_message->scg_mtype = GCA_TDESCR;

    *descmsg = desc_message;
    return (E_DB_OK);
}

/*{
** Name: scs_allocate_tdata	- Allocate GCA tdata message.
**
** Description:
**      This routine allocates and initializes a  GCA tuple data message of
**	sufficiant size to return a row of data as described by the passed in
**	tuple descriptor.
**
** Inputs:
**      scb                             Session control block
**	desc_area			Ptr to tuple descriptor message
**	datamsg				Ptr to tuple data message.
**
** Outputs:
**      descmsg				Will point to allocated tdata.
**
**	Returns:
**	    GCA status
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-july-1992 (jhahn)
**          Created.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
[@history_template@]...
*/
static DB_STATUS
scs_allocate_tdata(
	SCD_SCB	  *scb,
	GCA_TD_DATA *desc_area,
	SCC_GCMSG **datamsg)
{
    i4			tuple_size = desc_area->gca_tsize;
    DB_STATUS		status;
    SCC_GCMSG		*data_message;
    PTR			block;

    /* allocate and format tuple descriptor and tuple data messages */

    status = sc0m_allocate(0,
			   sizeof(SCC_GCMSG) + tuple_size,
			   DB_SCF_ID,
			   (PTR)SCS_MEM,
			   SCG_TAG,
			   &block);
    data_message = (SCC_GCMSG *)block;
    if (status)
    {
	sc0e_0_put(status, 0);
	sc0e_0_uput(status, 0);
	scd_note(E_DB_SEVERE, DB_SCF_ID);
	return status;
    }
    /* initialize the message */
    data_message->scg_marea = ((char *) data_message + sizeof(SCC_GCMSG));
    data_message->scg_mdescriptor = (PTR) desc_area;
    data_message->scg_mask = 0;
    data_message->scg_mtype = GCA_TUPLES;
    data_message->scg_msize = tuple_size;

    *datamsg = data_message;
    return (E_DB_OK);
}

/*{
** Name: scs_save_tdesc     - Save this tdesc in SCF storage.
**
** Description:
**      This routine allocates a tuple descriptor for later use in
**	SCF memory.  If the existing tuple descriptor is large enough
**	then it is reused, otherwise the existing descriptor is freed
**	and a new tuple descriptor is allocated.
**
** Inputs:
**      scb                             Session control block
**	descmsg				Ptr to tuple descriptor message
**
** Outputs:
**      scb				sscb_cquery.cur_rdesc.rd_tdesc is updated
**
**	Returns:
**	    GCA status
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-apr-1997 (shero03)
**          Created.
**	14-may-1997 (nanpr01)
**	    Will cause memory leak.
**	14-nov-1997 (wonst02)
** 	    Bug 86490 - Varchar compression and cursors
** 	    Save tdesc in sscb_cquery only when scb ptr is supplied.
**	21-Dec-01 (gordy)
**	    GCA_COL_ATT no longer defined with DB_DATA_VALUE.
[@history_template@]...
*/
static PTR
scs_save_tdesc(
	SCD_SCB	    *scb,
	PTR 	    desc)
{
    i4			new_desc_size = 0;
    i4			name_length;
    i4			i;
    GCA_TD_DATA		*old_desc = NULL;
    GCA_TD_DATA		*new_desc;
    DB_STATUS		status;
    GCA_COL_ATT		*next_desc;
    PTR			block;
    PTR			tdesc;
    
    if (scb)
    	old_desc = (GCA_TD_DATA*)scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc;
    new_desc = (GCA_TD_DATA *)desc;

    if (desc == NULL)	/* no new tuple descriptor */
    {			/* free any existing tuple descriptor */
    	if (old_desc)
	{
	    block = (char *)old_desc - sizeof(SC0M_OBJECT);
	    status = sc0m_deallocate(0, &block);
	    if (scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc == (PTR)old_desc)
	    	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_tdesc = NULL;
	    scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc = NULL;
	    scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size = 0;
	}
    	return (PTR)NULL;
    }

    /* calculate size of new tuple descriptor and object header */
    new_desc_size = sizeof (GCA_TD_DATA) - sizeof(GCA_COL_ATT)
      			 + sizeof(SC0M_OBJECT);
      
    next_desc = new_desc->gca_col_desc;
    for (i = 0; i < new_desc->gca_l_col_desc; i++)
    {
	 if (new_desc->gca_result_modifier & GCA_NAMES_MASK)
	 { 
#ifdef BYTE_ALIGN
	     MEcopy((PTR)&next_desc->gca_l_attname, sizeof(i4),
	     			(PTR)&name_length);
#else
	     name_length = next_desc->gca_l_attname;
#endif
	 }
	 else
	    name_length = 0;
    	 new_desc_size += name_length + 
	     sizeof(new_desc->gca_col_desc[0].gca_attdbv) + 
	     sizeof(new_desc->gca_col_desc[0].gca_l_attname);
	 next_desc = (GCA_COL_ATT *)
	    ((char *) next_desc + name_length +
		sizeof(new_desc->gca_col_desc[0].gca_attdbv) + 
		sizeof(new_desc->gca_col_desc[0].gca_l_attname));
    } 	

    if (old_desc)
    {
	if (scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size >= new_desc_size)
	{
	    /* 
	    ** We just need to copy the new descriptor in the buffer
	    */ 
	    MEcopy(desc, new_desc_size - sizeof(SC0M_OBJECT), (PTR)old_desc);
	    return (PTR)old_desc;
	}
	/* Not enough ... We need more */
	block = (char *)old_desc - sizeof(SC0M_OBJECT);
	status = sc0m_deallocate(0, &block);
        scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc = NULL;
        scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size = 0;
    }

    /* allocate and format tuple descriptor and tuple data messages */

    status = sc0m_allocate(0,
    		   new_desc_size,
    		   DB_SCF_ID,
    		   (PTR)SCS_MEM,
    		   SCG_TAG,
    		   &block);
    if (status)
    {
        sc0e_0_put(status, 0);
        sc0e_0_uput(status, 0);
        scd_note(E_DB_SEVERE, DB_SCF_ID);
        return (PTR)NULL;
    }
    tdesc = (PTR)((char *)block + sizeof(SC0M_OBJECT));

    if (scb)
    {
    	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size = new_desc_size;
    	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc = tdesc;
    }

    /* copy in tuple descriptor */

    MEcopy(desc, new_desc_size - sizeof(SC0M_OBJECT), tdesc);

    return tdesc;
}

/*{
** Name: scs_chkretry - check for special retry errors on return from QEF to
**                     continue processing.
**
** Description:
**      This routine is called on return from QEF after PSQ_GRANT,
**      PSQ_RULE, PSQ_INDEX, and PSQ_PROCEDURE execute immediates have been
**	processed.
**
**      On return from QEF, we need to check for special retry error like
**      PARSE_EXECIMM_QTEXT to continue pocessing.
**
**      In case of Execute Immdiates, we check if we need to parse E.I. text.
**
** Inputs:
**      qe_ccb                 QEF Request Control Block
**      qef_wants_eiqp         indicates that QEF has asked for parsing of
**                             E.I. qtext.
**                             if set by this routine,then text needs to be
**                             parsed.
**      scb                   session control block for this session
**      status                output of qef_call()
**
** Outputs:
**	qe_ccb
**          .qef_qsf_handle     next E.I qtext to be parsed
**      scb
**          .sscb_thandle       next E.I. qtext to be parsed copied
**                              from QEF_RCB
**
**      Returns:
**          DB_STATUS
**      Exception:
**          none
**
** Side Effects:
**      none.
**
** History:
**	27-dec-92 (anitap)
**          Written for CREATE SCHEMA, FIPS constraints and CREATE VIEW
**          (WCO) projects.
**	03-mar-93 (anitap)
**	    Changed declaration of scs_chkretry() to static void.
**	19-mar-93 (rickh)
**	    Added PSQ_INDEX to the header comments since we can execute
**	    immediate CREATE INDEX statements, too.
**	23-jun-93 (anitap)
**	    Moved qef_call() from scs_sequencer into scs_chkretry().
**	2-jul-93 (rickh)
**	    qef_wants_eiqp (a pointer) was not being dereferenced.
**	06-jul-93 (rblumer)
**	    must break out if already had an error BEFORE this procedure was
**	    called; else can end up calling error handler with bogus error
**	    number, which shows up as an internal error.
**	22-jul-93 (anitap)
**	    We do not want to assign the handle to the E.I. query text to 
**	    sscb_thandle because we would stamp on the handle for the original
**	    query text for CREATE SCHEMA/CREATE TABLE with constraints/CREATE 
**	    VIEW(WCO).
*/

static void
scs_chkretry(
	SCD_SCB         *scb,
	DB_STATUS       status,
	DB_STATUS       *ret_val,
	i4             *cont,
	i4             **next_op,
	i4             *qef_wants_eiqp)
{
	QEF_RCB		*qe_ccb = scb->scb_sscb.sscb_qeccb;

        for(;;)
        {
	   if ((!status) && (*qef_wants_eiqp))
           {
              /* We must call back QEF with the original RCB
              ** intact.
              */
	      status = qef_call(QEQ_QUERY, qe_ccb);
	   }
	   else
	   {
	       break;	/* either had an error BEFORE this routine was called,
			** or don't have an execute-immediate query,
			** so just break and return immediately.
			*/
	   }
	       
           if (!status)
	   {
           
              qe_ccb->error.err_code = 0;
              scb->scb_sscb.sscb_cfac = DB_SCF_ID;
           }
           if (DB_SUCCESS_MACRO(status))
           {
              *qef_wants_eiqp = 0;
              scb->scb_sscb.sscb_noretries = 0;

              if (qe_ccb->qef_remnull)
                scb->scb_sscb.sscb_cquery.cur_result |= GCA_REMNULS_MASK;

              scb->scb_sscb.sscb_state = SCS_INPUT;
              **next_op = CS_EXCHANGE;

              /* FIXME: qef_rowcount needed? */
           }
           else
           {
              if ((qe_ccb->qef_intstate & QEF_EXEIMM_PROC)
                       && (qe_ccb->error.err_code ==
                          E_QE0303_PARSE_EXECIMM_QTEXT))
              {
                 *qef_wants_eiqp = 1;
                 scb->scb_sscb.sscb_noretries = 0;

                 *cont = 1;
              }
	      else
	      {
                 scs_qef_error(status, qe_ccb->error.err_code, DB_SCF_ID, scb);
                 *ret_val = E_DB_OK;

                 scb->scb_sscb.sscb_state = SCS_INPUT;
                 STRUCT_ASSIGN_MACRO(qe_ccb->qef_comstamp,
                   scb->scb_sscb.sscb_comstamp);              
	         **next_op = CS_EXCHANGE;
              }
	   }
           break;
    }
}
/*
** {
** Name:	scs_lowksp_alloc - Alloc large object workspace
**
** Description:
**	This routine allocates large object workspaces.  If the
**      session has never allocated one, then it is allocated and
**      stored in the sscb_lowksp area of the session control block.
**      If this session has already done so, then the known lowksp is
**      returned.
**
**	A blob couponify or redeem might be in progress when this
**	routine is called, so don't hurt anything that will confuse
**	an in-progress blob operation (such as db-blob-wksp flags).
**
** Re-entrancy:
**	Yes.
**
** Inputs:
**	scb             SCF's session control block address
**      lowkspp         Pointer to local vble into which to return the space
**
** Outputs:
**	scb->scb_sscb.sscb_lowksp
**                      Filled with the appropriate pointer if
**                      necessary
**      *lowkspp        Filled with a pointer to the workspace.
**
** Returns:
**	DB_STATUS
**
** Exceptions: (if any)
**	None.
**
** Side Effects:
**      None
**
** History:
**	24-Feb-1993 (fred)
**          Created (separated from scs_blob_fetch() so it can be
**          easily used by scs_cp_redeem().
**      10-Nov-1993 (fred)
**          Added from_copy parameter.  If called from copy, then any
**          temps created via this routine will be short rather than
**          long term, and can be destroyed at the end of the row
**          being processed.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	24-Mar-1998 (thaju02)
**	    Bug #87880 - inserting or copying blobs into a temp table chews
**	    up cpu, locks and file descriptors. If copy from query, then
**	    set pop_temporary to ADP_POP_FROM_COPY else setup pop_temporary
**	    to be short term temporary table (ADP_POP_SHORT_TEMP).
**      29-Jun-98 (thaju02)
**          Regression bug: with autocommit on, temporary tables created
**          during blob insertion, are not being destroyed at statement
**          commital, but are being held until session termination.
**          Regression due to fix for bug 87880. (B91469)
**	8-apr-99 (stephenb)
**	    Init pop_info field
**	11-May-2004 (schka24)
**	    Simplify pop-temporary setting.
**	21-Apr-2010 (kschendel) SIR 123485
**	    Always allocate a db-blob-wksp with the rest of it.
**	    Don't hurt anything related to query-in-progress.
*/
DB_STATUS
scs_lowksp_alloc(SCD_SCB    	*scb,
		 PTR     	*lowkspp)
{
    DB_STATUS 	status;
    ADP_LO_WKSP *lowksp;

    if (!scb->scb_sscb.sscb_lowksp)
    {
	/* ADP_LO_WKSP includes the usual scf/dmf header. */
	status = sc0m_allocate(0,
			       sizeof(ADP_LO_WKSP)
				+ sizeof(DB_BLOB_WKSP)
				+ Sc_main_cb->sc_maxtup,
			       DB_SCF_ID,
			       (PTR)ADP_ADW_TYPE,
			       ADP_ADW_ASCII_ID,
			       &scb->scb_sscb.sscb_lowksp);
	if (status)
	    return (status);
    }

    lowksp = (ADP_LO_WKSP *) scb->scb_sscb.sscb_lowksp;
    lowksp->adw_fip.fip_pop_cb.pop_info = (PTR) (lowksp + 1);
    lowksp->adw_fip.fip_work_dv.db_data =
	    (char *) lowksp->adw_fip.fip_pop_cb.pop_info + sizeof(DB_BLOB_WKSP);
    lowksp->adw_fip.fip_work_dv.db_length = Sc_main_cb->sc_maxtup;
    lowksp->adw_fip.fip_work_dv.db_prec = 0;
    *lowkspp = (PTR) lowksp;
    return(E_DB_OK);
}

/*{
** Name: scs_def_curs 	- process cursor definition
**
** Description:
**	This function will handle definition/opening of a cursor after the query
**	defining the cursor has been parsed and optimized.  For regular
**	(non-repeat) cursors, defining a cursor also means opening it, but for
**	repeat cursors the two events (defining a cursor and opening a cursor)
**	are distinct: the first occurs after PSF parses
**	DEFINE CURSOR FOR REPEATED SELECT and the second occurs when we are told
**	to EXECUTE a query plan representing a DEFINE CURSOR FOR REPEATED SELECT
**	query.
**
**	Defining a repeat cursor will involve:
**	- sending to FE a GCA_QC_ID message with ID of the QP
**
**	Defining a non-repeat cursor and opening a repeat cursor will involve:
**	- calling QEF to open a cursor (QEQ_OPEN),
**	- allocating an SCF cursor info block,
**	- sending to FE a GCA_QC_ID message with ID of the QP, and
**	- populating the SCF cursor info block
**
** Inputs:
**	scb		    session CB
**	qmode		    query mode will indicate whether we are defining or
**			    opening a cursor (which are one and the same if we
**			    are dealing with a non-repeat cursor, but are quite
**			    different for repeat cursors);
**	    PSQ_DEFCURS	    handle cursor definition of a cursor
**	    PSQ_EXECUTE	    handle opening of a repeat cursor
**	curs_id		    query plan id
**	handle		    query plan object QSF handle
**	qp_cb		    query plan CB (true, I could get it using handle,
**			    but by the time this function gets called, QP CB
**			    will have already been obtained)
**
** Outputs:
**	scb				State within sequencer may be altered.
**	message				message pointer may get reset to NULL if
**					an error occurs
**	next_op				what operation is expected next
**	qry_status			status of the query (may be reset to
**					GCA_FAIL_MASK if an error occurs)
**	retry_qry			set if QEQ_OPEN failed with error
**					E_QE0023_INVALID_QUERY or
**					E_QE0301_TBL_SIZE_CHANGE, so the query
**					needs to be retried
**	ret_val				return status from preceeding calls
**					may get reset to E_DB_OK under some
**					circumstances
**
** Returns:
**	    DB_STATUS
** Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-mar-94 (andre) 
**          plagiarized (with many amendments) from scs_sequencer()
**	17-mar-94 (anitap)
**	    Fixed bug where a repeat cursor query with host variables hung
**	    while opening repeat cursor. SCF was not passing the number of
**	    parameters to QEF. Thus QEF could not process execute opening
**	    of repeat cursor.
**	17-mar-94 (andre)
**	    Additional changes are required to handle repeat cursor parameters:
**	    unlike repeat query parameters which become uninteresting as soon 
**	    as the query completes, we must save repeat cursor parameters and 
**	    information about them so it can be made available to QEQ_FETCH.
**
**	    There are two cases to consider WRT saving repeat cursor parameters:
**	      - if the repeat cursor parameters spanned multiple messages, they 
**	    	and the array of pointers pointing at them will be placed in QSF
**	    	memory.  Ordinarily, this memory is deallocated at the end of 
**		the query, but with OPEN repeat CURSOR we need to make an 
**		exception and hold on to that memory until the cursor is closed.
**		In order to prevent SCF from deallocating that memory, if 
**		sscb_thandle is non-zero, we will (in scs_def_curs()) copy 
**		sscb_thandle into the cursor block and then reset it and 
**		sscb_troot (to prevent SCF from trying to deallocate memory 
**		through the root which will most likely fail because the memory
**		was not allocated using sc0m_allocate()).
**
**	      - if, on the other hand, cursor parameters could all fit inside 
**		the message, we will (inside scs_def_curs()) allocate QSF memory
**		to contain the array of pointers and a copy of the message, 
**		adjust copied pointers to reflect the fact that instead of 
**		pointing to the message buffer they now point inside the 
**		allocated QSF memory, and store the handle for the QSF object 
**		inside the cursor block.
**	17-mar-94 (andre)
**	    A minor oversight: we were using QEF_RCB that lives inside SCF
**	    session CB, but in some cases it was populated by the caller to
**	    lock the QP object and the caller expected that info to remain
**	    intact.  To correct this, we will use a local copy of QSF_RCB
**	21-mar-94 (andre)
**	    do not try to allocate memory for repeat cursor parameters if the 
**	    parameter count is 0
**	15-dec-95 (pchang)
**	    Set the query status to GCA_REMNULS_MASK after opening the cursor 
**	    in QEF, if null values were eliminated when evaluating aggregate 
**	    functions.  This will ensure that the SQLSTATE warning 
**	    SS01003_NULL_ELIM_IN_SETFUNC is correctly issued to the FE (B73095).
**	23-Aug-1995 (jenjo02)
**	    Replace references to cs_self with CSget_sid() when session id
**	    is needed.
**	17-feb-1998 (somsa01)
**	    Set CSI_GCA_COMP_MASK on csi_state if GCA_COMP_MASK is set.
**	    (Bug #89000)
**      08-jul-1999 (gygro01)
**	    If a cursor gets the qep invalidated due to QEF_ZEROTUP_CHK,
**	    scb->scb_sscb.sscb_cquery.cur_subopt should be set to TRUE to
**	    bypass flattening on retry. Otherwise it retries twice with
**	    flattening before getting E_US1265.  It get bypassed correctly
**	    in scs_sequencer, but not for cursors in scs_def_curs.
**	    (bug 93313)
**      06-nov-2003 (huazh01)
**          Delay sending out the tuple descriptor to FE until 
**          Star server has a chance to open the query.
**          This fixes bug 107835, INGSTR61.
**	06-dec-2004 (thaju02)
**	    May need the query text should the fetch fail and retry 
**	    must be done. Store query text object handle in the csi 
**	    entry at open cursor time. (B113588)
**	18-aug-2005 (thaju02)
**	    Use local_status in error handling to avoid trashing
**	    status if failure occurs.
**	30-may-06 (dougi)
**	    Add support for cursor text saving under sc924.
**	17-july-2007 (dougi)
**	    Flag scrollable cursors.
**	13-nov-2007 (dougi)
**	    Changes to support repeat dynamic queries.
**	27-feb-2008 (dougi)
**	    Flag repeat dynamic cursors in case of QEQ_OPEN failure.
**	19-march-2008 (dougi)
**	    Above change didn't cover all cases - fix it again.
**      27-Feb-2009 (gefei01)
**          Support cursor select for table procedure.
**	1-Jul-2009 (wanfr01)
**	    Bug 122248 - Need to set GCA_SCROLL_MASK in GCA if the
**	    cursor is scrollable to inform ODBC programs.
**	21-Apr-2010 (kschendel) SIR 123485
**	    Delete pointless fiddling with blob coupon lists, nobody cares.
*/
static DB_STATUS
scs_def_curs(
	SCD_SCB		*scb,
	i4		qmode,
	DB_CURSOR_ID	*curs_id,
	PTR		handle,
	bool		retry_fetch,
	QEF_QP_CB	*qp_cb,
	SCC_GCMSG	**message,
	i4		*qry_status,
	i4		*next_op,
	bool		*retry_qry,
	DB_STATUS	*ret_val)
{
    DB_STATUS	    status = E_DB_OK;
    PSQ_CB	    *ps_ccb = scb->scb_sscb.sscb_psccb;
    QSF_RCB	    qsfrcb;
    QSF_RCB	    *qs_ccb = (QSF_RCB *) NULL;
    QEF_RCB	    *qe_ccb = scb->scb_sscb.sscb_qeccb;
    PTR		    param_handle;
    i4		    param_lk_id;
    QEF_PARAM	    *params = (QEF_PARAM *) NULL;
    i4		    csr_modifier;
    i4		    qry_stat_modifier = 0;
    bool	    defining_rep_curs = FALSE;
    bool	    opening_rep_curs  = FALSE;
    bool	    non_rep_curs = TRUE;
    bool	    qef_cur_opened;
    bool	    qeq_open_failed;
    bool	    psf_cur_opened;
    bool	    destr_qsf_object;
    bool	    success;

    /* For STAR */
    SCF_TD_CB	    *rqf_td_desc = NULL;

    csr_modifier = 0;
    if (!(qp_cb->qp_status & (QEQP_DELETE|QEQP_UPDATE)))
    {
        csr_modifier |= CSI_READONLY_MASK;
    }
    if (qp_cb->qp_status & QEQP_SINGLETON)
    {
        csr_modifier |= CSI_SINGLETON_MASK;
    }
    if (qp_cb->qp_status & QEQP_RPT)
    {
	/* This should only happen for repeat dynamic queries, though it
	** was originally written for more general support of repeat cursors. */
        csr_modifier |= CSI_REPEAT_MASK;

	/* we are either defining or opening a repeat cursor */
	defining_rep_curs = qmode == PSQ_DEFCURS;
	opening_rep_curs = !defining_rep_curs;
	non_rep_curs = FALSE;

	/* For repeat dynamic queries, parms have already been set up in 
	** QEF_RCB. */
	params = qe_ccb->qef_param;
	param_handle = (params) ? scb->scb_sscb.sscb_thandle : (PTR) NULL;
	param_lk_id = (params) ? scb->scb_sscb.sscb_tlk_id : 0;
    }

    *retry_qry = FALSE;

    scb->scb_sscb.sscb_cquery.cur_row_count = -1;

    if (qp_cb->qp_status & QEQP_SCROLL_CURS)
	scb->scb_sscb.sscb_cquery.cur_scrollable = TRUE;
    else scb->scb_sscb.sscb_cquery.cur_scrollable = FALSE;

    for (success = FALSE; !success; success = TRUE)
    {
	/*
	** if we are processing DEFINE non-repeat cursor, PSF has already marked
	** this cursor as opened in its cursor block list, so we must rememeber
	** to close it shoudl something go wrong here
	*/
	psf_cur_opened = non_rep_curs;

	/* QEF is yet to be called to open a cursor */
	qef_cur_opened = qeq_open_failed = FALSE;

	/* QSF object is yet (if at all) to be created */
	destr_qsf_object = FALSE; 
	
	/*
	** if opening a repeat cursor, we need to call PSF to mark its
	** cursor block "opened"
	**
	** From the existing logic, it appears that opening_rep_curs
	** should never be set.
	*/
	if (opening_rep_curs)
	{
	    STRUCT_ASSIGN_MACRO((*curs_id), ps_ccb->psq_cursid);
	    status = psq_call(PSQ_OPEN_REP_CURS, ps_ccb,
	        scb->scb_sscb.sscb_psscb);
	    if (DB_FAILURE_MACRO(status))
	    {
		if (ps_ccb->psq_error.err_code == E_PS0401_CUR_NOT_FOUND)
		{
		    /*
		    ** PSF cursor block was nowhere to be found - most likely
		    ** this means that it has been reused while the cursor was
		    ** left opened - we need to persuade FE send us the cursor
		    ** definition so that we can reparse it and create a new PSF
		    ** cursor block
		    */
		    qry_stat_modifier = GCA_RPQ_UNK_MASK;
		}
		else
		{
		    sc0e_0_put(E_SC0215_PSF_ERROR, 0);
		    sc0e_0_put(ps_ccb->psq_error.err_code, 0);
		    sc0e_0_uput(ps_ccb->psq_error.err_code, 0);
		    scd_note(status, DB_PSF_ID);
		}

		break;
	    }

	    /*
	    ** remember that we marked the PSF's cursor block as opened so that,
	    ** if some error happens, we will remember to call PSF to
	    ** PSQ_CLSCURS it
	    */
	    psf_cur_opened = TRUE;
	}

	{
	    GCA_ID	    *gca_id = (GCA_ID *) scb->scb_cscb.cscb_tuples;

	    *message = scb->scb_cscb.cscb_outbuffer;

            /* Message not new if openning the same cursor
             * after recompiling/reloading tproc QP
             */
/*            if (qe_ccb->error.err_code != E_QE030F_LOAD_TPROC_QP) */
            if ((qe_ccb->qef_intstate & QEF_DBPROC_QP) == 0)
            {
	        (*message)->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
	        (*message)->scg_msize = sizeof(GCA_ID);
	        (*message)->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
	        scb->scb_cscb.cscb_mnext.scg_prev =
		scb->scb_cscb.cscb_mnext.scg_prev->scg_next = *message;
	        (*message)->scg_mask = SCG_NODEALLOC_MASK;
	        (*message)->scg_mtype = GCA_QC_ID;
	    
	        gca_id->gca_index[0] = curs_id->db_cursor_id[0];
	        gca_id->gca_index[1] = curs_id->db_cursor_id[1];

	        /* DON'T assume GCA_MAXNAME > DB_CURSOR_MAXNAME */
	        MEmove(sizeof(curs_id->db_cur_name),
                       (PTR)&curs_id->db_cur_name,' ',
	               sizeof(gca_id->gca_name), (PTR)&gca_id->gca_name);
            }
	}

	/*
	** if defining a non-repeat cursor or opening a repeat cursor, tell QEF
	** to open it and, if successful, find a slot in the SCF cursor info
	** list and build an entry describing this new cursor
	*/
	/* Pass the actual number of parameters to QEF in case of host
	** variables which are treated like repeat query parameters.
	*/

	/* Once we get here, the following block should always be executed,
	** so the "if" has been removed. */
	/* if (non_rep_curs || opening_rep_curs) */
	{
	    SCC_GCMSG	    *cmsg;
	    SCS_CSI	    *csi_entry;
	    i4		    max_cursors = scb->scb_sscb.sscb_cursor.curs_limit;
	    i4		    csi_no;

	    /*
	    ** if opening a repeat cursor, we need to ensure that repeat
	    ** parameters, if any, are safely stored in QSF until the cursor is
	    ** closed and the QSF object's handle and lock id are stored in 
	    ** *csi_entry + we have to save a copy of QEF_PARAM structure 
	    ** describing the parameters.  If there are no parameters, we will 
	    ** initialize csi_rep_parm_handle, csi_rep_parm_lk_id, and
	    ** csi_entry->csi_rep_parms accordingly
	    **
	    ** From the existing logic, it appears that opening_rep_curs
	    ** should never be set.
	    */
	    if (   opening_rep_curs 
		&& scb->scb_sscb.sscb_param 
	 	&& qe_ccb->qef_pcount > 0)
	    {
		QSF_RCB	    *sess_qs_ccb = scb->scb_sscb.sscb_qsccb;

		/*
		** we need to use a local copy of QSF_RCB whose pointer is found
		** in scb because the scb copy may be pre-initialized with info
		** about the QP object
		*/
		STRUCT_ASSIGN_MACRO((*sess_qs_ccb), qsfrcb);

		qs_ccb = &qsfrcb;

		/* 
		** opening a repeat cursor which uses some repeat parameters;
		**
		** if the parameters and pointers to them were copied into QSF 
		** memory, we simply need to copy sscb_thandle, sscb_tlk_id, and
		** *qe_ccb->qef_param into SCS_CSI structure describing the 
		** newly opened cursor and reset sscb_thandle, sscb_tlk_id, 
		** and sscb_troot to prevent code in scs_sequencer() from 
		** deallocating the object until the cursor is closed.
		** 
		** otherwise, we need to store parameters and pointers to them 
		** in QSF memory (and recompute the pointers to the parameters 
		** since they would be pointing into the message buffer and we 
		** want them to point into the QSF memory) and save the QSF 
		** object handle and lock id in SCS_CSI structure describing the
		** newly opened cursor + we will populate csi_rep_parms in that 
		** SCS_CSI structure (in this case we may not copy 
		** *qe_ccb->qef_param because it points at an array of pointers
		** which was allocated out of SCF memory and will be 
		** deallocated at the end of this query)
		*/
		if (scb->scb_sscb.sscb_thandle)
		{
		    /*
		    ** we need to allocate a QEF_PARAM structure address of 
		    ** which will be included in SCS_CSI structure describing 
		    ** the newly opened cursor out of the same stream as the 
		    ** descriptions of repeat parameters
		    */
		    param_handle = qs_ccb->qsf_obj_id.qso_handle = 
			scb->scb_sscb.sscb_thandle;
		    param_lk_id  = qs_ccb->qsf_lk_id = 
			scb->scb_sscb.sscb_tlk_id;

		    qs_ccb->qsf_sz_piece = sizeof(QEF_PARAM);
		    status = qsf_call(QSO_PALLOC, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			break;
		    }

		    params = (QEF_PARAM *) qs_ccb->qsf_piece;

		    STRUCT_ASSIGN_MACRO(*qe_ccb->qef_param, *params);

		    scb->scb_sscb.sscb_thandle = (PTR) NULL;
		    scb->scb_sscb.sscb_troot   = (PTR) NULL;
		    scb->scb_sscb.sscb_tlk_id  = 0;

		    /*
		    ** having zeroed out sscb_thandle and sscb_tlk_id, it now 
		    ** becomes our responsibility to free up thememory used to 
		    ** store repeat parameters and the QEF_PARAM structure
		    */
		    destr_qsf_object = TRUE;
		}
		else
		{
		    PTR		*param_ptrs, *pp_from, *pp_to;
		    PTR		buf;
		    i4	base_diff;

		    qs_ccb->qsf_obj_id.qso_type = QSO_QTEXT_OBJ;
		    qs_ccb->qsf_obj_id.qso_lname = 0;
		    status = qsf_call(QSO_CREATE, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			break;
		    }

		    /*
		    ** save the new object's handle and lock id; eventually 
		    ** they will be copied into csi_rep_parm_handle and 
		    ** csi_rep_parm_lk_id of the SCS_CSI structure describing 
		    ** the newly opened cursor
		    */
		    param_handle = qs_ccb->qsf_obj_id.qso_handle;
		    param_lk_id  = qs_ccb->qsf_lk_id;

		    /* 
		    ** remember that if something goes wrong, we must destroy 
		    ** the QSF object
		    */
		    destr_qsf_object = TRUE;

		    /*
		    ** allocate memory for parameter pointers (we allocate 1 
		    ** extra pointer because repeat parameters are numbered 
		    ** starting with 1)
		    */
		    qs_ccb->qsf_sz_piece = 
			(qe_ccb->qef_pcount + 1) * sizeof(PTR);
		    status = qsf_call(QSO_PALLOC, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			break;
		    }

		    param_ptrs = (PTR *) qs_ccb->qsf_piece;
		    qs_ccb->qsf_root = qs_ccb->qsf_piece;

		    status = qsf_call(QSO_SETROOT, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			break;
		    }

		    /*
		    ** allocate enough memory to copy the input message - some
		    ** memory will unavoidably be wasted, but at this point 
		    ** information about parameter length may be clobbered (on 
		    ** byte-alligned machines), so what's a few bytes between 
		    ** friends
		    */
		    qs_ccb->qsf_sz_piece = scb->scb_cscb.cscb_gci.gca_d_length;
		    status = qsf_call(QSO_PALLOC, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			break;
		    }
		
		    buf = (PTR) qs_ccb->qsf_piece;

		    /* 
		    ** copy the data area of the GCA_INVOKE message into the 
		    ** newly allocated buffer
		    */
		    MEcopy((PTR)scb->scb_cscb.cscb_gci.gca_data_area,
			    scb->scb_cscb.cscb_gci.gca_d_length, buf);

		    /*
		    ** we need to recompute pointers to parameters using the
		    ** difference between the beginning of the data area in 
		    ** the message and beginning of the QSF buffer
		    */
		    base_diff = (char *) buf - scb->scb_cscb.cscb_gci.gca_data_area;

		    for (pp_to = param_ptrs + qe_ccb->qef_pcount,
			 pp_from = scb->scb_sscb.sscb_param +qe_ccb->qef_pcount;

			 pp_to != param_ptrs;

			 pp_to--, pp_from--
			)
		    {
			*pp_to = (PTR) ((char *) *pp_from + base_diff);
		    }

		    /* 0-th element of the pointer array needs to be set to 0 */
		    *pp_to = (PTR) NULL;

		    /*
		    ** allocate memory for a QEF_PARAM structure whose address 
		    ** will be stored in SCS_CSI structure describing the newly 
		    ** opened cursor
		    */
		    qs_ccb->qsf_sz_piece = sizeof(QEF_PARAM);
		    status = qsf_call(QSO_PALLOC, qs_ccb);
		    if (DB_FAILURE_MACRO(status))
		    {
			scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			    E_SC021C_SCS_INPUT_ERROR);
			sc0e_0_uput(E_SC0206_CANNOT_PROCESS, 0);
			break;
		    }

		    params = (QEF_PARAM *) qs_ccb->qsf_piece;

		    params->dt_next = (QEF_PARAM *) NULL;
		    params->dt_size = qe_ccb->qef_pcount;

		    /* 
		    ** note that in this case we need to overwrite 
		    ** qe_ccb->qef_param->dt_data which was set in 
		    ** scs_sequencer() because it was pointing at an array of 
		    ** pointers which (as well as the memory at which they 
		    ** point) will not persist until the time when the cursor 
		    ** will be closed
		    */
		    qe_ccb->qef_param->dt_data = params->dt_data = param_ptrs;
		}
	    }
	    if (non_rep_curs)
	    {
		/* 
		** opening a non-repeat cursor or a repeat cursor with no 
		** parameters - initialize csi_rep_parm_handle, 
		** csi_rep_parm_lk_id, and csi_rep_parms accordingly
		*/
		param_handle = (PTR) NULL;
		param_lk_id  = 0;
		params = (QEF_PARAM *) NULL;
	    }

	    qe_ccb->qef_eflag = QEF_EXTERNAL;
	    qe_ccb->qef_qacc = QEF_UPDATE;
	    qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
	    qe_ccb->qef_db_id = scb->scb_sscb.sscb_ics.ics_opendb_id;
	    qe_ccb->qef_qso_handle = handle;
	    /* 
	    ** params->dt_size will contain number of repeat parameters, if any,
	    ** specified for the cursor (params will be 0, as well it should be,
	    ** for repeat cursors with no parameters and for non-repeat cursors)
	    */
	    qe_ccb->qef_pcount = (params) ? params->dt_size : 0;
	    
	    if (!(scb->scb_sscb.sscb_flags & SCS_STAR))
	    {
		/* Before OPEN, set psf_cur_opened, if repeat dynamic. */
		if (qp_cb && qp_cb->qp_status & QEQP_REPDYN)
		    psf_cur_opened = TRUE;

	        scb->scb_sscb.sscb_cfac = DB_QEF_ID;
	        status = qef_call(QEQ_OPEN, qe_ccb);
	        if (!status)
		    qe_ccb->error.err_code = 0;
	        scb->scb_sscb.sscb_cfac = DB_SCF_ID;

	        if (DB_FAILURE_MACRO(status))
	        {
		    qeq_open_failed = TRUE;
	            break;
	        }

		if (qe_ccb->qef_remnull)
		    *qry_status |= GCA_REMNULS_MASK;
	        /*
	        ** remember that we called QEF to open the cursor - if something
	        ** goes wrong from this point on, we must remember to 
		** QEQ_CLOSE it
	        */
	        qef_cur_opened = TRUE;
	    }
	    
	    scb->scb_sscb.sscb_noretries = 0;
	    
	    /* find an open cursor slot and install this one */
	    for (csi_entry = scb->scb_sscb.sscb_cursor.curs_csi->csitbl,
	         csi_no = 0;
		 (csi_no < max_cursors && csi_entry->csi_state & CSI_USED_MASK);
		 csi_entry++, csi_no++
		)
	    ;

	    if (   csi_no == max_cursors
		|| (status && (qe_ccb->error.err_code != E_QE0015_NO_MORE_ROWS))
	       )
	    {
		if (status)
		{
		    *message = (SCC_GCMSG *) NULL;
		}
		else
		{
		    sc0e_0_uput(E_SC021D_TOO_MANY_CURSORS, 0);
		}
		
		break;
	    }

	    /* If read-only (determined after OPC) indicate to client */
	    if (csr_modifier & CSI_READONLY_MASK)
	    {
		((GCA_TD_DATA *) qp_cb->qp_sqlda)->gca_result_modifier |=
		    GCA_READONLY_MASK;
	    }

	    /* If scrollable cursor indicate to client */
	    if (qp_cb->qp_status & QEQP_SCROLL_CURS)
	    {
		((GCA_TD_DATA *) qp_cb->qp_sqlda)->gca_result_modifier |=
		    GCA_SCROLL_MASK;
	    }

	    if (scb->scb_sscb.sscb_flags & SCS_STAR)
	    {
		rqf_td_desc =
		    &scb->scb_sscb.sscb_cquery.cur_rdesc.rd_rqf_desc;

		rqf_td_desc->scf_star_td_p = qp_cb->qp_sqlda;
	    }

	    if (scb->scb_sscb.sscb_flags & SCS_STAR)
	    {
		rqf_td_desc->scf_ldb_td_p = qp_cb->qp_sqlda;
		qe_ccb->qef_r3_ddb_req.qer_d10_tupdesc_p = (PTR) rqf_td_desc;

		/* Now open the query */
		scb->scb_sscb.sscb_cfac = DB_QEF_ID;
		status = qef_call(QEQ_OPEN, qe_ccb);
		if (!status)
		    qe_ccb->error.err_code = 0;
		scb->scb_sscb.sscb_cfac = DB_SCF_ID;

	        if (DB_FAILURE_MACRO(status))
	        {
		    qeq_open_failed = TRUE;
	            break;
	        }

		if (qe_ccb->qef_remnull)
		    *qry_status |= GCA_REMNULS_MASK;
	        /*
	        ** remember that we called QEF to open the cursor - if something
	        ** goes wrong from this point on, we must remember to 
		** QEQ_CLOSE it
	        */
	        qef_cur_opened = TRUE;
	    }

            status = scs_desc_send(scb, &csi_entry->csi_tdesc_msg);

            cmsg = (SCC_GCMSG *) csi_entry->csi_tdesc_msg;
            if (cmsg != NULL)
            {
                cmsg->scg_mask &= ~(SCG_QDONE_DEALLOC_MASK);
            }

	    /* Mark cursor in use + any modifiers already collected */
	    csi_entry->csi_state = CSI_USED_MASK | csr_modifier;
	    STRUCT_ASSIGN_MACRO((*curs_id), csi_entry->csi_id);
	    csi_entry->csi_qp = handle;
	    csi_entry->csi_rdesc.rd_tdesc = (PTR)qp_cb->qp_sqlda;
            if (((GCA_TD_DATA *)csi_entry->csi_rdesc.rd_tdesc)->
	    	gca_result_modifier & GCA_COMP_MASK)
	    {
	        csi_entry->csi_rdesc.rd_tdesc = 
		  scs_save_tdesc((SCD_SCB *)NULL, csi_entry->csi_rdesc.rd_tdesc);
		csi_entry->csi_state |= CSI_GCA_COMP_MASK;
	    }
	        

	    if (!(scb->scb_sscb.sscb_flags & SCS_STAR))
	    {
	        csi_entry->csi_rdesc.rd_modifier =
		    ((GCA_TD_DATA *) qp_cb->qp_sqlda)->gca_result_modifier;
	        csi_entry->csi_tsize =
		    ((GCA_TD_DATA *) qp_cb->qp_sqlda)->gca_tsize;
	    }
	    else
	    {
	        csi_entry->csi_rdesc.rd_modifier =
		    ((GCA_TD_DATA *) rqf_td_desc->scf_ldb_td_p)->
			gca_result_modifier;
	        csi_entry->csi_tsize =
		    ((GCA_TD_DATA *) rqf_td_desc->scf_ldb_td_p)->gca_tsize;
	        STRUCT_ASSIGN_MACRO(*rqf_td_desc, 
				    csi_entry->csi_rdesc.rd_rqf_desc);
	    }
	    scb->scb_sscb.sscb_ciu = 1;
	    STRUCT_ASSIGN_MACRO(qe_ccb->qef_comstamp, 
			        scb->scb_sscb.sscb_comstamp);

	    csi_entry->csi_rep_parm_handle = param_handle;
	    csi_entry->csi_rep_parm_lk_id  = param_lk_id;
	    csi_entry->csi_rep_parms = params;

	    /* store the query text handle in csi_entry */
	    if ((qmode == PSQ_DEFCURS) && (!retry_fetch))
	    {
		csi_entry->csi_thandle = scb->scb_sscb.sscb_thandle;
		csi_entry->csi_tlk_id = scb->scb_sscb.sscb_tlk_id;
	    }
	    else
	    {
		csi_entry->csi_thandle = (PTR)NULL;
		csi_entry->csi_tlk_id = 0;
	    }
	    /* Copy cursor text if we're debugging. */
	    /* if (Sc_main_cb->sc_trace_errno != 0) */
	    {
		csi_entry->csi_textl = ((PSQ_QDESC *)scb->scb_sscb.sscb_troot)
				->psq_qrysize;
		if (csi_entry->csi_textl > ER_MAX_LEN)
		    csi_entry->csi_textl = ER_MAX_LEN;

		if (csi_entry->csi_textl && csi_entry->csi_textp)
		    MEcopy(((PSQ_QDESC *)scb->scb_sscb.sscb_troot)->
			psq_qrytext, csi_entry->csi_textl,
			(PTR)csi_entry->csi_textp);
		else csi_entry->csi_textl = 0;
	    }
	    /* else csi_entry->csi_textl = 0; */
	}
    }

    if (!success && qe_ccb->error.err_code == E_QE030F_LOAD_TPROC_QP)
        return status;

    if (!success)
    {
        DB_STATUS	local_status = E_DB_OK;

	/* remember that something has failed */
	*qry_status = GCA_FAIL_MASK | qry_stat_modifier;
	
	/*
	** we left the loop because of some error - check if we
	** need to clean things up
	*/
	if (psf_cur_opened)
	{
	    STRUCT_ASSIGN_MACRO((*curs_id), ps_ccb->psq_cursid);
	    ps_ccb->psq_flag &= ~PSQ_CLSALL;
	    local_status = psq_call(PSQ_CLSCURS, ps_ccb, scb->scb_sscb.sscb_psscb);
	    if (DB_FAILURE_MACRO(local_status))
	    {
		sc0e_0_put(E_SC0215_PSF_ERROR, 0);
		sc0e_0_put(ps_ccb->psq_error.err_code, 0);
		sc0e_0_uput(ps_ccb->psq_error.err_code, 0);
		scd_note(local_status, DB_PSF_ID);
	    }
	}

	if (destr_qsf_object)
	{
	    qs_ccb->qsf_obj_id.qso_handle = param_handle;
	    qs_ccb->qsf_lk_id = param_lk_id;
	    local_status = qsf_call(QSO_DESTROY, qs_ccb);
	    if (DB_FAILURE_MACRO(local_status))
	    {
		scs_qsf_error(local_status, qs_ccb->qsf_error.err_code,
		    E_SC0210_SCS_SQNCR_ERROR);
	    }
	}

	if (qef_cur_opened)
	{
	    scb->scb_sscb.sscb_cfac = DB_QEF_ID;
	    local_status = qef_call(QEQ_CLOSE, qe_ccb);
	    scb->scb_sscb.sscb_cfac = DB_SCF_ID;
	}
	else if (qeq_open_failed)
	{
	    if (scb->scb_sscb.sscb_interrupt)
		return(status);

	    *ret_val = E_DB_OK;

	    if (   qe_ccb->error.err_code == E_QE0023_INVALID_QUERY
		|| qe_ccb->error.err_code == E_QE0301_TBL_SIZE_CHANGE)
	    {
		*retry_qry = TRUE;

		if (qe_ccb->qef_intstate & QEF_ZEROTUP_CHK)
		{
		    scb->scb_sscb.sscb_cquery.cur_subopt = TRUE;
		    qe_ccb->qef_intstate &= ~QEF_ZEROTUP_CHK;
		}

	        if (opening_rep_curs)
		{
		    /* need to ask FE for the cursor definition text */
		    *qry_status |= GCA_RPQ_UNK_MASK;
		}
		else
		{
		    /* will reparse internally available query text */
		    return(status);
		}
	    }
	    else if (qe_ccb->error.err_code != E_QE030F_LOAD_TPROC_QP)
	    {
                /* E_QE030F_LOAD_TPROC_QP is not an error.
                 * Table procedure QP wii be recompiled and reloaded. */
		scs_qef_error(status, qe_ccb->error.err_code, DB_SCF_ID, scb);
	    }
	}
    }

    scb->scb_sscb.sscb_state = SCS_INPUT;
    *next_op = CS_EXCHANGE;

    return(status);
}

/*
** Name: scs_set_session
**
** Description:
**	This routine implements the SET SESSION operation.
**
** Inputs:
**	scb
**
**	ps_ccb		PSQ_CB describing the statement
**
** Outputs:
**	none
**
** History:
**	30-jul-93 (robf)
**	   Initial Creation
**	27-Feb-1997 (jenjo02)
**	    Extended <set session> statement to support <session access mode>
**	    (READ ONLY | READ WRITE).
**      14-oct-1997 (stial01)
**          scs_set_session() Extended for set session isolation.
**	29-Nov-1999 (jenjo02)
**	    "status" was being cleared by call to QSO_DESTROY.
**      11-Feb_2000 (hayke02) Bug 99976 INGSRV 32
**          Disable setting of dm_ccb elements for 'set session isolation
**          read write | only' (PSQ_SET_ACCESSMODE) if this is a STAR
**          session - dm_ccb (scb->scb_sscb.sscb_dmccb) will be NULL.
**      19-Jan-2004 (horda03) Bug 111047/ INGSRV 2541
**          Raise error E_SC039A if SET SESSION <access mode> issued in a
**          Multi-Statement transaction.
**	20-Apr-2004 (jenjo02)
**	    Add SET SESSION WITH ON_LOGFULL ...
**	22-Jul-2005 (thaju02)
**	    Add SET SESSION WITH ON_USER_ERROR = ROLLBACK TRANSACTION.
**	20-Aug-2008 (kibro01) b120784
**	    Remove erroneous break from end of PSQ_SET_ACCESSMODE "if" section
**	    since that stops "set session read write, isolation level..."
**	    from taking effect by skipping over the isolation level check.
*/
static DB_STATUS
scs_set_session(
    SCD_SCB             *scb,
    PSQ_CB		*ps_ccb
)
{
    DB_STATUS 	status=E_DB_OK;
    QSF_RCB	*qs_ccb = 0;
    QEF_RCB	*qe_ccb;
    DMC_CB	*dm_ccb;
    i4		qsf_object = 0;

    /*
    ** Check each option in turn and process
    */
    for(;;)
    {
	/*
	** Get infoblock
	*/
	qs_ccb = scb->scb_sscb.sscb_qsccb;
	qs_ccb->qsf_obj_id.qso_handle = ps_ccb->psq_result.qso_handle;
	qs_ccb->qsf_lk_state = QSO_EXLOCK;
	status = qsf_call(QSO_LOCK, qs_ccb);
	if (DB_FAILURE_MACRO(status))
	{
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
		E_SC0210_SCS_SQNCR_ERROR);
		sc0e_0_put(E_SC033C_SET_SESSION_ERR, 0);
		break;
	}
	qsf_object = 1;

        if(ps_ccb->psq_ret_flag&(PSQ_SET_ON_ERROR|PSQ_SET_ACCESSMODE|
                                 PSQ_ON_USRERR_ROLLBACK|PSQ_ON_USRERR_NOROLL))
        {
	    /*
	    ** Verify that there is no transaction outstanding.
	    ** Return an error if there is one.
	    */
	    qe_ccb = scb->scb_sscb.sscb_qeccb;
	    status = qef_call(QEC_INFO, qe_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		scs_qsf_error(status, qs_ccb->qsf_error.err_code,
			E_SC0210_SCS_SQNCR_ERROR);
		sc0e_0_put(E_SC033C_SET_SESSION_ERR, 0);
		break;
	    }

	    if (qe_ccb->qef_stat != QEF_NOTRAN)
	    {
		if(ps_ccb->psq_ret_flag&PSQ_SET_ACCESSMODE)
		{
	           sc0e_uput(E_SC039A_SET_SESSION_READWRITE_ERR, 0, 0,
	            0, (PTR)0,
	            0, (PTR)0,
	            0, (PTR)0,
	            0, (PTR)0,
	            0, (PTR)0,
	            0, (PTR)0);
                }
                else
                {
	           sc0e_uput(E_SC031C_ONERR_XCT_INPROG, 0, 0,
	            0, (PTR)0,
	            0, (PTR)0,
	            0, (PTR)0,
	            0, (PTR)0,
	            0, (PTR)0,
	            0, (PTR)0);
                }

		sc0e_0_put(E_SC033C_SET_SESSION_ERR, 0);
		status=E_DB_ERROR;
	        break;
	    }

            if (ps_ccb->psq_ret_flag & PSQ_ON_USRERR_ROLLBACK)
                scb->scb_sscb.sscb_flags |= SCS_ON_USRERR_ROLLBACK;

            else if (ps_ccb->psq_ret_flag & PSQ_ON_USRERR_NOROLL)
                scb->scb_sscb.sscb_flags &= ~SCS_ON_USRERR_ROLLBACK;

            else if (ps_ccb->psq_ret_flag&PSQ_SET_ON_ERROR)
            {
	        /*
	        ** Call qec_alter to register error handling mode.
	        */
	        qe_ccb = (QEF_RCB *) qs_ccb->qsf_root;
	        qe_ccb->qef_cb = scb->scb_sscb.sscb_qescb;
	        status = qef_call(QEC_ALTER, qe_ccb);
	        if (DB_FAILURE_MACRO(status))
	        {
		    scs_qef_error(status, qe_ccb->error.err_code,
	    	     E_SC0210_SCS_SQNCR_ERROR, scb);
		    sc0e_0_put(E_SC033C_SET_SESSION_ERR, 0);
		    break;
	        }
            }
	}

	/* Check if privilege mask is changing */
        if(ps_ccb->psq_ret_flag&PSQ_SET_PRIV)
	{
	    /*
	    ** Set session privileges.
	    */
	    status=scs_set_priv(scb, ps_ccb);

	    if(status!=E_DB_OK)
		break;

	}

	/* Check if setting priority */
        if(ps_ccb->psq_ret_flag&PSQ_SET_PRIORITY)
	{
	    i4  priority;
	    /*
	    ** Set session priority.
	    */
	    /*
	    ** Check for initial/maximum/minimum priority
	    */
	    if(ps_ccb->psq_ret_flag&PSQ_SET_PRIO_INITIAL)
	    {
		/*
		** This matches with action in scs_initiate(), the
		** initial priority is the lower of (limit, normal) 
		** priorities, where normal is defined as 0.
		*/
		priority=scb->scb_sscb.sscb_ics.ics_priority_limit; 
		if(priority>0)
			priority=0;
	    }
	    else if(ps_ccb->psq_ret_flag&PSQ_SET_PRIO_MAX)
		priority=scb->scb_sscb.sscb_ics.ics_priority_limit; 
	    else if(ps_ccb->psq_ret_flag&PSQ_SET_PRIO_MIN)
		priority = Sc_main_cb->sc_min_usr_priority-
				Sc_main_cb->sc_norm_priority;
	    else
		priority=ps_ccb->psq_priority;

	    status=scs_set_priority(scb, priority);

	    if(status!=E_DB_OK)
		break;
	}
        if(ps_ccb->psq_ret_flag&PSQ_SET_DESCRIPTION)
	{
	    i4  len;
	    /*
	    ** Set session description
	    */
	    len=STlength(ps_ccb->psq_desc);
	    if(len>=SCS_DESC_SIZE)
		len=SCS_DESC_SIZE;
	    scb->scb_sscb.sscb_ics.ics_l_desc=len;
	    if(len>0)
	    {
	        MEcopy((PTR)ps_ccb->psq_desc, len,
	    		(PTR)scb->scb_sscb.sscb_ics.ics_description);
	    }
	    if(len<SCS_DESC_SIZE)
	    {
		MEfill(SCS_DESC_SIZE-len, 0, 
			scb->scb_sscb.sscb_ics.ics_description+len);
	    }
	}

        if (ps_ccb->psq_ret_flag & PSQ_SET_XA_XID)
	{
	    i4  len;
	    DB_DIS_TRAN_ID temp_db_distid;

	    /*
	    ** Set session distributed transaction id
	    */
	    len = STlength(ps_ccb->psq_distranid);
	    if(len >= DB_XA_XIDDATASIZE)
		len = DB_XA_XIDDATASIZE;
	    temp_db_distid.db_dis_tran_id_type = DB_XA_DIS_TRAN_ID_TYPE;
	    status = conv_to_struct_xa_xid(ps_ccb->psq_distranid,
		STlength(ps_ccb->psq_distranid), 
		&temp_db_distid.db_dis_tran_id.db_xa_extd_dis_tran_id );
	    if (DB_FAILURE_MACRO( status ))
	    {
		status = E_DB_ERROR;
		sc0e_0_put(E_SC033C_SET_SESSION_ERR, 0);
		break;
	    }

	    if (!(DB_DIS_TRAN_ID_EQL_MACRO(temp_db_distid,
			scb->scb_sscb.sscb_dis_tran_id)))
	    {
		STRUCT_ASSIGN_MACRO( temp_db_distid,
			scb->scb_sscb.sscb_dis_tran_id);
	    }
	    else
	    {
		/*
		** Need QEF_SP_SAVEPOINT every time we re-join a transaction
		** For example, application server (AS) could have done:
		** AS1 - xa_start, xa_end
		** AS2 - xa_start(TM_JOIN), xa_end
		** AS1 - xa_start(TM_JOIN), xa_end
		*/

		qe_ccb = scb->scb_sscb.sscb_qeccb;
		qe_ccb->qef_db_id = scb->scb_sscb.sscb_ics.ics_opendb_id;
		qe_ccb->qef_flag = 0;
		qe_ccb->qef_modifier = QEF_MSTRAN;
		qe_ccb->qef_eflag = QEF_EXTERNAL;
		qe_ccb->qef_cb = scb->scb_sscb.sscb_qescb;
		qe_ccb->qef_sess_id = scb->cs_scb.cs_self;
		qe_ccb->qef_no_xact_stmt =
		    (scb->scb_sscb.sscb_req_flags & SCS_NO_XACT_STMT) != 0;
		qe_ccb->qef_illegal_xact_stmt = FALSE;
		qe_ccb->qef_spoint = &Qef_s_cb->qef_sp_savepoint;
		status = qef_call(QET_SAVEPOINT, qe_ccb);
		if (DB_FAILURE_MACRO( status ))
		{
		    status = E_DB_ERROR;
		    sc0e_0_put(E_SC033C_SET_SESSION_ERR, 0);
		    break;
		}
	    }
	}

	if ((ps_ccb->psq_ret_flag & PSQ_SET_ACCESSMODE) &&
	    !(scb->scb_sscb.sscb_flags & SCS_STAR))
	{
	    dm_ccb = scb->scb_sscb.sscb_dmccb;
	    dm_ccb->dmc_op_type = DMC_SESSION_OP;   
	    dm_ccb->dmc_flags_mask = DMC_SET_SESS_AM;
	    dm_ccb->dmc_db_access_mode = ps_ccb->psq_accessmode;

	    status = dmf_call(DMC_ALTER, dm_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		status = E_DB_ERROR;
		sc0e_0_put(E_SC033C_SET_SESSION_ERR, 0);
		break;
	    }
	}

	/* Check if setting isolation level */
        if ((ps_ccb->psq_ret_flag & PSQ_SET_ISOLATION) &&
	    !(scb->scb_sscb.sscb_flags & SCS_STAR))
	{
	    /*
	    ** Set session isolation.
	    */
	    dm_ccb = scb->scb_sscb.sscb_dmccb;
	    dm_ccb->dmc_op_type = DMC_SESSION_OP;   
	    dm_ccb->dmc_flags_mask = DMC_SET_SESS_ISO;
	    dm_ccb->dmc_isolation = ps_ccb->psq_isolation;

	    status = dmf_call(DMC_ALTER, dm_ccb);
	    if (DB_FAILURE_MACRO(status))
	    {
		status = E_DB_ERROR;
		sc0e_0_put(E_SC033C_SET_SESSION_ERR, 0);
		break;
	    }
	}

	/* Check if setting on_logfull option */
	if ( ps_ccb->psq_ret_flag & (PSQ_SET_ON_LOGFULL_ABORT |
				     PSQ_SET_ON_LOGFULL_COMMIT |
				     PSQ_SET_ON_LOGFULL_NOTIFY) &&
	    !(scb->scb_sscb.sscb_flags & SCS_STAR) )
	{
	    /* Set session with on_logfull = abort | commit | notify */
	    DMC_CHAR_ENTRY	dmc_char;

	    dm_ccb = scb->scb_sscb.sscb_dmccb;
	    dm_ccb->dmc_op_type = DMC_SESSION_OP;   
	    dm_ccb->dmc_flags_mask = 0;
	    dm_ccb->dmc_char_array.data_in_size = sizeof(DMC_CHAR_ENTRY);
	    dm_ccb->dmc_char_array.data_address = (char*)&dmc_char;

	    dmc_char.char_id = DMC_C_ON_LOGFULL;

	    if ( ps_ccb->psq_ret_flag & PSQ_SET_ON_LOGFULL_ABORT )
		dmc_char.char_value = DMC_C_LOGFULL_ABORT;
	    else if ( ps_ccb->psq_ret_flag & PSQ_SET_ON_LOGFULL_COMMIT )
		dmc_char.char_value = DMC_C_LOGFULL_COMMIT;
	    else
		dmc_char.char_value = DMC_C_LOGFULL_NOTIFY;

	    status = dmf_call(DMC_ALTER, dm_ccb);

	    /* Clear this for subsequent uses */
	    dm_ccb->dmc_char_array.data_address = NULL;

	    if (DB_FAILURE_MACRO(status))
	    {
		status = E_DB_ERROR;
		sc0e_0_put(E_SC033C_SET_SESSION_ERR, 0);
		break;
	    }
	}

	break;
    }
    if(qsf_object)
    {
	DB_STATUS 	sts;

	/*
	** Now free the qe_ccb control block which is in qsf.
	*/
	sts = qsf_call(QSO_DESTROY, qs_ccb);    /* qs_ccb is set up above */
	if (DB_FAILURE_MACRO(sts))
	{
		scs_qsf_error(sts, qs_ccb->qsf_error.err_code,
		    E_SC0210_SCS_SQNCR_ERROR);
		status = sts;
	}
    }
    return status;
}

/*
** Name: scs_set_priv
**
** Description:
**	This routine implements the SET SESSION PRIVILEGES operation.
**	It checks the requested privileges against the limiting set, and
**	if allowed updated the session privileges to the new values.
**
** Inputs:
**	scb
**
** Outputs:
**	none
**
** History:
**	30-jul-93 (robf)
**	   Initial Creation
**	22-feb-94 (robf)
**         Modularized out propagation of privileges to other  facilities
**	   so it can be  called from scs_setrole.
*/
static DB_STATUS
scs_set_priv(
    SCD_SCB  *scb,
    PSQ_CB   *ps_ccb
)
{
    DB_STATUS   status=E_DB_OK;
    i4	privs;
    i4  	flags_mask= ps_ccb->psq_ret_flag;
    i4  	iprivs= ps_ccb->psq_privs;
    SXF_ACCESS  access;
    bool	loop=FALSE;
    DB_ERROR	dberror;

    do
    {
	if(flags_mask & PSQ_SET_DEFPRIV)
	{
		/* Default privileges */
		privs=scb->scb_sscb.sscb_ics.ics_defustat;
	}
	else if(flags_mask & PSQ_SET_ALLPRIV)
	{
		/* All privileges */
		privs=(scb->scb_sscb.sscb_ics.ics_maxustat&DU_UALL_PRIVS);
	}
	else if(flags_mask & PSQ_SET_NOPRIV)
	{
		/* No privileges */
		privs=0;
	}
	else if(flags_mask & PSQ_SET_APRIV)
	{
		/* Add privileges */
		privs=(scb->scb_sscb.sscb_ics.ics_rustat|iprivs);
	}
	else if(flags_mask & PSQ_SET_DPRIV)
	{
		/* Drop privileges */
		privs=(scb->scb_sscb.sscb_ics.ics_rustat& ~iprivs);
	}
	else if (flags_mask & PSQ_SET_PRIV)
	{
		/* Use privileges */
		privs=iprivs;
	}
	else
	{
		/*
		** Nothing to do
		*/
		break;
	}
	/*
	** Make sure value includes any non-priv bits from the  maximum set,
	** typically audit values currently
	*/
	privs|=(scb->scb_sscb.sscb_ics.ics_maxustat& ~DU_UALL_PRIVS);
	/*
	** Now we have the privileges, check that they are included 
	** in the maximum set.
	*/
	if((privs& scb->scb_sscb.sscb_ics.ics_maxustat) != privs)
	{
		/*
		** Not all privileges are allowed
		*/
	        sc0e_uput(E_US2471_9329_INVLD_SET_PRIV, 0, 0,
	         0, (PTR)0,
	         0, (PTR)0,
	         0, (PTR)0,
	         0, (PTR)0,
	         0, (PTR)0,
	         0, (PTR)0);
		status=E_DB_ERROR;
		break;
	}
	/*
	** Propagate privilege changes
	*/
	status=scs_propagate_privs(scb, privs);

    } while(loop);
    /*
    ** Security audit changing session privileges
    */
    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
    {
	if(status==E_DB_OK)
	    access=SXF_A_SUCCESS;
	else
	    access=SXF_A_FAIL;

	access|=SXF_A_SESSION;	/* User session event */

	if (DB_FAILURE_MACRO(sc0a_write_audit(
	    scb,
	    SXF_E_USER,	/* Type */
	    access, 
	    "SESSION PRIVILEGES",
	    sizeof("SESSION PRIVILEGES")-1,
	    NULL,	/* Object Owner */
	    I_SX2726_SET_SESS_PRIV,	/* Mesg ID */
	    TRUE,			/* Force record */
	    privs,
	    &dberror		/* Error location */
	)))
	{
	    /*
	    **	sc0a_write_audit should already have logged
	    **  any lower level errors
	    */
	    sc0e_0_put(E_SC023C_SXF_ERROR, 0);
	}
    }
    return status;
}

/*{
** Name: scs_set_priority
**
** Description:
**      This routine tries to set the current session priority to the
**	indicated value. 
**
** Inputs:
**      scb		- SCB, session control block.
**
**	req_priority	- Requested priority, based from 'normal' value
**
** Outputs:
**      none
**
**	Returns:
**	    E_DB_OK	- Operation suceeded
**	    E_DB_ERROR  - Operation failed
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-oct-93 (robf)
**	    Created for Secure 2.0
*/
DB_STATUS
scs_set_priority(
	SCD_SCB         *scb,
	i4		req_priority
)
{
    DB_STATUS status=E_DB_OK;
    i4	      priority;
    SXF_ACCESS  sxfaccess;
    STATUS      clstat;
    DB_ERROR	dberror;
    bool	loop=FALSE;

    do
    {
	/*
	** Check if allowed to set priority
	*/
	if((req_priority> scb->scb_sscb.sscb_ics.ics_priority_limit) ||
	  ((DBPR_PRIORITY_LIMIT & scb->scb_sscb.sscb_ics.ics_ctl_dbpriv) &&
	  !(DBPR_PRIORITY_LIMIT & scb->scb_sscb.sscb_ics.ics_fl_dbpriv)))
	{
		sc0e_uput(E_SC033F_INVLD_PRIORITY, 0, 1,
			 sizeof(req_priority), (PTR)&req_priority,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
		status=E_DB_ERROR;
		break;
		
	}
	/*
	** Calculate requested absolute priority
	*/
	priority = Sc_main_cb->sc_norm_priority + req_priority; 
	/* 
	** Check within available limits
	*/
	if (priority > Sc_main_cb->sc_max_usr_priority)
		priority = Sc_main_cb->sc_max_usr_priority;

	if (priority < Sc_main_cb->sc_min_usr_priority)
		priority = Sc_main_cb->sc_min_usr_priority;

	/*
	** Call CS to do the work
	*/
	clstat=CSaltr_session((CS_SID)0, CS_AS_PRIORITY, (PTR)&priority);

	if(clstat!=OK)
	{
		/*
		** Unable to set priority
		*/
		sc0e_uput(E_SC033E_SET_SESSION_PRIORITY, 0, 2,
			 sizeof(req_priority), (PTR)&req_priority,
			 sizeof(priority), (PTR)&priority,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
		status=E_DB_ERROR;
		break;
	}
	/*
	** Update current session priority to new value.
	** Current priority is based from normal.
	*/
	scb->scb_sscb.sscb_ics.ics_cur_priority  = 
			priority-Sc_main_cb->sc_norm_priority;
   } while(loop);

   /*
   ** Security audit the change
   */

   if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
   {
       if(status==E_DB_OK)
		sxfaccess=SXF_A_SUCCESS;
       else
		sxfaccess=SXF_A_FAIL;

       sxfaccess|=SXF_A_SESSION;	/* User session event */

	if (DB_FAILURE_MACRO(sc0a_write_audit(
	    scb,
	    SXF_E_USER,	/* Type */
	    sxfaccess,  /* Action */
	    "SESSION PRIORITY", /* Object description*/
	    sizeof("SESSION PRIORITY")-1, 
	    NULL,			/* Object Owner */
	    I_SX273E_SET_SESS_PRIORITY,	/* Mesg ID */
	    FALSE,			/* Force record */
	    0,
	    &dberror		/* Error location */
	)))
	{
	    /*
	    **  sc0a_write_audit should already have logged
	    **  any lower level errors
	    */
	    sc0e_0_put(E_SC023C_SXF_ERROR, 0);
	}
    }
    return status;
}

/*
** Name: scs_mxsession
**
** Description:
**	Handle the SET MAX.. session-level commands
**
** Inputs:
**	scb	- SCB
**	psy_cb  - Info from PSF
**
** Outputs:
**	psy_cb->psy_error.err_code - Error.
**
** History:
**	27-oct-93 (robf)
**	   Created
*/
static DB_STATUS
scs_mxsession(SCD_SCB *scb, PSY_CB *psy_cb)
{
   DB_STATUS status=E_DB_OK;
   char	     objname[128];
   SXF_ACCESS  sxfaccess;
   DB_ERROR	dberror;
   bool		loop=FALSE;

   objname[0]=0;

   do
   {
	/*
	** Check operation, currently idle or connect limits
	*/
	if(psy_cb->psy_ctl_dbpriv & DBPR_IDLE_LIMIT)
	{
	    STcopy("IDLE_LIMIT",objname);
	    if(!(psy_cb->psy_fl_dbpriv & DBPR_IDLE_LIMIT))
	    {
		/* restore default */
        	scb->scb_sscb.sscb_ics.ics_cur_idle_limit = 
			scb->scb_sscb.sscb_ics.ics_idle_limit;
	    }
	    else
	    {
		/* check new value */
		if (psy_cb->psy_idle_limit > scb->scb_sscb.sscb_ics.ics_idle_limit &&
			scb->scb_sscb.sscb_ics.ics_idle_limit!= -1)
		{
		    /* Idle limit invalid */
		    sc0e_uput(E_SC0342_IDLE_LIMIT_INVALID, 0, 1,
			 sizeof(psy_cb->psy_idle_limit),
			 (PTR)&psy_cb->psy_idle_limit,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
		    psy_cb->psy_error.err_code=E_PS0001_USER_ERROR;
		    status=E_DB_ERROR;
		    break;
		}
		else
		{
        	    scb->scb_sscb.sscb_ics.ics_cur_idle_limit = 
				psy_cb->psy_idle_limit;
		    /* set new value */
		}
	    }
	}

	if(psy_cb->psy_ctl_dbpriv & DBPR_CONNECT_LIMIT)
	{
	    STcopy("CONNECT_LIMIT", objname);
	    if(!(psy_cb->psy_fl_dbpriv & DBPR_CONNECT_LIMIT))
	    {
		/* restore default */
        	scb->scb_sscb.sscb_ics.ics_cur_connect_limit = 
			scb->scb_sscb.sscb_ics.ics_connect_limit;
	    }
	    else
	    {
		/* check new value */
		if (psy_cb->psy_connect_limit > scb->scb_sscb.sscb_ics.ics_connect_limit &&
			scb->scb_sscb.sscb_ics.ics_connect_limit!= -1)
		{
		    /* Connect limit invalid */
		    sc0e_uput(E_SC0343_CONNECT_LIMIT_INVALID, 0, 1,
			 sizeof(psy_cb->psy_connect_limit),
			 (PTR)&psy_cb->psy_connect_limit,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
		    psy_cb->psy_error.err_code=E_PS0001_USER_ERROR;
		    status=E_DB_ERROR;
		    break;
		}
		else
		{
        	    scb->scb_sscb.sscb_ics.ics_cur_connect_limit = 
				psy_cb->psy_connect_limit;
		    /* set new value */
		}
	    }
	}
   } while(loop);
   /*
   ** Security audit changing session resource limits
   */

   if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
   {
       if (status == E_DB_OK)
	    sxfaccess = SXF_A_SUCCESS;
       else
	    sxfaccess = SXF_A_FAIL;

       sxfaccess |= SXF_A_SESSION;	/* Session event */

	if (DB_FAILURE_MACRO(sc0a_write_audit(
	    scb,
	    SXF_E_RESOURCE,	/* Type */
	    sxfaccess,  /* Action */
	    objname,
	    STlength(objname),
	    NULL,			/* Object Owner */
	    I_SX273F_SET_SESSION_LIMIT,	/* Mesg ID */
	    FALSE,			/* Force record */
	    0,
	    &dberror		/* Error location */
	)))
	{
	    /*
	    **  sc0a_write_audit should already have logged
	    **  any lower level errors
	    */
	    sc0e_0_put(E_SC023C_SXF_ERROR, 0);
	}
   }
   return status;
}

/*
** Name: scs_setrole
**
** Description:
**	This routine implements the SET ROLE operation.
**
** Inputs:
**	scb		SCb
**
**	ps_ccb		PSQ_CB describing the statement
**
** Outputs:
**	none
**
** History:
**	22-feb-94 (robf)
**	   Initial Creation
**	11-mar-94 (robf)
**         Distinguish between role check failure and  access not allowed,
**	   issue different message to the session.
**	18-mar-94 (robf)
**         Update dbprivs when setting role.
**	21-mar-94 (robf)
**	   Propagate UPDATE_SYSCAT from initial dbprivs when changing to
**	   no role. 
**	30-mar-94 (robf)
**         Pass new role to other facilites which have a private copy
**	   (currently only PSF). 
**	14-apr-94 (robf)
**         Preserve any non-privilege flags in rustat for role.
*/
static DB_STATUS
scs_setrole(
    SCD_SCB             *scb,
    PSQ_CB		*ps_ccb
)
{
    DB_STATUS 	status=E_DB_OK;
    DB_STATUS   local_status;
    i4	roleprivs=0;
    i4	privs=0;
    SXF_ACCESS	sxfaccess;
    DB_ERROR	error;
    DB_PASSWORD *rolepw;
    SCS_DBPRIV  *roledbpriv;
    SCS_DBPRIV  dbpriv;
    bool	access;


    for(;;)
    {
	/* Check if (unsetting) role */
        if(ps_ccb->psq_ret_flag&PSQ_SET_NOROLE)
	{
	    /*
	    ** Check if removing the role would eliminate access to
	    ** the current database, if so we reject.
	    */
	    if(!(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_DBPR_NOLIMIT) &&
	        (scb->scb_sscb.sscb_ics.ics_idbpriv.ctl_dbpriv & DBPR_ACCESS)&&
	       !(scb->scb_sscb.sscb_ics.ics_idbpriv.fl_dbpriv & DBPR_ACCESS))
	    {
		sc0e_uput(E_SC035F_SET_ROLE_NODBACCESS, 0,1,
			 sizeof("none")-1,
			 (PTR)"none",
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
		status=E_DB_ERROR;
		break;
	       
	    }

	    /*
	    ** Setting no role, so blank out
	    */
	    MEfill( sizeof(scb->scb_sscb.sscb_ics.ics_eaplid),
		    ' ',
		   (PTR)&scb->scb_sscb.sscb_ics.ics_eaplid);
	    /*
	    ** Restore old privilege mask.
	    */
	    scb->scb_sscb.sscb_ics.ics_maxustat=
		     scb->scb_sscb.sscb_ics.ics_iustat;

	    /*
	    ** Handle restoration of UPDATE_SYSCAT from initial
	    ** dbprivileges.
	    */
	    if((scb->scb_sscb.sscb_ics.ics_idbpriv.fl_dbpriv 
			& DBPR_UPDATE_SYSCAT))
	    {
		scb->scb_sscb.sscb_ics.ics_maxustat |= DU_UUPSYSCAT;
		scb->scb_sscb.sscb_ics.ics_rustat |= DU_UUPSYSCAT;
	    }
	    else
	    {
		scb->scb_sscb.sscb_ics.ics_maxustat &= ~DU_UUPSYSCAT;
		scb->scb_sscb.sscb_ics.ics_rustat &= ~DU_UUPSYSCAT;
	    }
			
	    privs= scb->scb_sscb.sscb_ics.ics_rustat &
			   scb->scb_sscb.sscb_ics.ics_maxustat;
	    /*
	    ** Make sure value includes any non-priv bits from the  maximum set,
	    ** typically audit values currently
	    */
	    privs|=(scb->scb_sscb.sscb_ics.ics_maxustat& ~DU_UALL_PRIVS);

	    /*
	    ** This propagates the potential lowering of
	    ** privileges to all facilities.
	    */
	    status=scs_propagate_privs(scb, privs);
	    
	    if(status!=E_DB_OK)
		break;
	
	    /* 
	    ** Restore initial dbprivileges
	    */
	    status=scs_put_sess_dbpriv(scb, 
			&scb->scb_sscb.sscb_ics.ics_idbpriv);
	}
	else
	{
	    /*
	    ** Setting role to a specific value
	    */
	    if ( ps_ccb->psq_ret_flag&PSQ_SET_ROLE_PASSWORD)
	    {
		rolepw= &ps_ccb->psq_password;
	    }
	    else
		rolepw=NULL;
	    /*
	    ** Check we are allowed to access the role
	    */
	    status=scs_check_session_role(scb, 
			&ps_ccb->psq_rolename,
			rolepw,
			&roleprivs,
			&roledbpriv);
	    if (status==E_DB_WARN)
	    {
		/* 
		** Not allowed to use this role
		*/
		sc0e_0_uput(E_SC034D_ROLE_NOT_AUTHORIZED, 0);
		status=E_DB_ERROR;
		break;
	    }
	    else if (status!=E_DB_OK)
	    {
		/*
		** Some other failure checking the  role
		*/
		sc0e_0_uput(E_SC035B_ROLE_CHECK_ERROR, 0);
		sc0e_0_put(E_SC035B_ROLE_CHECK_ERROR, 0);
		break;
	    }
	    /*
	    ** OK to use this role
	    */
	    /*
	    ** Check if changing the role would eliminate access to
	    ** the current database, if so we reject. We have access if
	    */
	    /* Default TRUE */
	    access=TRUE;

	    /* If rejected by User/Group/Public */
	    if((scb->scb_sscb.sscb_ics.ics_idbpriv.ctl_dbpriv & DBPR_ACCESS)&&
	       !(scb->scb_sscb.sscb_ics.ics_idbpriv.fl_dbpriv & DBPR_ACCESS))
		access=FALSE;

	    /* If rejected/allowed by new Role */
            if(roledbpriv)
	    {
	        if (roledbpriv->ctl_dbpriv & DBPR_ACCESS)
		{
	           if(roledbpriv->fl_dbpriv & DBPR_ACCESS)
			access=TRUE;
		   else
			access=FALSE;
		}
	    }

	    /* If no limits then we always have access */
	    if(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_DBPR_NOLIMIT) 
		 access=TRUE;

	    if(!access)
	    {
		sc0e_uput(E_SC035F_SET_ROLE_NODBACCESS, 0,1,
		          cus_trmwhite((u_i4)sizeof(DB_OWN_NAME),
				   (PTR) &ps_ccb->psq_rolename),
		          (PTR) &ps_ccb->psq_rolename,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0,
			  0, (PTR)0);
		status=E_DB_ERROR;
		break;
	       
	    }
	    MEcopy((PTR)&ps_ccb->psq_rolename,
		   sizeof(scb->scb_sscb.sscb_ics.ics_eaplid),
		   (PTR)&scb->scb_sscb.sscb_ics.ics_eaplid);

	    /*
	    ** Remove any privs added by old role, then add any new 
	    ** privs to the maximum allowed set.
	    */
	    scb->scb_sscb.sscb_ics.ics_maxustat=
		     scb->scb_sscb.sscb_ics.ics_iustat;
	    scb->scb_sscb.sscb_ics.ics_maxustat|=roleprivs;


	    /*
	    ** This propagates the potential lowering of
	    ** privileges to all facilities. 
	    */
	    privs= scb->scb_sscb.sscb_ics.ics_rustat &
		    scb->scb_sscb.sscb_ics.ics_maxustat;
	    /*
	    ** Make sure value includes any non-priv bits from the  maximum set,
	    ** typically audit values currently
	    */
	    privs|=(scb->scb_sscb.sscb_ics.ics_maxustat& ~DU_UALL_PRIVS);

	    status=scs_propagate_privs(scb, privs);

	    if(status!=E_DB_OK)
		break;

	    /*
	    ** Propagate any dbprivs. We have to restore to the initial
	    ** set then add any new dbprivileges in.
	    */

	    if( roledbpriv)
	    {
		/* Role takes priority over others, so merge that way */
		STRUCT_ASSIGN_MACRO(*roledbpriv, dbpriv);

		/* Merge in initial values */
		scs_merge_dbpriv(scb, &scb->scb_sscb.sscb_ics.ics_idbpriv,
				&dbpriv);
		
	    }
	    else
	    {
		/* Use initial values */
		STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_idbpriv, dbpriv);
	    }
	    /* Update session values */
	    status=scs_put_sess_dbpriv(scb, &dbpriv);

	}
        break; 
    }
    /*
    ** Tell frontend to flush ids
    */
    if (status==E_DB_OK)
    {
    	scb->scb_sscb.sscb_cquery.cur_result =
		GCA_NEW_EFF_USER_MASK | GCA_FLUSH_QRY_IDS_MASK;
    }

    /*
    ** Tell any related facilities of new role
    */
    if (status==E_DB_OK)
    {
	/* Update PSF */
	MEcopy((PTR)&scb->scb_sscb.sscb_ics.ics_eaplid, 
		sizeof(DB_OWN_NAME),
		(PTR)&ps_ccb->psq_aplid);
	ps_ccb->psq_alter_op =  PSQ_CHANGE_ROLE;
	status = psq_call(PSQ_ALTER, ps_ccb, scb->scb_sscb.sscb_psscb);
	ps_ccb->psq_alter_op =  0;
	if (DB_FAILURE_MACRO(status))
	{
		sc0e_0_put(E_SC0215_PSF_ERROR, 0);
		sc0e_0_put(ps_ccb->psq_error.err_code, 0);
	}
    }

    /*
    ** Security audit change
    */
    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
    {
	sxfaccess=SXF_A_SESSION;
	if(status==E_DB_OK)
	    sxfaccess|=SXF_A_SUCCESS;
	else
	    sxfaccess|=SXF_A_FAIL;

	local_status = sc0a_write_audit(
	    scb,
	    SXF_E_ROLE,	/* Type */
	    sxfaccess,
	    (char*)&ps_ccb->psq_rolename,
	    sizeof( ps_ccb->psq_rolename),
	    NULL,			/* Object Owner */
	    I_SX2746_SESSION_ROLE,	/* Mesg ID */
	    FALSE,			/* Force record */
	    0,
	    &error			/* Error location */
	    );	
	if (local_status != E_DB_OK)
	{
	    sc0e_0_put(error.err_code, 0);
	    status=E_DB_ERROR;
	}
    }
    return status;
}


/*
** Name: scs_realloc_outbuffer
**
** Description:
**	This routine reallocates a bigger session tuple buffer
**
** Inputs:
**	scb		SCB
**
** Outputs:
**	none
**
** History:
**      18-feb-2004 (stial01)
**	   Initial Creation
*/
static DB_STATUS
scs_realloc_outbuffer(
SCD_SCB		*scb)
{

    SCC_GCMSG		save_msg;
    SCC_GCMSG		*save_msg_ptr;
    SIZE_TYPE		size;
    DB_STATUS		status;

    save_msg_ptr = scb->scb_cscb.cscb_outbuffer;
    STRUCT_ASSIGN_MACRO(*scb->scb_cscb.cscb_outbuffer, save_msg);

    status = sc0m_deallocate(0, (PTR *)&scb->scb_cscb.cscb_outbuffer);
    if (status)
	return (status);

    size = sizeof(SCC_GCMSG) + scb->scb_sscb.sscb_cquery.cur_tup_size;
		
    status = sc0m_allocate(0, size, SC_SCG_TYPE, (PTR) SCD_MEM, SCG_TAG,
	      (PTR *) &scb->scb_cscb.cscb_outbuffer);

    if (status != E_DB_OK)
    {
	/* FIX ME new error */
	return (status);
    }

#ifdef xDEBUG
    TRdisplay("scs_reallocate_outbuffer %x %d -> %x %d\n",
	save_msg_ptr, scb->scb_cscb.cscb_tuples_max, 
	scb->scb_cscb.cscb_outbuffer, size);
#endif

    /* Zero-fill the SCC_GCMSG, all but the sc0m header portion */
    MEfill(sizeof(SCC_GCMSG) - sizeof(SC0M_OBJECT),
	    0, (char *)scb->scb_cscb.cscb_outbuffer + sizeof(SC0M_OBJECT));

    scb->scb_cscb.cscb_outbuffer->scg_mask = SCG_NODEALLOC_MASK;
    scb->scb_cscb.cscb_outbuffer->scg_marea =
    scb->scb_cscb.cscb_tuples =
	    (PTR)scb->scb_cscb.cscb_outbuffer + sizeof(SCC_GCMSG);

    scb->scb_cscb.cscb_tuples_max = scb->scb_sscb.sscb_cquery.cur_tup_size;
    return (status);

}

/* Name: scs_emit_rows -- Prepare output message from row retrieval
**
** Description:
**	This routine takes the rows that have been retrieved into
**	the SCF tuple buffer and prepares them for output.
**
**	If the fetch involved blobs, QEF may have redirected the
**	QEF_DATA pointer to point to who knows where.  We need to
**	run through the row-buffers filled in this time around,
**	and make sure that all the data has been moved to the
**	normal SCF tuple buffer.
**	
**	After doing data movement if necessary, we compress the tuple
**	buffer if compression has been agreed upon; point the output
**	message at the buffer;  and link it into the list to be output.
**	
**
** Inputs:
**	qmode			Query mode PSQ_xxxx
**	rowproc			TRUE if row-producing procedure
**	scb			Sequencer control block for session
**	qe_ccb			QEF_RCB qef request block back from QEF
**		.qef_count	Number of QEF_DATA buffers filled
**		.qef_rowcount	Number of rows generated
**				The two can be different if blobs around.
**	message			SCC_GCMSG * pointer to output message list
**
** Outputs:
**	None
**
** History:
**	11-Aug-2005 (schka24)
**	    Split this code out of the main sequencer while trying
**	    to get row-producing procedures working in all cases.
**	25-Feb-2008 (kiria01) b119976
**	    Allow scs_compress() to return an indication of
**	    an error condition relating to potentially corrupt data.
**	01-may-2008 (smeke01) b118780
**	    Corrected name of sc_avgrows to sc_totrows.
*/

static DB_STATUS
scs_emit_rows(i4 qmode, bool rowproc, SCD_SCB *scb, QEF_RCB *qe_ccb, SCC_GCMSG *message)

{
    DB_STATUS status = E_DB_OK;
    char *row_buf;		/* Where data was put */
    i4 i;
    i4 total_dif;
    i4 tupbuf_used;
    QEF_DATA *data_buffer;	/* QEF row descriptors */
    SCS_CURRENT *cquery;	/* Current query info in scb */

    cquery = &scb->scb_sscb.sscb_cquery;

    if (cquery->cur_state < CUR_RUNNING)
    {
	/* If we haven't reached "running" yet, say that we have
	** emitted rows
	*/
	cquery->cur_state = CUR_RUNNING;
    }

    tupbuf_used = 0;
    if (qe_ccb->qef_count ||
	    (qe_ccb->error.err_code == E_AD0002_INCOMPLETE))
	    /* {@fix_me@} -- Second test may be redundant */
    {
	/* Keep count of total rows for RETRIEVE */
	if (qmode == PSQ_RETRIEVE || rowproc)
	{
	    Sc_main_cb->sc_totrows += qe_ccb->qef_rowcount;
	}

	/* Make sure rows are set up in the SCF buffer.  Normally
	** they are output directly to the cscb tuple buffer by QEF,
	** but if blobs are involved the data could be elsewhere.
	*/
	if ((cquery->cur_rdesc.rd_modifier & GCA_LO_MASK )
	    || (qe_ccb->error.err_code == E_AD0002_INCOMPLETE))
	{
	    /* Blobs.  Make sure that we move everything to the
	    ** message buffer if QEF left it somewhere else.
	    */
	    row_buf = (char *) scb->scb_cscb.cscb_tuples;
	    data_buffer = cquery->cur_qef_data;
	    i = 0;
	    while (--qe_ccb->qef_count >= 0)
	    {
		i += data_buffer->dt_size;

		if (row_buf != (char *) data_buffer->dt_data)
		{
		    /* Move row piece to SCF buffer */

		    MEcopy((PTR) data_buffer->dt_data,
				data_buffer->dt_size,
				(PTR) row_buf);
		}
		row_buf += data_buffer->dt_size;
		++data_buffer;
	    }
	    tupbuf_used += i;
	    if (qe_ccb->error.err_code == E_AD0002_INCOMPLETE)
	    {
		message->scg_mask |= SCG_NOT_EOD_MASK;
	    }
	}
	else
	{
	    /* Non-blobs, no movement, just do the arithmetic */
	    tupbuf_used = qe_ccb->qef_count * cquery->cur_tup_size;
	}
	qe_ccb->qef_count = 0;
    }
    if (tupbuf_used == 0)
	return status;			/* No rows produced at all */

    message->scg_msize = tupbuf_used;
    /*
    **  Compress the tuples, if they have any
    **  varchars.  (compress rows, not buffers!)
    */
    if (cquery->cur_rdesc.rd_modifier & GCA_COMP_MASK)
    {
	GCA_TD_DATA	*tdesc;
	tdesc = (GCA_TD_DATA *) cquery->cur_rdesc.rd_tdesc;
	if (tdesc)
	{
	    status = scs_compress(tdesc, scb->scb_cscb.cscb_tuples,
			    qe_ccb->qef_rowcount, &total_dif);
	    message->scg_msize -= total_dif;
	}
    }

    /* Add tuple message to list to be sent */ 
    message->scg_next = (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext;
    message->scg_prev = scb->scb_cscb.cscb_mnext.scg_prev;
    scb->scb_cscb.cscb_mnext.scg_prev->scg_next = message;
    scb->scb_cscb.cscb_mnext.scg_prev = message;

    return status;
} /* scs_emit_rows */

/* Name - scs_cleanup_output - Clean up output buffering data
**
** Description:
**	This routine is called to clean up the output buffering
**	data that the sequencer allocates for a query.
**
**	If it's the first time for a retrieval query, we look for and
**	remove any tuple-descriptor message.  Then, the current
**	qef-data descriptors are deallocated, as is the current
**	message (if it's deallocate-able).
**
** Inputs:
**	qmode			Current query mode
**				(can use qmode in cquery??)
**	scb			SCD_SCB session control block
**	message			current SCC_GCMSG pointer
**
** Outputs:
**	none
**
** History:
**	15-Aug-2005 (schka24)
**	    Extracted from sequencer mainline.
*/

static void
scs_cleanup_output(i4 qmode, SCD_SCB *scb, SCC_GCMSG *message)
{
    SCS_CURRENT *cquery;	/* Current query info in scb */

    cquery = &scb->scb_sscb.sscb_cquery;

    /* Apparently we don't need to deal with RETCURS, I guess cursor
    ** cleanup takes care of it.
    ** If error before sending any rows out, look for / prune TDESCR.
    */
    if ((qmode == PSQ_RETRIEVE || qmode == PSQ_PRINT || cquery->cur_rowproc)
	&& (cquery->cur_state < CUR_RUNNING))
    {
	SCC_GCMSG   *cmsg;

	for (cmsg = scb->scb_cscb.cscb_mnext.scg_next;
		cmsg && (cmsg != (SCC_GCMSG *) &scb->scb_cscb.cscb_mnext);
		cmsg = cmsg->scg_next)
	{
	    if (cmsg->scg_mtype == GCA_TDESCR)
	    {
		/* We found an error before returning rows. */
		/* Don't bother to send the descriptor */

		cmsg->scg_prev->scg_next = cmsg->scg_next;
		cmsg->scg_next->scg_prev = cmsg->scg_prev;
		if ( (cmsg->scg_mask & SCG_NODEALLOC_MASK) == 0 ||
		     (cmsg->scg_mask & SCG_QDONE_DEALLOC_MASK) != 0 )
		{
		    sc0m_deallocate(0, (PTR *)&cmsg);
		}   /* if */
		break;
	    }	/* if */
	}   /* for */
    }	/* if */

    if (cquery->cur_amem)
    {
	if (message && 
	    ((message->scg_mask & SCG_NODEALLOC_MASK) == 0))
	    sc0m_deallocate(0, (PTR *) &message);

	sc0m_deallocate(0, &cquery->cur_amem);
	cquery->cur_qef_data = NULL;
    }
} /* scs_cleanup_output */

/*
**  29-Sep-2004 (fanch01)
**      Add LK_LOCAL flag support as this lock is never seen beyond one node.
*/
static DB_STATUS
scs_xa_lock_scb(
SCD_SCB		*scb, 
SCD_SCB		*lock_scb,
LK_LKID		*lkid,
i4		lock_action)
{
    DB_STATUS		status;
    DB_STATUS		lk_status;
    CL_SYS_ERR		sys_err;
    i4			local_error;
    DMC_CB		*dm_ccb;
    LK_LOCK_KEY		lockkey;
    LK_LLID		lock_list_id;

    dm_ccb = scb->scb_sscb.sscb_dmccb;
    dm_ccb->dmc_op_type = DMC_SESSION_OP;   
    dm_ccb->dmc_flags_mask = DMC_SESS_LLID;
    status = dmf_call(DMC_SHOW, dm_ccb);
    if (status == E_DB_OK)
    {
	lock_list_id = *(LK_LLID *)dm_ccb->dmc_scb_lkid;
	lockkey.lk_type = LK_XA_CONTROL;
	/* Init entire key, because lock_scb may be 4 or 8 byte pointer */
	MEfill(LK_KEY_LENGTH, '\0', (PTR)&lockkey.lk_key1);
	lockkey.lk_key1 = scb->cs_scb.cs_pid;
	MEcopy(&lock_scb, sizeof(lock_scb), (PTR)&lockkey.lk_key2);

	lk_status = LKrequest(lock_action | LK_PHYSICAL | LK_LOCAL, lock_list_id,
		&lockkey, LK_X, (LK_VALUE * )0, lkid, 0L, &sys_err);
	if (lk_status != OK)
	{
	    ule_format(lk_status, &sys_err, ULE_LOG, NULL, 0, 0, 0,
		&local_error, 0);
	    /* err_code = E_SC0390_BAD_LOCK_REQUEST; */
	    status = E_DB_ERROR;
	}
    }
    return (status);
}
static DB_STATUS
scs_xa_unlock_scb(
SCD_SCB		*scb,
SCD_SCB		*lock_scb,
LK_LKID		*lkid)
{
    DB_STATUS		status;
    DB_STATUS		lk_status;
    CL_SYS_ERR		sys_err;
    i4			local_error;
    DMC_CB		*dm_ccb;
    LK_LLID		lock_list_id;

    dm_ccb = scb->scb_sscb.sscb_dmccb;
    dm_ccb->dmc_op_type = DMC_SESSION_OP;   
    dm_ccb->dmc_flags_mask = DMC_SESS_LLID;
    status = dmf_call(DMC_SHOW, dm_ccb);
    if (status == E_DB_OK)
    {
	lock_list_id = *(LK_LLID *)dm_ccb->dmc_scb_lkid;
	lk_status = LKrelease((i4)0, lock_list_id, lkid,
		(LK_LOCK_KEY *)0, (LK_VALUE *)0, &sys_err);
	if (lk_status != OK)
	{
	    ule_format(lk_status, &sys_err, ULE_LOG, NULL,
		(char *)0, 0L, (i4 *)0, &local_error, 0);
	    /* err_code = E_SC0391_BAD_LOCK_RELEASE; */
	    status = E_DB_ERROR;
	}

    }
    return (status);
}

/*
**   Name: scs_xatrace()
**
**   Description: 
**       Trace GCA_XA messages
**
**   History:
**       03-may-2007 (stial01)
**          Created from IICXformat_xa_xid in libqxa
**      12-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          Modify all XA xid fields to i4 (int for supplied files)
**          to ensure consistent use across platforms.
*/
static VOID
scs_xatrace(SCD_SCB *scb,
i4		     qmode,
i4		     qflags,
char		     *qstr,
DB_DIS_TRAN_ID	     *dis_tran_id,
i4    xa_stat)
{
  CS_SID		sid;
  char			xid_str[IIXA_XID_STRING_LENGTH];
  char     *cp = xid_str;
  i4       data_lword_count = 0;
  i4       data_byte_count  = 0;
  u_char   *tp;                  /* pointer to xid data */
  u_i4 unum;                /* unsigned i4 work field */
  i4       i;
  DB_XA_EXTD_DIS_TRAN_ID *extId;
  DB_XA_DIS_TRAN_ID      *xa_xid_p;
  char			 qmtemp[20];
  char			 *qm;
  i4			 db_dis_tran_id_type;

  if (dis_tran_id)
  {
     extId = &(dis_tran_id->db_dis_tran_id.db_xa_extd_dis_tran_id);
     xa_xid_p = &extId->db_xa_dis_tran_id;
     db_dis_tran_id_type = dis_tran_id->db_dis_tran_id_type;
  }
  else
  {
     /* use the xa tran id in the scb */
     extId = 
	&(scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.db_xa_extd_dis_tran_id);
     xa_xid_p = &extId->db_xa_dis_tran_id;
     db_dis_tran_id_type = scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id_type;
  }

 switch (qmode)
 {
    case PSQ_GCA_XA_START: qm = "START"; break;
    case PSQ_GCA_XA_END: qm = "END"; break;
    case PSQ_GCA_XA_PREPARE: qm = "PREPARE"; break;
    case PSQ_GCA_XA_COMMIT: qm = "COMMIT"; break;
    case PSQ_GCA_XA_ROLLBACK: qm = "ROLLBACK"; break;
    case PSQ_RETRIEVE: qm = "RETRIEVE"; break;
    case PSQ_APPEND: qm = "INSERT"; break;
    case PSQ_DELETE: qm = "DELETE"; break;
    case PSQ_REPLACE: qm = "UPDATE"; break;
    default: 
	qm = &qmtemp[0];
	STprintf(qm, "QM-%d", qmode);
	break;
 }

  CVlx(xa_xid_p->formatID,cp);
  STcat(cp, ERx(":"));
  cp += STlength(cp);
  CVla((i4)(xa_xid_p->gtrid_length),cp); /* length 1-64 from i4 or i8 long gtrid_length */
  STcat(cp, ERx(":"));
  cp += STlength(cp);
  CVla((i4)(xa_xid_p->bqual_length),cp); /* length 1-64 from i4 or i8 long bqual_length */
  cp += STlength(cp);
  data_byte_count = (i4)(xa_xid_p->gtrid_length + xa_xid_p->bqual_length);
  data_lword_count = (data_byte_count + sizeof(u_i4) - 1) / sizeof(u_i4);
  tp = (u_char*)(xa_xid_p->data);  /* tp -> B0B1B2B3 xid binary data */
  for (i = 0; i < data_lword_count; i++)
  {
     STcat(cp, ERx(":"));
     cp++;
     unum = (u_i4)(((i4)(tp[0])<<24) |   /* watch out for byte order */
                        ((i4)(tp[1])<<16) | 
                        ((i4)(tp[2])<<8)  | 
                         (i4)(tp[3]));
     CVlx(unum, cp);     /* build string "B0B1B2B3" in byte order for all platforms*/
     cp += STlength(cp);
     tp += sizeof(u_i4);
  }

  if  (db_dis_tran_id_type == DB_XA_DIS_TRAN_ID_TYPE )
      STcat(cp, ERx(":XA"));
  else if (db_dis_tran_id_type == DB_INGRES_DIS_TRAN_ID_TYPE)
  {
      /* DONT expect this */
      STcat(cp, ERx(":INGRES"));
  }
  else if (db_dis_tran_id_type == 0)
  {
      STcat(cp, ERx(":ZERO"));
  }
  else
  {
      STcat(cp, ERx(":OTHER"));
  }

  CSget_sid(&sid);

  if (!qstr)
    qstr = "";

  TRdisplay("Sid %x %8s %8s %8x %8x XID: %s \n", 
      sid, qm, qstr, qflags, xa_stat, xid_str);

} /* scs_format_xa_xid */
