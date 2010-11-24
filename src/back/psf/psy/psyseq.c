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
#include    <st.h>
#include    <iicommon.h>
#include    <usererror.h>
#include    <dbdbms.h>
#include    <tm.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <rdf.h>
#include    <scf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <sxf.h>
#include    <psyaudit.h>

/**
**
**  Name: psyseq.c - PSF Qrymod sequence processor
**
**  Description:
**	This module contains the sequence processor. This processor
**	handles the creation, modification and destruction of sequences.
**
**  Defines:
**	psy_csequence	- Create a sequence.
**	psy_dsequence	- Destroy a sequence.
**	psy_asequence	- Alter a sequence.
**	psy_gsequence	- Get sequence descrition from RDF.
**	psy_seqperm	- Check permissions for sequence use.
**
**  History:
**	8-mar-02 (inkdo01)
**	    Written for sequences. 
**      6-July-04 (zhahu02)
**          Updated psy_gsequence and psy_seqperm for a user granted for 
**          select next (INGSRV2894/b112605).
[@history_template@]...
**/


/*{
** Name: psy_csequence	- Create a sequence.
**
**  INTERNAL PSF call format: status = psy_cseq(&psy_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_CSEQ, &psy_cb);
**
** Description:
** 	This routine stores the definition of a user-defined sequence in  
**	the system table iisequence. No other structural
**	(tree) data is associated with the event, such as in other qrymod 
**	catalogs.
**
**	The actual work of storing the sequence is done by RDF, which uses QEU.
**	Note that the sequence name is not yet validated within the user scope
**	and QEU may return a "duplicate event" error.
**
** Inputs:
**      psy_cb
**	    .psy_tuple
**	       .psy_sequence		Sequence tuple to insert.
**		  .dbs_name		Name of sequence filled in by the grammar.
**		  .dbs_type		Sequence data type.
**		  .dbs_length		Size.
**		  .dbs_start		Start value.
**		  .dbs_incr		Increment value.
**		  .dbs_max		Maximum value.
**		  .dbs_min		Minimum value.
**		  .dbs_cache		Cache size.
**		  .dbs_flag		Various flag values.
**					Internal fields (create date, unique
**					sequence and text ids) are filled in just
**					before storage when the ids are
**					constructed. 
**					The owner of the sequence is filled in
**					this routine from pss_user before
**					storing.
**	sess_cb				Pointer to session control block.
**	    .pss_user			User/owner of the event.
**
** Outputs:
**      psy_cb
**	    .psy_error.err_code		Filled in if an error happens:
**		Generally, the error codes returned by RDF and QEU
**		qrymod processing routines are passed back to the caller
**		except where they are user errors (ie, sequence exists).
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	1. Stores sequence tuple in iisequence
**	   identifying the sequence.
**	2. The above will cause write locks to be held on the updated tables.
**
** History:
**	8-mar-02 (inkdo01)
**	    Written for sequence support.
**	13-Jul-2006 (kiria01)  b116297
**	    Regress fix for b112605 as it didn't work and broke security.
*/

DB_STATUS
psy_csequence(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
{
    RDF_CB		rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status, loc_status;
    i4		err_code;

    /* Assign user */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user,
			psy_cb->psy_tuple.psy_sequence.dbs_owner);

    /* zero out RDF_CB and init common elements */
    pst_rdfcb_init(&rdf_cb, sess_cb);

    rdf_rb->rdr_l_querytext	= 0;
    rdf_rb->rdr_querytext	= NULL; 

    rdf_rb->rdr_2types_mask  = RDR2_SEQUENCE;		/* Sequence definition */
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_qrytuple    = (PTR)&psy_cb->psy_tuple.psy_sequence; /* sequence tup */

    /* Create new sequence in iisequence */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);   
    if (status != E_DB_OK)
    {
	_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, &psy_cb->psy_error);
    } /* If RDF error */

    return (status);
} /* psy_csequence */

/*{
** Name: psy_dsequence - Destroy a sequence.
**
**  INTERNAL PSF call format: status = psy_dsequence(&psy_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_DSEQ, &psy_cb);
**
** Description:
**	This routine removes the definition of a sequence from the system table
**	iisequence.
**
**	The actual work of deleting the sequence is done by RDF and QEU.  Note
**	that the sequence name is not yet validated within the user scope and
**	QEU may return an "unknown sequence" error.
**
** Inputs:
**      psy_cb
**	    .psy_tuple
**	       .psy_sequence		Sequence tuple to delete.
**		  .dbs_name		Name of sequence filled in by the grammar.
**					The owner of the sequence is filled in
**					this routine from pss_user before
**					calling RDR_UPDATE.
**	sess_cb				Pointer to session control block.
**	     .pss_user			Current user/assumed sequence owner.
**
** Outputs:
**      psy_cb
**	    .psy_error.err_code		Filled in if an error happens:
**		    Generally, the error code returned by RDF and QEU qrymod
**		    processing routines are passed back to the caller.
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	1. Deletes sequence tuple from iisequence.
**	2. The above will cause write locks to be held on the updated tables.
**
** History:
**	8-mar-02 (inkdo01)
**	    Written for sequence support.
**	24-apr-03 (inkdo01)
**	    Remove override of sequence owner name.
*/

DB_STATUS
psy_dsequence(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    RDF_CB              rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status;

    /* Fill in the RDF request block */

    /* first zero it out and fill in common elements */
    pst_rdfcb_init(&rdf_cb, sess_cb);
	
    rdf_rb->rdr_2types_mask  = RDR2_SEQUENCE;		/*Sequence delettion */
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_qrytuple    = (PTR)&psy_cb->psy_tuple.psy_sequence; /* sequence tup */

    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);	/* Destroy sequence */
    if (status != E_DB_OK)
	_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, &psy_cb->psy_error);
    return (status);
} /* psy_dsequence */

/*{
** Name: psy_asequence	- Alter a sequence.
**
**  INTERNAL PSF call format: status = psy_asequence(&psy_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_CSEQ, &psy_cb);
**
** Description:
** 	This routine alters the definition of a user-defined sequence in  
**	the system table iisequence. No other structural
**	(tree) data is associated with the sequence, such as in other qrymod 
**	catalogs.
**
**	The existing iisequence row has already been merged with the values 
**	from the alter command and the resulting values are checked for consistency.
**	The replacement is then done by RDF, which uses QEU.
**
** Inputs:
**      psy_cb
**	    .psy_tuple
**	       .psy_sequence		Sequence tuple to insert.
**		  .dbs_name		Name of sequence filled in by the grammar.
**		  .dbs_start		Restart value.
**		  .dbs_incr		Increment value.
**		  .dbs_max		Maximum value.
**		  .dbs_min		Minimum value.
**		  .dbs_cache		Cache size.
**		  .dbs_flag		Various flag values.
**					Internal fields (modify date, unique
**					sequence and text ids) are filled in just
**					before storage when the ids are
**					constructed. 
**					The owner of the sequence is filled in
**					this routine from pss_user before
**					storing.
**	sess_cb				Pointer to session control block.
**	    .pss_user			User/owner of the event.
**
** Outputs:
**      psy_cb
**	    .psy_error.err_code		Filled in if an error happens:
**		Generally, the error codes returned by RDF and QEU
**		qrymod processing routines are passed back to the caller
**		except where they are user errors (ie, sequence exists).
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	1. Replaces sequence tuple in iisequence
**	   identifying the sequence.
**	2. The above will cause write locks to be held on the updated tables.
**
** History:
**	8-mar-02 (inkdo01)
**	    Cloned from psy_csequence for sequence support. 
*/

DB_STATUS
psy_asequence(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
{
    RDF_CB		rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    DB_IISEQUENCE	*newseqp;
    DB_STATUS		status, loc_status;
    i4		err_code;
    bool		ascincr;

    newseqp = &psy_cb->psy_tuple.psy_sequence;	/* update values */

    /* zero out RDF_CB and init common elements */
    pst_rdfcb_init(&rdf_cb, sess_cb);

    rdf_rb->rdr_l_querytext	= 0;
    rdf_rb->rdr_querytext	= NULL; 

    rdf_rb->rdr_2types_mask  = RDR2_SEQUENCE;		/* Sequence definition */
    rdf_rb->rdr_update_op   = RDR_REPLACE;
    rdf_rb->rdr_qrytuple    = (PTR)newseqp;		/* modified iisequence tuple */

    /* Update existing sequence in iisequence. */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);   
    if (status != E_DB_OK)
    {
	_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, &psy_cb->psy_error);
    } /* If RDF error */

    return (status);
} /* psy_asequence */

/*{
** Name: psy_gsequence	- Get description of a sequence from RDF.
**
** Description:
** 	This routine calls RDF to return the iisequence tuple for a sequence,
**	then moves certain columns into the PSS_SEQINFO structure for its caller.
**	It is used by psl to verify the existence of a sequence referenced in 
**	a sequence operation (next/current value) and return definition information.
**
** Inputs:
**	sess_cb				Pointer to session control block.
**	    .pss_user			User/owner of the event.
**	seq_own				Sequence owner (from syntax).
**	seq_name			Sequence name (from syntax).
**	seq_mask			flag field
**	    PSS_USRSEQ	    search for sequence owned by the current user
**	    PSS_DBASEQ	    search for sequence owned by the DBA
**				    (must not be set unless 
**				    (seq_mask & PSS_USRSEQ))
**	    PSS_INGSEQ	    search for sequence owned by $ingres
**                                  (must not be set unless
**                                  (seq_mask & PSS_USRSEQ))
**	    PSS_SEQ_BY_OWNER	    search for sequence owned by the specified 
**				    owner
**	    PSS_SEQ_BY_ID	    search for sequence by id
**		NOTE:		    PSS_USRSEQ <==> !PSS_SEQ_BY_OWNER
**				    PSS_SEQ_BY_ID ==> !PSS_SEQ_BY_OWNER
**				    PSS_SEQ_BY_ID ==> !PSS_USRSEQ
**	privs				Pointer to privilege bit map (if we're to
**					check privileges on this sequence) or NULL.
**	qmode				Query mode (for errors).
**	grant_all			Flag indicating GRANT ALL accompanied request.
**
** Outputs:
**	seq_info			Pointer to sequence information block
**	    .pss_seqname		Sequence name.
**	    .pss_owner			Sequence owner.
**	    .pss_seqid			Internal sequence ID.
**	    .pss_dt			Sequence value datatype.
**	    .pss_length			Sequence value length.
**	    .pss_secid			Sequence security ID.
**      err_blk
**	    .err_code			Filled in if an error happens:
**		E_US1914_SEQ_NOT_FOUND	  Cannot find sequence definition.
**		Generally, the error codes returned by RDF and QEU
**		qrymod processing routines are passed back to the caller
**		except where they are user errors (eg, sequence doesn't exist).
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	18-mar-02 (inkdo01)
**	    Cloned from psy_gevent for sequence support.
**	21-feb-03 (inkdo01)
**	    Add seq_mask to control iisequence retrieval.
**	24-apr-03 (inkdo01)
**	    Reset IISEQUENCE ptr for secondary calls to RDF.
**	24-apr-03 (inkdo01)
**	    Fix "not found" message for "drop sequence".
**      09-Mar-2010 (coomi01) b123351
**          Block users from dropping dba sequences.
**      29-Apr-2010 (coomi01) b123638
**          Backout the above change, then put in a test
**          to prevent delete dba's sequence by non dba.  
**          This allows non-dba to find sequence for 
**          updating.
**	15-Oct-2010 (kschendel) SIR 124544
**	    Update psl-command-string call.
*/
DB_STATUS
psy_gsequence(
	PSS_SESBLK      *sess_cb,
	DB_OWN_NAME	*seq_own,
	DB_NAME		*seq_name,
	i4		seq_mask,
	PSS_SEQINFO	*seq_info,
	DB_IISEQUENCE	*seqp,
	i4		*ret_flags,
	i4		*privs,
	i4		qmode,
	i4		grant_all,
	DB_ERROR	*err_blk)
{
    DB_STATUS		status;
    RDF_CB		rdf_seq;	/* RDF for sequence */
    DB_IISEQUENCE	seqtuple;	/* sequence tuple to retrieve into */
    i4		err_code;
    bool	leave_loop = TRUE;

    *ret_flags = 0;

    /* First retrieve sequence tuple from RDF */
    if (seqp == NULL)
	seqp = &seqtuple;

    /* zero out RDF_CB and init common elements */
    pst_rdfcb_init(&rdf_seq, sess_cb);

    /* init relevant elements */
    {
	rdf_seq.rdf_rb.rdr_types_mask = RDR_BY_NAME;
	rdf_seq.rdf_rb.rdr_2types_mask = RDR2_SEQUENCE;
	MEmove(sizeof(DB_NAME), (PTR) seq_name, ' ',
	    sizeof(DB_NAME), (PTR) &rdf_seq.rdf_rb.rdr_name.rdr_seqname);
	STRUCT_ASSIGN_MACRO((*seq_own), rdf_seq.rdf_rb.rdr_owner);
    }
    
    rdf_seq.rdf_rb.rdr_update_op = RDR_OPEN;
    rdf_seq.rdf_rb.rdr_qtuple_count = 1;
    rdf_seq.rdf_rb.rdr_qrytuple = (PTR) seqp;

    do     		/* something to break out of */
    {
	status = rdf_call(RDF_GETINFO, (PTR) &rdf_seq);

	/*
	** if caller specified sequence owner name, or
	**    the sequence was found, or
	**    an error other than "sequence not found" was encountered,
	**  bail out
	*/
	if (   seq_mask & PSS_SEQ_BY_OWNER
	    || status == E_DB_OK
	    || rdf_seq.rdf_error.err_code != E_RD0013_NO_TUPLE_FOUND
	   )
	    break;

	/*
	** if sequence was not found, and
	**    - caller requested that DBA's sequences be considered, and
	**    - user is not the DBA,
	**    - check we are not attempting to destroy the sequence.
	** check if the sequence is owned by the DBA
	*/
	if ((qmode != PSQ_DSEQUENCE) &&
	       seq_mask & PSS_DBASEQ
	    && MEcmp((PTR) &sess_cb->pss_dba.db_tab_own, 
		   (PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME))
	   )
	{
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_dba.db_tab_own,
				rdf_seq.rdf_rb.rdr_owner);
	    rdf_seq.rdf_rb.rdr_qtuple_count = 1;
	    rdf_seq.rdf_rb.rdr_qrytuple = (PTR) &seqtuple;
	    status = rdf_call(RDF_GETINFO, (PTR) &rdf_seq);
	    if (status == E_DB_OK)
		STRUCT_ASSIGN_MACRO(seqtuple, *seqp);
	}

	/*
	** if still not found, and
	**   - caller requested that sequences owned by $ingres be considered, and
	**   - user is not $ingres and
	**   - DBA is not $ingres,
	** check if the sequence is owned by $ingres
	*/
	if (   status != E_DB_OK
	    && rdf_seq.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND
	    && seq_mask & PSS_INGSEQ
	   )
	{
	    if (   MEcmp((PTR) &sess_cb->pss_user,
			 (PTR) sess_cb->pss_cat_owner,
			 sizeof(DB_OWN_NAME)) 
		&& MEcmp((PTR) &sess_cb->pss_dba.db_tab_own,
			 (PTR) sess_cb->pss_cat_owner,
			 sizeof(DB_OWN_NAME))
	       )
	    {
		STRUCT_ASSIGN_MACRO((*sess_cb->pss_cat_owner),
				    rdf_seq.rdf_rb.rdr_owner);
		rdf_seq.rdf_rb.rdr_qtuple_count = 1;
		rdf_seq.rdf_rb.rdr_qrytuple = (PTR) &seqtuple;
		status = rdf_call(RDF_GETINFO, (PTR) &rdf_seq);
		if (status == E_DB_OK)
		    STRUCT_ASSIGN_MACRO(seqtuple, *seqp);
	    }
	}

	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    if (status != E_DB_OK)
    {
	if (rdf_seq.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
	{
	    if (qmode == PSQ_ASEQUENCE || qmode == PSQ_DSEQUENCE)
	    {
		_VOID_ psf_error(6419L, 0L,
		    PSF_USERERR, &err_code, err_blk, 1,
		    psf_trmwhite(sizeof(*seq_name), 
		    (PTR)seq_name), (PTR)seq_name);
	    }
	    else /* must be DML currval/nextval request */
	    {
		char        qry[PSL_MAX_COMM_STRING];
		i4     qry_len;

		psl_command_string(qmode, sess_cb, qry, &qry_len);
		_VOID_ psf_error(6420L, 0L,
		    PSF_USERERR, &err_code, err_blk, 2,
		    qry_len, qry,
		    psf_trmwhite(sizeof(*seq_name), (char *) seq_name),
		    (PTR) seq_name);
	    }
	    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
		sess_cb->pss_dbp_flags |= PSS_MISSING_OBJ;

	    *ret_flags |= PSS_MISSING_SEQUENCE;
	    status = E_DB_OK;
	}
	else /* some other error */
	{
	    _VOID_ psf_rdf_error(RDF_GETINFO, &rdf_seq.rdf_error, err_blk);
	}

	return (status);
    } /* If RDF did not return the sequence tuple */

    STRUCT_ASSIGN_MACRO(rdf_seq.rdf_rb.rdr_owner, (*seq_own));
					/* copy back successful owner */

    if (seq_info != NULL)
    {
	/* fill in a sequence descriptor */
	STRUCT_ASSIGN_MACRO(seqp->dbs_name, seq_info->pss_seqname);
	STRUCT_ASSIGN_MACRO(seqp->dbs_owner, seq_info->pss_seqown);
	STRUCT_ASSIGN_MACRO(seqp->dbs_uniqueid, seq_info->pss_seqid);
	seq_info->pss_dt = seqp->dbs_type;
	seq_info->pss_length = seqp->dbs_length;
	seq_info->pss_prec = seqp->dbs_prec;
    }

    /*
    ** if we are parsing a dbproc and the sequence which we have just looked up
    ** is owned by the dbproc's owner, we will add the sequence to the dbproc's
    ** independent object list unless it has already been added.
    ** Note that only sequences owned by the current user will be included into
    ** the list of independent objects.
    **
    ** NOTE: we do not build independent object list for system-generated
    **	     dbprocs
    */
    if (   sess_cb->pss_dbp_flags & PSS_DBPROC
	&& ~sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED
        && !MEcmp((PTR) &sess_cb->pss_user, (PTR) &seqp->dbs_owner,
		  sizeof(sess_cb->pss_user))
       )
    {
	status = pst_add_1indepobj(sess_cb, &seqp->dbs_uniqueid,
	    PSQ_OBJTYPE_IS_SEQUENCE, (DB_DBP_NAME *) NULL,
	    &sess_cb->pss_indep_objs.psq_objs, sess_cb->pss_dependencies_stream,
	    err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}
    }
		  
    if (privs && *privs)
    {
	i4	    privs_to_find = *privs;

	status = psy_seqperm(&rdf_seq, &privs_to_find, sess_cb, seq_info, qmode,
	    grant_all, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    return (status);
	}
	else if (privs_to_find)
	{
	    if (grant_all && *privs != privs_to_find)
	    {
		/*
		** if we are processing GRANT ALL and psy_seqperm() has
		** determined that the user may grant some but not all
		** privileges on the sequence, reset the privilege map
		** accordingly
		*/
		*privs &= ~(privs_to_find & ~((i4) DB_GRANT_OPTION));
	    }
	    else
	    {
		*ret_flags |= PSS_INSUF_SEQ_PRIVS;
		return(E_DB_OK);
	    }
	} /* If no permission */
    }

    return(E_DB_OK);

}

/*{
** Name: psy_seqperm	- Check if the user/group/role has sequence permission.
**
** Description:
**	This routine retrieves all protection tuples applicable to a given
**	sequence until it establishes that the user posesses all privileges
**	specified in *privs (SUCCESS) or until there are no more left (FAILURE).
**	(The only privilege defined on sequences is NEXT - like SELECT)
**
**	The routine assumes that if the current user is the same as the sequence
**	owner, then no 	privileges need be checked.
**	
** Inputs:
**	rdf_cb				RDF CB that was used to extract the
**					original sequence.  Note that the RDF
**					operation type masks will be modified
**					in this routine
**					(rdr_types_mask will be set to
**					RDR_PROTECT|RDR2_SEQUENCE)
**	privs				ptr to a map of privileges to verify
**	    DB_NEXT			user must posess NEXT on sequence
**	    DB_GRANT_OPTION		privilege(s) must be grantable
**	sess_cb				Session CB with various values plus:
**	    .pss_user			User name.
**	    .pss_group			Group name.
**	    .pss_aplid			Application/role name.
**      seq_info			sequence descriptor
**	qmode				mode of the query
**	grant_all			TRUE if processing GRANT ALL;
**					FALSE otherwise
**
** Outputs:
**	privs				if non-zero, indicates which privileges
**					the user does not posess
**	err_blk.err_code		Filled in if an error happened:
**					  errors from RDF & other PSF utilities
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Failure
**	Exceptions:
**	    none
**	
** Side Effects:
**	1. Via RDF will also access iiprotect.
**
** History:
**	3-may-02 (inkdo01)
**	    Written for sequence support (cloned from psy_evperm).
**	24-apr-03 (inkdo01)
**	    Corrected "missing privilege" message.
*/
DB_STATUS
psy_seqperm(
	RDF_CB		*rdf_cb,
	i4		*privs,
	PSS_SESBLK      *sess_cb,
	PSS_SEQINFO	*seq_info,
	i4		qmode,
	i4		grant_all,
	DB_ERROR	*err_blk)
{
    RDR_RB	    	*rdf_rb = &rdf_cb->rdf_rb;	/* RDR request block */
#define	PSY_RDF_SEQ	5
    DB_PROTECTION	seqprots[PSY_RDF_SEQ];	/* Set of returned tuples */
    DB_PROTECTION   	*protup;	/* Single tuple within set */
    i4		     	tupcount;	/* Number of tuples in scan */
    DB_STATUS	     	status;
    i4			save_privs = *privs, privs_wgo = (i4) 0;
    PSQ_OBJPRIV         *seqpriv_element = (PSQ_OBJPRIV *) NULL;

				/* 
				** will be used to remember whether the 
				** <auth id> attempting to grant privilege(s)
				** him/herself possesses any privilege on the 
				** object; 
				** will start off pessimistic, but may be reset 
				** to TRUE if we find at least one permit 
				** granting ANY privilege to <auth id> 
				** attempting to grant the privilege or to 
				** PUBLIC
				*/
    bool		cant_access = TRUE;

    status = E_DB_OK;

    if (!MEcmp((PTR) &sess_cb->pss_user,
	    (PTR) &seq_info->pss_seqown, sizeof(sess_cb->pss_user))
       )
    {
	/* owner of the sequence posesses all privileges on it */
	*privs = (i4) 0;
    }

    if (!*privs)
    {
	/* no privileges to check or the sequence is owned by the current user */
	return(E_DB_OK);
    }

    /*
    ** rdr_rec_access_id must be set before we try to scan a privilege list
    ** since under some circumstances we may skip all the way to saw_the_perms:
    ** and seqperm_exit, and code under seqperm_exit: relies on rdr_rec_access_id
    ** being set to NULL as indication that the tuple stream has not been opened
    */
    rdf_rb->rdr_rec_access_id  = NULL;    

    /*
    ** if processing CREATE PROCEDURE, before scanning IIPROTECT to check if the
    ** user has the required privileges on the sequence, check if some of the
    ** privileges have already been checked.
    */
    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
    {
	bool			missing;

	status = psy_check_objprivs(sess_cb, privs, &seqpriv_element,
	    &sess_cb->pss_indep_objs.psq_objprivs, &missing,
	    &seq_info->pss_seqid, sess_cb->pss_dependencies_stream,
	    PSQ_OBJTYPE_IS_SEQUENCE, err_blk);
	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}
	else if (missing)
	{
	    if (sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK)
	    {
		/* dbproc is clearly ungrantable now */
		sess_cb->pss_dbp_flags &= ~PSS_DBPGRANT_OK;
	    }

	    if (sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
	    {
		/*
		** if we determine that a user lacks some privilege(s) by
		** scanning IIPROTECT, code under saw_the_perms expects
		** priv to contain a map of privileges which could not be
		** satisfied;
		** if parsing a dbproc to determine its grantability, required
		** privileges must be all grantable (so we OR DB_GRANT_OPTION
		** into *privs);
		** strictly speaking, it is unnecessary to OR DB_GRANT_OPTION
		** into *privs, but I prefer to ensure that saw_the_perms: code
		** does not have to figure out whether or not IIPROTECT was
		** scanned
		*/
		*privs |= DB_GRANT_OPTION;
	    }

	    goto saw_the_perms;
	}
	else if (!*privs)
	{
	    /*
	    ** we were able to determine that the user posesses required
	    ** privilege(s) based on information contained in independent
	    ** privilege lists
	    */
	    return(E_DB_OK);
	}
    }

    /*
    ** if we are parsing a dbproc to determine if its owner can grant permit on
    ** it, we definitely want all of the privileges WGO;
    ** when processing GRANT, caller is expected to set DB_GRANT_OPTION in
    ** *privs
    */

    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
    {
	if (sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
	{
	    /*
	    ** if parsing a dbproc to determine if it is grantable, user must be
	    ** able to grant required privileges
	    */
	    *privs |= DB_GRANT_OPTION;
	}
	else if (sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK)
	{
	    /*
	    ** if we are processing a definition of a dbproc which appears to be
	    ** grantable so far, we will check for existence of all required
	    ** privileges WGO to ensure that it is, indeed, grantable.
	    */
	    privs_wgo = *privs;
	}
    }
    
    /*
    ** Some RDF fields are already set in calling routine.  Permit tuples are
    ** extracted by unique "dummy" table id.  In the case of sequences there
    ** cannot be any caching of permissions (as there are no timestamps) so we
    ** pass in an iiprotect result tuple set (seqprots).
    */
    STRUCT_ASSIGN_MACRO(seq_info->pss_seqid, rdf_rb->rdr_tabid);
    rdf_rb->rdr_types_mask     = RDR_PROTECT;
    rdf_rb->rdr_2types_mask    = RDR2_SEQUENCE; 
    rdf_rb->rdr_instr          = RDF_NO_INSTR;
    rdf_rb->rdr_update_op      = RDR_OPEN;
    rdf_rb->rdr_qrymod_id      = 0;		/* Get all protect tuples */
    rdf_cb->rdf_error.err_code = 0;

    /* For each group of permits */
    while (rdf_cb->rdf_error.err_code == 0 && (*privs || privs_wgo))
    {
	/* Get a set at a time */
	rdf_rb->rdr_qtuple_count = PSY_RDF_SEQ;
	rdf_rb->rdr_qrytuple     = (PTR) seqprots;  /* Result block for tuples */
	status = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);
	rdf_rb->rdr_update_op = RDR_GETNEXT;
	if (status != E_DB_OK)
	{
	    switch(rdf_cb->rdf_error.err_code)
	    {
	      case E_RD0011_NO_MORE_ROWS:
		status = E_DB_OK;
		break;
	      case E_RD0013_NO_TUPLE_FOUND:
		status = E_DB_OK;
		continue;			/* Will stop outer loop */
	      default:
		_VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_cb->rdf_error, err_blk);
		goto seqperm_exit;
	    } /* End switch error */
	} /* If error */

	/* For each permit tuple */
	for (tupcount = 0, protup = seqprots;
	     tupcount < rdf_rb->rdr_qtuple_count;
	     tupcount++, protup++)
	{
	    bool	applies;
	    i4		privs_found;

	    /* check if the permit applies to this session */
	    status = psy_grantee_ok(sess_cb, protup, &applies, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto seqperm_exit;
	    }
	    else if (!applies)
	    {
		continue;
	    }

	    /*
	    ** remember that the <auth id> attempting to grant a privilege 
	    ** possesses some privilege on the sequence
	    */
	    if (qmode == PSQ_GRANT && cant_access)
	        cant_access = FALSE;

	    /*
	    ** if some of the required privileges have not been satisfied yet,
	    ** we will check for privileges in *privs; otherwise we will check
	    ** for privileges in (privs_wgo|DB_GRANT_OPTION)
	    */
	    privs_found = prochk((*privs) ? *privs
					  : (privs_wgo | DB_GRANT_OPTION),
		(i4 *) NULL, protup, sess_cb);

	    if (!privs_found)
		continue;

	    /* mark the operations as having been handled */
	    *privs &= ~privs_found;

	    /*
	    ** if the newly found privileges are WGO and we are looking for
	    ** all/some of them WGO, update the map of privileges WGO being
	    ** sought
	    */
	    if (protup->dbp_popset & DB_GRANT_OPTION && privs_found & privs_wgo)
	    {
		privs_wgo &= ~privs_found;
	    }

	    /*
	    ** check if all required privileges have been satisfied;
	    ** note that DB_GRANT_OPTION bit does not get turned off when we
	    ** find a tuple matching the required privileges since when we
	    ** process GRANT ON SEQUENCE, more than one privilege may be required
	    */
	    if (*privs && !(*privs & ~DB_GRANT_OPTION))
	    {
		*privs = (i4) 0;
	    }

	    if (!*privs && !privs_wgo)
	    {
		/*
		** all the required privileges have been satisfied and the
		** required privileges WGO (if any) have been satisfied as
		** well -  we don't need to examine any more tuples
		*/
		break;
	    }
	} /* For all tuples */
    } /* While there are RDF tuples */

saw_the_perms:
    if (   sess_cb->pss_dbp_flags & PSS_DBPGRANT_OK
	&& (privs_wgo || *privs & DB_GRANT_OPTION))
    {
	/*
	** we are processing a dbproc definition; until now the dbproc appeared
	** to be grantable, but the user lacks the required privileges WGO on
	** this sequence - mark the dbproc as non-grantable
	*/
	sess_cb->pss_dbp_flags &= ~PSS_DBPGRANT_OK;
    }

    /*
    ** if processing a dbproc, we will record the privileges which the current
    ** user posesses if
    **	    - user posesses all the required privileges or
    **	    - we were parsing the dbproc to determine if it is grantable (in
    **	      which case the current dbproc will be marked as ungrantable, but
    **	      the privilege information may come in handy when processing the
    **	      next dbproc mentioned in the same GRANT statement.)
    **
    ** NOTE: we do not build independent privilege list for system-generated
    **	     dbprocs
    */
    if (   sess_cb->pss_dbp_flags & PSS_DBPROC
	&& ~sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED
	&& (!*privs || sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK))
    {
	if (save_privs & ~*privs)
	{
	    if (seqpriv_element)
	    {
		seqpriv_element->psq_privmap |= (save_privs & ~*privs);
	    }
	    else
	    {
		/* create a new descriptor */

		status = psy_insert_objpriv(sess_cb, &seq_info->pss_seqid,
		    PSQ_OBJTYPE_IS_SEQUENCE, save_privs & ~*privs,
		    sess_cb->pss_dependencies_stream,
		    &sess_cb->pss_indep_objs.psq_objprivs, err_blk);

		if (DB_FAILURE_MACRO(status))
		    goto seqperm_exit;
	    }
	}
    }

    if (*privs)
    {
	char                buf[60];  /* buffer for missing privilege string */
	DB_NAME	    	    *seq_name = &seq_info->pss_seqname;
	i4		    err_code;

	psy_prvmap_to_str((qmode == PSQ_GRANT) ? *privs & ~DB_GRANT_OPTION
					       : *privs,
	    buf, sess_cb->pss_lang);

	if (sess_cb->pss_dbp_flags & PSS_DBPROC)
	    sess_cb->pss_dbp_flags |= PSS_MISSING_PRIV;
	    
	if (qmode == PSQ_GRANT)
	{
	    if (cant_access)
	    {
		_VOID_ psf_error(E_US088F_2191_INACCESSIBLE_EVENT, 0L, 
		    PSF_USERERR, &err_code, err_blk, 3,
		    sizeof("GRANT") - 1, "GRANT",
		    psf_trmwhite(sizeof(DB_OWN_NAME), 
			(char *) &seq_info->pss_seqown), 
		    &seq_info->pss_seqown,
		    psf_trmwhite(sizeof(DB_NAME),
			(char *) &seq_info->pss_seqname),
		    &seq_info->pss_seqname);
	    }
	    else if (!grant_all)
	    {
		/* privileges were specified explicitly */
		_VOID_ psf_error(E_PS0551_INSUF_PRIV_GRANT_OBJ, 0L, PSF_USERERR,
		    &err_code, err_blk, 3,
		    STlength(buf), buf,
		    psf_trmwhite(sizeof(DB_NAME), 
			(char *) &seq_info->pss_seqname),
		    &seq_info->pss_seqname,
		    psf_trmwhite(sizeof(DB_OWN_NAME), 
			(char *) &seq_info->pss_seqown),
		    &seq_info->pss_seqown);
	    }
	    else if (save_privs == *privs)
	    {
		/* user entered GRANT ALL [PRIVILEGES] */
		_VOID_ psf_error(E_PS0563_NOPRIV_ON_GRANT_EV, 0L, PSF_USERERR,
		    &err_code, err_blk, 2,
		    psf_trmwhite(sizeof(DB_NAME), 
			(char *) &seq_info->pss_seqname),
		    &seq_info->pss_seqname,
		    psf_trmwhite(sizeof(DB_OWN_NAME), 
			(char *) &seq_info->pss_seqown),
		    &seq_info->pss_seqown);
	    }
	}
	else if (   sess_cb->pss_dbp_flags & PSS_DBPROC
		 && sess_cb->pss_dbp_flags & PSS_0DBPGRANT_CHECK)
	{
	    /*
	    ** if we were processing a dbproc definition to determine if it
	    ** is grantable, record those of required privileges which the
	    ** current user does not posess
	    */

	    _VOID_ psf_error(E_PS0557_DBPGRANT_LACK_EVPRIV, 0L, PSF_USERERR,
		&err_code, err_blk, 4,
		psf_trmwhite(sizeof(DB_NAME), (char *) &seq_info->pss_seqname),
		&seq_info->pss_seqname,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &seq_info->pss_seqown),
		&seq_info->pss_seqown,
		psf_trmwhite(sizeof(DB_DBP_NAME), 
		    (char *) sess_cb->pss_dbpgrant_name),
		sess_cb->pss_dbpgrant_name,
		STlength(buf), buf);

	    status = psy_insert_objpriv(sess_cb, &seq_info->pss_seqid,
		PSQ_OBJTYPE_IS_SEQUENCE | PSQ_MISSING_PRIVILEGE,
		save_privs & *privs, sess_cb->pss_dependencies_stream,
		&sess_cb->pss_indep_objs.psq_objprivs, err_blk);
	}
	else
	{
	    char	*op;

	    op = "NEXT VALUE";	/* only one possible */

	    _VOID_ psf_error(E_PS0D3D_MISSING_SEQUENCE_PRIV, 0L, PSF_USERERR,
		&err_code, err_blk, 4, 
		STlength(op), op, 
		STlength(buf), buf,
		psf_trmwhite(sizeof(DB_NAME), 
		    (char *) &seq_info->pss_seqname),
		&seq_info->pss_seqname,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &seq_info->pss_seqown),
		&seq_info->pss_seqown);
	}
    }
	    
seqperm_exit:
    /* Close iiprotect system catalog */
    if (rdf_rb->rdr_rec_access_id != NULL)
    {
	DB_STATUS	stat;

	rdf_rb->rdr_update_op = RDR_CLOSE;
	stat = rdf_call(RDF_READTUPLES, (PTR) rdf_cb);
	if (DB_FAILURE_MACRO(stat))
	{
	    _VOID_ psf_rdf_error(RDF_READTUPLES, &rdf_cb->rdf_error, err_blk);

	    if (stat > status)
	    {
		status = stat;	
	    }
	}
    }
    /*
    ** If user does not have privs we need to audit that they failed to perform
    ** the action on the event.
    */
    if ( *privs && Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
    {
	i4		msg_id;
	i4		accessmask = SXF_A_FAIL;
	DB_ERROR	e_error;
	DB_STATUS	stat;

	/* determin action */
	switch (qmode)
	{
	    default: 
		accessmask |= SXF_A_RAISE;
		msg_id = I_SX2704_RAISE_DBEVENT;
		break;
	}
/*	stat = psy_secaudit(FALSE, sess_cb, 
		ev_info->pss_alert_name.dba_alert.db_name, 
		&ev_info->pss_alert_name.dba_owner,
		sizeof(ev_info->pss_alert_name.dba_alert.db_name),
		SXF_E_EVENT, msg_id, accessmask, 
		(PTR)&ev_info->pss_evsecid,
		DB_SECID_TYPE,
		&e_error);
	if (stat > status)
	    status = stat;  */
    }
    return(status);
} /* psy_seqperm */
