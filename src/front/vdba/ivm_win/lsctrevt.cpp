/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : lsctrevt.cpp , Implementation File 
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : List Control for Message setting (bottom pane)
**
** History:
**
** 21-June-1999 (uk$so01)
**    Created
** 20-Jan-2000 (uk$so01)
**    Bug #100063, Eliminate the undesired compiler's warning
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 12-Fev-2001 (noifr01)
**    bug 102974 (manage multiline errlog.log messages)
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings.
** 07-Jun-2002 (uk$so01)
**    SIR #107951, Display date format according to the LOCALE.
**/



#include "stdafx.h"
#include "rcdepend.h"
#include "fevsetti.h"
#include "lsctrevt.h"
#include "ivmdml.h"
#include "ivmsgdml.h"
#include "wmusrmsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int CALLBACK CompareSubItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int nC;
	BOOL bAsc = TRUE;
	CaLoggedEvent* lpItem1 = (CaLoggedEvent*)lParam1;
	CaLoggedEvent* lpItem2 = (CaLoggedEvent*)lParam2;


	if (lpItem1 && lpItem2)
	{
		nC = bAsc? 1 : -1;
		return nC * lpItem1->m_strText.CompareNoCase (lpItem2->m_strText);
	}
	return 0;
}

static UINT GetCompatibleSourceTarget(CTreeCtrl* pTreeTarget, CaMessageNodeInfo& target)
{
	UINT nAction = 0;
	MSGSourceTarget t = target.GetMsgSourceTarget();

	switch (t)
	{
	case SV_R:
	case CV_O:
	case CV_U:
	case CV_OF:
	case CV_OFA:
	case CV_OFN:
	case CV_OFD:

	case SV_O:
	case SV_U:
	case SV_OFA:
	case SV_OFN:
	case SV_OFD:
		break;

	case CV_R:
	case CV_UF:
		nAction |= DD_ACTION_MOVE;
		break;
	case CV_UFA:
	case SV_UFA:
	case SV_FA:
		nAction |= DD_ACTION_ALERT;
		break;
	case CV_UFN:
	case SV_UFN:
	case SV_FN:
		nAction |= DD_ACTION_NOTIFY;
		break;
	case CV_UFD:
	case SV_UFD:
	case SV_FD:
		nAction |= DD_ACTION_DISCARD;
		break;

	default:
		nAction = 0;
	}
	return nAction;
}


/////////////////////////////////////////////////////////////////////////////
// CuListCtrlLoggedEvent

CuListCtrlLoggedEvent::CuListCtrlLoggedEvent()
{
	m_bIngresGenericMessage = FALSE;
	m_bDragging = FALSE;
	m_pImageListDrag = NULL;
	m_iItemDrag = -1;
	m_pEventSettingFrame = NULL;
}

CuListCtrlLoggedEvent::~CuListCtrlLoggedEvent()
{
}


BEGIN_MESSAGE_MAP(CuListCtrlLoggedEvent, CuListCtrlVScrollEvent)
	//{{AFX_MSG_MAP(CuListCtrlLoggedEvent)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBegindrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_NOTIFY_REFLECT(LVN_BEGINRDRAG, OnBegindrag)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuListCtrlLoggedEvent message handlers

void CuListCtrlLoggedEvent::OnBegindrag(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	CPoint p, ptItem, ptAction, ptImage;

	UINT nFlags;

	GetCursorPos(&ptAction);
	p = ptAction;
	ScreenToClient(&p);
	m_iItemDrag = HitTest(p, &nFlags);
	if (!(m_iItemDrag != -1 || (nFlags&TVHT_ONITEM)))
	{
		m_iItemDrag = -1;
		m_bDragging = FALSE;
		return;
	}
	if (!m_pEventSettingFrame)
		m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
	CvEventSettingBottom* pBottomView= m_pEventSettingFrame->GetBottomView();
	CListCtrl* pListCtrl = pBottomView->GetListCtrl();; 
	CaLoggedEvent* pMsg = (CaLoggedEvent*)pListCtrl->GetItemData (m_iItemDrag);
	if (pMsg->IsNotFirstLine()) {
		m_iItemDrag = -1;
		m_bDragging = FALSE;
		return;
	}



	ASSERT(!m_bDragging);
	m_bDragging = TRUE;

	ASSERT(m_pImageListDrag == NULL);
	m_pImageListDrag = CreateDragImage(m_iItemDrag, &ptImage);
	GetItemPosition(m_iItemDrag, &ptItem);         // ptItem is relative to (0,0) and not the view origin
	m_sizeDelta = ptAction - ptImage;              // difference between cursor pos and image pos
	m_ptHotSpot = ptAction - ptItem + m_ptOrigin;  // calculate hotspot for the cursor
	m_pImageListDrag->DragShowNolock(TRUE);        // lock updates and show drag image
	m_pImageListDrag->SetDragCursorImage(0, CPoint(0, 0) /*m_ptHotSpot*/);  // define the hot spot for the new cursor image
	m_pImageListDrag->BeginDrag(0, CPoint(0, 0));

	m_pImageListDrag->DragEnter(/*this*/GetParentFrame(), ptAction);
	m_pImageListDrag->DragMove(ptAction);  // move image to overlap original icon
	SetCapture();
}

void CuListCtrlLoggedEvent::OnMouseMove(UINT nFlags, CPoint point) 
{
	CPoint p = point;
	ClientToScreen (&p);
	CPoint pDragMove = p;

	if (!m_pEventSettingFrame)
		m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
	ASSERT (m_pEventSettingFrame);
	if (!m_pEventSettingFrame)
	{
		CuListCtrlVScrollEvent::OnMouseMove(nFlags, point);
		return;
	}
	m_pEventSettingFrame->ScreenToClient(&pDragMove);

	if (!m_bDragging)
	{
		CuListCtrlVScrollEvent::OnMouseMove(nFlags, point);
		return;
	}
	
	ASSERT(m_pImageListDrag != NULL);
	m_pImageListDrag->DragMove(pDragMove);  // move the image
	m_pImageListDrag->DragLeave(/*this*/GetParentFrame());

	//
	// Determine the pane under the mouse pointer:
	CRect rFrame, rLeftTree, rRightTree, rBottomList;
	m_pEventSettingFrame->GetWindowRect (rFrame);

	CvEventSettingLeft*   pLeftView  = m_pEventSettingFrame->GetLeftView();
	CvEventSettingRight*  pRightView = m_pEventSettingFrame->GetRightView();
	CvEventSettingBottom* pBottomView= m_pEventSettingFrame->GetBottomView();
	if (!(pLeftView && pRightView && pBottomView))
	{
		CuListCtrlVScrollEvent::OnMouseMove(nFlags, point);
		return;
	}
	CTreeCtrl* pTreeLeft  = pLeftView->GetTreeCtrl();
	CTreeCtrl* pTreeRight = pRightView->GetTreeCtrl();
	CListCtrl* pListBottom= pBottomView->GetListCtrl();
	if (!(pTreeLeft && pTreeRight && pListBottom))
	{
		CuListCtrlVScrollEvent::OnMouseMove(nFlags, point);
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

	if (nPane == 1 || nPane == 2)
	{
		CTreeCtrl* pTree = NULL;
		if (nPane == 1)
			pTree = pTreeLeft;
		else
			pTree = pTreeRight;

		UINT flags, nAction = 0;
		HTREEITEM hTreeItemTarget = NULL;
		CaMessageNode* pNodeTarget = NULL;
		CPoint pt = point;
		pt.y += 8;
		ClientToScreen (&pt);

		pTree->ScreenToClient(&pt);
		hTreeItemTarget = pTree->HitTest (pt, &flags);
		if (hTreeItemTarget && (flags&TVHT_ONITEM))
		{
			pTree->SelectItem (hTreeItemTarget);
			pNodeTarget = (CaMessageNode*)pTree->GetItemData (hTreeItemTarget);
			nAction = GetCompatibleSourceTarget(pTree,pNodeTarget->GetNodeInformation());
		}
		TRACE1 ("DD Action = %x\n", nAction);

		if (nAction != 0)
			::SetCursor (theApp.m_hCursorDropCategory);
		else
			::SetCursor (theApp.m_hCursorNoDropCategory);
	}
	else
	{
		::SetCursor (theApp.m_hCursorNoDropCategory);
	}
	m_pImageListDrag->DragEnter(GetParentFrame(), pDragMove);  // lock updates and show drag image

	CuListCtrlVScrollEvent::OnMouseMove(nFlags, point);
}

void CuListCtrlLoggedEvent::OnButtonUp(CPoint point)
{
	try
	{
		UINT flags = 0;
		UINT nFlags = 0;
		CPoint p = point;
		ClientToScreen (&p);

		if (!m_bDragging)
			return;
		m_bDragging = FALSE;

		ASSERT(m_pImageListDrag != NULL);
		m_pImageListDrag->DragLeave(GetParentFrame());
		m_pImageListDrag->EndDrag();
		delete m_pImageListDrag;
		m_pImageListDrag = NULL;

		if (m_iItemDrag == -1)
			return;

		//
		// Determine the pane under the mouse pointer:
		CRect rFrame, rLeftTree, rRightTree, rBottomList;
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

		if (nPane == 1 || nPane == 2)
		{
			CTreeCtrl* pTree = NULL;
			CListCtrl* pListCtrl = pListBottom; 

			if (nPane == 1)
				pTree = pTreeLeft;
			else
				pTree = pTreeRight;

			UINT flags, nAction = 0;
			HTREEITEM hTreeItemTarget = NULL;
			CaMessageNode* pNodeTarget = NULL;
			CPoint pt = point;
			pt.y += 8;
			ClientToScreen (&pt);

			pTree->ScreenToClient(&pt);
			hTreeItemTarget = pTree->HitTest (pt, &flags);
			if (hTreeItemTarget && (flags&TVHT_ONITEM))
			{
				pTree->SelectItem (hTreeItemTarget);
				pNodeTarget = (CaMessageNode*)pTree->GetItemData (hTreeItemTarget);

				nAction = GetCompatibleSourceTarget(pTree, pNodeTarget->GetNodeInformation());
				NewUserMessage (nPane, nAction, pTree, pNodeTarget, pListCtrl);
			}
		}
	}
	catch (...)
	{
		TRACE0 ("Internal error(1): CuListCtrlLoggedEvent::OnButtonUp \n");
	}
	::ReleaseCapture();
}


void CuListCtrlLoggedEvent::OnLButtonUp(UINT nFlags, CPoint point) 
{
	OnButtonUp (point);
	CuListCtrlVScrollEvent::OnLButtonUp(nFlags, point);
}

void CuListCtrlLoggedEvent::OnRButtonUp(UINT nFlags, CPoint point) 
{
	OnButtonUp (point);
	CuListCtrlVScrollEvent::OnRButtonUp(nFlags, point);
}


void CuListCtrlLoggedEvent::NewUserMessage (
	int nPane, 
	UINT nAction, 
	CTreeCtrl* pTree, 
	CaMessageNode* pNodeTarget, 
	CListCtrl* pListCtrl)
{
	if (!m_pEventSettingFrame)
		m_pEventSettingFrame = (CfEventSetting*)GetParentFrame();
	if (!m_pEventSettingFrame)
		return;

	CaMessageNodeInfo& nodeInfo = pNodeTarget->GetNodeInformation();
	MSGSourceTarget t = nodeInfo.GetMsgSourceTarget();

	CaMessage* pNewMessage = NULL;
	CaMessageNodeTree* pNewMessageNodeTree = NULL;
	CaLoggedEvent* pMsg = (CaLoggedEvent*)pListCtrl->GetItemData (m_iItemDrag);

	if (nAction & DD_ACTION_MOVE)
	{
		//
		// Classify the new message and retain its original class:
		if (NewMessageRetainState (pNodeTarget, pMsg))
		{
			pTree->GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
			GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)pMsg->GetCatType(), (LPARAM)pMsg->GetCode());
		}
		return;
	}

	if ((nAction & DD_ACTION_ALERT) || (nAction & DD_ACTION_NOTIFY) || (nAction & DD_ACTION_DISCARD))
		NewMessageNewState (pNodeTarget, pMsg, pTree);
}

CaMessageNodeTree* CuListCtrlLoggedEvent::CreateMessageNode(CaMessageManager* pMsgEntry, CaLoggedEvent* pMsg)
{
	CaMessage* pMsgExist = NULL;
	CaMessageEntry* pEntry = pMsgEntry->FindEntry (pMsg->GetCatType());
	ASSERT (pEntry);
	if (!pEntry)
		return NULL;
	pMsgExist = IVM_LookupMessage (pMsg->GetCatType(), pMsg->GetCode(), pMsgEntry);
	if (pMsgExist)
	{
		//_T("This message is already classified.");
		CString strMsg;
		strMsg.LoadString (IDS_MSG_MESSAGE_ALREADY_CLASSIFIED);
		AfxMessageBox (strMsg);
		return NULL;
	}

	CaMessageNodeTree* pNewMessageNodeTree = NULL;
	Imsgclass c = pMsg->GetClass();
	CString strMsgText = IVM_IngresGenericMessage(pMsg->GetCode(), pMsg->m_strText);
	CaMessage* pNewMessage = new CaMessage (strMsgText, pMsg->GetCode(), pMsg->GetCatType(), c);
	pNewMessage = pMsgEntry->Add (pNewMessage->GetCategory(), pNewMessage);
	
	pNewMessageNodeTree = new CaMessageNodeTree (pNewMessage);
	CString strMsgTitle = pNewMessage->GetTitle();
	if (m_bIngresGenericMessage)
		strMsgTitle = IVM_IngresGenericMessage(pNewMessage->GetCode(), pNewMessage->GetTitle());
	pNewMessageNodeTree->GetNodeInformation().SetNodeTitle(strMsgTitle);
	pNewMessageNodeTree->GetNodeInformation().SetMsgSourceTarget(CV_U);

	return pNewMessageNodeTree;
}


BOOL CuListCtrlLoggedEvent::NewMessageRetainState (CaMessageNode* pNodeTarget, CaLoggedEvent* pMsg)
{
	CaMessageManager* pMsgEntry = m_pEventSettingFrame->GetMessageManager();
	CaMessageNodeTree* pNewMessageNodeTree = NULL;
	CaMessageNodeInfo& nodeInfo = pNodeTarget->GetNodeInformation();
	MSGSourceTarget t = nodeInfo.GetMsgSourceTarget();

	switch (t)
	{
	case CV_R:
	case CV_UF:
		pNewMessageNodeTree = CreateMessageNode(pMsgEntry, pMsg);
		if (!pNewMessageNodeTree)
			return FALSE;
		pNodeTarget->AddMessage (pNewMessageNodeTree);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

void CuListCtrlLoggedEvent::NewMessageNewState (CaMessageNode* pNodeTarget, CaLoggedEvent* pMsg, CTreeCtrl* pTree)
{
	CaMessageManager* pMsgEntry = m_pEventSettingFrame->GetMessageManager();
	CaMessageNodeTree* pNewMessageNodeTree = NULL;
	CaMessageNodeInfo& nodeInfo = pNodeTarget->GetNodeInformation();
	MSGSourceTarget t = nodeInfo.GetMsgSourceTarget();

	HTREEITEM hParent = pTree->GetParentItem(nodeInfo.GetHTreeItem());
	ASSERT (hParent);
	if (!hParent)
		return;
	CaMessageNode* pNodeParent = (CaMessageNode*)pTree->GetItemData(hParent);
	ASSERT (pNodeParent && pNodeParent->IsInternal());
	if (!pNodeParent || (pNodeParent && !pNodeParent->IsInternal()))
		return;

	CaMessageNodeTreeInternal* pI = NULL;
	int nIdx = -1;
	CaMessageNode* pObj = NULL;
	POSITION pos = NULL;
	CString strItem;
	CString strPath = _T("");
	CString strTemp = _T("");
	CaTreeMessageCategory& treeCategoryView = m_pEventSettingFrame->GetTreeCategoryView();
	CTypedPtrList<CObList, CaMessageNode*>* pLs = treeCategoryView.GetListSubNode();
	
	CaMessage* pMessage = NULL;
	Imsgclass nClass = IMSG_NOTIFY;
	switch (t)
	{
	case SV_FA:
	case SV_UFA:
	case CV_UFA:
		nClass = IMSG_ALERT;
		break;
	case SV_FN:
	case SV_UFN:
	case CV_UFN:
		nClass = IMSG_NOTIFY;
		break;
	case SV_FD:
	case SV_UFD:
	case CV_UFD:
		nClass = IMSG_DISCARD;
		break;
	}

	switch (t)
	{
	case CV_UFA:
	case CV_UFN:
	case CV_UFD:
		if (!pNodeTarget->GetNodeInformation().GetHTreeItem())
			break;
		hParent = pTree->GetParentItem(pNodeTarget->GetNodeInformation().GetHTreeItem());
		pNodeParent = (CaMessageNode*)pTree->GetItemData(hParent);
		ASSERT (pNodeParent && pNodeParent->IsInternal());
		if (!(pNodeParent && pNodeParent->IsInternal()))
			break;
		pI = (CaMessageNodeTreeInternal*)pNodeParent;
		pNewMessageNodeTree = CreateMessageNode(pMsgEntry, pMsg);
		if (!pNewMessageNodeTree)
			break;
		pMessage = pNewMessageNodeTree->GetMessageCategory();
		if (pMessage)
			pMessage->SetClass (nClass);
		
		pI->AddMessage (pNewMessageNodeTree);
		pTree->GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
		GetParent()->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, (WPARAM)pMsg->GetCatType(), (LPARAM)pMsg->GetCode());
		break;

	case SV_FA:   // Ex: <Alert>
	case SV_UFA:  // Ex: <Alert>\Folder1\Folder2
	case SV_FN: 
	case SV_UFN:
	case SV_FD: 
	case SV_UFD:
		//
		// Find the equivalent path in the right tree:
		switch (pNodeTarget->GetNodeInformation().GetMsgSourceTarget() )
		{
		case SV_UFA:
		case SV_UFN:
		case SV_UFD:
			strPath = pNodeTarget->GetNodeInformation().GetNodeTitle();
			break;
		}

		while (pNodeParent->GetNodeInformation().GetMsgSourceTarget() != SV_R)
		{
			switch (pNodeParent->GetNodeInformation().GetMsgSourceTarget())
			{
			case SV_UFA:
			case SV_UFN:
			case SV_UFD:
				strTemp = pNodeParent->GetNodeInformation().GetNodeTitle();
				if (!strPath.IsEmpty())
					strTemp += _T("/");
				strTemp += strPath;
				strPath = strTemp;
			}
			hParent = pTree->GetParentItem(pNodeParent->GetNodeInformation().GetHTreeItem());
			pNodeParent = (CaMessageNode*)pTree->GetItemData(hParent);
		}

		if (!pLs)
			break;

		strItem = strPath;
		nIdx = strPath.Find (_T('/'));
		if (nIdx != -1)
		{
			strItem = strPath.Left(nIdx);
			strPath = strPath.Mid (nIdx +1);
		}
		
		if (strItem.IsEmpty())
		{
			pI = (CaMessageNodeTreeInternal*)&treeCategoryView;
			pNewMessageNodeTree = CreateMessageNode(pMsgEntry, pMsg);
			if (!pNewMessageNodeTree)
				break;
			pMessage = pNewMessageNodeTree->GetMessageCategory();
			if (pMessage)
				pMessage->SetClass (nClass);
			pI->AddMessage (pNewMessageNodeTree);

			CvEventSettingRight* pRview = m_pEventSettingFrame->GetRightView();
			(pRview->GetDialog())->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
			GetParent()->SendMessage (
				WMUSRMSG_IVM_PAGE_UPDATING, 
				(WPARAM)pMsg->GetCatType(), 
				(LPARAM)pMsg->GetCode());
			break;
		}

		pos = pLs->GetHeadPosition();
		while (pos != NULL)
		{
			pObj = pLs->GetNext (pos);
			if (pObj->GetNodeInformation().GetNodeTitle() != strItem)
				continue;
			
			ASSERT (pObj->IsInternal());
			pI = (CaMessageNodeTreeInternal*)pObj;
			if (nIdx != -1)
			{
				strItem = strPath;
				nIdx = strPath.Find (_T('/'));
				if (nIdx != -1)
				{
					strItem = strPath.Left(nIdx);
					strPath = strPath.Mid (nIdx +1);
				}
				pLs = pI->GetListSubNode();
				pos = pLs->GetHeadPosition();
			}
			else
			{
				pNewMessageNodeTree = CreateMessageNode(pMsgEntry, pMsg);
				if (!pNewMessageNodeTree)
					break;
				pMessage = pNewMessageNodeTree->GetMessageCategory();
				if (pMessage)
					pMessage->SetClass (nClass);
				pI->AddMessage (pNewMessageNodeTree);

				CvEventSettingRight* pRview = m_pEventSettingFrame->GetRightView();
				(pRview->GetDialog())->SendMessage (WMUSRMSG_IVM_PAGE_UPDATING, 0, 0);
				GetParent()->SendMessage (
					WMUSRMSG_IVM_PAGE_UPDATING, 
					(WPARAM)pMsg->GetCatType(), 
					(LPARAM)pMsg->GetCode());
			}
		}
		break;
	default:
		break;
	}
}



//
// Specific for the list of events (full description)
void CuListCtrlLoggedEvent::AddEvent (CaLoggedEvent* pEvent, int nIndex)
{
	int  nImage = 0;

	BOOL bClassified = pEvent->IsClassified();

	int nCount = (nIndex == -1)? GetItemCount(): nIndex;
	Imsgclass msgClass = pEvent->GetClass();

	switch (msgClass)
	{
	case IMSG_ALERT:
		nImage  = bClassified? IM_ALERT: IM_ALERT_U;
		break;
	case IMSG_NOTIFY:
		nImage  = bClassified? IM_NOTIFY: IM_NOTIFY_U;
		break;
	case IMSG_DISCARD:
		nImage  = bClassified? IM_DISCARD: IM_DISCARD_U;
		break;
	}
	
	nIndex = InsertItem  (nCount, pEvent->m_strText, nImage);
	if (nIndex != -1)
	{
		SetItemText (nIndex, 1, pEvent->m_strComponentType);
		SetItemText (nIndex, 2, pEvent->m_strComponentName);
		SetItemText (nIndex, 3, pEvent->m_strInstance);
		SetItemText (nIndex, 4, pEvent->GetDateLocale());
		SetItemData (nIndex, (DWORD_PTR)pEvent);
	}
}


