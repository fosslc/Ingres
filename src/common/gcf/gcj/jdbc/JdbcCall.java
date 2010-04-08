/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcCall.java
**
** Description:
**	Defines class which implements the JDBC CallableStatement interface.
**
**  Classes:
**
**	JdbcCall	Implements the JDBC CallableStatement interface.
**	BatchExec	Batch procedure execution.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	19-Feb-03 (gordy)
**	    Updated to JDBC 3.0.
**	15-Apr-03 (gordy)
**	    Added connection timezones separate from date formatters.
**	 6-Jun-03 (gordy)
**	    Batch parameters must be validated when adding batch since some
**	    validation criteria is outside the scope of the parameter set.
**	 2-Jul-03 (gordy)
**	    The types registered for output parameters must be used to
**	    determine what class of object to return from getObject().
**	 7-Jul-03 (gordy)
**	    Added Ingres timezone config which affects date/time literals.
**	 4-Aug-03 (gordy)
**	    Simplified pre-fetch calculations.
**	20-Aug-03 (gordy)
**	    Add statement AUTO_CLOSE optimization.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 6-Oct-03 (gordy)
**	    First block of rows may now be fetched with query response.
**	 1-Nov-03 (gordy)
**	    ParamSet enhanced to provide most parameter handling functionality.
**      28-Apr-04 (wansh01)
**          Added type as an input for setObejct().
**          The input type is used for init and coerce().
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	28-Jun-06 (gordy)
**	    OUT procedure parameter support.
**	15-Sep-06 (gordy)
**	    Support empty date strings using alternate storage format.
**	15-Nov-06 (gordy)
**	    Support LOB Locators via BLOB/CLOB.
**	26-Feb-07 (gordy)
**	    Additional LOB Locator configuration capabilities.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
**	 4-May-07 (gordy)
**	    Set class access for reflection.
**	20-Jul-07 (gordy)
**	    Byref result-set functionality combined into select-loop class.
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	16-Mar-09 (gordy)
**	    Cleanup handling of internal LOBs.
**      7-Apr-09 (rajus01) SIR 121238
**          Implemented the new JDBC 4.0 methods.
*/

import	java.io.InputStream;
import	java.io.InputStreamReader;
import	java.io.Reader;
import	java.io.IOException;
import	java.math.BigDecimal;
import	java.net.URL;
import	java.util.LinkedList;
import	java.util.Calendar;
import	java.util.Map;
import	java.util.NoSuchElementException;
import	java.sql.Statement;
import	java.sql.CallableStatement;
import	java.sql.ResultSet;
import	java.sql.ParameterMetaData;
import	java.sql.Date;
import	java.sql.Time;
import	java.sql.Timestamp;
import	java.sql.Array;
import	java.sql.Blob;
import	java.sql.Clob;
import	java.sql.NClob;
import	java.sql.SQLXML;
import	java.sql.RowId;
import	java.sql.Ref;
import	java.sql.Types;
import	java.sql.SQLException;
import	java.sql.DataTruncation;
import  java.sql.BatchUpdateException;
import	com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.SqlExFactory;


/*
** Name: JdbcCall
**
** Description:
**	JDBC driver class which implements the CallableStatement interface.
**
**	The following Database Procedure call syntax is supported:
**
**	{[? =] CALL [schema.]name[( params )]}
**      EXECUTE PROCEDURE [schema.]name[( params )] [INTO ?]
**	CALLPROC [schema.]name[( params )] [INTO ?]
**      
**	params: param | param, params
**	param: [name =] [value]
**	value: ? | numeric_literal | char_literal | hex_string |
**		SESSION.table_name
**
**	Parameters with no value are treated as default parameters and
**	are not sent to the DBMS.  Parameter names are optional (and an
**	extension for the JDBC syntax).  If any parameter names are used,
**	then all parameters should be named.  If non-dynamic parameters
**	are present, then parameter names are preferred because the only
**	other source is DBMS meta-data which requires a query.
**
**	Internally, parameter indexes represent actual procedure parameters, 
**	both dynamic and literal.  Externally, parameter indexes represent 
**	dynamic parameters and the procedure result parameter.  External 
**	indexes must be mapped to internal parameter indexes.  Dynamic 
**	parameters may also be registered as output parameters.  Output 
**	parameter indexes must be mapped to result-set indexes.  In both 
**	cases, the procedure result parameter must be handled as a special 
**	case prior to mapping.
**
**	Ingres requires all parameters to be named.  In JDBC 3.0 parameter
**	names may be obtained three ways: SQL text (driver extension), set
**	and register methods (dynamic parameters only) and DBMS meta-data.
**	If the SQL text includes unnamed non-dynamic parameters, meta-data 
**      names must be loaded since there is no other source for these names
**	and the assignment of parameter names is dependent on the order 
**	declared.  
**
**	When only unnamed dynamic parameters are present in the SQL text,
**	the loading of meta-data names is deferred since the names may be
**	provided by set/register methods.  These methods use findParams()
**	to map parameter names to actual parameters.  If parameters names
**	were provided in SQL text or meta-data names loaded due to non-
**	dynamic parameters, findParams() will search the procedure info
**	to map parameter names to the parameter set.  Otherwise, the
**	parameter set will be searched and an unset parameter selected
**	if the parameter name had not been previously assigned.  If the
**	ordinal index versions of the set/register methods are used, meta-
**	data names will need to be loaded and assigned to the parameter
**	set prior to executing the procedure.  This is done in exec().
**
**	In summary, the handling of parameter names is as follows:
**
**	o  Parameter are loaded from the SQL text, if present, when the
**	   text is parsed in the constructor.
**	o  Meta-data names are loaded in the constructor when parameter names
**	   are not provided in the SQL text and non-dynamic parameters are 
**	   present.
**	o  Parameter names are copied to the param-set in initParameters()
**	   when loaded in the previous two steps or in a previous iteration
**	   (initParameters() called from constructor and clearParameters()).
**	o  Parameter names are assigned to the param-set in register and set
**	   methods, using findParam(), when not copied in the preceding step.
**	o  Parameter names are matched to existing entries in the param-set 
**	   using findParam() when assigned in the previous two steps.
**	o  Finally, prior to execution, meta-data names are loaded and copied 
**	   (in checkParams()) to the param-set if not done in any prior step.
**	   Note that this implies all parameters are dynamic and register/set
**	   was done by ordinal.
**	o  Clearing the parameters clears the names from the param-set
**	   (may be restored in initParameters()) but not loaded names.
**
**  Interface Methods:
**
**	getParameterMetaData	Retrieves parameter meta-data.
**	registerOutParameter	Register SQL type of OUT parameters.
**	setNull			Send parameter as a NULL value.
**	setBoolean		Send parameter as a boolean value.
**	setByte			Send parameter as a byte value.
**	setShort		Send parameter as a short value.
**	setInt			Send parameter as a int value.
**	setLong			Send parameter as a long value.
**	setFloat		Send parameter as a float value.
**	setDouble		Send parameter as a double value.
**	setBigDecimal		Send parameter as a BigDecimal value.
**	setString		Send parameter as a string value.
**	setBytes		Send parameter as a byte array value.
**	setDate			Send parameter as a Date value.
**	setTime			Send parameter as a Time value.
**	setTimestamp		Send parameter as a Timestamp value.
**	setBinaryStream		Send parameter as a binary stream.
**	setAsciiStream		Send parameter as an ASCII stream.
**	setCharacterStream	Send parameter as a character stream.
**	setObject		Send parameter as a generic Java object.
**	setArray		Send parameter as an array.
**	setBlob			Send parameter as a Blob.
**	setClob			Send parameter as a Clob.
**	setRef			Send parameter as a Ref.
**	setURL			Send parameter as a URL.
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
**	getURL			Retrieve parameter as a URL.
**
**  Overriden Methods:
**
**      executeUpdate		Execute a non-row producing procedure.
**      executeQuery		Execute a row producing procedure.
**	execute			Execute a procedure.
**	addBatch		Add parameter set to batch list.
**	executeBatch		Execute procedure for param set in batch list.
**	interateBatch		Execute individual Batch entries.
**	clearParameters		Clear parameter set.
**
**  Private Data:
**
**	procInfo		Procedure and parameter information.
**	procQPMD		Parameter meta-data.
**	procRsltIndex		Index of procedure result parameter.
**	hasByRefParam		Is a parameter registered as a OUT parameter.
**	resultRow		Byref parameter result values.
**	null_param		True if last parameter retrieved was NULL.
**	dpMap			Dynamic parameter marker map.
**	opMap			Output parameter marker map.
**	opType			Output parameter datatype.
**	opScale			Output parameter scale.
**	noMap			Empty parameter map.
**
**  Private Methods:
**
**      initParameters		Initialize procedure parameters.
**      exec			Execute db procedure.
**      isProcRslt		Is parameter a return a value?
**	checkParams		Validate parameter info.
**	findParam		Map dynamic parameter name to param index.
**	paramMap		Map dynamic parameter index to param index.
**	resultMap		Map output param index to result-set index.
**	unMap			Convert result-set index to param index.
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
**          result sets.  Added local RsltProc variable and hasByRefParam 
**          boolean to separate output parameters from multiple-row result 
**          sets.  All getXXX methods now reference local RsltProc variable.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 4-Mar-03 (gordy)
**	    Added get, set and register by name methods, findParam().
**	    Added access_type and constants to to ensure name/ordinal
**	    consistency.  Replaced isNativeDBProc with procRsltIndex
**	    and renamed isProcRetVal() to isProcRslt().
**	 6-Jun-03 (gordy)
**	    Added checkParams() (extracted from exec()) to validate that
**	    parameter set is ready for execution.
**	 2-Jul-03 (gordy)
**	    Added resultMap() to map parameter names to result index,
**	    unMap() to undo resultMap() mapping, opType and opScale to 
**	    save registered output type information for getObject(), 
**	    and private getObject() method to convert parameter results 
**	    into registered output type.
**	 1-Nov-03 (gordy)
**	    Implemented setObject() method without scale parameter because
**	    ParamSet no longer overloads scale with negative values.
**	25-Aug-06 (gordy)
**	    Added procQPMD.
**	15-Nov-06 (gordy)
**	    Implemented getBlob(), getClob(), setBlob(), setClob()
**	    to support DBMS LOB Locators.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, permit access.
**	20-Jul-07 (gordy)
**	    Byref result-set functionality combined into select-loop class.
*/

public class
JdbcCall
    extends	JdbcPrep
    implements	CallableStatement
{

    /*
    ** Constants
    */
    private static final int	NONE = 0;		// Param access mode
    private static final int	ORDINAL = 1;
    private static final int	NAME = 2;

    private static final int	noMap[] = new int[0];	// Map for no params.

    private ProcInfo		procInfo;		// Parsing information
    private JdbcQPMD		procQPMD = null;	// Parameter meta-data
    private int			procRsltIndex = -1;	// Result param index.
    private boolean		hasByRefParam = false;	// BYREF parameters?
    private RsltSlct		resultRow = null;	// Output parameters
    private boolean		null_param = false;	// Last value null?
    private int			access_type = NONE;	// Param access mode.
    private int			dpMap[] = noMap;	// Dynamic param map
    private int			opMap[] = noMap;	// Output param map
    private int			opType[] = noMap;	// Output param type
    private int			opScale[] = noMap;	// Output param scale


/*
** Name: JdbcCall
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.  
**	19-Feb-03 (gordy)
**	    Delay loading parameter names until procedure is executed
**	    to allow for parameter names to be provided by application.
**	    Added cursor holdability.
**	15-Apr-03 (gordy)
**	    Added connection timezones separate from date formatters.
**	 6-Jun-03 (gordy)
**	    Clear parameter set if only default params.
**	 2-Jul-03 (gordy)
**	    Output mapping now requires dynamic mapping first.  Allocate
**	    arrays for output type and scale.
**	 7-Jul-03 (gordy)
**	    Added Ingres timezone config which affects date/time literals.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	19-Jun-06 (gordy)
**	    I give up... GMT indicator replaced with connection.
**	 4-May-07 (gordy)
**	    Class is exposed outside package, restrict constructor access.
**	25-Mar-10 (gordy)
**	    Moved parameter default flag initialization to initParameters()
**	    so that batch parameter sets will be properly initialized by
**	    addBatch() (which also calls initParameters()).
*/

// package access
JdbcCall( DrvConn conn, String sql )
    throws SQLException
{
    /*
    ** Initialize base class and override defaults.
    */
    super( conn, ResultSet.TYPE_FORWARD_ONLY, 
	   DRV_CRSR_READONLY, ResultSet.CLOSE_CURSORS_AT_COMMIT );
    title = trace.getTraceName() + "-CallableStatement[" + inst_id + "]";
    tr_id = "Call[" + inst_id + "]";

    
    /*
    ** Process the call procedure syntax.
    */
    boolean native_proc = false;

    try
    {
	SqlParse parser = new SqlParse( sql, conn );
	procInfo = new ProcInfo( conn );
	parser.parseCall( procInfo );
	native_proc = (parser.getQueryType() == parser.QT_NATIVE_PROC);
    }
    catch( SQLException ex )
    {
 	if ( trace.enabled() )  
	    trace.log( title + ": error parsing query text" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
   }

    /*
    ** Count the number of dynamic parameters and allocate
    ** the dynamic parameter mapping arrays. Clear the param
    ** information if only default parameters.
    */
    int	param_cnt = procInfo.getParamCount();
    int	dflt_cnt = 0;
    int	dynamic = 0; 

    for( int param = 0; param < param_cnt; param++ )
	switch( procInfo.getParamType( param ) )
	{
	case ProcInfo.PARAM_DYNAMIC :	dynamic++;	break;
	case ProcInfo.PARAM_DEFAULT :	dflt_cnt++;	break;
	}

    if ( param_cnt > 0 )  
	if ( param_cnt == dflt_cnt )
	    procInfo.clearParams();	// No real parameters.
	else  if ( param_cnt != dynamic )
	{
	    /*
	    ** Load meta-data parameter names to ensure
	    ** correct relationship between dynamic and
	    ** non-dynamic parameters.  See discussion
	    ** in class description for further details.
	    */
	    if ( ! procInfo.paramNamesLoaded() )  procInfo.loadParamNames();
	}
    
    
    if ( procInfo.getReturn() )  
    {
	/*
	** The mapping arrays include an entry for the procedure 
	** result parameter to provide the correct offest for other 
	** parameters, but the entry can't be used for mapping.  
	*/
	dynamic++;

	/*
	** Native procedure syntax places the procedure result
	** parameter last.  JDBC escape syntax places it first.
	*/
	procRsltIndex = (native_proc ? dynamic : 1);
    }

    /*
    ** The dynamic parameter map is used to map dynamic parameter 
    ** ordinal indexes into internal parameter indexes.  Mapping
    ** is required because the internal parameters include non-
    ** dynamic parameters and the dynamic parameters may include
    ** the procedure result.
    */
    dpMap = (dynamic > 0) ? new int[ dynamic ] : noMap;
    
    /*
    ** The output parameter map is used to map internal parameter
    ** indexes (see above) into output result map indexes.  The
    ** registered type/scale is saved in associated arrays (which
    ** also provide space, in the last entry, for the procedure
    ** result registration info).
    */
    opMap = (param_cnt > 0) ? new int[ param_cnt ] : noMap;
    opType = new int[ param_cnt + 1 ];	// Include procedure result
    opScale = new int[ param_cnt + 1 ];	// Include procedure result

    /*
    ** Initialize the dynamic parameter mapping array 
    ** and any static literal parameters.
    */
    initParameters();
    return;
} // JdbcCall


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
**          Initialize hasByRefParam to false.
**	 4-Mar-03 (gordy)
**	    Parameter names may now be provided by set methods.  Names
**	    for non-dynamic parameters must be available.  Changed
**	    tracking of procedure result parameter.  Strings now have
**	    alternate storage format.
**	 2-Jul-03 (gordy)
**	    Clear all output registrations.
**	 1-Nov-03 (gordy)
**	    Initialize scale to negative value to indicate no scale.
**	    ParamSet functionality extended.
**      28-Apr-04 (wansh01)
**          Changed setObject() to include type as input parameter. 
**	25-Mar-10 (gordy)
**	    Set default parameter flags as procedure parameters.
*/

private synchronized void
initParameters()
    throws SQLException
{
    boolean	names_loaded = procInfo.paramNamesLoaded();
    int		param_cnt = procInfo.getParamCount();
    int		dynamic = 0;
    
    access_type = NONE;
    hasByRefParam = false;

    /*
    ** Mark all parameters as unregistered (not output).
    */
    for( int i = 0; i < opMap.length; i++ )  opMap[ i ] = -1;
    
    /*
    ** Ingres procedures only return an integer result, so we
    ** don't require the procedure result parameter to be
    ** registered.  If it is registered, the type/scale info
    ** is saved in the last entry of registration arrays.
    ** Initialize the default procedure result type/scale.
    */
    opType[ opType.length - 1 ] = Types.INTEGER;
    opScale[ opScale.length - 1 ] = -1;	// No scale
    
    /*
    ** JDBC escape syntax places the procedure result parameter as
    ** the first dynamic parameter.  Mark as unmapped if present.
    */
    if ( isProcRslt( 1 ) )  dpMap[ dynamic++ ] = -1;

    /*
    ** Flag all parameters as procedure parameters.
    */
    params.setDefaultFlags( ParamSet.PS_FLG_PROC_IN );

    /*
    ** For non-dynamic parameters, the constructor made sure
    ** parameter names were loaded.  If only dynamic parameters
    ** were used, name loading is deferred to see if names will
    ** be provided by the set/register methods.
    */
    for( int param = 0; param < param_cnt; param++ )
    {
	switch( procInfo.getParamType( param ) )
	{
	case ProcInfo.PARAM_DEFAULT :	/* Unspecified parameters */
	    /*
	    ** Nothing is done with default parameters.  A param
	    ** entry is required to properly align parameter names
	    ** when fetched from the DBMS, but since the default
	    ** value is desired, the parameter is not actually
	    ** sent to the DBMS.
	    */
	    break;

	case ProcInfo.PARAM_DYNAMIC :	/* Dynamic parameters */
	    /*
	    ** Map parameter for assigning input (setXXX) and
	    ** registering as output.
	    */
	    dpMap[ dynamic++ ] = param;
	    break;

	case ProcInfo.PARAM_SESSION :	/* Global Temp Table */
	    params.init( param, Types.VARCHAR, true );
	    params.setObject( param, procInfo.getParamValue( param ), 
				Types.VARCHAR );
	    params.setFlags( param, ParamSet.PS_FLG_PROC_GTT );
	    break;

	default :			/* Literal input parameters */
	    {
	        Object value = procInfo.getParamValue( param );
		params.init( param, value );
	        params.setObject( param, value );
	    }
	    break;
	}
	
	if ( names_loaded )  
	    params.setName( param, procInfo.getParamName( param ) );
    }

    /*
    ** Mark as unmapped any unused entries. This should 
    ** only happen for native syntax where the procedure
    ** result is the last dynamic parameter.
    */
    for( ; dynamic < dpMap.length; dynamic++ )  dpMap[ dynamic ] = -1;
    return;
} // initParameters


/*
** Name: executeQuery
**
** Description:
**	Execute a DB procedure that returns a result-set.  Ingres
**	does not currently support OUT parameters (BYREF) with row
**	producing procedures.
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
**	 6-Jun-03 (gordy)
**	    Parameters must now be validated before calling exec().
*/

public ResultSet
executeQuery()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".executeQuery()" );
    if ( hasByRefParam )  throw SqlExFactory.get( ERR_GC4015_INVALID_OUT_PARAM );
    checkParams( params );
    exec( params );
    if ( resultSet == null ) throw SqlExFactory.get( ERR_GC4017_NO_RESULT_SET );
    if ( trace.enabled() )  trace.log(title + ".executeQuery(): " + resultSet);
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
**	 6-Jun-03 (gordy)
**	    Parameters must now be validated before calling exec().
*/

public int
executeUpdate()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".executeUpdate()" );
    checkParams( params );
    exec( params );
    
    /*
    ** We don't want result sets to be returned with this method.
    ** If present, clean up the tuple steam and thrown an exception.
    */
    if ( resultSet != null )
    {
        try { resultSet.shut(); }
        catch( SQLException ignore ) {}
        resultSet = null;
        throw SqlExFactory.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );    
    }

    /*
    ** Ingres DB procedures don't return the number of rows updated, so
    ** always return zero.
    */
    if ( trace.enabled() )  trace.log( title + ".executeUpdate(): 0" );
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
**	 6-Jun-03 (gordy)
**	    Parameters must now be validated before calling exec().
*/

public boolean
execute()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".execute()" );
    checkParams( params );
    exec( params );
    boolean isRS = (resultSet != null);
    if ( trace.enabled() )  trace.log( title + ".execute(): " + isRS );
    return( isRS );
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
**	31-Oct-02 (gordy)
**	    Super-class now provides newBatch() to initialize batch list.
**	24-Feb-03 (gordy)
**	    Presence of BYREF parameters now flagged when registered.
**	 6-Jun-03 (gordy)
**	    Parameters must now be validated before calling exec().
**	 1-Nov-03 (gordy)
**	    ParamSet no longer clonable.  Save current params and alloc new.
*/

public void
addBatch()
    throws SQLException
{
    boolean hasOutParam = false;

    if ( trace.enabled() )  trace.log( title + ".addBatch()" );
    checkParams( params );

    if ( batch == null )  newBatch();
    synchronized( batch ) 
    { 
	if ( hasByRefParam )
	{
	    batch.addLast( null );
	    params.clear( false );
	}
	else
	{
	    batch.addLast( params );
	    params = new ParamSet( conn );
	}
	
	initParameters();
    }
    
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
**	can do batch operations during execution.
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
**	25-Mar-10 (gordy)
**	    Implement support for actual batch processing.  Simulated
**	    batch processing moved to iterateBatch().
*/

public int []
executeBatch()
    throws SQLException
{
    int	    results[];

    if ( trace.enabled() )  trace.log( title + ".executeBatch()" );
    if ( batch == null )  return( BatchExec.noResults );

    /*
    ** Execute procedure for each parameter set individually when
    ** batch processing isn't supported on the connection or has
    ** been disabled.
    */
    if ( (conn.cnf_flags & conn.CNF_BATCH) == 0  ||
	 conn.db_protocol_level < DBMS_API_PROTO_6 ) 
	results = iterateBatch();
    else
	synchronized( batch )
	{
	    BatchExec batchEx = new BatchExec( conn );
	    try { results = batchEx.execute( batch, procInfo ); }
	    finally { batch.clear(); }
	}

    return( results );
} // executeBatch


/*
** Name: iterateBatch
**
** Description:
**	Individually execute the prepared statement for each
**	parameter set in the batch.
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
**	31-Oct-02 (gordy)
**	    Check for NULL batch instead of allocating.
**	25-Feb-03 (gordy)
**	    Batches can't have BYREF parameters.  Save the current
**	    indicator and set appropriatly.
**	25-Mar-10 (gordy)
**	    Extracted from executeBatch() as alternative to actual
**	    batch processing.
*/

private int []
iterateBatch()
    throws SQLException
{
    int	    results[];
    boolean savedByRef = hasByRefParam;
    hasByRefParam = false;

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
		ParamSet ps = (ParamSet)batch.pollFirst();

		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "] " );
		
		if ( ps == null )
		{
		    if ( trace.enabled() )  
		    {
		        trace.log( title + 
			    ".executeBatch(): OUT parameters not allowed" );
                    }
		    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
                }
		
		exec( ps );
		results[ i ] = ((rslt_items & RSLT_PROC_VAL) != 0) 
			       ? rslt_val_proc : Statement.SUCCESS_NO_INFO;

		if ( trace.enabled() )  
		    trace.log( title + ".executeBatch[" + i + "] = " 
				     + results[ i ] );
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
		hasByRefParam = savedByRef;
		throw bue;
	    }
	
	batch.clear();
    }

    hasByRefParam = savedByRef;
    return( results );
} // iterateBatch


/*
** Name: exec
**
** Description:
**	Request JDBC server to execute a Database Procedure.
**
** Input:
**	param_set	Parameters for procedure.
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
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.  New result-set class for
**	    BYREF parameters.
**	24-Feb-03 (gordy)
**	    Load parameter names if not already available.  Make sure
**	    non-default parameters have been set.
**	 6-Jun-03 (gordy)
**	    Extracted parameter set validation to checkParams() so that the
**	    parameter set can be validated before this call (in particular,
**	    in addBatch()).
**	20-Aug-03 (gordy)
**	    Send AUTO_CLOSE flag for query statements.
**	 6-Oct-03 (gordy)
**	    Fetch first block of rows.
**	 1-Nov-03 (gordy)
**	    ParamSet now builds DESC/DATA messages.
**	26-Feb-07 (gordy)
**	    Super class provides method to test for valid state
**	    for LOB Locators.
**	20-Jul-07 (gordy)
**	    Byref result-set functionality combined into select-loop class.
*/

private void
exec( ParamSet param_set )
    throws SQLException
{
    clearResults();
    msg.lock();
    try
    {
	String	    schema = procInfo.getSchema();
	boolean	    needEOG = true;
	JdbcRSMD    rsmd;

	msg.begin( MSG_QUERY );
	msg.write( MSG_QRY_EXP );
	
	if ( conn.msg_protocol_level >= MSG_PROTO_3 )
	{
	    short flags = MSG_QF_FETCH_FIRST | MSG_QF_AUTO_CLOSE;
	    if ( enableLocators( true ) )  flags |= MSG_QF_LOCATORS;

	    msg.write( MSG_QP_FLAGS );
	    msg.write( (short)2 );
	    msg.write(  flags );
	    needEOG = false;
	}
	
	if ( schema != null )
	{
	    msg.write( MSG_QP_SCHEMA_NAME );
	    msg.write( schema );
	}
	
	msg.write( MSG_QP_PROC_NAME );
	msg.write( procInfo.getName() );

	if ( procInfo.getParamCount() < 1 )
	    msg.done( true );
	else
	{
	    msg.done( false );
	    param_set.sendDesc( false );
	    param_set.sendData( true );
	}

	if ( (rsmd = readResults( timeout, needEOG )) == null )
	{
	    /*
	    ** Procedure execute has completed and all results have
	    ** been returned.  Unlock the connection.
	    */
	    if ( msg.moreMessages() )  readResults( timeout, true );
	    msg.unlock(); 
	}
	else  if ( hasByRefParam )
	{
	    /*
	    ** BYREF parameters are treated as a result-set with 
	    ** exactly one row and processed as a tuple stream.  
	    ** Select-loop result-set class provides a constructor
	    ** which loads the single row, shuts the statement and 
	    ** unlocks the connection.
	    */
	    resultRow = new RsltSlct( conn, this, rsmd, 
				      rslt_val_stmt, msg.moreMessages() );

	    /*
	    ** Map output parameters to the output result-set.
	    */
	    for( int i = 0, index = 1; i < opMap.length; i++ )
		if ( opMap[ i ] >= 0 )  opMap[ i ] = index++;
	}
        else
	{
	    /*
	    ** Procedure result rows are handled like select loop
	    ** results and the connection will be unlocked when
	    ** the end of the result set is reached.
	    */
	    resultSet = new RsltSlct( conn, this, rsmd, rslt_val_stmt, 
				      getPreFetchSize(), msg.moreMessages() );
	}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".execute(): error executing procedure" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	msg.unlock(); 
	throw ex;
    }

    return;
} // exec


/*
** Name: getParameterMetaData
**
** Description:
**	Returns meta-data object describing the input parameters.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ParameterMetaData   Parameter meta-data.
**
** History:
**	30-Jun-06 (gordy)
**	    Created.
**	25-Aug-06 (gordy)
**	    Implemented.
*/

public ParameterMetaData
getParameterMetaData()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getParameterMetaData()" );

    if ( procQPMD == null )  
	try
	{
	    procQPMD = new JdbcQPMD( conn, procInfo.getSchema(), 
					   procInfo.getName() );
	}
	catch( SQLException ex )
	{
	    if ( trace.enabled() )  
		trace.log( title + ": error retrieving parameter info" );
	    if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	    throw ex;
	}

    if ( trace.enabled() )  
	trace.log( title + ".getParameterMetaData(): " + procQPMD );

    return( procQPMD );
} // getParameterMetaData


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

protected synchronized void
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
** Name: clearParameters
**
** Description:
**	Clear stored parameter values and free resources.
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
    if ( trace.enabled() )  trace.log( title + ".clearParameters()" );
    super.clearParameters();
    initParameters();
    return;
} // clearParameters


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
**          Set hasByRefParam to true.
**	25-Feb-03 (gordy)
**	    Set NULL input value if not previously set.
**	 2-Jul-03 (gordy)
**	    Output map requires dynamic mapping first.
**	    Save type and scale information for getObject().
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	28-Jun-06 (gordy)
**	    OUT procedure parameter support.
*/

public void
registerOutParameter( int paramIndex, int sqlType, int scale )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".registerOutParameter(" + 
			    paramIndex + "," + sqlType + "," + scale + ")" );

    if ( isProcRslt( paramIndex ) )
    {
	/*
	** The procedure result parameter is not an actual procedure
	** parameter and is not mappable.  The last entry in the output
	** type info is saved for the procedure result.
	*/
	opType[ opType.length - 1 ] = sqlType;
	opScale[ opScale.length - 1 ] = scale;
    }
    else
    {
	int index = paramMap( paramIndex );
	
	/*
	** Set output parameter map and save type info for 
	** getObject().  The actual mapping will be done after 
	** all parameters have been registered, so for now 
	** zero is used to indicate a mappable parameter.
	*/
	opMap[ index ] = 0;
	opType[ index ] = sqlType;
	opScale[ index ] = scale;
	hasByRefParam = true;

	/*
	** Output parameters require at least a generic NULL
	** input value.  Normally, calling setNull() will
	** flag the parameter as an IN parameter, so the
	** default flags need to be manipulated to provide
	** a value for the output parameter while avoiding
	** marking the parameter as IN.
	*/
	if ( ! params.isSet( index ) )  
	{
	    params.setDefaultFlags( (short)0 );
	    params.init( index, Types.NULL );
	    params.setNull( index );
	    params.setDefaultFlags( ParamSet.PS_FLG_PROC_IN );
	}
	
	/*
	** Finally, flag parameter is an output parameter.
	*/
	params.setFlags( index, ParamSet.PS_FLG_PROC_OUT );
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
**	parameterName	Parameter name.
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
**	 4-Mar-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Save type and scale information for getObject().
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	28-Jun-06 (gordy)
**	    OUT procedure parameter support.
*/

public void
registerOutParameter( String parameterName, int sqlType, int scale )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".registerOutParameter('" + 
			    parameterName + "'," + sqlType + "," + scale + ")" );

    int index = findParam( parameterName );
    
    /*
    ** Set output parameter map and save type info for 
    ** getObject().  The actual mapping will be done after 
    ** all parameters have been registered, so for now 
    ** zero is used to indicate a mappable parameter.
    */
    opMap[ index ] = 0;
    opType[ index ] = sqlType;
    opScale[ index ] = scale;
    hasByRefParam = true;

    /*
    ** Output parameters require at least a generic NULL
    ** input value.  Normally, calling setNull() will
    ** flag the parameter as an IN parameter, so the
    ** default flags need to be manipulated to provide
    ** a value for the output parameter while avoiding
    ** marking the parameter as IN.
    */
    if ( ! params.isSet( index ) ) 
    {
	params.setDefaultFlags( (short)0 );
	params.init( index, Types.NULL );
	params.setNull( index );
	params.setDefaultFlags( ParamSet.PS_FLG_PROC_IN );
    }
    
    /*
    ** Finally, flag parameter is an output parameter.
    */
    params.setFlags( index, ParamSet.PS_FLG_PROC_OUT );
    return;
} // registerOutParameter


/*
** Register methods which are simple covers
** for the primary register methods.
*/
public void
registerOutParameter( int index, int type )  throws SQLException
{ registerOutParameter( index, type, -1 ); }

public void
registerOutParameter( String name, int type )  throws SQLException
{ registerOutParameter( name, type, -1 ); }

public void
registerOutParameter( int index, int type, String tn )  throws SQLException
{ registerOutParameter( index, type, -1 ); }

public void
registerOutParameter( String name, int type, String tn )  throws SQLException
{ registerOutParameter( name, type, -1 ); }


/*
** Name: setNull
**
** Description:
**	Set parameter to be a NULL value.  The parameter is assigned
**	a NULL value of the requested SQL type.
**
** Input:
**	parameterName	Parameter name.
**	sqlType		Column SQL type (java.sql.Types).
**	typeName	Name of SQL type (ignored).
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setNull( String parameterName, int sqlType, String typeName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setNull('" + parameterName + "'," + sqlType + 
		   ((typeName == null) ? ")" : ",'" + typeName + "')") );

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, sqlType );
    params.setNull( paramIndex );
    return;
} // setNull


/*
** Name: setBoolean
**
** Description:
**	Set parameter to a boolean value.
**
** Input:
**	parameterName	Parameter name.
**	value		Parameter value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setBoolean( String parameterName, boolean value )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setBoolean('" + 
				       parameterName + "'," + value + ")" );

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.BOOLEAN );
    params.setBoolean( paramIndex, value );
    return;
} // setBoolean


/*
** Name: setByte
**
** Description:
**	Set parameter to a byte value.
**
** Input:
**	parameterName	Parameter name.
**	value		Byte value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setByte( String parameterName, byte value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setByte('" + parameterName + "'," + value + ")" );
    
    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.TINYINT );
    params.setByte( paramIndex, value );
    return;
} // setByte


/*
** Name: setShort
**
** Description:
**	Set parameter to a short value.
**
** Input:
**	parameterName	Parameter name.
**	value		Short value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setShort( String parameterName, short value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setShort('" + parameterName + "'," + value + ")" );
    
    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.SMALLINT );
    params.setShort( paramIndex, value );
    return;
} // setShort


/*
** Name: setInt
**
** Description:
**	Set parameter to an int value.
**
** Input:
**	parameterName	Parameter name.
**	value		Int value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setInt( String parameterName, int value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + " setInt('" + parameterName + "'," + value + ")" );

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.INTEGER );
    params.setInt( paramIndex, value );
    return;
} // setInt


/*
** Name: setLong
**
** Description:
**	Set parameter to a long value.
**
** Input:
**	parameterName	Parameter name.
**	value		Long value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setLong( String parameterName, long value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setLong('" + parameterName + "'," + value + ")" );

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.BIGINT );
    params.setLong( paramIndex, value );
    return;
} // setLong


/*
** Name: setFloat
**
** Description:
**	Set parameter to a float value.
**
** Input:
**	parameterName	Parameter name.
**	value		Float value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setFloat( String parameterName, float value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setFloat('" + parameterName + "'," + value + ")" );

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.REAL );
    params.setFloat( paramIndex, value );
    return;
} // setFloat


/*
** Name: setDouble
**
** Description:
**	Set parameter to a double value.
**
** Input:
**	parameterName	Parameter name.
**	value		Double value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setDouble( String parameterName, double value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log(title + ".setDouble('" + parameterName + "'," + value + ")");

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.DOUBLE );
    params.setDouble( paramIndex, value );
    return;
} // setDouble


/*
** Name: setBigDecimal
**
** Description:
**	Set parameter to a BigDecimal value.
**
** Input:
**	parameterName	Parameter name.
**	value		BigDecimal value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setBigDecimal( String parameterName, BigDecimal value )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setBigDecimal('" + 
				       parameterName + "'," + value + ")" );

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.DECIMAL );
    params.setBigDecimal( paramIndex, value );
    return;
} // setBigDecimal


/*
** Name: setString
**
** Description:
**	Set parameter to a String value.
**
** Input:
**	parameterName	Parameter name.
**	value		String value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setString( String parameterName, String value )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log(title + ".setString('" + parameterName + "'," + value + ")");

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.VARCHAR );
    params.setString( paramIndex, value );
    return;
} // setString


/*
** Name: setBytes
**
** Description:
**	Set parameter to a byte array value.
**
** Input:
**	parameterName	Parameter name.
**	value		Byte array value.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Clean up tracing of array parameter.
*/

public void
setBytes( String parameterName, byte value[] )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBytes('" + parameterName + "'," + 
		   (value == null ? "null" : "[" + value.length + "]") + ")");

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.VARBINARY );
    params.setBytes( paramIndex, value );
    return;
} // setBytes


/*
** Name: setDate
**
** Description:
**	Set parameter to a Date value.
**
**	If the connection is not using the GMT timezone, the timezone
**	associated with the calendar parameter (or local timezone) is 
**	used.
**
** Input:
**	parameterName	Parameter name.
**	value		Date value.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setDate( String parameterName, Date value, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setDate('" + parameterName + "'," + value + 
		   ((cal == null) ? " )" : ", " + cal + " )") );
    
    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.DATE );
    params.setDate( paramIndex, value,
		    (cal == null) ? null : cal.getTimeZone() );
    return;
} // setDate


/*
** Name: setTime
**
** Description:
**	Set parameter to a Time value.
**
**	If the connection is not using the GMT timezone, the timezone
**	associated with the calendar parameter is used.
**
** Input:
**	parameterName	Parameter name.
**	value		Time value.
**	cal		Calendar for timezone, may be null.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	21-Jul-03 (gordy)
**	    Apply calendar generically before connection based processing.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setTime( String parameterName, Time value, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setTime('" + parameterName + "'," + value + 
		   ((cal == null) ? " )" : ", " + cal + " )") );
    
    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.TIME );
    params.setTime( paramIndex, value,
		    (cal == null) ? null : cal.getTimeZone() );
    return;
} // setTime


/*
** Name: setTimestamp
**
** Description:
**	Set parameter to a Timestamp value.
**
**	If the connection is not using the GMT timezone, the timezone
**	associated with the calendar parameter is used.
**
** Input:
**	parameterName	Parameter name.
**	value		Timestamp value.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	15-Apr-03 (gordy)
**	    Abstracted date formatters.
**	21-Jul-03 (gordy)
**	    Apply calendar generically before connection based processing.
**	12-Sep-03 (gordy)
**	    Replaced timezones with GMT indicator and SqlDates utility.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
*/

public void
setTimestamp( String parameterName, Timestamp value, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setTimestamp('" + parameterName + "'," + value + 
		   ((cal == null) ? " )" : ", " + cal + " )") );

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, Types.TIMESTAMP );
    params.setTimestamp( paramIndex, value,
			 (cal == null) ? null : cal.getTimeZone() );
    return;
} // setTimestamp


/*
** Name: setBinaryStream
**
** Description:
**	Set parameter to a binary stream.
**
** Input:
**	parameterName	Parameter name.
**	stream		Binary stream.
**	length		Length of stream in bytes.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	15-Nov-06 (gordy)
**	    Convert to VARBINARY if length is suitable.
*/

public void
setBinaryStream( String parameterName, InputStream stream, long length )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setBinaryStream('" + 
				       parameterName + "'," + length + ")" );

    /*
    ** Check length to see if can be sent as VARBINARY.
    ** Ingres won't coerce BLOB to VARBINARY, but will
    ** coerce VARBINARY to BLOB, so VARBINARY is preferred.
    */
    int paramIndex = findParam( parameterName );

    if ( length >= 0  &&  length <= conn.max_vbyt_len )
    {
        byte    bytes[] = new byte[ (int) length ];

        try
        {
            int len = (length > 0) ? stream.read( bytes ) : 0;

            if ( len != length )
            {
                /*
                ** Byte array limits read so any difference
                ** must be a truncation.
                */
                if ( trace.enabled( 1 ) )
                    trace.write( tr_id + ".setBinaryStream: read only " +
                                 len + " of " + length + " bytes!" );
                setWarning( new DataTruncation( paramIndex, true,
                                                false, (int)length, len ) );
            }
        }
        catch( IOException ex )
        {
            throw SqlExFactory.get( ERR_GC4007_BLOB_IO );
        }
        finally
        {
            try { stream.close(); }
            catch( IOException ignore ) {}
        }

        params.init( paramIndex, Types.VARBINARY );
        params.setBytes( paramIndex, bytes );
    }
    else
    {
	params.init( paramIndex, Types.LONGVARBINARY );
	params.setBinaryStream( paramIndex, stream );
    }

    return;
} // setBinaryStream


/*
** Name: setAsciiStream
**
** Description:
**	Set parameter to an ASCII character stream.
**
** Input:
**	parameterName	Parameter name.
**	stream		ASCII stream.
**	length		Length of stream in bytes.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	15-Nov-06 (gordy)
**	    Convert to VARCHAR if length is suitable.
*/

public void
setAsciiStream( String parameterName, InputStream stream, long length )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setAsciiStream('" + 
				       parameterName + "'," + length + ")" );

    /*
    ** Check length to see if can be sent as VARCHAR.
    ** Ingres won't coerce CLOB to VARCHAR, but will
    ** coerce VARCHAR to CLOB, so VARCHAR is preferred.
    */
    int paramIndex = findParam( parameterName );

    if ( length >= 0  &&  length <= (conn.ucs2_supported ? conn.max_nvch_len
                                                         : conn.max_vchr_len) )
    {
        char    chars[] = new char[ (int) length ];

	try
	{
	    Reader	rdr = new InputStreamReader( stream, "US-ASCII" );
	    int		len = (length > 0) ? rdr.read( chars ) : 0;

	    if ( len != length )
	    {
		/*
		** Character array limits read so any difference
		** must be a truncation.
		*/
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ".setCharacterStream: read only " +
				 len + " of " + length + " characters!" );
		setWarning( new DataTruncation( paramIndex, true,
						false, (int)length, len ) );
	    }
	}
	catch( IOException ex )
	{ 
	    throw SqlExFactory.get( ERR_GC4007_BLOB_IO ); 
	}
	finally
	{
	    try { stream.close(); }
	    catch( IOException ignore ) {}
	}

        params.init( paramIndex, Types.VARCHAR );
        params.setString( paramIndex, new String( chars ) );
    }
    else
    {
	params.init( paramIndex, Types.LONGVARCHAR );
	params.setAsciiStream( paramIndex, stream );
    }

    return;
} // setAsciiStream


/*
** Name: setCharacterStream
**
** Description:
**	Set parameter to a character stream.
**
** Input:
**	parameterName	Parameter name.
**	reader		Character stream.
**	length		Length of stream in bytes.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**	15-Nov-06 (gordy)
**	    Convert to VARCHAR if length is suitable.
*/

public void
setCharacterStream( String parameterName, Reader reader, long length )
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".setCharacterStream('" + 
				       parameterName + "'," + length + ")" );
    
    /*
    ** Check length to see if can be sent as VARCHAR.
    ** Ingres won't coerce CLOB to VARCHAR, but will
    ** coerce VARCHAR to CLOB, so VARCHAR is preferred.
    */
    int paramIndex = findParam( parameterName );

    if ( length >= 0  &&  length <= (conn.ucs2_supported ? conn.max_nvch_len
                                                         : conn.max_vchr_len) )
    {
        char    chars[] = new char[ (int) length ];

        try
        {
	    int len = (length > 0) ? reader.read( chars ) : 0;

	    if ( len != length )
	    {
		/*
		** Character array limits read so any difference
		** must be a truncation.
		*/
		if ( trace.enabled( 1 ) )
		    trace.write( tr_id + ".setCharacterStream: read only " +
				 len + " of " + length + " characters!" );
		setWarning( new DataTruncation( paramIndex, true,
						false, (int)length, len ) );
	    }
        }
        catch( IOException ex )
        { 
	    throw SqlExFactory.get( ERR_GC4007_BLOB_IO ); 
	}
	finally
	{
	    try { reader.close(); }
	    catch( IOException ignore ) {}
	}

        params.init( paramIndex, Types.VARCHAR );
        params.setString( paramIndex, new String( chars ) );
    }
    else
    {
	params.init( paramIndex, Types.LONGVARCHAR );
	params.setCharacterStream( paramIndex, reader );
    }

    return;
} // setCharacterStream


/*
** Name: setBlob
**
** Description:
**	Set parameter to a Blob value.
**
** Input:
**	parameterName	Parameter name.
**	blob		Blob value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize parameter by value.
**	    Otherwise, initialize by type.
*/

public void
setBlob( String parameterName, Blob blob )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setBlob('" + parameterName + "'," + blob + ")" );

    /*
    ** If the Blob represents a locator associated with
    ** the current connection, then the parameter can be
    ** be treated as a BLOB type.  Otherwise, the Blob
    ** is handled generically as a LONGVARBINARY byte
    ** stream.
    */
    int paramIndex = findParam( parameterName );

    if ( blob == null )
    {
	params.init( paramIndex, Types.BLOB );
	params.setNull( paramIndex );
    }
    else  if ( blob instanceof JdbcBlob  &&
	       ((JdbcBlob)blob).isValidLocator( conn ) )
    {
	DrvLOB lob = ((JdbcBlob)blob).getLOB();
	params.init( paramIndex, lob );

	if ( lob instanceof DrvBlob )
	    params.setBlob( paramIndex, (DrvBlob)lob );
	else	// Should not happen
	    params.setBinaryStream( paramIndex, blob.getBinaryStream() );
    }
    else
    {
	params.init( paramIndex, Types.LONGVARBINARY );
	params.setBinaryStream( paramIndex, blob.getBinaryStream() );
    }

    return;
} // setBlob


/*
** Name: setClob
**
** Description:
**	Set parameter to a Clob value.
**
** Input:
**	parameterName	Parameter name.
**	clob		Clob value.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize parameter by value.
**	    Otherwise, initialize by type.
*/

public void
setClob( String parameterName, Clob clob )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setClob('" + parameterName + "'," + clob + ")" );

    /*
    ** If the Clob represents a locator associated with
    ** the current connection, then the parameter can be
    ** be treated as a CLOB type.  Otherwise, the Clob
    ** is handled generically as a LONGVARCHAR character
    ** stream.
    */
    int paramIndex = findParam( parameterName );

    if ( clob == null )
    {
	params.init( paramIndex, Types.CLOB );
	params.setNull( paramIndex );
    }
    else  if ( clob instanceof JdbcClob  &&
	       ((JdbcClob)clob).isValidLocator( conn ) )
    {
	/*
	** Parameter alternate storage format is
	** determined by the type of the locator.
	*/
	DrvLOB lob = ((JdbcClob)clob).getLOB();
	params.init( paramIndex, lob );

	if ( lob instanceof DrvClob )
	    params.setClob( paramIndex, (DrvClob)lob );
	else  if ( lob instanceof DrvNlob )
	    params.setClob( paramIndex, (DrvNlob)lob );
	else	// Should not happen
	    params.setCharacterStream( paramIndex, clob.getCharacterStream() );
    }
    else
    {
	params.init( paramIndex, Types.LONGVARCHAR );
	params.setCharacterStream( paramIndex, clob.getCharacterStream() );
    }

    return;
} // setClob


/*
** Name: setObject
**
** Description:
**	Set parameter to a generic Java object.  SQL type and scale
**	are determined based on the class and value of the Java object.
**
** Input:
**	parameterName	Parameter name.
**	value		Java object.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 1-Nov-03 (gordy)
**	    Created.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize parameter by value.
**	    Otherwise, initialize by type.
*/

public void
setObject( String parameterName, Object value ) throws SQLException
{ 
    if ( trace.enabled() )  
	trace.log( title + ".setObject('" + parameterName + " )" );
    
    /*
    ** Blob/Clob require special handling because they can represent 
    ** LOB locators as well as stream data.  If the LOB represents a
    ** valid locator, extract the driver locator.  Otherwise, derive 
    ** a stream from the value.
    */
    if ( value != null )
	if ( value instanceof Blob )
	{
	    if ( value instanceof JdbcBlob  &&
		 ((JdbcBlob)value).isValidLocator( conn ) )
		value = ((JdbcBlob)value).getLOB();
	    else
		value = ((Blob)value).getBinaryStream();
	}
	else  if ( value instanceof Clob )
	{
	    if ( value instanceof JdbcClob  &&
		 ((JdbcClob)value).isValidLocator( conn ) )
		value = ((JdbcClob)value).getLOB();
	    else
		value = ((Clob)value).getCharacterStream();
	}

    int paramIndex = findParam( parameterName );
    params.init( paramIndex, value );
    params.setObject( paramIndex, value );
    return;
}


/*
** Name: setObject
**
** Description:
**	Set parameter to a generic Java object with the requested
**	SQL type and scale.  
**
**	Special handling of certain Java objects may be requested
**	by setting sqlType to OTHER.  If the object does not have
**	a processing alternative, the value will processed as if
**	setObject() with no SQL type was called (except that the
**	scale value provided will be used for DECIMAL values).
**
** Input:
**	parameterName	Parameter name.
**	value		Java object.
**	sqlType		Target SQL type.
**	scale		Number of digits following decimal point.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 4-Mar-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    ParamSet functionality extended.
**      28-Apr-04 (wansh01)
**          Passed input sqlType to setObejct().
**	    For Types.OTHER, type of the object will be used for setObject().
**	15-Sep-06 (gordy)
**	    Support empty date strings.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	16-Mar-09 (gordy)
**	    For internal LOBs, initialize parameter by value.
**	    Otherwise, initialize by type.
*/

public void
setObject( String parameterName, Object value, int sqlType, int scale )
    throws SQLException
{
    boolean alt = false;
    boolean force = false;

    if ( trace.enabled() )  trace.log( title + ".setObject('" + parameterName +
				       "'," + sqlType + "," + scale + ")" );
    
    /*
    ** Blob/Clob require special handling because they can represent 
    ** LOB locators as well as stream data.  If the LOB represents a
    ** valid locator, extract the driver locator.  Otherwise, derive 
    ** a stream from the value.
    */
    if ( value != null )
	if ( value instanceof Blob )
	{
	    if ( value instanceof JdbcBlob  &&
		 ((JdbcBlob)value).isValidLocator( conn ) )
		value = ((JdbcBlob)value).getLOB();
	    else
		value = ((Blob)value).getBinaryStream();
	}
	else  if ( value instanceof Clob )
	{
	    if ( value instanceof JdbcClob  &&
		 ((JdbcClob)value).isValidLocator( conn ) )
		value = ((JdbcClob)value).getLOB();
	    else
		value = ((Clob)value).getCharacterStream();
	}

    int paramIndex = findParam( parameterName );
    
    switch( sqlType )
    {
    case Types.OTHER :
	/*
	** Force alternate storage format.
	*/
	sqlType = SqlData.getSqlType( value );
	params.init( paramIndex, value, true );
	break;

    case Types.DATE :
    case Types.TIME :
    case Types.TIMESTAMP :
	/*
	** Empty dates are only supported by Ingres dates
	** which require the alternate storage format.
	*/
	if ( 
	     value != null  &&
	     value instanceof String  &&  
	     ((String)value).length() == 0 
	   )
	    params.init( paramIndex, sqlType, true );
	else
	    params.init( paramIndex, sqlType );
	break;

    case Types.LONGVARBINARY :
    case Types.BLOB :
	/*
	** Internally, the BLOB type only supports driver locators.
	** LONGVARBINARY supports all other Blobs and coercions.
	*/
	if ( value == null )
	    params.init( paramIndex, sqlType );
	else  if ( value instanceof DrvBlob )
	{
	    sqlType = Types.BLOB;
	    params.init( paramIndex, value );
	}
	else
	{
	    sqlType = Types.LONGVARBINARY;
	    params.init( paramIndex, sqlType );
	}
	break;

    case Types.LONGVARCHAR :
    case Types.CLOB :
	/*
	** Internally, the BLOB type only supports driver locators.
	** LONGVARCHAR supports all other Clobs and coercions.
	*/
	if ( value == null )
	    params.init( paramIndex, sqlType );
	else  if ( value instanceof DrvClob  ||  value instanceof DrvNlob )
	{
	    sqlType = Types.CLOB;
	    params.init( paramIndex, value );
	}
	else
	{
	    sqlType = Types.LONGVARCHAR;
	    params.init( paramIndex, sqlType );
	}
	break;

    default :
	params.init( paramIndex, sqlType );
	break;
    }
 
    params.setObject( paramIndex, value, sqlType, scale );
    return;
}


/*
** Data save methods which are simple covers 
** for the primary data save methods.
*/

public void 
setNull( String parameterName, int type ) throws SQLException
{ setNull( parameterName, type, null ); }

public void 
setDate( String parameterName, Date value ) throws SQLException
{ setDate( parameterName, value, null ); }

public void 
setTime( String parameterName, Time value ) throws SQLException
{ setTime( parameterName, value, null ); }

public void 
setTimestamp( String parameterName, Timestamp value ) throws SQLException
{ setTimestamp( parameterName, value, null ); }

public void 
setObject( String parameterName, Object value, int type ) throws SQLException
{ setObject( parameterName, value, type, 0 ); }

public void 
setNString(String parameterName, String value) throws SQLException
{ setString(parameterName,value); }

public void 
setNCharacterStream(String parameterName, Reader value, long length)
throws SQLException
{ setCharacterStream( parameterName, value, length ); }

public void 
setBlob(String parameterName, InputStream inputStream)
throws SQLException
{ setBinaryStream(parameterName, inputStream, -1L ); }

public void 
setBlob(String parameterName, InputStream inputStream, long length)
throws SQLException
{ setBinaryStream(parameterName, inputStream, length ); }

public void 
setBinaryStream(String parameterName, InputStream stream, int length)
throws SQLException
{ setBinaryStream( parameterName, stream, (long) length ); }

public void 
setClob(String parameterName, Reader reader) throws SQLException
{ setCharacterStream( parameterName, reader, -1L ); }

public void 
setNClob(String parameterName, NClob value) throws SQLException
{ setClob( parameterName, (Clob) value ); }

public void 
setNClob(String parameterName, Reader reader) throws SQLException
{ setCharacterStream( parameterName, reader, -1L ); }

public void 
setNClob(String parameterName, Reader reader, long length)
throws SQLException
{ setCharacterStream(parameterName, reader, length ); }

public void 
setAsciiStream(String parameterName, InputStream stream, int length)
throws SQLException
{ setAsciiStream(parameterName, stream, (long) length ); }

public void 
setCharacterStream(String parameterName, Reader reader, int length)
throws SQLException
{ setCharacterStream(parameterName, reader, (long) length ); }

public void 
setAsciiStream(String parameterName, InputStream stream) throws SQLException
{ setAsciiStream( parameterName, stream, -1L ); }

public void 
setBinaryStream(String parameterName, InputStream stream) throws SQLException
{ setBinaryStream( parameterName, stream, -1L ); }

public void 
setCharacterStream(String parameterName, Reader reader) throws SQLException
{ setCharacterStream(parameterName, reader, -1L ); }

public void 
setNCharacterStream(String parameterName, Reader value) throws SQLException
{ setCharacterStream(parameterName, value, -1L ); }

public void 
setClob(String parameterName, Reader reader, long length) throws SQLException
{ setCharacterStream(parameterName, reader, length ); }

/*
** Data save methods which are for unsupported types.
*/

public void
setArray( String parameterName, Array array )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setArray('" + parameterName + "')" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setArray


public void
setRef( String parameterName, Ref ref )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setRef('" + parameterName + "')" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setRef


public void
setURL( String parameterName, URL url )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setURL('" + parameterName + "')" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setURL

public void 
setSQLXML(String parameterName, SQLXML xmlObject)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setSQLXML('" + parameterName + "')" );
   throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // setSQLXML

public void 
setRowId(String parameterName, RowId x)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".setRowId('" + parameterName + "')" );
   throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
}

/*
** Name: isWrapperFor
**
** Description:
**      Returns true if this either implements the interface argument or is
**      directly or indirectly a wrapper for an object that does. Returns
**      false otherwise.
**
** Input:
**      iface   Class defining an interface
**
** Output:
**      None.
**
** Returns:
**      true or false.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public boolean
isWrapperFor(Class<?> iface)
throws SQLException
{
   if ( trace.enabled() )
       trace.log(title + ".isWrapperFor(" + iface + ")");
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
	return( iface.isInstance( this ) );

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);

}

/*
** Name: unwrap
**
** Description:
**      Returns an object that implements the given interface to allow
**      access to non-standard methods, or standard methods not exposed by the
**      proxy. If the receiver implements the interface then the result is the
**      receiver or a proxy for the receiver. If the receiver is a wrapper and
**      the wrapped object implements the interface then the result is the
**      wrapped object or a proxy for the wrapped object. Otherwise return the
**      the result of calling unwrap recursively on the wrapped object or
**      a proxy for that result. If the receiver is not a wrapper and does not
**      implement the interface, then an SQLException is thrown.
**
** Input:
**      iface   A Class defining an interface that the result must implement.
**
** Output:
**      None.
**
** Returns:
**      An object that implements the interface.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public <T> T
unwrap(Class<T> iface)
throws SQLException
{
   if ( trace.enabled() )
       trace.log(title + ".unwrap(" + iface + ")");
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
    {
	if( ! iface.isInstance( this ) )
	    throw SqlExFactory.get(ERR_GC4023_NO_OBJECT); 
	return( iface.cast(this) );
    }

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);
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

    if ( trace.enabled() )  
	trace.log( title + ".getBoolean( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getBoolean( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
        value = (rslt_val_proc == 0) ? false : true;
        null_param = false;
    }

    if ( trace.enabled() )  trace.log( title + ".getBoolean: " + value );
    return( value );
} // getBoolean


/*
** Name: getBoolean
**
** Description:
**	Retrieve parameter data as a boolean value.  If the parameter
**	is NULL, false is returned.
**
** Input:
**	parameterName	Parameter name
**
** Output:
**	None.
**
** Returns:
**	boolean		True if parameter data is boolean true, false otherwise.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
*/

public boolean
getBoolean( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getBoolean('" + parameterName + "')" );

    boolean value = resultRow.getBoolean( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

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

    if ( trace.enabled() )  
	trace.log( title + ".getByte( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getByte( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	value = (byte)rslt_val_proc;
	null_param = false;
    }

    if ( trace.enabled() )  trace.log( title + ".getByte: " + value );
    return( value );
} // getByte


/*
** Name: getByte
**
** Description:
**	Retrieve parameter data as a byte value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	byte		Parameter integer value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
*/

public byte
getByte( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getByte('" + parameterName + "')" );

    byte value = resultRow.getByte( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

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

    if ( trace.enabled() )  
	trace.log( title + ".getShort( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getShort( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	value = (short)rslt_val_proc;
	null_param = false;
    }

    if ( trace.enabled() )  trace.log( title + ".getShort: " + value );
    return( value );
} // getShort


/*
** Name: getShort
**
** Description:
**	Retrieve parameter data as a short value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	short		Parameter integer value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
*/

public short
getShort( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getShort('" + parameterName + "')" );

    short value = resultRow.getShort( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

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

    if ( trace.enabled() )  
	trace.log( title + ".getInt( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getInt( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	value = rslt_val_proc;
	null_param = false;
    }

    if ( trace.enabled() )  trace.log( title + ".getInt: " + value );
    return( value );
} // getInt


/*
** Name: getInt
**
** Description:
**	Retrieve parameter data as an int value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	int		Parameter integer value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
*/

public int
getInt( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getInt('" + parameterName + "')" );

    int value = resultRow.getInt( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

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

    if ( trace.enabled() )  
	trace.log( title + ".getLong( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getLong( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	value = rslt_val_proc;
	null_param = false;
    }

    if ( trace.enabled() )  trace.log( title + ".getLong: " + value );
    return( value );
} // getLong


/*
** Name: getLong
**
** Description:
**	Retrieve parameter data as a long value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	long		Parameter integer value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
*/

public long
getLong( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getLong('" + parameterName + "')" );

    long value = resultRow.getLong( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

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

    if ( trace.enabled() )  
	trace.log( title + ".getFloat( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getFloat( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	value = rslt_val_proc;
	null_param = false;
    }

    if ( trace.enabled() )  trace.log( title + ".getFloat: " + value );
    return( value );
} // getFloat


/*
** Name: getFloat
**
** Description:
**	Retrieve parameter data as a float value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	float		Parameter numeric value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
*/

public float
getFloat( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getFloat('" + parameterName + "')" );

    float value = resultRow.getFloat( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

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

    if ( trace.enabled() )  
	trace.log( title + ".getDouble( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getDouble( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	value = rslt_val_proc;
	null_param = false;
    }

    if ( trace.enabled() )  trace.log( title + ".getDouble: " + value );
    return( value );
} // getDouble


/*
** Name: getDouble
**
** Description:
**	Retrieve parameter data as a double value.  If the parameter
**	is NULL, zero is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	double		Parameter numeric value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
*/

public double
getDouble( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getDouble('" + parameterName + "')" );

    double value = resultRow.getDouble( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

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

    if ( trace.enabled() )  
	trace.log( title + ".getBigDecimal( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getBigDecimal( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	value = BigDecimal.valueOf( (long)rslt_val_proc );
	null_param = false;
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

    if ( trace.enabled() )  trace.log( title + ".getBigDecimal( " + 
				       paramIndex + ", " + scale + " )" );
    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getBigDecimal( resultMap( paramIndex ), scale );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	value = BigDecimal.valueOf( (long)rslt_val_proc );
	null_param = false;
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
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	BigDecimal	Parameter numeric value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
*/

public BigDecimal
getBigDecimal( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getBigDecimal('" + parameterName + "')" );

    BigDecimal value = resultRow.getBigDecimal( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

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

    if ( trace.enabled() )  
	trace.log( title + ".getString( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getString( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	value = Integer.toString( rslt_val_proc );
	if ( rs_max_len > 0  &&  value.length() > rs_max_len )
	    value = value.substring( 0, rs_max_len );
	null_param = false;
    }

    if ( trace.enabled() )  trace.log( title + ".getString: " + value );
    return( value );
} // getString


/*
** Name: getString
**
** Description:
**	Retrieve parameter data as a String value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	String		Parameter string value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
*/

public String
getString( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getString('" + parameterName + "')" );

    String value = resultRow.getString( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

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
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Clean up tracing of array parameter.
*/

public byte[]
getBytes( int paramIndex )
    throws SQLException
{
    byte    value[] = null;

    if ( trace.enabled() )  
	trace.log( title + ".getBytes( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getBytes( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    if ( trace.enabled() )  
	trace.log( title + ".getBytes: " + 
		   (value == null ? "null" : "[" + value.length + "]") );
    return( value );
} // getBytes


/*
** Name: getBytes
**
** Description:
**	Retrieve parameter data as a byte array value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	byte[]		Parameter binary array value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Clean up tracing of array parameter.
*/

public byte[]
getBytes( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getBytes('" + parameterName + "')" );

    byte value[] = resultRow.getBytes( resultMap( parameterName ) );
    null_param = resultRow.wasNull();

    if ( trace.enabled() )  
	trace.log( title + ".getBytes: " + 
		   (value == null ? "null" : "[" + value.length + "]") );
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
**	21-Jul-03 (gordy)
**	    Null calendars are handled by getTimestamp().
*/

public Date
getDate( int paramIndex, Calendar cal )
    throws SQLException
{
    Date    value = null;

    if ( trace.enabled() )  
	trace.log( title + ".getDate( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getDate( resultMap( paramIndex ), cal );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    if ( trace.enabled() )  trace.log( title + ".getDate: " + value );
    return( value );
} // getDate


/*
** Name: getDate
**
** Description:
**	Retrieve parameter data as a Date value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	parameterName	Parameter name.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Date		Parameter date value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
**	21-Jul-03 (gordy)
**	    Null calendars are handled by getTimestamp().
*/

public Date
getDate( String parameterName, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getDate('" + parameterName + "')" );

    Date value = resultRow.getDate( resultMap( parameterName ), cal );
    null_param = resultRow.wasNull();

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
**	21-Jul-03 (gordy)
**	    Null calendars are handled by getTimestamp().
*/

public Time
getTime( int paramIndex, Calendar cal )
    throws SQLException
{
    Time    value = null;

    if ( trace.enabled() )  
	trace.log( title + ".getTime( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getTime( resultMap( paramIndex ), cal );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    if ( trace.enabled() )  trace.log( title + ".getTime: " + value );
    return( value );
} // getTime


/*
** Name: getTime
**
** Description:
**	Retrieve parameter data as a Time value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	parameterName	Parameter name.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Time		Parameter time value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
**	21-Jul-03 (gordy)
**	    Null calendars are handled by getTimestamp().
*/

public Time
getTime( String parameterName, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getTime('" + parameterName + "')" );

    Time value = resultRow.getTime( resultMap( parameterName ), cal );
    null_param = resultRow.wasNull();

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
**	21-Jul-03 (gordy)
**	    Null calendars are handled by getTimestamp().
*/

public Timestamp
getTimestamp( int paramIndex, Calendar cal )
    throws SQLException
{
    Timestamp	value = null;

    if ( trace.enabled() )  
	trace.log( title + ".getTimestamp( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getTimestamp( resultMap( paramIndex ), cal );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    if ( trace.enabled() )  trace.log( title + ".getTimestamp: " + value );
    return( value );
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
**	parameterName	Parameter name.
**	cal		Calendar for timezone, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	Timestamp	Parameter timestamp value.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
**	21-Jul-03 (gordy)
**	    Null calendars are handled by getTimestamp().
*/

public Timestamp
getTimestamp( String parameterName, Calendar cal )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getTimestamp('" + parameterName + "')" );

    Timestamp value = resultRow.getTimestamp( resultMap(parameterName), cal );
    null_param = resultRow.wasNull();

    if ( trace.enabled() )  trace.log( title + ".getTimestamp: " + value );
    return( value );
} // getTimestamp


/*
** Name: getBlob
**
** Description:
**	Retrieve parameter data as a Blob value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Blob		Parameter blob value.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
*/

public Blob
getBlob( int paramIndex ) throws SQLException
{
    Blob value = null;

    if ( trace.enabled() )  
	trace.log( title + ".getBlob( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getBlob( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    if ( trace.enabled() )  trace.log( title + ".getBlob: " + value );
    return( value );
} // getBlob


/*
** Name: getBlob
**
** Description:
**	Retrieve parameter data as a Blob value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	Blob		Parameter blob value.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
*/

public Blob
getBlob( String parameterName ) throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getBlob('" + parameterName + "')" );

    Blob value = resultRow.getBlob( resultMap(parameterName) );
    null_param = resultRow.wasNull();

    if ( trace.enabled() )  trace.log( title + ".getBlob: " + value );
    return( value );
} // getBlob


/*
** Name: getClob
**
** Description:
**	Retrieve parameter data as a Clob value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Clob		Parameter clob value.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
*/

public Clob
getClob( int paramIndex ) throws SQLException
{
    Clob value = null;

    if ( trace.enabled() )  
	trace.log( title + ".getClob( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getClob( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    if ( trace.enabled() )  trace.log( title + ".getClob: " + value );
    return( value );
} // getClob


/*
** Name: getClob
**
** Description:
**	Retrieve parameter data as a Clob value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	Clob		Parameter clob value.
**
** History:
**	15-Nov-06 (gordy)
**	    Implemented.
*/

public Clob
getClob( String parameterName ) throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getClob('" + parameterName + "')" );

    Clob value = resultRow.getClob( resultMap(parameterName) );
    null_param = resultRow.wasNull();

    if ( trace.enabled() )  trace.log( title + ".getClob: " + value );
    return( value );
} // getClob

/*
** Name: getNClob
**
** Description:
**	Retrieve parameter data as a NClob value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	paramIndex	Parameter index.
**
** Output:
**	None.
**
** Returns:
**	NClob		Parameter nclob value.
**
** History:
**	 9-Apr-09 (rajus01) SIR 121238
**	    Implemented.
*/

public NClob
getNClob( int paramIndex ) throws SQLException
{
    NClob value = null;

    if ( trace.enabled() )  
	trace.log( title + ".getNClob( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getNClob( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    if ( trace.enabled() )  trace.log( title + ".getNClob: " + value );
    return( value );
} // getNClob

/*
** Name: getNClob
**
** Description:
**	Retrieve parameter data as a NClob value.  If the parameter
**	is NULL, null is returned.
**
** Input:
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	NClob		Parameter nclob value.
**
** History:
**      9-Apr-09 (rajus01) SIR 121238
**          Implemented.
*/
public NClob
getNClob( String parameterName ) throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getNClob('" + parameterName + "')" );

    NClob value = resultRow.getNClob( resultMap(parameterName) );
    null_param = resultRow.wasNull();

    if ( trace.enabled() )  trace.log( title + ".getNClob: " + value );
    return( value );
} // getNClob

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
**	 2-Jul-03 (gordy)
**	    Use helper method to coerce to registered output type.
**	    Procedure result must be handled separate from byref params.
**	 1-Nov-03 (gordy)
**	    A negative registered scale value means no scale.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Use more efficient value constructors.
*/

public Object
getObject( int paramIndex )
    throws SQLException
{
    Object obj = null;

    if ( trace.enabled() )  
	trace.log( title + ".getObject( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
	obj = getObject( resultRow, resultMap( paramIndex ) );
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
    {
	/*
	** Convert integer procedure result value to registered type.
	** The last entry in the registration info is reserved for the
	** procedure result.
	*/
	null_param = false;
    
	switch( opType[ opType.length - 1 ] )
	{
	case Types.BIT :
	case Types.BOOLEAN :
	    obj = Boolean.valueOf( rslt_val_proc != 0 );
	    break;
	
	case Types.TINYINT :
	case Types.SMALLINT :
	case Types.INTEGER :
	    obj = Integer.valueOf( rslt_val_proc );
	    break;
	
	case Types.BIGINT :
	    obj = Long.valueOf( (long)rslt_val_proc );
	    break;
	
	case Types.REAL :
	    obj = Float.valueOf( (float)rslt_val_proc );
	    break;
	
	case Types.FLOAT :
	case Types.DOUBLE :
	    obj = Double.valueOf( (double)rslt_val_proc );
	    break;
	
	case Types.DECIMAL :
	case Types.NUMERIC : 
	{
	    int scale = opScale[ opScale.length - 1 ];
	    
	    if ( scale < 0 )
		obj = BigDecimal.valueOf( (long)rslt_val_proc );
	    else
		obj = BigDecimal.valueOf( (long)rslt_val_proc, scale );
	    break;
	}
	
	case Types.CHAR :
	case Types.VARCHAR :
	case Types.LONGVARCHAR :	
	    obj = Integer.toString( rslt_val_proc );	
	    break;
    
	default :
	    	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
	}
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
**	parameterName	Parameter name.
**
** Output:
**	None.
**
** Returns:
**	Object		Java object.
**
** History:
**	28-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Don't rely on parameter names in result-set, map names
**	    using loaded/registered parameter names and result map.
**	    Use helper method to coerce to registered output type.
*/

public Object
getObject( String parameterName )
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getObject('" + parameterName + "')" );
    
    Object obj = getObject( resultRow, resultMap( parameterName ) );

    if ( trace.enabled() )  trace.log( title + ".getObject: " + obj );
    return( obj );
} // getObject


/*
** Name: getObject
**
** Description:
**	Retrieve parameter data as a generic Java object, mapping
**      to requested output type.  If the parameter is NULL, null 
**	is returned.
**
** Input:
**	results		ResultSet holding parameter data.
**	index		Parameter index.
**
** Output:
**	None.
**
** Returns:
**	Object		Java object.
**
** History:
**	 2-Jul-03 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    A negative registered scale value means no scale.
**	15-Nov-06 (gordy)
**	    Support BLOB/CLOB.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Use more efficient value constructors.
*/

private Object
getObject( ResultSet results, int index )
    throws SQLException
{
    Object	obj;
    int		param = unMap( index );
    
    switch( opType[ param ] )
    {
    case Types.BIT :
    case Types.BOOLEAN :
	obj = Boolean.valueOf( results.getBoolean( index ) );
	break;
	
    case Types.TINYINT :
    case Types.SMALLINT :
    case Types.INTEGER :
	obj = Integer.valueOf( results.getInt( index ) );
	break;
	
    case Types.BIGINT :
	obj = Long.valueOf( results.getLong( index ) );
	break;
	
    case Types.REAL :
	obj = Float.valueOf( results.getFloat( index ) );
	break;
	
    case Types.FLOAT :
    case Types.DOUBLE :
	obj = Double.valueOf( results.getDouble( index ) );
	break;
	
    case Types.DECIMAL :
    case Types.NUMERIC : 
    {
        int scale = opScale[ param ];
	    
        if ( scale < 0 )
    	    obj = results.getBigDecimal( index );
        else
    	    obj = results.getBigDecimal( index, scale );
        break;
    }
	
	
    case Types.CHAR :
    case Types.VARCHAR :
    case Types.LONGVARCHAR :	obj = results.getString( index );	break;
    
    case Types.BINARY :
    case Types.VARBINARY :
    case Types.LONGVARBINARY :	obj = results.getBytes( index );	break;
    
    case Types.DATE :		obj = results.getDate( index );		break;
    case Types.TIME :		obj = results.getTime( index );		break;
    case Types.TIMESTAMP :	obj = results.getTimestamp( index );	break;
	
    case Types.BLOB :		obj = results.getBlob( index );		break;
    case Types.CLOB :		obj = results.getClob( index );		break;

    default :
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );
    }
    
    null_param = results.wasNull();
    return( null_param ? null : obj );
} // getObject

/*
** Name: getCharaterStream
**
** Description:
**	Retrieve parameter data as a generic Java object.
**
** Input:
**	paramIndex		Parameter index.
**
** Output:
**	None.
**
** Returns:
**	java.io.Reader		Java object.
**
** History:
**	 3-Apr-09 (rajus01)
**	    Created.
*/
public Reader 
getCharacterStream(int paramIndex)
throws SQLException
{
    Reader value = null;

    if ( trace.enabled() )  
	trace.log( title + ".getCharacterStream( " + paramIndex + " )" );

    if ( ! isProcRslt( paramIndex ) )
    {
	value = resultRow.getCharacterStream( resultMap( paramIndex ) );
	null_param = resultRow.wasNull();
    }
    else  if ( (rslt_items & RSLT_PROC_VAL) == 0 )
	null_param = true;
    else
	throw SqlExFactory.get( ERR_GC401A_CONVERSION_ERR );

    if ( trace.enabled() )  trace.log( title + ".getCharacterStream: " + value );
    return( value );

} // getCharacterStream

/*
** Name: getCharaterStream
**
** Description:
**	Retrieve parameter data as a generic Java object.
**
** Input:
**	parameterName		Parameter Name.
**
** Output:
**	None.
**
** Returns:
**	java.io.Reader		Java object.
**
** History:
**	 3-Apr-09 (rajus01)
**	    Created.
*/
public Reader 
getCharacterStream(String parameterName)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getCharacterStream('" + parameterName + "')" );

    Reader value = resultRow.getCharacterStream( resultMap(parameterName) );
    null_param = resultRow.wasNull();

    if ( trace.enabled() )  trace.log( title + ".getCharacterStream: " + value );
    return( value );

} // getCharacterStream


/*
** Data access methods which are simple covers
** for the primary data access methods.
*/

public Date
getDate( int paramIndex ) throws SQLException
{ return( getDate( paramIndex, null ) ); }

public Date
getDate( String parameterName ) throws SQLException
{ return( getDate( parameterName, null ) ); }

public Time
getTime( int paramIndex ) throws SQLException
{ return( getTime( paramIndex, null ) ); }

public Time
getTime( String parameterName ) throws SQLException
{ return( getTime( parameterName, null ) ); }

public Timestamp
getTimestamp( int paramIndex ) throws SQLException
{ return( getTimestamp( paramIndex, null ) ); }

public Timestamp
getTimestamp( String parameterName ) throws SQLException
{ return( getTimestamp( parameterName, null ) ); }

public Object
getObject( int paramIndex, Map map ) throws SQLException
{ return( getObject( paramIndex ) ); }

public Object
getObject( String parameterName, Map map ) throws SQLException
{ return( getObject( parameterName ) ); }

public String 
getNString(int parameterIndex)
throws SQLException
{ return( getString(parameterIndex) ); }

public String 
getNString(String parameterName)
throws SQLException
{ return( getString(parameterName) ); }

public Reader 
getNCharacterStream(int parameterIndex)
throws SQLException
{ return( getCharacterStream(parameterIndex) ); }

public Reader 
getNCharacterStream(String parameterName)
throws SQLException
{ return( getCharacterStream(parameterName) ); }

/*
** Data access methods which are for unsupported types.
*/

public Array
getArray( int paramIndex ) throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getArray( " + paramIndex + " )" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getArray


public Array
getArray( String parameterName ) throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getArray('" + parameterName + "')" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getArray


public Ref
getRef( int paramIndex ) throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getRef( " + paramIndex + " )" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getRef


public Ref
getRef( String parameterName ) throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getRef('" + parameterName + "')" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getRef


public URL
getURL( int paramIndex ) throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getURL( " + paramIndex + " )" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getURL


public URL
getURL( String parameterName ) throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getURL('" + parameterName + "')" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getURL

public SQLXML 
getSQLXML(int paramIndex)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getSQLXML( " + paramIndex + " )" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getSQLXML

public SQLXML 
getSQLXML(String parameterName)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getSQLXML('" + parameterName + "')" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getSQLXML

public RowId 
getRowId(int index)
throws SQLException
{
    if ( trace.enabled() )  
        trace.log( title + ".getRowId( " + index + " )" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getRowId

public RowId 
getRowId(String parameterName)
throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getRowId('" + parameterName + "')" );
    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
} // getRowId

/*
** Name: isProcRslt
**
** Description:
**	Returns an indication that a dynamic parameter index
**	references the procedure result parameter.
**
** Input:
**	index	    Parameter index (1 based).
**
** Output:
**	None.
**
** Returns:
**	true	    True if reference to procedure result parameter.
**
** History:
**	 3-Aug-00 (Gordy)
**	    Created.
**      11-Jul-01 (loera01) SIR 105309
**          Added check for parameter at the end of dpMap for Ingres syntax.\
**	 4-Mar-03 (gordy)
**	    Renamed and simplified by saving procedure result index.
*/

private boolean
isProcRslt( int index )
{
    return( procRsltIndex > 0  &&  index == procRsltIndex );
} // isProcRslt


/*
** Name: checkParams
**
** Description:
**	Validates parameter set and makes sure the parameter info
**	is ready for execution.
**
** Input:
**	param_set	Parameter set to be validated.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 6-Jun-03 (gordy)
**	    Extracted from exec().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Added synchronization protection.
*/

private synchronized void
checkParams( ParamSet param_set )
    throws SQLException
{
    int param_cnt = procInfo.getParamCount();

    /*
    ** All non-default parameters must be set and ready to send.
    */
    for( int param = 0; param < param_cnt; param++ )
	if ( procInfo.getParamType( param ) != procInfo.PARAM_DEFAULT  &&
	     ! param_set.isSet( param ) )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": parameter not set: " + (param + 1) );
	    throw SqlExFactory.get( ERR_GC4020_NO_PARAM );
	}

    /*
    ** If non-dynamic parameters are present, parameter names were
    ** loaded in the constructor and assigned in initParameters().
    ** When all parameters are dynamic, the names may have been
    ** assigned in findParam() (access type NAME).  When ordinal
    ** indexes are used to access the parameters, it is possible
    ** that parameter names have not yet been loaded/assigned.
    */
    if ( access_type == ORDINAL  &&  ! procInfo.paramNamesLoaded() )
    {
	procInfo.loadParamNames();

	for( int param = 0; param < param_cnt; param++ )
	    param_set.setName( param, procInfo.getParamName( param ) );
    }

    return;
} // checkParams


/*
** Name: findParam
**
** Description:
**	Determines if a given parameter name is valid and maps 
**	the name to an internal index.  Accessible parameters 
**	are limited to those which were specified using dynamic 
**	parameter markers.
**
** Input:
**	name	    Parameter name.
**
** Output:
**	None.
**
** Returns:
**	int	    Internal parameter index [0,param_cnt - 1].
**
** History:
**	25-Feb-03 (gordy)
**	    Created.
**	 2-Jul-03 (gordy)
**	    Test if parameter name assigned rather than parameter value set.
**	    Use case insensitive comparison.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Added synchronization protection.
*/

private synchronized int
findParam( String name )
    throws SQLException
{
    if ( name == null )	 throw SqlExFactory.get( ERR_GC4012_INVALID_COLUMN_NAME );

    /*
    ** Access type must be consistent.
    */
    switch( access_type )
    {
    case NAME :	/* OK */		break;
    case NONE :	access_type = NAME;	break;

    case ORDINAL :
    default :
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".findParam(): mixed access by name & index" );
	throw SqlExFactory.get( ERR_GC4012_INVALID_COLUMN_NAME );
    }

    /*
    ** If parameter names have been loaded, then the names
    ** were assigned to the parameter set in initParameters().
    ** Otherwise, the name is either new (unassigned) or has
    ** been assigned by a previous call to this method.
    **
    ** Search the parameter set for previously assigned match.  
    ** If not found, assign to an unused dynamic parameter.
    */
    int unused = -1;

    for( int param = 0; param < dpMap.length; param++ )
	if ( dpMap[ param ] >= 0 )		// Dynamic parameter.
	{
	    int		index = dpMap[ param ];
	    String	paramName = params.getName( index );
	    
	    if ( paramName == null )		// Unassigned.
		unused = (unused < 0) ? index : unused;
	    else  if ( name.equalsIgnoreCase( paramName ) )
	    {
		if ( trace.enabled( 5 ) )
		    trace.write( tr_id + ".findParam(): matched '" + 
				 name + "' to index " + dpMap[ param ] );
		return( index );
	    }
	}

    /*
    ** Parameter name is not currently in the parameter set.
    ** Allocate a new dynamic parameter if there were any
    ** unused.  Otherwise, the name is either not a valid
    ** parameter name or too many dynamic names have been
    ** used.
    */
    if ( unused >= 0 )
    {
	if ( trace.enabled( 5 ) )  trace.write( tr_id + 
	    ".findParam(): assigned '" + name + "' to index " + unused );
	params.setName( unused, name );
	return( unused );
    }

    if ( trace.enabled( 1 ) )  trace.write( tr_id + 
	".findParam(): no unassigned dynamic parameters: '" + name + "'" );
    throw SqlExFactory.get( ERR_GC4012_INVALID_COLUMN_NAME );
} // findParam


/*
** Name: paramMap
**
** Description:
**	Determines if a given parameter index is valid
**	and maps the external index to the internal index.
**	Accessible parameters are limited to those which
**	were specified using dynamic parameter markers.
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
**	24-Feb-03 (gordy)
**	    Stored index values are now 0 based.  Access type
**	    must be consistent.
*/

protected synchronized int
paramMap( int index )
    throws SQLException
{
    /*
    ** Access type must be consistent.
    */
    switch( access_type )
    {
    case ORDINAL : /* OK */ break;
    case NONE :	access_type = ORDINAL;	break;

    case NAME :
    default :
	if ( trace.enabled( 1 ) )
	    trace.write( tr_id + ".paramMap(): mixed access by name & index" );
	throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );
    }

    /*
    ** Validate and map index.
    */
    index--;

    if ( index < 0  ||  index >= dpMap.length  ||  dpMap[ index ] < 0 )
	throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );

    return( dpMap[ index ] );
} // paramMap


/*
** Name: resultMap
**
** Description:
**	Determines if a given parameter index is valid
**	and maps the external index to the result index.
**
**	Accessible parameters are limited to those which
**	were specified using dynamic parameter markers
**	and were registered as output parameters.  These
**	parameters are mapped through the dynamic and
**	output mapping arrays to an external column index.
**
**	An exception will be thrown if this routine is
**	used to map the parameter for a procedure return
**	value as the dynamic parameter mapping does not
**	result in a valid index.
**
** Input:
**	index	    External parameter index [1,param_cnt].
**
** Output:
**	None.
**
** Returns:
**	int	    Result index [1,result_cnt].
**
** History:
**	 3-Aug-00 (gordy)
**	    Created.
**	 4-Mar-03 (gordy)
**	    Internal parameter indexes now 0 based.
**	 2-Jul-03 (gordy)
**	    Output map requires dynamic mapping first.
**	29-Jun-06 (gordy)
**	    Check for valid result data.
*/

private int
resultMap( int index )
    throws SQLException
{
    index--;

    if ( 
	 resultRow == null  ||  
	 index < 0  ||  
	 index >= dpMap.length  ||  
	 dpMap[ index ] < 0  ||  
	 opMap[ dpMap[ index ] ] < 0  ||
         opMap[ dpMap[ index ] ] > resultRow.getMetaData().getColumnCount()
       )
	throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );

    return( opMap[ dpMap[ index ] ] );
} // resultMap


/*
** Name: resultMap
**
** Description:
**	Determines if a given parameter name is valid
**	and can be mapped to a result index.
**
**	Accessible parameters are limited to those which
**	were specified using dynamic parameter markers
**	and were registered as output parameters.  These
**	parameters are mapped through the dynamic and
**	output mapping arrays to an external column index.
**
** Input:
**	name	    Parameter name.
**
** Output:
**	None.
**
** Returns:
**	int	    Result index [1,result_cnt].
**
** History:
**	 2-Jul-03 (gordy)
**	    Created.
**	29-Jun-06 (gordy)
**	    Check for valid result data.
*/

private int
resultMap( String name )
    throws SQLException
{
    if ( name == null  ||  resultRow == null )
	throw SqlExFactory.get( ERR_GC4012_INVALID_COLUMN_NAME );

    for( int param = 0; param < dpMap.length; param++ )
	if ( dpMap[ param ] >= 0 )		// Dynamic parameter.
	{
	    int		index = dpMap[ param ];
	    String	paramName = params.getName( index );
	    
	    if ( paramName != null  &&  name.equalsIgnoreCase( paramName ) )
	    {
		if ( 
		     opMap[ index ] < 0  ||	// Not registered.
		     opMap[ index ] > resultRow.getMetaData().getColumnCount()
		   )
		    throw SqlExFactory.get( ERR_GC4012_INVALID_COLUMN_NAME );
		return( opMap[ index ] );
	    }
	}
    
    /*
    ** Parameter name not in parameter set.
    */
    throw SqlExFactory.get( ERR_GC4012_INVALID_COLUMN_NAME );
}


/*
** Name: unMap
**
** Description:
**	Undoes the mapping of resultMap.  Given a result-set index,
**	returns the associated internal parameter index.
**
** Input:
**	index	Result-set index [1,result_cnt]
**
** Output:
**	None.
**
** Returns:
**	int	Internal parameter index [0,param_cnt-1]
**
** History:
**	 2-Jul-03 (gordy)
**	    Created.
*/

private int
unMap( int index )
    throws SQLException
{
    for( int i = 0; i < opMap.length; i++ )
	if ( index == opMap[ i ] )  return( i );
    throw SqlExFactory.get( ERR_GC4011_INDEX_RANGE );
} // unMap


/*
** Name: BatchExec
**
** Description:
**	Class which implements batch query execution.
**
**  Public Methods:
**
**	execute		Execute batch.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

protected static class
BatchExec
    extends JdbcStmt.BatchExec
{

/*
** Name: BatchExec
**
** Description:
**	Class constructor.
**
** Input:
**	conn	Database connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

public
BatchExec( DrvConn conn )
{ super( conn ); }


/*
** Name: execute
**
** Description:
**	Execute a prepared statement with a batch of parameter sets.
**	
**
** Input:
**	batch		List of parameter sets.
**	procInfo	Procedure information.
**
** Output:
**	None.
**
** Returns:
**	int []		Batch query results.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

public int []
execute( LinkedList batch, ProcInfo procInfo )
    throws SQLException
{
    if ( batch == null  ||  batch.peekFirst() == null )  return( noResults );

    clearResults();
    msg.lock();

    try
    {
	String		schema = procInfo.getSchema();
	String		pname = procInfo.getName();
	boolean		first = true;
	int		count = 0;
	ParamSet	ps;

	/*
	** Send each parameter set to batch server.
	*/
	while( (ps = (ParamSet)batch.pollFirst()) != null )
	{
	    int		paramCount = ps.getCount();
	    byte	hdrFlag = (batch.peekFirst() == null) ? MSG_HDR_EOG 
							      : MSG_HDR_EOB;

	    if ( trace.enabled() )
		trace.log( title + ".executeBatch[" + count + "]" );

	    count++;
	    msg.begin( MSG_BATCH );
	    msg.write( MSG_BAT_EXP );

	    if ( first )
	    {
		if ( schema != null )
		{
		    msg.write( MSG_BP_SCHEMA_NAME );
		    msg.write( schema );
		}

		msg.write( MSG_BP_PROC_NAME );
		msg.write( pname );
		first = false;
	    }
	    else
	    {
		msg.write( MSG_BP_FLAGS );
		msg.write( (short)2 );
		msg.write( MSG_BF_REPEAT );
	    }

	    if ( paramCount <= 0 )
		msg.done( hdrFlag );
	    else
	    {
		msg.done( false );
		ps.sendDesc( false );
		ps.sendData( hdrFlag );
	    }
	}

	/*
	** Read batch results returned by server.
	*/
	readResults( count );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".execute(): error executing batch" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace( ex, trace );
	throw ex;
    }
    finally
    {
	msg.unlock();
    }

    return( batchResults() );
} // execute


/*
** Name: getQueryResult
**
** Description:
**	Returns an integer result value extracted from the current
**	batch query set of result values.  If no suitable query
**	result is available, SUCCESS_NO_INFO is returned.
**
**	Returns the procedure return value if available, otherwise
**	the base class method is called to return the default value.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Batch query result.
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

protected int
getQueryResult()
{
    if ( (rslt_items & RSLT_PROC_VAL) != 0 )
    {
	/*
	** Consume and return the result value.
	*/
	rslt_items &= ~RSLT_PROC_VAL;
	return( rslt_val_proc );
    }

    return( super.getQueryResult() );
} // getQueryResult()


} // class BatchExec


} // class JdbcCall

