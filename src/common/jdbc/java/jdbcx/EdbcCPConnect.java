/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcCPConnect.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcCPConnect.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/


import	java.util.Vector;
import	java.sql.Connection;
import	java.sql.SQLException;
import	javax.sql.PooledConnection;
import	javax.sql.ConnectionEvent;
import	javax.sql.ConnectionEventListener;
import	ca.edbc.jdbc.EdbcTrace;
import	ca.edbc.util.EdbcEx;


/*
** Name: EdbcCPConnect
**
** Description:
**	EDBC JDBC extension class which implements the JDBC 
**	PooledConnection interface.
**
**  Interface Methods:
**
**	getConnection			Generate a virtual connection.
**	close				Close physical connection.
**	addConnectionEventListener	Register event listener.
**	removeCOnnectionEventListener	Deregister event listener.
**	connectionClosed		Signal connection closed event.
**	connectionErrorOccured		Signal connection error event.
**
**  Protected Data
**
**	physConn			Physical connection.
**	virtConn			Virtual connection.
**	trace				DataSource tracing.
**	title				Class tracing title.
**	tr_id				Class tracing ID.
**	inst_id				Instance ID.
**
**  Protected Methods:
**
**	clearConnection			Close the virtual connection.
**
**  Private Data
**
**	inst_count			Number of class instances.
**	listeners			Registered connection event listeners.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

class
EdbcCPConnect
    implements	PooledConnection, ConnectionEventListener, EdbcDSErr
{


    protected Connection	physConn = null;
    protected EdbcCPVirtConn	virtConn = null;

    protected EdbcTrace		trace = null;
    protected String		title = null;
    protected String		tr_id = null;
    protected int		inst_id = 0;

    private static int		inst_count = 0;
    private Vector		listeners = new Vector();


/*
** Name: EdbcCPConnect
**
** Description:
**	Class constructor.
**
** Input:
**	conn	Connection.
**	trace	Connection tracing.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public
EdbcCPConnect( Connection conn, EdbcTrace trace ) 
{
    physConn = conn;
    this.trace = trace;
    inst_id = inst_count++;
    title = "EDBC-PoolConnection[" + inst_id + "]";
    tr_id = "CPConn[" + inst_id + "]";
} // EdbcCPConnect


/*
** Name: toString
**
** Description:
**	Returns a string identifier for this instance.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	    Instance identfier.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public String
toString()
{
    return( title );
} // toString


/*
** Name: getConnection
**
** Description:
**	Generate a virtual connection encapsulating the
**	associated physical connection which provides
**	event notification for connection errors and
**	connection close requests.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Connection  A connection.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public Connection
getConnection()
    throws SQLException
{
    if ( physConn == null )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getConnection(): connection closed!" );
	throw EdbcEx.get( E_JD0004_CONNECTION_CLOSED );
    }

    if ( trace.enabled() )  trace.log( title + ".getConnection()" );

    clearConnection();
    virtConn = new EdbcCPVirtConn( physConn, this, this, trace );
    
    if (trace.enabled())  trace.log(title + ".getConnection(): " + virtConn);
    return( virtConn );
} // getConnection


/*
** Name: close
**
** Description:
**	Close the physical connection.
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
close()
    throws SQLException
{
    if ( physConn == null )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".close(): connection closed! " );
    }
    else
    {
	if ( trace.enabled() )  trace.log( title + ".close()" );
	clearConnection();
	physConn.close();
	physConn = null;
    }
    return;
} // close


/*
** Name: addConnectionEventListener
**
** Description:
**	Register a listener to be notified of connection events.
**
** Input:
**	listener    Listener to register.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
addConnectionEventListener( ConnectionEventListener listener )
{
    listeners.addElement( listener );
    return;
} // addConnectionEventListener


/*
** Name: removeConnectionEventListener
**
** Description:
**	Deregister a listener registered for connection events.
**
** Input:
**	listener    Listener to deregister.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
removeConnectionEventListener( ConnectionEventListener listener )
{
    listeners.removeElement( listener );
    return;
} // removeConnectionEventListener


/*
** Name: connectionClosed
**
** Description:
**	Event listener method for connection close requests.  
**	This class acts as a ConnectionEventListener for the 
**	EdbcConnection class and passes any events received 
**	to all registered listeners.
**
** Input:
**	event	    Connection event.
**
** Ouptut:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
connectionClosed( ConnectionEvent event )
{
    if ( virtConn != null )  
    {
	virtConn = null;

	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ": signalling connection closed" );

	for( int i = 0; i < listeners.size(); i++ )  
	{
	    ConnectionEventListener listener =
		(ConnectionEventListener)listeners.elementAt(i);
	    if ( listener != null )  listener.connectionClosed( event );
	}
    }

    return;
} // connectionClosed


/*
** Name: connectionErrorOccured
**
** Description:
**	Event listener method for connection errors.  This class
**	acts as a ConnectionEventListener for the EdbcConnection
**	class and passes any events received to all registered
**	listeners.
**
** Input:
**	event	    Connection event.
**
** Ouptut:
**	None.
**
** Returns:
**	void.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
connectionErrorOccurred( ConnectionEvent event )
{
    if ( virtConn != null )  
    {
	virtConn = null;

	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ": signalling connection error" );

	for( int i = 0; i < listeners.size(); i++ )  
	{
	    ConnectionEventListener listener =
		(ConnectionEventListener)listeners.elementAt(i);
	    if ( listener != null )  listener.connectionErrorOccurred( event );
	}
    }

    return;
} // connectionErrorOccurred


/*
** Name: clearConnection
**
** Description:
**	Any active virtual connection is closed.
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
**	29-Jan-01 (gordy)
**	    Created.
*/

protected void
clearConnection()
    throws SQLException
{
    /*
    ** No connection event should be signalled for
    ** this request, so set our reference to NULL
    ** so the event handler will not forward the
    ** resulting connection event.
    */
    EdbcCPVirtConn conn = virtConn;
    virtConn = null;
    if ( conn != null )  
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ": forcing virtual connection closed" );
	conn.close();
    }
    return;
} // clearConnection


} // class EdbcCPConnect
