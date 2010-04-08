/*
** Copyright (c) 2002, 2010 Ingres Corporation. All Rights Reserved.
*/

#define DBDATAREADER_ENABLED

using System;
using System.ComponentModel;
using System.Collections;
using System.Data;
using System.Globalization;
using Ingres.Utility;
using Ingres.ProviderInternals;

/*
** Name: datareader.cs
**
** Description:
**	Implements the .NET Provider connection to a database.
**
** Classes:
**	IngresDataReader	Low-level result set data access class.
**
** History:
**	27-Aug-02 (thoda04)
**	    Created.
**	24-Feb-04 (thoda04)
**	    Reworked SqlEx and IngresException to avoid forcing the user
**	    to add a reference to their application to reference the
**	    another assembly to resolve the SqlEx class at compile time.
**	    We want the application to only reference the Client assembly.
**	27-Feb-04 (thoda04)
**	    Added additional XML Intellisense comments.
**	21-jun-04 (thoda04)
**	    Cleaned up code for Open Source.
**	 3-mar-05 (thoda04)
**	    Added IEnumerable interface support.  Bug 114019.
**	19-Jul-05 (thoda04)
**	    Added DbDataReader base class.
**	11-Apr-06 (thoda04)
**	    If DateTime value is null, return DBNull, not DateTime.Min.  Bug 115983.
**	21-Mar-07 (thoda04)
**	    Added IntervalDayToSecond, IntervalYearToMonth.
**	 7-Dec-09 (thoda04)
**	    Added support for BOOLEAN data type.
**	 3-Mar-10 (thoda04)  SIR 123368
**	    Added support for IngresType.IngresDate parameter data type.
*/

namespace Ingres.Client
{
/// <summary>
/// Provides a means of reading a forward-only stream of rows
/// from an Ingres database.
/// </summary>
//	Allow this type to be visible to COM.
[System.Runtime.InteropServices.ComVisible(true)]
public sealed class IngresDataReader :
#if DBDATAREADER_ENABLED
		System.Data.Common.DbDataReader
#else
		MarshalByRefObject, IDataReader, IDisposable, IDataRecord, IEnumerable
#endif
	
{
	private bool            _isOpen       = false;
	private bool            _isPositioned = false;
	private CommandBehavior _commandBehavior;
	private MetaData        _keyInfoMetaData;  // MetaData for the result set key info


	// Keep track of the results and position
	// within the resultset (starts prior to first record).
	private AdvanRslt _resultset;
	private int       _fieldCount;
	private Object[]  _valueList;     // list of retrieved object from resultset

	// link SqlEx.get processing to the IngresException factory
	private  SqlEx sqlEx = new SqlEx(new IngresExceptionFactory());

	/* 
	 * Keep track of the connection in order to implement the
	 * CommandBehavior.CloseConnection flag. A null reference means
	 * normal behavior (do not automatically close).
	 */
	private IngresConnection _connection; // = null;

	/*
	 * The constructors are marked as internal because an application
	 * can't create an IngresDataReader directly.  It is created only
	 * by executing one of the reader methods in the IngresCommand.
	 */

	internal IngresDataReader(
		AdvanRslt    resultset,
		IngresConnection connection,
		CommandBehavior  commandBehavior,
		int              recordsAffected,
		MetaData         keyInfoMetaData)
	{
		if (resultset == null)  // if no result set, build an empty one
		{
			resultset = connection.advanConnect.CreateEmptyResultSet();
		}
		_resultset   = resultset;
		_fieldCount  = resultset.rsmd.getColumnCount();
		_connection  = connection;
		_valueList   = new Object[_fieldCount];
		_isOpen      = true;
		_commandBehavior = commandBehavior;
		_recordsAffected = recordsAffected;
		_keyInfoMetaData = keyInfoMetaData;
		if (connection != null)
			connection.ActiveDataReader = this;
			// only one active DataReader allowed at a time
		FireInfoMessageEvent();  // call the delegates with any warning messages
	}


	/*
	** Finalize
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Finalize method called by runtime finalizer.
	/// </summary>
	~IngresDataReader() // Finalize code called by runtime finalizer
	{
		Dispose(false);
	}


	/*
	** Dispose
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Release resources used by IngresDataReader.
	/// </summary>
	/// <param name="disposing"></param>
	protected override void Dispose(bool disposing)
	{
		/*if disposing == true  then method called by user code
		  if disposing == false then method called by runtime from inside the
		                            finalizer and we should not reference other 
		                            objects. */
		if (disposing)
		{
			Close();
			_connection  = null;
			_resultset   = null;
		}
	}

	/*
	** IDataReader interface methods/properties.
	*/

	/*
	** Depth property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// The depth of nesting for the current row.
	/// The Ingres data provider always returns a depth of
	/// zero to indicate no nesting of tables.
	/// </summary>
	[Description("The depth of nesting for the current row.  " +
	             "This data provider always returns a depth of " +
	             "zero to indicate no nesting of tables.")]
#if DBDATAREADER_ENABLED
	public override  int Depth
#else
	public           int Depth
#endif
	{
		/*
		** Always return a value of zero to indicate we never nest result sets.
		**/
		get { return 0;  }
	}

	/*
	** HasRows property
	**
	** History:
	**	 6-Nov-03 (thoda04)
	**	    Created.
	*/

	private bool            _hasRows       = false;
	private bool            _hasRowsIsKnown= false;

	/// <summary>
	/// A true/false indicator as to whether the
	/// data reader has one or more rows.
	/// </summary>
	[Description("A true/false indicator as to " +
		 "whether the data reader has one or more rows.")]
#if DBDATAREADER_ENABLED
	public override  bool HasRows
#else
	public           bool HasRows
#endif
	{
		get
		{
			CheckDataReaderIsOpen();

			if ( !_hasRowsIsKnown )  // if not already read 1st row
			{
				_hasRowsIsKnown = true;
				_hasRows = Read( true );  // peek at first row
			}

			return _hasRows;
		}
	}

	/*
	** IsClosed property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// A true/false indicator as to whether the data reader is closed.
	/// </summary>
	[Description("A true/false indicator as to " +
	             "whether the data reader is closed.")]
#if DBDATAREADER_ENABLED
	public override  bool IsClosed
#else
	public           bool IsClosed
#endif
	{
		get  { return !_isOpen; }
	}

	/*
	** RecordsAffected property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	private int _recordsAffected = 0;

	/// <summary>
	/// The number of rows updated, inserted, or
	/// deleted by execution of the SQL statement.
	/// </summary>
	[Description("The number of rows updated, inserted, or " +
	             "deleted by execution of the SQL statement.")]
#if DBDATAREADER_ENABLED
	public override  int RecordsAffected
#else
	public           int RecordsAffected
#endif
	{
		get
		{
			// This property can be called while DataReader is closed.
			return _recordsAffected;
		}
	}

	/*
	** Close method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Close the DataReader.  Close the Connection if
	/// CommandBehavior == CloseConnection.
	/// </summary>
	[Description("Close the DataReader.")]
#if DBDATAREADER_ENABLED
	public override  void Close()
#else
	public           void Close()
#endif
	{
		_isOpen = false;
		if (_resultset != null  && _fieldCount > 0)  // if result present, close it
		{
			try
			{
				_resultset.close();
			}
			catch( SqlEx ex )  { throw ex.createProviderException(); }
			FireInfoMessageEvent();  // call the delegates with any messages
			_resultset = null;
		}
		if (_connection != null)
		{
			_connection.ActiveDataReader = null;
			// If ExecuteReader(CommandBehavior.CloseConnection) was
			// specified then the associated Connection object is closed
			// when the DataReader object is closed.
			if ((_commandBehavior & CommandBehavior.CloseConnection) != 0)
				_connection.Close();
		}

	}

	/*
	** NextResult method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Advance the data reader to the next result set if present.
	/// </summary>
	[Description("Advance the data reader to the next result set if present.")]
#if DBDATAREADER_ENABLED
	public override  bool NextResult()
#else
	public           bool NextResult()
#endif
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed
		_isPositioned = false;    // indicate we need a new Read()

		return false;  // we never have serial result sets
	}

	/*
	** Read method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Advance the data reader to the next row in the result set.
	/// </summary>
	[Description("Advance the data reader to the next row in the result set.")]
#if DBDATAREADER_ENABLED
	public override  bool Read()
#else
	public           bool Read()
#endif
	{
		return Read(false);
	}


	private bool _hasNewRow      = false;
	private bool _alreadyReadRow = false;

	private bool Read(bool peekAhead)
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed

		if (_fieldCount == 0)  // if no real result set then just return EOF
			return false;

		try
		{   // if not already read row due to peeking
			if ( _alreadyReadRow == false )
				_hasNewRow = _resultset.next();  // read row from server
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }

		_alreadyReadRow = peekAhead;  // if peeking, remember we already read row

		if (!_hasRowsIsKnown )  // on 1st read, set the HasRows property
		{
			_hasRowsIsKnown = true;
			_hasRows = _hasNewRow;
		}

		for (int i=0; i<_valueList.Length; i++)  // clear the cached list
			_valueList[i] = null;

		if (!peekAhead)  // if not peeking ahead, then set position
			_isPositioned = _hasNewRow;  // if Read is OK then it is positioned

		FireInfoMessageEvent();  // call the delegates with any warning messages
		return _hasNewRow;
	}

	/*
	** GetSchemaTable method
	**
	** Remarks:
	**	A DataTable object is constructed.  One DataRow for each column in
	**	the result set.  For each DataRow, there is a collection of 
	**	DataColumns ("Columnname", "ColumnSize", etc.) that describe the
	**	metadata of each resultset column.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Returns a DataTable that describes the resultset column metadata.
	/// </summary>
	[Description("Returns a DataTable that describes " +
	             "the resultset column metadata.")]
#if DBDATAREADER_ENABLED
	public override  DataTable GetSchemaTable()
#else
	public           DataTable GetSchemaTable()
#endif
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed

		return this.GetSchemaTable(_keyInfoMetaData);
	}


	/*
	** GetSchemaTable method
	**
	** Remarks:
	**	A DataTable object is constructed.  One DataRow for each column in
	**	the result set.  For each DataRow, there is a collection of 
	**	DataColumns ("Columnname", "ColumnSize", etc.) that describe the
	**	metadata of each resultset column.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Returns a DataTable that describes the resultset column metadata.
	/// </summary>
	private DataTable GetSchemaTable(MetaData keyInfoMetaData)
	{
		try
		{
			int      fieldCount = _resultset.rsmd.getColumnCount();

			if (fieldCount == 0)     // if no real result set then return null
				return null;

			DataTable table = new DataTable("SchemaTable");

			table.Locale = System.Globalization.CultureInfo.InvariantCulture;

			table.Columns.Add("ColumnName",       typeof(String));
			table.Columns.Add("ColumnOrdinal",    typeof(System.Int32));
			table.Columns.Add("ColumnSize",       typeof(System.Int32));
			table.Columns.Add("NumericPrecision", typeof(System.Int16));
			table.Columns.Add("NumericScale",     typeof(System.Int16));
			table.Columns.Add("DataType",         typeof(System.Type));
			table.Columns.Add("ProviderType",     typeof(System.Int32));
			table.Columns.Add("IsLong",           typeof(System.Boolean));
			table.Columns.Add("AllowDBNull",      typeof(System.Boolean));
			table.Columns.Add("IsReadOnly",       typeof(System.Boolean));
			table.Columns.Add("IsRowVersion",     typeof(System.Boolean));
			table.Columns.Add("IsUnique",         typeof(System.Boolean));
			table.Columns.Add("IsKey",            typeof(System.Boolean));
			table.Columns.Add("IsAutoIncrement",  typeof(System.Boolean));
			table.Columns.Add("BaseSchemaName",   typeof(String));
			table.Columns.Add("BaseCatalogName",  typeof(String));
			table.Columns.Add("BaseTableName",    typeof(String));
			table.Columns.Add("BaseColumnName",   typeof(String));

			for (int i = 0; i < fieldCount; i++)
			{
				DataRow row = table.NewRow();

				ProviderType providerType = _resultset.rsmd.getColumnType(i);
				bool hasPrecision  = ProviderTypeMgr.hasPrecision(providerType);
				bool hasScale      = ProviderTypeMgr.hasScale    (providerType);

				row["ColumnName"]      = _resultset.rsmd.getColumnName(i);
				row["ColumnOrdinal"]   = i;
				row["ColumnSize"]      = _resultset.rsmd.getColumnSizeMax(i);

				if (hasPrecision)
					row["NumericPrecision"]= _resultset.rsmd.getPrecision(i);
				else
					row["NumericPrecision"]= DBNull.Value;

				if (hasScale)
					row["NumericScale"]    = _resultset.rsmd.getScale(i);
				else
					row["NumericScale"]    = DBNull.Value;

				row["DataType"]        =   // .NET data type
					ProviderTypeMgr.GetNETType(providerType);

				row["ProviderType"]    = providerType;
				row["IsLong"]          = ProviderTypeMgr.isLong(providerType);
				row["AllowDBNull"]     = _resultset.rsmd.isNullable(i);
				row["IsReadOnly"]      = _resultset.rsmd.isReadOnly(i);
				row["IsRowVersion"]    = false;
				row["IsUnique"]        = false;
				row["IsKey"]           = false;
				row["IsAutoIncrement"] = _resultset.rsmd.isAutoIncrement(i);
				row["BaseSchemaName"]  = DBNull.Value;
				row["BaseCatalogName"] = DBNull.Value;
				row["BaseTableName"]   = DBNull.Value;
				row["BaseColumnName"]  = row["ColumnName"];

				// if CommandBehavior.KeyInfo || CommandBehavior.SchemaOnly
				// was specified then a MetaData object was built with the info.
				if (keyInfoMetaData!=null  &&  i < keyInfoMetaData.Columns.Count)
				{
					MetaData.Column col = (MetaData.Column)keyInfoMetaData.Columns[i];
					if (col.ColumnName != null  &&   // if a column from SELECT * ...
						col.ColumnName.Equals("*"))
					{
						col.ColumnName = 
							_resultset.rsmd.getColumnName(i);
						col.ColumnNameSpecification =
							MetaData.WrapInQuotesIfEmbeddedSpaces(
							col.ColumnName);
						col.CatColumn  = 
							keyInfoMetaData.FindCatalogColumnForColumn(col);
					}

					if (col.CatColumn != null)
					{
						Catalog.Column catCol  = col.CatColumn;
						row["BaseSchemaName"]  = catCol.SchemaName;
						row["BaseTableName"]   = catCol.TableName;
						row["BaseColumnName"]  = catCol.ColumnName;
						row["IsKey"]           = catCol.PrimaryKeySequence != 0?
							true:false;
						row["IsUnique"]        = catCol.IsUnique;
					}
				}

				table.Rows.Add(row);
			}  // end loop

			return table;
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }
	}


	/*
	** IDataRecord interface methods/properties.
	*/

	/*
	** FieldCount property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// The number of columns in the current row.
	/// </summary>
	[Description("The number of columns in the current row.")]
#if DBDATAREADER_ENABLED
	public override  int FieldCount
#else
	public           int FieldCount
#endif
	{
		get
		{
			if (_connection == null)
				throw new NotSupportedException(
					"There is no current connection to the database");
			if (this.IsClosed)
				return 0;

			return _fieldCount;
		}
	}

	/*
	** GetName method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column’s name using a specified ordinal.
	/// </summary>
	[Description("Get the column’s name using a specified ordinal.")]
#if DBDATAREADER_ENABLED
	public override  String GetName(int i)
#else
	public           String GetName(int i)
#endif
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed

		try
		{
			return _resultset.rsmd.getColumnName(i);
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }
	}

	/*
	** GetDataTypeName method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column’s data type name as known in Ingres.
	/// </summary>
	[Description("Get the column’s data type name as known in Ingres.")]
#if DBDATAREADER_ENABLED
	public override  String GetDataTypeName(int i)
#else
	public           String GetDataTypeName(int i)
#endif
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed

		try
		{
			return _resultset.rsmd.getColumnTypeName(i);  // get Ingres type name
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }
	}

	/*
	** GetFieldType method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column’s .NET Type.
	/// </summary>
	[Description("Get the column’s .NET Type.")]
#if DBDATAREADER_ENABLED
	public override  Type GetFieldType(int i)
#else
	public           Type GetFieldType(int i)
#endif
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed

		try
		{
			ProviderType providerType = _resultset.rsmd.getColumnType(i);
			return ProviderTypeMgr.GetNETType(providerType);
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }
	}

	/*
	** GetValue method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	**	11-Apr-06 (thoda04)
	**	    If DateTime value is null, return DBNull, not DateTime.Min.  Bug 115983.
	*/

	/// <summary>
	/// Get the column value in its native format.
	/// </summary>
	[Description("Get the column value in its native format.")]
#if DBDATAREADER_ENABLED
	public override  Object GetValue(int i)
#else
	public           Object GetValue(int i)
#endif
	{
		object obj;

		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed
		CheckDataReaderIsPositioned(); // check that we are positioned on a row

		if (i < 0  ||  i >= _fieldCount)
			throw new IngresException( GcfErr.ERR_GC4011_INDEX_RANGE );

		if (_valueList[i] != null)  // if we already retrieved the object
			return _valueList[i];   // then return it because blob streams
			                        // don't like to be reused.

		try
		{
		ProviderType type = _resultset.rsmd.getColumnType(i);  // check the type first
		if (type == ProviderType.DateTime)   // if a DateTime object
		{
			obj = _resultset.getTimestamp(i); // get as timestamp for right timezone
			if (_resultset.wasNull())         // if null value, return DBNull
				obj = DBNull.Value;
			else
			{
				obj = ((DateTime)obj).ToLocalTime();  // DateTimeKind also set to Local
			}
		}
		else
		{
			obj = _resultset.getObject(i);
			if (_resultset.wasNull())         // if null value, return DBNull
				obj = DBNull.Value;
			else if (type == ProviderType.IntervalDayToSecond  &&  obj is String)
			{
				TimeSpan span;
				if (SqlInterval.getTryTimeSpan((string)obj, out span))
					obj = span;
			}
		}
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }

		_valueList[i] = obj;

		FireInfoMessageEvent();  // call the delegates with any warning messages
		return obj;
	}

	/*
	** GetValues method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the all of column values into an Object array.
	/// </summary>
	[Description("Get the all of column values into an Object array.")]
#if DBDATAREADER_ENABLED
	public override  int GetValues(object[] values)
#else
	public           int GetValues(object[] values)
#endif
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed

		int i = 0;

		for (i = 0; i < values.Length && i < _fieldCount; i++)
		{
			values[i] = this.GetValue(i);
		}

		return i;
	}

	/*
	** GetOrdinal method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column’s ordinal using a specified name.
	/// </summary>
	[Description("Get the column’s ordinal using a specified name.")]
#if DBDATAREADER_ENABLED
	public override  int GetOrdinal(string name)
#else
	public           int GetOrdinal(string name)
#endif
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed

		try
		{
			return _resultset.findColumn(name);
		}
		catch (SqlEx)
		{
			// Throw an exception if the ordinal cannot be found.
			throw new IndexOutOfRangeException(
				"Could not find specified column in results");
		}
		catch (IngresException)
		{
			// Throw an exception if the ordinal cannot be found.
			throw new IndexOutOfRangeException(
				"Could not find specified column in results");
		}
	}

	/*
	** Item property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Gets the column value in its native format for a given column ordinal.
	/// </summary>
	[Description("Gets the column value in its native format for " +
	             "a given column ordinal.")]
#if DBDATAREADER_ENABLED
	public override  object this [ int i ]
#else
	public           object this [ int i ]
#endif
	{
		get
		{
			return this.GetValue(i);
		}
	}

	/// <summary>
	/// Gets the column value in its native format for a given column name.
	/// </summary>
	[Description("Gets the column value in its native format for " +
		 "a given column name.")]
#if DBDATAREADER_ENABLED
	public override  object this [ String name ]
#else
	public           object this [ String name ]
#endif
	{
		// Look up the ordinal and return 
		// the value at that position.
		get
		{
			return this.GetValue(GetOrdinal(name));
		}
	}

	/*
	** GetBoolean method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// et the column value as a Boolean.
	/// </summary>
	[Description("Get the column value as a Boolean.")]
#if DBDATAREADER_ENABLED
	public override  bool GetBoolean(int i)
#else
	public           bool GetBoolean(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (bool)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetByte method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a Byte.
	/// </summary>
	[Description("Get the column value as a Byte.")]
#if DBDATAREADER_ENABLED
	public override  byte GetByte(int i)
#else
	public           byte GetByte(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.

		// One exception: since all other data provider convert to Byte.
		// if SByte then convert to Byte if they are using the GetByte accessor.
		if (obj is SByte)
			obj = unchecked((byte)((SByte)obj));

		if (obj is Byte  ||
			obj is Int16 ||
			obj is Int32 ||
			obj is Int64)
			return Convert.ToByte(obj);

		try { return (byte)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetBytes method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a byte stream into a Byte array.
	/// </summary>
	[Description("Get the column value as a byte stream into a Byte array.")]
#if DBDATAREADER_ENABLED
	public override  long GetBytes(
#else
	public           long GetBytes(
#endif
		int i, long dataIndex, byte[] buffer, int bufferIndex, int length)
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		byte[] bytesValue;
		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { bytesValue = (byte[])obj; } //try getting data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}

		// validate range of index into field
		if (dataIndex < 0  ||  dataIndex >= Math.Max(bytesValue.Length, 1))
			throw new IndexOutOfRangeException("dataIndex");
		int maxDataLength = bytesValue.Length - (int)dataIndex;

		// if buffer is a null reference, return length of data in bytes
		if (buffer == null)
			return bytesValue.Length;

		// if empty string, return 0
		if (bytesValue.Length == 0)
			return 0;

		// validate range of index into buffer
		int lb = buffer.GetLowerBound(0);
		int ub = buffer.GetUpperBound(0);
		if (bufferIndex < lb  ||  bufferIndex > ub)
			throw new IndexOutOfRangeException("bufferIndex");
		int maxBufferLength = ub - bufferIndex + 1;

		if (length > maxBufferLength)
		{
			string msg = String.Format(
				"bufferIndex '{0}' + length '{1}' > buffer.Length '{2}'",
				bufferIndex, length, buffer.Length);
			throw new ArgumentOutOfRangeException(msg);
		}

		if (length > maxDataLength)
			length = maxDataLength;
		if (length == 0)
			return 0;

		Array.Copy(bytesValue, (int)dataIndex, buffer, bufferIndex, length);
		return length;
	}

	/*
	** GetChar method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a Char.
	/// </summary>
	[Description("Get the column value as a Char.")]
#if DBDATAREADER_ENABLED
	public override  char GetChar(int i)
#else
	public           char GetChar(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (char)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetChars method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a character stream into a Char array.
	/// </summary>
	[Description("Get the column value as a character stream into a Char array.")]
#if DBDATAREADER_ENABLED
	public override  long GetChars(
#else
	public           long GetChars(
#endif
		int i, long dataIndex, char[] buffer, int bufferIndex, int length)
{
		object obj = this.GetValue(i);  // check reader is open and get object

		String stringValue;
		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { stringValue = (String)obj; } //try getting data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}

		// validate range of index into field
		if (dataIndex < 0  ||  dataIndex >= Math.Max(stringValue.Length, 1))
			throw new IndexOutOfRangeException("dataIndex");
		int maxDataLength = stringValue.Length - (int)dataIndex;

		// if buffer is a null reference, return length of data in bytes
		if (buffer == null)
			return stringValue.Length;

		// if empty string, return 0
		if (stringValue.Length == 0)
			return 0;

		// validate range of index into buffer
		int lb = buffer.GetLowerBound(0);
		int ub = buffer.GetUpperBound(0);
		if (bufferIndex < lb  ||  bufferIndex > ub)
			throw new IndexOutOfRangeException("bufferIndex");
		int maxBufferLength = ub - bufferIndex + 1;

		if (length > maxBufferLength)
		{
			string msg = String.Format(
				"bufferIndex '{0}' + length '{1}' > buffer.Length '{2}'",
				bufferIndex, length, buffer.Length);
			throw new ArgumentOutOfRangeException(msg);
		}

		if (length > maxDataLength)
			length = maxDataLength;
		if (length == 0)
			return 0;

		stringValue.CopyTo((int)dataIndex, buffer, bufferIndex, length);
		return length;
	}

	/*
	** GetDecimal method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a Decimal.
	/// </summary>
	[Description("Get the column value as a Decimal.")]
#if DBDATAREADER_ENABLED
	public override  Decimal GetDecimal(int i)
#else
	public           Decimal GetDecimal(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (Decimal)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetDateTime method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a DateTime.
	/// </summary>
	[Description("Get the column value as a DateTime.")]
#if DBDATAREADER_ENABLED
	public override  DateTime GetDateTime(int i)
#else
	public           DateTime GetDateTime(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (DateTime)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetData method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get a next nested data table.
	/// </summary>
	[Description("Get a nested data table.")]
#if DBDATAREADER_ENABLED
	public new  IDataReader GetData(int i)
#else
	public      IDataReader GetData(int i)
#endif
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed

		//used to expose nested tables and other hierarchical data.
		return null;
	}

	/*
	** GetGuid method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a Guid.
	/// </summary>
	[Description("Get the column value as a Guid.")]
#if DBDATAREADER_ENABLED
	public override  Guid GetGuid(int i)
#else
	public           Guid GetGuid(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (Guid)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetInt16 method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a Int16.
	/// </summary>
	[Description("Get the column value as a Int16.")]
#if DBDATAREADER_ENABLED
	public override  Int16 GetInt16(int i)
#else
	public           Int16 GetInt16(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (Int16)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException( SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetInt32 method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a Int32.
	/// </summary>
	[Description("Get the column value as a Int32.")]
#if DBDATAREADER_ENABLED
	public override  Int32 GetInt32(int i)
#else
	public           Int32 GetInt32(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (Int32)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetInt64 method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a Int64.
	/// </summary>
	[Description("Get the column value as a Int64.")]
#if DBDATAREADER_ENABLED
	public override  Int64 GetInt64(int i)
#else
	public           Int64 GetInt64(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (Int64)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetFloat method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a float.
	/// </summary>
	[Description("Get the column value as a float.")]
#if DBDATAREADER_ENABLED
	public override  float GetFloat(int i)
#else
	public           float GetFloat(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (float)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetDouble method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a Double.
	/// </summary>
	[Description("Get the column value as a Double.")]
#if DBDATAREADER_ENABLED
	public override  double GetDouble(int i)
#else
	public           double GetDouble(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (double)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetString method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a String.
	/// </summary>
	[Description("Get the column value as a String.")]
#if DBDATAREADER_ENABLED
	public override  String GetString(int i)
#else
	public           String GetString(int i)
#endif
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (String)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** GetString method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Get the column value as a TimeSpan.
	/// </summary>
	[Description("Get the column value as a TimeSpan.")]
	public TimeSpan GetTimeSpan(int i)
	{
		object obj = this.GetValue(i);  // check reader is open and get object

		if (obj is String)
		{
			TimeSpan span;
			if (SqlInterval.getTryTimeSpan((string)obj, out span))
				obj = span;
		}

		// No conversions are performed and
		// InvalidCastException is thrown if not right type already.
		try { return (TimeSpan)obj; } // try returning data with the right cast
		catch (InvalidCastException)
		{
			throw new InvalidCastException(SpecifiedCastIsNotValid(obj, i));
		}
	}

	/*
	** IsDBNull method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Returns true/false indicating if the column value is null.
	/// </summary>
	[Description("Returns true/false indicating if the column value is null.")]
#if DBDATAREADER_ENABLED
	public override  bool IsDBNull(int i)
#else
	public           bool IsDBNull(int i)
#endif
	{
		CheckDataReaderIsOpen();  // throw InvalidOperationException if closed
		CheckDataReaderIsPositioned(); // check that we are positioned on a row

		return _resultset.IsDBNull(i);
	}

	/*
	** GetEnumerator method
	**
	** History:
	**	 3-mar-05 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Return an IEnumerator for a simple iteration over the collection.
	/// </summary>
#if DBDATAREADER_ENABLED
	public override  IEnumerator GetEnumerator()
#else
	public           IEnumerator GetEnumerator()
#endif
	{
		return new System.Data.Common.DbEnumerator(this,
			(_commandBehavior & CommandBehavior.CloseConnection) != 0 ?
				true:false);  
	}

	/*
	** CheckDataReaderIsOpen method
	**
	** History:
	**	 6-Nov-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Check that DataReader is open.  Throw InvalidOperationException if not.
	/// </summary>
	[Description("Check that DataReader is open.")]
	private void CheckDataReaderIsOpen()
	{
		if (this.IsClosed)
			throw new InvalidOperationException(
				"Invalid attempt to read data when DataReader is closed.");
	}

	/*
	** CheckDataReaderIsPositioned method
	**
	** History:
	**	 6-Nov-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Check that DataReader is positioned on a row.
	/// Throw InvalidOperationException if not.
	/// </summary>
	[Description("Check that DataReader is positioned on a row.")]
	private void CheckDataReaderIsPositioned()
	{
		if (!_isPositioned)
			throw new InvalidOperationException(
				"Invalid attempt to read data when DataReader is not " +
				"positioned on a row.  Read() method call is " +
				"missing or all rows have already been read.");
	}

	/*
	** SpecifiedCastIsNotValid method
	**
	** History:
	**	 6-Nov-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Format 'InvalidCastException' msg with more information.
	/// </summary>
	[Description("Format 'InvalidCastException' msg with more information.")]
	private string SpecifiedCastIsNotValid(Object obj, int ordinal)
	{
		string s;

		if (obj == null)
			s = "<null>";
		else
			s = "Type." + obj.GetType().ToString();
		//	s = "TypeCode." + Type.GetTypeCode(obj.GetType()).ToString();
		return String.Format(
			"Specified cast is not valid for object of {0} at column ordinal {1}.",
				s, ordinal);
	}


	/*
	** FireInfoMessageEvent method
	**
	** History:
	**	15-Nov-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Call the delegates with any warning messages.
	/// </summary>
	[Description("Call the delegates with any warning messages.")]
	private void FireInfoMessageEvent()
	{
		IngresErrorCollection      messages;
		SQLWarningCollection         warnings;

		if (_resultset == null)
			return;

		warnings = _resultset.Warnings;
		if (warnings == null  ||  this._connection == null)
			return;

		messages = new IngresErrorCollection();

		foreach (object obj in warnings)
		{
			SqlEx ex = (SqlEx) obj;
			IngresException ingresEx =
				(IngresException)ex.createProviderException();
			foreach (IngresError msg in ingresEx.Errors)
				messages.Add(msg);
		}

		if (messages == null)
			return;

		this._connection.FireInfoMessageEvent(messages);

		_resultset.clearWarnings();  // make sure we don't re-send warnings
	}  // FireInfoMessageEvent

}  // class


	/*
	** Name: IngresType
	**
	** Description:
	**	Base enumeration for the ProviderType datatypes.
	*/

	/// <summary>
	/// Base enumeration for the Ingres Data Provider datatypes.
	/// </summary>
	public enum IngresType
	{
		/// <summary>
		/// Fixed length stream of binary data.
		/// </summary>
		Binary           =(  -2),
		/// <summary>
		/// Variable length stream of binary data.
		/// </summary>
		VarBinary        =  (-3),
		/// <summary>
		/// Binary large object.
		/// </summary>
		LongVarBinary    =  (-4),
		/// <summary>
		/// Fixed length stream of character data.
		/// </summary>
		Char             =    1,
		/// <summary>
		/// Variable length stream of character data.
		/// </summary>
		VarChar          =   12,
		/// <summary>
		/// Character large object.
		/// </summary>
		LongVarChar      =  (-1),
		/// <summary>
		/// Signed 8-bit integer data.
		/// </summary>
		TinyInt          =  (-6),
		/// <summary>
		/// Signed 64-bit integer data.
		/// </summary>
		BigInt           =  (-5),
		/// <summary>
		/// Exact numeric data.
		/// </summary>
		Decimal          =    3,
		/// <summary>
		/// Signed 16-bit integer data.
		/// </summary>
		SmallInt         =    5,
		/// <summary>
		/// Signed 32-bit integer data.
		/// </summary>
		Int              =    4,
		/// <summary>
		/// Approximate numeric data.
		/// </summary>
		Real             =    7,
		/// <summary>
		/// Approximate numeric data.
		/// </summary>
		Double           =    8,
		/// <summary>
		/// Date and time data.
		/// </summary>
		DateTime         =   93,
		/// <summary>
		/// Ingres Date and time data.
		/// </summary>
		IngresDate       = 1093,
		/// <summary>
		/// Fixed length stream of Unicode data.
		/// </summary>
		NChar            = (-95),
		/// <summary>
		/// Variable length stream of Unicode data.
		/// </summary>
		NVarChar         = (-96),
		/// <summary>
		/// Unicode large object.
		/// </summary>
		LongNVarChar     = (-97),
		/// <summary>
		/// ANSI Date.
		/// </summary>
		Date             =   91,
		/// <summary>
		/// ANSI Time.
		/// </summary>
		Time             =   92,

//		Interval         =   10,   // Interval (generic, not used)

		/// <summary>
		/// ANSI Interval Year to Month.
		/// </summary>
		IntervalYearToMonth = 107, // Interval Year To Month
		/// <summary>
		/// ANSI Interval Day to Second.
		/// </summary>
		IntervalDayToSecond = 110, // Interval Day To Second

		/// <summary>
		/// Boolean.
		/// </summary>
		Boolean = 16,              // Boolean
	} // Type

}  // namespace
