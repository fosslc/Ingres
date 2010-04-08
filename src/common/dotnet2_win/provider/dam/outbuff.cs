/*
** Copyright (c) 1999, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: outbuff.cs
	**
	** Description:
	**	Defines class which supports buffered output to the server 
	**	for the Data Access Messaging - Transport Layer protocol.
	**
	** History:
	**	16-Jun-99 (gordy)
	**	    Created.
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	22-Sep-99 (gordy)
	**	    Removed write( String ) method since it did not handle
	**	    character set/encoding.
	**	29-Sep-99 (gordy)
	**	    Added write( InputStream ) method to fill current output
	**	    buffer from the input stream.
	**	28-Jan-00 (gordy)
	**	    Replaced redundant code with check() method.
	**	    Added TL msg ID tracing.
	**	20-Apr-00 (gordy)
	**	    Moved to package io.
	**	14-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	20-Dec-02 (gordy)
	**	    Header ID protocol level dependent.
	**	15-Mar-04 (gordy)
	**	    Added support for eight-byte integers.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/
	
	
	/*
	** Name: OutBuff
	**
	** Description:
	**	Implements buffered output to server corresponding to the
	**	DAM-TL communications protocol.  The output may be treated as
	**	a stream (internal buffering) or the output buffer may be
	**	accessed directly.
	**
	**	A message is created by first invoking the begin() method,
	**	followed by any number of calls to write the body of the
	**	message, and finished by invoking the flush() method.  It
	**	is the callers responsibility to provide multi-threaded
	**	protection from just prior to invoking begin() until after
	**	the invocation of flush().
	**
	**  Public Methods:
	**
	**	begin	    Start a new message.
	**	avail	    Returns amount of remaining space in buffer.
	**	position    Current position in output buffer.
	**	write	    Append/write values to output buffer.
	**	flush	    Send output buffer to server.
	**
	**  Protected Data
	**
	**	trace	    DAM-TL tracing
	**
	**  Private Methods:
	**
	**	need	    Ensure space in output buffer.
	**
	** History:
	**	16-Jun-99 (gordy)
	**	    Created.
	**	29-Sep-99 (gordy)
	**	    Added write( InputStream ) method to fill current output
	**	    buffer from the input stream.
	**	 1-Oct-02 (gordy)
	**	    Added trace to override super-class to separate 
	**	    Transport and Network Layer tracing.
	**	15-Mar-04 (gordy)
	**	    Added write methods for long values.
	*/

	/// <summary>
	/// Implements buffered output to server corresponding to the
	/// DAM-TL communications protocol.  The output may be treated as
	/// a stream (internal buffering) or the output buffer may be
	/// accessed directly.
	/// </summary>
	class OutBuff : IoBuff
	{

		new protected internal ITrace trace;    // DAM-TL tracing (overrides IoBuff)

		/*
		** Name: OutBuff
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	out	Associated output stream.
		**	id	Connection ID.
		**	size	Length of output buffer.
		**
		** Output:
		**	None.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		**	 1-Oct-02 (gordy)
		**	    Buffer size now set explicitly rather than super-class 
		**	    initialization.
		*/

		/// <summary>
		/// Create an OutputStream buffer with spwecified size.
		/// </summary>
		/// <param name="outputStream">Associated output stream.</param>
		/// <param name="id">Connection ID.</param>
		/// <param name="size">Length of output buffer.</param>
		public OutBuff(OutputStream outputStream, int id, int size)
			:base(outputStream)
		{
			title = "OutBuff[" + id + "]";
			trace = new Tracing(DAM_TL_TRACE_ID);
			BuffSize = size;
		}  // OutBuff


		/*
		** Name: begin
		**
		** Description:
		**	Start a new message of the requested type.
		**
		** Input:
		**	type	Message type.
		**	size	Minimum buffer space required for message.
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
		**	28-Jan-00 (gordy)
		**	    Added TL msg ID tracing.
		**	20-Dec-02 (gordy)
		**	    Header ID protocol level dependent.
		*/

		/// <summary>
		/// Start a new message of the requested type
		/// with the minimum buffer space required for the message.
		/// </summary>
		/// <param name="type">Message type.</param>
		/// <param name="size">Minimum buffer space required for message.</param>
		public virtual void  begin(short type, int size)
		{
			size += 6;     // Include TL header

			if (trace.enabled(3))
			{
				trace.write(title + ": begin TL packet " + IdMap.map(type, tlMap));
				if (trace.enabled(5))
					trace.write(title + ": reserving " + size);
			}
			
			base.begin(size);
			// write TL header
			write( (int)((proto_lvl == DAM_TL_PROTO_1) ?
				DAM_TL_ID_1 :
				DAM_TL_ID_2) );
			write((short) type);
			return;
		}  // begin


		/*
		** Name: avail
		**
		** Description:
		**	Determine amount of unused space in output buffer.
		**	Information returned is only valid after calling
		**	begin() and prior to calling flush().
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Amount of unused space.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Determine amount of unused space in output buffer.
		/// Information returned is only valid after calling
		/// begin() and prior to calling flush().
		/// </summary>
		/// <returns>Amount of unused space.</returns>
		public virtual int avail()
		{
			return (data_end - data_ptr);
		}  // avail


		/*
		** Name: position
		**
		** Description:
		**	Returns the current position in the output buffer.
		**	Position is only valid after calling begin() and
		**	prior to calling flush().  An appending write()
		**	call may also invalidate the position (if length 
		**	written exceeds avail() amount).
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Current position.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Returns the current position in the output buffer.
		/// Position is only valid after calling begin() and
		/// prior to calling flush().  An appending write()
		/// call may also invalidate the position (if length
		/// written exceeds avail() amount).
		/// </summary>
		/// <returns>Current position.</returns>
		public virtual int position()
		{
			return (data_ptr);
		}  // position


		/*
		** Name: write (byte)
		**
		** Description:
		**	Append a byte value in the output buffer.
		**
		** Input:
		**	byteValue	    Byte value.
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
		*/

		/// <summary>
		/// Append a byte value in the output buffer.
		/// </summary>
		/// <param name="byteValue">Byte value.</param>
		public virtual void  write(byte byteValue)
		{
			need(1);
			write(data_ptr, byteValue);
			return;
		}  // write

		public virtual void  write(SByte byteValue)
		{
			write(unchecked((byte)byteValue));
		}  // write

		/*
		** Name: write (byte) at a position
		**
		** Description:
		**	Write a byte value in the output buffer at a particular position.
		**
		** Input:
		**	position 	    Buffer position.
		**	byteValue	    Byte value.
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
		**	28-Jan-00 (gordy)
		**	    Replaced redundat code with check() method.
		*/

		/// <summary>
		/// Write a byte value in the output buffer at a particular position.
		/// </summary>
		/// <param name="position">Buffer position.</param>
		/// <param name="byteValue">Byte value.</param>
		public virtual void  write(int position, byte byteValue)
		{
			check(position, 1);
			buffer[position++] = byteValue;
			if (position > data_ptr)
				data_ptr = position;
			return;
		}  // write

		public virtual void write(int position, SByte byteValue)
		{
			write(position, unchecked((byte)byteValue));
		}  // write


		/*
		** Name: write (short)
		**
		** Description:
		**	Append a short value in the output buffer.
		**
		** Input:
		**	shortValue	    Short value.
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
		*/

		/// <summary>
		/// Append a short value in the output buffer.
		/// </summary>
		/// <param name="shortValue">Short value.</param>
		public virtual void  write(short shortValue)
		{
			need(2);
			write(data_ptr, shortValue);
			return;
		}  // write 


		/*
		** Name: write (short) at a position
		**
		** Description:
		**	Write a short value in the output buffer at a particular position.
		**
		** Input:
		**	position  	Buffer position.
		**	shortValue	Short value.
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
		**	28-Jan-00 (gordy)
		**	    Replaced redundat code with check() method.
		*/

		/// <summary>
		/// Write a short value in the output buffer at a particular position.
		/// </summary>
		/// <param name="position">Buffer position.</param>
		/// <param name="shortValue">Short value.</param>
		public virtual void  write(int position, short shortValue)
		{
			check(position, 2);
			buffer[position++] = (byte) ( shortValue       & 0xff);
			buffer[position++] = (byte) ((shortValue >> 8) & 0xff);
			if (position > data_ptr)
				data_ptr = position;
			return;
		}  // write


		/*
		** Name: write (integer)
		**
		** Description:
		**	Append an integer value in the output buffer.
		**
		** Input:
		**	intValue	    Integer value.
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
		*/

		/// <summary>
		/// Append an integer value in the output buffer.
		/// </summary>
		/// <param name="intValue">Integer value.</param>
		public virtual void  write(int intValue)
		{
			need(4);
			write(data_ptr, intValue);
			return;
		}  // write


		/*
		** Name: write (integer) at a position
		**
		** Description:
		**	Write an integer value in the output buffer at a particular position.
		**
		** Input:
		**	position    Buffer position.
		**	intValue	    Integer value.
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
		**	28-Jan-00 (gordy)
		**	    Replaced redundat code with check() method.
		*/

		/// <summary>
		/// Write an integer value in the output buffer at a particular position.
		/// </summary>
		/// <param name="position">Buffer position.</param>
		/// <param name="intValue">Integer value.</param>
		public virtual void  write(int position, int intValue)
		{
			check(position, 4);
			buffer[position++] = (byte) ( intValue        & 0xff);
			buffer[position++] = (byte) ((intValue >>  8) & 0xff);
			buffer[position++] = (byte) ((intValue >> 16) & 0xff);
			buffer[position++] = (byte) ((intValue >> 24) & 0xff);

			if (position > data_ptr)
				data_ptr = position;
			return;
		}  // write


		/*
		** Name: write
		**
		** Description:
		**	Append a long value in the output buffer.
		**
		** Input:
		**	value	    Long value.
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
		/// Write an bigint value into the output buffer.
		/// </summary>
		/// <param name="value"></param>
		public virtual void
			write( long value )
		{
			need( 8 );
			write( data_ptr, value );
			return;
		} // write


		/*
		** Name: write
		**
		** Description:
		**	Write a long value in the output buffer.
		**
		** Input:
		**	position    Buffer position.
		**	value	    Long value.
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
		/// Write an bigint value into the output buffer at a particular position.
		/// </summary>
		/// <param name="position"></param>
		/// <param name="value"></param>
		public virtual void
			write( int position, long value )
		{
			check( position, 8 );
			buffer[ position++ ] = (byte) (value         & 0xff);
			buffer[ position++ ] = (byte)((value >> 8 )  & 0xff);
			buffer[ position++ ] = (byte)((value >> 16 ) & 0xff);
			buffer[ position++ ] = (byte)((value >> 24 ) & 0xff);
			buffer[ position++ ] = (byte)((value >> 32 ) & 0xff);
			buffer[ position++ ] = (byte)((value >> 40 ) & 0xff);
			buffer[ position++ ] = (byte)((value >> 48 ) & 0xff);
			buffer[ position++ ] = (byte)((value >> 56 ) & 0xff);
			if ( position > data_ptr )  data_ptr = position;
			return;
		} // write


		/*
		** Name: write (float)
		**
		** Description:
		**	Append a float value in the output buffer.
		**
		** Input:
		**	floatValue	    Float value.
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
		*/

		/// <summary>
		/// Append a float value in the output buffer.
		/// </summary>
		/// <param name="floatValue">float value.</param>
		public virtual void  write(float floatValue)
		{
			need(DAM_TL_F4_LEN);
			write(data_ptr, floatValue);
			return;
		}  // write


		/*
		** Name: write (float) at a position
		**
		** Description:
		**	Write a float value in the output buffer.
		**
		** Input:
		**	position  	    Buffer position.
		**	floatValue	    Float value.
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
		**	28-Jan-00 (gordy)
		**	    Replaced redundat code with check() method.
		**	14-Aug-02 (thoda04)
		**	    Ported for .NET Provider.
		*/

		/// <summary>
		/// Write an float value in the output buffer at a particular position.
		/// </summary>
		/// <param name="position">Buffer position.</param>
		/// <param name="floatValue">float value.</param>
		public virtual void  write(int position, float floatValue)
		{
			uint  bits, man;
			short exp;
			byte  sign;
			
			check(position, DAM_TL_F4_LEN);
			
			/*
			** Extract bits of each component.
			*/
			bits = Float.floatToIntBits(floatValue);
			sign = (byte) ((bits & 0x80000000) >> 31);
			exp = (short) ((bits & 0x7f800000) >> 23);
			man =  (uint) ((bits & 0x007fffff) << 9);
			
			/*
			** Re-bias the exponent
			*/
			if (exp != 0)
				exp = (short) (exp - (126 - 16382));
			
			/*
			** Pack the bits in network format.
			** (note that byte pairs are swapped).
			*/
			buffer[position + 1] = (byte) ((sign << 7) | (exp >> 8));
			buffer[position + 0] = (byte) exp;
			buffer[position + 3] = (byte) (man >> 24);
			buffer[position + 2] = (byte) (man >> 16);
			buffer[position + 5] = (byte) (man >>  8);
			buffer[position + 4] = (byte) man;

			position += DAM_TL_F4_LEN;

			if (position > data_ptr)
				data_ptr = position;
			
			return;
		}  // write


		/*
		** Name: write
		**
		** Description:
		**	Append a double value in the output buffer.
		**
		** Input:
		**	doubleValue	    Double value.
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
		*/

		/// <summary>
		/// Append an double value into the output buffer.
		/// </summary>
		/// <param name="doubleValue">float value.</param>
		internal virtual void  write(double doubleValue)
		{
			need(DAM_TL_F8_LEN);
			write(data_ptr, doubleValue);
			return;
		}  // write


		/*
		** Name: write
		**
		** Description:
		**	Write a double value in the output buffer.
		**
		** Input:
		**	position    Buffer position.
		**	doubleValue	    Double value.
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
		**	28-Jan-00 (gordy)
		**	    Replaced redundat code with check() method.
		**	14-Aug-02 (thoda04)
		**	    Ported for .NET Provider.
		*/

		/// <summary>
		/// Write an double value in the output buffer at a particular position.
		/// </summary>
		/// <param name="position">Buffer position.</param>
		/// <param name="doubleValue">float value.</param>
		public virtual void  write(int position, double doubleValue)
		{
			ulong bits, man;
			short exp;
			byte  sign;
			
			check(position, DAM_TL_F8_LEN);
			
			/*
			** Extract bits of each component.
			*/
			bits = Double.doubleToLongBits(doubleValue);
			sign = (byte) ((bits & 0x8000000000000000L) >> 63);
			exp = (short) ((bits & 0x7ff0000000000000L) >> 52);
			man =          (bits & 0x000fffffffffffffL) << 12;
			
			/*
			** Re-bias the exponent
			*/
			if (exp != 0)
				exp = (short) (exp - (1022 - 16382));
			
			/*
			** Pack the bits in network format.
			** (note that byte pairs are swapped).
			*/
			buffer[position + 1] = (byte) ((sign << 7) | (exp >> 8));
			buffer[position + 0] = (byte)  exp;
			buffer[position + 3] = (byte) (man >> 56);
			buffer[position + 2] = (byte) (man >> 48);
			buffer[position + 5] = (byte) (man >> 40);
			buffer[position + 4] = (byte) (man >> 32);
			buffer[position + 7] = (byte) (man >> 24);
			buffer[position + 6] = (byte) (man >> 16);
			buffer[position + 9] = (byte) (man >>  8);
			buffer[position + 8] = (byte)  man;

			position += DAM_TL_F8_LEN;
			if (position > data_ptr)
				data_ptr = position;

			return;
		}  // write


		/*
		** Name: write (byteArray) at a position
		**
		** Description:
		**	Append a byte array to the output buffer.  This
		**	routine does not split arrays across buffers, the
		**	current buffer is flushed and the array must fit
		**	in the new buffer, or an exception is thrown.
		**
		** Input:
		**	byteArray	    Byte array.
		**	offset   	    Position of data in array.
		**	length   	    Length of data in array.
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
		*/

		/// <summary>
		/// Append a byte array to the output buffer.  This
		/// routine does not split arrays across buffers, the
		/// current buffer is flushed and the array must fit
		/// in the new buffer, or an exception is thrown.
		/// </summary>
		/// <param name="byteArray">Byte array.</param>
		/// <param name="offset">Position of data in array to read from.</param>
		/// <param name="length">Length of data in array.</param>
		public virtual void  write(byte[] byteArray, int offset, int length)
		{
			if (!need(length))
			{
				if (trace.enabled(1))
					trace.write(title + ": array length " + length + " too long");
				close();
				throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
			}
			
			if (trace.enabled(5))
				trace.write(title + ": appending " + length);
			try
			{
				Array.Copy(byteArray, offset, buffer, data_ptr, length);
			}
			catch (System.Exception ex)
			{
				// Should not happen!
				throw SqlEx.get(title + ": array copy exception.  " +
					ex.Message, "HY000", 0);
			}
			
			data_ptr += length;
			return;
		}  // write


		/*
		** Name: write
		**
		** Description:
		**	Append a portion of a ByteArray to the output buffer.  
		**	Number of bytes copied from ByteArray may be limited 
		**	by current length of the array and requested position.  
		**	This routine does not split arrays across buffers, the 
		**	current buffer is flushed and the array must fit in 
		**	the new buffer, or an exception is thrown.
		**
		** Input:
		**	bytes	ByteArray.
		**	offset	Position of data in array.
		**	length	Length of data in array.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Number of bytes appended.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public int
		write(IByteArray bytes, int offset, int length)
		{
			if (!need(length))
			{
				if (trace.enabled(1))
					trace.write(title + ": ByteArray length " + length + " too long");
				close();
				throw SqlEx.get(ERR_GC4002_PROTOCOL_ERR);
			}

			if (trace.enabled(4)) trace.write(title + ": appending " + length);
			length = bytes.get(offset, length, buffer, data_ptr);
			data_ptr += length;
			return (length);
		} // write


		/*
		** Name: write (inputStream)
		**
		** Description:
		**	Append input stream to the output buffer.  This
		**	routine does not split buffers, the input stream
		**	is read until the buffer is filled or the stream
		**	is exhausted.  The number of bytes read from the 
		**	input stream is returned.  It is the callers
		**	responsibility to assure that there is space
		**	available in the output buffer.
		**
		** Input:
		**	inputStream	InputStream
		**
		** Output:
		**	None.
		**
		** Returns:
		**	short	Number of bytes written.
		**
		** History:
		**	29-Sep-99 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Append input stream to the output buffer.  This
		/// routine does not split buffers, the input stream
		/// is read until the buffer is filled or the stream
		/// is exhausted.  The number of bytes read from the
		/// input stream is returned.  It is the callers
		/// responsibility to assure that there is space
		/// available in the output buffer.
		/// </summary>
		/// <param name="inputStream"></param>
		/// <returns></returns>
		public virtual short write(InputStream inputStream)
		{
			int length = avail();
			
			try
			{
				length = inputStream.read(buffer, data_ptr, length);
			}
			catch (System.Exception ex)
			{
				throw SqlEx.get( ERR_GC4007_BLOB_IO, ex);
			}
			
			if (length < 0)
				length = 0;
			data_ptr += length;
			if (trace.enabled(5))
				trace.write(title + ": appending " + length);
			return ((short) length);
		}  // write


		/*
		** Name: flush
		**
		** Description:
		**	Send buffer to server.
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
		**	16-Jun-99 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Send buffer to server.
		/// </summary>
		protected internal override void  flush()
		{
			if (trace.enabled(3))
				trace.write(title + ": sending TL packet; length= " +
					(data_ptr - data_beg));

			base.flush();
			return;
		}  // flush


		/*
		** Name: need
		**
		** Description:
		**	Make sure enough room is available in the output
		**	buffer for an object of the requested size.  If 
		**	the current buffer is full, it will be flushed and 
		**	a new DATA message started.
		**
		** Input:
		**	size	    Amount of space required.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if room is available, false otherwise.
		**
		** History:
		**	16-Jun-99 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Make sure enough room is available in the output
		/// buffer for an object of the requested size.  If
		/// the current buffer is full, it will be flushed and
		/// a new DATA message started.
		/// </summary>
		/// <param name="size">Amount of space required.</param>
		/// <returns>True if room is available, false otherwise.</returns>
		private bool need(int size)
		{
			if ((data_ptr + size) > data_end)
			{
				base.flush();
				begin(DAM_TL_DT, size);
			}
			
			return ((data_ptr + size) <= data_end);
		}  // need


		/*
		** Name: check
		**
		** Description:
		**	Validate a buffer write request.
		**
		** Input:
		**	position:	Position in buffer.
		**	length:		Length of data.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	28-Jan-00 (gordy)
		**	    Created to replace redundant code and provide tracing.
		*/

		/// <summary>
		/// Validate a buffer write request.
		/// </summary>
		/// <param name="position">Position in buffer.</param>
		/// <param name="size">Length of data.</param>
		private void  check(int position, int size)
		{
			if (position < data_beg || (position + size) > data_end)
			{
				if (trace.enabled(1))
					trace.write(title + ": position out-of-bounds");
				close();
				throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
			}
			else if (trace.enabled(5))
			{
				if (position != data_ptr)
					trace.write(title + ": writing " + size + " @ " + position);
				else
					trace.write(title + ": appending " + size);
			}

			return;
		}  // check


	}  // class OutBuff
}