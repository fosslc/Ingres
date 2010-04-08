/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : Dglogipt.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : EDBC Client 1.1                                                       //
//    Author   : Yunlong Zhao                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for entering username and password (Login data)      //
//               for a Server (Virtual Node) and test the connection.                  //
****************************************************************************************/
#ifdef	EDBC

#ifndef DGLOGIPT_HEADER
#define DGLOGIPT_HEADER

class CxDlgVirtualNodeEnterLogin : public CDialog
{
public:
	CxDlgVirtualNodeEnterLogin(CWnd* pParent = NULL);   // standard constructor

	// Dialog Data
	//{{AFX_DATA(CxDlgVirtualNodeEnterLogin)
	enum { IDD = IDD_VNODELOGINPROMPT };
	CEdit    m_cEditPassword;
	CEdit    m_cUserName;
	CButton  m_cOK;
	CString  m_strVNodeName;
	CString  m_strUserName;
	CString  m_strPassword;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgVirtualNodeEnterLogin)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bDataAltered;
	// Generated message map functions
	//{{AFX_MSG(CxDlgVirtualNodeEnterLogin)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeUserName();
	afx_msg void OnChangePassword();
	afx_msg void OnSetfocusPassword();
	afx_msg void OnKillfocusPassword();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void EnableOK ();
};
#endif

#endif