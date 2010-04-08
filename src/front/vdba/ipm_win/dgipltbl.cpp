/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipltbl.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Locked Table page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipltbl.h"
#include "ipmprdta.h"
#include "ipmfast.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 4

static void GetDisplayInfo (
	LPRESOURCEDATAMIN pData,
	CString& strID,
	CString& strGR, 
	CString& strCV,
	CString& strTable)
{
	TCHAR tchszBuffer [64];
	strID = FormatHexaL (pData->resource_id, tchszBuffer);
	strGR = pData->resource_grant_mode;
	strCV = pData->resource_convert_mode;
	strTable = IPM_GetResTableNameStr(pData,tchszBuffer,sizeof(tchszBuffer));
}
CuDlgIpmPageLockTables::CuDlgIpmPageLockTables(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageLockTables::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageLockTables)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageLockTables::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageLockTables)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageLockTables::AddLockTable (
	LPCTSTR strID,
	LPCTSTR strGR, 
	LPCTSTR strCV,
	LPCTSTR strTable)
{
	int nCount, index;
	nCount = m_cListCtrl.GetItemCount ();
	//
	// Add the ID
	index  = m_cListCtrl.InsertItem (nCount, (LPTSTR)strID, 0);
	//
	// Add the GR
	m_cListCtrl.SetItemText (index, 1, (LPTSTR)strGR);
	//
	// Add the CV
	m_cListCtrl.SetItemText (index, 2, (LPTSTR)strCV);
	//
	// Add the Table
	m_cListCtrl.SetItemText (index, 3, (LPTSTR)strTable);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageLockTables, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageLockTables)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,   OnPropertiesChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageLockTables message handlers

void CuDlgIpmPageLockTables::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageLockTables::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_ID,    116,  LVCFMT_LEFT, FALSE},
		{IDS_TC_GR,     65,  LVCFMT_LEFT, FALSE},
		{IDS_TC_CV,     50,  LVCFMT_LEFT, FALSE},
		{IDS_TC_TABLE, 100,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	m_ImageList.Create(IDB_TM_LOCKEDTBL, 16, 1, RGB(255, 0, 255));
	m_cListCtrl.SetImageList(&m_ImageList, LVSIL_SMALL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageLockTables::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmPageLockTables::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	RESOURCEDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;

	CString strID;
	CString strGR; 
	CString strCV;
	CString strTable;
	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_NULL_RESOURCES:
		break;
	default:
		return 0L;
	}
	try
	{
		RESOURCEDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_RES_TABLE, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (RESOURCEDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pData, strID, strGR, strCV, strTable);
				AddLockTable (strID, strGR, strCV, strTable);
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

LONG CuDlgIpmPageLockTables::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageLockTabless")) == 0);
	CTypedPtrList<CObList, CuDataTupleLockTable*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleLockTable*>*)lParam;
	CuDataTupleLockTable* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleLockTable*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddLockTable (pObj->m_strID, pObj->m_strGR, pObj->m_strCV, pObj->m_strTable);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageLockTables::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageLockTables* pData = NULL;
	CString strID;
	CString strGR;
	CString strCV;
	CString strTable;
	try
	{
		CuDataTupleLockTable* pObj;
		pData = new CuIpmPropertyDataPageLockTables ();
		CTypedPtrList<CObList, CuDataTupleLockTable*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strID = m_cListCtrl.GetItemText (i, 0);
			strGR = m_cListCtrl.GetItemText (i, 1); 
			strCV = m_cListCtrl.GetItemText (i, 2);
			strTable = m_cListCtrl.GetItemText (i, 3);
			pObj = new CuDataTupleLockTable (strID, strGR, strCV, strTable);
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

void CuDlgIpmPageLockTables::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageLockTables::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageLockTables::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}

