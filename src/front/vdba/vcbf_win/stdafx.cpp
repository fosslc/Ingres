/*
** History:
** 06-Jun-2000 (uk$so01)
**    bug 99242 Handle DBCS 
**
**/

// stdafx.cpp : source file that includes just the standard includes
//	vcbf.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information
#include "stdafx.h"

#if defined (_UNICODE)
//
// REMOVE the following macro if the ll.h and ll.cpp have used the 
// the data types with DBCS and UNICODE compliant, eg
// TCHAR   instead of char
// LPTSTR  instead of char*
// LPCTSTR instead of const char*
// ESPECIALLY, the parameters of functions VCBFll??? shoud be LPTSTR for char*  or LPCTSTR for const char*
#error  The file "ll.h" and "ll.cpp" contain the data types that are not UNOCODE compiant ... 
#endif
