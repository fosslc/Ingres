/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidausr.cpp, Implementation file.
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control. The Ice Active User Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgidausr.h"
#include "ipmicdta.h"
#include "vtree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmIceDetailActiveUser, CFormView)


CuDlgIpmIceDetailActiveUser::CuDlgIpmIceDetailActiveUser()
	: CFormView(CuDlgIpmIceDetailActiveUser::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmIceDetailActiveUser)
	m_strUser = _T("");
	m_strSession= _T("");
	m_strQuery= _T("");
	m_strHttpHostServer= _T("");
	m_strError= _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmIceDetailActiveUser::~CuDlgIpmIceDetailActiveUser()
{
}

void CuDlgIpmIceDetailActiveUser::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIceDetailActiveUser)
	DDX_Text(pDX, IDC_EDIT1, m_strUser);
	DDX_Text(pDX, IDC_EDIT2, m_strSession);
	DDX_Text(pDX, IDC_EDIT3, m_strQuery);
	DDX_Text(pDX, IDC_EDIT4, m_strHttpHostServer);
	DDX_Text(pDX, IDC_EDIT5, m_strError);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIceDetailActiveUser, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmIceDetailActiveUser)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailActiveUser diagnostics

#ifdef _DEBUG
void CuDlgIpmIceDetailActiveUser::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmIceDetailActiveUser::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailActiveUser message handlers


void CuDlgIpmIceDetailActiveUser::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmIceDetailActiveUser::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmIceDetailActiveUser::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strUser = lpszNA;
		m_strSession = lpszNA;
		m_strQuery = lpszNA;
		m_strHttpHostServer = lpszNA;
		m_strError = lpszNA;
	}
	else
	{
		m_strUser.LoadString(IDS_NOT_AVAILABLE);
		m_strSession = (LPCTSTR)m_Struct.name;
		m_strQuery   = (LPCTSTR)m_Struct.query;
		m_strHttpHostServer = (LPCTSTR)m_Struct.host;
		m_strError.Format (_T("%ld"), m_Struct.count);
	}
}

LONG CuDlgIpmIceDetailActiveUser::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
			memcpy (&m_Struct, (ICE_ACTIVE_USERDATAMIN*)pUps->pStruct, sizeof(m_Struct));
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

LONG CuDlgIpmIceDetailActiveUser::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ICE_ACTIVE_USERDATAMIN* pStruct = (ICE_ACTIVE_USERDATAMIN*)lParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIceDetailActiveUser")) == 0);
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

LONG CuDlgIpmIceDetailActiveUser::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaIpmPropertyDataIceDetailActiveUser* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIceDetailActiveUser (&m_Struct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmIceDetailActiveUser::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}