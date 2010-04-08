/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : xdgsysfo.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : System Info (Installation Password, OS LEvel Setting ...)
**
** History:
**
** 21-Sep-1999 (uk$so01)
**    created.
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
**
**/

#if !defined(AFX_XDGSYSFO_H__298C69A2_6FF8_11D3_A30C_00609725DDAF__INCLUDED_)
#define AFX_XDGSYSFO_H__298C69A2_6FF8_11D3_A30C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CxDlgSystemInfo : public CDialog
{
public:
	CxDlgSystemInfo(CWnd* pParent = NULL);  

	// Dialog Data
	//{{AFX_DATA(CxDlgSystemInfo)
	enum { IDD = IDD_XSYSTEM_INFO };
	CButton	m_cButtonTest;
	CEdit	m_cEditPassword;
	CEdit	m_cEditName;
	CButton	m_cCheckInstallPassword;
	BOOL	m_bInstallPassword;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgSystemInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void EnableButtons();
	void ExternalAccessTest (CString& strName, CString& strPassword);

	// Generated message map functions
	//{{AFX_MSG(CxDlgSystemInfo)
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnCheckInstallation();
	afx_msg void OnChangeEditName();
	afx_msg void OnChangeEditPassword();
	afx_msg void OnButtonTest();
	afx_msg void OnButtonExit();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDGSYSFO_H__298C69A2_6FF8_11D3_A30C_00609725DDAF__INCLUDED_)
