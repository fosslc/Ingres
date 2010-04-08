/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : rvstatus.cpp : implementation file
**    Project  : CA-OpenIngres/Replicator.
**    Author   : UK Sotheavut
**    Purpose  : Page of a Replication Server Item  (Status)
**
** History:
**
** xx-Dec-1997 (uk$so01)
**    Created
** 20-May-1999 (schph01)
**    Change the column width in the CListCtrl (CuListCtrl).
**
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "rvstatus.h"
#include "ipmprdta.h"
#include "ipmframe.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgReplicationServerPageStatus::CuDlgReplicationServerPageStatus(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgReplicationServerPageStatus::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgReplicationServerPageStatus)
	m_strEdit1 = _T("");
	m_strEdit2 = _T("");
	//}}AFX_DATA_INIT
	m_bDisplayFirstTime = TRUE;
}


void CuDlgReplicationServerPageStatus::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgReplicationServerPageStatus)
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonStop);
	DDX_Control(pDX, IDC_BUTTON1, m_cButtonStart);
	DDX_Control(pDX, IDC_EDIT2, m_cEdit2);
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_EDIT1, m_strEdit1);
	DDX_Text(pDX, IDC_EDIT2, m_strEdit2);
	//}}AFX_DATA_MAP

	// Ensure last line visible in multiline edit (may cause flash)
	if (!pDX->m_bSaveAndValidate)
		m_cEdit2.LineScroll(m_cEdit2.GetLineCount());
}


BEGIN_MESSAGE_MAP(CuDlgReplicationServerPageStatus, CDialog)
	//{{AFX_MSG_MAP(CuDlgReplicationServerPageStatus)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonStart)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonStop)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON3, OnButtonViewLog)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
	ON_MESSAGE (WM_USER_IPMPAGE_LEAVINGPAGE, OnLeavingPage)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationServerPageStatus message handlers

void CuDlgReplicationServerPageStatus::OnButtonStart() 
{
	CWaitCursor hourglass;
	try
	{
		CfIpmFrame* pFrame = (CfIpmFrame*)GetParentFrame();
		CdIpmDoc* pDoc = (CdIpmDoc*)pFrame->GetActiveDocument();

		IPM_StartReplicationServer(pDoc, &m_SvrDta, m_strEdit2);
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
	}

	EnableButtons();
}

void CuDlgReplicationServerPageStatus::OnButtonStop() 
{
	CWaitCursor hourglass;
	try
	{
		CfIpmFrame* pFrame = (CfIpmFrame*)GetParentFrame();
		CdIpmDoc* pDoc = (CdIpmDoc*)pFrame->GetActiveDocument();

		IPM_StopReplicationServer(pDoc, &m_SvrDta, m_strEdit2);
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
	}

	EnableButtons();
}

void CuDlgReplicationServerPageStatus::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgReplicationServerPageStatus::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	if (!IsWindow (m_cEdit1.m_hWnd))
		return;
	CRect rDlg, r;
	GetClientRect (rDlg);
	m_cEdit1.GetWindowRect (r);
	ScreenToClient (r);
	r.right = rDlg.right - r.left;
	m_cEdit1.MoveWindow (r);

	m_cEdit2.GetWindowRect (r);
	ScreenToClient (r);
	r.right  = rDlg.right  - r.left;
	r.bottom = rDlg.bottom - r.left;
	m_cEdit2.MoveWindow (r);
}

void CuDlgReplicationServerPageStatus::EnableButtons()
{
	BOOL bEnableStart = FALSE;
	BOOL bEnableStop  = FALSE;
	switch (m_SvrDta.startstatus)
	{
	case REPSVR_UNKNOWNSTATUS:
		bEnableStart = TRUE;
		bEnableStop  = TRUE;
		break;
	case REPSVR_STARTED:
		bEnableStart = FALSE;
		bEnableStop  = TRUE;
		break;
	case REPSVR_STOPPED:
		bEnableStart = TRUE;
		bEnableStop  = FALSE;
		break;
	case REPSVR_ERROR:
		bEnableStart = FALSE;
		bEnableStop  = FALSE;
		break;
	default:
		bEnableStart = FALSE;
		bEnableStop  = FALSE;
		break;
	}
	m_cButtonStart.EnableWindow (bEnableStart);
	m_cButtonStop.EnableWindow (bEnableStop);
}

LONG CuDlgReplicationServerPageStatus::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	//m_nNodeHandle = (int)wParam;
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	
	memcpy (&m_SvrDta, (LPREPLICSERVERDATAMIN)pUps->pStruct, sizeof(REPLICSERVERDATAMIN));
	m_strEdit1 = (LPTSTR)(m_SvrDta.FullStatus);
	if ( m_bDisplayFirstTime ) 
	{
		m_strEdit2.LoadString(IDS_E_VIEW_FIRST_REPLIC_LOG);
		//_T("To view the replicat.log file, please press\r\n"
		//"the \"View Log\" button.");
		m_bDisplayFirstTime = FALSE;
	}
	UpdateData (FALSE);
	EnableButtons();
	return 0L;
}

LONG CuDlgReplicationServerPageStatus::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CaReplicationServerDataPageStatus")) == 0);
	CaReplicationServerDataPageStatus* pData = (CaReplicationServerDataPageStatus*)lParam;
	memcpy (&m_SvrDta, &(pData->m_dlgData), sizeof(REPLICSERVERDATAMIN));

	m_strEdit1 = (LPTSTR)(pData->m_dlgData.FullStatus);
	m_strEdit2 = pData->m_strStartLogFile;
	m_bDisplayFirstTime = FALSE;
	UpdateData (FALSE);
	EnableButtons();
	return 0L;
}

LONG CuDlgReplicationServerPageStatus::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CaReplicationServerDataPageStatus* pData = NULL;
	try
	{
		pData = new CaReplicationServerDataPageStatus ();
		memcpy (&(pData->m_dlgData), &m_SvrDta, sizeof (REPLICSERVERDATAMIN));
		pData->m_strStartLogFile = m_strEdit2;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	return (LRESULT)pData;
}

LONG CuDlgReplicationServerPageStatus::OnLeavingPage(WPARAM wParam, LPARAM lParam)
{
	/* if necessary: */
	LeavePageType type = (LeavePageType)wParam;
	switch (type) {
		case LEAVINGPAGE_CHANGELEFTSEL:
			m_strEdit2.LoadString(IDS_E_VIEW_FIRST_REPLIC_LOG);
				//_T("To view the replicat.log file, please press\r\n"
				//"the \"View Log\" button.");
			UpdateData (FALSE);
			// specific
			break;
		case LEAVINGPAGE_CHANGEPROPPAGE:
			// specific
			break;
		case LEAVINGPAGE_CLOSEDOCUMENT:
			// specific
			break;
		default:
			ASSERT(FALSE);
	}
	return 0L;
	
}

void CuDlgReplicationServerPageStatus::OnButtonViewLog() 
{
	try
	{
		IPM_ViewReplicationServerLog (&m_SvrDta, m_strEdit2);
		UpdateData (FALSE);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	catch (...)
	{
	}
}

