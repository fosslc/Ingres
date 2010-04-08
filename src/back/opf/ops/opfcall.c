/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#include    <ex.h>
#define	    OPX_EXROUTINES	    TRUE
#include    <opxlint.h> /* EXTERNAL DEFINITION */ 
/**
**
**  Name: OPFCALL.C - CALL AN OPTIMIZER OPERATION
**
**  Description:
**      This file will contain all the procedures necessary to support the
**      OPF_call entry point to the optimizer facility.
**
**  History:    
**      31-dec-85 (seputis)    
**          initial creation
**	8-nov-92 (ed)
**	    add prototype definitions
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      5-dec-96 (stephenb)
**          Add performance profiling code
**	21-may-1997 (shero03)
**	    Added facility tracing diagnostic
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**/

/*
** Name  ops_handler	- last chance exception handler for optimizer
**
** Description:
**      This is the outmost exception handler declared for the optimizer.  This
**      exception handler should never be invoked except in cases of unexpected
**      exceptions.  Each phase of the optimizer will process 
**      its' own errors and exceptions,
**      and this exception handler will only be called if something is
**      terribly wrong (i.e. like an exception within an exception handler )
**
** Inputs:
**      none
**
** Outputs:
**      none
**	Returns:
**	    (return status from opx_exception)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-mar-86 (seputis)
**          initial creation
**	13-mar-92 (rog)
**          Remove EX_DECLARE case since it should never be signalled.
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
*/
static STATUS
ops_handler(
	EX_ARGS			*args)
{
        return (opx_exception(args)); /* handle unexpected exception */
}

/*{
** Name: opf_call - Call an Optimizer operation
**
** External call format:
**	status = (DB_STATUS) opf_call((i4)opcode, (OPF_CB *)opf_cb);
**
** Description:
**	This is the main entry point to the Optimizer facility.  
**
** Inputs:
**      opcode				     Code representing operation to be
**					     performed
**      opf_cb				     Pointer to the OPF control block
**					     containing the parameters. Which
**					     fields are used depends on the
**					     operation.
**
** Outputs:
**      opf_cb
**	    .opf_errorblock                  error status of operation
**                                           - see appendix for error codes
**	Returns:
**	    E_DB_OK			     Function completed normally
**	    E_DB_WARN			     Function completed but with 
**                                           warnings and an 
**					     error code in opf_cb.errorblock
**          E_DB_INFO                        Function completed but with some
**                                           info
**	    E_DB_ERROR			     Function failed - error by caller
**					     error status in opf_cb.errorblock
**                                           or internal error which OPF has
**                                           fully recovered from.
**                                           E_OP0001_USER_ERROR - error
**                                           by user which was already been
**                                           reported by OPF so caller should
**                                           stop processing and not report
**                                           any error
**                                           - otherwise an internal OPF
**                                           inconsistency has been detected
**                                           and has been logged
**          E_DB_SEVERE                      Function failed - internal error
**                                           from which the session may not
**                                           have recovered from i.e. session
**                                           level structures are inconsistent
**	    E_DB_FATAL			     Function failed - internal problem
**					     -OPF server level structures are
**                                           inconsistent
**	Exceptions:
** 
** Side Effects:
**	There may be session state information updated in the OPF server and
**	control blocks associated with this session.  This information
**	will persist between calls to the OPF.  Also, if certain tracing
**	features are selected then some information will be directed towards
**	specified trace files or devices.
** History:
**	31-dec-85 (seputis)
**          initial creation
**	28-jan-91 (seputis)
**	    added support for OPF active flag
[@history_line@]...
*/

DB_STATUS
opf_call(
	i4		   opcode,
	OPF_CB             *opf_cb)
{
    EX_CONTEXT     excontext;		/* context block for exception handler*/
    DB_STATUS	   status;		/* return status for user */
/* initialize error values prior to setting up the exception handler */
#ifdef PERF_TEST
    CSfac_stats(TRUE, CS_OPF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(TRUE, CS_OPF_ID, opcode, opf_cb);
#endif


    CLRDBERR(&opf_cb->opf_errorblock);
    if ( (opcode != OPF_STARTUP)
	&&
	 (opcode != OPF_SHUTDOWN)
	)				/* check if session control block is
                                        ** available */
    {
        if (!opf_cb->opf_scb)
	    opf_cb->opf_scb = (PTR*)ops_getcb(); /* get the session control block
                                        ** since OPF is called directly by
                                        ** PSF which does not have access to it
                                        ** (done when integrities are verified)
                                        ** - this can be removed if PSF does
                                        ** not call OPF directly */
	((OPS_CB *)opf_cb->opf_scb)->ops_retstatus = E_DB_OK;
                                        /* this status could
                                        ** be reset by one of the error handling
                                        ** routines */
	((OPS_CB *)opf_cb->opf_scb)->ops_callercb = opf_cb;
                                        /* error handling routines can find the
                                        ** caller's control block and update
                                        ** appropriate error structure */
    }
    else
        opf_cb->opf_scb = NULL;         /* make sure this is initialized if
                                        ** server startup occurs
                                        */
/*
**      An exception handler is established to catch all exceptions except 
**      those inside the OPF_CREATE_QEP code.  OPF_CREATE_QEP establishes 
**      private exception handlers at appropriate cleanup points.  
*/
    if ( EXdeclare(ops_handler, &excontext) != OK)
    {
	EXdelete();			/* set up new exception to avoid
                                        ** recursive exceptions */
	if ( EXdeclare(ops_handler, &excontext) != OK)
	{   /* if exception handler called again then do not do anything
	    ** except exit as quickly as possible*/
	    status = E_DB_FATAL;
	}
	else
	/* exception handler was invoked */
	    status = opx_status(opf_cb, (OPS_CB *)opf_cb->opf_scb); /* get 
					** status, and set error code if 
					** necessary */
    }
    else
    {
	switch (opcode)
	{
	case OPF_STARTUP:
	    status = ops_startup(opf_cb);
	    break;
	case OPF_SHUTDOWN:
	    status = ops_shutdown(opf_cb);
	    break;
	case OPF_BGN_SESSION:
	    status = ops_bgn_session(opf_cb);
	    break;
	case OPF_END_SESSION:
	    status = ops_end_session(opf_cb);
	    break;
	case OPF_CREATE_QEP:
	    /* This routine will sequence the optimization of the query through
	    ** the various phases.
	    */
	    status = ops_sequencer(opf_cb);
	    break;
	case OPF_ALTER:
	    status = ops_alter(opf_cb);
	    break;
	default:
	    /* invalid opcode - this case should never be reached */
	    opx_rerror(opf_cb, E_OP0007_INVALID_OPCODE);
	    status = E_DB_ERROR;
	}
    }

    if (DB_FAILURE_MACRO(status)
	&&
	(opf_cb->opf_errorblock.err_code != E_OP000A_RETRY)
	&&
	(opf_cb->opf_errorblock.err_code != E_OP000D_RETRY)
	&&
	(opf_cb->opf_errorblock.err_code != E_OP008F_RDF_MISMATCH)
	)
	/* map all non-recoverable errors into E_OP0001_USER_ERROR */
	SETDBERR(&opf_cb->opf_errorblock, 0, E_OP0001_USER_ERROR);
    EXdelete();
#ifdef PERF_TEST
    CSfac_stats(FALSE, CS_OPF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(FALSE, CS_OPF_ID, opcode, opf_cb);
#endif
    return(status);    
}
