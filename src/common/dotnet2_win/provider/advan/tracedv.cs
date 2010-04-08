/*
** Copyright (c) 2002, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: tracedv.cs
	**
	** Description:
	**	Defines class which implements the AdvanTrace interface for
	**	internal driver tracing only. 
	**
	** History:
	**	16-Mar-01 (gordy)
	**	    Created.
	**	23-Aug-02 (thoda04)
	**	    Ported for .NET Provider.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/
	
	
	/*
	** Name: TraceDV
	**
	** Description:
	**	Implements AdvanTrace interface to support internal driver tracing.
	**
	**  Interface Methods:
	**
	**	setTraceLog	Set the tracing output log.
	**	enabled		Is tracing enabled?
	**	log		Log message to trace log.
	**
	**  Private Methods:
	**
	**	init_log	Write initial trace message.
	**
	** History:
	**	16-Mar-01 (gordy)
	**	    Created.*/
	
	internal class TraceDV : Tracing, ITrace
	{
		/*
		** Name: <class initializer>
		**
		** Description:
		**	Initialize the trace log.
		**
		** History:
		**	27-Mar-01 (gordy)
		**	    Created.*/
		static TraceDV()
		{
			// Class Initializer
			init_log(); // Write initial message if log opened by super class.
		}
		
		
		/*
		** Name: TraceDV
		**
		** Description:
		**	Class constructor.  Set tracing level based on tracing ID.
		**
		** Input:
		**	id	Tracing ID.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	None.
		**
		** History:
		**	16-Mar-01 (gordy)
		**	    Created.*/
		
		public TraceDV(String id):base(id)
		{
		} // TraceDV
		
		
		/*
		** Name: setTraceLog
		**
		** Description:
		**	Set the trace log.  If the log name is NULL, tracing will 
		**	be disabled.
		**
		**	Writes the initial trace message to the new log.
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
		**	27-Mar-01 (gordy)
		**	    Created.*/
		public override bool setTraceLog(String name)
		{
			bool success = base.setTraceLog(name);
			if (success)
				init_log();
			return (success);
			
		}
		
		
		/*
		** Name: init_log
		**
		** Description:
		**	Writes the initial trace message to the output log.
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
		**	27-Mar-01 (gordy)
		**	    Created.*/
		
		private static void  init_log()
		{
			System.DateTime now = System.DateTime.Now;
			println(DrvConst.driverID + ": " + DateTime.UtcNow.ToString("r"));
		}

	} // class TraceDV
}