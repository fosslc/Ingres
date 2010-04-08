/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <qu.h>
#include    <me.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/*
**
**  Name: PSYDSCHM.C - Code related to destruction of schemas.
**
**  Description:
**	This module contains code used to drop schemas.
**
**	    psy_dschema	    - Drop a schema
**
**  History:
**      19-feb-93 (andre)
**	    Created.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-sep-93 (swm)
**	    Added cs.h include above other header files which need its
**	    definition of CS_SID.
[@history_template@]...
*/

/*{
** Name: psy_dschema - Drop schema
**
** Description:
**	Call RDF_UPDATE to drop a schema.  Depending on whether a cascading or
**	restricted destruction was specified, all objects in the specified
**	schema (i.e. tables, views, dbprocs, dbevents, and everything that
**	depends on them (except for dbprocs in other schemas which will be
**	simply marked dormant.)
** Inputs:
**	psy_cb				PSY control block.
**	    psy_tuple
**		psy_schema		DB_IISCHEMA structure describing the
**					schema to be dropped
**		    db_schname		schema name
**		    db_schowner		schema owner name
**	    psy_flags			if CASCADE was specified,
**					PSY_DROP_CASCADE will be set
**	sess_cb		PSF session control block.
** Outputs:
**	Exceptions:
**	    none
** Returns:
**	E_DB_OK			schema was dropped successfully
**	error status from RDF	otherwise
**
** Side Effects:
**	    Modifies system catalogs.  During cascading destruction of a schema,
**	    many catalogs will get locked
**
** History:
**      19-feb-93 (andre)
**	    Created.
**	10-aug-93 (andre)
**	    fixed cause of a compiler warning
*/
DB_STATUS
psy_dschema(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
{
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status;
    i4		err_code;

    /* Initialize the RDF request block. */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_rb->rdr_owner);

    rdf_rb->rdr_2types_mask = (RDF_TYPES) RDR2_SCHEMA;
    if (psy_cb->psy_flags & PSY_DROP_CASCADE)
	rdf_rb->rdr_2types_mask |= RDR2_DROP_CASCADE;

    rdf_rb->rdr_update_op = RDR_DELETE;
    rdf_rb->rdr_qrytuple = (PTR) &psy_cb->psy_tuple.psy_schema;

    /* Atempt to drop a schema */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);

    if (DB_FAILURE_MACRO(status))
    {
	/*
	** an eror could happen because
	**	the schema did not exist,
	**	user specified RESTRICT, but the schema was not empty, or
	**	something went terribly wrong
	*/
	if (rdf_cb.rdf_error.err_code == E_RD0211_NONEXISTENT_SCHEMA)
	{
	    DB_IISCHEMA	    *schema =
		(DB_IISCHEMA *) &psy_cb->psy_tuple.psy_schema;
		
	    (void) psf_error(E_PS0570_NO_SCHEMA_TO_DROP, 0L, PSF_USERERR,
		&err_code, &psy_cb->psy_error, 2,
		psf_trmwhite((u_i4) sizeof(schema->db_schname),
		    (char *) &schema->db_schname),
		(PTR) &schema->db_schname,
		psf_trmwhite((u_i4) sizeof(schema->db_schowner),
		    (char *) &schema->db_schowner),
		(PTR)&schema->db_schowner);
	}
	else if (rdf_cb.rdf_error.err_code == E_RD0212_NONEMPTY_SCHEMA)
	{
	    DB_IISCHEMA	    *schema =
		(DB_IISCHEMA *) &psy_cb->psy_tuple.psy_schema;
		
	    (void) psf_error(E_PS0571_NONEMPTY_SCHEMA, 0L, PSF_USERERR,
		&err_code, &psy_cb->psy_error, 2,
		psf_trmwhite((u_i4) sizeof(schema->db_schname),
		    (char *) &schema->db_schname),
		(PTR) &schema->db_schname,
		psf_trmwhite((u_i4) sizeof(schema->db_schowner),
		    (char *) &schema->db_schowner),
		(PTR)&schema->db_schowner);
	}
	else
	{
	    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
		&psy_cb->psy_error);
	}
    }

    return (status);
}
