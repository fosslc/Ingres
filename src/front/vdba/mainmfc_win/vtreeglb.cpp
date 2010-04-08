/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source  : vtreeglb.cpp: 
**    Author  : Emmanuel Blattes 
**
** History :
** 
** 07-Feb-1997 (Emmanuel Blattes)
**    created.
** 26-Feb-2002 (uk$so01)
**    SIR #106648, Integrate IPM ActiveX Control
** 01-Dec-2004 (uk$so01)
**    VDBA BUG #113548 / ISSUE #13768610 
**    Fix the problem of serialization.
*/

// vTreeDat.cpp - Emmanuel Blattes - started Feb. 7, 97
// 

#include "stdafx.h"
#include "rcdepend.h"
#include "vtreeglb.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef MAINWIN
#define BUGGY_IMAGELIST
#endif

// declaration for serialization
#ifdef BUGGY_IMAGELIST
IMPLEMENT_SERIAL ( CTreeGlobalData, CObject, 3)
#else
IMPLEMENT_SERIAL ( CTreeGlobalData, CObject, 2)
#endif

// "normal" constructor
CTreeGlobalData::CTreeGlobalData(CTreeCtrl *pTree, LPSFILTER psFilter, int hNode)
{
  // pointer on filter, and hNode can be null ---> no assertion

  m_pTree    = pTree;
  m_psFilter = psFilter;
  m_hNode    = hNode;

  // image list
  m_imageList.Create(IMAGE_WIDTH,
                     IMAGE_HEIGHT,
                     TRUE,    // bMask
                     10, 10);
  if (m_pTree)
    m_pTree->SetImageList(&m_imageList, TVSIL_NORMAL);

  // insert blank bitmap for non-static items
  int blankIndex = AddBitmap(IDB_BITMAP_DEFAULT);

  // Not loading yet!
  m_bCurrentlyLoading = FALSE;
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
}

// destructor
CTreeGlobalData::~CTreeGlobalData()
{
  m_imageList.DeleteImageList();
}

// serialization method
void CTreeGlobalData::Serialize (CArchive& ar)
{
	CObject::Serialize(ar);
  if (ar.IsStoring()) {
    m_aBitmapIndex.Serialize(ar);
    ar << m_hNode;

#ifdef BUGGY_IMAGELIST
    m_aBmpId.Serialize(ar);
#else   // #ifdef BUGGY_IMAGELIST
    BOOL bSuccess = m_imageList.Write(&ar);
    ASSERT (bSuccess);
#endif  // BUGGY_IMAGELIST

    // don't serialize m_psFilter
    // Don't serialize pTree!
  }
  else {
    m_aBitmapIndex.Serialize(ar);
    ar >> m_hNode;

#ifdef BUGGY_IMAGELIST
    m_aBmpId.Serialize(ar);
    int nbImages = m_aBmpId.GetSize();
    ASSERT (nbImages > 0);
    ASSERT (nbImages ==  m_aBitmapIndex.GetCount());
    m_imageList.Create(IMAGE_WIDTH,
                       IMAGE_HEIGHT,
                       TRUE,    // bMask
                       10, 10);
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
  }

  // tree lines
  if (ar.IsStoring())
    FillSerialListFromTree();
  m_treeLineSerialList.Serialize(ar);
  if (ar.IsStoring())
    CleanupSerialList();    // clean after store!

  // extra information for tree
  ASSERT (sizeof(DWORD) == sizeof(m_hSelectedItem));    // HTREEITEM
  if (ar.IsStoring()) {
    m_hSelectedItem = m_pTree->GetSelectedItem();
    m_hFirstVisibleItem = m_pTree->GetFirstVisibleItem();
    ar << (DWORD)m_hSelectedItem;
    ar << (DWORD)m_hFirstVisibleItem;
  }
  else {
    DWORD dw;
    ar >> dw;
    m_hSelectedItem = (HTREEITEM)dw;
    ar >> dw;
    m_hFirstVisibleItem = (HTREEITEM)dw;
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
                                            //(COLORREF)0xFFFFFF);

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

void CTreeGlobalData::FillSerialListFromTree()
{
  CleanupSerialList();

  HTREEITEM hItem;
  hItem = m_pTree->GetRootItem();
  if (hItem) {
    StoreTreeBranch(hItem, NULL);
    while (1) {
      hItem = m_pTree->GetNextSiblingItem(hItem);
      if (!hItem)
        break;
      StoreTreeBranch(hItem, NULL);
    }
  }
}

void CTreeGlobalData::StoreTreeBranch(HTREEITEM hItem, HTREEITEM hParentItem)
{
  //
  // store current branch information:
  //

  m_treeLineSerialList.AddTail(new CTreeLineSerial(m_pTree, hItem, hParentItem));

  //
  // store child branches
  //
  HTREEITEM hChildItem;
  hChildItem = m_pTree->GetChildItem(hItem);
  while (hChildItem) {
    StoreTreeBranch(hChildItem, hItem);
    hChildItem = m_pTree->GetNextSiblingItem(hChildItem);
  }
}

void CTreeGlobalData::FillTreeFromSerialList()
{

  // Map between stored and recreated HTREEITEMS
  // make use of conversion between HTREEITEM and DWORD (since HTREEITEM is a pointer)
  // because HTREEITEM cannot be used directly on right side
  ASSERT (sizeof(HTREEITEM) == sizeof(DWORD) );
  CMap< DWORD, DWORD, DWORD, DWORD > hTreeItemMap;

  m_bCurrentlyLoading = TRUE;

  UINT      prevLineItemState = 0;      // no prev. item ---> like not expanded
  HTREEITEM prevHItem         = NULL;   // no previous item

  POSITION pos = m_treeLineSerialList.GetHeadPosition();
  while (pos) {
    CTreeLineSerial* tLine = m_treeLineSerialList.GetNext (pos);

    // extract data from tLine
    HTREEITEM         lineHItem          = tLine->GetHItem()         ;
    HTREEITEM         lineHParentItem    = tLine->GetHParentItem()   ;
    CString           lineText           = tLine->GetText()          ;
    int               lineNImage         = tLine->GetNImage()        ;
    int               lineNSelectedImage = tLine->GetNSelectedImage();
    UINT              lineItemState      = tLine->GetItemState()     ;
    CTreeItem        *linePItem          = tLine->GetPItem()         ;
    CTreeGlobalData  *linePGlobData      = tLine->GetPGlobData()     ;

    // checks
    ASSERT (lineHItem);
    ASSERT (linePGlobData == 0 || linePGlobData == this);
    // Note : linePItem can be NULL or any value

    // manage HTREEITEMS map
    HTREEITEM hNewParentItem = NULL;
    if (lineHParentItem) {
      DWORD dwHNewParentItem = 0;
      if (!hTreeItemMap.Lookup((DWORD)lineHParentItem, dwHNewParentItem))   // key, ref. on value
        ASSERT (FALSE);     // The parent MUST have been previously inserted!
      hNewParentItem = (HTREEITEM)dwHNewParentItem;
      ASSERT(hNewParentItem);
    }

    // create the appropriate line
    HTREEITEM hNewItem = m_pTree->InsertItem(lineText, hNewParentItem);
    ASSERT (hNewItem);

    // Store result htreeitem for future use as a parent
    DWORD dwLineHItem = (DWORD)lineHItem;
    DWORD dwHNewItem  = (DWORD)hNewItem;
    hTreeItemMap.SetAt(dwLineHItem, dwHNewItem);

    // add complimentary info to created line
    m_pTree->SetItemData(hNewItem, (DWORD)linePItem);
    m_pTree->SetItemImage(hNewItem, lineNImage, lineNSelectedImage);

    // Set previous item state (expanded/collapsed)
    if (prevHItem) {
      if (prevLineItemState & TVIS_EXPANDED)
        m_pTree->Expand(prevHItem, TVE_EXPAND);
      //else
      //  m_pTree->Expand(prevHItem, TVE_COLLAPSE);
    }

    // store current item for use as the previous in next loop iteration
    prevLineItemState = lineItemState;
    prevHItem = hNewItem;
  }

  // selected item
  DWORD dwHItem = 0;
  if (m_hSelectedItem) {
    if (!hTreeItemMap.Lookup((DWORD)m_hSelectedItem, dwHItem))
      ASSERT (FALSE);
    HTREEITEM hItem = (HTREEITEM)dwHItem;
    m_pTree->SelectItem(hItem);
  }

  // first visible item
  dwHItem = 0;
  ASSERT (m_hFirstVisibleItem);
  if (!hTreeItemMap.Lookup((DWORD)m_hFirstVisibleItem, dwHItem))
    ASSERT (FALSE);
  HTREEITEM hItem = (HTREEITEM)dwHItem;
  m_pTree->Select(hItem, TVGN_FIRSTVISIBLE);

  // Cleanup Map
  hTreeItemMap.RemoveAll();

  CleanupSerialList();

  // finished loading
  m_bCurrentlyLoading = FALSE;
}

void CTreeGlobalData::CleanupSerialList()
{
  while (!m_treeLineSerialList.IsEmpty())
    delete m_treeLineSerialList.RemoveHead();
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

