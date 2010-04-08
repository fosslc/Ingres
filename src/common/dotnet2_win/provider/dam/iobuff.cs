/*
** Copyright (c) 1999, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: iobuff.cs
	**
	** Description:
	**	Defines the Ingres provider class IoBuff which supports
	**	buffered client-server I/O at the Ingres/Net TCP-IP driver
	**	level.
	**
	** History:
	**	 9-Jun-99 (gordy)
	**	    Created.
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	29-Sep-99 (gordy)
	**	    A recursive loop can occur if a flush fails, which
	**	    calls close() which calls flush(), etc.  Made sure
	**	    close() won't recurse by setting the closed flag
	**	    prior to calling flush().
	**	28-Jan-00 (gordy)
	**	    Added TL segment tracing.
	**	20-Apr-00 (gordy)
	**	    Moved to package io.
	**	28-Mar-01 (gordy)
	**	    Separated tracing interface and implementation.
	**	 8-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	20-Dec-02 (gordy)
	**	    Track DAM-TL protocol level.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	17-nov-04 (thoda04) Bug 113485
	**	    Trap closed connection (EOF returned) on Read of server data.
	*/

	
	/*
	** Name: IoBuff
	**
	** Description:
	**	Implements buffered client-server I/O corresponding to the
	**	Ingres/Net TCP-IP driver (Network Layer) protocol.  Block
	**	buffer operations are supported, but no direct buffer access
	**	is provided publically.  Instances of this class support
	**	either input or output, not both.  It is expected that sub-
	**	classes will be defined providing input or output access to 
	**	the I/O buffer.
	**
	**	The DAM-NL protocol support segmentation of the data stream.
	**	Data segments are exposed to sub-classes to support
	**	concatenation without requiring buffer copies.
	**
	**	Access to the I/O buffer and related data is not multi-threaded
	**	protected at this level.  Sub-classes must provide protection
	**	or define access requirements to assure thread-safe access.
	**	Public and protected methods which access the I/O stream are
	**	synchronized.  Its not entirely obvious that this is required,
	**	since the same conventions which protect the I/O buffer should
	**	protect the I/O stream.  Synchronization is provided because of
	**	the close() method which may fall outside the protection of the
	**	I/O buffer.
	**
	**  Public Methods:
	**
	**	setBuffSize 	Set the size of the I/O buffer.
	**	clear       	Clear the I/O buffer.
	**	close       	Close the associated I/O stream.
	**
	**  Protected Data:
	**
	**	title       	Class title for tracing.
	**	trace       	Tracing for class.
	**	buffer      	The I/O buffer.
	**	data_beg    	Start of current data.
	**	data_ptr    	Current I/O position.
	**	data_end    	End of data (input) or buffer (output).
	**
	**  Protected Methods:
	**
	**	begin       	Start a new segment in the I/O buffer.
	**	flush       	Send segments in the I/O buffer to server.
	**	next        	Read next segment in buffer/stream.
	**
	**  Private Data:
	**
	**	inputStream 	The input data stream.
	**	outputStream	The output data stream.
	**	closed      	Has data stream been closed?
	**	buf_max     	Size of I/O buffer.
	**	buf_len     	End of data (multi-segment).
	**	seg_beg     	Start of current segment.
	**	seg_end     	End of current segment.
	**
	**  Private Methods:
	**
	**	clearBuffer 	Initialize I/O buffer as empty.
	**	flushBuffer 	Write buffer on output stream.
	**	fillBuffer  	Read buffer from input stream.
	**	getSegSize  	Read segment length in segment header.
	**	setSegSize  	Update the segment header with segment length.
	**
	** History:
	**	 9-Jun-99 (gordy)
	**	    Created.
	**	20-Apr-00 (gordy)
	**	    Removed super class AdvanObject.
	**	 8-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	20-Dec-02 (gordy)
	**	    Track DAM-TL protocol level.  Added proto_lvl and setProtoLvl().
	*/
	
	/// <summary>
	/// Defines the Ingres provider class IoBuff which supports
	/// buffered client-server for the Data Access Messaging - 
	/// Network Layer protocol.
	/// </summary>
	internal class IoBuff : TlConst
	{

		/*
		** Constants
		*/
		public const String DAM_TL_TRACE_ID	= "msg.tl"; // DAM-TL trace ID.
		public const String DAM_NL_TRACE_ID	= "msg.nl"; // DAM-NL trace ID.

		/*
		** I/O buffer and data indices.
		*/
		protected internal byte[] buffer; // IO buffer.
		protected internal int data_beg;  // Start of data.
		protected internal int data_ptr;  // Current data position.
		protected internal int data_end;  // End of data/buffer.
		protected internal byte proto_lvl = DAM_TL_PROTO_1;  // TL Protocol level.

		/*
		** Tracing.
		*/
		protected internal String title; // Object title for tracing.
		protected internal ITrace trace;        // Tracing.

		/*
		** I/O streams.
		*/
		public InputStream  inputStream  = null; // Input data stream.
		public OutputStream outputStream = null; // Output data stream.
		private bool closed = false;      // Is data stream closed?

		/*
		** Local buffer indices.
		*/
		private int buf_max;              // Buffer size.
		private int buf_len;              // End of data.
		private int seg_beg;              // Start of current segment.
		private int seg_end;              // End of current segment.


		/*
		** Name: IoBuff
		**
		** Description:
		**	Class constructor for input buffers.  Sub-class must set
		**	buffer size to initialize the input buffer.
		**
		** Input:
		**	ioStream  	The associated input/output stream.
		**	fileAccess	Read or Write to indicate direction of stream
		**
		** Output:
		**	None.
		**
		** History:
		**	 9-Jun-99 (gordy)
		**	    Created.
		**	28-Mar-01 (gordy)
		**	    Separated tracing interface and implementation.*/
		
		/// <summary>
		/// Class constructor for input buffers.  Sub-class must set
		/// buffer size to initialize the input buffer.
		/// </summary>
		/// <param name="inStream"></param>
		protected internal IoBuff(InputStream  inStream)
		{
			trace = new Tracing(DAM_NL_TRACE_ID);

				title = "IoBuff[in]";
				this.inputStream = inStream;
		}
		// IoBuff


		/// <summary>
		/// Class constructor for output buffers.  Sub-class must set
		/// buffer size to initialize the input buffer.
		/// </summary>
		/// <param name="outStream">The associated output stream.</param>
		protected internal IoBuff(OutputStream outStream)
		{
			trace = new Tracing(DAM_NL_TRACE_ID);

				title = "IoBuff[out]";
				this.outputStream = outStream;
		}
		// IoBuff


		/*
		** Name: BuffSize property
		**
		** Description:
		**	Allocate and initialize the I/O buffer.  Any previous
		**	data is lost.
		**
		** Input:
		**	size	Length of I/O buffer.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 9-Jun-99 (gordy)
		**	    Created.
		*/
		
		/// <summary>
		/// Allocate and initialize the I/O buffer.  Any previous data is lost.
		/// </summary>
		public int BuffSize
		{
			set
			{
				/*
				** The Ingres/Net Network Layer reserves 16 bytes
				** at the start of the buffer for its own use.  The
				** TCP-IP drivers only actually use 2 bytes, so we
				** extend the buffer size by the lesser amount.
				*/
				buf_max = value + 2;
				buffer = new byte[buf_max];
				clearBuffer();
				if (trace.enabled(5))
					trace.write(title + ": set buffer size " + value);
				return ;
			}
		}  // BuffSize


		/*
		** Name: TL_ProtocolLevel property
		**
		** Description:
		**	Get/Set the DAM-TL protocol level.
		**
		** History:
		**	20-Dec-02 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Set the DAM-TL protocol level.
		/// </summary>
		public byte TL_ProtocolLevel
		{
			set { this.proto_lvl = value; }
		} // TL_ProtocolLevel


		/*
		** Name: clearBuffer
		**
		** Description:
		**	Initialize the I/O buffer as empty.
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
		**	 9-Jun-99 (gordy)
		**	    Created.
		*/
		
		/// <summary>
		/// Initialize the I/O buffer as empty.
		/// </summary>
		private void  clearBuffer()
		{
			seg_beg  = seg_end  = buf_len = 0;
			data_beg = data_ptr = data_end = 0;
			return ;
		}  // clearBuffer


		/*
		** Name: flushBuffer
		**
		** Description:
		**	Write buffer to output stream and clear output buffer.
		**	The output stream is not actually flushed, since this
		**	routine may be called between a pair of buffers.
		**
		** Input:
		**	force	Force socket to flush buffers?
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 9-Jun-99 (gordy)
		**	    Created.
		**	29-Sep-99 (gordy)
		**	    Moved socket flush (optional) to this routine so
		**	    it can be requested from local methods without the
		**	    overhead of calling flush().*/
		
		/// <summary>
		/// Write buffer to output stream and clear output buffer.  The
		/// output stream is not actually flushed, since this routine
		/// may be called between a pair of buffers.
		/// </summary>
		/// <param name="force">True to force socket to flush buffers</param>
		private void  flushBuffer(bool force)
		{
			if (buf_len <= 0)
				return ;
			
			if (trace.enabled(4))
			{
				trace.write(title + ": sending " + buf_len + " bytes.");
				trace.hexdump(buffer, 0, buf_len);
			}
			
			try
			{
				outputStream.write(buffer, 0, buf_len);
			}
			catch (Exception ex)
			{
				if (trace.enabled(1))
					trace.write(title + ": write error: " + ex.Message);
				clearBuffer();
				close();
				throw SqlEx.get( ERR_GC4003_CONNECT_FAIL, ex );
			}
			
			clearBuffer();
			
			if (force)
				try { outputStream.flush(); }
				catch (System.Exception ex)
				{
					if (trace.enabled(1))
						trace.write(title + ": flush error: " + ex.Message);
					close();
					throw SqlEx.get( ERR_GC4003_CONNECT_FAIL, ex );
				}
			return ;
		}  // flushBuffer
		
		/*
		** Name: fillBuffer
		**
		** Description:
		**	Read buffer from input stream if requested amount of data
		**	is not currently available ((seg_beg + size) > buf_len).
		**
		** Input:
		**	size	Amount of data required.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 9-Jun-99 (gordy)
		**	    Created.
		**	17-nov-04 (thoda04) Bug 113485
		**	    Trap closed connection (EOF returned) on Read of server data.
		*/
		
		/// <summary>
		/// Read buffer from input stream if requested amount of data
		/// is not currently available ((seg_beg + size) > buf_len).
		/// </summary>
		/// <param name="size">Amount of data required.</param>
		private void  fillBuffer(int size)
		{
			/*
			** Check to see if data is currently available.
			*/
			if ((seg_beg + size) <= buf_len)
				return ;
			
			/*
			** The buffer size should have been negotiated
			** at connection time and any segment should
			** fit in the buffer.
			*/
			if (size > buf_max)
			{
				if (trace.enabled(1))
					trace.write(title + ": buffer overflow (" + size + "," + buf_max + ")");
				close();
				throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
			}
			
			/*
			** Maximize space available for new data.
			*/
			if (seg_beg > 0)
			{
				int offset;
				
				 for (offset = 0; (seg_beg + offset) < buf_len; offset++)
					buffer[offset] = buffer[seg_beg + offset];
				
				buf_len -= seg_beg;
				seg_end -= seg_beg;
				seg_beg = 0;
			}
			
			while (buf_len < size)
			{
				int length;
				
				if (trace.enabled(5))
					trace.write(title + ": reading " + (buf_max - buf_len) + " bytes (" + size + " requested, " + buf_len + " buffered)");
				
				try
				{
					length = inputStream.Read(buffer, buf_len, buf_max - buf_len);
				}
				catch (Exception ex)
				{
					if (trace.enabled(1))
						trace.write(title + ": read error: " + ex.Message);
					close();
					throw SqlEx.get( ERR_GC4003_CONNECT_FAIL, ex );
				}
				
				if (length < 1)  // if EOF due to server closing the connection
				{
					if (trace.enabled(1))
						trace.write(title + ": connection closed by server");
					close();
					throw SqlEx.get( ERR_GC4003_CONNECT_FAIL );
				}
				
				if (trace.enabled(4))
				{
					trace.write(title + ": received " + length + " bytes.");
					trace.hexdump(buffer, buf_len, length);
				}
				
				buf_len += length;
			}
			
			return ;
		} // fillBuffer
		
		/*
		** Name: getSegSize
		**
		** Description:
		**	Reads the Ingres/Net TCP-IP header and returns the length
		**	of the current segment.  The current position (buf_beg) is
		**	moved past the TCP-IP header.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Segment length.
		**
		** History:
		**	 9-Jun-99 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Reads the Ingres/Net TCP-IP header and returns the length
		/// of the current segment.  The current position (buf_beg) is
		/// moved past the TCP-IP header.
		/// </summary>
		/// <returns>Segment length.</returns>
		private int getSegSize()
		{
			return ( (buffer[seg_beg]           & 0x00ff) |
			        ((buffer[seg_beg + 1] << 8) & 0xff00));
			
		}  // getSegSize


		/*
		** Name: setSegSize
		**
		** Description:
		**	Updates the Ingres/Net TCP-IP header with the length
		**	of the current segment.
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
		**	 9-Jun-99 (gordy)
		**	    Created.
		**	28-Jan-00 (gordy)
		**	    Added TL segment tracing.*/
		
		/// <summary>
		/// Updates the Ingres/Net TCP-IP header with the length
		/// of the current segment.
		/// </summary>
		private void  setSegSize()
		{
			int size;
			
			/*
			** For output, data_ptr points at the end of
			** the data written by our subclass.
			*/
			if (data_ptr <= (seg_beg + 2))
				data_ptr = seg_beg;  // just to be safe

			seg_end = buf_len = data_ptr;
			size = seg_end - seg_beg;
			
			if (size > 0)
			{
				if (trace.enabled(5))
					trace.write(title + ": write segment " + size);
				
				/*
				** The Ingres NL standard requires little endian
				** for the length indicator.
				*/
				buffer[seg_beg]     = (byte) ( size & 0xff);
				buffer[seg_beg + 1] = (byte) ((size >> 8) & 0xff);
			}
			
			return ;
		}  // setSegSize


		/*
		** Name: begin
		**
		** Description:
		**	Begin a new Ingres NL data segment in the output buffer.
		**	reserves space for the Ingres TCP-IP header.  Flushes
		**	current buffer if insufficient space for next segment.
		**
		** Input:
		**	size	Minimum amount of space required for next segment.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 9-Jun-99 (gordy)
		**	    Created.
		*/
		
		/// <summary>
		/// Begin a new Ingres NL data segment in the output buffer.
		/// reserves space for the Ingres TCP-IP header.  Flushes
		/// current buffer if insufficient space for next segment.
		/// </summary>
		/// <param name="size">Minimum amount of space required for
		/// next segment.</param>
		protected internal virtual void  begin(int size)
		{
			lock(this)
			{
				if (closed || outputStream == null)
					throw SqlEx.get( ERR_GC4004_CONNECTION_CLOSED );
				
				setSegSize(); // Update the current segment length (if any).
				seg_beg = buf_len; // New segment starts at end of current data.
				
				/*
				** Flush current segments if insufficient space
				** for next segment.  Note that flushBuffer()
				** handles the case of no current segments.
				*/
				if ((buf_len + size + 2) > buf_max)
					flushBuffer(false);
				
				/*
				** Reserve space for the Ingres TCP-IP header.
				*/
				data_beg = data_ptr = seg_beg + 2;
				data_end = buf_max;
				
				return ;
			}
		}  // begin


		/*
		** Name: flush
		**
		** Description:
		**	Send output buffer to server.
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
		**	 9-Jun-99 (gordy)
		**	    Created.
		**	29-Sep-99 (gordy)
		**	    Moved socket flush to flushBuffer().*/

		/// <summary>
		/// Send output buffer to server.
		/// </summary>
		protected internal virtual void  flush()
		{
			lock(this)
			{
				if (closed || outputStream == null)
					throw SqlEx.get( ERR_GC4004_CONNECTION_CLOSED );
				
				setSegSize(); // Update current segment length.
				flushBuffer(true);
				return ;
			}
		}  // flush


		/*
		** Name: next
		**
		** Description:
		**	Read next segment from input stream.
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
		**	 9-Jun-99 (gordy)
		**	    Created.
		**	28-Jan-00 (gordy)
		**	    Added TL segment tracing.*/

		/// <summary>
		/// Read next segment from input stream.
		/// </summary>
		protected internal virtual void  next()
		{
			lock(this)
			{
				int size;
				
				if (closed || inputStream == null)
					throw SqlEx.get( ERR_GC4004_CONNECTION_CLOSED );
				
				if (seg_beg < seg_end)
					seg_beg = seg_end;
				if (seg_beg >= buf_len)
					clearBuffer();
				
				fillBuffer(2); // Make sure length is available
				size = getSegSize();
				if (trace.enabled(5))
					trace.write(title + ": read segment " + size);
				
				fillBuffer(size); // Make sure entire segment is available
				seg_end = seg_beg + size;
				data_beg = data_ptr = seg_beg + 2;
				data_end = seg_end;
				
				return ;
			}
		}  // next


		/*
		** Name: clear
		**
		** Description:
		**	Clear the I/O buffer.  Any existing data will be lost.
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
		**	 9-Jun-99 (gordy)
		**	    Created.
		*/
		
		/// <summary>
		/// Clear the I/O buffer.  Any existing data will be lost.
		/// </summary>
		public virtual void  clear()
		{
			clearBuffer();
			return ;
		} // clear


		/*
		** Name: close
		**
		** Description:
		**	Close the associated I/O stream.
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
		**	 9-Jun-99 (gordy)
		**	    Created.
		**	29-Sep-99 (gordy)
		**	    A recursive loop can occur if a flush fails, which
		**	    calls close() which calls flush(), etc.  Made sure
		**	    close() won't recurse by setting the closed flag
		**	    prior to calling flush().*/
		
		/// <summary>
		/// Close the associated I/O stream.
		/// </summary>
		public virtual void  close()
		{
			lock(this)
			{
				if (!closed)
				{
					closed = true;
					
					if (inputStream != null)
						try
						{
							inputStream.Close();
						}
						catch (Exception /* ignore */)
						{
						}
					
					if (outputStream != null)
					{
						try
						{
							setSegSize();
							flushBuffer(true);
						}
						catch (Exception /* ignore */)
						{
						}
						try
						{
							outputStream.Close();
						}
						catch (Exception /* ignore */)
						{
						}
					}
					
					clearBuffer();
					inputStream  = null;
					outputStream = null;
					if (trace.enabled(3))
						trace.write(title + ": closed");
				}
				
				return ;
			}
		} // close


		/*
		** Name: Float.intBitsToFloat
		**
		** Description:
		**	Converts a sign, exponent, and mantissa values into
		**	an IEEE 754 single floating-point value.
		**	The IEEE single format consists of three fields:
		**	a 1-bit sign s , a 8-bit biased exponent e, and a 23-bit
		**	fraction f.  The high-order bit (bit 31) contains the sign bit,
		**	followed by the exponent (bits 30-23), followed by 
		**	the mantissa (bits 22-0).
		**	------------------------------------------------------
		**	| s|   e[30:23]      |           f[22:0]             |
		**	------------------------------------------------------
		**	 31 30             23 22                            0
		**	
		**
		** Input:
		**	val	        uint 32-bit value.
		**
		** Returns:
		**	32-bit float.
		**
		** History:
		**	12-Aug-02 (thoda04)
		**	    Created since .NET does not have an equivalent to 
		**	    Float.intBitsToFloat(int).
		*/

		public class Float
		{
			static public float intBitsToFloat(uint val)
			{
				byte[] bitpattern = new byte[4];

				if (System.BitConverter.IsLittleEndian)  // Intel, DEC
				{
					bitpattern[3] = (byte)(val >> 24);
					bitpattern[2] = (byte)(val >> 16);
					bitpattern[1] = (byte)(val >>  8);
					bitpattern[0] = (byte)(val      );
				}
				else  // IsBigEndian                     // Sun, IBM 370
				{
					bitpattern[0] = (byte)(val >> 24);
					bitpattern[1] = (byte)(val >> 16);
					bitpattern[2] = (byte)(val >>  8);
					bitpattern[3] = (byte)(val      );
				}

				return System.BitConverter.ToSingle(bitpattern, 0);
			}  // intBitsToFloat


			static public uint floatToIntBits(float valueFloat)
			{
				byte[] bitpattern;

				bitpattern = System.BitConverter.GetBytes(valueFloat);
				return System.BitConverter.ToUInt32(bitpattern, 0);
			}  // floatToIntBits

		}  // Float


		/*
		** Name: Double.longBitsToDouble
		**
		** Description:
		**	Converts a sign, exponent, and mantissa values into
		**	an IEEE 754 double floating-point value.
		**	The IEEE single format consists of three fields:
		**	a 1-bit sign s , a 11-bit biased exponent e, and a 52-bit
		**	fraction f.  The high-order bit (bit 31) contains the sign bit,
		**	followed by the exponent (bits 30-23), followed by 
		**	the mantissa (bits 22-0).
		**	------------------------------------------------------
		**	| s|   e[62:52]      |           f[51:0]             |
		**	------------------------------------------------------
		**	 63 62             52 51                            0
		**	
		**	The 64-bit value is stored contiguously in two 32-bit
		**	words.  In the SPARC architecture, the higher address 32-bit
		**	word contains the least significant [31:0] 32 bits of the fraction,
		**	while in the Intel x86 architecture the lower address 32-bit
		**	word contains the the least significant fraction bits.
		**
		** Input:
		**	val	        ulong 64-bit value.
		**
		** Returns:
		**	64-bit double.
		**
		** History:
		**	12-Aug-02 (thoda04)
		**	    Created since .NET does not have an equivalent to 
		**	    Float.longBitsToDouble(ulong).
		*/

		public class Double
		{
			static public double longBitsToDouble(ulong val)
			{
				byte[] bitpattern = new byte[8];

				if (System.BitConverter.IsLittleEndian)  // Intel, DEC
				{
					bitpattern[7] = (byte)(val >> 56);
					bitpattern[6] = (byte)(val >> 48);
					bitpattern[5] = (byte)(val >> 40);
					bitpattern[4] = (byte)(val >> 32);
					bitpattern[3] = (byte)(val >> 24);
					bitpattern[2] = (byte)(val >> 16);
					bitpattern[1] = (byte)(val >>  8);
					bitpattern[0] = (byte)(val      );
				}
				else  // IsBigEndian                     // Sun, IBM 370
				{
					bitpattern[0] = (byte)(val >> 56);
					bitpattern[1] = (byte)(val >> 48);
					bitpattern[2] = (byte)(val >> 40);
					bitpattern[3] = (byte)(val >> 32);
					bitpattern[4] = (byte)(val >> 24);
					bitpattern[5] = (byte)(val >> 16);
					bitpattern[6] = (byte)(val >>  8);
					bitpattern[7] = (byte)(val      );
				}

				return System.BitConverter.ToDouble(bitpattern, 0);
			}  // longBitsToDouble


			static public ulong doubleToLongBits(double valueDouble)
			{
				byte[] bitpattern;

				bitpattern = System.BitConverter.GetBytes(valueDouble);
				return System.BitConverter.ToUInt64(bitpattern, 0);
			}  // doubleToLongBits

		}  // Double


	} // class IoBuff
}