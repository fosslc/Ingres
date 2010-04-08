/*
** Copyright (c) 2004, 2007 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Data;
using System.Collections;
using System.Globalization;
using System.Threading;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: advanconnectpool.cs
	**
	** Description:
	**	Base classes for connection pool manager.
	**
	**  Classes
	**
	**	AdvanConnectionPoolManager  Manages the pool of connection pools.
	**	AdvanConnectionPool         Represents one connection pool.
	**
	** History:
	**	18-Aug-03 (thoda04)
	**	    Created.
	**	25-Feb-04 (thoda04)
	**	    Convert SqlEx exception to an IngresException.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	17-sep-07 (thoda04) Bug 119139
	**	    Meaning of connection string "Pooling=true/false" was reversed.
	**	27-sep-07 (thoda04) Bug 119188
	**	    Test for severed socket into and out of connection pool.
*/


	/*
	** Class Name:   AdvanConnectionPoolManager
	**
	** Description:
	**	Connection pool manager class for AdvanConnect connections.
	**
	**  Static Data:
	**
	**	poolOfConnectionPools        HashTable collection that is the pool of
	**	                             connection pools.  It is also the lock to
	**	                             to control update of this list.  Do not
	**	                             hold the lock for long (i.e. no DB access).
	**	                             Each pool is indexed by its connection
	**	                             string values.
	**	connectionPoolWorkerThread   Thread that wakes every so often and
	**	                             closes inactive connections in the pools.
	**	                             Also releases empty pools.
	**	shuttingDownEvent            Shutting down event signaled on by
	**	                             AppDomain.DomainUnload or ProcessExit.
	**	                             Signals the connectionPoolWorkerThread
	**	                             to close all inactive connections in the
	**	                             pool and to terminate itself.
	**
	**  Internal Methods:
	**
	**	ConnectionPoolWorkerThread   Method underlying connectionPoolWorkerThread.
	**	CreateConnectionPoolWorkerThread  Create the connection pool background
	**	                                  thread.
	**	Get                          Find the correct connection pool based on
	**	                             connection string.  Get an AdvanConnect
	**	                             connection from the pool or create
	**	                             the AdvanConnect if none is available.
	**	                             Wait on pool if connection limit on pool
	**	                             would be exceeded.
	**	Put                          Put the AdvanConnect connection back into
	**	                             the connection pool to make it available
	**	                             to others.
	**	ListenForAppDomainUnload     Add EventHandlers to AppDomain to listen for
	**	                             DomainUnload or ProcessExit events at
	**	                             domain shutdown.
	**	AppDomainUnloading           Process the DomainUnload or ProcessExit
	**	                             events as a shutdown.  Signal
	**	                             connectionPoolWorkerThread to close off
	**	                             the connections in the pool.  Wait for
	**	                             connectionPoolWorkerThread to finish
	**	                             its work (up to a point).
	**
	*/

	/// <summary>
	/// Connection pool manager class for AdvanConnect connections.
	/// </summary>
	internal class AdvanConnectionPoolManager
	{
		static private Hashtable poolOfConnectionPools = new Hashtable();
		static private Thread    connectionPoolWorkerThread;
		static private ManualResetEvent
		                         shuttingDownEvent = new ManualResetEvent(false);

		/// <summary>
		/// Find the correct connection pool based on connection string.
		/// Get an AdvanConnect connection from the pool or create
		/// the AdvanConnect if none is available.  Wait on pool if
		/// connection limit on pool would be exceeded.
		/// </summary>
		/// <param name="connectionString"></param>
		/// <param name="providerConnection"></param>
		/// <param name="host"></param>
		/// <param name="config"></param>
		/// <param name="trace"></param>
		/// <param name="timeout"></param>
		/// <returns></returns>
		static internal AdvanConnect Get(
			string connectionString,
			IDbConnection providerConnection,
			String host,
			IConfig config,
			ITrace trace,
			int timeout)
		{
			try
			{
				return GetConnectionFromPool(
					connectionString,
					providerConnection,
					host,
					config,
					trace,
					timeout);
			}
			catch (SqlEx ex)  { throw ex.createProviderException(); }
		}


		/// <summary>
		/// Find the correct connection pool based on connection string.
		/// Get an AdvanConnect connection from the pool or create
		/// the AdvanConnect if none is available.  Wait on pool if
		/// connection limit on pool would be exceeded.
		/// </summary>
		/// <param name="connectionString"></param>
		/// <param name="providerConnection"></param>
		/// <param name="host"></param>
		/// <param name="config"></param>
		/// <param name="trace"></param>
		/// <param name="timeout"></param>
		/// <returns></returns>
		static private AdvanConnect GetConnectionFromPool(
			string connectionString,
			IDbConnection providerConnection,
			String host,
			IConfig config,
			ITrace trace,
			int timeout)
		{
			string   strValue;
			AdvanConnect advanConnect;
			ConnectionPool pool;
			int minPoolSize =   0;
			int maxPoolSize = 100;
			int expirationSeconds = 60;  // inactive connection expiration
			                              // timespan before removal from pool

			if ( trace == null )
				trace = new TraceDV( DrvConst.DRV_TRACE_ID ); // "drv"

			// if 'Pooling=false' was specified, get a new connection
			string pooling = config.get(DrvConst.DRV_PROP_CLIENTPOOL);
			if (pooling != null  &&
				(pooling.ToLower(CultureInfo.InvariantCulture) == "false"  ||
				 pooling.ToLower(CultureInfo.InvariantCulture) == "no"))
			{
				advanConnect = new AdvanConnect(
					providerConnection, host, config, trace, timeout);
				// leave advanConnect.CanBePooled == false so that
				// it does not go back into the pool and is physically closed later.
				return advanConnect;
			}

			// Min Pool Size
			strValue = config.get(DrvConst.DRV_PROP_MINPOOLSIZE);
			if (strValue != null)
			{
				try
				{minPoolSize=Int32.Parse(strValue);}
				catch
				{}
			}

			// Max Pool Size
			strValue = config.get(DrvConst.DRV_PROP_MAXPOOLSIZE);
			if (strValue != null)
			{
				try
				{maxPoolSize=Int32.Parse(strValue);}
				catch
				{}
			}

			while (true)  // retry loop to get a pooled connection
			{
				// Get a pooled connection.
				// Wait on the lock for the pool of pools since
				// the ConnectionPoolWorkerThread might have it and
				// is busy cleaning up inactive pools.
				// This lock controls update of this list.  Do not
				// hold the lock for long (i.e. no DB access).
				lock (poolOfConnectionPools)  // lock pool of pools and possibly wait
				{
					// if first time, start the worker thread that cleans up connections
					if (connectionPoolWorkerThread == null)
					{
						// create thread and fill in connectionPoolWorkerThread
						CreateConnectionPoolWorkerThread();

						// listen for AppDomain.DomainUnload event so we can
						// tell the ConnectionPoolWorkerThread to gracefully
						// shut down inactive connections in the pools.
						ListenForAppDomainUnload();
					}

					// get a pooled connection for given connection string
					pool = (ConnectionPool)poolOfConnectionPools[connectionString];

					// if no pool for given connection string, build one
					if (pool == null)
					{
						pool = new ConnectionPool(
							providerConnection.ConnectionTimeout,
							minPoolSize,
							maxPoolSize,
							expirationSeconds);
						poolOfConnectionPools[connectionString] = pool;  // insert
					} // end if (pool == null)

					pool.PrepareToGetPoolSlot();  // mark pool for "connect-in-progress"
				}  // end lock(poolOfConnectionPools)

				// Wait for and lock up a slot in the connection pool
				// and release the "connect-in-progress" indicator in the pool.
				// The pool.ConnectingCount will be > 0 and will prevent
				// the pool from being released while we wait for a slot.
				// We hold no locks on pool of pools nor individual pool while we wait.
				advanConnect = pool.GetPoolSlot(providerConnection);

				// if a physical connection was found in the pool then we're all done!
				if (advanConnect != null)
				{
					if (advanConnect.msg.QuerySocketHealth() == true) // if still alive
						return advanConnect;
					advanConnect.close();     // close the bad connection
					continue;                  // retry to get a pooled connection
				}
				else break;  // no more available connections in pool
			}   // retry loop to get a pooled connection


			// we own a slot in the pool
			// but we need to build a brand new connection
			try
			{
				// connect to the server
				advanConnect = new AdvanConnect(
					providerConnection, host, config, trace, timeout);

				// at this point, we have a good connection
				advanConnect.connectionPool = pool;
				advanConnect.CanBePooled    = true;  // we can pool good ones
			}
			catch(Exception)
			{
				pool.PutPoolSlot();  // release our slot back into the pool
				throw;
			}
			return advanConnect;
		}


		/// <summary>
		/// Put the AdvanConnect connection back into the connection pool
		/// to make it available to others.
		/// </summary>
		/// <param name="advanConnect"></param>
		static internal void Put( AdvanConnect advanConnect )
		{
			if (advanConnect == null)         // safety check
				return;

			ConnectionPool pool = advanConnect.connectionPool;

			if (advanConnect.msg != null &&
				advanConnect.msg.isSocketConnected() == false)
			{
				// underlying connection socket to msg is damaged
				advanConnect.CanBePooled = false;
			}

			// if cannot be pooled
			if (advanConnect.CanBePooled == false  ||  pool == null)
			{
				// Null out connectionPool reference to avoid the close()
				// method trying to remove the connection pool slot.
				// We will do it ourselves.
				advanConnect.connectionPool = null;
//				Console.WriteLine("Closing connection physically by Put");
				advanConnect.close();  // close the internal connection

				// if connection was in the pool in the past but is now damaged
				if (pool != null)
				{
					pool.PutPoolSlot(null);  // release the pool slot
				}
			}
			else
			{
				pool.PutPoolSlot(advanConnect);  // put good conn back into pool
			}
			return;
		}

		/// <summary>
		/// Create a worker thread that cleans up timed-out connections.
		/// Field 'poolOfConnectionPools' must be locked to avoid sync issues.
		/// </summary>
		static internal void CreateConnectionPoolWorkerThread()
		{
			ThreadStart start =  // create the  thread delegate
				new ThreadStart(ConnectionPoolWorkerThread);
			connectionPoolWorkerThread = new Thread(start);
			// Background means that CLR will not wait for this
			// thread to terminate before shutting down.  This
			// connectionPoolWorkerThread must be background
			// so that CLR will go ahead and start the shutdown,
			// so that we receive DomainUnload or ProcessExit
			// notification, so that we can shut down this
			// connectionPoolWorkerThread at the end.
			connectionPoolWorkerThread.IsBackground = true;
			connectionPoolWorkerThread.Start();
		}

		/// <summary>
		/// Worker thread that cleans up timed-out connections.
		/// </summary>
		static internal void ConnectionPoolWorkerThread()
		{
			bool      shuttingDown = false;

//			Console.WriteLine("ConnectionPoolWorkerThread started");
			while (true)
			{
				bool      didWork      = false;
				DateTime  now          = DateTime.Now;
				ArrayList pools        = null;

				// Wait on the lock for the pool of pools since
				// the ConnectionPoolManager.Get might have it and
				// is declaring its intent to get a slot from one of the pools.
				lock(poolOfConnectionPools)
				{
					pools = new ArrayList(poolOfConnectionPools.Keys);
					foreach(object obj in pools)
					{
						string key = (string)obj;  // connection string
						ConnectionPool pool = 
							(ConnectionPool)poolOfConnectionPools[key];
						// if no active slots and no connecting
						// and no inactive connections and too old
						if (pool.IsTooOld(now))
							  // disconnect old, empty pool from the pool of pools
							poolOfConnectionPools.Remove(key);
					}

					// Get a snapshot list of pools.  Other threads might
					// be adding more pools to the pool of pools but we
					// don't care; we'll catch on the next timer iteration.
					// We are the only thread that removes pools so we
					// the snapahot will be good for this iteration.
					pools = new ArrayList(poolOfConnectionPools.Values);

					// release our lock on the pool of pools
				}  // end lock(poolOfConnectionPools)

				foreach(object obj in pools)
				{
					ConnectionPool pool = (ConnectionPool)obj;
					// We now have a pool that we can check for inactive
					// physical connections that we can close physically.

					while(true)  // loop thru the connections in the pool
					{
						AdvanConnect advanConnect = null;

						advanConnect = pool.GetPoolSlotExpired(shuttingDown);
						if (advanConnect == null)
							break;

						try
						{
//							Console.WriteLine("Closing connection physically"+
//								" by ConnectionPoolWorkerThread");
							// Close the connection.  Do not hold any
							// locks for the pool of pools or a pool since
							// the I/O for the close may take a long time.
							advanConnect.close();  // close the connection
						}
						catch
						{}
						didWork = true;
					}

				}

				if (didWork)   // if we did something then restart the scan
					continue;

				pools = null;  // release the object to garbage collection

				if (shuttingDown)
				{
//					Console.WriteLine("ConnectionPoolWorkerThread returning");
					return;
				}

				// sleep 10 seconds or until shutdown signaled
				shuttingDown = shuttingDownEvent.WaitOne(10000, false);
//				if (shuttingDown)
//					Console.WriteLine("ConnectionPoolWorkerThread ShuttingDown");

				//Thread.Sleep(10000);  // sleep 10 seconds
			}  // end while
		}

		/// <summary>
		/// Add EventHandlers to AppDomain to listen for
		/// DomainUnload or ProcessExit events at domain shutdown.
		/// </summary>
		static internal void ListenForAppDomainUnload()
		{
//			System.Diagnostics.Debug.WriteLine("ListenForAppDomainUnload");
//			Console.WriteLine("ListenForAppDomainUnload");
			AppDomain appDomain = AppDomain.CurrentDomain;

			// DomainUnload event is fired just before the AppDomain
			// is unloading.  It is not fired if the process that
			// contains the AppDomain is terminating.
			appDomain.DomainUnload += new EventHandler(AppDomainUnloading);

			// ProcessExit event is fired just before the process terminates.
			// It is fired only for the default AppDomain.
			appDomain.ProcessExit += new EventHandler(AppDomainUnloading);
		}


		/// <summary>
		/// Process the DomainUnload or ProcessExit events as a shutdown.
		/// Signal connectionPoolWorkerThread to close off the connections
		/// in the pool.  Wait for connectionPoolWorkerThread to finish its
		/// work (up to a point).
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		static private void AppDomainUnloading(object sender, System.EventArgs e)
		{
			try
			{

//				System.Diagnostics.Debug.WriteLine("appDomainUnloading");
//				Console.WriteLine("appDomainUnloading");

				if (connectionPoolWorkerThread == null  ||
					(connectionPoolWorkerThread.ThreadState &
						(ThreadState.Stopped | ThreadState.Unstarted)) != 0)
					return;

//				Console.WriteLine("appDomainUnloading shuttingDown");
				// signal connectionPoolWorkerThread to wake up and close all
				shuttingDownEvent.Set();

				// wake up the connection pool clean up for the last time
				connectionPoolWorkerThread.Join(30000);

//				Console.WriteLine("appDomainUnloading exiting");
			}
			catch {}
		}


		/*
		** Name: ReleasePool
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
		public static void ReleasePool()
		{
			// this method is a nop for now.
		}




	}  // end class AdvanConnectionPoolManager



	/*
	** Class Name:   ConnectionPool
	**
	** Description:
	**	Represents the pool of AdvanConnect connections that can be drawn from.
	**
	**  Internal Methods:
	**
	**	PrepareToGetPoolSlot         Mark the pool as having a slot allocation
	**	                             in progress.  This will prevent the pool
	**	                             from going away during the middle of the
	**	                             connection process.
	**	GetPoolSlot                  Get a slot and maybe a real AdvanConnect
	**	                             connection from the pool.  Returns
	**	                             AdvanConnect connection if availablein
	**	                             the pool; else null.
	**	GetPoolSlotExpired           Get the oldest expired AdvanConnect
	**	                             connection from the pool.
	**	PutPoolSlot                  Release a connection slot, and possibly
	**	                             a good AdvanConnect object back into the pool.
	**	IsTooOld                     Return true if pool is empty and inactive. 
	**	                             lock(poolOfConnectionPools) must be set
	**	                             before calling this method.
	**	AppDomainUnloading           Process the DomainUnload or ProcessExit
	**	                             events as a shutdown.  Signal
	**	                             connectionPoolWorkerThread to close off
	**	                             the connections in the pool.  Wait for
	**	                             connectionPoolWorkerThread to finish
	**	                             its work (up to a point).
	**
	*/

	/// <summary>
	/// Represents the pool of connections that can be drawn from.
	/// </summary>
	internal class ConnectionPool
	{
		/// <summary>
		/// List of AdvanConnect connections that can be reused.
		/// </summary>
		private ArrayList  AvailableList = new ArrayList();
		/// <summary>
		/// Count of active connection slots in pool.
		/// </summary>
		private int ActiveCount;
		/// <summary>
		/// Count of threads in the process of connecting to pool.
		/// </summary>
		private int ConnectingCount;
		/// <summary>
		/// Count of minimum connections in pool.
		/// </summary>
		private  int MinCount;
		/// <summary>
		/// Count of maximum connections in pool.
		/// </summary>
		private  int MaxCount;
		/// <summary>
		/// Connection timeout in milliseconds.
		/// </summary>
		private  int Timeout;
		/// <summary>
		/// Inactive connection expiration timespan.
		/// </summary>
		private TimeSpan ConnectExpirationTimeSpan;
		/// <summary>
		/// Inactive pool expiration datetime.
		/// </summary>
		internal DateTime PoolExpirationDate = DateTime.MaxValue;
		/// <summary>
		/// Event of wait on if waiting on the pool for a slot
		/// counted against ActiveCount less than MaxCount.
		/// </summary>
		private AutoResetEvent waitingOnPoolSize = new AutoResetEvent(false);


		/// <summary>
		/// Constructor for ConnectionPool.
		/// </summary>
		/// <param name="_timeout">Connection timeout in seconds.</param>
		/// <param name="_minCount">Count of minimum connections in pool.</param>
		/// <param name="_maxCount">Count of maximum connections in pool.</param>
		/// <param name="_expirationSeconds">Connection expiration from pool
		/// in seconds.</param>
		internal ConnectionPool(
			int _timeout, int _minCount, int _maxCount, int _expirationSeconds)
		{
			Timeout = _timeout * 1000;  // connection timeout in milliseconds
			MinCount= _minCount;        // count of minimum connections in pool
			MaxCount= _maxCount;        // count of maximum connections in pool
			if (MaxCount == 0)         // treat 0 as unlimited max connections
				MaxCount = int.MaxValue;
			try
			{
				ConnectExpirationTimeSpan = TimeSpan.FromTicks(
					_expirationSeconds * TimeSpan.TicksPerSecond);
			}
			catch
			{
				ConnectExpirationTimeSpan = TimeSpan.MaxValue;
			}

		}


		/// <summary>
		/// Mark the pool as having a slot allocation in progress.
		/// This will prevent the pool from going away during the 
		/// middle of the connection process.
		/// </summary>
		internal void PrepareToGetPoolSlot()
		{
			Interlocked.Increment(ref ConnectingCount);
		}

		/// <summary>
		/// Get a slot and maybe a real AdvanConnect connection from the pool.
		/// </summary>
		/// <param name="providerConnection">IngresConnection
		/// to associate with the AdvanConnect.</param>
		/// <returns>AdvanConnect connection if available; else null.</returns>
		internal AdvanConnect GetPoolSlot(IDbConnection providerConnection)
		{
			AdvanConnect advanConnect;

			while(true)
			{
				lock(this)
				{
					if (ActiveCount < MaxCount) // check count of connection slots
					{
						ActiveCount++;   // count of active connection slots
						Interlocked.Decrement(ref ConnectingCount);
							// no longer connecting

						int availableListCount = AvailableList.Count;
						if (availableListCount > 0) // if there a reusable one
						{
							int index = availableListCount-1;
							advanConnect = (AdvanConnect)AvailableList[index];
							AvailableList.RemoveAt(index);  // no longer available
							advanConnect.Connection = providerConnection;
							advanConnect.connectionPool = this;
							return advanConnect;
						}  // end if (count > 0)
						return null;
					}
					else  // pool slots are full, make'em wait
						waitingOnPoolSize.Reset();  // event is nonsignaled
				}  // end lock(this)

				waitingOnPoolSize.WaitOne(Timeout, false);  // wait for avail slot
			}  // end while(true)
		}  // end GetPoolSlot


		/// <summary>
		/// Get the oldest expired AdvanConnect connection from the pool.
		/// </summary>
		/// <param name="shuttingDown">If true then return all in avail list.  
		/// Otherwise, just return the "too old" connections.</param>
		/// <returns></returns>
		internal AdvanConnect GetPoolSlotExpired(bool shuttingDown)
		{
			lock(this)
			{
				// if no inactive physical connections then return
				if (AvailableList.Count == 0)
					return null;

				// if physical connections at or below min then return
				if (ActiveCount + AvailableList.Count <=  MinCount  &&
					!shuttingDown)
					return null;

				DateTime now = DateTime.Now;

				AdvanConnect advanConnect = // get first (and oldest in list)
					(AdvanConnect)AvailableList[0];

				if (advanConnect.expirationDate > now  &&  // if still good
					!shuttingDown)
					return null;

				// remove the old, expired connection from the pool
				AvailableList.RemoveAt(0);
				advanConnect.connectionPool = null;

				// if no one is using the pool, start the death knell on it
				if (ActiveCount         == 0  &&
					AvailableList.Count == 0  &&
					ConnectingCount     == 0)
				{
					PoolExpirationDate = now + TimeSpan.FromTicks(
						60 * TimeSpan.TicksPerSecond);  // gone in 60 seconds
				}

				return advanConnect;  // return the oldest, expired AdvanConnect
			}  // end lock(pool)

		}  // end GetPoolSlotExpired()


		/// <summary>
		/// Release a connection slot back into the pool.
		/// </summary>
		internal void PutPoolSlot()
		{
			PutPoolSlot(null);
		}

		/// <summary>
		/// Release a connection slot, and possibly
		/// a good AdvanConnect object back into the pool.
		/// </summary>
		/// <param name="advanConnect"></param>
		internal void PutPoolSlot(AdvanConnect advanConnect)
		{
			lock(this)  // lock the pool as we update the list
			{
				ActiveCount--;   // count of active connection slots
				if (ActiveCount < MaxCount)
					waitingOnPoolSize.Set();  // pool is signaled available

				if (advanConnect == null)  // if just releasing our slot
					return;

				DateTime expire;
				if (ConnectExpirationTimeSpan == TimeSpan.MaxValue)
					expire = DateTime.MaxValue;  // never expire the connection
				else
					try
					{
						// Expire inactive phys connection after specified interval.
						// If connection comes off the available list, is used,
						// and is put back on the available list then
						// this code here will be called again to push
						// the expiration time into the future.
						expire = DateTime.Now + ConnectExpirationTimeSpan;
					}
					catch(ArgumentOutOfRangeException) // catch any overflow
					{
						expire = DateTime.MaxValue;  // never expire the connection
					}
				advanConnect.expirationDate = expire;
					// inactive connection expiration time

				advanConnect.Connection = null;  // release the IngresConnection obj
				advanConnect.connectionPool = null;

				AvailableList.Add(advanConnect);  // add connection back into avail
			}  // end lock(this)
		}  // end PutPoolSlot

		// 				lock(poolOfConnectionPools)

		/// <summary>
		/// Return true if pool is empty and inactive. 
		/// lock(poolOfConnectionPools) must be set before calling
		/// this method.
		/// </summary>
		/// <returns> Returns true if pool is empty and inactive.</returns>
		internal bool IsTooOld(DateTime datetime)
		{
			if (ActiveCount         == 0  &&    // if no active slots
				ConnectingCount     == 0  &&    // and no connecting-in-progress
				AvailableList.Count == 0  &&    // and no inactive conn
				PoolExpirationDate  <= datetime)// and too old
				return true;

			return false;
		}
	}  // end Pool


}
