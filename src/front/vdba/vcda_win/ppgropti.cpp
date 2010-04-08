/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ppgropti.cpp : implementation file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Restore: Option od restore (can be final step) of restore operation
**
** History:
**
** 18-Mar-2003 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 03-May-2005 (komve01)
**    Issue#13921873/Bug#114438, Added message boxes for VNode Definitions 
**    and Restore Parameters and Variables with paths.
** 06-Jun-2005 (lakvi01)
**    If none of the check boxes are selected (in the screen 1 of the Restore wizard)
**	  disable the 'Next' button. For this purpose added OnBtnCheckConfigParams(),
**	  OnBtnCheckSysVariables(), OnBtnCheckUserVariables(), EnableNextButton() functions.
** 16-Jun-2005 (drivi01)
**    Removed <<<< symbols at the end of the file, looks like bad propagation.
** 08-Sep-2008 (drivi01)
**    Updated wizard to use Wizard97 style and updated smaller
**    wizard graphic.
*/


#include "stdafx.h"
#include "vcda.h"
#include "pshresto.h"
#include "ppgropti.h"
#include "rctools.h"  // Resource symbols of rctools.rc
#include "ingobdml.h"
#include "environm.h"
#include "constdef.h"
#include <io.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CuPropertyPageRestoreOption property page

IMPLEMENT_DYNCREATE(CuPropertyPageRestoreOption, CPropertyPage)

CuPropertyPageRestoreOption::CuPropertyPageRestoreOption() : CPropertyPage(CuPropertyPageRestoreOption::IDD)
{
	//{{AFX_DATA_INIT(CuPropertyPageRestoreOption)
	m_bRestoreConfig = TRUE;
	m_bRestoreSystemVariable = TRUE;
	m_bRestoreVirtualNode = FALSE;
	m_bRestoreUserVariable = TRUE;
	m_bNotResorePath = TRUE;
	m_strBackupFile = _T("");
	//}}AFX_DATA_INIT
	m_bitmap.SetCenterVertical(TRUE);
	m_bFinalStep = TRUE; // Can have the Finish Button
	m_bNoPrevious = TRUE;// Can have the Next Button
	m_psp.dwFlags |= PSP_DEFAULT|PSP_HIDEHEADER;
}

CuPropertyPageRestoreOption::~CuPropertyPageRestoreOption()
{
}

void CuPropertyPageRestoreOption::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuPropertyPageRestoreOption)
	DDX_Control(pDX, IDC_STATIC1, m_cStaticNonEditVariableInfo);
	DDX_Control(pDX, IDC_LIST1, m_cListROVariable);
	DDX_Control(pDX, IDC_EDIT1, m_cEditBackup);
	DDX_Control(pDX, IDC_CHECK5, m_cCheckNotRestorePath);
	DDX_Control(pDX, IDC_CHECK4, m_cCheckUserVariable);
	DDX_Control(pDX, IDC_CHECK3, m_cCheckVNode);
	DDX_Control(pDX, IDC_CHECK2, m_cCheckSystemVariable);
	DDX_Control(pDX, IDC_CHECK1, m_cCheckParameter);
	DDX_Control(pDX, IDC_STATIC_IDI_WARNING, m_cIconWarning);
	DDX_Control(pDX, IDC_STATIC_IMAGE, m_bitmap);
	DDX_Check(pDX, IDC_CHECK1, m_bRestoreConfig);
	DDX_Check(pDX, IDC_CHECK2, m_bRestoreSystemVariable);
	DDX_Check(pDX, IDC_CHECK3, m_bRestoreVirtualNode);
	DDX_Check(pDX, IDC_CHECK4, m_bRestoreUserVariable);
	DDX_Check(pDX, IDC_CHECK5, m_bNotResorePath);
	DDX_Text(pDX, IDC_EDIT1, m_strBackupFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuPropertyPageRestoreOption, CPropertyPage)
	//{{AFX_MSG_MAP(CuPropertyPageRestoreOption)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CHECK3, OnBtnCheckVNodeDefs)
	ON_BN_CLICKED(IDC_CHECK5, OnBtnCheckParams)

	ON_BN_CLICKED(IDC_CHECK1, OnBtnCheckConfigParams)
	ON_BN_CLICKED(IDC_CHECK2, OnBtnCheckSysVariables)
	ON_BN_CLICKED(IDC_CHECK4, OnBtnCheckUserVariables)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuPropertyPageRestoreOption message handlers

BOOL CuPropertyPageRestoreOption::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	CxPSheetRestore* pParent = (CxPSheetRestore*)GetParent();
	CaRestoreParam& Restore = pParent->GetData();

	HICON hIcon = theApp.LoadStandardIcon(IDI_EXCLAMATION);
	m_cIconWarning.SetIcon(hIcon);
	m_cIconWarning.Invalidate();
	DestroyIcon(hIcon);

#if !defined (_VIRTUAL_NODE_AVAILABLE)
	m_cCheckVNode.ShowWindow(SW_HIDE);
#endif

	CTypedPtrList<CObList, CaNameValue*> listEnvirenment;
	CaNameValue* pEnv = new CaNameValue(_T("II_TEMPORARY"), _T(""));
	listEnvirenment.AddTail(pEnv);
	INGRESII_CheckVariable (listEnvirenment);

	CString strBackupFile = pEnv->GetValue();
	if (!strBackupFile.IsEmpty())
	{
		strBackupFile += consttchszPathSep;
	}

	int i = 0;
	CString strFile = strBackupFile + _T("backup.ii_vcda");
	for (i=0; i<1024; i++)
	{
		if (i==0)
		{
			if (_taccess(strFile, 0) == -1)
				break;
		}
		else
		{
			strFile.Format (_T("backup%04d.ii_vcda"), i);
			strFile = strBackupFile + strFile;
			if (_taccess(strFile, 0) == -1)
				break;
		}
	}
	if (i == 1024)
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_TOOMANAY_BACKUPFILE, (LPCTSTR)pEnv->GetValue());
		AfxMessageBox (strMsg);
	}
	else
	{
		m_cEditBackup.SetWindowText(strFile);
	}

	while (!listEnvirenment.IsEmpty())
		delete listEnvirenment.RemoveHead();

	//
	// Initialize the list of READONLY variables:
	BOOL bVariableDiff = FALSE;
	CString strII = INGRESII_QueryInstallationID(FALSE);
	CTypedPtrList< CObList, CaCdaDifference* >* pLd = Restore.m_plistDifference;
	if (pLd)
	{
		POSITION pos = pLd->GetHeadPosition();
		while (pos != NULL)
		{
			CaCdaDifference* pDiff = pLd->GetNext(pos);
			if (pDiff->GetType() == CDA_ENVSYSTEM || pDiff->GetType() == CDA_ENVUSER)
			{
				if (CaIngresVariable::IsReadOnly(pDiff->GetOriginalName(), strII))
				{
					m_cListROVariable.AddString(pDiff->GetName());
					bVariableDiff = TRUE;
				}
			}
		}
	}

	if (!bVariableDiff)
	{
		HICON hIcon = theApp.LoadStandardIcon(IDI_ASTERISK);
		m_cIconWarning.SetIcon(hIcon);
		m_cIconWarning.Invalidate();
		DestroyIcon(hIcon);
		m_cListROVariable.ShowWindow(SW_HIDE);
		CString strMsgInfo;
		strMsgInfo.LoadString(IDS_MSG_VARIABLE_RO);
		m_cStaticNonEditVariableInfo.SetWindowText(strMsgInfo);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CuPropertyPageRestoreOption::OnSetActive() 
{
	CxPSheetRestore* pParent = (CxPSheetRestore*)GetParent();
	UINT nMode = m_bFinalStep? PSWIZB_FINISH: PSWIZB_NEXT;
	if (m_bNoPrevious)
		nMode |= PSWIZB_BACK;

	pParent->SetWizardButtons(nMode);
	return CPropertyPage::OnSetActive();
}

LRESULT CuPropertyPageRestoreOption::OnWizardNext() 
{
	CxPSheetRestore* pParent = (CxPSheetRestore*)GetParent();
	CaRestoreParam& Restore = pParent->GetData();
	UpdateData (TRUE);
	if (!CheckOverwriteBackupFile())
		return E_FAIL;

	Restore.Cleanup();
	Restore.SetBackup(m_strBackupFile);
	Restore.SetConfig(m_bRestoreConfig);
	Restore.SetSystemVariable(m_bRestoreSystemVariable);
	Restore.SetUserVariable(m_bRestoreUserVariable);
	Restore.SetVirtualNode(m_bRestoreVirtualNode);
	Restore.SetPath(!m_bNotResorePath);
	int i, nCount = m_cListROVariable.GetCount();
	for (i=0; i<nCount; i++)
	{
		CString strItem;
		m_cListROVariable.GetText(i, strItem);

		Restore.m_listROVariable.AddTail(strItem);
	}
	VCDA_ConstructListRestoreParam (&Restore);

	return CPropertyPage::OnWizardNext();
}

BOOL CuPropertyPageRestoreOption::OnWizardFinish() 
{
	CWaitCursor doWaitCursor;
	CxPSheetRestore* pParent = (CxPSheetRestore*)GetParent();
	CaRestoreParam& Restore = pParent->GetData();
	UpdateData(TRUE);
	if (!CheckOverwriteBackupFile())
		return E_FAIL;

	Restore.Cleanup();
	Restore.SetBackup(m_strBackupFile);
	Restore.SetConfig(m_bRestoreConfig);
	Restore.SetSystemVariable(m_bRestoreSystemVariable);
	Restore.SetUserVariable(m_bRestoreUserVariable);
	Restore.SetVirtualNode(m_bRestoreVirtualNode);
	Restore.SetPath(!m_bNotResorePath);
	VCDA_ConstructListRestoreParam (&Restore);
	BOOL bOK = VCDA_RestoreParam (&Restore);
	if (!bOK)
		return FALSE;
	
	return CPropertyPage::OnWizardFinish();
}

BOOL CuPropertyPageRestoreOption::CheckOverwriteBackupFile()
{
	CString strDir = m_strBackupFile.Left(m_strBackupFile.ReverseFind('\\'));
	if (_taccess(strDir, 0) != 0)
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_DIR_NOACCESS, strDir);
		AfxMessageBox(strMsg);
		return FALSE;
	}

	if (_taccess(m_strBackupFile, 0) == 0)
	{
		int nAnswer = AfxMessageBox (IDS_MSG_OVERWRITE_BACKUPFILE, MB_ICONQUESTION|MB_YESNO);
		if (nAnswer != IDYES)
			return FALSE;
	}
	return TRUE;
}

void CuPropertyPageRestoreOption::OnBtnCheckVNodeDefs()
{
	UpdateData(TRUE);
	if(m_bRestoreVirtualNode)
	{
		//Display a Message Box that Passwords will not be restored.
		AfxMessageBox(IDS_MSG_VNODE_PASSWORDS,MB_OK);
	}
	EnableNextButton();
}

void CuPropertyPageRestoreOption::OnBtnCheckParams()
{
	UpdateData(TRUE);
	if(!m_bNotResorePath)
	{
		//Display a Message Box that non temporary files must be manually moved.
		AfxMessageBox(IDS_MSG_NON_TEMP_FILES,MB_OK);
	}
	EnableNextButton();
}

void CuPropertyPageRestoreOption::OnBtnCheckConfigParams()
{
	UpdateData(TRUE);
	EnableNextButton();
}
void CuPropertyPageRestoreOption::OnBtnCheckSysVariables()
{
	UpdateData(TRUE);
	EnableNextButton();
}
void CuPropertyPageRestoreOption::OnBtnCheckUserVariables()
{
	UpdateData(TRUE);
	EnableNextButton();
}
void CuPropertyPageRestoreOption::EnableNextButton()
{
	if(!m_bRestoreConfig &&
	   !m_bRestoreSystemVariable &&
	   !m_bRestoreVirtualNode &&
	   !m_bRestoreUserVariable)
	{
		((CPropertySheet* )GetParent())->SetWizardButtons( 0 );
	}
	else
	{
		((CPropertySheet* )GetParent())->SetWizardButtons( PSWIZB_NEXT );
	}

}
