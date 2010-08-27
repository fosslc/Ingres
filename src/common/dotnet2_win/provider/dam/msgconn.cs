/*
** Copyright (c) 1999, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: msgconn.cs
	**
	** Description:
	**	Defines class representing a connection to a Data Access Server.
	**
	**	The Data Access Messaging (DAM) protocol classes present 
	**	a single unified interface, through inheritance, for access 
	**	to a Data Access Server.  They are divided into separate 
	**	classes to group related functionality for easier management.  
	**	The order of inheritance is determined by the initial actions
	**	required during initialization of the connection:
	**
	**	    MsgIo	Establish socket connection.
	**	    MsgOut	Send TL Connection Request packet.
	**	    MsgIn	Receive TL Connection Confirm packet.
	**	    MsgConn	General initialization.
	**
	**  Classes:
	**
	**	MsgConn		Extends MsgIn.
	**
	** History:
	**	 7-Jun-99 (gordy)
	**	    Created.
	**	10-Sep-99 (gordy)
	**	    Parameterized the initial TL connection data.
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	22-Sep-99 (gordy)
	**	    Added character set/encoding.
	**	29-Sep-99 (gordy)
	**	    Added methods (lock()/unlock()) and fields to provide
	**	    synchronization.
	**	 2-Nov-99 (gordy)
	**	    Added methods setDbmsInfo() and getDbmsInfo() to DbConn.
	**	 4-Nov-99 (gordy)
	**	    Added ability to cancel a query.
	**	12-Nov-99 (gordy)
	**	    Allow gmt or local timezone to support gateways.
	**	17-Nov-99 (gordy)
	**	    Extracted I/O functionality to DbConnIo.cs, DbConnOut.cs
	**	    and DbConnIn.cs.
	**	 4-Feb-00 (gordy)
	**	    A single thread can dead-lock if it starts another operation
	**	    while it already holds the lock.  Save the lock owner and
	**	    throw an exception in this case.
	**	21-Apr-00 (gordy)
	**	    Moved to package io.
	**	19-May-00 (gordy)
	**	    Added public field for select_loops and make additional
	**	    validation check during unlock().
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
	**	10-May-01 (gordy)
	**	    Added support for UCS2 datatypes.
	**	20-Aug-01 (gordy)
	**	    Added support for default cursor mode.
	**	20-Feb-02 (gordy)
	**	    Added fields for DBMS protocol level and Ingres/gateway distinction.
	**	20-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	 6-Sep-02 (gordy)
	**	    Character encoding now encapsulated in CharSet class.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 6-jul-04 (gordy; ported by thoda04)
	**	    Added option key mask to encoding routines.
	**	 1-Mar-07 (gordy, ported by thoda04)
	**	    Dropped char_set parameter, don't default CharSet.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Adding support for multiple DAS targets.
	**	 5-May-09 (gordy, ported by thoda04)
	**	    Support multiple host/port list targets.
	**	    Added Target structure and TargetList class.
	**	 4-Aug-09 (thoda04) Bug 122412 fix
	**	    Added getTargetListString() to support "(local)" again.
	**	13-Aug-10 (thoda04) Bug 124123
	**	    Allow one thread to unlock a connection that was locked 
	**	    by another thread.  E.g. a DataReader created by one thread
	**	    and processed/closed by another thread.
	*/


	/*
	** Name: MsgConn
	**
	** Description:
	**	Class representing a Data Access Server connection using
	**	the Data Access Messaging protocol (DAM).  
	**
	**	When reading/writing messages, the caller must provide
	**	multi-threaded protection for each message.  Since the 
	**	DAM communication channel is not multi-session, the 
	**	caller should provide multi-threaded protection from the 
	**	first message sent in a request until the last response 
	**	message has been received.  The methods lock() and unlock()
	**	permit the caller to synchronize multi-threaded access
	**	to the communication channel.
	**
	**  Public Data:
	**
	**	CRSR_DBMS    	    Cursor mode determined by DBMS.
	**	CRSR_READONLY	    Readonly cursor mode.
	**	CRSR_UPDATE  	    Updatable cursor mode.
	**
	**	msg_protocol_level	Negotiated message protocol level.
	**	use_gmt_tz        	Is connection using GMT timezone?
	**	ucs2_supported    	Is UCS2 data supported?
	**	select_loops      	Are select loops enabled?
	**	cursor_mode       	Default cursor mode.
	**
	**  Public Methods:
	**
	**	close      	Close the connection.
	**	cancel     	Cancel current query.
	**	LockConnection  	Lock connection.
	**	UnLockConnection	Unlock connection.
	**	encode     	Encrypt a string using a key.
	**	endXact    	Declare end of transaction.
	**	getUniqueID	Return a new unique identifier.
	**	getStmtID  	Return a statement ID for a query.
	**	setDbmsInfo	Save DBMS information.
	**	getDbmsInfo	Retrieve DBMS information.
	**
	**  Private Data:
	**
	**	cncl       	Cancel output buffer.
	**	dbmsInfo   	DBMS information keys and values.
	**	stmtID     	Statement ID cache.
	**	tran_id    	Transaction ID.
	**	obj_id     	Object ID.
	**	lock_obj   	Object used to lock() and unlock().
	**	lock_active	Connection is locked.
	**	lock_thread_hashcode	Lock owner.
	**
	**	KS         	Static encryption buffers.
	**	kbuff
	**	rand
	**
	**  Private Methods:
	**
	**	disconnect	Disconnect from the server.
	**	getHost		Extract host name from connection target.
	**	getPort		Extract port from connection target.
	**	parseList	Parses a list delimited by a caller supplied character.
	**	randomSort	Randomly order the entries in an array.
	**
	** History:
	**	 7-Jun-99 (gordy)
	**	    Created.
	**	10-Sep-99 (gordy)
	**	    Parameterized the initial TL connection data.
	**	29-Sep-99 (gordy)
	**	    Added methods (lock()/unlock()) and fields to provide
	**	    synchronization.
	**	 2-Nov-99 (gordy)
	**	    Added methods setDbmsInfo() and getDbmsInfo().
	**	 4-Nov-99 (gordy)
	**	    Added cancel() method and cncl output buffer.
	**	12-Nov-99 (gordy)
	**	    Added methods to permit configuration of timezone.
	**	 4-Feb-00 (gordy)
	**	    Added a thread reference for owner fo the current lock.
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
	**	10-May-01 (gordy)
	**	    Added public flag, ucs2_supported, for UCS2 support.
	**	20-Aug-01 (gordy)
	**	    Added default for connection cursors, cursor_mode, and
	**	    related constants: CRSR_DBMS, CRSR_READONLY, CRSR_UPDATE.
	**	20-Feb-02 (gordy)
	**	    Added db_protocol_level and is_ingres to handle differences
	**	    in DBMS protocol levels and gateways.
	**	20-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	 6-jul-04 (gordy; ported by thoda04)
	**	    Seed the random number generator.
	*/

	internal class MsgConn : MsgIn
	{
		/*
		** Output buffer used to cancel the current query (interrupt the
		** server).  The connection may be locked for the current query,
		** so synchronization of the output buffer cannot be done using
		** that mechanism.  Using a separate buffer avoids potential
		** conflicts with operations in the regular message buffer.
		*/
		private OutBuff	    cncl = null;

		/*
		** The connection must be locked from the start
		** of a message until the last of the response.
		** BLOB handling causes any synchronized method
		** or block to terminate prematurely, so methods
		** to lock and unlock the connection are provided.
		** A lock object is used for synchronization and
		** a boolean defines the state of the lock.
		*/
		private bool                    lock_active = false;
		private int                     lock_thread_id = 0;
		private System.Object           lock_obj = new System.Object();
		private int                     lock_nest_count;
		private bool                    lock_nesting_allowed;
		
		/*
		** Buffers used by encode() (require synchronization).
		*/
		private static byte[] KS    = new byte[CI.KS_SIZE];
		private static byte[] kbuff = new byte[CI.CRYPT_SIZE];
		private static System.Random rand =  // seed the random num generator
			new System.Random( unchecked((int)DateTime.Now.Ticks) );


		/*
		** Name: MsgConn
		**
		** Description:
		**	Class constructor.  Opens a socket connection to target
		**	host and initializes the I/O buffers.  Sends the DAM-ML
		**	connection parameters and returns the response.
		**
		**	The DAM-ML connection parameters are formatted as an
		**	array of bytes.  The byte array reference is provided as
		**	the first entry in a multi-dimensional array parameter, 
		**	msg_cp.  The input array reference is sent to the 
		**	server and the response is placed in a new byte array 
		**	whose reference is returned in place of the input
		**	reference, msg_cp.
		**
		** Input:
		**	targetList	Hostname/port target list.
		**	msg_cp  	Message layer parameter info.
		**
		** Output:
		**	msg_cp  	DAM-ML connection parameter response.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		**	10-Sep-99 (gordy)
		**	    Parameterized the initial TL connection data.
		**	17-Nov-99 (gordy)
		**	    Extracted I/O functionality to DbConnIo, DbConnOut, DbConnIn.
		**	 1-Mar-07 (gordy, ported by thoda04)
		**	    Dropped char_set parameter, don't default CharSet.
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Separated connection logic from super-class constructors
		**	    to facilitate multiple server targets.
		**	 5-May-09 (gordy, ported by thoda04)
		**	    Multiple host/port target list is parsed into host/port pairs.
		*/

		/// <summary>
		/// Class constructor.  Opens a socket connection to target
		/// host and initializes the I/O buffers.  Sends the DAM-ML
		/// connection parameters and returns the response.
		/// The DAM-ML connection parameters are formatted as an
		/// array of bytes.  The byte array reference is provided as
		/// the first entry in a multi-dimensional array parameter,
		/// msg_cp.  The input array reference is sent to the
		/// server and the response is placed in a new byte array
		/// whose reference is returned in place of the input
		/// reference, msg_cp.
		/// </summary>
		/// <param name="targetListString">Hostname/port target list.</param>
		/// <param name="msg_cp">Message layer parameter info.</param>
		public MsgConn(String targetListString, ref byte[] msg_cp)
		{
			title = "Msg[" + ConnID + "]";

			TargetList	targets;
			SqlEx lastEx = null;

			try
			{
				targets = getTargets(targetListString);
			}
			catch (SqlEx)
			{
				throw;
			}
			catch (Exception ex)
			{
				if (trace.enabled(1))
					trace.write(title + ": URL parsing error - " + ex.Message);
				throw SqlEx.get(ERR_GC4000_BAD_URL, ex);
			}

			targets.RandomizeOrder();

			foreach(Target target in targets)
			{
				try
				{
					connect(target.Host, target.Port);
					sendCR(msg_cp);
					msg_cp = recvCC();
					return;			// Connection established
				}
				catch (SqlEx ex)
				{
					lastEx = ex;
				}
			}  // end for loop thru targets

			throw (lastEx != null) ? lastEx : SqlEx.get(ERR_GC4000_BAD_URL);
		} // MsgConn
		
		
		/*
		** Name: disconnect
		**
		** Description:
		**	Disconnect from server and free all I/O resources.
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
		**	17-Nov-99 (gordy)
		**	    Extracted I/O functionality to DbConnIo, DbConnOut, DbConnIn.*/
		
		protected internal override void  disconnect()
		{
			/*
			** We don't set the I/O buffer reference to null
			** here so that we don't have to check it on each
			** use.  I/O buffer functions will continue to work
			** until a request results in a stream I/O request,
			** in which case an exception will be thrown by the
			** I/O buffer.
			**
			** We must, however, test the reference for null
			** since we may be called by the constructor with
			** a null cancel buffer.
			*/
			if (cncl != null)
			{
				try { cncl.close(); }
				catch( Exception ) {}
				finally { cncl = null; }
			}
			
			base.disconnect();
			return ;
		} // disconnect
		
		
		/*
		** Name: close
		**
		** Description:
		**	Close the connection with the server.
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
		**	17-Nov-99 (gordy)
		**	    Extracted I/O functionality to DbConnIo, DbConnOut, DbConnIn.*/
		
		protected internal override void  close()
		{
			lock(this)
			{
				if (!isClosed())
				{
					base.close();
					disconnect();
				}
				return ;
			}
		} // close
		
		
		/*
		** Name: cancel
		**
		** Description:
		**	Issues an interrupt to the Data Access Server which
		**	will attempt to cancel any active query.
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
		**	 4-Nov-99 (gordy)
		**	    Created.
		**	26-Dec-02 (gordy)
		**	    Allocate cncl buffer on first request.
		*/
		
		public virtual void  cancel()
		{
			lock(this)
			{
				/*
				** Allocate cancel buffer on first request.
				*/
				if ( cncl == null )
					try 
					{
						// wrap the socket's NetworkStream in our OutputStream
						OutputStream outputStream = new OutputStream(socket.GetStream());
						cncl = new OutBuff(outputStream, ConnID, 16);
						cncl.TL_ProtocolLevel = this.TL_ProtocolLevel;
					}
					catch( Exception ex )
					{
						if ( trace.enabled( 1 ) )  
							trace.write( title + ": error creating cancel buffer: " + 
								ex.Message );
						disconnect();
						throw SqlEx.get( ERR_GC4001_CONNECT_ERR, ex );
					}

				try
				{
					if ( trace.enabled( 2 ) )
						trace.write( title + ": interrupt network connection" );
					cncl.begin( DAM_TL_INT, 0 );
					cncl.flush();
				}
				catch( SqlEx ex )
				{
					disconnect();
					throw ex;
				}

				return;
			}
		} // cancel
		
		
		/*
		** Name: LockConnection
		**
		** Description:
		**	Set a lock which will block all other threads until the
		**	connection is unlocked.  The lock is released by invoking.
		**	unlock().  A thread must not invoke lock() twice without
		**	invoking unlock() in between.
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
		**	29-Sep-99 (gordy)
		**	    Created.
		**	 4-Feb-00 (gordy)
		**	    A single thread can dead-lock if it starts another operation
		**	    while it already holds the lock.  Save the lock owner and
		**	    throw an exception in this case.
		**	19-May-00 (gordy)
		**	    Changed trace messages.*/
		
		public virtual void  LockConnection()
		{
			LockConnection(false);  // by default, no nested LockConnection's
		}

		public virtual void  LockConnection(bool nesting_allowed)
		{
			lock(lock_obj)
			{
				System.Threading.Thread currentThread =
				    System.Threading.Thread.CurrentThread;

				while (lock_active)  // if already locked by someone
				{
					// if recursively locked by the same thread,
					// make sure it was intentional as in the case
					// of the DTCEnlistment's TxResourceAsyncThread.
					if (lock_thread_id ==
						currentThread.ManagedThreadId)
					{
						if (lock_nesting_allowed)
						{
							lock_nest_count++;
							return;
						}

						if (trace.enabled(1))  // unintentional nested locking!
							trace.write(title + ".lock(): nested locking!!, current " +
								lock_thread_id.ToString());
						throw SqlEx.get( ERR_GC4005_SEQUENCING );
					}
					
					try
					{
						System.Threading.Monitor.Wait(lock_obj);
						// wait for a Pulse from another
						// thread's UnlockConnection
					}
					catch (System.Exception /* ignore */)
					{
					}
				}
				
				// we have the lock!
				lock_thread_id =
					currentThread.ManagedThreadId;
				lock_active = true;
				lock_nesting_allowed = nesting_allowed;
				lock_nest_count = 1;

				if (trace.enabled(5))
					trace.write(title + ".lock(): locked by " +
						lock_thread_id.ToString());
			}  // end lock(lock_obj)
			
			return ;
		}


		/*
		** Name: UnlockConnection
		**
		** Description:
		**	Clear the lock set by LockConnection() and activate blocked threads.
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
		**	29-Sep-99 (gordy)
		**	    Created.
		**	19-May-00 (gordy)
		**	    Check for additional invalid unlock conditions.
		**	13-Aug-10 (thoda04) Bug 124123
		**	    Allow a non-owner thread to unlock a connection 
		**	    that was locked by an owner thread.
		**/

		public virtual void  UnlockConnection()
		{
			lock(lock_obj)
			{
				System.Threading.Thread currentThread =
					System.Threading.Thread.CurrentThread;

				if (lock_thread_id == 0 || !lock_active)
				{
					if (trace.enabled(1))
						trace.write(title + ".unlock(): not locked!, current " +
							currentThread.ManagedThreadId.ToString());
					lock_thread_id = 0;
					lock_active = false;
				}
				else if (lock_thread_id !=
					currentThread.ManagedThreadId)
				{
					if (trace.enabled(1))
						trace.write(title + ".unlock(): not owner, current " +
							currentThread.ManagedThreadId.ToString() +
							", owner " + lock_thread_id.ToString());
				}
				else
				{
					if (trace.enabled(5))
						trace.write(title + ".unlock(): owner " +
							lock_thread_id.ToString());
				}

				// if we had nested LockConnection calls then just return;
				if (--lock_nest_count > 0)
					return;

				// get ready to release the next in the waiting queue
				lock_active = false;
				lock_thread_id = 0;
				lock_nest_count = 0;
				lock_nesting_allowed = false;

				// move the next in the waiting queue to ready queue
				// to get it to ready to come out of its wait
				System.Threading.Monitor.Pulse(lock_obj);
			}  // end lock(lock_obj)
			
			return ;
		}
		
		
		/*
		** Name: encode
		**
		** Description:
		**	Encode a string using a key.  The key and string are translated
		**	to the Server character set prior to encoding.  An optional key
		**	mask, CompatCI.CRYPT_SIZE in length, will be combined with the 
		**	key if provided.
		**
		** Input:
		**	key 	    Encryption key.
		**	mask	    Key mask, may be NULL.
		**	data	    Data to be encrypted
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte []	    Encrypted data.
		**
		** History:
		**	22-Sep-99 (gordy)
		**	    Created.
		**	 6-Sep-02 (gordy)
		**	    Character encoding now encapsulated in CharSet class.
		**	 6-jul-04 (gordy; ported by thoda04)
		**	    Added key mask.
		*/
		
		public virtual byte[] encode(String key, byte[] mask, String data)
			{
			byte[] buff;
			
			try
			{
				// encode password using obfuscated encode name method
				buff = encode( getCharSet().getBytes(key), mask,
				               getCharSet().getBytes( data ) );
			}
			catch (Exception ex)
			{
				throw SqlEx.get( ERR_GC401E_CHAR_ENCODE, ex );	// Should not happen!
			}

			return (buff);
		}
		
		
		/*
		** Name: encode
		**
		** Description:
		**	Applies the Driver semantics to convert the data and key 
		**	into form acceptable to CI and encodes the data using the 
		**	key provided.  An optional key mask, CompatCI.CRYPT_SIZE
		**	in length, will be combined with the key if provided.
		**
		** Input:
		**	key 	    Encryption key.
		**	mask	    Key mask, may be NULL.
		**	data	    Data to be encrypted.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte []	    Encrypted data.
		**
		** History:
		**	 6-May-99 (gordy)
		**	    Created.
		**	22-Sep-99 (gordy)
		**	    Converted parameters to byte arrays for easier processing
		**	    here and to support character set/encoding at a higher level.
		**	21-Apr-00 (gordy)
		**	    Extracted from class AdvanObject.
		**	 6-jul-04 (gordy; ported by thoda04)
		**	    Added key mask.
		*/
		
		public static byte[] encode(byte[] key, byte[] mask, byte[] data)
			{
			byte[] buff = null;
			int i, j, m, n, blocks;
			
			lock(KS)
			{
				/*
				** The key schedule is built from a single CRYPT_SIZE
				** byte array.  We use the input key to build the byte
				** array, truncating or duplicating as necessary.
				*/
				 for (m = n = 0; m < CI.CRYPT_SIZE; m++)
				{
					if (n >= key.Length)
						n = 0;
					kbuff[ m ] = (byte)(key[ n++ ] ^ 
						(mask != null ? mask[ m ] : (byte)0));
				 }
				
				CI.set(kbuff, KS);
				
				/*
				** The data to be encoded must be padded to a multiple
				** of CRYPT_SIZE bytes.  Random data is used for the
				** pad, so for strings the null terminator is included
				** in the data.  A random value is added to each block
				** so that a given key/data pair does not encode the
				** same way twice.  
				**
				** The total number of blocks can be calculated from 
				** the string length plus the null terminator divided 
				** into CRYPT_SIZE blocks (with one random byte): 
				**	((len+1) + (CRYPT_SIZE-2)) / (CRYPT_SIZE-1)
				** which can be simplified as: 
				**	(len / (CRYPT_SIZE - 1) + 1
				*/
				blocks = (data.Length / (CI.CRYPT_SIZE - 1)) + 1;
				buff = new byte[blocks * CI.CRYPT_SIZE];
				
				 for (i = m = n = 0; i < blocks; i++)
				{
					buff[m++] = (byte) (rand.Next( 256 ));
					
					 for (j = 1; j < CI.CRYPT_SIZE; j++)
						if (n < data.Length)
							buff[m++] = data[n++];
						else if (n > data.Length)
							buff[m++] = (byte) (rand.Next( 256 ));
						else
						{
							buff[m++] = 0; // null terminator
							n++;
						}
				}
				
				// encode using obfuscated encode name method
//				CI.encode  (buff, 0, buff.Length, KS, buff, 0);
				CI.toString(buff, 0, buff.Length, KS, buff, 0);
			}
			
			return (buff);
		} // encode


		/*
		** Name: getTargets
		**
		** Description:
		**	Parses a target list and returns an array of host/port pairs.
		**
		**	    (<host>:<port>[,<port>][;<host>:port[,port]])
		**
		** Input:
		**	targetList
		**
		** Output:
		**	None.
		**
		** Returns:
		**	TargetList    ArrayList of host/port pairs.
		**
		** History:
		**	12-May-09 (thoda04)
		**	    Created.
		*/

		private TargetList
		getTargets(String targetListString)
		{
			TargetList targetList = new TargetList();
			string     defaultPorts = "II7";

			// Before we start, strip any leading and trailing parentheses,
			// get the host names inside the list,
			// and save the default ports after the host list.
			targetListString = 
				getTargetListString(targetListString, ref defaultPorts);

			//
			// First, extract the semi-colon separated host/port targets.
			//
			String[] targets = parseList(targetListString, ';');

			//
			// Next, extract host and port list from each target.
			//
			for (int i = 0; i < targets.Length; i++)
			{
				String   host  = getHost(targets[i]);
				String[] ports = getPort(targets[i], defaultPorts);

				for (int j = 0; j < ports.Length; j++)
				{
					targetList.Add(host, ports[j]);
				}
			}

			return( targetList );
		} // getTargets


		/*
		** Name: getHost
		**
		** Description:
		**	Extracts and returns the name of the host from the target
		**	string:
		**
		**	    <host>[:<port>]
		**
		** Input:
		**	target
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Host name.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		**	19-Oct-06 (lunbr01)  Sir 116548
		**	    Allow for IPv6 addresses enclosed in square brackets '[]'.
		*/

		/// <summary>
		/// Extracts and returns the name of the host from the target.
		/// </summary>
		/// <param name="target"></param>
		/// <returns>Name of the host from the target.</returns>
		private String getHost(String target)
		{
			int index;
			int start = 0;
			if (target.StartsWith("["))
			{
				start = 1;
				index = target.IndexOf(']');
				if (index < 0)
				{
					if (trace.enabled(1))
						trace.write(title + ": right bracket ']' missing at end of IPv6 @ in '" + target + "'");
					throw SqlEx.get(ERR_GC4000_BAD_URL);
				}
			}
			else
				index = target.IndexOf(':');

			return (index < 0 ? 
				target : 
				target.Substring(start, index - start));
		}  // getHost


		/*
		** Name: getPort
		**
		** Description:
		**	Extracts and returns the port number from the target string:
		**
		**	    <host>:<port>{,<port>}
		**
		**	The port may be specified as a numeric string or in standard
		**	Ingres installation ID format such as II0.
		**
		** Input:
		**	target	Host/port list target string.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String []	Port ID list.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		**	 1-Oct-02 (gordy)
		**	    Extracted symbolic port translation to separate method.
		**	19-Oct-06 (lunbr01)  Sir 116548
		**	    Allow for IPv6 addresses enclosed in square brackets '[]'.
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    List of port ID's now supported and returned as individual
		**	    entries in an array.  Ports are no longer translated into
		**	    numeric values.
		*/

		private String[] getPort(String target, String defaultPorts)
		{
			int index = 0;

			/*
			** Skip past IPv6 host address if present.
			*/
			if (target.StartsWith("["))
			{
				if ((index = target.IndexOf(']')) < 0)
				{
					if (trace.enabled(1))
						trace.write(title + ": right bracket ']' missing " +
								 "in IPv6 address '" + target + "'");
					throw SqlEx.get(ERR_GC4000_BAD_URL);
				}
			}

			index = target.IndexOf(':', index);

			if (index < 0 || target.Length <= (index+1))
			{
				if (trace.enabled(1))
					trace.write(title + ":port number missing '" + target + "'");
				if (defaultPorts == null  ||  defaultPorts.Length == 0)
					return new String[] { "II7" };  // default
				else return (parseList(defaultPorts, ','));
			}

			return (parseList(target, ',', index + 1, 0));
		}  // getPort


		/*
		** Name: parseList
		**
		** Description:
		**	Parses a list delimited by a caller supplied character.
		**	Returns an array of the list components.
		**
		** Input:
		**	list	Delimited list.
		**	delim	Delimiting character.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String []	Array of list components.
		**
		** History:
		**	 5-May-09 (gordy, ported by thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Parses a list delimited by a caller supplied character.
		/// </summary>
		/// <param name="list">String to be parsed.</param>
		/// <param name="delim">Delimiter (usually a semicolon or comma).
		/// </param>
		/// <returns>String[] array of list components.</returns>
		private String[]
		parseList(String list, char delim)
		{
			return (parseList(list, delim, 0, 0));
		}


		/*
		** Name: parseList
		**
		** Description:
		**	Parses a list delimited by a caller supplied character.  
		**	Returns an array of the list components.
		**
		**	This method is called recursively to process each component.  
		**	Initial call should pass 0 for offset and count.
		**
		** Input:
		**	list	Delimited list.
		**	delim	Delimiting character.
		**	offset	Offset of next component.
		**	count	Number of preceding components.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String []	Array of list components.
		**
		** History:
		**	 5-Dec-07 (gordy)
		**	    Created.
		**	 5-May-09 (gordy, ported by thoda04)
		**	    Generalized and renamed.
		*/

		/// <summary>
		/// Parses a list delimited by a caller supplied character.
		/// </summary>
		/// <param name="list">String to be parsed.</param>
		/// <param name="delim">Delimiter (usually a semicolon or comma).
		/// </param>
		/// <param name="offset">Offset of next component.</param>
		/// <param name="count">Number of preceding components.</param>
		/// <returns>String[] array of list components.</returns>
		private String[]
		parseList(String list, char delim, int offset, int count)
		{
			String[]	entries;
			int		index;

			if ( offset >= list.Length )
			{
			/*
			** Current component is empty and there are no
			** additional components.  Allocate the entry array
			** for all preceding components (which may be 0).
			*/
			entries = new String[ count ];
			}
			else  if ( (index = list.IndexOf( delim, offset )) >= 0 )
			{
			if ( (index - offset) <= 1 )
			{
				/*
				** Current component is empty.  Recursively
				** process the tail of the list.
				*/
				entries = parseList( list, delim, index + 1, count );
			}
			else
			{
				/*
				** Recursively process the tail of the list.
				*/
				entries = parseList( list, delim, index + 1, count + 1 );

				/*
				** Save current component.
				*/
					entries[ count ] = 
						list.Substring( offset, index - offset ).Trim();
			}
			}
			else
			{
			/*
			** This is the last, or only, component.  Allocate 
			** the entry array including room for all preceding 
			** components, and save this entry in the list.
			*/
			entries = new String[ count + 1 ];
			entries[ count ] = (offset > 0) ? 
				list.Substring( offset ).Trim() : 
				list;
			}

			return( entries );
		} // parseList


		/*
		** Name: getTargetListString
		**
		** Description:
		**	Parses a host/port list.
		**	Returns a string of just the hosts and 
		**	a string of just the default ports after the "(...):" host list
		**	into the ref defaultPorts.
		**
		** Input:
		**	Hosts and ports string, e.g.
		**
		**	    (<host>:<port>[,<port>][;<host>:port[,port]]):II7,II8
		**
		** Returns:
		**	Target list string, e.g.
		**
		**	    <host>:<port>[,<port>][;<host>:port[,port]]
		**
		** Output:
		**	Default ports list string, e.g.
		**	    II7,II8
		**
		** History:
		**	04-Aug-09 (thoda04)
		**	    Created.
		*/
		/// <summary>
		/// Parses a host/port list. Returns a string of just the hosts
		/// and a string of just the default ports.
		/// E.g. for an input of hosts and ports:
		///     (host:port[,port][;host:port[,port]]):II7,II8
		/// returns
		///     host:port[,port][;host:port[,port]]
		/// and the default ports into the ref defaultPorts.
		/// </summary>
		/// <param name="targetListString"></param>
		/// <param name="defaultPorts"></param>
		/// <returns></returns>
		private String
		getTargetListString(String targetListString, ref string defaultPorts)
		{
			string hostListString = targetListString;

			// if target list is not in list format, return original
			if (
				(targetListString.StartsWith("(") &&
				 targetListString.Contains(")")) == false)
				return targetListString;

			//strip out the contents of the host list inside the parens
			int parenCount = 0;
			int charCount  = 0;

			// scan the host list, keeping count of parentheses level
			foreach (char c in targetListString)
			{
				charCount++;

				if (c == '(')   // opening paren
				{
					parenCount++;
					continue;
				}  // end '('

				if (c == ')')   // closinging paren
				{
					parenCount--;
					if (parenCount == 0)  // if all done, return string
					{
						hostListString =
						    targetListString.Substring(1, charCount-2);
						// if "(local)" was the list, return this special value
						if (hostListString.ToLowerInvariant() == "local")
						    hostListString =          // return "(local)"
						        targetListString.Substring(0,charCount);
						break;   // break out of host list scan
					}
				}  // end ')'

				continue;
			}  // end foreach (char c in targetListString)


			// process the :default_port_numbers that follow the (hosts)
			string portsString = targetListString.Substring(charCount);

			if (portsString.Length > 1 &&
				portsString.StartsWith(":"))
				defaultPorts = portsString.Substring(1);

			return hostListString;
		}


	/// <summary>
	/// Return a random true or false boolean value.
	/// </summary>
	/// <returns>A random true or false.</returns>
	private static Boolean RandomNextBoolean()
	{
		Random	rand = new Random();

		int ZeroOrOne = rand.Next(2);  // gets    a random 0 or 1
		return (ZeroOrOne == 0);       // returns a random false or true
	}

	private struct Target
	{
		public string Host;
		public string Port;

		public Target(string host, string port)
		{
			Host = host;
			Port = port;
		}

		public Target Clone()
		{
			return new Target(Host, Port);
		}

		public override String ToString()
		{
			if (Host == null)
				return "<null>";
			if (Port == null | Port.Length == 0)
				return Host;
			else
				return Host + ":" + Port;
		}
	}

	/// <summary>
	/// A collection of the Target information (hostname and port).
	/// </summary>
	private  class TargetList : 
		System.Collections.ICollection,
		System.Collections.IEnumerable
	{
		private System.Collections.ArrayList targetlist;

		internal TargetList()
		{
			targetlist = new System.Collections.ArrayList();
		}

		/// <summary>
		/// Get the enumerator to iterate through the collection.
		/// </summary>
		/// <returns>An IEnumerator to iterate
		/// through the collection.</returns>
		public System.Collections.IEnumerator GetEnumerator()
		{
			return targetlist.GetEnumerator();
		}

		/// <summary>
		/// Get the item at the specified zero-based index.
		/// </summary>
		public Target this[int index]
		{
			get { return (Target)targetlist[index]; }
			set { targetlist[index] = value; }
		}

		/// <summary>
		/// Get the count of items in the collection.
		/// </summary>
		public int Count
		{
			get { return targetlist.Count; }
		}

		/// <summary>
		/// Returns true if access to the collection is synchronized.
		/// </summary>
		public bool IsSynchronized       /* ICollection interface */
		{
			get { return false; }
		}

		/// <summary>
		/// Return an object that can the used to
		/// synchronize access to the collection.
		/// </summary>
		public object SyncRoot           /* ICollection interface */
		{
			get { return this; }
		}

		internal int Add(Target value)
		{
			return targetlist.Add(value);
		}

		internal int Add(string host, string port)
		{
			return targetlist.Add(new Target(host, port));
		}

		internal void Clear()
		{
			targetlist.Clear();
		}

		/// <summary>
		/// Copies the elements of the Collection into a one-dimension,
		/// zero-based  Array object, beginning at the specified zero-based index.
		/// </summary>
		void System.Collections.ICollection.CopyTo(Array array, int index)
		{
			targetlist.CopyTo(array, index);
		}

		/// <summary>
		/// Copies the elements of the Collection into a one-dimension,
		/// zero-based  Array object, beginning at the specified zero-based index.
		/// </summary>
		public void CopyTo(Target[] array, int index)
		{
			((System.Collections.ICollection)this).CopyTo(array, index);
		}

		public void RandomizeOrder()
		{
			int Count = targetlist.Count;
			if (Count < 2)  // nothing to do if 0 or 1 entries
				return;

			/*
			** Step through all entries and
			** randomly switch with some other entry.
			*/
			Random	rand = new Random();


			for (int i = 0; i < Count; i++)
			{
				int j = rand.Next(Count);

				/*
				** Nothing to do if target is same as source.
				*/
				if (i != j)
				{
					// swap t1 and t2 values
					Target t1 = (Target)targetlist[i];
					Target t2 = (Target)targetlist[j];
					targetlist[i] = t2;
					targetlist[j] = t1;
				}

				if (Count == 2) break;  // quit if only two entries
			}  // end for (int i = 0; i < Count; i++)
		}

		public override String ToString()
		{
			if (targetlist.Count == 0)
				return "";

			System.Text.StringBuilder sb = new System.Text.StringBuilder();
			foreach (Target target in targetlist)
			{
				sb.Append(target.ToString());
				sb.Append(';');
			}  // end foreach
			return sb.ToString();
		}  // end ToString()

	}  // class TargetList

	}  // class MsgConn
}