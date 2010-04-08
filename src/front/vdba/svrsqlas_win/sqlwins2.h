/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwins2.h, Header File
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Insert Page 2)
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

#if !defined(AFX_SQLWINS2_H__20074491_043A_11D2_A285_00609725DDAF__INCLUDED_)
#define AFX_SQLWINS2_H__20074491_043A_11D2_A285_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "edlsinsv.h"
#include "256bmp.h"


class CuDlgPropertyPageSqlWizardInsert2 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlWizardInsert2)

public:
	CuDlgPropertyPageSqlWizardInsert2();
	~CuDlgPropertyPageSqlWizardInsert2();

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlWizardInsert2)
	enum { IDD = IDD_SQLWIZARD_INSERT2 };
	C256bmp	m_bitmap;
	//}}AFX_DATA
	CuEditableListCtrlSqlWizardInsertValue m_cListCtrl;

	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlWizardInsert2)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL OnWizardFinish();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CImageList m_ImageList;
	CString    m_strAll;

	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlWizardInsert2)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLWINS2_H__20074491_043A_11D2_A285_00609725DDAF__INCLUDED_)
