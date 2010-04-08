/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlpgtab.h : header file 
**    Project  : SqlTest ActiveX.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Property page for Tab Layout
**
** History:
**
** 06-Mar-2002 (uk$so01)
**    Created
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/


#if !defined(AFX_SQLPGTAB_H__FC40B059_2CEE_11D6_87A5_00C04F1F754A__INCLUDED_)
#define AFX_SQLPGTAB_H__FC40B059_2CEE_11D6_87A5_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSqlqueryPropPageTabLayout : public COlePropertyPage
{
	DECLARE_DYNCREATE(CSqlqueryPropPageTabLayout)
	DECLARE_OLECREATE_EX(CSqlqueryPropPageTabLayout)

public:
	CSqlqueryPropPageTabLayout();

	// Dialog Data
	//{{AFX_DATA(CSqlqueryPropPageTabLayout)
	enum { IDD = IDD_PROPPAGE_TABLAYOUT };
	CButton	m_cCheckNonSelect;
	CEdit	m_cEditMaxTraceSize;
	CEdit	m_cEditMaxTab;
	CSpinButtonCtrl	m_cSpinMaxTraceSize;
	CSpinButtonCtrl	m_cSpinMaxTab;
	CButton	m_cCheckTraceTabToTop;
	CButton	m_cCheckActivateTrace;
	BOOL	m_bTraceTabActivated;
	BOOL	m_bShowNonSelectTab;
	BOOL	m_bTraceTabToTop;
	long	m_nMaxTab;
	long	m_nMaxTraceSize;
	//}}AFX_DATA

	BOOL OnHelp(LPCTSTR lpszHelpDir);
	// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	// Message maps
protected:
	//{{AFX_MSG(CSqlqueryPropPageTabLayout)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckActivatetrace();
	afx_msg void OnKillfocusEditMaxtab();
	afx_msg void OnKillfocusEditMaxtracesize();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLPGTAB_H__FC40B059_2CEE_11D6_87A5_00C04F1F754A__INCLUDED_)
