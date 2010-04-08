/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ctltree.cpp : implementation file.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : CTreeCtrl derived class to be used when creating the control
**               without the left pane (tree view)
**
** History:
**
** 15-Apr-2002 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ctltree.h"
#include "ipmdoc.h"
#include "vtree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CuTreeCtrlInvisible::CuTreeCtrlInvisible()
{
	m_pDoc = NULL;
}

CuTreeCtrlInvisible::~CuTreeCtrlInvisible()
{
}


BEGIN_MESSAGE_MAP(CuTreeCtrlInvisible, CTreeCtrl)
	//{{AFX_MSG_MAP(CuTreeCtrlInvisible)
	ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemexpanding)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CuTreeCtrlInvisible::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;
}

void CuTreeCtrlInvisible::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;
}

void CuTreeCtrlInvisible::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

void CuTreeCtrlInvisible::OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;

	if (!m_pDoc)
		return;
	// Manage "update all after load on first action"
	if (m_pDoc->ManageMonSpecialState()) 
	{
		*pResult = 1;   // do not allow expanding
		return;
	}


	if (pNMTreeView->action == TVE_EXPAND) 
	{
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;
		CTreeItem* pItem;
		pItem = (CTreeItem *)GetItemData(hItem);
		if (pItem && !pItem->IsAlreadyExpanded())
		{
			if (pItem->CreateSubBranches(hItem))
				pItem->SetAlreadyExpanded(TRUE);
			else
				*pResult = 1;     // prevent expanding
		}
	}
}

void CuTreeCtrlInvisible::OnDestroy() 
{
	// delete itemdata associated with each item
	CTreeCtrl::OnDestroy();
}
