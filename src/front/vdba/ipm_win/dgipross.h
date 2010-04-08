/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgipross.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: Detail page of Process
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
*/


#ifndef DGIPROSS_HEADER
#define DGIPROSS_HEADER

class CuDlgIpmDetailProcess : public CFormView
{
protected:
	CuDlgIpmDetailProcess();
	DECLARE_DYNCREATE(CuDlgIpmDetailProcess)
public:
	// Dialog Data
	//{{AFX_DATA(CuDlgIpmDetailProcess)
	enum { IDD = IDD_IPMDETAIL_PROCESS };
	CString    m_strOSPID;
	CString    m_strType;
	CString    m_strLogFileWriteRequest;
	CString    m_strWait;
	CString    m_strOpenDB;
	CString    m_strLogForce;
	CString    m_strStarted;
	CString    m_strEnded;
	//}}AFX_DATA
	LOGPROCESSDATAMAX m_pssStruct;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmDetailProcess)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmDetailProcess();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif 
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmDetailProcess)
	virtual void OnInitialUpdate();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
