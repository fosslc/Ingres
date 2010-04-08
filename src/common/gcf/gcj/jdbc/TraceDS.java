/*
** Copyright (c) 2001, 2002 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.jdbc;

/*
** Name: TraceDS.java
**
** Description:
**	Defines class which implements the DrvTrace interface for
**	both internal driver tracing and JDBC DataSource tracing. 
**
**  Classes
**	TraceDS
**
** History:
**      21-Mar-01 (gordy)
**          Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

import	java.io.PrintWriter;
import	com.ingres.gcf.util.Tracing;
import	com.ingres.gcf.util.TraceLog;


/*
** Name: TraceDS
**
** Description:
**	Implements DrvTrace interface to support both internal 
**	driver tracing and JDBC DataSource tracing.
**
**  Interface Methods:
**
**	getTraceName	Returns tracing instance name.
**	enabled		Is tracing enabled?
**	log		Log message to trace log.
**
**  Public Methods:
**
**	getWriter	Returns the DataSource tracing output.
**	setWriter	Set the DataSource tracing output.
**
**  Private Data:
**
**	name		Tracing instance name.
**	out		DS Tracing log.
**
** History:
**	21-Mar-01 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Added name for tracing ID instead of interface constant,
**	    getTraceName() to return tracing ID.
*/

class
TraceDS
    extends Tracing
    implements DrvTrace
{

    private PrintWriter		out = null;	// DS Tracing output
    private String		name = "JDBC";	// Tracing instance name.


/*
** Name: TraceDS
**
** Description:
**	Class constructor.  Set tracing level based on tracing ID.
**
** Input:
**	log	Internal trace log.
**	name	Tracing instance name.
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
**	31-Oct-02 (gordy)
**	    Added trace log and name for configuration by driver
**	    implementation wrappers.
*/

public
TraceDS( TraceLog log, String name, String id )
{
    super( log, id );
    this.name = name;
    return;
} // TraceDS


/*
** Name: getTraceName
**
** Description:
**	Returns a string which describes this tracing instance.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String	Tracing name.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

public String 
getTraceName()
{
    return( name );
} // getTraceName


/*
** Name: enabled
**
** Description:
**	Returns True if DataSource tracing is active or internal 
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
**	31-Oct-02 (gordy)
**	    Super-class no longer implements driver multi-log methods.
*/

public boolean
enabled()
{
    return( out != null  ||  super.enabled( 1 ) );
} // enabled


/*
** Name: log
**
** Description:
**	Writes trace output to both the JDBC DataSource output
**	(if active) and internal driver trace log.
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
**	31-Oct-02 (gordy)
**	    Super-class no longer implements driver multi-log methods.
*/

public void
log( String str )
{
    if ( out != null )  out.println( str );
    write( str );
    return;
} // log


/*
** Name: getWriter
**
** Description:
**	Returns the DataSource tracing output.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	PrintWriter	Tracing output, may be NULL.
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
**	Set the DataSource tracing output.  Set to NULL to
**	disable.
**
** Input:
**	out	Tracing log, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	void.
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
