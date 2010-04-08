/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.     	       //
//                                                                                     //
//    Source   : DgVNLogi.cpp, Implementation File                                     //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating the Virtual Node. (Login Data)          //
//               The Advanced Concept of Virtual Node.                                 //
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "dgvnlogi.h"

extern "C"
{
#include "msghandl.h"
#include "dba.h"
#include "dbaginfo.h"
#include "domdloca.h"
#include "domdata.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgVirtualNodeLogin::CxDlgVirtualNodeLogin(CWnd* pParent /*=NULL*/)
    : CDialog(CxDlgVirtualNodeLogin::IDD, pParent)
{
    m_bEnableCheck = TRUE;
    m_bAlter = FALSE;
    m_bClearPassword = TRUE;
    m_bDataAltered   = FALSE;
    //{{AFX_DATA_INIT(CxDlgVirtualNodeLogin)
    m_bPrivate = FALSE;
    m_strVNodeName = _T("");
    m_strUserName = _T("");
    m_strPassword = _T("");
    m_strConfirmPassword = _T("");
    //}}AFX_DATA_INIT
}


void CxDlgVirtualNodeLogin::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CxDlgVirtualNodeLogin)
    DDX_Control(pDX, IDC_MFC_EDIT4, m_cEditConfirmPassword);
    DDX_Control(pDX, IDC_MFC_EDIT3, m_cEditPassword);
    DDX_Control(pDX, IDOK, m_cOK);
    DDX_Check(pDX, IDC_MFC_CHECK1, m_bPrivate);
    DDX_Text(pDX, IDC_MFC_EDIT1, m_strVNodeName);
    DDV_MaxChars(pDX, m_strVNodeName, 32);
    DDX_Text(pDX, IDC_MFC_EDIT2, m_strUserName);
    DDV_MaxChars(pDX, m_strUserName, 24);
    DDX_Text(pDX, IDC_MFC_EDIT3, m_strPassword);
    DDV_MaxChars(pDX, m_strPassword, 16);
    DDX_Text(pDX, IDC_MFC_EDIT4, m_strConfirmPassword);
    DDV_MaxChars(pDX, m_strConfirmPassword, 16);
    //}}AFX_DATA_MAP
}

void CxDlgVirtualNodeLogin::ClearPassword()
{
    if (!m_bAlter)
        return;
    if (m_bClearPassword)
        return;
    m_bClearPassword = TRUE;   
    m_cEditPassword       .SetWindowText(_T("")); 
    m_cEditConfirmPassword.SetWindowText(_T(""));
}

void CxDlgVirtualNodeLogin::EnableOK()
{
    if (m_bAlter && m_bDataAltered)
    {
        UpdateData();
    }
    else
    if (!m_bAlter)
    {
        UpdateData();
    }
    else
    {
        m_cOK.EnableWindow (FALSE);
        return;
    }
    if (m_strVNodeName.GetLength() == 0         ||
        m_strUserName .GetLength() == 0         ||
        m_strPassword .GetLength() == 0         ||
        m_strConfirmPassword.GetLength() == 0)
    {
        m_cOK.EnableWindow (FALSE);
    }
    else
    {
        m_cOK.EnableWindow (TRUE);
    }
}

BEGIN_MESSAGE_MAP(CxDlgVirtualNodeLogin, CDialog)
    //{{AFX_MSG_MAP(CxDlgVirtualNodeLogin)
    ON_BN_CLICKED(IDC_MFC_CHECK1, OnCheckPrivate)
    ON_EN_CHANGE(IDC_MFC_EDIT2, OnChangeUserName)
    ON_EN_CHANGE(IDC_MFC_EDIT3, OnChangePassword)
    ON_EN_CHANGE(IDC_MFC_EDIT4, OnChangeConfirmPassword)
    ON_EN_SETFOCUS(IDC_MFC_EDIT3, OnSetfocusPassword)
    ON_EN_KILLFOCUS(IDC_MFC_EDIT3, OnKillfocusPassword)
    ON_EN_SETFOCUS(IDC_MFC_EDIT4, OnSetfocusConfirmPassword)
    ON_EN_KILLFOCUS(IDC_MFC_EDIT4, OnKillfocusConfirmPassword)
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNodeLogin message handlers

BOOL CxDlgVirtualNodeLogin::OnInitDialog() 
{
    CDialog::OnInitDialog();
    CButton* pButtonPrivate = (CButton*)GetDlgItem (IDC_MFC_CHECK1);
    pButtonPrivate->EnableWindow (m_bEnableCheck);
    m_cOK.EnableWindow (FALSE);    

    if (m_bAlter)
        PushHelpId(IDHELP_IDD_VNODELOGIN_ALTER);
    else
        PushHelpId(IDHELP_IDD_VNODELOGIN_ADD);

    if (m_bAlter)
    {
        CString strTitle;
        if (strTitle.LoadString (IDS_ALTER_NODELOGIN_TITLE) == 0)
            strTitle = _T("Alter the Definition of Virtual Node");
        CEdit* pEdit = (CEdit*)GetDlgItem (IDC_MFC_EDIT1);
        pEdit->SetReadOnly();
        pEdit->EnableWindow (FALSE);
        SetWindowText (strTitle);
           m_strOldVNodeName    = m_strVNodeName;
        m_strOldUserName        = m_strUserName;
        m_strOldPassword        = _T("");
        m_strOldConfirmPassword = _T("");
        m_bOldPrivate           = m_bPrivate;
        CEdit* pEdit2 = (CEdit*)GetDlgItem (IDC_MFC_EDIT2);
        pEdit2->SetFocus();
        pEdit2->SetSel (0, -1);
        OnKillfocusPassword();
        return FALSE;
    }
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgVirtualNodeLogin::OnCheckPrivate() 
{
    m_bDataAltered = TRUE;    
    ClearPassword();
    EnableOK();
}

void CxDlgVirtualNodeLogin::OnChangeUserName() 
{
    m_bDataAltered = TRUE;    
    ClearPassword();
    EnableOK();
}

void CxDlgVirtualNodeLogin::OnChangePassword() 
{
    m_bDataAltered = TRUE;    
    if (!m_bClearPassword)
    {
        m_bClearPassword = TRUE;   
        m_cEditConfirmPassword.SetWindowText(_T(""));    
    }    
    EnableOK();
}


void CxDlgVirtualNodeLogin::OnChangeConfirmPassword() 
{
    m_bDataAltered = TRUE;    
    if (!m_bClearPassword)
    {
        m_bClearPassword = TRUE;   
        m_cEditPassword.SetWindowText(_T(""));    
    }
    EnableOK();
}

void CxDlgVirtualNodeLogin::OnSetfocusPassword() 
{
    if (!m_bDataAltered)
    {
        m_strPassword = _T("");
        UpdateData (FALSE);
    }
}


void CxDlgVirtualNodeLogin::OnKillfocusPassword() 
{
    if (!m_bDataAltered)
    {
        m_strPassword = _T("**********");
        UpdateData (FALSE);
    }
}

void CxDlgVirtualNodeLogin::OnSetfocusConfirmPassword() 
{
    if (!m_bDataAltered)
    {
        m_strConfirmPassword = _T("");
        UpdateData (FALSE);
    }
}

void CxDlgVirtualNodeLogin::OnKillfocusConfirmPassword() 
{
    if (!m_bDataAltered)
    {
        m_strConfirmPassword = _T("**********");
        UpdateData (FALSE);
    }
}

void CxDlgVirtualNodeLogin::OnOK() 
{
    if (m_strPassword != m_strConfirmPassword)
    {
        CString strPasswordError;
        if (strPasswordError.LoadString (IDS_E_PLEASE_RETYPE_PASSWORD) == 0)
            strPasswordError = _T("Password is not correct.\nPlease retype the password");
        BfxMessageBox (strPasswordError, MB_OK|MB_ICONEXCLAMATION);
        return;
    }
    //
    // Call the low level to add or change the definition of Virtual Node's Login Password
    int result;
    NODELOGINDATAMIN s;
    CWinApp* pApp = AfxGetApp();
    pApp->DoWaitCursor (1);
    memset (&s, 0, sizeof (s));
    s.bPrivate = m_bPrivate;
    
    lstrcpy ((LPTSTR)s.NodeName,   (LPCTSTR)m_strVNodeName);
    lstrcpy ((LPTSTR)s.Login,      (LPCTSTR)m_strUserName);
    lstrcpy ((LPTSTR)s.Passwd,     (LPCTSTR)m_strPassword);
  
    if (!m_bAlter)
    {
        if (NodeLLInit() == RES_SUCCESS)
        {
            result = LLAddNodeLoginData (&s);
            UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
            NodeLLTerminate();
        }
        else
            result = RES_ERR;
    }
    else
    {
        NODELOGINDATAMIN so;
        
        memset (&so, 0, sizeof (so));
        so.bPrivate  = m_bOldPrivate;
        
        lstrcpy ((LPTSTR)so.NodeName,   (LPCTSTR)m_strOldVNodeName);
        lstrcpy ((LPTSTR)so.Login,      (LPCTSTR)m_strOldUserName);
        lstrcpy ((LPTSTR)so.Passwd,     (LPCTSTR)m_strOldPassword);
        if (NodeLLInit() == RES_SUCCESS)
        {
            result =  LLAlterNodeLoginData (&so, &s);
            UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
            NodeLLTerminate();
        }
        else
            result = RES_ERR;
    }
    pApp->DoWaitCursor (-1);
    if (result != RES_SUCCESS)
    {
        CString strMessage;
        if (m_bAlter)
            AfxFormatString1 (strMessage, IDS_ALTER_NODE_LOGIN_FAILED, m_strOldVNodeName);
        else
            AfxFormatString1 (strMessage, IDS_ADD_NODE_LOGIN_FAILED, m_strVNodeName);
        BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
        return;
    }
    CDialog::OnOK();
}


void CxDlgVirtualNodeLogin::OnDestroy() 
{
    CDialog::OnDestroy();
    PopHelpId();
}
