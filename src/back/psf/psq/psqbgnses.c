/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <qu.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmccb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <pslyalloc.h>

/**
**
**  Name: PSQBGNSES.C - Functions used to begin a parser session
**
**  Description:
**      This file contains the functions necessary to begin a parser session.
**
**          psq_bgn_session - Begin a parser session.
**
**
**  History:
**      01-oct-85 (jeff)    
**          written
**	13-apr-87 (puree)
**	    add support for dynamic SQL
**	24-apr-87 (stec)
**	    init pss_project.
**	11-may-87 (stec)
**	    store psq_udbid to pss_dbid.
**	04-sep-87 (stec)
**	    Added critical region code where needed.
**	05-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Copy psq_aplid to pss_aplid;
**	    Copy psq_group to pss_group.
**	16-mar-89 (neil)
**	    Initialized rule field.
**	27-jul-89 (jrb)
**	    Copy numeric literals scanning flag into session cb at session
**	    startup.
**	27-oct-89 (ralph)
**	    Copy user status flags to session control block.
**	11-oct-89 (ralph)
**	    Initialize pss_rgset and pss_raset.
**	12-sep-90 (sandyh)
**	    Added session memory value handling, passed in thru psf.memory.
**	13-sep-90 (teresa)
**	    The following booleans have become bitmasks in PSQ_CB.psq_flag:
**	    psq_catupd, psq_warnings, psq_dba_drop_all, psq_fips_mode;
**	    The following booleans have become bitmasks in sess_cb.pss_ses_flag:
**	    pss_catupd, pss_warnings, pss_project, pss_journaling, pss_ruset,
**	    pss_rgset, pss_raset, pss_dba_drop_all, pss_fips_mode
**	    NOTE: this effectively initializes PSS_JOURNALING to off, and
**		  Indicates that pss_rusr(PSS_RUSET), pss_rgroup (PSS_RGSET) 
**		  and pss_raplid (PSS_RASET) are not set.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.  Updated to check for distributed
**	    specification.
**	30-mar-1992 (bryanp)
**	    Fill in pss_sess_owner with a session-unique owner name for use
**	    by temporary tables which are owned by this session.
**	24-nov-92 (ralph)
**	    CREATE SCHEMA:
**	    Initialize pss_prvgoval
**	22-dec-92 (rblumer)
**	    initialize pointer for statement-level rule list.
**	15-mar-93 (ralph)
**	    DELIM_IDENT: initialize pss_dbxlate to zero
**	26-mar-93 (ralph)
**	    DELIM_IDENT: Must initialize pss_dbxlate from psq_cb.psq_dbxlate
**	    and pss_cat_owner from psq_cat_owner.
**	29-jun-93 (andre)
**	    #included CV.H; removed now redudndant declaration of CVlx()
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	20-sep-93 (rogerk)
**	    Changed default table create semantics to be WITH JOURNALING.
**	    Initialized the pss_ses_flag setting to include PSS_JOURNALING
**	    which mimics the user requesting "set journaling" to indicate that
**	    tables created should be journaled.
**	08-oct-93 (rblumer)
**	    increased values allowed in pss_trace vector, using PSS_TVALS.
**	18-oct-93 (rogerk)
**	    Added support for journal default override.  Check psf server
**	    control block flag for PSF_NO_JNL_DEFAULT override before setting
**	    the session parse flag to assume journaling on table creates.
**	01-nov-93 (anitap)
**	    if PSQ_INGRES_PRIV is set in psq_cb->psq_flag, set
**	    PSS_INGRES_PRIV in sess_cb->pss_ses_flags. Changes in 
**	    psq_bgn_session() so that verifydb can "alter table drop constraint"
**	    on another user's table.
**	17-dec-93 (rblumer)
**	    removed all FIPS_MODE flag references.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Remove callback to SCF to get session id and ADF_CB;
**	    *ADF_CB now supplied by scsinit in PSQ_CB.
**	    Initialize (once), QSF_RCB embedded in PSS_SESBLK for
**	    use by psfmem.c functions.
**      02-jan-2004 (stial01)
**          Changes for SET [NO]BLOBJOURNALING
**      14-jan-2004 (stial01)
**          set [no]blobjournaling only when table specified.
**/


/*{
** Name: psq_bgn_session	- Begin a parser session.
**
**  INTERNAL PSF call format: status = psq_bgn_session(&psq_cb, &sess_cb);
**
**  EXTERNAL call format: status = psq_call(PSQ_BGN_SESSION, &psq_cb, &sess_cb);
**
** Description:
**      The psq_bgn_session function begins a parser session.  It should be
**	called each time a new user connects to a server.  There may be 
**      many parser sessions per database server.  There should be one parser 
**      session for each invocation of the database system that is connected 
**      to the server.  When starting a parser session, one has to tell it
**	what query language to use, and other session parameters.
**
** Inputs:
**      psq_cb
**          .psq_qlang                  The query language to use.
**          .psq_decimal
**		.psf_decspec		TRUE indicates that the decimal marker
**					has been specified.  FALSE means use the
**					default (a ".").
**		.psf_decimal		The character to use as a decimal marker
**					(if specified).
**	    .psq_distrib		Indicator for whether distributed
**					statements and constructs should be
**					accepted.
**	    .psq_sessid			Session id 
**	    .psq_server			address of server control block
**	    .psq_adf_cb			Pointer to session's ADF_CB
**	    .psq_dbid			Database id for this session.
**	    .psq_user			User name of 
**	    .psq_dba			User name of dba
**	    .psq_group			Group id of session
**	    .psq_aplid			Application id of session
**	    .psq_flag			bitmask containing the following flags:
**	      .psq_catupd		  TRUE means catalogs updateable
**	      .psq_warnings		  Set to TRUE if user wishes to see
**					  warnings on unsupported commands
**	    .psq_idxstruct		Structure for creating new indexes
**					(e.g. DB_ISAM_STORE)
**	    .psq_udbid			Unique database id for this session.
**	    .psq_ustat	 		User status flags from SCS_ICS
**	    .psq_dbxlate		Case translation semantics for the db
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psq_cb
**          .psq_error                  Error information
**		.err_code		    What error occurred
**		    E_PS0000_OK			Success
**		    E_PS0001_INTERNAL_ERROR	    Internal PSF problem
**		    E_PS0201_BAD_QLANG		    Bad query language specifier
**		    E_PS0203_NO_DECIMAL		    No decimal marker specified
**		    E_PS0204_BAD_DISTRIB	    Bad distributed
**						    specification
**		    E_PS0205_SRV_NOT_INIT	    Server not initialized
**		    E_PS0206_TOO_MANY_SESS	    Too many sessions at one
**						    time
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Session is to be aborted
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Causes memory to be allocated.
**	    Increments the session count in the server control block.
**
** History:
**	01-oct-85 (jeff)
**          written
**	28-jul-86 (jeff)
**	    Added initialization of pss_catupd and pss_idxstruct
**      26-aug-86 (seputis)
**          Removed definition of yaccstream
**	13-apr-87 (puree)
**	    Initialize prototype list for dynamic SQL.
**	24-apr-87 (stec)
**	    init pss_project.
**	11-may-87 (stec)
**	    store psq_udbid to pss_dbid.
**	04-sep-87 (stec)
**	    Added critical region code where needed.
**	02-oct-87 (stec)
**	    Added pss_journaling initialization.
**	13-jun-88 (stec)
**	    Added initialization of pss_ruset for DB procs.
**	08-mar-89 (andre)
**	    Copy dba_drop_all from PSQ_CB to PSS_SESBLK.
**	15-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Copy psq_aplid to pss_aplid;
**	    Copy psq_group to pss_group.
**	16-mar-89 (neil)
**	    Initialized rule field.
**	27-jul-89 (jrb)
**	    Copy numeric literals flag into session cb.
**	27-oct-89 (ralph)
**	    Copy user status flags to session control block.
**	11-oct-89 (ralph)
**	    Initialize pss_rgset and pss_raset.
**	28-dec-89 (andre)
**	    Copy fips_mode from PSQ_CB to PSS_SESBLK.
**	13-feb-90 (andre)
**	    set scf_stype to SCU_EXCLUSIVE before calling scu_swait.
**	12-sep-90 (sandyh)
**	    Added support for session memory value calculated from psf
**	    memory startup parameter.
**	15-nov-90 (andre)
**	    check the return status after calling SCF to acquire or to release a
**	    semaphore.
**	    If an error occurred when trying to acquire the semaphore, return
**	    E_DB_SEVERE to abort the session.
**	    If an error occurred when trying to release the semaphore, return
**	    E_DB_FATAL to bring down the server.
**	17-may-91 (andre)
**	    store DBA name into sess_cb->pss_dbaname and NULL-terminate.
**	08-nov-91 (rblumer)
**          merged from 6.4:  25-jul-91 (andre)
**		if (psq_cb->psq_flag & PSQ_STRIP_NL_IN_STRCONST), set bit
**		PSS_STRIP_NL_IN_STRCONST in sess_cb->pss_ses_flag.  this will
**		indicate that we are connected to an older FE, so the scanners
**		will continue to strip NLs inside quoted strings;
**		this is required to fix bug 38098
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.  Updated to check for distributed
**	    specification.
**	26-feb-92 (andre)
**	    if PSQ_REPAIR_SYSCAT is set in psq_cb->psq_flag, set
**	    PSS_REPAIR_SYSCAT in sess_cb->pss_ses_flags
**	30-mar-1992 (bryanp)
**	    Fill in pss_sess_owner with a session-unique owner name for use
**	    by temporary tables which are owned by this session.
**	02-jun-92 (andre)
**	    initialize pss_dependencies_stream to NULL to avloid use of illegal
**	    address throughout the parser.
**	24-nov-92 (ralph)
**	    CREATE SCHEMA:
**	    Initialize pss_prvgoval
**	22-dec-92 (rblumer)
**	    initialize pointer for statement-level rule list.
**	14-jan-93 (andre)
**	    remember whether we are running UPGRADEDB - this will enable us to
**	    decide whether IIDEVICES can be dropped - which is needed by
**	    UPGRADEDB
**	15-mar-93 (ralph)
**	    DELIM_IDENT: initialize pss_dbxlate to zero
**	08-apr-93 (andre)
**	    names of rule list headers in sess_cb have changed (and their
**	    number has doubled)
**	26-mar-93 (ralph)
**	    DELIM_IDENT: Must initialize pss_dbxlate from psq_cb.psq_dbxlate
**	    and pss_cat_owner from psq_cat_owner.
**	10-aug-93 (andre)
**	    fixed cause of a compiler warning
**	08-sep-93 (swm)
**	    Changed sizeof(DB_SESSID) to sizeof(CS_SID) to reflect recent CL
**	    interface revision.
**	20-sep-93 (rogerk)
**	    Changed default table create semantics to be WITH JOURNALING.
**	    Initialized the pss_ses_flag setting to include PSS_JOURNALING
**	    which mimics the user requesting "set journaling" to indicate that
**	    tables created should be journaled.
**	08-oct-93 (rblumer)
**	    increased values allowed in pss_trace vector, using PSS_TVALS.
**	18-oct-93 (rogerk)
**	    Added support for journal default override.  Check psf server
**	    control block flag for PSF_NO_JNL_DEFAULT override before setting
**	    the session parse flag to assume journaling on table creates.
**	15-nov-93 (andre)
**	    add code to initialize a newly added sess_cb->pss_flattening_flags
**	01-nov-93 (anitap)
**	    if PSQ_INGRES_PRIV is set in psq_cb->psq_flag, set
**	    PSS_INGRES_PRIV in sess_cb->pss_ses_flags.
**	17-dec-93 (rblumer)
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).  So I removed all FIPS_MODE flags.
**	02-jan-94 (andre)
**	    if starting a local session, call DMF to determine whether the 
**	    database to which we are connected is being journaled and record 
**	    that information by setting (or not setting) PSS_JOURNALED_DB bit 
**	    in pss_ses_flags
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for pss_owner which has changed type to PTR.
**	17-mar-94 (robf)
**          Add support for PSQ_SELECT_ALL flag
**	13-Feb-1995 (canor01)
**	    initialize the pss_audit field in the session control block
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	23-mar-1999 (thaju02)
**	    Modified '$Sess' to use #define DB_SESS_TEMP_OWNER. (B94067)
**      01-Dec-2000 (hanal04) Bug 100680 INGSRV 1123
**          If PSQ_RULE_UPD_PREFETCH is set turn on PSS_RULE_UPD_PREFETCH
**          in the session control block to signify that we should use
**          the prefetch stategy required to ensure consitent behaviour in
**          updating rules fired by updates.
**	10-Jan-2001 (jenjo02)
**	    Remove callback to SCF to get session id and ADF_CB;
**	    *ADF_CB now supplied by scsinit in PSQ_CB.
**	30-Jan-2004 (schka24)
**	    Get rid of a type-cast warning on adf cb.
**	3-Feb-2005 (schka24)
**	    Num-literals renamed to parser-compat, fix here.
**	15-june-06 (dougi)
**	    Add support for "before" triggers.
**	30-aug-06 (thaju02)
**	    If PSQ_RULE_DEL_PREFETCH is set turn on PSS_RULE_DEL_PREFETCH
**          in the session control block, for prefetch strategy to 
**          be applied for deletes. (B116355)
**	26-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query support: Added copy of
**	    psq_flag.PSQ_NOCHK_SINGLETON_CARD to session flag
**	    for defaulting SET CARDINALITY_CHECK
**      November 2009 (stephenb)
**          Batch execution; initilization of new fields.
**      29-apr-2010 (stephenb)
**          Init batch_copy_optim.
**	04-may-2010 (miket) SIR 122403
**	    Init new sess_cb->pss_stmt_flags2.
*/
DB_STATUS
psq_bgn_session(
	register PSQ_CB     *psq_cb,
	register PSS_SESBLK *sess_cb)
{
    i4		    err_code;
    i4			    i;
    DB_STATUS		    status = E_DB_OK;
    STATUS		    sem_status;
    i4		    sem_errno;
    bool		    leave_loop = TRUE;
    extern PSF_SERVBLK	    *Psf_srvblk;
    ULM_RCB		    ulm_rcb;

    /*
    ** No error to begin with.
    */
    psq_cb->psq_error.err_code = E_PS0000_OK;

    /*
    ** Do as much validity checking as possible before allocating any memory.
    ** That way, there won't be any cleaning up to do for the majority of
    ** errors.
    */

    /*
    ** Check for server initialized. This code could be placed within
    ** critical region, but this is not necessary, since this is a flag
    ** test.
    */
    if (!Psf_srvblk->psf_srvinit)
    {
	(VOID) psf_error(E_PS0205_SRV_NOT_INIT, 0L, PSF_CALLERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }

    /*
    ** Check for valid language spec.
    */
    if (psq_cb->psq_qlang != DB_QUEL && psq_cb->psq_qlang != DB_SQL)
    {
	(VOID) psf_error(E_PS0201_BAD_QLANG, 0L, PSF_CALLERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }
	
    /*
    ** Check whether language is allowed in this server.  This will be useful
    ** when we have configurable servers, where some query languages can be
    ** used and some can't. This code could be placed within a critical region
    ** but it is not necessary, since this is a flag test only.
    */
    if ((psq_cb->psq_qlang & Psf_srvblk->psf_lang_allowed) == 0)
    {
	(VOID) psf_error(E_PS0202_QLANG_NOT_ALLOWED, 0L, PSF_CALLERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }

    /*
    ** Make sure that the decimal character is actually specified.
    */
    if (!psq_cb->psq_decimal.db_decspec)
    {
	(VOID) psf_error(E_PS0203_NO_DECIMAL, 0L, PSF_CALLERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (E_DB_ERROR);
    }

     /* Check distributed specification
    **
    ** a=local_server, b=distrib_server, c=distrib_session
    **
    **   a,b
    **
    **     00  01  11  10
    **    -----------------
    ** c  |   |   |   |   |
    **  0 | 1 | 1 | 0 | 0 |
    **    |   |   |   |   |
    **    -----------------  ==> ERROR
    **    |   |   |   |   |
    **  1 | 1 | 0 | 0 | 1 |
    **    |   |   |   |   |
    **    -----------------
    */
    if (   !(psq_cb->psq_distrib & (DB_1_LOCAL_SVR | DB_3_DDB_SESS))
	|| ((~psq_cb->psq_distrib & DB_2_DISTRIB_SVR)
	    && (psq_cb->psq_distrib & DB_3_DDB_SESS))
	)
    {
	psf_error(E_PS0204_BAD_DISTRIB, 0L, PSF_CALLERR, &err_code,
	    &psq_cb->psq_error,0);
	return (E_DB_ERROR);
    }

    /*
    ** Check for too many sessions in server at one time.
    ** This code must be executed as a critical region.
    */

    do		    /* something to break out of */
    {
	/* get the semaphore */
	if (sem_status = CSp_semaphore(1, &Psf_srvblk->psf_sem)) /* exclusive */
	{
	    status = E_DB_SEVERE;	/* abort the session */
	    sem_errno = E_PS020A_BGNSES_GETSEM_FAILURE;
	    break;
	}

	if (Psf_srvblk->psf_nmsess >= Psf_srvblk->psf_mxsess)
	{
	    (VOID) psf_error(E_PS0208_TOO_MANY_SESS, 0L, PSF_CALLERR, &err_code,
		&psq_cb->psq_error, 0);
	    status = E_DB_ERROR;
	    break;
	}

	/* Increment the session count */
	Psf_srvblk->psf_nmsess++;
	sess_cb->pss_psessid = ++Psf_srvblk->psf_sess_num;

	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    /* if semaphore has been successfully acquired, try to release it */
    if (sem_status == OK)
    {
	if (sem_status = CSv_semaphore(&Psf_srvblk->psf_sem))
	{
	    status = E_DB_FATAL;	/* bring down the server */
	    sem_errno = E_PS020B_BGNSES_RELSEM_FAILURE;
	}
    }

    /*
    ** if an error was encountered while trying to get or to release a
    ** semaphore, report it here
    */
    if (sem_status != OK)
    {
	(VOID) psf_error(sem_errno, sem_status, PSF_INTERR,
	    &err_code, &psq_cb->psq_error, 0);
    }

    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    /*
    ** Initialize the case translation semantics stuff
    */
    sess_cb->pss_dbxlate = psq_cb->psq_dbxlate;
    sess_cb->pss_cat_owner = psq_cb->psq_cat_owner;

    /*
    ** Copy the user name and dba name to the session control block.
    */
    STRUCT_ASSIGN_MACRO(psq_cb->psq_user.db_tab_own, sess_cb->pss_user);
    STRUCT_ASSIGN_MACRO(psq_cb->psq_dba, sess_cb->pss_dba);
    STRUCT_ASSIGN_MACRO(psq_cb->psq_group, sess_cb->pss_group);
    STRUCT_ASSIGN_MACRO(psq_cb->psq_aplid, sess_cb->pss_aplid);

    /* copy DBA name into sess_cb->pss_dbaname and NULL-terminate */
    {
	u_i2			dba_name_len;

	dba_name_len = (u_i2) psf_trmwhite((u_i4) sizeof(sess_cb->pss_dba),
			    (char *) &sess_cb->pss_dba);
	MEcopy((PTR) &sess_cb->pss_dba, dba_name_len,
	       (PTR) sess_cb->pss_dbaname);

	sess_cb->pss_dbaname[dba_name_len] = EOS;
    }

    /*
    ** Build a DB_OWN_NAME which contains a session-unique owner name. This
    ** owner name will be used for temporary tables which are owned by this
    ** session.
    */
    {
	char	    temp_sess_id[10];

	STmove(DB_SESS_TEMP_OWNER, ' ', sizeof(sess_cb->pss_sess_owner),
		(char *)&sess_cb->pss_sess_owner);
	/*
	** We can't convert directly into the sess_owner field because CVlx
	** null-terminates the result, and we don't want the trailing null
	*/
	CVlx(sess_cb->pss_psessid, temp_sess_id);
	MEcopy(temp_sess_id, 8, &sess_cb->pss_sess_owner.db_own_name[5]);
    }

    /*
    ** Start with per-user quota of memory.  Note that user may have overridden
    ** the default value at server startup in which case we will use calculated
    ** amount (pool/sessions); otherwise, default amount will be used.
    */
    sess_cb->pss_memleft = (Psf_srvblk->psf_sess_mem) ? Psf_srvblk->psf_sess_mem
						      : PSF_SESMEM;

    /*
    ** Initialize the user range table.
    */
    if (pst_rginit(&sess_cb->pss_usrrange) != E_DB_OK)
    {
	return (E_DB_FATAL);
    }
	
    /*
    ** Initialize the auxiliary range table.
    */
    if (pst_rginit(&sess_cb->pss_auxrng) != E_DB_OK)
    {
	return (E_DB_FATAL);
    }

    /*
    ** Open a memory stream for the symbol table.  The symbol table is
    ** composed of a list of blocks.
    ** Allocate the symbol table at the same time.
    */
    ulm_rcb.ulm_facility = DB_PSF_ID;
    ulm_rcb.ulm_poolid = Psf_srvblk->psf_poolid;
    ulm_rcb.ulm_blocksize = sizeof(PSS_SYMBLK);
    ulm_rcb.ulm_memleft = &sess_cb->pss_memleft;
    /* Set pointer to stream handle for ULM */
    ulm_rcb.ulm_streamid_p = &sess_cb->pss_symstr;
    /* Open a private, thread-safe stream */
    ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    ulm_rcb.ulm_psize = sizeof(PSS_SYMBLK);
    if (ulm_openstream(&ulm_rcb) != E_DB_OK)
    {
	if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    (VOID) psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, &psq_cb->psq_error, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0A02_BADALLOC, ulm_rcb.ulm_error.err_code,
		PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	}

	return((ulm_rcb.ulm_error.err_code == E_UL0004_CORRUPT) ? E_DB_FATAL
								: E_DB_ERROR);
    }

    sess_cb->pss_symtab = (PSS_SYMBLK*) ulm_rcb.ulm_pptr;
    sess_cb->pss_symtab->pss_sbnext = (PSS_SYMBLK *) NULL;

    /*
    ** Allocate the YACC_CB.  
    */

    if ((status = psl_yalloc(sess_cb->pss_symstr, 
	&sess_cb->pss_memleft,
	(PTR *) &sess_cb->pss_yacc, &psq_cb->psq_error)) != E_DB_OK)
    {
	/*
	** If the allocation failed, remember to close the streams, so the
	** memory associated with it will be freed.
	*/
	(VOID) ulm_closestream(&ulm_rcb);
	return (status);
    }
	
    /*
    ** Fill in the control block header.
    */
    sess_cb->pss_next = (PSS_SESBLK *) NULL;
    sess_cb->pss_prev = (PSS_SESBLK *) NULL;
    sess_cb->pss_length = sizeof(PSS_SESBLK);
    sess_cb->pss_type = PSS_SBID;
    sess_cb->pss_owner = (PTR)DB_PSF_ID;
    sess_cb->pss_ascii_id = PSSSES_ID;

    /*
    ** Initialize the session control block.
    */
    /* Save the session id */
    sess_cb->pss_sessid	    = psq_cb->psq_sessid;

    /* Set pointer to session's ADF_CB */
    sess_cb->pss_adfcb	    = (ADF_CB *) psq_cb->psq_adfcb;

    /* No cursors yet */
    sess_cb->pss_numcursors = 0;

    /* Language has already been validated */
    sess_cb->pss_lang = psq_cb->psq_qlang;

    /* Decimal spec has already been validated */
    sess_cb->pss_decimal = psq_cb->psq_decimal.db_decimal;

    /* Distributed spec has already been validated */
    sess_cb->pss_distrib = psq_cb->psq_distrib;

    /* Save the database id */
    sess_cb->pss_dbid = psq_cb->psq_dbid;

    /* Save the unique database id */
    sess_cb->pss_udbid = psq_cb->psq_udbid;

    /* Initialize QSF_RCB for use by psfmem.c functions */
    sess_cb->pss_qsf_rcb.qsf_type = QSFRB_CB;
    sess_cb->pss_qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
    sess_cb->pss_qsf_rcb.qsf_length = sizeof(sess_cb->pss_qsf_rcb);
    sess_cb->pss_qsf_rcb.qsf_owner = (PTR)DB_PSF_ID;
    sess_cb->pss_qsf_rcb.qsf_sid = sess_cb->pss_sessid;

    /*
    **	so session reset all bit flags
    */
    sess_cb->pss_stmt_flags = sess_cb->pss_stmt_flags2 =
	sess_cb->pss_dbp_flags = sess_cb->pss_ses_flag = 0L;
    sess_cb->pss_flattening_flags = 0;

    /*
    ** Default table create semantics are to assume journaling unless
    ** the PSF_NO_JNL_DEFAULT override is set.
    */
    if ((Psf_srvblk->psf_flags & PSF_NO_JNL_DEFAULT) == 0)
	sess_cb->pss_ses_flag |= PSS_JOURNALING;

	    /* catalog update flag */
    if (psq_cb->psq_flag & PSQ_CATUPD)
	sess_cb->pss_ses_flag |= PSS_CATUPD;

	    /* warnings on unsupported commands */
    if (psq_cb->psq_flag & PSQ_WARNINGS)
	sess_cb->pss_ses_flag |= PSS_WARNINGS;

	/* INDICATE if the DBA may DROP everyone's tables */
    if (psq_cb->psq_flag & PSQ_DBA_DROP_ALL)
	sess_cb->pss_ses_flag |= PSS_DBA_DROP_ALL;

	/* INDICATE if the session may SELECT everyone's tables */
    if (psq_cb->psq_flag & PSQ_SELECT_ALL)
	sess_cb->pss_ses_flag |= PSS_SELECT_ALL;

	/*
	** indicate that the session is allowed to INSERT/DELETE/UPDATE an index
	** which is a catalog (but not an extended catalog
	*/
    if (psq_cb->psq_flag & PSQ_REPAIR_SYSCAT)
	sess_cb->pss_ses_flag |= PSS_REPAIR_SYSCAT;

	/* 
	** indicate that the session allows $ingres to drop/add constraint on
	** tables owned by other users
	*/
    if (psq_cb->psq_flag & PSQ_INGRES_PRIV)
	sess_cb->pss_ses_flag |= PSS_INGRES_PRIV;	

    if (psq_cb->psq_flag & PSQ_ROW_SEC_KEY)
	sess_cb->pss_ses_flag |= PSS_ROW_SEC_KEY;

    /* See if passwords, roles allowed */
    if (psq_cb->psq_flag & PSQ_PASSWORD_NONE)
	sess_cb->pss_ses_flag |= PSS_PASSWORD_NONE;

    if (psq_cb->psq_flag & PSQ_ROLE_NONE)
	sess_cb->pss_ses_flag |= PSS_ROLE_NONE;

    if (psq_cb->psq_flag & PSQ_ROLE_NEED_PW)
	sess_cb->pss_ses_flag |= PSS_ROLE_NEED_PW;

	/* remember whether we are running UPGRADEDB */
    if (psq_cb->psq_flag & PSQ_RUNNING_UPGRADEDB)
	sess_cb->pss_ses_flag |= PSS_RUNNING_UPGRADEDB;

    /* Pick up serverwide default for card check */
    if (psq_cb->psq_flag & PSQ_NOCHK_SINGLETON_CARD)
	sess_cb->pss_ses_flag |=  PSS_NOCHK_SINGLETON_CARD;

        /* Initialize pss_project. */
    sess_cb->pss_ses_flag |= PSS_PROJECT;	/* pss_project = TRUE */
    
    /* init last statement */
    sess_cb->pss_last_sname[0] = EOS;
    /* batch optimization switch starts undefined */
    sess_cb->batch_copy_optim = PSS_BATCH_OPTIM_UNDEF;

    /* 
    ** if starting a local session, determine whether the database is being 
    ** journaled
    */
    if (~psq_cb->psq_distrib & DB_3_DDB_SESS)
    {
        DMC_CB		    dmc_cb, *dmc = &dmc_cb;
        DMC_CHAR_ENTRY	    dmc_char;

	MEfill(sizeof(dmc_cb), (u_char) 0, (PTR) dmc);

        dmc->type = DMC_CONTROL_CB;
        dmc->length = sizeof(*dmc);
        dmc->dmc_op_type = DMC_DATABASE_OP;
	dmc->dmc_session_id = (PTR) sess_cb->pss_sessid;
        dmc->dmc_flags_mask = DMC_JOURNAL;
        dmc->dmc_char_array.data_address= (PTR) &dmc_char;
	dmc->dmc_char_array.data_out_size = sizeof(dmc_char);
	dmc->dmc_db_id = (char *) sess_cb->pss_dbid;
	
	status = dmf_call(DMC_SHOW, (PTR) dmc);
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_error(E_PS020E_CANT_GET_DB_JOUR_STATUS, 
		dmc->error.err_code, PSF_INTERR, &err_code, 
		&psq_cb->psq_error, 0);
	    return(status);
	}

	if (dmc_char.char_value == DMC_C_ON)
	{
	    sess_cb->pss_ses_flag |= PSS_JOURNALED_DB;
	}
    }

    /* Save the storage structure for indexes */
    sess_cb->pss_idxstruct = psq_cb->psq_idxstruct;

    /* Make session copy of parser compatability settings */
    sess_cb->pss_parser_compat = psq_cb->psq_parser_compat;

    /* remember if NLs inside string constants need to be stripped */
    if (psq_cb->psq_flag & PSQ_STRIP_NL_IN_STRCONST)
	sess_cb->pss_ses_flag |= PSS_STRIP_NL_IN_STRCONST;

    /* no rule tree yet */
    sess_cb->pss_row_lvl_usr_rules  =
    sess_cb->pss_row_lvl_sys_rules  =
    sess_cb->pss_stmt_lvl_usr_rules =
    sess_cb->pss_stmt_lvl_sys_rules =
    sess_cb->pss_row_lvl_usr_before_rules  =
    sess_cb->pss_row_lvl_sys_before_rules  =
    sess_cb->pss_stmt_lvl_usr_before_rules =
    sess_cb->pss_stmt_lvl_sys_before_rules = (PST_STATEMENT *) NULL;

    if (psq_cb->psq_flag & PSQ_RULE_DEL_PREFETCH)
	sess_cb->pss_ses_flag |= PSS_RULE_DEL_PREFETCH;

    if(psq_cb->psq_flag2 & PSQ_RULE_UPD_PREFETCH)
        sess_cb->pss_ses_flag |= PSS_RULE_UPD_PREFETCH;

    /* copy user status flags to session control block */
    sess_cb->pss_ustat = psq_cb->psq_ustat;

    /*
    ** Initialize lots of pointer to NULL because nothing is happening yet.
    */
    sess_cb->pss_qbuf = sess_cb->pss_nxtchar = sess_cb->pss_prvtok =
	sess_cb->pss_bgnstmt = sess_cb->pss_endbuf =
	sess_cb->pss_prvgoval = (u_char *) NULL;

    /* initialize pss_audit */
    sess_cb->pss_audit = NULL;

    for (i = 0; i < PSS_CURTABSIZE; i++)
    {
	sess_cb->pss_curstab.pss_curque[i] = (PSC_CURBLK *) NULL;
    }

    /* initialize prototype list for dynamic SQL */
    sess_cb->pss_proto = (PST_PROTO *) NULL;

    /*
    ** pss_dependencies_stream, when not NULL, is expected to point at a valid
    ** stream descriptor.  After closing the stream we always reset
    ** pss_dependencies_stream to NULL, but in some cases we may end up checking
    ** pss_dependencies_stream before ever opening (and closing it).  As a
    ** result, you may end up using invalid address as a stream pointer.
    ** Initializing it here to NULL will ensure that it is non-NULL iff it
    ** points at a valid open stream descriptor.
    */
    sess_cb->pss_dependencies_stream = (PSF_MSTREAM *) NULL;

    /* No trace flags set */
    /* expect lint message */
    ult_init_macro(&sess_cb->pss_trace, PSS_TBITS, PSS_TVALS, PSS_TVAO);

    /* Cursor id set to 0, no cursors open yet */
    sess_cb->pss_crsid = 0;

    return (E_DB_OK);
}
