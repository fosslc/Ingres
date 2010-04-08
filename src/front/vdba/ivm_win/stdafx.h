/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : istdafx.h : Header File.
**    Project  : IVM
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : include file for standard system include files
**               or project specific include files that are used frequently,
**               but are changed infrequently
**
** History:
**
** 14-Dec-1998 (uk$so01)
**    Created
** 27-Feb-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries.
** 10-Mar-2004 (schph01)
**    (BUG #111014), defined WINVER for fixing various problems when building
**    with recent version(s) of compiler
** 11-Aug-2004 (uk$so01)
**    SIR #109775, Additional change:
**    Add a menu item "Ingres Command Prompt" in Ivm's taskbar icon menu.
** 25-Jul-2007 (drivi01)
**    Add headers to make sure Vista changes compile.
**  20-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 19-Mar-2009 (drivi01)
**    Move fstream header includes to stdafx.h from fileioex.cpp.
*/

#if !defined(AFX_STDAFX_H__63A2E047_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
#define AFX_STDAFX_H__63A2E047_936D_11D2_A2B5_00609725DDAF__INCLUDED_

#define WINVER 0x0500

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#ifndef _WIN32_IE           // Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0500    // Change this to the appropriate value to target IE 5.0 or later.
#endif

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#ifdef NT_GENERIC
#include <fstream>
using namespace std;
#else
#include <fstream.h>
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>       // MFC Template classes
#include <afxcview.h>
#include "wmusrmsg.h"
#include "portto64.h"
#include <shellapi.h>
#include <shappmgr.h>
#include <Windows.h>

// 
// Mapping to the existing message:
#define WMUSRMSG_IVM_PAGE_UPDATING     WMUSRMSG_UPDATEDATA
#define WMUSRMSG_IVM_START_STOP        WMUSRMSG_START_STOP
#define WMUSRMSG_IVM_REMOVE_EVENT      WMUSRMSG_REMOVE_EVENT
#define WMUSRMSG_IVM_COMPONENT_CHANGED WMUSRMSG_COMPONENT_CHANGED
#define WMUSRMSG_IVM_NEW_EVENT         WMUSRMSG_NEW_EVENT
#define WMUSRMSG_IVM_GETITEMDATA       WMUSRMSG_GETITEMDATA
#define WMUSRMSG_IVM_GETHELPID         WMUSRMSG_GETHELPID
#define WMUSRMSG_IVM_MESSAGEBOX        WMUSRMSG_MESSAGEBOX

#if defined (MAINWIN)
#define NOSHELLICON
#endif


#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__63A2E047_936D_11D2_A2B5_00609725DDAF__INCLUDED_)
