/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmccb.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmftrace.h>
#include    <dm0llctx.h>

/**
** Name: DMCLWRT.C  - The log writer threads of the server
**
** Description:
**    
**      dmc_logwriter()  -  A logfile writer thread in the server
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system
**      8-nov-1992 (ed)
**          remove DB_MAXNAME dependency
**	3-nov-1992 (bryanp)
**	    We don't need a lock list, so don't create one.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	18-jan-1993 (bryanp)
**	    Function protoypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <lgdef.h>.
**	19-oct-1993 (rmuth)
**	    Remove <lgdef.h> this has a GLOBALREF in it which produces a
**	    link warning on VMS.
**      27-Jul-1998 (horda03)
**          The "error code" returned by dmc_logwriter() when an error has
**          occurred can be set to 0. (Bug 92183).
**      22-Feb-1999 (bonro01)
**          Replace references to svcb_log_id with
**          dmf_svcb->svcb_lctx_ptr->lctx_lgid
**	15-Dec-1999 (jenjo02)
**	    Added DB_DIS_TRAN_ID parm to LGbegin() prototype.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**	12-Apr-2008 (kschendel)
**	    Need to include adf.h now.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**/

/*{
**
** Name: dmc_logwriter	-- Performs I/O to the transaction logfile
**
** EXTERNAL call format:	status = dmf_call(DMC_LOGWRITER, &dmc_cb);
**
** Description:
**	This thread writes log blocks to the log file.
**
**	If all goes well, this routine will not return under normal
**	circumstances until server shutdown time.
**
** Inputs:
**     dmc_cb
** 	.type		    Must be set to DMC_CONTROL_CB.
** 	.length		    Must be at least sizeof(DMC_CB).
**
** Outputs:
**     dmc_cb
** 	.error.err_code	    One of the following error numbers.
**			    E_DB_OK
**			    E_DM004A_INTERNAL_ERROR
**
** Returns:
**     E_DB_OK
**     E_DB_FATAL
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system
**	3-nov-1992 (bryanp)
**	    We don't need a lock list, so don't create one.
**	10-oct-93 (swm)
**	    Bug #56438
**	    Put LG_DBID into autmatic variable lg_dbid rather than overloading
**	    dmc_cb->dmc_db_id.
**      27-Jul-1998 (horda03)
**          When an error is detected, do not use error_code to store the status
**          of the ule_format() call. This variable contains the error code to be
**          returned to the calling function. (Bug 92183)
*/
DB_STATUS
dmc_logwriter(DMC_CB	    *dmc_cb)
{
    DMC_CB	    *dmc = dmc_cb;
    DM_SVCB	    *svcb = dmf_svcb;
    i4	    	    error_code;
    DB_STATUS	    status = E_DB_OK;
    STATUS	    cl_status;
    CL_ERR_DESC	    sys_err;
    DB_TRAN_ID	    tran_id;
    LG_LXID	    lx_id;
    DM0L_ADDDB	    add_info;
    i4		    len_add_info;
    i4		    have_transaction = FALSE;
    STATUS	    stat;
    i4	    error;
    DB_OWN_NAME	    user_name;
    LG_DBID	    lg_dbid;

#ifdef xDEBUG
    TRdisplay("Starting server Logwriter Thread for server id 0x%x\n",
	dmc->dmc_id);
#endif

    CLRDBERR(&dmc->error);

    STmove((PTR)DB_LOGFILEIO_THREAD, ' ', sizeof(add_info.ad_dbname),
	(PTR) &add_info.ad_dbname);
    MEcopy((PTR)DB_INGRES_NAME, sizeof(add_info.ad_dbowner),
	(PTR) &add_info.ad_dbowner);
    MEcopy((PTR)"None", 4, (PTR) &add_info.ad_root);
    add_info.ad_dbid = 0;
    add_info.ad_l_root = 4;
    len_add_info = sizeof(add_info) - sizeof(add_info.ad_root) + 4;

    stat = LGadd(dmf_svcb->svcb_lctx_ptr->lctx_lgid, LG_NOTDB, (char *)&add_info,
		    len_add_info, &lg_dbid, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM900A_BAD_LOG_DBADD, &sys_err, ULE_LOG, NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &error, 4, 0,
	    dmf_svcb->svcb_lctx_ptr->lctx_lgid,
	    sizeof(add_info.ad_dbname), (PTR) &add_info.ad_dbname,
	    sizeof(add_info.ad_dbowner), (PTR) &add_info.ad_dbowner,
	    4, (PTR) &add_info.ad_root);
	if (stat == LG_EXCEED_LIMIT)
	    SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	else
	    SETDBERR(&dmc->error, 0, E_DM014D_LOGWRITER);
	return (E_DB_ERROR);
    }

    for (;;)
    {
	STmove((PTR)DB_LOGFILEIO_THROWN, ' ', sizeof(DB_OWN_NAME),
		(PTR) &user_name);
	stat = LGbegin(LG_NOPROTECT | LG_LOGWRITER_THREAD,
			lg_dbid, &tran_id, &lx_id, sizeof(DB_OWN_NAME),
			&user_name.db_own_name[0], 
			(DB_DIS_TRAN_ID*)NULL, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900C_BAD_LOG_BEGIN, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lg_dbid);
	    if (stat == LG_EXCEED_LIMIT)
		SETDBERR(&dmc->error, 0, E_DM0062_TRAN_QUOTA_EXCEEDED);
	    else
		SETDBERR(&dmc->error, 0, E_DM014D_LOGWRITER);
	    status = E_DB_ERROR;
	    break;
	}
	have_transaction = TRUE;

	break;
    }

    for (;status == E_DB_OK;)
    {

	/*
	** The logwriter thread is awakened when a log block needs to be
	** written. It then calls LG_write_block() to write the block out.
	** Then it goes back to sleep until the next log block needs to be
	** written out.
	*/
	cl_status = LG_write_block(lx_id, &sys_err);

	if (cl_status)
	{
	    TRdisplay("Logwriter thread: error %x from LG_write_block.\n",
			cl_status);
	    /*
	    ** Something is fatally wrong in the CL.
	    */
	    uleFormat(NULL, cl_status, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    SETDBERR(&dmc->error, 0, E_DM014D_LOGWRITER);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Wait for next buffer to write:
	*/
	cl_status = CSsuspend(CS_INTERRUPT_MASK, 0, 0);

	if (cl_status == E_CS0008_INTERRUPTED)
	{
	    /*
	    ** Special threads are interrupted when they should shut down.
	    */

	    /* display a message? */
	    TRdisplay("Logwriter Thread was interrupted; terminating...\n");
	    status = E_DB_OK;
	    break;
	}

	/*
	** loop back around and wait for the next time to write a block:
	*/
    }
    /*
    ** Clean up transaction or lock list left hanging around.
    */
    if (have_transaction)
    {
	stat = LGend(lx_id, 0, &sys_err);
	if (stat != OK)
	{
	    uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM900E_BAD_LOG_END, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lx_id);
	    if ( status == E_DB_OK )
	    {
		SETDBERR(&dmc->error, 0, E_DM014D_LOGWRITER);
		status = E_DB_ERROR;
	    }
	}
    }

    stat = LGremove(lg_dbid, &sys_err);
    if (stat != OK)
    {
	uleFormat(NULL, stat, (CL_ERR_DESC *)&sys_err, ULE_LOG, NULL,
			(char *)NULL, 0L, (i4 *)NULL, &error, 0);
	uleFormat(NULL, E_DM9016_BAD_LOG_REMOVE, &sys_err, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 1, 0, lg_dbid);
	if ( status == E_DB_OK )
	{
	    SETDBERR(&dmc->error, 0, E_DM014D_LOGWRITER);
	    status = E_DB_ERROR;
	}
    }

    return (status);
}
