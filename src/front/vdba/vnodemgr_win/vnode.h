/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : vnode.h, Header File
**
**  Project  : Virtual Node Manager.
**
**  Author   : UK Sotheavut
**
**  Purpose  : Implement the behavior of Individual Tree Node,
**             Node Local, Node Installation, ...
**
**  History:
** 24-jan-2000 (uk$so01)
**    Sir #100102
**    Add the "Performance Monitor" command in toolbar and menu
** 08-mar-2000 (somsa01)
**    Removed extra comma in enum declaration.
** 11-Sep-2000 (uk$so01)
**    Bug #102486, Fix the probem of Alter/Drop Connection data.
**    The problem is a side effect of that there might have multiple same remote
**    addresses but different protocol and listen address, and test is previously performed
**    only on the remote address.
** 22-may-2001 (zhayu01) SIR 104751 for EDBC
**    1. Add m_bSave, IsSave(), SetSave() and Enter() to CaIngnetNodeLogin.
**    2. Declare VNODE_EnterLogin().
** 28-Nov-2000 (schph01)
**    (SIR 102945) Grayed menu "Add Installation Password..." when the selection
**    on tree is on another branch that "Nodes" and "(Local)".
** 01-nov-2001 (somsa01)
**    Cleaned up 64-bit compiler warnings / errors.
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
*/

#if !defined(AFX_VNODE_H__2D0C26D7_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
#define AFX_VNODE_H__2D0C26D7_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "nodeinfo.h"


#define SQL_EXE 1
#define DOM_EXE 2
#define IPM_EXE 3


BOOL VNODE_Add();
BOOL VNODE_Alter(LPCTSTR lpszNode);
BOOL VNODE_Drop (LPCTSTR lpszNode);

BOOL VNODE_AddInstallation();
BOOL VNODE_AlterInstallation(LPCTSTR lpszNode, LPCTSTR lpszLogin);
BOOL VNODE_DropInstallation(LPCTSTR lpszNode);

BOOL VNODE_AddLogin  (LPCTSTR lpszNode);
BOOL VNODE_AlterLogin(LPCTSTR lpszNode, LPCTSTR lpszLogin);
BOOL VNODE_DropLogin (LPCTSTR lpszNode, LPCTSTR lpszLogin);
#ifdef	EDBC
BOOL VNODE_EnterLogin  (LPCTSTR lpszNode);
#endif

BOOL VNODE_AddConnection  (LPCTSTR lpszNode);

BOOL VNODE_AddAttribute  (LPCTSTR lpszNode);
BOOL VNODE_AlterAttribute(LPCTSTR lpszNode, LPCTSTR lpszAttribute);
BOOL VNODE_DropAttribute (LPCTSTR lpszNode, LPCTSTR lpszAttribute);
//
// Test existence of exe:
// nExe can be:
//  SQL_EXE
//  DOM_EXE
BOOL VNODE_IsExist (int nExe);


enum
{
	IM_FOLDER_CLOSE = 0,
	IM_FOLDER_OPEN,

	IM_NODE_LOCAL,
	IM_NODE_NORMAL,
	IM_NODE_COMPLEX,
	IM_NODE_INSTALLATION,

	IM_SERVER,

	IM_LOGIN_PUBLIC,
	IM_LOGIN_PRIVATE,

	IM_CONNECTDATA_PUBLIC,
	IM_CONNECTDATA_PRIVATE,

	IM_ATTRIBUTE_PUBLIC,
	IM_ATTRIBUTE_PRIVATE,

	IM_EMPTY_SERVER,
	IM_EMPTY_NODE
};

enum
{
	NODE_ODER_LOCAL = 0,
	NODE_ODER_INSTALLATION,
	NODE_ODER_NODENORMAL,
	NODE_ODER_OTHER
};

typedef enum tagNODESTATE
{
	NODE_DELETE = 0,
	NODE_EXIST,
	NODE_NEW
} NODESTATE;

typedef enum tagNODETYPE
{
	NODExLOCAL = 0,
	NODExINSTALLATION, 
	NODExNORMAL,
	NODExSERVER,
	NODExFOLDER,
	NODExCHARACTERISTICS
} NODETYPE;

class CaVNodeTreeItemData: public CObject
{
public:
	CaVNodeTreeItemData():
		m_nState(NODE_NEW), 
		m_nNodeType (NODExFOLDER), 
		m_strName(_T("")), 
		m_nOrder(NODE_ODER_OTHER){}

	virtual ~CaVNodeTreeItemData(){}
	virtual BOOL IsAddEnabled(){return FALSE;}
	virtual BOOL IsAlterEnabled(){return FALSE;}
	virtual BOOL IsDropEnabled(){return FALSE;}
	virtual BOOL IsSQLEnabled(){return FALSE;}
	virtual BOOL IsDOMEnabled(){return FALSE;}
	virtual BOOL IsIPMEnabled(){return FALSE;}
	virtual BOOL IsTestNodeEnabled(){return FALSE;}
	virtual BOOL IsAddInstallationEnabled();
	virtual BOOL QueryData(CTreeCtrl* pTree){return FALSE;}

	virtual BOOL Add()  {ASSERT (FALSE); return FALSE;}
	virtual BOOL Alter(){ASSERT (FALSE); return FALSE;}
	virtual BOOL Drop() {ASSERT (FALSE); return FALSE;}
	virtual BOOL AddInstallation();

	virtual BOOL OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand)
	{
		m_nodeInfo.SetExpanded(bExpand);
		return TRUE;
	}

	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent){ASSERT (FALSE);}
	virtual void SetName (LPCTSTR lpszName){m_strName = lpszName;}

	int  GetOrder(){return m_nOrder;}
	void SetOrder(int nOrder){m_nOrder = nOrder;}
	void SetInternalState(NODESTATE state){m_nState = state;}
	NODESTATE GetInternalState(){return m_nState;}

	CString GetName () {return m_strName;}
	CaTreeNodeInformation& GetNodeInformation(){return m_nodeInfo;}
	NODETYPE  GetNodeType() {return m_nNodeType;}
	void SetNodeType (NODETYPE nType){m_nNodeType = nType;}


protected:
	NODESTATE m_nState;
	NODETYPE  m_nNodeType;
	CString m_strName;
	CaTreeNodeInformation m_nodeInfo;
	int m_nOrder;

};

class CaNodeInstallation : public CaVNodeTreeItemData
{
public:
	CaNodeInstallation();
	virtual ~CaNodeInstallation();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL IsAddEnabled(){return TRUE;}
	virtual BOOL IsAlterEnabled(){return TRUE;}
	virtual BOOL IsDropEnabled(){return TRUE;}

	virtual BOOL Add();
	virtual BOOL Alter();
	virtual BOOL Drop();
	
	void SetDisplayString (LPCTSTR lpszDisplay){m_strDisplay = lpszDisplay;}
	void SetLogin(LPCTSTR lpszLogin){m_strLogin = lpszLogin;}
	CString GetLogin(){return m_strLogin;}

protected:
	CString m_strDisplay;
	CString m_strLogin;

};


class CaIngnetNodeServer : public CaVNodeTreeItemData
{
public:
	CaIngnetNodeServer();
	virtual ~CaIngnetNodeServer();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL IsSQLEnabled(){return VNODE_IsExist(SQL_EXE);}
	virtual BOOL IsDOMEnabled();
	virtual BOOL IsIPMEnabled();
	void SetLocal(BOOL bLocal){m_bLocal = bLocal;};
	BOOL IsLocal(){return m_bLocal;}
	void SetNodeName(LPCTSTR lpszNode){m_strNodeName = lpszNode;}
	CString GetNodeName(){return m_strNodeName;}

protected:
	BOOL m_bLocal;
	CString m_strNodeName;

};


class CaNodeServerFolder : public CaVNodeTreeItemData
{
public:
	CaNodeServerFolder();
	virtual ~CaNodeServerFolder();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand);
	BOOL InitData (LPCTSTR lpszNode);
	void SetNodeName(LPCTSTR lpszNode){m_strNodeName = lpszNode;}
	void SetLocal(BOOL bLocal){m_bLocal = bLocal;};
	BOOL IsLocal(){return m_bLocal;}
protected:
	CTypedPtrList<CObList, CaVNodeTreeItemData*> m_listServer;
	CString m_strNodeName;
	CaIngnetNodeServer m_DummyItem;
	BOOL m_bLocal;
};

//
// Login Folder:
class CaNodeLoginFolder : public CaVNodeTreeItemData
{
public:
	CaNodeLoginFolder();
	virtual ~CaNodeLoginFolder();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand);

	virtual BOOL IsAddEnabled(){return TRUE;}
	virtual BOOL Add();

	BOOL InitData (LPCTSTR lpszNode);
	INT_PTR GetCount() {return m_listLogin.GetCount();}
	CTypedPtrList<CObList, CaVNodeTreeItemData*>& GetListLogin(){return m_listLogin;}

protected:
	CString m_strNodeName;
	//
	// List of logins:
	CTypedPtrList<CObList, CaVNodeTreeItemData*> m_listLogin;

};

class CaIngnetNodeLogin : public CaVNodeTreeItemData
{
public:
	CaIngnetNodeLogin();
	virtual ~CaIngnetNodeLogin();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);

	virtual BOOL IsAddEnabled(){return TRUE;}
	virtual BOOL IsAlterEnabled(){return TRUE;}
	virtual BOOL IsDropEnabled(){return TRUE;}

	virtual BOOL Add();
	virtual BOOL Alter();
	virtual BOOL Drop();
#ifdef	EDBC
	virtual BOOL Enter();
#endif

	void SetNodeName (LPCTSTR lpszNode){m_strNodeName = lpszNode;}
	void SetPrivate(BOOL bPrivate){m_bPrivate = bPrivate;}
	BOOL IsPrivate(){return m_bPrivate;}
#ifdef	EDBC
	BOOL IsSave(){return m_bSave;}
	void SetSave(BOOL bSave){m_bSave = bSave;}
#endif

protected:
	BOOL m_bPrivate;
	CString m_strNodeName;
#ifdef	EDBC
	BOOL m_bSave;
#endif

};


//
// Connection Data Folder:
class CaNodeConnectionDataFolder : public CaVNodeTreeItemData
{
public:
	CaNodeConnectionDataFolder();
	virtual ~CaNodeConnectionDataFolder();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand);
	virtual BOOL IsAddEnabled(){return TRUE;}
	virtual BOOL Add();

	BOOL InitData (LPCTSTR lpszNode);
	CTypedPtrList<CObList, CaVNodeTreeItemData*>& GetListConnection() {return m_listConnectionData;}
protected:
	CString m_strNodeName;
	//
	// List of Connection Data:
	CTypedPtrList<CObList, CaVNodeTreeItemData*> m_listConnectionData;

};

class CaIngnetNodeConnectData : public CaVNodeTreeItemData
{
public:
	CaIngnetNodeConnectData();
	virtual ~CaIngnetNodeConnectData();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL IsAddEnabled(){return TRUE;}
	virtual BOOL IsAlterEnabled(){return TRUE;}
	virtual BOOL IsDropEnabled(){return TRUE;}

	virtual BOOL Add();
	virtual BOOL Alter();
	virtual BOOL Drop();

	void SetNodeName (LPCTSTR lpszNode){m_strNodeName = lpszNode;}
	void SetPrivate(BOOL bPrivate){m_bPrivate = bPrivate;}
	BOOL IsPrivate(){return m_bPrivate;}
	CString GetProtocol(){return m_strProtocol;}
	CString GetListenAddress(){return m_strListenAddress;}
	void SetProtocol(LPCTSTR lpszProtocol){m_strProtocol = lpszProtocol;}
	void SetListenAddress(LPCTSTR lpszListenAddress){m_strListenAddress = lpszListenAddress;}

protected:
	BOOL m_bPrivate;
	CString m_strNodeName;
	CString m_strProtocol;
	CString m_strListenAddress;

};


//
// Attribute Folder:
class CaNodeAttributeFolder : public CaVNodeTreeItemData
{
public:
	CaNodeAttributeFolder();
	virtual ~CaNodeAttributeFolder();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand);
	virtual BOOL IsAddEnabled(){return TRUE;}
	virtual BOOL Add();
	
	BOOL InitData (LPCTSTR lpszNode);
	CTypedPtrList<CObList, CaVNodeTreeItemData*>& GetListAttribute() {return m_listAttribute;}
protected:
	CString m_strNodeName;
	//
	// List of Attribute:
	CTypedPtrList<CObList, CaVNodeTreeItemData*> m_listAttribute;

};

class CaIngnetNodeAttribute : public CaVNodeTreeItemData
{
public:
	CaIngnetNodeAttribute();
	virtual ~CaIngnetNodeAttribute();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL IsAddEnabled(){return TRUE;}
	virtual BOOL IsAlterEnabled(){return TRUE;}
	virtual BOOL IsDropEnabled(){return TRUE;}

	virtual BOOL Add();
	virtual BOOL Alter();
	virtual BOOL Drop();

	void SetNodeName (LPCTSTR lpszNode){m_strNodeName = lpszNode;}
	void SetPrivate(BOOL bPrivate){m_bPrivate = bPrivate;}
	BOOL IsPrivate(){return m_bPrivate;}
	CString GetAttributeValue(){return m_strAttributeValue;}
	void SetAttributeValue(LPCTSTR lpszValue){m_strAttributeValue = lpszValue;}

protected:
	BOOL m_bPrivate;
	CString m_strNodeName;
	CString m_strAttributeValue;

};



class CaNodeAdvancedFolder : public CaVNodeTreeItemData
{
public:
	CaNodeAdvancedFolder();
	virtual ~CaNodeAdvancedFolder();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand);

	BOOL InitData (LPCTSTR lpszNode);
	CaNodeLoginFolder& GetLoginFolder(){return m_folderLogin;}
	CaNodeConnectionDataFolder& GetConnectionFolder(){return m_folderConnectionData;}
	CaNodeAttributeFolder& GetAttributeFolder(){return m_folderAttribute;}
protected:
	CString m_strNodeName;
	//
	// Login Folder:
	CaNodeLoginFolder m_folderLogin;
	//
	// Connection Data Folder:
	CaNodeConnectionDataFolder m_folderConnectionData;
	//
	// Attribute Folder:
	CaNodeAttributeFolder m_folderAttribute;

};

class CaNodeLocal : public CaVNodeTreeItemData
{
public:
	CaNodeLocal();
	virtual ~CaNodeLocal();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL IsAddInstallationEnabled();
	virtual BOOL IsAddEnabled(){return TRUE;}
	virtual BOOL IsSQLEnabled(){return VNODE_IsExist(SQL_EXE);}
	virtual BOOL IsDOMEnabled(){return VNODE_IsExist(DOM_EXE);}
	virtual BOOL IsIPMEnabled(){return VNODE_IsExist(IPM_EXE);}
	virtual void SetName (LPCTSTR lpszName)
	{
		m_strName = lpszName;
		m_nodeServerFolder.SetNodeName(lpszName);
	}

	virtual BOOL Add();

	BOOL InitData (LPCTSTR lpszNode){return m_nodeServerFolder.InitData (lpszNode);}

	//
	// Server/Gateway folder:
	CaNodeServerFolder m_nodeServerFolder;
protected:

};


class CaIngnetNode : public CaNodeLocal
{
public:
	CaIngnetNode();
	virtual ~CaIngnetNode();
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent);
	virtual BOOL IsAddEnabled(){return TRUE;}
	virtual BOOL IsAlterEnabled(){return TRUE;}
	virtual BOOL IsDropEnabled(){return TRUE;}
	virtual BOOL IsSQLEnabled(){return VNODE_IsExist(SQL_EXE);}
	virtual BOOL IsDOMEnabled(){return VNODE_IsExist(DOM_EXE);}
	virtual BOOL IsIPMEnabled(){return VNODE_IsExist(IPM_EXE);}
	virtual BOOL IsTestNodeEnabled(){return TRUE;}

	virtual BOOL Add();
	virtual BOOL Alter();
	virtual BOOL Drop();
	virtual void SetName (LPCTSTR lpszName)
	{
		m_strName = lpszName;
		m_nodeServerFolder.SetNodeName(lpszName);
	}


	BOOL InitData (LPCTSTR lpszNode);
	BOOL IsSimplified(){return m_bSimplified;}
	void SetSimplified(BOOL bSimplifed){m_bSimplified = bSimplifed;}

	//
	// Advanced Node Parameter folder:
	CaNodeAdvancedFolder m_nodeAdvancedFolder;

protected:
	BOOL m_bSimplified;

};


class CaNodeDummy: public CaVNodeTreeItemData
{
public:
	CaNodeDummy():CaVNodeTreeItemData(){}

	virtual ~CaNodeDummy(){}
	virtual BOOL IsAddInstallationEnabled(){return TRUE;}
	virtual BOOL IsAddEnabled(){return TRUE;}

	virtual BOOL Add(){return VNODE_Add();};
	virtual BOOL AddInstallation(){return VNODE_AddInstallation();}
	virtual void Display (CTreeCtrl* pTree, HTREEITEM hParent){ASSERT (FALSE);}
};


#endif // !defined(AFX_VNODE_H__2D0C26D7_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
