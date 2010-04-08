/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.jdbc;

/*
** Name: EdbcTrace.java
**
** Description:
**	Defines interface which extends internal driver tracing to
**	support additional external tracing.
**
** History:
**       6-May-99 (gordy)
**          Created.
**	 2-May-00 (gordy)
**	    Extracted general tracing to package util.
**	16-Mar-01 (gordy)
**	    Separated interface and implementation.
*/

import	ca.edbc.util.Trace;


/*
** Name: EdbcTrace
**
** Description:
**	Defines tracing methods which support both internal driver
**	tracing and additional external tracing.
**
**  Constants:
**
**	EdbcTraceID	Trace ID for EDBC Driver
**
**  Public Methods:
**
**	enabled		Is any associated tracing enabled?
**	log		Write message to trace logs.
**
** History:
**	 2-May-00 (gordy)
**	    Created.
**	 2-May-00 (gordy)
**	    Extracted general tracing to package util.
**	16-Mar-01 (gordy)
**	    Separated interface and implementation.
*/

public interface
EdbcTrace
    extends Trace
{

    public static final String		edbcTraceID = "EDBC";


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


} // interface EdbcTrace

