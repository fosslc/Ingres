/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <qu.h>
#include    <me.h>
#include    <cs.h>
#include    <mu.h>
#include    <tr.h>
#include    <gca.h>
#include    <gc.h>
#include    <gcaint.h>

/**
**
**  Name: gcacmpt.c
**
**  Description:
**      Contains the following functions:
**
**          gca_complete - GCA service completion management
**
**  History:
**      28-apr-87 (berrow)
**          Initial module creation
**      23-Sept-87 (jbowers)
**          Added code to support "no_async_exit" for asynchronous requests.
**      21-Dec-87 (jbowers)
**          Add return of OS error status to each parm list type
**      23-Feb-88 (jbowers)
**          Added reset of indicator in GCA_STATIC on successful GCA_LISTEN
**          completion.
**      30-Aug-88 (jbowers)
**          Added setting of gca_os_status to 0 when no error.
**      17-Nov-88 (jbowers)
**          Added return of user_id and password information to completion of
**	    GCA_LISTEN.
**      01-Dec-88 (jbowers)
**          At completion of GCA_LISTEN and GCA_REQUEST set gca_flags in parm 
**	    lists from gca_flags sent by partner in peer_info.
**      10-Feb-89 (jbowers)
**          Added completion for GCA_DCONN function.
**      06-Mar-89 (jbowers)
**          Modified to deal with a race condition while purging: 
**	    test chop_mks_owed flag when completing a GCA_SEND operation, and
**	    call GCpurge to send chop marks if it is set.
**      27-Mar-89 (jbowers)
**          Modified mechanism for transforming CL status codes to GCA mainline
**	    status codes.
**      03-Apr-89 (jbowers)
**          Corrected bug introduced by 06-Mar-89 fix: check to see that the ACB
**	    is not null before looking at "chop_mks_owed".
**      10-Apr-89 (jbowers)
**          Added completion for GCA_ATTRIBS function.
**      27-Apr-89 (jbowers)
**          Added support for driving alternate completion exits on completion
**	    of GCA_REQUEST, GCA_SEND and GCA_RECEIVE service requests.
**	23-Jul-89 (jorge)
**	    Fixed the GCA_REQUEST completion to return the GCA_PEER_PROTOCOL
**	29-Jul-89 (seiwald)
**	    Allow for alternate completion exits, as indicated by the user 
**	    at the IIGCa_call with the GCA_ALT_EXIT modifier and set in the 
**	    svc_parms as flags.alt_exit.  In the future, all completion
**	    routines will be specified at IIGCa_call time; for now, they
**	    are "alterate" completion exits.
**	31-Jul-89 (seiwald)
**	    Take min of local and partner protocol on GCA_LISTEN, GCA_REQUEST.
**	1-Aug-89 (seiwald)
**	    New completion for GCA_RQRESP.  Reverse change above: return 
**	    only partner protocol on GCA_LISTEN, GCA_REQUEST.
**	09-OCT-89 (jorge)
**	    Merged the B1 stuff back in (sec_label)
**	12-Oct-89 (seiwald)
**	   Ifdef'ed the B1 stuff until they get predator working.
**	24-Oct-89 (seiwald)
**	   Changed mapping of CL messages to GCF ones: the previous
**	   check for E_GC_MASK didn't work.  I apologize in advance 
**	   for the bit twiddling.
**	25-Oct-89 (seiwald)
**	    Shortened gcainternal.h to gcaint.h.
**	10-Nov-89 (seiwald)
**	    DCONN is dead.
**	    Changed ifndef GCF63 to ifdef xORANGE.
**	15-Nov-89 (seiwald)
**	    New GCA_DEBUG_MACRO tracing.
**	16-Nov-89 (seiwald)
**	    Driving of user completion exits controlled by API_LEVEL_2:
**	    at and after this level, all async requests drive completion
**	    exits.  Previously, those completions resulting from a 
**	    GCA_DISASSOC stopped here in gca_complete.
**	12-Dec-89 (seiwald)
**	    Removed antiquated gca_listen_assoc.  
**	18-Dec-89 (seiwald)
**	    Moved completion code for GCA_REQUEST and GCA_LISTEN out into
**	    the functions for those requests.  This will be the trend.
**	30-Dec-89 (seiwald)
**	    Check the nsync_indicator for sync operations, since
**	    sync_indicator is (temporarily) always 0 to make the CL run
**	    asynchronously.
**	31-Dec-89 (seiwald)
**	    Moved completion code for GCA_SEND and GCA_RECEIVE out into
**	    the functions for those requests.
**	28-Jan-90 (seiwald)
**	    Tracing changed to line up with IIGCa_call's.
**	16-Feb-90 (seiwald)
**	    Renamed nsync_indicator to sync_indicator, and removed old
**	    references to sync_indicator.  The CL must now be async.
**	26-Feb-90 (seiwald)
**	    Consolidated error numbers: svc_parms->sys_err now points to 
**	    parmlist gca_os_status, and svc_parms->statusp points to 
**	    parmlist gca_status.
**	06-Mar-90 (seiwald)
**	    Sorted out running sync vs. driving completion exits.
**	    The SVC_PARMS flag "run_sync" indicates that the operation
**	    must be driven to completion, and "drive_exit" indicates
**	    that gca_complete should drive the completion routine.
**	29-Jan-91 (seiwald)
**	    Shuffled GCA tracing levels.
**	11-Mar-91 (seiwald)
**	    Included all necessary CL headers as per PC group.
**	09-Sep-92 (brucek)
**	    Support for alternate completion exit for GCA_FASTSELECT.
**	07-Jan-93 (edg)
**	    Removed FUNC_EXTERN's (now in gcaint.h) and #include gcagref.h.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	13-Oct-93 (seiwald)
**	    Introduced regular parameter placement into GCA_PARMLIST
**	    so that GCA code doesn't have to hunt for status, syserr,
**	    etc.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems.  In GCA, change from 
**	    using handed in semaphore routines to MU semaphores; these
**	    can be initialized, named, and removed.  The others can't
**	    without expanding the GCA interface, which is painful and
**	    practically impossible.
**	11-Oct-95 (gordy)
**	    Added prototypes.  Allow for uninitialized IIGCa_static.
**	 3-Sep-96 (gordy)
**	    Restructured global GCA data.  Added GCA control blocks.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	28-May-97 (thoda04)
**	    Fix to GCA_DEBUG_MACRO call to cast arguments correctly for WIN16.
**	18-Dec-97 (gordy)
**	    Multi-threaded sync requests now supported.  Sync counter
**	    moved to ACB.  ACB now deleted here when flagged.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-apr-2001 (mofja02)
**	    Made a change so that only synchronous GCA requests 
**	    should reference the syncing counter in the GCA
**	    Association Control Block.
**	    Gordy directed me to make this change.  According to 
**	    Gordy this change just returns things to the way they 
**	    used to be.  This change was required to allow Gateways
**	    to operate in an Ingres 2.5 environment.
**/

/*{
** Name: gca_complete	- GCA service completion management
**
** Description:
**	The task of this routine is to perform any completion activities
**	private to GCA required before driving the users completion exit
**	or simply returning control to the user.
**
** Inputs:
**
**	svc_parms			Service parameter list for the
**					completing service.
**
** Outputs:
**
**	parameter_list			In possesion of service requestor.
**					This contains:
**
**      .gca_status                     Result of service request completion.
**                                      The following values may be returned:
**	    E_GCFFFF_IN_PROCESS         Request is not complete
**	    E_GC0000_OK                 Normal completion
**	    E_GC0001_ASSOC_FAIL         Association failure
**	    E_GC0005_INV_ASSOC_ID       Invalid association identifier
**	    E_GC0008_INV_MSG_TYPE       Invalid message type identifier
**	    E_GC0009_INV_BUF_ADDR       Invalid buffer address
**	    E_GC0010_BUF_TOO_SMALL      Buffer is too small: no room for data
**	    E_GC0011_INV_CONTENTS       Invalid buffer contents
**
** Returns:
**	VOID
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      28-apr-87 (berrow)
**          Initial function creation.
**	11-Oct-95 (gordy)
**	    Added prototypes.  Allow for uninitialized IIGCa_static.
**	 3-Sep-96 (gordy)
**	    Restructured global GCA data.  Added GCA control blocks.
**	10-Apr-97 (gordy)
**	    GCA and GC service parameters separated.
**	18-Dec-97 (gordy)
**	    Multi-threaded sync requests now supported.  Sync counter
**	    moved to ACB.  ACB now deleted here when flagged.
*/

VOID
gca_complete( GCA_SVC_PARMS *svc_parms )
{
    GCA_CB		*gca_cb = (GCA_CB *)svc_parms->gca_cb;
    GCA_ALL_PARMS	*parmlist = (GCA_ALL_PARMS *)svc_parms->parameter_list;
    i4			service_code = svc_parms->service_code;
    i4			assoc_id = svc_parms->acb 
				   ? svc_parms->acb->assoc_id : -1;
    /* 
    ** If parmlist status indicates operation successful, propagate
    ** status (success or failure) from svc_parms.
    */
    if ( parmlist->gca_status == E_GCFFFF_IN_PROCESS )
	parmlist->gca_status = svc_parms->gc_parms.status;

    /* 
    ** Map GC CL errors into GCA errors.  Ugly. 
    ** This relies on gca.h and gc.h remaining in sync.
    */
    if ( ( parmlist->gca_status & ~0xFF ) == ( E_CL_MASK | E_GC_MASK ) )
	parmlist->gca_status ^= ( E_CL_MASK | E_GC_MASK ) ^ E_GCF_MASK;

    GCA_DEBUG_MACRO(2)( "%04d   GCA_COMPLETE %d status=%x\n",
			assoc_id, service_code, parmlist->gca_status );

    /*
    ** Free svc_parms in preparation for calling completion routine.
    */
    svc_parms->in_use = FALSE;

    if ( svc_parms->acb )
    {
	if ( svc_parms->gc_parms.flags.run_sync && svc_parms->acb->syncing )  
	{
	    /*
	    ** Decrement the synchronous request count.
	    ** When this drops to 0 (all requests done)
	    ** drop our reference to the counter.
	    */
	    if ( ! --(*svc_parms->acb->syncing) )
		svc_parms->acb->syncing = NULL;
	}

	/*
	** Delete ACB if flagged for deletion.
	*/
	if ( svc_parms->acb->flags.delete_acb )
	    gca_del_acb( svc_parms->acb->assoc_id );
    }

    /* 
    ** Drive completion exit:  only async requests drive completion
    ** exit.  Before API_LEVEL_2, "orphaned" requests on released 
    ** associations didn't drive user completion exits.  This is 
    ** because GCA_DISASSOC was synchronous and considered the last 
    ** operation necessary on an associations.  At and after 
    ** API_LEVEL_2, GCA_DISASSOC became async, and all requests
    ** complete, driving the completion exit with E_GC0023_ASSOC_RLSED.
    */
    if ( parmlist->gca_completion  &&
	 ( (gca_cb->gca_api_version >= GCA_API_LEVEL_2) ||
	   parmlist->gca_status != E_GC0023_ASSOC_RLSED ) )
    {
	GCA_DEBUG_MACRO(2)( "%04d   GCA_COMPLETE %d completing\n",
			    assoc_id, service_code );
    
	(*parmlist->gca_completion)( parmlist->gca_closure );
    }

    return;
}
