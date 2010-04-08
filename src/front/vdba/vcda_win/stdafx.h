/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h
**    Project  : INGRES II/ Visual Config Diff Control (vcda.ocx).
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : include file for standard system include files,
**               or project specific include files that are used frequently,
**               but are changed infrequently
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    Created
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
** 10-Mar-2004 (schph01)
**    (BUG #111014), defined WINVER for fixing various problems when building
**    with recent version(s) of compiler
** 18-Jun-2004 (schph01)
**    (SIR 111014) Delete two includes afxdb.h and afxdao.h generating some
**    build problem on certain machine.
**  21-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 18-Mar-2009 (drivi01)
**    Update WINVER in efforts to port to Visual Studio 2008.
**/

#if !defined(AFX_STDAFX_H__EAF345EE_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__EAF345EE_D514_11D6_87EA_00C04F1F754A__INCLUDED_

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

#include <afxtempl.h>       // MFC Templates
#include <afxpriv.h>        // MFC private
#include "wmusrmsg.h"


#define _CHECK_INCONSISTENCY // Check the files (config.dat and *.ii_vcda) for inconsistent lines
#define _VIRTUAL_NODE_AVAILABLE // Manage Vitual Node in the comparison.

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__EAF345EE_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
