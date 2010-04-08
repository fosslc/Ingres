/*
** Copyright (c) 2001, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcXAVirt.java
**
** Description:
**	Defines class which implements the JDBC Connection interface
**	as a virtual cover for an XA connection.
**
**  Classes:
**
**	JdbcXAVirt
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	18-Feb-03 (gordy)
**	    Upgrading to JDBC 3.0.
**	20-Jul-06 (gordy)
**	    Added ability to change the underlying physical connection.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	06-Feb-09 (rajus01) SIR 121238
**	    Removed the set/getClientInfo() methods as they are inherited
**	    from JdbcCPVirt class and should be implemented here only if the
**	    implementation differ from that of parent class methods. Related
**	    piccolo change is 496050.
*/

import	java.sql.Connection;
import	java.sql.Savepoint;
import	java.sql.SQLException;
import	java.sql.Blob;
import	java.sql.Clob;
import	java.sql.Array;
import	java.sql.NClob;
import	java.sql.SQLXML;
import	java.sql.Struct;
import	java.sql.Wrapper;
import  java.util.Map;
import  java.util.HashMap;
import  java.util.Iterator;
import	java.util.Properties;
import	javax.sql.PooledConnection;
import	javax.sql.ConnectionEventListener;
import	com.ingres.gcf.util.SqlExFactory;
import  com.ingres.gcf.util.SqlExType;
import	com.ingres.gcf.util.GcfErr;


/*
** Name: JdbcXAVirt
**
** Description:
**	JDBC driver class which implements the JDBC Connection interface 
**	as a virtual cover for an XA connection.
**
**	Transaction operations are controlled based on distributed 
**	transaction state of the connection.
**
**  Interface Methods:
**
**	setAutoCommit		Set autocommit state.
**	commit			Commit current transaction.
**	rollback		Rollback current transaction.
**	setSavepoint		Create savepoint.
**
**  Public Methods:
**
**	setActiveDTX		Set distributed transaction state.
**	setConnection		Set physical connection.
**
**  Private Data:
**
**	activeDTX		Distributed Transaction state.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	18-Feb-03 (gordy)
**	    Upgrading to JDBC 3.0: Added setActiveDTX() to permit XA 
**	    connection to change distributed transaction state after 
**	    virtual connection is instantiated.  Added setSavepoint(), 
**	    rollback( savepoint ).
**	20-Jul-06 (gordy)
**	    Added setConnection().
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
*/

public class
JdbcXAVirt
    extends JdbcCPVirt
    implements Connection, GcfErr
{

    private boolean		activeDTX = false;  // Distributed Transaction?


/*
** Name: JdbcXAVirt
**
** Description:
**	Class constructor.
**
** Input:
**	conn	    JDBC connection.
**	pool	    Pooled connection owner.
**	listener    Connection event listener.
**	trace	    Connection tracing.
**	activeDTX   Distributed Transaction state.
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
JdbcXAVirt
( 
    Connection		    conn,
    PooledConnection	    pool,
    ConnectionEventListener listener,
    DrvTrace		    trace,
    boolean		    activeDTX
)
{
    super( conn, pool, listener, trace );
    this.activeDTX = activeDTX;
    title = trace.getTraceName() + "-XAVConnection[" + inst_id + "]";
    tr_id = "XAVirt[" + inst_id + "]";

    if ( trace.enabled( 2 ) )
	trace.write( tr_id + "( " + conn + ", " + activeDTX + " )" );

    return;
} // JdbcXAVirt


/*
** Name: setActiveDTX
**
** Description:
**	Set the distributed transaction state.
**
** Input:
**	activeDTX	Distributed transaction state.
**
** Output:
**	None.
**
** Returns:
**	boolean		Previous state.
**
** History:
**	18-Feb-03 (gordy)
**	    Created.
*/

boolean // package access
setActiveDTX( boolean activeDTX )
{
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".setActiveDTX( " + activeDTX + " )" );

    boolean prev = this.activeDTX;
    this.activeDTX = activeDTX;
    return( prev );
} // setActiveDTX


/*
** Name: setConnection
**
** Description:
**	Set the physical connection underlying the virtual connection.
**
** Input:
**	conn		Connection.
**
** Output:
**	None.
**
** Returns:
**	Connection	Previous connection.
**
** History:
**	20-Jul-06 (gordy)
**	    Created.
*/

Connection // package access
setConnection( Connection conn )
{
    if ( trace.enabled( 3 ) )
	trace.write( tr_id + ".setConnection( " + conn + " )" );

    Connection prev = this.conn;
    this.conn = conn;
    return( prev );
} // setConnection


/*
** Name: setAutoCommit
**
** Description:
**	Set the autocommit state.
**
** Input:
**	autocommit  True to set autocommit ON, false for OFF.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public void
setAutoCommit( boolean autoCommit )
    throws SQLException
{
    if ( activeDTX  &&  autoCommit )  
    {
	if ( trace.enabled() )  trace.log( title + 
	    ".setAutoCommit(): not permitted during distributed transaction" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    super.setAutoCommit( autoCommit );
    return;
} // setAutoCommit


/*
** Name: commit
**
** Description:
**	Commit the current transaction
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
commit()
    throws SQLException
{
    if ( activeDTX )  
    {
	if ( trace.enabled() )  trace.log( title + 
	    ".commit(): commit must be done by transaction manager!" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    super.commit();
    return;
} // commit


/*
** Name: rollback
**
** Description:
**	Rollback the current transaction
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
rollback()
    throws SQLException
{
    if ( activeDTX )  
    {
	if ( trace.enabled() )  trace.log( title + 
	    ".rollback(): rollback must be done by transaction manager!" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    super.rollback();
    return;
} // rollback


/*
** Name: rollback
**
** Description:
**	Rollback to savepoint.
**
** Input:
**	savepoint   Target savepoint.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

public void
rollback( Savepoint savepoint )
    throws SQLException
{
    if ( activeDTX )  
    {
	if ( trace.enabled() )  trace.log( title + ".rollback(" + savepoint +
	    "): rollback must be done by transaction manager!" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    super.rollback( savepoint );
    return;
} // rollback


/*
** Name: setSavepoint
**
** Description:
**	Creates an unnamed savepoint.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Savepoint	The unnamed savepoint.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

public Savepoint
setSavepoint()
    throws SQLException
{
    if ( activeDTX )  
    {
	if ( trace.enabled() )  trace.log( title + 
	    ".setSavepoint(): savepoints not allowed in distributed xact!" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    return( super.setSavepoint() );
} // setSavepoint


/*
** Name: setSavepoint
**
** Description:
**	Creates a named savepoint.
**
** Input:
**	name	Savepoint name.
**
** Output:
**	None.
**
** Returns:
**	Savepoint	The named savepoint.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

public Savepoint
setSavepoint( String name )
    throws SQLException
{
    if ( activeDTX )  
    {
	if ( trace.enabled() )  trace.log( title + ".setSavepoint('" + name + 
	    "'): savepoints not allowed in distributed xact!" );
	throw SqlExFactory.get( ERR_GC401F_XACT_STATE );
    }

    return( super.setSavepoint( name ) );
} // setSavepoint

} // class JdbcXAVirt
