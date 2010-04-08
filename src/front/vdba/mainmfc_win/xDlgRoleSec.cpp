/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  xDlgRoleSec.cpp : Creates a tab panel for Security tab in
**					  "Create/Alter Role" dialog.
**
** History:
**	10-Mar-2010 (drivi01)
**		Created.
*/

#include "stdafx.h"
#include "xDlgRoleSec.h"
extern "C" 
{
#include "dbaset.h" // GetOIVers()
}

// CxDlgRoleSec dialog
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CxDlgRoleSec, CDialog)

CxDlgRoleSec::CxDlgRoleSec(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgRoleSec::IDD, pParent)
{
	m_strLimitSecurityLabel = _T("");
	m_strPassword = _T("");
	m_strConfirmPassword = _T("");
	//}}AFX_DATA_INIT
	m_pParam = NULL;
}

CxDlgRoleSec::~CxDlgRoleSec()
{
}

void CxDlgRoleSec::InitClassMembers(BOOL bInitMember)
{
	//
	// Must be set first:
	ASSERT (m_pParam);
	if (!m_pParam)
		return;
	if (bInitMember)
	{
		m_bAllEvents.SetCheck( m_pParam->GetAllEvent());
		m_bQueryText.SetCheck( m_pParam->GetQueryText());
		m_cCheckExternalPassword.SetCheck( m_pParam->m_bExternalPassword);
		m_strLimitSecurityLabel = m_pParam->m_strLimitSecLabels;
		m_strPassword = m_pParam->m_strPassword;
		m_strConfirmPassword = m_pParam->m_strPassword;

		POSITION pos=0;
		if (IsWindow (m_hWnd))
		{
			pos = m_pParam->m_strListDatabase.GetHeadPosition();
			while (pos != NULL)
			{
				CString strObj = m_pParam->m_strListDatabase.GetNext(pos);
				m_cCheckListBoxDatabase.SetCheck(strObj);
			}
		}
	}
	else
	{
		m_pParam->m_strPassword   = m_strPassword;
		m_pParam->m_strLimitSecLabels = m_strLimitSecurityLabel;
		m_pParam->m_bExternalPassword = m_cCheckExternalPassword.GetCheck();

		m_pParam->SetAllEvent(m_bAllEvents.GetCheck());
		m_pParam->SetQueryText(m_bQueryText.GetCheck());
		
		CString strObj;
		int i = 0;
		if (m_pParam->m_bCreate)
		{
			int nCount = m_cCheckListBoxDatabase.GetCount();
			for (i = 0; i < nCount; i++)
			{
				if (m_cCheckListBoxDatabase.GetCheck(i))
				{
					m_cCheckListBoxDatabase.GetText (i, strObj);
					m_pParam->m_strListDatabase.AddTail(strObj);
				}
			}
		}
	}
}

void CxDlgRoleSec::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFC_EDIT5, m_cEditConfirmPassword);
	DDX_Control(pDX, IDC_MFC_EDIT4, m_cEditPassword);
	DDX_Control(pDX, IDC_MFC_CHECK4, m_cCheckExternalPassword);
	DDX_Control(pDX, IDC_MFC_CHECK1, m_bAllEvents);
	DDX_Control(pDX, IDC_MFC_CHECK2, m_bQueryText);
	DDX_Text(pDX, IDC_MFC_EDIT3, m_strLimitSecurityLabel);
	DDX_Text(pDX, IDC_MFC_EDIT4, m_strPassword);
	DDX_Text(pDX, IDC_MFC_EDIT5, m_strConfirmPassword);
}


BEGIN_MESSAGE_MAP(CxDlgRoleSec, CDialog)
	ON_BN_CLICKED(IDC_MFC_CHECK4, OnCheckExternalPassword)
END_MESSAGE_MAP()

BOOL CxDlgRoleSec::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CxDlgRole *m_pParent;
	m_pParent = (CxDlgRole *)GetParent()->GetParent();
	m_pParam = m_pParent->m_pParam;
	InitClassMembers();

	ASSERT (m_pParam && m_pParam->m_nNodeHandle != -1);
	if (!m_pParam || m_pParam->m_nNodeHandle == -1)
	{
		EndDialog (IDCANCEL);
		return FALSE;
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
	if (m_pParam->m_bCreate)
	{
		VERIFY (m_cCheckListBoxDatabase.SubclassDlgItem (IDC_MFC_LIST2, this));
		HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if (hFont == NULL)
			hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	}
	else
	{
		GetDlgItem(IDC_MFC_LIST2)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_MFC_STATIC_ACCESSDB)->ShowWindow(SW_HIDE);
	}

	FillDatabases();
	OnCheckExternalPassword();


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgRoleSec::OnCheckExternalPassword() 
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

void CxDlgRoleSec::FillDatabases()
{
	if (!m_pParam->m_bCreate)
		return;
	CStringList listObj;
	m_cCheckListBoxDatabase.ResetContent();
	m_pParam->GetDatabases (listObj);

	POSITION pos = listObj.GetHeadPosition();
	while (pos != NULL)
	{
		CString strObj = listObj.GetNext (pos);
		m_cCheckListBoxDatabase.AddString (strObj);
	}
}
void CxDlgRoleSec::OnOK()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgRole* pParentDlg = (CxDlgRole*)GetParent()->GetParent();
   pParentDlg->OnOK();
}

void CxDlgRoleSec::OnDestroy()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgRole* pParentDlg = (CxDlgRole*)GetParent()->GetParent();
   pParentDlg->OnDestroy();
}

// CxDlgRoleSec message handlers


