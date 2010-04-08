/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiplres.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Locked Resource page
**
** History:
**
** xx-Feb-1997 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgiplres.h"
#include "ipmprdta.h"
#include "ipmfast.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define LAYOUT_NUMBER 7



static void GetDisplayInfo (
	LPRESOURCEDATAMIN pData,
	CString& strID,
	CString& strGR, 
	CString& strCV,
	CString& strCockType,
	CString& strDB,
	CString& strTable,
	CString& strPage)
{
	TCHAR tchszBuffer [80];
	strID = FormatHexaL (pData->resource_id, tchszBuffer);
	strGR = pData->resource_grant_mode;
	strCV = pData->resource_convert_mode;
	strCockType = FindResourceString(pData->resource_type);
	strDB = pData->res_database_name;
	strTable = IPM_GetResTableNameStr(pData,tchszBuffer,sizeof(tchszBuffer));
	strPage = IPM_GetResPageNameStr(pData,tchszBuffer);
}


CuDlgIpmPageLockResources::CuDlgIpmPageLockResources(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgIpmPageLockResources::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageLockResources)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CuDlgIpmPageLockResources::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageLockResources)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CuDlgIpmPageLockResources::AddLockResource (
	LPCTSTR strID,
	LPCTSTR strGR, 
	LPCTSTR strCV,
	LPCTSTR strLockType,
	LPCTSTR strDB,
	LPCTSTR strTable,
	LPCTSTR strPage)
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
	// Add the Lock Type
	m_cListCtrl.SetItemText (index, 3, (LPTSTR)strLockType);
	//
	// Add the strDB
	m_cListCtrl.SetItemText (index, 4, (LPTSTR)strDB);
	//
	// Add the Table
	m_cListCtrl.SetItemText (index, 5, (LPTSTR)strTable);
	//
	// Add the Page
	m_cListCtrl.SetItemText (index, 6, (LPTSTR)strPage);
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageLockResources, CDialog)
	//{{AFX_MSG_MAP(CuDlgIpmPageLockResources)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WMUSRMSG_CHANGE_SETTING,   OnPropertiesChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageLockResources message handlers

void CuDlgIpmPageLockResources::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

BOOL CuDlgIpmPageLockResources::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY (m_cListCtrl.SubclassDlgItem (IDC_LIST1, this));
	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_TC_RESOURCE_ID,   101,  LVCFMT_LEFT, FALSE},
		{IDS_TC_GR,             30,  LVCFMT_LEFT, FALSE},
		{IDS_TC_CV,             30,  LVCFMT_LEFT, FALSE},
		{IDS_TC_LOCK_TYPE,      80,  LVCFMT_LEFT, FALSE},
		{IDS_TC_DATABASE,       80,  LVCFMT_LEFT, FALSE},
		{IDS_TC_TABLE,          80,  LVCFMT_LEFT, FALSE},
		{IDS_TC_PAGE,           40,  LVCFMT_LEFT, FALSE}
	};
	InitializeHeader2(&m_cListCtrl, LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH, lsp, LAYOUT_NUMBER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgIpmPageLockResources::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cListCtrl.m_hWnd))
		return;
	CRect r;
	GetClientRect (r);
	m_cListCtrl.MoveWindow (r);
}

LONG CuDlgIpmPageLockResources::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	CPtrList listInfoStruct;
	RESOURCEDATAMIN* pData;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;

	CString strID;
	CString strGR; 
	CString strCV;
	CString strCockType;
	CString strDB;
	CString strTable;
	CString strPage;

	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_NULL_RESOURCES:
	case FILTER_RESOURCE_TYPE:
		break;
	default:
		return 0L;
	}
	try
	{
		RESOURCEDATAMIN structData;
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_RESOURCE_DTPO, pUps, (LPVOID)&structData, sizeof(structData));

		bOK = IPM_QueryInfo (&queryInfo, listInfoStruct);
		if (bOK)
		{
			while (!listInfoStruct.IsEmpty())
			{
				pData = (RESOURCEDATAMIN*)listInfoStruct.RemoveHead();
				GetDisplayInfo (pData, strID, strGR, strCV, strCockType, strDB, strTable, strPage);
				AddLockResource (strID, strGR, strCV, strCockType, strDB, strTable, strPage);
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

LONG CuDlgIpmPageLockResources::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataPageLockResources")) == 0);
	CTypedPtrList<CObList, CuDataTupleResource*>* pListTuple;
	pListTuple = (CTypedPtrList<CObList, CuDataTupleResource*>*)lParam;
	CuDataTupleResource* pObj = NULL;
	POSITION pos = pListTuple->GetHeadPosition();
	try
	{
		ResetDisplay();
		while (pos != NULL)
		{
			pObj = (CuDataTupleResource*)pListTuple->GetNext (pos);
			ASSERT (pObj);
			AddLockResource (
				pObj->m_strID, 
				pObj->m_strGR, 
				pObj->m_strCV, 
				pObj->m_strLockType, 
				pObj->m_strDB, 
				pObj->m_strTable,
				pObj->m_strPage);
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageLockResources::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageLockResources* pData = NULL;
	CString strID ;
	CString strGR;
	CString strCV;
	CString strLockType;
	CString strDB;
	CString strTable;
	CString strPage;
	try
	{
		CuDataTupleResource* pObj;
		pData = new CuIpmPropertyDataPageLockResources ();
		CTypedPtrList<CObList, CuDataTupleResource*>& listTuple = pData->m_listTuple;
		int i, nCount = m_cListCtrl.GetItemCount();
		for (i=0; i<nCount; i++)
		{
			strID = m_cListCtrl.GetItemText (i, 0);
			strGR = m_cListCtrl.GetItemText (i, 1); 
			strCV = m_cListCtrl.GetItemText (i, 2);
			strLockType = m_cListCtrl.GetItemText (i, 3);
			strDB = m_cListCtrl.GetItemText (i, 4);
			strTable = m_cListCtrl.GetItemText (i, 5);
			strPage = m_cListCtrl.GetItemText (i, 6);
			pObj = new CuDataTupleResource (strID, strGR, strCV, strLockType, strDB, strTable, strPage);
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

void CuDlgIpmPageLockResources::ResetDisplay()
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
}

void CuDlgIpmPageLockResources::OnDestroy() 
{
	CuListCtrlCleanVoidItemData(&m_cListCtrl);
	m_cListCtrl.DeleteAllItems();
	CDialog::OnDestroy();
}

long CuDlgIpmPageLockResources::OnPropertiesChange(WPARAM wParam, LPARAM lParam)
{
	PropertyChange(&m_cListCtrl, wParam, lParam);
	return 0;
}