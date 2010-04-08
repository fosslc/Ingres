/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipstre.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : Emmanuel Blattes - inspired from Sotheavut's code
**    Purpose  : Page of Table control: Display list of replications 
**               when tree selection is on static item "Replication"
**
** History:
**
** xx-Sep-1998 (Emmanuel Blattes)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipstre.h"
#include "ipmprdta.h"
#include "ipmfast.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 1


static int GetReplicationImageIndex(LPRESOURCEDATAMIN lpr)
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
			ASSERT (FALSE); // Unexpected ---> trapped in debug mode
			return 0; // IDB_TM_DB_TYPE_REGULAR;  // for release mode: does not 'catch the eye' of the user
	}
	ASSERT (FALSE);
	return -1;
}

void CuDlgIpmPageStaticReplications::GetDisplayInfo (CdIpmDoc* pDoc, LPRESOURCEDATAMIN pData, CString& strName, int& imageIndex)
{
	TCHAR tchszReplication [100];
	IPM_GetInfoName (pDoc, OT_DATABASE, (void*)pData, tchszReplication, sizeof (tchszReplication));
	strName  = tchszReplication;
	imageIndex  = GetReplicationImageIndex(pData);
}


CuDlgIpmPageStaticReplications::CuDlgIpmPageStaticReplications(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageStaticReplications::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageStaticReplications)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageStaticReplications::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageStaticReplications)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageStaticReplications::AddReplication (
	LPCTSTR strName, 
	int imageIndex,
	void* pStruct)
{
	int nCount, index;
	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the Name
	index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)strName, imageIndex);


	// Store the fast access structure
	if (pStruct)
		m_cListCtrl.SetItemData (index, (DWORD)pStruct);
	else
		m_cListCtrl.SetItemData (index, NULL);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageStaticReplications, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageStaticReplications)
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
// CuDlgIpmPageStaticReplications message handlers

void CuDlgIpmPageStaticReplications::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageStaticReplications::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_NAME,    216,  LVCFMT_LEFT, FALSE},
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

void CuDlgIpmPageStaticReplications::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}


LONG CuDlgIpmPageStaticReplications::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
				AddReplication (strName, imageIndex, pData);
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

LONG CuDlgIpmPageStaticReplications::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageStaticReplications")) == 0);
	CTypedPtrList<CObList, CuDataTupleReplication*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleReplication*>*)lParam;
	CuDataTupleReplication* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleReplication*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddReplication (
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

LONG CuDlgIpmPageStaticReplications::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageStaticReplications* pData = NULL;
	CString strName, strListenAddress, strClass, strDbList;
	try
	{
		CuDataTupleReplication* pObj;
		pData = new CuIpmPropertyDataPageStaticReplications ();
		CTypedPtrList<CObList, CuDataTupleReplication*>& listTuple = pData->m_listTuple;
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
			pObj = new CuDataTupleReplication (strName, nImage);
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

void CuDlgIpmPageStaticReplications::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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

void CuDlgIpmPageStaticReplications::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	void* pStruct = (void*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (pStruct)
		delete pStruct;
	
	*pResult = 0;
}

void CuDlgIpmPageStaticReplications::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageStaticReplications::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageStaticReplications::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
