/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdgvnpwd.cpp, Implementation File
** 
**    Project  : Ingres II./ VDBA 
**    Author   : UK Sotheavut 
**    Purpose  : Popup Dialog Box for Creating an Installation Password 
**               The Advanced concept of Virtual Node.
** History:
** xx-Mar-1999 (uk$so01)
**    Modified on march 1999 to be used independently in the new application
**    for managing the virtual node
** 14-may-2001 (zhayu01) SIR 104724
**    Vnode becomes Server for EDBC
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 13-Oct-2003 (schph01)
**    SIR #109864, manage -vnode command line option 
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "xdgvnpwd.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNodeInstallationPassword dialog


CxDlgVirtualNodeInstallationPassword::CxDlgVirtualNodeInstallationPassword(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgVirtualNodeInstallationPassword::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgVirtualNodeInstallationPassword)
	m_strPassword = _T("");
	m_strConfirmPassword = _T("");
	//}}AFX_DATA_INIT
	m_bAlter = FALSE;
	m_strNodeName = _T("");
	m_strUserName = _T("");
}


void CxDlgVirtualNodeInstallationPassword::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgVirtualNodeInstallationPassword)
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strPassword);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strConfirmPassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgVirtualNodeInstallationPassword, CDialog)
	//{{AFX_MSG_MAP(CxDlgVirtualNodeInstallationPassword)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNodeInstallationPassword message handlers

void CxDlgVirtualNodeInstallationPassword::OnOK() 
{
	CWaitCursor doWaitCursor;
	HRESULT hErr = NOERROR;
	try
	{
		UpdateData (TRUE);
		if (m_strPassword.IsEmpty() || m_strPassword != m_strConfirmPassword)
		{
			CString strPasswordError;
			if (!strPasswordError.LoadString (IDS_E_PLEASE_RETYPE_PASSWORD))
				strPasswordError = _T("Password is not correct.\nPlease retype the password");
			AfxMessageBox (strPasswordError, MB_OK|MB_ICONEXCLAMATION);
			return;
		}

		//
		// Call the low level to add or change the definition of Virtual Node.
		CaNodeDataAccess nodeAccess;
		nodeAccess.InitNodeList();
		CString csLocalVnode;
		if ( theApp.m_pNodeAccess->IsVnodeOptionDefined() )
			INGRESII_QueryNetLocalVnode ( csLocalVnode );
		else
			csLocalVnode = GetLocalHostName();
		CaNode node(csLocalVnode);
		node.SetLogin(_T("*"));
		node.SetPassword(m_strPassword);
		node.ConnectionPrivate(FALSE);
		node.LoginPrivate(FALSE);
		node.SetNbConnectData (0);
		node.SetNbLoginData(0);

		if (!m_bAlter)
		{
			hErr = VNODE_AddNode (&node);
		}
		else
		{
			CaNode nodeOld(m_strNodeName);
			nodeOld.SetLogin(m_strUserName);
			nodeOld.ConnectionPrivate(FALSE);
			nodeOld.LoginPrivate(FALSE);
			nodeOld.SetNbConnectData (0);
			nodeOld.SetNbLoginData(0);

			hErr = VNODE_DropNode (&nodeOld);
			if (hErr == NOERROR)
				hErr = VNODE_AddNode(&node);
		}

		if (hErr != NOERROR)
		{
			CString strMessage;
			if (m_bAlter)
				// _T("Alter Installation Password failed.");
#ifdef	EDBC
				AfxFormatString1 (strMessage, IDS_ALTER_SERVER_FAILED, m_strNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ALTER_NODE_FAILED, m_strNodeName);
#endif
			else
				// _T("Define an Installation Password failed.");
#ifdef	EDBC
				AfxFormatString1 (strMessage, IDS_ADD_SERVER_FAILED, m_strNodeName);
#else
				AfxFormatString1 (strMessage, IDS_ADD_NODE_FAILED, m_strNodeName);
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
		AfxMessageBox(_T("Internal error: raise exception in CxDlgVirtualNodeInstallationPassword::OnOK()"));
	}
}

BOOL CxDlgVirtualNodeInstallationPassword::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (m_bAlter)
	{
		CString strTitle;
#ifdef	EDBC
		if (!strTitle.LoadString (IDS_ALTER_SERVER_TITLE))
#else
		if (!strTitle.LoadString (IDS_ALTER_NODE_TITLE))
#endif
			strTitle = _T("Alter an Installation Password");
		SetWindowText (strTitle);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgVirtualNodeInstallationPassword::OnDestroy() 
{
	CDialog::OnDestroy();
}

BOOL CxDlgVirtualNodeInstallationPassword::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nID = m_bAlter? HELPID_IDD_NODE_INSTALLATION_PASSWORD_ALTER: HELPID_IDD_NODE_INSTALLATION_PASSWORD_ADD;
	return APP_HelpInfo (nID);
}
