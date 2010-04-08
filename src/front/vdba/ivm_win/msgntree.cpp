/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : msgntree.cpp, Implementation file
**  Project  : Visual Manager
**  Author   : UK Sotheavut
**
**  Purpose  : Define Message Categories and Notification Levels
**		Defenition of Tree Node of Message Category
**
**  History:
**	21-May-1999 (uk$so01)
**	    Created.
**	09-Feb-2000 (somsa01)
**	    Changed C-style short hand compare & set to normal "if...else"
**	    block. This is so that, on platforms such as HP, we can build
**	    the file in debug (if we need to).
**	31-May-2000 (uk$so01)
**	    bug 99242 Handle DBCS
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "msgcateg.h"
#include "msgntree.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



static void UpdateStateFolder (CaMessageNodeTreeInternal* pFolder, CaMessageNode* pMsgNode, Imsgclass nClass)
{
	CTypedPtrList<CObList, CaMessageNode*>* pLs = pFolder->GetListSubNode();
	if (!pLs)
	{
		pLs = new CTypedPtrList<CObList, CaMessageNode*>;
		pFolder->SetListSubNode(pLs);
	}

	CaMessageNodeTreeStateFolder* pNewFolder = new CaMessageNodeTreeStateFolder();
	pNewFolder->GetNodeInformation().SetNodeTitle (pMsgNode->GetNodeInformation().GetNodeTitle());
	pNewFolder->GetNodeInformation().SetNodeType (NODE_NEW);

	int nClose = IMGS_FOLDER_CLOSE, nOpen = IMGS_FOLDER_OPEN;
	MSGSourceTarget ddSourceTarget = DD_UNDEFINED;
	switch (nClass)
	{
	case IMSG_ALERT:
		nClose = IMGS_FOLDER_A_CLOSE;
		nOpen = IMGS_FOLDER_A_OPEN;
		ddSourceTarget = SV_UFA;
		break;
	case IMSG_NOTIFY:
		nClose = IMGS_FOLDER_N_CLOSE;
		nOpen = IMGS_FOLDER_N_OPEN;
		ddSourceTarget = SV_UFN;
		break;
	case IMSG_DISCARD:
		nClose = IMGS_FOLDER_D_CLOSE;
		nOpen = IMGS_FOLDER_D_OPEN;
		ddSourceTarget = SV_UFD;
		break;
	default:
		break;
	}
	CaMessageNodeInfo& nodeInfo = pNewFolder->GetNodeInformation();
	nodeInfo.SetIconImage (nClose, nOpen);
	nodeInfo.SetMsgSourceTarget(ddSourceTarget);
	
	pLs->AddTail (pNewFolder);
	pNewFolder->UpdateTree(pMsgNode, nClass);

}


static void DestroyTreeCtrlFolder (CTreeCtrl* pTree, CaMessageNode* pNode)
{
	if (pNode->IsInternal())
	{
		HTREEITEM hTreeItem = NULL;
		CaMessageNode* pObj = NULL;
		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pNode;
		CTypedPtrList<CObList, CaMessageNode*>* lPs = pI->GetListSubNode();
		if (lPs)
		{
			POSITION pos = lPs->GetHeadPosition();
			while (pos != NULL)
			{
				pObj = lPs->GetNext (pos);
				if (!pNode->IsInternal())
				{
					hTreeItem = pObj->GetNodeInformation().GetHTreeItem();
					if (hTreeItem)
						pTree->DeleteItem (hTreeItem);
					pObj->GetNodeInformation().SetHTreeItem(NULL);
				}
				else
				{
					DestroyTreeCtrlFolder (pTree, pObj);
				}
			}
		}
		hTreeItem = pNode->GetNodeInformation().GetHTreeItem();
		if (hTreeItem)
			pTree->DeleteItem (hTreeItem);
		pNode->GetNodeInformation().SetHTreeItem(NULL);
	}
}

IMPLEMENT_SERIAL (CaMessageNodeInfo, CObject, 1)
void CaMessageNode::SortItems(CTreeCtrl* pTree)
{
	TV_SORTCB tvsort;
	tvsort.hParent = GetNodeInformation().GetHTreeItem();
	tvsort.lpfnCompare = CaTreeMessageCategory::CompareItem;
	tvsort.lParam = NULL;
	pTree->SortChildrenCB (&tvsort);
}


//
// LEAF: A general class of Node of Tree's Message Category:
CaMessageNodeTree::CaMessageNodeTree():CaMessageNode()
{
	m_pMessageCategory = NULL;
	m_nodeInfo.SetOrder (ORDER_LEAF);
}

CaMessageNodeTree::CaMessageNodeTree(CaMessage* pMessage):CaMessageNode()
{
	m_pMessageCategory = pMessage;
	m_nodeInfo.SetOrder (ORDER_LEAF);
}

CaMessageNodeTree::~CaMessageNodeTree()
{

}


void CaMessageNodeTree::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	ASSERT (m_pMessageCategory);
	if (!m_pMessageCategory)
		return;

	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		//CString strTitle = m_pMessageCategory->GetTitle();
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree,
			TVI_LAST);
		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
		}
	}

	if (m_nodeInfo.GetHTreeItem())
	{
		int nImageClose = IMGS_ITEM_N, nImageOpen = IMGS_ITEM_N;
		switch (m_pMessageCategory->GetClass())
		{
		case IMSG_ALERT:
			nImageClose = IMGS_ITEM_A;
			nImageOpen = IMGS_ITEM_A;
			break;
		case IMSG_NOTIFY:
			nImageClose = IMGS_ITEM_N;
			nImageOpen = IMGS_ITEM_N;
			break;
		case IMSG_DISCARD:
			nImageClose = IMGS_ITEM_D;
			nImageOpen = IMGS_ITEM_D;
			break;
		}
		m_nodeInfo.SetIconImage (nImageClose, nImageOpen);
		pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageOpen);
	}

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}
}


void CaMessageNodeTree::AlertMessage(CTreeCtrl* pTree)
{
	if (!pTree)
		return;
	if (!m_pMessageCategory)
		return;
	m_pMessageCategory->SetClass (IMSG_ALERT);
}

void CaMessageNodeTree::NotifyMessage(CTreeCtrl* pTree)
{
	if (!pTree)
		return;
	if (!m_pMessageCategory)
		return;
	m_pMessageCategory->SetClass (IMSG_NOTIFY);
}

void CaMessageNodeTree::DiscardMessage(CTreeCtrl* pTree)
{
	if (!pTree)
		return;
	if (!m_pMessageCategory)
		return;
	m_pMessageCategory->SetClass (IMSG_DISCARD);
}

//
// INTERNAL: A general class of Internal Node of Tree's Message Category:
CaMessageNodeTreeInternal::CaMessageNodeTreeInternal():CaMessageNode()
{
	m_pListSubNode = NULL;
}

CaMessageNodeTreeInternal::~CaMessageNodeTreeInternal()
{
	if (!m_pListSubNode)
		return;
	while (!m_pListSubNode->IsEmpty())
		delete m_pListSubNode->RemoveHead();
	delete m_pListSubNode;
	m_pListSubNode = NULL;
}

void CaMessageNodeTreeInternal::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree,
			TVI_LAST);
		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
		}
	}

	if (m_nodeInfo.GetHTreeItem())
	{
		int nImageClose, nImageOpen;
		m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
		if (m_nodeInfo.IsExpanded())
			pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
		else
			pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
	}

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}

	if (m_pListSubNode && m_nodeInfo.GetHTreeItem())
	{

		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			if (pNode)
			{
				pNode->Display(pTree, m_nodeInfo.GetHTreeItem());
			}
		}
	}
}

void CaMessageNodeTreeInternal::AddMessage (CaMessageNode* pNode)
{
	if (!m_pListSubNode)
		m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;

	m_pListSubNode->AddTail (pNode);
}

void CaMessageNodeTreeInternal::AddFolder (CTreeCtrl* pTree)
{
	//
	// Each Folder will be added before the node <Unclassified Messages>

	try
	{
		CString strItem;
		CString strNewFolder;
		if (!strNewFolder.LoadString (IDS_NEW_FOLDER))
			strNewFolder = _T("New Folder");
		//
		// Check to see if the folder exists:
		strItem = strNewFolder;
		int nCount = 0;
		if (m_pListSubNode)
		{
			CaMessageNode* pNode = NULL;
			POSITION pos = m_pListSubNode->GetHeadPosition();
			while (pos != NULL)
			{
				pNode = m_pListSubNode->GetNext (pos);
				if (strItem.CompareNoCase (pNode->GetNodeInformation().GetNodeTitle()) == 0)
				{
					nCount++;
					strItem.Format (_T("%s %d"), (LPCTSTR)strNewFolder, nCount);
					pos = m_pListSubNode->GetHeadPosition();
				}
			}
			strNewFolder = strItem;
		}

		CaMessageNodeTreeInternal* pNewFolder = new CaMessageNodeTreeInternal();
		if (!m_pListSubNode)
			m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
		pNewFolder->GetNodeInformation().SetNodeTitle(strNewFolder);
		m_pListSubNode->AddTail (pNewFolder);
		pNewFolder->Display (pTree, m_nodeInfo.GetHTreeItem());

		if (!m_nodeInfo.IsExpanded())
		{
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
			m_nodeInfo.SetExpanded (TRUE);
		}
		
		pTree->SelectItem((pNewFolder->GetNodeInformation()).GetHTreeItem());
		pTree->EditLabel ((pNewFolder->GetNodeInformation()).GetHTreeItem());
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CaMessageNodeTreeInternal::AddFolder): \nAdd New Folder failed"));
	}
}



void CaMessageNodeTreeInternal::AlertMessage(CTreeCtrl* pTree)
{
	if (!m_pListSubNode)
		return;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
		pNode->AlertMessage(pTree);
	}
}

void CaMessageNodeTreeInternal::NotifyMessage(CTreeCtrl* pTree)
{
	if (!m_pListSubNode)
		return;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
		pNode->NotifyMessage(pTree);
	}
}

void CaMessageNodeTreeInternal::DiscardMessage(CTreeCtrl* pTree)
{
	if (!m_pListSubNode)
		return;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
		pNode->DiscardMessage(pTree);
	}
}


// ********************* Tree of State View ******************** 
//

//
// STATE VIEW: An Internal Node (Others) of Tree's Message Category:
CaMessageNodeTreeStateOthers::CaMessageNodeTreeStateOthers()
{
	m_nodeInfo.SetOrder (ORDER_OTHERS);
}

CaMessageNodeTreeStateOthers::~CaMessageNodeTreeStateOthers()
{

}


void CaMessageNodeTreeStateOthers::CleanData()
{
	m_nodeInfo.SetHTreeItem (NULL);
	if (m_pListSubNode)
	{
		while (!m_pListSubNode->IsEmpty())
			delete m_pListSubNode->RemoveHead();
	}
}


void CaMessageNodeTreeStateOthers::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if ((nMode & SHOW_CATEGORY_BRANCH) && m_nodeInfo.GetHTreeItem() == NULL)
	{
		//
		// Display the main branch <Unclassified Messages>
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree,
			TVI_LAST);
		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}

	if ((nMode & SHOW_CATEGORY_BRANCH) && !m_nodeInfo.GetHTreeItem())
		return;

	if (m_pListSubNode)
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			HTREEITEM hParent = (nMode & SHOW_CATEGORY_BRANCH)? m_nodeInfo.GetHTreeItem(): hTree;
			
			if (!pNode->IsInternal())
			{
				CaMessageNodeTree* pLeaf = (CaMessageNodeTree*)pNode;
				CaMessage* pMsg = pLeaf->GetMessageCategory();
				if (m_nClass == IMSG_ALERT && pMsg->GetClass() != IMSG_ALERT)
					continue;
				if (m_nClass == IMSG_NOTIFY && pMsg->GetClass() != IMSG_NOTIFY)
					continue;
				if (m_nClass == IMSG_DISCARD && pMsg->GetClass() != IMSG_DISCARD)
					continue;
			}
			pNode->Display(pTree, hParent, nMode);
		}
	}
	SortItems (pTree);

	if (nMode & SHOW_CATEGORY_BRANCH)
	{
		if (m_nodeInfo.IsExpanded())
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
		else
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
	}
}

void CaMessageNodeTreeStateOthers::SortItems(CTreeCtrl* pTree)
{
	TV_SORTCB tvsort;
	tvsort.hParent = GetNodeInformation().GetHTreeItem();
	tvsort.lpfnCompare = CaTreeMessageState::CompareItem;
	tvsort.lParam = NULL;
	if (tvsort.hParent)
		pTree->SortChildrenCB (&tvsort);
}


void CaMessageNodeTreeStateOthers::Populate (CTypedPtrList<CObList, CaMessageNode*>* pListOthers, Imsgclass nType)
{
	CaMessageNode* pMsgNode = NULL;
	CaMessage* pMsg = NULL;
	ASSERT (pListOthers);
	if (!pListOthers)
		return;
	POSITION pos = pListOthers->GetHeadPosition();
	while (pos != NULL)
	{
		pMsgNode = pListOthers->GetNext (pos);
		ASSERT (pMsgNode->IsInternal() == FALSE);
		if (pMsgNode->IsInternal())
			continue;
		CaMessageNodeTree* pNt = (CaMessageNodeTree*)pMsgNode;
		pMsg = pNt->GetMessageCategory();
		if (nType == pMsg->GetClass())
		{
			if (!m_pListSubNode)
				m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
			CaMessageNodeTree* pNewNode = new CaMessageNodeTree(pMsg);
			pNewNode->GetNodeInformation().SetNodeTitle(pMsg->GetTitle());
			pNewNode->GetNodeInformation().SetMsgSourceTarget(SV_O);
			m_pListSubNode->AddTail (pNewNode);
		}
	}
}


//
// STATE VIEW: An Internal Node (Alert) of Tree's Message Category:
CaMessageNodeTreeStateAlert::CaMessageNodeTreeStateAlert():CaMessageNodeTreeInternal()
{
	CString strTitle;
	if (!strTitle.LoadString(IDS_ALERT))
		strTitle = _T("<Alert>");
	m_nodeInfo.SetNodeTitle (strTitle);
	m_nodeInfo.SetOrder (ORDER_ALERT);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_A_CLOSE, IMGS_FOLDER_A_OPEN);
	m_nodeInfo.SetMsgSourceTarget(SV_FA);

	if (!strTitle.LoadString(IDS_UNCLASSIFIED_MESSAGE))
		strTitle = _T("<Unclassified Messages>");
	m_categoryOthers.SetClassification(IMSG_ALERT);
	m_categoryOthers.GetNodeInformation().SetNodeTitle(strTitle);
	m_categoryOthers.GetNodeInformation().SetIconImage (IMGS_FOLDER_A_CLOSE, IMGS_FOLDER_A_OPEN);
	m_categoryOthers.GetNodeInformation().SetMsgSourceTarget(SV_OFA);;
}

CaMessageNodeTreeStateAlert::~CaMessageNodeTreeStateAlert()
{

}

void CaMessageNodeTreeStateAlert::CleanData()
{
	m_categoryOthers.CleanData();
	if (m_pListSubNode)
	{
		while (!m_pListSubNode->IsEmpty())
			delete m_pListSubNode->RemoveHead();
	}
}

void CaMessageNodeTreeStateAlert::Populate (CTreeCtrl* pTree, CaTreeMessageCategory* pFrom)
{
	CaMessageNodeTreeCategoryOthers& others = pFrom->GetCategoryOthers();
	CTypedPtrList<CObList, CaMessageNode*>* pListOthers = others.GetListSubNode();
	m_categoryOthers.Populate(pListOthers, IMSG_ALERT);

	CTypedPtrList<CObList, CaMessageNode*>* pListUserCategory = pFrom->GetListSubNode();
	if (pListUserCategory && !pListUserCategory->IsEmpty())
	{
		//
		// Update all the User folders:
		CaMessageNode* pMsgNode = NULL;
		CTypedPtrList<CObList, CaMessageNode*>* pLs = NULL;
		POSITION pos = pListUserCategory->GetHeadPosition();
		while (pos != NULL)
		{
			pMsgNode = pListUserCategory->GetNext (pos);
			if (pMsgNode->IsInternal())
			{
				UpdateStateFolder (this,  pMsgNode, IMSG_ALERT);
			}
			else
			{
				CaMessageNodeTree* pNt = (CaMessageNodeTree*)pMsgNode;
				CaMessage* pMsg = pNt->GetMessageCategory();
				if (pMsg->GetClass() == IMSG_ALERT)
				{
					if (!m_pListSubNode)
					{
						m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
					}
					CaMessageNodeTree* pNewNode = new CaMessageNodeTree(pMsg);
					pNewNode->GetNodeInformation().SetNodeTitle(pMsg->GetTitle());
					pNewNode->GetNodeInformation().SetMsgSourceTarget(SV_U);
					m_pListSubNode->AddTail (pNewNode);
				}
			}
		}
	}
}



void CaMessageNodeTreeStateAlert::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		//
		// Main Node: <Alert>
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem  = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree, 
			TVI_LAST);

		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType  (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}
	//
	// Display the Categories:
	if (m_pListSubNode && m_nodeInfo.GetHTreeItem())
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			DestroyTreeCtrlFolder(pTree, pNode);
			pNode->Display(pTree, m_nodeInfo.GetHTreeItem(), nMode);
		}
	}

	//
	// Display the unclassified Category (Others):
	m_categoryOthers.Display (pTree, m_nodeInfo.GetHTreeItem(), nMode);

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}

	SortItems (pTree);
	if (m_nodeInfo.IsExpanded())
		pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
	else
		pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
}

void CaMessageNodeTreeStateAlert::SortItems(CTreeCtrl* pTree)
{
	TV_SORTCB tvsort;
	tvsort.hParent = GetNodeInformation().GetHTreeItem();
	tvsort.lpfnCompare = CaTreeMessageState::CompareItem;
	tvsort.lParam = NULL;
	pTree->SortChildrenCB (&tvsort);
	m_categoryOthers.SortItems (pTree);
}

void CaMessageNodeTreeStateAlert::NotifyMessage(CTreeCtrl* pTree)
{
	if (m_pListSubNode)
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			pNode->NotifyMessage(pTree);
		}
	}
	m_categoryOthers.NotifyMessage(pTree);
}

void CaMessageNodeTreeStateAlert::DiscardMessage(CTreeCtrl* pTree)
{
	if (m_pListSubNode)
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			pNode->DiscardMessage(pTree);
		}
	}
	m_categoryOthers.DiscardMessage(pTree);
}




//
// STATE VIEW: An Internal Node (Notify) of Tree's Message Category:
CaMessageNodeTreeStateNotify::CaMessageNodeTreeStateNotify():CaMessageNodeTreeInternal()
{
	CString strTitle;
	if (!strTitle.LoadString(IDS_NOTIFY))
		strTitle = _T("<Notify>");
	m_nodeInfo.SetNodeTitle (strTitle);
	m_nodeInfo.SetOrder (ORDER_NOTIFY);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_N_CLOSE, IMGS_FOLDER_N_OPEN);
	m_nodeInfo.SetMsgSourceTarget(SV_FN);

	if (!strTitle.LoadString(IDS_UNCLASSIFIED_MESSAGE))
		strTitle = _T("<Unclassified Messages>");
	m_categoryOthers.SetClassification(IMSG_NOTIFY);
	m_categoryOthers.GetNodeInformation().SetNodeTitle(strTitle);
	m_categoryOthers.GetNodeInformation().SetIconImage (IMGS_FOLDER_N_CLOSE, IMGS_FOLDER_N_OPEN);
	m_categoryOthers.GetNodeInformation().SetMsgSourceTarget(SV_OFN);
}

CaMessageNodeTreeStateNotify::~CaMessageNodeTreeStateNotify()
{
}

void CaMessageNodeTreeStateNotify::Populate (CTreeCtrl* pTree, CaTreeMessageCategory* pFrom)
{
	CaMessageNodeTreeCategoryOthers& others = pFrom->GetCategoryOthers();
	CTypedPtrList<CObList, CaMessageNode*>* pListOthers = others.GetListSubNode();
	m_categoryOthers.Populate(pListOthers, IMSG_NOTIFY);

	CTypedPtrList<CObList, CaMessageNode*>* pListUserCategory = pFrom->GetListSubNode();
	if (pListUserCategory && !pListUserCategory->IsEmpty())
	{
		//
		// Update all the User folders:
		CaMessageNode* pMsgNode = NULL;
		CTypedPtrList<CObList, CaMessageNode*>* pLs = NULL;
		POSITION pos = pListUserCategory->GetHeadPosition();
		while (pos != NULL)
		{
			pMsgNode = pListUserCategory->GetNext (pos);
			if (pMsgNode->IsInternal())
			{
				UpdateStateFolder (this,  pMsgNode, IMSG_NOTIFY);
			}
			else
			{
				CaMessageNodeTree* pNt = (CaMessageNodeTree*)pMsgNode;
				CaMessage* pMsg = pNt->GetMessageCategory();
				if (pMsg->GetClass() == IMSG_NOTIFY)
				{
					if (!m_pListSubNode)
					{
						m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
					}
					CaMessageNodeTree* pNewNode = new CaMessageNodeTree(pMsg);
					pNewNode->GetNodeInformation().SetNodeTitle(pMsg->GetTitle());
					pNewNode->GetNodeInformation().SetMsgSourceTarget(SV_U);
					m_pListSubNode->AddTail (pNewNode);
				}
			}
		}
	}
}

void CaMessageNodeTreeStateNotify::CleanData()
{
	m_categoryOthers.CleanData();
	if (m_pListSubNode)
	{
		while (!m_pListSubNode->IsEmpty())
			delete m_pListSubNode->RemoveHead();
	}
}


void CaMessageNodeTreeStateNotify::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		//
		// Main Node: <Notify>
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem  = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree, 
			TVI_LAST);

		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType  (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}
	//
	// Display the Categories:
	if (m_pListSubNode && m_nodeInfo.GetHTreeItem())
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			DestroyTreeCtrlFolder(pTree, pNode);
			pNode->Display(pTree, m_nodeInfo.GetHTreeItem(), nMode);
		}
	}

	//
	// Display the unclassified Category (Others):
	m_categoryOthers.Display (pTree, m_nodeInfo.GetHTreeItem(), nMode);

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}
	SortItems (pTree);

	if (m_nodeInfo.IsExpanded())
		pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
	else
		pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
}

void CaMessageNodeTreeStateNotify::SortItems(CTreeCtrl* pTree)
{
	TV_SORTCB tvsort;
	tvsort.hParent = GetNodeInformation().GetHTreeItem();
	tvsort.lpfnCompare = CaTreeMessageState::CompareItem;
	tvsort.lParam = NULL;
	pTree->SortChildrenCB (&tvsort);
	m_categoryOthers.SortItems (pTree);
}

void CaMessageNodeTreeStateNotify::AlertMessage(CTreeCtrl* pTree)
{
	if (m_pListSubNode)
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			pNode->AlertMessage(pTree);
		}
	}
	m_categoryOthers.AlertMessage(pTree);
}

void CaMessageNodeTreeStateNotify::DiscardMessage(CTreeCtrl* pTree)
{
	if (m_pListSubNode)
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			pNode->DiscardMessage(pTree);
		}
	}
	m_categoryOthers.DiscardMessage(pTree);
}


//
// STATE VIEW: An Internal Node (Discard) of Tree's Message Category:
CaMessageNodeTreeStateDiscard::CaMessageNodeTreeStateDiscard():CaMessageNodeTreeInternal()
{
	CString strTitle;
	if (!strTitle.LoadString(IDS_DISCARD))
		strTitle = _T("<Discard>");
	m_nodeInfo.SetNodeTitle (strTitle);
	m_nodeInfo.SetOrder (ORDER_DISCARD);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_D_CLOSE, IMGS_FOLDER_D_OPEN);
	m_nodeInfo.SetMsgSourceTarget(SV_FD);

	if (!strTitle.LoadString(IDS_UNCLASSIFIED_MESSAGE))
		strTitle = _T("<Unclassified Messages>");
	m_categoryOthers.SetClassification(IMSG_DISCARD);
	m_categoryOthers.GetNodeInformation().SetNodeTitle(strTitle);
	m_categoryOthers.GetNodeInformation().SetIconImage (IMGS_FOLDER_D_CLOSE, IMGS_FOLDER_D_OPEN);
	m_categoryOthers.GetNodeInformation().SetMsgSourceTarget(SV_OFD);
}

CaMessageNodeTreeStateDiscard::~CaMessageNodeTreeStateDiscard()
{

}


void CaMessageNodeTreeStateDiscard::Populate (CTreeCtrl* pTree, CaTreeMessageCategory* pFrom)
{
	CaMessageNodeTreeCategoryOthers& others = pFrom->GetCategoryOthers();
	CTypedPtrList<CObList, CaMessageNode*>* pListOthers = others.GetListSubNode();
	m_categoryOthers.Populate(pListOthers, IMSG_DISCARD);

	CTypedPtrList<CObList, CaMessageNode*>* pListUserCategory = pFrom->GetListSubNode();
	if (pListUserCategory && !pListUserCategory->IsEmpty())
	{
		//
		// Update all the User folders:
		CaMessageNode* pMsgNode = NULL;
		CTypedPtrList<CObList, CaMessageNode*>* pLs = NULL;
		POSITION pos = pListUserCategory->GetHeadPosition();
		while (pos != NULL)
		{
			pMsgNode = pListUserCategory->GetNext (pos);
			if (pMsgNode->IsInternal())
			{
				UpdateStateFolder (this,  pMsgNode, IMSG_DISCARD);
			}
			else
			{
				CaMessageNodeTree* pNt = (CaMessageNodeTree*)pMsgNode;
				CaMessage* pMsg = pNt->GetMessageCategory();
				if (pMsg->GetClass() == IMSG_DISCARD)
				{
					if (!m_pListSubNode)
					{
						m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
					}
					CaMessageNodeTree* pNewNode = new CaMessageNodeTree(pMsg);
					pNewNode->GetNodeInformation().SetNodeTitle(pMsg->GetTitle());
					pNewNode->GetNodeInformation().SetMsgSourceTarget(SV_U);
					m_pListSubNode->AddTail (pNewNode);
				}
			}
		}
	}
}

void CaMessageNodeTreeStateDiscard::CleanData()
{
	m_categoryOthers.CleanData();
	if (m_pListSubNode)
	{
		while (!m_pListSubNode->IsEmpty())
			delete m_pListSubNode->RemoveHead();
	}
}

void CaMessageNodeTreeStateDiscard::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		//
		// Main Node: <Discard>
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem  = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree, 
			TVI_LAST);

		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType  (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}
	//
	// Display the Categories:
	if (m_pListSubNode && m_nodeInfo.GetHTreeItem())
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			DestroyTreeCtrlFolder(pTree, pNode);
			pNode->Display(pTree, m_nodeInfo.GetHTreeItem(), nMode);
		}
	}

	//
	// Display the unclassified Category (Others):
	m_categoryOthers.Display (pTree, m_nodeInfo.GetHTreeItem(), nMode);

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}
	SortItems (pTree);

	if (m_nodeInfo.IsExpanded())
		pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
	else
		pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
}

void CaMessageNodeTreeStateDiscard::SortItems(CTreeCtrl* pTree)
{
	TV_SORTCB tvsort;
	tvsort.hParent = GetNodeInformation().GetHTreeItem();
	tvsort.lpfnCompare = CaTreeMessageState::CompareItem;
	tvsort.lParam = NULL;
	pTree->SortChildrenCB (&tvsort);
	m_categoryOthers.SortItems (pTree);
}

void CaMessageNodeTreeStateDiscard::AlertMessage(CTreeCtrl* pTree)
{
	if (m_pListSubNode)
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			pNode->AlertMessage(pTree);
		}
	}
	m_categoryOthers.AlertMessage(pTree);
}

void CaMessageNodeTreeStateDiscard::NotifyMessage(CTreeCtrl* pTree)
{
	if (m_pListSubNode)
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			pNode->NotifyMessage(pTree);
		}
	}
	m_categoryOthers.NotifyMessage(pTree);
}



//
// STATE VIEW: An Internal Node (Folder & Sub Folder) of Tree's Message Category:
CaMessageNodeTreeStateFolder::CaMessageNodeTreeStateFolder()
{
	m_nodeInfo.SetOrder (ORDER_USER);
	if (!m_pListSubNode)
		m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
}

CaMessageNodeTreeStateFolder::~CaMessageNodeTreeStateFolder()
{
	if (m_pListSubNode)
	{
		while (!m_pListSubNode->IsEmpty())
			delete m_pListSubNode->RemoveHead();
	}
}

void CaMessageNodeTreeStateFolder::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if ((nMode & SHOW_CATEGORY_BRANCH) && m_nodeInfo.GetHTreeItem() == NULL)
	{
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree,
			TVI_LAST);
		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}

	//
	// If create node failed !!!!
	if ((nMode & SHOW_CATEGORY_BRANCH) && !m_nodeInfo.GetHTreeItem())
		return;

	if (m_pListSubNode)
	{
		HTREEITEM hParent = (nMode & SHOW_CATEGORY_BRANCH)? m_nodeInfo.GetHTreeItem(): hTree;
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			DestroyTreeCtrlFolder(pTree, pNode);
			pNode->Display(pTree, hParent, nMode);
		}
	}

	if (nMode & SHOW_CATEGORY_BRANCH)
	{
		if (m_nodeInfo.IsExpanded())
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
		else
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
	}
}


static void StateViewCopyFolder (CaMessageNodeTreeStateFolder* pDest, CaMessageNode* pSource, Imsgclass nClass)
{
	if (pSource->IsInternal())
	{
		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pSource;
		CTypedPtrList<CObList, CaMessageNode*>* pLs = pI->GetListSubNode();
		if (pLs)
		{
			CTypedPtrList<CObList, CaMessageNode*>* pLd = pDest->GetListSubNode();
			if (!pLd)
			{
				pLd = new CTypedPtrList<CObList, CaMessageNode*>;
				pDest->SetListSubNode(pLd);
			}

			CaMessageNode* pObj = NULL;
			POSITION pos = pLs->GetHeadPosition();
			while (pos != NULL)
			{
				pObj = pLs->GetNext (pos);
				if (pObj->IsInternal())
				{
					CaMessageNodeTreeStateFolder* pNewFolder = new CaMessageNodeTreeStateFolder();
					pNewFolder->GetNodeInformation().SetNodeTitle (pObj->GetNodeInformation().GetNodeTitle());
					pNewFolder->GetNodeInformation().SetNodeType (NODE_EXIST);
					pLd->AddTail (pNewFolder);

					int nClose = IMGS_FOLDER_CLOSE, nOpen = IMGS_FOLDER_OPEN;
					MSGSourceTarget ddSourceTarget = DD_UNDEFINED;
					switch (nClass)
					{
					case IMSG_ALERT:
						nClose = IMGS_FOLDER_A_CLOSE;
						nOpen = IMGS_FOLDER_A_OPEN;
						ddSourceTarget = SV_UFA;
						break;
					case IMSG_NOTIFY:
						nClose = IMGS_FOLDER_N_CLOSE;
						nOpen = IMGS_FOLDER_N_OPEN;
						ddSourceTarget = SV_UFN;
						break;
					case IMSG_DISCARD:
						nClose = IMGS_FOLDER_D_CLOSE;
						nOpen = IMGS_FOLDER_D_OPEN;
						ddSourceTarget = SV_UFD;
						break;
					default:
						break;
					}
					CaMessageNodeInfo& nodeInfo = pNewFolder->GetNodeInformation();
					nodeInfo.SetIconImage (nClose, nOpen);
					nodeInfo.SetMsgSourceTarget(ddSourceTarget);

					pNewFolder->UpdateTree(pObj, nClass);
				}
				else
				{
					CaMessageNodeTree* pNt = (CaMessageNodeTree*)pObj;
					CaMessage* pMsg = pNt->GetMessageCategory();
					if (pMsg->GetClass() == nClass)
					{
						CaMessageNodeTree* pNewNode = new CaMessageNodeTree(pMsg);
						pNewNode->GetNodeInformation().SetNodeTitle(pMsg->GetTitle());
						pNewNode->GetNodeInformation().SetMsgSourceTarget(SV_U);
						pLd->AddTail (pNewNode);
					}
				}
			}
		}
	}
}

void CaMessageNodeTreeStateFolder::UpdateTree (CaMessageNode* pMsgNode, Imsgclass nClass)
{
	if (pMsgNode->IsInternal())
	{
		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pMsgNode;
		if (!m_pListSubNode)
			m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;

		StateViewCopyFolder (this, pMsgNode, nClass);
	}
}


//
// MAIN TREE of STATE VIEW:
CaTreeMessageState::CaTreeMessageState():CaMessageNodeTreeInternal()
{
	CString strTitle;
	if (!strTitle.LoadString(IDS_STATES))
		strTitle = _T("<States>");
	m_bShowCategory = TRUE;
	m_nodeInfo.SetNodeTitle (strTitle);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_CLOSE, IMGS_FOLDER_OPEN);
	m_nodeInfo.SetMsgSourceTarget(SV_R);
}

CaTreeMessageState::~CaTreeMessageState()
{
}

void CaTreeMessageState::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		//
		// Main root: <States>
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem  = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			TVI_ROOT, 
			TVI_FIRST);

		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType  (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}


	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}

	//
	// Display the classification (state) branches:
	m_nodeAlert.Display (pTree, m_nodeInfo.GetHTreeItem(), nMode);
	m_nodeNotify.Display (pTree, m_nodeInfo.GetHTreeItem(), nMode);
	m_nodeDiscard.Display (pTree, m_nodeInfo.GetHTreeItem(), nMode);

	pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
	m_nodeInfo.SetExpanded (TRUE);
}


//
// Construct the State View Tree data from the Category View Tree Data:
void CaTreeMessageState::UpdateTree (CTreeCtrl* pTree, CaTreeMessageCategory* pFrom)
{
	if (!pFrom)
		CleanData();

	// Tree Item:
	// Clean all the folders <Alert>, <Notify>, <Discard>:
	if (pTree)
	{
		if (m_nodeAlert.GetNodeInformation().GetHTreeItem())
		{
			pTree->DeleteItem (m_nodeAlert.GetNodeInformation().GetHTreeItem());
			m_nodeAlert.GetNodeInformation().SetHTreeItem(NULL);
		}

		if (m_nodeNotify.GetNodeInformation().GetHTreeItem())
		{
			pTree->DeleteItem (m_nodeNotify.GetNodeInformation().GetHTreeItem());
			m_nodeNotify.GetNodeInformation().SetHTreeItem(NULL);
		}

		if (m_nodeDiscard.GetNodeInformation().GetHTreeItem())
		{
			pTree->DeleteItem (m_nodeDiscard.GetNodeInformation().GetHTreeItem());
			m_nodeDiscard.GetNodeInformation().SetHTreeItem(NULL);
		}
	}
	// Tree Data:
	// Clean all the folders <Alert>, <Notify>, <Discard>:
	m_nodeAlert.CleanData();
	m_nodeNotify.CleanData();
	m_nodeDiscard.CleanData();


	//
	// Populate Tree Data:
	m_nodeAlert.Populate(pTree, pFrom);
	m_nodeNotify.Populate(pTree, pFrom);
	m_nodeDiscard.Populate(pTree, pFrom);
}



int CALLBACK CaTreeMessageState::CompareItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CaMessageNode* pObj1 = (CaMessageNode*)lParam1;
	CaMessageNode* pObj2 = (CaMessageNode*)lParam2;
	if (!(pObj1 && pObj2))
		return 0;
	int n1 = pObj1->GetNodeInformation().GetOrder();
	int n2 = pObj2->GetNodeInformation().GetOrder();

	if (n1 > n2) 
		return 1;
	else
	if (n1 < n2)
		return -1;
	else
	{
		//
		// Alphabetic order:
		CString strT1 = pObj1->GetNodeInformation().GetNodeTitle();
		CString strT2 = pObj2->GetNodeInformation().GetNodeTitle();
		return strT1.CompareNoCase (strT2);
	}
}


void CaTreeMessageState::SortItems(CTreeCtrl* pTree)
{
	TV_SORTCB tvsort;
	tvsort.hParent = GetNodeInformation().GetHTreeItem();
	tvsort.lpfnCompare = CaTreeMessageCategory::CompareItem;
	tvsort.lParam = NULL;
	pTree->SortChildrenCB (&tvsort);


	m_nodeAlert.SortItems(pTree);
	m_nodeNotify.SortItems(pTree);
	m_nodeDiscard.SortItems(pTree);
}




//
// CATEGROY VIEW: An Internal Node (Alert) of Tree's Message Category:
CaMessageNodeTreeCategoryAlert::CaMessageNodeTreeCategoryAlert():CaMessageNode()
{
	CString strTitle;
	if (!strTitle.LoadString(IDS_ALERT))
		strTitle = _T("<Alert>");
	m_nodeInfo.SetNodeTitle (strTitle);
	m_nodeInfo.SetOrder (ORDER_ALERT);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_A_CLOSE, IMGS_FOLDER_A_OPEN);
}

CaMessageNodeTreeCategoryAlert::~CaMessageNodeTreeCategoryAlert()
{

}



void CaMessageNodeTreeCategoryAlert::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		//
		// Main Node: <Alert>
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem  = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree, 
			TVI_LAST);

		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType  (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}

	if (nMode & SHOW_STATE_BRANCH)
	{
		if (m_nodeInfo.IsExpanded())
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
		else
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
	}
	SortItems (pTree);
}


void CaMessageNodeTreeCategoryAlert::AlertMessage(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_nodeInfo.GetHTreeItem();
	if (!pTree->ItemHasChildren(hItem))
		return;

	HTREEITEM hItemChild = pTree->GetChildItem (hItem);
	while (hItemChild != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)pTree->GetItemData (hItemChild);
		pNode->AlertMessage(pTree);
	
		hItemChild = pTree->GetNextSiblingItem (hItemChild);
	}
}

void CaMessageNodeTreeCategoryAlert::NotifyMessage(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_nodeInfo.GetHTreeItem();
	if (!pTree->ItemHasChildren(hItem))
		return;

	HTREEITEM hItemChild = pTree->GetChildItem (hItem);
	while (hItemChild != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)pTree->GetItemData (hItemChild);
		pNode->NotifyMessage(pTree);
	
		hItemChild = pTree->GetNextSiblingItem (hItemChild);
	}
}

void CaMessageNodeTreeCategoryAlert::DiscardMessage(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_nodeInfo.GetHTreeItem();
	if (!pTree->ItemHasChildren(hItem))
		return;

	HTREEITEM hItemChild = pTree->GetChildItem (hItem);
	while (hItemChild != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)pTree->GetItemData (hItemChild);
		pNode->DiscardMessage(pTree);
	
		hItemChild = pTree->GetNextSiblingItem (hItemChild);
	}
}


//
// CATEGROY VIEW: An Internal Node (Notify) of Tree's Message Category:
CaMessageNodeTreeCategoryNotify::CaMessageNodeTreeCategoryNotify():CaMessageNode()
{
	CString strTitle;
	if (!strTitle.LoadString(IDS_NOTIFY))
		strTitle = _T("<Notify>");
	m_nodeInfo.SetNodeTitle (strTitle);
	m_nodeInfo.SetOrder (ORDER_NOTIFY);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_N_CLOSE, IMGS_FOLDER_N_OPEN);
}

CaMessageNodeTreeCategoryNotify::~CaMessageNodeTreeCategoryNotify()
{

}


void CaMessageNodeTreeCategoryNotify::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		//
		// Main Node: <Notify>
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem  = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree, 
			TVI_LAST);

		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType  (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}

	if (nMode & SHOW_STATE_BRANCH)
	{
		if (m_nodeInfo.IsExpanded())
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
		else
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
	}
	SortItems (pTree);
}


void CaMessageNodeTreeCategoryNotify::AlertMessage(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_nodeInfo.GetHTreeItem();
	if (!pTree->ItemHasChildren(hItem))
		return;

	HTREEITEM hItemChild = pTree->GetChildItem (hItem);
	while (hItemChild != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)pTree->GetItemData (hItemChild);
		pNode->AlertMessage(pTree);
	
		hItemChild = pTree->GetNextSiblingItem (hItemChild);
	}
}

void CaMessageNodeTreeCategoryNotify::NotifyMessage(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_nodeInfo.GetHTreeItem();
	if (!pTree->ItemHasChildren(hItem))
		return;

	HTREEITEM hItemChild = pTree->GetChildItem (hItem);
	while (hItemChild != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)pTree->GetItemData (hItemChild);
		pNode->NotifyMessage(pTree);
	
		hItemChild = pTree->GetNextSiblingItem (hItemChild);
	}
}

void CaMessageNodeTreeCategoryNotify::DiscardMessage(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_nodeInfo.GetHTreeItem();
	if (!pTree->ItemHasChildren(hItem))
		return;

	HTREEITEM hItemChild = pTree->GetChildItem (hItem);
	while (hItemChild != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)pTree->GetItemData (hItemChild);
		pNode->DiscardMessage(pTree);
	
		hItemChild = pTree->GetNextSiblingItem (hItemChild);
	}
}


//
// CATEGORY VIEW: An Internal Node (Discard) of Tree's Message Category:
CaMessageNodeTreeCategoryDiscard::CaMessageNodeTreeCategoryDiscard():CaMessageNode()
{
	CString strTitle;
	if (!strTitle.LoadString(IDS_DISCARD))
		strTitle = _T("<Discard>");
	m_nodeInfo.SetNodeTitle (strTitle);
	m_nodeInfo.SetOrder (ORDER_DISCARD);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_D_CLOSE, IMGS_FOLDER_D_OPEN);
}

CaMessageNodeTreeCategoryDiscard::~CaMessageNodeTreeCategoryDiscard()
{

}



void CaMessageNodeTreeCategoryDiscard::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		//
		// Main Node: <Discard>
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem  = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree, 
			TVI_LAST);

		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType  (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}

	if (nMode & SHOW_STATE_BRANCH)
	{
		if (m_nodeInfo.IsExpanded())
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
		else
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
	}
	SortItems (pTree);
}

void CaMessageNodeTreeCategoryDiscard::AlertMessage(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_nodeInfo.GetHTreeItem();
	if (!pTree->ItemHasChildren(hItem))
		return;

	HTREEITEM hItemChild = pTree->GetChildItem (hItem);
	while (hItemChild != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)pTree->GetItemData (hItemChild);
		pNode->AlertMessage(pTree);
	
		hItemChild = pTree->GetNextSiblingItem (hItemChild);
	}
}

void CaMessageNodeTreeCategoryDiscard::NotifyMessage(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_nodeInfo.GetHTreeItem();
	if (!pTree->ItemHasChildren(hItem))
		return;

	HTREEITEM hItemChild = pTree->GetChildItem (hItem);
	while (hItemChild != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)pTree->GetItemData (hItemChild);
		pNode->NotifyMessage(pTree);
	
		hItemChild = pTree->GetNextSiblingItem (hItemChild);
	}
}

void CaMessageNodeTreeCategoryDiscard::DiscardMessage(CTreeCtrl* pTree)
{
	HTREEITEM hItem = m_nodeInfo.GetHTreeItem();
	if (!pTree->ItemHasChildren(hItem))
		return;

	HTREEITEM hItemChild = pTree->GetChildItem (hItem);
	while (hItemChild != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)pTree->GetItemData (hItemChild);
		pNode->DiscardMessage(pTree);
	
		hItemChild = pTree->GetNextSiblingItem (hItemChild);
	}
}


//
// CATEGORY VIEW: An Internal Node (Others) of Tree's Message Category:
CaMessageNodeTreeCategoryOthers::CaMessageNodeTreeCategoryOthers():CaMessageNodeTreeCategoryFolder()
{
	CString strTitle;
	if (!strTitle.LoadString(IDS_UNCLASSIFIED_MESSAGE))
		strTitle = _T("<Unclassified Messages>");
	m_nodeInfo.SetNodeTitle (strTitle);
	m_nodeInfo.SetOrder (ORDER_OTHERS);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_CLOSE, IMGS_FOLDER_OPEN);
	m_nodeInfo.SetMsgSourceTarget(CV_OF);

	m_FolderAlert.GetNodeInformation().SetMsgSourceTarget(CV_OFA);
	m_FolderNotify.GetNodeInformation().SetMsgSourceTarget(CV_OFN);
	m_FolderDiscard.GetNodeInformation().SetMsgSourceTarget(CV_OFD);
}

CaMessageNodeTreeCategoryOthers::~CaMessageNodeTreeCategoryOthers()
{

}



void CaMessageNodeTreeCategoryOthers::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	CaMessageNodeTreeCategoryFolder::Display (pTree, hTree, nMode);

	if (nMode & SHOW_STATE_BRANCH)
	{
		if (m_nodeInfo.IsExpanded())
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
		else
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
	}
}


void CaMessageNodeTreeCategoryOthers::Populate (CTypedPtrList<CObList, CaMessage*>& listMessageCategory)
{
	CaMessageNodeTree* pNode = NULL;
	CaMessage* pMessage = NULL;
	CTypedPtrList<CObList, CaMessageNode*>* pListSubNode = NULL;
	POSITION pos = listMessageCategory.GetHeadPosition();

	if (pos != NULL)
	{
		if (!m_pListSubNode)
			m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
	}

	while (pos != NULL)
	{
		pMessage = listMessageCategory.GetNext (pos);
		pNode = new CaMessageNodeTree(pMessage);
		pNode->GetNodeInformation().SetNodeTitle(pMessage->GetTitle());
		pNode->GetNodeInformation().SetMsgSourceTarget(CV_O);
		m_pListSubNode->AddTail(pNode);
	}
}


void CaMessageNodeTreeCategoryOthers::AlertMessage(CTreeCtrl* pTree)
{
	if (!m_pListSubNode)
		return;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
		pNode->AlertMessage(pTree);
	}
}

void CaMessageNodeTreeCategoryOthers::NotifyMessage(CTreeCtrl* pTree)
{
	if (!m_pListSubNode)
		return;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
		pNode->NotifyMessage(pTree);
	}
}

void CaMessageNodeTreeCategoryOthers::DiscardMessage(CTreeCtrl* pTree)
{
	if (!m_pListSubNode)
		return;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
		pNode->DiscardMessage(pTree);
	}
}



void CaMessageNodeTreeCategoryOthers::CleanData()
{

}


void CaMessageNodeTreeCategoryOthers::SortItems(CTreeCtrl* pTree)
{
	TV_SORTCB tvsort;
	tvsort.hParent = GetNodeInformation().GetHTreeItem();
	tvsort.lpfnCompare = CaTreeMessageCategory::CompareItem;
	tvsort.lParam = NULL;
	pTree->SortChildrenCB (&tvsort);
}


//
// CATEGORY VIEW: An Internal Node (Folder & Sub Folder) of Tree's Message Category:
CaMessageNodeTreeCategoryFolder::CaMessageNodeTreeCategoryFolder():CaMessageNodeTreeInternal()
{
	m_nodeInfo.SetOrder (ORDER_USER);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_CLOSE, IMGS_FOLDER_OPEN);
	m_nodeInfo.SetMsgSourceTarget (CV_UF);
	m_FolderAlert.GetNodeInformation().SetMsgSourceTarget(CV_UFA);
	m_FolderNotify.GetNodeInformation().SetMsgSourceTarget(CV_UFN);
	m_FolderDiscard.GetNodeInformation().SetMsgSourceTarget(CV_UFD);

	if (!m_pListSubNode)
		m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
}


CaMessageNodeTreeCategoryFolder::~CaMessageNodeTreeCategoryFolder()
{

}


void CaMessageNodeTreeCategoryFolder::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	CaMessageNode* pNode = NULL;
	POSITION pos = NULL;
	if (m_nodeInfo.GetHTreeItem() == NULL)
	{
		CString strTitle = m_nodeInfo.GetNodeTitle();
		HTREEITEM hTreeItem = pTree->InsertItem (
			strTitle, 
			0, 
			0, 
			hTree,
			TVI_LAST);
		if (hTreeItem)
		{
			pTree->SetItemData (hTreeItem, (DWORD_PTR)this);
			m_nodeInfo.SetNodeType (NODE_EXIST);
			m_nodeInfo.SetHTreeItem (hTreeItem);
			int nImageClose, nImageOpen;
			m_nodeInfo.GetIconImage (nImageClose, nImageOpen);
			if (m_nodeInfo.IsExpanded())
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageOpen, nImageOpen);
			else
				pTree->SetItemImage (m_nodeInfo.GetHTreeItem(), nImageClose, nImageClose);
		}
	}

	if (m_nodeInfo.GetNodeType() == NODE_DELETE)
	{
		pTree->DeleteItem (m_nodeInfo.GetHTreeItem());
		return;
	}
	if (!m_pListSubNode)
		return;

	//
	// Display the tree:
	BOOL bDisplayedStateFolder = FALSE;
	HTREEITEM hPA = m_nodeInfo.GetHTreeItem();
	HTREEITEM hPN = m_nodeInfo.GetHTreeItem();
	HTREEITEM hPD = m_nodeInfo.GetHTreeItem();
	pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_pListSubNode->GetNext(pos);
		if (!pNode->IsInternal())
		{
			if (!bDisplayedStateFolder && (nMode & SHOW_STATE_BRANCH))
			{
				m_FolderAlert.Display  (pTree, m_nodeInfo.GetHTreeItem(), nMode);
				m_FolderNotify.Display (pTree, m_nodeInfo.GetHTreeItem(), nMode);
				m_FolderDiscard.Display(pTree, m_nodeInfo.GetHTreeItem(), nMode);
				bDisplayedStateFolder = TRUE;
				hPA = m_FolderAlert.GetNodeInformation().GetHTreeItem();
				hPN = m_FolderNotify.GetNodeInformation().GetHTreeItem();
				hPD = m_FolderDiscard.GetNodeInformation().GetHTreeItem();
			}

			CaMessageNodeTree* pNt = (CaMessageNodeTree*)pNode;
			CaMessage* pMessage = pNt->GetMessageCategory();
			ASSERT (pMessage);
			if (!pMessage)
				continue;
		
			switch (pMessage->GetClass())
			{
			case IMSG_ALERT:
				pNode->Display (pTree, hPA, nMode);
				break;
			case IMSG_NOTIFY:
				pNode->Display (pTree, hPN, nMode);
				break;
			case IMSG_DISCARD:
				pNode->Display (pTree, hPD, nMode);
				break;
			default:
				pNode->Display (pTree, hPN, nMode);
				break;
			}
		}
		else
		{
			pNode->Display (pTree, m_nodeInfo.GetHTreeItem(), nMode);
		}
	}

	if (bDisplayedStateFolder)
	{
		if (m_FolderAlert.GetNodeInformation().IsExpanded())
			pTree->Expand (hPA, TVE_EXPAND);
		else
			pTree->Expand (hPA, TVE_COLLAPSE);

		if (m_FolderNotify.GetNodeInformation().IsExpanded())
			pTree->Expand (hPN, TVE_EXPAND);
		else
			pTree->Expand (hPN, TVE_COLLAPSE);

		if (m_FolderDiscard.GetNodeInformation().IsExpanded())
			pTree->Expand (hPD, TVE_EXPAND);
		else
			pTree->Expand (hPD, TVE_COLLAPSE);
	}

	SortItems(pTree);
	if (m_nodeInfo.IsExpanded())
		pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
	else
		pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_COLLAPSE);
}


void CaMessageNodeTreeCategoryFolder::AddFolder(CTreeCtrl* pTree)
{
	//
	// Each Folder will be added before the node <Unclassified Messages>

	try
	{
		CString strItem;
		CString strNewFolder;
		if (!strNewFolder.LoadString (IDS_NEW_FOLDER))
			strNewFolder = _T("New Folder");

		//
		// Check to see if the folder exists:
		strItem = strNewFolder;
		int nCount = 0;
		if (m_pListSubNode)
		{
			CaMessageNode* pNode = NULL;
			POSITION pos = m_pListSubNode->GetHeadPosition();
			while (pos != NULL)
			{
				pNode = m_pListSubNode->GetNext (pos);
				if (strItem.CompareNoCase (pNode->GetNodeInformation().GetNodeTitle()) == 0)
				{
					nCount++;
					strItem.Format (_T("%s %d"), (LPCTSTR)strNewFolder, nCount);
					pos = m_pListSubNode->GetHeadPosition();
				}
			}
			strNewFolder = strItem;
		}

		CaMessageNodeTreeCategoryFolder* pNewFolder = new CaMessageNodeTreeCategoryFolder();
		if (!m_pListSubNode)
			m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
		pNewFolder->GetNodeInformation().SetNodeTitle(strNewFolder);
		m_pListSubNode->AddTail (pNewFolder);
		pNewFolder->Display (pTree, m_nodeInfo.GetHTreeItem());
		SortItems(pTree);
		if (!m_nodeInfo.IsExpanded())
		{
			pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
			m_nodeInfo.SetExpanded (TRUE);
		}
		
		pTree->SelectItem((pNewFolder->GetNodeInformation()).GetHTreeItem());
		pTree->EditLabel ((pNewFolder->GetNodeInformation()).GetHTreeItem());
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CaMessageNodeTreeCategoryFolder::AddFolder): \nAdd New Folder failed"));
	}
}


CaMessageNodeTreeCategoryFolder* CaMessageNodeTreeCategoryFolder::FindFolder (LPCTSTR lpszFolder)
{
	if (!m_pListSubNode)
		return NULL;
	CString strItem;
	CaMessageNode* pObj = NULL;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		pObj = m_pListSubNode->GetNext (pos);
		if (!pObj->IsInternal())
			continue;
		strItem = pObj->GetNodeInformation().GetNodeTitle();
		if (strItem.CompareNoCase (lpszFolder) == 0)
			return (CaMessageNodeTreeCategoryFolder*)pObj;
	}
	return NULL;
}


void CaMessageNodeTreeCategoryFolder::AlertMessage(CTreeCtrl* pTree)
{
	if (!m_pListSubNode)
		return;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
		pNode->AlertMessage(pTree);
	}
}

void CaMessageNodeTreeCategoryFolder::NotifyMessage(CTreeCtrl* pTree)
{
	if (!m_pListSubNode)
		return;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
		pNode->NotifyMessage(pTree);
	}
}

void CaMessageNodeTreeCategoryFolder::DiscardMessage(CTreeCtrl* pTree)
{
	if (!m_pListSubNode)
		return;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
		pNode->DiscardMessage(pTree);
	}
}

void CaMessageNodeTreeCategoryFolder::ResetTreeCtrlData()
{
	CaMessageNode* pNode = NULL;
	POSITION pos = NULL;
	if (!m_pListSubNode)
		return;

	pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_pListSubNode->GetNext(pos);
		pNode->GetNodeInformation().SetHTreeItem(NULL);

		if (pNode->IsInternal())
		{
			pNode->ResetTreeCtrlData();
		}
	}

	m_FolderAlert.GetNodeInformation().SetHTreeItem(NULL);
	m_FolderNotify.GetNodeInformation().SetHTreeItem(NULL);
	m_FolderDiscard.GetNodeInformation().SetHTreeItem(NULL);
}


void CaMessageNodeTreeCategoryFolder::SortItems(CTreeCtrl* pTree)
{
	TV_SORTCB tvsort;
	tvsort.hParent = GetNodeInformation().GetHTreeItem();
	tvsort.lpfnCompare = CaTreeMessageCategory::CompareItem;
	tvsort.lParam = NULL;
	pTree->SortChildrenCB (&tvsort);

	if (m_pListSubNode)
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			if (pNode->IsInternal())
			{
				pNode->SortItems(pTree);
			}
		}
	}
}


//
// MAIN TREE of CATEGORY VIEW:
CaTreeMessageCategory::CaTreeMessageCategory():CaMessageNodeTreeCategoryFolder()
{
	CString strTitle;
	if (!strTitle.LoadString(IDS_CATEGORIES))
		strTitle = _T("<Categories>");
	m_bShowStateBranches = TRUE;
	m_nodeInfo.SetNodeTitle (strTitle);
	m_nodeInfo.SetIconImage (IMGS_FOLDER_CLOSE, IMGS_FOLDER_OPEN);
	m_nodeInfo.SetMsgSourceTarget(CV_R);
}

CaTreeMessageCategory::~CaTreeMessageCategory()
{
}



void CaTreeMessageCategory::Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode)
{
	UndisplayFolder(pTree);

	CaMessageNodeTreeCategoryFolder::Display (pTree, TVI_ROOT, nMode);

	m_categoryOthers.Display (pTree, m_nodeInfo.GetHTreeItem(), nMode);
	SortItems (pTree);
	pTree->Expand (m_nodeInfo.GetHTreeItem(), TVE_EXPAND);
}


void CaTreeMessageCategory::Populate (CTypedPtrList<CObList, CaMessage*>& listMessageCategory)
{
	m_categoryOthers.Populate (listMessageCategory);
}


//
// Populate the unclassified (others) message:
void CaTreeMessageCategory::Populate (CaMessageManager* pData)
{
	if (!pData)
		return;
	CTypedPtrArray <CObArray, CaMessageEntry*>& lm = pData->GetListEntry();
	CTypedPtrList<CObList, CaMessage*> ls;

	int i;
	INT_PTR nSize = lm.GetSize();
	for (i=0; i<nSize; i++)
		ls.AddTail ((CaMessage*)lm[i]);

	m_categoryOthers.Populate (ls);
}



void CaTreeMessageCategory::Populate (CaMessageManager* pMsgEntry, CaCategoryDataUserManager& data)
{
	CaCategoryDataUser* pObj = NULL;
	CTypedPtrList<CObList, CaCategoryDataUser*>& l = data.GetListUserFolder();
	POSITION pos = l.GetHeadPosition();

	while (pos != NULL)
	{
		if (!m_pListSubNode)
		{
			m_pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
		}

		pObj = l.GetNext (pos);
		if (pObj->IsFolder())
		{
			AddBranch (pObj->GetPath());
		}
		else
		{
			AddLeaf (pMsgEntry, pObj->GetPath(), pObj->GetCategory(), pObj->GetCode());
		}
	}
}


static void CreateBranch (CaMessageNodeTreeCategoryFolder* pNode, CString strPath)
{
	int nIdx = -1;
	CString strItem;
	while (!strPath.IsEmpty())
	{
		nIdx = strPath.Find (_T('/'));
		if (nIdx != -1)
		{
			strItem = strPath.Left(nIdx);
			strPath = strPath.Mid (nIdx+1);
		}
		else
		{
			strItem = strPath;
			strPath = _T("");
		}

		CaMessageNodeTreeCategoryFolder* pFolder = pNode->FindFolder (strItem);
		if (pFolder && !strPath.IsEmpty())
		{
			pNode = pFolder;
			continue;
		}
		else
		if (!pFolder)
		{
			CaMessageNodeTreeCategoryFolder* pNewFolder = new CaMessageNodeTreeCategoryFolder();
			pNewFolder->GetNodeInformation().SetNodeTitle (strItem);
			CTypedPtrList<CObList, CaMessageNode*>* pListSubNode = pNode->GetListSubNode();
			if (!pListSubNode)
			{
				pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
				pNode->SetListSubNode(pListSubNode);
			}
			pListSubNode->AddTail (pNewFolder);
			if (!strPath.IsEmpty())
				CreateBranch (pNewFolder, strPath);
		}
		else
		{
			//
			// Folder already exists
		}
	}
}

void CaTreeMessageCategory::AddBranch (CString strPath)
{
	CreateBranch (this, strPath);
}

static void CreateLeaf (
	CaMessageManager* pMsgEntry, 
	CaMessageNodeTreeCategoryFolder* pNode, 
	CString strPath, 
	long lCategory, 
	long lCode)
{
	int nIdx = -1;
	CString strItem;
	while (!strPath.IsEmpty())
	{
		nIdx = strPath.Find (_T('/'));
		if (nIdx != -1)
		{
			strItem = strPath.Left(nIdx);
			strPath = strPath.Mid (nIdx+1);
		}
		else
		{
			strItem = strPath;
			strPath = _T("");
		}

		if (nIdx != -1)
		{
			//
			// Look up the appropriate folder:
			CaMessageNodeTreeCategoryFolder* pFolder = pNode->FindFolder (strItem);
			if (pFolder && !strPath.IsEmpty())
			{
				pNode = pFolder;
				continue;
			}
		}
		else
		{
			CaMessageNodeTree* pNewTerminal = new CaMessageNodeTree();
			pNewTerminal->GetNodeInformation().SetNodeTitle (strItem);
			pNewTerminal->GetNodeInformation().SetMsgSourceTarget(CV_U);
			//
			// Use (lCategory, lCode) to find the meesage and call pNewTerminal->SetMessage()
			CaMessage* pMessage = pMsgEntry->Search(lCategory, lCode);
			if (pMessage)
			{
				pNewTerminal->SetMessageCategory (pMessage);
				CTypedPtrList<CObList, CaMessageNode*>* pListSubNode = pNode->GetListSubNode();
				if (!pListSubNode)
				{
					pListSubNode = new CTypedPtrList<CObList, CaMessageNode*>;
					pNode->SetListSubNode(pListSubNode);
				}
				pListSubNode->AddTail (pNewTerminal);
			}
			else
			{
				delete pNewTerminal;
				pNewTerminal = NULL;
			}
		}
	}
}

void CaTreeMessageCategory::AddLeaf (CaMessageManager* pMsgEntry, CString strPath, long lCategory, long lCode)
{
	CreateLeaf(pMsgEntry, this, strPath, lCategory, lCode);
}

void CaTreeMessageCategory::AlertMessage(CTreeCtrl* pTree)
{
	CaMessageNodeTreeCategoryFolder::AlertMessage(pTree);
	m_categoryOthers.AlertMessage(pTree);
}

void CaTreeMessageCategory::NotifyMessage(CTreeCtrl* pTree)
{
	CaMessageNodeTreeCategoryFolder::NotifyMessage(pTree);
	m_categoryOthers.NotifyMessage(pTree);
}

void CaTreeMessageCategory::DiscardMessage(CTreeCtrl* pTree)
{
	CaMessageNodeTreeCategoryFolder::DiscardMessage(pTree);
	m_categoryOthers.DiscardMessage(pTree);
}


static void CountMsg (CaMessageNode* pNode, long lCat, long lCode, int& nCount)
{
	if (pNode->IsInternal())
	{
		CTypedPtrList<CObList, CaMessageNode*>* pL = NULL;
		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pNode;
		pL = pI->GetListSubNode();
		if (pL)
		{
			CaMessageNode* pObj = NULL;
			POSITION pos = pL->GetHeadPosition();
			while (pos != NULL)
			{
				pObj = pL->GetNext (pos);
				CountMsg (pObj, lCat, lCode, nCount);
			}
		}
	}
	else
	{
		CaMessageNodeTree* pMsgNode = (CaMessageNodeTree*)pNode;
		CaMessage* pMsg = pMsgNode->GetMessageCategory();
		if (pMsg->GetCategory() == lCat && pMsg->GetCode() == lCode)
		{
			nCount++;
		}
	}
}

int CaTreeMessageCategory::CountMessage(CaMessage* pMessage)
{
	int nCount = 0;
	if (!m_pListSubNode)
		return nCount;
	CaMessageNode* pNode = NULL;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_pListSubNode->GetNext (pos);
		CountMsg (pNode, pMessage->GetCategory(), pMessage->GetCode(), nCount);
	}
	return nCount;
}

static void FindFirstMsg (CaMessageNode* pNode, long lCat, long lCode, CaMessageNode*& pFound)
{
	if (pNode->IsInternal())
	{
		CTypedPtrList<CObList, CaMessageNode*>* pL = NULL;
		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pNode;
		pL = pI->GetListSubNode();
		if (pL)
		{
			CaMessageNode* pObj = NULL;
			POSITION pos = pL->GetHeadPosition();
			while (pos != NULL)
			{
				pObj = pL->GetNext (pos);
				FindFirstMsg (pObj, lCat, lCode, pFound);
				if (pFound)
					return;
			}
		}
	}
	else
	{
		CaMessageNodeTree* pMsgNode = (CaMessageNodeTree*)pNode;
		CaMessage* pMsg = pMsgNode->GetMessageCategory();
		if (pMsg->GetCategory() == lCat && pMsg->GetCode() == lCode)
		{
			pFound = pNode;
			return;
		}
	}
}


CaMessageNode* CaTreeMessageCategory::FindFirstMessage (CaMessage* pMessage)
{
	CaMessageNode* pFound = NULL;
	if (!m_pListSubNode)
		return NULL;
	CaMessageNode* pNode = NULL;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_pListSubNode->GetNext (pos);
		FindFirstMsg (pNode, pMessage->GetCategory(), pMessage->GetCode(), pFound);
		if (pFound)
			return pFound;
	}
	return NULL;
}


void CaTreeMessageCategory::UndisplayFolder(CTreeCtrl* pTree)
{
	if (m_categoryOthers.GetNodeInformation().GetHTreeItem())
	{
		pTree->DeleteItem (m_categoryOthers.GetNodeInformation().GetHTreeItem());
		m_categoryOthers.GetNodeInformation().SetHTreeItem(NULL);
	}
	m_categoryOthers.ResetTreeCtrlData();

	if (!m_pListSubNode)
		return;
	HTREEITEM hTree = NULL;
	CaMessageNode* pNode = NULL;
	POSITION pos = m_pListSubNode->GetHeadPosition();
	while (pos != NULL)
	{
		pNode = m_pListSubNode->GetNext (pos);
		hTree = pNode->GetNodeInformation().GetHTreeItem();
		if (hTree)
		{
			pTree->DeleteItem (hTree);
			pNode->GetNodeInformation().SetHTreeItem(NULL);
		}

		if (pNode->IsInternal())
		{
			pNode->ResetTreeCtrlData();
		}
	}
	
	if (m_FolderAlert.GetNodeInformation().GetHTreeItem() != NULL)
	{
		pTree->DeleteItem (m_FolderAlert.GetNodeInformation().GetHTreeItem());
		m_FolderAlert.GetNodeInformation().SetHTreeItem(NULL);
	}
	if (m_FolderNotify.GetNodeInformation().GetHTreeItem() != NULL)
	{
		pTree->DeleteItem (m_FolderNotify.GetNodeInformation().GetHTreeItem());
		m_FolderNotify.GetNodeInformation().SetHTreeItem(NULL);
	}
	if (m_FolderDiscard.GetNodeInformation().GetHTreeItem() != NULL)
	{
		pTree->DeleteItem (m_FolderDiscard.GetNodeInformation().GetHTreeItem());
		m_FolderDiscard.GetNodeInformation().SetHTreeItem(NULL);
	}
}


int CALLBACK CaTreeMessageCategory::CompareItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CaMessageNode* pObj1 = (CaMessageNode*)lParam1;
	CaMessageNode* pObj2 = (CaMessageNode*)lParam2;
	if (!(pObj1 && pObj2))
		return 0;
	int n1 = pObj1->GetNodeInformation().GetOrder();
	int n2 = pObj2->GetNodeInformation().GetOrder();

	if (n1 > n2) 
		return 1;
	else
	if (n1 < n2)
		return -1;
	else
	{
		//
		// Alphabetic order:
		CString strT1 = pObj1->GetNodeInformation().GetNodeTitle();
		CString strT2 = pObj2->GetNodeInformation().GetNodeTitle();
		return strT1.CompareNoCase (strT2);
	}
}


void CaTreeMessageCategory::SortItems(CTreeCtrl* pTree)
{
	TV_SORTCB tvsort;
	tvsort.hParent = GetNodeInformation().GetHTreeItem();
	tvsort.lpfnCompare = CaTreeMessageCategory::CompareItem;
	tvsort.lParam = NULL;
	pTree->SortChildrenCB (&tvsort);

	if (m_pListSubNode)
	{
		POSITION pos = m_pListSubNode->GetHeadPosition();
		while (pos != NULL)
		{
			CaMessageNode* pNode = m_pListSubNode->GetNext(pos);
			if (pNode->IsInternal())
			{
				pNode->SortItems(pTree);
			}
		}
	}
	m_categoryOthers.SortItems(pTree);
}



#if defined (_DEBUG)
static void CaTreeMessageCategory_Output (CaMessageNode* pNode, int nLevel)
{
	CString strTab = _T("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
	CaMessageNode* pObj = NULL;
	CString strItem;
	CString strT;

	if (pNode->IsInternal())
	{
		if (nLevel == 0)
		    strT = (LPCTSTR)_T("");
		else
		    strT = strTab.Left(nLevel);

		strItem.Format (_T("%s%s \n"), 
			strT, 
			pNode->GetNodeInformation().GetNodeTitle());
		TRACE2 ("%s%s \n", (LPCTSTR)strT,(LPCTSTR) pNode->GetNodeInformation().GetNodeTitle());

		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pNode;
		CTypedPtrList<CObList, CaMessageNode*>* pL = pI->GetListSubNode();
		if (pL)
		{
			POSITION pos = pL->GetHeadPosition();
			while (pos != NULL)
			{
				pObj = pL->GetNext (pos);
				CaTreeMessageCategory_Output(pObj, nLevel + 1);
			}
		}
	}
	else
	{
		if (nLevel == 0)
		    strT = (LPCTSTR)_T("");
		else
		    strT = strTab.Left(nLevel);

		strItem.Format (_T("%s%s \n"), 
			strT, 
			pNode->GetNodeInformation().GetNodeTitle());
		TRACE2 ("%s%s \n", (LPCTSTR)strT,(LPCTSTR) pNode->GetNodeInformation().GetNodeTitle());
	}
}
#endif


void CaTreeMessageCategory::Output(CaMessageNode* pData, int nLevel)
{
#if defined (_DEBUG)
	TRACE0 ("CaTreeMessageCategory::Output ...\n");
	CString strTab = _T("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
	CString strItem;
	CString strT;

	CaMessageNode* pObj = NULL;
	CTypedPtrList<CObList, CaMessageNode*>* pL = NULL;
	if (pData->IsInternal())
	{
		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pData;
		pL = pI->GetListSubNode();
	}
	if (pL)
	{
		POSITION pos = pL->GetHeadPosition();
		while (pos != NULL)
		{
			pObj = pL->GetNext (pos);
			if (nLevel == 0)
			    strT = (LPCTSTR)_T("");
			else
			    strT = strTab.Left(nLevel);

			strItem.Format (_T("%s%s \n"), 
				strT, 
				pObj->GetNodeInformation().GetNodeTitle());
			TRACE2 ("%s%s \n", (LPCTSTR)strT,(LPCTSTR) pObj->GetNodeInformation().GetNodeTitle());
			if (pObj->IsInternal())
				Output(pObj, nLevel + 1);
		}
	}
#endif
}


static void ConstructListUserFolder(
	CTypedPtrList<CObList, CaCategoryDataUser*>& lUsr, 
	CaMessageNode* pData, int nLevel, LPCTSTR lpszPath)
{
	CString strT;

	CaMessageNode* pObj = NULL;
	CTypedPtrList<CObList, CaMessageNode*>* pL = NULL;
	if (pData->IsInternal())
	{
		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pData;
		pL = pI->GetListSubNode();
	}

	if (pL)
	{
		POSITION pos = pL->GetHeadPosition();
		BOOL bFolder = FALSE;
		int nCat, nCode;
		while (pos != NULL)
		{
			pObj = pL->GetNext (pos);
			strT = lpszPath;
			if (!strT.IsEmpty())
				strT+= _T("/");
			strT+=pObj->GetNodeInformation().GetNodeTitle();

			if (pObj->IsInternal())
			{
				bFolder = TRUE;
				nCode = 0;
				nCat  = 0;
			}
			else
			{
				CaMessageNodeTree* pMsgNode = (CaMessageNodeTree*)pObj;
				CaMessage* pMsg = pMsgNode->GetMessageCategory();
				bFolder = FALSE;
				nCode = pMsg->GetCode();
				nCat = pMsg->GetCategory();
			}

			CaCategoryDataUser* pUserFolder = new CaCategoryDataUser(nCat, nCode, bFolder, strT);
			lUsr.AddTail(pUserFolder);
			if (pObj->IsInternal())
			{
				CString strPath = lpszPath;
				if (!strPath.IsEmpty())
					strPath+= _T("/");
				strPath += pObj->GetNodeInformation().GetNodeTitle();
				ConstructListUserFolder(lUsr, pObj, nLevel + 1, strPath);
			}
		}
	}
}


void CaTreeMessageCategory::GenerateUserFolder (CaCategoryDataUserManager& usrFolder)
{
	CTypedPtrList<CObList, CaCategoryDataUser*>& lUsr = usrFolder.GetListUserFolder();
	ConstructListUserFolder (lUsr, this, 0, _T(""));
}


void CaTreeMessageCategory::Output2(CaMessageNode* pData, int nLevel, LPCTSTR lpszPath)
{
	CString strTab = _T("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
	CString strItem;
	CString strT;

	CaMessageNode* pObj = NULL;
	CTypedPtrList<CObList, CaMessageNode*>* pL = NULL;
	if (pData->IsInternal())
	{
		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pData;
		pL = pI->GetListSubNode();
	}

	if (pL)
	{
		POSITION pos = pL->GetHeadPosition();
		BOOL bFolder = FALSE;
		int nCat, nCode;
		while (pos != NULL)
		{
			pObj = pL->GetNext (pos);
			strT = lpszPath;
			if (!strT.IsEmpty())
				strT+= _T("/");
			strT+=pObj->GetNodeInformation().GetNodeTitle();

			if (pObj->IsInternal())
			{
				bFolder = TRUE;
				nCode = 0;
				nCat  = 0;
			}
			else
			{
				CaMessageNodeTree* pMsgNode = (CaMessageNodeTree*)pObj;
				CaMessage* pMsg = pMsgNode->GetMessageCategory();
				bFolder = FALSE;
				nCode = pMsg->GetCode();
				nCat = pMsg->GetCategory();
			}

			strItem.Format (_T("%d %d %d %s\n"), nCat, nCode, bFolder, (LPCTSTR)strT);
			TRACE1 ("%s", (LPCTSTR)strItem);
			if (pObj->IsInternal())
			{
				CString strPath = lpszPath;
				if (!strPath.IsEmpty())
					strPath+= _T("/");
				strPath += pObj->GetNodeInformation().GetNodeTitle();
				Output2(pObj, nLevel + 1, strPath);
			}
		}
	}
}

