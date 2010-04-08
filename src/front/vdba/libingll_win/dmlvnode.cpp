/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dmlvnode.h: interface for the CaNode, CaFolderCaNode class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Node object and folder
**
** History:
**
** 11-Feb-1998 (uk$so01)
**    Created
** 27-Jun-2003 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
**/

#include "stdafx.h"
#include "dmlvnode.h"
#include "ingobdml.h"
#include "constdef.h"
#include <winsock2.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Object: Node Attribute
// **********************
IMPLEMENT_SERIAL (CaNodeAttribute, CaDBObject, 1)
CaNodeAttribute::CaNodeAttribute(): CaDBObject(_T(""), _T(""), FALSE)
{
	SetObjectID(OBT_VNODE_ATTRIBUTE);
	m_strAttributeValue = _T("");
}

CaNodeAttribute::CaNodeAttribute(LPCTSTR lpszNode, LPCTSTR lpszAttrName, LPCTSTR lpszAttrValue, BOOL bPrivate):
    CaDBObject(lpszNode, lpszAttrName, bPrivate)
{
	SetObjectID(OBT_VNODE_ATTRIBUTE);
	m_strAttributeValue = lpszAttrValue;
}

CaNodeAttribute::CaNodeAttribute(const CaNodeAttribute& c)
{
	Copy (c);
}

CaNodeAttribute CaNodeAttribute::operator = (const CaNodeAttribute& c)
{
	Copy (c);
	return *this;
}

void CaNodeAttribute::Copy (const CaNodeAttribute& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	// 
	// Extra copy here:
	m_strAttributeValue = c.m_strAttributeValue;
}

void CaNodeAttribute::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_strAttributeValue;
	}
	else
	{
		ar >> m_strAttributeValue;
	}
}

BOOL CaNodeAttribute::Matched (CaNodeAttribute* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME)
	{
		return (pObj->GetAttributeName().CompareNoCase (GetAttributeName()) == 0);
	}
	else
	{
		BOOL bOk = FALSE;
		if (pObj->GetNodeName().CompareNoCase (GetNodeName()) != 0)
			return FALSE;
		if (pObj->GetAttributeName().CompareNoCase (GetAttributeName()) != 0)
			return FALSE;
		if (pObj->GetAttributeValue().CompareNoCase (GetAttributeValue()) != 0)
			return FALSE;
		if (pObj->IsAttributePrivate() != IsAttributePrivate())
			return FALSE;

		return TRUE;
	}
	//
	// Need the implementation ?
	ASSERT (FALSE);
	return FALSE;
}



//
// Object: Node Login
//*******************
IMPLEMENT_SERIAL (CaNodeLogin, CaDBObject, 1)
CaNodeLogin::CaNodeLogin() : CaDBObject(_T(""), _T(""), FALSE)
{
	SetObjectID(OBT_VNODE_LOGINPSW);
	m_strPassword = _T("");
#if defined(EDBC)
	m_bSaveLogin = FALSE;
#endif
}

CaNodeLogin::CaNodeLogin(LPCTSTR lpszNode, LPCTSTR lpszLogin, LPCTSTR lpszPassword, BOOL bPrivate):
    CaDBObject(lpszNode, lpszLogin, bPrivate)
{
	SetObjectID(OBT_VNODE_LOGINPSW);
	m_strPassword = lpszPassword;
#if defined(EDBC)
	m_bSaveLogin = FALSE;
#endif
}

CaNodeLogin::CaNodeLogin(const CaNodeLogin& c)
{
	Copy (c);
}

CaNodeLogin CaNodeLogin::operator = (const CaNodeLogin& c)
{
	Copy (c);
	return *this;
}

void CaNodeLogin::Copy (const CaNodeLogin& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	// 
	// Extra copy here:
	m_strPassword = c.m_strPassword;
#if defined(EDBC)
	m_bSaveLogin  = c.m_bSaveLogin;
#endif
}


void CaNodeLogin::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	//
	// Own properties:
	if (ar.IsStoring())
	{
		ar << m_strPassword;
	}
	else
	{
		ar >> m_strPassword;
	}
	
	if (!IstSerializeAll())
		return;
	//
	// Serialize the extra data manage by this object:
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

BOOL CaNodeLogin::Matched (CaNodeLogin* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME)
	{
		return (pObj->GetLogin().CompareNoCase (GetLogin()) == 0);
	}
	else
	{
		BOOL bOk = FALSE;
		if (pObj->GetNodeName().CompareNoCase (GetNodeName()) != 0)
			return FALSE;
		if (pObj->GetLogin().CompareNoCase (GetLogin()) != 0)
			return FALSE;
		if (pObj->IsLoginPrivate() != IsLoginPrivate())
			return FALSE;

		return TRUE;
	}
	//
	// Need the implementation ?
	ASSERT (FALSE);
	return FALSE;
}

//
// Object: Node Connection Data
// ****************************
IMPLEMENT_SERIAL (CaNodeConnectData, CaDBObject, 1)
CaNodeConnectData::CaNodeConnectData() :CaDBObject(_T(""), _T(""), FALSE)
{
	SetObjectID(OBT_VNODE_CONNECTIONDATA);
	m_strProtocol        = _T("");
	m_strListenAddress   = _T("");
}

CaNodeConnectData::CaNodeConnectData(LPCTSTR lpszNode, LPCTSTR lpszIP, LPCTSTR lpszProtocol, LPCTSTR lpszListen, BOOL bPrivate):
    CaDBObject(lpszNode, lpszIP, bPrivate)
{
	SetObjectID(OBT_VNODE_CONNECTIONDATA);
	m_strProtocol        = lpszProtocol;
	m_strListenAddress   = lpszListen;
}

CaNodeConnectData::CaNodeConnectData(const CaNodeConnectData& c)
{
	Copy (c);
}

CaNodeConnectData CaNodeConnectData::operator = (const CaNodeConnectData& c)
{
	Copy (c);
	return *this;
}

void CaNodeConnectData::Copy (const CaNodeConnectData& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	// 
	// Extra copy here:
	m_strProtocol        = c.m_strProtocol;
	m_strListenAddress   = c.m_strListenAddress;
}


void CaNodeConnectData::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	//
	// Own properties:
	if (ar.IsStoring())
	{
		ar << m_strProtocol;
		ar << m_strListenAddress;
	}
	else
	{
		ar >> m_strProtocol;
		ar >> m_strListenAddress;
	}
	
	if (!IstSerializeAll())
		return;
	//
	// Serialize the extra data manage by this object:
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

BOOL CaNodeConnectData::Matched (CaNodeConnectData* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME)
	{
		return (pObj->GetName().CompareNoCase (GetName()) == 0);
	}
	else
	{
		BOOL bOk = FALSE;
		if (pObj->GetNodeName().CompareNoCase (GetNodeName()) != 0)
			return FALSE;
		if (pObj->GetName().CompareNoCase (GetName()) != 0) // IP
			return FALSE;
		if (pObj->GetProtocol().CompareNoCase (GetProtocol()) != 0)
			return FALSE;
		if (pObj->GetListenAddress().CompareNoCase (GetListenAddress()) != 0)
			return FALSE;
		if (pObj->IsConnectionPrivate() != IsConnectionPrivate())
			return FALSE;

		return TRUE;
	}
	//
	// Need the implementation ?
	ASSERT (FALSE);
	return FALSE;
}


//
// Object: Node Server
// *******************
IMPLEMENT_SERIAL (CaNodeServer, CaDBObject, 1)
CaNodeServer::CaNodeServer() : CaDBObject(_T(""), _T(""), FALSE)
{
	SetObjectID(OBT_VNODE_SERVERCLASS);
}

CaNodeServer::CaNodeServer(LPCTSTR lpszNode, LPCTSTR lpszServer, BOOL bLocal):
    CaDBObject(lpszNode, lpszServer, bLocal)
{
	SetObjectID(OBT_VNODE_SERVERCLASS);
}

CaNodeServer::CaNodeServer(const CaNodeServer& c)
{
	Copy (c);
}

CaNodeServer CaNodeServer::operator = (const CaNodeServer& c)
{
	Copy (c);
	return *this;
}

void CaNodeServer::Copy (const CaNodeServer& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	// 
	// Extra copy here:
}


void CaNodeServer::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	//
	// Own properties:
	if (ar.IsStoring())
	{
	}
	else
	{
	}
	
	if (!IstSerializeAll())
		return;
	//
	// Serialize the extra data manage by this object:
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

BOOL CaNodeServer::Matched (CaNodeServer* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME)
	{
		return (pObj->GetName().CompareNoCase (GetName()) == 0);
	}
	else
	{
		BOOL bOk = FALSE;
		if (pObj->GetNodeName().CompareNoCase (GetNodeName()) != 0)
			return FALSE;
		if (pObj->GetName().CompareNoCase (GetName()) != 0) // Server
			return FALSE;
		if (pObj-> IsLocalNode() != IsLocalNode())
			return FALSE;

		return TRUE;
	}
	//
	// Need the implementation ?
	ASSERT (FALSE);
	return FALSE;
}


//
// Object: Node 
// ************
IMPLEMENT_SERIAL (CaNode, CaDBObject, 1)
CaNode::CaNode(): CaDBObject(_T(""), _T(""))
{
	Initialize();
}

CaNode::CaNode(LPCTSTR lpszVirtualNode):CaDBObject(lpszVirtualNode, _T(""))
{
	Initialize();
}

CaNode::CaNode(const CaNode& c)
{
	Copy (c);
}

CaNode CaNode::operator = (const CaNode& c)
{
	Copy (c);
	return *this;
}

void CaNode::Copy (const CaNode& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	
	CaDBObject::Copy(c);
	// 
	// Extra copy here:
	m_strLogin           = c.m_strLogin;
	m_strPassword        = c.m_strPassword;
	m_strIPAddress       = c.m_strIPAddress;
	m_strProtocol        = c.m_strProtocol;
	m_strListenAddress   = c.m_strListenAddress;
	m_bConnectionPrivate = c.m_bConnectionPrivate;
	m_bLoginPrivate      = c.m_bLoginPrivate;
	m_nClassify          = c.m_nClassify;
#if defined(EDBC)
	m_bSaveLogin         = c.m_bSaveLogin;
#endif
}


void CaNode::Initialize()
{
	SetObjectID(OBT_VIRTNODE);
	//
	// Own properties:
	m_strLogin = _T("");
	m_strPassword = _T("");
	m_strIPAddress = _T("");
	m_strProtocol = _T("");
	m_strListenAddress = _T("");
	m_bConnectionPrivate = FALSE;
	m_bLoginPrivate = TRUE;

	m_nClassify = 0;
	m_lNbConnectData = 0;
	m_lNbLoginData = 0;
#if defined(EDBC)
	m_bSaveLogin = FALSE;
#endif
}



CaNode::~CaNode()
{
}


CString CaNode::GetIPAddress()
{
	if (IsLocalNode())
	{
		/*
		class CaSockInit
		{
		public:
			CaSockInit()
			{
				WSADATA  winsock_data;
				m_bOk = WSAStartup(MAKEWORD(2,0), & winsock_data) == 0;
			}
			~CaSockInit() {WSACleanup();}

			BOOL m_bOk;
		};

		CString strLocalHostName = _T("");
		CaSockInit wsainit;
		if (!wsainit.m_bOk)
			return _T("");

		//
		// Get Local Host Name:
		char hostname[256];
		int  nLen = 255;
		if (gethostname(hostname, nLen) == 0)
		{
			strLocalHostName = hostname;
			return strLocalHostName;
		}
		*/
		return GetLocalHostName();
	}

	return m_strIPAddress;
}


void CaNode::Serialize (CArchive& ar)
{
	CaDBObject::Serialize (ar);
	//
	// Own properties:
	if (ar.IsStoring())
	{
		ar << m_strLogin;
		ar << m_strPassword;
		ar << m_strIPAddress;
		ar << m_strProtocol;
		ar << m_strListenAddress;
		ar << m_bConnectionPrivate;
		ar << m_bLoginPrivate;
		ar << m_nClassify;
	}
	else
	{
		ar >> m_strLogin;
		ar >> m_strPassword;
		ar >> m_strIPAddress;
		ar >> m_strProtocol;
		ar >> m_strListenAddress;
		ar >> m_bConnectionPrivate;
		ar >> m_bLoginPrivate;
		ar >> m_nClassify;
	}
	if (!IstSerializeAll())
		return;

	//
	// Serialize the extra data manage by user:
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

void CaNode::LocalNode()
{
	m_nClassify |= NODE_LOCAL;
}

BOOL CaNode::IsLocalNode()
{
	return (m_nClassify & NODE_LOCAL) != 0;
}

BOOL CaNode::Matched (CaNode* pObj, MatchObjectFlag nFlag)
{
	if (nFlag == MATCHED_NAME)
	{
		return (pObj->GetName().CompareNoCase (GetName()) == 0);
	}
	else
	{
		if (pObj->GetName().CompareNoCase (GetName()) != 0)
			return FALSE;
		if (pObj->GetLogin().CompareNoCase (m_strLogin) != 0) 
			return FALSE;
		if (pObj->GetIPAddress().CompareNoCase (m_strIPAddress) != 0) 
			return FALSE;
		if (pObj->GetProtocol().CompareNoCase (m_strProtocol) != 0) 
			return FALSE;
		if (pObj->GetListenAddress().CompareNoCase (m_strListenAddress) != 0) 
			return FALSE;

		if (pObj->IsConnectionPrivate() != m_bConnectionPrivate) 
			return FALSE;
		if (pObj->IsLoginPrivate() != m_bLoginPrivate) 
			return FALSE;

		return TRUE;
	}
	//
	// Need the implementation ?
	ASSERT (FALSE);
	return FALSE;
}

