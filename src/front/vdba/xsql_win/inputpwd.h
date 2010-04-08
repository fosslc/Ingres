/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : inputpwd.h header file
**    Project  : SQL/Test (stand alone)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Request the role's password.
**
** History:
**
** 12-Aug-2003 (uk$so01)
**    Created
**    SIR #106648
*/

#if !defined(AFX_INPUTPWD_H__03F2FB4A_34E8_40E5_9D88_3E9F89490F6A__INCLUDED_)
#define AFX_INPUTPWD_H__03F2FB4A_34E8_40E5_9D88_3E9F89490F6A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
class CxDlgInputPassword : public CDialog
{
public:
	CxDlgInputPassword(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgInputPassword)
	enum { IDD = IDD_DIALOG_PASSWORD };
	CString	m_strPassword;
	//}}AFX_DATA
	CString m_strRole;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgInputPassword)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgInputPassword)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INPUTPWD_H__03F2FB4A_34E8_40E5_9D88_3E9F89490F6A__INCLUDED_)
