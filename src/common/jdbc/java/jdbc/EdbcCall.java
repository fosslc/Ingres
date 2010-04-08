/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcCall.java
**
** Description:
**	Implements the EDBC JDBC CallableStatement class: EdbcCall.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	25-Aug-00 (gordy)
**	    Implemented actual support for database procedures.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	 2-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	15-Nov-00 (gordy)
**	    Parameters saved as protected ParamSet in super class.
**	 8-Jan-01 (gordy)
**	    Additional extraction of parameter info to ParamSet.
**	    Added support for batch processing.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
**      14-Mar-01 (loera01) Bug 104327
**          When addBatch encounters an OUT parameter, do not throw an
**          exception there; for some strange reason Sun wants us to
**          throw the execption in executeBatch.  We instead insert a null
**          entry into the parameter set, so that when executeBatch
**          retrieves the parameters, it assumes an OUT parameter was 
**          attempted when it fetches a null parameter set.
**	28-Mar-01 (gordy)
**	    ProcInfo now passed as parameter to parseCall()
**	    rather than returned as result value.
**	10-May-01 (gordy)
**	    Parameter flags now defined by ParamSet, rather than using
**	    DESCR message flags directly.
**      11-Jul-01 (loera01) SIR 105309
**          Added support for Ingres syntax for database procedures.
**      29-Aug-01 (loera01) SIR 105641
**          Added capability to handle row-returning database procedures.
**      15-Apr-03 (loera01) Bug 110079
**          In EdbcCall, do not include the procedure return parameter 
**          as part of the internal parameter count.
*/

import	java.math.BigDecimal;
import	java.util.LinkedList;
import	java.util.Calendar;
import	java.util.Map;
import	java.util.NoSuchElementException;
import	java.sql.CallableStatement;
import	java.sql.ResultSet;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Array;
import	java.sql.Blob;
import	java.sql.Clob;
import	java.sql.Ref;
import	java.sql.Types;
import	java.sql.SQLException;
import  java.sql.BatchUpdateException;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;


/*
** Name: EdbcCall
**
** Description:
**	Ingres JDBC Java driver class which implements the
**	JDBC CallableStatement interface.
**
**  Interface Methods:
**
**	registerOutParameter	Register SQL type of OUT parameters.
**	execute			Execute the procedure.
**	addBatch		Add parameter set to batch list.
**	executeBatch		Execute procedure for param set in batch list.
**	clearParameters		Clear parameter set.
**	wasNull			Determine if last parameter read was NULL.
**	getString		Retrieve parameter as a string value.
**	getBoolean		Retrieve parameter as a boolean value.
**	getByte			Retrieve parameter as a byte value.
**	getShort		Retrieve parameter as a short value.
**	getInt			Retrieve parameter as a int value.
**	getLong			Retrieve parameter as a long value.
**	getFloat		Retrieve parameter as a float value.
**	getDouble		Retrieve parameter as a double value.
**	getBigDecimal		Retrieve parameter as a BigDecimal value.
**	getBytes		Retrieve parameter as a byte array value.
**	getDate			Retrieve parameter as a Date value.
**	getTime			Retrieve parameter as a Time value.
**	getTimestamp		Retrieve parameter as a Timestamp value.
**	getObject		Retrieve parameter as a generic Java object.
**	getArray		Retrieve parameter as an array.
**	getBlob			Retrieve parameter as a Blob.
**	getClob			Retrieve parameter as a Clob.
**	getRef			Retrieve parameter as a Ref.
**
**  Overriden Methods:
**
**      executeUpdate		Execute an update query.
**      executeQuery		Execute a select query.
**
**  Private Methods:
**
**      exec                Execute db procedure.
**      initParameters      Initialize procedure parameters.
**      isProcRetVal        Is parameter a return a value?
**	paramMap	    Map an extern parameter index to the internal index.
**
**  Private Data:
**
**	procInfo	    Procedure and parameter information.
**	null_param	    True if last parameter retrieved was NULL.
**	dpMap		    Dynamic parameter marker map.
**	opMap		    Output parameter marker map.
**	noMap		    Empty parameter map.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented actual support for database procedures.
**	 2-Nov-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added getBigDecimal(), getDate(),
**	    getTime(), getTimestamp(), getObject(), getArray(), getBlob(),
**	    getClob(), getRef(), registerOutParameter().
**	11-Jan-01 (gordy)
**	    Added support for batch processing.  Added addBatch() and
**	    executeBatch().  Extracted exec() from execute().
**      29-Aug-01 (loera01) SIR 105641
**          Added executeUpdate and executeQuery methods that override their
**          counterparts in EdbcPrep.  Added detection of multiple-row
**          result sets.  Added local RsltProc variable and isByRefParam 
**          boolean to separate output parameters from multiple-row result 
**          sets.  All getXXX methods now reference local RsltProc variable.
*/

class
EdbcCall
    extends	EdbcPrep
    implements	CallableStatement
{

    private static final int	noMap[] = new int[0];	/* default mapping */

    private ProcInfo	procInfo;		/* Parsing information */ 
    private int		dpMap[] = noMap;	/* Dynamic parameter mapping */
    private int		opMap[] = noMap;	/* Output parameter mapping */
    private boolean	null_param = false;	/* Last value null? */
    private boolean     isNativeDBProc = false; /* Native (Ingres) DB proc? */
    private RsltProc    resultRow = null;
    private boolean     isByRefParam = false;   /* Proc has BYREF parameters? */

/*
** Name: EdbcCall
**
** Description:
**	Class Constructor.
**
** Input:
**	conn		Associated connection.
**	sql		SQL with procedure escape sequence.
**
** Output:
**	none.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
**	 4-Oct-00 (gordy)
**	    Create unique ID for standardized internal tracing.
**	 2-Nov-00 (gordy)
**	    Changed parameters for JDBC 2.0 extensions.
**	 8-Jan-01 (gordy)
**	    Default parameter flags stored in ParamSet.
**	28-Mar-01 (gordy)
**	    ProcInfo now passed as parameter to parseCall()
**	    rather than returned as result value.
**      11-Jul-01 (loera01) SIR 105309
**          Added check for Ingres DB proc syntax.
**	31-Oct-01 (gordy)
**	    Timezone now passed to EdbcSQL.
**      15-Apr-03 (loera01) Bug 110079
**          Do not include the procedure return parameter as part of the
**          internal parameter count.
*/

public
EdbcCall( EdbcConnect conn, String sql )
    throws SQLException
{
    /*
    ** Initialize base class and override defaults.
    */
    super( conn, ResultSet.TYPE_FORWARD_ONLY, ResultSet.CONCUR_READ_ONLY );
    title = shortTitle + "-CallableStatement[" + inst_id + "]";
    tr_id = "Call[" + inst_id + "]";

    /*
    ** Flag all parameters as procedure parameters.
    */
    params.setDefaultFlags( ParamSet.PS_FLG_PROC_PARAM );

    /*
    ** Process the call procedure syntax.
    */
    EdbcSQL parser = new EdbcSQL( sql, getConnDateFormat().getTimeZone() );
    procInfo = new ProcInfo( dbc, trace );
    parser.parseCall( procInfo );
    if (parser.getQueryType() == parser.QT_NATIVE_PROC)
    {
	/*
	** For Ingres syntax, no need to run loadParamNames().  Parameter names 
	** have already been set in the parseCall method.
	*/
	isNativeDBProc = true;
    }
    else if ( procInfo.getParamCount() > 0 )  
    {
        /*
        **  If the procedure has any BYREF or input parameters, get their
        **  names.
        */
	procInfo.loadParamNames();
    }
    /*
    ** Count the number of dynamic parameters and allocate
    ** the dynamic parameter mapping array.  The array also
    ** includes an entry for the procedure return value.
    ** Initialize the array and any static literal parameters.
    */
    int	dynamic, parm, parm_cnt = procInfo.getParamCount();

    for( dynamic = 0, parm = 1; parm <= parm_cnt; parm++ )
	if ( procInfo.getParamType( parm ) == procInfo.PARAM_DYNAMIC )
	    dynamic++;

    if ( procInfo.getReturn() )  dynamic++;
    dpMap = (dynamic > 0) ? new int[ dynamic ] : noMap;
    opMap = (dynamic > 0) ? new int[ dynamic ] : noMap;
    initParameters();

} // EdbcCall


/*
** Name: initParameters
**
** Description:
**	Initialize parameter flags, static parameter values,
**	and the dynamic parameter map.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 1-Aug-00 (gordy)
**	    Created.
**	15-Nov-00 (gordy)
**	    Parameters saved as protected ParamSet in super class.
**	 8-Jan-01 (gordy)
**	    setObject method moved to ParamSet.
**      11-Jul-01 (loera01) SIR 105309
**          If Ingres syntax, initialize return parameter at the end of
**          dpMap, rather than the start.
**      13-Aug-01 (loera01) SIR 105521
**          Added check for ProcInfo.PARAM_SESSION to support global temp 
**          table parameters. 
**      29-Aug-01 (loera01) SIR 105641
**          Initialize isByRefParam to false.
*/

private synchronized void
initParameters()
    throws SQLException
{
    int	    dynamic = 0;
    int	    parm_cnt = procInfo.getParamCount();
    Object  value = null;
    
    isByRefParam = false;

    /*
    ** If a procedure return value is expected, it
    ** takes the first place in the mapping array
    ** for the generic syntax, but last place if
    ** Ingres syntax.
    ** Return parameters need no mapping.
    */
    if ( procInfo.getReturn() )  
    {
	if ( isNativeDBProc )
	{
	    dpMap[ dpMap.length  - 1 ] = 0;
	    opMap[ dpMap.length - 1 ] = 0; 
	}
        else
	{
	    dpMap[ dynamic ] = 0;
	    opMap[ dynamic ] = 0; 
	    dynamic++; 
	}
    }

    /*
    ** Now initialize the parameters.  Dynamic
    ** parameters are simply mapped for later
    ** access.  Set the input value or default
    ** status for the other parameters.
    */
    for( int parm = 1; parm <= parm_cnt; parm++ )
    {
	int index = super.paramMap( parm );
	params.setName( index, procInfo.getParamName( parm ) );

	switch( procInfo.getParamType( parm ) )
	{
	case ProcInfo.PARAM_DEFAULT :	/* Unspecified parameters */
	    break;

	case ProcInfo.PARAM_DYNAMIC :	/* Dynamic parameters */
            /*       	
	    **  If native db proc (Ingres syntax), and this is the last
	    **  parameter, don't assign a value.
	    */
	    if ( (dynamic == dpMap.length) && procInfo.getReturn() &&
		isNativeDBProc )
		{
		    return;
		}
            else
	    {
	        dpMap[ dynamic ] = parm;
	        opMap[ dynamic ] = 0;
	        dynamic++;
	    }
	    break;

	case ProcInfo.PARAM_SESSION :	/* Dynamic parameters */
	    value = procInfo.getParamValue( parm );
	    params.setObject( index, value, params.getSqlType( value ), -1 );
	    params.setFlags( index, ParamSet.PS_FLG_PROC_GTT );
	    break;
	default :			/* Literal input parameters */
	    value = procInfo.getParamValue( parm );
	    params.setObject( index, value, params.getSqlType( value ), -1 );
	    break;
	}
    }

    return;
} // initParameters


/*
** Name: registerOutParameter
**
** Description:
**	Register SQL type of an OUT parameter.
**
** Input:
**	paramIndex	Parameter index.
**	sqlType		SQL type.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
**	15-Nov-00 (gordy)
**	    Parameters saved as protected ParamSet in super class.
**      29-Aug-01 (loera01) SIR 105641
**          Set isByRefParam to true.
*/

public void
registerOutParameter( int paramIndex, int sqlType )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".registerOutParameter( " + paramIndex + 
			   ", " + sqlType + " )" );
    /*
    ** We don't care about the registered datatype.  The JDBC
    ** server will tell us the output datatype and any coersion
    ** is determined by which getXXX method is called.  Also,
    ** we don't do anything for the procedure return value (if
    ** present) since it isn't a procedure parameter and output
    ** is expected.
    **
    ** Flag the parameter as an output parameter and set the
    ** output parameter map index.  An identity mapping is
    ** done for now.  Later, when all output parameters have 
    ** been registerred, the actual mapping to the result set
    ** will be done.
    */
    if ( ! isProcRetVal( paramIndex ) )
    {
	params.setFlags( paramMap( paramIndex ), ParamSet.PS_FLG_PROC_BYREF );
	opMap[ paramIndex - 1 ] = paramIndex;
	isByRefParam = true;
    }

    return;
} // registerOutParameter


/*
** Name: registerOutParameter
**
** Description:
**	Register SQL type and scale of an OUT parameter.
**
** Input:
**	paramIndex	Parameter index.
**	sqlType		SQL type.
**	scale		Number of digits following decimal point.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
**	15-Nov-00 (gordy)
**	    Parameters saved as protected ParamSet in super class.
**      29-Aug-01 (loera01) SIR 105641
**          Set isByRefParam to true.
*/

public void
registerOutParameter( int paramIndex, int sqlType, int scale )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".registerOutParameter( " + paramIndex + 
			   ", " + sqlType + ", " + scale + " )" );
    /*
    ** We don't care about the registered datatype.  The JDBC
    ** server will tell us the output datatype and any coersion
    ** is determined by which getXXX method is called.  Also,
    ** we don't do anything for the procedure return value (if
    ** present) since it isn't a procedure parameter and output
    ** is expected.
    **
    ** Flag the parameter as an output parameter and set the
    ** output parameter map index.  An identity mapping is
    ** done for now.  Later, when all output parameters have 
    ** been registerred, the actual mapping to the result set
    ** will be done.
    */
    if ( ! isProcRetVal( paramIndex ) )
    {
	params.setFlags( paramMap( paramIndex ), ParamSet.PS_FLG_PROC_BYREF );
	opMap[ paramIndex - 1 ] = paramIndex;
	isByRefParam = true;
    }

    return;
} // registerOutParameter


/*
** Name: registerOutParameter
**
** Description:
**	Register SQL type of an OUT parameter.
**
** Input:
**	paramIndex	Parameter index.
**	sqlType		SQL type.
**	typeName	Name of SQL type.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
**	15-Nov-00 (gordy)
**	    Parameters saved as protected ParamSet in super class.
**      29-Aug-01 (loera01) SIR 105641
**          Set isByRefParam to true.
*/

public void
registerOutParameter( int paramIndex, int sqlType, String typeName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".registerOutParameter( " + paramIndex + 
			   ", " + sqlType + ", " + typeName + " )" );
    /*
    ** We don't care about the registered datatype.  The JDBC
    ** server will tell us the output datatype and any coersion
    ** is determined by which getXXX method is called.  Also,
    ** we don't do anything for the procedure return value (if
    ** present) since it isn't a procedure parameter and output
    ** is expected.
    **
    ** Flag the parameter as an output parameter and set the
    ** output parameter map index.  An identity mapping is
    ** done for now.  Later, when all output parameters have 
    ** been registerred, the actual mapping to the result set
    ** will be done.
    */
    if ( ! isProcRetVal( paramIndex ) )
    {
	params.setFlags( paramMap( paramIndex ), ParamSet.PS_FLG_PROC_BYREF );
	opMap[ paramIndex - 1 ] = paramIndex;
	isByRefParam = true;
    }

    return;
} // registerOutParameter

/*
** Name: executeQuery
**
** Description:
**	Execute a DB procedure and return a result set.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSet   A new ResultSet object.
**
** History:
**      29-Aug-01 (loera01) SIR 105641
**          Created from EdbcPrep.
*/

public ResultSet
executeQuery()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".executeQuery()" );

    /*
    ** Throw an execption if output parameters were specified.
    */
    if ( isByRefParam )
        throw EdbcEx.get( E_JD0020_INVALID_OUT_PARAM );

    exec( params );

    /*
    ** Throw an execption if the DB proc doesn't return a result set.
    */
    if ( resultSet == null )
        throw EdbcEx.get( E_JD0021_NO_RESULT_SET );

    return( resultSet );
} // executeQuery


/*
** Name: executeUpdate
**
** Description:
**	Execute a db procedure.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Value of zero.	    
**
** History:
**      29-Aug-01 (loera01) SIR 105641
**          Created from EdbcPrep.
*/

public int
executeUpdate()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".executeUpdate()" );

    exec( params );
    /*
    ** Since we don't want result sets to be returned with this method, if
    ** present, then  clean up the tuple steam and thrown an exception.
    */
    if ( resultSet != null )
    {
        try { resultSet.shut(); }
        catch( SQLException ignore ) {}
        resultSet = null;
        throw EdbcEx.get( E_JD0022_RESULT_SET_NOT_PERMITTED );    
    }
    /*
    ** Ingres DB procedures don't return the number of rows updated, so
    ** always return zero.
    */
    return( 0 );
} // executeUpdate

/*
** Name: execute
**
** Description:
**	Execute an database procedure.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    False if no result set is returned; true if result
**                  set is present.
**
** History:
**	15-Jun-00 (gordy)
**	    Created.
**	 8-Jan-01 (gordy)
**	    Extracted to exec() method to support batch processing.
**      29-Aug-01 (loera01) SIR 105641
**          Return true if a result set is returned.
*/

public boolean
execute()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".execute()" );
    exec( params );
    return( resultSet != null );
} // execute


/*
** Name: addBatch
**
** Description:
**	Add parameters to batch set.  
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	11-Jan-01 (gordy)
**	    Created.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
**      14-Mar-01 (loera01) Bug 104327
**          If an out or in_out parameter is found, mark this case by
**          adding a null entry to the batch linked list.  The executeBatch
**          method will use this to determine that an out parameter was
**          attempted.  Clear parameter resources when done.
*/

public void
addBatch()
    throws SQLException
{
    boolean hasOutParam = false;

    if ( trace.enabled() )  trace.log( title + ".addBatch()" );

    for( int i = 0; i < opMap.length; i++ )
	if ( opMap[ i ] != 0 )
	{
	    hasOutParam = true; // Out parameters are not allowed.
	}
    
    if ( batch == null )  synchronized( this ) { batch = new LinkedList(); }
    synchronized( batch ) 
    { 
	if (hasOutParam == true)
	    batch.addLast(null);
	else
	    batch.addLast( params.clone() ); 
    }
    clearParameters();
    return;
} // addBatch


/*
** Name: executeBatch
**
** Description:
**	Execute the prepared statement for each parameter set
**	in the batch.
**
**	Synchronizes on the batch list so no other thread
**	can do batch operations during execution.  The
**	connection is locked for each individual query in
**	the batch, so other threads can intermix other
**	queries during the batch execution.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int []	    Array of update counts for each query.
**
** History:
**	11-Jan-01 (gordy)
**	    Created.
**	23-Jan-01 (gordy)
**	    Allocate batch list on first access.
**      14-Mar-01 (loera01) Bug 104327
**          If a null parameter set is encountered, assume this was
**          because addBatch() set it due to the presence of an out parameter 
**          and throw an exception.
*/

public int []
executeBatch()
    throws SQLException
{
    int results[];

    if ( trace.enabled() )  trace.log( title + ".executeBatch()" );
    if ( batch == null )  synchronized( this ) { batch = new LinkedList(); }

    synchronized( batch )
    {
	int count = batch.size();
	results = new int[ count ];

	/*
	** Execute each individual query in the batch.
	*/
	for( int i = 0; i < count; i++ )
	    try
	    {
		ParamSet ps = (ParamSet)batch.removeFirst();
		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "] " );
		if (ps == null)
		{
		    if ( trace.enabled() )  
		    {
		        trace.log( title + 
			    ".executeBatch(): OUT parameters not allowed" );
                    }
		    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
                }
		exec( ps );
		results[ i ] = resultProc ? procRetVal : -2;

		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "] = " 
				     + results[ i ] );
	    }
	    catch( NoSuchElementException e )	// Shouldn't happen
	    {
		/*
		** Return only the successful query results.
		*/
		int temp[] = new int[ i ];
		if ( i > 0 )  System.arraycopy( results, 0, temp, 0, i );
		results = temp;
		break;
	    }
	    catch( SQLException e )
	    {
		/*
		** Return only the successful query results.
		*/
		int temp[] = new int[ i ];
		if ( i > 0 )  System.arraycopy( results, 0, temp, 0, i );

		/*
		** Build a batch exception from the sql exception.
		*/
		BatchUpdateException bue;
		bue = new BatchUpdateException( e.getMessage(), e.getSQLState(),
						e.getErrorCode(), temp );
		bue.setNextException( e.getNextException() );
		batch.clear();
		throw bue;
	    }
	
	batch.clear();
    }

    return( results );
} // executeBatch


/*
** Name: exec
**
** Description:
**	Request JDBC server to execute a Database Procedure.
**
** Input:
**	params	    Parameters for procedure.
**
** Ouptut:
**	None.
**
** Returns:
**	void.
**
** History:
**	11-Jan-01 (gordy)
**	    Extracted from execute() to support batch processing.
**      29-Aug-01 (loera01) SIR 105641
**          Removed check for return value.  This is now handled by 
**          setResultProc().  If we have more data to read and output parameters
**          were not specified, use EdbcStmt's resultSet rather than the
**          local resultRow variable.
*/

private void
exec( ParamSet params )
    throws SQLException
{
    clearResults();
    dbc.lock();

    try
    {
	EdbcRSMD    rsmd;

	dbc.begin( JDBC_MSG_QUERY );
	dbc.write( JDBC_QRY_EXP );
	if ( procInfo.getSchema() != null )
	{
	    dbc.write( JDBC_QP_SCHEMA_NAME );
	    dbc.write( procInfo.getSchema() );
	}
	dbc.write( JDBC_QP_PROC_NAME );
	dbc.write( procInfo.getName() );
	dbc.done( procInfo.getParamCount() > 0 ? false : true );

	if ( procInfo.getParamCount() > 0 )
	{
	    send_desc( params );
	    send_params( params );
	}

	if ( (rsmd = readResults( timeout )) == null )
	    dbc.unlock(); 
	else
	{
	    /*
	    ** If output parameters were specified, the output parameters are 
	    ** treated as a result-set with exactly one row and processed as 
	    ** a tuple stream.  The procedure result set class loads the single
	    ** row, shuts the statement and unlocks the connection.
	    */
	    if ( isByRefParam )
	    {
	        resultRow = new RsltProc( this, rsmd, stmt_id );
	        /*
	        ** Map dynamic parameters to the output result set.
	        */
	        for( int i = 0, dynamic = 1; i < opMap.length; i++ )
		    if ( opMap[ i ] != 0 )  opMap[ i ] = dynamic++;
	    }
            else
	        /*
	        ** If no output parameters specified, check for returned rows.
	        */
	        resultSet = new RsltProc( this, rsmd, stmt_id, fetchSize );
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".execute(): error executing procedure" );
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	dbc.unlock(); 
	throw ex;
    }

    return;
} // exec


/*
** Name: clearParameters
**
** Description:
**	Clear stored parameter values and free resources..
**
**	Overrides base method to include the initialization
**	of procedure parameters according to the procedure
**	call syntax.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	19-May-99 (gordy)
**	    Created.
*/

public void
clearParameters()
    throws SQLException
{
    super.clearParameters();
    initParameters();
    return;
} // clearParameters


/*
** Name: clearResults
**
** Description:
**	Clear any existing query results.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Aug-00 (gordy)
**	    Created.
**      29-Aug-01 (loera01) SIR 105641
**          Clear result set if present.
*/

protected void
clearResults()
{
    null_param = false;
    super.clearResults();
    if ( resultRow != null )
    {
        try { resultRow.shut(); }
        catch( SQLException ignore ) {}
        resultRow = null;
    }

    return;
}


/*
** Name: wasNull
**
** Description:
**	Determine if last parameter retrieved was NULL.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    True if last parameter retrieved was NULL, false otherwise.
**
** History:
**	19-May-99 (gordy)
**	    Created.
*/

public boolean
wasNull()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".wasNull(): " + null_param );
    return( null_param );
} // wasNull


/*
** Name: getBoolean
**
** Description:
**	Retrieve parameter data as a boolean value.  If the parameter
**	is NULL, false is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if parameter data is boolean true, false otherwise.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public boolean
getBoolean( int paramIndex )
    throws SQLException
{
    boolean value = false;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getBoolean( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    value = (procRetVal == 0) ? false : true;
    else
    {
	value = resultRow.getBoolean( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getBoolean: " + value );
    return( value );
} // getBoolean

/*
** Name: getByte
**
** Description:
**	Retrieve parameter data as a byte value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	byte		Parameter integer value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public byte
getByte( int paramIndex )
    throws SQLException
{
    byte    value = 0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getByte( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    value = (byte)procRetVal;
    else
    {
	value = resultRow.getByte( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getByte: " + value );
    return( value );
} // getByte

/*
** Name: getShort
**
** Description:
**	Retrieve parameter data as a short value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	short		Parameter integer value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public short
getShort( int paramIndex )
    throws SQLException
{
    short   value = 0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getShort( " + paramIndex + " )" );

   null_param = false;

   if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    value = (short)procRetVal;
    else
    {
	value = resultRow.getShort( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getShort: " + value );
    return( value );
} // getShort

/*
** Name: getInt
**
** Description:
**	Retrieve parameter data as an int value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	int		Parameter integer value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public int
getInt( int paramIndex )
    throws SQLException
{
    int	    value = 0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getInt( " + paramIndex + " )" );

    null_param = false;
    
    if ( isProcRetVal( paramIndex ) )
    {
	if ( ! resultProc )
	    null_param = true;
	else
	    value = procRetVal;
    }
    else
    {
	value = resultRow.getInt( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getInt: " + value );
    return( value );
} // getInt

/*
** Name: getLong
**
** Description:
**	Retrieve parameter data as a long value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	long		Parameter integer value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public long
getLong( int paramIndex )
    throws SQLException
{
    long    value = 0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getLong( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    value = procRetVal;
    else
    {
	value = resultRow.getLong( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getLong: " + value );
    return( value );
} // getLong

/*
** Name: getFloat
**
** Description:
**	Retrieve parameter data as a float value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	float		Parameter numeric value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public float
getFloat( int paramIndex )
    throws SQLException
{
    float   value = 0.0F;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getFloat( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    value = procRetVal;
    else
    {
	value = resultRow.getFloat( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getFloat: " + value );
    return( value );
} // getFloat

/*
** Name: getDouble
**
** Description:
**	Retrieve parameter data as a double value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	double		Parameter numeric value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public double
getDouble( int paramIndex )
    throws SQLException
{
    double  value = 0.0;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getDouble( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    value = procRetVal;
    else
    {
	value = resultRow.getDouble( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getDouble: " + value );
    return( value );
} // getDouble


/*
** Name: getBigDecimal
**
** Description:
**	Retrieve parameter data as a BigDecimal value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	BigDecimal	Parameter numeric value.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
*/

public BigDecimal
getBigDecimal( int paramIndex )
    throws SQLException
{
    BigDecimal	value = null;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getBigDecimal( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    value = BigDecimal.valueOf( (long)procRetVal );
    else
    {
	value = resultRow.getBigDecimal( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getBigDecimal: " + value );
    return( value );
} // getBigDecimal


/*
** Name: getBigDecimal
**
** Description:
**	Retrieve parameter data as a BigDecimal value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**	scale		Number of decimal scale digits.
**
** Output:
**	None.
**
** Returns:
**	BigDecimal	Parameter numeric value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public BigDecimal
getBigDecimal( int paramIndex, int scale )
    throws SQLException
{
    BigDecimal	value = null;
    Object	obj;

    if ( trace.enabled() )  trace.log( title + ".getBigDecimal( " + 
				       paramIndex + ", " + scale + " )" );
    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    value = BigDecimal.valueOf( (long)procRetVal );
    else
    {
	value = resultRow.getBigDecimal( resultMap( paramIndex ), scale );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getBigDecimal: " + value );
    return( value );
} // getBigDecimal


/*
** Name: getString
**
** Description:
**	Retrieve parameter data as a String value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	String		Parameter string value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public String
getString( int paramIndex )
    throws SQLException
{
    String  value = null;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getString( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	{
	    value = Integer.toString( procRetVal );
	    if ( rs_max_len > 0  &&  value.length() > rs_max_len )
		value = value.substring( 0, rs_max_len );
	}
    else
    {
	value = resultRow.getString( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getString: " + value );
    return( value );
} // getString

/*
** Name: getBytes
**
** Description:
**	Retrieve parameter data as a byte array value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	byte[]		Parameter binary array value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public byte[]
getBytes( int paramIndex )
    throws SQLException
{
    byte    value[] = null;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getBytes( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
    else
    {
	value = resultRow.getBytes( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getBytes: " + value );
    return( value );
} // getBytes


/*
** Name: getDate
**
** Description:
**	Retrieve parameter data as a Date value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Date		Parameter date value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
**	 2-Nov-00 (gordy)
**	    Extract implementation to new method with calendar parameter.
*/

public Date
getDate( int paramIndex )
    throws SQLException
{
    return( getDate( paramIndex, null ) );
} // getDate


/*
** Name: getDate
**
** Description:
**	Retrieve parameter data as a Date value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Date		Parameter date value.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
*/

public Date
getDate( int paramIndex, Calendar cal )
    throws SQLException
{
    Date    value = null;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getDate( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
    else
    {
	paramIndex = resultMap( paramIndex );
	value = (cal == null) ? resultRow.getDate( paramIndex )
			      : resultRow.getDate( paramIndex, cal );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getDate: " + value );
    return( value );
} // getDate


/*
** Name: getTime
**
** Description:
**	Retrieve parameter data as a Time value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Time		Parameter time value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
**	 2-Nov-00 (gordy)
**	    Extract implementation to new method with calendar parameter.
*/

public Time
getTime( int paramIndex )
    throws SQLException
{
    return( getTime( paramIndex, null ) );
} // getTime


/*
** Name: getTime
**
** Description:
**	Retrieve parameter data as a Time value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Time		Parameter time value.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
*/

public Time
getTime( int paramIndex, Calendar cal )
    throws SQLException
{
    Time    value = null;
    Object  obj;

    if ( trace.enabled() )  
	trace.log( title + ".getTime( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
    else
    {
	paramIndex = resultMap( paramIndex );
	value = (cal == null) ? resultRow.getTime( paramIndex )
			      : resultRow.getTime( paramIndex, cal );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getTime: " + value );
    return( value );
} // getTime


/*
** Name: getTimestamp
**
** Description:
**	Retrieve parameter data as a Timestamp value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Timestamp	Parameter timestamp value.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
**	 2-Nov-00 (gordy)
**	    Extract implementation to new method with calendar parameter.
*/

public Timestamp
getTimestamp( int paramIndex )
    throws SQLException
{
    return( getTimestamp( paramIndex, null ) );
} // getTimestamp


/*
** Name: getTimestamp
**
** Description:
**	Retrieve parameter data as a Timestamp value.  If the parameter
**	is NULL, null is returned.
**
**	The calendar parameter is ignored since the date value
**	is stored as GMT and the server sends the date as GMT.
**
** Input:
**	paramIndex	Parameter index.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Timestamp	Parameter timestamp value.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
*/

public Timestamp
getTimestamp( int paramIndex, Calendar cal )
    throws SQLException
{
    Timestamp	value = null;
    Object	obj;

    if ( trace.enabled() )  
	trace.log( title + ".getTimestamp( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    throw EdbcEx.get( E_JD0006_CONVERSION_ERR );
    else
    {
	paramIndex = resultMap( paramIndex );
	value = (cal == null) ? resultRow.getTimestamp( paramIndex )
			      : resultRow.getTimestamp( paramIndex, cal );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getTimestamp: " + value );
    return( value );
} // getTimestamp


/*
** Name: getArray
**
** Description:
**	Retrieve parameter data as an array.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Array		Parameter Array value.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
*/

public Array
getArray( int paramIndex )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getArray( " + paramIndex + " )" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // getArray


/*
** Name: getBlob
**
** Description:
**	Retrieve parameter data as a Blob.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Blob		Parameter Blob value.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
*/

public Blob
getBlob( int paramIndex )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getBlob( " + paramIndex + " )" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // getBlob


/*
** Name: getClob
**
** Description:
**	Retrieve parameter data as a Clob.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Clob		Parameter Clob value.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
*/

public Clob
getClob( int paramIndex )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getClob( " + paramIndex + " )" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // getClob


/*
** Name: getRef
**
** Description:
**	Retrieve parameter data as a Ref.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Ref		Parameter Ref value.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
*/

public Ref
getRef( int paramIndex )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getRef( " + paramIndex + " )" );
    throw EdbcEx.get( E_JD0012_UNSUPPORTED );
} // getRef


/*
** Name: getObject
**
** Description:
**	Retrieve parameter data as a generic Java object.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Object		Java object.
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

public Object
getObject( int paramIndex )
    throws SQLException
{
    Object obj = null;

    if ( trace.enabled() )  
	trace.log( title + ".getObject( " + paramIndex + " )" );

    null_param = false;

    if ( isProcRetVal( paramIndex ) )
	if ( ! resultProc )
	    null_param = true;
	else
	    obj = new Integer( procRetVal );
    else
    {
	obj = resultRow.getObject( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }

    if ( trace.enabled() )  trace.log( title + ".getObject: " + obj );
    return( obj );
} // getObject


/*
** Name: getObject
**
** Description:
**	Retrieve parameter data as a generic Java object.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**	map		Type map (ignored).
**
** Output:
**	None.
**
** Returns:
**	Object		Java object.
**
** History:
**	 2-Nov-00 (gordy)
**	    Created.
*/

public Object
getObject( int paramIndex, Map map )
    throws SQLException
{
    return( getObject( paramIndex ) );
} // getObject


/*
** Name: isProcRetVal
**
** Description:
**	Returns an indication that a dynamic parameter index
**	references the procedure return value.
**
** Input:
**	index	    Parameter index (1 based).
**
** Output:
**	None.
**
** Returns:
**	true	    True if reference to procedure return value.
**
** History:
**	 3-Aug-00 (Gordy)
**	    Created.
**      11-Jul-01 (loera01) SIR 105309
**          Added check for parameter at the end of dpMap for Ingres syntax.
*/

private boolean
isProcRetVal( int index )
{
    boolean ret=false;

    if ( ! procInfo.getReturn() )
	ret = false;
    else if ( isNativeDBProc && index == dpMap.length )
	ret = true;
    else if (! isNativeDBProc && index == 1 )
	ret = true;
	
    return ret;
}


/*
** Name: paramMap
**
** Description:
**	Determines if a given parameter index is valid
**	and maps the external index to the internal index.
**
**	Accessible parameters are limited to those which
**	were specified using dynamic parameter markers.
**	These parameters are mapped through the dynamic
**	mapping array to an external parameter index and
**	then mapped to an internal index using the super
**	class mapping method.
**
**	An exception will be thrown if this routine is
**	used to map the parameter for a procedure return
**	value as the dynamic parameter mapping does not
**	result in a valid parameter index.
**
** Input:
**	index	    External parameter index [1,param_cnt].
**
** Output:
**	None.
**
** Returns:
**	int	    Internal parameter index [0,param_cnt - 1].
**
** History:
**	19-May-99 (gordy)
**	    Created.
**	25-Aug-00 (gordy)
**	    Implemented.
*/

protected int
paramMap( int index )
    throws EdbcEx
{
    if ( index < 1  ||  index > dpMap.length )
	throw EdbcEx.get( E_JD0007_INDEX_RANGE );

    return( super.paramMap( dpMap[ index - 1 ] ) );
} // paramMap


/*
** Name: resultMap
**
** Description:
**	Determines if a given parameter index is valid
**	and maps the external index to the internal index.
**
**	Accessible parameters are limited to those which
**	were specified using dynamic parameter markers
**	and were registered as output parameters.
**	These parameters are mapped through the output
**	mapping array to an external column index.
**
**	An exception will be thrown if this routine is
**	used to map the parameter for a procedure return
**	value as the output parameter mapping does not
**	result in a valid column index.
**
** Input:
**	index	    External parameter index [1,param_cnt].
**
** Output:
**	None.
**
** Returns:
**	int	    External column index [1,column_cnt].
**
** History:
**	 3-Aug-00 (gordy)
**	    Created.
*/

private int
resultMap( int index )
    throws EdbcEx
{
    index--;

    if ( index < 0  ||  index >= opMap.length  ||  opMap[ index ] == 0 )
	throw EdbcEx.get( E_JD0007_INDEX_RANGE );

    return( opMap[ index ] );
} // resultMap

} // class EdbcCall
