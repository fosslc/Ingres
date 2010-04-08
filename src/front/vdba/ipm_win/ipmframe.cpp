/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmframe.cpp, Implementation (Manual Created Frame).
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Frame for monitor windows
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 26-Mar-2004 (uk$so01)
**    SIR #111701, The default help page is available now!. 
**    Activate the default page if no specific ID help is specified.
*/



#include "stdafx.h"
#include <htmlhelp.h>
#include "rcdepend.h"
#include "ipmframe.h"
#include "viewleft.h"   // view in left pane
#include "viewrigh.h"   // view in right pane
#include "ipmdoc.h"
#include "dgipmc02.h"   // CuDlgIpmTabCtrl
#include "runnode.h"    // CuDlgReplicationRunNode

#include "ipmfast.h"    // Fast access through double click
#include "cmddrive.h"
#include "tlsfunct.h"   // fstrncpy ...

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define BUFSIZE 256

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID)
{
	int  nPos = 0;
	CString strHelpFilePath;
	CString strTemp;
	strTemp = theApp.m_pszHelpFilePath;
#if defined(MAINWIN)
	nPos = strTemp.ReverseFind( '/'); 
#else
	nPos = strTemp.ReverseFind( '\\'); 
#endif
	if (nPos != -1) 
	{
		strHelpFilePath = strTemp.Left( nPos +1 );
		strHelpFilePath += theApp.m_strHelpFile;
	}
	TRACE1 ("APP_HelpInfo, Help Context ID = %d\n", nHelpID);
	if (nHelpID == 0)
		HtmlHelp(hWnd, strHelpFilePath, HH_DISPLAY_TOPIC, NULL);
	else
		HtmlHelp(hWnd, strHelpFilePath, HH_HELP_CONTEXT, nHelpID);

	return TRUE;
}

static int nElap = 200;
static DWORD ThreadControlExpressRefresh(LPVOID pParam)
{
	UINT* arrayParam = (UINT*)pParam;
	long nWait = 0;
	long nSleep = 0;
	while (1)
	{
		nWait = (long)arrayParam[0];
		if (nWait == 0)
			break;

		Sleep (nElap);
		nSleep += nElap;
		if (nSleep >= (nWait * 1000))
		{
			nSleep = 0;

			HWND   hFrame = (HWND)arrayParam[1];
			HANDLE hEvent = (HANDLE)arrayParam[2];

			WaitForSingleObject(hEvent, INFINITE);
			ResetEvent (hEvent);
			::PostMessage(hFrame, WM_USER_EXPRESS_REFRESH, 0, 0);
		}
	}

	return 0;
}




/////////////////////////////////////////////////////////////////////////////
// CfIpmFrame

IMPLEMENT_DYNCREATE(CfIpmFrame, CFrameWnd)

void CfIpmFrame::Init()
{
	m_bHasSplitter = TRUE;
	m_bAllViewCreated = FALSE;
	m_pIpmDoc = NULL;
	for (int i=0; i<3; i++)
		m_arrayExpressRefeshParam[i] = 0;
	m_pThreadExpressRefresh = NULL;
}

CfIpmFrame::CfIpmFrame()
{
	Init();
}

CfIpmFrame::CfIpmFrame(CdIpmDoc* pLoadedDoc)
{
	Init();
	m_pIpmDoc = pLoadedDoc;
}

CfIpmFrame::~CfIpmFrame()
{
}

CuDlgIpmTabCtrl* CfIpmFrame::GetTabDialog ()
{
	if (!IsAllViewCreated())
	{
		ASSERT (NULL);
		return NULL;
	}
	CvIpmRight* pView = (CvIpmRight*)GetRightPane(); 
	return pView->m_pIpmTabDialog;    // Can be NULL
}


BEGIN_MESSAGE_MAP(CfIpmFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CfIpmFrame)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_DBEVENT_TRACE_INCOMING, OnDbeventIncoming)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING, OnPropertiesChange)
	ON_MESSAGE (WM_USER_EXPRESS_REFRESH, OnExpressRefresh)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CfIpmFrame message handlers

CTreeItem * CfIpmFrame::GetCurSelTreeItem()
{
	CdIpmDoc* pDoc = (CdIpmDoc*)GetActiveDocument();
	ASSERT (pDoc);
	CTreeGlobalData *pGD = pDoc->GetPTreeGD();
	ASSERT (pGD);

	HTREEITEM hItem = pGD->GetPTree()->GetSelectedItem();
	if (hItem) {
		CTreeItem *pItem = (CTreeItem *)pGD->GetPTree()->GetItemData(hItem);
		ASSERT(pItem);
		return pItem;
	}
	return 0;
}

CWnd* CfIpmFrame::GetLeftPane()
{
	if (m_bHasSplitter)
		return m_WndSplitter.GetPane (0, 0);
	return NULL;
}

CWnd* CfIpmFrame::GetRightPane () 
{
	if (m_bHasSplitter)
		return m_WndSplitter.GetPane (0, 1);
	CView* pView = GetActiveView();
	ASSERT(pView);
	return pView;
}

int CfIpmFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	return 0;
}

BOOL CfIpmFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	BOOL bOK = TRUE;
	CCreateContext context1, context2;
	if (!m_pIpmDoc)
		m_pIpmDoc  = new CdIpmDoc();
	m_bHasSplitter = m_pIpmDoc->GetShowTree();
	int ncxCur = 200, ncxMin = 10;
	m_pIpmDoc->GetSpliterSize(ncxCur, ncxMin);

	if (m_bHasSplitter)
	{
		CRuntimeClass* pIpmViewRight = RUNTIME_CLASS(CvIpmRight);
		CRuntimeClass* pViewLeft     = RUNTIME_CLASS(CvIpmLeft);
		context1.m_pNewViewClass = pViewLeft;
		context1.m_pCurrentDoc   = m_pIpmDoc;
		context2.m_pNewViewClass = pIpmViewRight;
		context2.m_pCurrentDoc   = context1.m_pCurrentDoc;

		// Create a splitter of 1 rows and 2 columns.
		if (!m_WndSplitter.CreateStatic (this, 1, 2)) 
		{
			TRACE0 ("CfIpmFrame::OnCreateClient: Failed to create Splitter\n");
			return FALSE;
		}

		// associate the default view with pane 0
		if (!m_WndSplitter.CreateView (0, 0, context1.m_pNewViewClass, CSize (400, 300), &context1)) 
		{
			TRACE0 ("CfIpmFrame::OnCreateClient: Failed to create first pane\n");
			return FALSE;
		}

		// associate the CvIpmRight view with pane 1
		if (!m_WndSplitter.CreateView (0, 1, context2.m_pNewViewClass, CSize (50, 300), &context2)) 
		{
			TRACE0 ("CChildFrameC01::OnCreateClient: Failed to create second pane\n");
			return FALSE;
		}

		m_WndSplitter.SetColumnInfo(0, ncxCur, ncxMin);
		m_WndSplitter.RecalcLayout();
	}
	else
	{
		CRuntimeClass* pIpmViewRight = RUNTIME_CLASS(CvIpmRight);
		context1.m_pNewViewClass = pIpmViewRight;
		context1.m_pCurrentDoc   = m_pIpmDoc;

		bOK = CFrameWnd::OnCreateClient(lpcs, &context1);
		if (m_pIpmDoc->IsLoadedDoc()) // When m_bHasSplitter is TRUE, this block is performed in OnInitialUpdate()
		{
			//
			// Recreate tree lines from serialization data
			m_pIpmDoc->GetPTreeGD()->FillTreeFromSerialList();
			if (m_bHasSplitter)
			{
				m_WndSplitter.SetColumnInfo(0, ncxCur, ncxMin);
				m_WndSplitter.RecalcLayout();
			}
		}
	}
	
	m_bAllViewCreated = TRUE;
	return bOK;
}


// utility for print
static void CloneTreeBranch(HTREEITEM hItem, HTREEITEM hParentItem, CTreeCtrl *pTree, HWND hwndTreeLb)
{
  // create a branch corresponding to hItem
  // create equivalent line in the new tree
  CString  caption = pTree->GetItemText(hItem);
/*UKS
  LINK_RECORD   lr;
  lr.lpString       = (char *)(LPCSTR)caption;
  lr.ulRecID        = (ULONG)hItem;
  lr.ulParentID     = (ULONG)hParentItem;
  lr.ulInsertAfter  = 0;      // we don't care since sequential inserts
  lr.lpItemData     = NULL;   // no data

  if (!SendMessage(hwndTreeLb,
                   LM_ADDRECORD,
                   0,
                   (LPARAM) (LPVOID) (LINK_RECORD FAR *) &lr)) {
    ASSERT (FALSE);
    return;
  }

  // create child branches
  HTREEITEM hChildItem;
  hChildItem = pTree->GetChildItem(hItem);
  while (hChildItem) {
    CloneTreeBranch(hChildItem, hItem, pTree, hwndTreeLb);
    hChildItem = pTree->GetNextSiblingItem(hChildItem);
  }

  // set expansion mode after children create
  if (pTree->GetItemState(hItem, TVIF_STATE) & TVIS_EXPANDED)
    SendMessage(hwndTreeLb, LM_EXPAND, 0, (LPARAM)(ULONG)hItem);
  else
    SendMessage(hwndTreeLb, LM_COLLAPSE, 0, (LPARAM)(ULONG)hItem);
*/
}




long CfIpmFrame::GetHelpID()
{
	long idIpmHelp = 8004; // monitor default help ID (HELPID_MONITOR); 
	CvIpmRight* pView2 = (CvIpmRight*)GetRightPane();
	if (pView2)
	{
		UINT id = pView2->GetHelpID();
		switch (id)
		{
		case IDD_IPMDETAIL_LOCK:          return 524;
		case IDD_IPMDETAIL_LOCKINFO:      return 538;
		case IDD_IPMDETAIL_LOCKLIST:      return 516;
		case IDD_IPMDETAIL_LOGINFO:       return 521;
		case IDD_IPMDETAIL_PROCESS:       return 523;
		case IDD_IPMDETAIL_RESOURCE:      return 518;
		case IDD_IPMDETAIL_SERVER:        return 514;
		case IDD_IPMDETAIL_SESSION:       return 515;
		case IDD_IPMDETAIL_TRANSACTION:   return 525;
		case IDD_IPMICEDETAIL_ACTIVEUSER: return 741;
		case IDD_IPMICEDETAIL_CACHEPAGE:  return 739;
		case IDD_IPMICEDETAIL_CONNECTEDUSER:           return 733;
		case IDD_IPMICEDETAIL_CURSOR:     return 737;
		case IDD_IPMICEPAGE_CACHEPAGE:    return 738;
		case IDD_IPMICEDETAIL_DATABASECONNECTION:      return 743;
		case IDD_IPMICEDETAIL_HTTPSERVERCONNECTION:    return 745;
		case IDD_IPMICEDETAIL_TRANSACTION:return 735;
		case IDD_IPMICEPAGE_ACTIVEUSER:   return 740;
		case IDD_IPMICEPAGE_CONNECTEDUSER:return 732;
		case IDD_IPMICEPAGE_CURSOR:       return 736;
		case IDD_IPMICEPAGE_DATABASECONNECTION:        return 742;
		case IDD_IPMICEPAGE_HTTPSERVERCONNECTION:      return 744;
		case IDD_IPMICEPAGE_TRANSACTION:  return 734;
		case IDD_IPMPAGE_ACTIVEDB:        return 534;
		case IDD_IPMPAGE_CLIENT:          return 543;
		case IDD_IPMPAGE_HEADER:          return 539;
		case IDD_IPMPAGE_LOCKEDRESOURCES: return 532;
		case IDD_IPMPAGE_LOCKEDTABLES:    return 533;
		case IDD_IPMPAGE_LOCKLISTS:       return 527;
		case IDD_IPMPAGE_LOCKS:           return 528;
		case IDD_IPMPAGE_LOGFILE:         return 628;
		case IDD_IPMPAGE_PROCESSES:       return 531;
		case IDD_IPMPAGE_SESSIONS:        return 526;
		case IDD_IPMPAGE_STATIC_ACTIVEUSERS:           return 686;
		case IDD_IPMPAGE_STATIC_DATABASES:             return 685;
		case IDD_IPMPAGE_STATIC_REPLICATIONS:          return 687;
		case IDD_IPMPAGE_STATIC_SERVERS:  return 684;
		case IDD_IPMPAGE_TRANSACTIONS:    return 529;
		case IDD_REPCOLLISION_PAGE_RIGHT: return 646;
		case IDD_REPCOLLISION_PAGE_RIGHT2:return 688;
		case IDD_REPSERVER_PAGE_ASSIGNMENT:            return 140;
		case IDD_REPSERVER_PAGE_RAISEEVENT:            return 139;
		case IDD_REPSERVER_PAGE_STARTSETTING:          return 138;
		case IDD_REPSERVER_PAGE_STATUS:   return 137;
		case IDD_REPSTATIC_PAGE_ACTIVITY: return 552;
		case IDD_REPSTATIC_PAGE_ACTIVITY_TAB_DATABASE: return 142;
		case IDD_REPSTATIC_PAGE_ACTIVITY_TAB_TABLE:    return 143;
		case IDD_REPSTATIC_PAGE_COLLISION:return 553;
		case IDD_REPSTATIC_PAGE_DISTRIB:  return 136;
		case IDD_REPSTATIC_PAGE_INTEGRITY:return 555;
		case IDD_REPSTATIC_PAGE_RAISEEVENT:            return 556;
		case IDD_REPSTATIC_PAGE_SERVER:   return 141;
		default:
			break;
		}
	}
	return idIpmHelp;
}


void CfIpmFrame::OnClose() 
{
	CFrameWnd::OnClose();
}

CTreeGlobalData* CfIpmFrame::GetPTreeGD()
{
	CdIpmDoc* pDoc = (CdIpmDoc*)GetActiveDocument();
	ASSERT (pDoc);
	CTreeGlobalData* pGD = pDoc->GetPTreeGD();
	ASSERT (pGD);
	return pGD;
}

///////////////////////////////////////////////////////////////////////////////////////////
// For Fast search through double click

CTreeCtrl* CfIpmFrame::GetPTree()
{
	CdIpmDoc* pDoc = (CdIpmDoc*)GetActiveDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return NULL;
	CTreeGlobalData* pGD = pDoc->GetPTreeGD();
	ASSERT (pGD);
	if (!pGD)
		return NULL;
	CTreeCtrl* pTree = pGD->GetPTree();
	ASSERT (pTree);
	return pTree;
}


HTREEITEM CfIpmFrame::GetSelectedItem()
{
	HTREEITEM hItem = GetPTree()->GetSelectedItem();
	ASSERT (hItem);
	return hItem;
}

BOOL CfIpmFrame::ExpandNotCurSelItem(HTREEITEM hItem)
{
	CTreeItem *pItem = (CTreeItem *)GetPTree()->GetItemData(hItem);
	ASSERT (pItem);
	return GetPTree()->Expand(hItem, TVE_EXPAND);
}

HTREEITEM CfIpmFrame::GetChildItem(HTREEITEM hItem)
{
	return GetPTree()->GetChildItem(hItem);
}

HTREEITEM CfIpmFrame::GetNextSiblingItem(HTREEITEM hItem)
{
	return GetPTree()->GetNextSiblingItem(hItem);
}

HTREEITEM CfIpmFrame::GetRootItem()
{
	return GetPTree()->GetRootItem();
}

HTREEITEM CfIpmFrame::FindSearchedItem(CuIpmTreeFastItem* pItemPath, HTREEITEM hAnchorItem)
{
	// pîck Fast Item relevant data
	BOOL    bReqStatic = pItemPath->GetBStatic();
	int     reqType    = pItemPath->GetType();
	void*   pReqStruct = pItemPath->GetPStruct();
	if (!bReqStatic)
		ASSERT (pReqStruct);  // Need structure if not static
	BOOL    bCheckName  = pItemPath->IsCheckNameNeeded();
	CString csCheckName = pItemPath->GetCheckName();
	
	// 1) expand except if hAnchorItem is null
	if (hAnchorItem) {
		BOOL bSuccess = ExpandNotCurSelItem(hAnchorItem);
		ASSERT (bSuccess);
	}
	
	// 2) scan children or root items until found
	HTREEITEM hChildItem;
	if (hAnchorItem)
		hChildItem = GetChildItem(hAnchorItem);
	else
		hChildItem = GetRootItem();
	ASSERT (hChildItem);
	while (hChildItem) 
	{
		CTreeItem* pItem = (CTreeItem*)GetPTree()->GetItemData(hChildItem);
		ASSERT (pItem);
		if (pItem) 
		{
			if (bCheckName) {
				if (pItem->GetDisplayName() == csCheckName)
					return hChildItem;
			}
			else {
				if (bReqStatic) {
					if (pItem->GetChildType() == reqType)
						return hChildItem;
				}
				else {
					if (pItem->GetType() == reqType) {
						CTreeItemData* pData = pItem->GetPTreeItemData();
						ASSERT (pData);
						if (pData) {
							void* pStruct = pData->GetDataPtr();
							ASSERT (pStruct);
							if (pStruct) {
								int verdict = IPM_CompareInfo(reqType, pReqStruct, pStruct);
								if (verdict == 0)
									return hChildItem;
							}
						}
					}
				}
			}
		}
		hChildItem = GetNextSiblingItem(hChildItem);
	}
	ASSERT (FALSE);
	return 0L;
}

void CfIpmFrame::SelectItem(HTREEITEM hItem)
{
	CTreeGlobalData* pGD = GetPTreeGD();
	ASSERT (pGD);
	CTreeCtrl *pTreeCtrl = pGD->GetPTree();
	ASSERT (pTreeCtrl);
	pTreeCtrl->SelectItem(hItem);
}

short CfIpmFrame::FindItem(LPCTSTR lpszText, UINT mode)
{
	CString strText2Find = lpszText;
	CTreeCtrl* pTree = GetPTree();
	if (!pTree)
		return 0;
	if (strText2Find.IsEmpty())
		return 0;

	HTREEITEM hItem = pTree->GetSelectedItem();
	if (!hItem)
		return 0;
	BOOL bMatchCase = (mode & FR_MATCHCASE ? TRUE : FALSE);
	BOOL bWholeWord = (mode & FR_WHOLEWORD ? TRUE : FALSE);
	BOOL bFound = FALSE;
	TCHAR tchszWordSeparator[] = _T(" .*[]()\t");

	if (!bMatchCase)
		strText2Find.MakeUpper();

	// loop starting from the current selection
	HTREEITEM  hItemCurSel = pTree->GetSelectedItem();
	HTREEITEM hItemNewSel = pTree->GetNextVisibleItem(hItemCurSel);
	if (hItemNewSel == NULL)
		hItemNewSel = pTree->GetRootItem();
	HTREEITEM hItemNextToCurSel = hItemNewSel;
	while (hItemNewSel) 
	{
		CTreeItem *pItem = (CTreeItem *)pTree->GetItemData(hItemNewSel);
		ASSERT(pItem);
		if (!pItem)
			return 0;
		CString csRecString = pItem->GetDisplayName();
		if (!csRecString.IsEmpty()) 
		{
			if (!bMatchCase)
				csRecString.MakeUpper();
			// search
			if (bWholeWord) 
			{
				bFound = FALSE;
				TCHAR* tchszTok = _tcstok((LPTSTR)(LPCTSTR)csRecString, tchszWordSeparator);
				while (tchszTok != NULL)
				{
					if (strText2Find.Compare (tchszTok) == 0)
					{
						bFound = TRUE;
						break;
					}
				}
			}
			else
				bFound = (csRecString.Find (strText2Find) != -1);

			// use search result
			if (bFound) 
			{
				// special management if no other than current
				if (hItemNewSel == hItemCurSel)
					break;

				// select item, ensure visibility, and return
				pTree->SelectItem(hItemNewSel);
				pTree->EnsureVisible(hItemNewSel);
				break;
			}
		}

		// Next item, and exit condition for the while() loop
		// Wraparoud managed here!
		hItemNewSel = pTree->GetNextVisibleItem(hItemNewSel);
		if (hItemNewSel==0)
			hItemNewSel = pTree->GetRootItem();
		if (hItemNewSel == hItemNextToCurSel)
			break;      // wrapped around the whole tree : not found
	}

	if (!bFound)
	{
		return 0; // Not found
	}
	else
	if (hItemNewSel == hItemCurSel)
	{
		return 1; // No Other Occurrences of 'strText2Find'
	}
	else
	{
		return 2; // Found and select item
	}
}

void CfIpmFrame::OnDestroy() 
{
	if (IsAllViewCreated()) 
	{
		CuDlgIpmTabCtrl* pTabDlg  = (CuDlgIpmTabCtrl*)GetTabDialog();
		if (pTabDlg)
			pTabDlg->NotifyPageDocClose();
	}

	StartExpressRefresh(0);
	Sleep (2*nElap);
	CFrameWnd::OnDestroy();
}

BOOL CfIpmFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
	return CFrameWnd::PreCreateWindow(cs);
}

void CfIpmFrame::OnSize(UINT nType, int cx, int cy) 
{
	CFrameWnd::OnSize(nType, cx, cy);
	if (!m_bAllViewCreated)
		return;
}


CdIpmDoc* CfIpmFrame::GetIpmDoc()
{
	CView* pView = (CView*)GetRightPane();
	ASSERT(pView);
	if (pView)
		return (CdIpmDoc*)pView->GetDocument();
	return NULL;
}


LONG CfIpmFrame::OnDbeventIncoming (WPARAM wParam, LPARAM lParam)
{
	CdIpmDoc* pIpmDoc = GetIpmDoc();
	ASSERT(pIpmDoc);
	if (pIpmDoc)
	{
		//
		// Set the state of 'm_hEventRaisedDBEvents' object to nonsignaled. to prevent the
		// thread 'm_pThreadScanDBEvents'to post the message:
		ResetEvent(pIpmDoc->GetEventRaisedDBEvents());

		BOOL bHasEvent = FALSE;
		BOOL bUpdateTree = FALSE;
		RAISEDREPLICDBE raisedDBEvent;
		CuDlgIpmTabCtrl* pTab   = GetTabDialog();
		if (pTab)
		{
			CuIpmProperty* pCurrentProperty = pTab->GetCurrentProperty();
			if (pCurrentProperty)
			{
				CuPageInformation* pPageInfo = pCurrentProperty->GetPageInfo();
				if ( pPageInfo )
				{
					BOOL bReplActivity  = (pPageInfo->GetClassName().CompareNoCase (_T("CuTMDbReplStatic")) == 0);
					bReplActivity = bReplActivity || (pPageInfo->GetClassName().CompareNoCase (_T("CuTMReplAllDb")) == 0);
					CWnd* pActivityPage = bReplActivity? pTab->GetPage (pPageInfo->GetDlgID(0)): NULL;

					BOOL bContinue = TRUE;
					BOOL b2UpdateTree = FALSE;
					while (bContinue)
					{
						memset (&raisedDBEvent, 0, sizeof(raisedDBEvent));
						bContinue = IPM_GetRaisedDBEvents(pIpmDoc, &raisedDBEvent, bHasEvent, bUpdateTree);
						if (bContinue && bUpdateTree)
						{
							bUpdateTree  = FALSE;
							b2UpdateTree = TRUE;
							continue;
						}
						bContinue = bContinue && bHasEvent;
						if (bContinue)
						{
							if (pActivityPage)
								pActivityPage->SendMessage (WM_USER_DBEVENT_TRACE_INCOMING, (BOOL)bUpdateTree, (LPARAM)&raisedDBEvent);
						}
					}
					if (b2UpdateTree)
						pIpmDoc->UpdateDisplay();
				}
			}
		}

		//
		// Set the state of 'm_hEventRaisedDBEvents' object to signaled to allow
		// the thread 'm_pThreadScanDBEvents' to post the message again:
		SetEvent(pIpmDoc->GetEventRaisedDBEvents());
	}

	return 0L;
}

BOOL CfIpmFrame::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	long nHelpID =  GetHelpID();
	return APP_HelpInfo(m_hWnd, nHelpID);
}

long CfIpmFrame::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	CuDlgIpmTabCtrl* pTab = GetTabDialog ();
	if (pTab)
		pTab->PostMessage (WMUSRMSG_CHANGE_SETTING, wParam, lParam);

	CWnd* pLeft = GetLeftPane();
	if (pLeft)
		pLeft->PostMessage (WMUSRMSG_CHANGE_SETTING, wParam, lParam);

	IPM_PropertiesChange(wParam, lParam);
	return 0;
}

void CfIpmFrame::StartExpressRefresh(long nSeconds)
{
	if (m_arrayExpressRefeshParam[2] == 0)
	{
		m_arrayExpressRefeshParam[2] = (UINT)CreateEvent (NULL, TRUE, TRUE, NULL);
	}

	if (m_arrayExpressRefeshParam[0] == 0 && nSeconds > 0)
	{
		m_arrayExpressRefeshParam[0] = (UINT)nSeconds;
		m_arrayExpressRefeshParam[1] = (UINT)m_hWnd;
		m_pThreadExpressRefresh = AfxBeginThread(
			(AFX_THREADPROC)ThreadControlExpressRefresh, 
			(void*)m_arrayExpressRefeshParam, 
			THREAD_PRIORITY_NORMAL,
			0,
			CREATE_SUSPENDED);
		if (m_pThreadExpressRefresh)
			m_pThreadExpressRefresh->ResumeThread();
	}
	else
	{
		m_arrayExpressRefeshParam[0] = (UINT)nSeconds;
		m_arrayExpressRefeshParam[1] = (UINT)m_hWnd;
	}

	if (m_arrayExpressRefeshParam[0] == 0 && m_pThreadExpressRefresh)
	{
		TerminateThread (m_pThreadExpressRefresh->m_hThread, 0);
		delete m_pThreadExpressRefresh;
		m_pThreadExpressRefresh = NULL;
	}

}

long CfIpmFrame::OnExpressRefresh(WPARAM wParam, LPARAM lParam)
{
	HANDLE hEventExpressRefresh = (HANDLE)m_arrayExpressRefeshParam[2];
	CdIpmDoc* pDoc = GetIpmDoc();
	ASSERT(pDoc);
	if (pDoc)
	{
		pDoc->ExpresRefresh();
	}
	SetEvent (hEventExpressRefresh);
	return 0;
}

long CfIpmFrame::GetMonitorObjectsCount()
{
	CTreeCtrl* pTree = GetPTree();
	if (!pTree)
		return -1;
	HTREEITEM hItem = pTree->GetSelectedItem();
	if (!hItem)
		return -1;
	CTreeItem* pItem = (CTreeItem *)pTree->GetItemData(hItem);
	ASSERT(pItem);
	if (!pItem)
		return -1;
	if (!pItem->IsStatic()) 
	{
		hItem = pTree->GetParentItem(hItem);
		// Must not assert since allow non-static item on root branch, for context-driven purposes ASSERT (hItem);
		if (!hItem)
			return -1;
		pItem = (CTreeItem *)pTree->GetItemData(hItem);
		ASSERT(pItem);
	}
	if (!pItem)
		return -1;
	if (pItem->GetSubBK() != SUBBRANCH_KIND_OBJ && pItem->GetSubBK() != SUBBRANCH_KIND_MIX)
		return -1;
	if (!pItem->IsAlreadyExpanded())
		return -1;
	HTREEITEM hChildItem = pTree->GetChildItem(hItem);
	if (!hChildItem)
		return -1;
	pItem = (CTreeItem *)pTree->GetItemData(hItem);
	ASSERT(pItem);
	if (!pItem)
		return -1;
	if (pItem->IsErrorItem())
		return -1;

	if (pItem->IsNoItem())
		return 0;
	int count;
	for (count = 0; hChildItem; count++)
		hChildItem = pTree->GetNextSiblingItem(hChildItem);
	return count;
}
