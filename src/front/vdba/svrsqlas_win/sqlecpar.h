/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlecpar.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression (Comparaison page)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#if !defined(AFX_SQLECPAR_H__25100EC7_FF90_11D1_A281_00609725DDAF__INCLUDED_)
#define AFX_SQLECPAR_H__25100EC7_FF90_11D1_A281_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CuDlgPropertyPageSqlExpressionCompareParams : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlExpressionCompareParams)

public:
	CuDlgPropertyPageSqlExpressionCompareParams();
	~CuDlgPropertyPageSqlExpressionCompareParams();
	void UpdateDisplay (LPGENFUNCPARAMS lpFparam);

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlExpressionCompareParams)
	enum { IDD = IDD_EXPR_COMPARE };
	CEdit	m_cParam2;
	CEdit	m_cParam1;
	CString	m_strParam1;
	CString	m_strParam2;
	CString	m_strFunctionName;
	CString	m_strHelp1;
	CString	m_strHelp2;
	CString	m_strHelp3;
	CString	m_strParam1Name;
	CString	m_strOperator;
	CString	m_strParam2Name;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlExpressionCompareParams)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	virtual BOOL OnKillActive();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CString GetOperator();
	void EnableWizardButtons();

	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlExpressionCompareParams)
	afx_msg void OnButton1Param();
	afx_msg void OnButton2Param();
	afx_msg void OnChangeEdit1();
	afx_msg void OnChangeEdit2();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLECPAR_H__25100EC7_FF90_11D1_A281_00609725DDAF__INCLUDED_)
