/*
**
**  Name: stdafx.h
**
**  Description:
**      The include file for standard system include files,
**      or project specific include files that are used frequently, but
**      are changed infrequently
**
**  History:
**      10-apr-2001 (somsa01)
**          Created.
**	23-Jan-2006 (drivi01)
**	    Included io.h header.
**	04-Jun-2010 (drivi01)
**	    Add Windows version.
*/

#if !defined(AFX_STDAFX_H__FABC208B_6DB7_4ADA_AC18_F3B2DFF8CF9E__INCLUDED_)
#define AFX_STDAFX_H__FABC208B_6DB7_4ADA_AC18_F3B2DFF8CF9E__INCLUDED_

#define WINVER 0x0500

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <io.h>
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC Automation classes
#endif // _AFX_NO_OLE_SUPPORT


#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>			// MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>			// MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__FABC208B_6DB7_4ADA_AC18_F3B2DFF8CF9E__INCLUDED_)

