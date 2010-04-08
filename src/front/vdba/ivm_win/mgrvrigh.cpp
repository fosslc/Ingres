/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mgrvrigh.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : The View that contains the Tab Control
**
**  History:
**	15-Dec-1998 (uk$so01)
**	    Created
**	31-May-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
** 07-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
*/

#include "stdafx.h"
#include "Ivm.h"
#include "mgrfrm.h"
#include "mgrvrigh.h"
#include "ivmdml.h"
#include "mainfrm.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CvManagerViewRight

IMPLEMENT_DYNCREATE(CvManagerViewRight, CView)

CvManagerViewRight::CvManagerViewRight()
{
	m_pMainTab = NULL;
}

CvManagerViewRight::~CvManagerViewRight()
{
}


BEGIN_MESSAGE_MAP(CvManagerViewRight, CView)
	//{{AFX_MSG_MAP(CvManagerViewRight)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_IVM_REMOVE_EVENT,  OnRemoveEvent)
	ON_MESSAGE (WMUSRMSG_IVM_GETHELPID,     OnGetHelpID)
	ON_MESSAGE (WMUSRMSG_IVM_COMPONENT_CHANGED, OnComponentChanged)
	ON_MESSAGE (WMUSRMSG_IVM_NEW_EVENT, OnNewEvents)
	ON_MESSAGE (WMUSRMSG_REINITIALIZE_EVENT, OnReinitializeEvent)
	ON_MESSAGE (WMUSRMSG_REACTIVATE_SELECTION, OnReactivateSelection)
	ON_MESSAGE (WMUSRMSG_NAMESERVER_STARTED, OnLastNameServerStartedup)
	ON_MESSAGE (WMUSRMSG_PANE_CREATED, OnGetCreatedPane)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvManagerViewRight drawing

void CvManagerViewRight::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvManagerViewRight diagnostics

#ifdef _DEBUG
void CvManagerViewRight::AssertValid() const
{
	CView::AssertValid();
}

void CvManagerViewRight::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvManagerViewRight message handlers

int CvManagerViewRight::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	TCHAR tchszError[] = _T("Create Tab Control Failed");
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	CdMainDoc* pDoc = (CdMainDoc*)GetDocument();
	ASSERT (pDoc);
	if (pDoc)
		pDoc->m_hwndRightView = m_hWnd;

	CfManagerFrame* pFrame = (CfManagerFrame*)GetParentFrame();
	ASSERT (pFrame);
	if (!pFrame)
		return -1;
	CWnd* pDlg = pFrame->GetParent();
	ASSERT (pDlg);
	if (!pDlg)
		return -1;

	try 
	{
		CRect r;
		DWORD dwStyle = WS_CHILD|WS_VISIBLE|TCS_TABS|TCS_SINGLELINE|TCS_RIGHTJUSTIFY;
		GetClientRect (r);
		
		m_pMainTab = new CuMainTabCtrl ();
		if (!m_pMainTab->Create (dwStyle, r, this, 100))
			throw tchszError;
		CFont* pFont = pDlg->GetFont();
		if (pFont)
			m_pMainTab->SetFont (pFont);
		else
		{
			HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
			if (hFont == NULL)
				hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
			m_pMainTab->SendMessage(WM_SETFONT, (WPARAM)hFont);
		}
	}
	catch (...)
	{
		TRACE0 ("CvManagerViewRight::OnCreate-> Failed to create Tab Control\n");
		return -1;
	}

	return 0;
}

void CvManagerViewRight::OnDestroy() 
{
	CView::OnDestroy();
	if (m_pMainTab)
		delete m_pMainTab;
}

void CvManagerViewRight::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	CRect r;
	if (m_pMainTab && IsWindow (m_pMainTab->m_hWnd))
	{
		GetClientRect (r);
		m_pMainTab->MoveWindow (r);
	}
}

LONG CvManagerViewRight::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	if (m_pMainTab && IsWindow (m_pMainTab->m_hWnd))
	{
		CWnd* pWnd = m_pMainTab->GetCurrentPage();
		if (pWnd)
			pWnd->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, wParam, lParam);
	}

	return 0L;
}

LONG CvManagerViewRight::OnRemoveEvent(WPARAM wParam, LPARAM lParam)
{
	if (m_pMainTab && IsWindow (m_pMainTab->m_hWnd))
	{
		m_pMainTab->SendMessage (WMUSRMSG_IVM_REMOVE_EVENT, wParam, lParam);
	}
	return 0;
}

LONG CvManagerViewRight::OnGetHelpID  (WPARAM wParam, LPARAM lParam)
{
	if (m_pMainTab && IsWindow (m_pMainTab->m_hWnd))
		return m_pMainTab->GetHelpID();
	return 0;
}


LONG CvManagerViewRight::OnComponentChanged  (WPARAM wParam, LPARAM lParam)
{
	if (m_pMainTab && IsWindow (m_pMainTab->m_hWnd))
		m_pMainTab->SendMessage (WMUSRMSG_IVM_COMPONENT_CHANGED, wParam, lParam);
	return 0;
}


LONG CvManagerViewRight::OnNewEvents  (WPARAM wParam, LPARAM lParam)
{
	if (m_pMainTab && IsWindow (m_pMainTab->m_hWnd))
	{
		CWnd* pWnd = m_pMainTab->GetCurrentPage();
		if (pWnd)
			pWnd->SendMessage (WMUSRMSG_IVM_NEW_EVENT, wParam, lParam);
	}
	return 0;
}


LONG CvManagerViewRight::OnReinitializeEvent(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor doWaitCursor;
	CWnd* pWndCreated = NULL;
	if (!m_pMainTab)
		return 0;
	if (!IsWindow (m_pMainTab->m_hWnd))
		return 0;
	pWndCreated = m_pMainTab->IsPageCreated (IDD_LOGEVENT_GENERIC);
	//
	// Remove All events:
	if (pWndCreated)
		pWndCreated->SendMessage (WMUSRMSG_IVM_REMOVE_EVENT, (WPARAM)1, (LPARAM)0);
	//
	// Unalert all events:
	HWND hwndViewLeft = NULL;
	CdMainDoc* pDoc = (CdMainDoc*)GetDocument();
	ASSERT (pDoc);
	if (pDoc)
		hwndViewLeft = pDoc->GetLeftViewHandle();
	if (hwndViewLeft)
		::SendMessage (hwndViewLeft, WMUSRMSG_UPDATE_UNALERT, (WPARAM)1, 0);
	return 0;
}


LONG CvManagerViewRight::OnReactivateSelection(WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CvManagerViewRight::OnReactivateSelection ...\n");
	CWaitCursor doWaitCursor;
	class CaOutScope
	{
	public:
		CaOutScope(){};
		~CaOutScope()
		{
			SetEvent (theApp.GetEventReinitializeEvent());
		}
	};
	CaOutScope oscoup;

	try
	{
		if (!m_pMainTab)
			return 0;
		if (!IsWindow (m_pMainTab->m_hWnd))
			return 0;

		CWnd* pCurrentPage = m_pMainTab->GetCurrentPage();
		if (pCurrentPage)
		{
			CfManagerFrame* pFrame = (CfManagerFrame*)GetParentFrame();
			ASSERT (pFrame);
			if (!pFrame)
				return 0;
			CvManagerViewLeft* pViewLeft = pFrame->GetLeftView();
			if (!pViewLeft)
				return 0;
			CTreeCtrl& treeCtrl = pViewLeft->GetTreeCtrl();
			HTREEITEM hSelected = treeCtrl.GetSelectedItem();
			if (!hSelected)
				return 0;
			CaTreeComponentItemData* pItem = (CaTreeComponentItemData*)treeCtrl.GetItemData (hSelected);
			CaPageInformation* pPageInfo = pItem? pItem->GetPageInformation(): NULL;
			if (pPageInfo && pItem)
			{
				pCurrentPage->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, (LPARAM)pPageInfo);
			}
		}
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CvManagerViewRight::OnReactivateSelection): \nReactivate the selection failed"));
	}

	return 0;
}


LONG CvManagerViewRight::OnLastNameServerStartedup(WPARAM wParam, LPARAM lParam)
{
	long lLast = (long)lParam;
	if (lLast < 0)
		return 0;

	CWaitCursor doWaitCursor;
	CWnd* pWndCreated = NULL;
	if (!m_pMainTab)
		return 0;
	if (!IsWindow (m_pMainTab->m_hWnd))
		return 0;
	pWndCreated = m_pMainTab->IsPageCreated (IDD_LOGEVENT_GENERIC);
	HWND hwndViewLeft = NULL;
	CdMainDoc* pDoc = (CdMainDoc*)GetDocument();
	ASSERT (pDoc);
	if (pDoc)
		hwndViewLeft = pDoc->GetLeftViewHandle();
	ASSERT (hwndViewLeft);
	if (!hwndViewLeft)
		return 0;

	//
	// Remove All events that are older than the name server startup:
	CaIvmEvent& eventData = theApp.GetEventData();
	CTypedPtrList<CObList, CaLoggedEvent*>& le = eventData.Get();
	CaLoggedEvent* pEvent = NULL;
	CaLoggedEvent* pRemoveEvent = NULL;
	POSITION p, pos = le.GetHeadPosition();
	
	while (pos != NULL)
	{
		p = pos;
		pRemoveEvent = le.GetNext (pos);

		if (pRemoveEvent->GetIdentifier() < lLast)
		{
			le.RemoveAt (p);
			//
			// Before removing event out of the cache, try to unalert on the left tree:
			if (theApp.IsAutomaticUnalert())
			{
				if (pRemoveEvent->IsAlert() && !pRemoveEvent->IsRead())
				{
					if (IsWindow (hwndViewLeft))
						::SendMessage (hwndViewLeft, WMUSRMSG_UPDATE_UNALERT, 0, (LPARAM)pRemoveEvent);
				}
			}
			delete pRemoveEvent;
		}
	}

	if (pWndCreated)
		pWndCreated->SendMessage (WMUSRMSG_IVM_REMOVE_EVENT, (WPARAM)1, (LPARAM)0);
	if (IsWindow (hwndViewLeft))
		::SendMessage (hwndViewLeft, WMUSRMSG_REACTIVATE_SELECTION, 0, (LPARAM)0);

	return 0;
}

LONG_PTR CvManagerViewRight::OnGetCreatedPane(WPARAM wParam, LPARAM lParam)
{
	CWnd* pWndCreated = NULL;
	UINT nID = (UINT)wParam;

	if (nID != -1 && (int)lParam != -1)
	{
		if (m_pMainTab && IsWindow (m_pMainTab->m_hWnd))
		{
			pWndCreated = m_pMainTab->IsPageCreated (nID);
		}
	}
	else
	{
		pWndCreated = m_pMainTab->GetCurrentPage();
	}
	
	return (LONG_PTR)pWndCreated;
}


void CvManagerViewRight::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	int nHint = (int)lHint;
	if (pSender == NULL && nHint == ID_MESSAGE_DESCRIPTION && m_pMainTab)
	{
		CfMainFrame* pFmain = (CfMainFrame*)AfxGetMainWnd();
		if (pFmain)
		{
			MSGCLASSANDID msg;
			msg.lMsgClass = -1;
			msg.lMsgFullID= -1;
			pFmain->ShowMessageDescriptionFrame(TRUE, &msg);
		}
	}
}
