/*
** Copyright (c) 1999, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: drvconst.cs
	**
	** Description:
	**	.NET Data Provider Advanatage Driver constants.
	**
	**  Structures:
	**
	**	DrvConst
	**
	**  Classes:
	**
	**	PropInfo
	**	AttrInfo
	**
	** History:
	**	 6-May-99 (gordy)
	**	    Created.
	**	18-Nov-99 (gordy)
	**	    Nested the local utility classes.
	**	14-Jan-00 (gordy)
	**	    Added Connection pool property.
	**	25-Apr-00 (gordy)
	**	    Converted to interface after after extracting utility
	**	    methods into other packages.
	**	19-May-00 (gordy)
	**	    Added select loops connection property and attribute.
	**	17-Oct-00 (gordy)
	**	    Added autocommit mode connection property and attribute.
	**	15-Nov-00 (gordy)
	**	    Set driver version for first non-beta release.
	**	28-Mar-01 (gordy)
	**	    Removed propDB.
	**	28-Mar-01 (gordy)
	**	    Added driverID.  Removed propDB as the property info 
	**	    may be accessed via its cp_db key.  Renamed effective
	**	    user to DBMS user to be generic and pair with DBMS pwd.
	**	18-Apr-01 (gordy)
	**	    Followup to previous change of effective user: also change
	**	    the connection parameter ID.
	**	22-Jun-01 (gordy)
	**	    Bumping version to 1.1 for first bug fix 2.0 version.
	**	20-Aug-01 (gordy)
	**	    Added connection parameter for default cursor mode.
	**	01-Oct-01 (loera01) SIR 105641
	**	    Bumped version to 1.2 to include extensions to DB proc syntax.
	**	30-Nov-01 (gordy)
	**	    Bumped version to 1.3 for gateway compatibility fixes.
	**	 5-Feb-02 (gordy)
	**	    Bumped version to 1.4 for quoted identfiers, tracing timestamps,
	**	    decimal to float conversion for gateways and null parameters in
	**	    setXXX() methods.
	**	19-Jul-02 (gordy)
	**	    Bumped version to 1.5 for the following bug fixes:
	**	    BUG 107269 - Exceptions from tracing when running in applet.
	**	    BUG 108004 - Trim schema names returned by getSchemas().
	**	    BUG 108286 - Close DBMS cursor before local result-set for BLOBs.
	**	    BUG 108288 - Physical table structure info in getPrimaryKeys().
	**	22-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	 9-Sep-02 (gordy)
	**	    Bumped version to 1.6 for the following bug fixes:
	**	    BUG 108475 - Ingres empty dates.
	**	    BUG 108554 - Updated character set mappings, UTF8 support.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	 3-Jul-06 (gordy)
	**	    Added DBMS key for tracing.
	**	27-feb-07 (thoda04)SIR 117018
	**	    Change CharacterEncoding to Charset.
	**	16-feb-10 (thoda04) Bug 123298
	**	    Added SendIngresDates support to send .NET DateTime parms
	**	    as ingresdate type rather than as timestamp_w_timezone.
	*/


	/*
	** Name: DrvConst
	**
	** Description:
	**	Driver constants.
	**
	**  Constants:
	**
	**	fullTitle	Long driver name.
	**	shortTitle	Short driver name.
	**	majorVersion	Primary driver version.
	**	minorVersion	Secondary driver version.
	**	driverID	Name and version of driver.
	**	cp_db		Database connection property.
	**	cp_usr		User connection property.
	**	cp_pwd		Password connection property.
	**	cp_eusr		Effective user connection property.
	**	cp_grp		Group connection property.
	**	cp_role		Role connection property.
	**	cp_dbpwd	DBMS password connection property.
	**	cp_pool		Connection pool connection property.
	**	cp_xacm		Autocommit mode connection property.
	**	cp_loop		Select loops connection property.
	**	cp_crsr		Cursor mode connection property.
	**	propInfo	Array of driver properties.
	**	attrInfo	Array of driver attributes.
	**
	** History:
	**	 6-May-99 (gordy)
	**	    Created.
	**	17-Sep-99 (rajus01)
	**	    Added connection parameters support.
	**	14-Jan-00 (gordy)
	**	    Added Connection pool property.
	**	25-Apr-00 (gordy)
	**	    Converted to interface after after extracting utility
	**	    methods into other packages.
	**	19-May-00 (gordy)
	**	    Added select loops connection property and attribute.
	**	17-Oct-00 (gordy)
	**	    Added autocommit mode connection property and attribute.
	**	28-Mar-01 (gordy)
	**	    Added driverID and protocol.  Removed propDB as the property 
	**	    info may be accessed via its cp_db key.  Renamed effective
	**	    user to DBMS user to be generic and pair with DBMS pwd.
	**	18-Apr-01 (gordy)
	**	    Followup to previous change of effective user: also change
	**	    the connection parameter ID.
	**	20-Aug-01 (gordy)
	**	    Added connection parameter for default cursor mode.*/
	
	/*
	** Driver constants
	*/
	/*
	** Driver connection properties and URL attributes.
	** The 'database' property is a special case since
	** it is generally extracted from the URL but is
	** not a URL attribute.
	*/

	/// <summary>
	/// Class to hold provider's driver constants.
	/// </summary>
	public sealed class DrvConst
	{
		/*
		** Ingres Layer Constants.
		*/
		internal const String fullTitle  = "Ingres .NET Provider";
		internal const String shortTitle = "Ingres";
		const int majorVersion = 1;
		const int minorVersion = 6;
		internal static String driverID = DrvConst.fullTitle +
			" [" + DrvConst.majorVersion + "." + DrvConst.minorVersion + "]";
		internal const String DRV_DBMS_KEY = "dbms";
		internal const String DRV_TRACE_ID = "drv";

		/*
		** Cursor concurrency modes
		*/
		internal const int DRV_CRSR_DBMS     = -1;
		internal const int DRV_CRSR_READONLY =  0;
		internal const int DRV_CRSR_UPDATE   =  1;

		/*
		** Cursor type modes
		*/
		internal const int TYPE_FORWARD_ONLY = 0;

		/*
		** Cursor holdability modes
		*/
		internal const int CLOSE_CURSORS_AT_COMMIT  = 1;
		internal const int HOLD_CURSORS_OVER_COMMIT = 2;

		/*
		** Non-DBMS dbcapability ID's returned by GCD server.
		*/
		internal const String	DRV_DBCAP_CONNECT_LVL0	= "JDBC_CONNECT_LEVEL";
		internal const String	DRV_DBCAP_CONNECT_LVL1	= "DBMS_CONNECT_LEVEL";

		/*
		** The following connection properties are internal to the driver.
		** Negative values are used since DAM-ML values are positive.
		*/
		const short	DRV_CP_SELECT_LOOP	= -1;
		const short	DRV_CP_CURSOR_MODE	= -2;
		const short	DRV_CP_CHAR_ENCODE	= -3;

		/*
		** Driver connection properties and URL attributes.
		** The 'database' property is a special case since
		** it is generally extracted from the URL but is
		** not a URL attribute.
		*/
		/// <summary>
		/// Database name to connect to.
		/// </summary>
		public   const String	DRV_PROP_DB 		= "database";
		/// <summary>
		/// User name to logon to database with.
		/// </summary>
		public   const String	DRV_PROP_USR		= "user";
		/// <summary>
		/// Password to logon to database with.
		/// </summary>
		public   const String	DRV_PROP_PWD		= "password";
		/// <summary>
		/// Group name.
		/// </summary>
		public   const String	DRV_PROP_GRP		= "group";
		/// <summary>
		/// Role name.
		/// </summary>
		public   const String	DRV_PROP_ROLE		= "role";
		/// <summary>
		/// DBMS user name. Equivalent to Ingres -u option.
		/// </summary>
		public   const String	DRV_PROP_DBUSR		= "dbms_user";
		/// <summary>
		/// DBMS password. Equivalent to Ingres -P option.
		/// </summary>
		public   const String	DRV_PROP_DBPWD		= "dbms_password";
		internal const String	DRV_PROP_POOL		= "connect_pool";
		internal const String	DRV_PROP_XACM		= "autocommit_mode";
		internal const String	DRV_PROP_VNODE		= "vnode_usage";
		internal const String	DRV_PROP_LOOP		= "select_loop";
		internal const String	DRV_PROP_CRSR		= "cursor_mode";
		internal const String	DRV_PROP_ENCODE		= "character encoding";
		internal const String	DRV_PROP_TIMEZONE	= "timezone";
		internal const String	DRV_PROP_DEC_CHAR	= "decimal_char";
		internal const String	DRV_PROP_DATE_FRMT	= "date_format";
		internal const String	DRV_PROP_MNY_FRMT	= "money_format";
		internal const String	DRV_PROP_MNY_PREC	= "money_precision";

		/// <summary>
		/// Server containg the DAS server.
		/// </summary>
		public   const String	DRV_PROP_HOST       = "Host";
		/// <summary>
		/// Port listen address.
		/// </summary>
		public   const String	DRV_PROP_PORT       = "Port";
		/// <summary>
		/// Role password.
		/// </summary>
		public   const String	DRV_PROP_ROLEPWD    = "Role PWD";
		/// <summary>
		/// Connection Timeout.
		/// </summary>
		public   const String	DRV_PROP_TIMEOUT    = "Connection Timeout";
		internal const String	DRV_PROP_TRACE      = "Trace";
		internal const String	DRV_PROP_TRACELVL   = "TraceLevel";
		internal const String	DRV_PROP_MINPOOLSIZE= "MinPoolSize";
		internal const String	DRV_PROP_MAXPOOLSIZE= "MaxPoolSize";
		internal const String	DRV_PROP_BLANKDATE  = "BlankDate";
		/// <summary>
		/// Automatically enlist the transaction in the global transaction.
		/// </summary>
		public   const String	DRV_PROP_ENLISTDTC  = "Enlist";
		/// <summary>
		/// Return or not the password information in then
		/// connection string property.
		/// </summary>
		public   const String	DRV_PROP_PERSISTSEC = "Persist Security Info";
		/// <summary>
		/// Keyword for disabling the connection pooling on the client side.
		/// </summary>
		public   const String	DRV_PROP_CLIENTPOOL = "Pooling";
		/// <summary>
		/// Keyword for sending .NET DateTime parameter as 
		/// ingresdate data type rather than as ANSI timestamp with timezone.
		/// </summary>
		public   const String	DRV_PROP_SENDINGRESDATES = "SendIngresDates";

		static String[]	prop_valid_off_on	=
			new String[] { "off", "on" };
		static String[]	prop_xacm_valid  	=
			new String[] { "dbms", "single", "multi" };
		static String[]	prop_vnode_valid 	=
			new String[] { "connect", "login" };
		static String[]	prop_crsr_valid  	=
			new String[] { "dbms", "readonly", "update" };

		internal static PropInfo[] propInfo = new PropInfo[]
		{
				new PropInfo( DRV_PROP_DB,	true,	"Database", null, null,
				DrvRsrc.RSRC_CP_DB,	MsgConst.MSG_CP_DATABASE ),

				new PropInfo( DRV_PROP_USR,	false,	"User ID", null, null,
				DrvRsrc.RSRC_CP_USR,	MsgConst.MSG_CP_USERNAME ),

				new PropInfo( DRV_PROP_PWD,	false,	"Password", null, null,
				DrvRsrc.RSRC_CP_PWD,	MsgConst.MSG_CP_PASSWORD ),

				new PropInfo( DRV_PROP_GRP,	false,	"Group ID", null, null,
				DrvRsrc.RSRC_CP_GRP,	MsgConst.MSG_CP_GROUP_ID),

				new PropInfo( DRV_PROP_ROLE,	false,	"Role ID", null, null,
				DrvRsrc.RSRC_CP_ROLE,	MsgConst.MSG_CP_ROLE_ID),

				new PropInfo( DRV_PROP_DBUSR,	false,	"DBMS User ID", null, null,
				DrvRsrc.RSRC_CP_DBUSR,	MsgConst.MSG_CP_DBUSERNAME),

				new PropInfo( DRV_PROP_DBPWD,	false,	"DBMS Password", null, null,
				DrvRsrc.RSRC_CP_DBPWD,	MsgConst.MSG_CP_DBPASSWORD ),

				new PropInfo( DRV_PROP_POOL,	false,	"Connection Pool", 
				null, prop_valid_off_on,
				DrvRsrc.RSRC_CP_POOL,	MsgConst.MSG_CP_CONNECT_POOL ),

				new PropInfo( DRV_PROP_XACM,	false,	"Autocommit Mode",
				prop_xacm_valid[0], prop_xacm_valid,
				DrvRsrc.RSRC_CP_XACM,	MsgConst.MSG_CP_AUTO_MODE ),

				new PropInfo( DRV_PROP_VNODE,	false,	"VNODE Usage",
				prop_vnode_valid[0], prop_vnode_valid,
				DrvRsrc.RSRC_CP_VNODE,	MsgConst.MSG_CP_LOGIN_TYPE ),

				new PropInfo( DRV_PROP_LOOP,	false,	"Select Loops",
				prop_valid_off_on[0], prop_valid_off_on,
				DrvRsrc.RSRC_CP_LOOP,	DRV_CP_SELECT_LOOP ),

				new PropInfo( DRV_PROP_CRSR,	false,	"Cursor Mode",
				prop_crsr_valid[0], prop_crsr_valid,
				DrvRsrc.RSRC_CP_CRSR,	DRV_CP_CURSOR_MODE ),
	
				new PropInfo( DRV_PROP_ENCODE, false, "Character Encoding", null, null,
				DrvRsrc.RSRC_CP_ENCODE,	DRV_CP_CHAR_ENCODE ),
	
				new PropInfo( DRV_PROP_TIMEZONE, false, "Timezone", null, null,
				DrvRsrc.RSRC_CP_TZ, MsgConst.MSG_CP_TIMEZONE ),
	
				new PropInfo( DRV_PROP_DEC_CHAR, false, "Decimal Character", null, null,
				DrvRsrc.RSRC_CP_DEC_CHR, MsgConst.MSG_CP_DECIMAL ),
	
				new PropInfo( DRV_PROP_DATE_FRMT, false, "Date Format", null, null,
				DrvRsrc.RSRC_CP_DTE_FMT, MsgConst.MSG_CP_DATE_FRMT ),
			
				new PropInfo( DRV_PROP_MNY_FRMT, false, "Money Format", null, null,
				DrvRsrc.RSRC_CP_MNY_FMT, MsgConst.MSG_CP_MNY_FRMT ),
	
				new PropInfo( DRV_PROP_MNY_PREC, false, "Money Precision", null, null,
				DrvRsrc.RSRC_CP_MNY_PRC, MsgConst.MSG_CP_MNY_PREC ),
	
				new PropInfo( DRV_PROP_BLANKDATE, false, "Blank Date", null, null,
				DrvRsrc.RSRC_CP_BLANKDATE, MsgConst.MSG_CP_BLANKDATE ),
	
				new PropInfo( DRV_PROP_SENDINGRESDATES, false, "SendIngresDates",
				null, null,
				DrvRsrc.RSRC_CP_SENDINGRESDATES, MsgConst.MSG_CP_SENDINGRESDATES ),
		};

		internal static AttrInfo[] attrInfo = new AttrInfo[]
		{
				new AttrInfo( "UID", DRV_PROP_USR ),
				new AttrInfo( "PWD", DRV_PROP_PWD ),
				new AttrInfo( "GRP", DRV_PROP_GRP ),
				new AttrInfo( "ROLE", DRV_PROP_ROLE ),
				new AttrInfo( "DBUSR", DRV_PROP_DBUSR ),
				new AttrInfo( "DBPWD", DRV_PROP_DBPWD ),
				new AttrInfo( "POOL", DRV_PROP_POOL ),
				new AttrInfo( "AUTO", DRV_PROP_XACM ),
				new AttrInfo( "VNODE", DRV_PROP_VNODE ),
				new AttrInfo( "LOOP", DRV_PROP_LOOP ),
				new AttrInfo( "CURSOR", DRV_PROP_CRSR ),
				new AttrInfo( "CHARACTERENCODING", DRV_PROP_ENCODE ),
				new AttrInfo( "TZ", DRV_PROP_TIMEZONE ),
				new AttrInfo( "DECIMAL", DRV_PROP_DEC_CHAR ),
				new AttrInfo( "DATE_FMT", DRV_PROP_DATE_FRMT ),
				new AttrInfo( "MNY_FMT", DRV_PROP_MNY_FRMT ),
				new AttrInfo( "MNY_PREC", DRV_PROP_MNY_PREC ),
				new AttrInfo( "BLANKDATE", DRV_PROP_BLANKDATE ),
		};

		/// <summary>
		/// Get the connection string keyword associated with the search key.
		/// </summary>
		/// <param name="key"></param>
		/// <returns>Connection string keyword.  E.g. "User ID".</returns>
		public   static String GetKey (string key)
		{
			if (key == null)
				return null;

			foreach (PropInfo info in propInfo)
			{
				if (key == info.name)
					return info.desc;
			}

			return key;
		}



	} // DrvConst


	/*
	** Name: PropInfo
	**
	** Description:
	**	Information associated with a Driver property.
	**
	**  Public Data:
	**
	**	name	    Property Name.
	**	req	    Is property required?
	**	desc	    Description.
	**	dflt	    Default value.
	**	valid	    List of valid values.
	**	rsrcID	    Resource ID.
	**	msgID	    DAM-ML MSG_CONNECT parameter ID.
	**
	** History:
	**	 6-May-99 (gordy)
	**	    Created.
	**	 8-Sep-99 (gordy)
	**	    Added ID to load description (also new) from resource.
	**	22-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	*/

	class PropInfo
	{
		
		public String   name;      // Property name.
		public bool     req;       // Is property required.
		public String   desc;      // Property description.
		public String   dflt;      // Default value.
		public String[] valid;     // List of valid values.
		public String   rsrcID;    // Resource ID.
		public short    msgID;     // Connection parameter ID.
		
		
		/*
		** Name: PropInfo
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	pID	    Property ID.
		**	name	    Property Name.
		**	desc	    Property Description.
		**	required    True if property is required, false otherwise.
		**	cID	    Associated connection parameter ID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 6-May-99 (gordy)
		**	    Created.
		**	 8-Sep-99 (gordy)
		**	    Added ID and description.*/
		
		public PropInfo(
			String    name,
			bool      req,
			String    desc,
			String    dflt,
			String[]  valid,
			String    rsrcID,
			short     msgID
			)
		{
			
			this.name   = name;
			this.req    = req;
			this.desc   = desc;
			this.dflt   = dflt;
			this.valid  = valid;
			this.rsrcID = rsrcID;
			this.msgID  = msgID;
			
		} // PropInfo
	}


	class AttrInfo
	{
		
		public String attrName; // Attribute name.
		public String propName; // Associated property name.
		
		
		/*
		** Name: AttrInfo
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	attr	Attribute Name.
		**	prop	Property Name.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 6-May-99 (gordy)
		**	    Created.*/
		
		public AttrInfo(String attr, String prop)
		{
			
			attrName = attr;
			propName = prop;
			
		} // AttrInfo
	}


}