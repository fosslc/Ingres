/*
** Copyright (c) 2006, 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcQPMD.java
**
** Description:
**	Defines class which implements the JDBC ParameterMetaData interface.
**
**  Classes:
**
**	JdbcQPMD
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
**	25-Aug-06 (gordy)
**	    Support procedure parameter meta-data.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
*/

import	java.sql.ParameterMetaData;
import	java.sql.ResultSetMetaData;
import	java.sql.SQLException;
import	com.ingres.gcf.dam.MsgConst;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: JdbcQPMD
**
** Description:
**	JDBC driver class which implements the ParameterMetaData interface.
**
**	The GCD Server returns dynamic parameter info using the same
**	format as result column info, so this class is implemented as
**	a cover for an underlying JdbcRSMD object loaded with the
**	DESCRIBE response data.  An empty JdbcRSMD object is created
**	if there no parameter info is returned by the server.
**
**  Interface Methods:
**
**	getParameterCount	Returns the number of dynamic parameters.
**	getParameterMode	Returns the mode of a parameter.
**	getParameterType	Returns SQL type for a parameter.
**	getParameterTypeName	Returns host DBMS type for a parameter.
**	getParameterClassName	Returns class name of object (getObject()).
**	getPrecision		Returns number of digits for a numeric param.
**	getScale		Returns number of digits after decimal point.
**	isNullable		Determine if parameter is nullable.
**	isSigned		Determine if parameter is a signed number.
**
**  Private Data:
**
**	rsmd			Parameter descriptors.
**	defaultMode		Default parameter mode.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
**	25-Aug-06 (gordy)
**	    Added constructors for common initialization and
**	    procedure parameter meta-data.  Added defaultMode.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
*/

public class
JdbcQPMD
    extends	DrvObj
    implements	ParameterMetaData, MsgConst
{

    private JdbcRSMD		rsmd = null;
    private int			defaultMode = parameterModeUnknown;

    
/*
** Name: JdbcQPMD
**
** Description:
**	Class constructor.  Describe input parameters and build meta-data.
**
** Input:
**	conn	    Associated connection.
**	stmt_name   Prepared statement to be described.
**
** Output:
**	None.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
**	25-Aug-06 (gordy)
**	    Default parameter mode is IN for input parameters.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcQPMD( DrvConn conn, String stmt_name )
    throws SQLException
{
    this( conn );
    defaultMode = parameterModeIn;
    
    if ( conn.msg_protocol_level < MSG_PROTO_5 )
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );

    if ( trace.enabled( 2 ) )
	trace.write( tr_id + ": describing statement '" + stmt_name + "'" );
    
    msg.lock();
    try
    {
	msg.begin( MSG_QUERY );
	msg.write( MSG_QRY_DESC );
	msg.write( MSG_QP_STMT_NAME );
	msg.write( stmt_name );
	msg.done( true );

	rsmd = readResults();
    }
    finally 
    { 
	msg.unlock(); 
    }

    /*
    ** If no meta-data was returned from the describe,
    ** provide an empty meta-data object for easy handling.
    */
    if ( rsmd == null )  rsmd = new JdbcRSMD( (short)0, trace );

} // JdbcQPMD


/*
** Name: JdbcQPMD
**
** Description:
**	Class constructor.  Describe procedure parameters and 
**	build meta-data.
**
** Input:
**	conn	    Associated connection.
**	procSchema  Procedure Schema, may be NULL.
**	procName    Procedure Name.
**
** Output:
**	None.
**
** History:
**	25-Aug-06 (gordy)
**	    Created.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
*/

// package access
JdbcQPMD( DrvConn conn, String procSchema, String procName )
    throws SQLException
{
    this( conn );
    
    if ( conn.msg_protocol_level < MSG_PROTO_5 )
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );

    if ( trace.enabled( 2 ) )
	if ( procSchema != null )
	    trace.write( tr_id + ": describing procedure '" + 
			 procSchema + "." + procName + "'" );
	else
	    trace.write( tr_id + ": describing procedure '" + procName + "'" );
    
    msg.lock();
    try
    {
 	msg.begin( MSG_REQUEST );
	msg.write( MSG_REQ_PARAM_INFO );
	
	if ( procSchema != null )
	{
	    msg.write( (short)MSG_RQP_SCHEMA_NAME );
	    msg.write( procSchema );
	}
	
	msg.write( (short)MSG_RQP_PROC_NAME );
	msg.write( procName );
	msg.done( true );

	rsmd = readResults();
    }
    finally 
    { 
	msg.unlock(); 
    }

    /*
    ** If no meta-data was returned from the describe,
    ** provide an empty meta-data object for easy handling.
    */
    if ( rsmd == null )  rsmd = new JdbcRSMD( (short)0, trace );

} // JdbcQPMD


/*
** Name: JdbcQPMD
**
** Description:
**	Class constructor.  General initialization.
**
** Input:
**	conn	    Associated connection.
**
** Output:
**	None.
**
** History:
**	25-Aug-06 (gordy)
**	    Created.
*/

private
JdbcQPMD( DrvConn conn )
{
    super( conn );
    title = trace.getTraceName() + "-JdbcQPMD[" + inst_id + "]";
    tr_id = "QPMD[" + inst_id + "]";
}


/*
** Name: getParameterCount
**
** Description:
**	Returns the number of parameter in the associated result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Number of parameters.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
*/

public int
getParameterCount()
    throws SQLException
{
    int count = rsmd.getColumnCount();
    if ( trace.enabled() )  
	trace.log( title + ".getParameterCount(): " + count );
    return( count );
} // getParameterCount


/*
** Name: getParameterMode
**
** Description:
**	Returns the mode of a prepared statement dynamic parameter.
**	Returns one of the following values:
**
**	    parameterModeIn
**	    parameterModeOut
**	    parameterModeInOut
**	    parameterModeUnknown
**
** Input:
**	param	Parameter index
**
** Output:
**	None.
**
** Returns:
**	int	Parameter mode.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
**	25-Aug-06 (gordy)
**	    Check parameter mode, provide default if unknown.
*/

public int
getParameterMode( int param )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getParameterMode( " + param + " )" );

    /*
    ** If parameter does not have an explicit mode, provide default.
    */
    int mode = rsmd.getParameterMode( param );
    if ( mode == parameterModeUnknown )  mode = defaultMode;

    if ( trace.enabled() )  
	trace.log( title + ".getParameterMode: " + mode );
    return( mode );
} // getParameterMode


/*
** Name: getParameterType
**
** Description:
**	Returns the SQL type of a prepared statement dynamic parameter.
**
** Input:
**	param	Parameter index
**
** Output:
**	None.
**
** Returns:
**	int	Parameter SQL type (java.sql.Types).
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
*/

public int
getParameterType( int param )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getParameterType( " + param + " )" );

    int type = rsmd.getColumnType( param );

    if ( trace.enabled() )  
	trace.log( title + ".getParameterType: " + type );
    return( type );
} // getParameterType


/*
** Name: getParameterTypeName
**
** Description:
**	Returns the host DBMS type for a parameter.
**
** Input:
**	param	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	String	DBMS type.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
*/

public String
getParameterTypeName( int param )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getParameterTypeName( " + param + " )" );

    String type = rsmd.getColumnTypeName( param );

    if ( trace.enabled() )  
	trace.log( title + ".getParameterTypeName: " + type );
    return( type );
} // getParameterTypeName


/*
** Name: getParameterClassName
**
** Description:
**	Returns the Java class name of the object returned by
**	getObject() for a parameter.
**
** Input:
**	param	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	String	Class name.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
*/

public String
getParameterClassName( int param )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getParameterClassName( " + param + " )" );
    
    String name = rsmd.getColumnClassName( param );

    if ( trace.enabled() )  
	trace.log( title + ".getParameterClassName: " + name );
    return( name );
} // getParameterClassName


/*
** Name: getPrecision
**
** Description:
**	Returns the number of decimal digits in a numeric parameter.
**
** Input:
**	param	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	int	Number of numeric digits.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
*/

public int
getPrecision( int param )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getPrecision( " + param + " )" );

    int precision = rsmd.getPrecision( param );

    if ( trace.enabled() )  trace.log( title + ".getPrecision: " + precision );
    return( precision );
} // getPrecision


/*
** Name: getScale
**
** Description:
**	Returns the number of numeric digits following the decimal point.
**
** Input:
**	param	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	int	Number of scale digits.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
*/

public int
getScale( int param )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getScale( " + param + " )" );

    int scale = rsmd.getScale( param );

    if ( trace.enabled() )  trace.log( title + ".getScale: " + scale );
    return( scale );
} // getScale


/*
** Name: isNullable
**
** Description:
**	Determine if parameter is nullable.  Returns one of the following:
**
**	    parameterNoNulls
**	    parameterNullable
**	    parameterNullableUnknown
**
** Input:
**	param	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	int	Parameter nullability.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
*/

public int
isNullable( int param )
    throws SQLException
{
    if ( trace.enabled() )  trace.log(title + ".isNullable( " + param + " )");
    int nulls = parameterNullableUnknown;

    switch( rsmd.isNullable( param ) )
    {
    case ResultSetMetaData.columnNullable : nulls = parameterNullable;	break;
    case ResultSetMetaData.columnNoNulls :  nulls = parameterNoNulls;	break;
    }

    if ( trace.enabled() )  trace.log( title + ".isNullable: " + nulls );
    return( nulls );
} // isNullable


/*
** Name: isSigned
**
** Description:
**	Determine if parameter is a signed numeric.
**
** Input:
**	param	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if parameter is a signed numeric, false otherwise.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
*/

public boolean
isSigned( int param )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".isSigned( " + param + " )" );

    boolean signed = rsmd.isSigned( param );

    if ( trace.enabled() )  trace.log( title + ".isSigned: " + signed );
    return( signed );
} // isSigned


/*
** Name: readDesc
**
** Description:
**	Read a query result DESC message.  Overrides the default 
**	method in DrvObj since descriptors are expected.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD    Query result data descriptor.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
*/

protected JdbcRSMD
readDesc()
    throws SQLException
{
    return( JdbcRSMD.load( conn ) );
} // readDesc

/*
** Name: isWrapperFor
**
** Description:
**      Returns true if this either implements the interface argument or is
**      directly or indirectly a wrapper for an object that does. Returns
**      false otherwise.
**
** Input:
**      iface   Class defining an interface
**
** Output:
**      None.
**
** Returns:
**      true or false.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public boolean
isWrapperFor(Class<?> iface)
throws SQLException
{
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
	return( iface.isInstance( this ) );

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);
} //isWrapperFor

/*
** Name: unwrap
**
** Description:
**      Returns an object that implements the given interface to allow
**      access to non-standard methods, or standard methods not exposed by the
**      proxy. If the receiver implements the interface then the result is the
**      receiver or a proxy for the receiver. If the receiver is a wrapper and
**      the wrapped object implements the interface then the result is the
**      wrapped object or a proxy for the wrapped object. Otherwise return the
**      the result of calling unwrap recursively on the wrapped object or
**      a proxy for that result. If the receiver is not a wrapper and does not
**      implement the interface, then an SQLException is thrown.
**
** Input:
**      iface   A Class defining an interface that the result must implement.
**
** Output:
**      None.
**
** Returns:
**      An object that implements the interface.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public <T> T
unwrap(Class<T> iface)
throws SQLException
{
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
    {
	if( ! iface.isInstance( this ) )
	    throw SqlExFactory.get(ERR_GC4023_NO_OBJECT); 
	return( iface.cast(this) );
    }

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);
} //unwrap


} // class JdbcQPMD
