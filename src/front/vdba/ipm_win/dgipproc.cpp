/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipproc.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Process page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgipproc.h"
#include "ipmprdta.h"
#include "ipmfast.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 9


static void GetDisplayInfo (
	LPLOGPROCESSDATAMIN pData,
	CString& strID,
	CString& strPID, 
	CString& strType,
	CString& strOpenDB,
	CString& strWrite,
	CString& strForce,
	CString& strWait,
	CString& strBegin,
	CString& strEnd)
{
	TCHAR tchszBuffer[32];

	strID = FormatHexaL (pData->log_process_id, tchszBuffer);
	strPID = FormatHexaL (pData->process_pid,  tchszBuffer); 
	strType = pData->process_status;
	strOpenDB.Format(_T("%ld"), pData->process_count);
	strWrite.Format (_T("%ld"), pData->process_writes); 
	strForce.Format (_T("%ld"), pData->process_log_forces); 
	strWait.Format(_T("%ld"), pData->process_waits); 
	strBegin.Format (_T("%ld"), pData->process_tx_begins); 
	strEnd.Format(_T("%ld"), pData->process_tx_ends); 
}

CuDlgIpmPageProcesses::CuDlgIpmPageProcesses(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageProcesses::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageProcesses)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageProcesses::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageProcesses)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageProcesses::AddProcess (
	LPCTSTR strID,
	LPCTSTR strPID, 
	LPCTSTR strType,
	LPCTSTR strOpenDB,
	LPCTSTR strWrite,
	LPCTSTR strForce,
	LPCTSTR strWait,
	LPCTSTR strBegin,
	LPCTSTR strEnd,
	void* pStruct)
{
	int nCount, index;
	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the ID
	index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)strID, 0);
	//
	// Add the PID
	m_cListCtrl.SetItemText (index, 1, (LPTSTR)strPID);
	//
	// Add the Type
	m_cListCtrl.SetItemText (index, 2, (LPTSTR)strType);
	//
	// Add the OpenDB
	m_cListCtrl.SetItemText (index, 3, (LPTSTR)strOpenDB);
	//
	// Add the Write
	m_cListCtrl.SetItemText (index, 4, (LPTSTR)strWrite);
	//
	// Add the Force
	m_cListCtrl.SetItemText (index, 5, (LPTSTR)strForce);
	//
	// Add the Wait
	m_cListCtrl.SetItemText (index, 6, (LPTSTR)strWait);
	//
	// Add the Begin
	m_cListCtrl.SetItemText (index, 7, (LPTSTR)strBegin);
	//
	// Add the End
	m_cListCtrl.SetItemText (index, 8, (LPTSTR)strEnd);

	// Store the fast access structure
	if (pStruct) 
		m_cListCtrl.SetItemData (index, (DWORD)pStruct);
	else
		m_cListCtrl.SetItemData (index, NULL);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageProcesses, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageProcesses)
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
// CuDlgIpmPageProcesses message handlers

void CuDlgIpmPageProcesses::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageProcesses::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_INTERNAL_ID, 76,  LVCFMT_LEFT, FALSE},
		{IDS_TC_OP_SYS_ID,   60,  LVCFMT_LEFT, FALSE},
		{IDS_TC_TYPE,       110,  LVCFMT_LEFT, FALSE},
		{IDS_TC_OPENDB,      60,  LVCFMT_LEFT, FALSE},
		{IDS_TC_WRITE,       40,  LVCFMT_LEFT, FALSE},
		{IDS_TC_FORCE,       40,  LVCFMT_LEFT, FALSE},
		{IDS_TC_WAIT,        40,  LVCFMT_LEFT, FALSE},
		{IDS_TC_BEGIN,       40,  LVCFMT_LEFT, FALSE},
		{IDS_TC_END,         40,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);
	m_ImageList.Create(IDB_TM_LOGPROCESS, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageProcesses::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmPageProcesses::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	LOGPROCESSDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CString strID;
	CString strPID; 
	CString strType;
	CString strOpenDB;
	CString strWrite;
	CString strForce;
	CString strWait;
	CString strBegin;
	CString strEnd;

	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
		break;
	default:
		return 0L;
	}

	try
	{
		LOGPROCESSDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_LOGPROCESS, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (LOGPROCESSDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pData, strID, strPID, strType, strOpenDB, strWrite, strForce, strWait, strBegin, strEnd);
				AddProcess (strID, strPID, strType, strOpenDB, strWrite, strForce, strWait, strBegin, strEnd, pData);
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
		delete (LOGPROCESSDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmPageProcesses::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageProcesses")) == 0);
	CTypedPtrList<CObList, CuDataTupleProcess*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleProcess*>*)lParam;
	CuDataTupleProcess* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleProcess*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddProcess (
				pObj->m_strID, 
				pObj->m_strPID, 
				pObj->m_strType, 
				pObj->m_strOpenDB, 
				pObj->m_strWrite, 
				pObj->m_strForce,
				pObj->m_strWait,
				pObj->m_strBegin,
				pObj->m_strEnd,
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

LONG CuDlgIpmPageProcesses::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageProcesses* pData = NULL;
	CString strID ;
	CString strPID;
	CString strType;
	CString strOpenDB;
	CString strWrite;
	CString strForce;
	CString strWait;
	CString strBegin;
	CString strEnd;
	try
	{
		CuDataTupleProcess* pObj;
		pData = new CuIpmPropertyDataPageProcesses ();
		CTypedPtrList<CObList, CuDataTupleProcess*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strID = m_cListCtrl.GetItemText (i, 0);
			strPID = m_cListCtrl.GetItemText (i, 1); 
			strType = m_cListCtrl.GetItemText (i, 2);
			strOpenDB = m_cListCtrl.GetItemText (i, 3);
			strWrite = m_cListCtrl.GetItemText (i, 4);
			strForce = m_cListCtrl.GetItemText (i, 5);
			strWait = m_cListCtrl.GetItemText (i, 6);
			strBegin = m_cListCtrl.GetItemText (i, 7);
			strEnd = m_cListCtrl.GetItemText (i, 8);
			pObj = new CuDataTupleProcess (strID, strPID, strType, strOpenDB, strWrite, strForce, strWait, strBegin, strEnd);
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

void CuDlgIpmPageProcesses::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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
			ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, OT_MON_LOGPROCESS));   // static -> No string needed
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_MON_LOGPROCESS, pFastStruct));
		
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmPageProcesses::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	void* pStruct = (void*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (pStruct)
		delete pStruct;
	
	*pResult = 0;
}

void CuDlgIpmPageProcesses::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageProcesses::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageProcesses::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
