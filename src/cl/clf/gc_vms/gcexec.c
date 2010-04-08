#include	<compat.h>
#include	<gl.h>
#include    	<ssdef.h>
#include	<starlet.h>
#ifdef OS_THREADS_USED
#     include <pthread.h>
#     include <assert.h>
#     include <builtins.h>
#define MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK
#     include <astjacket.h>
#endif



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
**	21-Sep-90 (jorge)
**	    Added 6.3/6.4 CL Spec comments.
**	    GCexec() now uses sys$setast rather than EXinterrupt(ON) and
**	    then resets AST environment to initial state.
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**      21-dec-2008 (stegr01)
**         Itanium VMS port.
**      11-may-2009 (stegr01)
**         move variable gc_running out of the OS_THREADS conditional compilation
**         so this will build on Ingres internal threads
*/	


/*{
** Name: GCexec - Drive Asynchronous Events
** 
** Description:
** 
** GCexec drives the event handling mechanism in several GCA
** applications.  The application typically posts a few initial
** asynchronous GCA CL requests (and in the case of the Comm Server, a few
** initial GCC CL requests), and then passes control to GCexec.  GCexec
** then allows asynchronous completion of events to occur, calling the
** callback routines listed for the asynchronous requests; these callback
** routines typically post more asynchronous requests.  This continues
** until one of the callback routines calls GCshut.
** 
** If the OS provides the event handling mechanism, GCexec presumably
** needs only to enable asynchronous callbacks and then block until
** GCshut is called.  If the CL must implement the event handling
** mechanism, then GCexec is the entry point to its main loop.
** 
** The event mechanism under GCexec must not interrupt one callback
** routine to run another: once a callback routine is called, the event
** mechanism must wait until the call returns before driving another
** callback.  That is, callback routines run atomically.
** 
** Inputs:
**     	none
** 
** Outputs:
** 	none
** 
** Returns:
** 	none
** 
** Exceptions:
** 	none
** 
** Side Effects:
** 	All events driven.
**
** History:
**	27-Nov-89 (seiwald)
**	    Written.
*/	

#if defined(OS_THREADS_USED) && defined(POSIX_THREADS)

static __align(LONGWORD) i4 pt_initialisation_lock   = 0;

static pthread_mutex_t    gcRunningMtx    = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t     gcRunningCv     = PTHREAD_COND_INITIALIZER;

static i4 pt_initialised = 0;

GLOBALREF bool GC_os_threaded;
#endif

static i4 GC_running     = 0;

GCexec()
{
#if defined(OS_THREADS_USED) && defined(POSIX_THREADS)

    __LOCK_LONG(&pt_initialisation_lock);
    if (!pt_initialised)
    {
       /*
       ** name and validate the statically initialised MTX
       **
       */

       pthread_mutexattr_t mutexAttr;
       assert (pthread_mutexattr_init(&mutexAttr) == 0);
       assert (pthread_mutexattr_settype(&mutexAttr, MUTEX_TYPE) == 0);

       assert (pthread_mutex_init (&gcRunningMtx, &mutexAttr) == 0);
       assert (pthread_mutex_setname_np (&gcRunningMtx, "GC_running_MTX" , NULL) == 0);
       assert (pthread_mutexattr_destroy (&mutexAttr) == 0);
       assert (pthread_mutex_lock (&gcRunningMtx) == 0);
       assert (pthread_mutex_unlock (&gcRunningMtx) == 0);

       /*
       ** name and validate the statically initialised CV
       **
       */

       assert (pthread_cond_setname_np(&gcRunningCv,   "GC_running_CV", NULL) == 0);
       pt_initialised = 1;
    }
    __UNLOCK_LONG(&pt_initialisation_lock);
#else
    long    old_ast;

    old_ast = sys$setast((char)TRUE);
#endif

    /* Running Internal threads in an OS Threaded program.
    ** Experience has shown that the main (only!) thread
    ** is woken following AST hqandling, so stay here
    ** until GCshut called.
    */

    GC_running++;
    while (GC_running > 0)
    {
#if defined(OS_THREADS_USED) && defined(POSIX_THREADS)
        assert (pthread_mutex_lock (&gcRunningMtx) == 0);
        assert (pthread_cond_wait (&gcRunningCv, &gcRunningMtx) == 0);
        assert (pthread_mutex_unlock (&gcRunningMtx) == 0);
#else
        sys$hiber();
#endif
    }

#if defined(OS_THREADS_USED) && defined(POSIX_THREADS)
#else
    if(old_ast == SS$_WASCLR)
        sys$setast((char)FALSE);        /* Disable ASTs again */
#endif
}


/*{
** Name: GCshut - Stop GCexec
** 
** Description:
** 
** GCshut causes GCexec to return.  When an application uses GCexec to drive
** callbacks of asynchronous GCA CL and GCC CL requests, it terminates
** asynchronous activity by calling GCshut.  When control returns to GCexec,
** it itself returns.  The application then presumably exits.
** 
** If the application calls GCshut before passing control to GCexec, GCexec
** should return immediately.  This can occur if the completion of asynchronous
** requests occurs before the application calls GCexec.
** 
** Inputs:
**     	none
** 
** Outputs:
** 	none
** 
** Returns:
** 	none
** 
** Exceptions:
** 	none
** 
** Side Effects:
** 	GCexec returns.
**
** History:
**	11-Dec-89 (seiwald)
**	    Written.
*/	

GCshut()
{
    GC_running--;

#if defined(OS_THREADS_USED) && defined(POSIX_THREADS)
    assert (pthread_mutex_lock (&gcRunningMtx) == 0);
    assert (pthread_cond_signal(&gcRunningCv)  == 0);
    assert (pthread_mutex_unlock (&gcRunningMtx) == 0);
#else
    sys$wake(0, 0);
#endif
}
