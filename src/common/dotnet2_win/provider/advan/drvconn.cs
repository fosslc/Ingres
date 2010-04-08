/*
** Copyright (c) 2003, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;


namespace Ingres.ProviderInternals
{
	/*
	** Name: drvconn.cs
	**
	** Description:
	**	Defines class representing a Driver connection.
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
	**	    extracting non Connection interface code from Conn
	**	    and connection attribute information from MsgConn.
	**	20-Dec-02 (gordy)
	**	    Support character encoding connection parameter. Inform
	**	    messaging system of the negotiated protocol level.
	**	28-Mar-03 (gordy)
	**	    Updated to 3.0.
	**	 7-Jul-03 (gordy)
	**	    Added Ingres configuration parameters.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 6-jul-04 (gordy; ported by thoda04)
	**	    Added negotiated session mask.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	04-may-06 (thoda04)
	**	    Change default cursor mode to READONLY
	**	    for better default performance.
	**	19-Jun-06 (gordy)
	**	    ANSI Date/Time data type support.
	**	 1-Mar-07 (gordy, ported by thoda04)
	**	    Dropped char_set parameter, don't default CharSet.
	**	15-Sep-08 (gordy, ported by thoda04)
	**	    Added max decimal precision as returned by DBMS.
	**	16-feb-10 (thoda04) Bug 123298
	**	    Added SendIngresDates support to send .NET DateTime parms
	**	    as ingresdate type rather than as timestamp_w_timezone.
	*/



	/*
	** Name: DrvConn
	**
	** Description:
	**	Class representing a Driver connection.  
	**
	**	This class is a central repository for driver information
	**	regarding the DBMS connection. 
	** 
	**  Public Data:
	**
	**	driverName	    Long driver name.
	**	protocol	    Driver URL protocol ID.
	**	host		    Target server address.
	**	database	    Target database name.
	**	dbCaps		    DBMS capabilities.
	**	dbInfo		    Access dbmsinfo().
	**	msg		    DAM-ML message I/O.
	**	msg_protocol_level  DAM-ML protocol level.
	**	db_protocol_level   DBMS protocol level
	**	session_mask        Session mask.
	**	is_dtmc		    Is the connection DTM.
	**	is_ingres	    Is the DBMS Ingres.
	**	sqlLevel	    Ingres SQL Level.
	**	osql_dates	    Use OpenSQL dates?
	**	is_gmt		    DBMS applies GMT timezone?
	**	ucs2_supported	    Is UCS2 data supported?
	**	select_loops	    Are select loops enabled?
	**	cursor_mode	    Default cursor mode.
	**	autoCommit	    Is autocommit enabled.
	**	readOnly	    Is connection READONLY.
	**	trace		    Tracing output.
	**	dbms_log	    DBMS trace log.
	**
	**  Public Methods:
	**
	**	close		    Close server connection.
	**	timeValuesInGMT     Use GMT for date/time values.
	**	timeLiteralsInGMT   Use GMT for date/time literals.
	**	endXact		    Declare end of transaction.
	**	getXactID	    Return internal transaction ID.
	**	getUniqueID	    Return a new unique identifier.
	**	getStmtID	    Return a statement ID for a query.
	**	loadDbCaps	    Load DBMS capabilities.
	**
	**  Private Data:
	**
	**	stmtID		    Statement ID cache.
	**	tran_id		    Transaction ID.
	**	obj_id		    Object ID.
	**	tr_id		    Tracing ID.
	**
	**  Private Methods:
	**
	**	connect		    Establish server connection.
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
	**	    server now has AUTOCOMMIT ON as its default state.
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
	**	    extracting non Connection interface code from Conn
	**	    and connection attribute information from MsgConn.
	**	 7-Jul-03 (gordy)
	**	    Added ingres_tz to indicate that Ingres timezone sent to DBMS.
	**	 6-jul-04 (gordy; ported by thoda04)
	**	    Added session mask of appropriate size for msgconn.encode().
	**	19-Jun-06 (gordy)
	**	    Replaced use_gmt_tz with osql_dates and ingres_tz with
	**	    is_gmt to better represent intent.  Added timeValuesInGMT()
	**	    and timeLiteralsInGMT() (from DrvObj) and sqlLevel.
	*/

	internal class
		DrvConn : MsgConst
	{

		/*
		** Connection objects.
		*/
		public  MsgConn msg = null;
		public  AdvanConnect advanConn = null;
		public  IConfig config   = null;
		public  ITrace trace     = null;
		public  ITrace  dbms_log = null;
		internal DbCaps dbCaps   = null;
		internal DbInfo dbInfo   = null;

		/*
		** Driver identifiers.
		*/
		public  String	driverName		= "Ingres .NET Data Provider";
		public  String	protocol		= "?";

		/*
		** Connection status.
		*/
		public  String	host			= null;
		public  String	database		= null;
		public  byte	msg_protocol_level	= MSG_PROTO_1;
		public  byte	db_protocol_level 	= DBMS_API_PROTO_1;
		public  byte[]	session_mask      	= new byte[CI.CRYPT_SIZE];
		public  bool	is_dtmc			= false;
		public  bool	is_ingres		= true;
		public  int 	sqlLevel = 0;
		/// <summary>
		/// How the native timestamp type reacts to timezones, lack of it indicates an Ingres DBMS.
		/// </summary>
		public  bool	osql_dates = false;
		public  bool	is_gmt = true;
		public  bool	ucs2_supported = false;
		public  bool	select_loops		= true;
		public  int 	cursor_mode		= DrvConst.DRV_CRSR_READONLY;
		public  bool	sendIngresDates	= false;
		public  bool	autoCommit		= true;
		public  bool	readOnly		= false;

		public  bool	isBlankDateNull	= false;

		public int  	max_char_len = DBMS_COL_MAX;
		public int  	max_vchr_len = DBMS_COL_MAX;
		public int  	max_byte_len = DBMS_COL_MAX;
		public int  	max_vbyt_len = DBMS_COL_MAX;
		public int  	max_nchr_len = DBMS_COL_MAX / 2;
		public int  	max_nvch_len = DBMS_COL_MAX / 2;
		public int		max_dec_prec = DBMS_PREC_MAX;

    
		/*
		** Local data.
		*/
		private System.Collections.Hashtable	stmtID =
			new System.Collections.Hashtable();
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
		**	config	Configuration properties.
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
		**	 6-jul-04 (gordy; ported by thoda04)
		**	    Initialize session mask with random values.
		**	 1-Mar-07 (gordy, ported by thoda04)
		**	    Character encoding now handled with other driver properties.
		*/

		/// <summary>
		/// Connect to the Data Access Server.
		/// </summary>
		/// <param name="host"></param>
		/// <param name="config"></param>
		/// <param name="trace"></param>
		/// <param name="dtmc"></param>
		public
			DrvConn( String host, IConfig config, ITrace trace, bool dtmc )
		{
			this.host = host;
			this.trace = trace;
			this.config = config;

			dbms_log = new TraceDV(DrvConst.DRV_DBMS_KEY); // "dbms"

			/*
			** Initialize random session mask.
			*/
			(new Random( unchecked((int)System.DateTime.Now.Ticks) )
				).NextBytes( session_mask );

			connect( dtmc );
			return;
		} // DrvConn


		/*
		** Name: connect
		**
		** Description:
		**	Establish a connection to the Data Access Server.  If requested,
		**	the connection will enable the management of distributed
		**	transactions.
		**
		** Input:
		**	charset	Character set/encoding for connection..
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
		**	    Extracted from Driver.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		**	20-Dec-02 (gordy)
		**	    Added charset parameter for connection character encoding.
		**	    Inform messaging system of the negotiated protocol level.
		**	 6-jul-04 (gordy; ported by thoda04)
		**	    Negotiate session mask.
		**	 1-Mar-07 (gordy, ported by thoda04)
		**	    Dropped char_set parameter, don't default CharSet.
		*/

		private void
		connect( bool dtmc )
		{
			byte[] 	msg_cp; 
			int    	len;
			bool   	mask_received = false;

			/*
			** Build DAM-ML protocol initialization parameters.
			*/
			len = 3;                                // Protocol
			len += (2 + session_mask.Length);       // Session mask
			if ( dtmc )  len += 2;                  // DTMC
			msg_cp = new byte[ len ];

			len = 0;
			msg_cp[len++] = MSG_P_PROTO;		// Protocol
			msg_cp[len++] = (byte)1;
			msg_cp[len++] = MSG_PROTO_DFLT;

			msg_cp[len++] = MSG_P_MASK; 		// Session mask
			msg_cp[len++] = (byte)session_mask.Length;
			for( int i = 0; i < session_mask.Length; i++ )  
				msg_cp[len++] = session_mask[ i ];

			if ( dtmc )                 		// DTMC
			{
				msg_cp[len++] = MSG_P_DTMC;
				msg_cp[len++] = (byte)0;
			}

			/*
			** Establish a connection with the server.
			*/
			if ( trace.enabled( 2 ) )  
				trace.write( tr_id + ": Connecting to server: " + host ); 

			msg = new MsgConn( host, ref msg_cp );
			tr_id += "[" + msg.ConnID + "]";		// Add instance ID

			/*
			** Process the DAM-ML protocol initialization result paramters
			*/
			try
			{
				for( int i = 0; i < msg_cp.Length - 1; )
				{
					byte    param_id = msg_cp[ i++ ];
					byte    param_len = msg_cp[ i++ ];

					switch( param_id )
					{
						case MSG_P_PROTO :
							if ( param_len != 1  ||  i >= msg_cp.Length )
							{
								if ( trace.enabled( 1 ) )  
									trace.write( tr_id + ": Invalid PROTO param length: " +
										param_len + "," + (msg_cp.Length - i) );
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
							}
		
							msg_protocol_level = msg_cp[ i++ ];
							msg.ML_ProtocolLevel = msg_protocol_level;
							break;

						case MSG_P_DTMC :
							if ( param_len != 0 )
							{
								if ( trace.enabled( 1 ) )  
									trace.write( tr_id + ": Invalid DTMC param length: " + 
										param_len );
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
							}

							is_dtmc = true;
							break;

						case MSG_P_MASK :
							/*
							** Server must respond with matching mask
							** or not at all.
							*/
							if ( param_len != session_mask.Length )
							{
								if ( trace.enabled( 1 ) )  
									trace.write( tr_id +
										": Invalid mask param length: " + 
										param_len );
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
							}
		
							/*
							** Combine server session mask with ours.
							*/
							for( int j = 0; j < session_mask.Length; j++, i++ )
								session_mask[ j ] ^= msg_cp[ i ];
		
							mask_received = true;
							break;
		
						default :
							if ( trace.enabled( 1 ) )
								trace.write(tr_id + ": Invalid param ID: " + param_id);
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
					}
				}

				/*
				** Validate protocol.
				*/
				if (msg_protocol_level < MSG_PROTO_1  ||  
					msg_protocol_level > MSG_PROTO_DFLT )
				{
					if ( trace.enabled( 1 ) )
						trace.write( tr_id + ": Invalid MSG protocol: " + 
							msg_protocol_level );
					throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
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
						throw SqlEx.get( ERR_GC4001_CONNECT_ERR );
					}

					if ( msg_protocol_level < MSG_PROTO_2 )
					{
						if ( trace.enabled( 1 ) )  
							trace.write( tr_id + ": 2PC not supported" );
						throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
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
					throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
				}
			}
			catch( SqlEx ex )
			{
				msg.close();
				throw ex;
			}

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
							session_mask.Length + " bytes" );
				}
			}

			return;
		} // connect


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
			if ( ! ( (msg == null)  || msg.isClosed()) )
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

		public bool
		timeValuesInGMT()
		{
			return (!osql_dates || !is_gmt);
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

		public bool
		timeLiteralsInGMT()
		{
			return (!osql_dates && is_gmt);
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
		*/

		public void
			endXact()
		{
			lock(this)
			{
				if ( trace.enabled( 4 ) )
					trace.write( tr_id + ": end of transaction" );
				tran_id++;
				obj_id = 0;
				stmtID.Clear();
			}
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
		}


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
		*/

		public String
			getUniqueID( String prefix )
		{
			lock(this)
			{
				return( prefix + SqlData.toHexString( tran_id ) + 
					"_" + SqlData.toHexString( obj_id++ ) );
			}
		} // uniqueID


		/*
		** Name: getStmtID
		**
		** Description:
		**	Returns a statement ID for a query.  The statment ID
		**	is generated from the prefix using getUniqueID().  For
		**	any single query text, the same statement ID will be
		**	for each call, as long as the statement ID is valid
		**	on the server.
		**
		** Input:
		**	query	Query text.
		**	prefix	Statement ID prefix.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Statement ID.
		**
		** History:
		**	 5-Feb-01 (gordy)
		**	    Created.
		*/

		public String
			getStmtID( String query, String prefix )
		{
			String stmt;

			lock(this)
			{
				if ( (stmt = (String)stmtID[ query ]) == null )
				{
					stmt = getUniqueID( prefix );
					stmtID[ query ]= stmt;
				}
			}

			return( stmt );
		}


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
		*/

		public void
			loadDbCaps()
		{
			String  value;
			int     ivalue;

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
				? DrvConst.DRV_DBCAP_CONNECT_LVL0 
				: DrvConst.DRV_DBCAP_CONNECT_LVL1 )) != null )
				try { db_protocol_level = Byte.Parse( value ); }
				catch( Exception ) {}

			if ( (value = dbCaps.getDbCap( DBMS_DBCAP_DBMS_TYPE )) != null )
				is_ingres = ToInvariantUpper(value).Equals( DBMS_TYPE_INGRES );

			if ((value = dbCaps.getDbCap(DBMS_DBCAP_ING_SQL_LVL)) != null)
				try { sqlLevel = Int32.Parse(value); }
				catch (Exception) { }

			// How the native timestamp type reacts to timezones,
			// lack of it indicates an Ingres DBMS
			if ((value = dbCaps.getDbCap(DBMS_DBCAP_OSQL_DATES)) != null)
				osql_dates = true;	// TODO: check for 'LEVEL 1'

			if ( (value = dbCaps.getDbCap( DBMS_DBCAP_UCS2_TYPES )) != null  &&
				value.Length >= 1 )
				ucs2_supported = (value[0] == 'Y' || value[0] == 'y');

			if ((value = dbCaps.getDbCap(DBMS_DBCAP_MAX_CHR_COL)) != null)
				try { max_char_len = Int32.Parse(value); }
				catch (Exception) { }

			if ((value = dbCaps.getDbCap(DBMS_DBCAP_MAX_VCH_COL)) != null)
				try { max_vchr_len = Int32.Parse(value); }
				catch (Exception) { }

			if ((value = dbCaps.getDbCap(DBMS_DBCAP_MAX_BYT_COL)) != null)
				try { max_byte_len = Int32.Parse(value); }
				catch (Exception) { }

			if ((value = dbCaps.getDbCap(DBMS_DBCAP_MAX_VBY_COL)) != null)
				try { max_vbyt_len = Int32.Parse(value); }
				catch (Exception) { }

			/*
			** The NCS max column lengths default to half (two bytes per char)
			** the max for regular columns to match prior behaviour.
			*/
			if ((value = dbCaps.getDbCap(DBMS_DBCAP_MAX_NCHR_COL)) == null)
				max_nchr_len = max_char_len / 2;
			else
				try { max_nchr_len = Int32.Parse(value); }
				catch (Exception)  { }

			if ((value = dbCaps.getDbCap(DBMS_DBCAP_MAX_NVCH_COL)) == null)
				max_nvch_len = max_vchr_len / 2;
			else
				try { max_nvch_len = Int32.Parse(value); }
				catch (Exception) { }

			if ((value = dbCaps.getDbCap(DBMS_DBCAP_MAX_DEC_PREC)) != null)
				if (Int32.TryParse(value, out ivalue))  // convert value to int ivalue
					max_dec_prec = ivalue;

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
		**	    Created as extract from AdvanConnect.
		**	16-Apr-01 (gordy)
		**	    Since the result-set is pre-defined, maintain a single RSMD.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		*/

		internal class DbCaps : DrvObj
		{

			private static AdvanRSMD	rsmd	= null;
			private System.Collections.Hashtable caps	=
				new System.Collections.Hashtable();


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
				DbCaps( DrvConn conn ) : base( conn )
			{
				title = base.trace.getTraceName() + "-DbCaps[" + base.msg.ConnID + "]";
				base.tr_id = "DbCap[" + base.msg.ConnID + "]";
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
				return( (String)caps[ key ] );
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
			{
				base.msg.LockConnection();
				try
				{
					base.msg.begin( MSG_REQUEST );
					base.msg.write( MSG_REQ_CAPABILITY );
					base.msg.done( true );
					readResults();
				}
				catch( SqlEx ex )
				{
					if ( base.trace.enabled( 1 ) )  
						base.trace.log( title + ": error loading DBMS capabilities" );
					throw ex;
				}
				finally
				{
					base.msg.UnlockConnection();
				}
				return;
			} // loadDbCaps


			/*
			** Name: readDesc
			**
			** Description:
			**	Read a data descriptor message.  Overrides default method 
			**	in AdvanObj.  It handles the reading of descriptor messages 
			**	for the method readDBCaps (above).
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	AdvanRSMD    Data descriptor.
			**
			** History:
			**	13-Jun-00 (gordy)
			**	    Created.
			**	16-Apr-01 (gordy)
			**	    Instead of creating a new RSMD on every invocation,
			**	    use load() and reload() methods of AdvanRSMD for just
			**	    one RSMD.  Return the RSMD, caller can ignore or use.
			*/

			protected internal override AdvanRSMD
				readDesc()
			{
				if ( rsmd == null )
					rsmd = AdvanRSMD.load( conn );
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

			protected internal override bool
				readData()
			{
				while( base.msg.moreData() )
				{
					// TODO: validate data
					base.msg.readByte();			// NULL byte (never null)
					String key = base.msg.readString();	// Capability name
					base.msg.readByte();			// NULL byte (never null)
					String val = base.msg.readString();	// Capability value

					if ( base.trace.enabled( 4 ) )  
						base.trace.write( base.tr_id + ".setDbCap('" + 
							key + "','" + val + "')");
					caps[ key ] = val;
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
		**	    Created as extract from AdvanConnect.
		**	16-Apr-01 (gordy)
		**	    Since the result-set is pre-defined, maintain a single RSMD.
		**	    DBMS Information is now returned rather than saving in DbConn.
		**	25-Oct-01 (gordy)
		**	    Added getDbmsInfo() cover method which restricts request to cache.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		*/

		internal class DbInfo : DrvObj
		{

			private static AdvanRSMD	rsmd = null;

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
				DbInfo( DrvConn conn ) : base( conn )
			{
				title = base.trace.getTraceName() + "-DbInfo[" + base.msg.ConnID + "]";
				base.tr_id = "DbInfo[" + base.msg.ConnID + "]";
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
			**	    DBMS info now returned by AdvanDBInfo.readDBInfo().
			**	31-Oct-02 (gordy)
			**	    Adapted for generic GCF driver.
			*/

			public String
				getDbmsInfo( String id )
			{
				String val;

				if ( this.conn.msg_protocol_level >= MSG_PROTO_2 )
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
			*/

			private String
				selectDbInfo( String id )
			{
				String val = null;

				try
				{
					System.Data.IDataReader rs;
					System.Data.IDbCommand  cmd =
						conn.advanConn.Connection.CreateCommand();

					String query = "select dbmsinfo('" + id + "')";
					if ( ! conn.is_ingres )  query += " from iidbconstants";
					
					cmd.CommandText = query;
					rs = cmd.ExecuteReader();
					if (rs.Read())
						val = System.Convert.ToString(rs[0]);
					rs.Close();
					cmd.Dispose();
				}
				catch( SqlEx ex )
				{
					if ( trace.enabled( 1 ) )  
						trace.write( tr_id + ": error retrieving dbmsinfo()" );
					throw ex;
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
			{
				String  dbmsInfo;

				base.msg.LockConnection();
				request_result = null;
				try
				{
					base.msg.begin( MSG_REQUEST );
					base.msg.write( MSG_REQ_DBMSINFO );
					base.msg.write( MSG_RQP_INFO_ID );
					base.msg.write( id );
					base.msg.done( true );
					readResults();
				}
				catch( SqlEx ex )
				{
					if ( base.trace.enabled( 1 ) )  
						base.trace.write( base.tr_id + ": error requesting dbmsinfo()" );
					throw ex;
				}
				finally
				{
					dbmsInfo = request_result;
					base.msg.UnlockConnection();
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
			**	AdvanRSMD    Data descriptor.
			**
			** History:
			**	  7-Mar-01 (gordy)
			**	    Created.
			**	16-Apr-01 (gordy)
			**	    Instead of creating a new RSMD on every invocation,
			**	    use load() and reload() methods of AdvanRSMD for just
			**	    one RSMD.  Return the RSMD, caller can ignore or use.
			*/

			protected internal override AdvanRSMD
				readDesc()
			{
				if ( rsmd == null )
					rsmd = AdvanRSMD.load( conn );
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

			protected internal override bool
				readData()
			{
				if ( base.msg.moreData() )
				{
					// TODO: validate data
					base.msg.readByte();			    // NULL byte (never null)
					request_result = base.msg.readString();    // Information value.
				}

				return( false );
			} // readData


		} // class DbInfo

	} // class DrvConn
}  // namespace
