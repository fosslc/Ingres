/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcXAVirtConn.java
**
** Description:
**	Implements the EDBC JDBC extension class: EdbcXAVirtConn.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	12-Feb-03 (gordy)
**	    Added setActiveDTX() to allow change in transaction state
**	    as required by JDBC 3.0 XA data source spec.
**          SIR109798
*/

import	java.sql.Connection;
import	java.sql.SQLException;
import	javax.sql.PooledConnection;
import	javax.sql.ConnectionEventListener;
import	ca.edbc.jdbc.EdbcTrace;
import	ca.edbc.util.EdbcEx;


/*
** Name: EdbcXAVirtConn
**
** Description:
**	EDBC JDBC extension class which implements the JDBC 
**	Connection interface as a cover for another JDBC 
**	connection.
**
**	Transaction operations are controlled based on 
**	distributed transaction state of the connection.
**
**  Interface Methods:
**
**	setAutoCommit		Set autocommit state.
**	commit			Commit current transaction.
**	rollback		Rollback current transaction.
**
**  Public Methods:
**
**	setActiveDTX		Set distributed transaction state.
**
**  Private Data:
**
**	activeDTX		Distributed Transaction state.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	12-Feb-03 (gordy)
**	    Added setActiveDTX() to allow change in transaction state
**	    as required by JDBC 3.0 XA data source spec.
*/

class
EdbcXAVirtConn
    extends EdbcCPVirtConn
{

    private boolean		activeDTX = false;  // Distributed Transaction?


/*
** Name: EdbcXAVirtConn
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
*/

public
EdbcXAVirtConn
( 
    Connection		    conn,
    PooledConnection	    pool,
    ConnectionEventListener listener,
    EdbcTrace		    trace,
    boolean		    activeDTX
)
{
    super( conn, pool, listener, trace );
    this.activeDTX = activeDTX;
    title = "EDBC-XAVirtConn[" + inst_id + "]";
    tr_id = "XaVirt[" + inst_id + "]";
} // EdbcXAVirtConn


/*
** Name: setActiveDTX
**
** Description:
**	Set the distributed transaction state.
**
** Input:
**	activeDTX   Distributed transaction state.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Previous state.
**
** History:
**	12-Feb-03 (gordy)
**	    Created.
*/

boolean // package access
setActiveDTX( boolean activeDTX )
{
    boolean prev = this.activeDTX;
    this.activeDTX = activeDTX;
    return( prev );
} // setActiveDTX


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
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
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
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
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
	throw EdbcEx.get( E_JD0012_UNSUPPORTED );
    }

    super.rollback();
    return;
} // rollback


} // class EdbcXAVirtConn
