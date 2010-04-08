/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : stdafx.cpp : source file that includes just the standard includes
**               libguids.pch will be the pre-compiled header 
**               stdafx.obj will contain the pre-compiled type information
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : QUIDS Management
**
** History:
**
** 29-Nov-2000 (uk$so01)
**    Created
** 29-Jun-2001 (hanje04)
**    Moved } outside #ifdef (MAINWIN) so that 'else' is aways closed.
** 23-Oct-2000 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 08-May-2009 (drivi01)
**    In effort to port to Visual Studio 2008, clean up the warnings.
**/


#include "stdafx.h"
#include "libguids.h"
#include "constdef.h"
#include <stdio.h>
#include <tchar.h>

//
// ICAS COM Server MTA Apartment:
// ************************************************************************************************
extern "C" LPCTSTR cstrCLSID_ICASServer             = TEXT("{2185DF31-C790-11d4-872D-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IQueryObject             = TEXT("{2185DF32-C790-11d4-872D-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IExecSQL                 = TEXT("{2185DF33-C790-11d4-872D-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IQueryDetail             = TEXT("{2185DF34-C790-11d4-872D-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IClientStateNotification = TEXT("{2185DF35-C790-11d4-872D-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IRmcmd                   = TEXT("{2185DF36-C790-11d4-872D-00C04F1F754A}");

extern "C" LPCTSTR cstrIID_ISinkDataChange          = TEXT("{2185DF3A-C790-11d4-872D-00C04F1F754A}");

extern "C" LPCTSTR cstrCLSID_SDCNServer             = TEXT("{2185DF3B-C790-11d4-872D-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_ISourceSDCN              = TEXT("{2185DF3C-C790-11d4-872D-00C04F1F754A}");

extern "C" const IID CLSID_ICASServer               = {0x2185DF31, 0xC790, 0x11d4, {0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IQueryObject               = {0x2185DF32, 0xC790, 0x11d4, {0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IExecSQL                   = {0x2185DF33, 0xC790, 0x11d4, {0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IQueryDetail               = {0x2185DF34, 0xC790, 0x11d4, {0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IClientStateNotification   = {0x2185DF35, 0xC790, 0x11d4, {0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IRmcmd                     = {0x2185DF36, 0xC790, 0x11d4, {0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};

extern "C" const IID IID_ISinkDataChange            = {0x2185DF3A, 0xC790, 0x11d4, {0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};

extern "C" const IID CLSID_SDCNServer               = {0x2185DF3B, 0xC790, 0x11d4, {0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_ISourceSDCN                = {0x2185DF3C, 0xC790, 0x11d4, {0x87, 0x2D, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};

//
// IMPORT & EXPORT ASSISTANT:
// ************************************************************************************************
extern "C" LPCTSTR cstrCLSID_IMPxEXPxASSISTANCT     = TEXT("{57CE8441-CF4E-11d4-872E-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IImportAssistant         = TEXT("{57CE8442-CF4E-11d4-872E-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IExportAssistant         = TEXT("{57CE8443-CF4E-11d4-872E-00C04F1F754A}");
extern "C" const IID CLSID_IMPxEXPxASSISTANCT       = {0x57CE8441, 0xCF4E, 0x11d4, {0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IImportAssistant           = {0x57CE8442, 0xCF4E, 0x11d4, {0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IExportAssistant           = {0x57CE8443, 0xCF4E, 0x11d4, {0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};


//
// SQL ASSISTANT:
// ************************************************************************************************
#if defined (EDBC)
extern "C" LPCTSTR cstrCLSID_SQLxASSISTANCT         = TEXT("{FC918E94-E9DC-4497-AD7E-7BF28DF7FD14}");
extern "C" LPCTSTR cstrIID_ISqlAssistant            = TEXT("{B053C09F-D831-4f4b-9929-21903C0230C3}");
extern "C" const IID CLSID_SQLxASSISTANCT           = {0xfc918e94, 0xe9dc, 0x4497, {0xad, 0x7e, 0x7b, 0xf2, 0x8d, 0xf7, 0xfd, 0x14}};
extern "C" const IID IID_ISqlAssistant              = {0xb053c09f, 0xd831, 0x4f4b, {0x99, 0x29, 0x21, 0x90, 0x3c, 0x02, 0x30, 0xc3}};
#else
extern "C" LPCTSTR cstrCLSID_SQLxASSISTANCT         = TEXT("{57CE8444-CF4E-11d4-872E-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_ISqlAssistant            = TEXT("{57CE8445-CF4E-11d4-872E-00C04F1F754A}");
extern "C" const IID CLSID_SQLxASSISTANCT           = {0x57CE8444, 0xCF4E, 0x11d4, {0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_ISqlAssistant              = {0x57CE8445, 0xCF4E, 0x11d4, {0x87, 0x2E, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
#endif

//
// Add/Alter/Drop Dialog (In-Process Server):
// ************************************************************************************************
extern "C" LPCTSTR cstrCLSID_DIALOGxADDxALTERxDROP  = TEXT("{D0338860-8264-11d5-8760-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IAdd                     = TEXT("{D0338861-8264-11d5-8760-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IAlter                   = TEXT("{D0338862-8264-11d5-8760-00C04F1F754A}");
extern "C" LPCTSTR cstrIID_IDrop                    = TEXT("{D0338863-8264-11d5-8760-00C04F1F754A}");
extern "C" const IID CLSID_DIALOGxADDxALTERxDROP    = {0xD0338860, 0x8264, 0x11d5, {0x87, 0x60, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IAdd                       = {0xD0338861, 0x8264, 0x11d5, {0x87, 0x60, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IAlter                     = {0xD0338862, 0x8264, 0x11d5, {0x87, 0x60, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};
extern "C" const IID IID_IDrop                      = {0xD0338863, 0x8264, 0x11d5, {0x87, 0x60, 0x00, 0xC0, 0x4F, 0x1F, 0x75, 0x4A}};



//
// ************************************************************************************************
// Register Funtions:
//
// Local function: check to see if CLSID is registered under the
// HKEY_CLASSES_ROOT\CLSID
BOOL LIBGUIDS_IsCLSIDRegistered(LPCTSTR lpszCLSID)
{
	HKEY hkResult = 0;
	TCHAR tchszFull[512];
	lstrcpy (tchszFull, _T("CLSID\\"));
	lstrcat (tchszFull, lpszCLSID);
	long lRes = RegOpenKey (
		HKEY_CLASSES_ROOT,
		tchszFull,
		&hkResult);
	if (hkResult)
		RegCloseKey(hkResult);
	if (lRes != ERROR_SUCCESS)
	{
		return FALSE;
	}
	return TRUE;
}


//
// lpszServerName: Dll name, ex: svriia.dll, ijactrl.ocx, ...
// lpszPath      : If this parameter is NULL, the function will use
//                 II_SYSTEM\ingres\vdba as path name.
// return:
//     0: successful.
//     1: fail
//     2: fail because of II_SYSTEM is not defined.
int LIBGUIDS_RegisterServer(LPCTSTR lpszServerName, LPCTSTR lpszPath)
{
	TCHAR tchszFull[512];
	TCHAR tchBackSlash;
#if defined (MAINWIN)
	tchBackSlash = _T('/');
#else
	tchBackSlash = _T('\\');
#endif


	if (lpszPath)
	{
		//
		// Is the path has the trailing '\' ?
		BOOL bEnd = FALSE;
		TCHAR* pText = (TCHAR *)_tcsrchr (lpszPath, tchBackSlash);
		if (pText && (*_tcsinc(pText) == _T('\0')))
			bEnd = TRUE;
		if (bEnd)
			_stprintf (tchszFull, _T("%s%s"), lpszPath, lpszServerName);
		else
		{
			_stprintf (tchszFull, _T("%s%c%s"), lpszPath, tchBackSlash, lpszServerName);
		}
	}
	else
	{
		BOOL bOK = TRUE;
		TCHAR* pEnv = _tgetenv(consttchszII_SYSTEM);
		if (!pEnv)
			return 2;
		_stprintf (tchszFull, _T("%s%s%s"), pEnv, consttchszIngVdba, lpszServerName);
	}

	BOOL bOK = TRUE;
	HINSTANCE hLib = LoadLibrary(tchszFull);
	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
		bOK = FALSE;
	if (bOK)
	{
		FARPROC lpDllEntryPoint = GetProcAddress (hLib, "DllRegisterServer");
		if (lpDllEntryPoint == NULL)
			bOK = FALSE;
		else
			(*lpDllEntryPoint)();
		FreeLibrary(hLib);
	}

	if (!bOK)
		return 1;

	return 0;
}

