/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiddbco.cpp, Implementation file.
**    Project  : INGRES II/ Ice Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control. The Ice Datbase Connection Detail.
**
** History:
**
** xx-Oct-1998 (uk$so01)
**    Created
**
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgiddbco.h"
#include "ipmicdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmIceDetailDatabaseConnection, CFormView)


CuDlgIpmIceDetailDatabaseConnection::CuDlgIpmIceDetailDatabaseConnection()
	: CFormView(CuDlgIpmIceDetailDatabaseConnection::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmIceDetailDatabaseConnection)
	m_strKey = _T("");
	m_strDriver= _T("");
	m_strDatabaseName= _T("");
	m_strTimeOut= _T("");
	m_bCurrentlyUsed = FALSE;
	//}}AFX_DATA_INIT
}

CuDlgIpmIceDetailDatabaseConnection::~CuDlgIpmIceDetailDatabaseConnection()
{
}

void CuDlgIpmIceDetailDatabaseConnection::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmIceDetailDatabaseConnection)
	DDX_Text(pDX, IDC_EDIT1, m_strKey);
	DDX_Text(pDX, IDC_EDIT2, m_strDriver);
	DDX_Text(pDX, IDC_EDIT3, m_strDatabaseName);
	DDX_Text(pDX, IDC_EDIT4, m_strTimeOut);
	DDX_Check(pDX, IDC_CHECK1, m_bCurrentlyUsed);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmIceDetailDatabaseConnection, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmIceDetailDatabaseConnection)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailDatabaseConnection diagnostics

#ifdef _DEBUG
void CuDlgIpmIceDetailDatabaseConnection::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmIceDetailDatabaseConnection::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmIceDetailDatabaseConnection message handlers


void CuDlgIpmIceDetailDatabaseConnection::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmIceDetailDatabaseConnection::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmIceDetailDatabaseConnection::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strKey = lpszNA;
		m_strDriver = lpszNA;
		m_strDatabaseName = lpszNA;
		m_strTimeOut = lpszNA;
		m_bCurrentlyUsed = FALSE;
	}
	else
	{
		m_strKey    = (LPCTSTR)m_Struct.sessionkey;
		m_strDriver.Format (_T("%s"), (m_Struct.driver == 0)? _T("Ingres"): _T("<Unknown>"));
		m_strDatabaseName   = (LPCTSTR)m_Struct.dbname;
		m_strTimeOut.Format (_T("%d s"), m_Struct.itimeout);
		m_bCurrentlyUsed = (m_Struct.iused == 1)? TRUE: FALSE;
	}
}

LONG CuDlgIpmIceDetailDatabaseConnection::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
			memcpy (&m_Struct, (ICE_DB_CONNECTIONDATAMIN*)pUps->pStruct, sizeof(m_Struct));
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

LONG CuDlgIpmIceDetailDatabaseConnection::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ICE_DB_CONNECTIONDATAMIN* pStruct = (ICE_DB_CONNECTIONDATAMIN*)lParam;
	ASSERT (lstrcmp (pClass, _T("CaIpmPropertyDataIceDetailDatabaseConnection")) == 0);
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

LONG CuDlgIpmIceDetailDatabaseConnection::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaIpmPropertyDataIceDetailDatabaseConnection* pData = NULL;
	try
	{
		pData = new CaIpmPropertyDataIceDetailDatabaseConnection (&m_Struct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}


void CuDlgIpmIceDetailDatabaseConnection::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}