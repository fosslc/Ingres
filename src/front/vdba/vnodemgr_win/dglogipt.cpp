/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : DgLogiPt.cpp, Implementation File 
**    Project  : EDBC Client 1.1.
**    Author   : Yunlong Zhao 
**    Purpose  : Popup Dialog Box for entering username and password (Login data) 
**               for a Server (Virtual Node) and test the connection.
**
** History (after 17-Sep-2003):
** 
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 10-Mar-2010 (drivi01)
**    SIR 123397, update return code for INGRESII_NodeCheckConnection to be
**    STATUS instead of BOOL.  This will allow the function to return 
**    error code which can be used to retrieve error message via ERlookup
**    function.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dglogipt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef EDBC
extern "C"
{
// 
// Unpublished function defined in libingll/nodes.c
BOOL EDBC_ModifyLogin(LPCTSTR lpszVNodeName, LPCTSTR lpszUserName, LPCTSTR lpszPassword);
}


CxDlgVirtualNodeEnterLogin::CxDlgVirtualNodeEnterLogin(CWnd* pParent /*=NULL*/)
    : CDialog(CxDlgVirtualNodeEnterLogin::IDD, pParent)
{
	m_bDataAltered	 = FALSE;
	//{{AFX_DATA_INIT(CxDlgVirtualNodeEnterLogin)
	m_strVNodeName = _T("");
	m_strUserName = _T("");
	m_strPassword = _T("");
	//}}AFX_DATA_INIT
}


void CxDlgVirtualNodeEnterLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgVirtualNodeEnterLogin)
	DDX_Control(pDX, IDC_MFC_EDIT3, m_cEditPassword);
	DDX_Control(pDX, IDOK, m_cOK);
	DDX_Control(pDX, IDC_MFC_EDIT2, m_cUserName);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strVNodeName);
	DDV_MaxChars(pDX, m_strVNodeName, 32);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strUserName);
	DDV_MaxChars(pDX, m_strUserName, 24);
	DDX_Text(pDX, IDC_MFC_EDIT3, m_strPassword);
	DDV_MaxChars(pDX, m_strPassword, 16);
	//}}AFX_DATA_MAP
}


void CxDlgVirtualNodeEnterLogin::EnableOK()
{
	UpdateData();
	if (m_strVNodeName.GetLength() == 0         ||
		m_strUserName .GetLength() == 0         ||
		m_strPassword .GetLength() == 0 )
	{
		m_cOK.EnableWindow (FALSE);
	}
	else
	{
		m_cOK.EnableWindow (TRUE);
	}
}

BEGIN_MESSAGE_MAP(CxDlgVirtualNodeEnterLogin, CDialog)
	//{{AFX_MSG_MAP(CxDlgVirtualNodeEnterLogin)
	ON_EN_CHANGE(IDC_MFC_EDIT2, OnChangeUserName)
	ON_EN_CHANGE(IDC_MFC_EDIT3, OnChangePassword)
	ON_EN_SETFOCUS(IDC_MFC_EDIT3, OnSetfocusPassword)
	ON_EN_KILLFOCUS(IDC_MFC_EDIT3, OnKillfocusPassword)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNodeEnterLogin message handlers

BOOL CxDlgVirtualNodeEnterLogin::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_cOK.EnableWindow (FALSE);    

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgVirtualNodeEnterLogin::OnChangeUserName() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

void CxDlgVirtualNodeEnterLogin::OnChangePassword() 
{
	m_bDataAltered = TRUE;
	EnableOK();
}

void CxDlgVirtualNodeEnterLogin::OnSetfocusPassword() 
{
	if (!m_bDataAltered)
	{
		m_strPassword = _T("");
		UpdateData (FALSE);
	}
}


void CxDlgVirtualNodeEnterLogin::OnKillfocusPassword() 
{
	if (!m_bDataAltered)
	{
		m_strPassword = _T("**********");
		UpdateData (FALSE);
	}
}



void CxDlgVirtualNodeEnterLogin::OnOK() 
{
	CWaitCursor doWaitCursor;
	BOOL bOK = EDBC_ModifyLogin(m_strVNodeName, m_strUserName, m_strPassword);
	if (!bOK)
	{
		CString strMessage;
		AfxFormatString1 (strMessage, IDS_ADD_SERVER_LOGIN_FAILED, m_strVNodeName);
		AfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	else 
	{
		/*
		** Test the server connection with the username and password.
		*/
		STATUS bSuccess = INGRESII_NodeCheckConnection(m_strVNodeName);
		if (bSuccess)
		{
			CString strMsg;
			strMsg.Format(_T("Failed to connect to server %s."), (LPCTSTR)m_strVNodeName);
			AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
			m_strUserName = _T("");
			m_strPassword = _T("");
			EDBC_ModifyLogin(m_strVNodeName, m_strUserName, m_strPassword);
			UpdateData (FALSE);
			m_cUserName.SetFocus();
			m_cOK.EnableWindow (FALSE);
			return;
		}
	}

	CDialog::OnOK();
}


void CxDlgVirtualNodeEnterLogin::OnDestroy() 
{
	CDialog::OnDestroy();
}

#endif
