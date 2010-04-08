/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcdadml.h: header file
**    Project  : Visual Config Diff 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation of data
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    SIR #109221, created
**    Created
**/

#if !defined (VCDADML_HEADER)
#define VCDADML_HEADER
#include "calsctrl.h"

class CuDlgMain;
class CaIngresVariable;
class CaCompareParam;
typedef enum {CDA_GENERAL=0, CDA_CONFIG, CDA_ENVSYSTEM, CDA_ENVUSER, CDA_VNODE} cdaType;
class CaCda: public CObject
{
public:
	CaCda(LPCTSTR lpszName, LPCTSTR lpszValue, cdaType nType);
	CaCda(LPCTSTR lpszHost, LPCTSTR lpszLeft, LPCTSTR lpszRight, LPCTSTR lpszValue, cdaType nType);
	CaCda(const CaCda& c);

	virtual ~CaCda(){}
	CString GetHost(){return m_strHost;}
	CString GetLeft(){return m_strLeft;}
	CString GetRight(){return m_strRight;}
	CString GetName();
	CString GetName2(CaCompareParam* pInfo, int nSnapshot, BOOL bSubstituteUser);
	CString GetValue(){return m_strValue;}
	cdaType GetType(){return m_nType;}

	static CaCda* Find(LPCTSTR lpRight, CTypedPtrList< CObList, CaCda* >* pList);
protected:
	CString m_strHost;

	CString m_strLeft;
	CString m_strRight;
	CString m_strValue;
	cdaType m_nType;

	void Copy (const CaCda& c);

};

class CaMappedHost: public CObject
{
public:
	CaMappedHost();
	CaMappedHost(LPCTSTR lpszHost1, LPCTSTR lpszHost2);
	~CaMappedHost(){}

	CString GetHost1(){return m_strHost1;}
	CString GetHost2(){return m_strHost2;}
protected:
	CString m_strHost1;
	CString m_strHost2;
};


class CaCdaDifference: public CObject
{
public:
	CaCdaDifference(LPCTSTR lpszName, LPCTSTR lpszValue1, LPCTSTR lpszValue2, cdaType nType, TCHAR chDiff, LPCTSTR lpszHost);
	CaCdaDifference(LPCTSTR lpszName, LPCTSTR lpszValue1, LPCTSTR lpszValue2, cdaType nType, TCHAR chDiff, LPCTSTR lpszHost, LPCTSTR lpszOriginalName);
	CString GetValue1(){return m_strValue1;}
	CString GetValue2(){return m_strValue2;}
	TCHAR& GetDifference(){return m_chszDifference;}
	CString GetHost(){return m_strHost;}
	CString GetName(){return m_strName;}
	CString GetOriginalName(){return m_strOriginalName;}
	cdaType GetType(){return m_nType;}

	virtual ~CaCdaDifference(){}

protected:
	CString m_strHost;
	CString m_strName;
	CString m_strValue1;
	CString m_strValue2;
	cdaType m_nType;
	//
	// Meaning of m_chszDifference when compararing two lists:
	// '+': It exists in the first list but not in the second list
	// '-': It exists in the second list but not in the first list
	// '=': the elements in the first and second lists are identical
	// '!': the elements in the first and second lists are different
	TCHAR m_chszDifference; 
	CString m_strOriginalName;

};

inline CaCdaDifference::CaCdaDifference(LPCTSTR lpszName, LPCTSTR lpszValue1, LPCTSTR lpszValue2, cdaType nType, TCHAR chDiff, LPCTSTR lpszHost)
{
	m_nType = nType;
	m_strHost = lpszHost;
	m_strName = lpszName;
	m_strValue1 = lpszValue1;
	m_strValue2 = lpszValue2;
	m_chszDifference = chDiff;
	m_strOriginalName = lpszName;
}

inline CaCdaDifference::CaCdaDifference(LPCTSTR lpszName, LPCTSTR lpszValue1, LPCTSTR lpszValue2, cdaType nType, TCHAR chDiff, LPCTSTR lpszHost, LPCTSTR lpszOriginalName)
{
	m_nType = nType;
	m_strHost = lpszHost;
	m_strName = lpszName;
	m_strValue1 = lpszValue1;
	m_strValue2 = lpszValue2;
	m_chszDifference = chDiff;
	m_strOriginalName = lpszOriginalName;
}

class CaCompareParam
{
public:
	CaCompareParam();
	CaCompareParam(const CaCompareParam& c);
	CaCompareParam operator = (const CaCompareParam& c);

	~CaCompareParam(){}
	void CleanIgnore();
	void AddIgnore(LPCTSTR lpszName);
	BOOL IsIgnore (LPCTSTR lpszName);

	void ManageGeneralParameters(int nSnapShot, CaCda* pObj);
	CStringList& GetListIIDependent(){return m_listIIDependent;}
	void CleanIIDependent(){m_listIIDependent.RemoveAll();}
	void SetPlatform1(LPCTSTR lpszPlatform){m_strPlatform1 = lpszPlatform;}
	void SetPlatform2(LPCTSTR lpszPlatform){m_strPlatform2 = lpszPlatform;}

	CString GetHost1(){return m_strHost1;}
	CString GetHost2(){return m_strHost2;}
	CString GetIISystem1(){return m_strIISystem1;}
	CString GetIISystem2(){return m_strIISystem2;}
	CString GetInstallation1(){return m_strII1;}
	CString GetInstallation2(){return m_strII2;}
	CString GetUser1(){return m_strUser1;}
	CString GetUser2(){return m_strUser2;}
	CString GetPlatform1(){return m_strPlatform1;}
	CString GetPlatform2(){return m_strPlatform2;}
	
	int WhatSubstituteHost(){return m_nSubstituteHost;}
	
	void SetHosts(LPCTSTR lpszS1, LPCTSTR lpszS2);
	void SetSubstituteHost(int nSet){m_nSubstituteHost = nSet;}
protected:
	CStringList m_listIgnore;
	CStringList m_listIIDependent;
	CString m_strHost1;
	CString m_strHost2;
	CString m_strII1;
	CString m_strII2;
	CString m_strIISystem1;
	CString m_strIISystem2;
	CString m_strUser1;
	CString m_strUser2;
	CString m_strPlatform1;
	CString m_strPlatform2;
	int m_nSubstituteHost; // 0: <host name>, 1: <host name1 / host name2>

	void Copy (const CaCompareParam& c);
};


inline void CaCompareParam::SetHosts(LPCTSTR lpszS1, LPCTSTR lpszS2)
{
	m_strHost1 = lpszS1; 
	m_strHost2 = lpszS2;
}


inline void CaCompareParam::ManageGeneralParameters(int nSnapShot, CaCda* pObj)
{
	if (nSnapShot == 1)
	{
		if (pObj->GetName().CompareNoCase(_T("HOST NAME")) == 0)
			m_strHost1 = pObj->GetValue();
		else
		if (pObj->GetName().CompareNoCase(_T("II_SYSTEM")) == 0)
			m_strIISystem1 = pObj->GetValue();
		else
		if (pObj->GetName().CompareNoCase(_T("II_INSTALLATION")) == 0)
			m_strII1 = pObj->GetValue();
		else
		if (pObj->GetName().CompareNoCase(_T("USER")) == 0)
			m_strUser1 = pObj->GetValue();
	}
	else
	{
		if (pObj->GetName().CompareNoCase(_T("HOST NAME")) == 0)
			m_strHost2 = pObj->GetValue();
		else
		if (pObj->GetName().CompareNoCase(_T("II_SYSTEM")) == 0)
			m_strIISystem2 = pObj->GetValue();
		else
		if (pObj->GetName().CompareNoCase(_T("II_INSTALLATION")) == 0)
			m_strII2 = pObj->GetValue();
		else
		if (pObj->GetName().CompareNoCase(_T("USER")) == 0)
			m_strUser2 = pObj->GetValue();
	}
}

void VCDA_CompareList(
    CTypedPtrList< CObList, CaCda* >& ls1, 
    CTypedPtrList< CObList, CaCda* >& ls2, 
    CTypedPtrList< CObList, CaCdaDifference* >& ldiff,
    CaCompareParam* pParam);
void VCDA_CompareList2(
    CTypedPtrList< CObList, CaCda* >& ls1, 
    CTypedPtrList< CObList, CaCda* >& ls2,
    CTypedPtrList< CObList, CaMappedHost* >& listMappedHost,
    CTypedPtrList< CObList, CaCdaDifference* >& ldiff,
    CaCompareParam* pParam);
void VCDA_CompareListVNode(
    CTypedPtrList< CObList, CaCda* >& ls1, 
    CTypedPtrList< CObList, CaCda* >& ls2, 
    CTypedPtrList< CObList, CaCdaDifference* >& ldiff,
    CaCompareParam* pParam);

void VCDA_SaveInstallation(LPCTSTR lpszFile, CaIngresVariable* pIngresVariable);
void VCDA_QueryCdaData(LPCTSTR lpszFile, CTypedPtrList< CObList, CaCda* >& ls, int nGcn = -1);
//
// if lpszFile2 is NULL then compare current installation with lpszFile1
// else compare lpszFile1 with lpszFile2:
BOOL VCDA_Compare (CuDlgMain* pDlgMain, LPCTSTR lpszFile1, LPCTSTR lpszFile2);

// 
// Check if network installation:
BOOL VCDA_IsNetworkInstallation();
//
// Inconsistency:
// If bCurrentInstallation = TRUE, then lpszFile1 is a config.dat
// lpszFile2 is always a snapshot file
void VCDA_CheckInconsistency(
    CTypedPtrList< CObList, CaCda* >& ls1, 
    CTypedPtrList< CObList, CaCda* >& ls2,
    BOOL bCurrentInstallation,
    LPCTSTR lpszFile1,
    LPCTSTR lpszFile2);
//
// Make file consistent:
// The new content of file is restored from the list 'ls'
// If bCurrentInstallation = TRUE, then lpszFile is a config.dat 
// else lpszFile is a snapshot file
void VCDA_MakeFileConsistent(CTypedPtrList< CObList, CaCda* >& ls,  LPCTSTR lpszFile, BOOL bCurrentInstallation);
BOOL VCDA_GetIngresVersionIDxPatches (CString& strIngresVersion, CString& strIngresPatches);

class CaCdaCouple: public CObject
{
public:
	CaCdaCouple(CaCda* p1, CaCda* p2){m_p1 = p1; m_p2 = p2;}
	~CaCdaCouple(){};

	CaCda* m_p1;
	CaCda* m_p2;
};

class CaRestoreParam
{
public:
	CaRestoreParam();
	~CaRestoreParam()
	{
		Cleanup();
	}
	void Cleanup();

	void SetConfig(BOOL bRestore){m_bConfig = bRestore;}
	void SetSystemVariable(BOOL bRestore){m_bSystemVariable = bRestore;}
	void SetUserVariable(BOOL bRestore){m_bUserVariable = bRestore;}
	void SetVirtualNode(BOOL bRestore){m_bVirtualNode = bRestore;}
	void SetPath(BOOL bRestore){m_bPath = bRestore;}
	void SetBackup(LPCTSTR lpszFile){m_strBackupFile = lpszFile;}

	BOOL GetConfig(){return m_bConfig;}
	BOOL GetSystemVariable(){return m_bSystemVariable;}
	BOOL GetUserVariable(){return m_bUserVariable;}
	BOOL GetVirtualNode(){return m_bVirtualNode;}
	BOOL GetPath(){return m_bPath;}
	CString GetBackup(){return m_strBackupFile ;}

protected:
	BOOL m_bConfig;            // TRUE: restore the config parameters
	BOOL m_bSystemVariable;    // TRUE: restore the system variables
	BOOL m_bUserVariable;      // TRUE: restore the user variables
	BOOL m_bVirtualNode;       // TRUE: restore the virtual nodes
	BOOL m_bPath;              // FALSE:do not restore everything representing paths

	CString m_strBackupFile;   // File name to backup the snapshot before restoring.

public:
	//
	// The following members are pointers point to the already living variables, specially to
	// those in class CuDlgMain
	//
	// General Parameters
	CTypedPtrList< CObList, CaCda* >* m_plg1;
	CTypedPtrList< CObList, CaCda* >* m_plg2;
	//
	// Configuration Parameters
	CTypedPtrList< CObList, CaCda* >* m_plc1;
	CTypedPtrList< CObList, CaCda* >* m_plc2;
	//
	// Environment Parameters
	CTypedPtrList< CObList, CaCda* >* m_ples1;
	CTypedPtrList< CObList, CaCda* >* m_ples2;
	CTypedPtrList< CObList, CaCda* >* m_pleu1;
	CTypedPtrList< CObList, CaCda* >* m_pleu2;
	//
	// Virtual Nodes:
	CTypedPtrList< CObList, CaCda* >* m_plvn1;
	CTypedPtrList< CObList, CaCda* >* m_plvn2;
	//
	// Configuration (Other configured host names):
	CTypedPtrList< CObList, CaCda* >* m_plOtherhost1;
	CTypedPtrList< CObList, CaCda* >* m_plOtherhost2;

	CTypedPtrList< CObList, CaCdaDifference* >* m_plistDifference;
	CaIngresVariable* m_pIngresVariable;

	CTypedPtrList< CObList, CaCda* > m_listRestoreParam;
	CTypedPtrList< CObList, CaCdaCouple* > m_listSubStituteInfo;
	CStringList m_listROVariable; // Read Only Variable
};

BOOL VCDA_ConstructListRestoreParam (CaRestoreParam* pRestore);
BOOL VCDA_RestoreParam (CaRestoreParam* pRestore);

BOOL VCDA_ConfigHasHostName(LPCTSTR lpszName);
//
// nFlag: 1 (II_GCCxx_0) where xx is substituted by $(II_INSTALLATION)
BOOL VCDA_ConfigHasII(LPCTSTR lpszName, int& nFlag);
BOOL VCDA_ConfigHasPath(LPCTSTR lpszName);


#endif // VCDADML_HEADER
