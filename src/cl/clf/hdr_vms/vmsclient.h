/*
** Copyright (c) 2007 Ingres Corporation
*/

/**
** Name: VMSCLIENT.H - pthread routines for multi-threaded VMS client.
**
** Description:
**   This module contains the macro definitions required for support of
**   multi-threading on front-end applications.
**
**   NOTE:  This include file is intended for front-end routines only.
**          This file will be merged into csnormal.h when VMS fully
**          supports VMS multi-threading.
**
** History: 
**   23-March-2007 (Ralph Loen) SIR 117989
**     Extracted from csnormal.h.
**   20-May-2007 (Ralph Loen) SIR 117989
**     Added more pthread routines. 
**   12-June-2007 (Ralph Loen) SIR 118032
**     Added gcacl modes definitions GC_CLIENT_MODE and GC_SERVER_MODE.
**     Create new ME_tls_createkeyFreeFunc() that inclues an input argument 
**     for the TLS destructor function.  Revised ME_TLS_KEY as pthread_key_t
**     instead of i4.
**   09-Jul-2007 (Ralph Loen) Bug 118698
**     Added GC_ASYNC_CLIENT_MODE for API clients.  Renamed
**     GC_CLIENT_MODE to GC_SYNC_CLIENT_MODE.
**   08-oct-2008 (stegr01)
**     include CS_xxxx defines only if CS_SYNCH not defined
**     define VMSCLIENT_H so we don't recurse
*/

#ifndef VMSCLIENT_H
#define VMSCLIENT_H

# include <pthread.h>

#ifndef CS_SYNCH

# define CS_SYNCH pthread_mutex_t

# define CS_THREAD_ID pthread_t

# define CS_DETACHED 0

# define CS_JOINABLE 1

# define CS_synch_init(synchptr) pthread_mutex_init(synchptr,NULL)

# define CS_thread_id_equal(a,b) pthread_equal(a,b)

# define CS_thread_id_assign(a,b) a = b

# define CS_synch_lock(sptr) pthread_mutex_lock(sptr) 

# define CS_synch_unlock(sptr) pthread_mutex_unlock(sptr)

# define CS_get_thread_id pthread_self

# define CS_synch_destroy(sptr)  pthread_mutex_destroy(sptr)

#   define CS_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
                { pthread_attr_t attr;\
                  pthread_attr_init(&attr);\
                  if((flag)==CS_DETACHED)\
                      pthread_attr_setdetachstate(&attr,\
                      PTHREAD_CREATE_DETACHED);\
                  pthread_attr_setstacksize(&attr, stksize);\
                  if (stkaddr !=0)\
                       pthread_attr_setstack(&attr, stkaddr, stksize);\
                  pthread_attr_setinheritsched (&attr, \
                      PTHREAD_EXPLICIT_SCHED); \
                  pthread_attr_setschedpolicy (&attr, SCHED_RR);\
                  *statp=pthread_create(tidptr,&attr,startfn,args);\
                  pthread_attr_destroy(&attr);\
}

# define CS_cond_wait pthread_cond_wait

# define CS_cond_signal pthread_cond_signal

# define CS_COND pthread_cond_t

# define CS_COND_TIMESTRUCT struct timespec

# define CS_cond_timedwait pthread_cond_timedwait

# define CS_cond_init(condptr) pthread_cond_init(condptr,NULL)

# define CS_cond_destroy pthread_cond_destroy

#endif  /* CSSYNCH */

# define ME_tls_createkey(key, status)   *status = \
                                           pthread_key_create(\
                                            (pthread_key_t*)key,\
                                            (void(*)(void*))MEfree)

# define ME_tls_createkeyFreeFunc(key, freeFunc, status)   *status = \
                                           pthread_key_create(\
					    (pthread_key_t*)key,\
					    (void(*)(void*))freeFunc)
# define ME_tls_set(key,value,status)    *status = \
                                           pthread_setspecific(\
					     (pthread_key_t)key,value)
# define ME_tls_get(key,value,status)    {*((void**)(value))=\
					   pthread_getspecific(\
					          (pthread_key_t)key);\
					 *status=OK;}
# define ME_TLS_KEY pthread_key_t
 
# define PCtid pthread_self

/*
** Run-time modes for GCACL are currently defined as client (GCsync required
** and no GCA callbacks from AST's), server (purely asynchronous mode),
** and async API mode.  Asynch API mode uses wflor to wait for I/O events,
** as in legacy versions of GCACL, but makes callbacks to GCA from 
** GCsync(), instead of acting as a purely asynchronous process.
*/
# define GC_SYNC_CLIENT_MODE 0
# define GC_ASYNC_CLIENT_MODE 1
# define GC_SERVER_MODE 2

#endif /* VMSCLIENT_H */
