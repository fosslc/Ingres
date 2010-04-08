/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlquery.cpp : Implementation of CSqlqueryApp and DLL registration.
**    Project  : sqlquery.ocx 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main application class
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 27-May-2002 (uk$so01)
**    BUG #107880, Add the XML decoration encoding='UTF-8' or 'WINDOWS-1252'...
**    depending on the Ingres II_CHARSETxx.
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 05-Nov-2004 (uk$so01)
**    BUG #113389 / ISSUE #13765425 Simplified Chinese WinXP
**    Part of the statement in multiple statements execution could
**    be incorrectly highlighted.
**/


#include "stdafx.h"
#include "sqlquery.h"
#include "qredoc.h"
#include "ingobdml.h" // Low level query objects
#include "tkwait.h"   // TaskAnimateInitialize(), 
#include <locale.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CSqlqueryApp NEAR theApp;
#if defined (EDBC)
// {ED36274A-0B58-41f1-8A4B-B111338B5A15}
const GUID CDECL BASED_CODE _tlid =
	{ 0xed36274a, 0x0b58, 0x41f1, { 0x8a, 0x4b, 0xb1, 0x11, 0x33, 0x8b, 0x5a, 0x15 } };
#else
// {634C383A-A069-11D5-8769-00C04F1F754A}
const GUID CDECL BASED_CODE _tlid =
	{ 0x634c383a, 0xa069, 0x11d5, { 0x87, 0x69, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
#endif

const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


////////////////////////////////////////////////////////////////////////////
// CSqlqueryApp::InitInstance - DLL initialization
CSqlqueryApp::CSqlqueryApp()
{
	lstrcpy (m_tchszOutOfMemoryMessage, _T("Low of Memory ...\nCannot allocate memory, please close some applications !"));
	m_strHelpFile = _T("sqltest.hlp");
	m_strInstallationID = _T("");
	m_nCursorSequence = 0;
	m_nSqlAssistantStart = 100;
	m_strEncoding = _T("UTF-8");
}


//
// This function should call the query objects from the Static library ("INGRESII_llQueryObject") or
// from the COM Server ICAS.
BOOL CSqlqueryApp::INGRESII_QueryObject(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject)
{
	CaSessionManager& ssMgr = GetSessionManager();
	BOOL bOk = INGRESII_llQueryObject (pInfo, listObject, &ssMgr);
	return bOk;
}

BOOL CSqlqueryApp::InitInstance()
{
	_tsetlocale(LC_CTYPE ,"");
	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		AfxEnableControlContainer();
		TaskAnimateInitialize();
		m_strInstallationID = INGRESII_QueryInstallationID();
	}

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CSqlqueryApp::ExitInstance - DLL termination

int CSqlqueryApp::ExitInstance()
{
	while (!m_listDoc.IsEmpty())
		delete m_listDoc.RemoveHead();

	return COleControlModule::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}
