/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : treenode.cpp, Implementation File
**
**
**    Project  : Virtual Node Manager.
**    Author   : UK Sotheavut,
**
**    Purpose  : Root node of Tree Node Data (Implement as Template
**               list of Objects)
**
**  History:
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 22-Nov-2004 (schph01)
**    BUG #113511 replaceCompareNoCase function by CollateNoCase
**    according to the LC_COLLATE category setting of the locale code
**    page currently in  use.
*/

#include "stdafx.h"
#include "vnodemgr.h"
#include "treenode.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CaTreeNodeFolder::CaTreeNodeFolder():CaVNodeTreeItemData()
{
	m_nodeInfo.SetImage (IM_FOLDER_CLOSE);
	m_DummyItem.SetName (theApp.m_strNoNodes);
	m_DummyItem.GetNodeInformation().SetImage (IM_EMPTY_NODE);
}

CaTreeNodeFolder::~CaTreeNodeFolder()
{
	while (!m_listNode.IsEmpty())
		delete m_listNode.RemoveTail();
}

CaVNodeTreeItemData* CaTreeNodeFolder::FindNode (LPCTSTR lpszNodeName)
{
	CaVNodeTreeItemData* pNode = NULL;
	CString strNodeName;
	POSITION pos = m_listNode.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_listNode.GetNext (pos);
		strNodeName = pNode->GetName();
		if (strNodeName.CompareNoCase (lpszNodeName) == 0)
			return pNode;
	}
	return NULL;
}


BOOL CaTreeNodeFolder::Refresh()
{
	CString strNodeName;
	CaVNodeTreeItemData* pNode = NULL;
	CaVNodeTreeItemData* pExistNode = NULL;
	POSITION pos = m_listNode.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_listNode.GetNext (pos);
		pNode->SetInternalState (NODE_DELETE);
	}
	
	CTypedPtrList<CObList, CaVNodeTreeItemData*> listNode;
	VNODE_QueryNode (m_listNode, listNode);
	pos = listNode.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = listNode.GetNext (pos);
		m_listNode.AddTail (pNode);
	}
	return TRUE;
}

static int CALLBACK fnCompareComponent(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CaVNodeTreeItemData* pComponent1 = (CaVNodeTreeItemData*)lParam1;
	CaVNodeTreeItemData* pComponent2 = (CaVNodeTreeItemData*)lParam2;
	if (pComponent1 == NULL || pComponent2 == NULL)
		return 0;

	if (pComponent1->GetOrder() == NODE_ODER_NODENORMAL && pComponent1->GetOrder() == NODE_ODER_NODENORMAL)
	{
		return pComponent1->GetName().CollateNoCase (pComponent2->GetName());
	}
	else
	{
		if (pComponent1->GetOrder() < pComponent2->GetOrder())
			return -1;
		else
		if (pComponent1->GetOrder() > pComponent2->GetOrder())
			return 1;
		else
			return 0;
	}
}


void CaTreeNodeFolder::Display (CTreeCtrl* pTree)
{
	CaVNodeTreeItemData* pNode = NULL;
	HTREEITEM hTreeRoot = m_nodeInfo.GetTreeItem();
	if (!hTreeRoot)
	{
		//
		// Create a Root Item:
		hTreeRoot = pTree->InsertItem (
			theApp.m_strFolderNode, 
			m_nodeInfo.GetImage(), 
			m_nodeInfo.GetImage(), 
			TVI_ROOT, 
			TVI_FIRST);
		if (hTreeRoot)
		{
			pTree->SetItemData (hTreeRoot, (DWORD_PTR)this);
			m_nodeInfo.SetTreeItem (hTreeRoot);
		}
	}

	POSITION p, pos = m_listNode.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pNode = m_listNode.GetNext (pos);
		if (pNode->GetInternalState() != NODE_DELETE)
		{
			pNode->Display (pTree, hTreeRoot);
			pNode->SetInternalState (NODE_EXIST);
		}
		else
		{
			CaTreeNodeInformation& nodeInfo = pNode->GetNodeInformation();
			HTREEITEM hTreeItem = nodeInfo.GetTreeItem();
			if (hTreeItem)
			{
				pTree->DeleteItem (hTreeItem);
				m_listNode.RemoveAt (p);
				delete pNode;
			}
		}
	}
	if (m_listNode.IsEmpty())
	{
		if (!m_DummyItem.GetNodeInformation().GetTreeItem())
		{
			HTREEITEM hTreeDummyItem = VNODE_TreeAddItem (
				m_DummyItem.GetName(), 
				pTree, 
				m_nodeInfo.GetTreeItem(),
				TVI_FIRST,
				m_DummyItem.GetNodeInformation().GetImage(), (DWORD_PTR)&m_DummyItem);
			m_DummyItem.GetNodeInformation().SetTreeItem(hTreeDummyItem);
		}
	}
	else
	{
		if (m_DummyItem.GetNodeInformation().GetTreeItem())
		{
			pTree->DeleteItem (m_DummyItem.GetNodeInformation().GetTreeItem());
			m_DummyItem.GetNodeInformation().SetTreeItem(NULL);
		}
	}
	pTree->Expand (m_nodeInfo.GetTreeItem(), TVE_EXPAND);

	TV_SORTCB sortcb;
	sortcb.hParent = m_nodeInfo.GetTreeItem();
	sortcb.lpfnCompare = fnCompareComponent;
	sortcb.lParam = NULL;
	if (!sortcb.hParent)
		return;
	pTree->SortChildrenCB (&sortcb);
}

BOOL CaTreeNodeFolder::OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand)
{
	m_nodeInfo.SetImage (bExpand? IM_FOLDER_OPEN: IM_FOLDER_CLOSE);
	pTree->SetItemImage (hItem, m_nodeInfo.GetImage(), m_nodeInfo.GetImage());
	m_nodeInfo.SetExpanded (bExpand);
	return TRUE;
}

BOOL CaTreeNodeFolder::Add()
{
	return VNODE_Add();
}
