/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : xdlguser.cpp , Implementation File
**
**  Project  : Ingres II / VDBA
**
**  Author   : Sotheavut UK (uk$so01)
**
**  Purpose  : Dialog box for Create/Alter User
**
** History:
** 26-Oct-1999 (uk$so01)
**    Created. Rewrite the dialog code, in C++/MFC.
** 16-feb-2000 (somsa01)
**    In InitClassMember(), modified setting of
**    m_pParam->m_strDefaultGroup and m_pParam->m_strProfileName to
**    prevent compiler errors.
** 24-Feb-2000 (uk$so01)
**    Bug #100557: Remove the (default profile) from the create user dialog box
** 14-Sep-2000 (uk$so01)
**    Bug #102609: Missing Pop help ID when destroying the dialog
**    15-Jun-2001(schph01)
**      SIR 104991 initialize and manage new controles and variables to manage
**      the grants for rmcmd
** 30-Jul-2002 (schph01)
**    Bug #108381 On OK button verify that the password and confirm password
**    are identical.
** 07-Oct-2002 (schph01)
**    Bug 108883 Disable the m_ctrlCheckGrantRMCMD control if current user is
**    the owner of rmcmd objects.
** 10-Sep-2004 (uk$so01)
**    BUG #113002 / ISSUE #13671785 & 13671608, Removal of B1 Security
**    from the back-end implies the removal of some equivalent functionalities
**    from VDBA.
**  07-Apr-2005 (srisu02)
**   Add functionality for 'Delete password' checkbox in 'Alter User' dialog box
** 30-May-2005 (lakvi01)
**    In InitClassMember(), make sure that the database list is empty before adding
**    the name of the database.
**  09-Mar-2010 (drivi01) SIR #123397
**    Update the dialog to be more user friendly.  Minimize the amount
**    of controls exposed to the user when dialog is initially displayed.
**    Add "Show/Hide Advanced" button to see more/less options in the 
**    dialog. Put advanced options into a tab control.
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "xdlguser.h"
extern "C" 
{
#include "dbaset.h" // GetOIVers()
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const int LAYOUT_NUMBER = 3;

CxDlgUser::CxDlgUser(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgUser::IDD, pParent)
{
	m_pParam = NULL;
	m_strGroup = _T("");
	m_strProfile = _T("");
	m_strUserName = _T("");
	m_strExpireDate = _T("");
}

CxDlgUser::CxDlgUser(CWnd* pParent /*=NULL*/, BOOL bAlter)
	: CDialog(IDD_XUSER_ALTER, pParent)
{
	//
	// It should be the same as the first constructor:
	m_pParam = NULL;
	m_strGroup = _T("");
	m_strProfile = _T("");
	m_strUserName = _T("");
	m_strExpireDate = _T("");
}

void CxDlgUser::InitClassMember(BOOL bInitMember)
{
	//
	// Must be set first:
	ASSERT (m_pParam);
	if (!m_pParam)
		return;
	if (bInitMember)
	{
		if (m_pParam->m_strDefaultGroup.CompareNoCase(m_pParam->m_strNoGroup) == 0 ||
			m_pParam->m_strDefaultGroup.IsEmpty())
		{
			m_strGroup = _T("");
		}
		else
			m_strGroup = m_pParam->m_strDefaultGroup;
		if (m_pParam->m_strProfileName.CompareNoCase(m_pParam->m_strNoProfile) == 0 ||
			m_pParam->m_strProfileName.IsEmpty())
		{
			m_strProfile = m_pParam->m_strNoProfile;
		}
		else
			m_strProfile = m_pParam->m_strProfileName;
		m_strUserName = m_pParam->m_strName;
		m_strExpireDate = m_pParam->m_strExpireDate;
	}
	else
	{
		m_pParam->m_strName = m_strUserName;
		if (m_strGroup.CompareNoCase(m_pParam->m_strNoGroup) == 0)
		    m_pParam->m_strDefaultGroup = (LPCTSTR)_T("");
		else
		    m_pParam->m_strDefaultGroup = m_strGroup;
		if (m_strProfile.CompareNoCase(m_pParam->m_strNoProfile) == 0)
		    m_pParam->m_strProfileName = (LPCTSTR)_T("");
		else
		    m_pParam->m_strProfileName = m_strProfile;

		m_pParam->m_strExpireDate = m_strExpireDate;
	}
}



void CxDlgUser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgUser)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_MFC_EDIT1, m_cEditUserName);
	DDX_Control(pDX, IDC_MFC_COMBO2, m_cComboProfile);
	DDX_Control(pDX, IDC_MFC_COMBO1, m_cComboGroup);
	DDX_Control(pDX, IDC_MORE_OPTIONS, m_ctrlMoreOptions);
	DDX_CBString(pDX, IDC_MFC_COMBO1, m_strGroup);
	DDX_CBString(pDX, IDC_MFC_COMBO2, m_strProfile);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strUserName);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strExpireDate);
	DDX_Control(pDX, IDC_XUSER_TAB, m_cTabUser);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgUser, CDialog)
	//{{AFX_MSG_MAP(CxDlgUser)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_MFC_EDIT1, OnEditChangeUserName)
	ON_NOTIFY(TCN_SELCHANGE, IDC_XUSER_TAB, OnSelchangeTabLocation)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_MORE_OPTIONS, &CxDlgUser::OnBnClickedMoreOptions)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgUser message handlers

BOOL CxDlgUser::OnInitDialog() 
{
	CDialog::OnInitDialog();
	ASSERT (m_pParam && m_pParam->m_nNodeHandle != -1);
	if (!m_pParam || m_pParam->m_nNodeHandle == -1)
	{
		EndDialog (IDCANCEL);
		return FALSE;
	}

	m_pParam->PushHelpID();
	SetWindowText(m_pParam->GetTitle());

	FillGroups();
	FillProfiles();

	OnEditChangeUserName();
	if (!m_pParam->m_bCreate)
		m_cEditUserName.EnableWindow (FALSE);

	//Setup tabs
	CString TabTitleOne,TabTitleTwo;
	TC_ITEM tcitem;
	TabTitleOne.Format(IDS_URP_TAB_PRIV);
	TabTitleTwo.Format(IDS_URP_TAB_SEC);
	tcitem.mask = TCIF_TEXT;

	tcitem.pszText = (LPSTR)(LPCTSTR)TabTitleOne;
	m_cTabUser.InsertItem(0, &tcitem);

	tcitem.pszText = (LPSTR)(LPCTSTR)TabTitleTwo;
	m_cTabUser.InsertItem(1, &tcitem);

	m_cDlgUserPriv = new CxDlgUserPriv(&m_cTabUser);
	m_cDlgUserPriv->Create(IDD_XUSER_PRIV, &m_cTabUser);
	if (m_pParam->m_bCreate)
	{
		m_cDlgUserSecurity = new CxDlgUserSecurity(&m_cTabUser);
		m_cDlgUserSecurity->Create(IDD_XUSER_SECURITY, &m_cTabUser);
		m_cDlgUserSecurity->ShowWindow(SW_HIDE);
	}
	else
	{
		m_cDlgUserAlterSec = new CxDlgUserAlterSec(&m_cTabUser);
		m_cDlgUserAlterSec->Create(IDD_XUSER_ALTER_SEC, &m_cTabUser);
		m_cDlgUserAlterSec->ShowWindow(SW_HIDE);
	}
	m_cTabUser.SetCurSel(0);
	m_cDlgUserPriv->ShowWindow(SW_SHOW);
	CRect r;
	m_cTabUser.GetClientRect (r);
	m_cTabUser.AdjustRect (FALSE, r);
	m_cDlgUserPriv->MoveWindow(r);

	/** Adding some code to simplify the dialog.  Adding Advanced button.
	*/
	CRect rect;
	this->GetWindowRect(&rect);
	this->MoveWindow(rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.5));
	bExpanded = false;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgUser::OnDestroy() 
{
	CDialog::OnDestroy();
	if (m_pParam)
		m_pParam->PopHelpID();
}

void CxDlgUser::OnSelchangeTabLocation(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int nSel = m_cTabUser.GetCurSel();

	CRect rect;
	m_cTabUser.GetClientRect (rect);
	m_cTabUser.AdjustRect (FALSE, rect);

	switch (nSel)
	{
	case 0:
		if (m_pParam->m_bCreate)
			m_cDlgUserSecurity->ShowWindow(SW_HIDE);
		else
			m_cDlgUserAlterSec->ShowWindow(SW_HIDE);
		m_cDlgUserPriv->ShowWindow(SW_SHOW);
		m_cDlgUserPriv->MoveWindow(rect);
		break;
	case 1:
		m_cDlgUserPriv->ShowWindow(SW_HIDE);
		if (m_pParam->m_bCreate)
		{
			m_cDlgUserSecurity->ShowWindow(SW_SHOW);
			m_cDlgUserSecurity->MoveWindow(rect);
		}
		else
		{
			m_cDlgUserAlterSec->ShowWindow(SW_SHOW);
			m_cDlgUserAlterSec->MoveWindow(rect);
		}
		break;
	}
}

void CxDlgUser::OnOK() 
{
	//
	// Construct the parameter for creating object:
	//
	UpdateData (TRUE);

	if ( (m_pParam->m_bCreate && m_cDlgUserSecurity->m_strConfirmPassword != m_cDlgUserSecurity->m_strPassword) ||
		(!m_pParam->m_bCreate && m_cDlgUserAlterSec->m_strConfirmPassword != m_cDlgUserAlterSec->m_strPassword))
	{
		AfxMessageBox (IDS_E_PASSWORD_ERROR, MB_OK|MB_ICONEXCLAMATION); //"The passwords do not match"
		return;
	}

	if ((m_pParam->m_bCreate && (x_strlen (m_cDlgUserSecurity->m_strPassword) > 0) && m_cDlgUserSecurity->m_bRemovePwd == 1) ||
		(!m_pParam->m_bCreate && (x_strlen (m_cDlgUserAlterSec->m_strPassword) > 0) && m_cDlgUserAlterSec->m_bRemovePwd == 1))
	{
		AfxMessageBox (IDS_E_DEL_OLD_PWD, MB_OK|MB_ICONEXCLAMATION); //"The passwords do not match"
		return;
	}
	
	

	InitClassMember (FALSE);
	if (m_pParam->m_bCreate)
		m_cDlgUserSecurity->InitClassMember(FALSE);
	else
		m_cDlgUserAlterSec->InitClassMember(FALSE);
	m_cDlgUserPriv->InitClassMember(FALSE);

	BOOL bOk = FALSE;
	if (m_pParam->m_bCreate)
		bOk = m_pParam->CreateObject(this);
	else
		bOk = m_pParam->AlterObject(this);
	if (!bOk)
		return;

	bOk = m_pParam->GrantRevoke4Rmcmd(this);
	if (!bOk)
		return;

	CDialog::OnOK();
}



void CxDlgUser::FillGroups()
{
	CStringList listObj;
	m_cComboGroup.ResetContent();
	m_pParam->GetGroups (listObj);
	m_cComboGroup.AddString(m_pParam->m_strNoGroup);

	POSITION pos = listObj.GetHeadPosition();
	while (pos != NULL)
	{
		CString strObj = listObj.GetNext (pos);
		m_cComboGroup.AddString (strObj);
	}
	int idx = 0;
	if (!m_pParam->m_bCreate)
	{
		idx = m_cComboGroup.FindStringExact (-1, m_strGroup);
		if (idx == -1)
			idx = 0;
	}
	m_cComboGroup.SetCurSel(idx);
}

void CxDlgUser::FillProfiles()
{
	CStringList listObj;
	m_cComboProfile.ResetContent();
	m_pParam->GetProfiles (listObj);
	m_cComboProfile.AddString(m_pParam->m_strNoProfile);

	POSITION pos = listObj.GetHeadPosition();
	while (pos != NULL)
	{
		CString strObj = listObj.GetNext (pos);
		//
		// Ignore the string "(default profile)":
		if (INGRESII_IsDefaultProfile (strObj))
			continue;
		m_cComboProfile.AddString (strObj);
	}
	int idx = m_cComboProfile.FindStringExact (-1, m_pParam->m_strNoProfile);

	if (!m_pParam->m_bCreate)
	{
		int idef = m_cComboProfile.FindStringExact (-1, m_strProfile);
		if (idef != -1)
			idx = idef;
	}
	m_cComboProfile.SetCurSel(idx);
}

void CxDlgUser::OnEditChangeUserName() 
{
	CString strItem;
	m_cEditUserName.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	if (strItem.IsEmpty())
		m_cButtonOK.EnableWindow(FALSE);
	else
		m_cButtonOK.EnableWindow(TRUE);
}

void CxDlgUser::OnBnClickedMoreOptions()
{
	if (bExpanded)
	{
		CRect rect;
		this->GetWindowRect(&rect);
		this->MoveWindow(rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.5));
		m_ctrlMoreOptions.SetWindowTextA("Show &Advanced >>");
		bExpanded = false;
	}
	else
	{
		CRect rect;
		this->GetWindowRect(&rect);
		this->MoveWindow(rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)*2.5));
		m_ctrlMoreOptions.SetWindowTextA("<< Hide &Advanced");
		bExpanded = true;
	}
}
