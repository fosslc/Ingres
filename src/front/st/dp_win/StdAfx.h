/*
**
**  Name: stdafx.h
**
**  Description:
**	Include file for standard system include files, or project
**	specific include files that are used frequently, but are
**	changed infrequently.
**
**  History:
**	08-jul-1999 (somsa01)
**	    Created.
*/

#if !defined(AFX_STDAFX_H__24E6898D_339F_11D3_B855_AA000400CF10__INCLUDED_)
#define AFX_STDAFX_H__24E6898D_339F_11D3_B855_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#define VC_EXTRALEAN		/* Exclude rarely-used stuff from Windows headers */

#include <afxwin.h>		/* MFC core and standard components */
#include <afxext.h>		/* MFC extensions */
#ifndef MAINWIN
#include <afxdtctl.h>		/* MFC support for Internet Explorer 4 Common Controls */
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>		/* MFC support for Windows Common Controls */
#endif // _AFX_NO_AFXCMN_SUPPORT


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__24E6898D_339F_11D3_B855_AA000400CF10__INCLUDED_)
