/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <qu.h>
#include    <bt.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <psyaudit.h>

/**
**
**  Name: PSYDPERM.C - Functions used to define a permit.
**
**  Description:
**      This file contains the functions necessary to define a permit on a 
**      database object (a table or a procedure).
**
**          psy_dpermit - Define a permit.
**
**
**  History:
**      02-oct-85 (jeff)    
**          written
**      29-apr-87 (stec)
**          Implemented changes for GRANT statement.
**	05-aug-88 (andre)
**	    Made changes for GRANTing permits on database procedures.
**	06-feb-89 (ralph)
**	    Added support for 300 attributes
**	06-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Initialize new DB_PROTECTION fields, dbp_seq and dbp_gtype
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Use DBGR_USER when initializing dbp_gtype.
**	08-may-89 (ralph)
**	    Initialize reserved field to blanks (was \0)
**	04-jun-89 (ralph)
**	    Initialize dbp_fill1 to zero
**	    Fix unix portability problems
**	02-nov-89 (neil)
**	    Alerters: Allowed privileges for events.
**	08-aug-90 (ralph)
**	    Initialize new fields in iiprotect tuple
**	14-dec-90 (ralph)
**	    Disallow use of GRANT by non-DBA if xORANGE
**	11-jan-90 (ralph)
**	    Allow user "$ingres" to use GRANT if xORANGE.
**	    This was done for CREATEDB (UPGRADEFE).
**	03-aug-92 (barbara)
**	    Initialize RDF_CB by call to pst_rdfcb_init().
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	16-nov-1992 (pholman)
**	    C2: replace obsolete 'qeu_audit' call with 'psy_secaudit'
**	04-apr-1993 (ralph)
**	    DELIM_IDENT:  Use pss_cat_owner instead of "$ingres"
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed cause of a compiler warning
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**	06-mar-96 (nanpr01)
**	    Move the QSF request block initialization up. because if  
**	    pst_rgnent returns a failure status code, subsequent QSF
**	    calls get bad control block error.
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**	21-Oct-2010 (kiria01) b124629
**	    Use the macro symbol with ult_check_macro instead of literal.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_dpermit(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_sqlview(
	PSS_RNGTAB *rngvar,
	PSS_SESBLK *sess_cb,
	DB_ERROR *err_blk,
	i4 *issql);

#ifdef	xORANGE
/*
** @FIX_ME@
** Don't allow non-DBA to issue GRANT, CREATE PERMIT,
** DEFINE PERMIT or CREATE SECUIRTY_ALARM in xORANGE.
** This restriction is bypassed for user "$ingres"
** so that CREATEDB (or actually UPGRADEFE) will function properly.
** The following constant is defined to compare the current user
** name with user name "$ingres".
** @FIX_ME@
*/
#endif

/*{
** Name: psy_dpermit	- Define a permit.
**
**  INTERNAL PSF call format: status = psy_dpermit(&psy_cb, sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_DPERMIT, &psy_cb, sess_cb);
**
** Description:
**	Given all of the parameters necessary to CREATE/DEFINE a permit on a
**	table or view, this function will store the permission in the system
**	catalogs.  This will include storing the query tree in the tree table,
**	storing the text of the query in the iiqrytext table (really done by
**	QEF), storing a row in the protect table, and issuing an "alter table"
**	operation to DMF to indicate that there are permissions on the given
**	table.
**
** Inputs:
**      psy_cb
**	    .psy_qrytext		Id of query text as stored in QSF.
**	    .psy_cols[]			Array of columns on which to grant
**					permission
**	    .psy_numcols		Number of columns listed above; 0 means
**					give permission on all columns
**          .psy_intree                 QSF id of query tree representing the
**					where clause in the permit
**          .psy_opctl                  Bit map of defined   operations
**          .psy_opmap                  Bit map of permitted operations
**          .psy_user                   Name of user who will get permission
**          .psy_terminal               Terminal at which permission is given
**					(blank if none specified)
**          .psy_timbgn                 Time of day at which the permission
**					begins (minutes since 00:00)
**          .psy_timend                 Time of day at which the permission ends
**					(minutes since 00:00)
**          .psy_daybgn                 Day of week at which the permission
**					begins (0 = Sunday)
**          .psy_dayend                 Day of week at which the permission ends
**					(0 = Sunday)
**	    .psy_grant
**		PSY_CPERM		CREATE/DEFINE PERMIT
**	    .psy_tblq			head of table queue
**	    .psy_colq			head of column queue
**	    .psy_usrq			head of user queue
**	    .psy_qlen			length of first iiqrytext
**	    .psy_flags			useful info
**		PSY_EXCLUDE_COLUMNS	user specified a list of columns to
**					which privilege should not apply
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**          .psy_txtid                  Id of query text as stored in the
**					iiqrytext system relation.
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores text of query in iiqrytext relation, query tree in tree
**	    relation, row in protect relation identifying the permit.  Does
**	    an alter table DMF operation to indicate that there are permissions
**	    on the table.
**
** History:
**	02-oct-85 (jeff)
**          written
**      03-sep-86 (seputis)
**          changed some psy_cb. to psy_cb->
**          added .db_att_id reference
**          changed rdr_cb. rdr_cb->
**	02-dec-86 (daved)
**	    bug fixing. check for permit on tables owned by user and not
**	    view.
**	29-apr-87 (stec)
**	    Implemented changes for GRANT statement.
**	10-may-88 (stec)
**	    Make changes for db procs.
**	03-oct-88 (andre)
**	    Modified call to pst_rgent to pass 0 as a query mode since it is
**	    clearly not PSQ_DESTROY
**	06-feb-89 (ralph)
**	    Added support for 300 attributes:
**		Use DB_COL_BITS in place of DB_MAX_COLS
**		Loop over domset array using DB_COL_WORDS
**	06-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Initialize new DB_PROTECTION fields, dbp_seq and dbp_gtype
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Use DBGR_USER when initializing dbp_gtype
**	08-may-89 (ralph)
**	    Initialize reserved field to blanks (was \0)
**	04-jun-89 (ralph)
**	    Initialize dbp_fill1 to zero
**	    Fix unix portability problems
**	02-nov-89 (neil)
**	    Alerters: Allowed privileges for events.
**	1-mar-90 (andre)
**	    If processing a GRANT on tables, check if 
**	    ALL-TO-ALL or RETRIEVE-TO-ALL has already been granted, and if so,
**	    mark psy_mask appropriately.
**	    If user tried to CREATE ALL/RETRIEVE-TO-ALL, and one already exists,
**	    skip to the exit.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	08-aug-90 (ralph)
**	    Initialize new fields in iiprotect tuple
**	14-dec-90 (ralph)
**	    Disallow use of GRANT by non-DBA if xORANGE
**	11-jan-90 (ralph)
**	    Allow user "$ingres" to use GRANT if xORANGE.
**	    This was done for CREATEDB (UPGRADEFE).
**	20-feb-91 (andre)
**	    For CREATE/DEFINE PERMIT, grantee type was stored in
**	    psy_cb->psy_gtype.
**	24-jun-91 (andre)
**	    IIPROTECT tuples for table permits will contain exactly one
**	    privilege.  IIQRYTEXT template built for table-wide privileges
**	    contains a placeholder for a privilege name which will be filled in
**	    with each of the table-wide privileges being granted, one at a time.
**	    PSY_CB.psy_opmap will be set to correspond with privilege name
**	    stored in the IIQRYTEXT permit.
**	16-jul-91 (andre)
**	    responsibility for splitting permit tuples will passed on to
**	    qeu_cprot().  If a permit specified only one privilege, we will
**	    substitute the appropriate privilege name here and will not ask
**	    qeu_cprot() to split tuples.
**	06-aug-91 (andre)
**	    before proceeding to CREATE a permit on a view owned by the current
**	    user, we will call psy_tbl_grant_check() to ensure that this user
**	    may create a permit on his view.  If the object is not owned by the
**	    current user, we will not try to verify that the user may
**	    CREATE/DEFINE a permit since (until the relevant FE changes are
**	    made) we intend to continue allowing any user with CATUPD to
**	    CREATE/DEFINE permits on catalogs and the dba will be allowed to
**	    CREATE/DEFINE permits on extended catalogs
**	11-nov-91 (rblumer)
**	  merged from 6.4:  26-feb-91 (andre)
**	    PST_QTREE was changed to store the range table as an array of
**	    pointers to PST_RNGENTRY structure.
**	14-feb-92 (andre)
**	    we will no longer have to fill in privilege name for permits
**	    specifying one privilege - it will be handled in respective
**	    grammars.
**	15-jun-92 (barbara)
**	    For Sybil, change interface to pst_rgent(), Star returns from
**	    psy_dpermit before permits get stored.
**	07-jul-92 (andre)
**	    DB_PROTECTION tuple will contain an indicator of how the permit was
**	    created, i.e. whether it was created using SQL or QUEL and if the
**	    former, then whether it was created using GRANT statement.  Having
**	    this information will facilitate merging similar and identical
**	    permit tuples.
**	14-jul-92 (andre)
**	    semantics of GRANT ALL [PRIVILEGES] is different from that of
**	    CREATE PERMIT ALL in that the former (as dictated by SQL92) means
**	    "grant all privileges which the current auth id posesses WGO"
**	    whereas the latter (as is presently interpreted) means "grant all
**	    privileges that can be defined on the object" which in case of
**	    tables and views means SELECT, INSERT, DELETE, UPDATE.
**	    psy_tbl_grant_check() (function responsible for determining whether
**	    a user may grant specified privilege on a specified table or view)
**	    will have to be notified whether we are processing GRANT ALL.  Its
**	    behaviour will change as follows:
**	      - if processing GRANT ALL and psy_tbl_grant_check() determines
**	        that the user does not possess some (but not all) of the
**		privileges passed to it by the caller it will not treat it as an
**		error, but will instead inform the caller of privileges that the
**		user does not posess,
**	      - if processing GRANT ALL and psy_tbl_grant_check() determines
**	        that the user does not possess any of the privileges passed to
**		it by the caller it will treat it as an error
**	      - if processing a statement other than GRANT ALL and
**	        psy_tbl_grant_check() determines that the user does not possess
**		some of the privileges passed to it by the caller it will treat
**		it as an error
**	16-jul-92 (andre)
**	    if a permit being created depends on some privileges, build a
**	    structure describing these privileges and store its address in
**	    rdf_cb->rdr_indep.
**	18-jul-92 (andre)
**	    we will no longer be telling QEF to turn off DMT_ALL_PROT or
**	    DMT_RETRIEVE_PRO when a user creates ALL/RETRIEVE TO ALL permit.
**	    QEF will figure out on its own whether PUBLIC now has RETRIEVE or
**	    ALL on a table/view
**	20-jul-92 (andre)
**	    if user specified a list of columns to which privilege(s) should
**	    not apply, set dbp_domset correctly
**	03-aug-92 (barbara)
**	    Invalidate base table infoblk from RDF cache for CREATE PERMIT
**	    and CREATE SEC_ALARM.
**	16-sep-92 (andre)
**	    privilege maps are build using bitwise ops, so care should be
**	    exercised when accessing it using BT*() functions
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	5-jul-93 (robf)
**	    changed interface of  psy_secaudit() to accept security label
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	06-mar-96 (nanpr01)
**	    Move the QSF request block initialization up. because if  
**	    pst_rgnent returns a failure status code, subsequent QSF
**	    calls get bad control block error.
*/
DB_STATUS
psy_dpermit(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    RDF_CB              rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    QSF_RCB		qsf_rb;
    DB_STATUS		status;
    DB_STATUS		stat;
    DB_PROTECTION	ptuple;
    register DB_PROTECTION *protup = &ptuple;
    i4			*domset	= ptuple.dbp_domset;
    register i4	i, j;
    i4		err_code;
    PSS_RNGTAB		*rngvar;
    PSS_USRRANGE	*rngtab;
    PST_PROCEDURE	*pnode;
    PST_QTREE		*qtree;
    DB_ERROR		*err_blk = &psy_cb->psy_error;
    i4			textlen;
    i4			tree_lock   = 0;
    i4			text_lock   = 0;
    DB_TAB_ID		tabids[PST_NUMVARS];
    PSQ_INDEP_OBJECTS   indep_objs;
    PSQ_OBJPRIV         obj_priv;       /* space for independent DELETE */
    PSQ_COLPRIV         col_privs[2];   /*
                                        ** space for independent INSERT and
					** UPDATE
					*/
    PST_VRMAP		varmap;
    PSY_TBL		*psy_tbl;
    DB_TIME_ID		timeid;

    /*
    ** For CREATE/DEFINE PERMIT execute code below.
    */

    /* initialize the QSF control block */
    qsf_rb.qsf_type	= QSFRB_CB;
    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb.qsf_length	= sizeof(qsf_rb);
    qsf_rb.qsf_owner	= (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid	= sess_cb->pss_sessid;

    rngtab = &sess_cb->pss_auxrng;

    /* table info is stored in the only entry in the table queue */
    psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;

    status = pst_rgent(sess_cb, rngtab, -1, "", PST_SHWID,
	(DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL,
	&psy_tbl->psy_tabid, TRUE, &rngvar, (i4) 0, err_blk);
	
    if (DB_FAILURE_MACRO(status))
	goto exit;

    /* In STAR, we do not actually store permits */
    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	qsf_rb.qsf_lk_state = QSO_EXLOCK;
	goto exit;
    }

    /* Fill in the RDF request block */
    pst_rdfcb_init(&rdf_cb, sess_cb);

    /* The table which is receiving the permit */
    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, rdf_rb->rdr_tabid);
    
    /* Tell RDF we're doing a permit definition */
    rdf_rb->rdr_update_op   = RDR_APPEND;

    rdf_rb->rdr_types_mask = RDR_PROTECT;
    rdf_rb->rdr_qrytuple = (PTR) protup;

    /* initialize independent object structure */
    indep_objs.psq_objs	= (PSQ_OBJ *) NULL;
    indep_objs.psq_objprivs = (PSQ_OBJPRIV *) NULL;
    indep_objs.psq_colprivs = (PSQ_COLPRIV *) NULL;
    indep_objs.psq_grantee  = &sess_cb->pss_user;

    rdf_rb->rdr_indep	    = (PTR) &indep_objs;

    /*
    ** populate the IIPROTECT tuple
    */

    /* Zero out the template */
    (VOID)MEfill(sizeof(ptuple), (u_char) 0, (PTR) protup);

    /* store grantee type */
    protup->dbp_gtype = psy_cb->psy_gtype;

    /* Init reserved block */
    (VOID)MEfill(sizeof(protup->dbp_reserve),
	(u_char) ' ', (PTR) protup->dbp_reserve);

    /* Init obj name */
    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm, protup->dbp_obname);

    /*@FIX_ME@ Where does this come from? */
    protup->dbp_obstat = ' ';

    /* store the object type indicator */
    if (psy_tbl->psy_mask & PSY_OBJ_IS_TABLE)
    {
	protup->dbp_obtype = DBOB_TABLE;
    }
    else if (psy_tbl->psy_mask & PSY_OBJ_IS_VIEW)
    {
	protup->dbp_obtype = DBOB_VIEW;
    }
    else
    {
	protup->dbp_obtype = DBOB_INDEX;
    }

    STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, protup->dbp_obown);

    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, protup->dbp_grantor);

    TMnow((SYSTIME *)&timeid);
    protup->dbp_timestamp.db_tim_high_time = timeid.db_tim_high_time;
    protup->dbp_timestamp.db_tim_low_time  = timeid.db_tim_low_time;

    /* The table on which we're giving permission */
    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, protup->dbp_tabid);

    /* Beginning and ending times of day */
    protup->dbp_pdbgn = psy_cb->psy_timbgn;
    protup->dbp_pdend = psy_cb->psy_timend;

    /* Beginning and ending days of week */
    protup->dbp_pwbgn = psy_cb->psy_daybgn;
    protup->dbp_pwend = psy_cb->psy_dayend;

    if (psy_cb->psy_numcols != 0 && ~psy_cb->psy_flags & PSY_EXCLUDE_COLUMNS)
    {
	/* user specified a list of columns to which privilege(s) will apply */
	
	/* Bit map of permitted columns */
	psy_fill_attmap(domset, ((i4) 0));

	for (i = 0; i < psy_cb->psy_numcols; i++)
	{
	    BTset((i4)psy_cb->psy_cols[i].db_att_id, (char *) domset);
	}	
    }
    else
    {
	/*
	** user specified table-wide privilege(s) or a list of columns L s.t.
	** privilege(s) will apply to the entire table except for columns in L
	*/

	psy_fill_attmap(domset, ~((i4) 0));

	if (psy_cb->psy_flags & PSY_EXCLUDE_COLUMNS)
	{
	    /*
	    ** exclude specified columns from the list of columns to which
	    ** privilege(s) will apply
	    */
	    for (i = 0; i < psy_cb->psy_numcols; i++)
	    {
		BTclear((i4) psy_cb->psy_cols[i].db_att_id, (char *) domset);
	    }
	}
    }

    if (rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
    {
	/*
	** if view is owned by the current user, psy_tbl_grant_check() will
	** determine if the permit can, indeed, be created;  as long as we are
	** preserving the kludge that allows users with CATUPD create permits on
	** catalogs and DBAs to create permits on extended catalogs, we shall
	** not call psy_tbl_grant_check() on view not owned by the current user,
	** since it is likely to result in psy_tbl_grant_check() complaining
	** about inadequate permissions
	*/
	if (!MEcmp((PTR) &rngvar->pss_ownname, (PTR) &sess_cb->pss_user,
	    sizeof(sess_cb->pss_user)))
	{
	    i4			    tbl_wide_privs;
	    PSY_COL_PRIVS	    col_specific_privs, *csp,
				    indep_col_specific_privs;
	    DB_TAB_ID		    indep_id;
	    i4			    indep_tbl_wide_privs;
	    bool		    insuf_privs, quel_view;
	    i4		    val1, val2;
	    u_i4	u;
	    /*
	    ** build maps of table-wide and column-specific privileges for
	    ** psy_tbl_grant_check()
	    ** if a column list was specified with CREATE PERMIT and
	    ** privileges specified in the statement include a set of
	    ** privileges S s.t. for all P in S, P can only be specified as
	    ** table-wide with GRANT statement (currently this includes
	    ** SELECT, INSERT, DELETE), we will make 
	    ** psy_tbl_grant_check() think that privileges in S are
	    ** table-wide.
	    ** This will work correctly since if the view was defined over
	    ** some objects owned by other user(s), for every P in S we
	    ** would need table-wide privilege WGO on the underlying object.
	    **
	    ** For the purposes of providing more descriptive output for
	    ** trace point ps131, if column-list was specified, we will pass
	    ** the map of attributes even if column-specific UPDATE was not
	    ** specified
	    */

	    if (psy_cb->psy_numcols != 0 &&
		(psy_cb->psy_opmap & DB_REPLACE ||
		 ult_check_macro(&sess_cb->pss_trace,
				PSS_TBL_VIEW_GRANT_TRACE, &val1, &val2)
		)
	       )
	    {
		i4	    *ip;

		csp = &col_specific_privs;

		/*
		** column-specific UPDATE privilege will not be translated into
		** a table-wide privilege since GRANT allows for specification
		** of column-specific UPDATE privilege
		*/
		csp->psy_col_privs = psy_cb->psy_opmap & DB_REPLACE;
		tbl_wide_privs = psy_cb->psy_opmap & ~DB_REPLACE;

		/*
		** if creating a permit on a set of columns and UPDATE is not
		** one of the privileges named in the statement, store the
		** attribute map in the first element of the attribute map list
		*/
		ip = (csp->psy_col_privs)
		    ? csp->psy_attmap[PSY_UPDATE_ATTRMAP].map
		    : csp->psy_attmap->map;

		/* copy the attribute map */
		for (u = 0; u < DB_COL_WORDS; u++, ip++)
		{
		    *ip = domset[u];
		}
	    }
	    else
	    {
		tbl_wide_privs = psy_cb->psy_opmap;
		csp = (PSY_COL_PRIVS *) NULL;
	    }

	    status = psy_tbl_grant_check(sess_cb, (i4) PSQ_PROT,
		&rngvar->pss_tabid, &tbl_wide_privs, csp, &indep_id,
		&indep_tbl_wide_privs, &indep_col_specific_privs,
		psy_cb->psy_flags, &insuf_privs, &quel_view,
		&psy_cb->psy_error);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto exit;
	    }

	    if (insuf_privs)
	    {
		/* must audit failure to create a permit */
		if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
		{
		    DB_ERROR	e_error;

		    /* Must audit CREATE PERMIT failure. */
		    status = psy_secaudit(FALSE, sess_cb,
			    (char *)&rngvar->pss_tabdesc->tbl_name,
			    &rngvar->pss_tabdesc->tbl_owner,
			    sizeof(DB_TAB_NAME), SXF_E_TABLE,
			    I_SX2016_PROT_TAB_CREATE, SXF_A_FAIL | SXF_A_CREATE,
			    &e_error);
		    
		    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
		}
		goto exit;
	    }
	    else if (quel_view)
	    {
		goto exit;
	    }
	    
	    /*
	    ** If user is trying to grant one or more of
	    ** INSERT/DELETE/UPDATE on his/her view whose underlying table
	    ** or view is owned by another user, psy_tbl_grant_check() will
	    ** return id of the underlying object along with map of
	    ** privileges.  We will convert maps of independent privileges
	    ** into elements of independent privilege list and pass them
	    ** along to QEF
	    */
	    if (   indep_id.db_tab_base != (i4) 0
		&& (   indep_id.db_tab_base != rngvar->pss_tabid.db_tab_base
		    || indep_id.db_tab_index !=
			   rngvar->pss_tabid.db_tab_index
		   )
	       )
	    {
		if (indep_tbl_wide_privs & DB_DELETE)
		{
		    /*
		    ** the only expected independent table-wide privilege
		    ** is DELETE
		    */
		    obj_priv.psq_next		= (PSQ_OBJPRIV *) NULL;
		    obj_priv.psq_objtype		= PSQ_OBJTYPE_IS_TABLE;
		    obj_priv.psq_privmap		= (i4) DB_DELETE;
		    obj_priv.psq_objid.db_tab_base	= indep_id.db_tab_base;
		    obj_priv.psq_objid.db_tab_index = indep_id.db_tab_index;
		    indep_objs.psq_objprivs		= &obj_priv;
		}

		if (indep_col_specific_privs.psy_col_privs)
		{
		    i4		i;
		    u_i4	u;
		    PSQ_COLPRIV	*csp;
		    i4		*att_map, *p;
		    i4		priv_map = 0;

		    /*
		    ** privilege map is built using bitwise operators, but
		    ** here using BTnext() makes code much more palatable,
		    ** so convert a privilege map
		    */
		    if (indep_col_specific_privs.psy_col_privs & DB_APPEND)
			BTset(DB_APPP, (char *) &priv_map);
		    if (indep_col_specific_privs.psy_col_privs & DB_REPLACE)
			BTset(DB_REPP, (char *) &priv_map);

		    for (i = -1, csp = col_privs;
			 (i = BTnext(i, (char *) &priv_map, BITS_IN(priv_map)))
			      != -1;
			  csp++
			)
		    {
			csp->psq_next = indep_objs.psq_colprivs;
			indep_objs.psq_colprivs = csp;
			csp->psq_objtype = PSQ_OBJTYPE_IS_TABLE;
			csp->psq_tabid.db_tab_base = indep_id.db_tab_base;
			csp->psq_tabid.db_tab_index = indep_id.db_tab_index;
			switch (i)
			{
			    case DB_APPP:	    /* INSERT privilege */
			    {
				csp->psq_privmap = (i4) DB_APPEND;
				att_map = indep_col_specific_privs.
				    psy_attmap[PSY_INSERT_ATTRMAP].map;
				break;
			    }
			    case DB_REPP:
			    {
				csp->psq_privmap = (i4) DB_REPLACE;
				att_map = indep_col_specific_privs.
				    psy_attmap[PSY_UPDATE_ATTRMAP].map;
				break;
			    }
			}

			for (p = csp->psq_attrmap, u = 0;
			     u < DB_COL_WORDS;
			     u++)
			{
			    *p++ = *att_map++;
			}
		    }
		}
	    }
	}
	else
	{
	    /*
	    ** either this is a catalog and the user has CATUPD or
	    ** this is an extended catalog and the user is the DBA;
	    ** since we may be allowing a user to create a permit by
	    ** circumventing the permit system, we only need to ascertain that
	    ** this is an SQL view
	    */
	    i4	    issql = 0;

	    status = psy_sqlview(rngvar, sess_cb, err_blk, &issql);
	    if (status)
	    {
		goto exit;
	    }
	    if (!issql)
	    {
		/* can only have permits on SQL views */
		psf_error(3598L, 0L, PSF_USERERR, &err_code, err_blk, 1, 
		    psf_trmwhite(sizeof(rngvar->pss_tabname),
			(char *) &rngvar->pss_tabname),
		    &rngvar->pss_tabname);
		status = E_DB_ERROR;
		goto exit;
	    }
	}
    }
    /* Name of user getting permission */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);

    /* Terminal at which permission given */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_terminal, protup->dbp_term);

    /* Give RDF pointer to query tree, if any */
    if (!psy_cb->psy_istree)
    {
	rdf_rb->rdr_qry_root_node = (PTR) NULL;
    }
    else
    {
	PST_VRMAP   varset;
	i4	    j;

	STRUCT_ASSIGN_MACRO(psy_cb->psy_intree, qsf_rb.qsf_obj_id);
	qsf_rb.qsf_lk_state = QSO_EXLOCK;
	status = qsf_call(QSO_LOCK, &qsf_rb);
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_error(E_PS0D19_QSF_INFO, qsf_rb.qsf_error.err_code,
		PSF_INTERR, &err_code, err_blk, 0);
	    goto exit;
	}

	tree_lock		= qsf_rb.qsf_lk_id;
	pnode = (PST_PROCEDURE *) qsf_rb.qsf_root;
	qtree = (PST_QTREE *) pnode->pst_stmts->pst_specific.pst_tree;
	rdf_rb->rdr_qry_root_node = (PTR) pnode;
	/* check for no views in the qualification.
	*/
	(VOID)psy_varset(qtree->pst_qtree, &varset);	
	j = BTnext(-1, (char *) &varset, BITS_IN(varset));
	for ( ; j >= 0; j = BTnext(j, (char *) &varset, BITS_IN(varset)))
	{
	    status = pst_rgent(sess_cb, rngtab, -1, "", PST_SHWID,
		(DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL,
		&qtree->pst_rangetab[j]->pst_rngvar, TRUE,
		&rngvar, (i4) 0, err_blk);
	    if (status)
		goto exit;		

	    if (rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
	    {
		psf_error(3597L, 0L, PSF_USERERR, &err_code, err_blk, 1,
		    psf_trmwhite(sizeof(rngvar->pss_tabname),
			(char *) &rngvar->pss_tabname),
		    &rngvar->pss_tabname);
		    status = E_DB_ERROR;
		    goto exit;
	    }
	}
    }
    
    /* Give RDF a pointer to the query text to be stored in iiqrytext */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, qsf_rb.qsf_obj_id);
    qsf_rb.qsf_lk_state = QSO_EXLOCK;
    status = qsf_call(QSO_LOCK, &qsf_rb);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(E_PS0D19_QSF_INFO, qsf_rb.qsf_error.err_code,
	    PSF_INTERR, &err_code, err_blk, 0);
	goto exit;
    }

    text_lock = qsf_rb.qsf_lk_id;

    MEcopy((char *) qsf_rb.qsf_root, sizeof(i4), (char *) &textlen);
    rdf_rb->rdr_l_querytext = textlen;
    rdf_rb->rdr_querytext = ((char *) qsf_rb.qsf_root) + sizeof(i4);
    rdf_rb->rdr_status = (sess_cb->pss_lang == DB_SQL) ? DB_SQL : 0;

    /* determine if the permit specifies exactly one privilege */
    if (BTcount((char *) &psy_cb->psy_opmap, BITS_IN(psy_cb->psy_opmap)) > 1)
    {
	/*
	** if permit specified more than one privilege, notify QEF that it will
	** have to split the permit into multiple IIPROTECT tuples
	*/
	rdf_rb->rdr_instr |= RDF_SPLIT_PERM;
    }
    else if (psy_cb->psy_opmap & DB_RETRIEVE)
    {
	/*
	** if qeu_cprot() will not be splitting the permit into multiple tuples
	** and RETRIEVE is the privilege mentioned in it, set the two bits
	** associated with DB_RETRIEVE
	*/
	psy_cb->psy_opmap |= DB_TEST | DB_AGGREGATE;
	psy_cb->psy_opctl |= DB_TEST | DB_AGGREGATE;
    }

    /* Null out the DMU control block pointer, just in case */
    rdf_rb->rdr_dmu_cb = (PTR) NULL;

    /* produce list of dependent tables */
    rdf_rb->rdr_cnt_base_id = 0;
    if (psy_cb->psy_istree && qtree->pst_qtree) 
    {
	j = 0;
	(VOID)psy_varset(qtree->pst_qtree, &varmap);
	for (i = -1; (i = BTnext(i, (char*) &varmap, PST_NUMVARS)) > -1;)
	{
	    /* if this is the table that is getting the permit, ignore */
	    if (qtree->pst_rangetab[i]->pst_rngvar.db_tab_base != 
		    psy_tbl->psy_tabid.db_tab_base
		||
		qtree->pst_rangetab[i]->pst_rngvar.db_tab_index !=
		    psy_tbl->psy_tabid.db_tab_index
	    )
	    {
		rdf_rb->rdr_cnt_base_id++;
		STRUCT_ASSIGN_MACRO(qtree->pst_rangetab[i]->pst_rngvar,
		    tabids[j++]);
	    }
	}
	rdf_rb->rdr_base_id = tabids;
    }

    protup->dbp_popctl = psy_cb->psy_opctl;
    protup->dbp_popset = psy_cb->psy_opmap;
    
    /*
    ** store an indication of whether this permit is being created using SQL or
    ** QUEL
    */
    protup->dbp_flags = (sess_cb->pss_lang == DB_SQL) ? DBP_SQL_PERM : (i2) 0;
    protup->dbp_flags |= DBP_65_PLUS_PERM;

    /* Now let RDF do all the work of the permit definition */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
	{
	    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
		&err_code, err_blk, 1,
		psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
		    (char *) &psy_tbl->psy_tabnm),
		&psy_tbl->psy_tabnm);
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		&psy_cb->psy_error);
	}
	goto exit;
    }

    /*
    ** Invalidate base object's infoblk from RDF cache.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[0], rdf_rb->rdr_tabid);
    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
				&psy_cb->psy_error);
    }

exit:

    qsf_rb.qsf_lk_state = QSO_EXLOCK;

    if (psy_cb->psy_istree)
    {
	/* Destroy query tree */
	STRUCT_ASSIGN_MACRO(psy_cb->psy_intree, qsf_rb.qsf_obj_id);

	if ((qsf_rb.qsf_lk_id = tree_lock) == 0)
	{
	    stat = qsf_call(QSO_LOCK, &qsf_rb);
	    if (DB_FAILURE_MACRO(stat))
	    {
		(VOID) psf_error(E_PS0D18_QSF_LOCK,
		    qsf_rb.qsf_error.err_code, PSF_INTERR,
		    &err_code, &psy_cb->psy_error, 0);
		if (!status || stat == E_DB_FATAL)
		    status = stat;
	    }
	    tree_lock = qsf_rb.qsf_lk_id;
	}

	stat = qsf_call(QSO_DESTROY, &qsf_rb);
	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_error(E_PS0D1A_QSF_DESTROY,
		qsf_rb.qsf_error.err_code, PSF_INTERR,
		&err_code, &psy_cb->psy_error, 0);
	    if (!status || stat == E_DB_FATAL)
		status = stat;
	}

	tree_lock = 0;
    }

    /* Destroy query text */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, qsf_rb.qsf_obj_id);

    if ((qsf_rb.qsf_lk_id = text_lock) == 0)
    {
	stat = qsf_call(QSO_LOCK, &qsf_rb);
	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_error(E_PS0D18_QSF_LOCK, qsf_rb.qsf_error.err_code,
		PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    if (!status || stat == E_DB_FATAL)
		status = stat;
	}
	text_lock = qsf_rb.qsf_lk_id;
    }

    stat = qsf_call(QSO_DESTROY, &qsf_rb);
    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_error(E_PS0D1A_QSF_DESTROY, qsf_rb.qsf_error.err_code,
	    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	if (!status || stat == E_DB_FATAL)
	    status = stat;
    }

    return (status);
}

/*  PSY_SQLVIEW - IS THIS AN SQL VIEW
**
**  Description:
**	This routine takes a range entry and determines if it is an
**  SQL view. This routine assumes that we know that the range entry
**  refers to a view.
**
** History:
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
*/
DB_STATUS
psy_sqlview(
	PSS_RNGTAB	    *rngvar,
	PSS_SESBLK	    *sess_cb,
	DB_ERROR	    *err_blk,
	i4		    *issql)
{
    DB_STATUS	    status = E_DB_OK;
    RDF_CB	    rdf_cb;
    PST_PROCEDURE   *pnode;
    PST_QTREE	    *vtree;
    i4	    err_code;

    *issql = FALSE;
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_cb.rdf_rb.rdr_tabid);
    rdf_cb.rdf_rb.rdr_types_mask = RDR_VIEW | RDR_QTREE ;
    rdf_cb.rdf_rb.rdr_qtuple_count = 1;
    rdf_cb.rdf_info_blk = rngvar->pss_rdrinfo;
    status = rdf_call(RDF_GETINFO, (PTR) &rdf_cb);

    /*
    ** RDF may choose to allocate a new info block and return its address in
    ** rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid memory
    ** leak and other assorted unpleasantries
    */
    rngvar->pss_rdrinfo = rdf_cb.rdf_info_blk;
    
    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
	{
	    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, rdf_cb.rdf_error.err_code,
		PSF_INTERR, &err_code, err_blk, 1,
		psf_trmwhite(sizeof(DB_TAB_NAME), 
		    (char *) &rngvar->pss_tabname),
		&rngvar->pss_tabname);
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb.rdf_error, err_blk);
	}
	return (status);
    }
    pnode = (PST_PROCEDURE *) rdf_cb.rdf_info_blk->rdr_view->qry_root_node;
    vtree = pnode->pst_stmts->pst_specific.pst_tree;
    if (vtree->pst_qtree->pst_sym.pst_value.pst_s_root.pst_qlang == DB_SQL)
	*issql = TRUE;

    return (status);
}
