/*
** Copyright (c) 1999, 2006 Ingres Corporation. All Rights Reserved.
*/

namespace Ingres.Utility
{
	using System;
	
	/*
	** Name: timer.cs
	**
	** Description:
	**	Defines the Timer class and ICallback interface.
	**
	** History:
	**	16-Nov-99 (gordy)
	**	    Created.
	**	18-Apr-00 (gordy)
	**	    Moved to util package.
	**	29-Jul-02 (thoda04)
	**	    Ported to .NET Provider.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/
	
	/*
	** Name: Timer
	**
	** Description:
	**	Class which provides the ability to asynchronously perform
	**	an action after a specified amount of time has expired.
	**	Uses the ICallback interface to perform the desired action.
	**
	**	The timer is implemented as a separate thread and should be
	**	started by calling the start() method inherited from the
	**	Thread class.  The timer may be cancelled by calling the
	**	interrupt() method.  If, after calling start(), the time
	**	expires, a callback is made, in the context of the timer
	**	thread, to perform whatever action is desired.  The ICallback
	**	interface is used to define the callback method: timeExpired().
	**
	** History:
	**	16-Nov-99 (gordy)
	**	    Created.
	**	18-Nov-99 (gordy)
	**	    Brought in Alarm as a nested interface.
	**	18-Apr-00 (gordy)
	**	    Renamed Alarm to Callback. (For .NET: ICallback (thoda04))*/
	
	internal class Timer : SupportClass.ThreadClass
	{
		private int time = 0; // Amount of time to sleep.
		private ICallback callback = null; // Object to be notified.
		
		/*
		** Name: Timer
		**
		** Description:
		**	Class constructor.  Defines the amount of time in the
		**	timer and the object which will perform the desired
		**	action when notified that the time has expired.
		**
		**	The time parameter is taken to be seconds if positive
		**	and milli-seconds if negative.  The timer should be 
		**	started by calling the start() method.
		**
		** Input:
		**	time	    Length of timer (negative for milli-seconds).
		**	callback    Object to be notified when time expired.
		**
		** Output:
		**	None.
		**
		** Returns:
		**	void.
		**
		** History:
		**	16-Nov-99 (gordy)
		**	    Created.*/
		
		public Timer(int time, ICallback callback)
		{
			this.time = time;
			this.callback = callback;
		} // Timer
		
		/*
		** Name: interrupt
		**
		** Description:
		**	Interrupts the timer and disables the callback.  Overrides
		**	Thread.interrupt().  This method may be called multiple times
		**	and at any time during the life of the timer.  If called prior
		**	to the start() method, the timer will be disable completely.
		**	If called prior to timer expiration, the timer thread will be
		**	iterrupted an no callback will occur.  
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
		**	16-Nov-99 (gordy)
		**	    Created.
		**	29-Jul-02 (thoda04)
		**	    Ported to .NET Provider.
		*/
		
		public override void  Interrupt()
		{
			lock(this)
			{
				if (callback != null)
				{
					callback = null;
					if (IsAlive)
						base.Interrupt();
				}
			}
			return ;
		} // interrupt
		
		/*
		** Name: run
		**
		** Description:
		**	Overrides Thread.run() to provide the timer action.
		**	Sleeps for the amount of time requested and performs
		**	the desired action (if timer was not interrupted).
		**
		**	This method should not be called directly.  The timer
		**	should be started by calling the start() method (see
		**	Thread.start()).
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
		**	16-Nov-99 (gordy)
		**	    Created.
		**	29-Jul-02 (thoda04)
		**	    Ported to .NET Provider.
		*/
		
		public override void  Run()
		{
			if (time != 0)
				try
				{
					long ticks;  // number of 100 nanosecond ticks (1e-7 seconds)
					if  (time >= 0)  // if seconds
						ticks = (long)time * 10000000;
					else             // else milliseconds
						ticks = (long)(-time) * 10000;

					System.Threading.Thread.Sleep(new System.TimeSpan(ticks));
				}
				catch (System.Exception )//ignore)
				{
				}
			
			lock(this)
			{
				if (callback != null)
					callback.timeExpired();
				callback = null;
			}
			return ;
		} // run
		
		
		/*
		** Name: ICallback
		**
		** Description:
		**	Interface defining the callback method used by the
		**	Timer class.
		**
		** History:
		**	16-Nov-99 (gordy)
		**	    Created.
		**	18-Apr-00 (gordy)
		**	    Renamed.*/
		
		internal interface ICallback
			{
				/*
				** Name: timeExpired
				**
				** Description:
				**	Implements the action to be performed when the callback
				**	is made.  This method is called by the run() method
				**	of the Timer class when the timer expires.
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
				**	16-Nov-99 (gordy)
				**	    Created.
				**	18-Apr-00 (gordy)
				**	    Renamed.*/
				void  timeExpired();
			} // interface ICallback
	} // class Timer
}