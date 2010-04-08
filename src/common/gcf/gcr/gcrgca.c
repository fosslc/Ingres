/*
** Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <gc.h>
#include    <gcccl.h>
#include    <mu.h>
#include    <qu.h>
#include    <st.h>
#include    <tm.h>
#include    <me.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcaint.h>
#include    <gcc.h>
#include    <gccgref.h>

/*
** Make sure the GCA CL function call replacement
** that is done for callers of GCR is not done for
** us (so that we can call the CL if needed).
*/
#define	_GCR_INTERNAL_

GLOBALDEF   i4  gcr_assoc_id = 0;         /* GCR assoc_id */

#include    <gcr.h>


/*
** Name: gcrgca.c
**
** Description:
**	GCA router main entry points:
**
**	    gcr_gca	Route GCA requests to CL GC or Embedded Comm Server
**	    gcr_gcc	Route GCC requests to GCA or as GCA callbacks.
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	22-Dec-95 (gordy)
**	    Allow for no CCB when releasing an association.  An 
**	    association which failed during initialization may 
**	    not have a CCB.
**      13-mar-96 (chech02)
**          Added <me.h> for win31 port.
**	14-Nov-96 (gordy)
**	    GCsync() now takes a timeout value.
**	18-Dec-96 (gordy)
**	    Fixed GLOBALREF (now GLOBALDEF).
**	18-Feb-97 (gordy)
**	    Converted boolean remote to flag.
**	11-Jun-97 (gordy)
**	    Removed peer info function reference, they are no more.
**	 2-Jul-99 (rajus01)
**	    Added gcr_assoc_id, gcr_fing_rcb(). Removed references to
**	    GCsendpeer(), GCrecvpeer(), acb. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Forward references.
*/
static	STATUS	gcr_init( VOID );
static	STATUS	gcr_term( VOID );

static	GCR_CB	*gcr_alloc_cb( VOID );
static	VOID	gcr_free_cb( GCR_CB *rcb );
static	VOID	gcr_release_cb( GCR_CB *rcb );
static	bool	gcr_check_cb( VOID );

static	STATUS	gcr_call_gc( i4  request, SVC_PARMS *svc_parms );
static	STATUS	gcr_call_gcc( i4  request, SVC_PARMS *svc_parms );
static	STATUS	gcr_call_gca( i4  request, GCA_PARMLIST *parmlist, 
			      i4  indicators, PTR async_id,
			      i4 time_out, STATUS *status );

static	GCR_CB	*gcr_find_rcb( i4 assoc_id );

/*
** Embedded Comm Server router global data area.
*/
GLOBALDEF GCR_STATIC	*IIGCr_static = NULL;


/*
** Name: gcr_gca
**
** Description:
**	GCA request router.  GCA generally makes calls to the CL
**	GC sub-system.  When the embedded Comm Server is active
**	and a remote request is made, these GC calls are re-
**	directed to the GCC embedded interface.
**
** Input:
**	request		GCA router request type.
**	svc_parms	GC service parameters.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	18-Feb-97 (gordy)
**	    Converted boolean remote to flag.
*/

STATUS
gcr_gca( i4  request, SVC_PARMS *svc_parms )
{
    STATUS 	status;
    bool 	gcr_gcc_flag;

#ifdef GCF_EMBEDDED_GCC
    gcr_gcc_flag = svc_parms ? svc_parms->flags.remote : FALSE; 
    if( gcr_gcc_flag && IIGCr_static && IIGCr_static->gcr_embed_flag ) 
	status = gcr_call_gcc( request, svc_parms );
    else
#endif
	status = gcr_call_gc( request, svc_parms );

    return( status );
}



/*
** Name: gcr_gcc
**
** Description:
**	GCC request router.  GCC generally makes calls to GCA.
**	When the embedded Comm Server is active, these calls
**	are usually the result of a prior call into the Comm
**	Server from GCA and must be converted into GCA callbacks.
**
** Input:
**	request		GCA router request type.
**	parmlist	GCA parameter list.
**	indicators	GCA flags.
**	async_id	Callback closure.
**	time_out	Time-out value.
**
** Output:
**	status		Call status.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

STATUS
gcr_gcc
( 
    i4			service_code, 
    GCA_PARMLIST	*parmlist, 
    i4			indicators,
    PTR			async_id,
    i4		time_out,
    STATUS		*status
)
{
    STATUS	r_stat;
    i4		request;

    *status = OK;

#ifdef GCF_EMBEDDED_GCC
    if ( IIGCc_global  &&  ! (IIGCc_global->gcc_flags & GCC_STANDALONE) )
    {
	/*
	** Convert GCA request to router request.
	*/
	switch( service_code )
	{
	    case GCA_INITIATE :		request = GCR_INITIATE;		break;
	    case GCA_TERMINATE :	request = GCR_TERMINATE;	break;
	    case GCA_REQUEST :		request = GCR_REQUEST;		break;
	    case GCA_LISTEN :		request = GCR_LISTEN;		break;
	    case GCA_SEND :		request = GCR_SEND;		break;
	    case GCA_RECEIVE :		request = GCR_RECEIVE;		break;
	    case GCA_REGISTER :		request = GCR_REGISTER;		break;
	    case GCA_DISASSOC:		request = GCR_DISCONN;		break;
	    case GCA_RQRESP :		request = GCR_RQRESP;		break;

	    /*
	    ** Unexpected GCA service codes
	    */
	    default :
		GCA_DEBUG_MACRO(5)( "gcr_gcc: invalid request %d\n", 
				    service_code );
		*status = E_GC0003_INV_SVC_CODE;
		return( E_DB_ERROR );
	}

	r_stat = gcr_call_gca( request, parmlist,
			       indicators, async_id, time_out, status );
    }
    else
#endif
    {
	GCA_DEBUG_MACRO(4)("%04d     GCR routing GCC request to GCA\n",(i4)-1);
	r_stat = IIGCa_call( service_code, parmlist,
			     indicators, async_id, time_out, status );
    }

    return( r_stat );
}


/*
** Name: gcr_call_gc
**
** Description:
**	Converts a router request from GCA into a CL GC call.
**
** Input:
**	request		GCA router request type.
**	svc_parms	GC service parameters.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	11-Jun-97 (gordy)
**	    Removed peer info function reference, they are no more.
*/

static STATUS
gcr_call_gc( i4  request, SVC_PARMS *svc_parms )
{
    STATUS status = OK;

    GCA_DEBUG_MACRO(4)("%04d     GCR routing GCA request to GC CL\n", (i4)-1);

    switch( request )
    {
	case GCR_REGISTER:	GCregister( svc_parms );	break;
	case GCR_LISTEN:	GClisten( svc_parms );		break;
	case GCR_REQUEST:	GCrequest( svc_parms );		break;
	case GCR_SEND:		GCsend( svc_parms );		break;
	case GCR_RECEIVE:	GCreceive( svc_parms );		break;
	case GCR_PURGE:		GCpurge( svc_parms );		break;
	case GCR_SAVE:		GCsave( svc_parms );		break;
	case GCR_RESTORE:	GCrestore( svc_parms );		break;
	case GCR_DISCONN:	GCdisconn( svc_parms );		break;
	case GCR_RELEASE:	GCrelease( svc_parms );		break;

	case GCR_INITIATE:	
#ifdef GCF_EMBEDDED_GCC

	    /*
	    ** Initialize the embedded Comm Server router.  If this
	    ** fails we have no way to return the error.  The failure
	    ** will have to be reported on the first request.
	    */
	    gcr_init();

#endif /* GCF_EMBEDDED_GCC */

	    GCinitiate( gca_alloc, gca_free );	
	    break;

	case GCR_TERMINATE:
#ifdef GCF_EMBEDDED_GCC

	    /*
	    ** Since there is no ACB during termination,
	    ** we will come here even if the embedded
	    ** Comm Server is initialized.
	    */
	    status = gcr_term();

	    if ( svc_parms )  svc_parms->status = status;
	    if ( status != OK )  return( status );
#endif

	    GCterminate( svc_parms );
	    break;

	/*
	** The following are GCA requests which do
	** not have GC CL equivilents.
	*/
	case GCR_RQRESP:

	/*
	** Invalid GCR request codes.
	*/
	default :
	    GCA_DEBUG_MACRO(5)("gcr_call_gc: invalid request %d\n", request);
	    status = E_GC0003_INV_SVC_CODE;

	    if ( svc_parms )  
	    {
		svc_parms->status = status;

		if ( svc_parms->gca_cl_completion )
		{
		    GCA_DEBUG_MACRO(4)( "%04d     GCR making GCA callback\n", 
					(i4)-1 );
		    (*svc_parms->gca_cl_completion)( svc_parms->closure );
		}
	    }
	    break;
    }

    return( status );
}



#ifdef GCF_EMBEDDED_GCC
/*
** Name: gcr_call_gcc
**
** Description:
**	Converts a router request from GCA into an embedded
**	Comm Server call.
**
** Input:
**	request		GCA router request type.
**	svc_parms	GC service parameters.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
**	 9-Jan-96 (gordy)
**	    Extract GCR_CB release code to gcr_release_cb() and gcr_free_cb()
**	    to handle cases when the CCB has not been freed by the GCC TL.
*/

static STATUS
gcr_call_gcc( i4  request, SVC_PARMS *svc_parms )
{
    GCR_CB	*rcb;
    GCC_CCB	*ccb;
    STATUS	status = OK;
    bool	callback = FALSE;

    GCA_DEBUG_MACRO(4)("%04d     GCR routing GCA request to GCC\n", (i4)-1);

    switch( request )
    {
	case GCR_REQUEST:
	    /*
	    ** Make sure we initialized successfully.
	    */


	    if ( ! IIGCr_static )
	    {
		status = E_GC0013_ASSFL_MEM;
		callback = TRUE;
		break;
	    }

	    /*
	    ** See if there are any waiting control blocks
	    ** which have completed processing and may be freed.
	    */
	    gcr_check_cb();

	    /*
	    ** Allocate a router control block to service
	    ** this association.  Since we are not calling
	    ** CL GC for this association, we can use the
	    ** gc_cb pointer in the ACB for our control block.
	    ** This will be released in GCR_RELEASE below.
	    */
	    rcb = gcr_alloc_cb();

	    if ( ! rcb )
	    {
		status = E_GC0013_ASSFL_MEM;
		callback = TRUE;
		break;
	    }

	    svc_parms->gc_cb = (PTR)rcb;

	    /*
	    ** Now call the embedded Comm Server interface
	    ** State Machine to complete the processing of
	    ** this request.  The state machine handles
	    ** the callback.
	    */
	    rcb->svc_parms = svc_parms;
	    status = gcr_gcc_sm( rcb, request, TRUE );
	    break;

	case GCR_SEND:
	case GCR_RECEIVE:
	case GCR_PURGE:
	case GCR_DISCONN:
	    /*
	    ** Call the embedded Comm Server interface
	    ** State Machine to process these requests.
	    ** The state machine handles the callbacks.
	    */
	    rcb = (GCR_CB *)svc_parms->gc_cb;
	    rcb->svc_parms = svc_parms;
	    status = gcr_gcc_sm( rcb, request, TRUE );
	    break;

	case GCR_RELEASE:
	    /*
	    ** Free the router control block.  GCrelease()
	    ** does not do a callback.
	    */
	    if ( (rcb = (GCR_CB *)svc_parms->gc_cb) )
	    {
		/*
		** A release is requested if and only if GCA has
		** disconnected.  The disconnect completed only
		** after the embedded Comm Server issued a dis-
		** connect request.  When the disconnect completed, 
		** the embedded Comm Server callback was done and 
		** the GCC AL released its lock on the CCB.  How-
		** ever, it is still possible that the GCC TL has
		** the CCB locked waiting for additional disconnect
		** processing.  From the GCA perspective we need
		** to free the router control block, but we also
		** need a way for a terminate request to know that
		** additional GCC TL processing is still required.
		**
		** We remove the router control block from the ACB.
		** If the GCC TL has released its lock on the CCB,
		** the router control block will be freed.  Other-
		** wise, the control block will be saved on a queue
		** until the GCC TL processing has completed.
		*/
		gcr_release_cb( rcb );
		svc_parms->gc_cb = NULL;
	    }
	    break;

	/*
	** The embedded Comm Server does not support
	** saving/restoring a connection due to network
	** protocol limitations.
	*/
	case GCR_SAVE:
	case GCR_RESTORE:

	/*
	** The following requests can not be issued 
	** for a remote connection.
	*/
	case GCR_INITIATE:
	case GCR_REGISTER:
	case GCR_LISTEN:
	case GCR_RQRESP:
	case GCR_TERMINATE:

	/*
	** Invalid GCR request codes.
	*/
	default :
	    GCA_DEBUG_MACRO(5)("gcr_call_gcc: invalid request %d\n", request);
	    status = E_GC0003_INV_SVC_CODE;
	    callback = TRUE;
	    break;
    }

    if ( callback )
    {
        svc_parms->status = status;

        if ( svc_parms->gca_cl_completion )
	{
	    GCA_DEBUG_MACRO(4)("%04d     GCR making GCA callback\n", (i4)-1);
	    (*svc_parms->gca_cl_completion)( svc_parms->closure );
	}
    }

    return( status );
}


/*
** Name: gcr_call_gca
**
** Description:
**	Process a GCA request from the embedded Comm Server.  This
**	usually gets converted into a callback to a preceeding 
**	request from GCA.
**
**	We make a lot of assumptions here based on our knowledge
**	of how GCC makes GCA calls and how we initiate requests to
**	GCC.  For example, we assume only certain GCR requests will
**	be made, and we ignore the indicator flags and time-out
**	values which we assume we already know.
**
** Input:
**	request		GCR request id.
**	parmlist	GCA parameter list.
**	indicators	Request flags.
**	async_id	Async closure.
**	time_out	Request time-out.
**
** Output:
**	status		Result Status.
**
** Returns:
**	STATUS
**
** History:
**	23-Oct-95 (gordy)
**	    Created.
*/

static STATUS	
gcr_call_gca( i4  request, GCA_PARMLIST *parmlist, i4  indicators, 
	      PTR async_id, i4 time_out, STATUS *status )
{
    GCR_CB	*rcb;
    STATUS	r_stat = OK;

    GCA_DEBUG_MACRO(4)("%04d     GCR routing GCC request to GCA via callback\n",
			(i4)-1);

    switch( request )
    {
	case GCR_RQRESP :
	case GCR_SEND :
	case GCR_RECEIVE :
	case GCR_DISCONN:
	    rcb = gcr_find_rcb( ((GCA_ALL_PARMS *)parmlist)->gca_assoc_id );

	    if ( ! rcb )
	    {
		*status = E_GC0005_INV_ASSOC_ID;
		r_stat = E_DB_ERROR;
	    }
	    else
	    {
		rcb->parmlist = parmlist;
		rcb->mde = async_id;
		*status = gcr_gcc_sm( rcb, request, FALSE );
		if ( *status != OK )  r_stat = E_DB_ERROR;
	    }
	    break;
	
	/*
	** The following requests are not expected
	** from the embedded Comm Server.
	*/
	case GCR_INITIATE:
	case GCR_REGISTER:
	case GCR_LISTEN:
	case GCR_REQUEST:
	case GCR_PURGE:
	case GCR_RELEASE:
	case GCR_SAVE:
	case GCR_RESTORE:
	case GCR_TERMINATE:

	/*
	** Invalid GCR request codes.
	*/
	default :
	    GCA_DEBUG_MACRO(5)("gcr_call_gca: invalid request %d\n", request);
	    *status = E_GC0003_INV_SVC_CODE;
	    r_stat = E_DB_ERROR;
	    break;
    }

    return( r_stat );
}


/*
** Name: gcr_init
**
** Description:
**	Initialize the router global data area.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	 9-Jan-96 (gordy)
**	    Created.
**	 4-Aug-99 (rajus01)
**	    Initialize gcr_embed_flag.
*/

static STATUS
gcr_init( VOID )
{
    char   *embed = NULL;

    if ( ! IIGCr_static )
    {
	IIGCr_static = (GCR_STATIC *)gca_alloc( sizeof( GCR_STATIC ) );

	if ( ! IIGCr_static )
	{
	    GCA_DEBUG_MACRO(3)( "gcr_init: error allocating IIGCr_static.\n" );
	    return( E_GC0002_INV_PARM );
	}

	MEfill( sizeof( GCR_STATIC ), (u_char)0, (PTR)IIGCr_static );

	/*
	** Initialize gcr_embed_flag
	*/
	IIGCr_static->gcr_embed_flag = TRUE;

	NMgtAt( "II_EMBEDDED_GCC", &embed );
	if ( embed  || *embed )
	    if ( ! STbcompare( embed, 0, "OFF", 0, TRUE )      ||
		 ! STbcompare( embed, 0, "DISABLED", 0, TRUE ) || 
		 ! STbcompare( embed, 0, "FALSE", 0, TRUE ) )
		IIGCr_static->gcr_embed_flag = FALSE;

	QUinit( &IIGCr_static->gcr_active_q );
	QUinit( &IIGCr_static->gcr_wait_q );
	QUinit( &IIGCr_static->gcr_free_q );
    }

    return( OK );
}


/*
** Name: gcr_term
**
** Description:
**	Shutdown the embedded Comm Server router.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 9-Jan-96 (gordy)
**	    Created.
**	14-Nov-96 (gordy)
**	    GCsync() now takes a timeout value.
**	 4-Aug-99 (rajus01)
**	    Add check for standalone comsvr before terminating
**	    embedded comsvr. 
*/

static STATUS
gcr_term( VOID )
{
    QUEUE	*q;
    STATUS	status = OK;

    if ( IIGCr_static )
    {
	/*
	** Clear out any waiting control blocks by giving
	** the GCC TL a chance to finish processing.
	**
	** At this point we have our fingers crossed that
	** this doesn't hang.
	*/
	while( gcr_check_cb() )
	{
	    GCA_DEBUG_MACRO(4)( "%04d     GCR waiting for GCC TL.\n",
				(i4)-1);
	    GCsync( -1 );
	}

	/*
	** Empty the control block queues.  The active
	** and wait queues should be empty, but check
	** just in case.  These queue entries are moved
	** to the free queue using gcr_free_cb() to free
	** any associated resources.  Control blocks on
	** the free queue are then removed and freed.
	*/
	while(IIGCr_static->gcr_active_q.q_next != &IIGCr_static->gcr_active_q)
	    gcr_free_cb( (GCR_CB *)IIGCr_static->gcr_active_q.q_next );

	while( IIGCr_static->gcr_wait_q.q_next != &IIGCr_static->gcr_wait_q ) 
	    gcr_free_cb( (GCR_CB *)IIGCr_static->gcr_wait_q.q_next );

	while( (q = QUremove( IIGCr_static->gcr_free_q.q_next )) )  
	    gca_free( (PTR)q );

	/*
	** Free the global data area.
	*/
	gca_free( (PTR)IIGCr_static );
	IIGCr_static = NULL;
    }

    /*
    ** Existence of the GCC global data area indicates the
    ** need to shut down the embedded Comm Server.
    */
    if ( IIGCc_global  &&  ! (IIGCc_global->gcc_flags & GCC_STANDALONE) )
    {
	GCA_DEBUG_MACRO(4)( "%04d     GCR terminating embedded GCC\n",
			    (i4)-1);

	status = gcr_term_gcc();
    }

    return( status );
}


/*
** Name: gcr_alloc_cb
**
** Description:
**	Allocate a router control block.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	GCR_CB *	Pointer to control block, NULL if error.
**
** History:
**	 9-Jan-96 (gordy)
**	    Created.
*/

static GCR_CB *
gcr_alloc_cb( VOID )
{
    GCR_CB *rcb;

    /*
    ** Remove first free control block from free queue, 
    ** will get NULL if queue is empty.  Allocate a new
    ** control block if queue is empty.
    */
    rcb = (GCR_CB *)QUremove( IIGCr_static->gcr_free_q.q_next );

    if ( ! rcb ) 
    {
        rcb = (GCR_CB *)gca_alloc( sizeof( GCR_CB ) );
	rcb->assoc_id = gcr_assoc_id++;
    }

    /*
    ** Place control block on the active queue.
    */
    if ( rcb )  
    {
	u_i4 	assoc;

	assoc = rcb->assoc_id;
	MEfill( sizeof( GCR_CB ), (u_char)0, (PTR)rcb );
	rcb->assoc_id = assoc;
	QUinit( &rcb->q );
	QUinsert( &rcb->q, &IIGCr_static->gcr_active_q );
    }


    GCA_DEBUG_MACRO(4) 
    	("%04d     GCR allocating router control block (0x%x).\n",
					rcb->assoc_id, (i4)rcb );
    return( rcb );
}


/*
** Name: gcr_free_cb
**
** Description:
**	Free a router control block and associated resources.
**
** Input:
**	rcb		Router control block.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 9-Jan-96 (gordy)
**	    Created.
*/

static VOID
gcr_free_cb( GCR_CB *rcb )
{
    GCC_CCB	*ccb = (GCC_CCB *)rcb->ccb;

    GCA_DEBUG_MACRO(4)("%04d     GCR freeing router control block (0x%x).\n",
			rcb->assoc_id, (i4)rcb );

    if ( ccb )
    {
	/*
	** Check the CCB allocated in gcr_request().
	** Remove our lock on the CCB. If all other
	** owners (GCC AL, GCC TL) have unlocked the 
	** CCB, free it.
	*/
	if ( --ccb->ccb_hdr.async_cnt <= 0 )
	    gcc_del_ccb( ccb );
	else
	{
	    GCA_DEBUG_MACRO(3)( "gcr_free_cb: CCB still in use (%d).\n",
				ccb->ccb_hdr.async_cnt );
	}

	rcb->ccb = NULL;
    }

    if ( rcb->flags & GCR_F_FS )  
    {
	/*
	** Free the fast select data buffer allocated in gcr_fs_info().
	*/
	gca_free( (PTR)rcb->fs_data );
	rcb->fs_data = NULL;
	rcb->flags &= ~GCR_F_FS;
    }

    if( rcb->aux_data )
    {
	/*
	** Free the aux_data buffer allocated in gcr_peer_info().
	*/
	gca_free( (PTR)rcb->aux_data );
	rcb->aux_data = NULL;
    }

    /*
    ** Move the control block to the free queue.
    */
    QUinsert( QUremove( &rcb->q ), &IIGCr_static->gcr_free_q );

    return;
}



/*
** Name: gcr_release_cb
**
** Description:
**	Check the state of a router control block and free if possible.
**	If the control block is still active, save it on a queue to be
**	checked again after additional processing.
**
**	At the point that GCA is ready to release association resources,
**	it is possible that the embedded Comm Server is still waiting for
**	some additional network processing to complete.  We need to be
**	able to detect this condition when a GCA terminate request is
**	made so that processing time can be given to the embedded Comm
**	Server to complete the disconnection at the GCC TL level.  If
**	the GCC TL has not released its lock on the CCB, then the router
**	control block is saved on a queue so that the progress of the
**	disconnect can be monitored.
**
** Input:
**	rcb		Router control block.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	 9-Jan-96 (gordy)
**	    Created.
*/

static VOID
gcr_release_cb( GCR_CB *rcb )
{
    GCC_CCB	*ccb = (GCC_CCB *)rcb->ccb;

    /*
    ** If there is no CCB or we are the only owner
    ** of the CCB, free the router control block
    ** Otherwise, move the control block to the
    ** wait queue to be checked later.
    */
    if ( ! ccb  ||  ccb->ccb_hdr.async_cnt <= 1 )
	gcr_free_cb( rcb );
    else
    {
	GCA_DEBUG_MACRO(4)( "%04d     GCR ccb still in use (%d).\n",
			    (i4)-1, ccb->ccb_hdr.async_cnt );

	QUinsert( QUremove( &rcb->q ), &IIGCr_static->gcr_wait_q );
    }

    return;
}


/*
** Name: gcr_check_cb
**
** Description:
**	Free any control blocks on the wait queue for which the
**	associated CCB is no longer locked by GCC AL/TL.  
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns
**	bool		TRUE if wait queue is not empty.
**
** History:
**	 9-Jan-96 (gordy)
**	    Created.
*/

static bool
gcr_check_cb( VOID )
{
    QUEUE	*qptr, *next;
    GCR_CB	*rcb;
    GCC_CCB	*ccb;

    /*
    ** Scan router control block wait queue.
    */
    for( 
	 qptr = IIGCr_static->gcr_wait_q.q_next; 
	 qptr != &IIGCr_static->gcr_wait_q; 
	 qptr = next 
       )
    {
	next = qptr->q_next;
	rcb = (GCR_CB *)qptr;
	ccb = (GCC_CCB *)rcb->ccb;

	/*
	** The router control block can be freed
	** if we are the only owner of the CCB,
	*/
	if ( ! ccb  ||  ccb->ccb_hdr.async_cnt <= 1 )
	    gcr_free_cb( rcb );
    }

    /*
    ** Return TRUE if any router control
    ** blocks still on the wait queue.
    */
    return( IIGCr_static->gcr_wait_q.q_next != &IIGCr_static->gcr_wait_q );
}


/*
** Name: gcr_find_rcb
**
** Description:
**	gcr_find_rcb accepts an association ID as input using
**	which it finds the matching router control
**	block(rcb) from the active rcb queue.
**
** Input:
**	assoc_id	Association ID
**
** Output:
**	None.
**
** Returns:
**	GCR_CB *	Pointer to the router control block or
**			NULL if invalid ID.
**
** History:
**	 2-Jul-99 (rajus01)
**	    Created.
*/

static	
GCR_CB	*gcr_find_rcb( i4 assoc_id )
{
    QUEUE	*qptr;
    GCR_CB	*rcb;

    /*
    ** Scan router control block in the active queue.
    */
    for( 
	 qptr = IIGCr_static->gcr_active_q.q_next; 
	 qptr != &IIGCr_static->gcr_active_q; 
	 qptr = qptr->q_next 
       )
    {
	rcb = (GCR_CB *)qptr;
	if ( rcb->assoc_id == assoc_id )
	    return( rcb );
    }

    return( NULL );
}

#endif /* GCF_EMBEDDED_GCC */

