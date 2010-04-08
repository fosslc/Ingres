/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqleopar.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression (Database Object page)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 01-Mars-2000 (schph01)
**    bug 97680 : management of objects names including special characters
**    in Sql Assistant.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#if !defined(AFX_SQLEOPAR_H__25100EC4_FF90_11D1_A281_00609725DDAF__INCLUDED_)
#define AFX_SQLEOPAR_H__25100EC4_FF90_11D1_A281_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CuDlgPropertyPageSqlExpressionDBObjectParams : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlExpressionDBObjectParams)

public:
	CuDlgPropertyPageSqlExpressionDBObjectParams();
	~CuDlgPropertyPageSqlExpressionDBObjectParams();
	void UpdateDisplay (LPGENFUNCPARAMS lpFparam);
	BOOL FillListBoxObjects();

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlExpressionDBObjectParams)
	enum { IDD = IDD_EXPR_DBOBJECT };
	CListBox	m_cListBox1;
	CString	m_strFunctionName;
	CString	m_strHelp1;
	CString	m_strHelp2;
	CString	m_strHelp3;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlExpressionDBObjectParams)
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
	//{{AFX_MSG(CuDlgPropertyPageSqlExpressionDBObjectParams)
	afx_msg void OnSelchangeList1();
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLEOPAR_H__25100EC4_FF90_11D1_A281_00609725DDAF__INCLUDED_)
