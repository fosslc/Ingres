/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.jdbc;

/*
** Name: TraceDM.java
**
** Description:
**	Defines class which implements the EdbcTrace interface for
**	both internal driver tracing and JDBC DriverManager tracing. 
**
** History:
**       6-May-99 (gordy)
**          Created.
**	 2-May-00 (gordy)
**	    Extracted general tracing to package util.
**	15-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	16-Mar-01 (gordy)
**	    Separated EdbcEx into interface and implementation.
*/

import	java.sql.DriverManager;


/*
** Name: TraceDM
**
** Description:
**	Implements EdbcTrace interface to support both internal 
**	driver tracing and JDBC DriverManager tracing.
**
**  Interface Methods:
**
**	enabled		Is tracing enabled?
**	log		Log message to trace log.
**
**  Private Data:
**
**	dm_20_interface	Is the JDBC 2.0 DriverManager interface available.
**
**  Private Methods:
**
**	dm_enabled	Is DriverManager logging enabled.
**
** History:
**	 2-May-00 (gordy)
**	    Created.
**	 2-May-00 (gordy)
**	    Extracted general tracing to package util.
**	15-Nov-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added static initializer, 
**	    dm_20_interface, and dm_enabled() to handle the new 
**	    DriverManager logging interface and deprecated 1.2 
**	    interface.
**	16-Mar-01 (gordy)
**	    Separate interface and implementation.
*/

class
TraceDM
    extends TraceDV
{

    private static boolean	dm_20_interface = false;    // DM interface.


/*
** Name: <class initializer>
**
** Description:
**	Determine if the JDBC 2.0 DriverManager logging interface
**	is available since the 1.2 interface has been deprecated.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

static // Class Initializer
{
    try
    {
	DriverManager.getLogWriter();
	dm_20_interface = true;
    }
    catch( Throwable ignore ) {}
} // static


/*
** Name: TraceDM
**
** Description:
**	Class constructor.  Set tracing level based on tracing ID.
**
** Input:
**	id	Tracing ID.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 2-May-00 (gordy)
**	    Created.
*/

public
TraceDM( String id )
{
    super( id );
} // TraceDM


/*
** Name: enabled
**
** Description:
**	Returns True if DriverManager tracing is active or internal 
**	driver tracing is enabled (at any level)
**	.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if tracing enabled.
**
** History:
**	 2-May-00 (gordy)
**	    Created.
*/

public boolean
enabled()
{
    return( super.enabled()  ||  dm_enabled() );
} // enabled


/*
** Name: log
**
** Description:
**	Writes trace output to both the JDBC DriverManager log
**	(if active) and internal driver trace log (if enabled 
**	at any level).
**
** Input:
**	str	Trace message.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 2-May-00 (gordy)
**	    Created.
**	15-Nov-00 (gordy)
**	    Call driver manager unconditionally to avoid problems
**	    with deprecated interface.
*/

public void
log( String str )
{
    DriverManager.println( str );
    super.log( str );
    return;
} // log


/*
** Name: dm_enabled
**
** Description:
**	Determine if DriverManager logging is enabled.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    TRUE if enabled, FALSE otherwise.
**
** History:
**	15-Nov-00 (gordy)
**	    Created.
*/

private boolean
dm_enabled()
{
    boolean enabled = false;

    try
    {
	if ( dm_20_interface )
	    enabled = (DriverManager.getLogWriter() != null);
	else
	    enabled = (DriverManager.getLogStream() != null);
    }
    catch( Exception ignore ) {}

    return( enabled );
}


} // class TraceDM
