// VTree.h : header file - Emmanuel Blattes - started Feb. 3, 97
//
// Intensively Revized April 7, 97 :
// CTreeItem, CTreeItemData and CTreeGlobalData in the same file
//
// Revizled May 12, 97: images for non-static items instead of static as previously
// 20-Aug-2008 (whiro01)
//    Remove redundant <afx...> include which is already in "stdafx.h"
//

#ifndef VTREE_INCLUDED
#define VTREE_INCLUDED

extern "C" {
#include "monitor.h"
};

// public utilities
BOOL MonIsReplicatorInstalled(int hNode, CString dbName);
int  MonGetReplicatorVersion(int hNode, CString dbName);

// forward definitions of classes
class CTreeItem;
class CTreeGlobalData;
class CTreeItemData;

class CuPageInformation;

#include "serialtr.h"

// constants
#define DEFAULT_IMAGE_ID    0

// Sub branch type
typedef enum
{   
  SUBBRANCH_KIND_NONE = 1,
  SUBBRANCH_KIND_STATIC,
  SUBBRANCH_KIND_OBJ,
  SUBBRANCH_KIND_MIX
} SubBK;

// action for display
typedef enum
{
  DISPLAY_ACT_EXPAND = 1,
  DISPLAY_ACT_REFRESH,
  DISPLAY_ACT_CHANGEFILTER
} DispAction;

// cause for refresh
typedef enum
{
  REFRESH_ADD = 1,
  REFRESH_ALTER,
  REFRESH_DROP,
  REFRESH_ACTIVATEWIN,
  REFRESH_CLOSEWIN,
  REFRESH_CONNECT,
  REFRESH_DBEVENT,
  REFRESH_DISCONNECT,
  REFRESH_MONITOR,
  REFRESH_SQLTEST,
  REFRESH_FORCE,
  REFRESH_KILLSHUTDOWN
} RefreshCause;

// cause for change filter
typedef enum
{
  FILTER_NOTHING = 1,
  FILTER_RESOURCE_TYPE,
  FILTER_NULL_RESOURCES,
  FILTER_INTERNAL_SESSIONS,
  FILTER_SYSTEM_LOCK_LISTS,
  FILTER_INACTIVE_TRANSACTIONS,
  FILTER_SEVERAL_ACTIONS,

  FILTER_DOM_SYSTEMOBJECTS,
  FILTER_DOM_BASEOWNER,
  FILTER_DOM_OTHEROWNER,

  // New as of May 25, 98: for detail window of type list: update if necessary
  // if used, need to test new field "UpdateType" of LPIPMUPDATEPARAMS against
  // type used by GetFirst/Next loop, for the relevant detail window
  FILTER_DOM_BKREFRESH,         // means refresh panes that look like lists
  FILTER_DOM_BKREFRESH_DETAIL,
  FILTER_IPM_EXPRESS_REFRESH,
  FILTER_SETTING_CHANGE         // SQL Act setting changes
} FilterCause;


// item kind
typedef enum
{   
  ITEM_KIND_NORMAL = 1,
  ITEM_KIND_NOITEM,
  ITEM_KIND_ERROR
} ItemKind;

#define CHILDTYPE_UNDEF (-1)

/////////////////////////////////////////////////////////////////////////////
// CTreeItem object

class CTreeItem : public CObject
{
// Construction
public:
  // Constructor for non-static branches
  CTreeItem(CTreeGlobalData *pTree, int type, SubBK subBranchKind, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);

  // Constructor for static branches
  CTreeItem(CTreeGlobalData *pTree, int type, UINT nameId, SubBK subBranchKind, UINT imageId, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);

  // minimum constructor for serialization preparation
  CTreeItem(CTreeGlobalData *pTree);

  virtual ~CTreeItem();

// Construction
protected:
  CTreeItem();   // serialization
  DECLARE_SERIAL (CTreeItem)

// Construction
private:

// Attributes
private:
  CTreeGlobalData *m_pTreeGD;
  int              m_type;       // Must be initialized by derived classes
  int              m_childtype;  // For items with non-static children
  CString          m_name;
  BOOL             m_bAlreadyExpanded;
  BOOL             m_bStatic;
  SubBK            m_subBranchKind;
  CTreeItemData   *m_pItemData;
  UINT             m_menuId;     // context menuId
  int              m_completetype;
  BOOL             m_bTypeCompleted;
  ItemKind         m_itemKind;

  // special
  BOOL    m_bToBeDeleted;

  // for non-static only as of may 12 (previously for static only)
  UINT    m_imageId;

  // for non-static only
  BOOL    m_bNeedsTypeSolve;      // OBSOLETE thanks to completetype/MonCompleteStruct()

  // for items having non-static children only
  UINT    m_noChildStringId;


// Attributes
protected:
    CuPageInformation*  m_pPageInfo;

// Attributes
public:

// Operations
public:
  void Serialize (CArchive& ar);
  void Constructor(CTreeGlobalData *pTree, int type, CString name, BOOL bStatic, SubBK subBranchKind, UINT imageId, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype, UINT noChildStringId);

  // attributes operations

  CTreeGlobalData *GetPTreeGlobData() const { return m_pTreeGD; }
  CTreeItemData   *GetPTreeItemData() const { return m_pItemData; }

  BOOL    IsAlreadyExpanded()             { return m_bAlreadyExpanded; }
  void    SetAlreadyExpanded(BOOL state)  { m_bAlreadyExpanded = state; }
  BOOL    IsStatic()                      { return m_bStatic; }
  SubBK   GetSubBK()                      { return m_subBranchKind; }
  void    SetSubBK(SubBK kind)            { m_subBranchKind = kind; }
  BOOL    HasSubBranches()                { return (m_subBranchKind!=SUBBRANCH_KIND_NONE? TRUE: FALSE); }
  int     GetType()                       { return m_type; }
  void    SetType(int type)               { m_type = type; }    // Dangerous! use with care
  int     GetChildType()                  { return m_childtype; }

  UINT    GetImageId()                    { ASSERT (!m_bStatic); return m_imageId; }
  UINT    GetNoChildStringId()            { ASSERT (m_noChildStringId); return m_noChildStringId; }

  void    MarkToBeDeleted()               { m_bToBeDeleted = TRUE; }
  void    ResetDeleteFlag()               { m_bToBeDeleted = FALSE; }
  BOOL    IsMarkedToBeDeleted()           { return m_bToBeDeleted; }

  CString GetDisplayName()                { return m_name; }
  void    SetDisplayName(CString name)    { m_name = name; }  // does not refresh display
  void    SetDisplayName(CString name, HTREEITEM hItem);    // refreshes display

  UINT    GetContextMenuId()              { return m_menuId; }
  void    SetContextMenuId(UINT menuId)   { m_menuId = menuId; }

  void    SetCompleteType(int type)       { m_completetype = type; }  // only for classes that need type change
  int     GetCompleteType()               { return m_completetype; }
  BOOL    IsTypeCompleted()               { return m_bTypeCompleted; }

  // methods for special branches ( errors, no child, ...)
  BOOL      CreateSpecialItem(HTREEITEM hParentItem, int iobjecttypeReq, UINT stringId, int res, ItemKind itemKind, CTreeItemData *pItemData = NULL);
  BOOL      CreateErrorItem(HTREEITEM hParentItem, int iobjecttypeReq, int res);
  BOOL      CreateNoChildItem(HTREEITEM hParentItem, int iobjecttypeReq, UINT noChildStringId, CTreeItemData *pItemData = NULL);
  BOOL      IsNoItem()            { return (m_itemKind == ITEM_KIND_NOITEM? TRUE: FALSE); }
  void      SetNoItem();
  BOOL      IsErrorItem()         { return (m_itemKind == ITEM_KIND_ERROR? TRUE: FALSE); }
  void      SetErrorItem();
  BOOL      IsSpecialItem()       { return (m_itemKind != ITEM_KIND_NORMAL? TRUE: FALSE); }

  // utility methods
  HTREEITEM MarkAllImmediateSubItems(HTREEITEM hParentItem);
  void      DeleteAllMarkedSubItems(HTREEITEM hParentItem, HTREEITEM hDummy);

  // special methods, non virtualized
  BOOL      CreateSubBranches(HTREEITEM hItem);
  BOOL      CreateDataSubBranches(HTREEITEM hParentItem);
  BOOL      RefreshDataSubBranches(HTREEITEM hParentItem, CTreeItem *pExistItem = NULL, BOOL * pbStillExists = NULL);
  BOOL      DisplayDataSubBranches(HTREEITEM hParentItem, DispAction action);
  BOOL      RefreshDisplayedItemsList(RefreshCause because);
  BOOL      FilterDisplayedItemsList(HTREEITEM hItem, FilterCause because);
  HTREEITEM CreateTreeLine(HTREEITEM hParent = TVI_ROOT, HTREEITEM hInsertAfter = TVI_LAST);
  BOOL      CompleteStruct();

  // special methods for image change, even for non_static tree lines
  BOOL          ChangeImageId(UINT newId, HTREEITEM hItem);
  virtual UINT  GetCustomImageId() { return m_imageId; }  // base class: never change image id

  // added May 12, 97 because non-static tree lines ALWAYS have an image id
  void    InitializeImageId(UINT newImageId) { ASSERT (newImageId); m_imageId = newImageId; }

  // special methods, virtualized
  virtual BOOL       CreateStaticSubBranches(HTREEITEM hParentItem);
  virtual CTreeItem *AllocateChildItem(CTreeItemData *pItemData);
  virtual BOOL       IsEnabled(UINT idMenu)   { return FALSE; }   // for UiUpdate
  virtual CuPageInformation*   GetPageInformation ();
  virtual BOOL       RefreshRelatedBranches(HTREEITEM hItem, RefreshCause because);
  virtual void       MinFillpStructReq(void *pStructReq) {}
  virtual BOOL       FilterApplies(FilterCause because);
  virtual void       UpdateParentBranchesAfterExpand(HTREEITEM hItem);

  // special methods for UiCommand, not virtualized
	BOOL TreeKillShutdown();    // will call virtual KillShutdown()
	BOOL TreeCloseServer();     // will call virtual CloseServer()
	BOOL TreeOpenServer();     // will call virtual OpenServer()

  // special methods for UiCommand, virtualized, must be derived
	virtual BOOL KillShutdown()   { ASSERT (FALSE); return FALSE; }
	virtual BOOL CloseServer()    { ASSERT (FALSE); return FALSE; }
	virtual BOOL OpenServer()     { ASSERT (FALSE); return FALSE; }

  // branches expandability according to filter, plus utility
  void MakeBranchNotExpandable(HTREEITEM hItem);
  void MakeBranchExpandable(HTREEITEM hItem);
  void DeleteAllChildrenDataOfItem(HTREEITEM hParentItem, BOOL bIncludingCurrent);

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode) {}
  virtual void StarCloseNodeStruct(int &hNode) {}

  // special methods to be able to call MonXYZ or DomXYZ functions
  virtual int  ItemStructSize(int iobjecttype);
  virtual int  ItemGetFirst(int hnodestruct, int iobjecttypeParent, void* pstructParent, int iobjecttypeReq, void* pstructReq, LPSFILTER pFilter, CString &rcsItemName);
  virtual int  ItemGetNext(int hnodestruct, int iobjecttypeReq, void* pstructParent, void *pstructReq, CString &rcsItemName, int iobjecttypeParent);
  virtual int  ItemCompare(int iobjecttype, void *pstruct1, void *pstruct2);
  virtual BOOL ItemFitWithFilter(int hNode, int iobjtype,void * pstructReq, LPSFILTER pfilter);
  virtual void ItemUpdateInfo(int hnodestruct,int iobjecttypeParent, void *pstructParent, int iobjecttypeReq);
  virtual int  ItemCompleteStruct(int hnode, void *pstruct, int* poldtype, int *pnewtype);

  virtual CString GetRightPaneTitle() { return m_name; }
  virtual BOOL    ItemDisplaysNoPage() { return FALSE; }
  virtual CString ItemNoPageCaption()  { return ""; }

  virtual BOOL    HasReplicMonitor() { return FALSE; }
  virtual CString GetDBName() { ASSERT (HasReplicMonitor()); return m_name; }
  //
  // For the class that needs to specialize the OnUpdateData member, you must
  // overwrite this virtual method:
  virtual void UpdateDataWhenSelChange() {}

  // Multiple expansion stop point - To be derived if necessary
  virtual BOOL CanBeExpanded();

  // Special methods for special values RES_NOT_OI, RES_CANNOT_SOLVE and RES_MULTIPLE_GRANTS
  virtual UINT GetStringId_ResNotOi()           { ASSERT (FALSE); return 0; }
  virtual UINT GetStringId_ResCannotSolve()     { ASSERT (FALSE); return 0; }
  virtual UINT GetStringId_ResMultipleGrants()  { ASSERT (FALSE); return 0; }

  // For pPageInfo modified from outside ( specifiedtabonly through context-driven launch )
  void  UpdatePPageInfo(CuPageInformation* pPageInfo) { ASSERT (pPageInfo); m_pPageInfo = pPageInfo; }

// Operations
private:
};


/////////////////////////////////////////////////////////////////////////////
// CTreeItemData object

class CTreeItemData : public CObject
{
// Construction
public:
  CTreeItemData(void *pstruct, int structsize, int hNode, CString name);
  CTreeItemData(const CTreeItemData &itemData);  // copy constructor
  CTreeItemData &operator=( CTreeItemData & );      // assignment operator
  ~CTreeItemData();

// Construction
protected:
  CTreeItemData();   // serialization
  DECLARE_SERIAL (CTreeItemData)

// Construction
private:

// Attributes
private:
  int     m_size;
  void *  m_pstruct;
  int     m_hNode;
  CString m_name;

// Attributes
protected:

// Attributes
public:

// Operations
public:
	CString GetItemName(int objType);
  int  GetDataSize()         { return m_size; }
  void *GetDataPtr() const   { return m_pstruct; }
  int  GetHNode()            { return m_hNode; }
  void Serialize (CArchive& ar);
  BOOL SetNewStructure(void *newpstruct);
  void SetItemName(CString name) { m_name = name; }

// Operations
private:
};


/////////////////////////////////////////////////////////////////////////////
// CTreeGlobalData object

// our image dimensions - compatibility with non-mfc dom tree
#define IMAGE_WIDTH  16   // ex-12
#define IMAGE_HEIGHT 15   // ex-12

class CTreeGlobalData : public CObject
{
// Construction
public:
  CTreeGlobalData(CTreeCtrl *pTree, LPSFILTER psFilter=NULL, int hNode=-1);
  ~CTreeGlobalData();

// Construction
protected:
  CTreeGlobalData();   // serialization
  DECLARE_SERIAL (CTreeGlobalData)

// Construction
private:
	void FreeItemData(HTREEITEM hItem);

// Attributes
private:
  CTreeCtrl *m_pTree;
	CImageList m_imageList;
  CMap< UINT, UINT, int, int > m_aBitmapIndex;
  CArray <UINT, UINT > m_aBmpId;
	LPSFILTER  m_psFilter;
  int        m_hNode;

  // Serialization data
  CTypedPtrList<CObList, CTreeLineSerial*>    m_treeLineSerialList;
  HTREEITEM m_hSelectedItem;
  HTREEITEM m_hFirstVisibleItem;
  BOOL      m_bCurrentlyLoading;

// Attributes
protected:

// Attributes
public:

// Operations
public:
  CTreeCtrl *GetPTree() const    { return m_pTree; }
	LPSFILTER  GetPSFilter() const { return m_psFilter; }
  int        GetHNode() const    { return m_hNode; }
  int        AddBitmap(UINT bitmapId);
  UINT       GetContextMenuId();
	void       FreeAllTreeItemsData();

  // Filtering
  BOOL        FilterAllDisplayedItemsLists(FilterCause because);
  BOOL        FilterDisplayedItemsList(HTREEITEM hItem, FilterCause because);

  // serialization
  void Serialize (CArchive& ar);
  void FillSerialListFromTree();
  void StoreTreeBranch(HTREEITEM hItem, HTREEITEM hParentItem);
  void FillTreeFromSerialList();
  void CleanupSerialList();
  CTypedPtrList<CObList, CTreeLineSerial*>& GetTreeLineSerialList() { return m_treeLineSerialList; }
  BOOL CurrentlyLoading()        { return m_bCurrentlyLoading; }

  //HTREEITEM GetSelectedItem() { return m_hSelectedItem; }
  //HTREEITEM GetFirstVisibleItem() { return m_hFirstVisibleItem; }

  void SetPTree(CTreeCtrl *pTree, BOOL bSetImageList = TRUE);
	void SetPSFilter(LPSFILTER psFilter) { m_psFilter = psFilter; }
	void SetHNode(int hNode) { m_hNode = hNode; }

  // refresh several branches
  void RefreshAllTreeBranches();
  void RefreshBranchAndSubBranches(HTREEITEM hItem);

  // Expand - Collapse
  void ExpandBranch(HTREEITEM hItem);
  void ExpandBranch();
  void ExpandAll();
  void CollapseBranch(HTREEITEM hItem);
  void CollapseBranch();
  void CollapseAll();

// Operations
private:
};

/////////////////////////////////////////////////////////////////////////////

#endif // VTREE_INCLUDED
