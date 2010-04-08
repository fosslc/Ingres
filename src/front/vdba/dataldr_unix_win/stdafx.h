/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : stdafx.h : header file
**    Project  : Data Loader 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : include file for standard system include files,
**               or project specific include files that are used frequently, but
**               are changed infrequently.
**
** History:
**
** 02-Jan-2004 (uk$so01)
**    Created.
** 15-Dec-2004 (toumi01)
**    Port to Linux.
** 28-Mar-2005 (shaha03)
**    Added check for UNIX in appropriate locations to avoid
**    compilation problem on Unix platforms.	
*/


#if !defined(AFX_STDAFX_H__B1C03A17_4183_4DF6_B371_0EEE4772213B__INCLUDED_)
#define AFX_STDAFX_H__B1C03A17_4183_4DF6_B371_0EEE4772213B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif /* _MSC_VER > 1000 */

#define WIN32_LEAN_AND_MEAN  /* Exclude rarely-used stuff from Windows headers */

#if defined (LINUX)
#define UNIX
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <compat.h>

#if !defined (UNIX)
#define NT_GENERIC
#endif

#include <cm.h>
#include <st.h>
#include <si.h>

#if ( defined (LINUX) || defined(UNIX) )
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#define _ADD_NL_
#if defined (UNIX)
#include <pthread.h>
#endif

/*#define _SHOW_COPYTABLE_SYNTAX*/
#if defined (_DEBUG)
#define SHOW_TRACE 1
#else
#define SHOW_TRACE 0
#endif

#define TRACE0(sz) if (SHOW_TRACE) printf("%s", sz)
#define TRACE1(sz, p1) if (SHOW_TRACE) printf(sz, p1)
#define TRACE2(sz, p1, p2) if (SHOW_TRACE) printf(sz, p1, p2)


#endif /* !defined(AFX_STDAFX_H__B1C03A17_4183_4DF6_B371_0EEE4772213B__INCLUDED_) */
