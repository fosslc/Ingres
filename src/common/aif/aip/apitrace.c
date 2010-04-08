/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <st.h>
# include <nm.h>
# include <cv.h>

# include <iicommon.h>

# include <iiapi.h>
# include <api.h>
# include <apitrace.h>

/*
** Name: apitrace.c
**
** Description:
**	This file defines functions to make trace messages more
**      user-friendly.
**
**	IIapi_initTrace		Initialize tracing.
**	IIapi_termTrace		Shutdown tracing.
**	IIapi_printEvent	Convert event number to ASCII string.
**	IIapi_printStatus	Convert status to ASCII string.
**	IIapi_printID		Convert number to ASCII string through a table.
**	IIapi_tranID2Str	Convert transaction ID to ASCII string.
**	IIapi_nopDisplay	Do nothing when tracing disabled.
**
** History:
**      30-sep-93 (ctham)
**          creation
**      13-apr-94 (ctham)
**          add the missing states IIAPI_CWSM and IIAPI_SWRM.
**	21-Oct-94 (gordy)
**	    Cleaned up savepoint processing.  Check array bounds.
**	31-Oct-94 (gordy)
**	    Cleaned up cursor processing.
**	 7-Nov-94 (gordy)
**	    Fixing support for readonly cursors.
**	 9-Nov-94 (gordy)
**	    Cleaned up initial Query states.
**	10-Nov-94 (gordy)
**	    Adding support for buffering of SELECT tuples.
**	14-Nov-94 (gordy)
**	    Converting catalog update functions to a more
**	    generic and extensible form.
**	 8-Dec-94 (gordy)
**	    Added state for start of transaction prior to
**	    confirmation by server that a transaction is active.
**	10-Dec-94 (gordy)
**	    Combined QUERY3 event with QUERY2.
**	23-Jan-95 (gordy)
**	    Added IIapi_printStatus().
**	 9-Feb-95 (gordy)
**	    Made trace levels ordered rather than bit masks.
**	 3-Mar-95 (gordy)
**	    Cleanup shutting down the trace file.
**	31-Mar-95 (gordy)
**	    TR documentation refers to the TR_T_ options as trace
**	    files while the code refers to terminals.  Switch to
**	    use TR_F_ which are for files.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	19-May-95 (gordy)
**	    Fixed include statements.
**	13-Jun-95 (gordy)
**	    Add cursor state for partial fetches: SRPD.
**	14-Jun-95 (gordy)
**	    Transaction name not significant in Ingres transaction IDs.
**	20-Jun-95 (gordy)
**	    Expanded QUERY_SENT events.
**	28-Jul-95 (gordy)
**	    Use fixed length types.
**	17-Jan-96 (gordy)
**	    Added environment handles.  Moved trace info to global 
**	    data structure.
**	23-Feb-96 (gordy)
**	    Converted copy tuple processing to use existing tuple processing.
**	    Removed IIAPI_EV_CDATA_RCVD, IIAPI_SWCD, IIAPI_SRCD.
**	 8-Mar-96 (gordy)
**	    Added events to handle processing of split messages.
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
**	24-Jun-96 (gordy)
**	    Condensed most SENT events into one.  With only a few
**	    exceptions, we don't care what message is sent.  The state
**	    determines what happens once a message is sent, not the
**	    message type.
**	 9-Jul-96 (gordy)
**	    Added support for autocommit transactions.
**	12-Jul-96 (gordy)
**	    Enhanced event support with IIapi_getEvent().
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	11-Feb-97 (gordy)
**	    Added init count to log name.
**	06-Oct-98 (rajus01)
**	    Added IIAPI_EV_PROMPT_RCVD.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	 6-Jun-03 (gordy)
**	    Use PM to check for API trace settings when used in servers.
**	25-Mar-10 (gordy)
**	    Added batch processing events.
*/

#define	CHAR_ARRAY_SIZE( array )	(sizeof(array)/sizeof(array[0]))

/*
** Local static data.
*/
static char	*errStr = "*** Invalid Value***";
static i4	init_count = 0;




/*
** Name: IIapi_initTrace
**
** Description:
**     This function initializes the tracing capability in API.
**
** Return value:
**     none
**
** Re-eneventcy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	 7-Nov-94 (gordy)
**	    Only set output log file if II_API_LOG is defined.
**	 3-Mar-95 (gordy)
**	    Save trace file name for IIapi_termTrace().
**	19-Jan-96 (gordy)
**	    Moved trace info to global data structure.
**	11-Feb-97 (gordy)
**	    Added init count to log name so that multiple inits
**	    will go to separate logs rather than the last over-
**	    writing all preceding.
**	 6-Jun-03 (gordy)
**	    Use PM to check for API trace settings when used in servers.
*/

II_EXTERN II_VOID
IIapi_initTrace( II_VOID )
{
    char	*retVal;
    STATUS	status;
    CL_ERR_DESC	err_code;
    
    if ( ! IIapi_static )  return;

    NMgtAt( "II_API_TRACE", &retVal );
    if ( ! (retVal && *retVal)  && 
	 PMget( "!.api_trace_level", &retVal ) != OK )  retVal = NULL;

    if ( retVal  &&  *retVal ) CVal( retVal, &IIapi_static->api_trace_level );

    NMgtAt( "II_API_LOG", &retVal );
    if ( ! (retVal && *retVal)  && 
	 PMget( "!.api_trace_log", &retVal ) != OK )  retVal = NULL;

    if ( retVal  &&  *retVal  &&  
	 (IIapi_static->api_trace_file = (char *)
		     MEreqmem( 0, STlength(retVal) + 16, FALSE, &status )) )
    {
	STprintf( IIapi_static->api_trace_file, retVal, init_count++ );
	TRset_file( TR_F_OPEN, IIapi_static->api_trace_file, 
		    STlength( IIapi_static->api_trace_file ), &err_code );
    }
    
    return;
}




/*
** Name: IIapi_termTrace
**
** Description:
**     This function shuts down the tracing capability in API.
**
** Return value:
**     none
**
** Re-eneventcy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	 3-Mar-93 (gordy)
**	    Close the actual trace file.
**	19-Jan-96 (gordy)
**	    Moved trace info to global data structure.
*/

II_EXTERN II_VOID
IIapi_termTrace( II_VOID )
{
    CL_ERR_DESC	    err_code;
    
    if ( IIapi_static )
    {
	if ( IIapi_static->api_trace_file )
	    TRset_file( TR_F_CLOSE, IIapi_static->api_trace_file, 
			STlength( IIapi_static->api_trace_file ), &err_code );

	IIapi_static->api_trace_level = 0;
	IIapi_static->api_trace_file = NULL;
    }

    return;
}




/*
** Name: IIapi_printEvent
**
** Description:
**     This function converts API evnt name to ASCII string.
**
**     eventNo  event number
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	21-Oct-94 (gordy)
**	    Cleaned up savepoint processing.  Check array bounds.
**	31-Oct-94 (gordy)
**	    Cleaned up cursor processing.
**	 9-Nov-94 (gordy)
**	    Cleaned up initial Query states.
**	10-Nov-94 (gordy)
**	    Adding support for buffering of SELECT tuples.
**	14-Nov-94 (gordy)
**	    Converting catalog update functions to a more
**	    generic and extensible form.
**	 9-Nov-94 (gordy)
**	    Added events for GCA_TRACE and unexpected messages.
**	10-Dec-95 (gordy)
**	    Combined QUERY3 event with QUERY2.
**	25-Apr-95 (gordy)
**	    Cleaned up Database Events.
**	20-Jun-95 (gordy)
**	    Expanded QUERY_SENT events.
**	23-Feb-96 (gordy)
**	    Converted copy tuple processing to use existing tuple processing.
**	    Removed IIAPI_EV_CDATA_RCVD.
**	 8-Mar-96 (gordy)
**	    Added events to handle processing of split messages.
**	22-Apr-96 (gordy)
**	    Finished support for COPY FROM.
**	24-Jun-96 (gordy)
**	    Condensed most SENT events into one.  With only a few
**	    exceptions, we don't care what message is sent.  The state
**	    determines what happens once a message is sent, not the
**	    message type.
**	 9-Jul-96 (gordy)
**	    Added support for autocommit transactions.
**	12-Jul-96 (gordy)
**	    Enhanced event support with IIapi_getEvent().
**	13-Mar-97 (gordy)
**	    Added event for Name Server result messages.
**	17-Aug-98 (rajus01)
**	    Added event IIAPI_EV_ABORT_FUNC.
**	15-Mar-07 (gordy)
**	    Added position and scroll functions.
**	25-Mar-10 (gordy)
**	    Added batch processing events.
*/

static char *
eventStr[] =
{

    "IIAPI_EV_AUTO_FUNC",
    "IIAPI_EV_BATCH_FUNC",
    "IIAPI_EV_CANCEL_FUNC",
    "IIAPI_EV_CATCHEVENT_FUNC",
    "IIAPI_EV_CLOSE_FUNC",
    "IIAPI_EV_COMMIT_FUNC",
    "IIAPI_EV_CONNECT_FUNC",
    "IIAPI_EV_DISCONN_FUNC",
    "IIAPI_EV_GETCOLUMN_FUNC",
    "IIAPI_EV_GETCOPYMAP_FUNC",
    "IIAPI_EV_GETDESCR_FUNC",
    "IIAPI_EV_GETEVENT_FUNC",
    "IIAPI_EV_GETQINFO_FUNC",
    "IIAPI_EV_MODCONN_FUNC",
    "IIAPI_EV_POSITION_FUNC",
    "IIAPI_EV_PRECOMMIT_FUNC",
    "IIAPI_EV_PUTCOLUMN_FUNC",
    "IIAPI_EV_PUTPARM_FUNC",
    "IIAPI_EV_QUERY_FUNC",
    "IIAPI_EV_ROLLBACK_FUNC",
    "IIAPI_EV_SAVEPOINT_FUNC",
    "IIAPI_EV_SCROLL_FUNC",
    "IIAPI_EV_SETCONNPARM_FUNC",
    "IIAPI_EV_SETDESCR_FUNC",
    "IIAPI_EV_ABORT_FUNC",
    "IIAPI_EV_XASTART_FUNC",
    "IIAPI_EV_XAEND_FUNC",
    "IIAPI_EV_XAPREP_FUNC",
    "IIAPI_EV_XACOMMIT_FUNC",
    "IIAPI_EV_XAROLL_FUNC",
    "IIAPI_EV_ACCEPT_RCVD",
    "IIAPI_EV_CFROM_RCVD",
    "IIAPI_EV_CINTO_RCVD",
    "IIAPI_EV_DONE_RCVD",
    "IIAPI_EV_ERROR_RCVD",
    "IIAPI_EV_EVENT_RCVD",
    "IIAPI_EV_IACK_RCVD",
    "IIAPI_EV_NPINTERUPT_RCVD",
    "IIAPI_EV_PROMPT_RCVD",
    "IIAPI_EV_QCID_RCVD",
    "IIAPI_EV_REFUSE_RCVD",
    "IIAPI_EV_REJECT_RCVD",
    "IIAPI_EV_RELEASE_RCVD",
    "IIAPI_EV_RESPONSE_RCVD",
    "IIAPI_EV_RETPROC_RCVD",
    "IIAPI_EV_TDESCR_RCVD",
    "IIAPI_EV_TRACE_RCVD",
    "IIAPI_EV_TUPLE_RCVD",
    "IIAPI_EV_GCN_RESULT_RCVD",
    "IIAPI_EV_UNEXPECTED_RCVD",
    "IIAPI_EV_RESUME",
    "IIAPI_EV_CONNECT_CMPL",
    "IIAPI_EV_DISCONN_CMPL",
    "IIAPI_EV_SEND_CMPL",
    "IIAPI_EV_SEND_ERROR",
    "IIAPI_EV_RECV_ERROR",
    "IIAPI_EV_DONE",

};

II_EXTERN char *
IIapi_printEvent( IIAPI_EVENT event )
{
    return( IIAPI_PRINT_ID( event, CHAR_ARRAY_SIZE( eventStr ), eventStr ) );
}



/*
** Name: IIapi_printStatus
**
** Description:
**     This function converts API status to ASCII string.
**
**     status	Status
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**	23-Jan-95 (gordy)
**	    Created.
*/

static char *
statusStr[] =
{
    "SUCCESS",
    "MESSAGE",
    "WARNING",
    "NO_DATA",
    "ERROR",
    "FAILURE",
    "NOT_INITIALIZED",
    "INVALID_HANDLE",
    "OUT_OF_MEMORY"
};

II_EXTERN char *
IIapi_printStatus( IIAPI_STATUS status )
{
    return( IIAPI_PRINT_ID(status, CHAR_ARRAY_SIZE( statusStr ), statusStr) );
}


/*
** Name: IIapi_printID
**
** Description:
**	Returns the text string from a string table representing 
**	a numeric value.  Provides bounds checking and and out of
**	bounds text indication.
**
** Input:
**	id		Numeric value to be translated.
**	id_cnt		Number of entries in string table.
**	ids		String table.
** 
** Output:
**	None.
**
** Returns:
**	char *		String table entry or boundary violation string.
**
** History:
**	 2-Oct-96 (gordy)
**	    Created.
*/

II_EXTERN char *
IIapi_printID( i4  id, i4  id_cnt, char **ids )
{
    return ( (id < 0  ||  id >= id_cnt) ? errStr : ids[ id ] );
}



/*
** Name: IIapi_tranID2Str
**
** Description:
**     This function converts transaction ID to ASCII string.
**
**     tranID   transaction ID
**     buffer   buffer for ASCII string representing the transaction ID
**
** Return value:
**     tranStr  ASCII string representing transaction ID
**
** Re-eneventcy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**	14-Jun-95 (gordy)
**	    Transaction name not significant in Ingres transaction IDs.
**	28-Jul-95 (gordy)
**	    Use fixed length types.
*/

II_EXTERN char *
IIapi_tranID2Str( IIAPI_TRAN_ID *tranID, char *buffer )
{
    buffer[0] = '\0';

    if ( tranID->ti_type == IIAPI_TI_IIXID )
    {
	IIAPI_II_DIS_TRAN_ID *iiTranID = &tranID->ti_value.iiXID;

	STprintf( buffer, "II = %d:%d", iiTranID->ii_tranID.it_highTran,
		  iiTranID->ii_tranID.it_lowTran );
    }
    else
    {
	IIAPI_XA_DIS_TRAN_ID *xaTranID = &tranID->ti_value.xaXID;
	
	STprintf( buffer, "XA = 0x%x:%d:%d:%d:%d",
		  xaTranID->xa_tranID.xt_formatID,
		  xaTranID->xa_tranID.xt_gtridLength,
		  xaTranID->xa_tranID.xt_bqualLength,
		  xaTranID->xa_branchSeqnum,
		  xaTranID->xa_branchFlag );
    }
    
    return( buffer );
}




/*
** Name: IIapi_nopDisplay
**
** Description:
**     This function is linked with the Asychronous API when IIAPI_TRACE is
**     off.  It does not do anything because it is not supposed to be
**     called, the trace level should always be 0.
**
** Re-entrancy:
**	This function does not update shared memory.
**
** History:
**      20-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
*/

II_EXTERN II_VOID
IIapi_nopDisplay()
{
    return;
}
