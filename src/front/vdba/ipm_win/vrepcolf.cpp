/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vrepcolf.cpp, Implementation file
**    Project  : INGRESII / Monitoring.
**    Author   : UK Sotheavut
**    Purpose  : Tree View to display the transaction having collision
**
** History:
**
** xx-May-1998 (uk$so01)
**    Created
** 15-Sep-1999 (schph01)
**    bug #98654
**    Add function GetDbaOwner() and format the vnode name with this user
** 24-Sep-1999 (schph01)
**    SIR #98839
**    Use the method CaCollisionTransaction::SequenceWithSameNumber() to
**    differentiate the way in which will be displayed the sequence number.
**    only one number, display               :  sequence #Nb
**    several with the same number, display  :  sequence #Nb [->vnode::target_database]
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "repcodoc.h"
#include "vrepcolf.h"
#include "vrepcort.h"
#include "frepcoll.h"
#include "ipmframe.h"
#include "ipmdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CvReplicationPageCollisionViewLeft

IMPLEMENT_DYNCREATE(CvReplicationPageCollisionViewLeft, CTreeView)

CvReplicationPageCollisionViewLeft::CvReplicationPageCollisionViewLeft()
{
}

CvReplicationPageCollisionViewLeft::~CvReplicationPageCollisionViewLeft()
{
}


BEGIN_MESSAGE_MAP(CvReplicationPageCollisionViewLeft, CTreeView)
	//{{AFX_MSG_MAP(CvReplicationPageCollisionViewLeft)
	ON_WM_CREATE()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING, OnUpdateData)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvReplicationPageCollisionViewLeft drawing

void CvReplicationPageCollisionViewLeft::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CvReplicationPageCollisionViewLeft diagnostics

#ifdef _DEBUG
void CvReplicationPageCollisionViewLeft::AssertValid() const
{
	CTreeView::AssertValid();
}

void CvReplicationPageCollisionViewLeft::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvReplicationPageCollisionViewLeft message handlers



BOOL CvReplicationPageCollisionViewLeft::PreCreateWindow(CREATESTRUCT& cs) 
{
	cs.style |= TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS;
	return CTreeView::PreCreateWindow(cs);
}

int CvReplicationPageCollisionViewLeft::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CTreeView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	CTreeCtrl& treectrl = GetTreeCtrl();
	m_ImageList.Create (IDB_COLLISION_IMAGE, 16, 0, RGB(255,0,255));
	treectrl.SetImageList (&m_ImageList, TVSIL_NORMAL);

	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	if (hFont == NULL)
		hFont = (HFONT)GetStockObject(ANSI_VAR_FONT);
	ASSERT (hFont);
	treectrl.SendMessage(WM_SETFONT, (WPARAM)hFont);

	return 0;
}

void CvReplicationPageCollisionViewLeft::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	CaCollisionItem* pItem   = NULL;
	CTreeCtrl& treeCtrl = GetTreeCtrl();
	CString strItemText = treeCtrl.GetItemText(pNMTreeView->itemNew.hItem);
	CfReplicationPageCollision* pFrame = (CfReplicationPageCollision*)GetParentFrame();
	ASSERT (pFrame);
	if (pFrame)
	{
		if (pNMTreeView->itemOld.hItem != NULL)
		{
			pItem = (CaCollisionItem*)treeCtrl.GetItemData (pNMTreeView->itemOld.hItem);
			pItem->SetSelected (FALSE);
		}
		CvReplicationPageCollisionViewRight* pViewR = (CvReplicationPageCollisionViewRight*)pFrame->GetRightPane();
		ASSERT (pViewR);
		if (pViewR)
		{
			pItem = (CaCollisionItem*)treeCtrl.GetItemData (pNMTreeView->itemNew.hItem);
			pItem->SetSelected (TRUE);
			pViewR->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)pNMTreeView->itemNew.hItem, (LPARAM)pItem);
		}
	}
	
	*pResult = 0;
}

LONG CvReplicationPageCollisionViewLeft::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	CTreeCtrl& treectrl = GetTreeCtrl();
	treectrl.DeleteAllItems();

	CdReplicationPageCollisionDoc* pDoc = (CdReplicationPageCollisionDoc*)GetDocument();
	ASSERT (pDoc);
	if (pDoc)
	{
		CaCollision* pData = pDoc->GetCollisionData();
		ASSERT (pData);
		UpdateDisplay (pData);
	}
	return 0;
}


void CvReplicationPageCollisionViewLeft::UpdateDisplay (CaCollision* pData)
{
	if (!pData)
		return;
	
	int nImage;
	CString strItem;
	HTREEITEM hRootTransaction = NULL;
	HTREEITEM hRootSequence = NULL;
	HTREEITEM hSelectedItem = NULL;

	CTreeCtrl& treectrl = GetTreeCtrl();
	CaCollisionTransaction* pTrans = NULL;
	CaCollisionSequence*    pSequence = NULL;
	CTypedPtrList<CObList, CaCollisionTransaction*>& transList = pData->GetListTransaction();
	POSITION p = NULL, pos = transList.GetHeadPosition();
	while (pos != NULL)
	{
		pTrans = transList.GetNext(pos);
		CTypedPtrList<CObList, CaCollisionSequence*>& listCollisionSequence = pTrans->GetListCollisionSequence();
		p = listCollisionSequence.GetHeadPosition();
		ASSERT (p);		// need at least one sequence
		pSequence = listCollisionSequence.GetNext (p);
		CString csVerif = pSequence->m_strSourceDatabase;		// for further check on Source Db Name

		strItem.Format (
			_T("[%s::%s] %d"),
			(LPCTSTR)pSequence->m_strSourceVNode,
			(LPCTSTR)csVerif, 
			pTrans->GetID());

		nImage = pTrans->GetImageType();
		hRootTransaction = treectrl.InsertItem (strItem, nImage, nImage, TVI_ROOT, TVI_LAST);
		treectrl.SetItemData (hRootTransaction, (DWORD)pTrans);
		if (pTrans->IsSelected())
			hSelectedItem = hRootTransaction;

		p = listCollisionSequence.GetHeadPosition();
		while (p != NULL)
		{
			pSequence = listCollisionSequence.GetNext (p);
			ASSERT (pSequence->m_strSourceDatabase == csVerif);  // check Source Db Name is the same for all sequences
			if (pTrans->SequenceWithSameNumber(pSequence->GetID()))
				strItem.Format (
				_T("Sequence #%d [->%s::%s]"), 
				pSequence->GetID(),
				(LPCTSTR)pSequence->m_strTargetVNode,
				(LPCTSTR)pSequence->m_strTargetDatabase);
			else
				strItem.Format (_T("Sequence #%d"), pSequence->GetID());
			nImage = pSequence->GetImageType();
			hRootSequence = treectrl.InsertItem (strItem, nImage, nImage, hRootTransaction, TVI_LAST);
			treectrl.SetItemData (hRootSequence, (DWORD)pSequence);
			if (pSequence->IsSelected())
				hSelectedItem = hRootSequence;
		}
		if (pTrans->IsExpanded())
			treectrl.Expand (hRootTransaction, TVE_EXPAND);
	}
	if (hSelectedItem != NULL)
		treectrl.SelectItem (hSelectedItem);
	if (transList.IsEmpty())
	{
		CfReplicationPageCollision* pFrame = (CfReplicationPageCollision*)GetParentFrame();
		ASSERT (pFrame);
		if (pFrame)
		{
			CvReplicationPageCollisionViewRight* pViewR = (CvReplicationPageCollisionViewRight*)pFrame->GetRightPane();
			ASSERT (pViewR);
			if (pViewR)
				pViewR->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)0, (LPARAM)0);
		}
	}
}


void CvReplicationPageCollisionViewLeft::CurrentSelectedUpdateImage()
{
	CTreeCtrl& treectrl = GetTreeCtrl();
	HTREEITEM hCurrentSelected = treectrl.GetSelectedItem();
	if (!hCurrentSelected)
		return;
	CaCollisionItem* pItem = (CaCollisionItem*)treectrl.GetItemData(hCurrentSelected);
	if (pItem)
		treectrl.SetItemImage (hCurrentSelected, pItem->GetImageType(), pItem->GetImageType());
}

void CvReplicationPageCollisionViewLeft::ResolveCurrentTransaction()
{
	CTreeCtrl& treectrl = GetTreeCtrl();
	HTREEITEM hCurrentSelected = treectrl.GetSelectedItem();
	if (!hCurrentSelected)
		return;
	HTREEITEM hSaveSelected = hCurrentSelected;
	CaCollisionItem* pItem = (CaCollisionItem*)treectrl.GetItemData(hCurrentSelected);
	if (!pItem)
		return;
	if (!pItem->IsTransaction())
	{
		hCurrentSelected = treectrl.GetParentItem(hCurrentSelected);
		pItem = (CaCollisionItem*)treectrl.GetItemData(hCurrentSelected);
	}

	if (pItem && pItem->IsTransaction())
	{
		CFrameWnd* pParent = (CFrameWnd*)GetParentFrame();
		CfIpmFrame* pFrame = (CfIpmFrame*)pParent->GetParentFrame();
		ASSERT(pFrame);
		if (!pFrame)
			return;
		CdIpmDoc* pDoc = pFrame->GetIpmDoc();

		CaCollisionTransaction* pTrans = (CaCollisionTransaction*)pItem;
		IPM_ResolveCurrentTransaction(pDoc, pItem);
		treectrl.SetItemImage (hCurrentSelected, pTrans->GetImageType(), pTrans->GetImageType());
	}


	CfReplicationPageCollision* pFrame = (CfReplicationPageCollision*)GetParentFrame();
	ASSERT (pFrame);
	if (pFrame)
	{
		CvReplicationPageCollisionViewRight* pViewR = (CvReplicationPageCollisionViewRight*)pFrame->GetRightPane();
		ASSERT (pViewR);
		if (pViewR)
		{
			CaCollisionItem* pItem = (CaCollisionItem*)treectrl.GetItemData (hSaveSelected);
			pViewR->SendMessage (WM_USER_IPMPAGE_UPDATEING, (WPARAM)hSaveSelected, (LPARAM)pItem);
		}
	}
}

BOOL CvReplicationPageCollisionViewLeft::CheckResolveCurrentTransaction()
{
	CTreeCtrl& treectrl = GetTreeCtrl();
	HTREEITEM hCurrentSelected = treectrl.GetSelectedItem();
	if (!hCurrentSelected)
		return FALSE;
	HTREEITEM hSaveSelected = hCurrentSelected;
	CaCollisionItem* pItem = (CaCollisionItem*)treectrl.GetItemData(hCurrentSelected);
	if (!pItem)
		return FALSE;
	if (!pItem->IsTransaction())
	{
		hCurrentSelected = treectrl.GetParentItem(hCurrentSelected);
		pItem = (CaCollisionItem*)treectrl.GetItemData(hCurrentSelected);
	}

	if (pItem && pItem->IsTransaction())
	{
		CaCollisionTransaction* pTrans = (CaCollisionTransaction*)pItem;
		CTypedPtrList<CObList, CaCollisionSequence*>& listSeq = pTrans->GetListCollisionSequence();
		POSITION pos = listSeq.GetHeadPosition();
		while (pos != NULL)
		{
			CaCollisionSequence* pSeq = listSeq.GetNext (pos);
			switch (pSeq->GetImageType())
			{
			case CaCollisionItem::PREVAIL_SOURCE:
			case CaCollisionItem::PREVAIL_TARGET:
				break;
			default:
				return FALSE;
			}
		}
		return TRUE;
	}
	return FALSE;
}



void CvReplicationPageCollisionViewLeft::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	CaCollisionItem* pItem = NULL;
	CTreeCtrl& treectrl = GetTreeCtrl();
	if (pNMTreeView->action & TVE_EXPAND)
	{
		pItem = (CaCollisionItem*)treectrl.GetItemData (pNMTreeView->itemNew.hItem);
		if (pItem)
			pItem->SetExpanded (TRUE);
	}
	else
	if (pNMTreeView->action & TVE_COLLAPSE)
	{
		pItem = (CaCollisionItem*)treectrl.GetItemData (pNMTreeView->itemNew.hItem);
		if (pItem)
			pItem->SetExpanded (FALSE);
	}
	*pResult = 0;
}

