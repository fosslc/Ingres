/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : istdafx.h : Header File.
**    Project  : Import Assistant (IIA)
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : include file for standard system include files
**               or project specific include files that are used frequently,
**               but are changed infrequently
**
** History:
**
** 12-Mar-2001 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
** 28-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
** 11-Feb-2004 (noifr01)
**    defined WINVER for fixing various problems when building with recent
**    version(s) of compiler
** 13-Jun-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 12-May-2009 (drivi01)
**    In effort to port to visual Studio 2008, update WINVER.
*/


#if !defined(AFX_STDAFX_H__3BFB8EB5_B55B_11D4_8728_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__3BFB8EB5_B55B_11D4_8728_00C04F1F754A__INCLUDED_

#define WINVER 0x0500

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN         // Exclude rarely-used stuff from Windows headers

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#include <afx.h>
#include <afxwin.h>

// TODO: reference additional headers your program requires here
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>       // MFC Template classes
#include <afxext.h>
#include <afxdlgs.h>
#include "portto64.h"

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__3BFB8EB5_B55B_11D4_8728_00C04F1F754A__INCLUDED_)
