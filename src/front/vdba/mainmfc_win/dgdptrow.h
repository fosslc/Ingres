/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdptrow.h, Header file    (Modeless Dialog)
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : It contains a list of rows resulting from the SELECT statement
**
** History:
**
** xx-Mar-1998 (uk$so01)
**    Created.
** 30-May-2001 (uk$so01)
**    SIR #104736, Integrate IIA and Cleanup old code of populate.
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 22-Sep-2004 (uk$so01)
**    BUG #113104 / ISSUE 13690527, Add the functionality
**    to allow vdba to request the IPM.OCX and SQLQUERY.OCX
**    to know if there is an SQL session through a given vnode
**    or through a given vnode and a database.
*/

#if !defined(AFX_DGDPTROW_H__760F3691_C22C_11D1_A26D_00609725DDAF__INCLUDED_)
#define AFX_DGDPTROW_H__760F3691_C22C_11D1_A26D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qsqlview.h"

class CuDlgDomPropTableRows : public CDialog
{
public:
	CuDlgDomPropTableRows(CWnd* pParent = NULL);   // standard constructor

	void OnOK (){return;}
	void OnCancel(){return;}
	void SetComponent (LPCTSTR lpszDatabase, LPCTSTR lpszStatement)
	{
		m_bSetComponent = TRUE;
		m_strDBName     = lpszDatabase;
		m_strStatement  = lpszStatement;
	}
	CuSqlQueryControl& GetSqlQueryControl() {return m_SqlQuery;}

	// Dialog Data
	//{{AFX_DATA(CuDlgDomPropTableRows)
	enum { IDD = IDD_DOMPROP_TABLE_ROWS };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropTableRows)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	CuSqlQueryControl m_SqlQuery;

protected:
	BOOL m_bExecuted;      // The statement has been executed (Rows has been fetched)

	CString m_strNode;
	CString m_strServer;
	CString m_strUser;
	CString m_strDBName;
	CString m_strStatement;
	CString m_strTable;
	CString m_strTableOwner;

	BOOL m_bSetComponent;
	void ResizeControls();

	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropTableRows)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData     (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData        (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad           (WPARAM wParam, LPARAM lParam);
	//
	// OnUpdateQuery only changes the hint to indicate the OnUpdateData that
	// the selection is occured in the right pane:
	afx_msg LONG OnUpdateQuery    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnChangeProperty (WPARAM wParam, LPARAM lParam);

	afx_msg LONG OnQueryType      (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnQueryOpenCursor(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnSettingChange  (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnCloseCursor    (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGDPTROW_H__760F3691_C22C_11D1_A26D_00609725DDAF__INCLUDED_)
