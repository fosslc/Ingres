/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <me.h>
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
**  Name: PSYKPERM.C - Functions used to destroy permits.
**
**  Description:
**      This file contains the functions necessary to destroy permits.
**
**          psy_kpermit - Destroy one or more permits on a database object
**			  (a table,  a procedure or an event).
**
**
**  History:
**      02-oct-85 (jeff)    
**          written
**	05-aug-88 (andre)
**	    make changes for dropping permit on database procedures.
**	02-nov-89 (neil)
**	    Alerters: Changes for dropping permits on events.
**	16-jan-90 (ralph)
**	    Add integrity check for DROP PERMIT/SECURITY_ALARM:
**	    don't allow drop permit to drop a security alarm;
**	    don't allow drop security_alarm to drop a permit.
**	04-feb-91 (neil)
**	    Fix 2 error cases in psy_permit.
**	30-nov-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**	26-apr-93 (markg)
**	    Fixed bug in psy_kpermit() which caused AV when attempting 
**	    to drop a security_alarm.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
[@history_template@]...
**/


/*{
** Name: psy_kpermit	- Destroy one or more permits on a database object
**			  (a table or a procedure or an event).
**
**  INTERNAL PSF call format: status = psy_kpermit(&psy_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_KPERMIT, &psy_cb, &sess_cb);
**
** Description:
**      The psy_kpermit function removes the definition of one or more permits 
**      on a table, a procedure, or an event from all system relations
**	(protect, tree, and iiqrytext).
**	Optionally, one can tell this function to destroy all of the permits
**	on a given table, procedure or event.
**
**	Dropping all permits on a table is similar enough to dropping those on
**	a procedure, so we will handle them together.  On the other hand, when
**	permit numbers are specified, processing is somewhat differen, e.g.
**	we (at least for now) disallow dropping 0 and 1 on a dbproc.
**	Events follow exactly the same model as procedures.
** Inputs:
**      psy_cb
**          .psy_tables[0]              Id of table for which to destroy
**					permit(s)
**          .psy_numbs[]                Id numbers of the permits to destroy
**					(20 maximum)
**          .psy_numnms                 Number of permit numbers given.  Zero
**					means to destroy all of the permits on
**					the given table.
**	    .psy_tabname[0]		Name of the table for which to destroy
**					permit(s)
**	    .psy_grant			PSY_PDROP if dropping permits on
**					dbproc(s);
**					PSY_TDROP if dropping permits on
**					table(s).
**					PSY_SDROP if dropping security alarms.
**					PSY_EVDROP if dropping permits on
**					event(s).
**	sess_cb				Pointer to session control block.
**
** Outputs:
**	psy_cb
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
**	    Deletes query tree representing predicate of permit from tree
**	    relation, query text of permit definition from iiqrytext relation,
**	    does a dmf alter table on the base table to indicate that there
**	    are no more permissions on it should that happen to be the case.
**	    
** History:
**	02-oct-85 (jeff)
**          written
**	05-aug-88 (andre)
**	    modified for DROPping permits on dbprocs.
**	03-oct-88 (andre)
**	    Modify call to pst_rgent to pass 0 as a query mode since it is
**	    clearly not PSQ_DESTROY.
**	04-oct-88 (andre)
**	    Make sure that the highest status received is reported to caller
**	02-nov-89 (neil)
**	    Alerters: Changes for dropping permits on events.
**	16-jan-90 (ralph)
**	    Add integrity check for DROP PERMIT/SECURITY_ALARM:
**	    don't allow drop permit to drop a security alarm;
**	    don't allow drop security_alarm to drop a permit.
**	    Initialise status to E_DB_OK.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	04-feb-91 (neil)
**	    Fix 2 error cases:
**	    1. If pst_rgent returns E_PS0903_TAB_NOTFOUND then return this
**	       error to the user.  Can happen through Dynamic SQL where you
**	       drop the table before you exec the statement that drops grant.
**	    2. Allow E_RD0025_USER_ERROR from RDF_UPDATE and continue.
**	29-aug-91 (andre)
**	    Do not call RDF to destroy permits if the object is a QUEL view
**	    owned by the DBA.
**
**	    Until now we could safely assume that if a view is non-grantable and
**	    there are permits defined on it, then the view is a QUEL view owned
**	    by the DBA and the only permit defined on it is an access permit.
**	    With advent of GRANT WGO, there may be permits defined on views
**	    marked as non-grantable (since "grantable" means that the view is
**	    defined on top of the objects owned by its owner so that its owner
**	    can always grant access to the view) and grantable QUEL views will
**	    be marked as such (this way if a user creates an SQL view on top of
**	    his grantable QUEL view we are guaranteed that the new view is also
**	    grantable).  By not trying to destroy permits on QUEL views owned by
**	    the DBA we will ensure that a user cannot destroy an access permit
**	    which gets created for QUEL views owned by the DBA.
**	16-jun-92 (barbara)
**	    Change interface to pst_rgent.
**	22-jul-92 (andre)
**	    permits 0 and 1 will no longer hold special meaning for tables; in
**	    the past they meant ALL/RETRIEVE TO ALL, but with introduction of
**	    grantable privileges, they will no longer be created.  Whether
**	    permit numbers 0 and 1 will become available for regular permits
**	    needs to be decided, but one thing is for sure, psy_kpermit() will
**	    not need to know about it.
**
**	    If user is destroying all permits or security_alarms, set
**	    RDR_DROP_ALL over rdr_types_mask
**	27-jul-92 (andre)
**	    we may be told that we may not drop a permit or all permits because
**	    it would render some permit or object abandoned
**	03-aug-92 (barbara)
**	    Invalidate base table info from RDF cache.
**	07-aug-92 (teresa)
**	    RDF_INVALID must be called to invalidate the base object from RDF's
**	    relation cache as well as to remove any permit trees from RDF's
**	    qtree cache.
**	08-nov-92 (andre)
**	    having dropped ALL permits on PROCEDURE, remember to set
**	    RDR_PROCEDURE in rdf_inv_cb.rdf_rb.rdr_types_mask before calling
**	    RDF_INVALIDATE
**	26-apr-93 (markg)
**	    Fixed bug which caused AV when attempting to drop a 
**	    security_alarm. The problem was caused by incorrectly
**	    referencing a NULL pointer when initializing a local variable. 
**	10-aug-93 (andre)
**	    fixed cause of compiler warning
**	13-sep-93 (andre)
**	    PSF will no longer be in business of altering timestamps of tables
**	    (or underlying base tables of views) on which permit(s) or 
** 	    security_alarm(s) have been dropped - responsibility for this has 
** 	    been assumed by QEF
*/
DB_STATUS
psy_kpermit(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    RDF_CB              rdf_cb;
    RDF_CB              rdf_inv_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status = E_DB_OK;
    i4		err_code;
    i4		msgid;
    register i4	i;
    PSS_RNGTAB		*rngvar;

    /* Fill in RDF control block */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    pst_rdfcb_init(&rdf_inv_cb, sess_cb);
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[0], rdf_rb->rdr_tabid);
    STRUCT_ASSIGN_MACRO(psy_cb->psy_tables[0], rdf_inv_cb.rdf_rb.rdr_tabid);
    rdf_inv_cb.rdf_rb.rdr_types_mask = RDR_PROTECT;

    rdf_rb->rdr_update_op = RDR_DELETE;

    if (psy_cb->psy_grant == PSY_TDROP)		/* dropping perm on a table */
    {
	/* get range table entry for this table */
	status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "", PST_SHWID,
	    (DB_TAB_NAME*) NULL, (DB_TAB_OWN*) NULL, &psy_cb->psy_tables[0],
	    TRUE, &rngvar, (i4) 0, &psy_cb->psy_error);
	if (DB_FAILURE_MACRO(status))
	{
	    if (psy_cb->psy_error.err_code == E_PS0903_TAB_NOTFOUND)
	    {
		(VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, 1,
		    psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
			(char *) &psy_cb->psy_tabname[0]),
		    &psy_cb->psy_tabname[0]);
	    }
	    return (status);
	}
	rdf_rb->rdr_types_mask = RDR_PROTECT;
    }
    else if (psy_cb->psy_grant == PSY_SDROP)	/* dropping security alarm  */
    {
	rdf_rb->rdr_types_mask = RDR_SECALM;
    }
    else if (psy_cb->psy_grant == PSY_PDROP)    /* dropping perm on a dbproc */
    {
	DB_DBP_NAME	*dbpname = (DB_DBP_NAME *) psy_cb->psy_tabname;

	/* save dbproc name and owner for RDF */

	STRUCT_ASSIGN_MACRO(*dbpname, rdf_rb->rdr_name.rdr_prcname); 
	STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_rb->rdr_owner);
	
	rdf_rb->rdr_types_mask = RDR_PROTECT | RDR_PROCEDURE;
	rdf_inv_cb.rdf_rb.rdr_types_mask |= RDR_PROCEDURE;
    }
    else if (psy_cb->psy_grant == PSY_EVDROP)	    /* dropping perm on event */
    {
	DB_NAME		*evname = (DB_NAME *)psy_cb->psy_tabname;

	/* Save event name and owner for RDF */
	STRUCT_ASSIGN_MACRO(*evname, rdf_rb->rdr_name.rdr_evname); 
	STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_rb->rdr_owner);
	rdf_rb->rdr_types_mask = RDR_PROTECT | RDR_EVENT;
    }

    /* Zero permit numbers means destroy all permits */
    
    if (psy_cb->psy_numnms == 0)
    {
	/*								
	** Note that this block handles destroying all permits on
	** a dbproc, event or a table or all security_alarms on a table
	*/

	/*
	** check if user may drop permits (access permits get special
	** treatment).  If not, we are done.
	*/
	if (psy_cb->psy_grant == PSY_TDROP)
	{
	    /*
	    ** if this is a QUEL view owned by the DBA, avoid calling RDF since
	    ** we need to rpevent a user from destroying access permit which is
	    ** created on such views.
	    */
	    if (   rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW
		&& !MEcmp((PTR) &sess_cb->pss_dba, (PTR) &rngvar->pss_ownname,
			sizeof(DB_OWN_NAME))
	       )
	    {
		/* check if this is a QUEL view */
		i4	    issql = 0;

		status = psy_sqlview(rngvar, sess_cb, &psy_cb->psy_error,
		    &issql);
		if (DB_FAILURE_MACRO(status))
		{
		    return(status);
		}

		if (!issql)
		{
		    return(E_DB_OK);
		}
	    }
	}
	
	/* tell RDF that we are dropping ALL permits or security_alarms */
	rdf_rb->rdr_types_mask |= RDR_DROP_ALL;

	status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    switch (rdf_cb.rdf_error.err_code)
	    {
		case E_RD0002_UNKNOWN_TBL:  
		{
		    (VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR,
			&err_code, &psy_cb->psy_error, 1,
			psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
			    (char *) &psy_cb->psy_tabname[0]),
			&psy_cb->psy_tabname[0]);
		    break;
		}
		case E_RD0201_PROC_NOT_FOUND:
		{
		    (VOID) psf_error(E_PS0905_DBP_NOTFOUND, 0L, PSF_USERERR,
		        &err_code, &psy_cb->psy_error, 1,
		        psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
			    (char *) &psy_cb->psy_tabname[0]),
		        &psy_cb->psy_tabname[0]);
		    break;
		}
		case E_RD0210_ABANDONED_OBJECTS:
		{
		    char	*obj_type;
		    i4		type_len;

		    switch (psy_cb->psy_grant)
		    {
			case PSY_TDROP:
			    obj_type = "table";
			    type_len = sizeof("table") - 1;
			    break;
			case PSY_PDROP:
			    obj_type = "database procedure";
			    type_len = sizeof("database procedure") - 1;
			    break;
			case PSY_EVDROP:
			    obj_type = "dbevent";
			    type_len = sizeof("dbevent") - 1;
			    break;
			default:
			    obj_type = "UNKNOWN OBJECT TYPE";
			    type_len = sizeof("UNKNOWN OBJECT TYPE") - 1;
			    break;
		    }
		    
		    (VOID) psf_error(E_PS0564_ALLPROT_ABANDONED_OBJ, 0L,
			PSF_USERERR, &err_code, &psy_cb->psy_error, 2,
			type_len, obj_type,
			psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
			    (char *) psy_cb->psy_tabname),
			psy_cb->psy_tabname);
		    break;
		}
		default: 
		{
		    /* Event errors are handled in QEF */
		    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
					 &psy_cb->psy_error);
		}
	    }

	    return (status);
	}
	/*
	** Invalidate base object's infoblk from RDF cache; rdr_tabid
	** already contains base table id; Call RDF to invalidate the
	** base object. Then call again to invalidate any permit trees,
	** set rdr_2_types mask for tree.
	*/
	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb); /* drop infoblk */
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				&psy_cb->psy_error);
	    return(status);
	}
	rdf_inv_cb.rdf_rb.rdr_2types_mask |= RDR2_CLASS; /* drop permit trees */
	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				&psy_cb->psy_error);
	    return(status);
	}
    }
    else if (psy_cb->psy_grant == PSY_PDROP || psy_cb->psy_grant == PSY_EVDROP)
    {
	DB_STATUS   stat;
	
	/* Run through permit numbers, destroying each one */
	for (i = 0; i < psy_cb->psy_numnms; i++)
	{
	    rdf_rb->rdr_qrymod_id = psy_cb->psy_numbs[i];

	    stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);

	    /* remember the error to report later */
	    status = (status > stat) ? status : stat;

	    if (DB_FAILURE_MACRO(status))
	    {
		switch (rdf_cb.rdf_error.err_code)
		{
		    case E_RD0201_PROC_NOT_FOUND:
		    {
			(VOID) psf_error(E_PS0905_DBP_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			    psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
			 	(char *) &psy_cb->psy_tabname[0]),
			    &psy_cb->psy_tabname[0]);
			break;
		    }
		    case E_RD0210_ABANDONED_OBJECTS:
		    {
			char	*obj_type;
			i4	type_len;

			if (psy_cb->psy_grant == PSY_PDROP)
			{
			    obj_type = "database procedure";
			    type_len = sizeof("database procedure") - 1;
			}
			else
			{
			    obj_type = "dbevent";
			    type_len = sizeof("dbevent") - 1;
			}

			(VOID) psf_error(E_PS0565_PROT_ABANDONED_OBJ, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 3,
			    sizeof(psy_cb->psy_numbs[i]), psy_cb->psy_numbs + i,
			    type_len, obj_type,
			    psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
				(char *) psy_cb->psy_tabname),
			    psy_cb->psy_tabname);
			break;
		    }
		    case E_RD0013_NO_TUPLE_FOUND:
		    {
			(VOID) psf_error((i4) 5204, 0L, PSF_USERERR,
					 &err_code, &psy_cb->psy_error, 1,
					 sizeof (rdf_rb->rdr_qrymod_id),
					 &rdf_rb->rdr_qrymod_id);
			continue;
		    }

		    case E_RD0025_USER_ERROR:
			/*
			** Warning already handled - may be repeated when
			** we try some more updates (if this is a multi-
			** permit drop).
			*/
			if (psy_cb->psy_grant == PSY_EVDROP)
			{
			    status = E_DB_OK;
			    continue;
			}
			/* else fall through */
		    
		    default:
		    {
			/* Event-specific errors are reported through QEF */
			(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
					     &psy_cb->psy_error);
			break;
		    }
		}

		return (status);
	    }

	    /*
	    ** Invalidate base object's infoblk from RDF cache: rdr_tabid
	    ** already contains base table id; rdr_sequence contains
	    ** permit number; set rdr_2_types mask.
	    */
	    rdf_inv_cb.rdf_rb.rdr_sequence = psy_cb->psy_numbs[i];
	    rdf_inv_cb.rdf_rb.rdr_2types_mask |= RDR2_ALIAS;
	    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	    if (DB_FAILURE_MACRO(status))
	    {
		(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				    &psy_cb->psy_error);
		return(status);
	    }
	}
	/* 
	** invalidate the relation cache entry that the permit was defined on 
	*/
	rdf_inv_cb.rdf_rb.rdr_2types_mask &= ~RDR2_ALIAS;
	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				&psy_cb->psy_error);
	    return(status);
	}
    }
    else	    /* drop permit or security alarm on a table */
    {
	DB_STATUS   stat;
	bool	    cant_drop_perms = FALSE;

	if (psy_cb->psy_grant == PSY_TDROP)
	{
	    i4	mask = rngvar->pss_tabdesc->tbl_status_mask;
	    /*
	    ** if this is a QUEL view owned by the DBA, avoid calling RDF since
	    ** we need to rpevent a user from destroying access permit which is
	    ** created on such views.  Instead, we will act as if the tuple was
	    ** not found by QEF (without actually calling it.)
	    */
	    if (   mask & DMT_VIEW
		&& !MEcmp((PTR) &sess_cb->pss_dba, (PTR) &rngvar->pss_ownname,
			sizeof(DB_OWN_NAME))
	       )
	    {
		/* check if this is a QUEL view */
		i4	    issql = 0;

		status = psy_sqlview(rngvar, sess_cb, &psy_cb->psy_error,
		    &issql);
		if (DB_FAILURE_MACRO(status))
		{
		    return(status);
		}

		if (!issql)
		{
		    cant_drop_perms = TRUE;
		    
		    /* errors will be reported later */
		    status = E_DB_ERROR;
		}
	    }
	}

	/* Run through permit or security_alarm numbers, destroying each one */
	for (i = 0; i < psy_cb->psy_numnms; i++)
	{
	    if (cant_drop_perms)
	    {
		(VOID) psf_error(5204L, 0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error, 1,
		    sizeof (psy_cb->psy_numbs[i]), (PTR) &psy_cb->psy_numbs[i]);
		continue;
	    }
	    else
	    {
		rdf_rb->rdr_qrymod_id = psy_cb->psy_numbs[i];
	    }

	    stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);

	    /* remember the error to report later */
	    status = (status > stat) ? status : stat;
	    
	    if (DB_FAILURE_MACRO(stat))
	    {
		switch (rdf_cb.rdf_error.err_code)
		{
		    case E_RD0002_UNKNOWN_TBL:
		    {
			(VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			    psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
			        (char *) &psy_cb->psy_tabname[0]),
			    &psy_cb->psy_tabname[0]);
			break;
		    }

		    case E_RD0210_ABANDONED_OBJECTS:
		    {
			(VOID) psf_error(E_PS0565_PROT_ABANDONED_OBJ, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 3,
			    sizeof(psy_cb->psy_numbs[i]), psy_cb->psy_numbs + i,
			    sizeof("table") - 1, "table",
			    psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
				(char *) psy_cb->psy_tabname),
			    psy_cb->psy_tabname);
			break;
		    }
		    case E_RD0013_NO_TUPLE_FOUND:
		    {
			if (psy_cb->psy_grant == PSY_SDROP)
			    msgid = (i4)5213;
			else
			    msgid = (i4)5204;
			(VOID) psf_error(msgid, 0L, PSF_USERERR,
					 &err_code, &psy_cb->psy_error, 1,
					 sizeof (rdf_rb->rdr_qrymod_id),
					 &rdf_rb->rdr_qrymod_id);
			continue;
		    }

		    default:
		    {
			(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
					     &psy_cb->psy_error);
			break;
		    }
		}

		return (status);
	    }

	    /*
	    ** Invalidate base object's infoblk from RDF cache: rdr_tabid
	    ** already contains base table id; rdr_sequence contains
	    ** permit number (if not drop security alarm); set rdr_2_types mask.
	    */
	    if (psy_cb->psy_grant == PSY_SDROP)
	    {
		rdf_inv_cb.rdf_rb.rdr_2types_mask |= RDR2_KILLTREES;
	    }
	    else
	    {
		rdf_inv_cb.rdf_rb.rdr_2types_mask |= RDR2_ALIAS;
		rdf_inv_cb.rdf_rb.rdr_types_mask = RDR_PROTECT;
	    }
	    status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	    if (DB_FAILURE_MACRO(status))
	    {
		(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				    &psy_cb->psy_error);
		return(status);
	    }
	}

	/* now invalidate the base object that the permit was defined on */
	rdf_inv_cb.rdf_rb.rdr_2types_mask &= ~(RDR2_ALIAS | RDR2_KILLTREES);
	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				&psy_cb->psy_error);
	    return(status);
	}
    }

    return    (status);
} 
