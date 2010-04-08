/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdgvnpwd.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II./ VDBA                                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating an Installation Password                //
//               The Advanced concept of Virtual Node.                                 //
****************************************************************************************/
#if !defined(AFX_XDGVNPWD_H__A4ADB043_594A_11D2_A2A5_00609725DDAF__INCLUDED_)
#define AFX_XDGVNPWD_H__A4ADB043_594A_11D2_A2A5_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CxDlgVirtualNodeInstallationPassword : public CDialog
{
public:
	CxDlgVirtualNodeInstallationPassword(CWnd* pParent = NULL);
	void SetAlter(){m_bAlter = TRUE;}

	// Dialog Data
	//{{AFX_DATA(CxDlgVirtualNodeInstallationPassword)
	enum { IDD = IDD_XVNODE_INSTALLATION_PASSWORD };
	CString	m_strPassword;
	CString	m_strConfirmPassword;
	//}}AFX_DATA
	CString m_strNodeName;
	CString m_strUserName;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgVirtualNodeInstallationPassword)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	BOOL m_bAlter;

	// Generated message map functions
	//{{AFX_MSG(CxDlgVirtualNodeInstallationPassword)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XDGVNPWD_H__A4ADB043_594A_11D2_A2A5_00609725DDAF__INCLUDED_)
