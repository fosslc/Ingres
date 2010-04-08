/*
**  Copyright (c) 2009 Ingres Corporation.
*/

/*
**  Name: ConfigType.cpp : implementation file
**
**  Description:
**	This dialog allows user to pick 1 of 4 configuration types.
**	Available configuration types are Content Management,
**	Business Intelligence, Transactional and Traditional.
**	Depending on the configuration type chosen at this dialog
**	the configuration settings for the installation are
**	adjusted to support configured system.
**
**  History:
**  28-Jul-2009 (drivi01)
**	Created.
**  04-Nov-2009 (drivi01)
**	Updated the Back button in the dialog to skip
**	install/modify/remove dialog if all the installations
**	on the machine are DBA Tools.
**
*/

#include "stdafx.h"
#include "enterprise.h"
#include "ConfigType.h"


// CConfigType dialog

IMPLEMENT_DYNAMIC(ConfigType, CPropertyPage)
ConfigType::ConfigType()
	: CPropertyPage(ConfigType::IDD)
{
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_CONFIGTITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_CONFIGSUBTITLE);
}

ConfigType::~ConfigType()
{
}

void ConfigType::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ConfigType)
	DDX_Control(pDX, IDC_RADIO_BI, bi_radio);
	DDX_Control(pDX, IDC_RADIO_TXN, txn_radio);
	DDX_Control(pDX, IDC_RADIO_CM, cm_radio);
	DDX_Control(pDX, IDC_RADIO_CLASSIC, default_radio);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ConfigType, CPropertyPage)
END_MESSAGE_MAP()


// CConfigType message handlers
BOOL ConfigType::OnInitDialog()
{
	BOOL result = CPropertyPage::OnInitDialog();
	LOGFONT lf;                        // Used to create the CFont.

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
    lf.lfHeight = 13; 
	lf.lfWeight=FW_BOLD;
    //strcpy(lf.lfFaceName, "Tahoma");    
	strcpy(lf.lfFaceName, "Ms Shell Dlg");    
    m_font_bold.CreateFontIndirect(&lf);    // Create the font.

	GetDlgItem(IDC_RADIO_BI)->SetFont(&m_font_bold);
	GetDlgItem(IDC_RADIO_TXN)->SetFont(&m_font_bold);
	GetDlgItem(IDC_RADIO_CM)->SetFont(&m_font_bold);
	GetDlgItem(IDC_RADIO_CLASSIC)->SetFont(&m_font_bold);

	((CButton *)GetDlgItem(IDC_RADIO_TXN))->SetCheck(1);

	return result;
}

BOOL ConfigType::OnSetActive()
{
	BOOL result = CPropertyPage::OnSetActive();

	//txn_radio.SetCheck(1);

	return result;
}

BOOL ConfigType::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
	case IDC_RADIO_TXN:
		thePreInstall.m_ConfigType=0;
		break;
	case IDC_RADIO_CLASSIC:
		thePreInstall.m_ConfigType=1;
		break;
	case IDC_RADIO_BI:
		thePreInstall.m_ConfigType=2;
		break;
	case IDC_RADIO_CM:
		thePreInstall.m_ConfigType=3;
		break;
	}

	return CPropertyPage::OnCommand(wParam, lParam);
}

LRESULT
ConfigType::OnWizardBack()
{
	int index = property->GetActiveIndex();
	if (thePreInstall.m_Installations.GetSize()>0 &&
		thePreInstall.m_Installations.GetSize() != thePreInstall.m_DBATools_count)
	{
		property->SetActivePage(index-1);
	}
	else
	{
		property->SetActivePage(index-2);
	}

	return CPropertyPage::OnWizardBack();
}