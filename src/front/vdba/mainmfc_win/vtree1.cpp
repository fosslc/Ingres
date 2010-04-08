// vTree1.cpp - Emmanuel Blattes - started Feb. 11, 97
// 

//
// Class CTreeItemAAD : AAD means AddAlterDelete
// Intermediary class for future use in mfc doms
//

#include "stdafx.h"

#include "vtree1.h"

extern "C" {
#include "dba.h"
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// declaration for serialization
IMPLEMENT_SERIAL ( CTreeItemAAD, CTreeItem, 2)

// constructor for static item
CTreeItemAAD::CTreeItemAAD(CTreeGlobalData *pTreeGD, int type, UINT nameId, SubBK subBranchKind, UINT imageId, CTreeItemData *pItemData, int childtype, UINT noChildStringId)
 : CTreeItem(pTreeGD, type, nameId, subBranchKind, imageId, pItemData, childtype, noChildStringId)
{
}

// constructor for non-static item
CTreeItemAAD::CTreeItemAAD(CTreeGlobalData *pTreeGD, int type, SubBK subBranchKind, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype, UINT noChildStringId)
  : CTreeItem(pTreeGD, type, subBranchKind, bNeedsTypeSolve, pItemData, childtype, noChildStringId)
{
}

// serialization constructor
CTreeItemAAD::CTreeItemAAD()
{
}

// virtual destructor
CTreeItemAAD::~CTreeItemAAD()
{
}

// serialization method
void CTreeItemAAD::Serialize (CArchive& ar)
{
  if (ar.IsStoring()) {
  }
  else {
  }

  CTreeItem::Serialize(ar);
}

BOOL CTreeItemAAD::TreeAdd()
{
  BOOL bResult = Add();       // virtual call
  if (bResult)
    bResult = RefreshDisplayedItemsList(REFRESH_ADD);
  return bResult;
}

BOOL CTreeItemAAD::TreeAlter()
{
  BOOL bResult = Alter();     // virtual call
  if (bResult)
    bResult = RefreshDisplayedItemsList(REFRESH_ALTER);
  return bResult;
}

BOOL CTreeItemAAD::TreeDrop()
{
  BOOL bResult = Drop();      // virtual call
  if (bResult)
    bResult = RefreshDisplayedItemsList(REFRESH_DROP);
  return bResult;
}

