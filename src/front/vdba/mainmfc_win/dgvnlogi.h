/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : DgVNLogi.h, Header File                                               //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating the Virtual Node. (Login data)          //
//               The Advanced concept of Virtual Node.                                 //
****************************************************************************************/
#ifndef DGVNLOGI_HEADER
#define DGVNLOGI_HEADER

class CxDlgVirtualNodeLogin : public CDialog
{
public:
    CxDlgVirtualNodeLogin(CWnd* pParent = NULL);   // standard constructor
    void SetAlter()
        {   
            m_bAlter = TRUE;
            m_bClearPassword = FALSE;
            m_strPassword        = _T("**********");
            m_strConfirmPassword = _T("**********");
        };

    BOOL     m_bEnableCheck;
    // Dialog Data
    //{{AFX_DATA(CxDlgVirtualNodeLogin)
    enum { IDD = IDD_XVNODELOGIN };
    CEdit    m_cEditConfirmPassword;
    CEdit    m_cEditPassword;
    CButton  m_cOK;
    BOOL     m_bPrivate;
    CString  m_strVNodeName;
    CString  m_strUserName;
    CString  m_strPassword;
    CString  m_strConfirmPassword;
    //}}AFX_DATA


    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CxDlgVirtualNodeLogin)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    // Implementation
protected:
    BOOL m_bAlter;
    BOOL m_bClearPassword;
    BOOL m_bDataAltered;
    // Generated message map functions
    //{{AFX_MSG(CxDlgVirtualNodeLogin)
    afx_msg void OnCheckPrivate();
    afx_msg void OnChangeUserName();
    afx_msg void OnChangePassword();
    afx_msg void OnChangeConfirmPassword();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnSetfocusPassword();
    afx_msg void OnKillfocusPassword();
    afx_msg void OnSetfocusConfirmPassword();
    afx_msg void OnKillfocusConfirmPassword();
	afx_msg void OnDestroy();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    CString  m_strOldVNodeName;
    CString  m_strOldUserName;
    CString  m_strOldPassword;
    CString  m_strOldConfirmPassword;
    BOOL     m_bOldPrivate;
    void ClearPassword();
    void EnableOK ();
};
#endif
