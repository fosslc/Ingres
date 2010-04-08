/*
**
**  Name: IntroPage.cpp
**
**  Description:
**	This file contains the routines necessary for the Intro Page Property
**	Page, which allows users to enter starting information.
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	15-oct-1999 (somsa01)
**	    Moved selection of new chart file name to the Final Property
**	    Page. Also, the Server selection page is now the final page.
**	21-oct-1999 (somsa01)
**	    Added confirmation message box for counter group removal.
**	29-oct-1999 (somsa01)
**	    When removing the Personal groups, remove the personal counter
**	    info as well.
**	28-jan-2000 (somsa01)
**	    Added Sleep() call while waiting for threads.
**	25-oct-2001 (somsa01)
**	    In OnWizardNext(), if we are supplied with an existing chart name
**	    which is not an absolute path, default the path to "My Documents".
**	06-nov-2001 (somsa01)
**	    Updated to follow the new splash format.
**	07-nov-2001 (somsa01)
**	    Changed the way we invoke the splash screen.
**	16-oct-2002 (somsa01)
**	    Changed RegDeleteKey() to RegDeleteValue().
**	23-Jun-2004 (drivi01)
**	    Removed Licensing.
**	07-May-2008 (drivi01)
**	    Apply Wizard97 template to this utility.
**	30-May-3008 (whiro01) Bug 119642, 115719; SD issue 122646
**	    Reference one common declaration of the main registry key (PERFCTRS_KEY),
**	    also using a common string to reference the directory location of our files.
*/

#include "stdafx.h"
#include "perfwiz.h"
#include "IntroPage.h"
#include "Splash.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static long ResetCounterGroups();

extern int  ReadInitFile();

/*
** CIntroPage property page
*/
IMPLEMENT_DYNCREATE(CIntroPage, CPropertyPage)

CIntroPage::CIntroPage() : CPropertyPage(CIntroPage::IDD)
{
    //{{AFX_DATA_INIT(CIntroPage)
	//}}AFX_DATA_INIT
}

CIntroPage::CIntroPage(CPropertySheet *ps) : CPropertyPage(CIntroPage::IDD)
{
    m_propertysheet = ps;
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CIntroPage::~CIntroPage()
{
}

void CIntroPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CIntroPage)
    DDX_Control(pDX, IDC_CHARTLOC_COMBO, m_ChartLoc);
    DDX_Control(pDX, IDC_IMAGE, m_image);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CIntroPage, CPropertyPage)
    //{{AFX_MSG_MAP(CIntroPage)
    ON_BN_CLICKED(IDC_CHECK_GROUP_MODIFY, OnCheckGroupModify)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	ON_BN_CLICKED(IDC_CHECK_PERSONAL, OnCheckPersonal)
	ON_BN_CLICKED(IDC_CHECK_REMOVE, OnCheckRemove)
	ON_BN_CLICKED(IDC_RADIO_CREATE, OnRadioCreate)
	ON_BN_CLICKED(IDC_RADIO_USE_EXISTING, OnRadioUseExisting)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CIntroPage message handlers
*/
BOOL
CIntroPage::OnSetActive() 
{
    CSplashWnd::ShowSplashScreen(this);
    SetForegroundWindow();

    m_propertysheet->SetWizardButtons(PSWIZB_NEXT);	
    return CPropertyPage::OnSetActive();
}

LRESULT
CIntroPage::OnWizardNext() 
{
    CString		HistFile, TempFile, Message, Message2;
    char		mline[2048];
    FILE		*outptr, *inptr;
    int			count;
    CButton		*pCheckPersonal = (CButton *)GetDlgItem(IDC_CHECK_GROUP_MODIFY);
    CButton		*pPersonal = (CButton *)GetDlgItem(IDC_CHECK_PERSONAL);
    LPITEMIDLIST	pidlPersonal = NULL;
    char		szPath[MAX_PATH];

    if (UseExisting)
    {
	GetDlgItemText(IDC_CHARTLOC_COMBO, ChartName);
	if (ChartName.Find('\\') == -1)
	{
	    /*
	    ** If we've just been given a file name, default it's location
	    ** to "My Documents".
	    */
	    SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidlPersonal);
	    if (pidlPersonal)
	    {
		SHGetPathFromIDList(pidlPersonal, szPath);
		ChartName = CString(szPath) + CString("\\") + ChartName;
		SetDlgItemText(IDC_CHARTLOC_COMBO, ChartName);
	    }
	}

	if (ChartName == "")
	{
	    Message.LoadString(IDS_NEED_CHARTFILE);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return -1;
	}

	if (GetFileAttributes(ChartName) == -1)
	{
	    Message.LoadString(IDS_CHARTFILE_NOEXIST);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return -1;
	}

	/*
	** Write out the history file. We'll only keep the last 15
	** unique recent files.
	*/
	HistFile = CString(getenv("II_SYSTEM")) +
		   CString("\\ingres\\files\\perfwiz.hst");
	TempFile = CString(getenv("II_SYSTEM")) +
		   CString("\\ingres\\files\\perfwiz.tmp");
	if ((outptr = fopen(TempFile, "w")) != NULL)
	{
	    fprintf(outptr, "%s\n", ChartName);
	    if ((inptr = fopen(HistFile, "r")) != NULL)
	    {
		count = 0;
		while ( (fgets(mline, sizeof(mline)-1, inptr) != NULL) &&
			(count < 15))
		{
		    mline[strlen(mline)-1] = '\0';
		    if (strcmp(mline, ChartName))
		    {
			fprintf(outptr, "%s\n", mline);
			count++;
		    }
		}
		fclose (inptr);
	    }
	    fclose (outptr);
	    CopyFile(TempFile, HistFile, FALSE);
	    DeleteFile(TempFile);
	}
    }

    if (pCheckPersonal->GetCheck())
    {
	PersonalGroup = pPersonal->GetCheck();
	if (PersonalGroup)
	{
	    if (!InitFileRead)
	    {
		CWaitDlg    wait;
		HANDLE	    hThread;
		DWORD	    ThreadID, status = 0;

		wait.m_WaitHeading.LoadString(IDS_READINIT_WAIT);
		MainWnd = NULL;
		hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)ReadInitFile,
					NULL, 0, &ThreadID );
		wait.DoModal();
		while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0)
			Sleep(200);
		GetExitCodeThread(hThread, &status);

		if (status)
		{
		    char    outmsg[2048];

		    Message.LoadString(IDS_READINIT_ERR);
		    sprintf(outmsg, Message, status);
		    Message2.LoadString(IDS_TITLE);
		    MessageBox(outmsg, Message2, MB_OK | MB_ICONEXCLAMATION);
		    return(-1);
		}
	    }

	    return IDD_PERSONAL_GROUP_PAGE;
	}
	else
	{
	    CWaitDlg	wait;
	    HANDLE	hThread;
	    DWORD	ThreadID, status = 0;

	    Message.LoadString(IDS_REMOVE_CONFIRM);
	    Message2.LoadString(IDS_TITLE);
	    status = MessageBox(Message, Message2, MB_YESNO | MB_ICONINFORMATION);
	    if (status != IDYES)
		return(-1);

	    wait.m_WaitHeading.LoadString(IDS_REMOVING_GROUPS);
	    MainWnd = NULL;
	    hThread = CreateThread( NULL, 0,
				    (LPTHREAD_START_ROUTINE)ResetCounterGroups,
				    NULL, 0, &ThreadID );
	    wait.DoModal();
	    while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0)
			Sleep(200);
	    GetExitCodeThread(hThread, &status);

	    if (status == ERROR_SUCCESS)
	    {
		Cleanup();
		InitFileRead = FALSE;
		NumCountersSelected = 0;

		Message.LoadString(IDS_FINISH_REMOVE);
		Message2.LoadString(IDS_TITLE);
		MessageBox(Message, Message2, MB_OK | MB_ICONINFORMATION);

		if (UseExisting)
		    return IDD_SERVER_PAGE;
		else
		    return IDD_FINAL_PAGE;
	    }
	    else
	    {
		char	outmsg[2048];

		Message.LoadString(IDS_ERROR_LODCTR);
		sprintf(outmsg, Message, status);
		Message2.LoadString(IDS_TITLE);
		MessageBox(outmsg, Message2, MB_OK | MB_ICONINFORMATION);
		return(-1);
	    }
	}
    }
    else
    {
	PersonalGroup = FALSE;
	if (UseExisting)
	    return IDD_SERVER_PAGE;
	else
	    return IDD_FINAL_PAGE;
    }
}

BOOL
CIntroPage::OnInitDialog() 
{
    CString Message, HistFile;
    FILE    *fptr;
    char    mline[2048];
    CButton *pCheckPersonal = (CButton *)GetDlgItem(IDC_CHECK_GROUP_MODIFY);
    CButton *pRadioPersonal = (CButton *)GetDlgItem(IDC_CHECK_PERSONAL);
    CButton *pRadioCreate   = (CButton *)GetDlgItem(IDC_RADIO_CREATE);
    CButton *pChartLoc	    = (CButton *)GetDlgItem(IDC_CHARTLOC_COMBO);
    CButton *pBrowse	    = (CButton *)GetDlgItem(IDC_BUTTON_BROWSE);

    CPropertyPage::OnInitDialog();

    Message.LoadString(IDS_INTRO_HEADING);
    SetDlgItemText(IDC_INTRO_HEADING, Message);
    Message.LoadString(IDS_INTRO_HEADING2);
    SetDlgItemText(IDC_INTRO_HEADING2, Message);
    Message.LoadString(IDS_GROUP_PERSONAL);
    SetDlgItemText(IDC_GROUP_PERSONAL, Message);
    Message.LoadString(IDS_CHECK_GROUP_MODIFY);
    SetDlgItemText(IDC_CHECK_GROUP_MODIFY, Message);
    Message.LoadString(IDS_CHECK_INTRO);
    SetDlgItemText(IDC_CHECK_INTRO, Message);
    Message.LoadString(IDS_CHECK_PERSONAL);
    SetDlgItemText(IDC_CHECK_PERSONAL, Message);
    Message.LoadString(IDS_CHECK_REMOVE);
    SetDlgItemText(IDC_CHECK_REMOVE, Message);
    Message.LoadString(IDS_BUTTON_BROWSE);
    SetDlgItemText(IDC_BUTTON_BROWSE, Message);
    SetDlgItemText(IDC_BUTTON_BROWSE2, Message);
    Message.LoadString(IDS_RADIO_CREATE);
    SetDlgItemText(IDC_RADIO_CREATE, Message);
    Message.LoadString(IDS_RADIO_USE_EXISTING);
    SetDlgItemText(IDC_RADIO_USE_EXISTING, Message);
    Message.LoadString(IDS_WIZARD_GROUP);
    SetDlgItemText(IDC_WIZARD_GROUP, Message);

    pCheckPersonal->SetCheck(TRUE);
    pRadioPersonal->SetCheck(TRUE);
    pRadioCreate->SetCheck(TRUE);
    pChartLoc->EnableWindow(FALSE);
    pBrowse->EnableWindow(FALSE);

    /*
    ** Fill up the destination combo box with the history file.
    */
    HistFile = CString(getenv("II_SYSTEM")) +
	       CString("\\ingres\\files\\perfwiz.hst");
    if ((fptr = fopen(HistFile, "r")) != NULL)
    {
	while ((fgets(mline, sizeof(mline)-1, fptr) != NULL))
	{
	    mline[strlen(mline)-1] = '\0';
	    m_ChartLoc.AddString((LPTSTR)&mline);
	}
	fclose (fptr);
    }

    return TRUE;
}

static long
ResetCounterGroups()
{
    CString		FilesLoc = CString(getenv("II_SYSTEM")) +
				   CString("\\ingres\\files\\");
    CString		InitHeader = FilesLoc + CString("oipfpers.h");
    CString		MarkFile = FilesLoc + CString("oipfctrs.ini");
    CString		InitFile = FilesLoc + CString("oipfpers.ini");
    CString		CmdLine = CString("lodctr ") + MarkFile;
    STARTUPINFO		siStInfo;
    PROCESS_INFORMATION	piProcInfo;
    DWORD		status = 0;
    struct grouplist	*gptr;
    struct PersCtrList	*PCPtr;
    HKEY		hKeyDriverPerf;

    /*
    ** Delete the personal init files.
    */
    DeleteFile(InitHeader);
    DeleteFile(InitFile);

    /*
    ** Delete any personal groups that have been added during this wizard.
    */
    while (GroupList)
    {
	/*
	** Free up its members.
	*/
	gptr = GroupList->next;
	FreePersonalCounters(GroupList);
	if (GroupList->GroupDescription)
	    free(GroupList->GroupDescription);
	if (GroupList->GroupName)
	    free(GroupList->GroupName);
	if (GroupList->PersCtrs)
	{
	    while (GroupList->PersCtrs)
	    {
		PCPtr = GroupList->PersCtrs->next;

		free(GroupList->PersCtrs->dbname);
		free(GroupList->PersCtrs->qry);
		free(GroupList->PersCtrs->PersCtr.CounterName);
		free(GroupList->PersCtrs->PersCtr.PCounterName);
		free(GroupList->PersCtrs->PersCtr.CounterHelp);
		free(GroupList->PersCtrs->PersCtr.UserName);
		free(GroupList->PersCtrs->PersCtr.UserHelp);
		if (GroupList->PersCtrs->PersCtr.DefineName)
		    free(GroupList->PersCtrs->PersCtr.DefineName);

		free(GroupList->PersCtrs);
		GroupList->PersCtrs = PCPtr;
	    }
	}

	free(GroupList);
	GroupList = gptr;
    }

    /*
    ** Remove the personal counter registry entry.
    */
    if (RegOpenKeyEx (
	    HKEY_LOCAL_MACHINE,
	    PERFCTRS_KEY,
	    0L,
	    KEY_ALL_ACCESS,
	    &hKeyDriverPerf) != ERROR_SUCCESS)
	return(1);

    RegDeleteValue(hKeyDriverPerf, "Personal Counters");
    DWORD junk = GetLastError();
    RegCloseKey (hKeyDriverPerf);

    /*
    ** Now, add the original Ingres counter groups back to the registry.
    */
    memset (&siStInfo, 0, sizeof (siStInfo)) ;

    siStInfo.cb          = sizeof (siStInfo);
    siStInfo.lpReserved  = NULL;
    siStInfo.lpReserved2 = NULL;
    siStInfo.cbReserved2 = 0;
    siStInfo.lpDesktop   = NULL;
    siStInfo.dwFlags     = STARTF_USESHOWWINDOW;
    siStInfo.wShowWindow = SW_HIDE;

    CreateProcess(NULL, "unlodctr oipfctrs", NULL, NULL, TRUE, 0,
		  NULL, NULL, &siStInfo, &piProcInfo);
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    GetExitCodeProcess(piProcInfo.hProcess, &status);

    if (status != ERROR_SUCCESS)
    {
	while (!MainWnd);
	::SendMessage(MainWnd, WM_CLOSE, 0, 0);
	return(status);
    }

    CreateProcess(NULL, CmdLine.GetBuffer(0), NULL, NULL, TRUE, 0,
		  NULL, NULL, &siStInfo, &piProcInfo);
    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
    GetExitCodeProcess(piProcInfo.hProcess, &status);

    Sleep(2000);
    while (!MainWnd);
    ::SendMessage(MainWnd, WM_CLOSE, 0, 0);
    return(status);
}

void
CIntroPage::OnCheckGroupModify() 
{
    CButton *pCheckPersonal = (CButton *)GetDlgItem(IDC_CHECK_GROUP_MODIFY);
    CButton *pRadioPersonal = (CButton *)GetDlgItem(IDC_CHECK_PERSONAL);
    CButton *pRadioRemove = (CButton *)GetDlgItem(IDC_CHECK_REMOVE);

    if (pCheckPersonal->GetCheck())
    {
	pRadioPersonal->EnableWindow(TRUE);
	pRadioRemove->EnableWindow(TRUE);
    }
    else
    {
	pRadioPersonal->EnableWindow(FALSE);
	pRadioRemove->EnableWindow(FALSE);
    }
}

void
CIntroPage::OnButtonBrowse() 
{
    CHAR	Filter[] =
	"Performance Monitor Chart File (*.pmc)|*.pmc|All Files (*.*)|*.*||";

    GetDlgItemText(IDC_CHARTLOC_COMBO, ChartName);

    CFileDialog	fDlg(TRUE, ".pmc", ChartName,
		     OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_FILEMUSTEXIST,
		     Filter, this);
    fDlg.m_ofn.lpstrTitle = "Ingres Performance Monitor Wizard";

    if (fDlg.DoModal() == IDOK)
    {
	ChartName = fDlg.GetPathName();
	SetDlgItemText(IDC_CHARTLOC_COMBO, ChartName);	
    }
}

void CIntroPage::OnCheckPersonal() 
{
    CButton *pRadioRemove = (CButton *)GetDlgItem(IDC_CHECK_REMOVE);

    pRadioRemove->SetCheck(FALSE);
}

void CIntroPage::OnCheckRemove() 
{
    CButton *pRadioPersonal = (CButton *)GetDlgItem(IDC_CHECK_PERSONAL);

    pRadioPersonal->SetCheck(FALSE);
}

void CIntroPage::OnRadioCreate() 
{
    CButton *pRadioExisting = (CButton *)GetDlgItem(IDC_RADIO_USE_EXISTING);
    CButton *pChartLoc	    = (CButton *)GetDlgItem(IDC_CHARTLOC_COMBO);
    CButton *pBrowse	    = (CButton *)GetDlgItem(IDC_BUTTON_BROWSE);

    pRadioExisting->SetCheck(FALSE);
    pChartLoc->EnableWindow(FALSE);
    pBrowse->EnableWindow(FALSE);

    UseExisting = FALSE;
}

void CIntroPage::OnRadioUseExisting() 
{
    CButton *pRadioCreate = (CButton *)GetDlgItem(IDC_RADIO_CREATE);
    CButton *pChartLoc	    = (CButton *)GetDlgItem(IDC_CHARTLOC_COMBO);
    CButton *pBrowse	    = (CButton *)GetDlgItem(IDC_BUTTON_BROWSE);

    pRadioCreate->SetCheck(FALSE);
    pChartLoc->EnableWindow(TRUE);
    pBrowse->EnableWindow(TRUE);

    UseExisting = TRUE;
}

