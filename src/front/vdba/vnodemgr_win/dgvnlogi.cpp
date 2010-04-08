/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : DgVNLogi.cpp, Implementation File
**    Project  : CA-OpenIngres/Monitoring. 
**    Author   : UK Sotheavut
** 
**    Purpose  : Popup Dialog Box for Creating the Virtual Node. (Login Data)
**               The Advanced Concept of Virtual Node.
** History
** 14-may-2001 (zhayu01) SIR 104724
**    Vnode becomes Server for EDBC
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgvnlogi.h"

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
	m_bDataAltered	 = FALSE;
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
	ON_WM_HELPINFO()
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
	{
		CString strTitle;
#ifdef	EDBC
		if (!strTitle.LoadString (IDS_ALTER_SERVERLOGIN_TITLE))
			strTitle = _T("Alter the Definition of Server");
#else
		if (!strTitle.LoadString (IDS_ALTER_NODELOGIN_TITLE))
			strTitle = _T("Alter the Definition of Virtual Node");
#endif
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
		if (!strPasswordError.LoadString (IDS_E_PLEASE_RETYPE_PASSWORD))
			strPasswordError = _T("Password is not correct.\nPlease retype the password");
		AfxMessageBox (strPasswordError, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	//
	// Call the low level to add or change the definition of Virtual Node's Login Password
	HRESULT hErr = NOERROR;
	try
	{
		CWaitCursor doWaitCursor;
		CaNodeDataAccess nodeAccess;
		nodeAccess.InitNodeList();
		CaNodeLogin login (m_strVNodeName, m_strUserName, m_strPassword, m_bPrivate);

		if (!m_bAlter)
		{
			hErr = VNODE_LoginAdd (&login);
		}
		else
		{
			CaNodeLogin loginOld (m_strOldVNodeName, m_strOldUserName, m_strOldPassword, m_bOldPrivate);
			hErr = VNODE_LoginAlter (&loginOld, &login);
		}
		if (hErr != NOERROR)
		{
			CString strMessage;
			if (m_bAlter)
#ifdef EDBC
				AfxFormatString1 (strMessage, IDS_ALTER_SERVER_LOGIN_FAILED, m_strOldVNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ALTER_NODE_LOGIN_FAILED, m_strOldVNodeName);
#endif
			else
#ifdef EDBC
				AfxFormatString1 (strMessage, IDS_ADD_SERVER_LOGIN_FAILED, m_strVNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ADD_NODE_LOGIN_FAILED, m_strVNodeName);
#endif
			AfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
			return;
		}
		CDialog::OnOK();
	}
	catch (CeNodeException e)
	{
		AfxMessageBox(e.GetReason());
	}
	catch (...)
	{
		AfxMessageBox(_T("Internal error: raise exception in CxDlgVirtualNodeLogin::OnOK()"));
	}
}


void CxDlgVirtualNodeLogin::OnDestroy() 
{
	CDialog::OnDestroy();
}

BOOL CxDlgVirtualNodeLogin::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nID = m_bAlter? IDHELP_IDD_VNODELOGIN_ALTER: IDHELP_IDD_VNODELOGIN_ADD;
	return APP_HelpInfo (nID);
}
