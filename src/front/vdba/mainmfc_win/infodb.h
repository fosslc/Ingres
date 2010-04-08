/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : infodb.h, header file                                                 //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual DBA.                                               //
//    Author   : Schalk Philippe                                                       //
//                                                                                     //
//    Purpose  : Manage dialog box for launch infodb command.                          //
****************************************************************************************/
#if !defined(AFX_INFODB_H__89596D13_D4F4_11D5_B659_00C04F1790C3__INCLUDED_)
#define AFX_INFODB_H__89596D13_D4F4_11D5_B659_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CxDlgInfoDB dialog

class CxDlgInfoDB : public CDialog
{
// Construction
public:
	CString m_csVnodeName;
	CString m_csOwnerName;
	CString m_csDBname;
	CxDlgInfoDB(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CxDlgInfoDB)
	enum { IDD = IDD_INFODB };
	CButton	m_ctrlBtCheckList;
	CButton	m_ctrlBtLastCkp;
	CButton	m_ctrlBtOK;
	CEdit	m_ctrlCheckNumber;
	UINT	m_uiCheckPointNumber;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgInfoDB)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgInfoDB)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonCheckpoint();
	afx_msg void OnDestroy();
	afx_msg void OnRadioLastCkp();
	afx_msg void OnCheckSpecify();
	afx_msg void OnRadioSpecifyCkp();
	afx_msg void OnChangeEditNumber();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INFODB_H__89596D13_D4F4_11D5_B659_00C04F1790C3__INCLUDED_)
