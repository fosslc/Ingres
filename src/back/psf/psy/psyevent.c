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
**  Name: psyevent.c - PSF Qrymod event processor
**
**  Description:
**	This module contains the event processor.  This processor handles
**	the creation and usa of events.
**
**  Defines:
**	psy_devent	- Define an event.
**	psy_kevent	- Destroy an event.
**	psy_gevent 	- Get events. 
**	psy_evperm	- Validate event permissions.
**	psy_evraise	- Raise a dbevent
**
**  History:
**	08-sep-89 (neil)
**	    Written for Terminator II/alerters.
**	25-oct-89 (neil)
**	    Added create/drop and permission functionality.
**	10-feb-92 (andre)
**	    made changes to the interface of psy_gevent() and psy_evperm() to
**	    facilitate their use with more statements (originally, they were
**	    used only by REGISTER, RAISE, and REMOVE, and the implementation was
**	    tailored to these 3 statements; now they will accomodate GRANT and,
**	    possibly, some other statements)
**	15-jul-92 (andre)
**	    change interface of psy_gevent() to make it possible to have GRANT
**	    ALL treated as "grant all grantable privileges".  mainly this
**	    involves passing address of a privilege map which could be changed
**	    if the user lacks some privilege + an indicator of whether GRANT ALL
**	    was specified.
**	08-dec-92 (anitap)
**	    Changed the order of #includes of header files for CREATE SCHEMA.
**	15-dec-92 (wolf) 
**	    ulf.h must be included before qsf.h
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	23-sep-93 (stephenb)
**	    Failure to perform actions on a event is auditable, add call to
**	    psy_secaudit() in psy_evperm() if we have a priv failure.
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping. And eliminate prototyping error.
**	26-nov-93 (robf)
**	    Added psy_evraise as a central point for raising dbevents within
**	    PSF. 
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout(),
**	    psy_check_objprivs(), psy_insert_objpriv().
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/


/*{
** Name: psy_devent	- Define an event.
**
**  INTERNAL PSF call format: status = psy_devent(&psy_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_DEVENT, &psy_cb);
**
** Description:
** 	This routine stores the definition of a user-defined event in the 
**	system table iievent and iiqrytext.  No other structural (tree)
**	data is associated with the event, such as in other qrymod catalogs.
**
**	The actual work of storing the event is done by RDF, which uses QEU.
**	Note that the event name is not yet validated within the user scope
**	and QEU may return a "duplicate event" error.
**
** Inputs:
**      psy_cb
**	    .psy_tuple
**	       .psy_event		Event tuple to insert.
**		  .dbe_name		Name of event filled in by the grammar.
**		  .dbe_type		DBE_T_ALERT
**					Internal fields (create date, unique
**					event and text ids) are filled in just
**					before storage when the ids are
**					constructed. 
**					The owner of the event is filled in
**					this routine from pss_user before
**					storing.
**          .psy_qrytext                Id of query text as stored in QSF.
**	sess_cb				Pointer to session control block.
**	    .pss_user			User/owner of the event.
**
** Outputs:
**      psy_cb
**	    .psy_error.err_code		Filled in if an error happens:
**		E_PS0D18_QSF_LOCK 	  Cannot lock query text.
**		E_PS0D19_QSF_INFO	  Cannot get information for text.
**		E_PS0D1A_QSF_DESTROY	  Could not destroy text.
**		Generally, the error codes returned by RDF and QEU
**		qrymod processing routines are passed back to the caller
**		except where they are user errors (ie, event exists).
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	1. Stores query text in iiqrytext and event tuple in iievent
**	   identifying the event.
**	2. The above will cause write locks to be held on the updated tables.
**
** History:
**	28-aug-89 (neil)
**	   Written for Terminator II/alerters.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
*/

DB_STATUS
psy_devent(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
{
    RDF_CB		rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    i4			textlen;		/* Length of query text */
    QSF_RCB		qsf_rb;
    DB_STATUS		status, loc_status;
    i4		err_code;

    /* Fill in QSF request to lock text */
    qsf_rb.qsf_type	= QSFRB_CB;
    qsf_rb.qsf_ascii_id	= QSFRB_ASCII_ID;
    qsf_rb.qsf_length	= sizeof(qsf_rb);
    qsf_rb.qsf_owner	= (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid	= sess_cb->pss_sessid;

    /*
    ** Get the query text from QSF.  QSF has stored the text 
    ** as a {nat, string} pair - maybe not be aligned.
    */
    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, qsf_rb.qsf_obj_id);
    status = qsf_call(QSO_INFO, &qsf_rb);
    if (status != E_DB_OK)
    {
	psf_error(E_PS0D19_QSF_INFO, qsf_rb.qsf_error.err_code,
		  PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	goto cleanup;
    }
    MEcopy((PTR)qsf_rb.qsf_root, sizeof(i4), (PTR)&textlen);
    /* Assign user */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user,
			psy_cb->psy_tuple.psy_event.dbe_owner);

    /* zero out RDF_CB and init common elements */
    pst_rdfcb_init(&rdf_cb, sess_cb);

    rdf_rb->rdr_l_querytext	= textlen;
    rdf_rb->rdr_querytext	= ((char *)qsf_rb.qsf_root) + sizeof(i4);

    rdf_rb->rdr_types_mask  = RDR_EVENT;		/* Event definition */
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_qrytuple    = (PTR)&psy_cb->psy_tuple.psy_event; /* Event tup */

    /* Create new event in iievent */
    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);   
    if (status != E_DB_OK)
    {
	_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, &psy_cb->psy_error);
	goto cleanup;
    } /* If RDF error */

cleanup:
    /* Destroy query text - lock it first */
    qsf_rb.qsf_lk_state	= QSO_EXLOCK;
    STRUCT_ASSIGN_MACRO(psy_cb->psy_qrytext, qsf_rb.qsf_obj_id);
    loc_status = qsf_call(QSO_LOCK, &qsf_rb);
    if (loc_status != E_DB_OK)
    {
	psf_error(E_PS0D18_QSF_LOCK, qsf_rb.qsf_error.err_code,
		  PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	if (loc_status > status)
	    status = loc_status;
    }
    loc_status = qsf_call(QSO_DESTROY, &qsf_rb);
    if (loc_status != E_DB_OK)
    {
	psf_error(E_PS0D1A_QSF_DESTROY, qsf_rb.qsf_error.err_code,
		  PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	if (loc_status > status)
	    status = loc_status;
    }
    return (status);
} /* psy_devent */

/*{
** Name: psy_kevent	- Destroy an event.
**
**  INTERNAL PSF call format: status = psy_kevent(&psy_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_KEVENT, &psy_cb);
**
** Description:
**	This routine removes the definition of an event from the system tables
**	iievent and iiqrytext.
**
**	The actual work of deleting the event is done by RDF and QEU.  Note
**	that the event name is not yet validated within the user scope and
**	QEU may return an "unknown event" error.
**
** Inputs:
**      psy_cb
**	    .psy_tuple
**	       .psy_event		Event tuple to delete.
**		  .dbe_name		Name of event filled in by the grammar.
**					The owner of the event is filled in
**					this routine from pss_user before
**					calling RDR_UPDATE.
**	sess_cb				Pointer to session control block.
**	     .pss_user			Current user/assumed event owner.
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
**	1. Deletes query text from iiqrytext and event tuple from iievent.
**	2. The above will cause write locks to be held on the updated tables.
**
** History:
**	28-aug-89 (neil)
**	   Written for Terminator II/alerters.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
*/

DB_STATUS
psy_kevent(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    RDF_CB              rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status;

    /* Assign user */
    STRUCT_ASSIGN_MACRO(sess_cb->pss_user,
			psy_cb->psy_tuple.psy_event.dbe_owner);

    /* Fill in the RDF request block */

    /* first zero it out and fill in common elements */
    pst_rdfcb_init(&rdf_cb, sess_cb);
	
    rdf_rb->rdr_types_mask  = RDR_EVENT;		/* Event deletion */
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_qrytuple    = (PTR)&psy_cb->psy_tuple.psy_event; /* Event tup */

    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);	/* Destroy event */
    if (status != E_DB_OK)
	_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, &psy_cb->psy_error);
    return (status);
} /* psy_kevent */

/*{
** Name: psy_gevent	- Get an event.
**
** Description:
**	This routine retrieves event information for the statements:
**		RAISE EVENT, REGISTER EVENT, REMOVE EVENT,
**		GRANT ON EVENT, REVOKE ON EVENT, DROP PERMIT ON EVENT
**	These statements guarantee that the named event exists before
**	continuing with the processing in the rest of the system.
**
**	The actual retrieval is done by RDF, but this routine validates that
**	the named event exists based on whether it was qualified or not.
**	If the event name was qualified (by owner) then only that event must 
**	exist.  Otherwise, the event name is first retrieved from the current
**	user's events and, if not found, then from the DBA's events.
**
**	Also, if the statement is RAISE or REGISTER then permission checking
**	is also done to confirm that the statement is permitted.  The checking
**	is done by calling psy_evperm.
**
**	Upon a successful event name check an initialized PST_STATEMENT with
**	event-specific data is returned to the caller.
**
** Inputs:
**	sess_cb		    	Session control block.  
**	event_own	    	Dbevent owner name (should be used only if 
**			    	ev_mask & PSS_EV_BY_OWNER).
**	event_name	    	Dbevent name if (ev_mask & PSS_EV_BY_OWNER)
**	ev_id			dbevent id if (ev_mask & PSS_EV_BY_ID)
**	ev_mask		    	flag field
**	    PSS_USREVENT	    search for dbevent owned by the current user
**	    PSS_DBAEVENT	    search for dbevent owned by the DBA
**				    (must not be set unless 
**				    (ev_mask & PSS_USREVENT))
**	    PSS_INGEVENT	    search for dbevent owned by $ingres
**                                  (must not be set unless
**                                  (ev_mask & PSS_USREVENT))
**	    PSS_EV_BY_OWNER	    search for dbevent owned by the specified 
**				    owner
**	    PSS_EV_BY_ID	    search for dbevent by id
**		NOTE:		    PSS_USREVENT <==> !PSS_EV_BY_OWNER
**				    PSS_EV_BY_ID ==> !PSS_EV_BY_OWNER
**				    PSS_EV_BY_ID ==> !PSS_USREVENT
**	privs				ptr to a map of privileges to verify
**	    0				no privilege checking is required
**	    DB_EVREGISTER		user must posess REGISTER on dbevent
**	    DB_EVRAISE			user must posess RAISE on dbevent
**	    DB_GRANT_OPTION		privilege(s) must be grantable
**	qmode			    mode of the query being parsed; one of
**				    PSQ_EVREGISTER, PSQ_EVDEREG, PSQ_EVRAISE, or
**				    PSQ_GRANT
**	grant_all		    TRUE if we are processing GRANT ALL;
**				    FALSE otherwise
**	    
** Outputs:
**	ev_info			    dbevent descriptor will be filled in 
**				    once we find the specified dbevent
**	    pss_alert_name	    dbevent name, owner, and database (the 
**				    last will be filled in only if user was 
**				    not found to lack a required privilege)
**	    pss_ev_id		    dbevent id
**	privs			    if (grant_all), map of privileges being
**				    granted could be modified to reflect
**				    privileges that the user may grant
**	ret_flags		    return useful info to the caller
**	    PSS_MISSING_EVENT	    dbevent was not found
**	    PSS_INSUF_EV_PRIVS	    user lacks some required privilege(s) o
**				    dbevent
**	err_blk
**	    err_code		    Filled in if an error happened:
**				    Errors from RDF & other PSF utilities
**	Returns:
**	    E_DB_OK		    Success
**	    E_DB_ERROR		    Failure
**	Exceptions:
**	    none
**	
** Side Effects:
**	1. RDF will read out tuples from iievent (these are not yet cached)
**	   and thus hold read locks on certain pages.  It is expected that
**	   RAISE will be issued from within database procedures (compiled
**	   once and executed many times) and the performance of RAISE is
**	   critical.
**	2. Via RDF may also access iiprotect.
**
** History:
**	28-aug-89 (neil)
**	    Written for Terminator II/alerters.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	08-feb-92 (andre)
**	    added ev_mask to the interface; this mask will be used to 
**	    communicate useful information to psy_gevent(); initially it will be
**	    used to indicate whose dbevents need to be looked up
**	    added ret_flags; it will enable the caller to distinguish between 
**	    "expected" error such as dbevent does not exist or the user lacks a 
**	    required privilege on it and other "unexpected" errors
**	11-feb-91 (andre)
**	    call pst_add_1indepobj() if parsing a dbproc and the dbevent is
**	    owned by the dbproc's owner.  pst_add_1indepobj() will scan 
**	    independent object sublist looking for a descriptor for this dbevent
**	    and, if none was found, a new descriptor will be inserted
**	08-apr-92 (andre)
**	    add privs to the interface.  this will contain a map of privileges
**	    to be checked (if any).
**	12-may-92 (Andre)
**	    when RDF returns E_RD0013_NO_TUPLE_FOUND, status is set to
**	    E_DB_WARN, so to detect whether this conditions occurred, we must
**	    check whether
**	    (status != E_DB_OK && err_code == E_RD0013_NO_TUPLE_FOUND)
**	30-jun-92 (andre)
**	    added ability to retrieve dbevent info by id
**	15-jul-92 (andre)
**	    change interface of psy_gevent() to make it possible to have GRANT
**	    ALL treated as "grant all grantable privileges".  mainly this
**	    involves passing address of a privilege map which could be changed
**	    if the user lacks some privilege + an indicator of whether GRANT ALL
**	    was specified.
**	12-apr-93 (andre)
**	    do not add dbevents to the independent object list if parsing a
**	    system-generated dbproc
**	03-apr-93 (ralph)
**	    DELIM_IDENT: Use pss_cat_owner instead of "$ingres"
**	10-jul-93 (andre)
**	    cast args to MEcmp() to agree with the prototype
**      03-nov-93 (andre)
**          if a user specified name of the schema and the dbevent was not 
**	    found, return a new "vague" message - 
**	    E_US088F_2191_INACCESSIBLE_EVENT - saying that either the dbevent 
**	    did not exist or the user lacked required privileges on it - this 
**	    is done to prevent an unauthorized user from deducing any 
**	    information about a dbevent on which he possesses no privileges
**	30-nov-93 (robf)
**          Allow for access when creating a security alarm.
**	30-dec-93 (robf)
**          Audit Grant/Revoke on Dbevent as CONTROL operations, not
**	    CREATE/DROP, which are for creating/dropping the actual event.
*/
DB_STATUS
psy_gevent(
	PSS_SESBLK      *sess_cb,
	DB_OWN_NAME	*event_own,
	DB_TAB_NAME	*event_name,
	DB_TAB_ID	*ev_id,
	i4		ev_mask,
	PSS_EVINFO	*ev_info,
	i4		*ret_flags,
	i4		*privs,
	i4		qmode,
	i4		grant_all,
	DB_ERROR	*err_blk)
{
    DB_STATUS		status;
    RDF_CB		rdf_ev;		/* RDF for event */
    DB_IIEVENT		evtuple;	/* Event tuple to retrieve into */
    SCF_CB		scf_cb;		/* Get the database name */
    SCF_SCI		sci_list[1];
    i4		err_code;
    bool		leave_loop = TRUE;
    SXF_ACCESS	        access_type;

    *ret_flags = 0;

    /* First retrieve event tuple from RDF */

    /* zero out RDF_CB and init common elements */
    pst_rdfcb_init(&rdf_ev, sess_cb);

    /* init relevant elements */
    if (ev_mask & PSS_EV_BY_ID)
    {
	STRUCT_ASSIGN_MACRO((*ev_id), rdf_ev.rdf_rb.rdr_tabid);
	rdf_ev.rdf_rb.rdr_types_mask = RDR_EVENT;
    }
    else
    {
	rdf_ev.rdf_rb.rdr_types_mask = RDR_EVENT | RDR_BY_NAME;
	MEmove(sizeof(DB_TAB_NAME), (PTR) event_name, ' ',
	    DB_EVENT_MAXNAME, (PTR) &rdf_ev.rdf_rb.rdr_name.rdr_evname);

	if (ev_mask & PSS_USREVENT)
	{
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_user, rdf_ev.rdf_rb.rdr_owner);
	}
	else				/* must be PSS_EV_BY_OWNER */
	{
	    STRUCT_ASSIGN_MACRO((*event_own), rdf_ev.rdf_rb.rdr_owner);
	}
    }
    
    rdf_ev.rdf_rb.rdr_update_op = RDR_OPEN;
    rdf_ev.rdf_rb.rdr_qtuple_count = 1;
    rdf_ev.rdf_rb.rdr_qrytuple = (PTR) &evtuple;

    do     		/* something to break out of */
    {
	status = rdf_call(RDF_GETINFO, (PTR) &rdf_ev);

	/*
	** if caller specified dbevent id, or
	**    caller specified dbevent owner name, or
	**    the dbevent was found, or
	**    an error other than "dbevent not found" was encountered,
	**  bail out
	*/
	if (   ev_mask & (PSS_EV_BY_ID | PSS_EV_BY_OWNER)
	    || status == E_DB_OK
	    || rdf_ev.rdf_error.err_code != E_RD0013_NO_TUPLE_FOUND
	   )
	    break;

	/*
	** if
	**	    - dbevent was not found, and
	**	    - caller requested that DBA's dbevents be considered, and
	**      - user is not the DBA,
	** check if the dbevent is owned by the DBA
	*/
	if (   ev_mask & PSS_DBAEVENT
	    && MEcmp((PTR) &sess_cb->pss_dba.db_tab_own, 
		   (PTR) &sess_cb->pss_user, sizeof(DB_OWN_NAME))
	   )
	{
	    STRUCT_ASSIGN_MACRO(sess_cb->pss_dba.db_tab_own,
				rdf_ev.rdf_rb.rdr_owner);
	    rdf_ev.rdf_rb.rdr_qtuple_count = 1;
	    rdf_ev.rdf_rb.rdr_qrytuple = (PTR) &evtuple;
	    status = rdf_call(RDF_GETINFO, (PTR) &rdf_ev);
	}

	/*
	** if
	**     - still not found, and
	**	   - caller requested that dbevents owned by $ingres be considered, and
	**	   - user is not $ingres and
	**	   - DBA is not $ingres,
	** check if the dbevent is owned by $ingres
	*/
	if (   status != E_DB_OK
	    && rdf_ev.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND
	    && ev_mask & PSS_INGEVENT
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
				    rdf_ev.rdf_rb.rdr_owner);
		rdf_ev.rdf_rb.rdr_qtuple_count = 1;
		rdf_ev.rdf_rb.rdr_qrytuple = (PTR) &evtuple;
		status = rdf_call(RDF_GETINFO, (PTR) &rdf_ev);
	    }
	}

	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    if (status != E_DB_OK)
    {
	if (rdf_ev.rdf_error.err_code == E_RD0013_NO_TUPLE_FOUND)
	{
	    char        qry[PSL_MAX_COMM_STRING];
	    i4     qry_len;

	    psl_command_string(qmode, sess_cb->pss_lang, qry, &qry_len);

	    if (ev_mask & PSS_EV_BY_OWNER)
	    {
		_VOID_ psf_error(E_US088F_2191_INACCESSIBLE_EVENT, 0L, 
		    PSF_USERERR, &err_code, err_blk, 3,
		    qry_len, qry,
		    psf_trmwhite(sizeof(*event_own), (char *) event_own),
		    (PTR) event_own,
		    psf_trmwhite(sizeof(*event_name), (char *) event_name),
		    (PTR) event_name);
	    }
	    else
	    {
	        char        ev_name[DB_EVENT_MAXNAME], *evname_p;
	        i4	    evname_len;

	        if (ev_mask & PSS_EV_BY_ID)
	        {
		    STprintf(ev_name, "(%d,%d)", ev_id->db_tab_base,
		        ev_id->db_tab_index);
		    evname_p = ev_name;
		    evname_len = STlength(ev_name);
	        }
	        else
	        {
		    evname_p = (char *) event_name;
		    evname_len = psf_trmwhite(sizeof(*event_name), evname_p);
	        }
	    
	        _VOID_ psf_error(E_PS0D3B_DBEVENT_NOT_FOUND, 0L, PSF_USERERR,
		    &err_code, err_blk, 2,
		    qry_len, qry,
		    evname_len, (PTR) evname_p);
	    }

	    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
		sess_cb->pss_dbp_flags |= PSS_MISSING_OBJ;

	    *ret_flags |= PSS_MISSING_EVENT;
	    status = E_DB_OK;
	}
	else
	{
	    _VOID_ psf_rdf_error(RDF_GETINFO, &rdf_ev.rdf_error, err_blk);
	}

	return (status);
    } /* If RDF did not return the event tuple */

    /* fill in a dbevent descriptor */
    STRUCT_ASSIGN_MACRO(evtuple.dbe_name, ev_info->pss_alert_name.dba_alert);
    STRUCT_ASSIGN_MACRO(evtuple.dbe_owner, ev_info->pss_alert_name.dba_owner);
    STRUCT_ASSIGN_MACRO(evtuple.dbe_uniqueid, ev_info->pss_ev_id);

    /*
    ** if we are parsing a dbproc and the dbevent which we have just looked up
    ** is owned by the dbproc's owner, we will add the dbevent to the dbproc's
    ** independent object list unless it has already been added.
    ** Note that only dbevents owned by the current user will be included into
    ** the list of independent objects.
    **
    ** NOTE: we do not build independent object list for system-generated
    **	     dbprocs
    */
    if (   sess_cb->pss_dbp_flags & PSS_DBPROC
	&& ~sess_cb->pss_dbp_flags & PSS_SYSTEM_GENERATED
        && !MEcmp((PTR) &sess_cb->pss_user, (PTR) &evtuple.dbe_owner,
		  sizeof(sess_cb->pss_user))
       )
    {
	status = pst_add_1indepobj(sess_cb, &evtuple.dbe_uniqueid,
	    PSQ_OBJTYPE_IS_DBEVENT, (DB_DBP_NAME *) NULL,
	    &sess_cb->pss_indep_objs.psq_objs, sess_cb->pss_dependencies_stream,
	    err_blk);

	if (DB_FAILURE_MACRO(status))
	{
	    return(status);
	}
    }
		  
    if (*privs)
    {
	i4	    privs_to_find = *privs;

	status = psy_evperm(&rdf_ev, &privs_to_find, sess_cb, ev_info, qmode,
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
		** if we are processing GRANT ALL and psy_evperm() has
		** determined that the user may grant some but not all
		** privileges on the dbevent, reset the privilege map
		** accordingly
		*/
		*privs &= ~(privs_to_find & ~((i4) DB_GRANT_OPTION));
	    }
	    else
	    {
		*ret_flags |= PSS_INSUF_EV_PRIVS;
		return(E_DB_OK);
	    }
	} /* If no permission */
    }

    /* Get the database name */
    scf_cb.scf_length	= sizeof(SCF_CB);
    scf_cb.scf_type	= SCF_CB_TYPE;
    scf_cb.scf_facility	= DB_PSF_ID;
    scf_cb.scf_session	= sess_cb->pss_sessid;
    scf_cb.scf_ptr_union.scf_sci   = (SCI_LIST *) sci_list;
    scf_cb.scf_len_union.scf_ilength = 1;
    sci_list[0].sci_length  = sizeof(DB_DB_NAME);
    sci_list[0].sci_code    = SCI_DBNAME;
    sci_list[0].sci_aresult = (char *) &ev_info->pss_alert_name.dba_dbname;
    sci_list[0].sci_rlength = NULL;
    status = scf_call(SCU_INFORMATION, &scf_cb);
    if (status != E_DB_OK)
    {
	_VOID_ psf_error(E_PS0D13_SCU_INFO_ERR, scf_cb.scf_error.err_code,
			 PSF_INTERR, &err_code, err_blk, 0);
	status = E_DB_ERROR;
    }
    return (status);
} /* psy_gevent */

/*{
** Name: psy_evperm	- Check if the user/group/role has event permission.
**
** Description:
**	This routine retrieves all protection tuples applicable to a given
**	event until it establishes that the user posesses all privileges
**	specified in *privs (SUCCESS) or until there are no more left (FAILURE).
**
**	The routine assumes that if the current user is the same as the event
**	owner, then no 	privileges need be checked.
**	
** Inputs:
**	rdf_cb				RDF CB that was used to extract the
**					original event.  Note that the RDF
**					operation type masks will be modified
**					in this routine
**					(rdr_types_mask will be set to
**					RDR_PROTECT|RDR_EVENT).
**	privs				ptr to a map of privileges to verify
**	    DB_EVREGISTER		user must posess REGISTER on dbevent
**	    DB_EVRAISE			user must posess RAISE on dbevent
**	    DB_GRANT_OPTION		privilege(s) must be grantable
**	sess_cb				Session CB with various values plus:
**	    .pss_user			User name.
**	    .pss_group			Group name.
**	    .pss_aplid			Application/role name.
**      ev_info				dbevent descriptor
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
**	28-aug-89 (neil)
**	    Written for Terminator II/alerters.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	09-feb-92 (andre)
**	    removed psq_cb, added privs and err_blk to the interface; replaced 
**	    evtuple with ev_info
**	12-feb-92 (andre)
**	    removed error_perm made privs an (i4 *).  If the user lacks some
**	    privileges, *privs will describe them and the function will return
**	    E_DB_OK since this is one of expected conditions
**	08-apr-92 (andre)
**	    returned rdf_cb to the function's interface - we are back to the
**	    case where psy_evperm() can only be called by psy_gevent().  If I
**	    understand it correctly, strictly speaking this is not necessary
**	    since dbevent info is not cached; however, in the event (ha-ha) that
**	    we start caching it, RDF likes to have the object descriptor in its
**	    cache before it starts looking up permits on it.
**	15-jul-92 (andre)
**	    added grant_all.  when set, it indicates that we are processing
**	    GRANT ALL [PRIVILEGES] and we only should report an error if the
**	    user may not grant any of specified privileges
**	10-Feb-93 (Teresa)
**	    Changed RDF_GETINFO to RDF_READTUPLES for new RDF I/F
**	13-apr-93 (andre)
**	    do not add privileges on dbevents to the independent privilege list
**	    if parsing a system-generated dbproc
**	23-sep-93 (stephenb)
**	    Failure to perform actions on a event is auditable, add call to
**	    psy_secaudit() if we have a priv failure.
**      02-nov-93 (andre)
**          if the user attempting to grant a privilege on a dbevent X possesses
**          no privileges whatsoever on X, rather than issuing a message saying
**          that he cannot grant some privilege(s) on X, we will issue a message
**          saying that EITHER X does not exist OR the user may not access it.
*/
DB_STATUS
psy_evperm(
	RDF_CB		*rdf_cb,
	i4		*privs,
	PSS_SESBLK      *sess_cb,
	PSS_EVINFO	*ev_info,
	i4		qmode,
	i4		grant_all,
	DB_ERROR	*err_blk)
{
    RDR_RB	    	*rdf_rb = &rdf_cb->rdf_rb;	/* RDR request block */
#define	PSY_RDF_EVENT	5
    DB_PROTECTION	evprots[PSY_RDF_EVENT];	/* Set of returned tuples */
    DB_PROTECTION   	*protup;	/* Single tuple within set */
    i4		     	tupcount;	/* Number of tuples in scan */
    DB_STATUS	     	status;
    i4			save_privs = *privs, privs_wgo = (i4) 0;
    PSQ_OBJPRIV         *evpriv_element = (PSQ_OBJPRIV *) NULL;

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
	    (PTR) &ev_info->pss_alert_name.dba_owner, sizeof(sess_cb->pss_user))
       )
    {
	/* owner of the dbevent posesses all privileges on it */
	*privs = (i4) 0;
    }

    if (!*privs)
    {
	/* no privileges to check or the dbevent is owned by the current user */
	return(E_DB_OK);
    }

    /*
    ** rdr_rec_access_id must be set before we try to scan a privilege list
    ** since under some circumstances we may skip all the way to saw_the_perms:
    ** and evperm_exit, and code under evperm_exit: relies on rdr_rec_access_id
    ** being set to NULL as indication that the tuple stream has not been opened
    */
    rdf_rb->rdr_rec_access_id  = NULL;    

    /*
    ** if processing CREATE PROCEDURE, before scanning IIPROTECT to check if the
    ** user has the required privileges on the dbevent, check if some of the
    ** privileges have already been checked.
    */
    if (sess_cb->pss_dbp_flags & PSS_DBPROC)
    {
	bool			missing;

	status = psy_check_objprivs(sess_cb, privs, &evpriv_element,
	    &sess_cb->pss_indep_objs.psq_objprivs, &missing,
	    &ev_info->pss_ev_id, sess_cb->pss_dependencies_stream,
	    PSQ_OBJTYPE_IS_DBEVENT, err_blk);
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
    ** extracted by unique "dummy" table id.  In the case of dbevents there
    ** cannot be any caching of permissions (as there are no timestamps) so we
    ** pass in an iiprotect result tuple set (evprots).
    */
    STRUCT_ASSIGN_MACRO(ev_info->pss_ev_id, rdf_rb->rdr_tabid);
    rdf_rb->rdr_types_mask     = RDR_PROTECT | RDR_EVENT;
    rdf_rb->rdr_2types_mask    = (RDF_TYPES) 0;
    rdf_rb->rdr_instr          = RDF_NO_INSTR;
    rdf_rb->rdr_update_op      = RDR_OPEN;
    rdf_rb->rdr_qrymod_id      = 0;		/* Get all protect tuples */
    rdf_cb->rdf_error.err_code = 0;

    /* For each group of permits */
    while (rdf_cb->rdf_error.err_code == 0 && (*privs || privs_wgo))
    {
	/* Get a set at a time */
	rdf_rb->rdr_qtuple_count = PSY_RDF_EVENT;
	rdf_rb->rdr_qrytuple     = (PTR) evprots;  /* Result block for tuples */
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
		goto evperm_exit;
	    } /* End switch error */
	} /* If error */

	/* For each permit tuple */
	for (tupcount = 0, protup = evprots;
	     tupcount < rdf_rb->rdr_qtuple_count;
	     tupcount++, protup++)
	{
	    bool	applies;
	    i4		privs_found;

	    /* check if the permit applies to this session */
	    status = psy_grantee_ok(sess_cb, protup, &applies, err_blk);
	    if (DB_FAILURE_MACRO(status))
	    {
		goto evperm_exit;
	    }
	    else if (!applies)
	    {
		continue;
	    }

	    /*
	    ** remember that the <auth id> attempting to grant a privilege 
	    ** possesses some privilege on the dbevent
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
	    ** process GRANT ON DBEVENT, more than one privilege may be required
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
	** this dbevent - mark the dbproc as non-grantable
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
	    if (evpriv_element)
	    {
		evpriv_element->psq_privmap |= (save_privs & ~*privs);
	    }
	    else
	    {
		/* create a new descriptor */

		status = psy_insert_objpriv(sess_cb, &ev_info->pss_ev_id,
		    PSQ_OBJTYPE_IS_DBEVENT, save_privs & ~*privs,
		    sess_cb->pss_dependencies_stream,
		    &sess_cb->pss_indep_objs.psq_objprivs, err_blk);

		if (DB_FAILURE_MACRO(status))
		    goto evperm_exit;
	    }
	}
    }

    if (*privs)
    {
	char                buf[60];  /* buffer for missing privilege string */
	DB_ALERT_NAME	    *alert_name = &ev_info->pss_alert_name;
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
			(char *) &alert_name->dba_owner),
		    &alert_name->dba_owner,
		    psf_trmwhite(sizeof(alert_name->dba_alert),
			(char *) &alert_name->dba_alert),
		    &alert_name->dba_alert);
	    }
	    else if (!grant_all)
	    {
		/* privileges were specified explicitly */
		_VOID_ psf_error(E_PS0551_INSUF_PRIV_GRANT_OBJ, 0L, PSF_USERERR,
		    &err_code, err_blk, 3,
		    STlength(buf), buf,
		    psf_trmwhite(sizeof(alert_name->dba_alert), 
			(char *) &alert_name->dba_alert),
		    &alert_name->dba_alert,
		    psf_trmwhite(sizeof(DB_OWN_NAME), 
			(char *) &alert_name->dba_owner),
		    &alert_name->dba_owner);
	    }
	    else if (save_privs == *privs)
	    {
		/* user entered GRANT ALL [PRIVILEGES] */
		_VOID_ psf_error(E_PS0563_NOPRIV_ON_GRANT_EV, 0L, PSF_USERERR,
		    &err_code, err_blk, 2,
		    psf_trmwhite(sizeof(alert_name->dba_alert), 
			(char *) &alert_name->dba_alert),
		    &alert_name->dba_alert,
		    psf_trmwhite(sizeof(DB_OWN_NAME), 
			(char *) &alert_name->dba_owner),
		    &alert_name->dba_owner);
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
		psf_trmwhite(sizeof(alert_name->dba_alert),
		    (char *) &alert_name->dba_alert),
		&alert_name->dba_alert,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &alert_name->dba_owner),
		&alert_name->dba_owner,
		psf_trmwhite(sizeof(DB_DBP_NAME), 
		    (char *) sess_cb->pss_dbpgrant_name),
		sess_cb->pss_dbpgrant_name,
		STlength(buf), buf);

	    status = psy_insert_objpriv(sess_cb, &ev_info->pss_ev_id,
		PSQ_OBJTYPE_IS_DBEVENT | PSQ_MISSING_PRIVILEGE,
		save_privs & *privs, sess_cb->pss_dependencies_stream,
		&sess_cb->pss_indep_objs.psq_objprivs, err_blk);
	}
	else
	{
	    char	*op;

	    switch (qmode)
	    {
		case PSQ_EVREGISTER:
		    op = "REGISTER";
		    break;
		case PSQ_EVRAISE:
		    op = "RAISE";
		    break;
		default:
		    op = "UNKNOWN";
		    break;
	    }

	    /* qmode in (PSQ_EVREGISTER, PSQ_EVRAISE) */
	    _VOID_ psf_error(E_PS0D3C_MISSING_DBEVENT_PRIV, 0L, PSF_USERERR,
		&err_code, err_blk, 4, 
		STlength(op), op, 
		STlength(buf), buf,
		psf_trmwhite(sizeof(alert_name->dba_alert), 
		    (char *) &alert_name->dba_alert),
		&alert_name->dba_alert,
		psf_trmwhite(sizeof(DB_OWN_NAME), 
		    (char *) &alert_name->dba_owner),
		&alert_name->dba_owner);
	}
    }
	    
evperm_exit:
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
	    case PSQ_EVREGISTER: /* register event */
		accessmask |= SXF_A_REGISTER;
		msg_id = I_SX2705_REGISTER_DBEVENT;
		break;
	    case PSQ_EVDEREG: /* remove event */
		accessmask |= SXF_A_REMOVE;
		msg_id = I_SX2706_REMOVE_DBEVENT;
		break;
	    case PSQ_GRANT: /* grant permit */
		accessmask |= SXF_A_CONTROL;
		msg_id = I_SX2030_PROT_EV_CREATE;
		break;
	    case PSQ_REVOKE: /* revoke permit */
		accessmask |= SXF_A_CONTROL;
		msg_id = I_SX2031_PROT_EV_DROP;
		break;
	    case PSQ_EVDROP: /* drop event */
		accessmask |= SXF_A_DROP;
		msg_id = I_SX203C_EVENT_DROP;
		break;
	    case PSQ_EVRAISE: /* raise event */
	    default: 
		accessmask |= SXF_A_RAISE;
		msg_id = I_SX2704_RAISE_DBEVENT;
		break;
	}
	stat = psy_secaudit(FALSE, sess_cb, 
		ev_info->pss_alert_name.dba_alert.db_ev_name, 
		&ev_info->pss_alert_name.dba_owner,
		sizeof(ev_info->pss_alert_name.dba_alert.db_ev_name),
		SXF_E_EVENT, msg_id, accessmask, 
		&e_error);
	if (stat > status)
	    status = stat;
    }
    return(status);
} /* psy_evperm */

/*
** Name: psy_evraise
**
** Description:
**	Raise a dbevent. This causes the indicated event to be raised.
**	Currently this is needed to raise events associated with 
**	security alarms which get fire on failure (i.e DAC/MAC fails in PSF
**	so the query doesn't complete but we still need to fire the alarms
**	hence do it here
**
** Inputs:
**	sess_cb	- Session CB
**	
**	evname  - Event name 
**
**      evowner - Event owner
**
**	evtext  - Event text
**      ev_l_text - Length of text 
**
** Outputs
**	errblk  - Error block
**
** History:
**	26-nov-93 (robf)
**         Created
**	10-Jan-2001 (jenjo02)
**	   Removed call to SCU_INFORMATION to locate QEF_CB;
**	   qef_call() takes care of this itself.
*/
DB_STATUS
psy_evraise(
	PSS_SESBLK *sess_cb,
	DB_EVENT_NAME *evname,
	DB_OWN_NAME *evowner,
	char	    *evtext,
	i4	    ev_l_text,
        DB_ERROR    *err_blk
)
{
    QEF_RCB	    qef_rcb;
    
    qef_rcb.qef_length	= sizeof(QEF_RCB);
    qef_rcb.qef_type	= QEFRCB_CB;
    qef_rcb.qef_ascii_id	= QEFRCB_ASCII_ID;
    qef_rcb.qef_sess_id	= sess_cb->pss_sessid;
    qef_rcb.qef_eflag	= QEF_EXTERNAL;
    qef_rcb.qef_evname  = evname;
    qef_rcb.qef_evowner = evowner;
    qef_rcb.qef_evtext  = evtext;
    qef_rcb.qef_ev_l_text=ev_l_text;

    if (qef_call(QEU_RAISE_EVENT, ( PTR ) &qef_rcb) != E_DB_OK)
    {
	(VOID) psf_error(E_QE0210_QEC_ALTER, 0L, PSF_INTERR,
			 &qef_rcb.error.err_code, err_blk, 0);
	return(E_DB_ERROR);
    }
    return E_DB_OK;
}

/*
** Name: psy_evget_by_id
**
** Description:
**	Gets an event tuple by id. Note that this is a simple lookup
**	of the event tuple, other processing such as DAC/MAC checks etc
**	are handled by higher level code.
**
** Inputs:
**	sess_cb	- Session CB
**
**	ev_id	- Event id
**
** Outputs:
**	evtuple - Event tuple
**
**	err_blk - Error block
**
** History:
**	20-dec-93 (robf)
**          Created
*/
DB_STATUS
psy_evget_by_id(
	PSS_SESBLK *sess_cb,
	DB_TAB_ID  *ev_id,
	DB_IIEVENT *evtuple,
        DB_ERROR    *err_blk
)
{
    DB_STATUS		status;
    RDF_CB		rdf_ev;		/* RDF for event */
    /* zero out RDF_CB and init common elements */
    pst_rdfcb_init(&rdf_ev, sess_cb);
    STRUCT_ASSIGN_MACRO((*ev_id), rdf_ev.rdf_rb.rdr_tabid);
    rdf_ev.rdf_rb.rdr_types_mask = RDR_EVENT;

    rdf_ev.rdf_rb.rdr_update_op = RDR_OPEN;
    rdf_ev.rdf_rb.rdr_qtuple_count = 1;
    rdf_ev.rdf_rb.rdr_qrytuple = (PTR) evtuple;

    status = rdf_call(RDF_GETINFO, (PTR) &rdf_ev);

    if(status==E_DB_OK)
    {
	if (rdf_ev.rdf_error.err_code ==E_RD0013_NO_TUPLE_FOUND)
	{
		/*
		** No tuple found, but expected to since by id
		*/
		(VOID) psf_rdf_error(RDF_GETINFO, &rdf_ev.rdf_error, err_blk);
		return E_DB_ERROR;
	}
	else
		return E_DB_OK;
    }
    else 
    {
        (VOID) psf_rdf_error(RDF_GETINFO, &rdf_ev.rdf_error, err_blk);
	return E_DB_ERROR;
    }
}
