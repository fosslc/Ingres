/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgilcsum.h, Header file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail (Summary) page of Lock Info
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
*/

#ifndef DGILCSUM_HEADER
#define DGILCSUM_HEADER

class CuDlgIpmSummaryLockInfo : public CFormView
{
protected:
	CuDlgIpmSummaryLockInfo();
	DECLARE_DYNCREATE(CuDlgIpmSummaryLockInfo)

	// Form Data
public:
	//{{AFX_DATA(CuDlgIpmSummaryLockInfo)
	enum { IDD = IDD_IPMDETAIL_LOCKINFO };
	CString    m_strStartup;
	CString    m_strLLCreated;
	CString    m_strLLReleased;
	CString    m_strLLInUse;
	CString    m_strLLRemaining;
	CString    m_strLLTotalAvailable;
	CString    m_strHTLock;
	CString    m_strHTResource;
	CString    m_strResourceInUse;
	CString    m_strLIRequested;
	CString    m_strLIReRequested;
	CString    m_strLIConverted;
	CString    m_strLIReleased;
	CString    m_strLICancelled;
	CString    m_strLIEscalated;
	CString    m_strLIInUse;
	CString    m_strLIRemaining;
	CString    m_strLITotalAvailable;
	CString    m_strLIDeadlockSearch;
	CString    m_strLIConvertDeadlock;
	CString    m_strLIDeadlock;
	CString    m_strLIConvertWait;
	CString    m_strLILockWait;
	CString    m_strLILockxTransaction;
	//}}AFX_DATA
	LOCKINFOSUMMARY m_locStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmSummaryLockInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual void OnInitialUpdate();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bParticularState;
	BOOL m_bStartup;
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmSummaryLockInfo();
	void ResetDisplay();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmSummaryLockInfo)
	afx_msg void OnButtonNow();
	afx_msg void OnButtonStartup();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif

