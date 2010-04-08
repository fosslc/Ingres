/*
** Copyright (c) 1999, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: inbuff.cs
	**
	** Description:
	**	Driver class InBuff which supports buffered input from the server 
	**	at the equivilent of the Ingres/Net Transport Layer level.
	**
	** History:
	**	16-Jun-99 (gordy)
	**	    Created.
	**	10-Sep-99 (gordy)
	**	    Moved test for TL DR to receive().
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	22-Sep-99 (gordy)
	**	    Added character set/char_encoding.
	**	29-Sep-99 (gordy)
	**	    Added the skip() method.
	**	25-Jan-00 (gordy)
	**	    Added some tracing.
	**	28-Jan-00 (gordy)
	**	    Added TL msg ID tracing.
	**	20-Apr-00 (gordy)
	**	    Moved to package io.
	**	 9-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	 6-Sep-02 (gordy)
	**	    Character encoding now encapsulated in CharSet class.
	**	20-Dec-02 (gordy)
	**	    Header ID now protocol level dependent.
	**	15-Mar-04 (gordy)
	**	    Added support for eight-byte integers.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/
	
	
	/*
	** Name: InBuff
	**
	** Description:
	**	Implements buffered input from server corresponding to the
	**	DAM-TL communications protocol.
	**
	**	The caller must provide multi-threading protection from the
	**	invocation of the receive() method until the entire message
	**	has been read.
	**
	**  Public Methods:
	**
	**	receive		Receive next message segment.
	**	avail		Determine amount of data left in buffer.
	**	readByte	Read a byte from the buffer/stream.
	**	readShort	Read a short from the buffer/stream.
	**	readInt		Read an integer from the buffer/stream.
	**	readFloat	Read a float from the buffer/stream.
	**	readDouble	Read a double from the buffer/stream.
	**	readBytes	Read a byte array from the buffer/stream.
	**	readString	Read a string from the buffer/stream.
	**	skip		Skip bytes in the buffer/stream.
	**
	**  Protected Data
	**
	**	trace	    DAM-TL tracing
	**
	**  Private Methods:
	**
	**	need		Make sure sufficient data is present.
	**
	** History:
	**	16-Jun-99 (gordy)
	**	    Created.
	**	29-Sep-99 (gordy)
	**	    Added the skip() method.
	**	 1-Oct-02 (gordy)
	**	    Added trace to override super-class to separate 
	**	    Transport and Network Layer tracing.
	**	15-Mar-04 (gordy)
	**	    Added method to read long values (readLong()).
	*/
	
	/// <summary>
	/// Implements buffered input from server corresponding to the
	/// DAM-TL communications protocol.
	/// </summary>
	class InBuff : IoBuff
	{

		new protected internal ITrace trace;    // DAM-TL tracing (overrides IoBuff)

		/*
		** Name: InBuff
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	in	Associated input stream.
		**	id	Connection ID.
		**	size	Length of input buffer.
		**
		** Output:
		**	None.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		*/
		
		/// <summary>
		/// Allocate an input stream and its buffer.
		/// </summary>
		/// <param name="inputStream">Associated input stream.</param>
		/// <param name="id">Connection ID.</param>
		/// <param name="size">Length of input buffer.</param>
		public InBuff(InputStream inputStream, int id, int size)
			:base(inputStream)
		{
			title = "InBuff[" + id + "]";
			trace = new Tracing(DAM_TL_TRACE_ID);
			BuffSize = size;
		}  // InBuff
		
		/*
		** Name: receive
		**
		** Description:
		**	Read the next message.  Any data remaining in the current
		**	message is discarded.  A server abort (TL disconnect request)
		**	is processed here and results in an exception being raised.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	short	    TL Message type.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		**	10-Sep-99 (gordy)
		**	    Check for TL disconnect request.
		**	28-Jan-00 (gordy)
		**	    Added TL msg ID tracing.
		**	20-Dec-02 (gordy)
		**	    Header ID now protocol level dependent.  Moved TL DR
		**	    processing to MSG classes where it is more appropriatte.
		*/
		
		/// <summary>
		/// Read the next message.  Any data remaining in the current
		/// message is discarded.
		/// </summary>
		/// <returns>TL Message type.</returns>
		public virtual short receive()
		{
			int   hdr_id;
			short msg_id;
			
			/*
			** Get the next segment in the input buffer.
			** If buffer is empty, receive next buffer.
			*/
			base.next();
			
			/*
			** Read and validate packet header: ID and type.
			*/
			hdr_id = readInt();

			if (
				(proto_lvl <= DAM_TL_PROTO_1  &&  hdr_id != DAM_TL_ID_1)  ||
				(proto_lvl >= DAM_TL_PROTO_2  &&  hdr_id != DAM_TL_ID_2)
				)
			{
				if (trace.enabled(1))
					trace.write(title + ": invalid TL header ID");
				close();
				throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
			}
			
			/*
			** Determine type of message.  A disconnect request is only
			** issued by the server when aborting the connection.  Handle
			** this exceptional case directly.
			*/
			msg_id = readShort();
			if (trace.enabled(3))
				trace.write(title + ": recv TL packet " +
					IdMap.map(msg_id, tlMap) + " length " + avail());
			
			return (msg_id);
		}  // receive
		
		/*
		** Name: avail
		**
		** Description:
		**	Returns the amount of unread data available in the
		**	current message segment.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Amount of data available.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		*/
		
		/// <summary>
		/// Returns the amount of unread data available in the
		/// current message segment.
		/// </summary>
		/// <returns>Amount of data available.</returns>
		public virtual int avail()
		{
			return (data_end - data_ptr);
		}
		// avail
		
		/*
		** Name: readByte
		**
		** Description:
		**	Read a byte from the input stream.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte	    Value from input stream.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		*/
		
		/// <summary>
		/// Read a byte from the input stream.
		/// </summary>
		/// <returns>Value from input stream.</returns>
		public virtual byte readByte()
		{
			need(1, true);
			return (buffer[data_ptr++]);
		}
		// readByte
		
		/*
		** Name: readSByte
		**
		** Description:
		**	Read a signed byte (tinyint) from the input stream.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte	Input signed byte value.
		**
		** History:
		**	 7-Nov-02 (thoda04)
		**	    Created.
		*/
		
		/// <summary>
		/// Read a signed byte (tinyint) from the input stream.
		/// </summary>
		/// <returns>Input signed byte value.</returns>
		public virtual SByte readSByte()
		{
			need(1, true);
			return (unchecked((SByte)(buffer[data_ptr++])));
		}  // readByte
		
		/*
		** Name: readShort
		**
		** Description:
		**	Read a short value from the input buffer.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	short	    Value from input stream.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		*/
		
		/// <summary>
		/// Read a short value from the input buffer.
		/// </summary>
		/// <returns>Value from input stream.</returns>
		public virtual short readShort()
		{
			need(2, true);
			return ((short) ((buffer[data_ptr++]       & 0x00ff) |
			                ((buffer[data_ptr++] << 8) & 0xff00)));
		}
		// readShort
		
		/*
		** Name: readInt
		**
		** Description:
		**	Read an integer value from the input buffer.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Value from input stream.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		*/
		
		/// <summary>
		/// Read an integer value from the input buffer.
		/// </summary>
		/// <returns>Value from input stream.</returns>
		public virtual int readInt()
		{
			need(4, true);
			return ((int) (buffer[data_ptr++]        & 0x000000ff) |
			             ((buffer[data_ptr++] <<  8) & 0x0000ff00) |
			             ((buffer[data_ptr++] << 16) & 0x00ff0000) |
			             ((buffer[data_ptr++] << 24) &
				(int) (- (0x100000000 - 0xff000000))));
		}
		// readInt


		/*
		** Name: readLong
		**
		** Description:
		**	Read a long value from the input buffer.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	long	    Value from input stream.
		**
		** History:
		**	15-Mar-04 (gordy)
		**	    Created.
		**	28-Apr-04 (thoda04)
		**	    Ported to .NET.
		*/

		public long
			readLong()
		{
			need( 8, true );
			return( unchecked ((long)(
				(((ulong)buffer[ data_ptr++ ])       & 0x00000000000000ffL) |
				(((ulong)buffer[ data_ptr++ ] << 8)  & 0x000000000000ff00L) |
				(((ulong)buffer[ data_ptr++ ] << 16) & 0x0000000000ff0000L) |
				(((ulong)buffer[ data_ptr++ ] << 24) & 0x00000000ff000000L) |
				(((ulong)buffer[ data_ptr++ ] << 32) & 0x000000ff00000000L) |
				(((ulong)buffer[ data_ptr++ ] << 40) & 0x0000ff0000000000L) |
				(((ulong)buffer[ data_ptr++ ] << 48) & 0x00ff000000000000L) |
				(((ulong)buffer[ data_ptr++ ] << 56) & 0xff00000000000000L) )
				));
		} // readLong


		/*
		** Name: readFloat
		**
		** Description:
		**	Read a float value from the input buffer.
		**
		** Input:
		**	Four-byte floats are sent as 6 binary bytes as follows:
		**	    F4:[E7:E0][S,E14:E8][M23:M16][M31:M24][M7:M0][M15:M8].
		**
		**	The sign is a single bit: 1 for negative, 0 for positive.
		**	The exponent is a 15-bit unsigned integer, biased by 16382.
		**	The mantissa is a 32-bit fraction normalized with the
		**	redundant most significant bit not represented.
		**
		** Processing:
		**	Convert the value from the network stream to 
		**	a .NET float value (System.Single)
		**	which conforms to IEEE 754 format.
		**
		** Returns:
		**	float	    Value from input stream.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		**	12-Aug-02 (thoda04)
		**	    Ported for .NET Provider.
		*/
		
		/// <summary>
		/// Read an float value from the input buffer.
		/// </summary>
		/// <returns>Value from input stream.</returns>
		public virtual float readFloat()
		{
			float floatValue = System.Single.NaN;
			uint  man;   // mantissa
			short exp;   // exponent
			byte  sign;  // sign
			
			/*
			** Extract components from network format.
			*/
			need(DAM_TL_F4_LEN, true);

			sign = (byte)((buffer[data_ptr + 1] >> 7)  & 0x00000001);
			exp =(short)(((buffer[data_ptr + 1] << 8)  & 0x00007f00) |
			              (buffer[data_ptr + 0]        & 0x000000ff));
			man =  (uint)((buffer[data_ptr + 3] << 24) & 0xff000000) |
			       (uint)((buffer[data_ptr + 2] << 16) & 0x00ff0000) |
			       (uint)((buffer[data_ptr + 5] << 8)  & 0x0000ff00) |
			       (uint)((buffer[data_ptr + 4]        & 0x000000ff));

			data_ptr += DAM_TL_F4_LEN;
			
			/*
			** Re-bias the exponent.
			*/
			if (exp != 0)
				exp = (short) (exp + (126 - 16382));  // adjust to IEEE bias (126)
			
			if (exp < 0 || exp > 255)
				floatValue = (sign != 0)?
					System.Single.MinValue:
					System.Single.MaxValue;
			else
			{
				/*
				** Rebuild the float from the components.
				*/
				uint bits;
				bits = (uint)((sign << 31) & 0x80000000) |
				       (uint)(( exp << 23) & 0x7f800000) |
				       (uint)(( man >> 9)  & 0x007fffff);
				
				floatValue = IoBuff.Float.intBitsToFloat(bits);
			}
			
			return (floatValue);
		}
		// readFloat
		
		/*
		** Name: readDouble
		**
		** Description:
		**	Read a double value from the input buffer.
		**
		** Input:
		**	Eight-byte floats are sent as 10 binary bytes as follows:
		**	    F4:[E7:E0][S,E14:E8][M55:M48][M63:M56][M39:M32][M47:M40]
		**	           [M23:M16][M31:M24][M7:M0][M15:M8].
		**
		**	The sign is a single bit: 1 for negative, 0 for positive.
		**	The exponent is a 15-bit unsigned integer, biased by 16382.
		**	The mantissa is a 64-bit fraction normalized with the
		**	redundant most significant bit not represented.
		**
		** Processing:
		**	Convert the value from the network stream to 
		**	a .NET float value (System.Double)
		**	which conforms to IEEE 754 format.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	double	    Value from input stream.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		**	12-Aug-02 (thoda04)
		**	    Ported for .NET Provider.
		*/
		
		/// <summary>
		/// Read an double value from the input buffer.
		/// </summary>
		/// <returns>Value from input stream.</returns>
		public virtual double readDouble()
		{
			double doubleValue = System.Double.NaN;
			ulong man;   // mantissa
			short exp;   // exponent
			byte sign;   // sign
			
			/*
			** Extract components from network format.
			*/
			need(DAM_TL_F8_LEN, true);

			sign = (byte) ((buffer[data_ptr + 1] >> 7)  & 0x00000001);
			exp = (short)(((buffer[data_ptr + 1] << 8)  & 0x00007f00) |
			               (buffer[data_ptr + 0]        & 0x000000ff));
			man = (((ulong) buffer[data_ptr + 3] << 56) & 0xff00000000000000L) |
			      (((ulong) buffer[data_ptr + 2] << 48) & 0x00ff000000000000L) |
			      (((ulong) buffer[data_ptr + 5] << 40) & 0x0000ff0000000000L) |
			      (((ulong) buffer[data_ptr + 4] << 32) & 0x000000ff00000000L) |
			      (((ulong) buffer[data_ptr + 7] << 24) & 0x00000000ff000000L) |
			      (((ulong) buffer[data_ptr + 6] << 16) & 0x0000000000ff0000L) |
			      (((ulong) buffer[data_ptr + 9] <<  8) & 0x000000000000ff00L) |
			      ( (ulong) buffer[data_ptr + 8]        & 0x00000000000000ffL);

			data_ptr += DAM_TL_F8_LEN;
			
			/*
			** Re-bias the exponent.
			*/
			if (exp != 0)
				exp = (short) (exp + (1022 - 16382));  // adjust to IEEE bias (1022)
			
			if (exp < 0 || exp > 2047)
				doubleValue = (sign != 0)?
					System.Double.MinValue:
					System.Double.MaxValue;
			else
			{
				/*
				** Rebuild the double from the components.
				*/
				ulong bits;
				bits = (((ulong) sign << 63) & 0x8000000000000000L) |
				       (((ulong) exp  << 52) & 0x7ff0000000000000L) |
				                (man  >> 12) & 0x000fffffffffffffL;

				doubleValue = IoBuff.Double.longBitsToDouble(bits);
			}

			return (doubleValue);
		}
		// readDouble

		/*
		** Name: readBytes
		**
		** Description:
		**	Read a byte array from the input stream.  There must be
		**	sufficient space in output buffer to receive the requested
		**	data.  Actual number of bytes read is limited to contents
		**	of current input buffer.
		**
		** Input:
		**	offset	    Offset in byte array for data.
		**	length	    Number of bytes to place in array.
		**
		** Output:
		**	bytes	    Bytes read from input stream.
		**
		** Returns:
		**	int	    Number of bytes actually read.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		**	22-Sep-99 (gordy)
		**	    Caller must provide valid length (no more discard).
		**	    Changed return semantics to permit processing of split
		**	    arrays.*/

		/// <summary>
		/// Read a byte array from the input stream.  There must be
		/// sufficient space in output buffer to receive the requested
		/// data.  Actual number of bytes read is limited to contents
		/// of current input buffer.
		/// </summary>
		/// <param name="bytes">Output Byte array to hold bytes
		/// read from input stream.</param>
		/// <param name="offset">Offset in byte array for data.</param>
		/// <param name="length">Number of bytes to place in array.</param>
		/// <returns>Number of bytes actually read.</returns>
		public virtual int readBytes(byte[] bytes, int offset, int length)
		{
			need(length, false); // Make sure some data is available.
			length = System.Math.Min(length, avail());

			Array.Copy(buffer, data_ptr, bytes, offset, length);
			
			data_ptr += length;
			return (length);
		}
		// readBytes


		/*
		** Name: readBytes
		**
		** Description:
		**	Read a ByteArray from the input stream.  ByteArray limits
		**	are ignored.  Any requested data which is available which
		**	exceeds the array limit is read and discarded.  It is the
		**	callers responsibility to ensure adaquate array limits 
		**	and capacity.  Actual number of bytes read is limited to 
		**	contents of current input buffer.
		**
		** Input:
		**	length	    Number of bytes to place in array.
		**
		** Output:
		**	bytes	    BytesArray filled from input stream
		**
		** Returns:
		**	int	    Number of bytes actually read from input stream.
		**
		** History:
		**	22-Sep-03 (gordy)
		**	    Created.
		*/

		public int
			readBytes( IByteArray bytes, int length )
		{
			need( length, false );	// Make sure some data is available.
			length = Math.Min( length, avail() );
			bytes.put( buffer, data_ptr, length );  // Array limit overruns ignored!
			data_ptr += length;
			return( length );
		} // readBytes


		/*
		** Name: readString
		**
		** Description:
		**	Read a string from the input stream.  The string is treated
		**	as an atomic value and the entire requested length must be
		**	available.  The input bytes are converted to Unicode using 
		**	the character encoding provided.  Strings which are split
		**	must be read as byte arrays.
		**
		** Input:
		**	length  	Number of bytes in input string.
		**	char_set	Character encoding for conversion.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    String value from input stream.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		**	22-Sep-99 (gordy)
		**	    Added character set/encoding.*/
		
		/// <summary>
		/// Read a string from the input stream.  The string is treated
		/// as an atomic value and the entire requested length must be
		/// available.  The input bytes are converted to Unicode using
		/// the character encoding provided.  Strings which are split
		/// must be read as byte arrays.
		/// </summary>
		/// <param name="length">Number of bytes in input string.</param>
		/// <param name="char_set">Character encoding for conversion.</param>
		/// <returns>String value from input stream.</returns>
		public virtual String readString(int length, CharSet char_set)
		{
			String str;
			need(length, true);
			
			try { str = char_set.getString( buffer, data_ptr, length ); }
			catch( Exception ex ) 
			{
				throw SqlEx.get( ERR_GC401E_CHAR_ENCODE, ex ); // Should not happen!
			}
			
			data_ptr += length;
			return (str);
		}  // readString
		

		/*
		** Name: skip
		**
		** Description:
		**	Skip bytes in the input stream.  Actual number of bytes 
		**	skipped is limited to contents of current input buffer.
		**
		** Input:
		**	length	    Number of bytes to skip.
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
		
		/// <summary>
		/// Skip bytes in the input stream.  Actual number of bytes
		/// skipped is limited to contents of current input buffer.
		/// </summary>
		/// <param name="length">Number of bytes to skip.</param>
		/// <returns>Number of bytes actually skipped.</returns>
		public virtual int skip(int length)
		{
			need(length, false); // Make sure some data is available.
			length = System.Math.Min(length, avail());
			data_ptr += length;
			return (length);
		}
		// skip
		
		/*
		** Name: need
		**
		** Description:
		**	Determines if sufficient data is available to read an
		**	object of a particular size.  Atomic objects may not be 
		**	split across buffers.  Non-atomic objects may be split
		**	and may need to be read piece by piece.
		**
		** Input:
		**	size	    Amount of data required.
		**	atomic	    True if data may not be split.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		**	10-Sep-99 (gordy)
		**	    Moved check for TL disconnect request to receive().
		**	22-Sep-99 (gordy)
		**	    Added atomic indicating to permit handling of split arrays.
		**	25-Jan-00 (gordy)
		**	    Added tracing.*/
		
		/// <summary>
		/// Determines if sufficient data is available to read an
		/// object of a particular size.  Atomic objects may not be
		/// split across buffers.  Non-atomic objects may be split
		/// and may need to be read piece by piece.
		/// </summary>
		/// <param name="size">Amount of data required.</param>
		/// <param name="atomic">True if data may not be split.</param>
		private void  need(int size, bool atomic)
		{
			int available;
			
			while ((available = avail()) < size)
			{
				short tl_id;
				
				if (available > 0)
					if (!atomic)
						break;
					else
					{
						if (trace.enabled(1))
							trace.write(title + ": atomic value split (" +
							            available + "," + size + ")");
						close();
						throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
					}
				
				if ((tl_id = receive()) != DAM_TL_DT)
				{
					if (trace.enabled(1))
						trace.write(title + ": invalid TL packet ID 0x" +
						            System.Convert.ToString(tl_id, 16));
					close();
					throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
				}
			}
			
			if (trace.enabled(5))
				if (available < size)
					trace.write(title + ": reading " + available + " (" + size + " requested)");
				else
					trace.write(title + ": reading " + size);
			
			return ;
		} // need


	}  // class InBuff
}