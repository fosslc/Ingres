/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : trctldta.h: interface for the CaDisplayInfo class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : INFO for displaying the Objects in a Tree Hierachy.
**
** History:
**
** 17-Jan-2000 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 22-Apr-2003 (schph01)
**    SIR #107523 Add O_SEQUENCE
**/

#if !defined (TREECONTROLDATA_HEADER)
#define TREECONTROLDATA_HEADER
#include "imageidx.h"        // 0-based index of ingres image list
#include "qryinfo.h"         // CaLLQueryInfo
#include "dmlbase.h"         // CaDBObject

class CmtSessionManager;
typedef BOOL (CALLBACK* PfnUserQueryObject)
	(CaLLQueryInfo* pInfo, 
	 CTypedPtrList<CObList, CaDBObject*>& listObject, 
	 CmtSessionManager* pSmgr);
typedef BOOL (CALLBACK* PfnCOMQueryObject) 
	(IUnknown* pAptAccess, 
	 CaLLQueryInfo* pInfo, 
	 CTypedPtrList<CObList, CaDBObject*>& listObject);


#define NUM_OBJECTFOLDER 51  // The total number of the enum O_XX bellow:
typedef enum
{
	O_BASE_ITEM = 0,
	O_FOLDER_INSTALLATION,

	O_FOLDER_NODE,
	O_FOLDER_NODE_SERVER,
	O_FOLDER_NODE_ADVANCE,
	O_FOLDER_NODE_LOGIN,
	O_FOLDER_NODE_CONNECTION,
	O_FOLDER_NODE_ATTRIBUTE,

	O_FOLDER_DATABASE,
	O_FOLDER_PROFILE,
	O_FOLDER_USER,
	O_FOLDER_GROUP,
	O_FOLDER_ROLE,
	O_FOLDER_LOCATION,

	O_FOLDER_TABLE,
	O_FOLDER_VIEW,
	O_FOLDER_PROCEDURE,
	O_FOLDER_SEQUENCE,
	O_FOLDER_DBEVENT,
	O_FOLDER_SYNONYM,

	O_FOLDER_COLUMN,
	O_FOLDER_INDEX,
	O_FOLDER_RULE,
	O_FOLDER_INTEGRITY,
	O_FOLDER_ALARM,
	O_FOLDER_GRANTEE,

	O_NODE,
	O_NODE_SERVER,
	O_NODE_ADVANCE,
	O_NODE_LOGIN,
	O_NODE_CONNECTION,
	O_NODE_ATTRIBUTE,

	O_DATABASE,
	O_PROFILE,
	O_USER,
	O_GROUP,
	O_ROLE,
	O_LOCATION,

	O_TABLE,
	O_VIEW,
	O_PROCEDURE,
	O_SEQUENCE,
	O_DBEVENT,
	O_SYNONYM,

	O_COLUMN,
	O_INDEX,
	O_RULE,
	O_INTEGRITY,
	O_ALARM,
	O_GRANTEE,
	O_PRIVILEGE
} ObjectInFolder, TreeItemType;




class CtrfItemData;
class CaDisplayInfo
{
public:
	CaDisplayInfo();
	~CaDisplayInfo(){}
	
	CString GetOwnerPrefixedFormat(){return m_strOWNERxITEM;}
	void SetOwnerPrefixedFormat(LPCTSTR lpszFormat){m_strOWNERxITEM = lpszFormat;}
	
	
	void GetEmptyImage          (int& nImage, int& nImageExpand){nImage = m_nEmpty; nImageExpand = m_nEmptyExpand;}
	void GetFolderImage         (int& nImage, int& nImageExpand){nImage = m_nFolder; nImageExpand = m_nFolderExpand;}
	void GetNodeImage           (int& nImage, int& nImageExpand){nImage = m_nNode; nImageExpand = m_nNodeExpand;}
	void GetNodeLocalImage      (int& nImage, int& nImageExpand){nImage = m_nNodeLocal; nImageExpand = m_nNodeLocalExpand;}
	void GetNodeServerImage     (int& nImage, int& nImageExpand){nImage = m_nNodeServer; nImageExpand = m_nNodeServerExpand;}
	void GetNodeLoginImage      (int& nImage, int& nImageExpand){nImage = m_nNodeLogin; nImageExpand = m_nNodeLoginExpand;}
	void GetNodeConnectionImage (int& nImage, int& nImageExpand){nImage = m_nNodeConnection; nImageExpand = m_nNodeConnectionExpand;}
	void GetNodeAttributeImage  (int& nImage, int& nImageExpand){nImage = m_nNodeAttribute; nImageExpand = m_nNodeAttributeExpand;}
	void GetUserImage           (int& nImage, int& nImageExpand){nImage = m_nUser; nImageExpand = m_nUserExpand;}
	void GetGroupImage          (int& nImage, int& nImageExpand){nImage = m_nGroup; nImageExpand = m_nGroupExpand;}
	void GetProfileImage        (int& nImage, int& nImageExpand){nImage = m_nProfile; nImageExpand = m_nGroupExpand;}
	void GetRoleImage           (int& nImage, int& nImageExpand){nImage = m_nRole; nImageExpand = m_nRoleExpand;}
	void GetLocationImage       (int& nImage, int& nImageExpand){nImage = m_nLocation; nImageExpand = m_nLocationExpand;}
	void GetDatabaseImage       (int& nImage, int& nImageExpand){nImage = m_nDatabase; nImageExpand = m_nDatabaseExpand;}
	void GetTableImage          (int& nImage, int& nImageExpand){nImage = m_nTable; nImageExpand = m_nTableExpand;}
	void GetViewImage           (int& nImage, int& nImageExpand){nImage = m_nView; nImageExpand = m_nViewExpand;}
	void GetProcedureImage      (int& nImage, int& nImageExpand){nImage = m_nProcedure; nImageExpand = m_nProcedureExpand;}
	void GetSequenceImage       (int& nImage, int& nImageExpand){nImage = m_nSequence; nImageExpand = m_nSequenceExpand;}
	void GetDBEventImage        (int& nImage, int& nImageExpand){nImage = m_nDBEvent; nImageExpand = m_nDBEventExpand;}
	void GetSynonymImage        (int& nImage, int& nImageExpand){nImage = m_nSynonym; nImageExpand = m_nSynonymExpand;}
	void GetColumnImage         (int& nImage, int& nImageExpand){nImage = m_nColumn; nImageExpand = m_nColumnExpand;}
	void GetIndexImage          (int& nImage, int& nImageExpand){nImage = m_nIndex; nImageExpand = m_nIndexExpand;}
	void GetRuleImage           (int& nImage, int& nImageExpand){nImage = m_nRule; nImageExpand = m_nRuleExpand;}
	void GetIntegrityImage      (int& nImage, int& nImageExpand){nImage = m_nIntegrity; nImageExpand = m_nIntegrityExpand;}

	void SetEmptyImage          (int nImage, int nImageExpand){m_nEmpty= nImage; m_nEmptyExpand= nImageExpand;}
	void SetFolderImage         (int nImage, int nImageExpand){m_nFolder= nImage; m_nFolderExpand= nImageExpand;}
	void SetNodeImage           (int nImage, int nImageExpand){m_nNode= nImage; m_nNodeExpand = nImageExpand;}
	void SetNodeLocalImage      (int nImage, int nImageExpand){m_nNodeLocal= nImage; m_nNodeLocalExpand = nImageExpand;}
	void SetNodeServerImage     (int nImage, int nImageExpand){m_nNodeServer= nImage; m_nNodeServerExpand = nImageExpand;}
	void SetNodeLoginImage      (int nImage, int nImageExpand){m_nNodeLogin =nImage; m_nNodeLoginExpand = nImageExpand;}
	void SetNodeConnectionImage (int nImage, int nImageExpand){m_nNodeConnection = nImage; m_nNodeConnectionExpand = nImageExpand;}
	void SetNodeAttributeImage  (int nImage, int nImageExpand) {m_nNodeAttribute = nImage; m_nNodeAttributeExpand = nImageExpand;}
	void SetUserImage           (int nImage, int nImageExpand){m_nUser= nImage; m_nUserExpand = nImageExpand;}
	void SetGroupImage          (int nImage, int nImageExpand){m_nGroup = nImage; m_nGroupExpand = nImageExpand;}
	void SetProfileImage        (int nImage, int nImageExpand){m_nProfile= nImage;m_nProfileExpand = nImageExpand;}
	void SetRoleImage           (int nImage, int nImageExpand){m_nRole= nImage;m_nRoleExpand = nImageExpand;}
	void SetLocationImage       (int nImage, int nImageExpand){m_nLocation= nImage;m_nLocationExpand = nImageExpand;}
	void SetDatabaseImage       (int nImage, int nImageExpand){m_nDatabase= nImage;m_nDatabaseExpand = nImageExpand;}
	void SetTableImage          (int nImage, int nImageExpand){m_nTable= nImage;m_nTableExpand = nImageExpand;}
	void SetViewImage           (int nImage, int nImageExpand){m_nView= nImage;m_nViewExpand = nImageExpand;}
	void SetProcedureImage      (int nImage, int nImageExpand){m_nProcedure= nImage; m_nProcedureExpand = nImageExpand;}
	void SetDBEventImage        (int nImage, int nImageExpand){m_nDBEvent= nImage; m_nDBEventExpand = nImageExpand;}
	void SetSynonymImage        (int nImage, int nImageExpand){m_nSynonym= nImage; m_nSynonymExpand = nImageExpand;}
	void SetColumnImage         (int nImage, int nImageExpand){m_nColumn= nImage; m_nColumnExpand = nImageExpand;}
	void SetIndexImage          (int nImage, int nImageExpand){m_nIndex= nImage; m_nIndexExpand = nImageExpand;}
	void SetRuleImage           (int nImage, int nImageExpand){m_nRule= nImage; m_nRuleExpand = nImageExpand;}
	void SetIntegrityImage      (int nImage, int nImageExpand){m_nIntegrity= nImage; m_nIntegrityExpand = nImageExpand;}

	void NodeChildrenFolderAddFlag(UINT nFlag);
	void NodeChildrenFolderRemoveFlag(UINT nFlag);
	UINT NodeChildrenFolderGetFlag(){return m_nNodeChildrenFolderFlag;}

	void ServerChildrenFolderAddFlag(UINT nFlag);
	void ServerChildrenFolderRemoveFlag(UINT nFlag);
	UINT ServerChildrenFolderGetFlag(){return m_nServerChildrenFolderFlag;}

	void DBChildrenFolderAddFlag(UINT nFlag);
	void DBChildrenFolderRemoveFlag(UINT nFlag);
	UINT DBChildrenFolderGetFlag(){return m_nDBChildrenFolderFlag;}

	void TableChildrenFolderAddFlag(UINT nFlag);
	void TableChildrenFolderRemoveFlag(UINT nFlag);
	UINT TableChildrenFolderGetFlag(){return m_nTableChildrenFolderFlag;}

	void ViewChildrenFolderAddFlag(UINT nFlag);
	void ViewChildrenFolderRemoveFlag(UINT nFlag);
	UINT ViewChildrenFolderGetFlag(){return m_nTableChildrenFolderFlag;}

	void ProcedureChildrenFolderAddFlag(UINT nFlag);
	void ProcedureChildrenFolderRemoveFlag(UINT nFlag);
	UINT ProcedureChildrenFolderGetFlag(){return m_nProcedureChildrenFolderFlag;}

	void SequenceChildrenFolderAddFlag(UINT nFlag);
	void SequenceChildrenFolderRemoveFlag(UINT nFlag);
	UINT SequenceChildrenFolderGetFlag(){return m_nSequenceChildrenFolderFlag;}

	void DBEventChildrenFolderAddFlag(UINT nFlag);
	void DBEventChildrenFolderRemoveFlag(UINT nFlag);
	UINT DBEventChildrenFolderGetFlag(){return m_nDBEventChildrenFolderFlag;}

	void SetDisplayObjectInFolder (ObjectInFolder nObject, BOOL bSet)
	{
		ASSERT (nObject >= 0 && nObject < NUM_OBJECTFOLDER);
		if (nObject >= 0 && nObject < NUM_OBJECTFOLDER)
			m_bArrayDispayInFolder[nObject] = bSet;
	}
	BOOL IsDisplayObjectInFolder (ObjectInFolder nObject)
	{
		ASSERT (nObject >= 0 && nObject < NUM_OBJECTFOLDER);
		if (nObject >= 0 && nObject < NUM_OBJECTFOLDER)
			return m_bArrayDispayInFolder[nObject];
		return FALSE;
	}


protected:
	CString m_strOWNERxITEM;

	int m_nEmpty;
	int m_nFolder;
	int m_nFolderExpand;

	int m_nEmptyExpand;
	int m_nNode;
	int m_nNodeExpand;
	int m_nNodeLocal;
	int m_nNodeLocalExpand;
	int m_nNodeServer;
	int m_nNodeServerExpand;
	int m_nNodeLogin;
	int m_nNodeLoginExpand;
	int m_nNodeConnection;
	int m_nNodeConnectionExpand;
	int m_nNodeAttribute;
	int m_nNodeAttributeExpand;

	int m_nUser;
	int m_nUserExpand;
	int m_nGroup;
	int m_nGroupExpand;
	int m_nProfile;
	int m_nProfileExpand;
	int m_nRole;
	int m_nRoleExpand;
	int m_nLocation;
	int m_nLocationExpand;
	int m_nDatabase;
	int m_nDatabaseExpand;
	int m_nTable;
	int m_nTableExpand;
	int m_nView;
	int m_nViewExpand;
	int m_nProcedure;
	int m_nProcedureExpand;
	int m_nSequence;
	int m_nSequenceExpand;
	int m_nDBEvent;
	int m_nDBEventExpand;
	int m_nSynonym;
	int m_nSynonymExpand;

	int m_nColumn;
	int m_nColumnExpand;
	int m_nIndex;
	int m_nIndexExpand;
	int m_nRule;
	int m_nRuleExpand;
	int m_nIntegrity;
	int m_nIntegrityExpand;

	//
	// Display the list of object (O_XX) in the Folder
	// if m_bArrayDispayInFolder[O_XX] = TRUE else display at the parent branch:
	BOOL m_bArrayDispayInFolder[NUM_OBJECTFOLDER];

	// Node:
	UINT m_nNodeChildrenFolderFlag;     // Which sub-folder of node is to display ?
	// Node Server:
	UINT m_nServerChildrenFolderFlag;   // Which sub-folder of node server is to display ?
	// Database:
	UINT m_nDBChildrenFolderFlag;       // Which sub-folder of database is to display ?
	// Table:
	UINT m_nTableChildrenFolderFlag  ;  // Which sub-folder of table is to display ?
	// View:
	UINT m_nViewChildrenFolderFlag;     // Which sub-folder of view is to display ?
	// Procedure:
	UINT m_nProcedureChildrenFolderFlag;// Which sub-folder of Procedure is to display ?
	// Sequence:
	UINT m_nSequenceChildrenFolderFlag; // Which sub-folder of Sequence is to display ?
	// DBEvent:
	UINT m_nDBEventChildrenFolderFlag;  // Which sub-folder of DBEvent is to display ?

};


//
// Store the TreeItem State along with the TreeData:
class CaTreeCtrlData
{
public:
	enum ItemState {ITEM_DELETE = -1, ITEM_EXIST, ITEM_NEW};
	CaTreeCtrlData()
	{
		Initialize();
	}

	CaTreeCtrlData(ItemState nFlag, HTREEITEM hTreeItem)
	{
		Initialize();
		m_nItemFlag = nFlag;
		m_hTreeItem = hTreeItem;
	}
	void Initialize()
	{
		m_nItemFlag = ITEM_NEW;
		m_hTreeItem = NULL;
		m_bExpanded = FALSE;
		m_bAlreadyExpanded = FALSE;
		m_nImage = -1;
		m_nExpandedImage = -1;
		m_bDisplay = FALSE;
		m_pTree = NULL;
	}

	~CaTreeCtrlData(){}

	ItemState GetState(){return m_nItemFlag;}
	void SetState(ItemState nFlag){m_nItemFlag = nFlag;}
	HTREEITEM GetTreeItem (){return m_hTreeItem;}
	void SetTreeItem (HTREEITEM hTreeItem)
	{
		m_hTreeItem = hTreeItem;
		if (m_hTreeItem)
			SetDisplay();
		else
			m_bDisplay = FALSE;
	}

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
	void SetDisplay (){m_bDisplay = TRUE;}
	BOOL IsDisplayed(){return m_bDisplay;}
	CTreeCtrl* GetTreeCtrl() {return m_pTree;}
	void SetTreeCtrl(CTreeCtrl* pTree) {m_pTree = pTree;}
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

	BOOL m_bDisplay;
	CTreeCtrl* m_pTree;

};


class CaRefreshTreeInfo
{
public:
	typedef enum {ACTION_QUERY=0, ACTION_REFRESH} RefreshAction;
	CaRefreshTreeInfo(RefreshAction nAction = ACTION_QUERY): m_nLevel(0), m_nAction(nAction) {}
	~CaRefreshTreeInfo(){}

	RefreshAction GetAction(){return m_nAction;}
	void SetAction(RefreshAction nAction){m_nAction = nAction;}
	void SetInfo (int nLevel = 0)
	{
		m_nLevel = nLevel;
	}

	//
	// m_nLevel = 0, this branch
	//          = 1, this branch and one level sub-branch
	//          =-1, this branch and all sub-branch.
	int m_nLevel;
	RefreshAction m_nAction;
};

class CmtSessionManager;
class CtrfItemData: public CObject
{
	DECLARE_SERIAL (CtrfItemData)
public:
	CtrfItemData(TreeItemType itemType= O_BASE_ITEM);
	CtrfItemData(LPCTSTR lpszDisplayString, TreeItemType itemType= O_BASE_ITEM);
	virtual ~CtrfItemData();

	static HTREEITEM TreeAddItem (LPCTSTR lpszItem, CTreeCtrl* pTree, HTREEITEM hParent, HTREEITEM hInsertAfter, int nImage, DWORD dwData);
	static void DisplayEmptyOrErrorNode (CtrfItemData* pEmptyNode, CTreeCtrl* pTree, HTREEITEM hParent = NULL);
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent = NULL);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand)
	{
		m_treeCtrlData.SetExpand(TRUE);
		if (pTree)
		{
			pTree->SetItemImage(hExpand, m_treeCtrlData.GetImageExpanded(), m_treeCtrlData.GetImageExpanded());
		}
		m_treeCtrlData.SetAlreadyExpanded();
	}
	virtual void Collapse(CTreeCtrl* pTree, HTREEITEM hExpand)
	{
		m_treeCtrlData.SetExpand(FALSE);
		if (pTree)
		{
			pTree->SetItemImage(hExpand, m_treeCtrlData.GetImage(),  m_treeCtrlData.GetImage());
		}
	}
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo){ASSERT (FALSE); return TRUE;}
	virtual BOOL IsRefreshCurrentBranchEnabled() {return FALSE;}
	virtual BOOL IsRefreshCurrentSubBranchEnabled() {return FALSE;}
	virtual BOOL IsFolder() {return FALSE;}

	CaTreeCtrlData& GetTreeCtrlData() {return m_treeCtrlData;}
	virtual CaDisplayInfo* GetDisplayInfo(){return m_pBackParent? m_pBackParent->GetDisplayInfo(): NULL;}

	// GetFolderFlag():
	// Dirived class should return FOLDER_XXX (Ex: FOLDER_DATABASE, ...)
	virtual UINT GetFolderFlag(){return 0;}
	virtual CtrfItemData* GetEmptyNode(){return NULL;} 
	// SetImageID():
	// Dirived class must overwride this funtion
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo){ASSERT(FALSE);} 
	// IsDisplayObjectInfolder():
	// If this object is a folder, Dirived class can overwride this funtion.
	virtual BOOL IsDisplayObjectInFolder(CaDisplayInfo* pDisplayInfo){return TRUE;} 
	// GetDisplayObjectFlag()
	// Derived class should overwrite this function to the folders
	// are to be displayed in this object.
	// It must call DBChildrenFolderGetFlag or NodeChildrenFolderGetFlag, ...
	virtual UINT GetDisplayObjectFlag(CaDisplayInfo* pDisplayInfo){return 0;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return TRUE;} 

	virtual CmtSessionManager* GetSessionManager(){return m_pBackParent? m_pBackParent->GetSessionManager(): NULL;}
	virtual void SetBackParent(CtrfItemData* pBackPointer){m_pBackParent = pBackPointer;}
	virtual CtrfItemData* GetBackParent(){return m_pBackParent;}
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL){return m_pBackParent? m_pBackParent->GetQueryInfo(pData): NULL;}

	virtual PfnCOMQueryObject  GetPfnCOMQueryObject( )
	{
		return m_pBackParent? m_pBackParent->GetPfnCOMQueryObject(): NULL;
	}
	virtual PfnUserQueryObject GetPfnUserQueryObject()
	{
		return m_pBackParent? m_pBackParent->GetPfnUserQueryObject(): NULL;
	}
	virtual IUnknown* GetAptAccess(){return m_pBackParent? m_pBackParent->GetAptAccess(): NULL;}

	virtual CString GetDisplayedItem(int nMode = 0){return m_strDisplayString;}
	virtual CString GetErrorString(){return m_strErrorString;}
	virtual void SetDisplayedItem(LPCTSTR lpszDisplayItem){m_strDisplayString = lpszDisplayItem;}
	virtual void SetErrorString  (LPCTSTR lpszError){m_strErrorString = lpszError;}

	//
	// The derived class must implement:
	virtual CaDBObject* GetDBObject();
	virtual TreeItemType GetTreeItemType(){return m_treeItemType;}
	void Initialize(){};
	virtual void Reset();
	virtual BOOL DisplayItem(){return m_bDisplayThisItem;}
	virtual void DisplayItem(BOOL bDisplay){m_bDisplayThisItem = bDisplay;}
	void UnDisplayItems(CArray<TreeItemType, TreeItemType>& arrayItem);
	//
	// Allow the derived class the mean of handling its specific data.
	virtual void* GetUserData(){return NULL;}
	CTypedPtrList< CObList, CtrfItemData* >& GetListObject() {return m_listObject;}

protected:
	TreeItemType m_treeItemType;
	BOOL m_bDisplayThisItem;
	BOOL m_bError;
	CaTreeCtrlData m_treeCtrlData;
	CtrfItemData* m_pBackParent;
	CString m_strDisplayString;
	CString m_strErrorString;
	//
	// List of Objects or Folders:
	CTypedPtrList< CObList, CtrfItemData* > m_listObject;

private:
	CaDBObject* m_pDBObject;
};



#endif // TREECONTROLDATA_HEADER