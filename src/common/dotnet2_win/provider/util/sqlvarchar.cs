/*
** Copyright (c) 2003, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{
	/*
	** Name: SqlVarChar.cs
	**
	** Description:
	**	Defines class which represents an SQL VarChar value.
	**
	**  Classes:
	**
	**	SqlVarChar	An SQL VarChar value.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 1-Dec-03 (gordy)
	**	    Added support for parameter types/values in addition to 
	**	    existing support for columns.
	**	24-May-06 (gordy)
	**	    Convert parsing exception to SQL conversion exception.
	**	21-Mar-07 (thoda04)
	**	    Added setTimeSpan override method to support IntervalDayToSecond.
	**	 9-dec-09 (thoda04)
	**	    Use BooleanParse() for BOOLEAN string parsing.
	*/


	/*
	** Name: sqlvarchar
	**
	** Description:
	**	Class which represents an SQL VarChar value.  SQL VarChar 
	**	values are variable length byte based strings.  A character-
	**	set is associated with the byte string for conversion to a 
	**	Java String.
	**
	**	Supports conversion to boolean, byte, short, int, long, float, 
	**	double, BigDecimal, Date, Time, Timestamp, Object and streams.  
	**
	**	This class implements the ByteArray interface as the means
	**	to set the string value.  The optional size limit defines
	**	the maximum length of the string.  Capacity and length vary
	**	as needed to handle the actual string size.
	**
	**	The string value may be set by first clearing the array and 
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
	**	setBoolean		Data value accessor assignment methods
	**	setByte
	**	setShort
	**	setInt
	**	setLong
	**	setFloat
	**	setDouble
	**	setBigDecimal
	**	setString
	**	setDate
	**	setTime
	**	setTimestamp
	**
	**	getBoolean		Data value accessor retrieval methods
	**	getByte
	**	getShort
	**	getInt
	**	getLong
	**	getFloat
	**	getDouble
	**	getBigDecimal
	**	getString
	**	getDate
	**	getTime
	**	getTimestamp
	**	getAsciiStream
	**	getUnicodeStream
	**	getCharacterStream
	**	getObject
	**
	**  Protected Data:
	**
	**	value			String value.
	**	limit			Size limit.
	**	length			Current length.
	**	charSet			Character-set.
	**
	**  Protected Methods:
	**
	**	ensure			Ensure minimum capacity.
	**
	**  Private Data:
	**
	**	empty			Default empty string.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 1-Dec-03 (gordy)
	**	    Removed dependency on SqlChar and implemented SqlData and
	**	    ByteArray interfaces to support unlimited length strings.
	**	    Added parameter data value oriented methods.
	*/

	internal class
		SqlVarChar : SqlData, IByteArray
	{

		private static byte[] empty = new byte[0];

		public byte[] value = empty;
		protected int limit = -1;	// Max (initially unlimited)
		protected CharSet charSet = null;

		private int _length = 0;
		public virtual int length	// Current length
		{
			get { return _length; }
			set { _length = value; }
		}

		/*
		** Name: SqlVarChar
		**
		** Description:
		**	Class constructor for variable length byte strings.  
		**	Data value is initially NULL.
		**
		** Input:
		**	charSet		Character-set of byte strings
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
		SqlVarChar( CharSet charSet ) : base( true )
		{
			this.charSet = charSet;
		} // SqlVarChar


		/*
		** Name: SqlVarChar
		**
		** Description:
		**	Class constructor for limited length byte strings.  
		**	Data value is initially NULL.
		**
		** Input:
		**	charSet		Character-set of byte strings
		**	size		The maximum string length.
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
		**	 1-Dec-03 (gordy)
		**	    Adapted to new super class.
		*/

		public
			SqlVarChar( CharSet charSet, int size ) : this( charSet )
		{
			limit = size;
		} // SqlVarChar


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
		**	    Adapted for SqlVarChar.
		*/

		public int
		capacity()
		{
			return( value.Length );
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
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		*/

		public void
		ensureCapacity( int capacity )
		{
			ensure( capacity );
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
			return( limit );
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
		**	    Support unlimited length strings.
		*/

		public virtual void
			valuelimit( int size )
		{
			limit = (size < 0) ? -1 : size;
			if ( limit >= 0  &&  length > limit )  length = limit;
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
		**	Note that limit(n, true) is equivilent to limit(n) followed 
		**	by ensureCapacity(n) and limit(n, false) is the same as 
		**	limit(n).
		**
		** Input:
		**	size	Maximum size.
		**	ensure	True to ensure capacity.
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
		**	    Adapted for SqlVarChar.
		*/

		public virtual void
		valuelimit( int size, bool do_ensure )
		{
			valuelimit( size );
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
		**	    Adapted for SqlVarChar.
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
		**	    Adapted for SqlVarChar.
		*/

		public void
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
		set(SqlVarChar data)
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
		**	    Adapted for SqlVarChar.
		*/

		public int
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
		**	(optional) size limit is ignored.  The number of bytes actually 
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
		**	    Adapted for SqlVarChar.
		*/

		public int
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
		**	beyond the (optional) size limit is ignored.  The number of 
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
		**	    Adapted for SqlVarChar.
		*/

		public int
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
			return ((byte)((position >= length) ? (byte)0 : value[position]));
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
		**	    Adapted for SqlVarChar.
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
		**	    Adapted for SqlVarChar.
		*/

		public int
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
		**	    Adapted for SqlVarChar.
		*/

		public virtual int
		get(int position, int length, byte[] value, int offset)
		{
			/*
			** Determine how many bytes to copy.
			*/
			int avail = (position >= this.length) ? 0 : this.length - position;
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

		protected void
		ensure(int capacity)
		{
			/*
			** Capacity does not need to exceed (optional) limit.
			*/
			if (limit >= 0 && capacity > limit) capacity = limit;

			if (capacity > value.Length)
			{
				byte[] ba = new byte[capacity];
				System.Array.Copy(value, 0, ba, 0, length);
				value = ba;
			}
			return;
		} // ensure


		/*
		** Name: setBoolean
		**
		** Description:
		**	Assign a boolean value as the non-NULL data value.
		**
		** Input:
		**	value	Boolean value.
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
		setBoolean(bool value)
		{
			setString(value.ToString());
			return;
		}


		/*
		** Name: setByte
		**
		** Description:
		**	Assign a byte value as the non-NULL data value.
		**
		** Input:
		**	value	Byte value.
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
		setByte(byte value)
		{
			setString(value.ToString());
			return;
		}


		/*
		** Name: setShort
		**
		** Description:
		**	Assign a short value as the non-NULL data value.
		**
		** Input:
		**	value	Short value.
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
		setShort(short value)
		{
			setString(value.ToString());
			return;
		}


		/*
		** Name: setInt
		**
		** Description:
		**	Assign a integer value as the non-NULL data value.
		**
		** Input:
		**	value	Integer value.
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
		setInt(int value)
		{
			setString(value.ToString());
			return;
		} // setInt


		/*
		** Name: setLong
		**
		** Description:
		**	Assign a long value as the non-NULL data value.
		**
		** Input:
		**	value	Long value.
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
		setLong(long value)
		{
			setString(value.ToString());
			return;
		} // setLong


		/*
		** Name: setFloat
		**
		** Description:
		**	Assign a float value as the non-NULL data value.
		**
		** Input:
		**	value	Float value.
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
		setFloat(float value)
		{
			setString(value.ToString());
			return;
		} // setFloat


		/*
		** Name: setDouble
		**
		** Description:
		**	Assign a double value as the non-NULL data value.
		**
		** Input:
		**	value	Double value.
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
		setDouble(double value)
		{
			setString(value.ToString());
			return;
		} // setDouble


		/*
		** Name: setBigDecimal
		**
		** Description:
		**	Assign a BigDecimal value as the data value.
		**	The data value will be NULL if the input value
		**	is null, otherwise non-NULL.
		**
		** Input:
		**	value	BigDecimal value (may be null).
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
		setDecimal(Decimal value)
		{
			// Decimal is never null
			//    if ( value == null )
			//	setNull();
			//    else
			setString(value.ToString());

			return;
		} // setDecimal


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
				byte[] ba;

				try { ba = charSet.getBytes(value); }
				catch (Exception ex)
				{ throw SqlEx.get(ERR_GC401E_CHAR_ENCODE, ex); }

				/*
				** Normally, ensure() would be used to size the internal
				** array, possibly allocating a new array.  The problem
				** is that a new array is already going to be allocated 
				** for Unicode conversion.  Rather than use ensure, which
				** at best will do nothing and at worst will allocate an
				** additional array, we simply use the longer of internal
				** of conversion array.
				*/
				if (ba.Length > this.value.Length)
				{
					/*
					** First determine the length of the new string
					** (apply limit if applicable).
					*/
					int length = ba.Length;
					if (limit >= 0 && length > limit) length = limit;

					/*
					** Now replace the internal buffer with the new string.
					*/
					setNotNull();
					this.value = ba;
					this.length = length;
				}
				else
				{
					/*
					** Since the converted string will fit in the current
					** internal buffer, we can just use standard access
					** methods to save the string.
					*/
					clear();
					put(ba);
				}
			}
			return;
		} // setString


		/*
		** Name: setDate
		**
		** Description:
		**	Assign a Date value as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		**	A relative timezone may be provided which 
		**	is applied to adjust the input such that 
		**	it represents the original date/time value 
		**	in the timezone provided.  The default is 
		**	to use the local timezone.
		**
		** Input:
		**	value	Date value (may be null).
		**	tz	Relative timezone, NULL for local.
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
		setDate(DateTime value, TimeZone tz)
		{
			//DateTime is never null
			//if (value == null)
			//    setNull();
			//else
			if (tz != null)
				setString(SqlDates.formatDate(value, tz));
			else
				setString(SqlDates.formatDate(value, false));
			return;
		} // setDate


		/*
		** Name: setTime
		**
		** Description:
		**	Assign a Time value as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		**	A relative timezone may be provided which 
		**	is applied to adjust the input such that 
		**	it represents the original date/time value 
		**	in the timezone provided.  The default is 
		**	to use the local timezone.
		**
		** Input:
		**	value	Time value (may be null).
		**	tz	Relative timezone, NULL for local.
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
		setTime(DateTime value, TimeZone tz)
		{
			//DateTime is never null
			//if (value == null)
			//    setNull();
			//else
			if (tz != null)
				setString(SqlDates.formatTime(value, tz));
			else
				setString(SqlDates.formatTime(value, false));
			return;
		} // setTime


		/*
		** Name: setTimestamp
		**
		** Description:
		**	Assign a Timestamp value as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		**	A relative timezone may be provided which 
		**	is applied to adjust the input such that 
		**	it represents the original date/time value 
		**	in the timezone provided.  The default is 
		**	to use the local timezone.
		**
		** Input:
		**	value	Timestamp value (may be null).
		**	tz	Relative timezone, NULL for local.
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
		setTimestamp(DateTime value, TimeZone tz)
		{
			//DateTime is never null
			//if (value == null)
			//    setNull();
			//else
			if (tz != null)
				setString(SqlDates.formatTimestamp(value, tz));
			else
				setString(SqlDates.formatTimestamp(value, false));
			return;
		} // setTimestamp


		public override void
		setTimeSpan(TimeSpan value)
		{
			setString(SqlInterval.ToStringIngresDayToSecondFormat(value));
		}


		/*
		** Name: getBoolean
		**
		** Description:
		**	Return the current data value as a boolean value.
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
		**	boolean		Current value converted to boolean.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		**	 9-dec-09 (thoda04)
		**	    Use BooleanParse() for BOOLEAN string parsing.
		*/

		public override bool
		getBoolean()
		{
			String str = getString(length);
			return BooleanParse(str);
		} // getBoolean


		/*
		** Name: getByte
		**
		** Description:
		**	Return the current data value as a byte value.
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
		**	byte	Current value converted to byte.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override byte
		getByte()
		{
			byte value;

			try { value = Byte.Parse(getString(length)); }
			catch (FormatException ex)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR, ex); }

			return (value);
		} // getByte


		/*
		** Name: getShort
		**
		** Description:
		**	Return the current data value as a short value.
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
		**	short	Current value converted to short.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override short
		getShort()
		{
			short value;

			try { value = Int16.Parse(getString(length)); }
			catch (FormatException)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

			return (value);
		} // getShort


		/*
		** Name: getInt
		**
		** Description:
		**	Return the current data value as a integer value.
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
		**	int	Current value converted to integer.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override int
		getInt()
		{
			int value;

			try { value = Int32.Parse(getString(length)); }
			catch (FormatException)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

			return (value);
		} // getInt


		/*
		** Name: getLong
		**
		** Description:
		**	Return the current data value as a long value.
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
		**	long	Current value converted to long.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override long
		getLong()
		{
			long value;

			try { value = Int64.Parse(getString(length)); }
			catch (FormatException)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

			return (value);
		} // getLong


		/*
		** Name: getFloat
		**
		** Description:
		**	Return the current data value as a float value.
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
		**	float	Current value converted to float.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override float
		getFloat()
		{
			float value;

			try { value = Single.Parse(getString(length)); }
			catch (FormatException)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

			return (value);
		} // getFloat


		/*
		** Name: getDouble
		**
		** Description:
		**	Return the current data value as a double value.
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
		**	double	Current value converted to double.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override double
		getDouble()
		{
			double value;

			try { value = Double.Parse(getString(length)); }
			catch (FormatException)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

			return (value);
		} // getDouble


		/*
		** Name: getDecimal
		**
		** Description:
		**	Return the current data value as a Decimal value.
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
		**	Decimal	Current value converted to BigDecimal.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override Decimal
		getDecimal()
		{
			Decimal value;

			try { value = DecimalParseInvariant(getString(length)); }
			catch (FormatException ex)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR, ex); }

			return (value);
		} // getDecimal


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
		**	    Adapted for SqlVarChar.
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
		**	    Adapted for SqlVarChar.
		*/

		public override String
		getString(int limit)
		{
			if (limit > length) limit = length;
			try { return (charSet.getString(value, 0, limit)); }
			catch (Exception ex)
			{ throw SqlEx.get(ERR_GC401E_CHAR_ENCODE, ex); }
		} // getString


		/*
		** Name: getDate
		**
		** Description:
		**	Return the current data value as a Date value.
		**	A relative timezone may be provided which is
		**	applied to adjust the final result such that it
		**	represents the original date/time value in the
		**	timezone provided.  The default is to use the
		**	local timezone.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	tz	Relative timezone, NULL for local.
		**	
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Date	Current value converted to Date.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		*/

		public override DateTime
		getDate(TimeZone tz)
		{
			String str = getString(length).Trim();
			return ((tz == null) ? SqlDates.parseDate(str, false)
					 : SqlDates.parseDate(str, tz));
		} // getDate


		/*
		** Name: getTime
		**
		** Description:
		**	Return the current data value as a Time value.
		**	A relative timezone may be provided which is
		**	applied to adjust the final result such that it
		**	represents the original date/time value in the
		**	timezone provided.  The default is to use the
		**	local timezone.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	tz	Relative timezone, NULL for local.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Time	Current value converted to Time.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		*/

		public override DateTime
		getTime(TimeZone tz)
		{
			String str = getString(length).Trim();
			return ((tz == null) ? SqlDates.parseTime(str, false)
					 : SqlDates.parseTime(str, tz));
		} // getTime


		/*
		** Name: getTimestamp
		**
		** Description:
		**	Return the current data value as a Timestamp value.
		**	A relative timezone may be provided which is
		**	applied to adjust the final result such that it
		**	represents the original date/time value in the
		**	timezone provided.  The default is to use the
		**	local timezone.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	tz	Relative timezone, NULL for local.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Timestamp	Current value converted to Timestamp.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlVarChar.
		*/

		public override DateTime
		getTimestamp(TimeZone tz)
		{
			String str = getString(length).Trim();
			return ((tz == null) ? SqlDates.parseTimestamp(str, false)
					 : SqlDates.parseTimestamp(str, tz));
		} // getTimestamp


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
		**	    Adapted for SqlVarChar.
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
		**	    Adapted for SqlVarChar.
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
		**	    Adapted for SqlVarChar.
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
		**	    Adapted for SqlVarChar.
		*/

		public override Object
		getObject()
		{
			return (getString());
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
		**	    Adapted for SqlVarChar.
		*/

		public override Object
		getObject(int limit)
		{
			return (getString(limit));
		} // getObject


	} // class SqlVarChar
}  // namespace
