// domfast.cpp : implementation file for fast access to an item.
//               thanks to double-click in right pane changing selection in the dom tree
//

#include "stdafx.h"

#include "domfast.h"
#include "cmdline.h"

////////////////////////////////////////////////////////////////////////////////////////////
// Class CuDomTreeFastItem

// IMPLEMENT_SERIAL (CuDomTreeFastItem, CObject, 1)

CuDomTreeFastItem::CuDomTreeFastItem(int type, LPCTSTR pszName, LPCTSTR pszOwner)
{
  m_type = type;
  if (pszName)
    m_csName = pszName;
  if (pszOwner)
    m_csOwner = pszOwner;
}

CuDomTreeFastItem::~CuDomTreeFastItem()
{
}


////////////////////////////////////////////////////////////////////////////////////////////
// Functions

BOOL ExpandUpToSearchedItem(CWnd* pPropWnd, CTypedPtrList<CObList, CuDomTreeFastItem*>& rDomTreeFastItemList, BOOL bAutomated)
{
  CChildFrame* pChildFrame = 0;
  if (bAutomated) {
    ASSERT (pPropWnd->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    pChildFrame = (CChildFrame*)pPropWnd;
  }
  else {
    CWnd* pParent  = pPropWnd->GetParent();   // ---> pParent:  CTabCtrl
    CWnd* pParent2 = pParent->GetParent();    // ---> pParent2: CuDlgDomTabCtrl
    CWnd* pParent3 = pParent2->GetParent();   // ---> pParent3: CMainMfcView2
    CWnd* pParent4 = pParent3->GetParent();   // ---> pParent4: CSplitterWnd
    CWnd* pParent5 = pParent4->GetParent();   // ---> pParent5: CChildFrame
    ASSERT (pParent5->IsKindOf(RUNTIME_CLASS(CChildFrame)));
    pChildFrame = (CChildFrame*)pParent5;
  }
  ASSERT (pChildFrame);

  DWORD dwZeSel = 0;
  if (bAutomated) {
    dwZeSel = 0;      // Will force search on all root items
  }
  else {
    dwZeSel = pChildFrame->GetTreeCurSel();
    ASSERT (dwZeSel);
  }
  POSITION pos = rDomTreeFastItemList.GetHeadPosition();
  while (pos) {
    CuDomTreeFastItem* pFastItem = rDomTreeFastItemList.GetNext (pos);
    dwZeSel = pChildFrame->FindSearchedItem(pFastItem, dwZeSel);
    if (dwZeSel == 0L) {
      ASSERT (FALSE);
      return FALSE;
    }
  }
  pChildFrame->SelectItem(dwZeSel);
  return TRUE;
}

