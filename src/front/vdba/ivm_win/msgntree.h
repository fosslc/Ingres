/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.f
*/

/*
**  Source   : msgntree.h, Header file
**
**  Project  : Visual Manager
**
**  Author   : UK Sotheavut
**
**  Purpose  : Define Message Categories and Notification Levels
**		Defenition of Tree Node of Message Category
**
**    History:
**	21-May-1999 (uk$so01)
**	    Created.
**	04-feb-2000 (somsa01)
**	    Took out extra "," in Drag Drop type definition.
*/

#if !defined(AFX_MSGNTREE_H__HEADER)
#define AFX_MSGNTREE_H__HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "msgcateg.h"


//
// Sort order of Category View:
typedef enum
{
	ORDER_LEAF = 1,
	ORDER_ALERT,
	ORDER_NOTIFY,
	ORDER_DISCARD,
	ORDER_USER,
	ORDER_OTHERS
} CategoryViewOrder;

enum
{
	IMGS_FOLDER_CLOSE = 0,
	IMGS_FOLDER_OPEN,
	IMGS_FOLDER_A_CLOSE,
	IMGS_FOLDER_A_OPEN,
	IMGS_FOLDER_N_CLOSE,
	IMGS_FOLDER_N_OPEN,
	IMGS_FOLDER_D_CLOSE,
	IMGS_FOLDER_D_OPEN,
	IMGS_ITEM_A,
	IMGS_ITEM_N,
	IMGS_ITEM_D
};

//
// Drag drop type:
// CV_: CATEGORY VIEW
// SV_: STATE VIEW
typedef enum 
{
	DD_UNDEFINED = 0,
	CV_R,     // Root
	CV_O,     // Item of Other Message
	CV_OF,    // Main folder of Other messages <Others>
	CV_OFA,   // Sub-folder <Alert> of main folder <Others>
	CV_OFN,   // Sub-folder <Notify> of main folder <Others>
	CV_OFD,   // Sub-folder <Discard> of main folder <Others>

	CV_U,     // Item of Message
	CV_UF,    // User defined folder
	CV_UFA,   // Sub-folder <Alert> of User defined folder or Root
	CV_UFN,   // Sub-folder <Notify> of User defined folder or Root
	CV_UFD,   // Sub-folder <Discard> of User defined folder or Root

	SV_R,     // Root
	SV_O,     // Item of Other Message
	SV_OFA,   // Sub-folder <Others> of main folder <Alert>
	SV_OFN,   // Sub-folder <Others> of main folder <Notify>
	SV_OFD,   // Sub-folder <Others> of main folder <Discard>

	SV_U,     // Item of Message
	SV_FA,    // main folder <Alert>
	SV_FN,    // main folder <Notify>
	SV_FD,    // main folder <Discard>
	SV_UFA,   // User defined folder in the main folder <Alert>
	SV_UFN,   // User defined folder in the main folder <Notify>
	SV_UFD    // User defined folder in the main folder <Discard>
} MSGSourceTarget;

//
// Drag / Drop Action:
#define DD_ACTION_NONE      0x0000
#define DD_ACTION_ALERT     0x0001
#define DD_ACTION_NOTIFY    0x0002
#define DD_ACTION_DISCARD   0x0004
#define DD_ACTION_MOVE      0x0008
#define DD_ACTION_COPY      0x0010

class CaMessageNodeInfo: public CObject
{
	DECLARE_SERIAL (CaMessageNodeInfo)
public:
	CaMessageNodeInfo():
		m_hTreeItem(NULL),
		m_bExpanded(FALSE),
		m_nodeType (NODE_NEW),
		m_nOrder(ORDER_LEAF)
	{
		m_nImageClose = -1;
		m_nImageOpen = -1;
		m_nSourceTarget = DD_UNDEFINED;
	}
	~CaMessageNodeInfo(){}

	int GetIconImage(BOOL bTerminal = FALSE);
	int GetStateImage(BOOL bTerminal = FALSE);

	//
	// Expand / Collapse:
	void SetExpanded(BOOL bExpanded){m_bExpanded = bExpanded;}
	BOOL IsExpanded(){return m_bExpanded;}
	//
	// Tree Item
	void SetHTreeItem(HTREEITEM hTreeItem){m_hTreeItem = hTreeItem;}
	HTREEITEM GetHTreeItem(){return m_hTreeItem;}
	//
	// Node Type:
	void SetNodeType (NODEINFOTYPE nType){m_nodeType = nType;}
	NODEINFOTYPE GetNodeType(){return m_nodeType;}
	//
	// Node Title:
	void SetNodeTitle (LPCTSTR lpszTitle){m_strNodeTitle = lpszTitle;}
	void SetNodeTitle (int nIDS){m_strNodeTitle.LoadString(nIDS);}
	CString GetNodeTitle(){return m_strNodeTitle;}
	//
	// Sort order:
	void SetOrder(CategoryViewOrder nOrder){m_nOrder = nOrder;}
	CategoryViewOrder GetOrder(){return m_nOrder;}
	//
	// Icon Images:
	void SetIconImage (int nClose, int nOpen){m_nImageClose = nClose; m_nImageOpen = nOpen;}
	void GetIconImage (int& nClose, int &nOpen){nClose = m_nImageClose; nOpen = m_nImageOpen;}
	//
	// Drag/Drop (Compatible Source / Target):
	void SetMsgSourceTarget (MSGSourceTarget s){m_nSourceTarget = s;}
	MSGSourceTarget GetMsgSourceTarget(){return m_nSourceTarget;}

protected:
	int m_nImageClose;
	int m_nImageOpen;
	HTREEITEM m_hTreeItem;
	BOOL m_bExpanded;
	NODEINFOTYPE m_nodeType;
	CString m_strNodeTitle;
	CategoryViewOrder m_nOrder;
	MSGSourceTarget m_nSourceTarget;
};


#define SHOW_CHANGED          0x0001
#define SHOW_STATE_BRANCH     0x0002
#define SHOW_CATEGORY_BRANCH  0x0004

//
// A general base class of Node of Tree's Message:
class CaMessageNode :public CObject
{
public:
	CaMessageNode(){};
	virtual ~CaMessageNode(){};
	virtual void ResetTreeCtrlData(){};
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0){}
	virtual void CleanData(){}
	virtual void AddFolder (CTreeCtrl* pTree){}
	virtual void AlertMessage(CTreeCtrl* pTree){};
	virtual void NotifyMessage(CTreeCtrl* pTree){};
	virtual void DiscardMessage(CTreeCtrl* pTree){};

	virtual void AddMessage (CaMessageNode* pNode){ASSERT (FALSE);}
	virtual void DeleteMessage (CaMessageNode* pNode){ASSERT (FALSE);}

	virtual BOOL IsInternal(){return FALSE;}
	virtual void SortItems(CTreeCtrl* pTree);

	CaMessageNodeInfo& GetNodeInformation(){return m_nodeInfo;}

protected:
	CaMessageNodeInfo m_nodeInfo;
};




//
// LEAF: A general class of Node of Tree's Message Category:
class CaMessageNodeTree :public CaMessageNode
{
public:
	CaMessageNodeTree();
	CaMessageNodeTree(CaMessage* pMessage);
	virtual ~CaMessageNodeTree();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void NotifyMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);

	//
	// Message Category Member:
	void SetMessageCategory(CaMessage* pMessage){m_pMessageCategory = pMessage;}
	CaMessage* GetMessageCategory(){return m_pMessageCategory;}

protected:
	CaMessage* m_pMessageCategory;
};




//
// INTERNAL: A general class of Internal Node of Tree's Message Category:
class CaMessageNodeTreeInternal :public CaMessageNode
{
public:
	CaMessageNodeTreeInternal();
	virtual ~CaMessageNodeTreeInternal();
	virtual void AddMessage (CaMessageNode* pNode);
	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void NotifyMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);
	
	virtual BOOL IsInternal(){return TRUE;}
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	virtual void AddFolder (CTreeCtrl* pTree);

	CTypedPtrList<CObList, CaMessageNode*>* GetListSubNode(){return m_pListSubNode;}
	void SetListSubNode(CTypedPtrList<CObList, CaMessageNode*>* pList){m_pListSubNode = pList;}
protected:
	//
	// If this member is NULL, then the current node has no children.
	CTypedPtrList<CObList, CaMessageNode*>* m_pListSubNode;

};



// ********************* Tree of State View ******************** 
//

class CaTreeMessageCategory;
//
// STATE VIEW: An Internal Node (Others) of Tree's Message Category:
class CaMessageNodeTreeStateOthers :public CaMessageNodeTreeInternal
{
public:
	CaMessageNodeTreeStateOthers();
	virtual ~CaMessageNodeTreeStateOthers();
	//
	// Add folder is not allowed:
	virtual void AddFolder (CTreeCtrl* pTree){}

/*
	virtual void AlertMessage();
	virtual void NotifyMessage();
	virtual void DiscardMessage();
*/	
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	virtual void SortItems(CTreeCtrl* pTree);

	virtual void CleanData();
	void Populate (CTypedPtrList<CObList, CaMessageNode*>* pListOthers, Imsgclass nType);
	void SetClassification(Imsgclass nc){m_nClass = nc;}
protected:
	Imsgclass m_nClass;
};


//
// STATE VIEW: An Internal Node (Alert) of Tree's Message Category:
class CaMessageNodeTreeStateAlert :public CaMessageNodeTreeInternal
{
public:
	CaMessageNodeTreeStateAlert();
	virtual ~CaMessageNodeTreeStateAlert();
	//
	// Add folder is not allowed:
	virtual void AddFolder (CTreeCtrl* pTree){}

	virtual void NotifyMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);

	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	virtual void CleanData();
	virtual void SortItems(CTreeCtrl* pTree);
	
	void Populate (CTreeCtrl* pTree, CaTreeMessageCategory* pFrom);

	CaMessageNodeTreeStateOthers& GetCategoryOthers(){return m_categoryOthers;}
protected:
	CaMessageNodeTreeStateOthers m_categoryOthers;

};



//
// STATE VIEW: An Internal Node (Notify) of Tree's Message Category:
class CaMessageNodeTreeStateNotify :public CaMessageNodeTreeInternal
{
public:
	CaMessageNodeTreeStateNotify();
	virtual ~CaMessageNodeTreeStateNotify();
	//
	// Add folder is not allowed:
	virtual void AddFolder (CTreeCtrl* pTree){}

	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);

	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	virtual void SortItems(CTreeCtrl* pTree);
	virtual void CleanData();
	void Populate (CTreeCtrl* pTree, CaTreeMessageCategory* pFrom);
	
	CaMessageNodeTreeStateOthers& GetCategoryOthers(){return m_categoryOthers;}
protected:
	CaMessageNodeTreeStateOthers m_categoryOthers;
};


//
// STATE VIEW: An Internal Node (Discard) of Tree's Message Category:
class CaMessageNodeTreeStateDiscard :public CaMessageNodeTreeInternal
{
public:
	CaMessageNodeTreeStateDiscard();
	virtual ~CaMessageNodeTreeStateDiscard();
	//
	// Add folder is not allowed:
	virtual void AddFolder (CTreeCtrl* pTree){}
	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void NotifyMessage(CTreeCtrl* pTree);

	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	virtual void SortItems(CTreeCtrl* pTree);
	virtual void CleanData();
	void Populate (CTreeCtrl* pTree, CaTreeMessageCategory* pFrom);

	CaMessageNodeTreeStateOthers& GetCategoryOthers(){return m_categoryOthers;}
protected:
	CaMessageNodeTreeStateOthers m_categoryOthers;
};


//
// STATE VIEW: An Internal Node (Folder & Sub Folder) of Tree's Message Category:
class CaMessageNodeTreeStateFolder :public CaMessageNodeTreeInternal
{
public:
	CaMessageNodeTreeStateFolder();
	virtual ~CaMessageNodeTreeStateFolder();
	//
	// Add folder is not allowed:
	virtual void AddFolder (CTreeCtrl* pTree){}

/*
	virtual void AlertMessage();
	virtual void NotifyMessage();
	virtual void DiscardMessage();
*/

	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	virtual void CleanData(){};

	void UpdateTree (CaMessageNode* pMsgNode, Imsgclass nClass);

};

//
// MAIN TREE of STATE VIEW:
class CaTreeMessageState: public CaMessageNodeTreeInternal
{
public:
	CaTreeMessageState();
	virtual ~CaTreeMessageState();

	//
	// Display the nodes (Create the node HTREEITEM and display the item): 
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	static int CALLBACK CompareItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	virtual void SortItems(CTreeCtrl* pTree);

	//
	// Show / Hide Category:
	void SetShowCategory(BOOL bShowCategory){m_bShowCategory = bShowCategory;}
	BOOL IsShowCategory(){return m_bShowCategory;}

	void UpdateTree (CTreeCtrl* pTree, CaTreeMessageCategory* pFrom);
protected:
	BOOL m_bShowCategory;

	CaMessageNodeTreeStateAlert   m_nodeAlert;
	CaMessageNodeTreeStateNotify  m_nodeNotify;
	CaMessageNodeTreeStateDiscard m_nodeDiscard;


	void CleanData(){};

};














// ********************* Tree of Category View ******************** 
//

//
// CATEGORY VIEW: An Internal Node (Alert) of Tree's Message Category:
class CaMessageNodeTreeCategoryAlert :public CaMessageNode
{
public:
	CaMessageNodeTreeCategoryAlert();
	virtual ~CaMessageNodeTreeCategoryAlert();
	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void NotifyMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);

	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);

};



//
// CATEGORY VIEW: An Internal Node (Notify) of Tree's Message Category:
class CaMessageNodeTreeCategoryNotify :public CaMessageNode
{
public:
	CaMessageNodeTreeCategoryNotify();
	virtual ~CaMessageNodeTreeCategoryNotify();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);

	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void NotifyMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);

};



//
// CATEGORY VIEW: An Internal Node (Discard) of Tree's Message Category:
class CaMessageNodeTreeCategoryDiscard :public CaMessageNode
{
public:
	CaMessageNodeTreeCategoryDiscard();
	virtual ~CaMessageNodeTreeCategoryDiscard();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);

	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void NotifyMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);

};


//
// CATEGORY VIEW: An Internal Node (Folder & Sub Folder) of Tree's Message Category:
class CaMessageNodeTreeCategoryFolder :public CaMessageNodeTreeInternal
{
public:
	CaMessageNodeTreeCategoryFolder();
	virtual ~CaMessageNodeTreeCategoryFolder();
	virtual void AddFolder (CTreeCtrl* pTree);
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	virtual void ResetTreeCtrlData();

	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void NotifyMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);

	virtual CaMessageNodeTreeCategoryFolder* FindFolder (LPCTSTR lpszFolder);
	void SortItems(CTreeCtrl* pTree);

protected:
	CaMessageNodeTreeCategoryAlert m_FolderAlert;
	CaMessageNodeTreeCategoryNotify m_FolderNotify;
	CaMessageNodeTreeCategoryDiscard m_FolderDiscard;

};


//
// CATEGORY VIEW: An Internal Node (Others) of Tree's Message Category:
class CaMessageNodeTreeCategoryOthers :public CaMessageNodeTreeCategoryFolder
{
public:
	CaMessageNodeTreeCategoryOthers();
	virtual ~CaMessageNodeTreeCategoryOthers();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);
	//
	// Add folder is not allowed:
	virtual void AddFolder (CTreeCtrl* pTree){}

	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void NotifyMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);

	virtual void CleanData();
	void Populate (CTypedPtrList<CObList, CaMessage*>& listMessageCategory);
	void SortItems(CTreeCtrl* pTree);
private:

};




//
// MAIN TREE of CATEGORY VIEW:
class CaTreeMessageCategory: public CaMessageNodeTreeCategoryFolder
{
public:
	CaTreeMessageCategory();
	virtual ~CaTreeMessageCategory();
	void GenerateUserFolder (CaCategoryDataUserManager& usrFolder);
	
	void Populate (CTypedPtrList<CObList, CaMessage*>& listMessageCategory);
	//
	// Populate the unclassified (others) message:
	void Populate (CaMessageManager* pData);
	//
	// Populate the user specialized message category:
	void Populate (CaMessageManager* pMsgEntry, CaCategoryDataUserManager& data);

	void AddBranch (CString strPath);
	void AddLeaf (CaMessageManager* pMsgEntry, CString strPath, long lCategory, long lCode);

	virtual void AlertMessage(CTreeCtrl* pTree);
	virtual void NotifyMessage(CTreeCtrl* pTree);
	virtual void DiscardMessage(CTreeCtrl* pTree);
	int CountMessage(CaMessage* pMessage);
	CaMessageNode* FindFirstMessage (CaMessage* pMessage);

	//
	// Display the nodes (Create the node HTREEITEM and display the item): 
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hTree, UINT nMode = 0);

//	virtual void AddFolder (CTreeCtrl* pTree);
	CaMessageNodeTreeCategoryOthers& GetCategoryOthers(){return m_categoryOthers;}

	static int CALLBACK CompareItem (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	virtual void SortItems(CTreeCtrl* pTree);
	//
	// Show / Hide State Branches:
	void SetShowStateBranches(BOOL bShowStateBranches){m_bShowStateBranches = bShowStateBranches;}
	BOOL IsShowStateBranches(){return m_bShowStateBranches;}

	void Output(CaMessageNode* pData, int nLevel = 0);
	void Output2(CaMessageNode* pData, int nLevel = 0, LPCTSTR lpszPath = _T(""));
protected:
	BOOL m_bShowStateBranches;
	CaMessageNodeTreeCategoryOthers m_categoryOthers;

	void CleanData(){};
	void UndisplayFolder(CTreeCtrl* pTree);

};


#endif // !defined(AFX_MSGNTREE_H__HEADER)
