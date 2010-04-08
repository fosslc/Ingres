/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlemain.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generate the sql expression (main page: function choice)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
*/

#if !defined(AFX_SQLEMAIN_H__25100EC2_FF90_11D1_A281_00609725DDAF__INCLUDED_)
#define AFX_SQLEMAIN_H__25100EC2_FF90_11D1_A281_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CuDlgPropertyPageSqlExpressionMain : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlExpressionMain)

public:
	CuDlgPropertyPageSqlExpressionMain();
	~CuDlgPropertyPageSqlExpressionMain();

	void ConstructNextStep();
	DWORD GetComponentCategory();

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlExpressionMain)
	enum { IDD = IDD_EXPR_MAIN };
	CListBox	m_cListBox2;
	CListBox	m_cListBox1;
	CString	m_strFunctionName;
	CString	m_strHelp1;
	CString	m_strHelp2;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlExpressionMain)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlExpressionMain)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeList1();
	afx_msg void OnSelchangeList2();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLEMAIN_H__25100EC2_FF90_11D1_A281_00609725DDAF__INCLUDED_)
