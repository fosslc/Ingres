/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <pc.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <ulf.h>
# include    <cs.h>
# include    <lk.h>
# include    <ex.h>
# include    <lo.h>
# include    <tm.h>
# include    <tr.h>
# include    <st.h>
# include    <me.h>
# include    <ulx.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <scf.h>

/*
**
**  Name: SXFCALL.C - The main SXF entry point.
**
**  Description:
**	This file contains the routine that implements the SXF call
**	interface.
**
**          sxf_call   - The primary SXF entry point.
**	    ex_handler - The SXF exception handler.
**
**
**  History:
**	9-july-1992 (markg)
**	    Initial creation.
**	29-july-1992 (pholman)
**	    Primative interface for initial integration.
**	20-oct-1992 (markg)
**	    Updated to have entry points for all SXF operations.
**	10-mar-1993 (markg)
**	    Fixed typo in ex_handler().
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-apr-1994 (robf)
**	    Added SXM/SXL/SXC interface requests for Secure 2.0
**	11-feb-94 (stephenb)
**	    Added routine sxf_reset to reset a counter under semaphore 
**	    protection.
**	14-apr-94 (robf)
**          Added SXC_AUDIT_WRITER_THREAD call
**	04-Aug-1997 (jenjo02)
**	    Removed sxf_incr(), sxf_reset(). These functions were being used soley
**	    to increment and decrement ordinary statistics, not critical
**	    variables. The semaphore sxf_incr_sem was not being initialized
**	    and so was useless on a threaded server anyway.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
*/

/*
**  Definition of static variables and forward static functions.
*/

static  STATUS  ex_handler(EX_ARGS *ex_args);  /* SXF exception handler */


/*{
** Name: sxf_call	- The main SXF entry point.
**
** Description:
**      The routine checks that the arguments to sxf_call look reasonable.
**	The implementing function is then called and operation completion
**	status is returned to the caller.
**
** Inputs:
**      op_code			The SXF operation code.
**      rcb			The SXF request control block for the operation.
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-July-1992 (markg)
**	    Initial creation.
**	03-sep-1992 (pholman)
**	    Give calls NULL functionlaity for initial integration
**	20-oct-1992 (markg)
**	    Updated to have entry points for all SXF operations.
*/
DB_STATUS
sxf_call(
    SXF_OPERATION	op_code, 
    SXF_RCB		*rcb)
{
    DB_STATUS	    	status = E_DB_OK;
    EX_CONTEXT	    	context;
    i4	    	error;
	
    CLRDBERR(&rcb->sxf_error);

    /*	Make a cursury check for bad parameters. */
    if (op_code < SXF_MIN_OPCODE 
     || op_code > SXF_MAX_OPCODE 
     || rcb->sxf_cb_type != SXFRCB_CB 
     || rcb->sxf_length != sizeof (SXF_RCB) 
     || (Sxf_svcb == NULL && op_code != SXC_STARTUP))
    {
	/*  Figure out the error in more detail. */
	if (op_code < SXF_MIN_OPCODE || op_code > SXF_MAX_OPCODE)
	    SETDBERR(&rcb->sxf_error, 0, E_SX0001_BAD_OP_CODE);
	else if (rcb->sxf_cb_type != SXFRCB_CB)
	    SETDBERR(&rcb->sxf_error, 0, E_SX0002_BAD_CB_TYPE);
	else if (rcb->sxf_length != sizeof (SXF_RCB))
	{
	    TRdisplay("Bad SXF CB length. Input length %d expected %d\n",
		rcb->sxf_length, sizeof(SXF_RCB));
	    SETDBERR(&rcb->sxf_error, 0, E_SX0003_BAD_CB_LENGTH);
	}
	else
	    SETDBERR(&rcb->sxf_error, 0, E_SX000F_SXF_NOT_ACTIVE);

	return (E_DB_ERROR);
    }

    if (EXdeclare(ex_handler, &context) == OK 
     && (Sxf_svcb == NULL || (Sxf_svcb->sxf_svcb_status & SXF_CHECK) == 0))
    {
	switch (op_code)
	{
	/* Control operations. */
	case SXC_STARTUP:
	    status = sxc_startup(rcb);
	    break;

	case SXC_SHUTDOWN:
	    status = sxc_shutdown(rcb);
	    break;

	case SXC_BGN_SESSION:
	    status = sxc_bgn_session(rcb);
	    break;

	case SXC_END_SESSION:
	    status = sxc_end_session(rcb);
	    break;
	
	case SXC_ALTER_SESSION:
	    status = sxc_alter_session(rcb);
	    break;

	case SXC_AUDIT_THREAD:
	    status = sxac_audit_thread(rcb);
	    break;

	case SXC_AUDIT_WRITER_THREAD:
	    status = sxac_audit_writer_thread(rcb);
	    break;

	/* Audit file oerations */
	case SXA_OPEN:
	    status = sxaf_open(rcb);
	    break;

	case SXA_CLOSE:
	    status = sxaf_close(rcb);
	    break;

	/* Audit record operations */
	case SXR_WRITE:
	    status = sxar_write(rcb);
	    break;

	case SXR_POSITION:
	    status = sxar_position(rcb);
	    break;

	case SXR_READ:
	    status = sxar_read(rcb);
	    break;

	case SXR_FLUSH:
	    status = sxar_flush(rcb);
	    break;

	/* Audit state operations */
	case SXS_ALTER:
	    status = sxas_alter(rcb);
	    break;

	case SXS_SHOW:
	    status = sxas_show(rcb);
	    break;
	}

	EXdelete();
        return (status);
    }

    /*
    ** If exception handler declares or the SXF_SVCB has already been 
    ** marked inconsistent, this is a server fatal condition. In most 
    ** cases it is sufficient to return a server fatal error, and let the 
    ** caller handle the rest. 
    **
    ** However, if this is an audit record write operation we have to 
    ** nuke the server ourselves. The reason for this is that if the
    ** client code does not handle the return status correctly the 
    ** security of the system could be compromised.
    */
    EXdelete();

    if (op_code == SXR_WRITE)
    {
	_VOID_ ule_format(E_SX0005_INTERNAL_ERROR, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
	_VOID_ ule_format(E_SX1048_SERVER_TERMINATE, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
	_VOID_ CSterminate(CS_KILL, NULL);
    }

    SETDBERR(&rcb->sxf_error, 0, E_SX0005_INTERNAL_ERROR);
    return (E_DB_FATAL);
}

/*
** Name: ex_handler	- SXF exception handler.
**
** Description:
**      This routine will catch all SXF exceptions. Any exception caught
**	by SXF is considered severe. SXF is marked as inconsistent and
**	all future callers will be returned an error.
**
** Inputs:
**      ex_args                         Exception arguments.
**
** Outputs:
**	none
**
** Returns:
**	EXDECLARE
**
** History:
**	9-july-1992 (markg)
**	    Initial creation.
**	12-jan-1993 (robf)
**	    Upgrade to new exception handling standard using ulx_exception()
**	    call.
**	10-mar-1993 (markg)
**	    Fixed typo in call to ulx_exception().
**	25-may-1993 (robf)
**	     New error message with appropriate params (fixes ule_format
**	     error on exceptions )
*/
static STATUS
ex_handler(
    EX_ARGS		*ex_args)
{
    /*
    ** Report the exception
    */
    ulx_exception (ex_args, DB_SXF_ID, E_SX10A1_SXF_EXCEPTION, TRUE);

    if (Sxf_svcb != NULL)
	Sxf_svcb->sxf_svcb_status |= SXF_CHECK;

    return (EXDECLARE);
}

/*
** sxf_set_activity - Set up the server activity, so user can see
** what we are doing.
**
** Inputs:
**	A string corrosponding to the activity to be set. The user will
**	see this in the output of "iimonitor" etc for the thread's activity.
**
** Outputs:
**	None
**
** History:
**	11-dec-92 (robf) 
**		Created
*/
VOID
sxf_set_activity(char *mesg)
{
	SCF_CB 	  scb;
	DB_STATUS status;
	i4   error;

	scb.scf_type=SCF_CB_TYPE;
	scb.scf_length=SCF_CB_SIZE;
	scb.scf_session=DB_NOSESSION;
	scb.scf_len_union.scf_blength=STlength(mesg);
	scb.scf_nbr_union.scf_atype = SCS_A_MAJOR;
	scb.scf_ptr_union.scf_buffer=mesg;
	scb.scf_facility=DB_SXF_ID;

	status=scf_call(SCS_SET_ACTIVITY, &scb);
	if (status!=E_DB_OK)
	{
		/*
		**	Log something went wrong and continue.
		*/
		_VOID_ ule_format(scb.scf_error.err_code, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
		_VOID_ ule_format(E_SX1062_SCF_SET_ACTIVITY, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
	}
}

/*
** sxf_clr_activity - Clear the server activity.
**
** History:
**	11-dec-92 (robf) 
**		Created
** Inputs:
**	None
**
** Outputs:
**	None
**
*/
VOID
sxf_clr_activity(void)
{
	SCF_CB 		scb;
	DB_STATUS 	status;
        i4	    	error;

	scb.scf_len_union.scf_blength=0;
	scb.scf_nbr_union.scf_atype = SCS_A_MAJOR;
	scb.scf_type=SCF_CB_TYPE;
	scb.scf_length=SCF_CB_SIZE;
	scb.scf_session=DB_NOSESSION;
	scb.scf_facility=DB_SXF_ID;

	status=scf_call(SCS_CLR_ACTIVITY, &scb);
	if (status!=E_DB_OK)
	{
		/*
		**	Log something went wrong and continue.
		*/
		_VOID_ ule_format(scb.scf_error.err_code, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
		_VOID_ ule_format(E_SX1063_SCF_CLR_ACTIVITY, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &error, 0);
	}
}
