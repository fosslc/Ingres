/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbcx;

/*
** Name: EdbcXAFactory.java
**
** Description:
**	Implements the EDBC JDBC extension class EdbcXAFactory.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
*/


import	java.util.Hashtable;
import	javax.naming.Context;
import	javax.naming.Name;
import	javax.naming.Reference;


/*
** Name: EdbcXAFactory
**
** Description:
**	EDBC JDBC extension class which acts as an object factory 
**	for the EdbcXADataSource class.
**
**  Interface Methods:
**
**	getObjectInstance   Returns EdbcXADataSource generated from a Reference.
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
*/

public class
EdbcXAFactory
    extends EdbcDSFactory
{


/*
** Name: getObjectInstance
**
** Description:
**	Generates an EdbcXADataSource object from a JNDI Reference.  
**
** Input:
**	obj	JNDI Reference to an EdbcXADataSource object.
**	name	Not used.
**	context	Not used.
**	env	Not used.
**
** Output:
**	None.
**
** Returns:
**	Object	EdbcXADataSource object.
**
** History:
**	31-Jan-01 (gordy)
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
    EdbcXADataSource	ds;

    if ( ! ref.getClassName().equals( "ca.edbc.jdbcx.EdbcXADataSource" ) )
	return( null );

    ds = new EdbcXADataSource();
    initInstance( ref, ds );

    return( ds );
} // getObjectInstance


} // class EdbcXAFactory
