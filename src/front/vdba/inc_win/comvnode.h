/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comvnode.h: Interface for the 
**               CmtNode, CmtNodeXXX, CmtFolderNode, CmtFolderNodeXXX classes
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Node object and folder
**
** History:
**
** 21-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMVNODE_HEADER)
#define COMVNODE_HEADER
#include "combase.h"
#include "dmlvnode.h"


//
// Forward declaration:
// ********************
class CmtNode;
class CmtFolderNode;
class CmtFolderNodeServer;
class CmtFolderNodeAttribute;
class CmtFolderNodeLogin;
class CmtFolderNodeConnectData;
class CmtAccount;

#include "comdbase.h"
#include "comuser.h"
#include "comrole.h"
#include "comprofi.h"
#include "comgroup.h"
#include "comlocat.h"



//
// Object: Node Attribute
// **********************
class CmtNodeAttribute: public CmtItemData, public CmtProtect
{
public:
	CmtNodeAttribute():CmtItemData(), CmtProtect(){}
	CmtNodeAttribute(CaNodeAttribute* pObj):m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtNodeAttribute(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderNodeAttribute* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderNodeAttribute* GetBackParent(){return m_pBackParent;}
	CaNodeAttribute& GetNodeAttribute(){return m_object;}
	void GetNodeAttribute(CaNodeAttribute*& pObject){pObject = new CaNodeAttribute(m_object);}

protected:
	CaNodeAttribute m_object;
	CmtFolderNodeAttribute* m_pBackParent;
};


//
// Object: Folder Node Attribute
// *****************************
class CmtFolderNodeAttribute: public CmtItemData, public CmtProtect
{
public:
	CmtFolderNodeAttribute():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderNodeAttribute();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtNodeAttribute* SearchObject (CaLLQueryInfo* pInfo, CaNodeAttribute* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

protected:
	CTypedPtrList< CObList, CmtNodeAttribute* > m_listObject;
	CmtNode* m_pBackParent;

};




//
// Object: Node Login
// ******************
class CmtNodeLogin: public CmtItemData, public CmtProtect
{
public:
	CmtNodeLogin():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtNodeLogin(CaNodeLogin* pObj):m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtNodeLogin(){};
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderNodeLogin* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderNodeLogin* GetBackParent(){return m_pBackParent;}
	CaNodeLogin& GetNodeLogin(){return m_object;}
	void GetNodeLogin(CaNodeLogin*& pObject){pObject = new CaNodeLogin(m_object);}
protected:
	CaNodeLogin m_object;
	CmtFolderNodeLogin* m_pBackParent;
};

//
// Object: Folder Node Login
// *************************

class CmtFolderNodeLogin: public CmtItemData, public CmtProtect
{
public:
	CmtFolderNodeLogin():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderNodeLogin();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtNodeLogin* SearchObject (CaLLQueryInfo* pInfo, CaNodeLogin* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

protected:
	CTypedPtrList< CObList, CmtNodeLogin* > m_listObject;
	CmtNode* m_pBackParent;

};


//
// Object: Node Connection Data
// ****************************
class CmtNodeConnectData: public CmtItemData, public CmtProtect
{
public:
	CmtNodeConnectData():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtNodeConnectData(CaNodeConnectData* pObj):m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtNodeConnectData(){};
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderNodeConnectData* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderNodeConnectData* GetBackParent(){return m_pBackParent;}
	CaNodeConnectData& GetNodeConnectData(){return m_object;}
	void GetNodeConnectData(CaNodeConnectData*& pObject){pObject = new CaNodeConnectData(m_object);}
protected:
	CaNodeConnectData m_object;
	CmtFolderNodeConnectData* m_pBackParent;
};



//
// Object: Folder Node Connection Data
// ***********************************
class CmtFolderNodeConnectData: public CmtItemData, public CmtProtect
{
public:
	CmtFolderNodeConnectData():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderNodeConnectData();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtNodeConnectData* SearchObject (CaLLQueryInfo* pInfo, CaNodeConnectData* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

protected:
	CTypedPtrList< CObList, CmtNodeConnectData* > m_listObject;
	CmtNode* m_pBackParent;
};



//
// Object: Node Server
// *******************
class CmtNodeServer: public CmtItemData, public CmtProtect
{
public:
	CmtNodeServer(CaNodeServer* pObj):m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtNodeServer():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtNodeServer(){};
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderNodeServer* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderNodeServer* GetBackParent(){return m_pBackParent;}
	CaNodeServer& GetNodeServer(){return m_object;}
	void GetNodeServer(CaNodeServer*& pObject){pObject = new CaNodeServer(m_object);}
protected:
	CaNodeServer m_object;
	CmtFolderNodeServer* m_pBackParent;

};

//
// Object: Folder Node Server
// **************************
class CmtFolderNodeServer: public CmtItemData, public CmtProtect
{
public:
	CmtFolderNodeServer():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderNodeServer();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtNodeServer* SearchObject (CaLLQueryInfo* pInfo, CaNodeServer* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

protected:
	CTypedPtrList< CObList, CmtNodeServer* > m_listObject;
	CmtNode* m_pBackParent;

};


//
// Object: Node 
// ************
class CmtNode: public CmtItemData, CmtProtect
{
public:
	CmtNode():m_pBackParent(NULL), CmtItemData(), CmtProtect(){Initialize();}
	CmtNode(CaNode* pObj):m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		Initialize();
		ASSERT (pObj);
		if (pObj)
			m_object = *pObj;
	}

	virtual ~CmtNode(){};
	virtual CmtSessionManager* GetSessionManager();
	void Initialize()
	{
		m_folderNodeServer.SetBackParent(this);
		m_folderNodeLogin.SetBackParent(this);
		m_folderNodeConnectData.SetBackParent(this);
		m_folderNodeAttribute.SetBackParent(this);

		m_folderUser.SetBackParent(this);
		m_folderProfile.SetBackParent(this);
		m_folderRole.SetBackParent(this);
		m_folderGroup.SetBackParent(this);
		m_folderLocation.SetBackParent(this);
		m_folderDatabase.SetBackParent(this);
	}

	CaNode& GetNode(){return m_object;}
	void GetNode(CaNode*& pObject){pObject = new CaNode(m_object);}
	void SetBackParent(CmtFolderNode* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderNode* GetBackParent(){return m_pBackParent;}

	CmtFolderNodeServer&      GetFolderNodeServer(){return m_folderNodeServer;}
	CmtFolderNodeLogin&       GetFolderNodeLogin(){return m_folderNodeLogin;}
	CmtFolderNodeConnectData& GetFolderNodeConnectData() {return m_folderNodeConnectData;}
	CmtFolderNodeAttribute&   GetFolderNodeAttribute(){return m_folderNodeAttribute;}

	CmtFolderUser&            GetFolderUser(){return m_folderUser;}
	CmtFolderProfile&         GetFolderProfile(){return m_folderProfile;}
	CmtFolderRole&            GetFolderRole(){return m_folderRole;}
	CmtFolderGroup&           GetFolderGroup(){return m_folderGroup;}
	CmtFolderLocation&        GetFolderLocation(){return m_folderLocation;}
	CmtFolderDatabase&        GetFolderDatabase(){return m_folderDatabase;}

protected:
	CaNode m_object;

	CmtFolderNodeServer      m_folderNodeServer;
	CmtFolderNodeLogin       m_folderNodeLogin;
	CmtFolderNodeConnectData m_folderNodeConnectData;
	CmtFolderNodeAttribute   m_folderNodeAttribute;

	CmtFolderUser            m_folderUser;
	CmtFolderProfile         m_folderProfile;
	CmtFolderRole            m_folderRole;
	CmtFolderGroup           m_folderGroup;
	CmtFolderLocation        m_folderLocation;
	CmtFolderDatabase        m_folderDatabase;

	CmtFolderNode* m_pBackParent;
};


//
// Object: Node 
// ----------------
class CmtFolderNode: public CmtItemData, CmtProtect
{
public:
	CmtFolderNode():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderNode();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtAccount* pBackParent){m_pBackParent = pBackParent;}
	CmtAccount* GetBackParent(){return m_pBackParent;}
	CmtNode* SearchObject (CaLLQueryInfo* pInfo, CaNode* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject (CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

protected:
	CTypedPtrList< CObList, CmtNode* > m_listObject;
	CmtAccount* m_pBackParent;

};



#endif // DMLVNODE_HEADER
