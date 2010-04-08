/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipstdb.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : Emmanuel Blattes
**    Purpose  : Page of Table control: Display list of databases 
**                when tree selection is on static item "Databases"
**
** History:
**
** xx-Sep-1997 (Emmanuel Blattes)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipstdb.h"
#include "ipmprdta.h"
#include "ipmfast.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 1


static int GetDatabaseImageIndex(LPRESOURCEDATAMIN lpr)
{
	switch (lpr->dbtype) {
		case DBTYPE_REGULAR:
			return 0; // IDB_TM_DB_TYPE_REGULAR;
		case DBTYPE_DISTRIBUTED:
			return 1; // IDB_TM_DB_TYPE_DISTRIBUTED;
		case DBTYPE_COORDINATOR:
			return 2; // IDB_TM_DB_TYPE_COORDINATOR;
		case DBTYPE_ERROR:
			return 3; // IDB_TM_DB_TYPE_ERROR;
		default:
			ASSERT (FALSE);// Unexpected ---> trapped in debug mode
			return 0; // IDB_TM_DB_TYPE_REGULAR;	// for release mode: does not 'catch the eye' of the user
	}
	ASSERT (FALSE);
	return -1;
}

void CuDlgIpmPageStaticDatabases::GetDisplayInfo (CdIpmDoc* pDoc, LPRESOURCEDATAMIN pData, CString& strName, int& imageIndex)
{
	TCHAR tchszDatabase [100];
	IPM_GetInfoName(pDoc, OT_DATABASE, (void*)pData, tchszDatabase, sizeof (tchszDatabase));
	strName = tchszDatabase;
	imageIndex = GetDatabaseImageIndex(pData);
}


CuDlgIpmPageStaticDatabases::CuDlgIpmPageStaticDatabases(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageStaticDatabases::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageStaticDatabases)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageStaticDatabases::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageStaticDatabases)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageStaticDatabases::AddDatabase (LPCTSTR strName, int imageIndex, void* pStruct)
{
	int nCount, index;
	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the Name
	index  = m_cListCtrl.InsertItem (nCount, strName, imageIndex);

	// Store the fast access structure
	if (pStruct)
		m_cListCtrl.SetItemData (index, (DWORD)pStruct);
	else
		m_cListCtrl.SetItemData (index, NULL);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageStaticDatabases, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageStaticDatabases)
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
// CuDlgIpmPageStaticDatabases message handlers

void CuDlgIpmPageStaticDatabases::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageStaticDatabases::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_NAME,    216,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	m_ImageList.Create(IDB_TM_DB_TYPE_REGULAR, 16, 1, RGB(255, 0, 255));
	CBitmap bmImage;
	bmImage.LoadBitmap(IDB_TM_DB_TYPE_DISTRIBUTED);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	bmImage.LoadBitmap(IDB_TM_DB_TYPE_COORDINATOR);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	bmImage.LoadBitmap(IDB_TM_DB_TYPE_ERROR);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageStaticDatabases::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}


LONG CuDlgIpmPageStaticDatabases::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	RESOURCEDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CString strName;
	int imageIndex;

	switch (pUps->nIpmHint)
	{
	case 0:
		break;
	default:
		return 0L;
	}

	ASSERT (pUps);
	try
	{
		RESOURCEDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_DATABASE, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (RESOURCEDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pDoc, pData, strName, imageIndex);
				AddDatabase (strName, imageIndex, pData);
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
		delete (RESOURCEDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgIpmPageStaticDatabases::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageStaticDatabases")) == 0);
	CTypedPtrList<CObList, CuDataTupleDatabase*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleDatabase*>*)lParam;
	CuDataTupleDatabase* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleDatabase*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddDatabase (
				pObj->m_strName,
				pObj->m_imageIndex,
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

LONG CuDlgIpmPageStaticDatabases::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageStaticDatabases* pData = NULL;
	CString strName, strListenAddress, strClass, strDbList;
	try
	{
		CuDataTupleDatabase* pObj;
		pData = new CuIpmPropertyDataPageStaticDatabases ();
		CTypedPtrList<CObList, CuDataTupleDatabase*>& listTuple = pData->m_listTuple;
		LVITEM lvitem;
		memset (&lvitem, 0, sizeof(lvitem));
		lvitem.mask = LVIF_IMAGE;
		int nImage = -1;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			// Pick displayed texts
			strName  = m_cListCtrl.GetItemText (i, 0);

			lvitem.iItem = i;
			if (m_cListCtrl.GetItem(&lvitem))
				nImage = lvitem.iImage;
			else
				nImage = -1;
			pObj = new CuDataTupleDatabase (strName, nImage);
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

void CuDlgIpmPageStaticDatabases::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = 0;

	ASSERT (pNMHDR->code == NM_DBLCLK);
	
	// Get the selected item
	ASSERT (m_cListCtrl.GetSelectedCount() == 1);
	int selItemId = m_cListCtrl.GetNextItem(-1, LVNI_SELECTED);
	ASSERT (selItemId != -1);
	
	if (selItemId != -1) 
	{
		void* pFastStruct = (void*)m_cListCtrl.GetItemData(selItemId);
		ASSERT(pFastStruct);
		if (!pFastStruct) {
			AfxMessageBox (IDS_E_DIRECT_ACCESS);//"Cannot have direct access after load"
			return;
		}
		
		CTypedPtrList<CObList, CuIpmTreeFastItem*> ipmTreeFastItemList;
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_DATABASE, pFastStruct));
		
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmPageStaticDatabases::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	void* pStruct = (void*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (pStruct)
		delete pStruct;
	
	*pResult = 0;
}

void CuDlgIpmPageStaticDatabases::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageStaticDatabases::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageStaticDatabases::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
