/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: RsltProc.java
**
** Description:
**	Implements the EDBC JDBC ResultSet class RsltProc
**	for handling procedure output parameter result-sets.
**
**  Classes
**
**	RsltProc	Procedure output parameter result-set.
**
** History:
**	17-May-00 (gordy)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	27-Oct-00 (gordy)
**	    New super class which doesn't enforce connection
**	    locking or cursor oriented operations.
**	 3-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
**	28-Mar-01 (gordy)
**	    Combined functionality of close() into shut(), which is
**	    replaced by closeCursor().
**      29-Aug-01 (loera01) SIR 105641
**          Added capability to return ResultSet objects for row-returning
**          DB procedures.  RsltSlct is now extended instead of RsltFtch.
**          Deleted closeCursor and procResult methods.  Added a new constructor
**          for returning multiple-row result sets.  Added setProcResult
**          method.
*/

import	java.sql.SQLException;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;


/*
** Name: RsltProc
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC ResultSet interface for DBMS result data which
**	is obtained from procedure output parameters.
**
**	Most of the functionality for this class is inherited
**	from the super-classes, especially the direct super-
**	class RsltSlct.  
**
**      If the application specifies output parameters, the 
**      procedure returns a single data row containing the output 
**      parameter values.  This row is retrieved and cached by the 
**      class constructor and the JDBC server statement representing 
**      the procedure is closed.  The result-set preFetch and 
**      max_row_cnt values are carefully chosen such that only 
**      one row will be returned, but a check will be made for 
**      additional rows and the end-of-data indication will be noted.
**
**	The only other significant extensions for a procedure 
**	row result are connection locking and the procedure 
**	return value.  For procedures, the connection is locked 
**	from the start of the query until all parameter have 
**	been received and the JDBC server statement closed.
**
**      If the application specifies a result set instead of
**      output parameters, zero or more rows may be returned. 
**      In this case, RsltProc does not manage the transaction or
**      row retrieval logic, relying instead on the application
**      to use the appropriate methods for result sets.
**
**	The EdbcObj super class is responsible for obtaining the
**      procedure return value, if present.  RsltProc overrides
**      the setProcResult method in EdbcObj so that the return
**      value obtained in the RsltProc instance may be passed back
**      to the EdbcCall instance.
**
**
**  Overriden Methods:
**
**	setProcResult	Procedure return value.
**
** History:
**	 3-Aug-00 (gordy)
**	    Created.
**	27-Oct-00 (gordy)
**	    New super class which doesn't enforce connection
**	    locking or cursor oriented operations.
**	28-Mar-01 (gordy)
**	    Renamed shut() to closeCursor().
**      29-Aug-01 (loera01) SIR 105641
**          Extend RsltSlct instead of RsltFtch.
*/

class
RsltProc
    extends RsltSlct
{

/*
** Name: RsltProc
**
** Description:
**	Class constructor for out parameters.
**
** Input:
**	stmt		Associated statement.
**	rsmd		Result set Meta-Data.
**	stmt_id		Statement ID.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 3-Aug-00 (gordy)
**	    Created.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	 3-Nov-00 (gordy)
**	    Changed parameters for JDBC 2.0 extensions.
**	23-Jan-01 (gordy)
**	    Changed parameter type to EdbcStmt for backward compatibility.
*/

public
RsltProc
( 
    EdbcStmt	stmt, 
    EdbcRSMD	rsmd, 
    long	stmt_id
)
    throws EdbcEx
{
    /*
    ** Only one row is expected, but by setting the pre-fetch
    ** limit to two, the JDBC server is able to detect the end
    ** of the tuple stream.
    */
    super( stmt, rsmd, stmt_id, 2 );
    tr_id = "Proc[" + inst_id + "]";

    try
    {
        /*
        ** Load the single expected row.
        */
        if ( ! next()  ||  cur_row_cnt > 1 )
	    throw EdbcEx.get( E_JD0002_PROTOCOL_ERR );
    }
    catch( SQLException ex )
    {     
        throw (EdbcEx)ex;
    }
    finally
    {
        /*
        ** We should have received the end-of-data flag while loading
        ** the data and shut the server side statement.  Just in case
        ** something unexpected happened, make sure the statement is
        ** closed.  This will most likely be a NO-OP.
        */
        try{ closeCursor(); } catch( EdbcEx ignore ) {}
    }
} // RsltProc

/*
** Name: RsltProc
**
** Description:
**	Class constructor for result sets.
**
** Input:
**	stmt		Associated statement.
**	rsmd		Result set Meta-Data.
**	stmt_id		Statement ID.
**      preFetch        Pre-fetch size.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	28-Aug-01 (loera01) SIR 105641
**	    Created.
*/

public
RsltProc
( 
    EdbcStmt	stmt, 
    EdbcRSMD	rsmd, 
    long	stmt_id,
    int         preFetch
)
    throws EdbcEx
{
    /*
    ** For result set rows, RsltProc uses any pre-fetch limit above zero.
    */
    super( stmt, rsmd, stmt_id, ( preFetch < 1 ) ? 1 : preFetch );
    tr_id = "Proc[" + inst_id + "]";

} // RsltProc

/*
** Name: setProcResult
**
** Description:
**	Set the procRetVal class variable.  Overrides setProcResult in 
**      EdbcObj, which sets resultProc to true.
**
** Input:
**	retVal		Procedure return value.
**
** Output:
**	None.
**
** Returns:
**	Void.
**
** History:
**      29-Aug-01 (loera01) SIR 105641
**	    Created.
*/

protected void
setProcResult( int retVal )
    throws EdbcEx
{
   if ( trace.enabled( 3 ) )  trace.write( tr_id + ": returning " + retVal);

   stmt.setProcResult( retVal );
}
} // class RsltProc
