/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
NO_OPTIM = 
*/
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cm.h>
#include    <cv.h>
#include    <ex.h>
#include    <lo.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <me.h>
#include    <tr.h>
#include    <er.h>
#include    <cs.h>
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
/*
** Name: scsdbprv - Routines which manipulate database privileges
**
** Description:    These routines set up and manipulate database privileges
**	           for a session, including loading privileges from
**	           the iidbpriv catalog and saving for later user.
** 
**	           The following functions are provided in this file:
**	           scs_merge_dbpriv - Merge a DB_PRIVILEGES tuple into
**                                    a SCS_DBPRIV structure appropriately.
**	           
**	           scs_get_dbpriv   - Look up from iidbpriv DB_PRIVILEGES
**	                              tuples matching some criteria and
**	                              merge them all together.
**
**	           scs_put_sess_dbpriv- Put SCS_DBPRIV into session values
**
**	           scs_init_dbpriv    - Initiate a SCS_DBPRIV structure
**
**		   scs_cvt_dbpr_tuple - Converts DB_PRIVILEGES to SCS_DBPRIV
**
** History:
**	18-mar-94 (robf)
**         Modularized out of scsinit.c 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
*/

/* Forward/Static declarations */
static VOID
scs_calc_dbpriv(SCD_SCB *scb ,
		SCS_DBPRIV     *inpriv,
		SCS_DBPRIV    *outpriv);


/*{
** Name: scs_merge_dbpriv- factor a set of dbprivs into output dbprivs
**
** Description:
**      This routine is used to factor in a set of database privileges 
**	specified for a grantee type into the session's database privileges.
**
**	This differs from scs_calc_dbpriv() in that it operates with
**	a hierarchy such that any dbprivs already set in outprivs have
**	higher priority than new privileges in inpriv. i.e. it merges in
**	privileges of a lower priority. It also ensure that inpriv is
**	unchanged on exit so may be called from different places.
**
** Inputs:
**	scb				address of session control block
**      inpriv				the SCS_DBPRIV tuple to be factored
**	outpriv				the SCS_DBPRIV output structure
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-89 (ralph)
**	    Written for GRANT Enhancements, Phase 2
**	2-Jul-1993 (daveb)
**	    prototyped.
**	22-oct-94 (robf)
**          Added idle/connect/priority limits
**	18-mar-94 (robf)
**          Reworked to operate on a SCS_DBPRIV structure rather than
**	    the scb directly.
**	21-mar-94 (robf)
**          Clarify usage and call scs_calc_dbpriv() to do the actual work.
[@history_template@]...
*/
VOID
scs_merge_dbpriv(SCD_SCB *scb ,
		SCS_DBPRIV     *inpriv,
		SCS_DBPRIV    *outpriv)
{
    /* Set privilege flags */
    u_i4	control;
    u_i4   in_ctl_dbpriv;

    /* Control is the mask of settable privileges */

    control = ~outpriv->ctl_dbpriv;

    /* Reset control flags to those not previously specified */

    in_ctl_dbpriv=inpriv->ctl_dbpriv;

    inpriv->ctl_dbpriv &= control;

    /* Do the merge */

    scs_calc_dbpriv(scb, inpriv, outpriv);

    inpriv->ctl_dbpriv=in_ctl_dbpriv;

    return;
}

/*{
** Name: scs_calc_dbpriv- factor a set of dbprivs into output dbprivs.
**
** Description:
**      This routine is used to factor in a set of database privileges 
**	specified for a grantee type into the session's database privileges.
**
** Inputs:
**	scb				address of session control block
**      inpriv				the SCS_DBPRIV tuple to be factored
**	outpriv				the SCS_DBPRIV output structure
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-mar-94 (robf)
**          Unmerged from scs_merge_dbpriv to allow scs_get_dbpriv to
**	    call directly.
[@history_template@]...
*/
static VOID
scs_calc_dbpriv(SCD_SCB *scb ,
		SCS_DBPRIV     *inpriv,
		SCS_DBPRIV    *outpriv)
{
    outpriv->fl_dbpriv |=	    /* Set ON if:	      */
	(~outpriv->ctl_dbpriv) &  /*   it was undefined,    */
	inpriv->ctl_dbpriv &	  /*	 it's being defined   */
	inpriv->fl_dbpriv;	  /*   it's value is TRUE   */

    outpriv->fl_dbpriv &=	    /* Don't set OFF if:      */
	(~inpriv->ctl_dbpriv) |	    /*   it's not defined     */
	inpriv->fl_dbpriv;	    /*   it's value is TRUE   */

    /* Revise values for valued privileges. */

    if (DBPR_QROW_LIMIT & inpriv->ctl_dbpriv)
    {
	if (DBPR_QROW_LIMIT & outpriv->fl_dbpriv)
	     outpriv->qrow_limit =
	    (outpriv->qrow_limit > inpriv->qrow_limit) ?
	     outpriv->qrow_limit : inpriv->qrow_limit;
	else
	    outpriv->qrow_limit = -1;
    }

    if (DBPR_QDIO_LIMIT & inpriv->ctl_dbpriv)
    {
	if (DBPR_QDIO_LIMIT & outpriv->fl_dbpriv)
	     outpriv->qdio_limit =
	    (outpriv->qdio_limit > inpriv->qdio_limit) ?
	     outpriv->qdio_limit : inpriv->qdio_limit;
	else
	    outpriv->qdio_limit = -1;
    }

    if (DBPR_IDLE_LIMIT & inpriv->ctl_dbpriv)
    {
	if (DBPR_IDLE_LIMIT & outpriv->fl_dbpriv)
	     outpriv->idle_limit =
	    (outpriv->idle_limit > inpriv->idle_limit) ?
	     outpriv->idle_limit : inpriv->idle_limit;
	else
	    outpriv->idle_limit = -1;

    }

    if (DBPR_CONNECT_LIMIT & inpriv->ctl_dbpriv)
    {
	if (DBPR_CONNECT_LIMIT & outpriv->fl_dbpriv)
	     outpriv->connect_limit =
	    (outpriv->connect_limit > inpriv->connect_limit) ?
	     outpriv->connect_limit : inpriv->connect_limit;
	else
	    outpriv->connect_limit = -1;

    }

    if (DBPR_PRIORITY_LIMIT & inpriv->ctl_dbpriv)
    {
	if (DBPR_PRIORITY_LIMIT & outpriv->fl_dbpriv)
	     outpriv->priority_limit =
	    (outpriv->priority_limit > inpriv->priority_limit) ?
	     outpriv->priority_limit : inpriv->priority_limit;
	else
	    outpriv->priority_limit = 0;
    }

    if (DBPR_QCPU_LIMIT & inpriv->ctl_dbpriv)
    {
	if (DBPR_QCPU_LIMIT & outpriv->fl_dbpriv)
	     outpriv->qcpu_limit =
	    (outpriv->qcpu_limit > inpriv->qcpu_limit) ?
	     outpriv->qcpu_limit : inpriv->qcpu_limit;
	else
	    outpriv->qcpu_limit = -1;
    }

    if (DBPR_QPAGE_LIMIT & inpriv->ctl_dbpriv)
    {
	if (DBPR_QPAGE_LIMIT & outpriv->fl_dbpriv)
	     outpriv->qpage_limit =
	    (outpriv->qpage_limit > inpriv->qpage_limit) ?
	     outpriv->qpage_limit : inpriv->qpage_limit;
	else
	    outpriv->qpage_limit = -1;
    }

    if (DBPR_QCOST_LIMIT & inpriv->ctl_dbpriv)
    {
	if (DBPR_QCOST_LIMIT & outpriv->fl_dbpriv)
	     outpriv->qcost_limit =
	    (outpriv->qcost_limit > inpriv->qcost_limit) ?
	     outpriv->qcost_limit : inpriv->qcost_limit;
	else
	    outpriv->qcost_limit = -1;
    }

    /* Turn on the control flags for the defined permissions. */

    outpriv->ctl_dbpriv |= inpriv->ctl_dbpriv;

    /* We are done */

    return;
}

/*{
** Name: scs_get_dbpriv	- obtain applicable database privileges from iidbpriv
**
** Description:
**      This routine reads privilege descriptors for the database/grantee
**	from the iidbpriv catalog, and calls scs_calc_dbpriv to factor
**	each tuple into the session's database privileges.
**
** Inputs:
**	scb				address of session control block
**	qeu				address of QEU request block
**      inpriv				the dbprivs to be factored
** Outputs:
**	outpriv				output privileges
**
**	Returns:
**	    E_DB_OK			Privileges were read/factored
**	    E_DB_INFO			No privileges found/factored
**	    E_DB_ERROR			Error reading iidbpriv
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-89 (ralph)
**	    Written for GRANT Enhancements, Phase 2
**	2-Jul-1993 (daveb)
**	    prototyped.
**	22-oct-94 (robf)
**          Added idle/connect/priority limits
**	18-mar-94 (robf)
**          Reworked to write to outpriv rather than SCB
**
[@history_template@]...
*/
DB_STATUS
scs_get_dbpriv(SCD_SCB *scb ,
                QEU_CB *qeu ,
                DB_PRIVILEGES *inpriv,
		SCS_DBPRIV    *outpriv)
{
    DB_STATUS	status;
    bool	found=FALSE;
    SCS_DBPRIV  cvtdbpriv;
    u_i4	control;
	

    /* Control is the mask of settable privileges */

    control = ~outpriv->ctl_dbpriv;

    /* "Prime the pump" */

    qeu->qeu_getnext = QEU_REPO;
    qeu->qeu_mask = 0;
    status = qef_call(QEU_GET, qeu);
    qeu->qeu_getnext = QEU_NOREPO;

    /* Process each applicable dbpriv record */

    for (;status == E_DB_OK;)
    {
        /* Reset control flags to those not previously specified */

        inpriv->dbpr_control &= control;

	/* Convert value to an SCS_DBPRIV */
	scs_cvt_dbpr_tuple(scb, inpriv, &cvtdbpriv);

	/* Invoke scs_calc_dbpriv to factor in these privileges */

	(VOID)scs_merge_dbpriv(scb, &cvtdbpriv, outpriv);

	found=TRUE;

	/* Get the next applicable privilege descriptor */

	status = qef_call(QEU_GET, qeu);
    }

    /* Check for errors */

    if (qeu->error.err_code != E_QE0015_NO_MORE_ROWS)
    {
	sc0e_0_put(E_US1888_IIDBPRIV, 0);
	sc0e_0_put(qeu->error.err_code, 0);
	qeu->error.err_code = E_US1888_IIDBPRIV;
	status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
    }
    else if(!found)
	status = E_DB_INFO;
    else
	status = E_DB_OK;

    /* Return to caller */

    return(status);
}

/*
** Name: scs_put_sess_dbpriv - put dbprivs into session values
**
** Description:
**	This takes a SCS_DBPRIV structure and assigns values appropriately
**	to the SCB values.
**
** Inputs:
**	scb     - SCB
**
**	dbpriv  - Dbprivilegs to be assigned
**
** History:
**	18-mar-94 (robf)
**          Created
**	21-mar-94 (robf)
**          Change session priority if current value would exceed 
**	    the new limit.
*/
DB_STATUS
scs_put_sess_dbpriv( SCD_SCB *scb, SCS_DBPRIV *dbpriv)
{
    DB_STATUS 	status=E_DB_OK;
    QEF_RCB	qef_cb;
    QEF_ALT	qef_alt[7];

    scb->scb_sscb.sscb_ics.ics_ctl_dbpriv  = dbpriv->ctl_dbpriv;
    scb->scb_sscb.sscb_ics.ics_fl_dbpriv   = dbpriv->fl_dbpriv;
    scb->scb_sscb.sscb_ics.ics_qrow_limit  = dbpriv->qrow_limit;	
    scb->scb_sscb.sscb_ics.ics_qdio_limit  = dbpriv->qdio_limit;	
    scb->scb_sscb.sscb_ics.ics_qcpu_limit  = dbpriv->qcpu_limit;	
    scb->scb_sscb.sscb_ics.ics_qpage_limit = dbpriv->qpage_limit;	
    scb->scb_sscb.sscb_ics.ics_qcost_limit = dbpriv->qcost_limit;
    scb->scb_sscb.sscb_ics.ics_idle_limit    = dbpriv->idle_limit;
    scb->scb_sscb.sscb_ics.ics_connect_limit = dbpriv->connect_limit;
    scb->scb_sscb.sscb_ics.ics_priority_limit=  dbpriv->priority_limit;
    /*
    ** Ensure current values follow limits. 
    */
    if ((DBPR_IDLE_LIMIT & dbpriv->ctl_dbpriv) &&
	(DBPR_IDLE_LIMIT & dbpriv->fl_dbpriv) &&
          (scb->scb_sscb.sscb_ics.ics_cur_idle_limit> dbpriv->idle_limit ||
	   scb->scb_sscb.sscb_ics.ics_cur_idle_limit <=0))
    {
	scb->scb_sscb.sscb_ics.ics_cur_idle_limit= dbpriv->idle_limit;

    }
    if ((DBPR_CONNECT_LIMIT & dbpriv->ctl_dbpriv) &&
	(DBPR_CONNECT_LIMIT & dbpriv->fl_dbpriv) &&
        (scb->scb_sscb.sscb_ics.ics_cur_connect_limit> dbpriv->connect_limit ||
 	 scb->scb_sscb.sscb_ics.ics_cur_connect_limit <=0))
    {
	scb->scb_sscb.sscb_ics.ics_cur_connect_limit= dbpriv->connect_limit;
	
    }
    if ((DBPR_PRIORITY_LIMIT & dbpriv->ctl_dbpriv) &&
	(DBPR_PRIORITY_LIMIT & dbpriv->fl_dbpriv))
    {
        if (scb->scb_sscb.sscb_ics.ics_cur_priority > dbpriv->priority_limit) 
    	{
	    /* 
	    ** Current session priority would exceed the limit so we
	    ** change the priority lower to be within the limit
	    */
            (VOID)scs_set_priority(scb, dbpriv->priority_limit);
	} 
    }
    /*
    ** Call QEF to stash the session's database privileges
    ** in the QEF session control block.  Note that QEF keeps
    ** it's own copy of the database privileges.
    */

    qef_alt[0].alt_code = QEC_DBPRIV;
    qef_alt[0].alt_value.alt_u_i4
	    = scb->scb_sscb.sscb_ics.ics_fl_dbpriv;
    qef_alt[1].alt_code = QEC_QROW_LIMIT;
    qef_alt[1].alt_value.alt_i4
	    = scb->scb_sscb.sscb_ics.ics_qrow_limit;
    qef_alt[2].alt_code = QEC_QDIO_LIMIT;
    qef_alt[2].alt_value.alt_i4
	    = scb->scb_sscb.sscb_ics.ics_qdio_limit;
    qef_alt[3].alt_code = QEC_QCPU_LIMIT;
    qef_alt[3].alt_value.alt_i4
	    = scb->scb_sscb.sscb_ics.ics_qcpu_limit;
    qef_alt[4].alt_code = QEC_QPAGE_LIMIT;
    qef_alt[4].alt_value.alt_i4
	    = scb->scb_sscb.sscb_ics.ics_qpage_limit;
    qef_alt[5].alt_code = QEC_QCOST_LIMIT;
    qef_alt[5].alt_value.alt_i4
	    = scb->scb_sscb.sscb_ics.ics_qcost_limit;

    qef_cb.qef_length = sizeof(QEF_RCB);
    qef_cb.qef_type = QEFRCB_CB;
    qef_cb.qef_ascii_id = QEFRCB_ASCII_ID;
    qef_cb.qef_sess_id = scb->cs_scb.cs_self;
    qef_cb.qef_asize = 6;
    qef_cb.qef_alt = qef_alt;
    qef_cb.qef_eflag = QEF_EXTERNAL;
    qef_cb.qef_cb = scb->scb_sscb.sscb_qescb;

    status = qef_call(QEC_ALTER, &qef_cb);
    if (status)
    {
	sc0e_0_put(qef_cb.error.err_code, 0);
	scd_note(status, DB_SCF_ID);
	return(status);
    }
    return E_DB_OK;
}

/*
** Name: scs_init_dbpriv - Initiates dbpriv values in a SCS_DBPRIV structure
**
** Description:
**	This initiates the values in a SCS_DBPRIV structure appropriately
**
** Inputs:
**	scb	- SCB
**
**	dbpriv  - dbpriv structure
**
**	all_priv- If TRUE set to all privileges (default is none)
**
** Outputs:
**	dbpriv  - initialized
**
** History:
**	18-mar-94 (robf)
**          Created
*/
VOID
scs_init_dbpriv(SCD_SCB *scb, SCS_DBPRIV *dbpriv, bool all_priv)
{
    if(all_priv)
    {
	    dbpriv->ctl_dbpriv  = DBPR_ALL;
	    /* 
	    ** Add priority limit here since the enables rather than
	    ** disables the use of the ability
	    */
	    dbpriv->fl_dbpriv   = (DBPR_BINARY|DBPR_PRIORITY_LIMIT);
	    dbpriv->qrow_limit  = -1;	
	    dbpriv->qdio_limit  = -1;	
	    dbpriv->qcpu_limit  = -1;	
	    dbpriv->qpage_limit = -1;	
	    dbpriv->qcost_limit = -1;
	    dbpriv->idle_limit    = -1;
	    dbpriv->connect_limit = -1;
	    dbpriv->priority_limit=  Sc_main_cb->sc_max_usr_priority;
    }
    else
    {
	    dbpriv->ctl_dbpriv  = 0;
	    dbpriv->fl_dbpriv   = 0;
	    dbpriv->qrow_limit	= -1;
	    dbpriv->qdio_limit	= -1;
	    dbpriv->qcpu_limit  = -1;
	    dbpriv->qpage_limit = -1;
	    dbpriv->qcost_limit = -1;
	    dbpriv->idle_limit  = -1;
	    dbpriv->connect_limit    = -1;
	    dbpriv->priority_limit    = 0;
    }
}

/*
** Name; scs_cvt_dbpr_tuple - Convert dbprivileges tuple
**
** Description:
**	Converts values in a DB_PRIVILEGES tuple to SCS_DBPRIV structure
**
** Inputs:
**	scb	 - SCB
**
**	indbpriv - input value
**
** Outputs:
**	outdbpriv - output value
**
** Returns
**	none
**
** History:
**	18-mar-94 (robf)
**	    Created
*/
VOID
scs_cvt_dbpr_tuple(SCD_SCB *scb, 
	DB_PRIVILEGES *indbpriv,
	SCS_DBPRIV    *outdbpriv
)
{
    outdbpriv->ctl_dbpriv  = indbpriv->dbpr_control;
    outdbpriv->fl_dbpriv   = indbpriv->dbpr_flags;
    outdbpriv->qrow_limit  = indbpriv->dbpr_qrow_limit;
    outdbpriv->qdio_limit  = indbpriv->dbpr_qdio_limit;
    outdbpriv->qcpu_limit  = indbpriv->dbpr_qcpu_limit;
    outdbpriv->qpage_limit = indbpriv->dbpr_qpage_limit;
    outdbpriv->qcost_limit = indbpriv->dbpr_qcost_limit;
    outdbpriv->idle_limit    = indbpriv->dbpr_idle_time_limit;
    outdbpriv->connect_limit = indbpriv->dbpr_connect_time_limit;
    outdbpriv->priority_limit=  indbpriv->dbpr_priority_limit;
	
}
