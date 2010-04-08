/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h : Header File.
**    Project  : IVM
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : include file for standard system include files
**               or project specific include files that are used frequently,
**               but are changed infrequently
**
** History:
**
** xx-xxx-1997 (uk$so01)
**    Created
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries.
** 25-Sep-2003 (uk$so01)
**    SIR #105781 of cbf
** 10-Mar-2004 (schph01)
**    (BUG #111014), defined WINVER for fixing various problems when building
**    with recent version(s) of compiler
**  21-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 07-May-2009 (drivi01)
**    Define WINVER in effort to port to Visual Studio 2008.
*/

#define WINVER 0x0500

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxdisp.h>
#include <afxpriv.h>
#endif // _AFX_NO_AFXCMN_SUPPORT
#include "portto64.h"
#include "wmusrmsg.h"

#define BASEx (WM_BASE + 1000)
#define WM_CONFIGRIGHT_DLG_BUTTON1          (BASEx + 1)
#define WM_CONFIGRIGHT_DLG_BUTTON2          (BASEx + 2)
#define WM_CONFIGRIGHT_DLG_BUTTON3          (BASEx + 3)
#define WM_CONFIGRIGHT_DLG_BUTTON4          (BASEx + 4)
#define WM_CONFIGRIGHT_DLG_BUTTON5          (BASEx + 5)
#define WM_COMPONENT_EXIT                   (BASEx + 6)
#define WM_COMPONENT_ENTER                  (BASEx + 7)
#define WMUSRMSG_CBF_PAGE_VALIDATE          (BASEx + 8)
#define WMUSRMSG_CBF_PAGE_UPDATING          (BASEx + 9)


#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

