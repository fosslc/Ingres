/*
** Copyright (c) 1998, 2002 Ingres Corporation
*/

/*
**
** Name: postinst.cpp - implementation file
**
** History:
**	23-apr-99 (cucjo01)
**	    Check for previous version of Acrobat before prompting user
**	18-aug-1999 (mcgem01)
**	    The Acrobat reader has been updated to 4.0, so we've renamed
**	    the check function to something more sensible.
**	24-jul-00 (cucjo01)
**	    Disabled help button on PostConfig dialog since it isn't required.
**	10-apr-2001 (somsa01)
**	    Removed functionality not needed by the post installation DLL.
**	17-may-2001 (somsa01)
**	    Show the main window when it's time.
**	23-may-2001 (somsa01)
**	    Added checking and running of Adobe Acrobat 5.0 .
**	12-jun-2001 (somsa01)
**	    Removed the printing of IDS_REBOOT for Replicator.
**	15-jun-2001 (somsa01)
**	    RE-added the running of the clock.
**	15-aug-2001 (somsa01)
**	    In CPostInst::OnTimer(), start up IVM when finished.
**	16-aug-2001 (somsa01)
**	    Commented out the running of IVM until IVM is fixed to not
**	    cause a "Low Memory" message box to pop up and die when IVM
**	    is executed from here.
**	11-Sep-2001 (penga03)
**	    Get RebootNeeded from the Registry to determine if a reboot needed.
**	08-nov-2001 (somsa01)
**	    Uncommented out the running of IVM; it now works.
**	06-mar-2002 (penga03)
**	    Set the proper messages and open the portal Login page for
**	    evaluation releases.
**	21-mar-2002 (penga03)
**	    For evaluation releases, if Apache HTTP Server already installed 
**	    on the target machine, at the end of post installation inform user 
**	    to include ice.conf in httpd.conf.
**	17-may-2002 (penga03)
**	    Use GetLocaleInfo to retrieve the system default language. 
**	    If the language is English, then install the latest adobe, 
**	    otherwise inform user to download appropriate adobe.
**	16-jul-2002 (penga03)
**	    If the user chooses "Apache", for enterprise edition, at 
**	    the end of the post install process, we will pop up a message 
**	    box informing the user that proper change needed to make 
**	    on httpd.conf.
**	22-aug-2002 (penga03)
**	    If the latest version of Adobe Acrobat was already installed, 
**	    remove the rp505enu.exe from the Ingres temp directory.
**	02-dec-2002 (somsa01)
**	    Changed the Acrobat version and name to
**	    "AcroReader51_ENU_full.exe".
**	02-jun-2003 (penga03)
**	    Do not remove "AcroReader51_ENU_full.exe".
**	06-feb-2004 (penga03)
**	    Changed the Acrobat version and name to
**	    "AdbeRdr60_enu_full.exe".
**	19-MAR-2004 (penga03)
**	    Made changes so that the user can install the English version 
**	    of Adobe Reader even the locale setting of the target machine 
**	    is not English.
**	23-mar-2004 (penga03)
**	    If startivm is set to be NO in the response file, the installer 
**	    will not start ivm.
**	26-jul-2004 (penga03)
**	    Removed Ingres specific environment variables.
**	13-aug-2004 (penga03)
**	    Put back the Ingres specific environment variables.
**	08-oct-2004 (penga03)
**	    Removed prompting readme files.
**	20-oct-2004 (penga03)
**	    Start ivm from its new location ingres\bin.
**	15-Feb-2006(drivi01)
**	    Fix path to the adobe acrobat.
**  15-Nov-2006 (drivi01)
**      SIR 116599
**      Enhanced post-installer in effort to improve installer usability.
**		Added routines for enabling wizard buttons after configuration
**		completes to enable user to move onto the second page of the
**		property sheet.
**  04-Apr-2008 (drivi01)
**		Adding functionality for DBA Tools installer to the Ingres code.
**		DBA Tools installer is a simplified package of Ingres install
**		containing only NET and Visual tools component.  
**		Adjust the wording on the dialogs to fit DBA Tools installation.
**		
*/

#include "stdafx.h"
#include "iipostinst.h"
#include <locale.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
** CPostInst property page
*/
IMPLEMENT_DYNCREATE(CPostInst, CPropertyPage)

#define TIMER1			(900)
#define TIMER2			(2000)


CPostInst::CPostInst() : CPropertyPage(CPostInst::IDD)
{
    m_psp.dwFlags = m_psp.dwFlags & (~PSP_HASHELP);
    m_uiAVI_Resource = IDR_CLOCK_AVI;
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
    //{{AFX_DATA_INIT(CPostInst)
    //}}AFX_DATA_INIT
}

CPostInst::~CPostInst()
{
}

void CPostInst::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPostInst)
    DDX_Control(pDX, IDC_OUTPUT, m_output);
	DDX_Control(pDX, IDC_MSG, m_msg);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPostInst, CPropertyPage)
    //{{AFX_MSG_MAP(CPostInst)
    ON_WM_TIMER()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
** CPostInst message handlers
*/
HDDEDATA EXPENTRY DdeCallBack20(UINT u1,UINT u2,HCONV h1,HSZ h2,HSZ h3,HDDEDATA d,DWORD dw1,DWORD dw2)
{
    return (HDDEDATA) 0;
}

void CPostInst::RemoveCAInstallerCrap()
{
    RemoveDirectory("C:\\CA_APPSW");
}

BOOL
CPostInst::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    
    ShowWindow(SW_SHOW);

    CWnd *hAniWnd = GetDlgItem(IDC_ANIMATE1);
    if(hAniWnd)
    {
	hAniWnd->SendMessage(ACM_OPEN, 0,
			     (LPARAM)(LPTSTR)(MAKEINTRESOURCE(m_uiAVI_Resource)));
	hAniWnd->SendMessage(ACM_PLAY, (WPARAM)(UINT)(-1),
			     (LPARAM)MAKELONG(0, 0xFFFF));
    }

    theInstall.PostInstallation((CStatic *)GetDlgItem(IDC_OUTPUT));

	if (theInstall.m_DBATools)
	{
		CString str;
		GetDlgItemText(IDC_MSG, str);
		str.Replace("Please wait", "Wait");
		str.Replace("Ingres", "DBA Tools");
		SetDlgItemText(IDC_MSG, str);
	}

    SetTimer(1,TIMER1,0);
    return TRUE; 
}

BOOL
CPostInst::OnSetActive() 
{
#ifdef EVALUATION_RELEASE
    CString s;
    s.LoadString(IDS_FINISHMSG);
    m_msg.SetWindowText(s);
#endif /* EVALUATION_RELEASE */

    return CPropertyPage::OnSetActive();
}

LRESULT
CPostInst::OnWizardNext()
{
	return CPropertyPage::OnWizardNext();
}

void
CPostInst::OnTimer(UINT nIDEvent) 
{
    CWnd *hAniWnd = GetDlgItem(IDC_ANIMATE1);


    if (nIDEvent == 1)
    {
	if (!theInstall.IsBusy())
	{
	    KillTimer(1);
	    KillTimer(2);
	    hAniWnd->SendMessage(ACM_STOP, 0, 0);
	    
	    if (theInstall.m_postInstRet)
	    {
			SetWindowPos(&CWnd::wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			SetFocus();
			theInstall.m_CompletedSuccessfully = TRUE;

			InvalidateRect(NULL, 1);
		
		} 
		else
		{
			if (AskUserYN(IDS_PROBLEMS,theInstall.m_installPath))
			    theInstall.ReadInstallLog();
			theInstall.m_CompletedSuccessfully = FALSE;
			
			this->OnWizardNext();
		}

		CWnd *parent_wnd = GetParent();
		if (parent_wnd)
		{
			CWnd *wnd = parent_wnd->GetDlgItem(ID_WIZNEXT);
			if (wnd)
				wnd->ShowWindow(SW_SHOW);

			wnd = parent_wnd->GetDlgItem(IDCANCEL);
			if (wnd)
				wnd->ShowWindow(SW_SHOW);
			wnd = parent_wnd->GetDlgItem(ID_WIZBACK);
			if (wnd)
			{
				wnd->ShowWindow(SW_SHOW);
				wnd->EnableWindow(FALSE);
			}
			wnd = GetDlgItem(IDC_MSG);
			if (wnd)
				wnd->ShowWindow(SW_HIDE);
			wnd = GetDlgItem(IDC_MSG2);
			if (wnd)
				wnd->ShowWindow(SW_SHOW);

		}
	} /* if (!theInstall.IsBusy()) */
	} /* if (nIDEvent == 1) */
}
