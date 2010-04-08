/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqleipar.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression (In Parameter page)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#if !defined(AFX_SQLEIPAR_H__25100EC8_FF90_11D1_A281_00609725DDAF__INCLUDED_)
#define AFX_SQLEIPAR_H__25100EC8_FF90_11D1_A281_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CuDlgPropertyPageSqlExpressionInParams : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlExpressionInParams)

public:
	CuDlgPropertyPageSqlExpressionInParams();
	~CuDlgPropertyPageSqlExpressionInParams();
	void UpdateDisplay (LPGENFUNCPARAMS lpFparam);

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlExpressionInParams)
	enum { IDD = IDD_EXPR_IN_PARAMETER };
	CListBox	m_cListBox1;
	CEdit	m_cParam3;
	CEdit	m_cParam2;
	CEdit	m_cParam1;
	CButton	m_cButtonExp3;
	CButton	m_cButtonExp2;
	CButton	m_cButtonExp1;
	CButton	m_cButtonRemove;
	CButton	m_cButtonReplace;
	CButton	m_cButtonAdd;
	CButton	m_cButtonInsert;
	CString	m_strParam1;
	CString	m_strParam2;
	CString	m_strParam3;
	CString	m_strFunctionName;
	CString	m_strHelp1;
	CString	m_strHelp2;
	CString	m_strHelp3;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlExpressionInParams)
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
	void EnableControls();
	void EnableButton4List();

	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlExpressionInParams)
	afx_msg void OnRadioIn();
	afx_msg void OnRadioNotIn();
	afx_msg void OnRadioSubQuery();
	afx_msg void OnRadioList();
	afx_msg void OnChangeEdit3();
	afx_msg void OnSelchangeList1();
	afx_msg void OnButtonInsert();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonReplace();
	afx_msg void OnButtonRemove();
	afx_msg void OnButton1Param();
	afx_msg void OnButton2Param();
	afx_msg void OnButton3Param();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLEIPAR_H__25100EC8_FF90_11D1_A281_00609725DDAF__INCLUDED_)
