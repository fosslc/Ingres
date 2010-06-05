/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijatree.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation tree data of the left pane
**
** History:
**
** 14-Dec-1999 (uk$so01)
**    Created
** 29-Jun-2001 (hanje04)
**    Removed , from end of defn of enum as it was causing build
**    problems on Solaris.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 05-Mar-2010 (drivi01)
**    Add m_bVW field to CaIjaDatabase class to use it to mark
**    VectorWise databases. Also add SetVW/GetVW functions.
**    VectorWise is not journaled and therefore shouldn't allow users
**    to view journals.
**/

#if !defined(IJATREE_HEADER)
#define IJATREE_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "imageidx.h"
//
// Forward declaration:
class CaIjaTreeData;
class CaIjaNode;
class CaIjaDatabase;
class CaIjaTable;
class CaUser;

enum 
{
	ITEM_UNKNOWN = 0,
	ITEM_ROOT,
	ITEM_NODE,
	ITEM_DATABASE,
	ITEM_TABLE
};

//
// Store the TreeItem State along with the TreeData:
class CaTreeCtrlData
{
public:
	enum ItemState {ITEM_DELETE = -1, ITEM_EXIST, ITEM_NEW};
	CaTreeCtrlData()
	{
		m_nItemFlag = ITEM_NEW;
		m_hTreeItem = NULL;
		m_bExpanded = FALSE;
		m_bAlreadyExpanded = FALSE;
		m_nImage = IM_FOLDER_CLOSED;
		m_nExpandedImage = IM_FOLDER_OPENED;
	}

	CaTreeCtrlData(ItemState nFlag, HTREEITEM hTreeItem)
	{
		m_nItemFlag = nFlag;
		m_hTreeItem = hTreeItem;
		m_bExpanded = FALSE;
		m_bAlreadyExpanded = FALSE;
		m_nImage = IM_FOLDER_CLOSED;
		m_nExpandedImage = IM_FOLDER_OPENED;
	}

	~CaTreeCtrlData(){}

	ItemState GetState(){return m_nItemFlag;}
	void SetState(ItemState nFlag){m_nItemFlag = nFlag;}
	HTREEITEM GetTreeItem (){return m_hTreeItem;}
	void SetTreeItem (HTREEITEM hTreeItem){m_hTreeItem = hTreeItem;}

	BOOL IsExpanded(){return m_bExpanded;}
	void SetExpand(BOOL bExpanded){m_bExpanded = bExpanded;};
	BOOL IsAlreadyExpanded(){ return m_bAlreadyExpanded;}
	void SetAlreadyExpanded(){m_bAlreadyExpanded = TRUE;}

	int GetImage(){return m_nImage;}
	int GetImageExpanded(){return m_nExpandedImage;}
	void SetImage (int nImage, int nExpandedImage)
	{
		m_nImage = nImage;
		m_nExpandedImage = nExpandedImage;
	}

private:
	//
	// When the tree item is created, it has a 'm_nItemFlag' set to ITEM_NEW.
	// When the thee item is displayed in TreeCtrl, it has a 'm_nItemFlag' set to ITEM_EXIST.
	// Before refreshing the Tree, all tree items have 'm_nItemFlag' initialized to ITEM_DELETE.
	// After requering the data: the tree items will have the following flags:
	//   ITEM_DELETE: the tree item must be delete:
	//   ITEM_EXIST : the tree item exist already (its properties changed ?)
	//   ITEM_NEW   : the tree item must be added.
	ItemState m_nItemFlag;
	HTREEITEM m_hTreeItem;  // Handle of Tree Item 
	BOOL m_bExpanded;       // Expanded / Collapsed
	BOOL m_bAlreadyExpanded;// Has been expanded.
	int m_nImage;
	int m_nExpandedImage;

};

class CaRefreshTreeInfo
{
public:
	CaRefreshTreeInfo(): m_nLevel(0) {}
	~CaRefreshTreeInfo(){}

	void SetInfo (int nLevel = 0)
	{
		m_nLevel = nLevel;
	}

	//
	// m_nLevel = 0, this branch
	//          = 1, this branch and one level sub-branch
	//          =-1, this branch and all sub-branch.
	int m_nLevel;
};

class CaIjaTreeItemData: public CaDBObject
{
public:
	CaIjaTreeItemData(): CaDBObject(), m_pBackParent(NULL){}
	CaIjaTreeItemData(LPCTSTR lpszItem, LPCTSTR lpszOwner = NULL): CaDBObject(lpszItem, lpszOwner),m_pBackParent(NULL){}
	virtual ~CaIjaTreeItemData(){}
	virtual int GetItemType(){return ITEM_UNKNOWN;}
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent = NULL);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData)
	{
		m_treeCtrlData.SetExpand(TRUE);
		m_treeCtrlData.SetAlreadyExpanded();
	}
	virtual void Collapse(CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData)
	{
		m_treeCtrlData.SetExpand(FALSE);
	}
	
	virtual BOOL IsRefreshCurrentBranchEnabled() {return FALSE;}
	virtual BOOL IsRefreshCurrentSubBranchEnabled() {return FALSE;}

	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo){ASSERT (FALSE); return TRUE;}
	CaTreeCtrlData& GetTreeCtrlData() {return m_treeCtrlData;}

	void SetBackParent(CaIjaTreeItemData* pBackPointer){m_pBackParent = pBackPointer;}
	CaIjaTreeItemData* GetBackParent(){return m_pBackParent;}

protected:
	CaTreeCtrlData m_treeCtrlData;

	CaIjaTreeItemData* m_pBackParent;
};


class CaIjaTable: public CaIjaTreeItemData
{
public:
	CaIjaTable();
	CaIjaTable(LPCTSTR lpszItem, LPCTSTR lpszOwner = NULL);
	virtual ~CaIjaTable(){};
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent = NULL);
	virtual int GetItemType(){return ITEM_TABLE;}


protected:
};


class CaIjaDatabase: public CaIjaTreeItemData
{
public:
	CaIjaDatabase();
	CaIjaDatabase(LPCTSTR lpszItem, LPCTSTR lpszOwner = NULL);
	virtual ~CaIjaDatabase();
	virtual int GetItemType(){return ITEM_DATABASE;}
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent = NULL);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);

	virtual BOOL IsRefreshCurrentBranchEnabled() {return TRUE;}
	virtual BOOL IsRefreshCurrentSubBranchEnabled() {return FALSE;}
	
	CTypedPtrList < CObList, CaIjaTreeItemData* >& GetListTable(){return m_listTable;}
	void SetStar(int nStar = 0);
	int  GetStar(){return m_nStar;}
	void SetVW(int bVW = 0){m_bVW = bVW;}
	int  GetVW(){return m_bVW;}

protected:
	int m_nStar;
	int m_bVW;
	CaIjaTreeItemData m_EmptyTable;
	CTypedPtrList < CObList, CaIjaTreeItemData* > m_listTable;
};


class CaIjaNode: public CaIjaTreeItemData
{
public:
	CaIjaNode();
	CaIjaNode(LPCTSTR lpszItem);
	virtual ~CaIjaNode();
	virtual int GetItemType(){return ITEM_NODE;}
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent = NULL);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);

	virtual BOOL IsRefreshCurrentBranchEnabled() {return TRUE;}
	virtual BOOL IsRefreshCurrentSubBranchEnabled() {return TRUE;}

	void LocalNode(){m_bLocal = TRUE;}
	BOOL IsLocalNode(){return m_bLocal;}
	CTypedPtrList < CObList, CaIjaTreeItemData* >& GetListDatabase(){return m_listDatabase;}
protected:
	BOOL m_bLocal;
	CaIjaTreeItemData m_EmptyDatabase;
	CTypedPtrList < CObList, CaIjaTreeItemData* > m_listDatabase;

};



class CaIjaTreeData: public CaIjaTreeItemData
{
public:
	CaIjaTreeData();
	virtual ~CaIjaTreeData();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent = NULL);

	virtual int GetItemType(){return ITEM_ROOT;}
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData);
	virtual void Collapse(CTreeCtrl* pTree, HTREEITEM hExpand, CaIjaTreeData* pTreeData);
	virtual BOOL IsRefreshCurrentBranchEnabled() {return TRUE;}
	virtual BOOL IsRefreshCurrentSubBranchEnabled() {return TRUE;}

	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	void SortFolder(CTreeCtrl* pTree);
	static int CALLBACK fnCompareComponent(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	void SetDisplaySpecial(int nLevel){m_nDisplaySpecial = nLevel;}

	CaIjaNode& GetSpecialRootNode(){return m_nodeRoot;}
	CaIjaDatabase& GetSpecialRootDatabase(){return m_databaseRoot;}
	CaIjaTable& GetSpecialRootTable(){return m_tableRoot;}

protected:
	CaIjaTreeItemData m_EmptyNode;
	CTypedPtrList < CObList, CaIjaTreeItemData* > m_listNode;

	//
	// SPECIAL Display the tree data:
	int m_nDisplaySpecial;
	CaIjaNode m_nodeRoot;         // In order to be able to display a Node at the Root level
	CaIjaDatabase m_databaseRoot; // In order to be able to display a Database at the Root level
	CaIjaTable m_tableRoot;       // In order to be able to display a Database at the Root level:

};



#endif
