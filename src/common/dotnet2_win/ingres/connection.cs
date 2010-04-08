/*
** Copyright (c) 2002, 2008 Ingres Corporation. All Rights Reserved.
*/

/*
	** Name: connection.cs
	**
	** Description:
	**	Implements the .NET Provider connection to a database.
	**
	** Classes:
	**	IngresConnection	Connection class.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	**	24-Feb-04 (thoda04)
	**	    Reworked SqlEx and IngresException to avoid forcing the user
	**	    to add a reference to their application to reference the
	**	    another assembly to resolve the SqlEx class at compile time.
	**	    We want the application to only reference the Client assembly.
	**	27-Feb-04 (thoda04)
	**	    Added additional XML Intellisense comments.
	**	 9-Mar-04 (thoda04)
	**	    Added support for ICloneable interface.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	19-Jul-05 (thoda04)
	**	    Added DbConnection base class.
	**	01-feb-08 (thoda04)
	**	    Added EnlistTransaction method.
	**	    Added TransactionScope support.
	**	10-sep-08 (thoda04)
	**	    BeginTransaction failed to turn off autocommit.
	*/

using System;
using System.ComponentModel;
using System.Collections;
using System.Data;
using System.Data.Common;
using System.Reflection;

using Ingres.Utility;
using Ingres.ProviderInternals;

namespace Ingres.Client
{
	/*
	** Name: IngresConnection
	**
	** Description:
	**	Represents a connection to the database.
	**	base functionality of the ResultSet interface.
	**	Provides access to a subset of the result-set which
	**	must be loaded by a sub-class.  Support is provided 
	**	for multi-row and partial-row subsets.
	**
	** Public Constructors:
	**	IngresConnection()
	**	IngresConnection(string connectionString)
	**	IngresConnection(System.ComponentModel.IContainer container)
	**
	** Public Properties
	**	ConnectionString   String to connect to a database.  Can only be
	**	                   set if connection is closed.  Resetting the
	**	                   connection string resets the ConnectionTimeOut
	**	                   and Database properties.
	**	ConnectionTimeOut  The time, in seconds, for an attempted connection
	**	                   to abort if the connection could not be establised.
	**	                   Default is 30 seconds.
	**	Container          Gets the IContainer that contains this object
	**	                   (inherited from Component)
	**	Database           Gets database currently in use.  Default is "".
	**	DataSource         Gets the name of the server if known, else "".
	**	ServerVersion      Gets the version string of the DBMS server.
	**	State              Gets the current state of the connection.
	**
	** Public Methods
	**	BeginTransaction   Begins a local transaction
	**	BeginTransaction(IsolationLevel)
	**	                   Begins a local transaction using the isolation level
	**	ChangeDatabase     Change the current database for a current connection
	**	Close              Close the connection (rollback pending transaction)
	**	                   and return connection to connection pool
	**	CreateCommand      Create a IngresCommand object
	**	Dispose            Release allocated resources
	**	Open               Open a database connection or use one from the
	**	                   connection pool
	*/

	/// <summary>
	/// Represents a connection to the database.
	/// </summary>
	//	Allow this type to be visible to COM.
	[System.Runtime.InteropServices.ComVisible(true)]
	//	 DefaultEvent will cause VS.NET editor to position in the
	//	 IngresConnection1_InfoMessage method source code.
	[DefaultEvent("InfoMessage")]
	[ToolboxItem(typeof(Ingres.Client.Design.IngresConnectionToolboxItem))]
	public sealed class IngresConnection :
		System.Data.Common.DbConnection,
			IDbConnection, IDisposable, ICloneable
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private  System.ComponentModel.Container components; // = null;
		internal AdvanConnect advanConnect; // = null;
		private  ConnectStringConfig config; // = null;
		internal DTCEnlistment DTCenlistment;   // = null;

		// link SqlEx.get processing to the IngresException factory
		private  SqlEx sqlEx = new SqlEx(new IngresExceptionFactory());


		/*
		** Name: Connection constructors
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Create an IngresConnection object and add it the container.
		/// </summary>
		/// <param name="container"></param>
		public IngresConnection(System.ComponentModel.IContainer container)
		{
			// Required for Windows.Forms Class Composition Designer support

			container.Add(this);
			InitializeComponent();
		}

		/// <summary>
		/// Create an IngresConnection object.
		/// </summary>
		public IngresConnection()
		{
			// Required for Windows.Forms Class Composition Designer support
			InitializeComponent();
		}

		/// <summary>
		/// Create an IngresConnection object and
		/// initialize the connection string.
		/// </summary>
		/// <param name="connectionString"></param>
		public IngresConnection(string connectionString)
		{
			// Required for Windows.Forms Class Composition Designer support
			InitializeComponent();
			ConnectionString = connectionString;  // validate and set connection string
		}


		/*
		** Name: ConnectionString property
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		private string _connectionString          = "";
		private string _connectionStringSanitized = "";

		/// <summary>
		/// Connection string property that specifies connection parameters
		/// in keyword=value pairs.
		/// </summary>
		[DefaultValue("")]
		[SettingsBindable(true)]
			// Tells VS.NET IDE property window to also display this property
			// under "(Dynamic)" to allow user to set the key in config file
		[RefreshProperties(RefreshProperties.All)]
			// Tells VS.NET IDE to refresh ALL properties in its browser
			// to pick up the new values that ConnectionString property changed.
		[Description("The string used to connect to a host server and database.")]
		[Editor(
			typeof(Ingres.Client.Design.ConnectionStringUITypeEditor),
			typeof(System.Drawing.Design.UITypeEditor))]
		public override string  ConnectionString
		{
			get 
			{
				return _connectionString;
			}
			set 
			{
				// Parse the connection string, return name/value list and
				// a security-sanitized connection string (stripped of passwords)
				config = ConnectStringConfig.ParseConnectionString(
					value, out _connectionStringSanitized);

				// reset connection properties based on new conn string values
				_connectionString = value;                     // ConnectionString

				string connval = config.Get(DrvConst.DRV_PROP_TIMEOUT);
				if (connval != null  &&  connval.Length > 0)
					_connectionTimeout = Int32.Parse(connval); // ConnectionTimeout
				else
					_connectionTimeout = ConnectionTimeoutDefault;

				_database = config.Get(DrvConst.DRV_PROP_DB);  // Database
				if (_database == null)
					_database = "";

				_dataSource = config.Get(DrvConst.DRV_PROP_HOST);  // DataSource
				if (_dataSource == null)
					_dataSource = "";
			}
		}


		/*
		** Name: ConnectionTimeout property
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		private const int  ConnectionTimeoutDefault = 15;
		private int  _connectionTimeout = ConnectionTimeoutDefault;

		/// <summary>
		/// The time (in seconds) to wait for the connection attempt.
		/// Returns 0 to indicate an indefinite time-out period.
		/// </summary>
		[Description("The time (in seconds) to wait for the connection attempt.")]
		public override int  ConnectionTimeout
		{
			get  {  return _connectionTimeout;  }
		}


		/*
		** Name: Database property
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		private string _database = "";
		/// <summary>
		/// Returns the database name opened or to be opened upon connection.
		/// </summary>
		[Description("The database name opened or to be used upon connection.")]
		public override string  Database
		{
			get  { return _database; }
		}


		/*
		** Name: DataSource property
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		private string _dataSource = "";
		/// <summary>
		/// Returns the name of the Ingres server (host target) connected to.
		/// </summary>
		[Description("The name of the Ingres server connected to.")]
		public override string  DataSource
		{
			get  { return _dataSource; }
		}


		/*
		** Name: GetMetaData
		**
		** Remarks:
		**	This method scans the SQL statement and returns its MetaData.
		**
		** History:
		**	20-Jan-04 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Get the metadata information for internal provider callers.
		/// </summary>
		/// <returns></returns>
		[Browsable(false)]
		public  MetaData  GetMetaData(string cmdText)
		{
			try
			{
			return new MetaData(
				this.advanConnect, cmdText, false);
			}
			catch( SqlEx ex )  { throw ex.createProviderException(); }
		}

		/// <summary>
		/// Return the available collections of metadata.
		/// </summary>
		/// <returns></returns>
		public override DataTable GetSchema()
		{
			return
				this.GetSchema(DbMetaDataCollectionNames.MetaDataCollections, null);
		}

		/// <summary>
		/// Return the collection of metadata specified.
		/// </summary>
		/// <param name="collectionName"></param>
		/// <returns></returns>
		public override DataTable GetSchema(string collectionName)
		{
			return
				this.GetSchema(collectionName, null);
		}

		/// <summary>
		/// Return the collection of metadata with the restrictions specified.
		/// </summary>
		/// <param name="collectionName"></param>
		/// <param name="restrictionValues"></param>
		/// <returns></returns>
		public override DataTable GetSchema(string collectionName, string[] restrictionValues)
		{
			//string debugName = (collectionName == null) ? "<null>" : collectionName;
			//System.Windows.Forms.MessageBox.Show(
			//    "GetSchema(collectionName=='"+debugName+"') Called", "IngresConnection");

			return IngresMetaData.GetSchema(
				this, collectionName, restrictionValues);
		}


		/// <summary>
		/// Get the primary key and index schema information about the query.
		/// </summary>
		/// <param name="cmdText">SQL statement to be examined.</param>
		/// <returns></returns>
		internal MetaData GetKeyInfoMetaData(string cmdText)
		{
			try
			{
				MetaData metadata =
					new MetaData(this.advanConnect, cmdText, true);
				if (metadata               == null ||  //safety check
					metadata.Columns       == null ||
					metadata.Columns.Count == 0)
					return null;

				return metadata;
			}
			catch( SqlEx ex )  { throw ex.createProviderException(); }
		}

		/*
		** Name: ServerVersion property
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		private string _serverVersion = null;

		/// <summary>
		/// The version of the database server that is connected.
		/// Format is "##.##.#### serverversionstring".
		/// </summary>
		[Browsable(false)]
		public override string  ServerVersion
		{
			get
			{
				string serverString;

				if (State == ConnectionState.Closed)
					throw new
						IngresException( GcfErr.ERR_GC4004_CONNECTION_CLOSED );
				// "A request was made on a closed connection."
				if (_serverVersion == null)
				{
					_serverVersion = "00.00.0000 ";

					IDbCommand cmd = this.CreateCommand();
					cmd.CommandText = "select dbmsinfo('_version') for readonly";
					object o = cmd.ExecuteScalar();
					cmd.Dispose();

					if (o == null)
					{
						_serverVersion += "(unknown)";
						return _serverVersion;
					}

					serverString = (string)o;
					// serverString now has the release string.  For example:
					//      "OI 1.2/01 (su4.us5/78)"
					//      "6.2/01 (mvs.mxa/09)"
					//      "II 2.5/0006 (su4.us5/39)"
					//      "EC DB2 2.2/9904"

					System.Text.StringBuilder sb =
						new System.Text.StringBuilder(serverString);
					_serverVersion += serverString;  // "00.00.0000 serverstring"
					if (sb.Length >= 6  &&  sb[0] == '6'  &&  sb[1] == '.')
					{  // change "6.2/01" to "00.62/1 " so that 621 < Ingres release no.
						sb[0] = '.';
						sb[1] = '6';
						sb.Insert(0, "00");  // "00.62/01"
					}

					int i = 0;
					string majorVersion = ScanServerString(ref i, sb, 2);
					string minorVersion = ScanServerString(ref i, sb, 2);
					string relseVersion = ScanServerString(ref i, sb, 4);

					_serverVersion =   // "##.##.#### II 2.5/0006 (su4.us5/39)"
						majorVersion + "." +
						minorVersion + "." +
						relseVersion + " " +
						serverString;
				}

				return _serverVersion;
			}
		}  // ServerVersion

		private string ScanServerString(
			ref int i, System.Text.StringBuilder sb, int resultLength)
		{
			System.Text.StringBuilder sbResult = new System.Text.StringBuilder("0");

			// scan past garbage until first digit or lparen
			while (i < sb.Length  &&  (!Char.IsDigit(sb[i]))  &&  sb[i] != '(')
				i++;

			while (i < sb.Length  &&  Char.IsDigit(sb[i]) )
				sbResult.Append( sb[i++] );

			int iresult = 0;
			try {iresult = Convert.ToInt32( sbResult.ToString() ); }
			catch (OverflowException) {}

			if (resultLength == 2)
			{
				if (iresult > 99)
					iresult = 0;

				return iresult.ToString("d2");   // ##
			}
			else // (resultLength == 4)
			{
				if (iresult > 9999)
					iresult = 0;

				return iresult.ToString("d4");   // ####
			}
		}  // ScanServerString


		/*
		** Name: State property
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		private ConnectionState _state = ConnectionState.Closed;

		/// <summary>
		/// Get the current ConnectionState: either Open or Closed.
		/// </summary>
		[Browsable(false)]
		public override  ConnectionState  State
		{
			get {  return _state;  }
		}


		/*
		** Name: ActiveDataReader property
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		private IngresDataReader _activeDataReader; // = null;

		[Browsable(false)]
		[Description("The open DataReader that the connection is busy with.")]
		internal  IngresDataReader  ActiveDataReader
		{
			get {  return _activeDataReader;  }
			set {  _activeDataReader = value;  }
		}


		/*
		** Name: Transaction property
		**
		** Description:
		**	Event occurs on a change in ConnectionState Open/Closed state.
		**
		** History:
		**	24-Sep-02 (thoda04)
		**	    Created.
		*/

		private IngresTransaction      _transaction;
		[Browsable(false)]
		[Description("The transaction that is active for this connection.")]
		internal IngresTransaction Transaction
		{
			get { return _transaction; }
			set { _transaction = value; }
		}


		/*
		** Name: BeginTransaction
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Begin a local database transaction with a default isolation level.
		/// </summary>
		/// <returns></returns>
		public new  IngresTransaction BeginTransaction()
		{
			return this.BeginTransaction(IsolationLevel.ReadCommitted);
		}

		/// <summary>
		/// Begin a local database transaction with a specified isolation level.
		/// </summary>
		/// <param name="isoLevel"></param>
		/// <returns>An IngresTransaction object.</returns>
		public new  IngresTransaction BeginTransaction(IsolationLevel isoLevel)
		{
			if (State != ConnectionState.Open)  // conn must be already open
				throw new InvalidOperationException(
					"The connection is not open.");

			// Check that not already enlisted in a DistributedTransaction.
			if (this.DTCenlistment != null)
				throw new InvalidOperationException(
					"The connection has already enlisted " +
					"in a distributed transaction.");

			// Check that not already in a local transaction from a BeginTransaction.
			if (this.Transaction != null)
				throw new InvalidOperationException(
					"The connection has already begun a local transaction.");

			// attempt to get an IngresTransaction with the specified isoLevel
			IngresTransaction tx = new IngresTransaction(this, isoLevel);

			// make sure we can turn off autocommit before setting Transaction
			advanConnect.setAutoCommit(false);   // turn autocommit off

			Transaction = tx;  // all's well, set the Transaction property
			return Transaction;
		}

		IDbTransaction IDbConnection.BeginTransaction()
		{
			return this.BeginTransaction();
		}

		IDbTransaction IDbConnection.BeginTransaction(IsolationLevel level)
		{
			return this.BeginTransaction(level);
		}

		/// <summary>
		/// Begin a database transaction using the specified isolation level.
		/// </summary>
		/// <param name="isolationLevel"></param>
		/// <returns></returns>
		protected override DbTransaction BeginDbTransaction(
			IsolationLevel isolationLevel)
		{
			return this.BeginTransaction(isolationLevel);
		}

		/// <summary>
		/// Create an IngresCommand object.
		/// </summary>
		/// <returns></returns>
		protected override DbCommand CreateDbCommand()
		{
			return this.CreateCommand();
		}


		/*
		** Name: ChangeDatabase
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Change the database by closing the current session and reopening.
		/// </summary>
		/// <param name="database">Name of the database to re-open to.</param>
		public override void ChangeDatabase(string database)
		{
			// new dbName cannot be null, empty string, or all blanks.
			if (database == null  ||  database.Trim().Length == 0)
				throw new ArgumentException("The database name is not valid.");

			if (State != ConnectionState.Open)  // conn must be already open
				throw new InvalidOperationException(
					"The connection is not open.");

			Close();
			this._database = database;
			Open();
		}


		/*
		** Name: Open
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Open the database connection and set the ConnectionState property.
		/// </summary>
		public override void Open()
		{
			System.EnterpriseServices.ITransaction transaction;

			if (_state == ConnectionState.Open)
				throw new InvalidOperationException("The connection is already open.");

			if (config == null)  // if no connection string then bad news
				throw new InvalidOperationException("Host specification missing");

			string host = DataSource;
			if (host == null)
				throw new InvalidOperationException("Host specification missing");

			// if not "servername:port" format then
			// get the port number from keyword=value NameValueCollection
			if (host.IndexOf(":") == -1)
			{
				string port = config.Get(DrvConst.DRV_PROP_PORT);
				if (port == null || port.Length == 0)
					port = "II7"; // no "Port=" then default to "II7"
				host += ":" + port.ToString();  // add ":portid" to host
			}

			advanConnect = AdvanConnectionPoolManager.Get(
				ConnectionString, this, host, config, null, ConnectionTimeout);

			// advanConnect internal connection object may be later used by
			// two threads: the application thread and the MSDTC proxy thread.
			// The advanConnect usage count will block release of the internal
			// connection advanConnect object back into the connection pool
			// until each is finished and calls
			// advanConnect.CloseOrPutBackIntoConnectionPool() method.
			// See DTCEnlistment.Enlist() method for MSDTC proxy claim stake.
			advanConnect.ReferenceCountIncrement();  // application thread stakes it claim

			ConnectionState oldState = _state;
			_state = ConnectionState.Open;

			// Raise the connection state change event
			FireStateChange(oldState, _state);  // old, new

			string persistSecurityInfo = config.Get(DrvConst.DRV_PROP_PERSISTSEC);
			if (persistSecurityInfo == null  ||
				(ToInvariantLower(persistSecurityInfo) != "true"  &&
				 ToInvariantLower(persistSecurityInfo) != "yes"))
				_connectionString = _connectionStringSanitized;

			transaction = EnlistDistributedTransactionIsNeeded();

			if (transaction != null)
				EnlistDistributedTransaction(transaction, false);  // implicit enlistment
		}


		/*
		** Name: Close
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Close the database connection or release it back into connection pool.
		/// </summary>
		public override void Close()
		{
			if (_state == ConnectionState.Closed)
				return;

			if (ActiveDataReader != null)  // Close any open DataReader
				ActiveDataReader.Close();

			if (advanConnect != null)
			{
				if (Transaction != null  &&
					Transaction.HasAlreadyBeenCommittedOrRolledBack == false)
				{
					Transaction.Rollback();
					Transaction = null;
				}

				// put the advantage connection back into pool or close it
				advanConnect.CloseOrPutBackIntoConnectionPool();
				advanConnect = null;
			}

			ConnectionState oldState = _state;
			_state = ConnectionState.Closed;
			_serverVersion = null;  // ServerVersion property is now invalid

			// Raise the connection state change
			FireStateChange(oldState, _state);  // old, new

		}

		/*
		** Name: OpenCatalog
		**
		** History:
		**	17-jun-04 (thoda04)
		**	    Created.  Moved from advanconnect.
		*/

		/// <summary>
		/// This function is for internal provider use only.
		/// </summary>
		/// <returns></returns>
		public Catalog OpenCatalog()
		{
			try
			{
				if (this.advanConnect == null)
					return null;

				if (advanConnect.Catalog == null)
					advanConnect.Catalog =
						new Catalog(advanConnect);
				return advanConnect.Catalog;
			}
			catch( SqlEx ex )  { throw ex.createProviderException(); }
		}

		/*
		** Name: CreateCommand
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Creates an IngresCommand object associated with the IngresConnection.
		/// </summary>
		/// <returns>An IngresCommand object.</returns>
		public new  IngresCommand CreateCommand()
		{
			// Return a new instance of a command object.
			IngresCommand cmd =  new IngresCommand(
				String.Empty, this, this.Transaction);
			return cmd;
		}

		IDbCommand IDbConnection.CreateCommand()
		{
			// Return new instance of an IngresCommand object.
			return this.CreateCommand();
		}


		/*
		** Name: Dispose
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Release all resources used by the object.
		/// </summary>
		public new void Dispose()
		{
			// Object is being explicitly disposed of, not finalized
			Dispose(true);
			// Take us off Finalization queue to prevent
			// executing finalization a second time
			GC.SuppressFinalize(this);
		}

		/// <summary>
		/// Release allocated resources of the Command and base Component.
		/// </summary>
		/// <param name="disposing">
		/// true if object is being explicitly disposed of, not finalized.
		/// false if method is called by runtime from inside the finalizer.
		/// </param>
		protected override void Dispose(bool disposing)
		{
			/*if disposing == true  Object is being explicitly disposed of, not finalized
			  if disposing == false then method called by runtime from inside the
									finalizer and we should not reference other 
									objects. */
			lock(this)
			{
				try
				{
					if (disposing)
					{
						Close();
						_connectionString = null;
						config                = null;
					}
				}
				finally
				{
					base.Dispose(disposing);  // let component base do its cleanup
				}
			}
		}  // Dispose


		/*
		** Name: EnlistDistributedTransaction
		**
		** History:
		**	25-Sep-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Enlist the connection into a distributed transaction.
		/// </summary>
		/// <param name="transaction">
		/// A transaction object based on the ITransaction interface.</param>
		public void EnlistDistributedTransaction(
			System.EnterpriseServices.ITransaction transaction)
		{
			if (transaction == null)
				throw new IngresException(
					"The transaction is null.");

			EnlistDistributedTransaction(transaction, true);
		}


		/// <summary>
		/// Enlist the connection into the distributed transaction.
		/// </summary>
		/// <param name="transaction"></param>
		/// <param name="isExplicitTransaction">User transaction if true;
		/// implicit Contextutil.Transaction or System.Transactions if false.</param>
		private void EnlistDistributedTransaction(
			System.EnterpriseServices.ITransaction transaction,
			bool                                   isExplicitTransaction)
		{
			// Check that a BeginTransaction has not already been issued.
			if (Transaction != null)
				if (Transaction.HasAlreadyBeenCommittedOrRolledBack)
					Transaction = null;
				else
					throw new InvalidOperationException(
						"The connection has already begun a transaction " +
						"using BeginTransaction.");

			// The connection must be open before 
			// calling EnlistDistributedTransaction.
			if (State != ConnectionState.Open)
				throw new InvalidOperationException(
					"The connection must be open before " +
					"calling EnlistDistributedTransaction.");

			try
			{
				// If an old DTCEnlistment exists then make sure its closed
				if (this.DTCenlistment != null)
				{
					this.DTCenlistment.Delist();
					this.DTCenlistment = null;
				}

				advanConnect.setAutoCommit(false);   // turn autocommit off

				DTCEnlistment DTCenlistment = new DTCEnlistment();
				DTCenlistment.Enlist(
					this.advanConnect, transaction, config);

				this.DTCenlistment = DTCenlistment;  // a good MSDTC enlistment in the tx
			}
			catch( SqlEx ex )
			{
				throw ex.createProviderException();
			}
		}


		/*
		** Name: EnlistDistributedTransactionIsNeeded
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Determine if running is a context of a COM or .NET transaction.
		/// </summary>
		/// <returns>EnterpriseServices.ITransaction if need to enlist in a
		/// ServicedComponent or System.Transactions.Transaction.</returns>
		private System.EnterpriseServices.ITransaction   EnlistDistributedTransactionIsNeeded()
		{
			if (Transaction != null)   // return null if already in a manual transaction
				return null;

			// Default is "Enlist=true"
			string enlist = config.Get(DrvConst.DRV_PROP_ENLISTDTC);
			if (enlist != null  &&
				(ToInvariantLower(enlist) == "false"  ||
				 ToInvariantLower(enlist) == "no"))
				return null;

			// if System.Transactions.Transaction context is present
			if (System.Transactions.Transaction.Current != null)
				return
					System.Transactions.TransactionInterop.GetDtcTransaction(
						System.Transactions.Transaction.Current)
							as System.EnterpriseServices.ITransaction;

			// if COM (MTS) transaction context is present
			if (System.EnterpriseServices.ContextUtil.IsInTransaction == true)
				return
					System.EnterpriseServices.ContextUtil.Transaction
						as System.EnterpriseServices.ITransaction;

			return null;
		}

		/// <summary>
		/// Enlist the connection into the distributed transaction.
		/// </summary>
		/// <param name="transaction">An existing transaction to enlist into.</param>
		public override void EnlistTransaction(System.Transactions.Transaction transaction)
		{
			if (transaction == null)
			    throw new IngresException(
			        "The transaction is null.");

			System.Transactions.IDtcTransaction dtcTransaction =
				System.Transactions.TransactionInterop.GetDtcTransaction(transaction);

			EnlistDistributedTransaction(
				(System.EnterpriseServices.ITransaction)dtcTransaction, true);
		}


		/*
		** Name: ReleaseObjectPool
		**
		** Description:
		**	Release the connection pool when last connection is closed.
		**
		** History:
		**	23-Dec-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Release the connection pool when last connection is closed.
		/// </summary>
		public static void ReleaseObjectPool()
		{
			AdvanConnectionPoolManager.ReleasePool();
		}



		/*
		** Name: InfoMessage Event
		**
		** Description:
		**	Event occurs when provider sends a warning or informational msg.
		**
		** History:
		**	24-Sep-02 (thoda04)
		**	    Created.
		*/

		/*
		 * The objEeventInfoMessage singleton object
		 * is used as a key into the Component's event list.
		 */
		static private readonly object objEeventInfoMessage = new object(); 

		/// <summary>
		/// Event occurs when provider sends a warning or informational msg.
		/// </summary>
		public event IngresInfoMessageEventHandler InfoMessage
		{
			add    { Events.AddHandler   (objEeventInfoMessage, value); }
			remove { Events.RemoveHandler(objEeventInfoMessage, value); }
		}

		internal void FireInfoMessageEvent(IngresErrorCollection errors)
		{
			if (errors == null  ||  errors.Count == 0)  // return if no warnings
				return;

			// retrieve the delegate from the Components.Events property
			IngresInfoMessageEventHandler infoMessageEventHandler =
				(IngresInfoMessageEventHandler)Events[objEeventInfoMessage];
			if (infoMessageEventHandler != null)
			{
				infoMessageEventHandler(
					this, new IngresInfoMessageEventArgs(errors));
			}
		}



		/*
		** Name: StateChange event
		**
		** Description:
		**	Event occurs on a change in ConnectionState Open/Closed state.
		**
		** History:
		**	24-Sep-02 (thoda04)
		**	    Created.
		*/

		/*
		 * The objEventStateChange singleton object
		 * is used as a key into the Component's event list.
		 */
		static private readonly object objEventStateChange = new object(); 

		/// <summary>
		/// Event occurs on a change in ConnectionState Open/Closed state.
		/// </summary>
		public override event StateChangeEventHandler StateChange
		{
			add    { Events.AddHandler   (objEventStateChange, value); }
			remove { Events.RemoveHandler(objEventStateChange, value); }
		}

		private void FireStateChange(ConnectionState oldState,
		                             ConnectionState newState)
		{
			// retrieve the delegate from the Components.Events property
			StateChangeEventHandler stateChangeEventHandler =
				(StateChangeEventHandler)Events[objEventStateChange];
			if (stateChangeEventHandler != null)
			{
				stateChangeEventHandler(
					this, new StateChangeEventArgs(oldState, newState));
			}
		}

		private static string ToInvariantLower(string str)
		{
			return str.ToLower(System.Globalization.CultureInfo.InvariantCulture);
		}

		private static string ToInvariantUpper(string str)
		{
			return str.ToUpper(System.Globalization.CultureInfo.InvariantCulture);
		}


#region Component Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			components = new System.ComponentModel.Container();
		}
#endregion


		#region ICloneable Members
		/// <summary>
		/// Returns a new Connection object and a cloned connection string.
		/// </summary>
		/// <returns></returns>
		public object Clone()
		{
			IngresConnection newConn = new IngresConnection();
			newConn.ConnectionString =
				ConnectStringConfig.ToConnectionString(this.config);
			// overlay the connection string with the sanitized version
			// if the old connection had been openned.
			newConn._connectionString= this._connectionString;

			return newConn;
		}

		#endregion
	}  // IngresConnection class



	/*
	** Name: InfoMessage handler definition and args
	**
	** Description:
	**	Event occurs when provider sends a warning or informational msg.
	**
	** History:
	**	24-Sep-02 (thoda04)
	**	    Created.
	*/
	
	/// <summary>
	/// Defines the delegate method that will handle
	/// the InfoMessage event of the IngresConnection.
	/// </summary>
	//	Allow this type to be visible to COM.
	[System.Runtime.InteropServices.ComVisible(true)]
	public delegate void IngresInfoMessageEventHandler(
			object sender, IngresInfoMessageEventArgs e);

	/// <summary>
	/// Provides the data for the IngresInfoMessageEventArgs
	/// for the Ingres data provider.
	/// </summary>
	//	Allow this type to be visible to COM.
	[System.Runtime.InteropServices.ComVisible(true)]
	public sealed class IngresInfoMessageEventArgs : EventArgs
	{

		internal IngresInfoMessageEventArgs(
			IngresErrorCollection errors)
		{
			this._errors    = errors;
		}

		/*
		** Name: ErrorCode property
		**
		** Description:
		**	Additonal native error information.
		**
		*/
		/// <summary>
		/// Additonal native error information from database server or system.
		/// </summary>
		[Description("Additonal native error information.")]
		public  int Number
		{
			get {return (this._errors[0].Number); }
		}

		/*
		** Name: Errors property
		**
		** Description:
		**	The collection of warnings sent from the database.
		**
		*/
		private IngresErrorCollection _errors;
		/// <summary>
		/// The collection of warnings sent from the database.
		/// </summary>
		[Description("Get the collection of warnings sent from the database.")]
		public  IngresErrorCollection Errors
		{
			get {return (this._errors); }
		}

		/*
		** Name: Message property
		**
		** Description:
		**	A description of the error.
		**
		*/
		/// <summary>
		/// Text of the warning information.
		/// </summary>
		[Description("Get the text of the warning information.")]
		public  String Message
		{
			get {return (this._errors[0].Message); }
		}

		/*
		** Name: Source property
		**
		** Description:
		**	Get the name of the object that generated the warning information.
		**
		*/
		/// <summary>
		/// Get the name of the object that generated the warning information.
		/// </summary>
		[Description("Get the name of the object that " +
		             "generated the warning information.")]
		public  String Source
		{
			get {return (this._errors[0].Source); }
		}

	}  // class xxxInfoMessageEventArgs

}  // namespace


namespace Ingres.Client.Design
{
	/*
	** Name: ConnectionToolboxItem class
	**
	** Description:
	**	ToolboxItem object that holds a place
	**	in the Visual Studio .NET toolbox property page.
	**
	** Called by:
	**	DataAdapter Wizard
	**	ConnectionString UITypeEditor
	**
	** History:
	**	17-Mar-03 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// ToolboxItem for Connection object to launch the Connection
	/// Editor and include the object into the component tray.
	/// </summary>
	[System.Runtime.InteropServices.ComVisible(false)]
	[Serializable]
	internal sealed class IngresConnectionToolboxItem :
		System.Drawing.Design.ToolboxItem
	{
		public IngresConnectionToolboxItem() : base()
		{
		}

		public IngresConnectionToolboxItem(Type type) : base(type)
		{
		}


		private IngresConnectionToolboxItem(
			System.Runtime.Serialization.SerializationInfo info,
			System.Runtime.Serialization.StreamingContext context)
		{
			Deserialize(info, context);
		}

		/*
		** Name: CreateComponentsCore
		**
		** Description:
		**	Create the Connection object
		**	in the components tray of the designer.
		**
		** Called by:
		**	CreateComponents()
		**
		** Input:
		**	host	IDesignerHost
		**
		** Returns:
		**	IComponent[] array of components to be displayed as selected
		**
		** History:
		**	17-Mar-03 (thoda04)
		**	    Created.
		*/
		
		protected override IComponent[]
			CreateComponentsCore(System.ComponentModel.Design.IDesignerHost host)
		{
			Assembly assemDesign =
				IngresDataAdapterToolboxItem.LoadDesignAssembly();
			if (assemDesign == null)
				return null;

			// call the counterpart method in the VSDesign assembly.
			Type t = assemDesign.GetType(
				"Ingres.Client.Design.IngresConnectionToolboxItem");
			if (t == null)
				return null;
			object o = Activator.CreateInstance(t);
			MethodInfo mi = t.GetMethod("CreateIngresComponents");
			Object[] parms = new Object[1];
			parms[0] = host;
			if (mi == null)
				return null;
			return mi.Invoke(o, parms) as IComponent[];
		}  // CreateComponentsCore

	}  // class IngresConnectionToolboxItem


	/*
	** Name: ConnectionStringUITypeEditor class
	**
	** Description:
	**	UserInterface editor used by the VS.NET designer.
	**
	** History:
	**	17-Mar-03 (thoda04)
	**	    Created.
	*/

	internal sealed class ConnectionStringUITypeEditor :
		System.Drawing.Design.UITypeEditor
	{
		public ConnectionStringUITypeEditor() : base()
		{
		}

		public override System.Drawing.Design.UITypeEditorEditStyle
			GetEditStyle(ITypeDescriptorContext context)
		{

			if (context          != null  &&
				context.Instance != null)
				return System.Drawing.Design.UITypeEditorEditStyle.Modal;
			return base.GetEditStyle(context);
		}

		public override object EditValue(
			ITypeDescriptorContext context,
			IServiceProvider       provider,
			object                 value)
		{
			// load the VSDesign assembly
			Assembly assemDesign =
				IngresDataAdapterToolboxItem.LoadDesignAssembly();
			if (assemDesign == null)
				return null;

			// call the counterpart method in the VSDesign assembly.
			Object[] parms;
			parms    = new Object[3];
			parms[0] = context;
			parms[1] = provider;
			parms[2] = value;

			Type t = assemDesign.GetType(
				"Ingres.Client.Design.ConnectionStringUITypeEditor");
			if (t == null)
				return null;
			object o = Activator.CreateInstance(t);
			MethodInfo mi = t.GetMethod("EditIngresValue");
			if (mi == null)
				return null;
			return mi.Invoke(o, parms);
		}
	}  // class ConnectionStringUITypeEditor

}  // namespace