/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : lbmnmtx.c : implementation file
**    Project  : LibMon 
**    Author   : shaha03
**    Purpose  : Synchronyzation Mutex
**
** History:
**
** 24-Mar-2004 (shaha03)
**    Created.
*/

#include "lbmnmtx.h"

int CreateLBMNMutex(LBMNMUTEX* pMutex)
{
#ifndef BLD_MULTI 
	return 1;
#endif
	if(pMutex->nCreated == 1)
		return pMutex->nCreated;
	pMutex->nCreated = 1;
#ifdef UNIX
	if (pthread_mutex_init (&(pMutex->mutex), pthread_mutexattr_default) != 0)
		pMutex->nCreated = 0;
#else
	pMutex->hMutex = CreateMutex (NULL, FALSE, NULL);
	if (!pMutex->hMutex)
		pMutex->nCreated = 0;
#endif
	return pMutex->nCreated;
}

int CleanLBMNMutex(LBMNMUTEX* pMutex)
{
	int ret =1;
#ifndef BLD_MULTI 
	return ret;
#endif
	if (!pMutex->nCreated)
		return ret;
#ifdef UNIX
	ret =pthread_mutex_destroy (&(pMutex->mutex));
#else
	ret = CloseHandle(pMutex->hMutex);
#endif
	memset (pMutex, 0, sizeof (LBMNMUTEX));
	return ret;
}

int OwnLBMNMutex (LBMNMUTEX* pMutex,DWORD flag)
{
	DWORD dwWait;
#ifndef BLD_MULTI 
	return 1;
#endif

#ifdef LBMN_TST
	if(!pMutex->ownCnt)
	{
		time_stamp();
		printf("Trying to own the Mutex ::[%d] \n",pMutex);
	}
#endif

#ifdef UNIX
	int nOk = pthread_mutex_lock (&(pMutex->mutex));
	switch(nOK)
	{
	case 0:
		pMutex->ownCnt++;
		return 1;
	default:
		return 0;
	}
#else
	dwWait = WaitForSingleObject(pMutex->hMutex, flag);
	switch (dwWait)
	{
	case WAIT_ABANDONED:
		pMutex->ownCnt = 1;
		break;
	case WAIT_OBJECT_0:
		pMutex->ownCnt++;
		break;
	default:
		return 0;
	}
#ifdef LBMN_TST
		if(pMutex->ownCnt == 1){
			time_stamp();
			printf("Owned the Mutex ::[%d] \n",pMutex);
			Sleep(5000);	/* A delay to simulate the mutex comflict---Only for testing */
		}
#endif
	return 1;
#endif
}

int UnownLBMNMutex(LBMNMUTEX* pMutex)
{
	int ret=1;
#ifndef BLD_MULTI 
	return ret;
#endif
	pMutex->ownCnt--;
	if(!pMutex->ownCnt)
	{
#ifdef UNIX
	ret = pthread_mutex_unlock (&(pMutex->mutex));
#else	
	ret = ReleaseMutex(pMutex->hMutex);
#endif

#ifdef LBMN_TST
		time_stamp();
		printf("Released the Mutex ::[%d] \n",pMutex);
#endif
	}
	return ret;
}


void time_stamp()
{
#ifdef LBMN_TST
	SYSTEMTIME sys_time;
	GetSystemTime(&sys_time);
	printf("%dm:%ds:%dms::::thread[%d]\t",sys_time.wMinute,sys_time.wSecond,sys_time.wMilliseconds, GetCurrentThreadId());
	return;
#endif
}

