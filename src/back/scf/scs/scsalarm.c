
/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = su4_u42 su4_cmw
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cm.h>
#include    <cv.h>
#include    <ex.h>
#include    <lo.h>
#include    <pc.h>
#include    <cs.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <me.h>
#include    <tr.h>
#include    <er.h>
#include    <lk.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <usererror.h>

#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <opfcb.h>
#include    <ddb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <tm.h>
#include    <sxf.h>
#include    <gwf.h>
#include    <lg.h>

#include    <duf.h>
#include    <dudbms.h>
#include    <copy.h>
#include    <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <sc0a.h>

#include    <scserver.h>

#include    <cui.h>

/**
**
**  Name: SCSALARM.C - Security Alarm services for sessions
**
**  Description:
**      This file contains security alarm services, primarily firing
**	alarms when necessery at session startup/shutdown.
**
**	    scs_get_db_alarms  - Get database security alarms.
**          scs_fire_db_alarms - Fire database security alarms
**
**  History:
**      3-dec-93 (robf)
**          Created.
**	    Initialize resource limits to non-existent.
**	21-jul-95 (dougb)
**	    Initialize qeu.qeu_flag values, preventing endless loops.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototype for
**	    sce_raise().
**	19-Jan-2001 (jenjo02)
**	    Stifle calls to sc0a_write_audit() if not C2SECURE.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
*/
/*
**  Forward and/or External function references.
*/


/*
** Definition of all global variables owned by this file.
*/

GLOBALREF SC_MAIN_CB          *Sc_main_cb; /* Central structure for SCF */

/*{
** Name: scs_get_db_alarms	- Get database security alarms  for the
**			          current session, and save.
**
** Description:
**      This routine looks through iisecalarm for alarms on the current
**	database which apply to the current session and saves them.
**
**	This routine must be called only when connected to the iidbdb,
**	and already in a transaction.
**
**	It is assumed  that scs_fire_db_alarms() will be called at
**	some later point to selectively fire alarms.
** 
** Inputs:
**      scb				the session control block
**
** Outputs:
**	    none
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-dec-93  (robf)
**	   Created
**	14-dec-93 (robf)
**         Use standard header for alarm structure
**	21-dec-93 (robf)
**         Add lookup for dbevent via iievent/iixevent 
**	29-dec-93 (robf)
**         Performance: security alarms only apply if C2 enabled.
**	12-jul-94 (robf)
**         Log QEF error returned when failure to open iisecalarm.
**      20-dec-94 (chech02)
**          removed some unnecessary casts (int to char*) that cause
**          the AIX 4.1 compiler to complain
**	21-jul-95 (dougb)
**	    Initialize qeu.qeu_flag values, preventing endless loops.  Since
**	    this structure is on the stack, every bit could be set.  On VMS
**	    platforms, this resulted in QEU_BY_TID operations for any
**	    QEU_NEXT operations after the first.  qef_call() will happily
**	    return the same record "forever".
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
*/
DB_STATUS
scs_get_db_alarms(SCD_SCB *scb)
{
    DB_STATUS 		status=E_DB_OK;
    bool		loop=FALSE;
    QEU_CB		qeu;
    QEU_CB		ev_qeu;
    QEU_CB		xev_qeu;
    DMR_ATTR_ENTRY	key[3];
    DMR_ATTR_ENTRY	*key_ptr[3];
    QEF_DATA		data;
    QEF_DATA		ev_data;
    QEF_DATA		xev_data;
    DMR_ATTR_ENTRY	xev_key[2];
    DMR_ATTR_ENTRY	*xev_key_ptr[2];
    bool		opened=FALSE;
    bool		event_opened=FALSE;
    bool		xevent_opened=FALSE;
    char		objtype;
    i4			tab0=0;
    DB_SECALARM 	atuple;
    DB_IIEVENT		evtuple;
    DB_IIXEVENT		xevtuple;
    SCS_ALARM		*scs_alarm;
    bool		applies;
    char		*grantee;

    do 
    {
	/*
	** Security alarms only apply on C2 servers
	*/
	if (!(Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
		break;

	scb->scb_sscb.sscb_alcb=NULL; /* No alarm list */
    	key_ptr[0] = &key[0];
    	key_ptr[1] = &key[1];
    	key_ptr[2] = &key[2];
        qeu.qeu_eflag = QEF_INTERNAL;
        qeu.qeu_db_id = (PTR) scb->scb_sscb.sscb_ics.ics_opendb_id;
        qeu.qeu_d_id =  scb->cs_scb.cs_self;
    	qeu.qeu_mask = 0;
	qeu.qeu_tab_id.db_tab_base = DM_B_IISECALARM_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_IISECALARM_TAB_ID;
        qeu.qeu_qual = 0;
        qeu.qeu_qarg = 0;
        qeu.qeu_lk_mode = DMT_IS;
        qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_flag = 0;

	status = qef_call(QEU_OPEN, &qeu);
	if (status)
	{
		sc0e_0_put(qeu.error.err_code, 0);
		sc0e_0_put(E_US2475_9333_IISECALARM_OPEN, 0);
		qeu.error.err_code = E_US2475_9333_IISECALARM_OPEN;
		break;
	}
	opened = TRUE;

	qeu.qeu_getnext = QEU_REPO;
	/*
	** Look up all database objects
	*/
	objtype=DBOB_DATABASE;
	key[0].attr_number = DM_1_IISECALARM_KEY;
	key[0].attr_operator = DMR_OP_EQ;
	key[0].attr_value = (char *) &objtype;
	key[1].attr_number = DM_2_IISECALARM_KEY;
	key[1].attr_operator = DMR_OP_EQ;
	key[1].attr_value = (char *) &tab0;
	key[2].attr_number = DM_3_IISECALARM_KEY;
	key[2].attr_operator = DMR_OP_EQ;
	key[2].attr_value = (char *) &tab0;
	qeu.qeu_klen = 3;
        qeu.qeu_key = (DMR_ATTR_ENTRY **) key_ptr;

	data.dt_data = (PTR) &atuple;
	data.dt_size = sizeof(atuple);
	data.dt_next=(QEF_DATA*)0;
	qeu.qeu_output= &data;
	qeu.qeu_count = 1;

	qeu.qeu_tup_length = sizeof(atuple);
	/*
	** Loop over all alarms
	*/
	qeu.error.err_code=0;

	do {
	    status = qef_call(QEU_GET, &qeu);

	    if(status!=E_DB_OK || qeu.error.err_code)
		break;

	    qeu.qeu_getnext = QEU_NOREPO;
	    /*
	    ** Process this alarm. We save alarms for the end database,
	    ** and alarms for the whole installation.
	    */
	    if((STskipblank((char*)&atuple.dba_objname,sizeof(atuple.dba_objname))) &&
		STbcompare((char*)&atuple.dba_objname,sizeof(atuple.dba_objname),
			    (char*)&scb->scb_sscb.sscb_ics.ics_dbname,
			    sizeof(scb->scb_sscb.sscb_ics.ics_dbname),TRUE)
		)
	    {
		/*
		** Ignore this alarm.
		*/
		continue;
	    }
	    /*
	    ** Check if applies to our session
	    */
	    applies=FALSE;
    	    switch (atuple.dba_subjtype)
    	    {
		    case DBGR_PUBLIC:
		    {
	    	    	applies = TRUE;
	    	    	break;
		    }
		    case DBGR_USER:
		    {
	    	   	grantee = (char *) scb->scb_sscb.sscb_ics.ics_eusername;
			if (!MEcmp((PTR) &atuple.dba_subjname, grantee, 
					sizeof(atuple.dba_subjname)))
				applies=TRUE;
	    	    	break;
		    }
		    case DBGR_GROUP:	   /* Check group id */
		    {
			grantee = (char *) &scb->scb_sscb.sscb_ics.ics_egrpid;
			if (!MEcmp((PTR) &atuple.dba_subjname, grantee, 
					sizeof(atuple.dba_subjname)))
				applies=TRUE;
	    		break;
		    }
		    case DBGR_APLID:	   /* Check application id */
	    	    {
			 grantee = (char *) &scb->scb_sscb.sscb_ics.ics_eaplid;
			if (!MEcmp((PTR) &atuple.dba_subjname, grantee, 
					sizeof(atuple.dba_subjname)))
				applies=TRUE;
	    		break;
	            }
		    default:	    /* this should never happen */
		    {

	    		sc0e_put(E_US2478_9336_BAD_ALARM_GRANTEE, 0L, 2,
			     sizeof(atuple.dba_subjtype), &atuple.dba_subjtype,
			     sizeof(atuple.dba_alarmname), (PTR)&atuple.dba_alarmname,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0,
			     0, (PTR)0 );
			status=E_DB_ERROR;
			break;
		     }
	    }
	    if(!applies)
	    {
		continue;
	    }
	    /*
	    ** Got an alarm, so save
	    */
	    status = sc0m_allocate(SCU_MZERO_MASK,
			    (i4)sizeof(SCS_ALARM),
			    DB_SCF_ID,
			    (PTR)SCS_MEM,
			    SCA_TAG,
			    (PTR *)&scs_alarm);
	    if(status!=E_DB_OK)
	    {
		/* sc0m_allocate puts error codes in return status */
	        sc0e_0_put(status, 0);
		status=E_DB_SEVERE;
		break;
	    }
	    scs_alarm->scal_next= scb->scb_sscb.sscb_alcb;
	    scb->scb_sscb.sscb_alcb= scs_alarm;
	    MEcopy((PTR)&atuple,sizeof(DB_SECALARM),(PTR)&scs_alarm->alarm);
	    /*
	    ** Check if we need event info
	    */
	    if(atuple.dba_flags&DBA_DBEVENT)
	    {
		/*
		** Need event info
		*/
		if(event_opened==FALSE)
		{
		    /*
		    ** Need to open iievent
		    */
        	    ev_qeu.qeu_eflag = QEF_INTERNAL;
                    ev_qeu.qeu_db_id = (PTR) scb->scb_sscb.sscb_ics.ics_opendb_id;
                    ev_qeu.qeu_d_id =  scb->cs_scb.cs_self;
    	            ev_qeu.qeu_mask = 0;
	            ev_qeu.qeu_tab_id.db_tab_base = DM_B_EVENT_TAB_ID;
	            ev_qeu.qeu_tab_id.db_tab_index = DM_I_EVENT_TAB_ID;
                    ev_qeu.qeu_qual = 0;
                    ev_qeu.qeu_qarg = 0;
                    ev_qeu.qeu_lk_mode = DMT_IS;
                    ev_qeu.qeu_access_mode = DMT_A_READ;
		    ev_qeu.qeu_flag = QEU_BY_TID;

		    status = qef_call(QEU_OPEN, &ev_qeu);
	            if (status)
		    {
		         sc0e_0_put(E_SC034B_ALARM_QRY_IIEVENT, 0);
		         ev_qeu.error.err_code = E_SC034B_ALARM_QRY_IIEVENT;
		         break;
	            }
		    ev_qeu.qeu_klen = 0;

		    ev_data.dt_data = (PTR) &evtuple;
		    ev_data.dt_size = sizeof(evtuple);
		    ev_data.dt_next=(QEF_DATA*)0;
		    ev_qeu.qeu_output= &ev_data;
		    ev_qeu.qeu_count = 1;
		    ev_qeu.qeu_getnext=QEU_NOREPO;
		    ev_qeu.qeu_tup_length = sizeof(evtuple);
		    event_opened=TRUE;
		} /* End not event_opened */
		/*
		** Open iixevent to lookup by id
		*/
		if(xevent_opened==FALSE)
		{
		    /*
		    ** Need to open iicevent
		    */
		    xev_key_ptr[0] = &xev_key[0];
		    xev_key_ptr[1] = &xev_key[1];
        	    xev_qeu.qeu_eflag = QEF_INTERNAL;
                    xev_qeu.qeu_db_id = (PTR) scb->scb_sscb.sscb_ics.ics_opendb_id;
                    xev_qeu.qeu_d_id =  scb->cs_scb.cs_self;
    	            xev_qeu.qeu_mask = 0;
	            xev_qeu.qeu_tab_id.db_tab_base = DM_B_XEVENT_TAB_ID;
	            xev_qeu.qeu_tab_id.db_tab_index = DM_I_XEVENT_TAB_ID;
                    xev_qeu.qeu_qual = 0;
                    xev_qeu.qeu_qarg = 0;
                    xev_qeu.qeu_lk_mode = DMT_IS;
                    xev_qeu.qeu_access_mode = DMT_A_READ;
		    xev_qeu.qeu_flag = 0;

		    status = qef_call(QEU_OPEN, &xev_qeu);
	            if (status)
		    {
		         sc0e_0_put(E_SC034B_ALARM_QRY_IIEVENT, 0);
		         xev_qeu.error.err_code = E_SC034B_ALARM_QRY_IIEVENT;
		         break;
	            }
		    xev_key[0].attr_number = DM_1_XEVENT_KEY;
		    xev_key[0].attr_operator = DMR_OP_EQ;
		    xev_key[0].attr_value = (char *) &atuple.dba_eventid.db_tab_base;
		    xev_key[1].attr_number = DM_2_XEVENT_KEY;
		    xev_key[1].attr_operator = DMR_OP_EQ;
		    xev_key[1].attr_value = (char *) &atuple.dba_eventid.db_tab_index;
		    xev_qeu.qeu_klen = 2;
		    xev_qeu.qeu_key = (DMR_ATTR_ENTRY **) xev_key_ptr;

		    xev_data.dt_data = (PTR) &xevtuple;
		    xev_data.dt_size = sizeof(xevtuple);
		    xev_data.dt_next=(QEF_DATA*)0;
		    xev_qeu.qeu_output= &xev_data;
		    xev_qeu.qeu_count = 1;

		    xev_qeu.qeu_tup_length = sizeof(xevtuple);
		    xevent_opened=TRUE;
		} /* End not xevent_opened */
		/*
		** Look up event tid via index
		*/
	        xev_qeu.qeu_getnext = QEU_REPO;
	        status = qef_call(QEU_GET, &xev_qeu);

	        if(status!=E_DB_OK || xev_qeu.error.err_code)
		{
			sc0e_0_put(xev_qeu.error.err_code, 0);
			sc0e_put(E_US247A_9338_MISSING_ALM_EVENT, 0,3,
				sizeof(atuple.dba_eventid.db_tab_base),
				        (PTR)&atuple.dba_eventid.db_tab_base,
				sizeof(atuple.dba_eventid.db_tab_index),
				        (PTR)&atuple.dba_eventid.db_tab_index,
				sizeof(atuple.dba_alarmname),
					(PTR)&atuple.dba_alarmname,
		     		0, (PTR)0,
		     		0, (PTR)0,
		     		0, (PTR)0 );
			break;
		}
		/*
		** Now get event by tid
		*/
		ev_qeu.qeu_tid=xevtuple.dbx_tidp;
	        status = qef_call(QEU_GET, &ev_qeu);

	        if(status!=E_DB_OK || ev_qeu.error.err_code)
		{
			sc0e_0_put(xev_qeu.error.err_code, 0);
			sc0e_put(E_US247A_9338_MISSING_ALM_EVENT, 0,3,
				sizeof(atuple.dba_eventid.db_tab_base),
				        (PTR)&atuple.dba_eventid.db_tab_base,
				sizeof(atuple.dba_eventid.db_tab_index),
				        (PTR)&atuple.dba_eventid.db_tab_index,
				sizeof(atuple.dba_alarmname),
					(PTR)&atuple.dba_alarmname,
		     		0, (PTR)0,
		     		0, (PTR)0,
		     		0, (PTR)0 );
			break;
		}
		/*
		** Got dbevent, so save name/owner for firing
		*/
		STRUCT_ASSIGN_MACRO(evtuple.dbe_name, scs_alarm->event_name);
		STRUCT_ASSIGN_MACRO(evtuple.dbe_owner, scs_alarm->event_owner);
	    } /* End has dbevent */

	}
	while (status == 0);

	if (status && qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    sc0e_0_put(E_US2476_9334_IISECALARM_ERROR, 0);
	    if(qeu.error.err_code)
		    sc0e_0_put(qeu.error.err_code, 0);
	    qeu.error.err_code = E_US2476_9334_IISECALARM_ERROR;
	    break;
	}

	status = qef_call(QEU_CLOSE, &qeu);
	if (status)
	{
		sc0e_0_put(qeu.error.err_code, 0);
		sc0e_0_put(E_SC034B_ALARM_QRY_IIEVENT, 0);
		sc0e_0_put(E_US2476_9334_IISECALARM_ERROR, 0);
	}
	opened = FALSE;

    } while (loop);

    if(event_opened)
    {
	if( qef_call(QEU_CLOSE, &ev_qeu)!=E_DB_OK)
	{
		sc0e_0_put(qeu.error.err_code, 0);
		sc0e_0_put(E_SC034B_ALARM_QRY_IIEVENT, 0);
		sc0e_0_put(E_US2476_9334_IISECALARM_ERROR, 0);
	}
    }
    if(xevent_opened)
    {
	if( qef_call(QEU_CLOSE, &xev_qeu)!=E_DB_OK)
	{
		sc0e_0_put(qeu.error.err_code, 0);
		sc0e_0_put(E_US2476_9334_IISECALARM_ERROR, 0);
	}
    }
    if(opened)
    {
	if( qef_call(QEU_CLOSE, &qeu)!=E_DB_OK)
	{
		sc0e_0_put(qeu.error.err_code, 0);
		sc0e_0_put(E_US2476_9334_IISECALARM_ERROR, 0);
	}
    }
    /*
    ** Report any errors
    */
    if(status!=E_DB_OK)
    {
	sc0e_0_put(E_SC0349_GET_DB_ALARMS, 0);
    }
    return status;
}

/*{
** Name: scs_fire_db_alarms	- Fire database security alarms for the
**			          current session matching certain criteria
**
** Description:
**      This routine looks through the database alarm list for the
**	current session, and fires any which meet the input criteria,
**	such as success/failure or connect/disconnect.
**
** Inputs:
**      scb				the session control block
**
**	alarm_spec			alarm specification.
**
** Outputs:
**	    none
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    Fires security alarms (and possible associated database events)
**
** History:
**	3-dec-93  (robf)
**	   Created
**	29-dec-93 (robf)
**         Performance: security alarms only apply if C2 enabled.
**	14-feb-94 (robf)
**         Use SXF_A_CONNECT/DISCONNECT
*/
DB_STATUS
scs_fire_db_alarms(SCD_SCB *scb, i4 alarm_spec)
{
    DB_STATUS status=E_DB_OK;
    bool	loop=FALSE;
    SCS_ALARM	*scs_alarm;
    DB_SECALARM *alarm;
    DB_ERROR	error;
    SXF_ACCESS  access;
    i4	mesgid;
    DB_DATA_VALUE tm;					
    DB_DATE	tm_date;
    SCF_ALERT	scfa;					
    SCF_CB	scf_cb;				
    DB_ALERT_NAME alert;
    i4	err;
    char	*errstr;
    char	*textstat;
    char	*textop;
    ADF_CB      *adf_scb = scb->scb_sscb.sscb_adscb;  
    ADF_CB	adf_cb;

    do 
    {
	/*
	** Security alarms only apply on C2 servers
	*/
	if (!(Sc_main_cb->sc_capabilities & SC_C_C2SECURE))
		break;

	/*
	** If no alarms we are done
	*/
	if (!scb->scb_sscb.sscb_alcb)
		break;

	/* Copy the session ADF block into local one, if available */
	if(adf_scb)
		STRUCT_ASSIGN_MACRO(*adf_scb, adf_cb);
	else
		MEfill(sizeof(adf_cb),0, (PTR)&adf_cb);
	/*
	** Build access spec, this is based on database actions
	*/
	if(alarm_spec&DB_ALMSUCCESS)
	{
		textstat="success";
		access=SXF_A_SUCCESS;
	}
	else
	{
		textstat="failure";
		access=SXF_A_FAIL;
	}

	if(alarm_spec&DB_CONNECT)
	{
		access|=SXF_A_CONNECT;
		mesgid=I_SX2743_DB_ALARM;
		textop="connect";
	}
	else if (alarm_spec & DB_DISCONNECT)
	{
		access|=SXF_A_DISCONNECT;
		mesgid=I_SX2743_DB_ALARM;
		textop="disconnect";
	}
	else
	{
		/* Unknown currently */
		access|=SXF_A_SELECT;
		mesgid=I_SX2743_DB_ALARM;
		textop="access";
	}

       	for(scs_alarm=scb->scb_sscb.sscb_alcb; 
	   scs_alarm!=NULL;
	   scs_alarm=scs_alarm->scal_next)
       	{
	    alarm= &scs_alarm->alarm;

	    if((alarm->dba_popset&alarm_spec)==alarm_spec)
	    {
		/*
		** This alarm matches spec, so lets fire it
		*/
		/*
		** Write the audit record. 
		** Currently all events here are database events, so
		** record database label/name for the object.
		*/
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		{
		    status = sc0a_write_audit(
			    scb,
			    SXF_E_ALARM,	/* Type */
			    access,        /* Action */
			    (char*)&scb->scb_sscb.sscb_ics.ics_dbname,
			    sizeof( scb->scb_sscb.sscb_ics.ics_dbname),
			    NULL,
			    mesgid, 
			    TRUE,			/* Force record */
			    scb->scb_sscb.sscb_ics.ics_rustat,
			    &error			/* Error location */
		    );	
		    if(status!=E_DB_OK)
			    break;
		}
		/*
		** Raise any needed dbevent
		*/
		if(alarm->dba_flags&DBA_DBEVENT)
		{
		    char evtext[DB_EVDATA_MAX+1];
    		    /*
    		    ** Build alert name
    		    */
    		    STRUCT_ASSIGN_MACRO(scs_alarm->event_name, alert.dba_alert);
    		    STRUCT_ASSIGN_MACRO(scs_alarm->event_owner, alert.dba_owner);
		    /*
		    ** Note: database alarms are always in the iidbdb,
		    ** so make sure we fire in the iidbdb context
		    */
		    MEcopy((PTR)DB_DBDB_NAME, sizeof(alert.dba_dbname),
					(PTR)&alert.dba_dbname);

    		    scf_cb.scf_ptr_union.scf_alert_parms = &scfa;

    		    scfa.scfa_name = &alert;
    		    scfa.scfa_text_length = 0;
    		    scfa.scfa_user_text = NULL;
    		    scfa.scfa_flags = 0;

    		    if (alarm->dba_flags&DBA_DBEVTEXT)		
    		    {
			scfa.scfa_text_length = sizeof(alarm->dba_eventtext);
			scfa.scfa_user_text = alarm->dba_eventtext;
		    }
		    else
		    {
			STprintf(evtext, ERx("Security alarm raised on database %.*s on %s when %s by %.*s "),
				scs_trimwhite(sizeof(scb->scb_sscb.sscb_ics.ics_dbname),
				(char*)&scb->scb_sscb.sscb_ics.ics_dbname),
				(char*)&scb->scb_sscb.sscb_ics.ics_dbname,
				  textstat,
				  textop,
				  scs_trimwhite(sizeof(scb->scb_sscb.sscb_ics.ics_eusername),
				      (char*)scb->scb_sscb.sscb_ics.ics_eusername),
				  scb->scb_sscb.sscb_ics.ics_eusername);
			scfa.scfa_user_text=evtext;
			scfa.scfa_text_length=STlength(evtext);
		    }
	            tm.db_datatype = DB_DTE_TYPE; 	
	            tm.db_prec = 0;
                    tm.db_length = sizeof(tm_date);
                    tm.db_data = (PTR)&tm_date;
                    status = adu_datenow(&adf_cb, &tm);
                    if (status != E_DB_OK)
                    {
			break;
		    }
    		    scfa.scfa_when = &tm_date;

		    status=sce_raise(&scf_cb, scb);
		    if(status!=E_DB_OK)
		    {
			sc0e_0_put(E_US2477_9335_ALARM_EVENT_ERR, 0);
			status=E_DB_SEVERE;
			break;
		    }
		}
	    }
	}
    } while (loop);

    if(status!=E_DB_OK)
    {
	sc0e_0_put(E_SC034A_FIRE_DB_ALARMS, 0);
    }
    return status;
}

/*{
** Name: scs_trimwhite 	- Trim trailing white space.
**
** Description:
**	Utility to trim white space off of trace and error names.
**	It is assumed that no nested blanks are contained within text.
**
** Inputs:
**      len		Max length to be scanned for white space
**	buf		Buf holding char data (may not be null-terminated)
**
** Outputs:
**	Returns:
**	    Length of non-white part of the string
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**	7-dec-93 (robf)
**	    Written for alarm events
*/
i4
scs_trimwhite(
i4	len,
char	*buf )
{
    register char   *ep;    /* pointer to end of buffer (one beyond) */
    register char   *p;
    register char   *save_p;

    if (!buf)
    {
       return (0);
    }

    for (save_p = p = buf, ep = p + len; (p < ep) && (*p != EOS); )
    {
        if (!CMwhite(p))
            save_p = CMnext(p);
        else
            CMnext(p);
    }

    return ((i4)(save_p - buf));
} /* scs_trimwhite */

