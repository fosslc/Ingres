/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdlgacco.cpp, Implementation File                                     //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut, Detail implementation.                                  //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Setting the service account   .                  //
****************************************************************************************/
/* History:
* --------
* uk$so01: (10-Sept-1999 Changed)
*          Replace include files mainmfc.h by rcdepend.h for more reusable code
*          Replace the VDBA_MfcResourceString by using LoadString to be independent from 
*          VDBA.
*          Remap the resource id: IDC_MFC_EDIT1, IDC_MFC_EDIT2, IDC_MFC_EDIT3
*          Do not include the file msghandl.h for the IVM project.
*
*
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "xdlgacco.h"


#if !defined (IVMPROJ)
extern "C"
{
#include "msghandl.h"
}
#endif

const BOOL B_ACCOUNT_CURRENT_USER = TRUE;
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgAccountPassword dialog


CxDlgAccountPassword::CxDlgAccountPassword(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgAccountPassword::IDD, pParent)
{
	HANDLE     hProcess, hAccessToken;
	DWORD      dwInfoBufferSize, dwDomainSize =1024;
	TCHAR      tchszUser   [1024];
	TCHAR      tchszDomain [1024];
	DWORD      dwUserNameLen = sizeof(tchszUser);
	TCHAR      InfoBuffer[1000];
	PTOKEN_USER pTokenUser = (PTOKEN_USER)InfoBuffer;
	SID_NAME_USE snu;

	//
	// Get the domain name that this user is logged on to. This could
	// also be the local machine name.
	CString strAccount = _T("");
	hProcess = GetCurrentThread();
	if (!OpenThreadToken (hProcess,TOKEN_QUERY, TRUE, &hAccessToken))
	{
		if(GetLastError() == ERROR_NO_TOKEN) 
		{
			// attempt to open the process token, since no thread token
			// exists
			BOOL bOk = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hAccessToken);
		}
	}
	if (GetTokenInformation(hAccessToken, TokenUser, InfoBuffer, 1000, &dwInfoBufferSize))
	{
		tchszDomain[0] = _T('\0');
		if (LookupAccountSid(NULL, pTokenUser->User.Sid, tchszUser, &dwUserNameLen, tchszDomain, &dwDomainSize, &snu))
		{
			if (tchszDomain[0])
			{
				//
				// Use the account name of form "DomainName\UserName"
				if (B_ACCOUNT_CURRENT_USER)
					strAccount.Format (_T("%s\\%s"), tchszDomain, tchszUser);
				else
					strAccount.Format (_T("%s\\%s"), tchszDomain, _T("ingres"));
			}
			else
			{
				strAccount = B_ACCOUNT_CURRENT_USER? tchszUser: _T("ingres");
			}
		}
	}

	//{{AFX_DATA_INIT(CxDlgAccountPassword)
	m_strAccount = _T("");
	m_strPassword = _T("");
	m_strConfirmPassword = _T("");
	//}}AFX_DATA_INIT
	m_strAccount = strAccount;
}


void CxDlgAccountPassword::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgAccountPassword)
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strAccount);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strPassword);
	DDX_Text(pDX, IDC_MFC_EDIT3, m_strConfirmPassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgAccountPassword, CDialog)
	//{{AFX_MSG_MAP(CxDlgAccountPassword)
	ON_EN_CHANGE(IDC_MFC_EDIT1, OnChangeEdit1)
	ON_EN_CHANGE(IDC_MFC_EDIT2, OnChangeEdit2)
	ON_EN_CHANGE(IDC_MFC_EDIT3, OnChangeEdit3)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgAccountPassword message handlers

void CxDlgAccountPassword::OnOK() 
{
	UpdateData (TRUE);
	if (m_strConfirmPassword != m_strPassword)
	{
		CString strPasswordError;
		if (!strPasswordError.LoadString (IDS_E_PLEASE_RETYPE_PASSWORD))
			strPasswordError = _T("Password is not correct.\nPlease retype the password");
		AfxMessageBox (strPasswordError, MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	CDialog::OnOK();
}

void CxDlgAccountPassword::EnableButtons ()
{
	BOOL bOK[3] = {FALSE, FALSE, FALSE};
	if (GetDlgItem(IDC_MFC_EDIT1)->GetWindowTextLength() > 0)
		bOK [0] = TRUE;
	if (GetDlgItem(IDC_MFC_EDIT2)->GetWindowTextLength() > 0)
		bOK [1] = TRUE;
	if (GetDlgItem(IDC_MFC_EDIT3)->GetWindowTextLength() > 0)
		bOK [2] = TRUE;
	if (bOK [0] && bOK [1] && bOK [2])
		GetDlgItem (IDOK)->EnableWindow (TRUE);
	else
		GetDlgItem (IDOK)->EnableWindow (FALSE);
}

BOOL CxDlgAccountPassword::OnInitDialog() 
{
	CDialog::OnInitDialog();
	EnableButtons();
#if !defined (IVMPROJ)
	PushHelpId(IDHELP_IDD_XSERVICE_ACCOUNT);
#endif
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgAccountPassword::OnChangeEdit1() 
{
	EnableButtons();
}

void CxDlgAccountPassword::OnChangeEdit2() 
{
	EnableButtons();
}

void CxDlgAccountPassword::OnChangeEdit3() 
{
	EnableButtons();
}

void CxDlgAccountPassword::OnDestroy() 
{
	CDialog::OnDestroy();
#if !defined (IVMPROJ)
	PopHelpId();
#endif
}
