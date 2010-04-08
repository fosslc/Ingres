/*
**  Copyright (c) 2006 Ingres Corporation. All Rights Reserved.
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
**	01-Mar-2006 (drivi01)
**	    Reused for MSI Patch Installer on Windows.
*/

#include "stdafx.h"
#include "enterprise.h"
#include "Welcome.h"
#include "Splash.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWelcome property page

IMPLEMENT_DYNCREATE(CWelcome, CPropertyPage)

CWelcome::CWelcome() : CPropertyPage(CWelcome::IDD)
{
    //{{AFX_DATA_INIT(CWelcome)
    //}}AFX_DATA_INIT

    m_psp.dwFlags &= ~(PSP_HASHELP);
}

CWelcome::~CWelcome()
{
}

void CWelcome::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWelcome)
	DDX_Control(pDX, IDC_IMAGE, m_image);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWelcome, CPropertyPage)
	//{{AFX_MSG_MAP(CWelcome)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWelcome message handlers

BOOL CWelcome::OnSetActive() 
{
    CSplashWnd::ShowSplashScreen(this);
    SetForegroundWindow();

    property->SetWizardButtons(PSWIZB_NEXT);
    return CPropertyPage::OnSetActive();
}
