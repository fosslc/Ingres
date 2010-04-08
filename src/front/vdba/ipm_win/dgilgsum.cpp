/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgilgsum.cpp, Implementation file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail (Summary) page of Log Info
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 05-Dec-2000 (schph01)
**    BUG 103387 Refresh m_strStartup only apply in methods OnButtonStartup() and
**    OnButtonNow().
** 06-Dec-2000 (schph01)
**    SIR 102822 Calculate and display the "Log Commits per second" and
**    "Log Write I/O per second" informations in the "log information"
**    "summary" right pane
** 06-Dec-2000 (schph01)
**    BUG 103388 call OnButtonStartup() method after message IDS_E_REFRESH_LOGGING
** 26-May-2003 (uk$so01)
**    BUG #110312, Fix the GPF after load.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmframe.h"
#include "ipmdoc.h"
#include "dgilgsum.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CuDlgIpmSummaryLogInfo, CFormView)

CuDlgIpmSummaryLogInfo::CuDlgIpmSummaryLogInfo()
	: CFormView(CuDlgIpmSummaryLogInfo::IDD)
{
	m_bParticularState = FALSE;
	m_bStartup = TRUE;
	memset (&m_logStruct, 0, sizeof (m_logStruct));
	//{{AFX_DATA_INIT(CuDlgIpmSummaryLogInfo)
	m_strStartup = _T("Startup");
	m_strDatabaseAdds = _T("");
	m_strDatabaseRemoves = _T("");
	m_strTransactionBegins = _T("");
	m_strTransactionEnds = _T("");
	m_strLogWrites = _T("");
	m_strLogWriteIOs = _T("");
	m_strLogReadIOs = _T("");
	m_strLogForces = _T("");
	m_strLogWaits = _T("");
	m_strLogSplitWaits = _T("");
	m_strLogFreeWaits = _T("");
	m_strLogStallWaits = _T("");
	m_strCheckCommitTimer = _T("");
	m_strTimerWrite = _T("");
	m_strLogGroupCommit = _T("");
	m_strLogGroupCount = _T("");
	m_strInconsistentDatabase = _T("");
	m_strKBytesWritten = _T("");
	m_strLogCommitAverage = _T("");
	m_strLogCommitCurrent = _T("");
	m_strLogCommitMax = _T("");
	m_strLogCommitMin = _T("");
	m_strLogWriteAverage = _T("");
	m_strLogWriteCurrent = _T("");
	m_strLogWriteMax = _T("");
	m_strLogWriteMin = _T("");
	//}}AFX_DATA_INIT
}

CuDlgIpmSummaryLogInfo::~CuDlgIpmSummaryLogInfo()
{
}

void CuDlgIpmSummaryLogInfo::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgIpmSummaryLogInfo)
	DDX_Control(pDX, IDC_STATIC_LOG_WRITE, m_ctrlLogWrite);
	DDX_Control(pDX, IDC_STATIC_LOG_SECOND, m_ctrlStaticLogSecond);
	DDX_Control(pDX, IDC_EDIT_LOG_WRITE_MAX, m_ctrlLogWriteMax);
	DDX_Control(pDX, IDC_EDIT_LOG_WRITE_MIN, m_ctrlLogWriteMin);
	DDX_Control(pDX, IDC_EDIT_LOG_WRITE_CURRENT, m_ctrlLogWriteCurrent);
	DDX_Control(pDX, IDC_EDIT_LOG_WRITE_AVERAGE, m_ctrlLlogWriteAverage);
	DDX_Control(pDX, IDC_STATIC_LOG_WRITE_AVERAGE2, m_ctrlStatLogWriteAverage);
	DDX_Control(pDX, IDC_STATIC_LOG_WRITE_CURRENT, m_ctrlStatLogWriteCurrent);
	DDX_Control(pDX, IDC_STATIC_LOG_WRITE_MAX, m_ctrlStatLogWriteMax);
	DDX_Control(pDX, IDC_STATIC_LOG_WRITE_MIN, m_ctrlStatLogWriteMin);
	DDX_Control(pDX, IDC_STATIC_LOG_AVERAGE, m_ctrlStatLogCommitAverage);
	DDX_Control(pDX, IDC_STATIC_LOG_MAX, m_ctrlStatLogCommitMax);
	DDX_Control(pDX, IDC_STATIC_LOG_MIN, m_ctrlStatLogCommitMin);
	DDX_Control(pDX, IDC_STATIC_LOG_CURRENT, m_ctrlStatLogCommitCurrent);
	DDX_Control(pDX, IDC_EDIT_LOG_COMMIT_MIN, m_ctrlLogCommitMin);
	DDX_Control(pDX, IDC_EDIT_LOG_COMMIT_MAX, m_ctrlLogCommitMax);
	DDX_Control(pDX, IDC_EDIT_LOG_COMMIT_CURRENT, m_ctrlLogCommitCurrent);
	DDX_Control(pDX, IDC_EDIT_LOG_COMMIT_AVERAGE, m_ctrlLogCommitAverage);
	DDX_Text(pDX, IDC_EDIT1, m_strStartup);
	DDX_Text(pDX, IDC_EDIT2, m_strDatabaseAdds);
	DDX_Text(pDX, IDC_EDIT3, m_strDatabaseRemoves);
	DDX_Text(pDX, IDC_EDIT4, m_strTransactionBegins);
	DDX_Text(pDX, IDC_EDIT5, m_strTransactionEnds);
	DDX_Text(pDX, IDC_EDIT6, m_strLogWrites);
	DDX_Text(pDX, IDC_EDIT7, m_strLogWriteIOs);
	DDX_Text(pDX, IDC_EDIT8, m_strLogReadIOs);
	DDX_Text(pDX, IDC_EDIT9, m_strLogForces);
	DDX_Text(pDX, IDC_EDIT10, m_strLogWaits);
	DDX_Text(pDX, IDC_EDIT11, m_strLogSplitWaits);
	DDX_Text(pDX, IDC_EDIT12, m_strLogFreeWaits);
	DDX_Text(pDX, IDC_EDIT13, m_strLogStallWaits);
	DDX_Text(pDX, IDC_EDIT14, m_strCheckCommitTimer);
	DDX_Text(pDX, IDC_EDIT15, m_strTimerWrite);
	DDX_Text(pDX, IDC_EDIT16, m_strLogGroupCommit);
	DDX_Text(pDX, IDC_EDIT17, m_strLogGroupCount);
	DDX_Text(pDX, IDC_EDIT18, m_strInconsistentDatabase);
	DDX_Text(pDX, IDC_EDIT19, m_strKBytesWritten);
	DDX_Text(pDX, IDC_EDIT_LOG_COMMIT_AVERAGE, m_strLogCommitAverage);
	DDX_Text(pDX, IDC_EDIT_LOG_COMMIT_CURRENT, m_strLogCommitCurrent);
	DDX_Text(pDX, IDC_EDIT_LOG_COMMIT_MAX, m_strLogCommitMax);
	DDX_Text(pDX, IDC_EDIT_LOG_COMMIT_MIN, m_strLogCommitMin);
	DDX_Text(pDX, IDC_EDIT_LOG_WRITE_AVERAGE, m_strLogWriteAverage);
	DDX_Text(pDX, IDC_EDIT_LOG_WRITE_CURRENT, m_strLogWriteCurrent);
	DDX_Text(pDX, IDC_EDIT_LOG_WRITE_MAX, m_strLogWriteMax);
	DDX_Text(pDX, IDC_EDIT_LOG_WRITE_MIN, m_strLogWriteMin);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgIpmSummaryLogInfo, CFormView)
	//{{AFX_MSG_MAP(CuDlgIpmSummaryLogInfo)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_STARTUP, OnButtonStartup)
	ON_BN_CLICKED(IDC_BUTTON_NOW, OnButtonNow)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,   OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,   OnGetData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmSummaryLogInfo diagnostics

#ifdef _DEBUG
void CuDlgIpmSummaryLogInfo::AssertValid() const
{
	CFormView::AssertValid();
}

void CuDlgIpmSummaryLogInfo::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CuDlgIpmSummaryLogInfo message handlers


void CuDlgIpmSummaryLogInfo::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
}

void CuDlgIpmSummaryLogInfo::OnSize(UINT nType, int cx, int cy) 
{
	CFormView::OnSize(nType, cx, cy);
}

void CuDlgIpmSummaryLogInfo::InitClassMembers (BOOL bUndefined, LPCTSTR lpszNA)
{
	if (bUndefined)
	{
		m_strDatabaseAdds = lpszNA;
		m_strDatabaseRemoves = lpszNA;
		m_strTransactionBegins = lpszNA;
		m_strTransactionEnds = lpszNA;
		m_strLogWrites = lpszNA;
		m_strLogWriteIOs = lpszNA;
		m_strLogReadIOs = lpszNA;
		m_strLogForces = lpszNA;
		m_strLogWaits = lpszNA;
		m_strLogSplitWaits = lpszNA;
		m_strLogFreeWaits = lpszNA;
		m_strLogStallWaits = lpszNA;
		m_strCheckCommitTimer = lpszNA;
		m_strTimerWrite = lpszNA;
		m_strLogGroupCommit = lpszNA;
		m_strLogGroupCount = lpszNA;
		m_strInconsistentDatabase = lpszNA;
		m_strKBytesWritten = lpszNA;
		m_strLogCommitAverage = lpszNA;
		m_strLogCommitCurrent = lpszNA;
		m_strLogCommitMax = lpszNA;
		m_strLogCommitMin = lpszNA;
		m_strLogWriteAverage = lpszNA;
		m_strLogWriteCurrent = lpszNA;
		m_strLogWriteMax = lpszNA;
		m_strLogWriteMin = lpszNA;
	}
	else
	{
		LOGSUMMARYDATAMIN lg;
		memcpy (&lg, &(m_logStruct.logStruct0), sizeof (lg));
		m_strDatabaseAdds        .Format (_T("%ld"), lg.lgd_add);
		m_strDatabaseRemoves     .Format (_T("%ld"), lg.lgd_remove);
		m_strTransactionBegins   .Format (_T("%ld"), lg.lgd_begin);
		m_strTransactionEnds     .Format (_T("%ld"), lg.lgd_end);
		m_strLogWrites           .Format (_T("%ld"), lg.lgd_write);
		m_strLogWriteIOs         .Format (_T("%ld"), lg.lgd_writeio);
		m_strLogReadIOs          .Format (_T("%ld"), lg.lgd_readio);
		m_strLogForces           .Format (_T("%ld"), lg.lgd_force);
		m_strLogWaits            .Format (_T("%ld"), lg.lgd_wait);
		m_strLogSplitWaits       .Format (_T("%ld"), lg.lgd_split);
		m_strLogFreeWaits        .Format (_T("%ld"), lg.lgd_free_wait);
		m_strLogStallWaits       .Format (_T("%ld"), lg.lgd_stall_wait);
		m_strCheckCommitTimer    .Format (_T("%ld"), lg.lgd_pgyback_check);
		m_strTimerWrite          .Format (_T("%ld"), lg.lgd_pgyback_write);
		m_strLogGroupCommit      .Format (_T("%ld"), lg.lgd_group_force);
		m_strLogGroupCount       .Format (_T("%ld"), lg.lgd_group_count);
		m_strInconsistentDatabase.Format (_T("%ld"), lg.lgd_inconsist_db);
		m_strKBytesWritten       .Format (_T("%ld"), lg.lgd_kbytes);
		if (m_bStartup)
		{
			m_strLogCommitAverage = _T("");
			m_strLogCommitCurrent = _T("");
			m_strLogCommitMax     = _T("");
			m_strLogCommitMin     = _T("");
			m_strLogWriteAverage  = _T("");
			m_strLogWriteCurrent  = _T("");
			m_strLogWriteMax      = _T("");
			m_strLogWriteMin      = _T("");
		}
		else
		{
			m_strLogCommitAverage   .Format (_T("%ld"), lg.lgd_Commit_Average);
			m_strLogCommitCurrent   .Format (_T("%ld"), lg.lgd_Commit_Current);
			m_strLogCommitMax       .Format (_T("%ld"), lg.lgd_Commit_Max);
			if (lg.lgd_Commit_Min == -1)
				m_strLogCommitMin = _T("0");
			else
				m_strLogCommitMin.Format (_T("%ld"), lg.lgd_Commit_Min);
			m_strLogWriteAverage    .Format (_T("%ld"), lg.lgd_Write_Average);
			m_strLogWriteCurrent    .Format (_T("%ld"), lg.lgd_Write_Current);
			m_strLogWriteMax        .Format (_T("%ld"), lg.lgd_Write_Max);
			if (lg.lgd_Write_Min == -1)
				m_strLogWriteMin = _T("0");
			else
				m_strLogWriteMin.Format (_T("%ld"), lg.lgd_Write_Min);
		}
	}
}

LONG CuDlgIpmSummaryLogInfo::OnUpdateData (WPARAM wParam, LPARAM lParam)
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
		lstrcpy (m_logStruct.tchszStartTime, (LPCTSTR)m_strStartup);
		m_logStruct.nUse = m_bStartup? 0: 1;
		if (m_bParticularState)
		{
			AfxMessageBox (IDS_E_REFRESH_LOGGING, MB_ICONEXCLAMATION|MB_OK);
			m_bParticularState = FALSE;
			OnButtonStartup();
			return 0L;
		}
		CaIpmQueryInfo queryInfo(pDoc, -1);
		BOOL bOK = FALSE;

		if (m_bStartup)
			bOK = IPM_GetLogSummaryInfo(&queryInfo, &(m_logStruct.logStruct0), &(m_logStruct.logStruct1), TRUE,  FALSE);
		else
			bOK = IPM_GetLogSummaryInfo(&queryInfo, &(m_logStruct.logStruct0), &(m_logStruct.logStruct1), FALSE, FALSE);
		if (!bOK)
		{
			InitClassMembers (TRUE);
		}
		else
		{
			if (!m_bStartup)
				CalculateDelta();
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

LONG CuDlgIpmSummaryLogInfo::OnLoad (WPARAM wParam, LPARAM lParam)
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	LPLOGINFOSUMMARY pSummary = (LPLOGINFOSUMMARY)lParam;
	ASSERT (lstrcmp (pClass, _T("CuIpmPropertyDataSummaryLogInfo")) == 0);
	memcpy (&m_logStruct, pSummary, sizeof (m_logStruct));
	m_bParticularState = TRUE;
	try
	{
		ResetDisplay();
		m_bStartup = (m_logStruct.nUse == 0)? TRUE: FALSE;
		m_strStartup = m_logStruct.tchszStartTime;
		if (!m_bStartup)
			EnableDisableCtrlAverage(TRUE);
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

LONG CuDlgIpmSummaryLogInfo::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuIpmPropertyDataSummaryLogInfo* pData = NULL;
	try
	{
		_tcscpy (m_logStruct.tchszStartTime, m_strStartup);
		pData = new CuIpmPropertyDataSummaryLogInfo (&m_logStruct);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage ();
		e->Delete();
	}
	return (LRESULT)pData;
}

void CuDlgIpmSummaryLogInfo::OnButtonStartup() 
{
	int res = RES_ERR;
	m_bStartup = TRUE;
	m_strStartup = _T("Startup");
	EnableDisableCtrlAverage(FALSE);
	ResetVariable4average();
	UpdateData(FALSE);
	m_logStruct.nUse = m_bStartup? 0: 1;
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
		BOOL bOK = IPM_GetLogSummaryInfo(&queryInfo, &(m_logStruct.logStruct0), &(m_logStruct.logStruct1), TRUE, FALSE);
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

void CuDlgIpmSummaryLogInfo::EnableDisableCtrlAverage(BOOL bEnable)
{
	m_ctrlLogWrite.EnableWindow(bEnable);
	m_ctrlStaticLogSecond.EnableWindow(bEnable);
	m_ctrlStatLogWriteAverage.EnableWindow(bEnable);
	m_ctrlStatLogWriteCurrent.EnableWindow(bEnable);
	m_ctrlStatLogWriteMax.EnableWindow(bEnable);
	m_ctrlStatLogWriteMin.EnableWindow(bEnable);
	m_ctrlStatLogCommitAverage.EnableWindow(bEnable);
	m_ctrlStatLogCommitMax.EnableWindow(bEnable);
	m_ctrlStatLogCommitMin.EnableWindow(bEnable);
	m_ctrlStatLogCommitCurrent.EnableWindow(bEnable);
	m_ctrlLogWriteMax.EnableWindow(bEnable);
	m_ctrlLogWriteMin.EnableWindow(bEnable);
	m_ctrlLogWriteCurrent.EnableWindow(bEnable);
	m_ctrlLlogWriteAverage.EnableWindow(bEnable);
	m_ctrlLogCommitMin.EnableWindow(bEnable);
	m_ctrlLogCommitMax.EnableWindow(bEnable);
	m_ctrlLogCommitCurrent.EnableWindow(bEnable);
	m_ctrlLogCommitAverage.EnableWindow(bEnable);
	if (!bEnable)
	{
		m_strLogCommitAverage = _T("");
		m_strLogCommitCurrent = _T("");
		m_strLogCommitMax     = _T("");
		m_strLogCommitMin     = _T("");
		m_strLogWriteAverage  = _T("");
		m_strLogWriteCurrent  = _T("");
		m_strLogWriteMax      = _T("");
		m_strLogWriteMin      = _T("");
	}
}

void CuDlgIpmSummaryLogInfo::OnButtonNow() 
{
	int res = RES_ERR;
	CTime t = CTime::GetCurrentTime();
	m_strStartup = t.Format (_T("%c"));
	m_StartTimeNow    = t;
	m_PreviousTimeNow = t;

	EnableDisableCtrlAverage(TRUE);
	ResetVariable4average();

	m_bStartup = FALSE;
	m_logStruct.nUse = m_bStartup? 0: 1;
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
		BOOL bOK = IPM_GetLogSummaryInfo(&queryInfo, &(m_logStruct.logStruct0), &(m_logStruct.logStruct1), FALSE, TRUE);
		if (!bOK)
		{
			InitClassMembers (TRUE);
		}
		else
		{
			LPLOGSUMMARYDATAMIN lg = &(m_logStruct.logStruct0);
			m_PreviousEnd     = lg->lgd_Present_end;
			m_PreviousWriteio = lg->lgd_Present_writeio;
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

void CuDlgIpmSummaryLogInfo::ResetDisplay()
{
	InitClassMembers (TRUE, _T(""));
	UpdateData (FALSE);
}

void CuDlgIpmSummaryLogInfo::ResetVariable4average()
{
	LPLOGSUMMARYDATAMIN lg = &(m_logStruct.logStruct0);

	lg->lgd_Commit_Average = 0;
	lg->lgd_Commit_Current = 0;
	lg->lgd_Commit_Max     = 0;
	lg->lgd_Commit_Min     = -1;
	lg->lgd_Write_Average  = 0;
	lg->lgd_Write_Current  = 0;
	lg->lgd_Write_Max      = 0;
	lg->lgd_Write_Min      = -1;
}

void CuDlgIpmSummaryLogInfo::CalculateDelta()
{
	long lDelta_Lgd_End,lDelta_Lgd_Writeio;
	LPLOGSUMMARYDATAMIN presult;
	presult = &(m_logStruct.logStruct0);

	CTimeSpan tTotalTime, tDeltaTime;
	CTime tCurrentTime = CTime::GetCurrentTime();


	tTotalTime   =  tCurrentTime - m_StartTimeNow;
	if (tTotalTime.GetTotalSeconds() <= 0)
		tTotalTime = 1; 

	tDeltaTime = tCurrentTime - m_PreviousTimeNow;
	if (tDeltaTime.GetTotalSeconds() <= 0)
		tDeltaTime = 1;
	m_PreviousTimeNow = tCurrentTime;

	lDelta_Lgd_End = presult->lgd_Present_end - m_PreviousEnd;
	m_PreviousEnd  = presult->lgd_Present_end;

	lDelta_Lgd_Writeio = presult->lgd_Present_writeio - m_PreviousWriteio;
	m_PreviousWriteio = presult->lgd_Present_writeio;

	presult->lgd_Commit_Average = presult->lgd_end / tTotalTime.GetTotalSeconds();

	presult->lgd_Commit_Current = lDelta_Lgd_End / tDeltaTime.GetTotalSeconds();

	if (presult->lgd_Commit_Current > presult->lgd_Commit_Max)
		presult->lgd_Commit_Max = presult->lgd_Commit_Current;

	presult->lgd_Write_Average = presult->lgd_writeio / tTotalTime.GetTotalSeconds();

	presult->lgd_Write_Current = lDelta_Lgd_Writeio / tDeltaTime.GetTotalSeconds();

	if (presult->lgd_Write_Current > presult->lgd_Write_Max)
		presult->lgd_Write_Max = presult->lgd_Write_Current;

	if (presult->lgd_Commit_Min == -1)
		presult->lgd_Commit_Min = presult->lgd_Commit_Current;
	else
	{
		if (presult->lgd_Commit_Current < presult->lgd_Commit_Min)
			presult->lgd_Commit_Min = presult->lgd_Commit_Current;
	}

	if (presult->lgd_Write_Min == -1)
		presult->lgd_Write_Min = presult->lgd_Write_Current;
	else
	{
		if (presult->lgd_Write_Current < presult->lgd_Write_Min)
			presult->lgd_Write_Min = presult->lgd_Write_Current;
	}
}
