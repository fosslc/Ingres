/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <er.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSQRECR.C - Recreate procedure.
**
**  Description:
**  This module contains the code necessary to identify an existing QEP
**  for a procedure, or if not found retrieve it from system catalogs
**  and parse it.
**
**	psq_recreate - procedure QP in QSF or recreates it from
**		       definition in system catalogs.
**
**      psq_dbpreg   - Identifies registered db procedure in QSF, in
**                     support of execution of a registered dbproc in star.
**
**
**  History:
**      24-may-88 (stec)
**	    Written.
**	03-aug-88 (stec)
**	    Improve recovery of resources.
**	03-aug-88 (andre)
**	    Modified to ensuree that QSF and RDF objects are released
**	    before leaving the procedure.
**	04-aug-88 (andre)
**	    made changes to check if the present user has permission to execute
**	    a dbproc.
**	23-aug-88 (stec)
**	    pss_ruser -> pss_rusr.
**	27-sep-88 (stec)
**	    Use DB proc owner name from rdf_cb.rdf_rb.rdr_owner.
**	29-nov-88 (stec)
**	    Change code related to translate_or_define.
**	16-mar-89(andre)
**	    Make sure that pss_user contains the real user name before the
**	    psq_recreate() returns.
**	29-mar-89 (andre)
**	    Make sure that we always UNFIX the RDF object and DESTROY the QSF
**	    object containing the query text before leaving psq_recreate().
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    psq_recreate() passes psy_dbpperm() the session's group/appl id's
**	26-mar-89 (neil)
**	    Added support for recreating procedures by owner.
**      07-jul-89 (jennifer)
**          Added auditing of the request to execute a procedure.
**	26-sep-89 (neil)
**	    Quel query followed by SQL procedure that must be recreated needs
**	    to set language to SQL before parsing.
**	11-oct-89 (ralph)
**	    Don't use group/role id's when compiling database procedures.
**	03-jan-90 (ralph)
**	    Change interface to QSO_JUST_TRANS.
**	14-sep-90 (teresa)
**	    change booleans to bitflags for PSQ_ALIAS_SET, PSS_RUSET, 
**	    PSS_RGSET, PSS_RASET
**	27-sep-90 (ralph)
**	    Call psy_dbpperm even if user own procedure, so that procedure
**	    execution can be audited in one centralized place (bug 21243).
**	    Move auditing to psy_dbpperm.
**      02-dec-92 (tam)
**          Add psq_dbpreg() to support execution of registered dbproc in
**          star.
**      08-mar-93 (tam)
**          Change psq_dbpreg() to first search RDF for the proper owner of 
**	    the procedure.
**      10-mar-93 (smc)
**          Commented out text after endif. Cast parameter to psl0_rngent()
**	    function to match prototype.
**	03-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Use pss_cat_owner instead of "$ingres"
**      12-apr-93 (tam)
**          In psq_recreate() and psq_dbpreg(), reuse psq_als_owner to handle 
**	    owner.procedure if PSQ_OWNER_SET is set.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**      16-jun-94 (andre)
**	    Bug #64395
**          it is dangerous to cast a (char *) into a (DB_CURSOR_ID *) and then
**          dereference the resulting pointer because the char ptr may not be
**          pointing at memory not aligned on an appopriate boundary.  Instead,
**          we will allocate a DB_CURSOR_ID structure on the stack, initialize
**          it and MEcopy() it into the char array
**	22-jul-97 (sarjo01)
**	    Bug 77372: Clear PSS_DISREGARD_GROUP_ROLE_PERMS flag set by 
**	    psq_parseqry() to rebuild procedure. This flag was causing 
**	    group and role privileges to be ignored on subsequent execution
**	    of db procs in the session.
**      23-Jul-98 (wanya01)
**          X-integrate change 433785 (sarjo01)
**          in psq_recreate()
**          Bug 76945: The db language setting must be SQL in the adf control
**          block prior to parsing the procedure.
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids().
**
[@history_template@]...
**/

/*{
** Name: psq_recreate	- Identifies procedure QP in memory or recreates
**			  the procedure tree.
**
** Description:
**	This module verifies that a procedure is define in the system
**	catalogs and tries to identify Query Execution Plan for the procedure
**	in QSF memory. If the plan is not present parser is invoked to 
**	recreate the procedure tree. The query mode returned for these two
**	cases is different, because the sequencer needs to take different
**	course of action for each one of the cases.
**
** Inputs:
**	psq_cb		    - query control block.
**	sess_cb		    - session control block.
**
** Outputs:
**	psq_cb
**	    .psq_mode	    - query mode
**
**	Returns:
**	    E_DB_OK status if succeeded, error otherwise.
**	Exceptions:
**	    None.
**
** Side Effects:
**	    Allocates QSF memory for the procedure tree if the QP cannot
**	    be located.
**
** History:
**      24-may-88
**	    Created.
**	03-aug-88 (stec)
**	    Improve recovery of resources.
**	03-aug-88 (andre)
**	    Modified the procedure to assure that under all conditions
**	    QSF object is released before returning.  Also made a change to
**	    ensure that RDF object is released as soon as it is no longer
**	    needed.
**	03-aug-88 (andre)
**	    Declare rdf_cb to pass to psy_gproc and psy_dbpproc
**	04-aug-88 (andre)
**	    Added a check to see if the user has permission to execute
**	    this procedure.
**	23-aug-88 (stec)
**	    pss_ruser -> pss_rusr.
**	27-sep-88 (stec)
**	    Use DB proc owner name from rdf_cb.rdf_rb.rdr_owner.
**	29-nov-88 (stec)
**	    Change code related to translate_or_define to reflect
**	    changes in the definition of QSO_OBID.
**	16-mar-89(andre)
**	    Make sure that pss_user contains the real user name before the
**	    psq_recreate() returns.  The following changes were made:  interface
**	    to psy_gproc was changes so that psy_gproc no longer sets any flags
**	    or switches owner names, instead it may set a ptr to the alternate
**	    name to be used.  PSQ_RECREATE() would set the flag and switch the
**	    names immediately before calling psq_parseqry() and would undo these
**	    changes immediately afterwards.
**	29-mar-89 (andre)
**	    Make sure that we always UNFIX the RDF object and DESTROY the QSF
**	    object containing the query text before leaving psq_recreate().
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    psq_recreate() passes psy_dbpperm() the session's group/appl id's
**	26-mar-89 (neil)
**	    Added support for recreating procedures by owner.
**      07-jul-89 (jennifer)
**          Added auditing of the request to execute a procedure.
**	26-sep-89 (neil)
**	    Quel query followed by SQL procedure that must be recreated needs
**	    to set language to SQL before parsing.
**	11-oct-89 (ralph)
**	    Don't use group/role id's when compiling database procedures.
**	1-jun-90 (andre)
**	    interface for psy_gproc() has changed somewhat.
**	13-sep-90 (teresa)
**	    some flags have become bitmasks: PSS_RUSET and PSQ_ALIAS_SET and
**	    PSS_RASET and PSS_RGSET
**	27-sep-90 (ralph)
**	    Call psy_dbpperm even if user own procedure, so that procedure
**	    execution can be audited in one centralized place (bug 21243).
**	    Move auditing to psy_dbpperm.
**	20-feb-91 (andre)
**	    As a part of fix for bug #32457, pss_raset, pss_raplid, pss_rgset,
**	    and pss_rgroup have been removed from PSS_SESBLK.
**	    Also, interface to psy_dbpperm() has changed.
**	20-sep-91 (andre)
**	    set psq_mode to PSQ_EXEDBP before calling psy_dbpperm()
**	30-sep-91 (andre)
**	    before calling psq_parseqry(), set sess_cb->pss_dependencies_stream
**	    to &sess_cb->pss_ostream.
**	12-feb-92 (andre)
**	    rather than passing a flag field to indicate whether the user
**	    posessed required privilege, we will pass an address of a map
**	    describing privilege - if upon return it is non-zero, user lacked
**	    required privilege
**	19-may-92 (andre)
**	    if recreating a dbproc owned by the current user which is not marked
**	    as at least ACTIVE, we will invoke psy_dbp_status() to check whether
**	    it is, in fact, dormant.
**
**	    we should not have pss_dependencies_stream point at pss_ostream
**	    since if we need to call psy_dbp_status(), pss_dependencies_stream
**	    will point at a stream with a handle which has been zeroed out -
**	    instead, in pslsgram.yi, we will open a new stream and have
**	    pss_dependencies_stream point at it; after calling psq_parseqry(),
**	    this function will be responsible for closing the stream pointed to
**	    by pss_dependencies_stream before returning.
**	27-jun-92 (andre)
**	    If recreating a dbproc owned by the current user and
**	    IIPROCEDURE.dbp_mask1 indicated that it is not active and that
**	    IIDBDEPENDS/IIPRIV do not contain the independent object/privilege
**	    list for the dbproc, but one was built in the course of reparsing
**	    the dbproc, enter the list into appropriate catalogs and modify
**	    IIPROCEDURE.dbp_mask1 to indicate that the list exists before
**	    calling psy_dbp_status() to verify that the dbproc is active
**	09-sep-92 (andre)
**	    psy_dbp_status() no longer accepts or returns an indicator of
**	    whether cache had to be flushed in psy_dbp_priv_check()
**	29-sep-92 (andre)
**	    renamed RDR_2DBP_STATUS to RDR2_DBP_STATUS
**	07-nov-92 (andre)
**	    we will no longer be creating private and public aliases for dbproc
**	    QPs because we claim that a permit on a dbproc will be allowed to
**	    remain in existsnce only if the dbproc is grantable; fot the same
**	    reason we'll stop setting PSS_1DBP_MUSTBE_GRANTABLE in pss_dbp_flags
**	12-apr-93 (tam)
**	    In support of owner.procedure, if PSQ_OWNER_SET is set by SCF, 
**	    use psq_als_owner for the owner field.  Note that psq_als_owner
**	    is already used for another purpose (to bypass permission check
**	    in an internal recreate situation).  But should be ok to reuse.
**	28-jul-93 (andre)
**	    sess_cb->pss_indep_objs.psq_grantee was left uninitialized when
**	    calling RDF to enter independent object/privilege list - very bad
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**	27-aug-93 (andre)
**		(fix for bug 54348)
**	    if an error occurs AFTER we call psq_parseqry() to reparse dbproc 
**	    definition, call psq_destr_dbp_qep() to destroy QEP object created 
**	    in create_dbproc: production
**	01-sep-93 (jhahn)
**	    Added support for multiple query plans for set input procedures.
**	01-sep-93 (andre)
**	    if the dbproc was marked dormant and now it appears to be active,
**	    sess_cb->pss_dbp_ubt_id may contain id of a base table (other than 
**	    a core catalog) on which this dbproc depends (if no such table was 
**	    found, sess_cb->pss_dbp_ubt_id will contain (0,0)).
**	    if the independent object/privilege list for it is non-empty, we 
**	    copy sess_cb->pss_dbp_ubt_id into proctuple.db_dbp_ubt_id to ensure
**	    that it gets stored in IIPROCEDURE tuple
**	12-jan-94 (andre)
**	    (fix for bug 58048)
**	    Here's my explanations of what caused the server to hang:
**	      - several sessions attempted to execute a database procedure P 
**		which for some reason (e.g. a table used in it got dropped and 
**		recreated) got marked dormant, but did not have a QP id, so 
**		they send INVPROC with a bogus id which brought us into 
**		psq_recreate()
**	      - since the QP object was yet to be created QSOJUST_TRANS call 
**		from psq_recreate() failed to locate an existing QP plan which 
**		resulted in all sessions proceeding to reparse the dbproc - 
**		side effect of this was that they all got a S lock on the page 
**		where the IIPROCEDURE tuple describing P resides (because when 
**		recreating a dbproc we tell RDF to ensure that we see no 
**		stale entries)
**	      - eventually all sessions finished reparsing the dbproc and tried 
**		to QSO_TRANS_OR_DEFINE a new QP object - since the object did 
**		not exist yet, one of the sessions created the object (and got 
**		an exclusive lock on it) while the others were left waiting on 
**		the lock (all the while holding a shared lock on IIPROCEDURE 
**		page)
**	      - the session that acquired exclusive lock on the QP object built
**		a list of objects and privileges on which the dbproc depends 
**		and tried to enter it into IIPRIV + to update IIPROCEDURE to 
**		indicate that the list exists and that the dbproc is no longer 
**		dormant; unfortunately, this required that it acquire an 
**		exclusive lock on an IIPROCEDURE page where P's description 
**		resided, and that it could not do because other sessions were 
**		holding shared lock on it
**
**	    To fix this bug, we will ensure that no two sessions (in a given 
**	    server) go through the motions of reparsing and recreating any 
**	    given dbproc.  The first session to get into psq_recreate() will 
**	    perform all work required to recreate a dbproc (including reparsing
**	    it and building an independent object/privilege list, if necessary)
**	    and the subsequent sessions will wait until the plan has been built
**	    and will never even try to repqrse the dbproc because they will 
**	    find the QP while still in psq_recreate().  This will have a 
**	    positive effect of fixing this bug and will help us avoid having 
**	    several sessions perform redundant work in the bargain.
**
**	    In order to ensure that no two sessions in a server attempt to 
**	    recreate the same dbproc at the same time, we will 
**	    QSO_TRANS_OR_DEFINE a QP object BEFORE (this is important) looking 
**	    up a dbproc definition.  This will ensure that the first session 
**	    to get into psq_recreate() will be the only session (if any) to go 
**	    through the full process of reparsing a dbproc and rebuilding and 
**	    updating (if necessary) its independent object/privilege list and 
**	    status.  Things get somewhat more complicated by the fact that we 
**	    may be given only the dbproc name and not the name of the schema in
**	    which it resides (this will happen if an ESQL application invokes 
**	    a dbproc without specifying name of the schema in which it resides.)
**
**	    In order to ensure that no two sessions go through the motions of 
**	    reparsing a dbproc, we will TRANS_OR_DEFINE a QP object BEFORE
**	    looking up definition of a dbproc by the current user, and 
**	    if unsuccessful, repeat the same for DBA and $INGRES.  If an 
**	    application invokes DBA's (or $ingres') dbproc without using 
**	    schema.object construct and the QP does not exist (or is not known 
**	    to the application), this will result in a slight increase in the 
**	    cost of executing the dbproc (since we may end up creating and 
**	    destroying QSF objects for QPs that turned out to not exist), but 
**	    assuming that in most cases user will be executing his own dbproc, 
**	    this change will result in no additional expense.  Furthermore, 
**	    because we will prevent multiple sessions from unnecessarily 
**	    reparsing the same dbproc, this will actually result in a saving 
**	    of CPU resources because only one session will go through the 
**	    reparsing process
**	13-jan-94 (andre)
**	    since now we may ask psy_gproc() whether the current user owns a 
**	    dbproc by a given name, then the DBA, and then $ingres, we no 
**	    longer want it to produce an error message if it does not find a 
**	    dbproc owned by a specified user - instead it will be up to the 
**	    caller to decide to issue an error message and which error message 
**	    to issue
**	04-mar-93 (andre)
**	    The original fix for bug 58048 was not as successful as I had hoped.
**	    While it has made some matters better, it has not completely 
**	    eliminated the cause of undetectable deadlocks (one session could 
**	    still end up holding a lock on a QSF object and attempt to acquire 
**	    X lock on a catalog page while other session(s) would be waiting 
**	    for a lock on QSF object all the while holding an S lock on the 
**	    catalog page of interest to the first session.)  The earlier fix 
**	    handled the situation described in the comment from (12-jan-94), but
**	    it was helpless against the scenario executed by our fine ITB 
**	    benchmark:
**	      briefly, ITB creates the relevant tables in the setup part but 
**	      also drops and recreates them before spanning the individual 
**	      sessions in the driver section.  As a result, after the first 
**	      invocation of the driver the following sequence of events 
**	      results in a hung server:
**		- ITB master drops and recreates a table used in the dbproc
**		- all sessions come into psq_recreate(), execute TRANS_OR_DEFINE
**		  and find the existing QP (no one knows yet that it was 
**		  rendered obsolete by destruction and recreation of a table 
**		  used in the dbproc)
**		- all sessions look up definition of the dbproc to make sure 
**		  they can execute it, acquiring in the process an S lock on 
**		  IIPROCEDURE page describing the dbproc
**		- QEF tries to execute the QP and discovers that it has become 
**		  obsolete - all sessions find themselves again in 
**		  psq_recreate()
**		- the first session to do TRANS_OR_DEFINE ends up DEFINing while
**		  the remaining sessions will wait on the QSF object,
**		- the first session will proceed to parse the text of the dbproc
**		  and attempt to update IIPROCEDURE tuple to indicate that the 
**		  dbproc is no longer dormant - unfortunately, the sessions 
**		  waiting for the QSF object are holding an S lock on 
**		  IIPROCEDURE page preventing the first session from updating 
**		  it - the server hangs.
**
**	    If we could guarantee that all sessions are connected to the 
**	    same server (or if we had a mechanism to syncronize QSF caches
**	    across servers in a given installation), it would suffice to alter 
**	    code in QEU that marks a dbproc dormant to also destroy its QP;
**
**	    However, given that we canNOT guarantee that all sessions will be
**	    connected to the same server AND we do NOT have a mechanism to 
**	    syncronize QSF caches across servers in a given installation, we 
**	    must settle for turning an undetectable deadlock into a 
**	    detectable one.  This will be accomplished by invoking 
**	    JUST_TRANS to determine whether a QP exists before diving into 
**	    psq_parseqry() and, failing that, parsing a dbproc and attempting 
**	    to update IIPROCEDURE (and IIDBDEPENDS/IIPRIV) before invoking
**	    TRANS_OR_DEFINE.
**
**	    Then, the scenario described above will result in multiple sessions 
**	    holding S locks on IIPROCEDURE page trying to update that page 
**	    resulting in a detectable deadlock.  DMF will rollback all but one 
**	    session which will succeed in updating the IIPROCEDURE page and 
**	    DEFINing the QP object.  The sessions which were rolled back in 
**	    the process of resolving the deadlock will come back into 
**	    psq_recreate(), wait for the lucky session to commit a transaction,
**	    and then discover that a QP for the dbproc already exists.
**      22-jul-97 (sarjo01)
**          Bug 77372: Clear PSS_DISREGARD_GROUP_ROLE_PERMS flag set by
**          psq_parseqry() to rebuild procedure. This flag was causing
**          group and role privileges to be ignored on subsequent execution
**          of db procs in the session.
**      23-Jul-98 (wanya01)
**          X-integrate change 433785 (sarjo01)
**          in psq_recreate()
**          Bug 76945: The db language setting must be SQL in the adf control
**          block prior to parsing the procedure.
**	17-feb-09 (wanfr01)
**	    Bug 121543
**	    Flag qsf object as a repeated query/database procedure prior to
**	    recreation.
*/
DB_STATUS
psq_recreate(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *sess_cb)
{
    DB_STATUS	    status, stat;
    QSF_RCB	    txt_qsf_rb;
    QSF_RCB	    dbp_qsf_rb;
    RDF_CB	    rdf_cb;
    i4	    err_code;
    i4		    gproc_mask;
    DB_OWN_NAME	    *dbp_owner;
    DB_DBP_NAME	    *dbp_name = (DB_DBP_NAME *) psq_cb->psq_cursid.db_cur_name;
    PSS_DBPALIAS    dbp_fe_id;
    DB_CURSOR_ID    dbp_be_id;
    i4		    gproc_flags;
    bool	    check_if_active;
    bool	    enter_indep_list;
    bool	    need_parse;
    i4              save_qlang;
				
				/*
				** if an error occurs AFTER we defined a QP 
				** object, this variable will be reset to TRUE 
				** to remind code under exit2: to destroy the
				** dbproc QP object
				*/
    bool	    destr_dbp_qep = FALSE;
				/*
				** if an error occurs AFTER we called 
				** psq_parseqry() to reparse dbproc definition,
				** this variable will be reset to TRUE to remind
				** code under exit2: to invoke to close the 
				** memory stream used to allocate the dbproc 
				** query tree
				*/
    bool	    destr_dbp_tree = FALSE;
    DB_OWN_NAME	    *alt_user;  /*
				** psy_gproc may set alt_user to point to the
				** alternate user name to be used if the dbproc
				** needs to be reparsed
				*/

    /* Initialize the QSF control blocks */
    dbp_qsf_rb.qsf_type = txt_qsf_rb.qsf_type = QSFRB_CB;
    dbp_qsf_rb.qsf_ascii_id = txt_qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    dbp_qsf_rb.qsf_length = txt_qsf_rb.qsf_length = sizeof(QSF_RCB);
    dbp_qsf_rb.qsf_owner = txt_qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
    dbp_qsf_rb.qsf_sid = txt_qsf_rb.qsf_sid = sess_cb->pss_sessid;

    /*
    ** The very first step is to check whether procedure has been
    ** defined in the catalogs.  Also retrieve the procedure text
    ** and place it in QSF.
    */
    if (psq_cb->psq_flag & (PSQ_ALIAS_SET | PSQ_OWNER_SET))
    {
	gproc_mask = PSS_DBP_BY_OWNER;
	dbp_owner = &psq_cb->psq_als_owner;
    }
    else
    {
	gproc_mask = PSS_USRDBP | PSS_DBADBP | PSS_INGDBP;
	dbp_owner = (DB_OWN_NAME *) NULL;
    }

    status = psy_gproc(psq_cb, sess_cb, &txt_qsf_rb, &rdf_cb, &alt_user, 
        dbp_owner, gproc_mask, &gproc_flags);

    if (DB_FAILURE_MACRO(status))
    {
        /*
	** if there was a problem in psy_gproc(), cleanup would occur there
        */
	return(status);
    }
    else if (gproc_flags & PSS_MISSING_DBPROC)
    {
        /*								    
        ** dbproc does not exist - produce an error mesage and return
        */ 
	(VOID) psf_error(2405L, 0L, PSF_USERERR, &err_code,
	    &psq_cb->psq_error, 1,
	    psf_trmwhite(sizeof(DB_DBP_NAME), (char *) dbp_name), 
	    (PTR) dbp_name);

	return(E_DB_ERROR);
    }

    /*
    ** If this point is reached there is a proc in the system catalog
    ** owned by rdf_cb.rdf_rb.rdr_owner.
    ** Next we check if the present user has permission to execute
    ** this procedure.
    ** NOTE: we call psy_dbpperm() even if the dbproc is owned by the current
    **       user.  This is done to ensure that auditing is performed (auditing
    **	     code has been moved to psy_dbpperm().)
    */

    {
	i4		    privs;

	/*
	** set psq_mode to PSQ_EXEDBP so that psy_dbpperm() can detect that the
	** dbproc is being recreated
	*/
	psq_cb->psq_mode = PSQ_EXEDBP;

	/* psy_dbpperm() expects caller to set the required privilege */
	privs = (i4) DB_EXECUTE;
	
	status = psy_dbpperm(sess_cb, &rdf_cb, psq_cb, &privs);

	/*
	** status will indicate FAILURE only if some unexpected error occurs;
	** failure to find required privileges will result in privs being
	** non-zero upon return
	*/
	if (DB_FAILURE_MACRO(status))
	{
	    goto exit1;
	}
	else if (privs)
	{
	    status = E_DB_ERROR;
	    goto exit1;
	}
    }

    /*
    ** At this point we know that the procedure exists, and that the user
    ** may execute it.
    */

    if (alt_user == (DB_OWN_NAME *) NULL)
    {
	/*
	** remember whether the dbproc was marked active - if not,
	** psy_dbp_status() may need to be called to verify that the user may
	** execute his/her dbproc
	*/
	check_if_active =
	    ((rdf_cb.rdf_info_blk->rdr_dbp->db_mask[0] & DB_ACTIVE_DBP) == 0);

	/*
	** remember whether IIPROCEDURE.dbp_mask1 indicated that independent
	** object/privilege list has been entered in catalogs
	*/
	enter_indep_list =
	    ((rdf_cb.rdf_info_blk->rdr_dbp->db_mask[0] & DB_DBP_INDEP_LIST) ==
	     0);
    }
    else
    {
	check_if_active = enter_indep_list = FALSE;
    }
    
    /* 
    ** Now we are ready to look up the QP object.  First, build FE and FE object
    ** ids
    ** (note that we want to build the FE object id before releasing the RDF
    ** object because the RDF object contains the name of the dbproc owner and
    ** that name is used in generating the FE object id)
    */
    psq_dbp_qp_ids(sess_cb, dbp_fe_id, &dbp_be_id, dbp_name, 
	(DB_OWN_NAME *) &rdf_cb.rdf_rb.rdr_owner);

    /* release RDF object */
    status = rdf_call(RDF_UNFIX, (PTR) &rdf_cb);

    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_rdf_error(RDF_UNFIX, &rdf_cb.rdf_error, &psq_cb->psq_error);

        goto exit2;
    }
    
    dbp_qsf_rb.qsf_feobj_id.qso_type = QSO_ALIAS_OBJ;
    dbp_qsf_rb.qsf_feobj_id.qso_lname = sizeof(dbp_fe_id);

    (VOID)MEcopy((PTR) dbp_fe_id, sizeof(dbp_fe_id),
        (PTR) dbp_qsf_rb.qsf_feobj_id.qso_name);

    /*
    ** if this is NOT a set-input dbproc, try to look up its QP in QSF - if one
    ** is found, we will forego reparsing the dbproc text and return QP object's
    ** BE id to SCF inside psq_cb->psq_cursid.  If this IS a set-input dbproc,
    ** we will always reparse the query text.
    */
    if (psq_cb->psq_set_input_tabid.db_tab_base != 0)
    {
	need_parse = TRUE;
    }
    else
    {
	/* See if alias already exists. */
	dbp_qsf_rb.qsf_lk_state = QSO_FREE;
	status = qsf_call(QSO_JUST_TRANS, &dbp_qsf_rb);
	if (DB_FAILURE_MACRO(status))
	{
	    if (dbp_qsf_rb.qsf_error.err_code != E_QS0019_UNKNOWN_OBJ)
	    {
		/* some unexpected error */
		(VOID) psf_error(E_PS037A_QSF_TRANS_ERR,
		    dbp_qsf_rb.qsf_error.err_code, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);

		goto exit2;
	    }
	    else
	    {
		/* QP object does not exist */
		need_parse = TRUE;
	    }
	}
	else
	{
	    /* QP object exists - we will not have to reparse the dbproc */
	    need_parse = FALSE;
	}
    }

    if (need_parse)
    {
	if (alt_user != (DB_OWN_NAME *) NULL)
	{
	    /*
	    ** If the owner of the dbproc is different from the current
	    ** user, temporarily set pss_user to the name of the dbproc
	    ** owner so that dbproc may be parsed in the context of its
	    ** owner
	    */

	    /* save current user's name */
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, sess_cb->pss_rusr);

	    /* replace it with the dbproc's owner name */
	    STRUCT_ASSIGN_MACRO(*alt_user, sess_cb->pss_user);

	    sess_cb->pss_dbp_flags |= PSS_RUSET;  /* pss_ruset = TRUE */
	}

	/*
	** We need to supply QSF object's BE id so that it can be copied into 
	** pnode->pst_dbpid of the dbproc tree.  
	**
	** Note that we are yet to TRANS_OR_DEFINE the QP object.  If we DEFINE
	** it, the correct BE id will be embedded in the tree.  If, on the other
	** hand, we TRANSlate, if this is NOT a set-input dbproc, the tree will
	** be destroyed, so there is not a thing to worry about, but if this IS 
	** a set-input dbproc, we need to store translated object id into 
	** pnode->pst_dbpid (pnode is at the root of the query tree object)
	** as well as in psq_cb->psq_cursid
	*/
	STRUCT_ASSIGN_MACRO(dbp_be_id, psq_cb->psq_cursid);

	/*
	** Call psq_parseqry() to parse the text retrieved by the call to
	** psy_gproc().  
	*/
	
	psq_cb->psq_qlang = DB_SQL;
        save_qlang = ((ADF_CB *)(sess_cb->pss_adfcb))->adf_qlang;
        ((ADF_CB *)(sess_cb->pss_adfcb))->adf_qlang = DB_SQL;

	status = psq_parseqry(psq_cb, sess_cb);

        ((ADF_CB *)(sess_cb->pss_adfcb))->adf_qlang = save_qlang;
	sess_cb->pss_stmt_flags &= ~PSS_DISREGARD_GROUP_ROLE_PERMS;

	/* Restore the real user name as soon as possible. */
	if (sess_cb->pss_dbp_flags & PSS_RUSET)
	{
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_rusr, sess_cb->pss_user);
	    sess_cb->pss_dbp_flags &= ~PSS_RUSET;  /* pss_ruset = FALSE */
	}

	/*
	** note that as a result of changes we made here, it would be impossible
	** for an existing QP to be found in create_dbproc: production (because
	** if recreating a dbproc, we will NOT EVEN TRY to look up an existing 
	** QP object, the understanding being that it was already done here.
	*/
	if (DB_SUCCESS_MACRO(status))
	{
	    /*
	    ** remember that a dbproc tree object has been created; if any error
	    ** occurs from this point on, we must remember to destroy it
	    */
	    destr_dbp_tree = TRUE;

	    /*
	    ** reset psq_mode (which would be set to PSQ_CREDBP in 
	    ** create_dbproc: production) to PSQ_EXEDBP
	    **
	    ** by the by, SCF does not seem to complain even if we do not reset
	    ** psq_mode (i.e. if it stays set to PSQ_CREDBP); it must keep
	    ** enough info to determine that even though PSF claims that we are
	    ** creating a new dbproc, we are in fact recreating an existing
	    ** dbproc (so, why do we bother resetting psq_mode?  historical
	    ** reasons?.. who knows...)
	    */
	    psq_cb->psq_mode = PSQ_EXEDBP;

	    /*
	    ** if IIPROCEDURE.dbp_mask1 indicated that there was no independent
	    ** object/privilege list associated with the dbproc, but one was
	    ** built in the course of reparsing the dbproc, enter it into
	    ** IIDBDEPENDS/IIPRIV now and set DB_DBP_INDEP_LIST bit in
	    ** IIPROCEDURE.dbp_mask1.
	    ** Once/if psy_dbp_status() determines that the dbproc is active
	    ** and/or grantable, it will set the remaining bits in
	    ** IIPROCEDURE.dbp_mask1
	    */
	    if (   enter_indep_list
		&& (   sess_cb->pss_indep_objs.psq_objs
		    || sess_cb->pss_indep_objs.psq_objprivs
		    || sess_cb->pss_indep_objs.psq_colprivs
		   )
	       )
	    {
		register RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
		DB_PROCEDURE		proctuple, *dbp_tuple = &proctuple;

		/* initialize important fields in RDF_CB */
		rdf_rb->rdr_update_op   = RDR_REPLACE;
		rdf_rb->rdr_types_mask  = RDR_PROCEDURE;
		rdf_rb->rdr_2types_mask = RDR2_DBP_STATUS;
		rdf_rb->rdr_status      = DB_SQL;
		rdf_rb->rdr_indep       = (PTR) &sess_cb->pss_indep_objs;
		rdf_rb->rdr_qrytuple    = (PTR) dbp_tuple;
		STRUCT_ASSIGN_MACRO(sess_cb->pss_user, dbp_tuple->db_owner);

		/*
		** remember to set psq_grantee field - otherwise, if recreating
		** a dbproc is the first activity involving
		** sess_cb->pss_indep_objs, we can pretty much guarantee that
		** psq_grantee will not be pointing anywhere in particular
		*/
		sess_cb->pss_indep_objs.psq_grantee = &sess_cb->pss_user;
		
		/*
		** need to pass dbproc name so that QEF can find the
		** tuple in IIPROCEDURE and the new value of dbp_mask1 so
		** QEF can correctly update it
		*/
		STRUCT_ASSIGN_MACRO(rdf_cb.rdf_rb.rdr_name.rdr_prcname,
		    dbp_tuple->db_dbpname);
		dbp_tuple->db_mask[0] = (i4) DB_DBP_INDEP_LIST;

		/*
		** copy id of the dbproc's underlying base table where 
		** qeu_dbp_status() expects to find it
		*/
		dbp_tuple->db_dbp_ubt_id.db_tab_base = 
		    sess_cb->pss_dbp_ubt_id.db_tab_base;
		dbp_tuple->db_dbp_ubt_id.db_tab_index =
		    sess_cb->pss_dbp_ubt_id.db_tab_index;

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0201_PROC_NOT_FOUND)
		    {
			_VOID_ psf_error(E_PS0905_DBP_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psq_cb->psq_error, 1,
			    psf_trmwhite(sizeof(dbp_tuple->db_dbpname),
				(char *) &dbp_tuple->db_dbpname),
			    &dbp_tuple->db_dbpname);
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
			    &psq_cb->psq_error);
		    }

		    goto exit2;
		}
	    }
	    
	    /*
	    ** if the dbproc was owned by the current user and was not marked as
	    ** ACTIVE in IIPROCEDURE, we'll call psy_dbp_status() to verify that
	    ** it is, in fact, not dormant
	    */
	    if (check_if_active)
	    {
		PSY_TBL             dbp_descr;
		i4                  dbp_mask[2];

		MEcopy((PTR) dbp_name,
		    sizeof (DB_DBP_NAME), (PTR) &dbp_descr.psy_tabnm);
		dbp_descr.psy_tabid.db_tab_base =
		    dbp_descr.psy_tabid.db_tab_index = 0;
		status = psy_dbp_status(&dbp_descr, sess_cb, (PSF_QUEUE *) NULL,
		    (i4) PSQ_EXEDBP, dbp_mask, &psq_cb->psq_error);
		if (DB_FAILURE_MACRO(status))
		    goto exit2;
	    }

	    /*
	    ** finally, we are ready to TRANS_OR_DEFINE a QP object.  If we find
	    ** an existing QP object, we must remember to destroy the tree which
	    ** was built as a part of parsing the dbproc and return the QP 
	    ** object id to SCF
	    */

	    dbp_qsf_rb.qsf_obj_id.qso_type  = QSO_QP_OBJ;
	    dbp_qsf_rb.qsf_obj_id.qso_lname = sizeof(DB_CURSOR_ID);

            MEcopy((PTR) &dbp_be_id, sizeof(dbp_be_id),
                (PTR) dbp_qsf_rb.qsf_obj_id.qso_name);
	    
	    dbp_qsf_rb.qsf_lk_state = QSO_FREE;
	    dbp_qsf_rb.qsf_flags = QSO_DBP_OBJ;
	    status = qsf_call(QSO_TRANS_OR_DEFINE, &dbp_qsf_rb);

	    if (DB_FAILURE_MACRO(status))
	    {
		(VOID) psf_error(E_PS0379_QSF_T_OR_D_ERR,
		    dbp_qsf_rb.qsf_error.err_code, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);
		
		goto exit2;
	    }

	    if (dbp_qsf_rb.qsf_t_or_d == QSO_WASTRANSLATED)
	    {
		/*
		** if recreating procedure and find an existing one,
		** it means someone else recreated it while we were
		** parsing it, so just use their QEP.   (fixes bug 39014)
		** ... unless, of course, we are parsing adefinition of a 
		** set-input dbproc, in which case we allow multiple query 
		** plans.  If two sessions create the same one at the same
		** time one of them will eventualy get LRU'd out.
		*/
		
		if (sess_cb->pss_dbp_flags & PSS_SET_INPUT_PARAM)
		{
		    QSF_RCB		tree_qsf_rb;
		    PST_PROCEDURE   	*pnode;

		    /* 
		    ** need to get the handle of query tree object and store 
		    ** the translated object id into pnode->pst_dbpid
		    */
		    tree_qsf_rb.qsf_type = QSFRB_CB;
		    tree_qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
		    tree_qsf_rb.qsf_length = sizeof(tree_qsf_rb);
		    tree_qsf_rb.qsf_owner = (PTR) DB_PSF_ID;
		    tree_qsf_rb.qsf_sid = sess_cb->pss_sessid;
		    tree_qsf_rb.qsf_obj_id.qso_handle = 
			psq_cb->psq_result.qso_handle;
		    
		    status = qsf_call(QSO_INFO, &tree_qsf_rb);
		    if (DB_FAILURE_MACRO(status))
		    {
			(VOID) psf_error(E_PS0A0A_CANTGETINFO, 
			    tree_qsf_rb.qsf_error.err_code, PSF_INTERR, 
			    &err_code, &psq_cb->psq_error, 0);

			goto exit2;
		    }

		    pnode = (PST_PROCEDURE *) tree_qsf_rb.qsf_root;

		    MEcopy((PTR) dbp_qsf_rb.qsf_obj_id.qso_name,
		        sizeof(DB_CURSOR_ID),
			(PTR) &pnode->pst_dbpid);
		}
		else
		{
		    PSF_MSTREAM		dbp_treestream;

		    /*
		    ** All we need to do is return the new ID, reset the
		    ** query mode to indicate that a QP already exists, and
		    ** destroy the now useless query tree
		    */
		    MEcopy((PTR) dbp_qsf_rb.qsf_obj_id.qso_name,
			sizeof(DB_CURSOR_ID),
			(PTR) &psq_cb->psq_cursid);
		    
		    psq_cb->psq_mode = PSQ_QRYDEFED;

		    /* stream has been unlocked in psq_parseqry() */
		    dbp_treestream.psf_mlock = 0;

		    STRUCT_ASSIGN_MACRO(psq_cb->psq_result, 
			dbp_treestream.psf_mstream);

		    /* 
		    ** remember to not try to destroy the query tree should 
		    ** something go wrong from this point on
		    */
		    destr_dbp_tree = FALSE;

		    status = psf_mclose(sess_cb, &dbp_treestream, &psq_cb->psq_error);

		    if (DB_FAILURE_MACRO(status))
			goto exit2;
		}
	    }
	    else
	    {
	        /* 
	        ** from this point on, we may not just return in the event 
	        ** of error - we must goto exit2 to ensure that 
	        ** the QP object gets destroyed
	        */
	        destr_dbp_qep = TRUE;
	    }
	}

	goto exit2;	/* destroy the QSF text object */
    }

    /*
    ** we were told to recreate a NON set-input dbproc and we seemed to have 
    ** found a QP for it already in QSF
    */

    /* Check the type of the translated object */
    if (dbp_qsf_rb.qsf_obj_id.qso_type != QSO_QP_OBJ)
    {
	status = E_DB_SEVERE;

	(VOID) psf_error(E_PS037A_QSF_TRANS_ERR,
	    dbp_qsf_rb.qsf_error.err_code, PSF_INTERR,
	    &err_code, &psq_cb->psq_error, 0);
	goto exit2;
    }

    /* This point is reached when translation was succesful.
    ** All we need to do is return the new ID (one that the FE
    ** id got translated into).
    */
    MEcopy((PTR) dbp_qsf_rb.qsf_obj_id.qso_name, sizeof(DB_CURSOR_ID),
	(PTR) &psq_cb->psq_cursid);

    psq_cb->psq_mode = PSQ_QRYDEFED;

    /* 
    ** RDF object has been unfixed.  We still have to DESTROY the text
    ** QSF object 
    */
    goto exit2;

exit1:
    /* release RDF object */
    stat = rdf_call(RDF_UNFIX, (PTR) &rdf_cb);

    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_rdf_error(RDF_UNFIX, &rdf_cb.rdf_error, &psq_cb->psq_error);

	status = (stat > status) ? stat : status;
    }

exit2:

    if (   sess_cb->pss_dependencies_stream
	&& sess_cb->pss_dependencies_stream->psf_mstream.qso_handle)
    {
	/*
	** close the memory stream which was used to allocate descriptors of
	** objects/privileges on which the dbproc depends
	*/
	stat = psf_mclose(sess_cb, sess_cb->pss_dependencies_stream, &psq_cb->psq_error);
	if (DB_FAILURE_MACRO(stat) && stat > status)
	    status = stat;

	sess_cb->pss_dependencies_stream = (PSF_MSTREAM *) NULL;
    }

    /* Destroy the QSF object holding the query text. */

    /* Lock the object exclusively in order to destroy it. */
    txt_qsf_rb.qsf_lk_state = QSO_EXLOCK;

    stat = qsf_call(QSO_LOCK, &txt_qsf_rb);

    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_error(E_PS0A08_CANTLOCK, txt_qsf_rb.qsf_error.err_code,
		PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	status = (stat > status) ? stat : status;
    }

    /* Destroy it now. */
    stat = qsf_call(QSO_DESTROY, &txt_qsf_rb);

    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_error(E_PS0A09_CANTDESTROY, txt_qsf_rb.qsf_error.err_code,
		PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	status = (stat > status) ? stat : status;
    }

    /*
    ** if a dbproc QP object has been created later some error was encountered,
    ** we need to destroy the QP object AND the dbproc query tree
    */
    if (DB_FAILURE_MACRO(status))
    {
	if (destr_dbp_qep)
	{
	    /*
	    ** if we DEFINEd the object, we hold a semaphore on it, but not a 
	    ** lock; before destroying an object, we need to lock it up first.
	    ** We may not use exclusive lock because there may be other sessions
	    ** waiting on the semaphore which will prevent us from locking the 
	    ** object - therefore we will get a shared lock on the object
	    */

	    dbp_qsf_rb.qsf_lk_state = QSO_SHLOCK;

	    stat = qsf_call(QSO_LOCK, &dbp_qsf_rb);
	    if (DB_FAILURE_MACRO(stat))
	    {
		(VOID) psf_error(E_PS0A08_CANTLOCK, 
		    dbp_qsf_rb.qsf_error.err_code, PSF_INTERR, &err_code, 
		    &psq_cb->psq_error, 0);
		status = (stat > status) ? stat : status;
	    }
	    else
	    {
	        stat = qsf_call(QSO_DESTROY, &dbp_qsf_rb);
	        if (DB_FAILURE_MACRO(stat))
	        {
		    _VOID_ psf_error(E_PS0A09_CANTDESTROY,
		        dbp_qsf_rb.qsf_error.err_code, PSF_INTERR, &err_code,
		        &psq_cb->psq_error, 0);
	            status = (stat > status) ? stat : status;
	        }
	    }
 	}

	if (destr_dbp_tree)
        {
	    PSF_MSTREAM		dbp_treestream;

	    /* stream has been unlocked in psq_parseqry() */
	    dbp_treestream.psf_mlock = 0;

	    STRUCT_ASSIGN_MACRO(psq_cb->psq_result, dbp_treestream.psf_mstream);

	    stat = psf_mclose(sess_cb, &dbp_treestream, &psq_cb->psq_error);
	    status = (stat > status) ? stat : status;
	}
    }

    return(status);
}

/*{
** Name: psq_dbpreg   - Identifies registered db procedure in QSF, in
**			support of execution of a registered dbproc in star.
**
** Description:
**	This function locates a registered dbproc in QSF.
**	If the dbproc is not found in QSF, it will be retrieved from 
**	the system catalog (through RDF).  An QSF QTREE object, containing
**	the ldb description of the actual procedure, will then
**	be created for the dbproc.  Since star doesn't care about the
**	query plan for the dbproc itself, a registered dbproc is more
**	like a registered table and there is no real parsing involved.
**
**	In searching the RDF cache, the user name, the dba
**	or $ingres are used as possible owner for the dbproc.
**
**	Note that psq_dbpreg() first consults with RDF to see if dbproc 
**	still exists in the system catalog.  More importantly, this will
**	find out the right procedure to use in case the dba also has a 
**	procedure with the same name defined.  This is like what the local
**	does in psy_gproc().
**	
**	Currently this function doesn't handle permission.
**	
**	Modelled after psq_recreate, psy_gproc and psl0_rngent.
**
** Inputs:
**	psq_cb		- query control block.
**	sess_cb		- session control block.
**
** Outputs:
**	psq_cb
**	    .psq_mode	- query mode 
**				. PSQ_QRYDEFED if query plan already in QSF. 
**				. PSQ_EXEDBP otherwise.	Then a QTREE object 
**				  and an empty QP_OBJ will be created for dbp. 
**
** Returns:
**	E_DB_OK status if dbproc is found, error otherwise.
**
** Side Effects:
**	Allocates QSF memory for the registered dbproc object
**	if it doesn't already exist.
**
** History:
**	01-dec-92 (tam)
**	    Created.
**	11-dec-92 (tam)
**	    Use QTREE for the QSF object.
**	08-mar-93 (tam)
**	    Change logic to first search RDF for the proper owner of the procedure.
**	12-apr-93 (tam)
**	    Reuse psq_als_owner to handle owner.procedure if PSQ_OWNER_SET is set.
**	03-mar-2008 (toumi01) BUG 120040
**	    Initialize pst_flags lest we make decisions based on random memory.
*/
psq_dbpreg(
	PSQ_CB		*psq_cb,
	PSS_SESBLK	*sess_cb)
{
    DB_CURSOR_ID	be_id;
    DB_STATUS		status;
    DB_STATUS		stat;
    i4		err_code;
    QSF_RCB		qsf_rb;
    RDF_CB		rdf_cb;
    PSS_DBPALIAS	dbpid;		/* the dbproc id template */
    PSS_RNGTAB		*resrange;
    i4			rngvar_info;
    i4			tbls_to_lookup;	/* controls the 3 tier logic */
    i4			lookup_mask;
    DB_OWN_NAME		ingres;
    DB_OWN_NAME		*owner;
    DD_REGPROC_DESC	*regproc_desc;
    DD_LDB_DESC		*ldb_desc;
    DD_OBJ_DESC		*dbp_desc;
    DD_2LDB_TAB_INFO	*dbp_local_desc;
    PST_STATEMENT	*stmt_cp;
    PST_PROCEDURE   	*prnode;    /* Procedure node to create */
    PST_REGPROC_STMT	*cps;

    /* lookup procedure name space only */

    lookup_mask = PST_SHWNAME | PST_REGPROC;

    if (psq_cb->psq_flag & PSQ_OWNER_SET)
    {
	/* Executing owner.procedure */

        owner = &psq_cb->psq_als_owner;
	status = psl0_orngent(&sess_cb->pss_auxrng, -1, "", owner,
			(DB_TAB_NAME *)&psq_cb->psq_cursid.db_cur_name, 
			sess_cb, TRUE, 
			&resrange, psq_cb->psq_mode, &psq_cb->psq_error, 
			&rngvar_info, lookup_mask);
    }
    else 
    {
	/* 
	** Look up dbproc in RDF.  The user can execute a dbproc owned by
	** (1) the user, (2) the dba, or (3) $ingres.  
	*/

	tbls_to_lookup = PSS_USRTBL | PSS_DBATBL | PSS_INGTBL;

	status = psl0_rngent(&sess_cb->pss_auxrng, -1, "",
			(DB_TAB_NAME *)&psq_cb->psq_cursid.db_cur_name, 
			sess_cb, TRUE, &resrange, psq_cb->psq_mode, 
			&psq_cb->psq_error, tbls_to_lookup, &rngvar_info, 
			lookup_mask);
    }

    if (DB_FAILURE_MACRO(status))
    {
    	return(status);
    }

    if (psq_cb->psq_error.err_code == E_PS0903_TAB_NOTFOUND)
    {
	/*
	** If reg proc wasn't found, report it to user here.  
	*/
        (VOID) psf_error(2405L, 0L, PSF_USERERR, &err_code, 
		    &psq_cb->psq_error, 1, 
		    psf_trmwhite(sizeof(DB_TAB_NAME), 
			psq_cb->psq_cursid.db_cur_name), 
		    psq_cb->psq_cursid.db_cur_name);

	return (E_DB_ERROR);
    }

    owner	= &resrange->pss_ownname;

    /*
    ** search in QSF in case the plan is already there 
    */

    qsf_rb.qsf_type 			= QSFRB_CB;
    qsf_rb.qsf_ascii_id 		= QSFRB_ASCII_ID;
    qsf_rb.qsf_length 			= sizeof(qsf_rb);
    qsf_rb.qsf_owner 			= (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid 			= sess_cb->pss_sessid;
    qsf_rb.qsf_lk_state		= QSO_FREE;
    qsf_rb.qsf_obj_id.qso_type		= QSO_QP_OBJ;
    qsf_rb.qsf_obj_id.qso_lname	= sizeof(DB_CURSOR_ID);
    MEcopy((PTR) &psq_cb->psq_cursid, sizeof(DB_CURSOR_ID), 
		(PTR) qsf_rb.qsf_obj_id.qso_name);
    qsf_rb.qsf_feobj_id.qso_type	= QSO_ALIAS_OBJ;
    qsf_rb.qsf_feobj_id.qso_lname	= sizeof(dbpid);

    /* 
    ** In QSF, the alias for the
    ** dbproc id is made up of two i4, followed by the proc name 
    ** (the cursor id), the owner name, and one i4 (the db id).
    */

    MEcopy((PTR) &psq_cb->psq_cursid, sizeof(DB_CURSOR_ID), 
	(PTR) dbpid);
    MEcopy((PTR) owner, DB_MAXNAME, (PTR) ((char *)
	dbpid + sizeof(DB_CURSOR_ID)));
    I4ASSIGN_MACRO(sess_cb->pss_udbid,
        *(i4 *) ((char *) dbpid + sizeof(DB_CURSOR_ID) + DB_MAXNAME));
    MEcopy((PTR) dbpid, sizeof(dbpid), 
    	(PTR) qsf_rb.qsf_feobj_id.qso_name);

    status	= qsf_call(QSO_JUST_TRANS, &qsf_rb);

    /* if found in QSF, we are done.  Remember to set query mode */
    if (DB_SUCCESS_MACRO(status))
    {
	psq_cb->psq_mode = PSQ_QRYDEFED;
        MEcopy((PTR) qsf_rb.qsf_obj_id.qso_name, sizeof(DB_CURSOR_ID),
    	    (PTR) &psq_cb->psq_cursid);	
    	
    	return (E_DB_OK);
    }
    else
    {
	if (qsf_rb.qsf_error.err_code != E_QS0019_UNKNOWN_OBJ)
	{
	    /* some unexpected QSF error */
	    (VOID) psf_error(E_PS037A_QSF_TRANS_ERR,
			qsf_rb.qsf_error.err_code, PSF_INTERR,
			&err_code, &psq_cb->psq_error, 0);

	    return(status);
	}

    	/* 
    	** if not found in QSF, recreate the procedure tree and an empty QP.
    	** First get a new cursor id for the QP.
    	*/

    	be_id.db_cursor_id[0] = (i4) sess_cb->pss_psessid;
    	be_id.db_cursor_id[1] = (i4) sess_cb->pss_crsid++;

    	dbp_desc = resrange->pss_rdrinfo->rdr_obj_desc;
    	dbp_local_desc = &dbp_desc->dd_o9_tab_info;

    	status = psf_mopen(sess_cb, QSO_QTREE_OBJ, &sess_cb->pss_ostream,
                                    &psq_cb->psq_error);
    	if (status != E_DB_OK)
	    return (status);

    	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_STATEMENT),
                            (PTR *)&stmt_cp, &psq_cb->psq_error);
    	if (status != E_DB_OK)
	    return (status);

    	MEfill(sizeof(PST_STATEMENT), (u_char) 0, (PTR) stmt_cp);

    	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DD_REGPROC_DESC),
                            (PTR *)&regproc_desc, &psq_cb->psq_error);
    	if (status != E_DB_OK)
	    return (status);

    	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DD_LDB_DESC),
                            (PTR *)&ldb_desc, &psq_cb->psq_error);
    	if (status != E_DB_OK)
	    return (status);

    	regproc_desc->dd_p5_ldbdesc = ldb_desc;

    	stmt_cp->pst_mode = PSQ_EXEDBP;
    	stmt_cp->pst_type = PST_REGPROC_TYPE;
#ifdef xDEBUG
    	stmt_cp->pst_lineno = sess_cb->pss_lineno;
#endif /* xDEBUG */
    	cps = &stmt_cp->pst_specific.pst_regproc;

    	MEcopy((PTR) &psq_cb->psq_cursid.db_cur_name, 
		DB_MAXNAME, (PTR) &cps->pst_procname);

    	STRUCT_ASSIGN_MACRO(*owner, cps->pst_ownname);

    	cps->pst_rproc_desc = regproc_desc;
	    
    	/* fill in regproc_desc with the local table info from RDF */

    	regproc_desc->dd_p1_local_proc_id.db_cursor_id[0] = 0;
    	regproc_desc->dd_p1_local_proc_id.db_cursor_id[1] = 0;
    	MEcopy((PTR) &dbp_local_desc->dd_t1_tab_name, DB_MAXNAME,
		(PTR) regproc_desc->dd_p1_local_proc_id.db_cur_name);
    	MEcopy((PTR) &dbp_local_desc->dd_t2_tab_owner, DB_MAXNAME,
		(PTR) regproc_desc->dd_p2_ldbproc_owner);
    	regproc_desc->dd_p3_regproc_id.db_cursor_id[0] =
			be_id.db_cursor_id[0];	
    	regproc_desc->dd_p3_regproc_id.db_cursor_id[1] =
			be_id.db_cursor_id[1];	
    	MEcopy((PTR) &dbp_desc->dd_o1_objname, DB_MAXNAME,
		(PTR) regproc_desc->dd_p3_regproc_id.db_cur_name);
    	MEcopy((PTR) &dbp_desc->dd_o2_objowner, DB_MAXNAME,
		(PTR) regproc_desc->dd_p4_regproc_owner);

    	STRUCT_ASSIGN_MACRO(dbp_local_desc->dd_t9_ldb_p->dd_i1_ldb_desc, 
		*(regproc_desc->dd_p5_ldbdesc));

    	/* Create procedure (for OPC) and attach the EXEDBP statement */
    	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(PST_PROCEDURE),
                                (PTR *)&prnode, &psq_cb->psq_error);
    	if (status != E_DB_OK)
	    return (status);

    	prnode->pst_mode = 0;
    	prnode->pst_vsn = PST_CUR_VSN;
    	prnode->pst_isdbp = TRUE;
    	prnode->pst_flags = 0;
    	prnode->pst_stmts = stmt_cp;
    	prnode->pst_parms = (PST_DECVAR *)NULL;
    	prnode->pst_dbpid.db_cursor_id[0] = be_id.db_cursor_id[0];	
    	prnode->pst_dbpid.db_cursor_id[1] = be_id.db_cursor_id[1];	
    	MEcopy((PTR) &psq_cb->psq_cursid.db_cur_name, 
	    sizeof(DB_DBP_NAME), (PTR) prnode->pst_dbpid.db_cur_name);

    	/* Fix the root in QSF */
    	if (prnode != (PST_PROCEDURE *) NULL)
    	{
    	    status = psf_mroot(sess_cb, &sess_cb->pss_ostream, (PTR) prnode,
    	                &psq_cb->psq_error);
    	    if (DB_FAILURE_MACRO(status))
		return (status);
    	}
	
    	/* 
    	** Now we have finished building a QTREE obj for our procedure. 
    	** We need to create a QP object in QSO
    	** because QEPs are shareable.  This is done for the local
    	** case INVPROC, inside the create_dbp production in the grammar.
    	*/

    	MEcopy((PTR) &prnode->pst_dbpid, sizeof(DB_CURSOR_ID),
                (PTR) qsf_rb.qsf_obj_id.qso_name);

    	qsf_rb.qsf_lk_state = QSO_FREE;
	qsf_rb.qsf_flags = QSO_DBP_OBJ;
    	status = qsf_call(QSO_TRANS_OR_DEFINE, &qsf_rb);
	
    	if (DB_FAILURE_MACRO(status))
    	{
	    (VOID) psf_error(E_PS0379_QSF_T_OR_D_ERR,
                    qsf_rb.qsf_error.err_code, PSF_INTERR,
                    &err_code, &psq_cb->psq_error, 0);
	    return (status);
    	}

    	if (qsf_rb.qsf_t_or_d == QSO_WASTRANSLATED)
    	{
	    /* This shouldn't happen but since the grammar file has
	    ** this logic in the create_dbp production (relating to
	    ** bug 39014), what the hack.
	    ** Seems related to some concurrency issue.
            ** All we need to do is return the new ID and reset the
            ** query mode to indicate that a QP already exists
            */
            MEcopy((PTR) qsf_rb.qsf_obj_id.qso_name,
                sizeof(DB_CURSOR_ID), (PTR) &psq_cb->psq_cursid);

            psq_cb->psq_mode = PSQ_QRYDEFED;

            /*
            ** The generated tree is useless, because shared QEP
            ** mechanism will be used, so close the QTREE memory stream
            ** to delete memory.
            */
            status = psf_mclose(sess_cb, &sess_cb->pss_ostream, &psq_cb->psq_error);
            if (DB_FAILURE_MACRO(status))
                return (status);
            sess_cb->pss_ostream.psf_mstream.qso_handle = (PTR) NULL;

	    return (E_DB_OK);
        }
    
        /* 
        ** Got here if QP not found.  A new QTREE and an empty QP created in QSF. 
        */

        /*
        ** Set the QSF id of the return object and unlock it, if any.
        */
        if (sess_cb->pss_ostream.psf_mstream.qso_handle != (PTR) NULL)
        {
            STRUCT_ASSIGN_MACRO(sess_cb->pss_ostream.psf_mstream, psq_cb->psq_result);
            qsf_rb.qsf_obj_id.qso_handle =
                sess_cb->pss_ostream.psf_mstream.qso_handle;
            qsf_rb.qsf_lk_id = sess_cb->pss_ostream.psf_mlock;
            if (status = qsf_call(QSO_UNLOCK, &qsf_rb))
            {
                (VOID) psf_error(E_PS0B05_CANT_UNLOCK,
                    qsf_rb.qsf_error.err_code, PSF_INTERR, &err_code,
                    &psq_cb->psq_error, 0);
                return (status);
            }
            sess_cb->pss_ostream.psf_mlock = 0;
        }
	    
        MEcopy((PTR) qsf_rb.qsf_obj_id.qso_name,
           sizeof(DB_CURSOR_ID), (PTR) &psq_cb->psq_cursid);

        psq_cb->psq_mode = PSQ_EXEDBP;

        return (E_DB_OK);
    }
}

/*{
** Name: psq_dbp_qp_ids   - construct BE and FE ids for a QP QSF object
**
** Description:
**	This function will construct BE and FE ids for a QP QSF object using
**	info supplied by the caller
**
** Inputs:
**	sess_cb		- PSF session CB.
**	fe_id		- address of FE id structure - may be NULL if the caller
**			  does not desire that we construct the FE id
**	be_id		- address of BE id structure - may be NULL if the caller
**			  does not desire that we construct the BE id
**	dbp_name	- name of the dbproc
**	dbp_own_name	- name of dbproc owner
**
** Outputs:
**	fe_id		- FE object id
**	be_id		- BE object id
**
** Returns:
**	none
**
** Side Effects:
**	Increments sess_cb->pss_crsid
**
** History:
**	12-jan-94 (andre)
**	    Written
**	04-mar-94 (andre)
**	    made changes to handle cases when the caller is interested in 
**	    constructing only one of the ids
**	16-jun-94 (andre)
**	    Bug #64395
**	    it is dangerous to cast a (char *) into a (DB_CURSOR_ID *) and then
**	    dereference the resulting pointer because the char ptr may not be 
**	    pointing at memory not aligned on an appopriate boundary.  Instead,
**	    we will allocate a DB_CURSOR_ID structure on the stack, initialize 
**	    it and MEcopy() it into the char array
*/
VOID
psq_dbp_qp_ids(
	       PSS_SESBLK	*sess_cb,
	       char         	*fe_id, 
	       DB_CURSOR_ID 	*be_id, 
	       DB_DBP_NAME 	*dbp_name, 
	       DB_OWN_NAME 	*dbp_own_name)
{
    /* 
    ** For `translate or define' QSF call we need two ids, one is
    ** the id used by the FEs, the other one is internal. The name
    ** part in both will be the procedure name, the numeric part
    ** will be zeroes in the FE case and unique id for the BE id.
    ** In the case of the FE id the user name and unique database id
    ** will be appended to the name part of the cursor id. The alias
    ** name will therefore be 2 zeroed i4s followed by proc name
    ** followed by the proc owner name and unique database id.
    */

    if (fe_id)
    {
	DB_CURSOR_ID	fe_curs_id;
    
        fe_curs_id.db_cursor_id[0] = fe_curs_id.db_cursor_id[1] = (i4) 0;
        MEcopy((PTR) dbp_name, sizeof((*dbp_name)), 
            (PTR) fe_curs_id.db_cur_name);
	MEcopy((PTR) &fe_curs_id, sizeof(DB_CURSOR_ID), (PTR) fe_id);

        MEcopy((PTR) dbp_own_name, sizeof((*dbp_own_name)), 
	    (PTR) (fe_id + sizeof(DB_CURSOR_ID)));
        I4ASSIGN_MACRO(sess_cb->pss_udbid,
	    *(i4 *) (fe_id + sizeof(DB_CURSOR_ID) + sizeof((*dbp_own_name))));
    }
    
    if (be_id)
    {
        /* construct BE QSF object id for a dbproc QP object */
        be_id->db_cursor_id[0] = (i4) sess_cb->pss_psessid;
        be_id->db_cursor_id[1] = (i4) sess_cb->pss_crsid++;
        MEcopy((PTR) dbp_name, sizeof((*dbp_own_name)), 
	    (PTR) be_id->db_cur_name);
    }

    return;
}
