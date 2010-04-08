/*
**
**  Name: SuccessDlg.cpp
**
**  Description:
**	This file contains the necessary routines to display the success
**      dialog box.
**
**  History:
**	10-jul-1999 (somsa01)
**	    Created.
*/

#include "stdafx.h"
#include "dp.h"
#include "SuccessDlg.h"
#include <mmsystem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** CSuccessDlg dialog
*/
CSuccessDlg::CSuccessDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSuccessDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CSuccessDlg)
	    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CSuccessDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSuccessDlg)
	    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSuccessDlg, CDialog)
    //{{AFX_MSG_MAP(CSuccessDlg)
    ON_BN_CLICKED(IDC_BUTTON_OUTFILE, OnButtonOutfile)
    ON_BN_CLICKED(IDC_BUTTON_ERRFILE, OnButtonErrfile)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CSuccessDlg message handlers
*/
BOOL
CSuccessDlg::OnInitDialog() 
{
    CString	Message;

    CDialog::OnInitDialog();
	
    Message.LoadString(IDS_SUCCESS_HEADING);
    SetDlgItemText(IDC_SUCCESS_HEADING, Message);
    Message.LoadString(IDS_BUTTON_OUTFILE2);
    SetDlgItemText(IDC_BUTTON_OUTFILE, Message);
    Message.LoadString(IDS_BUTTON_ERRFILE);
    SetDlgItemText(IDC_BUTTON_ERRFILE, Message);
    Message.LoadString(IDS_OK);
    SetDlgItemText(IDOK, Message);

#ifndef MAINWIN
    PlaySound("SystemAsterisk", NULL, SND_ASYNC | SND_NOWAIT);
#endif
	
    return TRUE;
}

void
CSuccessDlg::OnButtonOutfile() 
{
    CString ExecCmd;

#ifdef MAINWIN
    ExecCmd = CString("vi ");
#else
    ExecCmd = CString("notepad ");
#endif

    if (!OutputLog)
	ExecCmd += CString(getenv("II_SYSTEM")) +
#ifdef MAINWIN
		   CString("/ingres/sig/dp/dpout.log");
#else
		   CString("\\ingres\\sig\\dp\\dpout.log");
#endif
    else
	ExecCmd += OutFile;

    WinExec(ExecCmd, SW_SHOW);	
}

void
CSuccessDlg::OnButtonErrfile() 
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
