/*
** Copyright (c) 1999, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: paramset.cs
	**
	** Description:
	**	Defines classes representing a set of query parameters.
	**
	**  Classes:
	**
	**	ParamSet    A parameter data-set.
	**	Param	    An entry in a parameter data-set.
	**
	** History:
	**	15-Nov-00 (gordy)
	**	    Created.
	**	 8-Jan-01 (gordy)
	**	    Extracted additional parameter handling from AdvanPrep.
	**	11-Apr-01 (gordy)
	**	    Do not include time with DATE datatypes.
	**	10-May-01 (gordy)
	**	    Added support for UCS2 datatypes.
	**	 1-Jun-01 (gordy)
	**	    requires long strings to be converted to BLOBs, but
	**	    not the other way around.  Removed conversion of short
	**	    BLOBs to fixed length strings, which removed the need to
	**	    track stream lengths.  No longer need the wrapper classes
	**	    and interfaces associated with stream lengths, greatly
	**	    simplifying this implementation.
	**	13-Jun-01 (gordy)
	**	    No longer need setUnicode() and UniRdr class.  The 2.1
	**	    spec requires UTF-8 encoding for unicode streams, which can
	**	    be read using the standard class InputStreamReader.
	**	30-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	19-Feb-03 (gordy)
	**	    Updated to 3.0.
	**	15-Apr-03 (gordy)
	**	    Date formatters require synchronization.
	**	26-Jun-03 (gordy)
	**	    All CHAR/VARCHAR coercions need alt storage handling.
	**	11-Jul-03 (wansh01)
	**	    all BINARY/CHAR coercions using getBytes() instead of str2bin().
	**	21-Jul-03 (gordy)
	**	    Permit conversion from empty string to Date/Time/Timestamp.
	**	12-Sep-03 (gordy)
	**	    Date formatter replaced with SqlDates utility.
	**	15-Mar-04 (gordy)
	**	    Added support for BIGINT values sent to DBMS.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	28-Apr-04 (wansh01)
	**	    Added type as an input for setObejct().
	**	    The input type is used for init and coerce(). 	
	**	29-Apr-04 (wansh01)
	**	    Changed prec calculation for BigDecimal. 
	**	20-May-04 (gordy)
	**	    Allow for coerce of empty strings to support Ingres empty dates.
	**	28-May-04 (gordy)
	**	    Add max column lengths for NCS columns.
	**	11-Aug-04 (gordy)
	**	    The described length of NCS columns need to be in bytes.
	**	19-Jun-06 (gordy)
	**	    ANSI Date/Time data type support.
	**	28-Jun-06 (gordy)
	**	    OUT procedure parameter support.
	**	22-Sep-06 (gordy)
	**	    Fixed scale of TIMESTAMP values.
	**	21-Mar-07 (thoda04)
	**	    Added IntervalDayToSecond, IntervalYearToMonth.
	**	 7-Dec-09 (gordy, ported by thoda04)
	**	    Fully support BOOLEAN data type.
	**	16-feb-10 (thoda04) Bug 123298
	**	    Added SendIngresDates support to send .NET DateTime parms
	**	    as ingresdate type rather than as timestamp_w_timezone.
	**	 3-Mar-10 (thoda04)  SIR 123368
	**	    Added support for IngresType.IngresDate parameter data type.
	*/


	/*
	** Name: ParamSet
	**
	** Description:
	**	Class which manages a parameter data set.
	**
	**	Parameter values are stored as specific class instances
	**	depending on the associated SQL type.  The supported SQL
	**	types and their associated storage class are as follows:
	**	
	**	SQL Type	Storage Class
	**	-------------	--------------------
	**	NULL		null
	**	BOOLEAN		ProviderType.Boolean
	**	TINYINT		ProviderType.Byte
	**	SMALLINT	ProviderType.Short
	**	INTEGER		ProviderType.Integer
	**	BIGINT		ProviderType.Long
	**	REAL		ProviderType.Float
	**	DOUBLE		ProviderType.Double
	**	DECIMAL		ProviderType.Decimal
	**	CHAR		char[]
	**	VARCHAR		ProviderType.String 
	**	LONGVARCHAR	Reader
	**	BINARY		byte[]
	**	VARBINARY	byte[]
	**	LONGVARBINARY	InputStream
	**	DATE		ProviderType.String 
	**	TIME		ProviderType.String
	**	TIMESTAMP	ProviderType.String
	**
	**	The preferred alternate storage format for
	**	BIGINT is DECIMAL, but DECIMAL has it's own alternate
	**	storage format.  To make this managable, BIGINT has the
	**	same alternate format as DECIMAL, uses the same logic to
	**	select the aternate format, and is forced to the DECIMAL
	**	type when BIGINT is not supported.  This allows BIGINT
	**	to be sent to the DBMS as BIGINT, DECIMAL or DOUBLE as
	**	needed.
	**
	**	The following SQL types are supported on input as aliases
	**	for the indicated types:
	**
	**	Alias Type   	SQL Type
	**	-------------	-------------
	**	BIT          	BOOLEAN
	**	FLOAT        	DOUBLE
	**	NUMERIC      	DECIMAL
	**
	**	In addition to the primary storage class given above, some
	**	SQL types support an alternate storage class as listed below:
	**
	**	SQL Type     	Storage Class
	**	-------------	--------------------
	**	CHAR         	byte[]
	**	VARCHAR      	byte[] 
	**
	**	In all other cases, the alternate storage class is the same as
	**	the primary storage class.  Use of the alternate storage class 
	**	is indicated by presence of the ALT flag for the parameter.
	**	When the parameter value is available in the alternate storage
	**	class, the set() method can be used to assign the type, value
	**	and ALT flag with the least overhead.  Coercion to the alternate 
	**	storage class can be done by passing the primary storage class 
	**	to setObject() and passing TRUE for the alt parameter.  
	**
	**	Note that for the character types, coercion to the alternate 
	**	storage format must be done using the CharSet negotiated for 
	**	the server connection.
	**
	**	The setObject() method also supports the coercion of parameter 
	**	values to other SQL types as required by the driver spec.
	**	The following table lists the ProviderType types supported as input
	**	values and their mapping to SQL types:
	**
	**	ProviderType Type		SQL Type
	**	------------		--------
	**	null			NULL
	**	ProviderType.Boolean	BOOLEAN
	**	ProviderType.Byte		TINYINT
	**	ProviderType.Short		SMALLINT
	**	ProviderType.Integer	INTEGER
	**	ProviderType.Long		BIGINT
	**	ProviderType.Float		REAL
	**	ProviderType.Double	DOUBLE
	**	ProviderType.Decimal	DECIMAL
	**	char[]			CHAR
	**	ProviderType.String	VARCHAR
	**	Reader			LONGVARCHAR
	**	byte[]			VARBINARY
	**	InputStream		LONGVARBINARY
	**	ProviderType.Date		DATE
	**	ProviderType.Time		TIME
	**	ProviderType.DateTime	TIMESTAMP
	**	ProviderType.IngresDate	TIMESTAMP
	**
	**	In addition to coercion to SQL types other than the associated 
	**	type shown above, coercion is also supported for instances where 
	**	the ProviderType type listed above differs from the storage class of the 
	**	associated SQL type.
	**
	**  Constants:
	**
	**	PS_FLG_ALT	    Alternate Format
	**	PS_FLG_PROC_IN		Procedure IN parameter.
	**	PS_FLG_PROC_OUT		Procedure OUT parameter.
	**	PS_FLG_PROC_IO		Procedure INOUT parameter.
	**	PS_FLG_PROC_GTT	    Global Temp Table parameter.
	**
	**  Public Methods:
	**
	**	getSqlType	    Determine parameter type from value.
	**	setDefaultFlags	    Set default parameter flags.
	**	clone		    Make a copy of this parameter set.
	**	clear		    Clear all parameter settings.
	**	setObject	    Set parameter from object.
	**	set		    Set parameter type and value.
	**	setFlags	    Set parameter flags.
	**	setName		    Set parameter name.
	**	getCount	    Returns number of parameters.
	**	isSet		    Has a parameter been set.
	**	getType		    Returns parameter type.
	**	getValue	    Returns parameter value.
	**	getFlags	    Returns parameter flags.
	**	getName		    Returns parameter name.
	**
	**  Private Data:
	**
	**	PS_FLG_SET	    Parameter is set.
	**	EXPAND_SIZE	    Size to grow the parameter data array.
	**	paramArray		    Parameter array.
	**	param_cnt	    Number of actual parameters.
	**	dflt_flags	    Default parameter flags.
	**
	**	emptyVarChar		Empty data values.
	**	emptyNVarChar
	**	emptyVarByte
	**	tempLongChar		Data values to assist data conversion.
	**	tempLongNChar
	**	tempLongByte
	**
	**
	**  Private Methods:
	**
	**	check		    Validate parameter index.
	**	coerce		    Validate/convert parameter value.
	**	checkSqlType	    Validate/convert parameter type.
	**	str2bin		    Convert hex string to byte array.
	**
	** History:
	**	15-Nov-00 (gordy)
	**	    Created.
	**	 8-Jan-01 (gordy)
	**	    Extracted additional parameter handling from AdvanPrep.
	**	    Added default parameter flags, param_flags, and max
	**	    varchar string length, max_vchr_len.
	**	10-May-01 (gordy)
	**	    Added constants for parameter flags.  Removed setNull()
	**	    method as can be done through setObject().  Added str2bin
	**	    for String to BINARY conversion.
	**	 1-Jun-01 (gordy)
	**	    Removed setBinary(), setAscii() and setCharacter() since
	**	    wrapper classes are no longer needed to track stream lengths.
	**	13-Jun-01 (gordy)
	**	    No longer need setUnicode() and UniRdr class.  The 2.1
	**	    spec requires UTF-8 encoding for unicode streams, which can
	**	    be read using the class InputStreamReader.
	**      13-Aug-01 (loera01) SIR 105521
	**          Added PS_FLG_PROC_GTT constant to support global temp table
	**          parameters.
	**	31-Oct-02 (gordy)
	**	    Removed unused max_vchr_len and setMaxLength().
	**	25-Feb-03 (gordy)
	**	    Added internal PS_FLG_SET to indicate set parameters rather
	**	    than relying on simply object existence which may be a hold
	**	    over from a prior parameter set (cleared but not purged).
	**	    Changed flags to short for additional bits.  Added char_set 
	**	    to support string conversions.
	**	21-Jul-03 (gordy)
	**	    Added empty string constant for Ingres empty dates.
	**	12-Sep-03 (gordy)
	**	    Date formatter replaced with GMT indicator.
	**	 1-Dec-03 (gordy)
	**	    Completely reworked interface taking in aspects of statement
	**	    parameter handling while moving internal data handling to 
	**	    SqlData objects.  DrvObj is now base class giving access 
	**	    to connection information (dropped char_set and use_gmt).  
	**	    Renamed param_flags to dflt_flags.  Added tempLongChar, 
	**	    tempLongNChar, tempLongByte, emptyVarChar, emptyVarByte, and 
	**	    associated static initializer.  Moved getSqlType(), str2bin()
	**	    and functionality of coerce() to SqlData and SqlXXX classes.  
	**	    Dropped clone() method and added getStorage().  Removed old 
	**	    interface of set() and setObject() and replaced with init() 
	**	    and setXXX() methods (set() is now an internal method and 
	**	    setObject() reflects JDBC semantics).  Incorporated sendDesc(), 
	**	    sendData() and useAltStorage() from statement parameter handling.
	**	23-Jan-04 (gordy)
	**	    Added missing emptyNVarChar() to support 0 length NCHAR columns.
	**	    Restored coerce() function to perform conversions of date/time
	**	    values not supported by the common storage class IngresDate.
	**	15-Mar-04 (gordy)
	**	    Made checkSqlType non-static as it needs to check protocol level
	**	    to determine BIGINT support (force to DECIMAL).
	**	28-Jun-06 (gordy)
	**	    Added PS_FLG_PROC_OUT for procedure output parameters.
	*/

	class
		ParamSet
		: DrvObj
	{

		/*
		** Constants for parameter flags.
		*/
		public const ushort PS_FLG_PROC_IN = 0x01;
		public const ushort PS_FLG_PROC_OUT = 0x02;
		public const ushort PS_FLG_PROC_IO = 0x03;
		public const ushort	PS_FLG_PROC_GTT     = 0x04;

		public const ushort    PS_FLG_ALT = 0x10;
		public const ushort notPS_FLG_ALT = (ushort)(0xFFFF ^ PS_FLG_ALT);

		private const ushort    PS_FLG_SET = 0x80;	// Internal use
		private const ushort notPS_FLG_SET = (ushort)(0xFFFF ^ PS_FLG_SET);

		/*
		** A CharSet is required to generate a SqlVarChar instance,
		** but at static initialization the connection CharSet is not
		** known.  Since this will be used to represent a zero length
		** string, the CharSet does not actually matter so the default
		** CharSet is used.
		*/
		private static SqlVarChar	emptyVarChar = 
						new SqlVarChar( new CharSet(), 0 );
		private static SqlNVarChar	emptyNVarChar = new SqlNVarChar( 0 );
		private static SqlVarByte	emptyVarByte = new SqlVarByte( 0 );

		/*
		** While a generic Vector class is a possible
		** implementation for this class, a parameter
		** data set is best represented by a sparse
		** array which is not well suited to the Vector
		** class.
		*/
		private const int   	EXPAND_SIZE = 10;
		private int			param_cnt = 0;
		private Param[]		paramArray;

		private ushort		dflt_flags = 0;	// Default flags.
		private SqlLongChar tempLongChar = null;	// Data Conversion
		private SqlLongNChar tempLongNChar = null;
		private SqlLongByte tempLongByte = null;

		/*
		** Name: ParamSet
		**
		** Description:
		**	Class constructor.  Creates a parameter set of the
		**	default size.  A date formatter is required for
		**	converting date/time objects into internal format
		**	and a CharSet for string conversions.
		**
		** Input:
		**	use_gmt		True to use GMT with date/time values.
		**	char_set	Connection CharSet
		**
		** Output:
		**	None.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	31-Oct-02 (gordy)
		**	    Use negative value for default size.
		**	25-Feb-03 (gordy)
		**	    Added char_set parameter.
		**	12-Sep-03 (gordy)
		**	    Date formatter replaced with GMT indicator.
		*/

		public
			ParamSet( DrvConn conn ) :
				this(conn, EXPAND_SIZE)
		{
		} // ParamSet


		/*
		** Name: ParamSet
		**
		** Description:
		**	Class constructor.  Creates a parameter set of the
		**	requested size.  If the provided size is negative,
		**	a parameter set of the default size is created.  A 
		**	date formatter is required for converting date/time 
		**	objects into internal format and a CharSet for string
		**	conversions.
		**
		** Input:
		**	size		Initial size.
		**	use_gmt		True to use GMT with date/time values.
		**	char_set	Connection CharSet.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	31-Oct-02 (gordy)
		**	    Allow zero as valid initial size.
		**	25-Feb-03 (gordy)
		**	    Added char_set parameter.
		**	12-Sep-03 (gordy)
		**	    Date formatter replaced with GMT indicator.
		*/

		public
			ParamSet(DrvConn conn, int size ) :
				base(conn)
		{
			paramArray = new Param[ (size < 0) ? 0 : size ];

			title = trace.getTraceName() + "-ParamSet[" + inst_id + "]";
			tr_id = "Param[" + inst_id + "]";
		} // ParamSet


		/*
		** Name: setDefaultFlags
		**
		** Description:
		**	Set the default parameter flags which are assigned
		**	when a parameter is assigned using the set() method.
		**
		** Input:
		**	flags	    Descriptor flags.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	15-Jun-00 (gordy)
		**	    Created.
		**	 8-Jan-01 (gordy)
		**	    Set global flags rather than specific parameter flags.
		*/

		public void
			setDefaultFlags( ushort flags )
		{
			dflt_flags = flags;
			return;
		} // setDefaultFlags


		/*
		** Name: clear
		**
		** Description:
		**	Clear stored parameter values and optionally free resources
		**
		** Input:
		**	purge	Purge allocated data?
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	31-Oct-02 (gordy)
		**	    Add synchronization.
		**	25-Feb-03 (gordy)
		**	    Clear params to reset flags.  Purge all parameters.
		*/

		public void
		clear( bool purge )
		{
			lock(this)
			{
				/*
				** When purging, scan all parameters to get hold
				** overs from previous sets.  Otherwise, just scan
				** the current parameter set.
				*/
				if ( purge )  param_cnt = paramArray.Length;

				for( int i = 0; i < param_cnt; i++ )  
					if ( paramArray[ i ] != null )
					{
						paramArray[ i ].clear();
						if ( purge )  paramArray[ i ] = null;
					}

				param_cnt = 0;
				return;
			}
		} // clear


		/*
		** Name: getCount
		**
		** Description:
		**	Returns the number parameters in the data set
		**	(includes unset intermediate parameters).
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Number of parameters.
		**
		** History:
		**	15-Nov-00 (gordy)
		*/

		public int
			getCount()
		{
			return (param_cnt);
		} // getCount


		/*
		** Name: isSet
		**
		** Description:
		**	Has the requested parameter been set?
		**
		** Input:
		**	index	Parameter index (0 based).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	TRUE if parameter set, FALSE otherwise.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	25-Feb-03 (gordy)
		**	    Check set flag.
		*/

		public bool
			isSet(int index)
		{
			return (index < param_cnt && paramArray[index] != null &&
				(paramArray[index].flags & PS_FLG_SET) != 0);
		} // isSet


		/*
		** Name: getType
		**
		** Description:
		**	Retrieve the type of a parameter.
		**
		** Input:
		**	index	Parameter index (0 based).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Parameter type.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    The check() method now returns parameter object.
		*/

		public ProviderType
			getType(int index)
		{
			return (check(index).type);
		} // getType


		/*
		** Name: getValue
		**
		** Description:
		**	Retrieve the value of a parameter.
		**
		** Input:
		**	index	Parameter index (0 based).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Object	Parameter value.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    The check() method now returns parameter object.
		*/

		public Object
			getValue(int index)
		{
			return (check(index).value);
		} // getValue


		/*
		** Name: getFlags
		**
		** Description:
		**	Retrieve the flags of a parameter.
		**
		** Input:
		**	index	Parameter index (0 based).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	short	Parameter flags.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	25-Feb-03 (gordy)
		**	    Changed flags to short for additional bits.
		**	 1-Dec-03 (gordy)
		**	    The check() method now returns parameter object.
		*/

		public ushort
			getFlags(int index)
		{
			return (check(index).flags);
		} // getFlags


		/*
		** Name: getName
		**
		** Description:
		**	Retrieve the name of a parameter.
		**
		** Input:
		**	index	Parameter index (0 based).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Parameter name.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    The check() method now returns parameter object.
		*/

		public String
			getName(int index)
		{
			return (check(index).name);
		} // getName


		/*
		** Name: init
		**
		** Description:
		**	Initialize a parameter to hold values for a given
		**	SQL type.  Alternate storage format may be used
		**	based on attributes of the associated connection.
		**
		**	The parameter is not considered to be set.  Storage
		**	from prior usage may be re-used and the resulting
		**	parameter value is indeterminate.  Parameter flags
		**	and name are not changed.
		**
		** Input:
		**	index	Parameter index (0 based).
		**	type	SQL type.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		init(int index, ProviderType type)
		{
			init(index, type, useAltStorage(type));
			return;
		} // init


		/*
		** Name: init
		**
		** Description:
		**	Initialize a parameter to hold values for a SQL type
		**	which is associated with a Java object.  Alternate 
		**	storage format may be used based on attributes of the 
		**	associated connection.
		**
		**	The parameter is not considered to be set.  Storage
		**	from prior usage may be re-used and the resulting
		**	parameter value is indeterminate.  Parameter flags
		**	and name are not changed.
		**
		** Input:
		**	index	Parameter index (0 based).
		**	value	Java object.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		init(int index, Object value)
		{
			ProviderType type = SqlData.getSqlType(value);
			init(index, type, useAltStorage(type));
			return;
		} // init


		/*
		** Name: init
		**
		** Description:
		**	Initialize a parameter to hold values for a SQL type,
		**	which is associated with a Java object, and optional
		**	alternate storage format.
		**
		**	The parameter is not considered to be set.  Storage
		**	from prior usage may be re-used and the resulting
		**	parameter value is indeterminate.  Parameter flags
		**	and name are not changed.
		**
		** Input:
		**	index	Parameter index (0 based).
		**	value	Java object.
		**	alt	True for alternate storage format.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		init(int index, Object value, bool alt)
		{
			init(index, SqlData.getSqlType(value), alt);
			return;
		} // init


		/*
		** Name: init
		**
		** Description:
		**	Initialize a parameter to hold values for a given
		**	SQL type and optional alternate storage format.
		**
		**	The parameter is not considered to be set.  Storage
		**	from prior usage may be re-used and the resulting
		**	parameter value is indeterminate.  Parameter flags
		**	and name are not changed.
		**
		** Input:
		**	index	Parameter index (0 based).
		**	type	SQL type.
		**	alt	True for alternate storage format.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		init(int index, ProviderType type, bool alt)
		{
			/*
			** Validate index and SQL data type.  Allocates referenced
			** parameter if necessary and standardizes the type.
			*/
			Param param = check(index);
			type = checkSqlType(type);

			lock (param)
			{
				/*
				** Check if existing storage is compatible with requested type.
				*/
				if (
					 param.value == null || type != param.type ||
					 alt != ((param.flags & PS_FLG_ALT) != 0)
				   )
					set(param, type, alt);	// Allocate new storage

				param.flags &= notPS_FLG_SET;	// Clear SET flag
			}

			return;
		} // init


		/*
		** Name: setFlags
		**
		** Description:
		**      Add flags to a parameter.  Does not change existing
		**	flags or mark parameter as set.
		**
		** Input:
		**      index       Parameter index (0 based).
		**      flags       Parameter flags.
		**
		** Output:
		**      None.
		**
		** Returns:
		**      void.
		**
		** History:
		**      15-Jun-00 (gordy)
		**          Created.
		**	31-Oct-02 (gordy)
		**	    Add synchronization to ensure consistent update.
		**	25-Feb-03 (gordy)
		**	    Changed flags to short for additional bits.
		*/

		public void
			setFlags(int index, ushort flags)
		{
			Param param = check(index);	// Validate index & parameter.
			flags &= (notPS_FLG_SET & notPS_FLG_ALT);	// Clear internal flags.
			lock (param) { param.flags |= flags; }
			return;
		} // setFlags


		/*
		** Name: setName
		**
		** Description:
		**	Set the name of a parameter.  Does not mark parameter as set.
		**
		** Input:
		**	index	    Parameter index (0 based).
		**	name	    Parameter name.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	15-Jun-00 (gordy)
		**	    Created.
		*/

		public void
			setName(int index, String name)
		{
			check(index).name = name;
			return;
		} // setName


		/*
		** Name: setNull
		**
		** Description:
		**	Set a parameter to NULL.  If the parameter has not been
		**	initialized for a specific SQL type, a generic NULL type
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setNull(int index)
		{
			Param param = check(index);

			lock (param)
			{
				if (param.value == null)
					set(param, ProviderType.DBNull, useAltStorage(ProviderType.DBNull));

				param.value.setNull();
				param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
			}

			return;
		} // setNull


		/*
		** Name: setBoolean
		**
		** Description:
		**	Set parameter to a boolean value.  If the parameter has not
		**	been initialized for a specific SQL type, the BOOLEAN type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Boolean value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		**	15-Mar-04 (gordy)
		**	    BOOLEAN now always uses alternate storage format.
		*/

		public void
		setBoolean( int index, bool value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.Boolean, useAltStorage( ProviderType.Boolean ) );
			
			param.value.setBoolean( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
			}

			return;
		} // setBoolean


		/*
		** Name: setByte
		**
		** Description:
		**	Set parameter to a byte value.  If the parameter has not
		**	been initialized for a specific SQL type, the TINYINT 
		**	type will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Byte value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setByte( int index, byte value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.TinyInt, useAltStorage( ProviderType.TinyInt ) );
			
			param.value.setByte( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}    
		    
			return;
		} // setByte


		/*
		** Name: setShort
		**
		** Description:
		**	Set parameter to a short value.  If the parameter has not
		**	been initialized for a specific SQL type, the SMALLINT 
		**	type will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Short value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setShort( int index, short value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.SmallInt, useAltStorage( ProviderType.SmallInt ) );
			
			param.value.setShort( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
			}    
		    
			return;
		} // setShort


		/*
		** Name: setInt
		**
		** Description:
		**	Set parameter to an int value.  If the parameter has not
		**	been initialized for a specific SQL type, the INTEGER 
		**	type will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Int value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setInt( int index, int value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.Integer,
					useAltStorage( ProviderType.Integer ) );
			
			param.value.setInt( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}    
		    
			return;
		} // setInt


		/*
		** Name: setLong
		**
		** Description:
		**	Set parameter to a long value.  If the parameter has not
		**	been initialized for a specific SQL type, the BIGINT type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Long value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		**	15-Mar-04 (gordy)
		**	    Implement BIGINT support.  Need to call checkSqlType() in case
		**	    BIGINT not supported on connection (changes to DECIMAL).
		*/

		public void
		setLong( int index, long value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, checkSqlType( ProviderType.BigInt ), 
					useAltStorage( ProviderType.BigInt ) );
			
			param.value.setLong( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
			}    
		    
			return;
		} // setLong


		/*
		** Name: setFloat
		**
		** Description:
		**	Set parameter to a float value.  If the parameter has not
		**	been initialized for a specific SQL type, the REAL type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Float value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setFloat( int index, float value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.Real, useAltStorage( ProviderType.Real ) );
			
			param.value.setFloat( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
			}    
		    
			return;
		} // setFloat


		/*
		** Name: setDouble
		**
		** Description:
		**	Set parameter to a double value.  If the parameter has not
		**	been initialized for a specific SQL type, the DOUBLE type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Double value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setDouble( int index, double value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.Double, useAltStorage( ProviderType.Double ) );
			
			param.value.setDouble( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}    
		    
			return;
		} // setDouble


		/*
		** Name: setDecimal
		**
		** Description:
		**	Set parameter to a Decimal value.  If the parameter has not
		**	been initialized for a specific SQL type, the DECIMAL type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Decimal value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setDecimal( int index, Decimal value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.Decimal, useAltStorage( ProviderType.Decimal ) );
			
			param.value.setDecimal( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}

			return;
		} // setDecimal


		/*
		** Name: setString
		**
		** Description:
		**	Set parameter to a String value.  If the parameter has not
		**	been initialized for a specific SQL type, the VARCHAR type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	String value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setString( int index, String value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.VarChar, useAltStorage( ProviderType.VarChar ) );
			
			param.value.setString( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}    
		    
			return;
		} // setString


		/*
		** Name: setBytes
		**
		** Description:
		**	Set parameter to a byte array value.  If the parameter has not
		**	been initialized for a specific SQL type, the VARBINARY type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Byte array value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setBytes( int index, byte[] value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.VarBinary, useAltStorage( ProviderType.VarBinary ) );
			
			param.value.setBytes( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}

			return;
		} // setBytes


		/*
		** Name: setDate
		**
		** Description:
		**	Set parameter to a Date value.  If the parameter has not
		**	been initialized for a specific SQL type, the DATE type
		**	will be used.
		**
		**	A relative timezone may be provided which is applied to 
		**	adjust the input such that it represents the original 
		**	date/time value in the timezone provided.  The default 
		**	is to use the local timezone.
		**
		** Input:
		**	index	Parameter index.
		**	value	Date value.
		**	tz	Relative timezone, NULL for local.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		**	23-Jan-04 (gordy)
		**	    Init to DATE (keep date/time/timestamp distinct at this level).
		*/

		public void
		setDate( int index, DateTime value, TimeZone tz )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.Date, useAltStorage( ProviderType.Date ) );
			
			param.value.setDate( value, tz );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}

			return;
		} // setDate


		/*
		** Name: setTime
		**
		** Description:
		**	Set parameter to a Time value.  If the parameter has not
		**	been initialized for a specific SQL type, the TIME type 
		**	will be used.
		**
		**	A relative timezone may be provided which is applied to 
		**	adjust the input such that it represents the original 
		**	date/time value in the timezone provided.  The default 
		**	is to use the local timezone.
		**
		** Input:
		**	index	Parameter index.
		**	value	Time value.
		**	tz	Relative timezone, NULL for local.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		**	23-Jan-04 (gordy)
		**	    Init to TIME (keep date/time/timestamp distinct at this level).
		*/

		public void
		setTime( int index, DateTime value, TimeZone tz )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.Time, useAltStorage( ProviderType.Time ) );
			
			param.value.setTime( value, tz );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}

			return;
		} // setTime


		/*
		** Name: setTimestamp
		**
		** Description:
		**	Set parameter to a Timestamp value.  If the parameter has not
		**	been initialized for a specific SQL type, the TIMESTAMP type 
		**	will be used.
		**
		**	A relative timezone may be provided which is applied to 
		**	adjust the input such that it represents the original 
		**	date/time value in the timezone provided.  The default 
		**	is to use the local timezone.
		**
		** Input:
		**	index	Parameter index.
		**	value	Timestamp value.
		**	tz	Relative timezone, NULL for local.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setTimestamp( int index, DateTime value, TimeZone tz )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null )
				set( param, ProviderType.DateTime, useAltStorage( ProviderType.DateTime ) );
			
			param.value.setTimestamp( value, tz );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}

			return;
		} // setTimestamp


		/*
		** Name: setBinaryStream
		**
		** Description:
		**	Set parameter to a binary stream.  If the parameter has not
		**	been initialized for a specific SQL type, the LONGVARBINARY
		**	type will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Binary stream.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setBinaryStream( int index, InputStream value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null ) set( param, ProviderType.LongVarBinary, 
							useAltStorage( ProviderType.LongVarBinary ) );
			
			param.value.setBinaryStream( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}

			return;
		} // setBinaryStream


		/*
		** Name: setAsciiStream
		**
		** Description:
		**	Set parameter to a ASCII stream.  If the parameter has not
		**	been initialized for a specific SQL type, the LONGVARCHAR type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	ASCII stream.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setAsciiStream( int index, InputStream value )
		{
			Param param = check( index );

			lock( param )
			{
			if ( param.value == null ) set( param, ProviderType.LongVarChar, 
				useAltStorage( ProviderType.LongVarChar ) );
			
			param.value.setAsciiStream( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}

			return;
		} // setAsciiStream


		/*
		** Name: setUnicodeStream
		**
		** Description:
		**	Set parameter to a Unicode stream.  If the parameter has not
		**	been initialized for a specific SQL type, the LONGVARCHAR type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Unicode stream.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setUnicodeStream( int index, InputStream value )
		{
			Param param = check( index );

			lock( param )
			{
				if (param.value == null) set(param, ProviderType.LongVarChar, 
					useAltStorage( ProviderType .LongVarChar ) );
			
			param.value.setUnicodeStream( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}

			return;
		} // setUnicodeStream


		/*
		** Name: setCharacterStream
		**
		** Description:
		**	Set parameter to a character stream.  If the parameter has not
		**	been initialized for a specific SQL type, the LONGVARCHAR type 
		**	will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Character stream.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		setCharacterStream( int index, Reader value )
		{
			Param param = check( index );

			lock( param )
			{
				if (param.value == null) set(param, ProviderType.LongVarChar,
					useAltStorage(ProviderType.LongVarChar));
			
			param.value.setCharacterStream( value );
			param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
		}

			return;
		} // setCharacterStream


		/*
		** Name: setObject
		**
		** Description:
		**	Set a parameter to a Java object value.  If the parameter
		**	has not been initialized for a specific SQL type, the SQL
		**	type associated with the class of the Java object will be
		**	used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Java object.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		**	23-Jan-04 (gordy)
		**	    Added coerce() method to handle coercions which are not
		**	    provided directly by the storage classes.
		**	15-Mar-04 (gordy)
		**	    Need to call checkSqlType() in case object type is aliased.
		*/

		public void
		setObject(int index, Object value)
		{
			Param param = check(index);

			lock (param)
			{
				if (param.value == null)
				{
					ProviderType type = SqlData.getSqlType(value);
					set(param, checkSqlType(type), useAltStorage(type));
				}

				param.value.setObject(value);
				param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
			}

			return;
		} // setObject

		/*
		** Name: setObject
		**
		** Description:
		**	Set a parameter to a Java object value.  If the parameter 
		**	has not been initialized for a specific SQL type, the input 
		**	type will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Java object.
		**	type 	Parameter type.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	28-Apr-04 (wansh01)
		**	    Created.
		**	19-Jun-06 (gordy)
		**	    Pass alt indicator to coerce();
		*/

		public void
		setObject(int index, Object value, ProviderType type)
		{
			Param param = check(index);

			type = checkSqlType(type);
			lock (param)
			{
				if (param.value == null)
					set(param, type, useAltStorage(type));


				param.value.setObject(coerce(value, type,
								   (param.flags & PS_FLG_ALT) != 0, 0));
				param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
			}

			return;
		} // setObject


		/*
		** Name: setObject
		**
		** Description:
		**	Set a parameter to a Java object value.  If the parameter 
		**	has not been initialized for a specific SQL type, the input 
		**	type will be used.
		**
		** Input:
		**	index	Parameter index.
		**	value	Java object.
		**	type 	Parameter type.
		**	scale	Number of digits following decimal point.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		**	23-Jan-04 (gordy)
		**	    Added coerce() method to handle coercions which are not
		**	    provided directly by the storage classes.
		**	15-Mar-04 (gordy)
		**	    Need to call checkSqlType() in case object type is aliased.
		**	28-Apr-04 (wansh01)
		**	    Added type as an input parameter.The input type is used   
		** 	    for init and coerce(). 	
		**	19-Jun-06 (gordy)
		**	    Pass alt indicator to coerce();
		*/

		public void
		setObject(int index, Object value, ProviderType type, int scale, int maxsize)
		{
			Param param = check(index);

			type = checkSqlType(type);
			lock (param)
			{
				if (param.value == null)
					set(param, type, useAltStorage(type));

				SqlData sqldata = param.value;  // work around compiler bug
				sqldata.setObject(coerce(value, type,
					(param.flags & PS_FLG_ALT) != 0, maxsize),
					scale);
				param.flags |= (ushort)(PS_FLG_SET | dflt_flags);
			}

			return;
		} // setObject


		/*
		** Name: set
		**
		** Description:
		**	Sets the parameter type, allocates storage based on type
		**	and alternate storage format, and sets ALT flag according 
		**	to input.  Other flags and parameter name are not changed.
		**
		**	Caller must provide synchronization on parameter.
		**
		** Input:
		**	param	Parameter.
		**	type	SQL type.
		**	alt	True for alternate storage format.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		private void
		set( Param param, ProviderType type, bool alt )
		{
			ushort flags = (ushort)(param.flags & notPS_FLG_ALT);	// All but ALT
			if ( alt )  flags |= PS_FLG_ALT;
		    
			param.type = type;
			param.value = getStorage( type, alt );
			param.flags = flags;
			return;
		} // set


		/*
		** Name: check
		**
		** Description:
		**	Check the parameter index and expand the parameter
		**	array if necessary.  The referenced parameter is
		**	(optionally) allocated.
		**
		** Input:
		**	index	    Parameter index (0 based).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Param	Referenced parameter.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		*/

		private Param
		check( int index )
		{
			lock(this)
			{
				int	    new_max;

				if ( index < 0 )  throw new ArgumentOutOfRangeException();

				/*
				** If the new index is beyond the current limit,
				** expand the parameter array to include the new
				** index.
				*/
				for( new_max = paramArray.Length; index >= new_max; new_max += EXPAND_SIZE );

				if ( new_max > paramArray.Length )
				{
					Param[] new_array = new Param[ new_max ];
					System.Array.Copy( paramArray, 0, new_array, 0, paramArray.Length );
					paramArray = new_array;
				}

				/*
				** Any pre-allocated parameters between the previous
				** upper limit and the new limit are cleared and the
				** parameter count updated.  Allocate the parameter
				** for the current index if needed.
				*/
				for( ; param_cnt <= index; param_cnt++ )
					if ( paramArray[ param_cnt ] != null )  paramArray[ param_cnt ].clear();

				if ( paramArray[ index ] == null )  paramArray[ index ] = new Param();

				return paramArray[index];
			}
		} // check


		/*
		** Name: useAltStorage
		**
		** Description:
		**	Determine if the alternate storage format should be used
		**	for a given SQL type.
		**
		** Input:
		**	sqlType	    Target SQL type.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    TRUE if alternate storage should be used.
		**
		** History:
		**	25-Feb-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Moved to ParamSet to support updatable result-sets.
		**	15-Mar-04 (gordy)
		**	    Implemented BIGINT support, default to DECIMAL when not supported.
		**	    BOOLEAN now implemented as always using alternate storage format.
		**	19-Jun-06 (gordy)
		**	    ANSI Date/Time data type support.  ANSI is date/time primay
		**	    storage format and Ingres date is alternate.
		**	 7-Dec-09 (gordy, ported by thoda04)
		**	    BOOLEAN supported at API protocol level 6.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		private bool
			useAltStorage(ProviderType sqlType)
		{
			switch (sqlType)
			{
				/*
				** BOOLEAN is supported starting with API protocol level 6.
				** Alternate storage format is TINYINT.
				*/
				case ProviderType.Boolean:
					return (conn.db_protocol_level < DBMS_API_PROTO_6);

				/*
				** BIGINT is supported starting with API protocol level 3.
				** The preferred alternate storage format is DECIMAL, but
				** DECIMAL has it's own alternate format.  BIGINT uses the
				** same alternate format as DECIMAL and is forced to DECIMAL
				** when not supported.
				*/
				case ProviderType.BigInt:
					if (conn.db_protocol_level >= DBMS_API_PROTO_3)
						return (false);
					// utilize DECIMAL alternate logic
					goto case ProviderType.Decimal;

				/*
				** The messaging protocol indicates that DECIMAL values
				** will be converted to VARCHAR automatically if not 
				** supported by the DBMS.  This works well with Ingres, 
				** (which supports char to numeric coercion), but does
				** not work with gateways.  The alternate storage format
				** is DOUBLE, which is used for non-Ingres servers which
				** don't support DECIMAL.
				*/
				case ProviderType.Numeric:
				case ProviderType.Decimal:
					return (!conn.is_ingres &&
						conn.db_protocol_level < DBMS_API_PROTO_1);

				case ProviderType.Other: return (true);

				case ProviderType.Char:    // always convert to ASCII chars
				case ProviderType.VarChar:
				case ProviderType.LongVarChar:
					return (true);

				case ProviderType.NChar:
				case ProviderType.NVarChar:
				case ProviderType.LongNVarChar:
					return (!conn.ucs2_supported);

				/*
				** For date/time types, standard storage is ANSI while
				** alternate storage is Ingres date.
				*/
				case ProviderType.Date:
				case ProviderType.Time:
				case ProviderType.DateTime:
					return (conn.db_protocol_level < DBMS_API_PROTO_4  ||
					        conn.sendIngresDates);
				case ProviderType.IngresDate:
					return (true);

				case ProviderType.IntervalDayToSecond: // always convert to ASCII chars
				case ProviderType.IntervalYearToMonth:
					return (true);

			}

			return (false);
		} // useAltStorage


		/*
		** Name: getStorage
		**
		** Description:
		**	Allocates storage for SQL data values base on the SQL type
		**	and optional alternate storage format.  The class of the
		**	object returned is defined in the class description.
		**
		** Input:
		**	sqlType		SQL type.
		**	alt		True for alternate storage format.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	SqlData		SQL data value.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		**	19-Jun-06 (gordy)
		**	    ANSI Date/Time data type support.  IngresDate is now
		**	    the alternate storage for DATE/TIME/TIMESTAMP and
		**	    SqlDate/SqlTime/SqlTimestamp are the primary storage.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		/// <summary>
		/// Allocates storage for SQL data values base on the SQL type
		/// and optional alternate storage format.  The class of the
		/// object returned is defined in the class description.
		/// </summary>
		/// <param name="sqlType">SQL type.</param>
		/// <param name="alt">True for alternate storage format.</param>
		/// <returns>SQL data value.</returns>
		private SqlData
		getStorage(ProviderType sqlType, bool alt)
		{
			SqlData sqlData;

			switch (sqlType)
			{
				case ProviderType.DBNull: sqlData = new SqlNull(); break;
				case ProviderType.TinyInt: sqlData = new SqlTinyInt(); break;
				case ProviderType.SmallInt: sqlData = new SqlSmallInt(); break;
				case ProviderType.Integer: sqlData = new SqlInt(); break;
				case ProviderType.Real: sqlData = new SqlReal(); break;
				case ProviderType.Double: sqlData = new SqlDouble(); break;
				case ProviderType.Binary: sqlData = new SqlByte(); break;
				case ProviderType.VarBinary: sqlData = new SqlVarByte(); break;
				case ProviderType.LongVarBinary: sqlData = new SqlLongByte(); break;

				case ProviderType.Boolean:
					sqlData = alt ? (SqlData)(new SqlTinyInt())
							  : (SqlData)(new SqlBool());
					break;

				case ProviderType.BigInt:
					sqlData = alt ? (SqlData)(new SqlDouble())
							  : (SqlData)(new SqlBigInt());
					break;

				case ProviderType.Decimal:
					sqlData = alt ? (SqlData)(new SqlDouble())
							  : (SqlData)(new SqlDecimal());
					break;

				case ProviderType.Char:
					sqlData = alt ? (SqlData)(new SqlChar(msg.getCharSet()))
							  : (SqlData)(new SqlNChar());
					break;

				case ProviderType.VarChar:
					sqlData = alt ? (SqlData)(new SqlVarChar(msg.getCharSet()))
							  : (SqlData)(new SqlNVarChar());
					break;

				case ProviderType.LongVarChar:
					sqlData = alt ? (SqlData)(new SqlLongChar(msg.getCharSet()))
							  : (SqlData)(new SqlLongNChar());
					break;

				case ProviderType.Date:
					sqlData = alt ? (SqlData)(new IngresDate(conn.osql_dates,
										  conn.timeValuesInGMT()))
							  : (SqlData)(new SqlDate());
					break;

				case ProviderType.Time:
					sqlData = alt ? (SqlData)(new IngresDate(conn.osql_dates,
										  conn.timeValuesInGMT()))
							  : (SqlData)(new SqlTime(
							conn.osql_dates ? DBMS_TYPE_TMWO : DBMS_TYPE_TMTZ));
					break;

				case ProviderType.DateTime:
				case ProviderType.IngresDate:
					sqlData = alt ? (SqlData)(new IngresDate(conn.osql_dates,
										  conn.timeValuesInGMT()))
							  : (SqlData)(new SqlTimestamp(
							conn.osql_dates ? DBMS_TYPE_TSWO : DBMS_TYPE_TSTZ));
					break;

				default:
					throw SqlEx.get(ERR_GC401A_CONVERSION_ERR);
			}

			return (sqlData);
		} // getStorage


		/*
		** Name: checkSqlType
		**
		** Description:
		**	Validates that a SQL type is supported as a parameter type
		**	by this class.  Returns the supported parameter type after
		**	applying alias translations.
		**
		** Input:
		**	sqlType	    SQL type to validate.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Supported SQL type.
		**
		** History:
		**	19-May-99 (gordy)
		**	    Created.
		**	10-May-01 (gordy)
		**	    CHAR no longer forced to VARCHAR (character arrays fully 
		**	    supported), BINARY to VARBINARY (same storage format, but 
		**	    different transport format at Server), LONGVARCHAR
		**	    and LONGVARBINARY to VARCHAR and VARBINARY (IO classes
		**	    now supported).  NULL is now permitted.
		**	19-Feb-03 (gordy)
		**	    BIT now alias for BOOLEAN.
		**	15-Mar-04 (gordy)
		**	    BIGINT supported at DBMS/API protocol level 3.  Force back to
		**	    DECIMAL when not supported.  Method can no longer be static
		**	    due to need to check protocol level.
		**	    BOOLEAN is also now supported (but always as alternate format).
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		/// <summary>
		/// Validates that a SQL type is supported as a parameter type
		/// by this class.  Returns the supported parameter type after
		/// applying alias translations.
		/// </summary>
		/// <param name="sqlType"></param>
		/// <returns>The supported parameter type 
		/// after applying alias translations.</returns>
		private ProviderType
			checkSqlType(ProviderType sqlType)
		{
			switch (sqlType)
			{
				/*
				** Aliases
				*/
				case ProviderType.Numeric:
					return (ProviderType.Decimal);

				/*
				** BIGINT is supported starting at DBMS/API protocol level 3.
				** Otherwise, force to DECIMAL (BIGINT and DECIMAL use the
				** same alternate storage format to accomodate the change).
				*/
				case ProviderType.BigInt:
					return ((conn.db_protocol_level >= DBMS_API_PROTO_3)
						? ProviderType.BigInt : ProviderType.Decimal);

				/*
				** The following types are OK.
				*/
				case ProviderType.DBNull:
				case ProviderType.Boolean:
				case ProviderType.TinyInt:
				case ProviderType.SmallInt:
				case ProviderType.Integer:
				case ProviderType.Real:
				case ProviderType.Double:
				case ProviderType.Decimal:
				case ProviderType.Binary:
				case ProviderType.VarBinary:
				case ProviderType.LongVarBinary:
				case ProviderType.Char:
				case ProviderType.VarChar:
				case ProviderType.LongVarChar:
				case ProviderType.Date:
				case ProviderType.Time:
				case ProviderType.DateTime:
				case ProviderType.IngresDate:
					return (sqlType);

				case ProviderType.NChar:
					return (ProviderType.Char);

				case ProviderType.NVarChar:
					return (ProviderType.VarChar);

				case ProviderType.LongNVarChar:
					return (ProviderType.LongVarChar);

				case ProviderType.IntervalDayToSecond:
					return (ProviderType.VarChar);
			}

				/*
				** All others are not allowed.
				*/
				throw SqlEx.get(ERR_GC401A_CONVERSION_ERR);

		} // checkSqlType


		/*
		** Name: coerce
		**
		** Description:
		**	Applies required coercions to parameter values which
		**	are not directly supported by the underlying storage classes.
		**	Returns the original input if no coercion is applicable.
		**	Does not enforce any coercion restrictions.
		**
		** Input:
		**	obj	Source parameter value.
		**	type	Target SQL type.
		**	alt	Is alternate storage being used.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Object	Coerced value (original value if no coercion).
		**
		** History:
		**	23-Jan-04 (gordy)
		**	    Created.
		**	20-May-04 (gordy)
		**	    Allow for empty strings to support Ingres empty dates.
		**	19-Jun-06 (gordy)
		**	    ANSI Date/Time data type support.  Ingres dates now
		**	    used as alternate storage format.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		/// <summary>
		/// Applies required coercions to parameter values which
		/// are not directly supported by the underlying storage classes.
		/// Returns the original input if no coercion is applicable.
		/// Does not enforce any coercion restrictions.
		/// </summary>
		/// <param name="obj">Source parameter value.</param>
		/// <param name="tgt_type">Target SQL type.</param>
		/// <param name="alt">Is alternate storage being used?</param>
		/// <param name="maxsize">Max length (if > 0) for Arrays/Strings.</param>
		/// <returns>Coerced value (original value if no coercion).</returns>
		private static Object
		coerce(Object obj, ProviderType tgt_type, bool alt, int maxsize)
		{
			ProviderType src_type = SqlData.getSqlType(obj);

			DateTime dt;

			String str;
			char[] charArray;
			byte[] byteArray;

			/*
			** A null reference is returned for a null input
			** or a request to coerce to NULL.
			*/
			if (src_type == ProviderType.DBNull ||
				tgt_type == ProviderType.DBNull)
				return (null);

			/*
			** If input parameter is a char type
			**   then normalize into a char array type.
			*/
			if (obj is System.Char)
			{
				char[] char1Array = new Char[1];
				char1Array[0] = (char)obj;
				obj = char1Array;
			}

			/*
			** Apply any maximum size limits to the char and binary strings.
			*/
			if (maxsize > 0)
			{
				switch (src_type)
				{
					case ProviderType.Char:
						charArray = obj as char[];
						if (charArray != null)
							obj = ToCharArrayWithMaxSize(charArray, maxsize);
						break;

					case ProviderType.VarChar:
						str = obj as String;
						if (str != null)
							obj = ToStringWithMaxSize(str, maxsize);
						break;

					case ProviderType.VarBinary:
						byteArray = obj as byte[];
						if (byteArray != null)
							obj = ToByteArrayWithMaxSize(byteArray, maxsize);
						break;
				}
			}

			/*
			** Currently, the only coercions not supported by the underlying
			** storage class involve date/time values.  This is due to the
			** use of a single storage class for date/time values to represent
			** the Ingres date datatype.  The IngresDate storage class handles
			** assignment based on the input type, so our work simply entails
			** getting the input to match the target type.
			*/
			switch (tgt_type)
			{
				case ProviderType.Date:
					/*
					** IngresDate only used as aternate storage format.
					*/
					if (!alt) break;

					/*
					** IngresDate converts Date objects to date-only values,
					** Timestamp objects to timestamp values, and strings 
					** to whichever format they represent.  Need to coerce 
					** Timestamp objects to date-only values and restrict
					** string conversions to just dates.  JDBC does not 
					** require coercion of Time values.
					*/
					switch (src_type)
					{
						case ProviderType.Char:
							/*
							** Permit empty strings (will be handled by the
							** storage class) even though technically they
							** are not valid DATE values.
							*/
							if (((char[])obj).Length == 0) break;

							try { obj = DateTimeParseInvariant(new String((Char[])obj)); }
							catch (Exception)
							{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }
							break;

						case ProviderType.VarChar:
							/*
							** Permit empty strings (will be handled by the
							** storage class) even though technically they
							** are not valid DATE values.
							*/
							if (((String)obj).Length == 0) break;

							try { obj = DateTimeParseInvariant((String)obj); }
							catch (Exception)
							{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }
							break;

						case ProviderType.DateTime:
							dt = (DateTime)obj;
							obj = new DateTime(dt.Ticks, dt.Kind);
							break;
					}
					break;

				case ProviderType.Time:
					/*
					** IngresDate only used as aternate storage format.
					*/
					if (!alt) break;

					/*
					** IngresDate converts Time objects to time values,
					** Timestamp objects to timestamp values, and strings
					** to whichever format they represent.  Need to coerce
					** Timestamp objects to time values and restrict string
					** conversions to just times.  JDBC does not require
					** coercion of Date values.  
					*/
					switch (src_type)
					{
						case ProviderType.Char:
							/*
							** Permit empty strings (will be handled by the
							** storage class) even though technically they
							** are not valid TIME values.
							*/
							if (((char[])obj).Length == 0) break;

							try { obj = DateTimeParseInvariant(new String((char[])obj)); }
							catch (Exception)
							{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }
							break;

						case ProviderType.VarChar:
							/*
							** Permit empty strings (will be handled by the
							** storage class) even though technically they
							** are not valid TIME values.
							*/
							if (((String)obj).Length == 0) break;

							try { obj = DateTimeParseInvariant((String)obj); }
							catch (Exception)
							{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }
							break;

						case ProviderType.DateTime:
							dt = (DateTime)obj;
							obj = new DateTime(dt.Ticks, dt.Kind);
							break;
					}
					break;

				case ProviderType.DateTime:
				case ProviderType.IngresDate:
					/*
					** IngresDate only used as aternate storage format.
					*/
					if (!alt) break;

					/*
					** IngresDate converts Timestamp objects to timestamp
					** values, Date objects to date-only values, time
					** objects to time values, and strings to whichever
					** format they represent.  Need to coerce Date objects
					** to timestamp values, Time objects to timestamp
					** values, and restrict string conversions to just
					** timestamps.  
					*/
					switch (src_type)
					{
						case ProviderType.Char:
							/*
							** Permit empty strings (will be handled by the
							** storage class) even though technically they
							** are not valid TIMESTAMP values.
							*/
							if (((char[])obj).Length == 0) break;

							try { obj = DateTimeParseInvariant(new String((char[])obj)); }
							catch (Exception)
							{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }
							break;

						case ProviderType.VarChar:
							/*
							** Permit empty strings (will be handled by the
							** storage class) even though technically they
							** are not valid TIMESTAMP values.
							*/
							if (((String)obj).Length == 0) break;

							DateTime tempResult;
							if (!DateTime.TryParse(
									(String)obj,
									InvariantCulture,
									System.Globalization.DateTimeStyles.None,
									out tempResult))
								break;  // if not a datetime, assume interval
								        // string is being sent to ingresdate
								        // and leave as string for the DBMS

							try { obj = DateTimeParseInvariant((String)obj); }
							catch (Exception )
							{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }
							break;

						case ProviderType.Date:
							dt = (DateTime)obj;
							obj = new DateTime(dt.Ticks, dt.Kind);
							break;

						case ProviderType.Time:
							dt = (DateTime)obj;
							obj = new DateTime(dt.Ticks, dt.Kind);
							break;
					}
					break;
			}

			return (obj);
		} // coerce


		/*
		** Name: sendDesc
		**
		** Description:
		**	Send a DESC message for current set of parameters.
		**
		** Input:
		**	eog	Is this message end-of-group?
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
		**	17-Dec-99 (gordy)
		**	    Conversion to BLOB now determined by DBMS max varchar len.
		**	 2-Feb-00 (gordy)
		**	    Send short streams as VARCHAR or VARBINARY.
		**	 7-Feb-00 (gordy)
		**	    For the NULL datatype or NULL data values, send the generic
		**	    NULL datatype and the nullable indicator.
		**	13-Jun-00 (gordy)
		**	    Nullable byte converted to flags, parameter names supported
		**	    for database procedures (EdbcCall).
		**	15-Nov-00 (gordy)
		**	    Extracted Param class to create a stand-alone Paramset class.
		**	 8-Jan-01 (gordy)
		**	    Parameter set passed as parameter to support batch processing.
		**	10-May-01 (gordy)
		**	    Now pass DBMS type to support alternate formats (UCS2 for now).
		**	    Character arrays also supported.
		**	 1-Jun-01 (gordy)
		**	    Removed conversion of BLOB to fixed length type for short lengths.
		**      13-Aug-01 (loera01) SIR 105521
		**          Set the JDBC_DSC_GTT flag if the parameter is a global temp
		**          table.
		**	20-Feb-02 (gordy)
		**	    Decimal not supported by gateways at proto level 0.  Send as float.
		**	 6-Sep-02 (gordy)
		**	    Use formatted length of CHAR and VARCHAR parameters rather than
		**	    character length.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		**	19-Feb-03 (gordy)
		**	    Replaced BIT with BOOLEAN.  Skip unset parameters (default
		**	    procedure parameters).  ALT character parameters now stored
		**	    in communication format.
		**	24-June-03 (wansh01)
		**	    for binary type with 0 length send as varbinary.
		**	 1-Dec-03 (gordy)
		**	    Moved to ParamSet to support updatable result sets.
		**	23-Jan-04 (gordy)
		**	    Set NULL CHAR/BINARY columns to non-zero length to avoid coercion.
		**	15-Mar-04 (gordy)
		**	    BIGINT now full implemented with same alternate format as DECIMAL
		**	    (type is forced to DECIMAL when not supported).  BOOLEAN should
		**	    always be alternate, but force to alternate just in case.
		**	29-Apr-04 (wansh01)
		**	    To calculate prec for a Decimal, prec is one less if long  
		**	    value of the decimal is zero and scale is more than 0. 
		**	    i.e. 0.12345 prec is 5. 
		**	28-May-04 (gordy)
		**	    Add max column lengths for NCS columns.
		**	11-Aug-04 (gordy)
		**	    The described length of NCS columns need to be in bytes.
		**	28-Jun-06 (gordy)
		**	    OUT procedure parameter support.
		**	22-Sep-06 (gordy)
		**	    Determine scale of TIMESTAMP values when fractional
		**	    seconds are present.
		**	 7-Dec-09 (gordy, ported by thoda04)
		**	    Boolean now fully supported.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		public void
		sendDesc(bool eog)
		{
		lock (this)
		{
			/*
			** Count the number of paramters which are set.
			*/
			short set_cnt = 0;

			for (int param = 0; param < param_cnt; param++)
				if (paramArray[param] != null &&
					 (paramArray[param].flags & PS_FLG_SET) != 0)
					set_cnt++;

			msg.begin(MSG_DESC);
			msg.write(set_cnt);

			for (int param = 0; param < param_cnt; param++)
			{
				if (paramArray[param] == null ||
					 (paramArray[param].flags & PS_FLG_SET) == 0)
					continue;		// Skip unset params

				SqlData value = paramArray[param].value;
				ProviderType type = paramArray[param].type;
				ushort pflags = paramArray[param].flags;
				bool alt = (pflags & PS_FLG_ALT) != 0;

				short dbms = 0;
				int length = 0;
				byte prec = 0;
				byte scale = 0;
				byte flags = 0;

				switch (type)
				{
					case ProviderType.DBNull:
						dbms = DBMS_TYPE_LONG_TEXT;
						flags |= MSG_DSC_NULL;
						break;

					case ProviderType.Boolean:
						if (!alt)
						{
							dbms = DBMS_TYPE_BOOL;
						}
						else // BOOLEAN not supported by protocol level < 6
						{
							type = ProviderType.TinyInt;
							dbms = DBMS_TYPE_INT;
							length = 1;
						}
						break;

					case ProviderType.TinyInt:
						dbms = DBMS_TYPE_INT;
						length = 1;
						break;

					case ProviderType.SmallInt:
						dbms = DBMS_TYPE_INT;
						length = 2;
						break;

					case ProviderType.Integer:
						dbms = DBMS_TYPE_INT;
						length = 4;
						break;

					case ProviderType.BigInt:
						if (alt)
						{
							type = ProviderType.Double;
							dbms = DBMS_TYPE_FLOAT;
							length = 8;
						}
						else
						{
							dbms = DBMS_TYPE_INT;
							length = 8;
						}
						break;

					case ProviderType.Real:
						dbms = DBMS_TYPE_FLOAT;
						length = 4;
						break;

					case ProviderType.Double:
						dbms = DBMS_TYPE_FLOAT;
						length = 8;
						break;

					case ProviderType.Decimal:
						if (alt)
						{
							type = ProviderType.Double;
							dbms = DBMS_TYPE_FLOAT;
							length = 8;
						}
						else
						{
							dbms = DBMS_TYPE_DECIMAL;
							prec = 15;

							if (!value.isNull())
							{
								Decimal dec = value.getDecimal();
								String str = value.getString();

								prec = (byte)(str.Length -
									(Math.Sign(dec) < 0 ? 1 : 0) -
									(Scale(dec) > 0 ? 1 : 0));
								scale = (byte)Scale(dec);
							}
						}
						break;

					case ProviderType.Date:
						if (!alt)
							dbms = DBMS_TYPE_ADATE;
						else
						{
							type = ProviderType.DateTime;
							dbms = DBMS_TYPE_IDATE;
						}
						break;

					case ProviderType.Time:
						if (!alt)
							dbms = conn.osql_dates ? DBMS_TYPE_TMWO : DBMS_TYPE_TMTZ;
						else
						{
							type = ProviderType.DateTime;
							dbms = DBMS_TYPE_IDATE;
						}
						break;

					case ProviderType.IngresDate:
						type = ProviderType.DateTime;
						dbms = DBMS_TYPE_IDATE;
						break;

					case ProviderType.DateTime:
						if (alt)
							dbms = DBMS_TYPE_IDATE;
						else
						{
							dbms = conn.osql_dates ? DBMS_TYPE_TSWO : DBMS_TYPE_TSTZ;
							int nanos = ((SqlTimestamp)value).getNanos();

							/*
							** Determine scale if non-zero fractional seconds component.
							*/
							if (nanos > 0)
							{
								/*
								** Scale of nano-seconds is 9 but 
								** decreases for each trailing zero.
								*/
								scale = 9;
								String frac = nanos.ToString();

								for (int i = frac.Length - 1; i >= 0; i--)
									if (frac[i] == '0')
										scale--;
									else
										break;
							}
						}
						break;

					case ProviderType.Char:
						/*
						** Determine length in characters.  
						** NULL values assume {n}char(1).
						*/
						if (value.isNull())
							length = 1;
						else if (alt)
							length = ((SqlChar)value).valuelength();
						else
							length = ((SqlNChar)value).valuelength();

						/*
						** Long character arrays are sent as BLOBs.
						** Zero length arrays must be sent as VARCHAR.
						** UCS2 used when supported unless alternate 
						** format is requested.
						*/
						if (length <= 0)
						{
							type = ProviderType.VarChar;
							dbms = alt ? DBMS_TYPE_VARCHAR : DBMS_TYPE_NVARCHAR;
						}
						else if (alt && length <= conn.max_char_len)
							dbms = DBMS_TYPE_CHAR;
						else if (!alt && length <= conn.max_nchr_len)
						{
							dbms = DBMS_TYPE_NCHAR;
							length *= 2;	// Convert to bytes
						}
						else
						{
							type = ProviderType.LongVarChar;
							dbms = alt ? DBMS_TYPE_LONG_CHAR : DBMS_TYPE_LONG_NCHAR;
							length = 0;
						}
						break;

					case ProviderType.VarChar:
						/*
						** Determine length in characters.  
						** NULL values assume {n}varchar(1).
						*/
						if (value.isNull())
							length = 1;
						else if (alt)
							length = ((SqlVarChar)value).valuelength();
						else
							length = ((SqlNVarChar)value).valuelength();

						/*
						** Long strings are sent as BLOBs.  UCS2 used 
						** when supported unless alternate format is 
						** requested.
						*/
						if (alt && length <= conn.max_vchr_len)
							dbms = DBMS_TYPE_VARCHAR;
						else if (!alt && length <= conn.max_nvch_len)
						{
							dbms = DBMS_TYPE_NVARCHAR;
							length *= 2;	// Convert to bytes
						}
						else
						{
							type = ProviderType.LongVarChar;
							dbms = alt ? DBMS_TYPE_LONG_CHAR : DBMS_TYPE_LONG_NCHAR;
							length = 0;
						}
						break;

					case ProviderType.LongVarChar:
						/*
						** UCS2 used when supported unless alternate 
						** format is requested.
						*/
						dbms = alt ? DBMS_TYPE_LONG_CHAR : DBMS_TYPE_LONG_NCHAR;
						length = 0;
						break;

					case ProviderType.Binary:
						/*
						** Determine length.  NULL values assume byte(1).
						*/
						length = value.isNull() ? 1 : ((SqlByte)value).valuelength();

						/*
						** Long arrays are sent as BLOBs.  Zero length 
						** arrays must be sent as VARBINARY.
						*/
						if (length <= 0)
						{
							type = ProviderType.VarBinary;
							dbms = DBMS_TYPE_VARBYTE;
						}
						else if (length <= conn.max_byte_len)
							dbms = DBMS_TYPE_BYTE;
						else
						{
							type = ProviderType.LongVarBinary;
							dbms = DBMS_TYPE_LONG_BYTE;
							length = 0;
						}
						break;

					case ProviderType.VarBinary:
						/*
						** Determine length.  NULL values assume varbyte(1).
						*/
						length = value.isNull() ? 1 : ((SqlVarByte)value).valuelength();

						/*
						** Long arrays are sent as BLOBs.
						*/
						if (length <= conn.max_vbyt_len)
							dbms = DBMS_TYPE_VARBYTE;
						else
						{
							type = ProviderType.LongVarBinary;
							dbms = DBMS_TYPE_LONG_BYTE;
							length = 0;
						}
						break;

					case ProviderType.LongVarBinary:
						dbms = DBMS_TYPE_LONG_BYTE;
						length = 0;
						break;
				}

				/*
				** Translate flags.
				*/
				if (value.isNull()) flags |= MSG_DSC_NULL;

				switch (pflags & PS_FLG_PROC_IO)
				{
					case PS_FLG_PROC_IN: flags |= MSG_DSC_PIN; break;
					case PS_FLG_PROC_IO: flags |= MSG_DSC_PIO; break;

					case PS_FLG_PROC_OUT:
						/*
						** Since a generic NULL value is provided for output
						** parameters, handle as INOUT when OUT isn't supported.
						*/
						flags |= (conn.msg_protocol_level >= MSG_PROTO_5) ? MSG_DSC_POUT
												  : MSG_DSC_PIO;
						break;
				}

				if ((pflags & PS_FLG_PROC_GTT) != 0)
					flags |= MSG_DSC_GTT;

				msg.write((short)type);
				msg.write((short)dbms);
				msg.write((short)length);
				msg.write((byte)prec);
				msg.write((byte)scale);
				msg.write((byte)flags);

				if (paramArray[param].name == null)
					msg.write((short)0);	// no name.
				else
					msg.write(paramArray[param].name);
			}

			msg.done(eog);
			return;
		}  // lock
	} // sendDesc


		/*
		** Name: sendData
		**
		** Description:
		**	Send a DATA message for current set of parameters.
		**
		** Input:
		**	eog	Is this message end-of-group?
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
		**	29-Sep-99 (gordy)
		**	    Implemented support for BLOBs.
		**	12-Nov-99 (gordy)
		**	    Use configured date formatter.
		**	17-Dec-99 (gordy)
		**	    Conversion to BLOB now determined by DBMS max varchar len.
		**	 2-Feb-00 (gordy)
		**	    Send short streams as VARCHAR or VARBINARY.
		**	15-Nov-00 (gordy)
		**	    Extracted Param class to create a stand-alone Paramset class.
		**	 8-Jan-01 (gordy)
		**	    Parameter set passed as parameter to support batch processing.
		**	10-May-01 (gordy)
		**	    Support UCS2 as alternate format for character types.  Char
		**	    arrays fully supported as separate BINARY and VARBINARY types.
		**	 1-Jun-01 (gordy)
		**	    Removed conversion of BLOB to fixed length type for short lengths.
		**	20-Feb-02 (gordy)
		**	    Decimal not supported by gateways at proto level 0.  Send as float.
		**	 6-Sep-02 (gordy)
		**	    Use formatted length of CHAR and VARCHAR parameters rather than
		**	    character length.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.  
		**	19-Feb-03 (gordy)
		**	    Replaced BIT with BOOLEAN.  Skip unset parameters (default
		**	    procedure parameters).  ALT character parameters now stored
		**	    in communication format.
		**	26-Jun-03 (gordy)
		**	    Missed an ALT storage class.
		**	27-June-03 (wansh01)
		**	    Handle binary type with 0 length as varbinary. 	
		**	 1-Dec-03 (gordy)
		**	    Moved to ParamSet to support updatable result sets.
		**	23-Jan-04 (gordy)
		**	    Properly handle zero length NCHAR columns using emptyNVarChar.
		**	    Set NULL CHAR/BINARY columns to non-zero length to avoid coercion
		**	    (zero length values were being sent instead of mulls).
		**	15-Mar-04 (gordy)
		**	    BIGINT now supported.  BOOLEAN should always be in alternate
		**	    storage format (force to alternate until BOOLEAN supported).
		**	28-May-04 (gordy)
		**	    Add max column lengths for NCS columns.
		**	19-Jun-06 (gordy)
		**	    ANSI Date/Time data type support.  Ingres date is now
		**	    the alternate storage for DATE/TIME/TIMESTAMP and
		**	    SqlDate/SqlTime/SqlTimestamp are the primary storage.
		**	 7-Dec-09 (gordy, ported by thoda04)
		**	    Boolean now fully supported.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		public void
		sendData(bool eog)
		{
		lock (this)
		{
			msg.begin(MSG_DATA);

			for (int param = 0; param < param_cnt; param++)
			{
				if (paramArray[param] == null ||
					 (paramArray[param].flags & PS_FLG_SET) == 0)
					continue;		// Skip unset params.

				SqlData value = paramArray[param].value;
				ProviderType type = paramArray[param].type;
				bool alt = ((paramArray[param].flags & PS_FLG_ALT) != 0);
				int length;

				switch (type)
				{
					case ProviderType.DBNull: msg.write((SqlNull)value); break;
					case ProviderType.TinyInt: msg.write((SqlTinyInt)value); break;
					case ProviderType.SmallInt: msg.write((SqlSmallInt)value); break;
					case ProviderType.Integer: msg.write((SqlInt)value); break;
					case ProviderType.Real: msg.write((SqlReal)value); break;
					case ProviderType.Double: msg.write((SqlDouble)value); break;

					case ProviderType.Date:
						if (alt)
							msg.write((IngresDate)value);
						else
							msg.write((SqlDate)value);
						break;

					case ProviderType.Time:
						if (alt)
							msg.write((IngresDate)value);
						else
							msg.write((SqlTime)value);
						break;

					case ProviderType.IngresDate:
					case ProviderType.DateTime:
						if (alt)
							msg.write((IngresDate)value);
						else
							msg.write((SqlTimestamp)value);
						break;

					case ProviderType.Boolean:
						if (alt)
							msg.write((SqlTinyInt)value);
						else
							msg.write((SqlBool)value);
						break;

					case ProviderType.BigInt:
						if (alt)
							msg.write((SqlDouble)value);
						else
							msg.write((SqlBigInt)value);
						break;

					case ProviderType.Decimal:
						if (alt)
							msg.write((SqlDouble)value);
						else
							msg.write((SqlDecimal)value);
						break;

					case ProviderType.Char:
						/*
						** Determine length.  NULL values assume {n}char(1).
						*/
						if (value.isNull())
							length = 1;
						else if (alt)
							length = ((SqlChar)value).valuelength();
						else
							length = ((SqlNChar)value).valuelength();

						/*
						** Zero length strings must be sent as VARCHAR.
						** Long strings are sent as BLOBs.  UCS2 used 
						** when supported.
						*/
						if (alt)
						{
							if (length <= 0)
								msg.write(emptyVarChar);
							else if (length <= conn.max_char_len)
								msg.write((SqlChar)value);
							else
							{
								if (tempLongChar == null)
									tempLongChar = new SqlLongChar(msg.getCharSet());
								tempLongChar.set((SqlChar)value);
								msg.write(tempLongChar);
							}
						}
						else
						{
							if (length <= 0)
								msg.write(emptyNVarChar);
							else if (length <= conn.max_nchr_len)
								msg.write((SqlNChar)value);
							else
							{
								if (tempLongNChar == null)
									tempLongNChar = new SqlLongNChar();
								tempLongNChar.set((SqlNChar)value);
								msg.write(tempLongNChar);
							}
						}
						break;

					case ProviderType.VarChar:
						/*
						** Determine length.  NULL values assume {n}varchar(1).
						*/
						if (value.isNull())
							length = 1;
						else if (alt)
							length = ((SqlVarChar)value).valuelength();
						else
							length = ((SqlNVarChar)value).valuelength();

						/*
						** Long strings are sent as BLOBs.  
						** UCS2 used when supported.
						*/
						if (alt)
						{
							if (length <= conn.max_vchr_len)
								msg.write((SqlVarChar)value);
							else
							{
								if (tempLongChar == null)
									tempLongChar = new SqlLongChar(msg.getCharSet());
								tempLongChar.set((SqlVarChar)value);
								msg.write(tempLongChar);
							}
						}
						else
						{
							if (length <= conn.max_nvch_len)
								msg.write((SqlNVarChar)value);
							else
							{
								if (tempLongNChar == null)
									tempLongNChar = new SqlLongNChar();
								tempLongNChar.set((SqlNVarChar)value);
								msg.write(tempLongNChar);
							}
						}
						break;

					case ProviderType.LongVarChar:
						/*
						** UCS2 used when supported.
						*/
						if (alt)
							msg.write((SqlLongChar)value);
						else
							msg.write((SqlLongNChar)value);
						break;

					case ProviderType.Binary:
						/*
						** Determine length.  NULL values assume byte(1).
						*/
						length = value.isNull() ? 1 : ((SqlByte)value).valuelength();

						/*
						** Long arrays are sent as BLOBs.  Zero length 
						** arrays must be sent as VARBINARY.
						** 
						*/
						if (length <= 0)
							msg.write(emptyVarByte);
						else if (length <= conn.max_byte_len)
							msg.write((SqlByte)value);
						else
						{
							if (tempLongByte == null) tempLongByte = new SqlLongByte();
							tempLongByte.set((SqlByte)value);
							msg.write(tempLongByte);
						}
						break;

					case ProviderType.VarBinary:
						/*
						** Determine length.  NULL values assume varbyte(1).
						*/
						length = value.isNull() ? 1 : ((SqlVarByte)value).valuelength();

						/*
						** Long arrays are sent as BLOBs.
						*/
						if (length <= conn.max_vbyt_len)
							msg.write((SqlVarByte)value);
						else
						{
							if (tempLongByte == null) tempLongByte = new SqlLongByte();
							tempLongByte.set((SqlVarByte)value);
							msg.write(tempLongByte);
						}
						break;

					case ProviderType.LongVarBinary:
						msg.write((SqlLongByte)value);
						break;
				}
			}

			msg.done(eog);
			return;
		}  // lock
		} // sendData


		/*
		** Name: Scale
		**
		** Description:
		**	Get the scale of a decimal number.
		**
		** Input:
		**	decValue	Decimal value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte    	scale of the decimal number (0 to 28).
		**
		** History:
		**	16-Sep-02 (thoda04)
		**	    Created.
		*/

		private byte Scale(decimal decValue)
		{
			int[] decBits = System.Decimal.GetBits(decValue);
			//decBits[0:2] = low, middle, and high 32-bits of 96-bit integer.
			//decBits[3] = sign and scale of the decimal number.
			//	bits  0:15 = zero and are unused
			//	bits 16:23 = scale
			//	bits 24:30 = zero and are unused
			//	bit  31    = sign

			return (byte)(decBits[3] >> 16);
		}


		/*
		** Name: ToByteArrayWithMaxSize
		**
		** Description:
		**	Truncate parameter byte array down to Size maxsize.
		**
		** Input:
		**	byteArray	Byte array to limit.
		**	maxSize		Maximum size (as usually set by IDbDataParameter.Size.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	new byte array, or the original byte[] if no maxsize.
		**
		** History:
		**	 9-Jan-03 (thoda04)
		**	    Created.
		*/
		
		private static byte[] ToByteArrayWithMaxSize(byte[] byteArray, int maxSize)
		{
			if (maxSize > 0  &&  maxSize < byteArray.Length)  //over max size?
			{
				byte[] newArray = new byte[maxSize];
				Array.Copy(byteArray, 0, newArray, 0, maxSize);
				return (newArray);  // return downsized array
			}
			return (byteArray);  // no choppping needed, return original
		}


		/*
		** Name: ToCharArrayWithMaxSize
		**
		** Description:
		**	Truncate parameter char array down to Size maxsize.
		**
		** Input:
		**	charArray	Character array to limit.
		**	maxsize		Maximum size (as usually set by IDbDataParameter.Size).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	char[]	new char array, or the original char[] if no maxsize.
		**
		** History:
		**	 9-Jan-03 (thoda04)
		**	    Created.
		*/
		
		private static char[] ToCharArrayWithMaxSize(char[] charArray, int maxSize)
		{
			if (maxSize > 0  &&  maxSize < charArray.Length)  //over max size?
			{
				char[] newArray = new Char[maxSize];
				Array.Copy(charArray, 0, newArray, 0, maxSize);
				return (newArray);  // return downsized array
			}
			return (charArray);  // no choppping needed, return original
		}

		private static char[] ToCharArrayWithMaxSize(string str, int maxSize)
		{
			if (maxSize > 0  &&  maxSize < str.Length)  //over max size?
				return str.ToCharArray(0, maxSize);
			return str.ToCharArray();   // no choppping needed
		}


		/*
		** Name: ToStringWithMaxSize
		**
		** Description:
		**	Truncate parameter string down to maxsize.
		**
		** Input:
		**	str    	String to limit.
		**	maxSize	Maximum size (as usually set by IDbDataParameter.Size).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	new substring, or the original string if no maxsize.
		**
		** History:
		**	 9-Jan-03 (thoda04)
		**	    Created.
		*/
		
		private static String ToStringWithMaxSize(String str, int maxSize)
		{
			if (maxSize > 0  &&  maxSize < str.Length)  //max size?
			{
				str = str.Substring(0, maxSize);  // chop
			}
			return (str);  // return original string or new substring
		}

		/*
		** Name: Param
		**
		** Description:
		**	Class representing an entry in a parameter data-set.
		**
		**  Public data
		**
		**	type	    SQL type.
		**	value	    SQL data value.
		**	flags	    Flags.
		**	name	    Name.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	11-Jan-01 (gordy)
		**	    This class is Cloneable since parameter object
		**	    values are immutable.
		**	25-Feb-03 (gordy)
		**	    Changed flags to short for additional bits.
		**	 1-Dec-03 (gordy)
		**	    OTHER is now the default type.
		*/

		private class
			Param : Object
		{

			public  ProviderType	type = ProviderType.Other;
			public  SqlData     	value = null;
			public  ushort      	flags = 0;
			public  String      	name = null;


			/*
			** Name: clear
			**
			** Description:
			**	Reset parameter fields to initial settings.
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
			**	15-Nov-00 (gordy)
			**	    Created.
			**	 1-Dec-03 (gordy)
			**	    OTHER is now the default type.
			*/

			public void
				clear()
			{
				type = ProviderType.Other;
				value = null;
				flags = 0;
				name = null;
				return;
			} // clear

			/*
			** Name: set
			**
			** Description:
			**	Set parameter fields.
			**
			** Input:
			**	type	SQL type.
			**	value	SQL data storage.
			**	short	Flags.
			**	name	Name.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	void.
			**
			** History:
			**	 1-Dec-03 (gordy)
			**	    Created.
			*/

			public void
			set( ProviderType type, SqlData value, ushort flags, String name )
			{
				lock(this)
				{
				this.type = type;
				this.value = value;
				this.flags = flags;
				this.name = name;
				return;
				}
			} // set



		} // class Param

	} // class ParamSet
}