/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <er.h>
#include    <cv.h>
#include    <me.h>
#include    <st.h>
#include    <qu.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmacb.h>
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
#include    <usererror.h>
#include    <sxf.h>
#include    <scf.h>
#include    <dudbms.h>
#include    <psyaudit.h>

/**
**
**  Name: PSYALARM.C - Functions to create/drop security alarms
**
**  Description:
**      This file contains functions necessary to create and drop
**	security alarms:
**
**          psy_alarm - Routes requests for security alarm maintenance
**
**  History:
**	21-sep-89 (ralph)
**	    written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	28-oct-92 (robf)
**	    Changed CALARM to call dgrant rather than dpermit, this should
**	    get correct query info stored.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	    get correct query info stored..
**	16-nov-1992 (pholman)
**	    C2: Remove obsolete qeu_audit external.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	29-nov-93 (robf)
**          Largely rewritten, alarms are now handled distinctly from permits
**	    so no longer go down into psy_dgrant, instead new routines
**	    psy_calarm and psykalarm (defined in this file) are called.
**	12-jul-94 (robf)
**          Added PTR cast on qsf_owner assignment to quieten compiler.
**	21-feb-95 (brogr02)
**          Changed wording of error message US18D4, bug 67030
**	16-jul-95 (popri01)
**	    Eliminate extraneous characters from user name in alarm query text.
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**      15-Apr-2003 (bonro01)
**          Added include <psyaudit.h> for prototype of psy_secaudit()
**      01-apr-2010 (stial01)
**          Changes for Long IDs
[@history_template@]...
**/

/*
**  Definition of static variables and forward static functions.
*/


/*{
** Name: psy_alarm - Dispatch security alarm qrymod routines
**
**  INTERNAL PSF call format: status = psy_alarm(&psy_cb, sess_cb);
**  EXTERNAL     call format: status = psy_call (PSY_ALARM, &psy_cb, sess_cb);
**
** Description:
**	This procedure checks the psy_alflag field of the PSY_CB
**	to determine which qrymod processing routine to call:
**		PSY_CALARM results in call to psy_calarm()
**		PSY_KALARM results in call to psy_kalarm()
**
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_alflag			location operation
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
**	21-sep-89 (ralph)
**          written
**	29-nov-93 (robf)
**          Now really call psy_calarm/psy_kalarm()
*/
DB_STATUS
psy_alarm(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status=E_DB_OK;
    i4		err_code;
    i4			user_status;
    char		dbname[DB_DB_MAXNAME];
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[2];
    bool		loop=FALSE;
    DB_SECALARM		*alarm= &psy_cb->psy_tuple.psy_alarm;
    DB_STATUS		local_status;
    DB_ERROR		e_error;

    /* This code is called for SQL only */
    /* 
    ** Ensure we're connected to the iidbdb database for database/installation
    ** alarms, and have SECURITY privilege
    */

    if(alarm->dba_objtype==DBOB_DATABASE) 
    do
    {
	scf_cb.scf_length	= sizeof (SCF_CB);
	scf_cb.scf_type	= SCF_CB_TYPE;
	scf_cb.scf_facility	= DB_PSF_ID;
	scf_cb.scf_session	= sess_cb->pss_sessid;
	scf_cb.scf_ptr_union.scf_sci     = (SCI_LIST *) sci_list;
	scf_cb.scf_len_union.scf_ilength = 1;
	sci_list[0].sci_code    = SCI_DBNAME;
	sci_list[0].sci_length  = sizeof(dbname);
	sci_list[0].sci_aresult = (char *) dbname;
	sci_list[0].sci_rlength = NULL;

	status = scf_call(SCU_INFORMATION, &scf_cb);
	if (status != E_DB_OK)
	{
	    err_code = scf_cb.scf_error.err_code;
	    (VOID) psf_error(E_PS0D13_SCU_INFO_ERR, 0L,
			 PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
	    return(status);
	}
	if (MEcmp((PTR)dbname, (PTR)DB_DBDB_NAME, sizeof(dbname)))
	{
	    /* Session not connected to iidbdb */
	    err_code = E_US18D4_6356_NOT_DBDB;
	    (VOID) psf_error(E_US18D4_6356_NOT_DBDB, 0L,
			     PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			     sizeof("CREATE/DROP SECURITY_ALARM ON DATABASE")-1,
			     "CREATE/DROP SECURITY_ALARM ON DATABASE");
	    status=E_DB_ERROR;
	    break;
	}
	if( !(sess_cb->pss_ustat & DU_USECURITY))
	{
		err_code = E_US18D5_6357_FORM_NOT_AUTH;
		(VOID) psf_error(E_US18D5_6357_FORM_NOT_AUTH, 0L,
			 PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			 sizeof("CREATE/DROP SECURITY_ALARM")-1,
			 "CREATE/DROP SECURITY_ALARM");
		status=E_DB_ERROR;
		break;
	}
    } while(loop);

    if (status!=E_DB_OK)
    {
	/*
	** Audit failure
	*/
	if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	{
	    SXF_EVENT evtype;
	    i4   msgid;

	    if(alarm->dba_objtype==DBOB_DATABASE) 
		    evtype=SXF_E_DATABASE;
	    else
		    evtype=SXF_E_TABLE;

	    if(psy_cb->psy_alflag&PSY_CALARM)
		    msgid=I_SX202D_ALARM_CREATE;
	    else
		    msgid=I_SX202E_ALARM_DROP;

	    local_status = psy_secaudit(FALSE, sess_cb,
			    (char *)&alarm->dba_objname,
			    (DB_OWN_NAME *)NULL,
			    sizeof(alarm->dba_objname),
			    evtype,
			    msgid,
			    SXF_A_CONTROL|SXF_A_FAIL,
			    &e_error);
	}

       if(psy_cb->psy_alflag&PSY_CALARM)
       {
	    /*
	    ** Destroy query text for CREATE SECURITY_ALARM before
	    ** returning
	    */
	    DB_STATUS loc_status;
	    QSF_RCB	qsf_rb;

	    qsf_rb.qsf_lk_state	= QSO_EXLOCK;
	    qsf_rb.qsf_type	= QSFRB_CB;
	    qsf_rb.qsf_ascii_id	= QSFRB_ASCII_ID;
	    qsf_rb.qsf_length	= sizeof(qsf_rb);
	    qsf_rb.qsf_owner	= (PTR)DB_PSF_ID;
	    qsf_rb.qsf_sid	= sess_cb->pss_sessid;

	    /* Destroy query text - lock it first */
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
	}
	return status;
    }

    /* Select the proper routine to process this request */

    switch (psy_cb->psy_alflag & (PSY_CALARM | PSY_KALARM))
    {
	case PSY_CALARM:
	    status = psy_calarm(psy_cb, sess_cb);
	    break;

	case PSY_KALARM:
	    status = psy_kalarm(psy_cb, sess_cb);
	    break;

	default:
	    status = E_DB_SEVERE;
	    err_code = E_PS0D49_INVALID_ALARM_OP;
	    (VOID) psf_error(E_PS0D49_INVALID_ALARM_OP, 0L,
		    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    break;
    }

    return    (status);
}

/*{
** Name: psy_calarm	- Create a security alarm
**
** Description:
**      This routine stores the definition of a security alarm
**	in the system table iisecalarm and iiqrytext.  
**
**	The actual work of storing the alarm is done by RDF and QEU.  Note
**	that the alarm name is not yet validated within the user scope and
**	QEU may return a "duplicate alarm" error.
**
** Inputs:
**      psy_cb
**	    .psy_tuple.psy_alarm	Alarm tuple to insert.  All
**					user-specified fields of this
**					parameter are filled in by the
**					grammar.  Internal fields (text ids) 
**					are filled in just before
**					storage in QEU when the ids are
**					constructed.  
**          .psy_qrytext                Id of query text as stored in QSF.
**	sess_cb				Pointer to session control block.
**
** Outputs:
**      psy_cb
**	    .psy_error.err_code		Filled in if an error happens:
**		E_PS0D18_QSF_LOCK 	  Cannot lock text.
**		E_PS0D19_QSF_INFO	  Cannot get information for text.
**		E_PS0D1A_QSF_DESTROY	  Could not destroy text.
**		E_PS0903_TAB_NOTFOUND	  Table not found.
**		Generally, the error code returned by RDF and QEU
**		qrymod processing routines are passed back to the caller.
**
**	Returns:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_FATAL
**	Exceptions:
**	    none
**
** Side Effects:
**	Stores query text in iiqrytext, alarm tuple in
**	iisecalarm identifying the alarm.  
**
** History:
**	29-nov-93 (robf)
**	   Written for Secure 2.0
**	15-jul-95 (popri01)
**	   On nc4_us5, the alarm query text was picking up a garbage character
**	   at the end of user name in the "by user ..." clause in the alarm
**	   query text.  Given that we calculate the name length (subj_name_len),
**	   use it to force a string delimiter on subj_name if the length
**	   is less than the maximum.
*/

DB_STATUS
psy_calarm(
	PSY_CB	   *psy_cb,
	PSS_SESBLK *sess_cb)
{
    RDF_CB		rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    i4			textlen;		/* Length of query text */
    QSF_RCB		qsf_rb;
    DB_STATUS		status, loc_status;
    i4		err_code;
    DB_SECALARM		*alarm= &psy_cb->psy_tuple.psy_alarm;

    /* Fill in QSF request to lock text */
    qsf_rb.qsf_type	= QSFRB_CB;
    qsf_rb.qsf_ascii_id	= QSFRB_ASCII_ID;
    qsf_rb.qsf_length	= sizeof(qsf_rb);
    qsf_rb.qsf_owner	= (PTR)DB_PSF_ID;
    qsf_rb.qsf_sid	= sess_cb->pss_sessid;

    /* Initialize RDF_CB */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_types_mask  = RDR_SECALM;	/* Alarm definition */
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_qrytuple    = (PTR)&psy_cb->psy_tuple.psy_alarm; /* Alarm tuple */

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
    rdf_rb->rdr_l_querytext	= textlen;
    rdf_rb->rdr_querytext	= ((char *)qsf_rb.qsf_root) + sizeof(i4);
    rdf_rb->rdr_tabid.db_tab_base=alarm->dba_objid.db_tab_base;

    /*
    ** Check type of alarm, and handle accordingly.
    ** Main difference is where/how subjects are handled:
    ** - Named alarms have a single subject, which has already been 
    **   resolved in psl. Alarms to public are handled here also
    ** - Anonymous alarms (old style) may have multiple subjects. These
    **   operate much like permits, and are split into several alarms, one
    **   for each subject.
    */
    if(STskipblank((char*)&alarm->dba_alarmname,
		sizeof(alarm->dba_alarmname)) ==NULL &&
	    (alarm->dba_subjtype==DBGR_USER ||
	     alarm->dba_subjtype==DBGR_APLID ||
	     alarm->dba_subjtype==DBGR_GROUP)
	)
    {
        PSY_USR		*psy_usr;
	u_char		*subj_name;
	i4		subj_name_len;
        u_char          delim_subj_name[DB_MAX_DELIMID];
	/*
	** Anonymous alarms.
	** We loop over each subject, updating the query text for
	** each subject and creating the alarm.
	*/
	psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;
	do 
	{
	    subj_name = (u_char *) &psy_usr->psy_usrnm;
	    subj_name_len = 
		(i4) psf_trmwhite(sizeof(psy_usr->psy_usrnm), 
		    (char *) subj_name);
	    if (~psy_usr->psy_usr_flags & PSY_REGID_USRSPEC)
	    {
		status = psl_norm_id_2_delim_id(&subj_name, 
		    &subj_name_len, delim_subj_name, 
		    &psy_cb->psy_error);
		if (DB_FAILURE_MACRO(status))
			return status;
	    }
	    if (subj_name_len < DB_MAX_DELIMID)
		subj_name[ subj_name_len ] = EOS;
	    STmove((char*)subj_name, ' ', DB_MAX_DELIMID,
		  rdf_rb->rdr_querytext + psy_cb->psy_noncol_grantee_off);

            STmove((char*)subj_name, ' ', sizeof(alarm->dba_subjname),
		(char*)&alarm->dba_subjname);
			
	    /* Reset alarm name in case filled in by RDF/QEF */
	    MEfill(sizeof(alarm->dba_alarmname), ' ',
		    (PTR)&alarm->dba_alarmname);


	    /* Create a new alarm in iisecalarm */
	    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	    if (status != E_DB_OK)
	    {
		if (   rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
		{
		    psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR, &err_code,
			  &psy_cb->psy_error, 1, 
			  psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
				   (char *) &psy_cb->psy_tabname[0]),
			  &psy_cb->psy_tabname[0]);
		}
		else
		{
		    _VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
				 &psy_cb->psy_error);
	        }
		goto cleanup;
	    } /* If RDF error */

	    /* Next subject */
	    psy_usr = (PSY_USR *) psy_usr->queue.q_next;
        } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);
    }
    else
    {
	/* Named alarms */
	/* Create a new alarm in iisecalarm */
	status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	if (status != E_DB_OK)
	{
	    if (   rdf_cb.rdf_error.err_code == E_RD0002_UNKNOWN_TBL)
	    {
		psf_error(E_PS0903_TAB_NOTFOUND, 0L, PSF_USERERR, &err_code,
			  &psy_cb->psy_error, 1, 
			  psf_trmwhite(sizeof(psy_cb->psy_tabname[0]),
				   (char *) &psy_cb->psy_tabname[0]),
			  &psy_cb->psy_tabname[0]);
	    }
	    else
	    {
		_VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error,
				 &psy_cb->psy_error);
	    }
	    goto cleanup;
	} /* If RDF error */
    }

cleanup:
    qsf_rb.qsf_lk_state	= QSO_EXLOCK;

    /* Destroy query text - lock it first */
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

    /* Invalidate tableinfo from RDF cache if table object */
    if(psy_cb->psy_gtype==DBOB_TABLE)
    {
        pst_rdfcb_init(&rdf_cb, sess_cb);
        STRUCT_ASSIGN_MACRO(alarm->dba_objid, rdf_rb->rdr_tabid);
        loc_status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
        if (DB_FAILURE_MACRO(loc_status))
        {
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
				&psy_cb->psy_error);
	    if (loc_status > status)
	        status = loc_status;
	}
    }
    return (status);
} /* psy_calarm */

/*{
** Name: psy_kalarm	- Destroy a security alarm.
**
** Description:
**      This removes the definition of a security alarm  from the system tables.
**
**	The actual work of deleting the alarm is done by RDF and QEU.  Note
**	that the alarm name is not yet validated within the user scope and
**	QEU may return an "unknown alarm" error.
**
** Inputs:
**      psy_cb
**	    .psy_tuple.psy_alarm	Alarm tuple to delete.
**		.psy_name		Name of alarm.  The owner of the
**					alarm is filled in this routine
**					from pss_user before calling
**					RDR_UPDATE.
**	sess_cb				Pointer to session control block.
**	qeu_cb				Initialized QEU_CB.
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
**	Deletes query text from iiqrytext, security alarm tuple
**	from iisecalarm.  
**
** History:
**	29-nov-93  (robf)
**	   Written for Secure 2.0
**	16-feb-94 (robf)
**         Pass DROP ALL info down via RDR_DROP_ALL mask.
** 	6-may-94 (robf)
**         Add space between =& to quiten VMS compiler
*/

DB_STATUS
psy_kalarm(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    RDF_CB              rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    RDF_CB              rdf_inv_cb;
    DB_STATUS		status;
    DB_SECALARM		*atuple= &psy_cb->psy_tuple.psy_alarm;
    i4			i;
    PSY_OBJ		*psy_obj;
    bool		loop=FALSE;


    /* Fill in the RDF request blocks */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_types_mask  = RDR_SECALM;	/* Alarm deletion */
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_qrytuple    = (PTR)atuple;/* Alarm tuple */

    pst_rdfcb_init(&rdf_inv_cb, sess_cb);
    rdf_inv_cb.rdf_rb.rdr_types_mask |= RDR_SECALM;

    do
    {
	/*
	** Loop through each alarm in turn deleting them.
	** User may have specified alarms by name or by number, or by all
	*/
	if (psy_cb->psy_numnms == 0 && 
	    (PSY_OBJ*)psy_cb->psy_objq.q_next ==  (PSY_OBJ*)&psy_cb->psy_objq)
	{
	    /*
	    ** Drop all alarms on the indicated object
	    */
	    rdf_rb->rdr_types_mask  |= RDR_DROP_ALL;	
	    STmove(ERx("all alarms"),' ',
			sizeof(atuple->dba_alarmname),
			(char*)&atuple->dba_alarmname);
	    status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);	/* Destroy alarm */
	    if (status != E_DB_OK)
	    {
	        _VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			&psy_cb->psy_error);
	    }
	}
	if(psy_cb->psy_numnms)
	{
	    /*
	    ** Dropping alarms by number, so walk through all numbers dropping.
	    */
	    for (i=0;i <psy_cb->psy_numnms; i++)
	    {
		char tmpstr[64];
		CVla((i4)psy_cb->psy_numbs[i],tmpstr);
		STmove(tmpstr,' ', 
			sizeof(atuple->dba_alarmname),
			(char*)&atuple->dba_alarmname);
	     	atuple->dba_alarmno=psy_cb->psy_numbs[i];
	     	status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	     	if (status != E_DB_OK)
	     	{
		     _VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			&psy_cb->psy_error);
		     break;
	        }
	    }
	}
	if((PSY_OBJ*)psy_cb->psy_objq.q_next!= (PSY_OBJ*)&psy_cb->psy_objq)
	{
	    /*
	    ** Dropping alarms by name, so walk through all names dropping
	    */
	    for (psy_obj  = (PSY_OBJ *)  psy_cb->psy_objq.q_next;
	      psy_obj != (PSY_OBJ *) &psy_cb->psy_objq;
	      psy_obj  = (PSY_OBJ *)  psy_obj->queue.q_next
	    )
	    {
	     	MEcopy((PTR)&psy_obj->psy_objnm,
		    sizeof(atuple->dba_alarmname), (PTR)&atuple->dba_alarmname);
	     	atuple->dba_alarmno=0;
	     	status = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);/* Destroy alarm */
	     	if (status != E_DB_OK)
	     	{
		    _VOID_ psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			&psy_cb->psy_error);
		    break;
	         }
	    }
	}
    }
    while(loop);

    if(status==E_DB_OK && atuple->dba_objtype==DBOB_TABLE)
    {
	/*
	** Invalidate table infoblk from RDF cache.
	*/
	STRUCT_ASSIGN_MACRO(atuple->dba_objid, rdf_inv_cb.rdf_rb.rdr_tabid);
	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_inv_cb); /* drop infoblk */
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_inv_cb.rdf_error,
				&psy_cb->psy_error);
	    return(status);
	}
    }
    return (status);
} /* psy_kalarm */

