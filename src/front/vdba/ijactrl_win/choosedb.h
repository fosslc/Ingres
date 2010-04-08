/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : choosedb.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Implementation of Dialog Box to allow to choose "Node" and "Database".
**
** History:
**
** 03-Apr-2000 (uk$so01)
**    created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
**
**/


#if !defined(AFX_CHOOSEDB_H__387AC803_0948_11D4_A353_00C04F1F754A__INCLUDED_)
#define AFX_CHOOSEDB_H__387AC803_0948_11D4_A353_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CxDlgChooseDatabase : public CDialog
{
public:
	CxDlgChooseDatabase(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgChooseDatabase)
	enum { IDD = IDD_XDATABASE };
	CButton	m_cButtonOK;
	CComboBox	m_cComboDatabase;
	CComboBox	m_cComboNode;
	//}}AFX_DATA


	CString m_strNode;
	CString m_strDatabase;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgChooseDatabase)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void FillComboDatabase(LPCTSTR lpszNode);
	void EnableButtons();

	// Generated message map functions
	//{{AFX_MSG(CxDlgChooseDatabase)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeComboNode();
	afx_msg void OnSelchangeComboDatabase();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CHOOSEDB_H__387AC803_0948_11D4_A353_00C04F1F754A__INCLUDED_)
