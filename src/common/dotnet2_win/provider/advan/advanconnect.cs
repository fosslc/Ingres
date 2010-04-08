/*
** Copyright (c) 2002, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using System.Data;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: advanconnect.cs
	**
	** Description:
	**	Implements the internal Connection class: AdvanConnect.
	**
	**  Classes
	**
	**	AdvanConnect    	Implements the internal Connection interface.
	**
	** History:
	**	 5-May-99 (gordy)
	**	    Created.
	**	 8-Sep-99 (gordy)
	**	    Created new base class for class which interact with
	**	    the server and extracted common data and methods.
	**	    Synchronize entire request/response with server.
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	22-Sep-99 (gordy)
	**	    Convert user/password to server character set when encrypting.
	**	29-Sep-99 (gordy)
	**	    Used DbConn lock()/unlock() for synchronization to
	**	    support BLOB streams.
	**	26-Oct-99 (gordy)
	**	    Implemented ODBC escape parsing.
	**	 2-Nov-99 (gordy)
	**	    Read and save DBMS information.
	**	 4-Nov-99 (gordy)
	**	    Send timeout connection parameter to server.
	**	12-Nov-99 (gordy)
	**	    Gateways require the driver to use local time, not gmt.
	**	17-Dec-99 (gordy)
	**	    Use 'readonly' cursors.
	**	17-Jan-00 (gordy)
	**	    Connection pool parameter sent as boolean value.  Autocommit
	**	    is now enable by the server at connect time and disabled at
	**	    disconnect.  Replaced iidbcapabilities query with a request
	**	    for a pre-defined server query.  Added readData() method to
	**	    read the results of the server request.
	**	19-May-00 (gordy)
	**	    Check for select loops connection parameter (internal) and 
	**	    set the DB connection state appropriatly.
	**	13-Jun-00 (gordy)
	**	    Implemented AdvanCall class and unstubbed prepareCall().
	**	    REQUEST messages may return a descriptor so implemented
	**	    readDesc() method.
	**	30-Aug-00 (gordy)
	**	    Provide a more meaningful exception when user ID or password
	**	    is a zero-length string.
	**	27-Sep-00 (gordy)
	**	    Don't perform any action if requested readOnly state
	**	    is the same as the current readOnly state.
	**	 4-Oct-00 (gordy)
	**	    Standardized internal tracing.
	**	17-Oct-00 (gordy)
	**	    Added autocommit mode connection parameter.
	**	30-Oct-00 (gordy)
	**	    Added support for 2.0 extensions.
	**	20-Jan-01 (gordy)
	**	    Added messaging protocol level 2.
	**	 7-Mar-01 (gordy)
	**	    Revamped the DBMS capabilities processing so accomodate
	**	    support for SQL dbmsinfo() queries.
	**	21-Mar-01 (gordy)
	**	    Added support for distributed transactions with methods
	**	    startTransaction() and prepareTransaction().
	**	28-Mar-01 (gordy)
	**	    Moved server connection establishment into this class and
	**	    placed server and database connection code into separate
	**	    methods.  Tracing now passed as parameter in constructors.  
	**	    Dropped getDbConn().
	**	 2-Apr-01 (gordy)
	**	    Use AdvanXID, support Ingres transaction ID.
	**	11-Apr-01 (gordy)
	**	    Added timeout parameter to constructor.
	**	16-Apr-01 (gordy)
	**	    Optimize usage of Result-set Meta-data.
	**	18-Apr-01 (gordy)
	**	    Added support for Distributed Transaction Management Connections.
	**	10-May-01 (gordy)
	**	    Added support for UCS2 datatypes.
	**	 27-Jul-01 (loera01) Bug 105327
	**	    For setTransactionIsolation method, reject attempts to set the 
	**	    transaction level for anything other than serializable, if the 
	**	    SQL level of the dbms is 6.4.
	**	30-Jul-01 (loera01) Bug 105356
	**	    Since setTransactionIsolation() and setReadOnly() may cause
	**	    unexpected results when invoked separately, create a new private
	**	    method, sendTransactionQuery, which is invoked by the two methods
	**	    above.  The sendTransactionQuery method takes AdvanConnect's current
	**	    transaction isolation and read-only settings into account before
	**	    sending a SET SESSION query.
	**	20-Aug-01 (gordy)
	**	    Added connection parameter for default cursor mode.  Support
	**	    default and updatable statement concurrency modes.
	**	25-Oct-01 (gordy)
	**	    Only issue dbmsinfo() query for items which aren't pre-loaded
	**	    into the info cache.
	**	20-Feb-02 (gordy)
	**	    Added dbmsInfo checks for DBMS type and protocol level.
	**	26-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	30-sep-02 (loera01) Bug 108833
	**	    Since the intention of the setTransactionIsolaton and
	**	    setReadOnly methods is for the "set" query to persist in the
	**	    session, change "set transaction" to "set session" in the
	**	    sendTransactionQuery private method.
	**	25-Feb-04 (thoda04)
	**	    Convert SqlEx exception to an IngresException.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 6-jul-04 (gordy; ported by thoda04)
	**	    Added session_mask to password encryption.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	 1-Mar-07 (gordy, ported by thoda04)
	**	    Add char_encoding support.
	**	24-jul-07 (thoda04) Bug 119116
	**	    Write the role password along with the roleID.
	**	27-sep-07 (thoda04) Bug 119188
	**	    Test for severed socket into and out of connection pool.
	**	01-feb-08 (thoda04)
	**	    Added CloseOrPutBackIntoConnectionPool for TransactionScope support.
	**	10-aug-09 (thoda04)
	**	    To support commit/rollback by xid rather than by handle
	**	    for distributed transaction, ported Gordy's methods of
	**	    startTransaction(xid, flags), endTransaction(xid)
	**	    commit(xid), rollback(xid), abortTransaction(xid).
	**	16-feb-10 (thoda04) Bug 123298
	**	    Added SendIngresDates support to send .NET DateTime parms
	**	    as ingresdate type rather than as timestamp_w_timezone.
	*/


	/*
	** Name: AdvanConnect
	**
	** Description:
	**	Driver class which implements the
	**	Connection interface.
	**
	**  Interface Methods:
	**
	**	createStatement		Create a Statement object.
	**	prepareStatement	Create a PreparedStatement object.
	**	prepareCall		Create a CallableStatement object
	**	nativeSQL		Translate SQL into native DBMS SQL.
	**	setAutoCommit		Set autocommit state.
	**	getAutoCommit		Determine autocommit state.
	**	commit			Commit current transaction.
	**	rollback		Rollback current transaction.
	**	close			Close the connection.
	**	isClosed		Determine if connection is closed.
	**	getMetaData		Create a DatabaseMetaData object.
	**	setReadOnly		Set readonly state
	**	isReadOnly		Determine readonly state.
	**	setTransactionIsolation	Set the transaction isolation level.
	**	getTransactionIsolation	Determine the transaction isolation level.
	**	getTypeMap		Retrieve the type map.
	**	setTypeMap		Set the type map.
	**
	**  Public Methods:
	**
	**	startTransaction	Start distributed transaction.
	**	prepareTransaction	Prepare distributed transaction.
	**	getDbmsInfo		Retrieve DBMS info and capabilities.
	**
	**  Protected Data
	**
	**	host			Host address and port.
	**	database		Target database specification.
	**
	**  Private Data
	**
	**	autoCommit		Autocommit is ON?
	**	readOnly		Connection is readonly?
	**	isolationLevel		Current transaction isolationLevel.
	**	type_map		Map for DBMS datatypes.
	**	dbInfo			DBMS information access.
	**	dbXids			Distributed XID access.
	**
	**  Private Methods:
	**
	**	disconnect		Shutdown DB connection.
	**
	** History:
	**	 5-May-99 (gordy)
	**	    Created.
	**	 8-Sep-99 (gordy)
	**	    Created new base class for class which interact with
	**	    the server and extracted common data and methods.
	**	    Synchronize entire request/response with server.
	**	 2-Nov-99 (gordy)
	**	    Add getDbmsInfo(), readDbmsInfo() methods.
	**	11-Nov-99 (rajus01)
	**	    Add field url.
	**	17-Jan-00 (gordy)
	**	    server now has AUTOCOMMIT ON as its default state.
	**	30-Oct-00 (gordy)
	**	    Support 2.0 extensions.  Added type_map, getTypeMap(),
	**	    setTypeMap(), createStatement(), prepareStatement(),
	**	    prepareCall(), and getDbConn().  Removed getDbmsInfo().
	**	 7-Mar-01 (gordy)
	**	    Replace readDbmsInfo() method with class to perform same
	**	    operation.  Add getDbmsInfo() method and companion class
	**	    which implements the SQL dbmsinfo() function.  Added dbInfo
	**	    field for the getDbmsInfo() method companion class.
	**	21-Mar-01 (gordy)
	**	    Added support for distributed transactions with methods
	**	    startTransaction() and prepareTransaction().
	**	28-Mar-01 (gordy)
	**	    Replaced URL with host and database.  Dropped getDbConn() 
	**	    as msg now has package access.
	**	18-Apr-01 (gordy)
	**	    Added support for Distributed Transaction Management Connections.
	**	    Added getPreparedTransactionIDs() method and constructor which
	**	    establishes a DTMC.  Added is_dtmc flag and AdvanXids request class.
	**	25-Oct-01 (gordy)
	**	    Added getDbmsInfo() cover method which restricts request to cache.
	**	01-feb-08 (thoda04)
	**	    Added CloseOrPutBackIntoConnectionPool for TransactionScope support.
	*/

	/// <summary>
	/// Represents an internal connection to the Ingres session.
	/// Usually associated on a 1-to-1 basis with the external IngresConnection
	/// object, but the internal connection can be disassociated on its own if
	/// it is in the connection pool, or if it is used for finishing up
	/// a MSDTC two-phase transaction commit.
	/// </summary>
	internal class AdvanConnect : DrvObj
	{
		/*
		** Connection status.
		*/
		internal String host = null;
		internal String database = null;
		internal byte   msg_protocol_level	= MSG_PROTO_1;
		internal byte   db_protocol_level 	= DbmsConst.DBMS_API_PROTO_1;
		/// <summary>
		/// If connected to an Ingres server then true.
		/// </summary>
		private  bool   autoCommit = true;
		private  bool   readOnly = false;

		internal ConnectionPool connectionPool = null;  // isa member of this pool
		internal DateTime expirationDate = DateTime.MaxValue;  // expiration from pool
		
		private const int OI_SQL_LEVEL = 605;

		/// <summary>
		/// Count of application thread and/or MSDTC thread
		/// using this advanConnect instance concurrently.
		/// Current possible values are 0, 1, or 2.
		/// </summary>
		private int referenceCount = 0; 
		
		private System.Data.IsolationLevel isolationLevel = IsolationLevel.Serializable;
		private Hashtable type_map;
		private AdvanXids dbXids = null;
		private IDbConnection providerConnection = null;
		
		
		/*
		** Name AdvanConnect
		**
		** Description:
		**	Class constructor.  Common functionality for other
		**	constructors.  Initialize tracing.
		**
		** Input:
		**	providerConnection	IngresConnection
		**	host              	Target host and port.
		**	trace             	Connection tracing.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	18-Apr-01 (gordy)
		**	    Extracted from other constructors.*/
		
		internal AdvanConnect(
			IDbConnection providerConnection,
			String host,
			ITrace trace) :
				base(trace)
		{
			isolationLevel = System.Data.IsolationLevel.Serializable;
			type_map       = new System.Collections.Hashtable();

			this.providerConnection = providerConnection;  // IngresConnection
			this.host = host;
			title = DrvConst.shortTitle + "-Connect";
			tr_id = "Conn";
		}  // AdvanConnect
		
		
		/*
		** Name: AdvanConnect
		**
		** Description:
		**	Class constructor.  Send connection request to server
		**	containing connection parameters.  Throws SQLException
		**	if connection request fails.
		**
		** Input:
		**	host	Target host and port.
		**	info	Property list.
		**	trace	Connection tracing.
		**	timeout	Connection timeout.
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
		**	17-Sep-99 (rajus01)
		**	    Added connection parameters.
		**	22-Sep-99 (gordy)
		**	    Convert user/password to server character set when encrypting.
		**	 2-Nov-99 (gordy)
		**	    Read DBMS information after establishing connection.
		**	 4-Nov-99 (gordy)
		**	    Send timeout connection parameter to server.
		**	17-Jan-00 (gordy)
		**	    Connection pool parameter sent as boolean value.  No longer
		**	    need to turn on autocommit since it is the default state for
		**	    the server.
		**	19-May-00 (gordy)
		**	    Check for select loops connection parameter (internal) and 
		**	    set the DB connection state appropriatly.
		**	30-Aug-00 (gordy)
		**	    BUG #102472 - Fix the exception returned if user ID or
		**	    password are zero-length strings.  A zero-length string
		**	    was not being caught by the null test and was causing an
		**	    array exception during the password encoding.  This was
		**	    caught by intermediate code and turned into a 'character
		**	    encoding' exception.  Now, null and zero-length valued
		**	    parameters are skipped, which for user ID and password
		**	    result in a reasonably specific error from the server.
		**	 4-Oct-00 (gordy)
		**	    Create unique ID for standardized internal tracing.
		**	17-Oct-00 (gordy)
		**	    Added autocommit mode connection parameter.
		**	20-Jan-01 (gordy)
		**	    Autocommit mode only supported at protocol level 2 or higher.
		**	 7-Mar-01 (gordy)
		**	    Use new class to read DBMS capabilities.
		**	28-Mar-01 (gordy)
		**	    Moved server connection establishment into this class and
		**	    placed server and database connection code into separate
		**	    methods.  Replaced msg parameter with host.  Dropped URL
		**	    as a parameter and added tracing as a parameter.
		**	11-Apr-01 (gordy)
		**	    Added timeout parameter for dbms connection.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.*/
		
		internal AdvanConnect(IDbConnection providerConnection,
		                   String  host,
		                   IConfig config,
		                   ITrace  trace,
		                   int     timeout) :
		               this(providerConnection, host, trace)
		{
			// connect to the Data Access Server
			conn = new DrvConn(host, config, trace, false);
			set(conn);  // set the DrvConn into this DrvObj.
			conn.advanConn = this;

			dbms_connect(config, timeout, null);
			conn.loadDbCaps();
		} // AdvanConnect
		
		
		/*
		** Name: AdvanConnect
		**
		** Description:
		**	Class constructor.  Establish a distributed transaction
		**	management connection.  An XID may be provided, in which
		**	case the server will attempt to assign ownership of the
		**	transaction to the new connection.
		**
		** Input:
		**	host	Target host and port.
		**	info	Property list.
		**	trace	Connection tracing.
		**	xid	Distributed transaction ID, may be NULL.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	18-Apr-01 (gordy)
		**	    Created.*/
		
		internal AdvanConnect(IDbConnection providerConnection,
		                   String   host,
		                   IConfig  config,
		                   ITrace   trace,
		                   AdvanXID xid) :
		               this(providerConnection, host, trace)
		{
			if (xid != null)
				switch (xid.Type)
				{
					case AdvanXID.XID_INGRES:
					case AdvanXID.XID_XA: 
						// OK!
						break;
						
					
					
					default: 
						if (trace.enabled(1))
							trace.write(tr_id + ": unknown XID type: " + xid.Type);
						throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
					
				}
			
			// connect to the Data Access Server
			DrvConn conn = new DrvConn(host, config, trace, true);
			set(conn);  // set the DrvConn into this DrvObj.
			conn.advanConn = this;

			if (conn.msg_protocol_level < MSG_PROTO_2)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": 2PC protocol = " + conn.msg_protocol_level);
				conn.close();
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}
			
			dbms_connect(config, 0, xid);
		} // AdvanConnect
		
		
		public IDbConnection Connection
		{
			/*
			* The user should be able to set or change the connection at 
			* any time.
			*/
			get { return providerConnection;  }
			set {providerConnection = value;
			}
		}

		private  Catalog _catalog;
		internal Catalog  Catalog
		{
			get { return _catalog;  }
			set {_catalog = value;  }
		}

		private bool _canBePooled;  // = false;
		internal bool CanBePooled
		{
			get { return _canBePooled;  }
			set {_canBePooled = value;  }
		}

		/*
		** Name: dbms_connect
		**
		** Description:
		**	Establish connection with DBMS.
		**
		** Input:
		**	info	Connection property list.
		**	xid	Distributed transaction ID.
		**	timeout	Connection timeout.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	28-Mar-01 (gordy)
		**	    Separated from constructor.
		**	11-Apr-01 (gordy)
		**	    Added timeout parameter to replace DriverManager reference.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.
		**	10-May-01 (gordy)
		**	    Check DBMS capabilities relevant to driver configuration.
		**	20-Aug-01 (gordy)
		**	    Added connection parameter for default cursor mode.
		**	20-Feb-02 (gordy)
		**	    Added dbmsInfo checks for DBMS type and protocol level.
		**	 1-Mar-07 (gordy, ported by thoda04)
		**	    Check connection character encoding and property override.
		*/

		private void  dbms_connect(IConfig config, int timeout, AdvanXID xa_xid)
		{
			/*
			** Check if character encoding has been established
			** for the connection.  If not, it may be explicitly
			** set via driver properties.  Encoding must be set
			** before connection parameters are processed.
			*/
			String encodingName = config.get(DrvConst.DRV_PROP_ENCODE);

			if (encodingName != null && encodingName.Length != 0)
			{
				// encodingName was checked to be valid in ParseConnectionString
				System.Text.Encoding encoding =
					System.Text.Encoding.GetEncoding(encodingName);
				try
				{
					msg.setCharSet(
						new CharSet(encoding, true, "CharacterEncoding", encodingName));
					if (trace.enabled(1))
						trace.write(tr_id + ": encoding: " +
							encodingName + " (" + encoding.ToString() +
							",WindowsCodePage=" + encoding.WindowsCodePage.ToString()
							+ ")");
				}
				catch (Exception)
				{
					if (trace.enabled(1))
						trace.write(tr_id + ": unknown encoding: " + encodingName);
					throw SqlEx.get(ERR_GC4009_BAD_CHARSET);
				}
			}
			else if (msg.getCharSet() == null)
			{
				if (trace.enabled())
					trace.log(title + ": failed to establish character encoding!");
				throw SqlEx.get(ERR_GC4009_BAD_CHARSET);
			}

			if (trace.enabled(2))
				trace.write(tr_id + ": Connecting to database");
			
			try
			{
				/*
				** Send connection request and parameters.
				*/
				msg.begin(MSG_CONNECT);
				client_parms ( config );
				driver_parms( checkLocalUser( config ), timeout, xa_xid );
				msg.done( true );
				readResults();
			}
			catch ( SqlEx ex )
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": error connecting to database:" +
						ex.Message);
				conn.close();
				throw;
			}
		}  // dbms_connect


		/*
		** Name: checkLocalUser
		**
		** Description:
		**	Check to see if local user ID should be used to establish DBMS
		**	connection.  Any application provided user ID is used instead.
		**	The connection must be to a local server and not DTMC.
		**
		** Input:
		**	config	Configuration properties.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	TRUE if local user ID should be used, FALSE otherwise.
		**
		** History:
		**	31-Oct-02 (wansh01)
		**	    Created. 
		**	23-Dec-02 (gordy)
		**	    Check protocol level.
		**	 7-Jul-03 (gordy)
		**	    Changed properties parameter to support config heirarchy.
		**	    Connection parameters now set in driver_parms.  Don't need
		**	    to see if local user ID is accessible, it will be handled
		**	    when trying to send the user ID.
		*/

		private bool
			checkLocalUser( IConfig config )
		{
			String	userID;

			return( msg.isLocal()  &&  ! conn.is_dtmc  &&  
				conn.msg_protocol_level >= MSG_PROTO_3  &&
				( (userID = config.get( DrvConst.DRV_PROP_USR )) == null  ||  
					userID.Length == 0 ) );
		} // checkLocalUser




		/*
		** Name: client_parms
		**
		** Description:
		**	Process driver connection properties and send DAM-ML 
		**	connection parameters.
		**
		** Input:
		**	config	Configuration properties.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	17-Sep-99 (rajus01)
		**	    Added connection parameters.
		**	22-Sep-99 (gordy)
		**	    Convert user/password to server character set when encrypting.
		**	17-Jan-00 (gordy)
		**	    Connection pool parameter sent as boolean value.  No longer
		**	    need to turn on autocommit since it is the default state for
		**	    the server.
		**	19-May-00 (gordy)
		**	    Check for select loops connection parameter (internal) and 
		**	    set the DB connection state appropriatly.
		**	30-Aug-00 (gordy)
		**	    BUG #102472 - Fix the exception returned if user ID or
		**	    password are zero-length strings.  A zero-length string
		**	    was not being caught by the null test and was causing an
		**	    array exception during the password encoding.  This was
		**	    caught by intermediate code and turned into a 'character
		**	    encoding' exception.  Now, null and zero-length valued
		**	    parameters are skipped, which for user ID and password
		**	    result in a reasonably specific error from the server.
		**	17-Oct-00 (gordy)
		**	    Added autocommit mode connection parameter.
		**	20-Jan-01 (gordy)
		**	    Autocommit mode only supported at protocol level 2 or higher.
		**	28-Mar-01 (gordy)
		**	    Separated from constructor.
		**	20-Aug-01 (gordy)
		**	    Added connection parameter for default cursor mode.
		**	01-Oct-02 (wansh01)
		**	    Added support for MSG_CP_LOGIN_TYPE as a connection parameter. 	
		**	31-Oct-02 (gordy)
		**	    Extracted from connect().
		**	23-Dec-02 (gordy)
		**	    Check protocol level for login type.  Added (no-op) CHAR_ENCODE.
		**	18-Feb-03 (gordy)
		**	    Property login type changed to vnode usage and changed values:
		**	    LOCAL -> LOGIN, REMOTE -> CONNECT.  Message parameter unchanged.
		**	 7-Jul-03 (gordy)
		**	    Add Ingres configuration parameters.  Changed properties 
		**	    parameter to support config heirarchy.
		**	 6-jul-04 (gordy; ported by thoda04)
		**	    Added session_mask to password encryption.
		**	27-feb-07 (thoda04)
		**	    Add char_encoding support.
		**	24-jul-07 (thoda04)
		**	    Write the role password along with the roleID.
		*/

		private void
			client_parms( IConfig config )
		{
			byte i1;

			if (trace.enabled(2))
				trace.write(tr_id + ": sending connection parameters" );

			/*
			** Send connection request and parameters.
			*/
			if (trace.enabled(3))
				trace.write(tr_id + ": sending connection parameters");
				
			for (int i = 0; i < DrvConst.propInfo.Length; i++)
			{
				string propinfo = DrvConst.propInfo[i].name;
				string stringValue = config.get(propinfo);
				if (stringValue == null || stringValue.Length == 0)
					continue;

//				System.Windows.Forms.MessageBox.Show(
//					propinfo + "=" + stringValue, "Parms Debug",
//					System.Windows.Forms.MessageBoxButtons.OK,
//					System.Windows.Forms.MessageBoxIcon.Information,
//					System.Windows.Forms.MessageBoxDefaultButton.Button1,
//					System.Windows.Forms.MessageBoxOptions.ServiceNotification);

				switch (DrvConst.propInfo[i].msgID)
				{
					case MSG_CP_DATABASE: 
						database = stringValue;
						msg.write(DrvConst.propInfo[i].msgID);
						msg.write(stringValue);
						break;
							
						
						
					case MSG_CP_PASSWORD: 
						/*
						** Encrypt password using user ID (Don't send if user ID
						** not provided.
						*/
						String user = config.get(DrvConst.DRV_PROP_USR);
						if (user != null && user.Length > 0)
						{
							msg.write(DrvConst.propInfo[i].msgID);
							msg.write( msg.encode( user, conn.session_mask, stringValue ) );
						}
						stringValue = "*****";	// Hide password from tracing.
						break;
							
						
						
					case MSG_CP_DBPASSWORD: 
						msg.write(DrvConst.propInfo[i].msgID);
						msg.write(stringValue);
						stringValue = "*****";	// Hide password from tracing.
						break;


					case MSG_CP_ROLE_ID:
						String roleValue = stringValue;
						String rolePwd   = config.get(DrvConst.DRV_PROP_ROLEPWD);
						if (rolePwd != null && rolePwd.Length > 0)
						{
							roleValue   += "/" + rolePwd; // write out "myroleid/myrolepwd"
							stringValue += "/*****";	  // Hide password from tracing.
						}
						msg.write(DrvConst.propInfo[i].msgID);
						msg.write(roleValue);
						break;


					case MSG_CP_CONNECT_POOL: 
						if (ToInvariantLower(stringValue).Equals("off"))
						{
							msg.write(DrvConst.propInfo[i].msgID);
							msg.write((short) 1);
							msg.write(MSG_CPV_POOL_OFF);
						}
						else if (ToInvariantLower(stringValue).Equals("on"))
						{
							msg.write(DrvConst.propInfo[i].msgID);
							msg.write((short) 1);
							msg.write(MSG_CPV_POOL_ON);
						}
						break;
							
						
						
					case MSG_CP_AUTO_MODE:
						if      (ToInvariantLower(stringValue).Equals("dbms"))
							i1 = MSG_CPV_XACM_DBMS;
						else if (ToInvariantLower(stringValue).Equals("single"))
							i1 = MSG_CPV_XACM_SINGLE;
						else if (ToInvariantLower(stringValue).Equals("multi"))
							i1 = MSG_CPV_XACM_MULTI;
						else
						{
							if ( trace.enabled( 1 ) ) 
								trace.write( tr_id +
									": autocommit mode '" +
									stringValue + "'" );
							throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
						}

						if (conn.msg_protocol_level >= MSG_PROTO_2)
						{
							msg.write(DrvConst.propInfo[i].msgID);
							msg.write((short) 1);
							msg.write(i1);
						}
						else  if ( i1 != MSG_CPV_XACM_DBMS )
						{
							if ( trace.enabled( 1 ) ) 
								trace.write( tr_id +
									": autocommit mode '" + stringValue +
									"' @ proto lvl " + conn.msg_protocol_level );
							throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
						}
						break;
						
						
					case MSG_CP_LOGIN_TYPE :
						/*
							** Convert to symbolic value.  Only default setting
							** permitted at lower protocol levels and param is
							** not sent.
							*/
						if ( ToInvariantLower(stringValue).Equals( "login" ) )
							i1 = MSG_CPV_LOGIN_LOCAL;
						else  if ( ToInvariantLower(stringValue).Equals( "connect" ) )
							i1 = MSG_CPV_LOGIN_REMOTE;
						else
						{
							if ( trace.enabled( 1 ) ) 
								trace.write( tr_id + ": vnode usage '" +
									stringValue + "'" );
							throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
						}

						if ( conn.msg_protocol_level >= MSG_PROTO_3 )
						{
							msg.write( DrvConst.propInfo[ i ].msgID );
							msg.write( (short)1 );
							msg.write( i1 );
						}
						else  if ( i1 != MSG_CPV_LOGIN_REMOTE )
						{
							if ( trace.enabled( 1 ) ) 
								trace.write( tr_id + ": vnode usage '" +
									stringValue + 
									"' @ proto lvl " + conn.msg_protocol_level );
							throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
						}
						break;

					case MSG_CP_TIMEZONE :
						/*
						** Ignored at lower protocol levels to permit setting
						** in property file for all connections.
						*/
						if (conn.msg_protocol_level >= MSG_PROTO_3)
						{
							/*
							** Note that Ingres timezone has been sent to DBMS.
							*/
							conn.is_gmt = false;
							msg.write(DrvConst.propInfo[i].msgID);
							msg.write(stringValue);
						}
						break;

					case MSG_CP_DECIMAL :
					case MSG_CP_DATE_FRMT :
					case MSG_CP_MNY_FRMT :
						/*
							** Ignored at lower protocol levels to permit setting
							** in property file for all connections.
							*/
						if ( conn.msg_protocol_level >= MSG_PROTO_3 )
						{
							msg.write( DrvConst.propInfo[ i ].msgID );
							msg.write( stringValue );
						}
						break;

					case MSG_CP_MNY_PREC :
						/*
							** Ignored at lower protocol levels to permit setting
							** in property file for all connections.  Translate to
							** numeric value.
							*/
						if ( conn.msg_protocol_level >= MSG_PROTO_3 )
						{
							msg.write( DrvConst.propInfo[ i ].msgID );
							msg.write( (short)1 );
		
							try { msg.write( Byte.Parse( stringValue ) ); }
							catch( Exception ex )
							{ 
								if ( trace.enabled( 1 ) )
									trace.write( tr_id + ": money prec '" + stringValue + "'" );
								throw SqlEx.get( ERR_GC4010_PARAM_VALUE, ex ); 
							}
						}
						break;

					case MSG_CP_SELECT_LOOP: 
						/*
							** This is a local connection property
							** and is not forwarded to the server.
							*/
						if (ToInvariantLower(stringValue).Equals("off"))
							conn.select_loops = false;
						else if (ToInvariantLower(stringValue).Equals("on"))
							conn.select_loops = true;
						break;
							
						
						
					case MSG_CP_CURSOR_MODE: 
						/*
							** This is a local connection property
							** and is not forwarded to the server.
							*/
						if (ToInvariantLower(stringValue).Equals("dbms"))
							conn.cursor_mode = DrvConst.DRV_CRSR_DBMS;
						else if (ToInvariantLower(stringValue).Equals("update"))
							conn.cursor_mode = DrvConst.DRV_CRSR_UPDATE;
						else if (ToInvariantLower(stringValue).Equals("readonly"))
							conn.cursor_mode = DrvConst.DRV_CRSR_READONLY;
						break;
							
						
						
					case MSG_CP_BLANKDATE:   // BLANKDATE=NULL for empty date?
						/*
							** This is a local connection property
							** and is not forwarded to the server.
							*/
						if (ToInvariantLower(stringValue).Equals("null"))
							conn.isBlankDateNull = true;
						else
							conn.isBlankDateNull = false;
						break;

					case MSG_CP_CHAR_ENCODE:
						/*
						** This is a local connection property
						** and is not forwarded to the server.
						**
						** Character encoding is processed during
						** initial server connection establishment.
						*/
						break;

					case MSG_CP_SENDINGRESDATES:
						/*
							** This is a local connection property
							** and is not forwarded to the server.
							*/
						if (ToInvariantLower(stringValue).Equals("true"))
							conn.sendIngresDates = true;
						else 
							conn.sendIngresDates = false;
						break;


					default: 
						msg.write(DrvConst.propInfo[i].msgID);
						msg.write(stringValue);
						break;
						
				}  // end switch
					
				if (trace.enabled(3))
					trace.write(tr_id + ":     " +
						IdMap.get(DrvConst.propInfo[i].msgID, cpMap) +
						" '" + stringValue + "'");
			}  // end for loop through propInfo

			return;
		} // client_parms


		/*
		** Name: driver_parms
		**
		** Description:
		**	Send internal DAM-ML connection parameters.
		**
		** Input:
		**	local	Use local user ID.
		**	timeout	Connection timeout.
		**	xa_xid	XA transaction ID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 4-Nov-99 (gordy)
		**	    Send timeout connection parameter to server.
		**	11-Apr-01 (gordy)
		**	    Added timeout parameter to replace DriverManager reference.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.
		**	06-Dec-02 (wansh01)
		**	    Added support for optional userid for local connection.
		**	    set connection parameter MSG_CP_LOGIN_TYPE to MSG_CPV_LOGIN_USER
		**	    (an internal value) if no userid specified for local connection. 
		**	26-Dec-02 (gordy)
		**	    Send initial autocommit state to server.
		**	15-Jan-03 (gordy)
		**	    Extracted from connect().  Added local user, host name & address.
		**	 7-Jul-03 (gordy)
		**	    Write all parameters associated with local user connections.
		**	 6-jul-04 (gordy; ported by thoda04)
		**	    Added session_mask to password encryption.
		*/

		private void
			driver_parms( bool local, int timeout, AdvanXID xa_xid )
		{
			String value, userID = null;

			/*
			** User ID needed for various parameters.
			*/
			try { userID = Environment.UserName; }
			catch( Exception /*ignore*/ ) {}
			if ( userID != null  &&  userID.Length == 0 )  userID = null;
	
			/*
			** If connection should be made for local user, send the 
			** appropriate connection parameters. (protocol level 
			** checked by caller).
			*/
			if ( local  &&  userID != null )
			{
				/*
				** A special login type is used to indicate that
				** the local user ID of the client should be used
				** (without requiring a password) to establish
				** the connection.
				*/
				if ( trace.enabled( 3 ) )  trace.write( "    Login Type: 'user'" ); 
	
				msg.write( MSG_CP_LOGIN_TYPE );
				msg.write( (short)1 );
				msg.write( MSG_CPV_LOGIN_USER);
	
				/*
				** If a user ID had been provided, then we would not be
				** doing a local user connection.  Also, a password is 
				** only sent if a user ID is provided.  These parameters
				** are hijacked for this case to send the local user ID
				** with some verification of its authenticity.
				*/
				msg.write( MSG_CP_USERNAME );
				msg.write( userID );
				msg.write( MSG_CP_PASSWORD );
				msg.write( msg.encode( userID, conn.session_mask, userID ) );
			}

			/*
			** timeout of 0 means no timeout (full blocking).
			** Only send timeout if non-blocking.
			*/
			if ( timeout != 0 )
			{
				if ( trace.enabled( 3 ) )  
					trace.write( "    Timeout: " + timeout );
				/*
				** Ingres driver interprets a negative timeout
				** as milli-seconds.  Otherwise, convert seconds to
				** milli-seconds.
				*/
				timeout = (timeout < 0) ? -timeout : timeout * 1000;
				msg.write( MSG_CP_TIMEOUT );
				msg.write( (short)4 );
				msg.write( timeout );
			}

			if ( xa_xid != null  &&  xa_xid.Type == AdvanXID.XID_XA  &&
				conn.msg_protocol_level >= MSG_PROTO_2 )
			{
				if ( trace.enabled( 3 ) )  
					trace.write( "    XA Transaction ID: " + xa_xid );

				msg.write( MSG_CP_XA_FRMT );
				msg.write( (short)4 );
				msg.write( xa_xid.FormatId );
				msg.write( MSG_CP_XA_GTID );
				msg.write( xa_xid.getGlobalTransactionId() );
				msg.write( MSG_CP_XA_BQUAL );
				msg.write( xa_xid.getBranchQualifier() );

				/*
				** Autocommit is disabled when connecting to
				** an existing distributed transaction.
				*/
				autoCommit = false;
			}

			if ( conn.msg_protocol_level >= MSG_PROTO_3 )
			{
				/*
				** Force initial autocommit state rather than relying
				** on the server default.
				*/
				if ( trace.enabled( 3 ) )  
					trace.write( "    Autocommit State: " + 
						(autoCommit ? "on" : "off") );

				msg.write( MSG_CP_AUTO_STATE );
				msg.write( (short)1 );
				msg.write( (byte)(autoCommit ? MSG_CPV_POOL_ON 
					: MSG_CPV_POOL_OFF) );

				/*
				** Provide local user ID
				*/
				if ( userID != null )
				{
					if ( trace.enabled( 3 ) )  
						trace.write( "    Local User ID: " + userID ); 

					msg.write( MSG_CP_CLIENT_USER );
					msg.write( userID );
				}

				/*
				** Provide local host name
				*/
				value = msg.LocalHost;
				if ( value != null  &&  value.Length > 0 )
				{
					if ( trace.enabled( 3 ) )  
						trace.write( "    Local Host: " + value ); 

					msg.write( MSG_CP_CLIENT_HOST );
					msg.write( value );
				}

				/*
				** Provide local host address
				*/
				value = msg.LocalAddress;
				if ( value != null  &&  value.Length > 0 )
				{
					if ( trace.enabled( 3 ) )  
						trace.write( "    Local Address: " + value ); 

					msg.write( MSG_CP_CLIENT_ADDR );
					msg.write( value );
				}
			}  // end if MSG_PROTO_3

			return;
		} // driver_parms


		/*
		** Name: finalize
		**
		** Description:
		**	Make sure the database connection is releases.
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
		**	28-Aug-03 (thoda04)
		**	    Added connection pool slot release.
		*/
		
		~AdvanConnect()
		{
			// Release slot in connection pool.  If AdvanConnect had
			// been closed properly, this AdvanConnect would already
			// have released its slot in the pool and
			// this.connectPool would == null.
			// If not closed properly, at least try to release the
			// slot in the connection pool so that others who are
			// waiting on the Max Pool Size of the pool have a
			// chance to break out of wait.
			// If the AppDomain is shutting, forget everything.
			AppDomain appDomain = AppDomain.CurrentDomain;
			if (!appDomain.IsFinalizingForUnload()  &&  connectionPool != null)
			{
				try
				{
					connectionPool.PutPoolSlot(null);  // release the pool slot
				}
				finally
				{
					connectionPool = null;  // make sure this pool reference is gone
				}
			}

			// don't call close since needed objects might
			// already have been finalized
			// close();
		} // finalize
		
		
		/*
		** Name: isClosed
		**
		** Description:
		**	Determine if the connection is closed.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if connection closed, false otherwise.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.*/
		
		internal bool isClosed()
		{
			if (trace.enabled())
				trace.log(title + ".isClosed(): " + msg.isClosed());
			return (msg.isClosed());
		} // isClosed
		
		/*
		** Name: close
		**
		** Description:
		**	Close the connection.
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
		**	 5-May-99 (gordy)
		**	    Created.
		**	 8-Sep-99 (gordy)
		**	    Synchronize on DbConn.
		**	29-Sep-99 (gordy)
		**	    Used DbConn lock()/unlock() for synchronization.
		**	17-Jan-00 (gordy)
		**	    Server will now disable autocommit during disconnect.
		**	13-Oct-00 (gordy)
		**	    Return errors from close.*/
		
		internal void  close()
		{
			if (trace.enabled())
				trace.log(title + ".close()");

			warnings = null;
			CanBePooled = false;  // make sure it does not go back in pool
			if (connectionPool != null)
			{
				try
				{
					connectionPool.PutPoolSlot(null);  // release the pool slot
				}
				finally
				{
					connectionPool = null;  // make sure this pool reference is gone
				}
			}

			if (msg.isClosed())
				return ;
			// Nothing to do
			
			msg.LockConnection();
			try
			{
				/*
				** Send disconnect request and read results.
				*/
				if (msg.isSocketConnected() == true)
				{
					msg.begin(MSG_DISCONN);
					msg.done(true);
					readResults();
				}
			}
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".close(): error closing connection");
				if (trace.enabled(1))
					ex.trace(trace);
				throw;
			}
			finally
			{
				msg.UnlockConnection();
				conn.close();

				// release the IngresConnection
				// reference for garbase collection
				providerConnection = null;
			}
			conn.autoCommit = false;
			return ;
		} // close


		/*
		** Name: CloseOrPutBackIntoConnectionPool
		**
		** Description:
		**	Put the internal connection as represented by AdvanConnect back
		**	into connection pool or, if (CanBePooled == false), close it.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	01-Feb-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Put the internal connection as represented by AdvanConnect back
		/// into connection pool or, if (CanBePooled == false), close it.
		/// </summary>
		internal bool CloseOrPutBackIntoConnectionPool()
		{
			// if advanConnect object still in use by some other thread, return
			if (this.ReferenceCountDecrement() > 0)
				return false;

			AdvanConnectionPoolManager.Put(this);  // put advanConnect back in pool
			return true;
		}


		/*
		** Name: abort
		**
		** Description:
		**	Abort connection irregardless of current state.
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
		**	 2-Feb-05 (gordy)
		**	    Created.
		*/


		internal void
		abort()
		{
			if (trace.enabled(2)) trace.write(tr_id + ".abort()");
			msg.abort();
			return;
		} // abort()


		/*
		** Name: createStatement
		**
		** Description:
		**	Creates a Statement object associated with the
		**	connection.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Statement   A new Statement object.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		**	30-Oct-00 (gordy)
		**	    Need to pass Connection object to statement.  The
		**	    associated DbConn can be obtained via getDbConn().
		**	    Pass default ResultSet type and concurrency.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.
		**	20-Aug-01 (gordy)
		**	    Use default concurrency.*/
		
		internal AdvanStmt createStatement()
		{
			if (trace.enabled())
				trace.log(title + ".createStatement()");
			warnings = null;

			if (conn.is_dtmc)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": not permitted when DTMC");
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}
			
			AdvanStmt stmt = new AdvanStmt(
				this.conn,
				DrvConst.TYPE_FORWARD_ONLY,
				DrvConst.DRV_CRSR_DBMS,
				DrvConst.CLOSE_CURSORS_AT_COMMIT);

			if (trace.enabled())
				trace.log(title + ".createStatement(): " + stmt);
			return (stmt);
		} // createStatement
		
		
		/*
		** Name: prepareStatement
		**
		** Description:
		**	Creates a PreparedStatement object associated with the
		**	connection.
		**
		** Input:
		**	sql	Query text.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	PreparedStatement   A new PreparedStatement object.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		**	30-Oct-00 (gordy)
		**	    Need to pass Connection object to statement.  The
		**	    associated DbConn can be obtained via getDbConn().
		**	    Pass default ResultSet type and concurrency.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.
		**	20-Aug-01 (gordy)
		**	    Use default concurrency.*/
		
		internal AdvanPrep prepareStatement(String sql)
		{
			if (trace.enabled())
				trace.log(title + ".prepareStatement( " + sql + " )");
			warnings = null;

			if (conn.is_dtmc)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": not permitted when DTMC");
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}
			
			AdvanPrep prep = new AdvanPrep(
				this.conn, sql, DrvConst.TYPE_FORWARD_ONLY,
				DrvConst.DRV_CRSR_DBMS, DrvConst.CLOSE_CURSORS_AT_COMMIT);
			if (trace.enabled())
				trace.log(title + ".prepareStatement(): " + prep);
			return (prep);
		} // prepareStatement
		
		
		/*
		** Name: prepareCall
		**
		** Description:
		**	Creates a CallableStatement object associated with the
		**	connection.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	CallableStatement   A new CallableStatement object.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		**	13-Jun-00 (gordy)
		**	    Unstubbed.
		**	30-Oct-00 (gordy)
		**	    Need to pass Connection object to statement.  The
		**	    associated DbConn can be obtained via getDbConn().
		**	20-Jan-01 (gordy)
		**	    Callable statements not supported until protocol level 2.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.*/
		
		internal AdvanCall prepareCall(String sql)
		{
			if (trace.enabled())
				trace.log(title + ".prepareCall( " + sql + " )");
			warnings = null;

			if (conn.is_dtmc)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": not permitted when DTMC");
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}
			
			if (conn.msg_protocol_level < MSG_PROTO_2)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": protocol = " + conn.msg_protocol_level);
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}
			
			AdvanCall call = new AdvanCall(this.conn, sql);
			
			if (trace.enabled())
				trace.log(title + ".prepareCall()" + call);
			return (call);
		} // prepareCall
		
		
		/*
		** Name: getAutoCommit
		**
		** Description:
		**	Determine the autocommit state.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if autocommit is ON, false if OFF.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.*/
		
		/// <summary>
		/// Get autocommit mode.
		/// </summary>
		/// <returns>true if ON, false if OFF</returns>
		internal bool getAutoCommit()
		{
			if (trace.enabled())
				trace.log(title + ".getAutoCommit(): " + autoCommit);
			return (autoCommit);
		}
		// getAutoCommit
		
		/*
		** Name: setAutoCommit
		**
		** Description:
		**	Set the autocommit state.
		**
		** Input:
		**	autocommit  True to set autocommit ON, false for OFF.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		**	 8-Sep-99 (gordy)
		**	    Synchronize on DbConn.
		**	29-Sep-99 (gordy)
		**	    Used DbConn lock()/unlock() for synchronization.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.*/
		
		/// <summary>
		/// Set autocommit ON if true, OFF if false.
		/// </summary>
		/// <param name="autoCommit">ON if true, OFF if false</param>
		internal void  setAutoCommit(bool autoCommit)
		{
			if (trace.enabled())
				trace.log(title + ".setAutoCommit( " + autoCommit + " )");
			warnings = null;
			if (autoCommit == this.autoCommit)
				return ;
			// Nothing to do

			if (conn.is_dtmc)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": not permitted when DTMC");
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}
			
			msg.LockConnection();
			try
			{
				msg.begin(MSG_XACT);
				msg.write((short) (autoCommit?MSG_XACT_AC_ENABLE:
				                              MSG_XACT_AC_DISABLE));
				msg.done(true);
				readResults();
				this.autoCommit = autoCommit;
			}
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".setAutoCommit(): error changing autocommit");
				if (trace.enabled(1))
					ex.trace(trace);
				throw ex;
			}
			finally
			{
				msg.UnlockConnection();
			}
			
			return ;
		} // setAutoCommit
		
		/*
		** Name: commit
		**
		** Description:
		**	Commit the current transaction
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
		**	 5-May-99 (gordy)
		**	    Created.
		**	 8-Sep-99 (gordy)
		**	    Synchronize on DbConn.
		**	29-Sep-99 (gordy)
		**	    Used DbConn lock()/unlock() for synchronization.
		*/
		
		internal void  commit()
		{
			if (trace.enabled())
				trace.log(title + ".commit()");
			warnings = null;
			if (autoCommit)
				return ;
			// Nothing to do
			
			msg.LockConnection();
			try
			{
				msg.begin(MSG_XACT);
				msg.write(MSG_XACT_COMMIT);
				msg.done(true);
				readResults();
			}
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title +
						".commit(): error committing transaction");
				if (trace.enabled(1))
					ex.trace(trace);
				throw ex;
			}
			finally
			{
				msg.UnlockConnection();
			}
			
			return ;
		} // commit
		
		/*
		** Name: rollback
		**
		** Description:
		**	Rollback the current transaction
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
		**	 5-May-99 (gordy)
		**	    Created.
		**	 8-Sep-99 (gordy)
		**	    Synchronize on DbConn.
		**	29-Sep-99 (gordy)
		**	    Used DbConn lock()/unlock() for synchronization.
		*/
		
		internal void  rollback()
		{
			if (trace.enabled())
				trace.log(title + ".rollback()");
			warnings = null;
			if (autoCommit)
				return ;
			// Nothing to do
			
			msg.LockConnection();
			try
			{
				msg.begin(MSG_XACT);
				msg.write(MSG_XACT_ROLLBACK);
				msg.done(true);
				readResults();
			}
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".rollback(): error rolling back transaction");
				if (trace.enabled(1))
					ex.trace(trace);
				throw ex;
			}
			finally
			{
				msg.UnlockConnection();
			}
			
			return ;
		} // rollback
		
		/*
		** Name: isReadOnly
		**
		** Description:
		**	Determine the readonly state of a connection.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if connection is readonly, false otherwise.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		*/
		
		internal bool isReadOnly()
		{
			if (trace.enabled())
				trace.log(title + ".isReadOnly(): " + readOnly);
			return (readOnly);
		} // isReadOnly
		
		/*
		** Name: setReadOnly
		**
		** Description:
		**	Set the readonly state of a connection.
		**
		** Input:
		**	readOnly    True for readonly, false otherwise.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		**	27-Sep-00 (gordy)
		**	    Don't perform any action if requested state
		**	    is the same as the current state.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.
		**	29-Jul-01 (loera01) Bug 105356
		**	   Rather than just sending a SET SESSION [READ|WRITE] [aaa] query, 
		**	   send the fully qualified command SET SESSION READ [ONLY|WRITE], 
		**	   TRANSACTION ISOLATION [aaa] via the new sendTransactionQuery 
		**	   method.  Only do so if the read-only setting is different than 
		**	   current.
		*/
		
		internal void  setReadOnly(bool readOnly)
		{
			lock(this)
			{
				if (trace.enabled())
					trace.log(title + ".setReadOnly( " + readOnly + " )");
				warnings = null;
				if (this.readOnly == readOnly)
					return ;

				if (conn.is_dtmc)
				{
					if (trace.enabled(1))
						trace.write(tr_id + ": not permitted when DTMC");
					throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
				}
				
				sendTransactionQuery(readOnly, this.isolationLevel);
				return ;
			}
		} // setReadOnly
		
		/*
		** Name: getTransactionIsolation
		**
		** Description:
		**	Determine the transaction isolation level.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Transaction isolation level.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		*/
		
		internal System.Data.IsolationLevel getTransactionIsolation()
		{
			if (trace.enabled())
				trace.log(title + ".getTransactionIsolation: " + isolationLevel);
			return (isolationLevel);
		} // getTransactionIsolation
		
		/*
		** Name: setTransactionIsolation
		**
		** Description:
		**	Set the transaction isolation level.  The Connection
		**	interface defines the following transaction isolation
		**	levels:
		**
		**	    TRANSACTION_NONE
		**	    TRANSACTION_READ_UNCOMMITTED
		**	    TRANSACTION_READ_COMMITTED
		**	    TRANSACTION_REPEATABLE_READ
		**	    TRANSACTION_SERIALIZABLE
		**
		** Input:
		**	level	Transaction isolation level.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.
		**	27-Jul-01 (loera01) Bug 105327
		**	   Reject attempts to set the transaction level for anything other
		**	   than serializable, if the SQL level of the dbms is 6.4.
		**	29-Jul-01 (loera01) Bug 105356
		**	   Rather than just sending a SET SESSION TRANSACTION ISOLATION
		**	   [aaa] query, send the fully qualified command SET SESSION READ 
		**	   [ONLY|WRITE], TRANSACTION ISOLATION [aaa] via the new 
		**	   sendTransactionQuery method.  Only do so if the read-only setting 
		**	   is different than current.*/
		
		internal void  setTransactionIsolation(
			System.Data.IsolationLevel level)
		{
			lock(this)
			{
				if (trace.enabled())
					trace.log(title + ".setTransactionIsolation( " + level + " )");
				warnings = null;
				if (level == this.isolationLevel)
					return ;

				if (conn.is_dtmc)
				{
					if (trace.enabled(1))
						trace.write(tr_id + ": not permitted when DTMC");
					throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
				}
				
				/* 
				** If the SQL level is 6.4, only allow TRANSACTION_SERIALIZABLE.
				*/
				if (level != IsolationLevel.Serializable &&
				 conn.sqlLevel < DBMS_SQL_LEVEL_OI10)
				{
					/*
					** JDBC spec permits more restrictive transaction 
					** isolation levels when the input transaction level 
					** is not supported by the target DBMS server.
					*/
					if (trace.enabled(3))
						trace.write(tr_id + ": Requested isolation level not supported!");
					setWarning(SqlEx.get(ERR_GC4019_UNSUPPORTED));
					return;
				}
				
				sendTransactionQuery(this.readOnly, level);
				return ;
			}
		} // setTransactionIsolation
		
		
		/*
		** Name: sendTransactionQuery
		**
		** Description:
		**	Build and send "set session" query that includes both isolation 
		**	level and "read only" or "read/write" components.  Query will be in 
		**	this form:
		**	     SET SESSION [READ ONLY|WRITE] [,ISOLATION LEVEL isolation_level] 
		**
		** Input:
		**      readOnly        True if read-only, false if read/write
		**      isolationLevel  One of four values:
		**                      TRANSACTION_SERIALIZABLE 
		**                      TRANSACTION_READ_UNCOMMITTED 
		**                      TRANSACTION_READ_COMMITTED 
		**                      TRANSACTION_REPEATABLE_READ
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	27-Jul-01 (loera01) 
		**	    Created.
		**	30-sep-02 (loera01) Bug 108833
		**	    Since the intention of the setTransactionIsolaton and
		**	    setReadOnly methods is for the "set" query to persist in the
		**	    session, change "set transaction" to "set session".
		*/
		private void  sendTransactionQuery(
				bool readOnly, System.Data.IsolationLevel isolationLevel)
		{
			lock(this)
			{
				if (trace.enabled())
					trace.log(title + ".sendTransactionQuery( " + readOnly + " , " + isolationLevel + " )");
				
				String sql = "set session read ";
				sql += readOnly?"only":" write";
				sql += ", isolation level ";
				
				try
				{
					switch (isolationLevel)
					{
						case System.Data.IsolationLevel.ReadUncommitted: 
							sql += "read uncommitted"; break;
						
						case System.Data.IsolationLevel.ReadCommitted: 
							sql += "read committed"; break;
						
						case System.Data.IsolationLevel.RepeatableRead: 
							sql += "repeatable read"; break;
						
						case System.Data.IsolationLevel.Serializable: 
							sql += "serializable"; break;
							
						
						
						default: 
							throw SqlEx.get( ERR_GC4010_PARAM_VALUE );
						
					}

					// assume that the command will work OK and
					// set isolation level now to avoid looping recursive calls of
					// Execute->setIsolationLevel->sendTransactionQuery->Execute->...
					this.readOnly = readOnly;
					this.isolationLevel = isolationLevel;

					IDbCommand cmd = providerConnection.CreateCommand();
					cmd.CommandText = sql;
					cmd.ExecuteNonQuery();
			//TODO move errors to connection object
			//		warnings = stmt.warnings;
					cmd.Dispose();
				}
				catch (SqlEx ex)
				{
					if (trace.enabled())
						trace.log(title + ".sendTransactionQuery(): failed!");
					if (trace.enabled(1))
						ex.trace(trace);
					throw ex;
				}
				return ;
			}
		} // sendTransactionQuery
		
		
		/*
		** Name: TypeMap property
		**
		** Description:
		**	get/set the type map.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Map	Type map.
		**
		** History:
		**	30-Oct-00 (gordy)
		**	    Created.*/
		
		internal virtual Hashtable TypeMap
		{
			get
			{
				if (trace.enabled())
					trace.log(title + ".getTypeMap()");
				return (type_map);
			}
			
			set
			{
				if (trace.enabled())
					trace.log(title + ".setTypeMap()");
				type_map = value;
				return ;
			}
			
		} // getTypeMap


		/*
		** Name: startTransaction
		**
		** Description:
		**	Start a distributed transaction.  No transaction, autocommit
		**	or normal, may be active.
		**
		** Input:
		**	xid	XA XID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	20-Jul-06 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Start work on behalf of a Distributed Transaction branch.
		/// </summary>
		/// <param name="xid"></param>
		internal void
		startTransaction(AdvanXID xid)
		{
			startTransaction(xid, 0);
			return;
		} // startTransaction


		/*
		** Name: startTransaction
		**
		** Description:
		**	Start a distributed transaction.  No transaction, autocommit
		**	or normal, may be active.
		**
		** Input:
		**	xid	XID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	21-Mar-01 (gordy)
		**	    Created.
		**	28-Mar-01 (gordy)
		**	    Check protocol level for server support.
		**	 2-Apr-01 (gordy)
		**	    Use AdvanXID, support Ingres transaction ID.
		**	11-Apr-01 (gordy)
		**	    Trace transaction ID.
		**	18-Apr-01 (gordy)
		**	    Added support for Distributed Transaction Management Connections.*/

		internal virtual void startTransaction(AdvanXID xid, int flags)
		{
			warnings = null;
			
			if (trace.enabled())
			{
				trace.log(title + ".startTransaction( " + xid + ", 0x" +
						 Convert.ToString(flags, 16) + " )");
			}

			if (conn.is_dtmc)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": not permitted when DTMC");
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}

			if (conn.msg_protocol_level < MSG_PROTO_2 ||
				(flags != 0 && conn.msg_protocol_level < MSG_PROTO_5))
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": protocol = " + conn.msg_protocol_level);
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}
			
			switch (xid.Type)
			{
				case AdvanXID.XID_INGRES:
				case AdvanXID.XID_XA: 
					// OK!
					break;
					
				
				
				default: 
					if (trace.enabled(1))
						trace.write(tr_id + ": unknown XID type: " + xid.Type);
					throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
				
			}
			
			msg.LockConnection();
			try
			{
				msg.begin(MSG_XACT);
				msg.write(MSG_XACT_BEGIN);
				
				switch (xid.Type)
				{
					case AdvanXID.XID_INGRES: 
						msg.write(MSG_XP_II_XID);
						msg.write((short) 8);
						msg.write((int) xid.XId);
						msg.write((int) (xid.XId >> 32));
						break;
						
					
					
					case AdvanXID.XID_XA: 
						msg.write(MSG_XP_XA_FRMT);
						msg.write((short) 4);
						msg.write(xid.FormatId);
						msg.write(MSG_XP_XA_GTID);
						msg.write(xid.getGlobalTransactionId());
						msg.write(MSG_XP_XA_BQUAL);
						msg.write(xid.getBranchQualifier());

						if (flags != 0)
						{
							msg.write(MSG_XP_XA_FLAGS);
							msg.write((short)4);
							msg.write(flags);
						}
						break;
					
				}
				
				msg.done(true);
				readResults();
			}
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".startTransaction(): error starting xact");
				if (trace.enabled(1))
					ex.trace(trace);
				throw ex;
			}
			finally
			{
				msg.UnlockConnection();
			}
			
			return ;
		} // startTransaction


		/*
		** Name: endTransaction
		**
		** Description:
		**	End XA transaction association.
		**
		** Input:
		**	xid	XA XID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	20-Jul-06 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// End work on behalf of a Distributed Transaction branch.
		/// </summary>
		/// <param name="xid"></param>
		internal void
		endTransaction(AdvanXID xid)
		{
			endTransaction(xid, 0);
			return;
		} // endTransaction


		/*
		** Name: endTransaction
		**
		** Description:
		**	End XA transaction association.
		**
		** Input:
		**	xid	XA XID.
		**	flags	XA Flags.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	20-Jul-06 (gordy, ported by thoda04 05-Mar-09)
		**	    Created.
		*/

		/// <summary>
		/// End work on behalf of a Distributed Transaction branch.
		/// </summary>
		/// <param name="xid"></param>
		/// <param name="flags"></param>
		internal void
		endTransaction(AdvanXID xid, int flags)
		{
			if ( trace.enabled( 2 ) )  
			trace.write( tr_id + ".endTransaction( " + xid + ", 0x" + 
					 Convert.ToString( flags, 16 ) + " )" );

			warnings = null;

			if ( conn.is_dtmc )
			{
			if ( trace.enabled( 1 ) )
				trace.write( tr_id + ".endTransaction: not permitted when DTMC" );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}

			if ( conn.msg_protocol_level < MSG_PROTO_5 )
			{
			if ( trace.enabled( 1 ) ) 
				trace.write( tr_id + ".endTransaction: protocol = " + 
					 conn.msg_protocol_level );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}

			msg.LockConnection();
			try
			{
			msg.begin( MSG_XACT );
			msg.write( MSG_XACT_END );
			msg.write( MSG_XP_XA_FRMT );
			msg.write( (short)4 );
			msg.write( xid.FormatId );
			msg.write( MSG_XP_XA_GTID );
			msg.write( xid.getGlobalTransactionId() );
			msg.write( MSG_XP_XA_BQUAL );
			msg.write( xid.getBranchQualifier() );

			if ( flags != 0 )
			{
				msg.write( MSG_XP_XA_FLAGS );
				msg.write( (short)4 );
				msg.write( flags );
			}

			msg.done( true );
			readResults();
			}
			catch( SqlEx sqlEx )
			{
			if ( trace.enabled( 1 ) )  
			{
				trace.write( tr_id + ".endTransaction: error ending xact" );
				sqlEx.trace( trace );
			}
			throw;
			}
			finally 
			{
				msg.UnlockConnection();
			}

			return;
		} // endTransaction


		/*
			** Name: prepareTransaction
			**
			** Description:
			**	Prepare a distributed transaction.  A call to startTransaction(),
			**	with no intervening commit() or rollback() call, must already
			**	have been made.
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
			**	21-Mar-01 (gordy)
			**	    Created.
			**	28-Mar-01 (gordy)
			**	    Check protocol for server support.
			**	18-Apr-01 (gordy)
			**	    Added support for Distributed Transaction Management Connections.*/
		
		internal virtual void  prepareTransaction()
		{
			if (trace.enabled())
				trace.log(title + ".prepareTransaction()");
			warnings = null;

			if (conn.is_dtmc)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": not permitted when DTMC");
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}
			
			if (conn.msg_protocol_level < MSG_PROTO_2)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": protocol = " + conn.msg_protocol_level);
				throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}
			
			msg.LockConnection();
			try
			{
				msg.begin(MSG_XACT);
				msg.write(MSG_XACT_PREPARE);
				msg.done(true);
				readResults();
			}
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".prepareTransaction(): error preparing xact");
				if (trace.enabled(1))
					ex.trace(trace);
				throw ex;
			}
			finally
			{
				msg.UnlockConnection();
			}
			
			return ;
		} // prepareTransaction


		/*
		** Name: prepareTransaction
		**
		** Description:
		**	Prepare and XA transaction.
		**
		** Input:
		**	xid	XA XID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	20-Jul-06 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Prepare to commit work done
		/// on behalf of a Distributed Transaction branch.
		/// </summary>
		/// <param name="xid"></param>
		internal void
		prepareTransaction(AdvanXID xid)
		{
			if ( trace.enabled( 2 ) )  
			trace.write( tr_id + ".prepareTransaction(" + xid + ")" );

			warnings = null;

			if ( conn.msg_protocol_level < MSG_PROTO_5 )
			{
			if ( trace.enabled( 1 ) ) 
				trace.write( tr_id + ".prepareTransaction: protocol = " + 
					 conn.msg_protocol_level );
			throw SqlEx.get( ERR_GC4019_UNSUPPORTED );
			}

			msg.LockConnection();
			try
			{
			msg.begin( MSG_XACT );
			msg.write( MSG_XACT_PREPARE );
			msg.write( MSG_XP_XA_FRMT );
			msg.write( (short)4 );
			msg.write( xid.FormatId );
			msg.write( MSG_XP_XA_GTID );
			msg.write( xid.getGlobalTransactionId() );
			msg.write( MSG_XP_XA_BQUAL );
			msg.write( xid.getBranchQualifier() );
			msg.done( true );
			readResults();
			}
			catch( SqlEx sqlEx )
			{
			if ( trace.enabled( 1 ) )  
			{
				trace.write( tr_id + ".prepareTransaction: error preparing xact" );
				sqlEx.trace( trace );
			}
			throw sqlEx;
			}
			finally 
			{
				msg.UnlockConnection();
			}

			return;
		} // prepareTransaction (xid)


		/*
		** Name: commit
		**
		** Description:
		**	Commit XA transaction.
		**
		** Input:
		**	xid	XA XID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	20-Jul-06 (gordy, ported by thoda04 05-Mar-09)
		**	    Created.
		*/

		/// <summary>
		/// Commit work done on behalf of a Distributed Transaction branch.
		/// </summary>
		/// <param name="xid"></param>
		internal void
		commit(AdvanXID xid)
		{
			commit(xid, 0);
			return;
		} // commit


		/*
		** Name: commit
		**
		** Description:
		**	Commit XA transaction.
		**
		** Input:
		**	xid	XA XID.
		**	flags	XA Flags.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	20-Jul-06 (gordy, ported by thoda04 05-Mar-09)
		**	    Created.
		*/

		/// <summary>
		/// Commit work done on behalf of a Distributed Transaction branch.
		/// </summary>
		/// <param name="xid"></param>
		/// <param name="flags"></param>
		internal void
		commit(AdvanXID xid, int flags)
		{
			if (trace.enabled(2))
				trace.write(tr_id + ".commit( " + xid + ", 0x" +
					 Convert.ToString(flags, 16) + " )");

			warnings = null;

			if (conn.msg_protocol_level < MSG_PROTO_5)
			{
				if (trace.enabled(1)) trace.write(tr_id +
				".commit: protocol = " + conn.msg_protocol_level);
				throw SqlEx.get(ERR_GC4019_UNSUPPORTED);
			}

			msg.LockConnection();
			try
			{
				msg.begin(MSG_XACT);
				msg.write(MSG_XACT_COMMIT);
				msg.write(MSG_XP_XA_FRMT);
				msg.write((short)4);
				msg.write(xid.FormatId);
				msg.write(MSG_XP_XA_GTID);
				msg.write(xid.getGlobalTransactionId());
				msg.write(MSG_XP_XA_BQUAL);
				msg.write(xid.getBranchQualifier());

				if (flags != 0)
				{
					msg.write(MSG_XP_XA_FLAGS);
					msg.write((short)4);
					msg.write(flags);
				}

				msg.done(true);
				readResults();
			}
			catch (SqlEx sqlEx)
			{
				if (trace.enabled(1))
				{
					trace.write(tr_id + ".commit: error committing transaction");
					sqlEx.trace(trace);
				}
				throw sqlEx;
			}
			finally
			{
				msg.UnlockConnection();
			}

			return;
		} // commit


		/*
		** Name: rollback
		**
		** Description:
		**	Rollback XA transaction.
		**
		** Input:
		**	xid	XA XID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	20-Jul-06 (gordy, ported by thoda04 05-Mar-09)
		**	    Created.
		*/

		internal void
		rollback(AdvanXID xid)
		{
			if (trace.enabled(2)) trace.write(tr_id + ".rollback(" + xid + ")");
			warnings = null;

			if (conn.msg_protocol_level < MSG_PROTO_5)
			{
				if (trace.enabled(1)) trace.write(tr_id +
				".rollback: protocol = " + conn.msg_protocol_level);
				throw SqlEx.get(ERR_GC4019_UNSUPPORTED);
			}

			msg.LockConnection();
			try
			{
				msg.begin(MSG_XACT);
				msg.write(MSG_XACT_ROLLBACK);
				msg.write(MSG_XP_XA_FRMT);
				msg.write((short)4);
				msg.write(xid.FormatId);
				msg.write(MSG_XP_XA_GTID);
				msg.write(xid.getGlobalTransactionId());
				msg.write(MSG_XP_XA_BQUAL);
				msg.write(xid.getBranchQualifier());
				msg.done(true);
				readResults();
			}
			catch (SqlEx sqlEx)
			{
				if (trace.enabled(1))
				{
					trace.write(tr_id + ".rollback: error rolling back transaction");
					sqlEx.trace(trace);
				}
				throw sqlEx;
			}
			finally
			{
				msg.UnlockConnection();
			}

			return;
		} // rollback


		/*
		** Name: abortTransaction
		**
		** Description:
		**	Abort a distributed transaction which is not active on the 
		**	current session.  Requires a DTM connection to the server
		**	identified by getTransactionServer() of the session that
		**	is associated with the transaction.
		**
		** Input:
		**	xid	XA XID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	10-May-05 (gordy, ported by thoda04 05-Mar-09)
		**	    Created.
		*/

		internal void
		abortTransaction(AdvanXID xid)
		{
			warnings = null;

			if (trace.enabled())
				trace.log(title + ".abortTransaction( '" + xid + "' )");

			if (!conn.is_dtmc)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": DTM connection required");
				throw SqlEx.get(ERR_GC4019_UNSUPPORTED);
			}

			if (conn.msg_protocol_level < MSG_PROTO_4)
			{
				if (trace.enabled(1))
					trace.write(tr_id + ": protocol = " + conn.msg_protocol_level);
				throw SqlEx.get(ERR_GC4019_UNSUPPORTED);
			}

			msg.LockConnection();
			try
			{
				msg.begin(MSG_XACT);
				msg.write(MSG_XACT_ABORT);
				msg.write(MSG_XP_XA_FRMT);
				msg.write((short)4);
				msg.write(xid.FormatId);
				msg.write(MSG_XP_XA_GTID);
				msg.write(xid.getGlobalTransactionId());
				msg.write(MSG_XP_XA_BQUAL);
				msg.write(xid.getBranchQualifier());
				msg.done(true);
				readResults();
			}
			catch (SqlEx ex)
			{
				if (trace.enabled())
					trace.log(title + ".abortTransaction(): error aborting xact");
				if (trace.enabled(1)) ex.trace(trace);
				throw ex;
			}
			finally
			{
				msg.UnlockConnection();
			}

			return;
		} // abortTransaction


		/*
		** Name: getPreparedTransactionIDs
		**
		** Description:
		**	Returns a list of distributed transaction IDs which
		**	are active for the database specified.  The connection
		**	must be for the installation master database 'iidbdb'.
		**
		** Input:
		**	db	Database name.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	AdvanXID[]   Transaction IDs.
		**
		** History:
		**	18-Apr-01 (gordy)
		**	    Created.*/
		
		internal virtual AdvanXID[] getPreparedTransactionIDs(String db)
		{
			if (trace.enabled())
				trace.log(title + ".getPreparedTransactionIDs('" + db + "')");
			warnings = null;
			
			if (conn.msg_protocol_level >= MSG_PROTO_2)
			{
				/*
				** Request the info from the Server.
				*/
				if (dbXids == null)
					dbXids = new AdvanXids(conn);
				return (dbXids.readXids(db));
			}
			
			return (new AdvanXID[0]);  // no more XIDs, return empty list
		} // getPreparedTransactionIDs


		/*
		** Name: CreateEmptyResultSet
		**
		** Description:
		**	Returns and empty result set.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	RsltData   An empty result set.
		**
		** History:
		**	16-Dec-03 (thoda04)
		**	    Created.
		*/
		
		internal RsltData CreateEmptyResultSet()
		{
			return new RsltData(conn, new AdvanRSMD(0, trace), null);
		}


		/*
		** Name: ReferenceCountDecrement
		**
		** Description:
		**	Decrements the AdvanConnect reference count in a thread-safe context.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	The current AdvanConnect.referenceCount after the decrement.
		**
		** History:
		**	29-Feb-08 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Decrements the AdvanConnect reference count in a thread-safe context.
		/// </summary>
		/// <returns>Returns current AdvanConnect.referenceCount after the decrement.</returns>
		internal int ReferenceCountDecrement()
		{
			int refCount;

			lock(this)
			{
				refCount = this.referenceCount - 1;
				if (refCount >= 0)  // make sure use count does not go negative
					this.referenceCount = refCount;

				return this.referenceCount;
			}  // end lock
		}


		/*
		** Name: ReferenceCountIncrement
		**
		** Description:
		**	Decrements the AdvanConnect reference count in a thread-safe context.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	The current AdvanConnect.referenceCount after the increment.
		**
		** History:
		**	29-Feb-08 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Increments the AdvanConnect reference count in a thread-safe context.
		/// </summary>
		/// <returns>Returns current AdvanConnect.referenceCount after the increment.</returns>
		internal int ReferenceCountIncrement()
		{
			lock (this)
			{
				return ++this.referenceCount;
			}  // end lock
		}


		/*
		** Name: AdvanXids
		**
		** Description:
		**	Class which implements the ability to read prepared
		**	transaction IDs.
		**
		**
		**  Public Methods:
		**
		**	readXids		Read transaction IDs.
		**
		**  Private Data:
		**
		**	rsmd			Result-set Meta-data.
		**	xids			XID list.
		**
		**  Private Methods:
		**
		**	readDesc		Read data descriptor.
		**	readData		Read data.
		**
		** History:
		**	18-Apr-01 (gordy)
		**	    Created.*/
		
		private class AdvanXids : DrvObj
		{
			private static AdvanRSMD rsmd = null;
			private System.Collections.ArrayList xids;
			
			
			/*
			** Name: AdvanXids
			**
			** Description:
			**	Class constructor.
			**
			** Input:
			**	msg	Database connection.
			**	trace	Connection tracing.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	None.
			**
			** History:
			**	18-Apr-01 (gordy)
			**	    Created.*/
			
			public AdvanXids(DrvConn conn):base(conn)
			{
				xids = new System.Collections.ArrayList();
			}
			
			
			/*
			** Name: readXids
			**
			** Description:
			**	Query DBMS for transaction IDs.
			**
			** Input:
			**	db	    Database name.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	AdvanXID[]   Transaction IDs.
			**
			** History:
			**	18-Apr-01 (gordy)
			**	    Created.*/
			
			internal virtual AdvanXID[] readXids(String db)
			{
				lock(this)
				{
					xids.Clear();
					msg.LockConnection();
					try
					{
						msg.begin(MSG_REQUEST);
						msg.write(MSG_REQ_2PC_XIDS);
						msg.write(MSG_RQP_DB_NAME);
						msg.write(db);
						msg.done(true);
						readResults();
					}
					catch (SqlEx ex)
					{
						if (trace.enabled(1))
							trace.log("readXids(): failed!");
						if (trace.enabled(1))
							ex.trace(trace);
						throw;
					}
					finally
					{
						msg.UnlockConnection();
					}
					
					/*
					** XIDs have been collected in xids.
					** Allocate and populate the XID array.
					*/
					AdvanXID[] xid = new AdvanXID[xids.Count];
					
					int i=0;
					foreach(Object obj in xids)
					{
						xid[i++] = (AdvanXID) obj;
					}
					
					xids.Clear();
					return (xid);
				}
			} // readXids
			
			
			/*
			** Name: readDesc
			**
			** Description:
			**	Read a data descriptor message.  Overrides 
			**	default method in DrvObj.  It handles the
			**	reading of descriptor messages for the method
			**	readXids (above).
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
			**	18-Apr-01 (gordy)
			**	    Created.*/
			
			protected internal override AdvanRSMD readDesc()
			{
				if (rsmd == null)
					rsmd = AdvanRSMD.load(conn);
				else
					rsmd.reload(conn);
				return (rsmd);
			} // readDesc
			
			
			/*
			** Name: readData
			**
			** Description:
			**	Read a data message.  This method overrides the readData
			**	method of DrvObj and is called by the readResults
			**	method.  It handles the reading of data messages for the
			**	readXids() method (above).  The data should be in a
			**	pre-defined format (rows containing 3 non-null columns
			**	of type int, byte[], byte[]).
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
			**	18-Apr-01 (gordy)
			**	    Created.*/
			
			protected internal override bool readData()
			{
				while (msg.moreData())
				{
					msg.readByte(); // Format ID - NULL byte (never null)
					int fid = msg.readInt();
					
					msg.readByte(); // Global Transaction ID
					byte[] gtid = msg.readBytes();
					
					msg.readByte(); // Branch Qualifier
					byte[] bqual = msg.readBytes();
					
					xids.Add(new AdvanXID(fid, gtid, bqual));
				}
				
				return (false);
			} // readData

		} // class AdvanXids

	} // class AdvanConnect

}  // namespace
