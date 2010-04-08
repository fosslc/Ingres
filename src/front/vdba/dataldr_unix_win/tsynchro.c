/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : tsynchro.c : implementation file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Synchronyzation Mutex and Event
**
** History:
**
** xx-Jan-2004 (uk$so01)
**    Created.
** 05-Aug-2004 (uk$so01)
**    SIR #111688
**    Additional change to check if SIread returns a value other than
**    ENDFILE and OK as described in Compatlib doc. If so, dataldr will
**    log an error.
** 15-Dec-2004 (toumi01)
**    Port to Linux.
** 29-jan-2008 (huazh01)
**    replace all declarations of 'long' type variable into 'i4'.
**    Bug 119835.
** 02-apr-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, clean up warnings.
*/

#include "stdafx.h"
#include "tsynchro.h"
#include "dtstruct.h"


int CreateDTLMutex(DTLMUTEX* pMutex)
{
	pMutex->nCreated = 1;
#if defined (UNIX)
	if (pthread_mutex_init (&(pMutex->mutex), pthread_mutexattr_default) != 0)
		pMutex->nCreated = 0;
	if (pthread_cond_init (&(pMutex->cond), pthread_condattr_default) != 0)
		pMutex->nCreated = 0;
#else
	pMutex->hMutex = CreateMutex (NULL, FALSE, NULL);
	pMutex->hEvent = CreateEvent (NULL, TRUE, FALSE, NULL);
	if (!pMutex->hMutex || !pMutex->hEvent)
		pMutex->nCreated = 0;
#endif
	return pMutex->nCreated;
}

void CleanDTLMutex(DTLMUTEX* pMutex)
{
	if (!pMutex->nCreated)
		return;
#if defined (UNIX)
	pthread_mutex_destroy (&(pMutex->mutex));
	pthread_cond_destroy (&(pMutex->cond));
#else
	CloseHandle(pMutex->hMutex);
	CloseHandle(pMutex->hEvent);
#endif
	memset (pMutex, 0, sizeof (DTLMUTEX));
}

int OwnMutex (DTLMUTEX* pMutex)
{
#if defined (UNIX)
	int nOk = pthread_mutex_lock (&(pMutex->mutex));
	return (nOk == 0)? 1: 0;
#else
	DWORD dwWait = WaitForSingleObject(pMutex->hMutex, INFINITE);
	switch (dwWait)
	{
	case WAIT_OBJECT_0:
		return 1;
	default:
		return 0;
	}
#endif
	return 1;
}

int UnownMutex(DTLMUTEX* pMutex)
{
#if defined (UNIX)
	int nOk = pthread_mutex_unlock (&(pMutex->mutex));
	return (nOk == 0)? 1: 0;
#else
	ReleaseMutex(pMutex->hMutex);
	return 1;
#endif
}

/*
** nMode =   1: writing
**       =   0: sinal end of file
*/
int AccessQueueWrite (DTLMUTEX* pMutex, char* strLine, int nMode)
{
	OwnMutex(pMutex);
	switch (nMode)
	{
	case 0:
		if (strLine)
			g_pLoadData->nEnd = -1; /* SIread returns an error */
		else
			LOADDATA_Finish(g_pLoadData);
#if defined (UNIX)
		pthread_cond_signal (&(pMutex->cond));
#else
		SetEvent(pMutex->hEvent);
#endif
		UnownMutex(pMutex);
		break;
	case 1:
		/*
		** Event the waiting thread got the signal before
		** the data has been writing to the Q, the waiting thead
		** can effectively read the Q only the mutx has been released.
		** This will be done when the writing thread will complete the write operation:
		*/
#if defined (UNIX)
		pthread_cond_signal (&(pMutex->cond));
#else
		SetEvent(pMutex->hEvent);
#endif
		/*
		** Write data the the QUEUE:
		*/
		LOADDATA_AddRecord (g_pLoadData, strLine);
		UnownMutex(pMutex);
		g_lRecordRead++;
		break;
	default:
		UnownMutex(pMutex);
		break;
	}

	return 1;
}

static int CanAccessQ(LOADDATA* pLoadData)
{
	if (pLoadData->nEnd)
		return 1;
	if (pLoadData->pHead)
		return 1;
	return 0;
}

int AccessQueueRead (DTLMUTEX* pMutex)
{
	int nEnd = 0;
	OwnMutex(pMutex);
	while (!CanAccessQ(g_pLoadData))
	{
		/*
		** Wait until the DATA is available or Reading thread has terminated:
		*/
#if defined (UNIX)
		pthread_cond_wait (&(pMutex->cond), &(pMutex->mutex));
#else
		ResetEvent(pMutex->hEvent);
		ReleaseMutex(pMutex->hMutex);
		WaitForSingleObject(pMutex->hEvent, INFINITE);
		WaitForSingleObject(pMutex->hMutex, INFINITE);
#endif
	}
	return 1;
}

int CreateReadsizeEvent(READSIZEEVENT* pReadsizeEvent)
{
	pReadsizeEvent->nCreated = 1;
#if defined (UNIX)
	if (pthread_mutex_init (&(pReadsizeEvent->mutex), pthread_mutexattr_default) != 0)
		pReadsizeEvent->nCreated = 0;
	if (pthread_cond_init (&(pReadsizeEvent->cond), pthread_condattr_default) != 0)
		pReadsizeEvent->nCreated = 0;
#else
	pReadsizeEvent->hMutex = CreateMutex (NULL, FALSE, NULL);
	pReadsizeEvent->hEvent = CreateEvent (NULL, TRUE, TRUE, NULL);
	if (!pReadsizeEvent->hMutex || !pReadsizeEvent->hEvent)
		pReadsizeEvent->nCreated = 0;
#endif
	return pReadsizeEvent->nCreated;
}

void CleanReadsizeEvent(READSIZEEVENT* pReadsizeEvent)
{
	if (pReadsizeEvent->nCreated)
		return;
#if defined (UNIX)
	pthread_mutex_destroy (&(pReadsizeEvent->mutex));
	pthread_cond_destroy (&(pReadsizeEvent->cond));
#else
	CloseHandle(pReadsizeEvent->hMutex);
	CloseHandle(pReadsizeEvent->hEvent);
#endif
	memset (pReadsizeEvent, 0, sizeof (READSIZEEVENT));
}


static int CanWrite (long lCurrentBuffer, i4 lNewSize)
{
	if (lCurrentBuffer == 0 && lNewSize >= g_lReadSize)
		return 1; /* The record size is greater than the read size, we must allow one record otherwise deadlock*/ 
	if ((lNewSize <= g_lReadSize) && (lCurrentBuffer + lNewSize) <= g_lReadSize)
		return 1;
	return 0;
}

void WaitForBuffer(READSIZEEVENT* pEvent, i4 lbufferSize)
{
	if (lbufferSize == 0)
		return;
#if defined (UNIX)
	pthread_mutex_lock (&(pEvent->mutex));
#else
	WaitForSingleObject(pEvent->hMutex, INFINITE);
#endif

	if (lbufferSize > 0)
	{
		while (!CanWrite(pEvent->lBufferSize, lbufferSize))
		{
			/*
			** Wait until the buffer size has changesd it state:
			*/
#if defined (UNIX)
			pthread_cond_wait (&(pEvent->cond), &(pEvent->mutex));
#else
			ResetEvent(pEvent->hEvent);
			ReleaseMutex(pEvent->hMutex);
			WaitForSingleObject(pEvent->hEvent, INFINITE);
			WaitForSingleObject(pEvent->hMutex, INFINITE);
#endif
		}

		/*
		** Update the new buffer size:
		*/
		pEvent->lBufferSize += lbufferSize;
		/*
		** Release the mutex:
		*/
#if defined (UNIX)
		pthread_mutex_unlock (&(pEvent->mutex));
#else
		ReleaseMutex(pEvent->hMutex);
#endif
	}
	else
	{
		/*
		** Update the new buffer size:
		*/
		pEvent->lBufferSize += lbufferSize; /* lbufferSize is negative */
		if (pEvent->lBufferSize < 0)
			pEvent->lBufferSize = 0;
		/*
		** Release the mutex:
		*/
#if defined (UNIX)
		pthread_mutex_unlock (&(pEvent->mutex));
#else
		ReleaseMutex(pEvent->hMutex);
#endif
		/*
		** Signal if the buffer size satified the condition:
		*/
		if (pEvent->lBufferSize < g_lReadSize)
		{
#if defined (UNIX)
			pthread_cond_signal (&(pEvent->cond));
#else
			SetEvent(pEvent->hEvent);
#endif
		}
	}
}


