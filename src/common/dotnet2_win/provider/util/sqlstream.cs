/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{
	/*
	** Name: sqlstream.cs
	**
	** Description:
	**	Defines base class which represents an SQL stream value.
	**
	**	Provides interface definitions for stream event listeners
	**	and sources to support stream closure notification.
	**
	**  Classes:
	**
	**	SqlStream	An SQL stream value.
	**	Rdr2IS		Convert Reader to InputStream.
	**
	**  Interfaces:
	**
	**	IStreamListener	Listener for SqlStream events.
	**	IStreamSource	Source of SqlStream events.
	**
	** History:
	**	12-Sep-03 (gordy)
	**	    Created.
	**	 1-Dec-03 (gordy)
	**	    Added support for parameter types/values in addition to 
	**	    existing support for columns.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/



	/*
	** Name: SqlStream
	**
	** Description:
	**	Abstract Base class which represents an SQL stream value.  
	**	SQL stream values are potentially large and variable length.  
	**	Both binary and character based streams are supported via 
	**	the InputStream and Reader IO objects (respectively).
	**	Streams may only be read once, so only a single access is 
	**	permitted to a stream value.  
	**
	**	Sub-classes must implement the cnvtIS2Rdr() method to convert
	**	InputStream streams to Reader streams.  This method is used
	**	by the methods getAscii(), getUnicode(), and getCharacter().
	**	If a sub-class uses Reader streams directly or does not call
	**	the indicated methods, cnvtIS2Rdr() may be implemented as a
	**	stub.
	**
	**	A stream closure notification system is also supported via 
	**	StreamListener and StreamSource interfaces.  A stream must
	**	implement the StreamSource interface to participate in 
	**	stream notification.
	**
	**  Abstract Methods:
	**
	**	cnvtIS2Rdr		Convert InputStream to Reader.
	**
	**  Public Methods:
	**
	**	setNull			Set a NULL data value.
	**	toString		String representation of data value.
	**	closeStream		Close the current stream.
	**
	**  Protected Methods:
	**
	**	setStream		Assign a new non-NULL data value.
	**	getStream		Return current stream.
	**	getBinary		Convert to binary InputStream.
	**	getAscii		Convert to ASCII InputStream.
	**	getUnicode		Convert to Unicode (UTF8) InputStream.
	**	getCharacter		Convert to character Reader.
	**	copyIs2Os		Copy InputStream to OutputStream.
	**	copyRdr2Wtr		Copy Reader to Writer.
	**
	**  Private Data:
	**
	**	stream  		Stream value.
	**	active  		Has stream been accessed?
	**	listener		SqlStream event listener.
	**	bbuff   		Copy byte buffer.
	**	cbuff   		Copy character buffer.
	**
	**  Private Methods:
	**
	**	checkAccess		Check if access valid, mark accessed.
	**
	** History:
	**	12-Sep-03 (gordy)
	**	    Created.
	**	 1-Dec-03 (gordy)
	**	    Removed requirement for StreamSource to support parameters.
	**	    Added input-to-output copy methods and associated buffers.
	*/

	internal abstract class
		SqlStream : SqlData
	{

		private Object          stream = null;  	// InputStream/Reader
		private bool            active = false; 	// Has been accessed?
		private IStreamListener listener = null;	// Event listener.

		private byte[]          bbuff = null;		// Copy buffers.
		private char[]          cbuff = null;

		/*
		** Abstract Methods
		*/
		protected virtual Reader cnvtIS2Rdr( InputStream stream )
		{
			throw new NotImplementedException("SqlStream.cnvtIS2Rdr impl missing.");
		}


		/*
		** Name: SqlStream
		**
		** Description:
		**	Class constructor.  Data value is initially NULL.
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
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		protected
			SqlStream() : base( true )
		{
		} // SqlStream


		/*
		** Name: SqlStream
		**
		** Description:
		**	Class constructor.  Data value is initially NULL.
		**	Defines a SqlStream event listener for stream 
		**	closure event notification.
		**
		** Input:
		**	listener	Stream event listener.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		protected
			SqlStream( IStreamListener listener ) : this()
		{
			this.listener = listener;
		} // SqlStream


		/*
		** Name: setNull
		**
		** Description:
		**	Set the NULL state of the data value to NULL.
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
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override void
			setNull()
		{
			lock(this)
			{
				base.setNull();
				stream = null;
				active = false;
			}
			return;
		} // setNull


		/*
		** Name: toString
		**
		** Description:
		**	Returns a string representing the current object.
		**
		**	The SqlData super-class calls getString() to implement
		**	this method, which is not necessarily a good thing for
		**	all streams.  Override to simply call the toString()
		**	method of the current stream (watching out for NULL
		**	values).
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	String representation of this SQL data value.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public String
			toString()
		{
			return( "SqlStream: " + ((stream == null) ? "NULL" : stream.ToString()) );
		} // toString


		// avoid double calls to CheckAccess by setCharacter
		public override string ToString()
		{
			return this.toString();
		}

		/*
		** Name: closeStream
		**
		** Description:
		**	Close associated stream.
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
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    StreamSource no longer required, so check for stream classes.
		*/

		public void
			closeStream()
		{
			lock (this)
			{
				active = true;	// Don't allow access after close
				if (stream == null) return;

				try
				{
					if (stream is InputStream)
						((InputStream)stream).close();
					else
					if (stream is Reader)
						((Reader)stream).close();
				}
				catch (IOException /*ignore*/ ) { }
				return;
			}
		} // close


		/*
		** Name: setStream
		**
		** Description:
		**	Assign a new stream data value.  The data value will be 
		**	NULL if the input value is null, otherwise non-NULL.  The 
		**	data object must be an instance of InputStream or Reader 
		**	and may optionally implement the StreamSource interface.
		**
		** Input:
		**	stream	InputStream or Reader.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted from set(), StreamSource no longer required.
		*/

		protected void
		setStream( Object stream )
		{
			lock(this)
			{
				if ( stream == null )
					setNull();
				else
				{
				if ( 
					 ! (stream is InputStream)  &&
					 ! (stream is Reader) 
				   )
					throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );
				
				setNotNull();
				active = false;
				this.stream = stream;

				if (stream is IStreamSource)  // (ByteSegIS and Ucs2SegRdr)
					((IStreamSource)stream).addStreamListener( listener, this );
				}
			}

			return;
		} // set


		/*
		** Name: getStream
		**
		** Description:
		**	Return the current stream data value.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Object	Current stream value.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		protected Object
		getStream()
		{
			checkAccess();
			return( stream );
		} // getStream


		/*
		** Name: getBinary
		**
		** Description:
		**	Returns a binary InputStream representation of the
		**	current stream.  Enforces single access to the stream.
		**	Base stream must be of type InputStream.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	InputStream	Binary stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Check for NULL stream.
		*/

		protected InputStream
			getBinary()
		{
			checkAccess();

			if (stream == null)  return null;

			if ( stream is InputStream )
				return( (InputStream)stream );
			else
				throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );
		} // getBinary


		/*
		** Name: getAscii
		**
		** Description:
		**	Returns an ASCII InputStream representation of the
		**	current stream.  Enforces single access to the stream.
		**	Byte streams are converted to a hex character stream
		**	prior to conversion to ASCII.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	InputStream	ASCII stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Check for NULL stream.
		*/

		protected InputStream
			getAscii()
		{
			checkAccess();

			if (stream == null)  return null;

			if ( stream is InputStream )
				return( new Rdr2IS( cnvtIS2Rdr( (InputStream)stream ), "US-ASCII" ) );
			else
			if ( stream is Reader )
				return( new Rdr2IS( (Reader)stream, "US-ASCII" ) );
			else
				throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );
		} // getAscii


		/*
		** Name: getUnicode
		**
		** Description:
		**	Returns an UTF8 InputStream representation of the
		**	current stream.  Enforces single access to the stream.
		**	Byte streams are converted to a hex character stream
		**	prior to conversion to UTF8.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	InputStream	UTF8 stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Check for NULL stream.
		*/

		protected InputStream
			getUnicode()
		{
			checkAccess();

			if (stream == null)  return null;

			if ( stream is InputStream )
				return( new Rdr2IS( cnvtIS2Rdr( (InputStream)stream ), "UTF-8" ) );
			else
			if ( stream is Reader )
				return( new Rdr2IS( (Reader)stream, "UTF-8" ) );
			else
				throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );
		} // getUnicode


		/*
		** Name: getCharacer
		**
		** Description:
		**	Returns a character Reader representation of the
		**	current stream.  Enforces single access to the stream.
		**	Byte streams are converted to a hex character stream.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Reader		Character stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Check for NULL stream.
		*/

		protected Reader
			getCharacter()
		{
			checkAccess();

			if (stream == null)  return null;

			if ( stream is InputStream )
				return( cnvtIS2Rdr( (InputStream)stream ) );
			else
			if ( stream is Reader )
				return( (Reader)stream );
			else
				throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );
		} // getCharacter


		/*
		** Name: copyIs2Os
		**
		** Description:
		**	Writes the contents of an InputStream to an OutputStream.
		**	The InputStream is closed.  The OutputStream is flushed
		**	but not closed.
		**
		** Input:
		**	is	InputStream.
		**	os	OutputStream.
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

		protected void
		copyIs2Os(InputStream inputStream, OutputStream outputStream)
		{
			if (bbuff == null)
				lock (this)
				{
					{ if (bbuff == null)  bbuff = new byte[8192]; }
				}

			try
			{
				lock (bbuff)
				{
					for (int len = inputStream.read(bbuff); len >= 0;
						len = inputStream.read(bbuff))
						outputStream.write(bbuff, 0, len);
				}

				inputStream.close();
				outputStream.flush();
			}
			catch (Exception ex)
			{
				try { inputStream.close(); }
				catch (Exception /* ignore */) { }
				try { outputStream.flush(); }
				catch (Exception /* ignore */) { }
				throw SqlEx.get(ERR_GC4007_BLOB_IO, ex);
			}

			return;
		} // copyIs2Os


		/*
		** Name: coyRdr2Wtr
		**
		** Description:
		**	Writes the contents of a Reader stream to a Writer stream.
		**	The Reader stream is closed.  The Writer stream is flushed
		**	but not closed.
		**
		** Input:
		**	rdr	Reader stream.
		**	wtr	Writer stream.
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

		protected void
		copyRdr2Wtr(Reader rdr, Writer wtr)
		{
			if (cbuff == null)
				lock (this)
				{
					{ if (cbuff == null)  cbuff = new char[4096]; }
				}

			try
			{
				lock (cbuff)
				{
					for (int len = rdr.read(cbuff); len >= 0; len = rdr.read(cbuff))
						wtr.write(cbuff, 0, len);
				}

				rdr.close();
				wtr.flush();
			}
			catch (Exception ex)
			{
				try { rdr.close(); }
				catch (Exception /* ignore */ ) { }
				try { wtr.flush(); }
				catch (Exception /* ignore */ ) { }
				throw SqlEx.get(ERR_GC4007_BLOB_IO, ex);
			}

			return;
		} // copyRdr2Wtr


		/*
		** Name: checkAccess
		**
		** Description:
		**	Check state of stream.  Mark as accessed if permitted.
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
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		private void
			checkAccess()
		{
			lock(this)
			{
				if (active) throw SqlEx.get(ERR_GC401C_BLOB_DONE);
				active = true;
			}
			return;
		} // checkAccess


		/*
		** Name: IStreamListener
		**
		** Description:
		**	Interface defining a listener for SqlStream events.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public interface
			IStreamListener
		{

			/*
			** Name: streamClosed
			**
			** Description:
			**	Indicates that a SQL Stream object has been closed.
			**
			** Input:
			**	stream		SQL Stream which has closed.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	void.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			void streamClosed( SqlStream stream );


		} // interface IStreamListener


		/*
		** Name: IStreamSource
		**
		** Description:
		**	Interface defining a source of SqlStream events.
		**
		**  Interface Methods:
		**
		**	addStreamListener	Add a SqlStream event listener.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Removed close() method.  Appropriatte method of actual
		**	    stream class should be used instead.
		*/

		public interface
			IStreamSource
		{

			/*
			** Name: addStreamListener
			**
			** Description:
			**	Add a StreamListener to a StreamSource.  Only a single
			**	listener needs to be supported and a NULL listener 
			**	removes all registered listeners. The SqlStream to be
			**	returned to the listener is also provided.
			**
			** Input:
			**	listener	SqlStream listener (may be NULL).
			**	stream		SqlStream associated with source/listener.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	void.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			void addStreamListener( IStreamListener listener, SqlStream stream );

		} // interface IStreamSource


		/*
		** Name: Rdr2IS
		**
		** Description:
		**	Class which converts Reader stream to InputStream using
		**	a designated character-set.
		**
		**  Public Methods:
		**
		**	read
		**	skip
		**	close
		**
		**  Constants:
		**
		**	EOI_RDR			End-of-input: reader stage.
		**	EOI_ENC			End-of-input: encoder stage.
		**	EOI_OUT			End-of-input: output stage.
		**	EOI			End-of-input: all stages.
		**
		**  Private Data:
		**
		**	rdr			Input Reader stream.
		**	encoder			Character-set encoder.
		**	inBuff			Input character buffer.
		**	outBuff			Output byte buffer.
		**	eoi_flags		End-of-input stage flags.
		**	ba			Temporary byte buffer.
		**
		**  Private Methods:
		**
		**	fillInput
		**	fillOutput
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		private class
			Rdr2IS : InputStream
			{

				private const int	EOI_RDR	= 0x01;		// Reader finished.
				private const int	EOI_ENC	= 0x02;		// Encoder finished.
				private const int	EOI_OUT	= 0x04;		// Output finished.
				private const int	EOI	= EOI_RDR | EOI_ENC | EOI_OUT;

				private Reader		rdr = null;
				private CharsetEncoder	encoder = null;
				private CharBuffer		inBuff = CharBuffer.allocate( 8192 );
				private ByteBuffer		outBuff = ByteBuffer.allocate( 8192 );
				private int			eoi_flags = 0;
				private byte[]		ba = new byte[1];


			/*
			** Name: Rdr2IS
			**
			** Description:
			**	Class constructor.
			**
			** Input:
			**	rdr		Reader.
			**	charSet		Character-set name.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	None.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public
				Rdr2IS( Reader rdr, String charSet )
					: base(InputStream.InputStreamNull)
			{
				this.rdr = rdr;
				encoder = CharSet.forName( charSet ).newEncoder();
				encoder.onMalformedInput( CodingErrorAction.REPLACE );
				encoder.onUnmappableCharacter( CodingErrorAction.REPLACE );
    
				/*
				** Initialize input/output buffers as empty so that 
				** the first read() request will proceed to fill them.
				** The initial buffer state is clear() for reading,
				** so simply flip() ready for writing.
				*/
				inBuff.flip();
				outBuff.flip();
			} // Rdr2IS


			/*
			** Name: read
			**
			** Description:
			**	Read next byte.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	int    Next byte (0-255) or -1 at end-of-input.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public override int
				read()
			{
				return( (read( ba, 0, 1 ) == -1) ? -1 : (int)ba[0] & 0xff );
			} // read


			/*
			** Name: read
			**
			** Description:
			**	Read bytes into an array.
			**
			** Input:
			**	None.
			**
			** Output:
			**	bytes	Byte array.
			**
			** Returns:
			**	int	Number of bytes read or -1 at end-of-input.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public override int
				read( byte[] bytes )
			{
				return( read( bytes, 0, bytes.Length ) );
			} // read


			/*
			** Name: read
			**
			** Description:
			**	Read bytes into a sub-array.
			**
			** Input:
			**	offset	Starting position.
			**	length	Number of bytes.
			**
			** Output:
			**	bytes	Byte array.
			**
			** Returns:
			**	int	Number of bytes read or -1 if end-of-input.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public override int
				read( byte[] bytes, int offset, int length )
			{
				int limit = offset + length;	// Save boundaries
				int start = offset;

				if ( (eoi_flags & EOI_OUT) == 0 )	// Reached end-of-input?
					while( offset < limit )		// More data to transer
					{
						/*
						** Fill output buffer when empty.
						*/
						if ( ! outBuff.hasRemaining()  &&  ! fillOutput() )
						{
							eoi_flags |= EOI_OUT;	// End-of-input!
							break;
						}

						/*
						** Return available data
						*/
						length = Math.Min( limit - offset, outBuff.remaining() );
						outBuff.get( bytes, offset, length );
						offset += length;
					}
    
				/*
				** If some data was transferred, return the length
				** irregardless if end-of-input has been reached.
				** End-of-input only returned when no data has been
				** transferred.  Make sure some data was requested
				** otherwise a false end-of-input would be returned.
				*/
				return( (length > 0  &&  offset == start) ? -1 : offset - start );
			} // read


			/*
			** Name: skip
			**
			** Description:
			**	Skip bytes in input stream.
			**
			** Input:
			**	count	Number of bytes to skip.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	long	Number of bytes skipped.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public long
				skip( long count )
			{
				long start = count;			// Save original
    
				if ( (eoi_flags & EOI_OUT) == 0 )	// Reached end-of-input?
					while( count > 0 )		// More bytes to skip
					{
						/*
						** Fill output buffer when empty.
						*/
						if ( ! outBuff.hasRemaining()  &&  ! fillOutput() )
						{
							eoi_flags |= EOI_OUT;	// End-of-input!
							break;
						}
		
						/*
						** Data is skipped simply by resetting the 
						** output buffer position.
						*/
						long length = Math.Min( count, (long)outBuff.remaining() );
						outBuff.position( outBuff.position() + (int)length );
						count -= length;
					}
    
				return( start - count );		// Amount skipped
			} // skip


			/*
			** Name: close
			**
			** Description:
			**	Close the input stream.
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
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public override void
				close()
			{
				rdr.close();
				eoi_flags |= EOI;		// Force end-of-input state.
				return;
			} // close


			/*
			** Name: fillInput
			**
			** Description:
			**	Reads characters into input buffer from Reader stream.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	boolean		True if data available, False if EOI.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			private bool
				fillInput()
			{
				bool ready = false;
				inBuff.clear();			// Prepare for reading.
    
				if ( (eoi_flags & EOI_RDR) == 0 )	// Reached end-of-input?
				{
					/*
					** The Reader class does not directly support
					** transfer of data with our input buffer, so
					** need to read directly into the underlying
					** character array (buffer allocation method
					** assures the presence of the array).
					*/
					int length = rdr.read( inBuff.array() );

					/*
					** Check for end-of-input.
					*/
					if ( length < 0 )
						eoi_flags |= EOI_RDR;
					else
					{
						/*
						** Update buffer with amount transferred.
						*/
						inBuff.position( inBuff.position() + length );
						ready = true;
					}
				}
    
				inBuff.flip();			// Prepare for writing.
				return( ready );
			} // fillInput


			/*
			** Name: fillOutput
			**
			** Description:
			**	Encodes characters from input buffer into the byte
			**	output buffer.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	boolean	    True if data available, False if EOI.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			private bool
				fillOutput()
			{
				outBuff.clear();			// Prepare for reading.
    
				if ( (eoi_flags & EOI_ENC) == 0 )	// Reached end-of-input?
					do
					{
						/*
						** Fill input buffer when empty.
						*/
						if ( ! inBuff.hasRemaining()  &&  ! fillInput() )
						{
							/*
							** We only want to flush the encoder once,
							** and then only with an empty output buffer
							** to ensure that there will be room for any
							** flushed data.  If anything has been placed
							** in the output buffer, the end-of-input
							** processing will be done on the next call.
							*/
							if ( outBuff.position() == 0 )
							{
								encoder.encode( inBuff, outBuff, true );
								encoder.flush( outBuff );
								eoi_flags |= EOI_ENC;
							}
							break;
						}

						/*
						** Encode input characters into output bytes.
						** Loop to fill input buffer when empty.  Only
						** UNDERFLOW or OVERFLOW should be returned 
						** since the encoder has been set to REPLACE 
						** or IGNORE other coding errors.
						*/
					} while( encoder.encode( inBuff, 
						outBuff, false ) == CoderResult.UNDERFLOW );

				/*
				** Only end-of-input can keep somehting from being
				** placed in the output buffer.  We may have hit
				** end-of-input and still placed something in the
				** output buffer, in which case end-of-input will
				** be signaled on the next call.
				*/
				outBuff.flip();			// Prepare for writing.
				return( outBuff.hasRemaining() );
			} // fillOutput


		} // class Rdr2IS


	} // class SqlStream
}  // namespace
