/*
** Copyright (c) 2003, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{

	/*
	** Name: sqlnvarchar.cs
	**
	** Description:
	**	Defines class which represents an SQL NVarChar value.
	**
	**  Classes:
	**
	**	SqlNVarChar	An SQL NVarChar value.
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
	**	24-May-06 (gordy)
	**	    Convert parsing exception to SQL conversion exception.
	**	21-Mar-07 (thoda04)
	**	    Added setTimeSpan override method to support IntervalDayToSecond.
	**	 9-dec-09 (thoda04)
	**	    Use BooleanParse() for BOOLEAN string parsing.
	*/

	/*
	** Name: SqlNVarChar
	**
	** Description:
	**	Class which represents an SQL NVarChar value.  SQL NVarChar 
	**	values are variable length UCS2 based strings.
	**
	**	Supports conversion to boolean, byte, short, int, long, float, 
	**	double, Decimal, Date, Time, Timestamp, Object and streams.  
	**
	**	This class implements the ICharArray interface as the means
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
	**	condition.  The base class isNull() method should first
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
	**	put			Copy characters into the array.
	**	get			Copy characters out of the array.
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
	**	setDecimal
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
	**	getDecimal
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
	**	    Removed dependency on SqlNChar and implemented SqlData and
	**	    CharArray interfaces to support unlimited length strings.
	**	    Added parameter data value oriented methods.
	*/

	internal class
	SqlNVarChar
		: SqlData, ICharArray
	{

		private static char[]	empty = new char[ 0 ];

		public char[] value        = empty;
		protected int limit  = -1;  // Max (initially unlimited)

		private int _length;
		public virtual int length
		{
			get { return _length; }
			set { _length = value; }
		}


		/*
		** Name: SqlNVarChar
		**
		** Description:
		**	Class constructor for variable length UCS2 strings.
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
		SqlNVarChar() : base( true )
		{
		} // SqlNVarChar


		/*
		** Name: SqlNVarChar
		**
		** Description:
		**	Class constructor for limited length UCS2 strings.  
		**	Data value is initially NULL.
		**
		** Input:
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
		SqlNVarChar( int size ) : this()
		{
			limit = size;
		} // SqlNVarChar


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
		**	    Adapted for SqlNVarChar.
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
		**	    Adapted for SqlNVarChar.
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
		**	    Support unlimited length strings.
		*/

		public virtual void
		valuelimit( int size )
		{
			limit = (size < 0) ? -1 : size;
			if (limit >= 0 && length > limit)  length = limit;
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
		**	Note that valuelimit(n, true) is equivilent to valuelimit(n) followed 
		**	by ensureCapacity(n) and valuelimit(n, false) is the same as 
		**	valuelimit(n).
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
		**	    Adapted for SqlNVarChar.
		*/

		public virtual void
		valuelimit( int size, bool ensure_flag )
		{
			valuelimit( size );
			if (ensure_flag) ensure(size);
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
		**	    Adapted for SqlNVarChar.
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
		**	    Adapted for SqlNVarChar.
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
		set( SqlNVarChar data )
		{
			if ( data == null  ||  data.isNull() )
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
		**	Append a character value to the current array data.  The 
		**	portion of the input which would cause the array to grow 
		**	beyond the (optional) size limit is ignored.  The number 
		**	of characters actually appended is returned.
		**
		** Input:
		**	value	The character to append.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Number of characters appended.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlNVarChar.
		*/

		public int 
		put( char value )
		{
			/*
			** Is there room for the character.
			*/
			if (limit >= 0 && length >= limit) return (0);

			/*
			** Save character and update array length.
			*/
			ensure(length + 1);
			this.value[length++] = value;
			return( 1 );
		}


		/*
		** Name: put
		**
		** Description:
		**	Append a character array to the current array data.  The 
		**	portion of the input which would cause the array to grow 
		**	beyond the (optional) size limit is ignored.  The number 
		**	of characters actually appended is returned.
		**
		** Input:
		**	value	The character array to append.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Number of characters appended.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlNVarChar.
		*/

		public int
		put( char[] value )
		{
			return( put( value, 0, value.Length ) );
		} // put


		/*
		** Name: put
		**
		** Description:
		**	Append a portion of a character array to the current array 
		**	data.  The portion of the input which would cause the array 
		**	to grow beyond the (optional) size limit is ignored.  The 
		**	number of characters actually appended is returned.
		**
		** Input:
		**	value	Array containing characters to be appended.
		**	offset	Start of portion to append.
		**	length	Number of characters to append.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Number of characters appended.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlNVarChar.
		*/

		public int
		put( char[] value, int offset, int length )
		{
			/*
			** Determine how many characters to copy.
			*/
			int unused = (limit < 0) ? length : limit - this.length;
			if ( length > unused )  length = unused;
		    
			/*
			** Copy characters and update array length.
			*/
			ensure(this.length + length);
			System.Array.Copy(value, offset, this.value, this.length, length);
			this.length += length;
			return( length );
		} // put


		/*
		** Name: get
		**
		** Description:
		**	Returns the character at a specific position in the 
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
		**	char		Character or 0.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public virtual char 
		get( int position )
		{
			return ((char)((position >= length) ? 0 : value[position]));
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
		**	char[]	Copy of the current array.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlNVarChar.
		*/

		public virtual char[] 
		get()
		{
			char[] chars = new char[length];
			if (length > 0)
				System.Array.Copy(value, 0, chars, 0, length);
			return( chars );
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Copy characters out of the array.  Copying starts with the first
		**	character of the array.  The number of characters copied is the 
		**	minimum of the current array length and the length of the output 
		**	array.
		**
		** Input:
		**	None.
		**
		** Output:
		**	value	Character array to receive output.
		**
		** Returns:
		**	int	Number of characters copied.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlNVarChar.
		*/

		public virtual int
		get( char[] value )
		{
			return( get( 0, value.Length, value, 0 ) );
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Copy a portion of the array.  Copying starts at the position
		**	indicated.  The number of characters copied is the minimum of 
		**	the length requested and the number of characters in the array 
		**	starting at the requested position.  If position is greater 
		**	than the current length, no characters are copied.  The output 
		**	array must have sufficient space.  The actual number of 
		**	characters copied is returned.
		**
		** Input:
		**	position	Starting character to copy.
		**	length		Number of characters to copy.
		**	offset		Starting position in output array.
		**
		** Output:
		**	value		Character array to recieve output.
		**
		** Returns:
		**	int		Number of characters copied.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlNVarChar.
		*/

		public virtual int
		get( int position, int length, char[] value, int offset )
		{
			/*
			** Determine how many characters to copy.
			*/
			int avail = (position >= this.length) ? 0 : this.length - position;
			if ( length > avail )  length = avail;
		    
			if ( length > 0 )
			Array.Copy( this.value, position, value, offset, length );
			return( length );
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
		ensure( int capacity )
		{
			/*
			** Capacity does not need to exceed (optional) limit.
			*/
			if (limit >= 0 && capacity > limit) capacity = limit;

			if ( capacity > value.Length )
			{
			char[] ca = new char[ capacity ];
			Array.Copy(value, 0, ca, 0, value.Length);
			value = ca;
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
		setBoolean( bool value ) 
		{
			setString( value.ToString() );
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
		setByte( byte value ) 
		{
			setString( value.ToString() );
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
		setShort( short value ) 
		{
			setString( value.ToString() );
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
		setInt( int value ) 
		{
			setString( value.ToString() );
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
		setLong( long value ) 
		{
			setString( value.ToString() );
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
		setFloat( float value ) 
		{
			setString( value.ToString() );
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
		setDouble( double value ) 
		{
			setString( value.ToString() );
			return;
		} // setDouble


		/*
		** Name: setDecimal
		**
		** Description:
		**	Assign a Decimal value as the data value.
		**	The data value will be NULL if the input value
		**	is null, otherwise non-NULL.
		**
		** Input:
		**	value	Decimal value (may be null).
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
		setDecimal( Decimal value ) 
		{
			// Decimal is never null
			//if ( value == null )
			//setNull();
			//else
			setString( value.ToString() );

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
		setString( String value ) 
		{
			if ( value == null )
			setNull();
			else
			{
			/*
			** Apply length limit if applicable.
			*/
			int length = value.Length;
			if (limit >= 0 && length >= limit) length = limit;
			
			/*
			** Now assign the new value.
			*/
			clear();
			ensure( length );
			value.CopyTo( 0, this.value, 0, length );
			this.length = length;
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
		setDate( DateTime value, TimeZone tz ) 
		{
			// DateTime is never null
			//if ( value == null )
			//setNull();
			//else
			if ( tz != null )
				setString( SqlDates.formatDate( value, tz ) );
			else
				setString( SqlDates.formatDate( value, false ) );
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
		setTime( DateTime value, TimeZone tz ) 
		{
			// DateTime is never null
			//if ( value == null )
			//setNull();
			//else
			if ( tz != null )
				setString( SqlDates.formatTime( value, tz ) );
			else
				setString( SqlDates.formatTime( value, false ) );
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
		setTimestamp( DateTime value, TimeZone tz ) 
		{
			// DateTime is never null
			//if ( value == null )
			//setNull();
			//else
			if ( tz != null )
				setString( SqlDates.formatTimestamp( value, tz ) );
			else
				setString( SqlDates.formatTimestamp( value, false ) );
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
		**	    Adapted for SqlNVarChar.
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
		**	    Adapted for SqlNVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override byte 
		getByte() 
		{
			byte value;

			try { value = Byte.Parse(getString(length)); }
			catch( FormatException /* ex */ )
			{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

			return( value );
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
		**	    Adapted for SqlNVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override short 
		getShort() 
		{
			short value;

			try { value = Int16.Parse(getString(length)); }
			catch( FormatException /* ex */ )
			{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

			return( value );
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
		**	    Adapted for SqlNVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override int 
		getInt() 
		{
			int value;

			try { value = Int32.Parse(getString(length)); }
			catch (FormatException /* ex */ )
			{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

			return( value );
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
		**	    Adapted for SqlNVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override long 
		getLong() 
		{
			long value;

			try { value = Int64.Parse(getString(length)); }
			catch (FormatException /* ex */ )
			{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

			return( value );
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
		**	    Adapted for SqlNVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override Single 
		getFloat() 
		{
			float value;

			try { value = Single.Parse(getString(length)); }
			catch (FormatException /* ex */ )
			{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

			return( value );
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
		**	    Adapted for SqlNVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override double 
		getDouble() 
		{
			double value;

			try { value = Double.Parse(getString(length)); }
			catch( FormatException /* ex */ )
			{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

			return( value );
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
		**	Decimal	Current value converted to Decimal.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		**	 1-Dec-03 (gordy)
		**	    Adapted for SqlNVarChar.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override Decimal 
		getDecimal() 
		{
			Decimal value;

			try { value = DecimalParseInvariant(getString(length)); }
			catch (FormatException /* ex */ )
			{ throw SqlEx.get( ERR_GC401A_CONVERSION_ERR ); }

			return( value );
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
		**	    Adapted for SqlNVarChar.
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
		**	    Adapted for SqlNVarChar.
		*/

		public override String 
		getString( int limit ) 
		{
			if (limit > length) limit = length;
			return( new String( value, 0, limit ) );
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
		**	    Adapted for SqlNVarChar.
		*/

		public override DateTime 
		getDate( TimeZone tz ) 
		{
			String str = getString(length).Trim();
			return( (tz == null) ? SqlDates.parseDate( str, false )
					 : SqlDates.parseDate( str, tz ) );
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
		**	    Adapted for SqlNVarChar.
		*/

		public override DateTime 
		getTime( TimeZone tz ) 
		{
			String str = getString(length).Trim();
			return( (tz == null) ? SqlDates.parseTime( str, false )
					 : SqlDates.parseTime( str, tz ) );
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
		**	    Adapted for SqlNVarChar.
		*/

		public override DateTime
		getTimestamp( TimeZone tz ) 
		{
			String str = getString(length).Trim();
			return( (tz == null) ? SqlDates.parseTimestamp( str, false )
					 : SqlDates.parseTimestamp( str, tz ) );
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
		**	    Adapted for SqlNVarChar.
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
		**	    Adapted for SqlNVarChar.
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
		**	    Adapted for SqlNVarChar.
		*/

		public override Reader 
		getCharacterStream() 
		{
			return( getCharacter( value, length ) );
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
		**	    Adapted for SqlNVarChar.
		*/

		public override Object 
		getObject() 
		{
			return( getString() );
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
		**	    Adapted for SqlNVarChar.
		*/

		public override Object 
		getObject( int limit ) 
		{
			return( getString( limit ) );
		} // getObject


	} // class SqlNVarChar
}  // namespace
