/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : dginstt1.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Status Page for Installation branch  (Tab 1)
**
** History:
**
** 09-Feb-1999 (uk$so01)
**    Created
** 02-02-2000 (noifr01)
**    (bug 100315) rmcmd instances are now differentiated. take care of the fact that
**    there is one level less of branches in the tree than usually and use the same technique
**    as for the name server
** 01-Mar-2000 (uk$so01)
**    SIR #100635, Add the Functionality of Find operation.
** 24-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
**14-sep-2000 (somsa01)
**    Corrected typo left from cross-integration.
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
** 06-Mar-2003 (uk$so01)
**    SIR #109773, Add property frame for displaying the description 
**    of ingres messages.
**/

#include "stdafx.h"
#include "ivm.h"
#include "dginstt1.h"
#include "ivmdml.h"
#include "ivmdisp.h"
#include "wmusrmsg.h"
#include "findinfo.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const int GLAYOUT_NUMBER = 5;

CuDlgStatusInstallationTab1::CuDlgStatusInstallationTab1(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgStatusInstallationTab1::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgStatusInstallationTab1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgStatusInstallationTab1::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgStatusInstallationTab1)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgStatusInstallationTab1, CDialog)
	//{{AFX_MSG_MAP(CuDlgStatusInstallationTab1)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList1)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WMUSRMSG_IVM_PAGE_UPDATING, OnUpdateData)
	ON_MESSAGE (WMUSRMSG_FIND, OnFind)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgStatusInstallationTab1 message handlers

void CuDlgStatusInstallationTab1::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgStatusInstallationTab1::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	m_ImageList.Create (IDB_STARTSTOP_HISTORY, 16, 0, RGB(255,0,255));
	m_cListCtrl.SetImageList (&m_ImageList, LVSIL_SMALL);

	//
	// Initalize the Column Header of CListCtrl
	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[GLAYOUT_NUMBER] =
	{
		{IDS_HEADER_EV_DATE,      170,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_EV_COMPONENT,  90,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_EV_NAME,       70,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_EV_INSTANCE,   84,  LVCFMT_LEFT, FALSE},
		{IDS_HEADER_EV_TEXT,     700,  LVCFMT_LEFT, FALSE},
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
	
	// return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
	return TRUE;
}

void CuDlgStatusInstallationTab1::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}


LONG CuDlgStatusInstallationTab1::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	try
	{
		CaTreeComponentItemData* pItem = NULL;
		CaPageInformation* pPageInfo = (CaPageInformation*)lParam;
		if (pPageInfo)
			pItem = pPageInfo->GetIvmItem();
		if (!pItem)
			return 0L;
		m_cListCtrl.DeleteAllItems();

		//
		// Get events from the cache that match the filter and
		// and Add them to the list control:
	
		// Construct the filter:
		CaEventFilter& filter = pPageInfo->GetEventFilter();
		BOOL bNameServer = IsNameServer (pPageInfo);
		BOOL bRmcmdServer = IsRmcmdServer(pPageInfo);

		filter.m_strCategory = pItem->m_strComponentType.IsEmpty()? theApp.m_strAll: pItem->m_strComponentType;
		filter.m_strName = pItem->m_strComponentName.IsEmpty() && !bNameServer && !bRmcmdServer? theApp.m_strAll: pItem->m_strComponentName;
		filter.m_strInstance = pItem->m_strComponentInstance.IsEmpty()? theApp.m_strAll: pItem->m_strComponentInstance;

		CaLoggedEvent* pEvent = NULL;
		CTypedPtrList<CObList, CaLoggedEvent*> listLoggedEvent;
		CaIvmEventStartStop& loggedEvent = theApp.GetEventStartStopData();
		loggedEvent.Get (listLoggedEvent, &filter);

		POSITION pos = listLoggedEvent.GetHeadPosition();
		while (pos != NULL)
		{
			pEvent = listLoggedEvent.GetNext(pos);
			AddEvent (pEvent);
		}
		return 0L;
	}
	catch (...)
	{
		AfxMessageBox (_T("System error(raise exception):\nCuDlgStatusInstallationTab1::OnUpdateData, fail to refresh data"));
	}
	return 0L;
}

void CuDlgStatusInstallationTab1::AddEvent (CaLoggedEvent* pEvent)
{
	int nImage = IVM_IsStartMessage (pEvent->GetCode())? 0: 1;
	int nIndex = -1;
	int nCount = m_cListCtrl.GetItemCount();
	nIndex = m_cListCtrl.InsertItem  (nCount, pEvent->GetDateLocale(), nImage);
	if (nIndex != -1)
	{
		m_cListCtrl.SetItemText (nIndex, 1, pEvent->m_strComponentType);
		m_cListCtrl.SetItemText (nIndex, 2, pEvent->m_strComponentName);
		m_cListCtrl.SetItemText (nIndex, 3, pEvent->m_strInstance);
		m_cListCtrl.SetItemText (nIndex, 4, pEvent->m_strText);
		m_cListCtrl.SetItemData (nIndex, (DWORD_PTR)pEvent);
	}
}

LONG CuDlgStatusInstallationTab1::OnFind (WPARAM wParam, LPARAM lParam)
{
	TRACE0 ("CuDlgStatusInstallationTab1::OnFind \n");
	CaFindInformation* pFindInfo = (CaFindInformation*)lParam;
	ASSERT (pFindInfo);
	if (!pFindInfo)
		return 0L;
	CuListCtrl* pCtrl = &m_cListCtrl;
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
	CaLoggedEvent* pEvent = NULL;
	for (i=pFindInfo->GetListPos (); i<nCount; i++)
	{
		pEvent = (CaLoggedEvent*)pCtrl->GetItemData(i);
		pFindInfo->SetListPos (i);
		if (!pEvent)
			continue;

		int nCol = 0;
		while (nCol < GLAYOUT_NUMBER)
		{
			switch (nCol)
			{
			case 0:
				strItem = pEvent->GetDateLocale();
				break;
			case 1:
				strItem = pEvent->m_strComponentType;
				break;
			case 2:
				strItem = pEvent->m_strComponentName;
				break;
				break;
			case 3:
				strItem = pEvent->m_strInstance;
				break;
				break;
			case 4:
				strItem = pEvent->m_strText;
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

void CuDlgStatusInstallationTab1::OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	if (pNMListView->iItem >= 0 && pNMListView->uNewState > 0)
	{
		CfMainFrame* pFmain = (CfMainFrame*)AfxGetMainWnd();
		if (pFmain)
		{
			CaLoggedEvent* pEvent = (CaLoggedEvent*)m_cListCtrl.GetItemData(pNMListView->iItem);
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

void CuDlgStatusInstallationTab1::OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMLISTVIEW* pNMListView = (NMLISTVIEW*)pNMHDR;
	int nSel = pNMListView->iItem;
	ShowMessageDescriptionFrame(nSel);
	*pResult = 0;
}

void CuDlgStatusInstallationTab1::ShowMessageDescriptionFrame(int nSel)
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


