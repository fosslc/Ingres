/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <st.h>
# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apichndl.h>
# include <apilower.h>
# include <apidspth.h>
# include <apimisc.h>
# include <apimsg.h>
# include <apitrace.h>

/*
** Name: apilower.c
**
** Description:
**	This file contains the API lower level interface to GCA.
**
**      IIapi_initGCA		GCA_INITIATE service invocation
**      IIapi_termGCA		GCA_TERMINATE service invocation
**      IIapi_connGCA		GCA_REQUEST service invocation
**      IIapi_disconnGCA	GCA_DISASSOC service invocation
**	IIapi_abortGCA		GCA_DISASSOC (sync) server invocation
**      IIapi_sndGCA		GCA_SEND service invocation
**      IIapi_rcvNormalGCA	Normal GCA_RECEIVE service invocation
**      IIapi_rcvExpediteGCA	Expedited GCA_RECEIVE service invocation
**	IIapi_resumeGCA		Resume a GCA request
**
** History:
**      15-oct-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          IIapi_liEventDispatch is updated to deleted connection,
**          transaction, statement and event handle after the
**          state table marks them deletable.  Same change is
**          made to IIapi_uiEventDispatch.
**      19-apr-94 (ctham)
**          IIapi_connGCA() is updated to set the local protocol level
**          to GCA_PROTOCOL_LEVEL_60.
**      22-apr-94 (ctham)
**          provide a new function IIapi_serviceOpQue() to remove the
**          IIapi_liDispatch(), IIapi_uiDispatch() and IIapi_wait()
**          from going through and dealing with queued events.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	27-Oct-94 (gordy)
**	    Accept generic handle as input parameter.
**	14-Nov-94 (gordy)
**	    Can receive GCA_REJECT in response to GCA_MD_ASSOC.
**	18-Nov-94 (gordy)
**	    Set the API level for the GCA protocol in effect.
**	 9-Dec-94 (gordy)
**	    Handle unexpected GCA messages.
**	13-Dec-94 (gordy)
**	    Return sizeAdvise to client.
**	17-Jan-95 (gordy)
**	    GCA protocol level depends on what is available
**	    from GCA.  Use our own symbol rather than GCA symbol.
**	    Use local typedefs to handle changes in GCA_SD_PARMS 
**	    and GCA_RV_PARMS.
**	20-Jan-95 (gordy)
**	    Flag when GCA becomes active and completes.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 3-Mar-95
**	    Changed return type of IIapi_termGCA().
**	 9-Mar-95 (gordy)
**	    Converted dispatch tables to function.
**	 9-Mar-95 (gordy)
**	    Need evaluated event from dispatcher.
**	24-Mar-95 (gordy)
**	    Add II_CALLBACK symbol to callback routines.
**	25-Apr-95 (gordy)
**	    Removed GCA_EVENT code which is now handled by state machines.
**	10-May-95 (gordy)
**	    Set API level to 4 for remote user/password info.  Set
**	    request parm flag GCA_RQ_REMPW when using remote user/password.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	13-Jun-95 (gordy)
**	    Added IIAPI_GCA_BUFF to support buffering of GCA messages.
**	14-Jun-95 (gordy)
**	    Extracted handles prior to calling state managers in case
**	    the original handle is deleted before all are called.
**	20-Jun-95 (gordy)
**	    Set GCA formatted flag according to buffer parameter setting.
**	28-Jul-95 (gordy)
**	    Use fixed length types.  Fixed tracing of pointers.
**      26-Oct-95 (loera01)
**	    Changed buffer length for reads in IIapi_rcvNormalGCA from 
**          sizeAdvise to API_GCA_BUF_SIZE.  This allows VMS compatability 
**          when large buffers are read.
**	31-Oct-95 (gordy)
**	    Remove remote passwords.  The gca_user_name and gca_password
**	    parms are for the local installation, which may be different
**	    from the remote installation.  Use current API username and 
**	    password for local.  Need to add remote username and password 
**	    parameters to IIapi_connect().  Do not need to allocate dynamic 
**	    memory for some request parms as this has already been done in 
**	    the connection handle.
**	21-Dec-95 (gordy)
**	    Fix memory leak in IIapi_sndGCA(), deleteSend().
**	17-Jan-96 (gordy)
**	    Added environment handles.  Added global data structure.
**	    Implemented short functions as macros.
**	23-Feb-96 (gordy)
**	    Converted copy tuple processing to use existing tuple processing.
**	    Removed IIAPI_EV_CDATA_RCVD.
**	 8-Mar-96 (gordy)
**	    Added events to allow handling of split messages.  Added the
**	    ability to expand receive buffers if fail because buffer is
**	    too small.
**	 3-Apr-96 (gordy)
**	    Fixed casts in IIGCa_call().
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
**	26-Apr-96 (gordy)
**	    Expedited channel not tracked by GCA activity mechanism.
**	24-May-96 (gordy)
**	    Enhanced tagged memory support.
**	24-Jun-96 (gordy)
**	    Condensed most SENT events into one.  With only a few
**	    exceptions, we don't care what message is sent.  The state
**	    determines what happens once a message is sent, not the
**	    message type.
**	15-Jul-96 (gordy)
**	    Added timeout parameter to IIapi_rcvNormalGCA().
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
**	22-Nov-96 (gordy)
**	    Converted to new GCA interface, gca_call().
**	31-Jan-97 (gordy)
**	    Added het-net descriptor to IIapi_sndGCA() and flag to 
**	    connection handle.
**	10-Feb-97 (gordy)
**	    Added IIapi_abortGCA() for synchronous connection shutdown.
**	21-Feb-97 (gordy)
**	    Changed gca_formatted to gca_modifiers.
**	27-Feb-97 (gordy)
**	    Extracted buffer allocation to IIapi_allocGCABuffer().
**	10-Mar-97 (gordy)
**	    Connection parameters now available in connection handle.
**	13-Mar-97 (gordy)
**	    Added support for GCN message types.
**	25-Mar-97 (gordy)
**	    Bumped protocol level to 63 to support new Name Server protocol.
**	30-Apr-97 (gordy)
**	    Repost purged receive requests.
**	29-May-97 (gordy)
**	    When both username and password are provided, pass to GCA as
**	    remote username and password.  GCA/GCN now use the remote
**	    values for both local and remote connections.  Passwords
**	    without username are ignored by GCA.  Username without a
**	    password is only meaningful for trusted clients, and then
**	    only as local GCA username and password.
**	29-Oct-97 (gordy)
**	    Do not reset GCA started flag during resume.
**	10-Oct-98 (rajus01)
**	    Added GCA_PROMPT message.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	 1-Mar-01 (gordy)
**	    Added support for GCA protocol level 64 (Unicode datatypes).
**	12-Jul-02 (gordy)
**	    Fixing problems with long column values.  Max column length
**	    was bumped to 32000.  Reseting GCA buffer size to a more
**	    reasonable value.  Adding IIapi_expandBuffer() to copy data.
**      19-Aug-2002(wansh01)
**          Added support for login type local as a connection
**          parameter. 
**	15-Mar-04 (gordy)
**	    Add 8 byte int support: new API version & level, GCA protocol.
**	 6-Oct-04 (gordy)
**	    Make sure sizeAdvise is at least large enough for a tuple.
**	31-May-06 (gordy)
**	    New API version & level and GCA protocol.
**	14-Jul-2006 (kschendel)
**	    Remove u_i2 casts from MEcopy.
**	25-Oct-06 (gordy)
**	    Support Blob/Clob locators: new API version & level, GCA protocol.
**	 1-Oct-09 (gordy)
**	    New GCA protocol level for long procedure names.
**	26-Oct-09 (gordy)
**	    New API version 7, level 6 for GCA protcol 68.
**	35-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.
**	    Removed receive buffer and replaced with combined
**	    send/receive buffer.  Enhanced parameter memory 
**	    block handling.
*/

# define API_GCA_PROTOCOL_LEVEL		GCA_PROTOCOL_LEVEL_68
# define API_V6_PROTOCOL_LEVEL		GCA_PROTOCOL_LEVEL_67
# define API_V5_PROTOCOL_LEVEL		GCA_PROTOCOL_LEVEL_66
# define API_V4_PROTOCOL_LEVEL		GCA_PROTOCOL_LEVEL_65
# define API_V3_PROTOCOL_LEVEL		GCA_PROTOCOL_LEVEL_64
# define API_V1_PROTOCOL_LEVEL		GCA_PROTOCOL_LEVEL_63
# define API_GCA_API_LEVEL		GCA_API_LEVEL_5


/*
** Name: IIAPI_CLOSURE
**
** Description:
**      This data type defines the generic closure
**
**      All parameters within this structure must be prefixed with cl_.
**
**      cl_eventNo
**          event number.  This parameter is the event number corresponding
**          to the expected API input event.
**      cl_handle
**	    API handle which invoked the GCA service.
**	cl_resumeGCAService
**	    GCA service being performed.  Used to resume an incomplete
**	    service.
**	cl_resumeGCAFlags
**	    GCA flags to be used when resuming GCA service.
**      cl_cmplFunc
**          completion function.  This parameter contains the function
**          address of the appropriate completion function to be invoked
**          when GCA finishes the tasks of the original service.
**	cl_apiParm
**	    API parameter block.
**	cl_gcaParm
**	    Union of GCA parameter lists.
**
** History:
**      10-sep-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 3-Mar-95 (gordy)
**	    Changed the return type of IIapi_termGCA().
**	28-Jul-95 (gordy)
**	    Use fixed length types.
**	 8-Mar-96 (gordy)
**	    Added the ability to expand receive buffers if fail 
**	    because buffer is too small.
**	25-Mar-10 (gordy)
**	    Include space for GCA parameter blocks.
*/

typedef struct IIAPI_CLOSURE
{
    
    IIAPI_EVENT		cl_eventNo;
    IIAPI_HNDL		*cl_handle;
    i4			cl_resumeGCAService;
    i4			cl_resumeGCAFlags;
    II_VOID		(*cl_cmplFunc)(II_PTR closure);
    II_PTR		cl_apiParm;

    union
    {
	GCA_RQ_PARMS	rqst;
	GCA_DA_PARMS	disc;
	GCA_SD_PARMS	send;
	GCA_RV_PARMS	recv;
    } cl_gcaParm;

} IIAPI_CLOSURE;

# define IIAPI_NO_RESUME -1


/*
** Completion routines.
*/
II_EXTERN II_VOID II_CALLBACK	genericCmpl( II_PTR asyncID );
II_EXTERN II_VOID II_CALLBACK	reqCmpl( II_PTR closure );
II_EXTERN II_VOID II_CALLBACK	disasscCmpl( II_PTR closure );
II_EXTERN II_VOID II_CALLBACK	sndCmpl( II_PTR closure );
II_EXTERN II_VOID II_CALLBACK	rcvNormalCmpl( II_PTR closure );
II_EXTERN II_VOID II_CALLBACK	rcvExpediteCmpl( II_PTR closure );


/*
** Local Functions
*/
static II_VOID IIapi_gcaStarted( II_VOID );
static II_VOID IIapi_gcaDone( II_VOID );
static II_PTR  createAbortParm( STATUS gcaError );
static II_VOID deleteAbortParm( II_PTR parmBlock );



/*
** Name: eventNoTBL
**
** Description:
**	This table converts GCA message type to event number.
**
** Re-entrancy:
**	This table is read-only.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	14-Nov-94 (gordy)
**	    Can receive GCA_REJECT in response to GCA_MD_ASSOC.
**	 9-Dec-94 (gordy)
**	    Handle unexpected GCA messages.
**	23-Feb-96 (gordy)
**	    Converted copy tuple processing to use existing tuple processing.
**	    Removed IIAPI_EV_CDATA_RCVD.
**	 8-Mar-96 (gordy)
**	    Added events to allow handling of split messages.
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
*/

static IIAPI_EVENT
eventNoTBL[] =
{
    /* INVALID MESSAGE  0x00 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* INVALID MESSAGE  0x01 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_MD_ASSOC     0x02 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_ACCEPT	0x03 */ IIAPI_EV_ACCEPT_RCVD,
    /* GCA_REJECT	0x04 */ IIAPI_EV_REJECT_RCVD,
    /* GCA_RELEASE	0x05 */ IIAPI_EV_RELEASE_RCVD,
    /* GCA_Q_BTRAN	0x06 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_S_BTRAN	0x07 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_ABORT	0x08 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_SECURE	0x09 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_DONE		0x0A */ IIAPI_EV_DONE_RCVD,
    /* GCA_REFUSE	0x0B */ IIAPI_EV_REFUSE_RCVD,
    /* GCA_COMMIT	0x0C */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_QUERY	0x0D */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_DEFINE	0x0E */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_INVOKE	0x0F */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_FETCH	0x10 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_DELETE	0x11 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_CLOSE	0x12 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_ATTENTION	0x13 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_QC_ID	0x14 */ IIAPI_EV_QCID_RCVD,
    /* GCA_TDESCR	0x15 */ IIAPI_EV_TDESCR_RCVD,
    /* GCA_TUPLES	0x16 */ IIAPI_EV_TUPLE_RCVD,
    /* GCA_C_INTO	0x17 */ IIAPI_EV_CINTO_RCVD,
    /* GCA_C_FROM	0x18 */ IIAPI_EV_CFROM_RCVD,
    /* GCA_CDATA	0x19 */ IIAPI_EV_TUPLE_RCVD,
    /* GCA_ACK		0x1A */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_RESPONSE	0x1B */ IIAPI_EV_RESPONSE_RCVD,
    /* GCA_ERROR	0x1C */ IIAPI_EV_ERROR_RCVD,
    /* GCA_TRACE	0x1D */ IIAPI_EV_TRACE_RCVD,
    /* GCA_Q_ETRAN	0x1E */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_S_ETRAN	0x1F */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_IACK		0x20 */ IIAPI_EV_IACK_RCVD,
    /* GCA_NP_INTERRUPT 0x21 */ IIAPI_EV_NPINTERUPT_RCVD,
    /* GCA_ROLLBACK	0x22 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_Q_STATUS	0x23 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_CREPROC	0x24 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_DRPPROC	0x25 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_INVPROC	0x26 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_RETPROC	0x27 */ IIAPI_EV_RETPROC_RCVD,
    /* GCA_EVENT	0x28 */ IIAPI_EV_EVENT_RCVD,
    /* GCA1_C_INTO	0x29 */ IIAPI_EV_CINTO_RCVD,
    /* GCA1_C_FROM	0x2A */ IIAPI_EV_CFROM_RCVD,
    /* GCA1_DELETE	0x2B */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_XA_SECURE	0x2C */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA1_INVPROC	0x2D */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA_PROMPT	0x2E */ IIAPI_EV_PROMPT_RCVD,
    /* GCA_PRREPLY	0x2F */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA1_FETCH 	0x30 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA2_FETCH 	0x31 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCA2_INVPROC	0x32 */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x33 */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x34 */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x35 */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x36 */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x37 */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x38 */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x39 */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x3A */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x3B */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x3C */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x3D */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x3E */ IIAPI_EV_UNEXPECTED_RCVD,
    /*            	0x3F */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCN_NS_RESOLVE   0x40 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCN_RESOLVED     0x41 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCN_NS_OPER      0x42 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCN_RESULT       0x43 */ IIAPI_EV_GCN_RESULT_RCVD,
    /* GCN_NS_AUTHORIZE 0x44 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCN_AUTHORIZED	0x45 */ IIAPI_EV_UNEXPECTED_RCVD,
    /* GCN_NS_2_RESOLVE	0x46 */ IIAPI_EV_UNEXPECTED_RCVD
};


static	PTR	gca_cb = NULL;



/*
** Name: IIapi_initGCA
**
** Description:
**	This function invokes a GCA_INITIATE service to initialize
**      the GCA module.
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	17-Jan-95 (gordy)
**	    GCA protocol level depends on what is available
**	    from GCA.  Use our own symbol rather than GCA symbol.
**	    Conditionalize code for older GCA interface.
**	20-Jan-95 (gordy)
**	    Flag when GCA becomes active and completes.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	22-Nov-96 (gordy)
**	    Converted to new GCA interface, gca_call().
*/

II_EXTERN II_BOOL
IIapi_initGCA( II_LONG timeout )
{
    STATUS		status;
    GCA_IN_PARMS	initParm;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "IIapi_initGCA: Initializing GCA\n" );
    
    /*
    ** Set up GCA_INITIATE parameters.
    */
    MEfill( sizeof( initParm ), '\0', (PTR)&initParm );

#ifdef GCA_API_LEVEL_5
    initParm.gca_service = GCA_INITIATE;
#endif
    initParm.gca_modifiers = (GCA_ASY_SVC | GCA_API_VERSION_SPECD);
    initParm.gca_local_protocol = API_GCA_PROTOCOL_LEVEL;
    initParm.gca_api_version = GCA_API_LEVEL_5;
    initParm.gca_expedited_completion_exit = genericCmpl;
    initParm.gca_normal_completion_exit = genericCmpl;
    
    if ( gca_call( &gca_cb, GCA_INITIATE, (GCA_PARMLIST *)&initParm, 
    		   GCA_SYNC_FLAG, NULL, timeout, &status ) != E_DB_OK )
	return( FALSE );
    
    return( TRUE );
}




/*
** Name: IIapi_termGCA
**
** Description:
**	This function invokes a GCA_TERMINATE service to terminate
**      the GCA module.
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	17-Jan-95 (gordy)
**	    Conditionalize code for older GCA interface.
**	20-Jan-95 (gordy)
**	    Flag when GCA becomes active and completes.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 3-Mar-95 (gordy)
**	    Fixed return type.
**	22-Nov-96 (gordy)
**	    Converted to new GCA interface, gca_call().
*/

II_EXTERN II_VOID
IIapi_termGCA( II_VOID )
{
    STATUS	    status;
    GCA_TE_PARMS    termParm;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "IIapi_termGCA: Terminating GCA\n" );
    
    /*
    ** Set up GCA_TERMINATE parameters.
    */
    MEfill( sizeof( termParm ), '\0', (PTR)&termParm );

#ifdef GCA_API_LEVEL_5
    termParm.gca_service = GCA_TERMINATE;
#endif
    
    gca_call( &gca_cb, GCA_TERMINATE, 
	      (GCA_PARMLIST *)&termParm, GCA_SYNC_FLAG, NULL, -1, &status );
    
    return;
}




/*
** Name: IIapi_connGCA
**
** Description:
**	This function invokes a GCA_REQUEST service to establish a
**      connection with the DBMS server.
**
** Input:
**	handle		Connection or Transaction handle.
**	timeout		Timeout value in milli-seconds, -1 for no timeout.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_SUCCESS 
**			IIAPI_ST_FAILURE 
**			IIAPI_ST_OUT_OF_MEMORY
**
** History:
**      15-oct-93 (ctham)
**          creation
**      19-apr-94 (ctham)
**          IIapi_connGCA() is updated to set the local protocol level
**          to GCA_PROTOCOL_LEVEL_60.
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	27-Oct-94 (gordy)
**	    Accept generic handle as input parameter.
**	17-Jan-95 (gordy)
**	    GCA protocol level depends on what is available
**	    from GCA.  Use our own symbol rather than GCA symbol.
**	    Conditionalize code for older GCA interface.
**	20-Jan-95 (gordy)
**	    Flag GCA Active.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	10-May-95 (gordy)
**	    Set API level to 4 for remote user/password info.  Set
**	    request parm flag GCA_RQ_REMPW when using remote user/password.
**	31-Oct-95 (gordy)
**	    Remove remote passwords.  The gca_user_name and gca_password
**	    parms are for the local installation, which may be different
**	    from the remote installation.  Use current API username and 
**	    password for local.  Need to add remote username and password 
**	    parameters to IIapi_connect().  Do not need to allocate dynamic 
**	    memory for some request parms as this has already been done in 
**	    the connection handle.
**	24-Jun-96 (gordy)
**	    Condensed most SENT events into one.  With only a few
**	    exceptions, we don't care what message is sent.  The state
**	    determines what happens once a message is sent, not the
**	    message type.
**	16-Oct-96 (gordy)
**	    Allow non-connection handles to support reconnecting to
**	    a distributed connection.
**	22-Nov-96 (gordy)
**	    Converted to new GCA interface, gca_call().
**	10-Mar-97 (gordy)
**	    No longer need connection parameters since they are all
**	    available in the connection handle except for timeout
**	    which is now passed as a function parameter.
**	29-May-97 (gordy)
**	    When both username and password are provided, pass to GCA as
**	    remote username and password.  GCA/GCN now use the remote
**	    values for both local and remote connections.  Passwords
**	    without username are ignored by GCA.  Username without a
**	    password is only meaningful for trusted clients, and then
**	    only as local GCA username and password.
**	 1-Mar-00 (gordy)
**	    New API VERSION enables use of GCA Unicode protocol level.
**      19-Aug-2002(wansh01)
**          Only when both username and password are provided and ch_flag   
**          is not set to IIAPI_CH_LT_LOCAL then passed to GCA as remote
**	    username and password. 
**	15-Mar-04 (gordy)
**	    Add 8 byte int support: new API version & level, GCA protocol.
**	31-May-06 (gordy)
**	    New API version & level and GCA protocol.
**	25-Oct-06 (gordy)
**	    Support Blob/Clob locators: new API version & level, GCA protocol.
**	26-Oct-09 (gordy)
**	    New API version 7, level 6 and GCA protcol 68.
**	25-Mar-10 (gordy)
**	    GCA parameters moved to closure.
*/

II_EXTERN IIAPI_STATUS
IIapi_connGCA( IIAPI_HNDL *handle, II_LONG timeout )
{
    IIAPI_ENVHNDL	*envHndl;
    IIAPI_CONNHNDL	*connHndl;
    GCA_RQ_PARMS	*reqParm;
    IIAPI_CLOSURE	*closure;
    IIAPI_STATUS	api_status;
    STATUS		status;
    i4			gca_flags = GCA_ASYNC_FLAG;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "IIapi_connGCA: connecting via GCA\n" );
    
    if ( ! (envHndl = IIapi_getEnvHndl( handle ))  ||
	 ! (connHndl = IIapi_getConnHndl( handle )) )
    {
	/*
	** This is an internal error which (in a perfect world)
	** should not happen.  Fail the operation so an error
	** will be detected.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_connGCA: API ERROR - no connection handle\n" );
	return( IIAPI_ST_FAILURE );
    }

    if ( ! ( closure = (IIAPI_CLOSURE *)
	     MEreqmem( 0, sizeof(IIAPI_CLOSURE), FALSE, NULL ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_connGCA: closure allocation failed\n" );
	return( IIAPI_ST_OUT_OF_MEMORY );
    }
    
    /*
    ** Set up GCA_REQUEST parameters
    */
    reqParm = &closure->cl_gcaParm.rqst;
#ifdef GCA_API_LEVEL_5
    reqParm->gca_service = GCA_REQUEST;
    reqParm->gca_completion = reqCmpl;
    reqParm->gca_closure = (II_PTR)closure;
    gca_flags |= GCA_ALT_EXIT;
#endif
    reqParm->gca_partner_name = connHndl->ch_target;
    reqParm->gca_modifiers = 0;
    reqParm->gca_l_auth = 0;
    reqParm->gca_auth = NULL;
    reqParm->gca_sec_label = NULL;

    switch( envHndl->en_version )
    {
    case IIAPI_VERSION_1 : 
    case IIAPI_VERSION_2 : 
	reqParm->gca_peer_protocol = API_V1_PROTOCOL_LEVEL;
	break;

    case IIAPI_VERSION_3 : 
	reqParm->gca_peer_protocol = API_V3_PROTOCOL_LEVEL;
	break;

    case IIAPI_VERSION_4 : 
	reqParm->gca_peer_protocol = API_V4_PROTOCOL_LEVEL;
	break;

    case IIAPI_VERSION_5 :
	reqParm->gca_peer_protocol = API_V5_PROTOCOL_LEVEL;
	break;

    case IIAPI_VERSION_6 :
	reqParm->gca_peer_protocol = API_V6_PROTOCOL_LEVEL;
	break;

    default :
	reqParm->gca_peer_protocol = API_GCA_PROTOCOL_LEVEL;
	break;
    }
  
    if ( connHndl->ch_username  &&  connHndl->ch_password  && 
	 !(connHndl->ch_flags & IIAPI_CH_LT_LOCAL) )
    {
	reqParm->gca_modifiers |= GCA_RQ_REMPW;
	reqParm->gca_rem_username = connHndl->ch_username;
	reqParm->gca_rem_password = connHndl->ch_password;
	reqParm->gca_user_name = NULL;
	reqParm->gca_password = NULL;
    }
    else
    {
	reqParm->gca_user_name = connHndl->ch_username;
	reqParm->gca_password = connHndl->ch_password;
	reqParm->gca_rem_username = NULL;
	reqParm->gca_rem_password = NULL;
    }

    closure->cl_eventNo = IIAPI_EV_CONNECT_CMPL;
    closure->cl_resumeGCAService = GCA_REQUEST;
    closure->cl_resumeGCAFlags = gca_flags;
    closure->cl_apiParm = NULL;
    closure->cl_handle  = handle;
    closure->cl_cmplFunc = reqCmpl;
    
    IIapi_gcaStarted();

    if ( gca_call( &gca_cb, GCA_REQUEST, (GCA_PARMLIST *)reqParm, 
		   gca_flags, (PTR)closure, timeout, &status ) != E_DB_OK )
    {
	IIapi_gcaDone();
	IIapi_liDispatch( IIAPI_EV_SEND_ERROR, handle, 
			  createAbortParm( status ), deleteAbortParm );
	MEfree( (II_PTR)closure );
	return( IIAPI_ST_FAILURE );
    }
    
    return( IIAPI_ST_SUCCESS );
}




/*
** Name: IIapi_disconnGCA
**
** Description:
**	This function invokes a GCA_DISASSOC service.
**
**      handle    handle identifying the current connection
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	27-Oct-94 (gordy)
**	    Accept generic handle as input parameter.
**	20-Jan-95 (gordy)
**	    Flag GCA Active.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	24-Jun-96 (gordy)
**	    Condensed most SENT events into one.  With only a few
**	    exceptions, we don't care what message is sent.  The state
**	    determines what happens once a message is sent, not the
**	    message type.
**	22-Nov-96 (gordy)
**	    Converted to new GCA interface, gca_call().
**	25-Mar-10 (gordy)
**	    GCA parameters moved to closure.
*/

II_EXTERN IIAPI_STATUS
IIapi_disconnGCA( IIAPI_CONNHNDL *connHndl )
{
    GCA_DA_PARMS	*disasscParm;
    IIAPI_CLOSURE	*closure;
    STATUS		status;
    i4			gca_flags = GCA_ASYNC_FLAG;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_disconnGCA: cleaning up GCA connection\n" );
    
    if ( ! ( closure = (IIAPI_CLOSURE *)
	     MEreqmem( 0, sizeof(IIAPI_CLOSURE), FALSE, NULL ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_disconnGCA: can't allocate closure\n" );
	return( IIAPI_ST_OUT_OF_MEMORY );
    }

    /*
    ** Set up GCA_DISASSOC parameters.
    */
    disasscParm = &closure->cl_gcaParm.disc;
#ifdef GCA_API_LEVEL_5
    disasscParm->gca_service = GCA_DISASSOC;
    disasscParm->gca_completion = disasscCmpl;
    disasscParm->gca_closure = (II_PTR)closure;
    gca_flags |= GCA_ALT_EXIT;
#endif
    disasscParm->gca_assoc_id = connHndl->ch_connID;

    closure->cl_eventNo = IIAPI_EV_DISCONN_CMPL;
    closure->cl_resumeGCAService = GCA_DISASSOC;
    closure->cl_resumeGCAFlags = gca_flags;
    closure->cl_apiParm = NULL;
    closure->cl_handle = (IIAPI_HNDL *)connHndl;
    closure->cl_cmplFunc = disasscCmpl;
    
    IIapi_gcaStarted();

    if ( gca_call( &gca_cb, GCA_DISASSOC, (GCA_PARMLIST *)disasscParm,
		   gca_flags, (PTR)closure, -1, &status ) != E_DB_OK )
    {
	/*
	** If GCA_DISASSOC fails, there is little else we can do.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_disconnGCA: GCA_DISASSOC failed\n" );

	IIapi_gcaDone();
	MEfree( (II_PTR)closure );
	return( IIAPI_ST_FAILURE );
    }
    
    return( IIAPI_ST_SUCCESS );
}




/*
** Name: IIapi_abortGCA
**
** Description:
**	This function invokes a synchronous GCA_DISASSOC service.
**
** Input:
**      handle    handle identifying the current connection
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS
**
** History:
**	10-Feb-97 (gordy)
**	    Created.
*/

II_EXTERN IIAPI_STATUS
IIapi_abortGCA( IIAPI_CONNHNDL *connHndl )
{
    GCA_DA_PARMS	disasscParm;
    STATUS		status;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_abortGCA: shutting down GCA connection\n" );
    
    /*
    ** Set up GCA_DISASSOC parameters.
    */
#ifdef GCA_API_LEVEL_5
    disasscParm.gca_service = GCA_DISASSOC;
    disasscParm.gca_completion = NULL;
    disasscParm.gca_closure = NULL;
#endif
    disasscParm.gca_assoc_id = connHndl->ch_connID;
    
    if ( gca_call( &gca_cb, GCA_DISASSOC, (GCA_PARMLIST *)&disasscParm,
		   GCA_SYNC_FLAG, (PTR)NULL, -1, &status ) != E_DB_OK )
    {
	/*
	** If GCA_DISASSOC fails, there is little else we can do.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_abortGCA: GCA_DISASSOC failed\n" );

	return( IIAPI_ST_FAILURE );
    }
    
    return( IIAPI_ST_SUCCESS );
}




/*
** Name: IIapi_sndGCA
**
** Description:
**	This function invokes a GCA_SEND service.
**
**	The message buffer is reserved by this routine.  The
**	buffer is released if an error occurs before this
**	routine returns.  Otherwise, the buffer will be
**	released after the send completes.
**
**	The caller should reserve the buffer if it wishes
**	to preserve the buffer beyond the bounds of this
**	routine.
**
** Input:
**	handle     Handle associated with current connection.
**      msgBuff    Send message buffer.
**	descriptor GCA descriptor for het-net support
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_SUCCESS
**			IIAPI_ST_OUT_OF_MEMORY
**			IIAPI_ST_FAILURE
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	17-Jan-95 (gordy)
**	    Use local typedef to handle changes in GCA_SD_PARMS.
**	20-Jan-95 (gordy)
**	    Flag GCA Active.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	20-Jun-95 (gordy)
**	    Set GCA formatted flag according to buffer parameter setting.
**	28-Jul-95 (gordy)
**	    Use fixed length types.
**	21-Dec-95 (gordy)
**	    Fix memory leak by requiring parmNmem not to be NULL so that
**	    the GCA parameter block can be saved and deleted in deleteSend().
**	17-Oct-96 (gordy)
**	    Dropped event parameter.  All send completions now the same.
**	22-Nov-96 (gordy)
**	    Converted to new GCA interface, gca_call().
**	31-Jan-97 (gordy)
**	    Added het-net descriptor to IIapi_sndGCA().
**	21-Feb-97 (gordy)
**	    Changed gca_formatted to gca_modifiers.
**	25-Mar-10 (gordy)
**	    Removed formatted buffer, use byte stream buffer.  GCA 
**	    parameters moved to closure.  Free parameter memory 
**	    block if error occurs.  Set EOG or not-EOG if requested.
*/

II_EXTERN IIAPI_STATUS
IIapi_sndGCA
( 
    IIAPI_HNDL		*handle, 
    IIAPI_MSG_BUFF	*msgBuff,
    II_PTR		descriptor
)
{
    IIAPI_CONNHNDL	*connHndl;
    GCA_SD_PARMS	*sndParm;
    IIAPI_CLOSURE	*closure;
    IIAPI_STATUS	apiStatus;
    STATUS		gcaStatus;
    i4			gca_flags = GCA_ASYNC_FLAG;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_sndGCA: sending GCA message\n" );
    
    /*
    ** Message buffer is reserved when first called.  It
    ** must be released if an error occurs or after the
    ** send completes.
    */
    IIapi_reserveMsgBuffer( msgBuff );

    if ( ! ( connHndl = IIapi_getConnHndl( handle ) ) )
    {
	/*
	** This is an internal error which (in a perfect world)
	** should not happen.  Fail the operation so an error
	** will be detected.
	*/
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_sndGCA: API ERROR - no connection handle\n" );
	apiStatus = IIAPI_ST_FAILURE;
	goto Fail;
    }

    if ( ! ( closure = (IIAPI_CLOSURE *)
	     MEreqmem( 0, sizeof(IIAPI_CLOSURE), FALSE, NULL ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_sndGCA: can't allocate send closure\n" );
	apiStatus = IIAPI_ST_OUT_OF_MEMORY;
	goto Fail;
    }

    /*
    ** Set up GCA_SEND parameters.
    */
    sndParm = &closure->cl_gcaParm.send;
#ifdef GCA_API_LEVEL_5
    sndParm->gca_service = GCA_SEND;
    sndParm->gca_completion = sndCmpl;
    sndParm->gca_closure = (II_PTR)closure;
    gca_flags |= GCA_ALT_EXIT;
#endif
    sndParm->gca_assoc_id = connHndl->ch_connID;
    sndParm->gca_message_type = msgBuff->msgType;
    sndParm->gca_msg_length = msgBuff->length;
    sndParm->gca_buffer = (char *)msgBuff->data;
    sndParm->gca_descriptor = descriptor;
    sndParm->gca_modifiers = 0;
    sndParm->gca_end_of_data = ((msgBuff->flags & IIAPI_MSG_EOD) != 0);
    if (msgBuff->flags & IIAPI_MSG_EOG)  sndParm->gca_modifiers |= GCA_EOG;
    if (msgBuff->flags & IIAPI_MSG_NEOG) sndParm->gca_modifiers |= GCA_NOT_EOG;

    closure->cl_eventNo = IIAPI_EV_SEND_CMPL;
    closure->cl_resumeGCAService = IIAPI_NO_RESUME;
    closure->cl_resumeGCAFlags = gca_flags;
    closure->cl_apiParm = (II_PTR)msgBuff;
    closure->cl_handle  = handle;
    closure->cl_cmplFunc = sndCmpl;

    IIapi_gcaStarted();

    if ( gca_call( &gca_cb, GCA_SEND, (GCA_PARMLIST *)sndParm, 
		   gca_flags, (PTR)closure, -1, &gcaStatus ) != E_DB_OK )
    {
	IIapi_gcaDone();
	IIapi_liDispatch( IIAPI_EV_SEND_ERROR, handle,
			  createAbortParm( gcaStatus ), deleteAbortParm );
	MEfree( (II_PTR)closure );
	apiStatus = IIAPI_ST_FAILURE;
	goto Fail;
    }
    
    return( IIAPI_ST_SUCCESS );

  Fail:

    /*
    ** Message buffer is reserved before any other
    ** action in this routine.  It must be released
    ** if an error occurs so that it will be freed
    ** if caller has not also reserved it.
    */
    IIapi_releaseMsgBuffer( msgBuff );
    return( apiStatus );
}




/*
** Name: IIapi_rcvNormalGCA
**
** Description:
**	This function invokes a normal GCA_RECEIVE service.
**
**	The message buffer is reserved by this routine.  The
**	buffer is released if an error occurs, either before
**	this routine returns or if the posted receive fails.
**	Otherwise, the buffer will be released after it has
**	been processed by the callback for a successful
**	receive.
**
**	The caller (or the callback routine) should reserve
**	the buffer if it wishes to preserve the buffer 
**	beyond the bounds of this routine.
**		
** Input:
**      handle		Handle associated with current connection.
**	msgBuff		Receive message buffer.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS	IIAPI_ST_SUCCESS
**			IIAPI_ST_OUT_OF_MEMORY
**			IIAPI_ST_FAILURE
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	27-Oct-94 (gordy)
**	    Accept generic handle as input parameter.
**	17-Jan-95 (gordy)
**	    Use local typedef to handle changes in GCA_RV_PARMS.
**	20-Jan-95 (gordy)
**	    Flag GCA Active.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	13-Jun-95 (gordy)
**	    Added IIAPI_GCA_BUFF to support buffering of GCA messages.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**      26-Oct-95 (loera01)
**	    Changed buffer length for reads from sizeAdvise to
**          API_GCA_BUF_SIZE.  This allows VMS compatability when large
**          buffers are read.
**	 8-Mar-96 (gordy)
**	    Added the ability to expand receive buffers if fail 
**	    because buffer is too small.
**	15-Jul-96 (gordy)
**	    Added timeout parameter.
**	22-Nov-96 (gordy)
**	    Converted to new GCA interface, gca_call().
**	21-Feb-97 (gordy)
**	    Changed gca_formatted to gca_modifiers.
**	27-Feb-97 (gordy)
**	    Extracted buffer allocation to IIapi_allocGCABuffer().
**	12-Jun-02 (gordy)
**	    Expand GCA buffer if it is full.
**	25-Mar-10 (gordy)
**	    Combined send/recv buffer structures.  Caller must now
**	    allocate receive buffer.  GCA parameters moved to closure.
*/

II_EXTERN IIAPI_STATUS
IIapi_rcvNormalGCA( IIAPI_HNDL *handle, 
		    IIAPI_MSG_BUFF *msgBuff, II_LONG timeout )
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_CLOSURE	*closure;
    GCA_RV_PARMS	*rcvParm;
    IIAPI_STATUS	apiStatus;
    STATUS		gcaStatus;
    i4			gca_flags = GCA_ASYNC_FLAG;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_rcvNormalGCA: receiving normal GCA message\n" );
    
    /*
    ** Message buffer is reserved when first called.  It
    ** must be released if an error occurs or after a
    ** successful receive and callback has been made to
    ** process the results.
    */
    IIapi_reserveMsgBuffer( msgBuff );

    if ( ! (connHndl = IIapi_getConnHndl( handle )) )
    {
	/*
	** This is an internal error which (in a perfect world)
	** should not happen.  Fail the operation so an error
	** will be detected.
	*/
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_rcvNormalGCA: unable to retrieve connection handle\n" );
	apiStatus = IIAPI_ST_FAILURE;
	goto Fail;
    }

    if ( connHndl->ch_header.hd_smi.smi_state == IIAPI_IDLE )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_rcvNormalGCA: connection is in invalid state\n" );
	apiStatus = IIAPI_ST_FAILURE;
	goto Fail;
    }
    
    if ( IIAPI_MSG_UNUSED( msgBuff ) <= 0 )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_rcvNormalGCA: receive buffer is full\n" );
	apiStatus = IIAPI_ST_FAILURE;
	goto Fail;
    }

    /*
    ** Move any existing data to start of 
    ** buffer to be pre-pended to new data.
    */
    if ( msgBuff->data != msgBuff->buffer )
    {
	if ( msgBuff->length > 0 )
	    MEcopy( msgBuff->data, msgBuff->length, msgBuff->buffer );

	msgBuff->data = msgBuff->buffer;
    }

    if ( ! (closure = (IIAPI_CLOSURE *)MEreqmem( 0, sizeof( IIAPI_CLOSURE ), 
    						 FALSE, NULL )) )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_rcvNormalGCA: error allocating closure\n" );
	apiStatus = IIAPI_ST_OUT_OF_MEMORY;
	goto Fail;
    }

    /*
    ** Set up GCA_RECEIVE parameters
    */
    rcvParm = &closure->cl_gcaParm.recv;
#ifdef GCA_API_LEVEL_5
    rcvParm->gca_service = GCA_RECEIVE;
    rcvParm->gca_completion = rcvNormalCmpl;
    rcvParm->gca_closure = (II_PTR)closure;
    gca_flags |= GCA_ALT_EXIT;
#endif
    rcvParm->gca_assoc_id = connHndl->ch_connID;
    rcvParm->gca_flow_type_indicator = GCA_NORMAL;
    rcvParm->gca_modifiers = 0;
    rcvParm->gca_b_length = msgBuff->size - msgBuff->length;
    rcvParm->gca_buffer = msgBuff->data + msgBuff->length;

    closure->cl_eventNo = 0;	/* unknown */
    closure->cl_resumeGCAService = GCA_RECEIVE;
    closure->cl_resumeGCAFlags = gca_flags;
    closure->cl_apiParm = (II_PTR)msgBuff;
    closure->cl_handle  = handle;
    closure->cl_cmplFunc = rcvNormalCmpl;
    
    IIapi_gcaStarted();

    if ( gca_call( &gca_cb, GCA_RECEIVE, (GCA_PARMLIST *)rcvParm, 
		   gca_flags, (PTR)closure, timeout, &gcaStatus ) != E_DB_OK )
    {
	IIapi_gcaDone();
	IIapi_liDispatch( IIAPI_EV_RECV_ERROR, handle,
			  createAbortParm( gcaStatus ), deleteAbortParm );
	MEfree( (II_PTR)closure );
	apiStatus = IIAPI_ST_FAILURE;
	goto Fail;
    }
    
    return( IIAPI_ST_SUCCESS );
    
  Fail:

    /*
    ** Message buffer is reserved before any other
    ** action in this routine.  It must be released
    ** if an error occurs so that it will be freed
    ** if caller has not also reserved it.
    */
    IIapi_releaseMsgBuffer( msgBuff );
    return( apiStatus );
}


/*
** Name: IIapi_rcvExpediteGCA
**
** Description:
**	This function invokes an expedited GCA_RECEIVE service.
**	The expedited receive will automatically be reposted
**	upon completion.
**
**	The message buffer is reserved by this routine.  The
**	buffer is released if an error occurs, either before
**	this routine returns or if the posted receive fails.
**	Otherwise, the buffer will be released after it has
**	been processed by the callback for a successful
**	receive.
**
**	The caller (or the callback routine) should reserve
**	also the buffer if it wishes to preserve the buffer 
**	beyond the bounds of this routine.
**
**
** Input:
**      handle		Handle associated with current connection.
**	msgBuff		Receive message buffer.
**
** Output:
**	None.
**
** Returns:
**	VOID.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	27-Oct-94 (gordy)
**	    Accept generic handle as input parameter.
**	17-Jan-95 (gordy)
**	    Use local typedef to handle changes in GCA_RV_PARMS.
**	20-Jan-95 (gordy)
**	    Flag GCA Active.
**	13-Jun-95 (gordy)
**	    Added IIAPI_GCA_BUFF to support buffering of GCA messages.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	26-Apr-96 (gordy)
**	    Expedited channel not tracked by GCA activity mechanism.
**	22-Nov-96 (gordy)
**	    Converted to new GCA interface, gca_call().
**	21-Feb-97 (gordy)
**	    Changed gca_formatted to gca_modifiers.
**	27-Feb-97 (gordy)
**	    Extracted buffer allocation to IIapi_allocGCABuff().
**	25-Mar-10 (gordy)
**	    Combined send/recv buffer structures.  Caller must now
**	    allocate receive buffer.  GCA parameters moved to closure.
*/

II_EXTERN II_VOID
IIapi_rcvExpediteGCA( IIAPI_HNDL *handle, IIAPI_MSG_BUFF *msgBuff )
{
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_CLOSURE	*closure;
    GCA_RV_PARMS	*rcvParm;
    STATUS		gcaStatus;
    i4			gca_flags = GCA_ASYNC_FLAG;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_rcvExpediteGCA: receiving expedited GCA message\n" );
    
    /*
    ** Message buffer is reserved when first called.  It
    ** must be released if an error occurs or after a
    ** successful receive and callback has been made to
    ** process the results.
    */
    IIapi_reserveMsgBuffer( msgBuff );

    if ( ! ( connHndl = IIapi_getConnHndl( handle ) ) )
    {
	IIAPI_TRACE( IIAPI_TR_FATAL )
	    ( "IIapi_rcvExpediteGCA: no connection handle\n" );
	goto Fail;
    }

    if ( connHndl->ch_header.hd_smi.smi_state == IIAPI_IDLE )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ("IIapi_rcvExpediteGCA: connection is in invalid state\n");
	goto Fail;
    }
    
    if ( (closure = (IIAPI_CLOSURE *)MEreqmem( 0, sizeof( IIAPI_CLOSURE ), 
					       FALSE, NULL )) == NULL )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_rcvExpediteGCA: error allocating closure\n" );
	goto Fail;
    }

    /*
    ** Set up GCA_RECEIVE parameters
    */
    rcvParm = &closure->cl_gcaParm.recv;
#ifdef GCA_API_LEVEL_5
    rcvParm->gca_service = GCA_RECEIVE;
    rcvParm->gca_completion = rcvExpediteCmpl;
    rcvParm->gca_closure = (II_PTR)closure;
    gca_flags |= GCA_ALT_EXIT;
#endif
    rcvParm->gca_assoc_id = connHndl->ch_connID;
    rcvParm->gca_flow_type_indicator = GCA_EXPEDITED;
    rcvParm->gca_modifiers = 0;
    rcvParm->gca_descriptor = NULL;
    rcvParm->gca_buffer = msgBuff->data + msgBuff->length;
    rcvParm->gca_b_length = msgBuff->size - msgBuff->length;
    closure->cl_eventNo = 0;	/* unknown */
    closure->cl_resumeGCAService = IIAPI_NO_RESUME;
    closure->cl_resumeGCAFlags = gca_flags;
    closure->cl_apiParm = (II_PTR)msgBuff;
    closure->cl_handle = handle;
    closure->cl_cmplFunc = rcvExpediteCmpl;
    
    if ( gca_call( &gca_cb, GCA_RECEIVE, (GCA_PARMLIST *)rcvParm,
		   gca_flags, (PTR)closure, -1, &gcaStatus ) != E_DB_OK )
    {
	IIapi_liDispatch( IIAPI_EV_RECV_ERROR, handle,
			  createAbortParm( gcaStatus ), deleteAbortParm );
	MEfree( (II_PTR)closure );
	goto Fail;
    }
    
    return;
    
  Fail:

    /*
    ** Message buffer is reserved before any other
    ** action in this routine.  It must be released
    ** if an error occurs so that it will be freed
    ** if caller has not also reserved it.
    */
    IIapi_releaseMsgBuffer( msgBuff );
    return;
}


/*
** Name: IIapi_resumeGCA
**
** Description:
**	Call GCA with the request indicated by the closure.  This
**	function is usually used to resume and incomplete function.
**	It can also be used to issue a new GCA request, but this is
**	currently only done for the case when GCA_RECEIVE fails due
**	to insufficient buffer space and needs to be redone with a
**	larger buffer.
**
** Input:
**	handle		Handle identifying the current connection
**	closure		Closure for the GCA request.
**
** Output:
**	None.
**
** Returns:
**	IIAPI_STATUS		
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 8-Mar-96 (gordy)
**	    Added the ability to expand receive buffers if fail 
**	    because buffer is too small.
**	26-Apr-96 (gordy)
**	    Expedited channel not tracked by GCA activity mechanism.
**	18-Oct-96 (gordy)
**	    Now a global function.
**	22-Nov-96 (gordy)
**	    Converted to new GCA interface, gca_call().
**	29-Oct-97 (gordy)
**	    Do not reset GCA started flag during resume.
**	12-Jul-02 (gordy)
**	    Fix test for RESUME flag.
**	25-Mar-10 (gordy)
**	    GCA parameters moved to closure.
*/

II_EXTERN IIAPI_STATUS
IIapi_resumeGCA( IIAPI_HNDL *handle, II_PTR closure )
{
    IIAPI_CLOSURE	*apiClosure = (IIAPI_CLOSURE *)closure;
    STATUS		status;
    
    if ( apiClosure->cl_resumeGCAService == IIAPI_NO_RESUME )
    {
	IIAPI_TRACE( IIAPI_TR_ERROR )
	    ( "IIapi_resumeGCA: GCA service can not be resumed!\n" );
	IIapi_gcaDone();
	return( IIAPI_ST_FAILURE );
    }

    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "IIapi_resumeGCA: resume GCA service\n" );
    
    /*
    ** If the resume flag is not set then we are actually
    ** starting a new GCA request in place of a failed
    ** request, so we need to set the GCA active state.
    */
    if ( ! (apiClosure->cl_resumeGCAFlags & GCA_RESUME) )
	IIapi_gcaStarted();

    if ( gca_call( &gca_cb, apiClosure->cl_resumeGCAService,
		   (GCA_PARMLIST *)&apiClosure->cl_gcaParm,
		   apiClosure->cl_resumeGCAFlags,
		   (PTR)apiClosure, -1, &status ) != E_DB_OK )
    {
	IIapi_gcaDone();
	IIapi_liDispatch( (apiClosure->cl_resumeGCAService == GCA_RECEIVE)
			  ? IIAPI_EV_RECV_ERROR: IIAPI_EV_SEND_ERROR,
			  handle, createAbortParm( status ), deleteAbortParm );
	return( IIAPI_ST_FAILURE );
    }

    return( IIAPI_ST_SUCCESS );
}




/*
** Name: reqCmpl
**
** Description:
**	This function is invoked when GCA_REQUEST is issued by
**     GCA.  It is a means to notify the completion of this
**     GCA service.
**
**     closure  The closure returned by GCA which was originally
**              created by IIapi_connGCA().
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	18-Nov-94 (gordy)
**	    Set the API level for the GCA protocol in effect.
**	13-Dec-94 (gordy)
**	    Return sizeAdvise to client.
**	17-Jan-95 (gordy)
**	    Fix request parameter name for older (and new) GCA interface.
**	20-Jan-95 (gordy)
**	    Flag GCA inactive.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	24-Jun-96 (gordy)
**	    Condensed most SENT events into one.  With only a few
**	    exceptions, we don't care what message is sent.  The state
**	    determines what happens once a message is sent, not the
**	    message type.
**	31-Jan-97 (gordy)
**	    Added descriptor flag to connection handle.
**	29-Oct-97 (gordy)
**	    Do not reset GCA started flag during resume.
**	 1-Mar-01 (gordy)
**	    Check for GCA Unicode protocol level to set IIAPI_LEVEL_2.
**	15-Mar-04 (gordy)
**	    Add 8 byte int support: new API version & level, GCA protocol.
**	 6-Oct-04 (gordy)
**	    Make sure sizeAdvise is at least large enough for a tuple.
**	31-May-06 (gordy)
**	    New API version & level and GCA protocol.
**	25-Oct-06 (gordy)
**	    Support Blob/Clob locators: new API version & level, GCA protocol.
**	26-Oct-09 (gordy)
**	    New API version 7, level 6 and GCA protcol 68.
**	25-Mar-10 (gordy)
**	    GCA parameters moved to closure.
*/

II_EXTERN II_VOID II_CALLBACK
reqCmpl( II_PTR closure )
{
    IIAPI_CLOSURE	*apiClosure = (IIAPI_CLOSURE *)closure;
    GCA_RQ_PARMS	*reqParm = &apiClosure->cl_gcaParm.rqst;
    IIAPI_CONNHNDL	*connHndl;
    IIAPI_CONNPARM	*connParm;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "reqCmpl: GCA_REQUEST completes\n" );
    
    connHndl = IIapi_getConnHndl( apiClosure->cl_handle );
    connParm = (IIAPI_CONNPARM *)connHndl->ch_parm;

    if ( reqParm->gca_status == E_GCFFFE_INCOMPLETE )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )( "reqCmpl: reinvoke GCA_REQUEST\n" );

	apiClosure->cl_resumeGCAFlags |= GCA_RESUME;
	IIapi_liDispatch( IIAPI_EV_RESUME, apiClosure->cl_handle, 
			  (II_PTR)apiClosure, NULL );

	return;
    }
    
    /*
    ** We need to save the association ID now.
    ** Even if there is an error, we need to
    ** call GCA_DISASSOC on the failed ID to
    ** free GCA resources.
    */
    connHndl->ch_connID = reqParm->gca_assoc_id;
    IIapi_gcaDone();

    if ( reqParm->gca_status != E_GC0000_OK  ||  reqParm->gca_status != OK )
    {
	IIapi_liDispatch( IIAPI_EV_SEND_ERROR, apiClosure->cl_handle,
			  createAbortParm( reqParm->gca_status ),
			  deleteAbortParm );
	MEfree( (II_PTR)apiClosure );
	return;
    }
    
    /*
    ** Save the GCA info relevent to our connection.
    */
    connHndl->ch_partnerProtocol = reqParm->gca_peer_protocol;
    connHndl->ch_sizeAdvise = max( reqParm->gca_size_advise, DB_MAXTUP );
    connParm->co_sizeAdvise = connHndl->ch_sizeAdvise;

    if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_68 )
	connParm->co_apiLevel = IIAPI_LEVEL_6;
    else  if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_67 )
	connParm->co_apiLevel = IIAPI_LEVEL_5;
    else  if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_66 )
	connParm->co_apiLevel = IIAPI_LEVEL_4;
    else  if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_65 )
	connParm->co_apiLevel = IIAPI_LEVEL_3;
    else  if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_64 )
	connParm->co_apiLevel = IIAPI_LEVEL_2;
    else  if ( connHndl->ch_partnerProtocol >= GCA_PROTOCOL_LEVEL_60 )
	connParm->co_apiLevel = IIAPI_LEVEL_1;
    else
	connParm->co_apiLevel = IIAPI_LEVEL_0;

    if ( reqParm->gca_flags & GCA_DESCR_RQD )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )( "reqCmpl: Het-Net connection\n" );

	connHndl->ch_flags |= IIAPI_CH_DESCR_REQ;
    }

    IIapi_liDispatch( apiClosure->cl_eventNo, 
		      apiClosure->cl_handle, NULL, NULL );
    
    MEfree( (II_PTR)apiClosure );

    return;
}




/*
** Name: disasscCmpl
**
** Description:
**	This function is invoked when GCA_DISASSOC is issued by
**     GCA.  It is a means to notify the completion of this
**     GCA service.
**
**     closure  The closure returned by GCA which was originally
**              created by IIapi_disconnGCA().
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	27-Oct-94 (gordy)
**	    Accept generic handle as input parameter.
**	20-Jan-95 (gordy)
**	    Flag GCA inactive.
**	29-Oct-97 (gordy)
**	    Do not reset GCA started flag during resume.
**	25-Mar-10 (gordy)
**	    GCA parameters moved to closure.
*/

II_EXTERN II_VOID II_CALLBACK
disasscCmpl( II_PTR closure )
{
    IIAPI_CLOSURE	*apiClosure = (IIAPI_CLOSURE *)closure;
    GCA_DA_PARMS	*disasscParm = &apiClosure->cl_gcaParm.disc;
    IIAPI_CONNHNDL	*connHndl;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "disasscCmpl: GCA_DISASSOC completes\n" );
    
    connHndl = IIapi_getConnHndl( apiClosure->cl_handle );

    if ( disasscParm->gca_status == E_GCFFFE_INCOMPLETE )
    {
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "disasscCmpl: need to reinvoke GCA_DISASSOC\n" );

	apiClosure->cl_resumeGCAFlags |= GCA_RESUME;
	IIapi_liDispatch( IIAPI_EV_RESUME, apiClosure->cl_handle, 
			  (II_PTR)apiClosure, NULL );
	return;
    }
    
    connHndl->ch_connID = 0;
    IIapi_gcaDone();

    IIapi_liDispatch( apiClosure->cl_eventNo, 
		      apiClosure->cl_handle, NULL, NULL );
    
    MEfree( (II_PTR)closure );

    return;
}




/*
** Name: sndCmpl
**
** Description:
**	This function is invoked as a callback to GCA_SEND.
**
**	Frees the async closure allocated by IIapi_sndGCA().
**	The message buffer is released.
**
** Input:
**	closure		Async closure created by IIapi_sndGCA().
**
** Output:
**	None.
**
** Returns:
**      VOID
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	17-Jan-95 (gordy)
**	    Use local typedef to handle changes in GCA_SD_PARMS.
**	20-Jan-95 (gordy)
**	    Flag GCA inactive.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	25-Mar-10 (gordy)
**	    GCA parameters moved to closure.  Release message buffer.
*/

II_EXTERN II_VOID II_CALLBACK
sndCmpl( II_PTR closure )
{
    IIAPI_CLOSURE	*apiClosure = (IIAPI_CLOSURE *)closure;
    IIAPI_MSG_BUFF	*msgBuff = (IIAPI_MSG_BUFF *)apiClosure->cl_apiParm;
    GCA_SD_PARMS	*sndParm = &apiClosure->cl_gcaParm.send;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "sndCmpl: GCA_SEND completes\n" );
    
    IIapi_gcaDone();
    IIapi_releaseMsgBuffer( msgBuff );

    if ( sndParm->gca_status != E_GC0000_OK  ||  sndParm->gca_status != OK )
    {
	IIapi_liDispatch( IIAPI_EV_SEND_ERROR, apiClosure->cl_handle,
			  createAbortParm( sndParm->gca_status ),
			  deleteAbortParm );
	MEfree( (II_PTR)apiClosure );
	return;
    }
    
    IIapi_liDispatch( apiClosure->cl_eventNo, 
		      apiClosure->cl_handle, NULL, NULL );
    
    MEfree( (II_PTR)apiClosure );
    return;
}




/*
** Name: rcvNormalCmpl
**
** Description:
**	This function is invoked as a callback to GCA_RECEIVE.
**
**	Frees the async closure allocated by IIapi_rcvNormalGCA().
**	The message buffer is released if the receive fails.  Other-
**	wise, the message buffer will be released once the receive
**	has been dispatched.
**	
** Input:
**	closure		Async closure created by IIapi_rcvNormalGCA().
**
** Output:
**	None.
**
** Return:
**	VOID.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 9-Dec-94 (gordy)
**	    Verify GCA message type.
**	17-Jan-95 (gordy)
**	    Use local typedef to handle changes in GCA_RV_PARMS.
**	20-Jan-95 (gordy)
**	    Flag GCA inactive.
**	25-Apr-95 (gordy)
**	    Removed GCA_EVENT code which is now handled by state machines.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	 8-Mar-96 (gordy)
**	    Added the ability to expand receive buffers if fail 
**	    because buffer is too small.
**	30-Apr-97 (gordy)
**	    Repost purged receive requests.
**	12-Jul-02 (gordy)
**	    Extract buffer expansion code to IIapi_expandGCABuffer().
**	25-Mar-10 (gordy)
**	    Replaced formatted GCA interface with byte stream.  GCA 
**	    parameters moved to closure.  Combined send/recv buffer 
**	    structures.
*/

II_EXTERN II_VOID II_CALLBACK
rcvNormalCmpl( II_PTR closure )
{
    IIAPI_CLOSURE	*apiClosure = (IIAPI_CLOSURE *)closure;
    IIAPI_MSG_BUFF	*msgBuff = (IIAPI_MSG_BUFF *)apiClosure->cl_apiParm;
    GCA_RV_PARMS	*rcvParm = &apiClosure->cl_gcaParm.recv;
    IIAPI_EVENT		eventNo;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )("rcvNormalCmpl: GCA_RECEIVE completes\n");
    
    IIapi_gcaDone();

    if ( rcvParm->gca_status != E_GC0000_OK  ||  rcvParm->gca_status != OK )
    {
	if ( rcvParm->gca_status == E_GC0027_RQST_PURGED )
	{
	    /*
	    ** Repost the receive by hijacking the RESUME mechanism.
	    ** Note that the GCA_RESUME flag is not set.  The closure
	    ** is reused as the event parameter.  The message buffer 
	    ** is not released since it is still in use during the 
	    ** repost.
	    */
	    IIapi_liDispatch( IIAPI_EV_RESUME, apiClosure->cl_handle, 
			      (II_PTR)apiClosure, NULL );
	}
	else
	{
	    IIapi_liDispatch( IIAPI_EV_RECV_ERROR, apiClosure->cl_handle,
			      createAbortParm( rcvParm->gca_status ),
			      deleteAbortParm );
	    MEfree( (II_PTR)closure );
	    IIapi_releaseMsgBuffer( msgBuff );
	}

	return;
    }
    
    /*
    ** Update the received data length and message attributes.
    */
    msgBuff->msgType = rcvParm->gca_message_type;
    msgBuff->length += rcvParm->gca_d_length;
    msgBuff->flags = 0;

    if ( rcvParm->gca_end_of_data )  msgBuff->flags |= IIAPI_MSG_EOD;
    if ( rcvParm->gca_end_of_group ) msgBuff->flags |= IIAPI_MSG_EOG;

    if ( rcvParm->gca_message_type >= 0  && 
	 rcvParm->gca_message_type < (sizeof(eventNoTBL)/sizeof(IIAPI_EVENT)) )
	eventNo = eventNoTBL[ rcvParm->gca_message_type ];
    else
	eventNo = IIAPI_EV_UNEXPECTED_RCVD;

    /*
    ** Message buffer is released after the event
    ** has been fully dispatched (which may occur
    ** after this routine completes).
    */
    IIapi_liDispatch( eventNo, apiClosure->cl_handle, 
		      apiClosure->cl_apiParm,
		      (IIAPI_DELPARMFUNC)IIapi_releaseMsgBuffer );
    
    MEfree( (II_PTR)closure );
    return;
}


/*
** Name: rcvExpediteCmpl
**
** Description:
**	This function is invoked as a callback to GCA_RECEIVE.
**	The expedited receive is automatically reposted.
**
**	Frees the async closure allocated by IIapi_rcvExpediteGCA().
**	The message buffer is released if the receive fails.  Other-
**	wise, the message buffer will be released once the receive
**	has been dispatched.
**
** Input:
**	closure		Async closure created by IIapi_rcvExpediteGCA().
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	 9-Dec-94 (gordy)
**	    Verify GCA message type.
**	17-Jan-95 (gordy)
**	    Use local typedef to handle changes in GCA_RV_PARMS.
**	20-Jan-95 (gordy)
**	    Flag GCA inactive.
**	28-Jul-95 (gordy)
**	    Fixed tracing of pointers.
**	26-Apr-96 (gordy)
**	    Expedited channel not tracked by GCA activity mechanism.
**	25-Mar-10 (gordy)
**	    GCA parameters moved to closure.  Combined send/recv 
**	    buffer structures.
*/

II_EXTERN II_VOID II_CALLBACK
rcvExpediteCmpl( II_PTR closure )
{
    IIAPI_CLOSURE	*apiClosure = (IIAPI_CLOSURE *)closure;
    IIAPI_MSG_BUFF	*msgBuff = (IIAPI_MSG_BUFF *)apiClosure->cl_apiParm;
    GCA_RV_PARMS	*rcvParm = &apiClosure->cl_gcaParm.recv;
    IIAPI_EVENT		eventNo;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )
	( "rcvExpediteCmpl: GCA_RECEIVE completes\n" );
    
    if ( rcvParm->gca_status == E_GC0023_ASSOC_RLSED )
    {
	/*
	** When a GCA connection is shutdown, any pending
	** receives complete with this error.  We expect
	** this to happen on the expedited channel during
	** normal shutdown, so don't create a RCVABRT event
	** or repost the receive.  We be done!.
	*/
	IIAPI_TRACE( IIAPI_TR_DETAIL )
	    ( "rcvExpediteCmpl: expedited channel shutdown\n" );

	MEfree( (II_PTR)closure );
	IIapi_releaseMsgBuffer( msgBuff );
	return;
    }

    if ( rcvParm->gca_status != E_GC0000_OK  ||  rcvParm->gca_status != OK )
    {
	IIapi_liDispatch( IIAPI_EV_RECV_ERROR, apiClosure->cl_handle,
			  createAbortParm( rcvParm->gca_status ),
			  deleteAbortParm );
	MEfree( (II_PTR)closure );
	IIapi_releaseMsgBuffer( msgBuff );
	return;
    }
    
    /*
    ** Update the received data length and message attributes.
    */
    msgBuff->msgType = rcvParm->gca_message_type;
    msgBuff->length += rcvParm->gca_d_length;
    msgBuff->flags = 0;

    if ( rcvParm->gca_end_of_data )  msgBuff->flags |= IIAPI_MSG_EOD;
    if ( rcvParm->gca_end_of_group ) msgBuff->flags |= IIAPI_MSG_EOG;

    if ( rcvParm->gca_message_type >= 0  &&
	 rcvParm->gca_message_type < (sizeof(eventNoTBL)/sizeof(IIAPI_EVENT)) )
	eventNo = eventNoTBL[ rcvParm->gca_message_type ];
    else
	eventNo = IIAPI_EV_UNEXPECTED_RCVD;

    /*
    ** Expedited receives are automatically reposted.
    */
    if ( (msgBuff = IIapi_allocMsgBuffer( apiClosure->cl_handle )) )
	IIapi_rcvExpediteGCA( apiClosure->cl_handle, msgBuff );

    /*
    ** Message buffer is released after the event
    ** has been fully dispatched (which may occur
    ** after this routine completes).
    */
    IIapi_liDispatch( eventNo, apiClosure->cl_handle, apiClosure->cl_apiParm, 
		      (IIAPI_DELPARMFUNC)IIapi_releaseMsgBuffer );
    
    MEfree( (II_PTR)closure );
    return;
}




/*
** Name: genericCmpl
**
** Description:
**	This function is invoked whenever a GCA service
**     completes.
**
**     asyncID  The closure returned by GCA which was originally
**              created by one of the GCA service invocation
**              functions above.
**
** Return value:
**      none.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**		27-Jan-1997 (merja01)
**			Change definition of asyncID from II_LONG to II_PTR.
**			The II_LONG is only 4 bytes and causes 64 bit addresses
**			to get truncated.
*/

II_EXTERN II_VOID II_CALLBACK
genericCmpl( II_PTR asyncID )
{
    IIAPI_CLOSURE *apiClosure = (IIAPI_CLOSURE *)asyncID;
    
    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "genericCmpl: GCA service completes\n" );
    
    (* apiClosure->cl_cmplFunc)( (II_PTR)asyncID );
    return;
}




/*
** Name: IIapi_gcaStarted
**
** Description:
**	Flag GCA as active.
**
** History:
**	20-Jan-95 (gordy)
**	    Created.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
*/

static II_VOID
IIapi_gcaStarted( II_VOID )
{
    IIAPI_THREAD	*thread;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "API starting GCA request\n" );
    if ( (thread = IIapi_thread()) )  thread->api_gca_active++;

    return;
}
    
/*
** Name: IIapi_gcaDone
**
** Description:
**	Flag GCA as inactive.
**
** History:
**	20-Jan-95 (gordy)
**	    Created.
**	14-Nov-96 (gordy)
**	    Moved serialization control data to thread-local-storage.
*/

static II_VOID
IIapi_gcaDone( II_VOID )
{
    IIAPI_THREAD	*thread;

    IIAPI_TRACE( IIAPI_TR_VERBOSE )( "API finished GCA request\n" );
    if ( (thread = IIapi_thread()) )  thread->api_gca_active--;

    return;
}
    

/*
** Name: createAbortParm
**
** Description:
**	This function creates the parameter for SEND or RECEIVE abort
**     event.
**     
**     nativeError  the native error causing the abort.
**
** Return value:
**     parmBlock    pointer to the parameter block 
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      15-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
*/

static II_PTR
createAbortParm( STATUS gcaError )
{
    STATUS	*gcaErrorPtr;

    if (( gcaErrorPtr = (STATUS *)MEreqmem(0, sizeof(STATUS), FALSE, NULL ) ))
	*gcaErrorPtr = gcaError;

    return( (II_PTR)gcaErrorPtr );
}

static II_VOID
deleteAbortParm( II_PTR parmBlock )
{
    if ( parmBlock )  MEfree( parmBlock );
    return;
}

