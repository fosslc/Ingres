/****************************************************************************
**
**  Copyright (C) 2010 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  xDlgUserPriv.cpp : Creates a tab panel for Privileges tab in
**					  "Create/Alter User" dialog.
**
** History:
**	10-Mar-2010 (drivi01)
**		Created.
*/


#include "stdafx.h"
#include "xDlgUserPriv.h"
#include "dmlurp.h"
#include "xdlguser.h"

// CxDlgUserPriv dialog
static const int LAYOUT_NUMBER = 3;
IMPLEMENT_DYNAMIC(CxDlgUserPriv, CDialogEx)
CaUser *pParam;
CxDlgUser *m_pParent;
CxDlgUserPriv::CxDlgUserPriv(CWnd* pParent /*=NULL*/)
	: CDialogEx(CxDlgUserPriv::IDD, pParent)
{

}

CxDlgUserPriv::~CxDlgUserPriv()
{
}

void CxDlgUserPriv::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CxDlgUserPriv, CDialogEx)
END_MESSAGE_MAP()

void CxDlgUserPriv::InitClassMember(BOOL bInitMember)
{
	ASSERT (pParam);
	if (!pParam)
		return;
	if (bInitMember)
	{
		int i = 0;
		POSITION pos = pParam->m_listPrivileges.GetHeadPosition();
		while (pos != NULL && i < pParam->GetMaxPrivilege())
		{
			CaURPPrivilegesItemData* pObj = pParam->m_listPrivileges.GetNext(pos);
			if (pObj->m_strPrivilege.CompareNoCase("Create Database")==0 && pParam->m_bCreate)
			{
				pParam->m_Privilege[i] = 1;
				pParam->m_bDefaultPrivilege[i] = 1;
			}
			pObj->m_bUser = pParam->m_Privilege[i];
			pObj->m_bDefault = pParam->m_bDefaultPrivilege[i];
			i++;
		}
	}
	else
	{
		int i = 0;
		POSITION pos = pParam->m_listPrivileges.GetHeadPosition();
		while (pos != NULL && i < pParam->GetMaxPrivilege())
		{
			CaURPPrivilegesItemData* pObj = pParam->m_listPrivileges.GetNext(pos);
			pParam->m_Privilege[i] = pObj->m_bUser;
			pParam->m_bDefaultPrivilege[i] = pObj->m_bDefault;
			i++;
		}
	}
}

BOOL CxDlgUserPriv::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_pParent = (CxDlgUser *)GetParent()->GetParent();
	pParam = m_pParent->m_pParam;
	InitClassMember();

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
		{IDS_URP_USER,         110,  LVCFMT_CENTER, TRUE },
		{IDS_URP_DEFAULT,      100,  LVCFMT_CENTER, TRUE },
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
	return TRUE;

}

void CxDlgUserPriv::FillPrivileges()
{
	TCHAR tchszText[] =_T("");
	int nCount = 0;
	LV_ITEM lvitem;
	CaURPPrivilegesItemData* pItem = NULL;
	CTypedPtrList< CObList, CaURPPrivilegesItemData* >& lp = pParam->m_listPrivileges;
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

void CxDlgUserPriv::OnOK()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgUser* pParentDlg = (CxDlgUser*)GetParent()->GetParent();
   pParentDlg->OnOK();
}

void CxDlgUserPriv::OnCancel()
{
   ASSERT (GetParent()->GetParent()->IsKindOf(RUNTIME_CLASS(CDialogEx)));
   CxDlgUser* pParentDlg = (CxDlgUser*)GetParent()->GetParent();
   pParentDlg->OnCancel();
}
// CxDlgUserPriv message handlers
