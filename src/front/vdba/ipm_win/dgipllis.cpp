/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipllis.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Lock Lists page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
** 07-Jan-2000 (uk$so01)
**    Bug #99938. Add the image to differentiate the blocked/non-blocked locklist
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipllis.h"
#include "ipmprdta.h"
#include "ipmfast.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 6

static void GetDisplayInfo (
	LPLOCKLISTDATAMIN pData,
	CString& strID,
	CString& strSession,
	CString& strCount,
	CString& strLogical,
	CString& strMaxL,
	CString& strStatus)
{
	TCHAR tchszBuffer[32];

	strID = FormatHexaL (pData->locklist_id, tchszBuffer); 
	strSession = FormatHexa64 (pData->locklist_session_id, tchszBuffer);
	strCount.Format(_T("%d"), pData->locklist_lock_count);
	strLogical.Format(_T("%d"), pData->locklist_logical_count);
	strMaxL.Format(_T("%d"), pData->locklist_max_locks);
	strStatus = pData->locklist_status;
}


CuDlgIpmPageLockLists::CuDlgIpmPageLockLists(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageLockLists::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageLockLists)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageLockLists::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageLockLists)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageLockLists::AddLockList (
	LPCTSTR strID, 
	LPCTSTR strSession, 
	LPCTSTR strCount, 
	LPCTSTR strLogical, 
	LPCTSTR strMaxL,
	LPCTSTR strStatus,
	void* pStruct)
{
	int nCount, index;
	int nImage = 0; // Not Block;
	LOCKLISTDATAMIN* pLockList =(LOCKLISTDATAMIN*)pStruct;
	ASSERT (pLockList);
	if (pLockList)
	{
		nImage = (pLockList->locklist_wait_id != 0)? 1: 0;
	}

	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the ID
	index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)strID, nImage);
	//
	// Add the Name
	m_cListCtrl.SetItemText (index, 1, (LPTSTR)strSession);
	//
	// Add the TTY
	m_cListCtrl.SetItemText (index, 2, (LPTSTR)strCount);
	//
	// Add the Database
	m_cListCtrl.SetItemText (index, 3, (LPTSTR)strLogical);
	//
	// Add the State
	m_cListCtrl.SetItemText (index, 4, (LPTSTR)strMaxL);
	//
	// Add the Facil
	m_cListCtrl.SetItemText (index, 5, (LPTSTR)strStatus);
	if (index != -1)
		m_cListCtrl.SetItemData(index, (DWORD)pStruct);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageLockLists, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageLockLists)
	ON_WM_SIZE()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkMfcList1)
	ON_NOTIFY(LVN_DELETEITEM, IDC_LIST1, OnDeleteitemMfcList1)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,   OnPropertiesChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageLockLists message handlers

void CuDlgIpmPageLockLists::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageLockLists::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_LOCKLIST_ID, 91,  LVCFMT_LEFT, FALSE},
		{IDS_TC_SESSION,     60,  LVCFMT_LEFT, FALSE},
		{IDS_TC_LOCKS,       45,  LVCFMT_LEFT, FALSE},
		{IDS_TC_LOGICAL,     50,  LVCFMT_LEFT, FALSE},
		{IDS_TC_MAXL,        40,  LVCFMT_LEFT, FALSE},
		{IDS_TC_STATUS,     200,  LVCFMT_LEFT, FALSE}
	};
	
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	m_ImageList.Create(IDB_TM_LOCKLIST, 16, 1, RGB(255, 0, 255));
	CImageList im;
	if (im.Create(IDB_TM_LOCKLIST_BLOCKED_YES, 16, 1, RGB(255, 0, 255)))
	{
		HICON hIconBlock = im.ExtractIcon(0);
		if (hIconBlock)
		{
			m_ImageList.Add (hIconBlock);
			DestroyIcon (hIconBlock);
		}
	}
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageLockLists::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmPageLockLists::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	LOCKLISTDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CString strID;
	CString strSession;
	CString strCount;
	CString strLogical;
	CString strMaxL;
	CString strStatus;

	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_SYSTEM_LOCK_LISTS:
		break;
	default:
		return 0L;
	}
	try
	{
		LOCKLISTDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_LOCKLIST, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (LOCKLISTDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pData, strID, strSession, strCount, strLogical, strMaxL, strStatus);
				AddLockList (strID, strSession, strCount, strLogical, strMaxL, strStatus, pData);
			}
		}
		return 0L;
	}
	catch (CMemoryException* e)
	{
		MessageBeep (-1);
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	while (!listInfoStruct.IsEmpty())
		delete (LOCKLISTDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmPageLockLists::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageLockLists")) == 0);
	CTypedPtrList<CObList, CuDataTupleLockList*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleLockList*>*)lParam;
	CuDataTupleLockList* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleLockList*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddLockList (
				pObj->m_strID, 
				pObj->m_strSession, 
				pObj->m_strCount, 
				pObj->m_strLogical, 
				pObj->m_strMaxL, 
				pObj->m_strStatus,
				NULL);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageLockLists::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageLockLists* pData = NULL;
	CString strID ;
	CString strSession;
	CString strCount;
	CString strLogical;
	CString strMaxL;
	CString strStatus;
	try
	{
		CuDataTupleLockList* pObj;
		pData = new CuIpmPropertyDataPageLockLists ();
		CTypedPtrList<CObList, CuDataTupleLockList*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strID = m_cListCtrl.GetItemText (i, 0);
			strSession = m_cListCtrl.GetItemText (i, 1); 
			strCount = m_cListCtrl.GetItemText (i, 2);
			strLogical = m_cListCtrl.GetItemText (i, 3);
			strMaxL = m_cListCtrl.GetItemText (i, 4);
			strStatus = m_cListCtrl.GetItemText (i, 5);
			pObj = new CuDataTupleLockList (strID, strSession, strCount, strLogical, strMaxL, strStatus);
			listTuple.AddTail (pObj);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmPageLockLists::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	ASSERT (pNMHDR->code == NM_DBLCLK);
	
	// Get the selected item
	ASSERT (m_cListCtrl.GetSelectedCount() == 1);
	int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);
	ASSERT (selItemId != -1);
	
	if (selItemId != -1) {
		void* pFastStruct = (void*)m_cListCtrl.GetItemData(selItemId);
		if (!pFastStruct) {
			AfxMessageBox (IDS_E_DIRECT_ACCESS);//"Cannot have direct access after load"
			return;
		}
		
		CTypedPtrList<CObList, CuIpmTreeFastItem*>	ipmTreeFastItemList;
		
		// tree organization reflected in FastItemList
		if (IpmCurSelHasStaticChildren(this))
			ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, OT_MON_LOCKLIST));	 // static -> No string needed
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_MON_LOCKLIST, pFastStruct));
		
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmPageLockLists::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	void* pStruct = (void*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (pStruct)
		delete pStruct;
	*pResult = 0;
}

void CuDlgIpmPageLockLists::AddItem (LPCTSTR lpszItem, LOCKLISTDATAMIN* pData)
{
	int nPos = m_cListCtrl.InsertItem (m_cListCtrl.GetItemCount(), lpszItem, 0);
	if (nPos != -1)
		m_cListCtrl.SetItemData (nPos, (DWORD)pData);
}

void CuDlgIpmPageLockLists::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageLockLists::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageLockLists::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
