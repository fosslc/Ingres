/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : impexpas.h: is the common include file for the Import & Export Assistant
**    Project  : Meta Data / COM Server 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Interface of Ingres Import & Export Assistant
**
** History:
**
** 12-Dec-2000 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
**/

#if !defined(IIMPORTASSISTANTxIEXPORTASSISTANT_HEADER)
#define IIMPORTASSISTANTxIEXPORTASSISTANT_HEADER
#if !defined(RC_INCLUDE)
#include "libextnc.h"

#if defined (__cplusplus)
//
// Interface IImportAssistant
DECLARE_INTERFACE_(IImportAssistant, IUnknown)
{
	// IUnknown methods.
	STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	// IImportAssistant methods.
	STDMETHOD(ImportAssistant)   (THIS_ HWND) PURE;
	STDMETHOD(ImportAssistant2)  (THIS_ HWND, IIASTRUCT* pStruct) PURE;
	STDMETHOD(AboutBox) (THIS_ HWND) PURE;
};


//
// Interface IExportAssistant
DECLARE_INTERFACE_(IExportAssistant, IUnknown)
{
	// IUnknown methods.
	STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	// IImportAssistant methods.
	STDMETHOD(ExportAssistant)   (THIS_ HWND) PURE;
	STDMETHOD(ExportAssistant2)  (THIS_ HWND, IEASTRUCT* pStruct) PURE;
	STDMETHOD(AboutBox) (THIS_ HWND) PURE;
};

#endif // __cplusplus

#endif // RC_INCLUDE
#endif // IIMPORTASSISTANTxIEXPORTASSISTANT_HEADER
