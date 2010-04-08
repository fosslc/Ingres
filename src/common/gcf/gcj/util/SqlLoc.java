/*
** Copyright (c) 2006 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.util;

/*
** Name: SqlLoc.java
**
** Description:
**	Defines class which represents a LOB Locator value.
**
**  Classes:
**
**	SqlLoc		A LOB Locator value.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

import	java.sql.SQLException;

/*
** Name: SqlLoc
**
** Description:
**	Class which represents a LOB Locator value.  
**
**	This class is unable to convert a DBMS locator value into
**	any other representation.  Only the basic I/O access methods
**	are supported.  It is expected that a subclass will extend
**	this class to provide desired conversions.
**
**	The access methods do not check for the NULL condition.  
**	The super-class isNull() method should first be checked 
**	prior to calling any of the access methods.
**
**  Public Methods:
**
**	toString		String representation.
**	set			Assign a new data value.
**	get			Retrieve current data value.
**
**  Protected Data:
**
**	value			The locator value.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
**	17-Nov-08 (rajus01) SIR 121238
**	    Replaced SqlEx references with SQLException as SqlEx becomes
**	    obsolete to support JDBC 4.0 SQLException hierarchy.
*/

public class
SqlLoc
    extends SqlData
    implements DbmsConst
{

    protected int		value = 0;


/*
** Name: SqlLoc
**
** Description:
**	Class constructor.  Data value is initially NULL.
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
**	15-Nov-06 (gordy)
**	    Created.
*/

protected
SqlLoc()
    throws SQLException
{
    super( true );
} // SqlLoc


/*
** Name: toString
**
** Description:
**      Returns a string representing the current object.
**
**      Overrides super-class method which attempts to
**	utilize getString() which isn't support for this
**	class.
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      String  String representation of this SQL data value.
**
** History:
**	15-Nov-06 (gordy)
**          Created.
*/

public String
toString()
{
    return( Integer.toString( value ) );
} // toString


/*
** Name: set
**
** Description:
**	Assign a new non-NULL data value.
**
** Input:
**	value		The new data value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public void
set( int value )
{
    setNotNull();
    this.value = value;
    return;
} // set


/*
** Name: set
**
** Description:
**	Assign a new data value as a copy of an existing 
**	SQL data object.  If the input is NULL, a NULL 
**	data value results.
**
** Input:
**	data	The SQL data to be copied.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

protected void
set( SqlLoc data )
{
    if ( data == null  ||  data.isNull() )
	setNull();
    else
    {
	setNotNull();
	value = data.value;
    }
    return;
} // set


/*
** Name: get
**
** Description:
**	Return the current data value.
**
**	Note: the value returned when the data value is 
**	NULL is not well defined.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Current value.
**
** History:
**	15-Nov-06 (gordy)
**	    Created.
*/

public int 
get() 
{
    return( value );
} // get


} // class SqlLoc
