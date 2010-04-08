/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlasinf.h: is the common include file for the SQL Assistant
**    Project  : Meta Data / COM Server 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Interface of Ingres SQL Assistant
**
** History:
**
** 23-Oct-2001 (uk$so01)
**    Created, SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
**/

#if !defined(INTERFACE_SQL_ASSISTANT_HEADER)
#define INTERFACE_SQL_ASSISTANT_HEADER
#if !defined(RC_INCLUDE)
#include "libextnc.h"

#if defined (__cplusplus)
//
// Interface IImportAssistant
DECLARE_INTERFACE_(ISqlAssistant, IUnknown)
{
	// IUnknown methods.
	STDMETHOD(QueryInterface) (THIS_ REFIID, LPVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)  (THIS) PURE;
	STDMETHOD_(ULONG,Release) (THIS) PURE;

	// IImportAssistant methods.
	STDMETHOD(SqlAssistant)  (THIS_ HWND, SQLASSISTANTSTRUCT* pStruct, BSTR* bstrStatement) PURE;
	STDMETHOD(AboutBox) (THIS_ HWND) PURE;
};




#endif // __cplusplus

#endif // RC_INCLUDE
#endif // INTERFACE_SQL_ASSISTANT_HEADER
