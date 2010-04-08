/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgillist.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail page of Lock Lists
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgillist.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmDetailLockList, CFormView)

CuDlgIpmDetailLockList::CuDlgIpmDetailLockList()
	: CFormView(CuDlgIpmDetailLockList::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmDetailLockList)
	m_strSession = _T("");
	m_strStatus = _T("");
	m_strTransID = _T("");
	m_strTotal = _T("");
	m_strLogical = _T("");
	m_strMaxLogical = _T("");
	m_strRelatedLockListID = _T("");
	m_strCurrent = _T("");
	m_strWaitingResourceListID = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmDetailLockList::~CuDlgIpmDetailLockList()
{

}

void CuDlgIpmDetailLockList::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmDetailLockList)
	DDX_Text(pDX, IDC_EDIT1, m_strSession);
	DDX_Text(pDX, IDC_EDIT2, m_strTransID);
	DDX_Text(pDX, IDC_EDIT3, m_strStatus);
	DDX_Text(pDX, IDC_EDIT4, m_strTotal);
	DDX_Text(pDX, IDC_EDIT5, m_strLogical);
	DDX_Text(pDX, IDC_EDIT6, m_strMaxLogical);
	DDX_Text(pDX, IDC_EDIT7, m_strRelatedLockListID);
	DDX_Text(pDX, IDC_EDIT8, m_strCurrent);
	DDX_Text(pDX, IDC_EDIT9, m_strWaitingResourceListID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmDetailLockList, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmDetailLockList)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailLockList diagnostics

#ifdef _DEBUG
void CuDlgIpmDetailLockList::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmDetailLockList::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailLockList message handlers

void CuDlgIpmDetailLockList::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmDetailLockList::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmDetailLockList::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strSession = lpszNA;
		m_strStatus = lpszNA;
		m_strTransID = lpszNA;
		m_strTotal = lpszNA;
		m_strLogical = lpszNA;
		m_strMaxLogical = lpszNA;
		m_strRelatedLockListID = lpszNA;
		m_strCurrent = lpszNA;
		m_strWaitingResourceListID = lpszNA;
	}
	else
	{
		TCHAR tchszBuffer [34];

		m_strSession = FormatHexa64 (m_llStruct.locklistdatamin.locklist_session_id, tchszBuffer);
		m_strStatus = m_llStruct.locklistdatamin.locklist_status;
		m_strTransID = m_llStruct.locklistdatamin.locklist_transaction_id;
		m_strTotal.Format (_T("%ld"), m_llStruct.locklistdatamin.locklist_lock_count);
		m_strLogical.Format(_T("%ld"), m_llStruct.locklistdatamin.locklist_logical_count);
		m_strMaxLogical.Format(_T("%ld"), m_llStruct.locklistdatamin.locklist_max_locks);
		m_strRelatedLockListID = FormatHexaL (m_llStruct.locklist_related_llb_id_id, tchszBuffer);
		m_strCurrent.Format (_T("%ld"), m_llStruct.locklist_related_count);  
		m_strWaitingResourceListID = m_llStruct.WaitingResource;
	}
}


LONG CuDlgIpmDetailLockList::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	LPIPMUPDATEPARAMS  pUps = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_IPM_EXPRESS_REFRESH:
		break;
	default:
		return 0L;
	}

	try
	{
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
		{
			memset (&m_llStruct, 0, sizeof (m_llStruct));
			m_llStruct.locklistdatamin = *(LPLOCKLISTDATAMIN)pUps->pStruct;
		}
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_LOCKLIST);
		bOK = IPM_QueryDetailInfo (&queryInfo, (LPVOID)&m_llStruct);

		if (!bOK)
		{
			InitClassMembers (TRUE);
		}
		else
		{
			InitClassMembers ();
		}
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		InitClassMembers ();
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	return 0L;
}

LONG CuDlgIpmDetailLockList::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPLOCKLISTDATAMAX pLl = (LPLOCKLISTDATAMAX)lParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataDetailLockList")) == 0);
	memcpy (&m_llStruct, pLl, sizeof (m_llStruct));
	try
	{
		ResetDisplay();
		InitClassMembers ();
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmDetailLockList::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataDetailLockList* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataDetailLockList (&m_llStruct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmDetailLockList::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}
