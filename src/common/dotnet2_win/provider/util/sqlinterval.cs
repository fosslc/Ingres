/*
** Copyright (c) 2006, 2007 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections.Specialized;
using Ingres.ProviderInternals;

namespace Ingres.Utility
{
	/*
	** Name: sqlinterval.cs
	**
	** Description:
	**	Defines class which represents an ANSI Interval value.
	**
	**  Classes:
	**
	**	SqlInterval		An ANSI Interval value.
	**
	** History:
	**	25-Oct-06 (thoda04)
	**	    Created.
	**	17-Jan-07 (thoda04)
	**	    Accept interval form of "y-mm".
	**	21-Mar-07 (thoda04)
	**	    Added setTimeSpan override method to support IntervalDayToSecond.
	*/



	/*
	** Name: SqlInterval
	**
	** Description:
	**	Class which represents an ANSI Interval value.
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
	**
	**	getString		Data value accessor retrieval methods
	**	getTimeSpan
	**	getObject
	**
	**  Private Data:
	**
	**	value			Interval value.
	**
	**  Private Methods:
	**
	**	set			setXXX() helper method.
	**	get			getXXX() helper method.
	**
	** History:
	**	25-Oct-06 (thoda04)
	**	    Created.
	*/

	internal class
	SqlInterval : SqlData
	{

		private String value = null;
		private short dbms_type = DBMS_TYPE_INTYM;


		/*
		** Name: SqlInterval
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
		**	25-Oct-06 (thoda04)
		**	    Created.
		*/

		public
		SqlInterval(short dbms_type)
			: base(true)
		{
			switch (dbms_type)
			{
				case DBMS_TYPE_INTYM:
				case DBMS_TYPE_INTDS:
					this.dbms_type = dbms_type;
					break;

				default:	/* Should not happen! */
					throw SqlEx.get(ERR_GC401B_INVALID_DATE);
			}
		} // SqlInterval


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
		**	25-Oct-06 (thoda04)
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
		**	25-Oct-06 (thoda04)
		**	    Created.
		*/

		public void
		set(SqlInterval data)
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
		**	25-Oct-06 (thoda04)
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
		**	25-Oct-06 (thoda04)
		**	    Created.
		*/

		public override void
		setString(String value)
		{
			if (value == null)
				setNull();
			else
			{
				set(value);
			}

			return;
		} // setString


		public override void
		setTimeSpan(TimeSpan value)
		{
			setString(SqlInterval.ToStringIngresDayToSecondFormat(value));
		}


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
		**	25-Oct-06 (thoda04)
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
		**	25-Oct-06 (thoda04)
		**	    Created.
		*/

		public override String
		getString()
		{
			return value;
		} // getString


		/*
		** Name: getTimeSpan
		**
		** Description:
		**	Return the current data value as a TimeSpan value.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	TimeSpan	Current value converted to TimeSpan.
		**
		** History:
		**	25-Oct-06 (thoda04)
		**	    Created.
		*/
		public override TimeSpan
		getTimeSpan()
		{
			TimeSpan span = new TimeSpan(0);
			getTryTimeSpan(value, out span);
			return span;
		}

		/*
		** Name: getTryTimeSpan
		**
		** Description:
		**	Try to return the given string as a TimeSpan value.
		**
		** Input:
		**	String.  May be something like "1-06" or "1 yrs 6 mon"
		 *	         or "123 days 4 hrs" or "123.04:00:00".
		**
		** Output:
		**	TimeSpan	Current value converted to TimeSpan.
		**
		** Returns:
		**	true if string fit .NET or Ingres interval pattern
		**	false if string null, empty, or unrecognizable
		**
		** History:
		**	25-Oct-06 (thoda04)
		**	    Created.
		**	17-Jan-07 (thoda04)
		**	    Accept interval form of "y-mm".
		*/
		public static bool
		getTryTimeSpan(string value, out TimeSpan spanout)
		{
			TimeSpan span;
			int num = 0;
			long ticks = 0;
			long multiplier = 0;
			long multiplier2 = 0;
			bool negate = false;
			bool hitKey = false;
			spanout = new TimeSpan(0);

			if (value == null  || value.Length == 0)
				return false;

			if (TimeSpan.TryParse(value, out span))
			{
				spanout = span;
				return true;
			}

			string unsigned_value;
			if (value.StartsWith("+"))
			{
				unsigned_value = value.Substring(1);
			}
			else if (value.StartsWith("-"))
			{
				unsigned_value = value.Substring(1);
				negate = true;
			}
			else
				unsigned_value = value;


			StringCollection tokens = ScanDateTime(unsigned_value);

			// if "y-m" format
			if (tokens.Count == 3 && tokens[1] == "-")
			{
				if (MetaData.IsNumber(tokens[0]) == false  |
					MetaData.IsNumber(tokens[2]) == false)
					return false;
				num = Int32.Parse(tokens[0]);
				ticks  =  num * TimeSpan.TicksPerDay * 365;          // year
				num = Int32.Parse(tokens[2]);
				ticks += (num * TimeSpan.TicksPerDay * 365) / 12;  // month

				if (negate)
					ticks = -ticks;

				spanout = new TimeSpan(ticks);
				return true;
			}

			num         = 0;
			multiplier  = 0;
			multiplier2 = 0;

			foreach (string ss in tokens)
			{
				if (ss.Contains(":"))   // hh:mm:ss.mmmmmm item
				{
					ticks += (num * TimeSpan.TicksPerDay);
					ticks += TimeSpan.Parse(ss).Ticks;
					num = 0;
					hitKey = true;  // all is well
					continue;
				}

				if (Char.IsDigit(ss[0]))
				{
					num = Int32.Parse(ss);
					continue;
				}

				multiplier = 0;

				string key  = ss.ToLowerInvariant();
				string key1 = ss.Substring(0, 1);
				string key3 = (key.Length > 3) ? key.Substring(0, 3) : key;

				if (key1 == "y")
				{
					multiplier = TimeSpan.TicksPerDay * 365;        // year
					multiplier2= (TimeSpan.TicksPerDay * 365) / 12; // month
				}
				else
				if (key3 == "mon"  ||
					key3 == "mnt")
				{
					multiplier = (TimeSpan.TicksPerDay * 365)/12;  // month
					multiplier2= (TimeSpan.TicksPerDay);           // day
				}
				else
				if (key1 == "d")
				{
					multiplier = TimeSpan.TicksPerDay;  // day
					multiplier2= TimeSpan.TicksPerHour;  // hour
				}
				else
				if (key1 == "h")
				{
					multiplier = TimeSpan.TicksPerHour;    // day
					multiplier2= TimeSpan.TicksPerMinute;  // minute
				}
				else
				if (key3 == "min")
				{
					multiplier = TimeSpan.TicksPerMinute;  // minute
					multiplier2= TimeSpan.TicksPerSecond;  // second
				}
				else
				if (key1 == "s")
				{
					multiplier = TimeSpan.TicksPerSecond;  // second
					multiplier2= 0;
				}

				if (multiplier > 0)
				{
					ticks += (multiplier * num);
					multiplier = 0;
					num = 0;
					hitKey = true;
				}
				continue;
			}  // end foreach (string ss in tokens)

			if (num != 0)  // if last number missing its units
				ticks += (multiplier2 * num);

			if (!hitKey)   // if no Ingres keywords hit, return false
				return false;

			if (negate)
				ticks = -ticks;

			spanout = new TimeSpan(ticks);
			return true;
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
		**	25-Oct-06 (thoda04)
		**	    Created.
		*/

		public override Object
		getObject()
		{
			return (getString());
		} // getObject


		/*
		** Name: ScanDateTime
		**
		** Description:
		**	Scan the string and tokenize it.
		**
		** Input:
		**	String with identifiers and numbers.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	A collection list of strings.
		**
		** History:
		**	17-Jan-07 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Scan the date/time/interval string and tokenize it.
		/// </summary>
		/// <param name="dateTimeString"></param>
		/// <returns>A StringCollection of token strings.</returns>
		static public StringCollection ScanDateTime(string dateTimeString)
		{
			StringCollection list = new StringCollection();
			System.Text.StringBuilder sbToken = 
				new System.Text.StringBuilder(15);
			int i = 0;

			// loop thru chars and build tokens
			while (i < dateTimeString.Length)
			{
				if (Char.IsWhiteSpace(dateTimeString[i]))
				{
					i++;
					continue;  // skip over whitespace
				}

				if (Char.IsNumber(dateTimeString[i]))
				{
					char decimal_point = ':';
					while (i < dateTimeString.Length &&
						(Char.IsNumber(dateTimeString[i]) ||
						 dateTimeString[i] == ':'         ||
						 dateTimeString[i] == decimal_point))
					{
						if (dateTimeString[i] == ':')  // if nn:nn:nn form
							decimal_point = '.';       // enable decimal_pt
						sbToken.Append(dateTimeString[i++]);
					}
				}

				else if (Char.IsLetter(dateTimeString[i]))
				{
					while (i < dateTimeString.Length &&
						(Char.IsLetter(dateTimeString[i])))
					{
						sbToken.Append(dateTimeString[i++]);
					}
				}

				else  // not number, not identifier, must be hyphen
				{
					sbToken.Append(dateTimeString[i++]);
				}

				if (sbToken.Length > 0)
					list.Add(sbToken.ToString());
				sbToken.Length = 0;
				continue;   // loop to next char for next token
			}  // end while (i < dateTimeString.Length) loop thru chars

			return list;
		}

		/// <summary>
		/// Convert the .NET TimeSpan to an Ingres string format
		/// of IntervalDayToSecond.
		/// </summary>
		/// <param name="span"></param>
		/// <returns></returns>
		static public String ToStringIngresDayToSecondFormat(TimeSpan span)
		{
			String s = span.ToString();

			// find the period after days and before the "hh:mm:ss.ffffff"
			// and replace it with a space
			int i = s.IndexOf(':');  // stay away from the period in the fractional
			if (i == -1)             // return if no time.  Should not happen.
				return s;
			int j = s.IndexOf('.');  // j has index of period just after days
			if (j == -1  || j > i)   // return if no days (j==-1 or period beyond ":"
				return "0 " + s;     // return "0 hh:mm:ss.ffffff"

			return s.Substring(0, j) + " " + s.Substring(j+1);
		}

	} // class SqlDate

}  // namespace
