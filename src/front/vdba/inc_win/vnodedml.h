/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vnodedml.h: interface for accessing Virtual Node Data
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Access Virtual Node object
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 27-Jun-2003 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 13-Oct-2003 (schph01)
**    SIR #109864, Add new methods IsVnodeOptionDefined()and InitHostName() in
**    CaNodeDataAccess class for managed the -vnode command line option.
**    Add prototype for INGRESII_QueryNetLocalVnode function.
** 19-Dec-2003 (uk$so01)
**    SIR #111475, Coorperative shutdown between the DBMS Client & Server.
**    Created.
** 10-Mar-2010 (drivi01) SIR 123397
**    Updated return code from INGRESII_NodeCheckConnection to long
**    to be able to return error number.  The preferred return code
**    is STATUS, but including compat.h here was causing build problems
**    in other solutions therefore the return value is set to long as
**    it is typedef for STATUS.
**/


#if !defined(VNODEDML_HEADER)
#define VNODEDML_HEADER
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "dmlvnode.h"
#include "usrexcep.h"

class CaQueryNodeInfo
{
public:
	CaQueryNodeInfo(): m_nTypeObject(-1), m_strNode(_T("")){}
	~CaQueryNodeInfo(){}

	int m_nTypeObject;
	CString m_strNode;

};

class CaNodeDataAccess
{
public:
	//
	// If 'bDestroyOnDestructor' is TRUE, then call 'NodeLLTerminate()'
	// on Destructor.
	CaNodeDataAccess(BOOL bDestroyOnDestructor = TRUE);
	~CaNodeDataAccess();

	BOOL IsVnodeOptionDefined();
	BOOL InitHostName(LPCTSTR lpszHostName);
	BOOL InitNodeList();
	BOOL GetStatus(){return m_bStatus;}

protected:
	BOOL m_bStatus;
	BOOL m_bDestroy;
};

BOOL INGRESII_NetQueryNode (CaQueryNodeInfo* pQueryInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);

BOOL INGRESII_QueryNetLocalVnode ( CString &rcsLocalVnode );

//
// Query the list of Virtual Nodes, Servers, Logins, Connection Data, Attributes:
HRESULT VNODE_QueryNode         (CTypedPtrList<CObList, CaDBObject*>& list);
HRESULT VNODE_QueryServer       (CaNode* pNode, CTypedPtrList<CObList, CaDBObject*>& listObject);
HRESULT VNODE_QueryLogin        (CaNode* pNode, CTypedPtrList<CObList, CaDBObject*>& listObject);
HRESULT VNODE_QueryConnection   (CaNode* pNode, CTypedPtrList<CObList, CaDBObject*>& listObject);
HRESULT VNODE_QueryAttribute    (CaNode* pNode, CTypedPtrList<CObList, CaDBObject*>& listObject);


HRESULT VNODE_AddNode  (CaNode* pNode);
HRESULT VNODE_DropNode (CaNode* pNode);
HRESULT VNODE_AlterNode(CaNode* pNodeOld, CaNode* pNodeNew);

HRESULT VNODE_LoginAdd       (CaNodeLogin* pLogin);
HRESULT VNODE_LoginDrop      (CaNodeLogin* pLogin);
HRESULT VNODE_LoginAlter     (CaNodeLogin* pLoginOld, CaNodeLogin* pLoginNew);

HRESULT VNODE_ConnectionAdd  (CaNodeConnectData* pConnection);
HRESULT VNODE_ConnectionDrop (CaNodeConnectData* pConnection);
HRESULT VNODE_ConnectionAlter(CaNodeConnectData* pConnectionOld, CaNodeConnectData* pConnectionNew);

HRESULT VNODE_AttributeAdd   (CaNodeAttribute* pAttribute);
HRESULT VNODE_AttributeAlter (CaNodeAttribute* pAttributeOld, CaNodeAttribute* pAttributeNew);
HRESULT VNODE_AttributeDrop  (CaNodeAttribute* pAttribute);

CaNode* VNODE_Search (CTypedPtrList<CObList, CaNode*>& listNode, LPCTSTR lpszNode);
CaNode* VNODE_Search (CTypedPtrList<CObList, CaNode*>& listNode, CaNode* pNode);

class CaIngresRunning
{
public:
	CaIngresRunning(HWND wndCaller, LPCTSTR lpszCmdStart, LPCTSTR lpszRequestToStart, LPCTSTR lpszBoxCaption)
	{
		m_hwndCaller = wndCaller;
		m_strIngresStartCmd = lpszCmdStart;
		m_strRequestToStart = lpszRequestToStart;
		m_strCaption = lpszBoxCaption;
		m_bRequestToStart = TRUE;
		m_hEventExit = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_bShowWindow = TRUE;
	}
	CaIngresRunning(HWND wndCaller, LPCTSTR lpszCmdStart, UINT idsRequestToStart, UINT idsBoxCaption)
	{
		m_hwndCaller = wndCaller;
		m_strIngresStartCmd = lpszCmdStart;
		m_strRequestToStart.LoadString(idsRequestToStart);
		m_strCaption.LoadString(idsBoxCaption);
		m_bRequestToStart = TRUE;
		m_hEventExit = CreateEvent(NULL, TRUE, FALSE, NULL);
		m_bShowWindow = TRUE;
	}

	~CaIngresRunning()
	{
		CloseHandle(m_hEventExit);
	}
	HWND GetCaller(){return m_hwndCaller;}
	CString GetCmdStart(){return m_strIngresStartCmd;}
	BOOL GetRequestToStart(){return m_bRequestToStart;}
	CString GetRequestToStartText(){return m_strRequestToStart;}
	CString GetBoxCaption(){return m_strCaption;}
	HANDLE  GetEventExit(){return m_hEventExit;}
	BOOL IsShowWindow(){return m_bShowWindow;}
	void SetShowWindow(BOOL bSet){m_bShowWindow = bSet;}
protected:
	CString m_strIngresStartCmd;
	CString m_strCaption;
	CString m_strRequestToStart;
	HWND m_hwndCaller;
	BOOL m_bRequestToStart;
	HANDLE m_hEventExit;
	BOOL m_bShowWindow;
};

BOOL INGRESII_IsRunning();
//
// return:
//   0: not started
//   1: started
//   2: failed to started
long INGRESII_IsRunning(CaIngresRunning& ingresRunning);
//
// Check to see if there is an installation password.
BOOL HasInstallationPassword(BOOL bNoLocalInCache);
long INGRESII_NodeCheckConnection(LPCTSTR lpszNode);


//
// Object: CaNetTraficInfo 
// ************************
class CaNetTraficInfo: public CObject
{
	DECLARE_SERIAL (CaNetTraficInfo)
public:
	CaNetTraficInfo();
	void Cleanup();
	virtual ~CaNetTraficInfo();
	CaNetTraficInfo(const CaNetTraficInfo& c);
	CaNetTraficInfo operator = (const CaNetTraficInfo& c);
	//
	// Manually manage the object version:
	// When you detect that you must manage the newer version (class members have been changed),
	// the derived class should override GetObjectVersion() and return
	// the value N same as IMPLEMENT_SERIAL (CaXyz, CaDBObject, N).
	virtual UINT GetObjectVersion(){return 1;}

	virtual void Serialize (CArchive& ar);

	CTime GetTimeStamp(){return m_tTimeStamp;}
	void SetTimeStamp();
	void SetInboundMax(long nVal){m_nInboundMax = nVal;}
	void SetInbound(long nVal){m_nInbound = nVal;}
	void SetOutboundMax(long nVal){m_nOutboundMax = nVal;}
	void SetOutbound(long nVal){m_nOutbound = nVal;}
	void SetPacketIn(long nVal){m_nPacketIn = nVal;}
	void SetPacketOut(long nVal){m_nPacketOut = nVal;}
	void SetDataIn(long nVal){m_nDataIn = nVal;}
	void SetDataOut(long nVal){m_nDataOut = nVal;}

	long GetInboundMax(){return m_nInboundMax;}
	long GetInbound(){return m_nInbound;}
	long GetOutboundMax(){return m_nOutboundMax;}
	long GetOutbound(){return m_nOutbound;}
	long GetPacketIn(){return m_nPacketIn;}
	long GetPacketOut(){return m_nPacketOut;}
	long GetDataIn(){return m_nDataIn;}
	long GetDataOut(){return m_nDataOut;}


protected:
	void Copy (const CaNetTraficInfo& c);

	CTime m_tTimeStamp;       // The time (the last queried object, local time)
	long m_nInboundMax;
	long m_nInbound;
	long m_nOutboundMax;
	long m_nOutbound;

	long m_nPacketIn;
	long m_nPacketOut;
	long m_nDataIn;
	long m_nDataOut;

};
BOOL INGRESII_QueryNetTrafic (LPCTSTR lpszNode, LPCTSTR lpszListenAddress, CaNetTraficInfo& infoNetTrafic);



extern "C"
{
//
// TestInstallationThroughNet's body is coded in nodes.c:
BOOL TestInstallationThroughNet(char * puser, char * ppassword);
}

#endif // !defined(VNODEDML_HEADER)
