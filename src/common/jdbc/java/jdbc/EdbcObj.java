/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcObj.java
**
** Description:
**	Base class for EDBC driver classes which implement JDBC
**	interfaces and which drive communications with the server.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	29-Sep-99 (gordy)
**	    Permit BLOB data to interrupt message processing.
**	16-Nov-99 (gordy)
**	    Added query timeouts.
**	13-Dec-99 (gordy)
**	    Added fetch limit.
**	19-May-00 (gordy)
**	    Added result parameter end-of-data and corresponding status field.
**	16-Jun-00 (gordy)
**	    Added procedure return value.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	31-Oct-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	28-Mar-01 (gordy)
**	    Database connection and tracing given package access
**	    so as to ease access to connection oriented info.
**	    Tracing and default tracing added.
**	11-Apr-01 (gordy)
**	    Replaced date formatter repository with static formatters.
**	18-Apr-01 (gordy)
**	    Dropped getDefaultTrace().
**      29-Aug-01 (loera01) SIR 105641
**          Added setProcResult method for retrieving return values of
**          database procedures.
**      10-Apr-03 (weife01) BUG 110037
**          Added parseConnDate and formatConnDate to synchronize the 
**          DateFormat returned by getConnDateFormat
*/

import	java.text.DateFormat;
import	java.text.SimpleDateFormat;
import	java.util.TimeZone;
import	java.util.Hashtable;
import	java.sql.SQLException;
import	java.sql.SQLWarning;
import	ca.edbc.util.MsgConst;
import	ca.edbc.util.DbmsConst;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.Timer;
import	ca.edbc.io.DbConn;

/*
** Name: EdbcObj
**
** Description:
**	Base class for EDBC classes which implement JDBC interfaces
**	and which share common functionality associated with server
**	communications.
**
**  Constants:
**
**	D_EPOCH			Date epoch value (zero or start date).
**	T_EPOCH			Time epoch value (zero or start time).
**	TS_EPOCH		Timestamp epoch value.
**	D_FMT			Date format template.
**	T_FMT			Time format template.
**	TS_FMT			Timestamp format template.
**
**  Interface Methods:
**
**	getWarnings		Retrieve the connection message list.
**	clearWarnings		Clear the connection message list.
**	timeExpired		Timer callback method.
**
**  Public Methods:
**
**	getDefaultTrace		Default internal tracing.
**	toString		EDBC tracing title.
**
**  Protected Data:
**
**	dbc			The database connection.
**	trace			Tracing.
**	title			Class title for tracing.
**	tr_id			Instance ID for tracing.
**	df_ts			Date formatter for Timestamp values.
**	df_date			Date formatter for Date values.
**	df_time			Date formatter for Time values.
**	stmt_id			Statement ID.
**	updateCount		Query update count.
**	fetchSize		Optimal number of rows to fetch.
**	cursorReadonly		Cursor is read-only.
**	resultEOD		End-of-data.
**	resultProc		Procedure result in procRetVal.
**	procRetVal		Procedure return value.
**	warnings		SQL warnings.
**
**  Protected Methods:
**
**	setWarning		Add warning to warning list.
**	getConnDateFormat	Get date formatter for connection.
**	clearResults		Clear query results.
**	readResults		Read Server result messages.
**	readDesc		Default handler for descriptor messages.
**	readData		Default handler for data messages.
**      setProcResult           Set procedure return value.
**
**  Private Data:
**
**	inst_count		Instance counter.
**	default_trace		Default internal tracing.
**	df_gmt			GMT Date formatter.
**	df_lcl			Local Date formatter.
**	timer			Statement execution timer.
**	timed_out		Time-out flag.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	16-Nov-99 (gordy)
**	    Added query timeouts.
**	13-Dec-99 (gordy)
**	    Added fetch limit.
**	19-May-00 (gordy)
**	    Added result end-of-data indicator.
**	16-Jun-00 (gordy)
**	    Added procedure return value.
**	 4-Oct-00 (gordy)
**	    Added instance counter, inst_count, and tracing ID,
**	    tr_id, to standardize internal tracing.
**	31-Oct-00 (gordy)
**	    Renamed fetchLimit to fetchSize, added cursorReadonly.
**	 3-Nov-00 (gordy)
**	    Added date formatting info: D_EPOCH, T_EPOCH, TS_EPOCH, 
**	    D_FMT, T_FMT, TS_FMT, GMT, df_date, df_time, getDateFormat(), 
**	    getLocalDateFormat(), getConnDateFormat().
**	28-Mar-01 (gordy)
**	    Database connection and tracing given package access
**	    so as to ease access to connection oriented info.
**	    Added default_trace and getDefaultTrace().
**	11-Apr-01 (gordy)
**	    Replaced date formatter repository with static formatters
**	    for GMT (df_gmt) and local (df_lcl) timezones and a re-
**	    usable formatter (df_ts) for other timezones.  Methods
**	    getLocalDateFormat() and getDateFormat() no longer needed.
**	18-Apr-01 (gordy)
**	    Dropped getDefaultTrace() now that tracing passed as needed.
*/

class
EdbcObj
    implements	MsgConst, DbmsConst, EdbcConst, EdbcErr, Timer.Callback
{

    /*
    ** Constants used in date processing.
    */
    public static final String	D_EPOCH = "1970-01-01";
    public static final String	T_EPOCH = "00:00:00";
    public static final String	TS_EPOCH = D_EPOCH + " " + T_EPOCH;
    public static final String	D_FMT = "yyyy-MM-dd";
    public static final String	T_FMT = "HH:mm:ss";
    public static final String	TS_FMT = D_FMT + " " + T_FMT;
    
    /*
    ** The following need package access to allow easy
    ** access to this connection relevent info.
    */
    DbConn			dbc = null;
    EdbcTrace			trace = null;

    protected String		title;	// Tracing Ids
    protected String		tr_id;

    /*
    ** Date formatters for use by sub-classes in handling date, time
    ** and timestamp values.  There are two fixed timezone formatters
    ** which are generally used for handling values between the driver
    ** and the DBMS.  The GMT formatter should be used for an Ingres
    ** DBMS since Ingres date values are stored in GMT.  The local
    ** timezone formatter should be used for a non-Ingres DBMS since
    ** they generally ignore timezone issues and client timezone is
    ** assumed.  The correct formatter for the currect connection can
    ** be accessed by calling getConnDateFormat().
    **
    ** The other formatters are for use when a timezone other than GMT
    ** or the local timezone is needed.  These formatters are generally
    ** used for handling values between the Java application and the
    ** driver.  There is a formatter for timestamps, dates, and times.
    ** A timezone must be assigned to these formatters when used.  Use 
    ** of these formatters requires synchronization on the formatters 
    ** themselves.
    */
    protected static final DateFormat	df_gmt = new SimpleDateFormat( TS_FMT );
    protected static final DateFormat	df_lcl = new SimpleDateFormat( TS_FMT );
    protected static final DateFormat	df_ts = new SimpleDateFormat( TS_FMT );
    protected static final DateFormat	df_date = new SimpleDateFormat( D_FMT );
    protected static final DateFormat	df_time = new SimpleDateFormat( T_FMT );

    /*
    ** Server request result information.
    */
    protected long		stmt_id = 0;
    protected int		updateCount = -1;
    protected int		fetchSize = 1;
    protected boolean		cursorReadonly = false;
    protected boolean		resultEOD = false;
    protected boolean		resultProc = false;
    protected int		procRetVal = 0;
    protected SQLWarning	warnings = null;

    /*
    ** Count number of EDBC objects created.
    ** Used to assign unique tracing ID.
    */
    private static int		inst_count = 0;

    /*
    ** Internal tracing used if not provided in constructor.
    */
    private static EdbcTrace	default_trace = null;

    /*
    ** GMT and Local timezone date formatters.
    ** Timezone is assigned in static initializer.
    */

    /*
    ** The only thing we can do to timeout a query is
    ** set a timer locally and attempt to cancel the
    ** query if the time expires.  We also flag the
    ** timeout condition to translate the cancelled
    ** query error into a timeout error.
    */
    private Timer		timer = null;
    private boolean		timed_out = false;


/*
** Name: <static>
**
** Description:
**	Static initializer.
**
**	Set the timezones for the fixed timezone date formatters.
**
** History:
**	11-Apr-01 (gordy)
**	    Created.
*/

static
{
    df_gmt.setTimeZone( TimeZone.getTimeZone( "GMT" ) );
    df_lcl.setTimeZone( TimeZone.getDefault() );
}


/*
** Name: EdbcObj
**
** Description:
**	Class constructor.
**
** Input:
**	dbc	Database connection.
**	trace	Connection tracing.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Create unique tracing ID.
**	28-Mar-01 (gordy)
**	    Tracing added as parameter.
*/

protected
EdbcObj( DbConn dbc, EdbcTrace trace )
{
    this.dbc = dbc;

    if ( trace != null )
	this.trace = trace;
    else
    {
	if ( default_trace == null )
	    default_trace = new TraceDV( EdbcTrace.edbcTraceID );
	this.trace = default_trace;
    }

    title = shortTitle + "[" + inst_count + "]";
    tr_id = "Edbc[" + (inst_count++) + "]";
} // EdbcObj


/*
** Name: toString
**
** Description:
**	Return the formatted name of this object.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Object name.
**
** History:
**	 7-Nov-00 (gordy)
**	    Created.
*/

public String
toString()
{
    return( title );
}


/*
** Name: getWarnings
**
** Description:
**	Retrieve the connection warning/message list.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	SQLWarning  SQL warning/message list, may be NULL.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public SQLWarning
getWarnings()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getWarnings()" );
    return( warnings );
} // getWarnings


/*
** Name: clearWarnings
**
** Description:
**	Clear the connection warning/message list.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 5-May-99 (gordy)
**	    Created.
*/

public void
clearWarnings()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".clearWarnings()" );
    warnings = null;
    return;
} // clearWarnings


/*
** Name: setWarning
**
** Description:
**	Add a new warning to the list of warnings.
**
** Input:
**	warn	New warning.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 8-Nov-00 (gordy)
**	    Created.
*/

protected void
setWarning( SQLWarning warn )
{
    if ( warnings == null )
	warnings = warn;
    else
	warnings.setNextWarning( warn );
    return;
}


/*
** Name: timeExpired
**
** Description:
**	Implements the callback method for the Timer Callback 
**	interface.  Called by the Timer object created in 
**	readResults().  Cancel the current query.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	16-Nov-99 (gordy)
**	    Created.
*/

public void
timeExpired()
{
    try 
    { 
	if ( timer != null )  timed_out = true;
	dbc.cancel();
    }
    catch( Exception ignore ) {}
    return;
}


/*
** Name: getConnDateFormat
**
** Description:
**	Returns the date formatter/parser for the timezone used
**	on the connection.  The connection timezone is either GMT
**	or the local timezone.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	DateFormat  Date formatter.
**
** History:
**	 3-Nov-00 (gordy)
**	    Created.
**	11-Apr-01 (gordy)
**	    Replaced repository with formatters for GMT and local timezone.
*/

protected DateFormat
getConnDateFormat()
    throws EdbcEx
{
    return( dbc.use_gmt_tz ? df_gmt : df_lcl );
}


/*
** Name: clearResults
**
** Description:
**	Clear query result fields.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Nov-00 (gordy)
**	    Created.
**	 6-Mar-01 (gordy)
**	    Clear warnings also.
*/

protected void
clearResults()
{
    warnings = null;
    stmt_id = 0;
    updateCount = -1;
    fetchSize = 1;
    cursorReadonly = false;
    resultEOD = false;
    resultProc = false;
    procRetVal = 0;
    return;
}


/*
** Name: readResults
**
** Description:
**	Read server result messages with optional timeout.
**	See readResult() method with no parameter for details
**	on message processing.
**
** Input:
**	timeout	    Time in seconds (negative for milli-seconds).
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 16-Nov-99 (gordy)
**	    Created.
*/

protected EdbcRSMD
readResults( int timeout )
    throws EdbcEx
{
    EdbcRSMD rsmd;

    if ( timeout == 0 )
	timer = null;
    else
    {
	timed_out = false;
	timer = new Timer( timeout, (Timer.Callback)this );
	timer.start();
    }

    try { rsmd = readResults(); }
    finally
    {
	if ( timer != null )  
	{
	    timer.interrupt();
	    timer = null;
	}

	timed_out = false;
    }

    return( rsmd );
}


/*
** Name: readResults
**
** Description:
**	Read server result messages.  Error messages are turned into
**	SQLExceptions and SQLWarnings as appropriate.  Returns once
**	the DONE message is received.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 5-May-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Permit BLOB data to interrupt message processing.
**	16-Nov-99 (gordy)
**	    Added query timeouts.
**	16-Dec-99 (gordy)
**	    Added JDBC_RP_FETCH_LIMIT.
**	19-May-00 (gordy)
**	    Added JDBC_RP_EOD.
**	16-Jun-00 (gordy)
**	    Added procedure return value.
**	31-Oct-00 (gordy)
**	    Added read-only cursor return value.
**      29-Aug-01 (loera01) SIR 105641
**          Execute setProcReturn if a DB procedure return value is read.
*/

protected EdbcRSMD
readResults()
    throws EdbcEx
{
    EdbcEx	ex_list = null;
    EdbcRSMD	rsmd = null;
    int         pRetVal = 0;

    try
    {
	byte	    msg_id;

	do
	{
	    resultEOD = false;
	    msg_id = dbc.receive();

	    if ( timer != null )  
	    {
		/*
		** Disable timer when first response received.
		*/
		timer.interrupt();
		timer = null;
	    }

	    switch( msg_id )
	    {
	    case JDBC_MSG_DESC : 
		rsmd = readDesc();
		break;

	    case JDBC_MSG_DATA : 
		if ( readData() )  return( rsmd );
		break;

	    case JDBC_MSG_DONE :
		while( dbc.moreData() )
		{
		    short   param_id = dbc.readShort();
		    short   param_len = dbc.readShort();

		    switch( param_id )
		    {
		    case JDBC_RP_ROWCOUNT :
			switch( param_len )
			{
			case 1 :  updateCount = dbc.readByte();  break;
			case 2 :  updateCount = dbc.readShort(); break;
			case 4 :  updateCount = dbc.readInt();   break;

			default :
			    if ( trace.enabled( 1 ) )
				trace.write( tr_id + ": Invalid rowcount len " + 
					     param_len );
			    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
			}
			break;

		    case JDBC_RP_XACT_END :
			if ( param_len > 0 )
			{
			    if ( trace.enabled( 1 ) )
				trace.write( tr_id + ": Invalid XACT param len " + 
					     param_len );
			    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
			}

			dbc.endXact();
			break;

		    case JDBC_RP_STMT_ID :
			if ( param_len != 8 )
			{
			    if ( trace.enabled( 1 ) )
				trace.write( tr_id + ": Invalid STMT_ID len " + 
					     param_len );
			    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
			}

			stmt_id = (((long)dbc.readInt()) << 32) |
			          (((long)dbc.readInt()) & 0xffffffff);
			break;

		    case JDBC_RP_FETCH_LIMIT :
			switch( param_len )
			{
			case 1 :  fetchSize = dbc.readByte();  break;
			case 2 :  fetchSize = dbc.readShort(); break;
			case 4 :  fetchSize = dbc.readInt();   break;

			default :
			    if ( trace.enabled( 1 ) )
				trace.write( tr_id + ": Invalid fetchlimit len " +
					     param_len );
			    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
			}
			break;

		    case JDBC_RP_EOD :
			if ( param_len != 0 )
			{
			    trace.write( tr_id + ": Invalid EOD len " + 
					 param_len );
			    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
			}
			resultEOD = true;
			break;

		    case JDBC_RP_PROC_RESULT :
			switch( param_len )
			{
			case 1 : pRetVal = dbc.readByte();	break;
			case 2 : pRetVal = dbc.readShort();	break;
			case 4 : pRetVal = dbc.readInt();	break;

			default :
			    if ( trace.enabled( 1 ) )
				trace.write( tr_id + ": Invalid proc result len" +
					     param_len );
			    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
			}
			setProcResult( pRetVal );
			break;

		    case JDBC_RP_READ_ONLY :
			if ( param_len != 0 )
			{
			    trace.write( tr_id + ": Invalid READ_ONLY len " + 
					 param_len );
			    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
			}
			cursorReadonly = true;
			break;
			
		    default :
			if ( trace.enabled( 1 ) )
			    trace.write( tr_id + ": Invalid result param ID " + 
					 param_id );
			throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
		    }
		}
		break;
	    
	    case JDBC_MSG_ERROR :
		{
		    int	    status = dbc.readInt();
		    String  sqlState = dbc.readString( 5 );
		    byte    type = dbc.readByte();
		    String  msg = dbc.readString();

		    if ( type == JDBC_ET_ERR )
		    {
			/*
			** We translate the query canceled error
			** from a timeout into a JDBC driver error.
			*/
			EdbcEx ex =
			    ( timed_out  &&  
				    status == E_AP0009_QUERY_CANCELLED.code ) 
			    ? EdbcEx.get( E_JD000F_TIMEOUT )
			    : new EdbcEx( msg, sqlState, status );
			
			if ( trace.enabled( 1 ) )
			    trace.write( tr_id + ": Received error" + 
					 " '" + sqlState + "' 0x" + 
					 Integer.toHexString( status ) + 
					 " -- " + msg );
		
			if ( ex_list == null )
			    ex_list = ex;
			else
			    ex_list.setNextException( ex );
		    }
		    else
		    {
			if ( trace.enabled( (type == JDBC_ET_MSG) ? 3 : 1 ) )
			    trace.write( tr_id + ": Received " + 
					 (type == JDBC_ET_MSG ? "message" 
							      : "warning") + 
					 " '" + sqlState + "' 0x" + 
					 Integer.toHexString( status ) + 
					 " -- " + msg );

			setWarning( new SQLWarning( msg, sqlState, status ) );
		    }
		}
		break;

	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid message ID " + msg_id );
		throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
	    }

	    if ( dbc.moreData() )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": end-of-message not reached" );
		throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
	    }
	} while( msg_id != JDBC_MSG_DONE );
    }
    catch( EdbcEx ex )
    {
	if ( ex_list == null )
	    ex_list = ex;
	else
	    ex_list.setNextException( ex );
    }

    if ( ex_list != null )  throw ex_list;
    return( rsmd );
} // readResults

/*
** Name: readDesc
**
** Description:
**	Read a data descriptor message.  The default action is to
**	throw an exception.  A sub-class should override this method
**	if a data descriptor message is a valid request result.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	EdbcRSMD    Result set data descriptor.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
*/

protected EdbcRSMD
readDesc()
    throws EdbcEx
{
    if ( trace.enabled( 1 ) )  trace.write( tr_id + ": unexpected result set" );
    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
} // readDesc

/*
** Name: readData
**
** Description:
**	Read a data message.  The default action is to throw an exception.  
**	A sub-class should override this method if a data message is a 
**	valid request result.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Interrupt reading results.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Changed return type to permit BLOB segments to
**	    interrupt the normal processing of messages.
*/

protected boolean
readData()
    throws EdbcEx
{
    if ( trace.enabled(1) )  trace.write(tr_id + ": unexpected result data.");
    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
} // readData

/*
** Name: setProcResult
**
** Description:
**	Set the procRetVal and returnProc class variables.
**
** Input:
**	retval		Procedure return value.
**
** Output:
**	None.
**
** Returns:
**	Void.
**
** History:
**      29-Aug-01 (loera01) SIR 105641
**	    Created.
*/

protected void
setProcResult(int retVal )
    throws EdbcEx
{
   if ( trace.enabled( 3 ) )  trace.write( tr_id + ".setProcResult(" +
       retVal + ")" );
   procRetVal = retVal;
   resultProc = true;
}

 /*
 ** Name: parseConnDate
 **
 ** Description:
 **    When parse a DateFormat reutrned by getConnDateFormat,
 **    the DateFormat df will be synchronized for thread safety.
 **
 ** Input:
 **    String             A string represent a date format
 **
 ** Output:
 **    None.
 **
 ** Returns:
 **    java.util.date     A formatted date
 **
 ** History:
 **     9-Apr-03 (weife01)
 **        Created.
 **    11-Jan-04 (rajus01) Startrak #:EDJDBC95, Bug# 113722
 **	   Changed the more specific ParseException to generic Exception to
 **	   be caught while handling dates.
 */

 protected java.util.Date
 parseConnDate( String str)
     throws EdbcEx
 {
     java.util.Date date;
     DateFormat df;

     try
     {
         df = getConnDateFormat();
         synchronized( df )
         {
             date = df.parse( str );
         }
         return date;
     }
     catch( Exception ex )
     {
         if ( trace.enabled( 1 ) )
             trace.write(tr_id + ": invalid date '" + str + "'");
         throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
     }
 }

 /*
 ** Name: formatConnDate
 **
 ** Description:
 **    When format a DateFormat reutrned by getConnDateFormat,
 **    the DateFormat df will be synchronized for thread safety.
 **
 ** Input:
 **    java.util.date     A date
 **
 ** Output:
 **    None.
 **
 ** Returns:
 **    String             A string represent a formatted date
 **
 ** History:
 **     9-Apr-03 (weife01)
 **        Created.
 */

 protected String
 formatConnDate( java.util.Date date1 )
     throws EdbcEx
 {
     DateFormat df;
     String str;

     try
     {
         df = getConnDateFormat();
         synchronized( df )
         {
             str = df.format( date1 );
         }
         return str;
     }
     catch( IllegalArgumentException  ex )
     {
         if ( trace.enabled( 1 ) )
             trace.write(tr_id + ": invalid date '" + date1 + "'");
         throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
     }
 }

} // class EdbcObj
