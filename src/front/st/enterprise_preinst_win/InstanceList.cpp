/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: InstanceList.cpp : implementation file
**
**  Description:
**	This module will only display in upgrade/modify scenario.
**  When user selects Upgrade Install Type, this module will
**  scan through the list of existing installations and
**  select/display only installations that can be upgraded to the
**  current release.  When user selects Modify Install Type, 
**  this module will scan through the list of existing 
**  installations and select/display only installations that
**  are of the same version as the current release.
**
**  History:
**  15-Nov-2006 (drivi01)
**	SIR 116599
**	Enhanced pre-installer in effort to improve installer usability.
**  06-Dec-2006 (drivi01)
**	Changed "Installation ID" column name to "Instance Name" to 
**	reflect terminology change.
**  06-Mar-2007 (drivi01)
**	Changed title and subtitle of this dialog, they were from
**	"Installation Type" dialog.
**  22-Jun-2007 (horda03)
**  LaunchWindowsInstaller() needs clean up the environment for the installation code
**  if this is not an upgrade to an existing installation.
**  04-Apr-2008 (drivi01)
**  Adding functionality for DBA Tools installer to the Ingres code.
**  DBA Tools installer is a simplified package of Ingres install
**  containing only NET and Visual tools component.  
**	DBA Tools is going to use InstanceList dialog when there's multiple
**	installations to choose from in upgrade scenario.
**  17-Sep-2009 (drivi01)
**	Add code to block upgrade of Ingres releases before 9.2
**	if the character set on those installations is UTF8.
**  07-Oct-2009 (drivi01)
**	Fix IsEmpty function call.
*/


#include "stdafx.h"
#include "enterprise.h"
#include "InstanceList.h"
#include ".\instancelist.h"


int CompareIngresVersion(char *);
int IsPre92Release(CString);
// InstanceList dialog

IMPLEMENT_DYNAMIC(InstanceList, CPropertyPage)
InstanceList::InstanceList()
	: CPropertyPage(InstanceList::IDD)
{

	m_psp.dwFlags &= ~(PSP_HASHELP);
	m_psp.dwFlags |= PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
	m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_INSTLISTTITLE);
	m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_INSTLISTSUBTITLE);
	

	/*
	** Update title, and subtitle in the dialog for DBA Tools
	*/ 
	if (thePreInstall.m_DBATools)
	{
		m_psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_INSTLISTTITLE2);
		m_psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_INSTLISTSUBTITLE2);
	}
}

InstanceList::~InstanceList()
{
}

void InstanceList::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInstallCode)
	DDX_Control(pDX, IDC_INSTANCE_LIST, m_list);
	DDX_Control(pDX, IDC_CHECK_AUTOUPGRADE, m_autoupgrade);
	DDX_Control(pDX, IDC_STATIC_AUTOID, m_autoid);
	DDX_Control(pDX, IDC_STATIC_AUTOR, m_autorecommend);
	DDX_Control(pDX, IDC_STATIC1, m_text1);
	DDX_Control(pDX, IDC_STATIC2, m_text2);
	DDX_Control(pDX, IDC_STATIC3, m_text3);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(InstanceList, CPropertyPage)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTANCE_LIST, OnLvnItemchangedInstanceList)
END_MESSAGE_MAP()


BOOL
InstanceList::OnSetActive()
{
	m_list.DeleteAllItems();
	m_list.DeleteColumn(2);
	m_list.DeleteColumn(1);
	m_list.DeleteColumn(0);
	SetList();

	/*
	** If there's only one item in the list, select it by default.
	** And update text under upgradedb checkbox
	*/
	if (thePreInstall.m_Installations.GetSize() == 1)
		m_list.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	
	
	if (thePreInstall.m_InstallType == 0)
	{
		m_autoupgrade.SetCheck(1);
		m_autoupgrade.ShowWindow(SW_SHOW);
		m_autoid.ShowWindow(SW_SHOW);
		if (thePreInstall.m_Installations.GetSize() > 1)
			m_autoid.SetWindowText("Ingres \?\?");
		m_autorecommend.ShowWindow(SW_SHOW);
		CRect rect1, rect2, rect3;
		m_autoupgrade.GetWindowRect(&rect1);
		m_autoid.GetWindowRect(&rect2);
		m_autorecommend.GetWindowRect(&rect3);
		ScreenToClient(&rect1);
		ScreenToClient(&rect2);
		ScreenToClient(&rect3);
		rect2.left=rect1.right-1;
		rect2.right=rect2.left+rect2.Width();
		rect3.left=rect2.right;
		rect3.right=rect3.left+rect3.Width();
		m_autoid.MoveWindow(&rect2);
		m_autorecommend.MoveWindow(&rect3);

	}
	else
	{
		m_autoupgrade.ShowWindow(SW_HIDE);
		m_autoid.ShowWindow(SW_HIDE);
		m_autorecommend.ShowWindow(SW_HIDE);
	}

	/*
	** Format the InstanceList dialog for DBA Tools
	** purposes, replace "Ingres" with "DBA Tools"
	** or something to that effect and disable
	** some controls that do not make sense in a context
	** of DBA Tools.
	*/
	if (thePreInstall.m_DBATools)
	{
		CString text;
		m_text1.GetWindowText(text);
		text.Replace("Ingres", "DBA Tools Package");
		m_text1.SetWindowText((LPCSTR )text);
		m_text2.GetWindowText(text);
		text.Replace("an Ingres", "a DBA Tools");
		m_text2.SetWindowText((LPCSTR )text);
		m_text3.GetWindowText(text);
		text.Replace("Ingres", "DBA Tools");
		m_text3.SetWindowText((LPCSTR )text);
		m_autoupgrade.ShowWindow(SW_HIDE);
		m_autoid.ShowWindow(SW_HIDE);
		m_autorecommend.ShowWindow(SW_HIDE);
	}
	return CPropertyPage::OnSetActive();
}

void 
InstanceList::SetList()
{
	LV_ITEM lvitem;
	int index;
	CRect rect;
	
	m_list.GetClientRect(&rect);
	m_list.InsertColumn(0, "Instance Name", LVCFMT_LEFT, rect.Width() * 1/4, 0);
	m_list.InsertColumn(1, "Version", LVCFMT_LEFT, rect.Width() * 1/4, 1);
	m_list.InsertColumn(2, "Install Path (II_SYSTEM)", LVCFMT_LEFT,	rect.Width() * 2/4, 2);
	m_list.SetExtendedStyle(LVS_EX_FLATSB |LVS_EX_FULLROWSELECT |LVS_EX_GRIDLINES );
	for(int i=0; i<thePreInstall.m_Installations.GetSize(); i++)
	{
		CInstallation *inst=(CInstallation *) thePreInstall.m_Installations.GetAt(i);
		char ii_system[MAX_PATH];
		sprintf(ii_system, inst->m_path, inst->m_path.GetLength());
		int ver = CompareIngresVersion(ii_system);
		int itype = thePreInstall.m_InstallType;
		CString install_id;  //DBA Tools should show instance as "DBA Tools" instance
		if (thePreInstall.m_DBATools)
			install_id = "DBA Tools "+inst->m_id;
		else
			install_id = "Ingres "+inst->m_id;
		if(inst && !thePreInstall.m_DBATools && 
			((itype == 0 && (ver == 2)) || ( itype == 1 && (ver == 0 || ver == 3))) && !inst->m_isDBATools)
		{
			/* 
			** This block will display existing installations for Ingres only installs and
			** will not include existing DBA Tools installations in the list since they can't
			** be upgraded.
			*/
			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=i;
			lvitem.iSubItem=0;
			lvitem.pszText=install_id.GetBuffer(9);
			index = m_list.InsertItem(&lvitem);
			
			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=index;
			lvitem.iSubItem=1;
			lvitem.pszText=inst->m_ReleaseVer.GetBuffer(15);
			m_list.SetItem(&lvitem);

			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=index;
			lvitem.iSubItem=2;
			lvitem.pszText=inst->m_path.GetBuffer(MAX_PATH);
			m_list.SetItem(&lvitem);
		}
		else if (inst && thePreInstall.m_DBATools 
			&& itype==0 && inst->m_UpgradeCode>0 && inst->m_isDBATools)
		{
			/*
			** This block will display existing DBA Tools installations for DBA Tools install
			** and will filter out any existing Ingres installations.
			*/
			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=i;
			lvitem.iSubItem=0;
			lvitem.pszText=install_id.GetBuffer(9);
			index = m_list.InsertItem(&lvitem);
			
			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=index;
			lvitem.iSubItem=1;
			lvitem.pszText=inst->m_ReleaseVer.GetBuffer(15);
			m_list.SetItem(&lvitem);

			lvitem.mask=LVIF_TEXT;
			lvitem.iItem=index;
			lvitem.iSubItem=2;
			lvitem.pszText=inst->m_path.GetBuffer(MAX_PATH);
			m_list.SetItem(&lvitem);
		}
	}
}

LRESULT
InstanceList::OnWizardNext()
{
	int index = property->GetActiveIndex();
	int count = property->GetPageCount();
	for (int i=count-1; i>index; i--)
		property->RemovePage(property->GetPage(i));


	if (m_autoupgrade.GetCheck())
		thePreInstall.m_UpgradeDatabases=1;
	else
		thePreInstall.m_UpgradeDatabases=0;

	if (m_list.GetFirstSelectedItemPosition() == NULL)
	{
		this->GotoDlgCtrl(&m_list);
		AfxMessageBox(IDS_MUSTCHOOSE, MB_OK);
		return NULL;
	}
	else
	{
		if (this->OnWizardFinish())
			exit(1);
		return CPropertyPage::OnWizardNext();
	}
}
LRESULT
InstanceList::OnWizardBack()
{
	return CPropertyPage::OnWizardBack();
}


/** 
 ** This routine will handle upgrade/modify installs only.
 ** This dialog will only be displayed if ingres is being modified or upgraded.
 */
BOOL
InstanceList::OnWizardFinish()
{
	char code[3];
	char ach[2048], ii_system[1024];
	int flag=0;
	BOOL bInstalled=FALSE;

	sprintf(code, "%s", m_iicode);
	thePreInstall.m_InstallCode=m_iicode;
	if(strlen(code)!=2 || !isalpha(code[0]) || !isalnum(code[1]))
	{
		Error(IDS_INVALIDINSTALLATIONCODE, code);
		return FALSE;
	}

	CInstallation *inst=NULL;
	for(int i=0; i<thePreInstall.m_Installations.GetSize(); i++)
	{
		inst=(CInstallation *) thePreInstall.m_Installations.GetAt(i);
		if (inst && !thePreInstall.m_InstallCode.CompareNoCase(inst->m_id))
		{
			bInstalled=TRUE;
			sprintf(ii_system, "%s", inst->m_path);
			thePreInstall.m_UpgradeType=CompareIngresVersion(ii_system);
			if (thePreInstall.m_DBATools)
				thePreInstall.m_UpgradeType=inst->m_UpgradeCode;
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
			GotoDlgCtrl(&m_list);
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
		if ((thePreInstall.m_UpgradeType ==1 || thePreInstall.m_UpgradeType ==2) && 			!thePreInstall.m_DBATools)
		{
			CString charset;
			if (IsPre92Release(inst->m_ReleaseVer))
			{
			Local_NMgtIngAt("II_CHARSET", inst->m_path, charset);
			if (charset.IsEmpty())
			{
				char charsetinst[MAX_PATH];
				sprintf(charsetinst, "II_CHARSET%s", inst->m_id);
				Local_NMgtIngAt(charsetinst, inst->m_path, charset);
			}
			if (!charset.IsEmpty() && charset.CompareNoCase("UTF8")==0)
			{
				Error(IDS_BLOCKUPGRADE, code);
				return FALSE;
			}
			}
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
					GotoDlgCtrl(&m_list);
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

void InstanceList::OnLvnItemchangedInstanceList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	int nSelect=pNMListView->iItem;
	*pResult = 0;

	POSITION pos = m_list.GetFirstSelectedItemPosition();
	if (pos!=NULL)
	{
		if(nSelect>=0)
		{
			CString strTemp;
			strTemp = m_list.GetItemText(nSelect, 0);
			m_iicode = strTemp.Mid(sizeof("Ingres"+1),2);
			m_autoid.SetWindowText(strTemp);
			if (thePreInstall.m_DBATools)
			{
				//Strip out installation id from the installation line chosen in a dialog
				if (strTemp.CompareNoCase("DBA Tools")==0)
					m_iicode = "VT";
				else
					m_iicode = strTemp.Mid(sizeof("DBA Tools"+1), 2);
			}
		}
	}
	else
	{
		m_iicode.Empty();
		m_autoid.SetWindowText("Ingres \?\?");
	}
	
}
