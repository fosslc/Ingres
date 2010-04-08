// VTree3.h : header file - Emmanuel Blattes - started Oct. 20, 97
//
#ifndef VTREE_STAR_INCLUDED
#define VTREE_STAR_INCLUDED

#include "vtree.h"    // base class

#include "starutil.h"   // what do we link to ? LINK_TABLE, LINK_VIEW, LINK_PROCEDURE

//
// Global structures definitions
//
typedef struct  _sStarItem
{
  int   ObjType;                    // record type

  UCHAR NodeName[MAXOBJECTNAME];    // On which node ?

  UCHAR parent0[MAXOBJECTNAME];     // extra info: parenthood of level 1
  UCHAR parent1[MAXOBJECTNAME];     // extra info: parenthood of level 2
  UCHAR parent2[MAXOBJECTNAME];     // extra info: parenthood of level 3
  UCHAR objName[MAXOBJECTNAME];     // Object name (caption may be different)
  UCHAR ownerName[MAXOBJECTNAME];   // Object's owner name
  UCHAR szComplim[MAXOBJECTNAME];   // Complimentary data received (maxio,...)
  UCHAR tableOwner[MAXOBJECTNAME];  // if parent 1 is table: it's schema
} STARITEM, FAR* LPSTARITEM;


/////////////////////////////////////////////////////////////////////////////
// CTreeItemStar object

class CTreeItemStar : public CTreeItem
{
// Construction
public:
  // Constructor for non-static branches
  CTreeItemStar(CTreeGlobalData *pTree, int type, SubBK subBranchKind, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);

  // Constructor for static branches
  CTreeItemStar(CTreeGlobalData *pTree, int type, UINT nameId, SubBK subBranchKind, UINT imageId, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);

  virtual ~CTreeItemStar();

// Construction
protected:
  CTreeItemStar();   // serialization
  DECLARE_SERIAL (CTreeItemStar)

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

  // special methods for multiple nodes in the same tree,
  // MUST be derived for classes whose child branches are non-static,
  // i.e. child branches built by calls to MonGetFirst/Next
  // otherwise classes having AllocateChildItem() method implemented
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode) { ASSERT (FALSE); }
  virtual void StarCloseNodeStruct(int &hNode) { ASSERT (FALSE); }

  /* OBSOLETE :
  HTREEITEM FindItemInTree(CTreeItemStar *pSearchedItem);
  HTREEITEM FindItemInTreeBranch(HTREEITEM hItem, CTreeItemStar *pSearchedItem);
  */

  // special methods to be able to call MonXYZ or DomXYZ functions
  virtual int  ItemStructSize(int iobjecttype);
  virtual int  ItemGetFirst(int hnodestruct, int iobjecttypeParent, void* pstructParent, int iobjecttypeReq, void* pstructReq, LPSFILTER pFilter, CString &rcsItemName);
  virtual int  ItemGetNext(int hnodestruct, int iobjecttypeReq, void* pstructParent, void *pstructReq, CString &rcsItemName, int iobjecttypeParent);
  virtual int  ItemCompare(int iobjecttype, void *pstruct1, void *pstruct2);
  virtual BOOL ItemFitWithFilter(int hNode, int iobjtype,void * pstructReq,LPSFILTER pfilter);
  virtual void ItemUpdateInfo(int hnodestruct,int iobjecttypeParent, void *pstructParent, int iobjecttypeReq);
  virtual int  ItemCompleteStruct(int hnode, void *pstruct, int* poldtype, int *pnewtype);

  // new method for display name full customization -
  // MUST be derived for branches that have child items
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct) { ASSERT (FALSE); return "ERROR"; }

  // Specific for star items: do we display Coordinator Databases
  virtual BOOL DoNotShowCDBs() { return TRUE; }

// Operations
private:
};

/////////////////////////////////////////////////////////////////////////////

#endif // VTREE_STAR_INCLUDED
