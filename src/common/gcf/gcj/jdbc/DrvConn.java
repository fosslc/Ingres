/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package	com.ingres.gcf.jdbc;

/*
** Name: DrvConn.java
**
** Description:
**	Defines class representing a JDBC Driver connection.
**
**  Classes:
**
**	DrvConn		Driver Connection.
**	DbCaps		DBMS Capabilities.
**	DbInfo		Access dbmsinfo().
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	13-Sep-99 (gordy)
**	    Implemented error code support.
**	29-Sep-99 (gordy)
**	    Used DbConn lock()/unlock() for synchronization to
**	    support BLOB streams.
**	 2-Nov-99 (gordy)
**	    Read and save DBMS information.  Added methods 
**	    setDbmsInfo() and getDbmsInfo() to DbConn.
**	12-Nov-99 (gordy)
**	    Allow gmt or local timezone to support gateways.
**	17-Dec-99 (gordy)
**	    Use 'readonly' cursors.
**	17-Jan-00 (gordy)
**	    Connection pool parameter sent as boolean value.  Autocommit
**	    is now enable by the server at connect time and disabled at
**	    disconnect.  Replaced iidbcapabilities query with a request
**	    for a pre-defined server query.  Added readData() method to
**	    read the results of the server request.
**	19-May-00 (gordy)
**	    Added public field for select_loops and make additional
**	    validation check during unlock().
**	 4-Oct-00 (gordy)
**	    Standardized internal tracing.
**	19-Oct-00 (gordy)
**	    Added getXactID().
**	 3-Nov-00 (gordy)
**	    Removed timezone fields/methods and replaced with public
**	    boolean field: use_gmt_tz.
**	20-Jan-01 (gordy)
**	    Added msg_protocol_level.
**	 5-Feb-01 (gordy)
**	    Coalesce statement IDs using a statement ID cache with the
**	    query text as key which is cleared at transaction boundaries.
**	 7-Mar-01 (gordy)
**	    Revamped the DBMS capabilities processing so accomodate
**	    support for SQL dbmsinfo() queries.
**	28-Mar-01 (gordy)
**	    Moved server connection establishment into this class and
**	    placed server and database connection code into separate
**	    methods.  Tracing now passed as parameter in constructors.  
**	16-Apr-01 (gordy)
**	    Optimize usage of Result-set Meta-data.
**	10-May-01 (gordy)
**	    Added support for UCS2 datatypes.
**	20-Aug-01 (gordy)
**	    Added support for default cursor mode.
**	25-Oct-01 (gordy)
**	    Only issue dbmsinfo() query for items which aren't pre-loaded
**	    into the info cache.
**	20-Feb-02 (gordy)
**	    Added fields for DBMS protocol level and Ingres/gateway distinction.
**	31-Oct-02 (gordy)
**	    Created DrvConn class as a centralized connection object by
**	    extracting non JDBC Connection interface code from JdbcConn
**	    and connection attribute information from MsgConn.
**	20-Dec-02 (gordy)
**	    Support character encoding connection parameter. Inform
**	    messaging system of the negotiated protocol level.
**	28-Mar-03 (gordy)
**	    Updated to JDBC 3.0.
**	 7-Jul-03 (gordy)
**	    Added Ingres configuration parameters.
**	 1-Nov-03 (gordy)
**	    Generalized parameter handling for updatable result-sets.
**	 5-May-04 (gordy)
**	    Further optimize prepared statement handling.
**	28-May-04 (gordy)
**	    Add max column lengths for NCS columns.
**	16-Jun-04 (gordy)
**	    Added negotiated session mask.
**	20-Dec-04 (gordy)
**	    Allow prepared statement cache to be cleared externally.
**	24-Jan-06 <gordy)
**	    Re-branded for Ingres Corporation.
**	 4-May-06 (gordy)
**	    Changed default cursor mode to READONLY to better match
**	    JDBC semantics and provide better default performance.
**	19-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	 3-Jul-06 (gordy)
**	    Driver properties moved to JdbcConn, replaced with general
**	    configuration settings.
**	30-Jan-07 (gordy)
**	    Backward compatibility LOB Locator support.
**	26-Feb-07 (gordy)
**	    Enhanced LOB compatibility configuration.
**	12-Sep-07 (gordy)
**	    Added option to define value used in place of empty dates.
**	 9-Sep-08 (gordy)
**	    Determine max decimal precision from DBMS capabilities.
**      24-Nov-08 (rajus01) SIR 121238
**          - Added new JDBC 4.0 methods to avoid compilation errors with
**            JDK 1.6. The new methods return E_GC4019 error for now.
**          - Replaced SqlEx references with SQLException or SqlExFactory
**            depending upon the usage of it. SqlEx becomes obsolete to support
**            JDBC 4.0 SQLException hierarchy.
**	24-Dec-08 (gordy)
**	    Added date/time formatter instances.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	 3-Mar-09 (gordy)
**	    Add config setting to enable/disable scrollable cursors.
**	25-Mar-10 (gordy)
**	    Add config setting to enable/disable batch processing.
**	30-Jul-10 (gordy)
**	    Add config setting to force booleans parameters to integer.
**	16-Nov-10 (gordy)
**	    Add config setting to enable LOB locator stream access.
*/

import	java.util.Hashtable;
import	java.util.Stack;
import	java.util.Properties;
import	java.util.Random;
import	java.sql.Statement;
import	java.sql.ResultSet;
import	java.sql.SQLException;
import	com.ingres.gcf.util.CompatCI;
import	com.ingres.gcf.util.CharSet;
import	com.ingres.gcf.util.Config;
import	com.ingres.gcf.util.ConfigKey;
import	com.ingres.gcf.util.DbmsConst;
import	com.ingres.gcf.util.SqlDates;
import	com.ingres.gcf.util.TraceLog;
import	com.ingres.gcf.util.GcfErr;
import	com.ingres.gcf.util.SqlExFactory;
import	com.ingres.gcf.dam.MsgConn;
import	com.ingres.gcf.dam.MsgConst;


/*
** Name: DrvConn
**
** Description:
**	Class representing a JDBC Driver connection.  
**
**	This class is a central repository for driver information
**	regarding the DBMS connection. 
** 
**  Constants:
**
**	CNF_LOCATORS	    	LOB Locators enabled.
**	CNF_LOC_AUTO		LOB Locators enabled during autocommit.
**	CNF_LOC_LOOP		LOB Locators enabled during select loops.
**	CNF_LOC_STRM		LOB Locators stream access enabled.
**	CNF_LOB_CACHE		LOB buffering enabled.
**	CNF_CURS_SCROLL		Scrollable cursors enabled.
**	CNF_BATCH		Batch processing enabled.
**
**  Public Data:
**
**	driverName		Long driver name.
**	protocol		Driver URL protocol ID.
**	host			Target server address.
**	database		Target database name.
**	msg			DAM-ML message I/O.
**	jdbc			JDBC Connection.
**	config			Configuration settings.
**	dbCaps			DBMS capabilities.
**	dbInfo			Access dbmsinfo().
**	msg_protocol_level	DAM-ML protocol level.
**	db_protocol_level	DBMS protocol level
**	session_mask		Session mask.
**	cnf_flags		Configuration flags.
**	cnf_lob_segSize		Configured LOB segment size.
**	cnf_empty_date		Configured empty date replacement.
**	is_dtmc			Is the connection DTM.
**	is_ingres		Is the DBMS Ingres.
**	sqlLevel		Ingres SQL Level.
**	osql_dates		Use OpenSQL dates?
**	is_gmt			DBMS applies GMT timezone?
**	ucs2_supported		Is UCS2 data supported?
**	select_loops		Are select loops enabled?
**	cursor_mode		Default cursor mode.
**	autoCommit		Is autocommit enabled.
**	readOnly		Is connection READONLY.
**	max_char_len		Max char string length.
**	max_vchr_len		Max varchar string length.
**	max_byte_len		Max byte string length.
**	max_vbyt_len		Max varbyte string length.
**	max_nchr_len		Max nchar string length.
**	max_nvch_len		Max nvarchar string length.
**	max_dec_prec		Max decimal precision.
**	dt_frmt			Standard date/time format.
**	dt_lit			Literal date/time format.
**	trace			Tracing output.
**	dbms_log		DBMS trace log.
**
**  Public Methods:
**
**	close			Close server connection.
**	timeValuesInGMT 	Use GMT for date/time values.
**	timeLiteralsInGMT	Use GMT for date/time literals.
**	endXact			Declare end of transaction.
**	getXactID		Return internal transaction ID.
**	getUniqueID		Return a new unique identifier.
**	createPrepStmt		Create prepared statement.
**	findPrepStmt		Find existing prepared statement.
**	dropPrepStmt		Destroy prepared statement.
**	clearPrepStmts		Drop prepared statements.
**	loadDbCaps		Load DBMS capabilities.
**
**  Private Data:
**
**	prepStmts		Prepared statement pool.
**	activeStmts		Active statement pool.
**	stmtNames		Statement name pool.
**	tran_id			Transaction ID.
**	obj_id			Object ID.
**	tr_id			Tracing ID.
**
**  Private Methods:
**
**	connect			Establish server connection.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	 2-Nov-99 (gordy)
**	    Add getDbmsInfo(), readDbmsInfo() methods.
**	11-Nov-99 (rajus01)
**	    Add field url.
**	12-Nov-99 (gordy)
**	    Added methods to permit configuration of timezone.
**	17-Jan-00 (gordy)
**	    JDBC server now has AUTOCOMMIT ON as its default state.
**	19-May-00 (gordy)
**	    Added public select_loops status field.
**	31-May-00 (gordy)
**	    Make date formatters public.
**	19-Oct-00 (gordy)
**	    Added getXactID().
**	 3-Nov-00 (gordy)
**	    Removed timezone fields/methods and replaced with public
**	    boolean field, use_gmt_tz, indicating if connection uses
**	    GMT timezone (default is to use GMT).
**	20-Jan-01 (gordy)
**	    Added msg_protocol_level now that there are more than 1 level.
**	 5-Feb-01 (gordy)
**	    Added stmtID as statment ID cache for the new method
**	    getStmtID().
**	 7-Mar-01 (gordy)
**	    Replace readDbmsInfo() method with class to perform same
**	    operation.  Add getDbmsInfo() method and companion class
**	    which implements the SQL dbmsinfo() function.  Added dbInfo
**	    field for the getDbmsInfo() method companion class.
**	28-Mar-01 (gordy)
**	    Replaced URL with host and database.
**	10-May-01 (gordy)
**	    Added public flag, ucs2_supported, for UCS2 support.
**	20-Aug-01 (gordy)
**	    Added default for connection cursors, cursor_mode, and
**	    related constants: CRSR_DBMS, CRSR_READONLY, CRSR_UPDATE.
**	20-Feb-02 (gordy)
**	    Added db_protocol_level and is_ingres to handle differences
**	    in DBMS protocol levels and gateways.
**	31-Oct-02 (gordy)
**	    Created DrvConn class as a centralized connection object by
**	    extracting non JDBC Connection interface code from JdbcConn
**	    and connection attribute information from MsgConn.
**	 7-Jul-03 (gordy)
**	    Added ingres_tz to indicate that Ingres timezone sent to DBMS.
**	 1-Nov-03 (gordy)
**	    Max column lengths moved to DrvConn for general access.
**	 5-May-04 (gordy)
**	    Replaced statement ID cache, stmtID, with prepared statement
**	    cache, prepStmts.  Replaced getStmtID() with getPrepStmt().
**	28-May-04 (gordy)
**	    Add max column lengths for NCS columns.
**	16-Jun-04 (gordy)
**	    Added session mask of appropriate size for MsgConn.encode().
**	20-Dec-04 (gordy)
**	    Added method clearPrepStmts().
**	24-Jan-06 <gordy)
**	    Re-branded for Ingres Corporation.
**	 4-May-06 (gordy)
**	    Changed default cursor mode to READONLY to better match
**	    JDBC semantics and provide better default performance.
**	19-Jun-06 (gordy)
**	    Replaced use_gmt_tz with osql_dates and ingres_tz with
**	    is_gmt to better represent intent.  Added timeValuesInGMT()
**	    and timeLiteralsInGMT() (from DrvObj) and sqlLevel.
**	 3-Jul-06 (gordy)
**	    Added configuration settings and DBMS trace log.
**	10-Nov-06 (gordy)
**	    Added activeStmts and stmtNames.  Replaced getPrepStmt()
**	    with createPrepStmt(), findPrepStmt(), and dropPrepStmt().
**	30-Jan-07 (gordy)
**	    Added lob_locators to indicate if LOB Locators are enabled.
**	26-Feb-07 (gordy)
**	    Converted lob_locators into cnf_flags and added flag constants.
**	12-Sep-07 (gordy)
**	    Added replacement string for empty dates: cnf_empty_date.
**	 9-Sep-08 (gordy)
**	    Added maximum decimal precision: max_dec_prec.
**	24-Dec-08 (gordy)
**	    Added dt_frmt and dt_lit.
**	 3-Mar-09 (gordy)
**	    Added config flag to enable/disable scrollable cursors.
**      11-Feb-10 (rajus01) Bug 123277
**	    Added snd_ing_dte.
**	25-Mar-10 (gordy)
**	    Added config flag to enable/disable batch processing.
**	30-Jul-10 (gordy)
**	    Added config flag to force booleans to be sent as integers.
**	    Converted similar snd_ing_dte config item to config flag.
**	16-Nov-10 (gordy)
**	    Added CNF_LOC_STRM as default to enable LOB locator stream
**	    access.
*/

class
DrvConn 
    implements DrvConst, MsgConst, DbmsConst, GcfErr
{

    /*
    ** Connection objects.
    */
    public  MsgConn	msg			= null;
    public  JdbcConn	jdbc			= null;
    public  Config	config			= null;
    public  DrvTrace	trace			= null;
    public  TraceLog	dbms_log		= null;
    public  DbCaps	dbCaps			= null;
    public  DbInfo	dbInfo			= null;

    /*
    ** Driver identifiers.
    */
    public  String	driverName		= "Ingres JDBC Driver";
    public  String	protocol		= "?";

    /*
    ** Configuration properties.
    */
    public static final int	CNF_LOCATORS	= 0x0001;
    public static final int	CNF_LOC_AUTO	= 0x0002;
    public static final int	CNF_LOC_LOOP	= 0x0004;
    public static final int	CNF_LOC_STRM	= 0x0008;
    public static final int	CNF_LOB_CACHE	= 0x0010;
    public static final int	CNF_CURS_SCROLL	= 0x0100;
    public static final int	CNF_BATCH	= 0x0200;
    public static final int	CNF_INGDATE	= 0x0400;
    public static final int	CNF_INTBOOL	= 0x0800;

    public int		cnf_flags		= CNF_LOCATORS | CNF_LOC_STRM |
						  CNF_CURS_SCROLL | CNF_BATCH;
    public int		cnf_lob_segSize		= DRV_DFLT_SEGSIZE;
    public String	cnf_empty_date		= DRV_DFLT_EMPTY_DATE;

    /*
    ** Connection status.
    */
    public  String	host			= null;
    public  String	database		= null;
    public  byte	msg_protocol_level	= MSG_PROTO_1;
    public  byte	db_protocol_level	= DBMS_API_PROTO_1;
    public  byte	session_mask[]		= new byte[CompatCI.CRYPT_SIZE];
    public  boolean	is_dtmc			= false;
    public  boolean	is_ingres		= true;
    public  int		sqlLevel		= 0;
    public  boolean	osql_dates		= false;
    public  boolean	is_gmt			= true;
    public  boolean	ucs2_supported		= false;
    public  boolean	select_loops		= false;
    public  int		cursor_mode		= DRV_CRSR_READONLY;
    public  boolean	autoCommit		= true;
    public  boolean	readOnly		= false;
    public  int		max_char_len		= DBMS_COL_MAX;
    public  int		max_vchr_len		= DBMS_COL_MAX;
    public  int		max_byte_len		= DBMS_COL_MAX;
    public  int		max_vbyt_len		= DBMS_COL_MAX;
    public  int		max_nchr_len		= DBMS_COL_MAX / 2;
    public  int		max_nvch_len		= DBMS_COL_MAX / 2;
    public  int		max_dec_prec		= DBMS_PREC_MAX;
    public  SqlDates	dt_frmt			= SqlDates.getInstance();
    public  SqlDates	dt_lit			= SqlDates.getLiteralInstance();

    
    /*
    ** Local data.
    */
    private static final String	STMT_PREFIX 	= "JDBC_STMT_";

    private Hashtable	prepStmts		= new Hashtable();
    private Hashtable	activeStmts		= new Hashtable();
    private Stack	stmtNames		= new Stack();
    private int		tran_id			= 0;
    private int		obj_id			= 0;
    private String	tr_id			= "Conn";


/*
** Name: DrvConn
**
** Description:
**	Class constructor.  Establishes a connection with the 
**	Data Access Server.
**
** Input:
**	host	Target host and port.
**	config	Configuration settings.
**	trace	Tracing output
**	dtmc	Is a DTM connection requested?
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Moved server connection establishment into this class and
**	    placed server and database connection code into separate
**	    methods.  Replaced dbc parameter with host.  Dropped URL
**	    as a parameter and added tracing as a parameter.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	26-Dec-02 (gordy)
**	    Added info parameter to support character encoding.
**	 7-Jul-03 (gordy)
**	    Changed property parameter to support config heirarchy.
**	16-Jun-04 (gordy)
**	    Initialize session mask with random values.
**	 3-Jul-06 (gordy)
**	    Character encoding now handled with other driver properties.
**	    Configuration settings now general rather than driver
**	    properties.  Initialize DBMS trace log.
**	26-Feb-07 (gordy)
**	    Extracted initialization to init().
*/

public
DrvConn( String host, Config config, DrvTrace trace, boolean dtmc )
    throws SQLException
{
    this.host = host;
    this.trace = trace;
    this.config = config;

    init();
    connect( dtmc );
    return;
} // DrvConn


/*
** Name: init
**
** Description:
**	General connection initialization.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	26-Feb-07 (gordy)
**	    Created.
**	12-Sep-07 (gordy)
**	    Check for empty date configuration.
**	 3-Mar-09 (gordy)
**	    Check scrollable cursor configuration.
**	25-Mar-10 (gordy)
**	    Check batch processing configuration.
**	16-Nov-10 (gordy)
**	    Added CNF_LOC_STRM and config setting for LOB locator
**	    streaming access.
*/

private void
init()
    throws SQLException
{
    String val;

    /*
    ** Initialize random session mask.
    */
    (new Random( System.currentTimeMillis() )).nextBytes( session_mask );
    
    /*
    ** System properties configuration.
    */
    dbms_log = TraceLog.getTraceLog( config.getKey( DRV_DBMS_KEY ), 
				     new ConfigKey( DRV_DBMS_KEY, config ) );

    if ( (val = config.get( DRV_CNF_LOB_CACHE )) != null )
    {
	val = val.trim();

	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ": " + DRV_CNF_LOB_CACHE + "=" + val );

	if ( val.equalsIgnoreCase( "true" ) )
	    cnf_flags |= CNF_LOB_CACHE;
	else  if ( val.equalsIgnoreCase( "false" ) )
	    cnf_flags &= ~CNF_LOB_CACHE;
	else
	    throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    if ( (val = config.get( DRV_CNF_LOB_SEGSIZE )) != null )
    {
	int segSize = DRV_DFLT_SEGSIZE;
	val = val.trim();

	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ": " + DRV_CNF_LOB_SEGSIZE + "=" + val );

	try 
	{ 
	    segSize = Integer.parseInt( val ); 
	    if ( segSize > 0 )  cnf_lob_segSize = segSize;
	}
	catch( Exception ex )
	{ throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE ); }
    }

    if ( (val = config.get( DRV_CNF_LOB_LOCATORS )) != null )
    {
	val = val.trim();

	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ": " + DRV_CNF_LOB_LOCATORS + "=" + val );

	if ( val.equalsIgnoreCase( "true" ) )
	    cnf_flags |= CNF_LOCATORS;
	else  if ( val.equalsIgnoreCase( "false" ) )
	    cnf_flags &= ~CNF_LOCATORS;
	else
	    throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    if ( (cnf_flags & CNF_LOCATORS) == 0 )
	cnf_flags &= ~(CNF_LOC_AUTO | CNF_LOC_LOOP | CNF_LOC_STRM);
    else
    {
	if ( (val = config.get( DRV_CNF_LOB_LOC_AUTO )) != null )
	{
	    val = val.trim();

	    if ( trace.enabled( 3 ) )  
		trace.write( tr_id + ": " + DRV_CNF_LOB_LOC_AUTO + "=" + val );

	    if ( val.equalsIgnoreCase( "true" ) )
		cnf_flags |= CNF_LOC_AUTO;
	    else  if ( val.equalsIgnoreCase( "false" ) )
		cnf_flags &= ~CNF_LOC_AUTO;
	    else
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	}

	if ( (val = config.get( DRV_CNF_LOB_LOC_LOOP )) != null )
	{
	    val = val.trim();

	    if ( trace.enabled( 3 ) )  
		trace.write( tr_id + ": " + DRV_CNF_LOB_LOC_LOOP + "=" + val );

	    if ( val.equalsIgnoreCase( "true" ) )
		cnf_flags |= CNF_LOC_LOOP;
	    else  if ( val.equalsIgnoreCase( "false" ) )
		cnf_flags &= ~CNF_LOC_LOOP;
	    else
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	}

	if ( (val = config.get( DRV_CNF_LOB_LOC_STREAM )) != null )
	{
	    val = val.trim();

	    if ( trace.enabled( 3 ) )  
		trace.write(tr_id + ": " + DRV_CNF_LOB_LOC_STREAM + "=" + val);

	    if ( val.equalsIgnoreCase( "true" ) )
		cnf_flags |= CNF_LOC_STRM;
	    else  if ( val.equalsIgnoreCase( "false" ) )
		cnf_flags &= ~CNF_LOC_STRM;
	    else
		throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
	}
    }

    if ( (val = config.get( DRV_CNF_EMPTY_DATE )) != null )
    {
	val = val.trim();

	if ( trace.enabled( 3 ) )
	    trace.write( tr_id + ": " + DRV_CNF_EMPTY_DATE + "=" + val );

	if ( val.length() == 0  ||  val.equalsIgnoreCase( "empty" ) )
	    cnf_empty_date = "";
	else  if ( val.equalsIgnoreCase( "default" ) )
	    cnf_empty_date = DRV_DFLT_EMPTY_DATE;
	else  if ( val.equalsIgnoreCase( "null" ) )
	    cnf_empty_date = null;
	else
	    cnf_empty_date = val;	// TODO: validate?
    }

    if ( (val = config.get( DRV_CNF_CURS_SCROLL )) != null )
    {
	val = val.trim();

	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ": " + DRV_CNF_CURS_SCROLL + "=" + val );

	if ( val.equalsIgnoreCase( "true" ) )
	    cnf_flags |= CNF_CURS_SCROLL;
	else  if ( val.equalsIgnoreCase( "false" ) )
	    cnf_flags &= ~CNF_CURS_SCROLL;
	else
	    throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    if ( (val = config.get( DRV_CNF_BATCH )) != null )
    {
	val = val.trim();

	if ( trace.enabled( 3 ) )  
	    trace.write( tr_id + ": " + DRV_CNF_BATCH + "=" + val );

	if ( val.equalsIgnoreCase( "true" ) )
	    cnf_flags |= CNF_BATCH;
	else  if ( val.equalsIgnoreCase( "false" ) )
	    cnf_flags &= ~CNF_BATCH;
	else
	    throw SqlExFactory.get( ERR_GC4010_PARAM_VALUE );
    }

    return;
} // init


/*
** Name: connect
**
** Description:
**	Establish a connection to the Data Access Server.  If requested,
**	the connection will enable the management of distributed
**	transactions.
**
** Input:
**	dtmc	Distributed Transaction Management Connection?
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	28-Mar-01 (gordy)
**	    Extracted from EdbcDriver.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	20-Dec-02 (gordy)
**	    Added charset parameter for connection character encoding.
**	    Inform messaging system of the negotiated protocol level.
**	16-Jun-04 (gordy)
**	    Negotiate session mask.
**	 3-Jul-06 (gordy)
**	    Dropped character encoding.
**	26-Feb-07 (gordy)
**	    Check protocol level configuration items.
**	16-Nov-10 (gordy)
**	    Added CNF_LOC_STRM.
*/

private void
connect( boolean dtmc )
    throws SQLException
{
    byte	msg_cp[][] = new byte[1][]; 
    int		len;
    boolean	mask_received = false;

    /*
    ** Build DAM-ML protocol initialization parameters.
    */
    len = 3;						// Protocol
    len += 2 + session_mask.length;			// Session mask
    if ( dtmc )  len += 2;				// DTMC
    msg_cp[0] = new byte[ len ];
    
    len = 0;
    msg_cp[0][len++] = MSG_P_PROTO;			// Protocol
    msg_cp[0][len++] = (byte)1;
    msg_cp[0][len++] = MSG_PROTO_DFLT;
    
    msg_cp[0][len++] = MSG_P_MASK;			// Session mask
    msg_cp[0][len++] = (byte)session_mask.length;
    for( int i = 0; i < session_mask.length; i++ )  
	msg_cp[0][len++] = session_mask[ i ];
    
    if ( dtmc )						// DTMC
    {
	msg_cp[0][len++] = MSG_P_DTMC;
	msg_cp[0][len++] = (byte)0;
    }

    /*
    ** Establish a connection with the server.
    */
    if ( trace.enabled( 2 ) )  
	trace.write( tr_id + ": Connecting to server: " + host ); 

    msg = new MsgConn( host, msg_cp, trace.getTraceLog() );
    tr_id += "[" + msg.connID() + "]";		// Add instance ID

    /*
    ** Process the DAM-ML protocol initialization result paramters
    */
    try
    {
	for( int i = 0; i < msg_cp[0].length - 1; )
	{
	    byte    param_id = msg_cp[0][ i++ ];
	    byte    param_len = msg_cp[0][ i++ ];

	    switch( param_id )
	    {
	    case MSG_P_PROTO :
		if ( param_len != 1  ||  i >= msg_cp[0].length )
		{
		    if ( trace.enabled( 1 ) )  
			trace.write( tr_id + ": Invalid PROTO param length: " +
				param_len + "," + (msg_cp[0].length - i) );
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
		}
		
		msg_protocol_level = msg_cp[0][ i++ ];
		msg.setProtocolLevel( msg_protocol_level );
		break;

	    case MSG_P_DTMC :
		if ( param_len != 0 )
		{
		    if ( trace.enabled( 1 ) )  
			trace.write( tr_id + ": Invalid DTMC param length: " + 
				     param_len );
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
		}

		is_dtmc = true;
		break;

	    case MSG_P_MASK :
		/*
		** Server must respond with matching mask
		** or not at all.
		*/
		if ( param_len != session_mask.length )
		{
		    if ( trace.enabled( 1 ) )  
			trace.write( tr_id + ": Invalid mask param length: " + 
				     param_len );
		    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
		}
		
		/*
		** Combine server session mask with ours.
		*/
		for( int j = 0; j < session_mask.length; j++, i++ )
		    session_mask[ j ] ^= msg_cp[0][ i ];
		
		mask_received = true;
		break;
		
	    default :
		if ( trace.enabled( 1 ) )
		    trace.write(tr_id + ": Invalid param ID: " + param_id);
		throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	    }
	}

	/*
	** Validate protocol.
	*/
	if ( msg_protocol_level < MSG_PROTO_1  ||  
	     msg_protocol_level > MSG_PROTO_DFLT )
	{
	    if ( trace.enabled( 1 ) )
		trace.write( tr_id + ": Invalid MSG protocol: " + 
				     msg_protocol_level );
	    throw SqlExFactory.get( ERR_GC4002_PROTOCOL_ERR );
	}

	/*
	** Validate session mask.
	*/
	if ( ! mask_received )  session_mask = null;
	
	/*
	** Validate DTMC environment.  
	*/
	if ( is_dtmc )
	{
	    if ( ! dtmc )	// Should not happen.
	    {
		if ( trace.enabled( 1 ) )
		    trace.write(tr_id + ": DTM connection not requested!");
		throw SqlExFactory.get( ERR_GC4001_CONNECT_ERR );
	    }

	    if ( msg_protocol_level < MSG_PROTO_2 )
	    {
		if ( trace.enabled( 1 ) )  
		    trace.write( tr_id + ": 2PC not supported" );
		throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
	    }

	    /*
	    ** Autocommit is disabled for DTM connections.
	    */
	    autoCommit = false;
	}
	else  if ( dtmc )
	{
	    if ( trace.enabled( 1 ) )
		trace.write(tr_id+": Could not establish DTM connection!");
	    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
	}
    }
    catch( SQLException ex )
    {
	msg.close();
	throw ex;
    }

    /*
    ** Check protocol dependent configuration
    */
    if ( msg_protocol_level < MSG_PROTO_6 )
	cnf_flags &= ~(CNF_LOCATORS | CNF_LOC_AUTO | 
				      CNF_LOC_LOOP | CNF_LOC_STRM);

    if ( trace.enabled( 2 ) )  
    {
	trace.write( tr_id + ": connected to server" );

	if ( trace.enabled( 3 ) )
	{
	    trace.write( tr_id + ": messaging protocol initialized:" );
	    trace.write( "    MSG protocol: " + msg_protocol_level );
	    if ( is_dtmc )  trace.write( "    DTM Connection" );
	    if ( session_mask != null )  
		trace.write( "    Session mask: " + 
			     session_mask.length + " bytes" );
	}
    }

    return;
} // connect


/*
** Name: finalize
**
** Description:
**	Make sure the server connection is closed.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
*/

protected void
finalize()
    throws Throwable
{
    close();
    super.finalize();
    return;
} // finalize


/*
** Name: close
**
** Description:
**	Close the server connection.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	31-Oct-02 (gordy)
**	    Created.
*/

public void
close()
{
    if ( ! msg.isClosed() )
    {
	if ( trace.enabled( 2 ) )  
	    trace.write( tr_id + ": Disconnecting from server " ); 

	msg.close();

	if ( trace.enabled( 2 ) )
	    trace.write( tr_id + ": disconnected from server" );
    }
    return;
}


/*
** Name: timeValuesInGMT
**
** Description:
**	Returns True if date/time values should use GMT timezone
**	for the current connection.  Returns False if the local
**	timezone should be used.
**
**	Java date/time internal values are GMT as are Ingres dates.
**	The Ingres DBMS does not use the client TZ when processing
**	internal date/time values, so GMT is always used in this case.  
**
**	Gateways also convert to GMT when the client TZ has been sent 
**	to the gateway.  
**
**	If the client TZ is not sent to a gateway, GMT will be used 
**	by the Gateway to convert the host DBMS date/time value into 
**	an Ingres date/time value (effectively no change).  Since  
**	host date/time values are assumed to be in the same TZ as 
**	the client, the local TZ must be used for conversions with 
**	the internal Java representation.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if date/time values are GMT.
**
** History:
**	12-Sep-03 (gordy)
**	    Created.
**	19-Jun-06 (gordy)
**	    Replaced use_gmt_tz with osql_dates and ingres_tz with
**	    is_gmt to better reflect intent.
*/

protected boolean
timeValuesInGMT()
{
    return( ! osql_dates  ||  ! is_gmt );
} // timeValuesInGMT


/*
** Name: timeLiteralsInGMT()
**
** Description:
**	Returns True if date/time literals should use GMT timezone
**	for the current connection.  Returns False if the local
**	timezone should be used.
**
**	Since literals are assumed to be relative to the client TZ,
**	Gateways perform no timezone conversion of literals when
**	converting them to host DBMS format, so literals should be
**	formatted using the local TZ.  
**
**	Ingres uses the client TZ to convert literals to internal 
**	GMT date/time values, so the local TZ should be used when 
**	the client TZ has been sent to the Ingres DBMS.  
**
**	When no TZ has been sent, the Ingres DBMS will use GMT for 
**	literal conversion (effectively no change) so literals must 
**	then be formatted as GMT.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean		True if date/time literals are GMT.
**
** History:
**	 12-Sep-03 (gordy)
**	    Created.
**	19-Jun-06 (gordy)
**	    Replaced use_gmt_tz with osql_dates and ingres_tz with
**	    is_gmt to better reflect intent.
*/

protected boolean
timeLiteralsInGMT()
{
    return( ! osql_dates  &&  is_gmt );
} // timeLiteralsInGMT


/*
** Name: endXact
**
** Description:
**	Declares the end of a transaction on the connection.
**	Transaction boundaries are used to track objects
**	associated with the connection which are transaction
**	sensitive such as cursor and prepared statements.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	 5-Feb-01 (gordy)
**	    Clear the statement ID cache.
**	20-Dec-04 (gordy)
**	    Extracted clearing of prepared statement cache
**	    to method clearPrepStmts().
*/

public synchronized void
endXact()
{
    if ( trace.enabled( 4 ) )
	trace.write( tr_id + ": end of transaction" );
    
    /*
    ** Update unique identifier counters.
    */
    tran_id++;
    obj_id = 0;
    
    /*
    ** Clear the prepared statement cache
    ** when transaction ends.
    */
    clearPrepStmts();
    return;
} // endXact


/*
** Name: getXactID
**
** Description:
**	Returns the current driver internal transaction ID.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	int	    Transaction ID.
**
** History:
**	19-Oct-00 (gordy)
**	    Created.
*/

public int
getXactID()
{
    return( tran_id );
} // getXactID


/*
** Name: getUniqueID
**
** Description:
**	Generates a unique identifier to represent objects
**	associated with the database connection (cursors and
**	statements).  The unique identifier is a combination
**	of the callers provided prefix a hex representation
**	of sequence information associated with the connection.
**
** Input:
**	prefix	    Prefix for the unique identifier.
**
** Output:
**	None.
**
** Returns:
**	String	    The unique identifier.
**
** History:
**	 7-Jun-99 (gordy)
**	    Created.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Don't need to construct an additional string for the string 
**	    which is built.
*/

public synchronized String
getUniqueID( String prefix )
{
    return( prefix + Integer.toHexString( tran_id ) + "_" + 
		     Integer.toHexString( obj_id++ ) );
} // getUniqueID


/*
** Name: createPrepStmt
**
** Description:
**	Returns a reference to a DBMS prepared statement for a query.  
**	Prepared statements are pooled and identical queries share the
**	same prepared statement.  Unused statement names are pooled
**	and re-used to convserve DBMS statement resources.
**
**	This method should be called once for each reference to a
**	prepared statement.  Subsequent usages of a prepared statement
**	should call findPrepStmt().  Prepared statement associations
**	must be released by a final call to dropPrepStmt().
**
**	The referenced prepared statement is physically prepared, even
**	if it was currently prepared in the DBMS.  This guarantees that
**	the current statement is valid even if a preceding DDL statement 
**	invalidated the previous statement.
**	
** Input:
**	query	Query text.
**
** Output:
**	None.
**
** Returns:
**	DrvPrep	Prepared statement.
**
** History:
**	 5-May-04 (gordy)
**	    Created (replaced getStmtID()).
**	10-Nov-06 (gordy)
**	    Added active statement pool and statement name pool.
*/

public DrvPrep
createPrepStmt( String query )
    throws SQLException
{
    DrvPrep prep;

    synchronized( prepStmts )
    {
	/*
	** See if query is currently prepared.
	**
	** Even if statement is current prepared, it will be
	** re-prepared to ensure it has not been invalidated
	** by a preceding DDL statement.
	*/
	prep = (DrvPrep)prepStmts.get( query );
	
	if ( prep != null )
	    prep.prepare();	/* !!! TODO: handle prepare failure */
	else
	{
	    /*
	    ** See if there is an active prepared statement
	    ** (existing associations) for the query.
	    */
	    prep = (DrvPrep)activeStmts.get( query );

	    if ( prep == null )
	    {
		/*
		** See if there is a free statement name available.
		** Otherwise, create a new statement name.
		*/
		String stmtName = null;

		if ( stmtNames.empty() )
		    stmtName = getUniqueID( STMT_PREFIX );
		else
		    try { stmtName = (String)stmtNames.pop(); }
		    catch( Exception ignore ) {}	// Should not happen!

		/*
		** Create and save new statment.
		*/
		prep = new DrvPrep( this, query, stmtName );
		activeStmts.put( query, prep );
	    }

	    prep.prepare();	/* !!! TODO: handle prepare failure */
	    prepStmts.put( query, prep );
	}

	/*
	** Establish association with the statement to ensure
	** statement is kept active until dropped.
	*/
	prep.attach();
    }
    
    return( prep );
} // createPrepStmt


/*
** Name: findPrepStmt
**
** Description:
**	Returns a reference to a DBMS prepared statement for a query.  
**	The first reference to a prepared statement should use the
**	createPrepStmt() method.  This method may then be called
**	multiple times for subsequent references to the prepared
**	statement.  The referenced prepared statement is re-prepared 
**	if it is no longer prepared in the DBMS.
**	
** Input:
**	query	Query text.
**
** Output:
**	None.
**
** Returns:
**	DrvPrep	Prepared statement.
**
** History:
**	10-Nov-06 (gordy)
**	    Created.
*/

public DrvPrep
findPrepStmt( String query )
    throws SQLException
{
    DrvPrep prep;

    synchronized( prepStmts )
    {
	/*
	** See if statement is currently prepared.
	*/
	prep = (DrvPrep)prepStmts.get( query );
	
	if ( prep == null )
	{
	    /*
	    ** Statement should be active but needs to be prepared.
	    */
	    prep = (DrvPrep)activeStmts.get( query );

	    if ( prep == null )
	    {
		/*
		** This is an internal error most likely caused by
		** calling this method after dropPrepStmt().
		*/
	        throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
	    }

	    prep.prepare();
	    prepStmts.put( query, prep );
	}
    }
    
    return( prep );
} // findPrepStmt


/*
** Name: dropPrepStmt
**
** Description:
**	Drop association with a prepared statement.
**	Once all associations have been dropped, the
**	statment name is made available for re-use
**	effectively allowing the DBMS resources to
**	be recovered.
**
**	This method should be called once for each
**	call to createPrepStmt() and after all
**	calls to findPrepStmt().
**
** Input:
**	query
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	10-Nov-06 (gordy)
**	    Created.
*/

public void
dropPrepStmt( String query )
    throws SQLException
{
    DrvPrep prep;

    synchronized( prepStmts )
    {
	/*
	** Retrieve the active statements and
	** drop the association.
	*/
	prep = (DrvPrep)activeStmts.get( query );

	if ( prep == null )
	{
	    /*
	    ** This is an internal error most likely caused
	    ** by calling this method too many times.
	    */
	    throw SqlExFactory.get( ERR_GC4019_UNSUPPORTED );
	}

        prep.detach();

	/*
	** Check to see if the statement has become
	** inactive and its resources re-used.
	*/
	if ( ! prep.isActive() )
	{
	    /*
	    ** Remove statement from active pool.
	    ** Save the statement name for re-use.
	    ** Statement may also be prepared and
	    ** should be removed from the prepared
	    ** pool as well.
	    */
	    prepStmts.remove( query );
	    activeStmts.remove( query );
	    stmtNames.push( prep.getStmtName() );
	}
    }

    return;
} // dropPrepStmt


/*
** Name: clearPrepStmts
**
** Description:
**	Clear the prepared statement pool.  Statements
**	remain active and available for re-use until
**	individually dropped.
**
**	Calling this method ensures statements will be
**	re-prepared upon next reference by createPrepStmt()
**	or findPrepStmt().
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	20-Dec-04 (gordy)
**	    Created.
**	10-Nov-06 (gordy)
**	    Clear the statement names pool.
*/

public void
clearPrepStmts()
{
    synchronized( prepStmts )
    {
	/*
	** In addition to clearing the prepared statement pool,
	** the statement names pool is cleared as well since 
	** the unused names are no longer needed.
	*/
	prepStmts.clear();
	stmtNames.clear();
    }

    return;
} // clearPrepStmts


/*
** Name: loadDbCaps
**
** Description:
**	Requests DBMS capabilities from the server and
**	initialize dbmsinfo() access.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	12-Nov-99 (gordy)
**	    If OPEN_SQL_DATES, use local time rather than gmt.
**	 3-Nov-00 (gordy)
**	    Timezone stuff removed from DbConn.  Replaced with a public 
**	    boolean indicating which timezone to use for the connection.
**	 7-Mar-01 (gordy)
**	    Use new class to read DBMS capabilities.
**	10-May-01 (gordy)
**	    Check DBMS capabilities relevant to driver configuration.
**	20-Feb-02 (gordy)
**	    Added dbmsInfo checks for DBMS type and protocol level.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	23-Dec-02 (gordy)
**	    Connection level ID changed at protocol level 3.
**	 1-Nov-03 (gordy)
**	    Check max column lengths.
**	28-May-04 (gordy)
**	    Add max column lengths for NCS columns.
**	19-Jun-06 (gordy)
**	    Determine Ingres SQL level.
**	26-Feb-07 (gordy)
**	    Disable LOB Locators when unsupported in DBMS.
**	 9-Sep-08 (gordy)
**	    Check for max decimal precision.
**	16-Nov-10 (gordy)
**	    Added CNF_LOC_STRM.
*/

public void
loadDbCaps()
    throws SQLException
{
    String  value;

    /*
    ** Initialize helper classes and load DB capabilities.
    */
    dbInfo = new DbInfo( this );
    dbCaps = new DbCaps( this );
    dbCaps.loadDbCaps();

    /*
    ** Check DB capabilties.
    */
    ;
    if ( (value = dbCaps.getDbCap( (msg_protocol_level < MSG_PROTO_3) 
				   ? DRV_DBCAP_CONNECT_LVL0 
				   : DRV_DBCAP_CONNECT_LVL1 )) != null )
	try { db_protocol_level = Byte.parseByte( value ); }
	catch( Exception ignore ) {}

    if ( db_protocol_level < DBMS_API_PROTO_5 )
	cnf_flags &= ~(CNF_LOCATORS | CNF_LOC_AUTO | 
				      CNF_LOC_LOOP | CNF_LOC_STRM);

    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_DBMS_TYPE )) != null )
	is_ingres = value.equalsIgnoreCase( DBMS_TYPE_INGRES );

    if ( ( value = dbCaps.getDbCap( DBMS_DBCAP_ING_SQL_LVL ) ) != null )  
        try { sqlLevel = Integer.parseInt( value ); }
	catch( Exception ignore ) {}

    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_OSQL_DATES )) != null )  
	osql_dates = true;	// TODO: check for 'LEVEL 1'

    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_UCS2_TYPES )) != null  &&
	 value.length() >= 1 )
	ucs2_supported = (value.charAt(0) == 'Y' || value.charAt(0) == 'y');

    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_MAX_CHR_COL )) != null )  
        try { max_char_len = Integer.parseInt( value ); }
	catch( Exception ignore ) {}
    
    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_MAX_VCH_COL )) != null )  
        try { max_vchr_len = Integer.parseInt( value ); }
	catch( Exception ignore ) {}

    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_MAX_BYT_COL )) != null )  
        try { max_byte_len = Integer.parseInt( value ); }
	catch( Exception ignore ) {}

    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_MAX_VBY_COL )) != null )  
        try { max_vbyt_len = Integer.parseInt( value ); }
	catch( Exception ignore ) {}

    /*
    ** The NCS max column lengths default to half (two bytes per char)
    ** the max for regular columns to match prior behaviour.
    */
    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_MAX_NCHR_COL )) == null )  
	max_nchr_len = max_char_len / 2;
    else
        try { max_nchr_len = Integer.parseInt( value ); }
	catch( Exception ignore ) {}
    
    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_MAX_NVCH_COL )) == null )  
	max_nvch_len = max_vchr_len /2;
    else
        try { max_nvch_len = Integer.parseInt( value ); }
	catch( Exception ignore ) {}

    if ( (value = dbCaps.getDbCap( DBMS_DBCAP_MAX_DEC_PREC )) != null )
	try { max_dec_prec = Integer.parseInt( value ); }
	catch( Exception ignore ) {}

    return;
} // loadDbCaps()


/*
** Name: DbCaps
**
** Description:
**	Provides the ability to access the DBMS capabilities.
**
**  Public Methods:
**
**	getDbCap		Retrieve DBMS capability.
**	loadDbCaps		Load DBMS capabilities.
**
**  Private Data:
**
**	rsmd			Result-set Meta-data.
**	caps			DBMS capability keys and values.
**
**  Private Methods:
**
**	readDesc		Read data descriptor.
**	readData		Read data.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created as extract from EdbcConnect.
**	16-Apr-01 (gordy)
**	    Since the result-set is pre-defined, maintain a single RSMD.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 1-Dec-03 (gordy)
**	    Can't guarantee consistent meta-data across all connections.
**	    RSMD should not be static.
*/

static class	// package access
DbCaps
    extends DrvObj
{

    private JdbcRSMD		rsmd	= null;
    private Hashtable		caps	= new Hashtable();


/*
** Name: DbCaps
**
** Description:
**	Class constructor.
**
** Input:
**	conn	Database connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Added tracing parameter.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public
DbCaps( DrvConn conn )
    throws SQLException
{
    super( conn );
    title = trace.getTraceName() + "-DbCaps[" + msg.connID() + "]";
    tr_id = "DbCap[" + msg.connID() + "]";
    return;
}


/*
** Name: getDbCap
**
** Description:
**	Returns the value associated with a given key.
**
** Input:
**	key	Information key.
**
** Output:
**	None.
**
** Returns:
**	String	Associated value or null.
**
** History:
**	 2-Nov-99 (gordy)
**	    Created.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public String
getDbCap( String key )
{
    return( (String)caps.get( key ) );
} // getDbCap


/*
** Name: loadDBCaps
**
** Description:
**	Query DBMS for capabilities.  Results stored in DrvConn.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 2-Nov-99 (gordy)
**	    Created.
**	17-Dec-99 (gordy)
**	    Add 'for readonly' now that cursor pre-fetch is supported.
**	17-Jan-00 (gordy)
**	    Replaced iidbcapabilities query with a new REQUEST message
**	    to execute a pre-defined server query/statement.
**	 7-Mar-01 (gordy)
**	    Extracted and renamed.
**	10-May-01 (gordy)
**	    Moved capability check to a more relevent method.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public void
loadDbCaps()
    throws SQLException
{
    msg.lock();
    try
    {
	msg.begin( MSG_REQUEST );
	msg.write( MSG_REQ_CAPABILITY );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.log( title + ": error loading DBMS capabilities" );
	throw ex;
    }
    finally
    {
	msg.unlock();
    }
    return;
} // loadDbCaps


/*
** Name: readDesc
**
** Description:
**	Read a data descriptor message.  Overrides default method 
**	in EdbcObj.  It handles the reading of descriptor messages 
**	for the method readDBCaps (above).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD    Data descriptor.
**
** History:
**	13-Jun-00 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Instead of creating a new RSMD on every invocation,
**	    use load() and reload() methods of EdbcRSMD for just
**	    one RSMD.  Return the RSMD, caller can ignore or use.
*/

protected JdbcRSMD
readDesc()
    throws SQLException
{
    if ( rsmd == null )
	rsmd = JdbcRSMD.load( conn );
    else
	rsmd.reload( conn );

    // TODO: validate descriptor
    return( rsmd );
} // readDesc


/*
** Name: readData
**
** Description:
**	Read a data message.  This method overrides the readData
**	method of DrvObj and is called by the readResults method.
**	It handles the reading of data messages for loadDbCaps().  
**	The data should be in a pre-defined format (rows containing 
**	2 non-null varchar columns).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Interrupt reading results.
**
** History:
**	17-Jan-00 (gordy)
**	    Created.
*/

protected boolean
readData()
    throws SQLException
{
    while( msg.moreData() )
    {
	// TODO: validate data
	msg.readByte();				// NULL byte (never null)
	String key = msg.readString();		// Capability name
	msg.readByte();				// NULL byte (never null)
	String val = msg.readString();		// Capability value

	if ( trace.enabled( 4 ) )  
	    trace.write( tr_id + ": '" + key + "' = '" + val + "'" );
	caps.put( key, val );
    }

    return( false );
} // readData


} // class DbCaps


/*
** Name: DbInfo
**
** Description:
**	Provides access to dbmsinfo().
**
**  Public Methods:
**
**	getDbmsInfo		Returns dbmsinfo().
**
**  Private Data:
**
**	rsmd			Result-set Meta-data.
**	request_result		Temp result from readData().
**
**  Private Methods:
**
**	selectDbInfo		Retrieve dbmsinfo() from DBMS.
**	requestDbInfo		Request dbmsinfo() from server.
**	readDesc		Read data descriptor.
**	readData		Read data.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created as extract from EdbcConnect.
**	16-Apr-01 (gordy)
**	    Since the result-set is pre-defined, maintain a single RSMD.
**	    DBMS Information is now returned rather than saving in DbConn.
**	25-Oct-01 (gordy)
**	    Added getDbmsInfo() cover method which restricts request to cache.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	 1-Dec-03 (gordy)
**	    Can't guarantee consistent meta-data across all connections.
**	    RSMD should not be static.
*/

static class	// package access
DbInfo
    extends DrvObj
    implements DrvConst, MsgConst
{

    private JdbcRSMD		rsmd = null;

    /*
    ** The readData() method returns the result value in
    ** the following member (requires synchronization).
    */
    private String		request_result;


/*
** Name: DbInfo
**
** Description:
**	Class constructor.
**
** Input:
**	conn	Database connection.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	28-Mar-01 (gordy)
**	    Added tracing parameter.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public
DbInfo( DrvConn conn )
{
    super( conn );
    title = trace.getTraceName() + "-DbInfo[" + msg.connID() + "]";
    tr_id = "DbInfo[" + msg.connID() + "]";
    return;
}


/*
** Name: getDbmsInfo
**
** Description:
**	Retrieve dbmsinfo() information from DBMS.
**
** Input:
**	id	ID of DBMS info.
**
** Output:
**	None.
**
** Returns:
**	String	DBMS info, may be NULL.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    DBMS info now returned by EdbcDBInfo.readDBInfo().
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

public String
getDbmsInfo( String id )
    throws SQLException
{
    String val;

    if ( conn.msg_protocol_level >= MSG_PROTO_2 )
	val = requestDbInfo( id );
    else
	val = selectDbInfo( id );
    
    return( val );
}


/*
** Name: selectDbInfo
**
** Description:
**	Retrieve dbmsinfo() information from DBMS using a standard
**	driver statement and a SELECT query.  This method of access
**	is not preferred since it likely uses a cursor and may have
**	syntax problems with various DBMS.
**
** Input:
**	id	ID of DBMS info.
**
** Output:
**	None.
**
** Returns:
**	String	DBMS info, may be NULL.
**
** History:
**	25-Oct-01 (gordy)
**	    Added cache_only parameter so as to avoid issuing
**	    server request for items loaded when connection
**	    started.  Make sure 'select dbmsinfo()' query is
**	    readonly.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
**	28-Mar-03 (gordy)
**	    Added cursor holdability.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Ensure resources are closed even if exception occurs.
*/

private String
selectDbInfo( String id )
    throws SQLException
{
    String	val = null;
    Statement	stmt = null;
    ResultSet	rs = null;

    try
    {
	stmt = new JdbcStmt( conn, ResultSet.TYPE_FORWARD_ONLY, 
			DRV_CRSR_READONLY, ResultSet.CLOSE_CURSORS_AT_COMMIT );

	String query = "select dbmsinfo('" + id + "')";
	if ( ! conn.is_ingres )  query += " from iidbconstants";
	
	rs = stmt.executeQuery( query );
	if ( rs.next() ) val = rs.getString(1);
	
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ": error retrieving dbmsinfo()" );
	throw (SQLException)ex;
    }
    finally
    {
	if ( rs != null )  
	    try { rs.close(); }
	    catch( SQLException ignore ) {}

	if ( stmt != null )  
	    try { stmt.close(); }
	    catch( SQLException ignore ) {}
    }

    return( val );
}


/*
** Name: requestDbInfo
**
** Description:
**	Retrieve dbmsinfo() information from DBMS using a pre-defined
**	server request.  This is the prefered method of access since
**	the server adapts the request to the target DBMS.
**
** Input:
**	id	ID of DBMS info.
**
** Output:
**	None.
**
** Returns:
**	String	DBMS info, may be NULL.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Return DBMS information.
**	31-Oct-02 (gordy)
**	    Adapted for generic GCF driver.
*/

private String
requestDbInfo( String id )
    throws SQLException
{
    String  dbmsInfo;

    msg.lock();
    request_result = null;
    try
    {
	msg.begin( MSG_REQUEST );
	msg.write( MSG_REQ_DBMSINFO );
	msg.write( MSG_RQP_INFO_ID );
	msg.write( id );
	msg.done( true );
	readResults();
    }
    catch( SQLException ex )
    {
	if ( trace.enabled( 1 ) )  
	    trace.write( tr_id + ": error requesting dbmsinfo()" );
	throw ex;
    }
    finally
    {
	dbmsInfo = request_result;
	msg.unlock();
    }

    return( dbmsInfo );
} // readDbInfo


/*
** Name: readDesc
**
** Description:
**	Read a data descriptor message.  Overrides default method in 
**	DrvObj.  Handles the reading of descriptor messages for the 
**	method requestDbInfo (above).
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	JdbcRSMD    Data descriptor.
**
** History:
**	  7-Mar-01 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Instead of creating a new RSMD on every invocation,
**	    use load() and reload() methods of EdbcRSMD for just
**	    one RSMD.  Return the RSMD, caller can ignore or use.
*/

protected JdbcRSMD
readDesc()
    throws SQLException
{
    if ( rsmd == null )
	rsmd = JdbcRSMD.load( conn );
    else
	rsmd.reload( conn );

    // TODO: validate descriptor
    return( rsmd );
} // readDesc


/*
** Name: readData
**
** Description:
**	Read a data message.  This method overrides the readData
**	method of DrvObj and is called by the readResults method.  
**	It handles reading of data messages for requestDbInfo() 
**	method (above).  The data should be in a pre-defined 
**	format (row containing 1 non-null varchar column).
**
**	The result data is returned in request_result.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	boolean	    Interrupt reading results.
**
** History:
**	 7-Mar-01 (gordy)
**	    Created.
**	16-Apr-01 (gordy)
**	    Save info result locally rather than in DbConn.
*/

protected boolean
readData()
    throws SQLException
{
    if ( msg.moreData() )
    {
	// TODO: validate data
	msg.readByte();				// NULL byte (never null)
	request_result = msg.readString();	// Information value.
    }

    return( false );
} // readData


} // class DbInfo

} // class DrvConn
