/*
**  Copyright (c) 2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: InstallCode.cpp : implementation file
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	11-Jun-2001 (penga03)
**	    Changed the text in the Finish button to "Next".
**	13-jun-2001 (somsa01)
**	    Re-modified the Finish button text, and removed
**	    Help button.
**	14-Jun-2001 (penga03)
**	    The column of "Installation" lists only the values of
**	    installation codes.
**	23-July-2001 (penga03)
**	    List the installation installed by old Ingres installers.
**	23-July-2001 (penga03)
**	    Before start installing, shut down ingres and ivm.
**	20-aug-2001 (somsa01)
**	    Changed column heading from "Installation" to "Installation ID".
**	14-Sep-2001 (penga03)
**	    Supports an upgrade from II 2.6/0106.
**	21-oct-2002 (penga03)
**	    Put the text of the Finish button into the String table, so 
**	    that it can be localized.
**	    Check whether Ingres is running or not ONLY if this is an 
**	    upgrade.
**	18-oct-2004 (penga03)
**	    Changed vdba directory from ingres\vdba to ingres\bin.
**	20-oct-2004 (penga03)
**	    When shutting down ivm if we cannot find ivm from ingres\bin,
**	    check ingres\vdba, which means this is an upgrade from 26 or 
**	    previous versions.
**	13-dec-2004 (penga03)
**	    Modified OnWizardFinish() to get the upgrade type based on the
**	    chosen installation identifier.
**	15-dec-2004 (penga03)
**	    In OnWizardFinish(), instead of waiting for winstart start up,
**	    sleep one second. And while winstart shutting down Ingres, 
**	    sleep half a second each time updating the parent window. This 
**	    will fix the problem that install.exe takes 90% CPU time.
**	16-dec-2004 (penga03)
**	    Corrected the error when stopping ivm.
**	17-dec-2004 (penga03)
**	    Instead of using ingwrap, we set environment variables.
**	14-march-2005 (penga03)
**	    Shut down Ingres by taking the resource offline in a cluster 
**	    environment.
**	30-jun-2005 (penga03)
**	    Added two more controls to stop Ingres. 
**	01-Mar-2006 (drivi01)
**	    Reused and adopted for MSI Patch Installer on Windows.
**  21-Mar-2007 (wonca01) BUG 117968
**      Change CInstallCode::OnInitDialog() to initialize the default 
**      m_code to an existing II_INSTALLATION on the server.
*/

#include "stdafx.h"
#include "enterprise.h"
#include "InstallCode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

INT CompareIngresVersion(char *ii_system);

/////////////////////////////////////////////////////////////////////////////
// CInstallCode property page

IMPLEMENT_DYNCREATE(CInstallCode, CPropertyPage)

CInstallCode::CInstallCode() : CPropertyPage(CInstallCode::IDD)
{
	//{{AFX_DATA_INIT(CInstallCode)
	//}}AFX_DATA_INIT

    m_psp.dwFlags &= ~(PSP_HASHELP);
}

CInstallCode::~CInstallCode()
{
}

void CInstallCode::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInstallCode)
	DDX_Control(pDX, IDC_INSTALLATIONCODE, m_code);
	DDX_Control(pDX, IDC_INSTALLEDLIST, m_list);
	DDX_Control(pDX, IDC_IMAGE, m_image);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInstallCode, CPropertyPage)
	//{{AFX_MSG_MAP(CInstallCode)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTALLEDLIST, OnItemchangedInstalledlist)
	ON_EN_SETFOCUS(IDC_INSTALLATIONCODE, OnSetfocusInstallationcode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInstallCode message handlers

BOOL CInstallCode::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	CInstallation *inst=NULL;
	
	SetList();
	m_code.SetLimitText(2);
	inst = (CInstallation *)thePreInstall.m_Installations.GetAt(0);
	if (inst)
		m_code.SetWindowText(inst->m_id);
	else
		m_code.SetWindowText("II");
	return TRUE;
}

LRESULT CInstallCode::OnWizardBack() 
{
	return CPropertyPage::OnWizardBack();
}

BOOL CInstallCode::OnSetActive() 
{
	property->SetWizardButtons(PSWIZB_FINISH);
	CString s;
	s.LoadString(IDS_FINISH_TEXT);
	property->SetFinishText(s);
	return CPropertyPage::OnSetActive();
}

/*
**	History:
**	23-July-2001 (penga03)
**	   List the installation installed by old Ingres installers. 
**	14-Sep-2001 (penga03)
**	   If "careglog.log" exists in II_SYSTEM, then this version of Ingres 
**	   was installed by old installer.
**	05-nov-2001 (penga03)
**	    List all the installations found by FindOldInstallations() and saved 
**	    in m_Installations.
*/
void CInstallCode::SetList()
{
	LV_ITEM lvitem;
	int index;
	CRect rect;
	
	m_list.GetClientRect(&rect);
	m_list.InsertColumn(0, "Installation ID", LVCFMT_LEFT,
		rect.Width() * 1/3, 0);
	m_list.InsertColumn(1, "Install Path (II_SYSTEM)", LVCFMT_LEFT,
		rect.Width() * 2/3, 1);
	
	for(int i=0; i<thePreInstall.m_Installations.GetSize(); i++)
	{
		CInstallation *inst=(CInstallation *) thePreInstall.m_Installations.GetAt(i);
		if(inst)
		{
			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=i;
			lvitem.iSubItem=0;
			lvitem.pszText=inst->m_id.GetBuffer(2);
			index = m_list.InsertItem(&lvitem);
			
			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=index;
			lvitem.iSubItem=1;
			lvitem.pszText=inst->m_path.GetBuffer(MAX_PATH);
			m_list.SetItem(&lvitem);
		}
	}
}

/*
**	History:
**	23-July-2001 (penga03)
**	    If there exists an installation installed by old installer, ask 
**	    user if upgrade this installation.
**	23-July-2001 (penga03)
**	    Before start installing, shut down ingres and ivm.
**	05-nov-2001 (penga03)
**	    Since we support upgrading multiple installations, there is no point 
**	    to ask user if he/she wants to upgrade the installation or not. All 
**	    we need to do is compare the installation id that user inputs with 
**	    the ids already found, and according to the result, set the property 
**	    INGRES_VER25 and pass it to MsiExec.exe.
**	30-jul-2002 (penga03)
**	    If the Ingres instance being upgraded was embedded, 
**	    keep the way it was.
**  01-apr-2008 (wonca01) Bug 120024
**      Check return code for IngresAlreadyRunning()
*/
BOOL CInstallCode::OnWizardFinish() 
{
	char code[3];
	char ach[2048], ii_system[1024];
	int flag=0;
	BOOL bInstalled=FALSE;


	char buf[2048], szInstallCode[3];
	char szKey[256], szResName[256];
	HKEY hKey;
	int idx;
	BOOL bCluster;
	int rc;

	/* verify installation code entered in a dialog is a valid installation id */
	m_code.GetWindowText(code, sizeof(code));
	if(strlen(code)!=2 || !isalpha(code[0]) || !isalnum(code[1]))
	{
		Error(IDS_INVALIDINSTALLATIONCODE, code);
		return FALSE;
	}

	m_code.GetWindowText(thePreInstall.m_InstallCode);
    /* retrieve ii_system for the installation we're upgrading */
	for(int i=0; i<thePreInstall.m_Installations.GetSize(); i++)
	{
		CInstallation *inst=(CInstallation *) thePreInstall.m_Installations.GetAt(i);
		if (inst && !thePreInstall.m_InstallCode.CompareNoCase(inst->m_id))
		{
			bInstalled=TRUE;
			sprintf(ii_system, "%s", inst->m_path);
			thePreInstall.m_UpgradeType=CompareIngresVersion(ii_system);
			break;
		}
	}

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

	/* figure out if this is a cluster install, b/c we'll need to shut it down */
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
	
	rc = IngresAlreadyRunning();
	/* shut down ingres if it's running */
	while( rc ==1)
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
		rc = IngresAlreadyRunning();
	}
	
	if (rc == 2)
		exit(1);

	sprintf(ach, "\"%s\\ingres\\bin\\ivm.exe\"", ii_installpath);
	if (!Execute(ach, "-stop", FALSE))
	{
		sprintf(ach, "\"%s\\ingres\\vdba\\ivm.exe\"", ii_installpath);
		Execute(ach, "-stop", FALSE);
	}


	if(thePreInstall.LaunchPatchInstaller() != ERROR_SUCCESS)
		exit(0);

	return TRUE;
}

void CInstallCode::OnItemchangedInstalledlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CListCtrl *pList=(CListCtrl *)GetDlgItem(IDC_INSTALLEDLIST);
	int nSelect=pNMListView->iItem;
	if(nSelect>=0)
	{
		CString strTemp;

		strTemp=pList->GetItemText(nSelect, 0);
		thePreInstall.m_InstallCode =strTemp.Mid(0,2);
		SetDlgItemText(IDC_INSTALLATIONCODE, thePreInstall.m_InstallCode);
	}

	*pResult = 0;
}

void CInstallCode::OnSetfocusInstallationcode() 
{
	CListCtrl *pList=(CListCtrl *)GetDlgItem(IDC_INSTALLEDLIST);
	int iItem=-1;
	
	while((iItem=pList->GetNextItem(iItem, LVNI_ALL)) > -1)
	{
		pList->SetItemState(iItem, 0, LVIS_SELECTED);
		pList->SetItemState(iItem, 0, LVIS_FOCUSED);
	}
}
