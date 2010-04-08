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
** 10-Dec-2001 (uk$so01)
**    Created
** 11-Feb-2004 (noifr01)
**    defined WINVER for fixing GPF when exiting vdbamon in
**    release mode
**  21-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 18-Mar-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, update WINVER.
*/

#if !defined(AFX_STDAFX_H__AE712EF6_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__AE712EF6_E8C7_11D5_8792_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#if !defined (MAINWIN)
#define DESKTOP          // Define target for Ingres compat library
#define NT_GENERIC       // Define target for Ingres compat library
#define IMPORT_DLL_DATA  // Define target for Ingres compat library
#define INGRESII         // Define target for Ingres compat library
#endif
#define WINVER 0x0500
//#define _PRINT_ENABLE_ Only if allow printing...
#define _TOOTBAR_HOT_IMAGE_ // Toolbar has hot images
#define _TOOTBAR_BTN_TEXT_  // Toolbar has text under the button
#define _INITIATE_CONTROL_ON_CREATE // Connect to the DBMS on creation
#define _PERSISTANT_STATE_  // Save the persistant state of application
#define _MIN_TOOLBARS_      // Management of minumum toolbars (invoked by command line args)

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers


// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>       // MFC Template classes
#include <afxole.h>
#include <afxpriv.h>


#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__AE712EF6_E8C7_11D5_8792_00C04F1F754A__INCLUDED_)
