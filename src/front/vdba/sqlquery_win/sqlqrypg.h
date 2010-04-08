/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlqrypg.h : Header file.
**    Project  : sqlquery.ocx 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Declaration of the CSqlqueryPropPage property page class
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#if !defined(AFX_SQLQRYPG_H__634C384D_A069_11D5_8769_00C04F1F754A__INCLUDED_)
#define AFX_SQLQRYPG_H__634C384D_A069_11D5_8769_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CSqlqueryPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CSqlqueryPropPage)
	DECLARE_OLECREATE_EX(CSqlqueryPropPage)

	// Constructor
public:
	CSqlqueryPropPage();

	// Dialog Data
	//{{AFX_DATA(CSqlqueryPropPage)
	enum { IDD = IDD_PROPPAGE_SQL };
	CButton	m_cAutocommit;
	CEdit	m_cEditTimeOut;
	CSpinButtonCtrl	m_cSpinSelectLimit;
	CSpinButtonCtrl	m_cSpinTimeOut;
	CEdit	m_cEditSelectLimit;
	BOOL	m_bAutoCommit;
	BOOL	m_bReadLock;
	long	m_nTimeOut;
	long	m_nSelectLimit;
	//}}AFX_DATA
	int m_nSelectMode;

	BOOL OnHelp(LPCTSTR lpszHelpDir);
	// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void EnableAutocommit();

	// Message maps
protected:
	//{{AFX_MSG(CSqlqueryPropPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusEditTimeout();
	afx_msg void OnKillfocusEditMaxtuple();
	//}}AFX_MSG
	afx_msg void OnSelectMode();
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLQRYPG_H__634C384D_A069_11D5_8769_00C04F1F754A__INCLUDED)
