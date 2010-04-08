/*
** Copyright (c) 1999, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Globalization;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: drvobj.cs
	**
	** Description:
	**	Defines the base class for driver classes which send 
	**	and/or receive DAM-ML messages to/from the server.
	**
	** History:
	**	 8-Sep-99 (gordy)
	**	    Created.
	**	13-Sep-99 (gordy)
	**	    Implemented error code support.
	**	29-Sep-99 (gordy)
	**	    Permit BLOB data to interrupt message processing.
	**	16-Nov-99 (gordy)
	**	    Added query timeouts.
	**	13-Dec-99 (gordy)
	**	    Added fetch limit.
	**	19-May-00 (gordy)
	**	    Added result parameter end-of-data and corresponding status field.
	**	16-Jun-00 (gordy)
	**	    Added procedure return value.
	**	 4-Oct-00 (gordy)
	**	    Standardized internal tracing.
	**	31-Oct-00 (gordy)
	**	    Added support for 2.0 extensions.
	**	28-Mar-01 (gordy)
	**	    Database connection and tracing given package access
	**	    so as to ease access to connection oriented info.
	**	    Tracing and default tracing added.
	**	11-Apr-01 (gordy)
	**	    Replaced date formatter repository with static formatters.
	**	18-Apr-01 (gordy)
	**	    Dropped getDefaultTrace().
	**	29-Aug-01 (loera01) SIR 105641
	**	    Added setProcResult method for retrieving return values of
	**	    database procedures.
	**	22-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	20-Aug-03 (gordy)
	**	    Consolidated result flags into single parameter.
	**	 6-Oct-03 (gordy)
	**	    RESULT messages (formerly DONE) can now occur before end-of-group.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-dec-04 (thoda04) SIR 113624
	**	    Add BLANKDATE=NULL support.
	**	21-Apr-06 (gordy)
	**	    Added INFO messages.
	**	19-Jun-06 (gordy)
	**	    ANSI Date/Time data type support.
	**	10-aug-09 (thoda04)
	**	    Ported Gordy's support of the MSG_ET_XA message for
	**	    additional information on any possible XA errors
	**	    to support commit/rollback by xid rather than by handle.
	*/


	/*
	** Name: DrvObj
	**
	** Description:
	**	Base class for driver classes which send and/or receive 
	**	DAM-ML messages to/from the server.
	**
	**	Provides the readResults() method for reading and processing
	**	messages expected as the result of a server request.  This
	**	method in turn calls the default handler methods to read 
	**	and process specific message types: readDesc(), readData(), 
	**	readInfo(), readError() and readResult().  There is also a 
	**	default handler method for procedure return values: 
	**	setProcResult().  These methods are expected to be over-
	**	ridden by sub-classes which need to provide non-default 
	**	processing.
	**
	**  Constants:
	**
	**	D_EPOCH			Date epoch value (zero or start date).
	**	T_EPOCH			Time epoch value (zero or start time).
	**	TS_EPOCH		Timestamp epoch value.
	**	D_FMT			Date format template.
	**	T_FMT			Time format template.
	**	TS_FMT			Timestamp format template.
	**
	**  Properties:
	**
	**	Warnings  	Retrieve the connection message list.
	**
	**  Interface Methods:
	**
	**	clearWarnings	Clear the connection message list.
	**	timeExpired  	Timer callback method.
	**
	**  Public Methods:
	**
	**	getDefaultTrace	Default internal tracing.
	**	toString       	Ingres tracing title.
	**
	**  Protected Data:
	**
	**	msg  			The database connection.
	**	trace			Tracing output.
	**	title			Class title for tracing.
	**	tr_id			Instance ID for tracing.
	**	inst_id			Object instance ID.
	**	warnings		SQL warnings.
	**	rslt_flags		Result flags.
	**	rslt_rowcount		Row count received.
	**	rslt_prefetch		Prefetch row size received.
	**	rslt_stmt_id		Statement ID received.
	**	rslt_proc_val		Procedure return value received.
	**	rslt_tblkey		Table key received.
	**	rslt_objkey		Object key received.
	**	rslt_val_rowcnt		Query update count.
	**	rslt_val_fetch		Optimal number of rows to fetch.
	**	rslt_val_stmt		Statement ID.
	**	rslt_val_proc		Procedure return value.
	**	rslt_val_tblkey		Table key.
	**	rslt_val_objkey		Object key.
	**
	**  Protected Methods:
	**
	**	setWarning		Add warning to warning list.
	**	getConnDateFormat	Get date formatter for connection.
	**	clearResults		Clear query results.
	**	readResults		Read Server result messages.
	**	readDesc		Default handler for descriptor messages.
	**	readData		Default handler for data messages.
	**	readInfo		Default handler for info messages.
	**	readError		Default handler for error messages.
	**	readResult		Default handler for result messages.
	**
	**  Properties
	**	ProcResult           Set procedure return value.
	**
	**  Private Data:
	**
	**	inst_count		Instance counter.
	**	default_trace		Default internal tracing.
	**	df_gmt			GMT Date formatter.
	**	df_lcl			Local Date formatter.
	**	timer			Statement execution timer.
	**	timed_out		Time-out flag.
	**
	** History:
	**	 8-Sep-99 (gordy)
	**	    Created.
	**	16-Nov-99 (gordy)
	**	    Added query timeouts.
	**	13-Dec-99 (gordy)
	**	    Added fetch limit.
	**	19-May-00 (gordy)
	**	    Added result end-of-data indicator.
	**	16-Jun-00 (gordy)
	**	    Added procedure return value.
	**	 4-Oct-00 (gordy)
	**	    Added instance counter, inst_count, and tracing ID,
	**	    tr_id, to standardize internal tracing.
	**	31-Oct-00 (gordy)
	**	    Renamed fetchLimit to fetchSize, added cursorReadonly.
	**	 3-Nov-00 (gordy)
	**	    Added date formatting info: D_EPOCH, T_EPOCH, TS_EPOCH, 
	**	    D_FMT, T_FMT, TS_FMT, GMT, df_date, df_time, getDateFormat(), 
	**	    getLocalDateFormat(), getConnDateFormat().
	**	28-Mar-01 (gordy)
	**	    Database connection and tracing given package access
	**	    so as to ease access to connection oriented info.
	**	    Added default_trace and getDefaultTrace().
	**	11-Apr-01 (gordy)
	**	    Replaced date formatter repository with static formatters
	**	    for GMT (df_gmt) and local (df_lcl) timezones and a re-
	**	    usable formatter (df_ts) for other timezones.  Methods
	**	    getLocalDateFormat() and getDateFormat() no longer needed.
	**	18-Apr-01 (gordy)
	**	    Dropped getDefaultTrace() now that tracing passed as needed.
	**	21-Feb-03 (gordy)
	**	    Added table and object keys.  Standardized result values
	**	    and flags.
	**	20-Aug-03 (gordy)
	**	    Consolidated result flags into single parameter.  Replaced
	**	    rslt_eod and rslt_readonly with rslt_flags.
	**	 6-Oct-03 (gordy)
	**	    Renamed readDone() to readResult().
	**	21-Apr-06 (gordy)
	**	    Added readInfo().
	**	19-Jun-06 (gordy)
	**	    Moved timeValuesInGMT() and timeLiteralsInGMT() to DrvConn.
	*/

	internal class DrvObj : MsgConst
	{
		/*
		** Constants used in date processing.
		*/
		public const String D_EPOCH = "9999-12-21";
		public const String T_EPOCH = "23:59:59";
		public const String TS_EPOCH = D_EPOCH + " " + T_EPOCH;
		public const String D_FMT = "yyyy-MM-dd";
		public const String T_FMT = "HH:mm:ss";
		public const String TS_FMT = D_FMT + " " + T_FMT;

		protected internal static readonly DateTime DT_EPOCH =
			new DateTime(9999, 12, 31, 23, 59, 59,0);

		protected internal const int FETCH_FORWARD = 0;
		protected internal const int SUCCESS_NO_INFO = (-2);

		/*
		** Connection relevent info.
		*/
		protected internal DrvConn conn;  // = null;
		protected internal MsgConn msg;   // = null;
		protected internal ITrace trace;  // = null;
		
		protected internal String title; // Tracing Ids
		protected internal String tr_id;
		protected int      inst_id;
		
		/*
		** Date formatters for use by sub-classes in handling date, time
		** and timestamp values.  There are two fixed timezone formatters
		** which are generally used for handling values between the driver
		** and the DBMS.  The GMT formatter should be used for an Ingres
		** DBMS since Ingres date values are stored in GMT.  The local
		** timezone formatter should be used for a non-Ingres DBMS since
		** they generally ignore timezone issues and client timezone is
		** assumed.  The correct formatter for the currect connection can
		** be accessed by calling getConnDateFormat().
		**
		** The other formatters are for use when a timezone other than GMT
		** or the local timezone is needed.  These formatters are generally
		** used for handling values between the application and the
		** driver.  There is a formatter for timestamps, dates, and times.
		** A timezone must be assigned to these formatters when used.  Use 
		** of these formatters requires synchronization on the formatters 
		** themselves.
		*/
		protected internal static DateFormat df_gmt  = new DateFormat(TS_FMT, "GMT");
		protected internal static DateFormat df_lcl  = new DateFormat(TS_FMT, "LOCAL");
		protected internal static DateFormat df_ts   = new DateFormat(TS_FMT, "GMT");
		protected internal static DateFormat df_date = new DateFormat(TS_FMT, "GMT");
		protected internal static DateFormat df_time = new DateFormat(T_FMT);
		
		/*
		** Server request result information.
		*/
		protected internal int    rslt_flags     = 0;
		protected internal int    rslt_items     = 0;
		protected internal bool   rslt_rowcount  = false;
		protected internal bool   rslt_prefetch  = false;
		protected internal bool   rslt_stmt_id   = false;
		protected internal bool   rslt_proc_val  = false;
		protected internal bool   rslt_tblkey    = false;
		protected internal bool   rslt_objkey    = false;

		protected const int RSLT_PREFETCH = 0x01;
		protected const int RSLT_ROW_STAT = 0x02;
		protected const int RSLT_ROW_POS  = 0x04;
		protected const int RSLT_ROW_CNT  = 0x08;
		protected const int RSLT_STMT_ID  = 0x10;
		protected const int RSLT_PROC_VAL = 0x20;
		protected const int RSLT_TBLKEY   = 0x40;
		protected const int RSLT_OBJKEY   = 0x80;

		protected internal int    rslt_val_fetch = 1;
		protected internal int    rslt_val_rowstat = 0;
		protected internal int    rslt_val_rowpos = 0;
		protected internal int    rslt_val_rowcnt = -1;
		protected internal long   rslt_val_stmt  = 0;
		protected internal int    rslt_val_proc  = 0;
		protected internal byte[] rslt_val_tblkey= null;
		protected internal byte[] rslt_val_objkey= null;

		protected internal SQLWarningCollection warnings = null;
		
		/*
		** Count number of driver objects created.
		** Used to assign unique tracing ID.
		*/
		private static int inst_count = 0;
		
		/*
		** GMT and Local timezone date formatters.
		** Timezone is assigned in static initializer.
		*/
		
		
		
		/*
		** Name: DrvObj
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	msg  	Database connection.
		**	trace	Connection tracing.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 8-Sep-99 (gordy)
		**	    Created.
		**	 4-Oct-00 (gordy)
		**	    Create unique tracing ID.
		**	28-Mar-01 (gordy)
		**	    Tracing added as parameter.
		*/
		
		protected internal DrvObj(DrvConn conn)
		{
			set(conn);

			/*
			** Default tracing identifiers.  Not very usefull,
			** but better than nothing.
			*/
			inst_id = inst_count++;
			title = DrvConst.shortTitle + "[" + inst_id + "]";
			tr_id = "Ingres[" + (inst_id) + "]";
		}  // DrvObj


		protected internal DrvObj(ITrace trace)
		{
			this.trace = trace;

			/*
			** Default tracing identifiers.  Not very usefull,
			** but better than nothing.
			*/
			inst_id = inst_count++;
			title = DrvConst.shortTitle + "[" + inst_id + "]";
			tr_id = DrvConst.shortTitle + "[" + (inst_id) + "]";
		}  // DrvObj


		/// <summary>
		/// Set the DrvConn server connection information.
		/// </summary>
		/// <param name="conn"></param>
		protected internal void set(DrvConn conn)
		{
			this.conn  = conn;
			if (conn == null)
			{
				this.msg   = null;
				this.trace = null;
				return;
			}

			this.msg   = conn.msg;
			this.trace = conn.trace;
		}


		/*
		** Name: toString
		**
		** Description:
		**	Return the formatted name of this object.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	String	    Object name.
		**
		** History:
		**	 7-Nov-00 (gordy)
		**	    Created.
		*/
		
		public override String ToString()
		{
			return (title);
		}


		/*
		** Name: Warnings property
		**
		** Description:
		**	Retrieve the connection warning/message list.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	SQLWarning  SQL warning/message list, may be null.
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		*/
		public virtual SQLWarningCollection Warnings
		{
			get
			{
				//if (trace.enabled())
				//	trace.log(title + ".Warnings()");
				return (warnings);
			}
			
		}
		
		
		/*
		** Name: clearWarnings
		**
		** Description:
		**	Clear the connection warning/message list.
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
		**	 5-May-99 (gordy)
		**	    Created.
		*/
		
		public virtual void  clearWarnings()
		{
			//if (trace.enabled())
			//	trace.log(title + ".clearWarnings()");
			warnings = null;
			return ;
		}
		// clearWarnings
		
		
		/*
		** Name: setWarning
		**
		** Description:
		**	Add a new warning to the list of warnings.
		**
		** Input:
		**	warn	New warning.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	 8-Nov-00 (gordy)
		**	    Created.
		*/
		
		protected internal virtual void setWarning(Exception warning)
		{
			if (warnings == null)
				warnings = new SQLWarningCollection();

			Warnings.Add(warning);

			return;
		}


		/*
		** Name: clearResults
		**
		** Description:
		**	Clear query result fields.
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
		**	 3-Nov-00 (gordy)
		**	    Created.
		**	 6-Mar-01 (gordy)
		**	    Clear warnings also.*/
		
		protected internal virtual void  clearResults()
		{
			warnings       = null;
			rslt_flags     = 0;
			rslt_rowcount  = false;
			rslt_val_rowstat= 0;
			rslt_val_rowpos = 0;
			rslt_val_rowcnt = -1;
			rslt_prefetch  = false;
			rslt_val_fetch = 0;
			rslt_stmt_id   = false;
			rslt_val_stmt  = 0;
			rslt_proc_val  = false;
			rslt_val_proc  = 0;
			rslt_tblkey    = false;
			rslt_objkey    = false;
			return ;
		}


		/*
		** Name: readResults
		**
		** Description:
		**	Read server result messages.  Processes messages until
		**	end-of-group is received or sub-class implementations
		**	of readData() and readResult() interrupt processing.
		**
		**	MSG_DESC and MSG_DATA messages are processed by methods
		**	readDesc() and readData() respectively.  The default
		**	implementation of these methods throws an exception.
		**	Sub-classes must provide their own implementation of
		**	these methods if these messages are expected as a part
		**	of the server response.
		**
		**	The readDesc() method returns a Result-set meta-data
		**	object representing the MSG_DESC message received,
		**	which is then returned if readResults() completes
		**	successfully.
		**
		**	MSG_ERROR and MSG_RESULT messages are processed by methods
		**	readError() and readResult() respectively.  The default
		**	implementation of these methods are described below.
		**	Sub-classes may provide their own implementation of
		**	these methods to provide non-default processing.
		**
		**	MSG_ERROR error messages are converted into exceptions
		**	which get thrown after all messages are processed.  
		**	MSG_ERROR warning and user messages are saved as warnings. 
		**	Multiple exceptions and warnings are chained together.
		**
		**	MSG_RESULT parameter info is saved in result info members. 
		**	The method setProcResult() is also called when a procedure 
		**      return value is received.  Sub-classes may override this 
		**	method to provide alternate handling of procedure results.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	 5-May-99 (gordy)
		**	    Created.
		**	29-Sep-99 (gordy)
		**	    Permit BLOB data to interrupt message processing.
		**	16-Nov-99 (gordy)
		**	    Added query timeouts.
		**	16-Dec-99 (gordy)
		**	    Added MSG_RP_FETCH_LIMIT.
		**	19-May-00 (gordy)
		**	    Added MSG_RP_EOD.
		**	16-Jun-00 (gordy)
		**	    Added procedure return value.
		**	31-Oct-00 (gordy)
		**	    Added read-only cursor return value.
		**	29-Aug-01 (loera01) SIR 105641
		**	    Execute setProcReturn if a DB procedure return value is read.
		**	21-Apr-06 (gordy)
		**	    Added INFO messages.
		*/

		/// <summary>
		/// Read server result messages.  Processes messages until
		/// end-of-group is received or sub-class implementations
		/// of readData() and readResult() interrupt processing.
		/// </summary>
		/// <returns>A result-set meta-data object representing
		/// the MSG_DESC message received, or null if no MSG_DESC msg.</returns>
		protected internal virtual AdvanRSMD readResults()
		{
			SqlEx ex = null;  // outstanding exception 
			                  // that hasn't been throw yet
			AdvanRSMD rsmd = null;
			
			byte msg_id;
			
			try
			{
				do
				{
					switch (msg_id = msg.receive() )
					{
						case MSG_DESC: 
							rsmd = readDesc();
							break;

						case MSG_DATA: 
							if (readData())
								goto break_msg_process_loop;  // if interrupt occurred
							break;

						case MSG_INFO:
							readInfo();
							break;

						case MSG_ERROR: 
							SqlEx exnew = readError();
							if (exnew != null)
							{
								if (ex == null)  // if first error message
									ex = exnew;  // then set is as the base ex
								else // append new error messages to existing ex
									ex.setNextException( exnew );
							}
							break;

						case MSG_RESULT:
							if (readResult())
								goto break_msg_process_loop;  // interrupt
							break;

						default: 
							if (trace.enabled(1))
								trace.write(tr_id + ": Invalid message ID " + msg_id);
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
					}  // end switch
				
					if (msg.moreData())
					{
						if (trace.enabled(1))
							trace.write(tr_id + ": end-of-message not reached");
						throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
					}
				}
				while ( msg.moreMessages());

			} // end try
			catch (SqlEx exnew)
			{
				if (ex == null)  // if first error message
					ex = exnew;  // then set is as the base ex
				else // append new error message to existing ex
					ex.setNextException( exnew );
			}
			
			break_msg_process_loop:
				if (ex != null)  // if any pending exceptions, throw now
				throw ex;
			return (rsmd);
		} // readResults


		/*
		** Name: readDesc
		**
		** Description:
		**	Read a data descriptor message.  The default action is to
		**	throw an exception.  A sub-class should override this method
		**	if a data descriptor message is a valid request result.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	AdvanRSMD    Result set data descriptor.
		**
		** History:
		**	 8-Sep-99 (gordy)
		**	    Created.
		*/
		
		protected internal virtual AdvanRSMD readDesc()
		{
			if (trace.enabled(1))
				trace.write(tr_id + ": unexpected result set");
			throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
		} // readDesc


		/*
		** Name: readData
		**
		** Description:
		**	Read a data message.  The default action is to throw an exception.  
		**	A sub-class should override this method if a data message is a 
		**	valid request result.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	    Interrupt reading results.
		**
		** History:
		**	 8-Sep-99 (gordy)
		**	    Created.
		**	29-Sep-99 (gordy)
		**	    Changed return type to permit BLOB segments to
		**	    interrupt the normal processing of messages.
		*/
		
		protected internal virtual bool readData()
		{
			if (trace.enabled(1))
				trace.write(tr_id + ": unexpected result data.");
			throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
		} // readData


		/*
		** Name: readInfo
		**
		** Description:
		**	Read an INFO message.  The INFO message parameters are read 
		**	and processed.  Currently, the only INFO parameter supported
		**	is Trace Text which is simply written to the trace file.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void
		**
		** History:
		**	21-Apr-06 (gordy)
		**	    Created.
		**	 3-Jul-06 (gordy)
		**	    Support dedicated DBMS trace log.
		*/

		protected internal virtual void readInfo()
		{
			while( msg.moreData() )
			{
			short   param_id = msg.readShort();
			short   param_len = msg.readShort();

			switch( param_id )
			{
			case MSG_IP_TRACE :
			{
				if ( trace.enabled()  ||  conn.dbms_log.enabled() )  
				{
				String txt = msg.readString( param_len );
				trace.log( "DBMS TRACE: " + txt );
				conn.dbms_log.write( txt );
				}
				else
				msg.skip( param_len );
				break;
			}

			default :
				if ( trace.enabled( 1 ) )
				trace.write( tr_id + ": Invalid info param ID " + param_id );
				throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
			}
			}

			return;
		} // readInfo


		/*
		** Name: readError
		**
		** Description:
		**	Read an ERROR message.  Warnings and User messages are saved
		**	on the warnings list and NULL is returned.  Errors are returned 
		**	as SqlEx objects.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	SqlEx	Error or NULL.
		**
		** History:
		**	31-Oct-02 (gordy)
		**	    Extracted from readResults().
		**	04-Mar-09 (thoda04)
		**	    Support XA errors.
		*/

		protected internal virtual SqlEx readError()
		{
			SqlEx ex = null;

			int status      = msg.readInt();
			String sqlState = msg.readString(5);
			byte   type     = msg.readByte();
			String message  = msg.readString();

			switch( type )
			{
			case MSG_ET_ERR:
				if (trace.enabled(1))
					trace.write(tr_id + ": Received error '" + sqlState + "' 0x" +
						 System.Convert.ToString(status, 16) + " -- " + message);

				ex = SqlEx.get(message, sqlState, status);
				break;

			case MSG_ET_WRN:
				if (trace.enabled(1))
					trace.write(tr_id + ": Received warning '" + sqlState + "' 0x" +
						 System.Convert.ToString(status, 16) + " -- " + message);

				setWarning(SqlEx.get(message, sqlState, status));
				break;

			case MSG_ET_MSG:
				if (trace.enabled(1))
					trace.write(tr_id + ": Received message '" + sqlState + "' 0x" +
						 System.Convert.ToString(status, 16) + " -- " + message);

				setWarning(SqlEx.get(message, sqlState, status));
				break;

			case MSG_ET_XA:
				if (status >= 0)  // not an error if all is OK
					break;

				//get a readable message of the XAER_xxx such as
				//   "XA error XAER_NOTA (the XID is not valid)"
				message = XASwitch.XAER_ToString(status);

				if (trace.enabled(1))
					trace.write(tr_id + ": Received XA error " + "0x" +
						 System.Convert.ToString(status, 16) + " -- " + message);

				ex = SqlEx.get(message, "5000R", status);
				break;
		}
			return ex;
		}  // readError


		/*
		** Name: readResult
		**
		** Description:
		**	Read a RESULT message.  The RESULT message parameters are read 
		**	and saved in instance fields.
		**
		**	Returning a value of TRUE will interrupt the processing of 
		**	server messages by readResults() and cause an immediate return.
		**
		** Input:
		**	None.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	boolean	True for interrupt, False to continue.
		**
		** History:
		**	31-Oct-02 (gordy)
		**	    Extracted from readResults().
		**	21-Feb-03 (gordy)
		**	    Added table and object keys.
		**	20-Aug-03 (gordy)
		**	    Consolidated result flags into single parameter.
		**	 6-Oct-03 (gordy)
		**	    Renamed and changed return type to reflect ability
		**	    for intermediate RESULT messages to be processed.
		*/

		protected internal virtual bool readResult()
		{
			while (msg.moreData())
			{
				short param_id  = msg.readShort();
				short param_len = msg.readShort();

				switch (param_id)
				{
					case MSG_RP_ROWCOUNT: 
						switch (param_len)
						{
							case 1:  rslt_val_rowcnt = msg.readByte(); break;
							case 2:  rslt_val_rowcnt = msg.readShort(); break;
							case 4:  rslt_val_rowcnt = msg.readInt(); break;
							default: 
								if (trace.enabled(1))
									trace.write(tr_id +
										": Invalid rowcount len " +param_len);
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
											
						}
						rslt_rowcount = true;
						break;

					case MSG_RP_XACT_END: 
						if (param_len > 0)
						{
							if (trace.enabled(1))
								trace.write(tr_id + ": Invalid XACT param len " + param_len);
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}

						rslt_flags |= MSG_RF_XACT_END;
						conn.endXact();
						break;

					case MSG_RP_STMT_ID: 
						if (param_len != 8)
						{
							if (trace.enabled(1))
								trace.write(tr_id + ": Invalid STMT_ID len " + param_len);
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}
									
						rslt_val_stmt =
							(((long) msg.readInt()) << 32) |
							(((long) msg.readInt()) &
								(int) (- (0x100000000 - 0xffffffff)));
						break;

					case MSG_RP_FETCH_LIMIT: 
						switch (param_len)
						{
							case 1:  rslt_val_fetch = msg.readByte();  break;
							case 2:  rslt_val_fetch = msg.readShort(); break;
							case 4:  rslt_val_fetch = msg.readInt();   break;
							default: 
								if (trace.enabled(1))
									trace.write(tr_id +
										": Invalid fetchlimit len " +
										param_len);
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}
						rslt_prefetch = true;
						break;

					case MSG_RP_EOD: 
						if (param_len != 0)
						{
							trace.write(tr_id + ": Invalid EOD len " +
								param_len);
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}
						rslt_flags |= MSG_RF_EOD;
						break;

					case MSG_RP_PROC_RESULT: 
					switch (param_len)
					{
						case 1: setProcResult((int)msg.readByte());  break;
						case 2: setProcResult((int)msg.readShort()); break;
						case 4: setProcResult(     msg.readInt());   break;
						default: 
							if (trace.enabled(1))
								trace.write(tr_id + ": Invalid proc result len" + param_len);
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
					}
						break;


					case MSG_RP_READ_ONLY: 
						if (param_len != 0)
						{
							trace.write(tr_id + ": Invalid READ_ONLY len " + param_len);
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}
						rslt_flags |= MSG_RF_READ_ONLY;
						break;

					case MSG_RP_TBLKEY :
						if ( param_len != MSG_RPV_TBLKEY_LEN )
						{
							if ( trace.enabled( 1 ) )
								trace.write( tr_id +
									": Invalid table key length: " +
									param_len );
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}

						if (rslt_val_tblkey == null)
							rslt_val_tblkey = new byte[param_len];
						msg.readBytes( rslt_val_tblkey, 0, param_len );
						rslt_tblkey = true;
						break;

					case MSG_RP_OBJKEY :
						if ( param_len != MSG_RPV_OBJKEY_LEN )
						{
							if ( trace.enabled( 1 ) )
								trace.write( tr_id + ": Invalid object key length: " +
									param_len );
							throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}

						if (rslt_val_objkey == null) rslt_val_objkey = new byte[param_len];
						msg.readBytes( rslt_val_objkey, 0, param_len );
						rslt_objkey = true;
						break;

					case MSG_RP_FLAGS :
					{
						int flags;

						switch( param_len )
						{
							case 1 : flags = msg.readByte();   break;
							case 2 : flags = msg.readShort();  break;
							case 4 : flags = msg.readInt();    break;

							default :
								if ( trace.enabled( 1 ) )
									trace.write( tr_id +
										": Invalid result flags length: " +
										param_len );
								throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );
						}

						rslt_flags |= flags;
						if ( (flags & MSG_RF_XACT_END) != 0 )
							conn.endXact();
						break;
					}

					case MSG_RP_ROW_STATUS:
					switch (param_len)
					{
						case 1: rslt_val_rowstat = msg.readByte(); break;
						case 2: rslt_val_rowstat = msg.readShort(); break;
						case 4: rslt_val_rowstat = msg.readInt(); break;

						default:
							if (trace.enabled(1))
								trace.write(tr_id +
									": Invalid row status length: " +
									 param_len);
							throw SqlEx.get(ERR_GC4002_PROTOCOL_ERR);
					}

					rslt_items |= RSLT_ROW_STAT;
					break;

					case MSG_RP_ROW_POS:
					switch (param_len)
					{
						case 1: rslt_val_rowpos = msg.readByte(); break;
						case 2: rslt_val_rowpos = msg.readShort(); break;
						case 4: rslt_val_rowpos = msg.readInt(); break;

						default:
							if (trace.enabled(1))
								trace.write(tr_id +
									": Invalid row position length: " +
									 param_len);
							throw SqlEx.get(ERR_GC4002_PROTOCOL_ERR);
					}

					rslt_items |= RSLT_ROW_POS;
					break;

					default: 
						if (trace.enabled(1))
							trace.write(tr_id +
								": Invalid result param ID " + param_id);
						throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR );


				}  // end switch
			}  // end while

			return false;
		}  // readResult


		/*
		** Name: setProcResult
		**
		** Description:
		**	Receive procedure return value.  The default action is to save
		**	the return value in the instance member rslt_val_proc and set 
		**	the result indication flag.  A sub-class should override this 
		**	method to provide alternate handling of procedure results.
		**
		** Input:
		**	retval		Procedure return value.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	Void.
		**
		** History:
		**	29-Aug-01 (loera01) SIR 105641
		**	    Created.
		*/

		protected internal virtual void setProcResult(int value)
		{
			this.rslt_val_proc = value;
			rslt_proc_val = true;
		} // setProcResult


		/*
		** Name DataTruncation
		**
		** Description:
		**	Constructs an IngresError for a data truncation warning.
		**
		** Input:
		**	colName   	column name that was truncated.
		**	colOrdinal	ordinal number of column.
		**	parm      	if true, it's a parameter, else a result column.
		**	reading   	if ture, truncated on read, else truncated on write
		**
		** Output:
		**	Build an error
		**	   "Data truncation on read/write of parameter/result column." +
		**	   "<column name> at ordinal nn"
		**
		** Returns:
		**	IngresError.
		**
		** History:
		**	29-Aug-02 (thoda04)
		**	    Created.
		*/

		internal Exception DataTruncation(
			string colName,
			int colOrdinal,
			bool parm,
			bool reading)
		{
			return SqlEx.get(
				"Data truncation on " +
				(reading ? "read of " : "write of ") +
				(parm ? "parameter " : "result column ") +
				(colName == null ? "" : colName.ToString()) +
				" at ordinal " + colOrdinal + ".",
				"01004", 0);  // SQLState and native code
		} // DataTruncation

		/*
		** Name DataTruncation
		**
		** Description:
		**	Constructs an IngresError for a data truncation warning.
		**
		** Input:
		**	colOrdinal	ordinal number of column.
		**	parm      	if true, it's a parameter, else a result column.
		**	reading   	if ture, truncated on read, else truncated on write
		**
		** Output:
		**	Build an error
		**	   "Data truncation on read/write of parameter/result column." +
		**	   "at ordinal nn"
		**
		** Returns:
		**	IngresError.
		**
		** History:
		**	29-Aug-02 (thoda04)
		**	    Created.
		*/

		internal Exception DataTruncation(
			int colOrdinal,
			bool parm,
			bool reading,
			int dataSize,
			int transferSize)
		{
			return SqlEx.get(
				"Data truncation on " +
				(reading ? "read of " : "write of ") +
				(parm ? "parameter " : "result column ") +
				"at ordinal " + colOrdinal +
				".  Original size of data = " + dataSize +
				".  Size after truncation = " + transferSize + ".",
				"01004", 0);  // SQLState and native code
		} // DataTruncation

		/*
		** Name DataTruncation
		**
		** Description:
		**	Constructs an IngresError for a data truncation warning.
		**
		** Input:
		**	colName   	column name that was truncated.
		**	colOrdinal	ordinal number of column.
		**	parm      	if true, it's a parameter, else a result column.
		**	reading   	if ture, truncated on read, else truncated on write
		**
		** Output:
		**	Build an error
		**	   "Data truncation on read/write of parameter/result column." +
		**	   "<column name> at ordinal nn"
		**
		** Returns:
		**	IngresError.
		**
		** History:
		**	29-Aug-02 (thoda04)
		**	    Created.
		*/

		internal Exception DataTruncation(
			string colName,
			int colOrdinal,
			bool parm,
			bool reading,
			int dataSize,
			int transferSize)
		{
			return SqlEx.get(
				"Data truncation on " +
				(reading ? "read of " : "write of ") +
				(parm ? "parameter " : "result column ") +
				(colName == null ? "" : colName.ToString()) +
				" at ordinal " + colOrdinal +
				".  Original size of data = " + dataSize +
				".  Size after truncation = " + transferSize + ".",
				"01004", 0);  // SQLState and native code
		} // DataTruncation

	} // class DrvObj
}