/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiphead.h, Header file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Page of Table control: The Header page..
**
** History:
**
** xx-Apr-1997 (uk$so01)
**    Created
*/

#ifndef DGIPHEAD_HEADER
#define DGIPHEAD_HEADER
#include "logindic.h"
#include "statleg2.h"

class CuDlgIpmPageHeader : public CFormView
{
private:
	BOOL m_bMustInitialize;

protected:
	CuDlgIpmPageHeader();
	DECLARE_DYNCREATE(CuDlgIpmPageHeader)

	// Form Data
public:
	//{{AFX_DATA(CuDlgIpmPageHeader)
	enum { IDD = IDD_IPMPAGE_HEADER };
	CLogFileIndicator m_LogDiagramIndicator;
	CString    m_strBlockCount;
	CString    m_strBlockSize;
	CString    m_strBufferCount;
	CString    m_strLogFullInterval;
	CString    m_strAbortInterval;
	CString    m_strCPInterval;
	CString    m_strBlockInUse;
	CString    m_strBlockAvailable;
	CString    m_strBegin;
	CString    m_strEnd;
	CString    m_strPreviousCP;
	CString    m_strExpectedNextCP;
	CString    m_strStatus;
	CString    m_strReserved;
	CString    m_strAvailable;
	//}}AFX_DATA
	LOGHEADERDATAMIN m_headStruct;
	CuStatisticBarLegendList2 m_cListLegend;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmPageHeader)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmPageHeader();
	void ResetDisplay();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmPageHeader)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

#endif
