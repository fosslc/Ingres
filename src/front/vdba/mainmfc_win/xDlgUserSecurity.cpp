/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  xDlgUserSecurity.cpp : Creates a tab panel for Security tab in
**						   "Create User" dialog.
**
** History:
**	10-Feb-2010 (drivi01)
**		Created.
**	2-24-2010 (drivi01)
**		Fixed a bug that would prevent from giving user access to  
**		multiple databases b/c the database list was being cleared 
**		out before each database was added.
*/

#include "stdafx.h"
#include "xDlgUserSecurity.h"
#include "rcdepend.h"
#include "dmlurp.h"
#include "xdlguser.h"
extern "C" 
{
#include "dbaset.h" // GetOIVers()
}

IMPLEMENT_DYNAMIC(CxDlgUserSecurity, CDialogEx)

CxDlgUserSecurity::CxDlgUserSecurity(CWnd* pParent /*=NULL*/)
	: CDialogEx(CxDlgUserSecurity::IDD, pParent)
{
	m_bAllEvents = FALSE;
	m_bQueryText = FALSE;
	m_bExternalPassword = FALSE;
	m_strLimitSecurityLabel = _T("");
	m_strPassword = _T("");
	m_strConfirmPassword = _T("");
	m_strOldPassword = _T("");
}

CxDlgUserSecurity::~CxDlgUserSecurity()
{
}

void CxDlgUserSecurity::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_RMCMD_TITLE, m_RmcmdTitle);
	DDX_Control(pDX, IDC_CHECK_GRANT_REVOKE_RMCMD, m_ctrlCheckGrantRMCMD);
	DDX_Control(pDX, DELETE_OLD_PASSWD, m_ctrlRemovePwd);
	DDX_Control(pDX, IDC_STATIC_PARTIAL_GRANT, m_ctrlStaticPartial);
	DDX_Control(pDX, IDC_STATIC_ICON_PARTIALGRANT, m_ctrlIconPartial);
	DDX_Control(pDX, IDC_MFC_EDIT5, m_cEditConfirmPassword);
	DDX_Control(pDX, IDC_MFC_EDIT4, m_cEditPassword);
	DDX_Control(pDX, IDC_MFC_CHECK4, m_cCheckExternalPassword);
	DDX_Control(pDX, IDC_MFC_CHECK1, m_AllEvents);
	DDX_Control(pDX, IDC_MFC_CHECK2, m_QueryText);
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bAllEvents);
	DDX_Check(pDX, IDC_MFC_CHECK2, m_bQueryText);
	DDX_Check(pDX, IDC_MFC_CHECK4, m_bExternalPassword);
	DDX_Text(pDX, IDC_MFC_EDIT3, m_strLimitSecurityLabel);
	DDX_Text(pDX, IDC_MFC_EDIT4, m_strPassword);
	DDX_Text(pDX, IDC_MFC_EDIT5, m_strConfirmPassword);
	DDX_Check(pDX, IDC_CHECK_GRANT_REVOKE_RMCMD, m_bGrantRmcmd);
	DDX_Check(pDX, DELETE_OLD_PASSWD, m_bRemovePwd);

}


BEGIN_MESSAGE_MAP(CxDlgUserSecurity, CDialogEx)
	ON_BN_CLICKED(IDC_MFC_CHECK4, OnCheckExternalPassword)
END_MESSAGE_MAP()

void CxDlgUserSecurity::InitClassMember(BOOL bInitMember)
{
	ASSERT (pParam2);
	if (!pParam2)
		return;
	if (bInitMember)
	{
		m_ctrlCheckGrantRMCMD.SetCheck(pParam2->m_bGrantUser4Rmcmd);
		m_ctrlRemovePwd.SetCheck(pParam2->m_bRemovePwd);
		m_AllEvents.SetCheck(pParam2->GetAllEvent());
		m_QueryText.SetCheck(pParam2->GetQueryText());
		m_cCheckExternalPassword.SetCheck(pParam2->m_bExternalPassword);
		m_strLimitSecurityLabel = pParam2->m_strLimitSecLabels;
		m_strPassword = pParam2->m_strPassword;
		m_strConfirmPassword = pParam2->m_strPassword;

		if (!pParam2->m_bCreate)
			m_strOldPassword = pParam2->m_strOldPassword;
	}
	else
	{
		pParam2->m_bGrantUser4Rmcmd = m_ctrlCheckGrantRMCMD.GetCheck();
		pParam2->m_bRemovePwd = m_ctrlRemovePwd.GetCheck();
		pParam2->m_strLimitSecLabels = m_strLimitSecurityLabel;
		pParam2->m_bExternalPassword = m_cCheckExternalPassword.GetCheck();

		if (!pParam2->m_bCreate)
			pParam2->m_strOldPassword= m_strOldPassword;
		
		pParam2->SetAllEvent(m_AllEvents.GetCheck());
		pParam2->SetQueryText(m_QueryText.GetCheck());
		
		if (pParam2->m_bCreate)
		{
			CString strObj;
			int i = 0;
			int nCount = m_cCheckListBoxDatabase.GetCount();
			//remove all items from the list before we start
			//checking names of the databases.
			if (!pParam2->m_strListDatabase.IsEmpty())
				pParam2->m_strListDatabase.RemoveAll();
			for (i = 0; i < nCount; i++)
			{
				if (m_cCheckListBoxDatabase.GetCheck(i))
				{
					m_cCheckListBoxDatabase.GetText (i, strObj);
					pParam2->m_strListDatabase.AddTail(strObj);
				}
			}
		}
	}
}

BOOL CxDlgUserSecurity::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CxDlgUser *m_pParent;

	m_pParent = (CxDlgUser *)GetParent()->GetParent();
	pParam2 = m_pParent->m_pParam;
	InitClassMember();

	if (pParam2->m_bCreate)
	{
		m_bGrantRmcmd = FALSE;
		m_bRemovePwd = FALSE;
	}

	if (GetOIVers() >= OIVERS_30)
	{
		CWnd* pWnd = GetDlgItem(IDC_MFC_STATIC1);
		if (pWnd && pWnd->m_hWnd)
			pWnd->ShowWindow(SW_HIDE);
		pWnd = GetDlgItem(IDC_MFC_EDIT3);
		if (pWnd && pWnd->m_hWnd)
			pWnd->ShowWindow(SW_HIDE);
	}

	if (pParam2->m_bCreate)
	{
		VERIFY (m_cCheckListBoxDatabase.SubclassDlgItem (IDC_MFC_LIST2, this));
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (hFont == NULL)
			hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	}
	else
	{
		GetDlgItem(IDC_MFC_LIST2)->EnableWindow(FALSE);
	}
	if (pParam2->m_bPartialGrant && !pParam2->m_bCreate)
	{
		HICON hIcon = theApp.LoadStandardIcon(IDI_EXCLAMATION);
		m_ctrlIconPartial.SetIcon(hIcon);
		m_ctrlIconPartial.Invalidate();
		DestroyIcon(hIcon);
		m_ctrlIconPartial.ShowWindow(SW_SHOW);
		m_ctrlStaticPartial.ShowWindow(SW_SHOW);
		m_RmcmdTitle.ShowWindow(SW_SHOW);
	}
	else
	{
		m_ctrlIconPartial.ShowWindow(SW_HIDE);
		m_ctrlStaticPartial.ShowWindow(SW_HIDE);
		m_RmcmdTitle.ShowWindow(SW_HIDE);
	}

	FillDatabases();
	OnCheckExternalPassword();

	if ( pParam2->m_bGrantUser4Rmcmd )
		m_ctrlCheckGrantRMCMD.SetCheck(BST_CHECKED);
	else
		m_ctrlCheckGrantRMCMD.SetCheck(BST_UNCHECKED);
	if ( pParam2->m_bRemovePwd )
		m_ctrlRemovePwd.SetCheck(BST_CHECKED);
	else
		m_ctrlRemovePwd.SetCheck(BST_UNCHECKED);
	if ( pParam2->m_bOwnerRmcmdObjects )
		m_ctrlCheckGrantRMCMD.EnableWindow(FALSE);

	//m_cCheckExternalPassword.SetCheck(1);
	OnCheckExternalPassword();

	return TRUE;
}


void CxDlgUserSecurity::FillDatabases()
{
	if (!pParam2->m_bCreate)
		return;
	CStringList listObj;
	m_cCheckListBoxDatabase.ResetContent();
	pParam2->GetDatabases (listObj);

	POSITION pos = listObj.GetHeadPosition();
	while (pos != NULL)
	{
		CString strObj = listObj.GetNext (pos);
		m_cCheckListBoxDatabase.AddString (strObj);
	}
}

void CxDlgUserSecurity::OnCheckExternalPassword() 
{
	int nCheck = m_cCheckExternalPassword.GetCheck();
	if (nCheck == 1)
	{
		m_cEditConfirmPassword.EnableWindow(FALSE);
		m_cEditPassword.EnableWindow(FALSE);
	}
	else
	{
		m_cEditConfirmPassword.EnableWindow(TRUE);
		m_cEditPassword.EnableWindow(TRUE);
	}
}

void CxDlgUserSecurity::OnOK()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgUser* pParentDlg = (CxDlgUser*)GetParent()->GetParent();
   pParentDlg->OnOK();
}

void CxDlgUserSecurity::OnCancel()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgUser* pParentDlg = (CxDlgUser*)GetParent()->GetParent();
   pParentDlg->OnCancel();
}

// CxDlgUserSecurity message handlers
