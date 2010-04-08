/*
** Copyright (c) 2003, 2008 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.IO;
using System.Text;

namespace Ingres.Utility
{
	/*
	** Name: sqlDecimal.cs
	**
	** Description:
	**	Defines class which represents an SQL Decimal value.
	**
	**  Classes:
	**
	**	SqlDecimal	An SQL Decimal value.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	15-Aug-06 (thoda04) B116461
	**	    Parse decimal string (with a possible decimal-point character)
	**	    with InvariantCulture, regardless of CurrentCulture.
	**	 1-Dec-03 (gordy)
	**	    Added support for parameter types/values in addition to 
	**	    existing support for columns.
	**	15-Sep-08 (gordy, ported by thoda04)
	**	    Removed maxPrecision constant since 
	**	    DBMS determines max decimal precision.
	*/



	/*
	** Name: SqlDecimal
	**
	** Description:
	**	Class which represents an SQL Decimal value.  
	**
	**	Supports conversion to boolean, byte, short, integer, long, 
	**	float, double, String and Object.
	**
	**	The data value accessor methods do not check for the NULL
	**	condition.  The super-class isNull() method should first
	**	be checked prior to calling any of the accessor methods.
	**
	**	Decimal values are truncatable.  A truncated decimal value
	**	results in data value of 0.0.
	**
	**  Public Methods:
	**
	**	set			Assign a new data value.
	**	get			Retrieve current data value.
	**	isTruncated		Data value is truncated?
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
	**	value			The decimal value.
	**	truncated		Decimal value is truncted.
	**
	** History:
	**	 5-Sep-03 (gordy)
	**	    Created.
	**	 1-Dec-03 (gordy)
	**	    Added parameter data value oriented methods.
	**	 4-Aug-05 (gordy)
	**	    Added maxPrecision constant.
	**	15-Sep-08 (gordy, ported by thoda04)
	**	    Removed maxPrecision constant since 
	**	    DBMS determines max decimal precision.
	*/

	internal class
		SqlDecimal : SqlData
		{

			private Decimal		value = Decimal.Zero;
			private bool   		truncated = false;


		/*
		** Name: SqlDecimal
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
			SqlDecimal() : base( true )
		{
		} // SqlDecimal


		/*
		** Name: set
		**
		** Description:
		**	Assign a new data value.  If the input is NULL,
		**	a NULL data value results.
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
			set( String value )
		{
			if ( value == null )
			{
				setNull();
				truncated = false;
			}
			else  if ( value.Length > 0 )
			{
				this.value = DecimalParseInvariant(value);
				setNotNull();
				truncated = false;
			}
			else
			{
				this.value = new Decimal( 0.0 );
				setNotNull();
				truncated = true;
			}
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
		set(SqlDecimal data)
		{
			if (data == null || data.isNull())
			{
				setNull();
				truncated = false;
			}
			else
			{
				setNotNull();
				value = data.value;
				truncated = data.truncated;
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
		**	String	Current value.
		**
		** History:
		**	 1-Dec-03 (gordy)
		**	    Created.
		*/

		public String
		get()
		{
			return (value.ToString());
		} // get


		/*
		** Name: isTruncated
		**
		** Description:
		**	Returns an indication that the data value was truncated.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean		True if truncated.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public override bool
			isTruncated()
		{
			return( truncated );
		} // isTruncated


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
			truncated = false;
			this.value = value ? 1 : 0;
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
			truncated = false;
			this.value = Convert.ToDecimal(value);
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
			truncated = false;
			this.value = Convert.ToDecimal(value);
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
			truncated = false;
			this.value = Convert.ToDecimal(value);
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
			truncated = false;
			this.value = Convert.ToDecimal(value);
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
		**	 4-Aug-05 (gordy)
		**	    Adjust precision to valid range.
		*/

		public override void
		setFloat(float value)
		{
			Decimal temp;

			try { temp = Convert.ToDecimal(value); }
			catch (FormatException /* ex */)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

//			this.value = checkPrecision(temp);
			this.value = temp;
			setNotNull();
			truncated = false;
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
		**	 4-Aug-05 (gordy)
		**	    Adjust precision to valid range.
		*/

		public override void
		setDouble(double value)
		{
			Decimal temp;

			try { temp = Convert.ToDecimal(value); }
			catch (FormatException /* ex */)
			{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

//			this.value = checkPrecision(temp);
			this.value = temp;
			setNotNull();
			truncated = false;
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
		**	 4-Aug-05 (gordy)
		**	    Adjust precision to valid range.
		*/

		public override void
		setDecimal(Decimal value)
		{
			// Decimal is never null
			//if (value == null)
			//    setNull();
			//else
			{
//				this.value = checkPrecision(value);
				this.value = value;
				setNotNull();
			}

			truncated = false;
			return;
		} // setBigDecimal


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
		**	 4-Aug-05 (gordy)
		**	    Adjust precision to valid range.
		*/

		public override void
		setString(String value)
		{
			if (value == null)
				setNull();
			else
			{
				Decimal temp;

				try
				{
					temp = DecimalParseInvariant(value);
				}
				catch (FormatException /* ex */)
				{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

//				this.value = checkPrecision(temp);
				this.value = temp;
				setNotNull();
			}

			truncated = false;
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
			return( (value == 0) ? false : true );
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
			return( Decimal.ToByte(value) );
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
			return( Decimal.ToInt16( value ));
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
			return( Decimal.ToInt32( value ));
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
			return( Decimal.ToInt64( value ) );
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
			return( Decimal.ToSingle( value ) );
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
			return( Decimal.ToDouble( value ) );
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
		*/

		public override Decimal 
			getDecimal() 
		{
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
		**	Return the current data value as a Decimal object.
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
		**	Object	Current value converted to Decimal.
		**
		** History:
		**	 5-Sep-03 (gordy)
		**	    Created.
		*/

		public override Object 
			getObject() 
		{
			return( getDecimal() );
		} // getObject


	} // class SqlDecimal
}  // namespace
