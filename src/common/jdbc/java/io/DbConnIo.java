/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.io;

/*
** Name: DbConnIo.java
**
** Description:
**	EDBC Driver class DbConnIo representing a connection
**	with the JDBC (and DBMS) server.  
**
**	The DbConn* classes present a single unified class,
**	through inheritance from DbConnIo to DbConn, for
**	access to the JDBC server.  They are divided into
**	separate classes to group related functionality for
**	ease in management.
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
*/

import	java.net.InetAddress;
import	java.net.Socket;
import	ca.edbc.util.CharSet;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.util.Trace;
import	ca.edbc.util.Tracing;


/*
** Name: DbConnIo
**
** Description:
**	Ingres JDBC Driver class representing a database connection.
**	This is the base class for the DbConn* group of classes.
**	Provides functionality for establishing and dropping the 
**	connection.
**
**  Public Methods:
**
**	connID		Return connection identifier.
**	isClosed	Is the connection closed?
**
**  Protected Data:
**
**	title		Class title for tracing.
**	conn_id		Connection ID.
**	socket		Network socket used for the connection.
**	char_set	Character encoding.
**	tl_proto	Connection protocol level.
**
**  Protected Methods:
**
**	disconnect	Disconnect from the server.
**
**  Private Data:
**
**	connections	Static counter for connection ID.
**
**  Private Methods:
**
**	getHost		Extract host name from connection target.
**	getPort		Extract port from connection target.
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
*/

class
DbConnIo
    implements IoConst, IoErr
{

    private static int	connections = 0;	// Number of connections

    protected String	title = null;		// Title for tracing
    protected Trace	trace;			// Tracing.
    protected int	conn_id = -1;		// connection ID
    protected Socket	socket = null;

    /*
    ** The following are not used by this class,
    ** but are provided for the use by all sub-classes.
    **
    ** Protocol must be assumed to be lowest level
    ** until a higher level is negotiated.
    */
    protected byte	tl_proto = JDBC_TL_PROTO_1;
    protected CharSet	char_set = null;


/*
** Name: DbConnIo
**
** Description:
**	Class constructor.  Opens a socket connection to target host.
**
** Input:
**	host_id		Hostname and port.
**
** Output:
**	None.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	17-Nov-99 (gordy)
**	    Extracted base functionality from DbConn.
**	28-Mar-01 (gordy)
**	    Separated tracing interface and implementating.
*/

protected
DbConnIo( String host_id )
    throws EdbcEx
{
    String  host;
    int	    port;
    
    conn_id = connections++;
    title = "DbConn[" + conn_id + "]";
    trace = new Tracing( "IO" );
    host = getHost( host_id );
    port = getPort( host_id );

    if ( trace.enabled( 2 ) )  
	trace.write( title + ": connecting to " + host + ":" + port );

    try
    {
	InetAddress addr = InetAddress.getByName( host );
	if ( trace.enabled( 3 ) )  trace.write(title + ": host address " + addr);
	socket = new Socket( addr, port );
    }
    catch( Exception ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( title + ": error connect to host - " + ex.getMessage() );
	throw EdbcEx.get( E_IO0001_CONNECT_ERR, ex );
    }
} // DbConnIo


/*  
** Name: finalize
**
** Description:
**	Class destructor.  Close connection if still open.
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
finalize() 
    throws Throwable 
{
    disconnect();
    super.finalize();
    return;
} // finalize


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
    if ( socket != null )
    {
	try { socket.close(); }
	catch( Exception ignore ) {}
	    
	socket = null;
	if ( trace.enabled( 2 ) )  trace.write( title + ": disconnected" );
    }

    return;
} // disconnect


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
** Name: getHost
**
** Description:
**	Extracts and returns the name of the host from the target
**	string:
**
**	    <host>[:<port>]
**
** Input:
**	target
**
** Ouptut:
**	None.
**
** Returns:
**	String	    Host name.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

private String
getHost( String target )
{
    int index = target.indexOf( ':' );
    return( index < 0 ? target : target.substring( 0, index ) );
} // getHost


/*
** Name: getPort
**
** Description:
**	Extracts and returns the port number from the target string:
**
**	    <host>:<port>
**
**	The port may be specified as a numeric string or in standard
**	Ingres installation ID format such as II0.
**
** Input:
**	target
**
** Output:
**	None.
**
** Returns:
**	int	    Port number.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

private int
getPort( String target )
    throws EdbcEx
{
    String  port;
    int	    portnum;
    int	    index = target.indexOf( ':' );

    if ( index < 0  ||  target.length() <= ++index )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( title + ":port number missing '" + target + "'" );
	throw EdbcEx.get( E_IO0003_BAD_URL );
    }

    port = target.substring( index );

    /*
    ** Check for Ingres Installation ID format: 
    ** [a-zA-Z][a-zA-Z0-9]{[0-7]}
    */
    if ( port.length() == 2  ||  port.length() == 3 )
    {
	char c0 = port.charAt( 0 );
	char c1 = port.charAt( 1 );
	char c2 = (port.length() == 3) ? port.charAt( 2 ) : '0';

	if ( 
	     Character.isLetter( c0 )  &&  
	     Character.isLetterOrDigit( c1 )  &&
	     Character.isDigit( c2 ) 
	   )
	{
	    c0 = Character.toUpperCase( c0 );
	    c1 = Character.toUpperCase( c1 );

	    return( 1 << 14 | (c0 & 0x1f) << 9 | (c1 & 0x3f) << 3 | (c2 & 0x07) );
	}
    }

    try { portnum = Integer.parseInt( port ); }
    catch( Exception ex ) 
    {
	if ( trace.enabled( 1 ) )
	    trace.write( title + ": port number '" + port + "'" );
	throw EdbcEx.get( E_IO0003_BAD_URL );
    }

    return( portnum );
} // getPort

} // class DbConnIo

