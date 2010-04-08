/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcdadml.cpp: implementation file
**    Project  : Visual Config Diff 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation of data
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 06-Jan-2004 (schph01)
**    SIR #111462, Put string into resource files
** 18-Mar-04 (uk$so01)
**    SIR #109221, Correct spelling error.
** 14-Apr-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**    Also eliminated some warnings under .NET
** 01-Jun-04 (uk$so01)
**    SIR #109221, Additional change.
**    (If the current config.dat is corrupt then the ii.<hostname> will be
**    the ii.h where h=GChostname())
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
** 25-Oct-2004 (noifr01)
**    applied similar fix as in the import assistant (for bug 113319)
**    where an error in DBCS character management was found in GetFieldsFromLine()
** 18-Mar-2009 (drivi01)
**    Declare int i used in for loops throughout this file.
**/

#include "stdafx.h"
#include "vcda.h"
#include "vcdadml.h"
#include "udlgmain.h"
#include "ingobdml.h"
#include "vnodedml.h"
#include "tlsfunct.h"
#include "constdef.h"
#include <io.h>

extern "C"
{
#include <compat.h>
#include <er.h>
#include <locl.h>
#include <pm.h>
BOOL ping_iigcn( void );
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define WORKAROUND_PASSWORD
//#define TEST_RESTORE

#if !defined (_DEBUG) && defined (TEST_RESTORE)
#error "If you have tested the restore operation successfully, then turn TEST_RESTORE off !!!
#endif
class CaRestoreVNode: public CObject
{
public:
	CaRestoreVNode(TCHAR tchDiff)
	{
		m_tchDifference = tchDiff;
		m_pOldStruct = NULL;
		m_pNewStruct = NULL;
	}
	virtual ~CaRestoreVNode()
	{
		if (m_pOldStruct)
			delete m_pOldStruct;
		if (m_pNewStruct)
			delete m_pNewStruct;
	}

	TCHAR       m_tchDifference; // {'+', '-', '!'}
	CaDBObject* m_pOldStruct;    // must be not null if m_tchDifference = '!'
	CaDBObject* m_pNewStruct;
};

static void VCDA_ExtractCommon(LPCTSTR lpText, int nCount, CString& left, CString& right);
static void CompareGeneralParam (
	CaCda* pObj1, 
	CaCda* pObj2, 
	CaCompareParam* pParam, 
	CTypedPtrList< CObList, CaCdaDifference* >& ldiff);
static void CompareConfigParam (
	CaCda* pObj1, 
	CaCda* pObj2, 
	CaCompareParam* pParam, 
	CTypedPtrList< CObList, CaCdaDifference* >& ldiff);
static void CompareEnvironmentParam (
	CaCda* pObj1, 
	CaCda* pObj2, 
	CaCompareParam* pParam, 
	CTypedPtrList< CObList, CaCdaDifference* >& ldiff);
static void CompareVirtualNodeParam (
	CaCda* pObj1, 
	CaCda* pObj2, 
	CaCompareParam* pParam, 
	CTypedPtrList< CObList, CaCdaDifference* >& ldiff);
static BOOL QueryNodesDefinition(CTypedPtrList< CObList, CaCda* >& ls);
static void SaveNodeDefinitions(FILE* pFile);
static void GetFieldsFromLine (CString& strLine, CStringArray& arrayFields);
static void GenerateVnodeParam(LPCTSTR lpszVNode, CStringArray& arrayFields, CString& strName, CString& strValue);
static CaCda* SearchVNode (CaCda* pData, CTypedPtrList< CObList, CaCda* >& ls, BOOL bRemove, BOOL bName = FALSE);



static void ParseConnectionData(LPCTSTR lpszValue, CString& strIPAdr, CString& strProtocol, CString& strListenAdr)
{
	strIPAdr = _T("");
	strProtocol = _T("");
	strListenAdr = _T("");

	CString strLine = lpszValue;
	int nPos = strLine.ReverseFind(_T('/'));
	if (nPos != -1)
	{
		strListenAdr = strLine.Mid(nPos+1);
		strLine = strLine.Left(nPos);
		
		nPos = strLine.ReverseFind(_T('/'));
		if (nPos != -1)
		{
			strProtocol = strLine.Mid(nPos+1);
			strIPAdr = strLine.Left(nPos);
		}
	}
}

static int compareVnodeDiff (const void* e1, const void* e2)
{
	UINT o1 = *((UINT*)e1);
	UINT o2 = *((UINT*)e2);

	CaCdaDifference* p1 = (CaCdaDifference*)o1;
	CaCdaDifference* p2 = (CaCdaDifference*)o2;
	if (p1 && p2)
	{
		CString s1 = p1->GetName();
		CString s2 = p2->GetName();
		if (s1.Find (_T("/Private ")) != -1)
			s1.Replace(_T("/Private "), _T("/"));
		else
			s1.Replace(_T("/Global "), _T("/"));
		if (s2.Find (_T("/Private ")) != -1)
			s2.Replace(_T("/Private "), _T("/"));
		else
			s2.Replace(_T("/Global "), _T("/"));

		return s1.CompareNoCase (s2);
	}
	else
		return 0;
}


static int compareVnodeItem (const void* e1, const void* e2)
{
	UINT o1 = *((UINT*)e1);
	UINT o2 = *((UINT*)e2);

	CaCda* p1 = (CaCda*)o1;
	CaCda* p2 = (CaCda*)o2;
	if (p1 && p2)
	{
		CString s1 = p1->GetName();
		CString s2 = p2->GetName();
		if (s1.Find (_T("/Private ")) != -1)
			s1.Replace(_T("/Private "), _T("/"));
		else
			s1.Replace(_T("/Global "), _T("/"));
		if (s2.Find (_T("/Private ")) != -1)
			s2.Replace(_T("/Private "), _T("/"));
		else
			s2.Replace(_T("/Global "), _T("/"));

		return s1.CompareNoCase (s2);
	}
	else
		return 0;
}


class CaStartGcn
{
public:
	CaStartGcn();
	~CaStartGcn();
protected:
	BOOL m_bStarted;
};

extern UINT IngresStartThreadProc(LPVOID pParam);
CaStartGcn::CaStartGcn()
{
	m_bStarted = FALSE;
	if (!ping_iigcn())
	{
		//
		// The Name Server has not been started.
		// Start the iigcn first.
		CaIngresRunning run (NULL, _T("ingstart -iigcn"), _T(""), _T(""));
		run.SetShowWindow(FALSE);
		IngresStartThreadProc((LPVOID)&run);
		ResetEvent(run.GetEventExit());
		m_bStarted = TRUE;
		Sleep (1000*3);
	}
}

CaStartGcn::~CaStartGcn()
{
	if (m_bStarted)
	{
		//
		// The Name Server has been started by this class.
		// Stop the iigcn.
		CaIngresRunning run (NULL, _T("ingstop -iigcn"), _T(""), _T(""));
		run.SetShowWindow(FALSE);
		IngresStartThreadProc((LPVOID)&run);
		ResetEvent(run.GetEventExit());
		m_bStarted = FALSE;
	}
}


//
// CaRestoreParam
// ************************************************************************************************
CaRestoreParam::CaRestoreParam()
{
	m_bConfig = TRUE;
	m_bSystemVariable = TRUE; 
	m_bUserVariable = TRUE; 
	m_bVirtualNode = TRUE; 
	m_bPath; 

	m_strBackupFile = _T("");

	m_plg1 = NULL;
	m_plg2 = NULL;
	m_plc1 = NULL;
	m_plc2 = NULL;
	m_ples1 = NULL;
	m_ples2 = NULL;
	m_pleu1 = NULL;
	m_pleu2 = NULL;
	m_plvn1 = NULL;
	m_plvn2 = NULL;
	m_plOtherhost1 = NULL;
	m_plOtherhost2 = NULL;
	m_plistDifference = NULL;
	m_pIngresVariable = NULL;
}

void CaRestoreParam::Cleanup()
{
	while (!m_listSubStituteInfo.IsEmpty())
		delete m_listSubStituteInfo.RemoveHead();
	while (!m_listRestoreParam.IsEmpty())
		delete m_listRestoreParam.RemoveHead();
	m_listROVariable.RemoveAll();
}


//
// CaCompareParam
// ************************************************************************************************
CaCompareParam::CaCompareParam()
{
	m_strHost1 = _T("");
	m_strHost2 = _T("");
	m_strII1 = _T("");
	m_strII2 = _T("");
	m_strIISystem1 = _T("");
	m_strIISystem2 = _T("");
	m_strUser1 = _T("");
	m_strUser2 = _T("");
	m_strPlatform1 = _T("");
	m_strPlatform2 = _T("");
	m_nSubstituteHost = 0;

	m_listIIDependent.AddTail(_T("II_INSTALLATION"));
	m_listIIDependent.AddTail(_T("II_BIND_SVC_%s"));
	m_listIIDependent.AddTail(_T("II_CHARSET%s"));
	m_listIIDependent.AddTail(_T("II_GCN%s_PORT"));
	m_listIIDependent.AddTail(_T("II_GCN%s_LCL_VNODE"));
	m_listIIDependent.AddTail(_T("II_GC%s_TRACE"));
}

CaCompareParam::CaCompareParam(const CaCompareParam& c)
{
	Copy (c);
}

CaCompareParam CaCompareParam::operator = (const CaCompareParam& c)
{
	Copy (c);
	return *this;
}

void CaCompareParam::Copy (const CaCompareParam& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;

	m_strHost1 = c.m_strHost1;
	m_strHost2 = c.m_strHost2;
	m_strII1 = c.m_strII1;
	m_strII2 = c.m_strII2;
	m_strIISystem1 = c.m_strIISystem1;
	m_strIISystem2 = c.m_strIISystem2;
	m_strUser1 = c.m_strUser1;
	m_strUser2 = c.m_strUser2;
	m_strPlatform1 = c.m_strPlatform1;
	m_strPlatform2 = c.m_strPlatform2;
	m_nSubstituteHost = c.m_nSubstituteHost;

	POSITION pos = c.m_listIIDependent.GetHeadPosition();
	while (pos != NULL)
	{
		CString strItem = c.m_listIIDependent.GetNext(pos);
		m_listIIDependent.AddTail(strItem);
	}
}


void CaCompareParam::CleanIgnore()
{
	m_listIgnore.RemoveAll();
}

void CaCompareParam::AddIgnore(LPCTSTR lpszName)
{
	m_listIgnore.AddTail(lpszName);
}

BOOL CaCompareParam::IsIgnore (LPCTSTR lpszName)
{
	return (m_listIgnore.Find(lpszName) == NULL)? FALSE: TRUE;
}


//
// CaCda
// ************************************************************************************************
CaCda::CaCda(LPCTSTR lpszName, LPCTSTR lpszValue, cdaType nType)
{
	m_strHost = _T("");
	m_nType = nType;
	
	if (m_nType == CDA_CONFIG)
	{
		VCDA_ExtractCommon(lpszName, 2, m_strLeft, m_strRight);
		CString strLeft = m_strLeft;
		strLeft.MakeLower();
		if (strLeft.Find(_T("ii.*.")) != 0)
		{
			//
			// It must be "ii.<host name>.???". Then parse host name:
			CString str = m_strLeft; // Preserve the case
			int nFound = str.Find(_T('.'));
			if (nFound != -1)
			{
				str = str.Mid(nFound +1);
				nFound = str.Find(_T('.'));
				if (nFound != -1)
					m_strHost = str.Left(nFound);
			}
		}
	}
	else 
	{
		m_strLeft = lpszName;
		m_strRight = _T("");
	}
	m_strValue= lpszValue;
}

CaCda::CaCda(LPCTSTR lpszHost, LPCTSTR lpszLeft, LPCTSTR lpszRight, LPCTSTR lpszValue, cdaType nType)
{
	m_strHost = lpszHost;
	m_strLeft = lpszLeft;
	m_strRight= lpszRight;
	m_strValue= lpszValue;
	m_nType   = nType;
}


CaCda::CaCda(const CaCda& c)
{
	Copy (c);
}

void CaCda::Copy(const CaCda& c)
{
	m_strHost = c.m_strHost;
	m_strLeft = c.m_strLeft;
	m_strRight = c.m_strRight;
	m_strValue = c.m_strValue;
	m_nType = c.m_nType;
}

CString CaCda::GetName()
{
	if (m_nType != CDA_CONFIG)
		return m_strLeft;
	else
		return (m_strLeft + m_strRight);
}


CString CaCda::GetName2(CaCompareParam* pInfo, int nSnapshot, BOOL bSubstituteUser)
{
	CString strSnapshotHost = (nSnapshot== 1)? pInfo->GetHost1(): pInfo->GetHost2();
	CString strSnapshotUser = (nSnapshot== 1)? pInfo->GetUser1(): pInfo->GetUser2();
	CString strII =           (nSnapshot== 1)? pInfo->GetInstallation1(): pInfo->GetInstallation2();

	if (m_nType == CDA_CONFIG)
	{
		CString strItem  = m_strLeft;
		CString strRight = m_strRight;
		if (m_strHost.IsEmpty())
		{
			//
			// It belongs to the "ii.*." family.
		}
		else
		{
			//
			// It belongs to the "ii.xxx." family. where xxx is the host name.
			strItem = m_strLeft;
			//
			// Host name is not case sensitive
			if (pInfo->IsIgnore(_T("host name")) && m_strHost.CompareNoCase(strSnapshotHost) == 0)
			{
				strItem.Replace(m_strHost, _T("<host name>"));
			}

			if (bSubstituteUser)
			{
				if (pInfo->IsIgnore(_T("user")))
				{
					int nPos = m_strRight.Find (_T("privileges.user."));
					if (nPos != -1)
					{
						CString strR = m_strRight.Mid  (nPos + _tcslen(_T("privileges.user.")));
						CString strL = m_strRight.Left (nPos + _tcslen(_T("privileges.user.")));
						//
						// User name is case sensitive
						if (strR.Compare (strSnapshotUser) == 0)
						{
							strRight = strL + _T("<user name>");
						}
					}
				}
			}
		}
		strItem += strRight;
		return strItem;
	}
	else
	if (m_nType == CDA_ENVSYSTEM || m_nType == CDA_ENVUSER)
	{
		if (pInfo->IsIgnore(_T("ii_installation")))
		{
			CString strEnv;
			CStringList& listIIDependent = pInfo->GetListIIDependent();
			POSITION pos = listIIDependent.GetHeadPosition();
			while (pos != NULL)
			{
				CString strFmt = listIIDependent.GetNext(pos);
				strEnv.Format(strFmt, (LPCTSTR)strII);
				if (strEnv == m_strLeft)
				{
					strEnv.Format(strFmt, _T("<installation id>"));
					return strEnv;
				}
			}
		}
		return m_strLeft;
	}
	else
	{
		return m_strLeft;
	}
}

CaCda* CaCda::Find(LPCTSTR lpRight, CTypedPtrList< CObList, CaCda* >* pList)
{
	if (!pList)
		return NULL;
	POSITION pos = pList->GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = pList->GetNext(pos);
		if (pObj->GetType() == CDA_CONFIG)
		{
			if (pObj->GetRight().CompareNoCase(lpRight) == 0)
				return pObj;
		}
		else
		{
			if (pObj->GetLeft().CompareNoCase(lpRight) == 0)
				return pObj;
		}
	}
	return NULL;
}



//
// CaMappedHost
// ************************************************************************************************
CaMappedHost::CaMappedHost()
{
	m_strHost1 = _T("");
	m_strHost2 = _T("");
}

CaMappedHost::CaMappedHost(LPCTSTR lpszHost1, LPCTSTR lpszHost2)
{
	m_strHost1 = lpszHost1;
	m_strHost2 = lpszHost2;
}


//
// GLOBAL & STATIC FUNCTIONS
// ************************************************************************************************
static CString GetHost()
{
	USES_CONVERSION;
	char* phost = PMhost();
	CString strHost = A2T(phost);
	return strHost;
}

static void NormalizePath (CString& strPath, CaCompareParam* pParam, int nSnapshot)
{
	CString strIISystem = (nSnapshot == 1)? pParam->GetIISystem1(): pParam->GetIISystem2();
	CString strNewPath = _T("");
	int nPos = strPath.Find (_T('\\'));
	while (nPos != -1)
	{
		strNewPath += strPath.Left (nPos +1);
		strPath =  strPath.Mid (nPos +1);
		strPath.TrimLeft(_T('\\'));

		nPos = strPath.Find (_T('\\'));
	}
	if (strNewPath.IsEmpty())
		strNewPath = strPath;
	else
	{
		strNewPath += strPath;
	}

	CString strTemp = strNewPath;
	strTemp.TrimLeft(_T('\''));
	strTemp.TrimRight(_T('\''));
	if (strTemp.Find (strIISystem) == 0)
	{
		strNewPath.Replace(strIISystem, _T("<ii_system>"));
	}

	strPath = strNewPath;
}


static void sWriteTo (FILE* pFile, LPCTSTR lpszName, LPCTSTR lpszValue)
{
	CString strItem;
	TCHAR tchszBuff [128];

	_tcscpy (tchszBuff, lpszName);
	int i, nLen = _tcslen(tchszBuff);
	tchszBuff[nLen] = _T(':');
	tchszBuff[nLen+1] = _T('\0');
	for (i=nLen+1; i<48; i++)
	{
		tchszBuff[i] = _T(' ');
		tchszBuff[i+1] = _T('\0');
	}
	strItem.Format(_T("%s%s\n"), tchszBuff, lpszValue);

	_fputts(strItem, pFile);
}

static void sWriteTo (FILE* pFile, LPCTSTR lpszName, int nVale)
{
	CString strItem;
	strItem.Format(_T("%d"), nVale);
	sWriteTo(pFile, lpszName, strItem);
}


// 
// Check if network installation:
BOOL VCDA_IsNetworkInstallation()
{
	TCHAR* pEnv;
	pEnv = _tgetenv(_T("II_CONFIG_LOCAL"));
	if (pEnv)
		return TRUE;
	pEnv = _tgetenv(_T("II_ADMIN"));
	if (pEnv)
		return TRUE;
	return FALSE;
}

//
// The use of ManageMainEnvironment:
//   - for writing: lMain must be NULL and pFile must not be NULL
//   - for reading: lMain must not be NULL and pFile must be NULL
static void ManageMainEnvironment(FILE* pFile, CString& strConfigPath, CTypedPtrList< CObList, CaCda* >* lMain)
{
	CString strItem;
	TCHAR* pEnv = NULL;
	const int nItem = 6;
	TCHAR tchszEnv [nItem][32] = 
	{
		_T("II_SYSTEM"),
		_T("II_INSTALLATION"),
		_T("II_CLUSTER"),
		_T("II_CONFIG_LOCAL"),
		_T("II_ADMIN"),
		_T("II_NETBIOS_NAME")
	};
	CStringArray arrayValue;
	arrayValue.SetSize(nItem);

	for (int i=0; i<nItem; i++)
	{
		strItem = _T("");
		if (i==1) // II_INSTALLATION
			strItem = INGRESII_QueryInstallationID(FALSE);
		else
		{
			pEnv = _tgetenv(tchszEnv[i]);
			strItem = pEnv;
		}
		arrayValue.SetAt(i, strItem);

		if (i==0 && !strItem.IsEmpty()) // II_SYSTEM
		{
			strConfigPath = strItem;
			strConfigPath += consttchszIngFiles;
			strConfigPath += _T("config.dat");
		}
		else
		if (i==3 && !strItem.IsEmpty()) // II_CONFIG_LOCAL
		{
			strConfigPath = strItem;
#if defined (MAINWIN)
			strConfigPath += _T("/config.dat");
#else
			strConfigPath += _T("\\config.dat");
#endif
		}
	}

	if (lMain == NULL)
	{
		for (int i=0; i<nItem; i++)
		{
			strItem = arrayValue.GetAt(i);
			if (!strItem.IsEmpty())
				sWriteTo(pFile, tchszEnv[i], (LPCTSTR)strItem);
		}
	}
	else
	{
		for (int i=0; i<nItem; i++)
		{
			strItem = arrayValue.GetAt(i);
			if (!strItem.IsEmpty())
			{
				lMain->AddTail(new CaCda(tchszEnv[i], strItem, CDA_GENERAL));
			}
		}
	}
}

void VCDA_SaveInstallation(LPCTSTR lpszFile, CaIngresVariable* pIngresVariable)
{
	FILE* pFile = _tfopen(lpszFile, _T("w"));
	if (!pFile)
	{
		AfxMessageBox (IDS_MSG_FAIL_2_CREATE_SNAPSHOT);
		throw (int)1; // Cannot create file
	}
	CString strIngresVersion, strIngresPatches;
	CString strHost, strUser, strConfigPath;
	DWORD dwSize = 255;
	TCHAR tchszUser[255 + 1];
	strUser = _T("???");
	if (GetUserName (tchszUser, &dwSize))
		strUser = tchszUser;
	strHost = GetHost();
	VCDA_GetIngresVersionIDxPatches (strIngresVersion, strIngresPatches);
	//
	// General Parameters:
	_fputts(_T("[GENERAL PARAMETERS]\n"), pFile);
#if defined (MAINWIN)
	sWriteTo(pFile, _T("PLATFORM"), _T("UNIX"));
#else
	sWriteTo(pFile, _T("PLATFORM"), _T("WINDOWS"));
#endif
	sWriteTo(pFile, _T("USER"), strUser);
	sWriteTo(pFile, _T("HOST NAME"), strHost);
	sWriteTo(pFile, _T("VERSION"), strIngresVersion);
	sWriteTo(pFile, _T("PATCHES"), strIngresPatches);
	ManageMainEnvironment(pFile, strConfigPath, NULL);

	//
	// Configuration Parameters:
	_fputts(_T("[CONFIGURATION PARAMETERS]\n"), pFile);
	if (!strConfigPath.IsEmpty())
	{
		FILE* pFileIn = _tfopen(strConfigPath, _T("r"));
		if (pFileIn)
		{
			TCHAR tchszBuff [512];
			while (_fgetts (tchszBuff, 500, pFileIn) != NULL)
			{
				_fputts(tchszBuff, pFile);

				if (feof(pFileIn) != 0)
					break;
			}
			fclose(pFileIn);
		}
		else
		{
			CString strMsg;
			fclose(pFile);
			AfxFormatString1(strMsg, IDS_MSG_NOOPEN_FILE, (LPCTSTR)strConfigPath);
			AfxMessageBox (strMsg);
			throw (int)2; // Cannot open the config.dat file
		}
	}

	//
	// Environment Parameters(SYSTEM):
	_fputts(_T("[ENVIRONMENT VARIABLES (SYSTEM)]\n"), pFile);
	CTypedPtrList<CObList, CaEnvironmentParameter*> listParameter;
	pIngresVariable->QueryEnvironment(listParameter, FALSE, FALSE);
	pIngresVariable->QueryEnvironment(listParameter, FALSE, TRUE);

	while (!listParameter.IsEmpty())
	{
		CaEnvironmentParameter* pObj = listParameter.RemoveHead();
		sWriteTo(pFile, pObj->GetName(), pObj->GetValue());
		delete pObj;
	}
	//
	// Environment Parameters(USER):
	_fputts(_T("[ENVIRONMENT VARIABLES (USER)]\n"), pFile);
	pIngresVariable->QueryEnvironmentUser(listParameter, FALSE);
	while (!listParameter.IsEmpty())
	{
		CaEnvironmentParameter* pObj = listParameter.RemoveHead();
		sWriteTo(pFile, pObj->GetName(), pObj->GetValue());
		delete pObj;
	}
	//
	// The definitions of Virtual Nodes:
#if defined (_VIRTUAL_NODE_AVAILABLE)
	SaveNodeDefinitions(pFile);
#endif

	fclose(pFile);
}


static BOOL Section(LPCTSTR lpszText, cdaType& nSection)
{
	CString s = lpszText;
	s.MakeUpper();

	if (s.Find(_T("[GENERAL PARAMETERS]")) != -1)
	{
		nSection = CDA_GENERAL;
		return TRUE;
	}
	if (s.Find(_T("[CONFIGURATION PARAMETERS]")) != -1)
	{
		nSection = CDA_CONFIG;
		return TRUE;
	}
	if (s.Find(_T("[ENVIRONMENT VARIABLES (SYSTEM)]")) != -1)
	{
		nSection = CDA_ENVSYSTEM;
		return TRUE;
	}
	if (s.Find(_T("[ENVIRONMENT VARIABLES (USER)]")) != -1)
	{
		nSection = CDA_ENVUSER;
		return TRUE;
	}
	if (s.Find(_T("[VIRTUAL NODE DEFINITIONS]")) != -1)
	{
		nSection = CDA_VNODE;
		return TRUE;
	}
	return FALSE;
}

static void ExtractNameValue (LPCTSTR lpszText, CString& strName, CString& strValue)
{
	CString s = lpszText;
	//
	// The first matched ':' must be the separator of NAME in string "<NAME>: <VALUE>".
	// It cannot be part of value like "C:\\IngresII"
	int nPos = s.Find(_T(':')); 
	strName = _T("");
	strValue = _T("");

	if (nPos != 0)
	{
		strName = s.Left(nPos);
		strValue = s.Mid(nPos +1);

		strName.TrimLeft();
		strName.TrimRight();
		strValue.TrimLeft();
		strValue.TrimRight();
	}
	else
	{
		strName = s;
		strName.TrimLeft();
		strName.TrimRight();
	}
}

//
// nGcn = -1 (ignore)
//      =  0 (stopped)
//      =  1 (started)
void VCDA_QueryCdaData(LPCTSTR lpszFile, CTypedPtrList< CObList, CaCda* >& ls, int nGcn)
{
	FILE* pFileIn = _tfopen(lpszFile, _T("r"));
	if (pFileIn)
	{
		CString strName, strValue;
		cdaType nType, nSection;
		TCHAR tchszBuff [512];
		while (_fgetts (tchszBuff, 500, pFileIn) != NULL)
		{
			RemoveEndingCRLF(tchszBuff);
			if (Section(tchszBuff, nSection))
			{
				nType = nSection;
			}
			else
			{
				ExtractNameValue (tchszBuff, strName, strValue);
				BOOL bAdd = TRUE;
				if (nType == CDA_VNODE && nGcn == 0)
					bAdd = FALSE;
				if (bAdd)
				{
					CaCda* pData = NULL;
					if (nType == CDA_VNODE)
					{
						CString strTemp = strValue;
						//
						// Parse the Virtual Node definitions:
						CStringArray arrayFields;
						GetFieldsFromLine (strTemp, arrayFields);
						GenerateVnodeParam(strName, arrayFields, strTemp, strValue);
						pData = new CaCda(strName, strTemp, _T(""), strValue, CDA_VNODE);
					}
					else
						pData = new CaCda(strName, strValue, nType);
					ls.AddTail(pData);
				}
			}

			if (feof(pFileIn) != 0)
				break;
		}
		fclose(pFileIn);
	}
	else
	{
		CString strMsg;
		AfxFormatString1(strMsg, IDS_MSG_NOOPEN_FILE, lpszFile);
		AfxMessageBox (strMsg);
		throw (int)1;
	}
}

BOOL VCDA_Compare (CuDlgMain* pDlgMain, LPCTSTR lpszFile1, LPCTSTR lpszFile2)
{
	CString strMsg;
	CString strSnapshot1, strSnapshot2;
	CTypedPtrList< CObList, CaCda* > ls1;
	CTypedPtrList< CObList, CaCda* > ls2;
	CaIngresVariable* pIngresVariable = pDlgMain->GetIngresVariable();
	try
	{
		BOOL bVnode = FALSE;
		CString strPath = _T("");
		if (lpszFile2 == NULL) // Compare current installation with a given snap shot file:
		{
			//
			// General (main) Parameters:
			CString strIngresVersion, strIngresPatches;
			CString strHost, strUser;
			DWORD dwSize = 255;
			TCHAR tchszUser[255 + 1];
			VCDA_GetIngresVersionIDxPatches (strIngresVersion, strIngresPatches);

#if defined (MAINWIN)
			ls1.AddTail(new CaCda(_T("PLATFORM"), _T("UNIX"), CDA_GENERAL));
#else
			ls1.AddTail(new CaCda(_T("PLATFORM"), _T("WINDOWS"), CDA_GENERAL));
#endif
			if (GetUserName (tchszUser, &dwSize))
			{
				strUser = tchszUser;
				ls1.AddTail(new CaCda(_T("USER"), strUser, CDA_GENERAL));
			}
			strHost = GetHost();
			ls1.AddTail(new CaCda(_T("HOST NAME"), strHost, CDA_GENERAL));
			ls1.AddTail(new CaCda(_T("VERSION"), strIngresVersion, CDA_GENERAL));
			ls1.AddTail(new CaCda(_T("PATCHES"), strIngresPatches, CDA_GENERAL));
			ManageMainEnvironment(NULL, strPath, &ls1);

			//
			// Check if the file can be read/accessed:
			if (_taccess(strPath, 0) == -1) // File does not exist
			{
				AfxFormatString1(strMsg, IDS_MSG_NOSNAPSHOT, (LPCTSTR)strPath);
				AfxMessageBox (strMsg);
				throw (int)1;
			}
			if (_taccess(strPath, 4) == -1) // File cannot be accessed
			{
				AfxFormatString1(strMsg, IDS_MSG_NOACCESS_FILE, (LPCTSTR)strPath);
				AfxMessageBox (strMsg);
				throw (int)1;
			}
			//
			// Read the current installation (config.dat):
			FILE* pFileIn = _tfopen(strPath, _T("r"));
			if (pFileIn)
			{
				TCHAR tchszBuff [512];
				CString strName, strValue;
				while (_fgetts (tchszBuff, 500, pFileIn) != NULL)
				{
					RemoveEndingCRLF(tchszBuff);
					ExtractNameValue (tchszBuff, strName, strValue);
					CaCda* pData = new CaCda(strName, strValue, CDA_CONFIG);
					ls1.AddTail(pData);

					if (feof(pFileIn) != 0)
						break;
				}
				fclose(pFileIn);
			}
			else
			{
				AfxFormatString1(strMsg, IDS_MSG_NOOPEN_FILE, (LPCTSTR)strPath);
				AfxMessageBox (strMsg);
				throw (int)1;
			}
			//
			// Read the current installation (Environment Variable):
			CTypedPtrList<CObList, CaEnvironmentParameter*> listParameter;
			pIngresVariable->QueryEnvironment(listParameter, FALSE, FALSE);
			pIngresVariable->QueryEnvironment(listParameter, FALSE, TRUE);
			while (!listParameter.IsEmpty())
			{
				CaEnvironmentParameter* pObj = listParameter.RemoveHead();
				CaCda* pData = new CaCda(pObj->GetName(), pObj->GetValue(), CDA_ENVSYSTEM);
				ls1.AddTail(pData);
				delete pObj;
			}
			pIngresVariable->QueryEnvironmentUser(listParameter, FALSE);
			while (!listParameter.IsEmpty())
			{
				CaEnvironmentParameter* pObj = listParameter.RemoveHead();
				CaCda* pData = new CaCda(pObj->GetName(), pObj->GetValue(), CDA_ENVUSER);
				ls1.AddTail(pData);
				delete pObj;
			}
			//
			// Read the current installation (Virtual Nodes):
			bVnode = QueryNodesDefinition(ls1);

			//
			// Read from the snap-shot file:
			VCDA_QueryCdaData(lpszFile1, ls2, bVnode? 1: 0);
			strSnapshot1.LoadString(IDS_TAB_CURRENT_INSTALL);
			strSnapshot2 = lpszFile1;
		}
		else // Compare two snap shot files:
		{
			//
			// Read from the snap-shot files:
			VCDA_QueryCdaData(lpszFile1, ls1);
			VCDA_QueryCdaData(lpszFile2, ls2);
			strSnapshot1 = lpszFile1;
			strSnapshot2 = lpszFile2;
		}

		if (lpszFile2 == NULL)
			VCDA_CheckInconsistency(ls1, ls2, TRUE,  strPath, lpszFile1);
		else
			VCDA_CheckInconsistency(ls1, ls2, FALSE, lpszFile1, lpszFile2);

		pDlgMain->SetSnapshotCaptions(strSnapshot1, strSnapshot2);
		pDlgMain->PrepareData(ls1, ls2);  // Update parameter m_compareParam (class CaCompareParam)
		UINT nMask = PARAM_GENERAL|PARAM_CONFIGxENV|PARAM_OTHERHOST;
		CaCompareParam& pCp = pDlgMain->GetComparedParam();
		if (pCp.GetPlatform1().IsEmpty() || pCp.GetPlatform2().IsEmpty())
		{
			AfxMessageBox (IDS_MSG_UNKNOWN_PLATFORM);
			pDlgMain->HideFrame(SW_HIDE);
			return FALSE;
		}
		pDlgMain->UpdateDifferences(nMask);
	}
	catch (...)
	{
		while (!ls1.IsEmpty())
			delete ls1.RemoveHead();
		while (!ls2.IsEmpty())
			delete ls2.RemoveHead();
		pDlgMain->HideFrame(SW_HIDE);
		return FALSE;
	}
	pDlgMain->m_listMainParam.SetFocus();
	return TRUE;
}

void VCDA_ExtractCommon(LPCTSTR lpText, int nCount, CString& left, CString& right)
{
	CString str = lpText;
	left = _T("");
	right = _T("");
	for (int i=0; i<nCount; i++)
	{
		int nFound = str.Find(_T('.'));
		if (nFound != -1)
		{
			left += str.Left(nFound +1);
			str = str.Mid(nFound +1);
		}
	}
	right = str;
	left.TrimLeft();
	right.TrimLeft();
}

static CaCda* Search (CaCda* pData, CTypedPtrList< CObList, CaCda* >& ls, CaCompareParam* pInfo, BOOL bRemove)
{
	CString strCouple;
	BOOL bSubstituteUser = FALSE;
	for (int i=0; i<2; i++)
	{
		CString strTemplate1 = pData->GetName2(pInfo, 1, bSubstituteUser); // From first snap-shot
		POSITION p = NULL, pos = ls.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaCda* pExist = ls.GetNext(pos);
			if (pExist->GetType() == pData->GetType())
			{
				CString strTemplate2 = pExist->GetName2(pInfo, 2, bSubstituteUser); // From second snap-shot
				if (strTemplate1.Compare(strTemplate2) == 0)
				{
					if (bRemove)
						ls.RemoveAt(p);
					return pExist;
				}
			}
		}

		if (strTemplate1.Find(_T(".privileges.user.")) == -1)
			break;
		bSubstituteUser = TRUE;
	}
	return NULL;
}

static CaCda* SearchVNode (CaCda* pData, CTypedPtrList< CObList, CaCda* >& ls, BOOL bRemove, BOOL bName)
{
	CString strCouple;
	CString strTemplate1 = bName? pData->GetName(): pData->GetHost(); 
	POSITION p = NULL, pos = ls.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaCda* pExist = ls.GetNext(pos);
		if (pExist->GetType() == pData->GetType())
		{
			CString strTemplate2 = bName? pExist->GetName(): pExist->GetHost();
			if (strTemplate1.Compare(strTemplate2) == 0)
			{
				if (bRemove)
					ls.RemoveAt(p);
				return pExist;
			}
		}
	}
	return NULL;
}

static void CompareGeneralParam (
	CaCda* pObj1, 
	CaCda* pObj2, 
	CaCompareParam* pParam, 
	CTypedPtrList< CObList, CaCdaDifference* >& ldiff)
{
	CaCdaDifference* pDiff = NULL;
	CString strName = pObj1->GetName();
	strName.MakeLower();
	if (strName.CompareNoCase(_T("PLATFORM")) == 0)
		return;

	if (pObj1->GetValue().CompareNoCase(pObj2->GetValue()) != 0)
	{
		pDiff = new CaCdaDifference(
			strName, 
			pObj1->GetValue(), 
			pObj2->GetValue(), 
			pObj1->GetType(), 
			_T('!'), 
			pObj1->GetHost());
		ldiff.AddTail(pDiff);
	}

}

static void ManagePath (CaCompareParam* pParam, CString& strValue1, CString& strValue2)
{
	BOOL bNormalise = TRUE;
	int nRp1 = 0;
	int nRp2 = 0;
	if (bNormalise)
	{
		NormalizePath (strValue1, pParam, 1);
		NormalizePath (strValue2, pParam, 2);
		nRp1 = strValue1.Replace(_T('/'), _T('\\'));
		nRp2 = strValue2.Replace(_T('/'), _T('\\'));
	}

	BOOL bDiff = FALSE;
	CString s1 = strValue1;
	CString s2 = strValue2;
	if (bNormalise)
	{
		s1.TrimLeft ('\'');
		s1.TrimRight('\'');
		s2.TrimLeft ('\'');
		s2.TrimRight('\'');
	}

	strValue1 = s1;
	strValue2 = s2;
}

static void CompareConfigParam (
	CaCda* pObj1, 
	CaCda* pObj2, 
	CaCompareParam* pParam, 
	CTypedPtrList< CObList, CaCdaDifference* >& ldiff)
{
	CString strIISystem;
	CString strInstallationID;
	CString strUser1 = pParam->GetUser1();
	CString strUser2 = pParam->GetUser2();
	CString strMainHost1 = pParam->GetHost1();
	CString strName1 = pObj1->GetName();
	CString strName2 = pObj2->GetName();
	CString strGenericHost = _T("<host name>");
	CaCdaDifference* pDiff = NULL;
	//
	// Convert to lover case before comparing:
	strName1.MakeLower();
	strName2.MakeLower();
	strMainHost1.MakeLower();
	if (pParam->WhatSubstituteHost() != 0)
		strGenericHost.Format (_T("<%s / %s>"), (LPCTSTR)pParam->GetHost1(), (LPCTSTR)pParam->GetHost2());

	BOOL bManagePath = FALSE;
	int nFlag = 0;
	CString strRightPart = pObj1->GetRight();
	CString strValue1 = pObj1->GetValue();
	CString strValue2 = pObj2->GetValue();

	if (VCDA_ConfigHasHostName(strRightPart)) // The value has host name
	{
		if (pParam->IsIgnore(_T("host name")))
		{
			if (strValue1.CompareNoCase(pParam->GetHost1()) == 0 && strValue2.CompareNoCase(pParam->GetHost2()) == 0)
			{
				strValue1 = _T("<host name>");
				strValue2 = _T("<host name>");
			}
		}
	}
	else
	if (VCDA_ConfigHasII(strRightPart, nFlag)) // The value has Installation ID
	{
		if (pParam->IsIgnore(_T("ii_installation")))
		{
			if (nFlag == 1) // II_GCC$(II_INSTALLATION)_0
			{
				CString strRep1, strRep2;
				strRep1.Format(_T("_GCC%s_"), (LPCTSTR)pParam->GetInstallation1());
				strRep2.Format(_T("_GCC%s_"), (LPCTSTR)pParam->GetInstallation2());

				if (strValue1.Find(strRep1) !=-1 && strValue2.Find(strRep2) != -1)
				{
					strValue1.Replace(strRep1, _T("_GCC<installation id>_"));
					strValue2.Replace(strRep2, _T("_GCC<installation id>_"));
				}
			}
			else
			{
				if (strValue1.Find(pParam->GetInstallation1()) !=-1 && strValue2.Find(pParam->GetInstallation2()) != -1)
				{
					strValue1.Replace(pParam->GetInstallation1(), _T("<installation id>"));
					strValue2.Replace(pParam->GetInstallation2(), _T("<installation id>"));
				}
			}
		}
	}
	else
	if (VCDA_ConfigHasPath(strRightPart)) // The value has path
	{
		if (pParam->IsIgnore(_T("ii_system")))
		{
			ManagePath (pParam, strValue1, strValue2);
			bManagePath = TRUE;
		}
	}


	if (pObj1->GetLeft().CompareNoCase (_T("ii.*.")) != 0)
	{
		if (pParam->IsIgnore(_T("host name")))
		{
			CString strItem = _T("ii.<hostname>.");
			strItem += pObj1->GetRight();
			strName1 = strItem;
		}
	}

	if (pObj2->GetLeft().CompareNoCase (_T("ii.*.")) != 0)
	{
		if (pParam->IsIgnore(_T("host name")))
		{
			CString strItem = _T("ii.<hostname>.");
			strItem += pObj2->GetRight();
			strName2 = strItem;
		}
	}

	if (pParam->IsIgnore(_T("user")))
	{
		int nPos = strName1.Find (_T("privileges.user."));

		if (nPos != -1)
		{
			CString strR = strName1.Mid (nPos + _tcslen(_T("privileges.user.")));
			CString strL = strName1.Left(nPos + _tcslen(_T("privileges.user.")));

			if (strR.CompareNoCase(strUser1) == 0)
			{
				strL += _T("<user name>");
				strName1 = strL;
			}
		}

		nPos = strName2.Find (_T("privileges.user."));
		if (nPos != -1)
		{
			CString strR = strName2.Mid (nPos + _tcslen(_T("privileges.user.")));
			CString strL = strName2.Left(nPos + _tcslen(_T("privileges.user.")));

			if (strR.CompareNoCase(strUser2) == 0)
			{
				strL += _T("<user name>");
				strName2 = strL;
			}
		}
	}

	//
	// Difference in value ?
	if (bManagePath)
	{
		BOOL bDiff = FALSE;
		if (pParam->GetPlatform1() == pParam->GetPlatform2() && pParam->GetPlatform1().CompareNoCase(_T("UNIX")) == 0)
			bDiff = (strValue1 != strValue2);
		else
			bDiff = (strValue1.CompareNoCase(strValue2) != 0);
		if (bDiff)
		{
			pDiff = new CaCdaDifference(strName1, strValue1, strValue2, pObj1->GetType(), _T('!'), pObj1->GetHost());
			ldiff.AddTail(pDiff);
		}
	}
	else
	if (strName1.CompareNoCase(strName2) == 0 && strValue1.CompareNoCase(strValue2) != 0)
	{
		if (strName1.Find(_T("<hostname>")) != -1 && pParam->WhatSubstituteHost() != 0)
			strName1.Replace(_T("<hostname>"), strGenericHost);

		pDiff = new CaCdaDifference(strName1, strValue1, strValue2, pObj1->GetType(), _T('!'), pObj1->GetHost());
		ldiff.AddTail(pDiff);
	}
}

static void CompareEnvironmentParam (
	CaCda* pObj1, 
	CaCda* pObj2, 
	CaCompareParam* pParam, 
	CTypedPtrList< CObList, CaCdaDifference* >& ldiff)
{
	BOOL bChecked = FALSE;
	CString strValue1 = pObj1->GetValue();
	CString strValue2 = pObj2->GetValue();
	CString strEnv = pObj1->GetName();
	BOOL bManagePath = FALSE;
	CaCdaDifference* pDiff = NULL;

	if (CaIngresVariable::IsPath(strEnv))
	{
		if (pParam->IsIgnore(_T("ii_system")))
		{
			ManagePath (pParam, strValue1, strValue2);
			bManagePath = TRUE;
		}
	}

	if (pParam->IsIgnore(_T("ii_installation")))
	{
		CString strTest;
		CStringList& listIIDependent = pParam->GetListIIDependent();
		POSITION pos = listIIDependent.GetHeadPosition();
		while (pos != NULL)
		{
			CString strFmt = listIIDependent.GetNext(pos);
			strTest.Format(strFmt, (LPCTSTR)pParam->GetInstallation1());
			if (strTest == pObj1->GetName())
			{
				strTest.Format(strFmt, _T("<installation id>"));
				strEnv = strTest;
				bChecked = TRUE;
				break;
			}
		}

		if (bChecked)
		{
			if (strValue1.CompareNoCase(pParam->GetInstallation1()) == 0 && strValue2.CompareNoCase(pParam->GetInstallation2()) == 0)
			{
				strValue1 = _T("<installation id>");
				strValue2 = _T("<installation id>");
			}
			else
			if (pParam->IsIgnore(_T("host name")))
			{
				if (strValue1.CompareNoCase(pParam->GetHost1()) == 0 && strValue2.CompareNoCase(pParam->GetHost2()) == 0)
				{
					strValue1 = _T("<host name>");
					strValue2 = _T("<host name>");
				}
			}
		}
	}

	BOOL bDiff = FALSE;
	if (bManagePath)
	{
		if (pParam->GetPlatform1() == pParam->GetPlatform2() && pParam->GetPlatform1().CompareNoCase(_T("UNIX")) == 0)
			bDiff = (strValue1 != strValue2);
		else
			bDiff = (strValue1.CompareNoCase(strValue2) != 0);
	}
	else
	{
		bDiff = (strValue1 !=  strValue2);
	}

	if (bDiff)
	{
		pDiff = new CaCdaDifference(
			strEnv, 
			strValue1, 
			strValue2, 
			pObj1->GetType(), 
			_T('!'), 
			pObj1->GetHost(), 
			pObj1->GetName());
		ldiff.AddTail(pDiff);
	}
}

static void CompareVirtualNodeParam (
	CaCda* pObj1, 
	CaCda* pObj2, 
	CaCompareParam* pParam, 
	CTypedPtrList< CObList, 
	CaCdaDifference* >& ldiff)
{
	CString strValue1 = pObj1->GetValue();
	CString strValue2 = pObj2->GetValue();
	CString strName = pObj1->GetName();
	CaCdaDifference* pDiff = NULL;

	BOOL bDiff = FALSE;
	bDiff = (strValue1.CompareNoCase(strValue2) != 0);
	if (bDiff)
	{
		pDiff = new CaCdaDifference(
			strName, 
			strValue1, 
			strValue2, 
			pObj1->GetType(), 
			_T('!'), 
			pObj1->GetHost(), 
			pObj1->GetName());
		ldiff.AddTail(pDiff);
	}
}


void VCDA_CompareList(
    CTypedPtrList< CObList, CaCda* >& ls1, 
    CTypedPtrList< CObList, CaCda* >& ls2, 
    CTypedPtrList< CObList, CaCdaDifference* >& ldiff,
    CaCompareParam* pParam)
{
	CaCdaDifference* pDiff = NULL;
	CTypedPtrList< CObList, CaCda* > lplus;  // Contains all elements that exist in ls1 but not exist in ls2
	CTypedPtrList< CObList, CaCda* > lminus; // Contains all elements that exist in ls2 but not exist in ls1

	POSITION p = NULL, pos = ls1.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = ls1.GetNext(pos);
		lplus.AddTail(pObj);
	}
	pos = ls2.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = ls2.GetNext(pos);
		lminus.AddTail(pObj);
	}

	CaCda* pExist = NULL;
	pos = lplus.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaCda* pData = lplus.GetNext(pos);
		cdaType nType = pData->GetType();
		//
		// Search element of ltemp1 in ltemp2:
		pExist = Search (pData, lminus, pParam, TRUE);
		if (pExist)
		{
			lplus.RemoveAt(p);

			switch (nType)
			{
			case CDA_GENERAL:
				CompareGeneralParam (pData, pExist, pParam, ldiff);
				break;
			case CDA_CONFIG:
				CompareConfigParam (pData, pExist, pParam, ldiff);
				break;
			case CDA_ENVSYSTEM:
			case CDA_ENVUSER:
				CompareEnvironmentParam (pData, pExist, pParam, ldiff);
				break;
			case CDA_VNODE:
				ASSERT(FALSE);
				break;
			default:
				ASSERT(FALSE);
				break;
			}
		}
	}

	pos = lplus.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pData = lplus.GetNext(pos);
		cdaType nType = pData->GetType();
		pDiff = new CaCdaDifference(pData->GetName(), pData->GetValue(), _T(""), nType, _T('+'), pData->GetHost());
		ldiff.AddTail(pDiff);
	}
	pos = lminus.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pData = lminus.GetNext(pos);
		cdaType nType = pData->GetType();
		pDiff = new CaCdaDifference(pData->GetName(), _T(""), pData->GetValue(), nType, _T('-'), pData->GetHost());
		ldiff.AddTail(pDiff);
	}
}

static void RemoveHosts(CTypedPtrList< CObList, CaCda* >& ls, LPCTSTR lpszHost)
{
	POSITION p, pos = ls.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaCda* pData = ls.GetNext(pos);
		if (pData->GetHost().CompareNoCase(lpszHost) == 0)
		{
			ls.RemoveAt(p);
		}
	}
}

void VCDA_CompareListVNode(
    CTypedPtrList< CObList, CaCda* >& ls1, 
    CTypedPtrList< CObList, CaCda* >& ls2, 
    CTypedPtrList< CObList, CaCdaDifference* >& ldiff,
    CaCompareParam* pParam)
{
	CaCdaDifference* pDiff = NULL;
	CaCda* pExist = NULL;
	CTypedPtrList< CObList, CaCda* > lt1; 
	CTypedPtrList< CObList, CaCda* > lt2; 
	POSITION p = NULL, pos = ls1.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = ls1.GetNext(pos);
		lt1.AddTail(pObj);
	}
	pos = ls2.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = ls2.GetNext(pos);
		lt2.AddTail(pObj);
	}

	pos = lt1.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pData = lt1.GetNext(pos);
		pExist = SearchVNode (pData, lt2, FALSE);
		if (!pExist)
		{
			pDiff = new CaCdaDifference(
				_T("Virtual Node Name"), 
				pData->GetHost(), 
				_T(""), 
				pData->GetType(), 
				_T('+'), 
				pData->GetHost());
			ldiff.AddTail(pDiff);
			//
			// Remove all objects from lt1 where object->GetHost() == pData->GetHost()
			RemoveHosts(lt1, pData->GetHost());
			pos = lt1.GetHeadPosition();
			continue;
		}
	}
	pos = lt2.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pData = lt2.GetNext(pos);
		pExist = SearchVNode (pData, lt1, FALSE);
		if (!pExist)
		{
			pDiff = new CaCdaDifference(
				_T("Virtual Node Name"),
				_T(""), 
				pData->GetHost(), 
				pData->GetType(), 
				_T('-'), 
				pData->GetHost());
			ldiff.AddTail(pDiff);
			//
			// Remove all objects from lt2 where object->GetHost() == pData->GetHost()
			RemoveHosts(lt2, pData->GetHost());
			pos = lt2.GetHeadPosition();
			continue;
		}
	}

	pos = lt1.GetHeadPosition();
	while (pos != NULL)
	{
		p = pos;
		CaCda* pData = lt1.GetNext(pos);
		cdaType nType = pData->GetType();
		//
		// Search element of lt1 in lt2:
		pExist = SearchVNode (pData, lt2, TRUE, TRUE);
		if (pExist)
		{
			lt1.RemoveAt(p);
			switch (nType)
			{
			case CDA_VNODE:
				CompareVirtualNodeParam (pData, pExist, pParam, ldiff);
				break;
			default:
				ASSERT(FALSE);
				break;
			}
		}
	}

	pos = lt1.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pData = lt1.GetNext(pos);
		cdaType nType = pData->GetType();
		pDiff = new CaCdaDifference(
			pData->GetName(), 
			pData->GetValue(), 
			_T(""), 
			nType, 
			_T('+'), 
			pData->GetHost());
		ldiff.AddTail(pDiff);
	}
	pos = lt2.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pData = lt2.GetNext(pos);
		cdaType nType = pData->GetType();
		pDiff = new CaCdaDifference(
			pData->GetName(), 
			_T(""), 
			pData->GetValue(), 
			nType, 
			_T('-'), 
			pData->GetHost());
		ldiff.AddTail(pDiff);
	}
}


static BOOL MatchHost(CaCda* pObj, CTypedPtrList< CObList, CaMappedHost* >& listMappedHost, int nSnapShot)
{
	POSITION pos = listMappedHost.GetHeadPosition();
	while (pos != NULL)
	{
		CaMappedHost* pMapped = listMappedHost.GetNext(pos);
		CString strMH = (nSnapShot==1)? pMapped->GetHost1(): pMapped->GetHost2();
		if (pObj->GetHost().CompareNoCase(strMH) == 0)
			return TRUE;
	}
	return FALSE;
}

void VCDA_CompareList2(
    CTypedPtrList< CObList, CaCda* >& ls1, 
    CTypedPtrList< CObList, CaCda* >& ls2,
    CTypedPtrList< CObList, CaMappedHost* >& listMappedHost,
    CTypedPtrList< CObList, CaCdaDifference* >& ldiff,
    CaCompareParam* pParam)
{
	CaCdaDifference* pDiff = NULL;
	CTypedPtrList< CObList, CaCda* > lplus;  // Contains all elements that exist in ls1 but not exist in ls2
	CTypedPtrList< CObList, CaCda* > lminus; // Contains all elements that exist in ls2 but not exist in ls1

	POSITION p = NULL, pos = ls1.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = ls1.GetNext(pos);
		if (MatchHost(pObj, listMappedHost, 1))
			lplus.AddTail(pObj);
	}
	pos = ls2.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = ls2.GetNext(pos);
		if (MatchHost(pObj, listMappedHost, 2))
			lminus.AddTail(pObj);
	}
	CTypedPtrList< CObList, CaMappedHost* > lmap;
	pos = listMappedHost.GetHeadPosition();
	while (pos != NULL)
	{
		CaMappedHost* pObj = listMappedHost.GetNext(pos);
		lmap.AddTail(pObj);
	}

	CaCda* pExist = NULL;
	while (!lmap.IsEmpty())
	{
		CaMappedHost* pMappedHost = lmap.RemoveHead();
		pParam->SetHosts(pMappedHost->GetHost1(), pMappedHost->GetHost2());

		pos = lplus.GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaCda* pData = lplus.GetNext(pos);
			cdaType nType = pData->GetType();
			//
			// Search element of ltemp1 in ltemp2:
			pExist = Search (pData, lminus, pParam, TRUE);
			if (pExist)
			{
				lplus.RemoveAt(p);

				switch (nType)
				{
				case CDA_CONFIG:
					CompareConfigParam (pData, pExist, pParam, ldiff);
					break;
				default:
					ASSERT(FALSE);
					break;
				}
			}
		}
	}

	pos = lplus.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pData = lplus.GetNext(pos);
		cdaType nType = pData->GetType();
		pDiff = new CaCdaDifference(pData->GetName(), pData->GetValue(), _T(""), nType, _T('+'), pData->GetHost());
		ldiff.AddTail(pDiff);
	}
	pos = lminus.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pData = lminus.GetNext(pos);
		cdaType nType = pData->GetType();
		pDiff = new CaCdaDifference(pData->GetName(), _T(""), pData->GetValue(), nType, _T('-'), pData->GetHost());
		ldiff.AddTail(pDiff);
	}
}

//
// If bCurrentInstallation = TRUE, then lpszFile1 is a config.dat
// lpszFile2 is always a snapshot file
void VCDA_CheckInconsistency(
    CTypedPtrList< CObList, CaCda* >& ls1, 
    CTypedPtrList< CObList, CaCda* >& ls2,
    BOOL bCurrentInstallation,
    LPCTSTR lpszFile1,
    LPCTSTR lpszFile2)
{
	BOOL bHasInconsistency[2] = {FALSE, FALSE};
#if !defined (_CHECK_INCONSISTENCY)
	return;
#endif
	CTypedPtrList< CObList, CaCda* >* ls [2] = {&ls1,    &ls2};
	for (int i=0; i<2; i++)
	{
		POSITION p, pos = ls[i]->GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaCda* pData = ls[i]->GetNext(pos);

			POSITION pCur= pos;
			while (pCur != NULL)
			{
				CaCda* pCurData = ls[i]->GetAt(pCur);
				cdaType nType = pCurData->GetType();
				if (pData->GetType() == nType && nType == CDA_CONFIG)
				{
					CString strN1 = pData->GetName();
					CString strN2 = pCurData->GetName();
					if (strN1 == strN2)
					{
						ls[i]->RemoveAt(p);
						delete pData;
						bHasInconsistency[i] = TRUE;
						break;
					}
				}
				ls[i]->GetNext(pCur);
			}
		}
	}

	if (bHasInconsistency[0])
	{
		if (bCurrentInstallation)
		{
			//
			// Check if ingres is running, if so, issue the message that
			// vcda cannot modify file config.dat to be consistent while ingres
			// is running.
			BOOL bIngresRunning = INGRESII_IsRunning();
			if( bIngresRunning)
			{
				// _T("Vcda detects that the config.dat of the current installation is inconsistent.\n"
				//    "Therefore, vcda cannot make it consistent while ingres is running.");
				AfxMessageBox (IDS_MSG_CONFIG_INCONSISTENTxINGRES);
			}
			else
			{
				// _T("Vcda detects that the config.dat is inconsistent.\nDo you wish vcda make the file consistent?")
				int nAnswer = AfxMessageBox(IDS_MSG_CONFIG_INCONSISTENT, MB_YESNO|MB_ICONQUESTION);
				if (nAnswer == IDYES)
				{
					TRACE0("Make config.dat consistent.\n");
					VCDA_MakeFileConsistent (ls1, lpszFile1, bCurrentInstallation);
				}
			}
		}
		else
		{
			CString strMsg; // _T("Vcda detects that the snapshot: '%1' is inconsistent.\nDo you wish vcda make the file consistent?")
			AfxFormatString1(strMsg, IDS_MSG_SNAPSHOT_INCONSISTENT, lpszFile1);
			int nAnswer = AfxMessageBox(strMsg, MB_YESNO|MB_ICONQUESTION);
			if (nAnswer == IDYES)
			{
				TRACE0("Make snapshot1 consistent.\n");
				VCDA_MakeFileConsistent (ls1, lpszFile1, bCurrentInstallation);
			}
		}
	}
	if (bHasInconsistency[1])
	{
		CString strMsg; // _T("Vcda detects that the snapshot: '%1' is inconsistent.\nDo you wish vcda make the file consistent?")
		AfxFormatString1(strMsg, IDS_MSG_SNAPSHOT_INCONSISTENT, lpszFile2);
		AfxMessageBox(strMsg);
	}
}


//
// Make file consistent:
// The new content of file is restored from the list 'ls'
// If bCurrentInstallation = TRUE, then lpszFile is a config.dat 
// else lpszFile is a snapshot file
void VCDA_MakeFileConsistent(CTypedPtrList< CObList, CaCda* >& ls, LPCTSTR lpszFile, BOOL bCurrentInstallation)
{
	POSITION pos = ls.GetHeadPosition();
	if (bCurrentInstallation)
	{
		FILE* pFile = _tfopen(lpszFile, _T("w"));
		if (!pFile)
		{
			AfxMessageBox (IDS_MSG_FAIL_2_CREATE_SNAPSHOT);
			return; // Cannot create file
		}

		while (pos != NULL)
		{
			CaCda* pData = ls.GetNext(pos);
			if (pData->GetType() == CDA_CONFIG)
			{
				sWriteTo(pFile, pData->GetName(), pData->GetValue());
			}
		}
		fclose(pFile);
	}
	else
	{
		//
		// Vcda does not propose to make the snapshot file consistent.
	}
}

BOOL VCDA_GetIngresVersionIDxPatches (CString& strIngresVersion, CString& strIngresPatches)
{
	strIngresVersion = _T("");
	strIngresPatches = _T("");
#if defined (MAINWIN)
	CString strFile = _T("/ingres/version.rel");
#else
	CString strFile = _T("\\ingres\\version.rel");
#endif
	TCHAR* pEnv = _tgetenv(_T("II_SYSTEM"));
	CString strVersionFile = pEnv;
	strVersionFile += strFile;

	FILE* pFileIn = _tfopen(strVersionFile, _T("r"));
	if (pFileIn)
	{
		TCHAR tchszBuff [512];
		if (_fgetts (tchszBuff, 500, pFileIn) != NULL)
		{
			RemoveEndingCRLF(tchszBuff);
			CString str = tchszBuff;
			if (str.Find(_T("II")) == 0)
				strIngresVersion = str;
			if (!feof(pFileIn))
			{
				BOOL bOne = TRUE;
				while (_fgetts (tchszBuff, 500, pFileIn) != NULL)
				{
					RemoveEndingCRLF(tchszBuff);
					if (tchszBuff[0])
					{
						if (!bOne)
							strIngresPatches += _T(", ");
						strIngresPatches += tchszBuff;
						bOne = FALSE;
					}
					if (feof(pFileIn) != 0)
						break;
				}
			}
		}
		fclose(pFileIn);
	}

	return TRUE;
}

BOOL VCDA_ConstructListRestoreParam (CaRestoreParam* pRestore)
{
	CString strII1 = INGRESII_QueryInstallationID(FALSE);
	CString strII2 = _T("");
	POSITION pos = NULL;
	CTypedPtrList< CObList, CaCda* >& listResore = pRestore->m_listRestoreParam;
	CTypedPtrList< CObList, CaCdaCouple* >& listSubstituteInfo = pRestore->m_listSubStituteInfo;

	BOOL bResorePath = pRestore->GetPath();
	BOOL bDiffII = FALSE;
	BOOL bDiffHost = FALSE;
	CTypedPtrList< CObList, CaCdaDifference* >* ldiff = pRestore->m_plistDifference;
	pos = ldiff->GetHeadPosition();
	while (pos != NULL)
	{
		CaCdaDifference* pObj = ldiff->GetNext (pos);
		if (pObj->GetType() != CDA_GENERAL)
			break;
		if (pObj->GetName().CompareNoCase(_T("HOST NAME")) == 0)
			bDiffHost = TRUE;
		else
		if (pObj->GetName().CompareNoCase(_T("II_INSTALLATION")) == 0)
		{
			bDiffII = TRUE;
			strII1  = pObj->GetValue1();
			strII2  = pObj->GetValue2();
		}
	}

	// 
	// Construct the list of configuration parameters to be restored:
	CString strHost1 = GetLocalHostName();
	CString strHost2 = _T("???");
	CString strValue = _T("");
	CTypedPtrList< CObList, CaCda* >* lc1 = pRestore->m_plc1;
	CTypedPtrList< CObList, CaCda* >* lc2 = pRestore->m_plc2;
	strHost1.MakeLower();
	/*
	pos = lc1->GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = lc1->GetNext (pos);
		if (!pObj->GetHost().IsEmpty())
		{
			strHost1 = pObj->GetHost();
			break;
		}
	}
	*/
	pos = lc2->GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = lc2->GetNext (pos);
		if (!pObj->GetHost().IsEmpty())
		{
			strHost2 = pObj->GetHost();
			break;
		}
	}
	if (pRestore->GetConfig())
	{
		pos = lc2->GetHeadPosition();
		while (pos != NULL)
		{
			CaCda* pObj = lc2->GetNext (pos);
			strValue = pObj->GetValue();
			if (!bResorePath)
			{
				BOOL bHasPath = VCDA_ConfigHasPath(pObj->GetRight());
				if (bHasPath)
				{
					//
					// Find the same line in the current config.dat and keep its value!
					CaCda* pFound = CaCda::Find(pObj->GetRight(), lc1);
					if (pFound)
					{
						CaCda* pNew = new CaCda(*pFound); // pFound is an element from snapshot file
						listResore.AddTail(pNew);
						continue;
					}
				}
			}

			BOOL bSubstitute = FALSE;
			if (bDiffHost || bDiffII)
			{
				int nFlag = 0;
				if (bDiffHost && VCDA_ConfigHasHostName(pObj->GetRight()))
				{
					CaCda* pFound = CaCda::Find(pObj->GetRight(), lc1);
					if (pFound)
					{
						CaCda* pNew = new CaCda(*pFound);
						listResore.AddTail(pNew);

						listSubstituteInfo.AddTail(new CaCdaCouple(pObj, pNew));
						continue;
					}
					else
					{
						strValue.Replace(strHost2 ,strHost1);
						bSubstitute = TRUE;
					}
				}
				else
				if (bDiffII && VCDA_ConfigHasII(pObj->GetRight(), nFlag))
				{
					CaCda* pFound = CaCda::Find(pObj->GetRight(), lc1);
					if (pFound)
					{
						CaCda* pNew = new CaCda(*pFound);
						listResore.AddTail(pNew);

						listSubstituteInfo.AddTail(new CaCdaCouple(pObj, pNew));
						continue;
					}
					else
					{
						if (nFlag == 1) // (II_GCCxx_0 or II_GCDxx7)
						{
							CString strPart1, strPart2;
							strPart1 = strValue.Left(6);
							strPart2 = strValue.Mid(6);
							strPart2.Replace(strII2 ,strII1);
							strValue = strPart1 + strPart2;
						}
						else
						if (nFlag == 2) // OSLAN_xx_7 or OSLAN_xx
						{
							CString strPart1, strPart2;
							strPart1 = strValue.Left(5);
							strPart2 = strValue.Mid(5);
							strPart2.Replace(strII2 ,strII1);
							strValue = strPart1 + strPart2;
						}
						else
						if (nFlag == 3) // X25_xx_7 or X25_xx
						{
							CString strPart1, strPart2;
							strPart1 = strValue.Left(3);
							strPart2 = strValue.Mid(3);
							strPart2.Replace(strII2 ,strII1);
							strValue = strPart1 + strPart2;
						}
						else
						{
							strValue.Replace(strII2 ,strII1);
						}
						bSubstitute = TRUE;
					}
				}
			}

			CString strHost = _T("");
			CString strLeft = pObj->GetLeft();
			if (!pObj->GetHost().IsEmpty())
			{
				strHost = strHost1;
				strLeft.Format (_T("ii.%s."), (LPCTSTR)strHost);
			}

			CaCda* pNew = new CaCda(strHost, strLeft, pObj->GetRight(), strValue, pObj->GetType());
			listResore.AddTail(pNew);

			if (bSubstitute)
				listSubstituteInfo.AddTail(new CaCdaCouple(pObj, pNew));
		}
	}

	// 
	// Construct the list of System parameters to be restored:
	if (pRestore->GetSystemVariable())
	{
		pos = ldiff->GetHeadPosition();
		while (pos != NULL)
		{
			CaCdaDifference* pObj = ldiff->GetNext (pos);
			if (pObj->GetType() != CDA_ENVSYSTEM)
				continue;
			if (CaIngresVariable::IsReadOnly(pObj->GetOriginalName(), strII1))
				continue;
			if (!bResorePath && CaIngresVariable::IsPath(pObj->GetOriginalName()))
				continue;

			CString strFormatName;
			int nDep = CaIngresVariable::IsIIDependent(pObj->GetOriginalName(), strII1, strFormatName);
			switch (nDep)
			{
			case 1:
				if (strFormatName.CompareNoCase(_T("II_GCN%s_LCL_VNODE")) == 0)
				{
					if (pObj->GetValue2().CompareNoCase(strHost2) != 0)
					{
						CString strEnv;
						strEnv.Format(strFormatName, (LPCTSTR)strII2);
						CaCda* pFound = CaCda::Find(strEnv, pRestore->m_ples2);
						if (pFound)
						{
							CaCda* pNew = new CaCda(pObj->GetOriginalName(), pObj->GetValue2(), pObj->GetType());
							listResore.AddTail(pNew);

							listSubstituteInfo.AddTail(new CaCdaCouple(pFound, pNew));
						}
					}
					continue;
				}
				else
				{
					CaCda* pFound = CaCda::Find(pObj->GetOriginalName(), pRestore->m_ples1);
					if (pFound)
					{
						CaCda* pNew = new CaCda(pObj->GetOriginalName(), pObj->GetValue2(), pObj->GetType());
						listResore.AddTail(pNew);
						continue;
						// listSubstituteInfo.AddTail(new CaCdaCouple(pFound, pNew));
					}
					else
					{

					}
				}
				break;
			case 2: // The value depends on the II_INSTALLATION
				continue;
				break;
			default:
				{
					CaCda* pFound = CaCda::Find(pObj->GetOriginalName(), pRestore->m_ples1);
					if (pFound)
					{
						CaCda* pNew = new CaCda(pObj->GetOriginalName(), pObj->GetValue2(), pObj->GetType());
						listResore.AddTail(pNew);
						continue;
						// listSubstituteInfo.AddTail(new CaCdaCouple(pFound, pNew));
					}
					else
					{

					}
				}
			}

			CaCda* pNew = new CaCda(pObj->GetOriginalName(), pObj->GetValue2(), pObj->GetType());
			listResore.AddTail(pNew);
		}
	}

	// 
	// Construct the list of User parameters to be restored:
	if (pRestore->GetUserVariable())
	{
#if !defined (MAINWIN) // Under MAINWIN these variables are readonly
		pos = ldiff->GetHeadPosition();
		while (pos != NULL)
		{
			CaCdaDifference* pObj = ldiff->GetNext (pos);
			if (pObj->GetType() != CDA_ENVUSER)
				continue;
			if (CaIngresVariable::IsReadOnly(pObj->GetOriginalName(), strII1))
				continue;
			if (!bResorePath && CaIngresVariable::IsPath(pObj->GetOriginalName()))
				continue;

			CaCda* pNew = new CaCda(pObj->GetOriginalName(), pObj->GetValue2(), pObj->GetType());
			listResore.AddTail(pNew);
		}
#endif
	}

	// 
	// Construct the list of Virtual Node parameters to be restored:
	// (We only sort the different items of vnode)
	if (pRestore->GetVirtualNode())
	{
		int nNodeSize = 0;
		int nCount = pRestore->m_plvn1->GetCount() + pRestore->m_plvn2->GetCount();
		if (nCount == 0)
			nCount = 1;
		UINT* pArray = new UINT[nCount];
		for (int i=0; i<nCount; i++)
			pArray[i] = 0;

		POSITION p;
		pos = ldiff->GetHeadPosition();
		while (pos != NULL)
		{
			p = pos;
			CaCdaDifference* pObj = ldiff->GetNext (pos);
			if (pObj->GetType() != CDA_VNODE)
				continue;
			pArray[nNodeSize] = (UINT)pObj;
			nNodeSize++;
			ldiff->RemoveAt(p);
		}
		qsort ((void*)pArray, (size_t)nNodeSize, (size_t)sizeof(UINT), compareVnodeDiff);
		for (int i=0; i<nNodeSize; i++)
		{
			ldiff->AddTail((CaCdaDifference*)pArray[i]);
		}

		if (pArray)
			delete pArray;
	}
	return TRUE;
}



static void RestoreVirtualNodes(CaRestoreParam* pRestore)
{
	CString strIPAdr;
	CString strProtocol;
	CString strListenAdr;
	CTypedPtrList< CObList, CaCdaDifference* >* ldiff = pRestore->m_plistDifference;
	// 
	// Construct the list of Virtual Node parameters to be restored:
	if (!pRestore->GetVirtualNode())
		return;
	int nNodeSize = 0;
	int nCount = max (pRestore->m_plvn1->GetCount(), pRestore->m_plvn2->GetCount());
	if (nCount == 0)
		nCount = 1;
	UINT* pArray = new UINT[nCount];
	for (int i=0; i<nCount; i++)
		pArray[i] = 0;

	CaRestoreVNode* pVnDef = NULL;
	CTypedPtrList < CObList, CaRestoreVNode* > listVNodeDef;
	POSITION pos = ldiff->GetHeadPosition();
	while (pos != NULL)
	{
		CaCdaDifference* pObj = ldiff->GetNext (pos);
		if (pObj->GetType() != CDA_VNODE)
			continue;

		BOOL b1 = FALSE;
		TCHAR ch = pObj->GetDifference();
		//
		// Check the characteristics of node envolving in the differences:
		if(pObj->GetName().CompareNoCase(_T("Virtual Node Name")) == 0)
		{
			switch (ch)
			{
			case _T('+'):
				// Drop Virtual Node pObj->GetValue1()
				pVnDef = new CaRestoreVNode(ch);
				pVnDef->m_pNewStruct = new CaNode(pObj->GetValue1());
				listVNodeDef.AddTail(pVnDef);
				break;
			case _T('-'):
				// Add Virtual Node pObj->GetValue2()
				// Note: we only display the name of the missing virtual node,
				//       so add this virtual node, we need to find all its characteristics - list of logins,
				//       list of connections, list of attributes:
				pVnDef = new CaRestoreVNode(ch);
				pVnDef->m_pNewStruct = new CaNode(pObj->GetValue2());
				if (pRestore->m_plvn2)
				{
					CString strNode = pObj->GetValue2();
					listVNodeDef.AddTail(pVnDef);
					CTypedPtrList< CObList, CaCda* >* pL = pRestore->m_plvn2;
					//
					// Sort the items of the VNode 'strNode':
					int nItemSize = 0;
					POSITION p = pL->GetHeadPosition();
					while (p != NULL)
					{
						CaCda* pItem = pL->GetNext(p);
						if (pItem->GetHost().CompareNoCase(strNode) == 0)
						{
							pArray[nItemSize] = (UINT)pItem;
							nItemSize++;
						}
					}
					qsort ((void*)pArray, (size_t)nItemSize, (size_t)sizeof(UINT), compareVnodeItem);

					BOOL bOne[2] = {TRUE, TRUE}; // login, connection
					for (int i=0; i<nItemSize; i++)
					{
						CaCda* pItem = (CaCda*)pArray[i];
						if (pItem->GetHost().CompareNoCase(strNode) == 0)
						{
							CString strTemplate = pItem->GetName();
							BOOL bPrivate = TRUE;
							if (strTemplate.Find(_T("Global")) != -1)
								bPrivate = FALSE;

							if (strTemplate.Find(_T("Login")) != -1) // login item
							{
								if (bOne[0])
								{
									CaNode* pVNode = (CaNode*)pVnDef->m_pNewStruct;
									pVNode->SetLogin(pItem->GetValue());
									pVNode->LoginPrivate(bPrivate);
								}
								else
								{
									CaRestoreVNode* pNewDef = new CaRestoreVNode(ch);
									pNewDef->m_pNewStruct = new CaNodeLogin(strNode, pItem->GetValue(), _T(""), bPrivate);
									listVNodeDef.AddTail(pNewDef);
								}
								bOne[0] = FALSE; 
							}
							else
							if (strTemplate.Find(_T("Connection")) != -1) // connection data item
							{
								ParseConnectionData(pItem->GetValue(), strIPAdr, strProtocol, strListenAdr);
								if (bOne[1])
								{
									CaNode* pVNode = (CaNode*)pVnDef->m_pNewStruct;
				
									pVNode->SetIPAddress(strIPAdr);
									pVNode->SetProtocol(strProtocol);
									pVNode->SetListenAddress(strListenAdr);
									pVNode->ConnectionPrivate(bPrivate);
								}
								else
								{
									CaRestoreVNode* pNewDef = new CaRestoreVNode(ch);
									pNewDef->m_pNewStruct = new CaNodeConnectData(strNode, strIPAdr, strProtocol, strListenAdr, bPrivate);
									listVNodeDef.AddTail(pNewDef);
								}
								bOne[1] = FALSE; 
							}
							else
							if (strTemplate.Find(_T("Attribute")) != -1) // attribute data item
							{
								CString strAttrName = _T("???");
								int nPos = strTemplate.Find(_T("Attribute "));
								if (nPos != -1)
									strAttrName = strTemplate.Mid (nPos + _tcslen (_T("Attribute ")));
								CaRestoreVNode* pNewDef = new CaRestoreVNode(ch);
								pNewDef->m_pNewStruct = new CaNodeAttribute(strNode, strAttrName, pItem->GetValue(), bPrivate);
								listVNodeDef.AddTail(pNewDef);
							}
						}
					}
				}
				break;
			default:
				break;
			}
		}
		else
		if ((b1 = pObj->GetName().Find(_T("Private Login")) != -1) || pObj->GetName().Find(_T("Global Login")) != -1)
		{
			BOOL bPrivate = b1? TRUE: FALSE;
			switch (ch)
			{
			case _T('+'):
				pVnDef = new CaRestoreVNode(ch);
				pVnDef->m_pNewStruct = new CaNodeLogin(pObj->GetHost(), pObj->GetValue1(), _T(""), bPrivate);
				listVNodeDef.AddTail(pVnDef);
				break;
			case _T('-'):
				pVnDef = new CaRestoreVNode(ch);
				pVnDef->m_pNewStruct = new CaNodeLogin(pObj->GetHost(), pObj->GetValue2(), _T(""), bPrivate);
				listVNodeDef.AddTail(pVnDef);
				break;
			case _T('!'):
				pVnDef = new CaRestoreVNode(ch);
				pVnDef->m_pOldStruct = new CaNodeLogin(pObj->GetHost(), pObj->GetValue1(), _T(""), bPrivate);
				pVnDef->m_pNewStruct = new CaNodeLogin(pObj->GetHost(), pObj->GetValue2(), _T(""), bPrivate);
				listVNodeDef.AddTail(pVnDef);
				break;
			default:
				break;
			}
		}
		else
		if ((b1 = pObj->GetName().Find(_T("Private Connection")) != -1) || pObj->GetName().Find(_T("Global Connection")) != -1)
		{
			BOOL bPrivate = b1? TRUE: FALSE;

			switch (ch)
			{
			case _T('+'):
				pVnDef = new CaRestoreVNode(ch);
				ParseConnectionData(pObj->GetValue1(), strIPAdr, strProtocol, strListenAdr);
				pVnDef->m_pNewStruct = new CaNodeConnectData(pObj->GetHost(), strIPAdr, strProtocol, strListenAdr, bPrivate);
				listVNodeDef.AddTail(pVnDef);
				break;
			case _T('-'):
				pVnDef = new CaRestoreVNode(ch);
				ParseConnectionData(pObj->GetValue2(), strIPAdr, strProtocol, strListenAdr);
				pVnDef->m_pNewStruct = new CaNodeConnectData(pObj->GetHost(), strIPAdr, strProtocol, strListenAdr, bPrivate);
				listVNodeDef.AddTail(pVnDef);
				break;
			case _T('!'):
				pVnDef = new CaRestoreVNode(ch);
				ParseConnectionData(pObj->GetValue1(), strIPAdr, strProtocol, strListenAdr);
				pVnDef->m_pOldStruct = new CaNodeConnectData(pObj->GetHost(), strIPAdr, strProtocol, strListenAdr, bPrivate);
				ParseConnectionData(pObj->GetValue2(), strIPAdr, strProtocol, strListenAdr);
				pVnDef->m_pNewStruct = new CaNodeConnectData(pObj->GetHost(), strIPAdr, strProtocol, strListenAdr, bPrivate);
				listVNodeDef.AddTail(pVnDef);
				break;
			default:
				break;
			}
		}
		else
		if ((b1 = pObj->GetName().Find(_T("Private Attribute")) != -1) || pObj->GetName().Find(_T("Global Attribute")) != -1)
		{
			BOOL bPrivate = b1? TRUE: FALSE;
			CString strTemplate = pObj->GetName();
			CString strAttrName = _T("???");
			int nPos = strTemplate.Find(_T("Attribute "));
			if (nPos != -1)
				strAttrName = strTemplate.Mid (nPos + _tcslen (_T("Attribute ")));

			switch (ch)
			{
			case _T('+'):
				pVnDef = new CaRestoreVNode(ch);
				pVnDef->m_pNewStruct = new CaNodeAttribute(pObj->GetHost(), strAttrName, pObj->GetValue1(), bPrivate);
				listVNodeDef.AddTail(pVnDef);
				break;
			case _T('-'):
				pVnDef = new CaRestoreVNode(ch);
				pVnDef->m_pNewStruct = new CaNodeAttribute(pObj->GetHost(), strAttrName, pObj->GetValue2(), bPrivate);
				listVNodeDef.AddTail(pVnDef);
				break;
			case _T('!'):
				pVnDef = new CaRestoreVNode(ch);
				pVnDef->m_pOldStruct = new CaNodeAttribute(pObj->GetHost(), strAttrName, pObj->GetValue1(), bPrivate);
				pVnDef->m_pNewStruct = new CaNodeAttribute(pObj->GetHost(), strAttrName, pObj->GetValue2(), bPrivate);
				listVNodeDef.AddTail(pVnDef);
				break;
			default:
				break;
			}
		}
	}

	CaStartGcn startGcn;
	BOOL bGcnUp = ping_iigcn();
	if (!bGcnUp)
	{
		//
		// Failed to start the Name Server.\nThe definition of virtual nodes won't be saved.
		AfxMessageBox (IDS_MSG_FAIL_START_NAME_SERVER);
	}

	if (bGcnUp)
	{
		BOOL HasError = FALSE;
		BOOL b2Restore = TRUE;
#if defined (TEST_RESTORE)
		b2Restore = FALSE;
#endif
		CaNodeDataAccess nodeAccess;
		nodeAccess.InitNodeList();

		pos = listVNodeDef.GetHeadPosition();
		while (pos != NULL)
		{
			HRESULT hErr = NOERROR;
			CaRestoreVNode* pObj = listVNodeDef.GetNext (pos);
			if (pObj->m_tchDifference == _T('+'))
			{
				if (pObj->m_pNewStruct)
				{
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VIRTNODE)
					{
						CaNode* pNodeObj = (CaNode*)pObj->m_pNewStruct;
						TRACE1("Delete Virtual Node = %s\n", pNodeObj->GetName());
						if (b2Restore)
							hErr = VNODE_DropNode (pNodeObj);
					}
					else
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VNODE_LOGINPSW)
					{
						CaNodeLogin* pLoginObj = (CaNodeLogin*)pObj->m_pNewStruct;
						TRACE2("Delete Login = %s/%s\n", pLoginObj->GetNodeName(), pLoginObj->GetName());
						if (b2Restore)
							hErr = VNODE_LoginDrop (pLoginObj);
					}
					else
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VNODE_CONNECTIONDATA)
					{
						CaNodeConnectData* pConnectObj = (CaNodeConnectData*)pObj->m_pNewStruct;
						TRACE2("Delete Connect Data = %s/%s\n", pConnectObj->GetNodeName(), pConnectObj->GetName());
						if (b2Restore)
							hErr = VNODE_ConnectionDrop (pConnectObj);
					}
					else
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VNODE_ATTRIBUTE)
					{
						CaNodeAttribute* pAttrObj = (CaNodeAttribute*)pObj->m_pNewStruct;
						TRACE2("Delete Attribute = %s/%s\n", pAttrObj->GetNodeName(), pAttrObj->GetName());
						if (b2Restore)
							hErr = VNODE_AttributeDrop  (pAttrObj);
					}
				}
			}
			else
			if (pObj->m_tchDifference == _T('-'))
			{
				if (pObj->m_pNewStruct)
				{
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VIRTNODE)
					{
						CaNode* pNodeObj = (CaNode*)pObj->m_pNewStruct;
						CString strItem ;
						strItem.Format(_T("%s Login=%s Connection=/%s/%s/%s"), 
							(LPCTSTR)pNodeObj->GetNodeName(),
							(LPCTSTR)pNodeObj->GetLogin(),
							(LPCTSTR)pNodeObj->GetIPAddress(),
							(LPCTSTR)pNodeObj->GetProtocol(),
							(LPCTSTR)pNodeObj->GetListenAddress());

						TRACE1("Add Virtual Node = %s\n", strItem);
#if defined (WORKAROUND_PASSWORD)
						pNodeObj->SetPassword(pNodeObj->GetLogin());
#endif
						if (b2Restore)
							hErr = VNODE_AddNode  (pNodeObj);
					}
					else
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VNODE_LOGINPSW)
					{
						CaNodeLogin* pLoginObj = (CaNodeLogin*)pObj->m_pNewStruct;
						TRACE2("Add Login = %s/%s\n", pLoginObj->GetNodeName(), pLoginObj->GetName());
#if defined (WORKAROUND_PASSWORD)
						pLoginObj->SetPassword(pLoginObj->GetName());
#endif
						if (b2Restore)
							hErr = VNODE_LoginAdd (pLoginObj);
					}
					else
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VNODE_CONNECTIONDATA)
					{
						CaNodeConnectData* pConnectObj = (CaNodeConnectData*)pObj->m_pNewStruct;
						CString strItem ;
						strItem.Format(_T("%s %s/%s/%s"), 
							(LPCTSTR)pConnectObj->GetNodeName(),
							(LPCTSTR)pConnectObj->GetIPAddress(),
							(LPCTSTR)pConnectObj->GetProtocol(),
							(LPCTSTR)pConnectObj->GetListenAddress());

						TRACE1("Add Connect Data = %s\n", strItem);
						if (b2Restore)
							hErr = VNODE_ConnectionAdd  (pConnectObj);
					}
					else
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VNODE_ATTRIBUTE)
					{
						CaNodeAttribute* pAttrObj = (CaNodeAttribute*)pObj->m_pNewStruct;
						TRACE3("Add Attribute = %s/%s %s\n", pAttrObj->GetNodeName(), pAttrObj->GetAttributeName(), pAttrObj->GetAttributeValue());
						if (b2Restore)
							hErr = VNODE_AttributeAdd   (pAttrObj);
					}
				}
			}
			else
			if (pObj->m_tchDifference == _T('!'))
			{
				if (pObj->m_pNewStruct && pObj->m_pOldStruct)
				{
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VIRTNODE)
					{
						CaNode* pNodeObj = (CaNode*)pObj->m_pNewStruct;
						TRACE1("Alter Virtual Node = %s\n", pNodeObj->GetName());
						ASSERT(FALSE);
					}
					else
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VNODE_LOGINPSW)
					{
						CaNodeLogin* pLoginObjO = (CaNodeLogin*)pObj->m_pOldStruct;
						CaNodeLogin* pLoginObjN = (CaNodeLogin*)pObj->m_pNewStruct;
						TRACE3("Alter Login = %s/%s <-> %s\n", pLoginObjO->GetNodeName(), pLoginObjO->GetName(), pLoginObjN->GetName());
						if (b2Restore)
							hErr = VNODE_LoginAlter (pLoginObjO,pLoginObjN);
					}
					else
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VNODE_CONNECTIONDATA)
					{
						CaNodeConnectData* pConnectObjO = (CaNodeConnectData*)pObj->m_pOldStruct;
						CaNodeConnectData* pConnectObjN = (CaNodeConnectData*)pObj->m_pNewStruct;
						CString strItem;
						strItem.Format(_T("%s/%s/%s <-> %s/%s/%s"), 
							(LPCTSTR)pConnectObjO->GetIPAddress(),
							(LPCTSTR)pConnectObjO->GetProtocol(),
							(LPCTSTR)pConnectObjO->GetListenAddress(),
							(LPCTSTR)pConnectObjN->GetIPAddress(),
							(LPCTSTR)pConnectObjN->GetProtocol(),
							(LPCTSTR)pConnectObjN->GetListenAddress());

						TRACE2("Alter Connect Data = %s/%s\n", pConnectObjO->GetNodeName(), strItem);
						if (b2Restore)
							hErr = VNODE_ConnectionAlter(pConnectObjO, pConnectObjN);
					}
					else
					if (pObj->m_pNewStruct->GetObjectID() == OBT_VNODE_ATTRIBUTE)
					{
						CaNodeAttribute* pAttrObjO = (CaNodeAttribute*)pObj->m_pOldStruct;
						CaNodeAttribute* pAttrObjN = (CaNodeAttribute*)pObj->m_pNewStruct;
						CString strItem;
						strItem.Format(_T("%s %s <-> %s"), 
							(LPCTSTR)pAttrObjO->GetAttributeName(),
							(LPCTSTR)pAttrObjO->GetAttributeValue(),
							(LPCTSTR)pAttrObjN->GetAttributeValue());

						TRACE2("Alter Attribute %s/%s\n", pAttrObjO->GetNodeName(), strItem);
						if (b2Restore)
							hErr = VNODE_AttributeAlter (pAttrObjO, pAttrObjN);
					}
				}
			}

			delete pObj;
			if (hErr != NOERROR)
				HasError = TRUE;
		}
		if (HasError)
		{
			AfxMessageBox (IDS_MSG_FAIL_2_RESTORE_VNODE);
		}
	}
	delete pArray;
}

BOOL VCDA_RestoreParam (CaRestoreParam* pRestore)
{
	CString strBackupFile = pRestore->GetBackup();
	if (!strBackupFile.IsEmpty())
	{
		//
		// Backup the Current Installation:
		VCDA_SaveInstallation(strBackupFile, pRestore->m_pIngresVariable);
	}
	BOOL b2Restore = TRUE;
	CString strFile = _T("config.dat");
#if defined (TEST_RESTORE)
	strFile = _T("xxconfig.dat");
	b2Restore = FALSE;
#endif
	FILE* pFile = NULL;
	TCHAR* pEnv = _tgetenv(_T("II_SYSTEM"));
	CString strFileConfigDat = pEnv;
	strFileConfigDat += consttchszIngFiles;
	strFileConfigDat += strFile;

	if (pRestore->GetConfig())
		pFile = _tfopen(strFileConfigDat, _T("w"));

	CTypedPtrList< CObList, CaCda* >& listResore = pRestore->m_listRestoreParam;
	POSITION pos = listResore.GetHeadPosition();
	while (pos != NULL)
	{
		CaCda* pObj = listResore.GetNext(pos);
		cdaType type = pObj->GetType();
		switch (type)
		{
		case CDA_CONFIG:
			if (pFile)
				sWriteTo (pFile, pObj->GetLeft() + pObj->GetRight(), pObj->GetValue());
			break;
		case CDA_ENVSYSTEM:
			if (b2Restore)
			{
				CString strValue = pObj->GetValue();
				if (strValue.IsEmpty()) // Unset
					pRestore->m_pIngresVariable->SetIngresVariable (pObj->GetName(), pObj->GetValue(), TRUE, TRUE);
				else
					pRestore->m_pIngresVariable->SetIngresVariable (pObj->GetName(), pObj->GetValue(), TRUE, FALSE);
			}
			TRACE2("VCDA SetIngresVariable (SYSTEM): %s = %s\n", (LPCTSTR)pObj->GetName(), (LPCTSTR)pObj->GetValue());
			break;
		case CDA_ENVUSER:
			if (b2Restore)
			{
				CString strValue = pObj->GetValue();
				if (strValue.IsEmpty()) // Unset
					pRestore->m_pIngresVariable->SetIngresVariable (pObj->GetName(), strValue, FALSE, TRUE);
				else
					pRestore->m_pIngresVariable->SetIngresVariable (pObj->GetName(), strValue, FALSE, FALSE);
			}
			TRACE2("VCDA SetIngresVariable (USER): %s = %s\n", (LPCTSTR)pObj->GetName(), (LPCTSTR)pObj->GetValue());
			break;
		default:
			break;
		}
	}
	if (pFile)
		fclose(pFile);
	RestoreVirtualNodes(pRestore);

	while (!listResore.IsEmpty())
		delete listResore.RemoveHead();
	return TRUE;
}

BOOL VCDA_ConfigHasHostName(LPCTSTR lpszName)
{
	const int nSize = 4;
	CString strName = lpszName;
	strName.MakeLower();
	TCHAR tchszTemplate[nSize][32] =
	{
		_T("gcn.local_vnode"),
		_T("config.server_host"),
		_T("dictionary_node"),
		_T("gcf.mech.kerberos.domain")
	};

	for (int i=0; i<nSize; i++)
	{
		if (strName.Find (tchszTemplate[i]) != -1)
			return TRUE;
	}
	return FALSE;
}

//
// nFlag: 1 (II_GCCxx_0 or II_GCDxx7) where xx is substituted by $(II_INSTALLATION)
// nFlag: 2 (OSLAN_xx_7 or OSLAN_xx)  where xx is substituted by $(II_INSTALLATION)
// nFlag: 3 (X25_xx_7 or X25_xx)      where xx is substituted by $(II_INSTALLATION)
BOOL VCDA_ConfigHasII(LPCTSTR lpszName, int& nFlag)
{
	const int nSize = 18; // be careful, if you change the size, make sure that the affectation of nFlag = <number> 
	                      // in the loop below corresponds to the name in the array !!!
	CString strName = lpszName;
	strName.MakeLower();
	TCHAR tchszTemplate[nSize][32] =
	{
		_T("registry.wintcp.port"),
		_T("registry.decnet.port"),
		_T("setup.ii_installation"),
		_T("wintcp.port"),
		_T("decnet.port"),
		_T("jdbc.*.port"),

		_T("async.port"),
		_T("sna_lu62.port"),
		_T("iso_oslan.port"),
		_T("iso_x25.port"),
		_T("sockets.port"),
		_T("spx.port"),
		_T("tcp_ip.port.vnode"),
		_T("tcp_ip.port"),
		_T("tcp_wol.port"),

		_T("gcb.*.lanman.port"),
		_T("gcc.*.lanman.port"),
		_T("registry.lanman.port")
	};

	nFlag = 0;
	for (int i=0; i<nSize; i++)
	{
		if (strName.Find (tchszTemplate[i]) != -1)
		{
			switch (i)
			{
			case 1:
			case 4:
				nFlag = 1; // II_GCCxx_0 or II_GCDxx7
				break;
			case 8:
				nFlag = 2; // OSLAN_xx_7 or OSLAN_xx
				break;
			case 9:
				nFlag = 3; // X25_xx_7 or X25_xx
				break;
			default:
				break;
			}

			return TRUE;
		}
	}
	return FALSE;
}


BOOL VCDA_ConfigHasPath(LPCTSTR lpszName)
{
	const int nSize = 8;
	CString strName = lpszName;
	strName.MakeLower();
	TCHAR tchszTemplate[nSize][32] =
	{
		_T("gcf.mechanism_location"),
		_T("app_dir"),
		_T("log.audit_log_1"),
		_T("log.audit_log_2"),
		_T("audit_log_1"),
		_T("audit_log_2"),
		_T("log.dual_log_1"),
		_T("log.log_file_1")
	};

	for (int i=0; i<nSize; i++)
	{
		if (strName.Find (tchszTemplate[i]) != -1)
			return TRUE;
	}
	return FALSE;
}

static void SaveNodeDefinitions(FILE* pFile)
{
	CaStartGcn startGcn;
	if (!ping_iigcn())
	{
		//
		// Failed to start the Name Server.\nThe definition of virtual nodes won't be saved.
		AfxMessageBox (IDS_MSG_FAIL_START_NAME_SERVER);
		return;
	}

	_fputts(_T("[VIRTUAL NODE DEFINITIONS]\n"), pFile);
	CTypedPtrList<CObList, CaDBObject*> listNode;
	HRESULT hErr = VNODE_QueryNode (listNode);
	if (hErr == NOERROR)
	{
		CString strItem;
		POSITION pos = listNode.GetHeadPosition();
		while (pos != NULL)
		{
			CaNode* pNode = (CaNode*)listNode.GetNext(pos);
			if (pNode->IsLocalNode())
				continue; // Skip the local node from the comparison

			CTypedPtrList<CObList, CaDBObject*> listLogins;
			CTypedPtrList<CObList, CaDBObject*> listConnections;
			CTypedPtrList<CObList, CaDBObject*> listAttributes;
			HRESULT hErr1 = VNODE_QueryLogin (pNode, listLogins);
			HRESULT hErr2 = VNODE_QueryConnection (pNode, listConnections);
			HRESULT hErr3 = VNODE_QueryAttribute (pNode, listAttributes);

			POSITION p = listLogins.GetHeadPosition();
			while (p != NULL)
			{
				CaNodeLogin* pObj = (CaNodeLogin*)listLogins.GetNext (p);
				strItem.Format(_T("%c, Login, \"%s\""), pObj->IsLoginPrivate()? _T('P'): _T('G'), (LPCTSTR)pObj->GetName());
				sWriteTo(pFile, pNode->GetName(), strItem);
				delete pObj;
			}

			int nCountG = 0;
			int nCountP = 0;
			p = listConnections.GetHeadPosition();
			while (p != NULL)
			{
				CaNodeConnectData* pObj = (CaNodeConnectData*)listConnections.GetNext (p);
				TCHAR tchPG = _T('G');
				int nCount = 1;
				if (pObj->IsConnectionPrivate())
				{
					tchPG = _T('P');
					nCountP++;
					nCount = nCountP;
				}
				else
				{
					tchPG = _T('G');
					nCountG++;
					nCount = nCountG;
				}
				strItem.Format(_T("%c, Connection %d, \"%s\",\"%s\",\"%s\""),
					tchPG,
					nCount,
					(LPCTSTR)pObj->GetName(),
					(LPCTSTR)pObj->GetProtocol(),
					(LPCTSTR)pObj->GetListenAddress());
				sWriteTo(pFile, pNode->GetName(), strItem);
				delete pObj;
			}

			p = listAttributes.GetHeadPosition();
			while (p != NULL)
			{
				CaNodeAttribute* pObj = (CaNodeAttribute*)listAttributes.GetNext (p);
				strItem.Format(_T("%c, Attribute, \"%s\",\"%s\""),
					pObj->IsAttributePrivate()? _T('P'): _T('G'), 
					(LPCTSTR)pObj->GetAttributeName(),
					(LPCTSTR)pObj->GetAttributeValue());
				sWriteTo(pFile, pNode->GetName(), strItem);

				delete pObj;
			}

			delete pNode;
		}
	}
}



static BOOL QueryNodesDefinition(CTypedPtrList< CObList, CaCda* >& ls)
{
	CaStartGcn startGcn;
	if (!ping_iigcn())
	{
		//
		// Failed to start the Name Server.\nThe definition of virtual nodes won't be compared.
		CString strMsg = _T("Failed to start the Name Server.\nThe definition of virtual nodes won't be compared.");
		AfxMessageBox (strMsg);
		return FALSE;
	}

	CTypedPtrList<CObList, CaDBObject*> listNode;
	HRESULT hErr = VNODE_QueryNode (listNode);
	if (hErr == NOERROR)
	{
		CString strItem;
		CString strValue;
		CString strPrivate;
		POSITION pos = listNode.GetHeadPosition();
		while (pos != NULL)
		{
			CaNode* pNode = (CaNode*)listNode.GetNext(pos);
			if (pNode->IsLocalNode())
				continue; // Skip the local node from the comparison

			CTypedPtrList<CObList, CaDBObject*> listLogins;
			CTypedPtrList<CObList, CaDBObject*> listConnections;
			CTypedPtrList<CObList, CaDBObject*> listAttributes;
			HRESULT hErr1 = VNODE_QueryLogin (pNode, listLogins);
			HRESULT hErr2 = VNODE_QueryConnection (pNode, listConnections);
			HRESULT hErr3 = VNODE_QueryAttribute (pNode, listAttributes);

			POSITION p = listLogins.GetHeadPosition();
			while (p != NULL)
			{
				CaNodeLogin* pObj = (CaNodeLogin*)listLogins.GetNext (p);
				strPrivate = pObj->IsLoginPrivate()? _T("Private"): _T("Global");
				strItem.Format(_T("%s/%s Login"), (LPCTSTR)pNode->GetName(), (LPCTSTR)strPrivate);
				strValue = pObj->GetName();
				CaCda* pData = new CaCda(pNode->GetName(), strItem, _T(""), strValue, CDA_VNODE);
				ls.AddTail(pData);
				delete pObj;
			}

			int nCountG = 0;
			int nCountP = 0;
			p = listConnections.GetHeadPosition();
			while (p != NULL)
			{
				CaNodeConnectData* pObj = (CaNodeConnectData*)listConnections.GetNext (p);
				int nCount = 1;
				if (pObj->IsConnectionPrivate())
				{
					strPrivate = _T("Private");
					nCountP++;
					nCount = nCountP;
				}
				else
				{
					strPrivate = _T("Global");
					nCountG++;
					nCount = nCountG;
				}

				strItem.Format(_T("%s/%s Connection %d"), (LPCTSTR)pNode->GetName(), (LPCTSTR)strPrivate, nCount);
				strValue.Format(_T("%s/%s/%s"), 
					(LPCTSTR)pObj->GetIPAddress(), 
					(LPCTSTR)pObj->GetProtocol(), 
					(LPCTSTR)pObj->GetListenAddress());
				CaCda* pData = new CaCda(pNode->GetName(), strItem, _T(""), strValue, CDA_VNODE);
				ls.AddTail(pData);
				delete pObj;
			}

			p = listAttributes.GetHeadPosition();
			while (p != NULL)
			{
				CaNodeAttribute* pObj = (CaNodeAttribute*)listAttributes.GetNext (p);
				strPrivate = pObj->IsAttributePrivate()? _T("Private"): _T("Global");
				strItem.Format(_T("%s/%s Attribute %s"), 
					(LPCTSTR)pNode->GetName(), 
					(LPCTSTR)strPrivate, 
					(LPCTSTR)pObj->GetAttributeName());
				strValue = pObj->GetAttributeValue();
				CaCda* pData = new CaCda(pNode->GetName(), strItem, _T(""), strValue, CDA_VNODE);
				ls.AddTail(pData);
				delete pObj;
			}

			delete pNode;
		}
	}

	return TRUE;
}

static void GenerateVnodeParam(LPCTSTR lpszVNode, CStringArray& arrayFields, CString& strName, CString& strValue)
{
	CString s;
	int i, cSize = arrayFields.GetSize();

	strName = _T("");
	strValue = _T("");
	if (cSize >= 2)
	{
		int nStart = 2;
		CString s1 = arrayFields.GetAt(0);
		CString s2 = arrayFields.GetAt(1);

		s1.MakeUpper();
		s2.MakeUpper();

		if (s1.Find('G') == 0)
			s1 = _T("Global");
		else
			s1 = _T("Private");
		if (s2.Find(_T("LOGIN")) != -1)
		{
			strName.Format(_T("%s/%s Login"), lpszVNode, (LPCTSTR)s1);
		}
		else
		if (s2.Find(_T("CONNECTION")) != -1)
		{
			strName.Format(_T("%s/%s %s"), lpszVNode, (LPCTSTR)s1, (LPCTSTR)arrayFields.GetAt(1));
		}
		else
		if (s2.Find(_T("ATTRIBUTE")) != -1)
		{
			nStart = 3;
			if (cSize > 2)
			{
				strName.Format(_T("%s/%s Attribute %s"), lpszVNode, (LPCTSTR)s1, (LPCTSTR)arrayFields.GetAt(2));
			}
		}

		BOOL bOne = TRUE;
		for (i=nStart; i< cSize; i++)
		{
			s = arrayFields.GetAt(i);
			if (!bOne)
				strValue += _T('/');
			strValue += s;
			bOne = FALSE;
		}
	}
}

//
// This function is from dmlccsv.cpp (svriia.dll project)
// Get the next character:
// It does not modify the parameter 'lpcText'
// ************************************************************************************************
static TCHAR PeekNextChar (const TCHAR* lpcText)
{
	TCHAR* pch = (TCHAR*)lpcText;
	if (!pch)
		return _T('\0');
	pch = _tcsinc(pch);

	if (!pch)
		return _T('\0');
	if (_istleadbyte(*pch))
		return _T('\0');
	return *pch;
}

//
// This function is from dmlccsv.cpp (svriia.dll project) with some 
// modifications.
// ************************************************************************************************
static void GetFieldsFromLine (CString& strLine, CStringArray& arrayFields)
{
	CString strField = _T("");
	TCHAR tchszSep  = _T('\0');
	CString strPrev = _T("");
	struct StateStack
	{
		BOOL IsEmpty(){return m_stack.IsEmpty();}
		TCHAR Top()
		{
			TCHAR state;
			if (IsEmpty())
				return _T('\0');
			state = m_stack.GetHead(); 
			return state;
		}
		void PushState(TCHAR state){ m_stack.AddHead(state); };
		void PopState() { m_stack.RemoveHead(); }
	
		CList< TCHAR, TCHAR > m_stack;
	};

	StateStack stack;
	TCHAR tchszTextQualifier = _T('"');

	TCHAR* pch = (LPTSTR)(LPCTSTR)strLine;
	while (pch && *pch != _T('\0'))
	{
		tchszSep = *pch;
		if (_istleadbyte(tchszSep))
		{
			strField += tchszSep;
			strField += *(pch+1); /* trailing byte of effective DBCS char */
			pch = _tcsinc(pch);
			if (pch && *pch == _T('\0'))
			{
				strField.TrimLeft();
				strField.TrimRight();
				arrayFields.Add (strField);
				strField = _T("");
			}
			continue;
		}
		if (!stack.IsEmpty())
		{
			TCHAR tchTop = stack.Top();
			if (tchTop == tchszSep && PeekNextChar(pch) != tchTop)
				stack.PopState();
			else
				strField += tchszSep;
		}
		else
		if (tchszTextQualifier != _T('\0') && (tchszSep == _T('\"')))
		{
			BOOL bValidTextQualifier = TRUE;
			if (!bValidTextQualifier)
			{
				strField += tchszSep; // The TQ is considered as normal character;
				strPrev = tchszSep;
				pch = _tcsinc(pch);
				continue;
			}
			//
			// The text qualifier is valid and even though the 'pSeparatorSet' does not
			// tell us to handle the text qualifier, we handle it any way, because all
			// walid TQ without ambiguity, the 'pSeparatorSet' does not tell to handle the TQ.
			// EX: line "ABCD","RRR" has the TQ without ambiguity. 
			//     And if such a line presents we must produce the result 
			//     col1=ABCD
			//     col2=RRR.

			// So we push it on the stack 
			if (bValidTextQualifier && tchszTextQualifier == tchszSep)
				stack.PushState(tchszSep);
			else
				strField += tchszSep;

			strPrev = tchszSep;
			pch = _tcsinc(pch);
			continue;
		}
		else
		{
			if (tchszSep == _T(','))
			{
				strField.TrimLeft();
				strField.TrimRight();

				arrayFields.Add (strField);
				strField = _T("");
				
				//
				// Just skip the separator:
				strPrev = tchszSep;
				pch = _tcsninc (pch, strPrev.GetLength());
				tchszSep = *pch;
				continue;
			}
			else
				strField+= tchszSep;
		}

		strPrev = tchszSep;
		pch = _tcsinc(pch);

		if (pch && *pch == _T('\0'))
		{
			strField.TrimLeft();
			strField.TrimRight();
			arrayFields.Add (strField);
			strField = _T("");
		}
	}
}
