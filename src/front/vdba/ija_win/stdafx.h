/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h , Header File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Precompile Header
**
** History:
**
** 22-Dec-1999 (uk$so01)
**    Created
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
** 10-Mar-2004 (schph01)
**    (BUG #111014), defined WINVER for fixing various problems when building
**    with recent version(s) of compiler
** 19-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 07-May-2009 (drivi01)
**    Define WINVER in effort to port to Visual Studio 2008.
**/

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__DA2AAD98_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__DA2AAD98_AF05_11D3_A322_00C04F1F754A__INCLUDED_

#define WINVER 0x0500

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxcview.h>
#include <afxtempl.h>
#include <afxpriv.h>
#include "constdef.h"
#include "dmlbase.h"
#include "dmluser.h"
#include "dmltable.h"
#include "ingobdml.h"
#include "sessimgr.h"
#include "tlsfunct.h"
#include "usrexcep.h"

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__DA2AAD98_AF05_11D3_A322_00C04F1F754A__INCLUDED_)
