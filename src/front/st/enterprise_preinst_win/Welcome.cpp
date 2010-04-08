/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: Welcome.cpp : implementation file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	13-jun-2001 (somsa01)
**	    Removed the HELP button.
**	08-nov-2001 (somsa01) 
**	    Made changes corresponding to the new CA branding.
**	14-Sep-2004 (drivi01)
**	    Took out licensing.
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
**		Change the font on the Welcome Dialog to have bold title.
**		Move wizard buttons to the right if "Help" button is removed.
**		If this is a first installation on the machine scroll forward to
**		InstallMode page.
**	13-Feb-2007 (drivi01)
**	    Remove routines that move wizard buttons to the right.  After
**	    context sensitive help was removed the buttons fell in place
**	    and didn't require to be moved programmatically.
**  04-Apr-2008 (drivi01)
**		Adding functionality for DBA Tools installer to the Ingres code.
**		DBA Tools installer is a simplified package of Ingres install
**		containing only NET and Visual tools component.  
**		Updated OnWizardNext to find existing installations and mark
**		m_isDBATools field in CInstallation object if the existing
**		installation is DBA Tools installation.
**	04-Nov-2009 (drivi01)
**	    Welcome dialog will now always be followed by the license agreement
**	    dialog.  As a result the code in OnWizardNext will now be moved
**	    to LicenseInformation dialog which will be the new place to
**	    determine how the next page will be displayed.	    
*/

#include "stdafx.h"
#include "enterprise.h"
#include "Welcome.h"
#include "Splash.h"
#include ".\welcome.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


INT CompareIngresVersion(char *ii_system);
CString GetVersion(char *ii_system);
/////////////////////////////////////////////////////////////////////////////
// CWelcome property page

IMPLEMENT_DYNCREATE(CWelcome, CPropertyPage)

CWelcome::CWelcome() : CPropertyPage(CWelcome::IDD)
{     //{{AFX_DATA_INIT(CWelcome)
	  //}}AFX_DATA_INIT

	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;

}

CWelcome::~CWelcome()
{
}

void CWelcome::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_IMAGE, m_image);
	//{{AFX_DATA_MAP(CWelcome)
	DDX_Control(pDX, IDC_STATIC1, m_title);
	DDX_Control(pDX, IDC_STATIC2, m_text);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWelcome, CPropertyPage)
	//{{AFX_MSG_MAP(CWelcome)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWelcome message handlers

BOOL CWelcome::OnInitDialog()
{
	BOOL bResult = CPropertyPage::OnInitDialog();
	LOGFONT lf;                        // Used to create the CFont.


	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
    lf.lfHeight = 16; 
	lf.lfWeight=FW_BOLD;
    strcpy(lf.lfFaceName, "Tahoma");    
    m_font_bold.CreateFontIndirect(&lf);    // Create the font.


    GetDlgItem(IDC_STATIC1)->SetFont(&m_font_bold);
	lf.lfWeight=FW_NORMAL;
	lf.lfHeight=13;
	m_font.CreateFontIndirect(&lf);
	GetDlgItem(IDC_STATIC2)->SetFont(&m_font);


	return bResult;
}

BOOL CWelcome::OnSetActive() 
{
    CSplashWnd::ShowSplashScreen(this);
    SetForegroundWindow();

	CPropertySheet* pSheet = ( CPropertySheet *) GetParent();
	HWND dlgitem, dlgitem2, dlgitem3, dlgitem4;
	dlgitem = ::GetDlgItem(pSheet->m_hWnd,ID_WIZNEXT);
	dlgitem2 = ::GetDlgItem(pSheet->m_hWnd, IDCANCEL);
	dlgitem3 = ::GetDlgItem(pSheet->m_hWnd, ID_WIZBACK);
	dlgitem4 = ::GetDlgItem(pSheet->m_hWnd, IDHELP);
	

	property->GetDlgItem(ID_WIZBACK)->EnableWindow(FALSE);
	property->GetDlgItem(ID_WIZNEXT)->EnableWindow(TRUE);

    return CPropertyPage::OnSetActive();
}

LRESULT
CWelcome::OnWizardNext()
{
	return CPropertyPage::OnWizardNext();
}


