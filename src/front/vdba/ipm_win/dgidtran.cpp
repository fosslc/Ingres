/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidtran.cpp, Implementation file.
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Transaction Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgidtran.h"
#include "ipmicdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmIceDetailTransaction, CFormView)


CuDlgIpmIceDetailTransaction::CuDlgIpmIceDetailTransaction()
	: CFormView(CuDlgIpmIceDetailTransaction::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmIceDetailTransaction)
	m_strKey = _T("");
	m_strName = _T("");
	m_strUserSession = _T("");
	m_strConnectionKey = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmIceDetailTransaction::~CuDlgIpmIceDetailTransaction()
{
}

void CuDlgIpmIceDetailTransaction::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIceDetailTransaction)
	DDX_Text(pDX, IDC_EDIT1, m_strKey);
	DDX_Text(pDX, IDC_EDIT2, m_strName);
	DDX_Text(pDX, IDC_EDIT3, m_strUserSession);
	DDX_Text(pDX, IDC_EDIT4, m_strConnectionKey);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIceDetailTransaction, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmIceDetailTransaction)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailTransaction diagnostics

#ifdef _DEBUG
void CuDlgIpmIceDetailTransaction::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmIceDetailTransaction::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailTransaction message handlers


void CuDlgIpmIceDetailTransaction::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmIceDetailTransaction::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmIceDetailTransaction::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strKey = lpszNA;
		m_strName = lpszNA;
		m_strUserSession = lpszNA;
		m_strConnectionKey = lpszNA;
	}
	else
	{
		m_strKey = (LPCTSTR)m_Struct.trans_key;
		m_strName = (LPCTSTR)m_Struct.name;
		m_strUserSession = (LPCTSTR)m_Struct.connect;
		m_strConnectionKey = (LPCTSTR)m_Struct.owner;
	}
}

LONG CuDlgIpmIceDetailTransaction::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int res = RES_ERR;
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
		ResetDisplay();
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
			memcpy (&m_Struct, (ICE_TRANSACTIONDATAMIN*)pUps->pStruct, sizeof(m_Struct));
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

LONG CuDlgIpmIceDetailTransaction::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ICE_TRANSACTIONDATAMIN* pStruct = (ICE_TRANSACTIONDATAMIN*)lParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIceDetailTransaction")) == 0);
	memcpy (&m_Struct, pStruct, sizeof (m_Struct));
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

LONG CuDlgIpmIceDetailTransaction::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaIpmPropertyDataIceDetailTransaction* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIceDetailTransaction (&m_Struct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmIceDetailTransaction::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}