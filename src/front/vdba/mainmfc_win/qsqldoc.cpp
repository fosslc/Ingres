/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qsqldoc.cpp : implementation file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : implementation of the CdSqlQuery class
**
** History:
**
** 18-Feb-2002 (uk$so01)
**    Created
**    SIR #106648, Integrate SQL Test ActiveX Control
** 15-Mar-2002 (schph01)
**    BUG 107331 initialize and serialize the  m_IngresVersion variable.
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "mainfrm.h"
#include "qsqlfram.h"
#include "qsqldoc.h"
#include "qsqlview.h"
#include "a2stream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
extern "C" BOOL CloseNodeStruct (int hnodestruct, BOOL bkeep); 


IMPLEMENT_DYNCREATE(CdSqlQuery, CDocument)

BEGIN_MESSAGE_MAP(CdSqlQuery, CDocument)
	//{{AFX_MSG_MAP(CdSqlQuery)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CdSqlQuery::CdSqlQuery()
{
	m_strNode = _T("");
	m_strServer = _T("");
	m_strUser = GetDefaultConnectedUser();
	m_strDatabase = _T("");
	m_bConnected = FALSE;

	m_SeqNum = 0;
	m_nNodeHandle = -1;
	m_bLoaded = FALSE;
	m_IngresVersion = -1;
	m_nDbFlag = 0;
}

CdSqlQuery::~CdSqlQuery()
{
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

void CdSqlQuery::SetNode(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strNode = lpszItem;
}

void CdSqlQuery::SetServer(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strServer = lpszItem;
}

void CdSqlQuery::SetUser(LPCTSTR lpszItem)
{
	m_bConnected=FALSE; 
	m_strUser = lpszItem;
}

void CdSqlQuery::SetDatabase(LPCTSTR lpszItem, UINT nFlag)
{
	m_bConnected=FALSE; 
	m_strDatabase = lpszItem;
	m_nDbFlag = nFlag;
}

CToolBar* CdSqlQuery::GetToolBar()
{
	CfSqlQueryFrame* pFrame = GetFrameWindow();
	ASSERT (pFrame);
	if (pFrame)
		return pFrame->GetToolBar();
	return NULL;
}

CfSqlQueryFrame* CdSqlQuery::GetFrameWindow()
{
	POSITION pos = GetFirstViewPosition();
	CView* pView = GetNextView(pos);
	ASSERT (pView);
	if (pView)
	{
		CfSqlQueryFrame* pFrame = (CfSqlQueryFrame*)pView->GetParentFrame();
		ASSERT (pFrame);
		return pFrame;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CdSqlQuery serialization

void CdSqlQuery::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		CfSqlQueryFrame* pFrame = GetFrameWindow();
		if (!pFrame)
			return;
		ar << m_strNode;
		ar << m_strServer;
		ar << m_strUser;
		ar << m_strDatabase;
		ar << m_nDbFlag;

		ar << m_SeqNum;
		ar << m_nNodeHandle;
		ar << GetTitle();
		ar << m_IngresVersion;

		BOOL bToolBarVisible = FALSE;
		CToolBar* pTbar = GetToolBar();
		if (pTbar && IsWindowVisible(pTbar->m_hWnd))
			bToolBarVisible = TRUE;
		ar << bToolBarVisible;
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
		ar >> m_strDatabase;
		ar >> m_nDbFlag;

		ar >> m_SeqNum;
		ar >> m_nNodeHandle;
		ar >> strTitle; SetTitle(strTitle);
		ar >> m_IngresVersion;

		ar >> m_bToolbarVisible;
		//
		// Full state of all toolbars in the frame
		m_toolbarState.Serialize(ar);
		//
		// Frame window placement
		ar.Read(&m_wplj, sizeof(m_wplj));
	}
	SerializeSqlControl(ar);
}


void CdSqlQuery::SerializeSqlControl(CArchive& ar)
{
	if (ar.IsStoring())
	{
		CuSqlQueryControl* pCtrl = GetSqlQueryCtrl();
		ASSERT(pCtrl);
		if (!pCtrl)
			return;
		IStream* pStream = m_sqlStream.Detach();
		if (pStream)
			pStream->Release();

		pCtrl->Storing((LPUNKNOWN*)(&pStream));
		if (pStream)
		{
			m_sqlStream.Attach(pStream);
			m_sqlStream.SeekToBegin();
			CArchiveFromIStream(ar, pStream);
		}
	}
	else
	{
		IStream* pStream = m_sqlStream.Detach();
		if (pStream)
			pStream->Release();
		CArchiveToIStream  (ar, &pStream);
		if (pStream)
			m_sqlStream.Attach(pStream);
	}
}

/////////////////////////////////////////////////////////////////////////////
// CdSqlQuery diagnostics

#ifdef _DEBUG
void CdSqlQuery::AssertValid() const
{
	CDocument::AssertValid();
}

void CdSqlQuery::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdSqlQuery commands
CuSqlQueryControl* CdSqlQuery::GetSqlQueryCtrl()
{
	POSITION pos = GetFirstViewPosition();
	while (pos != NULL)
	{
		CView* pView = GetNextView(pos);
		if (pView->IsKindOf(RUNTIME_CLASS(CvSqlQuery)))
		{
			CvSqlQuery* pSqlView = (CvSqlQuery* )pView;
			return &pSqlView->m_SqlQuery;
		}
	}
	return NULL;
}

IUnknown* CdSqlQuery::GetSqlUnknown()
{
	CuSqlQueryControl* pCtrl = GetSqlQueryCtrl();
	if (pCtrl)
		return pCtrl->GetControlUnknown();
	return NULL;
}
