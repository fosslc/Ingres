/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : viewleft.cpp, Implementation file
**    Project  : INGRESII / Monitoring.
**    Author   : UK Sotheavut (old code from IpmView1.cpp of Emmanuel Blattes)
**    Purpose  : TreeView of the left pane
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 22-May-2003 (uk$so01)
**    SIR #106648
**    Change the serialize machanism. Do not serialize the HTREEITEM
**    because the stored HTREEITEM may different of when the tree is
**    reconstructed from the serialize.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "wmusrmsg.h"
#include "vtree.h"
#include "ipmdoc.h"
#include "viewleft.h"
#include "viewrigh.h"
#include "ipmframe.h"
#include "montree.h"   // General Monitor Tree Items
#include "treerepl.h"  // Replication Tree Items
#include "ipmctl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFindReplaceDialog* CvIpmLeft::m_pDlgFind = NULL;
UINT CvIpmLeft::WM_FINDREPLACE = ::RegisterWindowMessage(FINDMSGSTRING);


IMPLEMENT_DYNCREATE(CvIpmLeft, CTreeView)
CvIpmLeft::CvIpmLeft()
{
	m_strLastFindWhat = _T("");
	m_bMatchCase = FALSE;
	m_bWholeWord = FALSE;

	m_bProhibitActionOnTreeCtrl = FALSE;
}

CvIpmLeft::~CvIpmLeft()
{
}


BEGIN_MESSAGE_MAP(CvIpmLeft, CTreeView)
	//{{AFX_MSG_MAP(CvIpmLeft)
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
	ON_COMMAND(IDM_IPMBAR_CLOSESVR, OnMenuCloseServer)
	ON_COMMAND(IDM_IPMBAR_OPENSVR, OnMenuOpenServer)
	ON_COMMAND(IDM_IPMBAR_REFRESH, OnMenuForceRefresh)
	ON_COMMAND(IDM_IPMBAR_REPLICSVR_CHANGERUNNODE, OnMenuReplicationChangeRunNode)
	ON_COMMAND(IDM_IPMBAR_SHUTDOWN, OnMenuShutdown)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_CLOSESVR, OnUpdateMenuCloseServer)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_OPENSVR, OnUpdateMenuOpenServer)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_REFRESH, OnUpdateMenuForceRefresh)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_REPLICSVR_CHANGERUNNODE, OnUpdateMenuReplicationChangeRunNode)
	ON_UPDATE_COMMAND_UI(IDM_IPMBAR_SHUTDOWN, OnUpdateMenuShutdown)
	//}}AFX_MSG_MAP
	ON_REGISTERED_MESSAGE(WM_FINDREPLACE, OnFindReplace )
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,  OnSettingChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvIpmLeft drawing

void CvIpmLeft::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvIpmLeft diagnostics

#ifdef _DEBUG
void CvIpmLeft::AssertValid() const
{
	CTreeView::AssertValid();
}

void CvIpmLeft::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvIpmLeft message handlers

void CvIpmLeft::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();

	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	CTreeCtrl &tree = GetTreeCtrl();
	pDoc->GetPTreeGD()->SetPTree(&tree);

	CaIpmProperty& property = pDoc->GetProperty();
	tree.SendMessage (WM_SETFONT, (WPARAM)property.GetFont(), MAKELPARAM(TRUE, 0));
	if (pDoc->IsLoadedDoc())
	{
		//
		// Recreate tree lines from serialization data
		pDoc->GetPTreeGD()->FillTreeFromSerialList();
	}
}

BOOL CvIpmLeft::PreCreateWindow(CREATESTRUCT& cs) 
{
	// change the style of the tree control
	cs.style |= TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS;
	return CTreeView::PreCreateWindow(cs);
}

void CvIpmLeft::OnDestroy() 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);

	// delete itemdata associated with each item
	pDoc->GetPTreeGD()->FreeAllTreeItemsData();
	// standard management
	CTreeView::OnDestroy();
}

void CvIpmLeft::SearchItem()
{
	if (m_pDlgFind == NULL)
	{
		UINT nFlag = FR_HIDEUPDOWN|FR_DOWN;
		if (m_bWholeWord)
			nFlag |= FR_WHOLEWORD;
		if (m_bMatchCase)
			nFlag |= FR_MATCHCASE;
			
		m_pDlgFind = new CFindReplaceDialog();
		m_pDlgFind->Create (TRUE, m_strLastFindWhat, NULL, nFlag, this);
		m_pDlgFind->SetWindowPos (&wndTop, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
	}
	else
	if (IsWindow (m_pDlgFind->m_hWnd))
	{
		m_pDlgFind->SetFocus();
	}
}

LONG CvIpmLeft::OnFindReplace(WPARAM wParam, LPARAM lParam)
{
	TRACE0("CvIpmLeft::OnFindReplace\n");
	CString strFind;
	LPFINDREPLACE lpfr = (LPFINDREPLACE) lParam;
	if (!lpfr)
		return 0;
	
	if (lpfr->Flags & FR_DIALOGTERM)
	{
		m_strLastFindWhat = m_pDlgFind->GetFindString();
		m_pDlgFind = NULL;
	}
	else
	if (lpfr->Flags & FR_FINDNEXT)
	{
		//
		// Find Next:
		CString strMsg;
		m_strLastFindWhat = m_pDlgFind->GetFindString();
		
		CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
		ASSERT (pDoc);
		if (!pDoc)
			return -1L;
		CTreeCtrl& treeCtrl = GetTreeCtrl();
		HTREEITEM hItem = treeCtrl.GetSelectedItem();
		if (!hItem)
			return -1L;

		HTREEITEM hItemCurSel;
		HTREEITEM hItemNewSel;
		HTREEITEM hItemNextToCurSel;
		BOOL      bFound;
		LPTSTR    pCur, pPrec;
		static    TCHAR tchszWordSeparator[] = _T(" .*[]()\t");

		m_bMatchCase = m_pDlgFind->MatchCase();
		m_bWholeWord = m_pDlgFind->MatchWholeWord();
		strFind    = m_strLastFindWhat;
		if (!m_bMatchCase)
			strFind.MakeUpper();

		//
		// Loop starting from the current selection
		hItemCurSel = treeCtrl.GetSelectedItem();
		hItemNewSel = treeCtrl.GetNextVisibleItem(hItemCurSel);
		hItemNextToCurSel = hItemNewSel;
		while (hItemNewSel) 
		{
			CTreeItem* pItem = (CTreeItem *)treeCtrl.GetItemData(hItemNewSel);
			ASSERT(pItem);
			if (!pItem)
				return -1L;
			CString strItem = pItem->GetDisplayName();
			if (!strItem.IsEmpty()) 
			{
				if (!m_bMatchCase)
					strItem.MakeUpper();

				//
				// search
				if (m_bWholeWord) 
				{
					bFound = FALSE;
					pPrec  = (LPTSTR)(LPCTSTR)strItem;
					while (1) 
					{
						// search substring
						pCur = _tcsstr(pPrec, strFind);
						if (pCur == 0) 
						{
							bFound = FALSE;
							break;    // Not found. Cannot be found anymore. Bueno
						}

						// check previous character is a separator or first in the string
						if (pCur > (LPTSTR)(LPCTSTR)strItem)
						{
							if (_tcschr(tchszWordSeparator, *(pCur-1)) == 0) 
							{
								pPrec = pCur + 1;
								continue;   // try next occurence in the string
							}
						}

						// check ending character is a separator or terminates the string
						if (*(pCur + strFind.GetLength()) != _T('\0'))
						{
							if (_tcschr(tchszWordSeparator, *(pCur + strFind.GetLength())) == 0) 
							{
								pPrec = _tcsinc(pCur);
								continue;   // try next occurence in the string
							}
						}
						// all tests fit
						bFound = TRUE;
						break;
					}
				}
				else
					bFound = ((_tcsstr(strItem, strFind)==NULL) ? FALSE : TRUE);

				// use search result
				if (bFound) 
				{
					// special management if no other than current
					if (hItemNewSel == hItemCurSel)
						break;
					// select item, ensure visibility, and return
					treeCtrl.SelectItem(hItemNewSel);
					treeCtrl.EnsureVisible(hItemNewSel);
					return -1L;
				}
			}

			// Next item, and exit condition for the while() loop
			// Wraparoud managed here!
			hItemNewSel = treeCtrl.GetNextVisibleItem(hItemNewSel);
			if (hItemNewSel==0)
				hItemNewSel = treeCtrl.GetRootItem();
			if (hItemNewSel == hItemNextToCurSel)
				break;      // wrapped around the whole tree : not found
		}

		//
		// Not found : display message box - truncate text if too long
		if (hItemNewSel == hItemCurSel)
			AfxFormatString1(strMsg, IDS_FIND_NO_OCCURENCE, (LPCTSTR)strFind);
		else
			AfxFormatString1(strMsg, IDS_FIND_NOTFOUND, (LPCTSTR)strFind);
		AfxMessageBox (strMsg);
		m_pDlgFind->SetFocus();
	}
	return 0;
}


void CvIpmLeft::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CTreeCtrl& tree = GetTreeCtrl();
	*pResult = 0;   // default to allow expanding

	CdIpmDoc* pDoc = (CdIpmDoc*)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	// Manage "update all after load on first action"
	if (pDoc->GetPTreeGD()->CurrentlyLoading())
		return;
	if (pDoc->ManageMonSpecialState() || m_bProhibitActionOnTreeCtrl) 
	{
		*pResult = 1;   // do not allow expanding
		return;
	}

	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	if (pNMTreeView->action == TVE_EXPAND) 
	{
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;
		CTreeItem *pItem;
		pItem = (CTreeItem *)tree.GetItemData(hItem);
		if (pItem && !pItem->IsAlreadyExpanded())
		{
			if (pItem->CreateSubBranches(hItem))
				pItem->SetAlreadyExpanded(TRUE);
			else
				*pResult = 1;     // prevent expanding
		}
	}
}


void CvIpmLeft::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CTreeCtrl& tree = GetTreeCtrl();
	if (m_bProhibitActionOnTreeCtrl)
	{
		CTreeView::OnRButtonDown(nFlags, point);
		return;
	}

	HTREEITEM hHit = tree.HitTest (point, &nFlags);
	if (hHit)
		tree.SelectItem(hHit);
	CTreeView::OnRButtonDown(nFlags, point);

	// popup menu
	if (hHit) {
		CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
		ASSERT(pDoc);
		if (!pDoc)
			return;

		// get menu id according to current menuitem
		UINT menuId = pDoc->GetPTreeGD()->GetContextMenuId();
		if (!menuId)
			return;
		// load menu
		CMenu menu;
		BOOL bSuccess = menu.LoadMenu (menuId);   // Note : DestroyMenu automatic in CMenu destructor
		ASSERT (bSuccess);
		if (!bSuccess)
			return;
		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);

		CfIpmFrame* pFrame = (CfIpmFrame*)GetParent()->GetParent();   // 2 levels due to splitter wnd
		ASSERT (pFrame);
		CWnd* pWndPopupOwner = pFrame;
		ClientToScreen(&point);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWndPopupOwner);
	}
}


void CvIpmLeft::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	IPMUPDATEPARAMS ups;
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();

	try 
	{
		ASSERT(pDoc);
		if (!pDoc)
			return;
		if (pDoc->GetLockDisplayRightPane())
			return;
		CfIpmFrame* pFrame = (CfIpmFrame*)GetParentFrame();
		ASSERT (pFrame);
		if (!pFrame)
			return;
		if (!pFrame->IsAllViewCreated())
			return;

		CuDlgIpmTabCtrl* pTabDlg  = (CuDlgIpmTabCtrl*)pFrame->GetTabDialog();
		if (!pTabDlg)
			return;
		CWaitCursor doWaitCursor ;
		//
		// Notify the container of sel change:
		CIpmCtrl* pIpmCtrl = (CIpmCtrl*)pFrame->GetParent();
		if (pIpmCtrl)
			pIpmCtrl->ContainerNotifySelChange();

		memset (&ups, 0, sizeof (ups));
		CTreeCtrl& treeCtrl = GetTreeCtrl();
		CTreeItem* pItem = (CTreeItem*)treeCtrl.GetItemData (pNMTreeView->itemNew.hItem);
		if (pItem->IsNoItem() || pItem->IsErrorItem())
		{
			pTabDlg->DisplayPage (NULL);
			*pResult = 0;
			return;
		}
		if (pItem->ItemDisplaysNoPage()) {
			CString caption = pItem->ItemNoPageCaption();
			pTabDlg->DisplayPage (NULL, caption);
			*pResult = 0;
			return;
		}

		if (pItem->HasReplicMonitor())
		{
			pDoc->InitializeReplicator(pItem->GetDBName());
		}

		int nImage = -1, nSelectedImage = -1;
		CImageList* pImageList = treeCtrl.GetImageList (TVSIL_NORMAL);
		HICON hIcon = NULL;
		int nImageCount = pImageList? pImageList->GetImageCount(): 0;
		if (pImageList && treeCtrl.GetItemImage(pNMTreeView->itemNew.hItem, nImage, nSelectedImage))
		{
			if (nImage < nImageCount)
				hIcon = pImageList->ExtractIcon(nImage);
		}
		CuPageInformation* pPageInfo = pItem->GetPageInformation();

		CString strItem = pItem->GetRightPaneTitle();
		pItem->UpdateDataWhenSelChange(); // Has an effect only if the class has specialied the method.
		ups.nType   = pItem->GetType();
		ups.pStruct = pItem->GetPTreeItemData()? pItem->GetPTreeItemData()->GetDataPtr(): NULL;
		ups.pSFilter= pDoc->GetPTreeGD()->GetPSFilter();
		pPageInfo->SetUpdateParam (&ups);
		pPageInfo->SetTitle ((LPCTSTR)strItem, pItem, pDoc);

		pPageInfo->SetImage  (hIcon);
		pTabDlg->DisplayPage (pPageInfo);
	}
	catch(CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch(CResourceException* e)
	{
		AfxMessageBox (IDS_E_LOAD_DLG);
		e->Delete();
	}
	catch(...)
	{
		AfxMessageBox (IDS_E_CONSTRUCT_PROPERTY);
	}
	*pResult = 0;
}


void CvIpmLeft::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult) 
{
	CTreeCtrl& tree = GetTreeCtrl();
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);

	*pResult = 0;   // default to allow selchange
	try
	{
		// Manage "update all after load on first action"
		if (!pDoc)
			return;
		if (pDoc->GetPTreeGD()->CurrentlyLoading())
			return;
		if (pDoc->GetLockDisplayRightPane())
			return;
		if (pDoc->ManageMonSpecialState() || m_bProhibitActionOnTreeCtrl)
		{
			*pResult = 1;
			return;
		}

		NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;
		CTreeItem *pItem;
		pItem = (CTreeItem *)tree.GetItemData(hItem);
		if (pItem) {
			if (!pItem->CompleteStruct()) { //"Structure Not Completed"
				AfxMessageBox(IDS_E_STRUCTURE, MB_OK | MB_ICONEXCLAMATION);
				*pResult = 1;     // prevent selchange
				return;
			}
			// Manage old item replicator dbevents
			HTREEITEM hOldItem = pNMTreeView->itemOld.hItem;
			if (hOldItem) 
			{
				CTreeItem *pOldItem;
				pOldItem = (CTreeItem *)tree.GetItemData(hOldItem);
				ASSERT (pOldItem);
				if (pOldItem) {
					// Cannot rely on "if (!pOldItem->ItemDisplaysNoPage())"
					// because replication can have been installed/uninstalled since then
					// ---> must terminate or not according to m_ReplMonHandle value
					if (pOldItem->HasReplicMonitor()) 
					{
						pDoc->TerminateReplicator();
					}
				}
			}

			// Notify current page on the right that we are about to leave it
			CfIpmFrame* pFrame = (CfIpmFrame*)GetParentFrame();
			ASSERT (pFrame);
			if (pFrame) 
			{
				if (pFrame->IsAllViewCreated()) 
				{
					CuDlgIpmTabCtrl* pTabDlg  = (CuDlgIpmTabCtrl*)pFrame->GetTabDialog();
					if (pTabDlg)
						pTabDlg->NotifyPageSelChanging();
				}
			}
		}
	}
	catch (...)
	{
		AfxMessageBox (IDS_E_CONSTRUCT_PROPERTY);
	}
}


void CvIpmLeft::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CdIpmDoc* pDoc = (CdIpmDoc*)GetDocument();
	ASSERT (pDoc);
	if (!pDoc)
		return;
	CaIpmProperty& property = pDoc->GetProperty();

	BOOL bSetToDefaultPercentage = FALSE; //UKS
	BOOL bRegularStaticSet = TRUE;
	int nHint = (int)lHint;
	switch (nHint)
	{
	case 1: // Invoked by CdIpmDoc::Initiate()
		GetTreeCtrl().SendMessage (WM_SETFONT, (WPARAM)property.GetFont(), MAKELPARAM(TRUE, 0));
		if (bSetToDefaultPercentage) 
		{
			CRect rcClient;
			CfIpmFrame* pFrame = (CfIpmFrame*)GetParentFrame();
			ASSERT (pFrame);
			CSplitterWnd* pSplit = (CSplitterWnd *)pFrame->GetSplitterWnd();
			ASSERT (pSplit);
			pFrame->GetClientRect (rcClient);
			pSplit->SetColumnInfo(0, (int) (0.4 * (double)rcClient.Width()), 10);
			pSplit->RecalcLayout();
		}

		// Create regular static set, if needed
		if (bRegularStaticSet) 
		{
			CuTMServerStatic    *pItem1 = new CuTMServerStatic   (pDoc->GetPTreeGD());
			CuTMLockinfoStatic  *pItem3 = new CuTMLockinfoStatic (pDoc->GetPTreeGD());
			CuTMLoginfoStatic   *pItem4 = new CuTMLoginfoStatic  (pDoc->GetPTreeGD());
			CuTMAllDbStatic     *pItem5 = new CuTMAllDbStatic    (pDoc->GetPTreeGD());
			CuTMActiveUsrStatic *pItem6 = new CuTMActiveUsrStatic(pDoc->GetPTreeGD());
			CuTMReplAllDbStatic *pItem7 = new CuTMReplAllDbStatic(pDoc->GetPTreeGD());

			HTREEITEM hItem1 = pItem1->CreateTreeLine();
			HTREEITEM hItem2 = pItem3->CreateTreeLine();
			HTREEITEM hItem3 = pItem4->CreateTreeLine();
			HTREEITEM hItem4 = pItem5->CreateTreeLine();
			HTREEITEM hItem5 = pItem6->CreateTreeLine();
			HTREEITEM hItem6 = pItem7->CreateTreeLine();
			ASSERT (hItem1 && hItem2 && hItem3 && hItem4 && hItem5 && hItem6);
			CTreeCtrl& cTree = GetTreeCtrl();
			cTree.SelectItem(hItem1);
		}
		break;
	default:
		break;
	}
}

void CvIpmLeft::OnMenuCloseServer() 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	pDoc->ItemCloseServer();
}

void CvIpmLeft::OnMenuOpenServer() 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	pDoc->ItemOpenServer();
}

void CvIpmLeft::OnMenuForceRefresh() 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	pDoc->ForceRefresh();
}

void CvIpmLeft::OnMenuReplicationChangeRunNode() 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	pDoc->ItemReplicationServerChangeNode();
}

void CvIpmLeft::OnMenuShutdown() 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	pDoc->ItemShutdown();
}

void CvIpmLeft::OnUpdateMenuCloseServer(CCmdUI* pCmdUI) 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	BOOL bEnable = pDoc->IsEnabledItemCloseServer();
	pCmdUI->Enable(bEnable);
}

void CvIpmLeft::OnUpdateMenuOpenServer(CCmdUI* pCmdUI) 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	BOOL bEnable = pDoc->IsEnabledItemOpenServer();
	pCmdUI->Enable(bEnable);
}

void CvIpmLeft::OnUpdateMenuForceRefresh(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(TRUE);
}

void CvIpmLeft::OnUpdateMenuReplicationChangeRunNode(CCmdUI* pCmdUI) 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	BOOL bEnable = pDoc->IsEnabledItemReplicationServerChangeNode();
	pCmdUI->Enable(bEnable);
}

void CvIpmLeft::OnUpdateMenuShutdown(CCmdUI* pCmdUI) 
{
	CdIpmDoc* pDoc = (CdIpmDoc *)GetDocument();
	ASSERT(pDoc);
	if (!pDoc)
		return;
	BOOL bEnable = pDoc->IsEnabledItemShutdown();
	pCmdUI->Enable(bEnable);
}

long CvIpmLeft::OnSettingChange(WPARAM wParam, LPARAM lParam)
{
	UINT nMask = (UINT)wParam;
	CaIpmProperty* pProperty = (CaIpmProperty*)lParam;

	if ((nMask & IPMMASK_FONT) && pProperty && pProperty->GetFont())
	{
		CTreeCtrl& treeCtrl = GetTreeCtrl();
		treeCtrl.SendMessage (WM_SETFONT, (WPARAM)pProperty->GetFont(), MAKELPARAM(TRUE, 0));
	}
	return 0;
}
