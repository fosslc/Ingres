/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcXADataSource.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcXADataSource.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Implemented support for recovery.
*/

import	java.util.Hashtable;
import	java.util.Properties;
import	java.io.ObjectInputStream;
import	java.io.IOException;
import	java.sql.Connection;
import	java.sql.SQLException;
import	javax.naming.Reference;
import	javax.naming.NamingException;
import	javax.sql.XADataSource;
import	javax.sql.XAConnection;
import	javax.transaction.xa.XAResource;
import	javax.transaction.xa.Xid;
import	javax.transaction.xa.XAException;
import	ca.edbc.jdbc.EdbcConnect;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.jdbc.EdbcXID;
import	ca.edbc.jdbc.EdbcConst;


/*
** Name: EdbcXADataSource
**
** Description:
**	EDBC JDBC extension class which implements the JDBC 
**	XADataSource interface.  This class also implements
**	the EdbcXARM interface to support XA requirements 
**	beyound the capabilities of EdbcXAConnect support 
**	for XAResource.
**
**	This class is a subclass of EdbcDataSource and so may be
**	used as a DataSource object and supports all the properties
**	of the EdbcDataSource class.
**
**  Interface Methods:
**
**	getXAConnection		Generate a XAConnection object.
**
**	getReference		Returns JNDI Reference to EdbcXADataSource obj.
**
**	registerXID		Register distributed transaction ID.
**	deregisterXID		Deregister distributed transaction ID.
**	prepareXID		Prepare distributed transaction.
**	commitXID		Commit distributed transaction.
**	rollbackXID		Rollback distributed transaction.
**	recoverXID		Retrieve list of distributed transaction IDs.
**
**  Private Data:
**
**	MASTER_DB		Installation master database.
**	regXID			Registry for XID/XAResource.
**
**  Private Methods:
**
**	find			Returns XAResource registered for XID.
**	endXID			Commit/Rollback XID via DBMS connection.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Implemented support for recovery: added endXID().
*/

public class
EdbcXADataSource
    extends EdbcDataSource
    implements XADataSource, EdbcXARM
{

    /*
    ** Recovery constants.
    */
    private static final String	MASTER_DB = "iidbdb";

    /*
    ** XID/XAResource Registry.
    */
    private transient Hashtable	regXID = null;


/*
** Name: EdbcXADataSource
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
*/

public
EdbcXADataSource() 
{
    initialize();
} // EdbcXADataSource


private void
initialize()
{
    regXID = new Hashtable();
    title = "EDBC-XADataSource";
    tr_id = "XaDS";
    return;
}


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
*/

public XAConnection
getXAConnection()
    throws SQLException
{
    return( getXAConnection( null, null ) );
} // getXAConnection


/*
** Name:getXAConnection
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
*/

public XAConnection
getXAConnection( String user, String password )
    throws SQLException
{
    XAConnection conn;

    if ( trace.enabled() )
	trace.log( title + ".getXAConnection('" + user + "')" );

    conn = new EdbcXAConnect( getConnection( user, password ), this, trace );

    if ( trace.enabled() )  trace.log( title + ".getXAConnection(): " + conn );
    return( conn );
} // getXAConnection


/*
** Name: getReference
**
** Description:
**	Returns a JNDI Reference to this object.  Encodes the property
**	values to be retrieved by the getObjectInstance() method of the
**	class EdbcDSFactory.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Reference   A JNDI Reference.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
*/

public Reference
getReference()
    throws NamingException
{
    Reference	ref = new Reference( this.getClass().getName(),
				     "ca.edbc.jdbcx.EdbcXAFactory", null );
    initReference( ref );
    return( ref );
} // getReference


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
*/

public void
registerXID( XAResource rsrc, EdbcXAXid xid )
    throws XAException
{
    if ( trace.enabled( 3 ) )  
    {
	trace.write( tr_id + ".registerXID()" );
	if ( trace.enabled( 4 ) )  xid.trace( trace );
    }

    if ( find( xid ) != null )  
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
*/

public void
deregisterXID( XAResource rsrc, EdbcXAXid xid )
    throws XAException
{
    if ( trace.enabled( 3 ) )  
    {
	trace.write( tr_id + ".deregisterXID()" );
	if ( trace.enabled( 4 ) )  xid.trace( trace );
    }

    XAResource regRsrc = find( xid );
    
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
** Name: prepareXID
**
** Description:
**	Prepare (2PC) a Distributed Transaction.
**
**	Find XA resource registered to service the XID
**	and pass along the request.
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
*/

public int
prepareXID( EdbcXAXid xid ) 
    throws XAException
{
    XAResource rsrc = find( xid );

    if ( rsrc == null )  
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".prepareXID(): XID not registered" );
	throw new XAException( XAException.XAER_NOTA );
    }

    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ".prepareXID(): passing request to " + rsrc );
    return( rsrc.prepare( (Xid)xid ) );
} // prepareXID


/*
** Name: commitXID
**
** Description:
**	Commit a Distributed Transaction.
**
**	Find XA resource registered to service the XID
**	and pass along the request.
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
*/

public void
commitXID( EdbcXAXid xid, boolean onePhase ) 
    throws XAException
{
    XAResource rsrc = find( xid );

    if ( rsrc == null )  
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".commitXID(): XID not registered" );
	endXID( xid, false );
    }
    else
    {
	if ( trace.enabled( 2 ) )  
	    trace.write( tr_id + ".commitXID(): passing request to " + rsrc );
	rsrc.commit( (Xid)xid, onePhase );
    }

    return;
} // commitXID


/*
** Name: rollbackXID
**
** Description:
**	Rollback a Distributed Transaction.
**
**	Find XA resource registered to service the XID
**	and pass along the request.
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
*/

public void
rollbackXID( EdbcXAXid xid ) 
    throws XAException
{
    XAResource rsrc = find( xid );

    if ( rsrc == null )  
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".rollbackXID(): XID not registered" );
	endXID( xid, true );
    }
    else
    {
	if ( trace.enabled( 2 ) )  
	    trace.write(tr_id + ".rollbackXID(): passing request to " + rsrc);
	rsrc.rollback( (Xid)xid );
    }
    return;
} // rollbackXID


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
    String  xact_db = databaseName;
    String  conn_db = MASTER_DB;
    EdbcXID xid[];
    int	    idx;

    if ( (idx = xact_db.indexOf("::")) >= 0 )
    {
	idx += 2;
	conn_db = xact_db.substring( 0, idx ) + conn_db;
	xact_db = xact_db.substring( idx );
    }

    /*
    ** Connect to the master database in the target installation.
    */
    EdbcConnect	conn;
    String	host = getHost();

    if ( trace.enabled( 2 ) )
    {
	trace.write( tr_id + ".recoverXID: making 2PC management connection" );
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".recoverXID: " + host + "/" + conn_db );
    }

    try
    {
	Properties info = new Properties();
	info.setProperty( EdbcConst.cp_db, conn_db );
	conn = new EdbcConnect( host, info, conn_trace, (EdbcXID)null );
    }
    catch( EdbcEx ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".recoverXID: connection failed!" );
	    ex.trace( trace );
	}

	throw new XAException( XAException.XAER_RMFAIL );
    }

    /*
    ** Request prepared transaction XIDs.
    */
    try
    {
	xid = conn.getPreparedTransactionIDs( xact_db );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".recoverXID: error in XID query" );
	    ((EdbcEx)ex).trace( trace );
	}

	throw new XAException( XAException.XAER_RMERR );
    }
    finally
    {
	try { conn.close(); }
	catch( SQLException ignore ) {}
    }

    /*
    ** The EDBC XID is not an Xid subclass, so
    ** we must convert to our EdbcXAXid class.
    */
    Xid xids[] = new Xid[ xid.length ];
    
    for( int i = 0; i < xids.length; i++ )
	xids[ i ] = (Xid)(new EdbcXAXid( xid[ i ].getFormatId(), 
					 xid[ i ].getGlobalTransactionId(), 
					 xid[ i ].getBranchQualifier() ));
    return( xids );
} // recoverXID


/*
** Name: find
**
** Description:
**	Search for XA resource registered to service an XID.
**
** Input:
**	xid	XA transaction ID.
**
** Output:
**	None.
**
** Returns:
**	XAResource  XA resource or NULL.
**
** History:
**	14-Mar-01 (gordy)
**	    Created.
*/

private XAResource
find( EdbcXAXid xid )
{
    return( (XAResource)regXID.get( xid ) );
} // find


/*
** Name: endXID
**
** Description:
**	Establish a connection to the DBMS to commit/rollback an
**	existing distributed transaction.
**
** Input:
**	xid	XA transaction ID.
**	abort	True to rollback transaction, false to commit.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	18-Apr-01 (gordy)
**	    Created.
*/

private void
endXID( EdbcXAXid xid, boolean abort )
    throws XAException
{
    Connection	conn;
    String	host = getHost();

    if ( trace.enabled( 2 ) )
    {
	trace.write( tr_id + ".endXID: making XID management connection" );
	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".endXID: " + host + "/" + databaseName );
    }

    /*
    ** Connect to database requesting XID to associated
    ** with the new connection.  Username and password
    ** are not required by the JDBC Server when making
    ** a distributed transaction management connection.
    */
    try
    {
	Properties info = new Properties();
	info.setProperty( EdbcConst.cp_db, databaseName );
	conn = new EdbcConnect( host, info, conn_trace, xid );
    }
    catch( EdbcEx ex )
    { 
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".endXID: XID connection error" );
	    ex.trace( trace );
	}

	// TODO: distinguish NO XID (NOTA) and general failure (RMFAIL).
	throw new XAException( XAException.XAER_NOTA );
    }

    /*
    ** Now end the transaction and drop the connection.
    */
    try
    {
	if ( abort )
	{
	    if ( trace.enabled( 3 ) )  
		trace.write( tr_id + ".endXID(): aborting xact on " + conn ); 
	    
	    conn.rollback();
	}
	else
	{
	    if ( trace.enabled( 3 ) )  
		trace.write( tr_id + ".endXID(): commiting xact on " + conn ); 
	
	    try { conn.commit(); }
	    catch( SQLException ex )
	    {
		try { conn.rollback(); }
		catch( SQLException ignore ) {}
		throw ex;
	    }
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".endXID: error ending 2PC transaction" );
	    ((EdbcEx)ex).trace( trace );
	}

	throw new XAException( XAException.XAER_RMERR );
    }
    finally
    {
	try { conn.close(); }
	catch( SQLException ignore ) {}
    }
    return;
} // endXID


private void
readObject( ObjectInputStream in )
    throws IOException, ClassNotFoundException
{
    in.defaultReadObject();
    initialize();
    return;
}


} // class EdbcXADataSource
