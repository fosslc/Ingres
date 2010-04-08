/*
**
**  Name: FailDlg.cpp
**
**  Description:
**	This file contains the necessary routines to display the failure
**	dialog box.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "FailDlg.h"
#include <mmsystem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** CFailDlg dialog
*/
CFailDlg::CFailDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFailDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CFailDlg)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CFailDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CFailDlg)
	    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFailDlg, CDialog)
    //{{AFX_MSG_MAP(CFailDlg)
    ON_BN_CLICKED(IDC_BUTTON_ERRFILE, OnButtonErrfile)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CFailDlg message handlers
*/
BOOL
CFailDlg::OnInitDialog() 
{
    CString	Message;

    CDialog::OnInitDialog();
	
    Message.LoadString(IDS_FAIL_HEADING);
    SetDlgItemText(IDC_FAIL_HEADING, Message);
    Message.LoadString(IDS_BUTTON_ERRFILE);
    SetDlgItemText(IDC_BUTTON_ERRFILE, Message);
    Message.LoadString(IDS_OK);
    SetDlgItemText(IDOK, Message);

#ifndef MAINWIN
    PlaySound("SystemExclamation", NULL, SND_ASYNC | SND_NOWAIT);
#endif

    return TRUE;
}

void
CFailDlg::OnButtonErrfile() 
{
    CString ExecCmd;

#ifdef MAINWIN
    ExecCmd = CString("vi ");
#else
    ExecCmd = CString("notepad ");
#endif

    if (!ErrorLog)
	ExecCmd += CString(getenv("II_SYSTEM")) +
#ifdef MAINWIN
		   CString("/ingres/sig/dp/dperr.log");
#else
		   CString("\\ingres\\sig\\dp\\dperr.log");
#endif
    else
	ExecCmd += ErrFile;

    WinExec(ExecCmd, SW_SHOW);
}
