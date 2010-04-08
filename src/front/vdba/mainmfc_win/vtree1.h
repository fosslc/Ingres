// VTree1.h : header file - Emmanuel Blattes - started Feb. 11, 97
//
#ifndef VTREE_AAD_INCLUDED
#define VTREE_AAD_INCLUDED

#include "vtree.h"    // base class

/////////////////////////////////////////////////////////////////////////////
// CTreeItemAAD object

class CTreeItemAAD : public CTreeItem
{
// Construction
public:
  // Constructor for non-static branches
  CTreeItemAAD(CTreeGlobalData *pTree, int type, SubBK subBranchKind, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);

  // Constructor for static branches
  CTreeItemAAD(CTreeGlobalData *pTree, int type, UINT nameId, SubBK subBranchKind, UINT imageId, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);

  virtual ~CTreeItemAAD();

// Construction
protected:
  CTreeItemAAD();   // serialization
  DECLARE_SERIAL (CTreeItemAAD)

// Construction
private:

// Attributes
private:

// Attributes
protected:

// Attributes
public:

// Operations
public:
  void Serialize (CArchive& ar);

  // reminder : UiUpdate must be managed
  // by derivation of virtual BOOL IsEnabled(UINT idMenu)

  // special methods for UiCommand, not virtualized
	BOOL TreeAdd();         // will call virtual Add()
	BOOL TreeAlter();       // will call virtual Alter()
	BOOL TreeDrop();        // will call virtual Delete()

  // special methods for UiCommand, virtualized, must be derived
	virtual BOOL Add()      { ASSERT (FALSE); return FALSE; }
	virtual BOOL Alter()    { ASSERT (FALSE); return FALSE; }
	virtual BOOL Drop()     { ASSERT (FALSE); return FALSE; }

// Operations
private:
};

/////////////////////////////////////////////////////////////////////////////

#endif // VTREE_AAD_INCLUDED
