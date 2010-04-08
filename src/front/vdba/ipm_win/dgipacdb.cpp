/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipacdb.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Active Database page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
** 17-Jan-2000 (uk$so01)
**    99938, Add the image to differentiate the type of Actived Database.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgipacdb.h"
#include "ipmprdta.h"
#include "ipmfast.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 7

static void GetDisplayInfo (
	LPLOGDBDATAMIN pData,
	CString& strDB,
	CString& strStatus, 
	CString& strTXCnt,
	CString& strBegin,
	CString& strEnd,
	CString& strRead,
	CString& strWrite)
{
	strDB  = pData->db_name;
	strStatus = pData->db_status; 
	strTXCnt.Format (_T("%ld"), pData->db_tx_count);
	strBegin.Format (_T("%ld"), pData->db_tx_begins);
	strEnd.Format (_T("%ld"), pData->db_tx_ends);
	strRead.Format(_T("%ld"), pData->db_reads);
	strWrite.Format (_T("%ld"), pData->db_writes);
}

CuDlgIpmPageActiveDatabases::CuDlgIpmPageActiveDatabases(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageActiveDatabases::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageActiveDatabases)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageActiveDatabases::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageActiveDatabases)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

static UINT GetDbTypeImageId(LPLOGDBDATAMIN lpldb)
{
	ASSERT (lpldb);
	if (!lpldb)
		return -1;
	switch (lpldb->dbtype) 
	{
	case DBTYPE_REGULAR:
		return 0; //IDB_TM_DB_TYPE_REGULAR;
	case DBTYPE_DISTRIBUTED:
		return 1; //IDB_TM_DB_TYPE_DISTRIBUTED;
	case DBTYPE_COORDINATOR:
		return 2; //IDB_TM_DB_TYPE_COORDINATOR;
	case DBTYPE_ERROR:
		return 3; //IDB_TM_DB_TYPE_ERROR;
	default:
		ASSERT (FALSE); // Unexpected ---> trapped in debug mode
		return 0; //IDB_TM_DB_TYPE_REGULAR;
	}
}


void CuDlgIpmPageActiveDatabases::AddActiveDB (
	LPCTSTR strDB,
	LPCTSTR strStatus, 
	LPCTSTR strTXCnt,
	LPCTSTR strBegin,
	LPCTSTR strEnd,
	LPCTSTR strRead,
	LPCTSTR strWrite,
	void* pStruct)
{
	int nCount, index;
	int nImage = GetDbTypeImageId((LPLOGDBDATAMIN)pStruct);
	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the DB Name
	index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)strDB, nImage);
	//
	// Add the Status
	m_cListCtrl.SetItemText (index, 1, (LPTSTR)strStatus);
	//
	// Add the TX Count
	m_cListCtrl.SetItemText (index, 2, (LPTSTR)strTXCnt);
	//
	// Add the Begin
	m_cListCtrl.SetItemText (index, 3, (LPTSTR)strBegin);
	//
	// Add the End
	m_cListCtrl.SetItemText (index, 4, (LPTSTR)strEnd);
	//
	// Add the Read
	m_cListCtrl.SetItemText (index, 5, (LPTSTR)strRead);
	//
	// Add the Write
	m_cListCtrl.SetItemText (index, 6, (LPTSTR)strWrite);

	// Store the fast access structure
	if (pStruct)
	{
		m_cListCtrl.SetItemData (index, (DWORD)pStruct);
	}
	else
		m_cListCtrl.SetItemData (index, NULL);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageActiveDatabases, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageActiveDatabases)
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
// CuDlgIpmPageActiveDatabases message handlers

void CuDlgIpmPageActiveDatabases::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageActiveDatabases::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_DATABASE,  91,  LVCFMT_LEFT, FALSE},
		{IDS_TC_STATUS,   110,  LVCFMT_LEFT, FALSE},
		{IDS_TC_TX_CNT,    50,  LVCFMT_LEFT, FALSE},
		{IDS_TC_BEGIN,     55,  LVCFMT_LEFT, FALSE},
		{IDS_TC_END,       55,  LVCFMT_LEFT, FALSE},
		{IDS_TC_READ,      55,  LVCFMT_LEFT, FALSE},
		{IDS_TC_WRITE,     55,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);
	//
	// Possible enhancement: personnalize according to database type
	m_ImageList.Create(IDB_TM_DB, 16, 1, RGB(255, 0, 255));
	UINT narrayImage[3] = {IDB_TM_DB_TYPE_DISTRIBUTED, IDB_TM_DB_TYPE_COORDINATOR, IDB_TM_DB_TYPE_ERROR};
	for (int i = 0; i<3; i++)
	{
		CImageList im;
		if (im.Create(narrayImage[i], 16, 1, RGB(255, 0, 255)))
		{
			HICON hIconBlock = im.ExtractIcon(0);
			if (hIconBlock)
			{
				m_ImageList.Add (hIconBlock);
				DestroyIcon (hIconBlock);
			}
		}
	}
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageActiveDatabases::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmPageActiveDatabases::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	LOGDBDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CString strDB;
	CString strStatus;
	CString strTXCnt;
	CString strBegin;
	CString strEnd;
	CString strRead;
	CString strWrite;
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
		LOGDBDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_LOGDATABASE, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (LOGDBDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pData, strDB, strStatus, strTXCnt, strBegin, strEnd, strRead, strWrite);
				AddActiveDB (strDB, strStatus, strTXCnt, strBegin, strEnd, strRead, strWrite, pData);
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
		delete (LOGDBDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmPageActiveDatabases::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageActiveDatabases")) == 0);
	CTypedPtrList<CObList, CuDataTupleActiveDatabase*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleActiveDatabase*>*)lParam;
	CuDataTupleActiveDatabase* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleActiveDatabase*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddActiveDB (
				pObj->m_strDatabase,
				pObj->m_strStatus,
				pObj->m_strTXCnt,
				pObj->m_strBegin,
				pObj->m_strEnd,
				pObj->m_strRead,
				pObj->m_strWrite,
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

LONG CuDlgIpmPageActiveDatabases::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageActiveDatabases* pData = NULL;
	CString strDatabase;
	CString strStatus;
	CString strTXCnt;
	CString strBegin;
	CString strEnd;  
	CString strRead; 
	CString strWrite;

	try
	{
		CuDataTupleActiveDatabase* pObj;
		pData = new CuIpmPropertyDataPageActiveDatabases ();
		CTypedPtrList<CObList, CuDataTupleActiveDatabase*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strDatabase = m_cListCtrl.GetItemText (i, 0);
			strStatus	= m_cListCtrl.GetItemText (i, 1); 
			strTXCnt	= m_cListCtrl.GetItemText (i, 2);
			strBegin	= m_cListCtrl.GetItemText (i, 3);
			strEnd		= m_cListCtrl.GetItemText (i, 4);
			strRead 	= m_cListCtrl.GetItemText (i, 5);
			strWrite	= m_cListCtrl.GetItemText (i, 6);
			pObj = new CuDataTupleActiveDatabase (strDatabase, strStatus, strTXCnt, strBegin, strEnd, strRead, strWrite);
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

void CuDlgIpmPageActiveDatabases::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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
		
		CTypedPtrList<CObList, CuIpmTreeFastItem*> ipmTreeFastItemList;
		
		// tree organization reflected in FastItemList
		if (IpmCurSelHasStaticChildren(this))
			ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, OT_MON_LOGDATABASE)); // static -> No string needed
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_MON_LOGDATABASE, pFastStruct));
		
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmPageActiveDatabases::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	void* pStruct = (void*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (pStruct)
		delete pStruct;
	
	*pResult = 0;
}

void CuDlgIpmPageActiveDatabases::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageActiveDatabases::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageActiveDatabases::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}