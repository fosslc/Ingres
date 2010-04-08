/*
**Copyright (c) 2004 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>


/*{
** Name: GCEXEC.C - drive GCA events
**
** Description:
**	Contains:
**	    GCexec - drive I/O completions
**	    GCshut - stop GCexec
** History:
**	27-Nov-89 (seiwald)
**	    Written.
**	11-Dec-89 (seiwald)
**	    Turned back into a CL level loop after realizing that VMS
**	    doesn't return between each I/O completion.
**	21-Dec-89 (seiwald)
**	    Changed logic so that GCexec continues as long as GCexec
**	    has been called more times that GCshut.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Update the function declarations to fix gcc 4.3 problems.
*/	


/*{
** Name: GCexec() - drive GCA events
**
** Description:
**	Drive I/O completion for async GCA.  Return when GCshut is called.
**	
**	GCexec drives the event handling mechanism in several GCA
**	applications.  The application typically posts a few initial
**	asynchronous GCA CL requests (and in the case of the Comm
**	Server, a few initial GCC CL requests), and then passes control
**	to GCexec.  GCexec then allows asynchronous completion of
**	events to occur, calling the callback routines listed for the
**	asynchronous requests; these callback routines typically post
**	more asynchronous requests.  This continues until one of the
**	callback routines calls GCshut.
**	
**	If the OS provides the event handling mechanism, GCexec
**	presumably needs only to enable asynchronous callbacks and then
**	block until GCshut is called.  If the CL must implement the
**	event handling mechanism, then GCexec is the entry point to its
**	main loop.
**	
**	The event mechanism under GCexec must not interrupt one
**	callback routine to run another: once a callback routine is
**	called, the event mechanism must wait until the call returns
**	before driving another callback.  That is, callback routines
**	run atomically.
**
** Side effects:
**	All events driven.
**
** History:
**	27-Nov-89 (seiwald)
**	    Written.
*/	

static i4  GC_running = 0;

void GCexec(void)
{
	GC_running++;

	while( GC_running > 0 )
	{
		i4 timeout = -1;
		(void)iiCLpoll( &timeout );
	}
}


/*{
** Name: GCshut() - stop GCexec
**
** Description:
**	Terminate GCexec.
**
**	GCshut causes GCexec to return.  When an application uses
**	GCexec to drive callbacks of asynchronous GCA CL and GCC CL
**	requests, it terminates asynchronous activity by calling
**	GCshut.  When control returns to GCexec, it itself returns.
**	The application then presumably exits.
**	
**	If the application calls GCshut before passing control to
**	GCexec, GCexec should return immediately.  This can occur if
**	the completion of asynchronous requests occurs before the
**	application calls GCexec.
**	
** History:
**	11-Dec-89 (seiwald)
**	    Written.
*/	

void GCshut(void)
{
	GC_running--;
}
