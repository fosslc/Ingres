/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ingobdml.cpp: implementation for including the file generated
**               from ESQLCC -multi -fdetaidml.inc detaidml.scc
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Access low lewel data (Ingres Database)
**
** History:
**
** 23-Dec-1999 (uk$so01)
**    Created
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
** 04-Apr-2002 (uk$so01)
**    BUG #107506, Handle the reserved keyword
** 10-Apr-2002 (uk$so01)
**    BUG #107506, Add the function INGRESII_llQuote to automatically quote string.
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 18-Sep-2003 (uk$so01)
**    SIR #106648, Integrate the libingll.lib and cleanup codes.
** 10-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 13-Oct-2003 (schph01)
**   SIR 109864 Add functions INGRESII_MuteRefresh() and
**   INGRESII_UnMuteRefresh for low level optimization.
** 30-Oct-2003 (noifr01)
**    fixed propagation error upon massive ingres30->main propagation
** 20-Aug-2008 (whiro01)
**    Remove redundant <afx...> includes (already in stdafx.h)
**/


#include "stdafx.h"
#include "ingobdml.h"
#include "constdef.h"
#include "dmlcolum.h"
#include "usrexcep.h"
#include <winsock2.h>

extern "C"
{
#include "nodes.h"
#include <compat.h>
#include <er.h>
#include <nm.h>
#include <gc.h>
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static LPCTSTR cstrIngresSpecialChars = _T("$&*:,\"=/<>()-%.+?;' |");

BOOL INGRESII_llIsReservedIngresKeyword(LPCTSTR lpszIngresObject)
{
	if (_tcsicmp (lpszIngresObject, _T("user")) == 0)
		return TRUE;
	if (_tcsicmp (lpszIngresObject, _T("key")) == 0)
		return TRUE;

	return FALSE;
}

BOOL INGRESII_llIsNeededQuote(LPCTSTR lpszIngresObject)
{
	CString strObj = lpszIngresObject;
	if (strObj.FindOneOf (cstrIngresSpecialChars) != -1)
		return TRUE;
	return FALSE;
}

//
// Return the quote string if neccessary:
// The text qualifier is tchTQ.
CString INGRESII_llQuoteIfNeeded(LPCTSTR lpszIngresObject, TCHAR tchTQ, BOOL bIngresCommand)
{
	CString strObj = lpszIngresObject;
	BOOL bNeedQuote = INGRESII_llIsNeededQuote(lpszIngresObject);

	if (bNeedQuote)
	{
		strObj = INGRESII_llQuote(lpszIngresObject, tchTQ, bIngresCommand);
	}
	return strObj;
}

//
// Always Return the quote string:
// The text qualifier is tchTQ.
CString INGRESII_llQuote(LPCTSTR lpszIngresObject, TCHAR tchTQ, BOOL bIngresCommand)
{
	CString strObj = lpszIngresObject;
	BOOL bNeedQuote = INGRESII_llIsNeededQuote(lpszIngresObject);
	if (bNeedQuote)
	{
		CString strRes = strObj;
		int nDQ = strObj.Find (_T('\"'));
		if (nDQ != -1)
		{
			strRes = _T("");
			while (nDQ != -1)
			{
				if (bIngresCommand)
				{
					//
					// The " is changed to \\\"
					strRes += strObj.Left (nDQ);
					strRes += _T("\\\\\\\"");
					strObj = strObj.Mid(nDQ+1);
				}
				else
				{
					strRes += strObj.Left (nDQ+1);
					strRes += _T('\"');
					strObj = strObj.Mid(nDQ+1);
				}

				nDQ = strObj.Find (_T('\"'));
			}
			strRes += strObj;
		}

		if (bIngresCommand)
			strObj.Format (_T("%c\\\"%s\\\"%c"), tchTQ, (LPCTSTR)strRes, tchTQ);
		else
			strObj.Format (_T("%c%s%c"), tchTQ, (LPCTSTR)strRes, tchTQ);
	}
	else
	{
		strObj.Format (_T("%c%s%c"), tchTQ, lpszIngresObject, tchTQ);
	}

	return strObj;
}



BOOL INGRESII_llIsSystemObject (LPCTSTR lpszObject, LPCTSTR lpszObjectOwner, int nOTType)
{
	CString strOwner = lpszObjectOwner;
	strOwner.MakeLower();
	if (!strOwner.IsEmpty())
	{
		if (strOwner.Find(_T("$ingres")) != -1)
			return TRUE;
		if (strOwner.Find(_T("\"$")) != -1)
			return TRUE;
		if (strOwner[0] == _T('$'))
			return TRUE;
	}

	ASSERT(lpszObject);
	if (!lpszObject)
		return FALSE;
	if (lpszObject[0] == _T('$'))
		return TRUE;
	if (_tcsnicmp (lpszObject, _T("ii"), 2) == 0)
		return TRUE;
	//
	// Prefixed dd_ of table name:
	// dd_ is replicator tables, and the owners are not $INGRES !!!
	// If we decide to TELL THAT the dd_ tables are SYSTEM OBJECTS then the dd_ tables 
	// created by user will not be CONSIDERED as SYSTEM OBJECTS.
	// 
	if (_tcsnicmp (lpszObject, _T("dd_"), 3) == 0)
		return TRUE;
	
	return FALSE;
}


CString GetLocalHostName()
{
#if defined (WINDOWS_SOCKET)
	class CaSockInit
	{
	public:
		CaSockInit()
		{
			WSADATA  winsock_data;
			m_bOk = WSAStartup(MAKEWORD(2,0), & winsock_data) == 0;
		}
		~CaSockInit() {WSACleanup();}

		BOOL m_bOk;
	};

	CString strLocalHostName = _T("");
	CaSockInit wsainit;
	if (!wsainit.m_bOk)
		return _T("");

	//
	// Get Local Host Name:
	char hostname[256];
	int  nLen = 255;
	if (gethostname(hostname, nLen) == 0)
	{
		strLocalHostName = hostname;
		return strLocalHostName;
	}
#else
	TCHAR tchszGCHostName[256];
	tchszGCHostName[0] = _T('\0');
	GChostname(tchszGCHostName, 256);
	ASSERT (tchszGCHostName[0]);
	if (!tchszGCHostName[0])
		return _T("UNKNOWN LOCAL HOST NAME");
	CString csLocal = tchszGCHostName;
	return csLocal;
#endif
	return _T("");
}

void INGRESII_MuteRefresh()
{
	MuteLLRefresh();
}
void INGRESII_UnMuteRefresh()
{
	UnMuteLLRefresh();
}

//
// This file "" is generated from ESQLCC -multi -fingobdml.inc ingobdml.scc
#include "ingobdml.inc"



