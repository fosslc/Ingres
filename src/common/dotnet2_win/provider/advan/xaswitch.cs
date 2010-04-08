/*
** Copyright (c) 2003, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Runtime.InteropServices;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: xaswitch.cs
	**
	** Description:
	**	Base classes for XaSwitch support.
	**
	**  Classes
	**
	**	XASwitch  X/Open XASwitch style methods, but in a .NET environment.
	**	RM        Resource Manager class that hold the association context.
	**
	** History:
	**	18-Aug-03 (thoda04)
	**	    Created.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	10-aug-09 (thoda04)
	**	    Added XAER_ToString() for human-readable XA error message.
	*/


	/*
	** Class Name:   XASwitch
	**
	** Description:
	**	X/Open XASwitch style methods, but in a .NET environment.
	**
	*/


	/// <summary>
	/// X/Open XASwitch style methods, but in a .NET environment.
	/// </summary>
	[CLSCompliant(false)]
	public class XASwitch
	{
		internal String title = null; // Title for tracing
		internal Ingres.Utility.ITrace trace;

		System.Collections.Hashtable RMcollection =
			new System.Collections.Hashtable();
		int CountOfRMsTracing = 0;


		/*
		[StructLayout(LayoutKind.Sequential,CharSet=CharSet.Ansi)]
		public struct xa_switch
		{
			[MarshalAs(UnmanagedType.ByValTStr, SizeConst=32)]
			public string name;
			[MarshalAs(UnmanagedType.I4)]
			public int flags;
			[MarshalAs(UnmanagedType.I4)]
			public int version;

			public Delegate_xa_open     delegate_xa_open;
			public Delegate_xa_close    delegate_xa_close;
			public Delegate_xa_start    delegate_xa_start;
			public Delegate_xa_end      delegate_xa_end;
			public Delegate_xa_rollback delegate_xa_rollback;
			public Delegate_xa_prepare  delegate_xa_prepare;
			public Delegate_xa_commit   delegate_xa_commit;
			public Delegate_xa_recover  delegate_xa_recover;
			public Delegate_xa_forget   delegate_xa_forget;
			public Delegate_xa_complete delegate_xa_complete;
			}
			*/

		/*
		 * xa_() return codes (resource manager reports to transaction manager)
		 */
const int XA_RBBASE     = 100;           	// The inclusive lower bound of the rollback codes
const int XA_RBROLLBACK = XA_RBBASE;     	// The rollback was caused by an unspecified reason
const int XA_RBCOMMFAIL = XA_RBBASE+1;   	// The rollback was caused by a communication failure
const int XA_RBDEADLOCK = XA_RBBASE+2;   	// A deadlock was detected
const int XA_RBINTEGRITY= XA_RBBASE+3;   	// A condition that violates the integrity of the resources was detected
const int XA_RBOTHER    = XA_RBBASE+4;   	// The resource manager rolled back the tx branch for a reason not on this list
const int XA_RBPROTO    = XA_RBBASE+5;   	// A protocol error occurred in the resource manager
const int XA_RBTIMEOUT  = XA_RBBASE+6;   	// A tx branch took too long
const int XA_RBTRANSIENT= XA_RBBASE+7;   	// May retry the tx branch
const int XA_RBEND      = XA_RBTRANSIENT;	// The inclusive upper bound of the rollback codes

const int XA_NOMIGRATE  = 9;   	// resumption must occur where suspension occurred
const int XA_HEURHAZ    = 8;   	// the tx branch may have been heuristically completed
const int XA_HEURCOM    = 7;   	// the tx branch has been heuristically committed
const int XA_HEURRB     = 6;   	// the tx branch has been heuristically rolled back
const int XA_HEURMIX    = 5;   	// the tx branch has been heuristically committed and rolled back
const int XA_RETRY      = 4;   	// routine returned with no effect and may be re-issued
const int XA_RDONLY     = 3;   	// the tx branch was read-only and has been committed
const int XA_OK         = 0;   	// normal execution
const int XAER_ASYNC    = (-2);	// asynchronous operation already outstanding
const int XAER_RMERR    = (-3);	// a resource manager error occurred in the tx branch
const int XAER_NOTA     = (-4);	// the XID is not valid
const int XAER_INVAL    = (-5);	// invalid arguments were given
const int XAER_PROTO    = (-6);	// routine invoked in an improper context
const int XAER_RMFAIL   = (-7);	// resource manager unavailable
const int XAER_DUPID    = (-8);	// the XID already exists
const int XAER_OUTSIDE  = (-9);	// resource manager doing work outside
								// global transaction
		/*
		 * Flag definitions for the RM switch
		 */
const uint TMNOFLAGS   =0x00000000; /* no RM features selected */
const uint TMREGISTER  =0x00000001; /* RM dynamically registers */
const uint TMNOMIGRATE =0x00000002; /* RM does not support association migration */
const uint TMUSEASYNC  =0x00000004; /* RM supports asynchronous operations */
/*
 * Flag definitions for xa_ and ax_ routines
 */
/* use TMNOFLAGS, defined above, when not specifying other flags */
const uint TMASYNC     =0x80000000; /* perform routine asynchronously */
const uint TMONEPHASE  =0x40000000; /* caller is using one-phase commit optimisation */
const uint TMFAIL      =0x20000000; /* dissociates caller and marks tx branch rollback-only */
const uint TMNOWAIT    =0x10000000; /* return if blocking condition exists */
const uint TMRESUME    =0x08000000; /* caller is resuming association with suspended tx branch */
const uint TMSUCCESS   =0x04000000; /* dissociate caller from tx branch */
const uint TMSUSPEND   =0x02000000; /* caller is suspending, not ending, association */
const uint TMSTARTRSCAN=0x01000000; /* start a recovery scan */
const uint TMENDRSCAN  =0x00800000; /* end a recovery scan */
const uint TMMULTIPLE  =0x00400000; /* wait for any asynchronous operation */
const uint TMJOIN      =0x00200000; /* caller is joining existing tx branch */
const uint TMMIGRATE   =0x00100000; /* caller intends to perform migration */


		/// <summary>
		/// XASwitch constructor.  Used to start tracing.
		/// </summary>
		public XASwitch()
		{
			title = "XaSwitch";
			trace = new Ingres.Utility.TracingToEventLog("XA");
		}


		/*
		** Name: xa_open  - Open an RMI, registered earlier with the TP system.
		**
		** Description: 
		**
		** Inputs:
		**      xa_info    - character string, contains the RMI open string.
		**      rmid       - RMI identifier, assigned by the TP system, unique to the
		**                   AS process w/which LIBQXA is linked in.
		**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
		**
		** Outputs:
		**	Returns:
		**          XA_OK          - normal execution.
		**          XAER_ASYNC     - if TMASYNC was specified. Currently, we don't
		**                           support TMASYNC (post-Ingres65 feature).
		**          XAER_RMERR     - various internal errors.
		**          XAER_INVAL     - illegal input arguments.
		**          XAER_PROTO     - if the RMI is in the wrong state. Enforces the 
		**                           state tables in the XA spec. 
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// xa_open is called by the transaction manager to initialize
		/// the resource manager.  This method is called first, before 
		/// any other xa_ methods are called.  xa_open is loaded and
		/// called at MSDTC enlistment.  It allows MSDTC to
		/// test that the connection parameters work while running under
		/// the MSDTC user identity and authorization.  If there is a
		/// problem, we want to know at enlistment time and not at
		/// a recovery situation.  MSDTC also calls xa_open
		/// when it wants to xa_recover and commit/rollback
		/// in-doubt transactions.  Unfortunately, these connections have
		/// different connection parms depending on the recover or 
		/// commit/rollback.  Therefore, we can't re-use the connection
		/// and we immediately close the connection after the successful
		/// test of the xa_open.
		/// </summary>
		/// <param name="xa_info">XA OpenString containing information to
		/// connect to the host and database.  Max length is 256 characters.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.  The TM passes this rmid on subsequent calls to XA
		/// routines to identify the RM.  This identifier remains constant
		/// until the TM in this thread closes the RM.</param>
		/// <param name="flags">TMASYNC if function to be performed async.
		/// TMNOFLAGS is no other flags are set.</param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		public int xa_open    (
			[MarshalAs(UnmanagedType.LPStr)]
			string xa_info,
			int rmid,
			long flags)
		{
			int rc = XA_OK;
			int trace_level = 0;

			string sanitizedString;
			IConfig config;

			if (xa_info == null)
			{
				trace.write("xa_open: xa_info = <null>"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid arguments were specified
			}

			if (xa_info.Length == 0)
			{
				trace.write("xa_open: xid = <Empty String>"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid arguments were specified
			}

			try
			{
				config = ConnectStringConfig.ParseConnectionString(
					xa_info, out sanitizedString, true);
			}
			catch (Exception)
			{
				trace.write("xa_open: xa_info = <invalid connection string>"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid arguments were specified
			}

			// propagate the XA tracing from the application process
			// to this MSDTC process and send output to event log.
			string    strValue;
			// Check for XA tracing
			strValue = config.get(DrvConst.DRV_PROP_TRACE);
			if (strValue != null  &&  strValue.ToUpper(
				System.Globalization.CultureInfo.InvariantCulture)=="XA")
			{
				strValue = config.get(DrvConst.DRV_PROP_TRACELVL);
				if (strValue != null)
				{
					try
					{
						trace_level = Int32.Parse(strValue);
						if (trace_level < 0)
							trace_level = 0;
					}
					catch {}
				}
			}

			if (trace_level >= 3)  // until tracing is setup, do this manually
			{
				System.Text.StringBuilder sb = new System.Text.StringBuilder(100);

				sb.Append("xa_info = ");
				sb.Append("\"");
				sb.Append(sanitizedString);

				sb.Append(", rmid = ");
				sb.Append(rmid);

				sb.Append(", flags = ");
				sb.Append(XAFlagsToString(flags));

				trace.write("xa_open: " + sb.ToString());
			}

			string host;

			host = config.get(DrvConst.DRV_PROP_HOST);
			if (host == null)  // default to local
				host = "";

			if (host == null)
			{
				trace.write("xa_open: Error: host specification missing"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid connection string was specified
			}

			// if not "servername:port" format then
			// get the port number from keyword=value NameValueCollection
			if (host.IndexOf(":") == -1)
			{
				string port = config.get(DrvConst.DRV_PROP_PORT);
				if (port == null || port.Length == 0)
					port = "II7"; // no "Port=" then default to "II7"
				host += ":" + port.ToString();  // add ":portid" to host
			}

			AdvanConnect conn;
			try
			{
				conn =
					new AdvanConnect(null, host, config, trace, (AdvanXID) null);
			}
			catch (Exception ex)
			{
				if (trace_level >= 1) // until tracing is setup, do this manually
				trace.write("xa_open: Exception thrown on Connection open: "+
					ex.ToString());
				return XAER_RMERR;  // could not connect
			}

			RM rm = new RM(rmid, conn, host, config, trace_level);
			lock(RMcollection)
			{
				RMcollection[rmid] = rm;

				if (trace_level > 0)  // if tracing requested, turn if on
				{
					int old_trace_level = trace.setTraceLevel(trace_level);
					if (old_trace_level > trace_level)  // if old > new
						trace.setTraceLevel(old_trace_level);  // set it back
					CountOfRMsTracing++;
				}
			}
			try
			{
				// we don't need this connection, close it.  We just needed
				// to test that it could open and get its values for later.
				// We don't want to find out that the connection string
				// is bad for the MSDTC user context at real recovery.
				conn.close();   // we don't need this connection, close it.
			}
			catch (Exception /* ignore any close errors*/) 
			{}

			return rc;
		}

		/*
		** Name: xa_close   - Close an RMI.
		**
		** Description: 
		**
		** Inputs:
		**      xa_info    - character string, contains the RMI open string. Currently,
		**                   we will ignore this argument (Ingres65.)
		**      rmid       - RMI identifier, assigned by the TP system, unique to the 
		**                   AS process w/which LIBQXA is linked in.
		**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
		**
		** Outputs:
		**	Returns:
		**          XA_OK          - normal execution.
		**          XAER_ASYNC     - if TMASYNC was specified. We don't support
		**                           TMASYNC (post-Ingres65 feature).
		**          XAER_RMERR     - internal error w/gca association mgmt.
		**          XAER_INVAL     - illegal status argument.
		**          XAER_PROTO     - if the RMI/thread is in the wrong state for this 
		**                           call. Enforces the state table rules in the spec.
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// Close the resource manager.
		/// </summary>
		/// <param name="xa_info">XA OpenString containing information to
		/// connect to the host and database.  Max length is 256 characters.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.  The TM passes this rmid on subsequent calls to XA
		/// routines to identify the RM.  This identifier remains constant
		/// until the TM in this thread closes the RM.</param>
		/// <param name="flags">TMASYNC if function to be performed async.
		/// TMNOFLAGS is no other flags are set.</param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		public int xa_close   (
			[MarshalAs(UnmanagedType.LPStr)]
			string xa_info,
			int rmid,
			long flags)
		{
			if (trace.enabled(3))
			{
				trace.write("xa_close: rmid = " +
					rmid.ToString() + ", flags = " + XAFlagsToString(flags));
			}
			RM rm = FindRM(rmid);
			if (rm == null)  // if RM not found, just ignore the close
				return XA_OK;

			lock(RMcollection)
			{
				RMcollection.Remove(rmid);  // remove the RM from collection

				if (rm.Trace_level > 0)          // if we were tracing this RM
					if(--CountOfRMsTracing <= 0) // and no more RMs are tracing
						trace.setTraceLevel(0);  // turn off tracing
			}

			return XA_OK;
		}


		/*
		** Name: xa_start   -  Start/Resume binding a thread w/an XID.
		**
		** Description: 
		**
		** Inputs:
		**      xid        - pointer to XID.
		**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
		**                   process.
		**      flags      - TP system-specified flags, one of
		**                       TMJOIN/TMRESUME/TMNOWAIT/TMASYNC/TMNOFLAGS.
		**
		** Outputs:
		**	Returns:
		**          XA_OK          - normal execution.
		**          XA_RETRY       - if TMNOWAIT is set in flags. We will *always* 
		**                           assume a blocking call and return this (Ingres65).
		**          XA_RB*         - if this XID is marked "rollback only".
		**             XA_RBROLLBACK - rollback only, unspecified reason.  
		**             XA_RBCOMMFAIL - on any error on a GCA call.         
		**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
		**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
		**             XA_RBOTHER    - any other reason for rollback.
		**             XA_RBPROTO    - protocol error, this XA call made in wrong state
		**                             for the thread/RMI.
		**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
		**             XA_RBTRANSIENT- a transient error was detected by the DBMS svr.
		**          XAER_ASYNC     - if TMASYNC was specified. We don't support
		**                           TMASYNC (post-Ingres65 feature).
		**          XAER_RMERR     - internal error w/gca association mgmt.
		**          XAER_RMFAIL    - RMI made unavailable by some error.
		**          XAER_DUPID     - XID already exists, when it's not expected to be
		**                           "known" at this AS, i.e. it's not TMRESUME/TMJOIN.
		**          XAER_OUTSIDE   - RMI/thread is doing work "outside" of global xns.
		**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
		**          XAER_INVAL     - illegal input arguments.
		**          XAER_PROTO     - if there are active/suspended XIDs bound to
		**                           the RMI. Enforces XA Spec's "state table" rules.
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// Start work on behalf of a transaction branch.
		/// </summary>
		/// <param name="xid">XID to perform the work on.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.</param>
		/// <param name="flags"></param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		public int xa_start(
			AdvanXID xid,
			int rmid,
			long flags)
		{
			if (xid == null)
			{
				trace.write("xa_start: xid = <null>"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid arguments were specified
			}

			if (trace.enabled(3))
			{
				trace.write("xa_start: xid = " + xid.ToString() +
					", rmid = " + rmid.ToString() + 
					", flags = " + XAFlagsToString(flags));
			}

			return XAER_PROTO;  // unexpected context
		}


		/*
		** Name: xa_end   -  End/Suspend the binding of a thread w/an XID..
		**
		** Description: 
		**
		** Inputs:
		**      xid        - pointer to XID.
		**      rmid       - RMI identifier, assigned by the TP system, unique to the
		**                   AS process w/which LIBQXA is linked in.
		**      flags      - TP system-specified flags, one of
		**                       TMSUSPEND/TMMIGRATE/TMSUCCESS/TMFAIL/TMASYNC
		**
		** Outputs:
		**	Returns:
		**          XA_OK          - normal execution.
		**          XA_NOMIGRATE   - if TMSUSPEND|TMMIGRATE is specified. We won't
		**                          have support for Association Migration in Ingres65.
		**          XA_RB*         - if this XID is marked "rollback only".
		**             XA_RBROLLBACK - rollback only, unspecified reason.  
		**             XA_RBCOMMFAIL - on any error on a GCA call.         
		**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
		**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
		**             XA_RBOTHER    - any other reason for rollback.
		**             XA_RBPROTO    - protocol error, this XA call made in wrong state
		**                             for the thread/RMI.
		**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
		**             XA_RBTRANSIENT- a transient error was detected by the DBMS server
		**          XAER_ASYNC     - if TMASYNC was specified. We don't support
		**                           TMASYNC (post-Ingres65 feature).
		**          XAER_RMERR     - internal error w/gca association mgmt.
		**          XAER_RMFAIL    - RMI made unavailable by some error.
		**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
		**          XAER_INVAL     - illegal status argument.
		**          XAER_PROTO     - if there are active/suspended XIDs bound to
		**                           the RMI. Enforces XA Spec's "state table" rules.
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// End the work on behalf of a transaction branch.
		/// </summary>
		/// <param name="xid">XID to perform the work on.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.</param>
		/// <param name="flags"></param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		public int xa_end   (
			AdvanXID xid,
			int rmid,
			long flags)
		{
			if (xid == null)
			{
				trace.write("xa_end: xid = <null>"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid arguments were specified
			}

			if (trace.enabled(3))
			{
				trace.write("xa_end: xid = " + xid.ToString() +
					", rmid = " + rmid.ToString() + 
					", flags = " + XAFlagsToString(flags));
			}

			return XAER_PROTO;  // unexpected context
		}


		/*
		** Name: xa_prepare   -  Prepare a local xn bound to an XID.
		**
		** Description: 
		**
		** Inputs:
		**      xid        - pointer to XID.
		**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
		**                   process w/which LIBQXA is linked in.
		**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
		**
		** Outputs:
		**	Returns:
		**          XA_OK          - normal execution.
		**          XA_RB*         - if this XID is marked "rollback only".
		**             XA_RBROLLBACK - rollback only, unspecified reason.  
		**             XA_RBCOMMFAIL - on any error on a GCA call.         
		**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
		**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
		**             XA_RBOTHER    - any other reason for rollback.
		**             XA_RBPROTO    - protocol error, this XA call made in wrong state
		**                             for the thread/RMI.
		**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
		**             XA_RBTRANSIENT- a transient error was detected by the DBMS server
		**          XAER_ASYNC     - if TMASYNC was specified. We don't support
		**                           TMASYNC (Ingres65).
		**          XAER_RMERR     - internal error w/gca association mgmt.
		**          XAER_RMFAIL    - RMI made unavailable by some error.
		**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
		**          XAER_INVAL     - illegal arguments.
		**          XAER_PROTO     - if there are active/suspended XIDs bound to
		**                           the RMI. Enforces XA Spec's "state table" rules.
		**
		**          UNSUPPORTED in Ingres65
		**          -----------------------
		**
		**          XA_RDONLY      - the Xn branch was read-only, and has been
		**                           committed.
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// Prepare-to-commit the work on behalf of a transaction branch.
		/// </summary>
		/// <param name="xid">XID to perform the work on.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.</param>
		/// <param name="flags"></param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		public int xa_prepare   (
			AdvanXID xid,
			int rmid,
			long flags)
		{
			if (xid == null)
			{
				trace.write("xa_prepare: xid = <null>"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid arguments were specified
			}

			if (trace.enabled(3))
			{
				trace.write("xa_prepare: xid = " + xid.ToString() +
					", rmid = " + rmid.ToString() + 
					", flags = " + XAFlagsToString(flags));
			}

			return XAER_PROTO;  // unexpected context
		}


		/*
		** Name: xa_commit   -  Commit a local xn bound to an XID.
		**
		** Description: 
		**
		** Inputs:
		**      xid        - pointer to XID.
		**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
		**                   process w/which LIBQXA is linked in.
		**      flags      - TP system-specified flags, one of
		**                          TMNOWAIT/TMASYNC/TMONEPHASE/TMNOFLAGS.
		**
		** Outputs:
		**	Returns (possible future):
		**          XA_OK          - normal execution.
		**          XA_RETRY       - if TMNOWAIT is set in flags.
		**          XA_RB*         - if this XID is marked "rollback only". These are
		**                           allowed *only* if this is a 1-phase Xn.
		**             XA_RBROLLBACK - rollback only, unspecified reason.  
		**             XA_RBCOMMFAIL - on any error on a GCA call.         
		**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
		**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
		**             XA_RBOTHER    - any other reason for rollback.
		**             XA_RBPROTO    - protocol error, this XA call made in wrong state
		**                             for the thread/RMI.
		**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
		**             XA_RBTRANSIENT- a transient error was detected by the DBMS server
		**          XAER_ASYNC     - if TMASYNC was specified. We don't support
		**                           TMASYNC (Ingres65).
		**          XAER_RMERR     - internal error w/gca association mgmt.
		**          XAER_RMFAIL    - RMI made unavailable by some error.
		**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
		**          XAER_INVAL     - illegal arguments.
		**          XAER_PROTO     - if there are active/suspended XIDs bound to
		**                           the RMI. Enforces XA Spec's "state table" rules.
		**
		**          UNSUPPORTED in Ingres65
		**          -----------------------
		**
		**          XA_HEURHAZ     - Work done in global xn branch may have been 
		**                           heuristically completed.
		**          XA_HEURCOM     - Work done may have been heuristically committed.
		**          XA_HEURRB      - Work done was heuristically rolled back.
		**          XA_HEURMIX     - Work done was partially committed and partially
		**                           rolled back heuristically.
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// Commit work on behalf of a transaction branch.
		/// </summary>
		/// <param name="xid">XID to perform the work on.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.</param>
		/// <param name="flags"></param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		public int xa_commit    (
			AdvanXID xid,
			int rmid,
			long flags)
		{
			if (xid == null)
			{
				trace.write("xa_commit: xid = <null>"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid arguments were specified
			}

			if (trace.enabled(3))
			{
				trace.write("xa_commit: xid = " + xid.ToString() +
					", rmid = " + rmid.ToString() + 
					", flags = " + XAFlagsToString(flags));
			}

			int rc = xa_commit_or_rollback(true, xid, rmid, flags);
			return rc;
		}  // xa_commit


		/*
		** Name: xa_rollback   -  Propagate rollback of a local xn bound to an XID.
		**
		** Description: 
		**
		** Inputs:
		**      xid        - pointer to XID.
		**      rmid       - RMI identifier, assigned by the TP system, unique to the
		**                   AS process w/which LIBQXA is linked in.
		**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
		**
		** Outputs:
		**	Returns:
		**          XA_OK          - normal execution.
		**          XA_RB*         - if this XID is marked "rollback only".
		**             XA_RBROLLBACK - rollback only, unspecified reason.  
		**             XA_RBCOMMFAIL - on any error on a GCA call.         
		**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
		**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
		**             XA_RBOTHER    - any other reason for rollback.
		**             XA_RBPROTO    - protocol error, this XA call made in wrong state
		**                             for the thread/RMI.
		**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
		**             XA_RBTRANSIENT- a transient error was detected by the DBMS 
		**                             server.
		**          XAER_ASYNC     - if TMASYNC was specified. We don't support
		**                           TMASYNC (Ingres65).
		**          XAER_RMERR     - internal error w/gca association mgmt.
		**          XAER_RMFAIL    - RMI made unavailable by some error.
		**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
		**          XAER_INVAL     - illegal arguments.
		**          XAER_PROTO     - if there are active/suspended XIDs bound to
		**                           the RMI. Enforces XA Spec's "state table" rules.
		**
		**          UNSUPPORTED in Ingres65
		**          -----------------------
		**
		**          XA_HEURHAZ     - Work done in global xn branch may have been 
		**                           heuristically completed.
		**          XA_HEURCOM     - Work done may have been heuristically committed.
		**          XA_HEURRB      - Work done was heuristically rolled back.
		**          XA_HEURMIX     - Work done was partially committed and partially
		**                           rolled back heuristically.
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// Roll back the work on behalf of a transaction branch.
		/// </summary>
		/// <param name="xid">XID to perform the work on.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.</param>
		/// <param name="flags"></param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		public int xa_rollback    (
			AdvanXID xid,
			int rmid,
			long flags)
		{
			if (xid == null)
			{
				trace.write("xa_rollback: xid = <null>"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid arguments were specified
			}

			if (trace.enabled(3))
			{
				trace.write("xa_rollback: xid = " + xid.ToString() +
					", rmid = " + rmid.ToString() + 
					", flags = " + XAFlagsToString(flags));
			}

			int rc = xa_commit_or_rollback(false, xid, rmid, flags);
			return rc;
		}  // xa_rollback


		/// <summary>
		/// Common code for xa_commit and xa_rollback
		/// </summary>
		/// <param name="isCommit">If commit then true, else rollback.</param>
		/// <param name="xid">XID to perform the work on.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.</param>
		/// <param name="flags"></param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		private int xa_commit_or_rollback    (
			bool     isCommit,
			AdvanXID xid,
			int rmid,
			long flags)
		{
			string title = isCommit?"xa_commit":"xa_rollback";

			RM rm = FindRM(rmid);
			if (rm == null)  // if RM not found, error
				return XAER_INVAL;

			AdvanConnect conn;
			try
			{
				conn =
					new AdvanConnect(
						null, rm.Host, rm.Config, trace, xid);
			}
			catch (Exception ex)
			{
				trace.write(title+": Exception thrown on Connection open: "+
					ex.ToString());
				return XAER_RMERR;  // could not connect
			}

			try
			{
				if (isCommit)
					conn.commit();
				else
					conn.rollback();
			}
			catch (Exception ex)
			{
				trace.write(title+": Exception thrown on "+
					(isCommit?"commit()":"rollback()") + ": " +
					ex.ToString());
				return XAER_RMERR;  // could not connect
			}

			try
			{
				conn.close();
			}
			catch
			{}

			return XA_OK;
		}


		/*
		** Name: xa_recover   -  Recover a set of prepared XIDs for a specific RMI.
		**
		** Description: 
		**
		** Inputs:
		**      xids       - pointer to XID list.
		**      count      - maximum number of XIDs expected by the TP system.
		**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
		**                   process w/which LIBQXA is linked in.
		**      flags      - TP system-specified flags, one of
		**                          TMSTARTRSCAN/TMENDRSCAN/TMNOFLAGS.
		**
		** Outputs:
		**	Returns:
		**          list of AdvanXID's or an empty list
		**	XaSwitch.xa_recover will return
		**          >= 0           - number of XIDs returned.
		**          XAER_RMERR     - internal error w/gca association mgmt.
		**          XAER_RMFAIL    - RMI made unavailable by some error.
		**          XAER_INVAL     - illegal arguments.
		**          XAER_PROTO     - if there are active/suspended XIDs bound to
		**                           the RMI. Enforces XA Spec's "state table" rules.
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// Obtain a list of prepared transaction branches from a resource manager.
		/// </summary>
		/// <param name="xids">Output area to place XIDs.</param>
		/// <param name="count">Capacity of output area hold XIDs.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.</param>
		/// <param name="flags"></param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		public int xa_recover    (
			out AdvanXID[] xids,
			long count,
			int rmid,
			long flags)
		{
			int rc = XA_OK;

			trace.write(3, "xa_recover: count = " + count.ToString() +
				", rmid = " + rmid.ToString() + 
				", flags = " + XAFlagsToString(flags));

			xids = new AdvanXID[0];  // init to empty list

			RM rm = FindRM(rmid);
			if (rm == null)  // if RM not found, error
				return XAER_INVAL;

			if ((flags & TMSTARTRSCAN) != 0)  // get a brand new list of XIDs
			{
				rc = xa_recover_startrscan(rm);
				if (rc <= 0)      // return count==0 or error of some kind
					return rc;
				// else fall through to return sublist
			}

			AdvanXID[] newxids = rm.NextXids(count); // get next, bump up cursor
			if (newxids.Length > 0)  // if some were found, return list
				xids = newxids;

			return xids.Length;
		}  // xa_recover


		/// <summary>
		/// Start the recovery scan for XIDs and position cursor at start of list.
		/// </summary>
		/// <param name="rm">Resource manager to start scan for.</param>
		/// <returns>Count of XIDs in whole list or XAER* error code.</returns>
		private int xa_recover_startrscan(RM rm)
		{
			const string MASTER_DB = "iidbdb";

			int rc = XA_OK;
			rm.Xids = new AdvanXID[0];  // init to empty in case bad things happen
			AdvanXID[] xids = rm.Xids;

			AdvanConnect conn;

			ConnectStringConfig config =  // make a copy
				new ConnectStringConfig(rm.Config);

			// XIDs need to come from iidbdb database
			config.Set(DrvConst.DRV_PROP_DB, MASTER_DB);

			try
			{
				conn =
					new AdvanConnect(
					null, rm.Host, config, trace, (AdvanXID)null);
			}
			catch (Exception ex)
			{
				trace.write("xa_recover: Exception thrown on Connection open: "+
					ex.ToString());
				return XAER_RMFAIL;  // could not connect
			}

			try
			{
				xids = conn.getPreparedTransactionIDs(rm.DatabaseName);
			}
			catch (Exception ex)
			{
				trace.write("xa_recover: Exception thrown on GetXIDs: "+
					ex.ToString());
				rc = XAER_RMERR;  // could not connect
			}
			finally
			{
				try
				{
					conn.close();
				}
				catch(Exception /* ignore */) {}
			}

			if (rc != XA_OK)
				return rc;

			rm.Cursor =0;      // reset cursor to XID list back to start
			if (xids != null)  // safety check
				rm.Xids = xids;  // new list of XIDs

			return rm.Xids.Length;
		}  // xa_recover_startrscan


		/*
		** Name: xa_forget   -  Forget a heuristically completed xn branch.
		**
		** Description: 
		**        Unexpected !
		**
		** Inputs:
		**      xid        - pointer to XID.
		**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
		**                   process w/which LIBQXA is linked in.
		**      flags      - TP system-specified flags, one of
		**                          TMASYNC/TMNOFLAGS.
		**
		** Outputs:
		**	Returns:
		**          XA_OK          - normal execution.
		**          XAER_ASYNC     - if TMASYNC was specified. We don't support
		**                           TMASYNC (Ingres65).
		**          XAER_RMERR     - internal error w/gca association mgmt.
		**          XAER_RMFAIL    - RMI made unavailable by some error.
		**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
		**          XAER_INVAL     - illegal arguments.
		**          XAER_PROTO     - if there are active/suspended XIDs bound to
		**                           the RMI. Enforces XA Spec's "state table" rules.
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// Forget about a heurististically completed transaction branch.
		/// </summary>
		/// <param name="xid">XID to perform the work on.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.</param>
		/// <param name="flags"></param>
		/// <returns>XA_OK for normal execution or an XAER* error code.</returns>
		public int xa_forget    (
			AdvanXID xid,
			int rmid,
			long flags)
		{
			if (xid == null)
			{
				trace.write("xa_forget: xid = <null>"+
					", rmid = " + rmid.ToString());
				return XAER_INVAL;  // invalid arguments were specified
			}

			if (trace.enabled(3))
			{
				trace.write("xa_forget: xid = " + xid.ToString() +
					", rmid = " + rmid.ToString() + 
					", flags = " + XAFlagsToString(flags));
			}

			return XAER_PROTO;  // unexpected context
		}


		/*
		** Name: xa_complete   -  Wait for an asynchronous operation to complete.
		**
		** Description: 
		**        UNSUPPORTED IN Ingres65 - Hence unexpected !
		**
		** Inputs:
		**      handle     - handle returned in a previous XA call.
		**      retval     - pointer to the return value of the previously activated
		**                   XA call.
		**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
		**                   process w/which LIBQXA is linked in.
		**      flags      - TP system-specified flags, one of
		**                          TMMULTIPLE/TMNOWAIT/TMNOFLAGS.
		**
		**  Outputs:
		**	Returns:
		**          XA_RETRY       - TMNOWAIT was set in flags, no asynch ops completed.
		**          XA_OK          - normal execution.
		**          XAER_INVAL     - illegal arguments.
		**          XAER_PROTO     - if there are active/suspended xns bound to
		**                           the RMI. Enforces XA Spec's "state table" rules.
		**
		** History:
		**	18-Aug-03 (thoda04)
		**	    Created.  Rich description copied from front/embed/libqxa.
		*/

		/// <summary>
		/// Wait for asynchronous operation to complete.
		/// </summary>
		/// <param name="handle">Handle returned in a previous XA call.</param>
		/// <param name="retval">Pointer to the return value of the
		/// previously activated XA call.</param>
		/// <param name="rmid">An integer, assigned by the transaction manager,
		/// uniquely identifies the called resource manager within the thread
		/// of control.</param>
		/// <param name="flags">TP system-specified flags, one of
		/// TMMULTIPLE/TMNOWAIT/TMNOFLAGS.</param>
		/// <returns></returns>
		public int xa_complete  (
			ref int handle,
			ref int retval,
			int rmid,
			long flags)
		{
			if (trace.enabled(3))
			{
				trace.write("xa_complete: rmid = " + rmid.ToString() + 
					", flags = " + XAFlagsToString(flags));
			}

			return XAER_PROTO;  // unexpected context
		}


		/// <summary>
		/// Convert the XA error status to a readable string for reporting.
		/// </summary>
		/// <param name="status">One of the XAER_xxx values.</param>
		/// <returns>The description text of the XAER_xxx value.</returns>
		internal static string XAER_ToString(int status)
		{
			string message = "XA error ";

			switch (status)
			{
				case XASwitch.XAER_ASYNC:
					message +=
						"XAER_ASYNC (asynchronous operation already outstanding)";
					break;
				case XASwitch.XAER_RMERR:
					message +=
						"XAER_RMERR (a resource manager error occurred in the tx branch)";
					break;
				case XASwitch.XAER_NOTA:
					message +=
						"XAER_NOTA (the XID is not valid)";
					break;
				case XASwitch.XAER_INVAL:
					message +=
						"XAER_INVAL (invalid arguments were given)";
					break;
				case XASwitch.XAER_PROTO:
					message +=
						"XAER_PROTO (routine invoked in an improper context)";
					break;
				case XASwitch.XAER_RMFAIL:
					message +=
						"XAER_RMFAIL (resource manager unavailable)";
					break;
				case XASwitch.XAER_DUPID:
					message +=
						"XAER_DUPID (the XID already exists)";
					break;
				case XASwitch.XAER_OUTSIDE:
					message +=
						"XAER_OUTSIDE (resource manager doing work outside the global transaction)";
					break;
				default:
					message += "code = " + status.ToString();
					break;
			}
			return message;
		}


		private string XAFlagsToString(long _flags)
		{
			ulong flags = unchecked((ulong)_flags);

			if (flags == TMNOFLAGS)
				return "TMNOFLAGS";

			System.Text.StringBuilder sb = new System.Text.StringBuilder(100);

			flags = XAFlagToString(flags, sb, "TMREGISTER",  TMREGISTER);
			flags = XAFlagToString(flags, sb, "TMNOMIGRATE", TMNOMIGRATE);
			flags = XAFlagToString(flags, sb, "TMUSEASYNC",  TMUSEASYNC);
			flags = XAFlagToString(flags, sb, "TMASYNC",     TMASYNC);
			if (flags == 0)
				goto shortcut;
			flags = XAFlagToString(flags, sb, "TMONEPHASE",  TMONEPHASE);
			flags = XAFlagToString(flags, sb, "TMFAIL",      TMFAIL);
			flags = XAFlagToString(flags, sb, "TMNOWAIT",    TMNOWAIT);
			flags = XAFlagToString(flags, sb, "TMRESUME",    TMRESUME);
			flags = XAFlagToString(flags, sb, "TMSUCCESS",   TMSUCCESS);
			flags = XAFlagToString(flags, sb, "TMSUSPEND",   TMSUSPEND);
			flags = XAFlagToString(flags, sb, "TMSTARTRSCAN",TMSTARTRSCAN);
			flags = XAFlagToString(flags, sb, "TMENDRSCAN",  TMENDRSCAN);
			flags = XAFlagToString(flags, sb, "TMMULTIPLE",  TMMULTIPLE);
			flags = XAFlagToString(flags, sb, "TMJOIN",      TMJOIN);
			flags = XAFlagToString(flags, sb, "TMMIGRATE",   TMMIGRATE);

			if (flags != 0)
			{
				string key = "0X" + flags.ToString("X8");
				XAFlagToString(flags, sb, key, flags);  // pick up stragglers
			}

			shortcut:
				if (sb.Length >= 3)  // safety check
					sb.Length -= 3;  // trim the trailing " + " char

			return sb.ToString();
		}

		private ulong XAFlagToString(
			ulong flags, System.Text.StringBuilder sb, string key, ulong flag)
		{
			if ((flags & flag) != 0)
			{
				flags ^= flag;
				sb.Append(key);
				sb.Append(" + ");
			}
			return flags;
		}

		private RM FindRM(int rmid)
		{
			object obj = RMcollection[rmid];
			if (obj == null)
				return null;
			return (RM)obj;
		}


		/// <summary>
		/// Resource Manager class that hold the association context.
		/// </summary>
		private class RM
		{
			/// <summary>
			/// Constructor for Resource Manager class.
			/// </summary>
			/// <param name="rmid"></param>
			/// <param name="conn"></param>
			/// <param name="host"></param>
			/// <param name="config"></param>
			/// <param name="trace_level"></param>
			public RM(int rmid, AdvanConnect conn, string host, 
				IConfig config,
				int trace_level)
			{
				this._conn         = conn;
				this._config           = config;
				this._host         = conn.host;
				this._databaseName = conn.database;
				this._trace_level  = trace_level;
			}

			private AdvanConnect _conn;
			public  AdvanConnect AdvanConnect
			{
				get {return _conn;}
			}

			private IConfig _config;
			public  IConfig  Config
			{
				get {return _config;}
			}

			private string _databaseName;
			public  string  DatabaseName
			{
				get {return _databaseName;}
			}

			private string _host;
			public  string  Host
			{
				get {return _host;}
			}

			private int _cursor = 0;
			public  int  Cursor
			{
				get {return _cursor;}
				set {_cursor = value;}
			}

			private AdvanXID[] _xids = new AdvanXID[0];
			public  AdvanXID[]  Xids
			{
				get {return _xids;}
				set {_xids = value;}
			}

			private int _trace_level = 0;
			public  int  Trace_level
			{
				get {return _trace_level;}
				set {_trace_level = value;}
			}

			/// <summary>
			/// Return the next batch of XIDs in the list.
			/// </summary>
			/// <param name="_count">Max number of XIDs requested.</param>
			/// <returns>An array of AdvanXID containing the XIDs.</returns>
			public AdvanXID[] NextXids(long _count)
			{
				int count = (int)_count;
				int CountRemaining = Xids.Length - Cursor;

				if (count > CountRemaining)
					count = CountRemaining;

				if (count <= 0)
					return new AdvanXID[0];

				AdvanXID[] newxids = new AdvanXID[count];
				int i, j;
				for (i = Cursor, j = 0; i < count;)
					newxids[j++] = Xids[i++];

				Cursor = count;  // bump up the Xid list cursor
				return newxids;
			}

		}  // class RM
	}  // end class XASwitch
}
