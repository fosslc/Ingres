/*
**    Copyright (c) 2008 Ingres Corporation
**
*/

/*
** Name: pthreadmgmt.h - Pthread/AST management
**
**
** History:
**  18-Jun-2007 (stegr01)
*/

#ifndef PTHREAD_MGMT_H_INCLUDED
#define PTHREAD_MGMT_H_INCLUDED

#include <compat.h>

#include <pthread.h>

/*
** Pthread.h in every version of VMS prior to 8.3 has included a space
** between the macro name and its arg list - the compiler will treat
** this (correctly I might add) as a function call - so these 2 macros
** need to be undefed and redefined correctly
*/

#ifdef sched_get_priority_max
#undef sched_get_priority_max
#endif
#define sched_get_priority_max(_pol_) ((_pol_ == SCHED_OTHER) ? PRI_FG_MAX_NP : PRI_FIFO_MAX)

#ifdef sched_get_priority_min
#undef sched_get_priority_min
#endif
#define sched_get_priority_min(_pol_) ((_pol_ == SCHED_OTHER) ? PRI_FG_MIN_NP : PRI_FIFO_MIN)



/* status returned by pthread library */

typedef i4 PTHREAD_STS;

#define PTHREAD_MAX_OBJNAMLEN   31

#define  REPORT_PTHREAD_ERROR(pthread_sts, pthread_func, pthread_obj) \
         if (pthread_sts) report_pthread_error(__func__, __LINE__, pthread_func, pthread_sts, pthread_obj)

void  report_pthread_error(const char* func,
                           i4 line,
                           char* pthread_func,
                           PTHREAD_STS pthread_sts,
                           char* pthread_obj);
char* getObjectName(u_i8* obj);


#define  II_LOCK_C_RTL   pthread_lock_global_np
#define  II_UNLOCK_C_RTL pthread_unlock_global_np



#endif /* PTHREAD_MGMT_H_INCLUDED */
