/*
** Copyright (c) 1999, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: advancall.cs
	**
	** Description:
	**	Defines class which implements the CallableStatement interface.
	**
	**  Classes:
	**
	**	AdvanCall
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
	**	    Added support for 2.0 extensions.
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
	**	    Updated to 3.0.
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
	**	25-Feb-04 (thoda04)
	**	    Convert SqlEx exception to an IngresException.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 1-Nov-04 (thoda04) B113360
	**	    Based procedure return value index relative to 0.
	**	 8-Feb-04 (thoda04) B113867
	**	    DB procedure Output and InputOutput parameter
	**	    values were not being returned.
	**	19-Jun-06 (gordy)
	**	    ANSI Date/Time data type support.
	**	28-Jun-06 (gordy)
	**	    OUT procedure parameter support.
	**	15-Sep-06 (gordy)
	**	    Support empty date strings using alternate storage format.
	*/



	/*
	** Name: AdvanCall
	**
	** Description:
	**	Driver class which implements the CallableStatement interface.
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
	**	extension for the syntax).  If any parameter names are used,
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
	**	Ingres requires all parameters to be named.  In 3.0 parameter
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
	**	setDecimal		Send parameter as a Decimal value.
	**	setString		Send parameter as a string value.
	**	setBytes		Send parameter as a byte array value.
	**	setDate			Send parameter as a Date value.
	**	setTime			Send parameter as a Time value.
	**	setTimestamp		Send parameter as a Timestamp value.
	**	setBinaryStream		Send parameter as a binary stream.
	**	setAsciiStream		Send parameter as an ASCII stream.
	**	setCharacterStream	Send parameter as a character stream.
	**	setObject		Send parameter as a generic object.
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
	**	getDecimal		Retrieve parameter as a Decimal value.
	**	getBytes		Retrieve parameter as a byte array value.
	**	getDate			Retrieve parameter as a Date value.
	**	getTime			Retrieve parameter as a Time value.
	**	getTimestamp		Retrieve parameter as a Timestamp value.
	**	getObject		Retrieve parameter as a generic object.
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
	**	    Support 2.0 extensions.  Added getBigDecimal(), getDate(),
	**	    getTime(), getTimestamp(), getObject(), getArray(), getBlob(),
	**	    getClob(), getRef(), registerOutParameter().
	**	11-Jan-01 (gordy)
	**	    Added support for batch processing.  Added addBatch() and
	**	    executeBatch().  Extracted exec() from execute().
	**      29-Aug-01 (loera01) SIR 105641
	**          Added executeUpdate and executeQuery methods that override their
	**          counterparts in advanprep.  Added detection of multiple-row
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
	*/

	class
		AdvanCall : AdvanPrep
	{

		/*
		** Constants
		*/
		private const int	NONE = 0;		// Param access mode
		private const int	ORDINAL = 1;
		private const int	NAME = 2;

		private static int[]	noMap = new int[0];	// Map for no params.

		private ProcInfo		procInfo;		// Parsing information
//		private JdbcQPMD procQPMD = null;	// Parameter meta-data
		private int procRsltIndex = -1;	// Result param index.
		private bool		hasByRefParam = false;	// BYREF parameters?
		private RsltByref		resultRow = null;	// Output parameters
		private bool		null_param = false;	// Last value null?
		private int			access_type = NONE;	// Param access mode.
		private int[]			dpMap = noMap;	// Dynamic param map
		private int[]			opMap = noMap;	// Output param map
		private ProviderType[]	opType = new ProviderType[0]; // Output param type
		private int[]			opScale = noMap;	// Output param scale


		/*
		** Name: AdvanCall
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
		**	    Changed parameters for 2.0 extensions.
		**	 8-Jan-01 (gordy)
		**	    Default parameter flags stored in ParamSet.
		**	28-Mar-01 (gordy)
		**	    ProcInfo now passed as parameter to parseCall()
		**	    rather than returned as result value.
		**      11-Jul-01 (loera01) SIR 105309
		**          Added check for Ingres DB proc syntax.
		**	31-Oct-01 (gordy)
		**	    Timezone now passed to advanSQL.
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
		**	 1-Nov-04 (thoda04) B113360
		**	    Based procedure return value index relative to 0.
		**	19-Jun-06 (gordy)
		**	    I give up... GMT indicator replaced with connection.
		*/

		public
			AdvanCall( DrvConn conn, String sql ) :
				// Initialize base class and override defaults.
					base(
						conn,
						DrvConst.TYPE_FORWARD_ONLY,
						DrvConst.DRV_CRSR_READONLY,
						DrvConst.CLOSE_CURSORS_AT_COMMIT )
		{
			title = trace.getTraceName() + "-CallableStatement[" + inst_id + "]";
			tr_id = "Call[" + inst_id + "]";

			/*
			** Flag all parameters as procedure parameters.
			*/
			paramSet.setDefaultFlags( ParamSet.PS_FLG_PROC_IN );
			bool native_proc = false;

			/*
			** Process the call procedure syntax.
			*/
			try
			{
				SqlParse parser = new SqlParse( sql, conn );
				procInfo = new ProcInfo( conn );
				parser.parseCall( procInfo );
				native_proc = (parser.getQueryType() == SqlParse.QT_NATIVE_PROC);
			}
			catch( SqlEx ex )
			{
				if ( trace.enabled() )  
					trace.log( title + ": error parsing query text" );
				if ( trace.enabled( 1 ) )  ex.trace( trace );
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
				** parameter last.  Escape syntax places it first.
				*/
				procRsltIndex = (native_proc ? dynamic-1 : 0);
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
			opType =  new ProviderType[ param_cnt + 1 ];	// Include procedure result
			opScale = new int[ param_cnt + 1 ];	// Include procedure result

			/*
			** Initialize the dynamic parameter mapping array 
			** and any static literal parameters.
			*/
			initParameters();
			return;
		} // AdvanCall


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
		*/

		private void
			initParameters()
		{
			lock(this)
			{
				bool	names_loaded = procInfo.paramNamesLoaded();
				int		param_cnt = procInfo.getParamCount();
				int		dynamic = 0;

				access_type = NONE;
				hasByRefParam = false;

				/*
				** Mark all parameters as unregistered (not output).
				*/
				for( int i = 0; i < opMap.Length; i++ )  opMap[ i ] = -1;

				/*
				** Ingres procedures only return an integer result, so we
				** don't require the procedure result parameter to be
				** registered.  If it is registered, the type/scale info
				** is saved in the last entry of registration arrays.
				** Initialize the default procedure result type/scale.
				*/
				opType[ opType.Length - 1 ] = ProviderType.Integer;
				opScale[opScale.Length - 1] = -1;	// No scale

				/*
				** Escape syntax places the procedure result parameter as
				** the first dynamic parameter.  Mark as unmapped if present.
				*/
				if ( isProcRslt( 0 ) )  dpMap[ dynamic++ ] = -1;

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
							paramSet.init(param, ProviderType.VarChar, true);
							paramSet.setObject( param, procInfo.getParamValue( param ), 
								ProviderType.VarChar );
							paramSet.setFlags( param, ParamSet.PS_FLG_PROC_GTT );
							break;

						default :			/* Literal input parameters */
						{
							Object	value = procInfo.getParamValue( param );
							paramSet.init( param, value );
							paramSet.setObject(param, value);
						}
							break;
					}
	
					if ( names_loaded )  
						paramSet.setName( param, procInfo.getParamName( param ) );
				}

				/*
				** Mark as unmapped any unused entries. This should 
				** only happen for native syntax where the procedure
				** result is the last dynamic parameter.
				*/
				for( ; dynamic < dpMap.Length; dynamic++ )  dpMap[ dynamic ] = -1;
				return;
			}
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
		**          Created from advanPrep.
		**	 6-Jun-03 (gordy)
		**	    Parameters must now be validated before calling exec().
		*/

		public new AdvanRslt
			executeQuery()
		{
			if ( trace.enabled() )  trace.log( title + ".executeQuery()" );
			if ( hasByRefParam )  throw SqlEx.get( ERR_GC4015_INVALID_OUT_PARAM );
			checkParams( paramSet );
			exec( paramSet );
			if ( resultSet == null ) throw SqlEx.get( ERR_GC4017_NO_RESULT_SET );
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
		**          Created from advanPrep.
		**	 6-Jun-03 (gordy)
		**	    Parameters must now be validated before calling exec().
		*/

		public new int
			executeUpdate()
		{
			if ( trace.enabled() )  trace.log( title + ".executeUpdate()" );
			checkParams( paramSet );
			exec( paramSet );
    
			/*
			** We don't want result sets to be returned with this method.
			** If present, clean up the tuple steam and thrown an exception.
			*/
			if ( resultSet != null )
			{
				try { resultSet.shut(); }
				catch( SqlEx ) {}
				resultSet = null;
				throw SqlEx.get( ERR_GC4018_RESULT_SET_NOT_PERMITTED );    
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

		public override bool
			execute()
		{
			if ( trace.enabled() )  trace.log( title + ".execute()" );
			checkParams( paramSet );
			exec( paramSet );
			bool isRS = (resultSet != null);
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

		internal override void
			addBatch()
		{
			if (trace.enabled()) trace.log(title + ".addBatch()");
			checkParams( paramSet );

			if ( batch == null )  newBatch();
			lock( batch ) 
			{
				if ( hasByRefParam )
				{
					batch.Add( null );
					paramSet.clear( false );
				}
				else
				{
					batch.Add( paramSet );
					paramSet = new ParamSet( conn );
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
		*/

		protected internal override int []
			executeBatch()
		{
			if ( trace.enabled() )  trace.log( title + ".executeBatch()" );
			if ( batch == null )  return( new int[0] );

			int[]	    results;
			bool savedByRef = hasByRefParam;
			hasByRefParam = false;

			lock( batch )
			{
				int count = batch.Count;
				results = new int[ count ];

				/*
				** Execute each individual query in the batch.
				*/
				for( int i = 0; i < count; i++ )
					try
					{
						ParamSet ps = (ParamSet)batch[0];
						batch.RemoveAt(0);
						if ( trace.enabled() )  
							trace.log( title + ".executeBatch[" + i + "] " );
		
						if (ps == null)
						{
							if ( trace.enabled() )  
							{
								trace.log( title + 
									".executeBatch(): OUT parameters not allowed" );
							}
							throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
						}
		
						exec( ps );
						results[ i ] = rslt_proc_val ? rslt_val_proc 
							: SUCCESS_NO_INFO;

						if ( trace.enabled() )  
							trace.log( title + ".executeBatch[" + i + "] = " 
								+ results[ i ] );
					}
					catch (SqlEx /* ex */)
					{
						/*
						** Return only the successful query results.
						*/
						int[] temp = new int[i];
						if (i > 0)
							Array.Copy(results, 0, temp, 0, i);

						batch.Clear();
						throw;
					}
					catch (System.Exception /* ex */)
					{
						// Shouldn't happen
						/*
						** Return only the successful query results.
						*/
						int[] temp = new int[i];
						if (i > 0)
							Array.Copy(results, 0, temp, 0, i);
						results = temp;
						throw;
					}
	
				batch.Clear();
			}

			hasByRefParam = savedByRef;
			return( results );
		} // executeBatch


		/*
		** Name: exec
		**
		** Description:
		**	Request server to execute a Database Procedure.
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
		**          were not specified, use advanStmt's resultSet rather than the
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
		*/

		private void
			exec( ParamSet param_set )
		{
			clearResults();
			msg.LockConnection();
			try
			{
				String	    schema = procInfo.getSchema();
				bool	    needEOG = true;
				AdvanRSMD    rsmd;

				msg.begin( MSG_QUERY );
				msg.write( MSG_QRY_EXP );
	
				if ( conn.msg_protocol_level >= MSG_PROTO_3 )
				{
					msg.write( MSG_QP_FLAGS );
					msg.write( (short)4 );
					msg.write( MSG_QF_FETCH_FIRST | MSG_QF_AUTO_CLOSE );
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
					param_set.sendDesc(false);
					param_set.sendData(true);
				}

				if ( (rsmd = readResults( timeout, needEOG )) == null )
				{
					/*
					** Procedure execute has completed and all results have
					** been returned.  Unlock the connection.
					*/
					if ( msg.moreMessages() )  readResults( timeout, true );
					msg.UnlockConnection(); 
				}
				else  if ( hasByRefParam )
				{
					/*
					** BYREF parameters are treated as a result-set with 
					** exactly one row and processed as a tuple stream.  
					** The BYREF parameter result-set class loads the single
					** row, shuts the statement and unlocks the connection.
					*/
					resultRow = new RsltByref( conn, this, rsmd, 
						rslt_val_stmt, msg.moreMessages() );

					/*
					** Map output parameters to the output result-set.
					*/
					for( int i = 0, index = 0; i < opMap.Length; i++ )
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
			catch( SqlEx ex )
			{
				if ( trace.enabled() )  
					trace.log( title + ".execute(): error executing procedure" );
				if ( trace.enabled( 1 ) )  ex.trace( trace );
				msg.UnlockConnection(); 
				throw ex;
			}

			return;
		} // exec

#if NOT_IMPLEMENTED_YET
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
				procQPMD = new advanQPMD( conn, procInfo.getSchema(), 
							   procInfo.getName() );
			}
			catch( SqlEx ex )
			{
				if ( trace.enabled() )  
				trace.log( title + ": error retrieving parameter info" );
				if ( trace.enabled( 1 ) )  ex.trace( trace );
				throw ex;
			}

			if ( trace.enabled() )  
			trace.log( title + ".getParameterMetaData(): " + procQPMD );

			return( procQPMD );
		} // getParameterMetaData
#endif

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

		protected internal override void
			clearResults()
		{
			lock(this)
			{
				null_param = false;
				base.clearResults();

				if ( resultRow != null )
				{
					try { resultRow.shut(); }
					catch( SqlEx ) {}
					resultRow = null;
							
				}
				return;
			}
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

		public new void
			clearParameters()
		{
			if ( trace.enabled() )  trace.log( title + ".clearParameters()" );
			base.clearParameters();
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
			registerOutParameter( int paramIndex, ProviderType sqlType, int scale )
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
				opType[ opType.Length - 1 ] = sqlType;
				opScale[ opScale.Length - 1 ] = scale;
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
				opMap[index] = 0;
				opType[index] = sqlType;
				opScale[index] = scale;
				hasByRefParam = true;

				/*
				** Output parameters require at least a generic NULL
				** input value.  Normally, calling setNull() will
				** flag the parameter as an IN parameter, so the
				** default flags need to be manipulated to provide
				** a value for the output parameter while avoiding
				** marking the parameter as IN.
				*/
				if ( ! paramSet.isSet( index ) )  
				{
					paramSet.setDefaultFlags( (ushort)0 );
					paramSet.init( index, ProviderType.DBNull );
					paramSet.setNull( index );
					paramSet.setDefaultFlags( ParamSet.PS_FLG_PROC_IN );
				}
				
				/*
				** Finally, flag parameter is an output parameter.
				*/
				paramSet.setFlags(index, ParamSet.PS_FLG_PROC_OUT);
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
			registerOutParameter( String parameterName, ProviderType sqlType, int scale )
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
			opMap[index] = 0;
			opType[index] = sqlType;
			opScale[index] = scale;
			hasByRefParam = true;

			/*
			** Output parameters require at least a generic NULL
			** input value.  Normally, calling setNull() will
			** flag the parameter as an IN parameter, so the
			** default flags need to be manipulated to provide
			** a value for the output parameter while avoiding
			** marking the parameter as IN.
			*/
			if ( ! paramSet.isSet( index ) ) 
			{
			paramSet.setDefaultFlags( (ushort)0 );
			paramSet.init( index, ProviderType.DBNull );
			paramSet.setNull( index );
			paramSet.setDefaultFlags( ParamSet.PS_FLG_PROC_IN );
			}
		    
			/*
			** Finally, flag parameter is an output parameter.
			*/
			paramSet.setFlags(index, ParamSet.PS_FLG_PROC_OUT);
			return;
		} // registerOutParameter


		/*
		** Register methods which are simple covers
		** for the primary register methods.
		*/
		public void
			registerOutParameter( int index, ProviderType type )
		{ registerOutParameter(index, type, -1); }

		public void
			registerOutParameter( String name, ProviderType type )
		{ registerOutParameter(name, type, -1); }

#if IsNotNeededForDotNET
		public void
		registerOutParameter( int index, int type, String tn )
		{ registerOutParameter( index, type, -1 ); }

		public void
			registerOutParameter( String name, ProviderType type, String tn )
		{ registerOutParameter( name, type, -1); }
#endif


		/*
		** Name: setNull
		**
		** Description:
		**	Set parameter to be a NULL value.  The parameter is assigned
		**	a NULL value of the requested SQL type.
		**
		** Input:
		**	parameterName	Parameter name.
		**	sqlType		Column SQL type (sql.Types).
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
			setNull( String parameterName, ProviderType sqlType, String typeName )
		{
			if (trace.enabled())
				trace.log(title + ".setNull('" + parameterName + "'," + sqlType +
					   ((typeName == null) ? ")" : ",'" + typeName + "')"));

			int paramIndex = findParam( parameterName );
			paramSet.init( paramIndex, sqlType );
			paramSet.setNull(paramIndex);
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
			setBoolean( String parameterName, bool value )
		{
			if ( trace.enabled() )  trace.log( title + ".setBoolean('" + 
										parameterName + "', " + value + " )" );

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.Boolean);
			paramSet.setBoolean(paramIndex, value);
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
		{
			if (trace.enabled())
				trace.log(title + ".setByte('" + parameterName + "'," + value + ")");

			int paramIndex = findParam(parameterName);
			paramSet.init(paramIndex, ProviderType.TinyInt);
			paramSet.setByte(paramIndex, value);
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
		{
			if (trace.enabled())
				trace.log(title + ".setShort('" + parameterName + "'," + value + ")");

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.SmallInt);
			paramSet.setShort(paramIndex, value);
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
		{
			if (trace.enabled())
				trace.log(title + " setInt('" + parameterName + "'," + value + ")");

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.Integer);
			paramSet.setInt(paramIndex, value);
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
		{
			if (trace.enabled())
				trace.log(title + ".setLong('" + parameterName + "'," + value + ")");

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.BigInt);
			paramSet.setLong(paramIndex, value);
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
		{
			if (trace.enabled())
				trace.log(title + ".setFloat('" + parameterName + "'," + value + ")");

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.Real);
			paramSet.setFloat(paramIndex, value);
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
		{
			if (trace.enabled())
				trace.log(title + ".setDouble('" + parameterName + "'," + value + ")");

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.Double);
			paramSet.setDouble(paramIndex, value);
			return;
		} // setDouble


		/*
		** Name: setDecimal
		**
		** Description:
		**	Set parameter to a Decimal value.
		**
		** Input:
		**	parameterName	Parameter name.
		**	value		Decimal value.
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
			setDecimal( String parameterName, Decimal value )
		{
			if (trace.enabled()) trace.log(title + ".setDecimal('" +
							parameterName + "'," + value + ")");

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.Decimal);
			paramSet.setDecimal(paramIndex, value);
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
		{
			if (trace.enabled())
				trace.log(title + ".setString('" + parameterName + "'," + value + ")");

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.VarChar);
			paramSet.setString(paramIndex, value);
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
		*/

		public void
			setBytes( String parameterName, byte[] value )
		{
			if (trace.enabled())
				trace.log(title + ".setBytes('" + parameterName + "'," + value + ")");

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.VarBinary);
			paramSet.setBytes(paramIndex, value);
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
			setDate( String parameterName, DateTime value, TimeZone tz )
		{
			if ( trace.enabled() )  
			trace.log( title + ".setDate('" + parameterName + "'," + value + 
				   ((tz == null) ? " )" : ", " + tz.StandardName + " )") );

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.Date);
			paramSet.setDate( paramIndex, value, tz );
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
			setTime( String parameterName, DateTime value, TimeZone tz )
		{
			if (trace.enabled())
				trace.log(title + ".setTime('" + parameterName + "'," + value +
					   ((tz == null) ? " )" : ", " + tz.StandardName + " )"));

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.Time);
			paramSet.setTime(paramIndex, value, tz );
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
			setTimestamp( String parameterName, DateTime value, TimeZone tz )
		{
			if (trace.enabled())
				trace.log(title + ".setTimestamp('" + parameterName + "'," + value +
					   ((tz == null) ? " )" : ", " + tz.StandardName + " )"));

			int paramIndex = findParam( parameterName );
			paramSet.init(paramIndex, ProviderType.DateTime);
			paramSet.setTimestamp(paramIndex, value, tz );
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
		*/

		public void
			setBinaryStream( String parameterName, InputStream stream, int length )
		{
			if ( trace.enabled() )  trace.log( title + ".setBinaryStream('" + 
										parameterName + "', " + length + " )" );

			int paramIndex = findParam( parameterName );
			paramSet.init( paramIndex, ProviderType.LongVarBinary );
			paramSet.setBinaryStream(paramIndex, stream);
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
		*/

		public void
			setAsciiStream( String parameterName, InputStream stream, int length )
		{
			if ( trace.enabled() )  trace.log( title + ".setAsciiStream('" + 
										parameterName + "', " + length + " )" );

			int paramIndex = findParam(parameterName);
			paramSet.init(paramIndex, ProviderType.LongVarChar);
			paramSet.setAsciiStream(paramIndex, stream);
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
		*/

		public void
			setCharacterStream( String parameterName, Reader reader, int length )
		{
			if ( trace.enabled() )  trace.log( title + ".setCharacterStream('" + 
				parameterName + "', " + length + " )" );

			int paramIndex = findParam(parameterName);
			paramSet.init(paramIndex, ProviderType.LongVarChar);
			paramSet.setCharacterStream(paramIndex, reader);
			return;
		} // setCharacterStream


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
		*/

		public void
		setObject( String parameterName, Object value )
		{ 
			if ( trace.enabled() )  
			trace.log( title + ".setObject('" + parameterName + " )" );

			int paramIndex = findParam( parameterName );
			paramSet.init( paramIndex, value );
			paramSet.setObject(paramIndex, value);
			return;
		}


		/*
		** Name: setObject
		**
		** Description:
		**	Set parameter to a generic object with the requested
		**	SQL type and scale.  A scale value of -1 indicates that the
		**	scale of the value should be used.
		**
		**	Special handling of certain objects may be requested
		**	by setting sqlType to OTHER.  If the object does not have
		**	a processing alternative, the value will processed as if
		**	setObject() with no SQL type was called (except that the
		**	scale value provided will be used for DECIMAL values).
		**
		** Input:
		**	parameterName	Parameter name.
		**	value		object.
		**	sqlType		Target SQL type.
		**	scale		Number of digits following decimal point, or -1.
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
		**	28-Apr-04 (wansh01)
		**	    Passed input sqlType to setObject().
		**	    For Types.OTHER, type of the object will be used for setObject().
		**	15-Sep-06 (gordy)
		**	    Support empty date strings.
		*/

		public void
			setObject(
				String parameterName, Object value, ProviderType sqlType, int scale )
		{
			if (trace.enabled()) trace.log(title + ".setObject('" + parameterName +
				"'," + sqlType + "," + scale + ")");

			int paramIndex = findParam( parameterName );

			if (sqlType == ProviderType.Other)
			{
				/*
				** Force alternate storage format.
				*/
				paramSet.init(paramIndex, value, true);
				sqlType = SqlData.getSqlType( value ); 
			}
			else
			{
				switch( sqlType )
				{
				case ProviderType.Date :
				case ProviderType.Time:
				case ProviderType.DateTime:
					/*
					** Check for an empty date string.
					*/
					if ( value is String  &&  ((String)value).Length == 0 )
					{
					/*
					** Empty dates are only supported by Ingres dates
					** which require the alternate storage format.
					*/
					paramSet.init( paramIndex, sqlType, true );
					break;
					}

					/*
					** Fall through for default behaviour.
					*/
					goto default;

				default :
					paramSet.init( paramIndex, sqlType );
					break;
				}
			}

			paramSet.setObject( paramIndex, value, sqlType, scale, 0 );
			return;
		}


		/*
		** Data save methods which are simple covers 
		** for the primary data save methods.
		*/

		public void 
		setNull( String parameterName, ProviderType type )
		{ setNull( parameterName, type, null ); }

		public void 
		setDate( String parameterName, DateTime value )
		{ setDate( parameterName, value, null ); }

		public void
			setTime(String parameterName, DateTime value)
		{ setTime( parameterName, value, null ); }

		public void
		setTimestamp(String parameterName, DateTime value)
		{ setTimestamp( parameterName, value, null ); }

		public void 
		setObject( String parameterName, Object value, ProviderType type )
		{ setObject( parameterName, value, type, 0 ); }


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

		public bool
			wasNull()
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

		public bool
			getBoolean( int paramIndex )
		{
			bool value = false;

			if ( trace.enabled() )  
				trace.log( title + ".getBoolean( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getBoolean( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
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

		public bool
			getBoolean( String parameterName )
		{
			if ( trace.enabled() )  
				trace.log( title + ".getBoolean('" + parameterName + "')" );

			bool value = resultRow.getBoolean( resultMap( parameterName ) );
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
		{
			byte    value = 0;

			if ( trace.enabled() )  
				trace.log( title + ".getByte( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getByte( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
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
		{
			short   value = 0;

			if ( trace.enabled() )  
				trace.log( title + ".getShort( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getShort( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
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
		{
			int	    value = 0;

			if ( trace.enabled() )  
				trace.log( title + ".getInt( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getInt( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
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
		{
			long    value = 0;

			if ( trace.enabled() )  
				trace.log( title + ".getLong( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getLong( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
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
		{
			float   value = 0.0F;

			if ( trace.enabled() )  
				trace.log( title + ".getFloat( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getFloat( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
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
		{
			double  value = 0.0;

			if ( trace.enabled() )  
				trace.log( title + ".getDouble( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getDouble( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
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
		{
			if ( trace.enabled() )  
				trace.log( title + ".getDouble('" + parameterName + "')" );

			double value = resultRow.getDouble( resultMap( parameterName ) );
			null_param = resultRow.wasNull();

			if ( trace.enabled() )  trace.log( title + ".getDouble: " + value );
			return( value );
		} // getDouble


		/*
		** Name: getDecimal
		**
		** Description:
		**	Retrieve parameter data as a Decimal value.  If the parameter
		**	is NULL, null is returned.
		**
		** Input:
		**	paramIndex	Parameter index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Decimal	Parameter numeric value.
		**
		** History:
		**	 2-Nov-00 (gordy)
		**	    Created.
		*/

		public Decimal
			getDecimal( int paramIndex )
		{
			Decimal	value = Decimal.MinValue;

			if ( trace.enabled() )  
				trace.log( title + ".getDecimal( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getDecimal( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
				null_param = true;
			else
			{
				value = Convert.ToDecimal( rslt_val_proc );
				null_param = false;
			}

			if ( trace.enabled() )  trace.log( title + ".getDecimal: " + value );
			return( value );
		} // getDecimal


		/*
		** Name: getDecimal
		**
		** Description:
		**	Retrieve parameter data as a Decimal value.  If the parameter
		**	is NULL, null is returned.
		**
		** Input:
		**	parameterName	Parameter name.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Decimal	Parameter numeric value.
		**
		** History:
		**	28-Feb-03 (gordy)
		**	    Created.
		**	 2-Jul-03 (gordy)
		**	    Don't rely on parameter names in result-set, map names
		**	    using loaded/registered parameter names and result map.
		*/

		public Decimal
			getDecimal( String parameterName )
		{
			if ( trace.enabled() )  
				trace.log( title + ".getDecimal('" + parameterName + "')" );

			Decimal value = resultRow.getDecimal( resultMap( parameterName ) );
			null_param = resultRow.wasNull();

			if ( trace.enabled() )  trace.log( title + ".getDecimal: " + value );
			return( value );
		} // getDecimal


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
		{
			String  value = null;

			if ( trace.enabled() )  
				trace.log( title + ".getString( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getString( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
				null_param = true;
			else
			{
				value = rslt_val_proc.ToString();
				if ( rs_max_len > 0  &&  value.Length > rs_max_len )
					value = value.Substring( 0, rs_max_len );
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
		*/

		public byte[]
			getBytes( int paramIndex )
		{
			byte[]    value = null;

			if ( trace.enabled() )  
				trace.log( title + ".getBytes( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getBytes( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
				null_param = true;
			else
				throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );

			if ( trace.enabled() )  trace.log( title + ".getBytes: " + value );
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
		*/

		public byte[]
			getBytes( String parameterName )
		{
			if ( trace.enabled() )  
				trace.log( title + ".getBytes('" + parameterName + "')" );

			byte[] value = resultRow.getBytes( resultMap( parameterName ) );
			null_param = resultRow.wasNull();

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

		public DateTime
			getDate( int paramIndex, TimeZone tz )
		{
			DateTime value = DateTime.MinValue;

			if ( trace.enabled() )  
				trace.log( title + ".getDate( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getDate(resultMap(paramIndex) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
				null_param = true;
			else
				throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );

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

		public DateTime
			getDate( String parameterName, TimeZone tz )
		{
			if ( trace.enabled() )  
				trace.log( title + ".getDate('" + parameterName + "')" );

			DateTime value = resultRow.getDate( resultMap( parameterName ) );
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

		public DateTime
			getTime( int paramIndex, TimeZone tz )
		{
			DateTime    value = DateTime.MinValue;

			if ( trace.enabled() )  
				trace.log( title + ".getTime( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getTime( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
				null_param = true;
			else
				throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );

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

		public DateTime
			getTime( String parameterName, TimeZone tz )
		{
			if ( trace.enabled() )  
				trace.log( title + ".getTime('" + parameterName + "')" );

			DateTime value = resultRow.getTime( resultMap( parameterName ) );
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

		public DateTime
			getTimestamp( int paramIndex, TimeZone tz )
		{
			DateTime	value = DateTime.MinValue;

			if ( trace.enabled() )  
				trace.log( title + ".getTimestamp( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
			{
				value = resultRow.getTimestamp( resultMap( paramIndex ) );
				null_param = resultRow.wasNull();
			}
			else  if ( ! rslt_proc_val )
				null_param = true;
			else
				throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );

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

		public DateTime
			getTimestamp(String parameterName, TimeZone tz)
		{
			if ( trace.enabled() )  
				trace.log( title + ".getTimestamp('" + parameterName + "')" );

			DateTime value = resultRow.getTimestamp( resultMap(parameterName) );
			null_param = resultRow.wasNull();

			if ( trace.enabled() )  trace.log( title + ".getTimestamp: " + value );
			return( value );
		} // getTimestamp


		/*
		** Name: getObject
		**
		** Description:
		**	Retrieve parameter data as a generic object.  If the parameter
		**	is NULL, null is returned.
		**
		** Input:
		**	paramIndex	Parameter index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Object		object.
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
		*/

		public Object
			getObject( int paramIndex )
		{
			Object obj = null;

			if ( trace.enabled() )  
				trace.log( title + ".getObject( " + paramIndex + " )" );

			if ( ! isProcRslt( paramIndex ) )
				obj = getObject( resultRow, resultMap( paramIndex ) );
			else  if ( ! rslt_proc_val )
				null_param = true;
			else
			{
				/*
				** Convert integer procedure result value to registered type.
				** The last entry in the registration info is reserved for the
				** procedure result.
				*/
				null_param = false;
    
				switch( opType[ opType.Length - 1 ] )
				{
					case ProviderType.Boolean :
						obj = (rslt_val_proc == 0) ? false : true ;
						break;
	
					case ProviderType.TinyInt :
						obj = Convert.ToSByte(rslt_val_proc);
						break;

					case ProviderType.SmallInt :
						obj = Convert.ToInt16(rslt_val_proc);
						break;

					case ProviderType.Integer :
						obj = Convert.ToInt32(rslt_val_proc);
						break;

					case ProviderType.BigInt :
						obj = Convert.ToInt64(rslt_val_proc);
						break;
	
					case ProviderType.Real :
						obj = Convert.ToSingle( rslt_val_proc );
						break;
	
					case ProviderType.Double :
						obj = Convert.ToDouble( rslt_val_proc );
						break;
	
					case ProviderType.Decimal :
					case ProviderType.Numeric : 
						obj = Convert.ToDecimal( rslt_val_proc );
						break;
	
					case ProviderType.Char :
					case ProviderType.VarChar :
					case ProviderType.LongVarChar :	
						obj = rslt_val_proc.ToString();	
						break;

					default :
						throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );
				}
			}

			if ( trace.enabled() )  trace.log( title + ".getObject: " + obj );
			return( obj );
		} // getObject


		/*
		** Name: getObject
		**
		** Description:
		**	Retrieve parameter data as a generic object.  If the parameter
		**	is NULL, null is returned.
		**
		** Input:
		**	parameterName	Parameter name.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Object		object.
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
		**	Retrieve parameter data as a generic object, mapping
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
		**	Object		object.
		**
		** History:
		**	 2-Jul-03 (gordy)
		**	    Created.
		**	 1-Nov-03 (gordy)
		**	    A negative registered scale value means no scale.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		private Object
			getObject( AdvanRslt results, int index )
		{
			Object	obj;
			int		param = unMap( index );

			switch( opType[ param ] )
			{
				case ProviderType.Boolean :
					obj = results.getBoolean( index );
					break;
	
				case ProviderType.TinyInt :
				case ProviderType.SmallInt :
				case ProviderType.Integer :
					obj = results.getInt( index );
					break;
	
				case ProviderType.BigInt :
					obj = results.getLong( index );
					break;
	
				case ProviderType.Real :
					obj = results.getFloat( index );
					break;
	
				case ProviderType.Double :
					obj = results.getDouble( index );
					break;
	
				case ProviderType.Decimal :
				case ProviderType.Numeric : 
					obj = results.getDecimal( index, opScale[ param ] );
					break;
	
				case ProviderType.Char :
				case ProviderType.VarChar :
				case ProviderType.LongVarChar :
				case ProviderType.NChar :
				case ProviderType.NVarChar :
				case ProviderType.LongNVarChar :
					obj = results.getString( index );	break;

				case ProviderType.Binary :
				case ProviderType.VarBinary :
				case ProviderType.LongVarBinary :
					obj = results.getBytes( index );	break;

				case ProviderType.Date:      obj = results.getDate(index); break;
				case ProviderType.Time:      obj = results.getTime(index); break;
				case ProviderType.IngresDate:
				case ProviderType.DateTime: obj = results.getTimestamp(index); break;

				default :
					throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );
			}

			null_param = results.wasNull();
			return( null_param ? null : obj );
		} // getObject


		/*
		** Data access methods which are simple covers
		** for the primary data access methods.
		*/

		public DateTime
		getDate( int paramIndex )
		{ return( getDate( paramIndex, null ) ); }

		public DateTime
		getDate( String parameterName )
		{ return( getDate( parameterName, null ) ); }

		public DateTime
		getTime( int paramIndex )
		{ return( getTime( paramIndex, null ) ); }

		public DateTime
		getTime( String parameterName )
		{ return( getTime( parameterName, null ) ); }

		public DateTime
		getTimestamp( int paramIndex )
		{ return( getTimestamp( paramIndex, null ) ); }

		public DateTime
		getTimestamp( String parameterName )
		{ return( getTimestamp( parameterName, null ) ); }

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
		**	 1-Nov-04 (thoda04) B113360
		**	    Based procedure return value index relative to 0.
		*/

		private bool
			isProcRslt( int index )
		{
			return( procRsltIndex >= 0  &&  index == procRsltIndex );
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
		*/

		private void
			checkParams( ParamSet param_set )
		{
			int param_cnt = procInfo.getParamCount();

			/*
			** All non-default parameters must be set and ready to send.
			*/
			for( int param = 0; param < param_cnt; param++ )
				if ( procInfo.getParamType( param ) != ProcInfo.PARAM_DEFAULT  &&
					! param_set.isSet( param ) )
				{
					if ( trace.enabled( 1 ) )
						trace.write( tr_id + ": parameter not set: " + (param + 1) );
					throw SqlEx.get( ERR_GC4020_NO_PARAM );
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
		*/

		private int
			findParam( String name )
		{
			if ( name == null )	 throw SqlEx.get( ERR_GC4012_INVALID_COLUMN_NAME );

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
					throw SqlEx.get( ERR_GC4012_INVALID_COLUMN_NAME );
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

			for( int param = 0; param < dpMap.Length; param++ )
				if ( dpMap[ param ] >= 0 )		// Dynamic parameter.
				{
					int		index = dpMap[ param ];
					String	paramName = paramSet.getName( index );

					if ( paramName == null )		// Unassigned.
						unused = (unused < 0) ? index : unused;
					else  if ( ToInvariantUpper(name).Equals(
						       ToInvariantUpper(paramName) ) )
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
				paramSet.setName( unused, name );
				return( unused );
			}

			if ( trace.enabled( 1 ) )  trace.write( tr_id + 
										   ".findParam(): no unassigned dynamic parameters: '" + name + "'" );
			throw SqlEx.get( ERR_GC4012_INVALID_COLUMN_NAME );
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
		**	index	    External parameter index [0,param_cnt - 1].
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

		protected internal override int
			paramMap( int index )
		{
			lock(this)
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
							trace.write( tr_id +
							".paramMap(): mixed access by name & index" );
						throw SqlEx.get( ERR_GC4011_INDEX_RANGE );
				}

				/*
				** Validate and map index.
				*/
				if ( index < 0  ||  index > dpMap.Length  ||  dpMap[ index ] < 0 )
					throw SqlEx.get( ERR_GC4011_INDEX_RANGE );

				return( dpMap[ index ] );
			}
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
		**	index	    External parameter index [0,param_cnt-1].
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
		**	 3-Feb-05 (thoda04)
		**	    External parameter indexes 0 based in .NET.
		**	29-Jun-06 (gordy)
		**	    Check for valid result data.
		*/

		private int
			resultMap( int index )
		{
			if (
				resultRow == null  ||
				index < 0  ||
				index >= dpMap.Length  ||
				dpMap[ index ] < 0  ||
				opMap[ dpMap[ index ] ] < 0  ||
				opMap[ dpMap[ index ] ] > resultRow.getMetaData().getColumnCount()
			)
				throw SqlEx.get( ERR_GC4011_INDEX_RANGE );

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
		{
			if ( name == null  ||  resultRow == null )
				throw SqlEx.get( ERR_GC4012_INVALID_COLUMN_NAME );

			for( int param = 0; param < dpMap.Length; param++ )
				if ( dpMap[ param ] >= 0 )		// Dynamic parameter.
				{
					int		index = dpMap[ param ];
					String	paramName = paramSet.getName( index );

					if ( paramName != null  &&
						ToInvariantUpper(name).Equals(
						ToInvariantUpper(paramName) ) )
					{
						if (
							 opMap[index] < 0 ||	// Not registered.
							 opMap[index] > resultRow.getMetaData().getColumnCount()
						   )
							throw SqlEx.get(ERR_GC4012_INVALID_COLUMN_NAME);
						return( opMap[ index ] );
					}
				}

			/*
			** Parameter name not in parameter set.
			*/
			throw SqlEx.get( ERR_GC4012_INVALID_COLUMN_NAME );
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
		{
			for( int i = 0; i < opMap.Length; i++ )
				if ( index == opMap[ i ] )  return( i );
			throw SqlEx.get( ERR_GC4011_INDEX_RANGE );
		} // unMap


	} // class AdvanCall

}  // namespace
