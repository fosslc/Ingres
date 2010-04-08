/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmdoc.cpp : implementation File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : implementation of the CdIpm class (MFC frame/view/doc).
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
*/


#include "stdafx.h"
#include "xipm.h"
#include "ipmdoc.h"
#include "ipmview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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
	m_hMutexAccessnExpressRefreshFrequency = CreateMutex (NULL, FALSE, NULL);

	m_strWorkingFile = _T("");
	m_FilesErrors.InitializeFilesNames();
}

CdIpm::~CdIpm()
{
	ReleaseMutex(m_hMutexAccessnExpressRefreshFrequency);
	CloseHandle(m_hMutexAccessnExpressRefreshFrequency);
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



/////////////////////////////////////////////////////////////////////////////
// CdIpm serialization

void CdIpm::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strNode;
		ar << m_strServer;
		ar << m_strUser;
		ar << m_bConnected;
		ar << m_nExpressRefreshFrequency;
		ar << m_strWorkingFile;
	}
	else
	{
		ar >> m_strNode;
		ar >> m_strServer;
		ar >> m_strUser;
		ar >> m_bConnected;
		ar >> m_nExpressRefreshFrequency;
		ar >> m_strWorkingFile;
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
