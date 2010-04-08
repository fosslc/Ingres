/*
** Copyright (c) 2002, 2007 Ingres Corporation. All Rights Reserved.
*/

#define DBPARAMETER_ENABLED
#define DBPARAMETERCOLLECTION_ENABLED

using System;
using System.Collections;
using System.ComponentModel;
using System.Data;
using System.Globalization;
using Ingres.Utility;

namespace Ingres.Client
{
	/*
	** Name: param.cs
	**
	** Description:
	**	A parameter to an IngresCommand.
	**
	** Classes:
	**	IngresParameter	Parameter class.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	**	27-Feb-04 (thoda04)
	**	    Added additional XML Intellisense comments.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-jul-04 (thoda04)
	**	    Corrected parameter names for IngresParameterCollection.Contains
	**	    IngresParameterCollection.IndexOf.
	**	 8-Oct-04 (thoda04) Bug 113213
	**	    Added RefreshProperties(RefreshProperties.All) to
	**	    DbType and IngresType, DefaultValueAttribute to most
	**	    properties and TypeConverter to Value property for the
	**	    proper serialization to code of the Parameter Collection Editor.
	**	19-Jul-05 (thoda04)
	**	    Added DbConnection base class.
	**	21-Mar-07 (thoda04)
	**	    Added IntervalDayToSecond, IntervalYearToMonth.
	*/

	/*
	** Name: IngresParameter
	**
	** Description:
	**	Represents a parameter to an IngresCommand's parameter,
	**	and optionally, its mapping to a DataSet column..
	**
	** Public Constructors:
	**	IngresParameter()
	**	IngresParameter(string parameterName)
	**	IngresParameter(string parameterName, object value)
	**	IngresParameter(string parameterName, IngresType type)
	**	IngresParameter(string parameterName, IngresType type, int size)
	**	IngresParameter(string parameterName, IngresType type, int size, string srcColName)
	**	IngresParameter(string parameterName, IngresType type, int size, string srcColName,
	**	              DataRowVersion srcVersion, object value)
	**
	** Public Properties
	**	DbType             Get/set the DbType of the parameter. Linked to IngresType.
	**	Direction          Get/set the parameter direction.  Possible values:
	**	                      Input
	**	                      InputOutput
	**	                      Output
	**	                      ReturnValue
	**	IsNullable         Get/set if null values are accepted (true).
	**	IngresType         Get/set the IngresType of the parameter. Linked to DbType.
	**	ParameterName      Get/set the name of the parameter.  Default is "".
	**	Precision          Get/set the maximum precision of the parameter.
	**	                   Default is 0.
	**	Scale              Get/set the number of decimal places of the parameter.
	**	                   Default is 0.
	**	Size               Get/set the maximum sizeof the parameter
	**	                   for binary and string types.  For binary types, size
	**	                   is in bytes.  For Unicode string types, size is 
	**	                   in characters.
	**	SourceColumn       Get/set the source column name in the mapping to the
	**	                   DataSet.  Default is "".
	**	SourceVersion      Get/set DataRowVersion to be used when loading
	**	                   IngresParameter.Value property.
	**	Value              Get/set the vale of the parameter or the DBNull object.
	**
	** Public Methods
	**	ToString           Returns a string containing the ParameterName
	**	                   property's value.
	*/

/// <summary>
/// Represents a parameter to an IngresCommand,
/// and optionally, its mapping to DataSet columns.
/// </summary>
//	Allow this type to be visible to COM.
[System.Runtime.InteropServices.ComVisible(true)]
[TypeConverter(typeof(IngresParameterTypeConverter))]
public sealed class IngresParameter :
#if DBPARAMETER_ENABLED
		System.Data.Common.DbParameter,
#else
		MarshalByRefObject,
#endif
			IDataParameter,
			IDbDataParameter,
			ICloneable
{
	private bool  _typeSpecified = false;

	// link SqlEx.get processing to the IngresException factory
	private  SqlEx sqlEx = new SqlEx(new IngresExceptionFactory());


	/*
	** Name: Provider's Parameter constructors
	**
	** Description:
	**	Implements the instantiation of the IngresParameter.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Create an empty parameter object.
	/// </summary>
	public IngresParameter()
	{
	}

	/// <summary>
	/// Create a parameter object with a parameter name and DbType data type.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="dbType"></param>
	public IngresParameter(
		string parameterName,
		DbType dbType)
	{
		this.ParameterName = parameterName;
		this.DbType        = dbType;  // also infers the IngresType
	}

	/// <summary>
	/// Create a parameter object with a parameter name and a provider data type.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="provType"></param>
	public IngresParameter(
		string parameterName,
		IngresType provType)  // IngresType
	{
		this.ParameterName = parameterName;
		this.ProviderType  = (ProviderType)provType;
	}

	/// <summary>
	/// Create a parameter object with a parameter name and a data value.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="value"></param>
	public IngresParameter(
		string parameterName,
		object value)
	{
		this.ParameterName = parameterName;
		this.Value         = value;  // also infers the DbType and IngresType
	}

	/// <summary>
	/// Create a parameter object with a parameter name,
	/// DbType data type, and source column name.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="dbType"></param>
	/// <param name="sourceColumn"></param>
	public IngresParameter(
		string parameterName,
		DbType dbType,
		string sourceColumn)
	{
		this.ParameterName = parameterName;
		this.DbType        = dbType;  // also infers the IngresType
		this.SourceColumn  = sourceColumn;
	}

	/// <summary>
	/// Create a parameter object with a parameter name,
	/// provider data type, and source column name.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="provType"></param>
	/// <param name="sourceColumn"></param>
	public IngresParameter(
		string parameterName,
		IngresType provType,  // IngresType
		string sourceColumn)
	{
		this.ParameterName = parameterName;
		this.ProviderType  = (ProviderType)provType;
		this.SourceColumn  = sourceColumn;
	}

	/// <summary>
	/// Create a parameter object with a parameter name,
	/// DbType data type, and parameter length.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="dbType"></param>
	/// <param name="size"></param>
	public IngresParameter(
		string parameterName,
		DbType dbType,
		int size)
	{
		this.ParameterName = parameterName;
		this.DbType        = dbType;  // also infers the IngresType
		this.Size          = size;
	}

	/// <summary>
	/// Create a parameter object with a parameter name,
	/// provider data type, and parameter length.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="provType"></param>
	/// <param name="size"></param>
	public IngresParameter(
		string parameterName,
		IngresType provType,  // IngresType
		int size)
	{
		this.ParameterName = parameterName;
		this.ProviderType  = (ProviderType)provType;
		this.Size          = size;
	}

	/// <summary>
	/// Create a parameter object with a parameter name,
	/// DbType data type, parameter length, and source column name.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="dbType"></param>
	/// <param name="size"></param>
	/// <param name="sourceColumn"></param>
	public IngresParameter(
		string parameterName,
		DbType dbType,
		int size,
		string sourceColumn)
	{
		this.ParameterName = parameterName;
		this.DbType        = dbType;  // also infers the IngresType
		this.Size          = size;
		this.SourceColumn  = sourceColumn;
	}

	/// <summary>
	/// Create a parameter object with a parameter name,
	/// provider data type, parameter length, and source column name.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="provType"></param>
	/// <param name="size"></param>
	/// <param name="sourceColumn"></param>
	public IngresParameter(
		string parameterName,
		IngresType provType,  // IngresType
		int size,
		string sourceColumn)
	{
		this.ParameterName = parameterName;
		this.ProviderType  = (ProviderType)provType;  // also infers the DbType
		this.Size          = size;
		this.SourceColumn  = sourceColumn;
	}

	/// <summary>
	/// Create a parameter object with a parameter name,
	/// DbType data type, parameter length, direction, nullability,
	/// numeric precision, numeric scale, source column name,
	/// source data row version, and data value.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="dbType"></param>
	/// <param name="size"></param>
	/// <param name="direction"></param>
	/// <param name="isNullable"></param>
	/// <param name="precision"></param>
	/// <param name="scale"></param>
	/// <param name="sourceColumn"></param>
	/// <param name="sourceVersion"></param>
	/// <param name="value"></param>
	public IngresParameter(
		string              parameterName,
		DbType              dbType,
		int                 size,
		ParameterDirection  direction,
		bool                isNullable,
		byte                precision,
		byte                scale,
		string              sourceColumn,
		DataRowVersion      sourceVersion,
		object              value)
	{
		this.ParameterName = parameterName;
		this.DbType        = dbType;  // also infers the IngresType
		this.Size          = size;
		this.Direction     = direction;
		this.IsNullable    = isNullable;
		this.Precision     = precision;
		this.Scale         = scale;
		this.SourceColumn  = sourceColumn;
		this.SourceVersion = sourceVersion;
		this.Value         = value;
	}

	/// <summary>
	/// Create a parameter object with a parameter name,
	/// provider data type, parameter length, direction, nullability,
	/// numeric precision, numeric scale, source column name,
	/// source data row version, and data value.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="provType"></param>
	/// <param name="size"></param>
	/// <param name="direction"></param>
	/// <param name="isNullable"></param>
	/// <param name="precision"></param>
	/// <param name="scale"></param>
	/// <param name="sourceColumn"></param>
	/// <param name="sourceVersion"></param>
	/// <param name="value"></param>
	public IngresParameter(
		string              parameterName,
		IngresType          provType,  // IngresType
		int                 size,
		ParameterDirection  direction,
		bool                isNullable,
		byte                precision,
		byte                scale,
		string              sourceColumn,
		DataRowVersion      sourceVersion,
		object              value)
	{
		this.ParameterName = parameterName;
		this.ProviderType  = (ProviderType)provType;  // also infers the DbType
		this.Size          = size;
		this.Direction     = direction;
		this.IsNullable    = isNullable;
		this.Precision     = precision;
		this.Scale         = scale;
		this.SourceColumn  = sourceColumn;
		this.SourceVersion = sourceVersion;
		this.Value         = value;
	}


	/*
	** Name: Properties
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	private DbType _dbType = DbType.String;
	/// <summary>
	/// Get or set the DbType for the parameter object.
	/// Implicitly set the provider data type.
	/// </summary>
	[DefaultValue(DbType.String)]
	[RefreshProperties(RefreshProperties.All)]
		// Tells VS.NET IDE to refresh ALL properties in its browser
		// to pick up the new values that IngresType property may have changed.
	[Browsable(false)]
		// Tells VS.NET Parameter Collection Editor not to list in Property windows.
	[EditorBrowsable]
		// Tells VS.NET IntelliSense not to list this in drop-down choices.
#if DBPARAMETER_ENABLED
		public override  DbType DbType
#else
		public           DbType DbType
#endif
	{
		get  { return _dbType; }
		set
		{
			// also infers the IngresType
			ProviderType  providerType = ProviderTypeMgr.GetProviderType(value);
			if (providerType == ProviderType.Other)
			{
				string s = value.ToString();
				throw new ArgumentException(
					SqlEx.get( GcfErr.ERR_GC401A_CONVERSION_ERR).Message +
					" DbType: " + s);
				//"An invalid datatype conversion was requested."
			}

			_dbType = value;
			_providerType = providerType;
			_typeSpecified = true;
		}
	}

	private  ProviderType  _providerType = ProviderType.NVarChar;
	internal ProviderType   ProviderType
	{
		get  { return _providerType; }
		set
		{
			// also infers the DbType
			_providerType = (ProviderType)value;
			_dbType       = ProviderTypeMgr.GetDbType(_providerType);
			_typeSpecified = true;
		}
	}

	/// <summary>
	/// Get or set the IngresType provider data type for the parameter.
	/// Implicitly set the DbType data type.
	/// </summary>
	[DefaultValue(IngresType.NVarChar)]
	[RefreshProperties(RefreshProperties.All)]
		// Tells VS.NET IDE to refresh ALL properties in its browser
		// to pick up the new values that DbType property may have changed.
	public  IngresType IngresType
	{
		get  { return (IngresType)_providerType; }
		set  { this.ProviderType = (ProviderType)value; }
	}

	private ParameterDirection  _direction = ParameterDirection.Input;
	/// <summary>
	/// Get or set the parameter direction.
	/// </summary>
	[DefaultValue(ParameterDirection.Input)]
#if DBPARAMETER_ENABLED
		public override  ParameterDirection   Direction
#else
		public           ParameterDirection   Direction
#endif
	{
		get { return _direction; }
		set { _direction = value; }
	}

	private bool     _nullable = false;
	/// <summary>
	/// Indicator as the nullability or the parameter.
	/// </summary>
	[DefaultValue(false)]
#if DBPARAMETER_ENABLED
		public override  Boolean IsNullable
#else
		public           Boolean IsNullable
#endif
	{
		get { return _nullable; }
		set { _nullable = value; }
	}

	private string _parameterName= "";
	/// <summary>
	/// Get or set the parameter name.
	/// </summary>
	[DefaultValue("")]
#if DBPARAMETER_ENABLED
		public override  String  ParameterName
#else
		public           String  ParameterName
#endif
	{
		get { return _parameterName; }
		set {_parameterName = value; }
	}

	private byte _precision; //= 0;
	/// <summary>
	/// Get or set the numeric precision for the parameter.
	/// </summary>
	[DefaultValue((byte)0)]
	public  Byte  Precision 
	{
		get { return _precision; }
		set { _precision = value; }
	}

	private byte _scale;     //= 0;
	/// <summary>
	/// Get or set the numeric scale for the parameter.
	/// </summary>
	[DefaultValue((byte)0)]
	public  Byte  Scale 
	{
		get { return _scale; }
		set { _scale = value; }
	}

	private int _size;       //= 0;
	/// <summary>
	/// Get or set the length of the parameter.
	/// </summary>
	[DefaultValue((int)0)]
#if DBPARAMETER_ENABLED
		public override  int  Size
#else
		public           int  Size
#endif
	{
		get { return _size; }
		set
		{
			if (value < 0)
				throw new ArgumentException("Size < 0.");
			_size = value;
			if (value > 0)
				SizeSpecified = true;
			else
				SizeSpecified = false;
		}
	}

	private  bool _sizeSpecified = false;
	internal bool  SizeSpecified
	{
		get { return _sizeSpecified; }
		set { _sizeSpecified = value; }
	}

	private string _sourceColumn = "";
	/// <summary>
	/// Get or set the source column name for the parameter.
	/// </summary>
	[DefaultValue("")]
#if DBPARAMETER_ENABLED
		public override  String  SourceColumn
#else
		public           String  SourceColumn
#endif
	{
		get { return _sourceColumn; }
		set { _sourceColumn = value; }
	}

	private DataRowVersion _sourceVersion= DataRowVersion.Current;
	/// <summary>
	/// Get or set the source column row version for the parameter.
	/// </summary>
	[DefaultValue(DataRowVersion.Current)]
#if DBPARAMETER_ENABLED
		public override  DataRowVersion  SourceVersion
#else
		public           DataRowVersion  SourceVersion
#endif
	{
		get { return _sourceVersion; }
		set { _sourceVersion = value; }
	}

	private object _value;     //= null;
	/// <summary>
	/// Get or set the data value for the parameter.
	/// </summary>
	[DefaultValue("")]
	[TypeConverterAttribute(typeof(System.ComponentModel.StringConverter))]
#if DBPARAMETER_ENABLED
		public override  object  Value
#else
		public           object  Value
#endif
	{
		get
		{
			return _value;
		}
		set
		{
			_value    = value;

			if (_typeSpecified  ||  // if Dbtype or IngresType was
				_value == null  ||  // specified or if value is null or DBNull
				Convert.IsDBNull(_value))  // then do nothing
					return;

			_dbType  = InferType(value);

			if (value != null  &&
				value.GetType() == typeof(System.TimeSpan))
				_providerType = ProviderType.IntervalDayToSecond;
			else
				_providerType = ProviderTypeMgr.GetProviderType(_dbType);
		}
	}


#if DBPARAMETER_ENABLED
	private int _offset = 0;
	/// <summary>
	/// The offset into the value for parameter data.
	/// </summary>
	internal int Offset
	{
		get { return _offset;  }
	}

	// TODO ???? description of SourceColumnNullMapping
	private bool _sourceColumnNullMapping = false;
	/// <summary>
	/// TBD.
	/// </summary>
	public override bool SourceColumnNullMapping
	{
		get { return _sourceColumnNullMapping;  }
		set { _sourceColumnNullMapping = value; }
	}

	/// <summary>
	/// Reset DbType and IngresType and allow the types to be
	/// inferred from Value.
	/// </summary>
	public override void ResetDbType()
	{
		this.DbType = DbType.String; // ?????
		_typeSpecified = false;      // Allow type to be inferred from value.
	}

	/// <summary>
	/// Reset IngresType and DbType and allow the types to be
	/// inferred from Value.
	/// </summary>
	public void ResetIngresType()
	{
		ResetDbType();
	}
#endif

	/// <summary>
	/// Get the ParameterName for the parameter object.
	/// </summary>
	/// <returns></returns>
	public override string ToString()
	{
		return ParameterName;
	}

	private DbType InferType(Object value)
	{
		Type type = value.GetType();

		if (type == typeof(System.Byte[]))
			return DbType.Binary;

		if (type == typeof(System.Char[]))
			return DbType.String;

		if (type == typeof(System.TimeSpan))
			return DbType.Object;

		TypeCode typeCode = Type.GetTypeCode(type);
		switch (typeCode)
		{
			case     TypeCode.Boolean:
				return DbType.Boolean;

			case     TypeCode.Byte:
				return DbType.Byte;

			case     TypeCode.DateTime:
				return DbType.DateTime;

			case     TypeCode.Decimal:
				return DbType.Decimal;

			case     TypeCode.Double:
				return DbType.Double;

			case     TypeCode.Int16:
				return DbType.Int16;

			case     TypeCode.Int32:
				return DbType.Int32;

			case     TypeCode.Int64:
				return DbType.Int64;

			case     TypeCode.SByte:
				return DbType.SByte;

			case     TypeCode.Single:
				return DbType.Single;

			case     TypeCode.Char:
			case     TypeCode.String:
				return DbType.String;

			case     TypeCode.UInt16:
				return DbType.UInt16;

			case     TypeCode.UInt32:
				return DbType.UInt32;

			case     TypeCode.UInt64:
				return DbType.UInt64;

			case     TypeCode.DBNull:  // should not happen
			case     TypeCode.Object:
			default:
				string s = type.ToString();
				throw new ArgumentException(
					SqlEx.get( GcfErr.ERR_GC401A_CONVERSION_ERR ).Message +
					"  object type: " + s);
					//"An invalid datatype conversion was requested."
		}  // end switch
	}

	/*
		** Name: Clone
		**
		** Description:
		**	Create a copy of this parameter.  Implements ICloneable.Clone().
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	IngresParameter	Copy of this parameter.
		**
		** History:
		**	13-Sep-02 (thoda04)
		**	    Created.*/

	/// <summary>
	/// Return a copy of the parameter object.
	/// </summary>
	/// <returns></returns>
	public System.Object Clone()
	{
		return MemberwiseClone();
	} // Clone


}  // class


	/*
	** Name: IngresParameterCollection
	**
	** Description:
	**	Represents an IngresCommand's parameter collection
	**	and its mapping to a DataSet object.
	**
	** Public Constructors:
	**	None.  Constructor is for provider use only.
	**
	** Public Properties
	**	this[int]          Get/set Item at the specified index (zero-based).
	**	this[string name]  Get/set Item at the specified name.
	**
	** Public Methods
	**	Add                Add the IngresParameter to the IngresParameterCollection.
	**	Clear              Remove all items from the IngresParameterCollection.
	**	Contains(object)   Returns true if the IngresParameter object exists
	**	                   in the IngresParameterCollection.
	**	Contains(string)   Returns true if the IngresParameter with the specified
	**	                   parameter name exists in the IngresParameterCollection.
	**	CopyTo             Copy the IngresParameters to an Array.
	**	IndexOf            Returns the zero-based index of the IngresParameter in the
	**	                   IngresParameterCollection.
	**	Insert             Insert the IngresParameter into the
	**	                   IngresParameterCollection at the specified index.
	**	Remove             Remove the IngresParameter from the IngresParameterCollection.
	**	RemoveAt           Remove the IngresParameter from the IngresParameterCollection
	**	                   at the specified index or using the specified name.
	*/

/// <summary>
/// Collects all parameters relevant to a Command
/// as well as their respective mappings to DataSet columns.
/// </summary>
//	Allow this type to be visible to COM.
[System.Runtime.InteropServices.ComVisible(true)]
	public sealed class IngresParameterCollection :
#if DBPARAMETERCOLLECTION_ENABLED
		System.Data.Common.DbParameterCollection,
#else
		MarshalByRefObject,
#endif

			IDataParameterCollection, IList, ICollection, IEnumerable
{
	private ArrayList _parmCollection = new ArrayList();
	private int       _parameterNameConstructedCount;  // = 0;

	// link SqlEx.get processing to the IngresException factory
	private  SqlEx sqlEx = new SqlEx(new IngresExceptionFactory());

	/// <summary>
	/// Get or set the parameter object within the collection using an index.
	/// </summary>
#if DBPARAMETERCOLLECTION_ENABLED
		public new  IngresParameter this[int index]
#else
		public      IngresParameter this[int index]
#endif
	{
		get { return (IngresParameter)_parmCollection[index];  }
		set { _parmCollection[index] = value; }
	}

	object IList.this[int index] 
	{
		get { return (IngresParameter)_parmCollection[index];  }
		set { _parmCollection[index] = value; }
	}

	/// <summary>
	/// Get or set the parameter object within the collection using a name.
	/// </summary>
#if DBPARAMETERCOLLECTION_ENABLED
		public new  IngresParameter this[string parameterName]
#else
		public      IngresParameter this[string parameterName]
#endif
	{
		get { return (IngresParameter)_parmCollection[IndexOf(parameterName)];}
		set {        _parmCollection[IndexOf(parameterName)] = value; }
	}

	object IDataParameterCollection.this[string parameterName] 
	{
		get { return _parmCollection[IndexOf(parameterName)];  }
		set {        _parmCollection[IndexOf(parameterName)] = value; }
	}

	/// <summary>
	/// Return the count of parameters within the collection.
	/// </summary>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  int Count
#else
		public           int Count
#endif
	{
		get { return _parmCollection.Count;  }
	}

	/// <summary>
	/// Return false to indicate that the parameter collection is not fixed size.
	/// </summary>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  bool IsFixedSize   /* implement Ilist interface */
#else
		public           bool IsFixedSize
#endif
	{
		get { return false; }
	}

	/// <summary>
	/// Return false to indicate that the parameter collection is not readonly.
	/// </summary>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  bool IsReadOnly   /* implement Ilist interface */
#else
		public           bool IsReadOnly
#endif
	{
		get { return false; }
	}

	/// <summary>
	/// Return an indication as to whether or not the collection is synchronized.
	/// </summary>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  bool IsSynchronized  // implement ICollection interface
#else
		public           bool IsSynchronized
#endif
	{
		get { return _parmCollection.IsSynchronized; }
	}
	
	/// <summary>
	/// Return the object that can be used to synchronize.
	/// </summary>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  object SyncRoot      // implement ICollection interface
#else
		public           object SyncRoot
#endif
	{
		get { return _parmCollection.SyncRoot; }
	}
	
	/// <summary>
	/// Add a parameter object to the parameter collection.
	/// </summary>
	/// <param name="value"></param>
	/// <returns></returns>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  int Add(object value)
#else
		public           int Add(object value)
#endif
	{
		_CheckType(value);
		IngresParameter parm = (IngresParameter)value;
		_CheckParameterAlreadyContained(parm);
		_CheckParameterName(parm);
		return _parmCollection.Add(parm);
	}

	/// <summary>
	/// Add a parameter object to the parameter collection.
	/// </summary>
	/// <param name="value"></param>
	/// <returns></returns>
	public IngresParameter Add(IngresParameter value)
	{
		_CheckType(value);
		IngresParameter parm = (IngresParameter)value;
		_CheckParameterAlreadyContained(parm);
		_CheckParameterName(parm);
		_parmCollection.Add(parm);
		return parm;
	}

	/// <summary>
	/// Create a parameter object and add it to the parameter collection.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="dbType"></param>
	/// <returns></returns>
	public IngresParameter Add(string parameterName, DbType dbType)
	{
		return Add(new IngresParameter(parameterName, dbType));
	}

	/// <summary>
	/// Create a parameter object and add it to the parameter collection.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="type"></param>
	/// <returns></returns>
	public IngresParameter Add(string parameterName, IngresType type)
	{
		return Add(new IngresParameter(parameterName, type));
	}

	/// <summary>
	/// Create a parameter object and add it to the parameter collection.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="value"></param>
	/// <returns></returns>
	public IngresParameter Add(string parameterName, object value)
	{	// value is the Value property for the new Parameter
		return Add(new IngresParameter(parameterName, value));
	}

	/// <summary>
	/// Create a parameter object and add it to the parameter collection.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="dbType"></param>
	/// <param name="sourceColumn"></param>
	/// <returns></returns>
	public IngresParameter Add(
				string parameterName,
				DbType dbType,
				string sourceColumn)
	{
		return Add(new IngresParameter(parameterName, dbType, sourceColumn));
	}

	/// <summary>
	/// Create a parameter object and add it to the parameter collection.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="type"></param>
	/// <param name="sourceColumn"></param>
	/// <returns></returns>
	public IngresParameter Add(
				string parameterName,
				IngresType type,
				string sourceColumn)
	{
		return Add(new IngresParameter(parameterName, type, sourceColumn));
	}

	/// <summary>
	/// Create a parameter object and add it to the parameter collection.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="dbType"></param>
	/// <param name="size"></param>
	/// <returns></returns>
	public IngresParameter Add(
				string parameterName,
				DbType dbType,
				int size)
	{
		return Add(new IngresParameter(parameterName, dbType, size));
	}

	/// <summary>
	/// Create a parameter object and add it to the parameter collection.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="type"></param>
	/// <param name="size"></param>
	/// <returns></returns>
	public IngresParameter Add(
				string parameterName,
				IngresType type,
				int size)
	{
		return Add(new IngresParameter(parameterName, type, size));
	}

	/// <summary>
	/// Create a parameter object and add it to the parameter collection.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="dbType"></param>
	/// <param name="size"></param>
	/// <param name="sourceColumn"></param>
	/// <returns></returns>
	public IngresParameter Add(
				string parameterName,
				DbType dbType,
				int size,
				string sourceColumn)
	{
		return Add(
			new IngresParameter(parameterName, dbType, size, sourceColumn));
	}

	/// <summary>
	/// Create a parameter object and add it to the parameter collection.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="type"></param>
	/// <param name="size"></param>
	/// <param name="sourceColumn"></param>
	/// <returns></returns>
	public IngresParameter Add(
				string parameterName,
				IngresType type,
				int size,
				string sourceColumn)
	{
		return Add(
			new IngresParameter(parameterName, type, size, sourceColumn));
	}

#if DBPARAMETERCOLLECTION_ENABLED
	/// <summary>
	/// Adds an array of IngresParameter to the end of
	/// the IngresParameterCollection.
	/// </summary>
	/// <param name="values"></param>
	public override void AddRange(Array values)
	{
		if (values == null)
			throw new ArgumentNullException("values");

		int i = 0;
		foreach (Object obj in values) // validation of values
		{
			if (obj == null)
				throw new ArgumentNullException("values[" + i.ToString() + "]");

			if (obj.GetType() != typeof(IngresParameter))
				throw new ArgumentException(
					"Argument must be of type IngresParameter",
					"values[" + i.ToString() + "]");
		} // foreach validation of values

		foreach (Object obj in values)
			this.Add((IngresParameter)obj);
	}

	/// <summary>
	/// Adds an array of IngresParameter to the end of
	/// the IngresParameterCollection.
	/// </summary>
	/// <param name="values"></param>
	public void AddRange(IngresParameter[] values)
	{
		this.AddRange((Array)values);
	}
#endif  // DBPARAMETERCOLLECTION_ENABLED

	/// <summary>
	/// Clear all items from the parameter collection.
	/// </summary>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  void Clear()
#else
		public           void Clear()
#endif
	{
		_parmCollection.Clear();
		_parameterNameConstructedCount = 0;
	}

	/// <summary>
	/// Returns true if the parameter object exists
	/// within the parameter collection.
	/// </summary>
	/// <param name="value"></param>
	/// <returns></returns>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  bool Contains(object value)
#else
		public           bool Contains(object value)
#endif
	{
		return(IndexOf(value) != -1);
	}

	/// <summary>
	/// Returns true if a parameter object with the parameter name exists
	/// within the parameter collection.
	/// </summary>
	/// <param name="value"></param>
	/// <returns></returns>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  bool Contains(string value)
#else
		public           bool Contains(string value)
#endif
	{
		return(IndexOf(value) != -1);
	}

	/// <summary>
	/// Copy the IngresParameter objects from the collection into an
	/// IngresParameter[] array beginning at an index.
	/// </summary>
	/// <param name="array"></param>
	/// <param name="index"></param>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  void             CopyTo(Array array, int index)
#else
		public           void ICollection.CopyTo(Array array, int index)
#endif
	{
		_parmCollection.CopyTo(array, index);
	}

	/// <summary>
	/// Copy the IngresParameter objects from the collection into an
	/// IngresParameter[] array beginning at an index.
	/// </summary>
	/// <param name="array"></param>
	/// <param name="index"></param>
	public void CopyTo(IngresParameter[] array, int index)
	{
		((ICollection)this).CopyTo(array, index);
	}

	/// <summary>
	/// Get the enumerator to iterate through the error collection.
	/// </summary>
	/// <returns>An IEnumerator to iterate
	/// through the error collection.</returns>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  IEnumerator GetEnumerator()
#else
		public           IEnumerator GetEnumerator()
#endif
	{
		return _parmCollection.GetEnumerator();
	}


#if DBPARAMETERCOLLECTION_ENABLED
	/// <summary>
	/// Get the IngresParameter using the specified integer index.
	/// </summary>
	/// <param name="index"></param>
	/// <returns></returns>
	protected override System.Data.Common.DbParameter GetParameter(int index)
	{
		return this[index];
	}

	/// <summary>
	/// Get the IngresParameter using the specified string index.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <returns></returns>
	protected override System.Data.Common.DbParameter GetParameter(string parameterName)
	{
		if (parameterName == null)
			throw new ArgumentNullException("parameterName");

		int index = this.IndexOf(parameterName);
		if (index == -1)
			throw new IndexOutOfRangeException(
				"parameterName index does not exist in " +
				"IngresParameterCollection.SetParameter");

		return this[parameterName];
	}
#endif


	/// <summary>
	/// Returns the zero-based index of the
	/// Parameter in the ParameterCollection.
	/// </summary>
	/// <param name="value"></param>
	/// <returns></returns>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  int IndexOf(object value)
#else
		public           int IndexOf(object value)
#endif
	{
		_CheckType(value);

		int index = 0;
		foreach(IngresParameter item in this) 
		{
			if (value.Equals(item))
			{
				return index;
			}
			index++;
		}
		return -1;
	}

	/// <summary>
	/// Returns the zero-based index of the Parameter
	/// in the ParameterCollection using the parameter name.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <returns></returns>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  int IndexOf(string parameterName)
#else
		public           int IndexOf(string parameterName)
#endif
	{
		int index = 0;
		foreach(IngresParameter item in this) 
		{
			if (CurrentCultureCompare(item.ParameterName, parameterName) == 0)
			{
				return index;
			}
			index++;
		}
		return -1;
	}

	/// <summary>
	/// Insert the Parameter into the ParameterCollection
	/// at the specified index.
	/// </summary>
	/// <param name="index"></param>
	/// <param name="value"></param>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  void Insert(int index, object value)
#else
		public           void Insert(int index, object value)
#endif
	{
		_CheckType(value);
		_parmCollection.Insert(index, value);
	}

	/// <summary>
	/// Remove the Parameter object from the ParameterCollection.
	/// </summary>
	/// <param name="value"></param>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  void Remove(object value)
#else
		public           void Remove(object value)
#endif
	{
		_CheckType(value);
		_parmCollection.Remove(value);
	}
	
	/// <summary>
	/// Remove the Parameter object from the specified
	/// zero-based location within the ParameterCollection.
	/// </summary>
	/// <param name="index"></param>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  void RemoveAt(int index)
#else
		public           void RemoveAt(int index)
#endif
	{
		_parmCollection.RemoveAt(index);
	}

	/// <summary>
	/// Remove the Parameter from the ParameterCollection
	/// using the specified name.
	/// </summary>
	/// <param name="parameterName"></param>
#if DBPARAMETERCOLLECTION_ENABLED
		public override  void RemoveAt(string parameterName)
#else
		public           void RemoveAt(string parameterName)
#endif
	{
		RemoveAt(IndexOf(parameterName));
	}

#if DBPARAMETERCOLLECTION_ENABLED
	/// <summary>
	/// Set the IngresParameter using the specified integer index.
	/// </summary>
	/// <param name="index"></param>
	/// <param name="value"></param>
	protected override void SetParameter(
		int index, System.Data.Common.DbParameter value)
	{
		if (value == null)
			throw new ArgumentNullException("value");

		if (value.GetType() != typeof(IngresParameter))
			throw new ArgumentException(
				"Argument must be of type IngresParameter",
				"value");

		this.RemoveAt(index);
		this.Insert(index, value);
	}


	/// <summary>
	/// Set the IngresParameter using the specified string index.
	/// </summary>
	/// <param name="parameterName"></param>
	/// <param name="value"></param>
	protected override void SetParameter(
		string parameterName, System.Data.Common.DbParameter value)
	{
		if (parameterName == null)
			throw new ArgumentNullException("parameterName");

		int index = this.IndexOf(parameterName);
		if (index == -1)
			throw new IndexOutOfRangeException(
				"parameterName index does not exist in " +
				"IngresParameterCollection.SetParameter");

		SetParameter(index, value);
	}
#endif


	private int CurrentCultureCompare(string str1, string str2)
	{
		return CultureInfo.CurrentCulture.CompareInfo.Compare(
			str1, str2,
			CompareOptions.IgnoreKanaType |
			CompareOptions.IgnoreWidth    |
			CompareOptions.IgnoreCase);
	}

	private void _CheckParameterAlreadyContained(IngresParameter parm)
	{
		if (!this.Contains(parm))  // if not in the Collection then return OK
			return;

		string parmName = parm.ParameterName;
		if (parmName == null)
			parmName = "";

		//Already in the collection!  User forgot to cmd.Parameters.Clear()
		string msg = 
			"The ParameterCollection already contains " +
			"the Parameter object (ParameterName = '{0}').  " +
			"Consider a Clear of the ParameterCollection " +
			"such as cmd.Parameters.Clear() before adding the Parameter.";
		throw new ArgumentException(String.Format(msg, parmName));
	}


	private void _CheckParameterName(IngresParameter parm)
	{
		if (parm.ParameterName == null  ||  parm.ParameterName.Length == 0)
		{
			string ParameterN;
			while(true)
			{
				_parameterNameConstructedCount++;
				ParameterN = String.Format(  // build "Parameter1", etc.
					"Parameter{0}", _parameterNameConstructedCount);
				if (!this.Contains(ParameterN))  // if no duplicate then break
					break;
			}
			parm.ParameterName = ParameterN;
		}
	}


	private void _CheckType(object value)
	{
		if (value == null)
			throw new ArgumentNullException(
				"The parameter is null and is not an IngresParameter");

		if (value.GetType() != typeof(IngresParameter))
			throw new InvalidCastException(
				"The parameter is not an IngresParameter");
	}

}  // class


	/*
	** Name: IngresParameterTypeConverter
	**
	** Description:
	**	Converts IngresParameter to InstanceDescriptor representation
	**	for the Visual Studio code generation.
	**	The InstanceDescriptor provides the code generation engine:
	**	   What IngresParameter constructor should be invoked.
	**	   What values should be passed to that constructor to init the object.
	**	   Does the invocation completely describe the state of the object.
	**	More info on the process can be found in the MSDN document
	**	"Customizing Code Generation in the .NET Framework Visual Designers"
	**	in the "Generating Code for Custom Types" section.
	**
	** Public Methods
	**	CanConvertTo    Return an indication that we can convert 
	**	                an IngresParameter to an InstanceDescriptor.
	**	ConvertTo       Convert an IngresParameter to an
	**	                InstanceDescriptor representation.
	**/

	/// <summary>
	/// Converts the whole IngresParameter to/from a string representation
	/// for the Visual Studio code generation.
	/// </summary>
internal class IngresParameterTypeConverter :
	System.ComponentModel.ExpandableObjectConverter
{
	public override bool CanConvertTo(
		ITypeDescriptorContext context,
		Type                   destinationType)
	{
		if (destinationType == typeof(
			System.ComponentModel.Design.Serialization.InstanceDescriptor))
			return true;
		return base.CanConvertTo(context, destinationType);
	}  // CanConvert

	public override object ConvertTo(
		ITypeDescriptorContext context,
		CultureInfo culture,
		object value,
		Type destinationType)
	{
		if (destinationType != typeof(
			System.ComponentModel.Design.Serialization.InstanceDescriptor)
			||  !(value is IngresParameter))
			return base.ConvertTo (context, culture, value, destinationType);

		IngresParameter parm = (IngresParameter)value;
		string          parameterName = parm.ParameterName;// != null ?
		//	parm.ParameterName : "";

		System.Reflection.ConstructorInfo ctorInfo = null;
		Object[] values;

		const int NOTHING_SPECIFIED      = 0x00;
		const int INGRESTYPE_SPECIFIED   = 0x01;
		const int VALUE_SPECIFIED        = 0x02;
		const int SOURCECOLUMN_SPECIFIED = 0x04;
		const int SIZE_SPECIFIED         = 0x08;
		const int OTHER_SPECIFIED        = 0x10;
		const int INGRESTYPE_AND_SOURCECOLUMN_SPECIFIED =
		             INGRESTYPE_SPECIFIED |
		             SOURCECOLUMN_SPECIFIED;
		const int INGRESTYPE_AND_SIZE_SPECIFIED =
		             INGRESTYPE_SPECIFIED |
		             SIZE_SPECIFIED;
		const int INGRESTYPE_AND_SIZE_AND_SOURCECOLUMN_SPECIFIED =
		             INGRESTYPE_SPECIFIED |
		             SIZE_SPECIFIED       |
		             SOURCECOLUMN_SPECIFIED;

		int       specified              = NOTHING_SPECIFIED;

		if (parm.ProviderType != ProviderType.NVarChar)
			specified |= INGRESTYPE_SPECIFIED;

		if (parm.Size != 0)
			specified |= SIZE_SPECIFIED;

		if (parm.Direction != ParameterDirection.Input)
			specified |= OTHER_SPECIFIED;

		if (parm.IsNullable != false)
			specified |= OTHER_SPECIFIED;

		if (parm.Precision != 0)
			specified |= OTHER_SPECIFIED;

		if (parm.Scale != 0)
			specified |= OTHER_SPECIFIED;

		if (parm.SourceVersion != DataRowVersion.Current)
			specified |= OTHER_SPECIFIED;

		if (parm.SourceColumn != null   &&
			parm.SourceColumn.Length != 0)
			specified |= SOURCECOLUMN_SPECIFIED;

		if (parm.Value != null)
			specified |= VALUE_SPECIFIED;

		switch( specified )
		{
			case NOTHING_SPECIFIED:
			case INGRESTYPE_SPECIFIED:
			{
				ctorInfo = typeof(IngresParameter).GetConstructor( new Type[]
				{
					typeof(String),
					typeof(IngresType)
				});
				values = new Object[2];
				values[0] = parameterName;
				values[1] = parm.IngresType;
				break;
			}

			case VALUE_SPECIFIED:
			{
				ctorInfo = typeof(IngresParameter).GetConstructor( new Type[]
				{
					typeof(String),
					typeof(object)
				});
				values = new Object[2];
				values[0] = parameterName;
				values[1] = parm.Value;
				break;
			}

			case INGRESTYPE_AND_SOURCECOLUMN_SPECIFIED:
			case SOURCECOLUMN_SPECIFIED:
			{
				ctorInfo = typeof(IngresParameter).GetConstructor( new Type[]
				{
					typeof(String),
					typeof(IngresType),
					typeof(String)
				});
				values = new Object[3];
				values[0] = parameterName;
				values[1] = parm.IngresType;
				values[2] = parm.SourceColumn;
				break;
			}

			case INGRESTYPE_AND_SIZE_SPECIFIED:
			case SIZE_SPECIFIED:
			{
				ctorInfo = typeof(IngresParameter).GetConstructor( new Type[]
				{
					typeof(String),
					typeof(IngresType),
					typeof(int)
				});
				values = new Object[3];
				values[0] = parameterName;
				values[1] = parm.IngresType;
				values[2] = parm.Size;
				break;
			}

			case INGRESTYPE_AND_SIZE_AND_SOURCECOLUMN_SPECIFIED:
			{
				ctorInfo = typeof(IngresParameter).GetConstructor( new Type[]
				{
					typeof(String),
					typeof(IngresType),
					typeof(int),
					typeof(String)
				});
				values = new Object[4];
				values[0] = parameterName;
				values[1] = parm.IngresType;
				values[2] = parm.Size;
				values[3] = parm.SourceColumn;
				break;
			}

			default:
			{
				ctorInfo = typeof(IngresParameter).GetConstructor( new Type[]
				{
					typeof(String),
					typeof(IngresType),
					typeof(int),
					typeof(ParameterDirection),
					typeof(bool),
					typeof(byte),
					typeof(byte),
					typeof(String),
					typeof(DataRowVersion),
					typeof(object)
				});

				values = new Object[10];
				values[0] = parameterName;
				values[1] = parm.IngresType;
				values[2] = parm.Size;
				values[3] = parm.Direction;
				values[4] = parm.IsNullable;
				values[5] = parm.Precision;
				values[6] = parm.Scale;
				values[7] = parm.SourceColumn;
				values[8] = parm.SourceVersion;
				values[9] = parm.Value;
				break;
			}
		}

		System.ComponentModel.Design.Serialization.InstanceDescriptor
			instanceDescriptor = new
			System.ComponentModel.Design.Serialization.InstanceDescriptor(
			ctorInfo, values);
		// return the InstanceDescriptor so that the VS Parameter
		// Collection Editor can generate the constructor source code.
		return instanceDescriptor;
	}  // ConvertTo

}  // class IngresParameterTypeConverter

}  // namespace
