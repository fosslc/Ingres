/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>
#include    <psyrereg.h>

/**
**
**  Name: PSYREREG.C - Functions used to REREGISTER a STAR object.
**
**  Description:
**      This file contains the functions necessary to REREGISTER STAR objects.
**
**          psy_reregister - REREGISTER a STAR object.
**
**
**  History:
**      04-apr-89 (andre)    
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
[@history_template@]...
**/

/*{
** Name: psy_reregister	    - REREGISTER a STAR object.
**
**  INTERNAL PSF call format: status = psy_reregister(&psy_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psy_call(PSY_REREGISTER, &psy_cb,
**						&sess_cb);
**
** Description:
**      The psy_reregister function will call QEF to REREGISTER a STAR object.
**	QED_DDL_INFO block has been already prepared, so there is very little to
**	do here.
**
** Inputs:
**      psy_cb
**          .psy_dmucb 			ptr to QED_DDL_INFO
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**		.err_code		    What the error was
**		    E_PS0000_OK			Success
**		    E_PS0001_USER_ERROR		User made a mistake
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**		    E_PS0003_INTERRUPTED	User interrupt
**		    E_PS0005_USER_MUST_ABORT	User must abort xact
**		    E_PS0008_RETRY		Query should be retried.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    STAR object will be dropped and recreated with the SAME object id.
**	    RDF cache entry for this object will be destroyed.
** History:
**      04-apr-89 (andre)    
**          written
**	15-jun-92 (barbara)
**	    All psy functions called from psy_call pass the same parameters.
**	    Add session control block to fit in with this scheme.
**	03-aug-92 (barbara)
**	    Invalidate registered object from RDF cache.
**	07-dec-92 (andre)
**	    address of psy_dmucb (which is overloaded with the address of a
**	    QED_DDL_INFO) will be stored in QEU_CB.qeu_ddl_info instead
**	    of overloading qeu_qso.
**	10-aug-93 (andre)
**	    fixed the cause of a compiler warning
*/
DB_STATUS
psy_reregister(
	PSY_CB          *psy_cb,
	PSS_SESBLK	*sess_cb,
	QEU_CB		*qeu_cb)
{
    char		*ddb_obj_name;
    i4		err_code;
    RDF_CB		rdf_cb;
    RDR_RB		*rdf_rb = &rdf_cb.rdf_rb;
    DB_STATUS		status;

    qeu_cb->qeu_d_cb = (PTR) NULL;

    qeu_cb->qeu_ddl_info = psy_cb->psy_dmucb;

    qeu_cb->qeu_d_op = QED_RLINK;

    status = qef_call(QEU_DBU, ( PTR ) qeu_cb);

    if (DB_FAILURE_MACRO(status))
    {
	switch (qeu_cb->error.err_code)
	{
	    /* object unknown */
	    case E_QE0031_NONEXISTENT_TABLE:		
	    {
		ddb_obj_name =
			((QED_DDL_INFO *) psy_cb->psy_dmucb)->qed_d1_obj_name;
			
		(VOID) psf_error(E_PS0903_TAB_NOTFOUND, 0L,
		    PSF_USERERR, &err_code, &psy_cb->psy_error,1,
		   psf_trmwhite(sizeof(DD_NAME), ddb_obj_name), ddb_obj_name);
		break;
	    }
	    /* interrupt */
	    case E_QE0022_QUERY_ABORTED:
	    {
		(VOID) psf_error(E_PS0003_INTERRUPTED,
		    0L, PSF_USERERR,
		    &err_code, &psy_cb->psy_error,0);
		break;
	    }
	    /* should be retried */
	    case E_QE0023_INVALID_QUERY:
	    {
		psy_cb->psy_error.err_code = E_PS0008_RETRY;
		break;
	    }
	    /* user has some other locks on the object */
	    case E_QE0051_TABLE_ACCESS_CONFLICT:
	    {
		(VOID) psf_error(E_PS0D21_QEF_ERROR, 0L,
		    PSF_USERERR, &err_code, &psy_cb->psy_error,0);
		break;
	    }
	    /* resource related */
	    case E_QE000D_NO_MEMORY_LEFT:
	    case E_QE000E_ACTIVE_COUNT_EXCEEDED:
	    case E_QE000F_OUT_OF_OTHER_RESOURCES:
	    case E_QE001E_NO_MEM:
	    {
		(VOID) psf_error(E_PS0D23_QEF_ERROR,
		    qeu_cb->error.err_code, PSF_USERERR,
		    &err_code, &psy_cb->psy_error,0);
		break;
	    }
	    /* lock timer */
	    case E_QE0035_LOCK_RESOURCE_BUSY:
	    case E_QE0036_LOCK_TIMER_EXPIRED:
	    {
		(VOID) psf_error(4702L,
		    qeu_cb->error.err_code, PSF_USERERR,
		    &err_code, &psy_cb->psy_error,0);
		break;
	    }
	    /* resource quota */
	    case E_QE0052_RESOURCE_QUOTA_EXCEED:
	    case E_QE0067_DB_OPEN_QUOTA_EXCEEDED:
	    case E_QE0068_DB_QUOTA_EXCEEDED:
	    case E_QE006B_SESSION_QUOTA_EXCEEDED:
	    {
		(VOID) psf_error(4707L,
		    qeu_cb->error.err_code, PSF_USERERR,
		    &err_code, &psy_cb->psy_error,0);
		break;
	    }
	    /* log full */
	    case E_QE0024_TRANSACTION_ABORTED:
	    {
		(VOID) psf_error(4706L,
		    qeu_cb->error.err_code, PSF_USERERR,
		    &err_code, &psy_cb->psy_error,0);
		break;
	    }
	    /* deadlock */
	    case E_QE002A_DEADLOCK:
	    {
		(VOID) psf_error(4700L,
		    qeu_cb->error.err_code, PSF_USERERR,
		    &err_code, &psy_cb->psy_error,0);
		break;
	    }
	    /* lock quota */
	    case E_QE0034_LOCK_QUOTA_EXCEEDED:
	    {
		(VOID) psf_error(4705L,
		    qeu_cb->error.err_code, PSF_USERERR,
		    &err_code, &psy_cb->psy_error,0);
		break;
	    }
	    /* inconsistent database */
	    case E_QE0099_DB_INCONSISTENT:
	    {
		(VOID) psf_error(38L,
		    qeu_cb->error.err_code, PSF_USERERR,
		    &err_code, &psy_cb->psy_error,0);
		break;
	    }
	    case E_QE0025_USER_ERROR:
	    {
	      psy_cb->psy_error.err_code = E_PS0001_USER_ERROR;
	      break;
	    }
	    default:
	    {
		(VOID) psf_error(E_PS0D20_QEF_ERROR,
		    qeu_cb->error.err_code,
		    PSF_INTERR, &err_code, &psy_cb->psy_error,0);
	    }
	}

	return (status);
    }

    /* Invalidate registered object from RDF cache */
    {
	QED_DDL_INFO	*ddl_info = (QED_DDL_INFO *)psy_cb->psy_dmucb;

	pst_rdfcb_init(&rdf_cb, sess_cb);
	STRUCT_ASSIGN_MACRO(ddl_info->qed_d7_obj_id, rdf_rb->rdr_tabid);

	status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    (VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
				    &psy_cb->psy_error);
	}
    }

    return    (status);
}
