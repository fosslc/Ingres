/*
**
**  Name: FailDlg.cpp
**
**  Description:
**	This file contains all functions needed to display the Error Dialog.
**
**  History:
**	27-apr-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "iilink.h"
#include "FailDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** FailDlg dialog
*/
FailDlg::FailDlg(CWnd* pParent)
: CDialog(FailDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(FailDlg)
	// NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void FailDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(FailDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(FailDlg, CDialog)
    //{{AFX_MSG_MAP(FailDlg)
    ON_BN_CLICKED(IDC_BUTTON_SEE_ERRS, OnButtonSeeErrs)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** FailDlg message handlers
*/
void
FailDlg::OnButtonSeeErrs() 
{
    CString	ViewLog;
    
    ViewLog = CString("notepad \"") + CString(getenv("II_SYSTEM")) +
	      CString("\\ingres\\files\\iilink.log\"");
    WinExec(ViewLog, SW_SHOWDEFAULT);
}

BOOL
FailDlg::OnInitDialog() 
{
    CString	Message;

    CDialog::OnInitDialog();

    Message.LoadString(IDS_FAIL_HEADING);
    SetDlgItemText(IDC_FAIL_HEADING, Message);
    Message.LoadString(IDS_OK);
    SetDlgItemText(IDOK, Message);
    Message.LoadString(IDS_BUTTON_SEE_ERRS);
    SetDlgItemText(IDC_BUTTON_SEE_ERRS, Message);

    return TRUE;
}
