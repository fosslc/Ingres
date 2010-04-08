/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : xdgvnpwd.cpp, Implementation File                                     //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II./ VDBA                                                      //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Popup Dialog Box for Creating an Installation Password                //
//               The Advanced concept of Virtual Node.                                 //
**
**
** 24-Jan-2002 (noifr01)
**  (bug 106906) bInstallPassword was not set (in the NODEDATAMIN structure) for
**  identifying the previous installation password "node" for cleaunup before
**  regenerating the new one
****************************************************************************************/
#include "stdafx.h"
#include "mainmfc.h"
#include "xdgvnpwd.h"
#include "starutil.h"
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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgVirtualNodeInstallationPassword message handlers

void CxDlgVirtualNodeInstallationPassword::OnOK() 
{
	CWaitCursor doWaitCursor;
	UpdateData (TRUE);
	if (m_strPassword.IsEmpty() || m_strPassword != m_strConfirmPassword)
	{
		CString strPasswordError;
		if (strPasswordError.LoadString (IDS_E_PLEASE_RETYPE_PASSWORD) == 0)
			strPasswordError = _T("Password is not correct.\nPlease retype the password");
		BfxMessageBox (strPasswordError, MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	//
	// Call the low level to add or change the definition of Virtual Node.
	int result;
	NODEDATAMIN s;
	memset (&s, 0, sizeof (s));
	s.ConnectDta.bPrivate = FALSE;
	s.LoginDta.bPrivate   = FALSE;
	s.isSimplifiedModeOK  = FALSE;  // always
	
	lstrcpy ((LPTSTR)s.NodeName,          (LPCTSTR)GetLocalHostName());
	lstrcpy ((LPTSTR)s.LoginDta.NodeName, (LPCTSTR)s.NodeName);
	lstrcpy ((LPTSTR)s.LoginDta.Login,    (LPCTSTR)_T("*"));
	lstrcpy ((LPTSTR)s.LoginDta.Passwd,   (LPCTSTR)m_strPassword);

	if (!m_bAlter)
	{
		if (NodeLLInit() == RES_SUCCESS)
		{
			// If defining local node with installation password,
			// check we have no connections through replicator monitoring in ipm
			result = LLAddFullNode (&s);
			UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
			NodeLLTerminate();
			UpdateNodeDisplay();

		}
		else
			result = RES_ERR;
	}
	else
	{
		NODEDATAMIN so;
		memset (&so, 0, sizeof (so));
		so.ConnectDta.bPrivate = FALSE;
		so.LoginDta.bPrivate   = FALSE;
		so.isSimplifiedModeOK  = FALSE;    // always
		lstrcpy ((LPTSTR)so.NodeName, m_strNodeName);
		lstrcpy ((LPTSTR)so.LoginDta.NodeName, (LPCTSTR)m_strNodeName);
		lstrcpy ((LPTSTR)so.LoginDta.Login,    m_strUserName);
		so.bInstallPassword = TRUE;
		if (NodeLLInit() == RES_SUCCESS)
		{
			result = LLDropFullNode (&so);
			if (result == RES_SUCCESS)
			{
				result =  LLAddFullNode (&s);
				UpdateDOMData(0,OT_NODE,0,NULL,TRUE,FALSE);
				NodeLLTerminate();
			}
			else result = RES_ERR;
		}
		else
			result = RES_ERR;
	}

	if (result != RES_SUCCESS)
	{
		CString strMessage;
		if (m_bAlter)
			//AfxFormatString1 (strMessage, IDS_ALTER_NODE_FAILED, m_strOldVNodeName);
			strMessage = VDBA_MfcResourceString(IDS_E_ALTER_INSTALL_PASSWD);//_T("Alter Installation Password failed.");
		else
			//AfxFormatString1 (strMessage, IDS_ADD_NODE_FAILED, m_strVNodeName);
			strMessage = VDBA_MfcResourceString(IDS_E_DEFINE_INSTALL_PASSWD);//_T("Define an Installation Password failed.");
		BfxMessageBox (strMessage, MB_OK|MB_ICONEXCLAMATION);
		return;
	}

	CDialog::OnOK();
}

BOOL CxDlgVirtualNodeInstallationPassword::OnInitDialog() 
{
	CDialog::OnInitDialog();
	if (m_bAlter)
	{
		CString strTitle;
		//if (strTitle.LoadString (IDS_ALTER_NODE_TITLE) == 0)
		strTitle.LoadString(IDS_T_INSTALL_PWD); // = _T("Alter an Installation Password");
		SetWindowText (strTitle);
	}
	PushHelpId(m_bAlter? HELPID_IDD_NODE_INSTALLATION_PASSWORD_ALTER: HELPID_IDD_NODE_INSTALLATION_PASSWORD_ADD);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgVirtualNodeInstallationPassword::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId();
}
