/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmdoc.cpp : implementation File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : interface of the CdIpm class (MFC frame/view/doc).
**
** History:
**
** 20-Fev-2002 (uk$so01)
**    Created
**    SIR #106648, Integrate ipm.ocx.
** 14-Apr-2004: (uk$so01)
**    BUG #112150 /ISSUE 13350768 
**    VDBA, Load/Save fails to function correctly with .NET environment.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "mainfrm.h"
#include "ipmaxdoc.h"
#include "ipmaxvie.h"
#include "ipmaxfrm.h"
#include "a2stream.h"

extern "C" BOOL CloseNodeStruct (int hnodestruct, BOOL bkeep); 
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL (CaIpmFrameFilter, CObject, 1)
CaIpmFrameFilter::CaIpmFrameFilter()
{
	m_bExpreshRefresh = FALSE;
	m_arrayFilter.SetSize(6);
}

void CaIpmFrameFilter::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_bExpreshRefresh;
	}
	else
	{
		ar >> m_bExpreshRefresh;
	}
	m_arrayFilter.Serialize(ar);
}



IMPLEMENT_DYNCREATE(CdIpm, CDocument)
BEGIN_MESSAGE_MAP(CdIpm, CDocument)
	//{{AFX_MSG_MAP(CdIpm)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CdIpm::CdIpm()
{
	m_strNode = _T("");
	m_strServer = _T("");
	m_strUser = GetDefaultConnectedUser();
	m_bConnected = FALSE;
	m_nExpressRefreshFrequency = 2;
	m_hEventQuickRefresh = CreateEvent (NULL, TRUE, TRUE, NULL);
	m_hMutexAccessnExpressRefreshFrequency = CreateMutex (NULL, FALSE, NULL);

	m_SeqNum = 0;
	m_nNodeHandle = -1;
	m_IngresVersion = -1;
	m_bLoaded = FALSE;
	m_bToolbarVisible = TRUE;
}

CdIpm::~CdIpm()
{
	ReleaseMutex(m_hMutexAccessnExpressRefreshFrequency);
	SetEvent(m_hEventQuickRefresh);
	CloseHandle(m_hMutexAccessnExpressRefreshFrequency);
	CloseHandle(m_hEventQuickRefresh);

	if (m_nNodeHandle != -1)
	{
		CString strItem;
		CString strConnection = m_strNode;
		if (!m_strServer.IsEmpty())
		{
			strItem.Format (_T(" (/%s)"), (LPCTSTR)m_strServer);
			strConnection += strItem;
		}
		if (!m_strUser.IsEmpty())
		{
			strItem.Format (_T(" (user:%s)"), (LPCTSTR)m_strUser);
			strConnection += strItem;
		}

		LPTSTR lpszNode = (LPTSTR)(LPCTSTR)strConnection;
		CloseNodeStruct (m_nNodeHandle , TRUE);
		//
		// Request for refresh list of opened windows
		if (DelayedUpdatesAccepted())
			DelayUpdateOnNode(lpszNode);
	}
}

int  CdIpm::GetExpressRefreshFrequency()
{
	int nFrequency = 2;
	DWORD dwWait = WaitForSingleObject(m_hMutexAccessnExpressRefreshFrequency, 1000);
	switch (dwWait)
	{
	case WAIT_OBJECT_0:
		nFrequency = m_nExpressRefreshFrequency;
		ReleaseMutex(m_hMutexAccessnExpressRefreshFrequency);
		return nFrequency;
	default: 
		return 2;
	}
}

void CdIpm::SetExpressRefreshFrequency(int nFrequency)
{
	DWORD dwWait = WaitForSingleObject(m_hMutexAccessnExpressRefreshFrequency, 1000);
	switch (dwWait)
	{
	case WAIT_OBJECT_0:
		m_nExpressRefreshFrequency = nFrequency;
		ReleaseMutex(m_hMutexAccessnExpressRefreshFrequency);
		break;
	default:
		break;
	}
}

BOOL CdIpm::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}

CDialogBar* CdIpm::GetToolBar()
{
	CfIpm* pFrame = GetFrameWindow();
	ASSERT (pFrame);
	if (pFrame)
		return &(pFrame->GetToolBar());
	return NULL;
}

CfIpm* CdIpm::GetFrameWindow()
{
	POSITION pos = GetFirstViewPosition();
	CView* pView = GetNextView(pos);
	ASSERT (pView);
	if (pView)
	{
		CfIpm* pFrame = (CfIpm*)pView->GetParentFrame();
		ASSERT (pFrame);
		return pFrame;
	}
	return NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CdIpm serialization

void CdIpm::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		CfIpm* pFrame = GetFrameWindow();
		if (!pFrame)
			return;
		pFrame->GetFilter(&m_filter);

		ar << m_strNode;
		ar << m_strServer;
		ar << m_strUser;
		ar << m_bConnected;
		ar << m_nExpressRefreshFrequency;
		ar << m_IngresVersion;
		ar << GetTitle();
		BOOL bToolBarVisible = FALSE;
		CDialogBar* pTbar = GetToolBar();
		if (pTbar && IsWindowVisible(pTbar->m_hWnd))
			bToolBarVisible = TRUE;
		ar << bToolBarVisible;
		ar << m_nNodeHandle;

		//
		// Full state of all toolbars in the frame
		pFrame->GetDockState(m_toolbarState);
		m_toolbarState.Serialize(ar);
		//
		// Frame window placement
		memset(&m_wplj, 0, sizeof(m_wplj));
		BOOL bResult = pFrame->GetWindowPlacement(&m_wplj);
		ASSERT (bResult);
		ar.Write(&m_wplj, sizeof(m_wplj));
	}
	else
	{
		CString strTitle;
		m_bLoaded = TRUE;

		ar >> m_strNode;
		ar >> m_strServer;
		ar >> m_strUser;
		ar >> m_bConnected;
		ar >> m_nExpressRefreshFrequency;
		ar >> m_IngresVersion;
		ar >> strTitle; SetTitle(strTitle);
		ar >> m_bToolbarVisible;
		ar >> m_nNodeHandle;
		//
		// Full state of all toolbars in the frame
		m_toolbarState.Serialize(ar);
		//
		// Frame window placement
		ar.Read(&m_wplj, sizeof(m_wplj));
	}
	m_filter.Serialize(ar);
	SerializeIpmControl(ar);
}


void CdIpm::SerializeIpmControl(CArchive& ar)
{
	if (ar.IsStoring())
	{
		CuIpm* pCtrl = GetIpmCtrl();
		ASSERT(pCtrl);
		if (!pCtrl)
			return;
		IStream* pStream = m_ipmStream.Detach();
		if (pStream)
			pStream->Release();

		pCtrl->Storing((LPUNKNOWN*)(&pStream));
		if (pStream)
		{
			m_ipmStream.Attach(pStream);
			m_ipmStream.SeekToBegin();
			CArchiveFromIStream(ar, pStream);
		}
	}
	else
	{
		IStream* pStream = m_ipmStream.Detach();
		if (pStream)
			pStream->Release();
		CArchiveToIStream  (ar, &pStream);
		if (pStream)
			m_ipmStream.Attach(pStream);
	}
}


/////////////////////////////////////////////////////////////////////////////
// CdIpm diagnostics

#ifdef _DEBUG
void CdIpm::AssertValid() const
{
	CDocument::AssertValid();
}

void CdIpm::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdIpm commands

CuIpm* CdIpm::GetIpmCtrl()
{
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView* pView = GetNextView(pos);
		if (pView->IsKindOf(RUNTIME_CLASS(CvIpm)))
		{
			CvIpm* pIpmView = (CvIpm* )pView;
			return &pIpmView->m_cIpm;
		}
	}
	return NULL;
}

void CdIpm::SetNode(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strNode = lpszItem;
}

void CdIpm::SetServer(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strServer = lpszItem;
}

void CdIpm::SetUser(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strUser = lpszItem;
}

void CdIpm::SetConnectState(BOOL bOK)
{
	m_bConnected = bOK;
}

IUnknown* CdIpm::GetIpmUnknown()
{
	CuIpm* pCtrl = GetIpmCtrl();
	if (pCtrl)
		return pCtrl->GetControlUnknown();
	return NULL;
}

