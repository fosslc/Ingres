/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : levgener.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Generic Logevent Page 
**
** History:
**
** 16-Dec-1998 (uk$so01)
**    created
** 02-02-2000 (noifr01)
**    (bug 100315) rmcmd instances are now differentiated. take care of the fact that
**    there is one level less of branches in the tree than usually and use the same technique
**    as for the name server
** 02-02-2000 (noifr01)
**    (bug 100317) the filter didn't take into account, for the name server instance sub-branch, 
**     that the tree depth is one level less that the general case
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 02-Aug-2000 (uk$so01)
**    BUG #102169
**    Make the correct display when new events comming (scroll to the correct position, display
**    in the correct order). And also disable scrollbar if the control is large enought to contain
**    the items.
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 17-Oct-2002 (noifr01)
**    restored training } that had been lost in some past propagation
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#include "stdafx.h"
#include "ivm.h"
#include "mainfrm.h"
#include "mgrfrm.h"
#include "levgener.h"
#include "ivmdml.h"
#include "ivmsgdml.h"
#include "ivmdisp.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const int GLAYOUT_NUMBER = 6;
static int CALLBACK CompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

CuDlgLogEventGeneric::CuDlgLogEventGeneric(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgLogEventGeneric::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgLogEventGeneric)
	//}}AFX_DATA_INIT
	m_bInitialSize = FALSE;
	m_sizeInitial.cx = m_sizeInitial.cy = 0;

	m_nSortColumn = -1;
	m_bAsc = TRUE;

	BOOL bOwnerDraw = TRUE;
	m_cListCtrl.SetCtrlType(bOwnerDraw);
}


void CuDlgLogEventGeneric::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgLogEventGeneric)
	DDX_Control(pDX, IDC_SCROLLBAR1, m_cVerticalScroll);
	DDX_Control(pDX, IDC_EDIT3, m_cEdit3);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgLogEventGeneric, CDialog)
	//{{AFX_MSG_MAP(CuDlgLogEventGeneric)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnColumnclickList1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_IVM_REMOVE_EVENT, OnRemoveEvent)
	ON_MESSAGE (WMUSRMSG_IVM_NEW_EVENT, OnNewEvents)
	ON_MESSAGE (WMUSRMSG_REPAINT, OnRepaint)
	ON_MESSAGE (WMUSRMSG_FIND_ACCEPTED, OnFindAccepted)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgLogEventGeneric message handlers

void CuDlgLogEventGeneric::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgLogEventGeneric::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_ImageList.Create (IDB_EVENTINDICATOR, 26, 0, RGB(255,0,255));
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);
	m_cListCtrl.SetCustopmVScroll(&m_cVerticalScroll);

	long lStyle = 0;
	lStyle = m_cListCtrl.GetStyle();
	if ((lStyle & LVS_OWNERDRAWFIXED) && !m_cListCtrl.IsOwnerDraw())
	{
		ASSERT (FALSE);
		return FALSE;
	}

	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[GLAYOUT_NUMBER] =
	{
		{IDS_HEADER_EV_ALERT,      32,   LVCFMT_LEFT, FALSE},
		{IDS_HEADER_EV_DATE,      156,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_EV_COMPONENT,  90,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_EV_NAME,       52,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_EV_INSTANCE,   84,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_EV_TEXT,      700,  LVCFMT_LEFT, FALSE},
	};

	CString strHeader;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	for (int i=0; i<GLAYOUT_NUMBER; i++)
	{
		strHeader.LoadString (lsp[i].m_nIDS);
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListCtrl.InsertColumn (i, &lvcolumn, lsp[i].m_bUseCheckMark);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgLogEventGeneric::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!m_bInitialSize)
	{
		m_bInitialSize = TRUE;
		m_sizeInitial.cx = cx;
		m_sizeInitial.cy = cy;
	}

	if (cy <= m_sizeInitial.cy && IsWindow (m_cListCtrl.m_hWnd))
	{
		m_cListCtrl.ShowWindow (SW_HIDE);
		m_cVerticalScroll.ShowWindow (SW_HIDE);
		return;
	}

	ResizeControls();
}

void CuDlgLogEventGeneric::AddEvent (CaLoggedEvent* pEvent)
{
	int nImage = -1;
	int nIndex = -1;
	int nCount = m_cListCtrl.GetItemCount();
	BOOL bRead = pEvent->IsRead();

	if (bRead)
		nImage = pEvent->IsAlert()? IM_EVENT_R_ALERT: IM_EVENT_R_NORMAL;
	else
		nImage = pEvent->IsAlert()? IM_EVENT_N_ALERT: IM_EVENT_N_NORMAL;

	nIndex = m_cListCtrl.InsertItem  (nCount, _T(""), nImage);
	if (nIndex != -1)
	{
		m_cListCtrl.SetItemText (nIndex, 1, pEvent->GetDateLocale());
		m_cListCtrl.SetItemText (nIndex, 2, pEvent->m_strComponentType);
		m_cListCtrl.SetItemText (nIndex, 3, pEvent->m_strComponentName);
		m_cListCtrl.SetItemText (nIndex, 4, pEvent->m_strInstance);
		m_cListCtrl.SetItemText (nIndex, 5, pEvent->m_strText);
		m_cListCtrl.SetItemData (nIndex, (DWORD_PTR)pEvent);
	}
	pEvent->SetEventClass (CaLoggedEvent::EVENT_EXIST);
}

//
// Resulting from refresh notifying (message from the worker thread):
// This message is sent after the message WMUSRMSG_IVM_COMPONENT_CHANGED.
// So, the filter has already been updated !!
LONG CuDlgLogEventGeneric::OnNewEvents (WPARAM wParam, LPARAM lParam)
{
	CaLoggedEvent* pEvent = NULL;
	CTypedPtrList<CObList, CaLoggedEvent*>* plistEvent = NULL;
	plistEvent = (CTypedPtrList<CObList, CaLoggedEvent*>*)lParam;
	if (!plistEvent)
		return 0;

	//
	// Construct the filter:
	CaTreeComponentItemData* pItem;
	CaEventFilter filter;
	CfManagerFrame* pManagerFrame = (CfManagerFrame*)GetParentFrame();
	ASSERT (pManagerFrame);
	if (!pManagerFrame)
		return 0;
	CvManagerViewLeft* pTreeView = pManagerFrame->GetLeftView();
	if (!pTreeView)
		return 0;
	CTreeCtrl& cTree = pTreeView->GetTreeCtrl();
	HTREEITEM hSelected = cTree.GetSelectedItem();
	if (!hSelected)
		return 0;
	pItem = (CaTreeComponentItemData*)cTree.GetItemData (hSelected);
	CaPageInformation* pPageInfo = pItem? pItem->GetPageInformation(): NULL;
	if (pPageInfo)
	{
		//
		// Construct the filter:
		CaEventFilter& filter = pPageInfo->GetEventFilter();
		BOOL bNameServer = IsNameServer (pPageInfo);
		BOOL bRmcmdServer = IsRmcmdServer(pPageInfo);
		filter.m_strCategory = pItem->m_strComponentType.IsEmpty()? theApp.m_strAll: pItem->m_strComponentType;
		filter.m_strName = pItem->m_strComponentName.IsEmpty() && !bNameServer && !bRmcmdServer? theApp.m_strAll: pItem->m_strComponentName;
		filter.m_strInstance = pItem->m_strComponentInstance.IsEmpty()? theApp.m_strAll: pItem->m_strComponentInstance;

		//
		// Might be not all events in the pakage match the filter:
		POSITION p, pos = plistEvent->GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			pEvent = plistEvent->GetNext (pos);
			if (!pEvent->MatchFilter (&filter))
				plistEvent->RemoveAt (p);
		}
		if (plistEvent->IsEmpty())
			return 0;

		//
		// Array of events of the current pane:
		CObArray& arrayEvent = m_cListCtrl.GetArrayEvent();
		//
		// Init the current sort param:
		SORTPARAMS sr;
		sr.m_bAsc = m_bAsc;
		sr.m_nItem = m_nSortColumn;
		int nItemCount = m_cListCtrl.GetItemCount();
		INT_PTR nCount = arrayEvent.GetSize();
		int nIndex = -1;
		int nCountPerPage = m_cListCtrl.GetCountPerPage();
		int nPos = m_cVerticalScroll.GetScrollPosExtend();
		int nTop = m_cListCtrl.GetTopIndex();
		INT_PTR nUpperBound = arrayEvent.GetUpperBound();
		//
		// Check if the last item is visible:
		BOOL bLastItemVisible = FALSE;
		if (nItemCount > 0 && nUpperBound != -1 && nTop != -1 && (nTop + nCountPerPage - 1) >= 0)
		{
			int nLastItem = nTop + nCountPerPage - 1;
			if (nItemCount < nCountPerPage)
				nLastItem = nItemCount - 1;
			CaLoggedEvent* pLastVisible = (CaLoggedEvent*)m_cListCtrl.GetItemData(nLastItem);
			CaLoggedEvent* pLastItem    = (CaLoggedEvent*)arrayEvent.GetAt(nUpperBound);
			if (pLastVisible == pLastItem)
				bLastItemVisible = TRUE;
		}
		
		if (arrayEvent.GetSize() == 0)
		{
			arrayEvent.SetSize(plistEvent->GetCount(), 100);
		}

		if ((arrayEvent.GetSize() + plistEvent->GetCount()) < theApp.GetScrollManagementLimit())
		{
			m_cListCtrl.SetListCtrlScrollManagement(FALSE);
			m_cVerticalScroll.SetScrollRange (0, 0);
		}
		else
		{
			m_cListCtrl.SetListCtrlScrollManagement(TRUE);
			m_cVerticalScroll.SetScrollRange (0, 0);
		}

		m_cVerticalScroll.SetListCtrl    (&m_cListCtrl);
		int nMaxRange = (int)(arrayEvent.GetSize() + plistEvent->GetCount() - nCountPerPage);
		if (nMaxRange < 0)
			nMaxRange = (int)(arrayEvent.GetSize() + plistEvent->GetCount());
		m_cVerticalScroll.SetScrollRange (0, nMaxRange);
		ResizeControls();

		//
		// Save the current selected item:
		CaLoggedEvent* pEvSelected = NULL;
		int nSelectedEvent = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);
		if (nSelectedEvent != -1)
		{
			pEvSelected = (CaLoggedEvent*)m_cListCtrl.GetItemData(nSelectedEvent);
		}

		//
		// Add events that match the filter in the array and Display List:
		pos = plistEvent->GetHeadPosition();
		while (pos != NULL)
		{
			nPos = m_cVerticalScroll.GetScrollPosExtend();
			nItemCount = m_cListCtrl.GetItemCount();

			pEvent = plistEvent->GetNext (pos);
			if (sr.m_nItem == -1)
				nIndex = (int)arrayEvent.Add ((CObject*)pEvent);
			else
			{
				nIndex = IVM_GetDichoRange(
					arrayEvent, 
					0, 
					(int)(arrayEvent.GetSize() - 1),
					(CObject*)pEvent, 
					(LPARAM)&sr, 
					CompareSubItem);
				arrayEvent.InsertAt(nIndex, pEvent);
			}

			if (m_cListCtrl.IsListCtrlScrollManagement())
			{
				if (nIndex <= nPos)
				{
					m_cListCtrl.MoveAt(nPos);
					m_cVerticalScroll.SetScrollPos (nPos, TRUE, nPos);
				}
				else
				{
					if (bLastItemVisible)
					{
						//
						// If the last item is visible, then all new comming items that
						// placed after this last one will be scrolled to be visible:
						int nNewPos = nPos +1;
						if ((nItemCount + 1) > nCountPerPage)
						{
							m_cListCtrl.MoveAt(nNewPos);
							m_cVerticalScroll.SetScrollPos (nNewPos, TRUE, nNewPos);
						}
						else
						{
							m_cListCtrl.MoveAt(nPos);
							m_cVerticalScroll.SetScrollPos (nPos, TRUE, nNewPos);
						}
					}
					else
					{
						//
						// Refresh the display at the current scroll range:
						m_cListCtrl.MoveAt(nPos);
						m_cVerticalScroll.SetScrollPos (nPos, TRUE, nPos+1);
					}
				}
					
			}
			else
			{
				m_cListCtrl.AddEvent(pEvent, nIndex);
			}
		}

		//
		// Reselect the previous selected item if it is still there:
		if (pEvSelected != NULL)
		{
			int nItemCount = m_cListCtrl.GetItemCount();
			for (int i=0; i<nItemCount; i++)
			{
				if (pEvSelected == (CaLoggedEvent*)m_cListCtrl.GetItemData(i))
				{
					m_cListCtrl.SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
					break;
				}
			}
		}

		//
		// Enable/Disable scrollbar:
		nItemCount = m_cListCtrl.GetItemCount();
		if (nItemCount <= nCountPerPage)
			m_cVerticalScroll.EnableWindow (FALSE);
		else
			m_cVerticalScroll.EnableWindow (TRUE);
	}
	return 0L;
}

LONG CuDlgLogEventGeneric::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CaPageInformation* pPageInfo = (CaPageInformation*)lParam;
	if (pPageInfo)
	{
		//
		// Resulting from tree selchange:
		m_cEdit3.SetWindowText (_T(""));
		UpdateListEvents(pPageInfo);
		ResizeControls();
	}
	return 0L;
}

//
// wParam = 0, lParam = event -> Remove event from the list
// wParam = 1, lParam = 0      -> Remove all events from the list
LONG CuDlgLogEventGeneric::OnRemoveEvent(WPARAM wParam, LPARAM lParam)
{
	CaLoggedEvent* pEvent = (CaLoggedEvent*)lParam;
	CObArray& arrayEvent = m_cListCtrl.GetArrayEvent();

	switch ((int)wParam)
	{
	case 0:
		m_cListCtrl.RemoveEvent (pEvent);
		break;
	case 1:
		arrayEvent.RemoveAll();
		// This function causes to flash all apps in the whole desktop:
		// m_cListCtrl.LockWindowUpdate();
		m_cListCtrl.SetControlMaxEvent(FALSE);
		m_cListCtrl.DeleteAllItems();
		m_cListCtrl.SetControlMaxEvent(TRUE);
		// This function causes to flash all apps in the whole desktop:
		// m_cListCtrl.UnlockWindowUpdate();
		m_cEdit3.Clear();
		m_cEdit3.SetWindowText(_T(""));
		break;
	default:
		ASSERT (FALSE);
		break;
	}
	return 0;
}



void CuDlgLogEventGeneric::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	LV_ITEM lvitem;
	int nImage = -1;
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	*pResult = 0;
	if (pNMListView->uNewState && pNMListView->iItem >= 0)
	{
		CaLoggedEvent* pEvent = (CaLoggedEvent*)m_cListCtrl.GetItemData (pNMListView->iItem);
		if (!pEvent)
			return;
		m_cEdit3.SetWindowText (pEvent->m_strText);
		CfMainFrame* pFmain = (CfMainFrame*)AfxGetMainWnd();
		if (pFmain)
		{
			MSGCLASSANDID msg;
			msg.lMsgClass = pEvent->GetCatType();
			msg.lMsgFullID= pEvent->GetCode();
			pFmain->ShowMessageDescriptionFrame(FALSE, &msg);
		}
		SetFocus();
		if (pEvent->IsRead())
			return;
	
		pEvent->SetRead (TRUE);
		IVM_SetReadEvent(pEvent->GetIdentifier(), TRUE);
		memset (&lvitem, 0, sizeof (lvitem));
		lvitem.mask   = LVIF_IMAGE;
		lvitem.iImage = pEvent->IsAlert()? IM_EVENT_R_ALERT: IM_EVENT_R_NORMAL;
		lvitem.iItem = pNMListView->iItem;
		m_cListCtrl.SetItem (&lvitem);
		//
		// Try to unmark (not alert) in the Tree Control:
		CfManagerFrame* pManagerFrame = (CfManagerFrame*)GetParentFrame();
		ASSERT (pManagerFrame);
		if (!pManagerFrame)
			return;
		CvManagerViewLeft* pTreeView = pManagerFrame->GetLeftView();
		if (pTreeView && pEvent->IsAlert())
		{
			pTreeView->AlertRead (pEvent);
		}
	}
}




void CuDlgLogEventGeneric::UpdateListEvents(CaPageInformation* pPageInfo)
{
	INT_PTR nEventCount = 0;
	CaLoggedEvent* pEvent = NULL;
	if (!m_listLoggedEvent.IsEmpty())
		m_listLoggedEvent.RemoveAll();

	m_nSortColumn = -1;
	m_bAsc = TRUE;
	CaIvmTree& treeData = theApp.GetTreeData();
	if (pPageInfo)
	{
		CaIvmEvent& loggedEvent = theApp.GetEventData();
		CaTreeComponentItemData* pItem = pPageInfo->GetIvmItem();
		CaEventFilter& filter = pPageInfo->GetEventFilter();
		if (pPageInfo->GetClassName().CompareNoCase(_T("INSTALLATION")) == 0)
		{
			//
			// Performance tuning !!!
			// If the selection is on the INSTALLATION, do not compare with the filter.
			loggedEvent.GetAll (m_listLoggedEvent);
			nEventCount = m_listLoggedEvent.GetCount();
			m_cListCtrl.SetListEvent(&m_listLoggedEvent);
		}
		else
		{
			if (pItem)
			{
				BOOL bNameServer = IsNameServer (pPageInfo);
				BOOL bRmcmdServer = IsRmcmdServer(pPageInfo);

				filter.m_strCategory = pItem->m_strComponentType.IsEmpty()? theApp.m_strAll: pItem->m_strComponentType;
				filter.m_strName = pItem->m_strComponentName.IsEmpty() && !bNameServer && !bRmcmdServer? theApp.m_strAll: pItem->m_strComponentName;
				filter.m_strInstance = pItem->m_strComponentInstance.IsEmpty()? theApp.m_strAll: pItem->m_strComponentInstance;
			}
			
			loggedEvent.Get (m_listLoggedEvent, &filter);
			nEventCount = m_listLoggedEvent.GetCount();
			m_cListCtrl.SetListEvent(&m_listLoggedEvent);
		}

		if (m_nSortColumn != -1)
		{
			//
			// The list has been sorted, so sort it again:
			SORTPARAMS sr;
			sr.m_bAsc = m_bAsc;
			sr.m_nItem = m_nSortColumn;
			CObArray& arrayEvent = m_cListCtrl.GetArrayEvent();
			CProgressCtrl cProgress;
			CfMainFrame* pFmain = (CfMainFrame*)AfxGetMainWnd();
			if (pFmain)
			{
				CStatusBar& statbar = pFmain->GetStatusBarControl();
				RECT rc;
				statbar.GetItemRect (0, &rc);
				VERIFY (cProgress.Create(WS_CHILD | WS_VISIBLE, rc, &statbar, 1));
			}
			IVM_DichotomySort(arrayEvent, CompareSubItem, (LPARAM)&sr, &cProgress);
		}

		int nCountPerPage = m_cListCtrl.GetCountPerPage();
		if (nEventCount < theApp.GetScrollManagementLimit())
		{
			m_cListCtrl.SetListCtrlScrollManagement(FALSE);
			m_cVerticalScroll.SetScrollRange (0, 0);
		}
		else
		{
			m_cListCtrl.SetListCtrlScrollManagement(TRUE);
			m_cVerticalScroll.SetScrollRange (0, 0);
		}
		ResizeControls();

		m_cVerticalScroll.SetListCtrl    (&m_cListCtrl);
		m_cVerticalScroll.SetScrollRange (0, (int)(nEventCount - nCountPerPage));
		m_cVerticalScroll.SetScrollPos   (0);
		m_cListCtrl.InitializeItems (GLAYOUT_NUMBER);
	}
}

static int AlertNotifyOrder (CaLoggedEvent* lpItem)
{
	//
	// Ascending order:
	if (lpItem->IsAlert())
		return !lpItem->IsRead() ? 1: 2;
	else
		return !lpItem->IsRead() ? 3: 4;
}


static int CALLBACK CompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC;
	CaLoggedEvent* lpItem1 = (CaLoggedEvent*)lParam1;
	CaLoggedEvent* lpItem2 = (CaLoggedEvent*)lParam2;
	SORTPARAMS* lpSortData = (SORTPARAMS*)lParamSort;
	BOOL bAsc = lpSortData? lpSortData->m_bAsc: TRUE;
	int  nType= lpSortData? lpSortData->m_nItem:-1;

	if (nType == -1)
		return 0;

	if (lpItem1 && lpItem2)
	{
		nC = bAsc? 1 : -1;
		switch (nType)
		{
		case 0: // Alert / Notify
			return nC * (AlertNotifyOrder(lpItem1) > AlertNotifyOrder(lpItem2) ? 1: -1);
		case 1: // Date/Time
			//
			// Use the Unique ID of event instead of Date/time
			return nC * ((lpItem1->GetIdentifier() > lpItem2->GetIdentifier()) ? 1: -1);
		case 2: // Component Type
			return nC * lpItem1->m_strComponentType.CompareNoCase (lpItem2->m_strComponentType);
		case 3: // Componrent Name
			return nC * lpItem1->m_strComponentName.CompareNoCase (lpItem2->m_strComponentName);
		case 4: // Instance
			return nC * lpItem1->m_strInstance.CompareNoCase (lpItem2->m_strInstance);
		case 5: // Text
			return nC * lpItem1->m_strText.CompareNoCase (lpItem2->m_strText);
		default:
			return 0;
		}
	}
	else
	{
		nC = lpItem1? 1: lpItem2? -1: 0;
		return bAsc? nC: (-1)*nC;
	}

	return 0;
}

void CuDlgLogEventGeneric::OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;
	CWaitCursor doWaitCursor;
	SORTPARAMS sr;

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	m_bAsc = (m_nSortColumn == pNMListView->iSubItem)? !m_bAsc: TRUE;
	m_nSortColumn = pNMListView->iSubItem;
	sr.m_bAsc = m_bAsc;
	sr.m_nItem = m_nSortColumn;

	SortByInsert((LPARAM)&sr);
}



LONG CuDlgLogEventGeneric::OnRepaint (WPARAM wParam, LPARAM lParam)
{
	CRect r;
	m_cListCtrl.GetWindowRect(r);
	ScreenToClient (r);
	r.bottom = r.top + 40;
	InvalidateRect (r);
	return 0L;
}

void CuDlgLogEventGeneric::SortByInsert(LPARAM l)
{
	CWaitCursor doWaitCursor;
	if (!l)
		return;

	CaEventDataMutex mutextEvent(INFINITE);
	CObArray& arrayEvent = m_cListCtrl.GetArrayEvent();
	CProgressCtrl cProgress;
	CfMainFrame* pFmain = (CfMainFrame*)AfxGetMainWnd();
	if (pFmain)
	{
		CStatusBar& statbar = pFmain->GetStatusBarControl();
		RECT rc;
		statbar.GetItemRect (0, &rc);
		VERIFY (cProgress.Create(WS_CHILD | WS_VISIBLE, rc, &statbar, 1));
	}

	IVM_DichotomySort(arrayEvent, CompareSubItem, l, &cProgress);
	m_cListCtrl.Sort (l, CompareSubItem);
}

void CuDlgLogEventGeneric::ResizeControls()
{
	//
	// Resize Controls:
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r, rScroll;
	int H, nYMark;
	GetClientRect (r);
	int cx = r.right;
	int cy = r.bottom;

	m_cEdit3.GetWindowRect (r);
	ScreenToClient (r);
	H = r.Height();
	r.bottom = cy - 2;
	r.top  = r.bottom - H;
	r.left = 2;
	r.right= cx-2;
	m_cEdit3.MoveWindow (r);
	nYMark = r.top;

	m_cListCtrl.GetWindowRect (r);
	ScreenToClient (r);
	r.bottom = nYMark - 4;
	r.left = 2;
	r.right= cx-2;

	if (m_cListCtrl.IsListCtrlScrollManagement())
	{
		m_cVerticalScroll.GetWindowRect (rScroll);
		r.right -= rScroll.Width();
	}

	m_cListCtrl.MoveWindow (r);
	m_cListCtrl.ShowWindow (SW_SHOW);
	
	if (m_cListCtrl.IsListCtrlScrollManagement())
	{
		r.right  = cx-2;
		r.left   = r.right - rScroll.Width();
		r.top   += 1;
		r.bottom-= 1;
		m_cVerticalScroll.MoveWindow (r);
		m_cVerticalScroll.ShowWindow (SW_SHOW);
	}
	else
		m_cVerticalScroll.ShowWindow (SW_HIDE);

	if (m_cListCtrl.IsListCtrlScrollManagement())
	{
		//
		// Array of events of the current pane:
		CObArray& arrayEvent = m_cListCtrl.GetArrayEvent();
		int nItemCount = m_cListCtrl.GetItemCount();
		int nCountPerPage = m_cListCtrl.GetCountPerPage();
		int nPos = m_cVerticalScroll.GetScrollPosExtend();
		int nTop = m_cListCtrl.GetTopIndex();
		INT_PTR nUpperBound = arrayEvent.GetUpperBound();
		BOOL bLastItemVisible = FALSE;
		if (nItemCount > 0 && nUpperBound != -1 && nTop != -1 && (nTop + nCountPerPage - 1) >= 0)
		{
			int nLastItem = nTop + nCountPerPage - 1;
			if (nItemCount < nCountPerPage)
				nLastItem = nItemCount - 1;
			CaLoggedEvent* pLastVisible = (CaLoggedEvent*)m_cListCtrl.GetItemData(nLastItem);
			CaLoggedEvent* pLastItem    = (CaLoggedEvent*)arrayEvent.GetAt(nUpperBound);
			if (pLastVisible == pLastItem)
				bLastItemVisible = TRUE;
		}

		if (bLastItemVisible)
		{
			CaLoggedEvent* pEvSelected = NULL;
			int nSelectedEvent = m_cListCtrl.GetNextItem (-1, LVNI_SELECTED);
			if (nSelectedEvent != -1)
			{
				pEvSelected = (CaLoggedEvent*)m_cListCtrl.GetItemData(nSelectedEvent);
			}

			if (nUpperBound+1 <= nCountPerPage)
			{
				nPos = 0;
				m_cListCtrl.MoveAt(nPos);
				m_cVerticalScroll.SetScrollPos (nPos, TRUE, nPos);
				m_cVerticalScroll.EnableWindow(FALSE);
			}
			else
			{
				m_cVerticalScroll.EnableWindow(TRUE);
				nPos = (int)(nUpperBound - nCountPerPage + 1);
				if (nPos >= 0 && nPos <= nUpperBound)
				{
					m_cListCtrl.MoveAt(nPos);
					m_cVerticalScroll.SetScrollPos (nPos, TRUE, nPos);
				}
			}
			//
			// Reselect the previous selected item if it is still there:
			if (pEvSelected != NULL)
			{
				int nItemCount = m_cListCtrl.GetItemCount();
				for (int i=0; i<nItemCount; i++)
				{
					if (pEvSelected == (CaLoggedEvent*)m_cListCtrl.GetItemData(i))
					{
						m_cListCtrl.SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
						break;
					}
				}
			}
		}
	}
}

LONG CuDlgLogEventGeneric::OnFindAccepted (WPARAM wParam, LPARAM lParam)
{
	return (LONG)TRUE;
}


LONG CuDlgLogEventGeneric::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 (_T("CuDlgLogEventGeneric::OnFind \n"));
	return m_cListCtrl.FindText(wParam, lParam);
}

void CuDlgLogEventGeneric::OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	int nSel = pNMListView->iItem;
	ShowMessageDescriptionFrame(nSel);

	*pResult = 0;
}

void CuDlgLogEventGeneric::ShowMessageDescriptionFrame(int nSel)
{
	MSGCLASSANDID msg;
	msg.lMsgClass = -1;
	msg.lMsgFullID= -1;
	CfMainFrame* pFmain = (CfMainFrame*)AfxGetMainWnd();

	if (nSel >= 0 && (m_cListCtrl.GetItemState (nSel, LVIS_SELECTED)&LVIS_SELECTED))
	{
		CaLoggedEvent* pEvent = (CaLoggedEvent*)m_cListCtrl.GetItemData(nSel);
		if (pEvent)
		{
			msg.lMsgClass = pEvent->GetCatType();
			msg.lMsgFullID= pEvent->GetCode();
		}
	}
	if (pFmain)
	{
		pFmain->ShowMessageDescriptionFrame(TRUE, &msg);
	}
}

