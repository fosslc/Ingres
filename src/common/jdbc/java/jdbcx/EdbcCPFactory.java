/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcCPFactory.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcCPFactory.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/


import	java.util.Hashtable;
import	javax.naming.Context;
import	javax.naming.Name;
import	javax.naming.Reference;


/*
** Name: EdbcCPFactory
**
** Description:
**	EDBC JDBC extension class which acts as an object factory 
**	for the EdbcCPDataSource class.
**
**  Interface Methods:
**
**	getObjectInstance   Returns EdbcCPDataSource generated from Reference.
**
** History:
**	29-Jan-01 (gordy)
**	    Created.
*/

public class
EdbcCPFactory
    extends EdbcDSFactory
{


/*
** Name: getObjectInstance
**
** Description:
**	Generates an EdbcCPDataSource object from a JNDI Reference.  
**
** Input:
**	obj	JNDI Reference to an EdbcCPDataSource object.
**	name	Not used.
**	context	Not used.
**	env	Not used.
**
** Output:
**	None.
**
** Returns:
**	Object	EdbcCPDataSource object.
**
** History:
**	29-Jan-01 (gordy)
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
    Reference		ref = (Reference)obj;
    EdbcCPDataSource	ds;

    if ( ! ref.getClassName().equals( "ca.edbc.jdbcx.EdbcCPDataSource" ) )
	return( null );

    ds = new EdbcCPDataSource();
    initInstance( ref, ds );

    return( ds );
} // getObjectInstance


} // class EdbcCPFactory
