/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : istdafx.h : Header File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : include file for standard system include files
**               or project specific include files that are used frequently,
**               but are changed infrequently
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 09-Mar-2004 (noifr01)
**    defined WINVER for fixing various problems when building with recent
**    version(s) of compiler
** 18-Jun-2004 (schph01)
**    (SIR 111014) Delete two includes afxdb.h and afxdao.h generating some
**    build problem on certain machine.
**  20-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 18-Mar-2009 (drivi01)
**    Update WINVER in efforts to port to Visual Studio 2008.
*/

#if !defined(AFX_STDAFX_H__DAE12BD9_C91C_11D5_8786_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__DAE12BD9_C91C_11D5_8786_00C04F1F754A__INCLUDED_

#define WINVER 0x0500

#if !defined (MAINWIN)
#define DESKTOP          // Define target for Ingres compat library
#define NT_GENERIC       // Define target for Ingres compat library
#define IMPORT_DLL_DATA  // Define target for Ingres compat library
#define INGRESII         // Define target for Ingres compat library
#endif


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#include <afxctl.h>         // MFC support for ActiveX Controls
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Comon Controls
#include <afxcview.h>
#include <afxpriv.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxtempl.h>       // MFC Template classes
extern "C"
{
#include "libmon.h"
}
#include "ipmstruc.h"
#include "ipmdml.h"

#if !defined (MAINWIN)
#define _ANIMATION_         // Activate Animate Dialog if not mainwin
#endif

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__DAE12BD9_C91C_11D5_8786_00C04F1F754A__INCLUDED_)
