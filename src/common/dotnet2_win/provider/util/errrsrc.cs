/*
** Copyright (c) 1999, 2009 Ingres Corporation. All Rights Reserved.
*/

namespace Ingres.Utility
{
	/*
	** Name: errrsrc.cs
	**
	** Description:
	**	Locale specific information.  This file defines the
	**	default resources values (english,USA) for the .NET
	**	Provider.  These resources may be localized for
	**	different languages and customs by providing suitably
	**	named classes (see documentation for ResourceBundle)
	**	which define the same keys as this file and provides 
	**	the translated values.
	**
	** History:
	**	 2-Sep-99 (gordy)
	**	    Created.
	**	18-Apr-00 (gordy)
	**	    Moved to util package.
	**	13-Jun-00 (gordy)
	**	    Added procedure call syntax error E_JD0016.
	**	17-Oct-00 (gordy)
	**	    Added autocommit mode definitions.
	**	30-Oct-00 (gordy)
	**	    Added unsupported ResultSet type E_JD0017_RS_CHANGED,
	**	    invalid timezone E_JD0018_UNKNOWN_TIMEZONE.
	**	31-Jan-01 (gordy)
	**	    Added invalid date format E_JD0019_INVALID_DATE.
	**	28-Mar-01 (gordy)
	**	    Renamed effective user resource to DBMS user to
	**	    be more generic and pair with DBMS password.
	**	20-Aug-01 (gordy)
	**	    Added 'Cursor Mode' connection property.
	**	29-Aug-01 (loera01) SIR 105641
	**	    Added invalid output parameter: E_JD0020_INVALID_OUT_PARAM.
	**	    Added no result set returned: E_JD0021_NO_RESULT_SET.
	**	    Added result set not permitted: E_JD0022_RESULT_SET_NOT_PERMITTED.
	**	25-Jul-02 (thoda04)
	**	    Ported to .NET Provider.
	**	 1-Nov-03 (gordy)
	**	    Added invalid row request E_GC4021.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Added additional server error codes to provide local exceptions.
	**	12-may-09 (thoda04)
	**	    Added E_GC4207_CONNECTION_STRING_MISSING_PAREN.
	*/

	using System;
	using System.Runtime.Serialization;
	using System.Security;
	using System.Security.Permissions;

	/*
	** Name: ErrRsrc
	**
	** Description:
	**	ResourceBundle sub-class which defines the default resource
	**	keys and values (english,USA) for the Driver.
	**
	**
	**  Public Data:
	**
	**    Resource keys for Connection Properties:
	**
	**	cp_db		Database
	**	cp_usr		User ID
	**	cp_pwd		Password
	**	cp_grp		Group ID
	**	cp_role		Role ID
	**	cp_dbusr	EBMS user ID
	**	cp_dbpwd	DBMS password
	**	cp_pool		Connection pool
	**	cp_xacm		Autocommit mode
	**	cp_loop		Select loops
	**	cp_crsr		Cursor Mode
	**
	**  Public methods:
	**
	**	getContents	Returns the array of keys/values for defined Locale.
	**	getResource	Returns the Ingres resources for default Locale
	**
	**  Private Data:
	**
	**	info		Array of keys/values.
	**	resource	Resource for default Locale.
	**
	**
	** History:
	**	 2-Sep-99 (gordy)
	**	    Created.
	**	17-Sep-99 (rajus01)
	**	    Added connection parameters.
	**	29-Sep-99 (gordy)
	**	    Added BLOB support: E_JD000B_BLOB_IO,
	**	    E_JD000C_BLOB_ACTIVE, E_JD000D_BLOB_DONE.
	**	26-Oct-99 (gordy)
	**	    Added ODBC escape parsing: E_JD000E_UNMATCHED.
	**	16-Nov-99 (gordy)
	**	    Added query timeouts: E_JD000F_TIMEOUT.
	**	11-Nov-99 (rajus01)
	**	    Implement DatabaseMetaData: Added E_JD0010_CATALOG_UNSUPPORTED,
	**	    E_JD0011_SQLTYPE_UNSUPPORTED.
	**	21-Dec-99 (gordy)
	**	    Added generic unsupported: E_JD0012_UNSUPPORTED.
	**	14-Jan-00 (gordy)
	**	    Added Connection Pool parameter.
	**	25-Jan-00 (rajus01)
	**	    Added resultsSet closed: E_JD0013_RESULTSET_CLOSED.
	**	31-Jan-00 (rajus01)
	**	    Added column not found: E_JD0014_INVALID_COLUMN_NAME.
	**	18-Apr-00 (gordy)
	**	    Added static data, initializer, getResource() method,
	**	    and EmptyRsrc class definition to make class self-
	**	    contained.
	**	19-May-00 (gordy)
	**	    Added connection property for select loops.
	**	13-Jun-00 (gordy)
	**	    Added procedure call syntax error E_JD0016.
	**	17-Oct-00 (gordy)
	**	    Added autocommit mode definitions.
	**	30-Oct-00 (gordy)
	**	    Added unsupported ResultSet type E_JD0017_RS_CHANGED,
	**	    invalid timezone E_JD0018_UNKNOWN_TIMEZONE.
	**	31-Jan-01 (gordy)
	**	    Added invalid date format E_JD0019_INVALID_DATE.
	**	28-Mar-01 (gordy)
	**	    Renamed effective user resource to DBMS user to
	**	    be more generic and pair with DBMS password.
	**	20-Aug-01 (gordy)
	**	    Added 'Cursor Mode' connection property.
	**	25-Jul-02 (thoda04)
	**	    Ported to .NET Provider.
	**	 1-Nov-03 (gordy)
	**	    Added invalid row request E_GC4021.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Added additional server error codes to provide local exceptions:
	**	    E_GC480D, E_GC480E, E_GC481E, E_GC481F, E_GC4811.
	*/

	internal class ErrRsrc : ListResourceBundle
	{
		public const String	E_GC4000 = "E_GC4000";
		public const String	E_GC4001 = "E_GC4001";
		public const String	E_GC4002 = "E_GC4002";
		public const String	E_GC4003 = "E_GC4003";
		public const String	E_GC4004 = "E_GC4004";
		public const String	E_GC4005 = "E_GC4005";
		public const String	E_GC4006 = "E_GC4006";
		public const String	E_GC4007 = "E_GC4007";
		public const String	E_GC4008 = "E_GC4008";
		public const String	E_GC4009 = "E_GC4009";
		public const String	E_GC4010 = "E_GC4010";
		public const String	E_GC4011 = "E_GC4011";
		public const String	E_GC4012 = "E_GC4012";
		public const String	E_GC4013 = "E_GC4013";
		public const String	E_GC4014 = "E_GC4014";
		public const String	E_GC4015 = "E_GC4015";
		public const String	E_GC4016 = "E_GC4016";
		public const String	E_GC4017 = "E_GC4017";
		public const String	E_GC4018 = "E_GC4018";
		public const String	E_GC4019 = "E_GC4019";
		public const String	E_GC401A = "E_GC401A";
		public const String	E_GC401B = "E_GC401B";
		public const String	E_GC401C = "E_GC401C";
		public const String	E_GC401D = "E_GC401D";
		public const String	E_GC401E = "E_GC401E";
		public const String	E_GC401F = "E_GC401F";
		public const String	E_GC4020 = "E_GC4020";
		public const String	E_GC4021 = "E_GC4021";

		public const String E_GC4201 = "E_GC4201";
		public const String E_GC4202 = "E_GC4202";
		public const String E_GC4203 = "E_GC4203";
		public const String E_GC4204 = "E_GC4204";
		public const String E_GC4205 = "E_GC4205";
		public const String E_GC4206 = "E_GC4206";
		public const String E_GC4207 = "E_GC4207";

		public const String E_GC480D = "E_GC480D";
		public const String E_GC480E = "E_GC480E";
		public const String E_GC4811 = "E_GC4811";
		public const String E_GC481E = "E_GC481E";
		public const String E_GC481F = "E_GC481F";

		public const String E_AP0009 = "E_AP0009";


		private static Object[][]   info = 
	{
		/*
		** Error messages.
		*/
new Object[]{
		E_GC4000,	// E_GC4000_BAD_URL
	"Target connection string improperly formatted." },
new Object[]{
		E_GC4001,	// E_GC4001_CONNECT_ERR
	"Communications error while establishing connection." },
new Object[]{
		E_GC4002,	// E_GC4002_PROTOCOL_ERR
	"Connection aborted due to communications protocol error." },
new Object[]{
		E_GC4003,	// E_GC4003_CONNECT_FAIL
	"Connection failed." },
new Object[]{
		E_GC4004,	// E_GC4004_CONNECTION_CLOSED
	"Connection closed." },
new Object[]{
		E_GC4005,	// E_GC4005_SEQUENCING
	"A new request may not be started while a prior request is still active."},
new Object[]{
		E_GC4006,	// E_GC4006_TIMEOUT
	"Operation cancelled due to time-out." },
new Object[]{
		E_GC4007,	// E_GC4007_BLOB_IO
	"I/O error during BLOB processing." },
new Object[]{
		E_GC4008,	// E_GC4008_SERVER_ABORT
	"Server aborted connection." },
new Object[]{
		E_GC4009,	// E_GC4009_BAD_CHARSET
	"Could not map server character set to local character encoding." },
new Object[]{
		E_GC4010,	// E_GC4010_PARAM_VALUE
	"Invalid parameter value." },
new Object[]{
		E_GC4011,	// E_GC4011_INDEX_RANGE
	"Invalid column or parameter index." },
new Object[]{
		E_GC4012,	// E_GC4012_INVALID_COLUMN_NAME 
	"Invalid column or parameter name." },
new Object[]{
		E_GC4013,	// E_GC4013_UNMATCHED
	"Unmatched quote, parenthesis, bracket or brace." },
new Object[]{
		E_GC4014,	// E_GC4014_CALL_SYNTAX
	"Invalid procedure call syntax." },
new Object[]{
		E_GC4015,	// E_GC4015_INVALID_OUT_PARAM
	"Output parameters not allowed with row-producing procedures" },
new Object[]{
		E_GC4016,	// E_GC4016_RS_CHANGED
	"Requested result set type, concurrency and/or holdability " +
	"is not supported." },
new Object[]{
		E_GC4017,	// E_GC4017_NO_RESULT_SET
	"Query or procedure does not return a result set." },
new Object[]{
		E_GC4018,	// E_GC4018_RESULT_SET_NOT_PERMITTED
	"Query or procedure returns a result set" },
new Object[]{
		E_GC4019,	// E_GC4019_UNSUPPORTED
	"Request is not supported." },
new Object[]{
		E_GC401A,	// E_GC401A_CONVERSION_ERR
	"Invalid datatype conversion." },
new Object[]{
		E_GC401B,	// E_GC401B_INVALID_DATE
	"DATE/TIME value not in standard format." },
new Object[]{
		E_GC401C,	// E_GC401C_BLOB_DONE
	"Request to retrieve BLOB column which had already been accessed.  " +
	"BLOB columns may only be accessed once." },
new Object[]{
		E_GC401D,	// E_GC401D_RESULTSET_CLOSED 
	"The ResultSet is closed" },
new Object[]{
		E_GC401E,	// E_GC401E_CHAR_ENCODE 
	"Invalid character/string encoding." },
new Object[]{
		E_GC401F,	// E_GC401F_XACT_STATE
	"Invalid transaction state for requested operation." },
new Object[]{
		E_GC4020,	// E_GC4020_NO_PARAM
	"The value for a dynamic parameter was not provided." },
new Object[]{
		E_GC4021,	// E_GC4021_INVALID_ROW
	"Cursor is not positioned on a row for which the requested " +
	"operation is supported." },

new Object[]{
		E_GC4201,	// E_GC4201_CONNECTION_STRING_BAD_KEY
	"Unknown connection string keyword: '{0}'"},
new Object[]{
		E_GC4202,	// E_GC4202_CONNECTION_STRING_BAD_VALUE
	"Connection string keyword '{0}' has invalid value: '{1}'"},
new Object[]{
		E_GC4203,	// E_GC4203_CONNECTION_STRING_BAD_QUOTE
	"Invalid connection string. Only values, not keywords, "+
	"may be delimited by single or double quotes."},
new Object[]{
		E_GC4204,	// E_GC4204_CONNECTION_STRING_DUP_EQUAL
	"Invalid connection string has duplicate '=' character."},
new Object[]{
		E_GC4205,	// E_GC4205_CONNECTION_STRING_MISSING_KEY
	"Invalid connection string has " +
	"missing keyword before '=' character."},
new Object[]{
		E_GC4206,	// E_GC4206_CONNECTION_STRING_MISSING_QUOTE
	"Invalid connection string has unmatched quote character."},
new Object[]{
		E_GC4207,	// E_GC4207_CONNECTION_STRING_MISSING_PAREN
	"Invalid connection string has unmatched parenthesis character."},

new Object[]{
		E_GC480D,	// E_GC480D_IDLE_LIMIT
	"Connection aborted by server because idle limit was exceeded." },
new Object[]{
		E_GC480E,	// E_GC480E_CLIENT_MAX
	"Server rejected connection request because client limit exceeded." },
new Object[]{
		E_GC4811,	// E_GC4811_NO_STMT
	"Statement does not exist.  It may have already been closed." },
new Object[]{
		E_GC481E,	// E_GC481E_SESS_ABORT
	"Server aborted connection by administrator request." },
new Object[]{
		E_GC481F,	// E_GC481F_SVR_CLOSED
	"Server is not currently accepting connection requests." },

new Object[]{
		E_AP0009,	// E_AP0009_QUERY_CANCELLED
	"Query cancelled."},
};

public const String cp_db = "CP_db";
		public const String cp_usr = "CP_usr";
		public const String cp_pwd = "CP_pwd";
		public const String cp_grp = "CP_grp";
		public const String cp_role = "CP_role";
		public const String cp_dbusr = "CP_dbusr";
		public const String cp_dbpwd = "CP_dbpwd";
		public const String cp_pool = "CP_pool";
		public const String cp_xacm = "CP_xacm";
		public const String cp_loop = "CP_loop";
		public const String cp_crsr = "CP_crsr";
		
//		public const String E_JD0001 = "E_JD0001";
//		public const String E_JD0002 = "E_JD0002";
//		public const String E_JD0003 = "E_JD0003";
//		public const String E_JD0004 = "E_JD0004";
		public const String E_JD0005 = "E_JD0005";
		public const String E_JD0006 = "E_JD0006";
		public const String E_JD0007 = "E_JD0007";
		public const String E_JD0008 = "E_JD0008";
		public const String E_JD0009 = "E_JD0009";
		public const String E_JD000A = "E_JD000A";
		public const String E_JD000B = "E_JD000B";
//		public const String E_JD000C = "E_JD000C";
		public const String E_JD000D = "E_JD000D";
		public const String E_JD000E = "E_JD000E";
		public const String E_JD000F = "E_JD000F";
		public const String E_JD0010 = "E_JD0010";
		public const String E_JD0011 = "E_JD0011";
		public const String E_JD0012 = "E_JD0012";
		public const String E_JD0013 = "E_JD0013";
		public const String E_JD0014 = "E_JD0014";
		public const String E_JD0015 = "E_JD0015";
		public const String E_JD0016 = "E_JD0016";
		public const String E_JD0017 = "E_JD0017";
//		public const String E_JD0018 = "E_JD0018";
		public const String E_JD0019 = "E_JD0019";
		public const String E_JD0020 = "E_JD0020";
		public const String E_JD0021 = "E_JD0021";
		public const String E_JD0022 = "E_JD0022";
//		public const String E_GC4201 = "E_GC4201";
//		public const String E_GC4202 = "E_GC4202";
//		public const String E_GC4203 = "E_GC4203";
//		public const String E_GC4204 = "E_GC4204";
//		public const String E_GC4205 = "E_GC4205";
//		public const String E_GC4206 = "E_GC4206";
		public const String E_JD0029 = "E_JD0029";

		/*
		** Name: ListResourceBundle.getContents
		**
		** Description:
		**	Returns an array of the defined key/value pairs.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Object[][]	Key/value pair array.
		**
		** History:
		**	 2-Sep-99 (gordy)
		**	    Created.*/

		protected Object[][] getContents()
		{
			return info;
		}  // getContents
		
		
		/*
		** The following static data, method and class definition 
		** are only needed in the resource base class Rsrc.*/
		
		private static ResourceBundle resource = null;
		
		
		/*
		** Name: getResource
		**
		** Description:
		**	Returns the resource for the default Locale.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	ResourceBundle	Resource for default Locale.
		**
		** History:
		**	18-Apr-00 (gordy)
		**	    Created.*/
		
		public static ResourceBundle getResource()
		{
			if (resource == null)
			{
				/*
					** Load the Locale specific resources for the default Locale
					** and make sure that a resource bundle (even empty) exists.
					*/
				try
				{
					//resource = ResourceBundle.getBundle("ingres.util.Rsrc");
					resource = new ErrRsrc();
				}
				catch //(System.Exception ignore)
				{
					resource = new EmptyRsrc();
				}
			}  // endif (resource == null)

			return (resource);
		}  // getResource

		public override Object getObject(String key)
		{
			System.Object[][] contents = this.getContents();
			for (int i =0; i< contents.Length; i++)
			{
				if (((String)(contents[i][0]) == key))
					return contents[i][1];
			}
			return null;
		} // getObject

		
		
		/*
		** Name: EmptyRsrc
		**
		** Description:
		**	ResourceBundle sub-class which provides no resources.
		**	This class is used when the actual resource class can
		**	not be accessed to avoid a null reference.
		**
		** History:
		**	 2-Sep-99 (gordy)
		**	    Created.*/
		
		private class EmptyRsrc : ListResourceBundle
		{
			private static System.Object[][] contents = new System.Object[0][];

			public Object[][] getContents()
			{
				return (contents);
			} // getContents

			public override Object getObject(String key)
			{
				return contents[0];
			} // getObject

		}
		// class EmptyRsrc
	}
	// class ErrRsrc


	/*
		** Name: ResourceBundle
		**
		** Description:
		**	ResourceBundle abstract class contains locale-specific objects.
		**	Since .NET does not have counterpart yet we define it here
		**	and just throw a MissingResourceException.
		**
		** History:
		**	25-Jul-02 (thoda04)
		**	    Created.*/
		
	internal abstract class ResourceBundle:Object
	{
		public static ResourceBundle getBundle(String baseName)
		{
			throw new MissingResourceException(); // resource not implemented
		}

		public String getString(String key)
		{
			return (String) getObject(key);
		} // getString

		public abstract Object getObject(String key);
	}
	// class ResourceBundle


	/*
		** Name: ListResourceBundle
		**
		** Description:
		**	ListResourceBundle abstract class manages resources as a list.
		**	Since .NET does not have counterpart yet we define it here
		**	and just throw a MissingResourceException.
		**
		** History:
		**	25-Jul-02 (thoda04)
		**	    Created.*/
		
	internal abstract class ListResourceBundle:ResourceBundle
	{
	}
	// class ResourceBundle


	/*
		** Name: MissingResourceException
		**
		** Description:
		**	Exception for no resource bundle could be found for the 
		**	specified base name.
		**	Since .NET does not have counterpart yet we define it here.
		**
		** History:
		**	25-Jul-02 (thoda04)
		**	    Created.
		*/

	[Serializable()]
	internal sealed class MissingResourceException : Exception
	{
		public MissingResourceException() : base()
		{
		}

		public MissingResourceException(string msg) :
			base(msg)
		{
		}

		public MissingResourceException(string msg, Exception e) :
			base(msg, e)
		{
		}

		private MissingResourceException(
			SerializationInfo info, StreamingContext context)
				:base(info, context)     // Deserialization constructor
		{
		}

		[SecurityPermissionAttribute(
			 SecurityAction.Demand, SerializationFormatter=true)]
		public override void GetObjectData(SerializationInfo info,
			StreamingContext context)
			// Serialization method
		{
			base.GetObjectData(info, context);
		}

	}
	// exception MissingResourceException

}