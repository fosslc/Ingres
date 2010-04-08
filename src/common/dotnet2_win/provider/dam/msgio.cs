/*
** Copyright (c) 1999, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: msgio.cs
	**
	** Description:
	**	Defines base class representing a connection with a Data
	**	Access Server.
	**
	**	The Data Access Messaging (DAM) protocol classes present 
	**	a single unified interface, through inheritance, for access 
	**	to a Data Access Server.  They are divided into separate 
	**	classes to group related functionality for easier management.  
	**	The order of inheritance is determined by the initial actions
	**	required during initialization of the connection:
	**
	**	    MsgIo  	Establish socket connection.
	**	    MsgOut 	Send TL Connection Request packet.
	**	    MsgIn  	Receive TL Connection Confirm packet.
	**	    MsgConn	General initialization.
	**
	** History:
	**	 7-Jun-99 (gordy)
	**	    Created.
	**	17-Nov-99 (gordy)
	**	    Extracted from DbConn.cs.
	**	21-Apr-00 (gordy)
	**	    Moved to package io.
	**	28-Mar-01 (gordy)
	**	    Separated tracing interface and implementating.
	**	14-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	 6-Sep-02 (gordy)
	**	    Character encoding now encapsulated in CharSet class.
	**	 1-Oct-02 (gordy)
	**	    Moved to GCF dam package.  Renamed as DAM Message
	**	    Layer implementation class.
	**	29-Oct-02 (wansh01)
	**	    Added public method isLocal to determine if connection is local.
	**	31-Oct-02 (gordy)
	**	    Renamed GCF error codes.
	**	20-Dec-02 (gordy)
	**	    Track messaging protocol level.
	**	15-Jan-03 (gordy)
	**	    Added local/remote host name/address methods.
	**	26-Feb-03 (gordy)
	**	    Added getCharSet().
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	17-nov-04 (thoda04)
	**	    Be forgiving and treat "(local)" as localhost.
	**	04-may-06 (thoda04)
	**	    Replace the obsolete .NET 1.1 GetHostByName() method with
	**	    new .NET 2.0 System.Net.Dns.GetHostEntry() method.
	**	 3-May-05 (gordy)
	**	    Added public abort() to perform same action (at this level)
	**	    as protected disconnect().
	**	 3-Jul-06 (gordy)
	**	    Allow character-set to be overridden.
	**	29-Dec-06 (lunbr01, thoda04)  Sir 116548
	**	    Allow for IPv6 addresses enclosed in square brackets '[]'.
	**	    Try connecting to ALL IP @s for target host rather than just 1st.
	**	19-sep-07 (thoda04)
	**	    Added isSocketConnected indicator to return health of underlying socket.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Adding support for multiple server targets.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Support extended subports in symbolic port ID.
	**	04-aug-09 (thoda04)
	**	    Corrected text of error message that should refer to the
	**	    new .NET 2.0 System.Net.Dns.GetHostEntry() method.
	**	04-feb-10 (thoda04) B123229
	**	    Data provider connects slowly if using IPv6 addresses.
	**	    Use TcpClient(AddressFamily) constructor to avoid
	**	    SocketException if an IPv6 address is passed.
	**	    Replace System.Net.Dns.GetHostEntry() method with
	**	    GetHostAddresses() to avoid with DNS reverse lookup.
	*/


	/*
	** Name: MsgIo
	**
	** Description:
	**	Class representing a connection to a Data Access Server.
	**	Provides functionality for establishing and dropping the 
	**	connection.
	**
	**  Constants
	**
	**	DAM_ML_TRACE_ID	Message Layer trace ID.
	**
	**  Public Methods:
	**
	**	abort			Abort network connection.
	**	connID  	Return connection identifier.
	**	isClosed	Is the connection closed?
	**	isLocal		Is the connection local?
	**	getRemoteHost	Get server machine name.
	**	getRemoteAddr	Get server network address.
	**	getLocalHost	Get local machine name.
	**	getLocalAddr	Get local network address.
	**	setProtocolLevel	Set Messaging Layer protocol level.
	**	getCharSet		Get character-set used for connection.
	**	setCharSet		Set character-set used for connection.
	**
	**  Protected Data:
	**
	**	socket  	Network socket used for the connection.
	**	char_set	Character encoding.
	**	tl_proto	Connection protocol level.
	**	title		Class title for tracing.
	**	trace		Trace output.
	**
	**  Protected Methods:
	**
	**	connect   	Connect to target server.
	**	disconnect	Shutdown the physical connection.
	**
	**  Private Data:
	**
	**	conn_id      	Connection ID.
	**	connections  	Static counter for connection ID.
	**	msg_proto_lvl	Message Layer protocol level.
	**	tl_proto_lvl 	Transport Layer protocol level.
	**
	**  Private Methods:
	**
	**	close          	Close socket.
	**	translatePortID	Translate symbolic/numeric port ID to port number.
	**
	** History:
	**	 7-Jun-99 (gordy)
	**	    Created.
	**	22-Sep-99 (gordy)
	**	    Added character set/encoding.
	**	17-Nov-99 (gordy)
	**	    Extracted base level functionality from DbConn.
	**	 6-Sep-02 (gordy)
	**	    Character encoding now encapsulated in CharSet class.
	**	 1-Oct-02 (gordy)
	**	    Renamed as DAM-ML messaging class.  Added DAM-ML trace ID.
	**	    Moved tl_proto, char_set and finalize() to sub-class.
	**	29-Oct-02 (wansh01)
	**	    Added public method isLocal to determine if connection is local.
	**	20-Dec-02 (gordy)
	**	    Track messaging protocol level.  Added msg_proto_lvl and 
	**	    setProtocolLevel().  Restored tl_proto_lvl and char_set to
	**	    make this class the central repository for all data within
	**	    the messaging classes.
	**	15-Jan-03 (gordy)
	**	    Added getRemoteHost(), getRemoteAddr(), getLocalHost(), and
	**	    getLocalAddr().
	**	26-Feb-03 (gordy)
	**	    Added getCharSet().
	**	 3-May-05 (gordy)
	**	    Extracted disconnect() functionality to private method close()
	**	    so that it may be shared.  Added public method abort() which
	**	    calls close() to drop the network connection.
	**	19-Oct-06 (lunbr01)  Sir 116548
	**	    Try connecting to ALL IP @s for target host rather than just 1st.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Extract connection functionality from constructor to connect().
	**	    Moved target parsing methods getHost() and getPort() to MsgConn.
	*/

	internal class MsgIo : TlConst
	{
		/*
		** Constants
		*/
		public const String DAM_ML_TRACE_ID = "msg";    // DAM-ML trace ID.

		/*
		** The network connection.
		*/
		protected MsgSocket                     socket     = null;
		protected CharSet                       char_set = null;
		private   System.Net.IPAddress          ipAddress  = null;
		private   bool                          socketIsLocal = false;
    
		/*
		** Protocol must be assumed to be lowest level
		** until a higher level is negotiated.
		*/
		private byte  msg_proto_lvl = MSG_PROTO_1;
		private byte  tl_proto_lvl  = DAM_TL_PROTO_1;

		/*
		** Tracing.
		*/
		protected String	title = null;	// Title for tracing
		protected ITrace	trace;       	// Tracing.

		/*
		** Instance ID and connection count.
		*/
		private int       	    conn_id     = -1;	// connection ID
		private static int	    connections = 0; 	// Number of connections


		/*
		** Name: MsgIo
		**
		** Description:
		**	Class constructor.  Opens a socket connection to target host.
		**
		** Input:
		**	host_id		Hostname and port.
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
		**	17-Nov-99 (gordy)
		**	    Extracted base functionality from DbConn.
		**	28-Mar-01 (gordy)
		**	    Separated tracing interface and implementating.
		**	 1-Oct-02 (gordy)
		**	    Renamed as DAM-ML messaging class.
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Extracted connect() functionality.
		*/

		protected internal MsgIo()
		{
			conn_id = connections++;
			trace = new Tracing(DAM_ML_TRACE_ID);
			title = "MsgIo[" + conn_id + "]";
		} // MsgIo

		/*
		** Name: connect
		**
		** Description:
		**	Opens a socket connection to target host.
		**
		** Input:
		**	host		Host name or address.
		**	portID		Symbolic or numeric port ID.
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
		**	17-nov-04 (thoda04)
		**	    Be forgiving and treat "(local)" as localhost.
		**	04-may-06 (thoda04)
		**	    Replace the obsolete .NET 1.1 GetHostByName() method with
		**	    new .NET 2.0 System.Net.Dns.GetHostEntry() method.
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Extracted from constructor.
		**	04-feb-10 (thoda04) B123229
		**	    Data provider connects slowly if using IPv6 addresses.
		**	    Use TcpClient(AddressFamily) constructor to avoid
		**	    SocketException if an IPv6 address is passed.
		**	    Replace System.Net.Dns.GetHostEntry() method with
		**	    GetHostAddresses() to avoid with DNS reverse lookup.
		*/

		protected internal virtual void
		connect( String host, String portID )
		{
			int    i;
			System.Net.IPAddress[] ipAddresses;

			System.Net.IPEndPoint  ipEndPoint = null;
			socket                            = null;
			ipAddresses                       = null;
			ipAddress                         = null;
			socketIsLocal                     = false;

			try
			{
				if (trace.enabled(2))
					trace.write(title + ": opening network connection '" +
						host + "', '" + portID + "'");
			
				if (host == null    ||
					host.Length==0  ||    // if local machine
					host.ToLowerInvariant() == "(local)")
				{
					ipAddresses = System.Net.Dns.GetHostAddresses(System.Net.Dns.GetHostName());
					socketIsLocal = true;
				}
				else
					ipAddresses = System.Net.Dns.GetHostAddresses(host);

				int port = translatePortID(portID);

				for (i = 0; i < ipAddresses.Length; i++)  // loop addresses
				{
					ipAddress = ipAddresses[i];

					if (trace.enabled(3))
					{
						string address = ipAddress.ToString() + "  ";
						address += ipAddress.AddressFamily.ToString();
						if (ipAddress.AddressFamily ==
							System.Net.Sockets.AddressFamily.InterNetwork)
							address += "(IPv4)";
						else if (ipAddress.AddressFamily ==
							System.Net.Sockets.AddressFamily.InterNetworkV6)
						{
							address += "(IPv6),";
							address += "ScopeId=" + ipAddress.ScopeId.ToString();
						}
						trace.write(title + ": host address " + address +
							"; port=" + port);
					}

					ipEndPoint = new System.Net.IPEndPoint(ipAddress, port);
					socket = new MsgSocket(ipAddress.AddressFamily);
					try
					{
						socket.Connect(ipEndPoint);
						if (trace.enabled(3))
							trace.write(title + ": connected to host address and port");
						return;  // success!
					}
					catch (Exception ex)
					{
						if (trace.enabled(1))
							trace.write(title + ": connection failed to address[" + i + "]=" + ipAddress + " - " + ex.Message);
						if (i == (ipAddresses.Length - 1))
							throw;   // continue throw of last exception up chain
						             // else try next address on list.
					}
				}  // end for loop through ipHostInfo.AddressList

				if (i == ipAddresses.Length)
				{  // should not have reached this point but we will bullet-proof
					trace.write(title + ": connection error - all addresses failed");
					throw SqlEx.get(ERR_GC4001_CONNECT_ERR);
				}
			}  // end try block
			catch (System.Net.Sockets.SocketException ex)
			{
				if (trace.enabled(1))
					trace.write(title +
						": error connecting to host - (SocketException) " +
						ex.Message);
				SqlEx pex = (SqlEx)
					SqlEx.get( ERR_GC4001_CONNECT_ERR, ex );
					// "Unable to establish connection due to communications error."
				if (ipAddresses == null)  // is it the GetHostAddresses( )
					pex = SqlEx.get(pex, SqlEx.get(
						"SocketException thrown " +
						"by System.Net.Dns.GetHostAddresses(" +
						(host==null?"<null>":("\""+host+"\"")) + ") " +
						"in resolving the hostname",
							"08001",ex.ErrorCode));
				else  // must be TcpClient( ) call
					pex = SqlEx.get(pex, SqlEx.get(
						"SocketException thrown " +
						"by System.Net.Sockets.TcpClient( ) " +
						"while connecting to host " +
						(host == null ? "<null>" : ("\"" + host + "\".")),
							"08001",ex.ErrorCode));

				socket                            = null;
				ipAddress                         = null;

				throw pex;
			}
			catch (System.Exception ex)
			{
				if (trace.enabled(1))
					trace.write(title + ": error connecting to host - " + ex.Message);

				socket                            = null;
				ipAddress                         = null;

				throw SqlEx.get( ERR_GC4001_CONNECT_ERR, ex );
			}
		}  // connect


		/*
		** Name: close
		**
		** Description:
		**	Close socket used for network connection.
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
		**	 3-May-05 (gordy)
		**	    Created.
		*/

		private void
		close()
		{
			if (socket != null)
			{
				if (trace.enabled(2))
					trace.write(title + ": closing network connection");

				try { socket.Close(); }
				catch (Exception /* ignore */) { }
				finally
				{
					socket = null;
					ipAddress = null;
				}
			}

			return;
		} // close


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
		*/
		
		protected internal virtual void  disconnect()
		{
			close();
			return;
		}  // disconnect


		/*
		** Name: abort
		**
		** Description:
		**	Abort the network connection.
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
		**	 3-May-05 (gordy)
		**	    Created.
		*/

		public void
		abort()
		{
			close();
			return;
		}


		/*
		** Name: isClosed
		**
		** Description:
		**	Returns an indication that the connection is closed.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if connection is closed, False otherwise.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		*/
		
		public virtual bool isClosed()
		{
			return (socket == null);
		}  // isClosed


		/*
		** Name: isSocketConnected
		**
		** Description:
		**	Returns an indication that the underly socket connection is connected.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if socket connection is connected, False otherwise.
		**
		** History:
		**	19-sep-07 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Return an indication that the underly physical socket connection is still connected.
		/// </summary>
		/// <returns></returns>
		public virtual bool isSocketConnected()
		{
			if (socket == null)
				return false;
			return (socket.Connected);
		}  // isSocketConnected


		/*
		** Name: isLocal
		**
		** Description:
		**	Returns an indication that the connection is local.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if connection is local, False otherwise.
		**
		** History:
		**	 29-Oct-02 (wansh01)
		**	    Created.
		*/

		public bool isLocal()
		{
			return( (socket == null) ? false : this.socketIsLocal);
		} // isLocal


		/*
		** Name: connID property
		**
		** Description:
		**	Returns the numeric connection identifier.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Connection identifier.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// The numeric connection identifier.
		/// </summary>
		public int ConnID
		{
			get {return (conn_id); }
		}  // connID


		/*
		** Name: RemoteHost
		**
		** Description:
		**	Returns the host name of the server machine.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Server host name or NULL if error.
		**
		** History:
		**	15-Jan-03 (gordy)
		**	    Created.
		*/

		public String RemoteHost
		{
			get { return socket.RemoteHostName; }
		} // RemoteHost


		/*
		** Name: RemoteAddress
		**
		** Description:
		**	Returns the network address of the server machine.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Server network address or NULL if error.
		**
		** History:
		**	15-Jan-03 (gordy)
		**	    Created.
		*/

		public String RemoteAddress
		{
			get { return( socket.RemoteAddress.ToString() ); }
		} // RemoteAddress


		/*
		** Name: LocalHost
		**
		** Description:
		**	Returns the host name of the local machine.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Local host name or NULL if error.
		**
		** History:
		**	15-Jan-03 (gordy)
		**	    Created.
		*/

		public String LocalHost
		{
			get { return( socket.LocalHostName ); }
		} // LocalHost


		/*
		** Name: LocalAddress
		**
		** Description:
		**	Returns the network address of the local machine.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Local network address or NULL if error.
		**
		** History:
		**	15-Jan-03 (gordy)
		**	    Created.
		*/

		public String LocalAddress
		{
			get { return( socket.LocalAddress.ToString() ); }
		} // LocalAddress


		/*
		** Name: ML_ProtocolLevel property
		**
		** Description:
		**	Get/Set the Messaging Layer protocol level.  This level is used
		**	to build message packets.  Message content must still be
		**	controlled by the class user's.
		**
		** Input:
		**	value	Message Layer protocol level.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	20-Dec-02 (gordy)
		**	    Created.
		*/

		public byte ML_ProtocolLevel
		{
			get { return this.msg_proto_lvl ; }
			set { this.msg_proto_lvl = value; }
		} // setProtocolLevel


		/*
		** Name: TL_ProtocolLevel property
		**
		** Description:
		**	Get/Set the Transport Layer protocol level.  This level is used
		**	to build message packets.  Message content must still be
		**	controlled by the class user's.
		**
		** Input:
		**	value	Message Layer protocol level.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	20-Dec-02 (gordy)
		**	    Created.
		*/

		public byte TL_ProtocolLevel
		{
			get { return this.tl_proto_lvl ; }
			set { this.tl_proto_lvl = value; }
		} // setProtocolLevel


		/*
		** Name: getCharSet
		**
		** Description:
		**	Returns the CharSet used to encode strings for the connection.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	CharSet	    Connection CharSet.
		**
		** History:
		**	26-Feb-03 (gordy)
		**	    Created.
		*/

		public CharSet
		getCharSet()
		{
			return (char_set);
		} // getCharSet


		/*
		** Name: setCharSet
		**
		** Description:
		**	Set the CharSet used to encode strings for the connection.
		**
		** Input:
		**	char_set	CharSet to be used.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 3-Jul-06 (gordy)
		**	    Created.
		*/

		public void
		setCharSet(CharSet char_set)
		{
			this.char_set = char_set;
			return;
		} // setCharSet


		/*
		** Name: translatePortID
		**
		** Description:
		**	Translates a symbolic or numeric port ID to a port number.
		**	A negative value is returned if an error occurs.
		**
		** Input:
		**	port	Port ID.
		**
		** Ouptut:
		**	None.
		**
		** Returns:
		**	int	Port number.
		**
		** History:
		**	 1-Oct-02 (gordy)
		**	    Extracted from getPort().
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Throws exception if symbolic port ID cannot be translated
		**	    into numeric value.
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Support extended subports.
		*/

		private int  translatePortID( String port )
		{
			int portnum;
			int subport = 0;
			char c0, c1;

			/*
			** Check for Ingres Installation ID format: [a-zA-Z][a-zA-Z0-9]{n}
			** where n is [0-9] | 0[0-9] | 1[0-5]
			*/
			switch (port.Length)
			{
				case 4:
					/*
					** Check most significant digit of subport.
					*/
					c0 = port[2];
					if (!Char.IsDigit(c0))  // symbolic like II15 ?
						break;	/* Not a symbolic port ID */

					/*
					** Save most significant digit so that it may be 
					** combined with least significant digit below.
					*/
					subport = Int32.Parse(c0.ToString()) * 10;

					goto case 3;  // Fall through to process least significant digit...

				case 3:
					/*
					** Check least significant digit of subport
					** (Optional most significant digit may be
					** present and was processed above).
					*/
					c0 = port[port.Length - 1];  // look at last digit
					if (!Char.IsDigit(c0))       // symbolic like II15 or II7?
						break;	/* Not a symbolic port ID */

					/*
					** Combine least significant digit with optional
					** most significant digit processed in prior case
					** and check for valid range.
					*/
					subport += Int32.Parse(c0.ToString());
					if (subport > 15) break;	/* Not a symbolic port ID */

					goto case 2;  // Fall through to process instance ID...

				case 2:
					/*
					** Check for valid instance ID.
					*/
					c0 = port[0];
					c1 = port[1];
					if (!(Char.IsLetter(c0) &&     // symbolic like II ?
						  Char.IsLetterOrDigit(c1)))
						break;	/* Not a symbolic port ID */

					/*
					** Map symbolic port ID to numeric port.
					** Subport was generated in prior cases 
					** or defaulted to 0.
					*/
					c0 = Char.ToUpper(c0);
					c1 = Char.ToUpper(c1);

					portnum = (((subport > 0x07) ? 0x02 : 0x01) << 14)
							| ((c0 & 0x1f) << 9)
							| ((c1 & 0x3f) << 3)
							| (subport & 0x07);

					if (trace.enabled(5))
						trace.write(title + ": " + port + " => " + portnum);

					return (portnum);
			}

			try { portnum = Int32.Parse(port); }
			catch (Exception)
			{
				if (trace.enabled(1))
					trace.write(title + ": invalid port ID '" + port + "'");
				throw SqlEx.get(ERR_GC4000_BAD_URL);
			}

			return (portnum);
		} // translatePortID


		/*
		** Name: QuerySocketHealth
		**
		** Description:
		**	Query the MSG's underly socket to test if it is still connected.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if socket connection is connected, False otherwise.
		**
		** History:
		**	19-sep-07 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Test and return an indication that the underly physical socket connection is still connected.
		/// </summary>
		/// <returns></returns>
		public virtual bool QuerySocketHealth()
		{
			if (socket == null)
				return false;

			return socket.QuerySocketHealth();
		}  // QuerySocketHealth


	}  // class MsgIo



	/*
	** Name: MsgSocket
	**
	** Description:
	**	Class representing a socket connection to a Data Access Server.
	**	This class gives us access to the underlying Socket client under
	**	the TcpClient object.
	**
	**  Public Properties:
	**
	**	LocalAddress  	Returns IPAddress of local side of socket connection.
	**	RemoteAddress 	Returns IPAddress of remote side of socket connection.
	**	LocalHostName 	Returns the host name of the local machine.
	**	RemoteHostName	Returns the host name of the remote machine.
	**
	** History:
	**	12-Nov-03 (thoda04)
	**	    Created.
	**	04-feb-10 (thoda04) B123229
	**	    Data provider connects slowly if using IPv6 addresses.
	**	    Use TcpClient(AddressFamily) constructor to avoid
	**	    SocketException if an IPv6 address is passed.
	*/

	internal class MsgSocket : System.Net.Sockets.TcpClient
	{

		/// <summary>
		/// Message socket wrapper for TcpClient socket.
		/// </summary>
		/// <param name="family">AddressFamily indicator for IPv4 or IPv6 family.</param>
		public MsgSocket(System.Net.Sockets.AddressFamily family)
			: base(family)
		{
		} // MsgSocket

		/*
		** Name: LocalAddress
		**
		** Description:
		**	Returns the IPAddress of the local side of the socket connection.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	IPAddress	IPAddress of the local side of the socket connection.
		**
		** History:
		**	12-Nov-03 (thoda04)
		**	    Created.
		*/

		public System.Net.IPAddress LocalAddress
		{
			get
			{
				System.Net.Sockets.Socket socket = base.Client;
				System.Net.IPEndPoint ipEndPoint =
					(System.Net.IPEndPoint)socket.LocalEndPoint;
				return ipEndPoint.Address;
			}
		}


		/*
		** Name: RemoteAddress
		**
		** Description:
		**	Returns the IPAddress of the remote side of the socket connection.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	IPAddress	IPAddress of the remote side of the socket connection.
		**
		** History:
		**	12-Nov-03 (thoda04)
		**	    Created.
		*/

		public System.Net.IPAddress RemoteAddress
		{
			get
			{
				System.Net.Sockets.Socket socket = base.Client;
				System.Net.IPEndPoint ipEndPoint =
					(System.Net.IPEndPoint)socket.RemoteEndPoint;
				return ipEndPoint.Address;
			}
		}


		/*
		** Name: LocalHostName
		**
		** Description:
		**	Returns the host name of the local machine.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Local host name or NULL if error.
		**
		** History:
		**	12-Nov-03 (thoda04)
		**	    Created.
		*/

		public string LocalHostName
		{
			get
			{
				try
				{
					return System.Net.Dns.GetHostName();
				}
				catch (Exception)
				{
					return null;
				}
			}
		}


		/*
		** Name: RemoteHostName
		**
		** Description:
		**	Returns the host name of the remote machine.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Remote host name or NULL if error.
		**
		** History:
		**	12-Nov-03 (thoda04)
		**	    Created.
		*/

		public string RemoteHostName
		{
			get
			{
				try
				{
					System.Net.IPAddress ipAddress   = this.RemoteAddress;
					System.Net.IPHostEntry hostentry =
						System.Net.Dns.GetHostEntry(ipAddress);

					string hostname = hostentry.HostName;
					return hostname;
				}
				catch (Exception)
				{
					return null;
				}
			}
		}

		public InputStream getInputStream()
		{
			return new InputStream(base.GetStream());
		}


		/*
		** Name: QuerySocketHealth
		**
		** Description:
		**	Send a zero length message to test if socket is still connected.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Bool	Socket is Connected or not.
		**
		** History:
		**	19-Sep-07 (thoda04)
		**	    Created to test for severed socket into and out of connection pool.
		*/

		/// <summary>
		/// Query the underlying socket connection health to test if Connected.
		/// </summary>
		/// <returns>True if the underlying socket is Connected.</returns>
		public bool QuerySocketHealth()
		{
			if (Connected == false) // if socket already knows it's dead, then return
			   return false;

			bool blocking = this.Client.Blocking;  // save state of socket.Blocking

			try
			{
				// don't block execution if can't complete immediately
				this.Client.Blocking = false;

				byte[] emptymsg = new byte[1];
				Client.Send(emptymsg, 0,  // ping the socket with zero-length msg
					System.Net.Sockets.SocketFlags.None);
			}
			catch (Exception)
			{
			}
			finally
			{
				this.Client.Blocking = blocking; // restore state of socket.Blocking
			}

			return Connected;
		}

	}  // class MsgSocket
}