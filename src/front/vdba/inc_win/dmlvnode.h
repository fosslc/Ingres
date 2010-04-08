/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlvnode.h: interface for the CaNode and CaNodeXxx class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : CaNode, CaNodeXxx object
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


#if !defined (DMLVNODE_HEADER)
#define DMLVNODE_HEADER
#include "dmlbase.h"

//
// Object: Node Attribute
// **********************
class CaNodeAttribute: public CaDBObject
{
	DECLARE_SERIAL (CaNodeAttribute)
public:
	CaNodeAttribute();
	CaNodeAttribute(LPCTSTR lpszNode, LPCTSTR lpszAttrName, LPCTSTR lpszAttrValue, BOOL bPrivate);
	CaNodeAttribute(const CaNodeAttribute& c);
	CaNodeAttribute operator = (const CaNodeAttribute& c);
	virtual ~CaNodeAttribute(){};
	virtual void Serialize (CArchive& ar);


	BOOL Matched (CaNodeAttribute* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	BOOL IsAttributePrivate(){return IsSystem();}
	void AttributePrivate (BOOL bSet){SetSystem(bSet);}

	void SetNodeName (LPCTSTR lpszStr){SetItem(lpszStr);}       // Node Name
	void SetAttributeName(LPCTSTR lpszStr){SetOwner(lpszStr);}  // Attribute Name
	void SetAttributeValue(LPCTSTR lpszStr){m_strAttributeValue = lpszStr;}

	CString GetNodeName() {return m_strItem;}
	CString GetAttributeName(){return GetOwner();}
	CString GetAttributeValue(){return m_strAttributeValue;}

	CString GetName(){return GetAttributeName();} // Attribute Name
	CString GetItem(){return GetAttributeName();} // Attribute Name

protected:
	void Copy(const CaNodeAttribute& c);

	CString m_strAttributeValue;

};


//
// Object: Node Login
// ******************
class CaNodeLogin: public CaDBObject
{
	DECLARE_SERIAL (CaNodeLogin)
public:
	CaNodeLogin();
	CaNodeLogin(LPCTSTR lpszNode, LPCTSTR lpszLogin, LPCTSTR lpszPassword, BOOL bPrivate);
	CaNodeLogin (const CaNodeLogin& c);
	CaNodeLogin operator = (const CaNodeLogin& c);

	virtual ~CaNodeLogin(){};
	virtual void Serialize (CArchive& ar);

	BOOL Matched (CaNodeLogin* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	BOOL IsLoginPrivate(){return IsSystem();}
	void LoginPrivate (BOOL bSet){SetSystem(bSet);}

	void SetNodeName (LPCTSTR lpszStr){SetItem(lpszStr);}       // Node Name
	void SetLogin(LPCTSTR lpszStr){SetOwner(lpszStr);}          // Login
	void SetPassword(LPCTSTR lpszStr){m_strPassword = lpszStr;}
	CString GetName(){return GetLogin();}  // Login Name
	CString GetItem(){return GetLogin();}  // Login Name

	CString GetNodeName() {return m_strItem;}
	CString GetLogin(){return GetOwner();}
	CString GetPassword(){return m_strPassword;}
#if defined (EDBC)
	void SetSaveLogin(BOOL bSet){m_bSaveLogin = bSet;}
	BOOL GetSaveLogin(){return m_bSaveLogin;}
#endif

protected:
	void Copy (const CaNodeLogin& c);

	CString m_strPassword;
#if defined (EDBC)
	BOOL m_bSaveLogin;
#endif

};


//
// Object: Node Connection Data
// ****************************
class CaNodeConnectData: public CaDBObject
{
	DECLARE_SERIAL (CaNodeConnectData)
public:
	CaNodeConnectData();
	CaNodeConnectData(LPCTSTR lpszNode, LPCTSTR lpszIP, LPCTSTR lpszProtocol, LPCTSTR lpszListen, BOOL bPrivate);
	CaNodeConnectData (const CaNodeConnectData& c);
	CaNodeConnectData operator = (const CaNodeConnectData& c);

	virtual ~CaNodeConnectData(){};
	virtual void Serialize (CArchive& ar);

	BOOL Matched (CaNodeConnectData* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	BOOL IsConnectionPrivate(){return IsSystem();}
	void ConnectionPrivate (BOOL bSet){SetSystem(bSet);}

	void SetNodeName (LPCTSTR lpszStr){SetItem(lpszStr);}       // Node Name
	void SetIPAddress(LPCTSTR lpszStr){SetOwner(lpszStr);}      // Remote Address
	void SetProtocol (LPCTSTR lpszStr){m_strProtocol= lpszStr;}
	void SetListenAddress(LPCTSTR lpszStr){m_strListenAddress= lpszStr;}

	CString GetName(){return GetIPAddress();} // Remote address
	CString GetItem(){return GetIPAddress();} // Remote address

	CString GetNodeName() {return m_strItem;}
	CString GetIPAddress(){return GetOwner();}
	CString GetProtocol(){return m_strProtocol;}
	CString GetListenAddress(){return m_strListenAddress;}

protected:
	void Copy (const CaNodeConnectData& c);

	CString m_strProtocol;     // Protocol
	CString m_strListenAddress;// Listen Address (port)

};


//
// Object: Node Server
// *******************
class CaNodeServer: public CaDBObject
{
	DECLARE_SERIAL (CaNodeServer)
public:
	CaNodeServer();
	CaNodeServer(LPCTSTR lpszNode, LPCTSTR lpszServer, BOOL bLocal);
	CaNodeServer (const CaNodeServer& c);
	CaNodeServer operator = (const CaNodeServer& c);
	virtual ~CaNodeServer(){};
	virtual void Serialize (CArchive& ar);

	BOOL Matched (CaNodeServer* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	void SetNodeName(LPCTSTR lpszStr){SetItem(lpszStr);} // Node Name
	void SetName(LPCTSTR lpszStr){SetOwner(lpszStr);}    // Server Class
	CString GetNodeName(){return GetItem();}
	CString GetServerName(){return GetOwner();}

	CString GetName(){return GetServerName();}  // Server Name
	CString GetItem(){return GetServerName();}  // Server Name

	BOOL IsLocalNode(){return IsSystem();}
	void LocalNode (BOOL bSet){SetSystem(bSet);}

protected:
	void Copy(const CaNodeServer& c);

};



//
// Object: Node 
// ************
class CaNode: public CaDBObject
{
	DECLARE_SERIAL (CaNode)
public:
	CaNode();
	CaNode(LPCTSTR lpszVirtualNode);
	CaNode(const CaNode& c);
	CaNode operator = (const CaNode& c);
	virtual ~CaNode();

	void Initialize();
	virtual void Serialize (CArchive& ar);

	BOOL Matched (CaNode* pObj, MatchObjectFlag nFlag = MATCHED_NAME);
	void LocalNode();
	void SetNodeName(LPCTSTR lpszName){SetItem(lpszName);}
	void SetName(LPCTSTR lpszName){SetItem(lpszName);}
	void SetLogin(LPCTSTR lpszStr){m_strLogin = lpszStr;}
	void SetPassword(LPCTSTR lpszStr){m_strPassword = lpszStr;}
	void SetIPAddress(LPCTSTR lpszStr){m_strIPAddress = lpszStr;}
	void SetProtocol (LPCTSTR lpszStr){m_strProtocol= lpszStr;}
	void SetListenAddress(LPCTSTR lpszStr){m_strListenAddress= lpszStr;}
	void ConnectionPrivate(BOOL bPrivate){m_bConnectionPrivate = bPrivate;}
	void LoginPrivate     (BOOL bPrivate){m_bLoginPrivate = bPrivate;}
	void SetClassifiedFlag(UINT nFlag){m_nClassify=nFlag;}
	void SetNbConnectData (long lVal) {m_lNbConnectData = lVal;}
	void SetNbLoginData(long lVal) {m_lNbLoginData= lVal;}

	BOOL IsLocalNode();
	CString GetNodeName(){return GetItem();}
	CString GetName(){return GetItem();}
	CString GetLogin(){return m_strLogin;}
	CString GetPassword(){return m_strPassword;}
	CString GetIPAddress();
	CString GetProtocol(){return m_strProtocol;}
	CString GetListenAddress(){return m_strListenAddress;}
	BOOL IsConnectionPrivate(){return m_bConnectionPrivate;}
	BOOL IsLoginPrivate(){return m_bLoginPrivate;}
	UINT GetClassifiedFlag(){return m_nClassify;}
	long GetNbConnectData () {return m_lNbConnectData;}
	long GetNbLoginData() {return m_lNbLoginData;}

#if defined (EDBC)
	void SetSaveLogin(BOOL bSet){m_bSaveLogin = bSet;}
	BOOL GetSaveLogin(){return m_bSaveLogin;}
#endif
protected:
	void Copy(const CaNode& c);
	//
	// Own properties: (minimal data to serialize) + All in CaDBObject:
	// ****************************************************************
	//
	// Login Information:
	CString m_strLogin;        // Name of User
	CString m_strPassword;     // Password.
	//
	// Connection Information:
	CString m_strIPAddress;    // IP Address
	CString m_strProtocol;     // Protocol
	CString m_strListenAddress;// Listen Address (port)
	BOOL m_bConnectionPrivate; // Connection data is private
	BOOL m_bLoginPrivate;      // Connection data is private
	//
	// Characteristics of Node:
	UINT m_nClassify;  // NODE_SIMPLIFY | NODE_LOCAL | NODE_INSTALLATION | ...
	long m_lNbConnectData;
	long m_lNbLoginData;
#if defined (EDBC)
	BOOL m_bSaveLogin;
#endif
};




#endif // DMLVNODE_HEADER
