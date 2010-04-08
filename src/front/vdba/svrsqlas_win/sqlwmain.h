/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwmain.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Select, Insert, Update, Delete)
**               Main page
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

#if !defined(AFX_SQLWMAIN_H__98850B91_01CF_11D2_A283_00609725DDAF__INCLUDED_)
#define AFX_SQLWMAIN_H__98850B91_01CF_11D2_A283_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "256bmp.h"


class CuDlgPropertyPageSqlWizardMain : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlWizardMain)

public:
	CuDlgPropertyPageSqlWizardMain();
	~CuDlgPropertyPageSqlWizardMain();

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlWizardMain)
	enum { IDD = IDD_SQLWIZARD_MAIN };
	C256bmp	m_bitmap;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlWizardMain)
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
	//{{AFX_MSG(CuDlgPropertyPageSqlWizardMain)
	afx_msg void OnRadio1Select();
	afx_msg void OnRadio2Insert();
	afx_msg void OnRadio3Update();
	afx_msg void OnRadio4Delete();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLWMAIN_H__98850B91_01CF_11D2_A283_00609725DDAF__INCLUDED_)
