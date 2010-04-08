/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vnitem3.cpp: New branches for "Nodes" tree 
**               related to "SvrUsers" aliasing for a connection
**    Author  : Emmanuel Blattes 
**
** History :
** 
** 03-Feb-0997 (Emmanuel Blattes)
**    created
** 26-Feb-2002 (uk$so01)
**    SIR #106648, Integrate IPM ActiveX Control
** 01-Dec-2004 (uk$so01)
**    VDBA BUG #113548 / ISSUE #13768610 
**    Fix the problem of serialization.
*/

// vTree.cpp - Emmanuel Blattes - started Feb. 3, 97
// 

#include "stdafx.h"
#include "rcdepend.h"
#include "vtree.h"
#include "ipmdisp.h"    // CuPageInformation


extern "C" {
#include "dba.h"
#include "domdata.h"
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// declaration for serialization
IMPLEMENT_SERIAL ( CTreeItem, CObject, 3)


///////////////////////////////////////////////////////////////////////////////////////////
// Utilities for replicator
int  MonGetReplicatorVersion(int hNode, CString dbName)
{
  int     replicVer;
  char    szOwnerName[MAXOBJECTNAME];
  char    dummyBuf[MAXOBJECTNAME];
  LPUCHAR aDummyParents[MAXPLEVEL];
  int     dummyType;

  // 1) get ownername for database
  aDummyParents[0] = aDummyParents[1] = aDummyParents[2] = NULL;
  int result = DOMGetObjectLimitedInfo(hNode,
                                      (LPUCHAR)(LPCTSTR)dbName, // db name
                                      (LPUCHAR)"",              // parent
                                      OT_DATABASE,              // type
                                      0,                        // parent level
                                      aDummyParents,            // dummy parents table
                                      TRUE,                     // accept system object (hypotetical replication on Coordinator Database...)
                                      &dummyType,               // result type
                                      (LPUCHAR)dummyBuf,        // result objname
                                      (LPUCHAR)szOwnerName,     // result owner
                                      (LPUCHAR)dummyBuf);       // result extradata
  ASSERT (result != RES_ERR);
  if (result == RES_ERR)
    return REPLIC_NOINSTALL;

  // 2) get version number
  // can be REPLIC_V10, REPLIC_V105, REPLIC_V11 or REPLIC_NOINSTALL
  replicVer = GetReplicInstallStatus(hNode,
                                     (LPUCHAR)(LPCTSTR)dbName,
                                     (LPUCHAR)szOwnerName);

  // 3) return value
  return replicVer;
}

BOOL MonIsReplicatorInstalled(int hNode, CString dbName)
{
  int replicVer = MonGetReplicatorVersion(hNode, dbName);
  if (replicVer == REPLIC_NOINSTALL)
    return FALSE;
  else
    return TRUE;
}

// constructor for static item
CTreeItem::CTreeItem(CTreeGlobalData *pTreeGD, int type, UINT nameId, SubBK subBranchKind, UINT imageId, CTreeItemData *pItemData, int childtype, UINT noChildStringId)
{
  m_pPageInfo = NULL;
  CString name;
  name.LoadString(nameId);

  // changed May 12, 97: image id ignored for statics
  imageId = DEFAULT_IMAGE_ID;

  Constructor(pTreeGD, type, name, TRUE, subBranchKind, imageId, FALSE, pItemData, childtype, noChildStringId);
}

// constructor for non-static item
CTreeItem::CTreeItem(CTreeGlobalData *pTreeGD, int type, SubBK subBranchKind, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype, UINT noChildStringId)
{
  m_pPageInfo = NULL;
  if (pItemData)
    Constructor(pTreeGD, type, pItemData->GetItemName(type), FALSE, subBranchKind, DEFAULT_IMAGE_ID, bNeedsTypeSolve, pItemData, childtype, noChildStringId);
  else
    Constructor(pTreeGD, type, "", FALSE, subBranchKind, DEFAULT_IMAGE_ID, bNeedsTypeSolve, pItemData, childtype, noChildStringId);
}

// called by constructors
void CTreeItem::Constructor(CTreeGlobalData *pTreeGD, int type, CString name, BOOL bStatic, SubBK subBranchKind, UINT imageId, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype, UINT noChildStringId)
{
  m_bToBeDeleted = FALSE;               // not to be deleted!
  m_itemKind     = ITEM_KIND_NORMAL;    // standard item

  m_pTreeGD = pTreeGD;

  m_type = type;
  m_childtype = childtype;

  m_name = name;
  m_bAlreadyExpanded = FALSE;
  m_bStatic = bStatic;
  m_subBranchKind = subBranchKind;

  if (pItemData)
    m_pItemData = new CTreeItemData(*pItemData);
  else
    m_pItemData = NULL;

  // specific as of may 12 (previously static specific)
  m_imageId = imageId;
  if (bStatic)
    ASSERT(imageId==DEFAULT_IMAGE_ID);

  // non-static specific
  m_bNeedsTypeSolve = bNeedsTypeSolve;

  // having non-static children specific
  if (m_subBranchKind == SUBBRANCH_KIND_OBJ || m_subBranchKind == SUBBRANCH_KIND_MIX)
    ASSERT (noChildStringId);
  m_noChildStringId = noChildStringId;

  m_menuId = NULL;     // Assume no context menu id

  // completed type (eventually solved)
  m_completetype = m_type;    // default : identical
  m_bTypeCompleted = FALSE;   // not completed yet
}

// minimum constructor for serialization preparation
CTreeItem::CTreeItem(CTreeGlobalData *pTreeGD)
{
  Constructor(pTreeGD, OT_ERROR, "", FALSE, SUBBRANCH_KIND_NONE, DEFAULT_IMAGE_ID, FALSE, NULL, CHILDTYPE_UNDEF, 0);
}

// serialization constructor
CTreeItem::CTreeItem()
{
    m_pPageInfo = NULL;
}

// virtual destructor
CTreeItem::~CTreeItem()
{
  if (m_pItemData)
    delete(m_pItemData);
  if (m_pPageInfo) 
  {
    delete m_pPageInfo;
/*UKS
    // special for ipm : reset reference in right pane
    CTreeCtrl *pTree = GetPTreeGlobData()->GetPTree();
    ASSERT (pTree);
    for (CWnd* pWnd = pTree->GetParent();
         pWnd;
         pWnd = pWnd->GetParent()) {
      if (pWnd->IsKindOf(RUNTIME_CLASS(CIpmFrame))) {
        CIpmFrame* pIpmFrame = (CIpmFrame*)pWnd;
        CIpmView2* pView2 = (CIpmView2*)pIpmFrame->GetRightPane();
        ASSERT (pView2);
        CuDlgIpmTabCtrl* pTabCtrl = pView2->m_pIpmTabDialog;
        ASSERT (pTabCtrl);
        // Special management for replication ---> GetCurrentProperty() returns NULL if replicator not installed
        if (pTabCtrl->GetCurrentProperty()) {
          CuPageInformation* pCurrentPageInfo = pTabCtrl->GetCurrentProperty()->GetPageInfo();
          if (pCurrentPageInfo == m_pPageInfo)
            pTabCtrl->GetCurrentProperty()->SetPageInfo (NULL);
        }
        break;  // break the for
      }
    }
*/
  }
}

// serialization method
void CTreeItem::Serialize (CArchive& ar)
{
  int intermediary;
  CObject::Serialize(ar);
  if (ar.IsStoring()) {
    ar << m_pTreeGD;
    ar << m_type;
    ar << m_childtype;
    ar << m_name;
    ar << m_bAlreadyExpanded;
    ar << m_bStatic;
    ar << (int)m_subBranchKind;
    ar << m_pItemData;
    ar << m_menuId;
    ar << m_completetype;
    ar << m_bTypeCompleted;
    ar << (int)m_itemKind;
    ar << m_bToBeDeleted;
    ar << m_imageId;
    ar << m_bNeedsTypeSolve;
    ar << m_noChildStringId;

    ar << m_pPageInfo;
  }
  else {
    ar >> m_pTreeGD;
    ar >> m_type;
    ar >> m_childtype;
    ar >> m_name;
    ar >> m_bAlreadyExpanded;
    ar >> m_bStatic;
    ar >> intermediary;
    m_subBranchKind = (SubBK)intermediary;
    ar >> m_pItemData;
    ar >> m_menuId;
    ar >> m_completetype;
    ar >> m_bTypeCompleted;
    ar >> intermediary;
    m_itemKind = (ItemKind)intermediary;
    ar >> m_bToBeDeleted;
    ar >> m_imageId;
    ar >> m_bNeedsTypeSolve;
    ar >> m_noChildStringId;

    ar >> m_pPageInfo;
  }
}

BOOL CTreeItem::CreateStaticSubBranches(HTREEITEM hParentItem)
{
  // Does nothing in the base class, but... should not be called
  // Pure virtual impossible due to serialization mechanism

  // derived classes should code like this;
  //CTreeItemDeriv *pItemDeriv;
  //pItemDeriv = (CTreeItem *) new CTreeItemDeriv1(...);
  //if (!pItemDeriv)
  //  return FALSE;
  //if (pItemDeriv->CreateTreeLine(hParentItem)==NULL)
  //  return FALSE;
  // Repeat as many times as needed

  ASSERT(FALSE);
  return FALSE;    // act as if failure
}

BOOL CTreeItem::CreateDataSubBranches(HTREEITEM hParentItem)
{
  return DisplayDataSubBranches(hParentItem, DISPLAY_ACT_EXPAND);
}

BOOL CTreeItem::RefreshDataSubBranches(HTREEITEM hParentItem, CTreeItem *pExistItem, BOOL *pbStillExists)
{
  BOOL bRet = DisplayDataSubBranches(hParentItem, DISPLAY_ACT_REFRESH);
  if (pExistItem && pbStillExists) {
    *pbStillExists = FALSE;
    CTreeCtrl *pTree = GetPTreeGlobData()->GetPTree();
    HTREEITEM hChildItem = pTree->GetChildItem(hParentItem);
    ASSERT(hChildItem);   // must have at least one child
    for ( ; hChildItem; hChildItem = pTree->GetNextSiblingItem(hChildItem)) {
      CTreeItem *pChildItem = (CTreeItem *)pTree->GetItemData(hChildItem);
      if (pChildItem == pExistItem) {
        *pbStillExists = TRUE;
        break;
      }
    }
  }
  return bRet;
}

BOOL CTreeItem::DisplayDataSubBranches(HTREEITEM hParentItem, DispAction action)
{
  CTreeGlobalData *pTreeGD  = GetPTreeGlobData();
  CTreeCtrl       *pTree    = pTreeGD->GetPTree();
  LPSFILTER        psFilter = pTreeGD->GetPSFilter();
  int              hNode    = pTreeGD->GetHNode();

  BOOL             bAlreadyExpanded = IsAlreadyExpanded();

  // Hourglass
  CWaitCursor   hourglass;

  // Pre-test : for refresh or filter, nothing to do if not already expanded
  if (action == DISPLAY_ACT_REFRESH || action == DISPLAY_ACT_CHANGEFILTER) {
    if (!bAlreadyExpanded)
      return TRUE;
  }

  // mark all immediate sub-items as "to be deleted"
  HTREEITEM hDummy = MarkAllImmediateSubItems(hParentItem);

  // prepare variables for requested data
  int   iobjecttypeParent   = GetType();
  void *pstructParent = NULL;   // in case no parent
  if (GetPTreeItemData())
    pstructParent = GetPTreeItemData()->GetDataPtr();
  int   iobjecttypeReq      = GetChildType();
  int   reqSize             = ItemStructSize(iobjecttypeReq);
	CString csItemName;

  // allocate one requested structure
  void *pStructReq          = new char[reqSize];
  memset (pStructReq, 0, reqSize);

  // For Star: Manage tree with multiple nodes
  StarOpenNodeStruct(iobjecttypeParent, pstructParent, hNode);

  // Specific for replicator branches in monitor
  if (iobjecttypeReq == OT_MON_REPLIC_SERVER) {
    ASSERT (iobjecttypeParent==OT_DATABASE);
    if (iobjecttypeParent==OT_DATABASE) {
      BOOL bReplicatorInstalled = MonIsReplicatorInstalled(hNode, GetPTreeItemData()->GetItemName(iobjecttypeParent));
      if (!bReplicatorInstalled) {
        // delete all current sub-items
        DeleteAllMarkedSubItems(hParentItem, hDummy);

        // minimum fill of req struct starting from parent struct
        MinFillpStructReq(pStructReq);
        
        // create special child item using this "minimum-filled" structure and empty name
        CTreeItemData ItemData(pStructReq, reqSize, hNode, "");
        BOOL bSuccess = CreateNoChildItem(hParentItem, iobjecttypeReq, IDS_TM_REPLIC_NOTINSTALLED, &ItemData);
        delete pStructReq;                                    // free structure that has been duplicated.
        return bSuccess;
      }
    }
  }

  // call GetFirstMonInfo
  int res = ItemGetFirst(hNode,
                         iobjecttypeParent,
                         pstructParent,
                         iobjecttypeReq,
                         pStructReq,
                         psFilter,
                         csItemName);	// reference
  // manage errors
  switch(res) {
    case RES_ERR:
    case RES_TIMEOUT:
    case RES_NOGRANT:
    case RES_NOT_OI:
    case RES_CANNOT_SOLVE:
    case RES_MULTIPLE_GRANTS:

      // free useless structure
      delete pStructReq;

      // only if first expansion, or if refresh and not RES_ERR:
      // delete remaining marked items, then create error item
      if (!bAlreadyExpanded || (bAlreadyExpanded && res!=RES_ERR)) {
        DeleteAllMarkedSubItems(hParentItem, hDummy);
        CreateErrorItem(hParentItem, iobjecttypeReq, res);
      }
      else {
        // Special : if type is user, refresh connected state
        if (iobjecttypeReq == OT_USER) {
          HTREEITEM hCurChildItem = pTree->GetChildItem(hParentItem);
          ASSERT(hCurChildItem);   // must have at least one child
          for ( ; hCurChildItem; hCurChildItem = pTree->GetNextSiblingItem(hCurChildItem)) {
            CTreeItem* pCurChildItem = (CTreeItem *)pTree->GetItemData(hCurChildItem);
            if (pCurChildItem)
              pCurChildItem->ChangeImageId(pCurChildItem->GetCustomImageId(), hCurChildItem);
          }
        }
      }

      // For Star: Manage tree with multiple nodes
      StarCloseNodeStruct(hNode);

      return TRUE;   // Let the error item display
  }

  // loop
  while (res != RES_ENDOFDATA) {
    // search for the item in the tree
    // NOTE : can be improved in speed by starting next search from last found item,
    //        relying on the fact that the items are guaranteed sorted
    //        in the tree And in GetFirst/NextMonInfo() calls
    BOOL bFound = FALSE;
    CTreeItem *pChildItem;
    HTREEITEM hChildItem = pTree->GetChildItem(hParentItem);
    ASSERT(hChildItem);   // must have at least one child
    HTREEITEM hInsertAfter = TVI_LAST;    // we must default by appending the line
    for ( ; hChildItem; hChildItem = pTree->GetNextSiblingItem(hChildItem)) {
      pChildItem = (CTreeItem *)pTree->GetItemData(hChildItem);
      if (!pChildItem) {
        ASSERT (hChildItem == hDummy);
        if (hChildItem != hDummy) {
          // For Star: Manage tree with multiple nodes
          StarCloseNodeStruct(hNode);

          return FALSE;
        }
      }
      else {
        int cmp;
        if (pChildItem->IsSpecialItem())
          cmp = -1;   // assume our object is of "higher value" than the "< No xyz / Error >" string
        else 
          cmp = ItemCompare(iobjecttypeReq, pChildItem->GetPTreeItemData()->GetDataPtr(), pStructReq);
        if (cmp==0) {
          bFound = TRUE;
          break;
        }
        else if (cmp > 0) {
          // our object is "of lower value" than the current one:
          // since existing objects are assumed to be already sorted,
          // we will insert just after the previous item, or just before
          // the current item if it is the first child
          hInsertAfter = pTree->GetPrevSiblingItem(hChildItem);
          if (!hInsertAfter)
            hInsertAfter = TVI_FIRST;
          break;
        }
      }
    }

    if (bFound) {
      // spare current display name
      CString curDispName = pChildItem->GetDisplayName();   // name in child item
      CString newDispName = csItemName;

      // New as of March 13, 98: before replacement of "old structure":
      // update "new structure" fields that were not maintained by GetFirst/Next,
      // i.e. fields used by display, with their current value in "old structure"
      // Currently, only REPLICSERVERDATAMIN concerned with.
      if (iobjecttypeReq == OT_MON_REPLIC_SERVER) {
        LPREPLICSERVERDATAMIN lp1 = (LPREPLICSERVERDATAMIN)pChildItem->GetPTreeItemData()->GetDataPtr();
        LPREPLICSERVERDATAMIN lp2 = (LPREPLICSERVERDATAMIN)pStructReq;
        lp2->m_bRefresh = lp1->m_bRefresh;
      }

      // ABSOLUTELY NEEDED : replace "old structure" with "new structure",
      pChildItem->GetPTreeItemData()->SetNewStructure(pStructReq);

      // Added April 16, 97: displayed name may have changed
      if (newDispName != curDispName) {
        pChildItem->GetPTreeItemData()->SetItemName(newDispName); // store in treeitemdata
        pChildItem->SetDisplayName(newDispName, hChildItem);      // store and update display
      }

      // remove the "mark as to be deleted" feature
      pChildItem->ResetDeleteFlag();

      // image may have changed
      pChildItem->ChangeImageId(pChildItem->GetCustomImageId(), hChildItem);

      // Added April 14, 97: expandability feature according to filter
      BOOL bFitsWithFilter = ItemFitWithFilter(hNode, iobjecttypeReq, pStructReq, psFilter);
      if (bFitsWithFilter)
        pChildItem->MakeBranchExpandable(hChildItem);
      else 
        pChildItem->MakeBranchNotExpandable(hChildItem);

      // Added May 25, 98: Replace "old structure" with "new structure"
      // in immediate sub-branches of type "Static"
      if (pChildItem->IsAlreadyExpanded()) {
        HTREEITEM hSubItem = pTree->GetChildItem(hChildItem);
        ASSERT(hSubItem);   // must have at least one child
        for ( ; hSubItem; hSubItem = pTree->GetNextSiblingItem(hSubItem)) {
          CTreeItem *pSubItem = (CTreeItem *)pTree->GetItemData(hSubItem);
          ASSERT (pSubItem);
          if (pSubItem->IsStatic()) {
            pSubItem->GetPTreeItemData()->SetNewStructure(pStructReq);
          }
        }
      }

      // May 25, 98; moved after update loop on static sub branches
      delete pStructReq;            // won't be used anymore
      pStructReq = NULL;    // check not used anymore

    }
    else {
      // Insert an item
      CTreeItemData ItemData(pStructReq, reqSize, hNode, csItemName);		// can be local since will be duplicated by AllocateChildItem
      CTreeItem *pNewItem = AllocateChildItem(&ItemData);   // Virtual
      delete pStructReq;                                    // free structure that has been duplicated.
      pStructReq = NULL;    // check not used anymore
      if (!pNewItem) {
        // For Star: Manage tree with multiple nodes
        StarCloseNodeStruct(hNode);

        return FALSE;
      }
      ASSERT (iobjecttypeReq == pNewItem->GetType());       // Check types compatibility
      HTREEITEM hNewChild = pNewItem->CreateTreeLine(hParentItem, hInsertAfter);
      ASSERT (hNewChild);
      if (!hNewChild) {
        // For Star: Manage tree with multiple nodes
        StarCloseNodeStruct(hNode);

        return FALSE;
      }
      pNewItem->ChangeImageId(pNewItem->GetCustomImageId(), hNewChild);        // update image if necessary

      // Added April 14, 97: expandability feature according to filter
      BOOL bFitsWithFilter = ItemFitWithFilter(hNode,
                                               iobjecttypeReq,
                                               ItemData.GetDataPtr(),
                                               psFilter);
      if (!bFitsWithFilter)
        pNewItem->MakeBranchNotExpandable(hNewChild);
    }

    // call GetNextMonInfo()
    pStructReq = new char[reqSize];
    memset (pStructReq, 0, reqSize);
    res = ItemGetNext(hNode, iobjecttypeReq, pstructParent, pStructReq, csItemName, iobjecttypeParent);
  }

  // Here, successful loop ended with res_endofdata

  // For Star: Manage tree with multiple nodes
  StarCloseNodeStruct(hNode);

  // delete remaining marked items
  DeleteAllMarkedSubItems(hParentItem, hDummy);

  // if no item left, create one single sub-item "no xyz"
  HTREEITEM hChildItem = pTree->GetChildItem(hParentItem);
  BOOL bMustCreateNoChildItem = FALSE;
  if (!hChildItem)
    bMustCreateNoChildItem = TRUE;    // No item left at all
  else {
    // items left ---> check whether the first is of type static
    CTreeItem * pChildItem = (CTreeItem *)pTree->GetItemData(hChildItem);
    ASSERT (pChildItem);
    if (pChildItem->IsStatic())
      bMustCreateNoChildItem = TRUE;  // No "non-static" item left (ASSUMES STATIC ALWAYS AT THE END!)
  }

  if (bMustCreateNoChildItem) {
    // minimum fill of req struct starting from parent struct
    MinFillpStructReq(pStructReq);

    // create no child item using this "minimum-filled" structure and empty name
    CTreeItemData ItemData(pStructReq, reqSize, hNode, "");
    BOOL bSuccess = CreateNoChildItem(hParentItem, iobjecttypeReq, GetNoChildStringId(), &ItemData);
    delete pStructReq;                                    // free structure that has been duplicated.
    return bSuccess;
  }
  else
    delete pStructReq;
  return TRUE;
}

CTreeItem *CTreeItem::AllocateChildItem(CTreeItemData *pItemData)
{
  // MUST BE DERIVED -
  // CREATES AN OBJECT OF THE APPROPRIATE TYPE
  // I.E. CORRESPONDING TO THE GETCHILDTYPE()
  // CTreeItemXyz *pTreeItem = new CTreeItemXyz(GetPTreeGlobData(), pItemData);
  // return pTreeItem

  ASSERT(FALSE);
  return 0;
}

BOOL CTreeItem::CreateSubBranches(HTREEITEM hItem)
{
  HTREEITEM hDummyItem;

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl       *pTree   = pTreeGD->GetPTree();

  // Added May 12 for new branches management
  if (!IsStatic())
    ASSERT(GetImageId());

  switch(GetSubBK()) {
    case SUBBRANCH_KIND_STATIC:
      // create one or several static sub-branches
      if (!CompleteStruct()) {
        //"Structure Not Completed - Cannot Expand Branch"
        BfxMessageBox(VDBA_MfcResourceString(IDS_E_STRUCTURE_COMPLETED), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
      }
      hDummyItem = pTree->GetChildItem(hItem);
      ASSERT(hDummyItem);
      if (!CreateStaticSubBranches(hItem)) {//"Cannot create sub branches - Cannot Expand Branch"
        BfxMessageBox(VDBA_MfcResourceString(IDS_E_SUB_BRANCHES), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
      }
      pTree->DeleteItem(hDummyItem);
      break;

    case SUBBRANCH_KIND_OBJ:
      if (!CompleteStruct()) {//"Structure Not Completed - Cannot Expand Branch"
        BfxMessageBox(VDBA_MfcResourceString(IDS_E_STRUCTURE_COMPLETED), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
      }
      if(!CreateDataSubBranches(hItem)) {//"cannot create sub branches - Cannot Expand Branch"
        BfxMessageBox(VDBA_MfcResourceString(IDS_E_SUB_BRANCHES), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
      }
      UpdateParentBranchesAfterExpand(hItem);
      break;

    case SUBBRANCH_KIND_MIX:
      if (!CompleteStruct()) {//"Structure Not Completed - Cannot Expand Branch"
        BfxMessageBox(VDBA_MfcResourceString(IDS_E_STRUCTURE_COMPLETED), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
      }
      if(!CreateDataSubBranches(hItem)) {//"cannot create sub branches - Cannot Expand Branch"
        BfxMessageBox(VDBA_MfcResourceString(IDS_E_SUB_BRANCHES), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
      }
      if (!CreateStaticSubBranches(hItem)) {//"Cannot create sub branches - Cannot Expand Branch"
        BfxMessageBox(VDBA_MfcResourceString(IDS_E_SUB_BRANCHES), MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
      }
      UpdateParentBranchesAfterExpand(hItem);
      break;

    default:
      ASSERT(FALSE);
      return FALSE;
  }
  return TRUE;
}

void CTreeItem::UpdateParentBranchesAfterExpand(HTREEITEM hItem)
{
  // To be derived if necessary
}

HTREEITEM CTreeItem::CreateTreeLine(HTREEITEM hParent, HTREEITEM hInsertAfter)
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl       *pTree   = pTreeGD->GetPTree();

  HTREEITEM hItem;
  hItem = pTree->InsertItem(GetDisplayName(), hParent, hInsertAfter);
  ASSERT(hItem);
  if (!hItem)
    return NULL;
  BOOL bSuccess = pTree->SetItemData(hItem, (DWORD)this);
  if (!bSuccess)
    return NULL;

  // bitmap
  if (!IsStatic()) {
    int index = pTreeGD->AddBitmap(GetImageId());
    ASSERT(index);
    pTree->SetItemImage(hItem, index, index);
  }

  // sub-branch
  if (HasSubBranches()) {
    HTREEITEM hDummyItem;
    hDummyItem = pTree->InsertItem("Dummy", hItem);
    ASSERT(hDummyItem);
    if (!hDummyItem)
      return NULL;
    BOOL bSuccess = pTree->SetItemData(hDummyItem, 0);  // Flags dummy item
    if (!bSuccess)
      return NULL;
  }
  return hItem;
}

HTREEITEM CTreeItem::MarkAllImmediateSubItems(HTREEITEM hParentItem)
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl       *pTree   = pTreeGD->GetPTree();

  HTREEITEM hDummy = NULL;
  HTREEITEM hChildItem = pTree->GetChildItem(hParentItem);
  ASSERT(hChildItem);   // must have at least one child
  CTreeItem *pItem;
  while (hChildItem) {
    pItem = (CTreeItem *)pTree->GetItemData(hChildItem);
    if (!pItem)
      hDummy = hChildItem;    // Only one!
    else {
      if (!pItem->IsStatic())       // Do NOT mark trailing static items
        pItem->MarkToBeDeleted();
    }
    hChildItem = pTree->GetNextSiblingItem(hChildItem);
  }
  return hDummy;
}

void CTreeItem::DeleteAllMarkedSubItems(HTREEITEM hParentItem, HTREEITEM hDummy)
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl       *pTree   = pTreeGD->GetPTree();

  HTREEITEM hChildItem = pTree->GetChildItem(hParentItem);
  HTREEITEM hNextChild = 0;
  CTreeItem *pItem;
  while (hChildItem) {
    pItem = (CTreeItem *)pTree->GetItemData(hChildItem);
    hNextChild = pTree->GetNextSiblingItem(hChildItem);
    if (!pItem) {
      ASSERT (hChildItem == hDummy);
      pTree->DeleteItem(hChildItem);
    }
    else {
      if (pItem->IsMarkedToBeDeleted()) {
        if (hChildItem == pTree->GetSelectedItem())
          pTree->SelectItem(hParentItem);             // guaranteed to exist!
        DeleteAllChildrenDataOfItem(hChildItem, FALSE);   // delete all sub branches data, excluding current item data
        delete pItem;                     // After SelectItem() otherwise gpf in pane 2
        pTree->DeleteItem(hChildItem);
      }
    }
    hChildItem = hNextChild;
  }
}

BOOL CTreeItem::ChangeImageId(UINT newId, HTREEITEM hItem)
{
  if (newId == DEFAULT_IMAGE_ID)
    return TRUE;      // standard behaviour

  if (newId == m_imageId)
    return TRUE;      // assumed fine!

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl       *pTree   = pTreeGD->GetPTree();

  ASSERT ((CTreeItem *)pTree->GetItemData(hItem) == this);

  int index = pTreeGD->AddBitmap(newId);
  ASSERT(index);
  if (!index)
    return FALSE;
  pTree->SetItemImage(hItem, index, index);
  m_imageId = newId;

  return TRUE;
}

BOOL CTreeItem::RefreshDisplayedItemsList(RefreshCause because)
{
  // forces refresh of displayed list of items
  // 2 cases depending on current branch type
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl       *pTree   = pTreeGD->GetPTree();

  HTREEITEM hItem = pTree->GetSelectedItem();
  ASSERT(hItem);
  if (!hItem)
    return FALSE;
  CTreeItem *pItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pItem);
  if (!pItem)
    return FALSE;

  CTreeItem *pGoodItem = pItem;
  HTREEITEM  hGoodItem = hItem;
  if (!pItem->IsStatic()) {
    hItem = pTree->GetParentItem(hItem);
    ASSERT(hItem);
    if (!hItem)
      return FALSE;
    pItem = (CTreeItem *)pTree->GetItemData(hItem);
    ASSERT (pItem);
    if (!pItem)
      return FALSE;
  }

  if (because == REFRESH_FORCE) {
    // refresh low level first
    void *pstructParent = NULL;   // in case no parent structure
    if (pItem->GetPTreeItemData())
      pstructParent = pItem->GetPTreeItemData()->GetDataPtr();
    CWaitCursor hourGlass;
    ItemUpdateInfo(pTreeGD->GetHNode(),  // hnodestruct,
                   pItem->GetType(),     // parent type
                   pstructParent,        // parent structure
                   pItem->GetChildType() // requested type
                   );
  }

  BOOL bStillExists = FALSE;
  BOOL bSuccess = pItem->RefreshDataSubBranches(hItem, pGoodItem, &bStillExists);
  if (bSuccess)
    if (bStillExists)   //example : item does not exist anymore if drop - might not exist anymore if force refresh
      bSuccess = pGoodItem->RefreshRelatedBranches(hGoodItem, because);
  return bSuccess;
}

BOOL CTreeItem::FilterDisplayedItemsList(HTREEITEM hItem, FilterCause because)
{
  ASSERT(hItem);
  if (!hItem)
    return FALSE;

  ASSERT (IsStatic());
  ASSERT (GetSubBK() == SUBBRANCH_KIND_OBJ || GetSubBK() == SUBBRANCH_KIND_MIX);

  BOOL bSuccess = TRUE;
  if (FilterApplies(because))
    bSuccess = DisplayDataSubBranches(hItem, DISPLAY_ACT_CHANGEFILTER);
  return bSuccess;
}

void CTreeItem::SetDisplayName(CString name, HTREEITEM hItem)
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl       *pTree   = pTreeGD->GetPTree();

  m_name = name;
  pTree->SetItemText(hItem, name);
}

BOOL CTreeItem::CompleteStruct()
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl       *pTree   = pTreeGD->GetPTree();

  if (m_bTypeCompleted)
    return TRUE;

  void *pstructParent = NULL;   // in case no parent
  if (GetPTreeItemData())
    pstructParent = GetPTreeItemData()->GetDataPtr();
  else
    return TRUE;    // Error item : act as if successful, but don't mark for m_bTypeCompleted

  int ret = ItemCompleteStruct(pTreeGD->GetHNode(),
                               pstructParent,
                               &m_type,
                               &m_completetype);
  if (ret == RES_SUCCESS) {
    m_bTypeCompleted = TRUE;
    return TRUE;
  }
  else
    return FALSE;
}


void CTreeItem::SetNoItem()
{
  m_itemKind = ITEM_KIND_NOITEM;
  SetSubBK(SUBBRANCH_KIND_NONE);    // Force to terminal line kind
}

void CTreeItem::SetErrorItem()
{
  m_itemKind = ITEM_KIND_ERROR;
  SetSubBK(SUBBRANCH_KIND_NONE);    // Force to terminal line kind
}

BOOL CTreeItem::CreateErrorItem(HTREEITEM hParentItem, int iobjecttypeReq, int res)
{
  return CreateSpecialItem(hParentItem, iobjecttypeReq, 0, res, ITEM_KIND_ERROR);
}

BOOL CTreeItem::CreateNoChildItem(HTREEITEM hParentItem, int iobjecttypeReq, UINT noChildStringId, CTreeItemData *pItemData)
{
  return CreateSpecialItem(hParentItem, iobjecttypeReq, noChildStringId, RES_SUCCESS, ITEM_KIND_NOITEM, pItemData);
}

BOOL CTreeItem::CreateSpecialItem(HTREEITEM hParentItem, int iobjecttypeReq, UINT stringId, int res, ItemKind itemKind, CTreeItemData *pItemData)
{
  // precheck parameters compatibility
  switch (itemKind) {
    case ITEM_KIND_NOITEM:
      ASSERT (res == RES_SUCCESS);
      ASSERT (stringId);
      ASSERT (pItemData);
      break;
    case ITEM_KIND_ERROR:
      ASSERT (res != RES_SUCCESS);
      ASSERT (!stringId);
      ASSERT (!pItemData);
      break;
    default:
      ASSERT(FALSE);
      return FALSE;
  }

  // allocate error item
  CTreeItem *pNewItem = AllocateChildItem(pItemData);    // Virtual
  if (!pNewItem)
    return FALSE;
  ASSERT (iobjecttypeReq == pNewItem->GetType());   // Check types compatibility

  // set item caption
  UINT id;
  switch(res) {
    case RES_ERR:
      id = IDS_TM_ERR_ERR;     break;
    case RES_TIMEOUT:
      id = IDS_TM_ERR_TIMEOUT; break;
    case RES_NOGRANT:
      id = IDS_TM_ERR_NOGRANT; break;
    case RES_NOT_OI:
      id = GetStringId_ResNotOi(); break;
    case RES_CANNOT_SOLVE:
      id = GetStringId_ResCannotSolve(); break;
    case RES_MULTIPLE_GRANTS:
      id = GetStringId_ResMultipleGrants(); break;
    default:
      id = stringId;           break;
  }
  CString pull;
  BOOL bSuccess = pull.LoadString(id);
  ASSERT (bSuccess);
  if (!bSuccess)
    pull = "< Error/No Item >";   // default message

  // set item name
  pNewItem->SetDisplayName(pull);
  // Set itemdata new field "m_name" for consistency
  if (pNewItem->GetPTreeItemData())
    pNewItem->GetPTreeItemData()->SetItemName(pull);

  // Explicitely declare as special item type -MANDATORY
  switch (itemKind) {
    case ITEM_KIND_NOITEM:
      pNewItem->SetNoItem();
      break;
    case ITEM_KIND_ERROR:
      pNewItem->SetErrorItem();
      break;
  }

  // create tree line
  HTREEITEM hNewChild = pNewItem->CreateTreeLine(hParentItem);
  ASSERT (hNewChild);
  if (!hNewChild)
    return FALSE;
  return TRUE;
}


CuPageInformation* CTreeItem::GetPageInformation ()
{
    if (m_pPageInfo)
        return m_pPageInfo;

    m_pPageInfo = new CuPageInformation ((LPCTSTR)"CTreeItem", 0, NULL, NULL);
    return m_pPageInfo;
}

BOOL CTreeItem::RefreshRelatedBranches(HTREEITEM hItem, RefreshCause because)
{
  // Nothing to do!
  return TRUE;
}

BOOL CTreeItem::FilterApplies(FilterCause because)
{
  // Must be static
  ASSERT (IsStatic());

  // Must have children
  ASSERT (GetSubBK() == SUBBRANCH_KIND_OBJ || GetSubBK() == SUBBRANCH_KIND_MIX);

  // Nothing to do at base level
  return FALSE;
}

BOOL CTreeItem::TreeKillShutdown()
{
  BOOL bResult = KillShutdown();  // virtual call
  if (bResult) {
    //bResult = RefreshDisplayedItemsList(REFRESH_KILLSHUTDOWN);
    int hNode = m_pTreeGD->GetHNode();
    if (hNode != -1) {
      CWaitCursor hourGlass;
      UpdateMonInfo(hNode, 0, NULL, OT_MON_ALL);
      SetMonNormalState(hNode);     // Mandatory BEFORE UpdateMonDisplay() otherwise stack overflow
      UpdateMonDisplay(hNode, 0, NULL, OT_MON_ALL);
    }
  }
  return bResult;
}

BOOL CTreeItem::TreeCloseServer()
{
  BOOL bResult = CloseServer();   // virtual call
  return bResult;
}

BOOL CTreeItem::TreeOpenServer()
{
  BOOL bResult = OpenServer();    // virtual call
  return bResult;
}

void CTreeItem::DeleteAllChildrenDataOfItem(HTREEITEM hParentItem, BOOL bIncludingCurrent)
{
  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl       *pTree   = pTreeGD->GetPTree();

  HTREEITEM hChildItem = pTree->GetChildItem(hParentItem);
  while (hChildItem) {
    DeleteAllChildrenDataOfItem(hChildItem, TRUE);
    hChildItem = pTree->GetNextSiblingItem(hChildItem);
  }

  if (bIncludingCurrent) {
    CTreeItem *pItem;
    pItem = (CTreeItem *)pTree->GetItemData(hParentItem);
    if (pItem)
      delete pItem;
  }
}


// branches expandability according to filter
void CTreeItem::MakeBranchNotExpandable(HTREEITEM hItem)
{
  if (HasSubBranches()) {
    // delete all sub branches data, excluding current item data
    // otherwise "this" not valid anymore
    DeleteAllChildrenDataOfItem(hItem, FALSE);

    // delete all current item's immediate children, if any
    // NOTE: can already have no immediate children at all (ex: force refresh)
    CTreeGlobalData *pTreeGD = GetPTreeGlobData();
    CTreeCtrl       *pTree   = pTreeGD->GetPTree();
    HTREEITEM hChildItem = pTree->GetChildItem(hItem);
    while (hChildItem) {
      HTREEITEM hNextChildItem = pTree->GetNextSiblingItem(hChildItem);
      pTree->DeleteItem(hChildItem);
      hChildItem = hNextChildItem;
    }

    // set as never expanded
    SetAlreadyExpanded(FALSE);
  }
}

void CTreeItem::MakeBranchExpandable(HTREEITEM hItem)
{
  if (HasSubBranches()) {
    CTreeGlobalData *pTreeGD = GetPTreeGlobData();
    CTreeCtrl       *pTree   = pTreeGD->GetPTree();

    // do nothing if branch has at least a child
    HTREEITEM hChildItem = pTree->GetChildItem(hItem);
    if (hChildItem)
      return;

    HTREEITEM hDummyItem;
    hDummyItem = pTree->InsertItem("Dummy", hItem);
    ASSERT(hDummyItem);
    if (!hDummyItem) {
      ASSERT (FALSE);
      return;
    }
    BOOL bSuccess = pTree->SetItemData(hDummyItem, 0);  // Flags dummy item
    if (!bSuccess) {
      ASSERT (FALSE);
      return;
    }
    SetAlreadyExpanded(FALSE);
    pTree->InvalidateRect(NULL, FALSE);    // force redraw for '+' sign
  }
}


//
// special methods to be able to call MonXYZ or DomXYZ functions
//
int  CTreeItem::ItemStructSize(int iobjecttype)
{
  return GetMonInfoStructSize(iobjecttype);
}

int  CTreeItem::ItemGetFirst(int hnodestruct, int iobjecttypeParent, void* pstructParent, int iobjecttypeReq, void* pstructReq, LPSFILTER pFilter, CString &rcsItemName)
{
  rcsItemName = "";
  int retval = GetFirstMonInfo(hnodestruct, iobjecttypeParent, pstructParent, iobjecttypeReq, pstructReq, pFilter);
  if (retval == RES_SUCCESS) {
    char	name[MAXMONINFONAME];
    name[0] = '\0';
    GetMonInfoName(hnodestruct, iobjecttypeReq, pstructReq, name, sizeof(name));
    ASSERT (name[0]);
    rcsItemName = name;
  }
  return retval;
}

int  CTreeItem::ItemGetNext(int hnodestruct, int iobjecttypeReq, void* pstructParent, void *pstructReq, CString &rcsItemName, int iobjecttypeParent)
{
  rcsItemName = "";
  int retval = GetNextMonInfo(pstructReq);
  if (retval == RES_SUCCESS) {
    char	name[MAXMONINFONAME];
    name[0] = '\0';
    GetMonInfoName(hnodestruct, iobjecttypeReq, pstructReq, name, sizeof(name));
    ASSERT (name[0]);
    rcsItemName = name;
  }
  return retval;
}

int  CTreeItem::ItemCompare(int iobjecttype, void *pstruct1, void *pstruct2)
{
  return CompareMonInfo(iobjecttype, pstruct1, pstruct2);
}

BOOL CTreeItem::ItemFitWithFilter(int hNode, int iobjtype,void * pstructReq,LPSFILTER pfilter)
{
  // hNode not used in MonXyz() function
  return MonFitWithFilter(iobjtype,pstructReq,pfilter);
}

void CTreeItem::ItemUpdateInfo(int hnodestruct,int iobjecttypeParent, void *pstructParent, int iobjecttypeReq)
{
  UpdateMonInfo(hnodestruct,iobjecttypeParent, pstructParent, iobjecttypeReq);
}

int  CTreeItem::ItemCompleteStruct(int hnode, void *pstruct, int* poldtype, int *pnewtype)
{
  return MonCompleteStruct(hnode, pstruct, poldtype, pnewtype);
}

BOOL CTreeItem::CanBeExpanded()
{
  if (!HasSubBranches())
    return FALSE;

  if (IsSpecialItem())
    return FALSE;

  if (!IsStatic())
    return FALSE;

  return TRUE;
}
