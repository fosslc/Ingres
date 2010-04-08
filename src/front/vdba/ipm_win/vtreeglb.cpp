/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vtreeglb.cpp, implementation file
**    Project  : CA-OpenIngres/Monitoring
**    Author   : Emmanuel Blattes
**    Purpose  : Tree Item and Folder
**
** History:
**
** 11-Feb-1997 (Emmanuel Blattes)
**    Created
** 22-May-2003 (uk$so01)
**    SIR #106648
**    Change the serialize machanism. Do not serialize the HTREEITEM
**    because the stored HTREEITEM may different of when the tree is
**    reconstructed from the serialize.
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "vtreeglb.h"
#include "montree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef MAINWIN
#define BUGGY_IMAGELIST
#endif


IMPLEMENT_SERIAL (CaIpmTreeData, CObject, 1)
CaIpmTreeData::CaIpmTreeData()
{
	m_nuState = 0;
	m_strItemText = _T("");
	m_nImage = -1;
	m_nSelectedImage = -1;
	m_pItem = NULL;
	m_bFirtVisible = FALSE;
}

CaIpmTreeData::CaIpmTreeData(CTreeCtrl* pTree, HTREEITEM hItem)
{
	ASSERT(pTree);
	m_nuState = 0;
	m_strItemText = _T("");
	m_nImage = -1;
	m_nSelectedImage = -1;
	m_pItem = NULL;
	m_bFirtVisible = FALSE;

	if (pTree)
	{
		TCHAR tchszItemBuffer[256 +1];
		HTREEITEM h = hItem? hItem: pTree->GetRootItem();
		TVITEM tvi;
		memset (&tvi, 0, sizeof(tvi));
		tvi.hItem = h;
		tvi.mask  = TVIF_PARAM | TVIF_STATE | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.pszText = tchszItemBuffer;
		tvi.cchTextMax = 256;
		if (pTree->GetItem(&tvi))
		{
			m_nuState = tvi.state;
			m_strItemText =  tvi.pszText;
			m_nImage = tvi.iImage;
			m_nSelectedImage = tvi.iSelectedImage;
			m_pItem = (CTreeItem*)tvi.lParam;
		}

		HTREEITEM hFirstVisibleItem = pTree->GetFirstVisibleItem();
		if (hFirstVisibleItem == h)
			m_bFirtVisible = TRUE;
	}
}

CaIpmTreeData::~CaIpmTreeData()
{
	while (!m_listChildren.IsEmpty())
		delete m_listChildren.RemoveHead();
	//
	// Do not delete 'm_pItem'
}

void CaIpmTreeData::Serialize (CArchive& ar)
{
	m_listChildren.Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_nuState;
		ar << m_strItemText;
		ar << m_nImage;
		ar << m_nSelectedImage;
		ar << m_bFirtVisible;
		ar << m_pItem;
	}
	else
	{
		ar >> m_nuState;
		ar >> m_strItemText;
		ar >> m_nImage;
		ar >> m_nSelectedImage;
		ar >> m_bFirtVisible;
		ar >> m_pItem;
	}
}


// declaration for serialization
#ifdef BUGGY_IMAGELIST
IMPLEMENT_SERIAL ( CTreeGlobalData, CObject, 3)
#else
IMPLEMENT_SERIAL ( CTreeGlobalData, CObject, 2)
#endif

// "normal" constructor
CTreeGlobalData::CTreeGlobalData(CTreeCtrl *pTree, LPSFILTER psFilter, CdIpmDoc* pDoc)
{
	// pointer on filter, and hNode can be null ---> no assertion

	m_pTree    = pTree;
	m_psFilter = psFilter;
	m_pIpmDoc  = pDoc;

	// image list
	m_imageList.Create(IMAGE_WIDTH, IMAGE_HEIGHT, TRUE, 10, 10);
	if (m_pTree)
		m_pTree->SetImageList(&m_imageList, TVSIL_NORMAL);

	// insert blank bitmap for non-static items
	int blankIndex = AddBitmap(IDB_BITMAP_DEFAULT);

	// Not loading yet!
	m_bCurrentlyLoading = FALSE;
	m_pTreeData = NULL;
	m_nTreeHScrollPos = 0;
}

void CTreeGlobalData::SetPTree(CTreeCtrl *pTree, BOOL bSetImageList)
{
	m_pTree = pTree;
	if (pTree)
		if (bSetImageList)
			m_pTree->SetImageList(&m_imageList, TVSIL_NORMAL);
}

// serialization constructor
CTreeGlobalData::CTreeGlobalData()
{
	m_pIpmDoc = NULL;
	m_pTreeData = NULL;
	m_nTreeHScrollPos = 0;
}

// destructor
CTreeGlobalData::~CTreeGlobalData()
{
	m_imageList.DeleteImageList();
	if (m_pTreeData)
		delete m_pTreeData;
}

// serialization method
void CTreeGlobalData::Serialize (CArchive& ar)
{
	if (ar.IsStoring()) 
	{
		m_aBitmapIndex.Serialize(ar);

#ifdef BUGGY_IMAGELIST
		m_aBmpId.Serialize(ar);
#else  // #ifdef BUGGY_IMAGELIST
		BOOL bSuccess = m_imageList.Write(&ar);
		ASSERT (bSuccess);
#endif  // BUGGY_IMAGELIST

		// don't serialize m_psFilter
		// Don't serialize pTree!

		//
		// Store tree data:
		HTREEITEM hRoot = m_pTree->GetRootItem();
		if (hRoot)
		{
			m_nTreeHScrollPos = m_pTree->GetScrollPos(SB_HORZ);
			if (!m_pTreeData)
				m_pTreeData = new CaIpmTreeData(m_pTree, hRoot);
			FillSerialListFromTree(hRoot, m_pTreeData);
			ar << m_pTreeData;
			ar << m_nTreeHScrollPos;

			delete m_pTreeData;
			m_pTreeData = NULL;
		}
	}
	else 
	{
		m_aBitmapIndex.Serialize(ar);

#ifdef BUGGY_IMAGELIST
		m_aBmpId.Serialize(ar);
		int nbImages = m_aBmpId.GetSize();
		ASSERT (nbImages > 0);
		ASSERT (nbImages ==  m_aBitmapIndex.GetCount());
		m_imageList.Create(IMAGE_WIDTH, IMAGE_HEIGHT, TRUE, 10, 10);
		for (int cpt = 0; cpt < nbImages; cpt++) {
			CBitmap bitmap;
			int bitmapId = m_aBmpId[cpt];
			int mapId;
			ASSERT (m_aBitmapIndex.Lookup(bitmapId, mapId));
			ASSERT (mapId == cpt+1);
			BOOL bSuccess = bitmap.LoadBitmap(bitmapId);
			ASSERT (bSuccess);
			int iconIndex = m_imageList.Add(&bitmap,  RGB(255, 0, 255));
			ASSERT (iconIndex != -1);
		}
#else   // #ifdef BUGGY_IMAGELIST
		BOOL bSuccess = m_imageList.Read(&ar);
		ASSERT (bSuccess);
#endif  // BUGGY_IMAGELIST

		// m_psFilter reloaded by caller serialize
		// Don't serialize pTree!

		// 
		// Load tree data:
		if (m_pTreeData)
			delete m_pTreeData;
		ar >> m_pTreeData;
		ar >> m_nTreeHScrollPos;
	}
}

// Adapted from tree_dll.cpp :
// adds a bitmap in the associated iconlist,
// and returns the index on it,
// or -1 if error
int CTreeGlobalData::AddBitmap(UINT bitmapId)
{
	int bitmapIndex = -1;
	
	// check whether the bitmap has already been stored
	// in the image list
	// note that the index has been stored with +1
	// since it starts from 0 in CImageList
	// while 0 is not a valid value in a collection
	if (m_aBitmapIndex.Lookup(bitmapId, bitmapIndex))
		return bitmapIndex - 1;
	
	CBitmap bitmap;
	bitmap.LoadBitmap(bitmapId);
	int iconIndex = m_imageList.Add(&bitmap,  RGB(255, 0, 255));
	
	// add in iconHandle table with a +1 offset
	m_aBitmapIndex.SetAt(bitmapId, iconIndex+1);
	
	// add in bitmapIds Array for future serialisation special handling because of unix problem
	m_aBmpId.Add(bitmapId);
	
	return iconIndex;
}

// must be called once before tree destruction
void CTreeGlobalData::FreeAllTreeItemsData()
{
	HTREEITEM hItem;
	hItem = m_pTree->GetRootItem();
	if (hItem) {
		FreeItemData(hItem);
		while (1) {
			hItem = m_pTree->GetNextSiblingItem(hItem);
			if (!hItem)
				break;
			FreeItemData(hItem);
		}
	}
}

void CTreeGlobalData::FreeItemData(HTREEITEM hItem)
{
	CTreeItem *pData = (CTreeItem *)m_pTree->GetItemData(hItem);
	if (pData)
		delete pData;
	
	HTREEITEM hChildItem;
	hChildItem = m_pTree->GetChildItem(hItem);
	while (hChildItem) {
		FreeItemData(hChildItem);
		hChildItem = m_pTree->GetNextSiblingItem(hChildItem);
	}
}


UINT CTreeGlobalData::GetContextMenuId()
{
	HTREEITEM hItem = m_pTree->GetSelectedItem();
	if (!hItem)
		return 0;
	CTreeItem *pItem = (CTreeItem *)m_pTree->GetItemData(hItem);
	ASSERT(pItem);
	if (!pItem)
		return 0;
	UINT menuId = pItem->GetContextMenuId();
	ASSERT (menuId);
	return menuId;
}

BOOL CTreeGlobalData::FilterAllDisplayedItemsLists(FilterCause because)
{
	HTREEITEM hItem;
	hItem = m_pTree->GetRootItem();
	if (hItem) {
		if (!FilterDisplayedItemsList(hItem, because))
			return FALSE;
		while (1) {
			hItem = m_pTree->GetNextSiblingItem(hItem);
			if (!hItem)
				break;
			if (!FilterDisplayedItemsList(hItem, because))
				return FALSE;
		}
	}
	return TRUE;
}

BOOL CTreeGlobalData::FilterDisplayedItemsList(HTREEITEM hItem, FilterCause because)
{
	CTreeItem *pItem = (CTreeItem *)m_pTree->GetItemData(hItem);
	if (pItem)
		if (pItem->IsStatic())
			if (pItem->GetSubBK() == SUBBRANCH_KIND_OBJ || pItem->GetSubBK() == SUBBRANCH_KIND_MIX)
				if (!pItem->FilterDisplayedItemsList(hItem, because))
					return FALSE;

	HTREEITEM hChildItem;
	hChildItem = m_pTree->GetChildItem(hItem);
	while (hChildItem) {
		if (!FilterDisplayedItemsList(hChildItem, because))
			return FALSE;
		hChildItem = m_pTree->GetNextSiblingItem(hChildItem);
	}
	return TRUE;
}

//
// First call:
//   hItem     = m_pTree->GetRootItem();
//   pTreeNode = m_pTreeData;
void CTreeGlobalData::FillSerialListFromTree(HTREEITEM hItem, CaIpmTreeData* pTreeNode)
{
	ASSERT(m_pTree);
	if (!m_pTree)
		return;
	while (hItem)
	{
		//
		// Store the current item:
		CaIpmTreeData* pItemData = new CaIpmTreeData(m_pTree, hItem);
		pTreeNode->AddSibling(pItemData);

		//
		// Store the children:
		if (m_pTree->ItemHasChildren(hItem))
		{
			HTREEITEM hChild = m_pTree->GetChildItem(hItem);
			FillSerialListFromTree(hChild, pItemData);
		}

		//
		// Next sibling item:
		hItem = m_pTree->GetNextSiblingItem(hItem);
	}
}

//
// First call:
// pTreeData = m_pTreeData
void CTreeGlobalData::LoadTree(CaIpmTreeData* pTreeData, HTREEITEM hTree, HTREEITEM& hFirstVisible)
{
	ASSERT(pTreeData);
	if (pTreeData)
	{
		CTypedPtrList<CObList, CaIpmTreeData*>& listChildren = pTreeData->GetListChildren();
		POSITION pos = listChildren.GetHeadPosition();
		while (pos != NULL)
		{
			CaIpmTreeData* pItem = listChildren.GetNext(pos);
			CString strItemText  = pItem->GetItemText();
			HTREEITEM hParent    = hTree? hTree: m_pTree->GetRootItem();
			HTREEITEM hNewItem   = m_pTree->InsertItem(strItemText, hParent);
			UINT nState          = pItem->GetItemState();
			int nImage           = pItem->GetImage();
			int nSelectedImage   = pItem->GetSelectedImage();
			CTreeItem* pItemData = pItem->GetItemData();

			m_pTree->SetItemImage(hNewItem, nImage, nSelectedImage);
			m_pTree->SetItemData (hNewItem, (DWORD)pItemData);
			//
			// Create children of current item:
			if (!pItem->GetListChildren().IsEmpty())
				LoadTree(pItem, hNewItem, hFirstVisible);
			if (nState & TVIS_EXPANDED)
				m_pTree->Expand(hNewItem, TVE_EXPAND);
			if (nState & TVIS_SELECTED)
				m_pTree->SelectItem(hNewItem);

			if (pItem->IsFirstVisible())
				hFirstVisible = hNewItem;
		}
	}
}


//
// First call:
// pTreeData = m_pTreeData
void CTreeGlobalData::FillTreeFromSerialList()
{
	m_bCurrentlyLoading = TRUE;
	ASSERT(m_pTreeData);
	if (m_pTreeData)
	{
		HTREEITEM hFirstVisible = NULL;
		LoadTree(m_pTreeData, NULL, hFirstVisible);
		if (hFirstVisible)
			m_pTree->Select(hFirstVisible, TVGN_FIRSTVISIBLE);
		m_pTree->SetScrollPos(SB_HORZ, m_nTreeHScrollPos);

		delete m_pTreeData;
		m_pTreeData = NULL;
	}
	m_bCurrentlyLoading = FALSE;
}

void CTreeGlobalData::RefreshAllTreeBranches()
{
	HTREEITEM hItem;
	hItem = m_pTree->GetRootItem();
	if (hItem) {
		RefreshBranchAndSubBranches(hItem);
		while (1) {
			hItem = m_pTree->GetNextSiblingItem(hItem);
			if (!hItem)
				break;
			RefreshBranchAndSubBranches(hItem);
		}
	}
}

void CTreeGlobalData::RefreshBranchAndSubBranches(HTREEITEM hItem)
{
	CTreeItem *pItem = (CTreeItem *)m_pTree->GetItemData(hItem);
	if (pItem) {
		if (pItem->GetSubBK() == SUBBRANCH_KIND_OBJ || pItem->GetSubBK() == SUBBRANCH_KIND_MIX) {
			BOOL bRet = pItem->DisplayDataSubBranches(hItem, DISPLAY_ACT_REFRESH);
			ASSERT (bRet);
			if (!bRet)
				return;     // stop recursion
		}
	}

	HTREEITEM hChildItem;
	hChildItem = m_pTree->GetChildItem(hItem);
	while (hChildItem) {
		RefreshBranchAndSubBranches(hChildItem);
		hChildItem = m_pTree->GetNextSiblingItem(hChildItem);
	}
}

//
// Expand
//

void CTreeGlobalData::ExpandBranch(HTREEITEM hItem)
{
	ASSERT (hItem);

	// Recursivity until non-expandable branches
	CTreeItem *pData = (CTreeItem *)m_pTree->GetItemData(hItem);
	ASSERT (pData);
	if (pData->CanBeExpanded()) {
		m_pTree->Expand(hItem, TVE_EXPAND);
		HTREEITEM hChildItem;
		hChildItem = m_pTree->GetChildItem(hItem);
		while (hChildItem) {
			ExpandBranch(hChildItem);
			hChildItem = m_pTree->GetNextSiblingItem(hChildItem);
		}
	}
}

void CTreeGlobalData::ExpandBranch()
{
	HTREEITEM hItem;
	hItem = m_pTree->GetSelectedItem();
	ASSERT (hItem);
	if (hItem)
		ExpandBranch(hItem);
}

void CTreeGlobalData::ExpandAll()
{
	HTREEITEM hItem;
	hItem = m_pTree->GetRootItem();
	if (hItem) {
		do {
			ExpandBranch(hItem);
			hItem = m_pTree->GetNextSiblingItem(hItem);
		}
		while (hItem);
	}
}

//
// Collapse
//

void CTreeGlobalData::CollapseBranch(HTREEITEM hItem)
{
	ASSERT (hItem);
	m_pTree->Expand(hItem, TVE_COLLAPSE);
}

void CTreeGlobalData::CollapseBranch()
{
	HTREEITEM hItem;
	hItem = m_pTree->GetSelectedItem();
	ASSERT (hItem);
	if (hItem)
		CollapseBranch(hItem);
}

void CTreeGlobalData::CollapseAll()
{
	// If current selection is not a root item,
	// change sel to it's parent root BEFORE collapsing
	// (message reflection does not work for TVN_SELCHANGING and TVN_SELCHANGED while collapsing)
	HTREEITEM hSelItem = m_pTree->GetSelectedItem();
	ASSERT (hSelItem);
	HTREEITEM hRootItem = m_pTree->GetParentItem(hSelItem);
	if (hRootItem) {
		HTREEITEM hOldRootItem;
		do {
			hOldRootItem = hRootItem;
			hRootItem = m_pTree->GetParentItem(hOldRootItem);
		}
		while (hRootItem);
		m_pTree->SelectItem(hOldRootItem);   // will generate TVN_SELCHANGING and TVN_SELCHANGED notification messages.
	}
	
	// Collapse all branches
	HTREEITEM hItem = m_pTree->GetRootItem();
	if (hItem) {
		do {
			CollapseBranch(hItem);
			hItem = m_pTree->GetNextSiblingItem(hItem);
		}
		while (hItem);
	}
}

