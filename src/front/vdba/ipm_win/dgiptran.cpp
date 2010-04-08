/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiptran.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control: The Transaction page
**
** History:
**
** xx-Feb-1997 (Emmanuel Blattes)
**    Created
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgiptran.h"
#include "ipmprdta.h"
#include "ipmfast.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 6

static void GetDisplayInfo (
	LPLOGTRANSACTDATAMIN pData, 
	CString& strExternalTXID, 
	CString& strDB, 
	CString& strSession, 
	CString& strStatus, 
	CString& strWrite, 
	CString& strSplit)
{
	TCHAR tchszBuffer [32];

	strExternalTXID = pData->tx_transaction_id;
	strDB = pData->tx_db_name; 
	strSession = FormatHexa64 (pData->tx_session_id, tchszBuffer);
	strStatus = pData->tx_status;
	strWrite.Format (_T("%ld"), pData->tx_state_write);
	strSplit.Format (_T("%ld"), pData->tx_state_split);
}


CuDlgIpmPageTransactions::CuDlgIpmPageTransactions(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageTransactions::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageTransactions)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageTransactions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageTransactions)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageTransactions::AddTransaction (
	LPCTSTR strExternalTXID,
	LPCTSTR strDB, 
	LPCTSTR strSession,
	LPCTSTR strStatus,
	LPCTSTR strWrite,
	LPCTSTR strSplit,
	void* pStruct)
{
	int nCount, index;
	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the ID
	index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)strExternalTXID, 0);
	//
	// Add the Name
	m_cListCtrl.SetItemText (index, 1, (LPTSTR)strDB);
	//
	// Add the TTY
	m_cListCtrl.SetItemText (index, 2, (LPTSTR)strSession);
	//
	// Add the Database
	m_cListCtrl.SetItemText (index, 3, (LPTSTR)strStatus);
	//
	// Add the State
	m_cListCtrl.SetItemText (index, 4, (LPTSTR)strWrite);
	//
	// Add the Facil
	m_cListCtrl.SetItemText (index, 5, (LPTSTR)strSplit);

	// Store the fast access structure
	if (pStruct)
		m_cListCtrl.SetItemData (index, (DWORD)pStruct);
	else
		m_cListCtrl.SetItemData (index, NULL);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageTransactions, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageTransactions)
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
// CuDlgIpmPageTransactions message handlers

void CuDlgIpmPageTransactions::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageTransactions::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_EXT_TRANSACTION, 141,  LVCFMT_LEFT, FALSE},
		{IDS_TC_DATABASE,         65,  LVCFMT_LEFT, FALSE},
		{IDS_TC_SESSION,          65,  LVCFMT_LEFT, FALSE},
		{IDS_TC_STATUS,          120,  LVCFMT_LEFT, FALSE},
		{IDS_TC_WRITE,            40,  LVCFMT_LEFT, FALSE},
		{IDS_TC_SPLIT,            40,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);
	m_ImageList.Create(IDB_TM_TRANSAC, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageTransactions::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmPageTransactions::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	LOGTRANSACTDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CString strExternalTXID, strDB, strSession, strStatus, strWrite, strSplit;

	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_INACTIVE_TRANSACTIONS:
		break;
	default:
		return 0L;
	}
	try
	{
		LOGTRANSACTDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_TRANSACTION, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (LOGTRANSACTDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pData, strExternalTXID, strDB, strSession, strStatus, strWrite, strSplit);
				AddTransaction (strExternalTXID, strDB, strSession, strStatus, strWrite, strSplit, pData);
			}
		}
		return 0L;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	while (!listInfoStruct.IsEmpty())
		delete (LOGTRANSACTDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmPageTransactions::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageTransactions")) == 0);
	CTypedPtrList<CObList, CuDataTupleTransaction*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleTransaction*>*)lParam;
	CuDataTupleTransaction* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleTransaction*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddTransaction (
				pObj->m_strExternalTXID, 
				pObj->m_strDatabase, 
				pObj->m_strSession, 
				pObj->m_strStatus, 
				pObj->m_strWrite, 
				pObj->m_strSplit,
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

LONG CuDlgIpmPageTransactions::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageTransactions* pData = NULL;
	CString strExternalTXID;
	CString strDB;
	CString strSession;
	CString strStatus;
	CString strWrite;
	CString strSplit;
	try
	{
		CuDataTupleTransaction* pObj;
		pData = new CuIpmPropertyDataPageTransactions ();
		CTypedPtrList<CObList, CuDataTupleTransaction*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strExternalTXID = m_cListCtrl.GetItemText (i, 0);
			strDB = m_cListCtrl.GetItemText (i, 1); 
			strSession = m_cListCtrl.GetItemText (i, 2);
			strStatus = m_cListCtrl.GetItemText (i, 3);
			strWrite = m_cListCtrl.GetItemText (i, 4);
			strSplit = m_cListCtrl.GetItemText (i, 5);
			pObj = new CuDataTupleTransaction (strExternalTXID, strDB, strSession, strStatus, strWrite, strSplit);
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

void CuDlgIpmPageTransactions::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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
			ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, OT_MON_TRANSACTION)); // static -> No string needed
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_MON_TRANSACTION, pFastStruct));
		
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmPageTransactions::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	void* pStruct = (void*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (pStruct)
		delete pStruct;
	
	*pResult = 0;
}

void CuDlgIpmPageTransactions::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageTransactions::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageTransactions::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
