/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vtree.h : header file 
**    Project  : INGRESII / Monitoring.
**    Author   : Emmanuel Blattes 
**    Purpose  : The view containing the modeless dialog to display the conflict rows 
**
** History:
**
** 03-Feb-1998 (Emmanuel Blattes)
**    Created
** 22-May-2003 (uk$so01)
**    SIR #106648
**    Change the serialize machanism. Do not serialize the HTREEITEM
**    because the stored HTREEITEM may different of when the tree is
**    reconstructed from the serialize.
*/


// VTree.h : header file - Emmanuel Blattes - started Feb. 3, 97
//
// Intensively Revized April 7, 97 :
// CTreeItem, CTreeItemData and CTreeGlobalData in the same file
//
// Revizled May 12, 97: images for non-static items instead of static as previously
//

#ifndef VTREE_INCLUDED
#define VTREE_INCLUDED
// forward definitions of classes
class CTreeItem;
class CTreeGlobalData;
class CTreeItemData;
class CuPageInformation;

#include "ipmstruc.h"


class CTreeItem : public CObject
{
public:
	// Constructor for non-static branches
	CTreeItem(CTreeGlobalData *pTree, int type, SubBK subBranchKind, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);
	// Constructor for static branches
	CTreeItem(CTreeGlobalData *pTree, int type, UINT nameId, SubBK subBranchKind, UINT imageId, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);
	// minimum constructor for serialization preparation
	CTreeItem(CTreeGlobalData *pTree);
	virtual ~CTreeItem();

protected:
	CTreeItem();   // serialization
	DECLARE_SERIAL (CTreeItem)

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

	// special methods to be able to call MonXYZ or DomXYZ functions
	virtual int  ItemCompare(int iobjecttype, void *pstruct1, void *pstruct2);
	virtual BOOL ItemFitWithFilter(CdIpmDoc* pDoc, int iobjtype,void * pstructReq, LPSFILTER pfilter);
	virtual void ItemUpdateInfo(int hnodestruct,int iobjecttypeParent, void *pstructParent, int iobjecttypeReq);

	virtual CString GetRightPaneTitle() { return m_name; }
	virtual BOOL    ItemDisplaysNoPage() { return FALSE; }
	virtual CString ItemNoPageCaption()  { return _T(""); }

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
};


/////////////////////////////////////////////////////////////////////////////
// CTreeItemData object

class CTreeItemData : public CObject
{
public:
	CTreeItemData(void *pstruct, int structsize, CdIpmDoc* pDoc, CString name);
	CTreeItemData(const CTreeItemData &itemData);  // copy constructor
	CTreeItemData &operator=( CTreeItemData & );      // assignment operator
	~CTreeItemData();

protected:
	CTreeItemData();   // serialization
	DECLARE_SERIAL (CTreeItemData)

	// Attributes
private:
	int     m_size;
	void *  m_pstruct;
	CString m_name;
	CdIpmDoc* m_pIpmDoc;

	// Operations
public:
	CString GetItemName(int objType);
	int  GetDataSize()         { return m_size; }
	void*GetDataPtr() const   { return m_pstruct; }
	CdIpmDoc* GetDocument(){return m_pIpmDoc;}
	void Serialize (CArchive& ar);
	BOOL SetNewStructure(void *newpstruct);
	void SetItemName(CString name) { m_name = name; }
};



class CaIpmTreeData: public CObject
{
	DECLARE_SERIAL (CaIpmTreeData)
	CaIpmTreeData();
public:
	CaIpmTreeData(CTreeCtrl* pTree, HTREEITEM hItem = NULL);
	virtual ~CaIpmTreeData();
	virtual void Serialize (CArchive& ar);

	void Cleanup()
	{
		while (!m_listChildren.IsEmpty())
			delete m_listChildren.RemoveHead();
	}
	void AddSibling(CaIpmTreeData* pItem){m_listChildren.AddTail(pItem);}
	CTypedPtrList<CObList, CaIpmTreeData*>& GetListChildren(){return m_listChildren;}

	void SetItemState(UINT nState){m_nuState = nState;}
	void SetItemText (LPCTSTR lpszText){m_strItemText = lpszText;}
	void GetImage(int nImage){m_nImage = nImage;}
	void GetSelectedImage(int nImage){m_nSelectedImage = nImage;}
	void SetItemData(CTreeItem* pData){m_pItem = pData;}
	void SetFirstVisible(BOOL bSet){m_bFirtVisible = bSet;}

	UINT GetItemState(){return m_nuState;}
	CString GetItemText(){return m_strItemText;}
	int GetImage(){return m_nImage;}
	int GetSelectedImage(){return m_nSelectedImage;}
	CTreeItem* GetItemData(){return m_pItem;}
	BOOL IsFirstVisible(){return m_bFirtVisible;}

protected:
	UINT    m_nuState;
	CString m_strItemText;
	int     m_nImage;
	int     m_nSelectedImage;
	BOOL    m_bFirtVisible;
	CTreeItem* m_pItem;

	CTypedPtrList<CObList, CaIpmTreeData*> m_listChildren;

};

/////////////////////////////////////////////////////////////////////////////
// CTreeGlobalData object

class CTreeGlobalData : public CObject
{
public:
	CTreeGlobalData(CTreeCtrl *pTree, LPSFILTER psFilter=NULL, CdIpmDoc* pDoc = NULL);
	~CTreeGlobalData();

	void SetDocument(CdIpmDoc* pDoc){m_pIpmDoc = pDoc;}
	CdIpmDoc* GetDocument(){return m_pIpmDoc;}

protected:
	CTreeGlobalData();   // serialization
	DECLARE_SERIAL (CTreeGlobalData)

private:
	void FreeItemData(HTREEITEM hItem);

	// Attributes
private:
	CTreeCtrl *m_pTree;
	CImageList m_imageList;
	CMap< UINT, UINT, int, int > m_aBitmapIndex;
	CArray <UINT, UINT > m_aBmpId;
	LPSFILTER  m_psFilter;
	CdIpmDoc* m_pIpmDoc;
	int m_nTreeHScrollPos;
	// Serialization data
	CaIpmTreeData* m_pTreeData;
	BOOL      m_bCurrentlyLoading;

	// Operations
public:
	CTreeCtrl *GetPTree() const    { return m_pTree; }
	LPSFILTER  GetPSFilter() const { return m_psFilter; }
	int        AddBitmap(UINT bitmapId);
	UINT       GetContextMenuId();
	void       FreeAllTreeItemsData();

	// Filtering
	BOOL        FilterAllDisplayedItemsLists(FilterCause because);
	BOOL        FilterDisplayedItemsList(HTREEITEM hItem, FilterCause because);

	// serialization
	void Serialize (CArchive& ar);
	void FillSerialListFromTree(HTREEITEM hItem, CaIpmTreeData* pTreeNode);
	void FillTreeFromSerialList();
	void LoadTree(CaIpmTreeData* pTreeData, HTREEITEM hTree, HTREEITEM& hFirstVisible);
	BOOL CurrentlyLoading()        { return m_bCurrentlyLoading; }

	void SetPTree(CTreeCtrl *pTree, BOOL bSetImageList = TRUE);
	void SetPSFilter(LPSFILTER psFilter) { m_psFilter = psFilter; }

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
};


#endif // VTREE_INCLUDED
