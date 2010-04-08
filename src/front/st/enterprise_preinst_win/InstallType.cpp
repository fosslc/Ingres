/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: InstallType.cpp : implementation file
**
**  Description:
**	This module is displayed only if there's existing installation
**  on the machine.  It will provide a choice of upgrading or modifying
**  existing installations or proceeding with a new installation.
**  The goal of this dialog is to simplify user choices.
**  This dialog will proceed to InstanceList dialog if upgrade/modify is 
**  chosen and it will display installations that are available to be
**  modified in Upgrade scenario and installations available for modifying
**  in modify scenario.  New Instance scenario will skip InstanceList dialog.
**
**  History:
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
**  12-Mar-2007 (drivi01)
**      Updated the dialog with images.
**  05-May-2008 (drivi01)
**	    Update parameters for upgrade qualified existing installations.
**      if the installed version is greater than the version being
**      installed, isntallation qualifies only to be modified.
*/


#include "stdafx.h"
#include "enterprise.h"
#include "InstallType.h"
#include ".\installtype.h"


// InstallType dialog

IMPLEMENT_DYNAMIC(InstallType, CPropertyPage)
InstallType::InstallType()
	: CPropertyPage(InstallType::IDD)
{

	m_psp.dwFlags &= ~(PSP_HASHELP);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_INSTALLTYPETITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_INSTALLTYPESUBTITLE);
		

}

InstallType::~InstallType()
{
}

void InstallType::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_UPGRADE, m_upgrade);
	DDX_Control(pDX, IDC_RADIO_MODIFY, m_modify);
	DDX_Control(pDX, IDC_RADIO_INSTALL, m_installnew);
	//DDX_Control(pDX, IDC_SIDE_BAR, m_image);
}




BOOL
InstallType::OnSetActive()
{
	CWnd *wnd_u = GetDlgItem(IDC_STATIC_UPGRADE);
	CWnd *wnd_m = GetDlgItem(IDC_STATIC_MODIFY);
	CWnd *wnd_i = GetDlgItem(IDC_STATIC_INSTALL);

	BOOL is_upgrade = 0;
	BOOL is_modify = 0;
	m_upgrade.SetCheck(0);
	m_modify.SetCheck(0);
	m_installnew.SetCheck(0);

	m_upgrade.ShowWindow(SW_SHOW);
	m_modify.ShowWindow(SW_SHOW);
	m_installnew.ShowWindow(SW_SHOW);

	if (wnd_u && wnd_m && wnd_i)
	{
		wnd_u->ShowWindow(SW_SHOW);
		wnd_m->ShowWindow(SW_SHOW);
		wnd_i->ShowWindow(SW_SHOW);
	}
	
	if (thePreInstall.m_InstallType == 0)
		m_upgrade.SetCheck(1);
	else if (thePreInstall.m_InstallType == 1)
		m_modify.SetCheck(1);
	else if (thePreInstall.m_InstallType == 2)
		m_installnew.SetCheck(1);

	for(int i=0; i<thePreInstall.m_Installations.GetSize(); i++)
	{
		CInstallation *inst=(CInstallation *) thePreInstall.m_Installations.GetAt(i);
		if (inst)
		{
			if (inst->m_UpgradeCode == 1 || inst->m_UpgradeCode == 2)
				is_upgrade=1;
			if (inst->m_UpgradeCode == 0 || inst->m_UpgradeCode == 3)
				is_modify=1;

		}
	}


	if (is_upgrade == 0)
	{
		CRect urect;
		CWnd *wnd = GetDlgItem(IDC_STATIC_GROUP);
		if (wnd)
		{
			wnd->GetWindowRect(&urect);
			ScreenToClient(&urect);
		}
		CRect pic_m, pic_u, pic_i;
		if (wnd_u)
		{
			wnd_u->GetWindowRect(&pic_u);
			ScreenToClient(&pic_u);
		}
		if (wnd_m)
		{
			wnd_m->GetWindowRect(&pic_m);
			ScreenToClient(&pic_m);
		}
		if (wnd_i)
		{
			wnd_i->GetWindowRect(&pic_i);
			ScreenToClient(&pic_i);
		}
		
		m_upgrade.ShowWindow(SW_HIDE);
		wnd_u->ShowWindow(SW_HIDE);
		CRect rect, irect;
		m_modify.GetWindowRect(&rect);
		m_installnew.GetWindowRect(&irect);
		ScreenToClient(&rect);
		ScreenToClient(&irect);

		int height=pic_m.bottom - pic_m.top;
		rect.top = urect.top + ((urect.bottom - urect.top) * .15);
		rect.bottom = rect.top+rect.Height();
		pic_m.top = rect.top;
		pic_m.bottom = pic_m.top + height;
		irect.top = urect.bottom - ((urect.bottom-urect.top) * .35);
		irect.bottom = irect.top + irect.Height();
		pic_i.top = irect.top - (height * .5);
		pic_i.bottom = pic_i.top + height;
		m_modify.MoveWindow(&rect, 1);
		wnd_m->MoveWindow(&pic_m, 1);
		m_installnew.MoveWindow(&irect, 1);
		wnd_i->MoveWindow(&pic_i, 1);
		if (thePreInstall.m_InstallType == -1)
		{
			m_modify.SetCheck(1);
			thePreInstall.m_InstallType = 1;
		}
	}

	if (is_modify == 0)
	{
		CRect rect;
		CWnd *wnd = GetDlgItem(IDC_STATIC_GROUP);
		if (wnd)
		{
			wnd->GetWindowRect(&rect);
			ScreenToClient(&rect);
		}
		CRect pic_m, pic_u, pic_i;
		if (wnd_u)
		{
			wnd_u->GetWindowRect(&pic_u);
			ScreenToClient(&pic_u);
		}
		if (wnd_m)
		{
			wnd_m->GetWindowRect(&pic_m);
			ScreenToClient(&pic_m);
		}
		if (wnd_i)
		{
			wnd_i->GetWindowRect(&pic_i);
			ScreenToClient(&pic_i);
		}

		m_modify.ShowWindow(SW_HIDE);
		wnd_m->ShowWindow(SW_HIDE);
		CRect urect, irect;
		m_upgrade.GetWindowRect(&urect);
		m_installnew.GetWindowRect(&irect);
		ScreenToClient(&urect);
		ScreenToClient(&irect);

		int height = urect.Height();
		int img_height = pic_u.bottom - pic_u.top;
		urect.top = rect.top + ((rect.bottom - rect.top) *.25);
		urect.bottom = urect.top+height;
		pic_u.top = urect.top-(img_height * .5);
		pic_u.bottom = pic_u.top + img_height;
		height = irect.Height();
		irect.top = rect.bottom - ((rect.bottom - rect.top) *.35);
		irect.bottom = irect.top +height;
		pic_i.top = irect.top-(img_height * .5);
		pic_i.bottom = pic_i.top + img_height;
		m_upgrade.MoveWindow(&urect, 1);
		wnd_u->MoveWindow(&pic_u, 1);
		m_installnew.MoveWindow(&irect, 1);
		wnd_i->MoveWindow(&pic_i, 1);
		if (thePreInstall.m_InstallType == -1)
		{
			m_upgrade.SetCheck(1);
			thePreInstall.m_InstallType = 0;
		}
	}

	if (is_upgrade == 1 && is_modify == 1)
		if (thePreInstall.m_InstallType == -1)
		{
			m_upgrade.SetCheck(1);
			thePreInstall.m_InstallType = 0;
		}

	property->GetDlgItem(ID_WIZBACK)->EnableWindow(TRUE);

	return CPropertyPage::OnSetActive();
}

BOOL InstallType::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case IDC_RADIO_UPGRADE:
		thePreInstall.m_InstallType = 0;
		return TRUE;
	case IDC_RADIO_MODIFY:
		thePreInstall.m_InstallType = 1;
		return TRUE;
	case IDC_RADIO_INSTALL:
		thePreInstall.m_InstallType = 2;
		return TRUE;


	}

	return CPropertyPage::OnCommand(wParam, lParam);
}

HBRUSH
InstallType::OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor)
{
	CBrush brush;
	brush.CreateSolidBrush(RGB(255, 255, 255));
	return brush;
}

LRESULT
InstallType::OnWizardNext()
{
	int checked = m_installnew.GetCheck();
	if (checked)
	{
		int index = property->GetActiveIndex();
		property->SetActivePage(index+1);
	}

	if (checked == IDC_RADIO_UPGRADE)
		thePreInstall.m_InstallType=0;
	if (checked == IDC_RADIO_MODIFY)
		thePreInstall.m_InstallType=1;
	if (checked == IDC_RADIO_INSTALL)
		thePreInstall.m_InstallType=2;

	return CPropertyPage::OnWizardNext();
}


BEGIN_MESSAGE_MAP(InstallType, CPropertyPage)
	
END_MESSAGE_MAP()

