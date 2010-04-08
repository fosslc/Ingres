/*
** Copyright (c) 2001, 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcCPConn.java
**
** Description:
**	Defines class which implements the JDBC PooledConnection interface.
**
**  Classes
**
**	JdbcCPConn
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      24-Nov-08 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to support
**            JDBC 4.0 SQLException hierarchy.
*/


import	java.util.Vector;
import	java.sql.Connection;
import	java.sql.SQLException;
import	javax.sql.PooledConnection;
import	javax.sql.ConnectionEvent;
import	javax.sql.ConnectionEventListener;
import	javax.sql.StatementEventListener;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;


/*
** Name: JdbcCPConn
**
** Description:
**	JDBC driver base class which implements the JDBC PooledConnection 
**	interface.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
*/

public class
JdbcCPConn
    implements	PooledConnection, ConnectionEventListener, GcfErr
{


    /*
    ** A pooled connection represents a single physical connection
    ** and a set of virtual connections which interface between the
    ** application and the physical connection.  Only a single
    ** virtual connection may be active at any one time.  All other
    ** virtual connections must be in the closed state.
    */
    protected	JdbcConn	physConn = null;
    protected	JdbcCPVirt	virtConn = null;

    /*
    ** DataSource tracing.
    */
    protected	DrvTrace	trace = null;	
    protected	String		title = null;
    protected	String		tr_id = "CPConn";
    protected	int		inst_id = 0;

    /*
    ** List of registered event listeners.
    */
    private	Vector		listeners = new Vector();
    
    private static int		inst_count = 0;		// Instance counter


/*
** Name: JdbcCPConn
**
** Description:
**	Class constructor.
**
** Input:
**	conn	Connection.
**	trace	DataSource tracing.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcCPConn( JdbcConn conn, DrvTrace trace ) 
{
    physConn = conn;
    this.trace = trace;
    inst_id = inst_count++;
    title = trace.getTraceName() + "-PooledConnection[" + inst_id + "]";
    tr_id += "[" + inst_id + "]";
    return;
} // JdbcCPConn


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
	throw SqlExFactory.get( ERR_GC4004_CONNECTION_CLOSED );
    }

    if ( trace.enabled() )  trace.log( title + ".getConnection()" );

    clearConnection();
    virtConn = new JdbcCPVirt( physConn, this, this, trace );
    
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
**	JdbcCPVirt class and passes any events received to
**	all registered listeners.
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
**	acts as a ConnectionEventListener for the JdbcCPVirt class 
**	and passes any events received to all registered listeners.
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
    JdbcCPVirt conn = virtConn;
    virtConn = null;
    
    if ( conn != null )  
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ": forcing virtual connection closed" );
	conn.close();
    }
    return;
} // clearConnection

/*
** Name: addStatementEventListener
**
** Description:
**	Registers a StatementEventListener with this PooledConnection object. 
**
** Input:
**	listener  A component which implements the StatementEventListener 
**		  interface that is to be registered with this 
**		  PooledConnection object.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	24-Nov-08 (rajus01)
**	    Created.
**	06-May-09 (rajus01) SIR 121238
**	    Implemented.
*/
public void 
addStatementEventListener(StatementEventListener listener) 
{
    return;   // Not supported
}

/*
** Name: removeStatementEventListener
**
** Description:
**	Removes the specified StatementEventListener from the list of 
**	components that will be notified when the driver detects that a 
**	PreparedStatement has been closed or is invalid. 
**
** Input:
**	listener  A component which implements the StatementEventListener 
**		  interface that was previously registered with this 
**		  PooledConnection object.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	24-Nov-08 (rajus01)
**	    Created.
**	06-May-09 (rajus01) SIR 121238
**	    Implemented.
*/
public void 
removeStatementEventListener(StatementEventListener listener)
{
    return;   // Not supported
}

} // class JdbcCPConn
