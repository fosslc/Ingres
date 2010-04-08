/*
** Copyright (c) 2005, 2009 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.util;

/*
** Name: SqlExType.java
**
** Description:
**	The enumeration type to handle SQLException hierarchy. The String 
**	value in the paranthesis represents the SQLSTATE class. The values
**	for UNKNOWN_EXCEPTION, CLIENTINFO_EXCEPTION are chosen randomly. 
**
** History:
**      11-19-08 (rajus01) SIR 121238
**          Created.
*/

import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.sql.SQLNonTransientConnectionException;
import java.sql.SQLClientInfoException;
import java.sql.SQLDataException;
import java.sql.SQLIntegrityConstraintViolationException;
import java.sql.SQLSyntaxErrorException;
import java.sql.SQLInvalidAuthorizationSpecException;
import java.sql.SQLTransactionRollbackException;
import java.sql.ClientInfoStatus;
import java.util.Hashtable;

public enum 
SqlExType 
{
    UNKNOWN_EXCEPTION ("99"),
    CLIENTINFO_EXCEPTION("98"),
    UNSUPPORTED_FEATURE ("0A"),
    DATA_ERROR  ("22"),
    INTEGRITY_CONSTRAINT_VOILATION ("23"),
    INTEGRITY_AUTH_CREDENTIAL ("28"),
    CONN_EXCEPTION ("08"),
    SYNTAX_ERROR  ("42"),
    TRANSACTION_ROLLBACK("40");

    private final String sqlstate_class;

SqlExType(String sqlstate_class) 
{
    this.sqlstate_class = sqlstate_class;
}

public String 
sqlStateClass()
{ 
    return sqlstate_class;
}

/*
** Name: getSqlEx
**
** Description:
**
** Input:
**      extype         SQLException type 
**      msg            error message 
**      sqlState       SQLState 
**      code           vendor code 
**
** Output:
**      None.
**
** Returns:
**      SqlEx       The new exception object.
**
** History:
**      05-Jan-09 (rajus01) SIR 121238
**          Created.
**	03-Feb-09 (rajus01) SIR 121238
**	    Added few more SQLException types.
*/

public static SQLException 
getSqlEx(SqlExType extype, String msg, String sqlState, int code  )
{
    if( extype == null )
	extype = SqlExType.getType(sqlState);

    switch ( extype )
    {
	case UNSUPPORTED_FEATURE:
	   return( new SQLFeatureNotSupportedException( msg, sqlState, code ) );

        case CLIENTINFO_EXCEPTION:
         return( new SQLClientInfoException(msg, sqlState, code,
                               new Hashtable<String,ClientInfoStatus>()) );
	case CONN_EXCEPTION:
	   return( new SQLNonTransientConnectionException( msg, sqlState, code ) );
	case DATA_ERROR:
           return( new SQLDataException( msg, sqlState, code ) );

	case INTEGRITY_CONSTRAINT_VOILATION:
	   return( new SQLIntegrityConstraintViolationException( msg, sqlState, code ) );
	case SYNTAX_ERROR:
	   return( new SQLSyntaxErrorException( msg, sqlState, code ) );	   
	case INTEGRITY_AUTH_CREDENTIAL:
	   return( new SQLInvalidAuthorizationSpecException( msg, sqlState, code ) ); 
	case TRANSACTION_ROLLBACK:
	   return( new SQLTransactionRollbackException( msg, sqlState, code ) );

    }

    return new SQLException( msg, sqlState, code );
}

/*
** Name: getType
**
** Description:
**    Evaluates SQLstate class from the given SQLstate and returns the
**    correct SQLException cateogory.
**
** Input:
**      sqlState	SQL state
**
** Output:
**      None.
**
** Returns:
**      SqlExType       enum type.
**
** History:
**      05-Jan-09 (rajus01)
**          Created.
**	02-Feb-09 (rajus01)
**	    getType now takes sqlState as input rather than sqlState class.
*/
public static SqlExType 
getType( String sqlState )
{
    SqlExType extype = UNKNOWN_EXCEPTION;
    String classVal = null;

    if( sqlState == null )
	classVal = UNKNOWN_EXCEPTION.sqlStateClass();
    else
    {
	try { classVal = sqlState.substring(0,2); }
	catch( IndexOutOfBoundsException ex )
        { classVal = UNKNOWN_EXCEPTION.sqlStateClass(); }
    }

    for( SqlExType ext : SqlExType.values() )
	if(classVal.equals(ext.sqlStateClass() ) )
	    extype = ext;

    return extype;
}

}
