/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipmfast.cpp : implementation file
**    Project  : INGRESII/Monitoring.
**    Author   : Emannuel Blattes
**    Purpose  : implementation file for fast access to an item.
**               thanks to double-click in right pane changing selection in the ipm tree
**
** History:
**
** xx-Feb-1998 (Emannuel Blattes)
**    Created
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "ipmfast.h"
#include "calsctrl.h"

CuIpmTreeFastItem::CuIpmTreeFastItem(BOOL bStatic, int type, void* pFastStruct, BOOL bCheckName, LPCTSTR lpsCheckName)
{
	m_bStatic = bStatic;
	m_type    = type;
	m_pStruct = pFastStruct;
	m_bCheckName = bCheckName;
	if (bCheckName) {
		ASSERT (lpsCheckName);
		if (lpsCheckName)
			m_csCheckName = lpsCheckName;
		else
			m_csCheckName = _T("ERRONEOUS");
	}
}

CuIpmTreeFastItem::~CuIpmTreeFastItem()
{
}


////////////////////////////////////////////////////////////////////////////////////////////
// Functions

BOOL ExpandUpToSearchedItem(CWnd* pPropWnd, CTypedPtrList<CObList, CuIpmTreeFastItem*>& rIpmTreeFastItemList, BOOL bAutomated)
{
	CfIpmFrame* pIpmFrame = NULL;
	if (bAutomated) {
		ASSERT (pPropWnd->IsKindOf(RUNTIME_CLASS(CfIpmFrame)));
		pIpmFrame = (CfIpmFrame*)pPropWnd;
	}
	else {
		pIpmFrame = (CfIpmFrame*)pPropWnd->GetParentFrame();
		ASSERT (pIpmFrame->IsKindOf(RUNTIME_CLASS(CfIpmFrame)));
	}
	ASSERT (pIpmFrame);
	
	// Get selected item handle
	HTREEITEM hItem = 0;
	if (bAutomated) {
		hItem = 0;      // Will force search on all root items
	}
	else {
		hItem = pIpmFrame->GetSelectedItem();
		ASSERT (hItem);
	}
	
	// Loop on fastitemlist
	POSITION pos = rIpmTreeFastItemList.GetHeadPosition();
	while (pos) {
		CuIpmTreeFastItem* pFastItem = rIpmTreeFastItemList.GetNext (pos);
		hItem = pIpmFrame->FindSearchedItem(pFastItem, hItem);
		if (!hItem) {
			ASSERT (FALSE);
			return FALSE;
		}
	}
	pIpmFrame->SelectItem(hItem);
	return TRUE;
}


BOOL IpmCurSelHasStaticChildren(CWnd* pPropWnd)
{
	CfIpmFrame* pIpmFrame = (CfIpmFrame*)pPropWnd->GetParentFrame();
	ASSERT (pIpmFrame->IsKindOf(RUNTIME_CLASS(CfIpmFrame)));

	// Get selected item handle
	HTREEITEM hItem = pIpmFrame->GetSelectedItem();
	ASSERT (hItem);
	if (!hItem)
		return FALSE;
	
	CTreeItem *pItem = (CTreeItem*)pIpmFrame->GetPTree()->GetItemData(hItem);
	ASSERT (pItem);
	if (!pItem)
		return FALSE;
	SubBK subBranchKind = pItem->GetSubBK();
	if (subBranchKind == SUBBRANCH_KIND_STATIC)
		return TRUE;
	else
		return FALSE;
}

void CuListCtrlCleanVoidItemData(CuListCtrl* pControl)
{
	ASSERT(pControl);
	if (!pControl)
		return;
	int i, nCount = pControl->GetItemCount();
	for (i=0; i<nCount; i++)
	{
		void* pStruct = (void*)pControl->GetItemData(i);
		if (pStruct)
			delete pStruct;
		pControl->SetItemData(i, 0);
	}
}

