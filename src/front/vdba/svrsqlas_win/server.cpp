/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : server.cpp: implementation of the CaComServer class
**    Project  : Meta Data / COM Server 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Server, for internal use as Server of Component.
**
** History:
**
** 15-Oct-1999 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
**/


#include "stdafx.h"
#include "rcdepend.h"
#include "server.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// Constructor:
CaComServer::CaComServer()
{
	// Zero the Object and Lock counts for this server.
	m_cObjects = 0;
	m_cLocks = 0;

	// Zero the cached handles.
	m_hInstServer = NULL;
	m_hWndServer = NULL;
}

//
// Destructor:
CaComServer::~CaComServer()
{

}


BOOL CaComServer::OwnThis()
{
	BOOL bOwned = FALSE;
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hOwnerMutex, INFINITE))
	{
		m_bOwned = bOwned = TRUE;
	}
	return bOwned;
}

void CaComServer::UnOwnThis()
{
	if (m_bOwned)
	{
		m_bOwned = FALSE;
		ReleaseMutex(m_hOwnerMutex);
	}
}


void CaComServer::ObjectsUp()
{
	if (OwnThis())
	{
		m_cObjects += 1;

		UnOwnThis();
	}
}


void CaComServer::ObjectsDown()
{
	if (OwnThis())
	{
		if (m_cObjects > 0)
			m_cObjects -= 1;

		//
		// If no more living objects and no locks then shut down the server.
		if (0L == m_cObjects && 0L == m_cLocks )
		{
			// 
			// Destroy all the sessions in the cache
			theApp.GetSessionManager().Cleanup();
		}
		UnOwnThis();
	}
}


void CaComServer::Lock()
{
	if (OwnThis())
	{
		m_cLocks += 1;
		UnOwnThis();
	}
}


void CaComServer::Unlock()
{
	if (OwnThis())
	{
		m_cLocks -= 1;
		if (m_cLocks < 0)
			m_cLocks = 0;
		//
		// If no more living objects and no locks then shut down the server.
		if (0L == m_cObjects && 0L == m_cLocks && IsWindow(m_hWndServer))
		{
			//
			// Post a message to this local server's message queue requesting
			// a close of the application. This will force a termination of
			// all apartment threads.
			PostMessage(m_hWndServer, WM_CLOSE, 0, 0L);
		}
		UnOwnThis();
	}
}

//
//  Function: AptThreadProc
//
//  Summary:  The common apartment model thread procedure for this server.
//
//  Args:     LPARAM lparam
//              Standard Window Proc parameter.
//
//  Modifies: .
//
//  Returns:  DWORD
//              Thread procedure return (usually msg.wParam).
// ************************************************************************************************
DWORD WINAPI AptThreadProcIngresObject(LPARAM lparam)
{
	HRESULT hError;
	MSG msg;
	DWORD dwCFRegId;
	APT_INIT_DATA* paid = (APT_INIT_DATA*) lparam;

	// Initialize COM for this apartment thread. Default of apartment
	// model is assumed.
	hError = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	//
	// Now register the class factory with COM.
	hError = CoRegisterClassObject(
		paid->rclsid,
		paid->pcf,
		CLSCTX_LOCAL_SERVER,
		REGCLS_MULTIPLEUSE,
		&dwCFRegId);

	if (SUCCEEDED(hError))
	{
		//
		// Provide a message pump for this thread.
		while (GetMessage(&msg, 0, 0, 0))
			DispatchMessage(&msg);

		//
		// Unregister the class factory with COM when the thread dies.
		CoRevokeClassObject(dwCFRegId);
	}
	else
	{
	}

	//
	// Uninitialize COM in the context of this apartment thread.
	CoUninitialize();

	return msg.wParam;
}











