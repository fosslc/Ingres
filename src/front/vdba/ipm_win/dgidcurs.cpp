/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidcurs.cpp, Implementation file
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Cursor Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgidcurs.h"
#include "ipmicdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CuDlgIpmIceDetailCursor, CFormView)


CuDlgIpmIceDetailCursor::CuDlgIpmIceDetailCursor()
	: CFormView(CuDlgIpmIceDetailCursor::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmIceDetailCursor)
	m_strKey = _T("");
	m_strName= _T("");
	m_strOwner= _T("");
	m_strQuery= _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmIceDetailCursor::~CuDlgIpmIceDetailCursor()
{
}

void CuDlgIpmIceDetailCursor::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIceDetailCursor)
	DDX_Text(pDX, IDC_EDIT1, m_strKey);
	DDX_Text(pDX, IDC_EDIT2, m_strName);
	DDX_Text(pDX, IDC_EDIT3, m_strOwner);
	DDX_Text(pDX, IDC_EDIT4, m_strQuery);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIceDetailCursor, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmIceDetailCursor)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailCursor diagnostics

#ifdef _DEBUG
void CuDlgIpmIceDetailCursor::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmIceDetailCursor::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailCursor message handlers


void CuDlgIpmIceDetailCursor::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmIceDetailCursor::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmIceDetailCursor::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strKey   = lpszNA;
		m_strName  = lpszNA;
		m_strOwner = lpszNA;
		m_strQuery = lpszNA;
	}
	else
	{
		m_strKey   = (LPCTSTR)m_Struct.curs_key;
		m_strName  = (LPCTSTR)m_Struct.name;
		m_strOwner = (LPCTSTR)m_Struct.owner;
		m_strQuery = (LPCTSTR)m_Struct.query;
	}
}

LONG CuDlgIpmIceDetailCursor::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
			memcpy (&m_Struct, (ICE_CURSORDATAMIN*)pUps->pStruct, sizeof(m_Struct));
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

LONG CuDlgIpmIceDetailCursor::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ICE_CURSORDATAMIN* pStruct = (ICE_CURSORDATAMIN*)lParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIceDetailCursor")) == 0);
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

LONG CuDlgIpmIceDetailCursor::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaIpmPropertyDataIceDetailCursor* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIceDetailCursor (&m_Struct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmIceDetailCursor::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}
