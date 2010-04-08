/*
** Copyright (c) 1999, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: advanrsmd.cs
	**
	** Description:
	**	Implements a result set metadata class: AdvanRSMD.
	**
	**  Classes:
	**
	**	AdvanRSMD	ResultSet meta-data.
	**	Desc		Column descriptor.
	**
	** History:
	**	17-May-99 (gordy)
	**	    Created.
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	18-Nov-99 (gordy)
	**	    Made Desc a nested class.
	**	19-Nov-99 (gordy)
	**	    Completed stubbed methods.
	**	25-Aug-00 (gordy)
	**	    Nullable byte converted to flags.
	**	 4-Oct-00 (gordy)
	**	    Standardized internal tracing.
	**	10-Nov-00 (gordy)
	**	    Added support for 2.0 extensions.
	**	28-Mar-01 (gordy)
	**	    Tracing addes as constructor parameter.
	**	16-Apr-01 (gordy)
	**	    Convert constructor to factory method and add companion
	**	    methods to permit optimization of RSMD handling.
	**	10-May-01 (gordy)
	**	    Added support for UCS2 datatypes.
	**	22-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	20-Mar-03 (gordy)
	**	    Updated to 3.0.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	16-Jun-06 (gordy)
	**	    ANSI Date/Time data type support.
	**	25-Aug-06 (gordy)
	**	    Support parameter modes.
	**	21-Mar-07 (thoda04)
	**	    Added IntervalDayToSecond, IntervalYearToMonth.
	**	 7-Dec-09 (gordy, ported by thoda04)
	**	    Boolean support.
	**	 3-Mar-10 (thoda04)  SIR 123368
	**	    Added support for IngresType.IngresDate parameter data type.
	*/


	/*
	** Name: AdvanRSMD
	**
	** Description:
	**	Driver class which implements the
	**	ResultSetMetaData interface.
	**
	**  Interface Methods:
	**
	**	getColumnCount	    Returns the number of columns in the result set.
	**	isAutoIncrement	    Determine if column is automatically numbered.
	**	isCaseSensitive	    Determine if column is case sensitive.
	**	isSearchable	    Determine if column may be used in a where clause.
	**	isCurrency	    Determine if column is a cash value.
	**	isNullable	    Determine if column is nullable.
	**	isSigned	    Determine if column is a signed number.
	**	getColumnDisplaySize	Returns the display width of a column.
	**	getColumnLabel	    Returns the title for a column.
	**	getColumnName	    Returns the name for a column.
	**	getSchemaName	    Returns the schema name in which the column resides.
	**	getPrecision	    Returns the number of digits for a numeric column.
	**	getScale	    Returns the number of digits following decimal point.
	**	getTableName	    Returns the table name in which the column resides.
	**	getColumnType	    Returns the SQL type for a column.
	**	getColumnTypeName   Returns the host DBMS type for a column.
	**	isReadOnly	    Determine if a column is read-only.
	**	isWritable	    Determine if a column may be updated.
	**	isDefinitelyWritable	Determine if a column update will always succeed.
	**	getColumnClassName  Returns the class name of object from getObject().
	**
	**  Public Data:
	**
	**	count		    Number of columns in the result set.
	**	desc		    Column descriptors.
	**
	**  Public Methods:
	**
	**	load		    Read descriptor from connection, generate RSMD.
	**	reload		    Read descriptor from connection, update RSMD.
	**	setColumnInfo	    Set Column Information.
	**	getParameterMode    Returns the parameter mode for a column.
	**
	**  Private Data:
	**
	**	trace		    Tracing.
	**	title		    Class title for tracing.
	**	tr_id		    Tracing ID.
	**	inst_count	    Number of instances.
	**
	**  Private Methods:
	**
	**	resize		    Set number of column descriptors.
	**	columnMap	    Map an extern column index to the internal index.
	**
	** History:
	**	17-May-99 (gordy)
	**	    Created.
	**	18-Nov-99 (gordy)
	**	    Made Desc a nested class.
	**	 4-Oct-00 (gordy)
	**	    Added instance count, inst_count, for standardized internal tracing.
	**	10-Nov-00 (gordy)
	**	    Support 2.0 extensions.  Added getColumnClassName().
	**	16-Apr-01 (gordy)
	**	    Converted constructor with DbConn to load() factory method
	**	    and added companion reload() method to re-read Result-set
	**	    Meta-data into existing RSMD.  A re-read should not change
	**	    the number of columns, but added resize() method to be safe.
	**	31-Oct-02 (gordy)
	**	    Changed package fields to public (protection controlled by
	**	    package access on class).  Made reload() an instance method
	**	    and made resize() private.
	**	25-Aug-06 (gordy)
	**	    Added getParameterMode().
	*/

	internal class AdvanRSMD : MsgConst
	{

		/*
		** The following members are public to provide easy access 
		** to the meta-data to other parts of the driver.
		*/
		public short  count = 0; // Number of descriptors.
		public Desc[] desc;      // Descriptors.
		
		private ITrace trace = null;
		private String title = null;
		private String tr_id = null;
		private static int inst_count = 0;


		/*
		** Name: load
		**
		** Description:
		**	Reads the result-set descriptor (MSG_DESC) from the server
		**	connection and builds an ResultSet meta-data object.
		**	
		**
		** Input:
		**	conn	    Database Connection
		**
		** Output:
		**	None.
		**
		** Returns:
		**	AdvanRSMD    Result-set Meta-data.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		**	25-Aug-00 (gordy)
		**	    Nullable byte converted to flags.
		**	 4-Oct-00 (gordy)
		**	    Create unique ID for standardized internal tracing.
		**	28-Mar-01 (gordy)
		**	    Tracing added as a parameter.
		**	16-Apr-01 (gordy)
		**	    Converted from constructor to factory method.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		**	16-Jun-06 (gordy)
		**	    ANSI Date/Time data type support.  Map intervals to varchar.
		**	25-Aug-06 (gordy)
		**	    Save flags to make parameter modes available.
		*/

		public static AdvanRSMD
		load(DrvConn conn)
		{
			MsgConn msg = conn.msg;
			bool isBlankDateNull = conn.isBlankDateNull;

			short count = msg.readShort();
			AdvanRSMD rsmd = new AdvanRSMD(count, conn.trace);

			for (short col = 0; col < count; col++)
			{
				ProviderType sql_type = (ProviderType)
					msg.readShort();
				short dbms_type       = msg.readShort();
				short length          = msg.readShort();
				byte precision        = msg.readByte();
				byte scale            = msg.readByte();
				byte flags            = msg.readByte();
				if (isBlankDateNull && sql_type == ProviderType.DateTime)
					flags |= MSG_DSC_NULL;  // return empty date as null
				String name           = msg.readString();

				switch (sql_type)
				{
					case ProviderType.DateTime:
						if (dbms_type == 0) dbms_type = DBMS_TYPE_IDATE;
						break;

					case ProviderType.Interval:
						/*
						** Intervals are not supported directly
						*/
						if (dbms_type == DBMS_TYPE_INTDS)
							sql_type = ProviderType.IntervalDayToSecond;
						else
							sql_type = ProviderType.VarChar;
						length = (short)((dbms_type == DBMS_TYPE_INTYM) ? 8 : 15);
						precision = scale = 0;
						break;
				}

				rsmd.desc[col].name = name;
				rsmd.desc[col].sql_type = sql_type;
				rsmd.desc[col].dbms_type = dbms_type;
				rsmd.desc[col].length = length;
				rsmd.desc[col].precision = precision;
				rsmd.desc[col].scale = scale;
				rsmd.desc[col].flags = flags;
			}

			return (rsmd);
		} // load


		/*
		** Name: AdvanRSMD
		**
		** Description:
		**	Class constructor.  Allocates descriptors as requested
		**	by the caller.  It is the callers responsibility to
		**	populate the descriptor information.
		**	
		**
		** Input:
		**	count	Number of descriptors.
		**	trace	Connection tracing.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		**	28-Mar-01 (gordy)
		**	    Tracing added as a parameter.*/

		public AdvanRSMD(short count, ITrace trace)
		{
			this.count = count;
			this.trace = trace;

			desc = new Desc[count];
			for (int col = 0; col < count; col++)  desc[col] = new Desc(0);

			title = DrvConst.shortTitle + "-RSMD[" + inst_count + "]";
			tr_id = "RSMD[" + inst_count + "]";
			inst_count++;
			return;
		} // AdvanRSMD


		/*
		** Name: toString
		**
		** Description:
		**	Return the formatted name of this object.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Object name.
		**
		** History:
		**	 7-Nov-00 (gordy)
		**	    Created.
		*/
		
		public virtual String
			toString()
		{
			return (title);
		}


		/*
		** Name: reload
		**
		** Description:
		**	Reads a result-set descriptor from a database connection
		**	and loads an existing RSMD with the info.
		**
		** Input:
		**	conn	Database connection.
		**
		** Output:
		**	rsmd	Result-set Meta-data.
		**
		** Returns:
		**	void.
		**
		** History:
		**	16-Apr-01 (gordy)
		**	    Created.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.  Removed RSMD parameter and
		**	    changed to reload current object (instance method rather than 
		**	    static).
		**	16-Jun-06 (gordy)
		**	    ANSI Date/Time data type support.  Map intervals to varchar.
		**	25-Aug-06 (gordy)
		**	    Save flags to make parameter modes available.
		*/

		public void
		reload(DrvConn conn)
		{
			MsgConn msg = conn.msg;

			bool isBlankDateNull = conn.isBlankDateNull;

			short new_count = msg.readShort();
			short common = (short) System.Math.Min(new_count, count);
			
			if (new_count != count) resize(new_count);
			
			for (short col = 0; col < count; col++)
			{
				ProviderType sql_type = (ProviderType)msg.readShort();
				short dbms_type       = msg.readShort();
				short length          = msg.readShort();
				byte  precision       = msg.readByte();
				byte  scale           = msg.readByte();
				byte flags            = msg.readByte();
				if (isBlankDateNull && sql_type == ProviderType.DateTime)
					flags |= MSG_DSC_NULL;  // return empty date as null
				String name = msg.readString();

				switch (sql_type)
				{
					case ProviderType.DateTime:
						if (dbms_type == 0) dbms_type = DBMS_TYPE_IDATE;
						break;

					case ProviderType.Interval:
						/*
						** Intervals are not supported directly
						*/
						if (dbms_type == DBMS_TYPE_INTDS)
							sql_type = ProviderType.IntervalDayToSecond;
						else
							sql_type = ProviderType.VarChar;
						length = (short)((dbms_type == DBMS_TYPE_INTYM) ? 8 : 15);
						precision = scale = 0;
						break;
				}

				if (col < common && trace.enabled(5))
				{
					if (sql_type != desc[col].sql_type)
						trace.write(tr_id + ": reload[" + col + "] sql_type " +
							desc[col].sql_type + " => " + sql_type);
							
					
					if (dbms_type != desc[col].dbms_type)
						trace.write(tr_id + ": reload[" + col + "] dbms_type " +
							desc[col].dbms_type + " => " + dbms_type);
							
					
					if (length != desc[col].length)
						trace.write(tr_id + ": reload[" + col + "] length " +
							desc[col].length + " => " + length);
							
					
					if (precision != desc[col].precision)
						trace.write(tr_id + ": reload[" + col + "] precision " +
							desc[col].precision + " => " + precision);
							
					
					if (scale != desc[col].scale)
						trace.write(tr_id + ": reload[" + col + "] scale " +
							desc[col].scale + " => " + scale);


					if (flags != desc[col].flags)
						trace.write(tr_id + ": reload[" + col + "] flags " +
								 desc[col].flags + " => " + flags);
							
				}
				
				desc[ col ].name = name;
				desc[ col ].sql_type = sql_type;
				desc[ col ].dbms_type = dbms_type;
				desc[ col ].length = length;
				desc[ col ].precision = precision;
				desc[ col ].scale = scale;
				desc[ col ].flags = flags;
			}
			
			return ;
		}


		/*
		** Name: setColumnInfo
		**
		** Description:
		**	Set the descriptor information for a column.
		** 
		** Input:
		**	name        Name of column.
		**	idx         Column index;
		**	sql_type    SQL Type.
		**	dbms_type   DBMS Type.
		**	length      Length.
		**	precision   Precision.
		**	scale       Scale.
		**	nullable    Nullable?
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		**	25-Aug-06 (gordy)
		**	    Set NULL flag when nullable.
		*/

		internal virtual void
		setColumnInfo
		(
			String          name,
			int             idx, 
			ProviderType    sql_type,
			short           dbms_type,
			short           length, 
			byte            precision,
			byte            scale,
			bool            nullable
		)
		{
			idx = columnMap(idx);
			desc[idx].name = name;
			desc[idx].sql_type = sql_type;
			desc[idx].dbms_type = dbms_type;
			desc[idx].length = length;
			desc[idx].precision = precision;
			desc[idx].scale = scale;
			desc[idx].flags = (byte)(nullable ? MSG_DSC_NULL : (byte)0);
			return;
		}


		/*
		** Name: getColumnCount
		**
		** Description:
		**	Returns the number of columns in the associated result set.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	The count of columns in the result set.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public int
		getColumnCount()
		{
			if (trace.enabled())  trace.log(title + ".getColumnCount: " + count);
			return (count);
		} // getColumnCount
		
		/*
		** Name: getColumnName
		**
		** Description:
		**	Returns the name of the column.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Column name.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public String
		getColumnName(int column)
		{
			if (trace.enabled())
				trace.log(title + ".getColumnName( " + column + " )");
			column = columnMap(column);
			if (trace.enabled())
				trace.log(title + ".getColumnName: " + desc[column].name);
			return (desc[column].name);
		} // getColumnName
		
		/*
		** Name: getColumnLabel
		**
		** Description:
		**	Returns the title of the column.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Column title.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public String
		getColumnLabel(int column)
		{
			if (trace.enabled())
				trace.log(title + ".getColumnLabel( " + column + " )");
			column = columnMap(column);
			if (trace.enabled())
				trace.log(title + ".getColumnLabel: " + desc[column].name);
			return (desc[column].name);
		} // getColumnLabel
		
		/*
		** Name: getTableName
		**
		** Description:
		**	Returns the name of the table in which the column resides.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Table name.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public String
		getTableName(int column)
		{
			if (trace.enabled())
				trace.log(title + ".getTableName( " + column + " ): ''");
			column = columnMap(column);
			return ("");
		} // getTableName
		
		/*
		** Name: getSchemaName
		**
		** Description:
		**	Returns the name of the schema in which the column resides.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Schema name.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public String
		getSchemaName(int column)
		{
			if (trace.enabled())
				trace.log(title + ".getSchemaName( " + column + " )");
			column = columnMap(column);
			return ("");
		} // getSchemaName
		
		/*
		** Name: getCatalogName
		**
		** Description:
		**	Returns the name of the catalog in which the column resides.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Catalog name.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
	/* not needed
		public String
		getCatalogName( int column )
		{
			if ( trace.enabled() )  
			trace.log( title + ".getCatalogName( " + column + " )" );
			column = columnMap( column );
			if ( trace.enabled() )  trace.log( title + ".getCatalogName: ''" );
			return( "" );
		} // getCatalogName
	*/

		/*
		** Name: getColumnType
		**
		** Description:
		**	Execute an SQL statement and return an indication as to the
		**	type of the result.
		**
		** Input:
		**	column	    Column index
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Column SQL type (ProviderType).
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public ProviderType
		getColumnType(int column)
		{
			if (trace.enabled())
				trace.log(title + ".getColumnType( " + column + " )");
			column = columnMap(column);
			if (trace.enabled())
				trace.log(title + ".getColumnType: " + desc[column].sql_type);
			return ((ProviderType)desc[column].sql_type);
		} // getColumnType
		
		/*
		** Name: getColumnTypeName
		**
		** Description:
		**	Returns the host DBMS type for a column.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    DBMS type.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public String
		getColumnTypeName(int column)
		{
			String type = null;
			
			if (trace.enabled())
				trace.log(title + ".getColumnTypeName( " + column + " )");
			column = columnMap(column);
			
			switch (desc[column].dbms_type)
			{
				case DBMS_TYPE_INT: 
					type = IdMap.get(desc[column].length, intMap);
					break;

				case DBMS_TYPE_FLOAT: 
					type = IdMap.get(desc[column].length, floatMap);
					break;

				default: 
					type = IdMap.get(desc[column].dbms_type, typeMap);
					break;
			}
			
			if (trace.enabled())  trace.log(title + ".getColumnTypeName: " + type);
			return (type);
		} // getColumnTypeName
		
		
		/*
		** Name: getColumnClassName
		**
		** Description:
		**	Returns the class name of the object returned by
		**	getObject() for a column.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Class name.
		**
		** History:
		**	10-Nov-00 (gordy)
		**	    Created.
		*/
		
		public virtual String
		getColumnClassName(int column)
		{
			String name;
			System.Type oc = typeof(System.Object);
			
			if (trace.enabled())
				trace.log(title + ".getColumnClassName( " + column + " )");
			
			switch (desc[(column = columnMap(column))].sql_type)
			{
				case ProviderType.Boolean: 
					oc = typeof(System.Boolean); break;

				case ProviderType.TinyInt:
				case ProviderType.SmallInt:
				case ProviderType.Integer: 
					oc = typeof(System.Int32); break;

				case ProviderType.BigInt: 
					oc = typeof(System.Int64); break;

				case ProviderType.Single: 
					oc = typeof(System.Single); break;

				case ProviderType.Double: 
					oc = typeof(System.Double); break;

				case ProviderType.Numeric:
				case ProviderType.Decimal: 
					oc = typeof(System.Decimal); break;

				case ProviderType.Char:
				case ProviderType.VarChar:
				case ProviderType.LongVarChar:
					oc = typeof(String); break;
					

				case ProviderType.Binary:
				case ProviderType.VarBinary:
				case ProviderType.LongVarBinary: 
					oc = typeof(System.Byte); break;

				
				case ProviderType.Date:
				case ProviderType.Time:
				case ProviderType.DateTime: 
					oc = typeof(System.DateTime); break;

				default: 
					if (trace.enabled(1))
						trace.write(title + ": invalid SQL type " + desc[column].sql_type);
					throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
				
			}
			
			name = oc.FullName;
			if (trace.enabled())  trace.log(title + ".getColumnClassName: " + name);
			return (name);
		} // getColumnClassName
		
		/*
		** Name: getPrecision
		**
		** Description:
		**	Returns the number of decimal digits in a numeric column.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Number of numeric digits.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		**	25-Jan-00 (rajus01)
		**	    Added CHAR,VARCHAR,BINARY,VARBINARY case.
		**	10-May-01 (gordy)
		**	    Added support for UCS2 datatypes.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/
		
		public int
		getPrecision(int column)
		{
			if (trace.enabled())
				trace.log(title + ".getPrecision( " + column + " )");
			column = columnMap(column);
			int precision = 0;
			
			switch (desc[column].sql_type)
			{
				case ProviderType.TinyInt: 
					precision = 3; break;
				
				case ProviderType.SmallInt: 
					precision = 5; break;
				
				case ProviderType.Integer: 
					precision = 10; break;
				
				case ProviderType.BigInt: 
					precision = 19; break;
				
				case ProviderType.DateTime:
				case ProviderType.IngresDate:
					precision = 19; break;
				
				case ProviderType.Single: 
					precision = 7; break;
				
				case ProviderType.Double:
					precision = 15; break;
				
				case ProviderType.Numeric:
				case ProviderType.Decimal: 
					precision = desc[column].precision; break;
				
				case ProviderType.Binary:
				case ProviderType.VarBinary: 
					precision = desc[column].length; break;
					
				
				
				case ProviderType.Char: 
					precision = desc[column].length;
					if (desc[column].dbms_type == DBMS_TYPE_NCHAR)
						precision /= 2;
					break;
					
				
				
				case ProviderType.VarChar: 
					precision = desc[column].length;
					if (desc[column].dbms_type == DBMS_TYPE_NVARCHAR)
						precision /= 2;
					break;
				
			}
			
			if (trace.enabled())  trace.log(title + ".getPrecision: " + precision);
			return (precision);
		} // getPrecision
		
		/*
		** Name: getScale
		**
		** Description:
		**	Returns the number of numeric digits following the decimal point.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Number of scale digits.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public int
		getScale(int column)
		{
			if (trace.enabled())  trace.log(title + ".getScale( " + column + " )");
			column = columnMap(column);
			if (trace.enabled())
				trace.log(title + ".getScale: " + desc[column].scale);
			return (desc[column].scale);
		} // getScale

		/*
		** Name: getColumnDisplaySize
		**
		** Description:
		**	Returns the display width of a column.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Column display width.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		**	10-May-01 (gordy)
		**	    Added support for UCS2 datatypes.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		public int
		getColumnDisplaySize(int column)
		{
			if (trace.enabled())
				trace.log(title + ".getColumnDisplaySize( " + column + " )");
			column = columnMap(column);
			int len = 0;
			
			switch (desc[column].sql_type)
			{
				case ProviderType.Boolean: 
					len = 5; break;
				
				case ProviderType.TinyInt: 
					len = 4; break;
				
				case ProviderType.SmallInt: 
					len = 6; break;
				
				case ProviderType.Integer: 
					len = 11; break;
				
				case ProviderType.BigInt: 
					len = 20; break;
				
				case ProviderType.Single: 
					len = 14; break;
				
				case ProviderType.Double: 
					len = 24; break;
				
				case ProviderType.Date:
					len = 10; break;
				
				case ProviderType.Time:
					len = 8; break;
				
				case ProviderType.DateTime:
				case ProviderType.IngresDate:
					len = 26; break;
				
				case ProviderType.Binary:
				case ProviderType.VarBinary: 
					len = desc[column].length; break;
				
				case ProviderType.LongVarChar:
				case ProviderType.LongVarBinary: 
					len = 0; break; // what should this be?
					
				
				
				case ProviderType.Numeric:
				case ProviderType.Decimal: 
					len = desc[column].precision + 1;
					if (desc[column].scale > 0)  len++;
					break;
					
				
				
				case ProviderType.Char: 
					len = desc[column].length;
					if (desc[column].dbms_type == DbmsConst.DBMS_TYPE_NCHAR)
						len /= 2;
					break;
					
				
				
				case ProviderType.VarChar: 
					len = desc[column].length;
					if (desc[column].dbms_type == DbmsConst.DBMS_TYPE_NVARCHAR)
						len /= 2;
					break;
				
			}
			
			if (trace.enabled())
				trace.log(title + ".getColumnDisplaySize: " + len);
			return (len);
		}

		/*
		** Name: getColumnSizeMax
		**
		** Description:
		**	Returns the maximum width of a column.  For a fixed-length
		**	data type, the size of the data type.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Column display width.
		**
		** History:
		**	 5-Nov-02 (thoda04)
		**	    Created.
		**	 3-Mar-10 (thoda04)  SIR 123368
		**	    Added support for IngresType.IngresDate parameter data type.
		*/

		public int getColumnSizeMax(int column)
		{
			if (trace.enabled())
				trace.log(title + ".getColumnSizeMax( " + column + " )");
			column = columnMap(column);
			int len = 0;
			
			switch (desc[column].sql_type)
			{
				case ProviderType.Boolean: 
					len = 1; break;
				
				case ProviderType.TinyInt: 
					len = 1; break;
				
				case ProviderType.SmallInt: 
					len = 2; break;
				
				case ProviderType.Integer: 
					len = 4; break;
				
				case ProviderType.BigInt: 
					len = 8; break;
				
				case ProviderType.Single: 
					len = 4; break;
				
				case ProviderType.Double: 
					len = 8; break;
				
				case ProviderType.Date: 
					len = 10; break;
				
				case ProviderType.Time: 
					len = 8; break;
				
				case ProviderType.DateTime:
				case ProviderType.IngresDate:
					len = 16; break;
				
				case ProviderType.Binary:
				case ProviderType.VarBinary: 
					len = desc[column].length; break;
				
				case ProviderType.LongVarChar:
				case ProviderType.LongVarBinary: 
					if (desc[column].dbms_type == DBMS_TYPE_LONG_NCHAR)
						len = 1073741823;
					else
						len = 2147483647;
					break;
				
				
				case ProviderType.Numeric:
				case ProviderType.Decimal: 
					len = 19;
					break;
					
				
				
				case ProviderType.Char: 
					len = desc[column].length;
					if (desc[column].dbms_type == DBMS_TYPE_NCHAR)
						len /= 2;
					break;
					
				
				
				case ProviderType.VarChar: 
					len = desc[column].length;
					if (desc[column].dbms_type == DBMS_TYPE_NVARCHAR)
						len /= 2;
					break;
				
			}
			
			if (trace.enabled())
				trace.log(title + ".getColumnSizeMax: " + len);
			return (len);
		}


		/*
		** Name: isNullable
		**
		** Description:
		**	Determine if column is nullable.  Returns one of the following:
		**
		**	    columnNoNulls
		**	    columnNullable
		**	    columnNullableUnknown
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Column nullability.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		**	25-Aug-06 (gordy)
		**	    Check for NULL flag.
		*/

		public bool
		isNullable(int column)
		{
			if (trace.enabled())  trace.log(title + ".isNullable( " + column + " )");

			column = columnMap(column);
			bool nulls = (desc[column].flags & MSG_DSC_NULL)!=0 ? true : false;

			if (trace.enabled())
				trace.log(title + ".isNullable: " + (nulls?
					"columnNullable":
					"columnNoNulls"));
			return (nulls);
		} // isNullable
		
		/*
		** Name: isSearchable
		**
		** Description:
		**	Determine if column may be used in a where clause.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if column is searchable, false otherwise.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public bool
		isSearchable(int column)
		{
			if (trace.enabled())
				trace.log(title + ".isSearchable( " + column + " ): " + true);
			column = columnMap(column);
			return (true);
		} // isSearchable
		
		/*
		** Name: isCaseSensitive
		**
		** Description:
		**	Determine if column is case sensitive.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if column is case sensitive, false otherwise.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public bool
		isCaseSensitive(int column)
		{
			if (trace.enabled())
				trace.log(title + ".isCaseSensitive( " + column + " ): " + true);
			column = columnMap(column);
			bool sensitive = false;
			
			switch (desc[column].sql_type)
			{
				case ProviderType.Char:
				case ProviderType.VarChar:
				case ProviderType.LongVarChar: 
					sensitive = true;
					break;
				
			}
			
			if (trace.enabled())  trace.log(title + ".isCaseSensitive: " + sensitive);
			return (sensitive);
		} // isCaseSensitive

		/*
		** Name: isSigned
		**
		** Description:
		**	Determine if column is a signed numeric.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if column is a signed numeric, false otherwise.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		**	 7-Dec-09 (thoda04)
		**	    Use ProviderTypeMgr to get isSigned capability
		**	    rather than duplicating code.
		*/

		public bool
		isSigned(int column)
		{
			if (trace.enabled())  trace.log(title + ".isSigned( " + column + " )");
			column = columnMap(column);

			bool signed;
			signed = ProviderTypeMgr.isSigned(desc[column].sql_type);

			if (trace.enabled())  trace.log(title + ".isSigned: " + signed);
			return (signed);
		} // isSigned
		
		/*
		** Name: isCurrency
		**
		** Description:
		**	Determine if column is a money value.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if column a money type, false otherwise.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public bool
		isCurrency(int column)
		{
			if (trace.enabled())  trace.log(title + ".isCurrency( " + column + " )");

			column = columnMap(column);
			bool money = (desc[column].dbms_type == DBMS_TYPE_MONEY);

			if (trace.enabled())  trace.log(title + ".isCurrency: " + money);
			return (money);
		} // isCurrency
		
		/*
		** Name: isAutoIncrement
		**
		** Description:
		**	Determine if column is automatically numbered.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if column is automatically numbered, false otherwise.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public bool
		isAutoIncrement(int column)
		{
			if (trace.enabled())
				trace.log(title + ".isAutoIncrement( " + column + " )");
			column = columnMap(column);
			if (trace.enabled()) trace.log(title + ".isAutoIncrement: " + false);
			return (false);
		} // isAutoIncrement
		
		/*
		** Name: isReadOnly
		**
		** Description:
		**	Determine if column is read-only.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if column is read-only, false otherwise.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public bool
		isReadOnly(int column)
		{
			if (trace.enabled()) trace.log(title + ".isReadOnly( " + column + " )");
			column = columnMap(column);
			if (trace.enabled()) trace.log(title + ".isReadOnly: " + false);
			return (false);
		} // isReadOnly
		
		/*
		** Name: isWritable
		**
		** Description:
		**	Determine if column may be updated.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if column may be updated, false otherwise.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public bool
		isWritable(int column)
		{
			if (trace.enabled()) trace.log(title + ".isWritable( " + column + " )");
			column = columnMap(column);
			if (trace.enabled()) trace.log(title + ".isWritable: " + true);
			return (true);
		} // isWritable
		
		/*
		** Name: isDefinitelyWritable
		**
		** Description:
		**	Determine if column update will always succeed.
		**
		** Input:
		**	column	    Column index.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if column can be updated, false otherwise.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		public bool
		isDefinitelyWritable(int column)
		{
			if (trace.enabled())
				trace.log(title + ".isDefinitelyWritable( " + column + " )");
			column = columnMap(column);
			if (trace.enabled()) trace.log(title + ".isDefinitelyWritable: " + true);
			return (true);
		} // isDefinitelyWritable


		/*
		** Name: getParameterMode
		**
		** Description:
		**	Returns the parameter mode of a column.
		**	Returns one of the following values:
		**
		**	    parameterModeIn
		**	    parameterModeOut
		**	    parameterModeInOut
		**	    parameterModeUnknown
		**
		** Input:
		**	column	Column index
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Parameter mode.
		**
		** History:
		**	25-Aug-06 (gordy)
		**	    Created.
		*/

		public System.Data.ParameterDirection
		getParameterMode(int column)
		{
			if (trace.enabled())
				trace.log(title + ".getParameterMode( " + column + " )");

			column = columnMap(column);
			System.Data.ParameterDirection mode =
				System.Data.ParameterDirection.Input;

			if ((desc[column].flags & MSG_DSC_PIO) != 0)
				mode = System.Data.ParameterDirection.InputOutput;
			else if ((desc[column].flags & MSG_DSC_PIN) != 0)
			{
				/*
				** According to the DAM protocol, IN/OUT parameters
				** are indicated by the PIO flag.  Just to be safe,
				** IN/OUT mode is also set if both the PIN and POUT 
				** flags are set.
				*/
				if ((desc[column].flags & MSG_DSC_POUT) != 0)
					mode = System.Data.ParameterDirection.InputOutput;
				else
					mode = System.Data.ParameterDirection.Output;
			}
			else if ((desc[column].flags & MSG_DSC_POUT) != 0)
				mode = System.Data.ParameterDirection.Output;

			if (trace.enabled())
				trace.log(title + ".getParameterMode: " + mode);
			return (mode);
		} // getParameterMode


		/*
		** Name: resize
		**
		** Description:
		**	Adjusts the number of column descriptors.  Where possible,
		**	the original column descriptor information is retained.
		**
		** Input:
		**	count	Number of descriptors.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	16-Apr-01 (gordy)
		**	    Created.
		**	31-Oct-02 (gordy)
		**	    Made private.
		*/
		
		private void
		resize(short count)
		{
			if (count == this.count)  return ;
			Desc[] desc = new Desc[count];
			
			if (trace.enabled(4))
				trace.write(tr_id + ": resize " + this.count + " to " + count);
			
			 for (short col = 0; col < count; col++)
				if (col < this.count)
					desc[col] = this.desc[col];
				else
					desc[col] = new Desc(0);
			
			this.count = count;
			this.desc = desc;
			return ;
		} // resize
		
		
		/*
		** Name: columnMap
		**
		** Description:
		**	Determines if a given column index is a part of the result
		**	set and maps the external index to the internal index.
		**
		** Input:
		**	index	    External column index [1,count].
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Internal column index [0,count - 1].
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		*/
		
		private int
		columnMap(int index)
		{
			if (index < 0 || index >= count)
				throw SqlEx.get( ERR_GC4011_INDEX_RANGE );
			return (index);
		} // columnMap


		/*
		** Name: Desc
		**
		** Description:
		**	Represents a column in a result-set.
		**
		** History:
		**	17-May-99 (gordy)
		**	    Created.
		**	25-Aug-06 (gordy)
		**	    Changed nullable to flags to hold parameter modes.
		*/

		internal struct Desc
		{
			internal ProviderType  sql_type;
			internal short         dbms_type;
			internal short         length;
			internal byte          precision;
			internal byte          scale;
			internal byte          flags;
//				MSG_DSC_NULL
//				MSG_DSC_PIN
//				MSG_DSC_PIO
//				MSG_DSC_GTT
//				MSG_DSC_POUT
			internal String        name;

			internal Desc(ProviderType sql_type)
			{
				this.sql_type  = sql_type;
				this.dbms_type = 0;
				this.length    = 0;
				this.precision = 0;
				this.scale     = 0;
				this.flags     = 0;
				this.name      = null;
			}
		} // struct Desc
	} // class AdvanRSMD
}