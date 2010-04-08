/*
** Copyright (c) 1999, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: rsltdata.cs
	**
	** Description:
	**	Implements the class RsltData for handling fixed data sets.
	**
	** History:
	**	20-May-99 (gordy)
	**	    Created.
	**	12-Nov-99 (rajus01)
	**	    Implemented.
	**	 9-May-00 (gordy)
	**	    Reworked result set classes.  Extracted into own source file.
	**	 4-Oct-00 (gordy)
	**	    Standardized internal tracing.
	**	 3-Nov-00 (gordy)
	**	    Added support for 2.0 extensions.
	**	28-Mar-01 (gordy)
	**	    Tracing no longer globally accessible.
	**	    Combined close() and shut().
	**	 5-Sep-02 (thoda04)
	**	    Ported for .NET Provider.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	 4-Aug-03 (gordy)
	**	    Moved row cacheing out of AdvanRslt.
	**	26-Sep-03 (gordy)
	**	    Column data now stored as SqlData objects.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/
	
	
	/*
	** Name: RsltData
	**
	** Description:
	**	Driver class which implements the
	**	ResultSet interface for a fixed pre-defined set of data.
	**
	**  Methods Overridden:
	**
	**	load		    Load data subset.
	**
	**  Private Data:
	**
	**	dataSet		    Row/column data
	**	empty		    Empty row/column set.
	**
	** History:
	**	12-Nov-99 (rajus01)
	**	    Created.
	**	31-Oct-02 (gordy)
	**	    Adapted for generic GCF driver.
	**	 4-Aug-03 (gordy)
	**	    Row cache implemented with addition of data_set, so no longer
	**	    need loaded indicator which also allowed shut() to be dropped.
	**	    Super-class now only requires single load() method.
	**	26-Sep-03 (gordy)
	**	    Column data now stored as SqlData objects.  Renamed data_set
	**	    to dataSet.
	*/
	
	internal class RsltData : AdvanRslt
	{
		/*
		** Result set containing rows with column values.
		*/
		private SqlData[][]		dataSet = null;

		/*
		** Empty result set if initialized with null set.
		*/
		private static  SqlData[][]	empty = new SqlData[0][];
		
		
		/*
		** Name: RsltData
		**
		** Description:
		**	Class constructor.
		**
		** Input:
		**	conn		Associated connection.
		**	rsmd		ResultSet meta-data
		**	dataSet		Constant result set, may be null
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	12-Nov-99 (rajus01)
		**	    Created.
		**	 4-Oct-00 (gordy)
		**	    Create unique ID for standardized internal tracing.
		**	28-Mar-01 (gordy)
		**	    Tracing added as a parameter.
		**	31-Oct-02 (gordy)
		**	    Adapted for generic GCF driver.
		**	26-Sep-03 (gordy)
		**	    Column values now stored as SQL data objects.
		**	26-Sep-03 (gordy)
		**	    Column data now stored as SqlData objects.
		*/
		
		internal RsltData( DrvConn conn, AdvanRSMD rsmd, SqlData[][] dataSet ) :
		              base(conn, rsmd)
		{
			this.dataSet = (dataSet == null) ? empty : dataSet;
			tr_id = "Data[" + inst_id + "]";
		} // RsltData
		
		
		/*
		** Name: load
		**
		** Description:
		**	Load next data subset.  We have only the constant data set,
		**	so we either use it, or set to an empty data set (end-of-data).
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
		**	12-Nov-99 (rajus01)
		**	    Created.
		**	 4-Aug-03 (gordy)
		**	    Row caching support moved to this class.
		*/
		
		protected internal override void  load()
		{
			/*
			** Since base class guarantees the current row state to
			** be consistent and valid, our task is very easy.  We
			** are moving forward one row either to a valid row or
			** past the end of the result set.  If we are moving to
			** a valid row, we may be moving from before the first
			** or from another row and may land on the last row.
			** Note that in all cases it is correct to increment 
			** the row posiition.
			*/
			if ( row_number >= dataSet.Length )
			{
				pos_status = POS_AFTER;
				columns = null;
				row_number++;
				row_status = 0;
			}
			else
			{
				pos_status = POS_ON_ROW;
				columns = dataSet[ row_number ];
				row_number++;
				row_status = 0;
				if ( row_number == 1 )  row_status |= ROW_IS_FIRST;
				if ( row_number == dataSet.Length )  row_status |= ROW_IS_LAST;
			}

			return;
		}

	} // class RsltData
}