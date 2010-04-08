/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcDSFactory.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcDSFactory.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
*/


import	java.util.Hashtable;
import	javax.naming.Context;
import	javax.naming.Name;
import	javax.naming.Reference;
import	javax.naming.RefAddr;
import	javax.naming.BinaryRefAddr;
import	javax.naming.StringRefAddr;
import	javax.naming.spi.ObjectFactory;


/*
** Name: EdbcDSFactory
**
** Description:
**	EDBC JDBC extension class which acts as an object factory 
**	for the EdbcDataSource class.
**
**  Interface Methods:
**
**	getObjectInstance   Returns EdbcDataSource generated from a Reference.
**
**  Protected Methods
**
**	initInstance	    Initialize EdbcDataSource from Reference.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
*/

public class
EdbcDSFactory
    implements ObjectFactory
{


/*
** Name: getObjectInstance
**
** Description:
**	Generates an EdbcDataSource object from a JNDI Reference.  
**	Retrieves the property values encoded by the getReference()
**	method of class EdbcDataSource.
**
** Input:
**	obj	JNDI Reference to an EdbcDataSource object.
**	name	Not used.
**	context	Not used.
**	env	Not used.
**
** Output:
**	None.
**
** Returns:
**	Object	EdbcDataSource object.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
*/

public Object
getObjectInstance
(
    Object	obj,
    Name	name,
    Context	context,
    Hashtable	env
)
    throws Exception
{
    Reference	    ref = (Reference)obj;
    EdbcDataSource  ds;

    if ( ! ref.getClassName().equals( "ca.edbc.jdbcx.EdbcDataSource" ) )
	return( null );

    ds = new EdbcDataSource();
    initInstance( ref, ds );

    return( ds );
} // getObjectInstance


/*
** Name: initInstance
**
** Description:
**	Helper function to getObjectInstance to initialize an
**	EdbcDataSource object from a JNDI reference.
**
** Input:
**	ref	JNDI Reference.
**
** Output:
**	ds	EdbcDataSource
**
** Returns:
**	void.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
*/

protected void
initInstance( Reference ref, EdbcDataSource ds )
{
    RefAddr ra;

    if ( (ra = ref.get( "dataSourceName" )) != null )
	ds.setDataSourceName( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "description" )) != null )
	ds.setDescription( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "serverName" )) != null )
	ds.setServerName( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "portName" )) != null )
	ds.setPortName( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "databaseName" )) != null )
	ds.setDatabaseName( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "user" )) != null )
	ds.setUser( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "password" )) != null )
	ds.setPassword( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "roleName" )) != null )
	ds.setRoleName( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "groupName" )) != null )
	ds.setGroupName( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "dbmsUser" )) != null )
	ds.setDbmsUser( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "dbmsPassword" )) != null )
	ds.setDbmsPassword( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "connectionPool" )) != null )
	ds.setConnectionPool( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "autocommitMode" )) != null )
	ds.setAutocommitMode( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "selectLoops" )) != null )
	ds.setSelectLoops( (String)((StringRefAddr)ra).getContent() );

    if ( (ra = ref.get( "portNumber" )) != null )
    {
	byte	ival[] = (byte [])((BinaryRefAddr)ra).getContent();
	int	portNumber = (((int)ival[3] << 24) & 0xff000000) | 
			     (((int)ival[2] << 16) & 0x00ff0000) | 
			     (((int)ival[1] << 8)  & 0x0000ff00) | 
			     ( (int)ival[0]        & 0x000000ff);
	
	ds.setPortNumber( portNumber );
    }

    return;
} // initInstance


} // class EdbcDSFactory
