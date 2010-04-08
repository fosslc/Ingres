/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipstsv.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : Emmanuel Blattes - inspired from Sotheavut's code
**    Purpose  : Page of Table control: Display list of servers
**               when tree selection is on static item "Servers"
**
** History:
**
** xx-Sep-1998 (Emmanuel Blattes)
**    Created
** 03-Feb-2000 (noifr01)
**    (SIR 100331) manage the (new) RMCMD monitor server type
** 02-Aug-2001 (noifr01)
**    (sir 105275) manage the JDBC server type and misc cleanup
** 15-Mar-2004 (schph01)
**    (SIR #110013) Handle the DAS Server
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipstsv.h"
#include "ipmprdta.h"
#include "ipmfast.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER   4


static int GetServerImageIndex(LPSERVERDATAMIN lps)
{
	switch (lps->servertype) {
	case SVR_TYPE_DBMS:
		return 0; // IDB_TM_SERVER_TYPE_INGRES;
	case SVR_TYPE_GCN:
		return 1; // IDB_TM_SERVER_TYPE_NAME;
	case SVR_TYPE_NET:
		return 2; // IDB_TM_SERVER_TYPE_NET;
	case SVR_TYPE_STAR:
		return 3; // IDB_TM_SERVER_TYPE_STAR;
	case SVR_TYPE_RECOVERY:
		return 4; // IDB_TM_SERVER_TYPE_RECOVERY;
	case SVR_TYPE_ICE:
		return 5;
	case SVR_TYPE_RMCMD: 
		return 6; // IDB_TM_SERVER_TYPE_RMCMD;
	case SVR_TYPE_JDBC: 
	case SVR_TYPE_DASVR:
		return 7; // IDB_TM_SERVER_TYPE_JDBC;
	case SVR_TYPE_OTHER:
	default:
		return 8; // IDB_TM_SERVER_TYPE_OTHER;
	}
	ASSERT (FALSE);
	return -1;
}

void CuDlgIpmPageStaticServers::GetDisplayInfo (CdIpmDoc* pDoc, LPSERVERDATAMIN pData, CString& strName, CString& strListenAddress, CString& strClass, CString& strDbList, int& imageIndex)
{
	TCHAR tchszServer [100];
	IPM_GetInfoName (pDoc, OT_MON_SERVER, (void*)pData, tchszServer, sizeof (tchszServer));
	strName     = tchszServer;

	strListenAddress = pData->listen_address;
	strClass         = pData->server_class;
	strDbList        = pData->server_dblist;
	imageIndex       = GetServerImageIndex(pData);
}


CuDlgIpmPageStaticServers::CuDlgIpmPageStaticServers(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageStaticServers::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageStaticServers)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageStaticServers::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageStaticServers)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageStaticServers::AddServer (
	LPCTSTR strName, 
	LPCTSTR strListenAddress,
	LPCTSTR strClass,
	LPCTSTR strDbList,
	int imageIndex,
	void* pStruct)
{
	int nCount, index;
	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the Name
	index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)strName, imageIndex);

	// Add other columns
	m_cListCtrl.SetItemText (index, 1, (LPTSTR)strListenAddress);
	m_cListCtrl.SetItemText (index, 2, (LPTSTR)strClass);
	m_cListCtrl.SetItemText (index, 3, (LPTSTR)strDbList);

	// Store the fast access structure
	if (pStruct)
		m_cListCtrl.SetItemData (index, (DWORD)pStruct);
	else
		m_cListCtrl.SetItemData (index, NULL);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageStaticServers, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageStaticServers)
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
// CuDlgIpmPageStaticServers message handlers

void CuDlgIpmPageStaticServers::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageStaticServers::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_SVR_TYPE,      96,  LVCFMT_LEFT, FALSE},
		{IDS_TC_LISTEN_ADD,   100,  LVCFMT_LEFT, FALSE},
		{IDS_TC_CLASS,         70,  LVCFMT_LEFT, FALSE},
		{IDS_TC_CONNECT_TO_DB,130,  LVCFMT_LEFT, FALSE},
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	m_ImageList.Create(IDB_TM_SERVER_TYPE_INGRES, 16, 1, RGB(255, 0, 255));
	CBitmap bmImage;
	bmImage.LoadBitmap(IDB_TM_SERVER_TYPE_NAME);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	bmImage.LoadBitmap(IDB_TM_SERVER_TYPE_NET);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	bmImage.LoadBitmap(IDB_TM_SERVER_TYPE_STAR);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	bmImage.LoadBitmap(IDB_TM_SERVER_TYPE_RECOVERY);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	bmImage.LoadBitmap(IDB_TM_SERVER_TYPE_ICE);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	bmImage.LoadBitmap(IDB_TM_SERVER_TYPE_RMCMD);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	bmImage.LoadBitmap(IDB_TM_SERVER_TYPE_JDBC);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	bmImage.LoadBitmap(IDB_TM_SERVER_TYPE_OTHER);
	m_ImageList.Add(&bmImage, RGB(255, 0, 255));
	bmImage.Detach();
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageStaticServers::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}


LONG CuDlgIpmPageStaticServers::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	SERVERDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CString strName, strListenAddress, strClass, strDbList;
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
		SERVERDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_SERVER, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (SERVERDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pDoc, pData, strName, strListenAddress, strClass, strDbList, imageIndex);
				AddServer (strName, strListenAddress, strClass, strDbList, imageIndex, pData);
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
		delete (SERVERDATAMIN*)listInfoStruct.RemoveHead();
	return 0;
}

LONG CuDlgIpmPageStaticServers::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageServers")) == 0);
	CTypedPtrList<CObList, CuDataTupleServer*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleServer*>*)lParam;
	CuDataTupleServer* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleServer*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddServer (
				pObj->m_strName,
				pObj->m_strListenAddress,
				pObj->m_strClass,
				pObj->m_strDbList,
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

LONG CuDlgIpmPageStaticServers::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageServers* pData = NULL;
	CString strName, strListenAddress, strClass, strDbList;
	try
	{
		CuDataTupleServer* pObj;
		pData = new CuIpmPropertyDataPageServers ();
		CTypedPtrList<CObList, CuDataTupleServer*>& listTuple = pData->m_listTuple;
		LVITEM lvitem;
		memset (&lvitem, 0, sizeof(lvitem));
		lvitem.mask = LVIF_IMAGE;
		int nImage = -1;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			// Pick displayed texts
			strName  = m_cListCtrl.GetItemText (i, 0);
			strListenAddress  = m_cListCtrl.GetItemText (i, 1);
			strClass  = m_cListCtrl.GetItemText (i, 2);
			strDbList  = m_cListCtrl.GetItemText (i, 3);
			lvitem.iItem = i;
			if (m_cListCtrl.GetItem(&lvitem))
				nImage = lvitem.iImage;
			else
				nImage = -1;

			pObj = new CuDataTupleServer (strName, strListenAddress, strClass, strDbList, nImage);
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

void CuDlgIpmPageStaticServers::OnDblclkMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
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
		ipmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, OT_MON_SERVER, pFastStruct));
		
		if (!ExpandUpToSearchedItem(this, ipmTreeFastItemList)) {
			AfxMessageBox (IDS_IPM_FAST_CANNOTEXPAND);
		}
		while (!ipmTreeFastItemList.IsEmpty())
			delete ipmTreeFastItemList.RemoveHead();
	}
}

void CuDlgIpmPageStaticServers::OnDeleteitemMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	void* pStruct = (void*)m_cListCtrl.GetItemData(pNMListView->iItem);
	if (pStruct)
		delete pStruct;
	
	*pResult = 0;
}

void CuDlgIpmPageStaticServers::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageStaticServers::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageStaticServers::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
