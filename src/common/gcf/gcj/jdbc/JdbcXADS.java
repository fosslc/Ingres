/*
** Copyright (c) 2001, 2006 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcXADS.java
**
** Description:
**	Defines class which implements the JDBC XADataSource interface.
**
**  Classes:
** 
**	JdbcXADS
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Implemented support for recovery.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	15-Jul-03 (gordy)
**	    Enhanced configuration support.  Made serializable
**	10-May-05 (gordy)
**	    Added ability to abort a transaction using an independent
**	    connection.
**	20-Jul-06 (gordy)
**	    Removed XID registration.  All Resource Manager requests
**	    now generate a new connection to perform the operation.
**	31-Aug-06 (gordy)
**	    Restored XID registration for duration of start/end.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
*/

import	java.util.Hashtable;
import	java.util.Properties;
import	java.io.ObjectInputStream;
import	java.io.Serializable;
import	java.io.IOException;
import	java.sql.Connection;
import	java.sql.SQLException;
import	javax.sql.XADataSource;
import	javax.sql.XAConnection;
import	javax.transaction.xa.XAResource;
import	javax.transaction.xa.Xid;
import	javax.transaction.xa.XAException;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.XaEx;
import	com.ingres.gcf.util.XaXid;


/*
** Name: JdbcXADS
**
** Description:
**	JDBC driver class which implements the JDBC XADataSource interface.  
**	This class also implements the XARsrcMgr interface to support XA 
**	requirements beyound the capabilities of JdbcXAConn support for
**	XAResource.
**
**	This class is a subclass of JdbcDS and so may be used as a 
**	DataSource object and supports all the properties of the 
**	JdbcDS class.
**
**	Sub-classes must implement the following load methods defined by 
**	this class to provide driver identity and runtime configuration 
**	information.
**
**	    loadConfig		Configuration properties.
**	    loadDriverName	Full name of driver.
**	    loadProtocolID	Protocol accepted by driver.
**	    loadTraceName	Name of driver for tracing.
**	    loadTraceLog	Tracing log.
**
**  Constants
**
**	driverVendor		Vendor name
**	driverJdbcVersion	Supported JDBC version
**	driverMajorVersion	Internal driver version
**	driverMinorVersion
**
**  Abstract Methods:
**
**	loadConfig		Load config properties from sub-class.
**	loadDriverName		Load driver name from sub-class.
**	loadProtocolID		Load protocol ID from sub-class.
**	loadTraceName		Load tracing name from sub-class.
**	loadTraceLog		Load tracing log from sub-class.
**
**  Interface Methods:
**
**	getXAConnection		Generate a XAConnection object.
**
**	getRMConnection		Generate a Resource Manager connection.
**	registerXID		Register distributed transaction ID.
**	deregisterXID		Deregister distributed transaction ID.
**	startXID		Start distributed transaction.
**	endXID			End association with distributed transaction.
**	prepareXID		Prepare distributed transaction.
**	commitXID		Commit distributed transaction.
**	rollbackXID		Rollback distributed transaction.
**	abortXID		Abort distributed transaction.
**	recoverXID		Retrieve list of distributed transaction IDs.
**
**  Private Data:
**
**	MASTER_DB		Installation master database.
**	regXID			Registry for XID/XAResource.
**
**  Private Methods:
**
**	initialize		Initialize transient state.
**	readObject		Deserialize data source.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Implemented support for recovery: added endXID().
**	31-Oct-02 (gordy)
**	    Genericized and made abstract as base class for productized
**	    drivers.  Removed getReference() method which must be 
**	    implemented by sub-class.  Moved minimal functionality of
**	    find() inline.
**	15-Jul-03 (gordy)
**	    Replaced setDriverIdentity() with load methods and removed
**	    associated fields.  Marked non-property fields as transient
**	    and added initialize() and readObject to support serialization.
**	10-May-05 (gordy)
**	    Added abortXID() to abort transaction independent of a session.
**	20-Jul-06 (gordy)
**	    Removed regXID, registerXID(), deregisterXID(), and endXID().
**	    Added getRMConnection().
**	31-Aug-06 (gordy)
**	    Restored regXID, registerXID(), deregisterXID().  Re-implemented 
**	    endXID() to pass request to registered XAResource. Added startXID().
*/

public abstract class
JdbcXADS
    extends JdbcDS
    implements XADataSource, XARsrcMgr, Serializable
{

    /*
    ** Constants.
    */
    private static final String	MASTER_DB = "iidbdb";	// Master Database

    /*
    ** XID/XAResource Registry.
    */
    private transient Hashtable	regXID = null;


/*
** Name: JdbcXADS
**
** Description:
**	Class constructor.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	15-Jul-03 (gordy)
**	    Moved initialization to separate method to support serialization.
*/

protected
JdbcXADS() 
{
    initialize();	// Initialize transient data.
} // JdbcXADS


/*
** Name: initialize
**
** Description:
**	Initializes the transient data.
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
**	15-Jul-03 (gordy)
**	    Extracted from constructor.
*/

private void
initialize()
{
    title = trace.getTraceName() + "-XADataSource[" + inst_id + "]";
    tr_id = "XADSrc[" + inst_id + "]";
    regXID = new Hashtable();
    return;
} // initialize


/*
** Name: getXAConnection
**
** Description:
**	Generate an XA connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	XAConnection	An XA connection.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public XAConnection
getXAConnection()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getXAConnection()" );
    
    XAConnection conn = 
	new JdbcXAConn( connect( (String)null, (String)null ), 
			this, null, null, trace );
    
    if ( trace.enabled() )  trace.log( title + ".getXAConnection(): " + conn );
    return( conn );
} // getXAConnection


/*
** Name: getXAConnection
**
** Description:
**	Generate an XA connection.
**
** Input:
**	user	    User ID.
**	password    Password.
**
** Output:
**	None.
**
** Returns:
**	XAConnection	An XA connection.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public XAConnection
getXAConnection( String user, String password )
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".getXAConnection('" + user + "')" );

    XAConnection conn = new JdbcXAConn( connect( user, password ), 
					this, user, password, trace );

    if ( trace.enabled() )  trace.log( title + ".getXAConnection(): " + conn );
    return( conn );
} // getXAConnection


/*
** Name: getRMConnection
**
** Description:
**	Generate a Resource Manager connection.
**
** Input:
**	user		User ID.
**	password	Password.
**
** Output:
**	None.
**
** Returns:
**	JdbcConn	JDBC driver connection.
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

public JdbcConn
getRMConnection( String user, String password )
    throws SQLException
{
    return( connect( user, password ) );
} // getRMConnection


/*
** Name: registerXID
**
** Description:
**	Register a distributed transaction ID and the XAResource
**	object which is servicing the XID.  Any subsequent prepare(),
**	commit(), or rollback() request for the registered XID will
**	be forwarded to the servicing XAResource.
**
** Input:
**	rsrc	XA resource.
**	xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Jul-06 (gordy)
**	    Removed.
**	31-Aug-06 (gordy)
**	    Restored.
*/

public void
registerXID( XAResource rsrc, XaXid xid )
    throws XAException
{
    if ( trace.enabled( 3 ) )  
	trace.write( tr_id + ".registerXID( " + rsrc + ", '" + xid + "' )" );

    if ( regXID.get( xid ) != null )  
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".registerXID(): duplicate XID" );
	throw new XAException( XAException.XAER_DUPID );
    }

    regXID.put( xid, rsrc );
    return;
} // registerXID


/*
** Name: deregisterXID
**
** Description:
**	Deregister a previously registered distributed transaction ID.
**
** Input:
**	rsrc	XA resource.
**	xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Jul-06 (gordy)
**	    Removed.
**	31-Aug-06 (gordy)
**	    Restored.
*/

public void
deregisterXID( XAResource rsrc, XaXid xid )
{
    if ( trace.enabled( 3 ) )  
	trace.write( tr_id + ".deregisterXID( '" + xid + "' )" );

    XAResource regRsrc = (XAResource)regXID.get( xid );
    
    if ( regRsrc == null )
    {
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".deregisterXID(): XID not registered" );
    }
    else  if ( rsrc != regRsrc )  
    {
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".deregisterXID(): Mis-matched XAResource" );
    }
    else
    {
        regXID.remove( xid );
    }
    
    return;
} // deregisterXID


/*
** Name: startXID
**
** Description:
**	Start a Distributed Transaction.
**
**	The XID must be registered, therefore this implies a RESUME.
**
** Input:
**	xid	XA transaction ID.
**	flags	XA operational flags.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Aug-06 (gordy)
**	    Created.
*/

public void
startXID( XaXid xid, int flags )
    throws XAException
{
    XAResource rsrc = (XAResource)regXID.get( xid );

    if ( trace.enabled( 2 ) )
	trace.write( tr_id + ".startXID('" + xid + "')" );

    if ( rsrc == null )
    {
	if ( trace.enabled( 1 ) )
	    trace.write(tr_id + ".startXID: XID '" + xid + "' not registered");
	throw new XAException( XAException.XAER_NOTA );
    }

    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".startXID: passing request to " + rsrc );

    rsrc.start( xid, flags );
    return;
}


/*
** Name: endXID
**
** Description:
**	End association with a Distributed Transaction.
**
**	The XID must be registered.
**
** Input:
**	xid	XA transaction ID.
**	flags	XA operational flags.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Aug-06 (gordy)
**	    Created.
*/

public void
endXID( XaXid xid, int flags )
    throws XAException
{
    XAResource rsrc = (XAResource)regXID.get( xid );

    if ( trace.enabled( 2 ) )
	trace.write( tr_id + ".endXID('" + xid + "')" );

    if ( rsrc == null )
    {
	if ( trace.enabled( 1 ) )
	    trace.write(tr_id + ".endXID: XID '" + xid + "' not registered");
	throw new XAException( XAException.XAER_NOTA );
    }

    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".endXID: passing request to " + rsrc );

    rsrc.end( xid, flags );
    return;
}


/*
** Name: prepareXID
**
** Description:
**	Prepare (2PC) a Distributed Transaction.
**
** Input:
**	xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	int	XA_OK.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Jul-06 (gordy)
**	    Operation now performed on new connection rather than 
**	    passing to registered connection.
*/

public int
prepareXID( XaXid xid ) 
    throws XAException
{
    JdbcConn conn;

    if ( trace.enabled( 2 ) )
        trace.write( tr_id + ".prepareXID( '" + xid + "' ) " );
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".prepareXID: establishing DTM connection" );
    
    try { conn = connect( getHost(), getDatabaseName(), (XaXid)null ); }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".prepareXID: Connection error" );
	    SqlExFactory.trace( ex, trace );
	}

	throw new XAException( XAException.XAER_RMFAIL );
    }

    /*
    ** Now prepare the transaction and drop the connection.
    */
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".prepareXID: prepare transaction on " + conn );
    
    try { conn.prepareTransaction( xid ); }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  trace.write( tr_id + 
		".prepareXID: XA error preparing transaction - " + 
		xaEx.getErrorCode() );

	throw new XAException( xaEx.getErrorCode() );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".prepareXID: error preparing transaction" );
	    SqlExFactory.trace(sqlEx, trace );
	}

	throw new XAException( XAException.XAER_RMERR );
    }
    finally
    {
	try { conn.close(); } catch( SQLException ignore ) {}
    }

    return( XAResource.XA_OK );
} // prepareXID


/*
** Name: commitXID
**
** Description:
**	Commit a Distributed Transaction.
**
** Input:
**	xid	    XA transaction ID.
**	onePhase    Use 1PC protocol?
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Handle unregistered XIDs (may have come from recover()).
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Jul-06 (gordy)
**	    Operation now performed on new connection rather than 
**	    passing to registered connection.
*/

public void
commitXID( XaXid xid, boolean onePhase ) 
    throws XAException
{
    JdbcConn conn;

    if ( trace.enabled( 2 ) )
        trace.write( tr_id + ".commitXID( '" + xid + "', " + onePhase + " ) " );
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".commitXID: establishing DTM connection" );
    
    try { conn = connect( getHost(), getDatabaseName(), (XaXid)null ); }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".commitXID: Connection error" );
	    SqlExFactory.trace( ex, trace );
	}

	throw new XAException( XAException.XAER_RMFAIL );
    }

    /*
    ** Now commit the transaction and drop the connection.
    */
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".commitXID: commit transaction on " + conn );
    
    try 
    { 
	if ( onePhase )
	    conn.commit( xid, XAResource.TMONEPHASE );
	else
	    conn.commit( xid );
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  trace.write( tr_id + 
		".commitXID: XA error committing transaction - " + 
		xaEx.getErrorCode() );

	throw new XAException( xaEx.getErrorCode() );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".commitXID: error committing transaction" );
	    SqlExFactory.trace(sqlEx, trace );
	}

	throw new XAException( XAException.XAER_RMERR );
    }
    finally
    {
	try { conn.close(); } catch( SQLException ignore ) {}
    }

    return;
} // commitXID


/*
** Name: rollbackXID
**
** Description:
**	Rollback a Distributed Transaction.
**
** Input:
**	xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Handle unregistered XIDs (may have come from recover()).
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Jul-06 (gordy)
**	    Operation now performed on new connection rather than 
**	    passing to registered connection.
*/

public void
rollbackXID( XaXid xid ) 
    throws XAException
{
    JdbcConn conn;

    if ( trace.enabled( 2 ) )
        trace.write( tr_id + ".rollbackXID( '" + xid + "' ) " );
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".rollbackXID: establishing DTM connection" );
    
    try { conn = connect( getHost(), getDatabaseName(), (XaXid)null ); }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".rollbackXID: Connection error" );
	    SqlExFactory.trace( ex, trace );
	}

	throw new XAException( XAException.XAER_RMFAIL );
    }

    /*
    ** Now rollback the transaction and drop the connection.
    */
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".rollbackXID: rollback transaction on " + conn );
    
    try { conn.rollback( xid ); }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  trace.write( tr_id + 
		".rollbackXID: XA error rolling back transaction - " + 
		xaEx.getErrorCode() );

	throw new XAException( xaEx.getErrorCode() );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write(tr_id + ".rollbackXID: error rolling back transaction");
	    SqlExFactory.trace(sqlEx, trace );
	}

	throw new XAException( XAException.XAER_RMERR );
    }
    finally
    {
	try { conn.close(); } catch( SQLException ignore ) {}
    }

    return;
} // rollbackXID


/*
** Name: abortXID
**
** Description:
**	Establish a connection to the DBMS to forcefully abort
**	a distributed transaction.
**
** Input:
**	xid	XA transaction ID.
**	server	Server ID.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	10-May-05 (gordy)
**	    Created.
*/

public void
abortXID( XaXid xid, String server )
    throws XAException
{
    JdbcConn		conn;
    StringBuffer	target;
    int			index;
    
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ".abortXID( '" + xid + "', '" + server + "' )" ); 

    /*
    ** Connection must be made to the DBMS serving the XID.
    ** The server ID can override the server class to target
    ** a specific server: node::db/@server
    */
    target = new StringBuffer( getDatabaseName() );
    index = target.indexOf( "::" );
    index = target.indexOf( "/", (index < 0) ? 0 : index + 2 );
    if ( index >= 0 )  target.setLength( index );
    target.append( "/@" ).append( server );
    
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".abortXID: establishing DTM connection" );
    
    /*
    ** Connect to DBMS serving the XID.  Username and 
    ** password are not required by the JDBC Server 
    ** when making a distributed transaction management 
    ** connection.
    */
    try { conn = connect( getHost(), target.toString(), (XaXid)null ); }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".abortXID: Connection error" );
	    SqlExFactory.trace( ex, trace );
	}

	throw new XAException( XAException.XAER_RMFAIL );
    }

    /*
    ** Now abort the transaction and drop the connection.
    */
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".abortXID: aborting transaction on " + conn );
    
    try { conn.abortTransaction( xid ); }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".abortXID: error aborting transaction" );
	    SqlExFactory.trace(ex, trace );
	}

	throw new XAException( XAException.XAER_RMERR );
    }
    finally
    {
	try { conn.close(); }
	catch( SQLException ignore ) {}
    }

    return;
} // abortXID


/*
** Name: recoverXID
**
** Description:
**	Retrieve list of prepared transaction IDs.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Xid[]	Prepared transactions IDs.
**
** History:
**	14-Mar-01 (gordy)
**	    Created as STUB.
**	18-Apr-01 (gordy)
**	    Implemented.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public Xid[]
recoverXID() 
    throws XAException
{
    /*
    ** Prepared XIDs may be obtained from the master database 
    ** of the installation containing the target database.  If 
    ** the target database spec contains a VNODE, we need to 
    ** use it for the connection but not in the request param.
    */
    String	xact_db = getDatabaseName();
    String	conn_db = MASTER_DB;
    JdbcConn	conn;
    XaXid	xids[];
    int		idx;

    if ( trace.enabled( 2 ) )  trace.write( tr_id + ".recoverXID()" ); 

    /*
    ** If DataSource database contains a vnode, the vnode needs
    ** to be on the master DB and not the target DB for our usage.
    */
    if ( (idx = xact_db.indexOf("::")) >= 0 )
    {
	idx += 2;
	conn_db = xact_db.substring( 0, idx ) + conn_db;
	xact_db = xact_db.substring( idx );
    }

    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".recoverXID: establishing DTM connection" );
    
    /*
    ** Connect to the master database in the target installation.
    */
    try { conn = connect( getHost(), conn_db, (XaXid)null ); }
    catch( SQLException ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".recoverXID: connection failed!" );
	    SqlExFactory.trace( ex, trace );
	}

	throw new XAException( XAException.XAER_RMFAIL );
    }

    /*
    ** Request prepared transaction XIDs.
    */
    try { xids = conn.getPreparedTransactionIDs( xact_db ); }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".recoverXID: error in XID query" );
	    SqlExFactory.trace(ex, trace );
	}

	throw new XAException( XAException.XAER_RMERR );
    }
    finally
    {
	try { conn.close(); }
	catch( SQLException ignore ) {}
    }

    return( xids );
} // recoverXID


/*
** Name: readObject
**
** Description:
**	This method is called to deserialize an object of this class.
**	The serialized state is produced by the default serialization
**	methods, so we call the default deserialization method.  Then
**	the transient initialization method is called to enable the
**	complete runtime state.
**
** Input:
**	in	Serialized input stream.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

private void
readObject( ObjectInputStream in )
    throws IOException, ClassNotFoundException
{
    in.defaultReadObject();	// Read serialized state.
    initialize();		// Initialize transient data.
    return;
} // readObject


} // class JdbcXADS
