/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlefpar.h, Header File 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut 
**    Purpose  : Wizard for generate the sql expression (Any-All-Exist Paramter page)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#if !defined(AFX_SQLEFPAR_H__25100EC3_FF90_11D1_A281_00609725DDAF__INCLUDED_)
#define AFX_SQLEFPAR_H__25100EC3_FF90_11D1_A281_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
class CuDlgPropertyPageSqlExpressionFunctionParams : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlExpressionFunctionParams)

public:
	CuDlgPropertyPageSqlExpressionFunctionParams();
	~CuDlgPropertyPageSqlExpressionFunctionParams();

	void UpdateDisplay (LPGENFUNCPARAMS lpFparam);

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlExpressionFunctionParams)
	enum { IDD = IDD_EXPR_FUNCTION_PARAMETER };
	CButton	m_cButton3;
	CButton	m_cButton2;
	CButton	m_cButton1;
	CEdit	m_cEdit3;
	CEdit	m_cEdit2;
	CEdit	m_cEdit1;
	CString	m_strParam1;
	CString	m_strParam2;
	CString	m_strParam3;
	CString	m_strFunctionName;
	CString	m_strHelp1;
	CString	m_strHelp2;
	CString	m_strHelp3;
	CString	m_strParam1Name;
	CString	m_strParam1Text;
	CString	m_strParam1Op;
	CString	m_strParam2Name;
	CString	m_strParam2Text;
	CString	m_strParam2Op;
	CString	m_strParam3Name;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlExpressionFunctionParams)
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
	void EnableWizardButtons();

	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlExpressionFunctionParams)
	afx_msg void OnButton1Param();
	afx_msg void OnButton2Param();
	afx_msg void OnButton3Param();
	afx_msg void OnChangeEdit1();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLEFPAR_H__25100EC3_FF90_11D1_A281_00609725DDAF__INCLUDED_)
