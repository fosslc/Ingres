/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : tsynchro.h : header file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Synchronyzation Mutex and Event
**
** History:
**
** xx-Jan-2004 (uk$so01)
**    Created.
** 15-Dec-2004 (toumi01)
**    Port to Linux.
** 28-Mar-2005 (shaha03)
**    Added check for UNIX in appropriate locations to avoid
**    compilation problem on Unix platforms.	
** 29-jan-2008 (huazh01)
**    replace all declarations of 'long' type variable into 'i4'.
**    Bug 119835.
*/


#if !defined (SYNCHRONIZATION_T_HEADER)
#define SYNCHRONIZATION_T_HEADER
#include "listchar.h"

#if ( defined(LINUX) || defined(UNIX))
#define pthread_attr_default NULL
#define pthread_condattr_default NULL
#define pthread_mutexattr_default NULL
#endif

typedef struct tagDTLMUTEX
{
#if defined (UNIX)
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
#else
	HANDLE hMutex;
	HANDLE hEvent;
#endif
	int nCreated;
} DTLMUTEX;

extern DTLMUTEX g_mutexWrite2Log;
extern DTLMUTEX g_mutexLoadData;
int CreateDTLMutex(DTLMUTEX* pMutex);
void CleanDTLMutex(DTLMUTEX* pMutex);
int OwnMutex (DTLMUTEX* pMutex);
int UnownMutex(DTLMUTEX* pMutex);
int AccessQueueWrite (DTLMUTEX* pMutex, char* strLine, int nMode); /* no need to call UnownMutex(pMutex) */
int AccessQueueRead (DTLMUTEX* pMutex); /* UnownMutex(pMutex) must be called to release mutex */

typedef struct tagREADSIZEEVENT
{
	i4 lBufferSize;
#if defined (UNIX)
	pthread_mutex_t mutex;
	pthread_cond_t  cond;
#else
	HANDLE hEvent;
	HANDLE hMutex;
#endif
	int nCreated;
} READSIZEEVENT;
extern READSIZEEVENT g_ReadSizeEvent;
int CreateReadsizeEvent(READSIZEEVENT* pReadsizeEvent);
void CleanReadsizeEvent(READSIZEEVENT* pReadsizeEvent);
/*
** lbufferSize:
**   - Positive value: increase buffer size by 'lbufferSize'
**   - Negative value: decrease buffer size by 'lbufferSize'
*/
void WaitForBuffer(READSIZEEVENT* pEvent, i4 lbufferSize);



#endif /* SYNCHRONIZATION_T_HEADER */
