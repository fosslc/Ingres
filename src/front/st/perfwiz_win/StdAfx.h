/*
**
**  Name: stdafx.h
**
**  Description:
**	The include file for standard system include files,
**	or project specific include files that are used frequently, but
**	are changed infrequently
**
**  History:
**	06-sep-1999 (somsa01)
**	    Created.
**	07-Aug-2009 (drivi01)
**	    Add pragma to disable warning 4996 about
**	    deprecated POSIX functions as it is a bug.
*/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning(disable: 4996)

#if !defined(AFX_STDAFX_H__C6B114DD_5BEB_11D3_B867_AA000400CF10__INCLUDED_)
#define AFX_STDAFX_H__C6B114DD_5BEB_11D3_B867_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#define VC_EXTRALEAN		/* Exclude rarely-used stuff from Windows headers */

#include <afxwin.h>		/* MFC core and standard components */
#include <afxext.h>		/* MFC extensions */
#include <afxdisp.h>		/* MFC Automation classes */
#include <afxdtctl.h>		/* MFC support for Internet Explorer 4 Common Controls */
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>		/* MFC support for Windows Common Controls */
#endif // _AFX_NO_AFXCMN_SUPPORT

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C6B114DD_5BEB_11D3_B867_AA000400CF10__INCLUDED_)
