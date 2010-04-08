/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcda.cpp : Implementation of CVcdaApp and DLL registration.
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Initialize Instance
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 25-Oct-2004 (noifr01)
**    set the locale correctly for ensuring functions such
**    as _istleadbyte() work correctly
*/

#include "stdafx.h"
#include "vcda.h"
#include <locale.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CVcdaApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid = { 0xeaf345e7, 0xd514, 0x11d6, { 0x87, 0xea, 0, 0xc0, 0x4f, 0x1f, 0x75, 0x4a } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


////////////////////////////////////////////////////////////////////////////
// CVcdaApp::InitInstance - DLL initialization

BOOL CVcdaApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();
	if (bInit)
	{
		_tsetlocale(LC_CTYPE ,"");	
	}

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CVcdaApp::ExitInstance - DLL termination

int CVcdaApp::ExitInstance()
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

