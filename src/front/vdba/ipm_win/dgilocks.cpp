/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgilocks.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail page of Locks.
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 15-Apr-2004 (uk$so01)
**    BUG #112157 / ISSUE 13350172
**    Detail Page of Locked Table has an unexpected extra icons, this is
**    a side effect of change #462417.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgilocks.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmDetailLock, CFormView)


CuDlgIpmDetailLock::CuDlgIpmDetailLock()
	: CFormView(CuDlgIpmDetailLock::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmDetailLock)
	m_strRQ = _T("");
	m_strType = _T("");
	m_strDatabase = _T("");
	m_strPage = _T("");
	m_strState = _T("");
	m_strLockListID = _T("");
	m_strTable = _T("");
	m_strResourceID = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmDetailLock::~CuDlgIpmDetailLock()
{
}

void CuDlgIpmDetailLock::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmDetailLock)
	DDX_Control(pDX, IDC_IMAGE1, m_transparentBmp);
	DDX_Text(pDX, IDC_EDIT1, m_strRQ);
	DDX_Text(pDX, IDC_EDIT2, m_strType);
	DDX_Text(pDX, IDC_EDIT3, m_strDatabase);
	DDX_Text(pDX, IDC_EDIT4, m_strPage);
	DDX_Text(pDX, IDC_EDIT5, m_strState);
	DDX_Text(pDX, IDC_EDIT6, m_strLockListID);
	DDX_Text(pDX, IDC_EDIT7, m_strTable);
	DDX_Text(pDX, IDC_EDIT8, m_strResourceID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmDetailLock, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmDetailLock)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailLock diagnostics

#ifdef _DEBUG
void CuDlgIpmDetailLock::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmDetailLock::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG


void CuDlgIpmDetailLock::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmDetailLock::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmDetailLock::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strRQ = lpszNA;
		m_strType = lpszNA;
		m_strDatabase = lpszNA;
		m_strPage = lpszNA;
		m_strState = lpszNA;
		m_strLockListID = lpszNA;
		m_strTable = lpszNA;
		m_strResourceID = lpszNA;
	}
	else
	{
		TCHAR tchszBuffer [34];

		m_strRQ = m_lcStruct.lockdatamin.lock_request_mode;
		m_strType = FindResourceString(m_lcStruct.lockdatamin.resource_type);
		m_strDatabase = m_lcStruct.lockdatamin.res_database_name;
		m_strPage = IPM_GetLockPageNameStr(&(m_lcStruct.lockdatamin),tchszBuffer);
		m_strState = m_lcStruct.lockdatamin.lock_state;
		m_strLockListID = FormatHexaL (m_lcStruct.lockdatamin.locklist_id, tchszBuffer);
		m_strTable = m_lcStruct.lockdatamin.res_table_name;
		m_strResourceID = FormatHexaL (m_lcStruct.lockdatamin.resource_id, tchszBuffer);

		// custom bitmap
		UINT bmpId = GetLockImageId(& (m_lcStruct.lockdatamin) );
		CImageList img;
		img.Create(bmpId, 16, 1, RGB(255,0,255));
		HICON h = img.ExtractIcon(0);
		m_transparentBmp.SetWindowText(_T(""));
		m_transparentBmp.ResetBitmap(h);
	}
}

LONG CuDlgIpmDetailLock::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
			memset (&m_lcStruct, 0, sizeof (m_lcStruct));
			m_lcStruct.lockdatamin = *(LPLOCKDATAMIN)pUps->pStruct;
		}

		CaIpmQueryInfo queryInfo(pDoc, OT_MON_LOCK);
		bOK = IPM_QueryDetailInfo (&queryInfo, (LPVOID)&m_lcStruct);
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

LONG CuDlgIpmDetailLock::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPLOCKDATAMAX pLc = (LPLOCKDATAMAX)lParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataDetailLock")) == 0);
	memcpy (&m_lcStruct, pLc, sizeof (m_lcStruct));
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

LONG CuDlgIpmDetailLock::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataDetailLock* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataDetailLock (&m_lcStruct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmDetailLock::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}