/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : synchron.cpp, implementation file 
**    Project  : Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Synchronize Object
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    created
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
*/

#include "stdafx.h"
#include "synchron.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CaSychronizeInterrupt::CaSychronizeInterrupt()
{
	m_hEventWorkerThread  = CreateEvent(NULL, TRUE, TRUE, NULL);
	m_hEventUserInterface = CreateEvent(NULL, TRUE, TRUE, NULL);
}

CaSychronizeInterrupt::~CaSychronizeInterrupt()
{
	m_bRequestCansel = FALSE;
	if (m_hEventWorkerThread)
		CloseHandle(m_hEventWorkerThread);
	if (m_hEventUserInterface)
		CloseHandle(m_hEventUserInterface);
}

void CaSychronizeInterrupt::BlockUserInterface(BOOL bLock)
{
	if (bLock)
		ResetEvent(m_hEventUserInterface);
	else
		SetEvent(m_hEventUserInterface);
}

void CaSychronizeInterrupt::BlockWorkerThread (BOOL bLock)
{
	if (bLock)
		ResetEvent(m_hEventWorkerThread);
	else
		SetEvent(m_hEventWorkerThread);
}

void CaSychronizeInterrupt::WaitUserInterface()
{
	WaitForSingleObject (m_hEventUserInterface, INFINITE);
}

void CaSychronizeInterrupt::WaitWorkerThread()
{
	WaitForSingleObject (m_hEventWorkerThread, INFINITE);
}

