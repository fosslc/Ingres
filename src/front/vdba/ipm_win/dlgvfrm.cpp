/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dlgvfrm.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame for CFormView, Scrollable Dialog box of Detail Page
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 27-Mar-2001 (noifr01)
**   (sir 104270) removal of code for managing Ingres/Desktop
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dlgvfrm.h"
#include "dlgvdoc.h"

#include "dgiclien.h"
#include "dgillist.h"
#include "dgilocks.h"
#include "dgipross.h"
#include "dgiresou.h"
#include "dgiservr.h"
#include "dgisess.h"
#include "dgitrans.h"
#include "dgilgsum.h"
#include "dgilcsum.h"
#include "dgiphead.h"

//
// Ice Monitoring:
#include "dgidcusr.h"
#include "dgidausr.h"
#include "dgidtran.h"
#include "dgidcurs.h"
#include "dgidcpag.h"
#include "dgiddbco.h"
#include "dgidhttp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



CuDlgViewFrame::CuDlgViewFrame()
{
	m_nDlgID = 0;
}

CuDlgViewFrame::CuDlgViewFrame(UINT nDlgID)
{
	m_nDlgID = nDlgID;
}


CuDlgViewFrame::~CuDlgViewFrame()
{
}


BEGIN_MESSAGE_MAP(CuDlgViewFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CuDlgViewFrame)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING,   OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,     OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,     OnGetData)
	ON_MESSAGE (WM_USER_IPMPAGE_LEAVINGPAGE, OnLeavingPage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgViewFrame message handlers

int CuDlgViewFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

BOOL CuDlgViewFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	CCreateContext context1;
	context1.m_pCurrentDoc   = new CuDlgViewDoc;

	switch (m_nDlgID)
	{
	case IDD_IPMDETAIL_LOCKINFO:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmSummaryLockInfo);
		break;
	case IDD_IPMDETAIL_LOGINFO:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmSummaryLogInfo);
		break;
	case IDD_IPMPAGE_CLIENT:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmPageClient);
		break;
	case IDD_IPMPAGE_HEADER:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmPageHeader);
		break;
	case IDD_IPMDETAIL_LOCK:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmDetailLock);
		break;
	case IDD_IPMDETAIL_LOCKLIST:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmDetailLockList);
		break;
	case IDD_IPMDETAIL_PROCESS:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmDetailProcess);
		break;
	case IDD_IPMDETAIL_RESOURCE:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmDetailResource);
		break;
	case IDD_IPMDETAIL_SERVER:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmDetailServer);
		break;
	case IDD_IPMDETAIL_SESSION:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmDetailSession);
		break;
	case IDD_IPMDETAIL_TRANSACTION:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmDetailTransaction);
		break;

	//
	// Ice Monitoring:
	case IDD_IPMICEDETAIL_ACTIVEUSER:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmIceDetailActiveUser);
		break;
	case IDD_IPMICEDETAIL_CACHEPAGE:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmIceDetailCachePage);
		break;
	case IDD_IPMICEDETAIL_CONNECTEDUSER:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmIceDetailConnectedUser);
		break;
	case IDD_IPMICEDETAIL_CURSOR:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmIceDetailCursor);
		break;
	case IDD_IPMICEDETAIL_DATABASECONNECTION:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmIceDetailDatabaseConnection);
		break;
	case IDD_IPMICEDETAIL_HTTPSERVERCONNECTION:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmIceDetailHttpServerConnection);
		break;
	case IDD_IPMICEDETAIL_TRANSACTION:
		context1.m_pNewViewClass = RUNTIME_CLASS (CuDlgIpmIceDetailTransaction);
		break;

	default:
		ASSERT (FALSE);
		break;
	}
	return CFrameWnd::OnCreateClient(lpcs, &context1);
}

LONG CuDlgViewFrame::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WM_USER_IPMPAGE_UPDATEING, wParam, lParam);
	return 0L;
}


LONG CuDlgViewFrame::OnLeavingPage(WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WM_USER_IPMPAGE_LEAVINGPAGE, wParam, lParam);
	return 0L;
}


LONG CuDlgViewFrame::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		return (LRESULT)pView->SendMessage (WM_USER_IPMPAGE_GETDATA, wParam, lParam);
	return 0L;
}

LONG CuDlgViewFrame::OnLoad (WPARAM wParam, LPARAM lParam)
{
	CView* pView = GetActiveView();
	if (pView)
		pView->SendMessage (WM_USER_IPMPAGE_LOADING, wParam, lParam);
	return 0L;
}

