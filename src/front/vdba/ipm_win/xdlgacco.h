/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgacco.h, Header File 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Popup Dialog Box for Setting the service account
**
** History:
**
** xx-May-1997 (uk$so01)
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#if !defined(AFX_XDLGACCO_H__BCE90171_F3E8_11D1_A27D_00609725DDAF__INCLUDED_)
#define AFX_XDLGACCO_H__BCE90171_F3E8_11D1_A27D_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CxDlgAccountPassword : public CDialog
{
public:
	CxDlgAccountPassword(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgAccountPassword)
	enum { IDD = IDD_XSERVICE_ACCOUNT };
	CString	m_strAccount;
	CString	m_strPassword;
	CString	m_strConfirmPassword;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgAccountPassword)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	void EnableButtons ();
	// Generated message map functions
	//{{AFX_MSG(CxDlgAccountPassword)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEdit1();
	afx_msg void OnChangeEdit2();
	afx_msg void OnChangeEdit3();
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDLGACCO_H__BCE90171_F3E8_11D1_A27D_00609725DDAF__INCLUDED_)
