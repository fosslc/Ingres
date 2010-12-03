/*
**Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <iicommon.h>
#include <sl.h>
#include <dbdbms.h>
#include <pc.h>
#include <cs.h>
#include <er.h>
#include <tm.h>
#include <di.h>
#include <lo.h>
#include <lk.h>
#include <st.h>
#include <ulf.h>
#include <ulm.h>
#include <sxf.h>
#include <sxfint.h>
#include <sxap.h>
#include <sa.h>
#include "sxapoint.h"
/*
** Name: SXAPOC.C - control routines for SXAPO auditing system, to write to
**                  the operating system audit logfile.
**
** Description:
**	This file contains all the control routines used by the SXF low
**	level auditing system SXAPO (the operating system audit log version
**	of SXAP).
**
**	sxapo_startup    - initialize the auditing system.
**	sxapo_bgn_ses    - register a session with the auditing system.
**	sxapo_shutdown   - shutdown the auditing system
**	sxapo_end_ses    - remove a session from the auditing system
**	sxapo_show	 - return data about the sxapo configuration
**	sxapo_init_cnf   - Initialize SXAPO configuration from PM.
**	sxapo_alter	 - Alter the state of SXAPO.
**
** History:
**	1-dec-93 (stephenb)
**	    initial creation.
**	4-feb-94 (stephenb)
**	    Add sxapo_alter entry point to the sxap vector, and code
**	    sxapo_alter routine.
**	23-sep-1996 (canor01)
**	    Move global data definitions to sxapodata.c.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add missing prototypes
[@history_template@]...
*/

/*
** Global variables owned by this module
*/
GLOBALREF SXAPO_CB *Sxapo_cb;

static DB_STATUS sxapo_init_cnf( i4 *err_code );

/*
** Name: sxapo_bgn_ses - register a session with the auditing system
**
** Description:
**	This routine is called at session startup time to initialize the
**	audit specific data structures associated with a session. It should
**	be called only once for each session.
**
**	The only session specific data structure need for the operating system
**	audit log version of auditing is the record identifier that keeps
**	count of the number of records written since the last flush for this
**	session.
**
** Inputs:
**	scb		SXF session control block
**
** Outputs:
**	err_code	Error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	1-dec-93 (stephenb)
**	    Initial creation.
**
*/
DB_STATUS
sxapo_bgn_ses (
    SXF_SCB	*scb,
    i4	*err_code)
{
    ULM_RCB		ulm_rcb;
    DB_STATUS		status;
    i4		local_err;

    *err_code = E_SX0000_OK;

    ulm_rcb.ulm_facility = DB_SXF_ID;
    ulm_rcb.ulm_streamid_p = &scb->sxf_scb_mem;
    ulm_rcb.ulm_psize = sizeof (SXAPO_RECID);
    ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
    status = ulm_palloc(&ulm_rcb);
    if (status != E_DB_OK)
    {
	if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	    *err_code = E_SX106B_MEMORY_ERROR;
	else
	    *err_code = E_SX106B_MEMORY_ERROR;
	_VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 0);
    }
    else
	scb->sxf_sxap_scb = ulm_rcb.ulm_pptr;
    /* handle errors */
    if (*err_code != E_SX0000_OK)
    {
	_VOID_ ule_format(*err_code, NULL, ULE_LOG, NULL, NULL,
		0L, NULL, &local_err, 0);
	*err_code = E_SX101D_SXAP_BGN_SESSION;
	status = E_DB_ERROR;
    }

    return(status);
}

/*
** Name: sxapo_end_ses - remove a session from the auditing system
**
** Description:
**	This routine should be called at session end time to release the
**	audit specific data structures associated with a session. It should
**	be called only once for each session.
**
**	In sxapo, no session specific rundown is required, this routine is
**	a stub.
**
** Inputs:
**	scb		SXF session control block
**
** Outputs:
**	err_code	Error code returned to the caller (always E_SX0000_OK)
**
** Returns:
**	DB_STATUS
**
** History:
**	1-dec-93 (stephenb)
**	    Written as a stub.
**
*/
DB_STATUS
sxapo_end_ses (
    SXF_SCB	*scb,
    i4	*err_code)
{
    *err_code = E_SX0000_OK;
    return(E_DB_OK);
}

/*
** Name: sxapo_startup - startup the low level auditing system
**
** Description:
**
**	This routine initializes the operating system audit log version of
**	the SXAP auditing system. It should
**	be called once at SXF startup time. Its purpose is to allocate
**	and initialize the resources needed for the running of the low level
**	auditing system. 
**
**	This routine also initializes the vector used to access all other 
**	SXAP routines, all these routines must be called using this vector.
**
** Inputs:
**	None.
**
** Outputs:
**	rscb			RSCB for the current write audit file.
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	2-dec-93 (stephenb)
**	    first written.
**	4-jan-94 (stephenb)
**	    Add sxapo_alter entry point to the sxap vector.
*/
DB_STATUS
sxapo_startup(
    SXF_RSCB	*rscb,
    i4 	*err_code)
{
    DB_STATUS	    status = E_DB_OK;
    i4	    local_err;
    SXAPO_RSCB	    *rs;
    ULM_RCB	    ulm_rcb;
    SXAP_VECTOR	    *v;
    bool	    file_open = FALSE;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	/* Build the SXAP main control block */
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_streamid_p = &Sxf_svcb->sxf_svcb_mem;
	ulm_rcb.ulm_psize = sizeof (SXAPO_CB);
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	status = ulm_palloc(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		*err_code = E_SX1002_NO_MEMORY;
	    else
		*err_code = E_SX106B_MEMORY_ERROR;

	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	Sxapo_cb = (SXAPO_CB *) ulm_rcb.ulm_pptr;
	Sxapo_cb->sxapo_curr_rscb = NULL;


	/* Build and initialize the SXAP call vector */
	ulm_rcb.ulm_psize = sizeof (SXAP_VECTOR);
	status = ulm_palloc(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		*err_code = E_SX1002_NO_MEMORY;
	    else
		*err_code = E_SX106B_MEMORY_ERROR;

	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	Sxap_vector = (SXAP_VECTOR *) ulm_rcb.ulm_pptr;

	v = Sxap_vector;
	v->sxap_init = sxapo_startup;
	v->sxap_term = sxapo_shutdown;
	v->sxap_begin = sxapo_bgn_ses;
	v->sxap_end = sxapo_end_ses;
	v->sxap_open = sxapo_open;
	v->sxap_close = sxapo_close;
	v->sxap_pos = sxapo_position;
	v->sxap_read = sxapo_read;
	v->sxap_write = sxapo_write;
	v->sxap_flush = sxapo_flush;
	v->sxap_show = sxapo_show;
	v->sxap_alter = sxapo_alter;


	/* Initialize the audit file configuration data */
	status = sxapo_init_cnf(&local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/* 
	** Open the audit file for writing.
	*/
	status = sxapo_open( NULL, SXF_WRITE, rscb, NULL, NULL, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	     _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	file_open = TRUE;
	rs = (SXAPO_RSCB *) rscb->sxf_sxap;
	Sxapo_cb->sxapo_curr_rscb = rs;

	Sxapo_cb->sxapo_status |= SXAPO_ACTIVE;
	break;
    }

    /* Cleanup after any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (file_open)
	    _VOID_ sxapo_close(rscb, &local_err);


	*err_code = E_SX101A_SXAP_STARTUP;

	/*
	** Errors during startup of the auditing system are 
	** considered to be severe errors.
	*/
	status = E_DB_SEVERE;
    }
 
    return (status);
}

/*
** Name: sxapo_shutdown - Shut down the low level audititng system
**
** Description:
**	This routine is called to shutdown the low level auditing system, it
**	should be called only once at facility shutdown time. The function of
** 	the routine is to free any resources allocated in the low level
**	auditing system.
**
**	Since the version of the low level audititng system that writes to
**	the operating system audit logs does not allocate any resources in
**	sxapo_startup() this routine is virtually a no-op and just needs to
**	re-set some values in Sxapo_cb.
**
** Inputs:
**	none.
**
** Outputs:
**	err_code	error code terurned to the caller (always E_SX0000_OK)
**
** Returns:
**	DB_STATUS
**
** History:
**	2-dec-93 (stephenb)
**	    First written.
**
*/
DB_STATUS
sxapo_shutdown(
    i4	*err_code)
{
    Sxapo_cb->sxapo_status &= ~SXAPO_ACTIVE;

    return(E_DB_OK);
}
/*
** Name: sxapo_show - return data about the sxapo configuration.
**
** Description:
**	This routine is used to return data about the SXAPO configuation.
**	The routine is currently a no-op and will return null filename and
**	max file size.
**
** Inputs:
**	filename	pointer to the current audit file
**
** Outputs:
**	filename	name of current audit file 
**	max_file	maximum size of an audit file (always 0)
**	err_code	error code for the call (always E_SX0000_OK)
**
** Returns:
**	DB_STATUS
**
** History:
**	2-dec-93 (stephenb)
**	    Initial creation.
**	7-feb-94 (stephenb)
**	    Check if filename address has been set before setting value to NULL.
**	15-mar-94 (robf)
**          Return filename of OS audit file, lowers confusion since
**	    blank could mean no auditing.
**	    Also only set max_file if available.
**
*/
DB_STATUS
sxapo_show(
    PTR		filename,
    i4	*max_file,
    i4	*err_code)
{
    if (filename)
    {
	STcopy("<OS Audit File>", filename);
    }
    if(max_file)
	    *max_file = 0;
    *err_code = E_SX0000_OK;
    return(E_DB_OK);
}

/*
** Name: sxapo_init_cnf - Initialize SXAPO configuration from the PM file.
**
** Description:
**	This routine initializes the SXAPO configuration from the
**	PM configuration file.
**
** Inputs:
**	None.
**
** Outputs:
**	err_code		Error code returned to caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	6-jan-94 (stephenb)
**	    Initial creation.
*/
static DB_STATUS
sxapo_init_cnf(
	i4		*err_code)
{
    DB_STATUS		status = E_DB_OK;
    STATUS		clstat;
    i4		local_err;
    char		*pmfile;
    char		*pmvalue;

    *err_code = E_SX0000_OK;
    for (;;)
    {
    	/*
    	** Allow private override on PM file
    	*/
	NMgtAt("II_SXF_PMFILE", &pmfile);
	if (pmfile && *pmfile)
	{
		LOCATION	pmloc;
		TRdisplay("Loading SXF-PM file '%s'\n",pmfile);
		LOfroms(PATH & FILENAME, pmfile, &pmloc);
		if(PMload(&pmloc,NULL)!=OK)
		TRdisplay("Error loading PMfile '%s'\n",pmfile);
	}
	/*
	** Get auditing status
	*/
	if (PMget(SX_C2_MODE,&pmvalue) == OK)
	{
		if (!STbcompare(pmvalue, 0, SX_C2_MODE_ON, 0, TRUE))
		{
		    /*
		    ** Auditing on
		    */
		    Sxapo_cb->sxapo_status=SXAPO_ACTIVE;
		}
		else if ((STbcompare(pmvalue, 0, SX_C2_MODE_OFF, 0, TRUE)!=0))
		{
		    /*
		    ** Niether ON nor OFF, Invalid mode
		    */
		    *err_code=E_SX1061_SXAP_BAD_MODE;
		    break;
		}
	}
	else
	{
		/*
		** No value, this is an error
		*/
		*err_code=E_SX1061_SXAP_BAD_MODE;
		break;
	}
	/*
	** Get action on error
	*/
	if ((PMget("II.*.C2.ON_ERROR",&pmvalue) == OK))
	{
		if (!STcasecmp(pmvalue, "STOPAUDIT" ))
		{
		    Sxf_svcb->sxf_act_on_err=SXF_ERR_STOPAUDIT;
		}
		else if (!STcasecmp(pmvalue, "SHUTDOWN" ))
		{
		    Sxf_svcb->sxf_act_on_err=SXF_ERR_SHUTDOWN;
		}
		else
		{
		/*
		** Invalid value
		*/
		*err_code=E_SX1060_SXAP_BAD_ONERROR;
		break;
		}
	}
	else
	{
		/*
		** No value, this is an error
		*/
		*err_code=E_SX1060_SXAP_BAD_ONERROR;
		break;
	}
	break;
    }
    /* handle errors */
    if (*err_code != E_SX0000_OK)
    {
	_VOID_ ule_format(*err_code, NULL, ULE_LOG, NULL, NULL,
		0L, NULL, &local_err, 0);
	*err_code = E_SX1020_SXAP_INIT_CNF;
	status = E_DB_ERROR;
    }
    return (status);
}

/*
** Name: sxapo_alter - alter the state of sxapo
**
** Description:
**	Alter the SXAPO state, Some of that options available in the version
**	of this routine coded for ingres audit logs (sxap_alter) are not 
**	available here and will produce an error.
**
** Inputs:
**	filename	- Ignored, we do not have filenames in os auditing
**	flags		- Operation flags
**
** Outputs:
**	err_code 	- Error code.
**
** Returns:
**	DB_STATUS
**
** History:
**	4-jan-94 (stephenb)
**	    Initial creation.
*/
DB_STATUS
sxapo_alter (
	SXF_SCB 	*scb,
	PTR     	filename,
	i4 	flags,
	i4 	*err_code)
{
    SXF_RSCB	*rscb = Sxf_svcb->sxf_write_rscb;
    DB_STATUS	status = E_DB_OK;

    for (;;)
    {
	/*
	** Check for action to perform
	*/
	if (flags & SXAP_C_STOP_AUDIT)
	{
	    if (!(Sxapo_cb->sxapo_status & SXAPO_ACTIVE))
		break;
	    /*
	    ** Close audit trail
	    */
	    status = sxapo_close(rscb, err_code);
	    if (status != E_DB_OK)
		break;
	    Sxapo_cb->sxapo_status &= ~SXAPO_ACTIVE;
	}
	else if (flags & SXAP_C_RESTART_AUDIT)
	{
	    if (Sxapo_cb->sxapo_status & SXAPO_ACTIVE)
		break;
	    /*
	    ** User is not allowed to stipulate an audit file for OS auditing
	    */
	    if (filename)
	    {
		*err_code = E_SX10B8_OSAUDIT_NO_CHANGEFILE;
		status = E_DB_ERROR;
		break;
	    }
	    /*
	    ** Re-open audit trail
	    */
	    status = sxapo_open(NULL, SXF_WRITE, rscb, NULL, NULL, err_code);
	    if (status != E_DB_OK)
		break;
	    Sxapo_cb->sxapo_curr_rscb = (SXAPO_RSCB *) rscb->sxf_sxap;
	    Sxapo_cb->sxapo_status |= SXAPO_ACTIVE;
	}
	else if (flags & SXAP_C_CHANGE_FILE)
	{
	    /*
	    ** Not a valid opertion here, return an error
	    */
	    *err_code = E_SX10B8_OSAUDIT_NO_CHANGEFILE;
	    status = E_DB_ERROR;
	    break;
	}
	break;
    }
    return status;
}
