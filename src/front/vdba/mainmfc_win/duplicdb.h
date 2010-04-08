/****************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  duplicdb.h
**
**  History after 01-May-2002:
**
**  14-May-2002 (uk$so01)
**   (bug 107773) don't refuse any more international characters (such as
**   accented characters) for the database name
**
*/
#if !defined(AFX_DUPLICDB_H__25BE3F02_6CB4_11D2_9734_00609725DBF9__INCLUDED_)
#define AFX_DUPLICDB_H__25BE3F02_6CB4_11D2_9734_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// duplicdb.h : header file
//
//UKS #include "editobjn.h"
#include "edlduslo.h"
/////////////////////////////////////////////////////////////////////////////
// CxDlgDuplicateDb dialog

class CxDlgDuplicateDb : public CDialog
{
// Construction
public:
	CxDlgDuplicateDb(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CxDlgDuplicateDb)
	enum { IDD = IDD_DUPLICATE_DATABASE };
	CButton	m_ctrlOK;
	CButton	m_ctrlAssignLoc;
	//}}AFX_DATA

	CEdit m_ctrlDbName;

	CuEditableListCtrlDuplicateDbSelectLocation m_cListCtrl;
	CImageList m_ImageList;
	CString m_csCurrentDatabaseName;
	int m_nNodeHandle;
	int m_DbType;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgDuplicateDb)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgDuplicateDb)
	virtual void OnOK();
	afx_msg void OnChangeNewDbName();
	afx_msg void OnAssignLoc();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	void EnableDisableOK();
	void FillLocation();
	void ExecuteRemoteCommand();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DUPLICDB_H__25BE3F02_6CB4_11D2_9734_00609725DBF9__INCLUDED_)
