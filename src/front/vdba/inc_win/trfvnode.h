/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlvnode.h: interface for the CtrfNode, CtrfNodeXxx, 
**               CaFolderCaNode, CaFolderCaNodeXxx class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Node object and folder
**
** History:
**
** 11-Feb-1998 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFVNODE_HEADER)
#define TRFVNODE_HEADER
#include "imageidx.h" // 0-based index of ingres image list
#include "constdef.h" // FOLDER_XXX

#include "trfdbase.h" // Database
#include "trfuser.h"  // User
#include "trfgroup.h" // Group
#include "trfprofi.h" // Profile
#include "trfrole.h"  // Role
#include "trflocat.h" // Location
#include "dmlvnode.h" // CaNode, CaNodeXxx


//
// Object: Node Attribute
// **********************
class CtrfNodeAttribute: public CtrfItemData
{
	DECLARE_SERIAL(CtrfNodeAttribute)
public:
	CtrfNodeAttribute(): CtrfItemData(O_NODE_ATTRIBUTE){}
	CtrfNodeAttribute(CaNodeAttribute* pObj): CtrfItemData(O_NODE_ATTRIBUTE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfNodeAttribute(){}
	virtual void Initialize(){};
	virtual CString GetDisplayedItem(int nMode = 0)
	{
		CString strDisplay;
		strDisplay.Format(_T("%s:%s"), (LPCTSTR)m_object.GetAttributeName(), (LPCTSTR)m_object.GetAttributeValue());
		return strDisplay;
	}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetNodeLoginImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaNodeAttribute& GetObject(){return m_object;}
	void GetObject(CaNodeAttribute*& pObject){pObject = new CaNodeAttribute(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaNodeAttribute m_object;
};


//
// Object: Folder Node Attribute
// *****************************
class CtrfFolderNodeAttribute: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderNodeAttribute)
public:
	CtrfFolderNodeAttribute();
	virtual ~CtrfFolderNodeAttribute();

	virtual void Initialize(){};
	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);

	CtrfItemData* SearchObject (CaNodeAttribute* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
protected:

	CtrfItemData m_EmptyNode;
};


//
// Object: Node Login
// ******************
class CtrfNodeLogin: public CtrfItemData
{
	DECLARE_SERIAL(CtrfNodeLogin)
public:
	CtrfNodeLogin(): CtrfItemData(O_NODE_LOGIN){}
	CtrfNodeLogin(CaNodeLogin* pObj): CtrfItemData(O_NODE_LOGIN)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfNodeLogin(){}
	virtual void Initialize(){};
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetNodeLoginImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaNodeLogin& GetObject(){return m_object;}
	void GetObject(CaNodeLogin*& pObject){pObject = new CaNodeLogin(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaNodeLogin m_object;
};


//
// Object: Folder Node Login
// *************************
class CtrfFolderNodeLogin: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderNodeLogin)
public:
	CtrfFolderNodeLogin();
	virtual ~CtrfFolderNodeLogin();

	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);

	CtrfItemData* SearchObject (CaNodeLogin* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
protected:
	CtrfItemData m_EmptyNode;

};


//
// Object: Node Connection Data
// ****************************
class CtrfNodeConnectData: public CtrfItemData
{
	DECLARE_SERIAL(CtrfNodeConnectData)
public:
	CtrfNodeConnectData(): CtrfItemData(O_NODE_CONNECTION){}
	CtrfNodeConnectData(CaNodeConnectData* pObj): CtrfItemData(O_NODE_CONNECTION)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfNodeConnectData(){}
	virtual void Initialize(){};
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetNodeConnectionImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaNodeConnectData& GetObject(){return m_object;}
	void GetObject(CaNodeConnectData*& pObject){pObject = new CaNodeConnectData(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaNodeConnectData m_object;
};


//
// Object: Folder of Node Connection Data
// ***************************************
class CtrfFolderNodeConnectData: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderNodeConnectData)
public:
	CtrfFolderNodeConnectData();
	virtual ~CtrfFolderNodeConnectData();

	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);

	CtrfItemData* SearchObject (CaNodeConnectData* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
protected:
	CtrfItemData m_EmptyNode;

};

//
// Object: Node Server
// *******************
class CtrfNodeServer: public CtrfItemData
{
	DECLARE_SERIAL(CtrfNodeServer)
public:
	CtrfNodeServer(): CtrfItemData(O_NODE_SERVER){}
	CtrfNodeServer(CaNodeServer* pObj): CtrfItemData(O_NODE_SERVER)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfNodeServer(){}
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Initialize();
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	CaNodeServer& GetObject(){return m_object;}
	void GetObject(CaNodeServer*& pObject){pObject = new CaNodeServer(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaNodeServer m_object;
};


//
// Object: Folder of Node Server
// *****************************
class CtrfFolderNodeServer: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderNodeServer)
public:
	CtrfFolderNodeServer();
	virtual ~CtrfFolderNodeServer();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual UINT GetFolderFlag(){return FOLDER_SERVER;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_NODE_SERVER);}

	CtrfItemData* SearchObject (CaNodeServer* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}

protected:
	CtrfItemData m_EmptyNode;

};


//
// Advanced Node Folder:
// ********************
class CtrfFolderNodeAdvance: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderNodeAdvance)
public:
	CtrfFolderNodeAdvance();
	virtual ~CtrfFolderNodeAdvance();

	virtual BOOL IsFolder() {return TRUE;}
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);


	CtrfFolderNodeLogin&  GetFolderNodeLogin(){return *m_folderNodeLogin;}
	CtrfFolderNodeConnectData& GetFolderNodeConnectData() {return *m_folderNodeConnectData;}
	CtrfFolderNodeAttribute& GetFolderNodeAttribute(){return *m_folderNodeAttribute;}

protected:

	CtrfFolderNodeLogin*       m_folderNodeLogin;
	CtrfFolderNodeConnectData* m_folderNodeConnectData;
	CtrfFolderNodeAttribute*   m_folderNodeAttribute;

};



//
// Object: Node 
// ************
class CtrfNode: public CtrfItemData
{
	DECLARE_SERIAL(CtrfNode)
public:
	CtrfNode(): CtrfItemData(O_NODE){}
	CtrfNode(CaNode* pObj): CtrfItemData(O_NODE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}

	void Initialize();
	virtual ~CtrfNode();
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual UINT GetDisplayObjectFlag(CaDisplayInfo* pDisplayInfo);

	CaNode& GetObject(){return m_object;}
	void GetObject(CaNode*& pObject){pObject = new CaNode(m_object);}
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);


	CaDBObject* GetDBObject(){return &m_object;}
protected:

	//
	// Implementation:
protected:
	CaNode m_object;

};


//
// Object: Folder of Node
// ***********************
class CtrfFolderNode: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderNode)
public:
	CtrfFolderNode();
	virtual ~CtrfFolderNode();

	//
	// You must called initialize() befor starting to use cache !
	virtual BOOL Initialize();

	virtual void Display (CTreeCtrl* pTree,  HTREEITEM hParent = NULL);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void SetSessionManager(CmtSessionManager* pSessionManager){m_pSessionManager=pSessionManager;}
	virtual CmtSessionManager* GetSessionManager(){return m_pSessionManager;}
	virtual CaDisplayInfo* GetDisplayInfo(){return &m_displayInfo;}
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL)
	{
		//
		// The systems object flags must not be re-initialized
		BOOL bSystemObject = m_queryInfo.GetIncludeSystemObjects();

		m_queryInfo.Initialize();
		//
		// Exception: root item must be called with pData != NULL !
		ASSERT (pData);
		if (pData)
			m_queryInfo = *pData;

		//
		// The systems object flags must not be re-initialized
		m_queryInfo.SetIncludeSystemObjects(bSystemObject);
		return &m_queryInfo;
	}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_NODE);}

	CtrfItemData* SearchObject (CaNode* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}


	//
	// Management of Query Objects through the COM Server:
	IUnknown* GetAptAccess(){return m_pAptAccess;}
	PfnCOMQueryObject GetPfnCOMQueryObject(){return m_pfnCOMQueryObject;}
	void SetFunctionQueryObject (IUnknown* pAptAccess, PfnCOMQueryObject pfn)
	{
		m_pAptAccess = pAptAccess;
		m_pfnCOMQueryObject = pfn;
	}

	//
	// Management the Query Objects through the specific user defined function:
	void SetFunctionQueryObject (PfnUserQueryObject pfn){m_pfnUserQueryObject = pfn;}
	PfnUserQueryObject GetPfnUserQueryObject()
	{
		return m_pfnUserQueryObject;
	}


protected:
	CmtSessionManager* m_pSessionManager; // If the Query Object is not done through ICAS COM Server.
	CaDisplayInfo  m_displayInfo;         // Information for displaying tree folders
	CaLLQueryInfo  m_queryInfo;           // Query Information.
	CtrfItemData m_EmptyNode;             // Dummy Item ("<Data Unavailable>" or "<No Nodes>")
	CTypedPtrList< CObList, CtrfNode* > m_listObject; // List of Nodes

	PfnCOMQueryObject m_pfnCOMQueryObject;      // Callback function to query Objects (through COM Server)
	PfnUserQueryObject m_pfnUserQueryObject;    // Callback function to query Objects (user handler)
	IUnknown* m_pAptAccess;

};


#endif // TRFVNODE_HEADER
