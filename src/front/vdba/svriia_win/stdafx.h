/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h : include file for standard system include files
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Main dll.
**
** History:
**
** 11-Dec-2000 (uk$so01)
**    Created
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Date conversion and numeric display format.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 11-Feb-2004 (noifr01)
**    defined WINVER for fixing various problems when building with recent
**    version(s) of compiler
** 18-Jun-2004 (schph01)
**    (SIR 111014) Delete two includes afxdb.h and afxdao.h generating some
**    build problem on certain machine.
**  21-Aug-2008 (whiro01)
**    Undefine _DEBUG around MFC headers so we can always link to Release C runtime.
** 18-Mar-2009 (drivi01)
**    In efforts to port to Visual Studio 2008, update WINVER.
**    Disable warning 4996 which is a bug in a compiler about deprecated POSIX
**    functions.
**/
/* Turn off POSIX warning for this file until Microsoft fixes this bug */
#pragma warning(disable: 4996)


// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//
#if !defined(AFX_STDAFX_H__2B9B2EFE_CA8B_11D4_872E_00C04F1F754A__INCLUDED_)
#define AFX_STDAFX_H__2B9B2EFE_CA8B_11D4_872E_00C04F1F754A__INCLUDED_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#define _WIN32_DCOM

#define WINVER 0x0500

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
#include <afxtempl.h>       // MFC Template classes
#include <afxpriv.h>        // MFC Private
#include <io.h>             // _access()
#include <tchar.h>


#include "constdef.h"
#include "usrexcep.h"

//
// This is a control ID of help button (I use the SPY++ to detect it)
#define ID_W_HELP 9

//
// Define _GATEWAY_NOT_POSSIBLE to disable
// import into the gateway table
#define _GATEWAY_NOT_POSSIBLE

//
// The define CONFIRM_TABCHOICE_WITH_CHECKBOX.
// If this constant is defined then user must check this check box to confirm
// his choice before going to the STEP3. 
// If this constant is not defined, the current selected Tab is the user's choice.
// (this is very simple in implementation and "What You See Is What You Get")
#define CONFIRM_TABCHOICE_WITH_CHECKBOX

//
// CHECK_CONFIRM_CHOICE is associated with the WMUSRMSG_GETITEMDATA
// to retrieve the Page CheckBox of confirm Choice:
#define CHECK_CONFIRM_CHOICE 100


//
// The define MAXLENGTH_AND_EFFECTIVE.
// If this constant is defined then the column name will be displayed
// like col1 (n, p), col2 (n, p) where
//    n=effective length (without trailing spaces)
//    p=number of trailing space. If p = 0 then the column name is displayed like col1 (n)
// NOTE: n+p = max length.
#define MAXLENGTH_AND_EFFECTIVE

//
// Generate the SQL COPY syntax to use the callback
#define _SQLCOPY_CALLBACK

//
// _INTERRUPT_KILLTHREAD.
// When hanging the thread can be killed (It does not work properly yet)
// #define _INTERRUPT_KILLTHREAD

//
// Output trace (debug only)
#define _TRACEINFO

//
// _DISPLAY_OWNER_OBJECT.
// Define this constant to display table in the form of [Owner].Object
#define _DISPLAY_OWNER_OBJECT

//
// _SORT_TREEITEM.
// Define this constant to Sort the tree item in the ascending order of object name:
#define _SORT_TREEITEM

// _NEED_SPECIFYING_PAGESIZE, if this constant is defined, then create the table 
// on the with specifying the page size. 
//#define _NEED_SPECIFYING_PAGESIZE

#if !defined (MAINWIN)
#define _ANIMATION_         // Activate Animate Dialog if not mainwin
#endif
//#define _CHOICE_DBF_DATATYPE // (when exporting into DBF file, this const enable user to specify type)

#ifdef _REMEMBER_DEBUG_VERSION
#define _DEBUG
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__2B9B2EFE_CA8B_11D4_872E_00C04F1F754A__INCLUDED_)
