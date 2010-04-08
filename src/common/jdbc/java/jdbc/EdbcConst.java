/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.jdbc;

/*
** Name: EdbcConst.java
**
** Description:
**	EDBC JDBC Driver constants.
**
**  Interfaces:
**
**	EdbcConst
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
**	    Bumping version to 1.1 for first bug fix JDBC 2.0 version.
**	20-Aug-01 (gordy)
**	    Added connection parameter for default cursor mode.
**      01-Oct-01 (loera01) SIR 105641
**          Bumped version to 1.2 to include extensions to DB proc syntax.
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
**	 9-Sep-02 (gordy)
**	    Bumped version to 1.6 for the following bug fixes:
**	    BUG 108475 - Ingres empty dates.
**	    BUG 108554 - Updated character set mappings, UTF8 support.
**	27-Jan-03 (gordy)
**	    Bumped version to 1.7 for the following bug fixes:
**	    BUG 108833 - Use 'SET SESSION' to set persistent xact iso level.
**	    BUG 109484 - Fix Java encoding for ISO8859-15.
**      10-Apr-03 (weife01) BUG 110037
**	    Bumped version to 1.8 for the following bug fix:
**          BUG 110037 - Added parseConnDate and formatConnDate to synchronize
**                       the DateFormat returned by getConnDateFormat.
**	 8-Oct-03 (gordy)
**	    Bumping version to 1.9 for the following bug fixes:
**	    BUG 109535 - Mapping of EBCDIC character set.
**	    BUG 109798 - Changes in Java 3.0 XA DataSource protocol.
**	    BUG 110079 - Procedure parameter indexing.
**	    BUG 110460 - Mapping of KOREAN character set.
**	    BUG 110772 - Connection locking and multi-threading.
**	    BUG 111022 - Mapping of NCHAR, NVARCHAR and LONG NVARCHAR.
**	    BUG 111029 - IDMS implementation of iicolumns.
**	12-Jan-04 (gordy)
**	    Bumping version to 1.10 for the following bug fixes:
**	    BUG 110950 - Use default timezone for date only values.
**	    BUG 111088 - Implement getSchemas() for pre-65 catalogs.
**	    BUG 111119 - Pattern processing for IDMS in getColumns().
**	    BUG 111571 - XA changes for BEA server.
**	28-Apr-04 (gordy)
**	    Bumping version to 1.11 for the following bug fixes:
**	    BUG 111723 - Meta-Data mapping of column type for gateways.
**	    BUG 111769 - Validate timestamp objects.
**	    BUG 112045 - Support TMJOIN of an XA transaction.
**	19-Jul-04 (gordy)
**	    Bumping version to 1.12 for the following bug fixes:
**	    BUG 112279 - Prepared statement optimizations.
**	    BUG 112452 - Server character set sent as ASCII string.
**	    BUG 112665 - Transaction Isolation substitution.
**	 4-Oct-04 (gordy)
**	    Bumping version to 1.13 for the following bug fixes:
**	    BUG 112774 - Date string validation.
**  18-Jun-07 (weife01)
**      Bumping version to 1.14 for the following bug fixes:
**      BUG 117107 - Fix prepare statement during autocommit.
**      SIR 117451 - Add support for catalog functions.
**      BUG 118474 - Synchronize dateFormatter for time, timestamp.
*/

import	ca.edbc.util.EdbcRsrc;
import	ca.edbc.util.MsgConst;


/*
** Name: EdbcConst
**
** Description:
**	EDBC JDBC Driver constants.
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
**	    Added connection parameter for default cursor mode.
*/

public interface
EdbcConst
{

    /*
    ** Driver constants
    */
    String	fullTitle = "CA-EDBC JDBC Driver";
    String	shortTitle = "EDBC";
    
    int		majorVersion = 1;
    int		minorVersion = 14;
    
    String	driverID = fullTitle + " [" + majorVersion + "." +
					      minorVersion + "]";
    String	protocol = "jdbc:edbc:";
    
    /*
    ** Driver connection properties and URL attributes.
    ** The 'database' property is a special case since
    ** it is generally extracted from the URL but is
    ** not a URL attribute.
    */
    String	cp_db	= "database";	    // Connection property names
    String	cp_usr	= "user";
    String	cp_pwd	= "password";
    String	cp_grp	= "group";
    String	cp_role	= "role";
    String	cp_dbusr= "dbms_user";
    String	cp_dbpwd= "dbms_password";
    String	cp_pool	= "connect_pool";
    String	cp_xacm	= "autocommit_mode";
    String	cp_loop	= "select_loop";
    String	cp_crsr = "cursor_mode";

    PropInfo	propInfo[] =		    // Connection properties
    {
	new PropInfo( cp_db,	true,	"Database", 
			EdbcRsrc.cp_db,		MsgConst.JDBC_CP_DATABASE ),
	new PropInfo( cp_usr,	true,	"User ID",
			EdbcRsrc.cp_usr,	MsgConst.JDBC_CP_USERNAME ),
	new PropInfo( cp_pwd,	true,	"Password",
			EdbcRsrc.cp_pwd,	MsgConst.JDBC_CP_PASSWORD ),
	new PropInfo( cp_grp,	false,	"Group ID",
			EdbcRsrc.cp_grp,	MsgConst.JDBC_CP_GROUP_ID),
	new PropInfo( cp_role,	false,	"Role ID",
			EdbcRsrc.cp_role,	MsgConst.JDBC_CP_ROLE_ID),
	new PropInfo( cp_dbusr,	false,	"DBMS User ID",
			EdbcRsrc.cp_dbusr,	MsgConst.JDBC_CP_DBUSERNAME),
	new PropInfo( cp_dbpwd,	false,	"DBMS Password",
			EdbcRsrc.cp_dbpwd,	MsgConst.JDBC_CP_DBPASSWORD ),
	new PropInfo( cp_pool,	false,	"Connection Pool",
			EdbcRsrc.cp_pool,	MsgConst.JDBC_CP_CONNECT_POOL ),
	new PropInfo( cp_xacm,	false,	"Autocommit Mode",
			EdbcRsrc.cp_xacm,	MsgConst.JDBC_CP_AUTO_MODE ),
	new PropInfo( cp_loop,	false,	"Select Loops",
			EdbcRsrc.cp_loop,	MsgConst.JDBC_CP_SELECT_LOOP ),
	new PropInfo( cp_crsr,	false,	"Cursor Mode",
			EdbcRsrc.cp_crsr,	MsgConst.JDBC_CP_CURSOR_MODE ),
    };

    AttrInfo	attrInfo[] =		    // Attribute name mapping
    {
	new AttrInfo( "UID", cp_usr ),
	new AttrInfo( "PWD", cp_pwd ),
	new AttrInfo( "GRP", cp_grp ),
	new AttrInfo( "ROLE", cp_role ),
	new AttrInfo( "DBUSR", cp_dbusr ),
	new AttrInfo( "DBPWD", cp_dbpwd ),
	new AttrInfo( "POOL", cp_pool ),
	new AttrInfo( "AUTO", cp_xacm ),
	new AttrInfo( "LOOP", cp_loop ),
	new AttrInfo( "CURSOR", cp_crsr ),
    };


/*
** Name: PropInfo
**
** Description:
**	Information associated with a Driver property.
**
**  Public Data:
**
**	name		Name of the property.
**	req		Is property required.
**	desc		Description of the property (Locale specific).
**	rsrcID		Resource ID.
**	cpID		Connection parameter ID.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Added ID to load description (also new) from resource.
*/

static class
PropInfo
{

    public String   name;	// Property name.
    public boolean  req;	// Is property required.
    public String   desc;	// Property description.
    public String   rsrcID;	// Resource ID.
    public short    cpID;	// Connection parameter ID.


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
**	    Added ID and description.
*/

public
PropInfo( String name, boolean req, String desc, String rsrcID, short cpID )
{

    this.name = name;
    this.req = req;
    this.desc = desc;
    this.rsrcID = rsrcID;
    this.cpID = cpID;

} // PropInfo

} // class PropInfo


/*
** Name: AttrInfo
**
** Description:
**	Provides a mapping between a URL attribute and a Driver property.
**
**  Public Data:
**
**	attrName	Name of the attribute.
**	propName	Name of the associated property.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
*/

static class
AttrInfo
{

    public String   attrName;	// Attribute name.
    public String   propName;	// Associated property name.


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
**	    Created.
*/

public
AttrInfo( String attr, String prop )
{

    attrName = attr;
    propName = prop;

} // AttrInfo

} // class AttrInfo

} // interface EdbcConst


