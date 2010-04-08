/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqleepar.h, Header File 
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

#if !defined(AFX_SQLEEPAR_H__25100EC6_FF90_11D1_A281_00609725DDAF__INCLUDED_)
#define AFX_SQLEEPAR_H__25100EC6_FF90_11D1_A281_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CuDlgPropertyPageSqlExpressionAnyAllExistParams : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlExpressionAnyAllExistParams)

public:
	CuDlgPropertyPageSqlExpressionAnyAllExistParams();
	~CuDlgPropertyPageSqlExpressionAnyAllExistParams();
	void UpdateDisplay (LPGENFUNCPARAMS lpFparam);

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlExpressionAnyAllExistParams)
	enum { IDD = IDD_EXPR_ANYALLEXIST_PARAMETER };
	CEdit	m_cParam1;
	CString	m_strParam1;
	CString	m_strFunctionName;
	CString	m_strHelp1;
	CString	m_strHelp2;
	CString	m_strHelp3;
	CString	m_strParam1Name;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlExpressionAnyAllExistParams)
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
	//{{AFX_MSG(CuDlgPropertyPageSqlExpressionAnyAllExistParams)
	afx_msg void OnButton1Param();
	afx_msg void OnChangeEdit1();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLEEPAR_H__25100EC6_FF90_11D1_A281_00609725DDAF__INCLUDED_)
