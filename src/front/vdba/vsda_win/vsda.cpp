/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsda.cpp : Implementation of CappSda and DLL registration.
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : main header file for VSDA.OCX
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 22-Nov-2004 (schph01)
**    Bug #113511 initialize _tsetlocale() function according to the
**    LC_COLLATE category setting of the current locale.
*/


#include "stdafx.h"
#include "vsda.h"
#include "tkwait.h"   // TaskAnimateInitialize(), 
#include <locale.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CappSda NEAR theApp;
const GUID CDECL BASED_CODE _tlid = { 0xcc2da2b3, 0xb8f1, 0x11d6, { 0x87, 0xd8, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;

////////////////////////////////////////////////////////////////////////////
// CappSda::InitInstance - DLL initialization

BOOL CappSda::InitInstance()
{
	_tsetlocale(LC_COLLATE  ,_T(""));
	_tsetlocale(LC_CTYPE  ,_T(""));
	BOOL bInit = COleControlModule::InitInstance();
	if (bInit)
	{
		AfxEnableControlContainer();
		TaskAnimateInitialize();
		m_sessionManager.SetDescription(_T("Ingres Visual Database Objects Differences Analyzer"));
	}
	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CappSda::ExitInstance - DLL termination

int CappSda::ExitInstance()
{
	// TODO: Add your own module termination code here.

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
