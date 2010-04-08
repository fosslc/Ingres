/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.util;

/*
** Name: Timer.java
**
** Description:
**	Defines the Timer class and Callback interface.
**
** History:
**	16-Nov-99 (gordy)
**	    Created.
**	18-Apr-00 (gordy)
**	    Moved to util package.
*/

/*
** Name: Timer
**
** Description:
**	Class which provides the ability to asynchronously perform
**	an action after a specified amount of time has expired.
**	Uses the Callback interface to perform the desired action.
**
**	The timer is implemented as a separate thread and should be
**	started by calling the start() method inherited from the
**	Thread class.  The timer may be cancelled by calling the
**	interrupt() method.  If, after calling start(), the time
**	expires, a callback is made, in the context of the timer
**	thread, to perform whatever action is desired.  The Callback
**	interface is used to define the callback method: timeExpired().
**
** History:
**	16-Nov-99 (gordy)
**	    Created.
**	18-Nov-99 (gordy)
**	    Brought in Alarm as a nested interface.
**	18-Apr-00 (gordy)
**	    Renamed Alarm to Callback.
*/

public class
Timer
    extends Thread
{

    private int		time = 0;		// Amount of time to sleep.
    private Callback	callback = null;	// Object to be notified.
    private Object	lock = new Object();	// Internal synchronization
    
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
**	    Created.
*/

public
Timer( int time, Callback callback )
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
*/

public void
interrupt()
{
    synchronized( lock )
    {
	if ( callback != null )
	{
	    callback = null;
	    if ( isAlive() )  super.interrupt();	
	}
    }
    return;
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
*/

public void
run()
{
    if ( time != 0 )  
	try { sleep( time > 0 ? (long)time * 1000L : (long)(-time) ); }
	catch( Exception ignore ) {};

    synchronized( lock )
    {
	if ( callback != null )  callback.timeExpired();
	callback = null;
    }
    return;
} // run


/*
** Name: Callback
**
** Description:
**	Interface defining the callback method used by the
**	Timer class.
**
** History:
**	16-Nov-99 (gordy)
**	    Created.
**	18-Apr-00 (gordy)
**	    Renamed.
*/

public interface
Callback
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
**	    Renamed.
*/

public void 
timeExpired();

} // interface Callback

} // class Timer
