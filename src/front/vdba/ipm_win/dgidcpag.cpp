/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgidcpag.cpp, Implementation file
**    Project  : INGRES II/ Monitoring
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Cache Page Detail
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgidcpag.h"
#include "ipmicdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmIceDetailCachePage, CFormView)


CuDlgIpmIceDetailCachePage::CuDlgIpmIceDetailCachePage()
	: CFormView(CuDlgIpmIceDetailCachePage::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmIceDetailCachePage)
	m_strName = _T("");
	m_strLocation = _T("");
	m_strRequester = _T("");
	m_strCounter = _T("");
	m_strTimeOut = _T("");
	m_strStatus = _T("");
	m_bUsable = FALSE;
	m_bExistingFile = FALSE;
	//}}AFX_DATA_INIT
}

CuDlgIpmIceDetailCachePage::~CuDlgIpmIceDetailCachePage()
{
}

void CuDlgIpmIceDetailCachePage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIceDetailCachePage)
	DDX_Text(pDX, IDC_EDIT1, m_strName);
	DDX_Text(pDX, IDC_EDIT2, m_strLocation);
	DDX_Text(pDX, IDC_EDIT3, m_strRequester);
	DDX_Text(pDX, IDC_EDIT4, m_strCounter);
	DDX_Text(pDX, IDC_EDIT5, m_strTimeOut);
	DDX_Text(pDX, IDC_EDIT6, m_strStatus);
	DDX_Check(pDX, IDC_CHECK1, m_bUsable);
	DDX_Check(pDX, IDC_CHECK2, m_bExistingFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIceDetailCachePage, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmIceDetailCachePage)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailCachePage diagnostics

#ifdef _DEBUG
void CuDlgIpmIceDetailCachePage::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmIceDetailCachePage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailCachePage message handlers


void CuDlgIpmIceDetailCachePage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmIceDetailCachePage::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmIceDetailCachePage::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strName = lpszNA;
		m_strLocation = lpszNA;
		m_strRequester = lpszNA;
		m_strCounter = lpszNA;
		m_strTimeOut = lpszNA;
		m_strStatus = lpszNA;
		m_bUsable = FALSE;
		m_bExistingFile = FALSE;
	}
	else
	{
		m_strName = (LPCTSTR)m_Struct.name;
		m_strLocation = (LPCTSTR)m_Struct.loc_name;
		m_strRequester = (LPCTSTR)m_Struct.owner;
		m_strCounter.Format (_T("%d"), m_Struct.counter);
		m_strTimeOut.Format (_T("%d s"), m_Struct.itimeout);
		m_strStatus = (LPCTSTR)m_Struct.status;
		
		m_bUsable = (m_Struct.in_use == 1)? TRUE: FALSE;
		m_bExistingFile = (m_Struct.iexist == 1)? TRUE: FALSE;
	}
}



LONG CuDlgIpmIceDetailCachePage::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
			memcpy (&m_Struct, (ICE_CACHEINFODATAMIN*)pUps->pStruct, sizeof(m_Struct));
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

LONG CuDlgIpmIceDetailCachePage::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ICE_CACHEINFODATAMIN* pStruct = (ICE_CACHEINFODATAMIN*)lParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIceDetailCachePage")) == 0);
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

LONG CuDlgIpmIceDetailCachePage::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaIpmPropertyDataIceDetailCachePage* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIceDetailCachePage (&m_Struct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmIceDetailCachePage::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}
