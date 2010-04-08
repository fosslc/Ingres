/*
** Copyright (c) 1999, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: msgout.cs
	**
	**	Defines class providing capability to send messages to 
	**	a Data Access Server.
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
	**	MsgOut    	Extends MsgIo.
	**	ByteSegOS 	Segmented byte OutputStream.
	**	Ucs2SegWtr	Segmented UCS2 Writer.
	**
	** History:
	**	 7-Jun-99 (gordy)
	**	    Created.
	**	10-Sep-99 (gordy)
	**	    Parameterized the initial TL connection data.
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	29-Sep-99 (gordy)
	**	    Added Segmentor class to process BLOBs as streams.  
	**	    Added methods (write(InputStream) and write(Reader)) 
	**	    to process BLOBs.  Moved check() functionality to begin().
	**	17-Nov-99 (gordy)
	**	    Extracted output functionality from DbConn.cs.
	**	21-Apr-00 (gordy)
	**	    Moved to package io.
	**	12-Apr-01 (gordy)
	**	    Pass tracing to exception.
	**	10-May-01 (gordy)
	**	    Added support for UCS2 datatypes.  Renamed segmentor class.
	**	31-May-01 (gordy)
	**	    Fixed UCS2 BLOB support.
	**	14-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	 6-Sep-02 (gordy)
	**	    Character encoding now encapsulated in CharSet class.
	**	20-Dec-02 (gordy)
	**	    Header ID protocol level dependent.  Communicate TL protocol
	**	    level to rest of messaging system.
	**	15-Jan-03 (gordy)
	**	    Added MSG layer ID connection parameter.
	**	25-Feb-03 (gordy)
	**	    CharSet now supports character arrays.
	**	 1-Dec-03 (gordy)
	**	    Support SQL data object values.  Added UCS2 segmentor class.
	**	15-Mar-04 (gordy)
	**	    Support SQL BIGINT and .NET long values.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Adding support for multiple server targets.
	**	 7-Dec-09 (gordy, ported by thoda04)
	**	    Added support for BOOLEAN data type.
	*/


	/*
	** Name: MsgOut
	**
	** Description:
	**	Class providing methods for sending messages to a Data
	**	Access Server.  Provides methods for building messages
	**	of the Message Layer of the Data Access Messaging protocol 
	**	(DAM-ML).  Drives the Transport Layer (DAM-TL) using the 
	**	buffered output class OutBuff to send TL  packets to the 
	**	server.
	**
	**	When sending messages, the caller must provide multi-
	**	threaded protection for each message.
	**
	**  Public Methods:
	**
	**	begin            	Start a new output message.
	**	write            	Append data to output message.
	**	writeUCS2        	Append char data as UCS2 to output message.
	**	done             	Finish the current output message.
	**
	**  Protected Methods:
	**
	**	connect			Connect to target server.
	**	sendCR			Send connection request.
	**	disconnect	Disconnect from the server.
	**	close		Close the server connection.
	**	setBuffSize	Set the I/O buffer size.
	**
	**  Private Data:
	**
	**	out		Output buffer.
	**	segOS			Segmented OutputStream.
	**	segWtr			Segmented Writer.
	**	out_msg_id	Current output message ID.
	**	out_hdr_pos	Message header buffer position.
	**	out_hdr_len	Size of message header.
	**	char_buff	Character buffer.
	**
	**  Private Methods:
	**
	**	writeSqlDataIndicator	Append the SQL data/NULL inidicator byte.
	**	split			Send current output message and continue.
	**
	** History:
	**	 7-Jun-99 (gordy)
	**	    Created.
	**	10-Sep-99 (gordy)
	**	    Parameterized the initial TL connection data.
	**	29-Sep-99 (gordy)
	**	    Added methods (write(InputStream) and write(Reader)) 
	**	    to process BLOBs.  Moved check() functionality to begin().
	**	17-Nov-99 (gordy)
	**	    Extracted output functionality from DbConn.
	**	10-May-01 (gordy)
	**	    Added write() methods for byte and character arrays and
	**	    writeUCS2() methods for UCS2 data.  Added character buffers,
	**	    ca1 and char_buff, for write() methods.
	**	 6-Sep-02 (gordy)
	**	    Character encoding now encapsulated in CharSet class.  Added
	**	    methods to return the encoded length of strings.
	**	 1-Oct-02 (gordy)
	**	    Renamed as DAM-ML messaging class.  Moved tl_proto and 
	**	    char_set here from super-class.
	**	20-Dec-02 (gordy)
	**	    Communicate TL protocol level to rest of messaging system
	**	    by adding setIoProtoLvl().  Moved tl_proto and char_set back
	**	    to super-class to localize all internal data.
	**	15-Mar-04 (gordy)
	**	    Added write methods for long and SqlBigInt values.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Extract connection functionality from constructor to
	**	    connect() and sendCR().
	**	 7-Dec-09 (gordy, ported by thoda04)
	**	    Added write() method for boolean.
	*/

	internal class MsgOut : MsgIo
	{
		/*
		** Output buffer and BLOB stream.
		*/
		private OutBuff    outBuff; // = null;
		private ByteSegOS  segOS;   // = null;
		private Ucs2SegWtr segWtr;  // = null;

		/*
		** Current message info
		*/
		private byte   out_msg_id;  // = 0;
		private int    out_hdr_pos; // = 0;
		private int    out_hdr_len; // = 0;
		private char[] char_buff = new char[8192];


		/*
		** Name: MsgOut
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	log		Trace log.
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
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Extracted connect() and sendCR() functionality.
		*/

		protected
		MsgOut()
		{
			title = "MsgOut[" + ConnID + "]";
		} // MsgOut


		/*
		** Name: connect
		**
		** Description:
		**	Connect to target server.  Initializes the output buffer.  
		**
		** Input:
		**	host	Host name or address.
		**	portID	Symbolic or numeric port ID.
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
		**	    Extracted output functionality from DbConn.
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Extracted from constructor.
		*/

	protected internal override void
	connect(String host, String portID)
	{
		base.connect( host, portID );

		try
		{
			// wrap the socket's NetworkStream in our OutputStream
			OutputStream outputStream = new OutputStream(socket.GetStream());
			outBuff = new OutBuff(outputStream, ConnID, 1 << DAM_TL_PKT_MIN);
		}
		catch (System.Exception ex)
		{
			if (trace.enabled(1))
				trace.write(title + ": error creating output buffer: " + ex.Message);
			disconnect();
			throw SqlEx.get(ERR_GC4001_CONNECT_ERR, ex);
		}

		return;
	} // connect


	/*
	** Name: sendCR
	**
	** Description:
	**	Sends the DAM-TL Connection Request packet including
	**	the DAM-ML connection parameters.
	**
	** Input:
	**	target  	Hostname and port.
	**	msg_cp    	DAM-ML connection parameters., may be NULL.
	**
	** Output:
	**	None.
	**
	** History:
	**	 7-Jun-99 (gordy)
	**	    Created.
	**	10-Sep-99 (gordy)
	**	    Parameterized the initial TL connection data.
	**	17-Nov-99 (gordy)
	**	    Extracted output functionality from DbConn.
	**	 1-Oct-02 (gordy)
	**	    Renamed as DAM Message Layer.  Removed connection info 
	**	    length parameter (use array length).  Added trace log 
	**	    parameter.
	**	15-Jan-03 (gordy)
	**	    Added MSG layer ID connection parameter.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Extracted from constructor.
	*/

	/// <summary>
	/// Sends the DAM-TL Connection Request packet including
	/// the DAM-ML connection parameters.
	/// </summary>
	/// <param name="msg_cp">DAM-ML connection parameters.</param>
	protected internal void
	sendCR(byte[] msg_cp)
		{
			try
			{
				if (trace.enabled(2))
					trace.write(title + ": open TL Connection");

				/*
				** Send the Connection Request message with our desired
				** protocol level and buffer size.
				*/
				outBuff.begin(DAM_TL_CR, 12);
				outBuff.write(DAM_TL_CP_PROTO);
				outBuff.write((byte) 1);
				outBuff.write(DAM_TL_PROTO_DFLT);
				outBuff.write(DAM_TL_CP_SIZE);
				outBuff.write((byte) 1);
				outBuff.write(DAM_TL_PKT_MAX);
				outBuff.write(DAM_TL_CP_MSG_ID);
				outBuff.write((byte) 4);
				outBuff.write(DAM_TL_MSG_ID);  // "DMML"

				if (msg_cp != null && msg_cp.Length > 0)
				{
					outBuff.write(DAM_TL_CP_MSG);
					
					if (msg_cp.Length < 255)
						outBuff.write((byte) msg_cp.Length);
					else
					{
						outBuff.write((byte) 255);
						outBuff.write((short) msg_cp.Length);
					}
					
					outBuff.write(msg_cp, 0, msg_cp.Length);
				}
				
				outBuff.flush();
			}
			catch (SqlEx)
			{
				if (trace.enabled(1))
					trace.write(title + ": error formatting/sending parameters");
				disconnect();
				throw;
			}
		}  // sendCR
		
		
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
		**	    Extracted output functionality from DbConn.
		*/

		/// <summary>
		/// Disconnect from server and free all I/O resources.
		/// </summary>
		protected internal override void  disconnect()
		{
			/*
			** We don't set the output buffer references to null
			** here so that we don't have to check it on each
			** use.  I/O buffer functions will continue to work
			** until a request results in a stream I/O request,
			** in which case an exception will be thrown by the
			** I/O buffer.
			**
			** We must, however, test the reference for null
			** since we may be called by the constructor with
			** a null output buffer.
			*/
			if (outBuff != null)
			{
				try
				{
					outBuff.close();
				}
				catch (Exception /* ignore */)
				{
				}
			}
			
			base.disconnect();
			return ;
		}  // disconnect


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
		**	    Extracted output functionality from DbConn.
		*/

		/// <summary>
		/// Close the connection with the server.
		/// </summary>
		protected internal virtual void  close()
		{
			if (trace.enabled(2))
				trace.write(title + ": close TL connection");

			try
			{
				outBuff.clear(); // Clear partial messages from buffer.
				outBuff.begin(DAM_TL_DR, 0);
				outBuff.flush();
			}
			catch (Exception /* ignore */)
			{
			}
			
			return ;
		}  // close
		
		
		/*
		** Name: BuffSize property
		**
		** Description:
		**	Set the I/O buffer size.
		**
		** Input:
		**	size	    Size of I/O buffer.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	17-Nov-99 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Set the I/O buffer size.
		/// </summary>
		protected internal virtual int BuffSize
		{
			set { outBuff.BuffSize = value; }
		} // BuffSize


		/*
		** Name: TL_ProtocolLevel property
		**
		** Description:
		**	Get/Set the DAM-TL protocol level.
		**
		** History:
		**	20-Dec-02 (gordy)
		**	    Created; Ported to .NET as a property (thoda04).
		*/

		/// <summary>
		/// Set the DAM-TL protocol level into the output buffer.
		/// </summary>
		public new byte TL_ProtocolLevel
		{
			get { return base.TL_ProtocolLevel; }
			set
			{
				base.TL_ProtocolLevel    = value;
				outBuff.TL_ProtocolLevel = value;
			}
		} // TL_ProtocolLevel


		/*
		** Name: begin
		**
		** Description:
		**	Begin a new message.  Builds a message header for
		**	an initially empty message.  The caller should lock
		**	this MsgConn object from the time this method is
		**	called until the request is complete.
		**
		** Input:
		**	msg_id	    Message ID.
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
		**	29-Sep-99 (gordy)
		**	    Replaced check() with inline test.
		**	20-Dec-02 (gordy)
		**	    Header ID protocol level dependent.
		*/

		/// <summary>
		/// Begin a new message.  Builds a message header for
		/// an initially empty message.  The caller should lock
		/// this MsgConn object from the time this method is
		/// called until the request is complete.
		/// </summary>
		/// <param name="msg_id">Message ID.</param>
		public virtual void  begin(byte msg_id)
		{
			if (isClosed())
				throw SqlEx.get( ERR_GC4004_CONNECTION_CLOSED );
			if (out_hdr_pos > 0)
				done(false);  /* Complete current message */
			
			if (trace.enabled(3))
				trace.write(title + ": begin message " +
				            IdMap.map(msg_id, MsgConst.msgMap));
			
			/*
			** Reserve space in output buffer for header.
			** Note that we don't do message concatenation
			** at this level.  We rely on messages being
			** concatenated by the output buffer if not
			** explicitly flushed.
			*/
			try
			{
				outBuff.begin(DAM_TL_DT, 8); /* Begin new message */
				out_msg_id = msg_id;
				out_hdr_pos = outBuff.position();
				
				outBuff.write( (ML_ProtocolLevel >= MSG_PROTO_3)
					? MSG_HDR_ID_2    // "DMML"  Eyecatcher
					: MSG_HDR_ID_1 ); // "JDBC"
				outBuff.write((short) 0); /* Data length */
				outBuff.write(msg_id); /* Message ID */
				outBuff.write(MSG_HDR_EOD); /* Flags */
				
				out_hdr_len = outBuff.position() - out_hdr_pos;
			}
			catch (SqlEx ex)
			{
				disconnect();
				throw ex;
			}
			
			return ;
		}  // begin
		
		/*
		** Name: write (byte)
		**
		** Description:
		**	Append a byte value to current output message.  If buffer
		**	overflow would occur, the message is split.
		**
		** Input:
		**	valueByte	    The byte value to be appended.
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

		/// <summary>
		/// Append a byte value to current output message.  If buffer
		/// overflow would occur, the message is split.
		/// </summary>
		/// <param name="valueByte"></param>
		public virtual void  write(byte valueByte)
		{
			if (outBuff.avail() < 1)
				split();
			outBuff.write(valueByte);
			return ;
		}  // write

		public virtual void write(SByte valueByte)
		{
			write(unchecked((byte)valueByte));
		}  // write

		/*
		** Name: write (short)
		**
		** Description:
		**	Append a short value to current output message.  If buffer
		**	overflow would occur, the message is split.
		**
		** Input:
		**	valueShort	    The short value to be appended.
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

		/// <summary>
		/// Append a short value to current output message.  If buffer
		/// overflow would occur, the message is split.
		/// </summary>
		/// <param name="valueShort">The short value to be appended.</param>
		public virtual void  write(short valueShort)
		{
			if (outBuff.avail() < 2)
				split();
			outBuff.write(valueShort);
			return ;
		}  // write
		

		/*
		** Name: write (int)
		**
		** Description:
		**	Append an integer value to current output message.  If buffer
		**	overflow would occur, the message is split.
		**
		** Input:
		**	valueInt	    The integer value to be appended.
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

		/// <summary>
		/// Append an integer value to current output message.  If buffer
		/// overflow would occur, the message is split.
		/// </summary>
		/// <param name="valueInt">The integer value to be appended.</param>
		public virtual void  write(int valueInt)
		{
			if (outBuff.avail() < 4)
				split();
			outBuff.write(valueInt);
			return ;
		}  // write


		/*
		** Name: write
		**
		** Description:
		**	Append a long value to current output message.  If buffer
		**	overflow would occur, the message is split.
		**
		** Input:
		**	value	    The long value to be appended.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	15-Mar-04 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Append a long 64-bit value to current output message.  If buffer
		/// overflow would occur, the message is split.
		/// </summary>
		/// <param name="value"></param>
		public void
			write( long value )
		{
			if ( outBuff.avail() < 8 )  split();
			outBuff.write( value );
			return;
		} // write


		/*
		** Name: write (float)
		**
		** Description:
		**	Append a float value to current output message.  If buffer
		**	overflow would occur, the message is split.
		**
		** Input:
		**	valueFloat	    The float value to be appended.
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

		/// <summary>
		/// Append a float value to current output message.  If buffer
		/// overflow would occur, the message is split.
		/// </summary>
		/// <param name="valueFloat">The float value to be appended.</param>
		public virtual void  write(float valueFloat)
		{
			if (outBuff.avail() < DAM_TL_F4_LEN)
				split();
			outBuff.write(valueFloat);
			return ;
		}  // write


		/*
		** Name: write (double)
		**
		** Description:
		**	Append a double value to current output message.  If buffer
		**	overflow would occur, the message is split.
		**
		** Input:
		**	valueDouble	    The double value to be appended.
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

		/// <summary>
		/// Append a double value to current output message.  If buffer
		/// overflow would occur, the message is split.
		/// </summary>
		/// <param name="valueDouble">The double value to be appended.</param>
		public virtual void  write(double valueDouble)
		{
			if (outBuff.avail() < DAM_TL_F8_LEN)
				split();
			outBuff.write(valueDouble);
			return ;
		}
		// write


		/*
		** Name: write (byte[])
		**
		** Description:
		**	Append a byte array to current output message.  If buffer
		**	overflow would occur, the message (and array) is split.
		**	The byte array is preceded by a two byte length indicator.
		**
		** Input:
		**	byteArray	    The byte array to be appended.
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
		**	22-Sep-99 (gordy)
		**	    Send entire array, preceded by the length.
		*/

		/// <summary>
		/// Append a byte array to current output message.  If buffer
		/// overflow would occur, the message (and array) is split.
		/// </summary>
		/// <param name="byteArray">The byte array to be appended.</param>
		public virtual void  write(byte[] byteArray)
		{
			write((short) byteArray.Length);
			write(byteArray, 0, byteArray.Length);
			return ;
		}  // write


		/*
		** Name: write (byte[])
		**
		** Description:
		**	Append a byte sub-array to current output message.  If buffer
		**	overflow would occur, the message (and array) is split.
		**
		** Input:
		**	byteArray	    The byte array containing sub-array to be appended.
		**	offset   	    Start of sub-array.
		**	length   	    Length of sub-array.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	10-May-01 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Append a byte sub-array to current output message.  If buffer
		/// overflow would occur, the message (and array) is split.
		/// </summary>
		/// <param name="byteArray">The byte array containing sub-array
		/// to be appended.</param>
		/// <param name="offset">Start of sub-array.</param>
		/// <param name="length">Length of sub-array.</param>
		public virtual void  write(byte[] byteArray, int offset, int length)
		{
			 for (int end = offset + length; offset < end; )
			{
				if (outBuff.avail() <= 0)
					split();
				length = System.Math.Min(outBuff.avail(), end - offset);
				outBuff.write(byteArray, offset, length);
				offset += length;
			}
			
			return ;
		}  // write


		/*
		** Name: write
		**
		** Description:
		**	Append a ByteArray to current output message.  If buffer
		**	overflow would occur, the message (and array) is split.
		**
		** Input:
		**	value	    The ByteArray to be appended.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(IByteArray bytes)
		{
			int end = bytes.valuelength();

			for( int offset = 0; offset < end;  )
			{
			if ( outBuff.avail() <= 0 )  split();
			int len = Math.Min( outBuff.avail(), end - offset );
			len = outBuff.write(bytes, offset, len);
			offset += len;
			}

			return;
		} // write


		/*
		** Name: write (string)
		**
		** Description:
		**	Append a string value to current output message.  If buffer
		**	overflow would occur, the message (and string) is split.
		**	The string is preceded by a two byte length indicator.
		**
		** Input:
		**	valueString	    The string value to be appended.
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
		**	22-Sep-99 (gordy)
		**	    Added character set/encoding.
		**	 6-Sep-02 (gordy)
		**	    Character encoding now encapsulated in CharSet class.
		*/

		/// <summary>
		/// Append a string value to current output message.  If buffer
		/// overflow would occur, the message (and string) is split.
		/// </summary>
		/// <param name="valueString"></param>
		public virtual void  write(String valueString)
		{
			byte[] ba;
			
			try { ba = getCharSet().getBytes( valueString ); }
			catch( Exception ex )  // Should not happen!
			{
				throw SqlEx.get(
					title + ": character encoding failed.  " + ex.Message,
					"HY000", 0);
			}

			write( ba );
			return;
		}  // write


		/*
		** Name: writeUCS2
		**
		** Description:
		**	Append a character sub-array to current output message.  
		**	If buffer overflow would occur, the message (and array) 
		**	is split.
		**
		** Input:
		**	value	    The character array with sub-array to be appended.
		**	offset	    Start of sub-array.
		**	length	    Length of sub-array.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	10-May-01 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Append a character sub-array to current output message. 
		/// If buffer overflow would occur, the message (and array) is split.
		/// </summary>
		/// <param name="charArray">The character array with sub-array
		/// to be appended.</param>
		/// <param name="offset">Start of sub-array.</param>
		/// <param name="length">Length of sub-array.</param>
		public virtual void  writeUCS2(char[] charArray, int offset, int length)
		{
			 for (int end = offset + length; offset < end; offset++)
				write((short) charArray[offset]);
			return ;
		}  // writeUCS2


		/*
		** Name: writeUCS2
		**
		** Description:
		**	Append a character sub-array to current output message.  
		**	If buffer overflow would occur, the message (and array) 
		**	is split.
		**
		** Input:
		**	value	    The character array with sub-array to be appended.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		writeUCS2(ICharArray array)
		{
			int end = array.valuelength();

			for (int position = 0; position < end; position++)
				write((short)array.get(position));
			return;
		} // writeUCS2


		/*
		** Name: write
		**
		** Description:
		**	Writes a SqlNull data value to the current input message.
		**
		**	A SqlNull data value is composed of simply a data indicator
		**	byte.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlNull value)
		{
			if (writeSqlDataIndicator(value))	// Write data indicator byte.
				throw SqlEx.get(ERR_GC4002_PROTOCOL_ERR);	// Should be NULL.
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlTinyInt data value to the current input message.
		**
		**	A SqlTinyInt data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a one byte integer.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlTinyInt value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlSmallInt data value to the current input message.
		**
		**	A SqlSmallInt data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a two byte integer.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlSmallInt value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlInt data value to the current input message.
		**
		**	A SqlInt data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a four byte integer.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlInt value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlBigInt data value to the current input message.
		**
		**	A SqlBigInt data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by an eight byte integer.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	15-Mar-04 (gordy)
		**	    Created.
		*/

		public void
		write(SqlBigInt value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlReal data value to the current input message.
		**
		**	A SqlReal data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a four byte floating
		**	point value.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlReal value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlDouble data value to the current input message.
		**
		**	A SqlDouble data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a eight byte floating
		**	point value.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlDouble value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlDecimal data value to the current input message.
		**
		**	A SqlDecimal data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlDecimal value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlBool data value to the current input message.
		**
		**	A SqlBool data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a one byte integer.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 7-Dec-09 (gordy, ported by thoda04)
		**	    Created.
		*/

		public void
		write(SqlBool value)
		{
			if (writeSqlDataIndicator(value)) write((byte)(value.get() ? 1 : 0));
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlDate data value to the current input message.
		**
		**	A SqlDate data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		write(SqlDate value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlTime data value to the current input message.
		**
		**	A SqlTime data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		write(SqlTime value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlTimestamp data value to the current input message.
		**
		**	A SqlTimestamp data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		write(SqlTimestamp value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a IngresDate data value to the current input message.
		**
		**	A IngresDate data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(IngresDate value)
		{
			if (writeSqlDataIndicator(value)) write(value.get());
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlByte data value to the current input message.
		**	Uses the ByteArray interface to send the SqlByte data value.
		**
		**	A SqlByte data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a binary byte array
		**	whose length is detemined by the length of the SqlByte data
		**	value.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlByte value)
		{
			if (writeSqlDataIndicator(value)) write((IByteArray)value);
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlVarByte data value to the current input message.
		**	Uses the ByteArray interface to send the SqlVarByte data value.
		**
		**	A SqlVarByte data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a two byte integer length
		**	which is followed by a binary byte array of the indicated length.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlVarByte value)
		{
			if (writeSqlDataIndicator(value))	// Data indicator byte.
			{
				write((short)value.valuelength());		// Array length.
				write((IByteArray)value);		// Data array
			}
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlChar data value to the current input message.
		**	Uses the ByteArray interface to send the SqlChar data value.
		**
		**	A SqlChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a character byte array
		**	whose length is detemined by the length of the SqlChar data 
		**	value.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlChar value)
		{
			if (writeSqlDataIndicator(value)) write((IByteArray)value);
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlVarChar data value to the current input message.
		**	Uses the ByteArray interface to send the SqlVarChar data value.
		**
		**	A SqlVarChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a two byte integer length
		**	which is followed by a character byte array of the indicated length.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlVarChar value)
		{
			if (writeSqlDataIndicator(value))	// Data indicator byte.
			{
				write((short)value.valuelength());		// Array length.
				write((IByteArray)value);		// Data array
			}
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlNChar data value to the current input message.
		**	Uses the CharArray interface to send the SqlNChar data value.
		**
		**	A SqlNChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a UCS2 (two byte
		**	integer) array whose length is detemined by the length of 
		**	the SqlNChar data value.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlNChar value)
		{
			if (writeSqlDataIndicator(value)) writeUCS2((ICharArray)value);
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlNVarChar data value to the current input message.
		**	Uses the CharArray interface to send the SqlNVarChar data value.
		**
		**	A SqlNVarChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a two byte integer length
		**	which is followed by a UCS2 (two byte integer) array of the 
		**	indicated length.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlNVarChar value)
		{
			if (writeSqlDataIndicator(value))	// Data indicator byte.
			{
				write((short)value.valuelength());		// Array length.
				writeUCS2((ICharArray)value);		// Data array
			}
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlLongByte data value to the current input message.
		**	The binary stream is retrieved and segmented for output by 
		**	an instance of the local inner class ByteSegOS.
		**
		**	A SqlLongByte data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by zero or more segments
		**	which are then followed by and end-of-segments indicator.  Each
		**	segment is composed of a two byte integer length (non-zero) 
		**	followed by a binary byte array of the indicated length.  The
		**	end-of-segments indicator is a two byte integer with value
		**	zero.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlLongByte value)
		{
			if (writeSqlDataIndicator(value))	// Data indicator byte.
			{
				if (segOS == null) segOS = new ByteSegOS(this, outBuff, trace);
				segOS.begin(out_msg_id);		// begin a new BLOB.
				try { value.get(segOS); }		// Write stream to output.
				finally { segOS.end(); }		// End-of-segments.
			}
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlLongChar data value to the current input message.
		**	The byte stream is retrieved and segmented for output by an
		**	instance of the local inner class ByteSegOS.
		**
		**	A SqlLongChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by zero or more segments
		**	which are then followed by and end-of-segments indicator.  Each
		**	segment is composed of a two byte integer length (non-zero) 
		**	followed by a character byte array of the indicated length.  
		**	The end-of-segments indicator is a two byte integer with value
		**	zero.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlLongChar value)
		{
			if (writeSqlDataIndicator(value))	// Data indicator byte.
			{
//				if (segOS == null) segOS = new ByteSegOS();
				if (segOS == null)
					segOS = new ByteSegOS(this, outBuff, trace);
				segOS.begin(out_msg_id);		// begin a new BLOB.
				try { value.get(segOS); }		// Write stream to output.
				finally { segOS.end(); }		// End-of-segments.
			}
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a SqlLongNChar data value to the current input message.
		**	The character stream is retrieved and segmented for output by
		**	an instance of the local inner class Ucs2SegWtr.
		**
		**	A SqlLongNChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by zero or more segments
		**	which are then followed by an end-of-segments indicator.  Each
		**	segment is composed of a two byte integer length (non-zero) 
		**	followed by a UCS2 (two byte integer) array of the indicated 
		**	length.  The end-of-segments indicator is a two byte integer 
		**	with value zero.
		**
		** Input:
		**	value	SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		write(SqlLongNChar value)
		{
			if (writeSqlDataIndicator(value))	// Data indicator byte.
			{
				if (segWtr == null) segWtr = new Ucs2SegWtr(this, outBuff, trace);
				segWtr.begin(out_msg_id);		// begin a new BLOB.
				try { value.get(segWtr); }		// Write stream to output.
				finally { segWtr.end(); }		// End-of-segments.
			}
			return;
		} // write


		/*
		** Name: writeSqlDataIndicator
		**
		** Description:
		**	Writes a single byte (the SQL data/NULL indicator)
		**	to the current message and returns True if the data 
		**	value is not-NULL.
		**
		** Input:
		**	value		SQL data value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean		True if data value is not-NULL.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		private bool
		writeSqlDataIndicator(SqlData data)
		{
			bool isNull = data.isNull();
			write((byte)(isNull ? 0 : 1));
			return (!isNull);
		} // writeSqlNullIndicator


		/*
		** Name: done
		**
		** Description:
		**	Finish the current message and optionally send
		**	it to the server.
		**
		** Input:
		**	flush	    True if message should be sent to server.
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

		/// <summary>
		/// Finish the current message and optionally send
		/// it to the server.
		/// </summary>
		/// <param name="flush">True if message should be sent to server.</param>
		public virtual void  done(bool flush)
		{
			if (out_hdr_pos > 0)
			{
				// Update the message header.
				short length = (short) (outBuff.position() -
				                        (out_hdr_pos + out_hdr_len));
				
				if (trace.enabled(2))
					trace.write(title + ": sending message " +
					            IdMap.map(out_msg_id, MsgConst.msgMap) +
					            " length " + length +
					            " EOD" + (flush?" EOG":""));
				
				outBuff.write(out_hdr_pos + 4, length);
				outBuff.write(out_hdr_pos + 7,
				             (byte) (MSG_HDR_EOD | 
				                     (flush?MSG_HDR_EOG:(byte)0)));
			}
			
			if (flush)
				try
				{
					outBuff.flush();
				}
				catch (SqlEx ex)
				{
					disconnect();
					throw ex;
				}
			
			out_msg_id = 255;
			out_hdr_pos = 0;
			out_hdr_len = 0;
			
			return ;
		}  // done


		/*
		** Name: split
		**
		** Description:
		**	The current output message is sent to the server and 
		**	a message continuation is initialized.
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
		*/

		/// <summary>
		/// The current output message is sent to the server and
		/// a message continuation is initialized.
		/// </summary>
		private void  split()
		{
			if (out_hdr_pos > 0)
			{
				short length =
					(short) (outBuff.position() - (out_hdr_pos + out_hdr_len));
				
				if (trace.enabled(3))
					trace.write(title + ": splitting message " +
						IdMap.map(out_msg_id, MsgConst.msgMap) +
						" length " + length);
				
				// Update the message header.
				try
				{
					outBuff.write(out_hdr_pos + 4, length);
					outBuff.write(out_hdr_pos + 7, (byte) 0);
					outBuff.flush();
					
					outBuff.begin(DAM_TL_DT, 8); /* Begin continued message */
					out_hdr_pos = outBuff.position();
					outBuff.write( (ML_ProtocolLevel >= MSG_PROTO_3)
						? MSG_HDR_ID_2    // "DMML"  Eyecatcher
						: MSG_HDR_ID_1 ); // "JDBC"
					outBuff.write((short) 0); /* Data length */
					outBuff.write(out_msg_id); /* Message ID */
					outBuff.write(MSG_HDR_EOD); /* Flags */
					out_hdr_len = outBuff.position() - out_hdr_pos;
				}
				catch (SqlEx ex)
				{
					disconnect();
					throw ex;
				}
			}
			
			return ;
		}  // split


		/*
		** Name: ByteSegOS
		**
		** Description:
		**	Class for converting byte stream into segmented output stream.
		**
		**	An OutputStreamWriter object is required for conversion of 
		**	Unicode BLOBs into server character encoded byte streams.  
		**	OutputStreamWriter requires an OutputStream to process the
		**	resulting byte stream.  This class extends OutputStream to
		**	provide segmentation of the converted BLOB, bufferring the
		**	stream as needed.
		**
		**	This class is designed for repeated usage.  The begin() and
		**	end() methods provide the means for a single class instance
		**	to process multiple serial BLOBs on the same connection.  Note
		**	that the close method does not actually close the OS object,
		**	but simply ends the current BLOB output stream (same as end()).
		**
		**  Public Methods
		**
		**	begin   	Initialize processing of a BLOB.
		**	write   	Write bytes in BLOB stream.
		**	close   	Close current BLOB stream.
		**	end     	Complete processing of a BLOB.
		**
		**  Private Data
		**
		**	title   	Class title for tracing.
		**	trace   	Tracing.
		**	dbc_out 	Db Connection output.
		**	out     	Bufferred output stream.
		**	msg_id  	Message ID of message containing the BLOB.
		**	li_pos  	Position of segment length indicator in output buffer.
		**	ba      	Temporary storage for writing single byte.
		**
		**  Private Methods:
		**
		**	startSegment	Start a new data segment.
		**
		** History:
		**	29-Sep-99 (gordy)
		**	    Created.
		**	17-Nov-99 (gordy)
		**	    Made class nested (inner).
		**	10-May-01 (gordy)
		**	    Class renamed to match changes in DbConnIn.  Made class 
		**	    static and added dbc_out for access to DB connection.
		*/

		/// <summary>
		/// Class for converting byte stream into segmented output stream.
		/// An OutputStreamWriter object is required for conversion of
		/// Unicode BLOBs into server character encoded byte streams. 
		/// OutputStreamWriter requires an IO.Stream to process the
		/// resulting byte stream.  This class extends IO.Stream to
		/// provide segmentation of the converted BLOB, bufferring the
		/// stream as needed.
		/// </summary>

		private class ByteSegOS : OutputStream
		{
			private String title; // Class title for tracing.
			private ITrace trace;        // Tracing.
			
			private MsgOut msg_out;      // Db Connection output.
			private OutBuff outBuff = null; // Buffered output stream.
			private byte msg_id = 255;      // Current message ID.
			private int li_pos    = -1; // Position of length indicator.

			private byte[] ba;          // Temp storage.
			
			
			/*
			** Name: ByteSegOS
			**
			** Description:
			**	Class constructor.
			**
			** Input:
			**	msg_out	Message layer output.
			**	outBuff	Buffered output stream.
			**	trace  	Tracing.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	None.
			**
			** History:
			**	29-Sep-99 (gordy)
			**	    Created.
			**	17-Nov-99 (gordy)
			**	    Removed msg parameter when class nested.
			**	10-May-01 (gordy)
			**	    Added msg_out parameter when class made static.*/

			/// <summary>
			/// Class constructor for converting byte stream into
			/// segmented output stream.
			/// </summary>
			/// <param name="msg_out">Message layer output.</param>
			/// <param name="outBuff">Buffered output stream.</param>
			/// <param name="trace">Tracing</param>
			public ByteSegOS(MsgOut msg_out, OutBuff outBuff, ITrace trace)
				: base(outBuff.outputStream)
			{
				title = "ByteSegOS[" + msg_out.ConnID + "]";
				this.trace = trace;
				this.msg_out = msg_out;
				this.outBuff = outBuff;

				ba = new byte[1];
			}  // BytesSegOS


			/*
			** Name: begin segment
			**
			** Description:
			**	Initialize the segmentor for a new series of BLOB segments.
			**	The current message ID must be provided since segmentation 
			**	of the BLOB may require the current message to be completed 
			**	and a new message of the same type to be started.
			**
			** Input:
			**	msg_id	    ID of current message.
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
			**	 1-Dec-03 (gordy)
			**	    Use startSegment() to initialize segment.
			*/

			/// <summary>
			/// Initialize the segmentor for a new series of BLOB segments.
			/// The current message ID must be provided since segmentation
			/// of the BLOB may require the current message to be completed
			/// and a new message of the same type to be started.
			/// </summary>
			/// <param name="msg_id">ID of current message.</param>
			public virtual void
			begin(byte msg_id)
			{
				this.msg_id = msg_id;
				li_pos = - 1;

				if (trace.enabled(3))
					trace.write(title + ": start of BLOB");

				startSegment();
				return;
			}  // begin


			/*
			** Name: Write (int) into segment
			**
			** Description:
			**	Write a single byte to the segment stream.
			**
			** Input:
			**	b	Byte to be output.
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
			*/

			/// <summary>
			/// Write a single byte to the segment stream.
			/// </summary>
			/// <param name="b"></param>
			public  void  Write(int b)
			{
				ba[0] = (byte) b;
				Write(ba, 0, 1);
				return ;
			}  // Write


			/*
			** Name: Write
			**
			** Description:
			**	Write a byte array to the segment stream.
			**
			** Input:
			**	bytes	Byte array to be output.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	void.
			**
			** History:
			**	 1-Dec-03 (gordy)
			**	    Created.
			*/

			public void
			Write(byte[] bytes)
			{
				Write(bytes, 0, bytes.Length);
				return;
			} // Write


			/*
			** Name: Write byte[] into segment
			**
			** Description:
			**	Write a byte sub-array to the segment stream.  The remainder
			**	of the current message buffer is used to build a segment.
			**	When the buffer is filled, the current message is completed
			**	and a new message of the same type is started.  A non-zero
			**	length segment is always left in the current output buffer.
			**
			** Input:
			**	b     	Byte array.
			**	offset	Starting byte to be output.
			**	length	Number of bytes to be output.
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
			**	12-Apr-01 (gordy)
			**	    Pass tracing to exception.
			**	 1-Dec-03 (gordy)
			**	    Moved segment handling to startSegment().
			*/

			/// <summary>
			/// Write a byte sub-array to the segment stream.  The remainder
			/// of the current message buffer is used to build a segment.
			/// When the buffer is filled, the current message is completed
			/// and a new message of the same type is started.  A non-zero
			/// length segment is always left in the current output buffer.
			/// </summary>
			/// <param name="bytes">Byte array.</param>
			/// <param name="offset">Starting byte to be output.</param>
			/// <param name="length">Number of bytes to be output.</param>
			public override void Write(byte[] bytes, int offset, int length)
			{
				try
				{
					for( int end = offset + length; offset < end; offset += length )
					{
						/*
						** A data segment requires at least three bytes
						** of space: two for the length indicator and
						** one for the data.
						*/
						if ( outBuff.avail() < 3 )  startSegment();
						length = Math.Min( outBuff.avail(), end - offset );
						outBuff.write( bytes, offset, length );
					}
				}
				catch (SqlEx ex)
				{
					if (trace.enabled(1))
					{
						trace.write(title + ": exception writing data");
						ex.trace(trace);
					}
					
					msg_out.disconnect();
					throw; // new IOException(ex.Message);
				}
				
				return ;
			}  // Write


			/*
			** Name: Close segment
			**
			** Description:
			**	Close the output stream.
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
			**	12-Apr-01 (gordy)
			**	    Pass tracing to exception.
			*/

			/// <summary>
			/// Close the output stream.
			/// </summary>
			public override void  Close()
			{
				try { end(); }
				catch (SqlEx ex)
				{
					if (trace.enabled(1))
					{
						trace.write(title + ": exception writing end-of-segments");
						ex.trace(trace);
					}
					
					msg_out.disconnect();
					throw; // new IOException(ex.Message);
				}
				
				return ;
			}  // Close


			/*
			** Name: end segment
			**
			** Description:
			**	End the current output stream of segments.  The end-
			**	of-segments indicator is appended to the output buffer.
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
			**	 1-Dec-03 (gordy)
			**	    Starting a new segment results in 0 length segment
			**	    which is the end-of-segment indicator.
			*/

			/// <summary>
			/// End the current output stream of segments.  The
			/// end-of-segments indicator is appended to the output buffer.
			/// </summary>
			public virtual void  end()
			{
				if (msg_id != 255)
				{
					/*
					** Clean-up any prior segment.  The temporary
					** length indicator generated by startSegment()
					** is exactly what is needed as end-of-segment
					** indicator.
					*/
					startSegment();
					
					msg_id  = 255;
					li_pos  =  -1;

					if (trace.enabled(3)) trace.write(title + ": end of BLOB");
				}
				
				return ;
			}  // end


			/*
			** Name: startSegment
			**
			** Description:
			**	Start a data segment.  A temporary zero length indicator
			**	(end-of-segments) is written as a place holder for the 
			**	new segment.  If a prior segment exists, the prior segment 
			**	length indicator is updated.
			**
			**	The current message is split if necessary.
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
			**	 1-Dec-03 (gordy)
			**	    Created.
			*/

			private void
			startSegment()
			{ 
				if ( li_pos >= 0 )
				{
				/*
				** Determine current segment length, allowing
				** for size of segment length indicator.
				*/
				int length = (outBuff.position() - li_pos) - 2;
				
				/*
				** If current segment is empty, use it as the
				** new segment since empty data segments are 
				** not permitted.
				*/
				if ( length <= 0 )  return;
				
				/*
				** Update BLOB segment length indicator.
				*/
				if ( trace.enabled( 4 ) )
					trace.write( title + ": BLOB segment length " + length );
				outBuff.write( li_pos, (short)length );
				}

				/*
				** If there isn't sufficient room in the current
				** message for a data segment, next segment will
				** need to be in next message segment.
				*/
				if ( outBuff.avail() < 3 )
				{
					msg_out.done( false );    // Finish current message
					msg_out.begin(msg_id);    // Begin new message of same type.
				}

				/*
				** Save the current length indicator position 
				** so that it may be updated later.  Write an
				** end-of-segments (zero length) indicator as
				** a place holder.
				*/
				li_pos = outBuff.position();
				outBuff.write( (short)0 );
				return;
			} // startSegment


			/*
			** Name: miscellaneous properties and methods for ByteSegOS stream
			**
			** Description:
			**	System.IO.Stream abstract class has properties and 
			**	methods that need not be implemented for the ByteSegOS
			**	output stream but do need to be defined.
			**
			** History:
			**	19-Aug-02 (thoda04)
			**	    Created for .NET Provider.
			*/
			
			public override bool CanRead
			{
				get { return false; }
			}  // CanRead

			public override bool CanSeek
			{
				get { return false; }
			}  // CanSeek

			public override bool CanWrite
			{
				get { return true; }
			}  // CanWrite

			public override long Length
			{
				get { throw new NotSupportedException(
				         "ByteSegOS stream is a write-only stream");
				    }
			}  // Length

			public override long Position
			{
				get
				{
					throw new NotSupportedException(
						  "ByteSegOS stream does not support seeking");
				}
				set
				{
					throw new NotSupportedException(
						"ByteSegOS stream does not support seeking");
				}
			}  // Position

			public override void Flush()
			{
			}  // Flush

			public override int Read(byte[] byteArray, int offset, int count)
			{
				throw new NotSupportedException(
					"ByteSegOS stream does not support reading");
			}  // Read

			public override long Seek(long offset, System.IO.SeekOrigin origin)
			{
				throw new NotSupportedException(
					"ByteSegOS stream does not support seek");
			}  // Seek

			public override void SetLength(long length)
			{
				throw new NotSupportedException(
					"ByteSegOS stream does not support seek");
			}  // SetLength

		}  // class ByteSegOS


		/*
		** Name: Ucs2SegWtr
		**
		** Description:
		**	Class for converting character stream into segmented writer
		**	stream.
		**
		**	This class is designed for repeated usage.  The begin() and 
		**	end() methods provide the means for a single class instance 
		**	to process multiple serial BLOBs on the same connection.  
		**
		**	Note that the close method does not actually close the Writer 
		**	object, but simply ends the current BLOB output stream (same 
		**	as end()).
		**
		**  Public Methods
		**
		**	begin		Initialize processing of a BLOB.
		**	write		Write bytes in BLOB stream.
		**	flush		Flush output.
		**	close		Close current BLOB stream.
		**	end		Complete processing of a BLOB.
		**
		**  Private Data
		**
		**	title		Class title for tracing.
		**	msg_id		Message ID of message containing the BLOB.
		**	li_pos		Position of segment length indicator in output buffer.
		**
		**  Private Methods:
		**
		**	startSegment	Start a new data segment.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		**	 1-Oct-06 (thoda04)
		**	    Ported to .NET
		*/

		private class
		Ucs2SegWtr
			: Writer
		{

			private String	title;            // Class title for tracing.
			private ITrace trace;             // Tracing.

			private MsgOut  msg_out;          // Db Connection output.
			private OutBuff outBuff = null;   // Buffered output stream.
			private byte    msg_id = 255;     // Current message ID.
			private int     li_pos = -1;      // Position of length indicator.


		/*
		** Name: Ucs2SegWtr
		**
		** Description:
		**	Class constructor.
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
		**	 1-Dec-03 (gordy)
		**	    Created.
		**	 1-Oct-06 (thoda04)
		**	    Ported to .NET
		*/

		public
		Ucs2SegWtr(MsgOut msg_out, OutBuff outBuff, ITrace trace)
		{
			title = "Ucs2SegWtr[" + msg_out.ConnID + "]";

			this.trace = trace;
			this.msg_out = msg_out;
			this.outBuff = outBuff;
		} // Ucs2SegWtr


		/*
		** Name: begin
		**
		** Description:
		**	Initialize the segmentor for a new series of BLOB segments.
		**	The current message ID must be provided since segmentation 
		**	of the BLOB may require the current message to be completed 
		**	and a new message of the same type to be started.
		**
		** Input:
		**	msg_id	    ID of current message.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		begin( byte msg_id )
		{
			this.msg_id = msg_id;
			li_pos = -1;
		    
			if ( trace.enabled( 3 ) )  trace.write( title + ": start of CLOB" );
			startSegment();
			return;
		}


		/*
		** Name: Write
		**
		** Description:
		**	Write a char sub-array to the segment stream.  The remainder
		**	of the current message buffer is used to build a segment.
		**	When the buffer is filled, the current message is completed
		**	and a new message of the same type is started.
		**
		** Input:
		**	chars	Char array.
		**	offset	Starting char to be output.
		**	length	Number of chars to be output.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public override void
		Write( char[] chars, int offset, int length )
		{
			try
			{
			for( int end = offset + length; offset < end; offset += length )
			{
				/*
				** A data segment requires at least four bytes
				** of space: two for the length indicator and
				** two for the data.
				*/
				if ( outBuff.avail() < 4 )  startSegment();
				length = Math.Min( outBuff.avail() / 2, end - offset );
				this.msg_out.writeUCS2( chars, offset, length );
			}
			}
			catch( SqlEx ex )
			{ 
			if ( trace.enabled( 1 ) )
			{
				trace.write( title + ": exception writing data" );
				ex.trace( trace );
			}

			this.msg_out.disconnect();
			throw; // new IOException(ex.Message);
			}

			return;
		} // Write


		/*
		** Name: Flush
		**
		** Description:
		**	Flush the output stream.
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
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public override void
		Flush()
		{
			return;	// Nothing to do!
		} // Flush


		/*
		** Name: Close
		**
		** Description:
		**	Close the output stream.
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
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public override void
		Close()
		{
			try { end(); }
			catch( SqlEx ex )
			{ 
			if ( trace.enabled( 1 ) )
			{
				trace.write( title + ": exception writing end-of-segments" );
				ex.trace( trace );
			}

			this.msg_out.disconnect();
			throw; // new IOException(ex.Message);
			}

			return;
		}


		/*
		** Name: end
		**
		** Description:
		**	End the current output stream of segments.  The end-
		**	of-segments indicator is appended to the output buffer.
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
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public void
		end()
		{
			if ( msg_id != 255 )
			{
			/*
			** Clean-up any prior segment.  The temporary
			** length indicator generated by startSegment()
			** is exactly what is needed as end-of-segment
			** indicator.
			*/
			startSegment();
			msg_id = 255;
			li_pos = -1;

			if ( trace.enabled( 3 ) )  trace.write( title + ": end of CLOB" );
			}

			return;
		} // end


		/*
		** Name: startSegment
		**
		** Description:
		**	Start a data segment.  A temporary zero length indicator
		**	(end-of-segments) is written as a place holder for the 
		**	new segment.  If a prior segment exists, the prior segment 
		**	length indicator is updated.
		**
		**	The current message is split if necessary.
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
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		private void
		startSegment()
		{
			if ( li_pos >= 0 )
			{
			/*
			** Determine current segment length, allowing
			** for size of segment length indicator.
			*/
			int length = ((outBuff.position() - li_pos) - 2) / 2;
			
			/*
			** If current segment is empty, use it as the
			** new segment since empty data segments are 
			** not permitted.
			*/
			if ( length <= 0 )  return;
			
			/*
			** Update BLOB segment length indicator.
			*/
			if ( trace.enabled( 4 ) )
				trace.write( title + ": CLOB segment length " + length );
			outBuff.write( li_pos, (short)length );
			}

			/*
			** If there isn't sufficient room in the current
			** message for a data segment, next segment will
			** need to be in next message segment.
			*/
			if ( outBuff.avail() < 4 )
			{
				this.msg_out.done( false );	    // Finish current message
				this.msg_out.begin( msg_id );    // Begin new message of same type.
			}

			/*
			** Save the current length indicator position 
			** so that it may be updated later.  Write an
			** end-of-segments (zero length) indicator as
			** a place holder.
			*/
			li_pos = outBuff.position();
			outBuff.write( (short)0 );
			return;
		} // startSegment

		} // class Ucs2SegWtr

	}  // class MsgOut
}