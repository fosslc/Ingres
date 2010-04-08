/*
** Copyright (c) 2007 Ingres Corporation
*/
#include <pthread.h>

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
**   25-July-2007 (Ralph Loen) SIR 117786
**     Remove spurious #endif statements.
**	13-nov-2007 (joea)
**	    Include pthread.h for pthread_t type and prototypes.
*/

/* thread creation flags */
# define TH_DETACHED    0    /* releases resources on exit */
# define TH_JOINABLE    1    /* must be waited for with CS_join_thread() */
# define TH_THREAD_ID   pthread_t
# define TH_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp) \
                    { pthread_attr_t attr;\
                      pthread_attr_init(&attr);\
                      if((flag)==TH_DETACHED)\
                          pthread_attr_setdetachstate(&attr,\
                              PTHREAD_CREATE_DETACHED);\
		      pthread_attr_setstacksize(&attr, stksize);\
		      if (stkaddr !=0)\
		          pthread_attr_setstack(&attr, stkaddr, stksize);\
		      *statp=pthread_create(tidptr,&attr,startfn,args);\
                           pthread_attr_destroy(&attr);\
}
# define TH_exit_thread               pthread_exit
# define TH_join_thread(tid,retp)     pthread_join(tid,retp)
