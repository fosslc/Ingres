/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlerror.h : header file.
**    Project  : INGRES II/ Monitoring.
**    Author   : Schalk Philippe (schph01)
**    Purpose  : CDlgSqlErrorHistory dialog
**
** History:
**
** 30-May-2002 (schph01)
**    Created
** 04-Jun-2004 (uk$so01)
**    SIR #111701, Connect Help to History of SQL Statement error.
** 10-Jun-2004 (schph01)
**    BUG #113003, change the implementation for gray/ungray the "Next",
**    "Prev" buttons.
*/

#if !defined(AFX_SQLERROR_H__F92F1CB3_6D7D_11D6_B6A5_00C04F1790C3__INCLUDED_)
#define AFX_SQLERROR_H__F92F1CB3_6D7D_11D6_B6A5_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "fileerr.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgSqlErrorHistory dialog

class CDlgSqlErrorHistory : public CDialog
{
// Construction
public:
	CDlgSqlErrorHistory(CWnd* pParent = NULL);   // standard constructor
	CaFileError* m_FilesErrorClass;

// Dialog Data
	//{{AFX_DATA(CDlgSqlErrorHistory)
	enum { IDD = IDD_SQLERR };
	CButton	m_ctrlOK;
	CButton	m_ctrlPrev;
	CButton	m_ctrlNext;
	CButton	m_ctrlLast;
	CButton	m_ctrlFirst;
	CString	m_csErrorStatement;
	CString	m_csErrorCode;
	CString	m_csErrorText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgSqlErrorHistory)
	public:
	virtual int DoModal();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgSqlErrorHistory)
	afx_msg void OnSqlerrFirst();
	afx_msg void OnSqlerrLast();
	afx_msg void OnSqlerrNext();
	afx_msg void OnSqlerrPrev();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	void RefreshDisplay(CaSqlErrorInfo* SqlErr);
	void SqlErrUpdButtonsStates();

	// List of CaSqlErrorInfo:
	CTypedPtrList<CPtrList, CaSqlErrorInfo*> m_listSqlError;

	POSITION m_posPrevErrorView;
	POSITION m_posDisplayErrorView;
	POSITION m_posNextErrorView;

	BOOL m_bBegingList; // used to enable or disable "First" "Next" "Prev" and "Last" Buttons
	BOOL m_bEndList;    // used to enable or disable "First" "Next" "Prev" and "Last" Buttons
	BOOL m_bMidleList;  // Used to enable or disable "Next" "Prev" buttons

public:
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
protected:
};


/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SQLERROR_H__F92F1CB3_6D7D_11D6_B6A5_00C04F1790C3__INCLUDED_)
