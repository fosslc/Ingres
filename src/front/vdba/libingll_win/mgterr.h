/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/
/*
**    Source   : sqlerr.h : header file.
**    Project  : INGRES II / SQL/test.
**    Author   : Schalk Philippe (schph01)
**    Purpose  : keep trace of SQL error
**
** History:
**
** 16-July-2004 (schph01)
**    Created
*/
#if !defined (MANAGE_SQL_ERROR_HEADER)
#define MANAGE_SQL_ERROR_HEADER
#include "stdafx.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

LPTSTR INGRESII_llGetErrorLogFile();
void INGRESII_ManageErrorInLogFiles(LPTSTR SqlTextErr,LPTSTR SqlStatement,int iErr);

#endif // end MANAGE_SQL_ERROR_HEADER


