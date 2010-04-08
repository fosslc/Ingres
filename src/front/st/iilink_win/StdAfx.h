/*
**  Name: stdafx.h
**
**  Description:
**	include file for standard system include files,
**	or project specific include files that are used frequently, but
**	are changed infrequently
**
**  History:
**	30-apr-1999 (somsa01)
**	    Only include afxdtctl.h for MSVC 6.0 and above.
*/

#if !defined(AFX_STDAFX_H__E4AA1CBB_ECF9_11D2_B840_AA000400CF10__INCLUDED_)
#define AFX_STDAFX_H__E4AA1CBB_ECF9_11D2_B840_AA000400CF10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#define VC_EXTRALEAN	/* Exclude rarely-used stuff from Windows headers */

#include <afxwin.h>         /* MFC core and standard components */
#include <afxext.h>         /* MFC extensions */
#include <afxdisp.h>        /* MFC Automation classes */

#if _MSC_VER >= 6000
#include <afxdtctl.h>	/* MFC support for Internet Explorer 4 Common Controls */
#endif /* _MSC_VER >= 6000 */

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>		/* MFC support for Windows Common Controls */
#endif // _AFX_NO_AFXCMN_SUPPORT


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__E4AA1CBB_ECF9_11D2_B840_AA000400CF10__INCLUDED_)
