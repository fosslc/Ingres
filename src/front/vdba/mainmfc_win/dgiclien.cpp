/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : DgIClien.cpp, Implementation file.                                    //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control: Client page of Session.(host, user, terminal,.)//              
//                                                                                     //
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"
#include "dgiclien.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
#include "monitor.h"
#include "dbaginfo.h"
}

IMPLEMENT_DYNCREATE(CuDlgIpmPageClient, CFormView)

CuDlgIpmPageClient::CuDlgIpmPageClient()
	: CFormView(CuDlgIpmPageClient::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmPageClient)
	m_strHost = _T("");
	m_strUser = _T("");
	m_strConnectedString = _T("");
	m_strTerminal = _T("");
	m_strPID = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmPageClient::~CuDlgIpmPageClient()
{
}

void CuDlgIpmPageClient::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmPageClient)
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strHost);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strUser);
	DDX_Text(pDX, IDC_MFC_EDIT3, m_strConnectedString);
	DDX_Text(pDX, IDC_MFC_EDIT4, m_strTerminal);
	DDX_Text(pDX, IDC_MFC_EDIT5, m_strPID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmPageClient, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmPageClient)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageClient diagnostics

#ifdef _DEBUG
void CuDlgIpmPageClient::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmPageClient::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmPageClient message handlers


void CuDlgIpmPageClient::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
	UpdateData (FALSE);
}

void CuDlgIpmPageClient::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmPageClient::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strHost = lpszNA;
		m_strUser = lpszNA;
		m_strConnectedString = lpszNA;
		m_strTerminal = lpszNA;
		m_strPID = lpszNA;
	}
	else
	{
		m_strHost = m_ssStruct.client_host;
		m_strUser = m_ssStruct.client_user;
		m_strConnectedString = m_ssStruct.client_connect_string;
		m_strTerminal = m_ssStruct.client_terminal;
		m_strPID = m_ssStruct.client_pid;
	}
}


LONG CuDlgIpmPageClient::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	int res = RES_ERR;
	LPIPMUPDATEPARAMS  pUps = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_INTERNAL_SESSIONS:
	case FILTER_IPM_EXPRESS_REFRESH:
		break;
	default:
		return 0L;
	}
	//
	// Initialize the member variables: ...and call UpdateData();
	try
	{
		ResetDisplay();
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
		{
			memset (&m_ssStruct, 0, sizeof (m_ssStruct));
			m_ssStruct.sessiondatamin = *(LPSESSIONDATAMIN)pUps->pStruct;
		}
		res = MonGetDetailInfo((int)wParam, &m_ssStruct, OT_MON_SESSION, 0);
		if (res != RES_SUCCESS)
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
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageClient::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPSESSIONDATAMAX pSess = (LPSESSIONDATAMAX)lParam;
	ASSERT (lstrcmp (pClass, "CuIpmPropertyDataPageClient") == 0);
	memcpy (&m_ssStruct, pSess, sizeof (m_ssStruct));
	try
	{
		ResetDisplay();
		InitClassMembers ();
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmPageClient::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataPageClient* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataPageClient (&m_ssStruct);
	}
	catch (CMemoryException* e)
	{
		VDBA_OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmPageClient::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}