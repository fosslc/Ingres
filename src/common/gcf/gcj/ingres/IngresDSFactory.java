/*
** Copyright (c) 2001, 2003 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.jdbc;

/*
** Name: IngresDSFactory.java
**
** Description:
**	Defines class factory for the IngresDataSource class.
**
**  Classes
**
**	IngresDSFactory
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
** Name: IngresDSFactory
**
** Description:
**	Class factory for the IngresDataSource class.
**
**  Interface Methods:
**
**	getObjectInstance   Generate IngresDataSource from Reference.
**
** History:
**	26-Jan-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for Ingres data-source factory.
*/

public class
IngresDSFactory
    implements ObjectFactory, IngConst
{


/*
** Name: getObjectInstance
**
** Description:
**	Generates an IngresDataSource object from a JNDI Reference.  
**
** Input:
**	obj	JNDI Reference to an IngresDataSource object.
**	name	Not used.
**	context	Not used.
**	env	Not used.
**
** Output:
**	None.
**
** Returns:
**	Object	IngresDataSource object.
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
    if ( ! ref.getClassName().equals( IngresDataSource.class.getName() ) )
	return( null );

    /*
    ** Create DataSource and allow it to initialize
    ** itself from the Reference.
    */
    return( new IngresDataSource( ref ) );
} // getObjectInstance


} // class IngresDSFactory
