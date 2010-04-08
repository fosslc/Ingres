/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : nodeinfo.cpp, Implementation File 
** 
**    Project  : Virtual Node Manager.
**    Author   : UK Sotheavut, 
**    Purpose  : Information about a node of Tree View of Node Data 
**               (image, expanded, ...) 
**
** History:
** xx-Mar-1999 (uk$so01)
**    created.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 10-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 11-Oct-2003 (schph01)
**    SIR #109864, Add VNODE_RunIngNetLocalVnode() function for launched 
**    ingnet with the -vnode option.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 24-Mar-2004 (uk$so01)
**    BUG #112013 / ISSUE 13278495
**    On some platforms and WinXPs, CrateProcess failed if the path
**    contains characters '[' ']'.
**    This change will remove the full path from the command line of the
**    CreateProcess()or ShellExecute() because now the path for the GUI
**    tools has been set by the installer.
*/

#include "stdafx.h"
#include "vnodemgr.h"
#include "nodeinfo.h"
#include "ingobdml.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#if defined (EDBC)
extern "C"
{
	// 
	// Unpublished function defined in libingll/nodes.c
	BOOL EDBC_GetConnectionString(LPCTSTR ServerName, char* ConnStr);
}
#endif



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CaTreeNodeInformation::CaTreeNodeInformation()
	:m_hTreeItem(NULL), m_bExpanded(FALSE), m_bQueried(FALSE)
{
	m_nImage = 0;
}

CaTreeNodeInformation::~CaTreeNodeInformation()
{

}


HTREEITEM VNODE_TreeAddItem (LPCTSTR lpszItem, CTreeCtrl* pTree, HTREEITEM hParent, HTREEITEM hInsertAfter, int nImage, DWORD_PTR dwData)
{
	TV_ITEM tvi; 
	TV_INSERTSTRUCT tvins; 
	
	tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	//
	// Set the text of the item. 
	tvi.pszText = (LPTSTR)lpszItem; 
	tvi.cchTextMax = lstrlen(lpszItem); 
	
	tvi.iImage = nImage;
	tvi.iSelectedImage = nImage; 
	tvins.item = tvi; 
	tvins.hInsertAfter = hInsertAfter;
	tvins.hParent = hParent;
	
	HTREEITEM hItem = pTree->InsertItem(&tvins);
	if (hItem)
		pTree->SetItemData (hItem, dwData);
	return hItem; 
}

static CaVNodeTreeItemData* VNODE_Exist (CTypedPtrList<CObList, CaVNodeTreeItemData*>& oldList, LPCTSTR lpszNode)
{
	CaVNodeTreeItemData* pNode = NULL;
	if (oldList.IsEmpty())
		return NULL;
	POSITION pos = oldList.GetHeadPosition();
	while (pos != NULL)
	{
		pNode = oldList.GetNext (pos);
		if (pNode->GetName().CompareNoCase(lpszNode) == 0)
			return pNode;
	}

	return NULL;
}

BOOL VNODE_QueryNode (
	CTypedPtrList<CObList, CaVNodeTreeItemData*>& oldList, 
	CTypedPtrList<CObList, CaVNodeTreeItemData*>& listNode)
{
	BOOL bOK = TRUE;
	TCHAR tchszMsg [256];
	CTypedPtrList<CObList, CaDBObject*> ls;
	try
	{
		//CaNodeDataAccess nodeAccess;
		CaVNodeTreeItemData* pItem = NULL;
		CaVNodeTreeItemData* pExist = NULL;
		CString strNodeName;
		HRESULT hErr = VNODE_QueryNode (ls);
		if (hErr != NOERROR)
			return FALSE;
		POSITION pos = ls.GetHeadPosition();
		INGRESII_MuteRefresh();
		while (pos != NULL)
		{
			pItem = NULL;
			CaNode* pObj = (CaNode*)ls.GetNext(pos);
			strNodeName = pObj->GetName();
			if (strNodeName.IsEmpty())
				continue;

			pExist = VNODE_Exist (oldList, strNodeName);
			if (pExist)
			{
				pExist->SetInternalState (NODE_EXIST);

				//
				// Node already exists =>
				// Update its characteristics if needed:
				if (pExist->GetNodeType() == NODExLOCAL)
				{
				}
				if (pExist->GetNodeType() == NODExINSTALLATION)
				{
				}
				else
				if (pExist->GetNodeType() == NODExNORMAL)
				{
					//
					// Update the Advanced Node Parameters:
					
					CaIngnetNode* pNode = (CaIngnetNode*)pExist;
					pNode->SetSimplified ((pObj->GetClassifiedFlag() & NODE_SIMPLIFY));
					pNode->m_nodeAdvancedFolder.InitData (strNodeName);
				}

				continue;
			}

			if (pObj->IsLocalNode())
			{
				pItem = (CaVNodeTreeItemData*)new CaNodeLocal();
				CaNodeLocal* pNodeLocal = (CaNodeLocal*)pItem;
				pNodeLocal->m_nodeServerFolder.SetLocal(TRUE);
				pNodeLocal->InitData (strNodeName);
			}
			else
			if (pObj->GetClassifiedFlag() & NODE_INSTALLATION)
			{
#if !defined (EDBC)
				CString strInstName;
				strInstName.Format (_T("<installation password node>[%s]"), (LPCTSTR)pObj->GetName());
				pItem = (CaVNodeTreeItemData*)new CaNodeInstallation();

				CaNodeInstallation* pNodeInst = (CaNodeInstallation*)pItem;
				pNodeInst->SetLogin (pObj->GetLogin());
				pNodeInst->SetDisplayString(strInstName);
#endif
			}
			else
			{
				CaIngnetNode* pNode = new CaIngnetNode();
				pNode->InitData (strNodeName);
				pNode->SetSimplified ((pObj->GetClassifiedFlag() & NODE_SIMPLIFY));
				pItem = (CaVNodeTreeItemData*)pNode;
			}

			if (pItem)
			{
				pItem->SetName (strNodeName);
				listNode.AddTail (pItem);
			}
		}
		INGRESII_UnMuteRefresh();
		bOK = TRUE;
	}
	catch (CeNodeException e)
	{
		INGRESII_UnMuteRefresh();
		AfxMessageBox (e.GetReason());
		bOK = FALSE;
	}
	catch (...)
	{
		lstrcpy (tchszMsg, _T("VNODE_QueryNode::Internal error (1), Faile to query list of nodes"));
		AfxMessageBox (tchszMsg);
		INGRESII_UnMuteRefresh();
		bOK = FALSE;
	}
	while (!ls.IsEmpty())
		delete ls.RemoveHead();
	return bOK;
}


BOOL VNODE_QueryLogin (LPCTSTR lpszNode, CTypedPtrList<CObList, CaVNodeTreeItemData*>& listItem)
{
	//
	// Mark all logins as delete:
	BOOL bOK = TRUE;
	CaVNodeTreeItemData* pItem = NULL;
	POSITION pos = listItem.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = listItem.GetNext (pos);
		pItem->SetInternalState (NODE_DELETE);
	}
	CTypedPtrList<CObList, CaDBObject*> ls;
	try
	{
		//
		// New list of logins:
		CaIngnetNodeLogin* pLogin = NULL;

		CaNode node (lpszNode);
		HRESULT hErr = VNODE_QueryLogin (&node, ls);
		if (hErr != NOERROR)
			return FALSE;
		pos = ls.GetHeadPosition();
		while (pos != NULL)
		{
			BOOL bExist = FALSE;
			CaNodeLogin* pObj = (CaNodeLogin*)ls.GetNext(pos);
			//
			// Check if login already exists:
			POSITION p = listItem.GetHeadPosition();
			while (p != NULL)
			{
				pItem = listItem.GetNext (p);
				if (pItem->GetName().CompareNoCase (pObj->GetLogin()) == 0)
				{
					//
					// Login exists, only get the new characteristics
					pItem->SetInternalState (NODE_EXIST);
					pLogin = (CaIngnetNodeLogin*)pItem;
					pLogin->SetPrivate (pObj->IsLoginPrivate());
#ifdef EDBC
					pLogin->SetSave (pObj->GetSaveLogin());
#endif
					bExist = TRUE;
					break;
				}
			}
			if (!bExist)
			{
				pLogin = new CaIngnetNodeLogin();
				pLogin->SetNodeName (lpszNode);
				pLogin->SetName (pObj->GetLogin());
				pLogin->SetPrivate (pObj->IsLoginPrivate());
#ifdef EDBC
				pLogin->SetSave (pObj->GetSaveLogin());
#endif
				listItem.AddTail (pLogin);
			}
		}
		bOK = TRUE;
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
		bOK = FALSE;
	}
	catch (...)
	{
		AfxMessageBox (_T("VNODE_QueryLogin::Internal error (1), Faile to query list of logins"));
		bOK = FALSE;
	}
	while (!ls.IsEmpty())
		delete ls.RemoveHead();
	return bOK;
}


BOOL VNODE_QueryConnection (LPCTSTR lpszNode, CTypedPtrList<CObList, CaVNodeTreeItemData*>& listItem)
{
	//
	// Mark all Connection Data as delete:
	CaVNodeTreeItemData* pItem = NULL;
	POSITION pos = listItem.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = listItem.GetNext (pos);
		pItem->SetInternalState (NODE_DELETE);
	}
	BOOL bOK = TRUE;
	CTypedPtrList<CObList, CaDBObject*> ls;

	try
	{
		CaNode node (lpszNode);
		CaIngnetNodeConnectData* pConnection = NULL;
		HRESULT hErr = VNODE_QueryConnection (&node, ls);
		if (hErr != NOERROR)
			return FALSE;
		pos = ls.GetHeadPosition();
		while (pos != NULL)
		{
			BOOL bExist = FALSE;
			CaNodeConnectData* pObj = (CaNodeConnectData*)ls.GetNext(pos);
			//
			// Check if Connection Data already exists:
			POSITION p = listItem.GetHeadPosition();
			while (p != NULL)
			{
				pItem = listItem.GetNext (p);
				if (pItem->GetName().CompareNoCase (pObj->GetIPAddress()) == 0)
				{
					pConnection = (CaIngnetNodeConnectData*)pItem;
					//
					// Check extra parameter to confirm the matching (symbolic name comparaison with case insenitive)...:
					if (pConnection->GetProtocol().CompareNoCase (pObj->GetProtocol()) == 0 &&
					    pConnection->GetListenAddress().CompareNoCase (pObj->GetListenAddress()) == 0)
					{
						//
						// Connection Data exists, only get the new characteristics
						pItem->SetInternalState (NODE_EXIST);
						pConnection->SetPrivate (pObj->IsConnectionPrivate());
						pConnection->SetProtocol(pObj->GetProtocol());
						pConnection->SetListenAddress(pObj->GetListenAddress());
						bExist = TRUE;
						break;
					}
				}
			}

			if (!bExist)
			{
				pConnection = new CaIngnetNodeConnectData();
				pConnection->SetNodeName (lpszNode);
				pConnection->SetName (pObj->GetIPAddress());
				pConnection->SetPrivate (pObj->IsConnectionPrivate());
				pConnection->SetProtocol(pObj->GetProtocol());
				pConnection->SetListenAddress(pObj->GetListenAddress());

				listItem.AddTail (pConnection);
			}
		}

		bOK = TRUE;
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
		bOK = FALSE;
	}
	catch (...)
	{
		AfxMessageBox (_T("VNODE_QueryConnection::Internal error (1), Faile to query list of connection data"));
		bOK = FALSE;
	}
	while (!ls.IsEmpty())
		delete ls.RemoveHead();
	return bOK;
}

BOOL VNODE_QueryAttribute  (LPCTSTR lpszNode, CTypedPtrList<CObList, CaVNodeTreeItemData*>& listItem)
{
	//
	// Mark all Attribute Data as delete:
	CaVNodeTreeItemData* pItem = NULL;
	POSITION pos = listItem.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = listItem.GetNext (pos);
		pItem->SetInternalState (NODE_DELETE);
	}

	//
	// New List of Attributes:
	BOOL bOK = TRUE;
	CTypedPtrList<CObList, CaDBObject*> ls;
	try
	{
		CaNode node (lpszNode);
		CaIngnetNodeAttribute* pAttribute = NULL;
		HRESULT hErr = VNODE_QueryAttribute (&node, ls);
		if (hErr != NOERROR)
			return FALSE;
		pos = ls.GetHeadPosition();
		while (pos != NULL)
		{
			BOOL bExist = FALSE;
			CaNodeAttribute* pObj = (CaNodeAttribute*)ls.GetNext(pos);

			//
			// Check if Attribute already exists:
			POSITION p = listItem.GetHeadPosition();
			while (p != NULL)
			{
				pItem = listItem.GetNext (p);
				if (pItem->GetName().CompareNoCase (pObj->GetAttributeName()) == 0)
				{
					//
					// Attribute exists, only get the new characteristics
					pItem->SetInternalState (NODE_EXIST);
					pAttribute = (CaIngnetNodeAttribute*)pItem;
					pAttribute->SetPrivate (pObj->IsAttributePrivate());
					pAttribute->SetAttributeValue (pObj->GetAttributeValue());
					bExist = TRUE;
					break;
				}
			}

			if (!bExist)
			{
				pAttribute = new CaIngnetNodeAttribute();
				pAttribute->SetNodeName (lpszNode);
				pAttribute->SetName (pObj->GetAttributeName());
				pAttribute->SetPrivate (pObj->IsAttributePrivate());
				pAttribute->SetAttributeValue (pObj->GetAttributeValue());
				listItem.AddTail (pAttribute);
			}
		}
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
		bOK = FALSE;
	}
	catch (...)
	{
		AfxMessageBox (_T("VNODE_QueryAttribute::Internal error (1), Faile to query list of attributes"));
		bOK = FALSE;
	}
	while (!ls.IsEmpty())
		delete ls.RemoveHead();
	return bOK;
}

BOOL VNODE_QueryServer  (LPCTSTR lpszNode, CTypedPtrList<CObList, CaVNodeTreeItemData*>& listItem, BOOL bLocal)
{
	//
	// Mark all Servers as delete:
	CaVNodeTreeItemData* pItem = NULL;
	POSITION pos = listItem.GetHeadPosition();
	while (pos != NULL)
	{
		pItem = listItem.GetNext (pos);
		pItem->SetInternalState (NODE_DELETE);
	}
	//
	// New List of Servers:
	BOOL bOK = TRUE;
	CTypedPtrList<CObList, CaDBObject*> ls;
	try
	{
		CaIngnetNodeServer* pServer = NULL;
		CaNode node (lpszNode);
		HRESULT hErr = VNODE_QueryServer (&node, ls);
		if (hErr != NOERROR)
			return FALSE;
		pos = ls.GetHeadPosition();
		while (pos != NULL)
		{
			BOOL bExist = FALSE;
			CaNodeServer* pObj = (CaNodeServer*)ls.GetNext(pos);
			//
			// Check if Server already exists:
			POSITION p = listItem.GetHeadPosition();
			while (p != NULL)
			{
				pItem = listItem.GetNext(p);
				if (pItem->GetName().CompareNoCase (pObj->GetName()) == 0)
				{
					//
					// Server exists, only get the new characteristics
					pItem->SetInternalState (NODE_EXIST);
					bExist = TRUE;
					break;
				}
			}
			if (!bExist)
			{
				pServer = new CaIngnetNodeServer();
				pServer->SetNodeName(lpszNode);
				pServer->SetName(pObj->GetName());
				pServer->SetLocal(bLocal);
				listItem.AddTail (pServer);
			}
		}
	}
	catch (CeNodeException e)
	{
		AfxMessageBox (e.GetReason());
		bOK = FALSE;
	}
	catch (...)
	{
		AfxMessageBox (_T("VNODE_QueryServer::Internal error (1), Faile to query list of servers"));
		bOK = FALSE;
	}
	while (!ls.IsEmpty())
		delete ls.RemoveHead();
	return bOK;
}

//
// Start the VDBA on Context:
BOOL VNODE_RunVdbaSql(LPCTSTR lpszNode)
{
	CString strCmdline = _T("");
	CString strFile = SQLEXE;
	//
	// Old code that launches vdba incontext.
	// From now on, the vdbasql is used instead of vdba in context:
	/*
#ifdef EDBC
	BOOL bSuccess;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;
	CString strCommand;

	// char * VDBA_GetVdbaExeName() handles the #ifdef MAINWIN case
	char	ConnStr[200];
	char	vnode[128],*pS,*pT;
	BOOL bRet;

	//
	// If it is a "vnode /gateway", get the vnode.
	//
	if (strstr(lpszNode, " /"))
	{
		pT = vnode;
		pS = (char *)lpszNode;
		while (*pS != ' ' && *pS != '\0')
			*pT++ = *pS++;
		*pT = '\0';
		
		bRet = EDBC_GetConnectionString(vnode, ConnStr);
	}
	else
	{
		bRet = EDBC_GetConnectionString(lpszNode, ConnStr);
	}
	if (bRet == TRUE)
		//
		// When a connection string is used, the next parameter must be the server name.
		//
		strCommand.Format (_T("%s /c MAXWIN NONODESWINDOW NOMAINTOOLBAR NOSPLASHSCREEN NOSAVEONEXIT SQL NOMENU \"%s\" \"%s\""), (LPCTSTR)strFile, ConnStr, lpszNode);
	else
		strCommand.Format (_T("%s /c MAXWIN NONODESWINDOW NOMAINTOOLBAR NOSPLASHSCREEN NOSAVEONEXIT SQL NOMENU %s"), (LPCTSTR)strFile, lpszNode);
	
	LPTSTR lpszCommand = (LPTSTR)(LPCTSTR)strCommand;

	memset(&StartupInfo, 0, sizeof(STARTUPINFO));
	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartupInfo.wShowWindow = SW_SHOWDEFAULT;

	bSuccess = CreateProcess(
		NULL,           // pointer to name of executable module
		lpszCommand,    // pointer to command line string
		NULL,           // pointer to process security attributes
		NULL,           // pointer to thread security attributes
		FALSE,          // handle inheritance flag
		0,              // creation flags
		NULL,           // pointer to new environment block
		NULL,           // pointer to current directory name
		&StartupInfo,   // pointer to STARTUPINFO
		&ProcInfo       // pointer to PROCESS_INFORMATION
	);

	if (!bSuccess)
	{
		CString strMsg = _T("Failed to run SQL Test");
		return FALSE;
	}
	return TRUE;
#else 
	*/

	if (lpszNode && _tcslen(lpszNode) > 0)
	{
		strCmdline.Format(_T(" -node=%s"), lpszNode);
#if defined (EDBC)
		char ConnStr[200];
		char vnode[128],*pS,*pT;
		BOOL bRet = FALSE;

		ConnStr[0] = '\0';
		if (strstr(lpszNode, " /"))
		{
			pT = vnode;
			pS = (char *)lpszNode;
			while (*pS != ' ' && *pS != '\0')
				*pT++ = *pS++;
			*pT = '\0';
			
			bRet = EDBC_GetConnectionString(vnode, ConnStr);
		}
		else
		{
			bRet = EDBC_GetConnectionString(lpszNode, ConnStr);
		}
		if (bRet == TRUE && ConnStr[0])
		{
			CString strConn;
			strConn.Format(_T(" -connectinfo=\"%s\""), ConnStr);
			strCmdline += strConn;
		}
#endif
	}
	
	SHELLEXECUTEINFO shell;
	memset (&shell, 0, sizeof(shell));
	shell.cbSize = sizeof(shell);
	shell.fMask  = 0;
	shell.lpFile = strFile;
	shell.lpParameters = strCmdline;
	shell.nShow  = SW_SHOWNORMAL;
	CWnd* pMainWnd = AfxGetMainWnd();
	if (pMainWnd)
		shell.hwnd = pMainWnd->m_hWnd;
	ShellExecuteEx(&shell);
	return TRUE;
}

//
// Start the VDBA on Context:
BOOL VNODE_RunVdbaDom(LPCTSTR lpszNode)
{
	BOOL bSuccess;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;
	CString strCommand;

	CString strFile = DOMEXE;
#ifdef	EDBC
	char	ConnStr[200];
	char	vnode[128],*pS,*pT;
	BOOL bRet;

	/*
	** If it is a "vnode /gateway" string, get the vnode.
	*/
	if (strstr(lpszNode, " /"))
	{
		pT = vnode;
		pS = (char *)lpszNode;
		while (*pS != ' ' && *pS != '\0')
			*pT++ = *pS++;
		*pT = '\0';
		
		bRet = EDBC_GetConnectionString(vnode, ConnStr);
	}
	else
	{
		bRet = EDBC_GetConnectionString(lpszNode, ConnStr);
	}
	if (bRet == TRUE)
		/*
		** When a connection string is used, the next parameter must be the server name.
		*/
		strCommand.Format (_T("%s /c MAXWIN NONODESWINDOW NOMAINTOOLBAR NOSPLASHSCREEN NOSAVEONEXIT DOM NOMENU \"%s\" \"%s\""), (LPCTSTR)strFile, ConnStr, lpszNode);
	else
		strCommand.Format (_T("%s /c MAXWIN NONODESWINDOW NOMAINTOOLBAR NOSPLASHSCREEN NOSAVEONEXIT DOM NOMENU %s"), (LPCTSTR)strFile, lpszNode);
#else
	strCommand.Format (_T("%s /c MAXWIN NONODESWINDOW NOMAINTOOLBAR NOSPLASHSCREEN NOSAVEONEXIT DOM NOMENU %s"), (LPCTSTR)strFile, lpszNode);
#endif
	
	LPTSTR lpszCommand = (LPTSTR)(LPCTSTR)strCommand;

	memset(&StartupInfo, 0, sizeof(STARTUPINFO));
	StartupInfo.cb = sizeof(STARTUPINFO);
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartupInfo.wShowWindow = SW_SHOWDEFAULT;

	bSuccess = CreateProcess(
		NULL,           // pointer to name of executable module
		lpszCommand,    // pointer to command line string
		NULL,           // pointer to process security attributes
		NULL,           // pointer to thread security attributes
		FALSE,          // handle inheritance flag
		0,              // creation flags
		NULL,           // pointer to new environment block
		NULL,           // pointer to current directory name
		&StartupInfo,   // pointer to STARTUPINFO
		&ProcInfo       // pointer to PROCESS_INFORMATION
	);


	if (!bSuccess)
	{
		CString strMsg = _T("Failed to run DOM Window");
		return FALSE;
	}
	return TRUE;
}

//
// Start the VDBA on Context:
BOOL VNODE_RunVdbaIpm(LPCTSTR lpszNode)
{
	CString strCmdline = _T("");
	CString strFile = IPMEXE;
	if (lpszNode && _tcslen(lpszNode) > 0)
		strCmdline.Format(_T(" -node=%s -noapply"), lpszNode);
	SHELLEXECUTEINFO shell;
	memset (&shell, 0, sizeof(shell));
	shell.cbSize = sizeof(shell);
	shell.fMask  = 0;
	shell.lpFile = strFile;
	shell.lpParameters = strCmdline;
	shell.nShow  = SW_SHOWNORMAL;
	CWnd* pMainWnd = AfxGetMainWnd();
	if (pMainWnd)
		shell.hwnd = pMainWnd->m_hWnd;
	ShellExecuteEx(&shell);
	return TRUE;
}

//
// Start the INGNET on Context:
BOOL VNODE_RunIngNetLocalVnode(LPCTSTR lpszNode)
{
	CString strCmdline = _T("");
	CString strFile = _T("ingnet");
	if (lpszNode && _tcslen(lpszNode) > 0)
		strCmdline.Format(_T(" -vnode=%s"), lpszNode);
	SHELLEXECUTEINFO shell;
	memset (&shell, 0, sizeof(shell));
	shell.cbSize = sizeof(shell);
	shell.fMask  = 0;
	shell.lpFile = strFile;
	shell.lpParameters = strCmdline;
	shell.nShow  = SW_SHOWNORMAL;
	CWnd* pMainWnd = AfxGetMainWnd();
	if (pMainWnd)
		shell.hwnd = pMainWnd->m_hWnd;
	ShellExecuteEx(&shell);
	return TRUE;
}

