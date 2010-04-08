/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rvassign.cpp, Implementation file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a Replication Server Item  (Assignment)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
**
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "rvassign.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgReplicationServerPageAssignment::CuDlgReplicationServerPageAssignment(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationServerPageAssignment::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationServerPageAssignment)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgReplicationServerPageAssignment::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationServerPageAssignment)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgReplicationServerPageAssignment, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationServerPageAssignment)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,   OnPropertiesChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationServerPageAssignment message handlers

void CuDlgReplicationServerPageAssignment::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgReplicationServerPageAssignment::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	// If modify this constant and the column width, make sure do not forget
	// to port to the OnLoad() and OnGetData() members.
	const int LAYOUT_NUMBER = 5;
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_DATABASE_NUM, 75,  LVCFMT_RIGHT,FALSE},
		{IDS_TC_DB_NAME,     110,  LVCFMT_LEFT, FALSE},
		{IDS_TC_CDDS,         48,  LVCFMT_RIGHT,FALSE},
		{IDS_TC_CDDS_NAME,    82,  LVCFMT_LEFT, FALSE},
		{IDS_TC_TARGET_TYPE, 120,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgReplicationServerPageAssignment::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect rDlg;
	GetClientRect (rDlg);
	m_cListCtrl.MoveWindow (rDlg);
}

void CuDlgReplicationServerPageAssignment::AddItem (REPLICCDDSASSIGNDATAMIN* pdata)
{
	int nCount = m_cListCtrl.GetItemCount();
	CString strItem;

	//
	// Database Number:
	strItem.Format (_T("%d"), pdata->Database_No);
	m_cListCtrl.InsertItem (nCount, strItem);   
	//
	// Database Name:
	strItem.Format (_T("%s::%s"), (LPTSTR)pdata->szVnodeName, (LPTSTR)pdata->szDBName);
	m_cListCtrl.SetItemText(nCount, 1, strItem);
	//
	// CDDS:
	strItem.Format (_T("%d"), pdata->Cdds_No);
	m_cListCtrl.SetItemText(nCount, 2, strItem);
	//
	// CDDS Name:
	m_cListCtrl.SetItemText(nCount, 3, (LPCTSTR)(LPTSTR)pdata->szCddsName);
	//
	// Target Type:
	m_cListCtrl.SetItemText(nCount, 4, (LPCTSTR)(LPTSTR)pdata->szTargetType);
}


LONG CuDlgReplicationServerPageAssignment::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	CPtrList listInfoStruct;
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
		REPLICSERVERDATAMIN RepSvrdta  = *((LPREPLICSERVERDATAMIN)pUps->pStruct);
		//
		// Empty the control:
		m_cListCtrl.DeleteAllItems();
		CdIpmDoc* pIpmDoc = (CdIpmDoc*)wParam;

		CaIpmQueryInfo queryInfo(pIpmDoc, OT_MON_REPLIC_CDDS_ASSIGN,pUps);
		queryInfo.SetNode((LPCTSTR)RepSvrdta.LocalDBNode);
		queryInfo.SetDatabase ((LPCTSTR)RepSvrdta.LocalDBName);

		BOOL bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			POSITION pos = listInfoStruct.GetHeadPosition();
			while (pos != NULL)
			{
				REPLICCDDSASSIGNDATAMIN* pCdds = (REPLICCDDSASSIGNDATAMIN*)listInfoStruct.GetNext(pos);
				AddItem (pCdds);
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

	while (!listInfoStruct.IsEmpty())
		delete (REPLICCDDSASSIGNDATAMIN*)listInfoStruct.RemoveHead();
	return 0L;
}

LONG CuDlgReplicationServerPageAssignment::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaReplicationServerDataPageAssignment")) == 0);
	CTypedPtrList<CObList, CStringList*>* pListTuple;
	CaReplicationServerDataPageAssignment* pData = (CaReplicationServerDataPageAssignment*)lParam;
	ASSERT (pData);
	if (!pData)
		return 0L;
	pListTuple = &(pData->m_listTuple);
	CStringList* pObj = NULL;
	POSITION p, pos = pListTuple->GetHeadPosition();
	try
	{
		// For each column:
		const int LAYOUT_NUMBER = 5;
		for (int i=0; i<LAYOUT_NUMBER; i++)
			m_cListCtrl.SetColumnWidth(i, pData->m_cxHeader.GetAt(i));

		int nCount;
		while (pos != NULL)
		{
			pObj = pListTuple->GetNext (pos);
			ASSERT (pObj);
			ASSERT (pObj->GetCount() == 5);
			nCount = m_cListCtrl.GetItemCount();
			p = pObj->GetHeadPosition();
			m_cListCtrl.InsertItem  (nCount,    (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 1, (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 2, (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 3, (LPCTSTR)pObj->GetNext(p));
			m_cListCtrl.SetItemText (nCount, 4, (LPCTSTR)pObj->GetNext(p));
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

LONG CuDlgReplicationServerPageAssignment::OnGetData (WPARAM wParam, LPARAM lParam)
{
	LV_COLUMN lvcolumn;
	memset (&lvcolumn, 0, sizeof (lvcolumn));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_WIDTH;

	CaReplicationServerDataPageAssignment* pData = NULL;
	CString strDBNumber;
	CString strDBName;
	CString strCDDS;
	CString strCDDSName;
	CString strTargetType;
	try
	{
		CStringList* pObj;
		pData = new CaReplicationServerDataPageAssignment();
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strDBNumber   = m_cListCtrl.GetItemText (i, 0);
			strDBName     = m_cListCtrl.GetItemText (i, 1); 
			strCDDS       = m_cListCtrl.GetItemText (i, 2);
			strCDDSName   = m_cListCtrl.GetItemText (i, 3);
			strTargetType = m_cListCtrl.GetItemText (i, 4);
			pObj = new CStringList();
			pObj->AddTail (strDBNumber);
			pObj->AddTail (strDBName);
			pObj->AddTail (strCDDS);
			pObj->AddTail (strCDDSName);
			pObj->AddTail (strTargetType);
			pData->m_listTuple.AddTail (pObj);
		}
		//
		// For each column
		const int LAYOUT_NUMBER = 3;
		int hWidth   [LAYOUT_NUMBER] = {100, 80, 60};
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

long CuDlgReplicationServerPageAssignment::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}
