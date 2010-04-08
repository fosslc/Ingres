/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipsess.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The session page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
** 07-Jan-2000 (uk$so01)
**    Bug #99938. Add the image to differentiate the blocked/non-blocked session.
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipsess.h"
#include "ipmprdta.h"
#include "ipmfast.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 7

static void GetDisplayInfo (LPSESSIONDATAMIN pData, 
	CString& strID, 
	CString& strName, 
	CString& strTTY, 
	CString& strDatabase, 
	CString& strState, 
	CString& strFacil, 
	CString& strQuery)
{
	TCHAR tchszBuffer[32];

	strID = FormatHexa64(pData->session_id, tchszBuffer);
	strName = pData->real_user;
	strTTY = pData->session_terminal;
	strDatabase = pData->db_name; 
	strState = pData->session_wait_reason;
	strFacil = pData->server_facility; 
	strQuery = pData->session_query;
}


CuDlgIpmPageSessions::CuDlgIpmPageSessions(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageSessions::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageSessions)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageSessions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageSessions)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageSessions::AddSession (
	LPCTSTR strID, 
	LPCTSTR strName, 
	LPCTSTR strTTY,
	LPCTSTR strDB, 
	LPCTSTR strState, 
	LPCTSTR strFacil, 
	LPCTSTR strQuery,
	void* pStruct)
{
	int nCount, index;
	int nImage = 0; // Not Block;
	LPSESSIONDATAMIN pSession =(LPSESSIONDATAMIN)pStruct;
	ASSERT (pSession);
	if (pSession)
	{
		nImage = (pSession->locklist_wait_id != 0)? 1: 0;
	}

	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the ID
	index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)strID, nImage);
	//
	// Add the Name
	m_cListCtrl.SetItemText (index, 1, (LPTSTR)strName);
	//
	// Add the TTY
	m_cListCtrl.SetItemText (index, 2, (LPTSTR)strTTY);
	//
	// Add the Database
	m_cListCtrl.SetItemText (index, 3, (LPTSTR)strDB);
	//
	// Add the State
	m_cListCtrl.SetItemText (index, 4, (LPTSTR)strState);
	//
	// Add the Facil
	m_cListCtrl.SetItemText (index, 5, (LPTSTR)strFacil);
	//
	// Add the Query
	m_cListCtrl.SetItemText (index, 6, (LPTSTR)strQuery);

	// Store the fast access structure
	if (pStruct) 
		m_cListCtrl.SetItemData (index, (DWORD)pStruct);
	else
		m_cListCtrl.SetItemData (index, NULL);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageSessions, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageSessions)
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
// CuDlgIpmPageSessions message handlers

void CuDlgIpmPageSessions::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageSessions::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	//
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_SESS_ID,   160,  LVCFMT_LEFT, FALSE},
		{IDS_TC_NAME,       70,  LVCFMT_LEFT, FALSE},
		{IDS_TC_TTY,        50,  LVCFMT_LEFT, FALSE},
		{IDS_TC_DATABASE,   60,  LVCFMT_LEFT, FALSE},
		{IDS_TC_STATE,      60,  LVCFMT_LEFT, FALSE},
		{IDS_TC_FACILITY,   60,  LVCFMT_LEFT, FALSE},
		{IDS_TC_QUERY,     300,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	m_ImageList.Create(IDB_TM_SESSION, 16, 1, RGB(255, 0, 255));
	CImageList im;
	if (im.Create(IDB_TM_SESSION_BLOCKED, 16, 1, RGB(255, 0, 255)))
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

void CuDlgIpmPageSessions::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}


LONG CuDlgIpmPageSessions::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	SESSIONDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CString strID, strName, strTTY, strDatabase, strState, strFacil, strQuery, strFastAccess;

	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_INTERNAL_SESSIONS:
		break;
	default:
		return 0L;
	}

	ASSERT (pUps);
	try
	{
		SESSIONDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_SESSION, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (SESSIONDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pData, strID, strName, strTTY, strDatabase, strState, strFacil, strQuery);
				AddSession (strID, strName, strTTY, strDatabase, strState, strFacil, strQuery, pData);
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
		delete (SESSIONDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmPageSessions::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageSessions")) == 0);
	CTypedPtrList<CObList, CuDataTupleSession*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleSession*>*)lParam;
	CuDataTupleSession* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleSession*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddSession (
				pObj->m_strID, 
				pObj->m_strName, 
				pObj->m_strTTY, 
				pObj->m_strDatabase, 
				pObj->m_strState, 
				pObj->m_strFacil,
				pObj->m_strQuery,
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

LONG CuDlgIpmPageSessions::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageSessions* pData = NULL;
	CString strID ;
	CString strName;
	CString strTTY;
	CString strDatabase;
	CString strState;
	CString strFacil;
	CString strQuery;
	try
	{
		CuDataTupleSession* pObj;
		pData = new CuIpmPropertyDataPageSessions ();
		CTypedPtrList<CObList, CuDataTupleSession*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strID = m_cListCtrl.GetItemText (i, 0);
			strName = m_cListCtrl.GetItemText (i, 1); 
			strTTY = m_cListCtrl.GetItemText (i, 2);
			strDatabase = m_cListCtrl.GetItemText (i, 3);
			strState = m_cListCtrl.GetItemText (i, 4);
			strFacil = m_cListCtrl.GetItemText (i, 5);
			strQuery = m_cListCtrl.GetItemText (i, 6);
			pObj = new CuDataTupleSession (strID, strName, strTTY, strDatabase, strState, strFacil, strQuery);
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

void CuDlgIpmPageSessions::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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
			ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, OT_MON_SESSION)); // static -> No string needed
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_MON_SESSION, pFastStruct));
		
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmPageSessions::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	void* pStruct = (void*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (pStruct)
		delete pStruct;
	
	*pResult = 0;
}

void CuDlgIpmPageSessions::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageSessions::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageSessions::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
