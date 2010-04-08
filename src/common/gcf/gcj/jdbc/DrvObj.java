/*
** Copyright (c) 1999, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: DrvObj.java
**
** Description:
**	Defines the base class for JDBC driver classes which send 
**	and/or receive DAM-ML messages to/from the server.
**
**  Classes
**
**	DrvObj
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	29-Sep-99 (gordy)
**	    Permit BLOB data to interrupt message processing.
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
**	 3-Nov-00 (gordy)
**	    Added date formatting info.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	21-Feb-03 (gordy)
**	    Updated to JDBC 3.0.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters and timezones.
**	 7-Jul-03 (gordy)
**	    Centralized timezone selection and added timezone for literals.
**	20-Aug-03 (gordy)
**	    Consolidated result flags into single parameter.
**	12-Sep-03 (gordy)
**	    Date/Time routines moved to SqlDates utility.
**	 6-Oct-03 (gordy)
**	    RESULT messages (formerly DONE) can now occur before end-of-group.
**	21-Apr-06 (gordy)
**	    Added INFO messages.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	 3-Jul-06 (gordy)
**	    Write DBMS trace messages to dedicated log file.
**	20-Jul-06 (gordy)
**	    Support XA errors.
**	 9-Nov-06 (gordy)
**	    Handle query text over 64K.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
**      24-Nov-08 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to support
**            JDBC 4.0 SQLException hierarchy.
*/

import	java.sql.SQLException;
import	java.sql.SQLWarning;
import	com.ingres.gcf.util.CharSet;
import	com.ingres.gcf.util.TraceLog;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.SqlWarn;
import	com.ingres.gcf.util.XaEx;
import	com.ingres.gcf.dam.MsgConst;
import	com.ingres.gcf.dam.MsgConn;


/*
** Name: DrvObj
**
** Description:
**	Base class for JDBC driver classes which send and/or receive 
**	DAM-ML messages to/from the server.
**
**	Provides the readResults() method for reading and processing
**	messages expected as the result of a server request.  This
**	method in turn calls the default handler methods to read 
**	and process specific message types: readDesc(), readData(), 
**	readInfo(), readError() and readResult().  There is also a 
**	default handler method for procedure return values: 
**	setProcResult().  These methods are expected to be over-
**	ridden by sub-classes which need to provide non-default 
**	processing.
**
**  Interface Methods:
**
**	getWarnings		Retrieve the connection warning list.
**	clearWarnings		Clear the connection warning list.
**
**  Public Methods:
**
**	toString		Tracing title.
**
**  Constants:
**
**	RSLT_PREFETCH		Result pre-fetch row limit available.
**	RSLT_ROW_STAT		Result row status available.
**	RSLT_ROW_POS		Result row position available.
**	RSLT_ROW_CNT		Result row count available.
**	RSLT_STMT_ID		Result statement ID available.
**	RSLT_PROC_VAL		Result procedure return value available.
**	RSLT_TBLKEY		Result table key available.
**	RSLT_OBJKEY		Result object key available.
**
**  Protected Data:
**
**	conn			The database connection.
**	msg			The MSG connection.
**	trace			Tracing output.
**	title			Class title for tracing.
**	tr_id			Instance ID for tracing.
**	inst_id			Object instance ID.
**	warnings		SQL warnings.
**	rslt_flags		Result flags.
**	rslt_items		Result items received.
**	rslt_val_fetch		Optimal number of rows to fetch.
**	rslt_val_rowstat	Row status.
**	rslt_val_rowpos		Row position.
**	rslt_val_rowcnt		Query update count.
**	rslt_val_stmt		Statement ID.
**	rslt_val_proc		Procedure return value.
**	rslt_val_tblkey		Table key.
**	rslt_val_objkey		Object key.
**
**  Protected Methods:
**
**	setWarning		Add warning to warning list.
**	clearResults		Clear query results.
**	readResults		Read Server result messages.
**	readDesc		Default handler for descriptor messages.
**	readData		Default handler for data messages.
**	readInfo		Default handler for info messages.
**	readError		Default handler for error messages.
**	readResult		Default handler for result messages.
**      setProcResult           Default handler for procedure return value.
**
**  Private Data:
**
**	inst_count		Instance counter.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
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
**	    Added date formatting info: GMT, df_date, df_time, 
**	    getDateFormat(), getLocalDateFormat(), getConnDateFormat().
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
**	31-Oct-02 (gordy)
**	    Connection info now centralized in single object, conn,
**	    but copies of the messaging connection, msg, and tracing,
**	    trace, objects are held locally for easy access by sub-
**	    classes.  Date formatting constants moved to DrvConst 
**	    interface.  Added inst_id for use by sub-classes.  No
**	    longer need default_trace since tracing always provided
**	    by DrvConn.  Removed timed query results support and
**	    associated timer and timed_out fields and timeExpired()
**	    and readResults() methods.  Extracted message processing
**	    from readResults() to readError() and readDone() methods.
**	21-Feb-03 (gordy)
**	    Added table and object keys.  Standardized result values
**	    and flags.
**	 7-Jul-03 (gordy)
**	    Added getLiteralTimeZone().
**	20-Aug-03 (gordy)
**	    Consolidated result flags into single parameter.  Replaced
**	    rslt_eod and rslt_readonly with rslt_flags.
**	12-Sep-03 (gordy)
**	    Moved date/time parsers/formatters to SqlDates utility.
**	    Replaced timezone methods with GMT indicator methods.
**	 6-Oct-03 (gordy)
**	    Renamed readDone() to readResult().
**	21-Apr-06 (gordy)
**	    Added readInfo().
**	19-Jun-06 (gordy)
**	    Moved timeValuesInGMT() and timeLiteralsInGMT() to DrvConn.
**	 9-Nov-06 (gordy)
**	    Added writeQueryText().
**	 6-Apr-07 (gordy)
**	    Replaced individual result booleans with flags.  Added
**	    row status and position results.
*/

class
DrvObj
    implements	DrvConst, MsgConst, GcfErr
{

    protected DrvConn		conn		= null;

    /*
    ** The following are copies from DrvConn.
    */
    protected MsgConn		msg		= null;
    protected DrvTrace		trace		= null;

    /*
    ** Tracing Identifiers
    */
    protected String		title		= null;
    protected String		tr_id		= null;
    protected int		inst_id		= 0;

    /*
    ** Server request result information.
    */
    protected int		rslt_flags	= 0;
    protected int		rslt_items	= 0;

    protected static final int	RSLT_PREFETCH	= 0x01;
    protected static final int	RSLT_ROW_STAT	= 0x02;
    protected static final int	RSLT_ROW_POS	= 0x04;
    protected static final int	RSLT_ROW_CNT	= 0x08;
    protected static final int	RSLT_STMT_ID	= 0x10;
    protected static final int	RSLT_PROC_VAL	= 0x20;
    protected static final int	RSLT_TBLKEY	= 0x40;
    protected static final int	RSLT_OBJKEY	= 0x80;

    protected int		rslt_val_fetch	= 0;
    protected int		rslt_val_rowstat= 0;
    protected int		rslt_val_rowpos	= 0;
    protected int		rslt_val_rowcnt	= 0;
    protected long		rslt_val_stmt	= 0;
    protected int		rslt_val_proc	= 0;
    protected byte		rslt_val_tblkey[]	= null;
    protected byte		rslt_val_objkey[]	= null;

    protected SQLWarning	warnings	= null;

    /*
    ** Count number of driver objects created.
    ** Used to assign unique tracing ID.
    */
    private static int		inst_count = 0;


/*
** Name: DrvObj
**
** Description:
**	Class constructor.
**
** Input:
**	conn	Database connection.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

protected
DrvObj( DrvConn conn )
{
    this.conn = conn;
    this.msg = conn.msg;
    this.trace = conn.trace;

    /*
    ** Default tracing identifiers.  Not very usefull,
    ** but better than nothing.
    */
    inst_id = inst_count++;
    title = trace.getTraceName() + "[" + inst_id + "]";
    tr_id = "Jdbc[" + inst_id + "]";
    return;
} // DrvObj


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
} // toString


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
** Name: writeQueryText
**
** Description:
**	Write QUERY message Query Text parameter.
**	Query text is segmented if necessary.
**
** Input:
**	text	Query text.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 9-Nov-06 (gordy)
**	    Created.
*/

protected void
writeQueryText( String text )
    throws SQLException
{
    byte ba[];

    try { ba = msg.getCharSet().getBytes( text ); }
    catch( Exception ex )
    { throw SqlExFactory.get( ERR_GC401E_CHAR_ENCODE ); }	// Should not happen!

    for( int offset = 0; offset < ba.length; )
    {
	int length = Math.min( ba.length - offset, 32000 );
	msg.write( MSG_QP_QTXT );
	msg.write( (short)length );
	msg.write( ba, offset, length );
	offset += length;
    }

    return;
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
**	21-Feb-03 (gordy)
**	    Standardized the result values and flags.
**	 6-Apr-07 (gordy)
**	    Replaced individual boolean indicators with flags.
*/

protected void
clearResults()
{
    warnings = null;
    rslt_flags = 0;
    rslt_items = 0;
    rslt_val_fetch = 0;
    rslt_val_rowstat = 0;
    rslt_val_rowpos = 0;
    rslt_val_rowcnt = 0;
    rslt_val_stmt = 0;
    rslt_val_proc = 0;
    return;
}


/*
** Name: readResults
**
** Description:
**	Read server result messages.  Processes messages until
**	end-of-group is received or sub-class implementations
**	of readData() and readResult() interrupt processing.  
**
**	MSG_DESC and MSG_DATA messages are processed by methods
**	readDesc() and readData() respectively.  The default
**	implementation of these methods throws an exception.
**	Sub-classes must provide their own implementation of
**	these methods if these messages are expected as a part
**	of the server response.
**
**	The readDesc() method returns a Result-set meta-data
**	object representing the MSG_DESC message received,
**	which is then returned if readResults() completes
**	successfully.
**
**	MSG_ERROR and MSG_RESULT messages are processed by methods
**	readError() and readResult() respectively.  The default
**	implementation of these methods are described below.
**	Sub-classes may provide their own implementation of
**	these methods to provide non-default processing.
**
**	MSG_ERROR error messages are converted into exceptions
**	which get thrown after all messages are processed.  
**	MSG_ERROR warning and user messages are saved as warnings. 
**	Multiple exceptions and warnings are chained together.
**
**	MSG_RESULT parameter info is saved in result info members. 
**	The method setProcResult() is also called when a procedure 
**      return value is received.  Sub-classes may override this 
**	method to provide alternate handling of procedure results.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD	Result-set meta-data.
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
**	31-Oct-02 (gordy)
**	    Extracted ERROR and DONE message processing to readError()
**	    and readDone() methods.  Timed query responses processing
**	    removed completely (may be supported by sub-classes by
**	    overridding message processing methods).
**	 6-Oct-03 (gordy)
**	    Renamed readDone() to readResult() to reflect ability to
**	    process intermediate results.  Now returns at end-of-group
**	    message unless interrupted by readData() or readResult().
**	21-Apr-06 (gordy)
**	    Added INFO messages.
**	20-Jul-06 (gordy)
**	    Add support for XA errors.
*/

protected JdbcRSMD
readResults()
    throws SQLException, XaEx
{
    SQLException	ex_list = null;
    XaEx	xa_list = null;
    JdbcRSMD	rsmd = null;
    byte	msg_id;

    try
    {
      msg_process_loop:
	do
	{
	    switch( msg_id = msg.receive() )
	    {
	    case MSG_DESC : 
		rsmd = readDesc();
		break;

	    case MSG_DATA : 
		if ( readData() )  break msg_process_loop;	// interrupt
		break;

	    case MSG_INFO :
		readInfo();
		break;

	    case MSG_ERROR :
		SQLException ex = readError();
		
		if ( ex != null )
		    if ( ex instanceof XaEx )
			xa_list = (XaEx)ex;	/* Save only last error */
		    else  if ( ex_list == null )
			ex_list = ex;
		    else
			ex_list.setNextException( ex );
		break;

	    case MSG_RESULT :
		if ( readResult() )  break msg_process_loop;	// interrupt
		break;
	    
	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid message ID " + msg_id );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }

	    if ( msg.moreData() )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": end-of-message not reached" );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	} while( msg.moreMessages() );
    }
    catch( SQLException ex )
    {
	if ( ex_list == null )
	    ex_list = ex;
	else
	    ex_list.setNextException( ex );
    }

    if ( xa_list != null )  throw xa_list;
    if ( ex_list != null )  throw ex_list;
    return( rsmd );
} // readResults


/*
** Name: readDesc
**
** Description:
**	Read a DESC message.  The default action is to throw an 
**	exception.  A sub-class should override this method if a 
**	data descriptor message is a valid request result.
**
**	A Result-set meta-data object representing the descriptor 
**	is returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD    Result set data descriptor.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
*/

protected JdbcRSMD
readDesc()
    throws SQLException
{
    if ( trace.enabled( 1 ) )  trace.write( tr_id + ": unexpected result set" );
    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
} // readDesc


/*
** Name: readData
**
** Description:
**	Read a DATA message.  The default action is to throw an exception.  
**	A sub-class should override this method if a data message is a 
**	valid request result.
**
**	Returning a value of TRUE will interrupt the processing of server
**	messages by readResults() and cause an immediate return.
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
    throws SQLException
{
    if ( trace.enabled(1) )  trace.write(tr_id + ": unexpected result data.");
    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
} // readData


/*
** Name: readInfo
**
** Description:
**	Read an INFO message.  The INFO message parameters are read 
**	and processed.  Currently, the only INFO parameter supported
**	is Trace Text which is simply written to the trace file.
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
**	21-Apr-06 (gordy)
**	    Created.
**	 3-Jul-06 (gordy)
**	    Support dedicated DBMS trace log.
*/

protected void
readInfo()
    throws SQLException
{
    while( msg.moreData() )
    {
	short   param_id = msg.readShort();
	short   param_len = msg.readShort();

	switch( param_id )
	{
	case MSG_IP_TRACE :
	{
	    if ( trace.enabled()  ||  conn.dbms_log.enabled() )  
	    {
		String txt = msg.readString( param_len );
		trace.log( "DBMS TRACE: " + txt );
		conn.dbms_log.write( txt );
	    }
	    else
		msg.skip( param_len );
	    break;
	}

	default :
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": Invalid info param ID " + param_id );
	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	}
    }

    return;
} // readInfo


/*
** Name: readError
**
** Description:
**	Read an ERROR message.  Warnings and User messages are saved
**	on the warnings list and NULL is returned.  Errors are returned 
**	as SqlEx objects.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	SqlEx	Error or NULL.
**
** History:
**	31-Oct-02 (gordy)
**	    Extracted from readResults().
**	20-Jul-06 (gordy)
**	    Added support for XA errors.
**	02-Feb-09 (rajus01) SIR 121238
**	    Produce the correct SQLException based on the sqlState.
*/

protected SQLException
readError()
    throws SQLException
{
    SQLException   ex = null;
    int	    status = msg.readInt();
    String  sqlState = msg.readString( 5 );
    byte    type = msg.readByte();
    String  message = msg.readString();

    switch( type )
    {
    case MSG_ET_ERR :
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": Received error '" + sqlState + "' 0x" + 
			 Integer.toHexString( status ) + " -- " + message );

	ex = SqlExFactory.get( message, sqlState, status );
	break;

    case MSG_ET_WRN :
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": Received warning '" + sqlState + "' 0x" + 
			 Integer.toHexString( status ) + " -- " + message );

	setWarning( new SqlWarn( message, sqlState, status ) );
	break;

    case MSG_ET_MSG :
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ": Received message '" + sqlState + "' 0x" + 
			 Integer.toHexString( status ) + " -- " + message );

	setWarning( new SqlWarn( message, sqlState, status ) );
	break;

    case MSG_ET_XA :
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ": Received XA error " + status );

	ex = new XaEx( status );
	break;
    }

    return( ex );
} // readError


/*
** Name: readResult
**
** Description:
**	Read a RESULT message.  The RESULT message parameters are read 
**	and saved in instance fields.
**
**	Returning a value of TRUE will interrupt the processing of 
**	server messages by readResults() and cause an immediate return.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	True for interrupt, False to continue.
**
** History:
**	31-Oct-02 (gordy)
**	    Extracted from readResults().
**	21-Feb-03 (gordy)
**	    Added table and object keys.
**	20-Aug-03 (gordy)
**	    Consolidated result flags into single parameter.
**	 6-Oct-03 (gordy)
**	    Renamed and changed return type to reflect ability
**	    for intermediate RESULT messages to be processed.
**	 6-Apr-07 (gordy)
**	    Replaced result indicators with flags.  Added row
**	    status and position results.
*/

protected boolean
readResult()
    throws SQLException
{
    while( msg.moreData() )
    {
	short   param_id = msg.readShort();
	short   param_len = msg.readShort();

	switch( param_id )
	{
	case MSG_RP_ROWCOUNT :
	    switch( param_len )
	    {
	    case 1 :  rslt_val_rowcnt = msg.readByte();  break;
	    case 2 :  rslt_val_rowcnt = msg.readShort(); break;
	    case 4 :  rslt_val_rowcnt = msg.readInt();   break;

	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid row count length: " + 
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    
	    rslt_items |= RSLT_ROW_CNT;
	    break;

	case MSG_RP_XACT_END :
	    if ( param_len > 0 )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid XACT param length: " + 
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }

	    rslt_flags |= MSG_RF_XACT_END;
	    conn.endXact();
	    break;

	case MSG_RP_STMT_ID :
	    if ( param_len != 8 )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid stmt ID length: " + 
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }

	    rslt_val_stmt = (((long)msg.readInt()) << 32) |
			    (((long)msg.readInt()) & 0xffffffff);
	    rslt_items |= RSLT_STMT_ID;
	    break;

	case MSG_RP_FETCH_LIMIT :
	    switch( param_len )
	    {
	    case 1 :  rslt_val_fetch = msg.readByte();  break;
	    case 2 :  rslt_val_fetch = msg.readShort(); break;
	    case 4 :  rslt_val_fetch = msg.readInt();   break;

	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid fetch limit length: " +
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    
	    rslt_items |= RSLT_PREFETCH;
	    break;

	case MSG_RP_EOD :
	    if ( param_len != 0 )
	    {
		trace.write( tr_id + ": Invalid EOD length: " + 
			     param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    
	    rslt_flags |= MSG_RF_EOD;
	    break;

	case MSG_RP_PROC_RESULT :
	    switch( param_len )
	    {
	    case 1 : setProcResult( (int)msg.readByte() );	break;
	    case 2 : setProcResult( (int)msg.readShort() );	break;
	    case 4 : setProcResult( msg.readInt() );		break;

	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid proc result length: " +
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    break;

	case MSG_RP_READ_ONLY :
	    if ( param_len != 0 )
	    {
		trace.write( tr_id + ": Invalid READ_ONLY length: " + 
			     param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    
	    rslt_flags |= MSG_RF_READ_ONLY;
	    break;
	    
	case MSG_RP_TBLKEY :
	    if ( param_len != MSG_RPV_TBLKEY_LEN )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid table key length: " +
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    
	    if (rslt_val_tblkey == null) rslt_val_tblkey = new byte[param_len];
	    msg.readBytes( rslt_val_tblkey, 0, param_len );
	    rslt_items |= RSLT_TBLKEY;
	    break;

	case MSG_RP_OBJKEY :
	    if ( param_len != MSG_RPV_OBJKEY_LEN )
	    {
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid object key length: " +
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    
	    if (rslt_val_objkey == null) rslt_val_objkey = new byte[param_len];
	    msg.readBytes( rslt_val_objkey, 0, param_len );
	    rslt_items |= RSLT_OBJKEY;
	    break;

	case MSG_RP_FLAGS :
	{
	    int flags;
	    
	    switch( param_len )
	    {
	    case 1 : flags = (int)msg.readByte();	break;
	    case 2 : flags = (int)msg.readShort();	break;
	    case 4 : flags = msg.readInt();		break;

	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid result flags length: " +
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    
	    rslt_flags |= flags;
	    if ( (flags & MSG_RF_XACT_END) != 0 )  conn.endXact();	    
	    break;
	}

	case MSG_RP_ROW_STATUS :
	    switch( param_len )
	    {
	    case 1 :  rslt_val_rowstat = msg.readByte();  break;
	    case 2 :  rslt_val_rowstat = msg.readShort(); break;
	    case 4 :  rslt_val_rowstat = msg.readInt();   break;

	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid row status length: " + 
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    
	    rslt_items |= RSLT_ROW_STAT;
	    break;

	case MSG_RP_ROW_POS :
	    switch( param_len )
	    {
	    case 1 :  rslt_val_rowpos = msg.readByte();  break;
	    case 2 :  rslt_val_rowpos = msg.readShort(); break;
	    case 4 :  rslt_val_rowpos = msg.readInt();   break;

	    default :
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ": Invalid row position length: " + 
				 param_len );
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	    
	    rslt_items |= RSLT_ROW_POS;
	    break;

	default :
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": Invalid result param ID " + 
			     param_id );
	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	}
    }

    return( false );
} // readResult


/*
** Name: setProcResult
**
** Description:
**	Receive procedure return value.  The default action is to save
**	the return value in the instance member rslt_val_proc and set 
**	the result indication flag.  A sub-class should override this 
**	method to provide alternate handling of procedure results.
**
** Input:
**	value	Procedure return value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**      29-Aug-01 (loera01) SIR 105641
**	    Created.
**	 6-Apr-07 (gordy)
**	    Replaced result indicators with flags.
*/

protected void
setProcResult( int value )
    throws SQLException
{
    this.rslt_val_proc = value;
    rslt_items |= RSLT_PROC_VAL;
    return;
} // setProcResult


} // class DrvObj
