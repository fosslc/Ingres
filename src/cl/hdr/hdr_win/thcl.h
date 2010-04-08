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
**     Created.
**   22-Aug-2007 (drivi01)
**     Removed character ^M at the end of each line.
*/

/* thread creation flags */
# define TH_DETACHED    0    /* releases resources on exit */
# define TH_JOINABLE    1    /* must be waited for with CS_join_thread() */
 
# define TH_THREAD_ID   i4

/*
** Create a thread:
** TH_create_thread( size_t stksize,                size of stack for thread
**                   void *stkaddr,                 address of stack
**                   void *(*startfn)(void *),      start function
**                   void *args,                    args (in form of pointer)
**                   TH_THREAD_ID *tidptr,          return thread id here
**                   long flag,                     TH_JOINABLE: can use
**                                                      TH_join_thread() to
**                                                      get return value.
**                                                  TH_DETACHED: will release
**                                                      all resources and
**                                                      return value on
**                                                      termination
**                   STATUS *status )               return value
**  returns: OK on success 
*/

# define TH_create_thread(stksize,stkaddr,startfn,args,tidptr,flag,statp)\
    {  HANDLE handle;\
        handle = CreateThread(NULL, stksize, (LPTHREAD_START_ROUTINE)startfn,\
            (void *) args, flag, tidptr);\
        if (handle == NULL)\
            *statp = GetLastError();\
        else\
        {\
            CloseHandle( handle );\
            *statp = 0;\
        }\
    }
 
/* 
** Exit a thread.
*/
#   define TH_exit_thread(status)       *status = ExitThread(0)
 
/* 
** Wait for a thread to exit:
** TH_join_thread( TH_THREAD_ID tid,            id of thread to wait for
**                 void **status )              receives return status
*/
#   define TH_join_thread(tid, status) \
{ \
    HANDLE handle; \
    handle = OpenThread( SYNCHRONIZE, FALSE, tid); \
    if (handle == NULL) \
         *status = GetLastError(); \
    else \
    { \
        *status = OK; \
        WaitForSingleObject( handle, INFINITE); \
        CloseHandle( handle ); \
    } \
}
