/*
** Copyright (c) 2001, 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcXAConn.java
**
** Description:
**	Defines class which implements the JDBC XAConnection interface.
**
**   Classes:
**
**	JdbcXAConn		XAConnection implementation.
**	XactPool		Transaction pool.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	12-Apr-01 (gordy)
**	    Exceptions are now self tracing.
**	18-Apr-01 (gordy)
**	    Adjustment to tracing to work with EdbcXADataSource.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	18-Feb-03 (gordy)
**	    Brought getConnection() and start() in line with JDBC 3.0 spec.
**	 2-Feb-05 (gordy)
**	    Support XA multi-threading recovery.
**	10-May-05 (gordy)
**	    Enhance XA support to abort using independent connection.
**	20-Jul-06 (gordy)
**	    Re-implemented based on enhanced XA support in server.
**	31-Aug-06 (gordy)
**	    Register XID during start/end association.
**	 9-Nov-06 (gordy)
**	    It's probably not a good idea to save the inactive connections
**	    since they are only closed when this XA connection is closed,
**	    which may not happen for a long time during connection pooling.
**	    Since inactive connections are only caused by XA suspend requests,
**	    dropping inactive connections should not be a significant
**	    performance hit.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
*/

import	java.util.Stack;
import	java.util.Vector;
import	java.sql.Connection;
import	java.sql.SQLException;
import	javax.sql.XAConnection;
import	javax.sql.ConnectionEvent;
import	javax.sql.ConnectionEventListener;
import	javax.sql.StatementEventListener;
import	javax.transaction.xa.XAResource;
import	javax.transaction.xa.Xid;
import	javax.transaction.xa.XAException;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.XaEx;
import	com.ingres.gcf.util.XaXid;


/*
** Name: JdbcXAConn
**
** Description:
**	JDBC driver base class which implements the JDBC XAConnection 
**	interface.  Also implements the XAResource interface since there
**	is one-to-one relation between XAConnection and XAResource and
**	start()/end()  must occur in the context of the connection.
**
**  Interface Methods:
**
**	getConnection		Generate a virtual connection.
**	close			Close physical connection.
**	getXAResource		Return associated XA Resource.
**
**	isSameRM		Are XARM the same?
**	start			Start transaction.
**	end			End transaction.
**	prepare			Prepare transaction.
**	commit			Commit transaction.
**	rollback		Rollback transaction.
**	recover			Retrieve transaction IDs.
**	forget			Forget transaction ID.
**	getTransactionTimeout	Get transaction timeout value.
**	setTransactionTimeout	Set transaction timeout value.
**
**  Private Methods:
**
**	endSuspended		End suspended XA transaction.
**	suspend			Suspend XA transaction.
**	resume			Resume XA transaction.
**
**  Private Data:
**
**	xarm			XA Resource Manager.
**	user			User ID.
**	password		Password.
**	autoCommit		Original autocommit state.
**	xid			Current transaction ID.
**	suspended		Suspended connections/transactions.
**	noXIDs			Empty array of transaction IDs.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	08-Jan-04 (rajus01) startak problem # EDJDBC79 ; Bug # 111571
**	    Don't throw "invalid transaction state" exception if there is
**	    a virtual connection and if autocommit is disabled.
**	 2-Feb-05 (gordy)
**	    Added TX_ST_ABORT and xa_thread to support XA multi-threading
**	    recovery.
**	10-May-05 (gordy)
**	    Added serverID to permit connection to specific server for abort.
**	20-Jul-06 (gordy)
**	    Enhanced XA support in server simplifies driver state handling.
**	9-Nov-06 (gordy)
**	    Removed inactive connection pool.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
*/

public class
JdbcXAConn
    extends JdbcCPConn
    implements	XAConnection, XAResource, GcfErr
{

    private	XARsrcMgr	xarm		= null;	   /* XADataSource */
    private	String		user		= null;    /* User ID */
    private	String		password	= null;    /* Password */

    private	XaXid		xid		= null;	   /* Active XA XID */
    private	boolean		autoCommit	= false;   /* Non-XA state */
    private	XactPool	suspended = new XactPool(); /* Suspended Xact */

    private static final Xid	noXIDs[]	= new Xid[ 0 ];


/*
** Name: JdbcXAConn
**
** Description:
**	Class constructor.
**
** Input:
**	conn		JDBC Connection.
**	xarm		XA Resource Manager.
**	user		User ID.
**	password	Password.
**	trace		DataSource tracing.
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
**	10-May-05 (gordy)
**	    Retrieve server ID for transaction management.
**	20-Jul-06 (gordy)
**	    Enhanced XA support in server no longer requires server addr.
**	    Added user & password to facilitate suspend/resume.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcXAConn
(
    JdbcConn	conn,
    XARsrcMgr	xarm,
    String	user,
    String	password,
    DrvTrace	trace
) 
    throws SQLException
{
    super( conn, trace );
    
    this.xarm = xarm;
    this.user = user;
    this.password = password;

    title = trace.getTraceName() + "-XAConnection[" + inst_id + "]";
    tr_id = "XAConn[" + inst_id + "]";
    
    return;
} // JdbcXAConn


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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	18-Feb-03 (gordy)
**	    Can't support if distributed transaction is waiting to
**	    be prepared/committed.
**	20-Jul-06 (gordy)
**	    Simplified state handling.  Transaction is either
**	    idle (xid == null) or active (xid != null).
*/

public Connection
getConnection()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getConnection()" );

    if ( physConn == null )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ": physical connection is closed" );
	throw SqlExFactory.get( ERR_GC4004_CONNECTION_CLOSED );
    }

    /*
    ** Autocommit must be enabled for connections which
    ** are not participating in a distributed transaction.
    */
    if ( xid == null )  physConn.setAutoCommit( true );

    /*
    ** Any active virtual connection is automatically
    ** closed when a new virtual connection is activated.
    */
    clearConnection();
    virtConn = new JdbcXAVirt( physConn, this, this, trace, (xid != null) );

    if ( trace.enabled() ) trace.log(title + ".getConnection(): " + virtConn);
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
**	31-Jan-01 (gordy)
**	    Created.
**	10-May-05 (gordy)
**	    Cleaned up state handling.
**	20-Jul-06 (gordy)
**	    Simplified state handling.  Transaction is either
**	    idle (xid == null) or active (xid != null).
**	31-Aug-06 (gordy)
**	    Deregister transactions serviced by this XAResource.
**	 9-Nov-06 (gordy)
**	    Inactive connection pool no longer maintained.
*/

public void
close()
    throws SQLException
{
    Object	entry[];
    JdbcConn	conn;

    if ( physConn != null  &&  xid != null )
    {
	if ( trace.enabled() )  trace.log( title + ".close()" );
	if ( trace.enabled( 2 ) )  
	    trace.write( tr_id + ".close: abort active transaction " + 
			 xid + " on " + physConn );
	    
	try { physConn.rollback(); } catch( Exception ignore ) {}
	xarm.deregisterXID( this, xid );
	xid = null;
    }

    super.close();

    /*
    ** Abort suspended transactions and
    ** close associated connections.
    */
    while( (entry = suspended.remove()) != null )
    {
	XaXid xid = (XaXid)entry[0];
	conn = (JdbcConn)entry[1];

	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".close: abort suspended transaction " +
			 xid + " on " + conn );

	try { conn.rollback(); } catch( Exception ignore ) {}
	xarm.deregisterXID( this, xid );

	if ( trace.enabled( 3 ) )
	    trace.write(tr_id + ".close: close suspended connection " + conn);

	try { conn.close(); } catch( Exception ignore ) {}
    }

    return;
} // close


/*
** Name: getXAResource
**
** Description:
**	Returns reference to associated XAResource.
**
**	Since there is a 1-to-1 association between XAConnection
**	and XAResource, this XAConnection class also implements
**	XAResource and a reference to this object is returned.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	XAResource
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
*/

public XAResource
getXAResource()
    throws SQLException
{
    if ( physConn == null )  throw SqlExFactory.get( ERR_GC4004_CONNECTION_CLOSED );
    return( this );
} // getXAResource


/*
** Name: isSameRM
**
** Description:
**	Is the same Resource Manager referenced by another XAResource.
**
**	It is assumed that connections derived from the same DataSource
**	reference the same Resource Manager.
**
** Input:
**	xares	XAResource to test.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if same RM, false otherwise.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	20-Jul-06 (gordy)
**	    Check for same associated DataSource.
*/

public boolean
isSameRM( XAResource xares )
{
    if ( xares instanceof JdbcXAConn )  
	return( this.xarm == ((JdbcXAConn)xares).xarm );

    return( false );
} // isSameRM


/*
** Name: start
**
** Description:
**	Start a Distributed Transaction.
**
**	A virtual connection may be active outside a distributed
**	transaction and behaves like a normal JDBC connection,
**	including having autocommit enabled by default.  Auto-
**	commit is disabled if active and will be re-enabled at
**	the end of the distributed transaction.  A standard
**	transaction must not be active or an exception will be
**	thrown.
**	
** Input:
**	xid	XA Transaction ID.
**	flags	Operation flags.
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
**	12-Apr-01 (gordy)
**	    Exceptions are now self tracing.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	18-Feb-03 (gordy)
**	    JDBC 3.0 allows start() on connection with autocommit enabled.
**	08-Jan-04 (rajus01) startak problem # EDJDBC79 ; Bug # 111571
**	    Don't throw "invalid transaction state" exception if there is
**	    a virtual connection and if autocommit is disabled.
**	29-Mar-04 (rajus01) Star # EDJDBC86; Bug # 112045
**	    Added support for TMJOIN flag.
**	 2-Feb-05 (gordy)
**	    Check for aborted transaction state.  Save current thread
**	    when transaction is started/re-activated.
**	10-May-05 (gordy)
**	    Cleaned up state handling.  Rollback to free resources if 
**	    aborted and then allow start.
**	20-Jul-06 (gordy)
**	    Enhanced XA support in server simplifies driver.
**	31-Aug-06 (gordy)
**	    Register transaction with XARM so that subsequent
**	    start/end requests can be passed to correct XAResource.
*/

public void
start( Xid xid, int flags )
    throws XAException
{
    XaXid	xaxid = (xid instanceof XaXid) ? (XaXid)xid : new XaXid(xid);
    boolean	join = false;

    if ( trace.enabled() )  
	trace.log( title + ".start( '" + xaxid + "', 0x" + 
		   Integer.toHexString( flags ) + " )" );

    if ( physConn == null ) 
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".start: connection closed" );
	throw new XAException( XAException.XAER_PROTO );
    }

    switch( flags )
    {
    case TMNOFLAGS :	/* OK */		break;
    case TMJOIN :	join = true;		break;
    case TMRESUME :	resume( xaxid );	return;

    default :
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".start: unsupported flags - 0x" + 
			 Integer.toHexString( flags ) );
	throw new XAException( XAException.XAER_INVAL );
    }

    /*
    ** Generally, a start() request within an active transaction
    ** would be an XA protocol error.
    */
    if ( this.xid != null )
    {
	/*
	** Since an active XA transaction can be aborted via
	** an external connection, the first indication that
	** we may get is the start of the next transaction.
	** We must, therefore, allow this request and assume
	** that the TM action is correct.  Even though the 
	** transaction has been aborted in the DBMS, we need 
	** to clean up the intermediate JDBC server which
	** can be done with regular rollback.
	*/
	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ".start: end active transaction " + 
			 this.xid + " on " + physConn );

	try { physConn.rollback(); } catch( Exception ignore ) {}
	xarm.deregisterXID( this, this.xid );
	this.xid = null;
	if ( virtConn != null )  ((JdbcXAVirt)virtConn).setActiveDTX( false );
    }
 
    try 
    { 
	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ".start: issuing request on " + physConn );

	/*
	** Autocommit must be disabled for connections which
	** are participating in a distributed transaction.
	** Autocommit state is saved so that it may be re-
	** established when the distributed transaction is
	** completed.
	*/
	autoCommit = (virtConn != null) ? physConn.getAutoCommit() : false;
	physConn.setAutoCommit( false );

	/*
	** Start connections participation in distributed transaction.
	*/
	if ( join )
	    physConn.startTransaction( xaxid, TMJOIN );
	else
	    physConn.startTransaction( xaxid ); 

	this.xid = xaxid;
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ".start: XA error starting transaction - " +
			 xaEx.getErrorCode() );

	throw new XAException( xaEx.getErrorCode() );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".start: error starting transaction" );
	    SqlExFactory.trace(sqlEx, trace);
	}

	boolean failed = true;

	try { failed = physConn.isClosed(); }
	catch( SQLException ignore ) {}

	throw new XAException( failed ? XAException.XAER_RMFAIL
				      : XAException.XAER_RMERR );
    }

    /*
    ** Register as XAResource servicing current XID so that
    ** end() requests can be directed properly.
    */
    try { xarm.registerXID( this, this.xid ); }
    catch( XAException xaEx )
    {
	/*
	** Need to cancel started transaction.  Easiest 
	** way is to simply rollback the transaction.
	*/
	try { physConn.rollback(); } catch( Exception ignore ) {}
	this.xid = null;
	throw xaEx;
    }

    if ( virtConn != null )  ((JdbcXAVirt)virtConn).setActiveDTX( true );
    return;
} // start


/*
** Name: end
**
** Description:
**	End association with a Distributed Transaction.
**
** Input:
**	xid	XA Transaction ID.
**	flags	Operation flags.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 2-Feb-05 (gordy)
**	    Check for aborted transaction state.
**	10-May-05 (gordy)
**	    Cleaned up state handling.  Allow combination of TMFAIL
**	    and transaction aborted.
**	20-Jul-06 (gordy)
**	    Enhanced XA support in server simplifies driver.
**	31-Aug-06 (gordy)
**	    Deregister transactions.
*/

public void
end( Xid xid, int flags )
    throws XAException
{
    XaXid	xaxid = (xid instanceof XaXid) ? (XaXid)xid : new XaXid( xid );
    boolean	success = true;

    if ( trace.enabled() )  
	trace.log( title + ".end( '" + xaxid + "', 0x" + 
		   Integer.toHexString( flags ) + " )" );

    if ( physConn == null ) 
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".end: connection closed" );
	throw new XAException( XAException.XAER_PROTO );
    }

    switch( flags )
    {
    case TMSUCCESS :	/* OK */		break;
    case TMFAIL :	success = false;	break;
    case TMSUSPEND :	suspend( xaxid );	return;

    default :
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".end: unsupported flags - 0x" + 
			 Integer.toHexString( flags ) );
	throw new XAException( XAException.XAER_INVAL );
    }

    /*
    ** If the request is not for the current transaction,
    ** it may be for a suspended transaction.
    */
    if ( this.xid == null  ||  ! xaxid.equals( this.xid ) )
    {
	endSuspended( xaxid, success );
	return;
    }

    try 
    { 
	/*
	** End participation in distributed transaction.
	*/
	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ".end: issuing request on " + physConn );

	if ( success )
	    physConn.endTransaction( xaxid );
	else
	    physConn.endTransaction( xaxid, TMFAIL ); 

	/*
	** Re-establish the non-XA transaction state.
	*/
	physConn.setAutoCommit( autoCommit );
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ".end: XA error ending transaction - " +
			 xaEx.getErrorCode() );
	throw new XAException( xaEx.getErrorCode() );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".end: error ending transaction" );
	    SqlExFactory.trace(sqlEx, trace );
	}

	boolean failed = true;

	try { failed = physConn.isClosed(); }
	catch( SQLException ignore ) {}

	throw new XAException( failed ? XAException.XAER_RMFAIL
				      : XAException.XAER_RMERR );
    }
    finally
    {
	/*
	** Even if an error occurs, there nothing else to do.
	*/
	xarm.deregisterXID( this, this.xid );
	this.xid = null;
	if ( virtConn != null )  ((JdbcXAVirt)virtConn).setActiveDTX( false );
    }

    return;
} // end


/*
** Name: endSuspended
**
** Description:
**	End transaction association for a suspended transaction branch.
**	Suspended connection is placed into the connection pool.
**
** Input:
**	xid	XA XID.
**	success	Transaction successful.
**
** Output:
**	None.
**
** Returns:
**	20-Jul-06 (gordy)
**	    Created.
**	31-Aug-06 (gordy)
**	    Pass request to XARM if XID is not being serviced.
**	    Deregister transactions.
**	 9-Nov-06 (gordy)
**	    Inactive connection pool no longer maintained.
*/

private void
endSuspended( XaXid xid, boolean success )
    throws XAException
{
    JdbcConn conn = suspended.remove( xid );

    if ( conn == null )
    {
	/*
	** The XID is not being serviced by this XAResource.
	** See if another XAResource is registered for the XID.
	*/
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".end: passing request to XARM" );
	xarm.endXID( xid, success ? TMSUCCESS : TMFAIL );
	return;
    }

    try 
    { 
	/*
	** End connections participation in distributed transaction.
	*/
	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ".end: issuing request on " + conn );

	if ( success )
	    conn.endTransaction( xid );
	else
	    conn.endTransaction( xid, TMFAIL ); 
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ".end: XA error ending transaction - " +
			 xaEx.getErrorCode() );
	throw new XAException( xaEx.getErrorCode() );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".end: error ending transaction" );
	    SqlExFactory.trace( sqlEx, trace );
	}

	boolean failed = true;

	try { failed = physConn.isClosed(); }
	catch( SQLException ignore ) {}

	throw new XAException( failed ? XAException.XAER_RMFAIL
				      : XAException.XAER_RMERR );
    }
    finally
    {
	/*
	** Even if an error occurs, there is nothing else to do.
	*/
	xarm.deregisterXID( this, xid );

	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".end: close suspended connection " + conn );

	try { conn.close(); } catch( Exception ignore ) {}
    }

    return;
} // endSuspended


/*
** Name: suspend
**
** Description:
**	Suspend current transaction branch.
**
**	Operation not supported by server, so entire connection
**	is suspended and replaced with a new connection.
**
** Input:
**	xid	XA XID.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
**	31-Aug-06 (gordy)
**	    Pass request to XARM if XID is not being serviced.
**	    Check to see if XID is suspended (otherwise
**	    an infinite loop would occur with XARM).
*/

private void
suspend( XaXid xid )
    throws XAException
{
    JdbcConn conn;

    if ( trace.enabled( 2 ) )  
	trace.log( tr_id + ".suspend( '" + xid + "' )" );

    if ( this.xid == null  ||  ! xid.equals( this.xid ) )
    {
	/*
	** Check to see if XID is already suspended.
	** Currently, the only option is to attempt
	** to remove the transaction from the pool.
	*/
	conn = suspended.remove( xid );

	if ( conn != null )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ".suspend: XID is already suspended" );

	    suspended.add( xid, conn );		/* Restore to suspended pool */
	    throw new XAException( XAException.XAER_PROTO );
	}

	/*
	** The XID is not being serviced by this XAResource.
	** See if another XAResource is registered for the XID.
	*/
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".suspend: passing request to XARM" );
	xarm.endXID( xid, TMSUSPEND );
	return;
    }

    try
    {
	/*
	** Create replacement connection and re-establish 
	** the non-XA transaction state.
	*/
        conn = xarm.getRMConnection( user, password );
	conn.setAutoCommit( autoCommit );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".end: error ending transaction" );
	    SqlExFactory.trace( sqlEx, trace );
	}

	boolean failed = true;

	try { failed = physConn.isClosed(); }
	catch( SQLException ignore ) {}

	throw new XAException( failed ? XAException.XAER_RMFAIL
				      : XAException.XAER_RMERR );
    }

    /*
    ** Suspend current transaction/connection
    ** and activate alternate connection.
    */
    if ( trace.enabled( 3 ) )  
	trace.log( tr_id + ".suspend: activating " + conn );

    suspended.add( this.xid, physConn );
    physConn = conn;
    this.xid = null;

    if ( virtConn != null )  
    {
	((JdbcXAVirt)virtConn).setConnection( physConn );
	((JdbcXAVirt)virtConn).setActiveDTX( false );
    }

    return;
} // suspend


/*
** Name: resume
**
** Description:
**	Resume a suspended transaction branch.
**
**	Current connection is dropped and suspended connection 
**	is activated.
**
** Input:
**	xid	XA XID.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
**	31-Aug-06 (gordy)
**	    Pass request to XARM if XID is not being serviced.
**	 9-Nov-06 (gordy)
**	    Inactive connection pool no longer maintained.
*/

private void
resume( XaXid xid )
    throws XAException
{
    if ( trace.enabled( 2 ) )  
	trace.log( tr_id + ".resume( '" + xid + "' )" );

    JdbcConn conn = suspended.remove( xid );

    if ( conn == null )
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".resume: passing request to XARM" );
	xarm.startXID( xid, TMRESUME );
	return;
    }

    /*
    ** Generally, a start() request within an active transaction
    ** would be an XA protocol error.
    */
    if ( this.xid != null )
    {
	/*
	** Since an active XA transaction can be aborted via
	** an external connection, the first indication that
	** we may get is the start of the next transaction.
	** We must, therefore, allow this request and assume
	** that the TM action is correct.  Even though the 
	** transaction has been aborted in the DBMS, we need 
	** to clean up the intermediate JDBC server which
	** can be done with regular rollback.
	*/
	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ".resume: end active transaction " + 
			 this.xid + " on " + physConn );

	try { physConn.rollback(); } catch( Exception ignore ) {}
	xarm.deregisterXID( this, this.xid );
	this.xid = null;
	if ( virtConn != null )  ((JdbcXAVirt)virtConn).setActiveDTX( false );
    }
 
    /*
    ** Drop current connection and activate suspended 
    ** transaction/connection.
    */
    if ( trace.enabled( 3 ) )
    	trace.write( tr_id + ".resume: close active connection " + physConn );

    try { physConn.close(); } catch( Exception ignore ) {}

    if ( trace.enabled( 3 ) )  
	trace.log( tr_id + ".resume: activating " + conn );

    physConn = conn;
    this.xid = xid;

    if ( virtConn != null )  
    {
	((JdbcXAVirt)virtConn).setConnection( physConn );
	((JdbcXAVirt)virtConn).setActiveDTX( true );
    }

    return;
} // resume


/*
** Name: prepare
**
** Description:
**	Prepare (2PC) a Distributed Transaction.
**
** Input:
**	xid	XA Transaction ID.
**
** Output:
**	None.
**
** Returns:
**	int	XA_OK.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	12-Apr-01 (gordy)
**	    Exceptions are now self tracing.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 2-Feb-05 (gordy)
**	    Check for aborted transaction state.
**	10-May-05 (gordy)
**	    Cleaned up state handling.
**	20-Jul-06 (gordy)
**	    Enhanced XA support in server simplifies driver.
*/

public int
prepare( Xid xid )
    throws XAException
{
    XaXid xaxid = (xid instanceof XaXid) ? (XaXid)xid : new XaXid(xid);

    if ( trace.enabled() )  trace.log( title + ".prepare( '" + xaxid + "' )" );
    
    /*
    ** Can't prepare an XID which is currently active.
    */
    if ( this.xid != null  &&  xaxid.equals( this.xid ) )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".prepare: XID is active!" );
	throw new XAException( XAException.XAER_PROTO );
    }

    /*
    ** Check conditions requiring request to be passed to XARM:
    **    Connection closed
    **    XA transaction active
    **
    ** (Does (virtConn != null) indicate possible connection activity?)
    */
    if ( physConn == null  ||  this.xid != null )
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".prepare: passing request to XARM" );
	return( xarm.prepareXID( xaxid ) );
    }

    try { physConn.prepareTransaction( xaxid ); }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ".prepare: XA error preparing transaction - " +
			 xaEx.getErrorCode() );
	throw new XAException( xaEx.getErrorCode() );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".prepare: error preparing transaction" );
	    SqlExFactory.trace( sqlEx, trace );
	}

	boolean failed = true;

	try { failed = physConn.isClosed(); }
	catch( SQLException ignore ) {}

	throw new XAException( failed ? XAException.XAER_RMFAIL
				      : XAException.XAER_RMERR );
    }

    return( XA_OK );
} // prepare


/*
** Name: commit
**
** Description:
**	Commit a Distributed Transaction.
**
** Input:
**	xid	    XA Transaction ID.
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
**	12-Apr-01 (gordy)
**	    Exceptions are now self tracing.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	08-Jan-04 (rajus01) startak problem # EDJDBC79 ; Bug # 111571
**	    After commiting distributed transaction, enable autocommit 
**	    (default for connections not participating in distributed 
**	    transactions) if virtual connection exists. 
**	 2-Feb-05 (gordy)
**	    Check for aborted transaction state.
**	10-May-05 (gordy)
**	    Cleaned up state handling.
**	20-Jul-06 (gordy)
**	    Enhanced XA support in server simplifies driver.
*/

public void
commit( Xid xid, boolean onePhase )
    throws XAException
{
    XaXid xaxid = (xid instanceof XaXid) ? (XaXid)xid : new XaXid(xid);

    if ( trace.enabled() )  
	trace.log( title + ".commit( '" + xaxid + "', " + onePhase + " )" );

    /*
    ** Can't commit an XID which is currently active.
    */
    if ( this.xid != null  &&  xaxid.equals( this.xid ) )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".commit: XID is active!" );
	throw new XAException( XAException.XAER_PROTO );
    }

    /*
    ** Check conditions requiring request to be passed to XARM:
    **    Connection closed
    **    XA transaction active
    **
    ** (Does (virtConn != null) indicate possible connection activity?)
    */
    if ( physConn == null  ||  this.xid != null )
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".commit: passing request to XARM" );
	xarm.commitXID( xaxid, onePhase );
	return;
    }

    try 
    { 
	if ( onePhase )
	    physConn.commit( xaxid, TMONEPHASE ); 
	else
	    physConn.commit( xaxid );
    }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ".commit: XA error committing transaction - " +
			 xaEx.getErrorCode() );
	throw new XAException( xaEx.getErrorCode() );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".commit: error committing transaction" );
	    SqlExFactory.trace( sqlEx, trace );
	}

	boolean failed = true;

	try { failed = physConn.isClosed(); }
	catch( SQLException ignore ) {}

	throw new XAException( failed ? XAException.XAER_RMFAIL
				      : XAException.XAER_RMERR );
    }

    return;
} // commit


/*
** Name: rollback
**
** Description:
**	Rollback a Distributed Transaction.
**
**	If this request is made during the active state while
**	in recovery mode, the only thing that can be done is to
**	abort the connection since we don't know the connection
**	state.
**
** Input:
**	xid	    XA Transaction ID.
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
**	12-Apr-01 (gordy)
**	    Exceptions are now self tracing.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	08-Jan-04 (rajus01) startak problem # EDJDBC79 ; Bug # 111571
**	    After rollback of distributed transaction, enable autocommit 
**	    (default for connections not participating in distributed 
**	    transactions) if virtual connection exists. 
**	 2-Feb-05 (gordy)
**	    Handle a recovery rollback request from the TM during an
**	    active transaction (from some thread other than currently
**	    associated with the transaction).
**	10-May-05 (gordy)
**	    Cleaned up state handling.  Do rollback() if transaction
**	    previously aborted to free session resources.
**	20-Jul-06 (gordy)
**	    Enhanced XA support in server simplifies driver.
**	31-Aug-06 (gordy)
**	    Handle requests which target suspended transactions.
**	 9-Nov-06 (gordy)
**	    Inactive connection pool no longer maintained.
*/

public void
rollback( Xid xid )
    throws XAException
{
    XaXid xaxid = (xid instanceof XaXid) ? (XaXid)xid : new XaXid(xid);

    if ( trace.enabled() )  trace.log( title + ".rollback( '" + xaxid + "' )" );

    /*
    ** Check to see if requested transaction is suspended.
    */
    JdbcConn conn = suspended.remove( xaxid );

    if ( conn != null )
    {
	/*
	** Abort the suspended transaction.
	*/
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".rollback: aborting suspended transaction " + 
			 xid + " on " + conn );

	try { conn.rollback(); } catch( Exception ignore ) {}
	xarm.deregisterXID( this, xaxid );

	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ".rollback: close suspended connection " + 
			 conn );

	try { conn.close(); } catch( Exception ignore ) {}
	return;
    }

    /*
    ** Check conditions requiring request to be passed to XARM:
    ** connection closed or XA transaction active.
    **
    ** (Does (virtConn != null) indicate possible connection activity?)
    */
    if ( physConn == null  ||  this.xid != null )
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".rollback(): passing request to XARM" );
	xarm.rollbackXID( xaxid );
	return;
    }

    try { physConn.rollback( xaxid ); }
    catch( XaEx xaEx )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ".rollback: XA error rolling back xact - " + 
			 xaEx.getErrorCode() );
	throw new XAException( xaEx.getErrorCode() );
    }
    catch( SQLException sqlEx )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".rollback(): error aborting transaction" );
	    SqlExFactory.trace( sqlEx, trace );
	}

	boolean failed = true;

	try { failed = physConn.isClosed(); }
	catch( SQLException ignore ) {}

	throw new XAException( failed ? XAException.XAER_RMFAIL
				      : XAException.XAER_RMERR );
    }

    return;
} // rollback


/*
** Name: recover
**
** Description:
**	Retrieve list of prepared transaction IDs.
**
** Input:
**	flag	Operation flags.
**
** Output:
**	None.
**
** Returns:
**	Xid[]	Prepared transactions IDs.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
*/

public Xid[]
recover( int flag )
    throws XAException
{
    if ( trace.enabled() )  trace.log( title + ".recover()" );

    /*
    ** We only return transaction IDs at the start of the
    ** scan, in which case all transactions IDs are returned.
    */
    if ( (flag & TMSTARTRSCAN) == 0 )  
    {
	if ( trace.enabled() )
	    trace.log( title + ".recover: no XIDs (non-start request)" );
	return( noXIDs );
    }

    if ( trace.enabled( 2 ) )
	trace.write( tr_id + ".recover(): passing request to XARM" );
    return( xarm.recoverXID() );
} // recover


/*
** Name: forget
**
** Description:
**	Forget a completed Distributed Transaction.
**
**	Our XA transactions should never reach a state where
**	this method is applicable!
**
** Input:
**	xid	    XA Transaction ID.
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
*/

public void
forget( Xid xid )
    throws XAException
{
    if ( trace.enabled() )  trace.log( title + ".forget(): not supported!" );
    throw new XAException( XAException.XAER_NOTA );
} // forget


/*
** Name: getTransactionTimeout
**
** Description:
**	Get the timeout value for transactions.
**
** Input:
**	None.
**
** Ouput:
**	None.
**
** Returns:
**	int	Timeout in seconds.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
*/

public int
getTransactionTimeout()
{
    return( 0 );    // Not supported.
} // getTransactionTimeout


/*
** Name: setTransactionTimeout
**
** Description:
**	Set the timeout value for transactions.
**
** Input:
**	seconds	    Timeout value.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if successful, false otherwise.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
*/

public boolean
setTransactionTimeout( int seconds )
{
    return( false );	// Not supported
} // setTransactionTimeout

/*
** Name: XactPool
**
** Description:
**	Class which provides a pool of transactions.
**
**   Public Methods:
**
**	empty		Is pool empty?
**	add		Add transaction to pool.
**	remove		Remove transaction from pool.
**
**   Private Data;
**
**	pool		Transaction pool.
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

static private class
XactPool
{

    private	Vector		pool = new Vector();


/*
** Name: empty
**
** Description:
**	Returns an indication of whether the pool is empty or not.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		TRUE if empty, FALSE otherwise.
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

public boolean
empty()
{
    return( pool.isEmpty() );
} // empty


/*
** Name: add
**
** Description:
**	Add transaction (transaction ID and connection) to pool.
**
** Input:
**	xid		Transaction ID.
**	conn		Connection.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
**	31-Aug-06 (gordy)
**	    Synchronized with remove() methods.
*/

public synchronized void
add( XaXid xid, JdbcConn conn )
{
    Object entry[] = new Object[ 2 ];

    entry[ 0 ] = xid;
    entry[ 1 ] = conn;
    pool.add( entry );

    return;
} // add


/*
** Name: remove
**
** Description:
**	Removes a transaction from the pool.  A transaction is
**	selected at random and the associated XID and connection
**	are returned.  Null is returned if pool is empty.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Object[]	XaXid [0] and JdbcConn [1].
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
**	31-Aug-06 (gordy)
**	    Changed return type to provide both components.
**	    Synchronized due to separate size() and remove() calls.
*/

public synchronized Object[]
remove()
{
    Object	entry[] = null;
    int		count = pool.size();

    if ( count > 0 )  
	try { entry = (Object [])pool.remove( count - 1 ); }
	catch( Exception ex ) { entry = null; }

    return( entry );
} // remove


/*
** Name: remove
**
** Description:
**	Removes a transaction from the pool.  The pool is
**	searched for a transaction with matching transaction
**	ID and the associated connection is returned.  Null 
**	is returned if pool is empty.
**
** Input:
**	xid		Transaction ID.
**
** Output:
**	None.
**
** Returns:
**	JdbcConn	Connection or NULL.
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
**	31-Aug-06 (gordy)
**	    Synchronize to match other methods.
*/

public synchronized JdbcConn
remove( XaXid xid )
{
    try 
    { 
	for( int i = 0; i < pool.size(); i++ )
	{
	    Object entry[] = (Object [])pool.get( i ); 

	    if ( xid.equals( (XaXid)entry[ 0 ] ) )
	    {
		pool.remove( i );
		return( (JdbcConn)entry[ 1 ] );
	    }
	}
    }
    catch( Exception ignore ) {}

    return( null );
} // remove


} // class XactPool


} // class JdbcXAConn
