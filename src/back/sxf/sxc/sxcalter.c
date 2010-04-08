/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <pc.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <pc.h>
# include    <cs.h>
# include    <lk.h>
# include    <me.h>
# include    <tm.h>
# include    <scf.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>

/*
** Name: SXCALTER.C - SXF alter session routines
**
** Description:
**	This file contains the routines that are used to alter a 
**	running SXF session.
**
**	sxc_alter_session - begin a SXF session.
**
** History:
**	10-may-1993 (robf)
**	    Initial creation.
**	10-oct-93 (robf)
**          Updated include files to remove dbms.h
**	10-nov-93 (robf)
**          ingres_low/high now pure labels, change code accordingly.
**	10-oct-1996 (canor01)
**	    Changed ingres_low/high messages to ca_low/high.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name: SXC_ALTER_SESSION - alter a SXF session
**
** Description:
**
** 	This routine is used to alter an SXF session. It is called at 
**	session alter time via the sxf_call(SXC_BGN_SESSION) request. 
**
** Inputs:
**	rcb
**	    .sxf_server		Pointer to SXF server control block.
**	    .sxf_scb		Pointer to the session control block.
**	    .sxf_sess_id	The session identifier for this session.
**
** Outputs:
**	rcb
**	    .sxf_error		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	10-may-1993 (robf)
**	    Initial creation.
**	28-mar-94 (robf)
**	    Dump dblabel info to trace log if outside ingres_low/high,
**	    this should not normally happen.
*/
DB_STATUS
sxc_alter_session(
    SXF_RCB	*rcb)
{
    DB_STATUS		status = E_DB_OK;
    i4		err_code = E_SX0000_OK;
    i4		local_err;
    SXF_SCB		*scb;
    bool		scb_built = FALSE;

    for (;;)
    {
	/* Go and get the SCB for this user */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    break;
	}
	/*
	** Now set new values.
	** We potentially update:
	** - Privilege mask 
	** - User name 
	** - Database name 
	*/
	/*
	** Setting the database name also means updating the session dbcb.
	** Note that we change the dbname BEFORE to updating the database or
	** user labels to make sure they pick up the new label cache.
	*/
	if(rcb->sxf_db_name && (rcb->sxf_alter & SXF_X_DBNAME))
	{
	        SXF_DBCB	*sxf_dbcb;

		/*
		** Free old db info
		*/
		sxc_freedbcb(scb, scb->sxf_db_info);
		scb->sxf_db_info=(SXF_DBCB*)0;
		/*
		** Get new database info
		*/
		sxf_dbcb=sxc_getdbcb(scb, rcb->sxf_db_name, TRUE, &err_code );
		if (!sxf_dbcb)
		{
			/*
			** Unable to find/allocate a database control block
			*/
			/*
			** Log lower level error
			*/
			_VOID_ ule_format(err_code, 0,
				ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
			
			err_code = E_SX1077_SXC_GETDBCB;
			status=E_DB_ERROR;
			break;
		}
		/*
		** Connect the DBCB to the SCB
		*/
		scb->sxf_db_info=sxf_dbcb;
		/* Increment number of sessions using this db, need semaphore for this*/
		CSp_semaphore(TRUE, &Sxf_svcb->sxf_db_sem);
		sxf_dbcb->sxf_usage_cnt++;	
		CSv_semaphore( &Sxf_svcb->sxf_db_sem);
	}
	if(rcb->sxf_alter & SXF_X_USERPRIV)
		scb->sxf_ustat=rcb->sxf_ustat;
	
	if(rcb->sxf_user && (rcb->sxf_alter & SXF_X_RUSERNAME))
		STRUCT_ASSIGN_MACRO(*rcb->sxf_user, scb->sxf_ruser);

	if(rcb->sxf_user && (rcb->sxf_alter & SXF_X_USERNAME))
		STRUCT_ASSIGN_MACRO(*rcb->sxf_user, scb->sxf_user);

	break;
    }

    /* Clean up after any errors */
    if (err_code != E_SX0000_OK)
    {	
	if (err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    err_code = E_SX107A_SXC_ALTER_SESSION;
	}

	if(status<E_DB_ERROR)
		status=E_DB_ERROR;
    }
    rcb->sxf_error.err_code = err_code;
    return (status);
}
