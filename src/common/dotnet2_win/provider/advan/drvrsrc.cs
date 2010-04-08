/*
** Copyright (c) 1999, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: drvrsrc.cs
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
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	16-feb-10 (thoda04) Bug 123298
	**	    Added SendIngresDates support to send .NET DateTime parms
	**	    as ingresdate type rather than as timestamp_w_timezone.
	*/

	/*
	** Name: DrvRsrc
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
	*/
	
	internal class DrvRsrc : ListResourceBundle
	{
		/*
		** Connection property keys
		*/
		public const String	RSRC_CP_DB      = "RSRC_CP_DB";
		public const String	RSRC_CP_USR     = "RSRC_CP_USR";
		public const String	RSRC_CP_PWD     = "RSRC_CP_PWD";
		public const String	RSRC_CP_GRP     = "RSRC_CP_GRP";
		public const String	RSRC_CP_ROLE    = "RSRC_CP_ROLE";
		public const String	RSRC_CP_DBUSR   = "RSRC_CP_DBUSR";
		public const String	RSRC_CP_DBPWD   = "RSRC_CP_DBPWD";
		public const String	RSRC_CP_POOL    = "RSRC_CP_POOL";
		public const String	RSRC_CP_XACM    = "RSRC_CP_XACM";
		public const String	RSRC_CP_LOOP    = "RSRC_CP_LOOP";
		public const String	RSRC_CP_CRSR    = "RSRC_CP_CRSR";
		public const String	RSRC_CP_VNODE   = "RSRC_CP_VNODE";
		public const String	RSRC_CP_ENCODE  = "RSRC_CP_ENCODE";
		public const String	RSRC_CP_TZ      = "RSRC_CP_TZ";
		public const String	RSRC_CP_DEC_CHR = "RSRC_CP_DEC_CHR";
		public const String	RSRC_CP_DTE_FMT = "RSRC_CP_DTE_FMT";
		public const String	RSRC_CP_MNY_FMT = "RSRC_CP_MNY_FMT";
		public const String	RSRC_CP_MNY_PRC = "RSRC_CP_MNY_PRC";
		public const String	RSRC_CP_BLANKDATE="RSRC_CP_BLANKDATE";
		public const String RSRC_CP_SENDINGRESDATES = "RSRC_CP_SENDINGRESDATES";

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


		private static System.Object[][] info =
		{
			/*
			** Connection properties.
			*/
			new System.Object[]{ RSRC_CP_DB,     	"Database" },
			new System.Object[]{ RSRC_CP_USR,    	"User ID" },
			new System.Object[]{ RSRC_CP_PWD,    	"Password" },
			new System.Object[]{ RSRC_CP_GRP,    	"Group ID" },
			new System.Object[]{ RSRC_CP_ROLE,   	"Role ID" },
			new System.Object[]{ RSRC_CP_DBUSR,  	"DBMS User ID" },
			new System.Object[]{ RSRC_CP_DBPWD,  	"DBMS Password" },
			new System.Object[]{ RSRC_CP_POOL,   	"Connection Pool" },
			new System.Object[]{ RSRC_CP_XACM,   	"Autocommit Mode" },
			new System.Object[]{ RSRC_CP_LOOP,   	"Select Loops" },
			new System.Object[]{ RSRC_CP_CRSR,   	"Cursor Mode" },
			new System.Object[]{ RSRC_CP_VNODE,  	"VNODE Usage" },
			new System.Object[]{ RSRC_CP_ENCODE, 	"Character Encoding" },
			new System.Object[]{ RSRC_CP_TZ,     	"Timezone" },
			new System.Object[]{ RSRC_CP_DEC_CHR,	"Decimal Character" },
			new System.Object[]{ RSRC_CP_DTE_FMT,	"Date Format" },
			new System.Object[]{ RSRC_CP_MNY_FMT,	"Money Format" },
			new System.Object[]{ RSRC_CP_MNY_PRC,	"Money Precision" },
			new System.Object[]{ RSRC_CP_BLANKDATE,	"Blank Date" },
		};


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
		** are only needed in the Ingres resource base class Rsrc.*/
		
		private static ResourceBundle resource = null;
		
		
		/*
		** Name: getResource
		**
		** Description:
		**	Returns the Ingres resource for the default Locale.
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
		
		internal static ResourceBundle getResource()
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
					resource = new DrvRsrc();
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
	// class Rsrc


}