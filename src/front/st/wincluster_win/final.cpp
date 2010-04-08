/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: Final.cpp - Windows Cluster Service setup final property page 
** 
** Description:
** 	The Final property page displays an explanation and prompts the user to 
** 	click:
** 	- Finish  to complete setup of the Ingres High Availability Option
** 	- Back    to review or change settings
** 	- Cancel  to cancel setup and exit the wizard.
** 
**  History:
**	29-apr-2004 (wonst02)
**	    Created.
** 	03-aug-2004 (wonst02)
** 		Add header comments.
**	30-nov-2004 (wonst02) Bug 113345
**	    Remove HELP from the property page.
*/

// final.cpp : implementation file
//

#include "stdafx.h"
#include "wincluster.h"
#include ".\final.h"


// WcFinal dialog

IMPLEMENT_DYNAMIC(WcFinal, CPropertyPage)
WcFinal::WcFinal()
	: CPropertyPage(WcFinal::IDD)
{
    m_psp.dwFlags &= ~(PSP_HASHELP);
}

WcFinal::~WcFinal()
{
}

void WcFinal::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IMAGE, m_image);
}


BEGIN_MESSAGE_MAP(WcFinal, CPropertyPage)
END_MESSAGE_MAP()


// WcFinal message handlers

BOOL WcFinal::OnSetActive()
{
	property->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH);

	return CPropertyPage::OnSetActive();
}

BOOL WcFinal::OnWizardFinish()
{
	// Use collected information to setup the Ingres High Availability option
	if (! SetupHighAvailability() )
		return FALSE;

	property->MyMessageBox(IDS_SETUPCOMPLETE, MB_OK);

	return CPropertyPage::OnWizardFinish();
}
