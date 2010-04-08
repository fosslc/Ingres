/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h : include file for standard system include files
**    Project  : Extension DLL (Task Animation).
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Precompile Header
**
**  History:
**  07-Feb-1999 (uk$so01)
**     created
**  09-Mar-2004 (noifr01)
**    defined WINVER for fixing various problems when building with recent
**    version(s) of compiler
**  18-Jun-2004 (schph01)
**    (SIR 111014) Delete two includes afxdb.h and afxdao.h generating some
**    build problem on certain machine.
**  20-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
**  12-May-2009 (drivi01)
**    In effort to port to Visual Studio 2008, define WIN32_WINNT version.
*/

//
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
#define _WIN32_WINNT 0x0500

#if !defined(AFX_STDAFX_H__345CA318_7C07_11D3_A310_00609725DDAF__INCLUDED_)
#define AFX_STDAFX_H__345CA318_7C07_11D3_A310_00609725DDAF__INCLUDED_

#define WINVER 0x0400

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdllx.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC OLE automation classes
#endif // _AFX_NO_OLE_SUPPORT


#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

//
// Maintain the resource handle of the Extention DLL
class CaExtensionState
{
public:
	CaExtensionState();
	~CaExtensionState();
protected:
	HINSTANCE m_hInstOld;
};



#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__345CA318_7C07_11D3_A310_00609725DDAF__INCLUDED_)
