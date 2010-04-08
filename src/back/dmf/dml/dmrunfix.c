/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <sl.h>
#include    <sr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <pc.h>
#include    <tm.h>
#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmrcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <lg.h>
#include    <lk.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmse.h>
#include    <dm1b.h>
#include    <dm2umct.h>
#include    <dm2r.h>
#include    <dm2rlct.h>
#include    <dma.h>
#include    <gwf.h>

/**
** Name: DMRUNFIX.C - Implements the DMF unfix pages operation.
**
** Description:
**      This file contains the functions necessary to
**	unfix any pages still fixed by the DMR_CB's DMP_RCB.
**
**      This file defines:
**
**      dmr_unfix - Unfix pages.
**
** History:
**	09-Apr-2004 (jenjo02)
**	    Invented for partitioning.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2r_? functions converted to DB_ERROR *
**/

/*{
** Name:  dmr_unfix - Unfix pages.
**
**  INTERNAL DMF call format:      status = dmr_unfix(&dmr_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMR_UNFIX,&dmr_cb);
**
** Description:
**	This function unfixes any still-fixed pages in the
**	caller's DMR_CB->DMP_RCB.
**
**	It is typically used, for example, when crossing a
**	partition boundary when doing a table scan.
**
**	It lets the caller leave the table open but without any
**	of its pages fixed in cache.
**
**	Unless the partition's table is either closed or
**	unfixed, the pages remain fixed in cache and when
**	"lots" of partitions are involved in a query, the  
**	cache can quickly become saturated.
**
** Inputs:
**      dmr_cb
**          .type                           Must be set to DMR_RECORD_CB.
**          .length                         Must be at least
**                                          sizeof(DMR_RECORD_CB) bytes.
**          .dmr_access_id                  Record access identifer returned
**                                          from DMT_OPEN that identifies a
**                                          table.
**
** Outputs:
**      dmr_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM002A_BAD_PARAMETER
**	    				    E_DM019A_ERROR_UNFIXING_PAGES
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmr_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally 
**                                          with a 
**                                          termination status which is in
**                                          dmr_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmr_cb.err_code.
** History:
**	09-Apr-2004 (jenjo02)
**	    Invented for partitioning.
**      04-jan-2005 (stial01)
**          Set RCB_CSRR_LOCK so that locks can get released (b113231)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: No CSRR_LOCK when crow_locking().
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Set rcb_dmr_opcode here; dmpe bypasses dmf_call,
**	    which used to set it.
*/
DB_STATUS
dmr_unfix(
DMR_CB  *dmr_cb)
{
    DMR_CB	*dmr = dmr_cb;
    DMP_RCB	*rcb;
    DB_STATUS	status;
    i4		error;

    CLRDBERR(&dmr->error);

    if ( (rcb = (DMP_RCB *)dmr->dmr_access_id) &&
	 (dm0m_check((DM_OBJECT *)rcb, (i4)RCB_CB) == E_DB_OK) )
    {
	rcb->rcb_dmr_opcode = DMR_UNFIX;

	/* Unfix any pages, if any */
	if ( !crow_locking(rcb) &&
	       (rcb->rcb_iso_level == RCB_CURSOR_STABILITY ||
		rcb->rcb_iso_level == RCB_REPEATABLE_READ) )
	{
	    rcb->rcb_state |= RCB_CSRR_LOCK;
	}

	status = dm2r_unfix_pages(rcb, &dmr->error);
    }
    else
    {
	uleFormat(NULL, E_DM00E0_BAD_CB_PTR, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)0, (i4)0, (i4 *)0, &error, 1,
	    sizeof("record")-1, "record");
	SETDBERR(&dmr->error, 0, E_DM002A_BAD_PARAMETER);
	status = E_DB_ERROR;
    }

    if ( status )
    {
	/* This is pretty bad; abort the transaction */
	rcb->rcb_xcb_ptr->xcb_state |= XCB_TRANABORT;

	if (dmr->error.err_code > E_DM_INTERNAL)
	{
	    uleFormat( &dmr->error, 0, NULL, ULE_LOG , NULL,
		(char * )NULL, 0L, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM904F_ERROR_UNFIXING_PAGES, NULL, ULE_LOG, NULL,
		(char * )NULL, 0L, (i4 *)NULL, &error, 3,
		sizeof(DB_DB_NAME), &rcb->rcb_tcb_ptr->tcb_dcb_ptr->dcb_name,
		sizeof(DB_OWN_NAME), &rcb->rcb_tcb_ptr->tcb_rel.relowner,
		sizeof(DB_TAB_NAME), &rcb->rcb_tcb_ptr->tcb_rel.relid
		);
	    SETDBERR(&dmr->error, 0, E_DM019A_ERROR_UNFIXING_PAGES);
	}
    }

    return (status);
}
