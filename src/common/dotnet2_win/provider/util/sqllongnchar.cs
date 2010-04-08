/*
** Copyright (c) 2003, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{


	/*
	** Name: sqllongnchar.cs
	**
	** Description:
	**	Defines class which represents an SQL LongNVarchar value.
	**
	**  Classes:
	**
	**	SqlLongNChar	An SQL LongNVarchar value.
	**
	** History:
	**	12-Sep-03 (gordy)
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
	**	 9-dec-09 (thoda04)
	**	    Use BooleanParse() for BOOLEAN string parsing.
	*/



	/*
	** Name: SqlLongNChar
	**
	** Description:
	**	Class which represents an SQL LongNVarchar value.  SQL LongNVarchar 
	**	values are potentially large variable length streams.
	**
	**	Supports conversion to String, Binary, Object.  
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
	**	get			Retrieve current data value.
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
	**	setCharacterStream
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
	**  Protected Methods:
	**
	**	cnvtIS2Rdr		Convert InputStream to Reader (Stub).
	**
	**  Private Methods:
	**
	**	strm2str		Convert character stream to string.
	**
	** History:
	**	12-Sep-03 (gordy)
	**	    Created.
	**	 1-Dec-03 (gordy)
	**	    Added parameter data value oriented methods.
	*/

	internal class
		SqlLongNChar : SqlStream
	{


		/*
		** Name: SqlLongNChar
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
			SqlLongNChar() : base()
		{
		} // SqlLongNChar


		/*
		** Name: SqlLongNChar
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
			SqlLongNChar( IStreamListener listener ) : base( listener )
		{
		} // SqlLongNChar


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value.  The data value will be NULL if 
		**	the input value is null, otherwise non-NULL.  The input 
		**	stream may optionally implement the SqlStream.IStreamSource
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
			set( Reader stream )
		{
			setStream( (Object)stream );
			return;
		} // set


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value as a copy of a SqlNChar data 
		**	value.  The data value will be NULL if the input value 
		**	is null, otherwise non-NULL.  
		**
		** Input:
		**	data	SqlNChar data value to copy.
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
		set(SqlNChar data)
		{
			if (data.isNull())
				setNull();
			else
			{
				/*
				** The character data is stored in a character array.  
				** A character stream will produce the desired output.  
				** Note that we need to follow the SqlNChar convention 
				** and extend the data to the optional limit.
				*/
				data.extend();
				setStream(getCharacter(data.value, data.length));
			}
			return;
		} // set


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value as a copy of a SqlNVarChar data 
		**	value.  The data value will be NULL if the input value 
		**	is null, otherwise non-NULL.  
		**
		** Input:
		**	data	SqlNVarChar data value to copy.
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
		set(SqlNVarChar data)
		{
			if (data.isNull())
				setNull();
			else
			{
				/*
				** The character data is stored in a character array.  
				** A character stream will produce the desired output.  
				*/
				setStream(getCharacter(data.value, data.length));
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
		**	Reader	Current value.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public virtual Reader
		get()
		{
			return ((Reader)getStream());
		} // get


		/*
		** Name: get
		**
		** Description:
		**	Write the current data value to a Writer stream.
		**	The current data value is consumed.  The Writer
		**	stream is not closed but will be flushed.
		**
		**	Note: the value returned when the data value is 
		**	NULL is not well defined.
		**
		** Input:
		**	wtr	Writer to receive data value.
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

		public virtual void
		get(Writer wtr)
		{
			copyRdr2Wtr((Reader)getStream(), wtr);
			return;
		} // get


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
		setDecimal(Decimal value)
		{
			// Decimal is never null
			//if (value == null)
			//    setNull();
			//else
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
				setStream(getCharacter(value));

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
			// DateTime is never null
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
			// DateTime is never null
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
			// DateTime is never null
			//if (value == null)
			//    setNull();
			//else
			if (tz != null)
				setString(SqlDates.formatTimestamp(value, tz));
			else
				setString(SqlDates.formatTimestamp(value, false));
			return;
		} // setTimestamp


		/*
		** Name: setAsciiStream
		**
		** Description:
		**	Assign a ASCII stream as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		** Input:
		**	value	ASCII stream (may be null).
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
		setAsciiStream(InputStream value)
		{
			if (value == null)
				setNull();
			else
				setStream(cnvtAscii(value));
			return;
		} // setAsciiStream


		/*
		** Name: setUnicodeStream
		**
		** Description:
		**	Assign a Unicode stream as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		** Input:
		**	value	Unicode stream (may be null).
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
		setUnicodeStream(InputStream value)
		{
			if (value == null)
				setNull();
			else
				setStream(cnvtUnicode(value));
			return;
		} // setAsciiStream


		/*
		** Name: setCharacterStream
		**
		** Description:
		**	Assign a character stream as the data value.
		**	The data value will be NULL if the input 
		**	value is null, otherwise non-NULL.
		**
		** Input:
		**	value	Character stream (may be null).
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
		setCharacterStream(Reader value)
		{
			setStream(value);
			return;
		} // setCharacterStream


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
		**	12-Sep-03 (gordy)
		**	    Created.
		**	 9-dec-09 (thoda04)
		**	    Use BooleanParse() for BOOLEAN string parsing.
		*/

		public override bool
		getBoolean() 
		{
			String str = strm2str( getCharacter(), -1 );
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
		**	12-Sep-03 (gordy)
		**	    Created.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override byte 
			getByte() 
		{
			byte value;

			try { value = Byte.Parse(getString()); }
			catch (FormatException)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

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
		**	12-Sep-03 (gordy)
		**	    Created.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override short 
			getShort() 
		{
			short value;

			try { value = Int16.Parse(getString()); }
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
		**	12-Sep-03 (gordy)
		**	    Created.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override int 
			getInt() 
		{
			int value;

			try { value = Int32.Parse(getString()); }
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
		**	12-Sep-03 (gordy)
		**	    Created.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override long 
			getLong() 
		{
			long value;

			try { value = Int64.Parse(getString()); }
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
		**	12-Sep-03 (gordy)
		**	    Created.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override float 
			getFloat() 
		{
			float value;

			try { value = Single.Parse(getString()); }
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
		**	12-Sep-03 (gordy)
		**	    Created.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override double 
			getDouble() 
		{
			double value;

			try { value = Double.Parse(getString()); }
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
		**	Decimal	Current value converted to Decimal.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		**	24-May-06 (gordy)
		**	    Convert parsing exception to SQL conversion exception.
		*/

		public override Decimal 
			getDecimal() 
		{
			Decimal value;

			try { value = DecimalParseInvariant(getString()); }
			catch (FormatException)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

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
		**	String		Current value converted to String.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public override String 
			getString() 
		{
			return( strm2str( getCharacter(), -1 ) );
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
			return( strm2str( getCharacter(), limit ) );
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
		*/

		public override DateTime 
			getDate( TimeZone tz ) 
		{
			String str = getString().Trim();
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
		*/

		public override DateTime
			getTime( TimeZone tz ) 
		{
			String str = getString().Trim();
			return ((tz == null) ? SqlDates.parseTime(str, false)
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
		*/

		public override DateTime 
			getTimestamp( TimeZone tz ) 
		{
			String str = getString().Trim();
			return ((tz == null) ? SqlDates.parseTimestamp(str, false)
				: SqlDates.parseTimestamp( str, tz ) );
		}


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
			return( getString( limit ) );
		} // getObject


		/*
		** Name: cnvtIS2Rdr
		**
		** Description:
		**	Converts the byte string input stream into a Reader stream.
		**
		**	This class uses Reader streams, so implemented as stub.
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
			return( (Reader)null );	// Stub, should not be called.
		} // cnvtIS2Rdr


		/*
		** Name: strm2str
		**
		** Description:
		**	Read a character input stream and convert to a string object.  
		**	The stream is closed.
		**
		** Input:
		**	in	Character input stream.
		**	limit	Maximum size of result, negative for no limit
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	The stream as a string.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		private static String
			strm2str( Reader inReader, int limit )
		{
			char[]           cb = new char[ 8192 ];
			StringBuilder    sb = new StringBuilder();
			int              len;

			try
			{
				while( (limit < 0  ||  sb.Length < limit)  &&
					(len = inReader.read( cb, 0, cb.Length )) >= 0 )
				{
					if ( limit >= 0 )  len = Math.Min( len, limit - sb.Length );
					if ( len > 0 )  sb.Append( cb, 0, len );
				}
			}
			catch( IOException )
			{
				throw SqlEx.get( ERR_GC4007_BLOB_IO );
			}
			finally
			{
				try { inReader.close(); }
				catch( IOException ) {}
			}

			return( sb.ToString() );
		} // strm2str


	} // class SqlLongNChar
}  // namespace
