/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.dam;

/*
** Name: MsgIo.java
**
** Description:
**	Defines base class representing a connection with a Data
**	Access Server.
**
**	The Data Access Messaging (DAM) protocol classes present 
**	a single unified interface, through inheritance, for access 
**	to a Data Access Server.  They are divided into separate 
**	classes to group related functionality for easier management.  
**	The order of inheritance is determined by the initial actions
**	required during initialization of the connection:
**
**	    MsgIo	Establish socket connection.
**	    MsgOut	Send TL Connection Request packet.
**	    MsgIn	Receive TL Connection Confirm packet.
**	    MsgConn	General initialization.
**
**  Classes
**
**	MsgIo
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Extracted from DbConn.java.
**	21-Apr-00 (gordy)
**	    Moved to package io.
**	28-Mar-01 (gordy)
**	    Separated tracing interface and implementating.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
**	 1-Oct-02 (gordy)
**	    Moved to GCF dam package.  Renamed as DAM Message
**	    Layer implementation class.
**	29-Oct-02 (wansh01)
**	    Added public method isLocal to determine if connection is local.	
**	31-Oct-02 (gordy)
**	    Renamed GCF error codes.
**	20-Dec-02 (gordy)
**	    Track messaging protocol level.
**	15-Jan-03 (gordy)
**	    Added local/remote host name/address methods.
**	26-Feb-03 (gordy)
**	    Added getCharSet().
**	 3-May-05 (gordy)
**	    Added public abort() to perform same action (at this level)
**	    as protected disconnect().
**	 3-Jul-06 (gordy)
**	    Allow character-set to be overridden.
**	19-Oct-06 (lunbr01)  Sir 116548
**	    Allow for IPv6 addresses enclosed in square brackets '[]'.
**	    Try connecting to ALL IP @s for target host rather than just 1st.
**	 5-Dec-07 (gordy)
**	    Adding support for multiple server targets.
**	 2-Apr-08 (gordy)
**	    Support extended subports in symbolic port ID.
**      19-Nov-08 (rajus01) SIR 121238
**          Replaced SqlEx references with SQLException or SqlExFactory
**          depending upon the usage of it. SqlEx becomes obsolete to support
**          JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	25-Mar-10 (gordy)
**	    Added batch message.
**	19-May-10 (gordy)
**	    Enable TCP-IP NoDelay option.  The driver does its own
**	    buffering and only flushes buffers when full or the
**	    message group is complete.  With NoDelay disabled, there
**	    is a noticable impact on batch processing when the batch
**	    fills the first buffer and overflows just a few bytes
**	    into the next buffer.  Under this send-send condition,
**	    TCP will delay sending the second buffer in case there 
**	    is more data to be sent.  Enabling NoDelay will eliminate 
**	    this (unnecessary) delay.
*/

import	java.net.InetAddress;
import	java.net.Socket;
import	java.util.Random;
import	java.sql.SQLException;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.CharSet;
import	com.ingres.gcf.util.IdMap;
import	com.ingres.gcf.util.Trace;
import	com.ingres.gcf.util.Tracing;
import	com.ingres.gcf.util.TraceLog;


/*
** Name: MsgIo
**
** Description:
**	Class representing a connection to a Data Access Server.
**	Provides functionality for establishing and dropping the 
**	connection.
**
**  Constants
**
**	DAM_ML_TRACE_ID	Message Layer trace ID.
**
**  Public Methods:
**
**	abort			Abort network connection.
**	connID			Return connection identifier.
**	isClosed		Is the connection closed?
**	isLocal			Is the connection local?
**	getRemoteHost		Get server machine name.
**	getRemoteAddr		Get server network address.
**	getLocalHost		Get local machine name.
**	getLocalAddr		Get local network address.
**	setProtocolLevel	Set Messaging Layer protocol level.
**	getCharSet		Get character-set used for connection.
**	setCharSet		Set character-set used for connection.
**
**  Protected Data:
**
**	socket			Network socket used for the connection.
**	char_set		Character encoding.
**	msg_proto_lvl		Message Layer protocol level.
**	tl_proto_lvl		Transport Layer protocol level.
**	title			Class title for tracing.
**	trace			Trace output.
**	msgMap			Mapping table for message ID.
**
**  Protected Methods:
**
**	connect			Connect to target server.
**	disconnect		Shutdown the physical connection.
**
**  Private Data:
**
**	conn_id			Connection ID.
**	connections		Static counter for generating connection IDs.
**
**  Private Methods:
**
**	close			Close socket.
**	translatePortID		Translate port ID to port number.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Added character set/encoding.
**	17-Nov-99 (gordy)
**	    Extracted base level functionality from DbConn.
**	 6-Sep-02 (gordy)
**	    Character encoding now encapsulated in CharSet class.
**	 1-Oct-02 (gordy)
**	    Renamed as DAM-ML messaging class.  Added DAM-ML trace ID.
**	    Moved tl_proto, char_set and finalize() to sub-class.
**	29-Oct-02 (wansh01)
**	    Added public method isLocal to determine if connection is local.
**	20-Dec-02 (gordy)
**	    Track messaging protocol level.  Added msg_proto_lvl and 
**	    setProtocolLevel().  Restored tl_proto_lvl and char_set to
**	    make this class the central repository for all data within
**	    the messaging classes.
**	15-Jan-03 (gordy)
**	    Added getRemoteHost(), getRemoteAddr(), getLocalHost(), and
**	    getLocalAddr().
**	26-Feb-03 (gordy)
**	    Added getCharSet().
**	 3-May-05 (gordy)
**	    Extracted disconnect() functionality to private method close()
**	    so that it may be shared.  Added public method abort() which
**	    calls close() to drop the network connection.
**	 3-Jul-06 (gordy)
**	    Added setCharSet().
**	 5-Dec-07 (gordy)
**	    Extract connection functionality from constructor to connect().
**	    Moved target parsing methods getHost() and getPort() to MsgConn.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Moved map tables from public interface to private fields.
**	25-Mar-10 (gordy)
**	    Added batch message.
*/

class
MsgIo
    implements GcfErr, TlConst, MsgConst
{

    /*
    ** Constants
    */
    public static final	String	DAM_ML_TRACE_ID	= "msg";    // DAM-ML trace ID.

    /*
    ** The network connection.
    */
    protected Socket	socket		= null;
    protected CharSet	char_set	= null;
    
    /*
    ** Protocol must be assumed to be lowest level
    ** until a higher level is negotiated.
    */
    protected byte	msg_proto_lvl	= MSG_PROTO_1;
    protected byte	tl_proto_lvl	= DAM_TL_PROTO_1;

    /*
    ** Tracing.
    */
    protected String	title		= null;	// Title for tracing
    protected Tracing	trace;			// Tracing.

    protected static IdMap	msgMap[] =
    {
	new IdMap( MSG_CONNECT,	MSG_STR_CONNECT ),
	new IdMap( MSG_DISCONN,	MSG_STR_DISCONN ),
	new IdMap( MSG_XACT,	MSG_STR_XACT ),
	new IdMap( MSG_QUERY,	MSG_STR_QUERY ),
	new IdMap( MSG_CURSOR,	MSG_STR_CURSOR ),
	new IdMap( MSG_DESC,	MSG_STR_DESC ),
	new IdMap( MSG_DATA,	MSG_STR_DATA ),
	new IdMap( MSG_ERROR,	MSG_STR_ERROR ),
	new IdMap( MSG_RESULT,	MSG_STR_RESULT ),
	new IdMap( MSG_REQUEST,	MSG_STR_REQUEST ),
	new IdMap( MSG_INFO,	MSG_STR_INFO ),
	new IdMap( MSG_BATCH,	MSG_STR_BATCH ),
    };

    /*
    ** Instance ID and connection count.
    */
    private int		conn_id		= -1;	// connection ID
    private static int	connections	= 0;	// Number of connections


/*
** Name: MsgIo
**
** Description:
**	Class constructor.
**
** Input:
**	log		Trace log.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Extracted base functionality from DbConn.
**	28-Mar-01 (gordy)
**	    Separated tracing interface and implementation.
**	 1-Oct-02 (gordy)
**	    Renamed as DAM Message Layer.  Added trace log parameter.
**	 5-Dec-07 (gordy)
**	    Extracted connect() functionality.
*/

protected
MsgIo( TraceLog log )
    throws SQLException
{
    conn_id = connections++;
    trace = new Tracing( log, DAM_ML_TRACE_ID );
    title = "MsgIo[" + conn_id + "]";
} // MsgIo


/*
** Name: connect
**
** Description:
**	Opens a socket connection to target host.
**
** Input:
**	host		Host name or address.
**	portID		Symbolic or numeric port ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	19-Oct-06 (lunbr01)  Sir 116548
**	    Try connecting to ALL IP @s for target host rather than just 1st.
**	 5-Dec-07 (gordy)
**	    Extracted from constructor.
**	19-May-10 (gordy)
**	    Enable the TCP NoDelay option to avoid a delay in sending
**	    the overflow buffer when a message is slightly longer than
**	    what fits in the current buffer.
*/

protected void
connect( String host, String portID )
    throws SQLException
{
    InetAddress	addr[];
    Exception	ex_last = null;

    if ( trace.enabled( 2 ) )  
	trace.write( title + ": opening network connection '" + 
			     host + "', '" + portID + "'" );

    try { addr = InetAddress.getAllByName( host ); }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": Error resolving host address - " + 
				 ex.getMessage() );
	throw SqlExFactory.get( ERR_GC4001_CONNECT_ERR, ex );
    }

    int port = translatePortID( portID );

    for( int i = 0; i < addr.length; i++ )
    {
	String hostAddr = addr[i].getHostAddress();

	if ( trace.enabled( 5 )  &&  ! host.equalsIgnoreCase( hostAddr ) )
	    trace.write( title + ": " + host + " => " + hostAddr );

	try
	{
	    socket = new Socket( addr[i], port );
	    socket.setTcpNoDelay( true );

	    if ( trace.enabled( 3 ) )
		trace.write(title + ": connected to " + hostAddr + "," + port);
	    return;
	}
	catch( Exception ex )
	{
	    if ( trace.enabled( 1 ) )  
		trace.write( title + ": connection failed to " + hostAddr + 
				     "," + port + " - " + ex.getMessage() );
	    ex_last = ex;
	}
    }

    trace.write( title + ": all addresses have failed" );
    throw SqlExFactory.get( ERR_GC4001_CONNECT_ERR, ex_last );
} // connect


/*
** Name: close
**
** Description:
**	Close socket used for network connection.
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
**	 3-May-05 (gordy)
**	    Created.
*/

private void
close()
{
    if ( socket != null )
    {
	if ( trace.enabled( 2 ) )  
	    trace.write( title + ": closing network connection" );

	try { socket.close(); }
	catch( Exception ignore ) {}
	finally { socket = null; }
    }

    return;
} // close


/*
** Name: disconnect
**
** Description:
**	Disconnect from server and free all I/O resources.
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
**	 7-Jun-99 (gordy)
**	    Created.
*/

protected void
disconnect()
{
    close();
    return;
} // disconnect


/*
** Name: abort
**
** Description:
**	Abort the network connection.
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
**	 3-May-05 (gordy)
**	    Created.
*/

public void
abort()
{
    close();
    return;
}


/*
** Name: isClosed
**
** Description:
**	Returns an indication that the connection is closed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if connection is closed, False otherwise.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public boolean
isClosed()
{
    return( socket == null );
} // isClosed


/*
** Name: isLocal
**
** Description:
**	Returns an indication that the connection is local.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if connection is local, False otherwise.
**
** History:
**	 29-Oct-02 (wansh01)
**	    Created.
*/

public boolean
isLocal()
{
    return( (socket == null) ? false : 
	    socket.getInetAddress().equals( socket.getLocalAddress() ) );
} // isLocal


/*
** Name: connID
**
** Description:
**	Returns the numeric connection identifier.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Connection identifier.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

public int
connID()
{
    return( conn_id );
} // connID


/*
** Name: getRemoteHost
**
** Description:
**	Returns the host name of the server machine.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Server host name or NULL if error.
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

public String
getRemoteHost()
{
    String host;

    try { host = socket.getInetAddress().getHostName(); }
    catch( Exception ex ) { host = null; }

    return( host );
} // getRemoteHost


/*
** Name: getRemoteAddr
**
** Description:
**	Returns the network address of the server machine.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Server network address or NULL if error.
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

public String
getRemoteAddr()
{
    return( socket.getInetAddress().getHostAddress() );
} // getRemoteAddr


/*
** Name: getLocalHost
**
** Description:
**	Returns the host name of the local machine.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Local host name or NULL if error.
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

public String
getLocalHost()
{
    String host;

    try { host = InetAddress.getLocalHost().getHostName(); }
    catch( Exception ex ) { host = null; }

    return( host );
} // getLocalHost


/*
** Name: getLocalAddr
**
** Description:
**	Returns the network address of the local machine.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Local network address or NULL if error.
**
** History:
**	15-Jan-03 (gordy)
**	    Created.
*/

public String
getLocalAddr()
{
    String host;

    try { host = InetAddress.getLocalHost().getHostAddress(); }
    catch( Exception ex ) { host = null; }

    return( host );
} // getLocalAddr


/*
** Name: setProtocolLevel
**
** Description:
**	Set the Messaging Layer protocol level.  This level is used
**	to build message packets.  Message content must still be
**	controlled by the class user's.
**
** Input:
**	msg_proto_lvl	Message Layer protocol level.
**
** Ouput:
**	None.
**
** Returns:
**	byte	    Previous protocol level.
**
** History:
**	20-Dec-02 (gordy)
**	    Created.
*/

public byte
setProtocolLevel( byte msg_proto_lvl )
{
    byte prev = this.msg_proto_lvl;
    this.msg_proto_lvl = msg_proto_lvl;
    return( prev );
} // setProtocolLevel


/*
** Name: getCharSet
**
** Description:
**	Returns the CharSet used to encode strings for the connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	CharSet	    Connection CharSet.
**
** History:
**	26-Feb-03 (gordy)
**	    Created.
*/

public CharSet
getCharSet()
{
    return( char_set );
} // getCharSet


/*
** Name: setCharSet
**
** Description:
**	Set the CharSet used to encode strings for the connection.
**
** Input:
**	char_set	CharSet to be used.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jul-06 (gordy)
**	    Created.
*/

public void
setCharSet( CharSet char_set )
{
    this.char_set = char_set;
    return;
} // setCharSet


/*
** Name: translatePortID
**
** Description:
**	Translates a symbolic or numeric port ID to a port number.
**	A negative value is returned if an error occurs.
**
** Input:
**	port	Port ID.
**
** Ouptut:
**	None.
**
** Returns:
**	int	Port number.
**
** History:
**	 1-Oct-02 (gordy)
**	    Extracted from getPort().
**	 5-Dec-07 (gordy)
**	    Throws exception if symbolic port ID cannot be translated
**	    into numeric value.
**	 2-Apr-08 (gordy)
**	    Support extended subports.
*/

private int
translatePortID( String port )
    throws SQLException
{
    int		portnum;
    int		subport = 0;
    char	c0, c1;

    /*
    ** Check for Ingres Installation ID format: [a-zA-Z][a-zA-Z0-9]{n}
    ** where n is [0-9] | 0[0-9] | 1[0-5]
    */
    switch( port.length() )
    {
    case 4 :
	/*
	** Check most significant digit of subport.
	*/
	if ( ! Character.isDigit( (c0 = port.charAt( 2 )) ) )
	    break;	/* Not a symbolic port ID */

	/*
	** Save most significant digit so that it may be 
	** combined with least significant digit below.
	*/
	subport = Character.digit( c0, 10 ) * 10;

	/*
	** Fall through to process least significant digit...
	*/

    case 3 :
	/*
	** Check least significant digit of subport
	** (Optional most significant digit may be
	** present and was processed above).
	*/
	if ( ! Character.isDigit( (c0 = port.charAt( port.length() - 1 )) ) )
	    break;	/* Not a symbolic port ID */

	/*
	** Combine least significant digit with optional
	** most significant digit processed in prior case
	** and check for valid range.
	*/
	subport += Character.digit( c0, 10 );
	if ( subport > 15 )  break;	/* Not a symbolic port ID */

	/*
	** Fall through to process instance ID...
	*/

    case 2 :
	/*
	** Check for valid instance ID.
	*/
	if ( ! (Character.isLetter( (c0 = port.charAt( 0 )) )  &&  
		Character.isLetterOrDigit( (c1 = port.charAt( 1 )) )) )
	    break;	/* Not a symbolic port ID */

	/*
	** Map symbolic port ID to numeric port.
	** Subport was generated in prior cases 
	** or defaulted to 0.
	*/
	c0 = Character.toUpperCase( c0 );
	c1 = Character.toUpperCase( c1 );

	portnum = (((subport > 0x07) ? 0x02 : 0x01) << 14)
		    | ((c0 & 0x1f) << 9)
		    | ((c1 & 0x3f) << 3)
		    | (subport & 0x07);

	if ( trace.enabled( 5 ) )  
	    trace.write( title + ": " + port + " => " + portnum );

	return( portnum );
    }

    try { portnum = Integer.parseInt( port ); }
    catch( Exception ex ) 
    {
	if ( trace.enabled( 1 ) )
	    trace.write( title + ": invalid port ID '" + port + "'" );
	throw SqlExFactory.get( ERR_GC4000_BAD_URL );
    }

    return( portnum );
} // translatePortID


} // class MsgIo

