/*
** Copyright (c) 2003, 2007 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcSP.java
**
** Description:
**	Defines classes which implements the JDBC Savepoint interface.
**
**  Classes:
**
**	JdbcSP	    Abstract base class
**	SpName	    Named savepoints
**	SpId	    Unnamed savepoints
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**      05-Jan-09 (rajus01) SIR 121238
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
*/

import	java.sql.Savepoint;
import	java.sql.SQLException;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: JdbcSP
**
** Description:
**	JDBC driver base class which implements the JDBC Savepoint 
**	interface.  Factory methods provide access to internal 
**	implementations of named and unnamed savepoints.
**
**	Ingres savepoints are always named.  Unnamed savepoints 
**	generate a numeric ID whose string representation is used
**	for the internal name.  Named savepoints use the external 
**	name as the internal name.  
**
**	The JDBC Savepoint interface defines the access to the 
**	external name and numerid ID.  This implementation defines
**	access to the internal name and allows chaining of savepoints.
**
**  Interface Methods:
**
**	getSavepointName	Returns named savepoint name.
**	getSavepointId		Returns unnamed savepoint ID.
**
**  Public Methods:
**
**	getNamedSP		Factory method for named savepoints.
**	getUnnamedSP		Factory method for unnamed savepoints.
**	getName			Returns internal savepoint name.
**	getNext			Returns next chained savepoint.
**	setNext			Set next chained savepoint.
**
**
**  Private Data:
**
**	next			Chained savepoints.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
**	    Implement default interface methods.
*/

public abstract class
JdbcSP
    implements Savepoint, GcfErr
{

    private JdbcSP	next = null;	// Chained savepoints


/*
** Abstract methods.
*/
abstract String	getName();	// package access


/*
** Name: getNamedSP
**
** Description:
**	Factory method for named savepoints.
**
** Input:
**	name	Savepoint name.
**
** Output:
**	None.
**
** Returns:
**	JdbcSP	A named savepoint.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

static JdbcSP
getNamedSP( String name )
    throws SQLException
{
    return( new SpName( name ) );
} // getNamedSP


/*
** Name: getUnnamedSP
**
** Description:
**	Factory method for unnamed savepoints.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcSP	An unnamed savepoint.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

static JdbcSP
getUnnamedSP()
{
    return( new SpId() );
} // getUnnamedSP


/*
** Name: JdbcSP
**
** Description:
**	Class constructor.  Don't permit instantiation.
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
**	 4-May-07 (gordy)
**	    Created.
*/

private
JdbcSP()
{
} // JdbcSP


/*
** Name: toString
**
** Description:
**	Returns internal savepoint name.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Internal savepoint name.
**
** History:
**	14-Feb-03 (gordy)
*/

public String
toString()
{
    return( "JdbcSP." + getName() );
} // toString


/*
** Name: getSavepointID
**
** Description:
**	Returns the numeric savepoint ID.  Named savepoints do not 
**	have a numeric ID.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Savepoint ID.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

public int
getSavepointId()
    throws SQLException
{
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getSavepointId


/*
** Name: getSavepointName
**
** Description:
**	Returns the external savepoint name.  Unnamed savepoints
**	do not have an external name.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	External savepoint name.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

public String
getSavepointName()
    throws SQLException
{
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getSavepointName()


/*
** Name: getNext
**
** Description:
**	Returns next chained savepoint, NULL at end of chain.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcSP	Next savepoint, possibly NULL.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

JdbcSP	// package access
getNext()
{
    return( next );
} // getNext


/*
** Name: setNext
**
** Description:
**	Set the next savepoint in chain.  Returns previously chained
**	savepoint.
**
** Input:
**	next	Next savepoint for chain.
**
** Output:
**	None.
**
** Returns:
**	JdbcSP	Next savepoint, possibly NULL.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

JdbcSP	// package access
setNext( JdbcSP next )
{
    JdbcSP prev = this.next;
    this.next = next;
    return( prev );
} // setNext


/*
** Name: SpName
**
** Description:
**	Class which implements the JDBC Savepoint interface for
**	named savepoints.
**
**	Uses the assigned savepoint name as the internal savepoint
**	name so the name must be a valid identifier.  Unnamed
**	savepoints use numeric identifiers, so name can't start
**	with a digit.
**
**  Interface Methods:
**
**	getSavepointName	Returns named savepoint name.
**
**  Protected Methods:
**
**	getName			Returns internal savepoint name.
**
**  Private Data:
**
**	name			Savepoint name.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
**	 4-May-07 (gordy)
**	    Use parent default for non-implemented interface method.
*/

private static class
SpName
    extends JdbcSP
{

    private String	name = null;	// Savepoint name


/*
** Name: SpName
**
** Description:
**	Class constructor.
**
** Input:
**	name	Savepoint name.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

// package access
SpName( String name )
    throws SQLException
{
    if ( name == null  ||  name.length() == 0  ||  
	 Character.isDigit( name.charAt(0) ) )
        throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );

    this.name = name;
    return;
} // SpName


/*
** Name: getSavepointName
**
** Description:
**	Returns the savepoint name.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	External savepoint name.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

public String
getSavepointName()
    throws SQLException
{
    return( name );
} // getSavepointName()


/*
** Name: getName
**
** Description:
**	Returns the internal savepoint name which is simply the
**	savepoint name.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Internal savepoint name.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
**	 4-May-07 (gordy)
**	    Restrict access to non-interface method.
*/

String	// package access
getName()
{
    return( name );
} // getName()


} // class SpName


/*
** Name: SpId
**
** Description:
**	Class which implements the JDBC Savepoint interface for
**	unnamed savepoints.
**
**	Assigns a unique numeric ID to each savepoint.  The string
**	representation of the numeric ID is used for the internal
**	savepoint name.
**
**  Interface Methods:
**
**	getSavepointId		Returns unnamed savepoint ID.
**
**  Protected Methods:
**
**	getName			Returns internal savepoint name.
**
**  Private Data:
**
**	count			Instance counter.
**	id			Instance ID.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
**	 4-May-07 (gordy)
**	    Use parent default for non-implemented interface method.
*/

private static class
SpId
    extends JdbcSP
{

    private static int		count = 0;	// Instance counter
    private int			id = 0;		// Instance ID


/*
** Name: SpId
**
** Description:
**	Class constructor.  Assign numeric ID.
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
**	14-Feb-03 (gordy)
**	    Created.
*/

// package access
SpId()
{
    id = ++count;
    return;
}


/*
** Name: getSavepointID
**
** Description:
**	Returns the numeric savepoint ID.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Savepoint ID.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

public int
getSavepointId()
    throws SQLException
{
    return( id );
} // getSavepointId


/*
** Name: getName
**
** Description:
**	Returns the internal savepoint name which is the numeric
**	ID converted to string format.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Internal savepoint name.
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
**	 4-May-07 (gordy)
**	    Restrict access to non-interface method.
*/

String	// package access
getName()
{
    return( Integer.toString( id ) );
} // getName()


} // class SpId

} // class JdbcSP


