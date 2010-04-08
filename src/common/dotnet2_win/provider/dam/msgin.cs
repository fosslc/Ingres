/*
** Copyright (c) 1999, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: msgin.cs
	**
	** Description:
	**	Defines class providing the capability to receive messages
	**	from a Data Access Server.
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
	**	MsgIn		Extends MsgOut.
	**	ByteSegIS	Segmented byte BLOB input stream.
	**	Ucs2SegRdr	Segmented UCS2 BLOB reader.
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
	**	    Added Desegmentor class to process BLOBs as streams.  
	**	    Added methods (readBytes(),readBinary(),readUnicode(),
	**	    skip()) to process BLOBs.
	**	17-Nov-99 (gordy)
	**	    Extracted input functionality from DbConn.cs.
	**	25-Jan-00 (gordy)
	**	    Added tracing.
	**	21-Apr-00 (gordy)
	**	    Moved to package io.
	**	12-Apr-01 (gordy)
	**	    Pass tracing to exception.
	**	18-Apr-01 (gordy)
	**	    Added readBytes() method which reads the length from message.
	**	10-May-01 (gordy)
	**	    Support UCS2 character values.  Added UCS2 segmented BLOB class.
	**	 6-Sep-02 (gordy)
	**	    Character encoding now encapsulated in CharSet class.
	**	    Moved to GCF dam package.  Renamed as DAM Message
	**	    Layer implementation class.
	**	19-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Renamed GCF error codes.
	**	20-Dec-02 (gordy)
	**	    Message header ID protocol level dependent.  TL protocol level
	**	    communicated to rest of messaging system.  New connection param
	**	    for character set ID.  Enhanced character set/encoding processing.
	**	15-Jan-03 (gordy)
	**	    Added MSG layer ID connection parameter.
	**	22-Sep-03 (gordy)
	**	    Added support for ByteArray, CharArray, and SqlData values.
	**	 6-Oct-03 (gordy)
	**	    Added ability to check for end-of-group.
	**	15-Mar-04 (gordy)
	**	    Support SQL BIGINT and .NET long values.
	**	19-May-04 (thoda04)
	**	    Added Read and Close methods to ByteSegIS
	**	    to override MemoryStream methods.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	16-Jun-06 (gordy)
	**	    ANSI Date/Time data type support.
	**	 3-Jul-06 (gordy)
	**	    Provided alternate way to override character encoding.
	**	 5-Oct-06 (thoda04)
	**	    Ported to .NET Data Provider 2.
	**	 1-Mar-07 (gordy, ported by thoda04)
	**	    Dropped char_set parameter, don't default CharSet.
	**	21-Mar-07 (thoda04)
	**	    Added IntervalDayToSecond, IntervalYearToMonth.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Added support for multiple server targets.
	**	 7-Dec-09 (gordy, ported by thoda04)
	**	    Added support for BOOLEAN data type.
	*/



	/*
	** Name: MsgIn
	**
	** Description:
	**	Class providing methods for receiving messages from a Data 
	**	Access Server.  Provides methods for reading messages of
	**	the Message Layer of the Data Access Messaging protocol 
	**	(DAM-ML).  Processes Transport Layer (DAM-TL) packets from
	**	the server using the buffered input class InBuff
	**
	**	The DAM-ML connection parameters received from the server
	**	are provided as output of the class constructor.
	**
	**	When reading messages, the caller must provide multi-
	**	threaded protection for each message.
	**
	**  Public Methods:
	**
	**	receive			Load the next input message.
	**	moreData		More data in message?
	**	moreMessages		More messages follow current message?
	**	skip			Skip bytes in input message.
	**	readByte		Read a byte from input message.
	**	readShort		Read a short value from input message.
	**	readInt			Read an integer value from input message.
	**	readFloat		Read a float value from input message.
	**	readDouble		Read a double value from input message.
	**	readBytes		Read a byte array from input message.
	**	readString		Read a string value from input message.
	**	readUCS2		Read a UCS2 string value from input message.
	**	readSqlData		Read an SQL data value.
	**
	**  Protected Methods:
	**
	**	connect			Connect with target server.
	**	recvCC			Receive connection confirmation.
	**	disconnect		Disconnect from the server.
	**	close			Close the server connection.
	**	setBuffSize		Set the I/O buffer size.
	**	setIoProtoLvl		Set the I/O protocol level.
	**
	**  Private Data:
	**
	**	in			Input buffer.
	**	in_msg_id		Current input message ID.
	**	in_msg_len		Input message length.
	**	in_msg_flg		Input message flags.
	**	char_buff		UCS2 Character buffer (readUCS2()).
	**	empty			Zero length strings.
	**	no_params		No DAM-ML connection parameters.
	**
	**  Private Methods:
	**
	**	getCharSet		Map character set to encoding.
	**	serverDisconnect	Process server disconnect request.
	**	readSqlDataIndicator	Read the SQL data/NULL inidicator byte.
	**	need			Make sure data is available in input message.
	**
	** History:
	**	 7-Jun-99 (gordy)
	**	    Created.
	**	10-Sep-99 (gordy)
	**	    Parameterized the initial TL connection data.
	**	22-Sep-99 (gordy)
	**	    Added character set/encoding.
	**	29-Sep-99 (gordy)
	**	    Added methods (readBytes(),readBinary(),readUnicode(),
	**	    skip()) to process BLOBs.
	**	17-Nov-99 (gordy)
	**	    Extracted input functionality from DbConn.
	**	18-Apr-01 (gordy)
	**	    Added readBytes() method which reads the length from message.
	**	10-May-01 (gordy)
	**	    Added readUCS2() methods and char_buff to support UCS2
	**	    character strings, along with a UCS2 segmented BLOB class.
	**	 1-Oct-02 (gordy)
	**	    Renamed as DAM-ML messaging class.  Added empty.
	**	20-Dec-02 (gordy)
	**	    Added setIoProtoLvl() to set the TL protocol level.  Extracted
	**	    character set/encoding processing to getCharSet().
	**	22-Sep-03 (gordy)
	**	    Added readBytes() variant to support ByteArray values,
	**	    readUCS2() variant to support CharArray values, and
	**	    readSqlData(), readSqlDataIndicator() to support SQL 
	**	    data values.  Dropped readBinary(), readAscii(), and 
	**	    readUnicode() (BLOBs only supported as SQL data values).
	**	    Removed segIS and segRdr (new objects created for each
	**	    BLOB).
	**	 6-Oct-03 (gordy)
	**	    Added moreMessages().
	**	15-Mar-04 (gordy)
	**	    Added methods to read long (readLong()) values.
	**	 3-Nov-08 (gordy, ported by thoda04)
	**	    Extract connection functionality from constructor to
	**	    connect() and recvCC().  Added static default for no
	**	    DAM-ML connection parameters.
	**	 7-Dec-09 (gordy, ported by thoda04)
	**	    Added readSqlData() method for boolean.
	*/

	class
		MsgIn : MsgOut
	{

		/*
		** Input buffer and BLOB streams.
		*/
		private InBuff		inBuff = null;	// Input buffer.

		/*
		** Current message info
		*/
		private byte		in_msg_id = 0;	// Input message ID.
		private short		in_msg_len = 0;	// Input message length.
		private byte		in_msg_flg = 0;	// Input message flags.
		private char[]		char_buff = new char[0];  // UCS2 char buff.

		private static String	empty = "";	// Zero-length strings
		private static byte[]	no_params = new byte[0]; // No DAM-ML params


		/*
		** Name: MsgIn
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
		**	    Extracted connect() and recvCC() functionality.
		*/

		protected
		MsgIn()
			: base()
		{
			title = "MsgIn[" + ConnID + "]";
		}


		/*
		** Name: connect
		**
		** Description:
		**	Establish connection with target server.  Initializes 
		**	the input buffer.
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
		**	    Extracted input functionality from DbConn.
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Extracted from constructor.
		*/

		protected internal override void
		connect(String host, String portID)
		{
			base.connect( host, portID );

			try
			{
				InputStream inputStream = new InputStream(socket.getInputStream());
				inBuff = new InBuff(inputStream, ConnID, 1 << DAM_TL_PKT_MIN);
			}
			catch (Exception ex)
			{
				if (trace.enabled(1))
					trace.write(title + ": error creating input buffer: " +
						 ex.Message);
				disconnect();
				throw SqlEx.get(ERR_GC4001_CONNECT_ERR, ex);
			}

			return;
		}  // connect

		/*
		** Name: recvCC
		**
		** Description:
		**	Receives the DAM-TL Connection Confirm packet.  DAM-ML connection 
		**	parameters are returned
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte []		DAM-ML connection parameters.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		**	10-Sep-99 (gordy)
		**	    Parameterized the initial TL connection data.
		**	22-Sep-99 (gordy)
		**	    Added character set/encoding.
		**	 6-Sep-02 (gordy)
		**	    Character encoding now encapsulated in CharSet class.
		**	    Read character set during connection parameter processing
		**	    and leave mapping to encoding for parameter validation.
		**	    Trace character set and encoding mapping.
		**	 1-Oct-02 (gordy)
		**	    Renamed as DAM Message Layer.  Replaced connection info 
		**	    parameters with multi-dimensional array: input passed to
		**	    super-class, response returned as output.  Added trace log 
		**	    parameter.
		**	20-Dec-02 (gordy)
		**	    Added char_set parameter for connection character encoding.
		**	    Communicate TL protocol level to rest of messaging system.
		**	    Added conn param for char-set ID.  Check for server abort.
		**	15-Jan-03 (gordy)
		**	    Added MSG layer ID connection parameter.
		**	27-Feb-04 (gordy)
		**	    Use ASCII encoding for character-set parameter to avoid
		**	    bootstrapping problems.
		**	 1-Mar-07 (gordy, ported by thoda04)
		**	    Dropped char_set parameter, don't default CharSet.
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Extracted from constructor.
		*/

		protected byte[]
		recvCC()
		{
			String  cs = null;			// Character set.
			int	    cs_id = -1;			// Character set ID.
			int	    size = DAM_TL_PKT_MIN;	// TL packet size.
			short   tl_id;
			byte[]  msg_cp = no_params;	// No output yet.

			try
			{
				if ( trace.enabled( 3 ) )
					trace.write( title + ": confirm TL connect" );

				/*
				** A Connection Confirmation packet is expected.  Any
				** other response will throw an exception (perhaps
				** after processing a server disconnect request).
				*/
				switch( (tl_id = inBuff.receive()) )
				{
					case DAM_TL_CC :  /* OK */		break;
					case DAM_TL_DR: serverDisconnect(); return (no_params);
					default :
						if ( trace.enabled( 1 ) )  
							trace.write( title + ": invalid TL CC packet ID " + tl_id );
						throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
				}
    
				/*
				** Read negotiated values from server (if available).
				*/
				while( inBuff.avail() >= 2 )
				{
					byte    param_id = inBuff.readByte();
					short   param_len = (short)(inBuff.readByte() & 0xff);

					if ( param_len == 255 )
						if ( inBuff.avail() >= 2 )
							param_len = inBuff.readShort();
						else
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );

					if ( inBuff.avail() < param_len )
						throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );

					switch( param_id )
					{
						case DAM_TL_CP_PROTO :
							if ( param_len == 1 )  
								TL_ProtocolLevel = inBuff.readByte();
							else
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
							break;

						case DAM_TL_CP_SIZE :
						switch( param_len )
						{
							case 1 :  size = inBuff.readByte();		break;
							case 2 :  size = inBuff.readShort();	break;
							case 4 :  size = inBuff.readInt();		break;

							default :
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}
							break;

						case DAM_TL_CP_CHRSET :
							/*
							** This parameter is availabe at all protocol levels,
							** but may have a potential bootstrapping problem with
							** the character encoding.  The character-set ID is
							** preferred.
							*/
						{
							byte[] ba = new byte[ param_len ];
		    
							if ( inBuff.readBytes( ba, 0, param_len ) == param_len )
							{
								System.Text.Encoding ASCIIEncoding = System.Text.Encoding.ASCII;
								cs = ASCIIEncoding.GetString( ba );
							}
							else
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}
							break;

						case DAM_TL_CP_CHRSET_ID :
							/*
							** This parameter avoids the bootstrapping problem 
							** which exists with the character-set name, but is 
							** only availabe starting at protocol level 2.
							*/
							if ( TL_ProtocolLevel < DAM_TL_PROTO_2 )
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );

						switch( param_len )
						{
							case 1 :  cs_id = inBuff.readByte();	break;
							case 2 :  cs_id = inBuff.readShort();	break;
							case 4 :  cs_id = inBuff.readInt();		break;

							default :
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}
							break;

						case DAM_TL_CP_MSG :
							msg_cp = new byte[ param_len ];
							if ( inBuff.readBytes( msg_cp, 0, param_len ) != param_len )
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
							break;

						case DAM_TL_CP_MSG_ID :
							if ( param_len != 4  ||  inBuff.readInt() != DAM_TL_MSG_ID )
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
							break;

						default :
							if ( trace.enabled( 1 ) )  
								trace.write( title + ": Bad TL CC param ID: " + param_id );
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
					}
				}

				if ( inBuff.avail() > 0 )  throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );

				/*
				** Validate message parameters.
				*/
				if ( 
					TL_ProtocolLevel < DAM_TL_PROTO_1  ||  
					TL_ProtocolLevel > DAM_TL_PROTO_DFLT  ||
					size < DAM_TL_PKT_MIN  ||  
					size > DAM_TL_PKT_MAX 
					)
				{
					if ( trace.enabled( 1 ) )
						trace.write( title + ": invalid TL parameter: protocol " + 
							TL_ProtocolLevel + ", size " + size );
					throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
				}
			}
			catch( SqlEx ex )
			{
				if ( trace.enabled( 1 ) )  
					trace.write( title + ": error negotiating parameters" );
				disconnect();
				throw ex;
			}

			/*
			** Connection character encoding mapped from character-
			** set ID or name.  Mapping errors are ignored to allow 
			** an explicit encoding to be configured.
			*/
			if (cs_id >= 0)
				try { this.char_set = CharSet.getCharSet(cs_id); }
				catch (Exception)
				{
					if (trace.enabled(1))
						trace.write(title + ": unknown character-set: " + cs_id);
				}

			if (this.char_set == null && cs != null)
				try { this.char_set = CharSet.getCharSet(cs); }
				catch (Exception)
				{
					if (trace.enabled(1))
						trace.write(title + ": unknown character-set: " + cs);
				}

			/*
			** We initialized with minimum buffer size, then requested
			** maximum size for the connection.  If the server accepts
			** anything more than minimum, adjust the buffers accordingly.
			*/
			if ( size != DAM_TL_PKT_MIN )
				BuffSize = ( 1 << size );

			if ( trace.enabled( 2 ) )
				trace.write( title + ": TL connection opened" );

			if ( trace.enabled( 3 ) )
			{
				trace.write( "    TL protocol level : " + TL_ProtocolLevel );
				trace.write( "    TL buffer size    : " + (1 << size) );
				trace.write( "    Character encoding: " );
				trace.write("        " + this.char_set);
			}

			return (msg_cp);
		} // recvCC


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
		**	    Extracted input functionality from DbConn.
		*/

		protected internal override void
			disconnect()
		{
			/*
			** We don't set the input buffer reference to null
			** here so that we don't have to check it on each
			** use.  I/O buffer functions will continue to work
			** until a request results in a stream I/O request,
			** in which case an exception will be thrown by the
			** I/O buffer.
			**
			** We must, however, test the reference for null
			** since we may be called by the constructor with
			** a null input buffer.
			*/
			if ( inBuff != null )
			{
				try { inBuff.close(); }
				catch( Exception ) {}
			}

			base.disconnect();
			return;
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
		**	    Extracted input functionality from DbConn.
		**	26-Dec-02 (gordy)
		**	    Check for server abort during processing.
		*/

		protected internal override void
			close()
		{
			base.close();

			try 
			{ 
				if ( trace.enabled( 3 ) )
					trace.write( title + ": confirm TL disconnect" );

				/*
				** Receive and discard messages until Disconnection Confirm 
				** is received or exception is thrown.
				*/
				for(;;)
					switch( inBuff.receive() )
					{
						case DAM_TL_DC :  /* We be done! */		goto loopbreak;
						case DAM_TL_DR :  serverDisconnect();	break;
					}
				loopbreak :

				if ( trace.enabled( 2 ) )
					trace.write( title + ": TL connection closed" );
			}
			catch( SqlEx ) 
			{
				if ( trace.enabled( 2 ) )
					trace.write( title + ": TL connection aborted" );
			}
    
			return;
		} // close


		/*
		** Name: BuffSize
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

		protected internal override int BuffSize
		{
			set
			{
				inBuff.BuffSize = value;
				base.BuffSize = value;
				return;
			}
		} // BuffSize


		/*
		** Name: TL_ProtocolLevel property
		**
		** Description:
		**	Get/Set the DAM Transport Layer protocol level.
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
				base.TL_ProtocolLevel   = value;
				inBuff.TL_ProtocolLevel = value;
			}
		} // TL_ProtocolLevel


		/*
		** Name: serverDisconnect
		**
		** Description:
		**	Process a TL DR packet for a server disconnect request.
		**	Throws an exception indicating connection aborted.
		**
		**	A Disconnect Confirmation packet should be returned to
		**	the server, but there is no reasonable way to do so.
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
		**	26-Dec-02 (gordy)
		**	    Created.
		**	15-Jan-03 (gordy)
		**	    Handle all integer parameter value lengths.
		**	 3-Nov-08 (gordy, ported by thoda04)
		**	    Provide better handling for common errors.
		*/

		private void
			serverDisconnect()
		{
			/*
			** An error code may be provided as an optional parameter.
			*/
			if ( inBuff.avail() >= 2 )
			{
				byte    param_id = inBuff.readByte();
				byte    param_len = inBuff.readByte();
	
				if ( param_id == DAM_TL_DP_ERR  &&  inBuff.avail() >= param_len ) 
				{
					int error;

					switch( param_len )
					{
						case 1 : error = inBuff.readByte();	break;
						case 2 : error = inBuff.readShort();	break;
						case 4 : error = inBuff.readInt();	break;
						default: error = E_GC4008_SERVER_ABORT;	break;
					}

					if ( trace.enabled( 1 ) )  
						trace.write( title + ": server error 0x" + 
							toHexString( error ) );

					switch (error)
					{
						case E_GC4008_SERVER_ABORT:
							throw SqlEx.get(ERR_GC4008_SERVER_ABORT);

						case E_GC480E_CLIENT_MAX:
							throw SqlEx.get(ERR_GC480E_CLIENT_MAX);

						case E_GC481F_SVR_CLOSED:
							throw SqlEx.get(ERR_GC481F_SVR_CLOSED);

						default:
							/*
							** Need to throw a SERVER_ABORT exception, but need the 
							** server error code instead of the driver error code. 
							** Generate a driver exception and use the message text 
							** and SQLSTATE along with server error code to generate 
							** the actual exception.
							*/
							SqlEx sqlEx = SqlEx.get(ERR_GC4008_SERVER_ABORT);
							throw SqlEx.get(sqlEx.Message,
								sqlEx.getSQLState(), error);
					}  // end switch (error)
				}  // end if if ( param_id == DAM_TL_DP_ERR ...
			}  // end if ( inBuff.avail() ...

			/*
			** Server failed to provided an abort code.
			*/
			if ( trace.enabled( 1 ) )  trace.write( title + ": server abort" );
			throw SqlEx.get( ERR_GC4008_SERVER_ABORT );
		} // serverDisconnect


		/*
		** Name: receive
		**
		** Description:
		**	Read the next input message.  Message may be present in the
		**	input buffer (message concatenation) or will be read from
		**	input stream.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte	    Message ID.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		**	10-Sep-99 (gordy)
		**	    Removed test for TL DR (now done in inBuff.receive()).
		**	20-Dec-02 (gordy)
		**	    Header ID protocol level dependent.  Restored handling TL DR.
		*/

		public virtual byte
			receive()
		{
			int msg_id;

			if ( trace.enabled( 3 ) )  trace.write( title + ": check TL data" );

			/*
			** Check to see if message header is in the input
			** buffer (handles message concatenation).
			*/
			while( inBuff.avail() < 8 )
				try 
				{ 
					short tl_id;

					/*
					** Headers are not split across continued messages.
					** Data remaining in the current buffer indicates
					** a message processing error.
					*/
					if ( inBuff.avail() > 0 )
					{
						if ( trace.enabled( 1 ) )  
							trace.write( title + ": invalid header length " + 
								inBuff.avail() + " bytes" );
						throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
					}

					switch( (tl_id = inBuff.receive()) )
					{
						case DAM_TL_DT :  /* OK */			break;
						case DAM_TL_DR :  serverDisconnect();	break;
						default :
							if ( trace.enabled( 1 ) )  
								trace.write( title + ": invalid TL packet ID 0x" +
									SqlData.toHexString( tl_id ) );
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
					}
				}
				catch( SqlEx ex )
				{
					disconnect();
					throw ex;
				}

			/*
			** Read and validate message header.
			*/
			msg_id = inBuff.readInt();

			if ( 
				(ML_ProtocolLevel <  MSG_PROTO_3  &&  msg_id != MSG_HDR_ID_1)  ||
				(ML_ProtocolLevel >= MSG_PROTO_3  &&  msg_id != MSG_HDR_ID_2)
				)
			{
				if ( trace.enabled( 1 ) )  
					trace.write( title + ": invalid header ID 0x" +
						toHexString( msg_id ) );
				disconnect();
				throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
			}

			in_msg_len = inBuff.readShort();
			in_msg_id = inBuff.readByte();
			in_msg_flg = inBuff.readByte();

			if ( trace.enabled( 2 ) )  
				trace.write( title + ": received message " + 
					IdMap.map( in_msg_id, MsgConst.msgMap ) + 
					" length " + in_msg_len + 
					((in_msg_flg & MSG_HDR_EOD) == 0 ? "" : " EOD") +
					((in_msg_flg & MSG_HDR_EOG) == 0 ? "" : " EOG") );

			return( in_msg_id );
		} // receive


		/*
		** Name: moreData
		**
		** Description:
		**	Returns an indication that additional data remains in the
		**	current input message.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if additional data available, false otherwise.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		*/

		internal bool
			moreData()
		{
			byte msg_id = in_msg_id;

			while( in_msg_len <= 0  &&  (in_msg_flg & MSG_HDR_EOD) == 0 )
			{
				/*
				** Receive the message continuation and check that
				** it is a continuation of the current message.
				*/
				if ( receive() != msg_id )
				{
					if ( trace.enabled( 1 ) )  
						trace.write( title + ": invalid message continuation " +
							msg_id + " to " + in_msg_id );
					disconnect();
					throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
				}
			}

			return( in_msg_len > 0 );
		} // moreData


		/*
		** Name: moreMessages
		**
		** Description:
		**	Returns an indication that additional messages follow the
		**	current message (end-of-group has not been reached).
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if additional messages, false otherwise.
		**
		** History:
		**	 6-Oct-03 (gordy)
		**	    Created.
		*/

		public bool
			moreMessages()
		{
			return( (in_msg_flg & MSG_HDR_EOG) == 0 );
		} // moreMessages


		/*
		** Name: skip
		**
		** Description:
		**	Skip bytes in the current input message.
		**
		** Input:
		**	length	    Number of bytes.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Number of bytes actually skipped.
		**
		** History:
		**	29-Sep-99 (gordy)
		**	    Created.
		*/

		public int
			skip( int length )
		{
			int total = 0;

			if ( trace.enabled( 4 ) )
				trace.write( title + ": skipping " + length + " bytes" );

			while( length > 0 )
			{
				int len;
				need( length, false );
				len = Math.Min( in_msg_len, length );
				len = inBuff.skip( len );
				length -= len;
				in_msg_len -= (short)len;
				total += len;
			}

			return( total );
		} // skip


		/*
		** Name: readByte
		**
		** Description:
		**	Read a byte value from the current input message.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte	Input byte value.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		*/

		public byte
			readByte()
		{
			need( 1, true );
			in_msg_len -= 1;
			return( inBuff.readByte() );
		} // readByte

		/*
		** Name: readShort
		**
		** Description:
		**	Read a short value from the current input message.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	short	Input short value.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		*/

		public short
			readShort()
		{
			need( 2, true );
			in_msg_len -= 2;
			return( inBuff.readShort() );
		} // readShort

		/*
		** Name: readInt
		**
		** Description:
		**	Read an integer value from the current input message.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Input integer value.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		*/

		public int
			readInt()
		{
			need( 4, true );
			in_msg_len -= 4;
			return( inBuff.readInt() );
		} // readInt


		/*
		** Name: readLong
		**
		** Description:
		**	Read a long value from the current input message.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	long	Input long value.
		**
		** History:
		**	15-Mar-04 (gordy)
		**	    Created.
		*/

		public long
			readLong()
		{
			need( 8, true );
			in_msg_len -= 8;
			return( inBuff.readLong() );
		} // readLong


		/*
		** Name: readFloat
		**
		** Description:
		**	Read a float value from the current input message.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	float	Input float value.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		*/

		public float
			readFloat()
		{
			need( DAM_TL_F4_LEN, true );
			in_msg_len -= DAM_TL_F4_LEN;
			return( inBuff.readFloat() );
		} // readFloat

		/*
		** Name: readDouble
		**
		** Description:
		**	Read a double value from the current input message.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	double	Input double value.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		*/

		public double
			readDouble()
		{
			need( DAM_TL_F8_LEN, true );
			in_msg_len -= DAM_TL_F8_LEN;
			return( inBuff.readDouble() );
		} // readDouble


		/*
		** Name: readBytes
		**
		** Description:
		**	Reads a byte array from the current input message.
		**	The length of the array is read from the input message.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	    Byte array from message.
		**
		** History:
		**	18-Apr-01 (gordy)
		**	    Created.
		*/

		public byte[]
			readBytes()
		{
			return( readBytes( readShort() ) );
		} // readBytes


		/*
		** Name: readBytes
		**
		** Description:
		**	Reads a byte array from the current input message.
		**
		** Input:
		**	length	    Length of byte array to read.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	    Byte array from message
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		**	29-Jun-99 (gordy)
		**	    Created readBytes( byte[], offset, length ) and
		**	    simplified this method to create byte array of
		**	    desired length and call new readBytes() method.
		*/

		public byte[]
			readBytes( int length )
		{
			byte[]    bytes = new byte[ length ];
			readBytes( bytes, 0, length );
			return( bytes );
		} // readBytes


		/*
		** Name: readBytes
		**
		** Description:
		**	Reads bytes from the current input message into a
		**	byte sub-array.
		**
		** Input:
		**	offset	    Offset in byte array.
		**	length	    Number of bytes.
		**
		** Output:
		**	bytes	    Bytes read from message.
		**
		** Returns:
		**	int	    Number of bytes actually read.
		**
		** History:
		**	29-Sep-99 (gordy)
		**	    Created.
		*/

		public int
			readBytes( byte[] bytes, int offset, int length )
		{
			int requested = length;

			while( length > 0 )
			{
				need( length, false );
				int len = Math.Min( (int)in_msg_len, length );
				len = inBuff.readBytes( bytes, offset, len );
				offset += len;
				length -= len;
				in_msg_len -= (short)len;
			}

			return( requested - length );
		} // readBytes


		/*
		** Name: readBytes
		**
		** Description:
		**	Reads bytes from the current input message into a
		**	IByteArray.  The array limit is ignored.  Any bytes
		**	which exceed the limit are read and discarded.
		**	It is the callers responsible to ensure adaquate 
		**	limit and capacity.
		**
		** Input:
		**	offset	    Offset in byte array.
		**	length	    Number of bytes.
		**
		** Output:
		**	bytes	    Filled with bytes from message.
		**
		** Returns:
		**	int	    Number of bytes actually read.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public int
			readBytes( IByteArray bytes, int length )
		{
			int requested = length;
    
			while( length > 0 )
			{
				need( length, false );
				int len = Math.Min( (int)in_msg_len, length );
				len = inBuff.readBytes( bytes, len );
				length -= len;
				in_msg_len -= (short)len;
			}
    
			return( requested - length );
		} // readBytes


		/*
		** Name: readString
		**
		** Description:
		**	Reads a string value from the current input message.
		**	The length of the string is read from the input stream.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    String read from message.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		*/

		public String
			readString()
		{
			return( readString( readShort() ) );
		} // readString

		/*
		** Name: readString
		**
		** Description:
		**	Reads a string value from the current input message.
		**
		** Input:
		**	length	    Length of string.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    String read from message.
		**
		** History:
		**	 7-Jun-99 (gordy)
		**	    Created.
		**	22-Sep-99 (gordy)
		**	    Added character set/encoding.
		**	 6-Sep-02 (gordy)
		**	    Moved character encoding conversion to CharSet class.
		*/

		public String
			readString( int length )
		{
			String  str;

			if ( length <= in_msg_len )	    // Is entire string available?
				if ( length <= 0 )
					str = empty;
				else
				{
					str = inBuff.readString( length, char_set );
					in_msg_len -= (short)length;
				}
			else
			{
				/*
				** Entire string is not available.  Collect the fragments
				** (readBytes() does this automatically) and convert to string.
				*/
				byte[] buff = readBytes( length );

				try { str = char_set.getString( buff ); }
				catch( Exception )
				{
					throw SqlEx.get( ERR_GC401E_CHAR_ENCODE );	// Should not happen!
				}
			}

			return( str );
		} // readString


		/*
		** Name: readUCS2
		**
		** Description:
		**	Reads a UCS2 string from the current input message.  The
		**	length of the string is read from the input message.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    UCS2 string read from message.
		**
		** History:
		**	10-May-01 (gordy)
		**	    Created.
		*/

		public String
			readUCS2()
		{
			return( readUCS2( readShort() ) );
		} // readUCS2


		/*
		** Name: readUCS2
		**
		** Description:
		**	Reads a UCS2 string from the current input message.
		**
		** Input:
		**	length	    Length of the UCS2 string to read.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    UCS2 string read from message.
		**
		** History:
		**	10-May-01 (gordy)
		**	    Created.
		*/

		public String
			readUCS2( int length )
		{
			if ( length > char_buff.Length )  char_buff = new char[ length ];
			length = readUCS2( char_buff, 0, length );
			return( new String( char_buff, 0, length ) );
		} // readUCS2


		/*
		** Name: readUCS2
		**
		** Description:
		**	Reads UCS2 characters from the current input message
		**	into a character sub-array.
		**
		** Input:
		**	offset	    Offset in character array.
		**	length	    Number of characters.
		**
		** Output:
		**	chars	    Filled with characters from message.
		**
		** Returns:
		**	int	    Number of characters actually read.
		**
		** History:
		**	10-May-01 (gordy)
		**	    Created.
		*/

		public int
			readUCS2( char[] chars, int offset, int length )
		{
			for( int end = offset + length; offset < end; offset++ )
				chars[ offset ] = (char)readShort();
			return( length );
		} // readUCS2


		/*
		** Name: readUCS2
		**
		** Description:
		**	Reads UCS2 characters from the current input message
		**	into a ICharArray.  The array limit is ignored.  Any
		**	characters which exceed the array limit are read and
		**	discarded.  It is the callers responsibility to 
		**	ensure adaquate limit and capacity.
		**
		** Input:
		**	length	    Number of characters.
		**
		** Output:
		**	array	    Filled with characters from message.
		**
		** Returns:
		**	int	    Number of characters actually read.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public int
			readUCS2( ICharArray array, int length )
		{
			for( int count = length; count > 0; count-- )
				array.put( (char)readShort() );
			return( length );
		} // readUCS2


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlNull data value from the current input message.
		**
		**	A SqlNull data value is composed of simply a data indicator
		**	byte which must be 0 (NULL data value).
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlNull value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );	// Should be NULL.
			else
				value.setNull();	// Shouldn't be needed, but just to be safe...
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlTinyInt data value from the current input message.
		**
		**	A SqlTinyInt data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a one byte integer.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlTinyInt value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( readByte() );	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlSmallInt data value from the current input message.
		**
		**	A SqlSmallInt data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a two byte integer.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlSmallInt value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( readShort() );	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlInt data value from the current input message.
		**
		**	A SqlInt data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a four byte integer.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlInt value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( readInt() );		// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlBigInt data value from the current input message.
		**
		**	A SqlBigInt data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a eight byte integer.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlBigInt value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( readLong() );	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlReal data value from the current input message.
		**
		**	A SqlReal data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a four byte floating
		**	point value.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlReal value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( readFloat() );	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlDouble data value from the current input message.
		**
		**	A SqlDouble data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by an eight byte floating
		**	point value.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlDouble value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( readDouble() );	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlDecimal data value from the current input message.
		**
		**	A SqlDecimal data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlDecimal value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( readString() );	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlBool data value from the current input message.
		**
		**	A SqlBool data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a one byte integer.
		**	Only the low-order bit of the integer is significant:
		**	FALSE = 0x00 and TRUE = 0x01.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 7-Dec-09 (gordy, ported by thoda04)
		**	    Created.
		*/

		public void
		readSqlData(SqlBool value)
		{
			if (readSqlDataIndicator())	// Read data indicator byte.
				value.set(readByte()); 	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlDate data value from the current input message.
		**
		**	A SqlDate data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		readSqlData(SqlDate value)
		{
			if (readSqlDataIndicator())	// Read data indicator byte.
				value.set(readString());	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlTime data value from the current input message.
		**
		**	A SqlTime data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		readSqlData(SqlTime value)
		{
			if (readSqlDataIndicator())	// Read data indicator byte.
				value.set(readString());	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlTimestamp data value from the current input message.
		**
		**	A SqlTimestamp data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		readSqlData(SqlTimestamp value)
		{
			if (readSqlDataIndicator())	// Read data indicator byte.
				value.set(readString());	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlInterval data value from the current input message.
		**
		**	A SqlInterval data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	17-Jan-07 (thoda04)
		**	    Created.
		*/

		public void
		readSqlData(SqlInterval value)
		{
			if (readSqlDataIndicator())	// Read data indicator byte.
				value.set(readString());	// Read data value.
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads an IngresDate data value from the current input message.
		**
		**	An IngresDate data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a string which is
		**	composed of a two byte integer indicating the byte length
		**	of the string followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( IngresDate value, bool isBlankDateNull )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
			{
				value.set( readString() );	// Read data value.
				if (isBlankDateNull  &&  value.getTruncSize() == 0)
					value.setNull();  // blank date is null
			}
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlByte data value from the current input message.
		**	Uses the IByteArray interface to load the SqlByte data value.
		**
		**	A SqlByte data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a binary byte array
		**	whose length is detemined by the fixed size of the SqlByte 
		**	data value (the IByteArray valuelimit()).
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlByte value )
		{
			if ( ! readSqlDataIndicator() )	// Read data indicator byte.
				value.setNull();		// NULL data value.
			else
			{					// Read data value.
				int length = value.valuelimit();	// Fixed length from meta-data.
				value.clear();
				value.ensureCapacity( length );
				readBytes( (IByteArray)value, length );
			}
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlVarByte data value from the current input message.
		**	Uses the IByteArray interface to load the SqlVarByte data value.
		**
		**	A SqlVarByte data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a two byte integer length
		**	which is followed by a binary byte array of the indicated length.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlVarByte value )
		{
			if ( ! readSqlDataIndicator() )	// Read data indicator byte.
				value.setNull();		// NULL data value.
			else
			{					// Read data value.
				int length = readShort();	// Fixed length from meta-data.
				value.clear();
				value.ensureCapacity( length );
				readBytes( (IByteArray)value, length );
			}
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlChar data value from the current input message.
		**	Uses the IByteArray interface to load the SqlChar data value.
		**
		**	A SqlChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a character byte 
		**	array whose length is detemined by the fixed size of the
		**	SqlChar data value (the IByteArray valuelimit()).
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlChar value )
		{
			if ( ! readSqlDataIndicator() )	// Read data indicator byte.
				value.setNull();		// NULL data value.
			else
			{					// Read data value.
				int length = value.valuelimit();	// Fixed length from meta-data.
				value.clear();
				value.ensureCapacity( length );
				readBytes( (IByteArray)value, length );
			}
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlVarChar data value from the current input message.
		**	Uses the IByteArray interface to load the SqlVarChar data value.
		**
		**	A SqlVarChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a two byte integer 
		**	length which is followed by a character byte array of the
		**	indicated length.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlVarChar value )
		{
			if ( ! readSqlDataIndicator() )	// Read data indicator byte.
				value.setNull();		// NULL data value.
			else
			{					// Read data value.
				int length = readShort();	// Variable length from message.
				value.clear();
				value.ensureCapacity( length );
				readBytes( (IByteArray)value, length );
			}
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlNChar data value from the current input message.
		**	Uses the ICharArray interface to load the SqlNChar data value.
		**
		**	A SqlNChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a UCS2 (two byte
		**	integer) array whose length is detemined by the fixed size 
		**	of the SqlNChar data value (the ICharArray valuelimit()).
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlNChar value )
		{
			if ( ! readSqlDataIndicator() )	// Read data indicator byte.
				value.setNull();		// NULL data value.
			else
			{					// Read data value.
				int length = value.valuelimit();	// Fixed length from meta-data.
				value.clear();
				value.ensureCapacity( length );
				readUCS2( (ICharArray)value, length );
			}
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlNVarChar data value from the current input message.
		**	Uses the ICharArray interface to load the SqlNVarChar data value.
		**
		**	A SqlNVarChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by a two byte integer length 
		**	which is followed by a UCS2 (two byte integer) array of the 
		**	indicated length.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlNVarChar value )
		{
			if ( ! readSqlDataIndicator() )	// Read data indicator byte.
				value.setNull();		// NULL data value.
			else
			{					// Read data value.
				int length = readShort();	// Variable length from message.
				value.clear();
				value.ensureCapacity( length );
				readUCS2( (ICharArray)value, length );
			}
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlLongByte data value from the current input message.
		**	Only a single stream data value may be active at a given time.
		**	Attempting to read another stream data value will result in
		**	the closure of the preceding stream.
		**
		**	A SqlLongByte data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by zero or more segments
		**	which are then followed by and end-of-segments indicator.  Each
		**	segment is composed of a two byte integer length (non-zero) 
		**	followed by a binary byte array of the indicated length.  The
		**	end-of-segments indicator is a two byte integer with value
		**	zero.
		**
		**	Only the SQL data/NULL indicator is read by this method.  If
		**	the SqlLongByte value is non-NULL, an InputStream is provided
		**	for reading the segmented data stream.  The InputStream also
		**	implements the SqlStream StreamSource interface for providing
		**	notification of stream closure events.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlLongByte value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( new ByteSegIS( this, trace, in_msg_id ) );
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlLongChar data value from the current input message.
		**	Only a single stream data value may be active at a given time.
		**	Attempting to read another stream data value will result in
		**	the closure of the preceding stream.
		**
		**	A SqlLongChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by zero or more segments
		**	which are then followed by and end-of-segments indicator.  Each
		**	segment is composed of a two byte integer length (non-zero) 
		**	followed by a character byte array of the indicated length.  
		**	The end-of-segments indicator is a two byte integer with value
		**	zero.
		**
		**	Only the SQL data/NULL indicator is read by this method.  If
		**	the SqlLongChar value is non-NULL, an InputStream is provided
		**	for reading the segmented data stream.  The InputStream also
		**	implements the SqlStream StreamSource interface for providing
		**	notification of stream closure events.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlLongChar value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( new ByteSegIS( this, trace, in_msg_id ) );
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlData
		**
		** Description:
		**	Reads a SqlLongNChar data value from the current input message.
		**	Only a single stream data value may be active at a given time.
		**	Attempting to read another stream data value will result in
		**	the closure of the preceding stream.
		**
		**	A SqlLongNChar data value is composed of a data indicator byte 
		**	which, if not NULL, may be followed by zero or more segments
		**	which are then followed by and end-of-segments indicator.  Each
		**	segment is composed of a two byte integer length (non-zero) 
		**	followed by a UCS2 (two byte integer) array of the indicated 
		**	length.  The end-of-segments indicator is a two byte integer 
		**	with value zero.
		**
		**	Only the SQL data/NULL indicator is read by this method.  If
		**	the SqlLongChar value is non-NULL, an Reader is provided for
		**	reading the segmented data stream.  The Reader also implements 
		**	the SqlStream StreamSource interface for providing notification 
		**	of stream closure events.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	SQL data value.
		**
		** Returns:
		**	void.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public void
			readSqlData( SqlLongNChar value )
		{
			if ( readSqlDataIndicator() )	// Read data indicator byte.
				value.set( new Ucs2SegRdr( this, trace, in_msg_id ) );
			else
				value.setNull();		// NULL data value.
			return;
		} // readSqlData


		/*
		** Name: readSqlDataIndicator
		**
		** Description:
		**	Reads a single byte (the SQL data/NULL indicator)
		**	from the current message and returns True if a data 
		**	value is indicated as being present (indicator is 
		**	non-zero, or not-NULL).
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean		True if indicator is non-zero.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		private bool
			readSqlDataIndicator()
		{
			return( readByte() != 0 );
		} // readSqlNullIndicator


		/*
		** Name: need
		**
		** Description:
		**	Ensures that the required amount of data is available
		**	in the input buffer.  A continuation message may be
		**	loaded if required to satisfy the request.  Atomic
		**	values require that the entire value be available in
		**	in a message (no splitting).  Non-atomic values may
		**	split and only require a portion of the value in the
		**	message.  
		**
		** Input:
		**	length	Amount of data required for request.
		**	atomic	True if value is atomic, false otherwise.
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

		private void
			need( int length, bool atomic )
		{
			byte msg_id = in_msg_id;

			/*
			** Nothing to do if data is in current message.
			*/
			while( length > in_msg_len )
			{
				/*
				** Atomic values are not split across continued messages
				** and must be completely available or no portion at all.
				** Non-atomic values are satisfied if any portion is
				** available.
				*/
				if ( in_msg_len > 0 )
					if ( ! atomic )
						break;	    // Data is availabe for non-atomic
					else
					{
						if ( trace.enabled( 1 ) )  
							trace.write( title + ": atomic value split (" + 
								in_msg_len + "," + length + ")" );
						disconnect();
						throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
					}

				/*
				** See if any more data is available.
				*/
				if ( ! moreData() )
				{
					if ( trace.enabled( 1 ) )  
						trace.write( title + ": unexpected end-of-data" );
					disconnect();
					throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
				}
			}

			return;
		} // need


		/*
		** Name: ByteSegIS
		**
		** Description:
		**	Class for converting segmented byte input into a stream.
		**
		**	This class extends InputStream to provide desegmentation 
		**	of a BLOB (binary or character), bufferring the stream 
		**	as needed.
		**
		**  Methods Overriden
		**
		**	read			Read bytes from segmented stream.
		**	skip			Skip bytes in segmented stream.
		**	available		Number of bytes in current segment.
		**	close			Close (flush) segmented stream.
		**
		**  Public Methods
		**
		**	addStreamListener	Register SQL Stream event listener.
		**
		**  Private Methods
		**
		**	next			Discard segment and begin next.
		**	endStream		Set state at end of stream.
		**
		**  Private Data
		**
		**	listener		SQL Stream event listener.
		**	stream			The SQL Stream.
		**	msg_in			Associated MsgIn.
		**	msg_id			Current message ID.
		**	end_of_data		End-of-data received?
		**	seg_len			Remaining data in current segment.
		**	ba			Temporary storage.
		**
		** History:
		**	29-Sep-99 (gordy)
		**	    Created.
		**	17-Nov-99 (gordy)
		**	    Made class nested (inner) and removed constructor.
		**	25-Jan-00 (gordy)
		**	    Added tracing.
		**	22-Sep-03 (gordy)
		**	    Added support for SqlStream stream events: listener,
		**	    stream, addStreamListener(), and endStream().  Removed
		**	    begin() method as now represents a single BLOB value.
		**	19-May-04 (thoda04)
		**	    Added Read and Close methods to override MemoryStream methods.
		*/

		private class
			ByteSegIS : InputStream, SqlStream.IStreamSource
		{

			private String	title;			// Class title for tracing.
			private ITrace	trace;			// Tracing.

			private MsgIn	msg_in;			// Db connection input.
			private byte	msg_id = 255;		// Current message ID.
			private bool	end_of_data = true;	// Segmented stream closed.
			private short	seg_len = 0;		// Current segment length
			private byte[]	ba = new byte[ 1 ];	// Temporary storage.

			private SqlStream.IStreamListener	listener = null;
			private SqlStream			stream = null;


			/*
			** Name: ByteSegIS
			**
			** Description:
			**	Class constructor.
			**
			** Input:
			**	msg_in	    Db connection input.
			**	trace	    Tracing output.
			**	msg_id	    Message ID.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	None.
			**
			** History:
			**	21-Apr-00 (gordy)
			**	    Created.
			**	22-Sep-03 (gordy)
			**	    Added msg_id parameter from former begin() method.
			*/

			public
				ByteSegIS( MsgIn msg_in, ITrace trace, byte msg_id )
					: base(InputStream.InputStreamNull)
			{
				this.msg_in = msg_in;
				this.trace = trace;
				this.msg_id = msg_id;
				end_of_data = false;
				title = "ByteSegIS[" + msg_in.ConnID + "]";
				if ( trace.enabled( 3 ) )  trace.write( title + ": start of BLOB" );
			} // ByteSegIS


			/*
			** Name: addStreamListener
			**
			** Description:
			**	Add a IStreamListener.  Only a single listener is supported.
			**
			** Input:
			**	listener	SqlStream listener.
			**	stream		SqlStream associated with source/listener.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	void.
			**
			** History:
			**	22-Sep-03 (gordy)
			**	    Created.
			*/

			public void 
				addStreamListener( SqlStream.IStreamListener listener, SqlStream stream )
			{
				this.listener = listener;
				this.stream = stream;
				return;
			} // addStreamListener


			/*
			** Name: read
			**
			** Description:
			**	Read a single byte from the segmented stream.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	int	Byte read, -1 at end-of-data.
			**
			** History:
			**	29-Sep-99 (gordy)
			**	    Created.
			*/

			public override int
				read()
			{
				int len = read( ba, 0, 1 );
				return( len == 1 ? (int)ba[ 0 ] & 0xff : -1 );
			} // read


			/*
			** Name: read
			**
			** Description:
			**	Read a byte sub-array from the segment stream.
			**
			** Input:
			**	offset	Offset in byte array.
			**	length	Number of bytes to read.
			**
			** Output:
			**	bytes	Byte array.
			**
			** Returns:
			**	int	Number of bytes read, or -1 if end-of-data.
			**
			** History:
			**	29-Sep-99 (gordy)
			**	    Created.
			*/

			public override int
				read( byte[] bytes, int offset, int length )
			{
				int len, total = 0;

				try
				{
					while( length > 0  &&  ! end_of_data )
					{
						if ( seg_len <= 0  &&  ! next() )  break;

						len = Math.Min( length, seg_len );
						len = msg_in.readBytes( bytes, offset, len );
						offset += len;
						length -= len;
						seg_len -= (short)len;
						total += len;
					}
				}
				catch( Exception ex )
				{
					if ( trace.enabled( 1 ) )
						trace.write( title + ": error reading data: " + ex.Message );
					throw new IOException( ex.Message ); 
				}

				if ( trace.enabled( 4 ) )  trace.write( title + ": BLOB read " + total );
				return( total > 0 ? total : (end_of_data ? -1 : 0) );
			} // read


			public override int
				Read( byte[] bytes, int offset, int length )
			{
				int count = this.read(bytes, offset, length);
				return (count == -1) ? 0 : count;
			}


			/*
			** Name: skip
			**
			** Description:
			**	Discard bytes from segmented stream.
			**
			** Input:
			**	length	    Number of bytes to skip.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	long	    Number of bytes skipped.
			**
			** History:
			**	29-Sep-99 (gordy)
			**	    Created.
			*/

			public long
				skip( long length )
			{
				long    total = 0;
				int	    len;

				try
				{
					while( length > 0  &&  ! end_of_data )
					{
						if ( seg_len <= 0  &&  ! next() )  break;
						if ( (len = seg_len) > length )  len = (int)length;
						len = msg_in.skip( len );
						length -= len;
						seg_len -= (short)len;
						total += len;
					}
				}
				catch( Exception ex )
				{ 
					if ( trace.enabled( 1 ) )
						trace.write( title + ": error skipping data: " + ex.Message );
					throw new IOException( ex.Message ); 
				}

				if ( trace.enabled( 4 ) )  trace.write( title + ": BLOB skipped " + total );
				return( total );
			} // skip


			/*
			** Name: available
			**
			** Description:
			**	Returns the number of bytes remaining in the current segment.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	int	Number of bytes available.
			**
			** History:
			**	29-Sep-99 (gordy)
			**	    Created.
			*/

			public int
				available()
			{
				return( seg_len );
			} // available


			/*
			** Name: close
			**
			** Description:
			**	Close the segmented stream.  If the end-of-data has not
			**	yet been reached, the stream is read and discarded until
			**	end-of-data is reached.
			**
			** Input:
			**	None.
			**
			** Ouput:
			**	None.
			**
			** Returns:
			**	void.
			**
			** History:
			**	29-Sep-99 (gordy)
			**	    Created.
			*/

			public override void
				close()
			{
				while( ! end_of_data )  next();
				return;
			} // close

			public override void
				Close()
			{
				this.close();
			}


			/*
			** Name: next
			**
			** Description:
			**	Discard current segment and begin next segment.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	boolean	    Next segment available (not end-of-data).
			**
			** History:
			**	29-Sep-99 (gordy)
			**	    Created.
			**	12-Apr-01 (gordy)
			**	    Pass tracing to exception.
			*/

			private bool
				next()
			{
				try
				{
					while( seg_len > 0 )  
						seg_len -= (short)msg_in.skip( seg_len );

					if ( ! msg_in.moreData() )
						if ( msg_in.receive() != msg_id )
						{
							endStream();
							throw new IOException( "BLOB data stream truncated!" );
						}

					if ( (seg_len = msg_in.readShort()) == 0 )
					{
						if ( trace.enabled( 3 ) )  trace.write( title + ": end of BLOB" );
						endStream();
					}
					else  if ( trace.enabled( 4 ) )
						trace.write( title + ": BLOB segment length " + seg_len );
				}
				catch( SqlEx ex )
				{
					if ( trace.enabled( 1 ) )
					{
						trace.write( title + ": exception reading next segment" );
						ex.trace( trace );
					}

					throw new IOException( ex.Message ); 
				}

				return( ! end_of_data );
			} // next


			/*
			** Name: endStream
			**
			** Description:
			**	Set state when end of stream is reached.
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
			**	22-Sep-03 (gordy)
			**	    Created.
			*/

			private void
				endStream()
			{
				if ( ! end_of_data )
				{
					/*
					** Set inactive state.
					*/
					end_of_data = true;
					seg_len = 0;
					msg_id = 255;

					/*
					** If a listener has been registered, provide
					** notification of the stream closure event.
					*/
					if ( listener != null )  
					{
						if ( trace.enabled( 4 ) )  
							trace.write( title + ": stream closure event notification" );
						listener.streamClosed( stream );
					}
				}
				return;
			} // endStream


		} // class ByteSegIS


		/*
		** Name: Ucs2SegRdr
		**
		** Description:
		**	Class for converting segmented UCS2 input into a stream.
		**
		**	This class extends Reader to provide desegmentation of a 
		**	BLOB (UCS2), bufferring the stream as needed.
		**
		**  Methods Overriden
		**
		**	read			Read bytes from segmented stream.
		**	skip			Skip bytes in segmented stream.
		**	ready			Is there data buffered.
		**	close			Close (flush) segmented stream.
		**
		**  Public Methods
		**
		**	addStreamListener	Register SQL Stream event listener.
		**
		**  Private Methods
		**
		**	next			Discard segment and begin next.
		**	endStream		Set state at end of stream.
		**
		**  Private Data
		**
		**	listener		SQL Stream event listener.
		**	stream			The SQL Stream.
		**	msg_in			Message Layer input.
		**	msg_id			Current message ID.
		**	end_of_data		End-of-data received?
		**	seg_len			Remaining data in current segment.
		**	ca			Temporary storage.
		**
		** History:
		**	10-May-01 (gordy)
		**	    Created.
		**	22-Sep-03 (gordy)
		**	    Added support for SqlStream stream events: listener,
		**	    stream, addStreamListener(), and endStream().  Removed
		**	    begin() method as now represents a single BLOB value.
		*/

		private class
			Ucs2SegRdr :  Reader, SqlStream.IStreamSource
		{

			private String	title;			// Class title for tracing.
			private ITrace	trace;			// Tracing.
    
			private MsgIn	msg_in;			// Message Layer input.
			private byte	msg_id = 255;		// Current message ID.
			private bool	end_of_data = true;	// Segmented stream closed.
			private short	seg_len = 0;		// Current segment length
			private char[]	ca = new char[ 1 ];	// Temporary storage.

			private SqlStream.IStreamListener	listener = null;
			private SqlStream			stream = null;


			/*
			** Name: Ucs2SegRdr
			**
			** Description:
			**	Class constructor.
			**
			** Input:
			**	msg_in	    Message layer input.
			**	trace	    Tracing output.
			**	msg_id	    Message ID.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	None.
			**
			** History:
			**	10-May-01 (gordy)
			**	    Created.
			**	22-Sep-03 (gordy)
			**	    Added msg_id parameter from former begin() method.
			*/

			public
				Ucs2SegRdr( MsgIn msg_in, ITrace trace, byte msg_id )
			{
				this.msg_in = msg_in;
				this.trace = trace;
				this.msg_id = msg_id;
				end_of_data = false;
				title = "Ucs2SegRdr[" + msg_in.ConnID + "]";
				if ( trace.enabled( 3 ) )  trace.write( title + ": start of BLOB" );
			} // Ucs2SegRdr


			/*
			** Name: addStreamListener
			**
			** Description:
			**	Add a IStreamListener.  Only a single listener is supported.
			**
			** Input:
			**	listener	SqlStream listener.
			**	stream		SqlStream associated with source/listener.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	void.
			**
			** History:
			**	22-Sep-03 (gordy)
			**	    Created.
			*/

			public void 
				addStreamListener( SqlStream.IStreamListener listener, SqlStream stream )
			{
				this.listener = listener;
				this.stream = stream;
				return;
			} // addStreamListener


			/*
			** Name: read
			**
			** Description:
			**	Read a single character from the segmented stream.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	int	Character read, -1 at end-of-data.
			**
			** History:
			**	10-May-01 (gordy)
			**	    Created.
			*/

			public override int
				read()
			{
				int len = read( ca, 0, 1 );
				return( len == 1 ? (int)ca[ 0 ] & 0xffff : -1 );
			} // read


			/*
			** Name: read
			**
			** Description:
			**	Read a character array from the segment stream.
			**
			** Input:
			**	None
			**
			** Output:
			**	cbuf	Character array.
			**
			** Returns:
			**	int	Number of characters read, or -1 if end-of-data.
			**
			** History:
			**	10-May-01 (gordy)
			**	    Created.
			*/

			public override int
				read( char[] cbuf )
			{
				return( read( cbuf, 0, cbuf.Length ) );
			}


			/*
			** Name: read
			**
			** Description:
			**	Read a character sub-array from the segment stream.
			**
			** Input:
			**	offset	Offset in character array.
			**	length	Number of characters to read.
			**
			** Output:
			**	cbuf	Character array.
			**
			** Returns:
			**	int	Number of characters read, or -1 if end-of-data.
			**
			** History:
			**	10-May-01 (gordy)
			**	    Created.
			*/

			public override int
				read( char[] cbuf, int offset, int length )
			{
				int len, total = 0;

				try
				{
					while( length > 0  &&  ! end_of_data )
					{
						if ( seg_len <= 0  &&  ! next() )  break;
						len = Math.Min( length, seg_len );

						for( int end = offset + len; offset < end; offset++ )
							cbuf[ offset ] = (char)msg_in.readShort();

						seg_len -= (short)len;
						length -= len;
						total += len;
					}
				}
				catch( Exception ex )
				{
					if ( trace.enabled( 1 ) )
						trace.write( title + ": error reading data: " + ex.Message );
					throw new IOException( ex.Message ); 
				}

				if ( trace.enabled( 4 ) )  trace.write( title + ": BLOB read " + total );
				return( total > 0 ? total : (end_of_data ? -1 : 0) );
			} // read


			/*
			** Name: skip
			**
			** Description:
			**	Discard characters from segmented stream.
			**
			** Input:
			**	length	    Number of characters to skip.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	long	    Number of characters skipped.
			**
			** History:
			**	10-May-01 (gordy)
			**	    Created.
			*/

			public long
				skip( long length )
			{
				long    total = 0;
				int	    len;

				try
				{
					while( length > 0  &&  ! end_of_data )
					{
						if ( seg_len <= 0  &&  ! next() )  break;
						if ( (len = seg_len) > length )  len = (int)length;
						len = msg_in.skip( len * 2 ) / 2;
						seg_len -= (short)len;
						length -= len;
						total += len;
					}
				}
				catch( Exception ex )
				{ 
					if ( trace.enabled( 1 ) )
						trace.write( title + ": error skipping data: " + ex.Message );
					throw new IOException( ex.Message ); 
				}

				if ( trace.enabled( 4 ) )  trace.write( title + ": BLOB skipped " + total );
				return( total );
			} // skip


			/*
			** Name: ready
			**
			** Description:
			**	Is data available.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	boolean	    True if data is available, false otherwise..
			**
			** History:
			**	10-May-01 (gordy)
			**	    Created.
			*/

			public bool
				ready()
			{
				return( seg_len > 0 );
			} // ready


			/*
			** Name: close
			**
			** Description:
			**	Close the segmented stream.  If the end-of-data has not
			**	yet been reached, the stream is read and discarded until
			**	end-of-data is reached.
			**
			** Input:
			**	None.
			**
			** Ouput:
			**	None.
			**
			** Returns:
			**	void.
			**
			** History:
			**	10-May-01 (gordy)
			**	    Created.
			*/

			public override void
				close()
			{
				while( ! end_of_data )  next();
				return;
			} // close


			/*
			** Name: next
			**
			** Description:
			**	Discard current segment and begin next segment.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	boolean	    Next segment available (not end-of-data).
			**
			** History:
			**	10-May-01 (gordy)
			**	    Created.
			*/

			private bool
				next()
			{
				try
				{
					while( seg_len > 0 )  
						seg_len -= (short)(msg_in.skip( seg_len * 2 ) / 2);

					if ( ! msg_in.moreData() )
						if ( msg_in.receive() != msg_id )
						{
							endStream();
							throw new IOException( "BLOB data stream truncated!" );
						}

					if ( (seg_len = msg_in.readShort()) == 0 )
					{
						if ( trace.enabled( 3 ) )  trace.write( title + ": end of BLOB" );
						endStream();
					}
					else  if ( trace.enabled( 4 ) )
						trace.write( title + ": BLOB segment length " + seg_len );
				}
				catch( SqlEx ex )
				{
					if ( trace.enabled( 1 ) )
					{
						trace.write( title + ": exception reading next segment" );
						ex.trace( trace );
					}

					throw new IOException( ex.Message ); 
				}

				return( ! end_of_data );
			} // next


			/*
			** Name: endStream
			**
			** Description:
			**	Set state when end of stream is reached.
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
			**	22-Sep-03 (gordy)
			**	    Created.
			*/

			private void
				endStream()
			{
				if ( ! end_of_data )
				{
					/*
					** Set inactive state.
					*/
					end_of_data = true;
					seg_len = 0;
					msg_id = 255;
    
					/*
					** If a listener has been registered, provide
					** notification of the stream closure event.
					*/
					if ( listener != null )
					{
						if ( trace.enabled( 4 ) )  
							trace.write( title + ": stream closure event notification" );
						listener.streamClosed( stream );
					}
				}
				return;
			} // endStream


		} // class Ucs2SegRdr


	} // class MsgIn

}