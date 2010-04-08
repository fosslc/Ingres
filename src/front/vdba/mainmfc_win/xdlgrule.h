/****************************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : xdlgrule.h, Header file
**
**
**    Project  : Ingres II/VDBA
**    Author   : UK Sotheavut
**
**
**    Purpose  : Create Rule Dialog Box
**
**  History :
**  05-Mars-2001 (schph01)
**      BUG #103972
**      new message map functions : OnCheckInsert(),OnCheckDelete()
****************************************************************************************/
#if !defined(AFX_XDLGRULE_H__42CE3E23_722D_11D2_A2AA_00609725DDAF__INCLUDED_)
#define AFX_XDLGRULE_H__42CE3E23_722D_11D2_A2AA_00609725DDAF__INCLUDED_
#include "uchklbox.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgRule : public CDialog
{
public:
	CxDlgRule(CWnd* pParent = NULL);
	void SetParam (LPVOID pRuleparams){m_pRuleParam = pRuleparams;};
	// Dialog Data
	//{{AFX_DATA(CxDlgRule)
	enum { IDD = IDD_XRULE };
	CButton	m_cButtonOK;
	CEdit	m_cEditRuleName;
	CButton	m_cCheckSpecifiedColumn;
	CComboBox	m_cComboProcedure;
	CButton	m_cCheckDelete;
	CButton	m_cCheckUpdate;
	CButton	m_cCheckInsert;
	BOOL	m_bSpecifiedColumn;
	BOOL	m_bInsert;
	BOOL	m_bUpdate;
	BOOL	m_bDelete;
	int		m_nProcedure;
	CString	m_strRule;
	CString	m_strWhere;
	CString	m_strProcParam;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgRule)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	int m_nNodeHandle;
	LPVOID m_pRuleParam;
	CuCheckListBox m_cCheckListBox;
	BOOL m_bSpecifiedColumnChecked;

	BOOL InitializeComboProcedures();
	BOOL InitializeTableColumns();
	void EnableButtonOK();

	// Generated message map functions
	//{{AFX_MSG(CxDlgRule)
	virtual BOOL OnInitDialog();
	afx_msg void OnAssistant();
	afx_msg void OnCheckSpecifyColumns();
	virtual void OnOK();
	afx_msg void OnDestroy();
	afx_msg void OnCheckUpdate();
	afx_msg void OnChangeEditRuleName();
	afx_msg void OnSelchangeComboproc();
	afx_msg void OnCheckInsert();
	afx_msg void OnCheckDelete();
	//}}AFX_MSG
	afx_msg void OnCheckChangeColumn();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGRULE_H__42CE3E23_722D_11D2_A2AA_00609725DDAF__INCLUDED_)
