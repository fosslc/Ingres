/*
** Copyright (c) 2005, 2010 Ingres Corporation. All Rights Reserved.
*/

/*
** Name: connectionstringbuilder.cs
**
** Description:
**	Implements the .NET Provider IngresConnectionStringBuilder.
**
** Classes:
**	IngresConnectionStringBuilder
**
** History:
**	14-Jul-05 (thoda04)
**	    Created.
**	03-May-06 (thoda04)
**	    Added Cursor_Mode support.
**	27-feb-07 (thoda04)
**	    Add DbmsUser, DbmsPassword, CharacterEncoding support.
**	16-feb-10 (thoda04) Bug 123298
**	    Added SendIngresDates support to send .NET DateTime parms
**	    as ingresdate type rather than as timestamp_w_timezone.
*/

using System;
using System.Data.Common;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Text;
using Ingres.ProviderInternals;

namespace Ingres.Client
{
	/// <summary>
	/// Provides a series of properties and methods to create
	/// syntactically correct connection string and to parse and
	/// rebuild existing connection strings.
	/// </summary>
	public sealed class IngresConnectionStringBuilder :
		DbConnectionStringBuilder
	{
		private ConnectStringConfig _connectStringConfig;
		private DictionaryEntry[]   _items;

		/// <summary>
		/// Create a new IngresConnectionStringBuilder.
		/// </summary>
		public IngresConnectionStringBuilder() : this("")
		{
		}

		/// <summary>
		/// Create a new IngresConnectionStringBuilder with
		/// an initial connection string.
		/// </summary>
		/// <param name="connectionString"></param>
		public IngresConnectionStringBuilder(string connectionString)
		{
			_items = (DictionaryEntry[])
				ConnectStringConfig.ConnectBuilderKeywords.Clone();

			this.ConnectionString = connectionString;

			_connectStringConfig =
				ConnectStringConfig.ParseConnectionString(connectionString);
		}

		/// <summary>
		/// BlankDate=null specifies that an Ingres blank (empty) date
		/// result value is to be returned to the application as a
		/// null value.  The default is to return an Ingres blank date
		/// as a DateTime value of "9999-12-31 23:59:59".
		/// </summary>
		[Category("Locale")]
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		  "BlankDate=\"null\" specifies that an Ingres blank (empty) date "+
		  "result value is to be returned to the application as a " +
		  "null value.  The default is to return an Ingres blank date " +
		  "as a DateTime value of \"9999-12-31 23:59:59\".")]
		[DisplayName("Blank Date")]
		public String BlankDate
		{
			get { return this["Blank Date"].ToString(); }
			set { this["Blank Date"] = value; }
		}  // BlankDate

		/// <summary>
		/// Specifies the .NET character encoding (e.g. ISO-8859-1) or 
		/// code page (e.g. cp1252) to be used for conversions between 
		/// Unicode and character data types.  Generally, the character 
		/// encoding is determined automatically by the data provider 
		/// from the Data Access Server installation character set.  
		/// This keyword allows an alternate character encoding to be 
		/// specified (if desired) or a valid character encoding to be 
		/// used if the data provider is unable to map the server’s character set.
		/// </summary>
		[Category("Locale")]
		[RefreshProperties(RefreshProperties.All)]
		[Description(
			"Specifies the .NET character encoding (e.g. ISO-8859-1) " +
			"or code page (e.g. cp1252) to be used for conversions " +
			"between Unicode and character data types.  Generally, " +
			"the character encoding is determined automatically by " +
			"the data provider from the Data Access Server " +
			"installation character set.  This keyword allows an " +
			"alternate character encoding to be specified (if desired) " +
			"or a valid character encoding to be used if the data provider " +
			"is unable to map the server’s character set.")]
		[DisplayName("Character Encoding")]
		public String CharacterEncoding
		{
			get { return this["Character Encoding"].ToString(); }
			set { this["Character Encoding"] = value; }
		}  // CharacterEncoding

		/// <summary>
		/// The time, in seconds, to wait for an attempted connection
		/// to time out if the connection has not completed.
		/// Default is 15.
		/// </summary>
		[Description("The time (in seconds) to wait for the connection attempt.  Default is 15")]
		[DisplayName("Connect Timeout")]
		public int ConnectTimeout
		{
			get { return (int)this["Connect Timeout"]; }
			set { this["Connect Timeout"] = value; }
		} // ConnectTimeout

		/// <summary>
		/// Specifies the default cursor concurrency mode, 
		/// which determines the concurrency of cursors that have 
		/// no explicitly assigned option in the command text, 
		/// e.g. "FOR UPDATE" or "FOR READONLY".
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		  "Specifies the default cursor concurrency mode, "+
		  "which determines the concurrency of cursors that have "+
		  "no explicitly assigned option in the command text, "+
		  "e.g. \"FOR UPDATE\" or \"FOR READONLY\".  "+
		  "Available options are: "+
		  "\"readonly\" – Provides non-updateable cursors " +
		  "for best performance (default); "+
		  "\"update\" - Provides updateable cursors; " +
		  "\"dbms\" – Concurrencly is assigned by the DBMS server")]
		[DisplayName("Cursor_Mode")]
		public String CursorMode
		{
			get { return this["Cursor_Mode"].ToString(); }
			set { this["Cursor_Mode"] = value; }
		}  // CursorMode

		/// <summary>
		/// Name of the Ingres database being connected to.
		/// If a server class is required, use syntax: dbname/server_class
		/// </summary>
		[Category("(Database)")]
		[RefreshProperties(RefreshProperties.All)]
		[Description("The database name opened or to be used upon connection.")]
		public String Database
		{
			get { return this["Database"].ToString(); }
			set { this["Database"] = value; }
		}  // Database

		/// <summary>
		/// Specifies the Ingres date format to be used by the Ingres server
		/// for date literals.  Corresponds to the Ingres environment
		/// variable II_DATE_FORMAT and is assigned the same values.
		/// </summary>
		[Category("Locale")]
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		  "Specifies the Ingres date format to be used by the Ingres server "+
		  "for date literals.  Corresponds to the Ingres environment "+
		  "variable II_DATE_FORMAT and is assigned the same values.")]
		[DisplayName("Date_Format")]
		public String DateFormat
		{
			get { return this["Date_Format"].ToString(); }
			set { this["Date_Format"] = value; }
		}  // DateFormat

		/// <summary>
		/// The user name to be associated with the DBMS session. (Equivalent to
		/// the Ingres -u flag which can require administrator privileges).
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		[Description("The user name to be associated with the DBMS session. " +
			"(Equivalent to the Ingres -u flag "+
			"which can require administrator privileges).")]
		[DisplayName("DBMS User")]
		public String DbmsUser
		{
			get { return this["DBMS User"].ToString(); }
			set { this["DBMS User"] = value; }
		} // DbmsUser

		/// <summary>
		/// The user's DBMS password for the session. 
		/// (Equivalent to the Ingres -P flag).
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		[PasswordPropertyText(true)]
		[Description("The user's DBMS password for the session. " +
			" (Equivalent to the Ingres -P flag).")]
		[DisplayName("DBMS Password")]
		public String DbmsPassword
		{
			get { return this["DBMS Password"].ToString(); }
			set { this["DBMS Password"] = value; }
		} // DbmsPassword

		/// <summary>
		/// Specifies the character that the Ingres DBMS Server is to
		/// use the comma (',') or the period ('.') to separate fractional
		/// and non-fractional parts of a number.  Default is the period ('.').
		/// </summary>
		[Category("Locale")]
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		  "Specifies the character that the Ingres DBMS Server is to " +
		  "use the comma (',') or the period ('.') to separate fractional " +
		  "and non-fractional parts of a number.  Default is the period ('.').")]
		[DisplayName("Decimal_Char")]
		public String DecimalChar
		{
			get { return this["Decimal_Char"].ToString(); }
			set { this["Decimal_Char"] = value; }
		}  // DecimalChar

		/// <summary>
		/// If set to true and if the creation thread is within a transaction
		/// context as established by System.EnterpriseServices.ServicedComponent,
		/// the IngresConnection is automatically enlisted into the transaction
		/// context.  Default is true.
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		  "If set to true and if the creation thread is within a transaction " +
		  "context as established by System.EnterpriseServices.ServicedComponent, " +
		  "the IngresConnection is automatically enlisted into the transaction "+
		  "context.  Default is true.")]
		public bool Enlist
		{
			get { return (bool)this["Enlist"]; }
			set { this["Enlist"] = value; }
		} // Enlist

		/// <summary>
		/// Group identifier that has permissions for a group of users.
		/// </summary>
		[DisplayName("Group ID")]
		public String GroupID
		{
			get { return this["Group ID"].ToString(); }
			set { this["Group ID"] = value; }
		}  // GroupID

		/// <summary>
		/// Maximum number of connections that can be in the pool.
		/// Default is 100.
		/// </summary>
		[Category("Pooling")]
		[RefreshProperties(RefreshProperties.All)]
		[Description("The maximum number of connections that can be in the pool.  Default is 100")]
		[DisplayName("Max Pool Size")]
		public int MaxPoolSize
		{
			get { return (int)this["Max Pool Size"]; }
			set { this["Max Pool Size"] = value; }
		} // MaxPoolSize

		/// <summary>
		/// Minimum number of connections that can be in the pool.
		/// Default is 0.
		/// </summary>
		[Category("Pooling")]
		[RefreshProperties(RefreshProperties.All)]
		[Description("The minimum number of connections that can be in the pool.  Default is 0")]
		[DisplayName("Min Pool Size")]
		public int MinPoolSize
		{
			get { return (int)this["Min Pool Size"]; }
			set { this["Min Pool Size"] = value; }
		} // MinPoolSize

		/// <summary>
		/// Specifies the Ingres money format to be used by the Ingres server
		/// for money literals.  Corresponds to the Ingres environment
		/// variable II_MONEY_FORMAT and is assigned the same values.
		/// </summary>
		[Category("Locale")]
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		  "Specifies the Ingres money format to be used by the Ingres server " +
		  "for money literals.  Corresponds to the Ingres environment " +
		  "variable II_MONEY_FORMAT and is assigned the same values.")]
		[DisplayName("Money_Format")]
		public String MoneyFormat
		{
			get { return this["Money_Format"].ToString(); }
			set { this["Money_Format"] = value; }
		} // MoneyFormat

		/// <summary>
		/// Specifies the money precision to be used by the Ingres server
		/// for money literals.  Corresponds to the Ingres environment
		/// variable II_MONEY_PREC and is assigned the same values.
		/// </summary>
		[Category("Locale")]
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		  "Specifies the money precision to be used by the Ingres server " +
		  "for money literals.  Corresponds to the Ingres environment " +
		  "variable II_MONEY_PREC and is assigned the same values.")]
		[DisplayName("Money_Precision")]
		public String MoneyPrecision
		{
			get { return this["Money_Precision"].ToString(); }
			set { this["Money_Precision"] = value; }
		} // MoneyPrecision

		/// <summary>
		/// The password to the Ingres database.
		/// </summary>
		[Category("Authentication")]
		[RefreshProperties(RefreshProperties.All)]
		[PasswordPropertyText(true)]
		[Description("The password to the database.  This value may be "+
			"case-sensitive depending on the server.")]
		public String Password
		{
			get { return this["Password"].ToString(); }
			set { this["Password"] = value; }
		} // Password

		/// <summary>
		/// Indicate whether password information is returned in a get
		/// of the ConnectionString.
		/// </summary>
		[Category("Authentication")]
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		  "Indicate whether password information is returned in a get " +
		  "of the ConnectionString.  Default is 'false'.")]
		[DisplayName("Persist Security Info")]
		public bool PersistSecurityInfo
		{
			get { return (bool)this["Persist Security Info"]; }
			set { this["Persist Security Info"] = value; }
		} // PersistSecurityInfo

		/// <summary>
		/// Enables or disables connection pooling.  By default,
		/// connection pooling is enabled (true).
		/// </summary>
		[Category("Pooling")]
		[RefreshProperties(RefreshProperties.All)]
		[Description("Enables or disables connection pooling.  By default, "+
		   "connection pooling is enabled (true).")]
		public bool Pooling
		{
			get { return (bool)this["Pooling"]; }
			set { this["Pooling"] = value; }
		} // Pooling

		/// <summary>
		/// Port number on the target server machine that the
		/// Data Access Server (DAS) is listening to.  Default is "II7".
		/// </summary>
		[Category("(Server)")]
		[RefreshProperties(RefreshProperties.All)]
		[Description("The TCP port id that the Data Access Server listens to.  Default is \"II7\".")]
		[DefaultValue("II7")]
		public String Port
		{
			get { return this["Port"].ToString(); }
			set { this["Port"] = value; }
		} // Port

		/// <summary>
		/// Role identifier that has associated privileges for the role.
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		[Description("Role idenitifier that has associated privileges for the role.")]
		[DisplayName("Role ID")]
		public String RoleID
		{
			get { return this["Role ID"].ToString(); }
			set { this["Role ID"] = value; }
		} // RoleID

		/// <summary>
		/// Role password associated with the Role ID.
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		[PasswordPropertyText(true)]
		[Description("Role password associated with the Role ID.")]
		[DisplayName("Role Password")]
		public String RolePassword
		{
			get { return this["Role Password"].ToString(); }
			set { this["Role Password"] = value; }
		} // RolePassword

		/// <summary>
		/// If set to true, send DateTime, Date, and Time parameters as
		/// ingresdate data type rather than as ANSI timestamp_with_timezone,
		/// ANSI date, ANSI time respectively.  This option may be used for
		/// compatibility with semantic rules from older releases of Ingres.
		/// Default is false.
		/// </summary>
		[Category("Locale")]
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		 "If set to true, send DateTime, Date, and Time parameters as " +
		 "ingresdate data type rather than as ANSI timestamp_with_timezone, " +
		 "ANSI date, ANSI time respectively.  This option may be used for " +
		 "compatibility with semantic rules from older releases of Ingres.  " +
		 "Default is false.")]
		[DefaultValue(false)]
		[DisplayName("SendIngresDates")]
		public Boolean SendIngresDates
		{
			get { return (bool)this["SendIngresDates"]; }
			set { this["SendIngresDates"] = value; }
		}

		/// <summary>
		/// The Ingres server to connect to.
		/// </summary>
		[Category("(Server)")]
		[RefreshProperties(RefreshProperties.All)]
		[Description("The name of the Ingres server connected to.")]
		public String Server
		{
			get  { return this["Server"].ToString();  }
			set  { this["Server"] = value; }
		} // Server


		/// <summary>
		/// Specifies the Ingres time zone associated with the user's location.
		/// Used by the Ingres server only.  Corresponds to the Ingres environment
		/// variable II_TIMEZONE_NAME and is assigned the same values.
		/// </summary>
		[Category("Locale")]
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		  "Specifies the Ingres time zone associated with the user's location. " +
		  "Used by the Ingres server only.  Corresponds to the Ingres environment " +
		  "variable II_TIMEZONE_NAME and is assigned the same values.")]
		public String Timezone
		{
			get { return this["Timezone"].ToString(); }
			set { this["Timezone"] = value; }
		} // Timezone

		/// <summary>
		/// The name of the authorized user connecting to the DBMS Server.
		/// This value may be case-sensitive depending on the targer server.
		/// </summary>
		[Category("Authentication")]
		[RefreshProperties(RefreshProperties.All)]
		[Description("The name of the authorized user connecting to the DBMS Server. " +
		   "This value may be case-sensitive depending on the targer server.")]
		[DisplayName("User ID")]
		public String UserID
		{
			get { return this["User ID"].ToString(); }
			set { this["User ID"] = value; }
		}

		/// <summary>
		/// Allows the .NET application to control the portions of the vnode
		/// information that are used to establish the connection to the remote
		/// DBMS server through the Ingres DAS server.
		/// Valid options are: 
		///    "connect" - Only the vnode connection information is used (default).
		///    "login"   - Both the vnode connection and login information is used.
		/// </summary>
		[RefreshProperties(RefreshProperties.All)]
		[Description(
		 "Allows the .NET application to control the portions of the vnode " +
		 "information that are used to establish the connection to the remote " +
		 "DBMS server through the Ingres DAS server. "+
		   "Valid options are: " +
			"\"connect\" - Only the vnode connection information is used (default). "+
			"\"login\"   - Both the vnode connection and login information is used.")]
		[DisplayName("Vnode_Usage")]
		public String VnodeUsage
		{
			get { return this["Vnode_Usage"].ToString(); }
			set { this["Vnode_Usage"] = value; }
		} // VnodeUsage



		/// <summary>
		/// Returns boolean flag indicating whether
		/// IngresConnectionStringBuilder has a fixed number of entries.
		/// </summary>
		public override bool IsFixedSize
		{
			get { return true; }
		}

		/// <summary>
		/// Gets an ICollection containing the keys
		/// in the IngresConnectionStringBuilder.
		/// </summary>
		public override System.Collections.ICollection Keys
		{
			get
			{
				String[] keys = new String[_items.Length];
				int i = 0;
				foreach (DictionaryEntry entry in _items)
					keys[i++] = (String)entry.Key;
				return keys;
			}
		}

		/// <summary>
		/// Gets an ICollection containing the values
		/// in the IngresConnectionStringBuilder.
		/// </summary>
			public override System.Collections.ICollection Values
		{
			get
			{
				Object[] values = new Object[_items.Length];
				int i = 0;
				foreach (DictionaryEntry entry in _items)
					values[i++] = entry.Value;
				return values;
			}
		}

		/// <summary>
		/// Get/set the value associated with the specified key.
		/// </summary>
		/// <param name="keyword"></param>
		/// <returns></returns>
		public override  object this [ String keyword ]
		{
			// Look up the ordinal for and return 
			// the value at that position.
			get
			{
				int i = GetOrdinal(keyword);
				return _items[i].Value;
			}
			set
			{
				int i = GetOrdinal(keyword);

				if (value == null)
				{
					this.Remove(keyword);
					return;
				}

				Object nativeValue = value;  // assume no conversion needed

				TypeCode sourceTypeCode = Type.GetTypeCode(value.GetType());
				Type     targetType =
					ConnectStringConfig.ConnectBuilderKeywords[i].Value.GetType();
				TypeCode targetTypeCode = Type.GetTypeCode(targetType);

				if (targetTypeCode != sourceTypeCode)
				{
					switch (targetTypeCode)
					{
						case TypeCode.Int32:
							nativeValue =   Int32.Parse(value.ToString());
							break;
						case TypeCode.Boolean:
							String valueString = value.ToString().Trim();
							String valueLower = ToInvariantLower(valueString);

							// Catch yes/no literals that Boolean.Parse doesn't
							if (valueLower == "yes"  ||  valueLower == "1")
							{	nativeValue =  true;
								value       = "true";
								break;
							}
							if (valueLower == "no"   ||  valueLower == "0")
							{	nativeValue =  false;
								value =       "false";
								break;
							}
							nativeValue = Boolean.Parse(valueString);
							break;
						case TypeCode.String:
							nativeValue = value.ToString();
							break;
						default:
							throw new System.InvalidOperationException(
								"IngresConnectionStringBuilder internal error: " +
								"unexpected Type for keyword '" + keyword + "'.");
					} // end switch
				} // end if (targetTypeCode != sourceTypeCode)

				string connectionString = keyword + "=" +
					(value != null ? value.ToString() : "");
				ConnectStringConfig connectStringConfig =
					ConnectStringConfig.ParseConnectionString(connectionString);
				if (connectStringConfig.Count > 1)
					throw new ArgumentException("Invalid keyword/value pair: '" +
						connectionString + "'.");

				base[_items[i].Key.ToString()] = nativeValue;  // Standardize key
				_items[i].Value                = nativeValue;
			}
		}

		/// <summary>
		/// Clear the contents of IngresConnectionStringBuilder and
		/// reset the Values collection back to its defaults.
		/// </summary>
		public override void Clear()
		{
			base.Clear();
			_items = (DictionaryEntry[])
				ConnectStringConfig.ConnectBuilderKeywords.Clone();
		}

		/// <summary>
		/// Return true if IngresConnectionStringBuilder contains the key.
		/// </summary>
		/// <param name="keyword"></param>
		/// <returns></returns>
		public override bool ContainsKey(string keyword)
		{
			int i = TryGetOrdinal(keyword);  // find the key or it synonym
			if (i == -1)
				return false;
			return base.ContainsKey(_items[i].Key.ToString());
		}

		/// <summary>
		/// Remove the entry with the specified key from
		/// the IngresConnectionStringBuilder Values collection.
		/// </summary>
		/// <param name="keyword"></param>
		/// <returns></returns>
		public override bool Remove(string keyword)
		{
			int i = TryGetOrdinal(keyword);
			if (i != -1)
				_items[i].Value =  // restore default value in Values
					ConnectStringConfig.ConnectBuilderKeywords[i].Value;
			return base.Remove(keyword);
		}

		/// <summary>
		/// Returns true if specified key exists in the
		/// IngresConnectionStringBuilder.
		/// </summary>
		/// <param name="keyword"></param>
		/// <returns></returns>
		public override bool ShouldSerialize(string keyword)
		{
			int i = TryGetOrdinal(keyword);  // find the key or it synonym
			if (i == -1)
				return false;
			return base.ShouldSerialize(_items[i].Key.ToString());
		}



		private int TryGetOrdinal(String keyword)
		{
			int i = ConnectStringConfig.TryGetOrdinal(keyword);
			return i;
		}

		private int GetOrdinal(String keyword)
		{
			int i = TryGetOrdinal(keyword);
			if (i == -1)
				throw new ArgumentException(
					"Keyword not supported: '" +
						(keyword!=null?keyword.ToString():"<null>") + "'.");
			return i;
		}

		private static string ToInvariantLower(string str)
		{
			return str.ToLower(System.Globalization.CultureInfo.InvariantCulture);
		}

	}  // IngresConnectionStringBuilder
}
