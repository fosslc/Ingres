/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{

	/*
	** Name: sqllongbyte.cs
	**
	** Description:
	**	Defines class which represents an SQL LongBinary value.
	**
	**  Classes:
	**
	**	SqlLongByte	An SQL LongBinary value.
	**	BinIS2HexRdr	Convert binary InputStream to Hex Reader.
	**
	** History:
	**	12-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 1-Dec-03 (gordy; ported by thoda04 1-Oct-06)
	**	    Added parameter data value oriented methods.
	*/


	/*
	** Name: SqlLongByte
	**
	** Description:
	**	Class which represents an SQL LongBinary value.  SQL LongBinary 
	**	values are potentially large variable length streams.
	**
	**	Supports conversion to String, Binary, Object and character
	**	streams.  
	**
	**	The data value accessor methods do not check for the NULL
	**	condition.  The super-class isNull() method should first
	**	be checked prior to calling any of the accessor methods.
	**
	**  Inherited Methods:
	**
	**	setNull			Set a NULL data value.
	**	toString		String representation of data value.
	**	closeStream		Close the current stream.
	**
	**  Public Methods:
	**
	**	set			Assign a new non-NULL data value.
	**	getString		Data value accessor methods
	**	getBytes
	**	getBinaryStream
	**	getAsciiStream
	**	getUnicodeStream
	**	getCharacterStream
	**	getObject
	**
	**  Protected Methods:
	**
	**	cnvtIS2Rdr		Convert InputStream to Reader.
	**
	**  Private Methods:
	**
	**	strm2array		Convert binary stream to byte array.
	**
	** History:
	**	12-Sep-03 (gordy)
	**	    Created.
	*/

	internal class
		SqlLongByte : SqlStream
	{


		/*
		** Name: SqlLongByte
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

		public
			SqlLongByte() : base()
		{
		} // SqlLongByte


		/*
		** Name: SqlLongByte
		**
		** Description:
		**	Class constructor.  Data value is initially NULL.
		**	Defines a SqlStream event listener for stream 
		**	closure event notification.
		**
		** Input:
		**	listener	Stream listener.
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

		internal
			SqlLongByte( IStreamListener listener ) : base( listener )
		{
		} // SqlLongByte


		/*
		** Name: set
		**
		** Description:
		**	Assign a new non-NULL data value.  The input stream
		**	must also implement the SqlStream.IStreamSource
		**	interface.
		**
		** Input:
		**	stream		The new data value.
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

		public void
			set( InputStream stream )
		{
			setStream( stream );
			return;
		} // set


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value as a copy of a SqlByte data 
		**	value.  The data value will be NULL if the input value 
		**	is null, otherwise non-NULL.  
		**
		** Input:
		**	data	SqlByte data value to copy.
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
		set(SqlByte data)
		{
			if (data.isNull())
				setNull();
			else
			{
				/*
				** The binary data is stored in a byte array.  A simple 
				** binary stream will produce the desired output.  Note 
				** that we need to follow the SqlByte convention and 
				** extend the data to the optional limit.
				*/
				data.extend();
				setStream(getBinary(data.value, 0, data.length));
			}
			return;
		} // set


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value as a copy of a SqlVarByte data 
		**	value.  The data value will be NULL if the input value 
		**	is null, otherwise non-NULL.  
		**
		** Input:
		**	data	SqlVarByte data value to copy.
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
		set(SqlVarByte data)
		{
			if (data.isNull())
				setNull();
			else
			{
				/*
				** The binary data is stored in a byte array.  A simple 
				** binary stream will produce the desired output.
				*/
				setStream(getBinary(data.value, 0, data.length));
			}
			return;
		} // set


		/*
		** Name: get
		**
		** Description:
		**	Return the current data value.
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
		**	InputStream	Current value.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public InputStream
		get()
		{
			return ((InputStream)getStream());
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Write the current data value to an OutputStream.
		**	The current data value is consumed.  The Output-
		**	Stream is not closed.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	os	OutputStream to receive data value.  Usually ByteSegOS
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
		get(OutputStream os)
		{
			copyIs2Os((InputStream)getStream(), os);
			return;
		} // get


		/*
		** Name: setString
		**
		** Description:
		**	Assign a String value as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		** Input:
		**	value	String value (may be null).
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
		setString(String value)
		{
			if (value == null)
				setNull();
			else
			{
				//				setBytes(value.getBytes());
				System.Text.Encoding ASCIIEncoding = System.Text.Encoding.ASCII;
				setBytes(ASCIIEncoding.GetBytes(value));
			}
			return;
		} // setString


		/*
		** Name: setBytes
		**
		** Description:
		**	Assign a byte array as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		** Input:
		**	value	Byte array (may be null).
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
		setBytes(byte[] value)
		{
			if (value == null)
				setNull();
			else
				setStream(getBinary(value, 0, value.Length));
			return;
		} // setBytes


		/*
		** Name: setBinaryStream
		**
		** Description:
		**	Assign a byte stream as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		** Input:
		**	value	Byte stream (may be null).
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
		setBinaryStream(InputStream value)
		{
			setStream(value);
			return;
		} // setBinaryStream


		/*
		** Name: getString
		**
		** Description:
		**	Return the current data value as a String value.
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
		**	String	Current value converted to String.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override String 
			getString() 
		{
			byte[] ba = strm2array( getBinary(), -1 );
			return (SqlVarByte.bin2str(ba, 0, ba.Length));
		} // getString


		/*
		** Name: getString
		**
		** Description:
		**	Return the current data value as a String value
		**	with a maximum size limit.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	limit	Maximum length of result.	
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Current value converted to String.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override String 
			getString( int limit ) 
		{
			byte[] ba = strm2array( getBinary(), limit );
			return( SqlByte.bin2str( ba, 0, ba.Length ) );
		} // getString


		/*
		** Name: getBytes
		**
		** Description:
		**	Return the current data value as a byte array.
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
		**	byte[]	Current value converted to byte array.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override byte[] 
			getBytes() 
		{
			return( strm2array( getBinary(), -1 ) );
		} // getBytes


		/*
		** Name: getBytes
		**
		** Description:
		**	Return the current data value as a byte array
		**	with a maximum size limit.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	limit	Maximum length of result.	
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	Current value converted to byte array.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override byte[]
			getBytes( int limit )
		{
			return( strm2array( getBinary(), limit ) );
		} // getBytes


		/*
		** Name: getBinaryStream
		**
		** Description:
		**	Return the current data value as a binary stream.
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
		**	InputStream	Current value converted to binary stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override InputStream 
			getBinaryStream() 
		{ 
			return( getBinary() );
		} // getBinaryStream


		/*
		** Name: getAsciiStream
		**
		** Description:
		**	Return the current data value as an ASCII stream.
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
		**	InputStream	Current value converted to ASCII stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override InputStream 
			getAsciiStream() 
		{ 
			return( getAscii() );
		} // getAsciiStream


		/*
		** Name: getUnicodeStream
		**
		** Description:
		**	Return the current data value as a Unicode stream.
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
		**	InputStream	Current value converted to Unicode stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override InputStream 
			getUnicodeStream()
		{ 
			return( getUnicode() );
		} // getUnicodeStream


		/*
		** Name: getCharacterStream
		**
		** Description:
		**	Return the current data value as a character stream.
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
		**	Reader	Current value converted to character stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override Reader 
			getCharacterStream() 
		{ 
			return( getCharacter() );
		} // getCharacterStream


		/*
		** Name: getObject
		**
		** Description:
		**	Return the current data value as an Binary object.
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
		**	Object	Current value converted to Binary.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override Object 
			getObject() 
		{
			return( getBytes() );
		} // getObject


		/*
		** Name: getObject
		**
		** Description:
		**	Return the current data value as an Binary object
		**	with a maximum size limit.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	limit	Maximum length of result.	
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Object	Current value converted to Binary.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override Object 
			getObject( int limit ) 
		{
			return( getBytes( limit ) );
		} // getObject


		/*
		** Name: cnvtIS2Rdr
		**
		** Description:
		**	Converts the binary input stream into a Reader
		**	stream by converting to hex characters.
		**
		** Input:
		**	stream	    Input stream.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Reader		Converted Reader stream.
		**
		** History:
		**	12-Sep-03 (gordy)
		*/

		protected override Reader
			cnvtIS2Rdr( InputStream stream )
		{
			return( new BinIS2HexRdr( stream ) );
		} // cnvtIS2Rdr


		/*
		** Name: strm2array
		**
		** Description:
		**	Read a binary input stream and convert to a byte array.  
		**	The stream is closed.
		**
		** Input:
		**	in	Binary input stream.
		**	limit	Maximum length of result, negative for no limit.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	The stream as a byte array.
		**
		** History:
		**	12-Sep-03 (gordy)	
		**	    Created.
		*/

		private static byte[]
			strm2array( InputStream inStream, int limit )
		{
			byte[]	buff = new byte[ 8192 ];
			byte[]	ba   = new byte[ 0 ];
			int		len;

			try
			{
				while( (limit < 0  ||  ba.Length < limit)  &&
					(len = inStream.read( buff, 0, buff.Length )) >= 0 )
				{
					if ( limit >= 0 )  len = Math.Min( len, limit - ba.Length );
					byte[] tmp = new byte[ ba.Length + len ];
					if ( ba.Length > 0 )  Array.Copy( ba, 0, tmp, 0, ba.Length );
					if ( len > 0 )  Array.Copy( buff, 0, tmp, ba.Length, len );
					ba = tmp;
				}
			}
			catch( Exception )
			{
				throw SqlEx.get( ERR_GC4007_BLOB_IO );
			}
			finally
			{
				try { inStream.close(); }
				catch( Exception ) {}
			}

			return( ba );
		} // strm2array


		/*
		** Name: BinIS2HexRdr
		**
		** Description:
		**	Class which converts a binary InputStream into a hex
		**	character Reader.
		**
		**	This class is NOT expected to be used much, so the
		**	implementation is simplistic and inefficient.
		**
		**  Public Methods:
		**
		**	read			Read characters.
		**	skip			Skip characters.
		**	close			Close stream.
		**
		**  Private Data:
		**
		**	stream			Input stream.
		**	next			Next buffered character.
		**	avail			Is next character buffered?
		**
		** History:
		**	12-Sep-03 (gordy)
		*/

		private class
			BinIS2HexRdr : Reader
		{

			private InputStream		stream = null;
			private char       		next = Char.MinValue;
			private bool       		avail = false;
    

			/*
			** Name: BinIS2HexRdr
			**
			** Description:
			**	Class constructor.
			**
			** Input:
			**	stream		Input stream.
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
				BinIS2HexRdr( InputStream stream ) : base()
			{
				this.stream = stream;
			} // BinIS2HexRdr


			/*
			** Name: read
			**
			** Description:
			**	Read next character.  Returns -1 if at end-of-input.
			**
			**	Reads bytes from input stream, converts to two character
			**	hex string.  Alternates returning first/second hex char.
			**
			** Input:
			**	None.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	int	Next character (0-65535) or -1 at end-of-input.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public override int
				read()
			{
				lock(this)
				{
					/*
					** Check to see if next character is buffered.
					*/
					if ( avail )
					{
						avail = false;
						return( (int)next );
					}

					/*
					** Read next byte and check for EOF.
					*/
					int value = this.stream.read();
					if ( value == -1 )  return( -1 );

					/*
					** Convert to hex (1 or 2 digits).
					** Return first digit, buffer second.
					*/
					String str = toHexString( value & 0xff );
					char current;

					if ( str.Length == 1 )
					{
						current = '0';
						next = str[0];
					}
					else
					{
						current = str[0];
						next    = str[1];
					}
	
					avail = true;
					return( (int)current );
				}
			} // read


			/*
			** Name: read
			**
			** Description:
			**	Read characters into an array.
			**
			** Input:
			**	None.
			**
			** Output:
			**	chars	Character array.
			**
			** Returns:
			**	int	Number of characters read, or -1 at end-of-input.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public override int
				read( char[] chars )
			{
				return( this.read( chars, 0, chars.Length ) );
			} // read


			/*
			** Name: read
			**
			** Description:
			**	Read characters into a sub-array.
			**
			** Input:
			**	offset	Starting position.
			**	length	Number of characters to read.
			**
			** Output:
			**	chars	Character array.
			**
			** Returns:
			**	int	Number of characters read, or -1 at end-of-input.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public override int
				read( char[] chars, int offset, int length )
			{
				int	limit = offset + length;
				int start = offset;
    
				while( offset < limit )
				{
					int ch = this.read();	// Character or EOF (-1)
	
					if ( ch == -1 )  
						if ( offset == start )
							return( -1 );	// No characters read.
						else
							break;
	
					chars[ offset++ ] = (char)ch;
				}
    
				return( offset - start );
			} // read


			/*
			** Name: skip
			**
			** Description:
			**	Skip characters in stream.
			**
			** Input:
			**	count	Number of characters to skip.
			**
			** Output:
			**	None.
			**
			** Returns:
			**	long	Number of characters skipped.
			**
			** History:
			**	12-Sep-03 (gordy)
			**	    Created.
			*/

			public long
				skip( long count )
			{
				long start = count;
				while( count > 0  &&  this.stream.read() != -1 )  count--;
				return( start - count );
			} // skip


			/*
			** Name: close
			**
			** Description:
			**	Close the stream.
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
				stream.close();
				base.close();
				return;
			} // close


		} // class BinIS2HexRdr


	} // class SqlLongByte
}  // namespace
