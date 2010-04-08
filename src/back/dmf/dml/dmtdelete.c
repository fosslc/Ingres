/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <adf.h>
#include    <lk.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dmtcb.h>
#include    <dmccb.h>
#include    <dmxcb.h>
#include    <dmrcb.h>
#include    <lg.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dml.h>
#include    <dm2t.h>
#include    <dm0m.h>
#include    <dm0s.h>
#include    <ddb.h>             /* Buncha stuff needed just for rdf.h */
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>

/**
** Name: DMTDELETE.C - Implements the DMF destroy temporary table operation.
**
** Description:
**      This file contains the functions necessary to destroy a temporary table.
**      This file defines:
**
**      dmt_delete_temp - Destroy a temporary table.
**
** History:
**      26-feb-86 (jennifer)
**          Created for Jupiter.
**	01-mar-90 (sandyh)
**	    Added lock release on successful destroy.(bug 20270)
**	10-oct-91 (jnash) merged 24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.  
**	7-may-1992 (bryanp)
**	    Delete XCCB after deleting temp table.
**	8-jul-1992 (walt)
**	    Prototyping DMF.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	31-jan-1994 (bryanp) B58536
**	    Format LKrelease status if error occurs releasing table lock.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_destroy_temp_tcb() calls.
**	23-oct-1998 (somsa01)
**	    In the case of TCB_SESSION_TEMP, the table lock lives on the
**	    scb_lock_list.  (Bug #94059)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Nov-2000 (jenjo02)
**	    Deleted xcb_tq, xccb_tq, find session temp tables
**	    by searching odcb->xccb list instead of tcb hash queue.
**      09-feb-2007 (stial01)
**          New xccb_blob_temp_flags
**	12-Apr-2008 (kschendel)
**	    Move adf.h sooner.
**	20-Oct-2008 (jonj)
**	    SIR 120874 Modified to use new DB_ERROR based uleFormat 
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	16-Nov-2009 (kschendel) SIR 122890
**	    Destroy-temp doesn't need log ID nor opflag.
**/

/*{
** Name:  dmt_delete_temp - Destroy a temporary table.
**
**  INTERNAL DMF call format:  status = dmt_delete_temp(&dmt_cb);
**
**  EXTERNAL call format:      status = dmf_call(DMT_DELETE_TEMP,&dmt_cb);
**
** Description:
**      This function destroys a temporary table.  It is unknown what
**      the semantics of temporary tables will be, other than it can have
**      no system table entries.  Currently this only support calls from
**      the DBMS not the user, and only creates SEQuential (heap) tables.
**
** Inputs:
**      dmt_cb
**          .type                           Must be set to DMT_TABLE_CB.
**          .length                         Must be at least
**                                          sizeof(DMT_TABLE_CB) bytes.
**          .dmt_db_id                      Database to search.
**          .dmt_trans_id                   Transaction to associate with.
**          .dmt_id                         Internal table id of table to 
**                                          destroy.
**
** Outputs:
**      dmt_cb
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0112_RESOURCE_QUOTA_EXCEEDED
**                                          E_DM009D_BAD_TABLE_DESTROY
**
**      Returns:
**         E_DB_OK                          Function completed normally.
**         E_DB_WARN                        Function completed normally with a 
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_ERROR                       Function completed abnormally 
**                                          with a 
**                                          termination status which is in
**                                          dmt_cb.err_code.
**         E_DB_FATAL                       Function completed with a fatal
**                                          error which must be handled
**                                          immediately.  The fatal status is in
**                                          dmt_cb.err_code.
** History:
**      26-feb-86 (jennifer)
**          Created for Jupiter.
**	01-mar-90 (sandyh)
**	    Added LKrelease on temp table destroys. (bug 20270)
**	7-may-1992 (bryanp)
**	    Delete XCCB after deleting temp table.
**	31-jan-1994 (bryanp) B58536
**	    Format LKrelease status if error occurs releasing table lock.
**	    Added missing &error argument to ule_format call.
**	    Set error to E_DM009D_BAD_TABLE_DESTROY if LKrelease call fails,
**		rather than to E_DM901B, since DM901B requires an argument, and
**		setting error to DM901B was causing ER_BADPARAM in ule_format.
**	23-oct-1998 (somsa01)
**	    In the case of TCB_SESSION_TEMP, the table lock lives on the
**	    scb_lock_list.  (Bug #94059)
**	 3-mar-1999 (hayke02)
**	    We now test for TCB_SESSION_TEMP in relstat2 rather than relstat.
**	10-May-2004 (schka24)
**	    ODCB scb != thread/xcb scb in factota, remove test.
**	10-Jun-2004 (schka24)
**	    Remove XCCB for transaction-lifetime temp table too, on explicit
**	    delete.  No bug observed, but we certainly don't want any
**	    chance of reusing the table ID and dropping the wrong table
**	    later on.
**	    Add wrapper for deleting table and cleaning up RDF.
**	21-Jun-2004 (schka24)
**	    Change destroy utility to also release the table lock.  This
**	    will prevent callers who use dmt-destroy-temp directly from
**	    leaking locks.
**	8-Aug-2005 (schka24)
**	    Ignore session-temp flag in case caller leaves it around from
**	    an open.
**	15-Aug-2006 (jonj)
**	    Session temps may now have indexes; allow those to be explicitly
**	    deleted as well. Push release of table lock into dm2t_destroy_temp_tcb.
**	23-Mar-2010 (kschendel) SIR 123448
**	    XCB's XCCB list now has to be mutex protected.
*/
DB_STATUS
dmt_delete_temp(
DMT_CB   *dmt_cb)
{
    bool	    found;
    DMP_TCB	    *t, *it;
    DMT_CB	    *dmt = dmt_cb;
    DML_ODCB        *odcb;
    DML_XCB	    *xcb;
    DML_XCCB	    *xccb;
    i4	    	    error;
    DB_STATUS	    status;
   
    /* Check the input parameters for correctness. */

    CLRDBERR(&dmt->error);

    status = E_DB_ERROR;
    if ((dmt->dmt_flags_mask & ~(DMT_SESSION_TEMP)) == 0)
    {
	xcb = (DML_XCB *)dmt->dmt_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) == E_DB_OK)
	{
	    odcb = (DML_ODCB *)dmt->dmt_db_id;
	    if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) == E_DB_OK)
	    {
#ifdef xDEBUG
    {CS_SID sid; CSget_sid(&sid);
    TRdisplay("%@ Sess %x: temp delete id (%d,%d), name %~t\n",
    sid, dmt->dmt_id.db_tab_base, dmt->dmt_id.db_tab_index,
    sizeof(DB_TAB_NAME),&dmt->dmt_table);
    }
#endif
		/*
		** We need to find the xccb for the table destroy request.
		*/

		/*
		** Session-wide (TCB_SESSION_TEMP) base tables can be
		** found by searching the odcb->xccb list.
		** Session-wide index tables can be found by searching
		** that base table's TCB index list.
		**
		** Only Session Temps can have indexes.
		*/
		found = FALSE;
		dm0s_mlock(&odcb->odcb_cq_mutex);
		for ( xccb = odcb->odcb_cq_next;
		      xccb != (DML_XCCB*) &odcb->odcb_cq_next;
		      xccb = xccb->xccb_q_next )
		{
		    if ( xccb->xccb_operation == XCCB_T_DESTROY &&
			dmt->dmt_id.db_tab_base == 
			    xccb->xccb_t_table_id.db_tab_base )
		    {
			/* Found the base table */
			if ( dmt->dmt_id.db_tab_index == 0 )
			    found = TRUE;
			else if ( t = xccb->xccb_t_tcb )
			{
			    /* ...but we're looking for an index */
			    for ( it = t->tcb_iq_next;
				  it != (DMP_TCB*)&t->tcb_iq_next;
				  it = it->tcb_q_next )
			    {
				if ( dmt->dmt_id.db_tab_index == 
					it->tcb_rel.reltid.db_tab_index )
				{
				    found = TRUE;
				    break;
				}
			    }
			}
		    }
		    if ( found )
			break;
		}
		dm0s_munlock(&odcb->odcb_cq_mutex);

		if (!found)
		{
		    /* Try the XCB list of XCCB's (session temps won't be here) */
		    dm0s_mlock(&xcb->xcb_cq_mutex);
		    for ( xccb = xcb->xcb_cq_next;
			  xccb != (DML_XCCB*) &xcb->xcb_cq_next;
			  xccb = xccb->xccb_q_next )
		    {
			if ( xccb->xccb_operation == XCCB_T_DESTROY &&
			    dmt->dmt_id.db_tab_base == 
				xccb->xccb_t_table_id.db_tab_base &&
			    dmt->dmt_id.db_tab_index == 
				xccb->xccb_t_table_id.db_tab_index )
			{
			    found = TRUE;
			    break;
			}
		    }
		    dm0s_munlock(&xcb->xcb_cq_mutex);
		}

		if (!found)
		{
		    TRdisplay("%@ No xccb for destroy of (%d,%d)\n",
			dmt->dmt_id.db_tab_base,dmt->dmt_id.db_tab_index);
		    SETDBERR(&dmt->error, 0, E_DM009D_BAD_TABLE_DESTROY);
		    /* status is error */
		}
		else if ( xccb->xccb_is_session && dmt->dmt_id.db_tab_index )
		{
		    /* 
		    ** Destroy Session Temp Index directly, leaving the
		    ** XCCB with its base table and maybe other indexes.
		    **
		    ** Note that dm2t_destroy_temp_tcb() will release the
		    ** table lock (for just this index).
		    */
		    status = dm2t_destroy_temp_tcb(xcb->xcb_scb_ptr->scb_lock_list,
				xccb->xccb_t_dcb, 
				&dmt->dmt_id, &dmt->error);
		}
		else
		{
		    /*
		    ** Destroy Base table and all Indexes (if Session Temp)
		    */
		    status = dmt_destroy_temp(xcb->xcb_scb_ptr,xccb, &dmt->error);
		    if (status == E_DB_OK)
		    {
			/* Now that it worked, remove XCCB from list.
			** Have to be careful not to do it until now;
			** QEF likes to attempt temp table delete while
			** other handles to the same table are still open.
			*/
			if (xccb->xccb_is_session)
			    dm0s_mlock(&odcb->odcb_cq_mutex);
			xccb->xccb_q_prev->xccb_q_next =
					    xccb->xccb_q_next;
			xccb->xccb_q_next->xccb_q_prev =
					    xccb->xccb_q_prev;
			if (xccb->xccb_is_session)
			    dm0s_munlock(&odcb->odcb_cq_mutex);
			dm0m_deallocate((DM_OBJECT **)&xccb);
		    }
		}

	    }
	    else
		SETDBERR(&dmt->error, 0, E_DM0010_BAD_DB_ID);
	}
	else
	    SETDBERR(&dmt->error, 0, E_DM003B_BAD_TRAN_ID);
    }
    else
	SETDBERR(&dmt->error, 0, E_DM001A_BAD_FLAG);

    if (dmt->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmt->error, 0 , NULL, ULE_LOG, NULL, 
	    (char * )NULL, 0L, (i4 *)NULL, &error, 0);
	SETDBERR(&dmt->error, 0, E_DM009D_BAD_TABLE_DESTROY);
    }
# ifdef xDEV_TST
TRdisplay("\nDESTROYED TEMP TABLE id (%d,%d) \n",
dmt->dmt_id.db_tab_base,
dmt->dmt_id.db_tab_index);
# endif
    return (status);
}

/*
** Name: dmt_destroy_temp - Actual temp table delete wrapper
**
** Description:
**	This routine is a utility wrapper for DMF that does three things:
**	it calls the physical level temp-table deleter in dm2t,
**	(which releases the table lock (that reserves the temp table ID)), and
**	it invalidates the temp table out of RDF.
**
**	What the heck is a call to RDF doing here, you ask?  And
**	well you might.  As it happens, there is no centralized
**	delete focus point for dropping all of the various kinds of
**	temp table that the DBMS or user can create.  (Well, there
**	is now, you're looking at it.)  Both user (session) and
**	DBMS generated temp tables can end up being cached in RDF.
**	Session temps get cached by user reference, and DBMS temps
**	can get cached in more subtle ways (if they are used as a
**	set-of parameter to a rule-fired DB procedure, for instance.)
**	Leaving a temp table in RDF once it's deleted is a Very Bad
**	Idea, as RDF is quite happy to return completely wrong and
**	bogus information about the nonexistent table.
**
**	While it might be cleaner in some sense to put the RDF
**	invalidation into a higher level (QEF, say), it's just not
**	practical.  Temp tables come and go, and in some cases
**	(transaction abort, session end, etc) DMF is the facility
**	that dictates the temp table lifetime.  So the invalidate
**	call pretty much has to be in DMF where we actually delete
**	things.
**
**	One note:  the XCB pointer in the XCCB may not be valid if
**	we're doing a session temp.  There's no law that says we have
**	to be in a transaction.
**
**	The caller feeds us the table-destroy XCCB for the temp table.
**	The caller is responsible for deleting the XCCB and removing it
**	from the appropriate XCCB queue.
**
** Inputs:
**	scb			Current thread's SCB
**	    .scb_lock_list	SCB lock list ID for session temps
**	xccb			Pointer to temp table's XCCB
**	    .xccb_t_dcb		DCB for database
**	    .xccb_t_table_id	Table ID to drop
**	    .xccb_xcb_ptr	XCB, * may not be valid *
**	    .xccb_is_session	TRUE for session temps, use SCB lock-list
**          .xccb_blob_temp_flags if BLOB_HOLDING_TEMP, skip RDF stuff
**
**	error			An output
**
** Outputs:
**	Returns E_DB_xxx status
**	error			Where dm2t error details are returned
**
** History:
**	11-Jun-2004 (schka24)
**	    Write to consolidate 3-4 places that do this.
**	21-Jun-2004 (schka24)
**	    RDF poop not needed for blob temps, now that we know.
**	    Do table unlock here rather than driver, for those places who
**	    call this destroyer directly.
**	22-Jul-2004 (jenjo02)
**	    When called from dmx_abort, log/lock contexts may be
**	    gone (and all locks already released).
**	17-Nov-2005 (schka24)
**	    Minor cleanup to not even load XCB ptr that might be invalid.
**	6-Jul-2006 (kschendel)
**	    Pass the db id to RDF in the right place!
**	15-Aug-2006 (jonj)
**	    Pushed release of table lock into dm2t_destroy_temp_tcb().
**	    This function destroys the Base temp table and all
**	    attached indexes.
*/

DB_STATUS
dmt_destroy_temp(DML_SCB *scb, DML_XCCB *xccb, DB_ERROR *error)
{

    DB_STATUS status;		/* Usual status thing */
    i4 	      lockid;		/* Lock list ID */
    RDF_CB    rdfcb;		/* RDF call request block */

    if ((xccb->xccb_blob_temp_flags & BLOB_HOLDING_TEMP) == 0)
    {

	/* Start by ditching the temp table out of RDF.  It's sort of
	** unfortunate, but this has to be done for any kind of temp table
	** (except for blob holding tables -- they can't get into RDF,
	** at least there's no way I know of, since they are handled
	** entirely within DMF.  Optimize out the call for blob temps.)
	** Zero in rdr_fcb says don't send invalidate dbevents to other
	** servers.  Zero in the rdfcb rdf_info_blk says that we don't
	** have anything fixed.
	*/

	MEfill(sizeof(RDF_CB), 0, &rdfcb);
	STRUCT_ASSIGN_MACRO(xccb->xccb_t_table_id, rdfcb.rdf_rb.rdr_tabid);
	rdfcb.rdf_rb.rdr_session_id = scb->scb_sid;
	rdfcb.rdf_rb.rdr_unique_dbid = xccb->xccb_t_dcb->dcb_id;

	/* Note:  assumes only one ODCB.  Actually, doesn't matter, all we
	** need is something non-null to tell RDF how to invalidate.
	*/
	rdfcb.rdf_rb.rdr_db_id = (PTR) scb->scb_oq_next;

	/* Ignore error on this RDF invalidate, not much can be done */
	(void) rdf_call(RDF_INVALIDATE, &rdfcb);
    }

    /* Now actually destroy the table */

    lockid = scb->scb_lock_list;
    if (! xccb->xccb_is_session)
	lockid = xccb->xccb_xcb_ptr->xcb_lk_id;	/* Only ref XCB if we know it's there */

    status = dm2t_destroy_temp_tcb(lockid,
		xccb->xccb_t_dcb, &xccb->xccb_t_table_id,
		error);

    return (status);
} /* dmt_destroy_temp */
