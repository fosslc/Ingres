/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**    Source   : iia.cpp : Implementation of Ingres Import Assistant Interfaces
**    Project  : IIA for C-Application
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : IIA for C-Application
**
** History:
**
** 18-May-2001 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 14-Fev-2002 (uk$so01)
**    SIR #106648, Enhance library.
** 03-Feb-2004 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX or COM:
**    Integrate Export assistant into vdba
** 20-Aug-2008 (whiro01)
**    Move <afx...> include to "stdafx.h"
**/


#include "stdafx.h"
#include <objbase.h>
#include "libextnc.h"
#include "libguids.h" // guids
#include "impexpas.h" // import assistant interface
#include "sqlasinf.h" // sql wizard assistant interface

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static HRESULT GetInterfacePointer(IUnknown* pObj, REFIID riid, PVOID* ppv)
{
	HRESULT hError = NOERROR;

	*ppv=NULL;

	if (NULL != pObj)
	{
		hError = pObj->QueryInterface(riid, ppv);
	}
	return hError;
}

/*
** Import Assistant Interfaces:
** ************************************************************************************************
*/

/*
**  Function: ImportAssistant
**
**  Summary:  Invoke the wizard Ingres Import Assistant dialog.
**
**  Args:
**    -hwndCaller : Handle of Window of the caller.
**    -pStruct    : Pointer to the structure IIASTRUCT to passe extra information if needed.
**
**  Returns:  int
**    1 = OK.
**    2 = User has cancelled the import assistant
**    3 = FAIL (Failed to initialize COM Library)
**    4 = (REGDB_E_CLASSNOTREG): 
**        A specified class is not registered in the registration database. Also can indicate that 
**        the type of server you requested in the CLSCTX enumeration is not registered or the values 
**        for the server types in the registry are corrupt.
**    5 = (CLASS_E_NOAGGREGATION):
**        This class cannot be created as part of an aggregate. 
**    6 = (E_NOINTERFACE):
**        Fail to query interface pointer.
*/
static int ImportAssistant(HWND hwndCaller, IIASTRUCT* pStruct)
{
	USES_CONVERSION;
	CString strMsg;
	int nResult = 1;
	HRESULT hError = NOERROR;

	try
	{
		hError = CoInitialize(NULL);
		if (FAILED(hError))
		{
			hError = E_FAIL;
			nResult = 3;
		}
		else
		{
			IUnknown*   pAptImportAssistant = NULL;
			IImportAssistant* pIia;

			hError = CoCreateInstance(
				CLSID_IMPxEXPxASSISTANCT,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IUnknown,
				(PVOID*)&pAptImportAssistant);

			if (SUCCEEDED(hError))
			{
				hError = GetInterfacePointer(pAptImportAssistant, IID_IImportAssistant, (LPVOID*)&pIia);
				if (SUCCEEDED(hError))
				{
					if (!pStruct)
						pIia->ImportAssistant (hwndCaller);
					else
						pIia->ImportAssistant2 (hwndCaller, pStruct);
					pIia->Release();
				}
				else
				{
					nResult = 6;
				}
				
				pAptImportAssistant->Release();
				CoFreeUnusedLibraries();
			}
			else
			{
				switch (hError)
				{
				case E_FAIL:
					nResult = 3;
					break;
				case REGDB_E_CLASSNOTREG:
					nResult = 4;
					break;
				case CLASS_E_NOAGGREGATION:
					nResult = 5;
					break;
				case E_NOINTERFACE:
					nResult = 6;
					break;
				default:
					nResult = 3;
					break;
				}
			}
			CoUninitialize ();
		}
	}
	catch (...)
	{
		nResult = 3;
	}
	return nResult;
}

int Ingres_ImportAssistant(HWND hwndCaller)
{
	return ImportAssistant(hwndCaller, NULL);
}

int Ingres_ImportAssistantWithParam(HWND hwndCaller, IIASTRUCT* pStruct)
{
	return ImportAssistant(hwndCaller, pStruct);
}


/*
**  Function: ExportAssistant
**
**  Summary:  Invoke the wizard Ingres Export Assistant dialog.
**
**  Args:
**    -hwndCaller : Handle of Window of the caller.
**    -pStruct    : Pointer to the structure IEASTRUCT to passe extra information if needed.
**
**  Returns:  int
**    1 = OK.
**    2 = User has cancelled the import assistant
**    3 = FAIL (Failed to initialize COM Library)
**    4 = (REGDB_E_CLASSNOTREG): 
**        A specified class is not registered in the registration database. Also can indicate that 
**        the type of server you requested in the CLSCTX enumeration is not registered or the values 
**        for the server types in the registry are corrupt.
**    5 = (CLASS_E_NOAGGREGATION):
**        This class cannot be created as part of an aggregate. 
**    6 = (E_NOINTERFACE):
**        Fail to query interface pointer.
*/
static int ExportAssistant(HWND hwndCaller, IEASTRUCT* pStruct)
{
	USES_CONVERSION;
	CString strMsg;
	int nResult = 1;
	HRESULT hError = NOERROR;

	try
	{
		hError = CoInitialize(NULL);
		if (FAILED(hError))
		{
			hError = E_FAIL;
			nResult = 3;
		}
		else
		{
			IUnknown*   pAptExportAssistant = NULL;
			IExportAssistant* pIea;

			hError = CoCreateInstance(
				CLSID_IMPxEXPxASSISTANCT,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_IUnknown,
				(PVOID*)&pAptExportAssistant);

			if (SUCCEEDED(hError))
			{
				hError = GetInterfacePointer(pAptExportAssistant, IID_IExportAssistant, (LPVOID*)&pIea);
				if (SUCCEEDED(hError))
				{
					if (!pStruct)
						pIea->ExportAssistant (hwndCaller);
					else
						pIea->ExportAssistant2 (hwndCaller, pStruct);
					pIea->Release();
				}
				else
				{
					nResult = 6;
				}
				
				pAptExportAssistant->Release();
				CoFreeUnusedLibraries();
			}
			else
			{
				switch (hError)
				{
				case E_FAIL:
					nResult = 3;
					break;
				case REGDB_E_CLASSNOTREG:
					nResult = 4;
					break;
				case CLASS_E_NOAGGREGATION:
					nResult = 5;
					break;
				case E_NOINTERFACE:
					nResult = 6;
					break;
				default:
					nResult = 3;
					break;
				}
			}
			CoUninitialize ();
		}
	}
	catch (...)
	{
		nResult = 3;
	}
	return nResult;
}


int Ingres_ExportAssistant(HWND hwndCaller)
{
	return ExportAssistant(hwndCaller, NULL);
}

int Ingres_ExportAssistantWithParam(HWND hwndCaller, IEASTRUCT* pStruct)
{
	return ExportAssistant(hwndCaller, pStruct);
}



/*
** SQL Wizard Assistant Interfaces:
** ************************************************************************************************
*/

/*
**  Function: Ingres_SQLAssistant
**
**  Summary:  Invoke the wizard Ingres SQL Assistant dialog.
**
**  Args:
**    -hwndCaller : Handle of Window of the caller.
**    -pStruct    : Pointer to the structure SQLASSISTANTSTRUCT to passe extra information if needed.
**    -pBstrStatement: [out] the result sql statement.
**
**  Returns:  int
**    1 = OK.
**    2 = User has cancelled the SQL assistant
**    3 = FAIL (Failed to initialize COM Library)
**    4 = (REGDB_E_CLASSNOTREG): 
**        A specified class is not registered in the registration database. Also can indicate that 
**        the type of server you requested in the CLSCTX enumeration is not registered or the values 
**        for the server types in the registry are corrupt.
**    5 = (CLASS_E_NOAGGREGATION):
**        This class cannot be created as part of an aggregate. 
**    6 = (E_NOINTERFACE):
**        Fail to query interface pointer.
*/
int Ingres_SQLAssistant(HWND hwndCaller, SQLASSISTANTSTRUCT* pStruct, BSTR* pBstrStatement)
{
	USES_CONVERSION;
	int nResult = 1;
	IUnknown*   pAptSqlAssistant = NULL;
	ISqlAssistant* pSqlAssistant;
	HRESULT hError = NOERROR;

	try
	{
		hError = CoCreateInstance(
			CLSID_SQLxASSISTANCT,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUnknown,
			(PVOID*)&pAptSqlAssistant);

		if (SUCCEEDED(hError))
		{
			hError = GetInterfacePointer(pAptSqlAssistant, IID_ISqlAssistant, (LPVOID*)&pSqlAssistant);
			if (SUCCEEDED(hError))
			{
				hError = pSqlAssistant->SqlAssistant (hwndCaller, pStruct, pBstrStatement);
				pSqlAssistant->Release();
			}
			else
			{
				nResult = 6;
			}
			pAptSqlAssistant->Release();
			CoFreeUnusedLibraries();
		}
		else
		{
			switch (hError)
			{
			case E_FAIL:
				nResult = 3;
				break;
			case REGDB_E_CLASSNOTREG:
				nResult = 4;
				break;
			case CLASS_E_NOAGGREGATION:
				nResult = 5;
				break;
			case E_NOINTERFACE:
				nResult = 6;
				break;
			default:
				nResult = 3;
				break;
			}
		}
	}
	catch(...)
	{
		nResult = 3;
	}
	return nResult;
}

TOBJECTLIST* TOBJECTLIST_Add (TOBJECTLIST* lpList, TOBJECTLIST* lpObj)
{
	TOBJECTLIST* lpNewObj = new TOBJECTLIST;
	memcpy (lpNewObj, lpObj, sizeof(TOBJECTLIST));
	if (!lpList)
	{
		lpNewObj->pNext  = NULL;
		lpList = lpNewObj;
	}
	else
	{
		TOBJECTLIST* prev = NULL;
		TOBJECTLIST* lp = lpList;
		while (lp)
		{
			prev = lp;
			lp   = lp->pNext;
		}
		lpNewObj->pNext = NULL;
		prev->pNext = lpNewObj;
	}
	return lpList;
}

TOBJECTLIST* TOBJECTLIST_Done(TOBJECTLIST* lpList)
{
	TOBJECTLIST* lpTemp;
	while (lpList)
	{
		lpTemp = lpList;
		lpList = lpList->pNext;
		delete lpTemp;
	}
	return NULL;
}

