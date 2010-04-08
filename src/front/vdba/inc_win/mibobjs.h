/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : mibobjs.h: header file
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation of MIB Objects.
**
** History:
**
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
**    Created.
**/

#if !defined (MIBOBJECTS_GCF_HEADER)
#define MIBOBJECTS_GCF_HEADER

//
// Begin Extern "C" Section:
// ************************************************************************************************
#if defined (__cplusplus)
extern "C"
{
#endif

typedef struct tagLISTTCHAR
{
	TCHAR* lptchItem;
	struct tagLISTTCHAR* next;
} LISTTCHAR, FAR* LPLISTTCHAR;
int ListTchar_Count (LPLISTTCHAR oldList);
LPLISTTCHAR ListTchar_Add  (LPLISTTCHAR oldList, LPCTSTR lpszString);
LPLISTTCHAR ListTchar_Done (LPLISTTCHAR oldList);

typedef struct tagMIBOBJECT
{
	TCHAR tchszMIBCLASS[256]; // Example:  "exp.gcf.gcn.server.class"
	LPLISTTCHAR listValues;   // Example:   if not null, the list of strings represent the server classes.
	struct tagMIBOBJECT* next;
} MIBOBJECT, FAR* LPMIBOBJECT;
LPMIBOBJECT MibObject_Add  (LPMIBOBJECT oldList, LPCTSTR lpszMibClass);
LPMIBOBJECT MibObject_Search(LPMIBOBJECT oldList, LPCTSTR lpszMibClass);
LPMIBOBJECT MibObject_Done (LPMIBOBJECT oldList);

typedef void (CALLBACK* PfnUserFetchMIBObject) (void* pData, LPCTSTR lpszClass, LPCTSTR lpszInstance, LPCTSTR lpszValue);
typedef struct tagMIBREQUEST
{
	LPMIBOBJECT listMIB;
	BOOL bError;
	BOOL bEnd;
	PfnUserFetchMIBObject pfnUserFetch;
	LPARAM lUserData;
} MIBREQUEST, FAR* LPMIBREQUEST;
extern MIBREQUEST g_mibRequest;
BOOL QueryMIBObjects(LPMIBREQUEST pRequest, LPCTSTR lpszTarget, int nMode);
LPLISTTCHAR GetListServerClass(LPCTSTR lpszNode);
LPLISTTCHAR GetGWlist(LPCTSTR lpszNode, int* pError);

#if defined (__cplusplus)
}
#endif

// End of Extern "C" Section:
// ************************************************************************************************







//
// Begin C++ Only:
// ************************************************************************************************
#if defined (__cplusplus)
#include "dmlbase.h"
#define _USE_IMA_SESSIONxTABLE // Query sessions using IMA table instead of MIB Objects

class CaNode;
class CaSessionConnected: public CaDBObject
{
public:
	CaSessionConnected();
	virtual ~CaSessionConnected(){}
	BOOL IsSystemInternal();
	BOOL IsVisualToolInternal();

	void SetID(LPCTSTR lpszText){SetItem(lpszText);}
	void SetHost(LPCTSTR lpszText){SetOwner(lpszText);}

	void SetDescription(LPCTSTR lpszText){m_strDescription = lpszText;}
	void SetUser(LPCTSTR lpszText){m_strUser = lpszText;}
	void SetPID(LPCTSTR lpszText){m_strPID = lpszText;}
	void SetDatabase(LPCTSTR lpszText){m_strDatabase = lpszText;}
	void SetAplication(LPCTSTR lpszText){m_strApplication = lpszText;}
	void SetConnectString(LPCTSTR lpszText){m_strConnectString = lpszText;}
	void SetRUser(LPCTSTR lpszText){m_strUserReal = lpszText;}
	void SetEUser(LPCTSTR lpszText){m_strUserEffective = lpszText;}

	CString GetPID(){return m_strPID;}
	CString GetID(){return GetItem();}
	CString GetHost(){return GetOwner();}
	CString GetUser(){return m_strUser;}
	CString GetDatabase(){return m_strDatabase;}
	CString GetConnectString(){return m_strConnectString;}
	CString GetDescription(){return m_strDescription;}
	CString GetRUser(){return m_strUserReal;}
	CString GetEUser(){return m_strUserEffective;}


	static CaSessionConnected* Search (CTypedPtrList<CObList, CaSessionConnected*>* pList, LPCTSTR lpszKey, int nKey = 0);
protected:
	CString m_strPID;
	CString m_strUser;
	CString m_strDescription;
	CString m_strDatabase;
	CString m_strConnectString;
	CString m_strApplication;
	CString m_strUserReal;
	CString m_strUserEffective;
};


//
// Get the list of [remote] nodes to which there are connected session
// from the calling [current] installation:
BOOL QueryActiveOutboundNode(CTypedPtrList<CObList,CaNode*>& listVNode);

//
// Get the list of remote nodes 
// (the remote nodes where there is connection from the current host that calls this function)
BOOL QueryRemoteConnectingNodeName(CStringList& listNode);
//
// If pNode parameter in CaQuerySessionInfo represents a local node, "(local)", then this function attempts to query
//    the sessions connecting to the calling host's DBMS.
// If pNode parameter in CaQuerySessionInforepresents a remote node, then this function attempts to query
//    the sessions connecting from calling host to the remote DBMS.
// May raise exception: CeSqlException if _USE_IMA_SESSIONxTABLE is defined.
class CaQuerySessionInfo
{
public:
	CaQuerySessionInfo(CaNode* pNode):m_pNode(pNode), m_strStatement(_T("")), m_strSessionDescription(_T("")){}
	void SetStatement(LPCTSTR lpszText){m_strStatement = lpszText;}
	void SetSessionDescription(LPCTSTR lpszText){m_strSessionDescription = lpszText;}

	CaNode* GetNode(){return m_pNode;}
	CString GetStatement(){return m_strStatement;}
	CString GetSessionDescription(){return m_strSessionDescription;}

protected:
	CString m_strStatement;
	CString m_strSessionDescription;
	CaNode* m_pNode;
};
BOOL QuerySession (CaQuerySessionInfo* pInfo, CTypedPtrList<CObList, CaSessionConnected*>& listSession);


#endif // __cplusplus 
//
// End C++ Only:
// ************************************************************************************************








#endif // MIBOBJECTS_GCF_HEADER
