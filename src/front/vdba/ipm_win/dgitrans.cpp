/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgitrans.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control: Detail page of Transaction
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 24-Sep-2004 (uk$so01)
**    BUG #113124, Convert the 64-bits Session ID to 64 bits Hexa value
**/


#include "stdafx.h"
#include "rcdepend.h"
#include "dgitrans.h"
#include "ipmprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmDetailTransaction, CFormView)

CuDlgIpmDetailTransaction::CuDlgIpmDetailTransaction()
	: CFormView(CuDlgIpmDetailTransaction::IDD)
{
	//{{AFX_DATA_INIT(CuDlgIpmDetailTransaction)
	m_strDatabase = _T("");
	m_strStatus = _T("");
	m_strWrite = _T("");
	m_strSession = _T("");
	m_strSplit = _T("");
	m_strForce = _T("");
	m_strWait = _T("");
	m_strInternalPID = _T("");
	m_strExternalPID = _T("");
	m_strFirst = _T("");
	m_strLast = _T("");
	m_strCP = _T("");
	m_strInternalTXID = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmDetailTransaction::~CuDlgIpmDetailTransaction()
{

}

void CuDlgIpmDetailTransaction::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmDetailTransaction)
	DDX_Text(pDX, IDC_EDIT1, m_strDatabase);
	DDX_Text(pDX, IDC_EDIT2, m_strStatus);
	DDX_Text(pDX, IDC_EDIT3, m_strWrite);
	DDX_Text(pDX, IDC_EDIT4, m_strSession);
	DDX_Text(pDX, IDC_EDIT5, m_strSplit);
	DDX_Text(pDX, IDC_EDIT6, m_strForce);
	DDX_Text(pDX, IDC_EDIT7, m_strWait);
	DDX_Text(pDX, IDC_EDIT8, m_strInternalPID);
	DDX_Text(pDX, IDC_EDIT9, m_strExternalPID);
	DDX_Text(pDX, IDC_EDIT10, m_strFirst);
	DDX_Text(pDX, IDC_EDIT11, m_strLast);
	DDX_Text(pDX, IDC_EDIT12, m_strCP);
	 DDX_Text(pDX, IDC_EDIT13, m_strInternalTXID);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmDetailTransaction, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmDetailTransaction)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailTransaction diagnostics

#ifdef _DEBUG
void CuDlgIpmDetailTransaction::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmDetailTransaction::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmDetailTransaction message handlers

void CuDlgIpmDetailTransaction::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmDetailTransaction::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmDetailTransaction::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strDatabase = lpszNA;
		m_strStatus = lpszNA;
		m_strWrite = lpszNA;
		m_strSession = lpszNA;
		m_strSplit = lpszNA;
		m_strForce = lpszNA;
		m_strWait = lpszNA;
		m_strInternalPID = lpszNA;
		m_strExternalPID = lpszNA;
		m_strFirst = lpszNA;
		m_strLast = lpszNA;
		m_strCP = lpszNA;
		m_strInternalTXID = lpszNA;
	}
	else
	{
		TCHAR tchszBuffer [32];

		m_strDatabase = m_trStruct.logtxdatamin.tx_db_name;
		m_strStatus = m_trStruct.logtxdatamin.tx_status;
		m_strWrite.Format(_T("%ld"),m_trStruct.logtxdatamin.tx_state_write);
		m_strSession = FormatHexa64 (m_trStruct.logtxdatamin.tx_session_id,  tchszBuffer);
		m_strSplit.Format(_T("%ld"), m_trStruct.logtxdatamin.tx_state_split);
		m_strForce.Format(_T("%ld"), m_trStruct.tx_state_force);
		m_strWait.Format (_T("%ld"), m_trStruct.tx_state_wait);
		m_strInternalPID= FormatHexaL (m_trStruct.tx_internal_pid, tchszBuffer);
		m_strExternalPID= FormatHexaL (m_trStruct.tx_external_pid, tchszBuffer);
		m_strFirst = m_trStruct.tx_first_log_address;
		m_strLast = m_trStruct.tx_last_log_address;
		m_strCP = m_trStruct.tx_cp_log_address;
		m_strInternalTXID = FormatHexaL (((65536L*m_trStruct.logtxdatamin.tx_id_instance)+m_trStruct.logtxdatamin.tx_id_id), tchszBuffer);
	}
}

	
LONG CuDlgIpmDetailTransaction::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	BOOL bOK = FALSE;
	LPIPMUPDATEPARAMS  pUps = (LPIPMUPDATEPARAMS)lParam;

	ASSERT (pUps);
	switch (pUps->nIpmHint)
	{
	case 0:
	case FILTER_INACTIVE_TRANSACTIONS:
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
			memset (&m_trStruct, 0, sizeof (m_trStruct));
			m_trStruct.logtxdatamin = *(LPLOGTRANSACTDATAMIN)pUps->pStruct;
		}

		CaIpmQueryInfo queryInfo(pDoc, OT_MON_TRANSACTION);
		bOK = IPM_QueryDetailInfo (&queryInfo, (LPVOID)&m_trStruct);
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

	return 0L;
}

LONG CuDlgIpmDetailTransaction::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPLOGTRANSACTDATAMAX pTran = (LPLOGTRANSACTDATAMAX)lParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataDetailTransaction")) == 0);
	memcpy (&m_trStruct, pTran, sizeof (m_trStruct));
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

LONG CuDlgIpmDetailTransaction::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataDetailTransaction* pData = NULL;
	try
	{
		pData = new CuIpmPropertyDataDetailTransaction (&m_trStruct);
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
	return (LRESULT)pData;
}

void CuDlgIpmDetailTransaction::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}