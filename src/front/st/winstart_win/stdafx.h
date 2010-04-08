/*
** stdafx.h : include file for standard system include files,
**  or project specific include files that are used frequently, but
**      are changed infrequently
**
**  History:
**	19-jan-1999 (somsa01)
**	    It is unnecessary to include afxext.h .
**	25-Jul-2007 (drivi01)
**	    Added more header files to eliminate build problems.
*/

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <Prsht.h>
#endif // _AFX_NO_AFXCMN_SUPPORT
#include <afxdlgs.h>
#include <io.h>

