/*
** Copyright (c) 2001, 2003 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.jdbc;

/*
** Name: IngresXAFactory.java
**
** Description:
**	Defines class factory for the IngresXADataSource class.
**
**  Classes
**
**	IngresXAFactory
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for Ingres data-source factory.
**	15-Jul-03 (gordy)
**	    Get class name directly from class rather than constant.
*/


import	java.util.Hashtable;
import	javax.naming.Context;
import	javax.naming.Name;
import	javax.naming.Reference;
import	javax.naming.spi.ObjectFactory;


/*
** Name: IngresXAFactory
**
** Description:
**	Class factory for the IngresXADataSource class.
**
**  Interface Methods:
**
**	getObjectInstance   Generate IngresXADataSource from Reference.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for Ingres data-source factory.
*/

public class
IngresXAFactory
    implements ObjectFactory, IngConst
{


/*
** Name: getObjectInstance
**
** Description:
**	Generates an IngresXADataSource object from a JNDI Reference.  
**
** Input:
**	obj	JNDI Reference to an IngresXADataSource object.
**	name	Not used.
**	context	Not used.
**	env	Not used.
**
** Output:
**	None.
**
** Returns:
**	Object	IngresXADataSource object.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for Ingres data-source factory.
**	15-Jul-03 (gordy)
**	    Get class name directly from class rather than constant.
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
    Reference	ref = (Reference)obj;

    /*
    ** Make sure Reference is for the proper class.
    */
    if ( ! ref.getClassName().equals( IngresXADataSource.class.getName() ) )
	return( null );

    /*
    ** Create DataSource and allow it to initialize
    ** itself from the Reference.
    */
    return( new IngresXADataSource( ref ) );
} // getObjectInstance


} // class IngresXAFactory
