/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rsserver.cpp, Implementation file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a static item Replication.  (Server)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "rsserver.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgReplicationStaticPageServer::CuDlgReplicationStaticPageServer(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationStaticPageServer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationStaticPageServer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgReplicationStaticPageServer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationStaticPageServer)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationStaticPageServer, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationStaticPageServer)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonPingAll)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedMfcList1)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,   OnPropertiesChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationStaticPageServer message handlers

void CuDlgReplicationStaticPageServer::OnButtonPingAll() 
{
	CString strVnode;
	CStringList listItem;
	int nCount = m_cListCtrl.GetItemCount();
	for (int i=0; i<nCount; i++)
	{
		strVnode = m_cListCtrl.GetItemText(i,2);
		listItem.AddTail(strVnode);
	}
	IPM_PingAll(listItem);
}

void CuDlgReplicationStaticPageServer::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgReplicationStaticPageServer::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	// If modify this constant and the column width, make sure do not forget
	// to port to the OnLoad() and OnGetData() members.
	const int LAYOUT_NUMBER = 4;
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_RUN_NODE, 100,  LVCFMT_LEFT, FALSE},
		{IDS_TC_NUMSIGN,   40,  LVCFMT_RIGHT,FALSE},
		{IDS_TC_LOCAL_DB, 120,  LVCFMT_LEFT, FALSE},
		{IDS_TC_STATUS,   300,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicationStaticPageServer::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CuDlgReplicationStaticPageServer::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect rDlg, r;
	GetClientRect (rDlg);
	m_cListCtrl.GetWindowRect (r);
	ScreenToClient (r);
	r.right  = rDlg.right  - r.left;
	r.bottom = rDlg.bottom - r.left;
	m_cListCtrl.MoveWindow (r);
}

void CuDlgReplicationStaticPageServer::OnItemchangedMfcList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//EnableButtons();	
	*pResult = 0;
}

void CuDlgReplicationStaticPageServer::EnableButtons()
{
	BOOL bRaiseEventEnable = (m_cListCtrl.GetSelectedCount() == 1)? TRUE: FALSE;
	GetDlgItem(IDC_BUTTON1)->EnableWindow (bRaiseEventEnable);
}

LONG CuDlgReplicationStaticPageServer::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	REPLICSERVERDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CString strID, strName, strTTY, strDatabase, strState, strFacil, strQuery;
	
	switch (pUps->nIpmHint)
	{
	case 0:
	//case FILTER_INTERNAL_SESSIONS:
		break;
	default:
		return 0L;
	}
	
	ASSERT (pUps);
	try
	{
		int nCount = 0;
		CString strSvr;
		CString strLocalDB;
		m_cListCtrl.DeleteAllItems();

		CPtrList listInfoStruct;
		REPLICSERVERDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_REPLIC_SERVER, pUps, (LPVOID)&structData, sizeof(structData));

		BOOL bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (REPLICSERVERDATAMIN*)listInfoStruct.RemoveHead();
				m_cListCtrl.InsertItem (nCount, (LPCTSTR)pData->RunNode);
				strSvr.Format (_T("%d"), pData->serverno);
				m_cListCtrl.SetItemText(nCount, 1, strSvr);
				strLocalDB.Format (_T("%s::%s"), (LPTSTR)pData->LocalDBNode, (LPTSTR)pData->LocalDBName);
				m_cListCtrl.SetItemText(nCount, 2, strLocalDB);
				m_cListCtrl.SetItemText(nCount, 3, (LPCTSTR)pData->FullStatus);

				nCount++;
				delete pData;
			}
		}
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
	return 0L;
}

LONG CuDlgReplicationStaticPageServer::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaReplicationStaticDataPageServer")) == 0);
	CTypedPtrList<CObList, CStringList*>* pListTuple;
	CaReplicationStaticDataPageServer* pData = (CaReplicationStaticDataPageServer*)lParam;
	ASSERT (pData);
	if (!pData)
		return 0L;
	pListTuple = &(pData->m_listTuple);
	CStringList* pObj = NULL;
	POSITION p, pos = pListTuple->GetHeadPosition();
	try
	{
		// For each column:
		const int LAYOUT_NUMBER = 4;
		for (int i=0; i<LAYOUT_NUMBER; i++)
			m_cListCtrl.SetColumnWidth(i, pData->m_cxHeader.GetAt(i));

		int nCount;
		while (pos != NULL)
		{
			pObj = pListTuple->GetNext (pos);
			ASSERT (pObj);
			ASSERT (pObj->GetCount() == 4);
			nCount = m_cListCtrl.GetItemCount();
			p = pObj->GetHeadPosition();
			m_cListCtrl.InsertItem  (nCount,    (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 1, (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 2, (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 3, (LPCTSTR)pObj->GetNext(p));
		}
		m_cListCtrl.SetScrollPos (SB_HORZ, pData->m_scrollPos.cx);
		m_cListCtrl.SetScrollPos (SB_VERT, pData->m_scrollPos.cy);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgReplicationStaticPageServer::OnGetData (WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (lvcolumn));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_WIDTH;
	CaReplicationStaticDataPageServer* pData = NULL;
	CString strRunNode;       
	CString strServerNumber;
	CString strLocalDB;
	CString strStatus;
	try
	{
		CStringList* pObj;
		pData = new CaReplicationStaticDataPageServer();
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strRunNode     = m_cListCtrl.GetItemText (i, 0);
			strServerNumber= m_cListCtrl.GetItemText (i, 1); 
			strLocalDB     = m_cListCtrl.GetItemText (i, 2);
			strStatus      = m_cListCtrl.GetItemText (i, 3);
			pObj = new CStringList();
			pObj->AddTail (strRunNode);
			pObj->AddTail (strServerNumber);
			pObj->AddTail (strLocalDB);
			pObj->AddTail (strStatus);
			pData->m_listTuple.AddTail (pObj);
		}
		//
		// For each column
		const int LAYOUT_NUMBER = 4;
		int hWidth   [LAYOUT_NUMBER] = {100, 40, 120, 300};
		for (i=0; i<LAYOUT_NUMBER; i++)
		{
			if (m_cListCtrl.GetColumn(i, &lvcolumn))
				pData->m_cxHeader.InsertAt (i, lvcolumn.cx);
			else
				pData->m_cxHeader.InsertAt (i, hWidth[i]);
		}
		pData->m_scrollPos = CSize (m_cListCtrl.GetScrollPos (SB_HORZ), m_cListCtrl.GetScrollPos (SB_VERT));
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

long CuDlgReplicationStaticPageServer::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
