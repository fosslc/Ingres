/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidhttp.cpp, Implementation file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Http Server Connection Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgidhttp.h"
#include "ipmicdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmIceDetailHttpServerConnection, CFormView)


CuDlgIpmIceDetailHttpServerConnection::CuDlgIpmIceDetailHttpServerConnection()
	: CFormView(CuDlgIpmIceDetailHttpServerConnection::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmIceDetailHttpServerConnection)
	m_strRealUser = _T("");
	m_strID= _T("");
	m_strState= _T("");
	m_strWaitReason= _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmIceDetailHttpServerConnection::~CuDlgIpmIceDetailHttpServerConnection()
{
}

void CuDlgIpmIceDetailHttpServerConnection::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIceDetailHttpServerConnection)
	DDX_Text(pDX, IDC_EDIT1, m_strRealUser);
	DDX_Text(pDX, IDC_EDIT2, m_strID);
	DDX_Text(pDX, IDC_EDIT3, m_strState);
	DDX_Text(pDX, IDC_EDIT4, m_strWaitReason);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIceDetailHttpServerConnection, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmIceDetailHttpServerConnection)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailHttpServerConnection diagnostics

#ifdef _DEBUG
void CuDlgIpmIceDetailHttpServerConnection::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmIceDetailHttpServerConnection::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailHttpServerConnection message handlers


void CuDlgIpmIceDetailHttpServerConnection::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmIceDetailHttpServerConnection::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmIceDetailHttpServerConnection::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strRealUser = lpszNA;
		m_strID = lpszNA;
		m_strState = lpszNA;
		m_strWaitReason = lpszNA;
	}
	else
	{
		TCHAR tchszBuffer[32];
		m_strRealUser    = m_Struct.real_user;
		m_strID = (LPCTSTR)FormatHexa64 (m_Struct.session_id, tchszBuffer);
		m_strState   = (LPCTSTR)m_Struct.session_state;
		m_strWaitReason = (LPCTSTR)m_Struct.session_wait_reason;
	}
}

LONG CuDlgIpmIceDetailHttpServerConnection::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
			memcpy (&m_Struct, (SESSIONDATAMIN*)pUps->pStruct, sizeof(m_Struct));
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

LONG CuDlgIpmIceDetailHttpServerConnection::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	SESSIONDATAMIN* pStruct = (SESSIONDATAMIN*)lParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIceDetailHttpServerConnection")) == 0);
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

LONG CuDlgIpmIceDetailHttpServerConnection::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaIpmPropertyDataIceDetailHttpServerConnection* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIceDetailHttpServerConnection (&m_Struct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmIceDetailHttpServerConnection::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}