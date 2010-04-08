/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwsel1.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Select Page 1)
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

#if !defined(AFX_SQLWSEL1_H__98850B92_01CF_11D2_A283_00609725DDAF__INCLUDED_)
#define AFX_SQLWSEL1_H__98850B92_01CF_11D2_A283_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "calscbox.h"
#include "256bmp.h"

class CuDlgPropertyPageSqlWizardSelect1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlWizardSelect1)

public:
	CuDlgPropertyPageSqlWizardSelect1();
	~CuDlgPropertyPageSqlWizardSelect1();
	
	void EnableWizardButtons();

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlWizardSelect1)
	enum { IDD = IDD_SQLWIZARD_SELECT1 };
	C256bmp	m_bitmap;
	CButton	m_cCheckSystemObject;
	//}}AFX_DATA
	CuListCtrlCheckBox m_cListTable;
	CuListCtrlCheckBox m_cListView;

	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlWizardSelect1)
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
	int CountSelectedObject();
	int m_nObjectSelected;
	CImageList m_ImageListTable;
	CImageList m_ImageListView;
	CImageList m_StateImageList;

	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlWizardSelect1)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckSyetemObject();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg long OnListCtrlCheckChange (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLWSEL1_H__98850B92_01CF_11D2_A283_00609725DDAF__INCLUDED_)
