/*
** Copyright (c) 1999, 2009 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: JdbcDBMD.java
**
** Description:
**	Defines class which implements the JDBC DatabaseMetaData
**	interface.
**
**  Classes:
**
**	JdbcDBMD		Database Meta-data.
**	MDdesc			Pre-defined descriptor entries.
**	MDschemaRS		RsltXlat result-set for getSchemas()
**	MDprocedureRS		RsltXlat result-set for getProcedures()
**	MDprocedureColRS	RsltXlat result-set for getProcedureColumns()
**	MDfunctionRS		RsltXlat result-set for getFunctions()
**	MDfunctionColRS	        RsltXlat result-set for getFunctionColumns()
**	MDtableRS		RsltXlat result-set for getTables()
**	MDgetTablePrivRS	RsltXlat result-set for getTablePrivileges()
**	MDgetColRS		RsltXlat result-set for getColumns()
**	MDgetColPrivRS		RsltXlat result-set for getColumnPrivileges()
**	MDgetBestRowRS		RsltXlat result-set for getBestRowIdentifier()
**	MDindexRS		RsltXlat result-set for getIndexInfo()
**	MDprimaryKeyRS		RsltXlat result-set for getPrimaryKeys()
**	MDkeysRS		RsltXlat result-set for getImportedKeys(), etc.
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	22-Mar-00 (rajus01)
**	    Needed many changes to make gatways happy:
**	    Simplified SQL query for getIndexInfo() method. Added
**	    additional check for VSAM, IMS gateways in getPrimaryKeys().
**	    Use iidbconstants catalog to get DB username. Added a 
**	    private method makePattern(). Removed trim() function in
**	    SQL queries since DB2, DCOM gateways don't support trim()
**	    functions.
**	13-Jun-00 (gordy)
**	    EdbcSQL function methods made static.
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	10-Nov-00 (gordy)
**	    Added support for JDBC 2.0 extensions.
**	11-Jan-01 (gordy)
**	    Batch processing now supported.
**	20-Jan-01 (gordy)
**	    Changed return value (due to changes in driver behavior)
**	    for methods supportsOpenStatementsAcrossCommit() and
**	    supportsOpenStatementsAcrossRollback().
**	 6-Mar-01 (gordy)
**	    Sybase and MS-SQL don't need ' %' when generating LIKE patterns. 
**	 7-Mar-01 (gordy)
**	    Replaced dbmsinfo() method with connection method.
**	 8-Mar-01 (gordy)
**	    Restored original query to getIndexInfo() which was lost
**	    when union selects were removed and wrong query was retained.
**	28-Mar-01 (gordy)
**	    Tracing now passed as parameter to constructors.  The 
**	    getDbmsInfo() method in EdbcConnect  now combines DBMS 
**	    capabilities from DbConn and DBMS dbmsinfo() requests.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**      27-Jul-01 (loera01) Bug 105327
**          For supportsTransactionIsolationLevel method, only support 
**          serializable isolation level is the catalog level is 6.4.
**	20-Aug-01 (gordy)
**	    Support updatable cursors (non-scrollble).
**	 5-Feb-02 (gordy)
**	    Check for existence of DB_DELIMITED_CASE to determine
**	    support for delimited identifiers.
**	15-Feb-02 (gordy)
**	    Make sure storage class matches descriptor for Xlat result-sets.
**	21-Feb-02 (gordy)
**	    Ingres DBMS type now pre-determined.
**	19-Jul-02 (gordy)
**	    BUG 108004 - The getSchemas() method does not trim column results.
**	19-Jul-02 (gordy)
**	    BUG 108288 - The getPrimaryKeys() method does not return info on
**	    physical table structure.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Mar-03 (gordy)
**	    Upgraded to JDBC 3.0.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	30-Sep-03 (weife01)
**	    BUG 111022 - Add unicode data type NCHAR, NVARCHAR, LONG NVARCHAR
**	    map from DBMS to JAVA type CHAR, VARCHAR, LONG VARCHAR
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	15-Mar-04 (gordy)
**	    Support BIGINT columns.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	29-Aug-06 (rajus01) StarIssue:14303664;Bug# 114982;Prob# EDJDBC105
**	    Cross-integrating change from ingres!26 for Bug #114982.
**	    Added "table_name not like 'ii%'" to the where clause to suppress
**	    the displaying of iietab_xx_yy tables. These extension tables  
**	    get created when user tables have blob columns (LONG VARCHAR/LONG
**	    BYTE), and the actual blob data is stored in these extension tables
**	    where xx and yy are numbers chosen by the DBMS. These tables are 
**	    system catalogs in essence, eventhough the system_use flag is set 
**	    to 'U'. Users cannot create tables named ii*.
**	14-Sep-06 (lunbr01)  Bug 115453
**	    DatabaseMetaData.getColumns() returns NO_DATA when table name
**	    is 32 characters long (actually max length of target dbms type).  
**	    Also erroneously matches on similarly named tables with embedded 
**	    spaces (eg "t1" and "t1 a").  Revamped makePattern() to return
**	    updated WHERE clause rather than just pattern.
**	22-Sep-06 (gordy)
**	    Update keyword for Ingres and ANSI date types.
**      31-Jan-07 (weife01) SIR 117451
**          Add support for catalog.
**	22-May-07 (gordy)
**	    Add support for scrollable cursors.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**    14-Feb-08 (rajus01) SIR 119917
**	    Added support for JDBC driver patch version for patch builds.
**	14-Feb-08 (rajus01) Bug 119919
**	    getColumns() returns incorrect values for column datatype MONEY.
**	16-Dec-08 (gordy)  BUG 121345
**	    Include escape clause for LIKE predicate in accordance with
**	    JDBC spec for method DatabaseMetaData.getSearchStringEscape().
**      05-Jan-09 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to
**            support JDBC 4.0 SQLException hierarchy.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	26-Mar-09 (rajus01) Bug 121865
**	    While running OpenJPA testsuite with INGRES it has been brought
**	    to notice that metadata queries used the "LIKE" predicate even
**	    when database object names passed in to the DatabaseMetaData 
**	    methods didn't have any pattern matching characters in them.
**	    The changes made in this submission parses the pattern for
**	    pattern matching characters "%, \, _" and decides whether "="
**	    operator or LIKE predicate needs to be used. It DOES NOT 
**	    completely optimize when the database objects contain 
**	    unescaped "_". 
**      14-Apr-09 (rajus01) SIR 121238
**          Implemented the new JDBC 4.0 methods.
**	 1-Oct-09 (gordy)
**	    Enhance getMaxProcedureNameLength() to check dbcap entry.
**	26-Oct-09 (gordy)
**	    Support boolean data type.
*/

import	java.util.Enumeration;
import	java.sql.*;
import	com.ingres.gcf.util.DbmsConst;
import  com.ingres.gcf.util.IdMap;
import	com.ingres.gcf.util.SqlData;
import	com.ingres.gcf.util.SqlNull;
import	com.ingres.gcf.util.SqlBool;
import	com.ingres.gcf.util.SqlSmallInt;
import	com.ingres.gcf.util.SqlInt;
import	com.ingres.gcf.util.SqlString;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.util.GcfErr;


/*
** Name: JdbcDBMD
**
** Description:
**	JDBC driver class which implements the JDBC DatabaseMetaData
**	interface.
**
**  Interface Methods:
**
**	allProceduresAreCallable
**	allTablesAreSelectable
**      autoCommitFailureClosesAllResultSets
**	dataDefinitionCausesTransactionCommit
**	dataDefinitionIgnoredInTransactions
**	deletesAreDetected
**	doesMaxRowSizeIncludeBlobs
**	getAttributes
**	getBestRowIdentifier
**	getCatalogs
**	getCatalogSeparator
**	getCatalogTerm
**	getColumnPrivileges
**	getColumns
**	getConnection.
**	getCrossReference
**	getDatabaseMajorVersion
**	getDatabaseMinorVersion
**	getDatabaseProductName
**	getDatabaseProductVersion
**	getDefaultTransactionIsolation
**	getDriverMajorVersion
**	getDriverMinorVersion
**	getDriverName
**	getDriverVersion
**	getExtraNameCharacters
**	getExportedKeys
**	getFunctionColumns
**	getFunctions
**	getIdentifierQuoteString
**	getImportedKeys
**	getIndexInfo
**	getJDBCMajorVersion
**	getJDBCMinorVersion
**	getMaxBinaryLiteralLength
**	getMaxCatalogNameLength
**	getMaxCharLiteralLength
**	getMaxColumnNameLength
**	getMaxColumnsInGroupBy
**	getMaxColumnsInIndex
**	getMaxColumnsInOrderBy
**	getMaxColumnsInSelect
**	getMaxColumnsInTable
**	getMaxConnections
**	getMaxCursorNameLength
**	getMaxIndexLength
**	getMaxProcedureNameLength
**	getMaxRowSize
**	getMaxSchemaNameLength
**	getMaxStatementLength
**	getMaxStatements
**	getMaxTableNameLength
**	getMaxTablesInSelect
**	getMaxUserNameLength
**	getNumericFunctions
**	getPrimaryKeys
**	getProcedureColumns
**	getProcedures
**	getProcedureTerm
**	getResultSetHoldability
**      getRowIdLifetime()
**	getSchemas
**      getSchemas (overloaded)
**	getSchemaTerm
**	getSearchStringEscape
**	getSQLKeywords
**	getSQLStateType
**	getStringFunctions
**	getSuperTables
**	getSuperTypes
**	getSystemFunctions
**	getTables
**	getTablePrivileges
**	getTableTypes
**	getTimeDateFunctions
**	getTypeInfo
**	getUDTs
**	getURL
**	getUserName
**	getVersionColumns
**	insertsAreDetected
**	isCatalogAtStart
**	isReadOnly
**      isWrapperFor
**	locatorsUpdateCopy
**	nullsAreSortedHigh
**	nullsAreSortedLow
**	nullsAreSortedAtStart
**	nullsAreSortedAtEnd
**	nullPlusNonNullIsNull
**	ownUpdatesAreVisible
**	ownDeletesAreVisible
**	ownInsertsAreVisible
**	othersUpdatesAreVisible
**	othersDeletesAreVisible
**	othersInsertsAreVisible
**	storesLowerCaseIdentifiers
**	storesMixedCaseIdentifiers
**	storesUpperCaseIdentifiers
**	storesLowerCaseQuotedIdentifiers
**	storesMixedCaseQuotedIdentifiers
**	storesUpperCaseQuotedIdentifiers
**	supportsAlterTableWithAddColumn
**	supportsAlterTableWithDropColumn
**	supportsANSI92EntryLevelSQL
**	supportsANSI92IntermediateSQL
**	supportsANSI92FullSQL
**	supportsBatchUpdates
**	supportsCatalogsInDataManipulation
**	supportsCatalogsInIndexDefinitions
**	supportsCatalogsInPrivilegeDefinitions
**	supportsCatalogsInProcedureCalls
**	supportsCatalogsInTableDefinitions
**	supportsColumnAliasing
**	supportsConvert
**	supportsCoreSQLGrammar
**	supportsCorrelatedSubqueries
**	supportsDataDefinitionAndDataManipulationTransactions
**	supportsDataManipulationTransactionsOnly
**	supportsDifferentTableCorrelationNames
**	supportsExpressionsInOrderBy
**	supportsExtendedSQLGrammar
**	supportsFullOuterJoins
**	supportsGetGeneratedKeys
**	supportsGroupBy
**	supportsGroupByBeyondSelect
**	supportsGroupByUnrelated
**	supportsIntegrityEnhancementFacility
**	supportsLikeEscapeClause
**	supportsLimitedOuterJoins
**	supportsMinimumSQLGrammar
**	supportsMixedCaseIdentifiers
**	supportsMixedCaseQuotedIdentifiers
**	supportsMultipleOpenResults
**	supportsMultipleResultSets
**	supportsMultipleTransactions
**	supportsNamedParameters
**	supportsNonNullableColumns
**	supportsOpenCursorsAcrossCommit
**	supportsOpenCursorsAcrossRollback
**	supportsOpenStatementsAcrossCommit
**	supportsOpenStatementsAcrossRollback
**	supportsOrderByUnrelated
**	supportsOuterJoins
**	supportsPositionedDelete
**	supportsPositionedUpdate
**	supportsResultSetConcurrency
**	supportsResultSetHoldability
**	supportsResultSetType
**	supportsSavepoints
**	supportsSchemasInDataManipulation
**	supportsSchemasInIndexDefinitions
**	supportsSchemasInPrivilegeDefinitions
**	supportsSchemasInProcedureCalls
**	supportsSchemasInTableDefinitions
**	supportsSelectForUpdate
**	supportsStatementPooling
**	supportsStoredProcedures
**      supportsStoredFunctionUsingCallSyntax
**	supportsSubqueriesInComparisons
**	supportsSubqueriesInExists
**	supportsSubqueriesInIns
**	supportsSubqueriesInQuantifieds
**	supportsTableCorrelationNames
**	supportsTransactions
**	supportsTransactionIsolationLevel
**	supportsUnion
**	supportsUnionAll
**      unwrap
**	updatesAreDetected
**	usesLocalFiles
**	usesLocalFilePerTable
**
**  Constants:
**
**	empty			Empty string.
**	space			Space string
**	escape_char		Search string escape character: '\'
**	NO			"NO"
**	YES			"YES"
**	VARCHAR			DBMS VARCHAR numeric type.
**	INTEGER			DBMS INTEGER numeric type.
**
**  Private Data:
**
**	conn			Database connection.
**	trace			Tracing.
**	title			Class title for tracing.
**	upper			Are identifiers stored in upper case.
**	std_catl_lvl		Standard catalog level.
**	param_type		VARCHAR or OTHER (UCS2).
**	dataTypeMap		
**
**  Private Methods:
**
**	makePattern		Generates LIKE predicate pattern for DBMS.
** 	getStringType		Returns JDBC type as string for DBMS type.
**	convToJavaType		Returns JDBC type for DBMS type.
**	colSize			Redturns display size for DBMS type.
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	01-05-00 (rajus01)
**	    By default INGRES stores identifiers in lower case. The
**	    resultSet column names case is determined by DB_NAME_CASE
**	    capability from iidbcapablilites.
**	    Added upper. Removed I2147483647. Fixed getTypeInfo().
**	13-Jun-00 (gordy)
**	    Member edbcSql removed as methods used are now static.
**	10-Nov-00 (gordy)
**	    Support JDBC 2.0 extensions.  Added dbc, getConnection(),
**	    supportsBatchUpdates(), supportsResultSetType(),
**	    supportsResultSetConcurrency(), ownUpdatesAreVisible(),
**	    ownDeletesAreVisible(), ownInsertsAreVisible(),
**	    othersUpdatesAreVisible(), othersDeletesAreVisible()
**	    othersInsertsAreVisible(), updatesAreDetected,
**	    deletesAreDetected(), insertsAreDetected().
**	 7-Mar-01 (gordy)
**	    Replaced dbmsinfo() method with connection method.   
**	28-Mar-01 (gordy)
**	    Dropped dbc (info now available in EdbcConnect) and
**	    URL (now built when needed). 
**	10-May-01 (gordy)
**	    UCS2 Databases still use CHAR/VARCHAR for the standard
**	    catalogs.  Need to save parameters with special type
**	    OTHER so that driver will not send as UCS2.  Added field
**	    param_type to indicate what to send and changed all
**	    setString() calls to setObject().
**	31-Oct-02 (gordy)
**	    Moved OI_CAT_LVL and ING64_NAME_LEN constants to DrvConst.
**	    Moved functionality of getMaxCol() into calling methods.
**	20-Mar-03 (gordy)
**	    Added constants empty, space, NO and YES.  Added methods
**	    supportsSavepoints(), supportsNamedParameters(),
**	    supportsStatementPooling(), supportsGetGeneratedKeys(),
**	    locatorsUpdateCopy(), supportsMultipleOpenResults(), 
**	    supportsResultSetHoldability(), getResultSetHoldability(),
**	    getDatabaseMajorVersion(), getDatabaseMinorVersion(),
**	    getJDBCMajorVersion(), getJDBCMinorVersion(), getSQLStateType(),
**	    getSuperTables(), getSuperTypes(), getAttributes().	   
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.  Result-set
**	    constants moved to point where used.
**	16-Dec-08 (gordy)
**	    Added constant for search string escape character to be
**	    shared between getSearchStringEscape() and makePattern().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Don't need to construct an additional string for string constants.
**	    Moved map tables from public interface to private fields.
**      15-Apr-09 (rajus01) SIR 121238
**          Implemented the new JDBC 4.0 methods.
**	26-Oct-09 (gordy)
**	    Added support for boolean data type.
*/

public class
JdbcDBMD
    implements DatabaseMetaData, DrvConst, DbmsConst, GcfErr
{

    private final static String  empty		= "";
    private final static String  space		= " ";
    private final static String  NO		= "NO";
    private final static String  YES		= "YES";

    private final static short	VARCHAR		= DBMS_TYPE_VARCHAR;
    private final static short	INTEGER		= DBMS_TYPE_INT;

    /*
    ** Local data
    */
    private DrvConn	 	conn		= null;
    private DrvTrace		trace		= null;
    private String		title		= "DatabaseMetaData";

    /*
    ** DBMS configuration settings.
    */
    private boolean		upper		= false; // Upper case?
    private int			std_catl_lvl	= 0; // Standard catalog level.
    private int			param_type	= Types.VARCHAR; // Char type.
    private String		escape_clause	= empty;
    private String 		escape_char	= null;

    private static IdMap	dataTypeMap[] =
    {
	new IdMap( DBMS_TYPE_INT,	"tinyint" ),
	new IdMap( DBMS_TYPE_INT,	DBMS_TYPSTR_INT2 ),
	new IdMap( DBMS_TYPE_INT,	DBMS_TYPSTR_INT4 ),
	new IdMap( DBMS_TYPE_INT,	"int" ),
	new IdMap( DBMS_TYPE_INT,	DBMS_TYPSTR_INT8 ),
	new IdMap( DBMS_TYPE_FLOAT,	"real" ),
	new IdMap( DBMS_TYPE_FLOAT,	DBMS_TYPSTR_FLT8 ),
	new IdMap( DBMS_TYPE_FLOAT,	"double precision" ),
	new IdMap( DBMS_TYPE_FLOAT,	"double p" ),
	new IdMap( DBMS_TYPE_DECIMAL,	DBMS_TYPSTR_DECIMAL ),
	new IdMap( DBMS_TYPE_DECIMAL,	"numeric" ),
	new IdMap( DBMS_TYPE_CHAR,	DBMS_TYPSTR_CHAR ),
	new IdMap( DBMS_TYPE_CHAR,	"character" ),
	new IdMap( DBMS_TYPE_VARCHAR,	DBMS_TYPSTR_VARCHAR ),
	new IdMap( DBMS_TYPE_LONG_CHAR,	DBMS_TYPSTR_LONG_CHAR ),
	new IdMap( DBMS_TYPE_NCHAR,	DBMS_TYPSTR_NCHAR ),
	new IdMap( DBMS_TYPE_NVARCHAR,	"nvarchar" ),
	new IdMap( DBMS_TYPE_LONG_NCHAR,DBMS_TYPSTR_LONG_NCHAR ),
	new IdMap( DBMS_TYPE_BYTE,	DBMS_TYPSTR_BYTE ),
	new IdMap( DBMS_TYPE_VARBYTE,	"varbyte"),
	new IdMap( DBMS_TYPE_VARBYTE,	DBMS_TYPSTR_VARBYTE ),
	new IdMap( DBMS_TYPE_LONG_BYTE,	DBMS_TYPSTR_LONG_BYTE ),
	new IdMap( DBMS_TYPE_C,		DBMS_TYPSTR_C ),
	new IdMap( DBMS_TYPE_TEXT,	DBMS_TYPSTR_TEXT ),
	new IdMap( DBMS_TYPE_MONEY,	DBMS_TYPSTR_MONEY ),
	new IdMap( DBMS_TYPE_BOOL,	DBMS_TYPSTR_BOOL ),
	new IdMap( DBMS_TYPE_IDATE,	"date" ),
	new IdMap( DBMS_TYPE_IDATE,	DBMS_TYPSTR_IDATE ),
	new IdMap( DBMS_TYPE_ADATE,	DBMS_TYPSTR_ADATE ),
	new IdMap( DBMS_TYPE_TIME,	DBMS_TYPSTR_TIME ),
	new IdMap( DBMS_TYPE_TMWO,	DBMS_TYPSTR_TMWO ),
	new IdMap( DBMS_TYPE_TMTZ,	DBMS_TYPSTR_TMTZ ),
	new IdMap( DBMS_TYPE_TS,	DBMS_TYPSTR_TS ),
	new IdMap( DBMS_TYPE_TSWO,	DBMS_TYPSTR_TSWO ),
	new IdMap( DBMS_TYPE_TSTZ,	DBMS_TYPSTR_TSTZ ),
	new IdMap( DBMS_TYPE_VARCHAR,	DBMS_TYPSTR_INTYM ),
	new IdMap( DBMS_TYPE_VARCHAR,	DBMS_TYPSTR_INTDS ),
    };


/*
** Name: JdbcDBMD
**
** Description:
**	Class constructor.
**
** Input:
**	conn		Database connection.
**
** Output:
**	None.
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	28-Mar-01 (gordy)
**	    Dropped URL paramater.
**	10-May-01 (gordy)
**	    Check for UCS2 support.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	27-Apr-07 (rajus01) Bug # 118271
**	    Changed the class modifier to public to work with JdbcTemplate
**	    package of opensource Spring Framework.  The constructor is 
**	    given package access.
**	26-Mar-09 (rajus01) Bug 121865
**	    Moved ESCAPE clause related code from makePattern() 
**	    considering frequency of makePattern() call by the various 
**	    interface methods.
*/

// package access
JdbcDBMD( DrvConn conn )
    throws SQLException
{
    this.conn = conn;
    this.trace = conn.trace;
    this.title = trace.getTraceName() + "-DatabaseMetaData";

    /*
    ** Determine DBMS configuration.  These settings are used
    ** for building queries for other meta-data requests.  DBMS
    ** configuration settings which are determine the result of
    ** a meta-data requests are retrieved during the request.
    */
    String cat_lvl = conn.dbCaps.getDbCap( DBMS_DBCAP_STD_CAT_LVL );
    if ( cat_lvl != null )
	try { std_catl_lvl = Integer.parseInt( cat_lvl ); }
	catch( Exception ignore ) {};
    
    String  db_name_case = conn.dbCaps.getDbCap( DBMS_DBCAP_NAME_CASE );
    if ( db_name_case != null  &&  db_name_case.equals(DBMS_NAME_CASE_UPPER) )
	upper = true;

    /* 
    ** Determine DBMS settings for 'escape' clause to be used with
    ** LIKE perdicate in the metadata queries.
    */
    String dbms_escape = conn.dbCaps.getDbCap( DBMS_DBCAP_ESCAPE );
    String dbms_esc_chr  = conn.dbCaps.getDbCap( DBMS_DBCAP_ESCAPE_CHAR );

    if ( dbms_escape != null  &&  dbms_escape.equals( "Y" ) )
	escape_char = (dbms_esc_chr != null) ? dbms_esc_chr : "\\";

    if( escape_char != null )  escape_clause = " escape '" + escape_char + "'";

    /*
    ** UCS2 databases still use CHAR and VARCHAR for standard
    ** catalogs, so we must force non-UCS2 string parameters
    ** by using setObject() and the OTHER type recognized by 
    ** the driver as an alternate parameter format.
    */
    if ( conn.ucs2_supported )  param_type = Types.OTHER;
    return;
} // JdbcDBMD


/*
** Name: getDriverName
**
** Description:
**	Returns the name of this JDBC driver.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public String 
getDriverName() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getDriverName(): " + conn.driverName );
    return( conn.driverName );
} // getDriverName


/*
** Name: getDriverVersion
**
** Description:
**	Returns the version of this JDBC driver.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	14-Feb-08 (rajus01) SIR 119917
**	    Added support for JDBC driver patch version for patch builds.
*/

public String 
getDriverVersion() 
    throws SQLException
{
    String version = DRV_MAJOR_VERSION + "." + DRV_MINOR_VERSION + "." + DRV_PATCH_VERSION;
    if ( trace.enabled() )  
	trace.log( title + ".getDriverVersion(): " + version );
    return( version );
} // getDriverVersion


/*
** Name: getDriverMajorVersion
**
** Description:
**	Returns the JDBC driver's major version number.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
*/

public int 
getDriverMajorVersion()
{
    if ( trace.enabled() ) 
	trace.log( title + ".getDriverMajorVersion(): " + DRV_MAJOR_VERSION );
    return( DRV_MAJOR_VERSION );
} // getDriverMajorVersion


/*
** Name: getDriverMinorVersion
**
** Description:
**	Returns the JDBC driver's minor version number.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
*/

public int 
getDriverMinorVersion()
{
    if ( trace.enabled() )
	trace.log( title + ".getDriverMinorVersion(): " + DRV_MINOR_VERSION );
    return( DRV_MINOR_VERSION );
} // getDriverMinor Version


/*
** Name: getJDBCMajorVersion
**
** Description:
**	Returns the driver's supported JDBC major version number.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	JDBC major version.
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

public int 
getJDBCMajorVersion()
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".getJDBCMajorVersion(): " + DRV_JDBC_MAJ_VERS );
    return( DRV_JDBC_MAJ_VERS );
} // getJDBCMajorVersion


/*
** Name: getJDBCMinorVersion
**
** Description:
**	Returns the driver's supported JDBC minor version number.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	JDBC minor version.
**
** History:
**	20-Ma4-03 (gordy)
**	    Created.
*/

public int 
getJDBCMinorVersion()
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".getJDBCMinorVersion(): " + DRV_JDBC_MIN_VERS );
    return( DRV_JDBC_MIN_VERS );
} // getJDBCMinorVersion


/*
** Name: supportsStatementPooling
**
** Description:
**	Are pooled statements supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if supported, FALSE otherwise.
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

public boolean
supportsStatementPooling()
{
    if ( trace.enabled() )  
	trace.log( title + ".supportsStatementPooling(): " + false );
    return( false );
} // supportsStatementPooling


/*
** Name: supportsBatchUpdates
**
** Description:
**	Are batch updates supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if supported, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	11-Jan-01 (gordy)
**	    Batch processing has been implemented.
*/

public boolean
supportsBatchUpdates()
{
    if ( trace.enabled() )  
	trace.log(title + ".supportsBatchUpdates(): " + true);
    return( true );
} // supportsBatchUpdates


/*
** Name: supportsGetGeneratedKeys
**
** Description:
**	Is retrieval of auto-generated keys supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if supported, FALSE otherwise.
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

public boolean
supportsGetGeneratedKeys()
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log(title + ".supportsGetGeneratedKeys(): " + true);
    return( true );
} // supportsGetGeneratedKeys


/*
** Name: supportsMultipleResultSets
**
** Description:
**	Are multiple ResultSets from a single execute supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsMultipleResultSets() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsMultipleResultSets(): " + false );
    return( false );
} // supportsMultipleResultSets


/*
** Name: supportsMultipleOpenResults
**
** Description:
**	Are multiple open ResultSets from a single execute supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-Mar-99 (gordy)
**	    Created.
*/

public boolean 
supportsMultipleOpenResults() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsMultipleOpenResults(): " + false );
    return( false );
} // supportsMultipleOpenResults


/*
** Name: supportsResultSetType
**
** Description:
**	Is given result-set type supported.
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if supported, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	22-May-07 (gordy)
**	    All types supported with scrollable cursors.
*/

public boolean
supportsResultSetType( int type )
{
    boolean supported = false;

    switch( type )
    {
    case ResultSet.TYPE_FORWARD_ONLY :
    case ResultSet.TYPE_SCROLL_SENSITIVE :
    case ResultSet.TYPE_SCROLL_INSENSITIVE :	supported = true;	break;
    }

    if ( trace.enabled() )  trace.log( 
	title + ".supportsResultSetType( " + type + " ): " + supported );
    return( supported );
} // SupportsResultSetType


/*
** Name: supportsResultSetConcurrency
**
** Description:
**	Is given result-set concurrency supported for the
**	given result-set type.
**
** Input:
**	type	Result-set type.
**	concur	Result-set concurrency.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if supported, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	20-Aug-01 (gordy)
**	    Support updatable cursors.  Scrollable cursors not supported.
**	22-May-07 (gordy)
**	    Scrollable cursors are insensitive for readonly and
**	    sensative for updatable.
*/

public boolean
supportsResultSetConcurrency( int type, int concur )
{
    boolean supported = false;

    switch( type )
    {
    case ResultSet.TYPE_FORWARD_ONLY :
	switch( concur )
	{
	case ResultSet.CONCUR_READ_ONLY :
	case ResultSet.CONCUR_UPDATABLE :	supported = true;	break;
	}
	break;

    case ResultSet.TYPE_SCROLL_SENSITIVE :
	switch( concur )
	{
	case ResultSet.CONCUR_UPDATABLE :	supported = true;	break;
	}
	break;

    case ResultSet.TYPE_SCROLL_INSENSITIVE :
	switch( concur )
	{
	case ResultSet.CONCUR_READ_ONLY :	supported = true;	break;
	}
	break;
    }

    if ( trace.enabled() )  
	trace.log( title + ".supportsResultSetConcurrency( " + 
			   type + ", " + concur + " ): " + supported );
    return( supported );
} // supportsResultSetConcurrency


/*
** Name: supportsResultSetHoldability
**
** Description:
**	Is given result-set holdability supported.
**
** Input:
**	holdability	Result-set holdability.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if supported, FALSE otherwise.
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

public boolean
supportsResultSetHoldability( int holdability )
    throws SQLException
{
    boolean supported = (holdability == ResultSet.CLOSE_CURSORS_AT_COMMIT);

    if (trace.enabled()) trace.log(title + ".supportsResultSetHoldability(" + 
				   holdability + "): " + supported);
    return( supported );
} // supportsResultSetHoldability


/*
** Name: getResultSetHoldability
**
** Description:
**	What is the default result-set holdability?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Default holdability
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

public int 
getResultSetHoldability() 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getResultSetHoldability(): " +
					ResultSet.CLOSE_CURSORS_AT_COMMIT );
    return( ResultSet.CLOSE_CURSORS_AT_COMMIT );
} // getResultSetHoldability


/*
** Name: getSQLStateType
**
** Description:
**	What is the type of SQL state returned?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	SQL state type.
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
**	07-Jul-09 (rajus01) SIR 121238
**	    Changed the value to sqlStateSQL as per JDBC 4.0 spec. The
**	    value sqlStateSQL99 exists for compatibility reasons.
**	    
*/

public int 
getSQLStateType() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getSQLStateType(): " + sqlStateSQL );
    return( sqlStateSQL );
} // getSQLStateType


/*
** Name: usesLocalFiles
**
** Description:
**	Does the database store tables in local file?
**	FALSE: Eventhough Ingres stores tables in local file
**	       only the 'ingres' user can have access to it.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
usesLocalFiles() 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".usesLocalFiles(): " + false );
    return( false );
} // usesLocalFiles


/*
** Name: usesLocalFilePerTable
**
** Description:
**	Does the database use a file for each table?
**	FALSE: See the description for usesLocalFile().
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
usesLocalFilePerTable() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".usesLocalFilePerTable(): " + false);
    return( false );
} // usesLocalFilePerTable


/*
** Name: getConnection
**
** Description:
**	Return the JDBC connection associated with this meta-data.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	Connection  The JDBC connection.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
*/

public Connection
getConnection()
    throws SQLException
{
    return( conn.jdbc );
} // getConnection


/*
** Name: getURL
**
** Description:
**	Returns the url for this database.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	28-Mar-01 (gordy)
**	    Partial URL rebuilt from driver and connection info.
*/

public String 
getURL() 
    throws SQLException
{
    String url = DRV_JDBC_PROTOCOL_ID + ":" + conn.protocol + 
		 "://" + conn.host + "/" + conn.database;
    if ( trace.enabled() )  trace.log( title + ".getURL(): " + url );
    return( url );
} // getURL


/*
** Name: getDatabaseProductName
**
** Description:
**	Returns the database product name.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public String 
getDatabaseProductName() 
    throws SQLException
{
    String 	dbmsType = conn.dbCaps.getDbCap( DBMS_DBCAP_DBMS_TYPE );

    if ( trace.enabled() )
	trace.log( title + ".getDatabaseProductName(): " + dbmsType );
    return( dbmsType );
} // getDatabaseProductName


/*
** Name: getDatabaseProductVersion
**
** Description:
**	Returns the version of this database product.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	 7-Mar-01 (gordy)
**	    Replaced dbmsinfo() method with connection method.
**	25-Oct-01 (gordy)
**	    Dbmsinfo('_version') is not automatically loaded at
**	    driver startup, so don't limit getDbmsinfo() to cache.    	   
*/

public String 
getDatabaseProductVersion() 
    throws SQLException
{
    String version = conn.dbInfo.getDbmsInfo( DBMS_DBINFO_VERSION ); 
    if ( trace.enabled() ) 
	trace.log( title + ".getDatabaseProductVersion(): " + version );
    return( version );
} // getDatabaseProductVersion


/*
** Name: getDatabaseMajorVersion
**
** Description:
**	Returns the database major version.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Database major version.
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

public int 
getDatabaseMajorVersion() 
    throws SQLException
{
    String  version = conn.dbInfo.getDbmsInfo( DBMS_DBINFO_VERSION ); 
    int	    major = 0, minor = 0;

    try
    {
	/*
	** Search for the first instance of <num>.<num> in version string.
	** 
	**	scan for start of major version
	**	    sub-scan for decimal point
	**		sub-scan for end of minor version
	**		extract major & minor versions using start, point, end
	*/
      outer:
	for( int start = 0; start < version.length(); start++ )
	    if ( Character.isDigit( version.charAt( start ) ) )
		for( int point = start + 1; point < version.length(); point++ )
		    if ( ! Character.isDigit( version.charAt( point ) ) )
			if ( version.charAt( point ) != '.' )
			{
			    start = point;	// Not '<num>.'
			    continue outer;	// Resume scanning for start.
			}
			else
			{
			    int end = point;

			    while( ++end < version.length() )
				if ( ! Character.isDigit(version.charAt(end)) )
				    break;

			    if ( (end - point - 1) <= 0 )
			    {
				start = end;	// Not '<num>.<num>'
				continue outer;	// Resume scanning for start
			    }
			    else
			    {
				major = Integer.parseInt( 
					version.substring( start, point ) );
				minor = Integer.parseInt( 
					version.substring( point + 1, end ) );
				break outer;	// We be done!
			    }
			}
    }
    catch( Exception ignore ) {}	// use default

    if ( trace.enabled() ) 
	trace.log( title + ".getDatabaseMajorVersion(): " + major );
    if ( trace.enabled( 5 ) )
	trace.write( "\t: '" + version + "' => " + major + "." + minor );
    return( major );
} // getDatabaseMajorVersion


/*
** Name: getDatabaseMinorVersion
**
** Description:
**	Returns the database minor version.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	Database minor version.
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

public int
getDatabaseMinorVersion() 
    throws SQLException
{
    String  version = conn.dbInfo.getDbmsInfo( DBMS_DBINFO_VERSION ); 
    int	    major = 0, minor = 0;

    try
    {
	/*
	** Search for the first instance of <num>.<num> in version string.
	** 
	**	scan for start of major version
	**	    sub-scan for decimal point
	**		sub-scan for end of minor version
	**		extract major & minor versions using start, point, end
	*/
      outer:
	for( int start = 0; start < version.length(); start++ )
	    if ( Character.isDigit( version.charAt( start ) ) )
		for( int point = start + 1; point < version.length(); point++ )
		    if ( ! Character.isDigit( version.charAt( point ) ) )
			if ( version.charAt( point ) != '.' )
			{
			    start = point;	// Not '<num>.'
			    continue outer;	// Resume scanning for start.
			}
			else
			{
			    int end = point;

			    while( ++end < version.length() )
				if ( ! Character.isDigit(version.charAt(end)) )
				    break;

			    if ( (end - point - 1) <= 0 )
			    {
				start = end;	// Not '<num>.<num>'
				continue outer;	// Resume scanning for start
			    }
			    else
			    {
				major = Integer.parseInt( 
					version.substring( start, point ) );
				minor = Integer.parseInt( 
					version.substring( point + 1, end ) );
				break outer;	// We be done!
			    }
			}
    }
    catch( Exception ignore ) {}	// use default

    if ( trace.enabled() ) 
	trace.log( title + ".getDatabaseMinorVersion(): " + minor );
    if ( trace.enabled( 5 ) )
	trace.write( "\t: '" + version + "' => " + major + "." + minor );
    return( minor );
} // getDatabaseMinorVersion


/*
** Name: getUserName
**
** Description:
**	Returns the username as known to the database.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	17-Mar-00 (rajus01)
**	    Use iidbconstants catalog instead of dbmsinfo().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.  Return
**	    empty string rather than null if no data returned by query.
*/

public String 
getUserName() 
    throws SQLException
{

    String	user = empty;
    String 	qry_text = "select user_name from iidbconstants for readonly";
    Statement	stmt = null;
    ResultSet	rs = null;

    if ( trace.enabled() )  trace.log( title + ".getUserName()" );

    try
    {
        stmt = conn.jdbc.createStatement();
        rs = stmt.executeQuery( qry_text );

        if ( rs.next() ) 
	    user = (rs.getString(1)).trim();

	try { rs.close(); } catch( SQLException ignore ) {}
	try { stmt.close(); } catch( SQLException ignore ) {}
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getUsername(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace);

	if ( rs != null )
	    try { rs.close(); } catch( SQLException ignore ) {}

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }
    

    if ( trace.enabled() )  trace.log( title + ".getUserName(): " + user );

    return( user );
} // getUserName


/*
** Name: isReadOnly
**
** Description:
**	Is the database in read-only mode?
**	Returns the state of the connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
isReadOnly() 
    throws SQLException
{
    boolean ro = conn.readOnly;
    if ( trace.enabled() )  trace.log( title + ".isReadOnly(): " + ro );
    return( ro );
} // isReadOnly


/*
** Name: supportsANSI92EntryLevelSQL
**
** Description:
**	Is the ANSI92 entry level SQL grammar supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsANSI92EntryLevelSQL() 
    throws SQLException
{
    boolean entryLvl = true;
    String level = conn.dbCaps.getDbCap( DBMS_DBCAP_SQL92_LEVEL );
    if ( level != null && level.equals( DBMS_SQL92_NONE ) )  entryLvl = false;

    if ( trace.enabled() ) 
	trace.log( title + ".supportsANSI92EntryLevelSQL(): " + entryLvl );
    return( entryLvl );
} // supportsANSI92EntryLevelSQL


/*
** Name: supportsANSI92IntermediateSQL
**
** Description:
**	Is the ANSI92 intermediate SQL grammar supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsANSI92IntermediateSQL() 
    throws SQLException
{
    boolean	result = false;

    String level = conn.dbCaps.getDbCap( DBMS_DBCAP_SQL92_LEVEL );

    if ( 
	 level != null  &&
	 ( level.equals( DBMS_SQL92_INTERMEDIATE )  || 
	   level.equals( DBMS_SQL92_FULL ) ) 
       )
	result = true;

    if ( trace.enabled() )
	trace.log( title + ".supportsANSI92IntermediateSQL(): " + result );
    return( result );
} // supportsANSI92IntermediateSQL


/*
** Name: supportsANSI92FullSQL
**
** Description:
**	Is the ANSI92 full SQL grammar supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsANSI92FullSQL() 
    throws SQLException
{
    boolean   result = false;
    String level = conn.dbCaps.getDbCap( DBMS_DBCAP_SQL92_LEVEL );
    if( level != null && level.equals( DBMS_SQL92_FULL ) )  result = true;
    if ( trace.enabled() )  
	trace.log( title + ".supportsANSI92FullSQL(): " + result );
    return( result );
} // supportsANSI92FullSQL


/*
** Name: supportsIntegrityEnhancementFacility
**
** Description:
**	Is the SQL Integrity Enhancement Facility supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsIntegrityEnhancementFacility() 
    throws SQLException
{
    boolean  result = true;

    String level = conn.dbCaps.getDbCap( DBMS_DBCAP_SQL92_LEVEL );

    if ( 
	 level != null  && 
	 ! level.equals( DBMS_SQL92_FIPS )  &&
	 ! level.equals( DBMS_SQL92_INTERMEDIATE )  && 
	 ! level.equals( DBMS_SQL92_FULL ) 
       )
	result = false;

    if ( trace.enabled() )
	trace.log( title + ".supportsIntegrityEnhancementFacility(): " + result );

    return( result );
} // supportsIntegrityEnhancementFacility


/*
** Name: supportsMinimumSQLGrammar
**
** Description:
**	Is the ODBC Minimum SQL grammar supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsMinimumSQLGrammar() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".supportsMinimumSQLGrammar(): " + true );
    return( true );
} // supportsMinimumSQLGrammar


/*
** Name: supportsCoreSQLGrammar
**
** Description:
**	Is the ODBC Core SQL grammar supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsCoreSQLGrammar() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsCoreSQLGrammar(): " + true );
    return( true );
} // supportsCoreSQLGrammar

/*
** Name: supportsExtendedSQLGrammar
**
** Description:
**	Is the ODBC Extended SQL grammar supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsExtendedSQLGrammar() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsExtendedSQLGrammar(): " + false );
    return( false );
} // supportsExtendedSQLGrammar


/*
** Name: getCatalogTerm
**
** Description:
**	Returns the preferred term for "catalog". Catalogs are
**	not supported by INGRES. Throws Exception.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	25-Feb-00 (rajus01)
**	    Don't throw exception. Make Visual Cafe happy by returning 
**	    an empty string.
*/

public String 
getCatalogTerm() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getCatalogTerm(): '" + empty + "'" );
    return(empty);
} // getCatalogTerm


/*
** Name: getCatalogSeparator
**
** Description:
** 	What is the separator berween catalog and table name?
**	Catalogs NOT SUPPORTED. Throws Exception.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**      31-Jan-07 (weife01)
**          SIR117451. Add support for catalog instead of throw SQLException.
*/
public String 
getCatalogSeparator() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getCatalogSeparator(): '" + empty + "'"); 
        return(empty);
} // getCatalogSeparator


/*
** Name: isCatalogAtStart
**
** Description:
**	Does a catalog appear at the start of a qualified table name?
**	Catalogs NOT SUPPORTED. Throws Exception.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**      31-Jan-07 (weife01)
**          SIR117451. Add support for catalog instead of throw SQLException.
*/

public boolean 
isCatalogAtStart() 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".isCatalogAtStart(): " + false );
    return( false ); 
} // isCatalogAtStart


/*
** Name: getSchemaTerm
**
** Description:
**	Returns the preferred term for "schema"
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public String 
getSchemaTerm() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getSchemaTerm(): " + "schema" );
    return( "schema" );
} // getSchemaTerm


/*
** Name: getProcedureTerm
**
** Description:
**	Returns the preferred term for "procedure"
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public String 
getProcedureTerm() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getProcedureTerm(): " + "procedure" );
    return( "procedure" );
} // getProcedureTerm


/*
** Name: supportsStoredProcedures
**
** Description:
**	Are stored procedure calls supported using 
**	the stored procedure escape syntax?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**      09-Mar-01 (loera01) Bug 104331
**          Now supported.  Return "true".
*/

public boolean 
supportsStoredProcedures() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsStoredProcedures(): " + true );
    return( true );
} // supportsStoredProcedures


/*
** Name: supportsNamedParameters
**
** Description:
**	Are named parameters on callable statements supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-Ma4-03 (gordy)
**	    Created.
*/

public boolean 
supportsNamedParameters() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsNamedParameters(): " + true );
    return( true );
} // supportsNamedParameters


/*
** Name: allProceduresAreCallable
**
** Description:
**	Can all the procedures returned by getProcedures be called
**	the current user?
**
**	FALSE: Because getProcedures returns the procedures
**	created by other users to which the current user may
** 	not have execute privileges.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	02-Dec-99 (rajus01)
**	    Implemented.
*/

public boolean 
allProceduresAreCallable() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".allProceduresAreCallable(): " + false );
    return( false );
} // allProceduresAreCallable


/*
** Name: supportsTransactions
**
** Description:
**	Are transactions supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsTransactions() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".supportsTransactions(): " + true );
    return( true );
} // supportsTransactions


/*
** Name: supportsMultipleTransactions
**
** Description:
**	Can we have multiple transactions open at once (on
**	different connections)?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsMultipleTransactions() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsMultipleTransactions(): " + true );
    return( true );
} // supportsMultipleTransactions


/*
** Name: supportsTransactionIsolationLevel
**
** Description:
**	Does the database support the given transaction isolation level?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**      27-Jul-01 (loera01) Bug 105327
**          Only support serializable isolation level is the catalog level is
**          6.4.
*/

public boolean 
supportsTransactionIsolationLevel( int level )
    throws SQLException
{
    boolean ret=false;

    switch( level )
    {
    case Connection.TRANSACTION_SERIALIZABLE:
        ret = true;
        break;
    
    case Connection.TRANSACTION_REPEATABLE_READ:
    case Connection.TRANSACTION_READ_COMMITTED:
    case Connection.TRANSACTION_READ_UNCOMMITTED:
	if (std_catl_lvl < DBMS_SQL_LEVEL_OI10)
	    ret = false;
	else
	    ret = true;
	break;
    
    default:
	ret = false;
    }

    if ( trace.enabled() )  
	trace.log( title + ".supportsTransactionIsolationLevel( " + 
		   level + " ): " + ret );
    return ret;
} // supportsTransactionIsolationLevel


/*
** Name: getDefaultTransactionIsolation
**
** Description:
**	What is the databases's default transaction isolation level?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getDefaultTransactionIsolation() 
    throws SQLException
{

    if ( trace.enabled() )  
	trace.log( title + ".getDefaultTransactionIsolation(): " +
		   Connection.TRANSACTION_SERIALIZABLE );
    return( Connection.TRANSACTION_SERIALIZABLE );
} // getDefaultTransactionIsolation


/*
** Name: supportsSavepoints
**
** Description:
**	Are savepoints supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

public boolean 
supportsSavepoints() 
    throws SQLException
{
    boolean result = true;

    String sp = conn.dbCaps.getDbCap( DBMS_DBCAP_SAVEPOINT );
    if ( sp != null  &&  (sp.charAt(0) == 'N'  ||  sp.charAt(0) == 'n') )
	result = false;

    if ( trace.enabled() )  
	trace.log( title + ".supportsSavepoints(): " + result );
    return( result );
} // supportsSavepoints


/*
** Name: supportsOpenStatementsAcrossCommit
**
** Description:
**	Can statements remain open across commits?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	20-Jan-01 (gordy)
**	    Driver now tracks transactions and re-prepares as needed.
*/

public boolean 
supportsOpenStatementsAcrossCommit() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".supportsOpenStatementsAcrossCommit(): " + true );
    return( true );
} // supportsOpenStatementsAcrossCommit


/*
** Name: supportsOpenStatementsAcrossRollback
**
** Description:
**	Can statements remain open across rollbacks?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	20-Jan-01 (gordy)
**	    Driver now tracks transactions and re-prepares as needed.
*/

public boolean 
supportsOpenStatementsAcrossRollback() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsOpenStatementsAcrossRollback(): " + true );
    return( true );
} // supportsOpenStatementsAcrossRollback


/*
** Name: supportsOpenCursorsAcrossCommit
**
** Description:
**	Can cursors remain open across commits?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsOpenCursorsAcrossCommit() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsOpenCursorsAcrossCommit(): " + false );
    return( false );
} // supportsOpenCursorsAcrossCommit


/*
** Name: supportsOpenCursorsAcrossRollback
**
** Description:
**	Can cursors remain open across rollbacks?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsOpenCursorsAcrossRollback() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsOpenCursorsAcrossRollback(): " + false );
    return( false );
} // supportsOpenCursorsAcrossRollback


/*
** Name: supportsDataDefinitionAndDataManipulationTransactions
**
** Description:
**	Are both data definition and data manipulation statements
**	within a transaction supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsDataDefinitionAndDataManipulationTransactions()
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + 
		   ".supportsDataDefinitionAndDataManipulationTransactions() : "
		   + true );
    return( true );
} // supportsDataDefinitionAndDataManipulationTransactions


/*
** Name: supportsDataManipulationTransactionsOnly
**
** Description:
**	Are only data manipulation statements within a transaction
**	supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsDataManipulationTransactionsOnly()
    throws SQLException
{
    if ( trace.enabled() )
    	trace.log( title + ".supportsDataManipulationTransactionsOnly(): " +
		   false );
    return( false );
} // supportsDataManipulationTransactionsOnly


/*
** Name: dataDefinitionCausesTransactionCommit
**
** Description:
**	Does a data definition statement within a transaction force the
**	transaction to commit?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
dataDefinitionCausesTransactionCommit()
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".dataDefinitionCausesTransactionCommit(): " + false );
    return( false );
} // dataDefinitionsCausesTransactionCommit


/*
** Name: dataDefinitionIgnoredInTransactions
**
** Description:
**	Is a data definition statement within a transaction ignored?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
dataDefinitionIgnoredInTransactions()
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".dataDefinitionIgnoredInTransactions(): " + false );
    return( false );
} // dataDefinitionIngoredInTransactions


/*
** Name: supportsCatalogsInDataManipulation
**
** Description:
**	Can a catalog name be used in a data manipulation statement?
**	NOT SUPPORTED.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsCatalogsInDataManipulation() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsCatalogsInDataManipulation(): " + false );
    return( false );
} // supportsCatalogsInDataManipulation


/*
** Name: supportsCatalogsInProcedureCalls
**
** Description:
**	Can a catalog name be used in a procedure call statement?
**	NOT SUPPORTED.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsCatalogsInProcedureCalls() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsCatalogsInProcedureCalls(): " + false );
    return( false );
} // supportsCatalogsInProcedureCalls


/*
** Name: supportsCatalogsInTableDefinitions
**
** Description:
**	Can a catalog name be used in a table definition statement?
**	NOT SUPPORTED.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsCatalogsInTableDefinitions() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsCatalogsInTableDefinitions(): " + false );
    return( false );
} // supportsCatalogsinTableDefinitions


/*
** Name: supportsCatalogsInIndexDefinitions
**
** Description:
**	Can a catalog name be used in an index definition statement?
**	NOT SUPPORTED.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsCatalogsInIndexDefinitions() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsCatalogsInIndexDefinitions(): " + false );
    return( false );
} // supportsCatalogsInIndexDefinitions


/*
** Name: supportsCatalogsInPrivilegeDefinitions
**
** Description:
**	Can a catalog name be used in an privilege definition statement?
**	NOT SUPPORTED.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsCatalogsInPrivilegeDefinitions() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsCatalogsInPrivilegeDefinitions(): " + false);
    return( false );
} // supportsCatalogsInProvilegeDefinitions


/*
** Name: supportsSchemasInDataManipulation
**
** Description:
**	Can a schema name be used in a  data manipulation statement?
**	Default is true. For gateways, if the OWNER_NAME value from
**	iidbcapablilites is NO, it returns false. 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSchemasInDataManipulation() 
    throws SQLException
{
    boolean result = true;
    String owner = conn.dbCaps.getDbCap( DBMS_DBCAP_OWNER_NAME );
    
    if ( owner != null && (owner.charAt(0) == 'N' || owner.charAt(0) == 'n') )
	result = false;
    
    if ( trace.enabled() ) 
	trace.log( title + ".supportsSchemasInDataManipulation(): " + result );
    return( result );
} // supportsSchemasInDataManipulations


/*
** Name: supportsSchemasInProcedureCalls
**
** Description:
**	Can a schema name be used in a procedure call statement?
**	Default is true. For gateways, if the OWNER_NAME value from
**	iidbcapablilites is NO, it returns false. 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSchemasInProcedureCalls() 
    throws SQLException
{
    boolean result = true;
    String owner = conn.dbCaps.getDbCap( DBMS_DBCAP_OWNER_NAME );
    
    if ( owner != null && (owner.charAt(0) == 'N' || owner.charAt(0) == 'n') )
	result = false;
    
    if ( trace.enabled() ) 
	trace.log( title + ".supportsSchemasInProcedureCalls(): " + result );
    return( result );
} // supportsSchemaInProcedureCalls


/*
** Name: supportsSchemasInTableDefinitions
**
** Description:
**	Can a schema name be used in a table definition statement?
**	Default is true. For gateways, if the OWNER_NAME value from
**	iidbcapablilites is NO, it returns false. 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSchemasInTableDefinitions() 
    throws SQLException
{
    boolean result = true;
    String owner = conn.dbCaps.getDbCap( DBMS_DBCAP_OWNER_NAME );
    
    if ( owner != null && (owner.charAt(0) == 'N' || owner.charAt(0) == 'n') )
	result = false;
    
    if ( trace.enabled() )
	trace.log( title + ".supportsSchemasInTableDefinitions(): " + result );
    return( result );
} // supportsSchemasInTableDefinitions


/*
** Name: supportsSchemasInIndexDefinitions
**
** Description:
**	Can a schema name be used in an index definition statement?
**	Default is true. For gateways, if the OWNER_NAME value from
**	iidbcapablilites is NO, it returns false. 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSchemasInIndexDefinitions() 
    throws SQLException
{
    boolean result = true;
    String owner = conn.dbCaps.getDbCap( DBMS_DBCAP_OWNER_NAME );
    
    if ( owner != null && (owner.charAt(0) == 'N' || owner.charAt(0) == 'n') )
	result = false;
    
    if ( trace.enabled() )  
	trace.log( title + ".supportsSchemasInIndexDefinitions(): " + result );
    return( result );
} // supportsSchemasInIndexDefinitions


/*
** Name: supportsSchemasInPrivilegeDefinitions
**
** Description:
**	Can a schema name be used in a privilege definition statement?
**	Default is true. For gateways, if the OWNER_NAME value from
**	iidbcapablilites is NO, it returns false. 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSchemasInPrivilegeDefinitions() 
    throws SQLException
{
    boolean result = true;
    String owner = conn.dbCaps.getDbCap( DBMS_DBCAP_OWNER_NAME );
    
    if ( owner != null && (owner.charAt(0) == 'N' || owner.charAt(0) == 'n') )
	result = false;
    
    if ( trace.enabled() )
	trace.log( title + ".supportsSchemasInPrivilegeDefinitions(): " + result);
    return( result );
} // supportsSchemasInPrivilegeDefinitions


/*
** Name: supportsAlterTableWithAddColumn
**
** Description:
**	Is "ALTER TABLE" with add column supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsAlterTableWithAddColumn() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsAlterTableWithAddColumn(): " + false );
    return( false );
} // supportsAlterTableWithAddColumn


/*
** Name: supportsAlterTableWithDropColumn
**
** Description:
**	Is "ALTER TABLE" with drop column supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsAlterTableWithDropColumn() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsAlterTableWithDropColumn(): " + "false" );
    return( false );
} // supportsAlterTableWithDropColumn


/*
** Name: supportsPositionedDelete
**
** Description:
**	Is positioned DELETE supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsPositionedDelete() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsPositionedDelete(): " + true );
    return( true );
} // supportsPositionedDelete


/*
** Name: supportsPositionedUpdate
**
** Description:
**	Is positioned UPDATE supported?
**
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsPositionedUpdate() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsPositionedUpdate(): " + true );
    return( true );
} // supportsPositionedUpdate


/*
** Name: supportsSelectForUpdate
**
** Description:
**	Is SELECT for UPDATE supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSelectForUpdate() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsSelectForUpdate(): " + true );
    return( true );
} // supportsSelectForUpdate


/*
** Name: allTablesAreSelectable
**
** Description:
**	Can all the tables returned by getTable be SELECTed by the
**	current user?
**	
**	FALSE: Because getTable returns the tables
**	created by other users to which the current user may
** 	not have SELECT privileges.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	02-Dec-99 (rajus01)
**	    Implemented.
*/

public boolean 
allTablesAreSelectable() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".allTablesAreSelectable(): " + false );
    return( false );
} // allTablesAreSelectable


/*
** Name: supportsUnion
**
** Description:
**	Is SQL UNION supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsUnion() 
    throws SQLException
{
    boolean result = true;

    String union = conn.dbCaps.getDbCap( DBMS_DBCAP_UNION );
    if ( union != null && (union.charAt(0) == 'N' || union.charAt(0) == 'n') )
	result = false;

    if ( trace.enabled() )  trace.log( title + ".supportsUnion(): " + result );
    return( result );
} // supportsUnion


/*
** Name: supportsUnionAll
**
** Description:
**	Is SQL UNION ALL supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsUnionAll() 
    throws SQLException
{
    boolean result = true;
    String union = conn.dbCaps.getDbCap( DBMS_DBCAP_UNION );

    if( union != null && !union.equals( DBMS_UNION_ALL ) )
	result = false;
    if ( trace.enabled() )  
	trace.log( title + ".supportsUnionAll(): " + result );
    return( result );
} // supportsUnionAll


/*
** Name: supportsOuterJoins
**
** Description:
**	Is some form of outer join supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsOuterJoins() 
    throws SQLException
{
    boolean	result = true;
    String join = conn.dbCaps.getDbCap( DBMS_DBCAP_OUTER_JOIN );
    
    if( join != null && (join.charAt(0) == 'N' || join.charAt(0) == 'n') )  
	result = false;
    
    if ( trace.enabled() )  
	trace.log( title + ".supportsOuterJoins(): " + result );
    return( result );
} // supportsOuterJoins


/*
** Name: supportsLimitedOuterJoins
**
** Description:
**	Is there limited support for outer joins? This will be
**	true if supportFullOuterJoins is true.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsLimitedOuterJoins() 
    throws SQLException
{
    boolean	result = supportsOuterJoins(); 
    if ( trace.enabled() )  
	trace.log( title + ".supportsLimitedOuterJoins(): " + result );
    return( result );
} // supportsLimitedOuterJoins


/*
** Name: supportsFullOuterJoins
**
** Description:
**	Are full nested outer joins supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsFullOuterJoins() 
    throws SQLException
{
    boolean	result = supportsOuterJoins();
    if ( trace.enabled() ) 
	trace.log( title + ".supportsFullOuterJoins(): "  + result );
    return( result );
} // supportsFullOuterJoins


/*
** Name: supportsGroupBy
**
** Description:
**	Is some form of "GROUP BY" clause supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsGroupBy() 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".supportsGroupBy(): " + true );
    return( true );
} // supportsGroupBy


/*
** Name: supportsGroupByBeyondSelect
**
** Description:
**	Can a "GROUP BY" clause add columns not in the SELECT
**	provided it specifies all the columns in the SELECT?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsGroupByBeyondSelect() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsGroupByBeyondSelect(): " + true );
    return( true );
} // supportsGroupByBeyondSelect


/*
** Name: supportsGroupByUnrelated
**
** Description:
**	Can a "GROUP BY" clause use columns in the SELECT?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsGroupByUnrelated() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsGroupByUnrelated(): " + false );
    return( false );
} // supportsGroupByUnrelated


/*
** Name: supportsOrderByUnrelated
**
** Description:
**	Can an "ORDER BY" clause use columns not in the SELECT?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsOrderByUnrelated() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsOrderByUnrelated(): " + false );
    return( false );
} // supportsOrderByUnrelated


/*
** Name: supportsExpressionsInOrderBy
**
** Description:
**	Are expressions in "ORDER BY" lists supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsExpressionsInOrderBy() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsExpressionsInOrderBy(): " + false );
    return( false );
} // supportsExpressionsInOrderBy


/*
** Name: supportsSubqueriesInComparisons
**
** Description:
**	Are subqueries in comparison expressions supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSubqueriesInComparisons() 
    throws SQLException
{
    if ( trace.enabled() ) 
        trace.log( title + ".supportsSubqueriesInComparisons(): " + true );
    return( true );
} // supportsSubqueriesInComparisons


/*
** Name: supportsSubqueriesInExists
**
** Description:
**	Are subqueries in 'exists' expressions supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSubqueriesInExists() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsSubqueriesInExists(): " + true );
    return( true );
} // supportsSubqueriesInExists


/*
** Name: supportsSubqueriesInIns
**
** Description:
**	Are subqueries in 'in' statements supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSubqueriesInIns() 
    throws SQLException
{
    if ( trace.enabled() ) 
    	trace.log( title + ".supportsSubqueriesInIns(): " + true );
    return( true );
} // supportsSubqueriesInIns


/*
** Name: supportsSubqueriesInQuantifieds
**
** Description:
**	Are subqueries in quantified expressions supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsSubqueriesInQuantifieds() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsSubqueriesInQuantifieds(): " + true );
    return( true );
} // supportsSubqueriesInQuantifieds


/*
** Name: supportsCorrelatedSubqueries
**
** Description:
**	Are correlated subqueries supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsCorrelatedSubqueries() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsCorrelatedSubqueries(): " + true );
    return( true );
} // supportsCorrelatedSubqueries


/*
** Name: supportsTableCorrelationNames
**
** Description:
**	Are table correlation names supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsTableCorrelationNames() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsTableCorrelationNames(): " + true );
    return( true );
} // supportsTableCorrelationNames


/*
** Name: supportsDifferentTableCorrelationNames
**
** Description:
**	Are table correlation names restricted to be different
**	from the names of the tables?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsDifferentTableCorrelationNames() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".supportsDifferentTableCorrelationNames(): " + false );
    return( false );
} // supportsDifferentTableCorrelationNames


/*
** Name: supportsColumnAliasing
**
** Description:
**	Is Column aliasing supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsColumnAliasing() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".supportsColumnAliasing(): " + true );
    return( true );
} // supportsColumnAliasing


/*
** Name: supportsNonNullableColumns
**
** Description:
**	Can columns be defined as non-nullable?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsNonNullableColumns() 
    throws SQLException
{
    if ( trace.enabled() ) 
	trace.log( title + ".supportsNonNullableColumns(): " + true );
    return( true );
} // supportsNonNullableColumns


/*
** Name: supportsConvert
**
** Description:
**	Is the CONVERT function between SQL types supported ?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsConvert() 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".supportsConvert(): " + true );
    return( true );
} // supportsConvert

/*
** Name: supportsConvert
**
** Description:
**	Is the CONVERT between the given SQL types supported ?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	13-Jun-00 (gordy)
**	    Method getConvertTypes() now static.
**      15-Mar-01 (loera01) Bug 104330
**          Pre-emptively make sure fromType is supported.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
*/

public boolean 
supportsConvert( int fromType, int toType ) 
    throws SQLException
{
    String	fromStr = getStringType( fromType ); 
    String 	toStr   = getStringType( toType ); 
    boolean	match_found = false;
    boolean	convert = false;
    Enumeration	type_enum = SqlParse.getConvertTypes();

    if ( trace.enabled() )
	trace.log( title + ".supportsConvert( " +
		   fromType + ", " + toType + " ): " );

    for( Enumeration e = type_enum; e.hasMoreElements(); )
    {
        if( e.nextElement().toString().equals( fromStr ) )
	{
	    match_found = true;
	    break;
	}
    }
    if (match_found)
    {
	match_found = false;
        for( Enumeration e = type_enum; e.hasMoreElements(); )
        {
            if( e.nextElement().toString().equals( toStr ) )
	    {
	        match_found = true;
	        break;
	    }
        }
    }

    if ( match_found )
    {
	if ( toType == fromType )
	    convert = true;	
	else if ( toType == Types.LONGVARBINARY || toType == Types.LONGVARCHAR )
	    convert = false;
	else if ( toType == Types.BINARY  ||  toType == Types.VARBINARY )
	{
	    if ( fromType != Types.BIT  &&  fromType != Types.BOOLEAN  &&
		 fromType != Types.BIGINT  &&  fromType != Types.LONGVARCHAR )
	    	convert = true;
	}
	else if ( toType == Types.CHAR  ||  toType == Types.VARCHAR )
	{
	    if ( fromType != Types.BIT  &&  fromType != Types.BOOLEAN  &&
		 fromType != Types.BIGINT )
	    	convert = true;
	}
	else if( ( toType == Types.DECIMAL ) ||
		 ( toType == Types.DOUBLE ) ||
		 ( toType == Types.FLOAT ) ||
		 ( toType == Types.NUMERIC ) ||
		 ( toType == Types.INTEGER ) ||
		 ( toType == Types.REAL ) ||
		 ( toType == Types.SMALLINT ) ||
		 ( toType == Types.TINYINT )
	       ) 
	{
	    if( ( fromType == Types.CHAR ) ||
		( fromType == Types.DECIMAL ) ||
		( fromType == Types.DOUBLE ) ||
		( fromType == Types.FLOAT ) ||
		( fromType == Types.NUMERIC ) ||
		( fromType == Types.INTEGER ) ||
		( fromType == Types.REAL  ) ||
		( fromType == Types.SMALLINT ) ||
		( fromType == Types.TINYINT ) ||
		( fromType == Types.VARCHAR )
	      )
		convert = true;
	} else if( toType == Types.DATE )
	{
	    if( (fromType == Types.CHAR ) || ( fromType == Types.VARCHAR ) ||
		( fromType == Types.TIMESTAMP )
	      )
		convert = true;
	} else if( toType == Types.TIME )
	{
	    if( (fromType == Types.CHAR ) || ( fromType == Types.VARCHAR ) ||
		( fromType == Types.TIMESTAMP )
	      )
		convert = true;
	} else if( toType == Types.TIMESTAMP )
	{
	    if( (fromType == Types.CHAR ) || ( fromType == Types.VARCHAR ) ||
		( fromType == Types.DATE ) || ( fromType == Types.TIME ) 
	      )
		convert = true;
	}
    }

    if ( trace.enabled() )
	trace.log( title + ".supportsConvert( " +
		   fromType + ", " + toType + " ): " + convert );

    return( convert );
} // supportsConvert


/*
** Name: getSQLKeywords
**
** Description:
**	Returns a comma separated list of all a database's SQL
**	keywords that are NOT also SQL92 keywords.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public String 
getSQLKeywords() 
    throws SQLException
{
    String sql_keywords = "abort,byref,callproc,comment,copy,"+
			  "define,disable,do,elseif,"+
			  "enable,endif,endloop,endwhile,"+
			  "excluding,if,import,index,integrity,"+
			  "message,modify,permit,raise,"+
			  "referencing,register,relocate,remove,"+
			  "repeat,return,save,savepoint,until,while.";

    if ( trace.enabled() )  
	trace.log( title + ".getSQLKeywords(): " + sql_keywords);
    return( sql_keywords );
} // getSQLKeywords


/*
** Name: getNumericFunctions
**
** Description:
**	Returns a comma separted list of math functions.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	13-Jun-00 (gordy)
**	    Method getNumFuncs() now static.
*/

public String 
getNumericFunctions() 
    throws SQLException
{
    String str = SqlParse.getNumFuncs();
    if ( trace.enabled() ) 
	trace.log( title + ".getNumericFunctions(): " + str );
    return( str );
} // getNumericFunctions


/*
** Name: getStringFunctions
**
** Description:
**	Returns a comma separted list of string functions.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	13-Jun-00 (gordy)
**	    Method getStrFuncs() now static.
*/

public String 
getStringFunctions() 
    throws SQLException
{
    String str = SqlParse.getStrFuncs();
    if ( trace.enabled() ) 
	trace.log( title + ".getStringFunctions(): " + str );
    return( str );
} // getStringFunctions


/*
** Name: getSystemFunctions
**
** Description:
**	Returns a comma separted list of system functions.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	13-Jun-00 (gordy)
**	    Method getSysFuncs() now static.
*/

public String 
getSystemFunctions() 
    throws SQLException
{
    String str = SqlParse.getSysFuncs();
    if ( trace.enabled() ) 
	trace.log( title + ".getSystemFunctions(): " + str );
    return( str );
} // getSystemFunctions


/*
** Name: getTimeDateFunctions
**
** Description:
**	Returns a comma separated list of time and date functions. 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	13-Jun-00 (gordy)
**	    Method getTdtFuncs() now static.
*/

public String 
getTimeDateFunctions() 
    throws SQLException
{
    String str = SqlParse.getTdtFuncs();
    if ( trace.enabled() ) 
	trace.log( title + ".getTimeDateFunctions(): " + str );
    return( str );
} // getTimeDateFunctions


/*
** Name: supportsLikeEscapeClause
**
** Description:
**	Is the escape character in "LIKE" clauses supported?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	26-Mar-09 (rajus01) Bug 121865
**	    Cleaned up checking.
*/

public boolean 
supportsLikeEscapeClause() 
    throws SQLException
{
    boolean result = (escape_char != null);

    if ( trace.enabled() )  
	trace.log( title + ".supportsLikeEscapeClause(): " + result);

    return( result );
} // supportsLikeEscapeClause


/*
** Name: getSearchStringEscape
**
** Description:
**	Returns the string that can be used to escape wildcard
**	characters.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	16-Dec-08 (Gordy)
**	    Only return escape character if defined by DBMS or
**	    escape clause is supported by DBMS (in which case
**	    the default escape character is returned).
**	26-Mar-09 (rajus01) Bug 121865
**	    Cleaned up checking.
*/

public String 
getSearchStringEscape() 
    throws SQLException
{
    String  result = (escape_char != null) ? escape_char : empty; 

    if ( trace.enabled() )  
	trace.log( title + ".getSearchStringEscape(): '" + result + "'" );

    return( result );
} // getSearchStringEscape


/*
** Name: getExtraNameCharacters
**
** Description:
**	Returns all the "extra" characters that can be used in unquoted
**	identifier names.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public String 
getExtraNameCharacters() 
    throws SQLException
{
    String   result =  "@#$";
    String ident_char  = conn.dbCaps.getDbCap( DBMS_DBCAP_IDENT_CHAR );
    if ( ident_char != null )  result = ident_char;
    if ( trace.enabled() )
	trace.log( title + ".getExtraNameCharacters(): " + result );
    return( result );
} // getExtraNameCharacters


/*
** Name: getIdentifierQuoteString
**
** Description:
**	Returns the string used to quote SQL identifiers.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	 5-Feb-02 (gordy)
**	    Return " " if delimited identifiers not supported
*/

public String 
getIdentifierQuoteString() 
    throws SQLException
{
    String  quote = (conn.dbCaps.getDbCap( DBMS_DBCAP_DELIM_CASE ) != null) 
		    ? "\"" : space;
    if ( trace.enabled() ) 
	trace.log( title + ".getIdentifierQuoteString(): '" + quote + "'" );
    return( quote );
} // getIdentifierQuoteString


/*
** Name: storesLowerCaseIdentifiers
**
** Description:
**	Does the database treat mixed case unquoted SQL identifiers as
**	case insensitive and store them in lower case?
**	Returns the DB_NAME_CASE value from iidbcapabilities.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
storesLowerCaseIdentifiers() 
    throws SQLException
{
    boolean lower = true;
    String  db_name_case = conn.dbCaps.getDbCap( DBMS_DBCAP_NAME_CASE );

    if ( db_name_case != null  &&  
	 ! db_name_case.equals( DBMS_NAME_CASE_LOWER ) )
	lower = false;
    
    if ( trace.enabled() ) 
	trace.log( title + ".storesLowerCaseIdentifiers(): " + lower );
    return( lower );
} // storesLowerCaseIdentifiers


/*
** Name: storesUpperCaseIdentifiers
**
** Description:
**	Does the database treat mixed case unquoted SQL identifiers
**	as case insensitive and stores them in upper case?
**	Returns the DB_NAME_CASE value from iidbcapabilities.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
storesUpperCaseIdentifiers() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".storesUpperCaseIdentifiers(): " + upper );
    return( upper );
} // storesUpperCaseIdentifiers


/*
** Name: storesMixedCaseIdentifiers
**
** Description:
**	Does the database treat mixed case unquoted SQL identifiers
**	as case insensitive and store them in mixed case?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
storesMixedCaseIdentifiers() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".storesMixedCaseIdentifiers(): " + false );
    return( false );
} // storesMixedCaseIdentifiers


/*
** Name: supportsMixedCaseIdentifiers
**
** Description:
**	Does the database treat mixed case unquoted SQL identifiers
**	as case sensitive and as a result store them in mixed case?
**	Returns the DB_NAME_CASE value from iidbcapabilities.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsMixedCaseIdentifiers() 
    throws SQLException
{
    boolean mixed = false;
    String  db_name_case = conn.dbCaps.getDbCap( DBMS_DBCAP_NAME_CASE );

    if ( db_name_case != null  &&  db_name_case.equals( DBMS_NAME_CASE_MIXED ) )
	mixed = true;

    if ( trace.enabled() )
	trace.log( title + ".supportsMixedCaseIdentifiers(): " + mixed );
    return( mixed );
} // supportsMixedCaseIdentifiers


/*
** Name: storesLowerCaseQuotedIdentifiers
**
** Description:
**	Does the database treat mixed case quoted SQL identifiers as
**	case insensitive and store them in lower case?
**	Returns the DB_DELIMITED_CASE value from iidbcapabilities.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	 5-Feb-02 (gordy)
**	    Switched default to FALSE for DBMS which don't support
**	    delimited identifiers.
*/

public boolean 
storesLowerCaseQuotedIdentifiers() 
    throws SQLException
{
    boolean lower = false;
    String  db_delim_case = conn.dbCaps.getDbCap( DBMS_DBCAP_DELIM_CASE );

    if ( db_delim_case != null  &&  
	 db_delim_case.equals( DBMS_NAME_CASE_LOWER ) )  
	lower = true;
    
    if ( trace.enabled() ) 
	trace.log( title + ".storesLowerCaseQuotedIdentifiers(): " + lower );
    return( lower );
} // storesLowerCaseQuotedIdentifiers


/*
** Name: storesUpperCaseQuotedIdentifiers
**
** Description:
**	Does the database treat mixed case quoted SQL identifiers as
**	case insensitive and store them in upper case?
**	Returns the DB_DELIMITED_CASE value from iidbcapabilities.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
storesUpperCaseQuotedIdentifiers() 
    throws SQLException
{
    boolean upper = false;
    String  db_delim_case = conn.dbCaps.getDbCap( DBMS_DBCAP_DELIM_CASE );

    if ( db_delim_case != null  &&  
	 db_delim_case.equals( DBMS_NAME_CASE_UPPER ) )
	upper = true;

    if ( trace.enabled() ) 
	trace.log( title + ".storesUpperCaseQuotedIdentifiers(): " + upper );
    return( upper );
} // storesUpperCaseQuotedIdentifiers


/*
** Name: storesMixedCaseQuotedIdentifiers
**
** Description:
**	Does the database treat mixed case quoted SQL identifiers as
**	case insensitive and store them in mixed case?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
storesMixedCaseQuotedIdentifiers() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".storesMixedCaseQuotedIdentifiers(): " + false);
    return( false );
} // storesMixedCaseQuotedIdentifiers


/*
** Name: supportsMixedCaseQuotedIdentifiers
**
** Description:
**	Does the database treat mixed case quoted SQL identifiers
**	as case sensitive and as a result store them in mixed case?
**	Returns the DB_DELIMITED_CASE value from iidbcapabilities.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
supportsMixedCaseQuotedIdentifiers() 
    throws SQLException
{
    boolean mixed = false;
    String  db_delim_case = conn.dbCaps.getDbCap( DBMS_DBCAP_DELIM_CASE );

    if ( db_delim_case != null  &&  
	 db_delim_case.equals( DBMS_NAME_CASE_MIXED ) )
	mixed = true;
    
    if ( trace.enabled() ) 
	trace.log( title + ".supportsMixedCaseQuotedIdentifiers(): " + mixed );
    return( mixed );
} // supportsMixedCaseQuotedIdentifiers


/*
** Name: nullsAreSortedHigh
**
** Description:
**	Are NULL values sorted high?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
nullsAreSortedHigh() 
    throws SQLException
{
    boolean	result = true;

    String sort  = conn.dbCaps.getDbCap( DBMS_DBCAP_NULL_SORTING );

    if( sort != null && !sort.equals( DBMS_NULL_SORT_HIGH ) )
	result = false;

    if ( trace.enabled() )  
	trace.log( title + ".nullsAreSortedHigh(): " + result );
    return( result );
} // nullsAreSortedHigh


/*
** Name: nullsAreSortedLow
**
** Description:
**	Are NULL values sorted low?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
nullsAreSortedLow() 
    throws SQLException
{
    boolean	result = false;

    String sort  = conn.dbCaps.getDbCap( DBMS_DBCAP_NULL_SORTING );

    if( sort != null && sort.equals( DBMS_NULL_SORT_LOW ) )
	result = true;

    if ( trace.enabled() )  
	trace.log( title + ".nullsAreSortedLow(): " + result );
    return( result );
} // nullsAreSortedLow


/*
** Name: nullsAreSortedAtStart
**
** Description:
** 	Are NULL values sorted at the start regardless of the
**	sort order?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
nullsAreSortedAtStart() 
    throws SQLException
{
    boolean	result = false;

    String sort  = conn.dbCaps.getDbCap( DBMS_DBCAP_NULL_SORTING );

    if( sort != null && sort.equals( DBMS_NULL_SORT_FIRST ) )
	result = true;
    if ( trace.enabled() )  
	trace.log( title + ".nullsAreSortedAtStart(): " + result );
    return( result );
} // nullsAreSortedAtStart


/*
** Name: nullsAreSortedAtEnd
**
** Description:
** 	Are NULL values sorted at the end regardless of the
**	sort order?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
nullsAreSortedAtEnd() 
    throws SQLException
{
    boolean	result = false;

    String sort  = conn.dbCaps.getDbCap( DBMS_DBCAP_NULL_SORTING );

    if( sort != null && sort.equals( DBMS_NULL_SORT_LAST ) )
	result = true;

    if ( trace.enabled() )  
	trace.log( title + ".nullsAreSortedAtEnd(): " + result );
    return( result );
} // nullsAreSortedAtEnd


/*
** Name: nullPlusNonNullIsNull
**
** Description:
**	Are concatenations between NULL and non-NULL values NULL?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
nullPlusNonNullIsNull() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".nullPlusNonNullIsNull(): " + true);
    return( true );
} // nullPlusNonNullIsNull


/*
** Name: deletesAreDetected
**
** Description:
**	Are deletes detected by ResultSet.rowDeleted().
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if detected, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	22-May-07 (gordy)
**	    Visible deletes in scrollable cursors are detected.
*/

public boolean
deletesAreDetected( int type )
{
    boolean detected = false;
    
    switch( type )
    {
    case ResultSet.TYPE_FORWARD_ONLY :
    case ResultSet.TYPE_SCROLL_SENSITIVE :
    case ResultSet.TYPE_SCROLL_INSENSITIVE :	detected = true;	break;
    }
    
    if ( trace.enabled() )  
	trace.log(title + ".deletesAreDetected( " + type + " ): " + detected);
    return( detected );
} // deletesAreDetected


/*
** Name: insertsAreDetected
**
** Description:
**	Are inserts detected by ResultSet.rowInserted().
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if detected, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	22-May-07 (gordy)
**	    Inserts are not detected.
*/

public boolean
insertsAreDetected( int type )
{
    if ( trace.enabled() )  
	trace.log( title + ".insertsAreDetected( " + type + " ): " + false );
    return( false );
} // insertsAreDetected


/*
** Name: updatesAreDetected
**
** Description:
**	Are updates detected by ResultSet.rowUpdated().
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if detected, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	22-May-07 (gordy)
**	    Updates are not detected for scrollable cursors.
*/

public boolean
updatesAreDetected( int type )
{
    boolean detected = false;
    
    switch( type )
    {
    case ResultSet.TYPE_FORWARD_ONLY :		detected = true;	break;
    }
    
    if ( trace.enabled() )  
	trace.log(title + ".updatesAreDetected( " + type + " ): " + detected);
    return( detected );
} // updatesAreDetected


/*
** Name: ownDeletesAreVisible
**
** Description:
**	Are a result-sets deletes visible to the result-set.
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if visible, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	22-May-07 (gordy)
**	    Cursor deletes are visible to senstive cursors.
*/

public boolean
ownDeletesAreVisible( int type )
{
    boolean visible = false;
    
    switch( type )
    {
    case ResultSet.TYPE_FORWARD_ONLY :
    case ResultSet.TYPE_SCROLL_SENSITIVE :	visible = true;	break;
    }
    
    if ( trace.enabled() )  
	trace.log(title + ".ownDeletesAreVisible( " + type + " ): " + visible);
    return( visible );
} // ownDeletesAreVisible


/*
** Name: ownInsertsAreVisible
**
** Description:
**	Are a result-sets inserts visible to the result-set.
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if visible, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	22-May-07 (gordy)
**	    Inserts are not visible.
*/

public boolean
ownInsertsAreVisible( int type )
{
    if ( trace.enabled() )  
	trace.log( title + ".ownInsertsAreVisible( " + type + " ): " + false );
    return( false );
} // ownInsertsAreVisible


/*
** Name: ownUpdatesAreVisible
**
** Description:
**	Are a result-sets updates visible to the result-set.
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if visible, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	22-May-07 (gordy)
**	    Cursor updates are visible to sensitive cursors.
*/

public boolean
ownUpdatesAreVisible( int type )
{
    boolean visible = false;
    
    switch( type )
    {
    case ResultSet.TYPE_FORWARD_ONLY :
    case ResultSet.TYPE_SCROLL_SENSITIVE :	visible = true;	break;
    }
    
    if ( trace.enabled() )  
	trace.log(title + ".ownUpdatesAreVisible( " + type + " ): " + visible);
    return( visible );
} // ownUpdatesAreVisible


/*
** Name: othersDeletesAreVisible
**
** Description:
**	Are deletes visible to the result-set.
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if visible, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	22-May-07 (gordy)
**	    Non-cursor deletes are visible to sensitive cursors.
*/

public boolean
othersDeletesAreVisible( int type )
{
    boolean visible = false;
    
    switch( type )
    {
    case ResultSet.TYPE_SCROLL_SENSITIVE :	visible = true;	break;
    }
    
    if ( trace.enabled() )  
	trace.log(title + ".othersDeletesAreVisible(" + type + "): " + visible);
    return( visible );
} // othersDeletesAreVisible


/*
** Name: othersInsertsAreVisible
**
** Description:
**	Are inserts visible to the result-set.
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if visible, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	22-May-07 (gordy)
**	    Inserts are not visible.
*/

public boolean
othersInsertsAreVisible( int type )
{
    if ( trace.enabled() )  
	trace.log( title + ".othersInsertsAreVisible(" + type + "): " + false );
    return( false );
} // othersInsertsAreVisible


/*
** Name: othersUpdatesAreVisible
**
** Description:
**	Are updates visible to the result-set.
**
** Input:
**	type	Result-set type.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if visible, FALSE otherwise.
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	 1-Nov-03 (gordy)
**	    Implemented updatable result-sets.
**	22-May-07 (gordy)
**	    Non-cursor updates are visible to sensitive cursors.
*/

public boolean
othersUpdatesAreVisible( int type )
{
    boolean visible = false;
    
    switch( type )
    {
    case ResultSet.TYPE_SCROLL_SENSITIVE :	visible = true;	break;
    }
    
    if ( trace.enabled() )  
	trace.log(title + ".othersUpdatesAreVisible(" + type + "): " + visible);
    return( visible );
} // othersUpdatesAreVisible


/*
** Name: locatorsUpdateCopy
**
** Description:
**	Do locators update copy of object.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	TRUE if copy updated, FALSE otherwise.
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
**	26-Feb-07 (gordy)
**	    Implemented.
*/

public boolean
locatorsUpdateCopy()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".locatorsUpdateCopy(): true" );
    return( true );
} // locatorsUpdateCopy


/*
** Name: getMaxConnections
**
** Description:
**	How many active connections can we have at a time to this database?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxConnections() 
    throws SQLException
{
    int dont_know = 0;
    if ( trace.enabled() )  
	trace.log( title + ".getMaxConnections(): " + dont_know);
    return( dont_know );
} // getMaxConnections


/*
** Name: getMaxStatements
**
** Description:
**	How many active statements can we have open at one time to this
**	database?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxStatements() 
    throws SQLException
{
    int dont_know = 0;

    String max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_STMTS );

    if( max_len != null )
	try { dont_know = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if ( dont_know < 0 )  dont_know = 0;
    if ( trace.enabled() )  
	trace.log( title + ".getMaxStatements(): " + dont_know );
    return( dont_know );
} // getMaxStatements


/*
** Name: getMaxRowSize
**
** Description:
**	What is the maximum length of a single row?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxRowSize() 
    throws SQLException
{
    int result = 0;

    String max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_ROW_LEN );

    if( max_len != null )
	try { result = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};
    if( result < 0 )  result = 0;

    if ( trace.enabled() )  trace.log( title + ".getMaxRowSize(): " + result );
    return( result );
} // getMaxRowSize


/*
** Name: doesMaxRowSizeIncludeBlobs
**
** Description:
**	Did getMaxRowSize() include LONGVARCHAR and LONGVARBINARY
**	blobs?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public boolean 
doesMaxRowSizeIncludeBlobs() 
    throws SQLException
{
    if ( trace.enabled() )
	trace.log( title + ".doesMaxRowSizeIncludeBlobs(): " + false );
    return( false );
} // doesMaxRowSizeIncludeBlobs


/*
** Name: getMaxIndexLength
**
** Description:
**	What is the maximum length of an index (in bytes)?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxIndexLength() 
    throws SQLException
{
    int dont_know = 0;
    if ( trace.enabled() )  
	trace.log( title + ".getMaxIndexLength(): " + dont_know );
    return( dont_know );
} // getMaxIndexLength


/*
** Name: getMaxStatementLength
**
** Description:
**	What is the maximum length of a SQL statement?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxStatementLength() 
    throws SQLException
{
    int dont_know = 0;
    if ( trace.enabled() )
	trace.log( title + ".getMaxStatementLength(): " + dont_know );
    return( dont_know );
} // getMaxStatmentLength


/*
** Name: getMaxCatalogNameLength
**
** Description:
**	What is the maximum length of a catalog name?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**      31-Jan-07 (weife01)
**          SIR 117451. Add support for catalog instead of throw SQLException.
*/

public int 
getMaxCatalogNameLength() 
    throws SQLException
{
    int dont_know = 0;
    if ( trace.enabled() )  
	trace.log( title + ".getMaxCatalogNameLength(): " + dont_know );
        return 0;
} // getMaxCatalogNameLength


/*
** Name: getMaxSchemaNameLength
**
** Description:
**	What is the maximum length allowed for a schema name?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxSchemaNameLength() 
    throws SQLException
{
    int len = DBMS_OI10_IDENT_LEN;    // OpenIngres, Ingres II etc.

    if( std_catl_lvl < DBMS_SQL_LEVEL_OI10 )
	len = DBMS_ING64_IDENT_LEN;

    String max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_SCH_NAME );

    if( max_len != null )
	try { len = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if ( trace.enabled() )  
    	trace.log( title + ".getMaxSchemaNameLength(): " + len );
    return( len );
} // getMaxSchemaNameLength


/*
** Name: getMaxProcedureNameLength
**
** Description:
**	What is the maximum length of a procedure name?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	 1-Oct-09 (gordy)
**	    Check DBMS capabilities for max length of procedure names.
**	    If dbcap entry does not exist, try the max table name length
**	    since table and procedure names are at the same level of the
**	    object name hierarchy and the max table name dbcap existed
**	    before the max proc dbcap was added.
*/

public int 
getMaxProcedureNameLength() 
    throws SQLException
{
    int len = DBMS_OI10_IDENT_LEN;    // OpenIngres, Ingres II etc.

    if( std_catl_lvl < DBMS_SQL_LEVEL_OI10 )
	len = DBMS_ING64_IDENT_LEN;

    String max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_PRC_NAME );

    if ( max_len == null )
	max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_TAB_NAME );

    if( max_len != null )
	try { len = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if ( trace.enabled() )  
	trace.log( title + ".getMaxProcedureNameLength(): " + len );
    return( len );
} // getMaxProcedureNameLength


/*
** Name: getMaxTableNameLength
**
** Description:
**	What is the maximum length of a table name?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxTableNameLength() 
    throws SQLException
{
    int len = DBMS_OI10_IDENT_LEN;    // OpenIngres, Ingres II etc.

    if( std_catl_lvl < DBMS_SQL_LEVEL_OI10 )
	len = DBMS_ING64_IDENT_LEN;

    String max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_TAB_NAME );
    if( max_len != null )
	try { len = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if ( trace.enabled() ) 
	trace.log( title + ".getMaxTableNameLength(): " + len );
    return( len );
} // getMaxTableNameLength


/*
** Name: getMaxColumnNameLength
**
** Description:
**	What is the limit on column name length?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxColumnNameLength() 
    throws SQLException
{
    int len = DBMS_OI10_IDENT_LEN;    // OpenIngres, Ingres II etc., 

    if( std_catl_lvl < DBMS_SQL_LEVEL_OI10 )
	len = DBMS_ING64_IDENT_LEN;

    String max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_COL_NAME );

    if( max_len != null )
	try { len = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if ( trace.enabled() ) 
	trace.log( title + ".getMaxColumnNameLength(): " + len );
    return( len );
} // getMaxColumnNameLength


/*
** Name: getMaxCursorNameLength
**
** Description:
**	What is the maximum cursor name length?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxCursorNameLength() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getMaxCursorNameLength(): " + DBMS_OI10_IDENT_LEN );
    return( DBMS_OI10_IDENT_LEN );
} // getMaxCursorNameLength


/*
** Name: getMaxUserNameLength
**
** Description:
**	What is the maximum length of a user name?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxUserNameLength() 
    throws SQLException
{
    int len = DBMS_OI10_IDENT_LEN;    // OpenIngres, Ingres II etc. 

    if( std_catl_lvl < DBMS_SQL_LEVEL_OI10 )
	len = DBMS_ING64_IDENT_LEN;

    String max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_USR_NAME );

    if( max_len != null )
	try { len = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if( len < 0 ) len = 0;
    if ( trace.enabled() )
	trace.log( title + ".getMaxUserNameLength(): " + len );
    return( len );
} // getMaxUserNameLength


/*
** Name: getMaxBinaryLiteralLength
**
** Description:
**	Returns the number of hex characters in an inline binary
**	literal.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxBinaryLiteralLength() 
    throws SQLException
{
    int result = 0;

    String max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_BYT_LIT );

    if( max_len != null )
	try { result = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if( result < 0 ) result = 0;

    if ( trace.enabled() )  
	trace.log( title + ".getMaxBinaryLiteralLength(): " + result );
    return( result );
} // getMaxBinaryLiteralLength


/*
** Name: getMaxCharLiteralLength
**
** Description:
**	What is the maximum length for a character literal?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxCharLiteralLength() 
    throws SQLException
{
    int result = 0;

    String max_len = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_CHR_LIT );

    if( max_len != null )
	try { result = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};
    if( result < 0 ) result = 0;
    if ( trace.enabled() )  
	trace.log( title + ".getMaxCharLiteralLength(): " + result );
    return( result );
} // getMaxCharLiteralLength


/*
** Name: getMaxColumnsInTable
**
** Description:
**	What is the maximum number of columns in a table?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	31-Oct-02 (gordy)
**	    Replaced getMaxCol() call with DB caps request.
*/

public int 
getMaxColumnsInTable() 
    throws SQLException
{
    int		max_col = 0;
    String	str  = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_COLUMNS );

    if ( str != null )
        try { max_col = Integer.parseInt( str ); }
	catch( Exception ignore ) {};

    if ( trace.enabled() )  
	trace.log( title + ".getMaxColumnsInTable(): " + max_col );
    return( max_col );
} // getMaxColumnsInTable


/*
** Name: getMaxColumnsInIndex
**
** Description:
**	Returns the maximum number of columns allowed in an index?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
**	31-Oct-02 (gordy)
**	    Replaced getMaxCol() call with DB caps request.
*/

public int 
getMaxColumnsInIndex() 
    throws SQLException
{
    int		max_col = 0;
    String	str  = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_COLUMNS );

    if ( str != null )
        try { max_col = Integer.parseInt( str ); }
	catch( Exception ignore ) {};

    if ( trace.enabled() )
	trace.log( title + ".getMaxColumnsInIndex(): " + max_col );
    return( max_col );
} // getMaxColumnsInIndex


/*
** Name: getMaxTablesInSelect
**
** Description:
**	What is the maximum number of tables in a SELECT?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxTablesInSelect() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getMaxTablesInSelect(): " + DBMS_TBLS_INSEL );
    return( DBMS_TBLS_INSEL );
} // getMaxTablesInSelect


/*
** Name: getMaxColumnsInSelect
**
** Description:
**	What is the maximum number of columns in a 'SELECT" list?
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxColumnsInSelect() 
    throws SQLException
{
    int dont_know = 0;
    if ( trace.enabled() ) 
	trace.log( title + ".getMaxColumnsInSelect(): " + dont_know );
    return( dont_know );
} // getMaxColumnsInSelect


/*
** Name: getMaxColumnsInGroupBy
**
** Description:
**	Returns the maximum number of columns in "GROUP BY" clause.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxColumnsInGroupBy() 
    throws SQLException
{
    int dont_know = 0;
    if ( trace.enabled() ) 
	trace.log( title + ".getMaxColumnsInGroupBy(): " + dont_know );
    return( dont_know );
} // getMaxColumnsInGroupBy


/*
** Name: getMaxColumnsInOrderBy
**
** Description:
**	Returns the maximum number of columns in an "ORDER BY" clause.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	12-Nov-99 (rajus01)
**	    Implemented.
*/

public int 
getMaxColumnsInOrderBy() 
    throws SQLException
{
    int dont_know = 0;
    if ( trace.enabled() ) 
	trace.log( title + ".getMaxColumnsInOrderBy(): " + dont_know );
    return( dont_know );
} // getMaxColumnsInOrderBy


/*
** Name: getCatalogs
**
** Description:
**	Get the catalog names available in this database. The results
**	are ordered by catalog name.
**
**	Throws Exception since catalogs are NOT SUPPORTED.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSet 	// Single String column that is a catalog name.
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**      31-Jan-07 (weife01)
**          SIR 117451. Add support for catalog instead of throw SQLException.
*/

private static MDdesc cat_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
};

public ResultSet
getCatalogs()
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getCatalogs(): " + null );
         JdbcRSMD rsmd = new JdbcRSMD( (short)cat_desc.length, trace);
    for (int i = 0; i< cat_desc.length; i++)
    {
        if(upper)cat_desc[i].col_name = cat_desc[i].col_name.toUpperCase();
            rsmd.setColumnInfo(cat_desc[i].col_name, i + 1,
                               cat_desc[i].sql_type, cat_desc[i].dbms_type, 
                               cat_desc[i].length, (byte)0, (byte)0, false);
    }
    return( new RsltData( conn, rsmd, null ) );
} // getCatalogs

/*
** Name: getSchemas
**
** Description:
**	Get the schema names available in this database. The results
**	ordered by schema name.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSet 	// Single String column that is a schema name.
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	16-Feb-00 (rajus01)
**	    Added shema_desc. Return 0 rows for gateways.
**	19-Jul-02 (gordy)
**	    The table_schem column is declared to be varchar, but a DBMS
**	    result-set is used directly which makes it dependent on the
**	    host catalogs.  Added an XLAT result-set cover to ensure that
**	    the resulting column type is correct.
**	20-Mar-03 (gordy)
**	    Added catalog to the result-set for JDBC 3.0.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	13-Oct-03 (rajus01) Bug #111088
**          getSchemas() against IDMS returned empty resultSet.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
*/

private static  MDdesc schema_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_catalog"),
};

private static class
MDschemaRS
    extends RsltXlat
{

    public
    MDschemaRS( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
    } // MDschemaRS

    protected void
    load_data()
	throws SQLException
    {
	convert( 0, 1 );			// TABLE_SCHEM from schema_name
	setNull( 1 );				// TABLE_CATALOG
	return;
    } // load_data

} // class MDschemaRS

public ResultSet 
getSchemas() 
    throws SQLException
{
    return( getSchemas( null, null ) );
} // getSchemas

/*
** Name: getSchemas (overloaded)
**
** Description:
**      Gets schema names available in the database.
**
**      Only names matching the schemaPattern are returned. They are 
**      ordered by TABLE_SCHEM.
**      The catalog name is ignored. A null value for schemaPattern means 
**      that it will not be used to narrow down the search.
**
** Input:
**	catalog			// Catalog name
**	schemaPattern		// Schema name pattern
**
** Output:
**	None.
**
** Returns:
**	ResultSet 		// schema names
**
** History:
**	8-Apr-09 (rajus01) SIR 121238
**	    Implemented.
*/
public ResultSet 
getSchemas
(
    String catalog, 
    String schemaPattern
)
    throws SQLException
{
    String      qry_text;
    String 	whereClause = empty;
    String	orderByClause = empty;
    Statement	stmt = null;
    ResultSet	rs = null;
    JdbcRSMD	rsmd = new JdbcRSMD( (short)schema_desc.length, trace );

    if ( trace.enabled() )  trace.log( title + ".getSchemas()" );

    if ( std_catl_lvl >= DBMS_SQL_LEVEL_OI10 )
    {
        qry_text = "select schema_name from iischema";
	orderByClause = " order by schema_name";

	if( schemaPattern != null )
            whereClause += makePattern( schemaPattern, whereClause.length(), 
				"schema_name", false );

	qry_text = qry_text + whereClause + orderByClause; 
    }
    else
    {
	if( schemaPattern != null )
	    whereClause += makePattern( schemaPattern, whereClause.length(), 
				"table_owner", false );

        qry_text = "select table_owner from iitables" + whereClause +
	    " union select table_owner from iiviews" + whereClause;
    }

    for( int i = 0; i < schema_desc.length; i++ )
    {
	if ( upper ) 
	    schema_desc[i].col_name = schema_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( schema_desc[i].col_name, i+1,
			    schema_desc[i].sql_type, schema_desc[i].dbms_type,
			    schema_desc[i].length, (byte)0, (byte)0, false );
    }

    try
    {
	stmt = conn.jdbc.createStatement();
	rs = new MDschemaRS(conn, rsmd, (JdbcRslt)stmt.executeQuery(qry_text));
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getSchemas(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );

} // getSchemas

/*
** Name: getProcedures
**
** Description:
**	Gets a description of stored procedures available in a catalog.
**
**	Only procedure descriptions matching the schema and procedure
**	name criteria are returned. They are ordered by PROCEDURE_SCHEM,
**	and PROCEDURE_NAME.
**
**	A "" value for schemaPatten in the query will always return 0 rows
**	because all procedures have owner_name on it. Hence an input value
** 	of "" for schemaPattern is ignored. catalog name is ignored.
**
** Input:
**	catalog			// Catalog name
**	schemaPattern		// Schema name pattern
**	procedureNamePattern	// Procedure name pattern 
**
** Output:
**	None.
**
** Returns:
**	ResultSet 		// Procedure description
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	04-Feb-00 (rajus01)
**	    Used trim function to remove trailing blanks. Changed 
**	    column names to lower case.
**	21-Feb-02 (gordy)
**	    Ingres DBMS type now pre-determined.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
**	22-Apr-09 (rajus01) SIR 121238
**	    Added specific_name field. The specific_name filed uniquely
**	    identifies this procedure within its schema. The value of this
**	    filed is treated same as procedure_name field.
*/

private static  MDdesc proc_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "procedure_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "procedure_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "procedure_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "future_col1"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "future_col2"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "future_col3"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)128, "remarks"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2,  "procedure_type"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "specific_name"),
};

private static class
MDprocedureRS
    extends RsltXlat
{

    public
    MDprocedureRS( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
    } // MDprocedureRS

    protected void
    load_data()
	throws SQLException
    {
	setNull( 0 );		// PROCEDURE_CAT
	convert( 1, 1 );	// PROCEDURE_SCHEM from procedure_owner
	convert( 2, 2 );	// PROCEDURE_NAME from procedure_name
	setNull( 3 );		// FUTURE_COL1
	setNull( 4 );		// FUTURE_COL2
	setNull( 5 );		// FUTURE_COL3
	setNull( 6 );		// REMARKS
	setShort( 7, (short)procedureResultUnknown );	// PROCEDURE_TYPE
	convert( 8, 2 );	// SPECIFIC_NAME from procedure_name
	return;
    } // load_data

} // class MDprocedureRS

public ResultSet 
getProcedures
(
    String catalog, 
    String schemaPattern, 
    String procedureNamePattern 
) 
    throws SQLException
{
    String 	whereClause = empty;
    Statement	stmt = null;
    ResultSet 	rs = null;
    JdbcRSMD    rsmd = new JdbcRSMD( (short)proc_desc.length, trace );

    for( int i = 0; i < proc_desc.length; i++ )
    {
	if( upper ) proc_desc[i].col_name = proc_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( proc_desc[i].col_name, i+1, proc_desc[i].sql_type,
			    proc_desc[i].dbms_type, proc_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    String selectClause = "select distinct procedure_owner, procedure_name" +
			  " from iiprocedures";
    String orderByClause =  " order by procedure_owner, procedure_name";

    if ( conn.is_ingres )  whereClause = " where system_use ='U'";

    whereClause += makePattern( schemaPattern, whereClause.length(), 
				"procedure_owner", false );
    whereClause += makePattern( procedureNamePattern, whereClause.length(), 
				"procedure_name", false );

    String qry_txt = selectClause + whereClause + orderByClause;

    if ( trace.enabled() )  
	trace.log( title + ".getProcedures( " + catalog + ", " + 
		   schemaPattern + ", " + procedureNamePattern + " )" );

    try
    {
	stmt = conn.jdbc.createStatement();
	rs = new MDprocedureRS( conn, rsmd, 
				(JdbcRslt)stmt.executeQuery(qry_txt) );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log(title + ".getProcedures(): failed!");
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );
} // getProcedures


/*
** Name: getProcedureColumns
**
** Description:
**	Get a description of  catalog's stored procedure parameters
**	and result columns.
**	Only descriptions matching the schema, procedure and parameter
**	name criteria are returned. They are ordered by PROCEDURE_SCHEM
**	and PROCEDURE_NAME. 
**	A "" value for schemaPatten in the query will always return 0 rows
**	because all procedures have owner_name on it. Hence an input value
** 	of "" for schemaPattern is ignored. catalog name is ignored.
**
** Input:
**	catalog			// Catalog name
**	schemaPattern		// Schema name pattern
**	procedureNamePattern	// Procedure name pattern 
**	columnNamePattern	// Column name pattern
**
** Output:
**	None.
**
** Returns:
**	ResultSet 		// Stored procedure parameter or
**				// column description.
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	04-Feb-00 (rajus01)
**	    Used trim function to remove trailing blanks. Changed 
**	    column names to lower case.
**	21-Feb-02 (gordy)
**	    Ingres DBMS type now pre-determined.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	26-Jul-07 (rajus01) Bug #114211
**	    X-integrate bug fix for 114211 from ingres26 codeline.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
**	22-Apr-09 (rajus01) SIR 121238
**	    Added column_def, sql_data_type, sql_datetime_sub,
**	    char_octet_length, ordinal_position, is_nullable and
**	    specific_name fields.
*/

private static  MDdesc procCol_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "procedure_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "procedure_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "procedure_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "column_type"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "data_type"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "precision"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "length"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "scale"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "radix"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "nullable"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "remarks"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_def"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "sql_data_type"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "sql_datetime_sub"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "char_octet_length"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "ordinal_position"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)5,  "is_nullable"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "specific_name"),
};

private static class
MDprocedureColRS
    extends RsltXlat
{

    public
    MDprocedureColRS( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
    } // MDprocedureColRS

    protected void
    load_data()
	throws SQLException
    {
	setNull( 0 );		// PROCEDURE_CAT
	convert( 1, 1 );	// PROCEDURE_SCHEM from procedure_owner
	convert( 2, 2 );	// PROCEDURE_NAME from procedure_name
	convert( 3, 3 );	// COLUMN_NAME from param_name
	setShort( 4, (short)procedureColumnUnknown );	// COLUMN_TYPE

	// DATA_TYPE from param_datatype_code
	short s =  (short)Math.abs( dbmsRS.getInt(4) );
	int col_len = dbmsRS.getInt( 7 );
	int javaType = convToJavaType( s, col_len );
	setShort( 5, (short)javaType );

	convert( 6, 5 );	// TYPE_NAME from param_datatype
	convert( 7, 6 );	// PRECISION from  param_scale
	convert( 8, 7 );	// LENGTH from param_length
	convert( 9, 8 );	// SCALE from  param_scale
	setShort( 10, (short)10 );	// RADIX 

	// NULLABLE from param_nulls
	String param_null = dbmsRS.getString(9);

	if ( param_null == null  ||  param_null.length() < 1 ) 
	{
	    setString( 18, empty );    // IS_NULLABLE
	    setShort( 11, (short)procedureNullableUnknown );
	}
	else if ( param_null.equals("N") )
	{ 
	    setString( 18, NO );	// IS_NULLABLE
	    setShort( 11, (short)procedureNoNulls );
	}
	else
	{
	    setShort( 11, (short)procedureNullable );
	    setString( 18, YES );	// IS_NULLABLE
	}
    
	setNull( 12 );		// REMARKS
	setNull( 13 );		// COLUMN_DEF
	setNull( 14 );		// SQL_DATA_TYPE
	setNull( 15 );		// SQL_DATETIME_SUB
	convert( 16, 7 );	// CHAR_OCTET_LENGTH from param_length
	convert( 17, 10 );	// ORDINAL_POSITION from param_sequence
	convert( 19, 2 );	// SPECIFIC_NAME from  procedure_name

	return;
    } // load_data

} //MDprocedureColRS

public ResultSet 
getProcedureColumns
( 
    String catalog,
    String schemaPattern,
    String procedureNamePattern, 
    String columnNamePattern
) 
    throws SQLException
{
    String	selectClause = empty;
    String	orderByClause = empty;
    String	whereClause = empty;
    String	columnName;
    Statement	stmt = null;
    ResultSet 	rs = null;

    JdbcRSMD    rsmd = new JdbcRSMD( (short)procCol_desc.length, trace );

    for( int i = 0; i < procCol_desc.length; i++ )
    {
	if( upper ) 
	    procCol_desc[i].col_name = procCol_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( procCol_desc[i].col_name, i+1,
			    procCol_desc[i].sql_type,
			    procCol_desc[i].dbms_type, procCol_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    selectClause = "select distinct procedure_owner," + 
		   " procedure_name, param_name, param_datatype_code," +
		   " param_datatype, param_scale, param_length," +
		   " param_scale, param_nulls, param_sequence" +
		   " from iiproc_params";

    orderByClause = " order by procedure_owner, procedure_name, param_sequence";

    if ( ! conn.is_ingres )
    {
        selectClause = "select distinct proc_owner," + 
		   " proc_name, param_name, param_datatype_code," +
		   " param_datatype, param_scale, param_length," +
		   " param_scale, param_null, param_sequence" +
		   " from iigwprocparams";
        orderByClause = " order by proc_owner, proc_name, param_sequence";
    }


    if ( trace.enabled() )  
	trace.log( title + ".getProcedureColumns( "  + catalog + ", " + 
		   schemaPattern + ", " + procedureNamePattern + ", " + 
		   columnNamePattern + " )" );

    columnName = "procedure_owner";
    if ( ! conn.is_ingres )  columnName = "proc_owner";

    whereClause += makePattern( schemaPattern, whereClause.length(), 
				columnName, false );

    columnName = "procedure_name";
    if ( ! conn.is_ingres )  columnName = "proc_name";

    whereClause += makePattern( procedureNamePattern, whereClause.length(), 
				columnName, false );

    whereClause += makePattern( columnNamePattern, whereClause.length(), 
				"param_name", false );

    String qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	stmt = conn.jdbc.createStatement();
	rs = new MDprocedureColRS( conn, rsmd, 
				   (JdbcRslt)stmt.executeQuery(qry_txt) );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getProcedureColumns(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace);

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );
} // getProcedureColumns


/*
** Name: getTables
**
** Description:
**	Get a description of tables available in a catalog. 
**
**	Only table descriptions matching the catalog, schema, table
**	name and type criteria are returned. They are ordered by
**	TABLE_TYPE, TABLE_SCHEM and TABLE_NAME.
**
** Input:
**	catalog			// Catalog name
**	schemaPattern		// Schema name pattern
**	tableNamePattern	// Table name pattern
**	types			// Table types
**
** Output:
**	None.
**
** Returns:
**	ResultSet		// Table description 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	04-Feb-00 (rajus01)
**	    Used trim function to remove trailing blanks. Changed 
**	    column names to lower case.
**	20-Mar-03 (gordy)
**	    Added type columns to the result-set for JDBC 3.0.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.  Clean
**	    up tracing of array parameter.  
*/

private static final	MDdesc	tables_desc[] =
{
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "table_cat" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "table_schem" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "table_name" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)16,  "table_type"),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)256, "remarks"),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "type_cat" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "type_schem" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "type_name" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "self_referencing_col_name" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "ref_generation" ),
};

private static class
MDtableRS
    extends RsltXlat
{

    public
    MDtableRS( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
    } // MDtableRS

    protected void
    load_data()
	throws SQLException
    {
	String type = dbmsRS.getString( 3 );
	String system = dbmsRS.getString( 4 );

	if ( type == null  ||  type.length() < 1 )  type = space;
	if ( system == null  ||  system.length() < 1 )  system = space;

	setNull( 0 );		// Catalog
	convert( 1, 1 );	// TABLE_SCHEM from table_owner
	convert( 2, 2 );	// TABLE_NAME from table_name


	if ( system.charAt( 0 ) == 'S' )
	    setString( 3, "SYSTEM TABLE" );
	else
	    switch( type.charAt( 0 ) )
	    {
	    case 'T' :	
		setString( 3, "TABLE" );
		break;

	    case 'V' :	
		setString( 3, "VIEW" );
		break;

	    default :	
		setString( 3, "UNKNOWN" );
		break;
	    }

	setNull( 4 );		// Remarks
	setNull( 5 );		// Type catalog
	setNull( 6 );		// Type schema
	setNull( 7 );		// Type name
	setNull( 8 );		// Self referencing column name
	setNull( 9 );		// Reference generation
	return;
    } // load_data

} // class MDtableRS

public ResultSet 
getTables
(
    String catalog, 
    String schemaPattern,
    String tableNamePattern, 
    String types[]
) 
    throws SQLException
{

    String	selectClause = "select table_owner, table_name, table_type," +
				" system_use from iitables";
    String	whereClause = empty;
    String	orderByClause = " order by table_type, table_owner, table_name";
    String	systemTables = " system_use = 'S' and table_type in ('T','V')";
    String	clauses[] = new String[3];
    int		clause_cnt = 0;
    boolean	tables = false, views = false, system = false;
    JdbcRSMD    rsmd = new JdbcRSMD( (short)tables_desc.length, trace );
    
    if ( trace.enabled() )  
	trace.log( title + ".getTables( " + catalog + ", " + schemaPattern + 
		   ", " + tableNamePattern + ", " + 
		   (types == null ? "null" : "[" + types.length + "]") + " )" );

    for( int i = 0; i < tables_desc.length; i++ )
    {
	if( upper ) 
	    tables_desc[i].col_name = tables_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( tables_desc[i].col_name, i+1,
			    tables_desc[i].sql_type, tables_desc[i].dbms_type,
			    tables_desc[i].length, (byte)0, (byte)0, false );
    }

    whereClause += makePattern( schemaPattern, whereClause.length(), 
				"table_owner", false );

    whereClause += makePattern( tableNamePattern, whereClause.length(), 
				"table_name", false );

    if ( types == null )
	tables = views = system = true;
    else
	for( int i = 0; i < types.length; i++ )
	{
	    if ( types[i].equals( "TABLE" ) )  tables = true;
	    if ( types[i].equals( "VIEW" ) )  views = true;
	    if ( types[i].equals( "SYSTEM TABLE" ) )  system = true;
	}
				
    if ( ! tables  &&  ! views  &&  ! system )
	return( new RsltData( conn, rsmd, null ) );
    else  if ( ! (tables  ||  views) )
	clauses[ clause_cnt++ ] = systemTables;
    else
    {
 	clauses[ clause_cnt ] = " system_use <> 'S' and" +
				" table_name not like 'ii%' and table_type";
	    
	if ( tables  &&  views )
	    clauses[ clause_cnt ] += " in ('T','V')";
	else  if ( tables )
	    clauses[ clause_cnt ] += " = 'T'";
	else
	    clauses[ clause_cnt ] += " = 'V'";

	if ( system )
	    clauses[ clause_cnt ] = " ((" + clauses[ clause_cnt ] + 
				    ") or (" + systemTables + "))";
	clause_cnt++;
    }

    if ( clause_cnt > 0 )
    {
	StringBuilder str = new StringBuilder();

        if ( whereClause.length() == 0 )
            whereClause = " where";
        else
            str.append( " and" );

	str.append( clauses[ 0 ] );

	for( int i = 1; i < clause_cnt; i++ )
	{
	    str.append( " and" );
	    str.append( clauses[ i ] );
	}

	whereClause += str.toString();
    }

    String	qry_txt = selectClause + whereClause + orderByClause;
    Statement	stmt = null;
    ResultSet	rs = null;

    try
    {
	stmt = conn.jdbc.createStatement();
	rs = new MDtableRS( conn, rsmd, (JdbcRslt)stmt.executeQuery(qry_txt) );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getTables(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );
} // getTables


/*
** Name: getTableTypes
**
** Description:
**	Get the table types available in this database. The results
**	are ordered by table type.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSet	// single String column that is a table type. 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
*/

private static  MDdesc tabtyp_desc =
    new MDdesc(Types.VARCHAR, VARCHAR, (short)20, "table_type");

private static final SqlData	tableTypeValues[][] =
{ 
    { new SqlString( "TABLE" ) }, 
    { new SqlString( "VIEW" ) }, 
    { new SqlString( "SYSTEM TABLE" ) },
};

public ResultSet 
getTableTypes() 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getTableTypes()" );

    JdbcRSMD    rsmd = new JdbcRSMD( (short)1, trace );

    if ( upper ) tabtyp_desc.col_name = tabtyp_desc.col_name.toUpperCase();
    rsmd.setColumnInfo( tabtyp_desc.col_name, 1, tabtyp_desc.sql_type,
			tabtyp_desc.dbms_type, tabtyp_desc.length,
			(byte)0, (byte)0, false );

    return( new RsltData( conn, rsmd, tableTypeValues ) );
} // getTableTypes


/*
** Name: getTablePrivileges
**
** Description:
**	Get a description of the access rights for each table available
**	in a cataog. 
**
**	Only privileges matching the schema and table name
**	criteria are returned. They are ordered by TABLE_SCHEM, TABLE_NAME
**	and PRIVILEGE.
**
** Input:
**	catalog			// Catalog name
**	schemaPattern		// Schema name pattern
**	tableNamePattern	// Table name pattern 
**
** Output:
**	None.
**
** Returns:
**	ResultSet 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	04-Feb-00 (rajus01)
**	    Use trim function to remove trailing blanks. Changed 
**	    column names to lower case. Use iipermits catalog.
**	16-Feb-00 (rajus01)
**	    Gateways return 0 rows.
**	21-Feb-02 (gordy)
**	    Ingres DBMS type now pre-determined.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
**	27-Apr-09 (rajus01) SIR 121238
**	    Added text_segment (privilege is derived from text_segment) to
**	    order by clause.
*/

private static  MDdesc tabpriv_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "grantor"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "grantee"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "privilege"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "is_grantable")
};

private static class
MDgetTablePrivRS
    extends RsltXlat
{

    public
    MDgetTablePrivRS( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
    } // MDgetTablePrivRS

    protected void
    load_data()
	throws SQLException
    {

	setNull( 0 );		// TABLE_CAT
	convert( 1, 1 );	// TABLE_SCHEM from object_owner
	convert( 2, 2 );	// TABLE_NAME from object_name
	convert( 3, 3 );	// GRANTOR from permit_grantor
	convert( 4, 4 );	// GRANTEE from permit_user

	String s = dbmsRS.getString(5);
	if ( s == null )
	    setNull( 5 );
	else
	    setString( 5, s.substring(6, s.lastIndexOf("on")).toUpperCase() );

	setNull( 6 );		// IS_GRANTABLE
	return;
    } // load_data

} // MDgetTablePrivRS

public ResultSet 
getTablePrivileges
(
    String catalog, 
    String schemaPattern,
    String tableNamePattern
) 
    throws SQLException
{
    Statement	stmt = null;
    ResultSet 	rs = null;
    JdbcRSMD    rsmd = new JdbcRSMD( (short)tabpriv_desc.length, trace );

    String selectClause = " select object_owner, object_name," +
			  " permit_grantor, permit_user," +
			  " text_segment" +
			  " from iipermits";
    String whereClause = " where text_sequence = 1";
    String orderByClause = " order by object_owner, object_name, text_segment";

    whereClause += makePattern( schemaPattern, whereClause.length(), 
				"object_owner", false );

    whereClause += makePattern( tableNamePattern, whereClause.length(), 
				"object_name", false );

    if ( trace.enabled() )  
	trace.log( title + ".getTablePrivileges( " + catalog + ", " + 
		   schemaPattern + ", " + tableNamePattern + " )" );
    
    for( int i = 0; i < tabpriv_desc.length; i++ )
    {
	if( upper ) 
	    tabpriv_desc[i].col_name = tabpriv_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( tabpriv_desc[i].col_name, i + 1,
			    tabpriv_desc[i].sql_type, tabpriv_desc[i].dbms_type,
			    tabpriv_desc[i].length, (byte)0, (byte)0, false );
    }

    if ( ! conn.is_ingres )
    {
	/*
	** return 0 rows for gateways.
	*/
	return( new RsltData( conn, rsmd, null  ) ); 
    }

    String   qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	stmt = conn.jdbc.createStatement();
	rs = new MDgetTablePrivRS( conn, rsmd, 
				   (JdbcRslt)stmt.executeQuery(qry_txt) );

    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getTablePrivileges(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );
} // getTablePrivileges


/*
** Name: getSuperTables
**
** Description:
**	Get super table information.
**
** Input:
**	catalog			Catalog name.
**	schemaPattern		Schema name pattern.
**	tableNamePattern	Table name pattern.
**
** Output:
**	None.
**
** Returns:
**	ResultSet		Super tables
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

private static  MDdesc tabsup_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "supertable_name"),
};

public ResultSet 
getSuperTables
( 
    String	catalog, 
    String	schemaPattern, 
    String	tableNamePattern
) 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getSuperTables('" +
	catalog + "','" + schemaPattern + "','" + tableNamePattern + "')" );

    JdbcRSMD    rsmd = new JdbcRSMD( (short)tabsup_desc.length, trace );

    for( int i = 0; i < super_desc.length; i++ )
    {
	if ( upper ) 
	    tabsup_desc[i].col_name = tabsup_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( tabsup_desc[i].col_name, i + 1, 
			    tabsup_desc[i].sql_type, tabsup_desc[i].dbms_type, 
			    tabsup_desc[i].length, (byte)0, (byte)0, false );
    }

    return( new RsltData( conn, rsmd, null ) );
} // getSuperTables


/*
** Name: getColumns
**
** Description:
**	Get a description of table columns available in a catalog.
**
** 	Only column descriptions matching the catalog, schema, table 
**	and column name criteria are returned. They are ordered by
**	TABLE_SCHEM, TABLE_NAME and ORDINAL_POSITION.
**
** Input:
**	catalog			// Catalog name
**	schemaPattern		// Schema name pattern
**	tableNamePattern	// Table name pattern 
**	columnNamePattern	// Column name pattern
**
** Output:
**	None.
**
** Returns:
**	ResultSet 	// Column Description
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-12-99 (rajus01)
**	    Implemented.
**	22-Dec-99 (rajus01)
**	    Fixed DBMS column indexes.
**	15-Feb-02 (gordy)
**	    Make sure storage class matches descriptor.
**	20-Mar-03 (gordy)
**	    Added type columns to the result-set for JDBC 3.0.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	01-Oct-03 (rajus01) Bug #111029
**          IDMS restricts LIKE clause usage in queries against
**          iicolumns catalog.
**	15-Oct-03 (rajus01) Bug #111119 
**	     ColumnName is not used in the query forming logic for IDMS.
**           Converted "%" to null in case of IDMS as pattern matching is
**	     not supported( Refer Bug #111029 ) against iicolumns catalog.
**	29-Jan-04 (fei wei) Bug # 111723 Startrak # EDJDBC94
**	     Convert the column_datatype into a smallint for EDBC/gateway
**	     servers.
**	13-Jul-06 (lunbr01)  Bug 115453
**	    DatabaseMetaData.getColumns() returns NO_DATA when table name
**	    is 32 characters long (actually max length of target dbms type).  
**	    Also erroneously matches on similarly named tables with embedded 
**	    spaces (eg "t1" and "t1 a").  Put trim() back in where possible,
**	    which then requires no pattern change.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	14-Feb-08 (rajus01) Bug 119919
**	    getColumns() returns 0 for column_length, and column_scale 
**	    for MONEY datatype. The Ingres catalogs return incorrect
**	    values in this case. 
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
**	22-Apr-09 (rajus01) SIR 121238
**	    Added is_autoincrement field to the result-set. Removed 
**	    "column_sequence" from orderByClause;
*/

private static  MDdesc getc_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_name"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "data_type"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "column_size"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "buffer_length"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "decimal_digits"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "num_prec_radix"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "nullable"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)256,"remarks"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_def"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "sql_data_type"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "sql_datetime_sub"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "char_octet_length"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "ordinal_position"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "is_nullable"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "scope_catlog"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "scope_schema"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "scope_table"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "source_data_type" ),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)5, "is_autoincrement"),
};

private static class
MDgetColRS
    extends RsltXlat
{

    public
    MDgetColRS( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
    } // MDgetColRS

    protected void
    load_data()
	throws SQLException
    {
	setNull( 0 );		// TABLE_CAT
	convert( 1, 1 );	// TABLE_SCHEM from table_owner
	convert( 2, 2 );	// TABLE_NAME from table_name
	convert( 3, 3 );	// COLUMN_NAME from column_name

	// DATA_TYPE from column_ingdatatype
	short s = dbmsRS.getShort(4);

	if ( s != -1 )		// Have Ingres type
	    s = (short)Math.abs( (int)s );
	else
	{
	     /*
	     ** Attempt to map type string to Ingres type.
	     */
	     String dtype = dbmsRS.getString(5).toLowerCase();
	     s = (short)IdMap.get( dtype, dataTypeMap );
	}
   
	int col_len = dbmsRS.getInt( 6 );	// column_length
	int javaType = convToJavaType( s, col_len );
	setInt( 4, javaType );

	convert( 5, 5 );		// TYPE_NAME from column_datatype
	
	/*
	** COLUMN_SIZE
	** use column_length for CHAR_OCTET_LENGTH also (Not sure!!!).
	*/
	int size = colSize(s, javaType );

	if( size < 0 ) // DECIMAL, CHAR, VARCHAR, VARBINARY
	{
	    convert( 6, 6 );
	    convert( 15, 6 );
	}
	else
	{
	    setInt( 6, size );
	    setInt( 15, size );
	}

	setNull( 7 );		// BUFFER_LENGTH
        if( s == DBMS_TYPE_MONEY )
	    setInt( 8, (int) DBMS_MONEY_SCALE );
        else
	    convert( 8, 7 );	// DECIMAL_DIGITS from column_scale

	convert( 16, 9 );	// ORDINAL_POSITION from column_sequence
	// NUM_PREC_RADIX 
	setInt( 9, (int)10 );

	// NULLABLE from column_nulls
	String nullable = dbmsRS.getString(8);

	if ( nullable == null  ||  nullable.length() < 1 ) 
	{
	    setInt( 10, columnNullableUnknown );
	    setString( 17, empty );
	}
	else if ( nullable.equals("N") )
	{
	    setInt( 10, columnNoNulls );
	    setString( 17, NO );
	}
	else
	{
	    setInt( 10, columnNullable );
	    setString( 17, YES );
	}
    
	setNull( 11 );		// REMARKS
	setNull( 12 );		// COLUMN_DEF 
	setNull( 13 );		// SQL_DATA_TYPE
	setNull( 14 );		// SQL_DATETIME_SUB
	convert( 16, 9 );	// ORDINAL_POSITION from column_sequence
	setNull( 18 );		// SCOPE_CATLOG
	setNull( 19 );		// SCOPE_SCHEMA
	setNull( 20 );		// SCOPE_TABLE
	setNull( 21 );		// SOURCE_DATA_TYPE
	setString( 22, empty ); // IS_AUTOINCREMENT
	return;
    } // load_data

} // MDgetColRS

public ResultSet 
getColumns
(
    String catalog, 
    String schemaPattern,
    String tableNamePattern, 
    String columnNamePattern
)
    throws SQLException
{
    Statement	stmt = null;
    ResultSet	rs = null;
    JdbcRSMD	rsmd = new JdbcRSMD( (short)getc_desc.length, trace );
    String	dbmsType = conn.dbCaps.getDbCap( DBMS_DBCAP_DBMS_TYPE );
    boolean	likeNotSupported = false;

    for( int i = 0; i < getc_desc.length; i++ )
    {
	if( upper ) getc_desc[i].col_name = getc_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( getc_desc[i].col_name, i+1, getc_desc[i].sql_type,
			    getc_desc[i].dbms_type, getc_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    String selectClause = "select table_owner, table_name, column_name," +
			  " column_ingdatatype," +
			  " column_datatype," +
			  " column_length, column_scale, column_nulls," +
			  " column_sequence from iicolumns";
    String whereClause = empty;
    String orderByClause = " order by table_owner, table_name";

    /*
    ** IDMS iicolumns is a table procedure and does not support LIKE clause.
    */
    if ( dbmsType != null  &&  dbmsType.equals( "IDMS" ) ) 
	likeNotSupported = true;

    whereClause += makePattern( schemaPattern, whereClause.length(), 
				"table_owner", likeNotSupported );

    whereClause += makePattern( tableNamePattern, whereClause.length(), 
				"table_name", likeNotSupported );

    whereClause += makePattern( columnNamePattern, whereClause.length(), 
				"column_name", likeNotSupported );

    String qry_txt = selectClause + whereClause + orderByClause;

    if ( trace.enabled() )  
	trace.log( title + ".getColumns( " + catalog + ", " + schemaPattern + 
		   ", " + tableNamePattern + ", " + columnNamePattern + " )" );

    try
    {
	stmt = conn.jdbc.createStatement();
	rs = new MDgetColRS( conn, rsmd, (JdbcRslt)stmt.executeQuery(qry_txt) );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getColumns(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );
} // getColumns


/*
** Name: getColumnPrivileges
**
** Description:
**	Get a description of the access rights for a table's columns. 
**
**	Only privileges matching the column name criteria are returned.
**	They are ordered by COLUMN_NAME and PRIVILEGE.
**
** Input:
**	catalog			// Catalog name
**	schema			// Schema name
**	table			// table name 
**	columnNamePattern	// Column name pattern
**
** Output:
**	None.
**
** Returns:
**	ResultSet 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	04-Feb-00 (rajus01)
**	    Used trim function to remove trailing blanks. Changed 
**	    column names to lower case.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
*/

private static  MDdesc getColPriv_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "grantor"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "grantee"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "privilege"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "is_grantable")
};

private static class
MDgetColPrivRS
    extends RsltXlat
{

    public
    MDgetColPrivRS( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );

    } // MDgetColPrivRS

    protected void
    load_data()
	throws SQLException
    {
	setNull( 0 );		// TABLE_CAT
	convert( 1, 1 );	// TABLE_SCHEM from table_owner
	convert( 2, 2 );	// TABLE_NAME from table_name
	convert( 3, 3 );	// COLUMN_NAME from column_name
	convert( 4, 1 );	// GRANTOR from table_owner
	setNull( 5 );		// GRANTEE
	setNull( 6 );		// PRIVILEGE
	setNull( 7 );		// IS_GRANTABLE
	return;
    } // load_data

} // MDgetColPrivRS


public ResultSet 
getColumnPrivileges
(
    String catalog, 
    String schema,
    String table, 
    String columnNamePattern
) 
    throws SQLException
{
    boolean	likeNotSupported = false;
    String	dbmsType = conn.dbCaps.getDbCap( DBMS_DBCAP_DBMS_TYPE );

    String	selectClause = 
    			  "select table_owner, table_name, column_name" +
			  " from iicolumns";
    String	whereClause= " where table_name = '" + table + "'";
    String	orderByClause = " order by column_name";
    JdbcRSMD    rsmd = new JdbcRSMD( (short)getColPriv_desc.length, trace );
    Statement	stmt = null;
    ResultSet	rs = null;

    /*
    ** IDMS iicolumns is a table procedure and does not support LIKE clause.
    */
    if ( dbmsType != null  &&  dbmsType.equals( "IDMS" ) ) 
	likeNotSupported = true;

    whereClause += makePattern( columnNamePattern, whereClause.length(), 
				"column_name", likeNotSupported );

    if ( trace.enabled() )  
	trace.log( title + ".getColumnPrivileges( " + catalog + ", " + 
		   schema + ", " + table + ", " + columnNamePattern + " )" );

    for( int i = 0; i < getColPriv_desc.length; i++ )
    {
	if ( upper ) 
	    getColPriv_desc[i].col_name = 
			getColPriv_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( getColPriv_desc[i].col_name, i+1,
			    getColPriv_desc[i].sql_type,
			    getColPriv_desc[i].dbms_type,
			    getColPriv_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    String   qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	stmt = conn.jdbc.createStatement();
	rs = new MDgetColPrivRS( conn, rsmd, 
				 (JdbcRslt)stmt.executeQuery( qry_txt ) );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".getColumnPrivileges(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );
} // getColumnPrivileges


/*
** Name: getBestRowIdentifier
**
** Description:
**	Get a description of a table's optimal set of columns that
**	uniquely identifies a row. 
**
** Input:
**	catalog		// Catalog name 
**	schema 		// Schema name
**	table		// Table name
**	scope		// Scope of the result ( bestRowTemporary,
**			// bestRowTransaction, bestRowSession )
**	nullable	// Includes columns nullable?
**
** Output:
**	None.
**
** Returns:
**	ResultSet 	// Column Description 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	17-Dec-99 (rajus01)
**	    Implemented.
**	04-Feb-00 (rajus01)
**	    Used trim function to remove trailing blanks. Changed 
**	    column names to lower case.
**	15-Feb-02 (gordy)
**	    Make sure storage class matches descriptor.
**	21-Feb-02 (gordy)
**	    Ingres DBMS type now pre-determined.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
*/

private static  MDdesc bestRow_desc[] =
{
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "scope"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "data_type"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "column_size"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "buffer_length"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "decimal_digits"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "pseudo_column")
};

private static class
MDgetBestRowRS
    extends RsltXlat
{
    private boolean pkyOrIdx;

    public
    MDgetBestRowRS( DrvConn conn, JdbcRSMD rsmd, 
		    JdbcRslt dbmsRS, boolean qtype )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
	pkyOrIdx = qtype;
    } // MDgetBestRowRS

    protected void
    load_data()
	throws SQLException
    {
	setShort( 0, (short)bestRowTemporary );		// SCOPE

	if ( ! pkyOrIdx  )
	{
	    convert( 1, 1 );				// COLUMN_NAME (tid)
	    setShort( 2, (short)Types.INTEGER );	// DATA_TYPE
	    setString( 3, "TID" );			// TYPE_NAME
	    setInt( 4, (int)10 );			// COLUMN_SIZE
	    setNull( 5 );				// BUFFER_LENGTH
	    setShort( 7, (short)bestRowPseudo );	// PSEUDO_COLUMN
	}
	else
	{
	    convert( 1, 1 );		// COLUMN_NAME from column_name

	    // DATA_TYPE from column_ingdatatype
	    short s =  (short) Math.abs( dbmsRS.getInt(2) );
	    int col_len = dbmsRS.getInt( 4 );   // column_length
	    int javaType = convToJavaType( s, col_len );
	    setShort( 2, (short)javaType );

	    convert( 3, 3 );	// TYPE_NAME from column_datatype
	    // COLUMN_SIZE
	    int size = colSize(s, javaType ); 
	    if( size < 0 )		// DECIMAL, CHAR, VARCHAR, VARBINARY
		convert( 4, 4 );
	    else
		setInt( 4, size );

	    convert( 5, 5 );		// DECIMAL_DIGITS from column_scale
	    setShort( 7, (short)bestRowNotPseudo );	// NOT a PSEUDO_COLUMN
	}

	// DECIMAL_DIGITS
	setShort( 6, (short)0 );
	return;

    } //load_data

} // MDgetBestRowRS

public ResultSet 
getBestRowIdentifier
(
    String catalog, 
    String schema,
    String table, 
    int scope, 
    boolean nullable
) 
    throws SQLException
{
    String		qry_txt = null;
    String 		selectClause = null; 
    String 		whereClause = null;
    String 		iname = null;
    String 		iowner = null;
    String 		bname = null;
    String 		bowner = null;
    JdbcRSMD    	rsmd = new JdbcRSMD((short)bestRow_desc.length, trace);
    ResultSet   	rs = null;
    PreparedStatement	stmt = null;

    for( int i = 0; i < bestRow_desc.length; i++ )
    {
	if ( upper )
	    bestRow_desc[i].col_name = bestRow_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( bestRow_desc[i].col_name, i+1,
			    bestRow_desc[i].sql_type,
			    bestRow_desc[i].dbms_type,
			    bestRow_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    if ( trace.enabled() )  
	trace.log( title + ".getBestRowIdentifier( " + catalog + ", " + schema + 
		   ", " + table + ", " + scope + ", " + nullable + " )" );

    /*
    ** BEST ROW SCOPE - very temporary, while using row
    */
    if ( scope == bestRowTemporary  &&  conn.is_ingres )
    {
	selectClause = "select distinct 'tid' from iitables";
	whereClause = " where table_type = 'T'";
	if ( schema != null )  whereClause += " and table_owner = ?";
	if ( table != null )  whereClause += " and table_name = ?";
	qry_txt = selectClause + whereClause;
        
	try
        {
	    stmt = conn.jdbc.prepareStatement(qry_txt);
	    int i = 1;

	    if ( schema != null )  stmt.setObject( i++, schema, param_type );
	    if ( table != null )   stmt.setObject( i++, table, param_type );

	    rs = new MDgetBestRowRS( conn, rsmd, 
				     (JdbcRslt)stmt.executeQuery(), false );

        }
        catch( SQLException ex )
        {
	    if ( trace.enabled() )
		trace.log( title + ".getBestRowIdentifier(): failed (tid)!" ) ;
	    if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	    if ( stmt != null )
		try { stmt.close(); } catch( SQLException ignore ) {}

	    throw ex;
        }

        return( rs );
    }

    /*
    ** Use unique primary key if there is any.
    */

    selectClause = "select distinct c.column_name," +
  	           " c.column_ingdatatype,c.column_datatype," +
		   " c.column_length," +
		   " c.column_scale from iitables t, iicolumns c";
    whereClause = " where t.table_name = c.table_name and" +
		      " t.table_owner = c.table_owner and" +
		      " t.unique_rule = 'U' and c.key_sequence <> 0"; 
    if( ! nullable )
	whereClause += " and c.column_nulls <> 'Y'"; 
    if( schema != null )
	whereClause += " and c.table_owner = ?";
    if( table != null )
	whereClause += " and c.table_name = ?";

    qry_txt = selectClause + whereClause;

    try
    {
	stmt = conn.jdbc.prepareStatement(qry_txt);
	int i = 1;
	if ( schema != null )  stmt.setObject( i++, schema, param_type );
	if ( table != null )   stmt.setObject( i++, table, param_type );
	rs = new MDgetBestRowRS( conn, rsmd, 
				 (JdbcRslt)stmt.executeQuery(), true );
	if ( rs.next() )
	{
	    try { rs.close(); } catch( SQLException ignore ) {}
	    rs = null;

	    rs = new MDgetBestRowRS( conn, rsmd, 
				     (JdbcRslt)stmt.executeQuery(), true );
	    return( rs );	// unique primary key
	}

	try { rs.close(); } catch( SQLException ignore ) {}
	rs = null;
	try { stmt.close(); } catch( SQLException ignore ) {}
	stmt = null;
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".getBestRowIdentifier(): failed! (primary)" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( rs != null )
	    try { rs.close(); } catch( SQLException ignore ) {}

	if ( stmt != null ) 
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    /*
    ** Use secondary indexes.
    */
    selectClause = "select distinct i.index_name," +
		   "i.index_owner, i.base_name," +
	           "i.base_owner from iiindexes i";
    whereClause = " where i.unique_rule = 'U'";
    if ( schema != null )  whereClause += " and i.base_owner = ?";
    if ( table != null )   whereClause += " and i.base_name = ?";

    qry_txt = selectClause + whereClause;

    try
    {
	stmt = conn.jdbc.prepareStatement(qry_txt);
	int i = 1;
	if ( schema != null )  stmt.setObject( i++, schema, param_type );
	if ( table != null )   stmt.setObject( i++, table, param_type );
	rs = (JdbcRslt)stmt.executeQuery();

    }
    catch( SQLException ex )
    {
	if ( trace.enabled() ) 
	    trace.log( title + ".getBestRowIdentifier(): failed (idx1)!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    if ( rs.next() )
    {
	iname = rs.getString(1);
	iowner = rs.getString(2);
	bname = rs.getString(3);
	bowner = rs.getString(4);
    }

    try { rs.close(); } catch( SQLException ignore ) {}
    rs = null;
    try { stmt.close(); } catch( SQLException ignore ) {}
    stmt = null;

    selectClause = "select distinct c.column_name," +
  	           " c.column_ingdatatype, c.column_datatype," +
		   " c.column_length," +
		   " c.column_scale from iicolumns c," +
		   " iiindex_columns ic ";
    whereClause = empty;

    if ( iname != null  &&  iowner != null  &&  
	 bname != null  &&  bowner != null )
	whereClause = " where c.table_name = ? and c.table_owner = ? and " +
			"ic.index_name = ? and ic.index_owner = ? and " +
			"ic.column_name = c.column_name ";

    if ( ! nullable )
    {
	if ( whereClause.length() == 0 )
	    whereClause += " where c.column_nulls <> 'Y'"; 
	else
	    whereClause += " and c.column_nulls <> 'Y'"; 
    }

    qry_txt = selectClause + whereClause;

    try
    {
	stmt = conn.jdbc.prepareStatement(qry_txt);
	int i = 1;

	if ( iname != null  &&  iowner != null  &&  
	     bname != null  &&  bowner != null )
	{
	    stmt.setObject( i++, bname, param_type );
	    stmt.setObject( i++, bowner, param_type );
	    stmt.setObject( i++, iname, param_type );
	    stmt.setObject( i++, iowner, param_type );
	}

	rs = new MDgetBestRowRS( conn, rsmd, 
				 (JdbcRslt)stmt.executeQuery(), true );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() ) 
	    trace.log( title + ".getBestRowIdentifier(): failed (idx)!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;

    }

    return( rs );	// secondary indexes
} // getBestRowIdentifier


/*
** Name: getVersionColumns
**
** Description:
**	Get a description of a table's columns that are automatically
**	updated when any value in a row is updated. 0 rows are returned.
**
** Input:
**	catalog		// Catalog name 
**	schema 		// Schema name
**	table		// Table name
**
** Output:
**	None.
**
** Returns:
**	ResultSet  	// Column description
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	17-Dec-99 (rajus01)
**	    Implemented.
**	04-Feb-00 (rajus01)
**	    Used trim function to remove trailing blanks. Changed 
**	    column names to lower case.
*/

public ResultSet 
getVersionColumns( String catalog, String schema, String table ) 
    throws SQLException
{
    JdbcRSMD    rsmd = new JdbcRSMD( (short)bestRow_desc.length, trace );

    for( int i = 0; i < bestRow_desc.length; i++ )
    {
	rsmd.setColumnInfo( bestRow_desc[i].col_name, i+1,
			    bestRow_desc[i].sql_type,
			    bestRow_desc[i].dbms_type,
			    bestRow_desc[i].length, (byte)0,
			    (byte)0, false );
    }

    if ( trace.enabled() )  
	trace.log( title + ".getVersionColumns( " + 
		   catalog + ", " + schema + ", " + table + " )" );
    return( new RsltData( conn, rsmd, null ) ); 
} // getVersionColumns


/*
** Name: getIndexInfo
**
** Description:
**	Get a description of a table's indices and statistics. They are
**	ordered by NON_UNIQUE, TYPE, INDEX_NAME, and ORDINAL_POSITION.
**	The 'approximate" param value is ignored in the execution of
**	the query (Not sure!!!).
**
** Input:
**	catalog		// Catalog Name 
**	schema 		// Schema Name
**	table		// Table Name
**	unique		// Indices for unique values? (true or false) 
**	approximate	// Data values accurate?? (true or false)
**
** Output:
**	None.
**
** Returns:
**	ResultSet	// Index column description 
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	04-Feb-00 (rajus01)
**	    Used trim function to remove  trailing blanks. Changed 
**	    column names to lower case.
**	25-Feb-00 (rajus01)
**	    Fixed values for prepared statement. No need for private index
**	    variable in MDindexRS.
**	22-Mar-00 (rajus01)
**	    Simplified the SQL query: NULL columns are not supported by
**	    DB2, DCOM gateways. VSAM, IMS gateways didn't like 3-way
**	    unions causing ADF error, terminated the database connection.
**	 8-Mar-01 (gordy)
**	    The previous change, in removing the union select, retained
**	    the wrong query.  Removing iitables query and restoring the
**	    iiindexes query to obtain index and index column information.
**	20-Mar-03 (gordy)
**	    JDBC added BOOLEAN data type.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
*/

private static  MDdesc idx_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_name"),
    new MDdesc(Types.BOOLEAN, VARCHAR, (short)1,  "non_unique"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "index_qualifier"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "index_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "type"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "ordinal_position"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "asc_or_desc"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "cardinality"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "pages"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "filter_condition")
};

private static class
MDindexRS
    extends RsltXlat
{

    public
    MDindexRS( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
    } // MDindexRS

    protected void
    load_data()
	throws SQLException
    {
	setNull( 0 );		// TABLE_CAT
	convert( 1, 1 );	// TABLE_SCHEM from base_owner
	convert( 2, 2 );	// TABLE_NAME from base_name
	
	/* NON_UNIQUE from iiindexes.unique_rule */
	String unique = dbmsRS.getString( 3 );  
	setBoolean( 3, unique != null  &&  unique.equals( "D" ) );

	convert( 4, 4 );	// INDEX_QUALIFIER from index_owner
	convert( 5, 5 );	// INDEX_NAME from index_name
	setShort( 6, (short)tableIndexOther );		// TYPE
	convert( 7, 6 );	// ORDINAL_POSITION from key_sequence
	convert( 8, 7 );	// COLUMN_NAME from column_name
	convert( 9, 8 );	// ASC_OR_DESC from sort_direction
	setInt( 10, (int)0 );	// CARDINALITY
	setInt( 11, (int)0 );	// PAGES
	setNull( 12 ); 		// FILTER_CONDITION
	return;
    } // load_data

} // class MDindexRS

public ResultSet 
getIndexInfo
(
    String catalog, 
    String schema, 
    String table,
    boolean unique, 
    boolean approximate
)
    throws SQLException
{
    PreparedStatement	stmt = null;
    ResultSet		rs = null;
    JdbcRSMD		rsmd = new JdbcRSMD( (short)idx_desc.length, trace );
    String		select, where, order, query;

    if ( trace.enabled() )  
	trace.log( title + ".getIndexInfo( " + catalog + ", " + schema + ", " + 
		   table + ", " + unique + ", " + approximate + " )" );

    select = "select idx.base_owner, idx.base_name, idx.unique_rule, " +
	     "idx.index_owner, idx.index_name, idc.key_sequence, " +
	     "idc.column_name, idc.sort_direction " +
	     "from iiindexes idx, iiindex_columns idc ";

    where = "where idx.index_owner = idc.index_owner and " +
	    "idx.index_name = idc.index_name ";

    order = "order by 3 desc, 5, 6";

    if ( schema != null )  where += "and idx.base_owner = ? ";
    if ( table != null )  where += "and idx.base_name = ? ";
    if ( unique )  where += "and idx.unique_rule = 'U' ";
    
    query = select + where + order;

    for( int i = 0; i < idx_desc.length; i++ )
    {
	if ( upper ) idx_desc[i].col_name = idx_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( idx_desc[i].col_name, i+1, idx_desc[i].sql_type,
			    idx_desc[i].dbms_type, idx_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    try
    {
	stmt = conn.jdbc.prepareStatement( query );
	int i = 1;

	if ( schema != null )  stmt.setObject( i++, schema, param_type );
	if ( table != null )   stmt.setObject( i++, table, param_type );

	rs = new MDindexRS( conn, rsmd, (JdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getIndexInfo(): failed!" );
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }
    
    return( rs );
} // getIndexInfo


/*
** Name: getPrimaryKeys
**
** Description:
**	Get a description of a table's primary key columns. They are
**	ordered by COLUMN_NAME.
**
** Input:
**	catalog		// Catalog name 
**	schema 		// Schema name
**	table		// Table name
**
** Output:
**	None.
**
** Returns:
**	ResultSet 	// Primary key column description
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	22-Mar-00 (rajus01)
**	    Added additional check for IMS,VSAM gateways since their
**	    DBMS_TYPE is also INGRES.
**	21-Feb-02 (gordy)
**	    Ingres DBMS type now pre-determined.
**	19-Jul-02 (gordy)
**	    Query text for physical table structure query wasn't fully built.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
*/

private static  MDdesc pk_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "key_seq"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "pk_name")
};

private static class
MDprimaryKeyRS
    extends RsltXlat
{
    private boolean is_ingres;

    public
    MDprimaryKeyRS( DrvConn conn, JdbcRSMD rsmd, 
		    JdbcRslt dbmsRS, boolean isIngres )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
	is_ingres = isIngres;
    } // MDprimaryKeyRS

    protected void
    load_data()
	throws SQLException
    {
	setNull( 0 );		// TABLE_CAT
	convert( 1, 1 );	// TABLE_SCHEM 
	convert( 2, 2 );	// TABLE_NAME 
	convert( 3, 3 );	// COLUMN_NAME
	convert( 4, 4 );	// KEY_SEQ

	if ( is_ingres )	// PK_NAME 
	    convert( 5, 5 );
	else
	    setNull( 5 );
	return;
    } // load_data

} // MDprimaryKeyRS

public ResultSet 
getPrimaryKeys( String catalog, String schema, String table ) 
    throws SQLException
{

    String		level = conn.dbCaps.getDbCap( DBMS_DBCAP_STD_CAT_LVL );
    String		selectClause = empty; 
    String		whereClause = empty;
    String		orderByClause = empty;
    String		qry_txt = null;
    int			cat_lvl = 0;
    PreparedStatement	stmt = null;
    ResultSet		rs = null;

    if ( level != null )
	try { cat_lvl = Integer.parseInt( level ); }
	catch( Exception ignore ) {};

    JdbcRSMD    rsmd = new JdbcRSMD( (short)pk_desc.length, trace );

    /*
    ** Only Ingres servers have iikeys.
    */
    if ( conn.is_ingres  &&  cat_lvl >= DBMS_SQL_LEVEL_OI10 )
    {
	selectClause = "select distinct k.schema_name, k.table_name," +
		       " k.column_name, k.key_position, k.constraint_name" +
		       " from iikeys k, iiconstraints c";
	whereClause = " where c.constraint_type = 'P'  and " +
		      "k.constraint_name = c.constraint_name";  
	if( schema != null )
	    whereClause += " and k.schema_name = ?";
	if( table != null )
	    whereClause += " and k.table_name = ?";
    	orderByClause = " order by k.column_name";
    }

    for( int i = 0; i < pk_desc.length; i++ )
    {
	if( upper ) pk_desc[i].col_name = pk_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( pk_desc[i].col_name, i+1, pk_desc[i].sql_type,
			    pk_desc[i].dbms_type, pk_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    if ( trace.enabled() )  
	trace.log( title + ".getPrimaryKeys( " + 
		   catalog + ", " + schema + ", " + table + " )" );

    qry_txt = selectClause + whereClause + orderByClause;

    if( qry_txt.length() > 0 )
	try
    	{
	    stmt = conn.jdbc.prepareStatement(qry_txt);
	    int i = 1; 

	    if( schema != null )  stmt.setObject(i++, schema, param_type);
	    if( table != null )   stmt.setObject(i, table, param_type);

	    rs = new MDprimaryKeyRS( conn, rsmd, 
				     (JdbcRslt)stmt.executeQuery(), true );
	    if ( rs.next() )
	    {
	    	try { rs.close(); } catch( SQLException ignore ) {}
		rs = null;

	    	rs = new MDprimaryKeyRS( conn, rsmd,
					 (JdbcRslt)stmt.executeQuery(),true );
	        return( rs );
	    }

	    try { rs.close(); } catch( SQLException ignore ) {}
	    rs = null;
	    try { stmt.close(); } catch( SQLException ignore ) {}
	    stmt = null;
        }
        catch( SQLException ex )
        {
	    if ( trace.enabled() )
	        trace.log( title + ".getPrimaryKeys(): iikeys failed!" ) ;
	    if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	    if ( rs != null )
		try { rs.close(); } catch( SQLException ignore ) {}

	    if ( stmt != null )
		try { stmt.close(); } catch( SQLException ignore ) {}

	    throw ex;
    	}

    // Gateways
    selectClause = "select key_id from iialt_columns";
    whereClause = " where key_sequence <> 0 "; 
    if ( schema != null )  whereClause += " and table_owner = ?";
    if ( table != null )   whereClause += " and table_name = ?";
    qry_txt = selectClause + whereClause;
    int keyID; 

    try
    {
	stmt = conn.jdbc.prepareStatement(qry_txt);
	int i = 1; 

	if ( schema != null )  stmt.setObject(i++, schema, param_type);
	if ( table != null )   stmt.setObject(i, table, param_type);

	rs = (JdbcRslt) stmt.executeQuery();

	if ( rs.next() )
	{
	    keyID = rs.getInt( 1 );
	    try { rs.close(); } catch( SQLException ignore ) {}
	    rs = null;
	    try { stmt.close(); } catch( SQLException ignore ) {}
	    stmt = null;

	    selectClause = "select table_owner, table_name, " +
			   " column_name, key_sequence from iialt_columns";
	    whereClause = " where key_sequence <> 0  and key_id = " + keyID; 
	    orderByClause = " order by column_name";

	    if ( schema != null )  whereClause += " and table_owner = ?";
	    if ( table != null )   whereClause += " and table_name = ?";
	    qry_txt = selectClause + whereClause + orderByClause;

    	    try
    	    {
		stmt = conn.jdbc.prepareStatement(qry_txt);
		i = 1; 

	        if ( schema != null )  stmt.setObject(i++, schema, param_type);
		if ( table != null )   stmt.setObject(i, table, param_type);

		rs = new MDprimaryKeyRS( conn, rsmd, 
					 (JdbcRslt)stmt.executeQuery(), false );

	        return( rs );
	    }
            catch( SQLException ex )
            {
		if ( trace.enabled() )
	    	    trace.log( title + ".getPrimaryKeys(): full iialt_columns failed!" ) ;
	        if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

		if ( stmt != null )
		    try { stmt.close(); } catch( SQLException ignore ) {}

		throw ex;
	    }
	}

	try { rs.close(); } catch( SQLException ignore ) {}
	rs = null;
	try { stmt.close(); } catch( SQLException ignore ) {}
	stmt = null;
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getPrimaryKeys(): iialt_columns failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( rs != null )
	    try { rs.close(); } catch( SQLException ignore ) {}

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    /*
    ** Finally try the table's physical key 
    */
    selectClause = "select t.table_owner, c.table_name," +
	           " c.column_name, c.key_sequence" +
		   " from iitables t, iicolumns c";

    whereClause = " where t.table_name = c.table_name and" +
		  " t.unique_rule  = 'U' and c.key_sequence <> 0";

    if ( schema != null )  whereClause += " and t.table_owner = ?";
    if ( table != null )   whereClause += " and t.table_name = ?";

    orderByClause = " order by c.column_name";
    qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	stmt = conn.jdbc.prepareStatement(qry_txt);
	int i = 1; 

	if ( schema != null )  stmt.setObject( i++, schema, param_type );
	if ( table != null )   stmt.setObject( i, table, param_type );

	rs = new MDprimaryKeyRS( conn, rsmd, 
				 (JdbcRslt)stmt.executeQuery(), false );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() ) 
	    trace.log( title + ".getPrimaryKeys(): iitables failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );
} // getPrimaryKeys


/*
** Name: getImportedKeys
**
** Description:
**	Get a description of the primary key columns that are
**	referenced by a table's foreign key columns ( the primary
**	keys imported by a table ).
**	They are ordered by PKTABLE_CAT, PKTABLE_SCHEM, PKTABLE_NAME,
**	and KEY_SEQ.
**
** Input:
**	catalog		// catalog name 
**	schema 		// schema name
**	table		// table name
**
** Output:
**	None.
**
** Returns:
**	ResultSet 	// Primary key column description
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	16-Feb-00 (rajus01)
**	    Implemented. Return 0 rows for gateways.
**	21-Feb-02 (gordy)
**	    Ingres DBMS type now pre-determined.
**	 4-Aug-03 (gordy)
**	    Row cache removed from result-set.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	20-Jul-07 (gordy)
**	    Columns values may only be assigned in load_data().
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
*/

private static  MDdesc keys_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "pktable_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "pktable_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "pktable_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "pkcolumn_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "fktable_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "fktable_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "fktable_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "fkcolumn_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "key_seq"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "update_rule"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "delete_rule"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "fk_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "pk_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "deferrability")
};

private static class
MDkeysRS
    extends RsltXlat
{

    public
    MDkeysRS( DrvConn conn, JdbcRSMD rsmd, JdbcRslt dbmsRS )
	throws SQLException
    {
	super( conn, rsmd, dbmsRS );
    } // MDkeysRS

    protected void
    load_data()
	throws SQLException
    {
	setNull( 0 );		// PKTABLE_CAT
	convert( 1, 1 );	// PKTABLE_SCHEM  
	convert( 2, 2 );	// PKTABLE_NAME 
	convert( 3, 3 );	// PKCOLUMN_NAME
	setNull( 4 );		// FKTABLE_CAT
	convert( 5, 4 );	// FKTABLE_SCHEM 
	convert( 6, 5 );	// FKTABLE_NAME 
	convert( 7, 6 );	// FKCOLUMN_NAME 
	convert( 8, 7 );	// KEY_SEQ 
	setShort( 9, (short)importedKeyCascade );	// UPDATE_RULE 
	setShort( 10, (short)importedKeyCascade );	// DELETE_RULE 
	convert( 11, 8 );	// FK_NAME 
	convert( 12, 9 );	// PK_NAME 
	// DEFERABILITY (CHECK??)
	setShort( 13, (short)importedKeyNotDeferrable );
	return;
    } // load_data

} // MDkeysRS

public ResultSet 
getImportedKeys( String catalog, String schema, String table ) 
    throws SQLException
{
    PreparedStatement	stmt = null;
    ResultSet		rs = null;
    JdbcRSMD		rsmd = new JdbcRSMD( (short) keys_desc.length, trace );

    for( int i = 0; i < keys_desc.length; i++ )
    {
	if( upper ) keys_desc[i].col_name = keys_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( keys_desc[i].col_name, i+1, keys_desc[i].sql_type,
			    keys_desc[i].dbms_type, keys_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    if ( trace.enabled() )  
	trace.log( title + ".getImportedKeys( " + 
		   catalog + ", " + schema + ", " + table + " )" );

    if ( ! conn.is_ingres )
    {
	/*
	** return 0 rows for gateways.
	*/
	return( new RsltData( conn, rsmd, null  ) ); 
    }


    String selectClause = " select distinct p.schema_name," +
			  " p.table_name, p.column_name," +
			  " f.schema_name, f.table_name," +
			  " f.column_name, p.key_position," +
			  " f.constraint_name, p.constraint_name" +
			  " from iikeys p, iiconstraints c," +
			  " iiref_constraints rc, iikeys f";

    String whereClause = " where c.constraint_type = 'R' and" +
			 " c.constraint_name = rc.ref_constraint_name and" +
			 " p.constraint_name = rc.unique_constraint_name" +
			 " and f.constraint_name = rc.ref_constraint_name" +
			 " and p.key_position = f.key_position ";

    String orderByClause = " order by 1, 2, 7";
      
    if ( schema != null )  whereClause += " and f.schema_name = ?";
    if ( table != null )   whereClause += " and f.table_name = ?";

    String qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	stmt = conn.jdbc.prepareStatement(qry_txt);
	int i = 1; 

    	if ( schema != null )  stmt.setObject( i++, schema, param_type );
	if ( table != null )   stmt.setObject( i++, table, param_type );

	rs = new MDkeysRS( conn, rsmd, (JdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getImportedKeys(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );
} // getImportedKeys


/*
** Name: getExportedKeys
**
** Description:
**	Get a description of the foreign key columns that reference a 
**	table's primary key columns ( the foreign keys exported by a
**	table). THey are ordered by FKTABLE_CAT, FKTABLE_SCHEM, 
**	FKTABLE_NAME, and KEY_SEQ.
**
** Input:
**	catalog		// catalog name 
**	schema 		// schema name
**	table		// table name
**
** Output:
**	None.
**
** Returns:
**	ResultSet 	// Foreign key column description.
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	17-Dec-99 (rajus01)
**	    Implemented.
**	16-Feb-00 (rajus01)
**	    Return 0 rows for gateways.
**	21-Feb-02 (gordy)
**	    Ingres DBMS type now pre-determined.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
*/

public ResultSet 
getExportedKeys( String catalog, String schema, String table) 
    throws SQLException
{
    PreparedStatement	stmt = null;
    ResultSet		rs = null;
    JdbcRSMD		rsmd = new JdbcRSMD( (short) keys_desc.length, trace );

    for( int i = 0; i < keys_desc.length; i++ )
	rsmd.setColumnInfo( keys_desc[i].col_name, i+1, keys_desc[i].sql_type,
			    keys_desc[i].dbms_type, keys_desc[i].length,
			    (byte)0, (byte)0, false );

    if ( trace.enabled() )  
	trace.log( title + ".getExportedKeys( " + 
		   catalog + ", " + schema + ", " + table + " )" );

    if ( ! conn.is_ingres )
    {
	/*
	** return 0 rows for gateways.
	*/
	return( new RsltData( conn, rsmd, null ) ); 
    }

    String selectClause = " select distinct p.schema_name," +
			  " p.table_name, p.column_name," +
			  " f.schema_name, f.table_name," +
			  " f.column_name, f.key_position," +
			  " f.constraint_name, p.constraint_name" +
			  " from iikeys p, iiconstraints c," +
			  " iiref_constraints rc, iikeys f";

    String whereClause = " where c.constraint_type = 'R' and" +
			 " c.constraint_name = rc.ref_constraint_name and" +
			 " p.constraint_name = rc.unique_constraint_name" +
			 " and f.constraint_name = rc.ref_constraint_name" +
			 " and p.key_position = f.key_position ";

    String orderByClause = " order by 4, 5, 7";
      
    if ( schema != null )  whereClause += " and p.schema_name = ? ";
    if ( table != null )   whereClause += " and p.table_name = ? ";

    String qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	stmt = conn.jdbc.prepareStatement(qry_txt);
	int i = 1; 

    	if ( schema != null ) stmt.setObject( i++, schema, param_type );
	if ( table != null )  stmt.setObject( i++, table, param_type );

	rs = new MDkeysRS( conn, rsmd, (JdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getExportedKeys(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );
} // getExportedKeys


/*
** Name: getCrossReference
**
** Description:
**	Get a description of the foreign key columns in the foreign key
**	table that reference the primary key columns of the primary key
**	table (describe how one table imports another key. ) This
**	should normally return a single foreign key/primary key pair
**	(most tables only import a foreign key from a table once.) They
**	are ordered by FKTABLE_CAT, FKTABLE_SCHEM, FKTABLE_NAME, and
**	KEY_SEQ.
**
** Input:
**	primaryCatalog		// primary catalog name 
**	primarySchema 		// primary schema name
**	primaryTable		// primary table name
**	foreignCatalog		// foreign catalog name 
**	foreignSchema 		// foreign schema name
**	foreignTable		// foreign table name
**
** Output:
**	None.
**
** Returns:
**	ResultSet 	// Foreign key column description
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	04-Feb-00 (rajus01)
**	    Used trim function to remove trailing blanks. Changed 
**	    column names to lower case.
**	16-Feb-00 (rajus01)
**	    Return 0 rows for gateways.
**	21-Feb-02 (gordy)
**	    Ingres DBMS type now pre-determined.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are released when no longer needed.
*/

public ResultSet 
getCrossReference
(
    String primaryCatalog, 
    String primarySchema, 
    String primaryTable,
    String foreignCatalog, 
    String foreignSchema, 
    String foreignTable
) 
    throws SQLException
{
    PreparedStatement	stmt = null;
    ResultSet		rs = null;
    JdbcRSMD		rsmd = new JdbcRSMD( (short) keys_desc.length, trace );

    for( int i = 0; i < keys_desc.length; i++ )
	rsmd.setColumnInfo( keys_desc[i].col_name, i+1, keys_desc[i].sql_type,
			    keys_desc[i].dbms_type, keys_desc[i].length,
			    (byte)0, (byte)0, false );

    if ( trace.enabled() )  
	trace.log( title + ".getCrossReference( " + primaryCatalog + ", " + 
		   primarySchema + ", " + primaryTable + ", " + 
		   foreignCatalog + ", " + foreignSchema + ", " + 
		   foreignTable + " )" );

    if ( ! conn.is_ingres )
    {
	/*
	** return 0 rows for gateways.
	*/
	return( new RsltData( conn, rsmd, null  ) ); 
    }

    String selectClause = " select distinct p.schema_name," +
			  " p.table_name, p.column_name," +
			  " f.schema_name, f.table_name," +
			  " f.column_name, f.key_position," +
			  " f.constraint_name, p.constraint_name" +
			  " from iikeys p, iiconstraints c," +
			  " iiref_constraints rc, iikeys f";

    String whereClause = " where c.constraint_type = 'R' and" +
			 " c.constraint_name = rc.ref_constraint_name and" +
			 " p.constraint_name = rc.unique_constraint_name" +
			 " and f.constraint_name = rc.ref_constraint_name" +
			 " and p.key_position = f.key_position ";

    String orderByClause = " order by 4, 5, 7";
      
    if ( primarySchema != null )  whereClause += " and p.schema_name = ?";
    if ( foreignSchema != null )  whereClause += " and f.schema_name = ?";
    if ( primaryTable != null )   whereClause += " and p.table_name = ?";
    if ( foreignTable != null )   whereClause += " and f.table_name = ?";

    String qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	stmt = conn.jdbc.prepareStatement(qry_txt);
	int i = 1; 

    	if( primarySchema != null )
	    stmt.setObject(i++, primarySchema, param_type);
    	if( foreignSchema != null )
	    stmt.setObject(i++, foreignSchema, param_type);
	if( primaryTable != null )
	    stmt.setObject(i++, primaryTable, param_type);
	if( foreignTable != null )
	    stmt.setObject(i++, foreignTable, param_type);

	rs = new MDkeysRS( conn, rsmd, (JdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getCrossReference(): failed!" ) ;
	if ( trace.enabled( 1 ) )  SqlExFactory.trace(ex, trace );

	if ( stmt != null )
	    try { stmt.close(); } catch( SQLException ignore ) {}

	throw ex;
    }

    return( rs );

} // getCrossReference


/*
** Name: getTypeInfo
**
** Description:
**	Get a description of all the standard SQL types supported by
**	this database. They are ordered by DATA_TYPE and then by how
**	closely the data type maps to the corresponding JDBC SQL type.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	ResultSet 		// SQL Type description
**
** History:
**	20-May-99 (gordy)
**	    Created.
**	03-Dec-99 (rajus01)
**	    Implemented.
**	26-Sep-03 (gordy)
**	    Result-set values now stored as SqlData objects.
**	15-Mar-04 (gordy)
**	    Support BIGINT.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.  Order by SQL type.
**	    Ingres SQL level now pre-determined by DrvConn.
**	30-May-08 (rajus01) SD issue 128649, Bug 120439.
**	    The DBMD getTypeInfo() method returns the string 'precision'
**	    for CREATE_PARAMS attribute for double precision data type
**	    which causes create table statements fail in OpenOffice. 
**	    A NULL is expected for this attribute for double precision type
**	    which will enable openOffice not to issue create table statements
**	    with double precision(p) where p is the precision value. 
**
**	    The precision values returned for the real, float, and 
**	    double precision data types seem correct based on Java 6 API spec
**	    that precision value should indicate maximum numeric precision.
**	    And Ingres indicates that float8 have a decimal precision of 15. 
**	    The value range 24 <= x <= 53 for float, float(x), double precision
**	    (which are all synonyms for float8) indicates binary precision of 
**	    mantissa. And the synonyms for float4 are real and float(x) 
**	    where 1 <= x <= 23.
**
**	    In the OpenSqlTypes[] structure, changed the precision value from
**	    I7 to I15 for double precision.
**	04-Jun-08 (rajus01) SD issue 128696, Bug 120463
**	    It has been found that the getTypeInfo() method returns incorrect 
**	    values for the precision attribute for char, varchar, byte, varbyte
**	    data types. Also fixed the meta-data descriptor (md_desc) for the
**	    local_type_name attribute and the precision of DATE/TIME/TIMESTAMP
**	    data types.
**	26-Oct-09 (gordy)
**	    Added BOOLEAN type.
*/

    private final static SqlNull	NULL	= new SqlNull();
    private final static SqlSmallInt	S0	= new SqlSmallInt( (short)0 );
    private final static SqlSmallInt	S1	= new SqlSmallInt( (short)1 );
    private final static SqlSmallInt	S31	= new SqlSmallInt( (short)31 );
    private final static SqlInt		I0	= new SqlInt( 0 );
    private final static SqlInt		I3	= new SqlInt( 3 );
    private final static SqlInt		I5	= new SqlInt( 5 );
    private final static SqlInt		I7	= new SqlInt( 7 );
    private final static SqlInt		I8	= new SqlInt( 8 );
    private final static SqlInt		I10	= new SqlInt( 10 );
    private final static SqlInt		I15	= new SqlInt( 15 );
    private final static SqlInt		I19	= new SqlInt( 19 );
    private final static SqlInt		I29	= new SqlInt( 29 );
    private final static SqlInt		I31	= new SqlInt( 31 );
    private final static SqlInt		I2000	= new SqlInt( 2000 );
    private final static SqlString	QUOTE	= new SqlString( "'" ); 
    private final static SqlString	S_HEX	= new SqlString( "X'" ); 
    private static final SqlString	S_LEN	= new SqlString( "length" );
    private static final SqlString	S_PREC	= new SqlString( "precision" );
    private static final SqlString	S_PS	= new SqlString( "precision,scale" );

    private final static SqlSmallInt	T_NULL	= 
				new SqlSmallInt( (short)typeNullable );
    private final static SqlSmallInt	T_SRCH	= 
				new SqlSmallInt( (short)typeSearchable );
    private final static SqlSmallInt	T_PRDN	= 
				new SqlSmallInt( (short)typePredNone );
    private final static SqlSmallInt	T_PRDB	= 
				new SqlSmallInt( (short)typePredBasic );
    
    private static final SqlString	S_TINT	= 
					new SqlString( DBMS_TYPSTR_INT1 );
    private static final SqlString	S_SINT	= 
					new SqlString( DBMS_TYPSTR_INT2 );
    private static final SqlString	S_INT	= 
					new SqlString( DBMS_TYPSTR_INT4 );
    private static final SqlString	S_BINT	= 
					new SqlString( DBMS_TYPSTR_INT8 );
    private static final SqlString	S_FLT	= 
					new SqlString( DBMS_TYPSTR_FLT8 );
    private static final SqlString	S_REAL	= 
					new SqlString( "real" );
    private static final SqlString	S_DBL	= 
					new SqlString( "double precision" );
    private static final SqlString	S_DEC	= 
					new SqlString( DBMS_TYPSTR_DECIMAL );
    private static final SqlString	S_CHAR	= 
					new SqlString( DBMS_TYPSTR_CHAR );
    private static final SqlString	S_VCH	= 
					new SqlString( DBMS_TYPSTR_VARCHAR );
    private static final SqlString	S_LVCH	= 
					new SqlString( DBMS_TYPSTR_LONG_CHAR );
    private static final SqlString	S_NCHAR	= 
					new SqlString( DBMS_TYPSTR_NCHAR );
    private static final SqlString	S_NVCH	= 
					new SqlString( DBMS_TYPSTR_NVARCHAR );
    private static final SqlString	S_LNVCH	= 
					new SqlString( DBMS_TYPSTR_LONG_NCHAR );
    private static final SqlString	S_BYTE	= 
					new SqlString( DBMS_TYPSTR_BYTE );
    private static final SqlString	S_VBYT	= 
					new SqlString( DBMS_TYPSTR_VARBYTE );
    private static final SqlString	S_LBYT	= 
					new SqlString( DBMS_TYPSTR_LONG_BYTE );
    private static final SqlString	S_BOOL	= 
					new SqlString( DBMS_TYPSTR_BOOL );
    private static final SqlString	S_IDAT	= 
					new SqlString( DBMS_TYPSTR_IDATE );
    private static final SqlString	S_ADAT	= 
					new SqlString( DBMS_TYPSTR_ADATE );
    private static final SqlString	S_TIME	= 
					new SqlString( "time" );
    private static final SqlString	S_TS	= 
					new SqlString( "timestamp" );
    
    private static final SqlSmallInt	T_TINT	= 
				new SqlSmallInt( (short)Types.TINYINT );
    private static final SqlSmallInt	T_SINT	= 
				new SqlSmallInt( (short)Types.SMALLINT );
    private static final SqlSmallInt	T_INT	= 
				new SqlSmallInt( (short)Types.INTEGER );
    private static final SqlSmallInt	T_BINT	= 
				new SqlSmallInt( (short)Types.BIGINT );
    private static final SqlSmallInt	T_FLT	= 
				new SqlSmallInt( (short)Types.FLOAT );
    private static final SqlSmallInt	T_REAL	= 
				new SqlSmallInt( (short)Types.REAL );
    private static final SqlSmallInt	T_DBL	= 
				new SqlSmallInt( (short)Types.DOUBLE );
    private static final SqlSmallInt	T_DEC	= 
				new SqlSmallInt( (short)Types.DECIMAL );
    private static final SqlSmallInt	T_CHAR	= 
				new SqlSmallInt( (short)Types.CHAR );
    private static final SqlSmallInt	T_VCH	= 
				new SqlSmallInt( (short)Types.VARCHAR );
    private static final SqlSmallInt	T_LVCH	= 
				new SqlSmallInt( (short)Types.LONGVARCHAR );
    private static final SqlSmallInt	T_NCHAR	= 
				new SqlSmallInt( (short)Types.NCHAR );
    private static final SqlSmallInt	T_NVCH	= 
				new SqlSmallInt( (short)Types.NVARCHAR );
    private static final SqlSmallInt	T_LNVCH	= 
				new SqlSmallInt( (short)Types.LONGNVARCHAR );
    private static final SqlSmallInt	T_BYTE	= 
				new SqlSmallInt( (short)Types.BINARY );
    private static final SqlSmallInt	T_VBYT	= 
				new SqlSmallInt( (short)Types.VARBINARY );
    private static final SqlSmallInt	T_LBYT	= 
				new SqlSmallInt( (short)Types.LONGVARBINARY );
    private static final SqlSmallInt	T_BOOL	= 
				new SqlSmallInt( (short)Types.BOOLEAN );
    private static final SqlSmallInt	T_DATE	= 
				new SqlSmallInt( (short)Types.DATE );
    private static final SqlSmallInt	T_TIME	= 
				new SqlSmallInt( (short)Types.TIME );
    private static final SqlSmallInt	T_TS	= 
				new SqlSmallInt( (short)Types.TIMESTAMP );
	
private static final SqlData	OpenSqlTypeValues[][] =
{
    {	// Char
	S_CHAR, T_CHAR,	I2000,	QUOTE,	QUOTE,	S_LEN,	T_NULL,	S1,	T_SRCH,
	S0,	S0,	S0,	S_CHAR,	S0,	S0,	I0,	I0,	I10 
    },
    {	// Integer
	S_INT,	T_INT,	I10,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,	
	S1,	S0,	S0,	S_INT,	S0,	S0,	I0,	I0,	I10 
    },
    {	// Smallint
	S_SINT,	T_SINT,	I5,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,	
	S1,	S0,	S0,	S_SINT,	S0,	S0,	I0,	I0,	I10 
    },
    {	// Float (double)
	S_FLT,	T_FLT,	I7,	NULL,	NULL,	S_PREC,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_FLT,	S0,	S0,	I0,	I0,	I10
    },
    {	// Real
	S_REAL, T_REAL,	I7,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_REAL,	S0,	S0,	I0,	I0,	I10
    },
    {	// Double
	S_DBL,	T_DBL,	I15,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_DBL,	S0,	S0,	I0,	I0,	I10
    },
    {	// Varchar
	S_VCH,	T_VCH,	I2000,	QUOTE,	QUOTE,	S_LEN,	T_NULL,	S1,	T_SRCH,	
	S0,	S0,	S0,	S_VCH,	S0,	S0,	I0,	I0,	I10
    },
    {	// Ingres Date
	S_IDAT,	T_TS,	I29,	QUOTE,	QUOTE,	NULL,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_IDAT,	S0,	S0,	I0,	I0,	I10
    },
};

private static final SqlData	sqlTypeValues[][] =
{
    {	// Long nvarchar
	S_LNVCH, T_LNVCH, I0,	QUOTE,	QUOTE,	NULL,	T_NULL,	S1,	T_SRCH,
	S1,	S0,	S0,	S_LNVCH, S0,	S0,	I0,	I0,	I10
    },
    {	// Nchar
	S_NCHAR, T_NCHAR, I2000, QUOTE,	QUOTE,	S_LEN,	T_NULL,	S1,	T_SRCH,
	S1,	S0,	S0,	S_NCHAR, S0,	S0,	I0,	I0,	I10
    },
    {	// Nvarchar
	S_NVCH,	T_NVCH,	I2000,	QUOTE,	QUOTE,	S_LEN,	T_NULL,	S1,	T_SRCH,
	S1,	S0,	S0,	S_NVCH,	S0,	S0,	I0,	I0,	I10
    },
    {	// Tinyint
	S_TINT,	T_TINT,	I3,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,
	S0,	S0,	S0,	S_TINT,	S0,	S0,	I0,	I0,	I10
    },
    {	// Bigint
	S_BINT,	T_BINT,	I19,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,
	S0,	S0,	S0,	S_INT,	S0,	S0,	I0,	I0,	I10
    },
    {	// Long byte
	S_LBYT,	T_LBYT,	I0,	S_HEX,	QUOTE,	NULL,	T_NULL,	S0,	T_PRDN,
	S0,	S0,	S0,	S_LBYT,	S0,	S0,	I0,	I0,	I10
    },
    {	// Varbyte
	S_VBYT,	T_VBYT,	I2000,	S_HEX,	QUOTE,	S_LEN,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_VBYT,	S0,	S0,	I0,	I0,	I10
    },
    {	// Byte
	S_BYTE,	T_BYTE,	I2000,	S_HEX,	QUOTE,	S_LEN,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_BYTE,	S0,	S0,	I0,	I0,	I10
    },
    {	// Long varchar
	S_LVCH,	T_LVCH,	I0,	QUOTE,	QUOTE,	NULL,	T_NULL,	S1,	T_SRCH,
	S1,	S0,	S0,	S_LVCH,	S0,	S0,	I0,	I0,	I10
    },
    {	// Char
	S_CHAR,	T_CHAR,	I2000,	QUOTE,	QUOTE,	S_LEN,	T_NULL,	S1,	T_SRCH,
	S1,	S0,	S0,	S_CHAR,	S0,	S0,	I0,	I0,	I10
    },
    {	// Decimal
	S_DEC,	T_DEC,	I31,	NULL,	NULL,	S_PS,	T_NULL,	S0,	T_PRDB,
	S0,	S0,	S0,	S_DEC,	S0,	S31,	I0,	I0,	I10
    },
    {	// Integer
	S_INT,	T_INT,	I10,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,
	S0,	S0,	S0,	S_INT,	S0,	S0,	I0,	I0,	I10
    },
    {	// Smallint
	S_SINT,	T_SINT,	I5,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,
	S0,	S0,	S0,	S_SINT,	S0,	S0,	I0,	I0,	I10
    },
    {	// Float (double)
	S_FLT,	T_FLT,	I15,	NULL,	NULL,	S_PREC,	T_NULL,	S0,	T_PRDB,
	S0,	S0,	S0,	S_FLT,	S0,	S0,	I0,	I0,	I10
    },
    {	// Real
	S_REAL,	T_REAL,	I7,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,
	S0,	S0,	S0,	S_REAL,	S0,	S0,	I0,	I0,	I10
    },
    {	// Double
	S_DBL,	T_DBL,	I15,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,
	S0,	S0,	S0,	S_DBL,	S0,	S0,	I0,	I0,	I10
    },
    {	// Varchar
	S_VCH,	T_VCH,	I2000,	QUOTE,	QUOTE,	S_LEN,	T_NULL,	S1,	T_SRCH,
	S1,	S0,	S0,	S_VCH,	S0,	S0,	I0,	I0,	I10
    },
    {	// Boolean
	S_BOOL, T_BOOL, I0,	NULL,	NULL,	NULL,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_BOOL,	S0,	S0,	I0,	I0,	I10
    },
    {	// ANSI Date
	S_ADAT,	T_DATE,	I10,	QUOTE,	QUOTE,	NULL,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_ADAT,	S0,	S0,	I0,	I0,	I10
    },
    {	// ANSI Time
	S_TIME,	T_TIME,	I8,	QUOTE,	QUOTE,	NULL,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_TIME,	S0,	S0,	I0,	I0,	I10
    },
    {	// ANSI Timestamp
	S_TS,	T_TS,	I29,	QUOTE,	QUOTE,	NULL,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_TS,	S0,	S0,	I0,	I0,	I10
    },
    {	// Ingres Date
	S_IDAT,	T_TS,	I29,	QUOTE,	QUOTE,	NULL,	T_NULL,	S0,	T_PRDB,
	S1,	S0,	S0,	S_IDAT,	S0,	S0,	I0,	I0,	I10
    },
};

private static  MDdesc md_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "data_type"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "precision"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)1,  "literal_prefix"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)1,  "literal_suffix"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "create_params"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "nullable"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "case_sensitive"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "searchable"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "unsigned_attribute"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "fixed_prec_scale"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "auto_increment"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "local_type_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "minimum_scale"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "maximum_scale"),
    new MDdesc(Types.INTEGER,  INTEGER, (short)4, "sql_data_type"),
    new MDdesc(Types.INTEGER,  INTEGER, (short)4, "sql_datetime_sub"),
    new MDdesc(Types.INTEGER,  INTEGER, (short)4, "num_prec_radix")
};

public ResultSet 
getTypeInfo() 
    throws SQLException
{
    JdbcRSMD    rsmd = new JdbcRSMD( (short)md_desc.length, trace );

    for( int i = 0; i < md_desc.length; i++ )
    {
	if( upper ) md_desc[i].col_name = md_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( md_desc[i].col_name, i+1, md_desc[i].sql_type,
			    md_desc[i].dbms_type, md_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    if ( trace.enabled() )  trace.log( title + ".getTypeInfo()" );

    SqlData types[][] = (conn.sqlLevel > 0) ? sqlTypeValues : OpenSqlTypeValues;
    SqlData dataset[][] = new SqlData[ types.length ][];
     
    for( int i = 0; i < types.length; i++ )
    {
	String	dbcap = null;
	dataset[i] = new SqlData[ types[i].length];
	for( int j = 0; j < types[i].length; j++ )
	    dataset[i][j] = types[i][j];
	if( dataset[i][0] == S_CHAR )
	    dbcap = conn.dbCaps.getDbCap(DBMS_DBCAP_MAX_CHR_COL); 
	else if( dataset[i][0] == S_VCH )
	    dbcap = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_VCH_COL);
	else if( dataset[i][0] == S_BYTE )
	    dbcap = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_BYT_COL );
	else if( dataset[i][0] == S_VBYT )
	    dbcap = conn.dbCaps.getDbCap( DBMS_DBCAP_MAX_VBY_COL );
	if ( dbcap != null )
            try 
	    { 
		int dbcapVal = 0;
	        dbcapVal = Integer.parseInt( dbcap ); 
	        dataset[i][2] = new SqlInt(dbcapVal);
	    }
 	    catch( Exception ignore ) {};
    }

    return( new RsltData( conn, rsmd, dataset ) );

} // getTypeInfo


/*
** Name: getSuperTypes
**
** Description:
**	Get UDT and super-type information.
**
** Input:
**	catalog			Catalog name.
**	schemaPattern		Schema name pattern.
**	typeNamePattern		Type name pattern.
**
** Output:
**	None.
**
** Returns:
**	ResultSet		UDT super-types. 
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

private static  MDdesc super_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "supertype_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "supertype_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "supertype_name"),
};

public ResultSet 
getSuperTypes
( 
    String	catalog, 
    String	schemaPattern, 
    String	typeNamePattern
) 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getSuperTypes('" +
	catalog + "','" + schemaPattern + "','" + typeNamePattern + "')" );

    JdbcRSMD    rsmd = new JdbcRSMD( (short)super_desc.length, trace );

    for( int i = 0; i < super_desc.length; i++ )
    {
	if ( upper ) 
	    super_desc[i].col_name = super_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( super_desc[i].col_name, i+1, super_desc[i].sql_type,
			    super_desc[i].dbms_type, super_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    return( new RsltData( conn, rsmd, null ) );
} // getSuperTypes


/*
** Name: getUDTs
**
** Description:
**	Get UDT description for a schema.
**
** Input:
**	catalog			Catalog name.
**	schemaPattern		Schema name pattern.
**	typeNamePattern		Type name pattern.
**	types			Array of types.
**
** Output:
**	None.
**
** Returns:
**	ResultSet		UDT description. 
**
** History:
**	10-Nov-00 (gordy)
**	    Created.
**	20-Mar-03 (gordy)
**	    Added base_type to result-set for JDBC 3.0.
*/

private static  MDdesc udt_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "class_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "data_type"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)256, "remarks"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "base_type"),
};

public ResultSet 
getUDTs
( 
    String	catalog, 
    String	schemaPattern, 
    String	typeNamePattern, 
    int		types[]

) 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getUDTs( " +
	catalog + ", " + schemaPattern + ", " + typeNamePattern + " )" );

    JdbcRSMD    rsmd = new JdbcRSMD( (short)udt_desc.length, trace );

    for( int i = 0; i < udt_desc.length; i++ )
    {
	if( upper ) udt_desc[i].col_name = udt_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( udt_desc[i].col_name, i+1, udt_desc[i].sql_type,
			    udt_desc[i].dbms_type, udt_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    return( new RsltData( conn, rsmd, null ) );
} // getUDTs


/*
** Name: getAttributess
**
** Description:
**	Get UDT attribute descriptions.
**
** Input:
**	catalog			Catalog name.
**	schemaPattern		Schema name pattern.
**	typeNamePattern		Type name pattern.
**	attributeNamePattern	Attribute name pattern.
**
** Output:
**	None.
**
** Returns:
**	ResultSet		UDT attribute descriptions. 
**
** History:
**	20-Mar-03 (gordy)
**	    Created.
*/

private static  MDdesc att_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "attr_name"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4, "data_type" ),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "attr_type_name"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "attr_size"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "decimal_digits"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "num_prec_radix"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "nullable"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)256, "remarks"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "attr_def"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "sql_data_type"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "sql_datetime_sub"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "char_octet_length"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "ordinal_position"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "is_nullable"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "scope_catalog"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "scope_schema"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "scope_table"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "attr_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "source_data_type" ),
};

public ResultSet 
getAttributes
( 
    String	catalog, 
    String	schemaPattern, 
    String	typeNamePattern, 
    String	attributeNamePattern

) 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getUDTs('" + catalog + "','" + schemaPattern + 
	"','" + typeNamePattern + "','" + attributeNamePattern + "')" );

    JdbcRSMD    rsmd = new JdbcRSMD( (short)att_desc.length, trace );

    for( int i = 0; i < att_desc.length; i++ )
    {
	if ( upper ) att_desc[i].col_name = att_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( att_desc[i].col_name, i + 1, att_desc[i].sql_type,
			    att_desc[i].dbms_type, att_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    return( new RsltData( conn, rsmd, null ) );
} // getAttributes


/*
** Name: makePattern
**
** Description:
**	Returns a WHERE clause segment in syntax:
**	  " where|and trim(column) like pattern".
**
**	The pattern is not changed from its input value if pattern is '%'
**	or null or % is the last character or if DBMS server is one that
**	supports the trim() function, which is most DBMS's except EDBC
**	IMS and VSAM. Otherwise it is appended with ' %'.
**
**	Since rtrim() is now used for DB2, prior special code is no 
**	longer relevant and was removed that required special handling for
**	schema patterns due to the datatype being CHAR, rather than VARCHAR, 
**	for table_owner in 'iitables' catalog. 
**
**	Generally (though not required), callers of this method will want 
**	to append the returned where clause to their existing one; for example: 
**	   whereClause += makePattern(...);
**	If the input where clause length is zero, then the clause is started
**	with " where" instead of " and".
**
** Input:
**	pattern		String	- Pattern supplied to JDBC getXXX() by appl
**	whereClauseLength  int  - Existing "WHERE" clause length
**	columnName	String	- Catalog column that pattern relates to
**	likeNotSupported boolean- Like not supported in WHERE clause
**
** Output:
**	None.
**
** Returns:
**	String	whereClause - "WHERE" clause segment for input pattern
**
** History:
**	22-Mar-00 (rajus01)
**	    Created.
**	28-Mar-00 (rajus01)
**	    Removed ')'.
**	 6-Mar-01 (gordy)
**	    Sybase and MS-SQL don't need ' %'.    	   
**	13-Jul-06 (lunbr01)  Bug 115453
**	    DatabaseMetaData.getColumns() returns NO_DATA when table
**	    name is 32 characters long.  Expand functionality of this
**	    method to also update "where" clause to simplify overall
**	    code and reintroduce use of trim where supported.
**	    Fix for EDBC IDMS, IMS and VSAM must be handled differently 
**	    than Ingres and other gateways.
**	16-Dec-08 (gordy)  BUG 121345
**	    Include escape clause for LIKE predicate in accordance with
**	    JDBC spec for method DatabaseMetaData.getSearchStringEscape().
**	26-Mar-09 (rajus01) Bug 121865
**	    While running OpenJPA testsuite with INGRES it has been brought
**	    to notice that metadata queries used the "LIKE" predicate even
**	    when database object names passed in to the DatabaseMetaData 
**	    methods didn't have any pattern matching characters in them.
**	    The changes made in this submission parses the pattern for
**	    pattern matching characters "%, \, _" and decides whether "="
**	    operator or LIKE predicate needs to be used. It DOES NOT 
**	    completely optimize when the database objects contain 
**	    unescaped "_". 
**	    Gordy's further comments:
**	    There is an UNFORTUNATE condition exists where the '_' is valid 
**	    and frequent identifier character and also a pattern character.
**	    Thus a meta-data request for 'jdbc_date' appears to be a wild-card
**	    pattern.  Even stranger, if you wanted to indicate that it isn't a
**	    wild-card pattern, it would have to be 'jdbc\_date', which requires
**	    the like predicate so that the escape clause can be used, 
**	    so it is still technically a pattern. There are about 90 standard
**	    catalogs and over 1000 columns in the catalogs which contain 
**	    the '_'. It is very likely that object names with '_' will 
**	    frequently not be escaped simply because one doesn't tend to 
**	    think of the '_' as a pattern matching character when it is 
**	    actually a part of the object name.
**	    For example, an object name from a meta-data query, 
**	    say getTables(), used as an argument in another meta-data 
**	    query, say getColumns(), it is highly unlikely that names with '_'
**	    wil get escaped. There is no way to optimize a pattern that 
**	    contains an unescaped '_'. 
**	    Bruce's thoughts on this:
**	    One way to tackle this is to speed up the LIKE processing on the 
**	    catalogs in the database, in which case the driver could continue
**	    to send LIKE as it does today. I suspect this would require 
**	    special processing in the DBMS to handle LIKE for catalog tables 
**	    vs user tables, since doing a LIKE in this fashion on a 
**	    standard select statement seems like it is going to be inherently
**	    less efficient than an =.
*/

private String 
makePattern ( String pattern, int whereClauseLength, String columnName, boolean	likeNotSupported )
    throws SQLException
{
    String  dbmsType = conn.dbCaps.getDbCap( DBMS_DBCAP_DBMS_TYPE );
    String  whereClause = empty;
    String  operator;
    String  escape = empty;
    String  trim_fn_beg = "trim(";
    String  trim_fn_end = ")";

    int	    isql_lvl = 0;
    String  sql_level = conn.dbCaps.getDbCap( DBMS_DBCAP_ING_SQL_LVL );
    if( sql_level != null )
	try { isql_lvl = Integer.parseInt( sql_level ) ; }
	catch( Exception ignore ){};

    if ( trace.enabled() )  
	trace.log( title + ".makePattern(" + pattern + ", " + 
		   whereClauseLength + ", " + columnName + ", " + 
		   likeNotSupported + ") [dbmsType: " + dbmsType + "]" ) ;

    if ( likeNotSupported  || ! checkPattern( pattern, escape_char ) )
    {
	operator = " = ";
	trim_fn_beg = trim_fn_end = empty;
	if ( pattern != null  &&  pattern.equals( "%" ) )  pattern = null;
    }
    else   // patterns are not used for IDMS on iicolumns (table procedure)
    {

	operator = " like ";
	escape = escape_clause;

	if ( dbmsType != null)
	{
	    /*
	    ** DB2 only supports rtrim(), not trim().
	    */
	    if ( dbmsType.equals( "DB2" ) )
		trim_fn_beg = "rtrim(";
	    else 
	    /*
	    **  trim() fails with syntax error in where clause for IMS & VSAM 
	    **  (BUG in EDBC IMS & VSAM)
	    **  dbmsType=INGRES for IMS & VSAM, so must check INGRES/SQL_LEVEL
	    **  which is 601 for these EDBC servers.
	    */
	    if ( dbmsType.equals( "INGRES" ) && ( isql_lvl == DBMS_SQL_LEVEL_IMS_VSAM ) )
	    {
		trim_fn_beg = trim_fn_end = empty;
		if ( pattern != null && 
		     pattern.length() >=  1 && 
		     pattern.length() <  24 && 
		     pattern.charAt( pattern.length() - 1 ) != '%'
		   ) 
	    	    pattern = pattern + " %";
	    }
	}
    }

    if ( pattern != null )
	whereClause = (whereClauseLength == 0 ? " where " : " and ") + 
			trim_fn_beg + columnName + trim_fn_end + 
			operator + "'" + pattern + "'" + escape;


    if ( trace.enabled() )  
	trace.log( title + ".makePattern(): " + whereClause ) ;
    return( whereClause );
} // makePattern

/*
** Name: checkPattern
**
** Description:
**      Checks the input pattern string for pattern matching characters.
**      The pattern matching characters considered are %, \, and _
**
** Input:
**      pattern 	String
**      esc		String
**
** Output:
**      None.
**
** Returns:
**      boolean
**
** History:
**      25-Mar-09 (rajus01) Bug 121865
**         Created.
*/
boolean
checkPattern( String pattern, String esc )
{
    boolean result = false;

    if( pattern == null ) return( result );
    char [] cha = pattern.toCharArray();

    if( esc != null ) 
    {
	char [] escChar = esc.toCharArray();

    	for (int i = 0 ; i < cha.length; i++ )
	    if( cha[i] == '%' || cha[i] == '_' || cha[i] == escChar[0] )
	    {
		result = true;
		break;
	    }
    }
    else
    {
    	for (int i = 0 ; i < cha.length; i++ )
	    if( cha[i] == '%' || cha[i] == '_' )
	    {
		result = true;
		break;
	    }
    }

    if ( trace.enabled() )
        trace.log( title + ".checkPattern(): " + result) ;

    return( result );
} //checkPattern


/*
** Name: getStringType 
**
** Description:
**	Converts int to String.
**
** Input:
**	type	SQL type	
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	07-Dec-99 (rajus01)
**	    Created.
**      16-Mar-01 (loera01) Bug 104329
**          Added 2.0 extensions NULL, ARRAY, STRUCT, JAVA_OBJECT, OTHER,
**          REF, DISTINCT, CLOB and BLOB.
**	22-Apr-09 (rajus01) SIR 121238
**	    Added NCHAR,NVARCHAR,LONGNVARCHAR,NCLOB,SQLXML and ROWID. 
*/

private String
getStringType ( int type )
    throws SQLException
{
    switch( type )
    {
    case Types.NULL:		return( "NULL");
    case Types.BIT:		return( "BIT" );
    case Types.BOOLEAN:		return( "BOOLEAN" );
    case Types.TINYINT:		return( "TINYINT" );
    case Types.SMALLINT:    	return( "SMALLINT" );
    case Types.INTEGER:		return( "INTEGER" );
    case Types.BIGINT:		return( "BIGINT" );
    case Types.FLOAT:		return( "FLOAT" );
    case Types.REAL:		return( "REAL" );
    case Types.DOUBLE:		return( "DOUBLE" );
    case Types.NUMERIC:		return( "NUMERIC" );
    case Types.DECIMAL:		return( "DECIMAL" );
    case Types.CHAR:		return( "CHAR" );
    case Types.VARCHAR:		return( "VARCHAR" );
    case Types.LONGVARCHAR:	return( "LONGVARCHAR" );
    case Types.NCHAR:		return( "NCHAR" );
    case Types.NVARCHAR:	return( "NVARCHAR" );
    case Types.LONGNVARCHAR:	return( "LONGNVARCHAR" );
    case Types.BINARY:		return( "BINARY" );
    case Types.VARBINARY:	return( "VARBINARY" );
    case Types.LONGVARBINARY:	return( "LONGVARBINARY" );
    case Types.DATE:		return( "DATE" );
    case Types.TIME:		return( "TIME" );
    case Types.TIMESTAMP:	return( "TIMESTAMP" );
    case Types.ARRAY:		return( "ARRAY" );
    case Types.BLOB:		return( "BLOB" );
    case Types.CLOB:		return( "CLOB" );
    case Types.NCLOB:		return( "NCLOB" );
    case Types.REF:		return( "REF" );
    case Types.DISTINCT:	return( "DISTINCT" );
    case Types.STRUCT:		return( "STRUCT" );
    case Types.JAVA_OBJECT:	return( "JAVA_OBJECT" );
    case Types.DATALINK:	return( "DATALINK" );
    case Types.ROWID:		return( "ROWID" );
    case Types.SQLXML:		return( "SQLXML" );
    case Types.OTHER:		return( "OTHER");

    default:
	if ( trace.enabled() )
	    trace.log( title + ": Unsupported SQL type " + type );
	throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
    }
} // getStringType

/*
** Name: getRowIdLifetime 
**
** Description:
**     Returns ROWID_UNSUPPORTED. 
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	RowIdLifetime (Enum type). 
**
** History:
**	08-Apr-09 (rajus01) SIR 121238 
**          Implemented.
**	22-Jun-09 (rajus01) 
**	    Include the return value in the trace.
*/
public RowIdLifetime 
getRowIdLifetime()
throws SQLException
{
    if ( trace.enabled() )
        trace.log( title + ".getRowIdLifetime(): "  + 
				RowIdLifetime.ROWID_UNSUPPORTED );
    return( RowIdLifetime.ROWID_UNSUPPORTED );
} // getRowIdLifeTime

/*
** Name: supportsStoredFunctionUsingCallSyntax 
**
** Description:
**     Returns true as user-defined or Ingres functions can be called 
**     using the stored procedure escape syntax. 
**     Ex: prepareCall( "{ call sp1 }" ) or 
**         prepareCall( "{ ? = call sp2( ? ) }" ) or
**         prepareCall( "{ call sp3 ( ? ? ? ) }" )
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean  (true). 
**
** History:
**	08-Apr-09 (rajus01) SIR 121238 
**      Implemented.
*/
public boolean 
supportsStoredFunctionsUsingCallSyntax()
throws SQLException
{
    if ( trace.enabled() )
        trace.log( title + ".supportsStoredFunctionsUsingCallSyntax: " );
    return( false );
}

/*
** Name: autoCommitFailureClosesAllResultSets 
**
** Description:
**    Returns false indicating that a SQLException while autoCommit is true 
**    does not result in closure of all open ResultSets, even ones that 
**    are holdable.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean - false. 
**
** History:
**	08-Apr-09 (rajus01) SIR 121238 
**      Implemented.
*/
public boolean 
autoCommitFailureClosesAllResultSets()
throws SQLException
{
    if ( trace.enabled() )
        trace.log( title + ".autoCommitFailureClosesAllResultSets: " );
    return( false );
} //autoCommitFailureClosesAllResultSets

/*
** Name: getClientInfoProperties
**
** Description:
**    Retrieves a list of the client info properties that the driver supports.
**    The resultset contains the following columns
**
**   1. NAME String=> The name of the client info property
**   2. MAX_LEN int=> The maximum length of the value for the property
**   3. DEFAULT_VALUE String=> The default value of the property
**   4. DESCRIPTION String=> A description of the property. This will 
**                           typically contain information as to where 
**                           this property is stored in the database. 
**
**	The ResultSet is sorted by the NAME column 
**
** Input:
**      None.
**
** Output:
**      None.
**
** Returns:
**      ResultSet       // empty result.
**
** History:
**      04-Feb-09 (rajus01) SIR 121238
**          Implemented.
*/
private static  MDdesc clinfo_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "name"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "max_len"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "default_value"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)256, "description"),
};

public ResultSet 
getClientInfoProperties()
throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getClientInfoProperties()" );

    JdbcRSMD    rsmd = new JdbcRSMD( (short)clinfo_desc.length, trace );

    for( int i = 0; i < clinfo_desc.length; i++ )
        rsmd.setColumnInfo( clinfo_desc[i].col_name, i+1,
                            clinfo_desc[i].sql_type, clinfo_desc[i].dbms_type,
                            clinfo_desc[i].length, (byte)0, (byte)0, false );

    return( new RsltData( conn, rsmd, null ) );
}

/*
** Name: getFunctions
**
** Description:
**	Gets a description of system and user functions available in a catalog.
**
**	Only functions descriptions matching the schema and function
**	name criteria are returned. They are ordered by FUNCTIONS_SCHEM,
**	and FUNCTION_NAME.
**
**	A "" value for schemaPatten in the query will always return 0 rows
**	because all procedures have owner_name on it. Hence an input value
** 	of "" for schemaPattern is ignored. catalog name is ignored.
**
** Input:
**	catalog			// Catalog name
**	schemaPattern		// Schema name pattern
**	functionNamePattern	// Function name pattern 
**
** Output:
**	None.
**
** Returns:
**	ResultSet 		// Empty resultset
**
** History:
**	08-Apr-09 (rajus01) SIR 121328
**          Implemented.
*/

private static  MDdesc func_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "function_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "function_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32,  "function_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)128, "remarks"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2,  "function_type")
};

public ResultSet 
getFunctions
(
    String catalog,
    String schemaPattern,
    String functionNamePattern
)
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getFunctions()" );

    JdbcRSMD    rsmd = new JdbcRSMD( (short)func_desc.length, trace );
    for( int i = 0; i < func_desc.length; i++ )
        rsmd.setColumnInfo( func_desc[i].col_name, i+1, 
                            func_desc[i].sql_type,
                            func_desc[i].dbms_type, func_desc[i].length,
                            (byte)0, (byte)0, false );

    return( new RsltData( conn, rsmd, null ) );

} //getFunctions

/*
** Name: getFunctionColumns
**
** Description:
**	Get a description of  catalog's stored procedure parameters
**	and result columns.
**	Only descriptions matching the schema, procedure and parameter
**	name criteria are returned. They are ordered by FUNCTION_SCHEM
**	and FUNCTION_NAME. 
**	A "" value for schemaPatten in the query will always return 0 rows
**	because all procedures have owner_name on it. Hence an input value
** 	of "" for schemaPattern is ignored. catalog name is ignored.
**
** Input:
**	catalog			// Catalog name
**	schemaPattern		// Schema name pattern
**	functionNamePattern	// Function name pattern 
**	columnNamePattern	// Column name pattern
**
** Output:
**	None.
**
** Returns:
**	ResultSet 		// Empty Resultset
**
** History:
**	10-Apr-09 (rajus01)
**	    Implemented.
*/

private static  MDdesc funcCol_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "function_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "function_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "function_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "column_type"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "data_type"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "precision"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "length"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "scale"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "radix"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "nullable"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "remarks")
};

public ResultSet 
getFunctionColumns
(
    String catalog,
    String schemaPattern,
    String functionNamePattern,
    String columnNamePattern
)
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getFunctionColumns()" );

    JdbcRSMD    rsmd = new JdbcRSMD( (short)funcCol_desc.length, trace );
    for( int i = 0; i < funcCol_desc.length; i++ )
        rsmd.setColumnInfo( funcCol_desc[i].col_name, i+1, 
                            funcCol_desc[i].sql_type,
                            funcCol_desc[i].dbms_type, funcCol_desc[i].length,
                            (byte)0, (byte)0, false );

    return( new RsltData( conn, rsmd, null ) );
} // getFunctionColumns

/*
** Name: isWrapperFor
**
** Description:
**      Returns true if this either implements the interface argument or is
**      directly or indirectly a wrapper for an object that does. Returns
**      false otherwise.
**
** Input:
**      iface   Class defining an interface
**
** Output:
**      None.
**
** Returns:
**      true or false.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public boolean 
isWrapperFor(Class<?> iface)
throws SQLException
{
    if ( trace.enabled() )
        trace.log(title + ".isWrapperFor(" + iface + ")");
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
	return( iface.isInstance( this ) );

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);
}

/*
** Name: unwrap
**
** Description:
**      Returns an object that implements the given interface to allow
**      access to non-standard methods, or standard methods not exposed by the
**      proxy. If the receiver implements the interface then the result is the
**      receiver or a proxy for the receiver. If the receiver is a wrapper and
**      the wrapped object implements the interface then the result is the
**      wrapped object or a proxy for the wrapped object. Otherwise return the
**      the result of calling unwrap recursively on the wrapped object or
**      a proxy for that result. If the receiver is not a wrapper and does not
**      implement the interface, then an SQLException is thrown.
**
** Input:
**      iface   A Class defining an interface that the result must implement.
**
** Output:
**      None.
**
** Returns:
**      An object that implements the interface.
**
** History:
**      23-Jun-09 (rajus01)
**         Implemented.
*/
public <T> T 
unwrap(Class<T> iface)
throws SQLException
{
    if ( trace.enabled() )
        trace.log(title + ".Unwrap(" + iface + ")");
    /*
    ** This approach seems to work for classes that actually don't
    ** wrap anything.
    */
    if( iface != null )
    {
	if( ! iface.isInstance( this ) )
	    throw SqlExFactory.get(ERR_GC4023_NO_OBJECT); 
	return( iface.cast(this) );
    }

    throw SqlExFactory.get(ERR_GC4010_PARAM_VALUE);
}

/*
** Name: convToJavaType
**
** Description:
**	Convert a DBMS datatype (from the DBMS result set)
**	to a Java datatype 
**
** Input:
**	ingType	    DBMS datatype.
**	len         column_length
**
** Output:
**	None.
**
** Returns:
** 	int	    Java Type	
**
** History:
**	22-Dec-99 (rajus01)
**	    Created.
**	30-Sep-03 (weife01)
**	    BUG 111022 - Add unicode data type NCHAR, NVARCHAR, 
**	    LONG NVARCHAR map from DBMS to JAVA type CHAR, 
**	    VARCHAR, LONG VARCHAR
**	15-Mar-04 (gordy)
**	    Support BIGINT.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	22-Apr-09 (rajus01) SIR 121238
**	    DBMS types NCHAR, NVARCHAR, LONGNVARCHAR maps to the equivalent
**	    JDBC types instead of CHAR, VARCHAR and LONGVARCHAR types.
**	26-Oct-09 (gordy)
**	    Added BOOLEAN type.
*/

private static int 
convToJavaType( short ingType, int len ) 
{
    switch( ingType )
    {
    case DBMS_TYPE_BOOL:		return( Types.BOOLEAN );
    case DBMS_TYPE_IDATE:		return( Types.TIMESTAMP );
    case DBMS_TYPE_ADATE:		return( Types.DATE );
    case DBMS_TYPE_MONEY:		return( Types.DECIMAL );
    case DBMS_TYPE_TMWO:		return( Types.TIME );
    case DBMS_TYPE_TMTZ:		return( Types.TIME );
    case DBMS_TYPE_TIME:		return( Types.TIME );
    case DBMS_TYPE_TSWO:		return( Types.TIMESTAMP );
    case DBMS_TYPE_DECIMAL:		return( Types.DECIMAL );
    case DBMS_TYPE_TSTZ:		return( Types.TIMESTAMP );
    case DBMS_TYPE_TS:			return( Types.TIMESTAMP );
    case DBMS_TYPE_CHAR:		return( Types.CHAR );
    case DBMS_TYPE_NCHAR:		return( Types.NCHAR );
    case DBMS_TYPE_VARCHAR:		return( Types.VARCHAR );
    case DBMS_TYPE_NVARCHAR:		return( Types.NVARCHAR );
    case DBMS_TYPE_LONG_CHAR:		return( Types.LONGVARCHAR );
    case DBMS_TYPE_LONG_NCHAR:		return( Types.LONGNVARCHAR );
    case DBMS_TYPE_BYTE:		return( Types.BINARY );
    case DBMS_TYPE_VARBYTE:		return( Types.VARBINARY );
    case DBMS_TYPE_LONG_BYTE:		return( Types.LONGVARBINARY );
    case DBMS_TYPE_INT:
	switch( len )
	{
	case 1:				return( Types.TINYINT );
	case 2:				return( Types.SMALLINT );
	case 4:				return( Types.INTEGER );
	case 8:				return( Types.BIGINT );
	default:			return( Types.NULL );
	}
    case DBMS_TYPE_FLOAT:
	switch( len )
	{
	case 4:				return( Types.REAL );
	case 8:				return( Types.DOUBLE );
	default:			return( Types.NULL );
	}
    case DBMS_TYPE_C:			return( Types.CHAR );
    case DBMS_TYPE_INTYM:		return( Types.VARCHAR );
    case DBMS_TYPE_INTDS:		return( Types.VARCHAR );
    case DBMS_TYPE_TEXT:		return( Types.VARCHAR );
    case DBMS_TYPE_LONG_TEXT:		return( Types.VARCHAR );
    default:				return( Types.NULL );
    }
} // convToJavaType


/*
** Name: colSize
**
** Description:
**	Determines COLUMN_SIZE DatabaseMetaData  value. 
**
** Input:
**	javaType	 Java data type.
**	dbmsType	 DBMS data type.
**
** Output:
**	None.
**
** Returns:
**	int.
**
** History:
**	22-Dec-99 (rajus01)
**	    Created.
**	15-Mar-04 (gordy)
**	    Support BIGINT.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	26-Feb-07 (gordy)
**	    Support locators (BLOB/CLOB).
**	14-Feb-08 (rajus01) Bug 119919
**	    Added DBMS type as input parameter.	    
**	02-Jun-08 (rajus01) SD issue 128649, Bug 120439.
**	    The column_size (binary precision - number of bits in
**	    the mantissa) for real and double data types must match the 
**	    values returned by getTypeInfo(). Based on the Ingres doc 
**	    the maximum value for this is 23 for 4 byte float which equates to
**	    7 decimal digits and 53 for 8 byte float which equates to
**	    16 decimal digits, but Ingres rounds it to 15 decimal digits. 
**	04-Jun-08 (rajus01) SD issue 128696, Bug 120463
**	    The precision values of DATE/TIME/TIMESTAMP datatypes are fixed.
**	26-Oct-09 (gordy)
**	    Added BOOLEAN type.
*/

private static int 
colSize(int dbmsType, int javaType )
    throws SQLException
{
    switch( javaType )
    {
    case Types.DECIMAL:
	switch( dbmsType )
	{
	case DBMS_TYPE_MONEY:	return( DBMS_MONEY_PRECISION );
	default:		return(-1);
	}
    case Types.CHAR:
    case Types.VARCHAR:
    case Types.BINARY:
    case Types.VARBINARY:	return( -1 );
    case Types.DATE:		return( 10 );
    case Types.TIME:		return( 8 );
    case Types.TIMESTAMP:	return( 29 );
    case Types.TINYINT:		return( 4 );
    case Types.SMALLINT:	return( 6 );
    case Types.INTEGER:		return( 11 );
    case Types.BIGINT:		return( 20 );
    case Types.REAL:		return( 7 );
    case Types.DOUBLE:		return( 15 );
    case Types.BOOLEAN:
    case Types.BLOB:
    case Types.CLOB:
    case Types.LONGVARCHAR:
    case Types.LONGVARBINARY:	return( 0 );
    default:			return( -1 );
    }
} // colSize


/*
** Name: MDdesc
**
** Description:
**	Describes a single column in a MetaData result-set.
**
** History:
**	16-Nov-99 (rajus01)
**	    Created.
**	15-Mar-00 (rajus01)
**	    Constructor is public.
*/

private static class 
MDdesc
{
    public String	col_name = null;
    public int		sql_type = Types.OTHER;
    public short	dbms_type = 0;
    public short	length = 0;


public		// Constructor
MDdesc( int stype, short dtype, short len, String name )
{
    sql_type = stype;
    dbms_type = dtype;
    length = len;
    col_name = name;
    return;
}


} // MDdesc

} // JdbcDBMD

