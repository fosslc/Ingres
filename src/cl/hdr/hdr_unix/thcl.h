/*
** Copyright (c) 2007 Ingres Corporation
*/

/**
** Name: THCL.H - Client thread routines.
**
** Description:
**   This module contains the macro definitions required for support of
**   multi-threading on front-end applications.
**
**   NOTE:  This include file is intended for front-end applications only.
**          Refer to csnormal.h for applicable definitions for multi-
**          threaded DBMS servers.
**
** History:
**   23-July-2007 (Ralph Loen) SIR 117786
**     Created based on csnormal.h.
**	04-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with OSX and add support for Intel OSX.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

# ifdef OS_THREADS_USED
/* thread creation flags */
# define TH_DETACHED    0    /* releases resources on exit */
# define TH_JOINABLE    1    /* must be waited for with CS_join_thread() */
# ifdef SYS_V_THREADS
 
# include <sys/priocntl.h>
# include <sys/tspriocntl.h>
 
# define TH_THREAD_ID   thread_t

/*
** Create a thread:
** TH_create_thread( size_t stksize,            // size of stack for thread
**		     void *stkaddr,		// address of stack
**                   void *(*startfn)(void *),  // start function
**                   void *args,                // args (in form of pointer)
**                   TH_THREAD_ID *tidptr,      // return thread id here
**                   long flag,                 // TH_JOINABLE: can use
**                                                      TH_join_thread() to
**                                                      get return value.
**                                                 TH_DETACHED: will release
**                                                      all resources and
**                                                      return value on
**                                                      termination
**                   STATUS *status )           // return value
**  returns: OK on success */
#   define TH_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
                *statp = thr_create(stkaddr,stksize,(void*(*)())startfn,\
                           (void*)args,\
                           THR_BOUND|((flag)==TH_JOINABLE?0:THR_DETACHED),\
                           tidptr)
 
/* Exit a thread:
** TH_exit_thread( void *status )
**      if TH_JOINABLE, status is returned, and thread resources are
**      retained until status received by a TH_join_thread()
*/
#   define TH_exit_thread(status)       thr_exit(status)
 
/* Wait for a thread to exit:
** TH_join_thread( TH_THREAD_ID tid,            // id of thread to wait for
**                 void **status )              // receives return status
*/
#   define TH_join_thread(tid,status)   thr_join(tid,NULL,status)
 
 
# endif /* SYS_V_THREADS */
 
# ifdef POSIX_THREADS

# define TH_THREAD_ID   pthread_t

#if defined(DCE_THREADS)
#   define TH_create_thread(stksize,startfn,args,tidptr,flag,statp) \
                       { pthread_attr_t attr;\
			 pthread_attr_create(&attr);\
			 pthread_attr_setstacksize(&attr, stksize);\
                         if (pthread_create(tidptr,attr,\
                             (pthread_startroutine_t)startfn,args))\
                            *statp=errno;\
                         else\
                            *statp=0;\
                         pthread_attr_delete(&attr)\
                        }
#else
# if defined(SGI_UNBOUND_THREADS)
	/* SGI unbound threads -- so, what's different about 'em? */
#   define TH_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
                                { pthread_attr_t attr;\
                                  pthread_attr_init(&attr);\
                                  pthread_attr_setscope(&attr,\
                                        PTHREAD_SCOPE_PROCESS);\
                                  if((flag)==TH_DETACHED)\
                                    pthread_attr_setdetachstate(&attr,\
                                        PTHREAD_CREATE_DETACHED);\
				  pthread_attr_setstacksize(&attr, stksize);\
 			          if (stkaddr !=0)\
                                  pthread_attr_setstackaddr(&attr, stkaddr);\
                             *statp=pthread_create(tidptr,&attr,startfn,args);\
                                  pthread_attr_destroy(&attr);\
}
#else
	/* Normal (bound posix) thread creation */
# if defined(any_hpux)
#   define TH_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
		{ \
		    pthread_attr_t attr;\
		    pthread_attr_init(&attr);\
		    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);\
		    if ((flag) == TH_DETACHED)\
			pthread_attr_setdetachstate(&attr,\
				PTHREAD_CREATE_DETACHED);\
                    pthread_attr_setstacksize(&attr, stksize);\
 		    if (stkaddr != 0)\
			pthread_attr_setstackaddr(&attr, stkaddr);\
		    *statp = pthread_create(tidptr, &attr, startfn, args);\
		    pthread_attr_destroy(&attr);\
		}
# elif defined(LNX) || defined(OSX)
#   define TH_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
                    { pthread_attr_t attr;\
                      pthread_attr_init(&attr);\
                      pthread_attr_setscope(&attr,\
                          PTHREAD_SCOPE_SYSTEM);\
                      if((flag)==TH_DETACHED)\
                          pthread_attr_setdetachstate(&attr,\
                              PTHREAD_CREATE_DETACHED);\
		      pthread_attr_setstacksize(&attr, stksize);\
		      if (stkaddr !=0)\
		          pthread_attr_setstack(&attr, stkaddr, stksize);\
		      *statp=pthread_create(tidptr,&attr,startfn,args);\
                           pthread_attr_destroy(&attr);\
}
# else
#   define TH_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
                                { pthread_attr_t attr;\
                                  pthread_attr_init(&attr);\
                                  pthread_attr_setscope(&attr,\
                                        PTHREAD_SCOPE_SYSTEM);\
                                  if((flag)==TH_DETACHED)\
                                    pthread_attr_setdetachstate(&attr,\
                                        PTHREAD_CREATE_DETACHED);\
				  pthread_attr_setstacksize(&attr, stksize);\
 			          if (stkaddr !=0)\
                                  pthread_attr_setstackaddr(&attr, stkaddr);\
			    *statp=pthread_create(tidptr,&attr,startfn,args);\
                                  pthread_attr_destroy(&attr);\
}
# endif
# endif /* SGI_UNBOUND_THREADS */
# endif /* DCE_THREADS */

# define TH_exit_thread               pthread_exit
# define TH_join_thread(tid,retp)     pthread_join(tid,retp)

# endif /* POSIX_THREADS */
# else
  /*
  **  Hopefully there are no longer Unix or Linux platforms that don't support
  **  some type of threading interface.
  */
# define TH_create_thread {}
# define TH_exit_thread {}
# define TH_join_thread {}

# endif /* OS_THREADS_USED */
