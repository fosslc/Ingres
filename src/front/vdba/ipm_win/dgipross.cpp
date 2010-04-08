/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipross.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail page of Process
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgipross.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmDetailProcess, CFormView)

CuDlgIpmDetailProcess::CuDlgIpmDetailProcess()
	: CFormView(CuDlgIpmDetailProcess::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmDetailProcess)
	m_strOSPID = _T("");
	m_strType = _T("");
	m_strLogFileWriteRequest = _T("");
	m_strWait = _T("");
	m_strOpenDB = _T("");
	m_strLogForce = _T("");
	m_strStarted = _T("");
	m_strEnded = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmDetailProcess::~CuDlgIpmDetailProcess()
{

}

void CuDlgIpmDetailProcess::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmDetailProcess)
	DDX_Text(pDX, IDC_EDIT1, m_strOSPID);
	DDX_Text(pDX, IDC_EDIT2, m_strType);
	DDX_Text(pDX, IDC_EDIT3, m_strLogFileWriteRequest);
	DDX_Text(pDX, IDC_EDIT4, m_strWait);
	DDX_Text(pDX, IDC_EDIT5, m_strOpenDB);
	DDX_Text(pDX, IDC_EDIT6, m_strLogForce);
	DDX_Text(pDX, IDC_EDIT7, m_strStarted);
	DDX_Text(pDX, IDC_EDIT8, m_strEnded);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmDetailProcess, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmDetailProcess)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailProcess diagnostics

#ifdef _DEBUG
void CuDlgIpmDetailProcess::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmDetailProcess::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailProcess message handlers



void CuDlgIpmDetailProcess::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmDetailProcess::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmDetailProcess::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strOSPID = lpszNA;
		m_strType = lpszNA;
		m_strLogFileWriteRequest = lpszNA;
		m_strWait = lpszNA;
		m_strOpenDB = lpszNA;
		m_strLogForce = lpszNA;
		m_strStarted = lpszNA;
		m_strEnded = lpszNA;
	}
	else
	{
		TCHAR tchszBuffer [34];

		m_strOSPID = FormatHexaL(m_pssStruct.logprocessdatmin.process_pid,tchszBuffer);
		m_strType = m_pssStruct.logprocessdatmin.process_status;
		m_strLogFileWriteRequest.Format (_T("%ld"), m_pssStruct.logprocessdatmin.process_writes);
		m_strWait.Format(_T("%ld"), m_pssStruct.logprocessdatmin.process_waits);
		m_strOpenDB.Format(_T("%ld"), m_pssStruct.logprocessdatmin.process_count);
		m_strLogForce.Format(_T("%ld"), m_pssStruct.logprocessdatmin.process_log_forces);
		m_strStarted.Format (_T("%ld"), m_pssStruct.logprocessdatmin.process_tx_begins);
		m_strEnded.Format(_T("%ld"), m_pssStruct.logprocessdatmin.process_tx_ends);
	}
}


LONG CuDlgIpmDetailProcess::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
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
		CdIpmDoc* pDoc = (CdIpmDoc*)wParam;
		ResetDisplay();
		if (pUps->nIpmHint != FILTER_IPM_EXPRESS_REFRESH)
		{
			memset (&m_pssStruct, 0, sizeof (m_pssStruct));
			m_pssStruct.logprocessdatmin = *(LPLOGPROCESSDATAMIN)pUps->pStruct;
		}
		CaIpmQueryInfo queryInfo(pDoc, OT_MON_LOGPROCESS);
		bOK = IPM_QueryDetailInfo (&queryInfo, (LPVOID)&m_pssStruct);
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
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	catch (CeIpmException e)
	{
		InitClassMembers ();
		AfxMessageBox (e.GetReason(), MB_ICONEXCLAMATION|MB_OK);
	}
	return 0L;
}

LONG CuDlgIpmDetailProcess::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPLOGPROCESSDATAMAX pPss = (LPLOGPROCESSDATAMAX)lParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataDetailProcess")) == 0);
	memcpy (&m_pssStruct, pPss, sizeof (m_pssStruct));
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

LONG CuDlgIpmDetailProcess::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataDetailProcess* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataDetailProcess (&m_pssStruct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmDetailProcess::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}
