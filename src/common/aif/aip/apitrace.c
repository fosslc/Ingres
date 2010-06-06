/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <cm.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <mu.h>
# include <si.h>
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
**	IIapi_printTrace	Print DBMS trace message.
**	IIapi_flushTrace	Flush DBMS trace output.
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
**	26-May-10 (gordy)
**	    Log DBMS trace messages.
*/

#define	HEADER	ERx("---------- Connection %p ------------------------------------\n")
#define	FOOTER	ERx("-------------------------------------------------------------------\n")

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
**	26-May-10 (gordy)
**	    Initialize DBMS trace handling.
*/

II_EXTERN II_VOID
IIapi_initTrace( II_VOID )
{
    char	*retVal;
    STATUS	status;
    CL_ERR_DESC	err_code;
    
    if ( ! IIapi_static )  return;

    IIapi_static->api_trace_level = 0;
    IIapi_static->api_trace_file = NULL;
    IIapi_static->api_dbms_file = NULL;
    IIapi_static->api_trace_handle = NULL;
    IIapi_static->api_trace_flags = 0;

    if ( MUi_semaphore( &IIapi_static->api_trace_sem ) != OK )  return;

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
    
    NMgtAt( "II_API_SET", &retVal );

    if ( retVal  &&  *retVal )
    {
	char    *cp, buff[ ER_MAX_LEN + 1 ];
	char    *tracefile = NULL;
	i4	tflen;

	STlcopy( retVal, buff, ER_MAX_LEN );

	for( cp = buff; *cp; cp++ )
	{
	    while( CMwhite( cp ) )  CMnext( cp );
	    if ( *cp == EOS )  break;

	    if ( STbcompare( ERx("printtrace"), 0, cp, 10, TRUE ) == 0 )
		IIapi_static->api_trace_flags |= IIAPI_TRACE_DBMS;
	    else  if ( STbcompare( ERx("tracefile"), 0, cp, 9, TRUE ) == 0 )
	    {
		cp += 9;
	        while( CMwhite( cp ) )  CMnext( cp );

		if ( *cp != EOS  &&  *cp != ';' )  
		{
		    tracefile = cp;
		    CMnext( cp );

		    while( *cp != EOS  &&  *cp != ';'  &&  ! CMwhite( cp ) )
			CMnext( cp );

		    tflen = cp - tracefile;
		}
	    }

	    if ( ! (cp = STindex( cp, ERx(";"), 0 )) )  break;
	}

	if ( IIapi_static->api_trace_flags & IIAPI_TRACE_DBMS )
	{
	    LOCATION	loc;
	    STATUS	status;
	    
	    if ( tracefile == NULL )
	    	tracefile = ERx("iiprttrc.log");
	    else
		tracefile[ tflen ] = EOS;

	    LOfroms( FILENAME, tracefile, &loc );
	    status = SIopen( &loc, ERx("w"), &IIapi_static->api_dbms_file );

	    if ( status != OK )
	    {
		IIapi_static->api_trace_flags &= ~IIAPI_TRACE_DBMS;
		IIapi_static->api_dbms_file = NULL;
	    }
	}
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
**	26-May-10 (gordy)
**	    Terminate DBMS tracing.
*/

II_EXTERN II_VOID
IIapi_termTrace( II_VOID )
{
    CL_ERR_DESC	    err_code;
    
    if ( IIapi_static )
    {
        if ( IIapi_static->api_trace_flags & IIAPI_TRACE_DBMS )
	{
	    /*
	    ** If DBMS tracing has occured (trace handle has been changed),
	    ** write a final separation line to the trace file.
	    */
	    if ( IIapi_static->api_trace_handle != NULL )
	    {
		SIfprintf( IIapi_static->api_dbms_file, FOOTER );
		SIflush( IIapi_static->api_dbms_file );
	    }

	    SIclose( IIapi_static->api_dbms_file );
	}

	/*
	** Close the trace file.
	*/
	if ( IIapi_static->api_trace_file )
	    TRset_file( TR_F_CLOSE, IIapi_static->api_trace_file, 
			STlength( IIapi_static->api_trace_file ), &err_code );

	IIapi_static->api_trace_level = 0;
	IIapi_static->api_trace_file = NULL;
	IIapi_static->api_dbms_file = NULL;
	IIapi_static->api_trace_handle = NULL;
	IIapi_static->api_trace_flags = 0;

	MUr_semaphore( &IIapi_static->api_trace_sem );
    }

    return;
}



/*
** Name: IIapi_printTrace
**
** Description:
**	Write DBMS trace message to trace log.
**
** Input:
**	handle	Handle associated with trace message (may be NULL).
**	first	Is this the first trace of current group.
**	str	Message to be written.
**	length	Length of message, < 0 if EOS terminated string.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	26-May-10 (gordy)
**	    Created.
*/

II_EXTERN II_VOID
IIapi_printTrace( PTR handle, bool first, char *str, i4 length )
{
    if ( ! (IIapi_static->api_trace_flags & IIAPI_TRACE_DBMS) )  return;
    MUp_semaphore( &IIapi_static->api_trace_sem );

    /*
    ** Write trace header when new connection begins 
    ** tracing.  A separator is written when a new
    ** group of trace messages is started for the 
    ** active connection.
    */
    if ( handle  &&  handle != IIapi_static->api_trace_handle )
	SIfprintf( IIapi_static->api_dbms_file, HEADER, handle );
    else  if ( first )
	SIfprintf( IIapi_static->api_dbms_file, FOOTER );

    IIapi_static->api_trace_handle = handle;

    if ( length < 0 )
	SIfprintf( IIapi_static->api_dbms_file, str );
    else  if ( length > 0 )
    {
	i4 dummy;

        SIwrite( length, str, &dummy, IIapi_static->api_dbms_file );
    }

    MUv_semaphore( &IIapi_static->api_trace_sem );
    return;
}



/*
** Name: IIapi_flushTrace
**
** Description:
**	Flush the trace message written to the trace log.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	VOID.
**
** History:
**	26-May-10 (gordy)
**	    Created.
*/

II_EXTERN II_VOID
IIapi_flushTrace( II_VOID )
{
    if ( IIapi_static->api_trace_flags & IIAPI_TRACE_DBMS )
	SIflush( IIapi_static->api_dbms_file );
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
