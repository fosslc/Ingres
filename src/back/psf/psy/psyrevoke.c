/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <bt.h>
#include    <cm.h>
#include    <me.h>
#include    <qu.h>
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
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <tm.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSYREVOKE.C - Function used to drive revovation of privileges on
**			tables, views, sequences, dbprocs and dbevents
**
**  Description:
**      This file contains a function which will drive revocation of privileges
**	as specified by REVOKE statement + functions that will be used in the
**	course of processing of a REVOKE statement to translate a map of
**	attributes of a view into map of attributes of a table or view on which
**	it is based and vice versa.
**
**          psy_revoke		-   Revoke privilege(s)
**	    psy_v2b_col_xlate	-   Translate a map of attributes of a view into
**				    a map of attributes of a specified table or
**				    view on which it is defined
**	    psy_v2b_col_xlate	-   Translate a map of attributes of a table or
**				    view into a map of attributes of a specified
**				    view which is defined on top of it
**
**  History:
**      02-aug-92 (andre)
**          written
**      06-nov-1992 (rog)
**          Included <ulf.h> before <qsf.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	23-May-1997 (shero03)
**	    Save the rdr_info_blk after UNFIX
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	    Changed psf_sesscb() prototype to return PSS_SESBLK* instead
**	    of DB_STATUS. Supply scf_session to SCF when it's known.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/

/*
**  Definition of static variables and forward functions.
*/

extern PSF_SERVBLK	*Psf_srvblk;


/*{
** Name: psy_revoke	- Execute REVOKE ON [TABLE] | PROCEDURE | DBEVENT |
**						SEQUENCE
**
**  INTERNAL PSF call format: status psy_revoke(psy_cb, sess_cb);
**
** Description:
**	Given a list of objects, a list of grantees,specification of privilege
**	to revoke, and an indication of whether cascading or restricted
**	revocatioon is desired, this function will drive processing.
**
** Inputs:
**      psy_cb
**          .psy_opmap                  Bit map of privileges to revoke
**	    .psy_grant			PSY_TREVOKE  - REVOKE ON [TABLE]
**					PSY_PREVOKE  - REVOKE ON PROCEDURE
**					PSY_SQREVOKE  - REVOKE ON SEQUENCE
**					PSY_EVREVOKE - REVOKE ON DBEVENT
**	    .psy_gtype			Grantee type:
**					DBGR_USER if users (and/or PUBLIC) were
**					specified
**					DBGR_GROUP if groups were specified
**					DBGR_APLID if application ids were
**					specified
**	    .psy_tblq			head of table queue
**	    .psy_usrq			head of user (or applid/group) queue
**	    .psy_u_numcols		number of elements in the list of
**					attribute ids of columns for which
**					UPDATE privilege will/will not be
**					revoked
**	    .psy_u_col_offset		offset into the list of attribute ids
**					associated with a given PSY_TBL entry of
**					the first element associated with UPDATE
**					privilege
**	    .psy_i_numcols		number of elements in the list of
**					attribute ids of columns for which
**					INSERT privilege will/will not be
**					revoked
**	    .psy_i_col_offset		offset into the list of attribute ids
**					associated with a given PSY_TBL entry of
**					the first element associated with INSERT
**					privilege
**	    .psy_r_numcols		number of elements in the list of
**					attribute ids of columns for which
**					REFERENCES privilege will/will not be
**					revoked
**	    .psy_r_col_offset		offset into the list of attribute ids
**					associated with a given PSY_TBL entry of
**					the first element associated with
**					REFERENCES privilege
**	    .psy_flags
**	  PSY_EXCLUDE_UPDATE_COLUMNS	attribute ids associated with UPDATE
**					privilege indicate for which columns
**					UPDATE privilege will not be revoked
**	  PSY_EXCLUDE_INSERT_COLUMNS	attribute ids associated with INSERT
*					privilege indicate for which columns
**					INSERT privilege will not be revoked
**	PSY_EXCLUDE_REFERENCES_COLUMNS	attribute ids associated with REFERENCES
**					privilege indicate fpr which columns
**					REFERENCES privilege will not be revoked
**	PSY_PUBLIC			privilege(s) being revoked from PUBLIC
**					(possibly, in addition to other users)
**	PSY_ALL_PRIVS			REVOKE ALL [PRIVILEGES] was specified,
**					i.e. privileges being revoked were not
**					named explicitly
**	PSY_DROP_CASCADE		if set, cascading revocation was
**					specified; otherwise restricted
**					revocation will take place
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
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Alters or destroys permit tuples as appropriate; if cascading
**	    revocation was specified, DBMS objects besides permits may be
**	    affected as well
**
** History:
**	02-aug-92 (andre)
**          written
**	10-dec-92 (andre)
**	    added support for REVOKE REFERENCES (col_list)
**	08-mar-94 (andre)
**	    fix for bug (60413)
**	    it is possible that the list of objects privileges on which are to 
**	    be revoked is empty (this may happen if the objects whose names 
**	    were specified in the statement did not exist or if the user 
**	    attempted to revoke REFERENCES on view(s)).  In such cases, we used
**	    to return without ever initializing status which, in some cases,
**	    resulted in SC0206.  To correct this problem, we will ensure that
**	    status start off as E_DB_OK.
**	24-mar-03 (inkdo01)
**	    Add support for "revoke ... on sequence ...".
*/
DB_STATUS
psy_revoke(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    RDF_CB              rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status = E_DB_OK;
    DB_PROTECTION	ptuple;
    register DB_PROTECTION *protup = &ptuple;
    i4			*domset	= ptuple.dbp_domset;
    register i4	i;
    i4		err_code;
    PSY_USR		*psy_usr;
    PSY_TBL		*psy_tbl;
    bool		updtcols, insrtcols, refcols;
    i4			colpriv_map;
    bool		noncol;
    bool		revoke_from_public;
    char		*ch;
    i4			revoke_all = psy_cb->psy_flags & PSY_ALL_PRIVS;

    /*
    ** Initialize IIPROTECT tuple which will describe the privileges to be
    ** revoked, their original grantor and the <grantee> from which they will be
    ** revoked
    */

    MEfill(sizeof(ptuple), (u_char) 0, (PTR) protup);
    protup->dbp_gtype = psy_cb->psy_gtype;	  /* Copy in grantee type */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, protup->dbp_grantor);

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);

    rdf_rb->rdr_update_op	= RDR_DELETE;   /* revocation of privilege(s) */
    rdf_rb->rdr_status		= DB_SQL;
    rdf_rb->rdr_indep		= (PTR) NULL;
    rdf_rb->rdr_qrytuple	= (PTR) protup;
    rdf_rb->rdr_querytext	= (char *) NULL;
    rdf_rb->rdr_v2b_col_xlate	= psy_v2b_col_xlate;
    rdf_rb->rdr_b2v_col_xlate	= psy_b2v_col_xlate;

    rdf_rb->rdr_types_mask = RDR_PROTECT;
    if (revoke_all)
	rdf_rb->rdr_types_mask |= RDR_REVOKE_ALL;

    rdf_rb->rdr_2types_mask = RDR2_REVOKE;
    if (psy_cb->psy_flags & PSY_DROP_CASCADE)
	rdf_rb->rdr_2types_mask |= RDR2_DROP_CASCADE;

    if (psy_cb->psy_grant == PSY_EVREVOKE || psy_cb->psy_grant == PSY_PREVOKE ||
	psy_cb->psy_grant == PSY_SQREVOKE)
    {
	protup->dbp_popset = psy_cb->psy_opmap;
	protup->dbp_popctl = psy_cb->psy_opctl;

	/*
	** set all bits in the attribute map to indicate that object-wide
	** privilege(s) are being revoked
	*/
	psy_fill_attmap(domset, ~((i4) 0));
    }

    /* revoking privileges on dbevent(s)? */
    if (psy_cb->psy_grant == PSY_EVREVOKE)
    {
	rdf_rb->rdr_types_mask |= RDR_EVENT;
	protup->dbp_obtype = DBOB_EVENT;

	/*
	** Call RDF to revoke specified privileges for each dbevent from each
	** grantee.
	*/
	for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	     psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	     psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next
	    )
	{
	    DB_EVENT_NAME     *ev_name = (DB_EVENT_NAME *) &psy_tbl->psy_tabnm;

	    STRUCT_ASSIGN_MACRO(*ev_name, rdf_rb->rdr_name.rdr_evname);

	    /*
	    ** save owner name for RDF - note that it may be different from the
	    ** name of the current user
	    */
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, rdf_rb->rdr_owner);
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, rdf_rb->rdr_tabid);
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, protup->dbp_tabid);

	    /* Initialize object name field in the iiprotect tuple */
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm, protup->dbp_obname);

	    /* store the name of the object owner */
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, protup->dbp_obown);

	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

	    /*
	    ** remember whether user has specified FROM PUBLIC
	    */
	    revoke_from_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;

	    do
	    {
		/* Get name of user losing privileges */
		if (!revoke_from_public)
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);
		}
		else
		{
		    /* revoke privilege(s) from PUBLIC */
		    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);
		    protup->dbp_gtype = DBGR_PUBLIC;
		}

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0210_ABANDONED_OBJECTS)
		    {
			/*
			** user specified RESTRICTed revocation, but some
			** objects and/or permits would become abandoned;
			*/
			if (revoke_from_public)
			{
			    (VOID) psf_error(E_PS0566_REVOKE_FROM_PUBLIC_ERR,
				0L, PSF_USERERR, &err_code, &psy_cb->psy_error,
				3,
				sizeof("dbevent") - 1, "dbevent",
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm);
			}
			else
			{
			    (VOID) psf_error(E_PS0567_REVOKE_FROM_USER_ERR, 0L,
				PSF_USERERR, &err_code, &psy_cb->psy_error, 4,
				sizeof("dbevent") - 1, "dbevent",
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm,
				psf_trmwhite(sizeof(psy_usr->psy_usrnm),
				    (char *) &psy_usr->psy_usrnm),
				&psy_usr->psy_usrnm);
			}
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
			    &psy_cb->psy_error);
		    }

		    goto exit;
		}

		/*
		** if we just revoked privileges from PUBLIC, dbp_gtype in
		** IIPROTECT tuple need to be modified if some "real" grantees
		** were named
		*/
		if (revoke_from_public)
		{
		    protup->dbp_gtype = psy_cb->psy_gtype;

		    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
		    {
			/* some grantee(s) were named in addition to PUBLIC */
			revoke_from_public = FALSE;
		    }
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);
	}	/* for all dbevents */

	goto exit;
    }
    /* revoking privileges on sequence(s)? */
    else if (psy_cb->psy_grant == PSY_SQREVOKE)
    {
	rdf_rb->rdr_2types_mask |= RDR2_SEQUENCE;
	protup->dbp_obtype = DBOB_SEQUENCE;

	/*
	** Call RDF to revoke specified privileges for each sequence from each
	** grantee.
	*/
	for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	     psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	     psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next
	    )
	{
	    DB_NAME         *seq_name = (DB_NAME *) &psy_tbl->psy_tabnm;

	    STRUCT_ASSIGN_MACRO(*seq_name, rdf_rb->rdr_name.rdr_seqname);

	    /*
	    ** save owner name for RDF - note that it may be different from the
	    ** name of the current user
	    */
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, rdf_rb->rdr_owner);
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, rdf_rb->rdr_tabid);
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, protup->dbp_tabid);

	    /* Initialize object name field in the iiprotect tuple */
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm, protup->dbp_obname);

	    /* store the name of the object owner */
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, protup->dbp_obown);

	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

	    /*
	    ** remember whether user has specified FROM PUBLIC
	    */
	    revoke_from_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;

	    do
	    {
		/* Get name of user losing privileges */
		if (!revoke_from_public)
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);
		}
		else
		{
		    /* revoke privilege(s) from PUBLIC */
		    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);
		    protup->dbp_gtype = DBGR_PUBLIC;
		}

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0210_ABANDONED_OBJECTS)
		    {
			/*
			** user specified RESTRICTed revocation, but some
			** objects and/or permits would become abandoned;
			*/
			if (revoke_from_public)
			{
			    (VOID) psf_error(E_PS0566_REVOKE_FROM_PUBLIC_ERR,
				0L, PSF_USERERR, &err_code, &psy_cb->psy_error,
				3,
				sizeof("dbevent") - 1, "sequence",
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm);
			}
			else
			{
			    (VOID) psf_error(E_PS0567_REVOKE_FROM_USER_ERR, 0L,
				PSF_USERERR, &err_code, &psy_cb->psy_error, 4,
				sizeof("sequence") - 1, "sequence",
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm,
				psf_trmwhite(sizeof(psy_usr->psy_usrnm),
				    (char *) &psy_usr->psy_usrnm),
				&psy_usr->psy_usrnm);
			}
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
			    &psy_cb->psy_error);
		    }

		    goto exit;
		}

		/*
		** if we just revoked privileges from PUBLIC, dbp_gtype in
		** IIPROTECT tuple need to be modified if some "real" grantees
		** were named
		*/
		if (revoke_from_public)
		{
		    protup->dbp_gtype = psy_cb->psy_gtype;

		    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
		    {
			/* some grantee(s) were named in addition to PUBLIC */
			revoke_from_public = FALSE;
		    }
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);
	}	/* for all sequences */

	goto exit;
    }
    /* revoking privileges on dbprocs? */
    else if (psy_cb->psy_grant == PSY_PREVOKE)
    {
	rdf_rb->rdr_types_mask |= RDR_PROCEDURE;
	protup->dbp_obtype = DBOB_DBPROC;
	
	/*
	** Call RDF to revoke specified privileges for each dbproc.
	*/
	for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	     psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	     psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next
	    )
	{
	    DB_DBP_NAME	    *dbp_name = (DB_DBP_NAME *) &psy_tbl->psy_tabnm;

	    STRUCT_ASSIGN_MACRO(*dbp_name, rdf_rb->rdr_name.rdr_prcname);

	    /*
	    ** save owner name for RDF - note that it may be different from the
	    ** name of the current user
	    */
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, rdf_rb->rdr_owner);
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, rdf_rb->rdr_tabid);
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, protup->dbp_tabid);

	    /* Initialize object name field in the iiprotect tuple */
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm, protup->dbp_obname);

	    /* store the name of the object owner */
	    STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, protup->dbp_obown);

	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

	    /*
	    ** remember whether user has specified FROM PUBLIC
	    */
	    revoke_from_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;

	    do
	    {
		/* Get name of user losing privileges */
		if (!revoke_from_public)
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);
		}
		else
		{
		    /* revoke privilege(s) from PUBLIC */
		    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);
		    protup->dbp_gtype = DBGR_PUBLIC;
		}

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0210_ABANDONED_OBJECTS)
		    {
			/*
			** user specified RESTRICTed revocation, but some
			** objects and/or permits would become abandoned;
			*/
			if (revoke_from_public)
			{
			    (VOID) psf_error(E_PS0566_REVOKE_FROM_PUBLIC_ERR,
				0L, PSF_USERERR, &err_code, &psy_cb->psy_error,
				3,
				sizeof("database procedure") - 1,
				"database procedure",
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm);
			}
			else
			{
			    (VOID) psf_error(E_PS0567_REVOKE_FROM_USER_ERR, 0L,
				PSF_USERERR, &err_code, &psy_cb->psy_error, 4,
				sizeof("database procedure") - 1,
				"database procedure",
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm,
				psf_trmwhite(sizeof(psy_usr->psy_usrnm),
				    (char *) &psy_usr->psy_usrnm),
				&psy_usr->psy_usrnm);
			}
		    }
		    else if (rdf_cb.rdf_error.err_code ==
			         E_RD0201_PROC_NOT_FOUND)
		    {
			_VOID_ psf_error(E_PS0905_DBP_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			    psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				(char *) &psy_tbl->psy_tabnm),
			    &psy_tbl->psy_tabnm);
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
			    &psy_cb->psy_error);
		    }

		    goto exit;
		}

		/*
		** if we just revoked privileges from PUBLIC, dbp_gtype in
		** IIPROTECT tuple need to be modified if some "real" grantees
		** were named
		*/
		if (revoke_from_public)
		{
		    protup->dbp_gtype = psy_cb->psy_gtype;

		    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
		    {
			/* some grantee(s) were named in addition to PUBLIC */
			revoke_from_public = FALSE;
		    }
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);
	} /* for all dbprocs */

	goto exit;	/* skip the part dealing with tables */

    } /* if psy_grant == PSY_PREVOKE */

    /*
    ** Let's establish which (if any) column-specific and table-wide privileges
    ** are being revoked.
    */
    colpriv_map = 0;
    if (updtcols = psy_cb->psy_u_numcols != 0)
	colpriv_map |= (i4) DB_REPLACE;
	
    if (insrtcols = psy_cb->psy_i_numcols != 0)
	colpriv_map |= (i4) DB_APPEND;

    if (refcols = psy_cb->psy_r_numcols != 0)
	colpriv_map |= (i4) DB_REFERENCES;

    noncol = (psy_cb->psy_opmap & ~((i4) DB_GRANT_OPTION | colpriv_map)) != 0;

    if (psy_cb->psy_opmap & DB_RETRIEVE)
    {
	psy_cb->psy_opmap |= DB_TEST | DB_AGGREGATE;
	psy_cb->psy_opctl |= DB_TEST | DB_AGGREGATE;
    }

    /*
    ** if only table-wide privileges were specified, we can copy the privilege
    ** map into IIPROTECT tuple now since it will not have to change
    */
    if (!colpriv_map)
    {
	protup->dbp_popset = psy_cb->psy_opmap;
	protup->dbp_popctl = psy_cb->psy_opctl;
    }

    /*
    ** Call RDF to revoke privileges for each table.
    */
    for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next
	)
    {
	/* The table on which privilege is being revoked */
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, rdf_rb->rdr_tabid);
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid, protup->dbp_tabid);

	/* Initialize object name in the iiprotect tuple */
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabnm, protup->dbp_obname);

	/* store the name of the object owner */
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_owner, protup->dbp_obown);

	protup->dbp_obtype = (psy_tbl->psy_mask & PSY_OBJ_IS_TABLE)
	    ? DBOB_TABLE : DBOB_VIEW;
	    
	/*
	** If we have some table-wide and some column-specific privileges to
	** revoke, we will first revoke table-wide privileges and then go for
	** the column-specific ones (no, no particular reason...)
	*/

	if (noncol)
	{
	    if (colpriv_map)
	    {
		/*
		** if some column-specific privileges are being revoked in
		** addition to table-wide ones, turn off bits representing
		** column-specific privileges in the protection tuple
		**
		** Note that if a user specified ALL [PRIVILEGES],
		** colpriv_map would not be set
		*/
		protup->dbp_popset = psy_cb->psy_opmap & ~colpriv_map;
		protup->dbp_popctl = psy_cb->psy_opctl & ~colpriv_map;
	    }

	    /*
	    ** set all bits in the attribute map to indicate that table-wide
	    ** privilege(s) are being revoked
	    */
	    psy_fill_attmap(domset, ~((i4) 0));

	    /*
	    ** remember whether user has specified FROM TO PUBLIC 
	    */
	    revoke_from_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;

	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

	    do
	    {
		/*
		** Get name of user losing privileges
		*/
		if (!revoke_from_public)
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);
		}
		else
		{
		    /*
		    ** No grantee name was specified, use default.
		    */
		    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);
		    protup->dbp_gtype = DBGR_PUBLIC;
		}

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0210_ABANDONED_OBJECTS)
		    {
			/*
			** user specified RESTRICTed revocation, but some
			** objects and/or permits would become abandoned;
			*/
			char	    *obj_type;
			i4	    type_len;

			if (psy_tbl->psy_mask & PSY_OBJ_IS_TABLE)
			{
			    obj_type = "table";
			    type_len = sizeof("table") - 1;
			}
			else
			{
			    obj_type = "view";
			    type_len = sizeof("view") - 1;
			}
			
			if (revoke_from_public)
			{
			    (VOID) psf_error(E_PS0566_REVOKE_FROM_PUBLIC_ERR,
				0L, PSF_USERERR, &err_code, &psy_cb->psy_error,
				3,
				type_len, obj_type,
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm);
			}
			else
			{
			    (VOID) psf_error(E_PS0567_REVOKE_FROM_USER_ERR, 0L,
				PSF_USERERR, &err_code, &psy_cb->psy_error, 4,
				type_len, obj_type,
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm,
				psf_trmwhite(sizeof(psy_usr->psy_usrnm),
				    (char *) &psy_usr->psy_usrnm),
				&psy_usr->psy_usrnm);
			}
		    }
		    else if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		    {
			_VOID_ psf_error(E_PS0903_TAB_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			    psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				(char *) &psy_tbl->psy_tabnm),
			    &psy_tbl->psy_tabnm);
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			    &psy_cb->psy_error);
		    }

		    goto exit;
		}

		/*
		** if we just revoked privileges from PUBLIC, dbp_gtype in
		** IIPROTECT tuple need to be modified if some "real" grantees
		** were named
		*/
		if (revoke_from_public)
		{
		    protup->dbp_gtype = psy_cb->psy_gtype;

		    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
		    {
			/* some grantee(s) were named in addition to PUBLIC */
			revoke_from_public = FALSE;
		    }
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);
	}

	/*
	** Stage 2.
	** Now call RDF to revoke column-specific UPDATE privilege if it was
	** specified
	*/
	if (updtcols)
	{
	    i4	*attrid;

	    protup->dbp_popset = protup->dbp_popctl =
		DB_REPLACE | (psy_cb->psy_opmap & DB_GRANT_OPTION);

	    psy_prv_att_map((char *) domset,
		(bool) ((psy_cb->psy_flags & PSY_EXCLUDE_UPDATE_COLUMNS) != 0),
		psy_tbl->psy_colatno + psy_cb->psy_u_col_offset,
		psy_cb->psy_u_numcols);

	    /*
	    ** remember whether user has specified FROM PUBLIC 
	    */
	    revoke_from_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;

	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

	    do
	    {
		/* Get name of user losing privileges */
		if (!revoke_from_public)
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);
		}
		else
		{
		    /*
		    ** No grantee names were specified, use default.
		    */
		    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);
		    protup->dbp_gtype = DBGR_PUBLIC;
		}

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0210_ABANDONED_OBJECTS)
		    {
			/*
			** user specified RESTRICTed revocation, but some
			** objects and/or permits would become abandoned;
			*/
			char	    *obj_type;
			i4	    type_len;

			if (psy_tbl->psy_mask & PSY_OBJ_IS_TABLE)
			{
			    obj_type = "table";
			    type_len = sizeof("table") - 1;
			}
			else
			{
			    obj_type = "view";
			    type_len = sizeof("view") - 1;
			}
			
			if (revoke_from_public)
			{
			    (VOID) psf_error(E_PS0566_REVOKE_FROM_PUBLIC_ERR,
				0L, PSF_USERERR, &err_code, &psy_cb->psy_error,
				3,
				type_len, obj_type,
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm);
			}
			else
			{
			    (VOID) psf_error(E_PS0567_REVOKE_FROM_USER_ERR, 0L,
				PSF_USERERR, &err_code, &psy_cb->psy_error, 4,
				type_len, obj_type,
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm,
				psf_trmwhite(sizeof(psy_usr->psy_usrnm),
				    (char *) &psy_usr->psy_usrnm),
				&psy_usr->psy_usrnm);
			}
		    }
		    else if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		    {
			_VOID_ psf_error(E_PS0903_TAB_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			    psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				(char *) &psy_tbl->psy_tabnm),
			    &psy_tbl->psy_tabnm);
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			    &psy_cb->psy_error);
		    }

		    goto exit;
		}

		/*
		** if we just revoked privileges from PUBLIC, dbp_gtype in
		** IIPROTECT tuple need to be modified if some "real" grantees
		** were named
		*/
		if (revoke_from_public)
		{
		    protup->dbp_gtype = psy_cb->psy_gtype;

		    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
		    {
			/* some grantee(s) were named in addition to PUBLIC */
			revoke_from_public = FALSE;
		    }
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);	

	} /* if (updtcols) */

	/*
	** Stage 3.
	** Now call RDF to revoke column-specific REFERENCES privilege if
	** it was specified
	*/
	if (refcols)
	{
	    i4	*attrid;

	    protup->dbp_popset = protup->dbp_popctl =
		DB_REFERENCES | (psy_cb->psy_opmap & DB_GRANT_OPTION);

	    psy_prv_att_map((char *) domset,
		(bool) ((psy_cb->psy_flags & PSY_EXCLUDE_REFERENCES_COLUMNS)
			!= 0),
		psy_tbl->psy_colatno + psy_cb->psy_r_col_offset,
		psy_cb->psy_r_numcols);

	    /*
	    ** remember whether user has specified FROM PUBLIC 
	    */
	    revoke_from_public = (psy_cb->psy_flags & PSY_PUBLIC) != 0;

	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;

	    do
	    {
		/* Get name of user losing privileges */
		if (!revoke_from_public)
		{
		    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, protup->dbp_owner);
		}
		else
		{
		    /*
		    ** No grantee names were specified, use default.
		    */
		    STRUCT_ASSIGN_MACRO(psy_cb->psy_user, protup->dbp_owner);
		    protup->dbp_gtype = DBGR_PUBLIC;
		}

		status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
		if (DB_FAILURE_MACRO(status))
		{
		    if (rdf_cb.rdf_error.err_code == E_RD0210_ABANDONED_OBJECTS)
		    {
			/*
			** user specified RESTRICTed revocation, but some
			** objects and/or permits would become abandoned;
			*/
			char	    *obj_type;
			i4	    type_len;

			if (psy_tbl->psy_mask & PSY_OBJ_IS_TABLE)
			{
			    obj_type = "table";
			    type_len = sizeof("table") - 1;
			}
			else
			{
			    obj_type = "view";
			    type_len = sizeof("view") - 1;
			}
			
			if (revoke_from_public)
			{
			    (VOID) psf_error(E_PS0566_REVOKE_FROM_PUBLIC_ERR,
				0L, PSF_USERERR, &err_code, &psy_cb->psy_error,
				3,
				type_len, obj_type,
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm);
			}
			else
			{
			    (VOID) psf_error(E_PS0567_REVOKE_FROM_USER_ERR, 0L,
				PSF_USERERR, &err_code, &psy_cb->psy_error, 4,
				type_len, obj_type,
				psf_trmwhite(sizeof(psy_tbl->psy_owner),
				    (char *) &psy_tbl->psy_owner),
				&psy_tbl->psy_owner,
				psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				    (char *) &psy_tbl->psy_tabnm),
				&psy_tbl->psy_tabnm,
				psf_trmwhite(sizeof(psy_usr->psy_usrnm),
				    (char *) &psy_usr->psy_usrnm),
				&psy_usr->psy_usrnm);
			}
		    }
		    else if (rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		    {
			_VOID_ psf_error(E_PS0903_TAB_NOTFOUND, 0L,
			    PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			    psf_trmwhite(sizeof(psy_tbl->psy_tabnm),
				(char *) &psy_tbl->psy_tabnm),
			    &psy_tbl->psy_tabnm);
		    }
		    else
		    {
			_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			    &psy_cb->psy_error);
		    }

		    goto exit;
		}

		/*
		** if we just revoked privileges from PUBLIC, dbp_gtype in
		** IIPROTECT tuple need to be modified if some "real" grantees
		** were named
		*/
		if (revoke_from_public)
		{
		    protup->dbp_gtype = psy_cb->psy_gtype;

		    if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
		    {
			/* some grantee(s) were named in addition to PUBLIC */
			revoke_from_public = FALSE;
		    }
		}
		else
		{
		    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
		}

	    } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);	

	} /* if (refcols) */

    } /* for each table */

exit:

    return (status);
} 

/*
** Name: psy_v2b_col_xlate  -	translate a map of attributes of an updateable
**				view into a map of attributes of a specified
**				underlying table or view
**
** Description:
**	Given a map of attributes of an updateable view, translate it into a map
**	of attributes of specified underlying table or view.  We stipulate that
**	the view must be updateable since we are not prepared to deal with a
**	case when a view is defined on top of more than one table or view
**
** Input:
**	view_id		    id of the updateable view whose attribute map needs
**			    to be translated
**	view_attrmap	    map of attributes to be translated
**	base_id		    id of underlying table or view
**
** Output:
**	base_attrmap	    translated map of attributes
**
** Side effects:
**	none
**
** Returns:
**	E_DB_{OK,ERROR...}
**
** History:
**
**	12-aug-92 (andre)
**	    written
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	23-May-1997 (shero03)
**	    Save the rdr_info_blk after UNFIX
*/
DB_STATUS
psy_v2b_col_xlate(
	DB_TAB_ID	*view_id,
	i4		*view_attrmap,
	DB_TAB_ID	*base_id,
	i4		*base_attrmap)
{
    RDF_CB                  rdf_cntrl_block, *rdf_cb = &rdf_cntrl_block;
    RDR_RB                  *rdf_rb = &rdf_cb->rdf_rb;
    DB_ERROR		    err_blk;
    PSS_RNGTAB              *rngvar;
    i4			    i;
    i4			    attrmap[DB_COL_WORDS];
    DB_STATUS               status;
    i4			    unfix = TRUE;
    PSS_SESBLK              *sess_cb;

    sess_cb = psf_sesscb();

    sess_cb->pss_auxrng.pss_maxrng = -1;

    /*
    ** to facilitate translation, we will reverse the attribute map (this will
    ** save us many unnecessary traversals of the view definition target list)
    */
    psy_fill_attmap(attrmap, ((i4) 0));
    
    for (i = 0; (i = BTnext(i, (char *) view_attrmap, DB_MAX_COLS + 1)) > -1; )
    {
	BTset(DB_MAX_COLS - i, (char *) attrmap);
    }
	
    pst_rdfcb_init(rdf_cb, sess_cb);

    /* init custom fields in rdf_cb */
    rdf_rb->rdr_types_mask    = RDR_VIEW | RDR_QTREE;
    rdf_rb->rdr_qtuple_count  = 1;
    rdf_rb->rdr_rec_access_id = NULL;

    /*
    ** The algorithm here is pretty simple:
    **	- verify that the object pointed to by rngvar is a view;
    **	- get its view tree, translate the map of those of its attributes that
    **	  are of interest to us (attr_no-th bit set in attrmap) into a map of
    **	  attributes of its underlying table by setting bits corresponding to
    **	  the numbers of the underlying table or view attributes in base_map
    **	- if the id of the underlying table is the same as that in base_id, we
    **	  are done; 
    **	- otherwise, get the description of the underlying table and start all
    **	  over
    **	
    */

    /* get description of the view by id to get the algorithm going */
    status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "!", PST_SHWID,
	(DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL, view_id,
	TRUE, &rngvar, PSQ_REVOKE, &err_blk);
    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    for (;;)	    /* something to break out of */
    {
	i4		err_code;
	i4		cur_attr, next_attr;
	i4		found = FALSE;
	DB_TAB_ID	cur_id;
	PST_QNODE	*node;
	PST_QTREE       *vtree;
	PST_RT_NODE     *vtree_root;

	if (~rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
	{
	    /*
	    ** if this is not a view, then something is very wrong - this should
	    ** never happen 
	    */
	    status = E_DB_ERROR;
	    break;
	}

	/* obtain the view tree */
	STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_rb->rdr_tabid);
	rdf_cb->rdf_info_blk = rngvar->pss_rdrinfo;

	status = rdf_call(RDF_GETINFO, (PTR) rdf_cb);

        /*
        ** RDF may choose to allocate a new info block and return its address in
        ** rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid memory
        ** leak and other assorted unpleasantries
        */
	rngvar->pss_rdrinfo = rdf_cb->rdf_info_blk;

	if (DB_FAILURE_MACRO(status))
	{
	    _VOID_ psf_rdf_error(RDF_GETINFO, &rdf_cb->rdf_error, &err_blk);
	    break;
	}

	/* Make vtree point to the view tree retrieved */
	vtree = ((PST_PROCEDURE *) rdf_cb->rdf_info_blk->
	    rdr_view->qry_root_node)->pst_stmts->pst_specific.pst_tree;

	vtree_root = &vtree->pst_qtree->pst_sym.pst_value.pst_s_root;
	
	/*
	** now we will walk the view tree and translate map of view attributes
	** into a map of attributes of the underlying relation
	** If the id of the underlying relation matches that in base_id, when
	** translating the map we will set i-th bit to indicate the i-th
	** attribute; otherwise we will again build a reversed map
	*/
	psy_fill_attmap(base_attrmap, ((i4) 0));

	/* set cur_attr to the offset of the first possibly interesting bit */
	cur_attr = DB_MAX_COLS - rngvar->pss_tabdesc->tbl_attr_count - 1;

	/*
	** compute offset in the array of pst_rangetab entries of the id of the
	** table on which the view is based
	*/ 
	i = BTnext(-1, (char *) &vtree_root->pst_tvrm, PST_NUMVARS);

	/* determine if this is the id we've been looking for */
	STRUCT_ASSIGN_MACRO(vtree->pst_rangetab[i]->pst_rngvar, cur_id);
	
	if (   cur_id.db_tab_base  == base_id->db_tab_base
	    && cur_id.db_tab_index == base_id->db_tab_index)
	{
	    found = TRUE;

	    /*
	    ** store in base_attrmap a map of attributes of the specified
	    ** table (ID==base_id) which corespond to the updateable attributes
	    ** of the view (ID==view_id) which were specified in view_attrmap
	    */
	    for (node = vtree->pst_qtree,
		 next_attr = BTnext(cur_attr, (char *) attrmap, DB_MAX_COLS);

		 next_attr != -1;

		 next_attr = BTnext(next_attr, (char *) attrmap, DB_MAX_COLS)
		)
	    {
		PST_SYMBOL	*sym;

		for (; cur_attr < next_attr; cur_attr++, node = node->pst_left)
		;

		/*
		** only need to record the updateable attributes; avoid TID
		*/
		sym = &node->pst_right->pst_sym;
		
		if (   sym->pst_type == PST_VAR
		    && sym->pst_value.pst_s_var.pst_atno.db_att_id != 0)
		{
		    BTset(sym->pst_value.pst_s_var.pst_atno.db_att_id,
			(char *) base_attrmap);
		}
	    }
	}
	else
	{
	    i4		empty_map = TRUE;

	    /*
	    ** store in base_attrmap a reversed map of attributes of a table
	    ** (ID==cur_id) which corespond to the attributes of the view
	    ** (ID==view_id) which were specified in view_attrmap, then copy it
	    ** into attrmap for the next pass
	    */
	    for (node = vtree->pst_qtree,
		 next_attr = BTnext(cur_attr, (char *) attrmap, DB_MAX_COLS);

		 next_attr != -1;

		 next_attr = BTnext(next_attr, (char *) attrmap, DB_MAX_COLS)
		)
	    {
		PST_SYMBOL	*sym;

		for (; cur_attr < next_attr; cur_attr++, node = node->pst_left)
		;

		/*
		** only need to record the updateable attributes; avoid TID
		*/
		sym = &node->pst_right->pst_sym;
		
		if (   sym->pst_type == PST_VAR
		    && sym->pst_value.pst_s_var.pst_atno.db_att_id != 0)
		{
		    empty_map = FALSE;

		    BTset(DB_MAX_COLS -
			      sym->pst_value.pst_s_var.pst_atno.db_att_id,
			(char *) base_attrmap);
		}
	    }

	    /*
	    ** if the attribute map is empty, there is no reason to proceed
	    ** (I do not expect this to happen too often); otherwise copy the
	    ** attribute map from the base_attrmap into attrmap
	    */
	    if (!(found = empty_map))
	    {
		for (i = 0; i < DB_COL_WORDS; i++)
		    attrmap[i] = base_attrmap[i];
	    }
	}

	/* we are done with this entry - set it up for reuse */
	rngvar->pss_used = FALSE;
	QUremove((QUEUE *) rngvar);
	QUinsert((QUEUE *) rngvar, sess_cb->pss_auxrng.pss_qhead.q_prev);

	/*
	** if we need to continue the search, get the description of the next
	** view
	*/
	if (!found)
	{
	    status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "!", PST_SHWID,
		(DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL, &cur_id,
		TRUE, &rngvar, PSQ_REVOKE, &err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		unfix = FALSE;
		break;
	    }
	}
	else
	{
	    break;
	}
    }

    if (unfix)
    {
	DB_STATUS	unfix_status;

	rngvar->pss_used = FALSE;
	QUremove((QUEUE *) rngvar);
	QUinsert((QUEUE *) rngvar, sess_cb->pss_auxrng.pss_qhead.q_prev);

	rdf_rb->rdr_types_mask = rdf_rb->rdr_2types_mask = 0;
	
	unfix_status = rdf_call(RDF_UNFIX, (PTR) rdf_cb);
	rngvar->pss_rdrinfo = rdf_cb->rdf_info_blk;
	if (DB_FAILURE_MACRO(unfix_status))
	{
	    (VOID) psf_rdf_error(RDF_UNFIX, &rdf_cb->rdf_error, &err_blk);
	    if (unfix_status > status)
		status = unfix_status;
	}
    }

    return(status);
}

/*
** Name: psy_b2v_col_xlate  -	translate a map of attributes of a table or view
**				X1 into a map of attributes of a specified
**				updateable view X2 which is defined on top of X1
**
** Description:
**	Given a map of attributes of a table or view X1, translate it into a map
**	of attributes of a specified updateable view X2 which is defined on top
**	of X1.  We stipulate that X2 must be updateable since we are not
**	prepared to deal with a case when a view is defined on top of more than
**	one table or view
**
** Input:
**	view_id		    id of the updateable view for which were were asked
**			    to find attributes which correspond to specified
**			    attributes of a table or a view on which it was
**			    defined
**	base_id		    id of an underlying table or view
**	base_attrmap	    map of attributes of an underlying table or view to
**			    be translated
**
** Output:
**	view_attrmap	    translated map of attributes of the specified view
**
** Side effects:
**	none
**
** Returns:
**	E_DB_{OK,ERROR...}
**
** History:
**
**	13-aug-92 (andre)
**	    written
**	29-sep-92 (andre)
**	    RDF may choose to allocate a new info block and return its address
**	    in rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid
**	    memory leak and other assorted unpleasantries
**	23-May-1997 (shero03)
**	    Save the rdr_info_blk after UNFIX
*/
DB_STATUS
psy_b2v_col_xlate(
	DB_TAB_ID	*view_id,
	i4		*view_attrmap,
	DB_TAB_ID	*base_id,
	i4		*base_attrmap)
{
    RDF_CB                  rdf_cntrl_block, *rdf_cb = &rdf_cntrl_block;
    RDR_RB                  *rdf_rb = &rdf_cb->rdf_rb;
    DB_ERROR		    err_blk;
    PSS_RNGTAB              *rngvar;
    i4			    i;
    i2			    attrnums[2 * DB_MAX_COLS + 2];
    i2			    *cur_map, *copy_map;
    DB_STATUS               status;
    i4			    unfix = TRUE;
    PSS_SESBLK              *sess_cb;

    sess_cb = psf_sesscb();

    sess_cb->pss_auxrng.pss_maxrng = -1;

    pst_rdfcb_init(rdf_cb, sess_cb);

    /* init custom fields in rdf_cb */
    rdf_rb->rdr_types_mask    = RDR_VIEW | RDR_QTREE;
    rdf_rb->rdr_qtuple_count  = 1;
    rdf_rb->rdr_rec_access_id = NULL;

    /*
    ** The algorithm here is pretty simple:
    ** NOTE: if cur_map[i] != 0, it indicates that the i-th attribute of the
    **       relation represented by rngvar corresponds to cur_map[i]-th
    **	     attribute of the view whose id was specified in view_id
    **	- verify that the object pointed to by rngvar is a view;
    **	- get its view tree, translate the map of its attributes into a map
    **	  of attributes of its underlying table as follows: 
    **	      for every i in [1, rngvar->pss_tabdesc->tbl_attr_count]
    **	      {
    **	  	if (   cur_map[i] > 0 (i.e. cur_map[i]-th attribute of the
    **		       original view appears to be updateable so far) 
    **		    && the i-th attribute of the relation represented by rngvar
    **		       is updateable (i.e. it is defined on top of j-th
    **		       attribute of an underlying table or view without any
    **		       intervening expressions)
    **		   )
    **		{ 
    **		  set copy_map[j] to cur_map[i]
    **		}
    **	      }
    **  - if (the id of the underlying table is the same as that in base_id)
    **	  {
    **	    we are done - for every i s.t. i-th bit is set in base_attrmap and
    **	    copy_map[i] is non-zero, set copy_map[i]-th bit in view_attrmap;
    **	  }
    **    else
    **	  {
    **	    get the description of the underlying table, switch copy_map and
    **	    cur_map pointers and do it all over again
    **	  }
    */

    /*
    ** get description of the view by id and initialize cur_map[] and copy_map
    ** to get the algorithm going
    */ 
    status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "!", PST_SHWID,
	(DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL, view_id,
	TRUE, &rngvar, PSQ_REVOKE, &err_blk);
    if (DB_FAILURE_MACRO(status))
    {
	return(status);
    }

    cur_map = attrnums;
    copy_map = attrnums + DB_MAX_COLS + 1;

    /*
    ** for every attribute of the view, initialize cur_map[attr_no] to attr_no;
    ** initialize remaining entries to 0
    */
    for (i = 0; i <= rngvar->pss_tabdesc->tbl_attr_count; i++)
	cur_map[i] = i;
    for (; i <= DB_MAX_COLS; i++)
	cur_map[i] = 0;
    
    for (i = 0; i <= DB_MAX_COLS; i++)
	copy_map[i] = 0;

    for (;;)	    /* something to break out of */
    {
	i4		err_code;
	i4		cur_attr;
	DB_TAB_ID	cur_id;
	i2		*tmp;
	PST_QNODE	*node;
	i4		empty_map = TRUE;
	PST_QTREE       *vtree;
	PST_RT_NODE     *vtree_root;

	if (~rngvar->pss_tabdesc->tbl_status_mask & DMT_VIEW)
	{
	    /*
	    ** if this is not a view, then something is very wrong - this should
	    ** never happen 
	    */
	    status = E_DB_ERROR;
	    break;
	}

	/* obtain the view tree */
	STRUCT_ASSIGN_MACRO(rngvar->pss_tabid, rdf_rb->rdr_tabid);
	rdf_cb->rdf_info_blk = rngvar->pss_rdrinfo;

	status = rdf_call(RDF_GETINFO, (PTR) rdf_cb);

        /*
        ** RDF may choose to allocate a new info block and return its address in
        ** rdf_info_blk - we need to copy it over to pss_rdrinfo to avoid memory
        ** leak and other assorted unpleasantries
        */
	rngvar->pss_rdrinfo = rdf_cb->rdf_info_blk;

	if (DB_FAILURE_MACRO(status))
	{
	    _VOID_ psf_rdf_error(RDF_GETINFO, &rdf_cb->rdf_error, &err_blk);
	    break;
	}

	/* Make vtree point to the view tree retrieved */
	vtree = ((PST_PROCEDURE *) rdf_cb->rdf_info_blk->
	    rdr_view->qry_root_node)->pst_stmts->pst_specific.pst_tree;

	vtree_root = &vtree->pst_qtree->pst_sym.pst_value.pst_s_root;
	
	/*
	** now we will walk the view tree and translate map of view attributes
	** which are of interest to us into a map of attributes of the
	** underlying relation
	*/
	
	/*
	** compute offset in the array of pst_rangetab entries of the id of the
	** table on which the view is based
	*/ 
	i = BTnext(-1, (char *) &vtree_root->pst_tvrm, PST_NUMVARS);

	/*
	** save the id of the table since we may need to use it after we UNFIX
	** the current RDF cache entry
	*/
	STRUCT_ASSIGN_MACRO(vtree->pst_rangetab[i]->pst_rngvar, cur_id);

	for (cur_attr = rngvar->pss_tabdesc->tbl_attr_count,
	     node = vtree->pst_qtree->pst_left;
	     
	     cur_attr > 0;
	     
	     cur_attr--, node = node->pst_left)
	{
	    if (cur_map[cur_attr])
	    {
		PST_SYMBOL	*sym = &node->pst_right->pst_sym;
		
		/*
		** only need to record the updateable attributes; avoid TID
		*/
		if (   sym->pst_type == PST_VAR
		    && sym->pst_value.pst_s_var.pst_atno.db_att_id != 0)
		{
		    /*
		    ** remember that at least one entry in copy_map is non-sero
		    */
		    empty_map = FALSE;
		    
		    copy_map[sym->pst_value.pst_s_var.pst_atno.db_att_id] =
			cur_map[cur_attr];
		}
	    }
	}

	/* we no longer need this entry - set it up to be reused */
	rngvar->pss_used = FALSE;
	QUremove((QUEUE *) rngvar);
	QUinsert((QUEUE *) rngvar, sess_cb->pss_auxrng.pss_qhead.q_prev);

	if (empty_map)
	{
	    /*
	    ** none of the view's attributes appear to be updateable - this 
	    ** should not happen very often (actually, I never expect it to
	    ** happen but if it does, simply zero out the view_attrmap and
	    ** leave the loop)
	    */
	    psy_fill_attmap(view_attrmap, ((i4) 0));
	    break;
	}

	if (   cur_id.db_tab_base  == base_id->db_tab_base
	    && cur_id.db_tab_index == base_id->db_tab_index)
	{
	    /*
	    ** this is the relation for which we've been looking - transfer
	    ** information from the copy_map into the view_attrmap and leave the
	    ** loop
	    */
	    psy_fill_attmap(view_attrmap, ((i4) 0));

	    for (cur_attr = 1; cur_attr <= DB_MAX_COLS; cur_attr++)
	    {
		if (   copy_map[cur_attr]
		    && BTtest(cur_attr, (char *) base_attrmap))
		{
		    BTset(copy_map[cur_attr], (char *) view_attrmap);
		}
	    }

	    break;
	}
		    
	/*
	** we need to continue the search - get the description of the next view
	*/
	status = pst_rgent(sess_cb, &sess_cb->pss_auxrng, -1, "!", PST_SHWID,
	    (DB_TAB_NAME *) NULL, (DB_TAB_OWN *) NULL, &cur_id,
	    TRUE, &rngvar, PSQ_REVOKE, &err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    unfix = FALSE;
	    break;
	}

	/*
	** copy_map now contains description of attributes of the original view
	** which corespind to the attributes of the relation to which rngvar
	** corresponds - swap cur_map and copy_map and initialize NEW copy_map
	*/
	tmp = copy_map;
	copy_map = cur_map;
	cur_map = tmp;

	for (i = 0; i <= DB_MAX_COLS; i++)
	    copy_map[i] = 0;
    }

    if (unfix)
    {
	DB_STATUS	unfix_status;

	rngvar->pss_used = FALSE;
	QUremove((QUEUE *) rngvar);
	QUinsert((QUEUE *) rngvar, sess_cb->pss_auxrng.pss_qhead.q_prev);

	rdf_rb->rdr_types_mask = rdf_rb->rdr_2types_mask = 0;

	unfix_status = rdf_call(RDF_UNFIX, (PTR) rdf_cb);
	rngvar->pss_rdrinfo = rdf_cb->rdf_info_blk;
	if (DB_FAILURE_MACRO(unfix_status))
	{
	    (VOID) psf_rdf_error(RDF_UNFIX, &rdf_cb->rdf_error, &err_blk);
	    if (unfix_status > status)
		status = unfix_status;
	}
    }

    return(status);
}
