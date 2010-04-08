/*
** Copyright (c) 2006 Ingres Corporation
*/

/*
**
** Name: Summary.cpp - Summary dialog, displays summary and status
**					   of the installation upon its completion.
**
** History:
**	16-Nov-2006 (drivi01)
**	    Created.
**	03-May-2007 (drivi01)
**	    Added another entry for the Summary for Upgrade installs.
**	    Short (non-rtf) summary will notify user if user databases
**	    were upgraded or not.
**  04-Apr-2008 (drivi01)
**		Adding functionality for DBA Tools installer to the Ingres code.
**		DBA Tools installer is a simplified package of Ingres install
**		containing only NET and Visual tools component.  
**		Adjust the wording in the dialog to fit DBA Tools installation.	
**		Note: This change updates the title of the dialog by replacing
**		"Ingres" with "DBA Tools", this will require specials consideration
**		if these tools are ever to be translated.
**  31-Jul-2008 (drivi01)
**	     Cleaned up warnings in Visual Studio 2008.
*/


#include "stdafx.h"
#include "iipostinst.h"
#include "Summary.h"
#include ".\summary.h"

DWORD __stdcall MEditStreamInCallback( DWORD dwCookie,
                                       LPBYTE pbBuff,
                                       LONG cb,
                                       LONG *pcb);
// Summary dialog

IMPLEMENT_DYNAMIC(Summary, CPropertyPage)
Summary::Summary()	: CPropertyPage(Summary::IDD)
{
    m_psp.dwFlags = m_psp.dwFlags & (~PSP_HASHELP);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
	AfxInitRichEdit();
}

Summary::~Summary()
{
}

void Summary::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LAUNCH_PAD, m_launchpad);
	DDX_Control(pDX, IDC_SUMMARY_TITLE1, m_title1);
	DDX_Control(pDX, IDC_SUMMARY_TITLE2, m_title2);
	DDX_Control(pDX, IDC_SUMMARY_TITLE_FAILED, m_titleError);
	DDX_Control(pDX, IDC_STATIC_BLOCK, m_msgblock);
	DDX_Control(pDX, IDC_STATIC_PATH, m_sum_path);
	DDX_Control(pDX, IDC_STATIC_ID, m_sum_id);
	DDX_Control(pDX, IDC_IMAGE1, m_image1);
	DDX_Control(pDX, IDC_IMAGE2, m_image2);
	DDX_Control(pDX, IDC_BUTTON_INFO, m_info);
	DDX_Control(pDX, IDC_RICHEDIT_SUMMARY, m_rtf);
	DDX_Control(pDX, IDC_STATIC_SUCCESS, m_successText);
	DDX_Control(pDX, IDC_STATIC_UDBUPGRADED, m_udbupgraded);
	DDX_Control(pDX, IDC_STATIC_UDBUPGRADED2, m_udbupgraded2);
	DDX_Control(pDX, IDC_STATIC_ERROR, m_errorText);
}



BEGIN_MESSAGE_MAP(Summary, CPropertyPage)
	ON_BN_CLICKED(IDC_BUTTON_INFO, OnBnClickedButtonInfo)
END_MESSAGE_MAP()

BOOL
Summary::OnInitDialog()
{
	BOOL res =  CPropertyPage::OnInitDialog();

	LOGFONT lf;                        // Used to create the CFont.
	CPaintDC dc(this);

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
    lf.lfHeight = 16; 
	lf.lfWeight=FW_BOLD;
    strcpy(lf.lfFaceName, "Tahoma");    
    m_font_bold.CreateFontIndirect(&lf);    // Create the font.

	m_title1.SetFont(&m_font_bold);
	m_titleError.SetFont(&m_font_bold);
	lf.lfHeight = 13;
	m_font_bold_small.CreateFontIndirect(&lf);
	m_title2.SetFont(&m_font_bold_small);


	m_sum_id.SetWindowText(theInstall.m_installationcode);
	int index = theInstall.m_installPath.ReverseFind('\\');
	CString path = theInstall.m_installPath.Mid(0, index);
	m_sum_path.SetWindowText(path);


	return res;
}

BOOL
Summary::OnSetActive()
{
	CString s;
	BOOL res = CPropertyPage::OnSetActive();

	m_launchpad.SetCheck(1);
	HBITMAP hbitmap = ::LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_BITMAP_INFO));
	m_info.SetBitmap(hbitmap);
    if (theInstall.m_DBATools)
	{
		CString title;
		RECT rect;
		s.Format(IDS_TITLE_DBA, theInstall.m_installationcode);
		GetDlgItemText(IDC_STATIC_ID_TITLE, title);
		title.Replace("Ingres", "DBA Tools");
		SetDlgItemText(IDC_STATIC_ID_TITLE, title);
		
		CWnd *wnd1;
		wnd1 = GetDlgItem(IDC_STATIC_ID_TITLE);
		if (wnd1)
		{
			wnd1->GetWindowRect(&rect);
			ScreenToClient(&rect);
			rect.right=rect.right+20;
			wnd1->MoveWindow(&rect, true);
		}

		wnd1 = GetDlgItem(IDC_STATIC_ID);
		if (wnd1)
		{
			wnd1->GetWindowRect(&rect);
			ScreenToClient(&rect);
			rect.left=rect.left+20;
			wnd1->MoveWindow(&rect, TRUE);
		}

		GetDlgItemText(IDC_STATIC_BLOCK, title);
		title.Replace("An Ingres", "A DBA Tools");
		title.Replace("Ingres", "DBA Tools");
		SetDlgItemText(IDC_STATIC_BLOCK, title);
		GetDlgItemText(IDC_STATIC_BLOCK2, title);
		title.Replace("Ingres", "DBA Tools Package");
		SetDlgItemText(IDC_STATIC_BLOCK2, title);
		GetDlgItemText(IDC_STATIC_LOCTITLE, title);
		title.Replace("Ingres", "DBA Tools");
		SetDlgItemText(IDC_STATIC_LOCTITLE, title);
		GetDlgItemText(IDC_STATIC_SUCCESS, title);
		title.Replace("Ingres", "DBA Tools");
		SetDlgItemText(IDC_STATIC_SUCCESS, title);
		GetDlgItemText(IDC_SUMMARY_TITLE_FAILED, title);
		title.Replace("Ingres", "DBA Tools");
		SetDlgItemText(IDC_SUMMARY_TITLE_FAILED, title);
		GetDlgItemText(IDC_SUMMARY_TITLE1, title);
		title.Replace("Ingres", "DBA Tools");
		SetDlgItemText(IDC_SUMMARY_TITLE1, title);

	}
	else	
	s.Format(IDS_TITLE, theInstall.m_installationcode);
	m_udbupgraded.ShowWindow(SW_HIDE);
	m_udbupgraded2.ShowWindow(SW_HIDE);

	if (theInstall.m_CompletedSuccessfully)
	{
		m_image1.ShowWindow(SW_SHOW);
		m_successText.ShowWindow(SW_SHOW);
		m_title1.ShowWindow(SW_SHOW);
		m_image2.ShowWindow(SW_HIDE);
		m_errorText.ShowWindow(SW_HIDE);
		m_titleError.ShowWindow(SW_HIDE);
	}
	else
	{	
		m_image1.ShowWindow(SW_HIDE);
		m_successText.ShowWindow(SW_HIDE);
		m_title1.ShowWindow(SW_HIDE);
		m_image2.ShowWindow(SW_SHOW);
		m_errorText.ShowWindow(SW_SHOW);
		m_titleError.ShowWindow(SW_SHOW);
	}


	CWnd *wnd = GetParent();
	if (wnd)
	{
		CWnd *wnd2 = wnd->GetDlgItem(ID_WIZFINISH);
		if (wnd2)
		{
			wnd2->ShowWindow(SW_SHOW);
			this->GotoDlgCtrl(wnd2);
		}
		wnd2 = wnd->GetDlgItem(ID_WIZNEXT);
		if (wnd2)
			wnd2->ShowWindow(SW_HIDE);
		wnd2 = wnd->GetDlgItem(IDCANCEL);
		if (wnd2)
			wnd2->EnableWindow(0);

	}

	
	if (theInstall.m_express == 0 && theInstall.m_CompletedSuccessfully && !theInstall.m_DBMSupgrade)
	{
			//check if Summary file for advanced install was created by installer.
			char temp[MAX_PATH+1], summary_path[MAX_PATH+1];
			if (GetTempPath(sizeof(temp), temp))
				sprintf(summary_path, "%s\\%s%s%s", temp, "Summary", theInstall.m_installationcode, "-final.rtf");
			else
				sprintf(summary_path, "%s%s%s", "Summary", theInstall.m_installationcode, "-final.rtf");

			//if the Summary file is not in temporary location then simplified summary is displayed
			//the Summary wouldn't be there if this is express installation or upgrade/modify.
			if (_access(summary_path, 00) == 0)
			{
			CWnd *wnd3 = GetDlgItem(IDC_SUMMARY_TITLE2);
			if (wnd3)
			{
				CRect rect, erect;
				wnd3->GetWindowRect(&rect);
				ScreenToClient(&rect);
				rect.top=rect.top-10;
				rect.bottom=rect.bottom-10;
				CWnd *wnd4 = GetDlgItem(IDC_RICHEDIT_SUMMARY);
				if (wnd4)
				{
					wnd4->GetWindowRect(&erect);
					ScreenToClient(&erect);
					rect.left=erect.left;
				}
				wnd3->MoveWindow(&rect);
				
				
			}
			wnd3 = GetDlgItem(IDC_STATIC_LOCTITLE);
			if (wnd3)
				wnd3->ShowWindow(SW_HIDE);
			wnd3 = GetDlgItem(IDC_STATIC_GROUPBOX);
			if (wnd3)
				wnd3->ShowWindow(SW_HIDE);
			wnd3 = GetDlgItem(IDC_STATIC_PATH);
			if (wnd3)
				wnd3->ShowWindow(SW_HIDE);
			wnd3 = GetDlgItem(IDC_STATIC_ID_TITLE);
			if (wnd3)
				wnd3->ShowWindow(SW_HIDE);
			wnd3 = GetDlgItem(IDC_STATIC_ID);
			if (wnd3)
				wnd3->ShowWindow(SW_HIDE);
			wnd3 = GetDlgItem(IDC_STATIC_BLOCK);
			if (wnd3)
				wnd3->ShowWindow(SW_HIDE);
			wnd3 = GetDlgItem(IDC_STATIC_BLOCK2);
			if (wnd3)
				wnd3->ShowWindow(SW_HIDE);
	

			CFile file(summary_path, CFile::modeRead);
			EDITSTREAM es;
			m_rtf.ShowWindow(SW_SHOW);
	
			es.dwCookie = (DWORD)&file;
			es.pfnCallback = MEditStreamInCallback;
	
			m_rtf.StreamIn(SF_RTF,es);
			}


	}  
	else if(theInstall.m_attemptedUDBupgrade && theInstall.m_CompletedSuccessfully)
	{
		if (theInstall.m_upgradedatabases)
			m_udbupgraded.ShowWindow(SW_SHOW);
		else
			m_udbupgraded2.ShowWindow(SW_SHOW);
	}
	


	return res;
}

DWORD __stdcall MEditStreamInCallback( DWORD dwCookie,
                                       LPBYTE pbBuff,
                                       LONG cb,
                                       LONG *pcb)
{
   CFile* pFile = (CFile*) dwCookie;
   *pcb = pFile->Read(pbBuff, cb);

  return 0;
}


BOOL Summary::OnWizardFinish()
{
	    CComponent *online_doc = theInstall.GetOnLineDoc();


#ifdef EVALUATION_RELEASE
		Message(IDS_SDKSUCCEEDED);

		if (theInstall.m_ApacheInstalled)
		{
		    char szBuf[MAX_PATH];
		    CString t(theInstall.m_installPath); t.Replace("\\", "/");

		    sprintf(szBuf, "Include \"%s/ingres/ice/bin/apache/ice.conf\"", t);
		    Message(IDS_APACHEINSTALLED, szBuf);
		}
#else
/*		Message(IDS_SUCCEEDED, theInstall.m_installationcode);
		
		if (!theInstall.m_HTTP_Server.CompareNoCase("Apache"))
		{
		    char szBuf[MAX_PATH];
		    CString t(theInstall.m_installPath); t.Replace("\\", "/");

		    sprintf(szBuf, "Include \"%s/ingres/ice/bin/apache/ice.conf\"", t);
		    Message(IDS_APACHEINSTALLED2, szBuf);
		}
		*/
#endif /* EVALUATION_RELEASE */
		

		SetWindowPos( &CWnd::wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		if (online_doc && online_doc->m_selected)
		{
		    if (!theInstall.IsCurrentAcrobatAlreadyInstalled())
		    {
			char locale_lang[32];
			
			GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, 
			              LOCALE_SENGLANGUAGE, 
			              locale_lang, 
			              sizeof(locale_lang));
			if (!strcmp(locale_lang,"English"))
			{
			    if (AskUserYN(IDS_INSTALL_ACROBAT))
				theInstall.OpenPageForAcrobat();
				//theInstall.InstallAcrobat();
			}
			else
			{
			    if (AskUserYN(IDS_INSTALL_ENG_ACROBAT))
				theInstall.OpenPageForAcrobat();
					//theInstall.InstallAcrobat();
			    else 
				Message(IDS_DOWNLOAD_NONENG_ADOBE);
			}
		    }
		    else
		    {
			CString s=theInstall.m_installPath+"\\ingres\\temp\\AdbeRdr707_DLM_en_US.exe";
			}
		}
		/* Start up IVM */
		if (theInstall.m_StartIVM)
		{
		    SetCurrentDirectory(theInstall.m_installPath + "\\ingres\\bin");
		    theInstall.Execute( "\"" + theInstall.m_installPath + "\\ingres\\bin\\ivm.exe\"", FALSE);
		}
		BOOL reboot=theInstall.GetRegValueBOOL("RebootNeeded", FALSE);
		theInstall.DeleteRegValue("RebootNeeded");
		if (reboot)
		    theInstall.m_reboot = AskUserYN(IDS_REBOOT);

	return CPropertyPage::OnWizardFinish();
}

void Summary::OnBnClickedButtonInfo()
{
		InfoDialog infoDlg(0);
		infoDlg.DoModal();
}
