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
**  Name: PSYRPRIV.C - Functions used to manipulate the iirolegrant catalog.
**
**  Description:
**      This file contains functions necessary to grant, revoke,
**	and check role privileges:
**
**          psy_rolepriv- Grants and revokes role privileges
**
**  History:
**	4-mar-94 (robf
**	    Created
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name: psy_rolepriv - GRANT/REVOKE role privileges
**
**  INTERNAL PSF call format: status = psy_rolepriv(&psy_cb, sess_cb);
**  EXTERNAL     call format: status = psy_call (PSY_RPRIV,&psy_cb, sess_cb);
**
** Description:
**	This procedure is used to grant and revoke role privileges.
**	A template iirolegrant tuple is built and shipped to RDF.
**	Eventually, the tuple is added/merged into iirolegrant.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_usrq			list of grantees
**	    .psy_objq			list of objects
**	    .psy_gtype			grantee type
**	    .psy_grant			statement type (GRANT or REVOKE)
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
**	4-mar-94 (robf)
**          Created
**	8-jun-94 (robf)
**          Correct typo in comments.
**	31-jun-94 (robf)
**	    Initialize grantee name for public to "public" for
**	    consistency with other grants
*/
DB_STATUS
psy_rolepriv(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		stat;
    i4		err_code;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_ROLEGRANT	rolegrtuple;
    register DB_ROLEGRANT *rolegrtup = &rolegrtuple;
    PSY_USR		*psy_usr;
    PSY_OBJ		*psy_obj;
    bool		grant_to_public;
    bool		leave_loop = TRUE;
    
    /* This code is called for SQL only */

    do
    {
	/* Fill in the RDF request block */

	pst_rdfcb_init(&rdf_cb, sess_cb);
	if (psy_cb->psy_grant == PSY_RGRANT)
	    rdf_rb->rdr_update_op   = RDR_APPEND;
	else if (psy_cb->psy_grant == PSY_RREVOKE)
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
	rdf_rb->rdr_2types_mask  = RDR2_ROLEGRANT;
	rdf_rb->rdr_qrytuple    = (PTR) rolegrtup;
	rdf_rb->rdr_qtuple_count = 1;

	/* Initialize the template iirolegrant tuple */

	rolegrtup->rgr_gtype	  = psy_cb->psy_gtype;
	rolegrtup->rgr_flags	  = 0;

	MEfill(sizeof(rolegrtup->rgr_reserve), (u_char) ' ',
	    (PTR) rolegrtup->rgr_reserve);

	/* Point to first role object, if any */

	psy_obj = (PSY_OBJ *)psy_cb->psy_objq.q_next;

	/*
	** Pass the template tuples to RDF for each role/grantee.
	** RDF/QEF will return E_DB_INFO if a role/grantee is rejected,
	** or E_DB_WARN is a role is rejected.  Anything worse than
	** E_DB_WARN is a fatal error.
	*/

	/* Process each role/grantee */
	for (;;)
	{
	    /*
	    ** Copy in role name if one was provided.
	    */
	    if (psy_obj != (PSY_OBJ *) &psy_cb->psy_objq)
		MEcopy((PTR)&psy_obj->psy_objnm,
		       sizeof(rolegrtup->rgr_rolename),
		       (PTR)&rolegrtup->rgr_rolename);

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
		    rolegrtup->rgr_gtype = DBGR_PUBLIC;
		    
		    MEmove(sizeof("public") - 1, (PTR) "public", ' ',
		    	sizeof(rolegrtup->rgr_grantee),
			(PTR) &rolegrtup->rgr_grantee);
		}
		else
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm,
			rolegrtup->rgr_grantee);
		}
		
		stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		status = (stat > status) ? stat : status;
		if (stat > E_DB_INFO)
		    break;

		if (grant_to_public)
		{
		    rolegrtup->rgr_gtype = psy_cb->psy_gtype;

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

	    /* Point to next role */
	    psy_obj = (PSY_OBJ *)  psy_obj->queue.q_next;

	    /* Break out if no more objects */
	    if (psy_obj == (PSY_OBJ *) &psy_cb->psy_objq)
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
