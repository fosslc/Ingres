/*
**
**  Name: Options4Page.cpp
**
**  Description:
**	This file contains the necessary routines to display the Options4
**	property page (the logging options of DP).
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "Options4Page.h"
#include "WaitDlg.h"
#include "FailDlg.h"
#include "SuccessDlg.h"
#include <cderr.h>
#include <dlgs.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static INT ExecuteCommand();
static INT LocationSelect(HWND hDlg, CString *InputFile);
static UINT CALLBACK LocSelHookProc(HWND hDlg, UINT messg, WPARAM wParam,
				    LPARAM lParam);
static VOID ProcessCDError(DWORD dwErrorCode, HWND hWnd);

/*
** COptions4Page property page
*/
IMPLEMENT_DYNCREATE(COptions4Page, CPropertyPage)

COptions4Page::COptions4Page() : CPropertyPage(COptions4Page::IDD)
{
    //{{AFX_DATA_INIT(COptions4Page)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

COptions4Page::~COptions4Page()
{
}

COptions4Page::COptions4Page(CPropertySheet *ps) : CPropertyPage(COptions4Page::IDD)
{
    m_propertysheet = ps;
    m_psp.dwFlags |= (PSP_HASHELP);
}

void COptions4Page::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COptions4Page)
    DDX_Control(pDX, IDC_IMAGE, m_image);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COptions4Page, CPropertyPage)
    //{{AFX_MSG_MAP(COptions4Page)
    ON_BN_CLICKED(IDC_CHECK_OUTFILE, OnCheckOutfile)
    ON_BN_CLICKED(IDC_CHECK_ERRFILE, OnCheckErrfile)
    ON_BN_CLICKED(IDC_CHECK_OPTFILE, OnCheckOptfile)
    ON_BN_CLICKED(IDC_CHECK_SQLFILE, OnCheckSqlfile)
    ON_BN_CLICKED(IDC_BUTTON_OUTFILE, OnButtonOutfile)
    ON_BN_CLICKED(IDC_BUTTON_ERRFILE, OnButtonErrfile)
    ON_BN_CLICKED(IDC_BUTTON_OPTFILE, OnButtonOptfile)
    ON_BN_CLICKED(IDC_BUTTON_SQLFILE, OnButtonSqlfile)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** COptions4Page message handlers
*/
BOOL
COptions4Page::OnSetActive() 
{
    CButton *pCheckSQL = (CButton *)GetDlgItem(IDC_CHECK_SQLFILE);
    CEdit *pEditSQL = (CEdit *)GetDlgItem(IDC_EDIT_SQL_COPY);
    CButton *pButtonSQL = (CButton *)GetDlgItem(IDC_BUTTON_SQLFILE);

    m_propertysheet->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);

    if (!FFFLAG && !OFFLAG)
    {
	pCheckSQL->SetCheck(FALSE);
	pCheckSQL->EnableWindow(FALSE);
	pEditSQL->EnableWindow(FALSE);
	pButtonSQL->EnableWindow(FALSE);
    }
    else
    {
	pCheckSQL->EnableWindow(TRUE);
    }

    return CPropertyPage::OnSetActive();
}

BOOL
COptions4Page::OnInitDialog() 
{
    CEdit *pEditOutput = (CEdit *)GetDlgItem(IDC_EDIT_OUTPUT_LOG);
    CEdit *pEditError = (CEdit *)GetDlgItem(IDC_EDIT_ERROR_LOG);
    CEdit *pEditOptions = (CEdit *)GetDlgItem(IDC_EDIT_OPTIONS_LOG);
    CEdit *pEditSQL = (CEdit *)GetDlgItem(IDC_EDIT_SQL_COPY);
    CButton *pButtonOutput = (CButton *)GetDlgItem(IDC_BUTTON_OUTFILE);
    CButton *pButtonError = (CButton *)GetDlgItem(IDC_BUTTON_ERRFILE);
    CButton *pButtonOptions = (CButton *)GetDlgItem(IDC_BUTTON_OPTFILE);
    CButton *pButtonSQL = (CButton *)GetDlgItem(IDC_BUTTON_SQLFILE);

    Message.LoadString(IDS_OPTIONS4_HEADING);
    SetDlgItemText(IDC_OPTIONS4_HEADING, Message);
    Message.LoadString(IDS_CHECK_OUTFILE);
    SetDlgItemText(IDC_CHECK_OUTFILE, Message);
    Message.LoadString(IDS_CHECK_ERRFILE);
    SetDlgItemText(IDC_CHECK_ERRFILE, Message);
    Message.LoadString(IDS_CHECK_OPTFILE);
    SetDlgItemText(IDC_CHECK_OPTFILE, Message);
    Message.LoadString(IDS_CHECK_SQLFILE);
    SetDlgItemText(IDC_CHECK_SQLFILE, Message);
    Message.LoadString(IDS_BUTTON_OUTFILE);
    SetDlgItemText(IDC_BUTTON_OUTFILE, Message);
    Message.LoadString(IDS_BUTTON_ERRFILE2);
    SetDlgItemText(IDC_BUTTON_ERRFILE, Message);
    Message.LoadString(IDS_BUTTON_OPTFILE);
    SetDlgItemText(IDC_BUTTON_OPTFILE, Message);
    Message.LoadString(IDS_BUTTON_SQLFILE);
    SetDlgItemText(IDC_BUTTON_SQLFILE, Message);

    pEditOutput->EnableWindow(FALSE);
    pEditError->EnableWindow(FALSE);
    pEditOptions->EnableWindow(FALSE);
    pEditSQL->EnableWindow(FALSE);
    pButtonOutput->EnableWindow(FALSE);
    pButtonError->EnableWindow(FALSE);
    pButtonOptions->EnableWindow(FALSE);
    pButtonSQL->EnableWindow(FALSE);

    OutputLog = ErrorLog = FALSE;

    CPropertyPage::OnInitDialog();	
	
    return TRUE;
}

void
COptions4Page::OnCheckOutfile() 
{
    CButton *pCheckOutput = (CButton *)GetDlgItem(IDC_CHECK_OUTFILE);
    CEdit *pEditOutput = (CEdit *)GetDlgItem(IDC_EDIT_OUTPUT_LOG);
    CButton *pButtonOutput = (CButton *)GetDlgItem(IDC_BUTTON_OUTFILE);

    pEditOutput->EnableWindow(pCheckOutput->GetCheck());
    pButtonOutput->EnableWindow(pCheckOutput->GetCheck());
    OutputLog = pCheckOutput->GetCheck();
}

void
COptions4Page::OnCheckErrfile() 
{
    CButton *pCheckError = (CButton *)GetDlgItem(IDC_CHECK_ERRFILE);
    CEdit *pEditError = (CEdit *)GetDlgItem(IDC_EDIT_ERROR_LOG);
    CButton *pButtonError = (CButton *)GetDlgItem(IDC_BUTTON_ERRFILE);

    pEditError->EnableWindow(pCheckError->GetCheck());
    pButtonError->EnableWindow(pCheckError->GetCheck());
    ErrorLog = pCheckError->GetCheck();
}

void
COptions4Page::OnCheckOptfile() 
{
    CButton *pCheckOptions = (CButton *)GetDlgItem(IDC_CHECK_OPTFILE);
    CEdit *pEditOptions = (CEdit *)GetDlgItem(IDC_EDIT_OPTIONS_LOG);
    CButton *pButtonOptions = (CButton *)GetDlgItem(IDC_BUTTON_OPTFILE);
    
    pEditOptions->EnableWindow(pCheckOptions->GetCheck());
    pButtonOptions->EnableWindow(pCheckOptions->GetCheck());
}

void
COptions4Page::OnCheckSqlfile() 
{
    CButton *pCheckSQL = (CButton *)GetDlgItem(IDC_CHECK_SQLFILE);
    CEdit *pEditSQL = (CEdit *)GetDlgItem(IDC_EDIT_SQL_COPY);
    CButton *pButtonSQL = (CButton *)GetDlgItem(IDC_BUTTON_SQLFILE);

    pEditSQL->EnableWindow(pCheckSQL->GetCheck());
    pButtonSQL->EnableWindow(pCheckSQL->GetCheck());
}

BOOL
COptions4Page::OnWizardFinish() 
{
    CWaitDlg	wait;
    CFailDlg	fail;
    CSuccessDlg	success;
    HANDLE	hThread;
    DWORD	ThreadID, status = 0;
    CString	FileName;
    CButton	*pCheckOutput = (CButton *)GetDlgItem(IDC_CHECK_OUTFILE);
    CButton	*pCheckError = (CButton *)GetDlgItem(IDC_CHECK_ERRFILE);
    CButton	*pCheckOptions = (CButton *)GetDlgItem(IDC_CHECK_OPTFILE);
    CButton	*pCheckSQL = (CButton *)GetDlgItem(IDC_CHECK_SQLFILE);

    Options4 = "";
    if (pCheckOutput->GetCheck())
    {
	GetDlgItemText(IDC_EDIT_OUTPUT_LOG, OutFile);
	if (OutFile == "")
	{
	    Message.LoadString(IDS_NEED_OUTFILE);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return 0;
	}

	Options4 += CString("-fl") + OutFile + CString(" ");
    }
    if (pCheckError->GetCheck())
    {
	GetDlgItemText(IDC_EDIT_ERROR_LOG, ErrFile);
	if (ErrFile == "")
	{
	    Message.LoadString(IDS_NEED_ERRFILE);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return 0;
	}

	Options4 += CString("-fe") + ErrFile + CString(" ");
    }
    if (pCheckOptions->GetCheck())
    {
	GetDlgItemText(IDC_EDIT_OPTIONS_LOG, FileName);
	if (FileName == "")
	{
	    Message.LoadString(IDS_NEED_OPTFILE);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return 0;
	}

	Options4 += CString("-fo") + FileName + CString(" ");
    }
    if (pCheckSQL->GetCheck())
    {
	GetDlgItemText(IDC_EDIT_SQL_COPY, FileName);
	if (FileName == "")
	{
	    Message.LoadString(IDS_NEED_SQLFILE);
	    Message2.LoadString(IDS_TITLE);
	    MessageBox(Message, Message2, MB_OK | MB_ICONEXCLAMATION);
	    return 0;
	}

	Options4 += CString("-fc") + FileName + CString(" ");
    }

    hThread = CreateThread( NULL, 0,
			    (LPTHREAD_START_ROUTINE)ExecuteCommand,
			    NULL, 0, &ThreadID);
    wait.DoModal();
    while (WaitForSingleObject(hThread, 0) != WAIT_OBJECT_0);
    GetExitCodeThread(hThread, &status);
    if (status)
    {
	fail.DoModal();
	return 0;
    }

    success.DoModal();

    return CPropertyPage::OnWizardFinish();
}

static INT
ExecuteCommand()
{
    STARTUPINFO		siStInfo;
    PROCESS_INFORMATION	piProcInfo;
    SECURITY_ATTRIBUTES	sa;
    DWORD		status, wait_status = 0, NumWritten;
    HANDLE		newstdout, newstderr;
    CString		LogOutFile, LogErrFile, CommandLine;
    CString		II_SYSTEM = getenv("II_SYSTEM");

    CommandLine = CString(II_SYSTEM) +
#ifdef MAINWIN
		  CString("/ingres/sig/dp/dp ") +
#else
		  CString("\\ingres\\sig\\dp\\dp ") +
#endif
		  ConnectLine + CString(" ") + Options1 + CString(" ") +
		  Options2 + CString(" ") + Options3 + CString(" ") +
		  Options4;
    
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    LogOutFile = CString(getenv("II_SYSTEM")) +
#ifdef MAINWIN
		 CString("/ingres/sig/dp/dpout.log");
#else
		 CString("\\ingres\\sig\\dp\\dpout.log");
#endif
    LogErrFile = CString(getenv("II_SYSTEM")) +
#ifdef MAINWIN
		 CString("/ingres/sig/dp/dperr.log");
#else
		 CString("\\ingres\\sig\\dp\\dperr.log");
#endif
    memset (&siStInfo, 0, sizeof (siStInfo)) ;

    siStInfo.cb = sizeof (siStInfo);
    siStInfo.lpReserved = NULL;
    siStInfo.lpReserved2 = NULL;
    siStInfo.cbReserved2 = 0;
    siStInfo.lpDesktop = NULL;
    siStInfo.dwFlags = STARTF_USESHOWWINDOW;
    siStInfo.wShowWindow = SW_HIDE;

    /*
    ** Set the process' output and error handles to the log file.
    */
    if (!OutputLog)
    {
	newstdout = CreateFile( LogOutFile, GENERIC_WRITE, FILE_SHARE_WRITE,
				&sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
				NULL);
    }
    else
	newstdout = GetStdHandle (STD_OUTPUT_HANDLE);

    if (!ErrorLog)
    {
	newstderr = CreateFile( LogErrFile, GENERIC_WRITE, FILE_SHARE_WRITE,
				&sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
				NULL);
    }
    else
	newstderr = GetStdHandle (STD_ERROR_HANDLE);

    siStInfo.dwFlags |= STARTF_USESTDHANDLES;
    siStInfo.hStdInput  = GetStdHandle (STD_INPUT_HANDLE);
    siStInfo.hStdOutput = newstdout;
    siStInfo.hStdError = newstderr;

    if (CommandLine != "")
    {
	WriteFile(newstdout, CommandLine, CommandLine.GetLength(),
		  &NumWritten, NULL);
	WriteFile(newstdout, "\r\n", 2, &NumWritten, NULL);
	status = CreateProcess( NULL,
				CommandLine.GetBuffer(CommandLine.GetLength()),
				&sa, NULL, TRUE, 0, NULL, NULL, &siStInfo,
				&piProcInfo);
	if (!status)
	{
	    CloseHandle(newstdout);
	    CloseHandle(newstderr);
	    while (!MainWnd);
	    SendMessage(MainWnd, WM_CLOSE, 0, 0);
	    return (status);
	}
	else
	{
	    WaitForSingleObject(piProcInfo.hProcess, INFINITE);
	    GetExitCodeProcess(piProcInfo.hProcess, &wait_status);
	    CloseHandle(piProcInfo.hProcess);
	    CloseHandle(piProcInfo.hThread);
	    if (wait_status)
	    {
		CloseHandle(newstdout);
		CloseHandle(newstderr);
		while (!MainWnd);
		SendMessage(MainWnd, WM_CLOSE, 0, 0);
		return (wait_status);
	    }
	}
	WriteFile(newstdout, "\r\n", 2, &NumWritten, NULL);
    }

    CloseHandle(newstdout);
    CloseHandle(newstderr);
    Sleep(2000);
    SendMessage(MainWnd, WM_CLOSE, 0, 0);
    return 0;
}

void
COptions4Page::OnButtonOutfile() 
{
    CString FileName;

    LocationSelect(m_hWnd, &FileName);
    SetDlgItemText(IDC_EDIT_OUTPUT_LOG, FileName);
}

void
COptions4Page::OnButtonErrfile() 
{
    CString FileName;

    LocationSelect(m_hWnd, &FileName);
    SetDlgItemText(IDC_EDIT_ERROR_LOG, FileName);
}

void
COptions4Page::OnButtonOptfile() 
{
    CString FileName;

    LocationSelect(m_hWnd, &FileName);
    SetDlgItemText(IDC_EDIT_OPTIONS_LOG, FileName);
}

void
COptions4Page::OnButtonSqlfile() 
{
    CString FileName;

    LocationSelect(m_hWnd, &FileName);
    SetDlgItemText(IDC_EDIT_SQL_COPY, FileName);
}

/*
**  Name: LocSelHookProc
**
**  Description:
**      Hook Procedure for GetFileNameOpen() common dialog box
*/
static UINT CALLBACK 
LocSelHookProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
    BOOL	rc;
    CWnd	Parent;

    rc=TRUE;
    switch (messg)
    {
	case WM_INITDIALOG:          /* Set Default Values for Entry Fields */
	    Parent.Attach(::GetParent(hDlg));
	    Parent.CenterWindow();
	    Parent.Detach();
	    CommDlg_OpenSave_SetControlText(::GetParent(hDlg), IDOK, "OK");
	    rc=TRUE;
	    break;

	default:
	    rc=FALSE;
	    break;
    }
    return rc;
}

/*
**  Name: LocationSelect
**
**  Description:
**	Dialog procedure for selecting the source file that contains
**	the non-relational data description.
*/
static INT
LocationSelect(HWND hDlg, CString *InputFile)
{
    CHAR	FileFilter[] = "All Files (*.*)\0*.*\0\0";
    DWORD	status;
    CHAR	FileTitle[256], FileName[2048];
    OPENFILENAME OpenFileName;

    *InputFile = "";
    strcpy(FileName, "");
    strcpy(FileTitle, "");

    OpenFileName.lStructSize       = sizeof(OPENFILENAME);
    OpenFileName.hwndOwner         = hDlg;
    OpenFileName.lpstrCustomFilter = (LPSTR) NULL;
    OpenFileName.nMaxCustFilter    = 0L;
    OpenFileName.nFilterIndex      = 1L;
    OpenFileName.lpstrFilter       = FileFilter;
    OpenFileName.lpstrFile         = FileName;
    OpenFileName.nMaxFile          = sizeof(FileName);
    OpenFileName.lpstrDefExt       = NULL;
    OpenFileName.Flags             = OFN_ENABLEHOOK | OFN_HIDEREADONLY |
				     OFN_EXPLORER;
    OpenFileName.lpfnHook          = LocSelHookProc;
    OpenFileName.lpstrFileTitle    = FileTitle;
    OpenFileName.nMaxFileTitle     = sizeof(FileTitle);
    OpenFileName.lpstrInitialDir   = NULL;
    OpenFileName.lpstrTitle        = "DP";
    OpenFileName.nFileOffset       = 0;
    OpenFileName.nFileExtension    = 0;
    OpenFileName.lCustData         = 0;

    if (!GetOpenFileName(&OpenFileName))
    {
	status = CommDlgExtendedError();

	if (status)
	{
	    ProcessCDError(status, hDlg );
	    return FALSE;
	}
    }
    else
    {
	*InputFile = CString(FileName);
    }

    return TRUE;
}

/*
**  Name: ProcessCDError
**
**  Description:
**	Procedure for processing errors returned from opening common dialog
**	boxes (e.g., GetOpenFileName()).
*/
static VOID
ProcessCDError(DWORD dwErrorCode, HWND hWnd)
{
    WORD	wStringID;
    CString	buf, Message;

    switch(dwErrorCode)
    {
	case CDERR_DIALOGFAILURE:   wStringID=IDS_DIALOGFAILURE;   break;
	case CDERR_STRUCTSIZE:      wStringID=IDS_STRUCTSIZE;      break;
	case CDERR_INITIALIZATION:  wStringID=IDS_INITIALIZATION;  break;
	case CDERR_NOTEMPLATE:      wStringID=IDS_NOTEMPLATE;      break;
	case CDERR_NOHINSTANCE:     wStringID=IDS_NOHINSTANCE;     break;
	case CDERR_LOADSTRFAILURE:  wStringID=IDS_LOADSTRFAILURE;  break;
	case CDERR_FINDRESFAILURE:  wStringID=IDS_FINDRESFAILURE;  break;
	case CDERR_LOADRESFAILURE:  wStringID=IDS_LOADRESFAILURE;  break;
	case CDERR_LOCKRESFAILURE:  wStringID=IDS_LOCKRESFAILURE;  break;
	case CDERR_MEMALLOCFAILURE: wStringID=IDS_MEMALLOCFAILURE; break;
	case CDERR_MEMLOCKFAILURE:  wStringID=IDS_MEMLOCKFAILURE;  break;
	case CDERR_NOHOOK:          wStringID=IDS_NOHOOK;          break;
	case PDERR_SETUPFAILURE:    wStringID=IDS_SETUPFAILURE;    break;
	case PDERR_PARSEFAILURE:    wStringID=IDS_PARSEFAILURE;    break;
	case PDERR_RETDEFFAILURE:   wStringID=IDS_RETDEFFAILURE;   break;
	case PDERR_LOADDRVFAILURE:  wStringID=IDS_LOADDRVFAILURE;  break;
	case PDERR_GETDEVMODEFAIL:  wStringID=IDS_GETDEVMODEFAIL;  break;
	case PDERR_INITFAILURE:     wStringID=IDS_INITFAILURE;     break;
	case PDERR_NODEVICES:       wStringID=IDS_NODEVICES;       break;
	case PDERR_NODEFAULTPRN:    wStringID=IDS_NODEFAULTPRN;    break;
	case PDERR_DNDMMISMATCH:    wStringID=IDS_DNDMMISMATCH;    break;
	case PDERR_CREATEICFAILURE: wStringID=IDS_CREATEICFAILURE; break;
	case PDERR_PRINTERNOTFOUND: wStringID=IDS_PRINTERNOTFOUND; break;
	case CFERR_NOFONTS:         wStringID=IDS_NOFONTS;         break;
	case FNERR_SUBCLASSFAILURE: wStringID=IDS_SUBCLASSFAILURE; break;
	case FNERR_INVALIDFILENAME: wStringID=IDS_INVALIDFILENAME; break;
	case FNERR_BUFFERTOOSMALL:  wStringID=IDS_BUFFERTOOSMALL;  break;

	case 0:   /* User may have hit CANCEL or we got a *very* random error */
	    return;

	default:
	    wStringID=IDS_UNKNOWNERROR;
    }

    buf.LoadString(wStringID);
    Message.LoadString(IDS_TITLE);
    MessageBox(hWnd, buf, Message, MB_APPLMODAL | MB_OK);
    return;
}
