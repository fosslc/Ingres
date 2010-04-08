/*
** Copyright (c) 2003, 2007 Ingres Corporation. All Rights Reserved.
*/

using System;

namespace Ingres.Utility
{
	/*
	** Name: sqldata.cs
	**
	** Description:
	**	Defines base class which provides support for SQL data values.
	**
	**  Classes:
	**
	**	SqlData		Base class for NULL data.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	21-Mar-07 (thoda04)
	**	    Added IntervalDayToSecond, IntervalYearToMonth.
	*/


	/*
	** Name: SqlData
	**
	** Description:
	**	Utility base class which provides NULL data capabilities
	**	and defines basic data access interface.
	**
	**	This class, in and of itself, only represents NULL typeless
	**	data values and cannot be directly instantiated.  Sub-classes
	**	provide support for specific datatype values.
	**
	**	This class implements a set of getXXX() methods which always
	**	throw conversions exceptions.  Sub-classes override those
	**	methods for which conversion of their specific datatype is
	**	supported.
	**
	**	This class defines routines for handling data truncation but
	**	the routines are only implemented for the non-truncation case.
	**	A separate sub-class should implement truncation support and
	**	act as base class for support of truncatable datatypes.
	**
	**  Public Methods:
	**
	**	toString		String representation of data value.
	**	setNull			Set data value NULL.
	**	isNull			Data value is NULL?
	**	isTruncated		Data value is truncated?
	**	getDataSize		Expected size of data value.
	**	getTruncSize		Actual size of truncated data value.
	**
	**	getBoolean		Data value accessor methods
	**	getByte
	**	getShort
	**	getInt
	**	getLong
	**	getFloat
	**	getDouble
	**	getBigDecimal
	**	getString
	**	getBytes
	**	getDate
	**	getTime
	**	getTimestamp
	**	getBinaryStream
	**	getAsciiStream
	**	getUnicodeStream
	**	getCharacterStream
	**	getObject
	**
	**  Protected Methods:
	**
	**	setNotNull		Set data value not NULL.
	**
	**  Private Data:
	**
	**	is_null			Data value is NULL?.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	*/

	internal class
		SqlData : DbmsConst
	{

		private bool		is_null = true;


		/*
		** Name: getSqlType
		**
		** Description:
		**	Returns the SQL type associated with a object as defined 
		**	by this class for supported input parameter values.
		**
		** Input:
		**	obj	Parameter value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	SQL type.
		**
		** History:
		**	19-May-99 (gordy)
		**	    Created.
		**	10-May-01 (gordy)
		**	    Allow sub-classing by using instanceof instead of class ID.
		**	    Support IO classes for BLOBs.
		**	19-Feb-03 (gordy)
		**	    Replaced BIT with BOOLEAN.  Give priority to strings since
		**	    alternate storage class results in calls to this routine.
		**	 1-Dec-03 (gordy)
		**	    Extracted from parameter set class.
		*/

		public static ProviderType
			getSqlType(Object obj)
		{
			if (obj == null) return (ProviderType.DBNull);
			if (obj is String) return (ProviderType.VarChar);
			if (obj is Boolean) return (ProviderType.Boolean);
			if (obj is Char) return (ProviderType.Char);
			if (obj is Byte) return (ProviderType.Binary);
			if (obj is SByte) return (ProviderType.TinyInt);
			if (obj is Int16) return (ProviderType.SmallInt);
			if (obj is Int32) return (ProviderType.Integer);
			if (obj is Int64) return (ProviderType.BigInt);
			if (obj is Single) return (ProviderType.Real);
			if (obj is Double) return (ProviderType.Double);
			if (obj is Decimal) return (ProviderType.Decimal);
			if (obj is TimeSpan) return (ProviderType.IntervalDayToSecond);
			//			if ( obj is Reader )     	return( ProviderType.LongVarChar );
			//			if ( obj is InputStream )	return( ProviderType.LongVarBinary );
			//			if ( obj is Date )       	return( ProviderType.DATE );
			//			if ( obj is Time )       	return( ProviderType.TIME );
			if (obj is DateTime) return (ProviderType.DateTime);

			System.Type oc = obj.GetType();
			if (oc.IsArray)
			{
				if (oc.GetElementType() == System.Type.GetType("System.Char"))
					return ProviderType.Char;

				if (oc.GetElementType() == System.Type.GetType("System.Byte"))
					return ProviderType.Binary;
			}

			throw SqlEx.get(ERR_GC401A_CONVERSION_ERR);
		} // getSqlType


		/*
		** Name: getBinary
		**
		** Description:
		**	Helper method to convert byte array to binary stream.
		**
		** Input:
		**	array		Byte array.
		**	offset		Starting byte of stream.
		**	length		Number of bytes in stream.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	InputStream    Binary stream.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public static InputStream
		getBinary(byte[] array, int offset, int length)
		{
			return (new ByteArrayInputStream(array, offset, length));
		} // getBinary


		/*
		** Name: getAscii
		**
		** Description:
		**	Helper method to convert String into ASCII stream.
		**
		** Input:
		**	str		String to be converted.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	InputStream    ASCII stream.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public static InputStream
		getAscii(String str)
		{
			byte[] bytes;

			try
			{
				System.Text.Encoding ASCIIEncoding = System.Text.Encoding.ASCII;
				bytes = ASCIIEncoding.GetBytes(str);
			}
			catch (Exception)		// Should not happen!
			{ throw SqlEx.get(ERR_GC401E_CHAR_ENCODE); }

			return (new ByteArrayInputStream(bytes));
		} // getAscii


		/*
		** Name: getUnicode
		**
		** Description:
		**	Helper method to convert String into Unicode stream.
		**
		** Input:
		**	str		String to be converted.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	InputStream	Unicode stream.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public static InputStream
		getUnicode(String str)
		{
			byte[] bytes;

			try
			{
				System.Text.Encoding UTF8Encoding = System.Text.Encoding.UTF8;
				bytes = UTF8Encoding.GetBytes(str);
			}
			catch (Exception)		// Should not happen!
			{ throw SqlEx.get(ERR_GC401E_CHAR_ENCODE); }

			return (new ByteArrayInputStream(bytes));
		} // getUnicode


		/*
		** Name: getCharacter
		**
		** Description:
		**	Helper method to convert character array into stream.
		**
		** Input:
		**	array	Character array to be converted.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Reader	Character stream.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public static CharArrayReader
		getCharacter(char[] array, int length)
		{
			return (new CharArrayReader(array, length));
		} // getCharacter


		/*
		** Name: getCharacter
		**
		** Description:
		**	Helper method to convert String into character stream.
		**
		** Input:
		**	str	String to be converted.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Reader	Character stream.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public static Reader
		getCharacter(String str)
		{
			return (new Reader(str));
		} // getCharacter


		/*
		** Name: cnvtAscii
		**
		** Description:
		**	Helper method to convert ASCII stream into 
		**	character stream.
		**
		** Input:
		**	stream	ASCII stream to be converted.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Reader	Character stream.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public static Reader
		cnvtAscii(InputStream stream)
		{
			try { return (new InputStreamReader(stream, "US-ASCII")); }
			catch (Exception)
			{ throw SqlEx.get(ERR_GC401E_CHAR_ENCODE); }	// Should not happen!
		} // cnvtAscii


		/*
		** Name: cnvtUnicode
		**
		** Description:
		**	Helper method to convert Unicode stream into 
		**	character stream.
		**
		** Input:
		**	stream	Unicode stream to be converted.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Reader	Character stream.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public static Reader
		cnvtUnicode(InputStream stream)
		{
			try { return (new InputStreamReader(stream, "UTF-8")); }
			catch (Exception)
			{ throw SqlEx.get(ERR_GC401E_CHAR_ENCODE); }	// Should not happen!
		} // cnvtUnicode


		/*
		** Name: SqlData
		**
		** Description:
		**	Constructor for sub-classes to define initial NULL
		**	state of the data value.
		**
		** Input:
		**	is_null		True if data value is initially NULL.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		protected internal
			SqlData( bool is_null )
		{
			this.is_null = is_null;
		} // SqlData


		/*
		** Name: ToString
		**
		** Description:
		**	Returns a string representing the current object.
		**
		**	Overrides super-class method to attempt conversion
		**	of SQL value to string using getString() method.
		**	Since getString() may not be implemented by a sub-
		**	class, an exception from getString() defaults to
		**	using the super-class method.
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public override String 
			ToString()
		{
			String str;

			try { str = getString(); }
			catch( SqlEx ) { str = base.ToString(); }

			return( str );
		} // toString


		/*
		** Name: isNull
		**
		** Description:
		**	Returns the NULL state of the data value.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	bool		True if data value is NULL.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		internal virtual bool
			isNull()
		{
			return (is_null);
		} // isNull


		/*
		** Name: setNull
		**
		** Description:
		**	Set the NULL state of the data value to NULL.
		**
		**	Sub-classes may need to override this method if the
		**	stored data value is dependent on the NULL state.
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public virtual void
			setNull()
		{
			is_null = true;
			return;
		} // setNull


		/*
		** Name: setNotNull
		**
		** Description:
		**	Set the NULL state of the data value to not NULL.
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public virtual void
			setNotNull()
		{
			is_null = false;
			return;
		} // setNotNull


		/*
		** Name: isTruncated
		**
		** Description:
		**	Returns an indication that the data value was truncated.
		**
		**	This implementation always returns False.  Sub-classes should 
		**	override this method to support truncatable datatypes.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	bool		True if truncated.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public virtual bool
			isTruncated()
		{
			return( false );
		} // isTruncated


		/*
		** Name: getDataSize
		**
		** Description:
		**	Returns the expected size of an untruncated data value.
		**	For datatypes whose size is unknown or varies, -1 is returned.
		**	The returned value is only valid for truncated data values
		**	(isTruncated() returns True).
		**
		**	This implementation always returns -1.  Sub-classes should 
		**	override this method to support truncatable datatypes.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int    Expected size of data value.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public virtual int
			getDataSize()
		{
			return( -1 );
		} // getDataSize


		/*
		** Name: getTruncSize
		**
		** Description:
		**	Returns the actual size of a truncated data value.  For 
		**	datatypes whose size is unknown or varies, -1 is returned.
		**	The returned value is only valid for truncated data values
		**	(isTruncated() returns True).
		**
		**	This implementation always returns 0.  Sub-classes should 
		**	override this method to support truncatable datatypes.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int    Truncated size of data value.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public virtual int
			getTruncSize()
		{
			return( 0 );
		} // getTruncSize


		/*
		** Name: setScale
		**
		** Description:
		**	Set decimal scale of SQL data value.
		**
		**	Default implementation is no-op.  Sub-classes which
		**	have scale as an attribute of their SQL type should
		**	override this method.  
		**
		** Input:
		**	scale	Number of decimal places.
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

		protected virtual void
		setScale( int scale )
		{ 
			return; 
		} // setScale


		/*
		** Name: setObject
		**
		** Description:
		**	Assign a Java Object as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		**	The scale input is only referenced if
		**	permitted by the use_scale input, and
		**	then only for applicable SQL types.
		**
		**	Note: the object class coercion is dependent
		**	on the class/SQL type association performed
		**	by the getSqlType() method.
		**
		** Input:
		**	value		Java Object (may be null).
		**	use_scale	True if scale is valid.
		**	scale		Scale.
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
		setObject( Object value, bool use_scale, int scale ) 
		{
			switch( getSqlType( value ) )
			{
			case ProviderType.DBNull :  setNull();                  break;
			case ProviderType.Boolean:  setBoolean((Boolean)value); break;
			case ProviderType.SmallInt: setShort((short)value);     break;
			case ProviderType.Integer:  setInt((int)value);         break;
			case ProviderType.BigInt:   setLong((long)value);       break;
			case ProviderType.Real:     setFloat((Single)value);    break;
			case ProviderType.Double:   setDouble((Double)value);   break;
			case ProviderType.TinyInt:
				if (value is Byte)
					setByte((Byte)value);
				else
					setByte(unchecked((Byte)(SByte)value));
				break;
			case ProviderType.Binary:
				if (value is Byte)
					setByte((byte)value);
				else
					setBytes((byte[])value);
				break;
			case ProviderType.LongVarBinary: setBinaryStream((InputStream)value); break;
			case ProviderType.Char: setString(new String((char[])value)); break;
			case ProviderType.VarChar: setString((String)value); break;
			case ProviderType.LongVarChar: setCharacterStream((Reader)value); break;
			case ProviderType.Date: setDate((DateTime)value, null); break;
			case ProviderType.Time: setTime((DateTime)value, null); break;
			case ProviderType.DateTime: setTimestamp((DateTime)value, null); break;
			case ProviderType.IntervalDayToSecond: setTimeSpan((TimeSpan)value); break;

			case ProviderType.Decimal:
			if ( ! use_scale )
				setDecimal( (Decimal)value );	
			else
			{
				setDecimal( (Decimal)value /*, scale */ );
				use_scale = false;	// Scale used, don't reference further.
			}
			break;
			
			default :
			throw SqlEx.get( ERR_GC401A_CONVERSION_ERR );	// Shouldn't happen
			}

			/*
			** If scale could not be applied above, it may still
			** be applied if a sub-class implements setScale().
			*/
			if ( use_scale )  setScale( scale );

			return;
		} // setObject


		/*
		** The following routines represent the set of valid accessor
		** assignment methods for the data values supported by SQL.  
		** For this implementation these methods throw data conversion 
		** errors.  Sub-classes should override those methods for which 
		** conversion of their datatype is supported.
		*/

		public virtual void
		setBoolean(bool value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setByte(byte value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setShort(short value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setInt(int value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setLong(long value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setFloat(float value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setDouble(double value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setDecimal(Decimal value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setString(String value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setBytes(byte[] value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setDate(DateTime value, TimeZone tz)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setTime(DateTime value, TimeZone tz)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setTimestamp(DateTime value, TimeZone tz)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setTimeSpan(TimeSpan value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setBinaryStream(InputStream value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setAsciiStream(InputStream value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setUnicodeStream(InputStream value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual void
		setCharacterStream(Reader value)
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }


		/*
		** The following routines represent the set of valid accessor
		** retrieval methods for the data values supported by SQL.  
		** For this implementation these methods throw data conversion 
		** errors.  Sub-classes should override those methods for which 
		** conversion of their datatype is supported.
		*/

		/*
		** The following routines represent the set of valid accessor
		** methods for the data values supported by SQL.  For this
		** implementation these methods throw data conversion errors.  
		** Sub-classes should override those methods for which conversion 
		** of their datatype is supported.
		*/

		public virtual bool 
			getBoolean()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual byte 
			getByte()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual short 
			getShort()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual int 
			getInt()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual long 
			getLong()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual float 
			getFloat()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual double 
			getDouble()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual Decimal 
			getDecimal()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual String 
			getString()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual byte[] 
			getBytes()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual DateTime 
			getDate( TimeZone tz )
		{  throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual DateTime 
			getTime( TimeZone tz )
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual DateTime 
			getTimestamp( TimeZone tz )
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual TimeSpan
			getTimeSpan()
		{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

		public virtual InputStream 
			getBinaryStream()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual InputStream 
			getAsciiStream()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual InputStream 
			getUnicodeStream()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual Reader 
			getCharacterStream()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

		public virtual Object 
			getObject()
		{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }


		/*
		** The following methods are just covers for other
		** stub methods implemented above.  A sub-class may
		** implement these methods if parameter depedencies
		** exist.
		*/

		public virtual void 
		setDecimal( Decimal value, int scale )
		{ setDecimal( value ); }

		public virtual void 
		setObject( Object value )
		{ setObject( value, false, 0 ); }

		public virtual void 
		setObject( Object value, int scale )
		{ setObject( value, true, scale ); }

		public virtual Decimal 
		getDecimal( int scale )
		{ return( getDecimal() ); }

		public virtual String
		getString( int limit )
		{ return( getString() ); }

		public virtual byte[]
		getBytes( int limit )
		{ return( getBytes() ); }

		public virtual Object
		getObject( int limit )
		{ return( getObject() ); }

	} // class SqlData

	/// <summary>
	/// Class representing an input stream of bytes
	/// usually from a NetworkStream or MemoryStream(bytes).
	/// Base class is Stream.
	/// </summary>
	internal class InputStream : System.IO.Stream
	{
		System.IO.Stream stream;

		public static System.IO.Stream InputStreamNull = null;

		public InputStream(System.IO.Stream stream)
			: base()
		{
			this.stream = stream;  // save probably NetworkStream from Socket
		}

		public InputStream(byte[] buf) :
			this(buf, 0, buf.Length)
		{}

		public InputStream(byte[] buf, int maxlength) :
			this(buf, 0, (maxlength>0)?
				System.Math.Min(buf.Length, maxlength) : buf.Length )
		{}

		public InputStream(byte[] buf, int offset, int count) :
			base()
		{ this.stream = new System.IO.MemoryStream(buf, offset, count); }


		public virtual int read()
		{
			byte[] buffer = new byte[1];

			int countread = this.Read(buffer, 0, 1);
			if (countread == -1)
				return (-1);
			return (int)buffer[0];
		}

		public virtual int read(byte[] buffer)
		{
			return this.Read(buffer, 0, buffer.Length);
		}

		public virtual int read(byte[] buffer, int offset, int count)
		{
			return this.Read(buffer, offset, count);
		}

		public override int Read(byte[] buffer, int offset, int count)
		{
			if (stream == null)
				return (-1);
			int countread = stream.Read(buffer, offset, count);
			if (countread == 0)
				return (-1);
			return countread;
		}

		public virtual void close()
		{
			this.Close();
		}

		public override void Close()
		{
			if (stream == null)
				return;
			stream.Close();
		}

		public override bool CanRead
		{
			get
			{
				if (stream == null)
					return true;
				return stream.CanRead;
			}
		}

		public override bool CanSeek
		{
			get { return stream.CanSeek; }
		}  // CanSeek

		public override bool CanWrite
		{
			get
			{
				if (stream == null)
					return false;
				return stream.CanWrite;
			}
		}  // CanWrite

		public override long Length
		{
			get { return stream.Length; }
		}  // Length

		public override long Position
		{
			get { return stream.Position; }
			set { stream.Position = value; }
		}  // Position

		public override long Seek(long offset, System.IO.SeekOrigin origin)
		{
			return stream.Seek(offset, origin);
		}  // Seek

		public override void SetLength(long length)
		{
			stream.SetLength(length);
		}  // SetLength

		public override void Flush()
		{
			throw new Exception("The method Flush is not implemented.");
		}

		public override void Write(byte[] buffer, int offset, int count)
		{
			throw new Exception("The method Write is not implemented.");
		}

	}  // InputStream

	/// <summary>
	/// Class to read a byte stream and convert it into a character stream
	/// using an encoding.
	/// </summary>
	internal class InputStreamReader : Reader
	{
		private System.IO.StreamReader inStream;

		public InputStreamReader(
			InputStream stream)
		{
			inStream = new System.IO.StreamReader(stream);
		}

		public InputStreamReader(
			InputStream stream, System.Text.Encoding encoding)
		{
			inStream = new System.IO.StreamReader(stream, encoding);
		}

		public InputStreamReader(
			InputStream stream, String charSetName)
		{
			System.Text.Encoding encoding;
			if (charSetName == "UTF-8")
				encoding = System.Text.Encoding.UTF8;
			else if (charSetName == "ASCII"  ||  charSetName == "US-ASCII" )
				encoding = System.Text.Encoding.ASCII;
			else throw new ArgumentOutOfRangeException(
				"charSetName", "Must be \"UTF-8\" or \"ASCII\".");

			inStream = new System.IO.StreamReader(stream, encoding);
		}

		/// <summary>
		/// Read a single character.
		/// </summary>
		/// <returns></returns>
		public override int read()
		{
			return this.Read();
		}

		/// <summary>
		/// Read a single character.
		/// </summary>
		/// <returns></returns>
		public override int Read()
		{
			return inStream.Read();
		}

		public override int read(char[] buffer, int index, int count)
		{
			return this.Read(buffer, index, count);
		}

		public override int Read(char[] buffer, int index, int count)
		{
			int countread = inStream.Read(buffer, index, count);
			if (countread == 0)
				return (-1);
			return countread;
		}

		public override void close()
		{
			this.Close();
		}

		public override void Close()
		{
			inStream.Close();
		}

	}

	internal class ByteArrayInputStream : InputStream
	{
		public ByteArrayInputStream(byte[] buf) :
			base(buf)
		{}

		public ByteArrayInputStream(byte[] buf, int maxlength) :
			base(buf, 0, (maxlength>0)?
				System.Math.Min(buf.Length, maxlength) : buf.Length )
	{}

		public ByteArrayInputStream(byte[] buf, int offset, int count) :
			base(buf, offset, count)
		{}
	}

	/// <summary>
	/// Class representing an output stream of bytes
	/// (to NetworkStream on the socket).
	/// </summary>
	internal class OutputStream : System.IO.Stream
	{
		private System.IO.Stream stream;

		public OutputStream(System.IO.Stream stream)
			:  base()
		{
			this.stream = stream;
		}

		public virtual void write(int b)
		{
			this.write(Convert.ToByte(b));
		}

		public virtual void write(byte b)
		{
			byte[] ba = new byte[1];
			ba[0] = b;
			this.Write(ba, 0, 1);
		}

		public virtual void write(SByte b)
		{
			write(unchecked((byte)b));
		}

		public virtual void write(byte[] b, int offset, int length)
		{
			this.Write(b, offset, length);
		}

		public override void Write(byte[] b, int offset, int length)
		{
			stream.Write(b, offset, length);
		}

		public virtual void flush()
		{
			this.Flush();
		}

		public override void Flush()
		{
			stream.Flush();
		}

		public virtual void close()
		{
			this.Close();
		}

		public override void Close()
		{
			stream.Close();
		}

		public override bool CanRead
		{
			get { return stream.CanRead; }
		}

		public override bool CanSeek
		{
			get { return stream.CanSeek; }
		}  // CanSeek

		public override bool CanWrite
		{
			get { return stream.CanWrite; }
		}  // CanWrite

		public override long Length
		{
			get { return stream.Length; }
		}  // Length

		public override long Position
		{
			get { return stream.Position; }
			set { stream.Position = value; }
		}  // Position

		public override long Seek(long offset, System.IO.SeekOrigin origin)
		{
			return stream.Seek(offset, origin);
		}  // Seek

		public override void SetLength(long length)
		{
			stream.SetLength(length);
		}  // SetLength

		public override int Read(byte[] buffer, int offset, int count)
		{
			int countread = stream.Read(buffer, offset, count);

			if (countread == 0)
				return (-1);
			return countread;
		}

	}  // OutputStream


	/// <summary>
	/// Class for reading character streams.
	/// </summary>
	internal class Reader : System.IO.StringReader
	{
		public Reader() : base(String.Empty)
		{}

		public Reader(String s) : base(s)
		{}

		/// <summary>
		/// Read and return the next character, or -1 if end of stream.
		/// </summary>
		/// <returns>The next character from the stream or
		/// -1 if end of stream.</returns>
		public virtual int read()
		{
			return this.Read();
		}

		/// <summary>
		/// Read next characters into the char array.
		/// </summary>
		/// <returns>The number of characters read from the stream or
		/// -1 if end of stream.</returns>
		public virtual int read(char[] buffer)
		{
			return this.Read(buffer, 0, buffer.Length);
		}

		public virtual int read(char[] buffer, int index, int count)
		{
			return this.Read(buffer, index, count);
		}

		public override int Read(char[] buffer, int index, int count)
		{
			int countread = base.Read(buffer, index, count);
			if (countread == 0)
				return -1;
			return countread;
		}

		public virtual void close()
		{
			this.Close();
		}

		public override void Close()
		{
			base.Close();
		}

	}  // class Reader

	/// <summary>
	/// Base class for writing character strings.
	/// </summary>
	internal class Writer : System.IO.StreamWriter
	{
		public Writer()
			: base(System.IO.Stream.Null)
		{ }

		public Writer(System.IO.Stream stream)
			: base(stream)
		{ }

		public Writer(System.IO.Stream stream, System.Text.Encoding encoding)
			: base(stream, encoding)
		{ }

		public virtual void write(char[] buffer, int index, int count)
		{
			this.Write(buffer, index, count);
		}

		public override void Write(char[] buffer, int index, int count)
		{
			base.Write(buffer, index, count);
		}

		public virtual void flush()
		{
			this.Flush();
		}

		public override void Flush()
		{
			base.Flush();
		}

		public virtual void close()
		{
			this.Close();
		}

		public override void Close()
		{
			base.Close();
		}
	}

	/// <summary>
	/// Base class for a writer for writing character strings to a byte stream.
	/// </summary>
	internal class OutputStreamWriter : Writer
	{
		public OutputStreamWriter(OutputStream stream)
			: base(stream)
		{ }

		public OutputStreamWriter(OutputStream stream, System.Text.Encoding encoding)
			: base(stream, encoding)
		{ }
	}  // class OutputStreamWriter

}  // namespace
