/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : vnodedml.cpp: Implementation for accessing Virtual Node Data
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Access Virtual Node object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 02-May-2002 (noifr01)
**    (sir 106648) cleaunup for the VDBA split project. removed the
**    NODE_LOCAL_BOTHINGRESxDT definition (ingres/desktop no more managed
**    specifically)
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 27-Jun-2003 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 10-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 13-Oct-2003 (schph01)
**    SIR #109864 Manage the -vnode command line option.
** 14-Nov-2003 (uk$so01)
**    BUG #110983, If GCA Protocol Level = 50 then use MIB Objects 
**    to query the server classes.
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
** 09-Mar-2010 (drivi01) SIR 123397
**    Update NodeCheckConnection to return STATUS code instead of BOOL.
**    The error code will be used to get the error from ERlookup.
**    Add new function INGRESII_ERLookup to retrieve an error message.
**/



#include "stdafx.h"
#include "dmlvnode.h"
#include "vnodedml.h"
#include "constdef.h"
#include "usrexcep.h"
#include "mibobjs.h"

extern "C"
{
#include "nodes.h"
BOOL ping_iigcn( void );
}


CaNodeDataAccess::CaNodeDataAccess(BOOL bDestroyOnDestructor)
{
	m_bDestroy = bDestroyOnDestructor;
	m_bStatus = FALSE;
	int nResult = NodeLLInit();
	if (nResult == RES_SUCCESS)
		m_bStatus = TRUE;
	else
	{
		throw CeNodeException(_T("Internal error(low level 1): Cannot access node data"));
	}
}

CaNodeDataAccess::~CaNodeDataAccess()
{
	if (m_bStatus && m_bDestroy)
		NodeLLTerminate();
}

BOOL CaNodeDataAccess::InitNodeList()
{
	int nRes;
	if (IsVnodeOptionDefined()) // ingnet launch with option -vnode doesn't display "(local)" node
		nRes = NodeLLFillNodeLists_NoLocal();
	else
		nRes = NodeLLFillNodeLists();

	if (nRes != RES_SUCCESS)
		throw CeNodeException(_T("Internal error(low level 2): Cannot access node data"));
	return (nRes == RES_SUCCESS)? TRUE: FALSE;
}

BOOL CaNodeDataAccess::InitHostName(LPCTSTR lpszHostName)
{
	return(NodeLL_FillHostName(lpszHostName));
}

BOOL CaNodeDataAccess::IsVnodeOptionDefined()
{
	return (NodeLL_IsVnodeOptionDefined());
}


// ****************************************************************************
// Query the list of Virtual Nodes, Servers, Logins, Connection Data, Attributes:
// ****************************************************************************

//
// Query the list of Virtual Nodes:
HRESULT VNODE_QueryNode (CTypedPtrList<CObList, CaDBObject*>& list)
{
	HRESULT hErr = NOERROR;
	CaNodeDataAccess nodeAccess;
	CaNode* pItem = NULL;
	CString strNodeName;

	nodeAccess.InitNodeList();
	LPMONNODEDATA pFirtNode = GetFirstLLNode();
	while (pFirtNode)
	{
		pItem = NULL;
		strNodeName = pFirtNode->MonNodeDta.NodeName;
		if (strNodeName.IsEmpty())
		{
			//
			// Normally, this should not happen:
			pFirtNode = pFirtNode->pnext;
			continue;
		}
		
		pItem = new CaNode(strNodeName);
		UINT nClassifiedFlag = pItem->GetClassifiedFlag();
		if (pFirtNode->MonNodeDta.isSimplifiedModeOK)
			nClassifiedFlag |= NODE_SIMPLIFY;
		if (pFirtNode->MonNodeDta.bIsLocal)
			nClassifiedFlag |= NODE_LOCAL;
		if (pFirtNode->MonNodeDta.bInstallPassword)
			nClassifiedFlag |= NODE_INSTALLATION;
		if (pFirtNode->MonNodeDta.bTooMuchInfoInInstallPassword)
			nClassifiedFlag |= NODE_TOOMUCHINSTALLATION;
		if (pFirtNode->MonNodeDta.bWrongLocalName)
			nClassifiedFlag |= NODE_WRONG_LOCALNAME;
		pItem->SetClassifiedFlag (nClassifiedFlag);

		pItem->SetNbConnectData(pFirtNode->MonNodeDta.inbConnectData);
		pItem->SetNbLoginData(pFirtNode->MonNodeDta.inbLoginData);
#if defined (EDBC)
		pItem->SetSaveLogin(pFirtNode->MonNodeDta.LoginDta.bSave);
#endif

		//
		// Login Data:
		pItem->SetLogin ((LPTSTR)pFirtNode->MonNodeDta.LoginDta.Login);
		pItem->SetPassword((LPTSTR)pFirtNode->MonNodeDta.LoginDta.Passwd);
		pItem->LoginPrivate(pFirtNode->MonNodeDta.LoginDta.bPrivate);
		//
		// Connection Data:
		pItem->SetIPAddress ((LPTSTR)pFirtNode->MonNodeDta.ConnectDta.Address);
		pItem->SetProtocol ((LPTSTR)pFirtNode->MonNodeDta.ConnectDta.Protocol);
		pItem->SetListenAddress ((LPTSTR)pFirtNode->MonNodeDta.ConnectDta.Listen);
		pItem->ConnectionPrivate (pFirtNode->MonNodeDta.ConnectDta.bPrivate);

		if (pItem)
		{
			//pItem->SetName (strNodeName);
			list.AddTail (pItem);
		}
		
		pFirtNode = pFirtNode->pnext;
	}

	return NOERROR;
}


HRESULT VNODE_QueryServer (CaNode* pNode, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
#if !defined (EDBC)
	CaNodeDataAccess nodeAccess;
	nodeAccess.InitNodeList();
#endif

	int nError = 0;
	CaNodeServer* pServer = NULL;
	LPLISTTCHAR ls = GetGWlist(pNode->GetName(), &nError);
	LPLISTTCHAR lpgwlist = ls;
	if (nError != 0)
		return E_FAIL;
	//
	// New List of Servers:
	while (lpgwlist)
	{
		BOOL bExist = FALSE;
		//
		// Check if Server already exists:
		POSITION pos = listObject.GetHeadPosition();
		while (pos != NULL)
		{
			CaDBObject* pObj = listObject.GetNext (pos);
			if (pObj->GetName().CompareNoCase (lpgwlist->lptchItem) == 0)
			{
				//
				// Server exists, only get the new characteristics
				bExist = TRUE;
				break;
			}
		}

		if (!bExist)
		{
			pServer = new CaNodeServer(pNode->GetName(), lpgwlist->lptchItem, pNode->IsLocalNode());
			listObject.AddTail (pServer);
		}

		lpgwlist = lpgwlist->next;
	}
	ls = ListTchar_Done(ls);
	return NOERROR;
}


HRESULT VNODE_QueryLogin (CaNode* pNode, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	HRESULT hErr = NOERROR;
	CaNodeDataAccess nodeAccess;
	nodeAccess.InitNodeList();
	//
	// New list of logins:
	CaNodeLogin* pLogin = NULL;
	LPNODELOGINDATA pFirst = GetFirstLLLoginData();
	while (pFirst)
	{
		BOOL bExist = FALSE;
		if (lstrcmpi (pNode->GetName(), (LPCTSTR)pFirst->NodeLoginDta.NodeName) == 0)
		{
			pLogin = new CaNodeLogin(
			    pNode->GetName(), 
			    (LPCTSTR)pFirst->NodeLoginDta.Login, 
			    _T(""),
			    pFirst->NodeLoginDta.bPrivate);
#if defined (EDBC)
			pLogin->SetSaveLogin(pFirst->NodeLoginDta.bSave);
#endif
			listObject.AddTail (pLogin);
		}
		pFirst = pFirst->pnext;
	}
	return hErr;
}

HRESULT VNODE_QueryConnection (CaNode* pNode, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	HRESULT hErr = NOERROR;
	CaNodeDataAccess nodeAccess;
	nodeAccess.InitNodeList();
	//
	// New List of Connection Data:
	CaNodeConnectData* pConnection = NULL;
	LPNODECONNECTDATA pFirst = GetFirstLLConnectData();
	while (pFirst)
	{
		if (lstrcmpi (pNode->GetName(), (LPCTSTR)pFirst->NodeConnectionDta.NodeName) == 0)
		{
			pConnection = new CaNodeConnectData(
			    pNode->GetName(),
			    (LPCTSTR)pFirst->NodeConnectionDta.Address,
			    (LPCTSTR)pFirst->NodeConnectionDta.Protocol,
			    (LPCTSTR)pFirst->NodeConnectionDta.Listen,
			    pFirst->NodeConnectionDta.bPrivate);

			listObject.AddTail (pConnection);
		}
		pFirst = pFirst->pnext;
	}
	return hErr;
}


HRESULT VNODE_QueryAttribute (CaNode* pNode, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	HRESULT hErr = NOERROR;
	CaNodeDataAccess nodeAccess;
	nodeAccess.InitNodeList();
	//
	// New List of Attributes:
	CaNodeAttribute* pAttribute = NULL;
	LPNODEATTRIBUTEDATA pFirst = GetFirstLLAttributeData();
	while (pFirst)
	{
		if (lstrcmpi (pNode->GetName(), (LPCTSTR)pFirst->NodeAttributeDta.NodeName) == 0)
		{
			pAttribute = new CaNodeAttribute(
			    pNode->GetName(),
			    (LPCTSTR)pFirst->NodeAttributeDta.AttributeName,
			    (LPCTSTR)pFirst->NodeAttributeDta.AttributeValue,
			    pFirst->NodeAttributeDta.bPrivate);

			listObject.AddTail (pAttribute);
		}
		pFirst = pFirst->pnext;
	}
	return hErr;
}

















// ****************************************************************************
// Virtual Node  (Add, Drop, Alter)
// ****************************************************************************

HRESULT VNODE_AddNode (CaNode* pNode)
{
	HRESULT hErr = NOERROR;
	int result = RES_ERR;

	NODEDATAMIN s;
	memset (&s, 0, sizeof (s));
	s.ConnectDta.bPrivate = pNode->IsConnectionPrivate();
	s.LoginDta.bPrivate   = pNode->IsLoginPrivate();

	s.ConnectDta.bPrivate = pNode->IsConnectionPrivate();
	s.LoginDta.bPrivate   = pNode->IsLoginPrivate();
	s.isSimplifiedModeOK  = TRUE;    // always
	s.inbConnectData = 1;
	s.inbLoginData   = 1;
#if defined (EDBC)
	s.LoginDta.bSave = pNode->GetSaveLogin();
#endif

	//
	// Main Node:
	lstrcpy ((LPTSTR)s.NodeName,            (LPCTSTR)pNode->GetName());
	//
	// Login Data:
	lstrcpy ((LPTSTR)s.LoginDta.NodeName,   (LPCTSTR)pNode->GetName());
	lstrcpy ((LPTSTR)s.LoginDta.Login,      (LPCTSTR)pNode->GetLogin());
	lstrcpy ((LPTSTR)s.LoginDta.Passwd,     (LPCTSTR)pNode->GetPassword());
	//
	// Connection Data:
	lstrcpy ((LPTSTR)s.ConnectDta.NodeName, (LPCTSTR)pNode->GetName());
	lstrcpy ((LPTSTR)s.ConnectDta.Address,  (LPCTSTR)pNode->GetIPAddress());
	lstrcpy ((LPTSTR)s.ConnectDta.Protocol, (LPCTSTR)pNode->GetProtocol());
	lstrcpy ((LPTSTR)s.ConnectDta.Listen,   (LPCTSTR)pNode->GetListenAddress());

	if (NodeLLInit() == RES_SUCCESS)
	{
		result = LLAddFullNode (&s);
		NodeLLTerminate();
	}
	else
		result = RES_ERR;
	
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}



HRESULT VNODE_DropNode (CaNode* pNode)
{
	HRESULT hErr = NOERROR;
	int result = RES_ERR;

//	CaNodeDataAccess nodeAccess;
	NODEDATAMIN nodedatamin;
	memset (&nodedatamin, 0, sizeof(nodedatamin));
	lstrcpy ((LPTSTR)nodedatamin.NodeName, (LPCTSTR)pNode->GetName());
//	nodeAccess.InitNodeList();
	result = LLDropFullNode (&nodedatamin);

	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}


HRESULT VNODE_AlterNode(CaNode* pNodeOld, CaNode* pNodeNew)
{
	int result = RES_ERR;
	HRESULT hErr = NOERROR;

	NODEDATAMIN oldS;
	NODEDATAMIN newS;
	memset (&oldS, 0, sizeof(oldS));
	memset (&newS, 0, sizeof(newS));
//	CaNodeDataAccess nodeAccess;
//	nodeAccess.InitNodeList();

	oldS.ConnectDta.bPrivate = pNodeOld->IsConnectionPrivate();
	oldS.LoginDta.bPrivate   = pNodeOld->IsLoginPrivate();
	oldS.isSimplifiedModeOK  = TRUE;    // always
	oldS.inbConnectData = 1;
	oldS.inbLoginData   = 1;
#if defined (EDBC)
	oldS.LoginDta.bSave = pNodeOld->GetSaveLogin();
#endif
	//
	// Main Node:
	lstrcpy ((LPTSTR)oldS.NodeName,            (LPCTSTR)pNodeOld->GetName());
	//
	// Login Data:
	lstrcpy ((LPTSTR)oldS.LoginDta.NodeName,   (LPCTSTR)pNodeOld->GetName());
	lstrcpy ((LPTSTR)oldS.LoginDta.Login,      (LPCTSTR)pNodeOld->GetLogin());
	lstrcpy ((LPTSTR)oldS.LoginDta.Passwd,     (LPCTSTR)pNodeOld->GetPassword());
	//
	// Connection Data:
	lstrcpy ((LPTSTR)oldS.ConnectDta.NodeName, (LPCTSTR)pNodeOld->GetName());
	lstrcpy ((LPTSTR)oldS.ConnectDta.Address,  (LPCTSTR)pNodeOld->GetIPAddress());
	lstrcpy ((LPTSTR)oldS.ConnectDta.Protocol, (LPCTSTR)pNodeOld->GetProtocol());
	lstrcpy ((LPTSTR)oldS.ConnectDta.Listen,   (LPCTSTR)pNodeOld->GetListenAddress());


	newS.ConnectDta.bPrivate = pNodeNew->IsConnectionPrivate();
	newS.LoginDta.bPrivate   = pNodeNew->IsLoginPrivate();
	newS.isSimplifiedModeOK  = TRUE;    // always
	newS.inbConnectData = 1;
	newS.inbLoginData   = 1;
#if defined (EDBC)
	newS.LoginDta.bSave = pNodeNew->GetSaveLogin();
#endif
	//
	// Main Node:
	lstrcpy ((LPTSTR)newS.NodeName,            (LPCTSTR)pNodeNew->GetName());
	//
	// Login Data:
	lstrcpy ((LPTSTR)newS.LoginDta.NodeName,   (LPCTSTR)pNodeNew->GetName());
	lstrcpy ((LPTSTR)newS.LoginDta.Login,      (LPCTSTR)pNodeNew->GetLogin());
	lstrcpy ((LPTSTR)newS.LoginDta.Passwd,     (LPCTSTR)pNodeNew->GetPassword());
	//
	// Connection Data:
	lstrcpy ((LPTSTR)newS.ConnectDta.NodeName, (LPCTSTR)pNodeNew->GetName());
	lstrcpy ((LPTSTR)newS.ConnectDta.Address,  (LPCTSTR)pNodeNew->GetIPAddress());
	lstrcpy ((LPTSTR)newS.ConnectDta.Protocol, (LPCTSTR)pNodeNew->GetProtocol());
	lstrcpy ((LPTSTR)newS.ConnectDta.Listen,   (LPCTSTR)pNodeNew->GetListenAddress());

	result = LLAlterFullNode(&oldS, &newS);
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}


// ****************************************************************************
// Virtual Node Login  (Add, Drop, Alter)
// ****************************************************************************

HRESULT VNODE_LoginAdd (CaNodeLogin* pLogin)
{
	HRESULT hErr = NOERROR;
	int result = RES_ERR;
	NODELOGINDATAMIN s;
	memset (&s, 0, sizeof (s));

	s.bPrivate = pLogin->IsLoginPrivate();
	lstrcpy ((LPTSTR)s.NodeName,   (LPCTSTR)pLogin->GetNodeName());
	lstrcpy ((LPTSTR)s.Login,      (LPCTSTR)pLogin->GetLogin());
	lstrcpy ((LPTSTR)s.Passwd,     (LPCTSTR)pLogin->GetPassword());

//	CaNodeDataAccess nodeAccess;
	result = LLAddNodeLoginData (&s);
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}

HRESULT VNODE_LoginDrop (CaNodeLogin* pLogin)
{
	int result = RES_ERR;
	HRESULT hErr = NOERROR;
	NODELOGINDATAMIN login;
	memset (&login, 0, sizeof(login));
	lstrcpy ((LPTSTR)login.NodeName, (LPCTSTR)pLogin->GetNodeName());
	lstrcpy ((LPTSTR)login.Login,    (LPCTSTR)pLogin->GetLogin());
	login.bPrivate = pLogin->IsLoginPrivate();

//	CaNodeDataAccess nodeAccess;
//	nodeAccess.InitNodeList();
	result = LLDropNodeLoginData (&login);
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;

}

HRESULT VNODE_LoginAlter     (CaNodeLogin* pLoginOld, CaNodeLogin* pLoginNew)
{
	int result = RES_ERR;
	HRESULT hErr = NOERROR;
	NODELOGINDATAMIN s;
	NODELOGINDATAMIN so;
	memset (&s,  0, sizeof (s));
	memset (&so, 0, sizeof (so));
	s.bPrivate  = pLoginNew->IsLoginPrivate();
	so.bPrivate = pLoginOld->IsLoginPrivate();

	lstrcpy ((LPTSTR)s.NodeName,   (LPCTSTR)pLoginNew->GetNodeName());
	lstrcpy ((LPTSTR)s.Login,      (LPCTSTR)pLoginNew->GetLogin());
	lstrcpy ((LPTSTR)s.Passwd,     (LPCTSTR)pLoginNew->GetPassword());

	lstrcpy ((LPTSTR)so.NodeName,   (LPCTSTR)pLoginOld->GetNodeName());
	lstrcpy ((LPTSTR)so.Login,      (LPCTSTR)pLoginOld->GetLogin());
	lstrcpy ((LPTSTR)so.Passwd,     (LPCTSTR)pLoginOld->GetPassword());

//	CaNodeDataAccess nodeAccess;
	result =  LLAlterNodeLoginData (&so, &s);
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}


// ****************************************************************************
// Virtual Node Connection Data (Add, Drop, Alter)
// ****************************************************************************

HRESULT VNODE_ConnectionAdd  (CaNodeConnectData* pConnection)
{
	int result;
	HRESULT hErr = NOERROR;
	NODECONNECTDATAMIN s;
	memset (&s, 0, sizeof (s));
	s.bPrivate = pConnection->IsConnectionPrivate();

	lstrcpy ((LPTSTR)s.NodeName, (LPCTSTR)pConnection->GetNodeName());
	lstrcpy ((LPTSTR)s.Address,  (LPCTSTR)pConnection->GetIPAddress());
	lstrcpy ((LPTSTR)s.Protocol, (LPCTSTR)pConnection->GetProtocol());
	lstrcpy ((LPTSTR)s.Listen,   (LPCTSTR)pConnection->GetListenAddress());

//	CaNodeDataAccess nodeAccess;
	result = LLAddNodeConnectData (&s);
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}


HRESULT VNODE_ConnectionDrop (CaNodeConnectData* pConnection)
{
	int result = RES_ERR;
	HRESULT hErr = NOERROR;
	NODECONNECTDATAMIN connection;
	memset (&connection, 0, sizeof(connection));

	connection.bPrivate = pConnection->IsConnectionPrivate();
	lstrcpy ((LPTSTR)connection.NodeName, (LPCTSTR)pConnection->GetNodeName());
	lstrcpy ((LPTSTR)connection.Address,  (LPCTSTR)pConnection->GetIPAddress());
	lstrcpy ((LPTSTR)connection.Protocol, (LPCTSTR)pConnection->GetProtocol());
	lstrcpy ((LPTSTR)connection.Listen,   (LPCTSTR)pConnection->GetListenAddress());

//	CaNodeDataAccess nodeAccess;
//	nodeAccess.InitNodeList();
	result = LLDropNodeConnectData (&connection);
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}

HRESULT VNODE_ConnectionAlter(CaNodeConnectData* pConnectionOld, CaNodeConnectData* pConnectionNew)
{
	int result;
	HRESULT hErr = NOERROR;
	NODECONNECTDATAMIN s, so;
	memset (&s,  0, sizeof (s));
	memset (&so, 0, sizeof (so));
	s.bPrivate  = pConnectionNew->IsConnectionPrivate();
	so.bPrivate = pConnectionOld->IsConnectionPrivate();

	lstrcpy ((LPTSTR)s.NodeName, (LPCTSTR)pConnectionNew->GetNodeName());
	lstrcpy ((LPTSTR)s.Address,  (LPCTSTR)pConnectionNew->GetIPAddress());
	lstrcpy ((LPTSTR)s.Protocol, (LPCTSTR)pConnectionNew->GetProtocol());
	lstrcpy ((LPTSTR)s.Listen,   (LPCTSTR)pConnectionNew->GetListenAddress());

	lstrcpy ((LPTSTR)so.NodeName, (LPCTSTR)pConnectionOld->GetNodeName());
	lstrcpy ((LPTSTR)so.Address,  (LPCTSTR)pConnectionOld->GetIPAddress());
	lstrcpy ((LPTSTR)so.Protocol, (LPCTSTR)pConnectionOld->GetProtocol());
	lstrcpy ((LPTSTR)so.Listen,   (LPCTSTR)pConnectionOld->GetListenAddress());

//	CaNodeDataAccess nodeAccess;
	result =  LLAlterNodeConnectData (&so, &s);
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}


// ****************************************************************************
// Virtual Node Attribute (Add, Drop, Alter)
// ****************************************************************************

HRESULT VNODE_AttributeAdd (CaNodeAttribute* pAttribute)
{
	int result = RES_ERR;
	HRESULT hErr = NOERROR;
	NODEATTRIBUTEDATAMIN nattr;
	memset (&nattr,    0, sizeof(nattr));
	//
	// New:
	nattr.bPrivate = pAttribute->IsAttributePrivate();
	lstrcpy ((LPTSTR)nattr.NodeName,       (LPCTSTR)pAttribute->GetNodeName());
	lstrcpy ((LPTSTR)nattr.AttributeName,  (LPCTSTR)pAttribute->GetAttributeName());
	lstrcpy ((LPTSTR)nattr.AttributeValue, (LPCTSTR)pAttribute->GetAttributeValue());

//	CaNodeDataAccess nodeAccess;
	result = LLAddNodeAttributeData (&nattr);

	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}

HRESULT VNODE_AttributeDrop (CaNodeAttribute* pAttribute)
{
	int result = RES_ERR;
	HRESULT hErr = NOERROR;
	NODEATTRIBUTEDATAMIN nattr;
	memset (&nattr, 0, sizeof(nattr));

	nattr.bPrivate = pAttribute->IsAttributePrivate();
	lstrcpy ((LPTSTR)nattr.NodeName,       (LPCTSTR)pAttribute->GetNodeName());
	lstrcpy ((LPTSTR)nattr.AttributeName,  (LPCTSTR)pAttribute->GetAttributeName());
	lstrcpy ((LPTSTR)nattr.AttributeValue, (LPCTSTR)pAttribute->GetAttributeValue());

//	CaNodeDataAccess nodeAccess;
//	nodeAccess.InitNodeList();
	result = LLDropNodeAttributeData (&nattr);
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}

HRESULT VNODE_AttributeAlter (CaNodeAttribute* pAttributeOld, CaNodeAttribute* pAttributeNew)
{
	int result = RES_ERR;
	HRESULT hErr = NOERROR;
	NODEATTRIBUTEDATAMIN nattr, noldattr;
	memset (&nattr,    0, sizeof(nattr));
	//
	// New:
	nattr.bPrivate = pAttributeNew->IsAttributePrivate();
	lstrcpy ((LPTSTR)nattr.NodeName,       (LPCTSTR)pAttributeNew->GetNodeName());
	lstrcpy ((LPTSTR)nattr.AttributeName,  (LPCTSTR)pAttributeNew->GetAttributeName());
	lstrcpy ((LPTSTR)nattr.AttributeValue, (LPCTSTR)pAttributeNew->GetAttributeValue());
	//
	// Old:
	noldattr.bPrivate = pAttributeOld->IsAttributePrivate();
	lstrcpy ((LPTSTR)noldattr.NodeName,       (LPCTSTR)pAttributeOld->GetNodeName());
	lstrcpy ((LPTSTR)noldattr.AttributeName,  (LPCTSTR)pAttributeOld->GetAttributeName());
	lstrcpy ((LPTSTR)noldattr.AttributeValue, (LPCTSTR)pAttributeOld->GetAttributeValue());

//	CaNodeDataAccess nodeAccess;
	result = LLAlterNodeAttributeData (&noldattr, &nattr);
	hErr = (result == RES_SUCCESS)? NOERROR: E_FAIL;
	return hErr;
}



// ****************************************************************************
// Virtual Node Installation (Add, Drop, Alter)
// ****************************************************************************

UINT IngresStartThreadProc(LPVOID pParam)
{
	CaIngresRunning* pInfo = (CaIngresRunning*)pParam;

	BOOL bOk;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	CString strCmd = pInfo->GetCmdStart();

	// Execute the command with a call to the CreateProcess API call.
	memset(&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = pInfo->IsShowWindow()? SW_SHOW: SW_HIDE;
	DWORD dwCreate = 0;
	bOk = CreateProcess(NULL, (LPTSTR)(LPCTSTR)strCmd, NULL, NULL, FALSE, dwCreate, NULL, NULL, &si, &pi);
	if (!bOk)
		return 0;
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	HANDLE  hEvent = pInfo->GetEventExit();
	SetEvent (hEvent);

	return 1;
}


CaNode* VNODE_Search (CTypedPtrList<CObList, CaNode*>& listNode, LPCTSTR lpszNode)
{
	POSITION pos = listNode.GetHeadPosition();
	while (pos != NULL)
	{
		CaNode* pObj = listNode.GetNext (pos);
		if (pObj && pObj->GetName().CompareNoCase(lpszNode) == 0)
			return pObj;
	}
	return NULL;
}

CaNode* VNODE_Search (CTypedPtrList<CObList, CaNode*>& listNode, CaNode* pNode)
{
	POSITION pos = listNode.GetHeadPosition();
	while (pos != NULL)
	{
		CaNode* pObj = listNode.GetNext (pos);
		if (pObj && pObj->Matched(pNode, MATCHED_ALL))
			return pObj;
	}
	return NULL;
}


BOOL INGRESII_IsRunning(){return ping_iigcn();}
//
// return:
//   0: not started
//   1: started
//   2: failed to started
long INGRESII_IsRunning(CaIngresRunning& ingresRunning)
{
	BOOL bStart = ping_iigcn();
	if (bStart)
		return 1;
	if (ingresRunning.GetRequestToStart())
	{
		CWaitCursor doWaitCursor;
		HWND hwnd = ingresRunning.GetCaller();
		CString strCmd = ingresRunning.GetCmdStart();
		CString strCaption = ingresRunning.GetBoxCaption();
		CString strRequestToStart = ingresRunning.GetRequestToStartText();
		int nAwnswer = ::MessageBox(hwnd, strRequestToStart, strCaption, MB_ICONQUESTION|MB_YESNO|MB_TASKMODAL);
		if (nAwnswer != IDYES)
			return 0;

		MSG message;
		DWORD dwWait = 0;
		UINT  nStep  = 5, nPit = 0;

		CWinThread* pTread = AfxBeginThread(IngresStartThreadProc, &ingresRunning);
		if (pTread)
		{
			while (1)
			{
				Sleep(nStep);
				nPit += nStep;
				if (nPit >= 50)
					dwWait = WaitForSingleObject(ingresRunning.GetEventExit(), 0);

				if (PeekMessage(&message, ingresRunning.GetCaller(), 0, 0, PM_REMOVE)) 
				{
					if (message.message == WM_PAINT)
					{
						TranslateMessage (&message);
						DispatchMessage  (&message);
					}
				}

				if (nPit >= 50)
				{
					if (dwWait == WAIT_OBJECT_0 || dwWait == WAIT_FAILED)
						break;
					nPit = 0;
				}
			}
		}
	}

	bStart = ping_iigcn();
	return bStart? 1: 2;
}

//
// This function has been moved from IVM
// and replaced in its body, the function 'NodeLLFillNodeLists' by 'NodeLLFillNodeLists_NoLocal'
// The Parameter BOOL bNoLocalInCache has been added, IVM calls this function with
// bNoLocalInCache = FALSE.
BOOL HasInstallationPassword(BOOL bNoLocalInCache)
{
	int iret = RES_SUCCESS;
	BOOL bResult = FALSE;

	iret=NodeLLInit();
	if (iret ==RES_SUCCESS) {
		iret= bNoLocalInCache? NodeLLFillNodeLists(): NodeLLFillNodeLists_NoLocal();
		if (iret==RES_SUCCESS)  {
			for (LPMONNODEDATA p1 = GetFirstLLNode();p1;p1=p1->pnext) {
				if (p1->MonNodeDta.bInstallPassword) {
					bResult = TRUE;
					break;
				}
			}
		}
		NodeLLTerminate();
	}
	if (iret!=RES_SUCCESS) {
		AfxMessageBox("Error in getting the \"installation password\" information");
		return FALSE;
	}
	return bResult;
}


STATUS INGRESII_NodeCheckConnection(LPCTSTR lpszNode)
{
	return NodeCheckConnection((LPUCHAR)(LPTSTR)lpszNode);
}


//
// Object: CaNetTraficInfo 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaNetTraficInfo, CObject, 1)
CaNetTraficInfo::CaNetTraficInfo()
{
	Cleanup();
}

CaNetTraficInfo::~CaNetTraficInfo()
{

}

void CaNetTraficInfo::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << GetObjectVersion();

		ar << m_tTimeStamp; 
		ar << m_nInboundMax;
		ar << m_nInbound;
		ar << m_nOutboundMax;
		ar << m_nOutbound;
		ar << m_nPacketIn;
		ar << m_nPacketOut;
		ar << m_nDataIn;
		ar << m_nDataOut;
	}
	else
	{
		UINT nVersion;
		ar >> nVersion;

		ar >> m_tTimeStamp;
		ar >> m_nInboundMax;
		ar >> m_nInbound;
		ar >> m_nOutboundMax;
		ar >> m_nOutbound;
		ar >> m_nPacketIn;
		ar >> m_nPacketOut;
		ar >> m_nDataIn;
		ar >> m_nDataOut;
	}
}

CaNetTraficInfo::CaNetTraficInfo(const CaNetTraficInfo& c)
{
	Copy(c);
}

CaNetTraficInfo CaNetTraficInfo::operator = (const CaNetTraficInfo& c)
{
	Copy(c);
	return *this;
}

void CaNetTraficInfo::Copy (const CaNetTraficInfo& c)
{
	m_tTimeStamp  = c.m_tTimeStamp;
	m_nInboundMax = c.m_nInboundMax ;
	m_nInbound    = c.m_nInbound    ;
	m_nOutboundMax= c.m_nOutboundMax;
	m_nOutbound   = c.m_nOutbound   ;
	m_nPacketIn   = c.m_nPacketIn   ;
	m_nPacketOut  = c.m_nPacketOut  ;
	m_nDataIn     = c.m_nDataIn     ;
	m_nDataOut    = c.m_nDataOut    ;

}

void CaNetTraficInfo::SetTimeStamp()
{
	m_tTimeStamp = CTime::GetCurrentTime();
}

void CaNetTraficInfo::Cleanup()
{
	SetTimeStamp();
	m_nInboundMax = 0;
	m_nInbound = 0;
	m_nOutboundMax = 0;
	m_nOutbound = 0;
	m_nPacketIn = 0;
	m_nPacketOut = 0;
	m_nDataIn = 0;
	m_nDataOut = 0;
}



BOOL INGRESII_QueryNetTrafic (LPCTSTR lpszNode, LPCTSTR lpszListenAddress, CaNetTraficInfo& infoNetTrafic)
{
	infoNetTrafic.SetTimeStamp();
	NETTRAFIC netTrafic;
	memset (&netTrafic, 0, sizeof(netTrafic));

	BOOL bOK = NetQueryTraficInfo((LPTSTR)lpszNode, (LPTSTR)lpszListenAddress, &netTrafic);
	if (bOK)
	{
		infoNetTrafic.SetTimeStamp();
		infoNetTrafic.SetInboundMax(netTrafic.nInboundMax);
		infoNetTrafic.SetInbound(netTrafic.nInbound);
		infoNetTrafic.SetOutboundMax(netTrafic.nOutboundMax);
		infoNetTrafic.SetOutbound(netTrafic.nOutbound);
		infoNetTrafic.SetPacketIn(netTrafic.nPacketIn);
		infoNetTrafic.SetPacketOut(netTrafic.nPacketOut);
		infoNetTrafic.SetDataIn(netTrafic.nDataIn);
		infoNetTrafic.SetDataOut(netTrafic.nDataOut);
	}

	return bOK;
}

BOOL INGRESII_QueryNetLocalVnode ( CString &rcsLocalVnode )
{
	NETLOCALVNODENAME NewLocalVnode;
	memset (&NewLocalVnode, 0, sizeof(NETLOCALVNODENAME));

	BOOL bOK = NetQueryLocalVnode( &NewLocalVnode );
	if (bOK)
	{
		rcsLocalVnode = NewLocalVnode.tcLocalVnode;
	}
	return bOK;
}

