/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  xDlgUserAlterSec.cpp : Creates a tab panel for Security tab in
**					  "Alter User" dialog.
**
** History:
**	10-Mar-2010 (drivi01)
**		Created.
*/


#include "stdafx.h"
#include "xDlgUserAlterSec.h"
#include "rcdepend.h"
#include "dmlurp.h"
#include "xdlguser.h"
extern "C" 
{
#include "dbaset.h" // GetOIVers()
}

// CxDlgUserAlterSec dialog

IMPLEMENT_DYNAMIC(CxDlgUserAlterSec, CDialogEx)

CxDlgUserAlterSec::CxDlgUserAlterSec(CWnd* pParent /*=NULL*/)
	: CDialogEx(CxDlgUserAlterSec::IDD, pParent)
{
	m_bAllEvents = FALSE;
	m_bQueryText = FALSE;
	m_bExternalPassword = FALSE;
	m_strLimitSecurityLabel = _T("");
	m_strPassword = _T("");
	m_strConfirmPassword = _T("");
	//}}AFX_DATA_INIT
	m_strOldPassword = _T("");
}

CxDlgUserAlterSec::~CxDlgUserAlterSec()
{
}

void CxDlgUserAlterSec::DoDataExchange(CDataExchange* pDX)
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


BEGIN_MESSAGE_MAP(CxDlgUserAlterSec, CDialogEx)
	ON_BN_CLICKED(IDC_MFC_CHECK4, OnCheckExternalPassword)
END_MESSAGE_MAP()

void CxDlgUserAlterSec::InitClassMember(BOOL bInitMember)
{
	ASSERT (pParam3);
	if (!pParam3)
		return;
	if (bInitMember)
	{
		m_ctrlCheckGrantRMCMD.SetCheck(pParam3->m_bGrantUser4Rmcmd);
		m_ctrlRemovePwd.SetCheck(pParam3->m_bRemovePwd);
		m_bExternalPassword = pParam3->m_bExternalPassword;
		m_AllEvents.SetCheck(pParam3->GetAllEvent());
		m_QueryText.SetCheck(pParam3->GetQueryText());
		m_cCheckExternalPassword.SetCheck(pParam3->m_bExternalPassword);

		m_strLimitSecurityLabel = pParam3->m_strLimitSecLabels;
		m_strPassword = pParam3->m_strPassword;
		m_strConfirmPassword = pParam3->m_strPassword;

		if (!pParam3->m_bCreate)
			m_strOldPassword = pParam3->m_strOldPassword;
	}
	else
	{
		pParam3->m_bGrantUser4Rmcmd = m_ctrlCheckGrantRMCMD.GetCheck();
		pParam3->m_bRemovePwd = m_ctrlRemovePwd.GetCheck();
		pParam3->m_strLimitSecLabels = m_strLimitSecurityLabel;
		pParam3->m_bExternalPassword = m_cCheckExternalPassword.GetCheck();
		pParam3->m_bExternalPassword = m_bExternalPassword;

		if (!pParam3->m_bCreate)
			pParam3->m_strOldPassword= m_strOldPassword;
		
		pParam3->SetAllEvent(m_AllEvents.GetCheck());
		pParam3->SetQueryText(m_QueryText.GetCheck());
		
		if (pParam3->m_bCreate)
		{
			CString strObj;
			int i = 0;
			int nCount = m_cCheckListBoxDatabase.GetCount();
			for (i = 0; i < nCount; i++)
			{
				if (m_cCheckListBoxDatabase.GetCheck(i))
				{
					m_cCheckListBoxDatabase.GetText (i, strObj);
					if(!pParam3->m_strListDatabase.IsEmpty()) 
							pParam3->m_strListDatabase.RemoveAll();
					pParam3->m_strListDatabase.AddTail(strObj);
				}
			}
		}
	}
}

BOOL CxDlgUserAlterSec::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CxDlgUser *m_pParent;

	m_pParent = (CxDlgUser *)GetParent()->GetParent();
	pParam3 = m_pParent->m_pParam;
	InitClassMember();

	if (pParam3->m_bCreate)
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

	if (pParam3->m_bCreate)
	{
		VERIFY (m_cCheckListBoxDatabase.SubclassDlgItem (IDC_MFC_LIST2, this));
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (hFont == NULL)
			hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	}

	if (pParam3->m_bPartialGrant && !pParam3->m_bCreate)
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

	OnCheckExternalPassword();

	if ( pParam3->m_bGrantUser4Rmcmd )
		m_ctrlCheckGrantRMCMD.SetCheck(BST_CHECKED);
	else
		m_ctrlCheckGrantRMCMD.SetCheck(BST_UNCHECKED);
	if ( pParam3->m_bRemovePwd )
		m_ctrlRemovePwd.SetCheck(BST_CHECKED);
	else
		m_ctrlRemovePwd.SetCheck(BST_UNCHECKED);
	if ( pParam3->m_bOwnerRmcmdObjects )
		m_ctrlCheckGrantRMCMD.EnableWindow(FALSE);

	OnCheckExternalPassword();

	return TRUE;
}



void CxDlgUserAlterSec::OnCheckExternalPassword() 
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

void CxDlgUserAlterSec::OnOK()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgUser* pParentDlg = (CxDlgUser*)GetParent()->GetParent();
   pParentDlg->OnOK();
}

void CxDlgUserAlterSec::OnCancel()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgUser* pParentDlg = (CxDlgUser*)GetParent()->GetParent();
   pParentDlg->OnCancel();
}
// CxDlgUserAlterSec message handlers
