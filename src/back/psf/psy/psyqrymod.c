/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <tm.h>
#include    <qu.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <ulm.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <sxf.h>
#include    <psyaudit.h>

/**
**
**  Name: PSYQRYMOD.C - Apply views, permits, and integrities
**
**  Description:
**      This file contains the functions that apply views, permits, and
**	integrities to user queries.  The main part of the code is actually
**	in other files; this just calls subroutines.
**
**          psy_qrymod - Apply the qrymod algorithm
**          psy_qminit - Allocate and initialize the PSY_CB.
**
**
**  History: 
**      09-jul-86 (jeff)
**          Adapted from qrymod.c in 4.0
**	25-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added psy_qminit to allocate/initialize the psy_cb.
**	24-jul-89 (jennifer)
**	    For each statement, indicate that audit information is NULL.
**	08-aug-90 (ralph)
**	    Initialize new psy_cb fields in psy_qminit.
**	13-dec-90 (ralph)
**	    Move reset of pss_audit to before psy_view invocation (b34903)
**	08-dec-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**	08-jan-93 (rblumer) 
**	    don't check permissions for dummy set-input range variables.
**	30-mar-93 (rblumer) 
**	    in psy_qrymode, set check_perms to false for system-generated
**	    procedures and for (internal) queries with PSS_NO_CHECKPERMS set.
**	26-apr-93 (markg)
**	    If the server is running C2 then initialize each range
**	    entry to show that it needs to be audited.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	25-oct-93 (stephenb)
**	    If the server is running C2 then also initialize the result range
**	    entry to show that it needs to be audited.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**      15-Apr-2003 (bonro01)
**          Added include <psyaudit.h> for prototype of psy_secaudit()
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_qrymod(
	PST_QNODE *qtree,
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PST_J_ID *num_joins,
	i4 *resp_mask);
i4 psy_qminit(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PSF_MSTREAM *mstream);

/*{
** Name: psy_qrymod	- Process views, permits, and integrities
**
** Description:
**      This function applies the qrymod algorithm to user queries.
**	This algorithm is well-described in the specific functions
**	psyview.c, psyinteg.c, and psypermit.c.  It MUST be applied
**	in the order: views, integrities, permits.  This is because
**	permits are on base tables, not on views, and we don't want
**	the extra qualifications stuck on by the permit process to
**	affect the integrities.
**
**	Rules modifications are applied after all other query mods.
**	Rules processing (in psyrules.c) may add statement trees to rule lists
**	maintained in sess_cb.
**
** Inputs:
**      qtree                           The query tree to modify
**	sess_cb				Current session control block
**	psq_cb				Calling control block
**	num_joins			ptr to a var containing number of joins
**					in the query tree
**
** Outputs:
**      qtree                           Can be modified by views, permits,
**					and integrities
**	sess_cb				All kinds of miscellaneous stuff
**					that is too detailed to describe here
**	psq_cb
**	    .err_blk			Filled in if an error happens
**
**	*num_joins			Will be incremented if any of the views
**					involved in the query or any permits
**					applied use joins.
**
**	*resp_mask			if the qmode==PSQ_VIEW will contain
**					indicators of whether the view appears
**					to be updateable and/or grantable view 
**
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_FATAL			Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can allocate memory
**	    Catalog I/O
**
** History:
**      09-jul-86 (jeff)
**          Adapted from 4.0 qrymod() function
**      02-sep-86 (seputis)
**          used result relation from auxiliary table since RDF ptrs are NULLed
**	18-sep-86 (daved)
**	    the result relation in the session cb is not always the
**	    same as the one found in the user range table. this is because
**	    the one in the range table is accurate only in an append statement.
**	    thus, we will now change the session cb to point to the correct
**	    result relation.
**	2-apr-87 (daved)
**	    set a pss_permit flag in the range table for each used entry.
**	    psy_permit will clear this flag. Psy_view will set this flag
**	    in all range entries that a derived from a table that still has
**	    this flag set, else it will clear this flag for new entries.
**	    We will then call psy_permit again, to catch all the new entries.
**	24-sep-87 (stec)
**	    Change error handling - call psf_error.
**	25-oct-87 (stec)
**	    Remove transaction creation, this is now done by SCF.
**	26-jan-89 (neil)
**	    Now also calls psy_rules to fill in rule tree.
**	08-mar-89 (neil)
**	    Calls psy_rules with more arguments.
**	22-jun-89 (andre)
**	    remove call to psy_protect() before call to psy_view() since
**	    psy_view() was modified to call psy_protect() before performing
**	    query modification on SQL views not belonging to the user.
**	    Additionally, &qtree will not be passed to psy_view(), since there
**	    is no apparent reason to do so (we no longer attempt to assign to
**	    the corresponding argument inside of psy_view() )
**      24-jul-89 (jennifer)
**	    For each statement, indicate that audit information is NULL.
**	08-sep-89 (andre)
**	    recieve and pass to psy_view() and psy_permit() ptr to a var
**	    containing number of joins in the query tree.
**	19-sep-90 (andre)
**	    as a result of changes in psy_view(), we no longer need to call
**	    psy_permit() from psy_qrymod().
**	13-dec-90 (ralph)
**	    Move reset of pss_audit to before psy_view invocation (b34903)
**	11-mar-91 (andre)
**	    if qmode is PSQ_VIEW, pass PSQ_RETRIEVE to psy_view(), psy_integ(),
**	    and psy_rules() + do not try to copy pss_usrrange to pss_auxrng
**	    since psy_dview() already placed the range table entries into
**	    pss_auxrng.
**	    (this is a part of fix for bug 33038)
**	07-aug-91 (andre)
**	    if qmode is PSQ_VIEW do NOT translate it to PSQ_RETRIEVE +
**	    if qmode is PSQ_VIEW do not bother calling psy_integ() and
**	    psy_rules()
**
**	    Added resp_mask to the argument list.  This mask will be used to
**	    communicate info to the caller of psy_qrymod().  psy_qrymod() itself
**	    will not contribute anything to the information, instead, it will
**	    pass it to other functions (initially only to psy_view()) which will
**	    be responsible for providing the actual information.
**	30-dec-92 (andre)
**	    when processing a view tree in the course of processing CREATE RULE
**	    on a view we do NOT want to do any permit, rule, or integrity
**	    checking
**	04-jan-93 (andre)
**	    if we are processing DELETE, INSERT, or UPDATE, and a table or a
**	    view being updated has rules defined on it, set PSS_CHECK_RULES in
**	    sess_cb->pss_resrng->pss_var_mask to remind psy_view() to call
**	    psy_rules()
**
**	    since psy_dview() is going away, and psl_cview() calls psy_qrymod()
**	    with range variables as they were entered in the grammar, we must
**	    copy the range table from pss_usrrange to pss_auxrng for define VIEW
**	    just as we do for all other QUEL statements
**
**	    In most cases psy_rules() will NO LONGER be called from
**	    psy_qrymod().  It will be called from psy_view() when processing
**	    INSERT, DELETE, or UPDATE. This change was necessary because now
**	    rules can be defined on views, and it seemed reasonable that rule
**	    checking (just like privilege checking) can be driven from
**	    psy_view().  The once exception occurs when we are processing UPDATE
**	    WHERE CURRENT OF - psy_view() will simply return when presented with
**	    this query mode, but there may be rules that apply to this
**	    statement.  Consequently, psy_rules() will be called from
**	    psy_qrymod() only when psq_qmode == PSQ_REPCURS
**	08-jan-93 (rblumer) 
**	    set flag to bypass permission checking for dummy set-input
**	    range variables.
**	30-mar-93 (rblumer) 
**	    set check_perms to false for system-generated procedures
**	    and for (internal) queries with PSS_NO_CHECKPERMS set.
**	01-apr-93 (andre)
**	    (fix for bugs 50823, 50825, and 50899)
**	    do not copy QUEL range table to aux. range table when processing
**	    REPLACE CURSOR - the entry describing the table/view over
**	    which the cursor has been defined has already been placed into
**	    pss_auxrng
**	08-apr-93 (andre)
**	    do not call psy_rules() explicitly for PSQ_REPCURS.  psy_view() will
**	    now handle PSQ_REPCURS and call psy_rules() when necessary.
**	09-apr-93 (andre)
**	    set PSS_CHECK_RULES in the pss_var_mask corresponding to the
**	    table/view being updated via a cursor
**	26-apr-93 (markg)
**	    If the server is running C2 then initialize each range
**	    entry to show that it needs to be audited.
**	5-jul-93 (robf)
**	    If server is C2 then initialize any result range table to show
**	    it needs to be audited.
**	14-sep-93 (robf)
**	    Add QUERY_SYSCAT check, disallowing direct querying of
**	    base catalogs without privilege. This is done here rather than
**	    lower down in view/permit processing since we want to allow
**	    access to views on base catalogs (e.g. standard catalogs)
**	    to go ahead. 
**	13-oct-93 (robf)
**          Add <st.h>
**	25-oct-93 (stephenb)
**	    If the server is running C2 then also initialize the result range
**	    entry to show that it needs to be audited.
**	13-dec-93 (robf)
**          When checking for query_syscat privilege, allow for extended
**	    catalogs. Some catalogs (like iistar_cdbs) have both the
**	    DMT_CATALOG and DMT_EXTENDED_CAT bits set. In this case we allow
**          access since this priv restricts base catalogs only.
**	09-feb-94 (andre)
**	    fix for bug 59595:
**		we also need to forego checking permits if we are parsing a 
**		system-generated query
**	11-jul-94 (robf)
**          Restrict direct queries to table extensions the same way
**	    as system catalogs, i.e. to those with SELECT_SYSCAT privilege.
**	    The rationale for this is that info in extensions is really
**          part of the base table, and to present an appropriately
**          protected view of the data access should be through the base
**          table, not directly at the extension.
*/
DB_STATUS
psy_qrymod(
	PST_QNODE          *qtree,
	PSS_SESBLK	   *sess_cb,
	PSQ_CB		   *psq_cb,
	PST_J_ID	   *num_joins,
	i4		   *resp_mask)
{
    PSS_RNGTAB          *resrng;
    PSS_USRRANGE	*rngtab = &sess_cb->pss_auxrng;
    PSF_MSTREAM         *mstream = &sess_cb->pss_ostream;
    DB_ERROR		*err_blk = &psq_cb->psq_error;
    i4			qmode = psq_cb->psq_mode;
    DB_STATUS		status;
    i4			i;
    i4			check_perms = TRUE;
    GLOBALREF PSF_SERVBLK *Psf_srvblk;
    i4		err_code;
    bool		can_query_syscat=FALSE;

    *resp_mask = 0L;	    /* init the response mask */

    /*
    ** avoid permit checking when processing RULE trees,
    ** or when parsing/reparsing a system-generated procedure,
    ** or when parsing a system-generated query (e.g. a query to check whether
    ** constraints hold on existing data)
    */
    if ((psq_cb->psq_mode == PSQ_RULE)
	|| (   (sess_cb->pss_dbp_flags & PSS_DBPROC)
	    && (sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED))
	|| (psq_cb->psq_flag & PSQ_NO_CHECK_PERMS)
	|| (   (psq_cb->psq_info != (PST_INFO *) NULL)
	    && (psq_cb->psq_info->pst_execflags & PST_SYSTEM_GENERATED))
       )
    {
	check_perms = FALSE;
    }
    /*
    ** Can user directly query system catalogs ?
    ** Users with databse privilege QUERY_SYSCAT, or users with
    ** SECURITY subject privilege may directly query system cats
    */
    if(psy_ckdbpr(psq_cb, DBPR_QUERY_SYSCAT)==E_DB_OK)
	can_query_syscat=TRUE;
    else if(sess_cb->pss_ustat&DU_USECURITY)
	can_query_syscat=TRUE;
    else
	can_query_syscat=FALSE;

    /*
    ** Reset pss_audit prior to calling psy_view.
    */
    sess_cb->pss_audit = NULL;

    /*
    ** We don't want to clobber the user's view of the range table in
    ** the process of doing qrymod.  Therefore, we will copy the user's
    ** range table to the auxiliary range table, and use that instead.
    */

    /* don't need to copy range table for SQL because there are no range
    ** table entries that live longer than one statement
    **
    ** REPLACE CURSOR places description of the table/view over which it is
    ** defined into pss_auxrng
    */
    if (   sess_cb->pss_lang == DB_QUEL
	&& qmode != PSQ_REPCURS)
    {
	status = pst_rgcopy(&sess_cb->pss_usrrange, rngtab, 
	    &sess_cb->pss_resrng, err_blk);
	if (DB_FAILURE_MACRO(status))
	    return (status);
    }    
    for (i = 0; i < PST_NUMVARS; i++)
    {
	rngtab->pss_rngtab[i].pss_rgparent = -1;

	/* don't want to check permissions on
	** set-input temporary tables
	*/
	if (rngtab->pss_rngtab[i].pss_rgtype == PST_SETINPUT)
	    rngtab->pss_rngtab[i].pss_permit = FALSE;
	else
	    rngtab->pss_rngtab[i].pss_permit = check_perms;

	/*
	** See if this is a real base catalog.
	** We special case iidbcapabilities since its marked as a base 
	** catalog, but gets directly queried (and needs to be
	** accessed by frontends)
	*/
	if(rngtab->pss_rngtab[i].pss_used &&
	   rngtab->pss_rngtab[i].pss_rgno != -1 &&
	   rngtab->pss_rngtab[i].pss_tabdesc &&
	   ((
	   (rngtab->pss_rngtab[i].pss_tabdesc->tbl_2_status_mask&DMT_TEXTENSION)
	   ) || (
	   (rngtab->pss_rngtab[i].pss_tabdesc->tbl_status_mask&DMT_CATALOG) &&
	   !(rngtab->pss_rngtab[i].pss_tabdesc->tbl_status_mask&DMT_VIEW) &&
	   !(rngtab->pss_rngtab[i].pss_tabdesc->tbl_status_mask&DMT_EXTENDED_CAT) &&
	   STncasecmp((char*)&rngtab->pss_rngtab[i].pss_tabdesc->tbl_name,
			"iidbcapabilities", GL_MAXNAME)!=0
	   ))
        )
	{
		/*
		** This is a base catalog.
		*/
		if(!can_query_syscat)
		{
		    /*
		    ** Error, user can't query base catalogs.
		    */
		    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
			(VOID)psy_secaudit( 
			    FALSE,
			    sess_cb,
			    (char*)&rngtab->pss_rngtab[i].pss_tabdesc->tbl_name,
			    &rngtab->pss_rngtab[i].pss_tabdesc->tbl_owner,
			    DB_MAXNAME,
			    SXF_E_SECURITY,
			    I_SX273B_QUERY_SYSCAT,
			    (SXF_A_SELECT|SXF_A_FAIL),
			    err_blk);

		    (VOID) psf_error(E_PS035A_CANT_QUERY_SYSCAT, 
				0L, PSF_USERERR, &err_code,
			        err_blk, 1,
			        psf_trmwhite(sizeof(DB_NAME),
				  (char*)&rngtab->pss_rngtab[i].pss_tabdesc->tbl_name),
				(char*)&rngtab->pss_rngtab[i].pss_tabdesc->tbl_name);
		    return E_DB_ERROR;
		}
	}
	/*
	** If this is a C2 server set the PSS_DOAUDIT flag
	** for this entry.
	*/
	if (Psf_srvblk->psf_capabilities & PSF_C_C2SECURE)
	    rngtab->pss_rngtab[i].pss_var_mask |= PSS_DOAUDIT;
    }
    
    if (resrng = sess_cb->pss_resrng)
    {
	/* don't want to check permissions
	** on set-input temporary tables
	*/
	if (rngtab->pss_rngtab[i].pss_rgtype == PST_SETINPUT)
	    rngtab->pss_rngtab[i].pss_permit = FALSE;
	else
	    rngtab->pss_rngtab[i].pss_permit = check_perms;

	/*
	** if processing DELETE, UPDATE, INSERT, or UPDATE/REPLACE cursor, and
	** there are rules defined on the view/table being updated, set
	** PSS_CHECK_RULES in pss_var_mask
	*/
	if (   (   qmode == PSQ_APPEND
		|| qmode == PSQ_DELETE
		|| qmode == PSQ_REPLACE
		|| qmode == PSQ_REPCURS
	       )
	    && resrng->pss_tabdesc->tbl_status_mask & DMT_RULE
	   )
	{
	    resrng->pss_var_mask |= PSS_CHECK_RULES;
	}
        /*
        ** If this is a C2 server set the PSS_DOAUDIT flag
        ** for the result table.
        */
        if (Psf_srvblk->psf_capabilities & PSF_C_C2SECURE)
		rngtab->pss_rsrng.pss_var_mask |= PSS_DOAUDIT;
    }


    /* Apply view processing */
    status = psy_view(mstream, qtree, rngtab, qmode, sess_cb, err_blk,
		      num_joins, resp_mask);
    if (DB_FAILURE_MACRO(status))
    {
	return (status);
    }

    /*
    ** do not check for existing INTEGRITIES when processing trees as a
    ** part of CREATE/DEFINE VIEW or CREATE RULE processing
    */
    if (qmode != PSQ_VIEW && qmode != PSQ_RULE)
    {
	/* Apply integrity processing */
	status = psy_integ(mstream, qtree, rngtab, resrng, qmode,
	    sess_cb, &qtree, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}
    }

    return (status);
}

/*{
** Name: psy_qminit	- Allocate and initialize the PSY_CB
**
** Description:
**      This function allocates and initialzes the qrymod control
**	block, PSY_CB.  It is called by the grammar.
**
** Inputs:
**	sess_cb				Current session control block
**	psq_cb
**	    .err_blk			Filled in if an error happens
**	mstream				memory stream to use for allocating a
**					PSY_CB
**
** Outputs:
**	sess_cb
**	    .pss_object			Points to allocated psy_cb
**	Returns:
**	    E_DB_OK			Success
**	    Other			Return code from called functions
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates memory
**
** History:
**      22-jul-89 (ralph)
**          Written.
**	08-aug-90 (ralph)
**	    Initialize new psy_cb fields.
**	14-jul-92 (andre)
**	    init PSY_CB.psy_flags
**	14-jul-92 (andre)
**	    added mstream to the function's interface which will enable it to no
**	    longer assume that PSY_CB must be allocated from any given stream.
**	20-jul-92 (andre)
**	    added initialization for psy_xcolq
**	20-jul-92 (andre)
**	    iprocessing DROP PROCEDURE (psq_cb->psq_mode == PSQ_DRODBP), memory
**	    stream has already been opened, so don't do it here
**	21-jul-92 (andre)
**	    removed initialization for psy_xcolq;
**
**	    replaced code initializing various fields to 0 with a call to MEfill
**	    to fill a newly allocated PSY_CB with 0's
**	06-dec-92 (andre)
**	    Added code to initialize psy_u_colq and psy_r_colq
**	10-jul-93 (andre)
**	    cast QUinit()'s arg to (QUEUE *) to agree with the prototype
**	    declaration
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	28-apr-94 (andre)
**	    (part of fix for bug 62890)
**	    if the statement was generated internally 
**	    (psq_cb->psq_info->pst_execflags & PST_SYSTEM_GENERATED), set 
**	    PSY_SYSTEM_GENEARTED in psy_cb->psy_flags.
*/
DB_STATUS
psy_qminit(
	PSS_SESBLK	    *sess_cb,
	PSQ_CB		    *psq_cb,
	PSF_MSTREAM	    *mstream)
{
    DB_STATUS	    status;
    PSY_CB	    *psy_cb;

    /*
    ** open the memory stream unless we are processing DROP PROCEDURE, in which
    ** case it has already been opened
    */
    if (psq_cb->psq_mode != PSQ_DRODBP)
    {
	/* Allocate the PSY_CB statement */
	status = psf_mopen(sess_cb, QSO_QP_OBJ, mstream, &psq_cb->psq_error);
	if (status != E_DB_OK)
	    return (status);
    }

    status = psf_malloc(sess_cb, mstream, sizeof(PSY_CB), &sess_cb->pss_object,
	&psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    status = psf_mroot(sess_cb, mstream, sess_cb->pss_object, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    psy_cb = (PSY_CB *) sess_cb->pss_object;

    MEfill(sizeof(PSY_CB), (u_char) 0, (PTR) psy_cb);

    /* Fill in control block header */
    psy_cb->psy_length	 = sizeof(PSY_CB);
    psy_cb->psy_type	 = PSYCB_CB;
    psy_cb->psy_owner	 = (PTR)DB_PSF_ID;
    psy_cb->psy_ascii_id = PSYCB_ASCII_ID;

    (VOID) QUinit((QUEUE *) &psy_cb->psy_usrq);	/* No users */
    (VOID) QUinit((QUEUE *) &psy_cb->psy_tblq);	/* No tables */
    (VOID) QUinit((QUEUE *) &psy_cb->psy_colq);	/* No columns */
    (VOID) QUinit((QUEUE *) &psy_cb->psy_u_colq);
    (VOID) QUinit((QUEUE *) &psy_cb->psy_r_colq);

    /*
    ** remember whether the statement was generated internally
    */
    if (   psq_cb->psq_info 
	&& psq_cb->psq_info->pst_execflags & PST_SYSTEM_GENERATED)
    {
	psy_cb->psy_flags |= PSY_SYSTEM_GENERATED;
    }

    return(E_DB_OK);
}
