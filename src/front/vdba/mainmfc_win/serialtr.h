//
// SerialTr.h : header file for tree serialisation
// Emmanuel Blattes - started March 21, 03
//

#ifndef SERIALTREE_INCLUDED
#define SERIALTREE_INCLUDED

/////////////////////////////////////////////////////////////////////////////
// CTreeLineSerial object

class CTreeLineSerial : public CObject
{
// Construction
public:
  CTreeLineSerial(CTreeCtrl *pTree, HTREEITEM hItem, HTREEITEM hParentItem);
  ~CTreeLineSerial();

// Construction
protected:
  CTreeLineSerial();   // serialization
  DECLARE_SERIAL (CTreeLineSerial)

// Construction
private:

// Attributes
private:
  HTREEITEM         m_hItem;          // item handle
  HTREEITEM         m_hParentItem;
  CString           m_Text;           // caption
  int               m_nImage;
  int               m_nSelectedImage;
  UINT              m_itemState;      // state (collapsed/expande
  CTreeItem        *m_pItem;          // item data
  CTreeGlobalData  *m_pGlobData;      // global data of the item data

// Attributes
protected:

// Attributes
public:
  HTREEITEM         GetHItem()          { return m_hItem;           }
  HTREEITEM         GetHParentItem()    { return m_hParentItem;     }
  CString           GetText()           { return m_Text;            }
  int               GetNImage()         { return m_nImage;          }
  int               GetNSelectedImage() { return m_nSelectedImage;  }
  UINT              GetItemState()      { return m_itemState;       }
  CTreeItem        *GetPItem()          { return m_pItem;           }
  CTreeGlobalData  *GetPGlobData()      { return m_pGlobData;       }

// Operations
public:
  void Serialize (CArchive& ar);

// Operations
private:
};

/////////////////////////////////////////////////////////////////////////////

#endif  // SERIALTREE_INCLUDED
