/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  xDlgRolePriv.cpp : Creates a tab panel for Privileges tab in
**					  "Create/Alter Role" dialog.
**
** History:
**	10-Mar-2010 (drivi01)
**		Created.
*/

#include "stdafx.h"
#include "xDlgRolePriv.h"
#include "dmlurp.h"
#include "xdlgrole.h"


// CxDlgRolePriv dialog
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const int LAYOUT_NUMBER = 2;

IMPLEMENT_DYNAMIC(CxDlgRolePriv, CDialog)

CxDlgRolePriv::CxDlgRolePriv(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgRolePriv::IDD, pParent)
{

}

CxDlgRolePriv::~CxDlgRolePriv()
{
}

void CxDlgRolePriv::InitClassMembers(BOOL bInitMember)
{
	//
	// Must be set first:
	ASSERT (m_pParam);
	if (!m_pParam)
		return;
	if (bInitMember)
	{
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
		int i = 0;
		POSITION pos = m_pParam->m_listPrivileges.GetHeadPosition();
		while (pos != NULL && i < m_pParam->GetMaxPrivilege())
		{
			CaURPPrivilegesItemData* pObj = m_pParam->m_listPrivileges.GetNext(pos);
			m_pParam->m_Privilege[i] = pObj->m_bUser;
			m_pParam->m_bDefaultPrivilege[i] = pObj->m_bDefault;
			i++;
		}

	}
}

void CxDlgRolePriv::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CxDlgRolePriv, CDialog)
END_MESSAGE_MAP()

BOOL CxDlgRolePriv::OnInitDialog() 
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
		{IDS_URP_PRIVILEGE,   170,  LVCFMT_LEFT,   FALSE},
		{IDS_URP_ROLE,         60,  LVCFMT_CENTER, TRUE },
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

	FillPrivileges();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgRolePriv::FillPrivileges()
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

void CxDlgRolePriv::OnOK()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgRole* pParentDlg = (CxDlgRole*)GetParent()->GetParent();
   pParentDlg->OnOK();
}

void CxDlgRolePriv::OnDestroy()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgRole* pParentDlg = (CxDlgRole*)GetParent()->GetParent();
   pParentDlg->OnDestroy();
}

// CxDlgRolePriv message handlers
