/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h , Header File
**    Purpose  : include file for standard system include files,
**               or project specific include files that are used frequently,
**               but are changed infrequently
**
** History:
**
** 10-Mar-2004 (schph01)
**    (BUG #111014), defined WINVER for fixing various problems when building
**    with recent version(s) of compiler
**  21-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 07-May-2009 (drivi01)
**    Define WINVER in effort to port to Visual Studio 2008.
** 10-Mar-2010 (drivi01)
**    SIR 123397, update return code for INGRESII_NodeCheckConnection to be
**    STATUS instead of BOOL.  This will allow the function to return 
**    error code which can be used to retrieve error message via ERlookup
**    function.  Define needed header files.
**/

#if !defined(AFX_STDAFX_H__2D0C26C8_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
#define AFX_STDAFX_H__2D0C26C8_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_

#define WINVER 0x0500

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
#include <afxcview.h>
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>       // MFC Template classes
#include "portto64.h"
#include "ingobdml.h"
#include "vnodedml.h"
extern  "C"
{
#include <compat.h>
#include <er.h>
}


#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__2D0C26C8_E5AC_11D2_A2D9_00609725DDAF__INCLUDED_)
