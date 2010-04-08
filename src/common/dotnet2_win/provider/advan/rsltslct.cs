/*
** Copyright (c) 1999, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: rsltslct.cs
	**
	** Description:
	**	Defines class which implements the ResultSet interface
	**	extending server fetching for select-loop processing.
	**
	**  Classes
	**
	**	RsltSlct
	**
	** History:
	**	17-May-00 (gordy)
	**	    Created.
	**	 4-Oct-00 (gordy)
	**	    Standardized internal tracing.
	**	27-Oct-00 (gordy)
	**	    New super class which doesn't enforce connection
	**	    locking or cursor oriented operations.
	**	15-Nov-00 (gordy)
	**	    Added support for 2.0 extensions.
	**	23-Jan-01 (gordy)
	**	    Changed parameter type to AdvanStmt for backward compatibility.
	**	31-Jan-01 (gordy)
	**	    Use methods to handle SQLWarning.
	**	28-Mar-01 (gordy)
	**	    Combined functionality of close() into shut(), which is
	**	    replaced by closeCursor().
	**	 5-Sep-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	 6-Oct-03 (gordy)
	**	    Added pre-loading ability to result-sets.
	**	 4-Apr-04 (thoda04)
	**	    In closeCursor, don't throw original exception if it is null.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/



	/*
	** Name: RsltSlct
	**
	** Description:
	**	driver class which implements the ResultSet interface
	**	as an extension of server row fetching by adding support for
	**	select-loop processing.
	**
	**	Most of the functionality for this class is inherited from the 
	**	super-classes, especially the direct super-class RsltFtch.  The 
	**	only significant extension involves connection locking.  For 
	**	select-loops, the connection is locked from the start of the 
	**	query until the result set is closed.  The connection should
	**	not be unlocked after issuing the SELECT query.
	**
	**	Although select loops produce a continuous data stream from the 
	**	DBMS server, result-set data is still handled on a fetch request 
	**	basis between the driver and server.  The server hides the 
	**	difference between cursor fetches and select-loop tuple streams.
	**
	**  Overridden Methods:
	**
	**	closeCursor	    Close simulated cursor.
	**
	** History:
	**	17-May-00 (gordy)
	**	    Created.
	**	27-Oct-00 (gordy)
	**	    New super class which doesn't enforce connection
	**	    locking or cursor oriented operations.
	**	28-Mar-01 (gordy)
	**	    Rename shut() to closeCursor().
	*/

	class
		RsltSlct : RsltFtch
	{


		/*
		** Name: RsltSlct
		**
		** Description:
		**	Class constructor.
		**
		**	Initial row cache may be pre-loaded, but only if the
		**	message stream is active and DATA messages available.
		**
		** Input:
		**	conn		Associated connection.
		**	stmt		Associated statement.
		**	rsmd		ResultSet meta-data.
		**	stmt_id		Statement ID.
		**	preFetch	Pre-fetch row count.
		**	preLoad		Load initial row cache.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	17-May-00 (gordy)
		**	    Created.
		**	 4-Oct-00 (gordy)
		**	    Create unique ID for standardized internal tracing.
		**	27-Oct-00 (gordy)
		**	    New super-class constructor doesn't take cursor name.
		**	15-Nov-00 (gordy)
		**	    Changed parameters for 2.0 support.
		**	23-Jan-01 (gordy)
		**	    Changed parameter type to AdvanStmt for backward compatibility.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		**	 6-Oct-03 (gordy)
		**	    Added preLoad parameter to read initial row cache from server.
		*/

		public
			RsltSlct
			( 
			DrvConn	conn,
			AdvanStmt	stmt, 
			AdvanRSMD	rsmd, 
			long	stmt_id,
			int		preFetch,
			bool	do_preLoad ) : base( conn, stmt, rsmd, stmt_id, preFetch )
		{
			tr_id = "Slct[" + inst_id + "]";
    
			/*
			** Pre-load row cache if requested.
			*/
			if ( do_preLoad  &&  preLoad() )  
			{
				/* 
				** Close the statement when end-of-data detected.
				**
				** Note that our caller only locks the connection,
				** and expects the connection to be unlocked when
				** data stream is done.  Our closeCursor() method
				** will unlock the connection as needed.
				*/
				try { closeCursor(); }  
				catch( SqlEx ) {}
			}
    
			return;
		} // RsltSlct


		/*
		** Name: closeCursor
		**
		** Description:
		**	Close the select loop.  The super-class implementation of
		**	this method is used to perform the close.  The connection 
		**	is unlocked when the select-loop has been closed.
		**
		**	Closing the select loop may require cancelling the query.  
		**	Its not really an error, but the info may be useful to the 
		**	client, so the error is translated into a warning message.
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
		**	17-May-00 (gordy)
		**	    Created.
		**	31-Jan-01 (gordy)
		**	    Use methods to handle SQLWarning.
		**	28-Mar-01 (gordy)
		**	    Renamed to closeCursor().  Field closed renamed to cursorClosed.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		**	 4-Apr-04 (thoda04)
		**	    Don't throw original exception if it is null.
		*/

		protected internal override void
			closeCursor()
		{
			/*
			** If already closed then already unlocked
			** and there is nothing to do.
			*/
			if ( cursorClosed )  return;

			try { base.closeCursor(); }
			catch( SqlEx ex )
			{
				/*
				** Translate query cancellation error to a warning.
				*/
				if ( ex.getErrorCode() == E_AP0009_QUERY_CANCELLED )
				{
					setWarning( ex );
					Exception origex = ex.InnerException;	// Check for other errors.
					if (origex != null)
						throw origex;
				}
				else
					throw;  // Original or next exception
			}
			finally 
			{ 
				msg.UnlockConnection();	// Unlock the connection.
			}

			return;
		} // closeCursor


	} // class RsltSlct

}