/*
** Copyright (c) 2002, 2006 Ingres Corporation. All Rights Reserved.
*/

namespace Ingres.Utility
{
	/*
	** Name: tracingeventlog.cs
	**
	** Description:
	**	Class which implements the ITrace interface to provide
	**	 access the Windows Event Log.
	**
	** History:
	**	 1-Oct-03 (thoda04)
	**	    Created for .NET Provider.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	04-may-06 (thoda04)
	**	    Change 1.1 ConfigurationSettings.AppSettings
	**	    to .NET 2.0 ConfigurationManager.AppSettings.
	*/

	using System;
	using System.Configuration;
	using System.Diagnostics;
	
	
	/*
	** Name: TracingToEventLog
	**
	** Description:
	**	Class which implements ITrace interface providing access to a 
	**	centralized static trace log and registry of tracing levels.  
	**	Class instances provide simplified tracing level access to the 
	**	trace log.
	**
	**  Interface Methods:
	**
	**	setTraceLog  	Set tracing log.
	**	setTraceLevel	Set tracing level.
	**	getTraceLevel	Get tracing level.
	**	enabled      	Is tracing enabled.
	**	write        	Write to trace log.
	**	hexdump      	Write hexdump to trace log.
	**
	**  Private Methods:
	**
	**	timestamp	    Returns current timestamp.
	**
	**  Constants:
	**
	**	KEY_TRACE	    System properties base key.
	**	KEY_TRACE_LOG	    Property key for trace log.
	**	KEY_TRACE_TIME	    Property key for tracing timestamps.
	**	hex		    Decimal to Hex conversion characters.
	**
	**  Private Data:
	**
	**	trace_log	    Global trace log.
	**	levels		    Global tracing level registry.
	**	trace_time	    Traces include timestamps?
	**	df_ts		    Date format for timestamps.
	**	hex_buff	    Buffer used by hexdump();
	**	trace_level	    Current tracing level.
	**
	** History:
	**	 6-May-99 (gordy)
	**	    Created.
	**	22-Sep-99 (gordy)
	**	    Replaced deprecated PrintStream with PrintWriter.
	**	20-Apr-00 (gordy)
	**	    Reworked as stand alone class rather than base class.
	**	    Added per-instance data/members and static registry
	**	    to permit multiple tracing levels and ID's.
	**	15-Nov-00 (gordy)
	**	    Added KEY_TRACE, KEY_TRACE_LOG, and static initializer
	**	    to support trace settings in the system properties.
	**	16-Mar-00 (gordy)
	**	    Made all methods non-static as required for interface.
	**	12-Apr-01 (gordy)
	**	    Removed exception tracing (should be self tracing).
	**	 5-Feb-02 (gordy)
	**	    Added timestamp(), KEY_TRACE_TIME, trace_time, df_ts.
	**	29-Jul-02 (thoda04)
	**	    Ported to .NET Provider.
	*/
	
	internal class TracingToEventLog : ITrace
	{
		private const  String KEY_TRACE = "Ingres.trace";
		//private static String KEY_TRACE_LOG = KEY_TRACE + ".log";
		//private static String KEY_TRACE_TIME = KEY_TRACE + ".timestamp";
		
		private static EventLog trace_log;
		private static System.Collections.Hashtable levels =
			new System.Collections.Hashtable(); // Registry
		private static bool trace_time = false;
		private static DateFormat df_ts  = 
			new DateFormat("yyyy-MM-dd HH:mm:ss.SSS", "LOCAL");
		
		/*
		** Buffer used by hexdump (requires synchronization)
		*/
		private static char[] hex_buff = new char[(4 * 16) + 4];
		private static char[] hex = new char[]{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
		
		private int trace_level = 0; // Tracing ID level.
		
		
		/*
		** Name: <class initializer>
		**
		** Description:
		**	Check to see if the trace log has been defined as
		**	a system property and initialize appropriately.
		**
		** History:
		**	15-Nov-00 (gordy)
		**	    Created.
		**	 5-Feb-02 (gordy)
		**	    Check to see if timestamps are enabled.
		**	20-Mar-02 (gordy)
		**	    Handle security exceptions thrown by getProperty().
		**	29-Jul-02 (thoda04)
		**	    Ported to .NET Provider.
		*/
		
		static TracingToEventLog()  // Class Initializer
		{
			try
			{
				string source = "Ingres .NET Data Provider";
				if (!EventLog.SourceExists(source))
					 EventLog.CreateEventSource(source, "Application");

				// create new Windows event log object for local machine
				trace_log = new EventLog("Application", ".", source);
			}
			catch (System.Exception /* ignore and leave trace_log as null */)
			{
			}
		}  // static


		/*
		** Name: TracingToEventLog
		**
		** Description:
		**	Class constructor.  Instance supports tracing level access
		**	for a specific tracing ID.  The tracing level may be specified
		**	as a system property of the form Ingres.trace.<ID>=<level>.  If
		**	not set at the system level, the trace level registry is used.
		**
		** Input:
		**	id	Tracing ID.  Currently the only value used is "XA".
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 1-Oct-03 (thoda04)
		**	    Ported to .NET Provider from tracing.cs.
		*/
		
		public TracingToEventLog(String id)
		{
			try
			{
				String level;

				if (ConfigurationManager.AppSettings != null &&
					(level = ConfigurationManager.AppSettings.Get(
						KEY_TRACE + "." + id)) != null)
					trace_level = System.Int32.Parse(level);
				else
				{
					object obj = levels[id];
					if (obj == null)
						trace_level = 0;
					else
						trace_level = Convert.ToInt32(obj);
				}
			}
			catch (System.Exception /* ignore */)
			{
			}
		}
		// TracingToEventLog
		
		
		/*
		** Name: getTraceName
		**
		** Description:
		**	Returns a string which describes this tracing instance.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	Tracing name.
		**
		** History:
		**	31-Oct-02 (gordy)
		**	    Created.
		*/

		public String 
			getTraceName()
		{
			return( Trace_Const.TraceID );
		} // getTraceName


		/*
		** Name: setTraceLog
		**
		** Description:
		**	Set the trace log.  If the log name is NULL, tracing will 
		**	be disabled.  A single global trace log is maintained.
		**
		** Input:
		**	name	    New trace log file name, may be NULL.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    False if error opening trace log.
		**
		** History:
		**	 1-Oct-03 (thoda04)
		**	    Created.
		*/
		
		public virtual System.Boolean setTraceLog(String name)
		{
			return (true);  // not needed for Event Log
			
		}  // setTraceLog
		
		
		/*
		** Name: setTraceLevel
		**
		** Description:
		**	Set the tracing level for a given trace ID.  Returns
		**	the previous tracing level or zero if not previously
		**	set.
		**
		** Input:
		**	id   	    Trace ID.
		**	level	    New tracing level.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Previous tracing level.
		**
		** History:
		**	 6-May-99 (gordy)
		**	    Created.*/
		
		public virtual int setTraceLevel(String id, int level)
		{
			Object prev = levels[ id ];
			levels[ id ] = level;
			return ( prev != null? (Int32)prev:0);
		} // setTraceLevel
		
		
		/*
		** Name: setTraceLevel
		**
		** Description:
		**	Sets the current tracing level.  Note: does not change
		**	the tracing level for a registered ID.
		**
		** Input:
		**	level	    Tracing level.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Previous tracing level.
		**
		** History:
		**	20-Apr-00 (gordy)
		**	    Created.*/
		
		public int setTraceLevel(int level)
		{
			int old_level = trace_level;
			trace_level = level;
			return (old_level);
		} 	// setTraceLevel
		
		
		/*
		** Name: getTraceLevel
		**
		** Description:
		**	Get the tracing level for a trace ID.  Zero is returned
		**	for any ID which has not had its tracing level set.
		**
		** Input:
		**	id	    Trace ID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Current tracing level.
		**
		** History:
		**	20-Apr-00 (gordy)
		**	    Created.*/
		
		public virtual int getTraceLevel(String id)
		{
			object obj = levels[id];
			if (obj == null)
				return 0;
			int traceLevel = Convert.ToInt32(obj);
			return traceLevel;
		} // getTraceLevel
		
		
		/*
		** Name: getTraceLevel
		**
		** Description:
		**	Returns the current tracing level.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	int	    Current tracing level.
		**
		** History:
		**	20-Apr-00 (gordy)
		**	    Created.*/
		
		public virtual int getTraceLevel()
		{
			return (trace_level);
		} // getTraceLevel
		
		
		/*
		** Name: enabled
		**
		** Description:
		**	Returns True if internal driver tracing is enabled (at any level).
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if tracing enabled.
		**
		** History:
		**	16-Mar-01 (gordy)
		**	    Created.*/
		
		public virtual bool enabled()
		{
			return (enabled(1));
		} // enabled
		
		
		/*
		** Name: enabled
		**
		** Description:
		**	Returns the current tracing state for a target trace level.
		**
		** Input:
		**	level	    Tracing level.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    True if tracing enabled,
		**
		** History:
		**	 6-May-99 (gordy)
		**	    Created.*/
		
		public virtual bool enabled(int level)
		{
			return (trace_log != null  &&  level <= trace_level);
		} // enabled
		
		
		/*
		** Name: write
		**
		** Description:
		**	Write a line to the trace log.  A line terminator is appended 
		**	to the output.  No action is taken if tracing is not enabled.
		**
		** Input:
		**	str	    String to be written to the trace log.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 6-May-99 (gordy)
		**	    Created.
		**	 5-Feb-02 (gordy)
		**	    Added optional timestamp.*/
		
		public virtual void  write(String str)
		{
			if (trace_time)
				str = timestamp() + ": " + str;
			println(str);
			return ;
		} // write
		
		
		/*
		** Name: write
		**
		** Description:
		**	Write a line to the trace log.  A line terminator is appended
		**	to the output.  No action is taken if tracing is not enabled.
		**
		** Input:
		**	level	    Tracing level.
		**	str	    String to be written to the trace log.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 6-May-99 (gordy)
		**	    Created.
		**	 5-Feb-02 (gordy)
		**	    Added optional timestamp.*/
		
		public virtual void  write(int level, String str)
		{
			if (trace_log != null  &&  level <= trace_level)
			{
				if (trace_time)
					str = timestamp() + ": " + str;
				println(str);
			}
			return ;
		} // write
		
		
		/*
		** Name: hexdump
		**
		** Description:
		**	Format a hex dump of a byte array (subset) and write to
		**	the trace file.
		**
		** Input:
		**	buffer	    Byte array.
		**	offset	    Starting byte to be output.
		**	length	    Number of bytes to output.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 6-May-99 (gordy)
		**	    Created.*/
		
		public virtual void  hexdump(byte[] buffer, int offset, int length)
		{
			int    oldlength;
			string str;

			lock(hex_buff)
			{
				while (length > 0)
				{
					oldlength = length;
					length = Tracing.HexToString(buffer, offset, length, out str);
					offset += (oldlength-length);  // incr offset by decr'ed length
					if (str != null  && str.Length > 0)
						println(str);
				}
			}
			return;
		} // hexdump
		
		
		/*
		** Name: log
		**
		** Description:
		**	Writes trace output to trace log (if enabled at any level).
		**
		** Input:
		**	str	Trace message.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	16-Mar-01 (gordy)
		**	    Created.*/
		
		public virtual void  log(String str)
		{
			write(1, str);
			return ;
		} // log
		
		
		/*
		** Name: println
		**
		** Description:
		**	Print an output line to the trace log (if open).
		**
		** Input:
		**	str	    String to be written to the trace log.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 6-May-99 (gordy)
		**	    Created.*/
		
		protected internal static void  println(String str)
		{
			try
			{
				if (trace_log != null  &&  str != null)
					trace_log.WriteEntry(str);
			}
			catch (System.Exception /* ignore */)
			{
				trace_log = null;  // write failed, so don't try next time
			}
			return ;
		}
		
		
		/*
		** Name: timestamp
		**
		** Description:
		**	Returns a tracing timestamp string for the current time.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Timestamp string.
		**
		** History:
		**	 5-Feb-02 (gordy)
		**	    Created.
		**	29-Jul-02 (thoda04)
		**	    Ported to .NET Provider.
		*/
		
		private static String timestamp()
		{
			return df_ts.ToMillisecondString(System.DateTime.Now);
		}
		// timestamp
		
		
	}  // class TracingToEventLog
}