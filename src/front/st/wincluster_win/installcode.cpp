/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: InstallCode.cpp : Cluster setup wizard Installation Identifier property page
** 
** Description:
** 	The Installation Identifier property page displays the Installation ID (for 
** 	example, II) and Install Path (the II_SYSTEM value) of every Ingres 
** 	installation on this node. The user selects a single installation to set up 
** 	for the High Availability Option, aka, Windows cluster failover.
**
** History:
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
** 	03-aug-2004 (wonst02)
** 		Add header comments.
** 	23-nov-2004 (wonst02)
** 	    Cross-integration from main to ingresr30 for Bug 113344.
**	30-nov-2004 (wonst02) Bug 113345
**	    Remove HELP from the property page.
*/

#include "stdafx.h"
#include "wincluster.h"
#include "InstallCode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// WcInstallCode property page

IMPLEMENT_DYNCREATE(WcInstallCode, CPropertyPage)

WcInstallCode::WcInstallCode() : CPropertyPage(WcInstallCode::IDD)
{
	//{{AFX_DATA_INIT(WcInstallCode)
	//}}AFX_DATA_INIT
	m_UserSaidYes = FALSE;
    m_psp.dwFlags &= ~(PSP_HASHELP);
}

WcInstallCode::~WcInstallCode()
{
}

void WcInstallCode::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(WcInstallCode)
	DDX_Control(pDX, IDC_INSTALLATIONCODE, m_code);
	DDX_Control(pDX, IDC_INSTALLEDLIST, m_list);
	DDX_Control(pDX, IDC_IMAGE, m_image);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(WcInstallCode, CPropertyPage)
	//{{AFX_MSG_MAP(WcInstallCode)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTALLEDLIST, OnItemchangedInstalledlist)
	ON_EN_SETFOCUS(IDC_INSTALLATIONCODE, OnSetfocusInstallationcode)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// WcInstallCode message handlers

BOOL WcInstallCode::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	SetList();
	// If there's only one item in the list, select it.
	if (thePreSetup.m_Installations.GetCount() == 1)
	{
		CListCtrl *pList=(CListCtrl *)GetDlgItem(IDC_INSTALLEDLIST);
		int iItem=-1;
		
		if ((iItem=pList->GetNextItem(iItem, LVNI_ALL)) > -1)
		{
			pList->SetItemState(iItem, 1, LVIS_SELECTED);
			pList->SetItemState(iItem, 1, LVIS_FOCUSED);
		}
	}
	m_code.SetLimitText(2);
	return TRUE;
}

BOOL WcInstallCode::OnSetActive() 
{
	property->SetWizardButtons(PSWIZB_NEXT);
	return CPropertyPage::OnSetActive();
}

LRESULT WcInstallCode::OnWizardBack() 
{
	return CPropertyPage::OnWizardBack();
}

LRESULT WcInstallCode::OnWizardNext() 
{
	char code[3];
	char cmd[MAX_PATH+1];
	int flag=0;
	BOOL bUpgrade=FALSE;

	m_code.GetWindowText(code, sizeof(code));
	if(strlen(code)!=2 || !isalpha(code[0]) || !isalnum(code[1]))
	{
		MyError(IDS_INVALIDINSTALLATIONCODE, code);
		return -1;
	}

	m_code.GetWindowText(thePreSetup.m_InstallCode);

	for(int i=0; i<thePreSetup.m_Installations.GetSize(); i++)
	{
		WcInstallation *inst=(WcInstallation *) thePreSetup.m_Installations.GetAt(i);
		if (inst && !thePreSetup.m_InstallCode.CompareNoCase(inst->m_id))
		{
			bUpgrade=TRUE;
			if (inst->m_ver25)
			{
				thePreSetup.m_Ver25 ="1";
			}
			thePreSetup.m_EmbeddedRelease =inst->m_embedded;
			break;
		}
	}

	if (!bUpgrade)
	{	
		MyError(IDS_NOT_INSTALLATION);
		return -1;
	}

	while(IngresAlreadyRunning()==1)
	{
		MessageBeep(MB_ICONEXCLAMATION);
		if(!flag && AskUserYN(IDS_INGRESRUNNING))
		{
			CWaitCursor wait;
			
			flag=1;
			sprintf(cmd, "%s\\ingres\\bin\\winstart.exe", ii_installpath);
			Execute(cmd, "/stop", FALSE);
			while(!WinstartRunning())
				GetParent()->UpdateWindow();
			while(WinstartRunning())
				GetParent()->UpdateWindow();
		}
		else
		{
			GotoDlgCtrl(&m_code);
			return -1;
		}
	}

	sprintf(cmd, "%s\\ingres\\vdba\\ivm.exe", ii_installpath);
	if(GetFileAttributes(cmd) != -1)
		Execute(cmd, "-stop", FALSE);

	if(!m_UserSaidYes &&
		!AskUserYN(IDS_CONTINUE_SETUP))
	{
		GotoDlgCtrl(&m_code);
		return -1;
	}
	m_UserSaidYes = TRUE;

	return CPropertyPage::OnWizardNext();
}

void WcInstallCode::OnItemchangedInstalledlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CListCtrl *pList=(CListCtrl *)GetDlgItem(IDC_INSTALLEDLIST);
	int nSelect=pNMListView->iItem;
	if(nSelect>=0)
	{
		CString strTemp;

		strTemp=pList->GetItemText(nSelect, 0);
		thePreSetup.m_InstallCode =strTemp.Mid(0,2);
		SetDlgItemText(IDC_INSTALLATIONCODE, thePreSetup.m_InstallCode);
	}

	*pResult = 0;
}

void WcInstallCode::OnSetfocusInstallationcode() 
{
	CListCtrl *pList=(CListCtrl *)GetDlgItem(IDC_INSTALLEDLIST);
	int iItem=-1;
	// clear all the selected & focused states	
	while ((iItem=pList->GetNextItem(iItem, LVNI_ALL)) > -1)
	{
		pList->SetItemState(iItem, 0, LVIS_SELECTED);
		pList->SetItemState(iItem, 0, LVIS_FOCUSED);
	}
}

/*
**  Description:
**	Get and display the list of current Ingres installations.
*/
void WcInstallCode::SetList()
{
	LV_ITEM lvitem;
	int index;
	CRect rect;
	CString	strId, strPath;
	strId.LoadString(IDS_INSTALLATIONID);
	strPath.LoadString(IDS_INSTALLPATH);
	
	m_list.GetClientRect(&rect);
	m_list.InsertColumn(0, strId, LVCFMT_LEFT,
		rect.Width() * 1/3, 0);
	m_list.InsertColumn(1, strPath, LVCFMT_LEFT,
		rect.Width() * 2/3, 1);
	
	for(int i=0; i<thePreSetup.m_Installations.GetSize(); i++)
	{
		WcInstallation *inst=(WcInstallation *) thePreSetup.m_Installations.GetAt(i);
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

