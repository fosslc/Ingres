/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcCPDataSource.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcCPDataSource.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/


import	java.sql.SQLException;
import	javax.naming.Reference;
import	javax.naming.NamingException;
import	javax.sql.ConnectionPoolDataSource;
import	javax.sql.PooledConnection;


/*
** Name: EdbcCPDataSource
**
** Description:
**	EDBC JDBC extension class which implements the JDBC 
**	ConnectionPoolDataSource interface.
**
**	This class is a subclass of EdbcDataSource and so may be
**	used as a DataSource object and supports all the properties
**	of the EdbcDataSource class.
**
**  Interface Methods:
**
**	getPooledConnection	Generate a PooledConnection object.
**	getReference		Returns JNDI Reference to EdbcDataSource object.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public class
EdbcCPDataSource
    extends EdbcDataSource
    implements ConnectionPoolDataSource
{


/*
** Name: EdbcCPDataSource
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
**	29-Jan-01 (gordy)
**	    Created.
*/

public
EdbcCPDataSource() 
{
    title = "EDBC-PoolDataSource";
    tr_id = "PoolDS";
} // EdbcCPDataSource


/*
** Name: getPooledConnection
**
** Description:
**	Generate a pooled connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	PooledConnection    A pooled connection.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public PooledConnection
getPooledConnection()
    throws SQLException
{
    return( getPooledConnection( null, null ) );
} // getPooledConnection


/*
** Name:getPooledConnection
**
** Description:
**	Generate a pooled connection.
**
** Input:
**	user	    User ID.
**	password    Password.
**
** Output:
**	None.
**
** Returns:
**	PooledConnection    A pooled connection.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public PooledConnection
getPooledConnection( String user, String password )
    throws SQLException
{
    PooledConnection conn;

    if ( trace.enabled() )
	trace.log( title + ".getPooledConnection(" + user + ")" );

    conn = new EdbcCPConnect( getConnection( user, password ), trace );

    if (trace.enabled()) trace.log(title + ".getPooledConnection(): " + conn);
    return( conn );
} // getPooledConnection


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
**	29-Jan-01 (gordy)
**	    Created.
*/

public Reference
getReference()
    throws NamingException
{
    Reference	ref = new Reference( this.getClass().getName(),
				     "ca.edbc.jdbcx.EdbcCPFactory", null );
    initReference( ref );
    return( ref );
} // getReference


} // class EdbcCPDataSource
