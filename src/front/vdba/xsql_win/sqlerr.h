/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlerr.h : header file.
**    Project  : INGRES II/ Sql Test.
**    Author   : Schalk Philippe (schph01)
**    Purpose  : CDlgSqlErrorHistory dialog
**
** History:
**
** 01-Sept-2004 (schph01)
**    Created
*/
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "afxwin.h"
#include "fileerr.h"


// CDlgSqlErrorHistory dialog

class CDlgSqlErrorHistory : public CDialog
{

public:
	CDlgSqlErrorHistory(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSqlErrorHistory();

	CaFileError* m_FilesErrorClass;


// Dialog Data
	enum { IDD = IDD_SQLERROR };
	CButton m_ctrlOK;
	CButton m_ctrlPrev;
	CButton m_ctrlNext;
	CButton	m_ctrlLast;
	CButton m_ctrlFirst;
	CString m_csErrorStatement;
	CString m_csErrorText;
	CString m_csErrorCode;
	virtual int DoModal();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

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
	BOOL m_bMidleList;  // used to enable or disable "Next" "Prev" Buttons
	afx_msg void OnSqlerrFirst();
	afx_msg void OnSqlerrLast();
	afx_msg void OnSqlerrNext();
	afx_msg void OnSqlerrPrev();
public:
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
};
