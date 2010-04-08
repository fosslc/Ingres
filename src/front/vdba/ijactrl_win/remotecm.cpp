/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : remotecm.cpp, Implementation file
**    Project  : Ingres Journal Analyser 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  :  Include SQL file (remotecm.inc)

** History :
** 19-Jun-2000 (uk$so01)
**    created
**
**/

#include "stdafx.h"
#include "remotecm.h"
#include "ijadmlll.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ESQLCC_UNICODE_OK
#if defined (_UNICODE)
//
// In actual ESCLCC, we cannot use the TCHAR data type, there are some casts LPTSTR to char*
// If later, Ingres supports the UNICODE, then use the correct data type and remove the casts...
// Remove the #error macro if Ingres supports UNICODE
#error Ingres ESQLCC is not UNICODE compliant, the file "remotecm.inc" conatins the casts to char, char*
#undef ESQLCC_UNICODE_OK // Remove this line if Ingres supports UNICODE
#endif

//
// This file "" is generated from ESQLCC -multi -fremotecm.inc remotecm.scc
#if defined ESQLCC_UNICODE_OK
#include "remotecm.inc"
#endif



