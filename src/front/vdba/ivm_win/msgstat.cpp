/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : msgstat.cpp, Implementation file    (Modeless Dialog)
**    Project  : Visual Manager.
**    Author   : UK Sotheavut
**    Purpose  : Statistic pane of IVM.
**
**  History:
**	08-Jul-1999 (uk$so01)
**	    created
**	02-02-2000 (noifr01)
**	    (bug 100315) rmcmd instances are now differentiated. take care
**	    of the fact that there is one level less of branches in the
**	    tree than usually and use the same technique as for the name
**	    server
**	01-Mar-2000 (uk$so01)
**	    SIR #100635, Add the Functionality of Find operation.
**	24-May-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	12-Fev-2001 (noifr01)
**	    bug 102974 (manage multiline errlog.log messages)
**	14-Aug-2001 (uk$so01)
**	    SIR #105384 (Adjust the header width)
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
** 12-Mar-2003 (uk$so01)
**    SIR #109773, Activate the "Find" button for Event Statistic pane
**    because now we allow the items be selected.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "ivmdisp.h"
#include "mgrfrm.h"
#include "msgstat.h"
#include "ivmdml.h"
#include "wmusrmsg.h"
#include "findinfo.h"
#include "mainfrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const int LAYOUT_NUMBER_STATITEM = 3;


static int CALLBACK CompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC, n;

	SORTPARAMS* sp = (SORTPARAMS*)lParamSort;
	int nSubItem = sp->m_nItem;
	BOOL bAsc = sp->m_bAsc;

	CaMessageStatisticItemData* lpItem1 = (CaMessageStatisticItemData*)lParam1;
	CaMessageStatisticItemData* lpItem2 = (CaMessageStatisticItemData*)lParam2;

	if (lpItem1 && lpItem2)
	{
		nC = bAsc? 1 : -1;
		switch (nSubItem)
		{
		case 0:
			return nC * lpItem1->GetTitle().CompareNoCase (lpItem2->GetTitle());
		default:
			n = (lpItem1->GetCount() > lpItem2->GetCount())? 1: (lpItem1->GetCount() < lpItem2->GetCount())? -1: (lpItem1->GetCountExtra() > lpItem2->GetCountExtra())? 1: (lpItem1->GetCountExtra() < lpItem2->GetCountExtra())? -1: 0;
			return nC * n;
		}
	}
	return 0;
}

static CaMessageStatisticItemData* FindMessage(CTypedPtrList<CObList, CaMessageStatisticItemData*>& listMsg, CaLoggedEvent* pEvent)
{
	if (listMsg.IsEmpty())
		return NULL;
	CaMessageStatisticItemData* pMsg = NULL;
	POSITION pos = listMsg.GetHeadPosition();
	while (pos != NULL)
	{
		pMsg = listMsg.GetNext (pos);
		if (pMsg->GetCode() == pEvent->GetCode())
			return pMsg;
	}
	return NULL;
}


CuDlgMessageStatistic::CuDlgMessageStatistic(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgMessageStatistic::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgMessageStatistic)
	//}}AFX_DATA_INIT
	m_sort.m_nItem = 1;
	m_sort.m_bAsc = FALSE;
}

void CuDlgMessageStatistic::CleanStatisticItem()
{
	CaMessageStatisticItemData* pItemStat = NULL;
	int i, nCount = m_cListStatItem.GetItemCount();
	for (i=0; i<nCount; i++)
	{
		pItemStat = (CaMessageStatisticItemData*)m_cListStatItem.GetItemData (i);
		if (pItemStat)
			delete pItemStat;
	}
	m_cListStatItem.DeleteAllItems();
}



void CuDlgMessageStatistic::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgMessageStatistic)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgMessageStatistic, CDialog)
	//{{AFX_MSG_MAP(CuDlgMessageStatistic)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnColumnclickList1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING,   OnUpdateData)
	ON_MESSAGE (WMUSRMSG_IVM_NEW_EVENT, OnNewEvents)
	ON_MESSAGE (WMUSRMSG_FIND_ACCEPTED, OnFindAccepted)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgMessageStatistic message handlers



void CuDlgMessageStatistic::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgMessageStatistic::OnDestroy() 
{
	CleanStatisticItem();
	CDialog::OnDestroy();
}

BOOL CuDlgMessageStatistic::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_ImageList.Create(1, 18, TRUE, 1, 0);
	VERIFY (m_cListStatItem.SubclassDlgItem (IDC_LIST1, this));
	m_cListStatItem.SetImageList (&m_ImageList, LVSIL_SMALL);
	
	int i;
	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER_STATITEM] =
	{
		{IDS_HEADER_STAT_MESSAGE,  240,  LVCFMT_LEFT,  FALSE},
		{IDS_HEADER_STAT_COUNT,     65,  LVCFMT_RIGHT, FALSE},
		{IDS_HEADER_STAT_GRAPH,    160,  LVCFMT_LEFT,  FALSE}
	};

	//
	// Set the number of columns: LAYOUT_NUMBER_COLUMN and LAYOUT_NUMBER_STATITEM
	//
	CString strHeader;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (i=0; i<LAYOUT_NUMBER_STATITEM; i++)
	{
		strHeader.LoadString (lsp[i].m_nIDS);
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListStatItem.InsertColumn (i, &lvcolumn);
	}

	return TRUE;  
	// return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgMessageStatistic::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListStatItem.m_hWnd))
		return;
	CRect rcDlg;
	GetClientRect (rcDlg);
	m_cListStatItem.MoveWindow (rcDlg);
}


LONG CuDlgMessageStatistic::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CTypedPtrList<CObList, CaMessageStatisticItemData*> listMsg;
	try
	{
		CleanStatisticItem();
		CaTreeComponentItemData* pItem = NULL;
		CaPageInformation* pPageInfo = (CaPageInformation*)lParam;
		if (pPageInfo)
			pItem = pPageInfo->GetIvmItem();
		if (!pItem)
			return 0L;

		//
		// Get events from the cache that match the filter and
		// and Add them to the list control:
		CaEventFilter& filter = pPageInfo->GetEventFilter();
		BOOL bNameServer = IsNameServer (pPageInfo);
		BOOL bRmcmdServer = IsRmcmdServer(pPageInfo);
		filter.m_strCategory = pItem->m_strComponentType.IsEmpty()? theApp.m_strAll: pItem->m_strComponentType;
		filter.m_strName = pItem->m_strComponentName.IsEmpty() && !bNameServer && !bRmcmdServer? theApp.m_strAll: pItem->m_strComponentName;
		filter.m_strInstance = pItem->m_strComponentInstance.IsEmpty()? theApp.m_strAll: pItem->m_strComponentInstance;

		CaMessageStatisticItemData* pExistMsg = NULL;
		CaLoggedEvent* pEvent = NULL;
		CTypedPtrList<CObList, CaLoggedEvent*> listLoggedEvent;
		CaIvmEvent& loggedEvent = theApp.GetEventData();
		loggedEvent.Get (listLoggedEvent, &filter);

		POSITION pos = listLoggedEvent.GetHeadPosition();
		while (pos != NULL)
		{
			pEvent = listLoggedEvent.GetNext(pos);
			pExistMsg = FindMessage(listMsg, pEvent);
			if (pExistMsg)
			{
				if (pEvent->IsNotFirstLine()) {
					int nCount = pExistMsg->GetCountExtra() + 1;
					pExistMsg->SetCountExtra(nCount);
				}
				else {
					int nCount = pExistMsg->GetCount() + 1;
					pExistMsg->SetCount(nCount);
				}
				continue;
			}
			else
			{
				CaMessageStatisticItemData* pItemStatic = NULL;
				int iNormal, iExtra;
				if (pEvent->IsNotFirstLine()) {
					iNormal = 0;
					iExtra  = 1;
				}
				else {
					iNormal = 1;
					iExtra  = 0;
				}
				pItemStatic = new CaMessageStatisticItemData (pEvent->m_strText, pEvent->GetCode(), iNormal,iExtra);
				pItemStatic->SetCatType(pEvent->GetCatType());
				listMsg.AddTail(pItemStatic);
			}
		}
		DrawStatistic(listMsg);
	}
	catch (CMemoryException* e)
	{
		e->Delete();
		IVM_OutOfMemoryMessage();
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(CuDlgMessageStatistic::OnUpdateData): \nCannot display the statistic of the message."));
	}
	return 0L;
}



void CuDlgMessageStatistic::DrawStatistic (CTypedPtrList<CObList, CaMessageStatisticItemData*>& listMsg)
{
	//
	// Draw the statistic:
	int nCount = 0;
	int nMaxCount = 0;
	CaMessageStatisticItemData* pItemStatic = NULL;
	POSITION pos = listMsg.GetHeadPosition();
	while (pos != NULL)
	{
		pItemStatic = listMsg.GetNext (pos);
		if (pItemStatic->GetCount() > nMaxCount)
			nMaxCount = pItemStatic->GetCount();
	}

	int nMaxMessageLen = 0, nMaxCountLen = 0;
	CString strItem;
	CString strLongMessage = _T("");
	CString strLongCount = _T("");
	m_cListStatItem.SetMaxCountGroup(nMaxCount);
	while (!listMsg.IsEmpty())
	{
		pItemStatic = listMsg.RemoveHead();
		strItem = IVM_IngresGenericMessage (pItemStatic->GetCode(), pItemStatic->GetTitle());
		if (nMaxMessageLen < strItem.GetLength())
		{
			nMaxMessageLen = strItem.GetLength();
			strLongMessage = strItem;
		}

		if (pItemStatic->GetCountExtra()==0)
			strItem.Format (_T("%d"), pItemStatic->GetCount());
		else
			strItem.Format (_T("%d(+%d)"), pItemStatic->GetCount(), pItemStatic->GetCountExtra());
		if (nMaxCountLen < strItem.GetLength())
		{
			nMaxCountLen = strItem.GetLength();
			strLongCount = strItem;
		}

		m_cListStatItem.InsertItem  (nCount, _T(""));
		m_cListStatItem.SetItemData (nCount, (DWORD_PTR)pItemStatic);
		nCount++;
	}
	m_cListStatItem.SetAdjustInfo (strLongMessage, strLongCount);
	m_cListStatItem.SortItems(CompareSubItem, (LPARAM)&m_sort);
}


void CuDlgMessageStatistic::OnColumnclickList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	SORTPARAMS sp;
	sp.m_nItem = pNMListView->iSubItem;
	if (m_sort.m_nItem == sp.m_nItem)
		sp.m_bAsc = !m_sort.m_bAsc;
	else
		sp.m_bAsc = TRUE;
	memcpy (&m_sort, &sp, sizeof(m_sort));
	m_cListStatItem.SortItems(CompareSubItem, (LPARAM)&sp);
	*pResult = 0;
}

//
// There are new events, so refresh the pane:
LONG CuDlgMessageStatistic::OnNewEvents (WPARAM wParam, LPARAM lParam)
{
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

	if (pPageInfo && pItem)
	{
		SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, (LPARAM)pPageInfo);
	}
	return 0L;
}

LONG CuDlgMessageStatistic::OnFindAccepted (WPARAM wParam, LPARAM lParam)
{
	return (LONG)TRUE;
}


//
// Because we do not allow to select item in Statistic List Control
// then this function is not invoked actually:
LONG CuDlgMessageStatistic::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgMessageStatistic::OnFind \n");
	CaFindInformation* pFindInfo = (CaFindInformation*)lParam;
	ASSERT (pFindInfo);
	if (!pFindInfo)
		return 0L;
	CuListCtrlMessageStatistic* pCtrl = &m_cListStatItem;

	ASSERT (pCtrl);
	if (!pCtrl)
		return 0L;

	CString strItem;
	int i, nStart, nCount = pCtrl->GetItemCount();
	CString strWhat = pFindInfo->GetWhat();
	if (nCount == 0)
		return FIND_RESULT_NOTFOUND;
	if (pFindInfo->GetListWindow() != pCtrl || pFindInfo->GetListWindow() == NULL)
	{
		pFindInfo->SetListWindow(pCtrl);
		pFindInfo->SetListPos (0);
	}
	if (!(pFindInfo->GetFlag() & FR_MATCHCASE))
		strWhat.MakeUpper();
	
	nStart = pFindInfo->GetListPos();
	CaMessageStatisticItemData* pStat = NULL;
	for (i=pFindInfo->GetListPos (); i<nCount; i++)
	{
		pStat = (CaMessageStatisticItemData*)pCtrl->GetItemData(i);
		pFindInfo->SetListPos (i);
		if (!pStat)
			continue;

		int nCol = 0;
		while (nCol < (LAYOUT_NUMBER_STATITEM -1))
		{
			switch (nCol)
			{
			case 0:
				strItem = pStat->GetTitle();
				break;
			case 1:
				strItem.Format (_T("%d"), pStat->GetCount());
				break;
			default:
				ASSERT (FALSE);
				break;
			}

			if (!(pFindInfo->GetFlag() & FR_MATCHCASE))
				strItem.MakeUpper();

			int nBeginFind = 0, nLen = strItem.GetLength();
			int nFound = -1;

			if (!(pFindInfo->GetFlag() & FR_WHOLEWORD))
			{
				nFound = strItem.Find (strWhat);
				if (nFound != -1)
				{
					pFindInfo->SetListPos (i+1);
					pCtrl->EnsureVisible(i, FALSE);
					pCtrl->SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

					return FIND_RESULT_OK;
				}
			}
			else
			{
				TCHAR* chszSep = (TCHAR*)(LPCTSTR)theApp.m_strWordSeparator; // Seperator of whole word.
				TCHAR* token = NULL;
				// 
				// Establish string and get the first token:
				token = _tcstok ((TCHAR*)(LPCTSTR)strItem, chszSep);
				while (token != NULL )
				{
					//
					// While there are tokens in "strResult"
					if (strWhat.Compare (token) == 0)
					{
						pFindInfo->SetListPos (i+1);
						pCtrl->EnsureVisible(i, FALSE);
						pCtrl->SetItemState (i, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

						return FIND_RESULT_OK;
					}

					token = _tcstok(NULL, chszSep);
				}
			}

			//
			// Next column:
			nCol++;
		}
	}

	if (nCount > 0 && nStart > 0)
		return FIND_RESULT_END;
	return FIND_RESULT_NOTFOUND;
}

void CuDlgMessageStatistic::OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	int nSel = pNMListView->iItem;
	ShowMessageDescriptionFrame(nSel);
	*pResult = 0;
}

void CuDlgMessageStatistic::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	if (pNMListView->uNewState && pNMListView->iItem >= 0)
	{
		CfMainFrame* pFmain = (CfMainFrame*)AfxGetMainWnd();
		if (pFmain)
		{
			CaMessageStatisticItemData* pEvent = (CaMessageStatisticItemData*)m_cListStatItem.GetItemData(pNMListView->iItem);
			if (!pEvent)
				return;
			MSGCLASSANDID msg;
			msg.lMsgClass = pEvent->GetCatType();
			msg.lMsgFullID= pEvent->GetCode();
			pFmain->ShowMessageDescriptionFrame(FALSE, &msg);
		}
		SetFocus();
	}

	*pResult = 0;
}

void CuDlgMessageStatistic::ShowMessageDescriptionFrame(int nSel)
{
	MSGCLASSANDID msg;
	msg.lMsgClass = -1;
	msg.lMsgFullID= -1;
	CfMainFrame* pFmain = (CfMainFrame*)AfxGetMainWnd();

	if (nSel >= 0 && (m_cListStatItem.GetItemState (nSel, LVIS_SELECTED)&LVIS_SELECTED))
	{
		CaMessageStatisticItemData* pEvent = (CaMessageStatisticItemData*)m_cListStatItem.GetItemData(nSel);
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
