/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : environm.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Variable Environment Management
**
**
** History:
** 27-Aug-1999: (uk$so01) 
**    created
** 25-Nov-1999: (uk$so01) 
**    Port to UNIX OS, Ignore registry manipulation.
** 17-Jan-2000 (noifr01)
**    (bug 99994) added II_DATE_CENTURY_BOUNDARY to the list of Ingres
**    variables that are proposed in IVM 
** 28-Feb-2000 (uk$so01)
**    Bug #100621
**    Reoganize the way of manipulation of ingres variable.
**    Due to the unexpected behaviour of the release version of app,
**    now the app uses the member CTypedPtrList<CObList, CaEnvironmentParameter*>
**    m_listEnvirenment instead of static
**    CHARxINTxTYPExMINxMAX* IngresEnvironmentVariable allocated on heap.
** 29-Feb-2000 (uk$so01)
**    (SIR # 100634)
**    The "Add" button of Parameter Tab can be used to set 
**    Parameters for System and User Tab. 
** 03-Mars-2000 (uk$so01)
**    BUG #100707
**    ingsetenv NAME VALUE, If value contains space then quote the VALUE.
** 07-mar-2000 (mcgem01)
**    Add the OpenROAD environment variables.
** 08-Mar-2000 (noifr01)
**    (bug 100791)
**    set min-max values for a couple of variables that had the
**    SPINBOX style, but no minax was defined. Switched
**    II_W4GL_CACHE_LIMIT to the P_TEXT style because otherwise the
**    limit might have been too big for the spinbox control
** 27-jun-2000 (somsa01)
**    In IVM_SetIngresVariable(), print out a proper message when
**    the user tries to set a USER environment variable on UNIX.
** 05-jul-2000 (uk$so01)
**    bug 99242 Handle DBCS
** 18-oct-2000 (somsa01)
**    Added missing text for II_WORK.
** 18-apr-2001 (uk$so01)
**    BUG #104428: Add the unset parameters into the display list 
**    only if there are not there.
** 05-Jun-2001 (uk$so01)
**    BUG #104850: Notify setting changed (Environment) to Top Level 
** 27-Sep-2001 (noifr01)
**    (sir 105798) added the description for II_NUM_OF_PROCESSORS and
**    II_MAX_SEM_LOOPS when they appear in the "extra" sub-tab (but
**    don't propose them in the other tabs for now), and added
**    II_CKPTMPL_FILE
** 05-Oct-2001 (noifr01)
**    (sir 105979) added also II_PF_NODE
** 23-Dec-2003 (uk$so01)
**    SIR #109221
**    This file is from the IVM project. Modify and put the file in the library
**    libingll.lib to be used by Visual Config Diff Analyzer (VCDA).
**    Rename the CaEnvironmentParameter to CaEnvironmentParameter.
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 27-Mar-2003 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Enhance the library.
** 04-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 26-Jun-2003 (schph01)
**    BUG #110471 Add II_HALF_DUPLEX, II_STAR_LOG, DD_RSERVERS parameters in
**    CaIngresVariable::Init()
** 02-feb-2004 (somsa01)
**    Corrected ifdef.
*/

#include "stdafx.h"
#include "constdef.h"
#include "environm.h"
#include "ingobdml.h"

//
// Invironment variables used by Ingres:
static BOOL VarableExtract (LPCTSTR lpszVar, CString& strName, CString& strValue);
static CaEnvironmentParameter* ParameterExist (CTypedPtrList<CObList, CaEnvironmentParameter*>& listParameter, LPCTSTR lpszName);
static BOOL QueryKeyValues (HKEY hKey, LPCTSTR lpszSubKey, CTypedPtrList<CObList, CaEnvironmentParameter*>& listParameter);
static BOOL ChangeKeyValue (HKEY hKey, LPCTSTR lpszSubKey, LPCTSTR lpszName, LPCTSTR lpszValue, DWORD dwType = REG_SZ);
static BOOL DeleteKeyValue (HKEY hKey, LPCTSTR lpszSubKey, LPCTSTR lpszName);



//
// STATIC FUNCTIONS:
// ************************************************************************************************
static void UpdateParameter(CaEnvironmentParameter* pParam, CaEnvironmentParameter* pSmp)
{
	pParam->SetType (pSmp->GetType());
	int nMin, nMax;
	pSmp->GetMinMax(nMin, nMax);
	pParam->SetMinMax (nMin, nMax);
	pParam->LocallyResetable(pSmp->IsLocallyResetable());
	pParam->SetReadOnly(pSmp->IsReadOnly());
	pParam->SetDescription(pSmp->GetDescription());
}

//
// This function is DBCS compliant.
static BOOL VarableExtract (LPCTSTR lpszVar, CString& strName, CString& strValue)
{
	CString strV = lpszVar;
	int n0A = strV.Find (0xA);
	if (n0A != -1)
		strV.SetAt(n0A, _T(' '));
	strV.TrimRight();
	int nEq = strV.Find (_T('='));
	if (nEq == -1)
		return FALSE;
	strName = strV.Left (nEq);
	strValue= strV.Mid (nEq+1);
	return TRUE;
}

static CaEnvironmentParameter* ParameterExist (CTypedPtrList<CObList, CaEnvironmentParameter*>& listParameter, LPCTSTR lpszName)
{
	CaEnvironmentParameter* pObj = NULL;
	POSITION pos = listParameter.GetHeadPosition();
	while (pos != NULL)
	{
		pObj = listParameter.GetNext (pos);
		if (pObj->GetName().CompareNoCase (lpszName) == 0)
			return pObj;
	}
	return NULL;
}

static BOOL QueryKeyValues (HKEY hKey, LPCTSTR lpszSubKey, CTypedPtrList<CObList, CaEnvironmentParameter*>& listParameter)
{
	CaEnvironmentParameter* pItem = NULL;
	HKEY hResult = NULL;
	CString strSubkey = lpszSubKey; //_T("System\\ControlSet001\\Control\\Session Manager\\Environment");
	LONG lReg = RegOpenKey(hKey, strSubkey, &hResult);
	
	if (lReg != ERROR_SUCCESS)
		return FALSE;

	TCHAR tchszBufferName [256];
	TCHAR tchszBufferValue[1024];
	DWORD dwSize = 256;
	DWORD dwSizeR= 1024;
	DWORD dwIndex = 0;
	DWORD dwType;
	long ls = RegEnumValue (hResult, dwIndex, tchszBufferName, &dwSize, NULL, &dwType, (LPBYTE)tchszBufferValue, &dwSizeR);
	while (ls == ERROR_SUCCESS)
	{
		if (dwType == REG_SZ || dwType == REG_EXPAND_SZ)
		{
			pItem = new CaEnvironmentParameter(tchszBufferName, tchszBufferValue, _T(""));
			listParameter.AddTail (pItem);
		}
	
		dwIndex++;
		dwSize = 256;
		dwSizeR = 1024;
		ls = RegEnumValue (hResult, dwIndex, tchszBufferName, &dwSize, NULL, &dwType, (LPBYTE)tchszBufferValue, &dwSizeR);
	}
	return TRUE;
}

static BOOL ChangeKeyValue (HKEY hKey, LPCTSTR lpszSubKey, LPCTSTR lpszName, LPCTSTR lpszValue, DWORD dwType)
{
	CaEnvironmentParameter* pItem = NULL;
	HKEY hResult = NULL;
	CString strSubkey = lpszSubKey; //_T("System\\ControlSet001\\Control\\Session Manager\\Environment");
	LONG lReg = RegOpenKey(hKey, strSubkey, &hResult);
	
	if (lReg != ERROR_SUCCESS)
		return FALSE;
	if (!hResult)
		return FALSE;
	if (!lpszValue)
		return FALSE;
	int nLen = lstrlen (lpszValue) + 1;
	
	RegSetValueEx(hResult, lpszName, 0, dwType, (CONST BYTE*)lpszValue, nLen);
	if (hResult)
		RegCloseKey(hResult);

	return TRUE;
}

static BOOL DeleteKeyValue (HKEY hKey, LPCTSTR lpszSubKey, LPCTSTR lpszName)
{
	HKEY hResult = NULL;
	CString strSubkey = lpszSubKey;
	LONG lReg = RegOpenKey(hKey, strSubkey, &hResult);
	
	if (lReg != ERROR_SUCCESS)
		return FALSE;
	if (!hResult)
		return FALSE;
	
	RegDeleteValue(hResult, lpszName);
	if (hResult)
		RegCloseKey(hResult);

	return TRUE;
}


//
// CaNameValue:
// ************************************************************************************************
IMPLEMENT_SERIAL (CaNameValue, CObject, 1)
CaNameValue::CaNameValue()
{
	m_strParameter = _T("");
	m_strValue = _T("");
}

CaNameValue::CaNameValue(LPCTSTR lpszName, LPCTSTR lpszValue)
{
	m_strParameter = lpszName;
	m_strValue = lpszValue;
}

void CaNameValue::Serialize (CArchive& ar)
{
	if (ar.IsLoading())
	{
		ar >> m_strParameter;
		ar >> m_strValue;

	}
	else
	{
		ar << m_strParameter;
		ar << m_strValue;
	}
}

//
// CaEnvironmentParameter
// ************************************************************************************************
IMPLEMENT_SERIAL (CaEnvironmentParameter, CaNameValue, 1)
BOOL CaEnvironmentParameter::LoadDescription(UINT nIDS)
{
	return m_strDescription.LoadString(nIDS);
}

void CaEnvironmentParameter::Serialize (CArchive& ar)
{
	CaNameValue::Serialize(ar);
	if (ar.IsLoading())
	{
		int nType = 0;

		ar >> m_strDescription;
		ar >> m_bLocalMachine;
		ar >> m_bLocallyResetable;
		ar >> m_bReadOnly;
		ar >> m_bSet;
		ar >> nType; m_nType = ParameterType(nType);
		ar >> m_nMin;
		ar >> m_nMax;
	}
	else
	{
		ar << m_strDescription;
		ar << m_bLocalMachine;
		ar << m_bLocallyResetable;
		ar << m_bReadOnly;
		ar << m_bSet;
		ar << (int)m_nType;
		ar << m_nMin;
		ar << m_nMax;
	}
}

//
// CaIngresVariable:
// ************************************************************************************************
CaIngresVariable::CaIngresVariable()
{
	m_strVariableNotSet =_T("<not set>");
	m_strNoDescription = _T("This environment variable is not documented");
	m_strIngresBin = _T("");
	m_strUnixNotSupported = _T("This operation is not supported on UNIX platforms.");
}

CaIngresVariable::~CaIngresVariable()
{
	while (!m_listEnvirenment.IsEmpty())
		delete m_listEnvirenment.RemoveHead();
}

//
// Originated from: ivm static IngresEnvValid
CaEnvironmentParameter* CaIngresVariable::IngresEnvValid(LPCTSTR lpszName)
{
	if (lpszName == NULL)
		return NULL;
	CTypedPtrList<CObList, CaEnvironmentParameter*>& lenv = m_listEnvirenment;
	POSITION pos = lenv.GetHeadPosition();
	while (pos != NULL)
	{
		CaEnvironmentParameter* pObj = lenv.GetNext(pos);
		if (pObj->GetName().CompareNoCase(lpszName) == 0)
			return pObj;
	}

	return NULL;
}

BOOL CaIngresVariable::PreInitialize()
{
	TCHAR* pEnv;
	pEnv = _tgetenv(_T("II_SYSTEM"));
	if (!pEnv)
		return FALSE;
	m_strIngresBin = pEnv;
	m_strIngresBin += consttchszIngBin;
	return TRUE;
}

BOOL CaIngresVariable::Init()
{
	if (!PreInitialize())
		return FALSE;

	CString strItem;
	CString strII = INGRESII_QueryInstallationID(FALSE);
	if (strII.IsEmpty())
		strII = _T("II");

	CTypedPtrList<CObList, CaEnvironmentParameter*>& lenvMain = m_listEnvirenment;
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_BIND_SVC_xx"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_CHARSETxx"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_CLIENT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_COLLATION"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_CONFIG"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_CONNECT_RETRIES"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DATABASE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_CHECKPOINT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DUMP"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_JOURNAL"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_WORK"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DBMS_LOG"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DUAL_LOG"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DUAL_LOG_NAME"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_LOG_FILE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_LOG_FILE_NAME"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_GCNxx_PORT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_INSTALLATION"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_TIMEZONE_NAME"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("ING_SYSTEM_SET"), _T(""), _T("")));
#if defined (MAINWIN)
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DIRECT_IO"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_ERSEND"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_NUM_SLAVES"), _T(""), _T("")));
#endif
#if defined (VMS)
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_C_COMPILER"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_LOG_DEVICE"), _T(""), _T("")));
#endif
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_SYSTEM"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_CKPTMPL_FILE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_ABF_RUNOPT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_AFD_TIMEOUT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_APPLICATION_LANGUAGE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DATE_CENTURY_BOUNDARY"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DATE_FORMAT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DBMS_SERVER"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DECIMAL"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DML_DEF"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_EMBED_SET"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_FRS_KEYFIND"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_GCA_LOG"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_GCx_TRACE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_HELP_EDIT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_LANGUAGE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_MONEY_FORMAT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_MONEY_PREC"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_NULL_STRING"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_PATTERN_MATCH"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_POST_4GLGEN"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_PRINTSCREEN_FILE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_SQL_INIT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_TEMPORARY"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_TERMCAP_FILE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_TFDIR"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_TM_ON_ERROR"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_VNODE_PATH"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("ING_ABFDIR"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("ING_ABFOPT1"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("ING_EDIT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("ING_PRINT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("ING_SET"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("INGRES_KEYS"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("INIT_INGRES"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("TERM_INGRES"), _T(""), _T("")));
#if defined (MAINWIN)
	lenvMain.AddTail(new CaEnvironmentParameter(_T("TERM"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("ING_SHELL"), _T(""), _T("")));
#endif
	lenvMain.AddTail(new CaEnvironmentParameter(_T("FORCEDITHER"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("IIDLDIR"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("IIW4GL_DEBUG_3GL"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_4GL_DECIMAL"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_COLORTABLE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_COLORTABLE_FILE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DEBUG_W4GL"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DEFINE_COLORMAP"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_DOUBLE_CLICK"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_FONT_CONVENTION"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_FONT_FILE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_FORCE_C_CONVENTION"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_KEYBOARD"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_KEYBOARD_FILE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_LIBU3GL"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_MATCH_EMPTY_BLANK"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_MONOCHROME_DISPLAY"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_SCREEN_HEIGHT_INCHES"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_SCREEN_HEIGHT_MMS"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_SCREEN_WIDTH_INCHES"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_SCREEN_WIDTH_MMS"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_VIEW"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_W4GLAPPS_DIR"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_W4GLAPPS_SYS"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_W4GL_CACHE_LIMIT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_W4GL_COLOR_TABLE_METHOD"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_W4GL_LINEWIDTHS"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_W4GL_LOCKS"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_W4GL_OPEN_IMAGES"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_W4GL_USE_256_COLORS"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_W4GL_USE_PURE_COLORS"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_WINDOWEDIT"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_WINDOWVIEW"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_PF_NODE"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_HALF_DUPLEX"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("II_STAR_LOG"), _T(""), _T("")));
	lenvMain.AddTail(new CaEnvironmentParameter(_T("DD_RSERVERS"), _T(""), _T("")));

	CaEnvironmentParameter* pObj = NULL;
	POSITION pos = lenvMain.GetHeadPosition();
	while (pos != NULL)
	{
		pObj = lenvMain.GetNext(pos);
		if (pObj->GetName().CompareNoCase(_T("II_BIND_SVC_xx")) == 0)
		{
			strItem.Format (_T("II_BIND_SVC_%s"), (LPCTSTR)strII);
			pObj->SetName(strItem);
		}
		else
		if (pObj->GetName().CompareNoCase(_T("II_CHARSETxx")) == 0)
		{
			strItem.Format (_T("II_CHARSET%s"), (LPCTSTR)strII);
			pObj->SetName(strItem);
		}
		else
		if (pObj->GetName().CompareNoCase(_T("II_GCNxx_PORT")) == 0)
		{
			strItem.Format (_T("II_GCN%s_PORT"), (LPCTSTR)strII);
			pObj->SetName(strItem);
		}
		else
		if (pObj->GetName().CompareNoCase(_T("II_GCx_TRACE")) == 0)
		{
			strItem.Format (_T("II_GC%s_TRACE"), (LPCTSTR)strII);
			pObj->SetName(strItem);
		}
	}

	return TRUE;
}

BOOL CaIngresVariable::Init(CHARxINTxTYPExMINxMAX* pTemplate, int nSize)
{
	if (!PreInitialize())
		return FALSE;

	CString strItem;
	CString strII = INGRESII_QueryInstallationID(FALSE);
	if (strII.IsEmpty())
		strII = _T("II");

	int i = 0;
	CTypedPtrList<CObList, CaEnvironmentParameter*>& lenvMain = m_listEnvirenment;
	CaEnvironmentParameter* pObj = NULL;
	for (i=0; pTemplate[i].tchszName[0] != NULL && i<nSize; i++)
	{
		pObj = new CaEnvironmentParameter();
		pObj->SetName(pTemplate[i].tchszName);
		if (pTemplate[i].nV1 != -1)
		{
			if (!pObj->LoadDescription(pTemplate[i].nV1))
				pObj->SetDescription(m_strNoDescription);
		}
		pObj->LocallyResetable(pTemplate[i].bLocallyResetable);
		pObj->SetReadOnly(pTemplate[i].bReadOnlyAsSystem);
		pObj->SetType(pTemplate[i].nType);
		pObj->SetMinMax(pTemplate[i].nMin, pTemplate[i].nMax);
		lenvMain.AddTail(pObj);
	}

	POSITION pos = lenvMain.GetHeadPosition();
	while (pos != NULL)
	{
		pObj = lenvMain.GetNext(pos);
		if (pObj->GetName().CompareNoCase(_T("II_BIND_SVC_xx")) == 0)
		{
			strItem.Format (_T("II_BIND_SVC_%s"), (LPCTSTR)strII);
			pObj->SetName(strItem);
		}
		else
		if (pObj->GetName().CompareNoCase(_T("II_CHARSETxx")) == 0)
		{
			strItem.Format (_T("II_CHARSET%s"), (LPCTSTR)strII);
			pObj->SetName(strItem);
		}
		else
		if (pObj->GetName().CompareNoCase(_T("II_GCNxx_PORT")) == 0)
		{
			strItem.Format (_T("II_GCN%s_PORT"), (LPCTSTR)strII);
			pObj->SetName(strItem);
		}
		else
		if (pObj->GetName().CompareNoCase(_T("II_GCx_TRACE")) == 0)
		{
			strItem.Format (_T("II_GC%s_TRACE"), (LPCTSTR)strII);
			pObj->SetName(strItem);
		}
	}

	return TRUE;
}

// Ingres Environment Variable:
// Ex: II_LANGUAGE
// They are set/unset by using "ingsetenv" or "ingprenv".
// Originated from: IVM_llQueryRealParameterIngresEnv
BOOL CaIngresVariable::QueryEnvironment (CTypedPtrList<CObList, CaEnvironmentParameter*>& listParameter, BOOL bUnset, BOOL bExtra)
{
	HANDLE hChildStdoutRd, hChildStdoutWr;
	HANDLE hChildStdinRd, hChildStdinWr;
	DWORD  dwExitCode, dwLastError;
	DWORD  dwWaitResult = 0;
	BOOL   bError = FALSE;
	long   lTimeOut = 5*60*15; // 5 mins (Max time taken to finish execution of ingprenv)
	
	CString strResult = _T("");
	CString strError  = _T("Cannot execute command <ingprenv> !");
	CString strCmd = m_strIngresBin;
#if defined (MAINWIN)
	strCmd += _T("ingprenv");
#else
	strCmd += _T("ingprenv.exe");
#endif

	CaEnvironmentParameter* pItem = NULL;
	STARTUPINFO         StartInfo;
	PROCESS_INFORMATION ProcInfo;
	//
	// Init security attributes
	SECURITY_ATTRIBUTES sa;
	memset (&sa, 0, sizeof (sa));
	sa.nLength              = sizeof(sa);
	sa.bInheritHandle       = TRUE; // make pipe handles inheritable
	sa.lpSecurityDescriptor = NULL;

	memset (&StartInfo, 0, sizeof(StartInfo));
	memset (&ProcInfo,  0, sizeof(ProcInfo));

	try
	{
		//
		// Create a pipe for the child's STDOUT
		if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &sa, 0)) 
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}
		//
		// Create a pipe for the child's STDIN
		if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &sa, 0)) 
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}
		
		//
		// Duplicate the write handle to the STDIN pipe so it's not inherited
		if (!DuplicateHandle(
			 GetCurrentProcess(), 
			 hChildStdinWr, 
			 GetCurrentProcess(), 
			 &hChildStdinWr, 
			 0, FALSE, 
			 DUPLICATE_SAME_ACCESS)) 
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}
		//
		// Don't need it any more
		CloseHandle(hChildStdinWr);

		//
		// Create the child process
		StartInfo.cb = sizeof(STARTUPINFO);
		StartInfo.lpReserved = NULL;
		StartInfo.lpReserved2 = NULL;
		StartInfo.cbReserved2 = 0;
		StartInfo.lpDesktop = NULL;
		StartInfo.lpTitle = NULL;
		StartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		StartInfo.wShowWindow = SW_HIDE;
		StartInfo.hStdInput = hChildStdinRd;
		StartInfo.hStdOutput = hChildStdoutWr;
		StartInfo.hStdError = hChildStdoutWr;
		if (!CreateProcess (
			NULL, 
			(LPTSTR)(LPCTSTR)strCmd, 
			NULL, 
			NULL, 
			TRUE, 
			NORMAL_PRIORITY_CLASS, 
			NULL, 
			NULL, 
			&StartInfo, 
			&ProcInfo)) 
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}
		CloseHandle(hChildStdinRd);
		CloseHandle(hChildStdoutWr);
		
		WaitForSingleObject (ProcInfo.hProcess, lTimeOut);
		if (!GetExitCodeProcess(ProcInfo.hProcess, &dwExitCode))
		{
			dwLastError = GetLastError();
			throw (LPCTSTR)strError;
		}

		switch (dwExitCode)
		{
		case STILL_ACTIVE:
			//
			// Time out
			return FALSE;
			break;
		default:
			//
			// Execution complete successfully
			break;
		}

		//
		// Reading data from the pipe:
		const int nSize = 48;
		TCHAR tchszBuffer [nSize];
		DWORD dwBytesRead;

		while (ReadFile(hChildStdoutRd, tchszBuffer, nSize-2, &dwBytesRead, NULL))
		{
			if (dwBytesRead > 0)
			{
				tchszBuffer [(int)dwBytesRead] = _T('\0');
				strResult+= tchszBuffer;
				tchszBuffer[0] = _T('\0');
			}
			else
			{
				//
				// The file pointer was beyond the current end of the file
				break;
			}
		}

		TCHAR  chszSep[] = _T("\n");
		TCHAR* token;
		CString strName;
		CString strValue;
		// 
		// Establish string and get the first token:
		token = _tcstok ((TCHAR*)(LPCTSTR)strResult, chszSep);
		while (token != NULL )
		{
			BOOL bOK = VarableExtract ((LPCTSTR)token, strName, strValue);
			if (bOK)
			{
				if (!bExtra)
				{
					//
					// Verify if the variable exists and valide:
					CaEnvironmentParameter* pValid = IngresEnvValid(strName);
					if (pValid)
					{
						if (!ParameterExist(listParameter, strName))
						{
							pItem = new CaEnvironmentParameter(strName, strValue, _T(""));
							UpdateParameter(pItem, pValid);
							pItem->Set (TRUE);
							listParameter.AddTail (pItem);
						}
					}
					else
					{
						TRACE1 ("Documentation, Variable <%s> is not in the list.\n", (LPCTSTR)strName);
					}
				}
				else
				{
					CaEnvironmentParameter* pValid = IngresEnvValid(strName);
					if (!pValid)
					{
						pItem = new CaEnvironmentParameter(strName, strValue, _T(""));
						pItem->SetType (P_TEXT);
						pItem->LocallyResetable (FALSE);
						pItem->SetReadOnly (FALSE);
						pItem->SetDescription(m_strNoDescription);
						pItem->Set (TRUE);
						listParameter.AddTail (pItem);
					}
				}
			}
			//
			// While there are tokens in "strResult"
			token = _tcstok(NULL, chszSep);
		}

		if (!bExtra)
		{
			//
			// Query the Global ingres Invironments that have been under the register key:
			// HKEY_LOCAL_MACHINE\\System\\ControlSet001\\Control\\Session Manager\\Environment.
			CTypedPtrList<CObList, CaEnvironmentParameter*> lglobalParam;

#if defined (MAINWIN)
			//
			// Under the UNIX operating system:
			// actually, no registry manipulation !!
#else
			QueryKeyValues (HKEY_LOCAL_MACHINE, _T("System\\ControlSet001\\Control\\Session Manager\\Environment"), lglobalParam);
			while (!lglobalParam.IsEmpty())
			{
				CaEnvironmentParameter* pParam = lglobalParam.RemoveHead();
				if (pParam)
				{
					CaEnvironmentParameter* pValid = IngresEnvValid(pParam->GetName());
					if (pValid && !ParameterExist(listParameter, pParam->GetName()))
					{
						UpdateParameter(pParam, pValid);
						pParam->SetLocalMachine (TRUE);
						pParam->Set (TRUE);
						listParameter.AddHead (pParam);
					}
					//
					// I don't know what happen if the param is set both in the registry
					// and ingsetenv ?
					else
					{
						delete pParam;
					}
				}
			}
#endif // defined (MAINWIN)
		}

		//
		// Query the unset parameters:
		if (bUnset && !bExtra)
		{
			CaEnvironmentParameter* pObj = NULL;
			CTypedPtrList<CObList, CaEnvironmentParameter*>& lenvMain = m_listEnvirenment;
			POSITION p = lenvMain.GetHeadPosition();
			while (p != NULL)
			{
				CaEnvironmentParameter* pMainObj = lenvMain.GetNext(p);
				pObj = ParameterExist (listParameter, pMainObj->GetName());
				if (!pObj)
				{
					//
					// Unset variable:
					pItem = new CaEnvironmentParameter(pMainObj->GetName(), m_strVariableNotSet, _T(""));
					UpdateParameter(pItem, pMainObj);
					listParameter.AddTail (pItem);
				}
			}
		}
	}
	catch (LPCTSTR lpszError)
	{
		TRACE1 ("CaIngresVariable::QueryEnvironment, internal error: %s\n", lpszError);
		lpszError = NULL;
		bError = TRUE;
	}
	catch (...)
	{
		bError = TRUE;
	}
	return bError? FALSE: TRUE;
}

// Ingres Environment Variable:
// Ex: II_LANGUAGE
// They are set/unset by using the registry:
// Originated from: IVM_llQueryRealParameterIngresEnvUser
BOOL CaIngresVariable::QueryEnvironmentUser (CTypedPtrList<CObList, CaEnvironmentParameter*>& listParameter, BOOL bUnset)
{
	BOOL   bError = FALSE;
	CString strError  = _T("Cannot open register !");
	CaEnvironmentParameter* pItem = NULL;

	try
	{
		CTypedPtrList<CObList, CaEnvironmentParameter*> lenv;
#if defined (MAINWIN)
		//
		// Under the UNIX operating system:
		// actually, no registry manipulation !!
#else
		QueryKeyValues (HKEY_CURRENT_USER, _T("Environment"), lenv);

		while (!lenv.IsEmpty())
		{
			CaEnvironmentParameter* pEnv = lenv.RemoveHead();
			CaEnvironmentParameter* pValid = IngresEnvValid(pEnv->GetName());
			if (!pValid)
			{
				delete pEnv;
				continue;
			}

			if (!ParameterExist(listParameter, pEnv->GetName()))
			{
				UpdateParameter(pEnv, pValid);
				pEnv->LocallyResetable(TRUE);
				pEnv->Set(TRUE);
				listParameter.AddTail (pEnv);
			}
			else
			{
				delete pEnv;
			}
		}
#endif // defined (MAINWIN)
		if (bUnset)
		{
			CaEnvironmentParameter* pObj = NULL;
			CTypedPtrList<CObList, CaEnvironmentParameter*>& lenvMain = m_listEnvirenment;
			POSITION p = lenvMain.GetHeadPosition();
			while (p != NULL)
			{
				CaEnvironmentParameter* pMainObj = lenvMain.GetNext(p);
				if (pMainObj->IsLocallyResetable() && !ParameterExist(listParameter, pMainObj->GetName()))
				{
					//
					// Unset variable:
					pItem = new CaEnvironmentParameter(pMainObj->GetName(), m_strVariableNotSet, _T(""));
					UpdateParameter(pItem, pMainObj);
#if defined (MAINWIN)
					pItem->SetReadOnly(TRUE);
#endif
					listParameter.AddTail (pItem);
				}
			}
		}
	}
	catch (LPCTSTR lpszError)
	{
		TRACE1 ("CaIngresVariable::QueryEnvironmentUser:- Internal error: %s\n", lpszError);
		lpszError = NULL;
		bError = TRUE;
	}
	catch (...)
	{
		bError = TRUE;
	}

	return bError? FALSE: TRUE;
}

BOOL CaIngresVariable::SetIngresVariable (LPCTSTR lpszName, LPCTSTR lpszValue, BOOL bSystem, BOOL bUnset)
{
	CString strCmd;
	CString strError = _T("Cannot set ingres variable !");
#if defined (MAINWIN)
	strCmd = bUnset? _T("ingunset"):     _T("ingsetenv");
#else
	strCmd = bUnset? _T("ingunset.exe"): _T("ingsetenv.exe");
#endif

	if (bSystem)
	{
		STARTUPINFO         StartInfo;
		PROCESS_INFORMATION ProcInfo;
		//
		// Init security attributes
		SECURITY_ATTRIBUTES sa;
		memset (&sa, 0, sizeof (sa));
		sa.nLength              = sizeof(sa);
		sa.bInheritHandle       = FALSE;
		sa.lpSecurityDescriptor = NULL;
		memset (&StartInfo, 0, sizeof(StartInfo));
		memset (&ProcInfo,  0, sizeof(ProcInfo));

		//
		// Append the argument:
		strCmd += _T(" ");
		strCmd += lpszName;
		if (!bUnset)
		{
			strCmd += _T(" ");

			CString strValue = lpszValue;
			if (!strValue.IsEmpty() && strValue.Find (_T(' ')) != -1)
			{
				// 
				// containing space
				int nLen = strValue.GetLength();
				if (strValue.GetAt(0) != _T('\"'))
					strCmd += _T('\"');
				strCmd += lpszValue;

				TCHAR* ptcsLast = _tcsdec((const TCHAR*)strValue, (const TCHAR*)strValue + _tcslen((const TCHAR*)strValue));
				if (ptcsLast && *ptcsLast != _T('\"'))
					strCmd += _T('\"');
			}
			else
				strCmd += lpszValue;
		}

		//
		// Create the child process
		StartInfo.cb = sizeof(STARTUPINFO);
		StartInfo.lpReserved = NULL;
		StartInfo.lpReserved2 = NULL;
		StartInfo.cbReserved2 = 0;
		StartInfo.lpDesktop = NULL;
		StartInfo.lpTitle = NULL;
		StartInfo.dwFlags =  STARTF_USESHOWWINDOW;
		StartInfo.wShowWindow = SW_HIDE;
		StartInfo.hStdInput = NULL;
		StartInfo.hStdOutput = NULL;
		StartInfo.hStdError = NULL;
		if (!CreateProcess (
			NULL, 
			(LPTSTR)(LPCTSTR)strCmd, 
			NULL, 
			NULL, 
			TRUE, 
			NORMAL_PRIORITY_CLASS, 
			NULL, 
			NULL, 
			&StartInfo, 
			&ProcInfo)) 
		{
			throw strError;
		}

		WaitForSingleObject (ProcInfo.hProcess, INFINITE);
	}
	else
	{
#if defined (MAINWIN)
		//
		// Under the UNIX operating system, these parameters are read only !
		AfxMessageBox(m_strUnixNotSupported);
		return TRUE;
#endif
		if (bUnset)
			DeleteKeyValue (HKEY_CURRENT_USER, _T("Environment"), lpszName);
		else
		{
			CString strV = lpszValue;
			DWORD dwType = REG_SZ;
			int iFound = strV.Find (_T('%'));
			if (iFound != -1)
				dwType = REG_EXPAND_SZ;
			ChangeKeyValue (HKEY_CURRENT_USER, _T("Environment"), lpszName, lpszValue, dwType);
		}

		HWND hWndProgMan = ::FindWindow(_T("ProgMan"), NULL);
		if (hWndProgMan)
			::SendMessage( hWndProgMan, WM_SETTINGCHANGE, 0L, (LPARAM)_T("Environment"));
	}

	return TRUE;
}


BOOL CaIngresVariable::IsReadOnly(LPCTSTR lpszVariableName, LPCTSTR lpszII)
{
	const int nCount = 10;
	TCHAR tchszName[nCount][20] = 
	{
		_T("II_CHARSET%s"),
		_T("II_CONFIG"),
		_T("II_DATABASE"),
		_T("II_CHECKPOINT"),
		_T("II_DUMP"),
		_T("II_JOURNAL"),
		_T("II_GCN%s_PORT"),
		_T("II_INSTALLATION"),
		_T("II_MSGDIR"),
		_T("II_SYSTEM")
	};

	for (int i=0; i<nCount; i++)
	{
		if (i== 0 || i==6)
		{
			CString s;
			s.Format(tchszName[i], lpszII);
			if (s.CompareNoCase (lpszVariableName) == 0)
				return TRUE;
		}
		else
		{
			if (_tcsicmp(tchszName[i], lpszVariableName) == 0)
				return TRUE;
		}
	}
	return FALSE;
}

BOOL CaIngresVariable::IsPath(LPCTSTR lpszVariableName)
{
	const int nCount = 16;
	TCHAR tchszName[nCount][20] = 
	{
		_T("II_SYSTEM"),
		_T("II_CONFIG"),
		_T("II_CONFIG_LOCAL"),
		_T("II_DATABASE"),
		_T("II_CHECKPOINT"),
		_T("II_DUMP"),
		_T("II_JOURNAL"),
		_T("II_WORK"),
		_T("II_DBMS_LOG"),
		_T("II_DUAL_LOG"),
		_T("II_LOG_FILE"),
		_T("II_TEMPORARY"),
		_T("ING_ABFDIR"),
		_T("IIDLDIR"),
		_T("II_W4GLAPPS_DIR"),
		_T("II_W4GLAPPS_SYS")
	};

	for (int i=0; i<nCount; i++)
	{
		if (_tcsicmp(tchszName[i], lpszVariableName) == 0)
			return TRUE;
	}
	return FALSE;
}

// 
// Return
//   0: not dependent
//   1: name dependent
//   2: value dependent
int CaIngresVariable::IsIIDependent(LPCTSTR lpszVariableName, LPCTSTR lpszII, CString& strFormatName)
{
	const int nCount = 6;
	TCHAR tchszName[nCount][20] = 
	{
		_T("II_INSTALLATION"),
		_T("II_CHARSET%s"),
		_T("II_GCN%s_PORT"),
		_T("II_GCN%s_LCL_VNODE"),
		_T("II_BIND_SVC_%s"),
		_T("II_GC%s_TRACE")
	};

	strFormatName = _T("");
	CString s;
	for (int i=0; i<nCount; i++)
	{
		if (i==0)
		{
			if (_tcsicmp (lpszVariableName, tchszName[i]) == 0)
				return 2;
		}
		else
		{
			s.Format(tchszName[i], lpszII);
			if (s.CompareNoCase (lpszVariableName) == 0)
			{
				strFormatName = tchszName[i];
				return 1;
			}
		}
	}
	return 0;
}

CaEnvironmentParameter* CaEnvironmentParameter::Search (LPCTSTR lpszName, CTypedPtrList<CObList, CaEnvironmentParameter*>& lev)
{
	POSITION pos = lev.GetHeadPosition();
	while (pos != NULL)
	{
		CaEnvironmentParameter* pEnv = lev.GetNext(pos);
		if (pEnv->GetName().CompareNoCase(lpszName) == 0)
			return pEnv;
	}

	return NULL;
}


// Check the values of parameters:
// Input : listEnvirenment contains the names of parameters with their values are empty.
// Output: listEnvirenment contains the couples (name, value). If the value is not empty
//         it means the parameter is not set or set to empty.
void INGRESII_CheckVariable (CTypedPtrList<CObList, CaNameValue*>& listEnvirenment)
{
	BOOL bOK = FALSE;
	CTypedPtrList<CObList, CaEnvironmentParameter*> lev;
	CaIngresVariable env;
	env.Init();
	env.QueryEnvironment (lev, FALSE, FALSE);

	POSITION pos = listEnvirenment.GetHeadPosition();
	while (pos != NULL)
	{
		CaNameValue* pEnv = listEnvirenment.GetNext(pos);
		CaEnvironmentParameter* pExist = CaEnvironmentParameter::Search(pEnv->GetName(), lev);
		if (pExist && !pExist->GetValue().IsEmpty())
		{
			pEnv->SetValue(pExist->GetValue());
		}
	}

	while (!lev.IsEmpty())
		delete lev.RemoveHead();
}
