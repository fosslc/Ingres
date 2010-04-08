/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwsel3.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Select Page 3)
**
** History:
** xx-Jun-1998 (uk$so01)
**    Created.
** 01-Mar-2000 (schph01)
**    bug 97680 : management of objects names including special characters
**    in Sql Assistant.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
*/

#if !defined(AFX_SQLWSEL3_H__20074492_043A_11D2_A285_00609725DDAF__INCLUDED_)
#define AFX_SQLWSEL3_H__20074492_043A_11D2_A285_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "edlssele.h"
#include "256bmp.h"


class CuDlgPropertyPageSqlWizardSelect3 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlWizardSelect3)

public:
	CuDlgPropertyPageSqlWizardSelect3();
	~CuDlgPropertyPageSqlWizardSelect3();

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlWizardSelect3)
	enum { IDD = IDD_SQLWIZARD_SELECT3 };
	C256bmp	m_bitmap;
	CButton	m_cButtonODown;
	CButton	m_cButtonOUp;
	CButton	m_cButtonRemoveSortOrder;
	CButton	m_cButtonAddSortOrder;
	CButton	m_cButtonHaving;
	CButton	m_cButtonGDown;
	CButton	m_cButtonGUp;
	CStatic	m_cStaticHaving;
	CListBox	m_cListBoxOrderBy;
	CListBox	m_cListBoxGroupBy;
	CEdit	m_cEditHaving;
	CEdit	m_cEditUnion;
	CEdit	m_cEditWhere;
	CButton	m_cCheckGroupBy;
	CButton	m_cCheckDistinct;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlWizardSelect3)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	void CleanListBoxOrderBy();
	void CleanListBoxGroupBy();
	CImageList m_ImageList;
	CuEditableListCtrlSqlWizardSelectOrderColumn m_cListCtrl;

	void CleanColumnOrder();
	void EnableButtons();
	void OnExpressionAssistant(TCHAR tchszWH = _T('W')); // 'H': Having, 'W': Where
	void InitializeListBoxOrderColumn();
	void InitializeListBoxAllColumns(CListBox* pListBox);
	void InitializeListBoxGroupByColumn();

	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlWizardSelect3)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonWhere();
	afx_msg void OnButtonGroupUp();
	afx_msg void OnButtonGroupDown();
	afx_msg void OnButtonHaving();
	afx_msg void OnButtonUnion();
	afx_msg void OnButtonOrderAdd();
	afx_msg void OnButtonOrderRemove();
	afx_msg void OnButtonOrderUp();
	afx_msg void OnButtonOrderDown();
	afx_msg void OnCheckGroupBy();
	afx_msg void OnSelchangeListGroupBy();
	afx_msg void OnSelchangeListColumn();
	afx_msg void OnItemchangedListOrderColumn(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLWSEL3_H__20074492_043A_11D2_A285_00609725DDAF__INCLUDED_)
