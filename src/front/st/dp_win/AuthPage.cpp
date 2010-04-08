/*
**
**  Name: AuthPage.cpp
**
**  Description:
**	This file contains the necessary routines to obtain the authorization
**	string and to verify it.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "AuthPage.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** AuthPage property page
*/
IMPLEMENT_DYNCREATE(AuthPage, CPropertyPage)

AuthPage::AuthPage() : CPropertyPage(AuthPage::IDD)
{
    //{{AFX_DATA_INIT(AuthPage)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

AuthPage::~AuthPage()
{
}

AuthPage::AuthPage(CPropertySheet *ps) : CPropertyPage(AuthPage::IDD)
{
    CSplash splash;
    splash.DoModal();

    m_propertysheet = ps;
    m_psp.dwFlags |= PSP_HASHELP;
}

void AuthPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(AuthPage)
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(AuthPage, CPropertyPage)
    //{{AFX_MSG_MAP(AuthPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** AuthPage message handlers
*/
BOOL
AuthPage::OnSetActive() 
{
    m_propertysheet->SetWizardButtons(PSWIZB_NEXT);	
    return CPropertyPage::OnSetActive();
}

BOOL
AuthPage::OnInitDialog() 
{
    CString Auth = getenv("DP_AUTH");

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_AUTH_HEADER);
    SetDlgItemText(IDC_AUTH_HEADER, Message);
    Message.LoadString(IDS_AUTH_TITLE);
    SetDlgItemText(IDC_AUTH_TITLE, Message);
    SetDlgItemText(IDC_EDIT_AUTH, Auth);
	
    return TRUE;
}

LRESULT
AuthPage::OnWizardNext() 
{
    STARTUPINFO		siStInfo;
    PROCESS_INFORMATION	piProcInfo;
    SECURITY_ATTRIBUTES	sa;
    DWORD		status, wait_status = 0;
    CString		Auth, ExecCmd;
    
    GetDlgItemText(IDC_EDIT_AUTH, Auth);

    if (Auth == "")
    {
	Message.LoadString(IDS_INVALID_AUTH);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	return -1;
    }

    SetEnvironmentVariable("DP_AUTH", Auth);

    /*
    ** Check the auth string.
    */
    ExecCmd = CString(getenv("II_SYSTEM")) +
#ifdef MAINWIN
	      CString("/ingres/sig/dp/showauth");
#else
	      CString("\\ingres\\sig\\dp\\showauth");
#endif
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    memset (&siStInfo, 0, sizeof (siStInfo)) ;

    siStInfo.cb = sizeof (siStInfo);
    siStInfo.lpReserved = NULL;
    siStInfo.lpReserved2 = NULL;
    siStInfo.cbReserved2 = 0;
    siStInfo.lpDesktop = NULL;
    siStInfo.dwFlags = STARTF_USESHOWWINDOW;
    siStInfo.wShowWindow = SW_HIDE;

    status = CreateProcess( NULL,
			    ExecCmd.GetBuffer(ExecCmd.GetLength()),
			    &sa, NULL, TRUE, 0, NULL, NULL, &siStInfo,
			    &piProcInfo);
    if (!status)
    {
	Message.LoadString(IDS_INVALID_AUTH2);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	return -1;
    }
    else
    {
	WaitForSingleObject(piProcInfo.hProcess, INFINITE);
	GetExitCodeProcess(piProcInfo.hProcess, &wait_status);
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);
	if (wait_status)
	{
	    Message.LoadString(IDS_INVALID_AUTH2);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return -1;
	}
    }

    return CPropertyPage::OnWizardNext();
}


