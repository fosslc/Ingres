/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: Welcome.cpp : Welcome page of the High Availability Option setup wizard
** 
** Description:
** 	The Welcome page displays information and directions about the High 
** 	Availability Option setup wizard.
**
** History:
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
** 	03-aug-2004 (wonst02)
** 		Add header comments.
** 	02-Sep-2004 (lakvi01)
**		Removed Licensing.
**	30-nov-2004 (wonst02) Bug 113345
**	    Remove HELP from the property page.
*/

#include "stdafx.h"
#include "wincluster.h"
#include "Welcome.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// WcWelcome property page

IMPLEMENT_DYNCREATE(WcWelcome, CPropertyPage)

WcWelcome::WcWelcome() : CPropertyPage(WcWelcome::IDD)
{
    //{{AFX_DATA_INIT(WcWelcome)
    //}}AFX_DATA_INIT

    m_psp.dwFlags &= ~(PSP_HASHELP);
}

WcWelcome::~WcWelcome()
{
}

void WcWelcome::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(WcWelcome)
	DDX_Control(pDX, IDC_IMAGE, m_image);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(WcWelcome, CPropertyPage)
	//{{AFX_MSG_MAP(WcWelcome)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// WcWelcome message handlers

BOOL WcWelcome::OnSetActive() 
{
    CSplashWnd::ShowSplashScreen(this);
    SetForegroundWindow();

    property->SetWizardButtons(PSWIZB_NEXT);
    return CPropertyPage::OnSetActive();
}
