/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{

	/*
	** Name: sqlvarbyte.cs
	**
	** Description:
	**	Defines class which represents an SQL VarBinary value.
	**
	**  Classes:
	**
	**	SqlVarByte	An SQL VarBinary value.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 1-Dec-03 (gordy, ported by thoda04 1-Oct-06)
	**	    Added support for parameter types/values in addition to 
	**	    existing support for columns.
	*/


	/*
	** Name: SqlVarByte
	**
	** Description:
	**	Class which represents an SQL VarBinary value.  SQL VarBinary 
	**	values are variable length.
	**
	**	Supports conversion to String, Object and streams.  
	**
	**	This class implements the ByteArray interface as the means
	**	to set the binary value.  The optional size limit defines
	**	the maximum length of the value.  Capacity and length vary
	**	as needed to handle the actual binary value size.
	**
	**	The binary value may be set by first clearing the array and 
	**	then using the put() method to set the value.  Segmented 
	**	input values are handled by successive calls to put().  The 
	**	clear() method also clears the NULL setting.
	**
	**	The data value accessor methods do not check for the NULL
	**	condition.  The super-class isNull() method should first
	**	be checked prior to calling any of the accessor methods.
	**
	**  Interface Methods:
	**
	**	capacity		Determine capacity of the array.
	**	ensureCapacity		Set minimum capacity of the array.
	**	limit			Set or determine maximum length of the array.
	**	length			Determine the current length of the array.
	**	clear			Set array length to zero.
	**	set			Assign a new data value.
	**	put			Copy bytes into the array.
	**	get			Copy bytes out of the array.
	**
	**  Public Methods:
	**
	**	bin2str			Convert byte array to hex string.
	**
	**	setString		Data value accessor assignment methods
	**	setBytes
	**
	**	getString		Data value accessor retrieval methods
	**	getBytes
	**	getBinaryStream
	**	getAsciiStream
	**	getUnicodeStream
	**	getCharacterStream
	**	getObject
	**
	**  Protected Data:
	**
	**	value			The binary value.
	**	limit			Size limit.
	**	length			Current length.
	**
	**  Protected Methods:
	**
	**	ensure			Ensure minimum capacity.
	**
	**  Private Data:
	**
	**	empty			Default empty value.
**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 1-Dec-03 (gordy, ported by thoda04 1-Oct-06)
	**	    Removed dependency on SqlByte and implemented SqlData and
	**	    ByteArray interfaces to support unlimited length strings.
	**	    Added parameter data value oriented methods.
	*/

	internal class
		SqlVarByte :  SqlData, IByteArray
	{

		private static byte[] empty = new byte[ 0 ];

		public byte[] value = empty;
		public  int limit = -1;      // Max (initially unlimited)

		private int _length;
		public virtual int length   // Current length
		{
			get { return _length;  }
			set { _length = value; }
		}

		/*
		** Name: SqlVarByte
		**
		** Description:
		**	Class constructor for variable length binary values.
		**	Data value is initially NULL.
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
		*/

		public
		SqlVarByte() : base(true)
		{
		} // SqlVarByte


		/*
		** Name: SqlVarByte
		**
		** Description:
		**	Class constructor.  Data value is initially NULL.
		**
		** Input:
		**	size		The maximum size of the binary value.
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

		public
			SqlVarByte( int size ) : this()
		{
			limit = size;
		} // SqlVarByte


		/*
		** Name: capacity
		**
		** Description:
		**	Returns the current capacity of the array.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Current capacity.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public int
			capacity()
		{
			return (value == null ? 0 : value.Length);
		} // capacity


		/*
		** Name: ensureCapacity
		**
		** Description:
		**	Set minimum capacity of the array.  Negative values,
		**	values less than current capacity and greater than
		**	(optional) limit are ignored.
		**
		** Input:
		**	capacity	Minimum capacity.
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
			ensureCapacity(int capacity)
		{
			ensure(capacity);
			return;
		} // ensureCapacity


		/*
		** Name: limit
		**
		** Description:
		**	Determine the current maximum size of the array.
		**	If a maximum size has not been set, -1 is returned.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Maximum size or -1.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public virtual int
			valuelimit()
		{
			return (limit);
		} // limit


		/*
		** Name: limit
		**
		** Description:
		**	Set the maximum size of the array.  A negative size removes 
		**	any prior size limit.  The array will be truncated if the 
		**	current length is greater than the new maximum size.  Array 
		**	capacity is not affected.
		**
		** Input:
		**	size	Maximum size.
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
		**	    Support unlimited length values.
		*/

		public virtual void
			valuelimit( int size )
		{
			limit = (size < 0) ? -1 : size;
			if (limit >= 0 && length > limit)
				length = limit;
			return;
		} // limit


		/*
		** Name: limit
		**
		** Description:
		**	Set the maximum size of the array and optionally ensure
		**	the mimimum capacity.  A negative size removes any prior 
		**	size limit.  The array will be truncated if the current 
		**	length is greater than the new maximum size.  Array
		**	capacity will not decrease but may increase.
		**
		**	Note that valuelimit(n, true) is equivilent to valuelimit(n)
		**	followed by ensureCapacity(n) and valuelimit(n, false) is
		**	the same as valuelimit(n).
		**
		** Input:
		**	size	Maximum size.
		**	ensure	True if capacity should be ensured as max size.
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
		**	    Adapted for SqlVarByte.
		*/

		public virtual void
			valuelimit(int size, bool do_ensure)
		{
			valuelimit(size);
			if (do_ensure) ensure(size);
			return;
		} // limit


		/*
		** Name: length
		**
		** Description:
		**	Returns the current length of the array.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Current length.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public virtual int
		valuelength()
		{
			return (length);
		} // length


		/*
		** Name: clear
		**
		** Description:
		**	Sets the length of the array to zero.  Also clears NULL setting.
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
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public virtual void
		clear()
		{
			setNotNull();
			length = 0;
			return;
		} // clear


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value as a copy of an existing 
		**	SQL data object.  If the input is NULL, a NULL 
		**	data value results.
		**
		** Input:
		**	data	The SQL data to be copied.
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
			if (data == null || data.isNull())
				setNull();
			else
			{
				clear();
				put(data.value, 0, data.length);
			}
			return;
		} // set


		/*
		** Name: put
		**
		** Description:
		**	Append a byte value to the current array data.  The portion
		**	of the input which would cause the array to grow beyond the
		**	(optional) size limit is ignored.  Number of bytes actually
		**	appended is returned.
		**
		** Input:
		**	value	The byte to append.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Number of bytes appended.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public virtual int 
		put(byte value)
		{
			/*
			** Is there room for the byte.
			*/
			if (limit >= 0 && length >= limit) return (0);

			/*
			** Save byte and update array length.
			*/
			ensure(length + 1);
			this.value[length++] = value;
			return (1);
		}


		/*
		** Name: put
		**
		** Description:
		**	Append a byte array to the current array data.  The portion
		**	of the input which would cause the array to grow beyond the
		**	size limit (if set) is ignored.  The number of bytes actually 
		**	appended is returned.
		**
		** Input:
		**	value	    The byte array to append.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Number of bytes appended.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public virtual int
		put(byte[] value)
		{
			return (put(value, 0, value.Length));
		} // put


		/*
		** Name: put
		**
		** Description:
		**	Append a portion of a byte array to the current array data.  
		**	The portion of the input which would cause the array to grow 
		**	beyond the size limit (if set) is ignored.  The number of 
		**	bytes actually appended is returned.
		**
		** Input:
		**	value	    Array containing bytes to be appended.
		**	offset	    Start of portion to append.
		**	length	    Number of bytes to append.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Number of bytes appended.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public virtual int
		put(byte[] value, int offset, int length)
		{
			/*
			** Determine how many bytes to copy.
			*/
			int unused = (limit < 0) ? length : limit - this.length;
			if (length > unused) length = unused;

			/*
			** Copy bytes and update array length.
			*/
			ensure(this.length + length);
			System.Array.Copy(value, offset, this.value, this.length, length);
			this.length += length;
			return (length);
		} // put


		/*
		** Name: get
		**
		** Description:
		**	Returns the byte value at a specific position in the
		**	array.  If position is beyond the current array length, 
		**	0 is returned.
		**
		** Input:
		**	position	Array position.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte		Byte value or 0.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public virtual byte
		get(int position)
		{
			return ((byte)((position >= length) ? (byte)0 : (value[position])));
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Returns a copy of the array.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	Copy of the current array.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public virtual byte[]
		get()
		{
			byte[] bytes = new byte[length];
			System.Array.Copy(value, 0, bytes, 0, length);
			return (bytes);
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Copy bytes out of the array.  Copying starts with the first
		**	byte of the array.  The number of bytes copied is the minimum
		**	of the current array length and the length of the output array.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	Byte array to receive output.
		**
		** Returns:
		**	int	Number of bytes copied.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public virtual int
		get(byte[] value)
		{
			return (get(0, value.Length, value, 0));
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Copy a portion of the array.  Copying starts at the position
		**	indicated.  The number of bytes copied is the minimum of the
		**	length requested and the number of bytes in the array starting
		**	at the requested position.  If position is greater than the
		**	current length, no bytes are copied.  The output array must
		**	have sufficient space.  The actual number of bytes copied is
		**	returned.
		**
		** Input:
		**	position	Starting byte to copy.
		**	length		Number of bytes to copy.
		**	offset		Starting position in output array.
		**
		** Output:
		**	value		Byte array to recieve output.
		**
		** Returns:
		**	int		Number of bytes copied.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public virtual int
		get(int position, int length, byte[] value, int offset)
		{
			/*
			** Determine how many bytes to copy.
			*/
			int avail = (position >= this.length) ?
				0 : this.length - position;

			if (length > avail) length = avail;

			System.Array.Copy(this.value, position, value, offset, length);
			return (length);
		} // get


		/*
		** Name: ensure
		**
		** Description:
		**	Set minimum capacity of the array.  Values less than
		**	current capacity and greater than (optional) limit are
		**	ignored.
		**
		** Input:
		**	capacity	Required capacity.
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
		**	13-Oct-03 (gordy)
		**	    Forgot to reset value after copy into temp buffer.
		**	 1-Dec-03 (gordy)
		**	    Support unlimited string lengths.  Value never null.
		*/

		protected internal virtual void
			ensure( int capacity )
		{
			/*
			** Capacity does not need to exceed (optional) limit.
			*/
			if (limit >= 0 && capacity > limit) capacity = limit;

			if ( capacity > value.Length)
			{
				byte[] ba = new byte[ capacity ];
				System.Array.Copy(value, 0, ba, 0, length);
				value = ba;
			}
			return;
		} // ensure

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
				setBytes(System.Text.Encoding.UTF8.GetBytes(value));
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
			{
				clear();
				put(value);
			}
			return;
		} // setBytes


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
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override String
		getString()
		{
			return (getString(length));
		} // getString


		/*
		** Name: getString
		**
		** Description:
		**	Return the current data value as a String value
		**	with maximum size limit.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	limit	Maximum size of result.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Current value converted to String.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override  String
			getString(int limit)
		{
			if (limit > length) limit = length;
			return (bin2str(value, 0, limit));
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
		**	byte[]		Current value converted to byte array.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override byte[]
			getBytes()
		{
			return (getBytes(length));
		}


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
		**	limit	Maximum size of result.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	byte[]	Current value converted to byte array.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override byte[]
			getBytes(int limit)
		{
			if (limit > length) limit = length;
			byte[] bytes = new byte[limit];
			get(0, limit, bytes, 0);
			return (bytes);
		}


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
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override InputStream
			getBinaryStream()
		{
			return (getBinary(value, 0, length));
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override InputStream
			getAsciiStream()
		{
			return (getAscii(getString()));
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override InputStream
			getUnicodeStream()
		{
			return (getUnicode(getString()));
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override Reader
			getCharacterStream()
		{
			return (getCharacter(getString()));
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override Object
			getObject()
		{
			return (getBytes());
		} // getObject


		/*
		** Name: getObject
		**
		** Description:
		**	Return the current data value as an Binary object
		**	with maximum size limit.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	limit	Maximum size of result.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Object	Current value converted to Binary.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public override Object
			getObject(int limit)
		{
			return (getBytes(limit));
		} // getObject


		/*
		** Name: bin2str
		**
		** Description:
		**	Convert a byte array to a hex string.
		**
		** Input:
		**	bytes	Byte array.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Hex string representation.
		**
		** History:
		**	31-Jan-99 (rajus01)
		**	    Created.
		**	 5-Sep-03 (gordy)
		**	    Generalize parameters to allows sub-arrays.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarByte.
		*/

		public static String
			bin2str(byte[] bytes, int offset, int count)
		{
			StringBuilder sb = new StringBuilder(count * 2);
			String str;

			for (; count > 0; offset++, count--)
			{
				str = toHexString(bytes[offset]);
				if (str.Length == 1) sb.Append('0');
				sb.Append(str);
			}

			return (sb.ToString());
		} // bin2str



	} // class SqlVarByte
}  // namespace
