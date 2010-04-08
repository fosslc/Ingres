/*
**
**  Name: MainPage.cpp
**
**  Description:
**	This file contains all functions needed to display the Main wizard page.
**
**  History:
**	23-apr-1999 (somsa01)
**	    Created.
**	06-nov-2001 (somsa01)
**	    Updated splash to conform to new CA standard.
**	07-nov-2001 (somsa01)
**	    Changed the way we invoke the splash screen.
**	07-dec-2001 (somsa01)
**	    Win64 now has its libraries tagged with a "64" in the name.
**	22-jun-2004 (somsa01)
**	    DLLs now are prefixed with "II".
**	22-Jun-2004 (drivi01)
**	    Removed any references to licensing.
**	09-sep-2004 (somsa01)
**	    iiudtnt* became iilibudt*.
**	06-May-2008 (drivi01)
**	    Apply Wizard97 template to iilink.
*/

#include "stdafx.h"
#include "iilink.h"
#include "MainPage.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** MainPage property page
*/
IMPLEMENT_DYNCREATE(MainPage, CPropertyPage)

MainPage::MainPage() : CPropertyPage(MainPage::IDD)
{
    //{{AFX_DATA_INIT(MainPage)
	// NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

MainPage::MainPage(CPropertySheet *ps) : CPropertyPage(MainPage::IDD)
{
    m_propertysheet = ps;
    m_psp.dwFlags &= ~(PSP_HASHELP);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

MainPage::~MainPage()
{
}

void MainPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(MainPage)
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(MainPage, CPropertyPage)
    //{{AFX_MSG_MAP(MainPage)
    ON_BN_CLICKED(IDC_CHECK_BACKUP, OnCheckBackup)
	ON_BN_CLICKED(IDC_CHECK_RESTORE, OnCheckRestore)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** MainPage message handlers
*/
BOOL
MainPage::OnInitDialog() 
{
    CString	Message;

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_MAIN_DESC);
    SetDlgItemText(IDC_MAIN_DESC, Message);
    Message.LoadString(IDS_CHECK_BACKUP);
    SetDlgItemText(IDC_CHECK_BACKUP, Message);
    Message.LoadString(IDS_BACKUP_DESC);
    SetDlgItemText(IDC_BACKUP_DESC, Message);
    Message.LoadString(IDS_RESTORE_TXT);
    SetDlgItemText(IDC_RESTORE_TXT, Message);
    Message.LoadString(IDS_CHECK_RESTORE);
    SetDlgItemText(IDC_CHECK_RESTORE, Message);

    /*
    ** If the iilibudt.<backup extension> exists, increment the
    ** backup extension by 1 until it does not exist.
    */
    Message = CString(getenv("II_SYSTEM")) +
#ifdef WIN64
		  CString("\\ingres\\bin\\iilibudt64.") + BackupExt;
#else
		  CString("\\ingres\\bin\\iilibudt.") + BackupExt;
#endif  /* WIN64 */
    if (GetFileAttributes(Message) != -1)
    {
	int	index = 0;
	char	indexbuf[4];

	/*
	** Find a good backup extension.
	*/
	for (;;)
	{
	    index++;
	    BackupExt = CString("BAK") + CString(_itoa(index, indexbuf, 10));
	    Message = CString(getenv("II_SYSTEM")) +
#ifdef WIN64
		      CString("\\ingres\\bin\\iilibudt64.") + BackupExt;
#else
		      CString("\\ingres\\bin\\iilibudt.") + BackupExt;
#endif  /* WIN64 */
	    if (GetFileAttributes(Message) == -1)
		break;	/* We've found one! */
	}
    }
    SetDlgItemText(IDC_EDIT_EXTENSION, BackupExt);

    ::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_EXTENSION), FALSE);

    /*
    ** return TRUE unless you set the focus to a control
    ** EXCEPTION: OCX Property Pages should return FALSE
    */
    return TRUE;
}

BOOL
MainPage::OnSetActive() 
{
    CSplashWnd::ShowSplashScreen(this);
    SetForegroundWindow();

    m_propertysheet->SetWizardButtons(PSWIZB_NEXT);	
    return CPropertyPage::OnSetActive();
}

LRESULT
MainPage::OnWizardNext() 
{
    CString	Message, Message2;

    /*
    ** If we are backing up, make sure we have an extension.
    */
    if (Backup)
    {
	GetDlgItemText(IDC_EDIT_EXTENSION, BackupExt);
	if (BackupExt == "")
	{
	    Message.LoadString(IDS_NEED_EXTENSION);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_ICONINFORMATION | MB_OK);
	    return -1;
	}

	/*
	** Make sure a file called 'iilibudt.<extension> does not exist.
	*/
	Message = CString(getenv("II_SYSTEM")) +
#ifdef WIN64
		  CString("\\ingres\\bin\\iilibudt64.") + BackupExt;
#else
		  CString("\\ingres\\bin\\iilibudt.") + BackupExt;
#endif  /* WIN64 */
	if (GetFileAttributes(Message) != -1)
	{
	    int		status;
	    char	outmsg[1024];

	    Message.LoadString(IDS_BACKUP_EXISTS);
	    Message2.LoadString(IDS_TITLE);
	    sprintf(outmsg, Message, BackupExt, getenv("II_SYSTEM"));
	    status = MessageBox(outmsg, Message2, MB_YESNO | MB_ICONINFORMATION);
	    if (status == IDNO)
		return -1;
	}
    }

    return CPropertyPage::OnWizardNext();
}

void MainPage::OnCheckBackup() 
{
    CButton	*pCheckBackup = (CButton *)GetDlgItem(IDC_CHECK_BACKUP);

    Backup = pCheckBackup->GetCheck();
    if (Backup)
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_EXTENSION), TRUE);
    else
	::EnableWindow(::GetDlgItem(m_hWnd, IDC_EDIT_EXTENSION), FALSE);
}

void MainPage::OnCheckRestore() 
{
    CButton	*pCheckRestore = (CButton *)GetDlgItem(IDC_CHECK_RESTORE);

    if (pCheckRestore->GetCheck())
	m_propertysheet->SetWizardButtons(PSWIZB_FINISH);
    else
	m_propertysheet->SetWizardButtons(PSWIZB_NEXT);
}

BOOL MainPage::OnWizardFinish() 
{
    CString	BinDir, II_SYSTEM = getenv("II_SYSTEM");
    CString	Message, Message2;

    /*
    ** Change directory to the II_SYSTEM\ingres\bin directory.
    */
    BinDir = II_SYSTEM + CString("\\ingres\\bin");
    SetCurrentDirectory(BinDir);

    /*
    ** Move the distribution DLL into place if Ingres is not running.
    */
    if (!IngresUp)
    {
	if (Backup)
	{
#ifdef WIN64
	    CString	BackupDll = CString("iilibudt64.") + BackupExt;

	    if (!CopyFile("iilibudt64.DLL", BackupDll, FALSE))
#else
	    CString	BackupDll = CString("iilibudt.") + BackupExt;

	    if (!CopyFile("iilibudt.DLL", BackupDll, FALSE))
#endif  /* WIN64 */
	    {
		LPVOID lpMsgBuf;
		FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf, 0, NULL );
		Message2.LoadString(IDS_TITLE);
		Message.LoadString(IDS_FAIL_BACKUP);
		Message += CString((LPTSTR)lpMsgBuf);
		MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
		LocalFree(lpMsgBuf);
		return 0;
	    }
	}

#ifdef WIN64
	if (!CopyFile("iilibudt64.DIST", "iilibudt64.DLL", FALSE))
#else
	if (!CopyFile("iilibudt.DIST", "iilibudt.DLL", FALSE))
#endif  /* WIN64 */
	{
	    LPVOID lpMsgBuf;
	    FormatMessage(  FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			    FORMAT_MESSAGE_FROM_SYSTEM |
			    FORMAT_MESSAGE_IGNORE_INSERTS,
			    NULL, GetLastError(),
			    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			    (LPTSTR) &lpMsgBuf, 0, NULL );
	    Message2.LoadString(IDS_TITLE);
	    Message.LoadString(IDS_FAIL_RESTORE);
	    Message += CString((LPTSTR)lpMsgBuf);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    LocalFree(lpMsgBuf);
	    return 0;
	}

	Message.LoadString(IDS_SUCCESS_RESTORE);
	Message2.LoadString(IDS_TITLE);
	MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);	
    }
    else
    {
	char msgout[2048];

	Message.LoadString(IDS_FAIL_INGUP);
	sprintf(msgout, Message, getenv("II_SYSTEM"), getenv("II_SYSTEM"));
	Message2.LoadString(IDS_TITLE);
	MessageBox(msgout, Message2, MB_OK | MB_ICONEXCLAMATION);
    }
	
    return CPropertyPage::OnWizardFinish();
}
