// nodefast.h   : implementation file for fast access to an item in the nodes window,
//                for unicenter-driven feature

#include "stdafx.h"

#include "mainmfc.h"
#include "mainfrm.h"

#include "nodefast.h"
#include "monitor.h"


//
// Forward definitions
//
static HTREEITEM FindSearchedItem(CTreeCtrl* pNodeTree, CuNodeTreeFastItem* pFastItem, HTREEITEM hAnchorItem);

////////////////////////////////////////////////////////////////////////////////////////////
// Class CuNodeTreeFastItem

// IMPLEMENT_SERIAL (CuNodeTreeFastItem, CObject, 1)

CuNodeTreeFastItem::CuNodeTreeFastItem(BOOL bStatic, int type, LPCTSTR lpszName)
{
  m_bStatic = bStatic;
  m_type    = type;
  if (!bStatic)
    ASSERT (lpszName);
  if (lpszName)
    m_csName = lpszName;
}

CuNodeTreeFastItem::~CuNodeTreeFastItem()
{
}


////////////////////////////////////////////////////////////////////////////////////////////
// Functions

BOOL ExpandAndSelectUpToSearchedItem(CMainFrame* pMainFrame, CTypedPtrList<CObList, CuNodeTreeFastItem*>& rNodeTreeFastItemList)
{
  ASSERT (pMainFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)));

  // Get nodes tree pointer
  CTreeCtrl* pNodeTree = pMainFrame->GetVNodeTreeCtrl();
  ASSERT (pNodeTree);

  // ALWAYS: Set selection on root item
  pNodeTree->SelectItem(pNodeTree->GetRootItem());

  // Get selected item handle
  HTREEITEM hItem = pNodeTree->GetSelectedItem();
  ASSERT (hItem);

  // Loop on fastitemlist
  POSITION pos = rNodeTreeFastItemList.GetHeadPosition();
  while (pos) {
    CuNodeTreeFastItem* pFastItem = rNodeTreeFastItemList.GetNext (pos);
    hItem = FindSearchedItem(pNodeTree, pFastItem, hItem);
    if (!hItem) {
      ASSERT (FALSE);
      return FALSE;
    }
  }
  pNodeTree->SelectItem(hItem);
  return TRUE;
}

static HTREEITEM FindSearchedItem(CTreeCtrl* pNodeTree, CuNodeTreeFastItem* pFastItem, HTREEITEM hAnchorItem)
{
  // pîck Fast Item relevant data
  BOOL    bReqStatic = pFastItem->GetBStatic();
  int     reqType    = pFastItem->GetType();
  CString csReqName  = pFastItem->GetName();
  if (!bReqStatic)
    ASSERT (!csReqName.IsEmpty());  // Need name if not static

  // 1) expand
  BOOL bSuccess = pNodeTree->Expand(hAnchorItem, TVE_EXPAND);
  ASSERT (bSuccess);

  // 2) scan children until found
  HTREEITEM hChildItem;
  hChildItem = pNodeTree->GetChildItem(hAnchorItem);
  ASSERT (hChildItem);
  while (hChildItem) {
    CTreeItemNodes* pItem = (CTreeItemNodes*)pNodeTree->GetItemData(hChildItem);
    ASSERT (pItem);
    if (pItem) {
      ASSERT (pItem->IsKindOf(RUNTIME_CLASS(CTreeItemNodes)));
      if (bReqStatic) {
        if (pItem->GetChildType() == reqType) {
          return hChildItem;
        }
      }
      else {
        if (pItem->GetType() == reqType) {
          CTreeItemData* pData = pItem->GetPTreeItemData();
          ASSERT (pData);
          if (pData) {
            void* pStruct = pData->GetDataPtr();
            ASSERT (pStruct);
            if (pStruct) {
              char zeBuffer[MAXOBJECTNAME];
              GetMonInfoName(0, reqType, pStruct, zeBuffer, sizeof(zeBuffer));
              if (csReqName == zeBuffer)
                return hChildItem;
            }
          }
        }
      }
    }
    hChildItem = pNodeTree->GetNextSiblingItem(hChildItem);
  }
  ASSERT (FALSE);
  return 0L;
}


