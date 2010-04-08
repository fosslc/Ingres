// vTree3.cpp - Emmanuel Blattes - started Oct. 20, 97
// 

//
// Class CTreeItemStar : Star means Star
// Intermediary class for use in "Register as link" dialog boxes
//

#include "stdafx.h"

#include "vtree3.h"
#include "vnitem.h"

extern "C" {
#include "monitor.h"
#include "dba.h"
#include "domdata.h"      // DOMGetFirst/Next
};


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
//  Utility functions
//
// QUESTION: transform into virtual member fct: to be taken in account if star manages more objects types in the future.
static void FillParenthood(void* pstructReq, void* pstructParent, int iobjecttypeParent)
{
  ASSERT (pstructReq);
  ASSERT (pstructParent);

  LPSTARITEM pStarReq = (LPSTARITEM)pstructReq;
  LPSTARITEM pStarParent = 0;
  LPNODEDATAMIN pNodeData = 0;
  LPNODESVRCLASSDATAMIN pServerDataMin = 0;

  switch (pStarReq->ObjType) {
    case OT_DATABASE:
      switch (iobjecttypeParent) {
        case OT_NODE:
          pNodeData = (LPNODEDATAMIN)pstructParent;
          ASSERT (pNodeData);
          x_strcpy((char *)pStarReq->NodeName, (LPCTSTR)pNodeData->NodeName);
          break;
        case OT_NODE_SERVERCLASS:
          pServerDataMin = (LPNODESVRCLASSDATAMIN)pstructParent;
          ASSERT (pServerDataMin);
          x_strcpy((char *)pStarReq->NodeName, (LPCTSTR)BuildFullNodeName(pServerDataMin));
          break;
        default:
          ASSERT (FALSE);
          pStarReq->NodeName[0] = '\0';
          break;
      }
      break;

    case OT_TABLE:
    case OT_VIEW:
    case OT_PROCEDURE:
      pStarParent = (LPSTARITEM)pstructParent;
      x_strcpy((char *)pStarReq->parent0, (LPCTSTR)pStarParent->objName);
      x_strcpy((char *)pStarReq->NodeName, (LPCTSTR)pStarParent->NodeName);
      break;

    default:
      ASSERT (FALSE);
  }
}

// declaration for serialization
IMPLEMENT_SERIAL ( CTreeItemStar, CTreeItem, 2)

// constructor for static item
CTreeItemStar::CTreeItemStar(CTreeGlobalData *pTreeGD, int type, UINT nameId, SubBK subBranchKind, UINT imageId, CTreeItemData *pItemData, int childtype, UINT noChildStringId)
 : CTreeItem(pTreeGD, type, nameId, subBranchKind, imageId, pItemData, childtype, noChildStringId)
{
}

// constructor for non-static item
CTreeItemStar::CTreeItemStar(CTreeGlobalData *pTreeGD, int type, SubBK subBranchKind, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype, UINT noChildStringId)
  : CTreeItem(pTreeGD, type, subBranchKind, bNeedsTypeSolve, pItemData, childtype, noChildStringId)
{
}

// serialization constructor
CTreeItemStar::CTreeItemStar()
{
}

// virtual destructor
CTreeItemStar::~CTreeItemStar()
{
}

// serialization method
void CTreeItemStar::Serialize (CArchive& ar)
{
  if (ar.IsStoring()) {
  }
  else {
  }

  CTreeItem::Serialize(ar);
}


//
// special methods to be able to call MonXYZ or DomXYZ functions
//
int  CTreeItemStar::ItemStructSize(int iobjecttype)
{
  if (iobjecttype == OT_NODE)
    return GetMonInfoStructSize(iobjecttype);
  else
    return sizeof (STARITEM);
}

int  CTreeItemStar::ItemGetFirst(int hnodestruct, int iobjecttypeParent, void* pstructParent, int iobjecttypeReq, void* pstructReq, LPSFILTER pFilter, CString &rcsItemName)
{
  // Default value
  rcsItemName = "";

  // Preliminary test, otherwise DOMGetFirstObject/GetFirstMonInfo display a "system error 3" unexpected and misunderstandable message
  if (hnodestruct < 0)
    return RES_ERR;

  int retval;
  if (iobjecttypeReq == OT_NODE || iobjecttypeReq == OT_NODE_SERVERCLASS) {
    retval = GetFirstMonInfo(hnodestruct, iobjecttypeParent, pstructParent, iobjecttypeReq, pstructReq, pFilter);
  }
  else {
    LPSTARITEM pStar = (LPSTARITEM)pstructReq;

    // fill input fields in structure
    pStar->ObjType = iobjecttypeReq;
    FillParenthood(pStar, pstructParent, iobjecttypeParent);  // virtual
    LPUCHAR aParents[3];
    aParents[0] = pStar->parent0;
    aParents[1] = pStar->parent1;
    aParents[2] = pStar->parent2;

    // call DomGetXYZ
    retval = DOMGetFirstObject(hnodestruct,
                               iobjecttypeReq,
                               GetParentLevelFromObjType(iobjecttypeReq),
                               aParents,
                               NULL,       // No filter on owner
                               FALSE,      //  Not with system
                               pStar->objName,
                               pStar->ownerName,
                               pStar->szComplim);
    // STAR specificity : CDBs considered as system, EXCEPT...
    // ...EXCEPT for "Local Table" definition when creating object at star level
    // Exception managed by virtual method DoNotShowCDBs() defaulting to TRUE, derived for CuTreeStar4LocalTblServer as FALSE
    if (iobjecttypeReq == OT_DATABASE) {
      if (DoNotShowCDBs()) {
        while (retval == RES_SUCCESS) {
          int databaseType = getint(pStar->szComplim+STEPSMALLOBJ);
          if (databaseType != DBTYPE_COORDINATOR)
            break;
          retval = DOMGetNextObject(pStar->objName, pStar->ownerName, pStar->szComplim);
        }
      }
    }
  }

  // generate name to be displayed
  if (retval == RES_SUCCESS) {
    // Virtual method
    rcsItemName = MakeChildItemDisplayName(hnodestruct, iobjecttypeReq, pstructReq);
  }
  return retval;
}

int  CTreeItemStar::ItemGetNext(int hnodestruct, int iobjecttypeReq, void* pstructParent, void *pstructReq, CString &rcsItemName, int iobjecttypeParent)
{
  // Default value
  rcsItemName = "";

  int retval;
  if (iobjecttypeReq == OT_NODE || iobjecttypeReq == OT_NODE_SERVERCLASS) {
    retval = GetNextMonInfo(pstructReq);
  }
  else {
    LPSTARITEM pStar = (LPSTARITEM)pstructReq;

    // fill input fields in structure
    pStar->ObjType = iobjecttypeReq;
    FillParenthood(pstructReq, pstructParent, iobjecttypeParent);  // virtual
    LPUCHAR aParents[3];
    aParents[0] = pStar->parent0;
    aParents[1] = pStar->parent1;
    aParents[2] = pStar->parent2;

    // call DomGetXYZ
    retval = DOMGetNextObject(pStar->objName, pStar->ownerName, pStar->szComplim);

    // STAR specificity : CDBs considered as system, EXCEPT...
    // ...EXCEPT for "Local Table" definition when creating object at star level
    // Exception managed by virtual method DoNotShowCDBs() defaulting to TRUE, derived for CuTreeStar4LocalTblServer as FALSE
    if (iobjecttypeReq == OT_DATABASE) {
      if (DoNotShowCDBs()) {
        while (retval == RES_SUCCESS) {
          int databaseType = getint(pStar->szComplim+STEPSMALLOBJ);
          if (databaseType != DBTYPE_COORDINATOR)
            break;
          retval = DOMGetNextObject(pStar->objName, pStar->ownerName, pStar->szComplim);
        }
      }
    }

  }

  // generate name to be displayed
  if (retval == RES_SUCCESS) {
    // Virtual method
    rcsItemName = MakeChildItemDisplayName(hnodestruct, iobjecttypeReq, pstructReq);
  }
  return retval;
}

int  CTreeItemStar::ItemCompare(int iobjecttype, void *pstruct1, void *pstruct2)
{
  if (iobjecttype == OT_NODE)
    return CompareMonInfo(iobjecttype, pstruct1, pstruct2);
  else {
    // ASSERT (FALSE);
    return -1;    // Assume "bigger" than previous - TO BE ENHANCED IF NECESSARY
  }
}

BOOL CTreeItemStar::ItemFitWithFilter(int hNode, int iobjtype,void * pstructReq,LPSFILTER pfilter)
{
  if (iobjtype == OT_NODE)
    return MonFitWithFilter(iobjtype,pstructReq,pfilter);
  else if (iobjtype == OT_DATABASE) {
    // Steve ball says ddbs not eligible
    LPSTARITEM pStar = (LPSTARITEM)pstructReq;
    ASSERT (pStar);
    int databaseType = getint(pStar->szComplim+STEPSMALLOBJ);
    switch (databaseType) {
      case DBTYPE_REGULAR    :
        return TRUE;
      case DBTYPE_DISTRIBUTED:
        return FALSE;
      case DBTYPE_COORDINATOR:
        return FALSE;
      default:
        ASSERT (FALSE);
        return FALSE;
    }
  }
  else
    return TRUE;    // say it fits: system objects filter already done
}

void CTreeItemStar::ItemUpdateInfo(int hnodestruct,int iobjecttypeParent, void *pstructParent, int iobjecttypeReq)
{
  if (iobjecttypeReq == OT_NODE)
    UpdateMonInfo(hnodestruct,iobjecttypeParent, pstructParent, iobjecttypeReq);
  else {
    // Should not be called for star items
    ASSERT (FALSE);
  }
}

int  CTreeItemStar::ItemCompleteStruct(int hnode, void *pstruct, int* poldtype, int *pnewtype)
{
  if (*poldtype == OT_NODE)
    return MonCompleteStruct(hnode, pstruct, poldtype, pnewtype);
  else {
    return RES_SUCCESS;   // Nothing to do!
  }
}



/*********************

  OBSOLETE :

HTREEITEM CTreeItemStar::FindItemInTree(CTreeItemStar *pSearchedItem)
{
  CTreeGlobalData *pGD  = GetPTreeGlobData();
  ASSERT (pGD);
  CTreeCtrl *pTreeCtrl = pGD->GetPTree();
  ASSERT (pTreeCtrl);

  // scan each root branch
  HTREEITEM hItem;
  hItem = pTreeCtrl->GetRootItem();
  while (hItem) {
    HTREEITEM hFoundItem = FindItemInTreeBranch(hItem, pSearchedItem);
    if (hFoundItem)
      return hFoundItem;
    hItem = pTreeCtrl->GetNextSiblingItem(hItem);
  }

  ASSERT (FALSE);
  return NULL;
}


HTREEITEM CTreeItemStar::FindItemInTreeBranch(HTREEITEM hItem, CTreeItemStar *pSearchedItem)
{
  CTreeGlobalData *pGD  = GetPTreeGlobData();
  ASSERT (pGD);
  CTreeCtrl *pTreeCtrl = pGD->GetPTree();
  ASSERT (pTreeCtrl);

  // check against current item
  CTreeItemStar* pItemData = (CTreeItemStar*)pTreeCtrl->GetItemData(hItem);
  // Don't assert on pItemData nullity, since dummy subbranches don't have data yet!
  if (pItemData == pSearchedItem)
    return hItem;

  // check sub-branches
  HTREEITEM hChildItem;
  hChildItem = pTreeCtrl->GetChildItem(hItem);
  while (hChildItem) {
    HTREEITEM hFoundItem = FindItemInTreeBranch(hChildItem, pSearchedItem);
    if (hFoundItem)
      return hFoundItem;
    hChildItem = pTreeCtrl->GetNextSiblingItem(hChildItem);
  }
  return NULL;
}
*****************/
