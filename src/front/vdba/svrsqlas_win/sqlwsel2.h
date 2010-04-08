/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwsel2.h, Header File 
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Select Page 2)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
*/

#if !defined(AFX_SQLWSEL2_H__98850B93_01CF_11D2_A283_00609725DDAF__INCLUDED_)
#define AFX_SQLWSEL2_H__98850B93_01CF_11D2_A283_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "256bmp.h"


class CaDBObject;
class CuDlgPropertyPageSqlWizardSelect2 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlWizardSelect2)

public:
	CuDlgPropertyPageSqlWizardSelect2();
	~CuDlgPropertyPageSqlWizardSelect2();

	BOOL IsSpecifyColumn(){return m_bSpecifyColumn;}
	BOOL ExistColumnExpression();

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlWizardSelect2)
	enum { IDD = IDD_SQLWIZARD_SELECT2 };
	C256bmp	m_bitmap;
	CStatic	m_cStaticExpression;
	CStatic	m_cStaticSpecifyColumn;
	CStatic	m_cStaticResultColumn;
	CButton	m_cButtonAdd;
	CButton	m_cButtonExpression;
	CEdit	m_cEditExpression;
	CButton	m_cButtonDel;
	CButton	m_cButtonDown;
	CButton	m_cButtonUp;
	CListBox	m_cListBoxResultColumn;
	CListBox	m_cListBoxColumn;
	CComboBox	m_cComboTable;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlWizardSelect2)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	void CleanComboTable();
	void CleanListResultColumns();
	void EnableButtons();
	void UpdateDisplayResultColumns (BOOL bMultipleTable);
	void EnableWizardButtons();
	BOOL MatchResultColumn (CaDBObject* pObj, LPCTSTR strColumn);

	CString m_strCurrentTable;
	BOOL m_bSpecifyColumn;

	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlWizardSelect2)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeComboTable();
	afx_msg void OnDblclkListColumn();
	afx_msg void OnSelchangeListResultColumn();
	afx_msg void OnRadioSpecifyColumns();
	afx_msg void OnRadioAllColumns();
	afx_msg void OnButtonExpression();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnButtonDelete();
	afx_msg void OnSelchangeListColumn();
	afx_msg void OnChangeEditExpression();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLWSEL2_H__98850B93_01CF_11D2_A283_00609725DDAF__INCLUDED_)
