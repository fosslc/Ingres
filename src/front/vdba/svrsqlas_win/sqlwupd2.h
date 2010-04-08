/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlwupd2.h, Header File 
**    Project  : Ingres II/Vdba
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Wizard for generating Sql Statements (Update Page 2)
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

#if !defined(AFX_SQLWUPS2_H__20074493_043A_11D2_A285_00609725DDAF__INCLUDED_)
#define AFX_SQLWUPS2_H__20074493_043A_11D2_A285_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "256bmp.h"
#include "calscbox.h"



class CuDlgPropertyPageSqlWizardUpdate2 : public CPropertyPage
{
	DECLARE_DYNCREATE(CuDlgPropertyPageSqlWizardUpdate2)

public:
	CuDlgPropertyPageSqlWizardUpdate2();
	~CuDlgPropertyPageSqlWizardUpdate2();

	// Dialog Data
	//{{AFX_DATA(CuDlgPropertyPageSqlWizardUpdate2)
	enum { IDD = IDD_SQLWIZARD_UPDATE2 };
	C256bmp	m_bitmap;
	CButton	m_cCheckSpecifyTable;
	CEdit	m_cEditCriteria;
	//}}AFX_DATA
	CuListCtrlCheckBox m_cListTable;
	CuListCtrlCheckBox m_cListView;
	CImageList m_ImageListTable;
	CImageList m_ImageListView;
	CImageList m_StateImageList;
	//
	// List of Columns from other Tables:
	CStringList m_listStrColumn;

	// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CuDlgPropertyPageSqlWizardUpdate2)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	virtual void OnCancel();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL GetListColumns (CaDBObject* pTable, CStringList& listColumn);

	// Generated message map functions
	//{{AFX_MSG(CuDlgPropertyPageSqlWizardUpdate2)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnButtonCriteria();
	afx_msg void OnSpecifyTables();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLWUPS2_H__20074493_043A_11D2_A285_00609725DDAF__INCLUDED_)
