
/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: ChooseInstallCode.cpp : implementation file
**
**  Description:
**	This module provides a suggested instance name for the installation.
**  The instance name is derived via algoritm and user has an option of
**  specifying their own instance name if they don't like suggested.
**
**  History:
**  15-Nov-2006 (drivi01)
**		SIR 116599
**		Enhanced pre-installer in effort to improve installer usability.
**  12-Mar-2007 (drivi01)
**		Updated bitmap for info button.
**  12-Sep-2007 (horda03)
**     LaunchWindowsInstaller required 1 parameter.
**	11-Apr-2008 (drivi01)
**	   Add a check at ChooseInstallCode dialog after custom instance name 
**	   is chosen to check if the instance name already being used by
**	   another Ingres instance or DBA Tools, so that user doesn't go
**	   any further thinking they chose unique instance name.
**  08-Aug-2009 (drivi01)
**	Add pragma to remove deprecated POSIX functions warning 4996
**      b/c it is a bug.
*/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning (disable: 4996)


#include "stdafx.h"
#include "enterprise.h"
#include "ChooseInstallCode.h"
#include ".\chooseinstallcode.h"


INT CompareIngresVersion(char *ii_system);
CString CalculateInstallCode();

// ChooseInstallCode dialog

IMPLEMENT_DYNAMIC(ChooseInstallCode, CPropertyPage)
ChooseInstallCode::ChooseInstallCode()
	: CPropertyPage(ChooseInstallCode::IDD)
{
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_INSTNAMETITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_INSTNAMESUBTITLE);
}

ChooseInstallCode::~ChooseInstallCode()
{
}

void ChooseInstallCode::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_DEFAULT, m_default);
	DDX_Control(pDX, IDC_RADIO_CUSTOM, m_custom);
	DDX_Control(pDX, IDC_BUTTON_INFO, m_info);
	DDX_Control(pDX, IDC_EDIT_ID, m_code);
	DDX_Control(pDX, IDC_STATIC_ID, m_stat_id);
}


BEGIN_MESSAGE_MAP(ChooseInstallCode, CPropertyPage)
	ON_EN_CHANGE(IDC_EDIT_ID, OnEnChangeEditId)
	ON_BN_CLICKED(IDC_RADIO_CUSTOM, OnBnClickedRadioCustom)
	ON_BN_CLICKED(IDC_RADIO_DEFAULT, OnBnClickedRadioDefault)
	ON_BN_CLICKED(IDC_BUTTON_INFO, OnBnClickedButtonInfo)
END_MESSAGE_MAP()


// ChooseInstallCode message handlers
BOOL ChooseInstallCode::OnInitDialog()
{
	LOGFONT lf;                        // Used to create the CFont.
	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
    lf.lfHeight = 13; 
	lf.lfWeight=FW_BOLD;
    strcpy(lf.lfFaceName, "Tahoma");    
    m_font_bold.CreateFontIndirect(&lf);    // Create the font.

	GetDlgItem(IDC_RADIO_DEFAULT)->SetFont(&m_font_bold);
	GetDlgItem(IDC_RADIO_CUSTOM)->SetFont(&m_font_bold);

	return CPropertyPage::OnInitDialog();
}
BOOL ChooseInstallCode::OnSetActive()
{
	BOOL bResult = FALSE;
	CString code = CalculateInstallCode();
	m_code.ReplaceSel(code, TRUE);
	m_code.EnableWindow(false);
	m_stat_id.SetWindowText(code);
	if (thePreInstall.m_express == 0)
	{
		this->CheckRadioButton(IDC_RADIO_DEFAULT, IDC_RADIO_CUSTOM, IDC_RADIO_DEFAULT);

		HBITMAP hbitmap = ::LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_BITMAP_INFO_16));
		m_info.SetBitmap(hbitmap);

	
		m_code.SetLimitText(2);

		bResult = CPropertyPage::OnSetActive();
	}
	else
	{
		if (this->OnWizardFinish())
			exit(1);
	}

	return bResult;
}
void ChooseInstallCode::OnEnChangeEditId()
{
	char iicode[3];
	char iicodeUp[3];
	CString str;
	int len;
	memset((char *)iicode, 0, sizeof(iicode));
	m_code.GetLine(0, iicode, 2);
	if ((len = strlen(iicode))>0)
	{
		if ((len == 1 && !isalpha(iicode[0])) || (len == 2 && (!isalpha(iicode[0]) || !isalnum(iicode[1]))))
		{
				Error(IDS_INVALIDINSTALLATIONCODE, iicode);
		}
		else
		{
			if (islower(iicode[0]) || islower(iicode[1]))
			{
			iicodeUp[0] = toupper(iicode[0]);
			iicodeUp[1] = toupper(iicode[1]);
			iicodeUp[2] = iicode[2];
		
			m_code.SetSel(0, -1, 0);
			m_code.ReplaceSel(iicodeUp, TRUE);
			}
		}
	}

}


LRESULT ChooseInstallCode::OnWizardNext()
{
		CString id;
		char  code[4];
	if (GetCheckedRadioButton(IDC_RADIO_DEFAULT, IDC_RADIO_CUSTOM)==IDC_RADIO_DEFAULT)
	{

		GetDlgItemText(IDC_STATIC_ID, id);
		m_code.SetSel(0, -1, 0);
		m_code.ReplaceSel(id, TRUE);
		m_code.GetLine(0, code, 4);
	}

	/*
	** Check if instance name chosen is already taken.
	*/
	m_code.GetWindowText(thePreInstall.m_InstallCode);	
	for(int i=0; i<thePreInstall.m_Installations.GetSize(); i++)
	{
		CInstallation *inst=(CInstallation *) thePreInstall.m_Installations.GetAt(i);
		if (inst && !thePreInstall.m_InstallCode.CompareNoCase(inst->m_id))
		{
			char szData[1024];
			char szKey[256];
			HKEY hKey;
			DWORD dwSize;
			dwSize = sizeof(szData);
			sprintf(szKey, "SOFTWARE\\IngresCorporation\\Ingres\\%s_Installation", inst->m_id);
					if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
					{
						if (RegQueryValueEx(hKey, "dbatools", 0, 0, (BYTE*)szData, &dwSize)==ERROR_SUCCESS)
						{
							if (stricmp(szData, "1")==0)
							{
								Error((UINT)IDS_NAMETAKENBYDBATOOLS, inst->m_id);
								return FALSE;
							}
						}						
						else
						{
								if (!AskUserOC((UINT)IDS_NAMETAKEN, inst->m_id))
									return FALSE;
						}
						RegCloseKey(hKey);
					}
			break;
		}
	}

	if (this->OnWizardFinish())
		exit(1);
	return CPropertyPage::OnWizardNext();
}

/** 
 ** This routine will handle new installs only.
 ** This dialog will only be displayed if new instance is being installed.
 ** The error checking for upgrade/modify is here b/c user may mistakingly pick
 **	a custom id that already 
 **	a custom id that already 
 **	a custom id that already 

 */
BOOL ChooseInstallCode::OnWizardFinish()
{
	char code[3];
	char ach[2048], ii_system[1024];
	int flag=0;
	BOOL bInstalled=FALSE;

	m_code.GetWindowText(code, sizeof(code));
	m_code.GetWindowText(thePreInstall.m_InstallCode);	
	/** The validity of installation id is now being checked as it's being entered.
	 ** At this point installation id will never be of a wrong format.
	 ** Leaving this check for historical reasons, can probaby be pulled out in a future.
	 */
	if(strlen(code)!=2 || !isalpha(code[0]) || !isalnum(code[1]))
	{
		Error(IDS_INVALIDINSTALLATIONCODE, code);
		return FALSE;
	}

    /** This dialog will only ever be activated if we're installing a new instance of
	 ** Ingres.  The routine below is no longer required here.
	 **/
	for(int i=0; i<thePreInstall.m_Installations.GetSize(); i++)
	{
		CInstallation *inst=(CInstallation *) thePreInstall.m_Installations.GetAt(i);
		if (inst && !thePreInstall.m_InstallCode.CompareNoCase(inst->m_id))
		{
			bInstalled=TRUE;
			sprintf(ii_system, "%s", inst->m_path);
			thePreInstall.m_UpgradeType=CompareIngresVersion(ii_system);
			if (inst->m_ver25)
			{
				thePreInstall.m_Ver25 ="1";
				thePreInstall.m_EmbeddedRelease =inst->m_embedded;
			}
			break;
		}
	}

	if (!(bInstalled && (!thePreInstall.m_Ver25.Compare("1") || thePreInstall.m_UpgradeType)))
		goto Label;

	char buf[2048], szInstallCode[3];
	char szKey[256], szResName[256];
	int idx;
	HKEY hKey;
	BOOL bCluster;

	sprintf(szInstallCode, thePreInstall.m_InstallCode.GetBuffer(3));
	if (_stricmp(szInstallCode, "II"))
	{
		/* Compute the GUID index from the installation code */
		idx = (toupper(szInstallCode[0]) - 'A') * 26 + toupper(szInstallCode[1]) - 'A';
		if (idx <= 0)
			idx = (toupper(szInstallCode[0]) - 'A') * 26 + toupper(szInstallCode[1]) - '0';

		sprintf(szKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{A78D%04X-2979-11D5-BDFA-00B0D0AD4485}",idx);
	}
	else 
		sprintf(szKey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{A78D00D8-2979-11D5-BDFA-00B0D0AD4485}");
	thePreInstall.m_InstallCode.ReleaseBuffer();

	bCluster=FALSE;
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey))
	{
		char szData[2];
		DWORD dwSize;

		dwSize=sizeof(szData); *szData=0;
		if (!RegQueryValueEx(hKey,"IngresClusterInstall", NULL, NULL, (BYTE *)szData, &dwSize) && !strcmp(szData, "1"))
			bCluster=TRUE;
		dwSize=sizeof(szResName); *szResName=0;
		if (!RegQueryValueEx(hKey,"IngresClusterResource", NULL, NULL, (BYTE *)szResName, &dwSize))
			sprintf(szResName, "Ingres Service [ %s ]", szInstallCode);
		RegCloseKey(hKey);
	}
				
	SetEnvironmentVariable("II_SYSTEM", ii_system);
	GetEnvironmentVariable("PATH", buf, sizeof(buf));
	sprintf(ach, "%s\\ingres\\bin;%s\\ingres\\utility;%s", ii_system, ii_system, buf);
	SetEnvironmentVariable("PATH", ach);

	/** This routine isn't required in this dialog, however to prevent any problems with installing
	 ** Ingres over another installation which has been corrupted in the registry, the routine will be
	 ** left in here.
	 **/
	while(IngresAlreadyRunning()==1)
	{
		thePreInstall.m_RestartIngres=1;

		MessageBeep(MB_ICONEXCLAMATION);
		if(!flag && AskUserYN(IDS_INGRESRUNNING))
		{
			CWaitCursor wait;

			flag=1;
			if (!bCluster)
			{
				sprintf(ach, "\"%s\\ingres\\bin\\winstart.exe\"", ii_installpath);
				Execute(ach, "/stop", FALSE);
				Sleep(1000);
				while(WinstartRunning())
				{
					Sleep(1000);
					GetParent()->UpdateWindow();
				}
			}
			else
			{
				HCLUSTER hCluster = NULL;
				HRESOURCE hResource = NULL;
				WCHAR lpwResourceName[256];

				hCluster = OpenCluster(NULL);
				if (hCluster)
				{
					mbstowcs(lpwResourceName, szResName, 256);
					hResource = OpenClusterResource(hCluster, lpwResourceName);
					if (hResource)
					{
						OfflineClusterResource(hResource);
						CloseClusterResource(hResource);
					}
				}
				while (IngresAlreadyRunning()==1)
					Sleep(1000);

			}
		}
		else
		{
			GotoDlgCtrl(&m_code);
			return FALSE;
		}
	}

	sprintf(ach, "\"%s\\ingres\\bin\\ivm.exe\"", ii_installpath);
	if (!Execute(ach, "-stop", FALSE))
	{
		sprintf(ach, "\"%s\\ingres\\vdba\\ivm.exe\"", ii_installpath);
		Execute(ach, "-stop", FALSE);
	}

	if(IngresAlreadyRunning()==0)
	{
		if ((thePreInstall.m_UpgradeType ==1 || thePreInstall.m_UpgradeType ==2))
		{
			if (thePreInstall.m_Ver25.Compare("1"))
			{
				if(!AskUserYN(IDS_UPGRADE))
				{
					thePreInstall.m_UpgradeType=0;
				}
			}
			else
			{
				if (!AskUserYN(IDS_MUSTUPGRADEVER25, thePreInstall.m_InstallCode))
				{
					GotoDlgCtrl(&m_code);
					return FALSE;
				}
			}
		}
	}

Label:
	if(!thePreInstall.LaunchWindowsInstaller(!bInstalled))
		exit(0);

	return TRUE;
}
void ChooseInstallCode::OnBnClickedRadioCustom()
{
	if (GetCheckedRadioButton(IDC_RADIO_DEFAULT, IDC_RADIO_CUSTOM)==IDC_RADIO_CUSTOM)
	{
		CWnd *wnd2 = GetDlgItem(IDC_STATIC_INGRES);
		wnd2->EnableWindow(TRUE);
		CWnd *wnd3 = GetDlgItem(IDC_EDIT_ID);
		wnd3->EnableWindow(TRUE);
	}

}

void ChooseInstallCode::OnBnClickedRadioDefault()
{
	if (GetCheckedRadioButton(IDC_RADIO_DEFAULT, IDC_RADIO_CUSTOM)==IDC_RADIO_DEFAULT)
	{
		CWnd *wnd2 = GetDlgItem(IDC_STATIC_INGRES);
		wnd2->EnableWindow(FALSE);
		CWnd *wnd3 = GetDlgItem(IDC_EDIT_ID);
		wnd3->EnableWindow(FALSE);

	}
}

void ChooseInstallCode::OnBnClickedButtonInfo()
{
		InfoDialog infoDlg(0);
		infoDlg.DoModal();
}
