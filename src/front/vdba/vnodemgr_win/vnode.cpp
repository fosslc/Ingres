/*
**  Copyright (C) 2005-2008 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : vnode.cpp, Implementation File 
** Project  : Virtual Node Manager.
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Implement the behavior of Individual Tree Node,
**            Node Local, Node Installation,
**
**  HISTORY:
**	xx-Mar-1999 (uk$so01)
**	    created.
**	24-jan-2000 (uk$so01)
**	    Sir #100102, Add the "Performance Monitor" command in toolbar
**	    and menu
**	11-Sep-2000 (uk$so01)
**	    Bug #102486, Fix the probem of Alter/Drop Connection data.
**	    The problem is a side effect of that there might have multiple
**	    same remote addresses but different protocol and listen
**	    address, and test is previously performed only on the remote
**	    address.
**	14-may-2001 (zhayu01) SIR 104724
**	    Vnode becomes Server for EDBC
**	22-may-2001 (zhayu01) SIR 104751 for EDBC
**	    1. Handle the bLoginSave check box for EDBC.
**	    2. Set m_bSave to TRUE as default.
**	    3. Add VNODE_EnterLogin() for CaNodeLogin and include dglogipt.h.
**	28-Nov-2000 (schph01)
**	    (SIR 102945) Grayed menu "Add Installation Password..." when
**	    the selection on tree is on another branch that "Nodes" and
**	    "(Local)".
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings.
**	23-Jun-2003 (schph01)
**	   (bug 110439) in VNODE_AlterAttribute() function, fill the vnode name
**	   before launch the dialog box.
**      18-Sep-2003 (uk$so01)
**         SIR #106648, Integrate the libingll.lib and cleanup codes.
**	18-sep-2003 (musro02)
**	   For MAINWIN, revise to use directory bin rather than vdba
**      10-Oct-2003 (uk$so01)
**         SIR #106648, (Integrate 2.65 features for EDBC)
**      13-Oct-2003 (schph01)
**         SIR #109864, doesn't display CaNodeServerFolder class when -vnode
**         command is used.
**      14-Nov-2003 (uk$so01)
**         BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**         to query the server classes.
**      16-Oct-2006 (wridu01)
**         Bug #116838 Change the default protocol from wintcp to tcp_ip for IPV6
**                     project.
**      23-Sep-2008 (lunbr01) Sir 119985
**         Remove #ifdef MAINWIN around default network protocol since it is
**         no longer needed after Durwin's change for bug 116838.
*/

#include "stdafx.h"
#include "vnodemgr.h"
#include "vnode.h"
#include "treenode.h"
#include "dgvnode.h"
#include "xdgvnpwd.h"
#include "xdgvnatr.h"
#include "dgvnlogi.h"
#include "dgvndata.h"
#include "tlsfunct.h"
#ifdef	EDBC
#include "dglogipt.h"
#endif
#include <io.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



//////////////////////////////////////////////////////////////////////
// CaVNodeTreeItemData 
//////////////////////////////////////////////////////////////////////
BOOL CaVNodeTreeItemData::IsAddInstallationEnabled()
{
	if ( GetNodeType() == NODExFOLDER && GetName().IsEmpty() &&
	    !HasInstallationPassword(FALSE))
		return TRUE;
	else
		return FALSE;
}
BOOL CaVNodeTreeItemData::AddInstallation()
{
	return VNODE_AddInstallation();
}


//////////////////////////////////////////////////////////////////////
// CaNodeInstallation 
//////////////////////////////////////////////////////////////////////

CaNodeInstallation::CaNodeInstallation():CaVNodeTreeItemData()
{
	if (!m_strDisplay.LoadString(IDS_NODE_INSTALLATION))
		m_strDisplay = _T("<installation password node>");
	m_nodeInfo.SetImage (IM_NODE_INSTALLATION);
	SetNodeType(NODExINSTALLATION);
	SetOrder (NODE_ODER_INSTALLATION);
	m_strLogin= _T("");
}

CaNodeInstallation::~CaNodeInstallation()
{
}

void CaNodeInstallation::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strDisplay, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}
}

BOOL CaNodeInstallation::Add()  {return VNODE_Add();}
BOOL CaNodeInstallation::Alter(){return VNODE_AlterInstallation(m_strName, m_strLogin);}
BOOL CaNodeInstallation::Drop() {return VNODE_DropInstallation (m_strName);}

//////////////////////////////////////////////////////////////////////
// CaNodeLocal
//////////////////////////////////////////////////////////////////////
CaNodeLocal::CaNodeLocal()
	:CaVNodeTreeItemData()
{
	m_nodeInfo.SetImage (IM_NODE_LOCAL);
	SetNodeType(NODExLOCAL);
	SetOrder (NODE_ODER_LOCAL);
}

CaNodeLocal::~CaNodeLocal()
{

}

void CaNodeLocal::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);

		m_nodeServerFolder.Display (pTree, hTreeItem);
	}
}

BOOL CaNodeLocal::Add()  {return VNODE_Add();}

BOOL CaNodeLocal::IsAddInstallationEnabled()
{
	if (m_nodeServerFolder.IsLocal() && !HasInstallationPassword(FALSE))
		return TRUE;
	else
		return FALSE;
}

//////////////////////////////////////////////////////////////////////
// CaIngnetNode
//////////////////////////////////////////////////////////////////////
CaIngnetNode::CaIngnetNode()
	:CaNodeLocal()
{
	SetNodeType(NODExNORMAL);
	m_nodeInfo.SetImage (IM_NODE_NORMAL);
	SetOrder (NODE_ODER_NODENORMAL);
}

CaIngnetNode::~CaIngnetNode()
{

}

void CaIngnetNode::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	m_nodeInfo.SetImage (m_bSimplified? IM_NODE_NORMAL: IM_NODE_COMPLEX);
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}
	else
	{
		pTree->SetItemImage (m_nodeInfo.GetTreeItem (), nImage, nImage);
	}

	//
	// Create the Server and Advanced Folder:
	m_nodeServerFolder.Display (pTree, hTreeItem);
	m_nodeAdvancedFolder.Display (pTree, hTreeItem);
}

BOOL CaIngnetNode::InitData (LPCTSTR lpszNode)
{
	BOOL b[2];
	b[0] = m_nodeAdvancedFolder.InitData (lpszNode);
	b[1] = m_nodeServerFolder.InitData (lpszNode);
	return (b[0] && b[1]);
}

BOOL CaIngnetNode::Add()  {return VNODE_Add();}
BOOL CaIngnetNode::Alter(){return VNODE_Alter(m_strName);}
BOOL CaIngnetNode::Drop() {return VNODE_Drop(m_strName);}


//////////////////////////////////////////////////////////////////////
// CaNodeServerFolder
//////////////////////////////////////////////////////////////////////
CaNodeServerFolder::CaNodeServerFolder():CaVNodeTreeItemData()
{
	m_strName = theApp.m_strFolderServer;
	m_nodeInfo.SetImage (IM_FOLDER_CLOSE);
	SetNodeType(NODExFOLDER);
	m_strNodeName = _T("");
	m_DummyItem.SetName (theApp.m_strUnavailable);
	m_DummyItem.GetNodeInformation().SetImage (IM_EMPTY_SERVER);
	m_bLocal = FALSE;
}

CaNodeServerFolder::~CaNodeServerFolder()
{
	while (!m_listServer.IsEmpty())
		delete m_listServer.RemoveTail();
}

void CaNodeServerFolder::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	if (theApp.m_pNodeAccess->IsVnodeOptionDefined())
		return;
	//
	// Create the Folder: <Servers>
	// and add the dummy item <Data unavailable> so that the user can expand
	// the branch:
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}

	//
	// Display the list of Servers / Gateways in this Folder:
	hTreeItem = m_nodeInfo.GetTreeItem();
	CaVNodeTreeItemData* pServer = NULL;
	POSITION p, pos = m_listServer.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pServer = m_listServer.GetNext(pos);
		if (pServer->GetInternalState() != NODE_DELETE)
		{
			pServer->Display (pTree, hTreeItem);
			pServer->SetInternalState (NODE_EXIST);
		}
		else
		{
			CaTreeNodeInformation& nodeInfo = pServer->GetNodeInformation();
			HTREEITEM hTreeItem = nodeInfo.GetTreeItem();
			if (hTreeItem)
			{
				pTree->DeleteItem (hTreeItem);
				m_listServer.RemoveAt (p);
				delete pServer;
			}
		}
	}

	if (m_listServer.IsEmpty())
	{
		if (!m_DummyItem.GetNodeInformation().GetTreeItem())
		{
			HTREEITEM hTreeDummyItem = VNODE_TreeAddItem (
				m_DummyItem.GetName(), 
				pTree, 
				m_nodeInfo.GetTreeItem(),
				TVI_FIRST,
				m_DummyItem.GetNodeInformation().GetImage(), (DWORD_PTR)&m_DummyItem);
			m_DummyItem.GetNodeInformation().SetTreeItem(hTreeDummyItem);
		}
	}
	else
	{
		if (m_DummyItem.GetNodeInformation().GetTreeItem())
		{
			pTree->DeleteItem (m_DummyItem.GetNodeInformation().GetTreeItem());
			m_DummyItem.GetNodeInformation().SetTreeItem(NULL);
		}
	}
}


BOOL CaNodeServerFolder::InitData (LPCTSTR lpszNode)
{
	m_strNodeName = lpszNode;
	return TRUE;
	return VNODE_QueryServer (lpszNode, m_listServer, m_bLocal);
}

BOOL CaNodeServerFolder::OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand)
{
	m_nodeInfo.SetImage (bExpand? IM_FOLDER_OPEN: IM_FOLDER_CLOSE);
	pTree->SetItemImage (hItem, m_nodeInfo.GetImage(), m_nodeInfo.GetImage());
	m_nodeInfo.SetExpanded (bExpand);
	if (bExpand && !m_nodeInfo.IsQueried())
	{
		if (VNODE_QueryServer (m_strNodeName, m_listServer, m_bLocal))
			m_nodeInfo.SetQueried(TRUE);
		Display (pTree, m_nodeInfo.GetTreeItem());
	}

	return TRUE;
}


//////////////////////////////////////////////////////////////////////
// CaIngnetNodeServer
//////////////////////////////////////////////////////////////////////
CaIngnetNodeServer::CaIngnetNodeServer()
	:CaVNodeTreeItemData()
{
	m_nodeInfo.SetImage (IM_SERVER);
	SetNodeType(NODExSERVER);
	m_bLocal = FALSE;
	m_strNodeName = _T("");
}

CaIngnetNodeServer::~CaIngnetNodeServer()
{

}

BOOL CaIngnetNodeServer::IsIPMEnabled()
{
	BOOL bExist = VNODE_IsExist(IPM_EXE);
	if (!bExist)
		return FALSE;
	//
	// Only INGRES Server class is allowed:
	return (GetName().CompareNoCase(_T("INGRES")) == 0)? TRUE: FALSE;
}

void CaIngnetNodeServer::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}
}

BOOL CaIngnetNodeServer::IsDOMEnabled()
{
	BOOL bExist = VNODE_IsExist(DOM_EXE);
	if (!bExist)
		return FALSE;
	//
	// Not allowed is Ingres Server class is not "INGRES".
	BOOL bIngres = INGRESII_IsIngresGateway(GetName());
	if (!bIngres)
		return TRUE;
	return (GetName().CompareNoCase(_T("INGRES")) == 0)? TRUE: FALSE;
}

//////////////////////////////////////////////////////////////////////
// CaNodeAdvancedFolder
//////////////////////////////////////////////////////////////////////
CaNodeAdvancedFolder::CaNodeAdvancedFolder():CaVNodeTreeItemData()
{
	m_strName = theApp.m_strFolderAdvancedNode;
	m_nodeInfo.SetImage (IM_FOLDER_CLOSE);
	SetNodeType(NODExFOLDER);
}

CaNodeAdvancedFolder::~CaNodeAdvancedFolder()
{

}


void CaNodeAdvancedFolder::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	//
	// Create the Advanced Folder:
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}

	//
	// Create the sub-folders in this folder:
	hTreeItem = m_nodeInfo.GetTreeItem();
	m_folderLogin.Display(pTree, hTreeItem);
	m_folderConnectionData.Display(pTree, hTreeItem);
#if !defined (OPING20)
	m_folderAttribute.Display(pTree, hTreeItem);
#endif
}

BOOL CaNodeAdvancedFolder::InitData (LPCTSTR lpszNode)
{
	m_strNodeName = lpszNode;
	m_folderLogin.InitData (lpszNode);
	m_folderConnectionData.InitData (lpszNode);
#if !defined (OPING20)
	m_folderAttribute.InitData (lpszNode);
#endif
	return TRUE;
}

BOOL CaNodeAdvancedFolder::OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand)
{
	m_nodeInfo.SetImage (bExpand? IM_FOLDER_OPEN: IM_FOLDER_CLOSE);
	pTree->SetItemImage (hItem, m_nodeInfo.GetImage(), m_nodeInfo.GetImage());
	m_nodeInfo.SetExpanded(bExpand);
	return TRUE;
}


//////////////////////////////////////////////////////////////////////
// CaNodeLoginFolder
//////////////////////////////////////////////////////////////////////
CaNodeLoginFolder::CaNodeLoginFolder():CaVNodeTreeItemData()
{
	m_strName = theApp.m_strFolderLogin;
	m_nodeInfo.SetImage (IM_FOLDER_CLOSE);
	SetNodeType(NODExFOLDER);
}

CaNodeLoginFolder::~CaNodeLoginFolder()
{
	while (!m_listLogin.IsEmpty())
		delete m_listLogin.RemoveTail();
}

void CaNodeLoginFolder::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	//
	// Create the Login Folder:
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}
	
	//
	// Display the list of Logins in this Folder:
	hTreeItem = m_nodeInfo.GetTreeItem();
	CaVNodeTreeItemData* pLogin = NULL;
	POSITION p, pos = m_listLogin.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pLogin = m_listLogin.GetNext(pos);
		if (pLogin->GetInternalState() != NODE_DELETE)
		{
			pLogin->Display (pTree, hTreeItem);
			pLogin->SetInternalState (NODE_EXIST);
		}
		else
		{
			CaTreeNodeInformation& nodeInfo = pLogin->GetNodeInformation();
			HTREEITEM hTreeItem = nodeInfo.GetTreeItem();
			if (hTreeItem)
			{
				pTree->DeleteItem (hTreeItem);
				m_listLogin.RemoveAt (p);
				delete pLogin;
			}
		}
	}
}

BOOL CaNodeLoginFolder::InitData (LPCTSTR lpszNode)
{
	m_strNodeName = lpszNode;
	return VNODE_QueryLogin (lpszNode, m_listLogin);
}

BOOL CaNodeLoginFolder::OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand)
{
	m_nodeInfo.SetImage (bExpand? IM_FOLDER_OPEN: IM_FOLDER_CLOSE);
	pTree->SetItemImage (hItem, m_nodeInfo.GetImage(), m_nodeInfo.GetImage());
	m_nodeInfo.SetExpanded(bExpand);
	return TRUE;
}

BOOL CaNodeLoginFolder::Add()  {return VNODE_AddLogin(m_strNodeName);}

//////////////////////////////////////////////////////////////////////
// CaIngnetNodeLogin
//////////////////////////////////////////////////////////////////////
CaIngnetNodeLogin::CaIngnetNodeLogin():CaVNodeTreeItemData()
{
	m_bPrivate = FALSE;
	m_nodeInfo.SetImage (IM_LOGIN_PUBLIC);
	SetNodeType(NODExCHARACTERISTICS);
#ifdef	EDBC
	m_bSave = TRUE;
#endif
}
	
CaIngnetNodeLogin::~CaIngnetNodeLogin()
{

}

void CaIngnetNodeLogin::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	m_nodeInfo.SetImage (m_bPrivate? IM_LOGIN_PRIVATE: IM_LOGIN_PUBLIC);
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}
	else
	{
		pTree->SetItemImage (m_nodeInfo.GetTreeItem (), nImage, nImage);
	}
}

BOOL CaIngnetNodeLogin::Add()  {return VNODE_AddLogin(m_strNodeName);}
BOOL CaIngnetNodeLogin::Alter(){return VNODE_AlterLogin(m_strNodeName, m_strName);}
BOOL CaIngnetNodeLogin::Drop() {return VNODE_DropLogin(m_strNodeName, m_strName);}
#ifdef	EDBC
BOOL CaIngnetNodeLogin::Enter()  {return VNODE_EnterLogin(m_strNodeName);}
#endif

//////////////////////////////////////////////////////////////////////
// CaNodeConnectionDataFolder
//////////////////////////////////////////////////////////////////////
CaNodeConnectionDataFolder::CaNodeConnectionDataFolder():CaVNodeTreeItemData()
{
	m_strName = theApp.m_strFolderConnection;
	m_nodeInfo.SetImage (IM_FOLDER_CLOSE);
	SetNodeType(NODExFOLDER);
}

CaNodeConnectionDataFolder::~CaNodeConnectionDataFolder()
{
	while (!m_listConnectionData.IsEmpty())
		delete m_listConnectionData.RemoveTail();
}


void CaNodeConnectionDataFolder::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	//
	// Create the Connection Data Folder:
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}

	//
	// Display the list of Connection Data in this Folder:
	hTreeItem = m_nodeInfo.GetTreeItem();
	CaVNodeTreeItemData* pConnection = NULL;
	POSITION p, pos = m_listConnectionData.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pConnection = m_listConnectionData.GetNext(pos);
		if (pConnection->GetInternalState() != NODE_DELETE)
		{
			pConnection->Display (pTree, hTreeItem);
			pConnection->SetInternalState (NODE_EXIST);
		}
		else
		{
			CaTreeNodeInformation& nodeInfo = pConnection->GetNodeInformation();
			HTREEITEM hTreeItem = nodeInfo.GetTreeItem();
			if (hTreeItem)
			{
				pTree->DeleteItem (hTreeItem);
				m_listConnectionData.RemoveAt (p);
				delete pConnection;
			}
		}
	}
}

BOOL CaNodeConnectionDataFolder::InitData (LPCTSTR lpszNode)
{
	m_strNodeName = lpszNode;
	return VNODE_QueryConnection (lpszNode, m_listConnectionData);
}

BOOL CaNodeConnectionDataFolder::OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand)
{
	m_nodeInfo.SetImage (bExpand? IM_FOLDER_OPEN: IM_FOLDER_CLOSE);
	pTree->SetItemImage (hItem, m_nodeInfo.GetImage(), m_nodeInfo.GetImage());
	m_nodeInfo.SetExpanded(bExpand);
	return TRUE;
}

BOOL CaNodeConnectionDataFolder::Add()  {return VNODE_AddConnection(m_strNodeName);}

//////////////////////////////////////////////////////////////////////
// CaIngnetNodeConnectData
//////////////////////////////////////////////////////////////////////
CaIngnetNodeConnectData::CaIngnetNodeConnectData():CaVNodeTreeItemData()
{
	m_bPrivate = FALSE;
	m_nodeInfo.SetImage (IM_CONNECTDATA_PUBLIC);
	SetNodeType(NODExCHARACTERISTICS);
	m_strProtocol = _T("");
	m_strListenAddress = _T("");
}

CaIngnetNodeConnectData::~CaIngnetNodeConnectData()
{

}


void CaIngnetNodeConnectData::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	m_nodeInfo.SetImage (m_bPrivate? IM_CONNECTDATA_PRIVATE: IM_CONNECTDATA_PUBLIC);
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}
	else
	{
		pTree->SetItemImage (m_nodeInfo.GetTreeItem (), nImage, nImage);
	}
}

BOOL CaIngnetNodeConnectData::Add()  {return VNODE_AddConnection(m_strNodeName);}
BOOL CaIngnetNodeConnectData::Alter()
{
	CxDlgVirtualNodeData dlg;
	dlg.SetAlter();

	dlg.m_bPrivate          = IsPrivate();
	dlg.m_strVNodeName      = m_strNodeName; 
	dlg.m_strRemoteAddress  = m_strName; 
	dlg.m_strProtocol       = m_strProtocol; 
	dlg.m_strListenAddress  = m_strListenAddress; 
	INT_PTR res = dlg.DoModal();
	return (res == IDOK)? TRUE: FALSE;
}

BOOL CaIngnetNodeConnectData::Drop() 
{
	try
	{
		CaNodeDataAccess nodeAccess;
		nodeAccess.InitNodeList();
		CaNodeConnectData connectData(m_strNodeName, m_strName, m_strProtocol, m_strListenAddress, IsPrivate());
		CString strMsg;
#ifdef EDBC
		AfxFormatString2 (strMsg, IDS_DROP_SERVER_DATA, (LPCTSTR)m_strName, (LPCTSTR)m_strNodeName);
#else
		AfxFormatString2 (strMsg, IDS_DROP_NODE_DATA, (LPCTSTR)m_strName, (LPCTSTR)m_strNodeName);
#endif
		if (AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) != IDYES)
			return FALSE;
		HRESULT hErr = VNODE_ConnectionDrop (&connectData);
		if (hErr == NOERROR)
			return TRUE;
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
		return FALSE;
	}
	catch (...)
	{

	}
#ifdef EDBC
	AfxFormatString1 (theApp.m_strMessage, IDS_DROP_SERVER_DATA_FAILED, (LPCTSTR)m_strNodeName);
#else
	AfxFormatString1 (theApp.m_strMessage, IDS_DROP_NODE_DATA_FAILED, (LPCTSTR)m_strNodeName);
#endif
	AfxMessageBox (theApp.m_strMessage, MB_OK|MB_ICONEXCLAMATION);
	return FALSE;
}


//////////////////////////////////////////////////////////////////////
// CaNodeAttributeFolder
//////////////////////////////////////////////////////////////////////
CaNodeAttributeFolder::CaNodeAttributeFolder():CaVNodeTreeItemData()
{
	m_strName = theApp.m_strFolderAttribute;
	m_nodeInfo.SetImage (IM_FOLDER_CLOSE);
	SetNodeType(NODExFOLDER);
}

CaNodeAttributeFolder::~CaNodeAttributeFolder()
{
	while (!m_listAttribute.IsEmpty())
		delete m_listAttribute.RemoveTail();
}


void CaNodeAttributeFolder::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	//
	// Create the Attribute Folder:
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}
	
	//
	// Display the list of Attributes in this Folder:
	hTreeItem = m_nodeInfo.GetTreeItem();
	CaVNodeTreeItemData* pAttribute = NULL;
	POSITION p, pos = m_listAttribute.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		pAttribute = m_listAttribute.GetNext(pos);
		if (pAttribute->GetInternalState() != NODE_DELETE)
		{
			pAttribute->Display (pTree, hTreeItem);
			pAttribute->SetInternalState (NODE_EXIST);
		}
		else
		{
			CaTreeNodeInformation& nodeInfo = pAttribute->GetNodeInformation();
			HTREEITEM hTreeItem = nodeInfo.GetTreeItem();
			if (hTreeItem)
			{
				pTree->DeleteItem (hTreeItem);
				m_listAttribute.RemoveAt (p);
				delete pAttribute;
			}
		}
	}
}


BOOL CaNodeAttributeFolder::InitData (LPCTSTR lpszNode)
{
	m_strNodeName = lpszNode;
	return VNODE_QueryAttribute (lpszNode, m_listAttribute);
}

BOOL CaNodeAttributeFolder::OnExpandBranch(CTreeCtrl* pTree, HTREEITEM hItem, BOOL bExpand)
{
	m_nodeInfo.SetImage (bExpand? IM_FOLDER_OPEN: IM_FOLDER_CLOSE);
	pTree->SetItemImage (hItem, m_nodeInfo.GetImage(), m_nodeInfo.GetImage());
	m_nodeInfo.SetExpanded(bExpand);
	return TRUE;
}

BOOL CaNodeAttributeFolder::Add()  {return VNODE_AddAttribute(m_strNodeName);}


//////////////////////////////////////////////////////////////////////
// CaIngnetNodeAttribute
//////////////////////////////////////////////////////////////////////
CaIngnetNodeAttribute::CaIngnetNodeAttribute():CaVNodeTreeItemData()
{
	m_bPrivate = FALSE;
	m_nodeInfo.SetImage (IM_ATTRIBUTE_PUBLIC);
	SetNodeType(NODExCHARACTERISTICS);
	m_strAttributeValue = _T("");
}

CaIngnetNodeAttribute::~CaIngnetNodeAttribute()
{

}


void CaIngnetNodeAttribute::Display (CTreeCtrl* pTree, HTREEITEM hParent)
{
	m_nodeInfo.SetImage (m_bPrivate? IM_ATTRIBUTE_PRIVATE: IM_ATTRIBUTE_PUBLIC);
	int nImage = m_nodeInfo.GetImage();
	HTREEITEM hTreeItem = m_nodeInfo.GetTreeItem();
	if (!hTreeItem)
	{
		hTreeItem = VNODE_TreeAddItem (
			m_strName, 
			pTree, 
			hParent,
			TVI_LAST,
			nImage, (DWORD_PTR)this);
		m_nodeInfo.SetTreeItem (hTreeItem);
	}
	else
	{
		pTree->SetItemImage (m_nodeInfo.GetTreeItem (), nImage, nImage);
	}
}

BOOL CaIngnetNodeAttribute::Add()  {return VNODE_AddAttribute(m_strNodeName);}
BOOL CaIngnetNodeAttribute::Alter(){return VNODE_AlterAttribute(m_strNodeName, m_strName);}
BOOL CaIngnetNodeAttribute::Drop() {return VNODE_DropAttribute(m_strNodeName, m_strName);}



//
// Global Interface Dialog Functions:
BOOL VNODE_Add()
{
	TCHAR tchszProtocolDef[16];
	TCHAR tchszListenAddrDef[16];
	lstrcpy (tchszProtocolDef, _T("tcp_ip"));
#ifdef EDBC
	lstrcpy (tchszListenAddrDef, _T("134"));
#else
	lstrcpy (tchszListenAddrDef, _T("II"));
#endif

	CxDlgVirtualNode dlg;
	dlg.m_bLoginPrivate = TRUE;
#ifdef EDBC
	dlg.m_bLoginSave = FALSE;
#endif
	dlg.m_bConnectionPrivate = FALSE;
	dlg.m_strProtocol = tchszProtocolDef;
	dlg.m_strListenAddress = tchszListenAddrDef;

	INT_PTR res = dlg.DoModal();
	return	(res == IDOK)? TRUE: FALSE;
}

BOOL VNODE_Alter(LPCTSTR lpszNode)
{
	CTypedPtrList<CObList, CaDBObject*> ls;
	try
	{
		CxDlgVirtualNode dlg;
		CString strMsg;
		CString strNodeName = lpszNode;
		ASSERT (!strNodeName.IsEmpty());
		if (strNodeName.IsEmpty())
			return FALSE;
		CaNode node(lpszNode);
		HRESULT hErr = VNODE_QueryNode (ls);
		if (hErr != NOERROR)
			return FALSE;
		BOOL bFound = FALSE;
		POSITION pos = ls.GetHeadPosition();
		while (pos != NULL)
		{
			CaNode* pObj = (CaNode*)ls.GetNext(pos);
			if (pObj->GetName().CompareNoCase(lpszNode) == 0)
			{
				bFound = TRUE;
				node = *pObj;
				break;
			}
		}
		while (!ls.IsEmpty())
			delete ls.RemoveHead();

		dlg.SetAlter();
		if (!bFound)
		{
			//
			// _T("Node %s not found"), (LPCTSTR)strNodeName);
#ifdef EDBC
			AfxFormatString1 (strMsg, IDS_SERVER_NOTFOUND, (LPCTSTR)strNodeName);
#else
			AfxFormatString1 (strMsg, IDS_NODE_NOTFOUND, (LPCTSTR)strNodeName);
#endif
			AfxMessageBox (strMsg);
			return FALSE;
		}

		if ((node.GetClassifiedFlag() & NODE_SIMPLIFY) == 0)
		{
			//
			// _T("Node %s had advanced node parameters, and cannot be altered."), (LPCTSTR)strNodeName);
#ifdef EDBC
			AfxFormatString1 (strMsg, IDS_SERVER_HASADVANCED, (LPCTSTR)strNodeName);
#else
			AfxFormatString1 (strMsg, IDS_NODE_HASADVANCED, (LPCTSTR)strNodeName);
#endif
			AfxMessageBox (strMsg);
			return FALSE;
		}
		dlg.m_bConnectionPrivate= node.IsConnectionPrivate();
		dlg.m_bLoginPrivate     = node.IsLoginPrivate();
#ifdef EDBC
		dlg.m_bLoginSave        = node.GetSaveLogin();
#endif
		dlg.m_strVNodeName      = node.GetName();
		dlg.m_strUserName       = node.GetLogin();
		dlg.m_strRemoteAddress  = node.GetIPAddress();
		dlg.m_strProtocol       = node.GetProtocol();
		dlg.m_strListenAddress  = node.GetListenAddress();

		INT_PTR nResponse = dlg.DoModal();
		return (nResponse == IDOK)? TRUE: FALSE;
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch (...)
	{
	}
	return FALSE;
}

BOOL VNODE_Drop (LPCTSTR lpszNode)
{
	try
	{
		CString strMsg;
		CString strNodeName = lpszNode;
		ASSERT (!strNodeName.IsEmpty());
		if (strNodeName.IsEmpty())
			return FALSE;

		//
		// Ask for confirmation:
#ifdef EDBC
		AfxFormatString1 (strMsg, IDS_DROP_SERVER, strNodeName);
#else
		AfxFormatString1 (strMsg, IDS_DROP_NODE, strNodeName);
#endif
		if (AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) != IDYES)
			return FALSE;

		//
		// Drop Node:
		CaNodeDataAccess nodeAccess;
		nodeAccess.InitNodeList();
		CaNode node(lpszNode);
		HRESULT hErr = VNODE_DropNode(&node);
		if (hErr != NOERROR)
		{
			//
			// Cause: NodeLLInit() or LLDropFullNode() failed.
#ifdef EDBC
			AfxFormatString1 (strMsg, IDS_DROP_SERVER_FAILED, strNodeName);
#else
			AfxFormatString1 (strMsg, IDS_DROP_NODE_FAILED, strNodeName);
#endif
			AfxMessageBox (strMsg, MB_OK|MB_ICONEXCLAMATION);
			return FALSE;
		}
		return TRUE;
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch (...)
	{
#ifdef EDBC
		AfxFormatString1 (theApp.m_strMessage, IDS_DROP_SERVER_FAILED, lpszNode);
#else
		AfxFormatString1 (theApp.m_strMessage, IDS_DROP_NODE_FAILED, lpszNode);
#endif
		AfxMessageBox (theApp.m_strMessage, MB_OK|MB_ICONEXCLAMATION);
	}
	return FALSE;
}



BOOL VNODE_AddInstallation()
{
	if (HasInstallationPassword(FALSE)) 
	{
		CString strMsg;
		if (!strMsg.LoadString (IDS_NODE_INSTALLATION_EXIST))
			strMsg = _T("Installation password already defined on the node");
		AfxMessageBox(strMsg, MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	CxDlgVirtualNodeInstallationPassword dlg;
	INT_PTR res = dlg.DoModal();
	return (res == IDOK)? TRUE: FALSE;
}


BOOL VNODE_AlterInstallation(LPCTSTR lpszNode, LPCTSTR lpszLogin)
{
	CxDlgVirtualNodeInstallationPassword dlg;
	dlg.m_strNodeName = lpszNode;
	dlg.m_strUserName = lpszLogin;
	dlg.SetAlter();
	INT_PTR res = dlg.DoModal();
	return (res == IDOK)? TRUE: FALSE;
}

BOOL VNODE_DropInstallation(LPCTSTR lpszNode)
{
	try
	{
		CaNodeDataAccess nodeAccess;
		CString strMsg;
		strMsg.LoadString(IDS_DROP_INSTALLPASSWORD);
		if (AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) != IDYES)
			return FALSE;
		CaNode node(lpszNode);
	
		HRESULT hErr = VNODE_DropNode (&node);
		if (hErr == NOERROR)
			return TRUE;
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
	}
	catch (...)
	{
	}
#ifdef EDBC
	AfxFormatString1 (theApp.m_strMessage, IDS_DROP_SERVER_FAILED, lpszNode);
#else
	AfxFormatString1 (theApp.m_strMessage, IDS_DROP_NODE_FAILED, lpszNode);
#endif
	AfxMessageBox (theApp.m_strMessage, MB_OK|MB_ICONEXCLAMATION);
	return FALSE;
}

#ifdef EDBC
BOOL VNODE_EnterLogin(LPCTSTR lpszNode)
{
	INT_PTR  nLoginCount = 0;
	BOOL bLoginPrivate = TRUE;
	CdMainDoc* pDoc = theApp.m_pDocument;
	ASSERT (pDoc);
	if (!pDoc)
	{
		AfxMessageBox ("VNODE_EnterLogin::Internal error(1): document corrupted");
		return FALSE;
	}

	CaTreeNodeFolder& treeVNode = pDoc->GetDataVNode();
	CaVNodeTreeItemData* pNode = treeVNode.FindNode (lpszNode);
	ASSERT (pNode);
	if (!pNode)
	{
		AfxMessageBox ("VNODE_EnterLogin::Internal error(2): document corrupted");
		return FALSE;
	}
	CaIngnetNode* pNormalNode = (CaIngnetNode*)pNode;
	CaNodeLoginFolder& loginFolder = pNormalNode->m_nodeAdvancedFolder.GetLoginFolder();
	nLoginCount = loginFolder.GetCount();


	if (nLoginCount > 1)
	{
		CString strMsg;
		if (!strMsg.LoadString (IDS_ONLY_2LOGINS_PERNODE))
			strMsg = _T("You can only have two Logins per node.");
		AfxMessageBox (strMsg);
		return FALSE;
	}

	//
	// Check if the current node has one login 
	if (nLoginCount == 1)
	{
		CTypedPtrList<CObList, CaVNodeTreeItemData*>& listLogin = loginFolder.GetListLogin();
		POSITION pos = listLogin.GetHeadPosition();
		if (pos)
		{
			CString strMsg;
			CaIngnetNodeLogin* pLogin = (CaIngnetNodeLogin*)listLogin.GetNext(pos);
			bLoginPrivate = pLogin->IsPrivate();
		}
	}
	//
	// Launch the Dialog to prompt user to enter the login:
	CxDlgVirtualNodeEnterLogin dlg;
	dlg.m_strVNodeName = lpszNode;
	INT_PTR res = dlg.DoModal();
	return (res == IDOK)? TRUE: FALSE;
}
#endif

BOOL VNODE_AddLogin(LPCTSTR lpszNode)
{
	//
	// The can be only two Logins in a single node: if one is Private then
	// another one must be Global.
	int  ires;
	INT_PTR nLoginCount = 0;
	BOOL bLoginPrivate = TRUE;
	CdMainDoc* pDoc = theApp.m_pDocument;
	ASSERT (pDoc);
	if (!pDoc)
	{
		AfxMessageBox ("VNODE_AddLogin::Internal error(1): document corrupted");
		return FALSE;
	}

	CaTreeNodeFolder& treeVNode = pDoc->GetDataVNode();
	CaVNodeTreeItemData* pNode = treeVNode.FindNode (lpszNode);
	ASSERT (pNode);
	if (!pNode)
	{
		AfxMessageBox ("VNODE_AddLogin::Internal error(2): document corrupted");
		return FALSE;
	}
	CaIngnetNode* pNormalNode = (CaIngnetNode*)pNode;
	CaNodeLoginFolder& loginFolder = pNormalNode->m_nodeAdvancedFolder.GetLoginFolder();
	nLoginCount = loginFolder.GetCount();


	if (nLoginCount > 1)
	{
		CString strMsg;
		if (!strMsg.LoadString (IDS_ONLY_2LOGINS_PERNODE))
			strMsg = _T("You can only have two Logins per node.");
		AfxMessageBox (strMsg);
		return FALSE;
	}

	//
	// Check if the current node has one login 
	if (nLoginCount == 1)
	{
		CTypedPtrList<CObList, CaVNodeTreeItemData*>& listLogin = loginFolder.GetListLogin();
		POSITION pos = listLogin.GetHeadPosition();
		if (pos)
		{
			CString strMsg;
			CaIngnetNodeLogin* pLogin = (CaIngnetNodeLogin*)listLogin.GetNext(pos);
			if (pLogin->IsPrivate()) 
			{
				//
				// _T("You already have a private Login for node %s. This one will be a GLOBAL Login."), lpszNode)
				AfxFormatString1 (strMsg, IDS_ALREADY_HAS_PRIVATE, lpszNode);
				ires = AfxMessageBox (strMsg, MB_OKCANCEL);
			}
			else 
			{
				//
				// _T("You already have a global Login for node %s. This one will be a PRIVATE Login."), lpszNode)
				AfxFormatString1 (strMsg, IDS_ALREADY_HAS_GLOBAL, lpszNode);
				ires = AfxMessageBox (strMsg, MB_OKCANCEL);
			}
			if (ires == IDCANCEL)
				return FALSE;
			bLoginPrivate = pLogin->IsPrivate();
		}
	}
	//
	// Launch the Dialog to add the login:
	CxDlgVirtualNodeLogin dlg;
	if (nLoginCount == 1)
	{
		dlg.m_bEnableCheck = FALSE;  // Disable the private check box.
		dlg.m_bPrivate = !bLoginPrivate; 
	}
	else
		dlg.m_bPrivate = TRUE;
	dlg.m_strVNodeName = lpszNode;
	INT_PTR res = dlg.DoModal();
	return (res == IDOK)? TRUE: FALSE;
}

BOOL VNODE_AlterLogin(LPCTSTR lpszNode, LPCTSTR lpszLogin)
{
	INT_PTR  nLoginCount = 0;
	BOOL bLoginPrivate = TRUE;
	CdMainDoc* pDoc = theApp.m_pDocument;
	ASSERT (pDoc);
	if (!pDoc)
	{
		AfxMessageBox ("VNODE_AlterLogin::Internal error(1): document corrupted");
		return FALSE;
	}

	CaTreeNodeFolder& treeVNode = pDoc->GetDataVNode();
	CaVNodeTreeItemData* pNode = treeVNode.FindNode (lpszNode);
	ASSERT (pNode);
	if (!pNode)
	{
		AfxMessageBox ("VNODE_AlterLogin::Internal error(2): document corrupted");
		return FALSE;
	}
	CaIngnetNode* pNormalNode = (CaIngnetNode*)pNode;
	CaNodeLoginFolder& loginFolder = pNormalNode->m_nodeAdvancedFolder.GetLoginFolder();
	nLoginCount = loginFolder.GetCount();
	if (nLoginCount > 0)
	{
		CTypedPtrList<CObList, CaVNodeTreeItemData*>& listLogin = loginFolder.GetListLogin();
		POSITION pos = listLogin.GetHeadPosition();
		while (pos != NULL)
		{
			CaIngnetNodeLogin* pLogin = (CaIngnetNodeLogin*)listLogin.GetNext(pos);
			if (pLogin->GetName().CompareNoCase(lpszLogin) == 0)
			{
				bLoginPrivate = pLogin->IsPrivate();
				break;
			}
		}
	}
	else
	{
		AfxMessageBox ("VNODE_AlterLogin::Internal error(3): document corrupted");
		return FALSE;
	}

	CxDlgVirtualNodeLogin dlg;
	dlg.SetAlter();
	dlg.m_bEnableCheck = (nLoginCount > 1)? FALSE: TRUE;
	dlg.m_bPrivate     = bLoginPrivate;
	dlg.m_strVNodeName = lpszNode;
	dlg.m_strUserName  = lpszLogin;
	dlg.m_strPassword  = _T("");
	INT_PTR res = dlg.DoModal();
	return (res == IDOK)? TRUE: FALSE;
}

BOOL VNODE_DropLogin(LPCTSTR lpszNode, LPCTSTR lpszLogin)
{
	try
	{
		INT_PTR  nLoginCount = 0;
		BOOL bLoginPrivate = TRUE;
		CdMainDoc* pDoc = theApp.m_pDocument;
		ASSERT (pDoc);
		if (!pDoc)
		{
			AfxMessageBox ("VNODE_DropLogin::Internal error(1): document corrupted");
			return FALSE;
		}

		CaTreeNodeFolder& treeVNode = pDoc->GetDataVNode();
		CaVNodeTreeItemData* pNode = treeVNode.FindNode (lpszNode);
		ASSERT (pNode);
		if (!pNode)
		{
			AfxMessageBox ("VNODE_DropLogin::Internal error(2): document corrupted");
			return FALSE;
		}
		CaIngnetNode* pNormalNode = (CaIngnetNode*)pNode;
		CaNodeLoginFolder& loginFolder = pNormalNode->m_nodeAdvancedFolder.GetLoginFolder();
		nLoginCount = loginFolder.GetCount();
		if (nLoginCount > 0)
		{
			CTypedPtrList<CObList, CaVNodeTreeItemData*>& listLogin = loginFolder.GetListLogin();
			POSITION pos = listLogin.GetHeadPosition();
			while (pos != NULL)
			{
				CaIngnetNodeLogin* pLogin = (CaIngnetNodeLogin*)listLogin.GetNext(pos);
				if (pLogin->GetName().CompareNoCase(lpszLogin) == 0)
				{
					bLoginPrivate = pLogin->IsPrivate();
					break;
				}
			}
		}
		else
		{
			AfxMessageBox ("VNODE_DropLogin::Internal error(3): document corrupted");
			return FALSE;
		}

		CaNodeDataAccess nodeAccess;
		CaNodeLogin login(lpszNode, lpszLogin, _T(""), bLoginPrivate);
		HRESULT hErr = VNODE_LoginDrop (&login);
		if (hErr == NOERROR)
			return TRUE;
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
		return FALSE;
	}
	catch (...)
	{

	}
#ifdef EDBC
	AfxFormatString1 (theApp.m_strMessage, IDS_DROP_SERVER_LOGIN_FAILED, lpszNode);
#else
	AfxFormatString1 (theApp.m_strMessage, IDS_DROP_NODE_LOGIN_FAILED, lpszNode);
#endif
	AfxMessageBox (theApp.m_strMessage, MB_OK|MB_ICONEXCLAMATION);
	return FALSE;
}


BOOL VNODE_AddConnection  (LPCTSTR lpszNode)
{
	CxDlgVirtualNodeData dlg;
	dlg.m_bPrivate     = TRUE;
	dlg.m_strVNodeName = lpszNode;
	dlg.m_strProtocol  = _T("tcp_ip");
	INT_PTR res = dlg.DoModal();
	return (res == IDOK)? TRUE: FALSE;
}

BOOL VNODE_AddAttribute (LPCTSTR lpszNode)
{
	CxDlgVirtualNodeAttribute dlg;
	dlg.m_bPrivate     = TRUE;
	dlg.m_strVNodeName = lpszNode;
	INT_PTR res = dlg.DoModal();
	return (res == IDOK)? TRUE: FALSE;
}

BOOL VNODE_AlterAttribute(LPCTSTR lpszNode, LPCTSTR lpszAttribute)
{
	CdMainDoc* pDoc = theApp.m_pDocument;
	ASSERT (pDoc);
	if (!pDoc)
	{
		AfxMessageBox ("VNODE_AlterAttribute::Internal error(1): document corrupted");
		return FALSE;
	}

	CaTreeNodeFolder& treeVNode = pDoc->GetDataVNode();
	CaVNodeTreeItemData* pNode = treeVNode.FindNode (lpszNode);
	ASSERT (pNode);
	if (!pNode)
	{
		AfxMessageBox ("VNODE_AlterAttribute::Internal error(2): document corrupted");
		return FALSE;
	}
	if (pNode->GetNodeType() != NODExNORMAL)
		return FALSE;

	CaIngnetNode* pNormalNode = (CaIngnetNode*)pNode;
	CaNodeAttributeFolder& attributeFolder = pNormalNode->m_nodeAdvancedFolder.GetAttributeFolder();

	CaIngnetNodeAttribute* pFound = NULL;
	CTypedPtrList<CObList, CaVNodeTreeItemData*>& listAttribute = attributeFolder.GetListAttribute();
	POSITION pos = listAttribute.GetHeadPosition();
	while (pos != NULL)
	{
		CaIngnetNodeAttribute* pAttribute = (CaIngnetNodeAttribute*)listAttribute.GetNext(pos);
		if (pAttribute->GetName().CompareNoCase(lpszAttribute) == 0)
		{
			pFound = pAttribute;
			break;
		}
	}
	if (!pFound)
	{
		AfxMessageBox ("VNODE_AlterAttribute::Internal error(3): document corrupted");
		return FALSE;
	}

	CxDlgVirtualNodeAttribute dlg;
	dlg.SetAlter();
	dlg.m_strVNodeName     = lpszNode;
	dlg.m_strAttributeName = pFound->GetName() ;
	dlg.m_bPrivate = pFound->IsPrivate();
	INT_PTR res = dlg.DoModal();
	return (res == IDOK)? TRUE: FALSE;
}

BOOL VNODE_DropAttribute (LPCTSTR lpszNode, LPCTSTR lpszAttribute)
{
	try
	{
		CdMainDoc* pDoc = theApp.m_pDocument;
		ASSERT (pDoc);
		if (!pDoc)
		{
			AfxMessageBox ("VNODE_DropAttribute::Internal error(1): document corrupted");
			return FALSE;
		}

		CaTreeNodeFolder& treeVNode = pDoc->GetDataVNode();
		CaVNodeTreeItemData* pNode = treeVNode.FindNode (lpszNode);
		ASSERT (pNode);
		if (!pNode)
		{
			AfxMessageBox ("VNODE_DropAttribute::Internal error(2): document corrupted");
			return FALSE;
		}
		if (pNode->GetNodeType() != NODExNORMAL)
			return FALSE;

		CaIngnetNode* pNormalNode = (CaIngnetNode*)pNode;
		CaNodeAttributeFolder& attributeFolder = pNormalNode->m_nodeAdvancedFolder.GetAttributeFolder();

		CaIngnetNodeAttribute* pFound = NULL;
		CTypedPtrList<CObList, CaVNodeTreeItemData*>& listAttribute = attributeFolder.GetListAttribute();
		POSITION pos = listAttribute.GetHeadPosition();
		while (pos != NULL)
		{
			CaIngnetNodeAttribute* pAttribute = (CaIngnetNodeAttribute*)listAttribute.GetNext(pos);
			if (pAttribute->GetName().CompareNoCase(lpszAttribute) == 0)
			{
				pFound = pAttribute;
				break;
			}
		}
		if (!pFound)
		{
			AfxMessageBox ("VNODE_DropAttribute::Internal error(3): document corrupted");
			return FALSE;
		}
		
		CString strMsg;
#ifdef EDBC
		AfxFormatString2 (strMsg, IDS_DROP_SERVER_ATTRIBUTE, (LPCTSTR)lpszAttribute, lpszNode);
#else
		AfxFormatString2 (strMsg, IDS_DROP_NODE_ATTRIBUTE, (LPCTSTR)lpszAttribute, lpszNode);
#endif
		if (AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO) != IDYES)
			return FALSE;

		CaNodeDataAccess nodeAccess;
		nodeAccess.InitNodeList();
		CaNodeAttribute attr(lpszNode, lpszAttribute, pFound->GetAttributeValue(), pFound->IsPrivate());
		HRESULT hErr = VNODE_AttributeDrop(&attr);
		if (hErr == NOERROR)
			return TRUE;
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
		return FALSE;
	}
	catch (...)
	{

	}

	AfxFormatString1 (theApp.m_strMessage, IDS_DROP_NODE_ATTRIBUTE_FAILED, lpszNode);
	AfxMessageBox (theApp.m_strMessage, MB_OK|MB_ICONEXCLAMATION);
	return FALSE;
}

BOOL VNODE_IsExist (int nExe)
{
	CString strExe  = theApp.m_strInstallPath;
	strExe += consttchszIngVdba;
	//
	// Actually nExe is not differentiated: (whatever the value is, only the vdba.exe is to test the existence)
	switch (nExe)
	{
	case SQL_EXE:
		strExe += SQLEXE;
		break;
	case DOM_EXE:
		strExe += DOMEXE;
		break;
#if !defined (EDBC)
	case IPM_EXE:
		strExe += IPMEXE;
		break;
#endif
	default:
		return FALSE;
	}
	return (_taccess(strExe, 0) != -1)? TRUE: FALSE;
}
