/*
** Copyright (c) 2004 Ingres Corporation
*/

package	ca.edbc.jdbc;

/*
** Name: EdbcMetaData.java
**
** Description:
**	Implements the EDBC JDBC DatabaseMetaData class: EdbcMetaData.
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
**  30-Sep-03 (weife01)
**      BUG 111022 - Add unicode data type NCHAR, NVARCHAR, LONG NVARCHAR
**      map from DBMS to JAVA type CHAR, VARCHAR, LONG VARCHAR
**  12-Apr-05 (weife01)
**      Bug 114211 to add param_sequence in select and order by list in
**      getProcedureColumns().
**  03-Aug-05 (rajus01) StarIssue:14303664;Bug# 114982;Prob# EDJDBC105
**	Added "table_name not like 'ii%'" to the where clause to suppress
**	the displaying of iietab_xx_yy tables. These extension tables  
**	get created when user tables have blob columns (LONG VARCHAR/LONG
**	BYTE), and the actual blob data is stored in these extension tables
**	where xx and yy are numbers chosen by the DBMS. These tables are 
**	system catalogs in essence, eventhough the system_use flag is set 
**	to 'U'. Users cannot create tables named ii*.
*/

import	java.util.Enumeration;
import	java.sql.*;
import	ca.edbc.util.DbmsConst;
import	ca.edbc.util.EdbcEx;
import	ca.edbc.io.DbConn;
import  ca.edbc.util.IdMap;


/*
** Name: EdbcMetaData
**
** Description:
**	EDBC JDBC Java driver class which implements the
**	JDBC DatabaseMetaData interface.
**
**  Interface Methods:
**
**	allProceduresAreCallable
**	allTablesAreSelectable
**	getURL
**	getUserName
**	isReadOnly
**	nullsAreSortedHigh
**	nullsAreSortedLow
**	nullsAreSortedAtStart
**	nullsAreSortedAtEnd
**	getDatabaseProductName
**	getDatabaseProductVersion
**	getDriverName
**	getDriverVersion
**	getDriverMajorVersion
**	getDriverMinorVersion
**	usesLocalFiles
**	usesLocalFilePerTable
**	supportsMixedCaseIdentifiers
**	storesUpperCaseIdentifiers
**	storesLowerCaseIdentifiers
**	storesMixedCaseIdentifiers
**	supportsMixedCaseQuotedIdentifiers
**	storesUpperCaseQuotedIdentifiers
**	storesLowerCaseQuotedIdentifiers
**	storesMixedCaseQuotedIdentifiers
**	getIdentifierQuoteString
**	getSQLKeywords
**	getNumericFunctions
**	getStringFunctions
**	getSystemFunctions
**	getTimeDateFunctions
**	getSearchStringEscape
**	getExtraNameCharacters
**	supportsAlterTableWithAddColumn
**	supportsAlterTableWithDropColumn
**	supportsColumnAliasing
**	nullPlusNonNullIsNull
**	supportsConvert
**	supportsConvert
**	supportsTableCorrelationNames
**	supportsDifferentTableCorrelationNames
**	supportsExpressionsInOrderBy
**	supportsOrderByUnrelated
**	supportsGroupBy
**	supportsGroupByUnrelated
**	supportsGroupByBeyondSelect
**	supportsLikeEscapeClause
**	supportsMultipleResultSets
**	supportsMultipleTransactions
**	supportsNonNullableColumns
**	supportsMinimumSQLGrammar
**	supportsCoreSQLGrammar
**	supportsExtendedSQLGrammar
**	supportsANSI92EntryLevelSQL
**	supportsANSI92IntermediateSQL
**	supportsANSI92FullSQL
**	supportsIntegrityEnhancementFacility
**	supportsOuterJoins
**	supportsFullOuterJoins
**	supportsLimitedOuterJoins
**	getSchemaTerm
**	getProcedureTerm
**	getCatalogTerm
**	isCatalogAtStart
**	getCatalogSeparator
**	supportsSchemasInDataManipulation
**	supportsSchemasInProcedureCalls
**	supportsSchemasInTableDefinitions
**	supportsSchemasInIndexDefinitions
**	supportsSchemasInPrivilegeDefinitions
**	supportsCatalogsInDataManipulation
**	supportsCatalogsInProcedureCalls
**	supportsCatalogsInTableDefinitions
**	supportsCatalogsInIndexDefinitions
**	supportsCatalogsInPrivilegeDefinitions
**	supportsPositionedDelete
**	supportsPositionedUpdate
**	supportsSelectForUpdate
**	supportsStoredProcedures
**	supportsSubqueriesInComparisons
**	supportsSubqueriesInExists
**	supportsSubqueriesInIns
**	supportsSubqueriesInQuantifieds
**	supportsCorrelatedSubqueries
**	supportsUnion
**	supportsUnionAll
**	supportsOpenCursorsAcrossCommit
**	supportsOpenCursorsAcrossRollback
**	supportsOpenStatementsAcrossCommit
**	supportsOpenStatementsAcrossRollback
**	getMaxBinaryLiteralLength
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
**	getMaxSchemaNameLength
**	getMaxProcedureNameLength
**	getMaxCatalogNameLength
**	getMaxRowSize
**	doesMaxRowSizeIncludeBlobs
**	getMaxStatementLength
**	getMaxStatements
**	getMaxTableNameLength
**	getMaxTablesInSelect
**	getMaxUserNameLength
**	getDefaultTransactionIsolation
**	supportsTransactions
**	supportsTransactionIsolationLevel
**	supportsDataDefinitionAndDataManipulationTransactions
**	supportsDataManipulationTransactionsOnly
**	dataDefinitionCausesTransactionCommit
**	dataDefinitionIgnoredInTransactions
**	getProcedures
**	getProcedureColumns
**	getTables
**	getSchemas
**	getCatalogs
**	getTableTypes
**	getColumns
**	getColumnPrivileges
**	getTablePrivileges
**	getBestRowIdentifier
**	getVersionColumns
**	getPrimaryKeys
**	getImportedKeys
**	getExportedKeys
**	getCrossReference
**	getTypeInfo
**	getIndexInfo
**	supportsResultSetType
**	supportsResultSetConcurrency
**	ownUpdatesAreVisible
**	ownDeletesAreVisible
**	ownInsertsAreVisible
**	othersUpdatesAreVisible
**	othersDeletesAreVisible
**	othersInsertsAreVisible
**	updatesAreDetected
**	deletesAreDetected
**	insertsAreDetected
**	supportsBatchUpdates
**	getUDTs
**	getConnection.
**
**  Constants:
**
**	OI_CAT_LVL		Standard catalog level for OpenIngres.
**	ING64_NAME_LEN		Length of catalog names in Ingres 6.4.
**	nullable		Short object, value typeNullable.
**	searchB			Short object, value typePredBasic.
**	search			Short object, value typeSearchable.
**	searchN			Short object, value typePredNone.
**	I0			Integer object, value 0.
**	I2000			Integer object, value 2000.
**	I10			Integer object, value 10.
**	S1			Short object, value 1.
**	S0			Short object, value 0.
**	VARCHAR			DBMS VARCHAR numeric type.
**	INTEGER			DBMS INTEGER numeric type.
**
**  Local Data:
**
**	title			Class title for tracing.
**	conn			Database connection.
**	trace			Tracing.
**	upper			Are identifiers stored in upper case.
**	std_catl_lvl		Standard catalog level.
**	param_type		VARCHAR or OTHER (UCS2).
**
**  Private Methods:
**
** 	getStringType 	
** 	getMaxCol	
**	makePattern
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
*/

class
EdbcMetaData
    implements DatabaseMetaData, DbmsConst, EdbcConst, EdbcErr
{

    private static final String	title = shortTitle + "-DBMD";
    private EdbcConnect	 	conn = null;
    private EdbcTrace		trace = null;

    /*
    ** DBMS configuration settings.
    */
    private boolean		upper = false;	    // Upper case?
    private int			std_catl_lvl = 0;   // Standard catalog level.
    private int			param_type = Types.VARCHAR;	// Char type.

    /*
    ** Constants
    */
    private final static int	OI_CAT_LVL = 605;
    private final static int  	ING64_NAME_LEN = 24;

    /*
    ** getTypeInfo constants..
    */
    private final static Short nullable	= new Short((short)typeNullable);
    private final static Short searchB	= new Short((short)typePredBasic);
    private final static Short search	= new Short ((short)typeSearchable);
    private final static Short searchN	= new Short((short)typePredNone);

    private final static Integer I0		= new Integer(0);
    private final static Integer I2000		= new Integer(2000);
    private final static Integer I10		= new Integer(10);
    private final static Short   S1		= new Short((short)1);
    private final static Short	 S0		= new Short((short)0);

    private final static short VARCHAR  = DBMS_TYPE_VARCHAR;
    private final static short INTEGER  = DBMS_TYPE_INT;


/*
** Name: EdbcMetaData
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
*/

public
EdbcMetaData( EdbcConnect conn )
    throws SQLException
{
    this.conn = conn;
    this.trace = conn.trace;

    /*
    ** Determine DBMS configuration.  These settings are used
    ** for building queries for other meta-data requests.  DBMS
    ** configuration settings which are determine the result of
    ** a meta-data requests are retrieved during the request.
    */
    String cat_lvl = conn.getDbmsInfo("STANDARD_CATALOG_LEVEL");
    if ( cat_lvl != null )
	try { std_catl_lvl = Integer.parseInt( cat_lvl ); }
	catch( Exception ignore ) {};
    
    String  db_name_case = conn.getDbmsInfo("DB_NAME_CASE");
    if ( db_name_case != null  &&  db_name_case.equals("UPPER") )
	upper = true;

    /*
    ** UCS2 databases still use CHAR and VARCHAR for standard
    ** catalogs, so we must force non-UCS2 string parameters
    ** by using setObject() and the OTHER type recognized by 
    ** the driver as an alternate parameter format.
    */
    if ( conn.dbc.ucs2_supported )  param_type = Types.OTHER;
}


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
}

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
}

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
    String url = protocol + "//" + conn.host + "/" + conn.database;
    if ( trace.enabled() )  trace.log( title + ".getURL(): " + url );
    return( url );
}

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
*/

public String 
getUserName() 
    throws SQLException
{

    ResultSet	rs = null;
    String	user = null;
    String 	qry_text = "select user_name from iidbconstants for readonly";

    if ( trace.enabled() )  trace.log( title + ".getUserName()" );

    try
    {
        Statement stmt = conn.createStatement();
        rs = stmt.executeQuery( qry_text );
        if( rs.next() ) user = (rs.getString(1)).trim();
	rs.close();
        stmt.close();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getUsername(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }
    

    if ( trace.enabled() )  trace.log( title + ".getUserName(): " + user );

    return( user );
}

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
    boolean ro = conn.isReadOnly();
    if ( trace.enabled() )  trace.log( title + ".isReadOnly(): " + ro );
    return( ro );
} // isReadOnly

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

    String sort  = conn.getDbmsInfo("NULL_SORTING");

    if( sort != null && !sort.equals("HIGH") )
	result = false;

    if ( trace.enabled() )  
	trace.log( title + ".nullsAreSortedHigh(): " + result );
    return( result );
}

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

    String sort  = conn.getDbmsInfo("NULL_SORTING");

    if( sort != null && sort.equals("LOW") )
	result = true;

    if ( trace.enabled() )  
	trace.log( title + ".nullsAreSortedLow(): " + result );
    return( result );
}

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

    String sort  = conn.getDbmsInfo("NULL_SORTING");

    if( sort != null && sort.equals("FIRST") )
	result = true;
    if ( trace.enabled() )  
	trace.log( title + ".nullsAreSortedAtStart(): " + result );
    return( result );
}

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

    String sort  = conn.getDbmsInfo("NULL_SORTING");

    if( sort != null && sort.equals("LAST") )
	result = true;

    if ( trace.enabled() )  
	trace.log( title + ".nullsAreSortedAtEnd(): " + result );
    return( result );
}

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
    String 	dbmsType = null;
    dbmsType = conn.getDbmsInfo("DBMS_TYPE");

    if ( trace.enabled() )
	trace.log( title + ".getDatabaseProductName(): " + dbmsType );
    return( dbmsType );
}

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
    String version = conn.getDbmsInfo( "_version", false ); 
    if ( trace.enabled() ) 
	trace.log( title + ".getDatabaseProductVersion(): " + version );
    return( version );
}

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
	trace.log( title + ".getDriverName(): " + fullTitle );
    return( fullTitle );
}

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
*/

public String 
getDriverVersion() 
    throws SQLException
{
    String version = majorVersion + "." + minorVersion;
    if ( trace.enabled() )  
	trace.log( title + ".getDriverVersion(): " + version );
    return( version );
}

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
	trace.log( title + ".getDriverMajorVersion(): " + majorVersion );
    return( majorVersion );
}

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
	trace.log( title + ".getDriverMinorVersion(): " + minorVersion );
    return( minorVersion );
}

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
}

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
}

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
    String  db_name_case = conn.getDbmsInfo( "DB_NAME_CASE" );

    if ( db_name_case != null  &&  db_name_case.equals( "MIXED" ) )
	mixed = true;

    if ( trace.enabled() )
	trace.log( title + ".supportsMixedCaseIdentifiers(): " + mixed );
    
    return( mixed );
}

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
}

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
    String  db_name_case = conn.getDbmsInfo( "DB_NAME_CASE" );

    if ( db_name_case != null  &&  ! db_name_case.equals( "LOWER" ) )
	lower = false;
    
    if ( trace.enabled() ) 
	trace.log( title + ".storesLowerCaseIdentifiers(): " + lower );
    
    return( lower );
}

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
}

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
    String  db_delim_case = conn.getDbmsInfo( "DB_DELIMITED_CASE" );

    if ( db_delim_case != null  &&  db_delim_case.equals( "MIXED" ) )
	mixed = true;
    
    if ( trace.enabled() ) 
	trace.log( title + ".supportsMixedCaseQuotedIdentifiers(): " + mixed );
    
    return( mixed );
}

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
    String  db_delim_case = conn.getDbmsInfo( "DB_DELIMITED_CASE" );

    if ( db_delim_case != null  &&  db_delim_case.equals( "UPPER" ) )
	upper = true;

    if ( trace.enabled() ) 
	trace.log( title + ".storesUpperCaseQuotedIdentifiers(): " + upper );
    
    return( upper );
}

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
    String  db_delim_case = conn.getDbmsInfo( "DB_DELIMITED_CASE" );

    if ( db_delim_case != null  &&  db_delim_case.equals( "LOWER" ) )  
	lower = true;
    
    if ( trace.enabled() ) 
	trace.log( title + ".storesLowerCaseQuotedIdentifiers(): " + lower );
    
    return( lower );
}

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
}

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
    String  quote = (conn.getDbmsInfo( "DB_DELIMITED_CASE" ) != null) 
		    ? "\"" : " ";
    if ( trace.enabled() ) 
	trace.log( title + ".getIdentifierQuoteString(): '" + quote + "'" );
    return( quote );
}

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
}

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
    String str = EdbcSQL.getNumFuncs();
    if ( trace.enabled() ) 
	trace.log( title + ".getNumericFunctions(): " + str );
    return( str );
}

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
    String str = EdbcSQL.getStrFuncs();
    if ( trace.enabled() ) 
	trace.log( title + ".getStringFunctions(): " + str );
    return( str );
}

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
    String str = EdbcSQL.getSysFuncs();
    if ( trace.enabled() ) 
	trace.log( title + ".getSystemFunctions(): " + str );
    return( str );
}

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
    String str = EdbcSQL.getTdtFuncs();
    if ( trace.enabled() ) 
	trace.log( title + ".getTimeDateFunctions(): " + str );
    return( str );
}

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
*/

public String 
getSearchStringEscape() 
    throws SQLException
{
    String  result = "\\"; 

    String escape  = conn.getDbmsInfo("ESCAPE_CHAR");

    if ( escape != null )  result = escape;
    if ( trace.enabled() )  
	trace.log( title + ".getSearchStringEscape(): " + result);
    return( result );
}

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
    String ident_char  = conn.getDbmsInfo("IDENT_CHAR");
    if ( ident_char != null )  result = ident_char;
    if ( trace.enabled() )
	trace.log( title + ".getExtraNameCharacters(): " + result );
    return( result );
}

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
}

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
}

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
}

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
}

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
}

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
*/

public boolean 
supportsConvert( int fromType, int toType ) 
    throws SQLException
{
    String	fromStr = getStringType( fromType ); 
    String 	toStr   = getStringType( toType ); 
    boolean	match_found = false;
    boolean	convert = false;
    Enumeration	type_enum = EdbcSQL.getConvertTypes();

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

    if( match_found )
    {
	if( toType == fromType )
	    convert = true;	
	else if ( ( toType == Types.LONGVARBINARY ) ||
	    ( toType == Types.LONGVARCHAR ) )
	{
	    convert = false;
	}
	else if( ( toType == Types.BINARY ) ||
	    ( toType == Types.VARBINARY ) ||
	    ( toType == Types.CHAR ) ||
	    ( toType == Types.VARCHAR ) )
	{
	    if( !((fromType == Types.BIT) || (fromType == Types.BIGINT) ||
	       (fromType == Types.LONGVARCHAR && toType == Types.BINARY ) ||
	       (fromType == Types.LONGVARCHAR && toType == Types.VARBINARY )
	     ) )
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
		convert = true; // fromType should be one of the above.
	} else if( toType == Types.TIMESTAMP )
	{
	    if( (fromType == Types.CHAR ) || ( fromType == Types.VARCHAR ) ||
		( fromType == Types.DATE ) || ( fromType == Types.TIME ) 
	      )
		convert = true; // fromType should be one of the above.
	}
    }

    if ( trace.enabled() )
	trace.log( title + ".supportsConvert( " +
		   fromType + ", " + toType + " ): " + convert );

    return( convert );
}

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
}

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
}

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
}

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
}

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
}

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
}

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
}

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
*/

public boolean 
supportsLikeEscapeClause() 
    throws SQLException
{
    boolean result = true;

    String esc = conn.getDbmsInfo("ESCAPE");

    if( esc != null && esc.equals("N") )
	result = false;

    if ( trace.enabled() )  
	trace.log( title + ".supportsLikeEscapeClause(): " + result);
    return( result );
}

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
}

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
}

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
}

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
}

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
}

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
}

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
    String level = conn.getDbmsInfo("SQL92_LEVEL");
    if ( level != null && level.equals("NONE") )  entryLvl = false;

    if ( trace.enabled() ) 
	trace.log( title + ".supportsANSI92EntryLevelSQL(): " + entryLvl );
    return( entryLvl );
}

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

    String level = conn.getDbmsInfo("SQL92_LEVEL");

    if( level != null &&
	( level.equals("INTERMEDIATE") || level.equals("FULL") ) )
	result = true;

    if ( trace.enabled() )
	trace.log( title + ".supportsANSI92IntermediateSQL(): " + result );
    return( result );
}

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
    String level = conn.getDbmsInfo("SQL92_LEVEL");
    if( level != null && level.equals("FULL") )  result = true;
    if ( trace.enabled() )  
	trace.log( title + ".supportsANSI92FullSQL(): " + result );
    return( result );
}

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

    String level = conn.getDbmsInfo("SQL92_LEVEL");

    if( level != null && !level.equals("FIPS-IEF") &&
	!level.equals("INTERMEDIATE") && !level.equals("FULL") )
	result = false;

    if ( trace.enabled() )
	trace.log( title + ".supportsIntegrityEnhancementFacility(): " + result );

    return( result );
}

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
    String join = conn.getDbmsInfo("OUTER_JOIN");
    if( join != null && join.equals("N") )  result = false;
    if ( trace.enabled() )  
	trace.log( title + ".supportsOuterJoins(): " + result );
    return( result );
}

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
}

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
}

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
}

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
}

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
    if ( trace.enabled() )  trace.log( title + ".getCatalogTerm(): " + "" );
    return("");
}

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
**  09-Jan-07 (weife01)
**      SIR 117451. Add support for catalog instead of throw SQLException.
*/

public boolean 
isCatalogAtStart() 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".isCatalogAtStart(): " + false );
    return false;
}

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
**  09-Jan-07 (weife01)
**      SIR 117451. Add support for catalog instead of throw SQLException.
*/

public String 
getCatalogSeparator() 
    throws SQLException
{
    if ( trace.enabled() )  
	trace.log( title + ".getCatalogSeparator(): " + "" );
    return("");
}

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
    String owner = conn.getDbmsInfo("OWNER_NAME");
    if ( owner != null && owner.equals("NO") )  result = false;
    if ( trace.enabled() ) 
	trace.log( title + ".supportsSchemasInDataManipulation(): " + result );
    return( result );
}

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
    String owner = conn.getDbmsInfo("OWNER_NAME");
    if ( owner != null && owner.equals("NO") )  result = false;
    if ( trace.enabled() ) 
	trace.log( title + ".supportsSchemasInProcedureCalls(): " + result );
    return( result );
}

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
    String owner = conn.getDbmsInfo("OWNER_NAME");
    if ( owner != null && owner.equals("NO") )  result = false;
    if ( trace.enabled() )
	trace.log( title + ".supportsSchemasInTableDefinitions(): " + result );
    return( result );
}

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
    String owner = conn.getDbmsInfo("OWNER_NAME");
    if ( owner != null && owner.equals("NO") )  result = false;
    if ( trace.enabled() )  
	trace.log( title + ".supportsSchemasInIndexDefinitions(): " + result );
    return( result );
}

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
    String owner = conn.getDbmsInfo("OWNER_NAME");
    if ( owner != null && owner.equals("NO") )  result = false;
    if ( trace.enabled() )
	trace.log( title + ".supportsSchemasInPrivilegeDefinitions(): " + result);
    return( result );
}

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
}

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
}

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
}

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
}

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
}

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
}

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
}

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
}

/*
** Name: supportsStoredProcedures
**
** Description:
**	Are stored procedure calls using the stored procedure escape
**	syntax supported? Currently NOT supported.
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
	trace.log( title + ".supportsStoredProcedures(): " + false );
    return( true );
}

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
}

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
}

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
}

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
}

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
}

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

    String union = conn.getDbmsInfo("UNION");

    if( union != null && union.equals("N") )
	result = false;

    if ( trace.enabled() )  trace.log( title + ".supportsUnion(): " + result );
    return( result );
}

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
    String union = conn.getDbmsInfo("UNION");

    if( union != null && !union.equals("ALL") )
	result = false;
    if ( trace.enabled() )  
	trace.log( title + ".supportsUnionAll(): " + result );
    return( result );
}

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
}

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
}

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
}

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
}

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

    String max_len = conn.getDbmsInfo("SQL_MAX_BYTE_LITERAL_LEN");

    if( max_len != null )
	try { result = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if( result < 0 ) result = 0;

    if ( trace.enabled() )  
	trace.log( title + ".getMaxBinaryLiteralLength(): " + result );
    return( result );
}

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

    String max_len = conn.getDbmsInfo("SQL_MAX_CHAR_LITERAL_LEN");

    if( max_len != null )
	try { result = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};
    if( result < 0 ) result = 0;
    if ( trace.enabled() )  
	trace.log( title + ".getMaxCharLiteralLength(): " + result );
    return( result );
}

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
    int len = DBMS_NAME_LEN;    // OpenIngres, Ingres II etc., 

    if( std_catl_lvl < OI_CAT_LVL )
	len = ING64_NAME_LEN;

    String max_len = conn.getDbmsInfo("SQL_MAX_COLUMN_NAME_LEN");

    if( max_len != null )
	try { len = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if ( trace.enabled() ) 
	trace.log( title + ".getMaxColumnNameLength(): " + len );
    return( len );
}

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
}

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
*/

public int 
getMaxColumnsInIndex() 
    throws SQLException
{
    int max_col = getMaxCol();
    if ( trace.enabled() )
	trace.log( title + ".getMaxColumnsInIndex(): " + max_col );
    return( max_col );
}

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
}

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
}

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
*/

public int 
getMaxColumnsInTable() 
    throws SQLException
{
    int max_col = getMaxCol();
    if ( trace.enabled() )  
	trace.log( title + ".getMaxColumnsInTable(): " + max_col );
    return( max_col );
}

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
}

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
	trace.log( title + ".getMaxCursorNameLength(): " + DBMS_NAME_LEN );
    return( DBMS_NAME_LEN );
}

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
}

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
    int len = DBMS_NAME_LEN;    // OpenIngres, Ingres II etc.. 

    if( std_catl_lvl < OI_CAT_LVL )
	len = ING64_NAME_LEN;

    String max_len = conn.getDbmsInfo("SQL_MAX_SCHEMA_NAME_LEN");

    if( max_len != null )
	try { len = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if ( trace.enabled() )  
    	trace.log( title + ".getMaxSchemaNameLength(): " + len );
    return( len );
}

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
*/

public int 
getMaxProcedureNameLength() 
    throws SQLException
{
    int len = DBMS_NAME_LEN;    // OpenIngres, Ingres II etc.. 

    if( std_catl_lvl < OI_CAT_LVL )
	len = ING64_NAME_LEN;

    if ( trace.enabled() )  
	trace.log( title + ".getMaxProcedureNameLength(): " + len );
    return( len );
}

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
**  09-Jan-07 (weife01)
**      SIR 117451. Add support for catalog instead of throw SQLException.
*/

public int 
getMaxCatalogNameLength() 
    throws SQLException
{
    int dont_know = 0;
    if ( trace.enabled() )  
	trace.log( title + ".getMaxCatalogNameLength(): " + dont_know );
    return 0;
}

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

    String max_len = conn.getDbmsInfo("SQL_MAX_ROW_LEN");

    if( max_len != null )
	try { result = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};
    if( result < 0 )  result = 0;

    if ( trace.enabled() )  trace.log( title + ".getMaxRowSize(): " + result );
    return( result );
}

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
}

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
}

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

    String max_len = conn.getDbmsInfo("SQL_MAX_STATEMENTS");

    if( max_len != null )
	try { dont_know = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if ( dont_know < 0 )  dont_know = 0;
    if ( trace.enabled() )  
	trace.log( title + ".getMaxStatements(): " + dont_know );
    return( dont_know );
}

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
    int len = DBMS_NAME_LEN;    // OpenIngres, Ingres II etc.. 

    if( std_catl_lvl < OI_CAT_LVL )
	len = ING64_NAME_LEN;

    String max_len = conn.getDbmsInfo("SQL_MAX_TABLE_NAME_LEN");
    if( max_len != null )
	try { len = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if ( trace.enabled() ) 
	trace.log( title + ".getMaxTableNameLength(): " + len );
    return( len );
}

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
}

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
    int len = DBMS_NAME_LEN;    // OpenIngres, Ingres II etc.. 

    if( std_catl_lvl < OI_CAT_LVL )
	len = ING64_NAME_LEN;

    String max_len = conn.getDbmsInfo("SQL_MAX_USER_NAME_LEN");

    if( max_len != null )
	try { len = Integer.parseInt( max_len ); }
	catch( Exception ignore ) {};

    if( len < 0 ) len = 0;
    if ( trace.enabled() )
	trace.log( title + ".getMaxUserNameLength(): " + len );
    return( len );
}

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
}

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
}

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
	if (std_catl_lvl < OI_CAT_LVL)
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
}

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
}

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
}

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
}

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
}

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
    new MDdesc(Types.SMALLINT, INTEGER, (short)2,  "procedure_type")
};

private static class
MDprocedureRS
    extends RsltXlat
{

public
MDprocedureRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    data_set[ 0 ][ 0 ] = null;			// PROCEDURE_CAT
    data_set[ 0 ][ 3 ] = null;			// FUTURE_COL1
    data_set[ 0 ][ 4 ] = null;			// FUTURE_COL2
    data_set[ 0 ][ 5 ] = null;			// FUTURE_COL3
    data_set[ 0 ][ 6 ] = null;			// REMARKS
    data_set[ 0 ][ 7 ] = new Short((short)procedureResultUnknown);  // PROCEDURE_TYPE
} // MDprocedureRS

protected void
load_data()
    throws SQLException
{
    convert( 1, 1 );	// PROCEDURE_SCHEM from procedure_owner
    convert( 2, 2 );	// PROCEDURE_NAME from procedure_name
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
    ResultSet 	rs = null;
    EdbcRSMD    rsmd = new EdbcRSMD( (short)proc_desc.length, trace );
    String 	whereClause = "";

    schemaPattern = makePattern( schemaPattern, true );
    procedureNamePattern = makePattern( procedureNamePattern, false );
    for( int i = 0; i < proc_desc.length; i++ )
    {
	if( upper ) proc_desc[i].col_name = proc_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( proc_desc[i].col_name, i+1, proc_desc[i].sql_type,
			    proc_desc[i].dbms_type, proc_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    String	selectClause =
			  "select distinct procedure_owner, procedure_name" +
			  " from iiprocedures";
    String	orderByClause =  " order by procedure_owner, procedure_name";

    if ( conn.dbc.is_ingres )  whereClause = " where system_use ='U'";

    if( schemaPattern != null )
    {
	if( whereClause.length() == 0 )
            whereClause += " where procedure_owner like ?";
        else
	    whereClause += " and procedure_owner like ?";
    }

    if( procedureNamePattern != null )
    {
	if( whereClause.length() == 0 )
            whereClause += " where procedure_name like ?";
        else
	    whereClause += " and procedure_name like ?";
    }

    String qry_txt = selectClause + whereClause + orderByClause;

    if ( trace.enabled() )  
	trace.log( title + ".getProcedures( " + catalog + ", " + 
		   schemaPattern + ", " + procedureNamePattern + " )" );

    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int  i = 1;
	if( schemaPattern != null )
	    stmt.setObject(i++, schemaPattern, param_type);
	if( procedureNamePattern != null )
	    stmt.setObject(i, procedureNamePattern, param_type);
	rs = new MDprocedureRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getProcedures(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    return( rs );
}

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
**  12-Apr-05 (weife01)
**      Bug 114211 to add param_sequence in select and order by list.
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
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "remarks")
};

private static class
MDprocedureColRS
    extends RsltXlat
{

public
MDprocedureColRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    data_set[ 0 ][ 0 ] = null;					   // PROCEDURE_CAT
    data_set[ 0 ][ 4 ] = new Short((short)procedureColumnUnknown); // COLUMN_TYPE
    data_set[ 0 ][10 ] = new Short((short)10);			   // RADIX 
    data_set[ 0 ][12 ] = null;					   // REMARKS

} // MDprocedureColRS

protected void
load_data()
    throws SQLException
{

    convert( 1, 1 );	// PROCEDURE_SCHEM from procedure_owner
    convert( 2, 2 );	// PROCEDURE_NAME from procedure_name
    convert( 3, 3 );	// COLUMN_NAME from param_name

    short s =  (short) Math.abs( dbmsRS.getInt(4) );
			// DATA_TYPE from param_datatype_code
    int col_len = dbmsRS.getInt( 7 );
    int javaType = convToJavaType( s, col_len );
    data_set[ 0 ][ 5 ] = new Short( (short) javaType );

    convert( 6, 5 );	// TYPE_NAME from param_datatype
    convert( 7, 6 );	// PRECISION from  param_scale
    convert( 8, 7 );	// LENGTH from param_length
    convert( 9, 8 );	// SCALE from  param_scale

    String param_null = dbmsRS.getString(9); // NULLABLE from param_nulls

    if ( param_null == null  ||  param_null.length() < 1 ) 
	data_set[ 0 ][ 11 ]  = new Short((short)procedureNullableUnknown);
    else if ( param_null.equals("N") )
	data_set[ 0 ][ 11 ]  = new Short((short)procedureNoNulls);
    else
	data_set[ 0 ][ 11 ]  = new Short((short)procedureNullable);
    
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
    ResultSet 	rs = null;
    schemaPattern = makePattern( schemaPattern, true );
    procedureNamePattern = makePattern( procedureNamePattern, false );
    columnNamePattern = makePattern( columnNamePattern, false );
    String selectClause = "";
    String orderByClause = "";
    String whereClause = "";

    EdbcRSMD    rsmd = new EdbcRSMD( (short)procCol_desc.length, trace );

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
		   " param_scale, param_nulls, param_sequence from iiproc_params";

    orderByClause = " order by procedure_owner, procedure_name, param_sequence";

    if ( ! conn.dbc.is_ingres )
    {
        selectClause = "select distinct proc_owner," + 
		   " proc_name, param_name, param_datatype_code," +
		   " param_datatype, param_scale, param_length," +
		   " param_scale, param_null, param_sequence from iigwprocparams";
        orderByClause = " order by proc_owner, proc_name, param_sequence";
    }


    if ( trace.enabled() )  
	trace.log( title + ".getProcedureColumns( "  + catalog + ", " + 
		   schemaPattern + ", " + procedureNamePattern + ", " + 
		   columnNamePattern + " )" );

    if( schemaPattern != null )
    {
	whereClause += " where procedure_owner like ?";
        if ( ! conn.dbc.is_ingres )  whereClause += " where proc_owner like ?";
    }

    if( procedureNamePattern != null )
    {
    	if( whereClause.length() == 0 )
	{
	    whereClause += " where procedure_name like ?";
            if ( ! conn.dbc.is_ingres )  whereClause += " where proc_name like ?";
	}
	else
	{
	    whereClause += " and procedure_name like ?";
            if ( ! conn.dbc.is_ingres )  whereClause += " and proc_name like ?";
	}
    }

    if( columnNamePattern != null )
    {
    	if( whereClause.length() == 0 )
	    whereClause += " where param_name like ?";
	else
	    whereClause += " and param_name like ?";
    }

    String qry_txt = selectClause + whereClause + orderByClause;

     try
     {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i = 1;
	if( schemaPattern != null )
	    stmt.setObject(i++, schemaPattern, param_type);
	if( procedureNamePattern != null )
	    stmt.setObject(i++, procedureNamePattern, param_type);
	if( columnNamePattern != null )
	    stmt.setObject(i, columnNamePattern, param_type);
	rs = new MDprocedureColRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );

    }
    catch( SQLException ex )
    {
	    if ( trace.enabled() )  
		trace.log( title + ".getProcedureColumns(): failed!" ) ;
	    if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	    throw ex;
    }

    return( rs );
}

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
*/

private static final	MDdesc	tables_desc[] =
{
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "table_cat" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "table_schem" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)32,  "table_name" ),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)16,  "table_type"),
    new MDdesc( Types.VARCHAR, VARCHAR, (short)128, "remarks"),
};

private static class
MDtableRS
    extends RsltXlat
{

public
MDtableRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    data_set[ 0 ][ 0 ] = null;	// Catalog
    data_set[ 0 ][ 4 ] = null;	// Remarks
} // MDtableRS

protected void
load_data()
    throws SQLException
{
    String type = dbmsRS.getString( 3 );
    String system = dbmsRS.getString( 4 );

    if ( type == null  ||  type.length() < 1 )  type = " ";
    if ( system == null  ||  system.length() < 1 )  system = " ";

    convert( 1, 1 );	// TABLE_SCHEM from table_owner
    convert( 2, 2 );	// TABLE_NAME from table_name


    if ( system.charAt( 0 ) == 'S' )
	data_set[ 0 ][ 3 ] = "SYSTEM TABLE";
    else
	switch( type.charAt( 0 ) )
	{
	case 'T' :	data_set[ 0 ][ 3 ] = "TABLE";	break;
	case 'V' :	data_set[ 0 ][ 3 ] = "VIEW";	break;
	default :	data_set[ 0 ][ 3 ] = "UNKNOWN";	break;
	}

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
    schemaPattern = makePattern( schemaPattern, true );
    tableNamePattern = makePattern( tableNamePattern, false );

    String	selectClause = "select table_owner, table_name, table_type," +
				" system_use from iitables";
    String	whereClause = "";
    String	orderByClause = " order by table_type, table_owner, table_name";
    String	systemTables = " system_use = 'S' and table_type in ('T','V')";
    String	clauses[] = new String[3];
    int		clause_cnt = 0;
    boolean	tables = false, views = false, system = false;
    EdbcRSMD    rsmd = new EdbcRSMD( (short)tables_desc.length, trace );
    
    if ( trace.enabled() )  
	trace.log( title + ".getTables( " + catalog + ", " + schemaPattern + 
		   ", " + tableNamePattern + ", " + types + " )" );

    for( int i = 0; i < tables_desc.length; i++ )
    {
	if( upper ) 
	    tables_desc[i].col_name = tables_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( tables_desc[i].col_name, i+1,
			    tables_desc[i].sql_type, tables_desc[i].dbms_type,
			    tables_desc[i].length, (byte)0, (byte)0, false );
    }

    if ( schemaPattern != null )  
	clauses[ clause_cnt++ ] = " table_owner like ?";

    if ( tableNamePattern != null )  
	clauses[ clause_cnt++ ] = " table_name like ?";

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
	return( new RsltData( trace, rsmd, null ) );
    else  if ( ! (tables  ||  views) )
	clauses[ clause_cnt++ ] = systemTables;
    else
    {
	clauses[ clause_cnt ] = " system_use <> 'S' and table_name not like 'ii%' and table_type";
	    
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
	whereClause = " where" + clauses[ 0 ];

	for( int i = 1; i < clause_cnt; i++ )
	    whereClause += " and" + clauses[ i ];
    }

    String	qry_txt = selectClause + whereClause + orderByClause;
    ResultSet	rs = null;

    try
    {
	PreparedStatement stmt = conn.prepareStatement( qry_txt );
	int  i = 1;

	if ( schemaPattern != null )  
	    stmt.setObject( i++, schemaPattern, param_type );
	if ( tableNamePattern != null )  
	    stmt.setObject( i, tableNamePattern, param_type );
	rs = new MDtableRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getTables(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    return( rs );
}

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
**	13-Oct-03 (rajus01) Bug #111088
**          getSchemas() against IDMS returned empty resultSet.
*/

private static  MDdesc schema_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
};

private static class
MDschemaRS
    extends RsltXlat
{

public
MDschemaRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
} // MDschemaRS

protected void
load_data()
    throws SQLException
{
    convert( 0, 1 );	// TABLE_SCHEM from schema_name
    return;
} // load_data

} // class MDschemaRS

public ResultSet 
getSchemas() 
    throws SQLException
{
    ResultSet	rs = null;
    EdbcRSMD	rsmd = new EdbcRSMD( (short)schema_desc.length, trace );
    String	qry_text;

    if ( trace.enabled() )  trace.log( title + ".getSchemas()" );

    if ( std_catl_lvl < OI_CAT_LVL )
        qry_text = 
       "select table_owner from iitables union select table_owner from iiviews";
    else
        qry_text = 
		"select schema_name from iischema order by schema_name";

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
	Statement stmt = conn.createStatement();
	rs = new MDschemaRS( trace, rsmd, (EdbcRslt)stmt.executeQuery(qry_text) );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getSchemas(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    return( rs );
}

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
**  09-Jan-07 (weife01)
**      SIR 117451. Add support for catalog instead of throw SQLException.
*/

private static  MDdesc cat_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
};

public ResultSet 
getCatalogs() 
    throws SQLException
{ 
    if ( trace.enabled() )  trace.log( title + ".getCatalogs(): " + null);
        EdbcRSMD rsmd = new EdbcRSMD( (short)cat_desc.length, trace);
    for (int i = 0; i< cat_desc.length; i++)
    {
        if(upper)cat_desc[i].col_name = cat_desc[i].col_name.toUpperCase();
            rsmd.setColumnInfo(cat_desc[i].col_name, i+1, cat_desc[i].sql_type, 
                        cat_desc[i].dbms_type, cat_desc[i].length, (byte)0, (byte)0, false);
    }
    return( new RsltData( trace, rsmd, null ) ); 
}

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
*/

private static  MDdesc tt_desc =
    new MDdesc(Types.VARCHAR, VARCHAR, (short)20, "table_type");

static final Object  tableTypeValues[][] =
		{ { "TABLE"}, {"VIEW"}, {"SYSTEM TABLE"} };

public ResultSet 
getTableTypes() 
    throws SQLException
{
    if ( trace.enabled() )  trace.log( title + ".getTableTypes()" );

    EdbcRSMD	rsmd = new EdbcRSMD( (short)1, trace );

    if( upper ) tt_desc.col_name = tt_desc.col_name.toUpperCase();
    rsmd.setColumnInfo( tt_desc.col_name, 1, tt_desc.sql_type,
			tt_desc.dbms_type, tt_desc.length,
			(byte)0, (byte)0, false );

    return( new RsltData( trace, rsmd, tableTypeValues ) );
}

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
**	     
*/

private static  MDdesc getc_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "data_type"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "column_size"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "buffer_length"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "decimal_digits"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "num_prec_radix"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "nullable"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "remarks"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "column_def"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "sql_data_type"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "sql_datetime_sub"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "char_octet_length"),
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "ordinal_position"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "is_nullable")
};

private static class
MDgetColRS
    extends RsltXlat
{

public
MDgetColRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    data_set[ 0 ][ 0 ] = null;			 // TABLE_CAT
    data_set[ 0 ][ 7 ] = null;			 // BUFFER_LENGTH
    data_set[ 0 ][ 9 ] = I10;			 // NUM_PREC_RADIX 
    data_set[ 0 ][11 ] = null;			 // REMARKS
    data_set[ 0 ][12]  = null;			 // COLUMN_DEF 
    data_set[ 0 ][13 ] = null;			 // SQL_DATA_TYPE
    data_set[ 0 ][14 ] = null;			 // SQL_DATETIME_SUB
    data_set[ 0 ][17 ] = new String("");	 // IS_NULLABLE 

} // MDgetColRS

protected void
load_data()
    throws SQLException
{


    convert( 1, 1 );	// TABLE_SCHEM from table_owner
    convert( 2, 2 );	// TABLE_NAME from table_name
    convert( 3, 3 );	// COLUMN_NAME from column_name

    short s =  (short) Math.abs( dbmsRS.getInt(4) );
			// DATA_TYPE from column_ingdatatype
    if (s == 1)
    {
         String dtype = dbmsRS.getString(5).toLowerCase();
         s = (short) IdMap.getId(dtype, dataTypeMap);
    }

    int col_len = dbmsRS.getInt( 6 ); // column_length
    int javaType = convToJavaType( s, col_len );
    data_set[ 0 ][ 4 ] = new Short( (short) javaType );

    convert( 5, 5 );	// TYPE_NAME from column_datatype
    int size = colSize( javaType ); // COLUMN_SIZE
    if( size < 0 ) // DECIMAL, CHAR, VARCHAR, VARBINARY
	convert( 6, 6 );
    else
	data_set[ 0 ][ 6 ] = new Integer( size );
    convert( 8, 7 );	// DECIMAL_DIGITS from column_scale
    convert( 16, 9 );	// ORDINAL_POSITION from column_sequence

    /*
    ** use column_length for CHAR_OCTET_LENGTH also (Not sure!!!).
    */
    data_set[ 0 ][ 15 ] = data_set[ 0 ][ 6 ];  // CHAR_OCTET_LENGTH from column_length

    String nullable = dbmsRS.getString(8); // NULLABLE from column_nulls

    if ( nullable == null  ||  nullable.length() < 1 ) 
	data_set[ 0 ][ 10 ]  = new Integer( columnNullableUnknown );
    else if ( nullable.equals("N") )
    {
	data_set[ 0 ][ 10 ]  = new Integer( columnNoNulls );
	data_set[ 0 ][ 17 ]  = new String( "NO");
    }
    else
    {
	data_set[ 0 ][ 10 ]  = new Integer( columnNullable );
	data_set[ 0 ][ 17 ]  = new String( "YES");
    }
    
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
    
    ResultSet rs = null;
    boolean isIdms = false;
    String schemWhere = " table_owner";
    String tableWhere = " table_name";
    String colWhere = " column_name";
    String operator = " like ?";
    String whereClause = " where";

    EdbcRSMD    rsmd = new EdbcRSMD( (short)getc_desc.length, trace );

    String  dbmsType = conn.getDbmsInfo( "DBMS_TYPE" );

    if( dbmsType != null && dbmsType.equals("IDMS") )
    {
        isIdms = true;
        operator = " = ?";
	if( schemaPattern != null && schemaPattern.equals("%") )
	     schemaPattern = null;
	if( tableNamePattern != null && tableNamePattern.equals("%") )
	     tableNamePattern = null;
	if( columnNamePattern != null && columnNamePattern.equals("%") )
	     columnNamePattern = null;
    }
    else   // patterns are not used for IDMS 
    {
       schemaPattern = makePattern( schemaPattern, true );
       tableNamePattern = makePattern( tableNamePattern, false );
       columnNamePattern = makePattern( columnNamePattern, false );
    }

    for( int i = 0; i < getc_desc.length; i++ )
    {
	if( upper ) getc_desc[i].col_name = getc_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( getc_desc[i].col_name, i+1, getc_desc[i].sql_type,
			    getc_desc[i].dbms_type, getc_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    String selectClause  =
			  "select table_owner, table_name, column_name," +
			  " column_ingdatatype," +
			  " column_datatype," +
			  " column_length, column_scale, column_nulls," +
			  " column_sequence from iicolumns";
    String orderByClause = " order by table_owner, table_name," +
			   " column_sequence";

    if( schemaPattern == null &&
	tableNamePattern == null &&
	columnNamePattern == null )
        whereClause = "";
    else
    {
    
        if( schemaPattern != null ) 
	    whereClause += schemWhere + operator;

        if( tableNamePattern != null )
        {
    	    if( whereClause.length() == 6 )  // Length of ' where' string
	        whereClause += tableWhere + operator;
	    else
	        whereClause += " and" + tableWhere + operator; 
        }

        if( columnNamePattern != null )
        {
            if( whereClause.length() == 6 )
	        whereClause += colWhere + operator;
	    else
	        whereClause += " and" + colWhere + operator;
        }
    }

    String qry_txt = selectClause + whereClause + orderByClause;

    if ( trace.enabled() )  
	trace.log( title + ".getColumns( " + catalog + ", " + schemaPattern + 
		   ", " + tableNamePattern + ", " + columnNamePattern + " )" );
    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i = 1;
	if( schemaPattern != null )
	    stmt.setObject(i++, schemaPattern, param_type);
	if( tableNamePattern != null )
	    stmt.setObject(i++, tableNamePattern, param_type);
	if( columnNamePattern != null )
	    stmt.setObject(i, columnNamePattern, param_type);
	rs = new MDgetColRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );

    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getColumns(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    return( rs );
}

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
MDgetColPrivRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    data_set[ 0 ][ 0 ] = null;			 // TABLE_CAT
    data_set[ 0 ][ 5 ] = null;			 // GRANTEE
    data_set[ 0 ][ 6 ] = null;			 // PRIVILEGE
    data_set[ 0 ][ 7 ] = null;			 // IS_GRANTABLE

} // MDgetColPrivRS

protected void
load_data()
    throws SQLException
{

    convert( 1, 1 );		// TABLE_SCHEM from table_owner
    convert( 2, 2 );		// TABLE_NAME from table_name
    convert( 3, 3 );		// COLUMN_NAME from column_name
    data_set[ 0 ][ 4 ] = data_set[ 0 ][ 1 ];	// GRANTOR from table_owner

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
    ResultSet rs = null;
    columnNamePattern = makePattern( columnNamePattern, false );

    EdbcRSMD    rsmd = new EdbcRSMD( (short)getColPriv_desc.length, trace );

    String	selectClause = 
    			  "select table_owner, table_name, column_name" +
			  " from iicolumns";
    String	whereClause = " where table_name = ?";
    String	orderByClause = " order by column_name";

    if( columnNamePattern != null )
	whereClause += " and column_name like ?";

    if ( trace.enabled() )  
	trace.log( title + ".getColumnPrivileges( " + catalog + ", " + 
		   schema + ", " + table + ", " + columnNamePattern + " )" );

    for( int i = 0; i < getColPriv_desc.length; i++ )
    {
	if( upper ) 
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

	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i = 1;
	if( table != null )
	    stmt.setObject(i++, table, param_type);
	if( columnNamePattern != null )
	    stmt.setObject(i, columnNamePattern, param_type);
	rs = new MDgetColPrivRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".getColumnPrivileges(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    return( rs );
}

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
*/

private static  MDdesc getTablePriv_desc[] =
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
MDgetTablePrivRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    data_set[ 0 ][ 0 ] = null;			 // TABLE_CAT
    data_set[ 0 ][ 5 ] = null;			 // PRIVILEGE
    data_set[ 0 ][ 6 ] = null;			 // IS_GRANTABLE

} // MDgetTablePrivRS

protected void
load_data()
    throws SQLException
{

    convert( 1, 1 );		// TABLE_SCHEM from object_owner
    convert( 2, 2 );		// TABLE_NAME from object_name
    convert( 3, 3 );		// GRANTOR from permit_grantor
    convert( 4, 4 );		// GRANTEE from permit_user
    String s = dbmsRS.getString(5);
    if( s != null )
	data_set[ 0 ][ 5 ] = 
	   new String (s.substring(6, s.lastIndexOf( "on" )).toUpperCase());
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
    ResultSet 	rs = null;
    EdbcRSMD    rsmd = new EdbcRSMD( (short)getTablePriv_desc.length, trace );
    schemaPattern = makePattern( schemaPattern, true );
    tableNamePattern = makePattern( tableNamePattern, false );

    String 	selectClause =
			  " select object_owner, object_name," +
			  " permit_grantor, permit_user," +
			  " text_segment" +
			  " from iipermits";
    String	whereClause = " where text_sequence = 1";
    String	orderByClause = " order by object_owner, object_name";

    if( schemaPattern != null )
	whereClause += " and object_owner like ?";
    if( tableNamePattern != null )
	    whereClause += " and object_name like ?";

    if ( trace.enabled() )  
	trace.log( title + ".getTablePrivileges( " + catalog + ", " + 
		   schemaPattern + ", " + tableNamePattern + " )" );
    
    for( int i = 0; i < getTablePriv_desc.length; i++ )
    {
	if( upper ) 
	    getTablePriv_desc[i].col_name = 
				getTablePriv_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( getTablePriv_desc[i].col_name, i+1,
			    getTablePriv_desc[i].sql_type,
			    getTablePriv_desc[i].dbms_type,
			    getTablePriv_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    if ( ! conn.dbc.is_ingres )
    {
	/*
	** return 0 rows for gateways.
	*/
	return( new RsltData( trace, rsmd, null  ) ); 
    }

    String   qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i =1;
	if( schemaPattern != null )
	    stmt.setObject(i++, schemaPattern, param_type);
	if( tableNamePattern != null )
	    stmt.setObject(i++, tableNamePattern, param_type);
	rs = new MDgetTablePrivRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );

    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getTablePrivileges(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    return( rs );
}

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
MDgetBestRowRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS, boolean qtype )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    pkyOrIdx = qtype;
    data_set[ 0 ][ 0 ] = new Short((short)bestRowTemporary); // SCOPE
    data_set[ 0 ][ 6 ] = S0;				    // DECIMAL_DIGITS

} // MDgetBestRowRS

protected void
load_data()
    throws SQLException
{

    if ( ! pkyOrIdx  )
    {
	convert( 1, 1 );	// COLUMN_NAME (tid)
	data_set[ 0 ][ 2 ] = new Short((short)Types.INTEGER);	// DATA_TYPE
	data_set[ 0 ][ 3 ] = new String( "TID" );		// TYPE_NAME
	data_set[ 0 ][ 4 ] = I10;				// COLUMN_SIZE
	data_set[ 0 ][ 5 ] = null;				// BUFFER_LENGTH not used
	data_set[ 0 ][ 7 ] = new Short( (short)bestRowPseudo );	// PSEUDO_COLUMN
    }
    else
    {
	convert( 1, 1 );	// COLUMN_NAME from column_name

        short s =  (short) Math.abs( dbmsRS.getInt(2) );
				// DATA_TYPE from column_ingdatatype
        int col_len = dbmsRS.getInt( 4 );   // column_length
        int javaType = convToJavaType( s, col_len );
        data_set[ 0 ][ 2 ] = new Short( (short) javaType );

	convert( 3, 3 );	// TYPE_NAME from column_datatype
        int size = colSize( javaType ); // COLUMN_SIZE
        if( size < 0 ) // DECIMAL, CHAR, VARCHAR, VARBINARY
	    convert( 4,4 );
	else
	    data_set[ 0 ][ 4 ] = new Integer( size );
	convert( 5, 5 );	// DECIMAL_DIGITS from column_scale
        data_set[ 0 ][ 7 ] = new Short( (short)bestRowNotPseudo ); // NOT a PSEUDO_COLUMN
    }

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
    ResultSet   rs = null;
    String	qry_txt = null;
    String 	selectClause = null; 
    String 	whereClause = null;
    String 	iname = null;
    String 	iowner = null;
    String 	bname = null;
    String 	bowner = null;
    EdbcRSMD    rsmd = new EdbcRSMD( (short)bestRow_desc.length, trace );

    for( int i = 0; i < bestRow_desc.length; i++ )
    {
	if( upper )
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
    if ( scope == bestRowTemporary  &&  conn.dbc.is_ingres )
    {
	selectClause = "select distinct 'tid' from iitables";
	whereClause = " where table_type = 'T'";
	if ( schema != null )  whereClause += " and table_owner = ?";
	if ( table != null )  whereClause += " and table_name = ?";
	qry_txt = selectClause + whereClause;
        
	try
        {
	    PreparedStatement stmt = conn.prepareStatement(qry_txt);
	    int i =1;
	    if( schema != null )
	    	stmt.setObject(i++, schema, param_type);
	    if( table != null )
	        stmt.setObject(i++, table, param_type );
	    rs = new MDgetBestRowRS( trace, rsmd, 
				     (EdbcRslt)stmt.executeQuery(), false );

        }
        catch( SQLException ex )
        {
	    if ( trace.enabled() )
		trace.log( title + ".getBestRowIdentifier(): failed (tid)!" ) ;
	    if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
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
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i =1;
	if( schema != null )
	    stmt.setObject(i++, schema, param_type);
	if( table != null )
	    stmt.setObject(i++, table, param_type );
	rs = new MDgetBestRowRS( trace, rsmd, 
				 (EdbcRslt)stmt.executeQuery(), true );
	if( rs.next() )
	{
	    rs.close();
	    rs = new MDgetBestRowRS( trace, rsmd, 
				     (EdbcRslt)stmt.executeQuery(), true );
	    return( rs );	// unique primary key
	}
	rs.close();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )
	    trace.log( title + ".getBestRowIdentifier(): failed! (primary)" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    /*
     ** Use secondary indexes..
     */

    selectClause = "select distinct i.index_name," +
		   "i.index_owner, i.base_name," +
	           "i.base_owner from iiindexes i";
    whereClause = " where i.unique_rule = 'U'";
    if( schema != null )
	whereClause += " and i.base_owner = ?";
    if( table != null )
	whereClause += " and i.base_name = ?";

    qry_txt = selectClause + whereClause;

    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i =1;
	if( schema != null )
	    stmt.setObject(i++, schema, param_type);
	if( table != null )
	    stmt.setObject(i++, table, param_type );
	rs = (EdbcRslt)stmt.executeQuery();

    }
    catch( SQLException ex )
    {
	if ( trace.enabled() ) 
	    trace.log( title + ".getBestRowIdentifier(): failed (idx1)!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    if( rs.next() )
    {
	iname = rs.getString(1);
	iowner = rs.getString(2);
	bname = rs.getString(3);
	bowner = rs.getString(4);
	rs.close();
    }

    rs.close();

    selectClause = "select distinct c.column_name," +
  	           " c.column_ingdatatype, c.column_datatype," +
		   " c.column_length," +
		   " c.column_scale from iicolumns c," +
		   " iiindex_columns ic ";
    whereClause = "";
    if( iname !=null && iowner != null && bname !=null && bowner != null )
	whereClause = " where c.table_name = ? and" +
		         " c.table_owner = ? and" +
		         " ic.index_name = ? and" +
		         " ic.index_owner = ? and" +
		         " ic.column_name = c.column_name ";
    if( ! nullable )
    {
	if( whereClause.length() == 0 )
	     whereClause += " where c.column_nulls <> 'Y'"; 
	else
	   whereClause += " and c.column_nulls <> 'Y'"; 
    }

    qry_txt = selectClause + whereClause;

    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i =1;
	if( iname !=null && iowner != null && bname !=null && bowner != null )
	{
	    stmt.setObject(i++, bname, param_type );
	    stmt.setObject(i++, bowner, param_type );
	    stmt.setObject(i++, iname, param_type );
	    stmt.setObject(i++, iowner, param_type);
	}

	rs = new MDgetBestRowRS( trace, rsmd, 
				 (EdbcRslt)stmt.executeQuery(), true );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() ) 
	    trace.log( title + ".getBestRowIdentifier(): failed (idx)!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;

    }

    return( rs );	// secondary indexes
}

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
    EdbcRSMD    rsmd = new EdbcRSMD( (short)bestRow_desc.length, trace );

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
    return( new RsltData( trace, rsmd, null ) ); 
}

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
MDprimaryKeyRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS, boolean isIngres )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    is_ingres = isIngres;
    data_set[ 0 ][ 0 ] = null;	// TABLE_CAT
    data_set[ 0 ][ 5 ] = null;	// PK_NAME

} // MDprimaryKeyRS

protected void
load_data()
    throws SQLException
{

    convert( 1, 1 );	// TABLE_SCHEM 
    convert( 2, 2 );	// TABLE_NAME 
    convert( 3, 3 );	// COLUMN_NAME
    convert( 4, 4 );	// KEY_SEQ
    if ( is_ingres )  convert( 5, 5 );	// PK_NAME
    
} // load_data

} // MDprimaryKeyRS

public ResultSet 
getPrimaryKeys( String catalog, String schema, String table ) 
    throws SQLException
{

    ResultSet	rs = null;
    String 	sql_lvl = conn.getDbmsInfo("COMMON/SQL_LEVEL");
    String 	selectClause = ""; 
    String 	whereClause = "";
    String	orderByClause = "";
    String 	qry_txt = null;
    int		cs_lvl = 0;

    if ( sql_lvl != null )
	try { cs_lvl = Integer.parseInt( sql_lvl ); }
	catch( Exception ignore ) {};

    EdbcRSMD    rsmd = new EdbcRSMD( (short)pk_desc.length, trace );

    /*
    ** Only Ingres servers have iikeys.
    */
    if ( conn.dbc.is_ingres  &&  cs_lvl > 601 )
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
	    PreparedStatement stmt = conn.prepareStatement(qry_txt);
	    int i=1; 
	    if( schema != null )
	    stmt.setObject(i++, schema, param_type);
	    if( table != null )
	    	stmt.setObject(i, table, param_type);
	    rs = new MDprimaryKeyRS( trace, rsmd, 
				     (EdbcRslt)stmt.executeQuery(), true );
	    if( rs.next() )
	    {
	    	rs.close();
	    	rs = new MDprimaryKeyRS( trace, rsmd,
					 (EdbcRslt)stmt.executeQuery(),true );
	        return( rs );
	    }
	    rs.close();
        }
        catch( SQLException ex )
        {
	    if ( trace.enabled() )
	        trace.log( title + ".getPrimaryKeys(): iikeys failed!" ) ;
	    if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	       throw ex;
    	}

    // Gateways
    selectClause = "select key_id from iialt_columns";
    whereClause = " where key_sequence <> 0 "; 
    if( schema != null )
	whereClause += " and table_owner = ?";
    if( table != null )
	whereClause += " and table_name = ?";
    qry_txt = selectClause + whereClause;
    int keyID; 
    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i=1; 
	if( schema != null )
	    stmt.setObject(i++, schema, param_type);
	if( table != null )
	    stmt.setObject(i, table, param_type);
	rs = (EdbcRslt) stmt.executeQuery();
	if( rs.next() )
	{
	    keyID = rs.getInt( 1 );
	    rs.close();
	    selectClause = "select table_owner, table_name, " +
			   " column_name, key_sequence from iialt_columns";
	    whereClause = " where key_sequence <> 0  and key_id = " + keyID; 
	    orderByClause = " order by column_name";
	    if( schema != null )
		whereClause += " and table_owner = ?";
	    if( table != null )
		whereClause += " and table_name = ?";
	    qry_txt = selectClause + whereClause + orderByClause;
    	    try
    	    {
		stmt = conn.prepareStatement(qry_txt);
		i=1; 
	        if( schema != null )
	    	    stmt.setObject(i++, schema, param_type);
		if( table != null )
		    stmt.setObject(i, table, param_type);
		rs = new MDprimaryKeyRS( trace, rsmd, 
					 (EdbcRslt)stmt.executeQuery(), false );
	        return( rs );
	    }
            catch( SQLException ex )
            {
		if ( trace.enabled() )
	    	    trace.log( title + ".getPrimaryKeys(): full iialt_columns failed!" ) ;
		if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
		throw ex;
	    }
	}
	rs.close();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getPrimaryKeys(): iialt_columns failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
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
    if( schema != null )
	whereClause += " and t.table_owner = ?";
    if( table != null )
	whereClause += " and t.table_name = ?";
    orderByClause = " order by c.column_name";
    qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i=1; 
	if( schema != null )
	    stmt.setObject(i++, schema, param_type);
	if( table != null )
	    stmt.setObject(i, table, param_type);
	rs = new MDprimaryKeyRS( trace, rsmd, 
				 (EdbcRslt)stmt.executeQuery(), false );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() ) 
	    trace.log( title + ".getPrimaryKeys(): iitables failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    return( rs );
}

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
MDkeysRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    data_set[ 0 ][ 0 ] = null;	// PKTABLE_CAT
    data_set[ 0 ][ 4 ] = null;	// FKTABLE_CAT
    data_set[ 0 ][ 9 ] = new Short((short)importedKeyCascade);	// UPDATE_RULE 
    data_set[ 0 ][ 10 ] = new Short((short)importedKeyCascade);	// DELETE_RULE 
    //CHECK??
    data_set[ 0 ][ 13 ] = new Short((short)importedKeyNotDeferrable); // DEFERABILITY 

} // MDkeysRS

protected void
load_data()
    throws SQLException
{
    convert( 1, 1 );	// PKTABLE_SCHEM  
    convert( 2, 2 );	// PKTABLE_NAME 
    convert( 3, 3 );	// PKCOLUMN_NAME
    convert( 5, 4 );	// FKTABLE_SCHEM 
    convert( 6, 5 );	// FKTABLE_NAME 
    convert( 7, 6 );	// FKCOLUMN_NAME 
    convert( 8, 7 );	// KEY_SEQ 
    convert( 11, 8 );	// FK_NAME 
    convert( 12, 9 );	// PK_NAME 

} // load_data

} // MDkeysRS

public ResultSet 
getImportedKeys( String catalog, String schema, String table ) 
    throws SQLException
{
    ResultSet 	rs = null;
    EdbcRSMD 	rsmd = new EdbcRSMD( (short) keys_desc.length, trace );

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

    if ( ! conn.dbc.is_ingres )
    {
	/*
	** return 0 rows for gateways.
	*/
	return( new RsltData( trace, rsmd, null  ) ); 
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
      
    if( schema != null )
	whereClause += " and f.schema_name = ?";
    if( table != null )
	whereClause += " and f.table_name = ?";

    String qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i=1; 
    	if( schema != null )
	    stmt.setObject(i++, schema, param_type);
	if( table != null )
	    stmt.setObject(i++, table, param_type);
	rs = new MDkeysRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getImportedKeys(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    return( rs );
}

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
*/

public ResultSet 
getExportedKeys( String catalog, String schema, String table) 
    throws SQLException
{
    ResultSet 	rs = null;
    EdbcRSMD 	rsmd = new EdbcRSMD( (short) keys_desc.length, trace );

    for( int i = 0; i < keys_desc.length; i++ )
	rsmd.setColumnInfo( keys_desc[i].col_name, i+1, keys_desc[i].sql_type,
			    keys_desc[i].dbms_type, keys_desc[i].length,
			    (byte)0, (byte)0, false );

    if ( trace.enabled() )  
	trace.log( title + ".getExportedKeys( " + 
		   catalog + ", " + schema + ", " + table + " )" );

    if ( ! conn.dbc.is_ingres )
    {
	/*
	** return 0 rows for gateways.
	*/
	return( new RsltData( trace, rsmd, null ) ); 
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
      
    if( schema != null )
	whereClause += " and p.schema_name = ? ";
    if( table != null )
	whereClause += " and p.table_name = ? ";

    String qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i=1; 
    	if( schema != null )
	    stmt.setObject(i++, schema, param_type);
	if( table != null )
	    stmt.setObject(i++, table, param_type);
	rs = new MDkeysRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getExportedKeys(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );;
	throw ex;
    }

    return( rs );

}

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
    ResultSet 	rs = null;
    EdbcRSMD rsmd = new EdbcRSMD( (short) keys_desc.length, trace );

    for( int i = 0; i < keys_desc.length; i++ )
	rsmd.setColumnInfo( keys_desc[i].col_name, i+1, keys_desc[i].sql_type,
			    keys_desc[i].dbms_type, keys_desc[i].length,
			    (byte)0, (byte)0, false );

    if ( trace.enabled() )  
	trace.log( title + ".getCrossReference( " + primaryCatalog + ", " + 
		   primarySchema + ", " + primaryTable + ", " + 
		   foreignCatalog + ", " + foreignSchema + ", " + 
		   foreignTable + " )" );

    if ( ! conn.dbc.is_ingres )
    {
	/*
	** return 0 rows for gateways.
	*/
	return( new RsltData( trace, rsmd, null  ) ); 
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
      
    if( primarySchema != null )
	whereClause += " and p.schema_name = ?";
    if( foreignSchema != null )
	whereClause += " and f.schema_name = ?";
    if( primaryTable != null )
	whereClause += " and p.table_name = ?";
    if( foreignTable != null )
    	whereClause += " and f.table_name = ?";

    String qry_txt = selectClause + whereClause + orderByClause;

    try
    {
	PreparedStatement stmt = conn.prepareStatement(qry_txt);
	int i=1; 
    	if( primarySchema != null )
	    stmt.setObject(i++, primarySchema, param_type);
    	if( foreignSchema != null )
	    stmt.setObject(i++, foreignSchema, param_type);
	if( primaryTable != null )
	    stmt.setObject(i++, primaryTable, param_type);
	if( foreignTable != null )
	    stmt.setObject(i++, foreignTable, param_type);
	rs = new MDkeysRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  
	    trace.log( title + ".getCrossReference(): failed!" ) ;
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }

    return( rs );

}

static final Object  OpenSqlTypeValues[][] =
{

    { "date", new Short((short)Types.TIMESTAMP), I0, "'", "'", null, nullable, S0, searchB, S1, S0, S0, "date", S0, S0, I0, I0, I10 },
    
    { "char", new Short((short)Types.CHAR), I2000, "'", "'", "length", nullable, S1, search, S0, S0, S0, "char", S0, S0, I0, I0, I10 },

    { "varchar", new Short((short)Types.VARCHAR), I2000, "'", "'", "max length", nullable, S1, search, S0, S0, S0, "varchar", S0, S0, I0, I0, I10 },

    { "smallint", new Short((short)Types.SMALLINT), new Integer(5), null, null, null, nullable, S0, searchB, S1, S0, S0, "smallint", S0, S0, I0, I0, I10 },

    { "integer", new Short((short)Types.INTEGER), I10, null, null, null, nullable, S0, searchB, S1, S0, S0, "integer", S0, S0, I0, I0, I10 },

    { "real", new Short((short)Types.REAL), new Integer(7), null, null, null, nullable, S0, searchB, S1, S0, S0, "real", S0, S0, I0, I0, I10 },

    { "float", new Short((short)Types.DOUBLE), new Integer(7), null, null, "precision", nullable, S0, searchB, S1, S0, S0, "float", S0, S0, I0, I0, I10 }
};

static final Object  sqlTypeValues[][] =
{

    { "date", new Short((short)Types.TIMESTAMP), I0, "'", "'", null, nullable, S0, searchB, S1, S0, S0, "date", S0, S0, I0, I0, I10 },

    { "decimal", new Short((short)Types.DECIMAL), new Integer(31), null, null, "precision,scale", nullable, S0, searchB, S0, S0, S0, "decimal", S0, new Short((short)31), I0, I0, I10 },

    { "char", new Short((short)Types.CHAR), I2000, "'", "'", "length", nullable, S1, search, S1, S0, S0, "char", S0, S0, I0, I0, I10 },

    { "varchar", new Short((short)Types.VARCHAR), I2000, "'", "'", "max length", nullable, S1, search, S1, S0, S0, "varchar", S0, S0, I0, I0, I10 },

    { "long varchar", new Short((short)Types.LONGVARCHAR), I0, "'", "'", null, nullable, S1, search, S1, S0, S0, "long varchar", S0, S0, I0, I0, I10 },

    { "byte", new Short((short)Types.BINARY), I2000, "X'", "'", "length", nullable, S0, searchB, S1, S0, S0, "byte", S0, S0, I0, I0, I10 },

    { "byte varying", new Short((short)Types.VARBINARY), I2000, "X'", "'", "length", nullable, S0, searchB, S1, S0, S0, "byte varying", S0, S0, I0, I0, I10 },

    { "long byte", new Short((short)Types.LONGVARBINARY), I0, "X'", "'", null, nullable, S0, searchN, S0, S0, S0, "long byte", S0, S0, I0, I0, I10 },

    { "integer1", new Short((short)Types.TINYINT), new Integer(3), null, null, null, nullable, S0, searchB, S0, S0, S0, "integer1", S0, S0, I0, I0, I10 },

    { "smallint", new Short((short)Types.SMALLINT), new Integer(5), null, null, null, nullable, S0, searchB, S0, S0, S0, "smallint", S0, S0, I0, I0, I10 },

    { "integer", new Short((short)Types.INTEGER), I10, null, null, null, nullable, S0, searchB, S0, S0, S0, "integer", S0, S0, I0, I0, I10 },

    { "float4", new Short((short)Types.REAL), new Integer(7), null, null, null, nullable, S0, searchB, S0, S0, S0, "float4", S0, S0, I0, I0, I10 },

    { "float", new Short((short)Types.DOUBLE), new Integer(15), null, null, "precision", nullable, S0, searchB, S0, S0, S0, "float", S0, S0, I0, I0, I10 }

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
    new MDdesc(Types.INTEGER, INTEGER, (short)4,  "local_type_name"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "minimum_scale"),
    new MDdesc(Types.SMALLINT, INTEGER, (short)2, "maximum_scale"),
    new MDdesc(Types.INTEGER,  INTEGER, (short)4, "sql_data_type"),
    new MDdesc(Types.INTEGER,  INTEGER, (short)4, "sql_datetime_sub"),
    new MDdesc(Types.INTEGER,  INTEGER, (short)4, "num_prec_radix")
};
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
*/

public ResultSet 
getTypeInfo() 
    throws SQLException
{

    int	  isql_lvl = 0;

    String sql_level = conn.getDbmsInfo( "INGRES/SQL_LEVEL" );

    if( sql_level != null )
	try { isql_lvl = Integer.parseInt( sql_level ) ; }
	catch( Exception ignore ){};

    EdbcRSMD	rsmd = new EdbcRSMD( (short)md_desc.length, trace );

    for( int i = 0; i < md_desc.length; i++ )
    {
	if( upper ) md_desc[i].col_name = md_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( md_desc[i].col_name, i+1, md_desc[i].sql_type,
			    md_desc[i].dbms_type, md_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    if ( trace.enabled() )  trace.log( title + ".getTypeInfo()" );

    if ( isql_lvl > 0 ) 
        return( new RsltData( trace, rsmd, sqlTypeValues ) );
    else
        return( new RsltData( trace, rsmd, OpenSqlTypeValues ) );

}

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
*/

private static  MDdesc idx_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "table_name"),
    new MDdesc(Types.BIT,     VARCHAR, (short)1,  "non_unique"),
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
MDindexRS( EdbcTrace trace, EdbcRSMD rsmd, EdbcRslt dbmsRS )
    throws EdbcEx
{
    super( trace, rsmd, dbmsRS );
    data_set[ 0 ][ 0 ] = null;					 // TABLE_CAT
    data_set[ 0 ][ 6 ] = new Short( (short)tableIndexOther );    // TYPE
    data_set[ 0 ][ 10 ] = I0;					 // CARDINALITY
    data_set[ 0 ][ 11 ] = I0;					 // PAGES
    data_set[ 0 ][ 12 ] = null; 				 // FILTER_CONDITION

} // MDindexRS

protected void
load_data()
    throws SQLException
{
    /* NON_UNIQUE from iiindexes.unique_rule */
    String unique = dbmsRS.getString( 3 );  
    
    if ( unique != null  &&  unique.equals( "D" ) ) 
	data_set[ 0 ][ 3 ]  = Boolean.TRUE;
    else 
	data_set[ 0 ][ 3 ]  = Boolean.FALSE;

    convert( 1, 1 );	// TABLE_SCHEM from iiindexes.base_owner
    convert( 2, 2 );	// TABLE_NAME from iiindexes.base_name
    convert( 4, 4 );	// INDEX_QUALIFIER from iiindexes.index_owner
    convert( 5, 5 );	// INDEX_NAME from iiindexes.index_name
    convert( 7, 6 );	// ORDINAL_POSITION from iiindex_columns.key_sequence
    convert( 8, 7 );	// COLUMN_NAME from iiindex_columns.column_name
    convert( 9, 8 );	// ASC_OR_DESC from iiindex_columns.sort_direction

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

    ResultSet	rs = null;
    EdbcRSMD    rsmd = new EdbcRSMD( (short)idx_desc.length, trace );
    String	select, where, order, query;

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
	PreparedStatement stmt = conn.prepareStatement( query );
	int i = 1;

	if ( schema != null )  stmt.setObject( i++, schema, param_type );
	if ( table != null )  stmt.setObject( i++, table, param_type );
	rs = new MDindexRS( trace, rsmd, (EdbcRslt)stmt.executeQuery() );
    }
    catch( SQLException ex )
    {
	if ( trace.enabled() )  trace.log( title + ".getIndexInfo(): failed!" );
	if ( trace.enabled( 1 ) )  ((EdbcEx)ex).trace( trace );
	throw ex;
    }
    
    return( rs );
}


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
*/

public boolean
supportsResultSetType( int type )
{
    boolean supported = (type == ResultSet.TYPE_FORWARD_ONLY);
    if ( trace.enabled() )  trace.log( 
	title + ".supportsResultSetType( " + type + " ): " + supported );
    return( supported );
}


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
*/

public boolean
supportsResultSetConcurrency( int type, int concur )
{
    boolean supported = (type == ResultSet.TYPE_FORWARD_ONLY  &&
			 ( concur == ResultSet.CONCUR_READ_ONLY  ||
			   concur == ResultSet.CONCUR_UPDATABLE ));
    if ( trace.enabled() )  
	trace.log( title + ".supportsResultSetConcurrency( " + 
			   type + ", " + concur + " ): " + supported );
    return( supported );
}


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
*/

public boolean
ownUpdatesAreVisible( int type )
{
    if ( trace.enabled() )  
	trace.log( title + ".ownUpdatesAreVisible( " + type + " ): " + false );
    return( false );
}


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
*/

public boolean
ownDeletesAreVisible( int type )
{
    if ( trace.enabled() )  
	trace.log( title + ".ownDeletesAreVisible( " + type + " ): " + false );
    return( false );
}


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
*/

public boolean
ownInsertsAreVisible( int type )
{
    if ( trace.enabled() )  
	trace.log( title + ".ownInsertsAreVisible( " + type + " ): " + false );
    return( false );
}


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
*/

public boolean
othersUpdatesAreVisible( int type )
{
    if ( trace.enabled() )  
	trace.log(title + ".othersUpdatesAreVisible( " + type + " ): " + false);
    return( false );
}


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
*/

public boolean
othersDeletesAreVisible( int type )
{
    if ( trace.enabled() )  
	trace.log(title + ".othersDeletesAreVisible( " + type + " ): " + false);
    return( false );
}


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
*/

public boolean
othersInsertsAreVisible( int type )
{
    if ( trace.enabled() )  
	trace.log(title + ".othersInsertsAreVisible( " + type + " ): " + false);
    return( false );
}


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
*/

public boolean
updatesAreDetected( int type )
{
    if ( trace.enabled() )  
	trace.log(title + ".updatesAreDetected( " + type + " ): " + false);
    return( false );
}


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
*/

public boolean
deletesAreDetected( int type )
{
    if ( trace.enabled() )  
	trace.log(title + ".deletesAreDetected( " + type + " ): " + false);
    return( false );
}


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
*/

public boolean
insertsAreDetected( int type )
{
    if ( trace.enabled() )  
	trace.log(title + ".insertsAreDetected( " + type + " ): " + false);
    return( false );
}


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
}


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
*/

private static  MDdesc udt_desc[] =
{
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_cat"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_schem"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "type_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "class_name"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)32, "data_type"),
    new MDdesc(Types.VARCHAR, VARCHAR, (short)256, "remarks"),
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

    EdbcRSMD    rsmd = new EdbcRSMD( (short)udt_desc.length, trace );

    for( int i = 0; i < udt_desc.length; i++ )
    {
	if( upper ) udt_desc[i].col_name = udt_desc[i].col_name.toUpperCase();
	rsmd.setColumnInfo( udt_desc[i].col_name, i+1, udt_desc[i].sql_type,
			    udt_desc[i].dbms_type, udt_desc[i].length,
			    (byte)0, (byte)0, false );
    }

    return( new RsltData( trace, rsmd, null ) );
}


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
    return( conn );
}




/*
** Name: makePattern
**
** Description:
**	Returns pattern without any change if pattern is '%' or null
**	or if DBMS server is ORACLE or DB2 or if it already has a
**	% as the last character. Otherwise it is appended with ' %'. 
**
**	DB2 gateway requires special handling for schema patterns as the
**	datatype is CHAR for table_owner in 'iitables' catalog. 
**
** Input:
**	pattern		String	
**	schema		boolean
**
** Output:
**	None.
**
** Returns:
**	String 
**
** History:
**	22-Mar-00 (rajus01)
**	    Created.
**	28-Mar-00 (rajus01)
**	    Removed ')'.
**	 6-Mar-01 (gordy)
**	    Sybase and MS-SQL don't need ' %'.    	   
*/
private String 
makePattern ( String pattern, boolean schema )
    throws SQLException
{
    String  dbmsType = conn.getDbmsInfo( "DBMS_TYPE" );

    if ( trace.enabled() )  
	trace.log( title + ".makePattern(" + pattern + ") " + schema ) ;

    if ( pattern != null && 
         pattern.length() >=  1 && 
	 pattern.charAt( pattern.length() - 1 ) != '%' &&
	 dbmsType != null &&
	 ! dbmsType.equals("ORACLE") &&
	 ! dbmsType.equals("SYBASE")  &&
	 ! dbmsType.equals("MSSQL")  &&
	 (! dbmsType.equals("DB2") || schema) 
       ) 
	pattern = pattern + " %";

    if ( trace.enabled() )  trace.log( title + ".makePattern(): " + pattern ) ;
    return( pattern );
}


/*
** Name: getMaxCol 
**
** Description:
**	Returns integer from a String representation of an Integer.
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
**	07-Dec-99 (rajus01)
**	    Created.
*/

private int
getMaxCol()
    throws SQLException
{
    int		col = 0;
    String	str  = conn.getDbmsInfo("MAX_COLUMNS");

    if ( str != null )
        try { col = Integer.parseInt( str ); }
	catch( Exception ignore ) {};

    return( col );
}


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
*/

private String
getStringType ( int type )
    throws EdbcEx
{
    switch( type )
    {
    case Types.ARRAY:		return( "ARRAY" );
    case Types.BIT:		return( "BIT" );
    case Types.BLOB:		return( "BLOB" );
    case Types.CLOB:		return( "CLOB" );
    case Types.DISTINCT:	return( "DISTINCT" );
    case Types.JAVA_OBJECT:	return( "JAVA_OBJECT" );
    case Types.NULL:		return( "NULL");
    case Types.OTHER:		return( "OTHER");
    case Types.REF:		return( "REF" );
    case Types.STRUCT:		return( "STRUCT" );
    case Types.TINYINT:		return( "TINYINT" );
    case Types.SMALLINT:    	return( "SMALLINT" );
    case Types.INTEGER:		return( "INTEGER" );
    case Types.FLOAT:		return( "FLOAT" );
    case Types.REAL:		return( "REAL" );
    case Types.DOUBLE:		return( "DOUBLE" );
    case Types.NUMERIC:		return( "NUMERIC" );
    case Types.DECIMAL:		return( "DECIMAL" );
    case Types.CHAR:		return( "CHAR" );
    case Types.VARCHAR:		return( "VARCHAR" );
    case Types.LONGVARCHAR:	return( "LONGVARCHAR" );
    case Types.TIMESTAMP:	return( "TIMESTAMP" );
    case Types.BINARY:		return( "BINARY" );
    case Types.VARBINARY:	return( "VARBINARY" );
    case Types.LONGVARBINARY:	return( "LONGVARBINARY" );
    case Types.BIGINT:		return( "BIGINT" );
    case Types.DATE:		return( "DATE" );
    case Types.TIME:		return( "TIME" );

    default:
	if ( trace.enabled() )
	    trace.log( title + ": Unsupported SQL type " + type );
	throw EdbcEx.get( E_JD0011_SQLTYPE_UNSUPPORTED );
    }
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
**	 22-Dec-99 (rajus01)
**	    Created.
**       30-Sep-03 (weife01)
**          BUG 111022 - Add unicode data type NCHAR, NVARCHAR, 
**          LONG NVARCHAR map from DBMS to JAVA type CHAR, 
**          VARCHAR, LONG VARCHAR
*/

private static int 
convToJavaType( short ingType, int len ) 
{
    switch( ingType )
    {
    case DBMS_TYPE_DATE:		return( Types.TIMESTAMP );
    case DBMS_TYPE_MONEY:		return( Types.DECIMAL );
    case DBMS_TYPE_DECIMAL:		return( Types.DECIMAL );
    case DBMS_TYPE_CHAR:		return( Types.CHAR );
    case DBMS_TYPE_NCHAR:		return( Types.CHAR );
    case DBMS_TYPE_VARCHAR:		return( Types.VARCHAR );
    case DBMS_TYPE_NVARCHAR:		return( Types.VARCHAR );
    case DBMS_TYPE_LONG_CHAR:		return( Types.LONGVARCHAR );
    case DBMS_TYPE_LONG_NCHAR:		return( Types.LONGVARCHAR );
    case DBMS_TYPE_BYTE:		return( Types.BINARY );
    case DBMS_TYPE_VARBYTE:		return( Types.VARBINARY );
    case DBMS_TYPE_LONG_BYTE:		return( Types.LONGVARBINARY );
    case DBMS_TYPE_INT:
	switch( len )
	{
	case 1:				return( Types.TINYINT );
	case 2:				return( Types.SMALLINT );
	case 4:				return( Types.INTEGER );
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
    case DBMS_TYPE_TEXT:		return( Types.VARCHAR );
    case DBMS_TYPE_LONG_TEXT:		return( Types.VARCHAR );
    default:				return( Types.NULL );
    }
}


/*
** Name: colSize
**
** Description:
**	Determines COLUMN_SIZE DatabaseMetaData  value. 
**
** Input:
**	javaType	 Java data type.
**
** Output:
**	None.
**
** Returns:
**	int.
**
** History:
**	 22-Dec-99 (rajus01)
**	    Created.
*/

private static int 
colSize( int javaType )
    throws EdbcEx
{
    switch( javaType )
    {
    case Types.DECIMAL:
    case Types.CHAR:
    case Types.VARCHAR:
    case Types.BINARY:
    case Types.VARBINARY:	return( -1 );
    case Types.TIMESTAMP:	return( 26 );
    case Types.TINYINT:		return( 4 );
    case Types.SMALLINT:	return( 6 );
    case Types.INTEGER:		return( 11 );
    case Types.REAL:		return( 14 );
    case Types.DOUBLE:		return( 24 );
    case Types.LONGVARCHAR:
    case Types.LONGVARBINARY:	return( 0 );
    default:			return( -1 );
    }
}


/*
** Name: MDdesc
**
** Description:
**	Describes a single column in the constant MetaData result set.
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
    String  col_name = null;
    int	    sql_type = Types.OTHER;
    short   dbms_type = 0;
    short   length = 0;

public		// Constructor
MDdesc( int stype, short dtype, short len, String name )
{
    sql_type = stype;
    dbms_type = dtype;
    length = len;
    col_name = name;
}

} // MDdesc

} // EdbcMetaData
