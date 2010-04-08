/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.util;

/*
** Name: EdbcEx.java
**
** Description:
**	Defines the EDBC exception class and an error information 
**	class used to define the errors associated with exceptions.
**
**  Classes:
**
**	EdbcEx
**	ErrInfo
**
** History:
**	10-Sep-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Made ErrInfo a nested class.
**	20-Apr-00 (gordy)
**	    Moved to package util.  Converted from interface to class.
**	31-Oct-00 (gordy)
**	    Added getWarning().
**	31-Jan-01 (gordy)
**	    Added getWarning for translating SQLException to warning.
**	12-Apr-01 (gordy)
**	    Tracing changes require tracing to be passed as parameter.
*/

import java.sql.SQLException;
import java.sql.SQLWarning;


/*
** Name: EdbcEx
**
** Description:
**	EDBC exception class.
**
**  Public Methods:
**
**	trace		Write exception info to trace log.
**	get		Generate exception from ErrInfo.
**	getWarning	Generate warning from ErrInfo.
**
** History:
**	10-Sep-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Made ErrInfo a nested class.
**	20-Apr-00 (gordy)
**	    Converted from interface to class, added constructors,
**	    and implemented factory methods get().
**	31-Oct-00 (gordy)
**	    Added getWarning().
**	31-Jan-01 (gordy)
**	    Added getWarning for translating SQLException to warning.
*/

public class
EdbcEx
    extends SQLException
{

/*
** Name: EdbcEx
**
** Description:
**	Class constructor.  SQL State is defaulted to NULL,
**	error code is defaulted to 0.
**
** Input:
**	msg	    Detailed message for exception.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	20-Apr-00 (gordy)
**	    Created.
*/

public
EdbcEx( String msg )
{
    super( msg );
} // EdbcEx


/*
** Name: EdbcEx
**
** Description:
**	Class constructor.
**
** Input:
**	msg	Detailed message for exception.
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
**	20-Apr-00 (gordy)
**	    Created.
*/

public
EdbcEx( String msg, String sqlState, int err )
{
    super( msg, sqlState, err );
} // EdbcEx


/*
** Name: trace
**
** Description:
**	Writes the exception information to the trace log.
**	Any chained exceptions are also traced.
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
*/

public void
trace( Trace trace )
{
    for( EdbcEx ex = this; ex != null; ex = (EdbcEx)ex.getNextException() )
    {
	trace.write( "Exception: " + ex.getSQLState() + ", 0x" + 
		     Integer.toHexString( ex.getErrorCode() ) );
	trace.write( "  Message: " + ex.getMessage() );
    }
    return;
}


/*
** Name: get
**
** Description:
**	Generate an EdbcEx exception from an ErrInfo constant.
**
** Input:
**	err	    ErrInfo constant.
**
** Output:
**	None.
**
** Returns:
**	EdbcEx	    The new exception object.
**
** History:
**	20-Apr-00 (gordy)
**	    Created.
*/

public static EdbcEx
get( ErrInfo err )
{
    String msg;
    
    try { msg = EdbcRsrc.getResource().getString( err.id ); }
    catch( Exception ignore ){ msg = err.name; }

    return( new EdbcEx( msg, err.sqlState, err.code ) );
} // get


/*
** Name: get
**
** Description:
**	Generate an EdbcEx exception from an ErrInfo constant
**	and another EdbcEx exception.  The existing EdbcEx
**	exception is chained to the EdbcEx exception created
**	from the error information.
**
** Input:
**	err	    ErrInfo constant.
**	ex	    Associated exception,
**
** Output:
**	None.
**
** Returns:
**	EdbcEx	    The new exception object.
**
** History:
**	20-Apr-00 (gordy)
**	    Created.
*/

public static EdbcEx
get( ErrInfo err, EdbcEx ex )
{
    EdbcEx ex_list = get( err );
    ex_list.setNextException( ex );
    return( ex_list );
} // get


/*
** Name: get
**
** Description:
**	Generate an EdbcEx exception from an ErrInfo constant
**	and a general exception.  The general exception is
**	converted into an EdbcEx exception and chained to the
**	EdbcEx exception created from the error information.
**
** Input:
**	err	    ErrInfo constant
**	ex	    Associated exception.
**
** Ouptut:
**	None.
**
** Returns:
**	EdbcEx	    The new exception object.
**
** History:
**	20-Apr-00 (gordy)
**	    Created.
*/

public static EdbcEx
get( ErrInfo err, Exception ex )
{
    EdbcEx  edbc;
    String  msg = ex.getMessage();
    
    if ( msg == null )  msg = ex.toString();
    edbc = (msg == null) ? get( err ) : get( err, new EdbcEx( msg ) );
    
    return( edbc );
} // get


/*
** Name: getWarning
**
** Description:
**	Generate an SQLWarning from an ErrInfo constant.
**
** Input:
**	err	    ErrInfo constant.
**
** Output:
**	None.
**
** Returns:
**	SQLWarning  A new warning object
**
** History:
**	31-Oct-00 (gordy)
**	    Created.
*/

public static SQLWarning
getWarning( ErrInfo err )
{
    String msg;
    
    try { msg = EdbcRsrc.getResource().getString( err.id ); }
    catch( Exception ignore ){ msg = err.name; }

    return( new SQLWarning( msg, err.sqlState, err.code ) );
}


/*
** Name: getWarning
**
** Description:
**	Generate an SQLWarning from an SQLException.
**
** Input:
**	ex	    Exception.
**
** Output:
**	None.
**
** Returns:
**	SQLWarning  A new warning object
**
** History:
**	31-Jan-01 (gordy)
**	    Created.
*/

public static SQLWarning
getWarning( SQLException ex )
{
    return( new SQLWarning( ex.getMessage(),
			    ex.getSQLState(), ex.getErrorCode() ) );
}


/*
** Name: ErrInfo
**
** Description:
**	Class containing information for a single EDBC error.
**
**  Public Data
**
**	code	    Numeric error code.
**	sqlState    Associated SQL State.
**	id	    Resource key.
**	name	    Full name.
**
** History:
**	10-Sep-99 (gordy)
**	    Created.
*/

public static class
ErrInfo
{
    public  int	    code;
    public  String  sqlState;
    public  String  id;
    public  String  name;

/*
** Name: ErrInfo
**
** Description:
**	Class constructor.
**
** Input:
**	code	    Error code.
**	sqlState    SQL State.
**	id	    Resource key.
**	name	    Error name.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	10-Sep-99 (gordy)
**	    Created.
*/

public
ErrInfo( int code, String sqlState, String id, String name )
{
    this.code = code;
    this.sqlState = sqlState;
    this.id = id;
    this.name = name;
} // ErrInfo

} // class ErrInfo

} // class EdbcEx
