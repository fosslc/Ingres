/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ieatree.h: interface for the Ingres Objects and Folders 
**    Project  : IMPORT & EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Hierarchy of Ingres Objects.
**
** History:
**
** 12-Dec-2000 (uk$so01)
**    Created
** 16-Oct-2001 (uk$so01)
**    Export Assistant
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**/


#if !defined (IMPORTxEXPORTxASSISTANTxTREE_HEADER)
#define IMPORTxEXPORTxASSISTANTxTREE_HEADER
#include "imageidx.h" // 0-based index of ingres image list
#include "qryinfo.h"  // CaRefreshInfo
#include "trfvnode.h" // CaNode, CaNodeXxx

#include "qryinfo.h" // CaLLQueryInfo for CALLBACK functions
#include "comsessi.h"

class CaIeaTree;
class CaIeaNode;


//
// Object: Table 
// **************
class CaIeaTable: public CtrfTable
{
	DECLARE_SERIAL(CaIeaTable);
public:
	CaIeaTable():CtrfTable(){}
	CaIeaTable(CaTable* pObj):CtrfTable(pObj){}
	virtual ~CaIeaTable(){};

	BOOL Initialize(){return TRUE;}
};



//
// Object: Folder of Tables
// *****************************
class CaIeaFolderTable: public CtrfFolderTable
{
	DECLARE_SERIAL(CaIeaFolderTable);
public:
	CaIeaFolderTable():CtrfFolderTable(){}
	virtual ~CaIeaFolderTable(){}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
};


//
// Object: Database 
// ****************
class CaIeaDatabase: public CtrfDatabase
{
	DECLARE_SERIAL(CaIeaDatabase);
public:
	CaIeaDatabase():CtrfDatabase(), m_EmptyNode(_T("<No Tables>")), m_b4Import(TRUE){}
	CaIeaDatabase(CaDatabase* pObj):CtrfDatabase(pObj), m_EmptyNode(_T("<No Tables>")), m_b4Import(TRUE){}
	void ImportMode(BOOL bSet){m_b4Import = bSet;}
	void Initialize();
	virtual ~CaIeaDatabase(){};
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
protected:
	BOOL m_b4Import;
	CtrfItemData m_EmptyNode;
};

//
// Object: Folder of Database
// *****************************
class CaIeaFolderDatabase: public CtrfFolderDatabase
{
	DECLARE_SERIAL(CaIeaFolderDatabase);
public:
	CaIeaFolderDatabase():CtrfFolderDatabase(),m_b4Import(TRUE){}
	virtual ~CaIeaFolderDatabase(){}
	void ImportMode(BOOL bSet){m_b4Import = bSet;}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
protected:
	BOOL m_b4Import;
};


//
// Object: Node Server
// *******************
class CaIeaNodeServer: public CtrfNodeServer
{
	DECLARE_SERIAL(CaIeaNodeServer);
public:
	CaIeaNodeServer():CtrfNodeServer(), m_EmptyNode(_T("<No Databases>")), m_b4Import(TRUE){}
	CaIeaNodeServer(CaNodeServer* pObj):CtrfNodeServer(pObj), m_EmptyNode(_T("<No Databases>")), m_b4Import(TRUE){}
	virtual ~CaIeaNodeServer(){}
	virtual void Initialize();
	void ImportMode(BOOL bSet){m_b4Import = bSet;}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
protected:
	CtrfItemData m_EmptyNode;
	BOOL m_b4Import;
};



//
// Object: Folder of Node Server
// *****************************
class CaIeaFolderNodeServer: public CtrfFolderNodeServer
{
	DECLARE_SERIAL(CaIeaFolderNodeServer);
public:
	CaIeaFolderNodeServer():CtrfFolderNodeServer(), m_b4Import(TRUE){}
	virtual ~CaIeaFolderNodeServer(){}
	void ImportMode(BOOL bSet){m_b4Import = bSet;}

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
protected:
	BOOL m_b4Import;
};


//
// Object: Node 
// ************
class CaIeaNode: public CtrfNode
{
	DECLARE_SERIAL(CaIeaNode);
public:
	CaIeaNode():CtrfNode(), m_b4Import(TRUE){}
	CaIeaNode(CaNode* pObj): CtrfNode(pObj), m_b4Import(TRUE){}
	virtual ~CaIeaNode(){};

	void Initialize();
	void ImportMode(BOOL bSet){m_b4Import = bSet;}
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
protected:
	BOOL m_b4Import;
};


//
// Object: Iea Tree (Import & Export Assistant Tree)
// *************************************************
class CaIeaTree: public CtrfFolderNode
{
	DECLARE_SERIAL(CaIeaTree);
public:
	CaIeaTree();
	virtual ~CaIeaTree();

	//
	// You must called initialize() befor starting to use cache !
	virtual BOOL Initialize();
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);

};


class CaIexportTree: public CtrfFolderNode
{
	DECLARE_SERIAL(CaIexportTree);
public:
	CaIexportTree();
	virtual ~CaIexportTree();

	//
	// You must called initialize() befor starting to use cache !
	virtual BOOL Initialize();
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);

};




#endif // IMPORTxEXPORTxASSISTANTxTREE_HEADER
