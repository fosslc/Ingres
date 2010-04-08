/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h
**    Project  : INGRES II/ Vdba.
**    Author   : 
**    Purpose  : include file for standard system include files,
**               or project specific include files that are used frequently,
**               but are changed infrequently
**
** History (after 28-Mar-2003):
** 28-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
** 11-Feb-2004 (noifr01)
**    defined WINVER for fixing various problems when building with
**    recent version(s) of compiler
** 13-Jun-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 18-Mar-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, update WINVER and move
**    fstream includes from dgdpihtm.cpp to stdafx.h.
** 19-Mar-2010 (drivi01)
**    Add a header for CDialogEx.
**/


#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

#define WINVER 0x0500

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#ifdef NT_GENERIC
#include <search.h>
#include <fstream>
using namespace std;
#else
#include <fstream.h>
#endif

#include <afxdialogex.h>

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxrich.h>        // MFC rich edit classes
#include <afxtempl.h>       // MFC Class Template
#include <afxpriv.h>
#include <afxpriv2.h>
#include <afxole.h>
#include <afxadv.h>
#include <comdef.h>
#include "constdef.h"
#include "portto64.h"

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif
