/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcXAConnect.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcXAConnect.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	12-Apr-01 (gordy)
**	    Exceptions are now self tracing.
**	18-Apr-01 (gordy)
**	    Adjustment to tracing to work with EdbcXADataSource.
**      12-Feb-03 (gordy)
**	    JDBC 3.0 XA data source spec permits start() to be called
**	    after getConnection().
**          SIR109798
*/


import	java.sql.Connection;
import	java.sql.SQLException;
import	javax.sql.XAConnection;
import	javax.sql.ConnectionEvent;
import	javax.transaction.xa.XAResource;
import	javax.transaction.xa.Xid;
import	javax.transaction.xa.XAException;
import	ca.edbc.jdbc.EdbcConnect;
import	ca.edbc.jdbc.EdbcTrace;
import	ca.edbc.util.EdbcEx;


/*
** Name: EdbcXAConnect
**
** Description:
**	EDBC JDBC extension class which implements the JDBC 
**	XAConnection interface.  Also implements the XAResource
**	interface since there is one-to-one relation between
**	XAConnection and XAResource.
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
**  Constants:
**
**	TX_ST_IDLE		Trans State: no transaction.
**	TX_ST_ACTIVE		Trans State: transaction started.
**	TX_ST_WAIT		Trans State: transaction ended.
**	TX_ST_READY		Trans State: transaction prepared.
**	TX_ST_FAIL		Trans State: transaction failed.
**	TX_ST_ABORT		Trans State: transaction aborted.
**
**  Private Data:
**
**	xarm			XA Resource Manager.
**	tx_state		Current transaction state.
**	xid			Current transaction ID.
**	noXIDs			Empty array of transaction IDs.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**      14-Dec-04 (rajus01) Startrak #EDJDBC94; Bug # 113625
**	    Added xa_thread to store the thread ID at the begining of the
**	    transaction. Added TX_ST_ABORT transaction state.
*/

class
EdbcXAConnect
    extends EdbcCPConnect
    implements	XAConnection, XAResource, EdbcDSErr
{

    private static final int	TX_ST_IDLE	= 0;	// No transaction
    private static final int	TX_ST_ACTIVE	= 1;	// start().
    private static final int	TX_ST_WAIT	= 2;	// end( SUCCESS ).
    private static final int	TX_ST_READY	= 3;	// prepare().
    private static final int	TX_ST_FAIL	= 4;	// end( FAIL ).
    private static final int	TX_ST_ABORT	= 5;	// abort( rollback ).

    private static final Xid	noXIDs[] = new Xid[ 0 ];

    private EdbcXARM		xarm = null;
    private int			tx_state = TX_ST_IDLE;
    private EdbcXAXid		xid = null;

    private Thread		xa_thread = null;


/*
** Name: EdbcXAConnect
**
** Description:
**	Class constructor.
**
** Input:
**	conn	Connection.
**	xarm	EDBC XA Resource Manager.
**	trace	Connection tracing.
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
EdbcXAConnect( Connection conn, EdbcXARM xarm, EdbcTrace trace ) 
    throws SQLException
{
    super( conn, trace );
    this.xarm = xarm;
    title = "EDBC-XAConnection[" + inst_id + "]";
    tr_id = "XaConn[" + inst_id + "]";
} // EdbcXAConnect


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
    boolean dtx = (tx_state != TX_ST_IDLE);

    if ( physConn == null )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getConnection(): connection closed!" );
	throw EdbcEx.get( E_JD0004_CONNECTION_CLOSED );
    }

    if ( trace.enabled() )  trace.log( title + ".getConnection()" );
    clearConnection();

    /*
    ** Autocommit must be enabled for connections which
    ** are not participating in a distributed transaction.
    */
    if ( ! dtx  &&  ! physConn.getAutoCommit() ) physConn.setAutoCommit(true);
    virtConn = new EdbcXAVirtConn( physConn, this, this, trace, dtx );

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
**	31-Jan-01 (gordy)
**	    Created.
*/

public void
close()
    throws SQLException
{
    if ( physConn != null  &&  xid != null )
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ": aborting transaction on close request!" );
	tx_state = TX_ST_FAIL;
	try { rollback( xid ); } catch( Exception ignore ) {}
    }

    super.close();
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
    if ( physConn == null )  throw EdbcEx.get( E_JD0004_CONNECTION_CLOSED );
    return( this );
} // getXAResource


/*
** Name: isSameRM
**
** Description:
**	Is the same Resource Manager referenced by another XAResource.
**
**	XAResource's with the same RM may be given an XID with the same
**	branch qualifier.  Each connection should receive unique branch
**	qualifiers so that their part of the distributed transaction is
**	idependently prepared and committed.  So, contrary to the JDBC
**	spec where XAResource's from the same DataSource are assumed to
**	reference the same RM, only our own XAResource reference will
**	result in a valid RM match.  Hopefully, this may also limit the
**	calls to prepare/commit an XID to the XAResource holding the XID
**	(JDBC spec permits these calls to any XAResource associated with
**	the same DataSource).
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
*/

public boolean
isSameRM( XAResource xares )
{
    return( this == xares );
} // isSameRM


/*
** Name: start
**
** Description:
**	Start a Distributed Transaction.
**
**	We don't really expect TMJOIN and TMRESUME to be used.
**	So, even though some support for these operations could
**	be made, for the time being only new transactions are
**	supported.
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
**	12-Feb-03 (gordy)
**	    JDBC 3.0 XA data source spec permits start() to be called
**	    after getConnection().
**	08-Jan-04 (rajus01) startak problem # EDJDBC79 ; Bug # 111571
**	   Don't throw "invalid transaction state" exception if there is
**	   a virtual connection and if autocommit is disabled.
**	29-Mar-04 (rajus01) Star # EDJDBC86; Bug # 112045
**	   Added support for TMJOIN flag.
**      14-Dec-04 (rajus01) Startrak #EDJDBC94; Bug # 113625
**	   Thread ID is stored at the start of the transaction. 
**	   RMFAIL exception is thrown when 
**	      1. The transaction state is TX_ST_ABORT.
**	      2. The connection fails to participate in the distributed 
**	         transaction. 
**
*/

public void
start( Xid xid, int flags )
    throws XAException
{
    boolean	autocommit;
    EdbcXAXid	xaxid = (xid instanceof EdbcXAXid) 
			? (EdbcXAXid)xid : new EdbcXAXid(xid);

    xa_thread = Thread.currentThread();

    if ( trace.enabled() )  
    {
	trace.log( title + ".start()" );
	if ( trace.enabled( 3 ) )  xaxid.trace( trace );
    }
   
    if( tx_state == TX_ST_ABORT )    
	throw new XAException( XAException.XAER_RMFAIL );

    if( flags == TMJOIN &&
        xaxid.equals(this.xid) &&
        tx_state == TX_ST_WAIT )
    {
         tx_state = TX_ST_ACTIVE;
         return;
    }

    if ( flags != TMNOFLAGS )  
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".start(): flags not supported" );
	throw new XAException( XAException.XAER_INVAL );
    }

    try { autocommit = physConn.getAutoCommit(); }
    catch( SQLException ex ) { autocommit = true; }

    if ( physConn == null  ||  tx_state != TX_ST_IDLE ) 
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".start(): invalid transaction state" );
	throw new XAException( XAException.XAER_PROTO );
    }
 
    /*
    ** Register this XAResource as servicing the XID.
    */
    xarm.registerXID( this, xaxid );
    this.xid = xaxid;

    try 
    { 
	/*
	** Autocommit must be disabled for connections which
	** are participating in a distributed transaction.
	*/
	if ( autocommit )  physConn.setAutoCommit( false );

	/*
	** Start connection participation in distributed transaction.
	*/
	((EdbcConnect)physConn).startTransaction( this.xid ); 
	if ( virtConn != null )  
	    ((EdbcXAVirtConn)virtConn).setActiveDTX( true );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )  
	{
	    trace.write( tr_id + ".start: error starting transaction" );
	    ((EdbcEx)ex).trace( trace );
	}

	xarm.deregisterXID( this, this.xid );
	this.xid = null;
	throw new XAException( XAException.XAER_RMFAIL );
    }

    tx_state = TX_ST_ACTIVE;	// Distributed transaction started.
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
**      14-Dec-04 (rajus01) Startrak #EDJDBC94; Bug # 113625
**	    If the transaction state is ABORT while ending the transaction,
**	    then XA_RBROLLBACK exception is thrown. 
*/

public void
end( Xid xid, int flags )
    throws XAException
{

    if ( trace.enabled() )  
    {
	trace.log( title + ".end()" );
	if ( trace.enabled( 3 ) )  
	{
	    EdbcXAXid	xaxid = (xid instanceof EdbcXAXid) 
				? (EdbcXAXid)xid : new EdbcXAXid( xid );
	    xaxid.trace( trace );
	}

    }

    if( tx_state == TX_ST_ABORT )
       throw new XAException( XAException.XA_RBROLLBACK );

    if ( flags == TMSUSPEND )  
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".end(): flags not supported" );
	throw new XAException( XAException.XAER_INVAL );
    }

    if ( physConn == null  ||  tx_state != TX_ST_ACTIVE )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".end(): invalid transaction state" );
	throw new XAException( XAException.XAER_PROTO );
    }
    
    if ( ! this.xid.equals( xid ) )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".end(): invalid XID" );
	throw new XAException( XAException.XAER_NOTA );
    }

    tx_state = (flags == TMFAIL) ? TX_ST_FAIL : TX_ST_WAIT;
    return;
} // end


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
**      14-Dec-04 (rajus01) Startrak #EDJDBC94; Bug # 113625
**	    If the transaction state is ABORT while preparing the transaction,
**	    then XA_RBROLLBACK exception is thrown. 
*/

public int
prepare( Xid xid )
    throws XAException
{
    EdbcXAXid	xaxid = (xid instanceof EdbcXAXid) 
			? (EdbcXAXid)xid : new EdbcXAXid(xid);

    if ( trace.enabled() )  
    {
	trace.log( title + ".prepare()" );
	if ( trace.enabled( 3 ) )  xaxid.trace( trace );
    }
    
    if( tx_state == TX_ST_ABORT )
       throw new XAException( XAException.XA_RBROLLBACK );

    if ( this.xid == null  ||  ! xaxid.equals( this.xid ) )
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".prepare(): passing request to XARM" );
	return( xarm.prepareXID( xaxid ) );
    }

    if ( physConn == null  ||  tx_state != TX_ST_WAIT )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".prepare(): invalid transaction state" );
	throw new XAException( XAException.XAER_PROTO );
    }

    try 
    { 
	((EdbcConnect)physConn).prepareTransaction(); 
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".prepare(): error preparing transaction" );
	    ((EdbcEx)ex).trace( trace );
	}

	tx_state = TX_ST_FAIL;
	throw new XAException( XAException.XAER_RMERR );
    }

    tx_state = TX_ST_READY;
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
**	08-Jan-04 (rajus01) startak problem # EDJDBC79 ; Bug # 111571
**	    After commiting distributed transaction, enable autocommit 
**	    (default for connections not participating in distributed 
**	    transactions) if virtual connection exists. 
**      14-Dec-04 (rajus01) Startrak #EDJDBC94; Bug # 113625
**	    If the transaction state is ABORT while committing the transaction,
**	    then XA_RBROLLBACK exception is thrown. 
*/

public void
commit( Xid xid, boolean onePhase )
    throws XAException
{
    EdbcXAXid	xaxid = (xid instanceof EdbcXAXid) 
			? (EdbcXAXid)xid : new EdbcXAXid(xid);

    if ( trace.enabled() )  
    {
	trace.log( title + ".commit()" );
	if ( trace.enabled( 3 ) )  xaxid.trace( trace );
    }

    if( tx_state == TX_ST_ABORT )
       throw new XAException( XAException.XA_RBROLLBACK );

    if ( this.xid == null  ||  ! xaxid.equals( this.xid ) )
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".commit(): passing request to XARM" );
	xarm.commitXID( xaxid, onePhase );
	return;
    }

    if ( 
	 physConn == null  ||  
	 ( onePhase  &&  tx_state != TX_ST_WAIT )  ||
	 ( ! onePhase  &&  tx_state != TX_ST_READY ) 
       )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".commit(): invalid transaction state" );
	throw new XAException( XAException.XAER_PROTO );
    }
    
    try 
    { 
         physConn.commit(); 
         if(virtConn != null ) physConn.setAutoCommit(true);
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".commit(): error committing transaction" );
	    ((EdbcEx)ex).trace( trace );
	}

	try { physConn.rollback(); } catch( SQLException ignore ) {}
	throw new XAException( XAException.XAER_RMERR );
    }
    finally
    {
	xarm.deregisterXID( this, this.xid );
	this.xid = null;
	tx_state = TX_ST_IDLE;
    }
    return;
} // commit


/*
** Name: rollback
**
** Description:
**	Rollback a Distributed Transaction.
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
**	08-Jan-04 (rajus01) startak problem # EDJDBC79 ; Bug # 111571
**	    After rollback of distributed transaction, enable autocommit 
**	    (default for connections not participating in distributed 
**	    transactions) if virtual connection exists. 
**      14-Dec-04 (rajus01) Startrak #EDJDBC94; Bug # 113625
**	    Connection is aborted when rollback comes from differnet thread
**	    of control and XA_RBROLLBACK exception is thrown.
*/

public void
rollback( Xid xid )
    throws XAException
{
    EdbcXAXid	xaxid = (xid instanceof EdbcXAXid) 
			? (EdbcXAXid)xid : new EdbcXAXid(xid);

    boolean res = xa_thread.equals(Thread.currentThread() );

    if ( trace.enabled() )  
    {
	trace.log( title + ".rollback()" );
	if ( trace.enabled( 3 ) )  xaxid.trace( trace );
    }

    if ( this.xid == null  ||  ! xaxid.equals( this.xid ) )
    {
	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ".rollback(): passing request to XARM" );
	xarm.rollbackXID( xaxid );
	return;
    }

    if ( 
	 physConn != null  &&  
	 (tx_state == TX_ST_ACTIVE  &&  !res )
       )
    {
	abort( physConn, xid );
	throw new XAException( XAException.XA_RBROLLBACK );
    }

    if ( 
	 physConn == null  ||  
	 ( tx_state != TX_ST_WAIT  &&  
	   tx_state != TX_ST_READY  &&  
	   tx_state != TX_ST_FAIL )
	 )
    {
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".rollback(): invalid transaction state" );
	throw new XAException( XAException.XAER_PROTO );
    }
    
    try
    { 
	physConn.rollback(); 
	if(virtConn != null ) physConn.setAutoCommit(true);
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )
	{
	    trace.write( tr_id + ".rollback(): error aborting transaction" );
	    ((EdbcEx)ex).trace( trace );
	}

	throw new XAException( XAException.XAER_RMERR );
    }
    finally
    {
	xarm.deregisterXID( this, this.xid );
	this.xid = null;
	tx_state = TX_ST_IDLE;
    }

    return;

} // rollback

/*
** Name: abort
**
** Description:
**	Aborts the physical connection.
**
** Input:
**	Connection	Physical database connection.
**	Xid		XA Transaction ID
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	15-Dec-04 (rajus01)
**	    Created.
*/
private void 
abort( Connection physConn, Xid xid )
throws XAException
{
    if ( trace.enabled() )  
    {
	trace.log( title + ".abort()" );
    }
    ((EdbcConnect)physConn).abort();
    xarm.deregisterXID( this, this.xid );
    this.xid = null;
    tx_state = TX_ST_ABORT;

    // Notify the Connection Pool Manager that the pooled connection is
    // no longer usable.
    super.connectionErrorOccurred( new ConnectionEvent( this,
                      EdbcEx.get( E_JD0023_POOLED_CONNECTION_NOT_USABLE ) ) );
   
    return;

} // abort


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
    /*
    ** We only return transaction IDs at the start of the
    ** scan, in which case all transactions IDs are returned.
    */

    if ( (flag & TMSTARTRSCAN) == 0 )  
    {
	if ( trace.enabled() )
	    trace.log( title + ".recover(): no XIDs (non-start request)" );
	return( noXIDs );
    }

    if ( trace.enabled() )
	trace.log( title + ".recover()" );

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


} // class EdbcXAConnect
