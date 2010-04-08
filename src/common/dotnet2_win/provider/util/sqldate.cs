/*
** Copyright (c) 2006 Ingres Corporation. All Rights Reserved.
*/

using System;

namespace Ingres.Utility
{
	/*
	** Name: sqldate.cs
	**
	** Description:
	**	Defines class which represents an ANSI Date value.
	**
	**  Classes:
	**
	**	SqlDate		An ANSI Date value.
	**
	** History:
	**	16-Jun-06 (gordy)
	**	    Created.
	*/



	/*
	** Name: SqlDate
	**
	** Description:
	**	Class which represents an ANSI Date value.
	**
	**	Supports conversion to String, Date, Timestamp, and Object.  
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
	**	setString		Data value accessor assignment methods
	**	setDate
	**	setTimestamp
	**
	**	getString		Data value accessor retrieval methods
	**	getDate
	**	getTimestamp
	**	getObject
	**
	**  Private Data:
	**
	**	value			Date value.
	**
	**  Private Methods:
	**
	**	set			setXXX() helper method.
	**	get			getXXX() helper method.
	**
	** History:
	**	16-Jun-06 (gordy)
	**	    Created.
	*/

	internal class
	SqlDate : SqlData
	{

		private String value = null;


		/*
		** Name: SqlDate
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
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public
		SqlDate()
			: base(true)
		{
		} // SqlDate


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
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		set(String value)
		{
			if (value == null)
				setNull();
			else
			{
				setNotNull();
				this.value = value;
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
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		set(SqlDate data)
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
		**	String	Current value.
		**
		** History:
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public String
		get()
		{
			return (value);
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
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public override void
		setString(String value)
		{
			if (value == null)
				setNull();
			else
			{
				DateTime date;

				/*
				** Validate format by converting to DateTime.
				*/
				try { date = DateTime.Parse(value); }
				catch (Exception /* ignore */)
				{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR); }

				setDate(date, null);
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
		**	date	Date value (may be null).
		**	tz	Relative timezone, NULL for local.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public override void
		setDate(DateTime date, TimeZone tz)
		{
			set(date, tz);
			return;
		} // setDate


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
		**	ts	Timestamp value (may be null).
		**	tz	Relative timezone, NULL for local.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public override void
		setTimestamp(DateTime ts, TimeZone tz)
		{
			set(ts, tz);
			return;
		} // setTimestamp


		/*
		** Name: set
		**
		** Description:
		**	Assign a DateTime value as the data value.
		**
		**	The data value will be NULL if the input value 
		**	is null, otherwise non-NULL.
		**
		**	A relative timezone may be provided which is 
		**	applied to adjust the input such that it 
		**	represents the original date/time value in the 
		**	timezone provided.  The default is to use the 
		**	local timezone.
		**
		** Input:
		**	value	DateTime value (may be null).
		**	tz	Relative timezone, NULL for local.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		private void
		set(DateTime value, TimeZone tz)
		{
			// DateTime structure can never be null
			//if (value == null)
			//{
			//    setNull();
			//    return;
			//}

			/*
			** Dates should be independent of TZ, but JDBC date values
			** are stored in UTC.  Use the TZ provided to ensure the
			** formatted value represents the date in the desired TZ.
			** Otherwise, the local default TZ is used.
			*/
			setNotNull();
			this.value = (tz != null) ? SqlDates.formatDate(value, tz)
						  : SqlDates.formatDate(value, false);

			return;
		} // set


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
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public override String
		getString()
		{
			/*
			** Do conversion to check for valid format.
			*/
			return (SqlDates.formatDate(get(null), false));
		} // getString


		/*
		** Name: getDate
		**
		** Description:
		**	Return the current data value as a Date value.
		**	
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
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public override DateTime
		getDate(TimeZone tz)
		{
			return (get(tz));
		} // getDate


		/*
		** Name: getTimestamp
		**
		** Description:
		**	Return the current data value as a Timestamp value.
		**
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
		**	tz		Relative timezone, NULL for local.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Timestamp	Current value converted to Timestamp.
		**
		** History:
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public override DateTime
		getTimestamp(TimeZone tz)
		{
			/*
			** Return a Timestamp object with the resulting UTC value.
			*/
			return (get(tz));
		} // getTimestamp


		/*
		** Name: getObject
		**
		** Description:
		**	Return the current data value as a Date object.
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
		**	Object	Current value converted to Date.
		**
		** History:
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		public override Object
		getObject()
		{
			return (get(null));
		} // getObject


		/*
		** Name: get
		**
		** Description:
		**	Return the current data value as a Date value.
		**	
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
		**	16-Jun-06 (gordy)
		**	    Created.
		*/

		private DateTime
		get(TimeZone tz)
		{
			/*
			** Dates should be independent of TZ, but JDBC date values
			** are stored in UTC.  Use the TZ provided to ensure the
			** resulting UTC value represents the date in the desired 
			** TZ.  Otherwise, the local default TZ is used.
			*/
			return ((tz != null) ? SqlDates.parseDate(value, tz)
					  : SqlDates.parseDate(value, false));
		} // get


	} // class SqlDate

}  // namespace
