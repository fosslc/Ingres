/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipm.cpp : Implementation File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Implementation of CIpmApp and DLL registration.
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 24-Nov-2004 (schph01)
**    Bug #113511 initialize _tsetlocale() function according to the
**    LC_COLLATE category setting of the current locale,
**    for sorting object name in tree and Rigth pane list.
*/

#include "stdafx.h"
#include "ipm.h"
#include "calsctrl.h"
#include "wmusrmsg.h"
#include <locale.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CIpmApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0xab474683, 0xe577, 0x11d5, { 0x87, 0x8c, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;

static BOOL bStartLoopNotification = TRUE;
static UINT ThreadNotificationDataChanges(LPVOID pParam)
{
	HANDLE hEvent = (HANDLE)(LPVOID)(pParam);
	theApp.m_heThreadDataChange = CreateEvent (NULL, TRUE, FALSE, NULL);

	while (bStartLoopNotification)
	{
		DWORD dwWait = WaitForSingleObject(hEvent, INFINITE);
		if (dwWait == WAIT_OBJECT_0 && bStartLoopNotification)
		{
			theApp.GetControlTracker().Notify (NULL, 0, 0);
		}
		if (bStartLoopNotification)
			ResetEvent (hEvent);
	}

	SetEvent (theApp.m_heThreadDataChange);
	return 0;
}


static BOOL bStartLoopBkRefresh = TRUE;
static UINT ThreadBackgroundRefresh (LPVOID pParam)
{
	TRACE0("ThreadBackgroundRefresh started\n");
	long nStep = 0;
	theApp.m_heThreadBkRefresh = CreateEvent (NULL, TRUE, FALSE, NULL);

	while (bStartLoopBkRefresh)
	{
		Sleep (500);
		nStep+= 500;
		if (nStep >= (1000*theApp.GetRefreshFrequency()))
		{
			nStep = 0;
			theApp.GetControlTracker().Notify (NULL, (WPARAM)1, 0);
		}
	}

	SetEvent(theApp.m_heThreadBkRefresh);
	TRACE0("ThreadBackgroundRefresh stopped\n");
	return 0;
}

void PropertyChange(CuListCtrl* pCtrl, WPARAM wParam, LPARAM lParam)
{
	if (!pCtrl)
		return;
	CaIpmProperty* pProperty = (CaIpmProperty*)lParam;
	if (!pProperty)
		return;
	BOOL bSet = FALSE;
	UINT nMask = (UINT)wParam;
	if (nMask & IPMMASK_SHOWGRID)
	{
		pCtrl->SetLineSeperator(pProperty->IsGrid());
		bSet = TRUE;
	}
	if (nMask & IPMMASK_FONT)
	{
		HFONT fFont = (HFONT)pProperty->GetFont();
		pCtrl->SetFont (CFont::FromHandle(fFont));
		bSet = TRUE;
	}

	if (bSet)
		pCtrl->Invalidate();
}


CIpmApp::CIpmApp()
{
	lstrcpy (m_tchszOutOfMemoryMessage, _T("Low of Memory ...\nCannot allocate memory, please close some applications !"));
	m_hEventDataChange = NULL;
	m_pThreadDataChange = NULL;
	m_pThreadBackgroundRefresh = NULL;

	m_nRefreshFrequency=5;
	m_bActivateRefresh = FALSE;

	m_heThreadDataChange = NULL;
	m_heThreadBkRefresh = NULL;

	m_pFilter = new SFILTER;
	memset (m_pFilter, 0, sizeof(SFILTER));
	m_strHelpFile = _T("ipm.chm");
}

CIpmApp::~CIpmApp()
{
	delete m_pFilter;
}



void CIpmApp::StartDatachangeNotificationThread()
{
	if (!m_pThreadDataChange && m_hEventDataChange)
	{
		m_pThreadDataChange = AfxBeginThread(
			(AFX_THREADPROC)ThreadNotificationDataChanges, 
			(LPVOID)m_hEventDataChange, 
			THREAD_PRIORITY_NORMAL);
	}
}

void CIpmApp::StartBackgroundRefreshThread()
{
	if (m_bActivateRefresh)
	{
		if (!m_pThreadBackgroundRefresh)
		{
			m_pThreadBackgroundRefresh = AfxBeginThread(
				(AFX_THREADPROC)ThreadBackgroundRefresh, 
				(LPVOID)NULL, 
				THREAD_PRIORITY_NORMAL);
		}
		else
		{
			m_pThreadBackgroundRefresh->ResumeThread();
		}
	}

	if (!m_bActivateRefresh && m_pThreadBackgroundRefresh)
	{
		m_pThreadBackgroundRefresh->SuspendThread();
	}
}

////////////////////////////////////////////////////////////////////////////
// CIpmApp::InitInstance - DLL initialization

BOOL CIpmApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();
	_tsetlocale(LC_TIME, _T(""));
	_tsetlocale(LC_COLLATE, _T(""));
	_tsetlocale(LC_CTYPE, _T(""));

	if (bInit)
	{
		bInit = IPM_Initialize(m_hEventDataChange);
	}

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CIpmApp::ExitInstance - DLL termination

int CIpmApp::ExitInstance()
{
	if (m_heThreadDataChange)
	{
		bStartLoopNotification = FALSE;
		SetEvent(m_hEventDataChange);
		WaitForSingleObject(theApp.m_heThreadDataChange, 1000*60);
		Sleep(200); // Jest for a Thread function has time to return
		CloseHandle(m_heThreadDataChange);
	}

	if (m_heThreadBkRefresh && m_pThreadBackgroundRefresh)
	{
		bStartLoopBkRefresh = FALSE;
		m_pThreadBackgroundRefresh->ResumeThread();
		WaitForSingleObject(m_heThreadBkRefresh, 1000*60);
		Sleep(200); // Jest for a Thread function has time to return
		CloseHandle(m_heThreadBkRefresh);
	}

	IPM_Exit(m_hEventDataChange);
	return COleControlModule::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}
