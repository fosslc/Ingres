/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.jdbc;

/*
** Name: DrvConst.java
**
** Description:
**	Defines interface providing JDBC Driver constants.
**
**  Interfaces:
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
**	    Bumping version to 1.1 for first bug fix JDBC 2.0 version.
**	20-Aug-01 (gordy)
**	    Added connection parameter for default cursor mode.
**      01-Oct-01 (loera01) SIR 105641
**          Bumped version to 1.2 to include extensions to DB proc syntax.
**	 3-Nov-00 (gordy)
**	    Added date formatting info.
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
** 	 20-Sep-22 (wansh01)
** 	    Added login type connection property and attribute.
**	31-Oct-02 (gordy)
**	    Bumped version to 2.0 for generic driver implementation.
**	23-Dec-02 (gordy)
**	    Added dbcapability ID's for non-DBMS items, internal connection
**	    property definitions, new login_type connection property.
**	18-Feb-03 (gordy)
**	    Renamed DRV_PROP_LOGIN to DRV_PROP_VNODE.  Added default and
**	    valid property values.  Added DRV_JDBC_MAJ_VERS, DRV_JDBC_MIN_VERS.
**	 7-Jul-03 (gordy)
**	    Added driver properties and attributes for Ingres configuration.
**	12-Sep-03 (gordy)
**	    Moved date/time constants to SqlDates utility class.
**	28-Apr-04 (gordy)
**	    Bumped version to 2.1 for the following bug fixes:
**	    SIR 111507 - Support for BIGINT columns and parameters.
**	    BUG 111723 - Meta-Data mapping of column type for gateways.
**	    BUG 112045 - Support TMJOIN of an XA transaction.
**	    BUG	112200 - Don't allow NULL updates on non-nullable columns.
**	    BUG 112228 - Don't include time for updateObject( Date ).
**	    BUG 112234 - Allow for decimal(31,31).
**	19-Jul-04 (gordy)
**	    Bumped version to 2.2 for the following bug fixes:
**	    BUG 112279 - Prepared statement optimizations.
**	    BUG 112452 - Character-set adjustments.
**	    BUG 112665 - Substitute transaction isolation level.
**	    SIR 112690 - Enhanced password encryption.
**	18-Aug-04 (gordy)
**	    Bumped version to 2.3 for the following bug fixes:
**	    BUG 112848 - NCS parameter lengths.
**	 2-Feb-05 (gordy)
**	    Bumped version to 2.4 for the following bug fixes:
**	    BUG 113625 - XA rollback on active transaction.
**	    BUG 113659 - Savepoints & prepared statements.
**	19-Oct-05 (gordy)
**	    Bumped version to 2.5 for the following bug fixes:
**	    BUG 109809 - Decimal precision exceeds Ingres max.
**	    BUG 113625 - XA rollback on active transaction (comprehensive).
**	    BUG 115202 - Cursor closed errors.
**	    BUG 115203 - Result-set positioning for forward-only cursors.
**	    BUG 115204 - Convert short BLOB/CLOB to VARBYTE/VARCHAR.
**	    BUG 115412 - Handle errors during result-set data pre-load.
**	 4-May-06 (gordy)
**	    Bumped version to 2.6 for re-branding to Ingres:
**	    SIR 116068 - Change default cursor mode to READONLY
**	 6-Oct-06 (gordy)
**	    Version 3.0: support of ANSI date/time data types.
**	 5-Jan-07 (gordy)
**	    Bumped version to 3.1 for the following bug fixes:
**	    BUG 117078 - Query text greater than 64K.
**	    BUG 117107 - DDL invalidates prepared statements.
**	    BUG 117108 - Re-use prepared statement names.
**	30-Jan-07 (gordy)
**	    Backward compatibility for LOB Locators.
**	15-Feb-07 (gordy)
**	    Bumped version to 3.2 for the following bug fixes:
**	    BUG 117451 - Don't throw exceptions for catalog meta-data methods.
**	26-Feb-07 (gordy)
**	    Additional configuration properties for LOB Locators.
**	24-May-07 (gordy)
**	    Bumped version to 3.3 for LOB locators and scrollable cursors.
**	23-Jul-07 (gordy)
**	    Added connection property for date alias.
**	17-Aug-07 (gordy)
**	    Bumped version to 3.4 for following additional features:
**	    Pre-fetching of scrollable cursors.
**	    Date alias property.
**	12-Sep-07 (gordy)
**	    Added default empty date replacement value and config property
**	    at version 3.4.
**	12-Jun-08 (gordy)
**	    Bumped version to 3.5 for following bug fixes and features:
**	    SIR 119623 - Multiple DAS & extended symbolic ports.
**	    SIR 119917 - Support patch level versioning.
**	    BUG 119919 - Money meta-data.
**	    BUG 119928 - LOB term in error messages.
**	    BUG 120439 - Double meta-data.
**	    BUG 120463 - Max char length meta-data.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	30-Jan-09 (gordy)
**	    Bumped version to 3.6 for following bug fixes and enhancements:
**	    SIR 117537 - Decimal precision greater than 31.
**	    BUG 121345 - Escape clause in meta-data queries.
**	    SIR 121423 - Support getBytes() on character columns.
**	    BUG 121427 - Reduce DriverManager tracing overhead.
**	    BUG 121437 - Reduce DateFormat syncronization overhead.
**	    BUG 121538 - Code clean-up related to findbugs utility.
**	 3-Mar-09 (gordy)
**	    Add config setting to enable/disable scrollable cursors.
**	 1-Apr-09 (gordy)
**	    Bumped version to 3.7 for the following bug fixes and enhancements:
**	    BUG 121672 - No DataTruncation if date only value in getDate().
**	    SIR 121745 - Config setting for scrollable cursors.
**	    BUG 121789 - Cleanup of null LOB handling.
**	    BUG 121865 - Optimize patterns to equalities if no wild cards.
**	 5-May-09 (gordy)
**	    Bumped version to 3.8 for the following bug fixes and enhancements:
**	    BUG 121965 - Use VARBINARY for setObject( byte[] ).
**	    SIR 122036 - Multiple host/port connection targets.
**	 9-Oct-09 (gordy)
**	    New driver version 4.0 for JDBC 4.0 API.
**	11-Feb-10 (rajus01) Bug 123277
**	    Added connection property 'send_ingres_dates' to allow the
**	    driver to send the date/time/timestamp values as INGRESDATE
**	    datatype instead of TS_W_TZ (ANSI TIMESTAMP WITH TIMEZONE). 
**	25-Mar-10 (gordy)
**	    Bumped version to 4.0.1 for support of batch processing.
**	    Added driver property batch.enabled to allow batch requests
**	    to be executed as individual queries.
**	15-Apr-10 (gordy)
**	    Bumped version to 4.0.2 for positional DB proc parameters.
**	 1-Jun-10 (gordy)
**	    Bumped version to 4.0.3 for batch performance improvements.
**	20-Jul-10 (gordy)
**	    Bumped version to 4.0.4 for SQL comment handling.
**	30-Jul-10 (gordy)
**	    Added connection property 'send_integer_booleans' to allow
**	    the driver to send boolean values as tinyint.
**	13-Aug-10 (gordy)
**	    Bumped version to 4.0.5 for the following fixes and enhancements:
**	    SIR 124165 - Boolean to tinyint connection property.
**	    BUG 124214 - Fix isBeforeFirst() and isLast().
*/

import	com.ingres.gcf.dam.MsgConst;


/*
** Name: DrvConst
**
** Description:
**	JDBC Driver constants.
**
**  Constants:
**
**	DRV_VENDOR_NAME		Name of driver vendor.
**	DRV_MAJOR_VERSION	Primary driver version.
**	DRV_MINOR_VERSION	Secondary driver version.
**	DRV_PATCH_VERSION	Patch driver version for bug fixes.
**	DRV_JDBC_VERSION	JDBC version supported by driver.
**	DRV_JDBC_MAJ_VERS	JDBC major version supported.
**	DRV_JDBC_MIN_VERS	JDBC minor version supported.
**	DRV_JDBC_PROTOCOL_ID	Primary URL JDBC protocol.
**	DRV_PROPERTY_KEY	Property key prefix.
**	DRV_DBMS_KEY		DBMS key prefix.
**	DRV_TRACE_ID		Trace ID for driver tracing.
**	DSRC_TRACE_ID		Trace ID for data-source tracing.
**
**	DRV_CNF_LOB_CACHE	LOB cache configuration property.
**	DRV_CNF_LOB_SEGSIZE	LOB segment size configuration property.
**	DRV_CNF_LOB_LOCATORS	LOB Locator configuration property.
**	DRV_CNF_LOB_LOC_AUTO	LOB Locator/autocommit config property.
**	DRV_CNF_LOB_LOC_LOOP	LOB Locator/select loop config property.
**	DRV_CNF_EMPTY_DATE	Empty date replacement config property.
**	DRV_CNF_CURS_SCROLL	Scrollable cursor config property.
**	DRV_CNF_BATCH		Batch processing config property.
**
**	DRV_DFLT_SEGSIZE	Default LOB segment size.
**	DRV_DFLT_EMPTY_DATE	Default empty date value.
**
**	DRV_CRSR_DBMS		Cursor concurrency: DBMS default.
**	DRV_CRSR_READONLY	Cursor concurrency: read-only.
**	DRV_CRSR_UPDATE		Cursor concurrency: updatable.
**
**	DRV_DBCAP_CONNECT_LVL0	Entries for REQUEST DBMS Capabilities.
**	DRV_DBCAP_CONNECT_LVL1
**
**	DRV_CP_SELECT_LOOP	Connect parameter: select loops.
**	DRV_CP_CURSOR_MODE	Connect parameter: cursor mode.
**	DRV_CP_CHAR_ENCODE	Connect parameter: character encoding.
**
**	DRV_PROP_DB		Database connection property.
**	DRV_PROP_USR		User connection property.
**	DRV_PROP_PWD		Password connection property.
**	DRV_PROP_GRP		Group connection property.
**	DRV_PROP_ROLE		Role connection property.
**	DRV_PROP_DBUSR		Effective user connection property.
**	DRV_PROP_DBPWD		DBMS password connection property.
**	DRV_PROP_POOL		Connection pool connection property.
**	DRV_PROP_XACM		Autocommit mode connection property.
**	DRV_PROP_LOOP		Select loops connection property.
**	DRV_PROP_CRSR		Cursor mode connection property.
**	DRV_PROP_VNODE		VNODE usage connection property.
**	DRV_PROP_ENCODE		Character encoding connection property.
**	DRV_PROP_TIMEZONE	Timezone connection property.
**	DRV_PROP_DEC_CHAR	Decimal character connection property.
**	DRV_PROP_DATE_FRMT	Date format connection property.
**	DRV_PROP_MNY_FRMT	Money format connection property.
**	DRV_PROP_MNY_PREC	Money precision connection property.
**	DRV_PROP_DATE_ALIAS	Date alias connection property.
**	DRV_PROP_SND_ING_DTE	Send Ingres Dates connection property.
**	DRV_PROP_SND_INT_BOOL	Send integer booleans connection property.
**
**	prop_valid_off_on	Valid values for properties which are off/on.
**	prop_xacm_valid		Valid values for autocommit mode property.
**	prop_vnode_valid	Valid values for VNODE usage property.
**	prop_crsr_valid		Valid values for cursor mode property.
**	prop_valid_false_true	Valid values for send_ingres_dates property.
**
**  Public Data
**
**	propInfo		Array of driver properties.
**	attrInfo		Array of driver attributes.
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
**	 3-Nov-00 (gordy)
**	    Added date formatting info: D_EPOCH, T_EPOCH, TS_EPOCH, 
**	    D_FMT, T_FMT, TS_FMT.
**	28-Mar-01 (gordy)
**	    Added driverID and protocol.  Removed propDB as the property 
**	    info may be accessed via its cp_db key.  Renamed effective
**	    user to DBMS user to be generic and pair with DBMS pwd.
**	18-Apr-01 (gordy)
**	    Followup to previous change of effective user: also change
**	    the connection parameter ID.
**	20-Aug-01 (gordy)
**	    Added connection parameter for default cursor mode.
** 	20-Sep-22 (wansh01)
** 	    Added login type connection property and attribute.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	23-Dec-02 (gordy)
**	    Added dbcapability ID's for non-DBMS items, internal connection
**	    property definitions, new login_type connection property.
**	18-Feb-03 (gordy)
**	    Renamed DRV_PROP_LOGIN to DRV_PROP_VNODE.  Added default and
**	    valid property values.  Added DRV_JDBC_MAJ_VERS, DRV_JDBC_MIN_VERS.
**	 7-Jul-03 (gordy)
**	    Added properties DRV_PROP_TIMEZONE, DRV_PROP_DEC_CHAR,
**	    DRV_PROP_DATE_FRMT, DRV_PROP_MNY_FRMT, DRV_PROP_MNY_PREC
**	    and associated attributes.  Added property key prefix
**	    DRV_PROPERTY_KEY.
**	12-Sep-03 (gordy)
**	    Moved date/time constants to SqlDates utility class.
**	 3-Jul-06 (gordy)
**	    Added DBMS key for tracing.
**	 6-Oct-06 (gordy)
**	    Raised driver version to 3.0 for support of ANSI 
**	    date/time data types.
**	30-Jan-07 (gordy)
**	    Added configuration property DRV_CNF_LOB_LOCATORS
**	    for LOB Locators.
**	26-Feb-07 (gordy)
**	    Additional configuration properties for LOB Locators.
**	23-Jul-07 (gordy)
**	    Added property DRV_PROP_DATE_ALIAS and attribute DATE.
**	12-Sep-07 (gordy)
**	    Added default empty date replacement value and config property.
**      14-Feb-08 (rajus01) SIR 119917
**          Added support for JDBC driver patch version for patch builds and
**	    bumped the patch version to 1 for bug 119919.
**	 3-Mar-09 (gordy)
**	    Add scrollable cursor config property: DRV_CNF_CURS_SCROLL.
**	 9-Oct-09 (gordy)
**	    Updated versions for JDBC 4.0 support, driver version 4.0.
**	25-Mar-10 (gordy)
**	    Bumped version to 4.0.1 for batch support.  
**	    Added configuration property DRV_CNF_BATCH.
**	15-Apr-10 (gordy)
**	    Bumped version to 4.0.2 for positional DB proc parameters.
**	 1-Jun-10 (gordy)
**	    Bumped version to 4.0.3 for batch performance improvements.
**	20-Jul-10 (gordy)
**	    Bumped version to 4.0.4 for SQL comment handling.
**	30-Jul-10 (gordy)
**	    Added connection property DRV_CP_SND_INT_BOOL and
**	    DRV_PROP_SND_INT_BOOL.
**	13-Aug-10 (gordy)
**	    Bumped version to 4.0.5
**	10-Nov-10 (gordy)
**	    Added valid values for date alias parameter.
*/

interface
DrvConst
{

    /*
    ** Driver constants
    */
    String	DRV_VENDOR_NAME		= "Ingres Corporation";
    int		DRV_MAJOR_VERSION	= 4;
    int		DRV_MINOR_VERSION	= 0;
    int		DRV_PATCH_VERSION	= 5;
    String	DRV_JDBC_VERSION	= "JDBC 4.0";
    int		DRV_JDBC_MAJ_VERS	= 4;
    int		DRV_JDBC_MIN_VERS	= 0;
    String	DRV_JDBC_PROTOCOL_ID	= "jdbc";
    String	DRV_PROPERTY_KEY	= "property";
    String	DRV_DBMS_KEY		= "dbms";
    String	DRV_TRACE_ID		= "drv";
    String	DSRC_TRACE_ID		= "ds";
    
    /*
    ** Driver configuration properties
    */
    String	DRV_CNF_LOB_CACHE	= "lob.cache.enabled";
    String	DRV_CNF_LOB_SEGSIZE	= "lob.cache.segment_size";
    String	DRV_CNF_LOB_LOCATORS	= "lob.locators.enabled";
    String	DRV_CNF_LOB_LOC_AUTO	= "lob.locators.autocommit.enabled";
    String	DRV_CNF_LOB_LOC_LOOP	= "lob.locators.select_loop.enabled";
    String	DRV_CNF_EMPTY_DATE	= "date.empty";
    String	DRV_CNF_CURS_SCROLL	= "scroll.enabled";
    String	DRV_CNF_BATCH		= "batch.enabled";

    int		DRV_DFLT_SEGSIZE	= 8192;
    String	DRV_DFLT_EMPTY_DATE	= "";

    /*
    ** Cursor concurrency modes
    */
    int		DRV_CRSR_DBMS		= -1;
    int		DRV_CRSR_READONLY	= 0;
    int		DRV_CRSR_UPDATE		= 1;

    /*
    ** Non-DBMS dbcapability ID's returned by GCD server.
    */
    String	DRV_DBCAP_CONNECT_LVL0	= "JDBC_CONNECT_LEVEL";
    String	DRV_DBCAP_CONNECT_LVL1	= "DBMS_CONNECT_LEVEL";

    /*
    ** The following connection properties are internal to the driver.
    ** Negative values are used since DAM-ML values are positive.
    */
    short	DRV_CP_SELECT_LOOP	= -1;
    short	DRV_CP_CURSOR_MODE	= -2;
    short	DRV_CP_CHAR_ENCODE	= -3;
    short	DRV_CP_SND_ING_DTE	= -4;
    short	DRV_CP_SND_INT_BOOL	= -5;

    /*
    ** Driver connection properties and URL attributes.
    ** The 'database' property is a special case since
    ** it is generally extracted from the URL but is
    ** not a URL attribute.
    */
    String	DRV_PROP_DB		= "database";
    String	DRV_PROP_USR		= "user";
    String	DRV_PROP_PWD		= "password";
    String	DRV_PROP_GRP		= "group";
    String	DRV_PROP_ROLE		= "role";
    String	DRV_PROP_DBUSR		= "dbms_user";
    String	DRV_PROP_DBPWD		= "dbms_password";
    String	DRV_PROP_POOL		= "connect_pool";
    String	DRV_PROP_XACM		= "autocommit_mode";
    String	DRV_PROP_VNODE		= "vnode_usage";
    String	DRV_PROP_LOOP		= "select_loop";
    String	DRV_PROP_CRSR		= "cursor_mode";
    String	DRV_PROP_ENCODE		= "char_encode";
    String	DRV_PROP_TIMEZONE	= "timezone";
    String	DRV_PROP_DEC_CHAR	= "decimal_char";
    String	DRV_PROP_DATE_FRMT	= "date_format";
    String	DRV_PROP_MNY_FRMT	= "money_format";
    String	DRV_PROP_MNY_PREC	= "money_precision";
    String	DRV_PROP_DATE_ALIAS	= "date_alias";
    String	DRV_PROP_SND_ING_DTE	= "send_ingres_dates";	
    String	DRV_PROP_SND_INT_BOOL	= "send_integer_booleans";

    String	prop_valid_off_on[]	= { "off", "on" };
    String	prop_valid_false_true[]	= { "false", "true" };
    String	prop_xacm_valid[]	= { "dbms", "single", "multi" };
    String	prop_vnode_valid[]	= { "connect", "login" };
    String	prop_crsr_valid[]	= { "dbms", "readonly", "update" };
    String	prop_date_alias_valid[] = { "ansidate", "ingresdate" };
    
    PropInfo	propInfo[] =		    // Connection properties
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

	new PropInfo( DRV_PROP_DATE_ALIAS, false, "Date Alias", 
			prop_date_alias_valid[0], prop_date_alias_valid,
			DrvRsrc.RSRC_CP_DTE_TYP, MsgConst.MSG_CP_DATE_ALIAS ),

	new PropInfo( DRV_PROP_SND_ING_DTE, false, "Send Ingres Dates",
			prop_valid_false_true[0], prop_valid_false_true,
			DrvRsrc.RSRC_CP_SND_ING_DTE, DRV_CP_SND_ING_DTE ),

	new PropInfo( DRV_PROP_SND_INT_BOOL, false, "Send Integer Booleans",
			prop_valid_false_true[0], prop_valid_false_true,
			DrvRsrc.RSRC_CP_SND_INT_BOOL, DRV_CP_SND_INT_BOOL ),
    };

    AttrInfo	attrInfo[] =		    // URL Attributes
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
	new AttrInfo( "ENCODE", DRV_PROP_ENCODE ),
	new AttrInfo( "TZ", DRV_PROP_TIMEZONE ),
	new AttrInfo( "DECIMAL", DRV_PROP_DEC_CHAR ),
	new AttrInfo( "DATE_FMT", DRV_PROP_DATE_FRMT ),
	new AttrInfo( "MNY_FMT", DRV_PROP_MNY_FRMT ),
	new AttrInfo( "MNY_PREC", DRV_PROP_MNY_PREC ),
	new AttrInfo( "DATE", DRV_PROP_DATE_ALIAS ),
	new AttrInfo( "SEND_INGDATE", DRV_PROP_SND_ING_DTE ),
	new AttrInfo( "SEND_INTBOOL", DRV_PROP_SND_INT_BOOL ),
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
**	desc		Default property description (Locale specific).
**	dflt		Default value.
**	valid		List of valid values.
**	rsrcID		Resource ID.
**	msgID		DAM-ML MSG_CONNECT parameter ID.
**
** History:
**	 6-May-99 (gordy)
**	    Created.
**	 8-Sep-99 (gordy)
**	    Added ID to load description (also new) from resource.
**	27-Feb-03 (gordy)
**	    Added default and valid values.
*/

static class
PropInfo
{

    public String   name;	// Property name.
    public boolean  req;	// Is property required.
    public String   desc;	// Property description.
    public String   dflt;	// Default value.
    public String   valid[];	// List of valid values.
    public String   rsrcID;	// Resource ID.
    public short    msgID;	// Connection parameter ID.


/*
** Name: PropInfo
**
** Description:
**	Class constructor.
**
** Input:
**	name	    Property Name.
**	req	    Is property required?
**	desc	    Description.
**	dflt	    Default value.
**	valid	    List of valid values.
**	rsrcID	    Resource ID.
**	msgID	    DAM-ML MSG_CONNECT parameter ID.
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
**	27-Feb-03 (gordy)
**	    Added default and valid values.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Make a copy of the valid list so that subsquent external
**	    changes are ignored.
*/

public
PropInfo
( 
    String  name, 
    boolean req, 
    String  desc, 
    String  dflt,
    String  valid[],
    String  rsrcID, 
    short   msgID
)
{

    this.name = name;
    this.req = req;
    this.desc = desc;
    this.dflt = dflt;
    this.rsrcID = rsrcID;
    this.msgID = msgID;

    if ( valid == null )
	this.valid = null;
    else
    {
	this.valid = new String[ valid.length ];
	System.arraycopy( valid, 0, this.valid, 0, valid.length );
    }

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

} // interface DrvConst
