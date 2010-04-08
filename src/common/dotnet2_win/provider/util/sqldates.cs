/*
** Copyright (c) 2003, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using System.IO;
using System.Text;

namespace Ingres.Utility
{
	/*
	** Name: sqldates.cs
	**
	** Description:
	**	Defines class which provides constants and static methods
	**	for processing SQL date/time values.
	**
	**  Classes:
	**
	**	SqlDates
	**
	** History:
	**	12-Sep-03 (gordy)
	**	    Created.
	**	 9-Dec-03 (thoda04)
	**	    Ported to .NET.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	19-Jun-06 (gordy)
	**	    ANSI Date/Time data type support
	**	22-Sep-06 (gordy)
	**	    Added methods for handling ANSI time zone offsets.
	**	02-Nov-09 (thoda04) Bug 122809
	**	    Lock the tzCache and tzoCache collections to avoid
	**	    an enumerator throwing NullReferenceException should
	**	    another thread modify the collection.
	**	02-Nov-09 (thoda04) Bug 122833
	**	    Fix tzCache and tzoCache memory leak.
	**	01-apr-10 (thoda04)
	**	    Fix parse of timezone offset parseOffset() method.
	*/


	/*
	** Name: SqlDates
	**
	** Description:
	**	Utility class which provides constants and static methods
	**	for date/time processing.
	**
	**  Constants:
	**
	**	D_EPOCH			Date epoch value.
	**	T_EPOCH			Time epoch value.
	**	TS_EPOCH		Timestamp epoch value.
	**	D_FMT			Date format template.
	**	T_FMT			Time format template.
	**	TS_FMT			Timestamp format template.
	**	D_LIT_FMT		Date literal format template.
	**	T_LIT_FMT		Time literal format template.
	**	TS_LIT_FMT		Timestamp literal format template.
	**	TZ_FMT			Timezone standard format.
	**
	**  Public Methods:
	**
	**	getEpochDate		Returns epoch Date value.
	**	getEpochTime		Returns epoch Time value.
	**	getEpochTimestamp	Returns epoch Timestamp value.
	**	parseOffset		Parse Timezone offset.
	**	formatOffset		Format Timezone Offset.
	**	getTZ			Returns timezone for UTC offset.
	**	formatTZ		UTC offset of TZ in standard format.
	**	parseFrac		Parse fractional seconds.
	**	formatFrac		Format fractional seconds.
	**	parseTime		Parse SQL time values.
	**	formatTime		Format SQL time values.
	**	formatTimeLit		Format SQL time literals.
	**	parseDate		Parse SQL date values.
	**	formatDate		Format SQL date values.
	**	formatDateLit		Format SQL date literals.
	**	parseTimestamp		Parse SQL timestamp values.
	**	formatTimestamp		Format SQL timestamp values.
	**	formatTimestampLit	Format SQL timestamp literals.
	**
	**  Private Data:
	**
	**	tz_gmt			GMT Timezone.
	**	tz_lcl			Local Timezone.
	**	df_gmt			GMT Date formatter.
	**	df_lcl			Local Date formatter.
	**	df_ts_val		Date formatter for Timestamp values.
	**	df_d_val		Date formatter for Date values.
	**	df_t_val		Date formatter for Time values.
	**	df_ts_lit		Date formatter for Timestamp literals.
	**	df_d_lit		Date formatter for Date literals.
	**	df_t_lit		Date formatter for Time literals.
	**	epochDate		Epoch Date value.
	**	epochTime		Epoch Time value.
	**	epochTS			Epoch Timestamp value.
	**	tzCache			Timezone cache.
	**	tzoCache		Timezone offset cache.
	**
	** History:
	**	12-Sep-03 (gordy)
	**	    Created.
	**	19-Jun-06 (gordy)
	**	    ANSI Date/Time data type support: TZ_FMT, tzOffset(),
	**	    parseFrac(), and formatFrac().
	**	22-Sep-06 (gordy)
	**	    Added parseOffset(), formatOffset(), and getTZ(). 
	**	    Rename tzOffset() to formatTZ().  Cache timezone 
	**	    offsets: added tzoCache, tzCache.
	*/

	internal class
		SqlDates : GcfErr
	{

		/*
		** Date/Time constants for EPOCH values, literals, and
		** Ingres literals.
		**
		** Note: TZ_FMT is provided to show the standard TZ format
		** but isn't an actual usable format.
		*/
		internal const String	D_EPOCH		= "9999-12-31";
		internal const String	T_EPOCH		= "23:59:59";
		internal const String	TS_EPOCH	= D_EPOCH + " " + T_EPOCH;
		internal const String	D_FMT		= "yyyy-MM-dd";
		internal const String	T_FMT		= "HH:mm:ss";
		internal const String	TS_FMT		= D_FMT + " " + T_FMT;
		internal const String	D_LIT_FMT	= "yyyy_MM_dd";
		internal const String	T_LIT_FMT	= "HH:mm:ss";
		internal const String	TS_LIT_FMT	= D_LIT_FMT + " " + T_LIT_FMT;
		internal const String	TZ_FMT    	= "+HH:mm";

		/*
		** Common timezones and date/time formatters required by parsing and
		** formatting methods.  There are two fixed timezones for GMT and the
		** local machine timezone.  There are formatters for timestamp, date, 
		** and time values as well as literals.  
		**
		** Use of any formatter requires synchronization on the formatters 
		** themselves.
		*/
		private static TimeZone  	tz_gmt   = null; //TimeZone..getTimeZone( "GMT" );
		private static TimeZone  	tz_lcl   = TimeZone.CurrentTimeZone;
		private static DateFormat	df_ts_val= new DateFormat( TS_FMT );
		private static DateFormat	df_d_val = new DateFormat( D_FMT );
		private static DateFormat	df_t_val = new DateFormat( T_FMT );
		private static DateFormat	df_ts_lit= new DateFormat( TS_LIT_FMT );
		private static DateFormat	df_d_lit = new DateFormat( D_LIT_FMT );
		private static DateFormat	df_t_lit = new DateFormat( T_LIT_FMT );
		private static DateTime  	epochDate = DateTime.MinValue;
		private static DateTime  	epochTime = DateTime.MinValue;
		private static DateTime  	epochTS   = DateTime.MinValue;
		private static Hashtable 	tzCache   = new Hashtable();
		private static ArrayList 	tzoCache  = new ArrayList();


		/*
		** Name: getEpochDate
		**
		** Description:
		**	Returns epoch Date value relative to local timezone.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Date	Epoch date value.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static DateTime
			getEpochDate()
		{
			if (epochDate == DateTime.MinValue )
				epochDate = parseDate( D_EPOCH, tz_lcl );
			return( epochDate );
		} // getEpochDate


		/*
		** Name: getEpochTime
		**
		** Description:
		**	Returns epoch Time value relative to local timezone.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Time	Epoch Time value.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static DateTime
			getEpochTime()
		{
			if (epochTime == DateTime.MinValue)
				epochTime = parseTime( T_EPOCH, tz_lcl );
			return( epochTime );
		} // getEpochTime


		/*
		** Name: getEpochTimestamp
		**
		** Description:
		**	Returns epoch Timestamp value relative to local timezone.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Timestamp	Epoch Timestamp value.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static DateTime
			getEpochTimestamp()
		{
			if (epochTS == DateTime.MinValue )
				epochTS = parseTimestamp( TS_EPOCH, tz_lcl );
			return( epochTS );
		} // getEpochTimestamp


		/*
		** Name: parseOffset
		**
		** Description:
		**	Parses standard timezone offset "+HH:mm" and returns
		**	offset in minutes.
		**
		** Input:
		**	tzOffset	Standard timezone offset.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int		Offset in minutes.
		**
		** History:
		**	22-Sep-06 (gordy
		**	    Created.
		*/

		/// <summary>
		/// Parses standard timezone offset "+HH:mm"
		/// and returns offset in minutes.
		/// </summary>
		/// <param name="tzOffset"> string of timezone offset +HH:mm</param>
		/// <returns>Returns integer offset in minutes.</returns>
		public static int
		parseOffset( String tzOffset )
		{
			/*
			** Validate offset format.
			*/
			if (
			 tzOffset.Length != TZ_FMT.Length  ||
			 (tzOffset[0] != '+'  &&  tzOffset[0] != '-')  ||
			 ! Char.IsDigit( tzOffset[1] )  ||
			 !Char.IsDigit(tzOffset[2]) ||
			 tzOffset[3] != ':'  ||
			 !Char.IsDigit(tzOffset[4]) ||
			 !Char.IsDigit(tzOffset[5])
			   )
			throw SqlEx.get( ERR_GC401B_INVALID_DATE );

			int hours =
				Convert.ToInt32(tzOffset[1]-'0') * 10 +
				Convert.ToInt32(tzOffset[2]-'0');
			int minutes =
				Convert.ToInt32(tzOffset[4]-'0') * 10 +
				Convert.ToInt32(tzOffset[5]-'0');
			int offset = hours * 60 + minutes;
			if ( tzOffset[0] == '-' )  offset = -offset;

			return( offset );
		} // parseOffset


		/*
		** Name: formatOffset
		**
		** Description:
		**	Formats a timesone offset into standard format: "+HH:mm".
		**
		** Input:
		**	offset		Timezone offset in minutes.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted offset.
		**
		** History:
		**	22-Sep-06 (gordy)
		**	    Created.
		*/

		private static char[] digits = {'0','1','2','3','4','5','6','7','8','9'};

		public static String
		formatOffset( int offset )
		{
			String tzOffset = null;

			/*
			** Since only a small number of timezones are expected
			** to be active, a simple vector is used to map offsets.
			** Enumerating through the ArrayList is not thread-safe.
			** We lock the collection to avoid an enumerator throwing
			** NullReferenceException should another thread modify
			** the collection.
			*/
			lock (tzoCache.SyncRoot)
			{
				for (int i = 0; i < tzoCache.Count; i++)
				{
					IdMap tzo = (IdMap)tzoCache[i];

					if (tzo.equals(offset))
					{
						tzOffset = tzo.ToString();
						break;
					}
				}

				if (tzOffset == null)
				{
					bool neg = (offset < 0);
					if (neg) offset = -offset;
					int hour = offset / 60;
					int minute = offset % 60;

					char[] format = new char[6];
					format[0] = neg ? '-' : '+';
					format[1] = digits[(hour / 10) % 10];
					format[2] = digits[hour % 10];
					format[3] = ':';
					format[4] = digits[(minute / 10) % 10];
					format[5] = digits[minute % 10];

					if (neg) offset = -offset;	// Restore original offset
					tzOffset = new String(format);
					tzoCache.Add(new IdMap(offset, tzOffset));
				}
			}  // end lock (tzoCache.SyncRoot)

			return( tzOffset );
		} // formatOffset


		/*
		** Name: getTZ
		**
		** Description:
		**	Returns a timezone for a given TZ offset.
		**
		** Input:
		**	offset	Timezone offset in minutes.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	TimeZone	Timezone.
		**
		** History:
		**	22-Sep-06 (gordy)
		**	    Created.
		*/

		/// <summary>
		/// Returns a timezone for a given TZ offset (minutes).
		/// </summary>
		/// <param name="offset">Timezone offset in minutes.</param>
		/// <returns>Constructed TimeZone.</returns>
		public static TimeZone
		getTZ( int offset )
		{
			return( getTZ( formatOffset( offset ) ) );
		} // getTZ


		/*
		** Name: getTZ
		**
		** Description:
		**	Parses standard TZ offset format "+HH:mm" and returns
		**	a timezone with the same TZ offset.
		**
		** Input:
		**	tzOffset	TZ offset.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	TimeZone	Timezone.
		**
		** History:
		**	22-Sep-06 (gordy)
		**	    Created.
		*/

		public static TimeZone
		getTZ( String tzOffset )
		{
			/*
			** Only a small number timezones are expected to be active.
			** Timezones and offsets are cached for quick access.
			*/
			TimeZone tz;

			lock (tzCache.SyncRoot)  // sync access to the static tzCache hash
			{
			tz = (TimeZone)tzCache[ tzOffset ];

			if ( tz == null )
			{
				/*
				** Construct timezone for offset
				*/
				int offset = parseOffset( tzOffset );
				tz = new SimpleTimeZone( offset * 60000, "GMT" + tzOffset );
				tzCache.Add(tzOffset, tz);
			}
			}  // end lock

			return( tz );
		} // getTZ


		/*
		** Name: formatTZ
		**
		** Description:
		**	Returns the offset from UTC for a date/time value 
		**	in the local default timezone.  Standard TZ offset
		**	format is "+HH:mm".
		**
		** Input:
		**	value		Date/time value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		TZ offset.
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		**	22-Sep-06 (gordy)
		**	    Renamed.
		*/

		public static String
		formatTZ( DateTime value )
		{
			return (formatTZ(tz_lcl, value.Ticks / TimeSpan.TicksPerMillisecond));
		} // formatTZ


		/*
		** Name: formatTZ
		**
		** Description:
		**	Returns the offset from UTC for a date/time value 
		**	in a specific timezone.  Standard TZ offset format
		**	is "+HH:mm".
		**
		** Input:
		**	tz		Target timezone.
		**	value		Date/time value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		UTC offset.
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		**	22-Sep-06 (gordy)
		**	    Renamed.  
		*/

		public static String
		formatTZ( TimeZone tz, DateTime value )
		{
			return( formatTZ( tz, value.Ticks / TimeSpan.TicksPerMillisecond ) );
		} // formatTZ


		/*
		** Name: formatTZ
		**
		** Description:
		**	Returns the offset from UTC for a milli-second time
		**	value in the default timezone.  Standard TZ
		**	offset format is "+HH:mm".
		**
		** Input:
		**	millis		Milli-second time value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		UTC offset.
		**
		** History:
		**	22-Sep-06 (gordy)
		**	    Created.
		*/

		public static String
		formatTZ( long millis )
		{
			return( formatTZ( tz_lcl, millis ) );
		} // formatTZ


		/*
		** Name: formatTZ
		**
		** Description:
		**	Returns the offset from UTC for a date/time value
		**	in a specific timezone.  Standard TZ offset format
		**	is "+HH:mm".
		**
		** Input:
		**	tz		Target timezone.
		**	millis		Milli-second time value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		UTC ofset.
		**
		** History:
		**	22-Sep-06 (gordy)
		**	    Created.  Cache offset strings.
		*/

		public static String
		formatTZ( TimeZone tz, long millis )
		{
			const int MINUTES_PER_HOUR = 60;

			long     ticks    = millis * TimeSpan.TicksPerMillisecond;
			DateTime datetime = new DateTime(ticks, DateTimeKind.Local);
			TimeSpan timespan = tz.GetUtcOffset(datetime);

			int offsetMinutes =
				Math.Abs(timespan.Hours * MINUTES_PER_HOUR) +
				Math.Abs(timespan.Minutes);
			if (timespan.Ticks < 0) offsetMinutes = -offsetMinutes;

			return (formatOffset(offsetMinutes));
		} // formatTZ


		/*
		** Name: parseFrac
		**
		** Description:
		**	Parses a Fractional seconds timestamp component
		**	in the format ".ddddddddd" and returns the number
		**	of nano-seconds.  At a minimum, the decimal point
		**	must be present.  Any number of decimal digits 
		**	may be present.  Any digits beyond nano-seconds
		**	are ignored.
		**
		** Input:
		**	frac	Fractional seconds string.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	Nano-seconds
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		private static char[] zeros = {'0','0','0','0','0','0','0','0','0'};

		public static int
		parseFrac( String frac )
		{
			int nanos = 0;

			/*
			** Validate minimum format.
			*/
			if ( frac.Length < 1  ||  frac[ 0 ] != '.' )
			throw SqlEx.get( ERR_GC401B_INVALID_DATE );

			/*
			** Transform into standard format by truncating extra 
			** digits or appending trailing zeros as needed.
			*/
			StringBuilder sb = new StringBuilder(frac);
			if ( sb.Length > 10 )  sb.Length= 10;
			if ( sb.Length < 10 )  sb.Append( zeros, 0, 10 - sb.Length );

			/*
			** Extract number of nano-seconds that follow the ".".
			*/
			try { nanos = Int32.Parse( sb.ToString( 1,9 ) ); }
			catch( FormatException )
			{ throw SqlEx.get( ERR_GC401B_INVALID_DATE ); }

			return( nanos );
		} // parseFrac


		/*
		** Name: formatFrac
		**
		** Description:
		**	Formats nano-seconds into a fractional seconds
		**	timestamp component in the format ".dddddddddd".
		**	Trailing zeros are truncated, but at a minimum
		**	".0" will be returned.
		**
		** Input:
		**	nanos	Nano-seconds
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Fractional seconds string.
		**
		** History:
		**	19-Jun-06 (gordy)
		**	    Created.
		*/

		public static String
		formatFrac( int nanos )
		{
			/*
			** Handle minimal case.
			*/
			if ( nanos == 0 )  return( ".0" );
			if ( nanos < 0 )  nanos = -nanos;		// Shouldn't happen!

			StringBuilder sb = new StringBuilder();
			String	 frac = nanos.ToString();

			/*
			** Build-up the fractional seconds component.
			** Prepend zeros as necessary to get correct
			** magnitude.  
			*/
			sb.Append( '.' );
			if ( frac.Length < 9 )  sb.Append( zeros, 0, 9 - frac.Length );
			sb.Append( frac );

			/*
			** Remove trailing zeros.  There shouldn't be any
			** excess digits, but skip/truncate just in case.
			** Note: there is at least 1 non-zero digit since
			** the 0 case was handled above.
			*/
			int length = 10;
			while( sb[ length - 1 ] == '0' )  length--;
			sb.Length = length;

			return( sb.ToString() );
		} // formatFrac


		/*
		** Name: parseTime
		**
		** Description:
		**	Parse a string containing a time value using GMT or
		**	local timezone.
		**
		** Input:
		**	str		Time value.
		**	use_gmt		True for GMT, False for local timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Time		Time.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static DateTime
			parseTime( String str, bool use_gmt ) 
		{ 
			return( parseTime( str, use_gmt ? tz_gmt : tz_lcl ) ); 
		}


		/*
		** Name: parseTime
		**
		** Description:
		**	Parse a string containing a time value using an 
		**	explicit timezone.
		**
		** Input:
		**	str		Time value.
		**	tz		Timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Time		Time.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static DateTime
			parseTime( String str, TimeZone tz )
		{
			DateTime date;

			if (str.Length != T_FMT.Length)
				throw SqlEx.get(ERR_GC401B_INVALID_DATE);

			lock (df_t_val)
			{
				df_t_val.setTimeZone( tz );
				try { date = df_t_val.parse( str ); }
				catch( Exception )
				{ throw SqlEx.get( ERR_GC401B_INVALID_DATE ); }
			}

			return( date );
		} // parseTime


		/*
		** Name: formatTime
		**
		** Description:
		**	Format a date value into a time string using GMT or
		**	local timezone.
		**
		** Input:
		**	date		Jave date value.
		**	use_gmt		Use GMT or local timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted time.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatTime( DateTime date, bool use_gmt )
		{ 
			return( formatTime( date, use_gmt ? tz_gmt : tz_lcl ) ); 
		} // formatTime


		/*
		** Name: formatTime
		**
		** Description:
		**	Format a date value into a time string using an 
		**	explicit timezone.
		**
		** Input:
		**	date		Jave date value.
		**	tz		Timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted time.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatTime( DateTime date, TimeZone tz )
		{
			String str;

			lock( df_t_val )
			{
				df_t_val.setTimeZone( tz );
				str = df_t_val.format( date );
			}
    
			return( str );
		} // formatTime


		/*
		** Name: formatTimeLit
		**
		** Description:
		**	Format a date value into a time literal using GMT or
		**	local timezone.
		**
		** Input:
		**	date		Jave date value.
		**	use_gmt		Use GMT or local timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted time literal.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatTimeLit(DateTime date, bool use_gmt)
		{ 
			return( formatTimeLit( date, use_gmt ? tz_gmt : tz_lcl ) ); 
		} // formatTimeLit


		/*
		** Name: formatTimeLit
		**
		** Description:
		**	Format a date value into a time literal using an 
		**	explicit timezone.
		**
		** Input:
		**	date		Jave date value.
		**	tz		Timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted time literal.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatTimeLit(DateTime date, TimeZone tz)
		{
			String str;

			lock( df_t_lit )
			{
				df_t_lit.setTimeZone( tz );
				str = df_t_lit.format( date );
			}

			return( str );
		} // formatTimeLit


		/*
		** Name: parseDate
		**
		** Description:
		**	Parse a string containing a date value using GMT or
		**	local timezone.
		**
		** Input:
		**	str		Date value.
		**	use_gmt		True for GMT, False for local timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Date		Date.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static DateTime
			parseDate( String str, bool use_gmt ) 
		{ 
			return( parseDate( str, use_gmt ? tz_gmt : tz_lcl ) ); 
		} // parseDate


		/*
		** Name: parseDate
		**
		** Description:
		**	Parse a string containing a date value using an 
		**	explicit timezone.
		**
		** Input:
		**	str		Date value.
		**	tz		Timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Date		Date.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static DateTime
			parseDate( String str, TimeZone tz )
		{
			DateTime date;

			if (str.Length != D_FMT.Length)
				throw SqlEx.get(ERR_GC401B_INVALID_DATE);

			lock (df_d_val)
			{
				df_d_val.setTimeZone( tz );
				try { date = df_d_val.parse( str ); }
				catch( Exception )
				{ throw SqlEx.get( ERR_GC401B_INVALID_DATE ); }
			}

			return( date.Date );
		} // parseDate


		/*
		** Name: formatDate
		**
		** Description:
		**	Format a date value into a date string using GMT or
		**	local timezone.
		**
		** Input:
		**	date		Jave date value.
		**	use_gmt		True for GMT, False for local timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted date.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatDate( DateTime date, bool use_gmt )
		{ 
			return( formatDate( date, use_gmt ? tz_gmt : tz_lcl ) ); 
		} // formatDate


		/*
		** Name: formatDate
		**
		** Description:
		**	Format a date value into a date string using an 
		**	explicit timezone.
		**
		** Input:
		**	date		Jave date value.
		**	tz		Timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted date.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatDate( DateTime date, TimeZone tz )
		{
			String str;

			lock( df_d_val )
			{
				df_d_val.setTimeZone( tz );
				str = df_d_val.format( date );
			}

			return( str );
		} // formatDate


		/*
		** Name: formatDateLit
		**
		** Description:
		**	Format a date value into a date literal using GMT or
		**	local timezone.
		**
		** Input:
		**	date		Jave date value.
		**	use_gmt		True for GMT, False for local timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted date literal.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatDateLit( DateTime date, bool use_gmt )
		{ 
			return( formatDateLit( date, use_gmt ? tz_gmt : tz_lcl ) ); 
		} // formatDateLit


		/*
		** Name: formatDateLit
		**
		** Description:
		**	Format a date value into a date literal using an 
		**	explicit timezone.
		**
		** Input:
		**	date		Jave date value.
		**	tz		Timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted date literal.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatDateLit( DateTime date, TimeZone tz )
		{
			String str;
    
			lock( df_d_lit )
			{
				df_d_lit.setTimeZone( tz );
				str = df_d_lit.format( date );
			}
    
			return( str );
		} // formatDateLit


		/*
		** Name: parseTimestamp
		**
		** Description:
		**	Parse a string containing a timestamp value using GMT or
		**	local timezone.
		**
		** Input:
		**	str		Timestamp value.
		**	use_gmt		True for GMT, False for local timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Timestamp	Timestamp.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static DateTime
			parseTimestamp( String str, bool use_gmt ) 
		{ 
			return( parseTimestamp( str, use_gmt ? tz_gmt : tz_lcl ) ); 
		} // parseTimestamp


		/*
		** Name: parseTimestamp
		**
		** Description:
		**	Parse a string containing a timestamp value using an 
		**	explicit timezone.
		**
		** Input:
		**	str		Timestamp value.
		**	tz		Timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Timestamp	Timestamp.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static DateTime
			parseTimestamp( String str, TimeZone tz )
		{
			DateTime date;

			if (str.Length != TS_FMT.Length)
				throw SqlEx.get(ERR_GC401B_INVALID_DATE);

			lock (df_ts_val)
			{
				df_ts_val.setTimeZone( tz );
				try { date = df_ts_val.parse( str ); }
				catch( Exception )
				{ throw SqlEx.get( ERR_GC401B_INVALID_DATE ); }
			}

			return( date );
		} // parseTimestamp


		/*
		** Name: formatTimestamp
		**
		** Description:
		**	Format a date value into a timestamp string using GMT or
		**	local timezone.
		**
		** Input:
		**	date		Jave date value.
		**	use_gmt		True for GMT, False for local timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted timestamp.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatTimestamp( DateTime date, bool use_gmt )
		{ 
			return( formatTimestamp( date, use_gmt ? tz_gmt : tz_lcl ) ); 
		} // formatTimestamp


		/*
		** Name: formatTimestamp
		**
		** Description:
		**	Format a date value into a timestamp string using an
		**	explicit timezone.
		**
		** Input:
		**	date		Jave date value.
		**	tz		Timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted timestamp.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatTimestamp( DateTime date, TimeZone tz )
		{
			String str;
    
			lock( df_ts_val )
			{
				df_ts_val.setTimeZone( tz );
				str = df_ts_val.format( date );
			}
    
			return( str );
		} // formatTimestamp


		/*
		** Name: formatTimestampLit
		**
		** Description:
		**	Format a date value into a timestamp literal using GMT or 
		**	local timezone.
		**
		** Input:
		**	date		Jave date value.
		**	use_gmt		True for GMT, False for local timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted timestamp literal.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatTimestampLit(DateTime time, bool use_gmt)
		{
			DateTime date =
				new DateTime(1970, 1, 1,
					time.Hour, time.Minute, time.Second, time.Millisecond);
			string s = formatTimestampLit( date, use_gmt ? tz_gmt : tz_lcl );
			date = DateTime.Parse(s);
			return date.TimeOfDay.ToString();
		} // formatTimestampLit


		/*
		** Name: formatTimestampLit
		**
		** Description:
		**	Format a date value into a timestamp literal using an 
		**	explicit timezone.
		**
		** Input:
		**	date		Jave date value.
		**	tz		Timezone.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String		Formatted timestamp literal.
		**
		** History:
		**	12-Sep-03 (gordy)
		**	    Created.
		*/

		public static String
			formatTimestampLit( DateTime date, TimeZone tz )
		{
			String str;

			lock( df_ts_lit )
			{
				df_ts_lit.setTimeZone( tz );
				str = df_ts_lit.format( date );
			}

			return( str );
		} // formatTimestampLit

		/*
		** Name: getNanos
		**
		** Description:
		**	Constructor: removes ability to instantiate.
		**
		** Input:
		**	datetime		DateTime value.
		**
		** Output:
		**	Fractional part of the datatime in number of nanoseconds.
		**
		** Returns:
		**	None.
		**
		** History:
		**	18-Oct-06 (thoda04)
		**	    Created.
		**	24-Nov-09 (thoda04)  Bug 122955
		**	    Implemented the return of the nanoseconds from the DateTime.
		*/
		public static int getNanos(DateTime datetime)
		{
			TimeSpan timeofday  = datetime.TimeOfDay;

			long ticks =
				(timeofday.Ticks -             // total ticks less
				(timeofday.Hours * 
				    TimeSpan.TicksPerHour)  -  // hours' ticks less
				(timeofday.Minutes*
				    TimeSpan.TicksPerMinute)-  // minutes' ticks less
				(timeofday.Seconds*
				    TimeSpan.TicksPerSecond)); // seconds' ticks

			// ticks = ticks for the fraction of the last second

			return (int)ticks * 100;           // times 100 nanosec per tick
		}


		/*
		** Name: SqlDates
		**
		** Description:
		**	Constructor: removes ability to instantiate.
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

		private
			SqlDates()
		{
		} // SqlDates

		private class SimpleTimeZone : System.TimeZone
		{
			static TimeZone currentTimeZone = TimeZone.CurrentTimeZone;

			TimeSpan offset;
			string standardName;

			/// <summary>
			/// TimeZone wrapper to allow specification of offset and id.
			/// </summary>
			/// <param name="offset">Offset in milliseconds.</param>
			/// <param name="id">TimeZone ID.</param>
			public SimpleTimeZone(int offset, string id) :
				this(new TimeSpan(offset * TimeSpan.TicksPerMillisecond), id)
			{
			}

			public SimpleTimeZone(TimeSpan timespan, string id)
			{
				this.offset = timespan;
				this.standardName = id;
			}

			public override TimeSpan GetUtcOffset(DateTime time)
			{
				return this.offset;
			}

			public override DateTime ToLocalTime(DateTime time)
			{
				if (time.Kind == DateTimeKind.Local)
					return time;
				return time ;
			}

			public override DateTime ToUniversalTime(DateTime time)
			{
				if (time.Kind == DateTimeKind.Utc)
					return time;
				return base.ToUniversalTime(time);
			}

			public override string DaylightName
			{
				get { return this.standardName; }
			}

			public override string StandardName
			{
				get { return this.standardName; }
			}

			public override System.Globalization.DaylightTime GetDaylightChanges(int year)
			{
				return currentTimeZone.GetDaylightChanges(year);
			}
		}  // class SimpleTimeZone
	} // class SqlDates
}  // namespace
