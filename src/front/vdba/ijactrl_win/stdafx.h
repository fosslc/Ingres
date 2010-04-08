/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Precompile Header
**
** History:
**	22-Dec-1999 (uk$so01)
**	    created
**	30-oct-2001 (somsa01)
**	    Removed MFC database classes support, as it is not needed.
** 10-Mar-2004 (schph01)
**    (BUG #111014), defined WINVER for fixing various problems when building
**    with recent version(s) of compiler
** 19-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 07-May-2009 (drivi01)
**    Define WINVER in effort to port to Visual Studio 2008.
*/

#if !defined(AFX_STDAFX_H__C92B842B_B176_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__C92B842B_B176_11D3_A322_00C04F1F754A__INCLUDED_

#define WINVER 0x0500

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers

// In order to link DEBUG code with the Release C runtime, we must make MFC
// believe we are in Release mode
#ifdef _DEBUG
#define _REMEMBER_DEBUG_VERSION
#undef _DEBUG
#endif

#include <afxctl.h>         // MFC support for ActiveX Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxtempl.h>
#include "usrexcep.h"
#include <afxdtctl.h>
#include <afxpriv.h>
#include "calsctrl.h"
#include "fminifrm.h"

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C92B842B_B176_11D3_A322_00C04F1F754A__INCLUDED_)
