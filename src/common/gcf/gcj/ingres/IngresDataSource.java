/*
** Copyright (c) 2001, 2003 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.jdbc;

/*
** Name: IngresDataSource.java
**
** Description:
**	Defines class which implements the JDBC DataSource interface.
**
**  Classes
**
**	IngresDataSource
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Implemented as wrapper for new generic JDBC data-source.
**	15-Jul-03 (gordy)
**	    Added runtime configuration.
**	14-Feb-08 (rajus01) SIR 119917
**	    Added support for JDBC driver patch version for patch builds.
**      24-Nov-08 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to support
**            JDBC 4.0 SQLException hierarchy.
*/

import	java.io.Serializable;
import	java.sql.SQLException;
import	javax.sql.DataSource;
import	javax.naming.Reference;
import	javax.naming.Referenceable;
import	javax.naming.NamingException;
import	com.ingres.gcf.jdbc.JdbcDS;
import	com.ingres.gcf.util.Config;
import	com.ingres.gcf.util.TraceLog;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: IngresDataSource
**
** Description:
**	The Ingres JDBC DataSource class.  Extends and customizes the
**	generic JDBC DataSource class JdbcDS.
**
**  Overridden Methods:
**
**	loadConfig		Provide config properties to super-class.
**	loadDriverName		Provide Ingres driver name to super-class.
**	loadProtocolID		Provide Ingres protocol ID to super-class.
**	loadTraceName		Provide Ingres tracing name to super-class.
**	loadTraceLog		Provide tracing log to super-class.
**
**  Interface Methods:
**
**	getReference		Get JNDI Reference to IngresDataSource object.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Implemented as wrapper for new generic JDBC data-source.
**	15-Jul-03 (gordy)
**	    Replaced setDriverIdentity() with load methods.
**      14-Feb-08 (rajus01) SIR 119917
**          Added driverPatchVersrion to setDriverVers(). 
*/

public class
IngresDataSource
    extends JdbcDS
    implements	DataSource, Serializable, Referenceable, IngConst, GcfErr
{


/*
** Name: <class initializer>
**
** Description:
**	Initializes tracing.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Added runtime configuration.
*/

static 
{
    /*
    ** Initialize Ingres driver tracing.
    */
    IngConfig.setDriverVers( driverMajorVersion, driverMinorVersion, driverPatchVersion );

} // <class initializer>


/*
** Name: IngresDataSource
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
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Initialize and configure generic super-class.
**	15-Jul-03 (gordy)
**	    Super-class now loads identity information via load methods.
*/

public
IngresDataSource()
{
} // IngresDataSource


/*
** Name: IngresDataSource
**
** Description:
**	Class constructor used by class object factory IngresDSFactory.
**
** Input:
**	ref	Reference to use for initialization.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
**	15-Jul-03 (gordy)
**	    Don't need to call other constructor.
*/

// package access.
IngresDataSource( Reference ref ) 
{
    initInstance( ref );	// Restore instance properties.
} // IngresDataSource


/*
** The following methods implement the abstract methods of the
** super-class to provide driver identity information.
**
**	loadConfig		Configuration properties.
**	loadDriverName		Full name of driver.
**	loadProtocolID		Protocol accepted by driver.
**	loadTraceName		Name of driver for tracing.
**	loadTraceLog		Tracing log.
**
** History:
**	15-Jul-03 (gordy)
**	    Replaced setDriverIdentity() with load methods.
*/

protected Config
loadConfig()
{ return( IngConfig.getConfig() ); }

protected String
loadDriverName()
{ return( ING_DRIVER_NAME ); }

protected String
loadProtocolID()
{ return( ING_PROTOCOL_ID ); }

protected String
loadTraceName()
{ return( ING_TRACE_NAME ); }

protected TraceLog
loadTraceLog()
{ return( IngTrace.getTraceLog() ); }


/*
** Name: getReference
**
** Description:
**	Returns a JNDI Reference to this object and associated 
**	factory class.  Encodes property values in Reference 
**	which may be restored by using the class constructor 
**	that takes a Reference parameter.
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
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for Ingres data-source.
**	15-Jul-03 (gordy)
**	    Get class name directly from class rather than constant.
*/

public Reference
getReference()
    throws NamingException
{
    Reference	ref = new Reference( this.getClass().getName(),
				     IngresDSFactory.class.getName(), null );
    initReference( ref );
    return( ref );
} // getReference

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
**      24-Nov-08 (rajus01)
**         Created.
**	23-Jun-09 (rajus01)
**	   Implemented.  
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

}

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
**      24-Nov-08 (rajus01)
**         Created.
**	23-Jun-09 (rajus01)
**	   Implemented.
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
}

} // class IngresDataSource
