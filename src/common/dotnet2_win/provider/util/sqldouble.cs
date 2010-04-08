/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{
	/*
	** Name: sqldouble.cs
	**
	** Description:
	**	Defines class which represents an SQL Double value.
	**
	**  Classes:
	**
	**	SqlDouble	An SQL Double value.
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
	*/



	/*
	** Name: SqlDouble
	**
	** Description:
	**	Class which represents an SQL Double value.  
	**
	**	Supports conversion to boolean, byte, short, integer, long, 
	**	float, BigDecimal, String and Object.
	**
	**	The data value accessor methods do not check for the NULL
	**	condition.  The super-class isNull() method should first
	**	be checked prior to calling any of the accessor methods.
	**
	**  Public Methods:
	**
	**	set			Assign a new data value.
	**	get			Retrieve current data value.
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
	**	getObject
	**
	**  Private Data:
	**
	**	value			The double value.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 1-Dec-03 (gordy)
	**	    Added parameter data value oriented methods.
	*/

	internal class
		SqlDouble : SqlData
	{

		private double		value = 0.0;


		/*
		** Name: SqlDouble
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public
			SqlDouble() : base( true )
		{
		} // SqlDouble


		/*
		** Name: SqlDouble
		**
		** Description:
		**	Class constructor which establishes a non-NULL value.
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
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public
			SqlDouble( double value ) : base( false )
		{
			this.value = value;
		} // SqlDouble


		/*
		** Name: set
		**
		** Description:
		**	Assign a new non-NULL data value.
		**
		** Input:
		**	value		The new data value.
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

		public void
			set( double value )
		{
			setNotNull();
			this.value = value;
			return;
		} // set


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
		set(SqlDouble data)
		{
			if (data == null || data.isNull())
				setNull();
			else
			{
				setNotNull();
				value = data.value;
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
		**	double	Current value.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public double
		get()
		{
			return (value);
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
			setNotNull();
			this.value = value ? 1.0 : 0.0;
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
			setNotNull();
			this.value = Convert.ToDouble(value);
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
			setNotNull();
			this.value = Convert.ToDouble(value);
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
			setNotNull();
			this.value = Convert.ToDouble(value);
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
			setNotNull();
			this.value = Convert.ToDouble(value);
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
			setNotNull();
			this.value = Convert.ToDouble(value);
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
			setNotNull();
			this.value = value;
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
			{
				setNotNull();
				this.value = Convert.ToDouble(value);
			}
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
				setNotNull();

				try { this.value = Double.Parse(value); }
				catch (FormatException /* ex */ )
				{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }
			}
			return;
		} // setString


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
		*/

		public override bool
			getBoolean() 
		{
			return( (value == 0.0) ? false : true );
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
		*/

		public override byte 
			getByte() 
		{
			return (Convert.ToByte(value));
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
		*/

		public override short 
			getShort() 
		{
			return (Convert.ToInt16(value));
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
		*/

		public override int 
			getInt() 
		{
			return (Convert.ToInt32(value));
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
		*/

		public override long 
			getLong() 
		{
			return (Convert.ToInt64(value));
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
		*/

		public override float 
			getFloat() 
		{
			return (Convert.ToSingle(value));
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
		*/

		public override double 
			getDouble() 
		{
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
		**	BigDecimal	Current value converted to BigDecimal.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public override Decimal 
			getDecimal() 
		{
			return( new Decimal( (double)value ) );
		} // getBigDecimal


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
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public override String 
			getString() 
		{
			return( value.ToString() );
		} // getString


		/*
		** Name: getObject
		**
		** Description:
		**	Return the current data value as a Double object.
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
		**	Object	Current value converted to Double.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public override Object 
			getObject() 
		{
			return( value  );
		} // getObject


	} // class SqlDouble
}  // namespace
