/*
** Copyright (c) 1999, 2002 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: SqlWarn.java
**
** Description:
**	Defines the SQL warning class.
**
**  Classes:
**
**	SqlWarn		Extends java.sql.SQLWarning
**
** History:
**	10-Sep-99 (gordy)
**	    Created.
**	31-Oct-00 (gordy)
**	    Added getWarning().
**	31-Jan-01 (gordy)
**	    Added getWarning for translating SQLException to warning.
**	12-Apr-01 (gordy)
**	    Tracing changes require tracing to be passed as parameter.
**	11-Sep-02 (gordy)
**	    Extracted from SqlEx.
**    17-Nov-08 (rajus01) SIR 121238
**	    Replaced SqlEx references with SQLException as SqlEx becomes
**	    obsolete to support JDBC 4.0 SQLException hierarchy.
*/

import	java.sql.SQLWarning;
import	java.sql.SQLException;


/*
** Name: SqlWarn
**
** Description:
**	Class which extends standard SQL warning class to provide
**	construction from resource references and other useful sources.
**
**  Public Methods:
**
**	get		Factory methods for generating SqlWarn exceptions.
**	trace		Write exception info to trace log.
**
** History:
**	10-Sep-99 (gordy)
**	    Created.
**	31-Oct-00 (gordy)
**	    Added getWarning().
**	31-Jan-01 (gordy)
**	    Added getWarning for translating SQLException to warning.
**	11-Sep-02 (gordy)
**	    Extracted from SqlEx.
*/

public class
SqlWarn
    extends SQLWarning
{


/*
** Name: get
**
** Description:
**	Generate a SqlWarn warning from an ErrInfo constant.
**
** Input:
**	err	    ErrInfo constant.
**
** Output:
**	None.
**
** Returns:
**	SqlWarn	    A new warning object
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
**	11-Sep-02 (gordy)
**	    Extracted from SqlEx.
*/

public static SqlWarn
get( SqlExFactory.ErrInfo err )
{
    String msg;
    
    try { msg = ErrRsrc.getResource().getString( err.id ); }
    catch( Exception ignore ){ msg = err.name; }

    return( new SqlWarn( msg, err.sqlState, err.code ) );
}


/*
** Name: get
**
** Description:
**	Generate an SqlWarn from an SqlEx.
**
** Input:
**	ex	    SQL Exception.
**
** Output:
**	None.
**
** Returns:
**	SqlWarn	    A new warning object
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
**	11-Sep-02 (gordy)
**	    Extracted from SqlEx.
*/

public static SqlWarn
get( SQLException ex )
{
    return(new SqlWarn(ex.getMessage(), ex.getSQLState(), ex.getErrorCode()));
}


/*
** Name: SqlWarn
**
** Description:
**	Class constructor.
**
** Input:
**	msg	    Detailed message for warning.
**	sqlState    Associated SQL State.
**	err	    Associated error code.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	11-Sep-02 (gordy)
**	    Extracted from SqlEx.
*/

public
SqlWarn( String msg, String sqlState, int err )
{
    super( msg, sqlState, err );
    return;
} // SqlWarn


/*
** Name: trace
**
** Description:
**	Writes the warning information to the trace log.
**	Any chained warnings are also traced.
**
** Input:
**	trace	Tracing output.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	20-Apr-00 (gordy)
**	    Created.
**	12-Apr-01 (gordy)
**	    Tracing changes require tracing to be passed as parameter.
**	11-Sep-02 (gordy)
**	    Extracted from SqlEx.
*/

public void
trace( Trace trace )
{
    for( 
         SqlWarn warn = this; 
         warn != null; 
	 warn = (SqlWarn)warn.getNextWarning()
       )
    {
	trace.write( "Warning: " + warn.getSQLState() + ", 0x" + 
		     Integer.toHexString( warn.getErrorCode() ) );
	trace.write( "  Message: " + warn.getMessage() );
    }
    return;
}


} // class SqlWarn
