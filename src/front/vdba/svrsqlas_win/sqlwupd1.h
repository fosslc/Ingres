/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwupd1.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Update Page 1)
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

#if !defined(AFX_SQLWUPD1_H__98850B95_01CF_11D2_A283_00609725DDAF__INCLUDED_)
#define AFX_SQLWUPD1_H__98850B95_01CF_11D2_A283_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "uchklbox.h"
#include "256bmp.h"


class CuDlgPropertyPageSqlWizardUpdate1 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlWizardUpdate1)

public:
	CuDlgPropertyPageSqlWizardUpdate1();
	~CuDlgPropertyPageSqlWizardUpdate1();

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlWizardUpdate1)
	enum { IDD = IDD_SQLWIZARD_UPDATE1 };
	C256bmp	m_bitmap;
	CComboBox	m_cComboTable;
	//}}AFX_DATA
	CuCheckListBox m_cCheckListBoxColumn;

	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlWizardUpdate1)
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
	CString m_strAll;

	void EnableWizardButtons();
	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlWizardUpdate1)
	afx_msg void OnRadioTable();
	afx_msg void OnRadioView();
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboTable();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	afx_msg void OnCheckChange();
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLWUPD1_H__98850B95_01CF_11D2_A283_00609725DDAF__INCLUDED_)
