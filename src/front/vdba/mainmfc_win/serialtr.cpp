//
// SerialTr.cpp : tree serialisation
// Emmanuel Blattes - started March 21, 03
//
/*
** 01-Dec-2004 (uk$so01)
**    VDBA BUG #113548 / ISSUE #13768610 
**    Fix the problem of serialization.
*/

#include "stdafx.h"

#include "vtree.h"      // mandatory, includes SeriaTr.h
#include "serialtr.h"   // obsolete, but what the ...

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// declaration for serialization
IMPLEMENT_SERIAL ( CTreeLineSerial, CObject, 2)

// "normal" constructor
CTreeLineSerial::CTreeLineSerial(CTreeCtrl *pTree, HTREEITEM hItem, HTREEITEM hParentItem)
{
  m_hItem       = hItem;
  m_hParentItem = hParentItem;
  m_Text = pTree->GetItemText(hItem);
  pTree->GetItemImage(hItem, m_nImage, m_nSelectedImage);
  m_itemState = pTree->GetItemState(hItem, TVIF_STATE);
  m_pItem = (CTreeItem *)pTree->GetItemData(hItem);
  if (m_pItem)
    m_pGlobData = m_pItem->GetPTreeGlobData();
  else
    m_pGlobData = NULL;
}

// serialization constructor
CTreeLineSerial::CTreeLineSerial()
{
}

// destructor
CTreeLineSerial::~CTreeLineSerial()
{
}

// serialization method
void CTreeLineSerial::Serialize (CArchive& ar)
{
  ASSERT (sizeof(DWORD) == sizeof(m_hItem));  // HTREEITEM
  CObject::Serialize(ar);
  if (ar.IsStoring()) {
    ar << (DWORD)m_hItem;
    ar << (DWORD)m_hParentItem;
    ar << m_Text;
    ar << m_nImage;
    ar << m_nSelectedImage;
    ar << m_itemState;
    ar << m_pGlobData;
    ar << m_pItem;
  }
  else {
    DWORD dwH;
    ar >> dwH;
    m_hItem = (HTREEITEM)dwH;
    ar >> dwH;
    m_hParentItem = (HTREEITEM)dwH;
    ar >> m_Text;
    ar >> m_nImage;
    ar >> m_nSelectedImage;
    ar >> m_itemState;
    ar >> m_pGlobData;
    ar >> m_pItem;
  }
}

