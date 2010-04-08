/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: InstallMode.cpp : implementation file
**
**  Description:
**	This module sets up the mode for the rest of the installation. 
**	It contains two choices: Express and Advanced.
**	Express installation mode will minimize user interaction and install
**	Ingres with most commonly used options.
**	Advanced installation mode will provide extensive amount of options
**  for the user to modify.
**
**  History:
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
**  14-Jul-2009 (drivi01)
**		SIR 122309
**		Configuration Type dialog is added before this dialog and the
**		code for the "Back" button is moved to that dialog as the "Back"
**              button on this dialog will now point at Configuration Type dialog.
*/



#include "stdafx.h"
#include "enterprise.h"
#include "InstallMode.h"
#include ".\installmode.h"



// InstallMode dialog

IMPLEMENT_DYNCREATE(InstallMode, CPropertyPage)

InstallMode::InstallMode() : CPropertyPage(InstallMode::IDD)
{
	m_psp.dwFlags &= ~(PSP_HASHELP);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_MODETITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_MODESUBTITLE);

}

InstallMode::~InstallMode()
{
}

void InstallMode::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO1, m_express);
	DDX_Control(pDX, IDC_RADIO2, m_advanced);
}

BOOL InstallMode::OnInitDialog()
{
	LOGFONT lf;                        // Used to create the CFont.
	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
    lf.lfHeight = 13; 
	lf.lfWeight=FW_BOLD;
    strcpy(lf.lfFaceName, "Tahoma");    
    m_font_bold.CreateFontIndirect(&lf);    // Create the font.

	GetDlgItem(IDC_RADIO1)->SetFont(&m_font_bold);
	GetDlgItem(IDC_RADIO2)->SetFont(&m_font_bold);
	return CPropertyPage::OnInitDialog();  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(InstallMode, CPropertyPage)
END_MESSAGE_MAP()



BOOL InstallMode::OnSetActive() 
{
	
	BOOL bReturn = CPropertyPage::OnSetActive();
	if (thePreInstall.m_express == 1)
		this->CheckRadioButton(IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
	else
		this->CheckRadioButton(IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);

	property->GetDlgItem(ID_WIZBACK)->EnableWindow(TRUE);
	
	return bReturn;
}
BOOL InstallMode::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case IDC_RADIO1:
		thePreInstall.m_express=1;
		return TRUE;
	case IDC_RADIO2:
		thePreInstall.m_express=0;
		return TRUE;

	}

	return CPropertyPage::OnCommand(wParam, lParam);
}
LRESULT
InstallMode::OnWizardBack()
{
	return CPropertyPage::OnWizardBack();
}




