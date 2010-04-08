/*
** Copyright (c) 2003, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.EnterpriseServices;
using System.Runtime.InteropServices;
using System.Threading;
using Ingres.Utility;


namespace Ingres.ProviderInternals
{
	/*
	** Name: xaenlist.cs
	**
	** Description:
	**	Base classes for enlistment in the MS
	**	Distributed Transaction Coordinator.
	**
	**  Classes
	**
	**	DTCEnlistment         Represents an enlistment in DTC.
	**
	** Notes:
	**	If you wish to trace DTCEnlistment class and the XASwitch then
	**	construct a config file for the application.  The DTCEnlistment
	**	trace goes to the trace.log as usual and the XASwitch trace info
	**	(which is coming from a separate MSDTC process) goes to the
	**	event log.  Your config file looks something like this:

		<?xml version="1.0" encoding = "utf-8" ?>
		<configuration>
			<appSettings>
				<add key="Ingres.trace.log" value="C:\temp\Ingres.trace.log" />
				<add key="Ingres.trace.timestamp" value="true" />
				<add key="Ingres.trace.Ingres" value="4" />
				<add key="Ingres.trace.XA" value="4" />
			</appSettings>
		</configuration>

	** History:
	**	25-Sep-03 (thoda04)
	**	    Created.
	**	25-Feb-04 (thoda04)
	**	    Convert SqlEx exception to an IngresException.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 8-dec-04 (thoda04)
	**	    Added XACT_E_XA_TX_DISABLED case for XP SP2 KB817066.
	**	01-feb-08 (thoda04)
	**	    Added TransactionScope support.
	**	10-aug-09 (thoda04)
	**	    Use commit/rollback by xid rather than by handle.
	*/

	/* Notes:

	Typical flow of control for COMTransaction (System.EnterpriseServices.ITransaction), single phase commit:

		Application Enlisting...
		DTCEnlistment: Enlist(True) begins...
		DTCEnlistment: Asynch thread processing: ENLIST...
		DTCEnlistment:    MSDTCRequest.Enlisting with RM...
		DTCEnlistment:    MSDTCRequest.Enlisted with RM
		DTCEnlistment: Enlist() complete
		Application Enlisted
		Application tx.Committing...
		DTCEnlistment: ITransactionResourceAsync.PrepareRequest called
		DTCEnlistment: Asynch thread processing: PREPARE_SINGLE_PHASE...
		DTCEnlistment:    MSDTCRequest.PREPARE_SINGLE_PHASE...
		DTCEnlistment:       PrepareRequestDone called
		DTCEnlistment:       MSDTCRequest.PREPARE_SINGLE_PHASE releasing RMCookie...
		Application back from Aborting/Committing...
		Application Closing!!
		DTCEnlistment:       ReleaseRMCookie  (Asynch thread processing for PREPARE_SINGLE_PHASE is releasing)
		IngresConnection.Close()
		Application back from Close!!


	Typical flow of control for COMTransaction (System.EnterpriseServices.ITransaction), two phase commit:

		Application Enlisting...
		DTCEnlistment: Enlist(True) begins...
		DTCEnlistment: Asynch thread processing: ENLIST...
		DTCEnlistment:    MSDTCRequest.Enlisting with RM...
		DTCEnlistment:    MSDTCRequest.Enlisted with RM
		DTCEnlistment: Enlist() complete
		DTCEnlistment: Enlist(True) begins...
		DTCEnlistment: Asynch thread processing: ENLIST...
		DTCEnlistment:    MSDTCRequest.Enlisting with RM...
		DTCEnlistment:    MSDTCRequest.Enlisted with RM
		DTCEnlistment: Enlist() complete
		Application Enlisted

		Application tx.Committing...
		DTCEnlistment: ITransactionResourceAsync.PrepareRequest called
		DTCEnlistment: Asynch thread processing: PREPARE...
		DTCEnlistment: ITransactionResourceAsync.PrepareRequest called
		DTCEnlistment:    MSDTCRequest.PREPARE...
		DTCEnlistment: Asynch thread processing: PREPARE...
		DTCEnlistment:    MSDTCRequest.PREPARE...
		DTCEnlistment:    PrepareRequestDone called
		DTCEnlistment:    PrepareRequestDone called
		Application back from Aborting/Committing...
		Application Closing!!
		IngresConnection.Close()
		DTCEnlistment: ITransactionResourceAsync.CommitRequest called
		DTCEnlistment: Asynch thread processing: COMMIT...
		DTCEnlistment: ITransactionResourceAsync.CommitRequest called
		DTCEnlistment: Asynch thread processing: COMMIT...
		DTCEnlistment:    MSDTCRequest.COMMIT...
		DTCEnlistment:    MSDTCRequest.COMMIT...
		Application back from Close!!

	Typical flow of control for System.Transactions.Transaction in a TransactionScope, two phase commit:

		IngresConnection.Open()...
		DTCEnlistment: Enlist(False) begins...
		DTCEnlistment: Asynch thread processing: ENLIST...
		DTCEnlistment:    MSDTCRequest.Enlisting with RM...
		DTCEnlistment:    MSDTCRequest.Enlisted with RM
		DTCEnlistment: Enlist() complete
		DTCEnlistment: Enlist(False) begins...
		DTCEnlistment: Asynch thread processing: ENLIST...
		DTCEnlistment:    MSDTCRequest.Enlisting with RM...
		DTCEnlistment:    MSDTCRequest.Enlisted with RM
		DTCEnlistment: Enlist() complete
		IngresConnection.Open() complete

		Application tx.Committing...
		Application back from Committing...
		Application Closing!!
		IngresConnection.Close()
		Application back from Close!!
		TransactionScope.Dispose()...
		DTCEnlistment: ITransactionResourceAsync.PrepareRequest called
		DTCEnlistment: Asynch thread processing: PREPARE...
		DTCEnlistment:    MSDTCRequest.PREPARE...
		DTCEnlistment: ITransactionResourceAsync.PrepareRequest called
		DTCEnlistment: Asynch thread processing: PREPARE...
		DTCEnlistment:    MSDTCRequest.PREPARE...
		DTCEnlistment:    PrepareRequestDone called
		DTCEnlistment:    PrepareRequestDone called
		DTCEnlistment: ITransactionResourceAsync.CommitRequest called
		DTCEnlistment: Asynch thread processing: COMMIT...
		DTCEnlistment:    MSDTCRequest.COMMIT...
		TransactionScope.Dispose() complete
		DTCEnlistment: ITransactionResourceAsync.CommitRequest called
		DTCEnlistment: Asynch thread processing: COMMIT...
		DTCEnlistment:    MSDTCRequest.COMMIT...
		Application back from using (TransactionScope)
		DTCEnlistment:       CommitRequestDone called... Releasing RMCookie...
		DTCEnlistment: ReleaseRMCookie
		DTCEnlistment:       CommitRequestDone called... Releasing RMCookie...
		DTCEnlistment: ReleaseRMCookie
	*/


	/*
	** Class Name:   DTCEnlistment
	**
	** Description:
	**	Implementation of interfaces to MS Distributed Transaction
	**	Coordinator (MSDTC) for distributed transaction enlistment.
	**
	**  Methods:
	**
	**	Enlist   Locate MSDTC and enlist for callbacks from MSDTC Proxy
	**	Delist   Remove ourselves from MSDTC callbacks.
	**
	**
	**  ITransactionResourceAsync Implementation Methods:
	**
	**	AbortRequest   MSDTC Proxy calls this to abort a transaction.
	**	CommitRequest  MSDTC Proxy calls this to commit a transaction.
	**	PrepareRequest MSDTC Proxy calls this to prepare-to-commit a transaction.
	**	TMDown         MSDTC Proxy notification that Transaction Manager is down.
	**
	*/

	/// <summary>
	/// Represents an enlistment in MS Distributed Transaction Coordinator (MSDTC).
	/// </summary>
	[ClassInterface(ClassInterfaceType.None)]
	internal sealed class DTCEnlistment : ITransactionResourceAsync
	{
		AdvanConnect              advanConnect;
		IDtcToXaHelperSinglePipe  XaHelperSinglePipe;
		ITransactionResourceAsync TxResourceAsync;

		/// <summary>
		/// An opaque pointer to the resource manager object and
		/// represents the connection to the MS Transaction Manager.
		/// Returned by IDtcToXaHelperSinglePipe.XARMCreate.
		/// </summary>
		uint                      RMCookie = 0;

		Thread                    txResourceAsyncThread;
		private Exception         EnlistmentException;

		readonly bool debugging;

		internal String title;    // Title for tracing
		internal Ingres.Utility.ITrace trace;
		AdvanXID    advanXID;



		/// <summary>
		/// This is the MSDTC interface that the TxResourceAsync
		/// thread calls to signal completion of the prepare/commit
		/// work that MSDTC Proxy requested be done asynchonously.
		/// </summary>
		ITransactionEnlistmentAsync TxEnlistmentAsync;

		System.Collections.Queue MSDTCRequestQueue =
			new System.Collections.Queue();

		enum TXSTATE
		{
			TX_UNINITIALIZED=0,
			TX_INITIALIZING=1, TX_INITIALIZED=2,
			TX_ENLISTING=3,    TX_ENLISTED=4,
			TX_PREPARING=5,    TX_PREPARING_AND_COMMITTING_TOGETHER=6,
			TX_PREPARING_SINGLE_PHASE=7,    TX_PREPARED=8,
			TX_COMMITTING=9,   TX_COMMITTED=10,
			TX_ABORTING=13,    TX_ABORTED=14,
			TX_DONE=16, 
			TX_TMDOWNING=17, TX_TMDOWN=18, TX_INVALID_STATE=19
		};

		enum TXBRANCHSTATE
		{
			NonExistent,
			Active,
			Idle,
			Prepared,
			RollbackOnly,
			HeuristicallyCompleted
		};

		enum MSDTCRequest
		{
			UNDEFINED,
			ENLIST,
			DELIST,
			PREPARE,
			PREPARE_SINGLE_PHASE,
			PREPARE_E_UNEXPECTED,
			PREPARE_E_FAIL,
			COMMIT,
			ABORT,
			TMDOWN
		};

		const uint XACT_S_SINGLEPHASE= 0x4d009;  // from transact.h

		const int TMNOFLAGS  = 0x00000000; /* no RM features selected */
		const int TMONEPHASE = 0x40000000; /* caller is using one-phase commit optimisation */
		const int TMFAIL     = 0x20000000; /* dissociates caller and marks tx branch rollback-only */
		const int TMSUCCESS  = 0x04000000; /* dissociate caller from tx branch */


		public DTCEnlistment()
		{
			debugging = false;  // for developer debugging output to Console

			title = "DTCEnlistment";
			trace = new TraceDV("XA");
		}

		private TXSTATE _txState = TXSTATE.TX_UNINITIALIZED;
		private TXSTATE TxState
		{
			get { return _txState; }
			set
			{
				if (TxState != TXSTATE.TX_TMDOWN)  // don't overlay TMDOWN
					_txState = value;
			}
		}

		private TXBRANCHSTATE txBranchState = TXBRANCHSTATE.NonExistent;
		private TXBRANCHSTATE TxBranchState
		{
			get { return txBranchState; }
			set { txBranchState = value; }
		}

		private ITransaction _transaction = null;
		private ITransaction  Transaction
		{
			get { return _transaction; }
			set { _transaction = value; }
		}




		/*
		** Name: Enlist
		**
		** Description:
		**	Enlist in the Microsoft Distributed Transaction Coordinator (MSDTC).
		**	Called by Connection.EnlistDistributedTransaction(ITransaction).
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/
		
		/// <summary>
		/// Enlist in the Microsoft Distributed Transaction Coordinator (MSDTC).
		/// </summary>
		/// <param name="advanConnection">Internal connection.</param>
		/// <param name="tx">ITransaction interface of object.</param>
		/// <param name="nv">Connection string parameter values list.</param>
		internal void Enlist(
			AdvanConnect advanConnection,
			ITransaction tx,
			ConnectStringConfig nv)
		{
			this.advanConnect = advanConnection;
			this.Transaction = tx;

			trace.write(3, title + ": Enlist()");
			if (debugging)
				Console.WriteLine(title + ": Enlist() begins...");

			/*
			Call DtcGetTransactionManager in MS DTC proxy to 
			establish a connection to the Transaction Manager.
			From the TM, get the XATM's Single Pipe Helper interface
			that will help establish an enlistment in MSDTC
			using the XA protocol (XaSwitch and XIDs).
			*/
			XaHelperSinglePipe = GetDtcToXaHelperSinglePipe();

			/*
			Obtain the Resource Manager (RM) cookie.  The XARMCreate
			method is invoked once per connection.  The information provided
			makes it possible for the transaction manager to connect to the
			database to perform recovery.  The transaction manager durably records
			this information in it's log file.  If recovery is necessary, the 
			transaction manager reads the log file, reconnects to the database,
			and initiates XA recovery.

			Calling XARMCreate results in a message from the DTC proxy
			to the XA Transaction Manager (TM).  When the XA TM receives a message that
			contains the DSN and the name of the client DLL, TM will:
			   1) LoadLibrary the client dll (Ingres.Support.dll which will
			      drag in the referenced Ingres.ProviderInternals from the GAC).
			   2) GetProcAddress the GetXaSwitch function in the dll.
			   3) Call the GetXaSwitch function to obtain the XA_Switch.
			   4) Call the xa_open function to open an XA connection with the 
			      resource manager (Ingres).  When the our xa_open function is called,
			     the DTC TM passes in the DSN parm as the OPEN_STRING.  It then
			     closes the connection by calling xa_close.  (It's just testing
			     the connection information so that it won't be surprised
			     when it's in the middle of recovery later.)
			   5) Generate a RM GUID for the connection.
			   6) Write a record to the DTCXATM.LOG file indicating that a new
			     resource manager is being established.  The log record contains
			     the DSN parm, the name of our RM (Ingres.Support.dll), and 
			     the RM GUID.  This information will be used to reconnect to
			     Ingres should recovery be necessary.

			XARMCreate method also causes the MS DTC proxy to create a resource
			manager object and returns an opaque pointer to that object as
			the RMCookie.  This RMCookie represents the connection to the TM.
			*/

			nv = new ConnectStringConfig(nv);  // create a working copy
			// Connection attempts with MSG_P_DTMC will raise error:
			//    GC000B_RMT_LOGIN_FAIL Login failure: invalid username/password
			// if you try to send login information
			nv.Remove(DrvConst.DRV_PROP_USR);   // remove "user="
			nv.Remove(DrvConst.DRV_PROP_PWD);   // remove "password="
			nv.Remove(DrvConst.DRV_PROP_GRP);   // remove "group="
			nv.Remove(DrvConst.DRV_PROP_ROLE);  // remove "role="

			string xa_open_string = ConnectStringConfig.ToConnectionString(nv);
			int trace_level = trace.getTraceLevel("XA");
			if (trace_level > 0)
			{
				xa_open_string +=  // tell MSDTC process to trace XASwitch
					";Trace=XA;TraceLevel=" + trace_level.ToString();
			}


			// Locate the Ingres.Support.dll that holds our XASwitch
			// support.  The dll should be in the GAC so use a type from
			// the Ingres.Support assembly, qualified by the same
			// Version and PublicKeyToken that the currently executing
			// Ingres.Support assembly is using.
			System.Reflection.Assembly assembly =
				System.Reflection.Assembly.GetExecutingAssembly();
			string fullname = assembly.FullName;
			string version;
			int index = fullname.IndexOf(',');  // locate ", Verison=..."
			if (index != -1)
				version = fullname.Substring(index);
			else  // should not happen unless in a debugging environment
				version = ", Version=2.0.0.0, Culture=neutral" +
				          ", PublicKeyToken=1234567890123456";

			Type type = Type.GetType(
				"Ingres.Support.XAProvider" +
				", Ingres.Support" + version);
			// BTW, type will be null if Ingres.Support is not properly
			// in the GAC or has a different version number from
			// Ingres.Client.  A NullReferenceException would follow.
			string dll = type.Assembly.Location;

			XaHelperSinglePipe_XARMCreate(
				xa_open_string,
				dll,
				out this.RMCookie);

			/*
			Obtain an XID from an ITransaction interface pointer.
			The ITransaction pointer is offered by a transaction object.
			The transaction object contains a DTC transaction that is
			identified by a transaction identifier (TRID) which is a globally 
			unique identifier (GUID).  Given a TRID, one can always create an XID.
			The MS DTC proxy generates an XID and returns it.

			Into the XID's formatID, MS puts 0x00445443 ("DTC").
			Into the XID's gtrid (length=0x10), MS puts a GUID
			identifying the global transaction identifier.  (Same as
			Transaction.Current.TransactionInformation.DistributedIdentifier.)
			Into the XID's bqual (length=0x30), MS puts a second
			GUID identifying the TM that is coordinating the transaction 
			and also a third GUID identifying the database connection.
			If two components in the same transaction have their own connections,
			then they will be loosely coupled XA threads of control (different
			bquals).  MS goes beyond the XA spec and expects that the two
			loosely coupled threads will not deadlock on resource locks.
			*/
			XAXIDStruct xaXIDstruct = new XAXIDStruct(-1,0,0,new byte[128]);

			XaHelperSinglePipe_ConvertTridToXID(
				Transaction, this.RMCookie, ref xaXIDstruct);
			advanXID = new AdvanXID(xaXIDstruct);

			/*
			Tell the DTC proxy to enlist on behalf of the XA resource manager.
			EnlistWithRM is called for every transaction that the connection
			is requested to be enlisted in. The provider is responsible for
			implementing the ITransactionResourceAsync interface.  This
			interface includes the PrepareRequest, CommitRequest, AbortRequest,
			and TMDown callbacks.  The transaction manager will invoke these
			callback methods to deliver phase one and phase two notifications
			to the provider.
			The MS DTC proxy informs the transaction manager that the provider
			wishes to enlist in the transaction.  The transaction
			manager creates an internal enlistment object to record the fact
			that the provider is participating in the transaction.
			Upon return from EnlistWithRM, the MS DTC proxy will have created
			an enlistment object and returns the pITransactionEnlistmentAsync
			interface pointer of the enlistment object back to the provider.
			*/

			try
			{
			StartMSDTCRequest(MSDTCRequest.ENLIST);  // start worker thread

			if (EnlistmentException != null)
				throw EnlistmentException;

			// wait until EnlistWithRM is completed under the worker thread
			advanConnect.msg.LockConnection();
			advanConnect.msg.UnlockConnection();

			advanConnect.startTransaction(advanXID);
			TxBranchState = TXBRANCHSTATE.Active;
			}
			catch
			{
				// release our bad connection to MS DTC and
				// shutdown our worker thread to MS DTC.
				XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
				StartMSDTCRequest(MSDTCRequest.DELIST);
				throw;
			}

			// At this point we have a good MSDTC enlistment!

			// The advanConnect internal connection object is now being used by
			// two threads: the application thread and the MSDTC proxy thread.
			// The advanConnect usage count will block release of the internal
			// connection advanConnect object back into the connection pool
			// until each is finished and calls
			// advanConnect.CloseOrPutBackIntoConnectionPool() method.
			// See IngresConnection.Open() method for application's claim stake.
			advanConnect.ReferenceCountIncrement();  // MSDTC proxy thread stakes it claim

			trace.write(3, title + ": Enlist() complete");
			if (debugging)
				Console.WriteLine(title + ": Enlist() complete");

		}  // Enlist


		/*
		** Name: Delist
		**
		** Description:
		**	Wait for any pending prepare/commit sequences to complete,
		**	remove our enlistment in the Microsoft Distributed
		**	Transaction Coordinator (MSDTC), and release our cookie to it.
		**	Called by Connection.EnlistDistributedTransaction(ITransaction).
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/
		
		/// <summary>
		/// Wait for any pending prepare/commit sequences to complete,
		/// remove our enlistment in the Microsoft Distributed
		/// Transaction Coordinator (MSDTC), and release our cookie to it.
		/// </summary>
		public void Delist()
		{
			try
			{
			trace.write(3, title + ": Delist()");
			if (debugging)
				Console.WriteLine(title + ": Delist()");

			// Test if the TxResourceAsyncThread thread running 
			// on behalf of MSDTC Proxy is has had a chance to start up
			// and lock the connection before the application tries
			// to disconnect the connection that TxResourceAsyncThread
			// is trying to commit.  This allows
			// us to properly sequence the two threads so that
			// the prepare/commit thread under TxResourceAsyncThread
			// completes before the the disconnect thread 
			// under the application completes.
			advanConnect.msg.LockConnection();   // wait for CommitRequest of
			advanConnect.msg.UnlockConnection(); // 2PC to finish its work

			if (trace.enabled(4))
				trace.write(      title + ": Delist's MSDTCRequest.DELIST starting");
			if (debugging)
				Console.WriteLine(title + ": Delist's MSDTCRequest.DELIST starting");

			// release pooled connection if requested
			StartMSDTCRequest(MSDTCRequest.DELIST);

			trace.write(3, title + ": Delist() complete");
			if (debugging)
				Console.WriteLine(title + ": Delist() exiting...");
			}
			catch (SqlEx ex) { throw ex.createProviderException(); }
		}  // Delist


		/*
		** Name: ITransactionResourceAsync Interface Implementation
		**
		** Description:
		**	These four methods are called by MSDTC Proxy when it wants
		**	the Resource Manager (Ingres) to start transaction work asynchronously.
		**	We must return back as quickly as possible so that MSDTC
		**	can start other branches on their transaction work.
		**		PrepareRequest()   start prepare-to-commit
		**		CommitRequest()    start commit
		**		AbortRequest()     start rollback
		**		TMDown()           notification that Transaction Manager is down
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/


		/*
		**  PrepareRequest
		**
		**  The MS DTC Proxy (MSDTCPRX.DLL) calls this method to prepare, 
		**  phase one of the two-phase commit protocol, a transaction.
		**  The resource manager needs to return from this call as
		**  soon as the transaction object starts preparing.
		**  Once the transaction object is prepared the resource
		**  manager needs to call ITransactionEnlistmentAsync::PrepareRequestDone.
		**
		**  If fSinglePhase flag is TRUE, it indicates that the RM is the
		**  only resource manager enlisted on the transaction, and therefore
		**  it has the option to perform the single phase optimization.  
		**  If the RM does choose to perform the single phase optimization,
		**  then it lets the Transaction Coordinator know of this 
		**  optimization by providing XACT_S_SINGLEPHASE flag to the
		**  ITransactionEnlistmentAsync::PrepareRequestDone.
		**
		**  MS DTC Proxy will not return to the application program 
		**  until the ITransactionEnlistmentAsync::PrepareRequestDone
		**  is issued.  Note that control returns to the application
		**  before the the COMMIT phase begins!  This is because
		**  the provider has guaranteed (by saying PrepareRequestDone)
		**  that it can commit or rollback the transaction in hot or
		**  cold recovery.  The provider will stop further database
		**  work (by doing a advanConnect.msg.LockConnection) until
		**  the transaction is committed.
		*/
		/// <summary>
		/// Start prepare-to-commit asynchronously.
		/// </summary>
		/// <param name="fRetaining">Currently always 0.</param>
		/// <param name="grfRM">Currently always 0.</param>
		/// <param name="fWantMoniker"></param>
		/// <param name="fSinglePhase"> !0 if can skip prepare and just do a
		/// commit directly.</param>
		void ITransactionResourceAsync.PrepareRequest(
			int  fRetaining,
			uint grfRM,
			int  fWantMoniker,
			int  fSinglePhase)
		{
			trace.write(3, title + ": ITransactionResourceAsync.PrepareRequest");
			if (debugging)
				Console.WriteLine(title + ": ITransactionResourceAsync.PrepareRequest called");

			if (TxState != TXSTATE.TX_ENLISTED)
			{
				StartMSDTCRequest(MSDTCRequest.PREPARE_E_UNEXPECTED);
				return;
			}

			try
			{
				StartMSDTCRequest((fSinglePhase!=0)?
					MSDTCRequest.PREPARE_SINGLE_PHASE:
					MSDTCRequest.PREPARE);
			}
			catch (Exception)
			{
				try
				{
					StartMSDTCRequest(MSDTCRequest.PREPARE_E_FAIL);
				}
				catch (Exception)
				{
				}
			}
		}


		/*
		**  CommitRequest
		**
		**  The MS DTC Proxy (MSDTCPRX.DLL) calls this method to commit, 
		**  phase two of the two-phase commit protocol, a transaction.
		**  The resource manager needs to return from this call as
		**  soon as the transaction object starts committing.
		**  Once the transaction object is prepared, the resource
		**  manager needs to call ITransactionEnlistmentAsync::CommitRequestDone.
		**
		*/
		/// <summary>
		/// Start commit asynchronously.
		/// </summary>
		/// <param name="grfRM"></param>
		/// <param name="pNewUOW"></param>
		void ITransactionResourceAsync.CommitRequest(
			uint    grfRM,
			IntPtr /*BOID*/ pNewUOW)
		{
			trace.write(3, title + ": ITransactionResourceAsync.CommitRequest");
			if (debugging)
				Console.WriteLine(title + ": ITransactionResourceAsync.CommitRequest called");

			try
			{
				StartMSDTCRequest(MSDTCRequest.COMMIT);
			}
			catch (Exception)
			{
			}
		}


		/*
		**  AbortRequest
		**
		**  The MS DTC Proxy (MSDTCPRX.DLL) calls this method to abort a transaction.
		**  The resource manager needs to return from this call as
		**  soon as the transaction object starts aborting.
		**  Once the transaction object is aborted the resource
		**  manager needs to call ITransactionEnlistmentAsync::AbortRequestDone.
		**
		**  MS DTC Proxy returns to the application program
		**  immediately after we return from this method!  
		**  Therefore, the application must call Connection.Close()
		**  after the transaction is Abort'ed to prevent new database
		**  work being added to a transaction about to be aborted.
		**
		*/
		/// <summary>
		/// Start rollback asynchronously.
		/// </summary>
		/// <param name="pboidReason"></param>
		/// <param name="fRetaining"></param>
		/// <param name="pNewUOW"></param>
		void ITransactionResourceAsync.AbortRequest(
			IntPtr /*BOID*/  pboidReason,
			int              fRetaining,
			IntPtr /*BOID*/  pNewUOW)
		{
			trace.write(3, title + ": ITransactionResourceAsync.AbortRequest");
			if (debugging)
				Console.WriteLine(title + ": ITransactionResourceAsync.AbortRequest called");

			try
			{
				StartMSDTCRequest(MSDTCRequest.ABORT);
			}
			catch (Exception)
			{
			}
		}

		/// <summary>
		/// Notification that MSDTC's Transaction Monitor went down.
		/// </summary>
		void ITransactionResourceAsync.TMDown()
		{
			trace.write(3, title + ": ITransactionResourceAsync.TMDown");
			if (debugging)
				Console.WriteLine(title + ": ITransactionResourceAsync.TMDown called");
			try
			{
				StartMSDTCRequest(MSDTCRequest.TMDOWN);
			}
			catch (Exception)
			{
			}
		}


		/*
		** Name: StartMSDTCRequest
		**
		** Description:
		**	Start the TxResourceAsync thread that will asynchronously
		**	perform the transaction work.
		**	Called by one of our implentations under MSDTC Proxy thread.
		**		PrepareRequest()   start prepare-to-commit
		**		CommitRequest()    start commit
		**		AbortRequest()     start rollback
		**		TMDown()           notification that Transaction Monitor is down
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Start a MSDTC request on a worker thread.
		/// </summary>
		/// <param name="request"></param>
		/// <returns>True if request was successfully queued.</returns>
		bool StartMSDTCRequest(MSDTCRequest request)
		{
			lock(MSDTCRequestQueue)
			{
				// first time start the worker thread
				if (txResourceAsyncThread == null  ||
					txResourceAsyncThread.IsAlive == false)
				{
					// only start the thread if doing the ENLIST
					// (which should be the first call).
					if (request != MSDTCRequest.ENLIST)
						return false;  // should not happen, but bulletproof it

					// fire up the thread
					ThreadStart start =
						new ThreadStart(TxResourceAsyncThreadMethod);
					txResourceAsyncThread = new Thread(start);
					//Set ApartmentState to STA to minimize
					//any COM object creation issues.
					txResourceAsyncThread.TrySetApartmentState(ApartmentState.STA);
					txResourceAsyncThread.Start();
				}

				// put the request on the dispatch queue
				MSDTCRequestQueue.Enqueue(request);

				// wait for TxResourceAsync thread to start and give it
				// a chance to lock DBC connection before MSDTC Proxy might
				// return async control back to application which might try to
				// fire off another provider call before COMMIT has a
				// chance to finish.

				Monitor.Pulse(MSDTCRequestQueue);
				Monitor.Wait(MSDTCRequestQueue);
			}  // end lock

			return true;  // indicate request was successfully queued
		}

		/*
		** Name: TxResourceAsyncThreadMethod
		**
		** Description:
		**	The implentation and method that runs under the
		**	TxResourceAsync thread that asynchronously
		**	performs the transaction work.  This separate thread
		**	frees the MS DTC Proxy thread to start other branches
		**	of the commit tree on their work.
		**	This thread/method processes these MSDTCRequests:
		**		PREPARE                perform prepare-to-commit
		**		PREPARE_SINGLE_PHASE   perform shortcut prepare-to-commit
		**		COMMIT                 perform commit
		**		ABORT                  perform rollback
		**		TMDOWN                 receive notification that TM is down
		**		                          (we ignore it).
		**	If TxState == TX_PREPARED  keep thread alive while we wait
		**		                       for COMMIT or ABORT to finish PREPARE
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		void TxResourceAsyncThreadMethod()
		{
			int ConnectionLockCount = 0;  // have we done a LockConnection
			bool ShutdownThread     = false;  // ok to shutdown thread?
			MSDTCRequest request;

			while(true)
			{
				// wait for more work
				lock(MSDTCRequestQueue)
				{
					while (MSDTCRequestQueue.Count == 0)
					{
						// if Delist() occurred and MSDTC finished
						if (ShutdownThread  &&  this.RMCookie==0)
						{
							txResourceAsyncThread = null;
							break;
						}
						bool timeout = !Monitor.Wait(MSDTCRequestQueue,
							(this.RMCookie==0)? 60000 : Timeout.Infinite);

						if (timeout) // RMCookie was released and timed-out
						{
							ShutdownThread = true;
							continue;
						}
					}

					if (txResourceAsyncThread == null)  // if exiting
						break;

					request = (MSDTCRequest)MSDTCRequestQueue.Dequeue();

					trace.write(3, title + ": Asynch thread processing " +
						request.ToString() + "...");
					if (debugging)
						Console.WriteLine(title + ": Asynch thread processing: " +
							request.ToString() + "...");
				}  // end lock


				// if doing database work, lock the msg before awakening caller
				if (request == MSDTCRequest.ENLIST               ||
					request == MSDTCRequest.ABORT                ||
					request == MSDTCRequest.COMMIT               ||
					request == MSDTCRequest.PREPARE              ||
					request == MSDTCRequest.PREPARE_SINGLE_PHASE ||
					request == MSDTCRequest.TMDOWN)
				{
					// Lock the DBC so asynchronous queries can't sneak in.
					// Indicate that we will have nested LockConnection's.
					// Force MSDTC Proxy to wait until we can lock msg.
					advanConnect.msg.LockConnection(true);
					ConnectionLockCount++;
				}

				// wake up caller of StartMSDTCRequest now that
				// DBC is safely locked from other threads (like the
				// main application thread) until we finish the abort/commit
				lock(MSDTCRequestQueue)
				{
					Monitor.Pulse(MSDTCRequestQueue);
				}


				TxResourceAsyncThreadProcessRequest(request);

				// if msg was locked, now is the time to release it
				while (ConnectionLockCount > 0  &&  // if PREPARE, leave msg locked
					TxState != TXSTATE.TX_PREPARED)
				{
					ConnectionLockCount--;
					advanConnect.msg.UnlockConnection();
				}

				// if delisting from the transaction then close down thread
				if (request == MSDTCRequest.DELIST)
					ShutdownThread = true;

				// if enlistment in MSDTC is severed, then close down thread
				if (this.RMCookie == 0)
					ShutdownThread = true;

			}  // end while(true)

			// make sure that advanConnect.msg is unlocked
			// (could have been a PREPARE leftover)
			while (ConnectionLockCount > 0)
			{
				ConnectionLockCount--;
				if (advanConnect != null)
					advanConnect.msg.UnlockConnection();
			}

			// put the connection back in pool or close it
			// (if the advanConnect reference count decrements to 0)
			if (advanConnect != null)
				advanConnect.CloseOrPutBackIntoConnectionPool();

		}  //  TxResourceAsyncThreadMethod


		/*
		** Name: TxResourceAsyncThreadProcessRequest
		**
		** Description:
		**	Perform the prepare-to-commit, commit, or
		**	rollback on the database.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Perform the prepare-to-commit, commit, or rollback on the database.
		/// </summary>
		void TxResourceAsyncThreadProcessRequest(MSDTCRequest request)
		{
			uint hr=S_OK;

			switch(request)
			{
				case MSDTCRequest.ENLIST:
					try
					{
						trace.write(4, title + ":    MSDTCRequest.Enlisting with RM...");
						if (debugging)
							Console.WriteLine(title + ":    MSDTCRequest.Enlisting with RM...");
						TxResourceAsync = (ITransactionResourceAsync) this;
						XaHelperSinglePipe_EnlistWithRM(
							RMCookie,
							Transaction,      // pass on the incoming ITransaction
							TxResourceAsync,  // implements ITransactionResourceAsync
							out TxEnlistmentAsync);
							                  // receive ITransactionEnlistmentAsync

						TxState = TXSTATE.TX_ENLISTED;

						trace.write(4, title + ":    MSDTCRequest.Enlisted with RM successfully");
						if (debugging)
							Console.WriteLine(title + ":    MSDTCRequest.Enlisted with RM");
					}
					catch (Exception ex)
					{
						EnlistmentException = ex;
						string s = ex.ToString();  // debugging
						// release and clear the RMCookie
						XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
						break;
					}

					break;

				case MSDTCRequest.DELIST:
					if (debugging)
						Console.WriteLine(title + ":    MSDTCRequest.DELIST...");
					// make sure the RMCookie is released and cleared
					XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
					break;

				case MSDTCRequest.PREPARE:
				{
					if (debugging)
						Console.WriteLine(title + ":    MSDTCRequest.PREPARE...");

					hr = PrepareTx();  /* call Ingres to prepare-to-commit */
					if (hr==S_OK)      /* transaction is prepared */
					{
						TxState = TXSTATE.TX_PREPARED;
					}
					else
					{
						RollbackTx(); /* prepare failed; abort the transaction */
						TxState = TXSTATE.TX_INVALID_STATE;
					}
					try
					{
						TxEnlistmentAsync.PrepareRequestDone(
							hr, IntPtr.Zero, IntPtr.Zero);
						trace.write(3, title +
							":    TxEnlistmentAsync.PrepareRequestDone(0X" +
							hr.ToString("X8") + ")");
						if (debugging)
							Console.WriteLine(title + ":    PrepareRequestDone called");
						/* if hr==S_OK then MS DTC will dispatch the user application
									after the pITransaction->commit();
						 else hr==E_FAIL then DTC will 
									pITransactionResourceAsync->Release()
						 */
						if (TxState != TXSTATE.TX_PREPARED)
						{
							XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
						}
					}
					catch (COMException ex)
					{
						string s = ex.ToString();
						trace.write(3, title + ": MSDTCRequest.PREPARE catches COMException. Message=" +
							"\"" + ex.Message +"\"");
						XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
//						throw;
					}
					catch (Exception ex)
					{
						string s = ex.ToString();
						trace.write(3, title + ": MSDTCRequest.PREPARE catches Exception. Message=" +
							"\"" + ex.Message + "\"");
						XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
//						throw;
					}
					break;
				}

				case MSDTCRequest.PREPARE_E_FAIL:
					if (debugging)
						Console.WriteLine(title + ":    MSDTCRequest.PREPARE_E_FAIL...");
					TxEnlistmentAsync.PrepareRequestDone(
						E_FAIL, IntPtr.Zero, IntPtr.Zero);
					trace.write(3, title +
						": TxEnlistmentAsync.PrepareRequestDone(E_FAIL)");
					XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
					break;

				case MSDTCRequest.PREPARE_E_UNEXPECTED:
					if (debugging)
						Console.WriteLine(title + ":    MSDTCRequest.PREPARE_E_UNEXPECTED...");
					TxEnlistmentAsync.PrepareRequestDone(
						E_UNEXPECTED, IntPtr.Zero, IntPtr.Zero);
					trace.write(3, title +
						": TxEnlistmentAsync.PrepareRequestDone(E_FAIL)");
					XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
					break;

				case MSDTCRequest.PREPARE_SINGLE_PHASE:
				{
					if (debugging)
						Console.WriteLine(title + ":    MSDTCRequest.PREPARE_SINGLE_PHASE...");

					hr = CommitTx(TMONEPHASE);          /* single phase commit optimization */
					if (hr == S_OK)
					{
						hr=XACT_S_SINGLEPHASE;
						TxState = TXSTATE.TX_ENLISTED;
					}
					else
						TxState = TXSTATE.TX_INVALID_STATE;

					try
					{
						TxEnlistmentAsync.PrepareRequestDone(
							hr, IntPtr.Zero, IntPtr.Zero);
						trace.write(4, title +
							": TxEnlistmentAsync.PrepareRequestDone(0X" +
							hr.ToString("X8") + ")");
						if (debugging)
							Console.WriteLine(title + ":       PrepareRequestDone called");
						if (hr == XACT_S_SINGLEPHASE)
						{
							if (debugging)
								Console.WriteLine(title + ":       MSDTCRequest.PREPARE_SINGLE_PHASE releasing RMCookie...");
							XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
						}

					}
					catch (Exception ex)
					{
						string s = ex.ToString();
						trace.write(3, title + ": MSDTCRequest.PREPARE_SINGLE_PHASE catches Exception. Message=" +
							"\"" + ex.Message + "\"");
						XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
//						throw;
					}
					break;
				}

				case MSDTCRequest.COMMIT:
				{
					if (debugging)
						Console.WriteLine(title + ":    MSDTCRequest.COMMIT...");

					hr = CommitTx();
					if (hr == S_OK)
						TxState = TXSTATE.TX_ENLISTED;
					else
						TxState = TXSTATE.TX_INVALID_STATE;

					try
					{
						TxEnlistmentAsync.CommitRequestDone(hr);
						trace.write(3, title +
							": TxEnlistmentAsync.CommitRequestDone(0X" +
							hr.ToString("X8") + ")");
						if (debugging)
							Console.WriteLine(title + ":       CommitRequestDone called... Releasing RMCookie...");
						XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
					}
					catch (Exception ex)
					{
						string s = ex.ToString();  // for debugging; and ign ex
						trace.write(3, title + ": MSDTCRequest.COMMIT catches Exception. Message=" +
							"\"" + ex.Message + "\"");
					}
					break;
				}

				case MSDTCRequest.ABORT:
				{
					if (debugging)
						Console.WriteLine(title + ":    MSDTCRequest.ABORT...");

					hr = RollbackTx();
					TxState = TXSTATE.TX_ABORTED;

					try
					{
						TxEnlistmentAsync.AbortRequestDone(hr);
						trace.write(3, title +
							": TxEnlistmentAsync.AbortRequestDone(0X" +
							hr.ToString("X8") + ")");
						if (debugging)
							Console.WriteLine(title + ":       AbortRequestDone called... Releasing RMCookie...");
						XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
					}
					catch (Exception ex)
					{
						string s = ex.ToString();  // for debugging; and ign ex
						trace.write(3, title + ": MSDTCRequest.ABORT catches Exception. Message=" +
							"\"" + ex.Message + "\"");
					}
					break;
				}

				case MSDTCRequest.TMDOWN:
				{
					if (debugging)
						Console.WriteLine(title + ":    MSDTCRequest.TMDOWN...");

					RollbackTx();
					TxState = TXSTATE.TX_TMDOWN;
					XaHelperSinglePipe_ReleaseRMCookie(ref this.RMCookie);
					break;
				}

				default:
					break;
			} /* end switch */

		}  // TxResourceAsyncThreadProcessRequest


		/*
		** Name: EndTx
		**
		** Description:
		**	End work performed on behalf of a transaction branch.
		**	Required before a prepare/commit.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// End work performed on behalf of a transaction branch.
		/// </summary>
		/// <param name="flags">One of the TMxxxx flags.</param>
		/// <returns></returns>
		private uint EndTx(int flags)
		{
			if (TxBranchState != TXBRANCHSTATE.Active)  // if already ended, return
				return S_OK;

			try
			{
				if (advanConnect.conn.msg_protocol_level >= MsgConst.MSG_PROTO_5)
					advanConnect.endTransaction(advanXID, flags); // Ingres 9.1.0 and later
				TxBranchState = TXBRANCHSTATE.Idle;
				return S_OK;
			}
			catch (Exception ex)
			{
				TxBranchState = TXBRANCHSTATE.RollbackOnly;
				if (debugging)
					Console.WriteLine(title + ":    EndTx catches Exception. Message=\"" +
						ex.Message + "\"");
				return E_FAIL;
			}
		}


		/*
		** Name: PrepareTx
		**
		** Description:
		**	Perform the prepare-to-commit on the database.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Perform the prepare-to-commit on the database.
		/// </summary>
		/// <returns></returns>
		private uint PrepareTx()
		{
			if (EndTx(TMNOFLAGS) != S_OK)  // make sure xa_end was issued
				return E_FAIL;

			try
			{
				if (advanConnect.conn.msg_protocol_level < MsgConst.MSG_PROTO_5)
					advanConnect.prepareTransaction();         // old DAS msg level 4
				else
					advanConnect.prepareTransaction(advanXID); // Ingres 9.1.0 and later
				TxBranchState = TXBRANCHSTATE.Prepared;
				return S_OK;
			}
			catch (Exception ex)
			{
				if (debugging)
					Console.WriteLine(title + ":    PrepareTx catches Exception. Message=\"" +
						ex.Message + "\"");
				TxBranchState = TXBRANCHSTATE.Idle;
				return E_FAIL;
			}
		}


		/*
		** Name: CommitTx
		**
		** Description:
		**	Perform the commit on the database.
		**
		** History:
		**	05-Mar-09 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Perform the commit on the database.
		/// </summary>
		/// <returns></returns>
		private uint CommitTx()
		{
			return CommitTx(TMNOFLAGS);
		}


		/*
		** Name: CommitTx
		**
		** Description:
		**	Perform the commit on the database.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Perform the commit on the database.
		/// </summary>
		/// <param name="flags"></param>
		/// <returns></returns>
		private uint CommitTx(int flags)
		{
			if (EndTx(TMNOFLAGS) != S_OK)  // make sure xa_end was issued
				return E_FAIL;

			try
			{
				if (advanConnect.conn.msg_protocol_level < MsgConst.MSG_PROTO_5)
					advanConnect.commit();         // old DAS msg level 4
				else
					advanConnect.commit(advanXID, flags); // Ingres 9.1.0 and later
				TxBranchState = TXBRANCHSTATE.NonExistent;
				return S_OK;
			}
			catch (Exception ex)
			{
				if (debugging)
					Console.WriteLine(title + ":    CommitTx catches Exception. Message=\"" +
						ex.Message + "\"");
				TxBranchState = TXBRANCHSTATE.NonExistent;
				return E_FAIL;
			}
		}


		/*
		** Name: RollbackTx
		**
		** Description:
		**	Perform the rollback on the database.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Perform the rollback on the database.
		/// </summary>
		/// <returns></returns>
		private uint RollbackTx()
		{
			if (EndTx(TMNOFLAGS) != S_OK)  // make sure xa_end was issued
				return E_FAIL;

			try
			{
				if (advanConnect.conn.msg_protocol_level < MsgConst.MSG_PROTO_5)
					advanConnect.rollback();         // old DAS msg level 4
				else
					advanConnect.rollback(advanXID); // Ingres 9.1.0 and later
				TxBranchState = TXBRANCHSTATE.NonExistent;
				return S_OK;
			}
			catch (Exception ex)
			{
				if (debugging)
					Console.WriteLine(title + ":    CommitTx catches Exception. Message=\"" +
						ex.Message + "\"");
				TxBranchState = TXBRANCHSTATE.NonExistent;
				return E_FAIL;
			}
		}





		/*
		** Name: DtcGetTransactionManager
		**
		** Description:
		**	Call DtcGetTransactionManager in MS DTC proxy
		**	to establish a connection to the Transaction Manager.
		**	Connect with the default MS DTC Transaction
		**	Manager as defined in the system registry.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		static readonly Guid CLSID_MSDtcTransactionManager = new
			Guid("5B18AB61-091D-11d1-97DF-00C04FB9618A");

		/// <summary>
		/// Call DtcGetTransactionManager in MS DTC proxy to establish a connection
		/// to the Transaction Manager.  Connect with the default MS DTC Transaction
		/// Manager as defined in the system registry.
		/// </summary>
		/// <returns>MSDTC root object.</returns>
		static private object DtcGetTransactionManager()
		{
			try
			{
				Type t = Type.GetTypeFromCLSID(CLSID_MSDtcTransactionManager);
				return Activator.CreateInstance(t);
			}
			catch (COMException comex)
			{
				throw SqlEx.get(
					"EnlistInDTC failed to locate the MS Distributed " +
					"Transaction Coordinator.  Check that MS DTC " +
					"service is started.  " + HResultToString(comex.ErrorCode) +
					"  " + comex.Message, "HY000", comex.ErrorCode );
			}
		}  // DtcGetTransactionManager


		/*
		** Name: GetDtcToXaHelperSinglePipe
		**
		** Description:
		**	Get the XATM's Single Pipe Helper interface
		**	that will help establish an enlistment in MSDTC
		**	using the XA protocol (XaSwitch and XIDs).
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Get the XATM's Single Pipe Helper interface
		/// that will help establish an enlistment in MSDTC
		/// using the XA protocol (XaSwitch and XIDs).
		/// </summary>
		/// <returns>IDtcToXaHelperSinglePipe interface.</returns>
		static private IDtcToXaHelperSinglePipe GetDtcToXaHelperSinglePipe()
		{
			Object MSDTC = DtcGetTransactionManager();
			return ((IDtcToXaHelperSinglePipe) MSDTC);
		}


		/*
		** Name: XaHelperSinglePipe_XARMCreate
		**
		** Description:
		**	Obtain the Resource Manager (RM) cookie.  The XARMCreate
		**	method is invoked once per connection.  The information provided
		**	makes it possible for the transaction manager to connect to the
		**	database to perform recovery.  The transaction manager durably records
		**	this information in it's log file.  If recovery is necessary, the 
		**	transaction manager reads the log file, reconnects to the database,
		**	and initiates XA recovery.

		**	Calling XARMCreate results in a message from the DTC proxy
		**	to the XA Transaction Manager (TM).  When the XA TM receives a
		**	message that contains the DSN and the name of the client DLL:
		**	   1) LoadLibrary the client dll (Ingres.Support.dll which will
		**	      drag in the referenced Ingres.ProviderInternals from the GAC).
		**	   2) GetProcAddress the GetXaSwitch function in the dll.
		**	   3) Calls the GetXaSwitch function to obtain the XA_Switch.
		**	   4) Calls the xa_open function to open an XA connection with the 
		**	      resource manager (Ingres).  When the our xa_open function is called,
		**	     the DTC TM passes in the DSN as the OPEN_STRING.  It then
		**	     closes the connection by calling xa_close.  (It's just testing
		**	     the connection information so that it won't be surprised
		**	     when it's in the middle of recovery later.)
		**	   5) Generate a RM GUID for the connection.
		**	   6) Write a record to the DTCXATM.LOG file indicating that a new
		**	     resource manager is being established.  The log record contains
		**	     the DSN name, the name of our RM (Ingres.Support.dll), and 
		**	     the RM GUID.  This information will be used to reconnect to
		**	     Ingres should recovery be necessary.

		**	XARMCreate method also causes the MS DTC proxy to create a resource
		**	manager object and returns an opaque pointer to that object as
		**	the RMCookie.  This RMCookie represents the connection to the TM.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Obtain the Resource Manager (RM) cookie.  The XARMCreate
		/// method is invoked once per connection.  The information provided
		/// makes it possible for the transaction manager to connect to the
		/// database to perform recovery.
		/// </summary>
		/// <param name="XAOpenInfo"></param>
		/// <param name="ClientDll"></param>
		/// <param name="RMCookie"></param>
		private void XaHelperSinglePipe_XARMCreate(
			string      XAOpenInfo,
			string      ClientDll,
			out uint    RMCookie)
		{
			try
			{
				XaHelperSinglePipe.XARMCreate(
					XAOpenInfo,
					ClientDll,
					out RMCookie);
			}
			catch (COMException comex)
			{
				throw SqlEx.get(
					"XARMCreate failed to enlist in " +
					"MS Distributed Transaction Coordinator.  " +
						HResultToString(comex.ErrorCode) +
					"  " + comex.Message, "HY000", comex.ErrorCode );
			}
		}  // XARMCreate


		/*
		** Name: XaHelperSinglePipe_ConvertTridToXID
		**
		** Description:
		**	Convert a ITransaction to an XID.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Convert a ITransaction to an XID.
		/// </summary>
		/// <param name="pITransaction"></param>
		/// <param name="dwRMCookie"></param>
		/// <param name="xid"></param>
		private void XaHelperSinglePipe_ConvertTridToXID(
			ITransaction pITransaction,
			uint         dwRMCookie,
			ref XAXIDStruct  xid)
		{
			try
			{
				XaHelperSinglePipe.ConvertTridToXID(
					pITransaction,
					dwRMCookie,
					ref xid);
			}
			catch (COMException comex)
			{
				throw SqlEx.get(
					"ConvertTridToXID failed during enlistment in " +
					"MS Distributed Transaction Coordinator.  " +
					HResultToString(comex.ErrorCode) +"  " + comex.Message,
					"HY000", comex.ErrorCode );
			}
		}


		/*
		** Name: XaHelperSinglePipe_EnlistWithRM
		**
		** Description:
		**	Tell the DTC proxy to enlist on behalf of the XA resource manager.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Tell the DTC proxy to enlist on behalf of the XA resource manager.
		/// </summary>
		/// <param name="dwRMCookie"></param>
		/// <param name="ITransaction"></param>
		/// <param name="ITransRes"></param>
		/// <param name="ITransEnlistment"></param>
		private void XaHelperSinglePipe_EnlistWithRM(
			uint                        dwRMCookie,
			ITransaction                ITransaction,
			ITransactionResourceAsync   ITransRes,
			out ITransactionEnlistmentAsync ITransEnlistment)
		{
			try
			{
				XaHelperSinglePipe.EnlistWithRM(
					dwRMCookie,
					ITransaction,
					ITransRes,
					out ITransEnlistment);
			}
			catch (COMException comex)
			{
				throw SqlEx.get(
					"EnlistWithRM failed during enlistment in " +
					"MS Distributed Transaction Coordinator.  " +
					HResultToString(comex.ErrorCode) + "  " + comex.Message,
					"HY000", comex.ErrorCode );
			}
			catch (Exception ex)
			{
				throw SqlEx.get(
					"EnlistWithRM failed during enlistment in " +
					"MS Distributed Transaction Coordinator.  " +
					ex.Message,   "HY000", 0 );
			}
		}

		/*
		** Name: XaHelperSinglePipe_ReleaseRMCookie
		**
		** Description:
		**	Release and clear the Resource Manager cookie.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Release and clear the resource manager cookie.
		/// </summary>
		/// <param name="RMCookie"></param>
		private void XaHelperSinglePipe_ReleaseRMCookie(
			ref uint  RMCookie)
		{
			XaHelperSinglePipe_ReleaseRMCookie(
				ref RMCookie, /* TRUE */ 1);
		}

		private void XaHelperSinglePipe_ReleaseRMCookie(
			ref uint  RMCookie,
			int       fNormal)
		{
			if (RMCookie == 0)
				return;
			uint _RMCookie = RMCookie;  // make a copy
			RMCookie = 0;   // clear the original before we try to release it
			trace.write(3, title + ": ReleaseRMCookie");
			if (debugging)
				Console.WriteLine(title + ": ReleaseRMCookie");
			try
			{
				if (XaHelperSinglePipe != null)
					XaHelperSinglePipe.ReleaseRMCookie(_RMCookie, fNormal);
			}
			catch { /* ignore any errors */}
		}


		/*
		** Name: HResultToString
		**
		** Description:
		**	Convert the HResult code to a user-friendly string.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/

		private const uint S_OK                    = 0x00000000;

		private const uint E_FAIL                  = 0x80004005;
		private const uint E_INVALIDARG            = 0x80070057;
		private const uint E_NOINTERFACE           = 0x80004002;
		private const uint E_OUTOFMEMORY           = 0x8007000E;
		private const uint E_UNEXPECTED            = 0x8000FFFF;
		private const uint E_POINTER               = 0x80004003;
		private const uint XACT_E_CONNECTION_DOWN  = 0x8004d01c;
		private const uint XACT_E_TMNOTAVAILABLE   = 0x8004d01b;
		private const uint XACT_E_NOTRANSACTION    = 0x8004d00e;
		private const uint XACT_E_CONNECTION_DENIED= 0x8004d01d;
		private const uint XACT_E_LOGFULL          = 0x8004d01a;
		private const uint XACT_E_XA_TX_DISABLED   = 0x8004d026;

		/// <summary>
		/// Convert the HResult code to a user-friendly string.
		/// </summary>
		/// <param name="_hr"></param>
		/// <returns></returns>
		static private string HResultToString(int _hr)
		{
			uint hr = unchecked((uint)_hr);

			System.Text.StringBuilder sb = new System.Text.StringBuilder(100);

			sb.Append("ErrorCode=");

			switch (hr)
			{
				case E_FAIL:
					sb.Append("E_FAIL (General failure). Check Windows Event Viewer.");
					break;
				case E_INVALIDARG:
					sb.Append("E_INVALIDARG (One or more of the parameters " +
						"are NULL or invalid)");
					break;
				case E_NOINTERFACE:
					sb.Append("E_NOINTERFACE (Unable to provide the requested " +
						"interface)");
					break;
				case E_OUTOFMEMORY:
					sb.Append("E_OUTOFMEMORY (Unable to allocate resources for " +
						"the object)");
					break;
				case E_UNEXPECTED:
					sb.Append("E_UNEXPECTED (An unexpected error occurred)");
					break;
				case E_POINTER:
					sb.Append("E_POINTER (Invalid handle)");
					break;
				case XACT_E_CONNECTION_DOWN:
					sb.Append("XACT_E_CONNECTION_DOWN (Lost connection with " +
						"DTC Transaction Manager)");
					break;
				case XACT_E_TMNOTAVAILABLE:
					sb.Append("XACT_E_TMNOTAVAILABLE (Unable to establish a " +
						"connection with DTC (MS DTC service probably not " +
						"started))");
					break;
				case XACT_E_NOTRANSACTION:
					sb.Append("XACT_E_NOTRANSACTION (No transaction " +
						"corresponding to ITRANSACTION pointer)");
					break;
				case XACT_E_CONNECTION_DENIED:
					sb.Append("XACT_E_CONNECTION_DENIED (Transaction manager " +
						"refused to accept a connection)");
					break;
				case XACT_E_LOGFULL:
					sb.Append("XACT_E_LOGFULL (Transaction Manager has run " +
						"out of log space)");
					break;
				case XACT_E_XA_TX_DISABLED:
					sb.Append("XACT_E_XA_TX_DISABLED (Transaction Manager has " +
						"disabled its support for XA transactions.  " +
						"See MS KB article 817066)");
					break;
				default:
					sb.Append("0x");
					sb.Append(hr.ToString("X8"));
					break;
			}
			sb.Append(".");

			return sb.ToString();
		}


		/*
		** COM Definitions
		**
		** Description:
		**	P/Invoke definitions to the COM interoperabilty that we use.
		**
		** History:
		**	01-Oct-03 (thoda04)
		**	    Created.
		*/


		[DllImport("xolehlp.dll",CallingConvention=CallingConvention.Cdecl)]
			static private extern int XoleHlp_DtcGetTransactionManager(
			[In]
			string Host,
			[In]
			string TmName,
			[In]
			Guid   riid,
			[In]
			int    dwReserved1,
			[In]
			short  wcbReserved2,
			[In]
			IntPtr pvReserved1,
			[Out]
			out IntPtr pIUknown);

		[
			ComImport,
			Guid("47ED4971-53B3-11d1-BBB9-00C04FD658F6"),
			InterfaceType(ComInterfaceType.InterfaceIsIUnknown)
		]
		interface IDtcToXaHelperSinglePipe
		{
			void XARMCreate(
				[MarshalAs(UnmanagedType.LPStr)]
					string    XAOpenInfo,
				[MarshalAs(UnmanagedType.LPStr)]
					string    ClientDll,
				[Out]
					out uint  RMCookie);

			void ConvertTridToXID( 
				[In]
					ITransaction pITransaction,
				[In]
					uint         dwRMCookie,
				[Out][In]
					ref XAXIDStruct  xid);

			void EnlistWithRM( 
				[In]
					uint                        dwRMCookie,
				[In]
					ITransaction                ITransaction,
				[In]
					ITransactionResourceAsync   ITransRes,
//				[Out, MarshalAs(UnmanagedType.IUnknown)]
//					out object ITransEnlistment);
				[Out, MarshalAs(UnmanagedType.Interface)]
				out ITransactionEnlistmentAsync ITransEnlistment);

			[PreserveSig]
			void ReleaseRMCookie( 
				uint    RMCookie,
				int     fNormal);
		}  // interface IDtcToXaHelperSinglePipe


		[
		ComImport,
		Guid("0fb15081-af41-11ce-bd2b-204c4f4f5020"),
		InterfaceType(ComInterfaceType.InterfaceIsIUnknown)
		]
		interface ITransactionEnlistmentAsync
		{
		void PrepareRequestDone(
			uint     hr,
			IntPtr   moniker,
			IntPtr   boidReason);

		void CommitRequestDone(
			uint     hr);

		void AbortRequestDone(
			uint     hr);
		}  // interface ITransactionEnlistmentAsync

	}  // class DTCEnlistment


	[
	ComImport,
	Guid("69E971F0-23CE-11cf-AD60-00AA00A74CCD"),
	InterfaceType(ComInterfaceType.InterfaceIsIUnknown)
	]
	interface ITransactionResourceAsync
	{
		void PrepareRequest(
			int  fRetaining,    // should always be false
			uint grfRM,
			int  fWantMoniker,  // should always be false
			int  fSinglePhase);

		void CommitRequest(
			uint    grfRM,
			IntPtr /*BOID*/ pNewUOW);      // should always be null
			                               // MS does not marshal BOID correctly

		void AbortRequest(
			IntPtr /*BOID*/  pboidReason,
			int   fRetaining,               // should always be false
			IntPtr /*BOID*/  pNewUOW);      // should always be null
			                                // MS does not marshal BOID correctly

		void TMDown();
	}  // interface ITransactionResourceAsync

}  // namespace
