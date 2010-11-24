/*
** Copyright (c) 1986, 2010 Ingres Corporation
*/

/*
NO_OPTIM = 
*/

/*
** LEVEL1_OPTIM = axp_osf
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cv.h>
#include    <ex.h>
#include    <lo.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <me.h>
#include    <er.h>
#include    <pm.h>
#include    <tr.h>
#include    <cs.h>
#include    <lk.h>
#include    <cx.h>
#include    <ci.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <usererror.h>

#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <opfcb.h>
#include    <ddb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <tm.h>
#include    <sxf.h>
#include    <gwf.h>
#include    <lg.h>

#include    <duf.h>
#include    <dudbms.h>
#include    <copy.h>
#include    <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <sc0a.h>

#include    <scserver.h>

#include    <uld.h>
#include    <cut.h>
#include    <cui.h>

/**
**
**  Name: SCSINIT.C - Initiation and Termination services for sessions
**
**  Description:
**      This file contains the functions necessary to initiate and 
**      terminate a session.  These include the initialization of the
**      session control block (sequencer part), the determination of 
**      necessary information from the "dbdb session," the logging,
**      if appropriate, of a session starting up or shutting down, 
**      the acquisition and disposition of the requisite resources, 
**      etc.
**
**          scs_initiate - Initialize a session
**          scs_terminate - Terminate a session
**          scs_dbdb_info - fetch dbdb information about the session
**	    scs_disassoc() - Lower level dissconnect
**
**
**  History:
**      19-Jun-1986 (fred)    
**          Created.
**      14-Jul-1987 (fred)
**          GCF support added.
**	15-Jun-1988 (rogerk)
**	    Added support for Fast Commit.  Don't initiate GCA if SCS_NOGCA
**	    session flag is set.  Only initiate facilities which are needed
**	    for this particular session.
**     12-sep-1988 (rogerk)
**	    Changed DMC_FORCE to DMC_MAKE_CONSISTENT.
**	15-Sep-1988 (rogerk)
**	    Added support for Write Behind threads.
**	10-Mar-1989 (ac)
**	    Added 2PC support.
**	15-mar-1989 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    scs_initiate() copies group id to SCS_ICS, PSQ_CB
**	    scs_initiate() copies application id & password to SCS_ICS, PSQ_CB
**	    scs_dbdb_info() verifies existence of application_id and group
**	    scs_dbdb_info() copies encrypted applid password to SCS_ICS
**	04-apr-1989 (paul)
**	    Initialize max depth for QEF.
**	    Initialize resource limits to non-existent.
**      24-apr-1989 (fred)
**          Add support for a server initialization thread.  This thread will
**	    perform work, in a normal thread context, necessary to get a server
**	    started up completely.  At this point, it will open/read the IIDBDB,
**	    acquire new datatype information, reinitialize ADF, and then go
**	    away.  Unlike the other special threads, it will not hang around too
**	    long.  However, scd_initiate will have claimed sc_urdy_semaphore
**	    exclusively, to prevent normal user threads from starting up.  At
**	    the completion of this thread, this semaphore will be released,
**	    allowing the normal user threads to acquire it in a shared mode, and
**	    thereby allowing them to get on with their work.
**	10-may-1989 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    scs_dbdb_info() reads iidbpriv to set session's database privileges;
**	    scs_dbdb_info() sets the session's default group-id if none given;
**	    changed interface to scu_xsvc.
**	12-may-1989 (anton)
**	    Local collation support
**	15-may-1989 (rogerk)
**	    Added checks for TRAN_QUOTA_EXCEEDED when opening DB.
**      22-may-1989 (fred)
**	    Altered GCA_IDX_STRUCT and GCA_RES_STRUCT portion of argument
**	    (GCA_MD_ASSOC) processing so that it moves the structure parameter
**	    into an on stack buffer before altering it.  Previously, the code
**	    was null-terminating the string in place.  This caused other code
**	    failure (E_US0022_FLAG_FORMAT errors) since it stomped a zero where
**	    one was not expected.  This has now been corrected.
**	22-may-1989 (neil)
**	    Initialize max rule depth for QEF from server startup parameter.
**	30-may-1989 (ralph)
**	    GRANT Enhancements, Phase 2c:
**	    Don't validate group-id, role-id, or role password for super users.
**	29-jun-1989 (ralph)
**	    CONVERSION:  scs_dbdb_info() checks dbcmptlvl, dbcmptminor
**	25-jul-1989 (sandyh)
**	    Added invalid dcb handler for bug 6533.
**	31-jul-1989 (sandyh)
**	    Added '$ingres' as valid user when accessing private databases.
**	    This is primarily for FE utilities that run in this mode.
**      20-jun-89 (jennifer)
**          Changed the key initialization for qeu_get and dmf calls to 
**          use the new key attribute defintions instead of constants.
**      29-JUN-89 (JENNIFER)
**          Added DMC_ALTER call to alter the dmf session according 
**          to status from user tuple.  This insures that all the 
**          new user statuses are set for a session.
**      05-jul-89 (jennifer)
**          Added code to scs_dbdb_info to initialize the auditing 
**          state from the iisecuritystate catalog in iidbdb.
**	03-aug-89 (kimman) 
**	    Changed MEcopy to appropriate MECOPY/I4ASSIGN macro.
**	    Changed CL_SYS_ERR to CL_ERR_DESC.  
**	    Use LO_DB instead of DB in LOingpath call.
**	20-aug-1989 (ralph)
**	    Set DMC_CVCFG for config file conversion
**	05-sep-89 (jrb)
**	    Added support for -k flag from front-end.
**	13-sep-89 (paul)
**	    Added code to remove alert registrations on termination
**	22-sep-1989 (ralph)
**	    If real user is dba, give session unlimited dbprivs, even if
**	    effective user is not dba.
**	22-sep-1989 (ralph)
**          Changed dbcmptlvl to "6.3" in dummy config file entry for iidbdb
**	27-sep-1989 (ralph)
**	    If real user is dba, allow any role or group identifier.
**	27-sep-1989 (ralph)
**	    B1 Enhancements:
**	    Remove installation defaults from dbpriv calculation.
**	    Add dbprivs granted on all databases to dbpriv calculation.
**	    Use iidbpriv to determine access privileges rather than iidbaccess.
**	27-oct-89 (ralph)
**	    Pass user name & status flags to QEF and PSF during session init.
**	29-oct-89 (paul)
**	    Add support for event handling thread
**	27-nov-1989 (ralph)
**	    If effective user is dba, don't enforce database privileges
**	09-Jan-1990 (anton)
**	    Integrate porting group alignment and performance changes
**		from ingresug 90020
**	11-jan-90 (andre)
**	    Copy sc_fips to psq_fips_mode before calling PSQ_BGN_SESSION.
**	16-jan-90 (ralph)
**	    Verify user password.
**	19-jan-1990  (nancy)
**	    Fix bug 9600 -- setting logical II_DECIMAL AV's here on startup.
**	30-jan-90 (ralph)
**	    Initialize user status bits to zero for qef (scs_initiate).
**	    Pass user status bits to QEF via qec_alter (scs_dbdb_info).
**      11-apr-90 (jennifer)
**          Bug 20481 - Add qeu_audit call when a session terminates.
**	08-aug-90 (ralph)
**	    Initialize server auditing state (scs_dbdb_info).
**	    Add support for DB_ADMIN database privilege.
**	    Fix UNIX portability problems
**	    Allow unlimited dbprivs if appl code specified.
**	    Bug 30809 - Force OPF enumeration if query limit in effect
**	    Alter DMF in scs_dbdb_info if user has AUDIT_ALL privs (bug 20659)
**	14-sep-90 (teresa)
**	    the following psq_cb booleans have become bitflags in psq_flag: 
**	    PSQ_FORCE, PSQ_CATUPD, PSQ_WARNINGS, PSQ_DBA_DROP_ALL, 
**	    PSQ_FIPS_MODE
**	10-oct-90 (ralph)
**	    Audit session termination.
**	    Ignore invalid iisecuritystate tuples when initializing audit state
**	    6.3->6.5 merge:
**	    06-sep-90 (jrb)
**		Added code to handle accepting time zone from front-end and
**		passing to ADF.  Fixes bug 32792.
**	    03-apr-1990 (fred)
**		Added CScomment() call when performing an abort on behalf of a
**		terminated thread.
**      20-oct-90 (jennifer)
**          Only disallow +Y and +U type flags for B1, let them through for C2.
**	11-dec-90 (ralph)
**	    Restrict iidbdb creation to 'ingres' in xORANGE environment only
**	15-jan-91 (ralph)
**	    Use DBPR_OPFENUM to determine if OPF enumeration should be forced.
**	05-feb-91 (ralph)
**          Initialize SCS_ICS.ics_sessid
**	    Fold database name to lower case (bug 21691)
**	04-feb-91 (neil)
**	    Alerters: Initialize event fields in scs_initiate.
**	    Dispose of outstanding event registrations in scs_terminate.
**	08-Jan-1991 (anton)
**	    Honor a GCA_RELEASE message as a first message
**	    Replaced several MEcopys into flag_value into I4ASSIGN_MACROs.
**      26-MAR-1991 (JENNIFER)
**          For dmf_alter session added send of DOWNGRADE privilege.
**	07-nov-1991 (ralph)
**	    6.4->6.5 merge:
**		24-jan-91 (seputis)
**		    initialized new mask field for QEU
**		25-jul-91 (andre)
**		    if the session is connected to a pre-6.4 FE (GCA protocol
**		    level < GCA_PROTOCOL_LEVEL_50), notify PSF that NLs must
**		    be stripped inside the string constants; this required
**		    to fix bug 38098
**		25-jul-1991 (ralph)
**		    Correct minor UNIX compiler warnings
**		18-aug-1991 (ralph)
**		    scs_init() - Ignore GCA_GW_PARM
**		22-oct-1991 (ralph)
**		    Fix bug b40514 -- scs_dbdb_info() verified that the
**		    EFFECTIVE user is a member of the specified group,
**		    rather than the REAL user.  This is only significant
**		    for non-super, non-DBA sessions that specify a group
**		    or role that has DB_ADMIN privileges, and also specify
**		    the "-u" flag.
**      16-apr-1992 (rog)
**          Turned optimization back on for Sun 4.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              24-Aug-1988 (alexh)
**                  Upgrade session initialization to distinguish STAR and non
**                  STAR session.
**              11-Jan-1991 (fpang)
**                  Fixed byte alignment problem. Massaged reading of
**                  GCA_REQUEST block.
**              14-Feb-1991 (fpang)
**                  There is a small window where an incoming association can 
**		    get an empty cdb descriptor. It happens when two incoming 
**		    associations to the same ddb are close to together. The 
**		    cdb descriptor is allocated but empty when the second 
**		    association comes in.  Scs_inititate thinks that it has 
**		    already been filled in and does not call qef to confirm 
**		    the descriptor. Usual symptoms are RQF failing associations
**		    with errors from the local "Database does not exist".  
**		    Related to bug 35094.
**              13-aug-1991 (fpang)
**                  Modified scs_initiate() and scs_terminate() to support
**                  new GCA_MD_ASSOC flags GCA_TIMEZONE, and GCA_GW_PARMS.
**          End of STAR comments.
**	25-jun-1992 (fpang)
**	    Included ddb.h and related include files for Sybil merge.
**	29-jul-1992 (fpang)
**	    In scs_initiate(), inititalize rdf_ddb_id for distributed.
**      21-aug-1992 (stevet)
**          Added new timezone support by setting up appropriate 
**          session timezone value using a series of TMtz routines.
**	25-sep-92 (markg)
**	    Added code to support the initiation and termination of 
**	    SXF sessions.
**      25-sep-1992 (nandak)
**          Copy the distributed transaction id into new structure.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**      6-oct-1992 (nandak)
**          Initialize the distributed transaction id type.
**	7-oct-1992 (bryanp)
**	    No user connections in the Recovery Server.
**	23-Oct-1992 (daveb)
**	    Make GWF a first class facility, and set it up here.  Name the
**	    semaphore.
**	03-nov-92 (robf)
**	    Startup SXF on MONITOR sessions, otherwise could get failures
**	    on audit attempts later.
**	    Audit usage of MONITOR sessions
**	    Make sure User Stats are set prior to first SXR_WRITE SXF call.
**     29-oct-1992 (stevet)
**          Changed decimal literal from FLOAT to DECIMAL.  Also check
**          GCA protocol level to see if decimal can be supported and
**          update adf_proto_level accordingly.
**      8-nov-1992 (ed)
**          extended constants for DB_MAXNAME project
**	02-nov-92 (jrb)
**	    Changed "du_sortloc" comment to "du_workloc" and filled in name
**	    of initial work location.  (This is not yet used as far as I know.)
**	29-oct-1992 (fpang)
**	    If distributed, don't initialize GWF, DMF or SXF.
**	16-nov-92 (robf)
**	    When building the "default" iidbdb info (during CREATEDB iidbdb)
**	    make sure the SCF info (ics_rustat) is set appropriately, including
**	    SECURITY priv. (Note CREATEDB will have already verified the
**	    user has SECURITY priv before attempting the connect to the
**	    server). Without this some privileged commands issued later in
**	    the session will fail.
**      09-aug-1992 (stevet)
**          Fixed a problem where STcopy() is called with wrong arguments.
**	19-nov-92 (fpang)
**	    In scs_initiate(), when waiting for 2PC thread to finish, 
**	    reinitialize status after each timeout. Eliminates spurious 
**	    'Session initiate failures'.
**	20-nov-92 (robf)
**	    Remove initialization of iisecurity_state catalog, this is now
**	    handled by SXF. Turn direct calls for sxf_call into wrappers for
**	    new sc0a routines.
**	20-nov-1992 (markg)
**	    Add support for the audit thread.
**	30-nov-92 (robf)
**	    Removed references to "SUPERUSER" priv. Replaced by appropriate
**	    combination of SECURITY, OPERATOR etc.
**	18-jan-1993 (bryanp)
**	    Added support for CPTIMER thread.
**      03-dec-1992 (nandak,mani)
**          Added new message type GCA_XA_RESTART for Ingres65 XA Support.
**      06-jan-1993 (mani)
**          Rewrote code dealing w/Ingres and XA dist XIDs. Added static rtn.
**	1-mar-193 (markg)
**	    Fixed bugs where errors returned by sc0a_write_audit were not
**	    getting handled correctly.
**      03-mar-1993 (stevet)
**          Reset status to E_DB_FATAL if we got an error during loading
**          timezone file.
**      10-mar-1993 (mikem)
**          bug #47624
**          Disable CSresume() calls from scc_gcomplete() while session is
**          running down in scs_terminate() by setting the
**          sccb_terminating field.  Allowing CSresume()'s during this time
**          leads to early wakeups from CSsuspend()'s called by scs_terminate
**          (like DI disk I/O operations).
**	15-mar-1993 (ralph)
**	    DELIM_IDENT: Copy DU_DATABASE.du_dbservice into SCS_ICS.ics_dbserv
**	    to establish session's case translation semantics.
**	    Accept delimited effective user, group and role identifiers.
**	    Translate authorization identifiers according to database 
**	    case translation semantics.
**	15-mar-1993 (ralph)
**	    DELIM_IDENT: Temporarily check for DBA as real user in scs_dbdb_info
**	    to circumvent an integration problem.
**	17-mar-1993 (ralph)
**	    Corrected compile errors....
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Call dmf, psf, opf, rdf and qef to pass info wrt delim idents.
**	    Initialize ics_cat_owner in scs_icsxlate.
**	    Handle variable case translation semantics for iidbdb.
**      26-apr-1993 (fpang)
**          Star. In scs_initiate(), if monitor session is initiated by
**          ingres, add QEF to the facilities needed, just in case the
**          iidbdb needs to be opened.
**          Fixes 50327 (IIMONITOR on STAR server as ingres gets SEGV).
**      06-may-93 (anitap)
**          Initialzed qef_cb->qef_upd_rowcnt = QEF_ROWQLFD in scs_initiate().
**      24-May-1993 (fred)
**          Added byte protocol to the list of masks supported by the
**          6.5 protocol.
**	05-may-1993 (ralph)
**	    DELIM_IDENT:
**	    Bug fixes, including:
**		Don't translate delimited effective user, group and role
**		identifiers until the case translation semantics for the
**		database is known.
**		Rework handling of role and group passwords to allow
**		slash to appear in a delimited role or group.
**      23-jun-1993(shailaja)
**          Fixed prototype incompatibilities.
**	29-Jun-1993 (daveb)
**	    Fix compiler warnings.
**	    - cast initial arguments to cui_idxlate to u_char *.
**	    - cast ics_username to PTR before stuffing into data_address.
**	    - Cast temptuple to PTR before stuffing in sc_dbdb_tuple.
**	2-Jul-1993 (daveb)
**	    prototyped, removed func externs in headers.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	    Correct an off-by-one error in distributed tran ID parsing.
**	27-jul-1993 (ralph)
**	    DELIM_IDENT:
**	    Set case translation mask and dbservice value when creating
**	    the iidbdb database.  These values are passed in via the
**	    -G flag, at least until we specify a more formal interface.
**	    Pass case translation mask and catalog owner name to GWF when
**	    starting up, and when the mask changes.
**	    Use DU_REAL_USER_MIXED to influence case translation of real usr.
**	08-aug-1993 (shailaja)
**	    Cast 2nd argument to rdf_call to match prototype.
**      12-aug-1993 (stevet)
**          Added support for GCA_STRSTUNC_OPT for string truncate detection.
**          Also changed default for math exception (MATHEX) from ignore to 
**          error if protocol level is GCA_PROTOCOL_LEVEL_60 or above. 
**	23-aug-1993 (andys)
**          Add SCU_MZERO_MASK to sc0m_allocate allocate call for the
**	    sscb_dm[sc]cb control blocks. This is so that the value of
**	    scf_scb_ptr will be NULL if it has not been initialised. This
**	    value is used in dmc_ui_handler when you remove a thread with
**	    iimonitor. [bug 45020] [was 28-aug-92 (andys)]
**	 6-sep-93 (swm)
**	    Cast second parameter of CSattn() to i4 to match
**	    revised CL interface specification.
**	    Changed CVlx() call to CVptrax() since a PTR is large enough
**	    to hold a CS_SID.
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	15-sep-1993 (tam)
**	    In scs_initiate, append '\0' to star_comflags.dd_co26_msign for 
**	    proper initialization of II_MONEY_FORMAT in star (bug #54762).
**       20-sep-1993 (stevet)
**          Return E_DB_ERROR, not E_DB_FATAL, when session connected with an
**          invalid timezone name.
**	20-Sep-1993 (daveb)
**	    Don't dissasoc if we know channel already was.  Cast PTRs
**	    before using them.
**	16-Sep-1993 (fpang)
**	    Added Star support for -G (GCA_GRPID) and -R (GCA_APLID).
**	11-Oct-1993 (fpang)
**	    Added Star support for -kf (GCA_DECFLOAT).
**	    Fixes B55510 (Star doesn't support -kf flag).
**	18-Oct-1993 (rog)
**	    Moved QSF session termination after PSF, OPF, and QEF termination.
**	    (bug 54965)
**	18-oct-93 (jrb)
**          In scs_dbdb_info, tell DMF when we open the iidbdb just for 
**          administrative purposes.
**      23-Sep-1993 (iyer)
**          Modified the conversion function conv_to_struct_xa_xid to
**          accept a pointer to DB_XA_EXTD_DIS_TRAN_ID as a parameter instead
**          of DB_XA_DIS_TRAN_ID. The function has been modified to
**          extract the branch_seqnum and branch_flag from xid_str. 
**	25-oct-93 (vijay)
**	    Correct a bunch of casts for session_id's.
**	01-nov-93 (anitap)
**	    if scb->scb_sscb.sscb_ics.ics_appl_code == DBA_PRETEND_VFDB, set
**	    PSQ_INGRES_PRIV in PSQ_CB.psq_cb before calling PSF to start a
**	    session. Changes in scs_initiate().
**	03-nov-93 (stephenb)
**	    Change if block when checking for the "-u" flag so we reject if
**	    user does NOT have SCS_USR_RDBA, not if they do.
**	09-nov-93 (rblumer)
**	    Fixed bug 55170, where DBAname != username even when user IS the
**	    DBA!!  (happens in FIPS database when have INGRES iidbdb).
**	    The main problem was that the dbaname (dbowner) was not getting
**	    translated according to REAL_USER_CASE semantics.
**	    Also changed ics_eusername to always point to ics_susername.
**	    
**	    But this only fixes the problem when the iidbdb and the target db
**	    have the same case-translation semantics OR when REAL_USER_CASE is
**	    NOT mixed in the target db; for the non-mixed iidbdb/mixed target
**	    db case, will eventually have to make other changes, to either the
**	    CONFIG file or iidatabase catalog.  More on this to come ....
**	23-nov-93 (stephenb)
**	    Set DMC_S_SECURITY for special applications, because they need to
**	    manipulate security catalogs and the user may not have security
**	    privilege.
**	01-Dec-1993 (fpang)
**	    In scs_initiate(), initialize dd_co36_aplid and dd_c037_grpid to 
**	    EOS. 
**	    Fixes B57478, spurious remote connect failiures.
**	17-dec-93 (rblumer)
**	    removed sc_fips field.
**	24-jan-1994 (gmanning)
**	    Change references from CS_SID to CS_SCB and CSget_scb.
**	09-oct-93 (swm)
**	    Bugs #56437 #56447
**	    Put session id. (cs_self) into new dmc_session_id instead of
**	    dmc_id.
**	    Eliminate compiler warnings by removing PTR casts of CS_SID
**	    values.
**	11-oct-93 (johnst)
**	    Bug #56449
**	    Changed TRdisplay format args from %x to %p where necessary to
**	    display pointer types. We need to distinguish this on, eg 64-bit
**	    platforms, where sizeof(PTR) > sizeof(int).
**      31-jan-1994 (mikem)
**          Reordered #include of <cs.h> to before <lk.h>.  <lk.h> now
**          references a CS_SID and needs <cs.h> to be be included before
**          it.
**      17-feb-1994 (rachael) Bug 55942,58205
**          Remove changes made in 15-mar-1993 integration
**	    which noted if the target database (to be opened) was the iidbdb,
**          and if so, didn't open it the 2nd time. This could be done because 
**          at session initialization the iidbdb is opened first, then
**          the target database is opened.  It was at this second opening
**          that flags such as -l (locking) or +U were used.
**          Since the iidbdb wasn't being opened the 2nd time, these flags
**          were being ignored if the target database was the iidbdb.
**          and if so, open it only once. This could be done because the 
**	    iidbdb is always opened once at session initialization, then the
**	    target database is opened.  It was at this second opening
**	    that flags such as -l (locking) or +U were used.  These flags
**	    were ignored if the target database was the iidbdb.
**	08-mar-94 (rblumer)
**	    B60387: in scs_dbdb_info,
**          copy disk-version of iidbdb dbtuple into scf database control
**          block, in order to always have correct case-translation semantics.
**	28-mar-94 (mikem)
**	   bug #59865
**	   Upgradedb of the iidbdb from 6.4 to 6.5 was failing in the dmf
**	   automatic upgrade of system catalog portion.  During upgradedb
**	   of iidbdb you must get an exclusive lock, even during the initial
**	   "qry the iidbdb" about startup information phase.  Fixed 
**	   scs_dbdb_info() to get an exclusive lock on the iidbdb if the
**	   server is in the process of upgrading the iidbdb.
**	23-feb-94 (rblumer)
**	    Changed ics_dbxlate initialization to set the bits for LOWER case 
**	    ids, even though that is the default.  Otherwise, code that looks
**	    at the dbxlate flags must always check for the non-normal cases
**       2-Mar-1994 (daveb) 58423
**          change the way monitor sessions are validated.  Never
**          check the user in the dbdb, because the logging system may
**          be down.  Instead, always check the PM SERVER_STARTUP
**          privilege for the user (or DB_NDINGRES_NAME).  This way
**          you  can monitor servers when the logging system is down and
**          you can't get to the dbdb.  Unfortunately, it means you
**          must have the PM priv, and the DB stored privs will never
**          be considered.  We use scd_chk_priv() to do the PM work.
**      23-Mar-1994 (daveb) 60410
**          check the return from scs_allocate_cbs and give up
**  	    initiating the session if it fails; otherwise SEGV city.
**	20-apr-94 (rblumer)
**	    Added comments to note future possibility of putting
**	    case-translation flags into database control block, instead of each
**	    facility's control block.
**      21-Apr-1994 (daveb)
**          rename scd_chk_priv to scs_chk_priv for VMS shared library
**  	    reasons.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**	24-may-94 (stephenb)
**	    change "=&.." to " = &.." (note the spacing) to get a clean
**	    compile on VMS.
**  	2-Oct-1994 (Bin Li)
**          Fix bug 60932, for command like 'ingmenu <dbname>, sql <dbname>
**          and etc'. If <dbname> is invalid, make the function call 
**          sc0e_put directly, meanwhile pass the database name as one argument.
**          The error message will write to the ERRLOG.LOG file.
**	19-nov-1994 (HANCH04)
**	    Fixed sc0e_put to read sc0e_0_put
**      29-nov-1994 (harpa06)
**          Integrated bug fix #62708 from INGRES 6.4 by angusm:
**          Remap QEOO5A as for QE0054 (failure to reconnect to
**          WILLING_COMMIT TX), bug 62708.i (scs_initiate)
**      20-dec-94  (chech02)
**          fixed AIX compiler error:
**           from: sxf_cb->sxf_sess_id = (PTR) scb->cs_scb.cs_self;
**           to  : sxf_cb->sxf_sess_id = scb->cs_scb.cs_self;
**	4-may-1995 (dougb) 68425
**	    Fix for bug #68425:  UPGRADEDB fails when converting a 6.4 iidbdb.
**	    Error is E_US0061, caused by a failed test in scs_usr_expired.
**	    Fix is to initialize the correct expdate field (change to robf's
**	    fix of 12-jul-94) in scs_dbdb_info.
**	04-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**      10-May-1995 (lewda02/shust01)
**          RAAT API Project:
**          Initialize new sscb_raat_message pointer to NULL.
**          Free allocated memory (if any) from pointer.
**	8-jun-1995 (lewda02/thaju02)
**	    RAAT API Project:
**	    Deallocation of linked buffer list.
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer (MCT)
**	04-jul-95 (emmag)
**	    Reinstated a CSp_semaphore and CSv_semaphore in scs_dbdb_info()
**	    that had disappeared in the work that was done for MCT.  This
**	    semaphore is required in the cases where MCT is not defined.
**      17-jul-1995 (canor01)
**          use local buffer for db location for MCT
**      19-jul-1995 (canor01)
**          pass scb->cs_scb.cs_self instead of scb to CSattn(), which is 
**	    expecting a CS_SID
**	24-jul-95 (emmag)
**	    Cleaned up casting for previous submission.
**      14-Sep-1995 (shust01/thaju02)
**          RAAT API Project:
**          Initialize new sscb_big_buffer pointer to NULL.
**          Initialize new sscb_raat_workarea pointer to NULL.
**          Free allocated memory (if any) from pointers.
**	14-nov-1995 (nick)
**	    Changes to support II_DATE_CENTURY_BOUNDARY / GCA_YEAR_CUTOFF.
**	21-feb-96 (thaju02)
**	    Added history for change#422304 after-the-fact.
**	    Enhanced XA Support.  Re-declared function conv_to_struct_xa_xid
**	    from static to global.
**      06-mar-1996 (nanpr01)
**          Changes to Major database version du_dbcmptlvl to 8.0 from 7.0.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      03-jun-1996 (canor01)
**          OS-threads version of LOingpath no longer needs protection by
**          sc_loi_semaphore.
**	6-jun-96 (stephenb)
**	    Add code for replicator queue management thread.
**	13-jun-96 (nick)
**	    LINT directed changes.
**	16-jul-96 (stephenb)
**	    Add efective username to scs_check_external_password() parameters.
**	30-jul-96 (stephenb)
**	    Add QEF to needed facilities for replicator queue management thread.
**	16-aug-96 (pchang)
**	   When obtaining database attributes (DU_DATABASE) from iidatabase 
**	   into sc_dbdb_dbtuple in scs_dbdb_info(), a pointer to a temporary 
**	   tuple is used in the QEF calls.  However, a failure to take into 
**	   account the generic object header SC_OBJECT, when allocating memory 
**	   for this temptuple caused the server to SEGV in sc0m_deallocate(),
**	   should the call to QEU_GET or QEU_CLOSE fail. (B78133)
**	24-Oct-1996 (cohmi01)
**	    Don't free sscb_raat_message until after scc_dispose runs through
**	    the pending message queue. Bug 78155.
**	23-sep-1996 (canor01)
**	   Move global data definittions to scsdata.c.
**	10-oct-1996 (canor01)
**	   Add SystemProductName to E_SC034F message.  Add SystemVarPrefix
**	   to E_SC037B.
**	27-Mar-1997 (wonst02)
**	    Use II_ACCESS for the type of access: read-write or readonly.
** 	16-may-1997 (wonst02)
** 	    Changes for readonly databases; remove II_ACCESS.
**	22-May-1997 (jenjo02)
**	    Moved call to scs_iprime() to early in scs_initiate() rather
**	    than at the end. Sessions stalled waiting on some resource
**	    can't be broken out of until the expedited GCA channel is
**	    set up by scs_iprime(). (bug 77674)
**	27-Jun-1997 (jenjo02)
**	    Removed sc_urdy_semaphore, which served no useful purpose.
**	11-nov-97 (mcgem01)
**	    Remove the restriction that only the ingres user can create 
**	    iidbdb on NT - this means that only UNIX now has this restriction.
**	23-dec-1997 (hanch04)
**	    Upgradedb failed from 6.4, need to assign structures
**	4-feb-1998(angusm)
**	    Cross-integrate fix for bug 78296 from oping12
**	    to scs_terminate().
**	09-Mar-1998 (jenjo02)
**	    Added support for Factotum threads.
**	    Embed session's MCB in SCD_DSCB instead of separately allocating
**	    it.
**	    The facilities cb and work area memory is now allocated in
**	    scd_alloc_scb as a integral piece of the total SCB memory,
**	    hence scs_alloc_cbs() and scs_dealloc_cbs() are deprecated.
**	    "need_facilites" is established in scd_alloc_cbs() and passed to
**	    scs_initiate() in sscb_need_facilities.
**	16-Apr-1998 (jenjo02)
**	    Unconditionally initialize sscb_asemaphore and alert structures
**	    for all SCB types.
**	02-Jun-1998 (jenjo02)
**	    Instead of initializing ics_dbname to "<no_database>", leave it
**	    as it came from scd_alloc_scb(), initialized to nulls. The
**	    verbage just clutters IPM and the monitor.
**	29-jul-1998 (shust01)
**	    Initialized adf_cb->adf_errcb.ad_emsglen to 0 in scs_initiate(). 
**	    Client had instance of calling ule_format() without msg being set
**	    (user hit cntrl-C) causing SEGV, since ad_emsglen has extremely
**	    large value (was uninitialized).
**	30-jul-1998 (i4jo01)
**	    Bug #79212: Do not allow user to change effective user (with '-u'
**	    flag) for DESTROYDB if the user does not have the security
**	    privilege.  Changed scs_dbdb_info to check for required privilege.
**	18-sep-1998 (thoda04)
**	    Use CVuahxl for integer conversion of XID data to avoid sign 
**	    overflow.
**	05-oct-1998 (thoda04)
**	    Process XID data bytes in groups of four, high to low bytes for 
**	    all platforms.
**	10-nov-1998 (sarjo01)
**	    Added DBPR_TIMEOUT_ABORT to default internal DB PRIV exclude
**	    mask.
**      26-jan-1999 (horda03) X-Integration of change 437979.
**        28-Sep-1998 (horda03)
**          Bug 90634. Whilst terminating a session which failed to complete
**          its initialisation, a E_SC012B_SESSION_TERMINATE error may be
**          generated because the status varible is undefined.
**	16-Nov-1998 (jenjo02)
**	    When suspending on BIO read, use CS_BIOR_MASK.
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**	04-feb-1999 (muhpa01)
**	    Fix to scs_initiate() to correct screening of internal threads
**	    for calls to sc0a_write_audit().  This corrects DBMS server startup
**	    problems resulting in SX103F errors in sxas_check().
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	10-mar-1999 (mcgem01)
**	    Add a server-side license check for OpenROAD runtime and
**	    development.
**	18-Jun-1999 (schte01)
**          Added cast to avoid unaligned access in axp_osf.   
**      08-Feb-2001 (horda03) Bug 103927
**          Initialize event queue variables introduced for bug 103596.
**	    Add a server-side license check for OpenROAD runtime.
**	23-mar-1999 (mcgem01)
**	    Implement the server-side license check for OpenROAD development.
**	17-may-1999 (hanch04)
**	    Change prototypes and castings to quite compiler warnings.
**	01-Jul-1999 (shero03)
**	    Support II_MONEY_FORMAT=NONE for SIR 92541.
**	08-jul-1999 (mcgem01)
**	    Reinstate comments which were inadvertently dropped in change
**	    441753.  Also added comments for the two most recent piccolo
**	    changes.
**	18-Jun-1999 (schte01)
**       Added cast to avoid unaligned access in axp_osf.   
**	01-oct-1999 (somsa01)
**	    Added the setting of the Ingres system privileges to
**	    scs_initiate().
**	16-apr-1999 (hanch04)
**	    Call CVuahxl8 for LP64
**	03-nov-2000 (somsa01)
**	    In scs_initiate(), added setting of AD_B1_PROTO when B1
**	    security is turned on.
**      17-Nov-2000 (hweho01)
**          Added LEVEL1_OPTIM (-O1) for axp_osf.
**          With option "-O2" ("-O"), the compiler generates incorrect
**          codes that cauase unaligned access error during run_all/
**          ministar test.  
**          Compiler version: Compaq C V6.1-019 -- V6.1-110.
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          In scs_initiate() pass PSQ_RULE_UPD_PREFETCH to psq_bgn_session
**          unless sc_rule_upd_prefetch_off is set to TRUE.
**	08-Jan-2001 (jenjo02)
**	    Pass pointers to GWF_CB, ADF_CB, to DMF.
**	    Remove cast of gwr_session_id to CS_SID.
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Supply session's SID to RDF in rdf_sess_id,
**	    pointer to session's ADF cb in rdf_adf_cb.
**	    Deleted redundant qef_dmf_id, use qef_sess_id instead.
**	    Pass pointer to session's ADF_CB to PSQ_BGN_SESSION.
**	19-Jan-2001 (jenjo02)
**	    Don't call SXF if not enabled for session.
**	    Stifle calls to sc0a_write_audit() if not C2SECURE.
**	    Deleted setting of AD_B1_PROTO, ADF now knows globally.
**	06-Feb-2001 (hweho01)
**          Put back the change which was dropped in change 446909.   
**	31-Jul-2001 (hweho01)
**          Started the preprocessor statement "#if def" for LP64     
**          at the first column.
**	15-feb-2001 (somsa01)
**	    Corrected compiler warnings.
**	11-may-2001 (devjo01)
**	    Minor changes for s103715.
**      15-mar-2002 (stial01
**          scs_check_app_code() fixed security loophole (b107344)
**	12-apr-2002 (devjo01)
**	    Correct size of 'node' buffer.
**      29-apr-2002 (stial01)
**          scs_check_app_code() case DBA_SYSMOD must allow connect to iidbdb
**          and validate connection to all other databases. (b107683)
**      22-oct-2002 (horda03) Bug 108988
**          Wrong number of parameters specified for E_SC033D error.
**	22-nov-2002 (mcgem01)
**	    Implement a server-side license check for OpenROAD Tansformation
**	    runtime.
**      17-dec-2002 (stial01)
**          Fixed security loophole with application flags (b109322)
**	17-mar-2003 (somsa01)
**	    In scs_initiate(), corrected cross-integration of fix for bug
**	    102108.
**	04-sep-2003 (abbjo03)
**	    Add missing declaration of cp in xDEBUG block in scs_initiate.
**	20-Apr-2004 (bonro01)
**	    Allow Ingres to be installed as a user other than 'ingres'
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	30-aug-06 (thaju02)
**	    In scs_initiate(), only pass PSQ_RULE_DEL_PREFETCH to 
**	    psq_bgn_session() if sc_rule_upd_prefetch_off is FALSE. 
**	    (B116355)
**	30-Nov-2006 (kschendel) b122041
**	    Fix ult_check_macro usage.
**      01-dec-2008 (stial01)
**          scs_dbdb_info() Set sc_dbdb_dbtuple only if db upgraded.
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**      12-mar-2010 (joea)
**          Update adf_proto_level with AD_BOOLEAN_PROTO if at
**          GCA_PROTOCOL_LEVEL_68.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      11-Aug-2010 (hanal04) Bug 124180
**          Added money_compat for backwards compatibility of money
**          string constants.
**/

/*
**  Forward and/or External function references.
*/
FUNC_EXTERN STATUS	adu_datenow(ADF_CB *, DB_DATA_VALUE *);
FUNC_EXTERN DB_STATUS	scs_check_external_password(SCD_SCB *,
						    DB_OWN_NAME *,
						    DB_OWN_NAME *,
						    DB_PASSWORD *,
						    bool);

/* Static routine called within this module. This routine is supposed */
/* to be consistent with the routine IICXconv_to_struct_xa_xid in the */
/* IICX sub-component of LIBQXA. They will become one if/when IICX is */
/* moved to 'common'                                                  */
/* 22-sep-1993 (iyer)                                                 */
/*  Changed the prototype to accept pointer to DB_XA_EXTD_DIS_TRAN_ID */

DB_STATUS conv_to_struct_xa_xid( char               *xid_text,
                                 i4             xid_str_max_len,
                                 DB_XA_EXTD_DIS_TRAN_ID  *xa_extd_xid_p );

static void	scs_cut_cleanup(void);

static DB_STATUS scs_usr_expired ( 
    SCD_SCB	*scb);

static DB_STATUS scs_check_app_code(
    SCD_SCB	*scb,
    SCS_DBPRIV	*idbpriv,
    bool	is_dbdb);

/*
** Definition of all global variables owned by this file.
*/

GLOBALREF SC_MAIN_CB          *Sc_main_cb; /* Central structure for SCF */
GLOBALREF DU_DATABASE	      dbdb_dbtuple;

# define IGNORE_CASE 1

/*{
** Name: scs_initiate	- Start up a session
**
** Description:
**      This routine performs the initialization necessary to get a session 
**      going within the Ingres DBMS.   At the point this routine is called, 
**      the dispatcher will  have already  established a context in which to 
**      run the session;  that is, the scb will have already been allocated, 
**      and the communication links required will have been set up (that is, 
**      the status channel back to the frontend will be open).
** 
**      This routine, then, must  
**	    - log the begin time for the session 
**          - move the interesting information from the crb into the sscb 
**	    - find the interesting information from the dbdb session
**          - create this session for the other facilities 
**          - return the status to the caller so that the session may be 
**		be allowed to proceed.
**
** Inputs:
**      scb				the session control block
**
** Outputs:
**	    none
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-Jun-1986 (fred)
**          Created.
**      14-Jul-1987 (fred)
**          GCF support added.
**	15-Jun-1988 (rogerk)
**	    Added support for Fast Commit thread.
**      1-aug-88 (eric)
**          added support for verifydb
**	15-Sep-1988 (rogerk)
**	    Added support for Write Behind threads.  Only init facilities
**	    needed (adf and dmf).
**	07-nov-88 (thurston)
**	    Init new ADF_CB.adf_maxstring to DB_MAXSTRING.
**	10-Mar-1989 (ac)
**	    Added 2PC support.
**	15-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    copy group id to SCS_ICS, PSQ_CB;
**	    copy application id and password to SCS_ICS, PSQ_CB;
**	    call scu_xencode() to encrypt password
**	04-apr-1989 (paul)
**	    Initialize max depth for QEF
**	20-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Initialize database privileges in the SCS_ICS
**	12-may-1989 (anton)
**	    Set collation when database is opened
**	18-may-1989 (neil)
**	    Initialize max rule depth for QEF from server startup
**	    parameter.  For debugging allow it to be changed per session.
**      20-jun-89 (jennifer)
**          Changed the key initialization for qeu_get and dmf calls to 
**          use the new key attribute defintions instead of constants.
**      29-JUN-89 (JENNIFER)
**          Added DMC_ALTER call to alter the dmf session according 
**          to status from user tuple.  This insures that all the 
**          new user statuses are set for a session.
**      5-jul-89 (jennifer)
**          Added code to initialize DMF audit state from iidbdb catalog
**          iisecuritystate.  Only do this first database open per 
**          server.
**	20-aug-1989 (ralph)
**	    Set DMC_CVCFG for config file conversion
**	05-sep-89 (jrb)
**	    Added code to fill in new ics_num_literals field in scs_ics block
**	    depending on setting of GCA_MD_ASSOC index.  Also, sends this info
**	    to PSQ_BGN_SESS so that PSF can determine which numeric literals
**	    to use.  Default is decimal.
**	27-oct-89 (ralph)
**	    Pass user name & status flags to QEF and PSF during session init.
**	16-jan-90 (ralph)
**	    Verify user password.
**	30-jan-90 (ralph)
**	    Initialize user status bits to zero for qef.
**	08-aug-90 (ralph)
**	    Bug 30809 - Force OPF enumeration if query limit in effect
**	31-aug-90 (andre)
**	    If running createdb, RDF must be notified to avoid checking for
**	    synonyms.
**	14-sep-90 (teresa)
**	    many PSQ_CB booleans have become bitflags in psq_flag:
**	    PSQ_FORCE, PSQ_CATUPD, PSQ_WARNINGS, PSQ_DBA_DROP_ALL, 
**	    PSQ_FIPS_MODE
**	20-dec-90 (teresa)
**	    set PSQ_WARNINGS conditionally dependent of existance of a DUF
**	    QUEL application flag: 
**		DBA_CREATEDB or DBA_SYSMOD or DBA_DESTROYDB or DBA_FINDDBS or
**		DBA_CONVERT or DBA_CREATE_DBDBF1
**	15-jan-91 (ralph)
**	    Use DBPR_OPFENUM to determine if OPF enumeration should be forced.
**	05-feb-91 (ralph)
**          Initialize SCS_ICS.ics_sessid
**	    Fold database name to lower case (bug 21691)
**	04-feb-91 (neil)
**	    Alerters: Initialize event fields in scs_initiate.
**      26-MAR-1991 (JENNIFER)
**          For dmf_alter session added send of DOWNGRADE privilege.
**	21-may-91 (andre)
**	    will store values of user, role, and group identifiers which were in
**	    effect at session startup in the new SCS_ICS fields: ics_susername,
**	    ics_saplid, and ics_sgrpid;
**	    
**	    renamed some fields in SCS_ICS to make it easier to determine by the
**	    name whether the field contains info pertaining to the system (also
**	    known as "Real"), Session ("he who invoked us"), or the Effective
**	    identifier:
**		ics_ustat     --> ics_rustat	  real (system) user status bits
**		ics_estat     --> ics_sustat	  session user status bits
**		ics_username  --> ics_eusername   effective user id
**		ics_upass     --> ics_supass	  session user password
**		ics_grpid     --> ics_egrpid	  effective group id
**		ics_gpass     --> ics_sgpass	  password for session group id
**		ics_aplid     --> ics_eaplid      effective role id
**		ics_apass     --> ics_sapass	  password for session role id
**	25-nov-91 (teresa)
**	    If running upgradedb, RDF must be notified to avoid checking for
**	    synonyms.
**	26-feb-92 (andre)
**	    if scb->scb_sscb.sscb_ics.ics_appl_code == DBA_PRETEND_VFDB, set
**	    PSQ_REPAIR_SYSCAT in PSQ_CB.psq_cb before calling PSF to start a
**	    session
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              22-aug-88 (alexh)
**                  mark DMF off the facility list if STAR only.
**                  Upgrade for RDF, QEF for titan sessions.
**              23-jun-89 (seputis)
**                  fixed for iimonitor threads, so that DDB ptr consistency
**                  checks are not made
**              12-dec-90 (fpang)
**                  Skip GCA_TIMEZONE for now. Will need to be fixed when
**                  we figure out how to handle it.
**              08-feb-91 (fpang)
**                  Passed the wrong paramter to MEcopy when copying f4style
**                  and f8styles.
**              14-Feb-1991 (fpang)
**                  There is a small window where an incoming association can 
**		    get an empty cdb descriptor. It happens when two incoming 
**		    associations to the same ddb are close to together. The cdb 
**		    descriptor is allocated but empty when the second 
**		    association comes in.  Scs_inititate thinks that it has 
**		    already been filled in and does not call qef to confirm 
**		    the descriptor. Usual symptoms are RQF failing associations
**		    with errors from the local
**              13-aug-1991 (fpang)
**                  Support for GCA_TIMEZONE and GCA_GW_PARMS.
**          End of STAR comments.
**      29-jul-1992 (fpang)
**          Inititalize rdf_ddb_id for distributed.
**	25-sep-92 (markg)
**	    Added code to initiate SXF sessions.
**	7-oct-1992 (bryanp)
**	    No user connections in the Recovery Server.
**	7-oct-92 (daveb)
**	    Make GWF a first class facility, and set it up here.  Name the
**	    semaphore.
**	03-nov-92 (robf)
**	    Startup SXF on MONITOR sessions, otherwise could get failures
**	    on audit attempts later.
**	    Audit usage of MONITOR sessions
**	    Make sure User Stats are set prior to first SXR_WRITE SXF call.
**	23-nov-92 (ed)
**	    Changed DB_OWN_NAME to DB_PASSWORD for "maxname" project
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**
**	    As a part of desupporting SET GROUP/ROLE, ics_sapass was renamed to
**	    ics_eapass and ics_sgpass to ics_egpass + delete ics_sgrpid and
**	    ics_saplid
**	    
**	    the following fields in SCS_ICS have been renamed:
**		ics_supass	--> ics_iupass
**		ics_sustat	--> ics_iustat
**      09-aug-1992 (stevet)
**          Fixed a problem where STcopy() is called with wrong arguments.
**	14-dec-92 (rganski)
**	    Changed checking of application flag (case GCA_APPLICATION) from
**	    bunch of if statements to a switch statement. Added check for
**	    obsolete flag DBA_VERIFYDB, which returns an optimizedb error.
**	19-nov-92 (fpang)
**	    In scs_initiate(), when waiting for 2PC thread to finish, 
**	    reinitialize status after each timeout. Eliminates spurious 
**	    'Session initiate failures'.
**	20-nov-1992 (markg)
**	    Add support for the audit thread.
**	28-dec-1992 (robf)
**	    Removed DMF settings for AUDIT_ALL and  DOWNGRADE, these are now
**	    automatically handled by SXF.
**	14-jan-93 (andre)
**	    Tell PSF if this session is running UPGRADEDB
**	18-jan-1993 (bryanp)
**	    Added support for CPTIMER thread.
**	19-mar-1993 (barbara)
**	    Don't call scs_icsxlate if STAR thread.
**	24-mar-1993 (barbara)
**	    No need to copy username into ics_effect_user for Star.  This
**	    field is no longer used.
**	31-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Pass case translation flags and catalog owner name to 
**	    DMF, PSF, OPF, QEF and RDF when started in scs_initiate.
**	    Move call to scs_icsxlate() to scs_dbdb_info()
**	8-apr-1993 (robf)
**	    Make +U/+Y calls subservient to database privilege, rather than
**	    disallowing in a B1 environment. That is, a frontend may still
**	    issues these flags for backwards compatibility, but will only
**	    be accepted if the database privileges say that the session
**	    would be allowed UPDATE_SYSCAT privilege anyway.
**      26-apr-1993 (fpang)
**          Star. If monitor session is initiated by ingres, add QEF to the
**          facilities needed, just in case the iidbdb needs to be opened.
**          Fixes 50327 (IIMONITOR on STAR server as ingres gets SEGV).
**      07-may-93 (anitap)
**          Changes for "SET UPDATE_ROWCOUNT" statement.
**      24-May-1993 (fred)
**          Added byte protocol to the list of masks supported by the
**          6.5 protocol.
**	05-may-1993 (ralph)
**	    DELIM_IDENT:
**	    Bug fixes, including:
**		Don't translate delimited effective user, group and role
**		identifiers until the case translation semantics for the
**		database is known.
**		Rework handling of role and group passwords to allow
**		slash to appear in a delimited role or group.
**	26-jul-1993 (bryanp)
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	    Correct an off-by-one error in distributed tran ID parsing.
**	29-Jun-1993 (daveb)
**	    - cast initial arguments to cui_idxlate to u_char *.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	16-Jul-1993 (daveb)
**	    Add back cp, used in xDEBUG code.
**      28-aug-1993 (stevet)
**          Return E_DB_ERROR, not E_DB_FATAL, when session connected with an
**          invalid timezone name.
**	15-sep-1993 (tam)
**	    Append '\0' to star_comflags.dd_co26_msign for proper 
**	    initialization of II_MONEY_FORMAT in star (bug #54762).
**	16-Sep-1993 (fpang)
**	    Added Star support for -G (GCA_GRPID) and -R (GCA_APLID).
**	22-sep-93 (andre)
**	    tell QEF if starting up a session to run upgradedb
**	11-Oct-1993 (fpang)
**	    Added Star support for -kf (GCA_DECFLOAT).
**	    Fixes B55510 (Star doesn't support -kf flag).
**      22-Sep-1993 (iyer)
**          Call to conv_to_struct_xa_xid now passes a pointer to 
**          DB_XA_EXTD_DIS_TRAN_ID for XA transactions within case for
**          GCA_XA_RESTART.
**	01-nov-93 (anitap)
**	    if scb->scb_sscb.sscb_ics.ics_appl_code == DBA_PRETEND_VFDB, set
**	    PSQ_INGRES_PRIV in PSQ_CB.psq_cb before calling PSF to start a
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**          This results in owner within sc0m_allocate having to be PTR.
**          Also fixed up a number of casts.
**	09-nov-93 (rblumer)
**	    Fixed bug 55170, where DBAname != username even when user IS the
**	    DBA!!  (happens in FIPS database when have INGRES iidbdb).
**	    Changed ics_eusername to always point to ics_susername.
**	23-nov-93 (stephenb)
**	    Set DMC_S_SECURITY for special applications, because they need to
**	    manipulate security catalogs and the user may not have security
**	    privilege.
**	22-nov-93 (robf)
**          Add STAR support for passing user password to LDB
**	    Initialize group/role for QEF
**	01-Dec-1993 (fpang)
**	    Initialize dd_co36_aplid and dd_c037_grpid to EOS. 
**	    Fixes B57478, spurious remote connect failiures.
**	17-dec-93 (rblumer)
**	    removed sc_fips field.
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).
**      05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with
**	    DM0M_OBJECT.
**          This results in owner within sc0m_allocate having to be PTR.
**          Also fixed up a number of casts.
**	14-feb-94 (robf)
**          Terminator thread only needs SXF, ADF & DMF.
**	1-mar-94 (robf)
**          Log E_SC034F_NO_FLAG_PRIV since E_US0002 is rather vague
**	    for administrator traceback.
**       2-Mar-1994 (daveb) 58423
**          change the way monitor sessions are validated.  Never
**          check the user in the dbdb, because the logging system may
**          be down.  Instead, always check the PM SERVER_STARTUP
**          privilege for the user (or DB_NDINGRES_NAME).  This way
**          you  can monitor servers when the logging system is down and
**          you can't get to the dbdb.  Unfortunately, it means you
**          must have the PM priv, and the DB stored privs will never
**          be considered.  We use scs_chk_priv() to do the PM work.
**      23-Mar-1994 (daveb) 60410
**          check the return from scs_allocate_cbs and give up
**  	    initiating the session if it fails; otherwise SEGV city.
**	18-mar-94 (robf)
**          Rework dbprivs, modularized out into scsdbprv.c and controlled
**	    through SCS_DBPRIV structures. This allows better manipulation 
**	    of changing dbprivs with subsequent SET ROLE statements
**	6-may-94 (robf)
**           Added back cp, needed by by xDEBUG code
**	23-may-94 (robf)
**           DMC_ALTER call may be passed illegal combination of
**	     DMC_S_NORMAL|DMC_S_SECURITY. Reworked flag setting to only
**	     set DMC_S_NORMAL if no other flags are set.
**	18-jul-94 (robf)
**           Reordered GCA switch to restore behaviour of older code which
**           depends on a fall-through in the switch to get +U implying -l 
**           (most programs specify -l but some, like upgradedb, use the 
**           implicit lock.) I also added a comment in the switch to make
**           things clearer.
**      29-nov-1994 (harpa06)
**          Integrated bug fix #62708 from INGRES 6.4 by angusm:
**          Remap QEOO5A as for QE0054 (failure to reconnect to
**          WILLING_COMMIT TX), bug 62708.
**	15-dec-94 (nanpr01)
**           Bug # 63222 : Destroydb doesnot need DMC_CVCFG flag set. If we 
**           set that flag the cached iidbdb tuple gets reset by hardcoded 
**           iidbdb tuple.  Consequently, FIPS mode information were lost 
**	     and destroydb will ask for password.
**	27-apr-95 (cohmi01)
**	    Add restrictions for IOMASTER server.
**      10-May-1995 (lewda02/shust01)
**          RAAT API Project:
**          Initialize new sscb_raat_message pointer to NULL.
**      14-Sep-1995 (shust01/thaju02)
**          RAAT API Project:
**          Initialize new sscb_big_buffer pointer to NULL.
**          Initialize new sscb_raat_workarea pointer to NULL.
**          Free allocated memory (if any) from pointers.
**	13-nov-95 (nick)
**	    Support for II_DATE_CENTURY_BOUNDARY.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	6-jun-96 (stephenb)
**	    Add code for replicator queue management thread.
**	30-jul-96 (stephenb)
**	    Add QEF to needed facilities for replicator queue management thread.
**	22-May-1997 (jenjo02)
**	    Moved call to scs_iprime() to early in scs_initiate() rather
**	    than at the end. Sessions stalled waiting on some resource
**	    can't be broken out of until the expedited GCA channel is
**	    set up by scs_iprime(). (bug 77674)
**	27-Jun-1997 (jenjo02)
**	    Removed sc_urdy_semaphore, which served no useful purpose.
**	24-Jul-97 (nanpr01)
**	    Verifydb needs to do alter table drop constraint now with
**	    application code DBA_PURGE_VFDB for dropping constraints
**	    and needs PSQ_INGRES_PRIV.
**	22-jul-1997 (nanpr01)
**	    Moved the rdf begin session call ahead. For Star, it was calling
**	    rdf before session initialization and causing segmentation 
**	    fault.
*	09-feb-1998 (walro03 for stephenb)
**	    When starting up Replicator queue management threads, tell SCF that
**	    we should also start an SXF session.
**	29-jul-1998 (shust01)
**	    Initialized adf_cb->adf_errcb.ad_emsglen to 0 in scs_initiate(). 
**	    Client had instance of calling ule_format() without msg being set
**	    (user hit cntrl-C) causing SEGV, since ad_emsglen has extremely
**	    large value (was uninitialized).
**	09-Mar-1998 (jenjo02)
**	    Added support for Factotum threads.
**	    "need_facilites" is now established in scd_alloc_cbs() and 
**	    passed to scs_initiate() in sscb_need_facilities.
**	02-Jun-1998 (jenjo02)
**	    Instead of initializing ics_dbname to "<no_database>", leave it
**	    as it came from scd_alloc_scb(), initialized to nulls. The
**	    verbage just clutters IPM and the monitor.
**	28-oct-98 (stephenb)
**	    factotum thread changes altered the logic of when a thread
**	    initialization is audited, it was only if both GCA and SXF are needed
**	    it is now if either GCA or SXF is needed, this causes auditing to
**	    exit because the audit thread needs SXF (but not GCA) and tries to
**	    audit it's own initialization, which it can't do because the audit 
**	    thread is not started. Reverted to the old (correct) logic.
**	15-dec-98 (stephenb)
**	    Above change needs to fix user thread auditing, which was also
**	    broken. Added this.
**	04-feb-1999 (muhpa01)
**	    Fix to scs_initiate() to correct screening of internal threads
**	    for calls to sc0a_write_audit().  This corrects DBMS server startup
**	    problems resulting in SX103F errors in sxas_check().
**	01-Jul-1999 (shero03)
**	    Support II_MONEY_FORMAT=NONE for SIR 92541.
**	01-oct-1999 (somsa01)
**	    Added the setting of the Ingres system privileges.
**	14-Jul-2000 (hanje04)
**	    As work around for an E_CL2514_CS_ESCAPED_EXCEPTION during
**	    ingstart, added if(!scb->scb_sscb.sscb_quccb) to session
**	    reconnection
**      25-Jul-2000 (hanal04) Bug 102108 INGSRV 1230
**          Set SCS_ASEM once to indicate that the initialisation of the
**          sscb_asemaphore has taken place.
**      07-Feb-2001 (horda03) Bug 103927
**          Initialize Last event and Number of queued events.
**	08-Sep-2000 (hanch04)
**	    Added check for EMBED_USER so that iidbdb could be created by
**	    users other than ingres.
**	03-nov-2000 (somsa01)
**	    Added setting of AD_B1_PROTO when B1 security is turned on.
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          Pass PSQ_RULE_UPD_PREFETCH to psq_bgn_session unless
**          sc_rule_upd_prefetch_off is set to TRUE.
**	16-mar-2001 (stephenb)
**	    Set unicode collation in adf_cb.
**	12-apr-2002 (devjo01)
**	    Correct size of 'node' buffer.
**	17-mar-2003 (somsa01)
**	    Corrected cross-integration of fix for bug 102108.
**	12-apr-04 (inkdo01)
**	    Added support for BIGINT.
**	05-May-2004 (jenjo02)
**	    Notify DMF if this is a Factotum thread so it can
**	    modify its behavior.
**	21-feb-05 (inkdo01)
**	    Init Unicode normalization flag.
**	04-Apr-2005 (jenjo02)
**	    Init adf_autohash.
**	6-Jul-2006 (kschendel)
**	    i4-compat flag was not being set for pre-OI-1.2 clients.
**	    Update type of unique db ID in all control blocks (is i4).
**      30-aug-06 (thaju02)
**          Only pass PSQ_RULE_DEL_PREFETCH to psq_bgn_session() if
**          sc_rule_upd_prefetch_off is FALSE. (B116355)
**	18-Sep-2006 (gupsh01)
**	    Added support for handing DATE_TYPE_ALIAS configuration
**	    parameter.
**	02-Oct-2006 (gupsh01)
**	    Pick up the setting of DATE_TYPE_ALIAS parameter from 
**	    Sc_main_cb.
**	23-Apr-2007 (kiria01) 118141
**	    Explicitly initialise Unicode items in the ADF_CB so that
**	    the Unicode session characteristics get reset per session.
**	27-apr-2007 (dougi)
**	    Init adf_utf8_flag.
**	10-Jul-2007 (kibro01) b118702
**	    Ensure date_type_alias defaults to ingresdate
**	23-Jul-2007 (smeke01) b118798.
**	    Added adf_proto_level flag value AD_I8_PROTO. adf_proto_level 
**	    has AD_I8_PROTO set iff the client session can cope with i8s, 
**	    which is the case if the client session is at GCA_PROTOCOL_LEVEL_65 
**	    or greater. 
**	03-Aug-2007 (gupsh01)
**	    Modified the date_type_alias setting.
**	24-aug-2007 (toumi01) bug 119015
**	    Add dd_co42_date_alias to Star initializations. Noticed by code
**	    inspection; not necessarily required for related bug fix.
**	11-mar-2008 (toumi01)
**	    For UTF8 installs and STAR we can't rely on DMF to load the
**	    ucollation for us, so do it here.
**	13-May-2008 (kibro01) b120369
**	    Add dmc_flags_mask2 holding DMC_ADMIN_CMD flag
**	8-Jul-2008 (kibro01) b120584
**	    Add AD_ANSIDATE_PROTO.
**	15-Sep-2008 (kibro01)
**	    Add PSQ_ALLOW_TRACEFILE_CHANGE flag.
**      26-Nov-2009 (hanal04) Bug 122938
**          Different GCA protocol levels can handle different levels
**          of decimal precision.
**	15-Oct-2010 (kschendel) SIR 124544
**	    Session startup result-structure goes to PSF now, not OPF.
**	    Use generic structure looker-upper instead of hand coding.
**	04-nov-2010 (maspa05) bug 124654, 124687
**	    Moved SC930 SESSION BEGINS here from PSQ so that we get a session
**          begin even when no query is issued, as can happen with XA 
**          operations
*/
i4
scs_initiate(SCD_SCB *scb )
{
    i4			i;
    i4			dash_u = 0;
    i4			tbl_structure = 0;
    i4			result_structure = 0;
    i4			flag_value;
    GCA_USER_DATA	*flag;
    DB_STATUS		status = E_DB_OK;
    DMC_CB		*dmc;
    ADF_CB		*adf_cb;
    OPF_CB		*opf_cb;
    PSQ_CB		*psq_cb;
    QSF_RCB		*qsf_cb;
    QEF_RCB		*qef_cb;
    RDF_CCB		*rdf_cb;
    SXF_RCB		*sxf_cb;
    GW_RCB		*gw_rcb;
    GCA_SESSION_PARMS	*crb;
    DB_ERROR		error;
    DB_ERROR		local_error;
    char		node[CX_MAX_NODE_NAME_LEN];
    i4		p_index;
    i4		l_value;
    u_i4		l_pass, l_id;
    bool                timezone_recd = FALSE;
    bool                year_cutoff_recd = FALSE;
    char		*pass_loc, *id_loc;
    i4			sess_rule_depth = -1;	/* "unset" value */
    SCF_CB		scf_cb;
    i4		        access;
    DB_STATUS           local_status;
    char                dbstr[DB_DB_MAXNAME+1];
    char                typestr[DB_TYPE_MAXLEN+1];
    char                tempstr[DB_MAXNAME+1];
    DD_COM_FLAGS        star_comflags;
    bool                cdb_deferred = FALSE;
    char		sem_name[ CS_SEM_NAME_LEN ];
    u_i4		idmode;
    bool		want_updsyscat=FALSE;
#ifdef xDEBUG
    char		*cp;
#endif
    bool		date_alias_recd = FALSE;

    error.err_code = 0;
    error.err_data = 0;

    Sc_main_cb = (SC_MAIN_CB *) scb->cs_scb.cs_svcb;

    /*
    ** Here's what's happened up to this point:
    **
    ** scd_alloc_scb() has determined, based on the thread type,
    ** what facilities will be needed and has allocated the SCB
    ** and facilties storage in one piece of memory. Up til now, all
    ** the code to get this thread going has run under the creating
    ** session's thread; from here on in, the code is running under 
    ** the new thread and the creating thread is free to persue other 
    ** interests.
    **
    ** If a FACTOTUM session, scd_allocate_scb() has copied the
    ** ICS (sscb_ics) structure from the creating (parent) thread's
    ** SCB to this session's SCB, so we don't want to destroy
    ** that information by reinitializing it here.
    **
    ** What remains is to complete initialization of the SCB
    ** and then make the session initialization
    ** calls to the various "needed" facilities.
    **
    ** Once that's complete, we return to the sequencer, who'll
    ** then invoke the code to run or sequence this thread.
    **
    ** Whether this session has a f/e GCA connection is indicated
    ** by the mask (1 << DB_GCF_ID) in sscb_need_facilities.
    **
    ** One should not assume that a facility's cb and work area(s)
    ** exist; they do iff that facility's mask has been set
    ** in sscb_need_facilities!
    */

    /* Done to make MVS/UNIX (maybe) work */
    
    node[0] = 0;
    if (CXcluster_enabled())
    {
	CXnode_name(node);
    }
    ule_initiate(node,
		    (i4)STlength(node),
		    Sc_main_cb->sc_sname,
		    sizeof(Sc_main_cb->sc_sname));

    ult_init_macro(&scb->scb_sscb.sscb_trace, SCS_TBIT_COUNT, SCS_TVALUE_COUNT, SCS_TVALUE_COUNT);

    /* No alarms initially */
    scb->scb_sscb.sscb_alcb=NULL;
    
    /*
    ** Session connect time, as SYSTIME
    */
    TMnow(&scb->scb_sscb.sscb_connect_time);
	
    /* Initialize alert structures */
    scb->scb_sscb.sscb_alerts = NULL;
    scb->scb_sscb.sscb_aflags = 0;
    scb->scb_sscb.sscb_astate = SCS_ASDEFAULT;
    CSw_semaphore(&scb->scb_sscb.sscb_asemaphore, CS_SEM_SINGLE,
		    STprintf( sem_name, "SSCB %p ASEM", scb ));
    scb->scb_sscb.sscb_aflags |= SCS_ASEM;

    /*
    ** Initialize sscb_raat_message to NULL.  Memory will be allocated by
    ** the RAAT API if it is used.  This memory will be freed in
    ** scs_terminate.
    */
    scb->scb_sscb.sscb_raat_message = NULL;
    scb->scb_sscb.sscb_big_buffer   = NULL;
    scb->scb_sscb.sscb_raat_buffer_used = 0;
    scb->scb_sscb.sscb_raat_workarea = NULL;

    /*
    ** Initialize Ingres startup and session parameters.
    **
    ** If this is a Factotum thread, the creating (parent)
    ** thread's sscb_ics has been copied to this 
    ** thread's sscb_ics and must be preserved.
    */

    /* Update ics_sessid with this thread's id */
    (VOID)CVptrax((PTR)scb->cs_scb.cs_self, (PTR)tempstr);

    (VOID)MEcopy((PTR)tempstr,
		 sizeof(scb->scb_sscb.sscb_ics.ics_sessid),
		 (PTR)scb->scb_sscb.sscb_ics.ics_sessid);

    if (scb->scb_sscb.sscb_stype != SCS_SFACTOTUM)
    {
	scb->scb_sscb.sscb_ics.ics_db_access_mode = DMC_A_WRITE;
	scb->scb_sscb.sscb_ics.ics_lock_mode = DMC_L_SHARE;
	scb->scb_sscb.sscb_ics.ics_dmf_flags |= DMC_NOWAIT;


	/*
	** Initialize all dbprivs in scb, this is reset later in scs_dbdb_info
	*/
	scb->scb_sscb.sscb_ics.ics_ctl_dbpriv  = DBPR_ALL;
	scb->scb_sscb.sscb_ics.ics_fl_dbpriv   = (DBPR_BINARY|DBPR_PRIORITY_LIMIT);
	scb->scb_sscb.sscb_ics.ics_qrow_limit  = -1;	
	scb->scb_sscb.sscb_ics.ics_qdio_limit  = -1;	
	scb->scb_sscb.sscb_ics.ics_qcpu_limit  = -1;	
	scb->scb_sscb.sscb_ics.ics_qpage_limit = -1;	
	scb->scb_sscb.sscb_ics.ics_qcost_limit = -1;
	scb->scb_sscb.sscb_ics.ics_idle_limit    = -1;
	scb->scb_sscb.sscb_ics.ics_connect_limit = -1;
	scb->scb_sscb.sscb_ics.ics_priority_limit=  -1;
	scb->scb_sscb.sscb_ics.ics_cur_idle_limit    = -1;
	scb->scb_sscb.sscb_ics.ics_cur_connect_limit = -1;

	/*
	**  Leave ics_dbname null-filled for MONITOR and internal
	**  sessions which have no named database.
	*/

	scb->scb_sscb.sscb_ics.ics_language = 1;
	if (scb->scb_sscb.sscb_stype == SCS_S2PC)
	{
	    scb->scb_sscb.sscb_ics.ics_qlang = DB_SQL;
	}
	else
	{
	    scb->scb_sscb.sscb_ics.ics_qlang = DB_QUEL;
	}

	scb->scb_sscb.sscb_ics.ics_decimal.db_decimal = '.';
	scb->scb_sscb.sscb_ics.ics_decimal.db_decspec = TRUE;
	scb->scb_sscb.sscb_ics.ics_date_format = DB_US_DFMT;
	scb->scb_sscb.sscb_ics.ics_money_format.db_mny_sym[0] = '$',
	    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_sym[1] = ' ';
	scb->scb_sscb.sscb_ics.ics_money_format.db_mny_lort = DB_LEAD_MONY;
	scb->scb_sscb.sscb_ics.ics_money_format.db_mny_prec = 2;
	
	scb->scb_sscb.sscb_ics.ics_idxstruct = DB_ISAM_STORE;
	scb->scb_sscb.sscb_ics.ics_parser_compat = 0;
	scb->scb_sscb.sscb_ics.ics_mathex = (i4) ADX_ERR_MATHEX;
	/* scb_cscb not allocated if no GCA */
	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID) )
	{
	    /* Set parser compat flags for various situations. */
	    if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_60)
	    {
		/* If pre-OI1.2 client, default to float literals instead
		** of decimals, and ignore math exceptions.
		*/
		scb->scb_sscb.sscb_ics.ics_parser_compat |= (PSQ_FLOAT_LITS | PSQ_I4_TIDS);
		scb->scb_sscb.sscb_ics.ics_mathex = (i4) ADX_IGN_MATHEX;
	    }
	    else if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_65)
	    {
		/* If pre-r3 client, treat tids exposed to client as
		** i4 instead of i8.
		*/
		scb->scb_sscb.sscb_ics.ics_parser_compat |= PSQ_I4_TIDS;
	    }
	}
	scb->scb_sscb.sscb_ics.ics_outarg.ad_c0width = -1;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_i1width = -1;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_i2width = -1;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_i4width = -1;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_i8width = -1;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_f4width = -1;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_f8width = -1;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_f4prec = -1;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_f8prec = -1;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_f4style = 0;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_f8style = 0;
	scb->scb_sscb.sscb_ics.ics_outarg.ad_t0width = -1;

	(VOID)MEcopy((PTR)DB_ABSENT_NAME,
		     sizeof(scb->scb_sscb.sscb_ics.ics_xegrpid),
		     (PTR)&scb->scb_sscb.sscb_ics.ics_xegrpid);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_xegrpid,
			    scb->scb_sscb.sscb_ics.ics_egrpid);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_xegrpid,
			    scb->scb_sscb.sscb_ics.ics_xeaplid);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_xegrpid,
			    scb->scb_sscb.sscb_ics.ics_eaplid);

	(VOID)MEfill(sizeof(scb->scb_sscb.sscb_ics.ics_eapass),
		     (u_char)' ', 
		     (PTR)&scb->scb_sscb.sscb_ics.ics_eapass);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_eapass,
			    scb->scb_sscb.sscb_ics.ics_egpass);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_eapass,
			    scb->scb_sscb.sscb_ics.ics_iupass);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_eapass,
			    scb->scb_sscb.sscb_ics.ics_rupass);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_eapass,
			    scb->scb_sscb.sscb_ics.ics_srupass);

	(VOID)MEmove(sizeof(DB_INGRES_NAME)-1, (PTR)DB_INGRES_NAME, ' ',
		     sizeof(scb->scb_sscb.sscb_ics.ics_cat_owner),
		     (PTR)&scb->scb_sscb.sscb_ics.ics_cat_owner);

	scb->scb_sscb.sscb_ics.ics_dbxlate = 0x00;

	/*
	** Point the effective userid to the SQL session authid
	*/
	scb->scb_sscb.sscb_ics.ics_eusername =
	    &scb->scb_sscb.sscb_ics.ics_susername;
	

	scb->scb_sscb.sscb_ics.ics_strtrunc = (i4) ADF_IGN_STRTRUNC;
    }

    /*
    ** These are STAR command flags that need to be propagated to the LDBs
    ** at association startup time. Initialize them.
    */
    if (scb->scb_sscb.sscb_flags & SCS_STAR)
    {
	star_comflags.dd_co0_active_flags = 0;
	star_comflags.dd_co1_exlusive = FALSE;
	star_comflags.dd_co2_application =
			DBA_MIN_APPLICATION -1;         /* GCA_APPLICATION */
	star_comflags.dd_co3_qlanguage = DB_SQL;        /* GCA_QLANGUAGE */
	star_comflags.dd_co4_cwidth = -1;               /* GCA_CWIDTH */
	star_comflags.dd_co5_twidth = -1;               /* GCA_TWIDTH */
	star_comflags.dd_co6_i1width = -1;              /* GCA_I1WIDTH */
	star_comflags.dd_co7_i2width = -1;              /* GCA_I2WIDTH */
	star_comflags.dd_co8_i4width = -1;              /* GCA_I4WIDTH */
	star_comflags.dd_co9_f4width = -1;              /* GCA_F4WIDTH */
	star_comflags.dd_co10_f4precision = -1;         /* GCA_F4PRECISION */
	star_comflags.dd_co11_f8width = -1;             /* GCA_F8WIDTH */
	star_comflags.dd_co12_f8precision = -1;         /* GCA_F8PRECISION */
	star_comflags.dd_co13_nlanguage = -1;           /* GCA_NLANGUAGE */
	star_comflags.dd_co14_mprec = -1;               /* GCA_MPREC */
	star_comflags.dd_co15_mlort = -1;               /* GCA_MLORT */
	star_comflags.dd_co16_date_frmt = -1;           /* GCA_DATE_FORMAT */
	star_comflags.dd_co17_idx_struct[0] = EOS;      /* GCA_IDX_STRUCT */
	star_comflags.dd_co18_len_idx = 0;              /* length of previous */
	star_comflags.dd_co19_res_struct[0] = EOS;      /* GCA_RES_STRUCT */
	star_comflags.dd_co20_len_res = 0;              /* length of previous */
	star_comflags.dd_co21_euser[0] = EOS;           /* GCA_EUSER */
	star_comflags.dd_co22_len_euser = 0;            /* length of previous */
	star_comflags.dd_co23_mathex[0] = EOS;          /* GCA_MATHEX */
	star_comflags.dd_co24_f4style[0] = EOS;         /* GCA_F4STYLE */
	star_comflags.dd_co25_f8style[0] = EOS;         /* GCA_F8STYLE */
	star_comflags.dd_co26_msign[0] = EOS;           /* GCA_MSIGN */
	star_comflags.dd_co27_decimal = -1;		/* GCA_DECIMAL */
	star_comflags.dd_co28_xupsys = -1;		/* GCA_XUPSYS */
	star_comflags.dd_co29_supsys = -1;		/* GCA_SUPSYS */
	star_comflags.dd_co30_wait_lock = -1;		/* GCA_WAIT_LOCK */
	star_comflags.dd_co31_timezone = -1;		/* GCA_TIMEZONE */
	star_comflags.dd_co32_gw_parms = NULL;		/* GCA_GW_PARMS */
	star_comflags.dd_co33_len_gw_parms = 0;		/* length of previous */
	star_comflags.dd_co34_tz_name[0] = EOS;         /* GCA_TIMEZONE_NAME */
	star_comflags.dd_co35_strtrunc[0] = EOS;        /* GCA_STRTRUNC  */
	star_comflags.dd_co36_grpid[0] = EOS;		/* GCA_GRPLID */
	star_comflags.dd_co37_aplid[0] = EOS;		/* GCA_APLID */
	star_comflags.dd_co38_decformat[0] = EOS;	/* GCA_DECFLOAT */
	star_comflags.dd_co39_year_cutoff = TM_DEF_YEAR_CUTOFF;										/* GCA_YEAR_CUTOFF */
	star_comflags.dd_co40_usrpwd[0] = EOS;		/* GCA_RUPASS */
	star_comflags.dd_co41_i8width = -1;             /* GCA_I8WIDTH */
	star_comflags.dd_co42_date_alias[0] = EOS;	/* GCA_DATE_ALIAS */
    }

    /*
    ** If there is a Front End process associated with this session, then
    ** process the session startup parameters.
    */
    if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID))
    {
	scb->scb_sscb.sscb_facility |= (1 << DB_GCF_ID);
	if (scb->scb_cscb.cscb_gci.gca_status != E_GC0000_OK)
	{
	    sc0e_0_put(scb->scb_cscb.cscb_gci.gca_status,
		    &scb->scb_cscb.cscb_gci.gca_os_status );
	    sc0e_0_put(E_SC022F_BAD_GCA_READ, 0 );
	    sc0e_0_put(E_SC0123_SESSION_INITIATE, 0 );
	    error.err_code = E_SC022F_BAD_GCA_READ;
	    return(E_SC022F_BAD_GCA_READ);
	}    
	error.err_code = E_DB_OK;
	if (scb->scb_cscb.cscb_gci.gca_message_type == GCA_RELEASE)
	{
	    /* client send a GCA_RELEASE as the first message - honor it */
	    sc0e_0_put(E_SC0123_SESSION_INITIATE, 0);
	    error.err_code = E_SC0123_SESSION_INITIATE;
	    return(E_SC0123_SESSION_INITIATE);
	}
	if (scb->scb_cscb.cscb_gci.gca_message_type != GCA_MD_ASSOC)
	{
	    sc0e_put(E_SC022D_1ST_BLOCK_NOT_MD_ASSOC, 0, 1,
		    sizeof(scb->scb_cscb.cscb_gci.gca_message_type),
		    (PTR)&scb->scb_cscb.cscb_gci.gca_message_type,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
	    sc0e_0_put(E_SC022B_PROTOCOL_VIOLATION, 0);
	    sc0e_0_put(E_SC0123_SESSION_INITIATE, 0);
	    error.err_code = E_SC022B_PROTOCOL_VIOLATION;
	    return(E_SC022B_PROTOCOL_VIOLATION);
	}
	else if ((!scb->scb_cscb.cscb_gci.gca_end_of_data)
		    || (scb->scb_sscb.sscb_tptr))
	{
	    /*								    */
	    /* In these cases, we allocate memory to store the entire block as  */
	    /* one entity.  Only when we have the entire entity do we bother to */
	    /* process any of it.						    */
	
	    if (scb->scb_sscb.sscb_tnleft <
		    scb->scb_cscb.cscb_gci.gca_d_length)
	    {
		PTR	place;
		
		status = sc0m_allocate(0,
					(i4)(sizeof(SC0M_OBJECT) +
					    (scb->scb_sscb.sscb_tsize +
		      (2 * scb->scb_cscb.cscb_gci.gca_d_length))),
					DB_SCF_ID,
					(PTR) SCS_MEM,
					CV_C_CONST_MACRO('s', 'c', 's', 'i'),
					&place);
		if (status)
		    return(status);
		if (scb->scb_sscb.sscb_tptr)
		{
		    MECOPY_VAR_MACRO(scb->scb_sscb.sscb_troot,
			scb->scb_sscb.sscb_tsize - scb->scb_sscb.sscb_tnleft,
			((char *)place + sizeof(SC0M_OBJECT)));
		}
		scb->scb_sscb.sscb_tnleft +=
			(2 * scb->scb_cscb.cscb_gci.gca_d_length);
		scb->scb_sscb.sscb_tsize += 
			(2 * scb->scb_cscb.cscb_gci.gca_d_length);
		scb->scb_sscb.sscb_troot =
		    (PTR)((char *)place + sizeof(SC0M_OBJECT));
		if (scb->scb_sscb.sscb_tptr)
		{
		    sc0m_deallocate(0, &scb->scb_sscb.sscb_tptr);
		}
		scb->scb_sscb.sscb_tptr = place;
		MECOPY_VAR_MACRO(scb->scb_cscb.cscb_gci.gca_data_area,
			scb->scb_cscb.cscb_gci.gca_d_length,
			((char *) scb->scb_sscb.sscb_troot +
				scb->scb_sscb.sscb_tsize - scb->scb_sscb.sscb_tnleft));
		scb->scb_sscb.sscb_tnleft -=
			    scb->scb_cscb.cscb_gci.gca_d_length;
	    }
	    else if (scb->scb_sscb.sscb_tptr)
	    {
		MECOPY_VAR_MACRO(scb->scb_cscb.cscb_gci.gca_data_area,
			scb->scb_cscb.cscb_gci.gca_d_length,
			((char *) scb->scb_sscb.sscb_troot +
				scb->scb_sscb.sscb_tsize - scb->scb_sscb.sscb_tnleft));
		scb->scb_sscb.sscb_tnleft -=
			    scb->scb_cscb.cscb_gci.gca_d_length;
	    }
	    scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
	    scb->scb_sscb.sscb_state = SCS_INITIATE;
	    if (!scb->scb_cscb.cscb_gci.gca_end_of_data)
		return(E_DB_OK);
	}
	
	scb->scb_cscb.cscb_gci.gca_status = E_SC1000_PROCESSED;
	if (scb->scb_sscb.sscb_tptr)
	    crb = (GCA_SESSION_PARMS *) scb->scb_sscb.sscb_troot;
	else
	    crb = (GCA_SESSION_PARMS *)
			scb->scb_cscb.cscb_gci.gca_data_area;
	
	/*
	** Copy over the ics portion of the scb from the crb.
	** This is the ingres control stuff such as database name,
	** username, query language, etc.
	** Go thru the command line and update things as necessary.
	*/

	flag = &crb->gca_user_data[0];
	for ( i = 0;
		i < crb->gca_l_user_data && !status;
		i++
	    )
	{
	    MECOPY_CONST_MACRO((PTR)&flag->gca_p_index, sizeof (p_index), 
				(PTR)&p_index);
	    MECOPY_CONST_MACRO((PTR)&flag->gca_p_value.gca_l_value, 
			sizeof (l_value), (PTR)&l_value);
	    switch (p_index)
	    {
		case    GCA_VERSION:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co0_active_flags--;
			star_comflags.dd_co2_application = flag_value;
		    }
		    if (flag_value != GCA_V_60)
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
		    }
		    break;
			
		case    GCA_EUSER:
#ifdef xDEBUG	    /* Check session level rule depth "rule_depth=<#>" */

		    cp = (char *)flag->gca_p_value.gca_value;
		    if (   (STncasecmp(cp, "rule_depth=", l_value )==0)
			&& (CVan(cp + 11, &sess_rule_depth) == OK)
			&& (sess_rule_depth >= 0)
		       )
		    {
			break;
		    }
#endif
		    dash_u = 1;
		    scb->scb_sscb.sscb_ics.ics_xmap |= SCS_XEGRP_SPECIFIED;
		    MEfill(sizeof(scb->scb_sscb.sscb_ics.ics_xeusername),
			(u_char)' ', 
			(PTR)&scb->scb_sscb.sscb_ics.ics_xeusername);
		    l_id = l_value;
		    status = cui_idxlate((u_char *)flag->gca_p_value.gca_value, &l_id,
			(u_char *)&scb->scb_sscb.sscb_ics.ics_xeusername,
			(u_i4 *)NULL, (u_i4)CUI_ID_DLM_M,
			&idmode, &error);
		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_uput(error.err_code, 0, 2,
				 sizeof(ERx("Effective user identifier"))-1,
				 (PTR)ERx("Effective user identifier"),
				 l_id,
				  (PTR)&scb->scb_sscb.sscb_ics.ics_xeusername,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }	
		    if (idmode & CUI_ID_QUO)
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }	
		    if (idmode & CUI_ID_DLM)
			scb->scb_sscb.sscb_ics.ics_xmap |= SCS_XDLM_EUSER;
		    else
			scb->scb_sscb.sscb_ics.ics_xmap &= ~SCS_XDLM_EUSER;

		    /* Note: ics_eusername always points to ics_susername
		    ** (at least until we support modules), so the following
		    ** copy sets both names correctly.
		    */
		    MEcopy((PTR) &scb->scb_sscb.sscb_ics.ics_xeusername,
			   sizeof(scb->scb_sscb.sscb_ics.ics_susername),
			   (PTR) &scb->scb_sscb.sscb_ics.ics_susername);

		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			/* Star needs to propogate modify association params
			** to the LDB
			*/
			star_comflags.dd_co22_len_euser = (u_i2)l_value;
			MEmove(	l_value,
				flag->gca_p_value.gca_value,
				' ',
				sizeof(star_comflags.dd_co21_euser),
				star_comflags.dd_co21_euser
			      );
		    }
		    break;

		case    GCA_SUPSYS:
		case    GCA_XUPSYS:
		    want_updsyscat=TRUE;
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			if ( p_index == GCA_SUPSYS)
			    star_comflags.dd_co29_supsys = flag_value;
			else
			    star_comflags.dd_co28_xupsys = flag_value;
		    }
		    if (flag_value != GCA_ON)
			break;
		    scb->scb_sscb.sscb_ics.ics_requstat |= DU_UUPSYSCAT;
		    if (p_index == GCA_SUPSYS)
			break;

		    /* Fall through to GCA_EXCLUSIVE, since +U implies -l */

		case    GCA_EXCLUSIVE:
		    scb->scb_sscb.sscb_ics.ics_lock_mode = DMC_L_EXCLUSIVE;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co1_exlusive = TRUE;
		    }
		    break;

		case GCA_CAN_PROMPT:
		    /* Frontend user is promptable */
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if(flag_value == GCA_ON)
			    scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_PROMPT;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			/* Not handled by star currently */
			star_comflags.dd_co0_active_flags--;
		    }
		    break;
		case    GCA_WAIT_LOCK:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if (flag_value == GCA_ON)
			scb->scb_sscb.sscb_ics.ics_dmf_flags &= ~DMC_NOWAIT;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co30_wait_lock = (i4)flag_value;
		    }
		    break;

		case    GCA_IDX_STRUCT:
		case    GCA_RES_STRUCT:
		{
		    char	*sname;
		    char	value[30];

		    if (l_value >= sizeof(value))
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }

		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			if (p_index == GCA_IDX_STRUCT)
			{
			    star_comflags.dd_co18_len_idx = (u_i2)l_value;
			    MEcopy( flag->gca_p_value.gca_value,
				    l_value,
				    star_comflags.dd_co17_idx_struct
				  );
			}
			else
			{
			    star_comflags.dd_co20_len_res = (u_i2)l_value;
			    MEcopy( flag->gca_p_value.gca_value,
				    l_value,
				    star_comflags.dd_co19_res_struct
				  );
			}
		    }

		    MEcopy(flag->gca_p_value.gca_value, l_value, value);
		    value[l_value] = '\0';
		    CVlower(value);
		    sname = &value[0];
		    if (value[0] == 'c')
			++sname;
		    tbl_structure = uld_struct_xlate(sname);
		    if (tbl_structure == 0)
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    if (value[0] == 'c')
			tbl_structure = -tbl_structure;
		    if (p_index == GCA_IDX_STRUCT)
		    {
			if (abs(tbl_structure) == DB_HEAP_STORE)
			{
			    status = E_DB_ERROR;
			    error.err_code = E_US0022_FLAG_FORMAT;
			    break;
			}
			scb->scb_sscb.sscb_ics.ics_idxstruct = tbl_structure;
			tbl_structure = 0;
		    }
		    else
		    {
			result_structure = tbl_structure;
		    }
		    break;
		}

		case    GCA_MATHEX:
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co23_mathex[0] = 
#ifdef axp_osf
					CHCAST_MACRO(&flag->gca_p_value.gca_value[0]);
		    }
		    if (CHCAST_MACRO(&flag->gca_p_value.gca_value[0]) == 'w')
		    {
			scb->scb_sscb.sscb_ics.ics_mathex= (i4)ADX_WRN_MATHEX;
		    }
		    else if (CHCAST_MACRO(&flag->gca_p_value.gca_value[0]) == 'f')
		    {
			scb->scb_sscb.sscb_ics.ics_mathex= (i4)ADX_ERR_MATHEX;
		    }
		    else if (CHCAST_MACRO(&flag->gca_p_value.gca_value[0]) == 'i')
#else
					flag->gca_p_value.gca_value[0];
		    }
		    if (flag->gca_p_value.gca_value[0] == 'w')
		    {
			scb->scb_sscb.sscb_ics.ics_mathex= (i4)ADX_WRN_MATHEX;
		    }
		    else if (flag->gca_p_value.gca_value[0] == 'f')
		    {
			scb->scb_sscb.sscb_ics.ics_mathex= (i4)ADX_ERR_MATHEX;
		    }
		    else if (flag->gca_p_value.gca_value[0] == 'i')
#endif
		    {
			scb->scb_sscb.sscb_ics.ics_mathex= (i4)ADX_IGN_MATHEX;
		    }
		    else
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    break;

		case    GCA_CWIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			    scb->scb_sscb.sscb_ics.ics_outarg.ad_c0width);    
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co4_cwidth =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_c0width;
		    }
		    break;

		case    GCA_TWIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			    scb->scb_sscb.sscb_ics.ics_outarg.ad_t0width);    
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co5_twidth =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_t0width;
		    }
		    break;

		case    GCA_I1WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			    scb->scb_sscb.sscb_ics.ics_outarg.ad_i1width);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co6_i1width =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_i1width;
		    }
		    break;

		case    GCA_I2WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_i2width);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co7_i2width =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_i2width;
		    }
		    break;

		case    GCA_I4WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_i4width);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co8_i4width =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_i4width;
		    }
		    break;

		case    GCA_I8WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_i8width);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co41_i8width =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_i8width;
		    }
		    break;

		case    GCA_F4WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_f4width);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co9_f4width =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_f4width;
		    }
		    break;

		case    GCA_F8WIDTH:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_f8width);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co11_f8width =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_f8width;
		    }
		    break;

		case    GCA_F4STYLE:
		    scb->scb_sscb.sscb_ics.ics_outarg.ad_f4style = 
#ifdef axp_osf
			CHCAST_MACRO(&flag->gca_p_value.gca_value[0]);
#else
			flag->gca_p_value.gca_value[0];
#endif
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			MEcopy(flag->gca_p_value.gca_value,
				sizeof(star_comflags.dd_co24_f4style),
				star_comflags.dd_co24_f4style);
		    }
		    break;

		case    GCA_F8STYLE:
		    scb->scb_sscb.sscb_ics.ics_outarg.ad_f8style =    
#ifdef axp_osf
			CHCAST_MACRO(&flag->gca_p_value.gca_value[0]);
#else
			flag->gca_p_value.gca_value[0];
#endif
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			MEcopy(flag->gca_p_value.gca_value,
				sizeof(star_comflags.dd_co25_f8style),
				star_comflags.dd_co25_f8style);
		    }
		    break;

		case    GCA_F4PRECISION:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_f4prec);    
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co10_f4precision =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_f4prec;
		    }
		    break;

		case    GCA_F8PRECISION:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0],
			scb->scb_sscb.sscb_ics.ics_outarg.ad_f8prec);    
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co12_f8precision =
			      (i4)scb->scb_sscb.sscb_ics.ics_outarg.ad_f8prec;
		    }
		    break;

		case    GCA_APPLICATION:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);

		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
			star_comflags.dd_co2_application = (i4)flag_value;

		    scb->scb_sscb.sscb_ics.ics_appl_code = (i4) flag_value;

		    switch (flag_value)
		    {
		    case DBA_SYSMOD:
			scb->scb_sscb.sscb_ics.ics_requstat |= DU_UUPSYSCAT;
			want_updsyscat=TRUE;
			break;

		    case DBA_PRETEND_VFDB:
			scb->scb_sscb.sscb_ics.ics_dmf_flags |= DMC_PRETEND;
			break;
		    case DBA_FORCE_VFDB:
			scb->scb_sscb.sscb_ics.ics_dmf_flags |= 
						    DMC_MAKE_CONSISTENT;
			break;
		    case DBA_CONVERT:
			scb->scb_sscb.sscb_ics.ics_dmf_flags |= DMC_CVCFG;
			break;
		    case DBA_DESTROYDB:
			break;
		    case DBA_CREATE_DBDBF1:
			scb->scb_sscb.sscb_ics.ics_requstat |= DU_UALL_PRIVS;
			break;
		    case DBA_VERIFYDB:
			/* This flag was used by optimizedb before 6.5.
			** It is now obsolete. Return error, to prevent old
			** versions of optimizedb from connecting.
			*/
			status = E_DB_ERROR;
			error.err_code = E_US0004_CANNOT_ACCESS_DB;
			break;
		    /*
		    ** Allow for server side checking of the OR license.
		    */
		    case DBA_W4GLRUN_LIC_CHK:
		    case DBA_W4GLDEV_LIC_CHK:
		    case DBA_W4GLTR_LIC_CHK:
			break;
		    default:
			if ((scb->scb_sscb.sscb_ics.ics_appl_code <
						    DBA_MIN_APPLICATION)
			    || (scb->scb_sscb.sscb_ics.ics_appl_code >
						    DBA_MAX_APPLICATION))
			{
			    status = E_DB_ERROR;
			    error.err_code = E_US0022_FLAG_FORMAT;
			}
			break;
		    }
		    
		    break;

		case    GCA_QLANGUAGE:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_qlang = (DB_LANG) flag_value;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co0_active_flags--;
			star_comflags.dd_co3_qlanguage = (i4)flag_value;
		    }

		    if (scb->scb_sscb.sscb_ics.ics_qlang != DB_QUEL
		      && scb->scb_sscb.sscb_ics.ics_qlang != DB_SQL)
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
		    }
		    break;

		case    GCA_DBNAME:
		    /* Fold database name to lower case */
		    MEmove(l_value,
			flag->gca_p_value.gca_value,
			' ', sizeof(dbstr)-1, dbstr);
		    dbstr[sizeof(dbstr)-1] = '\0';
		    CVlower(dbstr);

		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			char *ptr;

			star_comflags.dd_co0_active_flags--;
			/* For Star, strip off the "/d" or "/star" */
			ptr = STrindex(dbstr, "/", l_value);
			if (ptr != NULL)
			{
			    MEfill(&dbstr[sizeof(dbstr)-1] - ptr,
				   ' ', ptr);
			}
		    }
		    MEmove(sizeof(dbstr)-1, dbstr, ' ',
			sizeof(scb->scb_sscb.sscb_ics.ics_dbname),
			(PTR)&scb->scb_sscb.sscb_ics.ics_dbname);
		    break;

		case    GCA_NLANGUAGE:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_language = (i4) flag_value;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co13_nlanguage = (i4)flag_value;
		    }
		    break;
	    
		case    GCA_MPREC:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_prec =
			    (i4) flag_value;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co14_mprec = (i4)flag_value;
		    }
		    break;
			
		case    GCA_MLORT:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_lort =
			    (i4) flag_value;
		    if (flag_value == DB_NONE_MONY)
			scb->scb_sscb.sscb_ics.ics_money_format.
						db_mny_sym[0] = EOS; 
		    	
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co15_mlort = (i4)flag_value;
		        if (flag_value == DB_NONE_MONY)
			    star_comflags.dd_co26_msign[0] = EOS;
		    }
		    break;

		case GCA_SL_TYPE:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co0_active_flags--;
		    }
		    break;

		case    GCA_DATE_FORMAT:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_date_format = (DB_DATE_FMT) flag_value;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co16_date_frmt = (i4)flag_value;
		    }
		    break;
		    
#ifdef GCA_TIMEZONE
		case    GCA_TIMEZONE:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if(flag_value > 0)
			STprintf(typestr, "GMT-%dM", flag_value);
		    else if(flag_value < 0)
			STprintf(typestr, "GMT%dM", abs(flag_value));
		    else
			STcopy("GMT", typestr);
		    MEcopy(typestr, STlength(typestr)+1,
			   scb->scb_sscb.sscb_ics.ics_tz_name);
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co31_timezone = (i4)flag_value;
			MEcopy(typestr, STlength(typestr)+1,
			       star_comflags.dd_co34_tz_name);
		    }
		    timezone_recd = TRUE;
		    break;

		case    GCA_TIMEZONE_NAME:
		{
		    i4  tmp_value;
			
		    /* ics_tz_name is size DB_TYPE_MAXLEN */
		    if(l_value < DB_TYPE_MAXLEN)
			tmp_value = l_value;
		    else
			tmp_value = DB_TYPE_MAXLEN-1;
		    MEcopy(flag->gca_p_value.gca_value, tmp_value,
			   scb->scb_sscb.sscb_ics.ics_tz_name);
		    scb->scb_sscb.sscb_ics.ics_tz_name[tmp_value] = EOS;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			MEcopy(flag->gca_p_value.gca_value, 
			       tmp_value,
			       star_comflags.dd_co34_tz_name);
			star_comflags.dd_co34_tz_name[tmp_value] = EOS;
		    }
		    timezone_recd = TRUE;
		    break;
		}
#endif

		case	GCA_YEAR_CUTOFF:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    scb->scb_sscb.sscb_ics.ics_year_cutoff = (i4)flag_value;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co39_year_cutoff = (i4)flag_value;
		    }
		    year_cutoff_recd = TRUE;
		    break;

		case    GCA_MSIGN:
		    MEcopy(flag->gca_p_value.gca_value,
			    l_value,
			    scb->scb_sscb.sscb_ics.ics_money_format.db_mny_sym);
			    scb->scb_sscb.sscb_ics.ics_money_format.
						    db_mny_sym[l_value] = '\0'; 
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			MEcopy(flag->gca_p_value.gca_value,
				l_value,
				star_comflags.dd_co26_msign);
			star_comflags.dd_co26_msign[l_value] = '\0';
		    }
		    break;

		case    GCA_DECIMAL:
		    /*
		    ** Even though db_decimal is a 'nat', gca transmits it
		    ** as a single byte character string, so we need to
		    ** convert it back from a character to a nat.
		    */

		    scb->scb_sscb.sscb_ics.ics_decimal.db_decimal =
			I1_CHECK_MACRO(*(i1 *)&flag->gca_p_value.gca_value[0]);
		    scb->scb_sscb.sscb_ics.ics_decimal.db_decspec = TRUE;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			/* FIXME -- check removal of cast (daveb) */
			star_comflags.dd_co27_decimal = 
			  I1_CHECK_MACRO( flag->gca_p_value.gca_value[0] );
		    }
		    break;

		case    GCA_SVTYPE:
		    I4ASSIGN_MACRO(flag->gca_p_value.gca_value[0], flag_value);
		    if (flag_value == GCA_MONITOR)
		    {
			scb->scb_sscb.sscb_stype = SCS_SMONITOR;
			/*
			** Adjust session type counts. It's not until now
			** that we know that this is a monitor session;
			** scdinit counted it as a "normal" session,
			** so bump monitor session stats now.
			*/
			CSp_semaphore( TRUE, &Sc_main_cb->sc_misc_semaphore );
			Sc_main_cb->sc_session_count[SCS_SMONITOR].created++;
			if (++Sc_main_cb->sc_session_count[SCS_SMONITOR].current >
			    Sc_main_cb->sc_session_count[SCS_SMONITOR].hwm)
			{
			    Sc_main_cb->sc_session_count[SCS_SMONITOR].hwm = 
			    Sc_main_cb->sc_session_count[SCS_SMONITOR].current;
			}
			CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
		    }
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co0_active_flags--;
		    }
		    break;

		case    GCA_RESTART:
		case    GCA_XA_RESTART:
		{
		    char        *xid_str;
		    char	txid[50];
		    char 	*index;
		    char        c;
		    i4	        length;

	       /* Parse the distributed transaction ID from the gca_value. */

		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }

		    xid_str = (char *)(flag->gca_p_value.gca_value);

		    /* Now switch between the Ingres Dist XID and XA XID */
		    if (p_index == GCA_RESTART)
		    {
  
		       /* Ingres Dist transaction id */
		       scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id_type =
				DB_INGRES_DIS_TRAN_ID_TYPE;
		       
		       if ((index = STindex(xid_str,
					    ":", 
					    l_value)) == NULL)
		       {
			   status = E_DB_ERROR;
			   error.err_code = E_US0009_BAD_PARAM;
			   break;
		       }

		       c = *index;
		       *index = EOS;

		       length = (i4)(index - xid_str);

		       CVal(xid_str,
			(int*)&scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.
				db_ingres_dis_tran_id.db_tran_id.db_high_tran);

		       *index = c;

		       MEcopy(index+1, l_value-length-1,txid);
		       txid[ l_value - length - 1 ] = EOS;

		       CVal(txid,
			(int*)&scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.
				db_ingres_dis_tran_id.db_tran_id.db_low_tran);

		     }
		     else
		     {

			scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id_type =
				DB_XA_DIS_TRAN_ID_TYPE;

			status = conv_to_struct_xa_xid( xid_str,
			   l_value,
			   &scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.
			      db_xa_extd_dis_tran_id );
			if (DB_FAILURE_MACRO( status ))
			{
			   status = E_DB_ERROR;
			   error.err_code = E_US0009_BAD_PARAM;
			   break;
			}
		     }

		    scb->scb_sscb.sscb_ics.ics_dmf_flags |= DMC_NLK_SESS;
		    break;
		}	
			
		case    GCA_GRPID:

		    /* Determine the length of the group identifier */

		    l_id = l_value;
		    id_loc = (char *)flag->gca_p_value.gca_value;

		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			MEmove( l_id, id_loc, '\0',
				sizeof(star_comflags.dd_co36_grpid),
				star_comflags.dd_co36_grpid );
		    }

		    if ((*id_loc == '"') ||
			(*id_loc == '\''))
		    {
			/*
			** The group is delimited or quoted;
			** let the translation routine figure out the length
			*/
		    }
		    else
		    {
			/*
			** The group is not delimited or quoted;
			** figure out the length here
			*/
			pass_loc = (char *)STindex((char *)id_loc,
						   (char *)"/", (u_i4)l_id);
			if (pass_loc != NULL)
			    l_id = (u_i2)(pass_loc - id_loc);
		    }

		    /* Move the group id into the SCS_ICS */

		    MEfill(sizeof(scb->scb_sscb.sscb_ics.ics_xegrpid),
			(u_char)' ', 
			(PTR)&scb->scb_sscb.sscb_ics.ics_xegrpid);
		    status = cui_idxlate((u_char *)id_loc, &l_id,
			(u_char *)&scb->scb_sscb.sscb_ics.ics_xegrpid,
			(u_i4 *)NULL, (u_i4)CUI_ID_DLM_M,
			&idmode, &error);
		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_uput(error.err_code, 0, 2,
				  sizeof(ERx("Group identifier"))-1,
				  (PTR) ERx("Group identifier"),
				  l_id,
				  (PTR)&scb->scb_sscb.sscb_ics.ics_xegrpid,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }	
		    if (idmode & CUI_ID_QUO)
		    {
			/* Quoted groups not allowed */
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }	
		    if (idmode & CUI_ID_DLM)
			scb->scb_sscb.sscb_ics.ics_xmap |= SCS_XDLM_EGRPID;
		    else
			scb->scb_sscb.sscb_ics.ics_xmap &= ~SCS_XDLM_EGRPID;

		    MEcopy((PTR) &scb->scb_sscb.sscb_ics.ics_xegrpid,
			   sizeof(scb->scb_sscb.sscb_ics.ics_egrpid),
			   (PTR) &scb->scb_sscb.sscb_ics.ics_egrpid);

		    /* Determine if a password was specified */

		    pass_loc = &id_loc[l_id];
		    l_pass = l_value - l_id - 1;
		    if ((l_pass > 0) &&
			(*pass_loc++ == '/'))
		    {
			/* Move the password into the SCS_ICS */
			MEmove(l_pass, (PTR)pass_loc, ' ',
			    sizeof(scb->scb_sscb.sscb_ics.ics_egpass),
			    (PTR)&scb->scb_sscb.sscb_ics.ics_egpass);
		    }

		    /* Encrypt the password if nonblank */

		    if (STskipblank((PTR)&scb->scb_sscb.sscb_ics.ics_egpass,
			      sizeof(scb->scb_sscb.sscb_ics.ics_egpass))
			    != NULL)
		    {
			scf_cb.scf_length = sizeof(scf_cb);
			scf_cb.scf_type   = SCF_CB_TYPE;
			scf_cb.scf_facility = DB_SCF_ID;
			scf_cb.scf_session  = DB_NOSESSION;
			scf_cb.scf_nbr_union.scf_xpasskey =
			    (PTR)scb->scb_sscb.sscb_ics.ics_egrpid.db_own_name;
			scf_cb.scf_ptr_union.scf_xpassword =
			    (PTR)scb->scb_sscb.sscb_ics.ics_egpass.db_password;
			scf_cb.scf_len_union.scf_xpwdlen =
			    sizeof(scb->scb_sscb.sscb_ics.ics_egpass);
			status = scf_call(SCU_XENCODE,&scf_cb);
			if (status)
			{
			    status = (status > E_DB_ERROR) ?
				      status : E_DB_ERROR;
			    error.err_code = E_SC0260_XENCODE_ERROR;
			    break;
			}
		    }
			
		    break;

		case    GCA_APLID:

		    /* Determine the length of the role identifier */
		    
		    l_id = l_value;
		    id_loc = (char *)flag->gca_p_value.gca_value;

		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			MEmove( l_id, id_loc, '\0',
				sizeof(star_comflags.dd_co37_aplid),
				star_comflags.dd_co37_aplid );
		    }

		    if (Sc_main_cb->sc_secflags&SC_SEC_ROLE_NONE)
		    {
			/*
			** No roles in this installation
			*/
			sc0e_put(E_US18E7_6375, 0, 1,
				 sizeof(ERx("Role identifier"))-1,
				 ERx("Role identifier"),
					 0, (PTR)0,
					 0, (PTR)0,
					 0, (PTR)0,
					 0, (PTR)0,
					 0, (PTR)0);
			status=E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    /* Determine the length of the role identifier */
		    
		    l_id = l_value;
		    id_loc = (char *)flag->gca_p_value.gca_value;		   
		    if ((*id_loc == '"') ||
			(*id_loc == '\''))
		    {
			/*
			** The role is delimited or quoted;
			** let the translation routine figure out the length
			*/
		    }
		    else
		    {
			/*
			** The role is not delimited or quoted;
			** figure out the length here
			*/
			pass_loc = (char *)STindex((char *)id_loc,
						   (char *)"/", (u_i4)l_id);
			if (pass_loc != NULL)
			    l_id = (u_i2)(pass_loc - id_loc);
		    }

		    /* Move the application_id into the SCS_ICS */

		    MEfill(sizeof(scb->scb_sscb.sscb_ics.ics_xeaplid),
			(u_char)' ', 
			(PTR)&scb->scb_sscb.sscb_ics.ics_xeaplid);
		    status = cui_idxlate((u_char *)id_loc, &l_id,
			(u_char *)&scb->scb_sscb.sscb_ics.ics_xeaplid,
			(u_i4 *)NULL, (u_i4)CUI_ID_DLM_M,
			&idmode, &error);
		    if (DB_FAILURE_MACRO(status))
		    {
			sc0e_put(error.err_code, 0, 2,
				 sizeof(ERx("Role identifier"))-1,
				 (PTR)ERx("Role identifier"),
				 l_id,
				  (PTR)&scb->scb_sscb.sscb_ics.ics_xeaplid,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }	
		    if (idmode & CUI_ID_QUO)
		    {
			/* Quoted roles not allowed */
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }	
		    if (idmode & CUI_ID_DLM)
			scb->scb_sscb.sscb_ics.ics_xmap |= SCS_XDLM_EAPLID;
		    else
			scb->scb_sscb.sscb_ics.ics_xmap &= ~SCS_XDLM_EAPLID;

		    MEcopy((PTR) &scb->scb_sscb.sscb_ics.ics_xeaplid,
			   sizeof(scb->scb_sscb.sscb_ics.ics_eaplid),
			   (PTR) &scb->scb_sscb.sscb_ics.ics_eaplid);

		    /* Determine if a password was specified */

		    pass_loc = &id_loc[l_id];
		    l_pass = l_value - l_id - 1;
		    if ((l_pass > 0) &&
			(*pass_loc++ == '/'))
		    {
			/* Move the password in the SCS_ICS */
			MEmove(l_pass, (PTR)pass_loc, ' ',
			    sizeof(scb->scb_sscb.sscb_ics.ics_eapass),
			    (PTR)&scb->scb_sscb.sscb_ics.ics_eapass);
		    }


		    break;

#ifdef	GCA_RUPASS
		case    GCA_RUPASS:

		    if (Sc_main_cb->sc_secflags&SC_SEC_PASSWORD_NONE)
		    {
			/*
			** No passwords in this installation
			*/
			sc0e_put(E_US18E7_6375, 0, 1,
				 sizeof(ERx("User Password"))-1,
				 ERx("User Password"),
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);
			status=E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    l_id = l_value;
		    id_loc = (char *)flag->gca_p_value.gca_value;
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			MEmove( l_id, id_loc, '\0',
				sizeof(star_comflags.dd_co40_usrpwd),
				star_comflags.dd_co40_usrpwd );
		    }
		    
		    MEmove(l_value,
			flag->gca_p_value.gca_value,
			' ',
			sizeof(scb->scb_sscb.sscb_ics.ics_rupass),
			(PTR)&scb->scb_sscb.sscb_ics.ics_rupass);


		    break;
#endif	/* GCA_RUPASS*/
			
		case    GCA_DECFLOAT:
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co38_decformat[0] = 
#ifdef axp_osf
				CHCAST_MACRO(&flag->gca_p_value.gca_value[0]);
		    }
		    
		    if (CHCAST_MACRO(&flag->gca_p_value.gca_value[0]) == 'f')
		    {
			scb->scb_sscb.sscb_ics.ics_parser_compat |=
							    PSQ_FLOAT_LITS;
		    }
		    else if (CHCAST_MACRO(&flag->gca_p_value.gca_value[0]) == 'd')
#else 
					flag->gca_p_value.gca_value[0];
		    }
		    
		    if (flag->gca_p_value.gca_value[0] == 'f')
		    {
			scb->scb_sscb.sscb_ics.ics_parser_compat |=
							    PSQ_FLOAT_LITS;
		    }
		    else if (flag->gca_p_value.gca_value[0] == 'd')
#endif
		    {
			scb->scb_sscb.sscb_ics.ics_parser_compat &=
							    ~PSQ_FLOAT_LITS;
		    }
		    else
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    break;

		case GCA_GW_PARM:
		    /* Always save this, EOS terminated.  This way
		       we can see the passed in value, either through
		       MO or via iimonitor.  We may pass in client
		       host, client pid, etc. this way.  (daveb) */

		    status = sc0m_allocate(0,
				(i4)(l_value + 1 + sizeof(SC0M_OBJECT)),
				DB_SCF_ID,
				(PTR) SCS_MEM,
				CV_C_CONST_MACRO('g', 'w', 'p', 'm'),
				&scb->scb_sscb.sscb_ics.ics_alloc_gw_parms);
		    if (status)
		    {
			sc0e_0_put(E_SC0204_MEMORY_ALLOC, 0);
			sc0e_0_put(status, 0);
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    scb->scb_sscb.sscb_ics.ics_gw_parms =
			(PTR)((char *)scb->scb_sscb.sscb_ics.ics_alloc_gw_parms +
			    sizeof(SC0M_OBJECT));

		    MEmove(l_value,
			    flag->gca_p_value.gca_value,
			    ' ',
			    l_value,
			    scb->scb_sscb.sscb_ics.ics_gw_parms
			  );
		    scb->scb_sscb.sscb_ics.ics_gw_parms[l_value] = EOS;

		    /* fill star block in only if star session */

		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co32_gw_parms =
			    scb->scb_sscb.sscb_ics.ics_gw_parms;
			star_comflags.dd_co33_len_gw_parms = l_value;
		    }
		    break;

		case    GCA_STRTRUNC:
		    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    {
			star_comflags.dd_co35_strtrunc[0] = 
#ifdef axp_osf
					CHCAST_MACRO(&flag->gca_p_value.gca_value[0]);
		    }
		    if (CHCAST_MACRO(&flag->gca_p_value.gca_value[0]) == 'f')
		    {
			scb->scb_sscb.sscb_ics.ics_strtrunc = 
			    (i4)ADF_ERR_STRTRUNC;
		    }
		    else if (CHCAST_MACRO(&flag->gca_p_value.gca_value[0]) == 'i')
#else
					flag->gca_p_value.gca_value[0];
		    }
		    if (flag->gca_p_value.gca_value[0] == 'f')
		    {
			scb->scb_sscb.sscb_ics.ics_strtrunc = 
			    (i4)ADF_ERR_STRTRUNC;
		    }
		    else if (flag->gca_p_value.gca_value[0] == 'i')
#endif
		    {
			scb->scb_sscb.sscb_ics.ics_strtrunc = 
			    (i4)ADF_IGN_STRTRUNC;
		    }
		    else
		    {
			status = E_DB_ERROR;
			error.err_code = E_US0022_FLAG_FORMAT;
			break;
		    }
		    break;

                case    GCA_DATE_ALIAS:
                {
                    i4  tmp_value;
                    char  tmp_chars[DB_TYPE_MAXLEN];

		    /* dd_co42_date_alias is size DB_TYPE_MAXLEN */
                    if(l_value < DB_TYPE_MAXLEN)
                        tmp_value = l_value;
                    else
                        tmp_value = DB_TYPE_MAXLEN-1;

                    MEcopy(flag->gca_p_value.gca_value, tmp_value, tmp_chars);
                    tmp_chars[tmp_value] = EOS;

                    if (STncasecmp(tmp_chars, "ansidate", 8) == 0)
                      scb->scb_sscb.sscb_ics.ics_date_alias =
                                        AD_DATE_TYPE_ALIAS_ANSI;
                    else
                      scb->scb_sscb.sscb_ics.ics_date_alias =
                                        AD_DATE_TYPE_ALIAS_INGRES;

                    if (scb->scb_sscb.sscb_flags & SCS_STAR)
                    {
                        MEcopy(flag->gca_p_value.gca_value,
                               tmp_value,
                               star_comflags.dd_co42_date_alias);
                        star_comflags.dd_co42_date_alias[tmp_value] = EOS;
                    }
                    date_alias_recd = TRUE;
                    break;
                }

		default:									  
		    status = E_DB_ERROR;
		    error.err_code = E_US0022_FLAG_FORMAT;
		    break;
	    }

	    if (i < crb->gca_l_user_data && !status)
	    {
		flag = (GCA_USER_DATA *)(flag->gca_p_value.gca_value + l_value);
	    }
	}

	/*
	** Set the Ingres system privileges for the session user.
	*/
	if (scs_chk_priv(scb->scb_sscb.sscb_ics.ics_susername.db_own_name,
			 "SERVER_CONTROL"))
	    scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_SVR_CONTROL;
	if (scs_chk_priv(scb->scb_sscb.sscb_ics.ics_susername.db_own_name,
			 "NET_ADMIN"))
	    scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_NET_ADMIN;
	if (scs_chk_priv(scb->scb_sscb.sscb_ics.ics_susername.db_own_name,
			 "MONITOR"))
	    scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_MONITOR;
	if (scs_chk_priv(scb->scb_sscb.sscb_ics.ics_susername.db_own_name,
			 "TRUSTED"))
	    scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_TRUSTED;

    }

    if (scb->scb_sscb.sscb_flags & SCS_STAR)
	star_comflags.dd_co0_active_flags += i;

    if (scb->scb_sscb.sscb_tptr)
    {
	sc0m_deallocate(0, &scb->scb_sscb.sscb_tptr);
	scb->scb_sscb.sscb_troot = NULL;
	scb->scb_sscb.sscb_tnleft = 0;
	scb->scb_sscb.sscb_tsize = 0;
    }

    if (scb->scb_sscb.sscb_stype == SCS_SNORMAL)
    {
	/* normal sessions not valid for recovery or iomaster servers */
	if (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER)
	{
	    status = E_DB_ERROR;
	    error.err_code = E_SC0327_RECOVERY_SERVER_NOCNCT;
	}
	else
	if (Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER)
	{
	    status = E_DB_ERROR;
	    error.err_code = E_SC0370_IOMASTER_SERVER_NOCNCT;
	}
    }


    for (;status == 0;)
    {
	/*
	** Prime the expedited GCA channel to handle attn interrupts.
	** Do this early in case we hang waiting on some resource
	** (e.g., fetching iidbdb info) and the user opts to break
	** out of the front end.
	*/
	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID))
	    scc_iprime(scb);

	/*
	** The facilities needed for this session have been 
	** determined by scd_alloc_cbs() and can be found in
	** sscb_need_facilities.
	*/


	if (scb->scb_sscb.sscb_flags & SCS_STAR)
	{
	    /*
	    ** If the thread is a STAR thread and the 2PC thread is
	    ** still alive and the incoming thread is not the monitor tread
	    ** stall the thread until TPF compiled the all the info it needs
	    ** from the DDB's for server recovery.
	    */
	    if (scb->scb_sscb.sscb_stype == SCS_SNORMAL)
	    {
		for (;
			(Sc_main_cb->sc_state == SC_2PC_RECOVER)
				||
			(Sc_main_cb->sc_state == SC_INIT)
		    ;)
		{
		    MEcopy("Waiting for DX Recovery to complete", 35,
					scb->scb_sscb.sscb_ics.ics_act1);
		    scb->scb_sscb.sscb_ics.ics_l_act1 = 35;
		    status = CSsuspend(CS_TIMEOUT_MASK, 1, 0);
		    if ( !((status == E_DB_OK) || (status == E_CS0009_TIMEOUT)) )
		    {
			status = E_DB_ERROR;
			break;
		    }
		    status = E_DB_OK;
		}
		scb->scb_sscb.sscb_ics.ics_l_act1 = 0;
		if (status)
		{
		    error.err_code = E_SC0123_SESSION_INITIATE;
		    break;
		}
	    }
	}

	if ((   (scb->scb_sscb.sscb_stype == SCS_SNORMAL)
	     ||	(scb->scb_sscb.sscb_stype == SCS_SMONITOR))
	    &&
	    (!(scb->scb_sscb.sscb_flags & SCS_STAR)))
	{
	    /* Then this is a user session.  See if we are ready yet. If    */
	    /* not, wait, then release...				    */
	    while (Sc_main_cb->sc_state == SC_INIT)
		(VOID) CSsuspend(CS_TIMEOUT_MASK, 1, 0);
	}

	if ((scb->scb_sscb.sscb_stype == SCS_SMONITOR) &&
	    ((STbcompare( DB_NDINGRES_NAME, sizeof(DB_NDINGRES_NAME)-1,
			 (char *)scb->scb_sscb.sscb_ics.ics_eusername,
			 sizeof(*scb->scb_sscb.sscb_ics.ics_eusername),
			 TRUE) == 0) ||
	     scs_chk_priv( (char *)scb->scb_sscb.sscb_ics.ics_eusername,
			  "SERVER_CONTROL" )))
	{
	    /*
	    ** DO NOT REMOVE,COMMENT OUT, or OTHERWISE OBLITERATE THIS CODE.
	    ** IT IS IMPORTANT TO BE ABLE TO IIMONITOR THINGS WHEN THE SYSTEM
	    ** IS `HUNG', AND THAT'S WHAT THIS DOES.  DO NOT REMOVE IT.
	    **
	    ** IF THERE ARE BUGS, OR WHATEVER, EITHER FIX THEM OR TALK
	    ** TO FRED.  I HAVE PUT THIS STUFF IN AT LEAST 10 TIMES, AND
	    ** HAD IT REMOVED EACH AND EVERY TIME, WITH NO COMMENTS
	    ** ABOUT WHY IT WAS DONE.  REMOVAL OF THIS CAPABILITY PUTS A
	    ** SEVERE CRIMP IN OUR ABILITY TO DEBUG THE SYSTEM.
	    **
	    ** PLEASE DO NOT REMOVE THIS CODE.
	    */

	    scb->scb_sscb.sscb_state = SCS_INPUT;
	    status = 0;         /* Just in case iidbdb didn't exist */
	    error.err_code = 0;
	    scb->scb_sscb.sscb_ics.ics_rustat = (DU_UALL_PRIVS);
	    scb->scb_sscb.sscb_ics.ics_maxustat = (DU_UALL_PRIVS);
	}

	/*
	** The facilities' memory has already been allocated by
	** scd_alloc_scb and the pointers to the various slots
	** established in the sscb.
	**
	** Note that only "need_facilities" cb's and work areas
	** have been allocated, so look before you leap!
	*/

	/*
	** If an internal thread then give it all privileges it needs
	** Internal threads are not normal nor monitor and may not have
	** frontends attached to them
	*/
	if (scb->scb_sscb.sscb_flags & SCS_INTERNAL_THREAD &&
    	   (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID)) == 0)
	{
	    scb->scb_sscb.sscb_ics.ics_rustat = (DU_UALL_PRIVS);
	    scb->scb_sscb.sscb_ics.ics_iustat = (DU_UALL_PRIVS);
	}

	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_ADF_ID))
	{
	    adf_cb = scb->scb_sscb.sscb_adscb;  /* note scb NOT ccb */
	    adf_cb->adf_slang = scb->scb_sscb.sscb_ics.ics_language;
	    adf_cb->adf_nullstr.nlst_length = 0;
	    adf_cb->adf_nullstr.nlst_string = 0;
	    adf_cb->adf_qlang = scb->scb_sscb.sscb_ics.ics_qlang;
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_date_format, adf_cb->adf_dfmt);
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_money_format, adf_cb->adf_mfmt);
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_decimal, adf_cb->adf_decimal);
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_date_alias, adf_cb->adf_date_type_alias);

	    adf_cb->adf_exmathopt =
		    (ADX_MATHEX_OPT) scb->scb_sscb.sscb_ics.ics_mathex;
	    adf_cb->adf_strtrunc_opt =
		    (ADF_STRTRUNC_OPT) scb->scb_sscb.sscb_ics.ics_strtrunc;
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_outarg, adf_cb->adf_outarg);
	    adf_cb->adf_errcb.ad_ebuflen = DB_ERR_SIZE;
	    adf_cb->adf_errcb.ad_errmsgp = (char *) adf_cb + sizeof(ADF_CB);
	    adf_cb->adf_errcb.ad_emsglen = 0;
	    adf_cb->adf_unisub_status = AD_UNISUB_OFF;
	    adf_cb->adf_unisub_char[0] = '\0';
	    adf_cb->adf_autohash = NULL;
	    if (!(scb->scb_sscb.sscb_flags & SCS_STAR))
	    {
	    	adf_cb->adf_maxstring = DB_MAXSTRING;
	    }
	    else
	    {
	    	adf_cb->adf_maxstring = DB_GW4_MAXSTRING;
	    }	
	    /* Add 65 datatype support */
	    adf_cb->adf_proto_level = 0;
	    adf_cb->adf_utf8_flag = 0;
	    /* There's no scb_cscb if no GCA! */
	    if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID))
	    {
	    	if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_60)
		{
		    adf_cb->adf_proto_level |= (AD_DECIMAL_PROTO | AD_BYTE_PROTO); 
		}

	    	if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_65)
		{
		    adf_cb->adf_proto_level |= AD_I8_PROTO; 
		}

	    	if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_66)
		{
		    adf_cb->adf_proto_level |= AD_ANSIDATE_PROTO; 
		}

                if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_67)
                {
                    adf_cb->adf_max_decprec = CL_MAX_DECPREC_39;
                }
                else
                {
                    adf_cb->adf_max_decprec = CL_MAX_DECPREC_31;
                }
                if (scb->scb_cscb.cscb_version >= GCA_PROTOCOL_LEVEL_68)
                    adf_cb->adf_proto_level |= AD_BOOLEAN_PROTO;
	    }

	    if (date_alias_recd == FALSE)
            {
                /* Default value of date_alias is 'ingresdate'. eg:
                ** If Client is older than 2006r2 then date_alias is automatically set
                ** to ingresdate.
                ** If Client is of 2006r2 level and date_alias is either not set or set
                ** to ingersdate then date_alias is set to ingresdate.
                ** else for newer clients, if date_alias is not provided in the
                ** config.dat file (unusual scenario) it is always set to ingresdate.
                */
                if ((scb->scb_cscb.cscb_version == GCA_PROTOCOL_LEVEL_66) &&
                        (Sc_main_cb->sc_date_type_alias == SC_DATE_ANSIDATE))
                {
                  scb->scb_sscb.sscb_ics.ics_date_alias = AD_DATE_TYPE_ALIAS_ANSI;
                  adf_cb->adf_date_type_alias = AD_DATE_TYPE_ALIAS_ANSI;
                }
                else
                {
                  scb->scb_sscb.sscb_ics.ics_date_alias = AD_DATE_TYPE_ALIAS_INGRES;
                  adf_cb->adf_date_type_alias = AD_DATE_TYPE_ALIAS_INGRES;
                }
            }

	    /* If front-end didn't send time zone, use our own time zone */
	    if (timezone_recd == FALSE)
		status = TMtz_init(&adf_cb->adf_tzcb);
	    else
	    {
		/* Search for correct timezone name */
		status = TMtz_lookup( scb->scb_sscb.sscb_ics.ics_tz_name,
				      &adf_cb->adf_tzcb);
		if( status == TM_TZLKUP_FAIL)
		{
		    /* Load new table */
		    CSp_semaphore(0, &Sc_main_cb->sc_tz_semaphore);
		    status = TMtz_load(scb->scb_sscb.sscb_ics.ics_tz_name,
				       &adf_cb->adf_tzcb);
		    CSv_semaphore(&Sc_main_cb->sc_tz_semaphore);
		}
	    }

	    if(status != OK)
	    {
		error.err_code = status;
		status = E_DB_ERROR;
		sc0e_uput(E_SC033D_TZNAME_INVALID, 0, 3,
			(i4)STlength(scb->scb_sscb.sscb_ics.ics_tz_name),
			(PTR)scb->scb_sscb.sscb_ics.ics_tz_name,
			(i4)STlength(SystemVarPrefix), SystemVarPrefix,
			(i4)STlength(SystemVarPrefix), SystemVarPrefix,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0);		
		break;
	    }
	   
	    if (year_cutoff_recd == FALSE)
	    {
		status = TMtz_year_cutoff(&adf_cb->adf_year_cutoff);
	    }
	    else
	    {
		if ((scb->scb_sscb.sscb_ics.ics_year_cutoff 
			<= TM_DEF_YEAR_CUTOFF) ||
		    (scb->scb_sscb.sscb_ics.ics_year_cutoff 
			> TM_MAX_YEAR_CUTOFF))
		{
		    status = TM_YEARCUT_BAD;
		}
		else
		{
		    adf_cb->adf_year_cutoff = 
			scb->scb_sscb.sscb_ics.ics_year_cutoff;
		}
	    }

	    if (status != OK)
	    {
		error.err_code = status;
		status = E_DB_ERROR;
		sc0e_uput(E_SC037B_BAD_DATE_CUTOFF, 0, 2,
				  (i4)STlength(SystemVarPrefix),
				  SystemVarPrefix,
				  0, 
				  (PTR)(SCALARP)scb->scb_sscb.sscb_ics.ics_year_cutoff,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0,
				  0, (PTR)0);		
		break;
	    }

	    adf_cb->adf_srv_cb = Sc_main_cb->sc_advcb;
	    status = adg_init(adf_cb);
	    if (status)
	    {
		error.err_code = adf_cb->adf_errcb.ad_errcode;
		if (error.err_code == E_AD1006_BAD_OUTARG_VAL)
		    error.err_code = E_US0022_FLAG_FORMAT;
		break;
	    }
	    scb->scb_sscb.sscb_facility |= (1 << DB_ADF_ID);
	}

	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_DMF_ID))
	{
	    dmc = scb->scb_sscb.sscb_dmccb;
	    dmc->type = DMC_CONTROL_CB;  
	    dmc->length = sizeof(*dmc);
	    dmc->dmc_flags_mask = 0;
	    dmc->dmc_flags_mask2 = 0;
	    dmc->dmc_name.data_address =
		    (PTR) scb->scb_sscb.sscb_ics.ics_eusername;
	    dmc->dmc_name.data_in_size =
		    sizeof(*scb->scb_sscb.sscb_ics.ics_eusername);
	    dmc->dmc_char_array.data_address= 0;	/* {@fix_me@} */
	    dmc->dmc_op_type = DMC_SESSION_OP;
	    dmc->dmc_session_id = (PTR)scb->cs_scb.cs_self;
	    dmc->dmc_server = Sc_main_cb->sc_dmvcb;
	    dmc->dmc_s_type = DMC_S_NORMAL;
	    if (scb->scb_sscb.sscb_ics.ics_requstat & DU_UUPSYSCAT)
		dmc->dmc_s_type |= DMC_S_SYSUPDATE;
	    if (scb->scb_sscb.sscb_ics.ics_requstat & DU_USECURITY)
		dmc->dmc_s_type |= DMC_S_SECURITY;
	    if ( scb->scb_sscb.sscb_stype == SCS_SFACTOTUM )
		dmc->dmc_s_type |= DMC_S_FACTOTUM;
	    dmc->dmc_dbxlate = &scb->scb_sscb.sscb_ics.ics_dbxlate;
	    dmc->dmc_cat_owner = &scb->scb_sscb.sscb_ics.ics_cat_owner;
	    /* Give DMF pointers to ADF and GWF session blocks */
	    dmc->dmc_gwf_cb = scb->scb_sscb.sscb_gwscb;
	    dmc->dmc_adf_cb = (PTR) scb->scb_sscb.sscb_adscb;
	    /*
	    ** Set security privilege for special applications
	    */
	    if (scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CREATEDB ||
		scb->scb_sscb.sscb_ics.ics_appl_code == DBA_SYSMOD ||
		scb->scb_sscb.sscb_ics.ics_appl_code == DBA_DESTROYDB ||
		scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CONVERT ||
		scb->scb_sscb.sscb_ics.ics_appl_code == DBA_NORMAL_VFDB ||
		scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CREATE_DBDBF1)
	    {
		dmc->dmc_s_type |= DMC_S_SECURITY;
	    }

	    status = dmf_call(DMC_BEGIN_SESSION, dmc);
	    if (status)
	    {
		error.err_code = dmc->error.err_code;
		error.err_data = dmc->error.err_data;
		break;
	    }
	    scb->scb_sscb.sscb_facility |= (1 << DB_DMF_ID);
	}
	
	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_GWF_ID))
	{
	    gw_rcb = scb->scb_sscb.sscb_gwccb;
	    gw_rcb->gwr_scf_session_id = scb->cs_scb.cs_self;
	    gw_rcb->gwr_type = GWR_CB_TYPE;
	    gw_rcb->gwr_length = sizeof(GW_RCB);
	    gw_rcb->gwr_session_id = scb->scb_sscb.sscb_gwscb;
	    gw_rcb->gwr_dbxlate = &scb->scb_sscb.sscb_ics.ics_dbxlate;
	    gw_rcb->gwr_cat_owner = &scb->scb_sscb.sscb_ics.ics_cat_owner;

	    status = gwf_call(GWS_INIT, gw_rcb);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(gw_rcb->gwr_error, error);
		break;
	    }
	    scb->scb_sscb.sscb_facility |= (1 << DB_GWF_ID);
	}
	
	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_SXF_ID))
	{
	    sxf_cb = scb->scb_sscb.sscb_sxccb;
	    sxf_cb->sxf_length = sizeof(SXF_RCB);
	    sxf_cb->sxf_cb_type = SXFRCB_CB;
	    sxf_cb->sxf_scb = (PTR) scb->scb_sscb.sscb_sxscb;
	    sxf_cb->sxf_sess_id = scb->cs_scb.cs_self;
	    sxf_cb->sxf_server = Sc_main_cb->sc_sxvcb;
	    status = sxf_call(SXC_BGN_SESSION, sxf_cb);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(sxf_cb->sxf_error, error);
		break;
	    }
	    scb->scb_sscb.sscb_facility |= (1 << DB_SXF_ID);
	}

	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_QSF_ID))
	{
	    qsf_cb = scb->scb_sscb.sscb_qsccb;
	    qsf_cb->qsf_length = sizeof(QSF_RCB);
	    qsf_cb->qsf_type = QSFRB_CB;
	    qsf_cb->qsf_ascii_id = QSFRB_ASCII_ID;
	    qsf_cb->qsf_owner = (PTR)DB_SCF_ID;
	    qsf_cb->qsf_scb = scb->scb_sscb.sscb_qsscb;
	    qsf_cb->qsf_server = Sc_main_cb->sc_qsvcb;
	    qsf_cb->qsf_sid = scb->cs_scb.cs_self;
	    status = qsf_call(QSS_BGN_SESSION, qsf_cb);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(qsf_cb->qsf_error, error);
		break;
	    }
	    scb->scb_sscb.sscb_facility |= (1 << DB_QSF_ID);
	}

	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_QEF_ID))
	{
	    qef_cb = scb->scb_sscb.sscb_qeccb;
	    qef_cb->qef_length = sizeof(QEF_RCB);
	    qef_cb->qef_type = QEFRCB_CB;
	    qef_cb->qef_ascii_id = QEFRCB_ASCII_ID;
	    qef_cb->qef_sess_id =  scb->cs_scb.cs_self;
	    qef_cb->qef_cb = scb->scb_sscb.sscb_qescb;
	    qef_cb->qef_adf_cb = scb->scb_sscb.sscb_adscb;
	    qef_cb->qef_asize = 0;
	    qef_cb->qef_alt = 0;
	    /* Initialize resource limits and database privs to none */
	    qef_cb->qef_cb->qef_fl_dbpriv = 0;
	    /* Default rule depth via server or session */
	    if (sess_rule_depth < 0)
		qef_cb->qef_cb->qef_max_stack = Sc_main_cb->sc_rule_depth;
	    else 				/* session value */
		qef_cb->qef_cb->qef_max_stack = sess_rule_depth;
	    qef_cb->qef_eflag = QEF_EXTERNAL;
	    qef_cb->qef_server = Sc_main_cb->sc_qevcb;
	    /* if db language is quel, set autocommit on */
	    if (scb->scb_sscb.sscb_ics.ics_qlang == DB_QUEL)
		qef_cb->qef_auto = QEF_ON;
	    else
		qef_cb->qef_auto = QEF_OFF;
            qef_cb->qef_upd_rowcnt = QEF_ROWQLFD;
	    /* Copy user name to QEF's session control block */
	    MEcopy((PTR) scb->scb_sscb.sscb_ics.ics_eusername,
			sizeof(qef_cb->qef_cb->qef_user),
			(PTR) &qef_cb->qef_cb->qef_user);
	    /* Set user status bits to zero; they are altered later. */
	    qef_cb->qef_cb->qef_ustat = 0;

	    if (scb->scb_sscb.sscb_flags & SCS_STAR)
	    {
		SCV_DBCB        *dbcb;
		QEF_CB          *qcb = qef_cb->qef_cb;

		/*
		** if the ddb description is known at this time (due
		** to caching) it should be passed to QEF. Otherwise
		** the ddb description is initialized by the
		** scs_ddbdb_info call.
		*/
		if (
		    (scb->scb_sscb.sscb_stype != SCS_S2PC)
		      &&
		    (scs_ddbsearch(scb->scb_sscb.sscb_ics.ics_dbname.db_db_name,
				   Sc_main_cb->sc_kdbl.kdb_next,
				   Sc_main_cb->sc_kdbl.kdb_next,
				   &dbcb) == TRUE)
		   )
		{
		    /* Check if only the structure has been allocated,
		    ** but not filled in yet. If it's only been allocated,
		    ** but not filled in, set cdb_deferred, so we can
		    ** force filling it in later.
		    */
		    if (dbcb->db_state == SCV_IN_PROGRESS)
			cdb_deferred = TRUE;
		    qef_cb->qef_r3_ddb_req.qer_d1_ddb_p = & dbcb->db_ddb_desc;
		}
		else
		{
		    qef_cb->qef_r3_ddb_req.qer_d1_ddb_p = NULL;
		}

		qef_cb->qef_r1_distrib = DB_2_DISTRIB_SVR | DB_3_DDB_SESS;

                /* set qef session language type */
		qcb->qef_c2_ddb_ses.qes_d8_union.u2_qry_ses.qes_q2_lang =
			scb->scb_sscb.sscb_ics.ics_qlang;
		/* supply the user name and effective user */
		qef_cb->qef_r3_ddb_req.qer_d8_usr_p = 
			&scb->scb_sscb.sscb_ics.ics_real_user;
		qef_cb->qef_r3_ddb_req.qer_d9_com_p =
			(DD_COM_FLAGS *)&star_comflags;
		qef_cb->qef_r3_ddb_req.qer_d11_modify.dd_m3_appval =
			(i4)scb->scb_sscb.sscb_ics.ics_appl_code;

		if ((scb->scb_sscb.sscb_ics.ics_requstat & DU_UUPSYSCAT) 
		    == DU_UUPSYSCAT)
		{
		    qef_cb->qef_r3_ddb_req.qer_d11_modify.dd_m2_y_flag 
			= DD_1MOD_ON;
		}
		else
		{
		    qef_cb->qef_r3_ddb_req.qer_d11_modify.dd_m2_y_flag 
			= DD_0MOD_NONE;
		}

		if (scb->scb_sscb.sscb_ics.ics_lock_mode == DMC_L_EXCLUSIVE)
		{
		    qef_cb->qef_r3_ddb_req.qer_d11_modify.dd_m1_u_flag 
			= DD_1MOD_ON;
		    qef_cb->qef_r3_ddb_req.qer_d11_modify.dd_m2_y_flag 
			= DD_0MOD_NONE;
		}
		else
		{
		    qef_cb->qef_r3_ddb_req.qer_d11_modify.dd_m1_u_flag 
			= DD_0MOD_NONE;
		}
	    }
	    else
	    {
		qef_cb->qef_r1_distrib = DB_1_LOCAL_SVR;
	    }

	    /* Tell QEF about the session's case translation semantics */
	    qef_cb->qef_dbxlate = &scb->scb_sscb.sscb_ics.ics_dbxlate;
	    qef_cb->qef_cat_owner = &scb->scb_sscb.sscb_ics.ics_cat_owner;

            /* Tell QEF the session's group and role */
	    qef_cb->qef_group = &scb->scb_sscb.sscb_ics.ics_egrpid;
	    qef_cb->qef_aplid = &scb->scb_sscb.sscb_ics.ics_eaplid;

	    /* Tell QEF how to set up session flags */
	    qef_cb->qef_sess_startup_flags = 0;

	    if (scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CONVERT)
		qef_cb->qef_sess_startup_flags |= QEF_UPGRADEDB_SESSION;

	    status = qef_call(QEC_BEGIN_SESSION, qef_cb);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(qef_cb->error, error);
		break;
	    }
	    scb->scb_sscb.sscb_facility |= (1 << DB_QEF_ID);
	}

	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_RDF_ID))
	{
	    rdf_cb = scb->scb_sscb.sscb_rdccb;

	    rdf_cb->rdf_flags_mask = 0L;
	    if (scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CREATEDB ||
	        scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CONVERT  ||
	        scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CREATE_DBDBF1)
	    {
		/* for createdb, avoid checking of synonyms */
		rdf_cb->rdf_flags_mask |= RDF_CCB_SKIP_SYNONYM_CHECK;
	    }

	    rdf_cb->rdf_server = Sc_main_cb->sc_rdvcb;
	    rdf_cb->rdf_sess_id = scb->cs_scb.cs_self;
	    rdf_cb->rdf_adf_cb = (PTR) scb->scb_sscb.sscb_adscb;

	    if (scb->scb_sscb.sscb_flags & SCS_STAR)
	    {
		rdf_cb->rdf_distrib = DB_2_DISTRIB_SVR | DB_3_DDB_SESS;
		rdf_cb->rdf_ddb_id = (PTR) scb->scb_sscb.sscb_ics.ics_opendb_id;
	    }
	    else
		rdf_cb->rdf_distrib = DB_1_LOCAL_SVR;

	    /* Tell RDF about the session's case translation semantics */
    	    rdf_cb->rdf_dbxlate = &scb->scb_sscb.sscb_ics.ics_dbxlate;
	    rdf_cb->rdf_cat_owner = &scb->scb_sscb.sscb_ics.ics_cat_owner;

	    status = rdf_call(RDF_BGN_SESSION, (PTR) rdf_cb);
	    if (status != E_DB_OK)
	    {
		STRUCT_ASSIGN_MACRO(rdf_cb->rdf_error, error);
		break;
	    }
	    scb->scb_sscb.sscb_facility |= (1 << DB_RDF_ID);
	}

	/*
	** If it's a monitor session, let it in if it is a privileged
	** user, without checking any database.  This lets monitor work
	** even when the logging system is hung (daveb).
	*/
	if( scb->scb_sscb.sscb_stype == SCS_SMONITOR) 
	{
	    if ((STbcompare( DB_NDINGRES_NAME, sizeof(DB_NDINGRES_NAME)-1,
			 (char *)scb->scb_sscb.sscb_ics.ics_eusername,
			 sizeof(*scb->scb_sscb.sscb_ics.ics_eusername),
			 TRUE) == 0) ||
		scs_chk_priv( scb->cs_scb.cs_username,
			     "SERVER_CONTROL") )
	    {
		status = E_DB_OK;
	    }
	    else
	    {
		error.err_code = E_US0003_NO_DB_PERM;
		status = E_DB_ERROR;
	    }
	}
	/*
	** If this is a regular user session, look up the database in
	** the IIDBDB.
	*/
	else if ( !(scb->scb_sscb.sscb_flags & SCS_STAR) &&
		 (scb->scb_sscb.sscb_stype == SCS_SNORMAL))
	{
	    /* now go fetch the database information from the iidbdb */

	    MEcopy("Fetching IIDBDB Information", 27,
		   scb->scb_sscb.sscb_ics.ics_act1);
	    scb->scb_sscb.sscb_ics.ics_l_act1 = 27;

	    /* NOTE: this status (and the status for the Star code in the
	     **       next IF statement) is checked in the SXF auditing code
	     **       several pages down from here (auditing SXF_A_EXECUTE).
	     */
	    status = scs_dbdb_info(scb,
				   dmc,
				   &error);

            scb->scb_sscb.sscb_ics.ics_l_act1 = 0;

	    /*
	    ** If problem getting dbdb info stop here. 
	    */
	    if (status!=E_DB_OK)
	    {
		/*
		** Preserve error code from dbdb_info (if any)
		*/
		if(error.err_code==0)
			error.err_code = E_SC0123_SESSION_INITIATE;
		break;
	    }
	    /*
	    ** If they explicitly requested update_syscat privilege and its
	    ** not allowed due to database privs, then we reject the 
	    ** association here. This allows  old frontends to request +U/+Y 
	    ** as previously, but still be subserviant to the database 
	    ** privileges for this session. For STAR compatibility we
	    ** allow the DBA to request +U/+Y when specifying -u$ingres
	    **
	    ** Note that a session can still get this privilege automatically
	    ** without issuing +U/+Y if required.
	    */
	    if(want_updsyscat==TRUE &&
	       !(scb->scb_sscb.sscb_ics.ics_fl_dbpriv  & DBPR_UPDATE_SYSCAT) &&
	       !((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_EINGRES) &&
	          (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)))
	    {
		sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		         sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
			    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
			 sizeof("+U/+Y ")-1, "+U/+Y ",
			 sizeof(SystemProductName), SystemProductName,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0 );
		error.err_code = E_US0002_NO_FLAG_PERM;
		status=E_DB_ERROR;
		break;
	    }
	    /*
	    ** Set session priority if determined from dbpriv, and
	    ** less than default. (we don't start sessions higher than
	    ** the default priority, they have to request that)
	    */
	    if(scb->scb_sscb.sscb_ics.ics_priority_limit<0)
	    {
		status=scs_set_priority(scb, 
			scb->scb_sscb.sscb_ics.ics_priority_limit);
		if(status!=E_DB_OK)
		{
		  sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		         sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
			    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
			 sizeof("(session priority)")-1, "(session priority)",
			 sizeof(SystemProductName), SystemProductName,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0 );
		    error.err_code = E_US0002_NO_FLAG_PERM;
		    status=E_DB_ERROR;
		    break;
		}
	    }
	}
	/*
	** For regular sessions in a star, get ddb info.
	*/
	else if( (scb->scb_sscb.sscb_flags & SCS_STAR) &&
		(scb->scb_sscb.sscb_stype == SCS_SNORMAL) ||
		(scb->scb_sscb.sscb_stype == SCS_S2PC))
	{
	    /*
	    ** For star initialize idle/connect limits to none.
	    ** This will need to readdressed when STAR more fully supports
	    ** session limits
	    */
	    scb->scb_sscb.sscb_ics.ics_idle_limit       = -1;
	    scb->scb_sscb.sscb_ics.ics_connect_limit    = -1;
	    scb->scb_sscb.sscb_ics.ics_cur_idle_limit   = -1;
	    scb->scb_sscb.sscb_ics.ics_cur_connect_limit= -1;

	    status = scs_ddbdb_info(scb, DB_DBDB_NAME, &error);
	    {
		if (( (qef_cb->qef_r3_ddb_req.qer_d1_ddb_p == NULL)
		     ||
		     (cdb_deferred == TRUE)
		     )
		    &&
		    (scb->scb_sscb.sscb_stype == SCS_SNORMAL))
		    /*
		     ** only check for user threads
		     ** ddb is not valid for iimonitor and DXrecovery threads
		     */
		{
		    SCV_DBCB        *dbcb1;

		    /*
		     ** QEF's session control block is incomplete, hence
		     ** must inform QEF to register the newly found DDB
		     ** descriptor pointer in the case where it is
		     ** established by a faster session
		     */

		    if (scs_ddbsearch(scb->scb_sscb.sscb_ics.ics_dbname.db_db_name,
				      Sc_main_cb->sc_kdbl.kdb_next,
				      Sc_main_cb->sc_kdbl.kdb_next,
				      &dbcb1) == TRUE)
		    {
			/* Cdb desc is still not filled in yet,
			 ** something is wrong. Fail session initiation.
			 */

			if (dbcb1->db_state == SCV_IN_PROGRESS)
			{
			    error.err_code = E_SC0123_SESSION_INITIATE;
			    status = E_DB_ERROR;
			}
			else
			{
			    qef_cb->qef_r3_ddb_req.qer_d1_ddb_p =
				&dbcb1->db_ddb_desc;

			    status = qef_call(QED_DESCPTR, qef_cb);
			    if (status)
			    {
				STRUCT_ASSIGN_MACRO(qef_cb->error, error);
				break;
			    }
			}
		    }
		    else
		    {
			/* session initiation fails */
			error.err_code = E_SC0123_SESSION_INITIATE;
			status = E_DB_ERROR;
		    }
		}
	    }
	}

	/*
	** by now the user (a value different from the Real user could be
	** specified via -u) identifier which will be in effect at the session
	** startup have been stored as the SQL-session <auth id>.
	** Since the SQL-session <auth id> may get changed in the course of the
	** session, save it as the Initial Session <auth id>.
	*/
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_susername,
			    scb->scb_sscb.sscb_ics.ics_iusername);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_susername,
			    scb->scb_sscb.sscb_ics.ics_musername);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_susername,
			    scb->scb_sscb.sscb_ics.ics_iucode);

	/*
	** Advise SXF of the user and privilege bits (on SXF session startup
	** we didn't know this)
	*/

	if (scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID))
	{
	    sxf_cb = scb->scb_sscb.sscb_sxccb;
	    sxf_cb->sxf_length = sizeof(SXF_RCB);
	    sxf_cb->sxf_cb_type = SXFRCB_CB;
	    sxf_cb->sxf_scb = (PTR) scb->scb_sscb.sscb_sxscb;
	    sxf_cb->sxf_sess_id =  scb->cs_scb.cs_self;
	    sxf_cb->sxf_server = Sc_main_cb->sc_sxvcb;
	    sxf_cb->sxf_alter= (SXF_X_USERPRIV|
				SXF_X_USERNAME|
				SXF_X_DBNAME);
	    /* User status (privs)
	    ** Note we always use the REAL stat, this stops people using
	    ** -u (e.g. DBA priv) getting privileges indirectly.
	    */
	    sxf_cb->sxf_ustat=scb->scb_sscb.sscb_ics.ics_rustat;

	    /* 
	    ** Database name. This is where we make sure SXF knows we 
	    ** are connected to the real database, not the iidbdb.
	    */
	    sxf_cb->sxf_db_name= &scb->scb_sscb.sscb_ics.ics_dbname;

	    /* Effective User name */
	    sxf_cb->sxf_user = &scb->scb_sscb.sscb_ics.ics_susername;

	    status = sxf_call(SXC_ALTER_SESSION, sxf_cb);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(sxf_cb->sxf_error, error);
		break;
	    }
	}
	/*
	** Check if user account has expired, only do this if there's a
	** frontend and the user is valid.
	*/
	if ( scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID) &&
	    (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_NOSUCHUSER) == 0 )
	{
	    status=scs_usr_expired(scb);
	    if(status!=E_DB_OK)
	    {
	    	/*
	    	** Security audit failed access
	    	*/
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
	    	{
	   	 local_status = sc0a_write_audit(
			scb,
			SXF_E_USER,	/* Type */
			SXF_A_FAIL|SXF_A_AUTHENT,  /* Action */
			scb->scb_sscb.sscb_ics.ics_terminal.db_term_name,
			sizeof(scb->scb_sscb.sscb_ics.ics_terminal.db_term_name),
			NULL,
			I_SX2727_USER_EXPIRE,	/* Mesg ID */
			TRUE,			/* Force record */
			scb->scb_sscb.sscb_ics.ics_rustat,
			&local_error		/* Error location */
			);	
	    	}
	    	error.err_code=E_US0061_USER_EXPIRED;
	    	status=E_DB_ERROR;
	    	break;
	    }
	}
	/*
	** If we  get this far authentication succeeded, so write an
	** audit record, but only for non-internal threads
	*/
	if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE &&
	     !(scb->scb_sscb.sscb_flags & SCS_INTERNAL_THREAD) )
	{
            local_status = sc0a_write_audit(
		scb,
		SXF_E_USER,	/* Type */
		SXF_A_AUTHENT|SXF_A_SUCCESS,        /* Action */
		scb->scb_sscb.sscb_ics.ics_terminal.db_term_name,
		sizeof( scb->scb_sscb.sscb_ics.ics_terminal.db_term_name),
		NULL,			/* Object Owner */
		I_SX2024_USER_ACCESS,	/* Mesg ID */
		FALSE,			/* Force record */
		scb->scb_sscb.sscb_ics.ics_rustat,
		&error			/* Error location */
		);	
	}

	/* 
	** If Monitor session, don't need SXF, DMF or QEF after dbdb lookup 
	** done. 
	*/
    	if (scb->scb_sscb.sscb_stype == SCS_SMONITOR) 
    	{
	    /*
	    **	Log the fact we are a monitor session. 
	    **  Only try this if C2SECURE.
	    */
	    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
            {
		local_status = sc0a_write_audit(
			scb,
			SXF_E_USER,	/* Type */
			SXF_A_SUCCESS | SXF_A_SELECT,  /* Action */
			scb->scb_sscb.sscb_ics.ics_terminal.db_term_name,
			sizeof(scb->scb_sscb.sscb_ics.ics_terminal.db_term_name),
			NULL,	/* Object Owner */
			I_SX2708_USER_MONITOR,	/* Mesg ID */
			TRUE,			/* Force record */
			scb->scb_sscb.sscb_ics.ics_rustat,
			&local_error		/* Error location */
			);	
		if (local_status != E_DB_OK)
		{
		    status = local_status;
		    STRUCT_ASSIGN_MACRO(local_error, error);
		    break;
		}
            }

	    /* Shut QEF down as it is unnecessary */
	    
	    if (scb->scb_sscb.sscb_facility & (1 << DB_QEF_ID))
	    {
		qef_cb = scb->scb_sscb.sscb_qeccb;
		qef_cb->qef_eflag = QEF_EXTERNAL;
		qef_cb->qef_sess_id = scb->cs_scb.cs_self;
		qef_cb->qef_cb = scb->scb_sscb.sscb_qescb;
		if (scb->scb_sscb.sscb_flags & SCS_STAR)
		    qef_cb->qef_r1_distrib = DB_2_DISTRIB_SVR | DB_3_DDB_SESS;
		else
		    qef_cb->qef_r1_distrib = DB_1_LOCAL_SVR;

		if (!qef_call(QEC_END_SESSION, qef_cb))
		    scb->scb_sscb.sscb_facility &= ~(1 << DB_QEF_ID);
	    }


	    /* If all is OK, let    	    	    	    	            */
	    /* monitor run as requested.  This is necessary to allow the    */
	    /* monitor to be used to stop servers when the iidbdb is	    */
	    /* unavailable (a desirable use of the monitor).		    */

	    if (!status)
	    {
		scb->scb_sscb.sscb_state = SCS_INPUT;
		if (status)
		{
		    status = 0;		/* Just in case iidbdb didn't exist */
		    error.err_code = 0;
		    scb->scb_sscb.sscb_ics.ics_rustat = (DU_UALL_PRIVS);
		    scb->scb_sscb.sscb_ics.ics_iustat = (DU_UALL_PRIVS);
		}
		break;
	    }
    	}

	/* 
        ** This is a security event, the attempt to open a 
        ** database.  Audit the event even if it fails. 
        */
	
	if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE &&
	     scb->scb_sscb.sscb_stype == SCS_SNORMAL )
	{
	    access = SXF_A_CONNECT;
	    if (status)
		access |= SXF_A_FAIL;
	    else
		access |= SXF_A_SUCCESS;

	    local_status = sc0a_write_audit(
			scb,
			SXF_E_DATABASE,	/* Type */
			access,  /* Action */
			(char*)&scb->scb_sscb.sscb_ics.ics_dbname,
			sizeof(scb->scb_sscb.sscb_ics.ics_dbname),
			&scb->scb_sscb.sscb_ics.ics_dbowner,
			I_SX201A_DATABASE_ACCESS,	/* Mesg ID */
			FALSE,			/* Force record */
			0,
			&local_error			/* Error location */
			);	
	    if (local_status != E_DB_OK)
	    {
		status = local_status;
		STRUCT_ASSIGN_MACRO(local_error, error);
		break;
	    }
	}
	if (status)
	    break;

	/* Alter the dmf session according to user status. */

	/*
	** Non-GCA (no f/e connection)-type sessions always run with
	** full privileges
	*/
	if ( scb->scb_sscb.sscb_need_facilities & 
		((1 << DB_GCF_ID) | (1 << DB_DMF_ID)) &&
	    (scb->scb_sscb.sscb_flags & SCS_STAR) == 0 )
	{
	    dmc = scb->scb_sscb.sscb_dmccb;
	    dmc->type = DMC_CONTROL_CB;  
	    dmc->length = sizeof(DMC_CB);
	    dmc->dmc_session_id = (PTR) scb->cs_scb.cs_self;
	    dmc->dmc_op_type = DMC_SESSION_OP;   
	    dmc->dmc_sl_scope = DMC_SL_SESSION;
	    dmc->dmc_flags_mask = DMC_SYSCAT_MODE;
	    dmc->dmc_s_type = 0;

            if (scb->scb_sscb.sscb_ics.ics_rustat & DU_UUPSYSCAT)
	        dmc->dmc_s_type |= DMC_S_SYSUPDATE;

	    /*
	    ** Real owner of database automatically gets update catalog
	    ** privilege when using a -u$ingres user_name override, and a user
	    ** with SECURITY privilege gets update catalog privilege to the
	    ** database if -u$ingres is specified.
	    */
	    if (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_EINGRES)
	    {
	        if ((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA) ||
		    (scb->scb_sscb.sscb_ics.ics_requstat & DU_USECURITY) ||
		    (scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY))
	        {
		    scb->scb_sscb.sscb_ics.ics_requstat |= DU_UUPSYSCAT;
		    scb->scb_sscb.sscb_ics.ics_maxustat |= DU_UUPSYSCAT;
		    dmc->dmc_s_type |= DMC_S_SYSUPDATE;
	        }
	    }
            if (scb->scb_sscb.sscb_ics.ics_rustat & DU_USECURITY)
	        dmc->dmc_s_type |= DMC_S_SECURITY;

	    /* Set normal state if no others set */
	    if(dmc->dmc_s_type==0)
		    dmc->dmc_s_type = DMC_S_NORMAL;

	    status = dmf_call(DMC_ALTER, dmc);
	    if (DB_FAILURE_MACRO(status))
	    {
	        if (dmc->error.err_code != E_DM0065_USER_INTR)
	        {
		    STRUCT_ASSIGN_MACRO(dmc->error, error);
		    break;
	        }
	    }
	}

	/* Move collation into adf_cb */
	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_ADF_ID))
        {
	    adf_cb->adf_collation = scb->scb_sscb.sscb_ics.ics_coldesc;
	    adf_cb->adf_ucollation = scb->scb_sscb.sscb_ics.ics_ucoldesc;
	    /*
	    ** FIXME this ucollation load may not be needed if catalog
	    ** objects that are really byte are changed from VCHAR to VBYTE?
	    **
	    ** For STAR server we don't call DMF to get ucollation, so ...
	    */
	    if ( (scb->scb_sscb.sscb_flags & SCS_STAR) &&
		CMischarset_utf8() &&
		!adf_cb->adf_ucollation )
	    {
		char		col[] = "udefault";
		CL_ERR_DESC	sys_err;
		PTR		ucode_ctbl;
		PTR		ucode_cvtbl;
		if (status = aduucolinit(col, MEreqmem, 
		    &ucode_ctbl, &ucode_cvtbl, &sys_err))
		{
		    error.err_code = E_DM00A0_UNKNOWN_COLLATION;
		    status = E_DB_ERROR;
		    break;
		}
		adf_cb->adf_ucollation = ucode_ctbl;
	    }
	    if (adf_cb->adf_ucollation)
	    {
		if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_UNICODE_NFC)
		    adf_cb->adf_uninorm_flag = AD_UNINORM_NFC;
		else adf_cb->adf_uninorm_flag = AD_UNINORM_NFD;
	    }
	    else adf_cb->adf_uninorm_flag = 0;
        }

	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_PSF_ID))
	{
	    psq_cb = scb->scb_sscb.sscb_psccb;
	    psq_cb->psq_length = sizeof(PSQ_CB);
	    psq_cb->psq_type = PSQCB_CB;
	    psq_cb->psq_ascii_id = PSQCB_ASCII_ID;
	    psq_cb->psq_qlang = scb->scb_sscb.sscb_ics.ics_qlang;
	    psq_cb->psq_parser_compat = scb->scb_sscb.sscb_ics.ics_parser_compat;
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_decimal, psq_cb->psq_decimal);
	    psq_cb->psq_dbid = (PTR) scb->scb_sscb.sscb_ics.ics_opendb_id;
	    psq_cb->psq_udbid = scb->scb_sscb.sscb_ics.ics_udbid;
	    psq_cb->psq_alter_op=0;
	    psq_cb->psq_sessid = scb->cs_scb.cs_self;
	    psq_cb->psq_adfcb = (PTR) scb->scb_sscb.sscb_adscb;
	    MECOPY_CONST_MACRO(scb->scb_sscb.sscb_ics.ics_eusername,
			sizeof(psq_cb->psq_user),
			&psq_cb->psq_user);
	    MECOPY_CONST_MACRO(&scb->scb_sscb.sscb_ics.ics_dbowner,
			sizeof(psq_cb->psq_dba),
			&psq_cb->psq_dba);
	    MECOPY_CONST_MACRO(&scb->scb_sscb.sscb_ics.ics_egrpid,
			sizeof(psq_cb->psq_group),
			&psq_cb->psq_group);
	    MECOPY_CONST_MACRO(&scb->scb_sscb.sscb_ics.ics_eaplid,
			sizeof(psq_cb->psq_aplid),
			&psq_cb->psq_aplid);
	    psq_cb->psq_idxstruct = scb->scb_sscb.sscb_ics.ics_idxstruct;

	    psq_cb->psq_flag = 0L;	/* start by cleaning all bits */

	    /*
	    ** Set PSQ_WARNINGS conditionaly.  If a DUF QUEL application code
	    ** is used, PSF must skip warnings of obsolete QUEL  statements.
	    ** Otherwise it should print them.
	    */
	    if ( scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CREATEDB	    &&
		 scb->scb_sscb.sscb_ics.ics_appl_code != DBA_SYSMOD	    &&
		 scb->scb_sscb.sscb_ics.ics_appl_code != DBA_DESTROYDB	    &&
		 scb->scb_sscb.sscb_ics.ics_appl_code != DBA_FINDDBS	    &&
		 scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CONVERT	    &&
		 scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CREATE_DBDBF1)
	    {
		/* turn on the print warnings flag: psq_warnings = TRUE; */
		psq_cb->psq_flag |= PSQ_WARNINGS;
	    }

	    if (scb->scb_sscb.sscb_ics.ics_requstat & DU_UUPSYSCAT)
		psq_cb->psq_flag |= PSQ_CATUPD;	    /* psq_catupd = TRUE */

	    if (scb->scb_sscb.sscb_ics.ics_appl_code == DBA_PURGE_VFDB)
	    {
		psq_cb->psq_flag |= PSQ_DBA_DROP_ALL; /*psq_dba_drop_all=TRUE*/
		psq_cb->psq_flag |= PSQ_SELECT_ALL;   
		psq_cb->psq_flag |= PSQ_INGRES_PRIV;
	    }
	    else if (scb->scb_sscb.sscb_ics.ics_appl_code == DBA_PRETEND_VFDB)
	    {
		psq_cb->psq_flag |= PSQ_REPAIR_SYSCAT;
		psq_cb->psq_flag |= PSQ_INGRES_PRIV;
	    }
	    else if (scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CONVERT)
		psq_cb->psq_flag |= PSQ_RUNNING_UPGRADEDB;

	    psq_cb->psq_server = Sc_main_cb->sc_psvcb;
	    psq_cb->psq_ustat = scb->scb_sscb.sscb_ics.ics_rustat;

	    /* There's no scb_cscb if no GCA! */
	    if ((scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID)) == 0 ||
                scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_50)
            {
                psq_cb->psq_flag |= PSQ_STRIP_NL_IN_STRCONST;
            }

	    /* let PSF know if the server has security row attributes */
	    if (Sc_main_cb->sc_secflags&SC_SEC_ROW_AUDIT)
		psq_cb->psq_flag |= PSQ_ROW_SEC_AUDIT;

	    if (Sc_main_cb->sc_secflags&SC_SEC_ROW_KEY)
		psq_cb->psq_flag |= PSQ_ROW_SEC_KEY;

	    if (Sc_main_cb->sc_secflags&SC_SEC_PASSWORD_NONE)
		psq_cb->psq_flag |= PSQ_PASSWORD_NONE;

	    if (Sc_main_cb->sc_secflags&SC_SEC_ROLE_NONE)
		psq_cb->psq_flag |= PSQ_ROLE_NONE;

	    if (Sc_main_cb->sc_secflags&SC_SEC_ROLE_NEED_PW)
		psq_cb->psq_flag |= PSQ_ROLE_NEED_PW;

	    if (!(Sc_main_cb->sc_rule_del_prefetch_off))
		psq_cb->psq_flag |= PSQ_RULE_DEL_PREFETCH;

	    if (scb->scb_sscb.sscb_flags & SCS_STAR)
		psq_cb->psq_distrib = DB_2_DISTRIB_SVR | DB_3_DDB_SESS;
	    else
		psq_cb->psq_distrib = DB_1_LOCAL_SVR;

	    /* Tell PSF about the session's case translation semantics */
    	    psq_cb->psq_dbxlate = &scb->scb_sscb.sscb_ics.ics_dbxlate;
	    psq_cb->psq_cat_owner = &scb->scb_sscb.sscb_ics.ics_cat_owner;

            if (scb->scb_sscb.sscb_ics.ics_rustat & DU_UTRACE)
                psq_cb->psq_flag |= PSQ_ALLOW_TRACEFILE_CHANGE;

            psq_cb->psq_flag2 = 0L;     /* start by cleaning all bits */
 
            if(!(Sc_main_cb->sc_rule_upd_prefetch_off))
                psq_cb->psq_flag2 |= PSQ_RULE_UPD_PREFETCH;

            if(Sc_main_cb->sc_money_compat)
                psq_cb->psq_flag2 |= PSQ_MONEY_COMPAT;

	    psq_cb->psq_result_struct = 0;
	    if (result_structure != 0)
	    {
		psq_cb->psq_result_struct = abs(result_structure);
		psq_cb->psq_result_compression = FALSE;
		if (result_structure < 0)
		    psq_cb->psq_result_compression = FALSE;
	    }

	    status = psq_call(PSQ_BGN_SESSION, psq_cb,
				scb->scb_sscb.sscb_psscb);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(psq_cb->psq_error, error);
		break;
	    }
	    scb->scb_sscb.sscb_facility |= (1 << DB_PSF_ID);
	}

	if (scb->scb_sscb.sscb_need_facilities & (1 << DB_OPF_ID))
	{
	    opf_cb = scb->scb_sscb.sscb_opccb;
	    opf_cb->opf_length = sizeof(OPF_CB);
	    opf_cb->opf_type = OPFCB_CB;
	    opf_cb->opf_ascii_id = OPFCB_ASCII_ID;
	    opf_cb->opf_scb = (PTR *)scb->scb_sscb.sscb_opscb;
	    opf_cb->opf_adfcb = scb->scb_sscb.sscb_adscb;
	    opf_cb->opf_udbid = scb->scb_sscb.sscb_ics.ics_udbid;
	    opf_cb->opf_dbid = scb->scb_sscb.sscb_ics.ics_opendb_id;
	    opf_cb->opf_sid = scb->cs_scb.cs_self;
	    opf_cb->opf_repeat = (PTR *) 0;	/* initialize this field    */
	    opf_cb->opf_server = Sc_main_cb->sc_opvcb;
	    opf_cb->opf_smask = 0;
	    opf_cb->opf_coordinator = NULL;
#ifdef	OPF_RESOURCE
            if (scb->scb_sscb.sscb_ics.ics_ctl_dbpriv &
                scb->scb_sscb.sscb_ics.ics_fl_dbpriv  &
                DBPR_OPFENUM
               )
                opf_cb->opf_smask |= OPF_RESOURCE;      /* Force enumeration */
#endif
	    /* STAR addition */
	    if (scb->scb_sscb.sscb_flags & SCS_STAR)
	    {
		opf_cb->opf_smask = OPF_SDISTRIBUTED;
		opf_cb->opf_qeftype = OPF_LQEF;
		opf_cb->opf_coordinator =
		    (PTR)&scb->scb_sscb.sscb_dbcb->db_ddb_desc.dd_d3_cdb_info;
	    }

	    /* Tell OPF about the session's case translation semantics */
	    opf_cb->opf_dbxlate = &scb->scb_sscb.sscb_ics.ics_dbxlate;
	    opf_cb->opf_cat_owner = &scb->scb_sscb.sscb_ics.ics_cat_owner;
	    MECOPY_CONST_MACRO(scb->scb_sscb.sscb_ics.ics_eusername,
			sizeof(opf_cb->opf_default_user),
			&opf_cb->opf_default_user);

	    status = opf_call(OPF_BGN_SESSION, opf_cb);
	    if (status != E_DB_OK)
	    {
		STRUCT_ASSIGN_MACRO(opf_cb->opf_errorblock, error);
		break;
	    }
	    scb->scb_sscb.sscb_facility |= (1 << DB_OPF_ID);
	}

	/* 
	** If re-connect to a session, resume the transacction context.
	*/

	if (scb->scb_sscb.sscb_ics.ics_dmf_flags & DMC_NLK_SESS)
	{
	    if(scb->scb_sscb.sscb_qeccb == NULL)
		break;
	    qef_cb = scb->scb_sscb.sscb_qeccb;
	    qef_cb->qef_db_id = scb->scb_sscb.sscb_ics.ics_opendb_id;
	    qef_cb->qef_modifier = QEF_MSTRAN;
	    qef_cb->qef_cb = scb->scb_sscb.sscb_qescb;
	    qef_cb->qef_sess_id = scb->cs_scb.cs_self;
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_dis_tran_id, qef_cb->qef_cb->qef_dis_tran_id);	
	    status = qef_call(QET_RESUME, qef_cb);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(qef_cb->error, error);
		if (error.err_code == E_QE0054_DIS_TRAN_UNKNOWN)
		    error.err_code = E_US1267_4711_DIS_TRAN_UNKNOWN;
		else if (error.err_code == E_QE0055_DIS_TRAN_OWNER)
		    error.err_code = E_US1268_4712_DIS_TRAN_OWNER;

/* from INGRES 6.4 bug fix #62708 by angusm (harpa06) */

                else if (error.err_code == E_QE005A_DIS_DB_UNKNOWN)
                    error.err_code = E_US126B_4715_DIS_DB_UNKNOWN;

		break;
	    }
	}

	/*
	** Get ready to handle user password if required
	*/
	if( scb->scb_sscb.sscb_ics.ics_uflags&SCS_USR_RPASSWD)
	{
	    /*
	    ** Can only prompt for password if there is a frontend,
	    ** so reject if no frontend
	    */
	    if ((scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID)) == 0)
	    {
	        error.err_code = E_US18FF_BAD_USER_ID;
		status=E_DB_ERROR;
		break;
	    }
	    /*
	    ** Setup to prompt for password
	    */
	    scb->scb_sscb.sscb_state = SCS_SPASSWORD;
	}
	else if (scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID))
	{
	    /*
	    ** If Connected to a front end then prime for input.
	    */
	    scb->scb_sscb.sscb_state = SCS_INPUT;
	}
	break;
    } 
    
    if (status)
    {
	if (error.err_code >> 16)
	    scd_note(status, error.err_code >> 16);
	else
	    scd_note(status, DB_SCF_ID);

        /* --- liibi01 Bug 60932                     --- */
	 /* --- Attach the <dbname> as one argument   --- */
        /* --- for error code E_US0010.              --- */
        /* --- Write the error message to error log. --- */

	if (status && error.err_code == 16) 
	    sc0e_put(error.err_code, 0, 1,
                     sizeof(scb->scb_sscb.sscb_ics.ics_dbname.db_db_name),
		     (PTR) scb->scb_sscb.sscb_ics.ics_dbname.db_db_name,
		     0, (PTR) 0,
                     0, (PTR) 0,
		     0, (PTR) 0,
                     0, (PTR) 0,
		     0, (PTR) 0 );
        else if (status)
           sc0e_0_put(error.err_code, 0);
           	     
	/* else reason for this has already been logged */

	/* Fire db security alarms on failure */
        (VOID) scs_fire_db_alarms(scb, DB_CONNECT|DB_ALMFAILURE);
	sc0e_0_put(E_SC0123_SESSION_INITIATE, 0);
	if (error.err_code == 0)
	    error.err_code = E_US0004_CANNOT_ACCESS_DB;
    }
    else if (scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID))
    {    /* Don't fire alarms for connectionless sessions */

	if(scb->scb_sscb.sscb_state != SCS_SPASSWORD)
	{
	    /*
	    ** Fire security alarms on successful connection, unless
	    ** need password to complete authentication process
	    */
	    (VOID) scs_fire_db_alarms(scb, DB_CONNECT|DB_ALMSUCCESS);
	}

	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_SESSION_LOG, NULL, NULL))
	{
	    sc0e_put(E_SC0233_SESSION_START, 0, 5,
		     ((char *)
		      STindex((PTR)scb->scb_sscb.sscb_ics.ics_eusername,
			      " ", 0)  -
		      (char *)
		      scb->scb_sscb.sscb_ics.ics_eusername),
		     (PTR)scb->scb_sscb.sscb_ics.ics_eusername,
		     ((char *)
		      STindex((PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
			      " ", 0)  -
		      (char *)
		      &scb->scb_sscb.sscb_ics.ics_rusername),
		     (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     (scb->scb_sscb.sscb_stype == SCS_SMONITOR
		      ? sizeof("<Monitor Session>") - 1
		      :((char *)
			STindex((PTR)&scb->scb_sscb.sscb_ics.ics_dbname,
				" ", 0)  -
			(char *)
			&scb->scb_sscb.sscb_ics.ics_dbname)),
		     (PTR)(scb->scb_sscb.sscb_stype == SCS_SMONITOR
			   ? "<Monitor Session>"
			   : (char *) &scb->scb_sscb.sscb_ics.ics_dbname),
		     ((scb->scb_sscb.sscb_ics.ics_lock_mode
		       == DMC_L_EXCLUSIVE)
		      ? sizeof("Exclusive") - 1
		      : sizeof("Shared") - 1),
		     (PTR)((scb->scb_sscb.sscb_ics.ics_lock_mode
			    == DMC_L_EXCLUSIVE)
			   ? "Exclusive"
			   : "Shared"),
		     ((scb->scb_sscb.sscb_ics.ics_requstat & DU_UUPSYSCAT)
		      ? sizeof(", System Catalog Update") - 1
		      : sizeof(", Normal") - 1),
		     (PTR)((scb->scb_sscb.sscb_ics.ics_requstat & DU_UUPSYSCAT)
			   ? (", System Catalog Update")
			   : ", Normal"),
		     0, (PTR)0);
	}

	/* SC930 trace of session begin */

	if ((scb->scb_sscb.sscb_stype != SCS_SMONITOR) &&
	    (ult_always_trace() & SC930_TRACE))
	{
	    void *f = ult_open_tracefile((PTR)scb->cs_scb.cs_self);
	    if (f)
	    {
		    char tmp[1000];

		    STprintf(tmp,"(DBID=%d)(%*s)(%*s)(%*s)(SVRCL=%*s)(%*s)(%08x:%08x)",
		       scb->scb_sscb.sscb_ics.ics_udbid,
				    /* username */
		      sizeof(scb->scb_sscb.sscb_ics.ics_iusername.db_own_name),
		      scb->scb_sscb.sscb_ics.ics_iusername.db_own_name,
				    /* role */
		      sizeof(scb->scb_sscb.sscb_ics.ics_eaplid.db_own_name),
		      scb->scb_sscb.sscb_ics.ics_eaplid.db_own_name,
				    /* group */
		      sizeof(scb->scb_sscb.sscb_ics.ics_egrpid.db_own_name),
		      scb->scb_sscb.sscb_ics.ics_egrpid.db_own_name,
				    /* Server class */
		      SVR_CLASS_MAXNAME,
		      Sc_main_cb->sc_server_class,
				    /* dbname */
		       sizeof(scb->scb_sscb.sscb_ics.ics_dbname.db_db_name),
		       scb->scb_sscb.sscb_ics.ics_dbname.db_db_name,
				    /* Dis transaction ID */
		      scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.
		           db_ingres_dis_tran_id.db_tran_id.db_high_tran,
		      scb->scb_sscb.sscb_dis_tran_id.db_dis_tran_id.
		           db_ingres_dis_tran_id.db_tran_id.db_low_tran
		      );

		    ult_print_tracefile(f,SC930_LTYPE_BEGINTRACE,tmp);
		    ult_close_tracefile(f);

	    }
	}

    }

    return(error.err_code);
}

/*{
** Name: scs_terminate	- Terminate the session marked as current.
**
** Description:
**      This routine terminates the current session.  It essentially undoes 
**      whatever has been done above. 
[@comment_line@]...
**
** Inputs:
**	scb				Session control block to terminate
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	2-Jul-1986 (fred)
**          Created on Jupiter.
**      11-apr-90 (jennifer)
**          Bug 20481 - Add qeu_audit call when a session terminates.
**	10-oct-90 (ralph)
**	    6.3->6.5 merge:
**	    03-apr-1990 (fred)
**		Added CScomment() call when performing an abort on behalf of a
**		terminated thread.
**	04-feb-91 (neil)
**	    Make sure to dispose of outstanding event registrations asap.
**	29-Jan-1991 (anton)
**	    Added support for GCA API 2 - call scd_disassoc
**	25-sep-92 (markg)
**	    Added code to terminate SXF sessions.
**	7-oct-92 (daveb)
**	    Promote GWF to first class citizenship, and shut it down
**	    here too.
**	17-oct-92 (robf)
**	     When auditing session termination use a specific message
**	     (I_SX2707_USER_TERMINATE) rather than the generic user message.
**	     Also log the Object as the user's terminal.
**      10-mar-93 (mikem)
**          bug #47624
**          Disable CSresume() calls from scc_gcomplete() while session is
**          running down in scs_terminate() by setting the
**          sccb_terminating field.  Allowing CSresume()'s during this time
**          leads to early wakeups from CSsuspend()'s called by scs_terminate
**          (like DI disk I/O operations).
**	2-Jul-1993 (daveb)
**	    prototyped.
**	30-Jul-1993 (daveb)
**	    Don't do more disassocs if we know the assoc_id is already down.
**	    (This shuts up some needless errors).
**	29-Sep-1993 (rog)
**	    QSF session termination should be after the termination of the
**	    facilities that use QSF so that QSF's orphan object detection
**	    code doesn't find things that the other facilities plan on
**	    destroying.
**	3-dec-93 (robf)
**          Handle disconnect alarms before cleaning up
**	14-dec-93 (robf)
**          Use normal header for alarm next pointer
**	22-feb-94 (robf)
**          Remove any role information associated with the session.
**	24-Apr-1994 (rog)
**	    dbdb_dbtuple should not be declared READONLY because it is being
**	    updated.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.
**      10-May-1995 (lewda02/shust01)
**          RAAT API Project:
**          Free memory from new sscb_raat_message pointer (if allocated).
**      8-jun-1995 (lewda02/thaju02)
**          RAAT API Project:
**          Deallocation of linked buffer list.
**      14-Sep-1995 (shust01/thaju02)
**          RAAT API Project:
**          Free allocated memory (if any) from pointers.
**	24-Oct-1996 (cohmi01)
**	    Don't free sscb_raat_message until after scc_dispose runs through
**	    the pending message queue. Bug 78155.
**    21-oct-1997(angusm)
**        Reset SCS_ROLE_NONE which was set by 'upgradedb', bug 72896
**        (side-effect of change 415832 for bug 66466)
**	18-Mar-1998 (jenjo02)
**	    Connectionless sessions don't have GCA, alarms, alerts, RAATs, etc,
**	    so skip all that unneeded work when ending one of these.
**	02-Apr-1998 (jenjo02)
**	    Added Factotum thread termination code.
**	04-May-1998 (jenjo02)
**	    Initialize status to E_DB_OK;
**	24-feb-98 (stephenb)
**	    clean up sscb_blobwork on disconnect.
**    28-Sep-1998 (horda03)
**        Bug 90634 - Initialise STATUS before use. It is possible for
**        status to remain undefined before it is tested to see if the
**        E_SC012B_SESSION_TERMINATE message should be raised.
**    25-Jul-2000 (hanal04) Bug 102108 INGSRV 1230
**        Don't try to remove the sscb_asemaphore if we terminated the
**        session before it could be initialised.
**	29-Apr-2004 (schka24)
**	    Terminate CUT usage in case thread was using CUT and neglected
**	    to clean up.  (This is really an internal error, but if we don't
**	    do this, CUT can get cluttered with garbage.)
**	15-Sep-2004 (schka24)
**	    Do a better job of CUT termination.
**	01-may-2008 (smeke01) b118780
**	    Corrected name of sc_avgrows to sc_totrows.
**	21-Apr-2010 (kschendel) SIR 123485
**	    Blobwork is now allocated along with lowksp.  Delete lowksp when
**	    terminating a session.
*/
DB_STATUS
scs_terminate(SCD_SCB *scb )
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		bad_status = E_DB_OK;
    DB_STATUS		err = E_DB_OK;
    DMC_CB		*dmc;
    OPF_CB		*opf_cb;
    PSQ_CB		*psq_cb;
    QSF_RCB		*qsf_cb;
    QEF_RCB		*qef_cb;
    SXF_RCB		*sxf_cb;
    GW_RCB		*gw_rcb;
    SCS_MCB		*mcb;
    SCS_MCB		*new_mcb;
    SCS_ALARM		*alarm;
    SCS_ROLE		*role;
    DB_ERROR		error;
    SCS_RAAT_MSG	*raat_msg;
    SCS_RAAT_MSG	*prev_raat_msg;
    char		*pmvalue;

    MEcopy("Terminating session", 19, scb->scb_sscb.sscb_ics.ics_act1);
    scb->scb_sscb.sscb_ics.ics_l_act1 = 19;

    /*
    ** reset SC_SEC_ROLE_NONE flag set for 'upgradedb'
    */
    if (scb->scb_sscb.sscb_ics.ics_dmf_flags & DMC_CVCFG)
    {
	if ((Sc_main_cb->sc_capabilities & (SC_C_C2SECURE))
	    && (PMget("II.$.SECURE.ROLES", &pmvalue) == OK))
	{
	    if (STcasecmp(pmvalue, "ON" )==0) 
	    {
		Sc_main_cb->sc_secflags &= ~SC_SEC_ROLE_NONE;
	    }
	    else if(STcasecmp(pmvalue, "OFF" )==0)
	    {
		Sc_main_cb->sc_secflags |= SC_SEC_ROLE_NONE;
	    }
	}
	else
	    Sc_main_cb->sc_secflags &= ~SC_SEC_ROLE_NONE;
    }

    /*
    ** If this is a Factotum thread and 
    ** the Factotum thread execution function was
    ** invoked, invoke the thread exit function
    ** (if any) after first creating a SCF_FTX
    ** containing:
    **
    **	o thread-creator-supplied data, if any
    **	o length of that data
    **	o the thread creator's thread_id
    **	o status and error info from the
    **	  thread execution code
    */
    if (scb->scb_sscb.sscb_stype == SCS_SFACTOTUM &&
	scb->scb_sscb.sscb_factotum_parms.ftc_state == FTC_ENTRY_CODE_CALLED &&
	scb->scb_sscb.sscb_factotum_parms.ftc_thread_exit)
    {
	SCF_FTX	ftx;

	ftx.ftx_data = scb->scb_sscb.sscb_factotum_parms.ftc_data;
	ftx.ftx_data_length = scb->scb_sscb.sscb_factotum_parms.ftc_data_length;
	ftx.ftx_thread_id = scb->scb_sscb.sscb_factotum_parms.ftc_thread_id;
	ftx.ftx_status = scb->scb_sscb.sscb_factotum_parms.ftc_status;
	ftx.ftx_error.err_code = scb->scb_sscb.sscb_factotum_parms.ftc_error.err_code;
	ftx.ftx_error.err_data = scb->scb_sscb.sscb_factotum_parms.ftc_error.err_data;

	scb->scb_sscb.sscb_factotum_parms.ftc_state = FTC_EXIT_CODE_CALLED;

	(*(scb->scb_sscb.sscb_factotum_parms.ftc_thread_exit))( &ftx );
    }

    /* Connectionless threads don't havee alarms, alerts, or RAAT api's */
    if (scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID))
    {
	/* The session is terminating, while this flag is set scc_gcomplete()
	** will not deliver communication related CSresume()'s to this
	** thread.
	*/

	scb->scb_sscb.sscb_terminating = 1;

	/* clear out possible outstanding GCA resumes, which could incorrectly
	** resume other suspends encountered during session shutdown
	*/
	CScancelled(0);
	CSintr_ack();

	/*
	** Remove any alert registrations.  This is done early on in
	** scs_terminate specifically to avoid cases where the alert
	** data structures still point at already-disposed thread session
	** ids.  This is explained in sce_end_session.
	*/
	sce_end_session(scb, (SCF_SESSION)scb->cs_scb.cs_self);

	/* If session used blobs, delete sequencer couponify/redeem
	** blob workspace.
	*/

	if (scb->scb_sscb.sscb_lowksp != NULL)
	    (void) sc0m_deallocate(0, &scb->scb_sscb.sscb_lowksp);

	/*
	** Free any memory the RAAT API allocated, except for raat_message,
	** which must not be freed until we run through the message queue.
	*/
	if (scb->scb_sscb.sscb_big_buffer != NULL)
	{
	    status = sc0m_deallocate(0, (PTR *)&scb->scb_sscb.sscb_big_buffer);
	    if (status)
	    {
		sc0e_0_put(status, 0);
	    }
	}
 
	if (scb->scb_sscb.sscb_raat_workarea != NULL)
	{
	    status = sc0m_deallocate(0, 
				(PTR *)&scb->scb_sscb.sscb_raat_workarea);
	    if (status)
	    {
		sc0e_0_put(status, 0);
	    }
	}

	/* Fire any session termination alarms */
	(VOID)scs_fire_db_alarms(scb, DB_DISCONNECT|DB_ALMSUCCESS);

	/*
	** Now deallocate alarms
	*/
	alarm = scb->scb_sscb.sscb_alcb;
	while(alarm)
	{
		SCS_ALARM *alnext = alarm->scal_next;
		status = sc0m_deallocate(0, (PTR *) &alarm);
		if (status)
		{
		    sc0e_0_put(status, 0);
		}
		alarm = alnext;
	}
	scb->scb_sscb.sscb_alcb = NULL;

	/*
	** Deallocate any role information for the session
	*/
	role = scb->scb_sscb.sscb_rlcb;
	while (role)
	{
	    SCS_ROLE *rlnext = role->scrl_next;
	    status = sc0m_deallocate(0, (PTR *) &role);
	    if (status)
	    {
		sc0e_0_put(status, 0);
	    }
	    role = rlnext;
	}
	scb->scb_sscb.sscb_rlcb=NULL;
    }

    /* Shut down any CUT child threads that have not been removed yet */
    scs_cut_cleanup();

    /* no rdf initialization */

    if (scb->scb_sscb.sscb_facility & (1 << DB_ADF_ID))
    {
	status = adg_release(scb->scb_sscb.sscb_adscb);
	if (status)
	{
	    error.err_code = status;
	    sc0e_0_put(error.err_code, 0);
	    status = E_DB_ERROR;
	}
    }

    if (scb->scb_sscb.sscb_facility & (1 << DB_PSF_ID))
    {
	psq_cb = scb->scb_sscb.sscb_psccb;
	psq_cb->psq_flag |= PSQ_FORCE;	    /*	psq_force = TRUE */
	status = psq_call(PSQ_END_SESSION, psq_cb, scb->scb_sscb.sscb_psscb);
	if (status)
	{
	    STRUCT_ASSIGN_MACRO(psq_cb->psq_error, error);
	    sc0e_0_put(error.err_code, 0);
	    bad_status = max(status, bad_status);
	}
    }
    if (scb->scb_sscb.sscb_facility & (1 << DB_OPF_ID))
    {
	opf_cb = scb->scb_sscb.sscb_opccb;
	opf_cb->opf_scb = (PTR *) scb->scb_sscb.sscb_opscb;
	/*opf_cb->opf_sid = (PTR) scb->cs_scb.cs_self;*/ /* {@fix_me@} */
	status = opf_call(OPF_END_SESSION, opf_cb);
	if (status != E_DB_OK)
	{
	    STRUCT_ASSIGN_MACRO(opf_cb->opf_errorblock, error);
	    sc0e_0_put(error.err_code, 0);
	    bad_status = max(status, bad_status);
	}
    }

    if (scb->scb_sscb.sscb_flags & SCS_STAR)
    {
	scs_ddbclose(scb);
    }
    else
    {
	dmc = scb->scb_sscb.sscb_dmccb;

	if (scb->scb_sscb.sscb_ics.ics_opendb_id)
	{
	    DB_ERROR		dmf_error;
	    status = scs_dbclose(0, scb, &dmf_error, dmc);
	    if (status)
	    {
		STRUCT_ASSIGN_MACRO(dmf_error, error);
		sc0e_0_put(error.err_code, 0);
	    }
	    bad_status = max(status, bad_status);
	}
    }

    if (scb->scb_sscb.sscb_facility & (1 << DB_QEF_ID))
    {
	qef_cb = scb->scb_sscb.sscb_qeccb;
	qef_cb->qef_eflag = QEF_EXTERNAL;
	qef_cb->qef_sess_id = scb->cs_scb.cs_self;
	qef_cb->qef_cb = scb->scb_sscb.sscb_qescb;

	if (scb->scb_sscb.sscb_flags & SCS_STAR)
	    qef_cb->qef_r1_distrib = DB_2_DISTRIB_SVR | DB_3_DDB_SESS;
	else
	    qef_cb->qef_r1_distrib = DB_1_LOCAL_SVR;

	status = qef_call(QEC_END_SESSION, qef_cb);
	if (status)
	{
	    STRUCT_ASSIGN_MACRO(qef_cb->error, error);
	    sc0e_0_put(error.err_code, 0);
	    bad_status = max(status, bad_status);
	}
    }

    /* QSF session termination should be after the users of QSF. */
    if (scb->scb_sscb.sscb_facility & (1 << DB_QSF_ID))
    {
	qsf_cb = scb->scb_sscb.sscb_qsccb;
	qsf_cb->qsf_scb = scb->scb_sscb.sscb_qsscb;
	status = qsf_call(QSS_END_SESSION, qsf_cb);
	if (status)
	{
	    STRUCT_ASSIGN_MACRO(qsf_cb->qsf_error, error);
	    sc0e_0_put(error.err_code, 0);
	    bad_status = max(status, bad_status);
	}
    }

    if (scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID))
    {
	sxf_cb = scb->scb_sscb.sscb_sxccb;
	sxf_cb->sxf_length = sizeof (SXF_RCB);
	sxf_cb->sxf_cb_type = SXFRCB_CB;
	sxf_cb->sxf_scb = (PTR) scb->scb_sscb.sscb_sxscb;
	sxf_cb->sxf_sess_id = scb->cs_scb.cs_self;
	sxf_cb->sxf_server = Sc_main_cb->sc_sxvcb;

	/* 
	** The exit of a session is a security event, it must 
	** be audited. SXF and DMF need to be running.
	*/
	if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE &&
	    scb->scb_sscb.sscb_stype == SCS_SNORMAL &&
	    scb->scb_sscb.sscb_facility & (1 << DB_DMF_ID) )
	{
	    status = sc0a_write_audit(
		    scb,
		    SXF_E_USER,	/* Type */
		    SXF_A_SUCCESS | SXF_A_TERMINATE,  /* Action */
		    scb->scb_sscb.sscb_ics.ics_terminal.db_term_name,
		    sizeof(scb->scb_sscb.sscb_ics.ics_terminal.db_term_name),
		    NULL,	/* Object Owner */
		    I_SX2707_USER_TERMINATE,	/* Mesg ID */
		    TRUE,			/* Force record */
		    scb->scb_sscb.sscb_ics.ics_rustat,
		    &error			/* Error location */
		    );	
	    if (status!=E_DB_OK)
	    {
		sc0e_0_put(error.err_code, 0);
		bad_status = max(status, bad_status);
	    }
	}

	status = sxf_call(SXC_END_SESSION, sxf_cb);
	if (status)
	{
	    STRUCT_ASSIGN_MACRO(sxf_cb->sxf_error, error);
	    sc0e_0_put(error.err_code, 0);
	    bad_status = max(status, bad_status);
	}
    }

    if (scb->scb_sscb.sscb_facility & (1 << DB_DMF_ID))
    {
	dmc->type = DMC_CONTROL_CB;
	dmc->length = sizeof(*dmc);
	dmc->dmc_op_type = DMC_SESSION_OP;
	dmc->dmc_db_id = scb->scb_sscb.sscb_ics.ics_opendb_id;
	dmc->dmc_session_id = (PTR) scb->cs_scb.cs_self;

	status = dmf_call(DMC_END_SESSION, dmc);
	if (status)
	{
	    error.err_code = dmc->error.err_code;
	    error.err_data = dmc->error.err_data;
	    sc0e_0_put(error.err_code, 0);
	    dmc->dmc_flags_mask = DMC_FORCE_END_SESS;
	    if (dmf_call(DMC_END_SESSION, dmc))
	    {
		sc0e_0_put(dmc->error.err_code, 0);
	    }
	    bad_status = max(status, bad_status);
	}
    }

    if (scb->scb_sscb.sscb_facility & (1 << DB_GWF_ID))
    {
	gw_rcb = scb->scb_sscb.sscb_gwccb;
	gw_rcb->gwr_session_id = scb->scb_sscb.sscb_gwscb;
	gw_rcb->gwr_type = GWR_CB_TYPE;
	gw_rcb->gwr_length = sizeof(GW_RCB);
	status = gwf_call(GWS_TERM, gw_rcb);
	if (status)
	{
	    error.err_code = gw_rcb->gwr_error.err_code;
	    error.err_data = gw_rcb->gwr_error.err_data;
	    sc0e_0_put(error.err_code, 0);
	    bad_status = max(status, bad_status);
	}
    }

    /* re-enable CSresume()'s from scc_gcomplete so session rundown works */
    scb->scb_sscb.sscb_terminating = 0;

    /* don't dissasoc again if we already did. */

    if (scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID) &&
	(scb->scb_sscb.sscb_flags & SCS_DROPPED_GCA) == 0 )
    {
	status = scs_disassoc(scb->scb_cscb.cscb_assoc_id);
	if (status)
	{
	    sc0e_0_put(status, 0);
	    error.err_code = status;
	}
    }

    if (scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID))
	scc_dispose(scb);	/* trash remainder of messages, if any */
    scb->scb_sscb.sscb_facility = 0;

    /* free up RAAT message buffer, now that scc_dispose() dequeued it */
    if (scb->scb_sscb.sscb_raat_message != NULL)
    {
	raat_msg = (SCS_RAAT_MSG *)scb->scb_sscb.sscb_raat_message;

	while (raat_msg->next)
	{
	    prev_raat_msg = raat_msg;
	    raat_msg = raat_msg->next;
	    status = sc0m_deallocate(0, (PTR *)&prev_raat_msg);
	    if (status)
		err = status;
	}
	status = sc0m_deallocate(0, (PTR *)&raat_msg);
	if (err)
	    status = err;

	if (status)
	{
	    sc0e_0_put(status, 0);
	}
	scb->scb_sscb.sscb_raat_message = NULL;
    }

    /*
    ** Run down list of this session's owned memory
    */

    for (mcb = scb->scb_sscb.sscb_mcb.mcb_sque.sque_next;
	    mcb != &scb->scb_sscb.sscb_mcb;
	    mcb = new_mcb)
    {
	sc0e_put(E_SC0227_MEM_NOT_FREE, 0, 1, 
		 sizeof (mcb->mcb_facility), &mcb->mcb_facility,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0 );

	/* first take it off the session queue, if appropriate */

	new_mcb = mcb->mcb_sque.sque_next;
	if (mcb->mcb_session != DB_NOSESSION)
	{
	    mcb->mcb_sque.sque_next->mcb_sque.sque_prev
		    = mcb->mcb_sque.sque_prev;
	    mcb->mcb_sque.sque_prev->mcb_sque.sque_next
		    = mcb->mcb_sque.sque_next;
	}

	status = sc0m_free_pages(mcb->mcb_page_count, &mcb->mcb_start_address);
	if (status != E_SC_OK)
	{
	    sc0e_0_put(status, 0);
	}
	
	/* now unlink the mcb from the general mcb list and return it */

	mcb->mcb_next->mcb_prev = mcb->mcb_prev;
	mcb->mcb_prev->mcb_next = mcb->mcb_next;
	status = sc0m_deallocate(0, (PTR *) &mcb);
	if (status)
	{
	    sc0e_0_put(status, 0);
	}
    }

    if (scb->scb_sscb.sscb_ics.ics_gw_parms != NULL)
    {
	status = sc0m_deallocate(0, &scb->scb_sscb.sscb_ics.ics_alloc_gw_parms);
	scb->scb_sscb.sscb_ics.ics_gw_parms = NULL;
	if (status)
	{
	    sc0e_0_put(E_SC0224_MEMORY_FREE, 0);
	    sc0e_0_put(status, 0);
	    return(E_DB_SEVERE);
	}
    }

    /* stop leak! */
    if(scb->scb_sscb.sscb_aflags & SCS_ASEM)
        CSr_semaphore( &scb->scb_sscb.sscb_asemaphore );

    if (status)
    {
	sc0e_0_put(E_SC012B_SESSION_TERMINATE, 0);
    }
    if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_SESSION_LOG, NULL, NULL))
    {
	sc0e_0_put(E_SC0234_SESSION_END, 0);
    }
    if ((Sc_main_cb->sc_totrows > MAXI4)
	|| (Sc_main_cb->sc_selcnt > MAXI4))
    {
	i4		avg;

	avg = (Sc_main_cb->sc_selcnt
		    ? Sc_main_cb->sc_totrows / Sc_main_cb->sc_selcnt
		    : 0);
	sc0e_put(E_SC0235_AVGROWS, 0, 2,
		 sizeof(Sc_main_cb->sc_selcnt),
		 (PTR)&Sc_main_cb->sc_selcnt,
		 sizeof(avg),
		 (PTR)&avg,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0);
	Sc_main_cb->sc_totrows= 0;
	Sc_main_cb->sc_selcnt = 0;
    }
			

    if (bad_status)
    	scd_note(bad_status, error.err_code >> 16);
    return(status);
}

/*{
** Name: scs_cut_cleanup - Clean up CUT buffers and child threads
**
** Description:
**	Although the sequencer is supposed to make reasonable effort
**	to shut down queries before terminating a user thread, there are
**	still potentially ways to terminate a thread when part of a
**	query is still running.  Specifically, what we want to worry
**	about here are CUT buffers and threads that could still
**	be outstanding.
**
**	It's possible to simply detach our end from CUT without too
**	much trouble, but that could leave the other (child) thread
**	end running, or just sitting there, possibly holding resources
**	and tables open.  So, what we'll do is query CUT for each of
**	our attached buffers in turn, and signal an error status to
**	each one.  Then, we'll wait until nobody else is attached to
**	that particular CUT buffer, and detach ourselves.  In this
**	way, all child threads ought to have an opportunity to
**	clean themselves up before we continue with the main thread
**	shutdown.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** History:
**	15-Sep-2004 (schka24)
**	    Written.
**	01-Oct-2004 (jenjo02)
**	    Catch cut_event_wait async signals, looping until
**	    the "parent" is really the last connection.
*/

static void
scs_cut_cleanup()
{
    bool parent;			/* Parent-thread flag */
    CUT_RCB rcb;			/* CUT request block */
    DB_ERROR error;			/* Place for CUT error info */
    DB_STATUS status;			/* Usual status thing */
    i4 one;

    /* Loop to find, then detach, a CUT buffer */
    for (;;)
    {
	MEfill(sizeof(CUT_RCB), 0, &rcb);
	status = cut_info(CUT_ATT_BUFS, &rcb, NULL, &error);
	/* "no more" comes thru as INFO or WARN status */
	if (status != E_DB_OK)
	    break;
	parent = (rcb.cut_buf_use & CUT_BUF_PARENT) != 0;

	/* Signal ERROR at the CUT buffers */
	(void) cut_signal_status(&rcb, E_DB_ERROR, &error);

	/* If we were the parent for this CUT buffer, wait for kids
	** to terminate.  (If we're not the parent, the parent might be
	** waiting for us to detach, so waiting would be ill advised!)
	** As long as everyone sets parentage properly, this should take
	** care of the situation where user thread spawns exch thread(s)
	** which spawn grandchild exch threads, ad nauseum.
	*/

	if (parent) do
	{
	    one = 1;
	    status = cut_event_wait(&rcb, CUT_DETACH, (PTR) &one, &error);
	} while ( status && error.err_code == E_CU0204_ASYNC_STATUS );

	/* Force-clear the cut status and detach ourselves from this buf */
	(void) cut_signal_status(&rcb, E_DB_OK, &error);
	(void) cut_detach_buf(&rcb, TRUE, &error);

    } /* for */

    /* Make sure that CUT forgets about our thread entirely */
    status = cut_thread_term(TRUE, &error);
    if (status != E_DB_OK)
    {
	/* After all of our careful work above, this should not happen, but
	** clear all pending statuses and try again.
	*/
	if (error.err_code == E_CU0204_ASYNC_STATUS)
	{
	    (void) cut_signal_status(NULL, E_DB_OK, &error);
	    (void) cut_thread_term(TRUE, &error);
	}
    }

} /* scs_cut_cleanup */

/*{
** Name: scs_icsxlate    - translate all identifiers associated with a session
**
** Description:
**      This routines translates all of the identifiers associated with
**	a session according to the database's translation semantics and
**	how the identifier was expressed (delimited or regular).
**
** Inputs:
**	scb				address of session control block
** Outputs:
**	error				error code from cui_idxlate, if any
**	Returns:
**	    E_DB_OK			Identifiers translated successfully
**	    E_DB_ERROR			Failure when translating an identifier
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-feb-93 (ralph)
**	    Written for DELIMITED IDENTIFIERS
**	31-mar-1993 (ralph)
**	    Call dmf and qef to pass in new session info wrt delim idents.
**	    Initialize ics_cat_own.
**	03-may-1993 (fpang)
**	    Check values of dm_ccb and qe_ccb before using them becuase they
**	    may be null for some type of servers ie, star.
**	29-Jun-1993 (daveb)
**	    cast ics_username to PTR before stuffing into data_address.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	27-jul-1993 (ralph)
**	    Call GWF_ALTER to advise GWF of changed case translation semantics
**	    Use DU_REAL_USER_MIXED to influence case translation of real uesr.
**	09-nov-93 (rblumer)
**	    Fixed bug 55170, where DBAname != username even when user IS the
**	    DBA!!  (happens in FIPS database when have INGRES iidbdb).
**	    The main problem was that the dbaname (dbowner) was getting
**	    translated according to DELIM_ID semantics instead of getting
**	    translated according to REAL_USER_CASE semantics.
**	    So we now use REAL_USER_CASE semantics for the iidbdb.
**	    
**	    But this only fixes the problem when the iidbdb and the target db
**	    have the same case-translation semantics OR when REAL_USER_CASE is
**	    NOT mixed in the target db; for the non-mixed iidbdb/mixed target
**	    db case, will eventually have to make other changes, to either the
**	    CONFIG file or iidatabase catalog.  More on this to come ....
**	23-feb-94 (rblumer)
**	    Changed ics_dbxlate initialization to set the bits for LOWER case 
**	    ids, even though that is the default.  Otherwise, code that looks
**	    at the dbxlate flags must always check for the non-normal cases
**	    and will break if anyone ever tries to check the _L bits.
[@history_template@]...
*/
DB_STATUS
scs_icsxlate(SCD_SCB	*scb ,
	     DB_ERROR	*error )
{
    u_i4	xlate_temp;
    u_i4	l_id;
    char	ownstr[DB_OWN_MAXNAME];
    char	dbstr[DB_DB_MAXNAME + 1];
    u_i4	templen;
    DB_STATUS	status;
    DMC_CB      *dm_ccb;
    QEF_RCB     *qe_ccb;
    QEF_ALT     qef_alt[1];
    GW_RCB	*gw_ccb;

    /*
    ** Now that we know the translation semantics of the target
    ** database, let's translate the identifiers as follows:
    **
    **	if (ics_dbserv & DU_NAME_UPPER)
    **	    translate regular to upper case	(CUI_ID_REG_U)
    **	else
    **	    translate regular to lower case	(default)
    **
    **	if (ics_dbserv & DU_DELIM_UPPER)
    **	    translate delimited to upper case	(CUI_ID_DLM_U)
    **	else if (ics_dbserv & DU_DELIM_MIXED)
    **	    don't translate delimited		(CUI_ID_DLM_M)
    **	else
    **	    translate delimited to lower case 	(default)
    **
    ** Here's the order of identifier translation:
    **
    **  if (ics_dbserv & DU_REAL_USER_MIXED)
    **      translate ics_xrusername into ics_rusername as delimited
    **  else if (ics_dbserv & DU_NAME_UPPER)
    **      translate ics_xrusername into ics_rusername as upper case
    **  else
    **      translate ics_xrusername into ics_rusername as lower case
    **
    **	if (ics_xmap & SCS_XDLM_EUSER)
    **	    translate ics_xeusername into ics_susername as delimited 
    **	else
    **	    translate ics_xeusername into ics_susername as regular
    **
    **	if (ics_xmap & SCS_XDLM_EGRPID)
    **	    translate ics_xegrpid into ics_egrpid as delimited
    **	else
    **	    translate ics_xegrpid into ics_egrpid as regular
    **
    **	if (ics_xmap & SCS_XDLM_EAPLID)
    **	    translate ics_xeaplid into ics_eaplid as delimited
    **	else
    **	    translate ics_xeaplid into ics_eaplid as regular
    **
    **	translate ics_xdbowner into ics_dbowner as delimited
    **
    **	translate ics_dbname in place as regular
    **
    **	what to do about ics_musername and ics_iucode?		@FIX_ME@
    **
    */

    /* NOTE:							@FIX_ME@    XXX
    **   the case-translation flags are currently stored in the SCF
    **   session control block, and other facilities have a pointer to that
    **   field in THEIR session control blocks.
    **
    ** Since the translation semantics are a DATABASE-specific property,
    ** it would make sense to store them in the DATABASE control block instead
    ** of the session control block of SCF or any other facility.
    ** Currently, each session can access only 1 database, so this is not too
    ** urgent; but if we someday allow a session to connect to more than 1
    ** database, it would be better to have each facility look into a database
    ** control block for the translation semantics (instead of a session
    ** control block).						@FIX_ME@    XXX
    ** 			(rblumer  20-apr-94, per comments from bryanp 16-feb-94)
    */

    /*
    ** Set up translation flags for the database
    */
    scb->scb_sscb.sscb_ics.ics_dbxlate = 0x00L;
    if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_NAME_UPPER)
	scb->scb_sscb.sscb_ics.ics_dbxlate |= CUI_ID_REG_U;
    else
	scb->scb_sscb.sscb_ics.ics_dbxlate |= CUI_ID_REG_L;

    if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_DELIM_UPPER)
	scb->scb_sscb.sscb_ics.ics_dbxlate |= CUI_ID_DLM_U;
    else if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_DELIM_MIXED)
	scb->scb_sscb.sscb_ics.ics_dbxlate |= CUI_ID_DLM_M;
    else
	scb->scb_sscb.sscb_ics.ics_dbxlate |= CUI_ID_DLM_L;

    /*
    ** Translate the real userid
    */
    l_id = sizeof(scb->scb_sscb.sscb_ics.ics_xrusername);
    templen = DB_OWN_MAXNAME;

    xlate_temp = CUI_ID_DLM | CUI_ID_NORM | CUI_ID_STRIP;
    if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_REAL_USER_MIXED)
	xlate_temp |= CUI_ID_DLM_M;
    else if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_NAME_UPPER)
	xlate_temp |= CUI_ID_DLM_U;
    else 
	xlate_temp |= CUI_ID_DLM_L;

    status = cui_idxlate((u_char *)&scb->scb_sscb.sscb_ics.ics_xrusername,
			 &l_id, (u_char *)ownstr, &templen, xlate_temp,
			 (u_i4 *)NULL, error);
    if (DB_FAILURE_MACRO(status))
    {
	sc0e_uput(error->err_code, 0, 2,
		 sizeof(ERx("Real user identifier"))-1,
		 (PTR)ERx("Real user identifier"),
		 l_id, (PTR)&scb->scb_sscb.sscb_ics.ics_xrusername,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0 );
	return(status);
    }
    MEmove(templen, (PTR)ownstr, ' ',
		 sizeof(scb->scb_sscb.sscb_ics.ics_rusername),
                 (PTR)&scb->scb_sscb.sscb_ics.ics_rusername);
    /*
    ** Translate the effective user if specified
    */
    if (scb->scb_sscb.sscb_ics.ics_xmap & SCS_XEGRP_SPECIFIED)
    {
	l_id = sizeof(scb->scb_sscb.sscb_ics.ics_xeusername);
	templen = DB_OWN_MAXNAME;
	if (scb->scb_sscb.sscb_ics.ics_xmap & SCS_XDLM_EUSER)
	    xlate_temp = scb->scb_sscb.sscb_ics.ics_dbxlate | CUI_ID_DLM
							    | CUI_ID_NORM
							    | CUI_ID_STRIP;
	else
	    xlate_temp = scb->scb_sscb.sscb_ics.ics_dbxlate | CUI_ID_REG
							    | CUI_ID_NORM
							    | CUI_ID_STRIP;
	status = cui_idxlate((u_char *)&scb->scb_sscb.sscb_ics.ics_xeusername,
			     &l_id, (u_char *)ownstr, &templen, xlate_temp,
			     (u_i4 *)NULL, error);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_uput(error->err_code, 0, 2,
		     sizeof(ERx("Effective user identifier"))-1,
		     (PTR)ERx("Effective user identifier"),
		     l_id, (PTR)&scb->scb_sscb.sscb_ics.ics_xeusername,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    return(status);
	}
	MEmove(templen, (PTR)ownstr, ' ',
		     sizeof(scb->scb_sscb.sscb_ics.ics_susername),
                     (PTR)&scb->scb_sscb.sscb_ics.ics_susername);
    }
    else
    {
	/*
	** Effective user was not specified; 
	** copy in the translated real user name instead.
	*/
	MEcopy((PTR) &scb->scb_sscb.sscb_ics.ics_rusername,
		sizeof(scb->scb_sscb.sscb_ics.ics_susername),
		(PTR) &scb->scb_sscb.sscb_ics.ics_susername);
    }
    /*
    ** Translate the group identifier if specified
    */
    if (MEcmp((PTR)&scb->scb_sscb.sscb_ics.ics_xegrpid,
		(PTR)DB_ABSENT_NAME,
		sizeof(scb->scb_sscb.sscb_ics.ics_xegrpid)
	     ) != 0)
    {
	l_id = sizeof(scb->scb_sscb.sscb_ics.ics_xegrpid);
	templen = DB_OWN_MAXNAME;
	if (scb->scb_sscb.sscb_ics.ics_xmap & SCS_XDLM_EGRPID)
	    xlate_temp = scb->scb_sscb.sscb_ics.ics_dbxlate | CUI_ID_DLM
							    | CUI_ID_NORM
							    | CUI_ID_STRIP;
	else
	    xlate_temp = scb->scb_sscb.sscb_ics.ics_dbxlate | CUI_ID_REG
							    | CUI_ID_NORM
							    | CUI_ID_STRIP;
	status = cui_idxlate((u_char *)&scb->scb_sscb.sscb_ics.ics_xegrpid,
			     &l_id, (u_char *)ownstr, &templen, xlate_temp,
			     (u_i4 *)NULL, error);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_uput(error->err_code, 0, 2,
		     sizeof(ERx("Group identifier"))-1,
		     (PTR)ERx("Group identifier"),
		     l_id, (PTR)&scb->scb_sscb.sscb_ics.ics_xegrpid,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    return(status);
	}
	MEmove(templen, (PTR)ownstr, ' ',
		 sizeof(scb->scb_sscb.sscb_ics.ics_egrpid),
                 (PTR)&scb->scb_sscb.sscb_ics.ics_egrpid);
    }
    /*
    ** Translate the role identifier if specified
    */
    if (MEcmp((PTR)&scb->scb_sscb.sscb_ics.ics_xeaplid,
		(PTR)DB_ABSENT_NAME,
		sizeof(scb->scb_sscb.sscb_ics.ics_xeaplid)
	     ) != 0)
    {
	l_id = sizeof(scb->scb_sscb.sscb_ics.ics_xeaplid);
	templen = DB_OWN_MAXNAME;
	if (scb->scb_sscb.sscb_ics.ics_xmap & SCS_XDLM_EAPLID)
	    xlate_temp = scb->scb_sscb.sscb_ics.ics_dbxlate | CUI_ID_DLM
							    | CUI_ID_NORM
							    | CUI_ID_STRIP;
	else
	    xlate_temp = scb->scb_sscb.sscb_ics.ics_dbxlate | CUI_ID_REG
							    | CUI_ID_NORM
							    | CUI_ID_STRIP;
	status = cui_idxlate((u_char *)&scb->scb_sscb.sscb_ics.ics_xeaplid,
			     &l_id, (u_char *)ownstr, &templen, xlate_temp,
			     (u_i4 *)NULL, error);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0e_uput(error->err_code, 0, 2,
		     sizeof(ERx("Role identifier"))-1,
		     (PTR)ERx("Role identifier"),
		     l_id, (PTR)&scb->scb_sscb.sscb_ics.ics_xeaplid,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    return(status);
	}
	MEmove(templen, (PTR)ownstr, ' ',
		 sizeof(scb->scb_sscb.sscb_ics.ics_eaplid),
                 (PTR)&scb->scb_sscb.sscb_ics.ics_eaplid);
    }

    /*
    ** Translate the database owner
    ** according to REAL_USER_CASE for the target database.
    **
    ** (Note that the database owner name (DBAname) is not currently stored
    **  in untranslated form in the iidatabase tuple, so if real-user-case is
    **  UPPER or LOWER in the iidbdb and MIXED in the target database,
    **  this translation won't work for mixed-case user names.
    **  But it will work in all other combinations of iidbdb/target db case
    **  semantics, or if real_user_case is mixed in all databases.
    **  
    **  Note also that this doesn't take into account the fact that the
    **  DBAname could actually be a delimited identifier specified with the
    **  createdb -u flag; to support that we would have to add a bit to the
    **  iidatabase tuple to tell us whether to use REAL/REGID/DELIMID case
    **  to do the translation.)
    */
    l_id = sizeof(scb->scb_sscb.sscb_ics.ics_xdbowner);
    templen = DB_OWN_MAXNAME;
    xlate_temp = CUI_ID_DLM | CUI_ID_NORM | CUI_ID_STRIP;

    if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_REAL_USER_MIXED)
	xlate_temp |= CUI_ID_DLM_M;
    else if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_NAME_UPPER)
	xlate_temp |= CUI_ID_DLM_U;
    else
	xlate_temp |= CUI_ID_DLM_L;

    status = cui_idxlate((u_char *)&scb->scb_sscb.sscb_ics.ics_xdbowner,
			 &l_id, (u_char *)ownstr, &templen, xlate_temp,
			 (u_i4 *)NULL, error);
    if (DB_FAILURE_MACRO(status))
    {
	sc0e_uput(error->err_code, 0, 2,
		 sizeof(ERx("DBA name"))-1,
		 (PTR)ERx("DBA name"),
		 l_id, (PTR)&scb->scb_sscb.sscb_ics.ics_xdbowner,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0 );
	return(status);
    }
    MEmove(templen, (PTR)ownstr, ' ',
		 sizeof(scb->scb_sscb.sscb_ics.ics_dbowner),
                 (PTR)&scb->scb_sscb.sscb_ics.ics_dbowner);

    /*
    ** Translate the database name if specified
    */
    if (scb->scb_sscb.sscb_stype != SCS_SMONITOR)
    {
	l_id = sizeof(scb->scb_sscb.sscb_ics.ics_dbname);
	templen = DB_DB_MAXNAME;
	/*
	** Database names are case-insensitive;
	** translate to lower case
	*/
	xlate_temp = CUI_ID_REG | CUI_ID_REG_L | CUI_ID_NORM | CUI_ID_STRIP;

    	status = cui_idxlate((u_char *)&scb->scb_sscb.sscb_ics.ics_dbname,
			     &l_id, (u_char *)dbstr, &templen, xlate_temp,
			     (u_i4 *)NULL, error);
    	if (DB_FAILURE_MACRO(status))
    	{
	    sc0e_uput(error->err_code, 0, 2,
		     sizeof(ERx("Database name"))-1,
		     (PTR)ERx("Database name"),
		     l_id, (PTR)&scb->scb_sscb.sscb_ics.ics_dbname,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0,
		      0, (PTR)0);
	    return(status);
    	}
    	MEmove(templen, (PTR)dbstr, ' ',
		     sizeof(scb->scb_sscb.sscb_ics.ics_dbname),
                     (PTR)&scb->scb_sscb.sscb_ics.ics_dbname);
    }

    /*
    ** Set the catalog owner name
    */
    l_id = sizeof(DB_INGRES_NAME)-1;
    templen = DB_OWN_MAXNAME;
    xlate_temp = scb->scb_sscb.sscb_ics.ics_dbxlate | CUI_ID_REG
						    | CUI_ID_NORM
						    | CUI_ID_STRIP;
    status = cui_idxlate((u_char *)DB_INGRES_NAME,
			 &l_id, (u_char *)ownstr, &templen, xlate_temp,
			 (u_i4 *)NULL, error);
    if (DB_FAILURE_MACRO(status))
    {
	sc0e_uput(error->err_code, 0, 2,
		 sizeof(ERx("Catalog owner"))-1,
		 (PTR)ERx("Catalog owner"),
		 l_id, (PTR)DB_INGRES_NAME,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0,
		  0, (PTR)0 );
	return(status);
    }
    MEmove(templen, (PTR)ownstr, ' ',
		 sizeof(scb->scb_sscb.sscb_ics.ics_cat_owner),
		 (PTR)&scb->scb_sscb.sscb_ics.ics_cat_owner);

    /*
    ** Set "special" user flags
    */
    if (MEcmp((PTR)scb->scb_sscb.sscb_ics.ics_eusername,
	      (PTR)&scb->scb_sscb.sscb_ics.ics_cat_owner,
              sizeof(*scb->scb_sscb.sscb_ics.ics_eusername)) == 0)
	scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_EINGRES;

    /* if we really want to make sure a user with a name that differs
    ** only in case with the DBA's name cannot masquerade as the DBA,
    ** we will have to store the dbaname in untranslated form somewhere
    ** (in iidatabase?) and use xdbowner name here instead of dbowner
    **				(rblumer  11/09/93)
    */
    if (MEcmp((PTR)&scb->scb_sscb.sscb_ics.ics_dbowner,
	      (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
	      sizeof(scb->scb_sscb.sscb_ics.ics_dbowner)) == 0)
	scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_RDBA;

    /*
    ** PSF is not up yet; will pass values in when we bring it up
    */

    /*
    ** Tell DMF about the revised user name
    */
    if ((dm_ccb = scb->scb_sscb.sscb_dmccb) != (DMC_CB *)NULL)
    {
	dm_ccb->type = DMC_CONTROL_CB;
	dm_ccb->length = sizeof(DMC_CB);
	dm_ccb->dmc_session_id = (PTR) scb->cs_scb.cs_self;
	dm_ccb->dmc_op_type = DMC_SESSION_OP;
	dm_ccb->dmc_sl_scope = DMC_SL_SESSION;
	dm_ccb->dmc_flags_mask = 0L;
	dm_ccb->dmc_name.data_in_size = sizeof(DB_OWN_NAME);
	dm_ccb->dmc_name.data_address = (PTR)scb->scb_sscb.sscb_ics.ics_eusername;
	status = dmf_call(DMC_RESET_EFF_USER_ID, dm_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);                                         /*@FIX_ME@*/
	}
    }

    /*
    ** Tell QEF about the revised user name
    */
    /*@FIX_ME@ Can this be removed from here and placed after the 
    ** second call in scs_dbdb_info() with the other QEC_ALTER calls?
    */
    if ((qe_ccb = scb->scb_sscb.sscb_qeccb) != (QEF_RCB *)NULL)
    {
	qe_ccb->qef_asize = 1;
	qe_ccb->qef_alt = qef_alt;
	qef_alt[0].alt_code = QEC_EFF_USR_ID;
	qef_alt[0].alt_value.alt_own_name = scb->scb_sscb.sscb_ics.ics_eusername;
	status = qef_call(QEC_ALTER, qe_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);                                         /*@FIX_ME@*/
	}
    }

    /*
    ** Tell GWF about the revised case translation semantics
    */
    if ((gw_ccb = scb->scb_sscb.sscb_gwccb) != (GW_RCB *)NULL)
    {
	gw_ccb->gwr_scf_session_id = scb->cs_scb.cs_self;
	gw_ccb->gwr_type = GWR_CB_TYPE;
	gw_ccb->gwr_length = sizeof(GW_RCB);
	gw_ccb->gwr_session_id = scb->scb_sscb.sscb_gwscb;
	gw_ccb->gwr_dbxlate = &scb->scb_sscb.sscb_ics.ics_dbxlate;
	gw_ccb->gwr_cat_owner = &scb->scb_sscb.sscb_ics.ics_cat_owner;
	status = gwf_call(GWS_ALTER, gw_ccb);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);                                         /*@FIX_ME@*/
	}
    }

    return(E_DB_OK);
}


/*{
** Name: scs_dbdb_info	- fetch information about the user & database
**
** Description:
**      This routine searches the dbdb for information about the user and 
**      the database for which the session is intended.  If all criteria match 
**      then the database is opened for the session.  If not, then the
**      appropriate error is returned.
**
** Inputs:
**      scb                             session control block for the session
**      dmc                             pointer to a dmc to use
**      error                           pointer to a DB_ERROR struct to fill
**
** Outputs:
[@PARAM_DESCR@]...
**	Returns:
**	    {@return_description@}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-Aug-1986 (fred)
**          Kludged for s/u integration
**      25-Mar-1987 (fred)
**          Completed.
**      01-Apr-1987 (fred)
**          Added private database support;  built transformation
**          from DMF openddb errors into Ingres errors.
**      09-Mar-1989 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    scs_dbdb_info() verifies existence of application_id and group
**	    scs_dbdb_info() copies encrypted applid password to SCS_ICS
**	10-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    scs_dbdb_info() reads iidbpriv to set session's database privileges;
**	    scs_dbdb_info() sets the session's default group-id if none given.
**	15-may-1989 (rogerk)
**	    Added check for TRAN_QUOTA_EXCEEDED.
**	30-may-1989 (ralph)
**	    GRANT Enhancements, Phase 2c:
**	    Don't validate group-id, role-id, or role password for super users.
**	29-jun-1989 (ralph)
**	    CONVERSION:  scs_dbdb_info() checks dbcmptlvl, dbcmptminor
**	31-jul-1989 (sandyh)
**	    Added '$ingres' as valid user when accessing private databases.
**	    This is primarily for FE utilities that run in this mode.
**      05-jul-89 (jennifer)
**          Added code to initialize audit state if not already done.
**	20-aug-1989 (ralph)
**	    Reset DMC_CVCFG if transitory iidbdb open
**	11-sep-1989 (ralph)
**	    B1 Enhancements:
**	    Remove installation defaults from dbpriv calculation.
**	    Add dbprivs granted on all databases to dbpriv calculation.
**	    Use iidbpriv to determine access privileges rather than iidbaccess.
**	16-jan-90 (ralph)
**	    Verify user password.
**	30-jan-90 (ralph)
**	    Pass user status bits to QEF via qec_alter.
**	08-aug-90 (ralph)
**	    Initialize server auditing state (scs_dbdb_info).
**	    Add support for DB_ADMIN database privilege.
**	    Allow unlimited dbprivs if appl code specified.
**	    Alter DMF in scs_dbdb_info if user has AUDIT_ALL privs (bug 20659)
**	11-dec-90 (ralph)
**	    Restrict iidbdb creation to 'ingres' in xORANGE environment only
**	21-may-91 (andre)
**	    renamed some fields in SCS_ICS to make it easier to determine by the
**	    name whether the field contains info pertaining to the system (also
**	    known as "Real"), Session ("he who invoked us"), or the Effective
**	    identifier:
**		ics_ustat     --> ics_rustat	  real (system) user status bits
**		ics_estat     --> ics_sustat      session user status bits
**		ics_username  --> ics_eusername   effective user id (at session
**						  startup it is the same as
**						  session user id)
**		ics_ucode     --> ics_sucode	  copy of the session user id
**		ics_grpid     --> ics_egrpid      effective group id
**		ics_aplid     --> ics_eaplid      effective role id
**		ics_apass     --> ics_sapass      password for session role id
**	16-nov-92 (robf)
**	    When building the "default" iidbdb info (during CREATEDB iidbdb)
**	    make sure the SCF info (ics_rustat) is set appropriately, including
**	    SECURITY priv. (Note CREATEDB will have already verified the
**	    user has SECURITY priv before attempting the connect to the
**	    server). Without this some privileged commands issued later in
**	    the session will fail.
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**	    
**	    As a part of desupporting SET GROUP/ROLE, ics_sapass was renamed to
**	    ics_eapass
**	    
**	    the following fields in SCS_ICS have been renamed:
**		ics_sucode	--> ics_iucode
**		ics_sustat	->  ics_iustat
**	03-feb-93 (andre)
**	    pass the database id ands the unique database id to QEF via
**	    QEC_ALTER
**	01-mar-1993 (markg)
**	    Fixed bug where errors getting returned by sc0a_write_audit
**	    were not getting handled correctly. Also fixed problem where
**	    the auditing of role execution could report incorrect
**	    success/failure status.
**	15-mar-1993 (ralph)
**	    DELIM_IDENT:
**	    Copy DU_DATABASE.du_dbservice into SCS_ICS.ics_dbserv
**	    to establish session's case translation semantics.
**	    Fetch iidbdb's DU_DATABASE tuple into dynamic buffer and
**	    anchor in SC_MAIN_CB.sc_dbdb_dbtuple.
**	    Cast temptuple to PTR before stuffing in sc_dbdb_tuple.
**	2-Jul-1993 (daveb)
**	    prototyped, use proper PTR for allocation
**	27-jul-1993 (ralph)
**	    DELIM_IDENT:
**	    Set case translation mask and dbservice value when creating
**	    the iidbdb database.  These values are passed in via the
**	    -G flag, at least until we specify a more formal interface.
**	29-sep-93 (jrb)
**          Set DMC_ADMIN_DB in dmc_flags_mask to tell DMF when we open the
**          iidbdb just for administrative purposes (e.g. when looking up
**	    user info).
**	22-oct-93 (robf)
**          Add connect_limit, idle_limit, priority_limit, table_statistics
**	03-nov-93 (stephenb)
**	    Change if block when checking for the "-u" flag so we reject if
**	    user does NOT have SCS_USR_RDBA, not if they do.
**	 5-nov-93 (robf)
**          Rework password checks, now done in scs_initiate.
**	09-nov-93 (rblumer)
**          don't overwrite case-translated dbowner field if the target db is
**          the iidbdb.  Otherwise dbmsinfo(dba) is wrong case in FIPS iidbdb.
**      17-feb-1994 (rachael) Bug 55942,58205
**          Remove changes made in 15-mar-1993 integration
**          which noted if the target database (to be opened) was the iidbdb,
**          and if so, didn't open it the 2nd time. This could be done because 
**          at session initialization the iidbdb is opened first, then
**          the target database is opened.  It was at this second opening
**          that flags such as -l (locking) or +U were used.
**          Since the iidbdb wasn't being opened the 2nd time, these flags
**          were being ignored if the target database was the iidbdb.
**	22-feb-94 (robf)
**          Get session role information.
**	3-mar-94 (robf)
**	    Rework the criteria for when application codes may be used.
**	    Previously any user could do (e.g) -A10 and get all application
**	    abilities. We now restrict that so only specified users may
**	    may use each application code, in most cases this is the DBA since
**          the operations are database-maintenance related typically.
**	    Note that this indirectly addresses the problem of application
**	    codes given -u privilege by restricting them to users who have
**	    -u anyway, like DBAs. Long term the whole issue of application
**	    codes should be readdressed.
**	08-mar-94 (rblumer)
**          B60387: in order to maintain the correct case-translation semantics
**          for the iidbdb, we need to get the du_dbservice value from the
**          on-disk version of the iidatabase tuple into the cached SCV_DBCB
**          in scf's database list (sc_kdbl queue).  WHY? The SCV_DBCB is
**          created using the hard-coded, compile-time version of the iidbdb
**          tuple (called dbdb_dbtuple), and this can be different than the
**          on-disk version if the iidbdb has non-default case-translation
**          semantics (for instance, a FIPS iidbdb).  Currently the differences
**          are in the du_dbservice field and the du_own (owner-name) field.
**          We also need to copy in the correct versions of these fields.  The
**          easiest way to do this was to copy the tuple fetched into
**          sc_dbdb_dbtuple back into the dbcb for the iidbdb.  This also makes
**          sure that we only do this copy once, since sc_dbdb_dbtuple is only
**          fetched the 1st time we open the iidbdb.
**          The copy is only needed for the 1st open of the iidbdb because,
**          for subsequent opens of the iidbdb, we can pass in the cached
**          from-disk tuple instead of using the hard-coded compile-time tuple.
**          Also, there is no need to try finding the 'iidbdb' iidatabase tuple
**          twice (once lower-cased and once upper-cased), since database names
**          are always lower-cased.  So I simplified that code by removing the
**          translation loop.
**	18-mar-94 (robf)
**          Security audit failed access permission on a database.
**	24-mar-94 (robf)
**          Initialize dbpriv consistently.
**	28-mar-94 (mikem)
**	   bug #59865
**	   Upgradedb of the iidbdb from 6.4 to 6.5 was failing in the dmf
**	   automatic upgrade of system catalog portion.  During upgradedb
**	   of iidbdb you must get an exclusive lock, even during the initial
**	   "qry the iidbdb" about startup information phase.  Fixed 
**	   scs_dbdb_info() to get an exclusive lock on the iidbdb if the
**	   server is in the process of upgrading the iidbdb.
**	30-mar-94 (robf)
**          When calculating dbprivs use values being built in dbpriv
**	    not those in the scb, since they are not configured yet.
**	18-apr-1994 (rog)
**	   If we are opening a 6.4 iidbdb to convert, i.e., DMC_CVCFG is set,
**	   then don't try to check iidatabase to see whether it is a FIPS
**	   database.  Otherwise, we will fill in the dbdb_dbtuple with the
**	   wrong length data.  (Bug 62057)
**	20-apr-94 (robf)
**          Make sure initial privilege are passed onto QEF
**	    since it isn't done elsewhere.
**	12-jul-94 (robf)
**          Update handling for upgradedb, continue processing when
**          converting is case when iirolegrant/iisecalarm don't yet
**          exist, and initialize user status when converting.
**      01-jan-95 (lawst01)  Bug 66466
**         Set 'No Roles' flag in the SCB->sc_secflags word to inhibit
**         access of the iirolegrant table - it doesn't exits in 6.4
**	4-may-1995 (dougb) Bug 68425
**	    Fix for bug #68425:  UPGRADEDB fails when converting a 6.4 iidbdb.
**	    Error is E_US0061, caused by a failed test in scs_usr_expired.
**	    Fix is to initialize the correct expdate field (change to robf's
**	    fix of 12-jul-94).  Without this change, ust.du_expdate may reach
**	    the copy to ssdb_ics.ics_expdate without being set.  (ics_expdate
**	    was being initialized properly for the upgrade, then overwritten
**	    with the "dirty" value.)
**	    Problem should be generic, but VMS seems to have hit it first.
**	7-nov-1995 (angusm)
**	    Change to prevent E_US001A when databases have delimited id 
**	    location containing embedded spaces.
**	24-apr-1996 (somsa01) Bug 75719
**	    If the effective user is different from the real user (using the
**	    -u flag), we need to reset the effective default group to the
**	    effective user's default group.
**	16-aug-96 (pchang)
**	    When obtaining database attributes (DU_DATABASE) from iidatabase 
**	    into sc_dbdb_dbtuple, a pointer to a temporary tuple is used in the 
**	    QEF calls.  However, a failure to take into account the generic 
**	    object header SC_OBJECT, when allocating memory for this temptuple 
**	    caused the server to SEGV in sc0m_deallocate(), should the call to 
**	    QEU_GET or QEU_CLOSE fail. (B78133)
**	    Fixed bug b78133.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**      03-jun-1996 (canor01)
**          OS-threads version of LOingpath no longer needs protection by
**          sc_loi_semaphore.
** 	16-may-1997 (wonst02)
** 	    Changes for readonly databases.
**	19-aug-1997 (nanpr01)
**	    FIPS database cannot be upgraded to 2.0.
**	19-Jan-2001 (jenjo02)
**	    Don't call SXF if not enabled for this session.
**	29-may-2001 (hayke02)
**	    Ensure DBA_W4GLRUN_LIC_CHK and DBA_W4GLDEV_LIC_CHK - OR server
**	    side license application codes - do not cause the session to
**	    get unlimited database privileges. This change fixes bug 104714.
**	3-Feb-2006 (kschendel)
**	    We're throwing so many copies of the dbdb iidatabase tuple
**	    around that upgradedb can sometimes get confused;  if the
**	    actual iidbdb upgrade session hangs around internally long
**	    enough, a reconnect grabs the old tuple with the old version
**	    and gives a bogus US0042 error.
**	    Remove unused "user-only" parameter (was always FALSE), clean
**	    up code a bit.
**	22-Apr-2009 (wanfr01)
**	    Bug 121699
**	    The dbprivs that have been loaded into dbpriv_tup are being
**	    overwritten with the default privileges.  This causes some privs
**	    set explicity for roles and groups to be lost.
**	26-Oct-2009 (kschendel)
**	    Temp (?) fix for new cmptlvl being less than all the old ones.
**	12-Nov-2009 (kschendel) SIR 122882
**	    dbcmptlvl is now an integer.
**	22-Apr-2010 (kschendel) SIR 123485
**	    Remove a couple bad deallocs of tmptupmem, found by accident.
**      09-aug-2010 (maspa05) b123189, b123960
**          Set DMC2_READONLYDB for true readonly database (DU_RDONLY) so
**          we can tell the difference between a readonlydb and one opened
**          for read-only access
*/
DB_STATUS
scs_dbdb_info(SCD_SCB *scb ,
               DMC_CB *dmc ,
               DB_ERROR *error )
{
    i4			status;
    i4				found_hard_way = 1;
    i4				xact = 0;
    i4				opened = 0;
    bool			db_open = FALSE;
    i4			save_lock_mode;
    i4			save_dmf_flags;
    i4				i;
    char			c;
    char			*dbplace;
    SCV_DBCB			*dbcb;
    SCV_DBCB			*dbdb_dbcb;
    DMR_ATTR_ENTRY		*key_ptr[3];
    SCV_LOC			location;
    LOCATION			dbloc;
    char			loc_buf[MAX_LOC];
    QEU_CB			qeu;
    DMR_ATTR_ENTRY		key[3];
    QEF_DATA			data;
    DU_DATABASE			dbt;
    DU_LOCATIONS		lct;
    DU_USER			ust;
    QEF_RCB			qef_cb;
    QEF_ALT			qef_alt[7];
    DB_USERGROUP		group_tup;
    DB_PRIVILEGES		dbpriv_tup;
    bool			grpid_specified = FALSE;
    bool			aplid_specified = FALSE;
    bool			grpid_failure   = FALSE;
    bool			aplid_failure   = FALSE;
    bool			rupass_failure  = FALSE;
    SXF_ACCESS			access;
    DB_STATUS			local_status;
    i4			is_dbdb;
    SXF_RCB			*sxf_cb;
    bool			invalid_user=FALSE;
    DB_ERROR			local_error;
    bool			all_dbpriv=FALSE;
    bool			got_role_dbpriv=FALSE;
    SCS_DBPRIV			roledbpriv;
    SCS_DBPRIV			dbpriv;
    SCS_DBPRIV			tmpdbpriv;
    bool			user_flag;
    bool			dba_only_app;
    bool			special_uflag_usage;

    key_ptr[0] = &key[0];
    key_ptr[1] = &key[1];
    key_ptr[2] = &key[2];
    qeu.qeu_mask = 0;
    /*
    ** Set database privileges to unrestricted values.
    ** This will be adjusted later on for non-super, non-dba users.
    */

    is_dbdb =  (MEcmp( (PTR)&scb->scb_sscb.sscb_ics.ics_dbname,
		(PTR)DB_DBDB_NAME, 
		sizeof(scb->scb_sscb.sscb_ics.ics_dbname)) == 0);

    scs_init_dbpriv(scb, &dbpriv, TRUE);
    scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_DBPR_NOLIMIT;

    if ((scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CREATE_DBDBF1) || !is_dbdb)
    {
	for (;;)
	{
	    DU_DATABASE *cur_dbtuple;	/* iidbdb iidatabase tuple */

	    save_lock_mode = scb->scb_sscb.sscb_ics.ics_lock_mode;

	    scb->scb_sscb.sscb_ics.ics_lock_mode = DMC_L_SHARE;

	    save_dmf_flags = scb->scb_sscb.sscb_ics.ics_dmf_flags;

	    /* 
	    ** Always turned off the DMC_NLK_SESS flag if just getting
	    ** information from the IIDBDB. 
	    */

	    scb->scb_sscb.sscb_ics.ics_dmf_flags &= ~DMC_NLK_SESS;

	    if (scb->scb_sscb.sscb_ics.ics_appl_code == DBA_PRETEND_VFDB)
	    {
		/* If the FE is VERIFYDB and it wants to pretend that
		** the database is consistent then turn off the PRETEND
		** flag in the DMF cb while we enter the IIDBDB, unless the 
		** desired database is the IIDBDB.
		*/
		if (is_dbdb)
		{
		    scb->scb_sscb.sscb_ics.ics_dmf_flags &= ~DMC_PRETEND;
		}
	    }
	    else if (scb->scb_sscb.sscb_ics.ics_appl_code == DBA_FORCE_VFDB)
	    {
		/* If the FE is VERIFYDB and it wants to force the
		** database to be consistent, then turn this bit off while
		** we look through the IIDBDB, unless the target is
		** the iidbdb
		*/
		scb->scb_sscb.sscb_ics.ics_dmf_flags &= ~DMC_MAKE_CONSISTENT;

		/* If VERIFYDB is forcing the IIDBDB to be consistent, then
		** turn on the pretend bit while we do the initial lookup.
		*/
		if (is_dbdb)
		{
		        scb->scb_sscb.sscb_ics.ics_dmf_flags |= DMC_PRETEND;
		}
	    }
	    else if ((scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CONVERT) &&
		     is_dbdb)
	    {
		/* if upgrading the iidbdb the database must be opened 
		** exclusively, even during the preliminary "get info from the
		** dbdb" phase, so that automatic conversion of catalogs done
		** as part of the open is only executed while holding an
		** exclusive database lock.
		*/
		scb->scb_sscb.sscb_ics.ics_lock_mode = DMC_L_EXCLUSIVE;
	    }

	    /*
	    **	If this is a transitory iidbdb open, do not allow
	    **  config file conversion.
	    */

	    if ((scb->scb_sscb.sscb_ics.ics_dmf_flags & DMC_CVCFG) &&
		!is_dbdb
	       )
	    {
		scb->scb_sscb.sscb_ics.ics_dmf_flags &= ~DMC_CVCFG;
	    }

	    /* if the sc_dbdb_dbtuple has been initialized,
	    ** use it instead of the hard-coded dbdb_dbtuple,
	    ** since the real iidbdb dbtuple may have different
	    ** case-translation semantics than the hard-coded one
	    */
	    status = scs_dbopen((DB_DB_NAME  *)DB_DBDB_NAME,
				(DB_DB_OWNER *)DB_INGRES_NAME,
				(SCV_LOC *) Sc_main_cb->sc_dbdb_loc,
				scb,
				error,
				0,
				dmc,
				(Sc_main_cb->sc_dbdb_dbtuple ?
				   (DU_DATABASE *)Sc_main_cb->sc_dbdb_dbtuple :
				   (DU_DATABASE *)&dbdb_dbtuple), 
				&dbdb_dbcb);

	    scb->scb_sscb.sscb_ics.ics_lock_mode = save_lock_mode;
	    if (status)
	    {
		if (error->err_code == E_DM004C_LOCK_RESOURCE_BUSY)
		{
		    qeu.error.err_code = E_US0073_IIDBDB_LOCKED;
		}
		else if (error->err_code == E_DM0041_DB_QUOTA_EXCEEDED)
		{
		    sc0e_0_put(E_US0048_DB_QUOTA_EXCEEDED, 0);
		    sc0e_0_put(error->err_code, 0);
		    qeu.error.err_code = E_US0048_DB_QUOTA_EXCEEDED;
		}
		else if (error->err_code == E_DM004B_LOCK_QUOTA_EXCEEDED)
		{
		    sc0e_0_put(E_US1260_IIDBDB_NO_LOCKS, 0);
		    sc0e_0_put(error->err_code, 0);
		    qeu.error.err_code = E_US1260_IIDBDB_NO_LOCKS;
		}
		else if (error->err_code == E_DM0062_TRAN_QUOTA_EXCEEDED)
		{
		    sc0e_0_put(E_US1271_4721_IIDBDB_TRAN_LIMIT, 0);
		    sc0e_0_put(error->err_code, 0);
		    qeu.error.err_code = E_US1271_4721_IIDBDB_TRAN_LIMIT;
		}
		else if (error->err_code == E_DM0113_DB_INCOMPATABLE)
		{
		    if (MEcmp((PTR)&scb->scb_sscb.sscb_ics.ics_dbname, 
		    	  (PTR)DB_DBDB_NAME, sizeof (DB_DBDB_NAME)) == 0)
		    {
			sc0e_0_put(E_US0008_CONVERT_DB, 0);
			sc0e_0_put(error->err_code, 0);
			qeu.error.err_code = E_US0008_CONVERT_DB;
		    }
		    else
		    {
			sc0e_0_put(E_US0044_CONVERT_DBDB, 0);
			sc0e_0_put(error->err_code, 0);
			qeu.error.err_code = E_US0044_CONVERT_DBDB;
		    }
		}
		else
		{
		    sc0e_0_put(E_US002B_DBDB_OPEN, 0);
		    sc0e_0_put(error->err_code, 0);
		    qeu.error.err_code = E_US002B_DBDB_OPEN;
		}
		break;
	    }
	    /*
	    **  Now the dbdb is open we  need to tell SXF we are accessing it
	    */

	    if ( scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID) )
	    {
		sxf_cb = scb->scb_sscb.sscb_sxccb;
		sxf_cb->sxf_length = sizeof(SXF_RCB);
		sxf_cb->sxf_cb_type = SXFRCB_CB;
		sxf_cb->sxf_sess_id = scb->cs_scb.cs_self;
		sxf_cb->sxf_alter = SXF_X_DBNAME;
		sxf_cb->sxf_db_name = (DB_DB_NAME *)DB_DBDB_NAME;
		status=sxf_call(SXC_ALTER_SESSION, sxf_cb);
		if(status!=E_DB_OK)
		{
		    sc0e_0_put(sxf_cb->sxf_error.err_code, 0);
		    qeu.error.err_code = E_SC0123_SESSION_INITIATE;
		    break;
		}
	    }

	    db_open = TRUE;

	    qeu.qeu_eflag = QEF_INTERNAL;
	    qeu.qeu_db_id = (PTR) scb->scb_sscb.sscb_ics.ics_opendb_id;
	    qeu.qeu_d_id = scb->cs_scb.cs_self;
	    qeu.qeu_flag = QEF_READONLY;
	    status = qef_call(QEU_BTRAN, &qeu);
	    if (status)
	    {
		if (qeu.error.err_code == E_QE0034_LOCK_QUOTA_EXCEEDED)
		{
		    sc0e_0_put(E_US1260_IIDBDB_NO_LOCKS, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_US1260_IIDBDB_NO_LOCKS;
		}
		else if (qeu.error.err_code == E_QE0070_TRAN_QUOTA_EXCEEDED)
		{
		    sc0e_0_put(E_US1271_4721_IIDBDB_TRAN_LIMIT, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_US1271_4721_IIDBDB_TRAN_LIMIT;
		}
		else if (qeu.error.err_code == E_QE0099_DB_INCONSISTENT)
		{
		    sc0e_0_put(E_US002C_DBDB_UNAVAILABLE, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_US002C_DBDB_UNAVAILABLE;
		}
		else
		{
		    sc0e_0_put(E_SC0206_CANNOT_PROCESS, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_SC0206_CANNOT_PROCESS;
		}
		break;
	    }

	    xact = 1;	

	    status = E_DB_OK;

	    qeu.qeu_getnext = QEU_REPO;
	    data.dt_next = (QEF_DATA *) 0;
	    qeu.qeu_output = &data;
	    qeu.qeu_count = 1;
	    qeu.qeu_key = (DMR_ATTR_ENTRY **) key_ptr;
	    qeu.qeu_qual = 0;
	    qeu.qeu_qarg = 0;
	    qeu.qeu_lk_mode = DMT_IS;
	    qeu.qeu_access_mode = DMT_A_READ;


	    /*
	    ** We need to get the real information associated with
	    ** the iidbdb in order to know the case translation semantics
	    ** and security label.
	    ** If we're upgrading, don't read iidbdb's iidatabase row.
	    ** We ought to be upgrading the iidbdb (although we deliberately
	    ** do not check), and the iidatabase row may well be in some
	    ** obsolete format.  Instead, rely on the hardwired iidatabase
	    ** tuple until we get here with a non-upgrade open.
	    ** (More on this below.)
	    **
	    ** The common case is of course that we have an up-to-date,
	    ** cached iidbdb iidatabase tuple, if so we skip all this.
	    */

	    if (Sc_main_cb->sc_dbdb_dbtuple != NULL)
	    {
		cur_dbtuple = (DU_DATABASE *) Sc_main_cb->sc_dbdb_dbtuple;
	    }
	    else if (scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CONVERT)
	    {
		PTR		tmptupmem;
		DU_DATABASE	*temptuple;

		qeu.qeu_tab_id.db_tab_base = DM_B_DATABASE_TAB_ID;
		qeu.qeu_tab_id.db_tab_index = DM_I_DATABASE_TAB_ID;
		status = qef_call(QEU_OPEN, &qeu);
		if (status)
		{
		    sc0e_0_put(E_US000E_DATABASE_TABLE_GONE, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_US000E_DATABASE_TABLE_GONE;
		    break;
		}
		opened = 1;

		(VOID)MEmove(sizeof(dbdb_dbtuple.du_dbname),
			     (PTR) dbdb_dbtuple.du_dbname, ' ',
			     sizeof(dbt.du_dbname),
			     dbt.du_dbname);
		qeu.qeu_getnext = QEU_REPO;
		key[0].attr_number = DM_1_DATABASE_KEY;
		key[0].attr_operator = DMR_OP_EQ;
		key[0].attr_value = (char *)dbt.du_dbname;
		data.dt_data = (PTR) &dbt;
		data.dt_size = sizeof(DU_DATABASE);
		qeu.qeu_tup_length = sizeof(DU_DATABASE);
		qeu.qeu_klen = 1;

		status = qef_call(QEU_GET, &qeu);
		if (status)
		{
		    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			qeu.error.err_code = E_US0010_NO_SUCH_DB;
		    }
		    else
		    {
			sc0e_0_put(E_US000F_DATABASE_TABLE, 0);
			sc0e_0_put(qeu.error.err_code, 0);
			qeu.error.err_code = E_US000F_DATABASE_TABLE;
		    }
		    break;
		}

		status = qef_call(QEU_CLOSE, &qeu);
		if (status)
		{
		    break;
		}
		opened = 0;
		/* Silently convert old-style 10.0 version to new style.
		** This can only happen in development DB's, never
		** production or upgraded ones.
		*/
		if (dbt.du_dbcmptlvl == DU_6DBV1000)
		    dbt.du_dbcmptlvl = DU_DBV1000;

		/* Ensure iidbdb is compatible with the server */

		if (((scb->scb_sscb.sscb_ics.ics_requstat & DU_UUPSYSCAT) == 0) &&
		    (scb->scb_sscb.sscb_ics.ics_appl_code != DBA_DESTROYDB)
		   )
		{
		    if (dbt.du_dbcmptlvl != DU_CUR_DBCMPTLVL ||
			dbt.du_1dbcmptminor > DU_1CUR_DBCMPTMINOR
		       )
		    {
			error->err_code = E_US0042_COMPAT_LEVEL;
			return(E_DB_ERROR);
		    }
		    else if (dbt.du_1dbcmptminor > DU_1CUR_DBCMPTMINOR)
		    {
			error->err_code = E_US0043_COMPAT_MINOR;
			return(E_DB_ERROR);
		    }
		}

		/* Ok to allocate buffer for iidbdb tuple now */
		status = sc0m_allocate(0, 
			  (i4)(sizeof(DU_DATABASE) + sizeof(SC0M_OBJECT)), 
			  DB_SCF_ID,
			  (PTR) SCS_MEM, 
			  CV_C_CONST_MACRO('d', 'b', 'd', 'b'),
			  &tmptupmem);
		temptuple = 
		    (DU_DATABASE *)((char *)tmptupmem + sizeof(SC0M_OBJECT));
		if (status)
		{
		    sc0e_0_put(E_SC0204_MEMORY_ALLOC, 0);
		    qeu.error.err_code = E_US000F_DATABASE_TABLE;
		    break;
		}
		MEcopy((PTR)&dbt, sizeof(dbt), temptuple);

		Sc_main_cb->sc_dbdb_dbtuple = (PTR)temptuple;
		cur_dbtuple = temptuple;

		/* copy the tuple just fetched from disk into the internal
		** scf database control block, since the internal cb could
		** have different case-semantics in du_dbservice, and also
		** a differently-cased owner name.
		** (The original tuple in the dbcb was copied from
		** the hard-coded dbdb_dbtuple, which is set up with
		** INGRES lower/lower case-semantics.    (rblumer))
		**
		** On subsequent opens of the iidbdb, we will use the cached
		** sc_dbdb_dbtuple, so this copy won't be necessary.
		*/
		STRUCT_ASSIGN_MACRO(*temptuple, dbdb_dbcb->db_dbtuple);

		/* end of no-cached-tuple, not upgrading */
	    } 
	    else
	    {
		/* Here if we have no cached iidbdb iidatabase tuple yet,
		** and we're upgrading.
		** There is no point in trying to read the iidatabase tuple
		** for iidbdb because it may well be in an obsolete format.
		** Instead, we'll continue to rely on the hardwired tuple.
		** We will however update it with the proper case translation
		** flags that DMF is kind enough to return (from the config
		** file) upon a successful open.
		** The cached tuple pointer is left NULL so that eventually
		** we can pick up the real one.
		** (We have to update the tuple in the dbcb as well.)
		*/

		/* 
		** Now dmc_add_db returns the FIPS related values from cnf 
		** file. If we are not converting from pre 1.x we donot
		** have to worry about FIPS.
		*/
		if (dmc->dmc_dbcmptlvl > DU_4DBV70
		  || dmc->dmc_dbcmptlvl < DU_OLD_DBCMPTLVL)
		{
		    dbdb_dbtuple.du_dbservice = dmc->dmc_dbservice;

		    /* Fix the iidbdb owner name too, if ANSI-compliant.
		    ** The only legit possibilities for iidbdb are lower,
		    ** which is wired in, and upper.
		    */
		    if (dmc->dmc_dbservice & DU_NAME_UPPER)
		    {
			u_char *ch = &dbdb_dbtuple.du_own.db_own_name[0];
			i4 i = sizeof(dbdb_dbtuple.du_own.db_own_name);
			/* Not null-terminated or we could use CVupper */
			while (--i >= 0)
			{
			    if (CMlower(ch))
				CMtoupper(ch, ch);
			    CMnext(ch);
			}
		    }
		}

		STRUCT_ASSIGN_MACRO(dbdb_dbtuple, dbdb_dbcb->db_dbtuple);
		cur_dbtuple = &dbdb_dbtuple;

		/*  lawst01  01-27-95  Bug 66466
		** set No roles flag in SCB to inhibit access of the iirolegrant
		** table.  These did not exist prior to 6.5 / 1.1
		*/
		Sc_main_cb->sc_secflags |= SC_SEC_ROLE_NONE;   
	    }

	    scb->scb_sscb.sscb_ics.ics_dbserv = cur_dbtuple->du_dbservice;
	    scb->scb_sscb.sscb_ics.ics_dbstat = cur_dbtuple->du_access;
	    MEcopy((PTR) &cur_dbtuple->du_own,
		   sizeof(scb->scb_sscb.sscb_ics.ics_dbowner),
		   (PTR) &scb->scb_sscb.sscb_ics.ics_dbowner);
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_dbowner,
				scb->scb_sscb.sscb_ics.ics_xdbowner);

	    /*
	    ** Now that the iidbdb is open, 
	    ** determine the case translation semantics	for that database
	    ** and translate the identifiers associated with the session
	    ** for the iidbdb phase of session startup processing.
	    */
	    status = scs_icsxlate(scb, error);
	    if (DB_FAILURE_MACRO(status))
	    {
		qeu.error.err_code = E_SC0123_SESSION_INITIATE;
	    	break;
	    }

	    /* Now get user table. */
	    qeu.qeu_tab_id.db_tab_base = DM_B_USER_TAB_ID;
	    qeu.qeu_tab_id.db_tab_index = DM_I_USER_TAB_ID;
	    qeu.qeu_lk_mode = DMT_IS;
	    qeu.qeu_access_mode = DMT_A_READ;
	    status = qef_call(QEU_OPEN, &qeu);
	    if (status)
	    {
		sc0e_0_put(E_US000A_USER_TABLE_GONE, 0);
		sc0e_0_put(qeu.error.err_code, 0);
		qeu.error.err_code = E_US000A_USER_TABLE_GONE;
		break;
	    }
	    opened = 1;

	    qeu.qeu_getnext = QEU_REPO;
	    data.dt_data = (PTR)&ust;
	    data.dt_size = sizeof(ust);
	    qeu.qeu_tup_length = sizeof(ust);
	    qeu.qeu_klen = 1;
	    qeu.qeu_key = (DMR_ATTR_ENTRY **) key_ptr;

	    key[0].attr_number = DM_1_USER_KEY;
	    key[0].attr_operator = DMR_OP_EQ;
	    key[0].attr_value = (PTR) &scb->scb_sscb.sscb_ics.ics_rusername;

	    status = qef_call(QEU_GET, &qeu);

	    if(scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CONVERT &&
	       status==E_DB_OK &&
	       ust.du_gid == 0x2020 /* WARNING- ASCII ports only */
	       )
	    {
	        ADF_CB	  *adf_scb = scb->scb_sscb.sscb_adscb;  
		ADF_CB		    adf_cb ;
		DB_DATA_VALUE	    tdv;
		/*
		** This is a special case when converting from 6.4 or
		** less databases, need to initialize appropriately
		*/
		MEfill(sizeof(ust.du_pass), ' ', (PTR)&ust.du_pass);
		ust.du_flagsmask=0;
		ust.du_status=(DU_USECURITY|DU_UAUDITOR);
		ust.du_defpriv=ust.du_status;
		ust.du_userpriv=ust.du_status;

		/*
		** Initialize date to empty
		*/
		STRUCT_ASSIGN_MACRO(*adf_scb, adf_cb);
		adf_cb.adf_errcb.ad_ebuflen = 0;
		adf_cb.adf_errcb.ad_errmsgp = 0;
		tdv.db_prec	    = 0;
		tdv.db_length	    = DB_DTE_LEN;
		tdv.db_datatype	    = DB_DTE_TYPE;
		tdv.db_data         = (PTR)&ust.du_expdate;

		status=adc_getempty( &adf_cb, &tdv);
	    }
	    /* Required for upgradedb */
	    if(scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CONVERT &&
	       (ust.du_status & DU_USECURITY))
	    {
		ust.du_status|=DU_UAUDITOR;
		ust.du_defpriv|=DU_UAUDITOR;
	    }

	    if(status==E_DB_OK)
	    {
	        /* Save system notion of password for later use */
	        MEcopy((PTR)&ust.du_pass, sizeof(ust.du_pass), 
			(PTR)&scb->scb_sscb.sscb_ics.ics_srupass);
		/*
		** If user has external password remember this
		*/
		if(ust.du_flagsmask&DU_UEXTPASS)
			  scb->scb_sscb.sscb_ics.ics_uflags|=SCS_USR_REXTPASSWD;

	    }

	    /* Handle user password. This is done here is user supplied
	    ** one on initial connection, otherwise we set a state 
	    ** to prompt for passwords
	    */
	    
	    if ((status == E_DB_OK) &&
		(scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CONVERT) &&
		(STskipblank((PTR)&ust.du_pass, sizeof(ust.du_pass))
			    != NULL ||
		  (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_REXTPASSWD))
		)
	    {
		/*
		** User is password protected
		*/

		/* Check if user specified an initial password.  */
		if(STskipblank((PTR)&scb->scb_sscb.sscb_ics.ics_rupass,
		       sizeof(scb->scb_sscb.sscb_ics.ics_rupass))!=NULL)
		{
		   /*
		   ** User entered a password initially, so lets check it
		   */
		   status=scs_check_password(scb,
			(char*)&scb->scb_sscb.sscb_ics.ics_rupass,
			sizeof(scb->scb_sscb.sscb_ics.ics_rupass),
			FALSE);

		   if(status!=E_DB_OK)
	    	   {
			rupass_failure = TRUE;	/* Password didn't match */
		        qeu.error.err_code = E_US18FF_BAD_USER_ID;
			status = E_DB_ERROR;
	    	   }
		}
		else
		{
		     /*
		     ** Need to prompt for  user password, so
		     ** set up.
		     */
		     scb->scb_sscb.sscb_ics.ics_uflags|=SCS_USR_RPASSWD;
		}
	    }
	    /* Must audit USER AUTHENTICATION failure. */

	    if ( status && Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
	    {
	    	    local_status = sc0a_write_audit(
			scb,
			SXF_E_USER,	/* Type */
			SXF_A_AUTHENT|SXF_A_FAIL,         /* Action */
			scb->scb_sscb.sscb_ics.ics_terminal.db_term_name,
			sizeof( scb->scb_sscb.sscb_ics.ics_terminal.db_term_name),
			NULL,			/* Object Owner */
			I_SX2024_USER_ACCESS,	/* Mesg ID */
			FALSE,			/* Force record */
			scb->scb_sscb.sscb_ics.ics_rustat,
			error			/* Error location */
			);	
	           if (local_status != E_DB_OK)
	           {
		       qeu.error.err_code = error->err_code;
		       status = local_status;
		       break;
	           }
	    }

	    if (status)
	    {
		if (rupass_failure)
		{
			/*
			** Password specified and illegal, so stop now
			*/
			break;
		}
		if(qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    /*
		    ** Unknown user, so for security prompt for
		    ** password (this will never allow  connection
		    ** but will still prompt)
		    */
		    scb->scb_sscb.sscb_ics.ics_uflags|=SCS_USR_RPASSWD;
		    scb->scb_sscb.sscb_ics.ics_uflags|=SCS_USR_NOSUCHUSER;
		    /*
		    ** Reset to default values and continue
		    */
		    invalid_user=TRUE;
		    ust.du_status=0;
		    ust.du_defpriv=0;
    		    MEfill(sizeof(ust.du_group),
		 	(u_char)' ', 
		 	(PTR)&ust.du_group);
	            qeu.error.err_code=0;
		    status=E_DB_OK;
		}
		else
		{
		    /*
		    ** Some other failure
		    */
	  	    break;
		}
	    }


	    /* 
	    ** Initialize Maximum status privileges. This is based
	    ** on the user status, plus any required privs.
	    */
	    scb->scb_sscb.sscb_ics.ics_maxustat = ust.du_status;
	    scb->scb_sscb.sscb_ics.ics_maxustat |= 
				scb->scb_sscb.sscb_ics.ics_requstat;

	    scb->scb_sscb.sscb_ics.ics_iustat = 
			scb->scb_sscb.sscb_ics.ics_maxustat;

	    /* Copy user's default privilege mask to SCS_ICS */
	    scb->scb_sscb.sscb_ics.ics_defustat = ust.du_defpriv;

	    /* Current real status set to default privilege, plus any 
	    ** non-priv bits, such as security auditing.
	    */
	    scb->scb_sscb.sscb_ics.ics_rustat = 
	    (
		ust.du_defpriv |
		(scb->scb_sscb.sscb_ics.ics_maxustat&~DU_UALL_PRIVS)
	    );

	    /* Copy user's default group id to SCS_ICS if none specified */
	    
	    if (MEcmp((PTR)&scb->scb_sscb.sscb_ics.ics_egrpid,
		      (PTR)DB_ABSENT_NAME,
		      sizeof(scb->scb_sscb.sscb_ics.ics_egrpid)
		     ) == 0)
	    {
		STRUCT_ASSIGN_MACRO(ust.du_group,
				    scb->scb_sscb.sscb_ics.ics_egrpid);
	    }
	
	    /* Copy user's expiration date to SCS_ICS */
	    MEcopy((PTR)&ust.du_expdate, sizeof(ust.du_expdate), 
			(PTR)&scb->scb_sscb.sscb_ics.ics_expdate);
	    
	    /*
	    ** Only check effective user if different than real user
	    */
	    if(MEcmp((PTR)scb->scb_sscb.sscb_ics.ics_eusername,
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		    sizeof(scb->scb_sscb.sscb_ics.ics_rusername))!=0)
	    {
	        key[0].attr_number = DM_1_USER_KEY;
	        key[0].attr_operator = DMR_OP_EQ;
	        key[0].attr_value = (PTR) scb->scb_sscb.sscb_ics.ics_eusername;

	        status = qef_call(QEU_GET, &qeu);
	        if (status)
	        {
		    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
		    	qeu.error.err_code = E_US000D_INVALID_EFFECTIVE_USER;
		    }
		    break;
	        }
	    }

	    STRUCT_ASSIGN_MACRO((*scb->scb_sscb.sscb_ics.ics_eusername),
				scb->scb_sscb.sscb_ics.ics_iucode);
	    status = qef_call(QEU_CLOSE, &qeu);
	    if (status)
		break;
	    opened = 0;


    	    /* Get tuple from iiusergroup if necessary */

	    grpid_specified = 
		(MEcmp((PTR)&scb->scb_sscb.sscb_ics.ics_egrpid,
		       (PTR)DB_ABSENT_NAME,
		       sizeof(scb->scb_sscb.sscb_ics.ics_egrpid)
		      ) != 0);
	    /*
	    **	Security priv allows group override
	    */
	    if (grpid_specified &&
		(!(scb->scb_sscb.sscb_ics.ics_rustat & DU_USECURITY))
	       )
	    {
		qeu.qeu_tab_id.db_tab_base = DM_B_GRPID_TAB_ID;
	        qeu.qeu_tab_id.db_tab_index = DM_I_GRPID_TAB_ID;
	        status = qef_call(QEU_OPEN, &qeu);
	        if (status)
	        {
		    sc0e_0_put(E_US186A_IIUSERGROUP, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_US186A_IIUSERGROUP;
		    break;
		}
		opened = 1;

		qeu.qeu_getnext = QEU_REPO;
	        key[0].attr_number = DM_1_GRPID_KEY;
	        key[0].attr_operator = DMR_OP_EQ;
	        key[0].attr_value = (char *) &scb->scb_sscb.sscb_ics.ics_egrpid;
	        
	        key[1].attr_number = DM_2_GRPID_KEY;
	        key[1].attr_operator = DMR_OP_EQ;
	        key[1].attr_value =
		    (char *) &scb->scb_sscb.sscb_ics.ics_rusername;

	        data.dt_data = (PTR) &group_tup;
	        data.dt_size = sizeof(group_tup);
	        qeu.qeu_tup_length = sizeof(group_tup);
	        qeu.qeu_klen = 2;
	        status = qef_call(QEU_GET, &qeu);
	        if (status)
	        {
		    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			grpid_failure = TRUE;
		    }
		    else
		    {
			sc0e_0_put(E_US186A_IIUSERGROUP, 0);
			sc0e_0_put(qeu.error.err_code, 0);
			qeu.error.err_code = E_US186A_IIUSERGROUP;
			break;
		    }
		}

	        status = qef_call(QEU_CLOSE, &qeu);
		opened = 0;
	        if (status)
		    break;
	    }

	    /* Get tuple from iiapplication_id if necessary */

	    aplid_specified = 
		(MEcmp((PTR)&scb->scb_sscb.sscb_ics.ics_eaplid,
		       (PTR)DB_ABSENT_NAME,
		       sizeof(scb->scb_sscb.sscb_ics.ics_eaplid)
		      ) != 0);

	    /*
	    ** Load role information, we do this once we know the
	    ** user information.
	    **
	    ** Note, if this fails we disallow the connection  *except*
	    ** for users with SECURITY privilege. This allows security users
	    ** in to repair things if a problem occurs with the alarm
	    ** catalogs.
	    */
	    status= scs_get_session_roles(scb);
	    if (status!=E_DB_OK)
	    {
		if(!(scb->scb_sscb.sscb_ics.ics_rustat & DU_USECURITY)  &&
		     scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CONVERT)
		{
			/* Error, stop */
			break;
		}
		else
		{	/* Security user, continue */
			status=E_DB_OK;
		}
	    }
	    /*
	    ** If user specified an initial role, try to change to it.
	    */
	    if (aplid_specified)
	    {
		i4 roleprivs=0;
		DB_PASSWORD	*rolepass;


		/*
		** Verify if an initial role password was provided, if so
		** use it
		*/
		if (STskipblank((PTR)&scb->scb_sscb.sscb_ics.ics_eapass,
			   sizeof(DB_PASSWORD))
			    != NULL)
			rolepass = &scb->scb_sscb.sscb_ics.ics_eapass;
		else
			rolepass=NULL;

		/* Check the initial session role (-R) */
	    	status=scs_check_session_role(scb, 
	        	(DB_OWN_NAME *) &scb->scb_sscb.sscb_ics.ics_eaplid,
			rolepass,
			&roleprivs,
			NULL);
		if(status==E_DB_OK)
		{
		    /*
		    ** Add any role privileges to maximum set
		    */
		    scb->scb_sscb.sscb_ics.ics_maxustat |= roleprivs;
		    /*
		    ** Add any role non-privilege status bits (like auditing)
		    ** to working set.
		    */
		    scb->scb_sscb.sscb_ics.ics_rustat |= 
	                 (scb->scb_sscb.sscb_ics.ics_maxustat& ~DU_UALL_PRIVS);
		}
		else
		{
		    /*
		    ** Mark aplid failure
		    */
		    aplid_failure=TRUE;
		    status=E_DB_ERROR;
		}
	    }
	    
	    /*
	    **  "Calculate" the session's database privileges,
	    **	but only if the REAL user is not a security user.
	    **	Note we check active privileges here, not maximum, so
	    **  any role privileges are excluded, since these are always
	    **  requestable-only currently.
	    */

	    if ( !(scb->scb_sscb.sscb_ics.ics_rustat & DU_USECURITY))
	    {
		/* Initialize the session's dbprivs to no privileges */
		scs_init_dbpriv(scb, &dbpriv, FALSE);

		/* Turn off nolimit flag */
		scb->scb_sscb.sscb_ics.ics_uflags &= ~SCS_USR_DBPR_NOLIMIT;

		/* Open the iidbpriv catalog */

		qeu.qeu_tab_id.db_tab_base = DM_B_DBPRIV_TAB_ID;
	        qeu.qeu_tab_id.db_tab_index = DM_I_DBPRIV_TAB_ID;
	        status = qef_call(QEU_OPEN, &qeu);
	        if (status)
	        {
		    sc0e_0_put(E_US1888_IIDBPRIV, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_US1888_IIDBPRIV;
		    break;
		}
		opened = 1;

		/* Prepare to get tuples from iidbpriv */
		MEfill( sizeof(dbpriv_tup), 0, &dbpriv_tup );

		qeu.qeu_getnext = QEU_REPO;
	        key[0].attr_number = DM_1_DBPRIV_KEY;
	        key[0].attr_operator = DMR_OP_EQ;
	        key[1].attr_number = DM_2_DBPRIV_KEY;
	        key[1].attr_operator = DMR_OP_EQ;
	        key[2].attr_number = DM_3_DBPRIV_KEY;
	        key[2].attr_operator = DMR_OP_EQ;
	        data.dt_size = sizeof(dbpriv_tup);
	        data.dt_data = (PTR) &dbpriv_tup;
	        qeu.qeu_tup_length = sizeof(dbpriv_tup);
	        qeu.qeu_klen = 3;


		/* Merge in any initial role privileges */
		if (aplid_specified)
		{
		    got_role_dbpriv=TRUE;
		    scs_init_dbpriv(scb, &roledbpriv,FALSE);

		    /* Merge privs granted to role on the specific database */
		    dbpriv_tup.dbpr_gtype = DBGR_APLID;
		    key[2].attr_value =
			(char *) &scb->scb_sscb.sscb_ics.ics_dbname;
		    key[0].attr_value =
			(char *) &scb->scb_sscb.sscb_ics.ics_eaplid;
		    key[1].attr_value = (char *) &dbpriv_tup.dbpr_gtype;
		    status = scs_get_dbpriv(scb, &qeu, &dbpriv_tup, 
					&roledbpriv);
		    if (status==E_DB_INFO)
			status=E_DB_OK;
		    else if (status)
			break;

		    /* Merge privs granted to role on all databases */
		    key[2].attr_value = (char *)DB_ABSENT_NAME;
		    status = scs_get_dbpriv(scb, &qeu, &dbpriv_tup,
				&roledbpriv);
		    if (status==E_DB_INFO)
			status=E_DB_OK;
		    else if(status)
			break;
		}

		/* Merge in the real USER database privileges */

		if ((dbpriv.ctl_dbpriv & DBPR_ALL) != DBPR_ALL )
		{
		    /* Merge privs granted to real user on the database */
		    dbpriv_tup.dbpr_gtype = DBGR_USER;
		    key[2].attr_value = (char *) &scb->scb_sscb.sscb_ics.ics_dbname;
		    key[0].attr_value = (char *) &scb->scb_sscb.sscb_ics.ics_rusername;
		    key[1].attr_value = (char *) &dbpriv_tup.dbpr_gtype;
		    status = scs_get_dbpriv(scb, &qeu, &dbpriv_tup, &dbpriv);
		    if (status==E_DB_INFO)
			status=E_DB_OK;
		    else if (status)
			break;

		    /* Merge privs granted to real user on all databases */
		    key[2].attr_value = (char *)DB_ABSENT_NAME;
		    status = scs_get_dbpriv(scb, &qeu, &dbpriv_tup, &dbpriv);
		    if (status==E_DB_INFO)
			status=E_DB_OK;
		    else if (status)
			break;
		}

		/* Merge in the GROUP database privileges */

		if ((grpid_specified) &&
		    ((dbpriv.ctl_dbpriv & DBPR_ALL) != DBPR_ALL )
		   )
		{
		    /* Merge privs granted to group on the specific database */
		    dbpriv_tup.dbpr_gtype = DBGR_GROUP;
		    key[2].attr_value =
			(char *) &scb->scb_sscb.sscb_ics.ics_dbname;
		    key[0].attr_value =
			(char *) &scb->scb_sscb.sscb_ics.ics_egrpid;
		    key[1].attr_value = (char *) &dbpriv_tup.dbpr_gtype;
		    status = scs_get_dbpriv(scb, &qeu, &dbpriv_tup, &dbpriv);
		    if (status==E_DB_INFO)
			status=E_DB_OK;
		    else if (status)
			break;

		    /* Merge privs granted to group on all databases */
		    key[2].attr_value = (char *)DB_ABSENT_NAME;
		    status = scs_get_dbpriv(scb, &qeu, &dbpriv_tup, &dbpriv);
		    if (status==E_DB_INFO)
			status=E_DB_OK;
		    else if (status)
			break;
		}

		/* Merge in the PUBLIC database privileges */

		if ((dbpriv.ctl_dbpriv & DBPR_ALL) != DBPR_ALL )
		{
		    /* Merge privs granted to public on the specific database */
		    dbpriv_tup.dbpr_gtype = DBGR_PUBLIC;
		    MEmove(6, (PTR)"public", ' ',
				 sizeof(dbpriv_tup.dbpr_gname),
				 (PTR)&dbpriv_tup.dbpr_gname);
		    key[2].attr_value = (char *) &scb->scb_sscb.sscb_ics.ics_dbname;
		    key[0].attr_value = (char *) &dbpriv_tup.dbpr_gname;
		    key[1].attr_value = (char *) &dbpriv_tup.dbpr_gtype;
		    status = scs_get_dbpriv(scb, &qeu, &dbpriv_tup, &dbpriv);
		    if (status==E_DB_INFO)
			status=E_DB_OK;
		    else if (status)
			break;

		    /* Merge privs granted to public on all databases */
		    key[2].attr_value = (char *)DB_ABSENT_NAME;
		    status = scs_get_dbpriv(scb, &qeu, &dbpriv_tup, &dbpriv);
		    if (status==E_DB_INFO)
			status=E_DB_OK;
		    else if (status)
			break;
		}

		/* 
		** Get role dbprivs
		** We do this here to save opening/closing iidbpriv twice.
		** The flow here is such the scs_get_session_roles()
		** *must* have been called prior to this call.
		*/
		status=scs_load_role_dbpriv(scb, &qeu, &dbpriv_tup);
		if (status)
			break;
		/* Close the iidbpriv catalog */

	        status = qef_call(QEU_CLOSE, &qeu);
		opened = 0;
	        if (status)
		    break;

		/* Merge in the internal default privileges */

		if ((dbpriv.ctl_dbpriv & DBPR_ALL) != DBPR_ALL )
		{
		    /* Set the internal defaults in the template tuple */

		    dbpriv_tup.dbpr_control	|= DBPR_ALL & ~DBPR_ACCESS;
		    dbpriv_tup.dbpr_flags	|= DBPR_BINARY &
						  ~(DBPR_ACCESS |
						    DBPR_UPDATE_SYSCAT |
						    DBPR_TIMEOUT_ABORT |
						    DBPR_DB_ADMIN);

		    dbpriv_tup.dbpr_qrow_limit	= -1;
		    dbpriv_tup.dbpr_qdio_limit	= -1;
		    dbpriv_tup.dbpr_qcpu_limit  = -1;
		    dbpriv_tup.dbpr_qpage_limit = -1;
		    dbpriv_tup.dbpr_qcost_limit = -1;
		    dbpriv_tup.dbpr_idle_time_limit = -1;
		    dbpriv_tup.dbpr_connect_time_limit = -1;
		    dbpriv_tup.dbpr_priority_limit = 0;

		    /* Set only those privileges that are still undefined */

		    dbpriv_tup.dbpr_control &= ~dbpriv.ctl_dbpriv;

		    /* Factor in the internal defaults */

		    scs_init_dbpriv(scb, &tmpdbpriv, FALSE);
		    scs_cvt_dbpr_tuple(scb, &dbpriv_tup, &tmpdbpriv);
		    scs_merge_dbpriv(scb, &tmpdbpriv, &dbpriv);
		}

		/*
		** Although we have calculated the session's dbprivs
		** based on the contents of the iidbpriv catalog, we
		** still don't know who the database owner is.  After
		** we find this out, we'll reset the dbprivs if the
		** current user is the owner.  Then we'll call QEF
		** to stash the dbprivs in the QEF control block.
		*/
	    }

	    /* Pick up the iidatabase tuple and location info for the
	    ** target database.
	    ** If the target database is iidbdb, we already have a dbcb
	    ** for it.
	    ** If the target database is something else, it might be open.
	    ** Look for its dbcb because the needed poop is in there.
	    ** If the target isn't already open, we'll have to read the
	    ** relevant iidatabase row for the target db.
	    */
	    dbcb = NULL;
	    if (is_dbdb)
	    {
		dbcb = dbdb_dbcb;
	    }
	    else
	    {

	if (scb->scb_sscb.sscb_ics.ics_dbstat & DU_RDONLY)
	{
	    scb->scb_sscb.sscb_ics.ics_db_access_mode = DMC_A_READ;
	    scb->scb_sscb.sscb_ics.ics_dmf_flags2 |= DMC2_READONLYDB;
	}
		/* This isn't an open, it just looks for the target dbcb */

		if ((status = scs_dbopen(&scb->scb_sscb.sscb_ics.ics_dbname,
					 0,	/* owner */
					 0,	/* location */
					 scb,
					 error,
					 SCV_NOOPEN_MASK, 
					 0,	/* dmc */
					 0,	/* no database tuple */
					 &dbcb)
		    	) != E_DB_OK)
		{
		    /* DB isn't open, have to read iidatabase */
		    dbcb = NULL;
		}
		
		if (status = CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore))
		    break;
	    }

	    if (dbcb != NULL)
	    {
		/* We had a dbcb, use it */
		STRUCT_ASSIGN_MACRO(dbcb->db_dbtuple, dbt);
		STRUCT_ASSIGN_MACRO(dbcb->db_location, location);
		found_hard_way = 0;
	    }
	    else
	    {
		struct
		{
		    i2		dbl_length;
		    char	dbl_loc[DB_LOC_MAXNAME];
		}	db_location;
		PTR	place;

		/* No luck, have to read iidatabase and iilocation */

		qeu.qeu_tab_id.db_tab_base = DM_B_DATABASE_TAB_ID;
		qeu.qeu_tab_id.db_tab_index = DM_I_DATABASE_TAB_ID;
		status = qef_call(QEU_OPEN, &qeu);
		if (status)
		{
		    sc0e_0_put(E_US000E_DATABASE_TABLE_GONE, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_US000E_DATABASE_TABLE_GONE;
		    break;
		}
		opened = 1;

		qeu.qeu_getnext = QEU_REPO;
		key[0].attr_number = DM_1_DATABASE_KEY;
		key[0].attr_operator = DMR_OP_EQ;
		key[0].attr_value = (char *) &scb->scb_sscb.sscb_ics.ics_dbname;
		data.dt_data = (PTR) &dbt;
		data.dt_size = sizeof(dbt);
		qeu.qeu_tup_length = sizeof(dbt);
		qeu.qeu_klen = 1;
		status = qef_call(QEU_GET, &qeu);
		if (status)
		{
		    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			qeu.error.err_code = E_US0010_NO_SUCH_DB;
		    }
		    else
		    {
			sc0e_0_put(E_US000F_DATABASE_TABLE, 0);
			sc0e_0_put(qeu.error.err_code, 0);
			qeu.error.err_code = E_US000F_DATABASE_TABLE;
		    }
		    break;
		}

		status = qef_call(QEU_CLOSE, &qeu);
		if (status)
		    break;
		opened = 0;
		/* Silently convert old-style 10.0 version to new style.
		** This can only happen in development DB's, never
		** production or upgraded ones.
		*/
		if (dbt.du_dbcmptlvl == DU_6DBV1000)
		    dbt.du_dbcmptlvl = DU_DBV1000;

		qeu.qeu_tab_id.db_tab_base = DM_B_LOCATIONS_TAB_ID;
		qeu.qeu_tab_id.db_tab_index = DM_I_LOCATIONS_TAB_ID;
		status = qef_call(QEU_OPEN, &qeu);
		if (status)
		{
		    sc0e_0_put(E_US0020_IILOCATIONS_OPEN, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_US0020_IILOCATIONS_OPEN;
		    break;
		}
		opened = 1;
		qeu.qeu_getnext = QEU_REPO;
		key[0].attr_number = DM_1_LOCATIONS_KEY;
		key[0].attr_operator = DMR_OP_EQ;
		key[0].attr_value = (char *) &db_location;
		place = STrindex(dbt.du_dbloc.db_loc_name, 
				" ",
				sizeof(dbt.du_dbloc.db_loc_name));
		if (place)
		{
		    db_location.dbl_length = (i2)
			((PTR )place - (PTR )dbt.du_dbloc.db_loc_name);
		}
		else
		{
		    db_location.dbl_length = sizeof(dbt.du_dbloc.db_loc_name);
		}
		MEcopy((PTR)dbt.du_dbloc.db_loc_name,
			db_location.dbl_length,
			(PTR)db_location.dbl_loc);

		data.dt_data = (PTR) &lct;
		data.dt_size = sizeof(lct);
		qeu.qeu_tup_length = sizeof(lct);
		qeu.qeu_klen = 1;
		status = qef_call(QEU_GET, &qeu);
		qeu.qeu_getnext = QEU_NOREPO;
		while ((status == 0) &&
			((lct.du_status & DU_DBS_LOC) == 0))
		{
		    status = qef_call(QEU_GET, &qeu);
		}			
		if ((status) || ((lct.du_status & DU_DBS_LOC) == 0))
		{
		    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		    {
			qeu.error.err_code = E_US001A_MISSING_DB_AREA;
		    }
		    else
		    {
			sc0e_0_put(E_US001D_IILOCATIONS_ERROR, 0);
			sc0e_0_put(qeu.error.err_code, 0);
			qeu.error.err_code = E_US001D_IILOCATIONS_ERROR;
		    }
		    break;
		}

		status = qef_call(QEU_CLOSE, &qeu);
		if (status)
		{
		    sc0e_0_put(E_US001D_IILOCATIONS_ERROR, 0);
		    sc0e_0_put(qeu.error.err_code, 0);
		    qeu.error.err_code = E_US001D_IILOCATIONS_ERROR;
		    break;
		}
		opened = 0;
	    } /* if read iidatabase */

	    scb->scb_sscb.sscb_ics.ics_dbstat = dbt.du_access;
	    scb->scb_sscb.sscb_ics.ics_dbserv = dbt.du_dbservice;
	    /* Don't set udbid until we get the DB opened in DMF,
	    ** as the iidatabase db_id is unreliable.
	    */

	    MEcopy((PTR)&dbt.du_own, sizeof(dbt.du_own),
		    (PTR)&scb->scb_sscb.sscb_ics.ics_dbowner);
	    STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_dbowner,
		    scb->scb_sscb.sscb_ics.ics_xdbowner);

            /*
	    ** Load any database alarms, we do this once we know the
	    ** user information.
	    **
	    ** Note, if this fails we disallow the connection  *except*
	    ** for users with SECURITY privilege. This allows security users
	    ** in to repair things if a problem occurs with the alarm
	    ** catalogs.
	    */
	    status= scs_get_db_alarms(scb);
	    if (status!=E_DB_OK)
	    {
		if(!(scb->scb_sscb.sscb_ics.ics_rustat & DU_USECURITY)  &&
		     scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CONVERT)
		{
			/* Error, stop */
			break;
		}
		else
		{	/* Security user, continue */
			status=E_DB_OK;
		}
	    }

	    status = qef_call(QEU_ETRAN, &qeu);
	    if (status)
		break;
	    xact = 0;

	    status = scs_dbclose((DB_DB_NAME *)DB_DBDB_NAME,
				    scb,
				    error,
				    dmc);
	    db_open = FALSE;
	
	    if (status)
	    {
		    sc0e_0_put(E_US002D_CLOSE_DB, 0);
		    sc0e_0_put(error->err_code, 0);
		    qeu.error.err_code = E_US002D_CLOSE_DB;
		    break;
	    }

	    /*
	    ** Tell SXF we are using the final database again
	    */

	    if ( scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID) )
	    {
		sxf_cb = scb->scb_sscb.sscb_sxccb;
		sxf_cb->sxf_length = sizeof(SXF_RCB);
		sxf_cb->sxf_cb_type = SXFRCB_CB;
		sxf_cb->sxf_sess_id = scb->cs_scb.cs_self;
		sxf_cb->sxf_alter=SXF_X_DBNAME;
		sxf_cb->sxf_db_name= &scb->scb_sscb.sscb_ics.ics_dbname;
		status=sxf_call(SXC_ALTER_SESSION, sxf_cb);
		if(status!=E_DB_OK)
		{
		    sc0e_0_put(sxf_cb->sxf_error.err_code, 0);
		    qeu.error.err_code = E_SC0123_SESSION_INITIATE;
		    break;
		}
	    }

	    break;
	}

	if (status)
	{
	    STRUCT_ASSIGN_MACRO(qeu.error, *error);
	    if (opened)
	    {
		(VOID) qef_call(QEU_CLOSE, &qeu);
	    }
	    if (xact)
	    {
		(VOID) qef_call(QEU_ATRAN, &qeu);
	    }
	    if (db_open)
	    {
		(VOID) scs_dbclose((DB_DB_NAME *)DB_DBDB_NAME,
				    scb,
				    &qeu.error,	/* don't care at this point */
				    dmc);
		db_open = FALSE;
	    }
	    scd_note(status, DB_SCF_ID);

	    /* Restore the saved dmf_flags. */
	    scb->scb_sscb.sscb_ics.ics_dmf_flags = save_dmf_flags;

	    return(status);
	}
	/* Restore the saved dmf_flags. */
	scb->scb_sscb.sscb_ics.ics_dmf_flags = save_dmf_flags;

    }
    else    /* it is createdb dbdb */
    {
	found_hard_way = 0;
	/* since we know the dbdb well, build these by hand */
	STRUCT_ASSIGN_MACRO(*((SCV_LOC *) Sc_main_cb->sc_dbdb_loc), location);
	STRUCT_ASSIGN_MACRO(dbdb_dbtuple, dbt);

	/*
	** Set the case translation masks for the iidbdb
	** based upon -G flag.  The first character has
	** the case translation semantics for delimited
	** identifiers, the second character represents
	** regular identifiers, and the third character
	** represents real user identifiers.
	*/
	switch(scb->scb_sscb.sscb_ics.ics_xegrpid.db_own_name[0])
	{
	    case 'u':
	    case 'U':
		dbt.du_dbservice += DU_DELIM_UPPER;
		break;
	    case 'm':
	    case 'M':
		dbt.du_dbservice += DU_DELIM_MIXED;
		break;
	    default:
		break;
	}
	switch(scb->scb_sscb.sscb_ics.ics_xegrpid.db_own_name[1])
	{
	    case 'u':
	    case 'U':
		dbt.du_dbservice += DU_NAME_UPPER;
		break;
	    default:
		break;
	}
	switch(scb->scb_sscb.sscb_ics.ics_xegrpid.db_own_name[2])
	{
	   case 'm':
	   case 'M':
		dbt.du_dbservice += DU_REAL_USER_MIXED;
		break;
	   default:
		break;
	}

	scb->scb_sscb.sscb_ics.ics_dbstat = dbt.du_access;
	scb->scb_sscb.sscb_ics.ics_dbserv = dbt.du_dbservice;
	STRUCT_ASSIGN_MACRO(dbt.du_own, scb->scb_sscb.sscb_ics.ics_dbowner);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_dbowner,
			    scb->scb_sscb.sscb_ics.ics_xdbowner);
	scb->scb_sscb.sscb_ics.ics_db_access_mode = DMC_A_CREATE;
	scb->scb_sscb.sscb_ics.ics_lock_mode = DMC_L_EXCLUSIVE;
	scb->scb_sscb.sscb_ics.ics_rustat |= DU_UALL_PRIVS;

	status = scs_icsxlate(scb, error);
	if (DB_FAILURE_MACRO(status))
	{
	    error->err_code = E_SC0123_SESSION_INITIATE;
	    return(status);
	}
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_cat_owner,
			    scb->scb_sscb.sscb_ics.ics_xdbowner);
	STRUCT_ASSIGN_MACRO(scb->scb_sscb.sscb_ics.ics_cat_owner,
			    scb->scb_sscb.sscb_ics.ics_dbowner);
    }

    /*
    ** Translate identifiers associated with the session
    ** according to the case translation semantics for the database
    */
    if ((status == E_DB_OK) &&
	(scb->scb_sscb.sscb_stype == SCS_SNORMAL) &&
	~scb->scb_sscb.sscb_flags & SCS_STAR)
    {
	status = scs_icsxlate(scb, error);
	if (DB_FAILURE_MACRO(status))
	{
	    error->err_code = E_SC0123_SESSION_INITIATE;
	    return(status);
	}
    }

    /*
    ** Allow the real dba for a database to specify any group
    ** or role identifer without verification.  Note that this
    ** applies to super users as well.
    */

    if (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
    {
	grpid_failure = FALSE;
	aplid_failure = FALSE;
    }

    if ( aplid_specified && Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
    {
	/* 
	** This success or failure to execute a role is 
        ** a security event and must be audited.
	*/
	    
	access = SXF_A_EXECUTE;
	if (aplid_failure)
	    access |= SXF_A_FAIL;
	else
	    access |= SXF_A_SUCCESS;

	status = sc0a_write_audit(
			scb,
			SXF_E_SECURITY,	/* Type */
			access,  /* Action */
			(char *)&scb->scb_sscb.sscb_ics.ics_eaplid,
			sizeof(scb->scb_sscb.sscb_ics.ics_eaplid),
			NULL,			/* No owner */
			I_SX201B_ROLE_ACCESS,	/* Mesg ID */
			FALSE,			/* Force record */
			0,
			error			/* Error location */
			);	

	if (status != E_DB_OK)
	    return(status);
    }

    if (scb->scb_sscb.sscb_ics.ics_uflags&SCS_USR_NOSUCHUSER)
    {
	/*
	** NOTE:  Special handling for invalid users. We continue
	** as if OK, but have set the password prompt flag, which
	** will invalidate the user later on in the sequencer.
	*/
	error->err_code = 0;
    }
    /*
    ** If group or role identifiers were specified, check to see
    ** if verification failed.  Note that verification cannot fail
    ** for super users or the dba.
    */

    if (grpid_failure)
    {
	error->err_code = E_US186C_BAD_GRP_ID;
	return(E_DB_ERROR);
    }

    if (aplid_failure)
    {
	error->err_code = E_US186D_BAD_APL_ID;
	return(E_DB_ERROR);
    }

#ifdef xDEBUG
    if (scb->scb_sscb.sscb_ics.ics_appl_code >= DBA_MIN_APPLICATION &&
	scb->scb_sscb.sscb_ics.ics_appl_code <= DBA_MAX_APPLICATION )
    {
	TRdisplay("scs_dbdb_info db %~t %~t %~t dbdb %d app_code %d\n",
	sizeof(DB_DB_NAME), scb->scb_sscb.sscb_ics.ics_dbname.db_db_name,
	sizeof(DB_OWN_NAME), scb->scb_sscb.sscb_ics.ics_eusername->db_own_name,
	sizeof(DB_OWN_NAME), scb->scb_sscb.sscb_ics.ics_rusername.db_own_name,
	is_dbdb, scb->scb_sscb.sscb_ics.ics_appl_code);
    }
#endif

    /* Check if -u flag was specified on the connect */
    if (MEcmp(scb->scb_sscb.sscb_ics.ics_eusername->db_own_name,
	    scb->scb_sscb.sscb_ics.ics_rusername.db_own_name,
		    sizeof(*scb->scb_sscb.sscb_ics.ics_eusername)) != 0)
	user_flag = TRUE;
    else
	user_flag = FALSE;
 
    /* 
    ** Dba-only ingres applications may use the -u flag to connect
    ** to the user database and/or iidbdb
    ** This list should only include the application flags
    ** used from createdb, sysmod, destroydb, upgradedb, verifydb
    ** and optmizedb.
    */
    if (scb->scb_sscb.sscb_ics.ics_appl_code >= DBA_MIN_APPLICATION &&
	(scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CREATEDB || 
	scb->scb_sscb.sscb_ics.ics_appl_code == DBA_SYSMOD ||
	scb->scb_sscb.sscb_ics.ics_appl_code == DBA_DESTROYDB ||
	scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CONVERT ||
	scb->scb_sscb.sscb_ics.ics_appl_code == DBA_NORMAL_VFDB ||
	scb->scb_sscb.sscb_ics.ics_appl_code == DBA_PRETEND_VFDB ||
	scb->scb_sscb.sscb_ics.ics_appl_code == DBA_FORCE_VFDB ||
	scb->scb_sscb.sscb_ics.ics_appl_code == DBA_PURGE_VFDB ||
	scb->scb_sscb.sscb_ics.ics_appl_code == DBA_CREATE_DBDBF1 ||
	scb->scb_sscb.sscb_ics.ics_appl_code == DBA_OPTIMIZEDB) )
    {
	    dba_only_app = TRUE;

	    /*
	    ** These dba flags should not bypass -u permission checking
	    ** especially DBA_OPTIMIZEDB which can be run by non-dba users
	    */
	    if (scb->scb_sscb.sscb_ics.ics_appl_code == DBA_SYSMOD ||
		  scb->scb_sscb.sscb_ics.ics_appl_code == DBA_DESTROYDB ||
		  scb->scb_sscb.sscb_ics.ics_appl_code == DBA_OPTIMIZEDB)
		special_uflag_usage = FALSE;
	    else
		special_uflag_usage = TRUE;
    }
    else
    {
	dba_only_app = FALSE;
	special_uflag_usage = FALSE;
    }

    /*
    ** If -u was specified ensure the session has permission to do so
    **
    ** We should only bypass this checking if connecting to iidbdb 
    ** and a dba-only application code was specified.
    ** Additional permission checking will be done in scs_check_app_code
    ** Non-dba's can do optimizedb... so don't bypass user flag checking
    */
    if (user_flag 
	&& (scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY) == 0
	&& (dbpriv.fl_dbpriv & DBPR_DB_ADMIN) == 0
	&& (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA) == 0)
    {
	if (is_dbdb && dba_only_app && special_uflag_usage)
	{
#ifdef xDEBUG
	    TRdisplay("Bypass permission checking for -u flag\n");
#endif
	}
	else
	{
	    sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		 sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		 sizeof("-u flag")-1, "-u flag",
		 sizeof(SystemProductName), SystemProductName,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0 );
	    error->err_code = E_US0002_NO_FLAG_PERM;
	    return(E_DB_ERROR);
	}
    }

    /*
    ** Check application code and determine if user has appropriate
    ** privileges to execute the application. We check this prior
    ** to setting the dbprivs, since that is one of the criteria
    ** for being able to use application flags.
    */
    if (scb->scb_sscb.sscb_ics.ics_appl_code >= DBA_MIN_APPLICATION &&
	 scb->scb_sscb.sscb_ics.ics_appl_code <= DBA_MAX_APPLICATION )
    {
    	status=scs_check_app_code(scb, &dbpriv, is_dbdb);
    	if(status!=E_DB_OK)
    	{
	     error->err_code = E_US0002_NO_FLAG_PERM;
	     return (E_DB_ERROR);
	}
    }

    /*
    ** Check to see if the current real user is the database owner,
    ** or if the session has DB_ADMIN privileges on the database.
    ** If so, allow unlimited database privileges to the session.
    */

    if ( (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
	|| (dbpriv.fl_dbpriv & DBPR_DB_ADMIN)
	|| dba_only_app)
    {
	/* All privileges */
	scs_init_dbpriv(scb, &dbpriv, TRUE);
	all_dbpriv=TRUE;
	/* Mark no limits */
	scb->scb_sscb.sscb_ics.ics_uflags |= SCS_USR_DBPR_NOLIMIT;
    }

    /*
    ** Save initial dbpriv set for later use. 
    ** not that this does NOT include any initial role
    ** privileges.
    */
    STRUCT_ASSIGN_MACRO(dbpriv, scb->scb_sscb.sscb_ics.ics_idbpriv);

    if (all_dbpriv==FALSE && got_role_dbpriv)
    {
	/*
	** Need to merge role dbprivs we got with current value.
	*/
	scs_merge_dbpriv(scb, &scb->scb_sscb.sscb_ics.ics_idbpriv,
			&roledbpriv);

	/*
	** Put role/current dbpriv values in SCB
	*/
        status=scs_put_sess_dbpriv(scb, &roledbpriv);
    }
    else
    {
	    /*
	    ** Put current dbpriv values in SCB for use
	    */
	    status=scs_put_sess_dbpriv(scb, &dbpriv);
    }

    if(status!=E_DB_OK)
    {
	return E_DB_ERROR;
    }
    /* Make sure QEF knows about session status */
    qef_alt[0].alt_code = QEC_USTAT;
    qef_alt[0].alt_value.alt_i4 = scb->scb_sscb.sscb_ics.ics_rustat;

    qef_cb.qef_length = sizeof(QEF_RCB);
    qef_cb.qef_type = QEFRCB_CB;
    qef_cb.qef_ascii_id = QEFRCB_ASCII_ID;
    qef_cb.qef_sess_id = scb->cs_scb.cs_self;
    qef_cb.qef_asize = 1;
    qef_cb.qef_alt = qef_alt;
    qef_cb.qef_eflag = QEF_EXTERNAL;
    qef_cb.qef_cb = scb->scb_sscb.sscb_qescb;

    status = qef_call(QEC_ALTER, &qef_cb);
    if (status)
    {
        sc0e_0_put(qef_cb.error.err_code, 0);
        error->err_code = E_QE0210_QEC_ALTER;
        status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
        scd_note(status, DB_SCF_ID);
        return(status);
    }



    /*
    ** Having reached here,
    ** we know that we have read valid information.
    ** Now, we simply have to check that the database is available
    ** and is allowed to be used.
    */	


    /* Ensure user is allowed ACCESS to the specified database */

    if (scb->scb_sscb.sscb_ics.ics_ctl_dbpriv & DBPR_ACCESS)
    {
	/* ACCESS was defined.  Check if allowed or denied. */
	if (!(scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_ACCESS))
	{
	    /* ACCESS was denied.  Abort the session */
	    error->err_code = E_US0003_NO_DB_PERM;
	    /*
	    ** Security audit access failure
	    */
	    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
	    {
		status = sc0a_write_audit(
			    scb,
			    SXF_E_DATABASE,	/* Type */
			    SXF_A_FAIL|SXF_A_CONNECT,  /* Action */
			    (char*)&scb->scb_sscb.sscb_ics.ics_dbname,
			    sizeof(scb->scb_sscb.sscb_ics.ics_dbname),
			    &scb->scb_sscb.sscb_ics.ics_dbowner,
			    I_SX201A_DATABASE_ACCESS,	/* Mesg ID */
			    FALSE,			/* Force record */
			    0,
			    &local_error	/* Error location */
			    );	
		if (status != E_DB_OK)
		    sc0e_0_put(local_error.err_code, 0);
	    }
	    return (E_DB_ERROR);
	}
    }
    /* Access was not defined.  Check other factors. */
    else if (((dbt.du_access & DU_GLOBAL) == 0) &&
	     (scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CREATEDB) &&
	     (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_EINGRES))
    {
	/* Access will be denied due to insufficient privileges */
	error->err_code = E_US0003_NO_DB_PERM;
        /*
        ** Security audit access failure
        */
	if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
	{
	    status = sc0a_write_audit(
		    scb,
		    SXF_E_DATABASE,	/* Type */
		    SXF_A_FAIL|SXF_A_CONNECT,  /* Action */
		    (char*)&scb->scb_sscb.sscb_ics.ics_dbname,
		    sizeof(scb->scb_sscb.sscb_ics.ics_dbname),
		    &scb->scb_sscb.sscb_ics.ics_dbowner,
		    I_SX201A_DATABASE_ACCESS,	/* Mesg ID */
		    FALSE,			/* Force record */
		    0,
		    &local_error		/* Error location */
		    );	
	    if (status != E_DB_OK)
		sc0e_0_put(local_error.err_code, 0);
	}
	return (E_DB_ERROR);
    }
    /*
    ** Since the update system catalog bit in iiuser is obsolete,
    ** we must handle this privilege with care.
    */

    if (scb->scb_sscb.sscb_ics.ics_fl_dbpriv  & DBPR_UPDATE_SYSCAT)
    {
	scb->scb_sscb.sscb_ics.ics_rustat |= DU_UUPSYSCAT;
	scb->scb_sscb.sscb_ics.ics_maxustat |= DU_UUPSYSCAT;
    }
    else
    {
	/* Disallow update system catalog requests */
	scb->scb_sscb.sscb_ics.ics_rustat &= ~DU_UUPSYSCAT;
	scb->scb_sscb.sscb_ics.ics_maxustat &= ~DU_UUPSYSCAT;
    }


    /* Ensure all flag requests are valid for this user */

    if ( (scb->scb_sscb.sscb_ics.ics_appl_code < DBA_MIN_APPLICATION ||
	    scb->scb_sscb.sscb_ics.ics_appl_code > DBA_MAX_APPLICATION ||
	    scb->scb_sscb.sscb_ics.ics_appl_code == DBA_IIFE
	 ) &&
	 ( (scb->scb_sscb.sscb_ics.ics_requstat 
		& scb->scb_sscb.sscb_ics.ics_rustat
	    ) != scb->scb_sscb.sscb_ics.ics_requstat
	 )
	)
    {
	sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		 sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		 sizeof("Requested status flags not in real status")-1, 
			"Requested status flags not in real status",
		 sizeof(SystemProductName), SystemProductName,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0 );
	error->err_code = E_US0002_NO_FLAG_PERM;
	return(E_DB_ERROR);
    }
    /* for DESTROYDB with '-u' flag, make sure user has security priv first */

    if ( scb->scb_sscb.sscb_ics.ics_appl_code == DBA_DESTROYDB
        &&
            MEcmp(scb->scb_sscb.sscb_ics.ics_eusername->db_own_name,
                scb->scb_sscb.sscb_ics.ics_rusername.db_own_name,
                sizeof(*scb->scb_sscb.sscb_ics.ics_eusername)) != 0
        &&
            (scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY) == 0
        )
        {
        sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 2,
                 sizeof (scb->scb_sscb.sscb_ics.ics_rusername),
                    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
                 sizeof("-u flag")-1, "-u flag",
                 0, (PTR)0,
                 0, (PTR)0,
                 0, (PTR)0,
                 0, (PTR)0 );
        error->err_code = E_US0002_NO_FLAG_PERM;
        return(E_DB_ERROR);
    }

    /*
    ** If -u was specified ensure the session has permission to do so
    **
    ** We should only bypass this checking if connecting to iidbdb 
    ** and a dba-only application code was specified.
    ** Non-dba's can do optimizedb... so don't bypass user flag checking
    */
    if (user_flag 
	&& (scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY) == 0
	&& (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_DB_ADMIN) == 0
	&& (scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA) == 0)
    {
	if (is_dbdb && dba_only_app && special_uflag_usage)
	{
#ifdef xDEBUG
	    TRdisplay("Bypass permission checking for -u flag\n");
#endif
	}
	else
	{
	    sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
			(PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     sizeof("-u flag")-1, "-u flag",
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
	    error->err_code = E_US0002_NO_FLAG_PERM;
	    return(E_DB_ERROR);
	}
    }

    if (((dbt.du_access & DU_OPERATIVE) == 0) &&
        (scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CREATE_DBDBF1) &&
        (scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CREATEDB) &&
	(scb->scb_sscb.sscb_ics.ics_appl_code != DBA_DESTROYDB) &&
	(scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CONVERT)
       )
    {
	error->err_code = E_US0014_DB_UNAVAILABLE;
	return(E_DB_ERROR);
    }

    /* Ensure the database is compatible with the server */

    if (((scb->scb_sscb.sscb_ics.ics_requstat & DU_UUPSYSCAT) == 0) &&
	(scb->scb_sscb.sscb_ics.ics_appl_code != DBA_CONVERT) &&
	(scb->scb_sscb.sscb_ics.ics_appl_code != DBA_DESTROYDB)
       )
    {
	if (dbt.du_dbcmptlvl != DU_CUR_DBCMPTLVL ||
	    dbt.du_1dbcmptminor > DU_1CUR_DBCMPTMINOR)
	{
	    error->err_code = E_US0042_COMPAT_LEVEL;
	    return(E_DB_ERROR);
	}
	else if (dbt.du_1dbcmptminor > DU_1CUR_DBCMPTMINOR)
	{
	    error->err_code = E_US0043_COMPAT_MINOR;
	    return(E_DB_ERROR);
	}
    }
    /*
    ** Tell QEF about new privileges
    */


    if (found_hard_way)
    {
	/*
	**	At this point, we construct the appropriate structure
	**	from the DBDB information to give to DMF to open the
	**	database.
	*/
	MEmove(8, "$default", ' ', sizeof(location.loc_entry.loc_name),
		(PTR)&location.loc_entry.loc_name);
	for (i = 0;
	     i < sizeof(scb->scb_sscb.sscb_ics.ics_dbname.db_db_name);
	     i += CMbytecnt(&scb->scb_sscb.sscb_ics.ics_dbname.db_db_name[i])
	    )
	{
	    if (CMspace(&scb->scb_sscb.sscb_ics.ics_dbname.db_db_name[i]))
		break;
	}
	c = scb->scb_sscb.sscb_ics.ics_dbname.db_db_name[i];
	scb->scb_sscb.sscb_ics.ics_dbname.db_db_name[i] = 0;
	lct.du_area[lct.du_l_area] = 0;
	LOingpath(lct.du_area,
	    scb->scb_sscb.sscb_ics.ics_dbname.db_db_name,
	    LO_DB,
	    &dbloc);
	scb->scb_sscb.sscb_ics.ics_dbname.db_db_name[i] = c;
	LOcopy(&dbloc, loc_buf, &dbloc);
	LOtos(&dbloc, &dbplace);
	location.loc_entry.loc_l_extent = (i4)STlength(dbplace);
	MEmove(location.loc_entry.loc_l_extent,
		dbplace,
		' ',
		sizeof(location.loc_entry.loc_extent),
		location.loc_entry.loc_extent);
	location.loc_entry.loc_flags = LOC_DEFAULT | LOC_DATA;
	location.loc_l_loc_entry = 1 * sizeof(DMC_LOC_ENTRY);
    }

    if (!db_open)
    {
    	scb->scb_sscb.sscb_ics.ics_loc = &location;
	if (scb->scb_sscb.sscb_ics.ics_dbstat & DU_RDONLY)
	{
	    scb->scb_sscb.sscb_ics.ics_db_access_mode = DMC_A_READ;
	    scb->scb_sscb.sscb_ics.ics_dmf_flags2 |= DMC2_READONLYDB;
	}

    	status = scs_dbopen(0, 0, 0, scb, error, 0, dmc, &dbt, 0);
    	if (status)
	{
	    scs_dmf_to_user(error);
        }
	else
	{
	    /*
	    ** give QEF the database id and the unique database id; qef_cb has
	    ** been largely initialized
	    */
	    qef_alt[0].alt_code = QEC_DBID;
	    qef_alt[0].alt_value.alt_ptr =
		(PTR) scb->scb_sscb.sscb_ics.ics_opendb_id;
	    qef_alt[1].alt_code = QEC_UDBID;
	    qef_alt[1].alt_value.alt_i4 = scb->scb_sscb.sscb_ics.ics_udbid;

	    qef_cb.qef_asize = 2;
	    qef_cb.qef_length = sizeof(QEF_RCB);
	    qef_cb.qef_type = QEFRCB_CB;
	    qef_cb.qef_ascii_id = QEFRCB_ASCII_ID;
	    qef_cb.qef_sess_id = scb->cs_scb.cs_self;
	    qef_cb.qef_alt = qef_alt;
	    qef_cb.qef_eflag = QEF_EXTERNAL;
	    qef_cb.qef_cb = scb->scb_sscb.sscb_qescb;

	    status = qef_call(QEC_ALTER, &qef_cb);
	    if (status)
	    {
		sc0e_0_put(qef_cb.error.err_code, 0);
		error->err_code = E_QE0210_QEC_ALTER;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
		scd_note(status, DB_SCF_ID);
	    }
	}
    	scb->scb_sscb.sscb_ics.ics_loc = 0;
    }
    else
    {
	status = E_DB_OK;
    }

    return(status);
}

/*{
** Name: scs_dmf_to_user	- Translate dmf add/open errors to user errors.
**
** Description:
**      This routine simply maps dmf add/open errors to user startup errors.
**
** Inputs:
**      error                           ptr to DB_ERROR struct to be mapped.
**
** Outputs:
**      error                           DB_ERROR struct is changed to user error
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    the error is logged, if necessary
**
** History:
**      01-Apr-1987 (fred)
**          Created.
**	15-may-1989 (rogerk)
**	    Added check for TRAN_QUOTA_EXCEEDED.
**	25-jul-1989 (sandyh)
**	    Added invalid dcb E_DM0133 handler.
**	20-aug-1989 (ralph)
**	    Check for E_DM0113_DB_INCOMPATABLE
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
VOID
scs_dmf_to_user(DB_ERROR *error )
{
    switch (error->err_code)
    {
        case E_DM000B_BAD_CB_LENGTH:
        case E_DM000C_BAD_CB_TYPE:
        case E_DM000F_BAD_DB_ACCESS_MODE:
        case E_DM0010_BAD_DB_ID:
        case E_DM001A_BAD_FLAG:
        case E_DM001D_BAD_LOCATION_NAME:
        case E_DM002D_BAD_SERVER_ID:
        case E_DM002F_BAD_SESSION_ID:
        case E_DM004A_INTERNAL_ERROR:
	case E_DM0084_ERROR_ADDING_DB:
	case E_DM0086_ERROR_OPENING_DB:
        case E_DM0065_USER_INTR:
        case E_DM0064_USER_ABORT:
	    sc0e_0_put(error->err_code, 0);
	    error->err_code = E_US0030_DMF_START_EXCEPT;
	    break;

        case E_DM0011_BAD_DB_NAME:
	    sc0e_0_put(error->err_code, 0);
	    error->err_code = E_US0009_BAD_PARAM;
	    break;

        case E_DM003E_DB_ACCESS_CONFLICT:
        case E_DM004B_LOCK_QUOTA_EXCEEDED:
	    sc0e_0_put(error->err_code, 0);
	    error->err_code = E_US002E_LOCK_EXCEPTION;
	    break;

        case E_DM0040_DB_OPEN_QUOTA_EXCEEDED:
        case E_DM0041_DB_QUOTA_EXCEEDED:
	    sc0e_0_put(error->err_code, 0);
	    error->err_code = E_US0048_DB_QUOTA_EXCEEDED;
	    break;

	case E_DM004C_LOCK_RESOURCE_BUSY:
	    error->err_code = E_US0014_DB_UNAVAILABLE;
	    break;

        case E_DM0053_NONEXISTENT_DB:
	    error->err_code = E_US0010_NO_SUCH_DB;
	    break;

        case E_DM0100_DB_INCONSISTENT:
	    sc0e_0_put(error->err_code, 0);
	    error->err_code = E_US0026_DB_INCONSIST;
	    break;

        case E_DM0062_TRAN_QUOTA_EXCEEDED:
	    sc0e_0_put(error->err_code, 0);
	    error->err_code = E_US1272_4722_DB_TRAN_LIMIT;
	    break;

        case E_DM0133_ERROR_ACCESSING_DB:
	    sc0e_0_put(error->err_code, 0);
	    error->err_code = E_US0049_ERROR_ACCESSING_DB;
	    break;

        case E_DM0113_DB_INCOMPATABLE:
	    sc0e_0_put(error->err_code, 0);
	    error->err_code = E_US0008_CONVERT_DB;
	    break;

	default:
	    break;
    }
}

/*{
** Name: scs_ctlc	- Handle Control-C type interrupts to the DBMS
**
** Description:
**      This routine translates the control-c interrupt from GCF into 
**      a form understandable by CSattn().  That is, the call is 
**      passed to CSattn() with an event id of CS_ATTN_EVENT.
**
** Inputs:
**      sid                             Session id of recipient.
**
** Outputs:
**      None
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    The session identified is notified of the outstanding event.
**
** History:
**      30-Nov-1987 (fred)
**          Created on Jupiter.
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
VOID
scs_ctlc(SCD_SCB *scb)
{
    CSattn(CS_ATTN_EVENT, scb->cs_scb.cs_self);
    return;
}

/*{
** Name: scs_log_event	- Handle log event type interrupts to the DBMS
**
** Description:
**      This routine translates the logger interrupt from DMF into 
**      a form understandable by CSattn().  That is, the call is 
**      passed to CSattn() with an event id of CS_RCVR_EVENT.
**
** Inputs:
**      sid                             Session id of recipient.
**
** Outputs:
**      None
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    The session identified is notified of the outstanding event.
**
** History:
**      30-Nov-1987 (fred)
**          Created on Jupiter.
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
VOID
scs_log_event(SCD_SCB *scb)
{
    CSattn(CS_RCVR_EVENT, scb->cs_scb.cs_self);
    return;
}

/*{
** Name: scs_disassoc - Low level GCA disconnect
**
** Description:
**      This routine is provided so that the GCA_DISASSOC call
**      can be easily used.
**
** Inputs:
**	assoc_id	GCA association id
**
** Outputs:
**      none
**
**	Returns:
**	    STATUS	error from GCA
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-Jan-91 (anton)
**	    Created for GCA API 2 support
**	2-Jul-1993 (daveb)
**	    prototyped.
**	08-sep-93 (swm)
**	    Changed sid type from i4 to CS_SID to match recent CL
**	    interface revision.
**	24-jan-1994 (gmanning)
**	    Change references from CS_SID to CS_SCB and CSget_scb.
**	16-Nov-1998 (jenjo02)
**	    When suspending on BIO read, use CS_BIOR_MASK.
*/
STATUS
scs_disassoc(i4 assoc_id )
{
    STATUS		status;
    GCA_DA_PARMS	da_parms;
    i4			indicators;
    STATUS		gca_error;
    CS_SCB		*cs_scb;

    CSget_scb(&cs_scb);

    da_parms.gca_association_id = assoc_id;
    indicators = 0;
    do {
	status = IIGCa_call(GCA_DISASSOC,
			    (GCA_PARMLIST *)&da_parms,
			    indicators,
			    (PTR)cs_scb,
			    (i4)-1,
			    &gca_error);

	if (status)
	    return( gca_error );
    
	status = CSsuspend(CS_BIOR_MASK, 0, 0);

	if (status)
	    return status;

	indicators = GCA_RESUME;

    } while (da_parms.gca_status == E_GCFFFE_INCOMPLETE);

    return da_parms.gca_status;
}



/*{
** Name: conv_to_struct_xa_xid
**
** Description:
**      This routine converts an XA XID from a string form to the XID
**      structure format. Is called at the time of GCA_XA_RESTART of an
**      Ingres session, over which the XA XN is sub-sequently committed 
**      or rolled back.
**
** WARNING:
**      Must match IICXconv_to_struct_xa_id() in libqxa/iicxxa.sc
**
** Inputs:
**      xid_text        - the xid in text string format.
**      xid_str_max_len - the max length of the xid string.
**      xa_xid_p        - pointer to an XA XID.
**
** Outputs:
**      none
**
**	Returns:
**	    DB_STATUS	if the routine is successful or not.
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-Jan-93 (mani)
**	    Created for Ingres65 XA support.
**      23-Sep-93 (iyer)
**          Modified to accept pointer to DB_XA_EXTD_DIS_TRAN_ID instead
**          DB_XA_DIS_TRAN_ID. Also added statements to extract branch_seqnum
**          branch_flag from the xid_str.
**	06-Feb-96 (toumi01; from 1.1 axp_osf port) (schte01)
**	    Modified calls for conversion for axp_osf for XID and lengths.
**	    Also modified casts from u_i4 to long for all XID and
**	    lengths.
**	21-feb-96 (thaju02)
**	    Added history for change#422304 after-the-fact.
**	    Enhanced XA Support.  Re-declared function conv_to_struct_xa_xid
**	    from static to global.
**	18-sep-1998 (thoda04)
**	    Use CVuahxl for integer conversion of XID data to avoid sign overflow.
**	05-oct-1998 (thoda04)
**	    Process XID data bytes in groups of four, high to low bytes for 
**          all platforms.
**      11-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          Extend ifdef's for 8 byte lon platforms to match that seen in 
**          IICXconv_to_struct_xa_id() in libqxa. Also changed cast to
**          u_i8 to quiet compiler warnings.
**      12-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          Modify all XA xid fields to i4 (int for supplied files)
**          to ensure consistent use across platforms.
*/

DB_STATUS conv_to_struct_xa_xid( char               *xid_text,
                                 i4            xid_str_max_len,
                                 DB_XA_EXTD_DIS_TRAN_ID  *xa_extd_xid_p )
{


   char        *cp, *next_cp, *prev_cp;
   u_char      *tp;
   char        c;
   i4          j, num_xid_words;
   i4     num;            /* work i4 */
   u_i4        unum;
   STATUS      cl_status;


   if ((cp = STindex(xid_text, ":", xid_str_max_len)) == NULL)
   {
       return( E_DB_ERROR ); 
   }

   c = *cp;
   *cp = EOS;

   if ((cl_status = CVuahxl(xid_text, (u_i4 *)
			  (&xa_extd_xid_p->db_xa_dis_tran_id.formatID))) != OK)
   {
      return( E_DB_ERROR );
   }

   *cp = c;    

   if ((next_cp = STindex(cp + 1, ":", 0)) == NULL)
   {
       return( E_DB_ERROR );
   }

   c = *next_cp;
   *next_cp = EOS;

   if ((cl_status = CVal(cp + 1, &num)) != OK)
   {
      return( E_DB_ERROR );
   }
   xa_extd_xid_p->db_xa_dis_tran_id.gtrid_length = num;
               /* 32 or 64 bit long gtrid_length = i4 num */

   *next_cp = c;    

   if ((cp = STindex(next_cp + 1, ":", 0)) == NULL)
   {
       return( E_DB_ERROR );
   }

   c = *cp;
   *cp = EOS;

   if ((cl_status = CVal(next_cp + 1, &num)) != OK)
   {
      return( E_DB_ERROR );
   }
   xa_extd_xid_p->db_xa_dis_tran_id.bqual_length = num;
               /* 32 or 64 bit long bqual_length = i4 num */


   *cp = c;    

    /* Now copy all the data                            */
    /* We now go through all the hex fields separated by */
    /* ":"s.  We convert it back to bytes and store it in */
    /* the data field of the distributed transaction id  */
    /*     cp      - points to a ":"                       */
    /*     prev_cp - points to the ":" previous to the     */
    /*               ":" cp points to, as we get into the  */
    /*               loop that converts the XID data field */
    /*     tp    -   points to the data field of the       */
    /*               distributed transaction id            */

    prev_cp = cp;
    tp = (u_char*) (&xa_extd_xid_p->db_xa_dis_tran_id.data[0]);
    num_xid_words = (xa_extd_xid_p->db_xa_dis_tran_id.gtrid_length + 
		     xa_extd_xid_p->db_xa_dis_tran_id.bqual_length +
                     sizeof(u_i4)-1) / sizeof(u_i4);
    for (j=0;
         j< num_xid_words;
         j++)
    {

         if ((cp = STindex(prev_cp + 1, ":", 0)) == NULL)
         {
             return( E_DB_ERROR );
         }
         c = *cp;
         *cp = EOS;
         if ((cl_status = CVuahxl(prev_cp + 1, &unum)) != OK)
         {
             return( E_DB_ERROR );
         }
         tp[0] = (u_char)((unum >> 24) & 0xff);  /* put back in byte order */
         tp[1] = (u_char)((unum >> 16) & 0xff);
         tp[2] = (u_char)((unum >>  8) & 0xff);
         tp[3] = (u_char)( unum        & 0xff);
         tp += sizeof(u_i4);
         *cp = c;
         prev_cp = cp;
    }

    next_cp = cp;                           /* Store last ":" position */
                                            /* Skip "XA" & look for next ":" */

    if ((cp = STindex(next_cp + 1, ":", 0)) == NULL) 
       {                                   /* First ":" before branch seq */
             return( E_DB_ERROR );
       }

    next_cp = cp;                          /* Skip branch seq for next ":" */

    if ((cp = STindex(next_cp + 1, ":", 0)) == NULL) 
       {                                   /* First ":" after branch seq */
             return( E_DB_ERROR );
       }

    c = *cp;                               /* Store actual char in that POS */
    *cp = EOS;                             /* Pretend end of string */

    if ((cl_status = CVan(next_cp + 1, 
                          &xa_extd_xid_p->branch_seqnum)) != OK)
    {
      return( E_DB_ERROR );
    }

   *cp = c;                              /* Restore char in that Position */
    
   if ((next_cp = STindex(cp + 1, ":", 0)) == NULL)
   {
       return( E_DB_ERROR );
   }

   c = *next_cp;
   *next_cp = EOS;

   if ((cl_status = CVan(cp + 1, 
			&xa_extd_xid_p->branch_flag)) != OK)
   {
      return( E_DB_ERROR );
   }

   *next_cp = c;    

   return( E_DB_OK );

} /* conv_to_struct_xa_xid */


/*{
** Name: scs_usr_expired
**
** Description:
**      This routine checks if the current user account has expired or not.
**
** Inputs:
**      scb		- SCB, session control block.
**
** Outputs:
**      none
**
**	Returns:
**	    E_DB_OK	- Not expired
**	    E_DB_WARN   - Expired
**	    E_DB_ERROR  - Error
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-jun-93  (robf)
**	    Created for Secure 2.0
*/
static DB_STATUS
scs_usr_expired(
	SCD_SCB            *scb
)
{
	DB_STATUS status;
	ADF_CB		    *adf_scb = scb->scb_sscb.sscb_adscb;  
	ADF_CB		    adf_cb ;
	DB_DATA_VALUE	    tdv;
	DB_DATA_VALUE	    fdv;
	DB_DATE		    date_now;
	DB_DATE		    date_empty;
	i4		    cmp;

	/* Copy the session ADF block into local one */
	STRUCT_ASSIGN_MACRO(*adf_scb, adf_cb);

	adf_cb.adf_errcb.ad_ebuflen = 0;
	adf_cb.adf_errcb.ad_errmsgp = 0;
	/*
	** Check if date is "empty" or not, if its the "empty" date we
	** ignore.
	*/
	tdv.db_prec	    = 0;
	tdv.db_length	    = DB_DTE_LEN;
	tdv.db_datatype	    = DB_DTE_TYPE;
	tdv.db_data         = (PTR)&date_empty;

	status=adc_getempty( &adf_cb, &tdv);
	if(DB_FAILURE_MACRO(status))
		return E_DB_ERROR;
	
	/*
	** Now compare the two dates
	*/
	fdv.db_prec	    = 0;
	fdv.db_length	    = DB_DTE_LEN;
	fdv.db_datatype	    = DB_DTE_TYPE;
	fdv.db_data         = (PTR)&scb->scb_sscb.sscb_ics.ics_expdate;

	status=adc_compare(&adf_cb,  &fdv, &tdv, &cmp);
	if(DB_FAILURE_MACRO(status))
		return E_DB_ERROR;

	if(!cmp)
	{
		/*
		** Expire date is empty, so return OK
		*/
		return E_DB_OK;
	}
	/*
	** Generate date "now"
	*/
	tdv.db_prec	    = 0;
	tdv.db_length	    = DB_DTE_LEN;
	tdv.db_datatype	    = DB_DTE_TYPE;
	tdv.db_data         = (PTR)&date_now;
	status=adu_datenow(&adf_cb, &tdv);
	if(status!=E_DB_OK)
	{
		return E_DB_ERROR;
	}

	/*
	**  Compare against expire date
	*/
	fdv.db_prec	    = 0;
	fdv.db_length	    = DB_DTE_LEN;
	fdv.db_datatype	    = DB_DTE_TYPE;
	fdv.db_data         = (PTR)&scb->scb_sscb.sscb_ics.ics_expdate;

	status=adc_compare(&adf_cb,  &fdv, &tdv, &cmp);
	if(DB_FAILURE_MACRO(status))
		return E_DB_ERROR;

	if(cmp>0)
	{
		/*
		** Expire date > current date, OK
		*/
		return E_DB_OK;
	}
	else
	{
		/*
		** Expire date <= current date, fail
		*/
		return E_DB_WARN;
	}
}

/*
** Name: scs_check_password
**
** Description:
**	This checks a user password following a prompt, performing
**	any required security auditing
**
** Inputs:
**	scb	- SCB
**
**	passwd	- Password, not encrypted
**
**	pwlen   - Length of passwd
**
**	do_alarm_audit - TRUE if fire database alarms, do auditing
**
** Outputs:
**	None
**
** Returns:
**	E_DB_OK	- Password is valid
**
**	E_DB_ERROR  -Password is not  valid
**
** History:
**	5-nov-93 (robf)
**          Created
**	14-dec-93 (robf)
**          Fire security alarms on successful password prompt.
**	15-mar-04 (robf)
**          Made external, check for external passwords, add
**	    do_alarm_audit parameter
**	16-jul-96 (stephenb)
**	    Add efective username to scs_check_external_password() parameters.
*/
DB_STATUS
scs_check_password( SCD_SCB *scb, 
		char *passwd, 
		i4 pwlen,
		bool do_alarm_audit)
{
    DB_STATUS 	status=E_DB_OK;
    bool  	loop=FALSE;
    SCF_CB	scf_cb;
    DB_ERROR	error;
    DB_STATUS   local_status;
    SXF_ACCESS	sxfaccess;

    do
    {
	/*
	** If no password then invalid
	*/
	if(pwlen<=0)
	{
		status=E_DB_ERROR;
		break;
	}
	/*
	** Save input password
	*/
	MEmove(pwlen, passwd, ' ',
		sizeof(scb->scb_sscb.sscb_ics.ics_rupass),
		(PTR)&scb->scb_sscb.sscb_ics.ics_rupass);
	/*
	** Check if user has external password or not
	*/
	if( scb->scb_sscb.sscb_ics.ics_uflags&SCS_USR_REXTPASSWD)
	{
		status=scs_check_external_password (
			scb,
			&scb->scb_sscb.sscb_ics.ics_rusername,
			(DB_OWN_NAME *)&scb->scb_sscb.sscb_ics.ics_eusername,
			&scb->scb_sscb.sscb_ics.ics_rupass,
			FALSE);
		if(status==E_DB_WARN)
		{
		     /*
		     ** Access is denied
		     */
		     status=E_DB_ERROR;
		     break;
		}
		else if (status!=E_DB_OK)
		{
		     /*
		     ** Some kind of error
		     */
		     sc0e_0_put(E_SC035C_USER_CHECK_EXT_PWD, 0);
		     break;

		}
		else
		{
		    /*
		    ** Access is allowed
		    */
		    break;
		}
	}
	/*
	** Encrypt password
	*/

	if (STskipblank((char*)&scb->scb_sscb.sscb_ics.ics_rupass,
	      sizeof(scb->scb_sscb.sscb_ics.ics_rupass))
	    != NULL)
	{
	    scf_cb.scf_length = sizeof(scf_cb);
	    scf_cb.scf_type   = SCF_CB_TYPE;
	    scf_cb.scf_facility = DB_SCF_ID;
	    scf_cb.scf_session  = DB_NOSESSION;
	    scf_cb.scf_nbr_union.scf_xpasskey =
	        (PTR)scb->scb_sscb.sscb_ics.ics_rusername.db_own_name;
	    scf_cb.scf_ptr_union.scf_xpassword =
	        (PTR)scb->scb_sscb.sscb_ics.ics_rupass.db_password;
	    scf_cb.scf_len_union.scf_xpwdlen =
	        sizeof(scb->scb_sscb.sscb_ics.ics_rupass);
	    status = scf_call(SCU_XENCODE,&scf_cb);
	    if (status)
	    {
	        status = (status > E_DB_ERROR) ?
		      status : E_DB_ERROR;
		sc0e_0_put(E_SC0260_XENCODE_ERROR, 0);
	        break;
	    }
	}
	/*
	** If no such user then invalid
	*/
	if( scb->scb_sscb.sscb_ics.ics_uflags&SCS_USR_NOSUCHUSER)
	{
		status=E_DB_ERROR;
		break;
	}
	/*
	** Now password is encrypted, compare it to real password
	*/
	if(MEcmp(
	       (PTR)&scb->scb_sscb.sscb_ics.ics_rupass,
	       (PTR)&scb->scb_sscb.sscb_ics.ics_srupass,
	       sizeof(scb->scb_sscb.sscb_ics.ics_rupass))
		    != 0)
	{
		/*
		** Password doesn't match
		*/
		status=E_DB_ERROR;
		break;
	}
    } while(loop);

    /*
    ** If not doing alarms/auditing we are done here
    */
    if(do_alarm_audit==FALSE)
	return status;

    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
    {
	sxfaccess=SXF_A_AUTHENT;
	if(status==E_DB_OK)
	    sxfaccess|=SXF_A_SUCCESS;
	else
	    sxfaccess|=SXF_A_FAIL;
	/*
	** Security audit failed password usage
	*/
	local_status = sc0a_write_audit(
	    scb,
	    SXF_E_USER,	/* Type */
	    sxfaccess,
	    scb->scb_sscb.sscb_ics.ics_terminal.db_term_name,
	    sizeof( scb->scb_sscb.sscb_ics.ics_terminal.db_term_name),
	    NULL,			/* Object Owner */
	    I_SX2024_USER_ACCESS,	/* Mesg ID */
	    TRUE,			/* Force record */
	    scb->scb_sscb.sscb_ics.ics_rustat,
	    &error			/* Error location */
	    );	
	if (local_status != E_DB_OK)
	{
	    sc0e_0_put(error.err_code, 0);
	    status=E_DB_ERROR;
	}
    }
    if(status!=E_DB_OK)
    {
	/* Fire db security alarms on failure 
	** (this assumes we will be disconnecting right away
	** on password failure)
	*/
        (VOID) scs_fire_db_alarms(scb, DB_CONNECT|DB_ALMFAILURE);
    }
    else
    {
	/* Fire db security alarms on success 
	*/
        (VOID) scs_fire_db_alarms(scb, DB_CONNECT|DB_ALMSUCCESS);
    }
    return status;
}

/*
** Name: scs_check_app_code - Check application code usage
**
** Description:
**	This routine checks whether a session may use a requested application
**	code.
**
** Inputs:
**	SCB
**
** Returns:
**	E_DB_OK - Access is allowed
**
**	E_DB_ERROR - Access not allowed
**
** History:
**	18-mar-94 (robf)
**          Modularized out of scs_dbdb_info()
**	24-mar-94 (robf)
**          Reworked DBA_DESTROYDB. This operates by connecting to the
**	    iidbdb, so we restrict to that.
**	12-dec-94 (forky01)
**	    Fix bug 65946 - Now checks for ingres (case insensitive)user for
**	    authorization to use createdb iidbdb and also upgradedb.
**	    Previously just did MEcmp on ingres.
*/
static DB_STATUS
scs_check_app_code(SCD_SCB *scb, SCS_DBPRIV *idbpriv, bool is_dbdb)
{
    char 	tmpbuf[32];
    DB_STATUS 	status=E_DB_OK;

    if (scb->scb_sscb.sscb_ics.ics_appl_code >= DBA_MIN_APPLICATION &&
	 scb->scb_sscb.sscb_ics.ics_appl_code <= DBA_MAX_APPLICATION )
    {
	switch (scb->scb_sscb.sscb_ics.ics_appl_code)
	{
	case DBA_CREATEDB:
	    /*
	    ** These are DBA-related operations, so may only be  specified
	    ** by the DBA, or a user with DB_ADMIN priv, or a user  with
	    ** CREATEDB priv.
	    */
    	    if (!((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_UCREATEDB)
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY)
		||
	       (idbpriv->fl_dbpriv & DBPR_DB_ADMIN))
	       )
	    {
		STprintf(tmpbuf,"DBA-related application code %d",
				scb->scb_sscb.sscb_ics.ics_appl_code);
	    	sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     (i4)STlength(tmpbuf),(PTR)tmpbuf,
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
		status=E_DB_ERROR;
	    }
	    break;
	case DBA_OPTIMIZEDB:
	    /*
	    ** Optimizedb is allowed for DBA, DB_ADMIN, TABLE_STATISTICS
	    */
    	    if (!((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
		||
	       (idbpriv->fl_dbpriv & DBPR_TABLE_STATS)
		||
	       (idbpriv->fl_dbpriv & DBPR_DB_ADMIN))
	       )
	    {
		STprintf(tmpbuf,"Optimizedb/Statdump code %d",
				scb->scb_sscb.sscb_ics.ics_appl_code);
	    	sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     (i4)STlength(tmpbuf),(PTR)tmpbuf,
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
		 status=E_DB_ERROR;
	    }
	    break;

	case DBA_SYSMOD:
	    /*
	    ** SYSMOD may only be specified by the DBA, or a user with 
	    ** DB_ADMIN priv, or a user with SECURITY priv.
	    ** 
	    ** NOTE SYSMOD case must be different from DBA_*_VFDB cases
	    ** because like destroydb, sysmod connects to iidbdb
	    ** and we must allow this connection.
	    ** however the connection to the database specified for sysmod
	    ** must be validated.  
	    */
    	    if (!((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
		|| (scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY) ))
	    {
		if (is_dbdb)
		{
		    /*
		    ** Connecting to iidbdb with DBA_SYSMOD flag
		    ** make sure this user has createdb permission
		    */
		    if (scb->scb_sscb.sscb_ics.ics_maxustat & DU_UCREATEDB)
			break;
		}
		else if (idbpriv->fl_dbpriv & DBPR_DB_ADMIN)
		    break;

		
		STprintf(tmpbuf,"DBA-related application code %d",
				scb->scb_sscb.sscb_ics.ics_appl_code);
	    	sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     (i4)STlength(tmpbuf),(PTR)tmpbuf,
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );

		 status=E_DB_ERROR;
	    }
	    break;

	case DBA_PRETEND_VFDB:
	case DBA_FORCE_VFDB:
	case DBA_PURGE_VFDB :
	case DBA_NORMAL_VFDB:
	    /*
	    ** These are DBA-related operations, so may only be  specified
	    ** by the DBA, or a user with DB_ADMIN priv, or a user  with
	    ** SECURITY priv.
	    */
    	    if (!((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY)
		||
	       (idbpriv->fl_dbpriv & DBPR_DB_ADMIN))
	       )
	    {
		STprintf(tmpbuf,"DBA-related application code %d",
				scb->scb_sscb.sscb_ics.ics_appl_code);
	    	sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     (i4)STlength(tmpbuf),(PTR)tmpbuf,
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );

		 status=E_DB_ERROR;
	    }
	    break;
	case DBA_DESTROYDB:
	    /*
	    ** Destroydb connects to iidbdb to destroy a database. 
	    ** (by executing system owned database procedure)
	    ** Restrict access to connections to iidbdb, 
	    ** by user having CREATEDB, SECURITY or DB_ADMIN privilege
	    */
	    if (is_dbdb)
	    {
		if ((scb->scb_sscb.sscb_ics.ics_maxustat & DU_UCREATEDB)
		    || (scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY)
		    || (idbpriv->fl_dbpriv & DBPR_DB_ADMIN))
		    break;
	    }

	    STprintf(tmpbuf,"Application code %d",
			    scb->scb_sscb.sscb_ics.ics_appl_code);
	    sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     (i4)STlength(tmpbuf),(PTR)tmpbuf,
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0, 0, (PTR)0, 0, (PTR)0 );

	    status=E_DB_ERROR;
	    break;
	case DBA_CONVERT:
    	    /*
    	    ** Restrict upgradedb to either DBA or users with SECURITY
	    ** privilege, or  ingres account (backwards compatibility)
    	    */
    	    if (!((scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA)
		||
		(scb->scb_sscb.sscb_ics.ics_maxustat & DU_USECURITY)
		||
	       (idbpriv->fl_dbpriv & DBPR_DB_ADMIN))
	       )
       	    {
	    	 sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		     sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
		     sizeof("Createdb IIDBDB application code")-1, 
			"Createdb IIDBDB application code",
		     sizeof(SystemProductName), SystemProductName,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
		 status=E_DB_ERROR;
	    }
	    break;
	case DBA_IIFE:
	case DBA_W4GLRUN_LIC_CHK:
	case DBA_W4GLDEV_LIC_CHK:
	case DBA_W4GLTR_LIC_CHK:
	   /*
	   ** This is a normal INGREs frontend, no extra checks
	   */
	   break;
	case DBA_CREATE_DBDBF1:
	    break;
	case DBA_FINDDBS:
	case DBA_VERIFYDB:
	default:
	    /*
	    ** Unknown/unused app code, reject
	    */
	    STprintf(tmpbuf,"Invalid application code %d",
			scb->scb_sscb.sscb_ics.ics_appl_code);
	    sc0e_put(E_SC034F_NO_FLAG_PRIV, 0, 3, 
		 sizeof (scb->scb_sscb.sscb_ics.ics_rusername), 
		    (PTR)&scb->scb_sscb.sscb_ics.ics_rusername,
	         (i4)STlength(tmpbuf),(PTR)tmpbuf,
		 sizeof(SystemProductName), SystemProductName,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0 );
	     status=E_DB_ERROR;
	     break;
	}
   }
   return status;
}

/*
** Name: scs_propagate_privs - propagate privileges
**
** Description:
**	This function propagates session privilege changes to other
**	facilities (QEF, SXF, PSF). 
**
** We do our own SCB last so we  can restore if anything goes wrong.
**
** The err_pass variable is used to allow an "undo" loop in case
** changing  one of the  facilities fails - if any fails we loop
** again and reset all to the  current value.
**
** Inputs:
**	scb 	Session to change
**
**	privs   New privilege mask
**
** Outputs:
**	none
**
** Returns:
**	DB_STATUS
**
** History:
**	22-feb-94 (robf)
**         Modularized out from scs_set_priv()
**	21-mar-94 (robf)
**         Ensure UPDATE_SYSCAT is always active if appropriate since this 
**	   is presented as a dbpriv to users and isn't changed with
**	   privileges. 
**	23-may-94 (robf)
**         Update DMF since it stores SECURITY and UPDATE_SYSCAT state.
**	19-Jan-2001 (jenjo02)
**	   Don't call SXF if not enabled for this session.
**	15-Jul-2005 (schka24)
**	    For the security priv, turn on all dbprivs for the session
**	    just like we do if security is set at initial connect time.
**       5-Aug-2005 (hanal04) Bug 114845 INGSTR73
**         Don't call DMF if not enabled for this session.
*/
DB_STATUS
scs_propagate_privs(SCD_SCB *scb, i4 privs)
{
    i4      err_pass=0;
    SXF_RCB 	sxfrcb;
    QEF_ALT	qef_alt[1];
    QEF_RCB	qef_cb;
    DB_STATUS status=E_DB_OK;
    PSQ_CB	*ps_ccb;
    DMC_CB	*dmc;
    SCS_DBPRIV	dbpriv;

    ps_ccb = scb->scb_sscb.sscb_psccb;

    for(;;)
    {
	if(err_pass>=2)
		break;
        /* 
        ** Users don't control activation of UPDATE_SYSCAT (or otherwise)
        ** since its presented as a dbpriv, so we have to make sure its 
	** always active if the session is allowed to use it..
    	*/
    	if( scb->scb_sscb.sscb_ics.ics_maxustat & DU_UUPSYSCAT)
		privs|=DU_UUPSYSCAT;

	/* Update QEF */
	qef_alt[0].alt_code = QEC_USTAT;
	qef_alt[0].alt_value.alt_i4 = privs;

	qef_cb.qef_length = sizeof(QEF_RCB);
	qef_cb.qef_type = QEFRCB_CB;
	qef_cb.qef_ascii_id = QEFRCB_ASCII_ID;
	qef_cb.qef_sess_id = (CS_SID) scb->cs_scb.cs_self;
	qef_cb.qef_asize = 1;
	qef_cb.qef_alt = qef_alt;
	qef_cb.qef_eflag = QEF_EXTERNAL;
	qef_cb.qef_cb = scb->scb_sscb.sscb_qescb;

	status = qef_call(QEC_ALTER, &qef_cb);
	if (status)
	{
		sc0e_0_put(qef_cb.error.err_code, 0);
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
		privs=scb->scb_sscb.sscb_ics.ics_rustat;
		err_pass++;
		continue;
	}

	/* Update SXF */
	if ( scb->scb_sscb.sscb_facility & (1 << DB_SXF_ID) )
	{
	    sxfrcb.sxf_length = sizeof(SXF_RCB);
	    sxfrcb.sxf_cb_type = SXFRCB_CB;
	    sxfrcb.sxf_sess_id = (CS_SID) scb->cs_scb.cs_self;
	    sxfrcb.sxf_alter=SXF_X_USERPRIV;
	    sxfrcb.sxf_ustat=privs;
	    status=sxf_call(SXC_ALTER_SESSION, &sxfrcb);
	    if(status)
	    {
		    sc0e_0_put(sxfrcb.sxf_error.err_code, 0);
		    privs=scb->scb_sscb.sscb_ics.ics_rustat;
		    err_pass++;
		    continue;
	    }
	}

	/* Update DMF */
	if ( scb->scb_sscb.sscb_facility & (1 << DB_DMF_ID) )
	{
	    dmc = scb->scb_sscb.sscb_dmccb;
	    dmc->type = DMC_CONTROL_CB;  
	    dmc->length = sizeof(DMC_CB);
	    dmc->dmc_session_id = (PTR) scb->cs_scb.cs_self;
	    dmc->dmc_op_type = DMC_SESSION_OP;   
	    dmc->dmc_sl_scope = DMC_SL_SESSION;
	    dmc->dmc_flags_mask = DMC_SYSCAT_MODE;

	    /*
	    ** DMC_ALTER has an unusual interface - to toggle flags you
	    ** have to turn off existing flags with DMC_S_NORMAL first,
	    ** then turn on new flags.
	    */
	    /*
	    ** Turn off any existing flags via DMC_NORMAL
	    */
            dmc->dmc_s_type = DMC_S_NORMAL;

	    status = dmf_call(DMC_ALTER, dmc);
	    if (DB_FAILURE_MACRO(status))
	    {
		    sc0e_0_put(dmc->error.err_code, 0);
		    privs=scb->scb_sscb.sscb_ics.ics_rustat;
		    err_pass++;
		    continue;
	    }

	    /*
	    ** Check privileges
	    */
            dmc->dmc_s_type = 0;
            if (privs & DU_UUPSYSCAT)
	            dmc->dmc_s_type |= DMC_S_SYSUPDATE;

	    if (privs & DU_USECURITY)
	            dmc->dmc_s_type |= DMC_S_SECURITY;

	    /*
	    ** Turn on any new flags
	    */
	    if(dmc->dmc_s_type>0)
	    {
	        status = dmf_call(DMC_ALTER, dmc);
	        if (DB_FAILURE_MACRO(status))
	        {
		    sc0e_0_put(dmc->error.err_code, 0);
		    privs=scb->scb_sscb.sscb_ics.ics_rustat;
		    err_pass++;
		    continue;
	        }
	    }
        }


	/* Update PSF */
	
	ps_ccb->psq_ustat=privs;
	ps_ccb->psq_alter_op =  PSQ_CHANGE_PRIVS;
	status = psq_call(PSQ_ALTER, ps_ccb, scb->scb_sscb.sscb_psscb);
	ps_ccb->psq_alter_op =  0;

	if (DB_FAILURE_MACRO(status))
	{
		sc0e_0_put(E_SC0215_PSF_ERROR, 0);
		sc0e_0_put(ps_ccb->psq_error.err_code, 0);
		privs=scb->scb_sscb.sscb_ics.ics_rustat;
		err_pass++;
		continue;
	}
	/* If turning on security priv, give ourselves all dbprivs, just
	** like initial connect does.
	*/
	if (privs & DU_USECURITY)
	{
	    scs_init_dbpriv(scb, &dbpriv, TRUE);
	    status = scs_put_sess_dbpriv(scb, &dbpriv);
	    if (DB_FAILURE_MACRO(status))
	    {
		privs = scb->scb_sscb.sscb_ics.ics_rustat;
		err_pass++;			/* Put 'em back */
		continue;
	    }
	}
	/* Update our own privilege mask */
	scb->scb_sscb.sscb_ics.ics_rustat=privs;
	break;
    }
    if(err_pass>0)
    {
	sc0e_0_put(E_SC034E_PRIV_PROPAGATE_ERROR, 0);
	return E_DB_FATAL;
    }
    else 
	return E_DB_OK;
}
