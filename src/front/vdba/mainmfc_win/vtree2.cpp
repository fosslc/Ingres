/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vtree2.cpp: 
**    Author  : Emmanuel Blattes 
**
** History (after 26-Sep-2000)
** 
** 26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Select the input database if specified.
*/

//
// Class CTreeItemNodes : Nodes management
// serves as a base class for nodes management window
//

#include "stdafx.h"

#include "vtree2.h"

#include "vnitem.h"   // Runtime classes

extern "C" {
#include "dba.h"
};

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// declaration for serialization
IMPLEMENT_SERIAL ( CTreeItemNodes, CTreeItemAAD, 2)

// constructor for static item
CTreeItemNodes::CTreeItemNodes(CTreeGlobalData *pTreeGD, int type, UINT nameId, SubBK subBranchKind, UINT imageId, CTreeItemData *pItemData, int childtype, UINT noChildStringId)
 : CTreeItemAAD(pTreeGD, type, nameId, subBranchKind, imageId, pItemData, childtype, noChildStringId)
{
}

// constructor for non-static item
CTreeItemNodes::CTreeItemNodes(CTreeGlobalData *pTreeGD, int type, SubBK subBranchKind, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype, UINT noChildStringId)
  : CTreeItemAAD(pTreeGD, type, subBranchKind, bNeedsTypeSolve, pItemData, childtype, noChildStringId)
{
}

// serialization constructor
CTreeItemNodes::CTreeItemNodes()
{
}

// virtual destructor
CTreeItemNodes::~CTreeItemNodes()
{
}

// serialization method
void CTreeItemNodes::Serialize (CArchive& ar)
{
  if (ar.IsStoring()) {
  }
  else {
  }

  CTreeItemAAD::Serialize(ar);
}

BOOL CTreeItemNodes::TreeConnect()
{
  BOOL bResult = Connect();  // virtual call
  if (bResult) {
    bResult = RefreshDisplayedItemsList(REFRESH_CONNECT);  // will update nodes icons
    if (bResult)
      bResult = RefreshDisplayedOpenWindowsList();
  }
  return bResult;
}

BOOL CTreeItemNodes::TreeSqlTest(LPCTSTR lpszDatabase)
{
  BOOL bResult = SqlTest(lpszDatabase);  // virtual call
  if (bResult) {
    bResult = RefreshDisplayedItemsList(REFRESH_SQLTEST);  // will update nodes icons
    if (bResult)
      bResult = RefreshDisplayedOpenWindowsList();
  }
  return bResult;
}

BOOL CTreeItemNodes::TreeMonitor()
{
  BOOL bResult = Monitor();  // virtual call
  if (bResult) {
    bResult = RefreshDisplayedItemsList(REFRESH_MONITOR);  // will update nodes icons
    if (bResult)
      bResult = RefreshDisplayedOpenWindowsList();
  }
  return bResult;
}

BOOL CTreeItemNodes::TreeDbevent(LPCTSTR lpszDatabase)
{
  BOOL bResult = Dbevent(lpszDatabase);  // virtual call
  if (bResult) {
    bResult = RefreshDisplayedItemsList(REFRESH_DBEVENT);  // will update nodes icons
    if (bResult)
      bResult = RefreshDisplayedOpenWindowsList();
  }
  return bResult;
}

BOOL CTreeItemNodes::TreeDisconnect()
{
  BOOL bResult = Disconnect();  // virtual call
  if (bResult) {
    bResult = RefreshDisplayedItemsList(REFRESH_DISCONNECT);  // will update nodes icons
    if (bResult)
      bResult = RefreshDisplayedOpenWindowsList();
  }
  return bResult;
}

BOOL CTreeItemNodes::TreeCloseWin()
{
  BOOL bResult = CloseWin();      // virtual call
  if (bResult) {
    bResult = RefreshDisplayedItemsList(REFRESH_CLOSEWIN);  // will update windows list
    if (bResult)
      UpdateNodeDisplay();    // simpliest way to update all related openwins
  }
  return bResult;
}

BOOL CTreeItemNodes::TreeActivateWin()
{
  return ActivateWin();   // virtual call
}

BOOL CTreeItemNodes::RefreshDisplayedOpenWindowsList()
{
  // update sub-branch "Open windows" of branch
  // assumed to be the current selection
  BOOL bSuccess = TRUE;

  CTreeGlobalData *pTreeGD = GetPTreeGlobData();
  CTreeCtrl *pTree = pTreeGD->GetPTree();

  // get hItem and check against current selection
  HTREEITEM hItem = pTree->GetSelectedItem();
  CTreeItem *pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
  ASSERT (pTreeItem == this);
  if (pTreeItem != this)
    return FALSE;

  // get children branches until static branch "Opened Windows" found,
  // only if already expanded
  if (pTreeItem->IsAlreadyExpanded()) {
    HTREEITEM hChildItem = pTree->GetChildItem(hItem);
    while (hChildItem) {
      pTreeItem = (CTreeItem *)pTree->GetItemData(hChildItem);
      if (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN)
        break;
      hChildItem = pTree->GetNextSiblingItem(hChildItem);
    }
    ASSERT (hChildItem);
    bSuccess = pTreeItem->RefreshDataSubBranches(hChildItem);
  }

  // Complimentary:
  // - if we are on a server branch, also refresh open windows for parent node branch
  // - else: also refresh open windows for all server branches whose subbranch "open windows" is expanded
  ASSERT (this->IsKindOf(RUNTIME_CLASS(CuTreeSvrClass)) ||
          this->IsKindOf(RUNTIME_CLASS(CuTreeServer))   ||
          this->IsKindOf(RUNTIME_CLASS(CuTreeUser))     ||
          this->IsKindOf(RUNTIME_CLASS(CuTreeSvrUser))  );

  if (this->IsKindOf(RUNTIME_CLASS(CuTreeSvrClass)) ||
      this->IsKindOf(RUNTIME_CLASS(CuTreeSvrUser))  ||
      this->IsKindOf(RUNTIME_CLASS(CuTreeUser)) ) {

    if (this->IsKindOf(RUNTIME_CLASS(CuTreeSvrUser)) ) {
      hItem = pTree->GetParentItem(hItem);  // svrclass user static
      hItem = pTree->GetParentItem(hItem);  // svrclass

      CuTreeServer* pTSV = (CuTreeServer*)pTree->GetItemData(hItem);
      ASSERT (pTSV);
      ASSERT (pTSV->IsKindOf(RUNTIME_CLASS(CuTreeSvrClass)));
      // get children branches until static branch "Opened Windows" found
      HTREEITEM hChildItem = pTree->GetChildItem(hItem);
      while (hChildItem) {
        pTreeItem = (CTreeItem *)pTree->GetItemData(hChildItem);
        if (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN)
          break;
        hChildItem = pTree->GetNextSiblingItem(hChildItem);
      }
      ASSERT (hChildItem);
      BOOL bSuccess2 = pTreeItem->RefreshDataSubBranches(hChildItem);
      bSuccess = bSuccess && bSuccess2;
    }

    if (this->IsKindOf(RUNTIME_CLASS(CuTreeSvrClass)) ) {
      // Must also refresh Svr users openwins
      pTreeItem = (CTreeItem *)pTree->GetItemData(hItem);
      if (pTreeItem->IsAlreadyExpanded()) {
        HTREEITEM hUsers = pTree->GetChildItem(hItem);  // static "Users" first child
        // Sub branches "Users" of Server Class
        pTreeItem = (CTreeItem *)pTree->GetItemData(hUsers);
        if (pTreeItem->IsAlreadyExpanded()) {
          HTREEITEM hUser;
          for (hUser = pTree->GetChildItem(hUsers);
               hUser;
               hUser = pTree->GetNextSiblingItem(hUser)) {
            pTreeItem = (CTreeItem *)pTree->GetItemData(hUser);
            pTreeItem->ChangeImageId(pTreeItem->GetCustomImageId(), hUser);
            if (pTreeItem->IsAlreadyExpanded()) {
              HTREEITEM hOpenWins = pTree->GetChildItem(hUser);
              pTreeItem = (CTreeItem *)pTree->GetItemData(hOpenWins);
              ASSERT (pTreeItem);
              ASSERT (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN);
              BOOL bSuccess2 = pTreeItem->RefreshDataSubBranches(hOpenWins);
              bSuccess = bSuccess && bSuccess2;
            }
          }
        }
      }
    }

    hItem = pTree->GetParentItem(hItem);  // svrclassstatic or userstatic
    hItem = pTree->GetParentItem(hItem);  // node

    CuTreeServer* pTSV = (CuTreeServer*)pTree->GetItemData(hItem);
    ASSERT (pTSV);
    ASSERT (pTSV->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
    // get children branches until static branch "Opened Windows" found
    HTREEITEM hChildItem = pTree->GetChildItem(hItem);
    while (hChildItem) {
      pTreeItem = (CTreeItem *)pTree->GetItemData(hChildItem);
      if (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN)
        break;
      hChildItem = pTree->GetNextSiblingItem(hChildItem);
    }
    ASSERT (hChildItem);
    BOOL bSuccess2 = pTreeItem->RefreshDataSubBranches(hChildItem);
    bSuccess = bSuccess && bSuccess2;
  }
  else {
    ASSERT (this->IsKindOf(RUNTIME_CLASS(CuTreeServer)));
    if (pTreeItem->IsAlreadyExpanded()) {
      // 1) refresh servers openwins lists
      HTREEITEM hServers = pTree->GetChildItem(hItem);
      pTreeItem = (CTreeItem *)pTree->GetItemData(hServers);
      if (pTreeItem->IsAlreadyExpanded()) {
        HTREEITEM hServer;
        for (hServer = pTree->GetChildItem(hServers);
             hServer;
             hServer = pTree->GetNextSiblingItem(hServer)) {
          pTreeItem = (CTreeItem *)pTree->GetItemData(hServer);
          if (pTreeItem->IsAlreadyExpanded()) {
            HTREEITEM hUsers = pTree->GetChildItem(hServer);          // static "Users"
            HTREEITEM hOpenWins = pTree->GetNextSiblingItem(hUsers);  // static "Openwins"
            pTreeItem = (CTreeItem *)pTree->GetItemData(hOpenWins);
            ASSERT (pTreeItem);
            ASSERT (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN);
            BOOL bSuccess2 = pTreeItem->RefreshDataSubBranches(hOpenWins);
            bSuccess = bSuccess && bSuccess2;

            // Sub branches "Users" of Server Class
            pTreeItem = (CTreeItem *)pTree->GetItemData(hUsers);
            if (pTreeItem->IsAlreadyExpanded()) {
              HTREEITEM hUser;
              for (hUser = pTree->GetChildItem(hUsers);
                   hUser;
                   hUser = pTree->GetNextSiblingItem(hUser)) {
                pTreeItem = (CTreeItem *)pTree->GetItemData(hUser);
                pTreeItem->ChangeImageId(pTreeItem->GetCustomImageId(), hUser);
                if (pTreeItem->IsAlreadyExpanded()) {
                  HTREEITEM hOpenWins = pTree->GetChildItem(hUser);
                  pTreeItem = (CTreeItem *)pTree->GetItemData(hOpenWins);
                  ASSERT (pTreeItem);
                  ASSERT (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN);
                  bSuccess2 = pTreeItem->RefreshDataSubBranches(hOpenWins);
                  bSuccess = bSuccess && bSuccess2;
                }
              }
            }
          }
        }
      }

      // 2) refresh users openwins lists
      HTREEITEM hUsers = pTree->GetNextSiblingItem(hServers);
      pTreeItem = (CTreeItem *)pTree->GetItemData(hUsers);
      if (pTreeItem->IsAlreadyExpanded()) {
        HTREEITEM hUser;
        for (hUser = pTree->GetChildItem(hUsers);
             hUser;
             hUser = pTree->GetNextSiblingItem(hUser)) {
          pTreeItem = (CTreeItem *)pTree->GetItemData(hUser);
          if (pTreeItem->IsAlreadyExpanded()) {
            HTREEITEM hOpenWins = pTree->GetChildItem(hUser);
            pTreeItem = (CTreeItem *)pTree->GetItemData(hOpenWins);
            ASSERT (pTreeItem);
            ASSERT (pTreeItem->IsStatic() && pTreeItem->GetChildType()==OT_NODE_OPENWIN);
            BOOL bSuccess2 = pTreeItem->RefreshDataSubBranches(hOpenWins);
            bSuccess = bSuccess && bSuccess2;
          }
        }
      }
    }
  }
  return bSuccess;
}

BOOL CTreeItemNodes::TreeScratchpad()
{
  BOOL bResult = Scratchpad();  // virtual call
  if (bResult) {
    bResult = RefreshDisplayedItemsList(REFRESH_CONNECT);  // will update nodes icons
    if (bResult)
      bResult = RefreshDisplayedOpenWindowsList();
  }
  return bResult;
}

BOOL CTreeItemNodes::TreeAdd()
{
  BOOL bResult = Add();       // virtual call
  if (bResult)
    UpdateNodeDisplay();
  return bResult;
}

BOOL CTreeItemNodes::TreeAlter()
{
  BOOL bResult = Alter();     // virtual call
  if (bResult)
    UpdateNodeDisplay();
  return bResult;
}

BOOL CTreeItemNodes::TreeDrop()
{
  BOOL bResult = Drop();      // virtual call
  if (bResult)
    UpdateNodeDisplay();
  return bResult;
}

