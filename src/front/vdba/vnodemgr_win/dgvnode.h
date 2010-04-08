/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : DgVNode.h, Header File                                                //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut, Detail implementation.                                  //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating the Virtual Node.                       //
//               The convenient dialog box.                                            //
//    History  :
//		21-may-2001 (zhayu01) SIR 104751
//			Add m_bLoginSave, m_bOldLoginSave and OnCheckSave() for EDBC.												                       
** 11-Aug-2004 (uk$so01)
**    BUG #112804, ISSUE #13403470, If user alters the virtual node and changes
**    only the connection data information, then the password should not be 
**    needed to re-enter.
****************************************************************************************/
#ifndef DGVNODE_HEADER
#define DGVNODE_HEADER
class CxDlgVirtualNode : public CDialog
{
public:
	CxDlgVirtualNode(CWnd* pParent = NULL);   
	void SetAlter()
	{   
		m_bAlter = TRUE;
		m_bClearPassword = FALSE;
		m_strPassword        = _T("**********");
		m_strConfirmPassword = _T("**********");
	};

	// Dialog Data
	//{{AFX_DATA(CxDlgVirtualNode)
	enum { IDD = IDD_XVIRTUALNODE };
	CEdit   m_cEditUserName;
	CEdit   m_cEditListenAddess;
	CEdit   m_cEditRemoteAddress;
	CEdit      m_cEditConfirmPassword;
	CEdit      m_cEditPassword;
	CButton    m_cOK;
	CComboBox  m_cCombo1;
	CString    m_strVNodeName;
	CString    m_strUserName;
	CString    m_strPassword;
	CString    m_strConfirmPassword;
	CString    m_strRemoteAddress;
	CString    m_strListenAddress;
	BOOL       m_bPrivate;
	CString    m_strProtocol;
	//}}AFX_DATA
	CEdit m_cEditVNodeName;
	BOOL       m_bConnectionPrivate;
	BOOL       m_bLoginPrivate;
#ifdef	EDBC
	BOOL       m_bLoginSave;
#endif
	CButton    m_cCheckInstallationPassword;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgVirtualNode)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	void InitProtocol();


	BOOL m_bAlter;
	BOOL m_bClearPassword;
	BOOL m_bDataAltered;
	// Generated message map functions
	//{{AFX_MSG(CxDlgVirtualNode)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeProtocol();
	afx_msg void OnCheckPrivate();
	afx_msg void OnCheckConnectPrivate();
#ifdef	EDBC
	afx_msg void OnCheckSave();
#endif
	afx_msg void OnChangeVNodeName();
	afx_msg void OnChangeUserName();
	afx_msg void OnChangePassword();
	afx_msg void OnChangeConfirmPassword();
	afx_msg void OnChangeIPAddress();
	afx_msg void OnChangeListenPort();
	virtual void OnOK();
	afx_msg void OnSetfocusPassword();
	afx_msg void OnKillfocusPassword();
	afx_msg void OnSetfocusConfirmPassword();
	afx_msg void OnKillfocusConfirmPassword();
	afx_msg void OnDestroy();
	afx_msg void OnCheckInstallationPassword();
	afx_msg void OnSetfocusVNodeName();
	afx_msg void OnKillfocusVNodeName();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CString    m_strOldVNodeName;
	CString    m_strOldUserName;
	CString    m_strOldPassword;
	CString    m_strOldConfirmPassword;
	CString    m_strOldRemoteAddress;
	CString    m_strOldListenAddress;
	BOOL       m_bOldConnectionPrivate;
	BOOL       m_bOldLoginPrivate;
#ifdef	EDBC
	BOOL       m_bOldLoginSave;
#endif
	CString    m_strOldProtocol;
	BOOL       m_bVNode2RemoteAddess;

	void ClearPassword();
	void EnableOK ();
	BOOL IsDefaultValues(BOOL bCheckInstallation = FALSE);
};
#endif
