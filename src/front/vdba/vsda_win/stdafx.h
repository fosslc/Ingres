/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h : Header File.
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : include file for standard system include files
**               or project specific include files that are used frequently,
**               but are changed infrequently
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 10-Mar-2004 (schph01)
**    (BUG #111014), defined WINVER for fixing various problems when building
**    with recent version(s) of compiler
** 18-Jun-2004 (schph01)
**    (SIR 111014) Delete two includes afxdb.h and afxdao.h generating some
**    build problem on certain machine.
**  21-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 18-Mar-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, update WINVER.
*/

#if !defined(AFX_STDAFX_H__CC2DA2BA_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__CC2DA2BA_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_

#define WINVER 0x0500

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
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxtempl.h>       // MFC Template
#include <afxpriv.h>        // Conversion Macros
#include <afxcview.h>       // TreeView

//
// _SHOWDIFF_SYMBOL:
// Add Prefix {C, N} to the tree item text.
// C can be:
//    '+': object exist in schema1 and not exist in schema2
//    '-': object exist in schema2 and not exist in schema1
//    '*': object's name is identical (need to compare the detail and sub-branch)
//    '#': object's name and object's owner are identical (need to compare the detail and sub-branch)
//#define _SHOWDIFF_SYMBOL
#define _DISPLAY_OWNER_OBJECT     // display [owner].object
//#define _MULTIPLE_OWNERS_PREFIXED // display [owner1/owner2].object
//#define _IGNORE_STAR_DATABASE  // star database will not be compared.

#if !defined (MAINWIN)
#define _ANIMATION_         // Activate Animate Dialog if not mainwin
#endif


#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__CC2DA2BA_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
