/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xdlgrole.cpp , Implementation File 
**    Project  : Ingres II / VDBA
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Dialog box for Create/Alter Role
**
** History:
**
** 27-Oct-1999 (uk$so01)
**    created, Rewrite the dialog code, in C++/MFC.
** 14-Sep-2000 (uk$so01)
**    Bug #102609: Missing Pop help ID when destroying the dialog
** 30-Jul-2002 (schph01)
**    Bug #108381 On OK button verify that the password and confirm password
**    are identical.
** 10-Sep-2004 (uk$so01)
**    BUG #113002 / ISSUE #13671785 & 13671608, Removal of B1 Security
**    from the back-end implies the removal of some equivalent functionalities
**    from VDBA.
**  09-Mar-2010 (drivi01) SIR #123397
**    Update the dialog to be more user friendly.  Minimize the amount
**    of controls exposed to the user when dialog is initially displayed.
**    Add "Show/Hide Advanced" button to see more/less options in the 
**    dialog. Put advanced options into a tab control.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "xdlgrole.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//static const int LAYOUT_NUMBER = 2;
BOOL bExpanded;

CxDlgRole::CxDlgRole(CWnd* pParent /*=NULL*/)
	: CDialogEx(CxDlgRole::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgRole)
	m_strRoleName = _T("");
	//}}AFX_DATA_INIT
	m_pParam = NULL;
}

CxDlgRole::CxDlgRole(CWnd* pParent /*=NULL*/, BOOL bAlter)
	: CDialogEx(IDD_XROLE_ALTER, pParent)
{
	//m_bExternalPassword = FALSE;
	m_strRoleName = _T("");
	m_pParam = NULL;
}


void CxDlgRole::InitClassMembers(BOOL bInitMember)
{
	//
	// Must be set first:
	ASSERT (m_pParam);
	if (!m_pParam)
		return;
	if (bInitMember)
	{
		m_strRoleName = m_pParam->m_strName;
	}
	else
	{
		m_pParam->m_strName = m_strRoleName;
	}
}


void CxDlgRole::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgRole)
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Control(pDX, IDC_MFC_EDIT1, m_cEditRoleName);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strRoleName);
	DDX_Control(pDX, IDC_TAB_XROLE, m_usrTab);
	DDX_Control(pDX, IDC_MORE_OPTIONS, m_cAdvanced);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgRole, CDialogEx)
	//{{AFX_MSG_MAP(CxDlgRole)
	ON_EN_CHANGE(IDC_MFC_EDIT1, OnChangeEditRoleName)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_XROLE, OnSelchangeTabLocation)
	ON_BN_CLICKED(IDC_MORE_OPTIONS, OnShowHideAdvanced)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgRole message handlers

BOOL CxDlgRole::OnInitDialog() 
{
	CDialogEx::OnInitDialog();
	
	ASSERT (m_pParam && m_pParam->m_nNodeHandle != -1);
	if (!m_pParam || m_pParam->m_nNodeHandle == -1)
	{
		EndDialog (IDCANCEL);
		return FALSE;
	}
	if (!m_pParam->m_bCreate)
		m_cEditRoleName.EnableWindow (FALSE);
	

	m_pParam->PushHelpID();
	SetWindowText(m_pParam->GetTitle());

	OnChangeEditRoleName();

	TC_ITEM tcitem;
	CString TabTitleOne, TabTitleTwo;


	TabTitleOne.Format(IDS_URP_TAB_PRIV);
	TabTitleTwo.Format(IDS_URP_TAB_SEC);
	tcitem.mask = TCIF_TEXT;

	tcitem.pszText = (LPSTR)(LPCTSTR)TabTitleOne;
	m_usrTab.InsertItem(0, &tcitem);

	tcitem.pszText = (LPSTR)(LPCTSTR)TabTitleTwo;
	m_usrTab.InsertItem(1, &tcitem);

	m_cDlgRolePriv = new CxDlgRolePriv(&m_usrTab);
	m_cDlgRolePriv->Create(IDD_XROLE_PRIV, &m_usrTab);
	m_cDlgRolePriv->ShowWindow(SW_SHOW);

	m_cDlgRoleSec = new CxDlgRoleSec(&m_usrTab);
	m_cDlgRoleSec->Create(IDD_XROLE_SEC, &m_usrTab);
	m_cDlgRoleSec->ShowWindow(SW_HIDE);

	m_usrTab.SetCurSel(0);

	CRect rect;
	m_usrTab.GetClientRect(&rect);
	m_usrTab.AdjustRect (FALSE, &rect);
	m_cDlgRolePriv->MoveWindow(&rect);

	if (!m_pParam->m_bCreate)
		m_cAdvanced.ShowWindow(FALSE);
	/** Adding some code to simplify the dialog.  Adding Advanced button.
	*/
	if (m_pParam->m_bCreate)
	{
	this->GetWindowRect(&rect);
	this->MoveWindow(rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.5));
	bExpanded = false;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgRole::OnSelchangeTabLocation(NMHDR* pNMHDR, LRESULT* pResult) 
{ 
	int nSel = m_usrTab.GetCurSel();

	CRect rect;
	m_usrTab.GetClientRect (rect);
	m_usrTab.AdjustRect (FALSE, rect);

	switch (nSel)
	{
	case 0:
		
		m_cDlgRoleSec->ShowWindow(SW_HIDE);
		m_cDlgRolePriv->ShowWindow(SW_SHOW);
		m_cDlgRolePriv->MoveWindow(rect);
		break;
	case 1:
		m_cDlgRolePriv->ShowWindow(SW_HIDE);
		m_cDlgRoleSec->ShowWindow(SW_SHOW);
		m_cDlgRoleSec->MoveWindow(rect);
		break;
	}
}

void CxDlgRole::OnShowHideAdvanced()
{
	CRect rect;
	if (bExpanded)
	{
		this->GetWindowRect(&rect);
		this->MoveWindow(rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)/2.5));
		bExpanded = false;
		m_cAdvanced.SetWindowText("Show &Advanced");
	}
	else
	{
		CRect rect;
		this->GetWindowRect(&rect);
		this->MoveWindow(rect.left, rect.top, rect.right-rect.left, (int)((rect.bottom-rect.top)*2.5));
		bExpanded = true;
		m_cAdvanced.SetWindowText("Hide &Advanced");
	}
}


void CxDlgRole::OnOK() 
{
	//
	// Construct the parameter for creating object:
	//
	UpdateData (TRUE);

	if ( m_cDlgRoleSec->m_strConfirmPassword != m_cDlgRoleSec->m_strPassword)
	{
		AfxMessageBox (IDS_E_PASSWORD_ERROR, MB_OK|MB_ICONEXCLAMATION); //"The passwords do not match"
		return;
	}

	InitClassMembers(FALSE);
	m_cDlgRolePriv->InitClassMembers(FALSE);
	m_cDlgRoleSec->InitClassMembers(FALSE);

	BOOL bOk = FALSE;
	if (m_pParam->m_bCreate)
		bOk = m_pParam->CreateObject(this);
	else
		bOk = m_pParam->AlterObject(this);
	if (!bOk)
		return;
	CDialogEx::OnOK();
}


void CxDlgRole::OnChangeEditRoleName() 
{
	CString strItem;
	m_cEditRoleName.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	if (strItem.IsEmpty())
		m_cButtonOK.EnableWindow(FALSE);
	else
		m_cButtonOK.EnableWindow(TRUE);
}


void CxDlgRole::OnDestroy() 
{
	CDialogEx::OnDestroy();
	if (m_pParam)
		m_pParam->PopHelpID();
}
