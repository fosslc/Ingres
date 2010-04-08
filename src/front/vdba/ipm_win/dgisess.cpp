/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgisess.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control: Detail page of Session
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 25-Jan-2000 (uk$so01)
**    SIR #100118: Add a description of the session.
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 30-Mar-2001 (noifr01)
**    (sir 104378) differentiation of II 2.6.
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgisess.h"
#include "ipmprdta.h"
#include "ipmframe.h"
#include "ipmdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmDetailSession, CFormView)

CuDlgIpmDetailSession::CuDlgIpmDetailSession()
	: CFormView(CuDlgIpmDetailSession::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmDetailSession)
	m_strName = _T("");
	m_strState = _T("");
	m_strMask = _T("");
	m_strTerminal = _T("");
	m_strID = _T("");
	m_strUserReal = _T("");
	m_strUserApparent = _T("");
	m_strDBName = _T("");
	m_strDBA = _T("");
	m_strGroupID = _T("");
	m_strRoleID = _T("");
	m_strServerType = _T("");
	m_strServerFacility = _T("");
	m_strServerListenAddress = _T("");
	m_strActivitySessionState = _T("");
	m_strActivityDetail = _T("");
	m_strQuery = _T("select * from IIDBDB");
	m_strDescription = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmDetailSession::~CuDlgIpmDetailSession()
{
}

void CuDlgIpmDetailSession::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmDetailSession)
	DDX_Control(pDX, IDC_STATIC18, m_cStaticDescription);
	DDX_Control(pDX, IDC_EDIT18, m_cEditDescription);
	DDX_Text(pDX, IDC_EDIT1, m_strName);
	DDX_Text(pDX, IDC_EDIT2, m_strState);
	DDX_Text(pDX, IDC_EDIT3, m_strMask);
	DDX_Text(pDX, IDC_EDIT4, m_strTerminal);
	DDX_Text(pDX, IDC_EDIT5, m_strID);
	DDX_Text(pDX, IDC_EDIT6, m_strUserReal);
	DDX_Text(pDX, IDC_EDIT7, m_strUserApparent);
	DDX_Text(pDX, IDC_EDIT8, m_strDBName);
	DDX_Text(pDX, IDC_EDIT9, m_strDBA);
	DDX_Text(pDX, IDC_EDIT10, m_strGroupID);
	DDX_Text(pDX, IDC_EDIT11, m_strRoleID);
	DDX_Text(pDX, IDC_EDIT12, m_strServerType);
	DDX_Text(pDX, IDC_EDIT14, m_strServerFacility);
	DDX_Text(pDX, IDC_EDIT13, m_strServerListenAddress);
	DDX_Text(pDX, IDC_EDIT15, m_strActivitySessionState);
	DDX_Text(pDX, IDC_EDIT16, m_strActivityDetail);
	DDX_Text(pDX, IDC_EDIT17, m_strQuery);
	DDX_Text(pDX, IDC_EDIT18, m_strDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmDetailSession, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmDetailSession)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailSession diagnostics

#ifdef _DEBUG
void CuDlgIpmDetailSession::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmDetailSession::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailSession message handlers


void CuDlgIpmDetailSession::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
	UpdateData (FALSE);
}

void CuDlgIpmDetailSession::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}
extern LPTSTR FormatHexa64(LPCTSTR lpszString,  LPTSTR lpszBuffer);

void CuDlgIpmDetailSession::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strName = lpszNA;
		m_strState = lpszNA;
		m_strMask = lpszNA;
		m_strTerminal = lpszNA;
		m_strID = lpszNA;
		m_strUserReal = lpszNA;
		m_strUserApparent = lpszNA;
		m_strDBName = lpszNA;
		m_strDBA = lpszNA;
		m_strGroupID = lpszNA;
		m_strRoleID = lpszNA;
		m_strServerType = lpszNA;
		m_strServerFacility = lpszNA;
		m_strServerListenAddress = lpszNA;
		m_strActivitySessionState = lpszNA;
		m_strActivityDetail = lpszNA;
		m_strQuery = lpszNA;
		m_strDescription = lpszNA;
	}
	else
	{
		TCHAR tchszBuffer [34];

		m_strName = m_ssStruct.sessiondatamin.session_name;
		m_strState = FormatStateString(&m_ssStruct.sessiondatamin, tchszBuffer);
		m_strMask = m_ssStruct.session_mask;
		m_strTerminal = m_ssStruct.sessiondatamin.session_terminal;
		m_strID = FormatHexa64(m_ssStruct.sessiondatamin.session_id, tchszBuffer);
		m_strUserReal = m_ssStruct.sessiondatamin.real_user;
		m_strUserApparent = m_ssStruct.sessiondatamin.effective_user;
		m_strDBName = m_ssStruct.sessiondatamin.db_name;
		m_strDBA = m_ssStruct.db_owner;
		m_strGroupID = m_ssStruct.session_group;
		m_strRoleID = m_ssStruct.session_role;
		m_strServerType = m_ssStruct.sessiondatamin.sesssvrdata.server_class;
		m_strServerFacility = m_ssStruct.sessiondatamin.server_facility;
		m_strServerListenAddress = m_ssStruct.sessiondatamin.sesssvrdata.listen_address;
		m_strActivitySessionState = m_ssStruct.session_activity;
		m_strActivityDetail = m_ssStruct.activity_detail;
		m_strQuery = m_ssStruct.session_full_query;
		m_strDescription = m_ssStruct.session_description;
	}
}


LONG CuDlgIpmDetailSession::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
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
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
		{
			memset (&m_ssStruct, 0, sizeof (m_ssStruct));
			m_ssStruct.sessiondatamin = *(LPSESSIONDATAMIN)pUps->pStruct;
		}

		CaIpmQueryInfo queryInfo(pDoc, OT_MON_SESSION);
		bOK = IPM_QueryDetailInfo (&queryInfo, (LPVOID)&m_ssStruct);
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
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		InitClassMembers ();
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	return 0L;
}

LONG CuDlgIpmDetailSession::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPSESSIONDATAMAX pSess = (LPSESSIONDATAMAX)lParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataDetailSession")) == 0);
	memcpy (&m_ssStruct, pSess, sizeof (m_ssStruct));
	try
	{
		ResetDisplay();
		InitClassMembers ();
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return 0L;
}

LONG CuDlgIpmDetailSession::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataDetailSession* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataDetailSession (&m_ssStruct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmDetailSession::ResetDisplay()
{
	int nOiVersion = 0;
	//
	// We have 2 frames here: 
	// 1) Frame used by CFormView derived class (this dialog)
	// 2) The Ipm frame (CfIpmFrame)
	CFrameWnd* pFormViewFrame = (CFrameWnd*)GetParentFrame();
	ASSERT (pFormViewFrame);
	if (!pFormViewFrame)
		return;
	CfIpmFrame* pIpmFrame = (CfIpmFrame*)pFormViewFrame->GetParentFrame();
	ASSERT(pIpmFrame);
	if (pIpmFrame)
	{
		CdIpmDoc* pIpmDoc = pIpmFrame->GetIpmDoc();
		ASSERT (pIpmDoc);
		nOiVersion = pIpmDoc->GetDbmsVersion();
	}

	if (nOiVersion < OIVERS_25)
	{
		if (IsWindow (m_cEditDescription.m_hWnd) && IsWindow (m_cStaticDescription.m_hWnd))
		{
			m_cEditDescription.ShowWindow (SW_HIDE);
			m_cStaticDescription.ShowWindow (SW_HIDE);
		}
	}
	else
	{
		if (IsWindow (m_cEditDescription.m_hWnd) && IsWindow (m_cStaticDescription.m_hWnd))
		{
			m_cEditDescription.ShowWindow (SW_SHOW);
			m_cStaticDescription.ShowWindow (SW_SHOW);
		}
	}

	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}
