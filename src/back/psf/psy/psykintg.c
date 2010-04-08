/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
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
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSYKINTG.C - Functions used to destroy integrities.
**
**  Description:
**      This files contains the functions necessary to destroy integrities.
**
**          psy_kinteg - Destroy one or more integrities for a table.
**
**
**  History:    $Log-for RCS$
**      02-oct-85 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	30-nov-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	30-sep-93 (stephenb)
**	    Pass tablename to RDF in psy_kinteg().
[@history_template@]...
**/


/*{
** Name: psy_kinteg	- Destroy one or more integrities for a table.
**
**  INTERNAL PSF call format: status = psy_kinteg(&psy_cb, sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_KINTEG, &psy_cb, sess_cb);
**
** Description:
**      The psy_kinteg function removes the definitions of one or more 
**	integrities on a table from the system relations (integrities, 
**	tree, and iiqrytext).
**	Optionally, one can tell this function to destroy all of the 
**	integrities on the given table.
**
** Inputs:
**      psy_cb
**          .psy_tables[0]              Id of table from which to remove
**					integrities
**          .psy_numbs[]                Array of integrity id numbers telling
**					which integrities to destroy (at most
**					20)
**          .psy_numnms                 Number telling how many integrity
**					numbers there are.  Zero means to
**					destroy all of the integrities 
**					on the given table.
**	    .psy_tabname[0]		Name of table from which to remove
**					integrities
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if an error happens
**		.err_code		    What the error was
**		    E_PS0000_OK			Success
**		    E_PS0001_USER_ERROR		User made a mistake
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removes query tree(s) representing predicates of integrities from
**	    tree relation, text of defining query (or queries) from iiqrytext
**	    relation, rows defining integrity (or integrities) from integrities
**	    relation.  If there are no more integrities on this table, will
**	    use DMF alter table function to indicate this.
**
** History:
**	02-oct-85 (jeff)
**          written
**	30-aug-88 (andre)
**	    check if the user specified integrity 0 to be deleted, 
**	    and if so, warn that it doesn't exist (this will prevent someone
**	    from inadvertently deleting all integrities by specifying that
**	    integrity 0 be deleted.)
**	30-aug-88 (andre)
**	    When looking at individual integrities, return E_DB_ERROR if any
**	    errors were found when processing individual integrities.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	03-aug-1992 (barbara)
**	    Call pst_rdfcb_init to initialize RDF_CB before calling RDF.
**	    Invalidate infoblk from RDF cache after dropping integrity.
**	07-aug-92 (teresa)
**	    RDF_INVALID must be called for the base object that the 
**	    integrity was defined on as well as for the integrity trees.
**	14-sep-92 (barbara)
**	    Set type field and tableid in the RDF cb used to invalidate
**	    the cached entries.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    removed declaration of rdf_call()
**	30-sep-93 (stephenb)
**	    Pass tablename from psy_cb to rdf_rb, so that we can use the
**	    information to audit in QEF.
*/
DB_STATUS
psy_kinteg(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    RDF_CB              rdf_cb;
    RDF_CB              rdf_inv_cb; 	/* For invalidation */
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status = E_DB_OK;
    i4		err_code;
    register i4	i;

    /* Fill in RDF control block */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    pst_rdfcb_init(&rdf_inv_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[0], rdf_rb->rdr_tabid);
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[0], rdf_inv_cb.rdf_rb.rdr_tabid);
    rdf_rb->rdr_types_mask = RDR_INTEGRITIES;
    rdf_inv_cb.rdf_rb.rdr_types_mask = RDR_INTEGRITIES;
    rdf_rb->rdr_update_op = RDR_DELETE;
    rdf_rb->rdr_name.rdr_tabname = psy_cb->psy_tabname[0];

    /* No integrity numbers means destroy all integrities. */
    if (psy_cb->psy_numnms == 0)
    {
	/* Zero integrity number means destroy all integs. */
	rdf_rb->rdr_qrymod_id = 0;
	status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
	    {
		(VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, 1,
		    psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
			(char *) &psy_cb->psy_tabname[0]),
		    &psy_cb->psy_tabname[0]);
	    }
	    else
	    {
		(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		    &psy_cb->psy_error);
	    }
	    return (status);
	}

	/* Invalidate table info from RDF cache */

	    /* first invalidate the object from the relation cache */
	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	if (DB_FAILURE_MACRO(status))
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				&psy_cb->psy_error);

	    /* now invalidate any integrity trees from the cache */
	rdf_inv_cb.rdf_rb.rdr_2types_mask |= RDR2_CLASS;
	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	if (DB_FAILURE_MACRO(status))
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				&psy_cb->psy_error);
    }
    else
    {
	DB_STATUS   stat;
	
	/* Run through the integrity numbers, destroying each one */
	for (i = 0; i < psy_cb->psy_numnms; i++)
	{

	    /* if user specified 0, complain and proceed with next integrity # */
	    if ((rdf_rb->rdr_qrymod_id = psy_cb->psy_numbs[i]) == 0)
	    {
		/* remember error to report later */
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;

		(VOID) psf_error(5203, 0L, PSF_USERERR, &err_code, 
				 &psy_cb->psy_error, 1, 
				 sizeof (rdf_rb->rdr_qrymod_id),
				 &rdf_rb->rdr_qrymod_id);
		continue;
	    }

	    stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);

	    /* remember the highest status seen so far */
	    status = (status > stat) ? status : stat;

	    if (DB_FAILURE_MACRO(stat))
	    {
		switch (rdf_cb.rdf_error.err_code)
		{
		case E_RD0002_UNKNOWN_TBL:
		    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
			&err_code, &psy_cb->psy_error, 1,
			psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
			    (char *) &psy_cb->psy_tabname[0]),
			&psy_cb->psy_tabname[0]);
		    break;

		case E_RD0013_NO_TUPLE_FOUND:
		    (VOID) psf_error(5203, 0L, PSF_USERERR,
			&err_code, &psy_cb->psy_error, 1,
			sizeof (rdf_rb->rdr_qrymod_id),
			&rdf_rb->rdr_qrymod_id);
		    continue;
		
		default:
		    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
			&psy_cb->psy_error);
		}

		return (status);
	    }

	    /* invalidate the integrity tree from the cache. */
	    rdf_inv_cb.rdf_rb.rdr_sequence = psy_cb->psy_numbs[i];
	    rdf_inv_cb.rdf_rb.rdr_2types_mask |= RDR2_ALIAS;
	    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	    if (DB_FAILURE_MACRO(status))
	    {
		(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				    &psy_cb->psy_error);
		break;
	    }
	}
	/* now invalidate the integrity base object from the relation cache */
	if (DB_SUCCESS_MACRO(status))
	{
	    rdf_inv_cb.rdf_rb.rdr_sequence = 0;
	    rdf_inv_cb.rdf_rb.rdr_2types_mask &= ~RDR2_ALIAS;
	    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	    if (DB_FAILURE_MACRO(status))
	    {
		(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				    &psy_cb->psy_error);
	    }
	}
    }

    return    (status);
}
