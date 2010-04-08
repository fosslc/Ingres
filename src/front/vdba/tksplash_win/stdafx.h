/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h : include file for standard system include files
**    Project  : Extension DLL (Task Animation).
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Precompile Header
**               include file for standard system include files
**               or project specific include files that are used frequently, but
**               are changed infrequently.
**
**  History:
**
**  06-Nov-2001 (uk$so01)
**     created
**  09-Mar-2004 (noifr01)
**    defined WINVER for fixing various problems when building with recent
**    version(s) of compiler
** 18-Jun-2004 (schph01)
**    (SIR 111014) Delete two includes afxdb.h and afxdao.h generating some
**    build problem on certain machine.
** 05-Oct-2004 (uk$so01)
**    BUG #113185
**    Manage the Prompt for password for the Ingres DBMS user that 
**    has been created with the password when connecting to the DBMS.
**  20-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 07-May-2009 (drivi01)
**    Define WINVER in effort to port to Visual Studio 2008.
*/

#if !defined(AFX_STDAFX_H__71950978_D2C6_11D5_8788_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__71950978_D2C6_11D5_8788_00C04F1F754A__INCLUDED_

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

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT


#include <afxdtctl.h>       // MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>         // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxpriv.h>        // MFC support for Windows Common Controls
#include <afxtempl.h>       // MFC Template classes


#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__71950978_D2C6_11D5_8788_00C04F1F754A__INCLUDED_)
