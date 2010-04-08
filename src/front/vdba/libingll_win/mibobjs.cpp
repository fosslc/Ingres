/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : mibobjs.cpp: implementation file
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation of MIB Objects.
**
** History:
**
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
**    Created.
** 13-Jan-2003 (schph01)
**    BUG 99242,  cleanup for DBCS compliance
**/

#include "stdafx.h"
#include "mibobjs.h"
#include "vnodedml.h"
#include "ingobdml.h"

#if defined (_USE_IMA_SESSIONxTABLE)
#include "sqlselec.h"
#include "lsselres.h"
#include "sessimgr.h"
#endif


extern "C"
{
#include "nodes.h"
//
// This is a call back implemented in nodes.c. The pData is the global structure 'g_mibRequest'.
void CALLBACK userFetchMIBObject (void* pData, LPCTSTR lpszClass, LPCTSTR lpszInstance, LPCTSTR lpszValue);
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CaSessionConnected::CaSessionConnected():CaDBObject()
{
	m_strPID           = _T("");
	m_strUser          = _T("");
	m_strDescription   = _T("");
	m_strDatabase      = _T("");
	m_strConnectString = _T("");
	m_strApplication   = _T("");
	m_strUserReal      = _T("");
	m_strUserEffective = _T("");
}

BOOL CaSessionConnected::IsSystemInternal()
{
	if (m_strUserReal.IsEmpty())
		return TRUE;
	int nLen = m_strUserReal.GetLength();
	if (m_strUserReal[0] == _T('<') && m_strUserReal[nLen-1] == _T('>')) /* DBCS checked */
		return TRUE;
	return FALSE;
}

BOOL CaSessionConnected::IsVisualToolInternal()
{
	if (m_strDescription.IsEmpty())
		return FALSE;
	if (m_strDescription.CompareNoCase("(unknown)") == 0)
		return FALSE;
	return TRUE;
}

CaSessionConnected* CaSessionConnected::Search (CTypedPtrList<CObList, CaSessionConnected*>* pList, LPCTSTR lpszKey, int nKey)
{
	if (!pList)
		return NULL;
	POSITION pos = pList->GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = _T("");
		CaSessionConnected* pObj = pList->GetNext(pos);

		switch (nKey)
		{
		case 0:
			strItem = pObj->GetID();
			break;
		case 1:
			strItem = pObj->GetHost();
			break;
		default:
			ASSERT(FALSE);
			break;
		}

		if (!strItem.IsEmpty())
		{
			if (strItem.CompareNoCase(lpszKey) == 0)
				return pObj;
		}
	}
	return NULL;
}


//
// Get the list of [remote] nodes to which there are connected session
// from the calling [current] installation:
BOOL QueryActiveOutboundNode(CTypedPtrList<CObList,CaNode*>& listVNode)
{
	BOOL bOK = FALSE;
	TCHAR tchszTarget[128];

	HRESULT hErr = NOERROR;
	CTypedPtrList<CObList,CaDBObject*> ln;
	try
	{
		hErr = VNODE_QueryNode (ln);
	}
	catch(...)
	{
		hErr = E_FAIL;
	}
	if (hErr != NOERROR)
	{
		TRACE0("ERROR: QueryActiveOutboundNode, calls VNODE_QueryNode -> FAIL\n");
		ReleaseMutexGCA_Initiate_Terminate();
		return FALSE;
	}

	MIBREQUEST mibRequest;
	memset (&mibRequest, 0, sizeof(mibRequest));
	_tcscpy(tchszTarget, _T("/comsvr"));
	mibRequest.pfnUserFetch = userFetchMIBObject;
	mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.gcf.gcc.conn.inbound"));
	mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.gcf.gcc.conn.trg_addr.node"));
	mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.gcf.gcc.conn.trg_addr.port"));
	mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.gcf.gcc.conn.trg_addr.protocol"));
	mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.gcf.gcc.conn.userid"));

	bOK = QueryMIBObjects(&mibRequest, tchszTarget, 0x61); // GCM_GETNEXT = 0x61
	LPMIBOBJECT lsMb = mibRequest.listMIB;
	LPMIBOBJECT pAddr     = MibObject_Search(lsMb, _T("exp.gcf.gcc.conn.trg_addr.node"));
	LPMIBOBJECT pPort     = MibObject_Search(lsMb, _T("exp.gcf.gcc.conn.trg_addr.port"));
	LPMIBOBJECT pProtocol = MibObject_Search(lsMb, _T("exp.gcf.gcc.conn.trg_addr.protocol"));
	LPMIBOBJECT pUserId   = MibObject_Search(lsMb, _T("exp.gcf.gcc.conn.userid"));

	if (pAddr && pPort && pProtocol && pUserId)
	{
		LPLISTTCHAR lsAddr = pAddr->listValues;
		LPLISTTCHAR lsPort = pPort->listValues;
		LPLISTTCHAR lsProt = pProtocol->listValues;
		LPLISTTCHAR lsUser = pUserId->listValues;

		while (lsAddr && lsPort && lsProt && lsUser)
		{
			CaNode node;
			node.SetName(_T("???"));

			node.SetIPAddress(lsAddr->lptchItem);
			node.SetListenAddress(lsPort->lptchItem);
			node.SetProtocol(lsProt->lptchItem);
			node.SetLogin(lsUser->lptchItem);
			if (!VNODE_Search(listVNode, &node))
				listVNode.AddTail(new CaNode (node));

			lsAddr = lsAddr->next;
			lsPort = lsPort->next;
			lsProt = lsProt->next;
			lsUser = lsUser->next;
		}
		//
		// Match Nodes Characteristics:
		POSITION pos = ln.GetHeadPosition();
		while (pos != NULL)
		{
			CaNode* pNode = (CaNode*)ln.GetNext(pos);
			CString strName = pNode->GetName(); // Save the real name
			pNode->SetName(_T("???"));          // The name is the same as the one in MIB Object
			CaNode* pExist = VNODE_Search(listVNode, pNode);
			if (pExist) 
				pExist->SetName(strName);
			pNode->SetName(strName);
		}
	}

	while (!ln.IsEmpty())
		delete ln.RemoveHead();
	mibRequest.listMIB = MibObject_Done(mibRequest.listMIB);

	return TRUE;
}


BOOL QueryRemoteConnectingNodeName(CStringList& listNodeName)
{
	CTypedPtrList<CObList,CaNode*> listVNode;

	BOOL bOK = QueryActiveOutboundNode(listVNode);
	if (!bOK)
	{
		TRACE0("ERROR: QueryRemoteConnectingNodeName, calls QueryActiveOutboundNode -> FALSE\n");
		return bOK;
	}

	POSITION pos = listVNode.GetHeadPosition();
	while (pos != NULL)
	{
		CaNode* pNode = listVNode.GetNext(pos);
		if (pNode->GetName().CompareNoCase(_T("???")) != 0)
		{
			if (!listNodeName.Find(pNode->GetName()))
				listNodeName.AddTail(pNode->GetName());
		}
	}
	while (!listVNode.IsEmpty())
		delete listVNode.RemoveHead();
	return TRUE;
}

extern "C" void CALLBACK userFetchSession (void* pData, LPCTSTR lpszClass, LPCTSTR lpszInstance, LPCTSTR lpszValue)
{
	MIBREQUEST* pRequest = (MIBREQUEST*)pData;
	ASSERT (pRequest);
	if (pRequest && pRequest->lUserData)
	{
		CString strValue = lpszValue;
		strValue.TrimLeft();
		strValue.TrimRight();
		CTypedPtrList<CObList, CaSessionConnected*>* plistSession = (CTypedPtrList<CObList, CaSessionConnected*>*)pRequest->lUserData;
		if ( _tcsicmp (lpszClass, _T("exp.scf.scs.scb_index")) == 0 &&  strValue.GetLength() > 0)
		{
			CaSessionConnected* pObject = new CaSessionConnected();
			pObject->SetID(strValue);
			plistSession->AddTail(pObject);
		}
		else
		{
			CaSessionConnected* pObj = CaSessionConnected::Search (plistSession, lpszInstance, 0);
			if (pObj)
			{
				if(_tcsicmp (lpszClass, _T("exp.scf.scs.scb_database")) == 0)
					pObj->SetDatabase(strValue);
				else
				if (_tcsicmp (lpszClass, _T("exp.scf.scs.scb_description")) == 0)
					pObj->SetDescription(strValue);
				else
				if (_tcsicmp (lpszClass, _T("exp.scf.scs.scb_client_connect")) == 0)
					pObj->SetConnectString(strValue);
				else
				if (_tcsicmp (lpszClass, _T("exp.scf.scs.scb_client_host")) == 0)
					pObj->SetHost(strValue);
				else
				if (_tcsicmp (lpszClass, _T("exp.scf.scs.scb_index")) == 0)
					pObj->SetID(strValue);
				else
				if (_tcsicmp (lpszClass, _T("exp.scf.scs.scb_client_pid")) == 0)
					pObj->SetPID(strValue);
				else
				if (_tcsicmp (lpszClass, _T("exp.scf.scs.scb_client_user")) == 0)
					pObj->SetUser(strValue);
				else
				if (_tcsicmp (lpszClass, _T("exp.scf.scs.scb_euser")) == 0)
					pObj->SetEUser(strValue);
				else
				if (_tcsicmp (lpszClass, _T("exp.scf.scs.scb_ruser")) == 0)
					pObj->SetRUser(strValue);
			}
		}
	}
}

static HANDLE g_hMutexQuerySession = NULL;
BOOL QuerySession (CaQuerySessionInfo* pInfo, CTypedPtrList<CObList, CaSessionConnected*>& listSession)
{
	class CaQuerySessionEnter
	{
	public:
		CaQuerySessionEnter()
		{
			m_bStatus = FALSE;
			if (!g_hMutexQuerySession)
				g_hMutexQuerySession = CreateMutex (NULL, FALSE, NULL);
			DWORD dwWait = WaitForSingleObject(g_hMutexQuerySession, 5*60*1000);
			switch(dwWait)
			{
			case WAIT_OBJECT_0:
				m_bStatus = TRUE;
				break;
			default:
				break;
			}
		}
		~CaQuerySessionEnter()
		{
			if (g_hMutexQuerySession && m_bStatus)
				ReleaseMutex(g_hMutexQuerySession);
		}
		BOOL GetStatus(){return m_bStatus;}

	protected:
		BOOL m_bStatus;
	};

	CaQuerySessionEnter enter;
	if (!enter.GetStatus())
		return FALSE;
TRACE1("------------------->QuerySession begin: %s\n", pInfo->GetNode()->GetName());
	BOOL bOK = FALSE;
	CStringList listAddress;
	//
	// 1) Query ther server classes of a given node. 
	//    Only the ingres dbms server classes are taken into account.
	CTypedPtrList<CObList, CaDBObject*> listServerClass;
	HRESULT hErr = VNODE_QueryServer (pInfo->GetNode(), listServerClass);
	if (hErr != NOERROR)
		return FALSE;

	//
	// 2) For each server class, query the addresses. [the address is in a format:
	//    <installationID>\<ServerClass>\<hexa-number>. EX: II\INGRES\1cf
	MIBREQUEST mibRequest;
	memset (&mibRequest, 0, sizeof(mibRequest));
	CString strTarget = _T("");
	CString strVNode = pInfo->GetNode()->GetName();
	if (!strVNode.IsEmpty() && strVNode.CompareNoCase(_T("(local)")) != 0)
	{
		strTarget = strVNode;
		strTarget+= _T("::");
	}
	strTarget += _T("/iinmsvr");
	mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.gcf.gcn.server.address"));
	bOK = QueryMIBObjects(&mibRequest, (LPCTSTR)strTarget, 0x61); // GCM_GETNEXT = 0x61
	LPMIBOBJECT lsMb = mibRequest.listMIB;
	LPLISTTCHAR lsAddr = lsMb->listValues;

	while (lsAddr)
	{
		CString strAddr = lsAddr->lptchItem;
		POSITION pos = listServerClass.GetHeadPosition();
		while (pos != NULL)
		{
			CaNodeServer* pServer = (CaNodeServer*)listServerClass.GetNext(pos);
			if (strAddr.Find(pServer->GetName()) != -1)
			{
				listAddress.AddTail(strAddr);
				break;
			}
		}

		lsAddr = lsAddr->next;
	}
	lsMb = MibObject_Done(lsMb);
	while (!listServerClass.IsEmpty())
		delete listServerClass.RemoveHead();

	//
	// 3) For each address, use it as target to get the MIB objects. The target is:
	//    /@<Address>. Ex: /@II\INGRES\1cf
	const int numFild = 9;
	int i;
	TCHAR tchFields[numFild][48] = 
	{
		_T("exp.scf.scs.scb_index"),
		_T("exp.scf.scs.scb_description"),
		_T("exp.scf.scs.scb_client_host"),
		_T("exp.scf.scs.scb_client_user"),
		_T("exp.scf.scs.scb_database"),
		_T("exp.scf.scs.scb_client_pid"),
		_T("exp.scf.scs.scb_client_connect"),
		_T("exp.scf.scs.scb_euser"),
		_T("exp.scf.scs.scb_ruser")
	};
	POSITION pos = listAddress.GetHeadPosition();
	while (pos != NULL)
	{
		CString strAddress = listAddress.GetNext(pos);

		MIBREQUEST mibRequest;
		memset (&mibRequest, 0, sizeof(mibRequest));
		strTarget = _T("");
		if (!strVNode.IsEmpty() && strVNode.CompareNoCase(_T("(local)")) != 0)
		{
			strTarget = strVNode;
			strTarget+= _T("::");
		}
		strTarget+= _T("/@");
		strTarget+= strAddress; 

		//
		// Fail to use the GROUP of select in the GCF call.
		// EXAMPLE of the following call will fail
		//      mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.scf.scs.scb_description"));
		//      mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.scf.scs.scb_client_connect"));
		//      mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.scf.scs.scb_client_host"));
		//      mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.scf.scs.scb_client_pid"));
		//      mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.scf.scs.scb_client_user"));
		//      mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.scf.scs.scb_euser"));
		//      mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, _T("exp.scf.scs.scb_ruser"));
		//      bOK = QueryMIBObjects(&mibRequest, strTarget, 0x61); // GCM_GETNEXT = 0x61
		// Instead use the call back userFetchSession and select one by one of the exp.scf.scs.scb_???:
		for (i=0; i<numFild; i++)
		{
			memset (&mibRequest, 0, sizeof(mibRequest));
			mibRequest.lUserData = (LPARAM)&listSession;
			mibRequest.pfnUserFetch = userFetchSession;
			mibRequest.listMIB = MibObject_Add (mibRequest.listMIB, tchFields[i]);

			bOK = QueryMIBObjects(&mibRequest, strTarget, 0x61); // GCM_GETNEXT = 0x61
			MibObject_Done(mibRequest.listMIB);
			if (!bOK)
			{
				while (!listSession.IsEmpty())
					delete listSession.RemoveHead();
				return FALSE;
			}
		}
	}

TRACE1("------------------->QuerySession end: %s\n", pInfo->GetNode()->GetName());
	return TRUE;
}

extern "C" void Tx3(LPCTSTR lpsz1, LPCTSTR lpsz2, LPCTSTR lpsz3)
{
	CString strMsg;
	strMsg.Format(_T("%s\t%s\t%s\n"), lpsz1, lpsz2, lpsz3);
	TRACE0(strMsg);
}
