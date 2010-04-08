/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidcusr.cpp, Implementation file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Connected User Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgidcusr.h"
#include "ipmicdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmIceDetailConnectedUser, CFormView)


CuDlgIpmIceDetailConnectedUser::CuDlgIpmIceDetailConnectedUser()
	: CFormView(CuDlgIpmIceDetailConnectedUser::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmIceDetailConnectedUser)
	m_strName = _T("");
	m_strIceUser= _T("");
	m_strActiveSession= _T("");
	m_strTimeOut= _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmIceDetailConnectedUser::~CuDlgIpmIceDetailConnectedUser()
{
}

void CuDlgIpmIceDetailConnectedUser::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIceDetailConnectedUser)
	DDX_Text(pDX, IDC_EDIT1, m_strName);
	DDX_Text(pDX, IDC_EDIT2, m_strIceUser);
	DDX_Text(pDX, IDC_EDIT3, m_strActiveSession);
	DDX_Text(pDX, IDC_EDIT4, m_strTimeOut);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIceDetailConnectedUser, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmIceDetailConnectedUser)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailConnectedUser diagnostics

#ifdef _DEBUG
void CuDlgIpmIceDetailConnectedUser::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmIceDetailConnectedUser::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailConnectedUser message handlers


void CuDlgIpmIceDetailConnectedUser::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmIceDetailConnectedUser::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmIceDetailConnectedUser::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strName = lpszNA;
		m_strIceUser= lpszNA;
		m_strActiveSession= lpszNA;
		m_strTimeOut= lpszNA;
	}
	else
	{
		m_strName = (LPCTSTR)m_Struct.name;
		m_strIceUser= (LPCTSTR)m_Struct.inguser;
		m_strActiveSession.Format (_T("%ld"), m_Struct.req_count);
		m_strTimeOut.Format (_T("%ld s"), m_Struct.timeout);
	}
}

LONG CuDlgIpmIceDetailConnectedUser::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
			memcpy (&m_Struct, (ICE_CONNECTED_USERDATAMIN*)pUps->pStruct, sizeof(m_Struct));
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

LONG CuDlgIpmIceDetailConnectedUser::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ICE_CONNECTED_USERDATAMIN* pStruct = (ICE_CONNECTED_USERDATAMIN*)lParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIceDetailConnectedUser")) == 0);
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

LONG CuDlgIpmIceDetailConnectedUser::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaIpmPropertyDataIceDetailConnectedUser* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIceDetailConnectedUser (&m_Struct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmIceDetailConnectedUser::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}