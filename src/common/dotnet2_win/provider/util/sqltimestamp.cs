/*
** Copyright (c) 2006, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;

namespace Ingres.Utility
{
	/*
	** Name: sqltimestamp.cs
	**
	** Description:
	**	Defines class which represents an ANSI Timestamp value.
	**
	**  Classes:
	**
	**	SqlTimestamp	An ANSI Timestamp value.
	**
	** History:
	**	19-Jun-06 (gordy)
	**	    Created.
	**	22-Sep-06 (gordy)
	**	    WITH TZ variant is now in 'local' time rather than
	**	    UTC time.  Added external access to nano-seconds.
	**	01-apr-10 (thoda04)
	**	    If DateTime passed with DateTimeKind.Utc,
	**	    normalize to DateTimeKind.Local.
	*/



	/*
	** Name: SqlTimestamp
	**
	** Description:
	**	Class which represents an ANSI Timestamp value.  Three 
	**	variants are supported:
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
	**	    formatting/parsing.  An external TZ or the local
	**	    default TZ is used to determine the timezone offset.
	**
	**	Fractional seconds are supported.  Due to the how fractional
	**	seconds are supported in java.util.Date, java.sql.Date, and 
	**	the date formatters, fractional seconds are stored as a 
	**	separate nano-second value.
	**
	**	Supports conversion to String, Date, Time, Timestamp, and Object.  
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
	**	setTime
	**	setTimestamp
	**
	**	getString		Data value accessor retrieval methods
	**	getDate
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
	SqlTimestamp
		: SqlData
	{

		private String value = null;
		private int nanos = 0;
		private String timezone = null;
		private short dbms_type = DBMS_TYPE_TS;


		/*
		** Name: SqlTimestamp
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
		SqlTimestamp(short dbms_type)
			: base(true)
		{
			switch (dbms_type)
			{
				case DBMS_TYPE_TS:
				case DBMS_TYPE_TSWO:
				case DBMS_TYPE_TSTZ:
					this.dbms_type = dbms_type;
					break;

				default:	/* Should not happen! */
					throw SqlEx.get(ERR_GC401B_INVALID_DATE);
			}
		} // SqlTimestamp


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
				if (dbms_type == DBMS_TYPE_TSTZ)
				{
					if (value.Length <
					 (SqlDates.TS_FMT.Length + SqlDates.TZ_FMT.Length))
						throw SqlEx.get(ERR_GC401B_INVALID_DATE);

					int offset = value.Length - SqlDates.TZ_FMT.Length;
					timezone = value.Substring(offset);
					value = value.Substring(0, offset);
				}

				/*
				** Separate fractional seconds.
				*/
				if (value.Length > SqlDates.TS_FMT.Length)
				{
					nanos = SqlDates.parseFrac(
							value.Substring(SqlDates.TS_FMT.Length));
					value = value.Substring(0, SqlDates.TS_FMT.Length);
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
		set(SqlTimestamp data)
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
			if (dbms_type == DBMS_TYPE_TSTZ) value += timezone;

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
				DateTime ts;

				/*
				** Validate format by converting to JDBC Timestamp.
				*/
				try { ts = DateTime.Parse(value); }
				catch (Exception  ex )
				{ throw SqlEx.get(ERR_GC401A_CONVERSION_ERR, ex); }

				setTimestamp(ts, null);
			}

			return;
		} // setString


		/*
		** Name: setDate
		**
		** Description:
		**	Assign a Date value as the data value.
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
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public override void
		setDate(DateTime date, TimeZone tz)
		{
			set(date, tz);
			return;
		} // setDate


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
		**	time	Time value (may be null).
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
		setTime(DateTime time, TimeZone tz)
		{
			set(time, tz);
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
		**	19-Jun-06 (gordy)
		**	    Created.
		**	24-Nov-09 (thoda04)  Bug 122955
		**	    Implemented the return of the nanoseconds.
		*/

		public override void
		setTimestamp(DateTime ts, TimeZone tz)
		{
			set(ts, tz);
			nanos = SqlDates.getNanos(ts);
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
		**	Date, Time or Timestamp object.  Since java Dates
		**	only support milli-seconds, the associated nano-
		**	second value is set to zero.
		**
		**	A relative timezone may be provided which is 
		**	applied to non-UTC time values to adjust the 
		**	input such that it represents the original 
		**	date/time value in the timezone provided.
		**
		** Input:
		**	value	Java Date value (may be null).
		**	tz	Relative timezone (null for local).
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		**	22-Sep-06 (gordy)
		**	    WITH TIME ZONE format now uses the local time value.
		*/

		private void
		set(DateTime value, TimeZone tz)
		{
			//DateTime does not have a null state
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
				case DBMS_TYPE_TS:
					/*
					** DAS parses local time using GMT.
					*/
					this.value = SqlDates.formatTimestamp(value, true);
					break;

				case DBMS_TYPE_TSWO:
					/*
					** Format as local time using requested or default timezone.
					*/
					this.value = (tz != null) ? SqlDates.formatTimestamp(value, tz)
								  : SqlDates.formatTimestamp(value, false);
					break;

				case DBMS_TYPE_TSTZ:
					/*
					** Format as local time using requested or default timezone.
					*/
					this.value = (tz != null) ? SqlDates.formatTimestamp(value, tz)
								  : SqlDates.formatTimestamp(value, false);

					/*
					** Get the TZ offset of the target value in the requested
					** or default timezone.
					*/
					timezone = (tz != null) ? SqlDates.formatTZ(tz, value)
								: SqlDates.formatTZ(value);
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
			** Do conversion to validate format.
			** Format using local default TZ for local time.
			** Nano-seconds must be manually formatted.
			*/
			String str = SqlDates.formatTimestamp(get(null), false);
			if (nanos > 0) str += SqlDates.formatFrac(nanos);

			return (str);
		} // getString


		/*
		** Name: getDate
		**
		** Description:
		**	Return the current data value as a Date value.
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
		**	Date	Current value converted to Date.
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public override DateTime
		getDate(TimeZone tz)
		{
			/*
			** Strip time component by re-formatting as date value.
			*/
			return (SqlDates.parseDate(SqlDates.formatDate(get(tz), false), false));
		} // getDate


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
			/*
			** Strip date component by re-formatting as time value.
			*/
			return (SqlDates.parseTime(SqlDates.formatTime(get(tz), false), false));
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
			return (get(tz));
		} // getTimestamp


		/*
		** Name: getObject
		**
		** Description:
		**	Return the current data value as a Timestamp object.
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
		**	Object	Current value converted to Timestamp.
		**
		** History:
		**	19-Jun-06 (gordy)
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

		private DateTime
		get(TimeZone tz)
		{
			DateTime ts;

			switch (dbms_type)
			{
				case DBMS_TYPE_TS:
					/*
					** DAS formats local time using GMT.
					*/
					ts = SqlDates.parseTimestamp(value, true);
					break;

				case DBMS_TYPE_TSWO:
					/*
					** Interpret as local time in requested or default timezone.
					*/
					ts = (tz != null) ? SqlDates.parseTimestamp(value, tz)
							  : SqlDates.parseTimestamp(value, false);
					break;

				case DBMS_TYPE_TSTZ:
					/*
					** Apply explicit timezone.
					*/
					ts = SqlDates.parseTimestamp(value, SqlDates.getTZ(timezone));
					break;

				default:  // should never happen since constructor checked
					throw SqlEx.get(ERR_GC401B_INVALID_DATE);
			}

//			ts.setNanos(nanos);
			if (nanos > 0)
			{
				TimeSpan span = new TimeSpan(nanos / 100L);  // one tick = 100 nanos
				ts += span;   // add the nanos back in
			}
			return (ts);
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


	} // class SqlTimestamp

}  // namespace
