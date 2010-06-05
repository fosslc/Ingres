/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <tm.h>
#include    <me.h>
#include    <qu.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <sxf.h>
#include    <psyaudit.h>

/**
**
**  Name: PSYDBPROC.C - Code related to database procedure related
**			operations.
**
**  Description:
**	This module contains the code used to store, delete and retrieve
**	database procedures.
**
**          psy_cproc - Create a database procedure definition in
**			the system catalogs.
**	    psy_kproc - Remove a database procedure definition from
**			the system catalogs.
**	    psy_gproc - Get a database procedure definition from
**			the system catalogs.
**
**  History:
**      27-apr-88 (stec)
**	    Created.
**	04-aug-88 (stec)
**	    Improve recovery of resources.
**	17-aug-88 (stec)
**	    Change bad STRUCT_ASSIGN statements.
**	23-aug-88 (stec)
**	    Added comments, changed psy_cproc.
**	27-sep-88 (stec)
**	    Fixed Lint warnings.
**	28-sep-88 (stec)
**	    Make changes in psy_gproc that cms merge missed.
**	28-sep-88 (stec)
**	    psy_gproc must not unfix RDF entry.
**	16-mar-89 (andre)
**	    psy_gproc will no longer reset pss_user or set pss_ruset.  Instead
**	    it may set the ptr passed to it to the name of the dbproc owner.
**	16-mar-89 (neil)
**	    Modified psy_gproc to access the procedure through the specified
**	    owner if psq_alias_set is on.
**	03-jan-90 (ralph)
**	    Change interface to QSO routines for dbprocs
**	11-nov-91 (rblumer)
**	  merged from 6.4:  19-aug-91 (andre)
**	    added parentheses around some pointer arithmetic 
**	03-aug-92 (barbara)
**	    Wherever we call rdf_call, call pst_rdfcb_init for some
**	    default setup of RDF_CB.
**	06-aug-92 (teresa)
**	    change interface to invalidate procedure by name, owner.
**	30-nov-92 (anitap)
**	    Moved around the order of #includes of header files for CREATE
**	    SCHEMA.
**	22-feb-93 (rblumer)
**	    in psy_cproc: initialize new RDF proc_param variables for both
**	    normal and set-input procedures; reverse order of MEcopy and
**	    I4assign in QP cleanup code so that qsf id gets set up correctly.
**	03-apr-93 (ralph)
**	    DELIM_IDENT:  Use pss_cat_owner instead of "$ingres"
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	15-oct-1993 (rog)
**	    Use ulm_copy() to copy more than 64k bytes rather than MEcopy().
**	    (bug 55486)
**	22-oct-93 (rblumer)
**	    changed psy_cproc to handle parameters for normal procedures,
**	    in addition to set-input procedures.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**      16-jun-94 (andre)
**          Bug #64395
**          it is dangerous to cast a (char *) into a (DB_CURSOR_ID *) and then
**          dereference the resulting pointer because the chat ptr may not be
**          pointing at memory not aligned on an appopriate boundary.  Instead,
**          we will allocate a DB_CURSOR_ID structure on the stack, initialize
**          it and MEcopy() it into the char array
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**      15-Apr-2003 (bonro01)
**          Added include <psyaudit.h> for prototype of psy_secaudit()
**	28-Jan-2004 (schka24)
**	    Don't lie about which memory pool runs out.  If it's QSF,
**	    say so.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/

/*{
** Name: psy_cproc - Create a database procedure definition in
**		     the system catalogs.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Exceptions:
**	    none
**
** Side Effects:
**	    Modifies system catalogs.
**
** History:
**      27-apr-88 (stec)
**	    Created.
**	23-aug-88 (stec)
**	    Added comments, initialize new fields in DB_PROCEDURE.
**	26-apr-89 (andre)
**	    For internal procedures, set db_mask[0] to DB_IPROC.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	12-jun-90 (andre)
**	    when trying to destroy a dbproc QEP, use "private" alias, which is
**	    always defined as opposed to the "public" alias which would not be
**	    defined if the dbproc is not grantable.
**	04-mar-92 (andre)
**	    set DB_ACTIVE_DBP in db_mask[0] to indicate that the dbproc is not
**	    abandoned.  If the dbproc is grantable, set DB_DBPGRANT_OK in
**	    db_mask[0].
**	18-may-92 (andre)
**	    call psy_dbp_status() to verify that the new dbproc is not abandoned
**	    and trust psy_dbp_status() to determine whether the dbproc is
**	    grantable or just active.
**	19-may-92 (andre)
**	    in pslsgram.yi, pss_dependencies_stream was opened to collect info
**	    about objects/privileges on which the new dbproc depends;
**	    this stream must be closed before leaving this function since the
**	    dependence information will be of no use once we return
**	01-jun-92 (andre)
**	    pass information about objects/privileges on which the new database
**	    procedure depends to QEF.
**	26-jun-92 (andre)
**	    enter information about the dbproc into IIPROCEDURE and the list of
**	    objects and privileges on which it depends into IIDBDEPENDS and
**	    IIPRIV respectively before calling psy_dbp_status() to determine
**	    whether it is active.  This is necessary since otherwise it would be
**	    impossible to create mutually recursive database procedures (if P1
**	    calls P2 and we are trying to create P2 calling P1, psy_dbp_status()
**	    will report that P1 is dormant and prevent us from creating P2)
**
**	    Since we won't know whether the dbproc is active and/or grantable
**	    until psy_dbp_status() is done, we will set only DB_DBP_INDEP_LIST
**	    bit in IIPROCEDURE.dbp_mask1 here, unless the independent
**	    object/privilege list is empty, in which case there is no reason to
**	    call psy_dbp_status(), so we will set DB_DBPGRANT_OK and
**	    DB_ACTIVE_DBP bits in IIPROCEDURE.dbp_mask1.
**
**	    If the independent object/privilege list is not empty and the
**	    procedure is not dormant, psy_dbp_status() will set appropriate bits
**	    (DB_DBPGRANT_OK and/or DB_ACTIVE_DBP) in IIPROCEDURE.dbp_mask1 once
**	    it has determined whether the dbproc is active or grantable
**	09-sep-92 (andre)
**	    psy_dbp_status() no longer accepts or returns an indicator of
**	    whether cache had to be flushed in psy_dbp_priv_check()
**	07-nov-92 (andre)
**	    we will no longer create private aliases for dbproc QPs
**	22-feb-93 (rblumer)
**	    initialize new RDF proc_param variables for both normal and
**	    set-input procedures; reverse order of MEcopy and I4assign in
**	    QP cleanup code so that qsf id gets set up correctly.
**	13-apr-93 (andre)
**	    if creating a system-generated procedure, independent
**	    object/privilege list will be empty - QEF will insert IIDBDEPENDS
**	    tuple recording dependence of the dbproc on a constraint (for
**	    constraint-enforcing dbprocs) or view (for CHECK OPTION-enforcing
**	    dbprocs.)  Here we will set bits indicating that there will be
**	    independent object list and that the dbproc is active but not
**	    grantable (we don't want the user to grant privileges on
**	    system-generated dbprocs)
**	01-sep-93 (andre)
**	    in the course of parsing a dbproc definition, we will try to 
**	    determine id of a base table on which the dbproc depends; here we
**	    will copy it into proctuple.dbPdbp_ubt_id to ensure that it gets 
**	    inserted into IIPROCEDURE
**	22-oct-93 (rblumer)
**	    normal procedures will now have their parameters inserted into the
**	    catalogs (previously just set-input procedures did); changed
**	    initialization of db_parameterCount, rdr_proc_param_cnt and
**	    rdr_proc_params to work for both types of procedures.
**	30-apr-94 (andre)
**	    fix for bug 61087:
**	    proctuple.db_mask[0] was being set before proctuple was MEfill'd 
**	    with NULLCHAR; moved the line initializing proctuple.db_mask[0] 
**	    below call to MEfill() 
**	16-jun-94 (andre)
**	    Bug #64395
**	    it is dangerous to cast a (char *) into a (DB_CURSOR_ID *) and then
**	    dereference the resulting pointer because the chat ptr may not be 
**	    pointing at memory not aligned on an appopriate boundary.  Instead,
**	    we will allocate a DB_CURSOR_ID structure on the stack, initialize 
**	    it and MEcopy() it into the char array
**	19-june-06 (dougi)
**	    Add DBP_DATA_CHANGE flag for BEFORE trigger validation.
**	28-march-2008 (dougi)
**	    Changes to support table procedures and named result row elements.
**	4-feb-2009 (dougi)
**	    Tidy up computation of table procedure result row length.
**	30-mar-2009 (toumi01) b121871
**	    Rewrite and simplify the computation of table procedure input
**	    and output parameter count and width (fixes a logic error that
**	    caused the result width to be decremented by the width of the
**	    first result parameter when there were no input parameters).
*/
DB_STATUS
psy_cproc(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
{
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    i4			textlen;
    QSF_RCB		qsf_rb;
    DB_PROCEDURE	proctuple;
    DB_STATUS		status, stat;
    i4		err_code;
    PSY_TBL		dbp_descr;
    i4			dbp_mask[2];
    bool		empty_indep_list, rowproc;

    /* If this is a recreate case, and this code gets called
    ** just return since there is no work to be done, not even
    ** recovery of resources.
    */
    if (sess_cb->pss_dbp_flags & PSS_RECREATE)
	return(E_DB_OK);

    /* determine if the new dbproc depends on any object or privileges */
    if (   sess_cb->pss_indep_objs.psq_objs
	|| sess_cb->pss_indep_objs.psq_objprivs
	|| sess_cb->pss_indep_objs.psq_colprivs
       )
    {
	empty_indep_list = FALSE;
    }
    else
    {
	empty_indep_list = TRUE;
    }

    /*
    ** Initialize the header of the QSF control block
    ** NOTE: it is important that we init qsf_rb before calling
    **       psy_dbp_status() - if the dbproc cannot be created, code under
    **	     exit: will expect the control block set up for deleting the query
    **	     text QSF object and the query plan QSF object
    */
    qsf_rb.qsf_type = QSFRB_CB;
    qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb.qsf_length = sizeof(qsf_rb);
    qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid = sess_cb->pss_sessid;
    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, qsf_rb.qsf_obj_id);

    /* Retrieve info about the procedure text object */
    status = qsf_call(QSO_INFO, &qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
	goto exit;
    }

    /* Get the text length. */
    MEcopy((PTR) qsf_rb.qsf_root, sizeof(i4), (PTR) &textlen);

    /* Initialize the II_PROCEDURE tuple. */
    MEfill(sizeof(proctuple), NULLCHAR, (PTR) &proctuple);

    (VOID) MEcopy((PTR)&psy_cb->psy_tabname[0], sizeof(DB_DBP_NAME),   
	(PTR)&proctuple.db_dbpname);

    /* Current user is the owner. */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, proctuple.db_owner);

    proctuple.db_txtlen = textlen;

    TMnow((SYSTIME *) &proctuple.db_txtid);

    /*
    ** if the independent object/privilege list is non-empty, we will set
    ** DB_DBP_INDEP_LIST bit and leave it to psy_dbp_status() to set the
    ** remaining bits, if appropriate; otherwise we will set DB_DBPGRANT_OK and
    ** DB_ACTIVE_DBP bits and avoid calling psy_dbp_status()
    **
    ** if creating a system-generated dbproc, mark the dbproc ACTIVE and
    ** indicate that there is independent object list (there is not one at this
    ** point, but QEF will insert IIDBDEPENDS tuple recording dependence of the
    ** dbproc on a constraint (for constraint-enforcing dbprocs) or view (for
    ** CHECK OPTION-enforcingdbprocs.)
    */
    
    /* If working on internal dbproc, set DB_IPROC in db_mask[0] */
    if (sess_cb->pss_dbp_flags & PSS_IPROC)
	proctuple.db_mask[0] |= DB_IPROC;

    if (sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED)
	proctuple.db_mask[0] |=
	    DBP_SYSTEM_GENERATED | DB_ACTIVE_DBP | DB_DBP_INDEP_LIST;
    else if (empty_indep_list)
	proctuple.db_mask[0] |= DB_DBPGRANT_OK | DB_ACTIVE_DBP;
    else
	proctuple.db_mask[0] |= DB_DBP_INDEP_LIST;

    if (sess_cb->pss_dbp_flags & PSS_SET_INPUT_PARAM)
	proctuple.db_mask[0] |= DBP_SETINPUT;

    if (sess_cb->pss_dbp_flags & PSS_NOT_DROPPABLE)
	proctuple.db_mask[0] |= DBP_NOT_DROPPABLE;

    if (sess_cb->pss_dbp_flags & PSS_SUPPORTS_CONS)
	proctuple.db_mask[0] |= DBP_CONS;

    if (sess_cb->pss_dbp_flags & PSS_DATA_CHANGE)
	proctuple.db_mask[0] |= DBP_DATA_CHANGE;

    if (sess_cb->pss_dbp_flags & PSS_OUT_PARMS)
	proctuple.db_mask[0] |= DBP_OUT_PARMS;

    if (sess_cb->pss_dbp_flags & PSS_TX_STMT)
	proctuple.db_mask[0] |= DBP_TX_STMT;

    if (sess_cb->pss_dbp_flags & PSS_ROW_PROC)
    {
	rowproc = TRUE;
	proctuple.db_mask[0] |= DBP_ROW_PROC;
    }
    else rowproc = FALSE;

    proctuple.db_mask[1] = 0;

    /*
    ** if we were able to determine id of a base table on which this dbproc will
    ** depend, copy it into proctuple
    */
    proctuple.db_dbp_ubt_id.db_tab_base  = sess_cb->pss_dbp_ubt_id.db_tab_base;
    proctuple.db_dbp_ubt_id.db_tab_index = sess_cb->pss_dbp_ubt_id.db_tab_index;

    /* db_procid to be filled in by RDF or QEF */

    proctuple.db_parameterCount = 0;
    proctuple.db_recordWidth    = 0;
    proctuple.db_rescolCount = 0;
    proctuple.db_resrowWidth    = 0;

    if (sess_cb->pss_procparmlist != (QEF_DATA *) NULL)
    {
	DB_PROCEDURE_PARAMETER	*param_tuple;
	QEF_DATA		*listptr;

	/* compute count and total width of input and result parameters */
	for (listptr = sess_cb->pss_procparmlist;
	     listptr != (QEF_DATA *) NULL;
	     listptr = listptr->dt_next)
	{
	    param_tuple = (DB_PROCEDURE_PARAMETER *)listptr->dt_data;
	    if (param_tuple->dbpp_flags & DBPP_RESULT_COL)
	    {
		proctuple.db_rescolCount++;
		proctuple.db_resrowWidth = param_tuple->dbpp_offset 
			+ param_tuple->dbpp_length;
	    }
	    else
	    {
		proctuple.db_parameterCount++;
		proctuple.db_recordWidth = param_tuple->dbpp_offset 
			+ param_tuple->dbpp_length;
	    }
	}
	if (proctuple.db_rescolCount > 0)
	    proctuple.db_resrowWidth -= proctuple.db_recordWidth;
    }

    /* Initialize the RDF request block. */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    (VOID) MEcopy((PTR)&psy_cb->psy_tabname[0], sizeof(DB_DBP_NAME),
	(PTR)&rdf_rb->rdr_name.rdr_prcname);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_rb->rdr_owner);
    rdf_rb->rdr_types_mask = RDR_PROCEDURE;
    rdf_rb->rdr_update_op = RDR_APPEND;
    rdf_rb->rdr_qrytuple = (PTR) &proctuple; 
    rdf_rb->rdr_l_querytext = textlen;
    rdf_rb->rdr_querytext = ((char *) qsf_rb.qsf_root) + sizeof(i4);

    /*
    ** pass information about objects/privileges on which the new database
    ** procedure depends to QEF
    */
    sess_cb->pss_indep_objs.psq_grantee = &sess_cb->pss_user;
    rdf_rb->rdr_indep = (PTR) &sess_cb->pss_indep_objs;

    /* fill in the description of the procedure's parameters
    ** (which RDF/QEF will store into iiprocedure_parameter);
    */
    rdf_rb->rdr_proc_param_cnt = proctuple.db_parameterCount
					+ proctuple.db_rescolCount;
    rdf_rb->rdr_proc_params    = sess_cb->pss_procparmlist;

    /* Create a new procedure in the system catalogs */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);

    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD0137_DUPLICATE_PROCS)
	{
	    /* Retry */
	    psy_cb->psy_error.err_code = E_PS0008_RETRY;
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		&psy_cb->psy_error);
	}

	goto exit;
    }

    /*
    ** if the independent object/privilege list is non-empty, verify that the
    ** dbproc we are about to create is not abandoned; strictly speaking, as
    ** long as the independent object list is empty, we are guaranteed that the
    ** dbproc is not abandoned (user posesses required privileges), but we will
    ** take an extra step and try to determine whether it is grantable
    */
    if (!empty_indep_list)
    {
	MEcopy((PTR) psy_cb->psy_tabname, sizeof(DB_DBP_NAME),
	    (PTR) &dbp_descr.psy_tabnm);
	dbp_descr.psy_tabid.db_tab_base = proctuple.db_procid.db_tab_base;
	dbp_descr.psy_tabid.db_tab_index = proctuple.db_procid.db_tab_index;
	
	status = psy_dbp_status(&dbp_descr, sess_cb, (PSF_QUEUE *) NULL,
	    (i4) PSQ_CREDBP, dbp_mask, &psy_cb->psy_error);
	if (DB_FAILURE_MACRO(status))
	{
	    goto exit;
	}
    }


exit:

    /*
    ** close the memory stream which was used to allocate descriptors of
    ** objects/privileges on which the new dbproc depends
    */
    stat = psf_mclose(sess_cb, sess_cb->pss_dependencies_stream, &psy_cb->psy_error);
    if (DB_FAILURE_MACRO(stat) && stat > status)
	status = stat;

    /*
    ** ensure that no one tries to use the stream that is no longer valid
    */
    sess_cb->pss_dependencies_stream = (PSF_MSTREAM *) NULL;

    /* Get a lock on the query text from QSF */
    qsf_rb.qsf_lk_state = QSO_EXLOCK;

    stat = qsf_call(QSO_LOCK, &qsf_rb);

    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_error(E_PS0A08_CANTLOCK, qsf_rb.qsf_error.err_code,
	    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	if (stat > status)
	    status = stat;
    }
    else
    {
	/* Now destroy the query text */
	stat = qsf_call(QSO_DESTROY, &qsf_rb);

	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_error(E_PS0A09_CANTDESTROY, qsf_rb.qsf_error.err_code,
		PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    if (stat > status)
		status = stat;
	}
    }


    /* Destroy the procedure QP in QSF if things went wrong. */
    if (DB_FAILURE_MACRO(status))
    {
	PSS_DBPALIAS	dbpid;
	DB_CURSOR_ID	dbp_curs_id;

	qsf_rb.qsf_feobj_id.qso_type = QSO_ALIAS_OBJ;
	qsf_rb.qsf_feobj_id.qso_lname = sizeof(dbpid);

	/* Identify the object first */
	dbp_curs_id.db_cursor_id[0] = dbp_curs_id.db_cursor_id[1] = 0;
	(VOID) MEcopy((PTR)&psy_cb->psy_tabname[0], DB_TAB_MAXNAME,
	    (PTR)dbp_curs_id.db_cur_name);
	MEcopy((PTR) &dbp_curs_id, sizeof(DB_CURSOR_ID), (PTR) dbpid);

	(VOID) MEcopy((PTR) &sess_cb->pss_user, DB_OWN_MAXNAME,
	    (PTR) (dbpid + sizeof(DB_CURSOR_ID)));

	I4ASSIGN_MACRO(sess_cb->pss_udbid,
		       *(i4 *) (dbpid + sizeof(DB_CURSOR_ID) + DB_OWN_MAXNAME));

	(VOID)MEcopy((PTR) dbpid, sizeof(dbpid),
	    (PTR) qsf_rb.qsf_feobj_id.qso_name);

	/* See if QP for the alias already exists. */
	qsf_rb.qsf_lk_state = QSO_SHLOCK;
	stat = qsf_call(QSO_JUST_TRANS, &qsf_rb);

	if (DB_FAILURE_MACRO(stat))
	{
	    if (qsf_rb.qsf_error.err_code != E_QS0019_UNKNOWN_OBJ)
	    {
		(VOID) psf_error(E_PS037A_QSF_TRANS_ERR,
		    qsf_rb.qsf_error.err_code, PSF_INTERR,
		    &err_code, &psy_cb->psy_error, 0);
		if (stat > status)
		    status = stat;
		goto exit1;
	    }
	    else
	    {
		/* QP disappeared, which is alright. */
		goto exit1;
	    }
	}

	/* Now destroy the QP object in QSF */
	stat = qsf_call(QSO_DESTROY, &qsf_rb);

	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_error(E_PS0A09_CANTDESTROY, qsf_rb.qsf_error.err_code,
		    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    if (stat > status)
		status = stat;
	}
    }

exit1:
    return (status);
}

/*{
** Name: psy_kproc - Drop a database procedure definition from
**		     the system catalogs.
**
** Description:
**
** Inputs:
**
** Outputs:

**	Exceptions:
**	    none
**
** Side Effects:
**	    Modifies system catalogs.
**
** History:
**      27-apr-88 (stec)
**	    Created.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	12-jun-90 (andre)
**	    when trying to destroy a dbproc QEP, use "private" alias, which is
**	    always defined as opposed to the "public" alias which would not be
**	    defined if the dbproc is not grantable.
**	06-aug-92 (teresa)
**	    change interface to invalidate procedure by name, owner.
**	    must set rdr_flags_mask = RDR_PROCEDURE when invalidating the
**	    procedure cache object.
**	07-nov-92 (andre)
**	    we will no longer create private aliases for dbproc QPs
**	16-jun-94 (andre)
**	    it is dangerous to cast a (char *) into a (DB_CURSOR_ID *) and then
**	    dereference the resulting pointer because the chat ptr may not be 
**	    pointing at memory not aligned on an appopriate boundary.  Instead,
**	    we will allocate a DB_CURSOR_ID structure on the stack, initialize 
**	    it and MEcopy() it into the char array
*/
DB_STATUS
psy_kproc(
	PSY_CB		*psy_cb,
	PSS_SESBLK	*sess_cb)
{
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status, stat;
    i4		err_code;

    pst_rdfcb_init(&rdf_cb, sess_cb);
    (VOID) MEcopy((PTR)&psy_cb->psy_tabname[0], sizeof(DB_DBP_NAME),
	(PTR)&rdf_rb->rdr_name.rdr_prcname);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_rb->rdr_owner);
    rdf_rb->rdr_types_mask = RDR_PROCEDURE;
    rdf_rb->rdr_update_op = RDR_DELETE;

    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);

    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
	{
	    /* Retry */
	    psy_cb->psy_error.err_code = E_PS0008_RETRY;
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		&psy_cb->psy_error);
	    return (status);
	}
    }

    /* Try to destroy the procedure QP in QSF if things went OK. */
    if (DB_SUCCESS_MACRO(status))
    {
    	QSF_RCB		qsf_rb;
	PSS_DBPALIAS	dbpid;
	DB_CURSOR_ID	dbp_curs_id;

	/* Initialize the header of the QSF control block */
	qsf_rb.qsf_type = QSFRB_CB;
	qsf_rb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rb.qsf_length = sizeof(qsf_rb);
	qsf_rb.qsf_owner = (PTR)DB_PSF_ID;
	qsf_rb.qsf_sid = sess_cb->pss_sessid;

	qsf_rb.qsf_feobj_id.qso_type = QSO_ALIAS_OBJ;
	qsf_rb.qsf_feobj_id.qso_lname = sizeof(dbpid);

	/* Identify the object first */
	dbp_curs_id.db_cursor_id[0] = dbp_curs_id.db_cursor_id[1] = 0;
	(VOID) MEcopy((PTR)&psy_cb->psy_tabname[0], DB_TAB_MAXNAME,
	    (PTR)dbp_curs_id.db_cur_name);
	MEcopy((PTR) &dbp_curs_id, sizeof(DB_CURSOR_ID), (PTR) dbpid);

	(VOID) MEcopy((PTR) &sess_cb->pss_user, DB_OWN_MAXNAME,
	    (PTR) (dbpid + sizeof(DB_CURSOR_ID)));

	I4ASSIGN_MACRO(sess_cb->pss_udbid,
		       *(i4 *) (dbpid + sizeof(DB_CURSOR_ID) + DB_OWN_MAXNAME));

	(VOID)MEcopy((PTR) dbpid, sizeof(dbpid),
	    (PTR) qsf_rb.qsf_feobj_id.qso_name);

	/* See if QEP for the alias already exists. */
	qsf_rb.qsf_lk_state = QSO_SHLOCK;
	stat = qsf_call(QSO_JUST_TRANS, &qsf_rb);

	if (DB_FAILURE_MACRO(stat))
	{
	    if (qsf_rb.qsf_error.err_code != E_QS0019_UNKNOWN_OBJ)
	    {
		(VOID) psf_error(E_PS037A_QSF_TRANS_ERR,
		    qsf_rb.qsf_error.err_code, PSF_INTERR,
		    &err_code, &psy_cb->psy_error, 0);
		if (stat > status)
		    status = stat;
		goto exit;
	    }
	    else
	    {
		/* Nothing to destroy, QP not in QSF */
		goto exit;
	    }
	}

	/* Now destroy the ALIAS and the QP objects in QSF */
	stat = qsf_call(QSO_DESTROY, &qsf_rb);

	if (DB_FAILURE_MACRO(stat))
	{
	    (VOID) psf_error(E_PS0A09_CANTDESTROY, qsf_rb.qsf_error.err_code,
		    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    if (stat > status)
		status = stat;
	}
    }

    /* Invalidate procedure object from RDF cache */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    (VOID) MEcopy((PTR)&psy_cb->psy_tabname[0], sizeof(DB_DBP_NAME),
	(PTR)&rdf_rb->rdr_name.rdr_prcname);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_rb->rdr_owner);
    rdf_rb->rdr_types_mask = RDR_PROCEDURE;
    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
			&psy_cb->psy_error);
    }

exit:
    return (status);
}

/*{
** Name: psy_gproc - Get a database procedure definition from
**		     the system catalogs.
**
** Description:
**
** Inputs:
**	    psq_cb	    
**		psq_cursid
**		    db_cur_name	    dbproc name
**	    sess_cb
**		pss_user	    current user name
**		pss_dba		    dba name
**	    qsf_rb	    
**	    rdf_cb
**	    dbp_owner		    name of the owner whose dbproc will be
**				    looked up iff gproc_mask & PSS_DBP_BY_OWNER.
**	    gproc_mask		    mask used to specify the possible owners of
**				    the dbproc to look for
**		PSS_USRDBP	    look for dbproc owned by the current user
**		PSS_DBADBP	    look for dbproc owned by the DBA
**		PSS_INGDBP	    look for dbproc owned by $INGRES
**		PSS_DBP_BY_OWNER    look for dbproc owned by the specific user
**
**				    NOTE: if PSS_DBP_BY_OWNER is set,
**				          PSS_USRDBP, PSS_DBADBP, and PSS_INGDBP
**					  will be disregarded
**					  otherwise, we expect that PSS_USRDBP
**					  will be always set, while PSS_DBADBP
**					  and PSS_INGDBP may or may not be set
**
** Outputs:
**	    alt_user		    set to point the name of dbproc owner if
**				    different from cb->pss_user
**	    psq_cb
**		psq_error	    filled in if an error occurred
**
**	    ret_flags		    bits may be set to pass info to the caller
**		PSS_MISSING_DBPROC  dbproc not found
**
** Exceptions:
**	    none
**
** Returns
**	E_DB_OK, E_DB_ERROR
**
** Side Effects:
**	    Allocates memory.
**
** History:
**      27-apr-88 (stec)
**	    Created.
**	04-aug-88 (stec)
**	    Improve recovery of resources.
**	17-aug-88 (stec)
**	    Change bad STRUCT_ASSIGN statements.
**	28-sep-88 (stec)
**	    Do not lock object on cleanup.
**	28-sep-88 (stec)
**	    Must not unfix RDF entry.
**	16-mar-89 (andre)
**	    Added a new parameter - alt_user.
**	    psy_gproc will no longer reset pss_user or set pss_ruset.  Instead
**	    it may set the ptr passed to it to the name of the dbproc owner.
**	16-mar-89 (neil)
**	    Modified psy_gproc to access the procedure through the specified
**	    owner (psq_als_owner) if psq_alias_set is on.
**	27-apr-89 (andre)
**	    Further modify to search for dbprocs owned by $ingres if the search
**	    by user and DBA failed.  The search part of the function has been,
**	    essentially, rewritten.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	01-jun-90 (andre)
**	    Changed interface to allow caller to explicitly specify the possible
**	    owners of the dbproc to look for.
**	01-oct-91 (andre)
**	    Added ret_flags to the interface - this field will be used to pass
**	    additional info to the caller.  in particular, it will allow us to
**	    signal certain classes of errors (e.g. dbproc not found) without
**	    setting return status to E_DB_ERROR, thus enabling the caller to
**	    distinguish such errors from unexpected errors
**	15-oct-1993 (rog)
**	    We need to use ulm_copy() instead of MEcopy() when copying query
**	    text that might be more than 64k.
**	12-jan-94 (andre)
**	    As a part of fix for bug 58048, we must remove the assumption that 
**	    either PSS_USRDBP or PSS_DBP_BY_OWNER bit will always be set in 
**	    gproc_mask.  Now we will be prepared to handle PSS_DBP_BY_OWNER or
**	    one or more of PSS_USRDBP, PSS_DBADBP, and PSS_INGDBP.
**	13-jan-94 (andre)
**	    if we fail to find a dbproc, we will set PSS_MISSING_DBPROC in 
**	    *ret_flags, but leave it up to the caller to issue an error 
**	    message - this way the caller can decide whether this warrants an 
**	    error message and if so can choose his favourite error message
*/
DB_STATUS
psy_gproc(
	PSQ_CB		*psq_cb,
	PSS_SESBLK	*sess_cb,
	QSF_RCB		*qsf_rb,
	RDF_CB		*rdf_cb,
	DB_OWN_NAME	**alt_user,
	DB_OWN_NAME	*dbp_owner,
	i4		gproc_mask,
	i4		*ret_flags)
{
    DB_STATUS		status, stat;
    i4		err_code;
    RDD_QRYMOD		*pinfo;
    PSQ_QDESC		*qdesc;
    bool		leave_loop = TRUE;
    DB_PROCEDURE *dbp;
    SXF_ACCESS  access;
    i4	msgid;
    i4	local_status;

    *ret_flags = 0;

    /* First call RDF to retrieve the definition
    ** of the procedure.
    */

    /* Initialize the RDF control block */
    pst_rdfcb_init(rdf_cb, sess_cb);

    (VOID) MEcopy((PTR) psq_cb->psq_cursid.db_cur_name,
	sizeof (DB_DBP_NAME),
	(PTR) &rdf_cb->rdf_rb.rdr_name.rdr_prcname);
    rdf_cb->rdf_rb.rdr_types_mask = RDR_PROCEDURE | RDR_BY_NAME;

    /* assume that the dbproc is owned by the current user */
    *alt_user = (DB_OWN_NAME *) NULL;
    
    do
    {
	if (gproc_mask & (PSS_USRDBP | PSS_DBP_BY_OWNER))
	{
	    if (gproc_mask & PSS_USRDBP)
	    {
		STRUCT_ASSIGN_MACRO(sess_cb->pss_user, 
		    rdf_cb->rdf_rb.rdr_owner);
	    }
	    else
	    {
		STRUCT_ASSIGN_MACRO((*dbp_owner), rdf_cb->rdf_rb.rdr_owner);
		if (MEcmp((PTR)&sess_cb->pss_user, (PTR) dbp_owner,
			  sizeof(DB_OWN_NAME)))
		{
		    *alt_user = dbp_owner;		/* Try someone else */
		}
	    }

	    /* Get the text */
	    status = rdf_call(RDF_GETINFO, (PTR) rdf_cb);

	    /*
	    ** We do not want to continue search if:
	    ** 1) dbproc was found					    OR
	    ** 2) we got an error other that PROC_NOT_FOUND		    OR
	    ** 3) caller requested a dbproc owned by a specific user
	    **    (gproc_mask & PSS_DBP_BY_OWNER)			    OR
	    */
	    if (   DB_SUCCESS_MACRO(status)				    
		|| rdf_cb->rdf_error.err_code != E_RD0201_PROC_NOT_FOUND   
		|| gproc_mask & PSS_DBP_BY_OWNER)
	    {
	        break;
	    }
	}

	/* 
	** if we were told to check whether a dbproc is owned by the DBA, do so
	** unless the DBA is the current user and we have already established 
	** that the current user does not own a dbproc with this name
	*/
	if (   gproc_mask & PSS_DBADBP 
	    && (   ~gproc_mask & PSS_USRDBP
		|| MEcmp((PTR) &sess_cb->pss_user, 
		       (PTR) &sess_cb->pss_dba.db_tab_own, sizeof(DB_OWN_NAME))
	       )
	   )
	{
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_dba.db_tab_own,
				rdf_cb->rdf_rb.rdr_owner);

	    /*
	    ** If we succeed and the DBA is different from the current user, 
	    ** the procedure text will have to be parsed in the context of 
	    ** dbproc's owner, i.e. DBA.  Note that if we were also asked to 
	    ** check whether the dbproc is owned by the current user and still 
	    ** found ourselves here, DBA must be different from the current user
	    */
	    if (   gproc_mask & PSS_USRDBP
		|| MEcmp((PTR) &sess_cb->pss_user,
		       (PTR) &sess_cb->pss_dba.db_tab_own, sizeof(DB_OWN_NAME)))
	    {
	        *alt_user = &sess_cb->pss_dba.db_tab_own;
	    }

	    /* Get the text */
	    status = rdf_call(RDF_GETINFO, (PTR) rdf_cb);

	    /*
	    ** We do not want to continue search if:
	    ** 1) dbproc was found				OR
	    ** 2) we got an error other that PROC_NOT_FOUND
	    */
	    if (DB_SUCCESS_MACRO(status) ||
	        rdf_cb->rdf_error.err_code != E_RD0201_PROC_NOT_FOUND)
	    {
		break;
	    }
	}

	if (gproc_mask & PSS_INGDBP)
	{
	    MEmove(sizeof(*sess_cb->pss_cat_owner), (PTR)sess_cb->pss_cat_owner,
		   ' ', DB_OWN_MAXNAME, (PTR) &rdf_cb->rdf_rb.rdr_owner); 
	}
	else
	{
	    /*
	    ** if user has not requested that a dbproc owned by $INGRES be
	    ** looked up, might as well get out of the loop
	    */
	    break;
	}
	
	/* 
	** if we were told to check whether a dbproc is owned by $ingres, do so
	** unless the $ingres is the current user and we have already 
	** established that the current user does not own a dbproc with this 
	** name or $ingres is the DBA and we have already established that the
	** DBA does not own a dbproc with this name
	*/
	if (   (   ~gproc_mask & PSS_USRDBP
	        || MEcmp((PTR)&sess_cb->pss_user, 
		       (PTR) &rdf_cb->rdf_rb.rdr_owner, sizeof(DB_OWN_NAME))
	       )
	    && (   ~gproc_mask & PSS_DBADBP
		|| MEcmp((PTR) &sess_cb->pss_dba.db_tab_own,
	               (PTR) &rdf_cb->rdf_rb.rdr_owner, sizeof(DB_OWN_NAME))
	       )
	   )
	{
	    /*
	    ** If we succeed, the procedure text will have to be parsed in
	    ** the context of the original owner, i.e. $ingres.
	    */
	    *alt_user = &rdf_cb->rdf_rb.rdr_owner;
	    /*
	    ** If we succeed and $ingres is not the current user, the procedure
	    ** text will have to be parsed in the context of dbproc's owner, 
	    ** i.e. $ingres.  Note that if we were also asked to check whether 
	    ** the dbproc is owned by the current user and still found ourselves
	    ** here, the current user must NOT be $ingres
	    */
	    if (   gproc_mask & PSS_USRDBP
		|| MEcmp((PTR) &sess_cb->pss_user,
		       (PTR) &rdf_cb->rdf_rb.rdr_owner, sizeof(DB_OWN_NAME)))
	    {
	        *alt_user = &sess_cb->pss_dba.db_tab_own;
	    }


	    /* Get the text */
	    status = rdf_call(RDF_GETINFO, (PTR) rdf_cb);
	}
	
	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    if (DB_FAILURE_MACRO(status))
    {
	if (rdf_cb->rdf_error.err_code == E_RD0201_PROC_NOT_FOUND)
	{
	    /*
	    ** dbproc was not found - set a bit in ret_flags and reset status to
	    ** E_DB_OK to enable callers to distinguish between this and other
	    ** errors
	    */
	    status = E_DB_OK;
	    *ret_flags |= PSS_MISSING_DBPROC;
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_GETINFO, &rdf_cb->rdf_error,
				 &psq_cb->psq_error);
	}
	return(status);
    }

    /* we get here only if the dbproc was found */
    
    /*
    ** If the procedure has a security label, validate it
    */
    dbp=rdf_cb->rdf_info_blk->rdr_dbp;

    /* Initialize the QSF control block */
    qsf_rb->qsf_type = QSFRB_CB;
    qsf_rb->qsf_ascii_id = QSFRB_ASCII_ID;
    qsf_rb->qsf_length = sizeof(QSF_RCB);
    qsf_rb->qsf_owner = (PTR)DB_PSF_ID;
    qsf_rb->qsf_sid = sess_cb->pss_sessid;
    qsf_rb->qsf_obj_id.qso_type = QSO_QTEXT_OBJ;
    qsf_rb->qsf_obj_id.qso_lname = 0;

    /* Having retrieved the text place it in QSF
    ** because the parser expects it to be there.
    */
    status = qsf_call(QSO_CREATE, qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
	i4	    qerr = qsf_rb->qsf_error.err_code;

	if (qerr == E_QS0001_NOMEM)
	{
	    (VOID) psf_error(qerr, qerr,
		PSF_CALLERR, &err_code, &psq_cb->psq_error, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0A05_BADMEMREQ, qerr,
		PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	}

	goto cleanup2;
    }

    /* Object is locked exclusively now. */

    pinfo = rdf_cb->rdf_rb.rdr_procedure;

    /* Allocate enough memory for the query descriptor
    ** plus the length of text.
    */
    qsf_rb->qsf_sz_piece = sizeof(PSQ_QDESC) +
	pinfo->rdf_l_querytext + 3; /* one space, one null,
				    ** one for safety.
				    */

    status = qsf_call(QSO_PALLOC, qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
	i4	    qerr = qsf_rb->qsf_error.err_code;

	if (qerr == E_QS0001_NOMEM)
	{
	    (VOID) psf_error(qerr, qerr,
		PSF_CALLERR, &err_code, &psq_cb->psq_error, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0A05_BADMEMREQ, qerr,
		PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	}

	goto cleanup1;
    }

    qdesc = (PSQ_QDESC *) qsf_rb->qsf_piece;

    /* Initialize query descriptor. */
    qdesc->psq_qrysize = pinfo->rdf_l_querytext + 1; /* 1 trailing space */
    qdesc->psq_datasize = 0;
    qdesc->psq_dnum = 0;
    qdesc->psq_qrytext = (char *)(qdesc + 1); /* Ptr arithmetic, should point
				    ** right after the PSQ_QDESC.
				    */
    qdesc->psq_qrydata = (DB_DATA_VALUE **) NULL;

    /* QSF memory has been allocated now, copy
    ** the text from RDF.
    */
    ulm_copy((PTR) pinfo->rdf_querytext,
	     (i4) pinfo->rdf_l_querytext,
	     (PTR) qdesc->psq_qrytext);

    /* Add a space after the text and null terminate. */
    {
	char	*p;

	p = qdesc->psq_qrytext;		/* beginning of text */
	p += pinfo->rdf_l_querytext;	/* 1st char past end */
	*p++ = ' ';
	*p = '\0';			/* 2nd char past end */
    }

    /* Set root for the QSF object */
    qsf_rb->qsf_root = qsf_rb->qsf_piece;

    status = qsf_call(QSO_SETROOT, qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(E_PS0A05_BADMEMREQ, qsf_rb->qsf_error.err_code,
		PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	goto cleanup1;
    }

    status = qsf_call(QSO_UNLOCK, qsf_rb);

    if (DB_FAILURE_MACRO(status))
    {
	(VOID) psf_error(E_PS0B05_CANT_UNLOCK, qsf_rb->qsf_error.err_code,
		PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	goto cleanup1;
    }

    psq_cb->psq_qid = qsf_rb->qsf_obj_id.qso_handle;

    return (status);

cleanup1:
    /* Destroy the object, it's already locked. */
    stat = qsf_call(QSO_DESTROY, qsf_rb);

    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_error(E_PS0A09_CANTDESTROY, qsf_rb->qsf_error.err_code,
		PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	if (stat > status)
	    status = stat;
    }

cleanup2:
    /* RDF object can be released now. */

    stat = rdf_call(RDF_UNFIX, (PTR) rdf_cb);

    if (DB_FAILURE_MACRO(stat))
    {
	(VOID) psf_rdf_error(RDF_UNFIX,
	    &rdf_cb->rdf_error, &psq_cb->psq_error);
	if (stat > status)
	    status = stat;
    }

    return (status);
}
