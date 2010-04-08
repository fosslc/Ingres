/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : xdlgprof.cpp , Implementation File
**
**  Project  : Ingres II / VDBA
**
**  Author   : Sotheavut UK (uk$so01)
**
**  Purpose  : Dialog box for Create/Alter Profile
**
**  History:
**  27-Oct-1999 (uk$so01)
**      Created. Rewrite the dialog code, in C++/MFC.
**  16-feb-2000 (somsa01)
**      In InitClassMembers(), changed method of setting
**      m_pParam->m_strDefaultGroup to prevent compiler error.
** 14-Sep-2000 (uk$so01)
**    Bug #102609: Pop help ID when destroying the dialog
** 10-Sep-2004 (uk$so01)
**    BUG #113002 / ISSUE #13671785 & 13671608, Removal of B1 Security
**    from the back-end implies the removal of some equivalent functionalities
**    from VDBA.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "xdlgprof.h"
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



CxDlgProfile::CxDlgProfile(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgProfile::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgProfile)
	m_bAllEvents = FALSE;
	m_bQueryText = FALSE;
	m_strGroup = _T("");
	m_strProfileName = _T("");
	m_strExpireDate = _T("");
	m_strLimitSecurityLabel = _T("");
	//}}AFX_DATA_INIT
	m_pParam = NULL;
}

void CxDlgProfile::InitClassMembers(BOOL bInitMember)
{
	//
	// Must be set first:
	ASSERT (m_pParam);
	if (!m_pParam)
		return;
	if (bInitMember)
	{
		m_bAllEvents = m_pParam->GetAllEvent();
		m_bQueryText = m_pParam->GetQueryText();
		if (m_pParam->m_strDefaultGroup.CompareNoCase(m_pParam->m_strNoGroup) == 0 ||
			m_pParam->m_strDefaultGroup.IsEmpty())
		{
			m_strGroup = m_pParam->m_strNoGroup;
		}
		else
			m_strGroup = m_pParam->m_strDefaultGroup;

		m_strProfileName = m_pParam->m_strName ;
		m_strExpireDate = m_pParam->m_strExpireDate;
		m_strLimitSecurityLabel = m_pParam->m_strLimitSecLabels;

		int i = 0;
		POSITION pos = m_pParam->m_listPrivileges.GetHeadPosition();
		while (pos != NULL && i < m_pParam->GetMaxPrivilege())
		{
			CaURPPrivilegesItemData* pObj = m_pParam->m_listPrivileges.GetNext(pos);
			pObj->m_bUser = m_pParam->m_Privilege[i];
			pObj->m_bDefault = m_pParam->m_bDefaultPrivilege[i];
			i++;
		}
	}
	else
	{
		if (m_strGroup.CompareNoCase(m_pParam->m_strNoGroup) == 0)
		    m_pParam->m_strDefaultGroup = (LPCTSTR)_T("");
		else
		    m_pParam->m_strDefaultGroup = m_strGroup;

		m_pParam->m_strName = m_strProfileName;
		m_pParam->m_strExpireDate = m_strExpireDate;
		m_pParam->m_strLimitSecLabels = m_strLimitSecurityLabel;
		
		int i = 0;
		POSITION pos = m_pParam->m_listPrivileges.GetHeadPosition();
		while (pos != NULL && i < m_pParam->GetMaxPrivilege())
		{
			CaURPPrivilegesItemData* pObj = m_pParam->m_listPrivileges.GetNext(pos);
			m_pParam->m_Privilege[i] = pObj->m_bUser;
			m_pParam->m_bDefaultPrivilege[i] = pObj->m_bDefault;
			i++;
		}

		m_pParam->SetAllEvent(m_bAllEvents);
		m_pParam->SetQueryText(m_bQueryText);
	}
}


void CxDlgProfile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgProfile)
	DDX_Control(pDX, IDC_MFC_COMBO1, m_cComboGroup);
	DDX_Control(pDX, IDC_MFC_EDIT1, m_cEditProfileName);
	DDX_Control(pDX, IDOK, m_cButtonOK);
	DDX_Check(pDX, IDC_MFC_CHECK1, m_bAllEvents);
	DDX_Check(pDX, IDC_MFC_CHECK2, m_bQueryText);
	DDX_CBString(pDX, IDC_MFC_COMBO1, m_strGroup);
	DDX_Text(pDX, IDC_MFC_EDIT1, m_strProfileName);
	DDX_Text(pDX, IDC_MFC_EDIT2, m_strExpireDate);
	DDX_Text(pDX, IDC_MFC_EDIT3, m_strLimitSecurityLabel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgProfile, CDialog)
	//{{AFX_MSG_MAP(CxDlgProfile)
	ON_EN_CHANGE(IDC_MFC_EDIT1, OnChangeEditName)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgProfile message handlers

BOOL CxDlgProfile::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
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
	m_cListPrivilege.SetFullRowSelected(FALSE);
	VERIFY (m_cListPrivilege.SubclassDlgItem (IDC_MFC_LIST1, this));
	m_ImageCheck.Create (IDB_CHECK, 16, 1, RGB (255, 0, 0));
	m_cListPrivilege.SetCheckImageList(&m_ImageCheck);

	//
	// Initalize the Column Header of CListCtrl (CuListCtrl)
	//
	LV_COLUMN lvcolumn;
	LSCTRLHEADERPARAMS2 lsp[LAYOUT_NUMBER] =
	{
		{IDS_URP_PRIVILEGE,   128,  LVCFMT_LEFT,   FALSE},
		{IDS_URP_USER,         50,  LVCFMT_CENTER, TRUE },
		{IDS_URP_DEFAULT,      60,  LVCFMT_CENTER, TRUE },
	};

	CString strHeader;
	memset (&lvcolumn, 0, sizeof (LV_COLUMN));
	lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
	for (int i=0; i<LAYOUT_NUMBER; i++)
	{
		strHeader.LoadString (lsp[i].m_nIDS);
		lvcolumn.fmt      = lsp[i].m_fmt;
		lvcolumn.pszText  = (LPTSTR)(LPCTSTR)strHeader;
		lvcolumn.iSubItem = i;
		lvcolumn.cx       = lsp[i].m_cxWidth;
		m_cListPrivilege.InsertColumn (i, &lvcolumn, lsp[i].m_bUseCheckMark);
	}

	m_pParam->PushHelpID();
	SetWindowText(m_pParam->GetTitle());

	FillPrivileges();
	FillGroups();
	OnChangeEditName();
	if (!m_pParam->m_bCreate)
		m_cEditProfileName.EnableWindow (FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgProfile::OnOK() 
{
	//
	// Construct the parameter for creating object:
	//
	UpdateData (TRUE);
	InitClassMembers (FALSE);
	BOOL bOk = FALSE;

	if (m_pParam->m_bCreate)
		bOk = m_pParam->CreateObject(this);
	else
		bOk = m_pParam->AlterObject(this);
	if (!bOk)
		return;
	CDialog::OnOK();
}

void CxDlgProfile::OnChangeEditName() 
{
	CString strItem;
	m_cEditProfileName.GetWindowText (strItem);
	strItem.TrimLeft();
	strItem.TrimRight();
	if (strItem.IsEmpty())
		m_cButtonOK.EnableWindow(FALSE);
	else
		m_cButtonOK.EnableWindow(TRUE);
}


void CxDlgProfile::FillPrivileges()
{
	TCHAR tchszText[] =_T("");
	int nCount = 0;
	LV_ITEM lvitem;
	CaURPPrivilegesItemData* pItem = NULL;
	CTypedPtrList< CObList, CaURPPrivilegesItemData* >& lp = m_pParam->m_listPrivileges;
	memset (&lvitem, 0, sizeof(lvitem));
	m_cListPrivilege.DeleteAllItems();
	POSITION pos = lp.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = lp.GetNext (pos);

		lvitem.mask   = LVIF_PARAM|LVIF_TEXT;
		lvitem.pszText= tchszText;
		lvitem.iItem  = nCount;
		lvitem.lParam = (LPARAM)pItem;

		m_cListPrivilege.InsertItem (&lvitem);
		nCount++;
	}
	m_cListPrivilege.DisplayPrivileges();
}


void CxDlgProfile::FillGroups()
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

void CxDlgProfile::OnDestroy() 
{
	CDialog::OnDestroy();
	if (m_pParam)
		m_pParam->PopHelpID();
}
