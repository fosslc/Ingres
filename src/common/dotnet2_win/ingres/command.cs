/*
** Copyright (c) 2002, 2010 Ingres Corporation. All Rights Reserved.
*/

#define DBCOMMAND_ENABLED

/*
	** Name: command.cs
	**
	** Description:
	**	Implements the .NET Provider command to be sent to a database.
	**
	** Classes:
	**	IngresCommand	Implementation of IDbCommand class.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	**	27-Feb-04 (thoda04)
	**	    Added additional XML Intellisense comments.
	**	 9-Mar-04 (thoda04)
	**	    Fix to { call myproc } and added support for { ?=call myproc }.
	**	 9-Mar-04 (thoda04)
	**	    Added support for ICloneable interface.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 8-Oct-04 (thoda04) Bug 113213
	**	    Added DesignerSerializationVisibility to indicate to VS that
	**	    code generator should "walk into" this property and
	**	    generate code for it.
	**	 1-Nov-04 (thoda04) B113360
	**	    DB procedure return value was not being returned.
	**	 8-Feb-04 (thoda04) B113867
	**	    DB procedure Output and InputOutput parameter
	**	    values were not being returned.
	**	22-Jul-05 (thoda04) B114913
	**	    DB procedure return parm error for CommandType.StoredProcedure.
	**	13-Oct-05 (thoda04) B115418
	**	    CommandBehavior.SchemaOnly should not call database and
	**	    needs to return an empty RSLT but with non-empty RSMD metadata.
	**	14-Oct-05 (thoda04)
	**	    Added DbCommand base class.
	**	 5-Oct-07 (thoda04) Bug 119265
	**	    If CommandText changes, discard old statement prep and force re-Prepare.
	**	10-Oct-07 (thoda04) Bug 119268
	**	    If CommandText==String.Empty, don't sever connection to server.
	**	12-apr-10 (thoda04) SIR 123585
	**	    Add named parameter marker support.
	*/

using System;
using System.ComponentModel;
using System.Collections;
using System.Collections.Specialized;
using System.Data;
using System.Reflection;

using Ingres.Utility;
using Ingres.ProviderInternals;


namespace Ingres.Client
{
	/*
	** Name: IngresCommand
	**
	** Description:
	**	Represents a command containing an SQL query text or
	**	database procedure to be sent to the database.
	**
	** Public Constructors:
	**	IngresCommand()
	**	IngresCommand(string connectionString)
	**	IngresCommand(string connectionString, IngresConnection connection)
	**	IngresCommand(string connectionString, IngresConnection connection, IngresTransaction tx)
	**	IngresCommand(System.ComponentModel.IContainer container)
	**
	** Public Properties
	**	CommandText        SQL statement string to execute or
	**	                   procedure name to call.
	**	CommandTimeOut     The time, in seconds, for an attempted query
	**	                   to time-out if the query has not completed yet.
	**	                   Default is 30 seconds.
	**	CommandType        An enumeration describing how to interpret the
	**	                   the CommandText property.  Possible values are:
	**	                      Text             CommandText is an SQL query (default).
	**	                      StoredProcedure  CommandText is name of a procedure.
	**	Connection         The IngresConnection object associated with the command.
	**	DesignTimeVisible  Gets/sets indicator as to whether command object
	**	                   should be visible on a customized WinForm Design control.
	**	                   Default is false.
	**	Parameters         Gets the IngresParameterCollection
	**	                   for the parameters associated with the SQL query or
	**	                   database procedure.
	**	Transaction        Gets/sets the IngresTransaction object
	**	                   associated with the command.  This transaction object
	**	                   must be compatible with the transaction object that
	**	                   is associated with the connection object.  Default null.
	**	UpdateRowSource    Gets/sets how results are applied to an updated rowset
	**	                   by the DbDataAdapter object (base of
	**	                   IngresDataAdatpter.
	**
	** Public Methods
	**	Cancel             Cancel the execution of the command.
	**	CreateParameter    Create a new instance of IngresParameter.
	**	Dispose            Release allocated resources.
	**	ExecuteNonQuery    Execute a command that does not return results.
	**	                   Returns the number of rows affected by the 
	**	                   update, delete, or insert SQL command.
	**	ExecuteReader()    Executes a command and builds an
	**	                   IngresDataReader.
	**	ExecuteReader(CommandBehavior) Executes a command and builds an
	**	                   IngresDataReader using CommandBehavior.
	**	ExecuteScalar      Executes a command and returns the first column of
	**	                   the first row of the result set.
	**	Prepare            Prepare the statement to executed later.
	**	ResetCommandTimeout Reset the CommandTimeout property back to its
	**	                   default value of 30 seconds.
	*/

	/// <summary>
	/// Represents a statement to execute against an Ingres database.
	/// </summary>
	//	Allow this type to be visible to COM.
	[System.Runtime.InteropServices.ComVisible(true)]
public sealed class IngresCommand :
#if DBCOMMAND_ENABLED
		System.Data.Common.DbCommand,
#else
		Component,
#endif
			IDbCommand, IDisposable, ICloneable
{
	/// <summary>
	/// Required designer variable.
	/// </summary>
	private System.ComponentModel.Container components;

	// link SqlEx.get processing to the IngresException factory
	private  SqlEx sqlEx = new SqlEx(new IngresExceptionFactory());

	IngresParameterCollection _parameters = new IngresParameterCollection();
	AdvanStmt        advanStmt;  // AdvanStmt or AdvanPrep or AdvanCall object

	/// <summary>
	/// Discard old statement prep and force a re-Prepare,
	/// usually due to the CommandText changed.
	/// </summary>
	private bool      forceRePrepare = false;

	private const int DefaultCommandTimeout = 30;

	/*
	** Name: Provider's Command constructors
	**
	** Description:
	**	Implements the instantiation of the IngresCommand.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Create a new instance of IngresCommand and add it to a container.
	/// </summary>
	/// <param name="container">IContainer to add this component to.</param>
	public IngresCommand(System.ComponentModel.IContainer container)
	{
		// Required for Windows.Forms Class Composition Designer support
		container.Add(this);
		InitializeComponent();
	}

	/// <summary>
	/// Create a new instance of IngresCommand for statement execution.
	/// </summary>
	public IngresCommand()
	{
		// Required for Windows.Forms Class Composition Designer support
		InitializeComponent();
	}
	/// <summary>
	/// Create a new instance of IngresCommand with specified SQL text string.
	/// </summary>
	/// <param name="cmdText">SQL text string.</param>
	public IngresCommand(string cmdText)
	{
		InitializeComponent();
		_commandText = cmdText;
	}

	/// <summary>
	/// Create a new instance of IngresCommand with specified SQL text string
	/// and IngresConnection.
	/// </summary>
	/// <param name="cmdText">SQL text string.</param>
	/// <param name="connection">
	/// IngresConnection to be associated with this command.</param>
	public IngresCommand(string cmdText, IngresConnection connection)
	{
		InitializeComponent();
		_commandText = cmdText;
		_connection  = connection;
	}

	/// <summary>
	/// Create a new instance of IngresCommand with specified SQL text string,
	/// IngresConnection, and Transaction objects.
	/// </summary>
	/// <param name="cmdText">SQL text string.</param>
	/// <param name="connection">
	/// IngresConnection to be associated with this command.</param>
	/// <param name="transaction">
	/// IngresTransaction to be associated with this command.</param>
	public IngresCommand(string cmdText, IngresConnection  connection,
				IngresTransaction transaction)
		{
		InitializeComponent();
		_commandText = cmdText;
		_connection  = connection;
		_transaction = transaction;
	}


	/*
	** PROPERTIES of IDbCommand
	*/

	/*
	** CommandText property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	string       _commandText = String.Empty;

	/// <summary>
	/// SQL statement string to execute or procedure name to call.
	/// </summary>
	[DefaultValue(""),RefreshProperties(RefreshProperties.All)]
	[Description("SQL statement string to execute or procedure name to call.")]
	[Editor(
		 typeof(Ingres.Client.Design.QueryDesignerUITypeEditor),
		 typeof(System.Drawing.Design.UITypeEditor))]
#if DBCOMMAND_ENABLED
	public override string CommandText
#else
	public          string CommandText
#endif
	{
		get { return _commandText;  }
		set
		{
			_commandText   = value;
			forceRePrepare = true;  // force a re-prepare of the statement
		}
	}


	/*
	** CommandTextAsBuilt private property
	**
	** History:
	**	10-Jan-03 (thoda04)
	**	    Created.
	**	10-Oct-07 (thoda04) Bug 119268
	**	    If CommandText==String.Empty, don't sever connection to server.
	**	    Wrap quotes around tablename if embedded spaces.
	*/

	/// <summary>
	/// SQL statement text suitable for execution.
	/// </summary>
	private string CommandTextAsBuilt
	{
		get
		{
			string text = _commandText;  // working copy
			if (text == null)            // safety check for null
				text = "";
			else
				text = text.Trim();      // trim leading and trailing spaces

			if (CommandType == CommandType.TableDirect)
			{
				if (text.Length == 0)
					return "select * from \"\"";
				else
					return "select * from " +
						Ingres.ProviderInternals.MetaData.WrapInQuotesIfEmbeddedSpaces(text);
			}

			if (text.Length == 0)
				text = " ";

			return text;
		}
	}


	/*
	** CommandTimeout property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	int       _commandTimeout   = DefaultCommandTimeout;

	/// <summary>
	/// The time, in seconds, for an attempted query
	/// to time-out if the query has not completed yet.
	/// </summary>
	[Description("The time, in seconds, for an attempted query " +
	             "to time-out if the query has not completed yet.")]
	[DefaultValue(DefaultCommandTimeout)]
#if DBCOMMAND_ENABLED
	public override int CommandTimeout
#else
	public          int CommandTimeout
#endif
	{
		get  { return _commandTimeout; }
		set
		{
			if (value < 0)
				throw new ArgumentException("CommandTimeout property is less than 0");
			_commandTimeout = value;
		}
	}


	/*
	** CommandType property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	CommandType   _commandType = CommandType.Text;

	/// <summary>
	/// An enumeration describing how to interpret the the CommandText property.
	/// </summary>
	[Description("An enumeration describing how to interpret the the CommandText property.")]
	[DefaultValue(CommandType.Text),RefreshProperties(RefreshProperties.All)]
#if DBCOMMAND_ENABLED
	public override  CommandType CommandType
#else
	public           CommandType CommandType
#endif
	{
			get {	return _commandType; }
			set
			{
				if (value != CommandType.Text  &&
					value != CommandType.StoredProcedure  &&
					value != CommandType.TableDirect)
						throw new NotSupportedException();
				_commandType = value;
			}
	}

	/*
	** Connection property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	IngresConnection      _connection;

	/// <summary>
	/// The IngresConnection object that is to be used by this command.
	/// </summary>
	[DefaultValue(null)]
	[Description("The IngresConnection to be used by this command.")]
#if DBCOMMAND_ENABLED
	public new  IngresConnection Connection
#else
	public      IngresConnection Connection
#endif
	{
		/*
		* The user should be able to set or change the connection at 
		* any time.
		*/
		get {	return _connection;  }
		set
		{
			if (_connection == value)  // if no change just return
				return;
			// else new connection is being set

			/*
			* The connection is associated with the transaction
			* so set the transaction object to return a null reference
			* if the connection is reset.
			*/
			if (this.Transaction != null)
			{
				/*  if (isTransactionInProgress)  TODO
				if (isTransactionInProgress())
					throw new InvalidOperationException(
						"Connection property was changed while " +
						"a transaction was in progress");
				*/
				this.Transaction = null;
			}

			// remove old event handler
			if (_connection != null)
				Connection.StateChange -=
					new StateChangeEventHandler(OnConnectionStateChange);

			_connection = value as IngresConnection;

			// add new event handler
			if (_connection != null)
				Connection.StateChange +=
					new StateChangeEventHandler(OnConnectionStateChange);
		}
	}

	IDbConnection IDbCommand.Connection
	{
		get { return this.Connection;  }
		set { this.Connection = (IngresConnection)value; }
	}

#if DBCOMMAND_ENABLED
	/// <summary>
	/// The DbConnection object that is to be used by this command.
	/// </summary>
	protected override System.Data.Common.DbConnection
		DbConnection
	{
		get { return this.Connection; }
		set { this.Connection = (IngresConnection)value; }
	}
#endif


	/*
	** DesignTimeVisible property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	private bool _designTimeVisible = false;

	/// <summary>
	/// Indicates whether the command object should be
	/// visible in a Windows Forms Designer control.
	/// </summary>
	[Browsable(false), DesignOnly(true),DefaultValue(true)]
	[Description("Indicates whether the command object should be " +
	             "visible in a Windows Forms Designer control.")]
#if DBCOMMAND_ENABLED
	public override  bool DesignTimeVisible
#else
	public           bool DesignTimeVisible
#endif
	{
		set
		{
			if (_designTimeVisible == value)
				return;
			_designTimeVisible = value;
			// clear the property and events from the cache and refresh it
			TypeDescriptor.Refresh(this);
		}
		get  { return _designTimeVisible; }
	}

	/*
	** Parameters property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	**	 8-Oct-04 (thoda04)
	**	    Added DesignerSerializationVisibility to indicate to VS that
	**	    code generator should "walk into" this property and
	**	    generate code for it.
	*/

	/// <summary>
	/// The collection of parameters to be used by this command.
	/// </summary>
	[Description("The collection of parameters to be used by this command.")]
	// DesignerSerializationVisibility signals that the Visual Studio
	// code generator should "walk into" this property and generate code for it.
	[DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
#if DBCOMMAND_ENABLED
	public new  IngresParameterCollection Parameters
#else
	public      IngresParameterCollection Parameters
#endif
	{
		get  { return _parameters; }
	}

	IDataParameterCollection IDbCommand.Parameters
	{
		get  { return _parameters; }
	}


#if DBCOMMAND_ENABLED
	/// <summary>
	/// The collection of parameters to be used by this command.
	/// </summary>
	protected override System.Data.Common.DbParameterCollection
		DbParameterCollection
	{
		get { return this.Parameters; }
	}
#endif

	/*
	** Transaction property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	IngresTransaction      _transaction;

	/// <summary>
	/// The transaction to be used by this command.
	/// </summary>
	[Browsable(false)]
	[DefaultValue(null)]
	[Description("The transaction to be used by this command.")]
#if DBCOMMAND_ENABLED
	public new  IngresTransaction Transaction
#else
	public      IngresTransaction Transaction
#endif
	{
		get {	return _transaction; }
		set {	_transaction = (value==null)?null:(IngresTransaction)value; }
	}

	IDbTransaction IDbCommand.Transaction
	{
		get {	return this.Transaction; }
		set {	this.Transaction = (IngresTransaction)value; }
	}

#if DBCOMMAND_ENABLED
	/// <summary>
	/// The DbTransaction to be used by this command.
	/// </summary>
	protected override System.Data.Common.DbTransaction DbTransaction
	{
		get { return this.Transaction; }
		set { this.Transaction = (IngresTransaction)value; }
	}
#endif

	/*
	** UpdatedRowSource property
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	UpdateRowSource _updatedRowSource = UpdateRowSource.None;

	/// <summary>
	/// How results are applied to a rowset by the DbDataAdapter.Update method.
	/// </summary>
	[DefaultValue(UpdateRowSource.None)]
	[Description("How results are applied to a rowset by the DbDataAdapter.Update method.")]
#if DBCOMMAND_ENABLED
	public override  UpdateRowSource UpdatedRowSource
#else
	public           UpdateRowSource UpdatedRowSource
#endif
	{
		get {	return _updatedRowSource;  }
		set {	_updatedRowSource = value; }
	}

	/*
	** METHODS of IdbCommand.
	*/

	/*
	** Cancel method
	**
	** Remarks:
	**	If there is no command to cancel then cancel is ignored.
	**	If cancels fails, then no exception is generated and
	**	problem is ignored.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Cancel the execution of the IngresCommand, if possible.
	/// </summary>
	[Description("Cancels the execution of the command.")]
#if DBCOMMAND_ENABLED
	public override  void Cancel()
#else
	public           void Cancel()
#endif
	{
		if (advanStmt                 == null  ||
			advanStmt.getConnection() == null)
			return;

		try
		{
			advanStmt.cancel();  // cancel the statement
		}
		catch (Exception)  // If cancels fails, then no exception is generated.
		{}
	}

	/*
	** CreateParameter method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Create a new instance of a IngresParameter.
	/// </summary>
	[Description("Create a new instance of a Parameter.")]
#if DBCOMMAND_ENABLED
	public new  IngresParameter CreateParameter()
#else
	public      IngresParameter CreateParameter()
#endif
	{
		return new IngresParameter();
	}

	IDbDataParameter IDbCommand.CreateParameter()
	{
		return new IngresParameter();
	}

#if DBCOMMAND_ENABLED
	/// <summary>
	/// Create an IngresParameter object.
	/// </summary>
	/// <returns></returns>
	protected override System.Data.Common.DbParameter CreateDbParameter()
	{
		return new IngresParameter();
	}
#endif


	/*
	** Execute method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Common code for ExecuteNonQuery and ExecuteReader.
	/// </summary>
	[Description("Common code for ExecuteNonQuery and ExecuteReader")]
	private void Execute()
	{
		/*
		* Common code for ExecuteNonQuery and ExecuteReader.
		*/

		// There must be a valid and open connection.
		if (_connection == null || _connection.State != ConnectionState.Open)
			throw new InvalidOperationException("Connection must valid and open");

		// if a DataReader is open then connection is too busy for anything else
		CheckIfDataReaderAlreadyOpen();

		// set transaction context, autocommit/manualcommit, isolation level
		SetTransactionContext();

		// if need to force a re-Prepare because CommandText changed
		if (forceRePrepare)
		{
			forceRePrepare = false;  // no longer need to force a re-Prepare
			if (advanStmt != null)   // close and clear
			{
				advanStmt.close();
				advanStmt = null;
			}
		}  // end if (forceRePrepare)

		AdvanCall advanCall = null;
		AdvanPrep advanPrep = null;

		int      paramCount;
		if (Parameters != null  &&  Parameters.Count > 0)
			paramCount = Parameters.Count;
		else
			paramCount = 0;

		if (this.isaProcedureCall())
		{
			string callSyntaxString = this.BuildCallProcedureSyntax();
			try
			{
				advanCall = _connection.advanConnect.prepareCall( callSyntaxString );
			}
			catch( SqlEx ex )  { throw ex.createProviderException(); }
			advanStmt = advanCall;  // advanStmt -> AdvanStmt obj
		}
		else  // not a procedure call
		{
			// if we have parameters then we must internally prepare the command.
			if (paramCount > 0)
				// If not already prepared, prepare the command
				// since we need to pass parameters.
				// If already prepared then advanStmt has an AdvanPrep object.
				if (advanStmt == null  ||  advanStmt as AdvanPrep == null)
					PrepareStmt();  // Build the statement as a prepared stmt.
					                // advanStmt -> AdvanStmt obj
		}

		try
		{
		if (advanStmt == null)  // if not AdvanCall nor AdvanPrep then AdvanStmt
			advanStmt = _connection.advanConnect.createStatement();
		else
			advanPrep = advanStmt as AdvanPrep; // prepared execution?
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }

		// if parameter list present then build up ParamSet.

		// default the parameter list to the 
		// existing command.Parameters collection.
		IngresParameterCollection parameters = Parameters;
		if (paramCount > 0)
		{
			// if named parameter markers are being used
			// rebuild the parameter list in the order of the markers.
			StringCollection  parameterMarkers = advanPrep.parameterMarkers;

			if (parameterMarkers.Count > 0 &&       // rebuild if "?myparm",
				parameterMarkers[0].Length > 1)     // but not if "?"
			{
				parameters = new IngresParameterCollection();
				foreach (String parameterMarker in parameterMarkers)
				{
					if (Parameters.Contains(parameterMarker) == false)
						throw new SqlEx(
							"Named parameter marker \"" +
							parameterMarker + "\" " +
							"was not found in the IngresCommand.Parameters " +
							"collection.");
					IngresParameter parm = Parameters[parameterMarker];
					parameters.Add(parm.Clone());
				}
			}


			int paramIndex = -1;
			foreach(IngresParameter parm in parameters)
			{
				paramIndex++;
				// if Output or ReturnValue parm then there is no value yet.
				if (parm.Direction == ParameterDirection.ReturnValue)
					continue;
				else
				if (parm.Direction == ParameterDirection.Output)
				{
					if (advanCall != null)
						advanCall.registerOutParameter(paramIndex, parm.ProviderType);
					continue;
				}
				else
				if (parm.Direction == ParameterDirection.InputOutput)
				{
					if (advanCall != null)
						advanCall.registerOutParameter(paramIndex, parm.ProviderType);
					// fall through to send out the Input value
				}

				if (parm.Value == null)
				{
					string parmName = parm.ParameterName;
					if (parmName == null)
						parmName = "";

					string msg =
						"Parameter '{0}' in ParameterCollection " +
						"at index {1} has Parameter.Value equal to null.  " +
						"Set Parameter.Value equal to DBNull.Value " +
						"if sending a null parameter to the database.";
					throw new IngresException(
						String.Format(msg, parmName, paramIndex));
				}

				try
				{
					if (Convert.IsDBNull(parm.Value))
						advanPrep.setNull(
							paramIndex, ProviderType.DBNull);
					else
						advanPrep.setObject(
							paramIndex, parm.Value, parm.ProviderType,
							parm.Scale, parm.Size);
				}
				catch( SqlEx ex )  { throw ex.createProviderException(); }
			}
		}

		// Execute the command, prepared command, or call procedure
		try
		{
			if (advanCall != null)
			{
				advanCall.execute();             // call procedure
				// if parms are present, check for Return Value parm
				if (this.Parameters.Count > 0)
				{
					int procRsltIndex = -1;      // Result param index
					if (this.Parameters[0].Direction ==
						    ParameterDirection.ReturnValue)
						procRsltIndex = 0;
					else
					if (this.Parameters[this.Parameters.Count-1].Direction ==
						    ParameterDirection.ReturnValue)
						procRsltIndex = this.Parameters.Count-1;

					// if ReturnValue is in parm list, set the value in parm
					if (procRsltIndex >= 0)
					{
						object obj =
							advanCall.getObject(procRsltIndex);

						try
						{
						Type type = ProviderTypeMgr.GetNETType(
							this.Parameters[procRsltIndex].ProviderType);
						// if DbType == Byte, a better conversion match is Byte
						if (this.Parameters[procRsltIndex].DbType == DbType.Byte)
							type = typeof(System.Byte);
						// if possible, convert to target type
							if (obj != null )
								obj = Convert.ChangeType(obj, type);
						}
						catch (InvalidCastException ex)
						{
							throw new IngresException(
								"InvalidCastException thrown while converting "+
								"DB procedure return value to target data type " +
								"as specified by IngresParameter.", ex);
						}
						this.Parameters[procRsltIndex].Value = obj;
					}  // if (procRsltIndex >= 0)

					// retrieve the value for Output and InputOutput parms
					int i = -1;  // parameter index [0:parmcnt-1]
					foreach(IngresParameter parm in this.Parameters)
					{
						i++;  // bump up parameter index
						// skip parm if Input or ReturnValue
						if (parm.Direction != ParameterDirection.Output  &&
							parm.Direction != ParameterDirection.InputOutput)
							continue;
						this.Parameters[i].Value = 
							advanCall.getObject(i);  // retrieve the output value
					}  // end foreach IngresParameter

				}  // if (this.Parameters.Count > 0)
			}
			else
			if (advanPrep != null)
				advanPrep.execute();             // execute prepared statement
			else
				advanStmt.execute(CommandTextAsBuilt); // execute statement directly
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }

		FireInfoMessageEvent();  // call the delegates with any warning messages

	}  // Execute


	/*
	** ExecuteNonQuery method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Execute a command that does not return results.
	/// Returns the number of rows affected by the update,
	/// delete, or insert SQL command.
	/// </summary>
	[Description("Execute a command that does not return results.  " + 
	             "Returns the number of rows affected by the update, " + 
	             "delete, or insert SQL command.")]
#if DBCOMMAND_ENABLED
	public override  int ExecuteNonQuery()
#else
	public           int ExecuteNonQuery()
#endif
	{
		int      updateCount;

		try
		{
			// call proc, execute the prepared stmt, or execute the stmt directly
			this.Execute();

			updateCount = advanStmt.getUpdateCount();  // get # of recs affected

			advanStmt.close();
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }

		return updateCount;
	} // ExecuteNonQuery

	/*
	** ExecuteReader methods
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	[Description("Executes a command and builds a DataReader.")]
	IDataReader IDbCommand.ExecuteReader()
	{
		return (IDataReader)ExecuteReader(CommandBehavior.Default);
	}

	[Description("Executes a command and builds a DataReader.")]
	IDataReader IDbCommand.ExecuteReader(CommandBehavior behavior)
	{
		return (IDataReader)ExecuteReader(behavior);
	}

	/// <summary>
	/// Executes a command and builds a DataReader.
	/// </summary>
	[Description("Executes a command and builds a DataReader.")]
#if DBCOMMAND_ENABLED
	public new  IngresDataReader ExecuteReader()
#else
	public      IngresDataReader ExecuteReader()
#endif
	{
		return ((IngresDataReader)ExecuteReader(CommandBehavior.Default));
	}

	/// <summary>
	/// Executes a command and builds a DataReader.
	/// </summary>
	[Description("Executes a command and builds a DataReader.")]
#if DBCOMMAND_ENABLED
	public new  IngresDataReader ExecuteReader(CommandBehavior behavior)
#else
	public      IngresDataReader ExecuteReader(CommandBehavior behavior)
#endif
	{
		int updateCount = -1;
		AdvanRslt resultset = null;

		// Check for an open connection.
		if (_connection == null || _connection.State != ConnectionState.Open)
			throw new InvalidOperationException("Connection must be valid and open");

		// if a DataReader is open then connection is too busy for anything else
		CheckIfDataReaderAlreadyOpen();

		// set transaction context, autocommit/manualcommit, isolation level
		SetTransactionContext();

		// if KeyInfo or Schema info wanted, go flesh out the metadata
		// information and possibly read the catalog (expensive!)
		MetaData keyInfoMetaData = null;
		if ((behavior &
			(CommandBehavior.KeyInfo | CommandBehavior.SchemaOnly)) != 0)
		{
			if ((behavior & CommandBehavior.SchemaOnly) != 0)
			{
				if (this.isaProcedureCall())
					throw new IngresException(
						"CommandBehavior.SchemaOnly is only supported for SELECT commands.");
			}

			keyInfoMetaData = Connection.GetKeyInfoMetaData(CommandTextAsBuilt);
		}


		// call proc, execute the prepared stmt, or execute the stmt directly
		try
		{
			if ((behavior & CommandBehavior.SchemaOnly) != 0)  // SchemaOnly
			{
				this.PrepareStmt();
				AdvanPrep advanPrep = advanStmt as AdvanPrep;
				// get an empty result set but with metadata (rsmd) for the GetSchema
				if (advanPrep != null)
					resultset = advanPrep.getResultSetEmpty();
			}
			else
			{
				this.Execute();
				updateCount = advanStmt.getUpdateCount();  // get # of recs affected
				resultset   = advanStmt.getResultSet();    // get the result set
			}
		}
		catch (SqlEx ex) { throw ex.createProviderException(); }

		return new IngresDataReader(  // construct the DataReader
			resultset, _connection, behavior, updateCount, keyInfoMetaData);
	} // ExecuteReader


#if DBCOMMAND_ENABLED
	/// <summary>
	/// Executes a command and builds an IngresDataReader.
	/// </summary>
	/// <param name="behavior"></param>
	/// <returns></returns>
	protected override System.Data.Common.DbDataReader
		ExecuteDbDataReader(CommandBehavior behavior)
	{
		return this.ExecuteReader(behavior);
	}
#endif


		/*
	** ExecuteScalar method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Executes a command and returns the first column
	/// of the first row of the result set.
	/// </summary>
	[Description("Executes a command and returns the first column" +
	             "of the first row of the result set.")]
#if DBCOMMAND_ENABLED
	public override  object ExecuteScalar()
#else
	public           object ExecuteScalar()
#endif
	{
		/*
		* ExecuteScalar assumes that the command will return a single
		* row with a single column.  If more rows/columns are returned
		* then just return the first column of the first row.
		*/

		IDataReader reader = this.ExecuteReader(CommandBehavior.SingleRow);

		Object obj = null;  // default to returning null if no result set

		if (reader.Read())  // read one row; return null if empty
		{
			obj = reader.GetValue(0);  // return first column of first row
			this.Cancel();             // cancel the remainder of the result set
		}

		reader.Close();
		return obj;
	} // ExecuteScalar

	/*
	** Prepare method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Prepare the statement for later execution.
	/// </summary>
	[Description("Prepare the statement to be executed later.")]
#if DBCOMMAND_ENABLED
	public override  void Prepare()
#else
	public           void Prepare()
#endif
	{
		// Check for an open connection.
		if (_connection == null || _connection.State != ConnectionState.Open)
			throw new InvalidOperationException("Connection must valid and open");

		CheckIfDataReaderAlreadyOpen();  // only one DataReader is allowed

		if (isaProcedureCall())  // if a procedure call then ignore Prepare
			return;              // and let the execute do the work.

		PrepareStmt();  // Prepare the statement.
	} // Prepare

	/*
	** PrepareStmt method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	[Description("Prepare the statement to be executed later.")]
	private void PrepareStmt()
	{
		try
		{
			if (advanStmt != null)  // close and clear any other statement
			{
				advanStmt.close();
				advanStmt = null;
			}

			forceRePrepare = false;  // no longer need to force a re-Prepare

			// Prepare the command.
			advanStmt = _connection.advanConnect.prepareStatement(
				CommandTextAsBuilt);
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }

		FireInfoMessageEvent();  // call the delegates with any warning messages
	}  // PrepareStmt

	/*
	** ResetCommandTimeout method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Reset the CommandTimeout property back to its default value of 30 seconds.
	/// </summary>
	[Description("Reset the CommandTimeout property back to its default value of 30 seconds.")]
	public void ResetCommandTimeout()
	{
		CommandTimeout = DefaultCommandTimeout;
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

	/*
	** Finalize method
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Finalizer for the IngresCommand object.
	/// </summary>
	~IngresCommand()   // Finalize code
	{
		Dispose(false);
	}

	/*
	** Dispose methods
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Release allocated resources of the Command and base Component.
	/// </summary>
	[Description("Release allocated resources of the Command and base Component.")]
	public new void Dispose()
	{
		Dispose(true);              // Object is being explicitly
		                            // disposed of, not finalized
		GC.SuppressFinalize(this);  // Take us off Finalization queue to prevent
		                            // executing finalization a second time
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
					Connection = null;
					_transaction = null;
					_commandText = null;
					_parameters = null;
					advanStmt   = null;
				}
			}
			finally
			{
				base.Dispose(disposing);  // let component base do its cleanup
			}
		}
	}


	/*
	** FireInfoMessageEvent method
	**
	** History:
	**	15-Nov-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Call the delegates with any warning messages.
	/// </summary>
	[Description("Call the delegates with any warning messages.")]
	private void FireInfoMessageEvent()

	{
		IngresErrorCollection      messages;
		SQLWarningCollection         warnings;

		if (advanStmt == null)
			return;

		warnings = advanStmt.Warnings;
		if (warnings == null)
			return;

		messages = new IngresErrorCollection();

		foreach (object obj in warnings)
		{
			SqlEx ex = (SqlEx) obj;
			IngresException ingresEx =
				(IngresException)ex.createProviderException();
			foreach (IngresError msg in ingresEx.Errors)
				messages.Add(msg);
		}

		if (messages == null)
			return;

		if (this._connection != null)
			this._connection.FireInfoMessageEvent(messages);

		advanStmt.clearWarnings();  // make sure we don't re-send warnings
	}


	/*
	** CheckIfDataReaderAlreadyOpen
	**
	** History:
	**	21-Nov-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Check if Connection is busy with an open DataReader.
	/// </summary>
	[Description("Check if Connection is busy with an open DataReader.")]
	private void CheckIfDataReaderAlreadyOpen()
	{
		if (this._connection.ActiveDataReader != null)
			throw new InvalidOperationException(
				"The connection is already associated with an open " +
				"DataReader.  The DataReader must be closed first.");
	}

	/*
	** isaProcedureCall
	**
	** History:
	**	21-Nov-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Return true if using \"{call ...}\" syntax or
	/// CommandType == CommandType.StoredProcedure.
	/// </summary>
	[Description("Return true if using \"{call ...}\" syntax or " +
		"CommandType == CommandType.StoredProcedure.")]
	private bool isaProcedureCall()
	{
		if (this._commandText == null)  // if no statement syntax, return false
			return false;

		string s = _commandText.TrimStart(null);
		if (s.Length == 0)
			return false;

		if (this._commandType == CommandType.StoredProcedure)
			return true;

		if (this._commandType == CommandType.Text  &&
			s.Length > 0  &&  s[0].Equals('{'))
		{
			s = s.Substring(1).TrimStart(null).ToLower(
				System.Globalization.CultureInfo.InvariantCulture);
			if (s.Length >= 1  &&  s[0].Equals('?'))  // "? = call myproc"
			{
				s = s.Substring(1).TrimStart(null);
				if (s.Length >= 1  &&  s[0].Equals('='))
					s = s.Substring(1).TrimStart(null);
				else
					return false;
			}
			if (s.Length >= 5  &&  s.Substring(0,5).Equals("call "))
					return true;
		}

		return false;
	}

	/*
	** BuildCallProcedureSyntax
	**
	** History:
	**	21-Nov-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Build the procedure name into {call procname(?,?)} format.
	/// </summary>
	[Description("Build the procedure name into {call procname(?,?)}.")]
	private string BuildCallProcedureSyntax()
	{
		if (CommandType == CommandType.Text)  // return if already "{call ...}"
			return _commandText;

		// check if already in "{ call ...}" form
		string t = _commandText.TrimStart(null);
		if (t.Length == 0)          // safety check
			return _commandText;
		if (t[0].Equals('{'))       // return if already in "{ call ...}" form
			return t;

		int parmCount = 0;    // count of non-ReturnValue parms
		bool hasReturnValue = false;
		if (Parameters != null && Parameters.Count != 0)
		{
			// drop ReturnValue parameters from the parameter marker list
			foreach (IngresParameter parm in Parameters)
				if (parm.Direction == ParameterDirection.ReturnValue)
				{
					hasReturnValue = true;
					if (parmCount != 0)  // ReturnValue parm must be first
						throw new IngresException(
							"A parameter with ParameterDirection.ReturnValue " +
							"must be specified before all other parameters");
				}
				else
					parmCount++;  // incr count of non-ReturnValue parms
		}

		System.Text.StringBuilder s = new System.Text.StringBuilder(100);

		if (hasReturnValue)
			s.Append("{?=call "); // {?=call
		else
			s.Append("{call ");  // {call

		s.Append(_commandText);  // {call myproc
		if (parmCount > 0)
		{
			s.Append("(?");      // {call myproc(?
			for (int i = 1; i < parmCount; i++)
				s.Append(",?");  // {call myproc(?,?,?
			s.Append(")");       // {call myproc(?,?,?)
		}
		s.Append("}");           // {call myproc(?,?,?)}

		return (s.ToString());
	}


	/*
	** SetTransactionContext
	**
	** Description:
	**	Set transaction context into the Connection,
	**	set autocommit mode (auto or manual), and
	**	set isolation level
	**
	** History:
	**	14-Jan-03 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Check and set the Transaction context of the connection.
	/// I.e. set the autocommit mode to the correct automatic/manual mode.
	/// </summary>
	[Description("Check and set the Transaction context of the connection.")]
	private void SetTransactionContext()
	{
		// set transaction context into the Connection and set autocommit mode

		if (Connection.Transaction != null) // if Connection has an existing txn
			if (this.Transaction == null)  //  but Command does not then error.
				throw new InvalidOperationException(
					"A local transaction is active but " +
					"Command.Transaction was not set.");
			else  // Command has a Transaction object
				if (this.Transaction != Connection.Transaction)  // must be same
					throw new InvalidOperationException(
						"Command.Transaction property is set to a different " +
						"Transaction object than the current transaction context.");
				else
					return;  // txn==txn.  Manual commit must already be set.

		// Connection has no Transaction.  Check if Command wants one.
		if (this.Transaction != null  &&
			this.Transaction.HasAlreadyBeenCommittedOrRolledBack)
			throw new InvalidOperationException(  // bad Transaction object
				"The transaction has already been committed or rolled back.");

//		if ( no Transaction of any kind)  // wants autocommit=on
//		else                              // wants manual commit
		bool autoCommitDesired = (
			this.Transaction == null  &&            // local transaction
			this.Connection.DTCenlistment == null); // distributed transaction
		try
		{
			bool autoCommitCurrent = Connection.advanConnect.getAutoCommit();
			if (autoCommitCurrent != autoCommitDesired)
					Connection.advanConnect.setAutoCommit(autoCommitDesired);
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }

		// set Connection's txn context to Command's Transaction object or null
		Connection.Transaction = this.Transaction;

		// set Connection's IsolationLevel from Transaction's IsolationLevel
		try
		{
			IsolationLevel isolationDesired;
			IsolationLevel isolationCurrent = 
				Connection.advanConnect.getTransactionIsolation();
			if (this.Transaction != null)
				isolationDesired = this.Transaction.IsolationLevel;
			else
				isolationDesired = IsolationLevel.Serializable;
					// if no Transaction object then default to Ingres default

			if (isolationDesired != isolationCurrent)
			{
				// assume OK to avoid looping recursion 
				// (Execute->setTransactionIsolation->
				//           sendTransactionQuery->Execute->...
				Connection.advanConnect.setTransactionIsolation(isolationDesired);
			}
		}
		catch( SqlEx ex )  { throw ex.createProviderException(); }

		return;
	}

	private void OnConnectionStateChange(
		object sender, StateChangeEventArgs args)
	{
		// if connection closing, clear out old context
		if (args.CurrentState == ConnectionState.Closed)
		{
			this.advanStmt = null;   // clear out old statement
		}
	}

	#region ICloneable Members
	/// <summary>
	/// Returns a clone of the Command object
	/// </summary>
	/// <returns></returns>
	public IngresCommand Clone()
	{
		IngresCommand newCommand = new IngresCommand();

		newCommand.CommandText      = this.CommandText;
		newCommand.CommandTimeout   = this.CommandTimeout;
		newCommand.CommandType      = this.CommandType;
		newCommand.Connection       = this.Connection;
		newCommand.DesignTimeVisible= this.DesignTimeVisible;
		newCommand.Transaction      = this.Transaction;
		newCommand.UpdatedRowSource = this.UpdatedRowSource;

		if (this.Parameters != null)
			foreach(IngresParameter oldParm in this.Parameters)
			{
				newCommand.Parameters.Add((IngresParameter)oldParm.Clone());
			}

		return newCommand;
	}

	object ICloneable.Clone()
	{
		return this.Clone();
	}

	#endregion
	}  // class
}  // namespace

namespace Ingres.Client.Design
{
	/*
	** Name: QueryDesignerUITypeEditor class
	**
	** Description:
	**	Launch an editor to show the QueryDesignerForm.
	**
	** Called by:
	**	CommandText UITypeEditor
	**
	** History:
	**	12-May-03 (thoda04)
	**	    Created.
	*/

	internal sealed class QueryDesignerUITypeEditor :
		System.Drawing.Design.UITypeEditor
	{
		public QueryDesignerUITypeEditor() : base()
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
				"Ingres.Client.Design.QueryDesignerUITypeEditor");
			if (t == null)
				return null;
			object o = Activator.CreateInstance(t);
			MethodInfo mi = t.GetMethod("EditIngresValue");
			if (mi == null)
				return null;
			return mi.Invoke(o, parms);
		}

	}  // class QueryDesignerUITypeEditor

}  // namespace

