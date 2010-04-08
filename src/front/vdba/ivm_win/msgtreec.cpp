/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : msgtreec.cpp, Implementation file
**    Project  : Visual Manager
**    Author   : UK Sotheavut
**    Purpose  : Define Message Categories and Notification Levels
**               Defenition of Tree Node of Message Category (Visual Design)
**
** History:
**
** 21-May-1999 (uk$so01)
**    Created
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
**/

#include "stdafx.h"
#include "rcdepend.h"
#include "msgtreec.h"
#include "msgntree.h"
#include "fevsetti.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static UINT SourceCV_OxTarget (CTreeCtrl* pTreeSource, CaMessageNodeInfo& source, MSGSourceTarget t)
{
	UINT nAction = 0;
	CaMessageNode* pNode = (CaMessageNode*)pTreeSource->GetItemData (source.GetHTreeItem());
	ASSERT (pNode->IsInternal() == FALSE);
	if (pNode->IsInternal())
		return nAction;
	CaMessageNodeTree* pNodeTree  = (CaMessageNodeTree*)pNode;
	CaMessage* pMsg = pNodeTree->GetMessageCategory();
	if (!pMsg)
		return nAction;

	switch (t)
	{
	case CV_OFA:
	case SV_OFA:
	case SV_FA:
		if (pMsg->GetClass() != IMSG_ALERT)
			nAction |= DD_ACTION_ALERT;
		break;
	case CV_OFN:
	case SV_OFN:
	case SV_FN:
		if (pMsg->GetClass() != IMSG_NOTIFY)
			nAction |= DD_ACTION_NOTIFY;
		break;
	case CV_OFD:
	case SV_OFD:
	case SV_FD:
		if (pMsg->GetClass() != IMSG_DISCARD)
			nAction |= DD_ACTION_DISCARD;
		break;
	default:
		break;
	}
	return nAction;
}

static UINT SourceCV_OFxTarget (MSGSourceTarget t)
{
	UINT nAction = 0;
	switch (t)
	{
	case CV_OFA:
	case SV_OFA:
	case SV_FA:
		nAction |= DD_ACTION_ALERT;
		break;
	case CV_OFN:
	case SV_OFN:
	case SV_FN:
		nAction |= DD_ACTION_NOTIFY;
		break;
	case CV_OFD:
	case SV_OFD:
	case SV_FD:
		nAction |= DD_ACTION_DISCARD;
		break;
	default:
		break;
	}
	return nAction;
}

static UINT SourceCV_OFAxCV_OFNxCV_OFDxTarget(MSGSourceTarget s, MSGSourceTarget t)
{
	UINT nAction = 0;
	switch (t)
	{
	case CV_OFA:
	case SV_OFA:
	case SV_FA:
		if (s != CV_OFA)
			nAction |= DD_ACTION_ALERT;
		break;
	case CV_OFN:
	case SV_OFN:
	case SV_FN:
		if (s != CV_OFN)
			nAction |= DD_ACTION_NOTIFY;
		break;
	case CV_OFD:
	case SV_OFD:
	case SV_FD:
		if (s != CV_OFD)
			nAction |= DD_ACTION_DISCARD;
		break;
	default:
		break;
	}
	return nAction;

}


static UINT SourceCV_UxTarget(
	CTreeCtrl* pTreeSource, 
	CTreeCtrl* pTreeTarget, 
	CaMessageNodeInfo& source, 
	CaMessageNodeInfo& target)
{
	UINT nAction = 0;
	MSGSourceTarget s = source.GetMsgSourceTarget();
	MSGSourceTarget t = target.GetMsgSourceTarget();
	HTREEITEM hItemS = source.GetHTreeItem();
	HTREEITEM hItemParentS = pTreeSource->GetParentItem(hItemS);
	HTREEITEM hItemT = target.GetHTreeItem();

	CaMessageNode* pNode = (CaMessageNode*)pTreeSource->GetItemData (source.GetHTreeItem());
	ASSERT (pNode->IsInternal() == FALSE);
	if (pNode->IsInternal())
		return nAction;
	CaMessageNodeTree* pNodeTree  = (CaMessageNodeTree*)pNode;
	CaMessage* pMsg = pNodeTree->GetMessageCategory();
	if (!pMsg)
		return nAction;

	if (pTreeSource->GetParentItem(hItemParentS) == pTreeTarget->GetParentItem(hItemT))
	{
		switch (t)
		{
		case CV_R:
		case CV_UF:
			nAction |= DD_ACTION_MOVE;
			break;
		case CV_UFA:
			if (pMsg->GetClass() != IMSG_ALERT)
				nAction |= DD_ACTION_ALERT;
			break;
		case CV_UFN:
			if (pMsg->GetClass() != IMSG_NOTIFY)
				nAction |= DD_ACTION_NOTIFY;
			break;
		case CV_UFD:
			if (pMsg->GetClass() != IMSG_DISCARD)
				nAction |= DD_ACTION_DISCARD;
			break;
		default:
			break;
		}
	}
	else
	{
		//
		// Move Item:
		nAction |= DD_ACTION_MOVE;
		switch (t)
		{
		case CV_R:
		case CV_UF:
			nAction |= DD_ACTION_MOVE;
			break;
		case CV_UFA:
			if (pMsg->GetClass() != IMSG_ALERT)
				nAction |= DD_ACTION_ALERT;
			break;
		case CV_UFN:
			if (pMsg->GetClass() != IMSG_NOTIFY)
				nAction |= DD_ACTION_NOTIFY;
			break;
		case CV_UFD:
			if (pMsg->GetClass() != IMSG_DISCARD)
				nAction |= DD_ACTION_DISCARD;
			break;

		case SV_FA:
			nAction &= ~DD_ACTION_MOVE;
			if (pMsg->GetClass() != IMSG_ALERT)
				nAction |= DD_ACTION_ALERT;
			break;

		case SV_FN:
			nAction &= ~DD_ACTION_MOVE;
			if (pMsg->GetClass() != IMSG_NOTIFY)
				nAction |= DD_ACTION_NOTIFY;
			break;
		case SV_FD:
			nAction &= ~DD_ACTION_MOVE;
			if (pMsg->GetClass() != IMSG_DISCARD)
				nAction |= DD_ACTION_DISCARD;
			break;

		default:
			nAction = 0;
			break;
		}
	}
	return nAction;
}


static UINT SourceCV_UFxTarget(
	CTreeCtrl* pTreeSource, 
	CTreeCtrl* pTreeTarget, 
	CaMessageNodeInfo& source, 
	CaMessageNodeInfo& target)
{
	UINT nAction = 0;
	MSGSourceTarget s = source.GetMsgSourceTarget();
	MSGSourceTarget t = target.GetMsgSourceTarget();
	HTREEITEM hItem  = NULL;
	HTREEITEM hItemS = NULL;
	HTREEITEM hItemT = NULL;

	switch (t)
	{
	case CV_R:
	case CV_UF:
		hItemS = source.GetHTreeItem();
		hItemT = target.GetHTreeItem();
		//
		// Verify if the target is a sub-folder of the source target:
		hItem = pTreeTarget->GetParentItem (hItemT);
		while (hItem)
		{
			if (hItem == hItemS)
			{
				nAction = 0;
				return nAction;
			}
			hItem = pTreeTarget->GetParentItem (hItem);
		}

		//
		// Verify if the parent of the source folder is the 
		// target folder:
		if (pTreeSource->GetParentItem (hItemS) == hItemT)
		{
			nAction = 0;
			return nAction;
		}
		nAction |= DD_ACTION_MOVE;
		break;

	case SV_FA:
		nAction |= DD_ACTION_ALERT;
		break;
	case SV_FN:
		nAction |= DD_ACTION_NOTIFY;
		break;
	case SV_FD:
		nAction |= DD_ACTION_DISCARD;
		break;
		break;
	}
	return nAction;
}


static UINT SourceCV_UFAxCV_UFNxCV_UFDxTarget(
	CTreeCtrl* pTreeSource, 
	CTreeCtrl* pTreeTarget, 
	CaMessageNodeInfo& source, 
	CaMessageNodeInfo& target)
{
	UINT nAction = 0;
	MSGSourceTarget s = source.GetMsgSourceTarget();
	MSGSourceTarget t = target.GetMsgSourceTarget();
	HTREEITEM hItemS = source.GetHTreeItem();
	HTREEITEM hItemT = target.GetHTreeItem();

	if (pTreeSource->GetParentItem(hItemS) == pTreeTarget->GetParentItem(hItemT))
	{
		switch (t)
		{
		case CV_R:
		case CV_UF:
			nAction |= DD_ACTION_MOVE;
			break;
		case CV_UFA:
			nAction |= DD_ACTION_ALERT;
			break;
		case CV_UFN:
			nAction |= DD_ACTION_NOTIFY;
			break;
		case CV_UFD:
			nAction |= DD_ACTION_DISCARD;
			break;
		default:
			break;
		}
		return nAction;
	}

	nAction |= DD_ACTION_MOVE;
	switch (t)
	{
	case CV_R:
	case CV_UF:
		break;
	case CV_UFA:
		nAction |= DD_ACTION_ALERT;
		break;
	case CV_UFN:
		nAction |= DD_ACTION_NOTIFY;
		break;
	case CV_UFD:
		nAction |= DD_ACTION_DISCARD;
		break;
	
	case SV_FA:
		nAction &= ~DD_ACTION_MOVE;
		nAction |= DD_ACTION_ALERT;
		if (s == CV_UFA)
			nAction = 0;
		break;
	case SV_FN:
		nAction &= ~DD_ACTION_MOVE;
		nAction |= DD_ACTION_NOTIFY;
		if (s == CV_UFN)
			nAction = 0;
		break;
	case SV_FD:
		nAction &= ~DD_ACTION_MOVE;
		nAction |= DD_ACTION_DISCARD;
		if (s == CV_UFD)
			nAction = 0;
		break;

	case SV_UFA:
		if (s != CV_UFA)
			nAction |= DD_ACTION_ALERT;
		break;
	case SV_UFN:
		if (s != CV_UFN)
			nAction |= DD_ACTION_NOTIFY;
		break;
	case SV_UFD:
		if (s != CV_UFD)
			nAction |= DD_ACTION_DISCARD;
		break;

	default:
		nAction = 0;
		break;
	}
	return nAction;
}

static UINT SourceSV_OxTarget(
	CTreeCtrl* pTreeSource, 
	CTreeCtrl* pTreeTarget, 
	CaMessageNodeInfo& source, 
	CaMessageNodeInfo& target)
{
	UINT nAction = 0;
	MSGSourceTarget t = target.GetMsgSourceTarget();

	CaMessageNode* pNode = (CaMessageNode*)pTreeSource->GetItemData (source.GetHTreeItem());
	ASSERT (pNode->IsInternal() == FALSE);
	if (pNode->IsInternal())
		return nAction;
	CaMessageNodeTree* pNodeTree  = (CaMessageNodeTree*)pNode;
	CaMessage* pMsg = pNodeTree->GetMessageCategory();
	if (!pMsg)
		return nAction;

	switch (t)
	{
	case SV_OFA:
	case SV_FA:
		if (pMsg->GetClass() != IMSG_ALERT)
			nAction |= DD_ACTION_ALERT;
		break;
	case SV_OFN:
	case SV_FN:
		if (pMsg->GetClass() != IMSG_NOTIFY)
			nAction |= DD_ACTION_NOTIFY;
		break;
	case SV_OFD:
	case SV_FD:
		if (pMsg->GetClass() != IMSG_DISCARD)
			nAction |= DD_ACTION_DISCARD;
		break;
	default:
		break;
	}
	return nAction;
}

static UINT SourceSV_UxTarget(
	CTreeCtrl* pTreeSource, 
	CTreeCtrl* pTreeTarget, 
	CaMessageNodeInfo& source, 
	CaMessageNodeInfo& target)
{
	UINT nAction = 0;
	MSGSourceTarget t = target.GetMsgSourceTarget();

	CaMessageNode* pNode = (CaMessageNode*)pTreeSource->GetItemData (source.GetHTreeItem());
	ASSERT (pNode->IsInternal() == FALSE);
	if (pNode->IsInternal())
		return nAction;
	CaMessageNodeTree* pNodeTree  = (CaMessageNodeTree*)pNode;
	CaMessage* pMsg = pNodeTree->GetMessageCategory();
	if (!pMsg)
		return nAction;

	switch (t)
	{
	case SV_FA:
		if (pMsg->GetClass() != IMSG_ALERT)
			nAction |= DD_ACTION_ALERT;
		break;
	case SV_FN:
		if (pMsg->GetClass() != IMSG_NOTIFY)
			nAction |= DD_ACTION_NOTIFY;
		break;
	case SV_FD:
		if (pMsg->GetClass() != IMSG_DISCARD)
			nAction |= DD_ACTION_DISCARD;
		break;
	default:
		break;
	}
	return nAction;
}

static UINT SourceSV_OFAxSV_OFNxSV_OFDxTarget(
	CTreeCtrl* pTreeSource, 
	CTreeCtrl* pTreeTarget, 
	CaMessageNodeInfo& source, 
	CaMessageNodeInfo& target)
{
	UINT nAction = 0;
	MSGSourceTarget s = source.GetMsgSourceTarget();
	MSGSourceTarget t = target.GetMsgSourceTarget();
	
	switch (t)
	{
	case SV_FA:
	case SV_OFA:
		if (s != SV_OFA)
			nAction |= DD_ACTION_ALERT;
		break;
	case SV_FN:
	case SV_OFN:
		if (s != SV_OFN)
			nAction |= DD_ACTION_NOTIFY;
		break;
	case SV_FD:
	case SV_OFD:
		if (s != SV_OFD)
			nAction |= DD_ACTION_DISCARD;
		break;
	default:
		break;
	}

	return nAction;
}


static UINT SourceSV_UFAxSV_UFNxSV_UFDxTarget(
	CTreeCtrl* pTreeSource, 
	CTreeCtrl* pTreeTarget, 
	CaMessageNodeInfo& source, 
	CaMessageNodeInfo& target)
{
	UINT nAction = 0;
	MSGSourceTarget s = source.GetMsgSourceTarget();
	MSGSourceTarget t = target.GetMsgSourceTarget();
	
	switch (t)
	{
	case SV_FA:
		if (s != SV_UFA)
			nAction |= DD_ACTION_ALERT;
		break;
	case SV_FN:
		if (s != SV_UFN)
			nAction |= DD_ACTION_NOTIFY;
		break;
	case SV_FD:
		if (s != SV_UFD)
			nAction |= DD_ACTION_DISCARD;
		break;
	default:
		break;
	}

	return nAction;
}

static UINT GetCompatibleSourceTarget(
	CTreeCtrl* pTreeSource, 
	CTreeCtrl* pTreeTarget, 
	CaMessageNodeInfo& source, 
	CaMessageNodeInfo& target)
{
	UINT nAction = 0;
	MSGSourceTarget s = source.GetMsgSourceTarget();
	MSGSourceTarget t = target.GetMsgSourceTarget();
	if (s == t && source.GetHTreeItem() == target.GetHTreeItem())
		return nAction;

	switch (s)
	{
	case CV_O:
		nAction = SourceCV_OxTarget(pTreeSource, source, t);
		break;
	case CV_OF:
		nAction = SourceCV_OFxTarget(t);
		break;

	case CV_OFA:
	case CV_OFN:
	case CV_OFD:
		nAction = SourceCV_OFAxCV_OFNxCV_OFDxTarget(s, t);
		break;

	case CV_U:
		nAction = SourceCV_UxTarget(pTreeSource, pTreeTarget, source, target);
		break;

	case CV_UF:
		nAction = SourceCV_UFxTarget(pTreeSource, pTreeTarget, source, target);
		break;

	case CV_UFA:
	case CV_UFN:
	case CV_UFD:
		nAction = SourceCV_UFAxCV_UFNxCV_UFDxTarget(pTreeSource, pTreeTarget, source, target);
		break;

	case SV_O:
		nAction = SourceSV_OxTarget(pTreeSource, pTreeTarget, source, target);
		break;
	case SV_U:
		nAction = SourceSV_UxTarget(pTreeSource, pTreeTarget, source, target);
		break;

	case SV_OFA:
	case SV_OFN:
	case SV_OFD:
		nAction = SourceSV_OFAxSV_OFNxSV_OFDxTarget(pTreeSource, pTreeTarget, source, target);
		break;

	case SV_UFA:
	case SV_UFN:
	case SV_UFD:
		nAction = SourceSV_UFAxSV_UFNxSV_UFDxTarget(pTreeSource, pTreeTarget, source, target);
		break;

	default:
		break;
	}

	return nAction;
}



/////////////////////////////////////////////////////////////////////////////
// CuMessageCategoryTreeCtrl

CuMessageCategoryTreeCtrl::CuMessageCategoryTreeCtrl()
{
	m_bDragging = FALSE;
	m_pImagelist= NULL;
	m_bCanbeDropTarget = FALSE;
	m_pEventSettingFrame = NULL;
}

CuMessageCategoryTreeCtrl::~CuMessageCategoryTreeCtrl()
{
}


BEGIN_MESSAGE_MAP(CuMessageCategoryTreeCtrl, CTreeCtrl)
	//{{AFX_MSG_MAP(CuMessageCategoryTreeCtrl)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(ID_ADD_FOLDER, OnAddFolder)
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginlabeledit)
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndlabeledit)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBegindrag)
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_NOTIFY, OnNotifyMessage)
	ON_COMMAND(ID_ALERT, OnAlertMessage)
	ON_COMMAND(ID_DISCARD, OnDiscardMessage)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_COMMAND(ID_DROP_FOLDER, OnDropFolder)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuMessageCategoryTreeCtrl message handlers

void CuMessageCategoryTreeCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	UINT flags = 0;
	CPoint p = point;
	CTreeCtrl::OnRButtonDown(nFlags, point);
	HTREEITEM hTreeItem = HitTest (p, &flags);
	if (hTreeItem && (flags&TVHT_ONITEM))
	{
		SelectItem (hTreeItem);
		CMenu menu;
		VERIFY(menu.LoadMenu(IDR_POPUP_TREEEVENTCATEGORY));

		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);
		CWnd* pWndPopupOwner = this;
		//
		// Menu handlers are coded in this source:
		// So do not use the following statement:
		// while (pWndPopupOwner->GetStyle() & WS_CHILD)
		//     pWndPopupOwner = pWndPopupOwner->GetParent();
		//
		ClientToScreen (&p);
		//
		// Enable/Disable the Menu Items:
		CaMessageNode* pItemData = (CaMessageNode*)GetItemData(hTreeItem);
		EnableDisableMenuItem (pItemData, &menu);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, p.x, p.y, pWndPopupOwner);
	}
}


void CuMessageCategoryTreeCtrl::OnAddFolder() 
{
	TCHAR tchszMsg[] = _T("Add New Folder failed");
	try
	{
		HTREEITEM hSelected = GetSelectedItem();
		if (!hSelected)
			return;
		CaMessageNode* pItemData = (CaMessageNode*)GetItemData(hSelected);
		if (!pItemData)
			return;

		pItemData->AddFolder (this);
		if (!m_pEventSettingFrame)
			m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
		if (m_pEventSettingFrame)
		{
			CvEventSettingLeft* pLeftView = m_pEventSettingFrame->GetLeftView();
			if (pLeftView)
			{
				CTreeCtrl* pTreeLeft = pLeftView->GetTreeCtrl();
				CWnd* pWnd = pTreeLeft? pTreeLeft->GetParent(): NULL;
				if (pWnd)
					pWnd->PostMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)1, 0);
			}
		}
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CuMessageCategoryTreeCtrl::OnAddFolder): \nAdd New Folder failed"));
	}
}

void CuMessageCategoryTreeCtrl::OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	*pResult = 1;
	CaMessageNode* pItemData = (CaMessageNode*)GetItemData(pTVDispInfo->item.hItem);
	if (!pItemData)
		return;
	if (pItemData->IsInternal())
	{
		switch (pItemData->GetNodeInformation().GetMsgSourceTarget())
		{
		case CV_UF:
			*pResult = 0;
			break;
		default:
			break;
		}
	}
}

void CuMessageCategoryTreeCtrl::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult) 
{
	*pResult = FALSE;
	TV_DISPINFO *ptvinfo;
	ptvinfo = (TV_DISPINFO *)pNMHDR;

	CaMessageNode* pItemData = (CaMessageNode*)GetItemData(ptvinfo->item.hItem);
	if (!pItemData)
		return;

	if (ptvinfo->item.pszText != NULL)
	{
		CString strMsg;
		CString strItem = ptvinfo->item.pszText;
		//
		// The character '/' is reserved:
		if (strItem.Find (_T('/')) != -1)
		{
			if (!strMsg.LoadString(IDS_MSG_RESERVE_C1))
				strMsg = _T("The character '/' is actually reserved for internal use.");
			AfxMessageBox (strMsg);
			return;
		}

		//
		// Check to see if the item exists:
		HTREEITEM hParentItem = GetParentItem (pItemData->GetNodeInformation().GetHTreeItem());
		if (hParentItem)
		{
			CaMessageNode* pItemParent = (CaMessageNode*)GetItemData(hParentItem);
			if (pItemParent && pItemParent->IsInternal())
			{
				CaMessageNode* pNode = NULL;
				CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pItemParent;
				CTypedPtrList<CObList, CaMessageNode*>* pLs = pI->GetListSubNode();
				if (pLs)
				{
					POSITION pos = pLs->GetHeadPosition();
					while (pos != NULL)
					{
						pNode = pLs->GetNext (pos);
						if (strItem.CompareNoCase (pNode->GetNodeInformation().GetNodeTitle()) == 0)
						{
							//_T("Cannot change the label.\nThe item '%s' exists already.")
							AfxFormatString1(strMsg, IDS_MSG_CANNOT_CHANGE_LABEL1, (LPCTSTR)strItem);
							AfxMessageBox (strMsg);
							return;
						}
					}
				}
			}
		}

		ptvinfo->item.mask = TVIF_TEXT;
		SetItem(&ptvinfo->item);
		pItemData->GetNodeInformation().SetNodeTitle (ptvinfo->item.pszText);
		*pResult = TRUE;

		if (hParentItem)
		{
			CaMessageNode* pItemParent = (CaMessageNode*)GetItemData(hParentItem);
			if (pItemParent && pItemParent->IsInternal())
				pItemParent->SortItems(this);
		}

		if (!m_pEventSettingFrame)
			m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
		if (m_pEventSettingFrame)
		{
			CvEventSettingLeft* pLeftView = m_pEventSettingFrame->GetLeftView();
			if (pLeftView)
			{
				CTreeCtrl* pTreeLeft = pLeftView->GetTreeCtrl();
				CWnd* pWnd = pTreeLeft? pTreeLeft->GetParent(): NULL;
				if (pWnd)
					pWnd->PostMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)1, 0);
			}
		}
	}
}

void CuMessageCategoryTreeCtrl::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hExpand = pNMTreeView->itemNew.hItem;

	CaMessageNode* pItemData = (CaMessageNode*)GetItemData(hExpand);
	if (!pItemData)
		return;
	
	if (pNMTreeView->action & TVE_COLLAPSE)
	{
		//
		// Collapseing:
		CaMessageNodeInfo& nodeInfo = pItemData->GetNodeInformation();
		nodeInfo.SetExpanded (FALSE);
		int nImageClose, nImageOpen;
		nodeInfo.GetIconImage (nImageClose, nImageOpen);
		if (nodeInfo.IsExpanded())
			SetItemImage (hExpand, nImageOpen, nImageOpen);
		else
			SetItemImage (hExpand, nImageClose, nImageClose);
	}
	else
	if (pNMTreeView->action & TVE_EXPAND)
	{
		//
		// Expanding:
		CaMessageNodeInfo& nodeInfo = pItemData->GetNodeInformation();
		nodeInfo.SetExpanded (TRUE);
		int nImageClose, nImageOpen;
		nodeInfo.GetIconImage (nImageClose, nImageOpen);
		if (nodeInfo.IsExpanded())
			SetItemImage (hExpand, nImageOpen, nImageOpen);
		else
			SetItemImage (hExpand, nImageClose, nImageClose);
	}
	*pResult = 0;
}

void CuMessageCategoryTreeCtrl::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	CPoint ptAction, p;
	UINT nFlags;

	GetCursorPos(&ptAction);
	p = ptAction;
	ASSERT(!m_bDragging);
	ScreenToClient(&p);
	m_hitemDrag = HitTest(p, &nFlags);
	if (!(m_hitemDrag || (nFlags&TVHT_ONITEM)))
	{
		m_hitemDrag = NULL;
		return;
	}
	SelectItem (m_hitemDrag);

	m_bDragging = TRUE;
	m_hitemDrop = NULL;

	ASSERT(m_pImagelist == NULL);
	m_pImagelist = CreateDragImage(m_hitemDrag);  // get the image list for dragging
	if (!m_pImagelist)
	{
		m_bDragging = FALSE;
		m_hitemDrag = NULL;
		return;
	}
	m_pImagelist->DragShowNolock(TRUE);
	m_pImagelist->SetDragCursorImage(0, CPoint(0, 0));
	m_pImagelist->BeginDrag(0, CPoint(0,0));
	m_pImagelist->DragMove(ptAction);
	m_pImagelist->DragEnter(GetParentFrame(), ptAction);
	SetCapture();
}


void CuMessageCategoryTreeCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	OnButtonUp(point);
	try
	{
		m_bDragging = FALSE;
		m_hitemDrop = NULL;
		m_pImagelist= NULL;
		CTreeCtrl::OnLButtonUp(nFlags, point);
	}
	catch (...)
	{
		MessageBeep (MB_ICONEXCLAMATION);
		TRACE0 ("Raise exception: CuMessageCategoryTreeCtrl::OnLButtonUp \n");
	}
}

void CuMessageCategoryTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	CTreeCtrl::OnLButtonDown(nFlags, point);
}

void CuMessageCategoryTreeCtrl::OnRButtonUp(UINT nFlags, CPoint point) 
{
	OnButtonUp(point);
	try 
	{
		m_bDragging = FALSE;
		m_hitemDrop = NULL;
		m_pImagelist= NULL;
		CTreeCtrl::OnRButtonUp(nFlags, point);
	}
	catch (...)
	{
		MessageBeep (MB_ICONEXCLAMATION);
		TRACE0 ("Raise exception: CuMessageCategoryTreeCtrl::OnRButtonUp\n");
	}
}

void CuMessageCategoryTreeCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
//TRACE2 ("OnMouseMove (%d, %d)\n", point.x, point.y);
	try
	{
		UINT flags = 0;
		CPoint p = point;
		ClientToScreen (&p);
		CPoint pDragMove = p;
		if (!m_bDragging)
		{
			CTreeCtrl::OnMouseMove(nFlags, point);
			return;
		}

		if (!m_pEventSettingFrame)
			m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
		ASSERT (m_pEventSettingFrame);
		if (!m_pEventSettingFrame)
		{
			CTreeCtrl::OnMouseMove(nFlags, point);
			return;
		}
		
		m_pEventSettingFrame->ScreenToClient(&pDragMove);
		ASSERT(m_pImagelist != NULL);
		if (!m_pImagelist)
		{
			m_bDragging = FALSE;
			m_hitemDrag = NULL;
			CTreeCtrl::OnMouseMove(nFlags, point);
			return;
		}

		m_pImagelist->DragMove(pDragMove);
		m_pImagelist->DragLeave(GetParentFrame());
		//
		// Determine the pane under the mouse pointer:
		CRect rFrame, rLeftTree, rRightTree, rBottomList;
		m_pEventSettingFrame->GetWindowRect (rFrame);

		CvEventSettingLeft*   pLeftView  = m_pEventSettingFrame->GetLeftView();
		CvEventSettingRight*  pRightView = m_pEventSettingFrame->GetRightView();
		CvEventSettingBottom* pBottomView= m_pEventSettingFrame->GetBottomView();
		if (!(pLeftView && pRightView && pBottomView))
		{
			CTreeCtrl::OnMouseMove(nFlags, point);
			return;
		}
		CTreeCtrl* pTreeLeft  = pLeftView->GetTreeCtrl();
		CTreeCtrl* pTreeRight = pRightView->GetTreeCtrl();
		CListCtrl* pListBottom= pBottomView->GetListCtrl();
		if (!(pTreeLeft && pTreeRight && pListBottom))
		{
			CTreeCtrl::OnMouseMove(nFlags, point);
			return;
		}

		pTreeLeft->GetWindowRect  (rLeftTree);
		pTreeRight->GetWindowRect (rRightTree);
		pListBottom->GetWindowRect(rBottomList);
		
		CWnd* pWndTarget = NULL;
		CPoint cursorPos;
		GetCursorPos (&cursorPos);

		int nPane = 0;
		if (rFrame.PtInRect (cursorPos))
		{
			if (rLeftTree.PtInRect (cursorPos))
			{
				pWndTarget = pTreeLeft;
				nPane = 1;
			}
			else
			if (rRightTree.PtInRect (cursorPos))
			{
				pWndTarget = pTreeRight;
				nPane = 2;
			}
			else
			if (rBottomList.PtInRect (cursorPos))
			{
				pWndTarget = pListBottom;
				nPane = 3;
			}
		}

		if (pWndTarget && m_hitemDrag)
		{
			UINT nAction = 0;
			HTREEITEM hTreeItemTarget = NULL;
			CPoint pt = point;
			pt.y += 8;
			ClientToScreen (&pt);

			CaMessageNode* pNodeSource = (CaMessageNode*)GetItemData (m_hitemDrag);
			CaMessageNode* pNodeTarget = NULL;
			switch (nPane)
			{
			case 1:
				pTreeLeft->ScreenToClient(&pt);
				hTreeItemTarget = pTreeLeft->HitTest (pt, &flags);
				if (hTreeItemTarget && (flags&TVHT_ONITEM))
				{
					pTreeLeft->SelectItem (hTreeItemTarget);
					pNodeTarget = (CaMessageNode*)pTreeLeft->GetItemData (hTreeItemTarget);
					nAction = GetCompatibleSourceTarget(
						this, 
						pTreeLeft, 
						pNodeSource->GetNodeInformation(), 
						pNodeTarget->GetNodeInformation());
				}
				break;
			case 2:
				pTreeRight->ScreenToClient(&pt);
				hTreeItemTarget = pTreeRight->HitTest (pt, &flags);
				if (hTreeItemTarget && (flags&TVHT_ONITEM))
				{
					pTreeRight->SelectItem (hTreeItemTarget);
					pNodeTarget = (CaMessageNode*)pTreeRight->GetItemData (hTreeItemTarget);
					nAction = GetCompatibleSourceTarget(
						this, 
						pTreeRight, 
						pNodeSource->GetNodeInformation(), 
						pNodeTarget->GetNodeInformation());
				}
				break;
			case 3:
				break;
			default :
				break;

			}

			TRACE1 ("DD Action = %x\n", nAction);
			if (nAction != 0)
				::SetCursor (theApp.m_hCursorDropCategory);
			else
				::SetCursor (theApp.m_hCursorNoDropCategory);
		}
		else
			::SetCursor (theApp.m_hCursorNoDropCategory);
		m_pImagelist->DragEnter(GetParentFrame(), pDragMove);
	}
	catch (...)
	{
		TRACE0 ("Internal error(1): CuMessageCategoryTreeCtrl::OnMouseMove \n");
	}
	CTreeCtrl::OnMouseMove(nFlags, point);
}


void CuMessageCategoryTreeCtrl::OnButtonUp(CPoint point)
{
	try
	{
		UINT flags = 0;
		UINT nFlags = 0;
		CPoint p = point;
		ClientToScreen (&p);
		class CRelease
		{
		public:
			CRelease(BOOL* bDrag){m_pbDrag = bDrag;}
			~CRelease()
			{
				ReleaseCapture();
				*m_pbDrag = FALSE;
			}
		private:
			BOOL* m_pbDrag;
		};
		CRelease rRelease (&m_bDragging);

		if (m_bDragging && m_hitemDrag)
		{
			ASSERT(m_pImagelist != NULL);
			m_pImagelist->DragLeave(GetParentFrame());
			m_pImagelist->EndDrag();
			delete m_pImagelist;
			m_pImagelist = NULL;

			//
			// Determine the pane under the mouse pointer:
			CRect rFrame, rLeftTree, rRightTree, rBottomList;
			if (!m_pEventSettingFrame)
				m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
			ASSERT (m_pEventSettingFrame);
			if (!m_pEventSettingFrame)
				return;
			m_pEventSettingFrame->GetWindowRect (rFrame);

			CvEventSettingLeft*   pLeftView  = m_pEventSettingFrame->GetLeftView();
			CvEventSettingRight*  pRightView = m_pEventSettingFrame->GetRightView();
			CvEventSettingBottom* pBottomView= m_pEventSettingFrame->GetBottomView();
			if (!(pLeftView && pRightView && pBottomView))
				return;
			CTreeCtrl* pTreeLeft  = pLeftView->GetTreeCtrl();
			CTreeCtrl* pTreeRight = pRightView->GetTreeCtrl();
			CListCtrl* pListBottom= pBottomView->GetListCtrl();
			if (!(pTreeLeft && pTreeRight && pListBottom))
				return;

			pTreeLeft->GetWindowRect  (rLeftTree);
			pTreeRight->GetWindowRect (rRightTree);
			pListBottom->GetWindowRect(rBottomList);
			
			CWnd* pWndTarget = NULL;
			CPoint cursorPos;
			GetCursorPos (&cursorPos);

			int nPane = 0;
			if (rFrame.PtInRect (cursorPos))
			{
				if (rLeftTree.PtInRect (cursorPos))
				{
					pWndTarget = pTreeLeft;
					nPane = 1;
				}
				else
				if (rRightTree.PtInRect (cursorPos))
				{
					pWndTarget = pTreeRight;
					nPane = 2;
				}
				else
				if (rBottomList.PtInRect (cursorPos))
				{
					pWndTarget = pListBottom;
					nPane = 3;
				}
			}

			UINT nAction = 0;
			HTREEITEM hTreeItemTarget = NULL;
			CPoint pt = point;
			pt.y += 8;
			ClientToScreen (&pt);

			CTreeCtrl* pTreeCtrl = NULL;
			CaMessageNode* pNodeSource = (CaMessageNode*)GetItemData (m_hitemDrag);
			CaMessageNode* pNodeTarget = NULL;
			switch (nPane)
			{
			case 1:
				pTreeCtrl = pTreeLeft;
				break;
			case 2:
				pTreeCtrl = pTreeRight;
				break;
			case 3:
				break;
			default :
				break;
			}

			if (!pTreeCtrl)
				return;
			pTreeCtrl->ScreenToClient(&pt);
			hTreeItemTarget = pTreeCtrl->HitTest (pt, &flags);
			if (hTreeItemTarget && (flags&TVHT_ONITEM))
			{
				pTreeCtrl->SelectItem (hTreeItemTarget);
				pNodeTarget = (CaMessageNode*)pTreeCtrl->GetItemData (hTreeItemTarget);
				nAction = GetCompatibleSourceTarget(
					this, 
					pTreeCtrl, 
					pNodeSource->GetNodeInformation(), 
					pNodeTarget->GetNodeInformation());
			}

			BOOL bParent = FALSE;
			switch (pNodeSource->GetNodeInformation().GetMsgSourceTarget())
			{
			case CV_OFA:
			case CV_OFN:
			case CV_OFD:
			case CV_UFA:
			case CV_UFN:
			case CV_UFD:
				bParent = TRUE;
				break;
			default:
				break;
			}

			if (nAction == DD_ACTION_MOVE)
			{
				//
				// Only move item and retain the current state:
				BOOL bAsk4Confirmation = FALSE;
				CString strMsg;
				if (pNodeSource->IsInternal() || bParent) {
					//_T("Move folder '%s' ?"), 
					AfxFormatString1 (strMsg, IDS_MSG_MOVE_FOLDER, (LPCTSTR)pNodeSource->GetNodeInformation().GetNodeTitle());
					bAsk4Confirmation = TRUE;
				}
				else
					// _T("Do you wish to move the item ?");
					strMsg.LoadString (IDS_MSG_MOVE_ITEM);

				if (bAsk4Confirmation) {
					int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
					if (nRes == IDNO)
						return;
				}

				MoveItems (pNodeSource, pNodeTarget, pTreeCtrl);
				GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
				CvEventSettingBottom* pView = m_pEventSettingFrame->GetBottomView();
				if (pView && pView->GetDialog())
					pView->GetDialog()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)-1, (LPARAM)-1);
				return;
			}

			CString strFolder;
			CString strItem;
			if (strFolder.LoadString(IDS_FOLDER_x))
				strFolder = _T("folder");
			if (strItem.LoadString(IDS_ITEM_x))
				strItem = _T("item");

			if (nAction & DD_ACTION_ALERT)
			{
				if (nAction & DD_ACTION_MOVE)
				{
					CString strSource = (pNodeSource->IsInternal() || bParent)? strFolder: strItem;
					CString strMsg;
					if (pNodeSource->IsInternal() || bParent)
						//_T("Do you wish to change the state to alert and move the %s '%s' ?"), 
						AfxFormatString2 (
							strMsg, 
							IDS_MSG_ALERTxMOVE_FOLDER, 
							(LPCTSTR)strSource, 
							(LPCTSTR)pNodeSource->GetNodeInformation().GetNodeTitle());
					else
						// _T("Do you wish to change the state to alert and move the item ?");
						strMsg.LoadString (IDS_MSG_ALERTxMOVE_ITEM);
					int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
					if (nRes == IDNO)
						return;

					MoveItems (pNodeSource, pNodeTarget, pTreeCtrl);
				}
				else
				{
					if (pNodeSource->IsInternal() || bParent)
					{
						// _T("All the messages in the sub-branches will be set to alert.\nDo you wish to continue ?.");
						CString strMsg;
						strMsg.LoadString (IDS_MSG_ALERT_BRANCHxSUB);
						int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
						if (nRes == IDNO)
							return;
					}
				}
				pNodeSource->AlertMessage(this);
				GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
			}
			else
			if (nAction & DD_ACTION_NOTIFY)
			{
				if (nAction & DD_ACTION_MOVE)
				{
					CString strSource = (pNodeSource->IsInternal() || bParent)? strFolder: strItem;
					CString strMsg;
					if (pNodeSource->IsInternal() || bParent)
						// _T("Do you wish to change the state to notify and move the %s '%s' ?")
						AfxFormatString2 (
							strMsg, 
							IDS_MSG_NOTIFYxMOVE_FOLDER, 
							(LPCTSTR)strSource, 
							(LPCTSTR)pNodeSource->GetNodeInformation().GetNodeTitle());
					else
						// _T("Do you wish to change the state to notify and move the item ?");
						strMsg.LoadString(IDS_MSG_NOTIFYxMOVE_ITEM);
		
					int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
					if (nRes == IDNO)
						return;

					MoveItems (pNodeSource, pNodeTarget, pTreeCtrl);
				}
				else
				{
					if (pNodeSource->IsInternal() || bParent)
					{
						// _T("All the messages in the sub-branches will be set to notify.\nDo you wish to continue ?.");
						CString strMsg;
						strMsg.LoadString(IDS_MSG_NOTIFY_BRANCHxSUB);
						int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
						if (nRes == IDNO)
							return;
					}
				}
				pNodeSource->NotifyMessage(this);
				GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
			}
			else
			if (nAction & DD_ACTION_DISCARD)
			{
				if (nAction & DD_ACTION_MOVE)
				{
					CString strSource = (pNodeSource->IsInternal() || bParent)? strFolder: strItem;
					CString strMsg;
					if (pNodeSource->IsInternal() || bParent)
						// _T("Do you wish to change the state to discard and move the %s '%s' ?")
						AfxFormatString2 (
							strMsg, 
							IDS_MSG_DISCARDxMOVE_FOLDER, 
							(LPCTSTR)strSource, 
							(LPCTSTR)pNodeSource->GetNodeInformation().GetNodeTitle());
						else
							// _T("Do you wish to change the state to discard and move the item ?");
							strMsg.LoadString (IDS_MSG_DISCARDxMOVE_ITEM); 
					int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
					if (nRes == IDNO)
						return;

					MoveItems (pNodeSource, pNodeTarget, pTreeCtrl);
				}
				else
				{
					if (pNodeSource->IsInternal() || bParent)
					{
						// _T("All the messages in the sub-branches will be set to discard.\nDo you wish to continue ?.")
						CString strMsg;
						strMsg.LoadString(IDS_MSG_DISCARD_BRANCHxSUB);
						int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
						if (nRes == IDNO)
							return;
					}
				}
				pNodeSource->DiscardMessage(this);
				GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
			}
			else
			{
				MessageBeep(MB_ICONEXCLAMATION);
				m_bDragging = FALSE;
				return;
			}
			CvEventSettingBottom* pView = m_pEventSettingFrame->GetBottomView();
			if (pView && pView->GetDialog())
				pView->GetDialog()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)-1, (LPARAM)-1);

			m_bDragging = FALSE;
		}
	}
	catch (...)
	{
		TRACE0 ("Internal error(1): CuMessageCategoryTreeCtrl::OnButtonUp\n");
	}
}

void CuMessageCategoryTreeCtrl::OnNotifyMessage() 
{
	CWaitCursor doWaitCursor;
	HTREEITEM hSelected  = GetSelectedItem();
	if (!hSelected)
		return;
	CaMessageNode* pNode = (CaMessageNode*)GetItemData(hSelected);
	ASSERT (pNode);
	if (!pNode)
		return;
	BOOL bParent = FALSE;
	switch (pNode->GetNodeInformation().GetMsgSourceTarget())
	{
	case CV_OFA:
	case CV_OFN:
	case CV_OFD:
	case CV_UFA:
	case CV_UFN:
	case CV_UFD:
		bParent = TRUE;
		break;
	default:
		break;
	}
	if (pNode->IsInternal() || bParent)
	{
		// _T("All the messages in the sub-branches will be set to notify.\nDo you wish to continue ?.");
		CString strMsg;
		strMsg.LoadString (IDS_MSG_NOTIFY_BRANCHxSUB);
		int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
		if (nRes == IDNO)
			return;
	}
	pNode->NotifyMessage(this);
	GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 1);

	if (!m_pEventSettingFrame)
		m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
	ASSERT (m_pEventSettingFrame);
	if (m_pEventSettingFrame)
	{
		CvEventSettingBottom* pView = m_pEventSettingFrame->GetBottomView();
		if (pView && pView->GetDialog())
			pView->GetDialog()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)-1, (LPARAM)-1);
	}

	TRACE0("CuMessageCategoryTreeCtrl::OnNotifyMessage\n");
}

void CuMessageCategoryTreeCtrl::OnAlertMessage() 
{
	CWaitCursor doWaitCursor;
	HTREEITEM hSelected  = GetSelectedItem();
	if (!hSelected)
		return;
	CaMessageNode* pNode = (CaMessageNode*)GetItemData(hSelected);
	ASSERT (pNode);
	if (!pNode)
		return;
	BOOL bParent = FALSE;
	switch (pNode->GetNodeInformation().GetMsgSourceTarget())
	{
	case CV_OFA:
	case CV_OFN:
	case CV_OFD:
	case CV_UFA:
	case CV_UFN:
	case CV_UFD:
		bParent = TRUE;
		break;
	default:
		break;
	}
	if (pNode->IsInternal() || bParent)
	{
		// _T("All the messages in the sub-branches will be set to alert.\nDo you wish to continue ?.");
		CString strMsg;
		strMsg.LoadString (IDS_MSG_ALERT_BRANCHxSUB);
		int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
		if (nRes == IDNO)
			return;
	}
	pNode->AlertMessage(this);
	GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 1);
	
	if (!m_pEventSettingFrame)
		m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
	ASSERT (m_pEventSettingFrame);
	if (m_pEventSettingFrame)
	{
		CvEventSettingBottom* pView = m_pEventSettingFrame->GetBottomView();
		if (pView && pView->GetDialog())
			pView->GetDialog()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)-1, (LPARAM)-1);
	}

	TRACE0("CuMessageCategoryTreeCtrl::OnAlertMessage\n");
}

void CuMessageCategoryTreeCtrl::OnDiscardMessage() 
{
	CWaitCursor doWaitCursor;
	HTREEITEM hSelected  = GetSelectedItem();
	if (!hSelected)
		return;
	CaMessageNode* pNode = (CaMessageNode*)GetItemData(hSelected);
	ASSERT (pNode);
	if (!pNode)
		return;
	BOOL bParent = FALSE;
	switch (pNode->GetNodeInformation().GetMsgSourceTarget())
	{
	case CV_OFA:
	case CV_OFN:
	case CV_OFD:
	case CV_UFA:
	case CV_UFN:
	case CV_UFD:
		bParent = TRUE;
		break;
	default:
		break;
	}
	if (pNode->IsInternal() || bParent)
	{
		// _T("All the messages in the sub-branches will be set to discard.\nDo you wish to continue ?.");
		CString strMsg;
		strMsg.LoadString (IDS_MSG_DISCARD_BRANCHxSUB);
		int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
		if (nRes == IDNO)
			return;
	}
	pNode->DiscardMessage(this);
	GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
	
	if (!m_pEventSettingFrame)
		m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
	ASSERT (m_pEventSettingFrame);
	if (m_pEventSettingFrame)
	{
		CvEventSettingBottom* pView = m_pEventSettingFrame->GetBottomView();
		if (pView && pView->GetDialog())
			pView->GetDialog()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)-1, (LPARAM)-1);
	}

	TRACE0("CuMessageCategoryTreeCtrl::OnDiscardMessage\n");
}

void CuMessageCategoryTreeCtrl::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	if (pNMTreeView->itemNew.hItem != NULL)
	{
		CaMessageNode* pNode = (CaMessageNode*)GetItemData(pNMTreeView->itemNew.hItem);
	}
	*pResult = 0;
}

BOOL CuMessageCategoryTreeCtrl::MoveItems (CaMessageNode* pNodeSource, CaMessageNode* pNodeTarget, CTreeCtrl* pTree)
{
	if (pNodeSource->GetNodeInformation().GetMsgSourceTarget() == CV_U)
		return MoveSingleItem (pNodeSource, pNodeTarget, pTree);
	else
		return MoveMultipleItems (pNodeSource, pNodeTarget, pTree);
}

BOOL CuMessageCategoryTreeCtrl::MoveSingleItem (CaMessageNode* pNodeSource, CaMessageNode* pNodeTarget, CTreeCtrl* pTree)
{
	CaMessageNodeInfo& source = pNodeSource->GetNodeInformation();
	CaMessageNodeInfo& target = pNodeTarget->GetNodeInformation();

	MSGSourceTarget s = source.GetMsgSourceTarget();
	MSGSourceTarget t = target.GetMsgSourceTarget();
	HTREEITEM hItemS, hItemParentS;
	CaMessageNode* pObjParent = NULL;

	hItemS = source.GetHTreeItem();
	ASSERT(hItemS);
	if (!hItemS)
		return FALSE;
	hItemParentS = GetParentItem (hItemS);
	ASSERT (hItemParentS);
	if (!hItemParentS)
		return FALSE;

	//
	// Check if the Source Item does not have the state folder <Alert>, <Notify>, <Discard>
	BOOL bHaveStateFolder = FALSE;
	pObjParent = (CaMessageNode*)GetItemData (hItemParentS);
	ASSERT (pObjParent);
	if (!pObjParent)
		return FALSE;
	switch (pObjParent->GetNodeInformation().GetMsgSourceTarget())
	{
	case CV_R:
	case CV_UF:
		//
		// Does not have the state folder <Alert>, <Notify>, <Discard>
		bHaveStateFolder = FALSE;
		break;
	case CV_UFA:
	case CV_UFN:
	case CV_UFD:
		//
		// Do have the state folder <Alert>, <Notify>, <Discard>
		bHaveStateFolder = TRUE;
		break;
	default:
		return FALSE;
		break;
	}

	CaMessageNodeTreeInternal* pI = NULL;
	if (bHaveStateFolder)
	{
		HTREEITEM hIternal = GetParentItem (hItemParentS);
		ASSERT (hIternal);
		if (!hIternal)
			return FALSE;
		pObjParent = (CaMessageNode*)GetItemData (hIternal);
		ASSERT (pObjParent->IsInternal());
		if (!pObjParent->IsInternal())
			return FALSE;
		pI = (CaMessageNodeTreeInternal*)pObjParent;
	}
	else
	{
		pI = (CaMessageNodeTreeInternal*)pObjParent;
	}

	//
	// Remove the source item:
	BOOL bFound = FALSE;
	CaMessageNode* pObj = NULL;
	CTypedPtrList<CObList, CaMessageNode*>* pLs = pI->GetListSubNode();
	if (!pLs)
		return FALSE;
	POSITION p, pos = pLs->GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pObj = pLs->GetNext (pos);
		if (pObj->IsInternal())
			continue;

		if (pObj->GetNodeInformation().GetHTreeItem() == pNodeSource->GetNodeInformation().GetHTreeItem())
		{
			pLs->RemoveAt (p);
			DeleteItem (pObj->GetNodeInformation().GetHTreeItem());
			pObj->GetNodeInformation().SetHTreeItem(NULL);
			bFound = TRUE;
			pObj = pNodeSource;
			break;
		}
		else
		{
			pObj = NULL;
		}
	}

	if (!(bFound && pObj))
		return FALSE;

	//
	// Determine the Target location:
	CaMessageNodeTreeInternal* pIt = NULL;
	switch (t)
	{
	case CV_R:
	case CV_UF:
		ASSERT (pNodeTarget->IsInternal());
		if (!pNodeTarget->IsInternal())
			return FALSE;
		pIt = (CaMessageNodeTreeInternal*)pNodeTarget;
		break;

	case CV_UFA:
	case CV_UFN:
	case CV_UFD:
		{
			HTREEITEM hParentT = pTree->GetParentItem (target.GetHTreeItem());
			pObjParent = (CaMessageNode*)pTree->GetItemData (hParentT);
			ASSERT (pObjParent->IsInternal());
			if (!pObjParent->IsInternal())
				return FALSE;
			pIt = (CaMessageNodeTreeInternal*)pObjParent;
		}
		break;
	default:
		return FALSE;
		break;
	}

	if (!pIt)
		return FALSE;

	//
	// Put the source item to the Destination:
	pIt->AddMessage (pObj);

	return TRUE;
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
					hTreeItem = pObj->GetNodeInformation().GetHTreeItem();
					if (hTreeItem)
						pTree->DeleteItem (hTreeItem);
					pObj->GetNodeInformation().SetHTreeItem(NULL);
				}
			}
		}
		hTreeItem = pNode->GetNodeInformation().GetHTreeItem();
		if (hTreeItem)
			pTree->DeleteItem (hTreeItem);
		pNode->GetNodeInformation().SetHTreeItem(NULL);
	}
	else
	{
		HTREEITEM hTreeItem = pNode->GetNodeInformation().GetHTreeItem();
		if (hTreeItem)
			pTree->DeleteItem (hTreeItem);
		pNode->GetNodeInformation().SetHTreeItem(NULL);
	}
}


BOOL CuMessageCategoryTreeCtrl::MoveMultipleItems (CaMessageNode* pNodeSource, CaMessageNode* pNodeTarget, CTreeCtrl* pTree)
{
	CaMessageNodeInfo& source = pNodeSource->GetNodeInformation();
	CaMessageNodeInfo& target = pNodeTarget->GetNodeInformation();

	MSGSourceTarget s = source.GetMsgSourceTarget();
	MSGSourceTarget t = target.GetMsgSourceTarget();
	HTREEITEM hItemS, hItemParentS;
	HTREEITEM hItem;
	HTREEITEM hItemT, hItemParentT;

	CaMessageNode* pObjParent = NULL;
	CaMessageNode* pObjParentT= NULL;
	hItemS = source.GetHTreeItem();
	ASSERT(hItemS);
	if (!hItemS)
		return FALSE;
	hItemParentS = GetParentItem (hItemS);
	ASSERT (hItemParentS);
	if (!hItemParentS)
		return FALSE;

	hItemT = target.GetHTreeItem();
	ASSERT(hItemT);
	if (!hItemT)
		return FALSE;
	hItemParentT = pTree->GetParentItem (hItemT);

	pObjParent = (CaMessageNode*)GetItemData (hItemParentS);
	ASSERT (pObjParent && pObjParent->IsInternal());
	if (!(pObjParent && pObjParent->IsInternal()))
		return FALSE;

	if (hItemParentT)
	{
		pObjParentT = (CaMessageNode*)pTree->GetItemData (hItemParentT);
		ASSERT (pObjParentT && pObjParentT->IsInternal());
		if (!(pObjParentT && pObjParentT->IsInternal()))
		return FALSE;
	}

	//
	// The Source Item must be a User Folder (CV_UF) or the state folder <Alert>, <Notify>, <Discard>.
	// The source Item must be internal node.
	CaMessageNodeTreeInternal* pI = NULL;

	BOOL bFound = FALSE;
	CaMessageNode* pObj = NULL;
	CTypedPtrList<CObList, CaMessageNode*>* pLs = NULL;
	POSITION p = NULL, pos = NULL;

	//
	// Determine the Target location:
	CaMessageNodeTreeInternal* pIt = NULL;
	switch (t)
	{
	case CV_R:
	case CV_UF:
		ASSERT (pNodeTarget->IsInternal());
		if (!pNodeTarget->IsInternal())
			return FALSE;
		pIt = (CaMessageNodeTreeInternal*)pNodeTarget;
		break;

	case CV_UFA:
	case CV_UFN:
	case CV_UFD:
		pIt = NULL;
		if (hItemParentT)
			pIt = (CaMessageNodeTreeInternal*)pObjParentT;
		break;
	default:
		return FALSE;
		break;
	}

	if (!pIt)
		return FALSE;

	switch (source.GetMsgSourceTarget())
	{
	case CV_UF:
		if (t == CV_UFA || t == CV_UFN || t == CV_UFD)
			break;
		//
		// Check to see if the Target contains already the same folder name:
		pLs = pIt->GetListSubNode();
		if (pLs)
		{
			CString strSTitle = source.GetNodeTitle();
			pos = pLs->GetHeadPosition();
			while (pos != NULL)
			{
				pObj = pLs->GetNext (pos);
				if (!pObj->IsInternal())
					continue;

				if (strSTitle.CompareNoCase (pObj->GetNodeInformation().GetNodeTitle()) == 0)
				{
					// _T("The target folder contains already the folder '%s'")
					CString strMsg;
					AfxFormatString1 (strMsg, IDS_MSG_FOLDER_EXIST_IN_TARGET, (LPCTSTR)strSTitle);
					AfxMessageBox (strMsg);
					return FALSE;
				}
			}
		}

		pI = (CaMessageNodeTreeInternal*)pObjParent;
		//
		// Remove the source item:
		pLs = pI->GetListSubNode();
		if (!pLs)
			return FALSE;
		pos = pLs->GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			pObj = pLs->GetNext (pos);
			if (!pObj->IsInternal())
				continue;

			if (pObj->GetNodeInformation().GetHTreeItem() == pNodeSource->GetNodeInformation().GetHTreeItem())
			{
				DestroyTreeCtrlFolder(this, pNodeSource);
				pLs->RemoveAt (p);
				pObj->GetNodeInformation().SetHTreeItem(NULL);
				bFound = TRUE;
				pObj = pNodeSource;
				pIt->AddMessage (pObj);
				break;
			}
		}
		break;

	case CV_UFA:
	case CV_UFN:
	case CV_UFD:
		pI = (CaMessageNodeTreeInternal*)pObjParent;
		//
		// Remove the source item:
		pLs = pI->GetListSubNode();
		if (!pLs)
			return FALSE;
		pos = pLs->GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			pObj = pLs->GetNext (pos);
			if (pObj->IsInternal())
				continue;

			hItem = pObj->GetNodeInformation().GetHTreeItem();
			if (hItem && pTree->GetParentItem(hItem) == pNodeSource->GetNodeInformation().GetHTreeItem())
			{
				pLs->RemoveAt (p);
				pTree->DeleteItem (hItem);
				pObj->GetNodeInformation().SetHTreeItem(NULL);
				bFound = TRUE;

				pIt->AddMessage (pObj);
			}
		}
		break;
	default:
		return FALSE;
		break;
	}

	return TRUE;
}


void CuMessageCategoryTreeCtrl::EnableDisableMenuItem (CaMessageNode* pItemData, CMenu* pPopup)
{
	pPopup->EnableMenuItem (ID_ALERT,      MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
	pPopup->EnableMenuItem (ID_NOTIFY,     MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
	pPopup->EnableMenuItem (ID_DISCARD,    MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
	pPopup->EnableMenuItem (ID_DROP_FOLDER,MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);
	pPopup->EnableMenuItem (ID_ADD_FOLDER, MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

	MSGSourceTarget t = pItemData->GetNodeInformation().GetMsgSourceTarget();
	switch (t)
	{
	case CV_R:
	case CV_UF:
		pPopup->EnableMenuItem (ID_ALERT,      MF_BYCOMMAND|MF_ENABLED);
		pPopup->EnableMenuItem (ID_NOTIFY,     MF_BYCOMMAND|MF_ENABLED);
		pPopup->EnableMenuItem (ID_DISCARD,    MF_BYCOMMAND|MF_ENABLED);
		pPopup->EnableMenuItem (ID_ADD_FOLDER, MF_BYCOMMAND|MF_ENABLED);
		if (t != CV_R)
			pPopup->EnableMenuItem (ID_DROP_FOLDER,MF_BYCOMMAND|MF_ENABLED);
		break;

	case CV_OF:
		pPopup->EnableMenuItem (ID_ALERT,      MF_BYCOMMAND|MF_ENABLED);
		pPopup->EnableMenuItem (ID_NOTIFY,     MF_BYCOMMAND|MF_ENABLED);
		pPopup->EnableMenuItem (ID_DISCARD,    MF_BYCOMMAND|MF_ENABLED);
		break;
	case CV_UFA:
	case CV_OFA:
	case SV_OFA:
	case SV_FA:
	case SV_UFA:
		pPopup->EnableMenuItem (ID_NOTIFY,     MF_BYCOMMAND|MF_ENABLED);
		pPopup->EnableMenuItem (ID_DISCARD,    MF_BYCOMMAND|MF_ENABLED);
		if (t == CV_UFA)
			pPopup->EnableMenuItem (ID_DROP_FOLDER,MF_BYCOMMAND|MF_ENABLED);
		break;
	case CV_UFN:
	case CV_OFN:
	case SV_OFN:
	case SV_FN:
	case SV_UFN:
		pPopup->EnableMenuItem (ID_ALERT,      MF_BYCOMMAND|MF_ENABLED);
		pPopup->EnableMenuItem (ID_DISCARD,    MF_BYCOMMAND|MF_ENABLED);
		if (t == CV_UFN)
			pPopup->EnableMenuItem (ID_DROP_FOLDER,MF_BYCOMMAND|MF_ENABLED);
		break;
	case CV_UFD:
	case CV_OFD:
	case SV_OFD:
	case SV_FD:
	case SV_UFD:
		pPopup->EnableMenuItem (ID_ALERT,      MF_BYCOMMAND|MF_ENABLED);
		pPopup->EnableMenuItem (ID_NOTIFY,     MF_BYCOMMAND|MF_ENABLED);
		if (t == CV_UFD)
			pPopup->EnableMenuItem (ID_DROP_FOLDER,MF_BYCOMMAND|MF_ENABLED);
		break;

	case CV_U:
	case CV_O:
	case SV_U:
	case SV_O:
		{
			ASSERT (!pItemData->IsInternal());
			if (pItemData->IsInternal())
				break;
			CaMessageNodeTree* pNode = (CaMessageNodeTree*)pItemData;
			CaMessage* pMsg = pNode->GetMessageCategory();
			if (!pMsg)
				break;
			switch (pMsg->GetClass())
			{
			case IMSG_ALERT:
				pPopup->EnableMenuItem (ID_NOTIFY,     MF_BYCOMMAND|MF_ENABLED);
				pPopup->EnableMenuItem (ID_DISCARD,    MF_BYCOMMAND|MF_ENABLED);
				break;
			case IMSG_NOTIFY:
				pPopup->EnableMenuItem (ID_ALERT,      MF_BYCOMMAND|MF_ENABLED);
				pPopup->EnableMenuItem (ID_DISCARD,    MF_BYCOMMAND|MF_ENABLED);
				break;
			case IMSG_DISCARD:
				pPopup->EnableMenuItem (ID_ALERT,      MF_BYCOMMAND|MF_ENABLED);
				pPopup->EnableMenuItem (ID_NOTIFY,     MF_BYCOMMAND|MF_ENABLED);
				break;
			}
			if (t == CV_U || t == SV_U)
				pPopup->EnableMenuItem (ID_DROP_FOLDER,MF_BYCOMMAND|MF_ENABLED);
		}
		break;

	default:
		break;
	}
}

static void UnclassifyMessages (CfEventSetting* pFrame, CaMessageNode* pNode, CaMessageManager* pMessageManager)
{
	if (pNode->IsInternal())
	{
		CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pNode;
		CTypedPtrList<CObList, CaMessageNode*>* pL = pI->GetListSubNode();
		if (!pL)
			return;
	
		CaMessageNode* pDelNode = NULL;
		POSITION p, pos = pL->GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			pDelNode = pL->GetNext (pos);
			UnclassifyMessages (pFrame, pDelNode, pMessageManager);
		}
	}
	else
	{
		CaMessageNodeTree* pMsgNode = (CaMessageNodeTree*)pNode;
		CaMessage* pMsg = pMsgNode->GetMessageCategory();
		int nMsgCount = pMsg? pFrame->GetTreeCategoryView().CountMessage(pMsg): 0;
		if (pMsg && nMsgCount == 1)
		{
			pMessageManager->Remove (pMsg->GetCategory(), pMsg->GetCode());
		}
	}
}

void CuMessageCategoryTreeCtrl::OnDropFolder() 
{
	CWaitCursor doWaitCursor;
	try
	{
		HTREEITEM hSelected  = GetSelectedItem();
		if (!hSelected)
			return;
		CaMessageNode* pNode = (CaMessageNode*)GetItemData(hSelected);
		ASSERT (pNode);
		if (!pNode)
			return;
		if (!m_pEventSettingFrame)
			m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
		ASSERT (m_pEventSettingFrame);
		if (!m_pEventSettingFrame)
			return;
		CaMessageManager* pMessageManager = m_pEventSettingFrame->GetMessageManager();
		ASSERT (pMessageManager);
		if (!pMessageManager)
			return;
		//
		// Check if the Item to drop is from the Left Tree:
		switch (pNode->GetNodeInformation().GetMsgSourceTarget())
		{
		case SV_U:
			{
				CaTreeMessageCategory& catview = m_pEventSettingFrame->GetTreeCategoryView();
				CaMessageNodeTree* pNodeMsg = (CaMessageNodeTree*)pNode;
				CaMessage* pMsg = pNodeMsg->GetMessageCategory();
				if (!pMsg)
					return;
				int nMsgCount = catview.CountMessage(pMsg);

				if (nMsgCount == 0)
					return;
				if (nMsgCount >= 1)
				{
					CvEventSettingLeft* pLeftview = m_pEventSettingFrame->GetLeftView();
					if (!pLeftview)
						return;
					CuDlgEventSettingLeft* pDlg = pLeftview->GetDialog();
					if (!pDlg)
						return;
					int nCheck = pDlg->m_cCheckShowCategories.GetCheck();
					if (nCheck != 1 && nMsgCount > 1)
					{
						// _T("Multiple instances of this message existed.\nCannot delete it if <Show Category> is not checked.");
						CString strMsg;
						strMsg.LoadString (IDS_MSG_CANNOT_DELETE_MULTIPLE_MESSAGES);
						AfxMessageBox (strMsg);
						return;
					}
					else
					{
						//
						// Find the corresponding item (node) in the right tree:
						CaMessageNode* pFound = catview.FindFirstMessage (pMsg);
						ASSERT (pFound);
						if (!pFound)
							return;
						pNode = pFound;
					}
				}
			}
			break;

		case SV_UFA:
		case SV_UFN:
		case SV_UFD:
			AfxMessageBox (_T("TODO: CuMessageCategoryTreeCtrl::OnDropFolder (SV_UFA|SV_UFN|SV_UFD)"));
			return;
		default:
			break;
		}

		BOOL bParent = FALSE;
		switch (pNode->GetNodeInformation().GetMsgSourceTarget())
		{
		case CV_OFA:
		case CV_OFN:
		case CV_OFD:
		case CV_UFA:
		case CV_UFN:
		case CV_UFD:
			bParent = TRUE;
			break;
		default:
			break;
		}
		if (pNode->IsInternal() || bParent)
		{
			// _T("All the messages in the sub-branches will be removed.\nDo you wish to continue ?.");
			CString strMsg;
			strMsg.LoadString (IDS_MSG_REMOVE_BRANCHxSUB);
			int nRes = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
			if (nRes == IDNO)
				return;

			if (bParent)
			{
				//
				// Drop all item under <Alert>, <Notify>, <Discard>:
				HTREEITEM hParent = GetParentItem(pNode->GetNodeInformation().GetHTreeItem());
				CaMessageNode* pParentNode = (CaMessageNode*)GetItemData(hParent);
				ASSERT (pParentNode);
				if (!pParentNode)
					return;
				while (!pParentNode->IsInternal())
				{
					hParent = GetParentItem(pParentNode->GetNodeInformation().GetHTreeItem());
					pParentNode = (CaMessageNode*)GetItemData(hParent);
					ASSERT (pParentNode);
					if (!pParentNode)
						return;
				}
				CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pParentNode;
				CTypedPtrList<CObList, CaMessageNode*>* pL = pI->GetListSubNode();
				ASSERT (pL);
				if (!pL)
					return;
				HTREEITEM hParentItem = NULL;
				CaMessageNode* pDelNode = NULL;
				POSITION p, pos = pL->GetHeadPosition();
				while (pos != NULL)
				{
					p = pos;
					pDelNode = pL->GetNext (pos);
					hParentItem = GetParentItem (pDelNode->GetNodeInformation().GetHTreeItem());

					if (pNode->GetNodeInformation().GetHTreeItem() == hParentItem)
					{
						ASSERT (!pDelNode->IsInternal());
						if (pDelNode->IsInternal())
							continue;
						DeleteItem (pDelNode->GetNodeInformation().GetHTreeItem());

						CaMessageNodeTree* pMsgNode = (CaMessageNodeTree*)pDelNode;
						CaMessage* pMsg = pMsgNode->GetMessageCategory();
						int nMsgCount = pMsg? m_pEventSettingFrame->GetTreeCategoryView().CountMessage(pMsg): 0;
						if (pMsg && nMsgCount == 1)
						{
							pMessageManager->Remove (pMsg->GetCategory(), pMsg->GetCode());
						}

						pL->RemoveAt (p);
						delete pDelNode;
					}
				}
			}
			else
			{
				//
				// Drop a User Folder:
				HTREEITEM hParent = GetParentItem(pNode->GetNodeInformation().GetHTreeItem());
				CaMessageNode* pParentNode = (CaMessageNode*)GetItemData(hParent);
				ASSERT (pParentNode);
				if (!pParentNode)
					return;
				while (!pParentNode->IsInternal())
				{
					hParent = GetParentItem(pParentNode->GetNodeInformation().GetHTreeItem());
					pParentNode = (CaMessageNode*)GetItemData(hParent);
					ASSERT (pParentNode);
					if (!pParentNode)
						return;
				}
				CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pParentNode;
				CTypedPtrList<CObList, CaMessageNode*>* pL = pI->GetListSubNode();
				ASSERT (pL);
				if (!pL)
					return;

				HTREEITEM hParentItem = NULL;
				CaMessageNode* pDelNode = NULL;
				POSITION p, pos = pL->GetHeadPosition();
				while (pos != NULL)
				{
					p = pos;
					pDelNode = pL->GetNext (pos);

					if (pNode->GetNodeInformation().GetHTreeItem() == pDelNode->GetNodeInformation().GetHTreeItem())
					{
						ASSERT (pDelNode->IsInternal());
						if (!pDelNode->IsInternal())
							continue;

						DeleteItem (pDelNode->GetNodeInformation().GetHTreeItem());
						UnclassifyMessages (m_pEventSettingFrame, pNode, pMessageManager);
						
						pL->RemoveAt (p);
						delete pDelNode;
						break;
					}
				}
			}

			//
			// Re-display the tree (left + right):
			GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
			//
			// Re-display the list of message (Actual message + Ingres category):
			CvEventSettingBottom* pBottomView = m_pEventSettingFrame->GetBottomView();
			ASSERT (pBottomView);
			if (pBottomView)
			{
				if (pBottomView->GetDialog())
					(pBottomView->GetDialog())->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)-1, (LPARAM)-1);
			}
			return;
		}

		//
		// A terminal message:
		if (!pNode->IsInternal())
		{
			CaMessageNodeTree* pMsgNode = (CaMessageNodeTree*)pNode;
			CaMessage* pMsg = pMsgNode->GetMessageCategory();
			int nMsgCount = pMsg? m_pEventSettingFrame->GetTreeCategoryView().CountMessage(pMsg): 0;
			if (pMsg && nMsgCount == 1)
			{
				pMessageManager->Remove (pMsg->GetCategory(), pMsg->GetCode());
			}
			
			HTREEITEM hParent = GetParentItem(pNode->GetNodeInformation().GetHTreeItem());
			CaMessageNode* pParentNode = (CaMessageNode*)GetItemData(hParent);
			ASSERT (pParentNode);
			if (!pParentNode)
				return;
			while (!pParentNode->IsInternal())
			{
				hParent = GetParentItem(pParentNode->GetNodeInformation().GetHTreeItem());
				pParentNode = (CaMessageNode*)GetItemData(hParent);
				ASSERT (pParentNode);
				if (!pParentNode)
					return;
			}

			CaMessageNodeTreeInternal* pI = (CaMessageNodeTreeInternal*)pParentNode;
			CTypedPtrList<CObList, CaMessageNode*>* pL = pI->GetListSubNode();
			ASSERT (pL);
			if (!pL)
				return;
			CaMessageNode* pDelNode = NULL;
			POSITION p, pos = pL->GetHeadPosition();
			while (pos != NULL)
			{
				p = pos;
				pDelNode = pL->GetNext (pos);
				if (pNode->GetNodeInformation().GetHTreeItem() == pDelNode->GetNodeInformation().GetHTreeItem())
				{
					DeleteItem (pNode->GetNodeInformation().GetHTreeItem());
					pL->RemoveAt (p);
					delete pDelNode;
					//
					// Re-display the tree (left + right):
					GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
					//
					// Re-display the list of message (Actual message + Ingres category):
					CvEventSettingBottom* pBottomView = m_pEventSettingFrame->GetBottomView();
					ASSERT (pBottomView);
					if (pBottomView)
					{
						if (pBottomView->GetDialog())
							(pBottomView->GetDialog())->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)-1, (LPARAM)-1);
					}
					break;
				}
			}
		}
	}
	catch (...)
	{
		AfxMessageBox (_T("System error (CuMessageCategoryTreeCtrl::OnDropFolder): Drop item failed"));
	}
}
