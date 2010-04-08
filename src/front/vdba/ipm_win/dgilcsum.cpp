/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgilcsum.cpp, Implementation file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail (Summary) page of Lock Info
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
** 26-May-2003 (uk$so01)
**    BUG #110312, Fix the GPF after load.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmframe.h"
#include "ipmdoc.h"
#include "dgilcsum.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmSummaryLockInfo, CFormView)

CuDlgIpmSummaryLockInfo::CuDlgIpmSummaryLockInfo()
	: CFormView(CuDlgIpmSummaryLockInfo::IDD)
{
	m_bParticularState = FALSE;
	m_bStartup = TRUE;
	memset (&m_locStruct, 0, sizeof (m_locStruct));
	//{{AFX_DATA_INIT(CuDlgIpmSummaryLockInfo)
	m_strStartup = _T("Startup");
	m_strLLCreated = _T("");
	m_strLLReleased = _T("");
	m_strLLInUse = _T("");
	m_strLLRemaining = _T("");
	m_strLLTotalAvailable = _T("");
	m_strHTLock = _T("");
	m_strHTResource = _T("");
	m_strResourceInUse = _T("");
	m_strLIRequested = _T("");
	m_strLIReRequested = _T("");
	m_strLIConverted = _T("");
	m_strLIReleased = _T("");
	m_strLICancelled = _T("");
	m_strLIEscalated = _T("");
	m_strLIInUse = _T("");
	m_strLIRemaining = _T("");
	m_strLITotalAvailable = _T("");
	m_strLIDeadlockSearch = _T("");
	m_strLIConvertDeadlock = _T("");
	m_strLIDeadlock = _T("");
	m_strLIConvertWait = _T("");
	m_strLILockWait = _T("");
	m_strLILockxTransaction = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmSummaryLockInfo::~CuDlgIpmSummaryLockInfo()
{
}

void CuDlgIpmSummaryLockInfo::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmSummaryLockInfo)
	DDX_Text(pDX, IDC_EDIT1, m_strStartup);
	DDX_Text(pDX, IDC_EDIT2, m_strLLCreated);
	DDX_Text(pDX, IDC_EDIT3, m_strLLReleased);
	DDX_Text(pDX, IDC_EDIT4, m_strLLInUse);
	DDX_Text(pDX, IDC_EDIT5, m_strLLRemaining);
	DDX_Text(pDX, IDC_EDIT6, m_strLLTotalAvailable);
	DDX_Text(pDX, IDC_EDIT7, m_strHTLock);
	DDX_Text(pDX, IDC_EDIT8, m_strHTResource);
	DDX_Text(pDX, IDC_EDIT9, m_strResourceInUse);
	DDX_Text(pDX, IDC_EDIT10, m_strLIRequested);
	DDX_Text(pDX, IDC_EDIT11, m_strLIReRequested);
	DDX_Text(pDX, IDC_EDIT12, m_strLIConverted);
	DDX_Text(pDX, IDC_EDIT13, m_strLIReleased);
	DDX_Text(pDX, IDC_EDIT14, m_strLICancelled);
	DDX_Text(pDX, IDC_EDIT15, m_strLIEscalated);
	DDX_Text(pDX, IDC_EDIT16, m_strLIInUse);
	DDX_Text(pDX, IDC_EDIT17, m_strLIRemaining);
	DDX_Text(pDX, IDC_EDIT18, m_strLITotalAvailable);
	DDX_Text(pDX, IDC_EDIT19, m_strLIDeadlockSearch);
	DDX_Text(pDX, IDC_EDIT20, m_strLIConvertDeadlock);
	DDX_Text(pDX, IDC_EDIT21, m_strLIDeadlock);
	DDX_Text(pDX, IDC_EDIT22, m_strLIConvertWait);
	DDX_Text(pDX, IDC_EDIT23, m_strLILockWait);
	DDX_Text(pDX, IDC_EDIT24, m_strLILockxTransaction);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmSummaryLockInfo, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmSummaryLockInfo)
	ON_BN_CLICKED(IDC_BUTTON_NOW, OnButtonNow)
	ON_BN_CLICKED(IDC_BUTTON_STARTUP, OnButtonStartup)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmSummaryLockInfo diagnostics

#ifdef _DEBUG
void CuDlgIpmSummaryLockInfo::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmSummaryLockInfo::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmSummaryLockInfo message handlers

void CuDlgIpmSummaryLockInfo::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmSummaryLockInfo::OnButtonNow() 
{
	int res = RES_ERR;
	CTime t = CTime::GetCurrentTime();
	m_strStartup = t.Format (_T("%c"));
	m_bStartup = FALSE;
	m_locStruct.nUse = m_bStartup? 0: 1;
	try
	{
		CFrameWnd* pParent = (CFrameWnd*)GetParentFrame();
		CfIpmFrame* pFrame = (CfIpmFrame*)pParent->GetParentFrame();
		ASSERT(pFrame);
		if (!pFrame)
			return;
		CdIpmDoc* pDoc = pFrame->GetIpmDoc();
		if (!pDoc)
			return;
		if (m_bParticularState)
		{
			m_bParticularState = FALSE;
			pDoc->ManageMonSpecialState();
		}

		CaIpmQueryInfo queryInfo(pDoc, -1);
		BOOL bOK = IPM_GetLockSummaryInfo(&queryInfo, &(m_locStruct.locStruct0), &(m_locStruct.locStruct1), FALSE, TRUE);
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
}

void CuDlgIpmSummaryLockInfo::OnButtonStartup() 
{
	int res = RES_ERR;
	m_bStartup = TRUE;
	m_strStartup = _T("Startup");
	UpdateData(FALSE);
	m_locStruct.nUse = m_bStartup? 0: 1;
	try
	{
		CFrameWnd* pParent = (CFrameWnd*)GetParentFrame();
		CfIpmFrame* pFrame = (CfIpmFrame*)pParent->GetParentFrame();
		ASSERT(pFrame);
		if (!pFrame)
			return;
		CdIpmDoc* pDoc = pFrame->GetIpmDoc();
		if (!pDoc)
			return;
		if (m_bParticularState)
		{
			m_bParticularState = FALSE;
			pDoc->ManageMonSpecialState();
		}

		CaIpmQueryInfo queryInfo(pDoc, -1);
		BOOL bOK = FALSE;
		if (m_bStartup)
			bOK = IPM_GetLockSummaryInfo(&queryInfo, &(m_locStruct.locStruct0), &(m_locStruct.locStruct1), TRUE, FALSE);
		else
			bOK = IPM_GetLockSummaryInfo(&queryInfo, &(m_locStruct.locStruct0), &(m_locStruct.locStruct1), FALSE, FALSE);
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
}

void CuDlgIpmSummaryLockInfo::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strLLCreated = lpszNA;
		m_strLLReleased = lpszNA;
		m_strLLInUse = lpszNA;
		m_strLLRemaining = lpszNA;
		m_strLLTotalAvailable = lpszNA;
		m_strHTLock = lpszNA;
		m_strHTResource = lpszNA;
		m_strResourceInUse = lpszNA;
		m_strLIRequested = lpszNA;
		m_strLIReRequested = lpszNA;
		m_strLIConverted = lpszNA;
		m_strLIReleased = lpszNA;
		m_strLICancelled = lpszNA;
		m_strLIEscalated = lpszNA;
		m_strLIInUse = lpszNA;
		m_strLIRemaining = lpszNA;
		m_strLITotalAvailable = lpszNA;
		m_strLIDeadlockSearch = lpszNA;
		m_strLIConvertDeadlock = lpszNA;
		m_strLIDeadlock = lpszNA;
		m_strLIConvertWait = lpszNA;
		m_strLILockWait = lpszNA;
		m_strLILockxTransaction = lpszNA;
	}
	else
	{
		LOCKSUMMARYDATAMIN lc;
		memcpy (&lc, &(m_locStruct.locStruct0), sizeof (lc));

		m_strLLCreated.Format (_T("%ld"), lc.sumlck_lklist_created_list);
		m_strLLReleased .Format (_T("%ld"), lc.sumlck_lklist_released_all);
		m_strLLInUse.Format (_T("%ld"), lc.sumlck_lklist_inuse);
		m_strLLRemaining.Format (_T("%ld"), lc.sumlck_lklist_remaining);
		m_strLLTotalAvailable.Format(_T("%ld"), lc.sumlck_total_lock_lists_available);
		m_strHTLock .Format (_T("%ld"), lc.sumlck_lkh_size);
		m_strHTResource .Format (_T("%ld"), lc.sumlck_rsh_size);
		m_strResourceInUse.Format (_T("%ld"), lc.sumlck_lkd_rsb_inuse);
		m_strLIRequested.Format (_T("%ld"), lc.sumlck_lkd_request_new);
		m_strLIReRequested.Format (_T("%ld"), lc.sumlck_lkd_request_cvt);
		m_strLIConverted.Format (_T("%ld"), lc.sumlck_lkd_convert);
		m_strLIReleased .Format (_T("%ld"), lc.sumlck_lkd_release);
		m_strLICancelled.Format (_T("%ld"), lc.sumlck_lkd_cancel);
		m_strLIEscalated.Format (_T("%ld"), lc.sumlck_lkd_rlse_partial);
		m_strLIInUse	.Format (_T("%ld"), lc.sumlck_lkd_inuse);
		m_strLIRemaining.Format (_T("%ld"), lc.sumlck_locks_remaining);
		m_strLITotalAvailable.Format (_T("%ld"), lc.sumlck_locks_available);
		m_strLIDeadlockSearch.Format (_T("%ld"), lc.sumlck_lkd_dlock_srch);
		m_strLIConvertDeadlock.Format (_T("%ld"), lc.sumlck_lkd_cvt_deadlock);
		m_strLIDeadlock.Format (_T("%ld"), lc.sumlck_lkd_deadlock);
		m_strLIConvertWait.Format (_T("%ld"), lc.sumlck_lkd_convert_wait);
		m_strLILockWait.Format (_T("%ld"), lc.sumlck_lkd_wait);
		m_strLILockxTransaction .Format (_T("%ld"), lc.sumlck_lkd_max_lkb);
	}
}

LONG CuDlgIpmSummaryLockInfo::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
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
		lstrcpy (m_locStruct.tchszStartTime, (LPCTSTR)m_strStartup);
		m_locStruct.nUse = m_bStartup? 0: 1;
		if (m_bParticularState)
		{
			AfxMessageBox (IDS_E_REFRESH_LOCKING, MB_ICONEXCLAMATION|MB_OK);
			m_bParticularState = FALSE;
		}
		CaIpmQueryInfo queryInfo(pDoc, -1);
		BOOL bOK = FALSE;

		if (m_bStartup)
			bOK = IPM_GetLockSummaryInfo(&queryInfo, &(m_locStruct.locStruct0), &(m_locStruct.locStruct1), TRUE, FALSE);
		else
			bOK = IPM_GetLockSummaryInfo(&queryInfo, &(m_locStruct.locStruct0), &(m_locStruct.locStruct1), FALSE, FALSE);
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

LONG CuDlgIpmSummaryLockInfo::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPLOCKINFOSUMMARY pSummary = (LPLOCKINFOSUMMARY)lParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataSummaryLockInfo")) == 0);
	memcpy (&m_locStruct, pSummary, sizeof (m_locStruct));
	m_bParticularState = TRUE;
	try
	{
		ResetDisplay();
		m_bStartup	 = (m_locStruct.nUse == 0)? TRUE: FALSE;
		m_strStartup = m_locStruct.tchszStartTime;
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

LONG CuDlgIpmSummaryLockInfo::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataSummaryLockInfo* pData = NULL;
	try
	{
		_tcscpy (m_locStruct.tchszStartTime, m_strStartup);
		pData = new CuIpmPropertyDataSummaryLockInfo (&m_locStruct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmSummaryLockInfo::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}
