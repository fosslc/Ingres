/*
** Copyright (c) 2002, 2006 Ingres Corporation. All Rights Reserved.
*/

#define DBTRANSACTION_ENABLED

/*
** Name: transact.cs
**
** Description:
**	Describes a local transaction to the database.
**
** Classes:
**	IngresTransaction.
**
** History:
**	27-Aug-02 (thoda04)
**	    Created.
**	27-Feb-04 (thoda04)
**	    Added additional XML Intellisense comments.
**	21-jun-04 (thoda04)
**	    Cleaned up code for Open Source.
**	19-Jul-05 (thoda04)
**	    Added DbTransaction base class.
**	20-Feb-08 (thoda04)
**	    Added support for IsolationLevel.Unspecified.  Fixes B119944.
*/

using System;
using System.Data;
using Ingres.Utility;

namespace Ingres.Client
{
/// <summary>
/// Represents a local transaction to be made in an Ingres database.
/// </summary>
//	Allow this type to be visible to COM.
[System.Runtime.InteropServices.ComVisible(true)]
public sealed class IngresTransaction :
#if DBTRANSACTION_ENABLED
		System.Data.Common.DbTransaction
#else
		MarshalByRefObject, IDbTransaction, IDisposable
#endif
{
	internal IngresTransaction(IngresConnection conn):
		this(conn, IsolationLevel.ReadCommitted)
	{
	}

	internal IngresTransaction(IngresConnection conn, IsolationLevel level)
	{
		switch(level)
		{
			case IsolationLevel.ReadUncommitted: 
			case IsolationLevel.ReadCommitted: 
			case IsolationLevel.RepeatableRead: 
			case IsolationLevel.Serializable: 
				break;
			case IsolationLevel.Unspecified:
				level = IsolationLevel.ReadCommitted;  // default
				break;
			default: 
				throw new NotSupportedException(
					"IngresTransaction(IsolationLevel."+level.ToString()+")");
		}

		_connection     = conn;
		_isolationLevel = level;
	}

	/// <summary>
	/// Finalizer for the IngresTransaction object.
	/// </summary>
	~IngresTransaction() // Finalize code
	{
		Dispose(false);
	}

	/// <summary>
	/// Release allocated resources of the Transaction.
	/// </summary>
	protected override void Dispose(bool disposing)
	{
		/*if disposing == true  then method called by user code
		  if disposing == false then method called by runtime from inside the
				finalizer and we should not reference other objects. */
		if (disposing)
		{
			_connection     = null;
		}
	}

	/*
	** PROPERTIES
	*/

	private IngresConnection _connection;
	/// <summary>
	/// Returns the Connection associated with the Transaction.
	/// </summary>
#if DBTRANSACTION_ENABLED
	public new   System.Data.Common.DbConnection Connection
#else
	public                         IDbConnection Connection
#endif
	{
		get { return _connection; }  // Connection object associated w/ txn
	}

#if DBTRANSACTION_ENABLED
	/// <summary>
	/// Returns the DbConnection associated with the Transaction.
	/// </summary>
	protected override System.Data.Common.DbConnection DbConnection
	{
		get { return _connection; }  // Connection object associated w/ txn
	}
#endif


	private IsolationLevel _isolationLevel;  // = IsolationLevel.ReadCommitted;
	/// <summary>
	/// Returns the current isolation level associated with the Transaction.
	/// </summary>
#if DBTRANSACTION_ENABLED
	public override  IsolationLevel  IsolationLevel
#else
	public           IsolationLevel  IsolationLevel
#endif
	{
		get { return _isolationLevel; }  // current transaction isolation level
	}

	private  bool _hasAlreadyBeenCommittedOrRolledBack = false;
	internal bool  HasAlreadyBeenCommittedOrRolledBack
	{
		get { return _hasAlreadyBeenCommittedOrRolledBack; }
		set { _hasAlreadyBeenCommittedOrRolledBack = value; }
	}

	/*
	** METHODS
	*/

	/// <summary>
	/// Commits the transaction and removes the transaction from the Connection.
	/// </summary>
#if DBTRANSACTION_ENABLED
	public override  void Commit()
#else
	public           void Commit()
#endif
	{
		// Connection must be already open
		if (_connection == null || _connection.State != ConnectionState.Open)
			throw new InvalidOperationException(
				"The connection is not open.");

		if (HasAlreadyBeenCommittedOrRolledBack)
			throw new InvalidOperationException(
				"The transaction has already been committed or rolled back.");

			try
			{
				_connection.advanConnect.commit();  // Commit
				_connection.Transaction = null;     // Connection has no tx now
			}
			catch( SqlEx ex )  { throw ex.createProviderException(); }

		// invalidate the Transaction object to avoid reusing
		HasAlreadyBeenCommittedOrRolledBack = true;
	}

	/// <summary>
	/// Rolls back the transaction and removes the
	/// transaction from the Connection.
	/// </summary>
#if DBTRANSACTION_ENABLED
	public override  void Rollback()
#else
	public           void Rollback()
#endif
	{
		// Connection must be already open
		if (_connection == null ||
			_connection.State != ConnectionState.Open)
				throw new InvalidOperationException(
					"The connection is not open.");

		if (HasAlreadyBeenCommittedOrRolledBack)
			throw new InvalidOperationException(
				"The transaction has already been committed or rolled back.");

			try
			{
				_connection.advanConnect.rollback(); // Rollback
				_connection.Transaction = null;     // Connection has no tx now
			}
			catch( SqlEx ex )  { throw ex.createProviderException(); }

		// invalidate the Transaction object to avoid reusing
		HasAlreadyBeenCommittedOrRolledBack = true;
	}

}  // class
}  // namespace
