/*
** Copyright (c) 1999, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: rsltupd.cs
	**
	** Description:
	**	Defines class which implements the ResultSet interface
	**	and supports updatable cursors.
	**
	**  Classes
	**
	**	RsltUpd
	**
	** History:
	**	 4-Aug-03 (gordy)
	**	    Created.
	**	 6-Oct-03 (gordy)
	**	    Added pre-loading ability to result-sets.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/


	/*
	** Name: RsltUpd
	**
	** Description:
	**	Driver class which implements the ResultSet interface
	**	and adds support for updatable cursors.
	**
	** History:
	**	 4-Aug-03 (gordy)
	**	    Created.
	*/

	class RsltUpd : RsltCurs
	{


		/*
		** Name: RsltUpd
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	conn		Associated connection.
		**	stmt		Associated statement.
		**	rsmd		ResultSet meta-data.
		**	stmt_id		Statement ID.
		**	cursor		Cursor name.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	 4-Aug-03 (gordy)
		**	    Created.
		**	 6-Oct-03 (gordy)
		**	    Disable pre-loading in super-class.
		*/

		public
			RsltUpd
			( 
			DrvConn  	conn,
			AdvanStmt	stmt,
			AdvanRSMD	rsmd,
			long     	stmt_id,
			String   	cursor
			) : base( conn, stmt, rsmd, stmt_id, cursor, 1, false )
		{
			/*
			** Updatable cursors only permit a single row per fetch,
			** so pre-fetching and pre-loading is disabled.
			*/
			tr_id = "Upd[" + inst_id + "]";
			disablePreFetch();
			return;
		} // RsltUpd


	} // class RsltUpd

}