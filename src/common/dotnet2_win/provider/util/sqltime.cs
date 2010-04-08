/*
** Copyright (c) 2006, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;

namespace Ingres.Utility
{
	/*
	** Name: sqltime.cs
	**
	** Description:
	**	Defines class which represents an ANSI Time value.
	**
	**  Classes:
	**
	**	SqlTime		An ANSI Time value.
	**
	** History:
	**	19-Jun-06 (gordy)
	**	    Created.
	**	22-Sep-06 (gordy)
	**	    WITH TZ variant is now in 'local' time rather than UTC time.
	**	30-Oct-06 (thoda04)
	**	    Added fractional seconds support in.
	**	21-Mar-07 (thoda04)
	**	    Added IntervalDayToSecond, IntervalYearToMonth.
	**	01-apr-10 (thoda04)
	**	    Fix the algorithms for timezone specification.
	*/


	/*
	** Name: SqlTime
	**
	** Description:
	**	Class which represents an ANSI Time value.  Three variants
	**	are supported:
	**
	**	WITHOUT TIME ZONE
	**	    Timezone independent.  Since local storage is UTC, an
	**	    external TZ or the local default TZ is used for 
	**	    formatting/parsing.
	**
	**	LOCAL TIME ZONE
	**	    Represents local time stored in UTC.  GMT is used for
	**	    formatting/parsing.
	**
	**	WITH TIME ZONE
	**	    UTC with explicit timezone offset.  GMT is used for
	**	    formatting/parsing.  An explicit TZ or the local 
	**	    default TZ is used to determine the timezone offset.
	**
	**	Fractional seconds are supported since they are not supported 
	**	by the .NET DateTime class.
	**
	**	Supports conversion to String, Time, Timestamp, and Object.  
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
	**	setTime
	**	setTimestamp
	**
	**	getString		Data value accessor retrieval methods
	**	getTime
	**	getTimestamp
	**	getObject
	**
	**	getNanos		Get franctional seconds (nano-seconds).
	**
	**  Private Data:
	**
	**	value			Date value.
	**	nanos			Fractional seconds expressed in nano-seconds.
	**	timezone		Associated timezone.
	**	dbms_type		DBMS specific variat.
	**	use_gmt			Use GMT for DBMS date/time values.
	**
	**
	**  Private Methods:
	**
	**	set			setXXX() helper method.
	**	get			getXXX() helper method.
	**
	** History:
	**	19-Jun-06 (gordy)
	**	    Created.
	*/

	internal class
	SqlTime
		: SqlData
	{

		private String value = null;
		private int nanos = 0;
		private String timezone = null;
		private short dbms_type = DBMS_TYPE_TIME;


		/*
		** Name: SqlTime
		**
		** Description:
		**	Class constructor.  Data value is initially NULL.
		**
		** Input:
		**	dbms_type	DBMS specific variant.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		**	22-Sep-06 (gordy)
		**	    Validate DBMS type.
		*/

		public
		SqlTime(short dbms_type)
			: base(true)
		{
			switch (dbms_type)
			{
				case DBMS_TYPE_TIME:
				case DBMS_TYPE_TMWO:
				case DBMS_TYPE_TMTZ:
					this.dbms_type = dbms_type;
					break;

				default:	/* Should not happen! */
					throw SqlEx.get(ERR_GC401B_INVALID_DATE);
			}
		} // SqlTime


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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		set(String value)
		{
			if (value == null)
				setNull();
			else
			{
				/*
				** Separate explicit timezone.
				*/
				if (dbms_type == DBMS_TYPE_TMTZ)
				{
					if (value.Length <
					 (SqlDates.T_FMT.Length + SqlDates.TZ_FMT.Length))
						throw SqlEx.get(ERR_GC401B_INVALID_DATE);

					int offset = value.Length - SqlDates.TZ_FMT.Length;
					timezone = value.Substring(offset);
					value = value.Substring(0, offset);
				}

				/*
				** Separate fractional seconds.
				*/
				if (value.Length > SqlDates.T_FMT.Length)
				{
					nanos = SqlDates.parseFrac(
							value.Substring(SqlDates.T_FMT.Length));
					value = value.Substring(0, SqlDates.T_FMT.Length);
				}

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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public void
		set(SqlTime data)
		{
			if (data == null || data.isNull())
				setNull();
			else
			{
				setNotNull();
				value = data.value;
				nanos = data.nanos;
				timezone = data.timezone;
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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public String
		get()
		{
			String value = this.value;

			if (nanos > 0) value += SqlDates.formatFrac(nanos);
			if (dbms_type == DBMS_TYPE_TMTZ) value += timezone;

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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public override void
		setString(String value)
		{
			if (value == null)
				setNull();
			else
			{
				DateTime time;

				/*
				** Validate format by converting to JDBC Time.
				*/
				try { time = DateTime.Parse(value); }
				catch (Exception ex)
				{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR, ex); }

				setTime(time, null);
			}

			return;
		} // setString


		/*
		** Name: setTime
		**
		** Description:
		**	Assign a Time value as the data value.
		**
		**	The data value will be NULL if the input value 
		**	is null, otherwise non-NULL.
		**
		**	A relative timezone may be provided which is 
		**	applied to non-UTC time values to adjust the 
		**	input such that it represents the original 
		**	date/time value in the timezone provided.
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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public override void
		setTime(DateTime value, TimeZone tz)
		{
			set(value, tz);
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
		**	A relative timezone may be provided which is 
		**	applied to non-UTC time values to adjust the 
		**	input such that it represents the original 
		**	date/time value in the timezone provided.
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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public override void
		setTimestamp(DateTime value, TimeZone tz)
		{
			set(value, tz);
			return;
		} // setTimestamp


		/*
		** Name: set
		**
		** Description:
		**	Assign a generic DateTime value as the data value.
		**
		**	The data value will be NULL if the input value 
		**	is null, otherwise non-NULL.
		**
		**	Functionally, only the raw offset of the Date value
		**	is used, so the actual source value may be a JDBC
		**	Time or Timestamp object.
		**
		**	A relative timezone may be provided which is 
		**	applied to non-UTC time values to adjust the 
		**	input such that it represents the original 
		**	date/time value in the timezone provided.
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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		private void
		set(DateTime value, TimeZone tz)
		{
			// DateTime does not have a null
			//if (value == null)
			//{
			//    setNull();
			//    return;
			//}

			setNotNull();
			nanos = 0;

			if (value.Kind == DateTimeKind.Utc)  // if UTC, normalize to LOCAL
				value = value.ToLocalTime();

			switch (dbms_type)
			{
				case DBMS_TYPE_TIME:
					/*
					** DAS parses local time using GMT.
					*/
					this.value = SqlDates.formatTime(value, true);
					break;

				case DBMS_TYPE_TMWO:
					/*
					** Format as local time using requested or default timezone.
					*/
					this.value = (tz != null) ? SqlDates.formatTime(value, tz)
								  : SqlDates.formatTime(value, false);
					break;

				case DBMS_TYPE_TMTZ:
					/*
					** Format as local time using requested or default timezone.
					*/
					this.value = (tz != null) ? SqlDates.formatTime(value, tz)
								  : SqlDates.formatTime(value, false);

					/*
					** Java applies TZ and DST of 'epoch' date: 1970-01-01.
					** Ingres applies TZ and DST of todays date, so use 
					** current date to determine explicit TZ offset.
					*/
					long millis = DateTime.Now.Ticks / TimeSpan.TicksPerMillisecond;
					timezone = (tz != null) ? SqlDates.formatTZ(tz, millis)
								: SqlDates.formatTZ(millis);
					break;
			}

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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public override String
		getString()
		{
			/*
			** Format using local default TZ for local time.
			** Nano-seconds must be manually formatted.
			*/
			String str = SqlDates.formatTime(get(null), false);
			if (nanos > 0) str += SqlDates.formatFrac(nanos);

			return (str);
		} // getString


		/*
		** Name: getTime
		**
		** Description:
		**	Return the current data value as a Time value.
		**
		**	A relative timezone may be provided which is
		**	applied to non-UTC time values to adjust the 
		**	final result such that it represents the 
		**	original date/time value in the timezone 
		**	provided.  The default is to use the local 
		**	timezone.
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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public override DateTime
		getTime(TimeZone tz)
		{
			return (get(tz));
		} // getTime


		/*
		** Name: getTimestamp
		**
		** Description:
		**	Return the current data value as a Timestamp value.
		**
		**	A relative timezone may be provided which is
		**	applied to non-UTC time values to adjust the 
		**	final result such that it represents the 
		**	original date/time value in the timezone 
		**	provided.  The default is to use the local 
		**	timezone.
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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public override DateTime
		getTimestamp(TimeZone tz)
		{
			/*
			** Return a Timestamp object with resulting UTC value.
			*/
			return (get(tz));
		} // getTimestamp


		/*
		** Name: getObject
		**
		** Description:
		**	Return the current data value as a Time object.
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
		**	Object	Current value converted to Time.
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public override Object
		getObject()
		{
			return getString();
		} // getObject


		/*
		** Name: get
		**
		** Description:
		**	Return the current data value as a Time value.
		**
		**	A relative timezone may be provided which is
		**	applied to non-UTC time values to adjust the 
		**	final result such that it represents the 
		**	original date/time value in the timezone 
		**	provided.  The default is to use the local 
		**	timezone.
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
		**	19-Jun-06 (gordy)
		**	    Created.
		**	22-Sep-06 (gordy)
		**	    WITH TIME ZONE format now uses the local time value.
		**	    Handle DST and day boundaries with UTC time.
		*/

		private DateTime
		get(TimeZone tz)
		{
			DateTime time;

			switch (dbms_type)
			{
				case DBMS_TYPE_TIME:
					time = SqlDates.parseTime(value, true);
					time = time.ToLocalTime();
					break;

				case DBMS_TYPE_TMWO:
					/*
					** Interpret as local time using requested or default timezone.
					*/
					time = (tz != null) ? SqlDates.parseTime(value, tz)
							: SqlDates.parseTime(value, false);
					break;

				case DBMS_TYPE_TMTZ:
					/*
					** TIME WITH TIMEZONE values are local with
					** explicit timezone offset.
					*/
					time = SqlDates.parseTime(value, SqlDates.getTZ(timezone));
					time = time.ToLocalTime();
					break;

				default:  // should never happen since constructor checked
					throw SqlEx.get(ERR_GC401B_INVALID_DATE);
			}  // end switch

			if (nanos > 0)
			{
				TimeSpan span = new TimeSpan(nanos / 100L);  // one tick = 100 nanos
				time += span;   // add the nanos back in
			}

			return (time);
		} // get


		/*
		** Name: getNanos
		**
		** Description:
		**	Get the timestamps fractional seconds (nano-second) component.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Return:
		**	int	Nano-seconds.
		**
		** History:
		**	22-Sep-06 (gordy)
		**	    Created.
		*/

		public int
		getNanos()
		{
			return (nanos);
		} // getNanos()


	} // class SqlTime

}  // namespace
