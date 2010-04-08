/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.jdbcx;

/*
** Name: TraceDS.java
**
** Description:
**	Defines class which implements the EdbcTrace interface for
**	both internal driver tracing and JDBCX DataSource tracing. 
**
** History:
**      21-Mar-01 (gordy)
**          Created.
*/

import	java.io.PrintWriter;
import	ca.edbc.jdbc.TraceDV;


/*
** Name: TraceDS
**
** Description:
**	Implements EdbcTrace interface to support both internal 
**	driver tracing and JDBCX DataSource tracing.
**
**  Interface Methods:
**
**	enabled		Is tracing enabled?
**	log		Log message to trace log.
**
**  Public Methods:
**
**	getWriter	Retrieve the DS tracing log.
**	setWriter	Set the DS tracing log.
**
**  Private Data:
**
**	out		DS Tracing log.
**
** History:
**	21-Mar-01 (gordy)
**	    Created.
*/

class
TraceDS
    extends TraceDV
{

    private PrintWriter		out = null;	// DS Tracing log


/*
** Name: TraceDS
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
**	21-Mar-01 (gordy)
**	    Created.
*/

public
TraceDS( String id )
{
    super( id );
} // TraceDS


/*
** Name: enabled
**
** Description:
**	Returns true if DataSource tracing is active or internal 
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
**	21-Mar-01 (gordy)
**	    Created.
*/

public boolean
enabled()
{
    return( out != null  ||  super.enabled() );
} // enabled


/*
** Name: log
**
** Description:
**	Writes trace output to both the JDBCX DataSource log
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
**	21-Mar-01 (gordy)
**	    Created.
*/

public void
log( String str )
{
    if ( out != null )  out.println( str );
    super.log( str );
    return;
} // log


/*
** Name: getWriter
**
** Description:
**	Returns the DS tracing log.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	PrintWriter	Tracing log.
**
** History:
**	21-Mar-01 (gordy)
**	    Created.
*/

public PrintWriter
getWriter()
{
    return( out );
} // getWriter


/*
** Name: setWriter
**
** Description:
**	Set the DS tracing log.
**
** Input:
**	out	Tracing log.
**
** Output:
**	None.
**
** Returns:
**	void..
**
** History:
**	21-Mar-01 (gordy)
**	    Created.
*/

public void
setWriter( PrintWriter out )
{
    this.out = out;
    return;
} // setWriter


} // class TraceDS
