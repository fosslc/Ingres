/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : lbmnmtx.h : header file
**    Project  : LibMon
**    Author   : Hareeswer S (shaha03)
**    Purpose  : Synchronyzation Mutex
**
** History:
**
** 24-Mar-2004 (shaha03)
**    Created.
*/

#if !defined (LBMNMTX_HEADER)
#define LBMNMTX_HEADER

#include "monitor.h"

#define MAX_THREADS	100

typedef struct tagLBMNMUTEX
{
#ifdef UNIX
	pthread_mutex_t mutex;
#else
	HANDLE hMutex;
#endif
	int ownCnt;
	int nCreated;
} LBMNMUTEX;

int CreateLBMNMutex(LBMNMUTEX* pMutex);
int CleanLBMNMutex(LBMNMUTEX* pMutex);
int OwnLBMNMutex (LBMNMUTEX* pMutex, DWORD flag);
int UnownLBMNMutex(LBMNMUTEX* pMutex);


typedef struct{
	BOOL used; /*Possible values....0:unused, 1:used*/	
	int  hnodestruct; 
	int	 monstacklevel;
	int          hnodemem;
	int          iparenttypemem;
	UNIONDATAMIN structparentmem;
	int          itypemem;
	SFILTER      sfiltermem;
	BOOL         bstructparentmem;
}mon4struct;

#ifdef LBMN_TST
	void time_stamp();
#endif

#endif /*LBMNMTX_HEADER*/
