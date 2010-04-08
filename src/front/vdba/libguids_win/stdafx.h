/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : stdafx.h : include file for standard system include files,
**               or project specific include files that are used frequently, but 
**               are changed infrequently
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : QUIDS Management
**
** History:
**
** 29-Nov-2000 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057 (sqltest as ActiveX & Sql Assistant as In-Process COM Server)
** 09-Mar-2004 (noifr01)
**    defined WINVER for fixing various problems when building with recent
**    version(s) of compiler
**  20-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 07-May-2009 (drivi01)
**    Define _WIN32_WINNT in effort to port to Visual Studio 2008.
**/
#define _WIN32_WINNT 0x0500

#if !defined(AFX_STDAFX_H__15BE48A3_C488_11D4_872D_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__15BE48A3_C488_11D4_872D_00C04F1F754A__INCLUDED_

#define WINVER 0x0400

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN  // Exclude rarely-used stuff from Windows headers

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#include <afx.h>
#include <afxwin.h>

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>   // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h> // MFC Template classes
#include <objidl.h>

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__15BE48A3_C488_11D4_872D_00C04F1F754A__INCLUDED_)
