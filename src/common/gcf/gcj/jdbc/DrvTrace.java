/*
** Copyright (c) 1999, 2007 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.jdbc;

/*
** Name: DrvTrace.java
**
** Description:
**	Defines interface which extends generic tracing to
**	support additional external tracing.
**
** History:
**       6-May-99 (gordy)
**          Created.
**	 2-May-00 (gordy)
**	    Extracted general tracing to package util.
**	16-Mar-01 (gordy)
**	    Separated interface and implementation.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
*/

import	com.ingres.gcf.util.Trace;
import	com.ingres.gcf.util.TraceLog;


/*
** Name: DrvTrace
**
** Description:
**	Defines tracing methods which support both internal driver
**	tracing and additional external tracing.
**
**  Public Methods:
**
**	getTraceName	Returns tracing instance name.
**	getTraceLog	Returns associated TraceLog.
**	enabled		Is any associated tracing enabled?
**	log		Write message to all trace logs.
**
** History:
**	 2-May-00 (gordy)
**	    Created.
**	 2-May-00 (gordy)
**	    Extracted general tracing to package util.
**	16-Mar-01 (gordy)
**	    Separated interface and implementation.
**	31-Oct-02 (gordy)
**	    Replaced trace ID constant with getTraceName(), added
**	    getTraceLog() to support tracing in non-driver classes.
**	 4-May-07 (gordy)
**	    Class is not exposed outside package, restrict access.
*/

interface	// package access
DrvTrace
    extends Trace
{


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

String getTraceName();


/*
** Name: getTraceLog
**
** Description:
**	Returns the TraceLog to which tracing is written.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	TraceLog    Associated tracing log.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

TraceLog getTraceLog();


/*
** Name: enabled
**
** Description:
**	Returns true if any tracing is enabled at any level.
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

boolean enabled();


/*
** Name: log
**
** Description:
**	Writes trace output to all applicable trace logs.
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
*/

void log( String str );


} // interface DrvTrace

