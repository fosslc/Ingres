/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdgsysfo.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : System Info (Installation Password, OS LEvel Setting ...)
**
** History:
**
** 21-Sep-1999 (uk$so01)
**    created.
** 08-Mar-2000 (uk$so01)
**    SIR #100706. Provide the different context help id.
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 03-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**/

#include "stdafx.h"
#include "ivm.h"
#include "xdgsysfo.h"
#include "ivmdml.h"
#include "maindoc.h"
#include "vnodedml.h"
#include "wmusrmsg.h"
#include "ll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CxDlgSystemInfo::CxDlgSystemInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgSystemInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgSystemInfo)
	m_bInstallPassword = FALSE;
	//}}AFX_DATA_INIT
	try
	{
		CWaitCursor doWaitCursor;
		m_bInstallPassword = HasInstallationPassword(FALSE);
	}
	catch(...)
	{
		TRACE0 ("CxDlgSystemInfo::CxDlgSystemInfo: error accessing low level\n");
	}
}


void CxDlgSystemInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgSystemInfo)
	DDX_Control(pDX, IDC_BUTTON2, m_cButtonTest);
	DDX_Control(pDX, IDC_EDIT2, m_cEditPassword);
	DDX_Control(pDX, IDC_EDIT1, m_cEditName);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckInstallPassword);
	DDX_Check(pDX, IDC_CHECK1, m_bInstallPassword);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgSystemInfo, CDialog)
	//{{AFX_MSG_MAP(CxDlgSystemInfo)
	ON_BN_CLICKED(IDC_CHECK1, OnCheckInstallation)
	ON_EN_CHANGE(IDC_EDIT1, OnChangeEditName)
	ON_EN_CHANGE(IDC_EDIT2, OnChangeEditPassword)
	ON_BN_CLICKED(IDC_BUTTON2, OnButtonTest)
	ON_BN_CLICKED(IDC_BUTTON1, OnButtonExit)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgSystemInfo message handlers

void CxDlgSystemInfo::OnOK() 
{
	return;
	CDialog::OnOK();
}

void CxDlgSystemInfo::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

void CxDlgSystemInfo::OnCheckInstallation() 
{
	// TODO: Add your control notification handler code here
	
}


void CxDlgSystemInfo::OnChangeEditName() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	EnableButtons();
	
}

void CxDlgSystemInfo::OnChangeEditPassword() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function to send the EM_SETEVENTMASK message to the control
	// with the ENM_CHANGE flag ORed into the lParam mask.
	
	EnableButtons();
}

void CxDlgSystemInfo::OnButtonTest() 
{
	try
	{
		CString strMsg;
		BOOL bNetStarted = FALSE;
		CdMainDoc* pDoc = theApp.GetMainDocument();
		if (pDoc && IsWindow (pDoc->m_hwndLeftView))
			bNetStarted = (BOOL)::SendMessage (pDoc->m_hwndLeftView, WMUSRMSG_GET_STATE, (WPARAM)COMP_TYPE_NET, 0);
		if (!bNetStarted)
		{
			//_T("A Net Server process is required for this operation");
			strMsg.LoadString (IDS_MSG_IIGCC_MUSTBE_RUNNING);
			AfxMessageBox (strMsg, MB_ICONEXCLAMATION|MB_OK);
			return;
		}
		//
		// Call test external access:
		CString strName;
		CString strPassword;
		m_cEditName.GetWindowText (strName);
		m_cEditPassword.GetWindowText (strPassword);
		strName.TrimLeft();
		strName.TrimRight();
		if (strName.CompareNoCase (_T("*")) == 0 && strPassword.CompareNoCase (_T("*")) == 0)
		{
			// _T("The * used for user name and password\nhas a signification for the security !");
			strMsg.LoadString(IDS_MSG_STAR_SIGN_IN_ACCOUNT);
			AfxMessageBox (strMsg);
			return;
		}
		ExternalAccessTest (strName, strPassword);
	}
	catch(...)
	{
		TRACE0 ("Raise exception: CxDlgSystemInfo::OnButtonTest ...\n");
	}
}

void CxDlgSystemInfo::EnableButtons()
{
	BOOL bEnable = TRUE;
	CString strText;
	m_cEditName.GetWindowText (strText);
	strText.TrimLeft();
	strText.TrimRight();
	if (strText.IsEmpty())
		bEnable = FALSE;
	if (bEnable)
	{
		m_cEditPassword.GetWindowText (strText);
		if (strText.IsEmpty())
			bEnable = FALSE;
	}
	m_cButtonTest.EnableWindow (bEnable);
}

void CxDlgSystemInfo::ExternalAccessTest (CString& strName, CString& strPassword)
{
	CString strMsg;
	if (TestInstallationThroughNet((LPTSTR)(LPCTSTR)strName, (LPTSTR)(LPCTSTR)strPassword))
		//_T("The External Access Test is Successful");
		strMsg.LoadString (IDS_MSG_EXTERNAL_TEST_SUCCESS);
	else
		// "The Test has Failed.\n\n"
		// "If the user name and password are correct, "
		// "please refer to the \"Creating the Ingres System Administrator's Account\" section "
		// "in the \"Getting Started\" documentation"
		strMsg.LoadString (IDS_MSG_EXTERNAL_TEST_FAILED);

	AfxMessageBox (strMsg);
}

void CxDlgSystemInfo::OnButtonExit() 
{
	CDialog::OnOK();
}

BOOL CxDlgSystemInfo::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	UINT nHelpID = IDD;
	return APP_HelpInfo(nHelpID);
}
