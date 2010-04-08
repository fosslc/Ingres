/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;


namespace Ingres.ProviderInternals
{
	/*
	** Name: RsltByref.cs
	**
	** Description:
	**	Defines class which implements the ResultSet interface
	**	and supports Database Procedure BYREF parameter results.
	**
	**  Classes
	**
	**	RsltByref
	**
	** History:
	**	17-May-00 (gordy)
	**	    Created.
	**	 4-Oct-00 (gordy)
	**	    Standardized internal tracing.
	**	 3-Nov-00 (gordy)
	**	    Added support for 2.0 extensions.
	**	23-Jan-01 (gordy)
	**	    Changed parameter type to AdvanStmt for backward compatibility.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.  Extracted Byref specifics 
	**	    into separate sub-class.
	**	 4-Aug-03 (gordy)
	**	    Pre-fetching now disabled since only one row expected.
	**	 6-Oct-03 (gordy)
	**	    Added pre-loading ability to result-sets.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/


	/*
	** Name: RsltByref
	**
	** Description:
	**	driver class which implements ResultSet interface
	**	and supports Database Procedure BYREF parameter results.
	**
	**	Most of the functionality for this class is inherited
	**	from the super-classes, especially AdvanRslt and RsltFtch.  
	**
	**	If the application specifies OUT parameters, a single data 
	**	row containing the BYREF parameter values is returned. This 
	**	row is retrieved and cached by the class constructor and 
	**	the server statement representing the procedure is closed.  
	**	The result-set pre-fetch and max row count values are 
	**	carefully chosen so that the super classes will check for 
	**	additional rows and the end-of-data indication will be noted.
	**
	**	The only other considerations for procedure and parameter
	**	results are connection locking and the procedure return
	**	value.  Locking is handled by the closeCursor() method of
	**	the RsltSlct class.  The procedure return value is handled 
	**	by the RsltProc class.
	**
	** History:
	**	 3-Aug-00 (gordy)
	**	    Created.
	**	31-Oct-02 (gordy)
	**	    Extracted Byref specifics into separate sub-class.
	**	 6-Oct-03 (gordy)
	**	    Changed super-class (RsltProc obsolete).
	*/

	class
		RsltByref : RsltSlct
	{

		/*
		** Name: RsltByref
		**
		** Description:
		**	Class constructor for Database Procedure BYREF parameters.
		**	Extends procedure row handling to load the single expected
		**	result row and release the server side resources.  Initial 
		**	row cache may be pre-loaded, but only if the message stream 
		**	is active and DATA message available.
		**
		** Input:
		**	conn		Associated connection.
		**	stmt		Associated call statement.
		**	rsmd		Result set Meta-Data.
		**	stmt_id		Statement ID.
		**	preLoad		Load initial row cache.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 3-Aug-00 (gordy)
		**	    Created.
		**	 4-Oct-00 (gordy)
		**	    Create unique ID for standardized internal tracing.
		**	 3-Nov-00 (gordy)
		**	    Changed parameters for 2.0 extensions.
		**	23-Jan-01 (gordy)
		**	    Changed parameter type to AdvanStmt for backward compatibility.
		**	31-Oct-02 (gordy)
		**	    Extracted Byref specifics into separate sub-class.
		**	 4-Aug-03 (gordy)
		**	    Server will handle details of detecting EOD and cleaning
		**	    up the connection, so we disable pre-fetch since only one
		**	    row is expected.
		**	 6-Oct-03 (gordy)
		**	    Added preLoad parameter to read initial row cache from server.
		*/

		public
			RsltByref
			( 
			DrvConn	conn, 
			AdvanCall	stmt,
			AdvanRSMD	rsmd,
			long	stmt_id,
			bool	do_preLoad
			) : base( conn, stmt, rsmd, stmt_id, 1, false )
			/*
			** BYREF parameters produce a single result row.
			** Even if row may be pre-loaded, we don't want
			** the super-class to do the loading.
			*/
		{
			rs_max_rows = 1;
			tr_id = "Byref[" + inst_id + "]";

			/*
			** Pre-load the row if available, otherwise disable pre-
			** fetching to better handle the single expected row
			** (pre-loading requires pre-fetch to be enabled).
			*/
			if ( do_preLoad )
				preLoad();
			else
				disablePreFetch();

			/*
			** Load the single expected row and close the
			** server statement to unlock the connection.
			*/
			try 
			{ 
				if ( ! next() )  throw SqlEx.get( ERR_GC4002_PROTOCOL_ERR ); 
			}
			catch( SqlEx ) { throw; }
			finally 
			{ 
				try { closeCursor(); } 
				catch( SqlEx ) {} 
			}
			return;
		} // RsltByref


	} // class RsltByref
}  // namespace
