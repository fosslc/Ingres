/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <cs.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefdsh.h>
#include    <qefqp.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <ulm.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name:   QEACOMMIT.C		- Commit a transaction as a QP action
**
**  Description:
**	Support for committing a transaction as part of a QP.
**	This currently occurs when a commit is issued within a
**	database procedure.
**
**	    qea_commit		- commit the current transaction and continue
**
**  History:
**	20-apr-89 (paul)
**	    Moved from qeq.c.
**	13-jun-89 (paul)
**	    Fix bug 6346. Do not reset local variables after a commit action.
**	19-jan-90 (sandyh)
**	    Added status change to prevent further action in a procedure
**	    which may lead to problems in DMF. Occurs on return from
**	    qeq_validate with error status E_QE0023_INVALID_QUERY.
**	08-jun-92 (nancy)
**	    Change related to fix for bug 38565.  Include E_QE0301 for
**	    special handling on return from qeq_validate.  Same action as
**	    for E_QE0023.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	20-apr-93 (jhahn)
**	    Added code to check to see if transaction statements should be
**	    allowed.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	16-jul-93 (jhahn)
**	    Added initialization of qef_rcb->qef_modifier before calling
**	    qet_commit()
**	31-jan-94 (jhahn)
**	    Fixed support for resource lists.
**	19-Nov-1997 (jenjo02)
**	    When committing a nested procedure, release table resources in 
**	    stacked procedures as well; the commit has closed all tables.
**      23-Jul-1998 (wanya01)
**          X-integrate change 435146 (sarjo01)
**          Bug 90062: Assure that a new transaction is started by a nested
**          DB proc when its last statement is COMMIT. Otherwise, the calling
**          proc has no open transaction when when it gets control back.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**/

/*{
** Name: QEA_COMMIT		- commit the current transaction
**
** Description:
**	The current transaction is committed and the QP is
**	set to continue execution. This involves re-validating
**	the QP to open any tables used by this QP.
**
** Inputs:
**	action			Delete current row of cursor action.
**      qef_rcb
**	reset
**	state			DSH_CT_INITIAL if this is an action call
**				DSH_CT_CONTINUE for call back after processing
**				a rule action list.
**
** Outputs:
**      qef_rcb
**	    .error.err_code	one of the following
**				E_QE0000_OK
**	Returns:
**	    E_DB_{OK,WARN,ERROR,FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-apr-89 (paul)
**	    Moved from qeq.c
**	13-jun-89 (paul)
**	    Fix bug 6346. Do not reset local variables after a commit action.
**	29-dec-03 (inkdo01)
**	    DSH is now parameter, "function" replaces "reset".
*/
DB_STATUS
qea_commit(
QEF_AHD		    *act,
QEF_RCB		    *qef_rcb,
QEE_DSH		    *dsh,
i4		    function,
i4		    state )
{
    i4		err;
    i4			operation;
    DB_STATUS		status	= E_DB_OK;
    QEF_CB		*qef_cb = dsh->dsh_qefcb;
    
    for (;;)
    {
		/*
		** If we are processing rules then issue an error - otherwise
		** a user could modify the internal state of the single-query-
		** transaction.  The RCB qef_rule_depth field indicates the level of
		** rule processing.
		*/
		if (qef_rcb->qef_rule_depth > 0)
		{
		    status = E_DB_ERROR;
		    qef_error(E_QE0209_BAD_RULE_STMT, 0L, status, &err,
			      &dsh->dsh_error, 1, sizeof("COMMIT")-1, "COMMIT");
		    dsh->dsh_error.err_code = E_QE0122_ALREADY_REPORTED;
		    break;
		}
		if (qef_rcb->qef_no_xact_stmt)
		{
		    qef_rcb->qef_illegal_xact_stmt = TRUE;
		    status = E_DB_ERROR;
		    dsh->dsh_error.err_code = E_QE0258_ILLEGAL_XACT_STMT;
		    break;
		}
		/* Commit the transaction */
		operation = qef_cb->qef_operation;
		qef_cb->qef_operation = QET_SCOMMIT;
		status = qet_commit(qef_cb);
		qef_cb->qef_operation = operation;
		dsh->dsh_qef_rowcount = -1;
		if (status)
		    break;

		/* Since qet_commit closes all open tables in the txn, we will 
		** mark the control blocks as not open and remove it from the
		** list of open tables in this DSH and all DSHs above us
		** in the stack */
		qeq_unwind_dsh_resources(dsh);
		
		/* If there are more actions to execute, restart the
		** transaction and reopen the tables */
	    
               if ( ((act->ahd_next != (QEF_AHD *)NULL)
                   && (act->ahd_next->ahd_atype != QEA_RETURN)) ||
                    qef_rcb->qef_context_cnt > 1 )
		{
		    qef_rcb->qef_modifier = QEF_SSTRAN;
		    qef_rcb->qef_flag = 0;
		    if (status = qet_begin(qef_cb))
			break;
		    ++qef_cb->qef_open_count; /* the current query is opened */

		    /* reopen and validate the tables */
		    /* Indicate through the init_action parameter that we   */
		    /* only want the tables opened, do not initialize the   */
		    /* first action. */
		    if (status = qeq_validate(qef_rcb, dsh, 
				dsh->dsh_qp_ptr->qp_ahd, (i4)NO_FUNC, 
				(bool)FALSE))
		    {
			if (dsh->dsh_error.err_code == E_QE0015_NO_MORE_ROWS)
			{
			    dsh->dsh_error.err_code = E_QE0000_OK;
			    status = E_DB_OK;
			}
			else if ((dsh->dsh_error.err_code == E_QE0023_INVALID_QUERY) ||
			         (dsh->dsh_error.err_code == E_QE0301_TBL_SIZE_CHANGE)) 
			{
			    /* Previously, the procedure execution was	    */
			    /* aborted at this point. Instead of	    */
			    /* complicating the cntrol flow of the main	    */
			    /* interpreter loop, we simply return the	    */
			    /* error. Normal procedure error handling will  */
			    /* encounter the validation error a second time */
			    /* and perform the proper cleanup operations.   */
			    /* This change was made when QEA_COMMIT was	    */
			    /* made a separate procedure. */
			    /*
			    ** Added status change to prevent further action
			    ** in the procedure which may lead to problems
			    ** in DMF. (sandyh)
			    */
			    status = E_DB_ERROR;
			    break;
			}
		    }
		}

		break;
    }
    return (status);
}		
