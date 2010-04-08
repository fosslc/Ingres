/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : istdafx.h : Header File.
**    Project  : SQL/Test (stand alone)
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : include file for standard system include files
**               or project specific include files that are used frequently,
**               but are changed infrequently
**
** History:
**
** 10-Dec-2001 (uk$so01)
**    Created
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 10-Mar-2004 (schph01)
**    (BUG #111014), defined WINVER for fixing various problems when building
**    with recent version(s) of compiler
** 02-Aug-2004 (uk$so01)
**    BUG #112765, ISSUE 13364531 (Activate the print of sql test)
**  21-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 18-Mar-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, update WINVER.
*/


#if !defined(AFX_STDAFX_H__1D04F616_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__1D04F616_EFC9_11D5_8795_00C04F1F754A__INCLUDED_

#define WINVER 0x0500

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _TOOTBAR_HOT_IMAGE_ // Toolbar has hot images
#define _TOOTBAR_BTN_TEXT_  // Toolbar has text under the button
#define _PRINT_ENABLE_ //Only if allow printing...
//#define _SHOW_INDICATOR_READLOCK_
#define _PERSISTANT_STATE_  // Save the persistant state of application
#if !defined (EDBC)
#define _OPTION_GROUPxROLE  // Add combo Group, Role 
#endif

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
#include <comdef.h>
#include <afxpriv.h>

#define _NO_OPENSAVE
//#define _MINI_CACHE
#define REFRESH_NODE     0x00000001
#define REFRESH_SERVER   0x00000002
#define REFRESH_USER     0x00000004
#define REFRESH_DATABASE 0x00000008
#define REFRESH_GROUP    0x00000010
#define REFRESH_ROLE     0x00000020

#define MASK_UNKNOWN     0x00000001
#define MASK_SETNODE     0x00000002
#define MASK_SETSERVER   0x00000004
#define MASK_SETUSER     0x00000008
#define MASK_SETDATABASE 0x00000010
#define MASK_SETGROUP    0x00000020
#define MASK_SETROLE     0x00000040
#define MASK_SETCONNECT  0x00000080

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__1D04F616_EFC9_11D5_8795_00C04F1F754A__INCLUDED_)
