/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgilgsum.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail (Summary) page of Log Info
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 06-Dec-2000 (schph01)
**    SIR 102822 Calculate and display the "Log Commits per second" and
**    "Log Write I/O per second" informations
*/

#ifndef DGILGSUM_HEADER
#define DGILGSUM_HEADER
#include "ipmicdta.h"

class CuDlgIpmSummaryLogInfo : public CFormView
{
protected:
	CuDlgIpmSummaryLogInfo();
	DECLARE_DYNCREATE(CuDlgIpmSummaryLogInfo)

public:
	// Form Data
	//{{AFX_DATA(CuDlgIpmSummaryLogInfo)
	enum { IDD = IDD_IPMDETAIL_LOGINFO };
	CButton	m_ctrlLogWrite;
	CButton	m_ctrlStaticLogSecond;
	CEdit	m_ctrlStartup;
	CEdit	m_ctrlLogWriteMax;
	CEdit	m_ctrlLogWriteMin;
	CEdit	m_ctrlLogWriteCurrent;
	CEdit	m_ctrlLlogWriteAverage;
	CStatic	m_ctrlStatLogWriteAverage;
	CStatic	m_ctrlStatLogWriteCurrent;
	CStatic	m_ctrlStatLogWriteMax;
	CStatic	m_ctrlStatLogWriteMin;
	CStatic	m_ctrlStatLogCommitAverage;
	CStatic	m_ctrlStatLogCommitMax;
	CStatic	m_ctrlStatLogCommitMin;
	CStatic	m_ctrlStatLogCommitCurrent;
	CEdit	m_ctrlLogCommitMin;
	CEdit	m_ctrlLogCommitMax;
	CEdit	m_ctrlLogCommitCurrent;
	CEdit	m_ctrlLogCommitAverage;
	CString    m_strStartup;
	CString    m_strDatabaseAdds;
	CString    m_strDatabaseRemoves;
	CString    m_strTransactionBegins;
	CString    m_strTransactionEnds;
	CString    m_strLogWrites;
	CString    m_strLogWriteIOs;
	CString    m_strLogReadIOs;
	CString    m_strLogForces;
	CString    m_strLogWaits;
	CString    m_strLogSplitWaits;
	CString    m_strLogFreeWaits;
	CString    m_strLogStallWaits;
	CString    m_strCheckCommitTimer;
	CString    m_strTimerWrite;
	CString    m_strLogGroupCommit;
	CString    m_strLogGroupCount;
	CString    m_strInconsistentDatabase;
	CString    m_strKBytesWritten;
	CString	m_strLogCommitAverage;
	CString	m_strLogCommitCurrent;
	CString	m_strLogCommitMax;
	CString	m_strLogCommitMin;
	CString	m_strLogWriteAverage;
	CString	m_strLogWriteCurrent;
	CString	m_strLogWriteMax;
	CString	m_strLogWriteMin;
	//}}AFX_DATA
	LOGINFOSUMMARY m_logStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmSummaryLogInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bParticularState;
	BOOL m_bStartup;

	CTime m_StartTimeNow;    // The starting time.
	CTime m_PreviousTimeNow; // Used to calculate length of time between screen refreshes
	long  m_PreviousEnd;     // Stores the previous # of end transactions (commits) from the last refresh.
	long  m_PreviousWriteio; // Stores the previous # of log write io's from the last refresh.

	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmSummaryLogInfo();
	void ResetDisplay();

	void EnableDisableCtrlAverage(BOOL bEnable);
	void ResetVariable4average();
	void CalculateDelta();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmSummaryLogInfo)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnButtonStartup();
	afx_msg void OnButtonNow();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
