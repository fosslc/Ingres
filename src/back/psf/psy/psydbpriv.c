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
#include    <dmrcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <scf.h>
#include    <dudbms.h>

/**
**
**  Name: PSYDBPRIV.C - Functions used to manipulate the iidbpriv catalog.
**
**  Description:
**      This file contains functions necessary to grant, revoke,
**	and check database privileges:
**
**          psy_dbpriv	- Grants and revokes database privileges
**          psy_ckdbpr	- Checks database privilege masks
**
**  History:
**	10-apr-89 (ralph)
**	    written
**	24-may-89 (ralph)
**	    added psy_ckdbpr
**	06-jun-89 (ralph)
**	    corrected unix portability problems
**	24-jun-89 (ralph)
**	    corrected internal error message
**	06-sep-89 (ralph)
**	    added support for GRANT/REVOKE ON ALL DATABASES
**	    removed DBGR_DEFAULT
**	03-aug-92 (barbara)
**	    Call pst_rdfcb_init to initalize RDF_CB before calling rdf_call.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	22-oct-93  (robf)
**          Add connect_limit/idle_limit/priority_limit handling
[@history_template@]...
**/

/*{
** Name: psy_dbpriv - GRANT/REVOKE database privileges
**
**  INTERNAL PSF call format: status = psy_dbpriv(&psy_cb, sess_cb);
**  EXTERNAL     call format: status = psy_call (PSY_DBPRIV,&psy_cb, sess_cb);
**
** Description:
**	This procedure is sued to grant and revoke database privileges.
**	A template iidbpriv tuple is built and shipped to RDF.
**	Eventually, the tuple is added/merged into iidbpriv.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_usrq			list of grantees
**	    .psy_tblq			list of objects
**	    .psy_gtype			grantee type
**	    .psy_grant			statement type (GRANT or REVOKE)
**	    .psy_fl_dbpriv		control flags
**	    .psy_qdio_limit		query I/O  limit
**	    .psy_qrow_limit		query row  limit
**	    .psy_qcpu_limit		query CPU  limit
**	    .psy_qpage_limit		query page limit
**	    .psy_qcost_limit		query cost limit
**	    .psy_idle_limit		idle time  limit
**	    .psy_connect_limit		connect time  limit
**	    .psy_priority_limit		priority   limit
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	10-apr-89 (ralph)
**          written
**	06-jun-89 (ralph)
**	    corrected unix portability problems
**	24-jun-89 (ralph)
**	    corrected internal error message
**	06-sep-89 (ralph)
**	    added support for GRANT/REVOKE ON ALL DATABASES
**	    removed DBGR_DEFAULT
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	19-jul-92 (andre)
**	    changes were made to enable a user to specify privileges to PUBLIC
**	    along with one or more user authorization identifiers
**	10-aug-93 (andre)
**	    fixed cause of a compiler warning
*/
DB_STATUS
psy_dbpriv(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		stat;
    i4		err_code;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_PRIVILEGES	dbprtuple;
    register DB_PRIVILEGES *dbprtup = &dbprtuple;
    PSY_USR		*psy_usr;
    PSY_TBL		*psy_tbl;
    bool		grant_to_public;
    bool		leave_loop = TRUE;
    
    /* This code is called for SQL only */

    do
    {
	/* Fill in the RDF request block */

	pst_rdfcb_init(&rdf_cb, sess_cb);
	if (psy_cb->psy_grant == PSY_DGRANT)
	    rdf_rb->rdr_update_op   = RDR_APPEND;
	else if (psy_cb->psy_grant == PSY_DREVOKE)
	    rdf_rb->rdr_update_op   = RDR_DELETE;
	else
	{
	    /* Invalid operation specified in psy_grant */
	    err_code = E_PS0D45_INVALID_GRANT_OP;
	    (VOID) psf_error(E_PS0D45_INVALID_GRANT_OP, 0L,
			     PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    status   = E_DB_ERROR;
	    break;
	}
	rdf_rb->rdr_status	    = DB_SQL;
	rdf_rb->rdr_types_mask  = RDR_DBPRIV;
	rdf_rb->rdr_qrytuple    = (PTR) dbprtup;
	rdf_rb->rdr_qtuple_count = 1;

	/* Initialize the template iidbpriv tuple */

	dbprtup->dbpr_gtype	  = psy_cb->psy_gtype;
	dbprtup->dbpr_control	  = psy_cb->psy_ctl_dbpriv;
	dbprtup->dbpr_dbflags	  = 0;
	dbprtup->dbpr_flags	  = psy_cb->psy_fl_dbpriv;
	dbprtup->dbpr_qrow_limit  = psy_cb->psy_qrow_limit;
	dbprtup->dbpr_qdio_limit  = psy_cb->psy_qdio_limit;
	dbprtup->dbpr_qcpu_limit  = psy_cb->psy_qcpu_limit;
	dbprtup->dbpr_qpage_limit = psy_cb->psy_qpage_limit;
	dbprtup->dbpr_qcost_limit = psy_cb->psy_qcost_limit;
	dbprtup->dbpr_connect_time_limit = psy_cb->psy_connect_limit;
	dbprtup->dbpr_idle_time_limit    = psy_cb->psy_idle_limit;
	dbprtup->dbpr_priority_limit     = psy_cb->psy_priority_limit;

	MEfill(sizeof(dbprtup->dbpr_database), (u_char) ' ',
	    (PTR) &dbprtup->dbpr_database);

	MEfill(sizeof(dbprtup->dbpr_reserve), (u_char) ' ',
	    (PTR) dbprtup->dbpr_reserve);

	/* Point to first database object, if any */

	psy_tbl = (PSY_TBL *)psy_cb->psy_tblq.q_next;

	/* No database objects means GRANT/REVOKE ON ALL DATABASES */

	if (psy_tbl == (PSY_TBL *) &psy_cb->psy_tblq)
	    dbprtup->dbpr_dbflags |= DBPR_ALLDBS;

	/*
	** Pass the template tuples to RDF for each database/grantee.
	** RDF/QEF will return E_DB_INFO if a database/grantee is rejected,
	** or E_DB_WARN is a database is rejected.  Anything worse than
	** E_DB_WARN is a fatal error.
	*/

	/* Process each database/grantee */
	for (;;)
	{
	    /*
	    ** Copy in database name if one was provided.
	    ** None will be provided if ON ALL DATABASES was specified.
	    */
	    if (psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq)
		MEcopy((PTR)&psy_tbl->psy_tabnm,
		       sizeof(dbprtup->dbpr_database),
		       (PTR)&dbprtup->dbpr_database);

	    grant_to_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;
	    psy_usr = (PSY_USR *)  psy_cb->psy_usrq.q_next;

	    do
	    {
		if (grant_to_public)
		{
		    /*
		    ** user specified TO/FROM PUBLIC, possibly along with other
		    ** user auth ids; we will process TO/FROM PUBLIC first
		    */
		    dbprtup->dbpr_gtype = DBGR_PUBLIC;
		    
		    MEmove(sizeof("public") - 1, (PTR) "public", ' ',
			sizeof(dbprtup->dbpr_gname),
			(PTR) &dbprtup->dbpr_gname);
		}
		else
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm,
			dbprtup->dbpr_gname);
		}
		
		stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		status = (stat > status) ? stat : status;
		if (stat > E_DB_INFO)
		    break;

		if (grant_to_public)
		{
		    dbprtup->dbpr_gtype = psy_cb->psy_gtype;

		    /*
		    ** remember that we have processed GRANT TO PUBLIC on this
		    ** database or on the installation
		    */
		    grant_to_public = FALSE;
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);

	    /* Break out if failure */
	    if (DB_FAILURE_MACRO(status))
		break;

	    /* Point to next database object */
	    psy_tbl = (PSY_TBL *)  psy_tbl->queue.q_next;

	    /* Break out if no more objects */
	    if (psy_tbl == (PSY_TBL *) &psy_cb->psy_tblq)
		break;
	}

	/* We are done */

	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			     &psy_cb->psy_error);

    return    (status);
}

/*{
** Name: psy_ckdbpr - Check database privileges for specific values
**
**  INTERNAL PSF call format: status = psy_ckdbpr(dbprmask)
**
** Description:
**	This procedure checks the session's database privileges mask
**	to ensure the specified privileges are allowed.
**
** Inputs:
**	psq_cb				
**	    .psq_error			error code
**	dbprmask			privileges required
**
** Outputs:
**	None.
**	Returns:
**	    E_DB_OK			the session has the required privileges.
**	    E_DB_ERROR			the session lacks one or more of the
**					required privileges.
**	    E_DB_SEVERE			scu_information() failed
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	24-may-89 (ralph)
**          written
*/
DB_STATUS
psy_ckdbpr(
	PSQ_CB		    *psq_cb,
	u_i4	    dbprmask)
{
    u_i4		dbprivs;
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[1];

    /* This code is called for SQL only */

    /* Make sure user is authorized */

    scf_cb.scf_length	= sizeof (SCF_CB);
    scf_cb.scf_type	= SCF_CB_TYPE;
    scf_cb.scf_facility	= DB_PSF_ID;
    scf_cb.scf_session	= DB_NOSESSION;
    scf_cb.scf_ptr_union.scf_sci     = (SCI_LIST *) sci_list;
    scf_cb.scf_len_union.scf_ilength = 1;
    sci_list[0].sci_length  = sizeof(dbprivs);
    sci_list[0].sci_code    = SCI_DBPRIV;
    sci_list[0].sci_aresult = (char *) &dbprivs;
    sci_list[0].sci_rlength = NULL;

    if (scf_call(SCU_INFORMATION, &scf_cb) != E_DB_OK)
    {
	(VOID) psf_error(E_PS0D13_SCU_INFO_ERR, 0L, PSF_INTERR,
			 &scf_cb.scf_error.err_code, &psq_cb->psq_error, 0);
	return(E_DB_SEVERE);
    }
	
    if (dbprmask != (dbprmask & dbprivs))
	return(E_DB_ERROR);
    else
	return(E_DB_OK);

}
